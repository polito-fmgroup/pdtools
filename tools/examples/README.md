# Example models

Small AIGER models used by the quick start in the top-level README and by the regression script [`../pdtrav-regress.sh`](../pdtrav-regress.sh). Keeping them in one place means a sample run and a regression run exercise the same inputs.

## Models

| File | Origin | Description |
|------|--------|-------------|
| `mod3.aag` | written for this repository | Two-bit counter 00 → 01 → 10 → 00. The bad state 11 is unreachable. |
| `cnt1.aag`, `cnt1e.aag`, `notcnt1.aag`, `toggle.aag` | `aiger-1.9.3/examples` | Tiny counters and toggles. |
| `s2cfair.aig`, `s2cunfair.aig`, `mult2.aig` | `aiger-1.9.3/examples/smv` | Binary AIGER (`.aig`) models. |

The AIGER files are copies of the files in AIGER's own directories and remain under the AIGER licence (`aiger-1.9.3/LICENSE`). `mod3.aag` is covered by the licence of this repository.

## Recorded results

What `pdtrav` prints for each model. These are baselines for detecting changes in behaviour: they do not independently establish correctness.

| Model | `-f` | `-b` | `-bmc 5` | `-strategy ic3` | IC3 invariant certified by `pdtapp certify` |
|-------|------|------|----------|-----------------|---------------------------------------------|
| `mod3.aag` | FAIL | FAIL | no verdict line | PASS | yes |
| `cnt1.aag` | FAIL | FAIL | no verdict line | FAIL | no (see below) |
| `cnt1e.aag` | FAIL | FAIL | no verdict line | FAIL | no (see below) |
| `notcnt1.aag` | FAIL | FAIL | no verdict line | FAIL | no (see below) |
| `toggle.aag` | FAIL | FAIL | no verdict line | FAIL | no (see below) |
| `s2cfair.aig` | PASS | PASS | no verdict line | PASS | no, `pdtapp` aborts |
| `s2cunfair.aig` | PASS | PASS | no verdict line | PASS | no, `pdtapp` aborts |
| `mult2.aig` | FAIL | FAIL | no verdict line | PASS | yes |

Notes on the table:

- **`-f`/`-b` and `-strategy ic3` do not agree on `mod3.aag` and `mult2.aig`.** Plain runs print `Property: o0.`, while `-strategy ic3` prints `Property: !o0.`. This is consistent with the first output being read as a property that must hold in plain runs and as a bad-state signal with IC3. The verdicts of the two groups are therefore not comparable.
- **Models where IC3 reports FAIL** get a constant-false invariant, and `certify` prints `Check Done for init disproved`. It also prints `invariant proved` and `property proved`, because a false invariant is trivially inductive. Certification is only meaningful when IC3 reports PASS.
- **`s2cfair.aig` and `s2cunfair.aig`**: IC3 reports PASS and writes an empty (constant-true) invariant, and `pdtapp certify` then aborts with an assertion in `ddiArray.c`.
- **`-bmc 5`** prints no `Verification Result` line for these models, as it does not reach a fix-point for the verification.

## Running

From the repository root, after building:

```bash
tools/pdtrav-regress.sh pdtrav/bin/pdtrav
```

The script runs every model with five option sets and runs the IC3 → invariant → certify flow on `mod3.aag` and `mult2.aig`. See the header of the script for comparing two builds with `-r`.
