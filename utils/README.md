# tools

Scripts and data for working with a built `pdtrav`: checking a build, and running experiments. The sources of `pdtrav` and `pdtapp` are in [`../pdtrav`](../pdtrav).

## Contents

| Path | What it is |
|------|------------|
| [`pdtrav-regress.sh`](pdtrav-regress.sh) | Regression check for a build of `pdtrav`: runs the example models with several options and compares the results with a reference build on request. |
| [`run-experiments.py`](run-experiments.py) | Runs `pdtrav` and `pdtapp` over many benchmarks and strategies, with timeouts, memory limits, logs and a summary table. |
| [`experiments/`](experiments) | The experiment template and the [documentation of the runner](experiments/README.md). |
| [`examples/`](examples) | Small AIGER models used by the regression script and the experiment template, with [their recorded results](examples/README.md). |

## Which tool for what

- You changed the build (compiler, flags, sources) and want to know whether `pdtrav` still behaves: use the [regression script](#checking-a-build).
- You want to run strategies over a set of benchmarks, with limits, logs and a summary: use the [experiment runner](#running-experiments).
- You need a few small models to try something: use the [example models](#example-models).

## Checking a build

```bash
utils/pdtrav-regress.sh pdtrav/bin/pdtrav
```

The script runs every example model with five option sets, and runs the IC3 → invariant → certify flow on `mod3.aag` and `mult2.aig`: 42 runs in total. A good build ends with:

```
runs: 42, failed: 0
```

The exit status is 0 in that case and 1 when any run fails. A run fails when `pdtrav` crashes, aborts, times out or exits with an error, or when the certify flow does not certify the invariant. `pdtapp` is expected in the same directory as `pdtrav`. The environment variable `TIMEOUT` sets the seconds allowed for each run (default 60).

To compare a build with a reference build, for example one made from another revision or with other flags, pass the reference with `-r`:

```bash
utils/pdtrav-regress.sh -r /path/to/reference/pdtrav/bin/pdtrav pdtrav/bin/pdtrav
```

The complete output of every run must then match, ignoring timings, dates, host names and build paths, and the invariant files written by the certify flow must be identical. Any difference makes the script fail and is shown next to the run.

The model set is small. The script catches gross breakage such as crashes and changed verdicts, but it is not a substitute for testing on your own designs. The header of the script describes every option.

## Running experiments

The runner is driven by a small experiment file that lists the benchmarks and the strategies. It runs every combination in its own directory, keeps a log per run and writes a summary.

```bash
utils/run-experiments.py --new /data/exp42/experiment.toml
utils/run-experiments.py /data/exp42/experiment.toml --dry-run
utils/run-experiments.py /data/exp42/experiment.toml -j 4
```

The runner works from any directory and keeps the results next to the experiment file. Strategies can have parameters, several commands (for example IC3 followed by certification) and their own limits. The [documentation](experiments/README.md) covers the experiment file, the options, the layout of the results and recipes such as comparing two builds. [`experiments/template.toml`](experiments/template.toml) is a commented starting point that runs on the example models.

## Example models

[`examples/`](examples) holds eight small models: seven copied from the examples of AIGER, and `mod3.aag`, which is written for this repository. Both the regression script and the experiment template use them, so a sample run and a regression run exercise the same inputs. The [README of the directory](examples/README.md) lists the models and what `pdtrav` prints for each.

## Requirements

- `pdtrav-regress.sh`: bash and the GNU tools `timeout`, `realpath` and `mktemp`.
- `run-experiments.py`: Python 3.9 or newer. TOML experiment files need Python 3.11 or newer (JSON files work on older versions), and memory limits need Linux.