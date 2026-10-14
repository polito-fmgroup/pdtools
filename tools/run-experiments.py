#!/usr/bin/env python3
"""Run pdtrav / pdtapp over a set of benchmarks and strategies.

An experiment is described in a small TOML (or JSON) file: the benchmarks, the
strategies (sequences of commands with placeholders) and where to put the
results. This script expands the benchmark x strategy (x parameter) matrix,
runs every combination in its own directory, keeps one log per run and writes a
summary table.

    tools/run-experiments.py --new my-experiment.toml     # start from the template
    tools/run-experiments.py my-experiment.toml --dry-run # show what would run
    tools/run-experiments.py my-experiment.toml -j 4      # run, 4 runs at a time

It can be run from any directory. Relative paths in the experiment file
(benchmarks, tools, output) are relative to the directory of the file, so an
experiment keeps everything in its own folder; {here} stands for that directory
and {repo} for the root of this repository. Paths given on the command line are
relative to the current directory. The template (tools/experiments/template.toml)
documents every setting.

Results are written to <output>/<name>/, by default results/<name>/ next to the
experiment file:

    manifest.json   how and when the experiment was run
    runner.log      the progress messages of the runner
    experiment.*    a copy of the experiment file
    summary.csv     one row per run (also summary.json)
    <strategy>/<benchmark>/run.log       output of all the steps of the run
    <strategy>/<benchmark>/result.json   status, timing and extracted values

Needs Python 3.9 or newer; TOML files need 3.11 or newer (JSON files work on
older versions).
"""

import argparse
import concurrent.futures
import csv
import datetime
import fnmatch
import glob
import itertools
import json
import os
import platform
import re
import shlex
import shutil
import signal
import subprocess
import sys
import threading
import time
from dataclasses import dataclass, field
from pathlib import Path

try:
    import tomllib
except ImportError:  # Python < 3.11
    tomllib = None

SCRIPT = Path(__file__).resolve()
REPO = SCRIPT.parent.parent
TEMPLATE = SCRIPT.parent / "experiments" / "template.toml"

# Names that experiment files may not redefine as variables or parameters.
RESERVED = ("benchmark", "name", "strategy", "outdir", "repo", "here")

# Values extracted from every run's log into summary.csv. An [[extract]] entry
# of the experiment file with the same name replaces a default.
DEFAULT_EXTRACT = [
    {
        "name": "verdict",
        "regex": r"(?:Invariant Verification|Verification Result): (PASS|FAIL)"
                 r"|Verification (PASSED|FAILED)",
    },
]
VERDICT_ALIASES = {"PASSED": "PASS", "FAILED": "FAIL"}


def die(message):
    sys.exit(f"error: {message}")


def sanitize(text):
    """Make a string usable as a file or directory name."""
    return re.sub(r"[^A-Za-z0-9._=+-]+", "_", str(text)).strip("_") or "_"


def as_list(value):
    return [value] if isinstance(value, str) else list(value or [])


def show(value):
    return "+".join(map(str, value)) if isinstance(value, list) else str(value)


def fmt(text, context, where):
    """Substitute {placeholders} in a string; report unknown ones clearly."""
    try:
        return text.format_map(context)
    except KeyError as exc:
        die(f"{where}: unknown placeholder {{{exc.args[0]}}} "
            f"(available: {', '.join(sorted(context))})")
    except (ValueError, IndexError, AttributeError) as exc:
        die(f"{where}: bad placeholder in '{text}' ({exc}); write {{{{ and }}}} for literal braces")


def parse_value(text):
    """Interpret a command-line value: JSON if it parses (numbers, lists), else text."""
    try:
        return json.loads(text)
    except ValueError:
        return text


# ---------------------------------------------------------------------------
# Experiment file
# ---------------------------------------------------------------------------

def load_config(path):
    path = Path(path)
    if not path.is_file():
        die(f"experiment file '{path}' not found")
    raw = path.read_bytes()
    if path.suffix == ".json":
        try:
            return json.loads(raw)
        except ValueError as exc:
            die(f"{path}: {exc}")
    if path.suffix == ".toml":
        if tomllib is None:
            die("reading TOML needs Python 3.11 or newer; use a .json file or a newer Python")
        try:
            return tomllib.loads(raw.decode("utf-8"))
        except tomllib.TOMLDecodeError as exc:
            die(f"{path}: {exc}")
    die(f"{path}: unsupported file type (use .toml or .json)")


