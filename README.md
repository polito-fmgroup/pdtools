# pdtools

A portfolio-based suite for hardware model checking, developed by the Formal Methods Group at Politecnico di Torino.

`pdtrav` (PdTRAV, Politecnico di Torino Reachability Analysis and Verification) verifies safety properties of hardware designs given in AIGER format. Instead of relying on a single algorithm, it combines several engines (bounded model checking, interpolation-based model checking, IC3/PDR, BDD-based traversal and induction) with circuit simplification and model transformation steps, and it can run several of them as a portfolio. It can write the inductive invariant it finds, so that `pdtapp certify` can check the result independently.

Two executables are built from the sources in `pdtrav/`:

- **`pdtrav`** is the verification tool. It accepts `.aag` (ASCII) and `.aig` (binary) models and runs the verification strategy you select, for example `-strategy ic3`.
- **`pdtapp`** bundles companion utilities. Its first argument is the name of an app: `aiger`, `certify`, `gfp` or `iso`. This README uses `certify`, which checks an invariant against a model.

## Contents

- [Repository layout](#repository-layout)
- [Requirements](#requirements)
- [Building](#building)
- [Checking the build](#checking-the-build)
- [Quick start](#quick-start)
- [Workflows](#workflows)
- [Troubleshooting](#troubleshooting)
- [Citing pdtools](#citing-pdtools)
- [Third-party components and licence](#third-party-components-and-licence)

## Repository layout

| Path | Contents |
|------|----------|
| `pdtrav/` | Sources of `pdtrav` and `pdtapp`. Executables are built into `pdtrav/bin/`. |
| `abc/` | ABC, built as a library and linked into `pdtrav`. |
| `cudd-2.5.0/` | CUDD 2.5.0, a decision diagram package. |
| `aiger-1.9.3/` | AIGER 1.9.3, the AIGER format library and tools. |
| `lingeling-bcj/` | Lingeling, a SAT solver. |
| `minisat-p_v1.14/`, `minisat-2.2.0-drup/` | Two MiniSat variants. |
| `tools/` | [`pdtrav-regress.sh`](tools/pdtrav-regress.sh) and the [example models](tools/examples/README.md) it runs. |
| `configure`, `config.mk`, `Makefile` | The build system, described below. |

## Requirements

- A C and a C++ compiler (GCC)
- GNU Make 4.3 or newer
- `flex` and `bison`
- The development files for GNU Readline (`readline/readline.h`)

`./configure` checks for the tools and headers. If something is missing, it prints the install command for your distribution, and `./configure --install-deps` runs it for you (using `sudo` if you are not root). The Makefile itself stops with a clear message if GNU Make is older than 4.3.

## Building

```bash
./configure
make
```

`./configure` selects the newest GCC it finds and records the choice in `config.local.mk`, which is generated for your machine and not committed. `make` then builds the bundled libraries, followed by `pdtrav` and `pdtapp`. The first build takes several minutes, mostly for ABC. Later builds only recompile what changed.

`make` also works without `./configure`. It uses `gcc` and `g++` and checks the dependencies itself before building.

To run the tools by name, put them on your `PATH`:

```bash
export PATH="$PWD/pdtrav/bin:$PATH"
```

### `./configure` options

| Option | Meaning |
|--------|---------|
| `--cc=COMPILER`, `--cxx=COMPILER` | Use a specific compiler, for example `--cc=gcc-14 --cxx=g++-14`. |
| `--std=STANDARD` | C++ standard to request (default `c++17`), with a fall-back to `c++14`. It applies to the CUDD build only. |
| `--build-mode=MODE` | `default`, `debug` or `sanitize` (see below). |
| `--install-deps` | Install missing packages with the system package manager. |

### `make` targets

| Command | Effect |
|---------|--------|
| `make` | Check dependencies, then build `pdtrav` and `pdtapp`. |
| `make check-deps` | Only check that the required tools and headers are present. |
| `make vendor-libs` | Build only the bundled third-party libraries. |
| `make info` | Show the compilers and flags in use. |
| `make clean` | Remove the output of `pdtrav` itself, keeping the libraries. |
| `make distclean` | Remove all build output, including the libraries. |
| `make help` | List the targets. |

### Build modes

| Mode | Use |
|------|-----|
| `default` | `pdtrav` compiled with `-g` and no optimisation; CUDD and Lingeling with `-O6`. |
| `debug` | Debug builds throughout (`make BUILD_MODE=debug`). |
| `sanitize` | AddressSanitizer build (`make BUILD_MODE=sanitize`). |

`pdtrav`'s own sources are built without optimisation in all modes. Note that `-O6` may trigger an internal assertion failure on some example models with GCC 13 (see also `config.mk`).

### Notes

- The Makefile tracks source changes only. After changing the compiler, the build mode or other flags, run `make distclean` before building again.
- Build serially: run `make` without `-j`.
- `make distclean` deletes `aiger-1.9.3/makefile` and `lingeling-bcj/makefile`, which are regenerated during the next build. Those file can also be restored from the source distribution if needed.

## Checking the build

```bash
tools/pdtrav-regress.sh pdtrav/bin/pdtrav
```

The script runs `pdtrav` on the models in [`tools/examples`](tools/examples/README.md) with several option sets, and runs the IC3 → invariant → certify flow on two of them. A good build ends with:

```
runs: 42, failed: 0
```

and exit status 0. With `-r REFERENCE` it also compares the output with that of another `pdtrav` build, for example one from a different revision or built with different flags. See the header of the script for details.

## Quick start

With `pdtrav` and `pdtapp` on your `PATH`, run IC3 on the example model `tools/examples/mod3.aag` and write the invariant it finds:

```bash
pdtrav -strategy ic3 -writeInvar mod3-invar.aig tools/examples/mod3.aag
```

The output includes:

```
Verification Result: PASS.
```

Then certify the invariant against the model:

```bash
pdtapp certify tools/examples/mod3.aag mod3-invar.aig
```

A successful certification prints:

```
Check Done for init proved
Check Done invariant proved
Check Done property proved
```

(each `Check Done` line is followed by timing information in the real output).

## Options reference

This is a selection of the most commonly used options. `pdtrav` has many more: run `pdtrav` without arguments to list all option names, and `pdtrav -h <option>` for the description of an option. The leading dash is optional and options are matched by prefix, so `pdtrav -h bmc` describes every option starting with `-bmc`.

The synopsis is `pdtrav [options] <model>`. Options are processed in the order given, which matters in the cases noted below.

### Engines

| Option | Description |
|--------|-------------|
| `-strategy <s>`, `-engine <s>` | Select an engine using either of the two available aliases. The values of `<s>` are listed below. Like `-ni`, it complements the property (see [Selecting the property](#selecting-the-property)). |
| `-bmc <k>` | SAT-based bounded model checking up to bound `k`. |
| `-inductive <s>` | SAT-based inductive model checking with step `s`. |
| `-interpolant` | SAT-based interpolation-based model checking. |

Values of `-strategy`:

- `bdd`: BDD-based verification.
- `ic3`: IC3.
- `itp<n>`: interpolation-based model checking. The digit `n` (0 to 6, default 0) selects the interpolation effort.
- `igr<n>`: interpolation with guided refinement, with the same digit.
- `thrd`: several engines running in threads.

Letters after the digit of `itp` and `igr` enable variants: `c` applies phase abstraction, `p` pushes interpolant components to the next iteration, e.g., `itp3` or `igr1p`.

### BDD-based verification

Without any of these options, `pdtrav` runs `-fb`.

| Option | Description |
|--------|-------------|
| `-f` | Exact forward verification. |
| `-b` | Exact backward verification. |
| `-fb` | Approximate forward, exact backward verification (default). |
| `-bf` | Approximate backward, exact forward verification. |
| `-fbf` | Approximate forward, approximate backward, then exact forward verification. |
| `-t` | Compute the forward reachable states only, without verifying. |
| `-bt` | Compute the backward reachable states only, without verifying. |

### Selecting the property

| Option | Description |
|--------|-------------|
| `-ni` | Complement the invariant specification. |
| `-ilambda <i>` | Use the `i`-th output as the invariant specification. |
| `-nlambda <name>` | Use the output with the given name as the invariant specification. |
| `-invarspec <file>` | Read the invariant specification from a file. |

By default the first output is the invariant that must hold, and `pdtrav` prints `Property: o0.`. The options `-ni`, `-strategy` and `-engine` complement it (`Property: !o0.`), so that the output is read as a bad-state signal. On the models in `tools/examples`, `-f -ni` and `-b -ni` give the same verdicts as `-strategy ic3`.

### Model reductions

| Option | Description |
|--------|-------------|
| `-peripheralLatches` | Enable peripheral latch reduction. `-noPeripheralLatches` disables it. |
| `-retime <s>` | Min-register retiming with strategy `s` (0: no retiming). |
| `-coi` | Functional cone-of-influence reduction. |
| `-cois` | Structural and functional cone-of-influence reduction. |
| `-noTernary` | Skip ternary simulation. |

`-strategy ic3` enables peripheral latch reduction and `-writeInvar` disables it. To keep the reduction while writing an invariant, give `-peripheralLatches` after `-writeInvar`, as in the temporal decomposition workflow above.

### Output

| Option | Description |
|--------|-------------|
| `-writeInvar <file>` | Write the inductive invariant found to `<file>` in AIGER format. |
| `-wres <file>` | Write the verification result to `<file>`. With `-strategy ic3`, a property that holds writes `0` and a failing property writes `1`. On failure a counterexample in AIGER witness format is also written to `<file>.cex`. |
| `-printNtkStats` | Print the size of the design and stop. |

### Verbosity and limits

| Option | Description |
|--------|-------------|
| `-verbosity <v>` | Verbosity level, from 0 (none) to 8 (debug). |
| `-itpTimeLimit <t>` | Time limit for the interpolation-based engines, in seconds. |
| `-totMemoryLimit <m>` | Memory limit, in megabytes. |
| `-satTimeLimit <t>` | Time limit for each SAT call, in seconds (default 30). |

### `pdtapp certify`

The synopsis is `pdtapp certify [options] <model> <invariant>`, with both files in AIGER format.

| Option | Description |
|--------|-------------|
| `-f <k>` | Number of phases or frames, as used by phase abstraction (default 1). |
| `-t <k>` | Steps of temporal decomposition (default 0). |
| `-s <file>` | An additional invariant, read from an AIGER file and used together with the invariant file. |

## Workflows

#### Generating an invariant with IC3 and checking it against the original model

The following command generates a baseline invariant by model checking the input model, given in AIGER format, with the *IC3* engine in `pdtrav`.

```
pdtrav -strategy ic3 [-verbosity 4] -writeInvar <output_invar_filename> <input_model_filename>
```

The following command certifies the generated invariant against the original model using the `pdtapp` utility.

```
pdtapp certify <input_model_filename> <input_invar_filename>
```

#### Phase abstraction

If phase abstraction (PA) is applied to the original input model, producing an intermediate model, verification runs on the transformed, simplified model, whereas certification has to take the original model into account.

```
pdtrav -strategy ic3 [-verbosity 4] -writeInvar <output_invar_filename> <input_pa_model_filename>
```

To support certification, the number of phases identified during phase abstraction must also be provided, as `-f <F>`:

```
pdtapp certify -f <F> <input_model_filename> <input_invar_filename>
```

#### Temporal decomposition

If temporal decomposition (TD) is used, an additional parameter, `-peripheralLatches`, has to be included in the verification command:

```
pdtrav -strategy ic3 [-verbosity 4] -writeInvar <output_invar_filename> -peripheralLatches <input_model_filename>
```

The `<output_invar_filename>` must end in `-T#.aig`, where `#` encodes the number of steps considered for the decomposition. This value must then be provided as a parameter during certification, as `-t <T>`:

```
pdtapp certify -t <T> <input_model_filename> <input_invar_filename>
```

#### Rewriting models with ABC

The two commands below use a standalone `abc` executable. The ABC in this repository is built only as a library (`libabc.a`) that is linked into `pdtrav`, and it cannot be run on its own. Install or build ABC separately (<https://github.com/berkeley-abc/abc>).

It can be convenient to include an explicit symbol table, in order to uniquely identify all the components of a model regardless of the sequence of transformations applied. ABC's rewriting capabilities can do this:

```
abc -c "read <input_file>;write <output_file>;"
```

It can also be convenient to preprocess models in bulk and store the versions obtained after phase abstraction. This can be done with ABC as well:

```
abc -c "read <input_file>;frames -F <F>;orpos;write <output_file>;"
```

## Troubleshooting

**`./configure` or `make` reports missing tools or headers.** Install the packages it names, or run `./configure --install-deps`.

**`Error Loading FSM line 1: expected space or new line after literal ...`** The model file has Windows (CRLF) line endings, usually because git converted it on checkout. The ASCII format (`.aag`) accepts only LF. Convert the file with `dos2unix` (or `sed -i 's/\r$//'`), and set `git config core.autocrlf false` so git stops converting. Do not convert the binary `.aig` files. The repository's `.gitattributes` keeps the example models and the build files as LF.

**`pdtrav` crashes or behaves differently after pulling changes or changing the compiler or flags.** Existing libraries are not rebuilt when only the flags change. Run `make distclean && ./configure && make`.

**`git status` shows `aiger-1.9.3/makefile` and `lingeling-bcj/makefile` as deleted.** This is the effect of `make distclean`, see [Notes](#notes). Restore them with `git restore` if need be.

Known behaviours of the example models, such as `pdtapp certify` aborting on two of them, are listed in [`tools/examples/README.md`](tools/examples/README.md).

## Citing pdtools

If you use pdtools in academic work, please cite:

> G. Cabodi, S. Nocco, S. Quer. *Benchmarking a model checker for algorithmic improvements and tuning for performance.* Formal Methods in System Design 39(2):205–227, 2011. [doi:10.1007/s10703-011-0123-3](https://doi.org/10.1007/s10703-011-0123-3)

```bibtex
@article{CabodiNoccoQuer2011,
  author  = {Cabodi, Gianpiero and Nocco, Sergio and Quer, Stefano},
  title   = {Benchmarking a model checker for algorithmic improvements and tuning for performance},
  journal = {Formal Methods in System Design},
  volume  = {39},
  number  = {2},
  pages   = {205--227},
  year    = {2011},
  doi     = {10.1007/s10703-011-0123-3}
}
```

In the literature the tool is referred to as PdTRAV (Politecnico di Torino Reachability Analysis and Verification).

### Recent publications using pdtools

- G. Cabodi, P. E. Camurati, M. Palena, P. Pasini. *Manipulating Proof Certificate Invariants in the Presence of Model Transformations.* IEEE Access 14:65572–65585, 2026. [doi:10.1109/ACCESS.2026.3687715](https://doi.org/10.1109/ACCESS.2026.3687715)
- G. Sindoni, P. Pasini, G. Cabodi, P. E. Camurati, A. Griggio, M. Palena, M. Roveri, S. Tonetta. *A Theorem Prover Based Approach for SAT-Based Model Checking Certification.* In Automated Deduction – CADE 30, LNCS 15943, pp. 449–467. Springer, 2025. [doi:10.1007/978-3-031-99984-0_24](https://doi.org/10.1007/978-3-031-99984-0_24)
- G. Cabodi, P. E. Camurati, M. Palena, P. Pasini. *Improving Bounded Model Checking Exploiting Interpolation-Based Learning and Strengthening.* IEEE Access 12:119341–119349, 2024. [doi:10.1109/ACCESS.2024.3446802](https://doi.org/10.1109/ACCESS.2024.3446802)

## Third-party components and licence

This repository is released under the MIT licence (see [`LICENSE`](LICENSE)).

The bundled third-party components (ABC, CUDD, AIGER, Lingeling and MiniSat, see [Repository layout](#repository-layout)) are distributed under their own terms. See the licence and readme files in each component's directory.
