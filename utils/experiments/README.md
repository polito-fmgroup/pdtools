# Experiment runner

`utils/run-experiments.py` runs `pdtrav` and `pdtapp` over many benchmarks and strategies and collects the results. An experiment is described in a small file: the benchmarks, the strategies, the limits and where the results go. The runner expands the benchmark × strategy matrix, runs every combination in its own directory with its own log, and writes a summary table.

It is meant as a starting point to facilitate setting up the runs, applying a timeout and a memory limit, keeping the logs, and finding the results afterwards.

## Contents

- [Requirements](#requirements)
- [Quick start](#quick-start)
- [Concepts](#concepts)
- [The experiment file](#the-experiment-file)
- [Command line](#command-line)
- [Results](#results)
- [Timeouts and memory limits](#timeouts-and-memory-limits)
- [Resuming, re-running and stopping](#resuming-re-running-and-stopping)
- [Recipes](#recipes)
- [Troubleshooting](#troubleshooting)

## Requirements

- Python 3.9 or newer. The runner uses only the standard library.
- TOML experiment files need Python 3.11 or newer. On 3.9 and 3.10 use JSON files, which have the same structure (see [The experiment file](#the-experiment-file)).
- Memory limits and memory measurements need Linux. Everything else uses only POSIX features. The runner is tested on Linux.

## Quick start

In the examples the script is called `run-experiments.py`. From the repository root use `utils/run-experiments.py`, or put a symlink to it on your `PATH`; the script finds its repository through the symlink.

Create the folder of an experiment from the template, look at what would run, then run it:

```bash
run-experiments.py --new /data/exp42/experiment.toml
run-experiments.py /data/exp42/experiment.toml --dry-run
run-experiments.py /data/exp42/experiment.toml -j 4
```

`--dry-run` prints the programs in use and the commands of every run, without running anything:

```
tools: pdtrav=/usr/local/bin/pdtrav, pdtapp=/usr/local/bin/pdtapp
[ic3-certify / mod3]  (timeout 60 s)
  $ /usr/local/bin/pdtrav -strategy ic3 -verbosity 1 -writeInvar /data/exp42/results/template/ic3-certify/mod3/invar.aig /repo/utils/examples/mod3.aag
  $ /usr/local/bin/pdtapp certify /repo/utils/examples/mod3.aag /data/exp42/results/template/ic3-certify/mod3/invar.aig
[bmc-k=5 / mod3]  (timeout 60 s)
  $ /usr/local/bin/pdtrav -bmc 5 /repo/utils/examples/mod3.aag
2 runs
```

A real run prints one line per finished run and a summary of each strategy:

```
experiment 'template': 8 runs, 2 at a time, results in /data/exp42/results/template
tools: pdtrav=/usr/local/bin/pdtrav, pdtapp=/usr/local/bin/pdtapp  (pdtrav: PdTRAV 3.5.0)
[1/8] bdd-forward / mod3: ok PASS 1.5 s
[2/8] bdd-forward / cnt1: ok FAIL 1.9 s
...
strategy     runs  ok  fail  timeout  memout  error  PASS  FAIL
bdd-forward     3   3     0        0       0      0     2     1
bmc-k=5         3   3     0        0       0      0     0     0
ic3-certify     2   2     0        0       0      0     2     0

results: /data/exp42/results/template  (8 runs in summary.csv)
```

As it comes, the template runs on the example models in `utils/examples`. Edit the copy to point at your own benchmarks and strategies. [`template.toml`](template.toml) documents every setting.

## Concepts

- **Experiment file**: the description of an experiment. Keep one per experiment, in the experiment's own folder.
- **Benchmark**: an input file for `pdtrav`, found by file name patterns. The benchmarks can live anywhere and are never copied.
- **Strategy**: a list of commands with placeholders, for example "run IC3, then certify the invariant". A strategy with parameters stands for several strategies, one per combination of parameter values.
- **Step**: one command of a strategy.
- **Run**: one strategy applied to one benchmark. Each run has its own directory, which is the working directory of its commands.

## The experiment file

The file is TOML. A JSON file with the same structure works too: tables become objects, and the `[[strategy]]` and `[[extract]]` entries become arrays named `strategy` and `extract`.

### Settings

These keys sit at the top of the file. All are optional.

| Key | Default | Meaning |
|-----|---------|---------|
| `name` | the file name without extension | Name of the experiment; results go to `<output>/<name>/`. |
| `output` | `results` | Base output directory. |
| `timeout` | 3600 | Seconds allowed for each step. `0` means no limit. |
| `memory` | no limit | Memory limit for each step: a number of gigabytes, or a string such as `"512M"` or `"32G"`. |
| `jobs` | 1 | Number of runs executed in parallel. |

### Paths

Relative paths in the file (`output`, benchmark patterns, programs) are relative to the folder that contains the file, so an experiment keeps its definition and its results together and can be run from any directory. Two names are available in paths, variables and commands:

- `{here}`: the folder of the experiment file.
- `{repo}`: the root of the pdtools repository.

Paths given on the command line (`-o`, `--tool`) are relative to the current directory instead, as for any shell command.

### `[tools]`

The programs that commands can use. Every entry becomes a placeholder: `{pdtrav}`, `{pdtapp}` and any name you add. By default `pdtrav` and `pdtapp` are the ones found on `PATH`, otherwise the build in this repository. The runner prints the programs it uses when it starts and records them in `manifest.json`.

A value without a directory separator is looked up on `PATH`. A value with one is a path, relative to the experiment file; write `./tool` for a file next to it.

```toml
[tools]
pdtrav = "/opt/pdtools/pdtrav/bin/pdtrav"
analyze = "./analyze.sh"
```

### `[vars]`

Your own values, available as `{key}` in commands and in the paths of the file. String values may use `{here}` and `{repo}`. Override a value from the command line with `--set key=value`.

```toml
[vars]
verbosity = 1
benchmarks = "/data/hwmcc"
```

### `[benchmarks]`

| Key | Meaning |
|-----|---------|
| `paths` | File names or glob patterns. `**` matches any number of directories. |
| `exclude` | Patterns to leave out. |

```toml
[benchmarks]
paths = ["{benchmarks}/*.aig", "{here}/extra/*.aag"]
exclude = ["{benchmarks}/huge*.aig"]
```

A benchmark is named after its file name without the extension, with characters other than letters, digits and `._=+-` replaced by `_`. If two benchmarks would get the same name, the name of their directory is added in front (`a__model`).

### `[[extract]]`

Extra columns for `summary.csv`. Each entry has a `name` and a `regex`. The value is the first capture group of the last match of the regular expression in the log of the run, or `yes` if the expression has no group and matches. The column is empty when nothing matches.

```toml
[[extract]]
name = "certified"
regex = "Check Done for init proved"
```

The column `verdict` always exists. Its default expression finds the verdict of `pdtrav` (`PASS` or `FAIL`). An entry named `verdict` replaces it.

### `[[strategy]]`

| Key | Meaning |
|-----|---------|
| `name` | Name of the strategy (required). |
| `args` | A single `pdtrav` command: the options and the benchmark, without the program. |
| `steps` | A list of commands, run one after the other. Give `args` or `steps`, not both. |
| `params` | A table of parameters; see below. |
| `only` | Patterns on benchmark names: the strategy runs only on matching benchmarks. |
| `timeout`, `memory` | Replace the global values for this strategy. `memory = 0` removes a global limit. |

A step that fails (non-zero exit code, timeout or memory limit) ends the run; the remaining steps are skipped.

Commands are split into arguments like a shell would, but they are not run by a shell, so there are no pipes or redirections. A command can also be written as a list of arguments, for example `["{pdtrav}", "-f", "{benchmark}"]`, which is not split further. To use them, run a shell as the program, for example `"{sh} -c 'pdtrav {benchmark} | tee {outdir}/out.txt'"` with `sh = "sh"` in `[tools]`.

```toml
# One command: "args" is shorthand for "{pdtrav} ..."
[[strategy]]
name = "bdd-forward"
args = "-f -ni -verbosity {verbosity} {benchmark}"

# Several commands, on some benchmarks only
[[strategy]]
name = "ic3-certify"
only = ["mod3", "mult2"]
steps = [
  "{pdtrav} -strategy ic3 -writeInvar {outdir}/invar.aig {benchmark}",
  "{pdtapp} certify {benchmark} {outdir}/invar.aig",
]
```

#### Parameters

A strategy with a `params` table runs once for every combination of the values. The strategy below becomes `bmc-k=5` and `bmc-k=10`, each with its own directory:

```toml
[[strategy]]
name = "bmc"
args = "-bmc {k} {benchmark}"
[strategy.params]
k = [5, 10]
```

With several parameters the runner takes every combination, and the name lists the values in order (`name-a=1-b=x`). A parameter value that is itself a list is expanded into several arguments when its placeholder stands alone in a command:

```toml
[strategy.params]
opts = [["-f", "-ni"], ["-b", "-ni"]]
```

with the command `{pdtrav} {opts} {benchmark}`.

### Placeholders

Placeholders are written `{like_this}` and are replaced in every command.

| Placeholder | Value |
|-------------|-------|
| `{benchmark}` | Path of the benchmark file. |
| `{name}` | Name of the benchmark. |
| `{strategy}` | Name of the strategy, including its parameters. |
| `{outdir}` | Directory of the run. Write output files there. |
| `{repo}`, `{here}` | The repository root and the folder of the experiment file. |
| `{pdtrav}`, `{pdtapp}`, ... | The entries of `[tools]`. |
| `{key}` | The entries of `[vars]`, and the parameters of the strategy. |

A placeholder stands for exactly one argument, even if its value contains spaces. Write `{{` and `}}` for literal braces. The names `benchmark`, `name`, `strategy`, `outdir`, `repo` and `here` are reserved. An unknown placeholder is reported before anything runs.

## Command line

```
run-experiments.py [options] EXPERIMENT
run-experiments.py --new FILE
```

| Option | Meaning |
|--------|---------|
| `--new FILE` | Write a copy of the template to `FILE` (creating its folder) and exit. |
| `-j N`, `--jobs N` | Runs executed in parallel. |
| `-o DIR`, `--output DIR` | Base output directory, relative to the current directory. |
| `--name NAME` | Name of the experiment, so the output goes to a different subdirectory. |
| `--timeout SECONDS` | Seconds allowed for each step (`0`: no limit). |
| `--memory LIMIT` | Memory limit for each step, for example `32G` or `512M`; a plain number is gigabytes (`0`: no limit). |
| `--tool NAME=PATH` | Use another program, for example `--tool pdtrav=/builds/b/pdtrav/bin/pdtrav`. Repeatable. |
| `--set KEY=VALUE` | Set or override a variable of the file. A value is read as JSON when it parses (numbers, lists), otherwise as text. Repeatable. |
| `--benchmark PATTERN` | Run only benchmarks whose name matches; wildcards allowed. Repeatable. |
| `--strategy PATTERN` | Run only strategies whose name matches; wildcards allowed. The name includes the parameters, so `bmc*` matches `bmc-k=5`. Repeatable. |
| `--list` | List the runs (strategy and benchmark, separated by a tab) and exit. |
| `--dry-run` | Show the programs and commands without running anything. |
| `--resume` | Reuse the results of runs that already finished successfully. |
| `--overwrite` | Use an existing results directory, replacing the results of the runs executed. |
| `--summarize` | Rebuild `summary.csv` from the saved results and exit. |

Settings given on the command line replace those of the file.

The exit status is 0 when every run finished with status `ok`, 1 when a run did not or the experiment could not start, and 130 when it was interrupted.

## Results

The results of an experiment are written to `<output>/<name>/`, by default `results/<name>/` next to the experiment file:

```
experiment.toml                  a copy of the experiment file
manifest.json                    how and when the experiment was run
runner.log                       the progress messages, with time stamps
summary.csv, summary.json        one row (entry) per run
bdd-forward/cnt1/run.log         output of all the steps of the run
bdd-forward/cnt1/result.json     status, timing and extracted values
ic3-certify/mod3/invar.aig       files written by the steps
...
```

Every step runs with the directory of its run as the working directory, so files that a tool writes into the current directory stay with their run. A step reads its standard input from `/dev/null`.

### Statuses

| Status | Meaning |
|--------|---------|
| `ok` | Every step exited with code 0. This says nothing about the verdict. |
| `fail` | A step exited with a non-zero code. |
| `timeout` | A step exceeded the time limit. |
| `memout` | A step exceeded the memory limit. |
| `error` | A step could not be started, for example because the program was not found. |
| `interrupted` | The run was stopped by an interrupt. |

### `summary.csv`

One row per run, with these columns:

| Column | Content |
|--------|---------|
| `strategy` | Name of the strategy, including its parameters. |
| `benchmark` | Name of the benchmark. |
| `params` | The parameter values, for example `k=5`. |
| `status` | One of the statuses above. |
| `seconds` | Wall-clock time of all the steps of the run. |
| `peak_mb` | Peak memory in megabytes; see below. Empty when not measured. |
| ... | One column for each `[[extract]]` entry, starting with `verdict`. |
| `outdir` | Directory of the run, relative to the results directory. |

```
strategy,benchmark,params,status,seconds,peak_mb,verdict,certified,outdir
bdd-forward,cnt1,,ok,1.938,59.0,FAIL,,bdd-forward/cnt1
bdd-forward,mod3,,ok,1.478,59.1,PASS,,bdd-forward/mod3
bmc-k=5,mod3,k=5,ok,1.438,59.3,,,bmc-k=5/mod3
ic3-certify,mod3,,ok,1.49,59.2,PASS,yes,ic3-certify/mod3
```

The summary covers every run saved in the results directory, also those of earlier invocations, sorted by strategy and benchmark.

### `run.log` and `result.json`

`run.log` starts each step with the command (`$ ...`), followed by its output, and ends it with a line such as `[step 1: ok, exit code 0, 1.37 s]`. `result.json` holds the status, the exit codes, the commands, the duration, the peak memory and the limit, the time the run started, the directory and the extracted values.

## Timeouts and memory limits

Both limits apply to each step, not to the run as a whole. A step that exceeds one is stopped together with every process it started, and the run ends with status `timeout` or `memout`.

A timed-out step is asked to stop first and killed if it has not stopped five seconds later.

For memory the runner samples the resident memory of each running step twice a second, summed over all the processes of the step, and kills the step at once when the sum exceeds the limit. Units are binary: `G` means GiB. This has some consequences:

- A memory spike shorter than half a second can be missed, and a program that allocates very quickly can exceed the limit by a margin before it is stopped.
- Resident memory includes the executable and the shared libraries. A small `pdtrav` run already shows about 60 MB.
- Pages shared between forked processes are counted once per process, so the figure is approximate for steps that start several processes. Threads of one process are counted once.
- Memory limits need Linux.

`peak_mb` is the highest sampled value, recorded for every run even when no limit is set, which helps in choosing a limit. `pdtrav` also has its own graceful memory limit (`-totMemoryLimit`, in megabytes); it can be used in the arguments of a strategy next to the runner's limit, which then acts as a safety net.

## Resuming, re-running and stopping

- Running an experiment into a results directory that already exists is refused, so that results are not overwritten by accident. Use `--resume` to continue, `--overwrite` to replace results, or `--name` or `--output` for a new directory.
- `--resume` keeps the results of runs that finished with status `ok` and runs everything else again, including failed, timed-out and interrupted runs. Together with `--benchmark` and `--strategy` it also reruns a part of an experiment; the summary keeps covering all saved runs.
- `--overwrite` runs the selected runs again and replaces their results. Results of other runs in the directory are kept and stay in the summary. Remove the directory, or use another name, to start clean.
- `--summarize` rebuilds `summary.csv` and `summary.json` from the saved `result.json` files without running anything and without needing the benchmarks. Use it when an experiment was killed before it could write its summary.
- Press Ctrl-C once to stop the experiment. The running commands are stopped (asked first, killed after three seconds), runs that have not started are skipped, the stopped runs are saved with status `interrupted`, and the exit status is 130. A second Ctrl-C kills the running commands immediately.

### Parallel runs

`-j N` runs `N` runs at the same time. Timings are only comparable between experiments that use the same `-j`, and memory use adds up: the memory limit applies to each step, not to the total.

## Recipes

**Another set of benchmarks, without editing the file.** If the file uses a variable in its benchmark patterns, as the template does, override it:

```bash
run-experiments.py experiment.toml --set examples=/data/hwmcc
```

**Compare two builds.** Run the same file with two programs, each into its own directory:

```bash
run-experiments.py experiment.toml --name build-a --tool pdtrav=/builds/a/pdtrav/bin/pdtrav
run-experiments.py experiment.toml --name build-b --tool pdtrav=/builds/b/pdtrav/bin/pdtrav
```

**Run a part.** Select by name, and check the selection first:

```bash
run-experiments.py experiment.toml --strategy 'bmc*' --benchmark 'mult*' --list
```

**Different limits for one strategy.** Give it its own values:

```toml
[[strategy]]
name = "bdd-heavy"
timeout = 7200
memory = "64G"
args = "-f {benchmark}"
```

**Keep the results on another disk.** Send the output elsewhere; relative paths on the command line are relative to the current directory:

```bash
run-experiments.py experiment.toml -o /scratch/exp42
```

**Run again what failed.** The runs that finished stay; the others run again:

```bash
run-experiments.py experiment.toml --resume
```

## Troubleshooting

- **`results already exist in ...`** The results directory of this experiment exists. Use `--resume`, `--overwrite`, `--name` or `--output`, as described in [Resuming, re-running and stopping](#resuming-re-running-and-stopping).
- **`no benchmark files found`** No pattern matched a file. Relative patterns are relative to the folder of the experiment file, not to the current directory. The message names that folder, and a warning lists each pattern that matched nothing.
- **`unknown placeholder {x}`** A command or path uses a name that is not defined. The message lists the available names. Literal braces must be doubled.
- **`reading TOML needs Python 3.11 or newer`** Use a newer Python, or write the experiment as a JSON file.
- **Runs end with status `error`.** The program could not be started. Open `run.log`: it says why, and the `tools:` line printed at the start of the run shows which programs were resolved. A bare name such as `pdtrav` is looked up on `PATH`; use `./name` for a file next to the experiment file.
- **A run shows `memout` unexpectedly.** Resident memory includes the executable and shared libraries, and is summed over all the processes of the step. Check `peak_mb` of similar runs to choose the limit.
- **`memory limits need Linux`** The memory monitor reads `/proc`. Remove the limit on systems without it.