def resolve_tool(value, base_dir):
    """Absolute path of a program. A value containing a directory separator is a path,
    relative to base_dir; a bare name is looked up on PATH (so 'pdtrav' never means a
    directory called pdtrav; write './pdtrav' for a file next to the experiment)."""
    if os.sep in value or (os.altsep and os.altsep in value) or Path(value).is_absolute():
        return os.path.abspath(base_dir / value)
    return shutil.which(value) or value


@dataclass
class Settings:
    name: str
    out: Path
    timeout: float
    jobs: int
    tools: dict
    variables: dict
    extract: list
    config_path: Path
    here: Path
    context: dict


def build_settings(cfg, args, here):
    cfg_path = Path(args.experiment)
    base = {"repo": str(REPO), "here": str(here)}
    name = sanitize(args.name or cfg.get("name") or cfg_path.stem)
    timeout = args.timeout if args.timeout is not None else float(cfg.get("timeout", 3600))
    jobs = args.jobs or int(cfg.get("jobs", 1))
    if jobs < 1:
        die("the number of jobs must be at least 1")

    variables = dict(cfg.get("vars", {}))
    for item in args.set:
        key, sep, value = item.partition("=")
        if not sep:
            die(f"--set expects KEY=VALUE, got '{item}'")
        variables[key] = parse_value(value)
    for key, value in list(variables.items()):
        if isinstance(value, str):
            variables[key] = fmt(value, base, f"variable '{key}'")
    context = {**base, **variables}

    tools = {}
    for tool in ("pdtrav", "pdtapp"):
        built = REPO / "pdtrav" / "bin" / tool
        tools[tool] = shutil.which(tool) or (str(built) if built.exists() else tool)
    for key, value in cfg.get("tools", {}).items():
        tools[key] = resolve_tool(fmt(str(value), context, f"[tools] {key}"), here)
    for item in args.tool:
        key, sep, value = item.partition("=")
        if not sep:
            die(f"--tool expects NAME=PATH, got '{item}'")
        tools[key] = resolve_tool(value, Path.cwd())
    for key in list(variables) + list(tools):
        if key in RESERVED:
            die(f"'{key}' is reserved and cannot be used as a variable or tool name")
    clash = set(variables) & set(tools)
    if clash:
        die(f"names used both as a tool and a variable: {', '.join(sorted(clash))}")

    if args.output:
        out_base = Path(args.output)
        out_base = out_base if out_base.is_absolute() else Path.cwd() / out_base
    else:
        out_base = Path(fmt(str(cfg.get("output", "results")), context, "output"))
        out_base = out_base if out_base.is_absolute() else here / out_base

    rules = {rule["name"]: rule for rule in DEFAULT_EXTRACT}
    for rule in cfg.get("extract", []):
        if "name" not in rule or "regex" not in rule:
            die("every [[extract]] entry needs 'name' and 'regex'")
        rules[rule["name"]] = rule
    for rule in rules.values():
        try:
            re.compile(rule["regex"])
        except re.error as exc:
            die(f"[[extract]] '{rule['name']}': bad regular expression ({exc})")

    return Settings(name, Path(os.path.abspath(out_base)) / name, timeout, jobs, tools,
                    variables, list(rules.values()), cfg_path, here, context)


# ---------------------------------------------------------------------------
# Benchmarks and strategies
# ---------------------------------------------------------------------------

def find_benchmarks(cfg, settings):
    section = cfg.get("benchmarks") or {}
    def expand(pattern):
        text = fmt(str(pattern), settings.context, "[benchmarks]")
        return str(settings.here / text)    # an absolute pattern stays as it is

    patterns = [expand(p) for p in as_list(section.get("paths"))]
    if not patterns:
        die("the experiment file lists no benchmarks ([benchmarks] paths = [...])")
    excluded = set()
    for pattern in map(expand, as_list(section.get("exclude"))):
        excluded.update(Path(m).resolve() for m in glob.glob(pattern, recursive=True))
    files = []
    for pattern in patterns:
        matches = sorted(m for m in glob.glob(pattern, recursive=True) if os.path.isfile(m))
        if not matches:
            print(f"warning: no benchmark matches '{pattern}'", file=sys.stderr)
        for match in matches:
            path = Path(match).resolve()
            if path not in excluded and path not in files:
                files.append(path)
    if not files:
        die("no benchmark files found (relative patterns are relative to the directory "
            f"of the experiment file, {settings.here})")
    stems = [f.stem for f in files]
    named = []
    for f in files:
        name = f.stem if stems.count(f.stem) == 1 else f"{f.parent.name}__{f.stem}"
        named.append((sanitize(name), f))
    names = [n for n, _ in named]
    if len(set(names)) != len(names):
        die("two benchmarks share the same name; rename one or narrow the paths")
    return named


