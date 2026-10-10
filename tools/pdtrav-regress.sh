#!/bin/bash
# pdtrav-regress.sh - regression check for pdtrav builds
#
# Runs pdtrav on the small sample models that ship with AIGER, with several
# engines, and either prints a verdict table for one build or compares the
# complete output of two builds.
#
# Usage:
#   pdtrav-regress.sh BUILD [EXAMPLES_DIR]
#       Print a verdict table for BUILD. Fails if any run crashes.
#   pdtrav-regress.sh -r REFERENCE BUILD [EXAMPLES_DIR]
#       Also compare the output of BUILD with that of REFERENCE, ignoring
#       timings, dates, host names and build paths. Fails on any difference.
#
# BUILD and REFERENCE are paths to pdtrav executables, for example
# pdtrav/bin/pdtrav. EXAMPLES_DIR defaults to aiger-1.9.3/examples in the
# repository that contains BUILD.
#
# Typical use after changing build flags or compilers: build unmodified
# upstream in a separate checkout, then
#   pdtrav-regress.sh -r /path/to/upstream/pdtrav/bin/pdtrav pdtrav/bin/pdtrav
#
# Environment:
#   TIMEOUT  seconds allowed for each run (default 60)
#
# Exit status: 0 if every run completed without crashing (and, with -r,
# every output matches); 1 otherwise; 2 on a usage error.
#
# The model set is small. It catches gross breakage such as crashes or wrong
# verdicts, but it is not a substitute for testing on your own designs.

TIMEOUT=${TIMEOUT:-60}

# Models (relative to EXAMPLES_DIR) and the option sets each one is run with.
MODELS="cnt1.aag cnt1e.aag notcnt1.aag toggle.aag smv/s2cfair.aig smv/s2cunfair.aig smv/mult2.aig"
OPTION_SETS=("-f" "-b" "-bmc 5" "-f -verbosity 3")

usage() { sed -n '2,/^$/p' "$0" | sed 's/^# \{0,1\}//'; exit 2; }

REF=""
if [ "$1" = "-r" ]; then
    [ $# -ge 3 ] || usage
    REF=$2; shift 2
fi
[ $# -ge 1 ] && [ $# -le 2 ] || usage
BUILD=$1
EXAMPLES=$2

for b in "$BUILD" $REF; do
    [ -x "$b" ] || { echo "error: '$b' is not an executable file" >&2; exit 2; }
done
BUILD=$(realpath "$BUILD")
[ -n "$REF" ] && REF=$(realpath "$REF")

# The repository root is three levels above <root>/pdtrav/bin/pdtrav.
root_of() { dirname "$(dirname "$(dirname "$1")")"; }
BUILD_ROOT=$(root_of "$BUILD")
[ -n "$REF" ] && REF_ROOT=$(root_of "$REF")
EXAMPLES=${EXAMPLES:-$BUILD_ROOT/aiger-1.9.3/examples}
[ -d "$EXAMPLES" ] || { echo "error: examples directory '$EXAMPLES' not found" >&2; exit 2; }

WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT

# run BINARY OPTIONS MODEL: set OUT to the combined output and RC to the exit
# status. Not called from a command substitution, so RC is visible to the caller.
run() {
    local bin=$1 opts=$2 model=$3
    # shellcheck disable=SC2086  # $opts is meant to word-split
    OUT=$(cd "$WORK" && timeout "$TIMEOUT" "$bin" $opts "$model" 2>&1)
    RC=$?
}

crashed() { [ "$1" -ne 0 ] || grep -q -E "Caught SIGNAL|FATAL ERROR|Assertion" <<<"$2"; }

# Drop lines that legitimately differ between runs and builds, and mask the
# repository paths that appear in assertion messages.
normalise() {
    grep -v -i -E "date|hostname|cpu time|command|time|[0-9] ?s(ec)?\b|memory|MB|scale factor" |
        sed -e "s#$BUILD_ROOT#ROOT#g" ${REF:+-e "s#$REF_ROOT#ROOT#g"}
}

verdict() {
    grep -m1 -o -E "(Invariant Verification|Verification Result): [A-Z]+" <<<"$1" || echo "-"
}

runs=0; crashes=0; diffs=0; skipped=0
printf '%-22s %-17s %-6s %s\n' MODEL OPTIONS STATUS VERDICT

for model in $MODELS; do
    path=$EXAMPLES/$model
    if [ ! -f "$path" ]; then
        echo "warning: $path not found, skipping" >&2; skipped=$((skipped + 1)); continue
    fi
    for opts in "${OPTION_SETS[@]}"; do
        runs=$((runs + 1))
        run "$BUILD" "$opts" "$path"; out=$OUT; rc=$RC
        status=ok
        if crashed "$rc" "$out"; then status=CRASH; crashes=$((crashes + 1)); fi
        if [ -n "$REF" ]; then
            run "$REF" "$opts" "$path"; ref_out=$OUT
            if [ "$(normalise <<<"$out")" != "$(normalise <<<"$ref_out")" ]; then
                [ "$status" = ok ] && status=DIFF
                diffs=$((diffs + 1))
                difflines=$(diff <(normalise <<<"$ref_out") <(normalise <<<"$out") | head -6)
            else
                difflines=""
            fi
        fi
        printf '%-22s %-17s %-6s %s\n' "$model" "$opts" "$status" "$(verdict "$out")"
        [ -n "$difflines" ] && sed 's/^/        /' <<<"$difflines"
        difflines=""
    done
done

echo
summary="runs: $runs, crashed: $crashes"
[ -n "$REF" ] && summary="$summary, differing from reference: $diffs"
[ "$skipped" -gt 0 ] && summary="$summary, skipped models: $skipped"
echo "$summary"
[ "$crashes" -eq 0 ] && [ "$diffs" -eq 0 ] && [ "$runs" -gt 0 ]