@dataclass
class Strategy:
    name: str
    params: dict
    spec: list
    timeout: float
    only: list = field(default_factory=list)


def build_strategies(cfg, default_timeout):
    entries = cfg.get("strategy") or []
    if not entries:
        die("the experiment file defines no strategy ([[strategy]])")
    strategies, seen = [], set()
    for entry in entries:
        name = entry.get("name")
        if not name:
            die("every [[strategy]] needs a 'name'")
        if ("steps" in entry) == ("args" in entry):
            die(f"strategy '{name}': give exactly one of 'steps' and 'args'")
        if "args" in entry:
            args = entry["args"]
            spec = [("{pdtrav} " + args) if isinstance(args, str) else ["{pdtrav}"] + list(args)]
        else:
            spec = list(entry["steps"])
        params = entry.get("params", {})
        for key, values in params.items():
            if key in RESERVED:
                die(f"strategy '{name}': '{key}' is reserved and cannot be a parameter")
            if not isinstance(values, list) or not values:
                die(f"strategy '{name}': parameter '{key}' must be a non-empty list")
        timeout = float(entry.get("timeout", default_timeout))
        only = as_list(entry.get("only"))
        keys = list(params)
        for combo in itertools.product(*(params[k] for k in keys)):
            chosen = dict(zip(keys, combo))
            variant = name + "".join(f"-{k}={show(v)}" for k, v in chosen.items())
            variant = sanitize(variant)
            if variant in seen:
                die(f"duplicate strategy name '{variant}'")
            seen.add(variant)
            strategies.append(Strategy(variant, chosen, spec, timeout, only))
    return strategies


def expand_token(token, context, where):
    """Substitute placeholders in one argument; a list-valued {x} becomes several."""
    whole = re.fullmatch(r"\{(\w+)\}", token)
    if whole and isinstance(context.get(whole.group(1)), list):
        return [str(item) for item in context[whole.group(1)]]
    return [fmt(token, context, where)]


@dataclass
class Run:
    strategy: str
    params: dict
    benchmark: Path
    name: str
    outdir: Path
    steps: list
    timeout: float


def plan_runs(settings, benchmarks, strategies):
    runs = []
    for strategy in strategies:
        for name, path in benchmarks:
            if strategy.only and not any(fnmatch.fnmatch(name, p) for p in strategy.only):
                continue
            outdir = settings.out / sanitize(strategy.name) / name
            context = {**settings.variables, **strategy.params, **settings.tools,
                       "benchmark": str(path), "name": name, "strategy": strategy.name,
                       "outdir": str(outdir), "repo": str(REPO),
                       "here": str(settings.here)}
            steps = []
            for number, spec in enumerate(strategy.spec, 1):
                where = f"strategy '{strategy.name}', step {number}"
                tokens = shlex.split(spec) if isinstance(spec, str) else [str(t) for t in spec]
                command = []
                for token in tokens:
                    command += expand_token(token, context, where)
                steps.append(command)
            runs.append(Run(strategy.name, strategy.params, path, name, outdir, steps,
                            strategy.timeout))
    return runs


# ---------------------------------------------------------------------------
# Running
# ---------------------------------------------------------------------------

RUNNING = set()
LOCK = threading.Lock()
STOP = threading.Event()
RUNNER_LOG = {"path": None}


def say(message, stream=None):
    """Print a progress message and keep it in runner.log of the experiment."""
    print(message, file=stream or sys.stdout, flush=True)
    if RUNNER_LOG["path"]:
        with open(RUNNER_LOG["path"], "a", encoding="utf-8") as handle:
            handle.write(f"{datetime.datetime.now():%Y-%m-%d %H:%M:%S} {message}\n")


def signal_group(proc, sig):
    if os.name == "posix":
        try:
            os.killpg(proc.pid, sig)
        except ProcessLookupError:
            pass
    elif sig == signal.SIGKILL:
        proc.kill()
    else:
        proc.terminate()


def terminate(proc):
    """Stop a process and everything it started: ask first, then kill."""
    for sig in (signal.SIGTERM, signal.SIGKILL):
        signal_group(proc, sig)
        try:
            proc.wait(timeout=5)
            return
        except subprocess.TimeoutExpired:
            continue


def kill_all(grace=3.0):
    """Stop every running command: ask first, then kill whatever is left after 'grace' seconds."""
    with LOCK:
        procs = list(RUNNING)
    for proc in procs:
        signal_group(proc, signal.SIGTERM)
    deadline = time.monotonic() + grace
    while time.monotonic() < deadline and any(p.poll() is None for p in procs):
        time.sleep(0.05)
    for proc in procs:
        if proc.poll() is None:
            signal_group(proc, signal.SIGKILL)


def run_step(command, log, timeout, cwd):
    """Run one command with its output appended to 'log'. Returns (status, exit code)."""
    options = {"start_new_session": True} if os.name == "posix" else {}
    try:
        proc = subprocess.Popen(command, stdout=log, stderr=subprocess.STDOUT,
                                stdin=subprocess.DEVNULL, cwd=cwd, **options)
    except OSError as exc:
        log.write(f"[cannot start the command: {exc}]\n")
        return "error", None
    with LOCK:
        RUNNING.add(proc)
    try:
        try:
            proc.wait(timeout=timeout or None)
        except subprocess.TimeoutExpired:
            terminate(proc)
            return "timeout", proc.returncode
        return ("ok" if proc.returncode == 0 else "fail"), proc.returncode
    finally:
        with LOCK:
            RUNNING.discard(proc)


def extract_values(text, rules):
    values = {}
    for rule in rules:
        last = None
        for last in re.finditer(rule["regex"], text, re.M):
            pass
        if last is None:
            values[rule["name"]] = ""
            continue
        groups = [g for g in last.groups() if g is not None]
        value = groups[0] if groups else "yes"
        values[rule["name"]] = VERDICT_ALIASES.get(value, value)
    return values


def execute(run, settings):
    if STOP.is_set():
        return None
    run.outdir.mkdir(parents=True, exist_ok=True)
    log_path = run.outdir / "run.log"
    started = datetime.datetime.now().isoformat(timespec="seconds")
    begin = time.monotonic()
    status, codes = "ok", []
    with open(log_path, "w", encoding="utf-8", errors="replace") as log:
        for number, command in enumerate(run.steps, 1):
            if STOP.is_set():
                status = "interrupted"
                break
            log.write(f"$ {shlex.join(command)}\n")
            log.flush()
            step_begin = time.monotonic()
            step_status, code = run_step(command, log, run.timeout, run.outdir)
            if STOP.is_set() and step_status != "ok":
                step_status = "interrupted"
            log.write(f"\n[step {number}: {step_status}, exit code {code}, "
                      f"{time.monotonic() - step_begin:.2f} s]\n\n")
            log.flush()
            codes.append(code)
            if step_status != "ok":
                status = step_status
                break
    text = log_path.read_text(encoding="utf-8", errors="replace")
    result = {
        "strategy": run.strategy,
        "params": run.params,
        "benchmark": str(run.benchmark),
        "name": run.name,
        "status": status,
        "seconds": round(time.monotonic() - begin, 3),
        "exit_codes": codes,
        "started": started,
        "commands": [shlex.join(c) for c in run.steps],
        "outdir": str(run.outdir),
        "values": extract_values(text, settings.extract),
    }
    (run.outdir / "result.json").write_text(json.dumps(result, indent=2) + "\n")
    return result


def load_finished(run):
    """The saved result of a run that already completed successfully, if any."""
    path = run.outdir / "result.json"
    if path.is_file():
        try:
            result = json.loads(path.read_text())
        except ValueError:
            return None
        if result.get("status") == "ok":
            return result
    return None


# ---------------------------------------------------------------------------
# Reporting
# ---------------------------------------------------------------------------

def tool_version(path):
    try:
        out = subprocess.run([path, "-version"], capture_output=True, text=True,
                             timeout=10, stdin=subprocess.DEVNULL).stdout
    except (OSError, subprocess.TimeoutExpired):
        return None
    match = re.search(r"Tool Version: (.*)", out)
    return match.group(1).strip() if match else None


def collect_results(settings):
    """Every saved result in the experiment directory, so that the summary also covers
    runs of earlier invocations (for example before a filtered --resume)."""
    results = []
    for path in sorted(settings.out.glob("*/*/result.json")):
        try:
            result = json.loads(path.read_text())
        except ValueError:
            continue
        result["outdir"] = str(path.parent)
        results.append(result)
    return sorted(results, key=lambda r: (r["strategy"], r["name"]))


def write_summary(settings, results):
    extra = [rule["name"] for rule in settings.extract]
    columns = ["strategy", "benchmark", "params", "status", "seconds"] + extra + ["outdir"]
    with open(settings.out / "summary.csv", "w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=columns, lineterminator="\n")
        writer.writeheader()
        for r in results:
            row = {
                "strategy": r["strategy"],
                "benchmark": r["name"],
                "params": " ".join(f"{k}={show(v)}" for k, v in r["params"].items()),
                "status": r["status"],
                "seconds": r["seconds"],
                "outdir": os.path.relpath(r["outdir"], settings.out),
            }
            row.update({name: r["values"].get(name, "") for name in extra})
            writer.writerow(row)
    (settings.out / "summary.json").write_text(json.dumps(results, indent=2) + "\n")


def print_overview(results):
    rows = {}
    for r in results:
        row = rows.setdefault(r["strategy"], {})
        for key in (r["status"], "verdict:" + r["values"].get("verdict", "")):
            row[key] = row.get(key, 0) + 1
        row["runs"] = row.get("runs", 0) + 1
    width = max(len(s) for s in rows)
    print(f"\n{'strategy':{width}}  runs  ok  fail  timeout  error  PASS  FAIL")
    for strategy, row in rows.items():
        print(f"{strategy:{width}}  {row['runs']:4d}  {row.get('ok', 0):2d}  "
              f"{row.get('fail', 0):4d}  {row.get('timeout', 0):7d}  {row.get('error', 0):5d}  "
              f"{row.get('verdict:PASS', 0):4d}  {row.get('verdict:FAIL', 0):4d}")


def main(argv=None):
    parser = argparse.ArgumentParser(
        description="Run pdtrav / pdtapp over a set of benchmarks and strategies.",
        epilog="See tools/experiments/template.toml for every setting.")
    parser.add_argument("experiment", nargs="?", help="experiment file (.toml or .json)")
    parser.add_argument("--new", metavar="FILE", help="write a copy of the template to FILE and exit")
    parser.add_argument("-j", "--jobs", type=int, help="runs executed in parallel")
    parser.add_argument("-o", "--output", help="base output directory")
    parser.add_argument("--name", help="experiment name (output subdirectory)")
    parser.add_argument("--timeout", type=float, help="seconds allowed for each step (0: no limit)")
    parser.add_argument("--tool", action="append", default=[], metavar="NAME=PATH",
                        help="override a program, e.g. --tool pdtrav=build2/pdtrav/bin/pdtrav")
    parser.add_argument("--set", action="append", default=[], metavar="KEY=VALUE",
                        help="set or override a variable of the experiment file")
    parser.add_argument("--benchmark", action="append", default=[], metavar="PATTERN",
                        help="only benchmarks whose name matches (wildcards allowed)")
    parser.add_argument("--strategy", action="append", default=[], metavar="PATTERN",
                        help="only strategies whose name matches (wildcards allowed)")
    parser.add_argument("--list", action="store_true", help="list the runs and exit")
    parser.add_argument("--dry-run", action="store_true", help="show the commands without running them")
    parser.add_argument("--resume", action="store_true",
                        help="reuse the results of runs that already finished successfully")
    parser.add_argument("--overwrite", action="store_true",
                        help="reuse an existing output directory, replacing the results of the runs")
    parser.add_argument("--summarize", action="store_true",
                        help="rebuild summary.csv from the saved results and exit "
                             "(for example after an experiment was killed)")
    args = parser.parse_args(argv)

    if args.new:
        target = Path(args.new)
        if target.exists():
            die(f"'{target}' already exists")
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(TEMPLATE, target)
        print(f"wrote {target}; edit it, then run: {Path(sys.argv[0]).name} {target} --dry-run")
        return 0
    if not args.experiment:
        parser.error("an experiment file is required (or use --new FILE)")

    cfg = load_config(args.experiment)
    here = Path(os.path.abspath(args.experiment)).parent
    settings = build_settings(cfg, args, here)
    if args.summarize:
        everything = collect_results(settings)
        if not everything:
            die(f"no results found in {settings.out}")
        write_summary(settings, everything)
        print_overview(everything)
        print(f"\nsummary rebuilt: {settings.out / 'summary.csv'}  ({len(everything)} runs)")
        return 0
    benchmarks = find_benchmarks(cfg, settings)
    strategies = build_strategies(cfg, settings.timeout)
    if args.benchmark:
        benchmarks = [b for b in benchmarks if any(fnmatch.fnmatch(b[0], p) for p in args.benchmark)]
    if args.strategy:
        strategies = [s for s in strategies if any(fnmatch.fnmatch(s.name, p) for p in args.strategy)]
    runs = plan_runs(settings, benchmarks, strategies)
    if not runs:
        die("nothing to run after applying the filters")

    if args.list:
        for run in runs:
            print(f"{run.strategy}\t{run.name}")
        print(f"{len(runs)} runs", file=sys.stderr)
        return 0
    tool_list = ", ".join(f"{k}={v}" for k, v in settings.tools.items())
    if args.dry_run:
        print(f"tools: {tool_list}")
        for run in runs:
            print(f"[{run.strategy} / {run.name}]  (timeout {run.timeout:g} s)")
            for command in run.steps:
                print(f"  $ {shlex.join(command)}")
        print(f"{len(runs)} runs", file=sys.stderr)
        return 0

    manifest = settings.out / "manifest.json"
    if manifest.exists() and not (args.resume or args.overwrite):
        die(f"results already exist in {settings.out}; use --resume to continue, --overwrite to "
            f"replace them, or --name / --output for a new directory")
    settings.out.mkdir(parents=True, exist_ok=True)
    version = tool_version(settings.tools.get("pdtrav", "pdtrav"))
    manifest.write_text(json.dumps({
        "experiment": settings.name,
        "started": datetime.datetime.now().isoformat(timespec="seconds"),
        "command": sys.argv,
        "directory": os.getcwd(),
        "python": platform.python_version(),
        "platform": platform.platform(),
        "tools": settings.tools,
        "pdtrav_version": version,
        "experiment_file": str(Path(os.path.abspath(settings.config_path))),
        "runs": len(runs),
        "jobs": settings.jobs,
    }, indent=2) + "\n")
    shutil.copyfile(settings.config_path, settings.out / ("experiment" + settings.config_path.suffix))
    RUNNER_LOG["path"] = settings.out / "runner.log"
    say(f"experiment '{settings.name}': {len(runs)} runs, {settings.jobs} at a time, "
        f"results in {settings.out}")
    say(f"tools: {tool_list}" + (f"  (pdtrav: {version})" if version else ""))

    results, finished, interrupted = {}, 0, False
    total = len(runs)

    def task(run):
        previous = load_finished(run) if args.resume else None
        return (previous, True) if previous else (execute(run, settings), False)

    executor = concurrent.futures.ThreadPoolExecutor(max_workers=settings.jobs)
    futures = {executor.submit(task, run): run for run in runs}
    try:
        for future in concurrent.futures.as_completed(futures):
            run = futures[future]
            result, reused = future.result()
            if result is None:
                continue
            results[id(run)] = result
            finished += 1
            verdict = result["values"].get("verdict", "")
            note = " (reused)" if reused else ""
            say(f"[{finished}/{total}] {run.strategy} / {run.name}: "
                f"{result['status']} {verdict} {result['seconds']:.1f} s{note}")
    except KeyboardInterrupt:
        interrupted = True
        STOP.set()
        say("interrupted: stopping the running commands", sys.stderr)
        try:
            kill_all()
        except KeyboardInterrupt:        # a second Ctrl-C: do not wait
            kill_all(grace=0)
    finally:
        executor.shutdown(wait=True, cancel_futures=True)

    ordered = [results[id(run)] for run in runs if id(run) in results]
    if ordered:
        everything = collect_results(settings)
        write_summary(settings, everything)
        print_overview(everything)
        print(f"\nresults: {settings.out}  ({len(everything)} runs in summary.csv)")
    if interrupted:
        return 130
    return 0 if all(r["status"] == "ok" for r in ordered) and len(ordered) == total else 1


if __name__ == "__main__":
    sys.exit(main())
