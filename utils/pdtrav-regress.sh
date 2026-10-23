#!/bin/bash
# pdtrav-regress.sh - regression check for pdtrav builds
#
# Runs pdtrav on the models in utils/examples with several option sets, and
# runs the IC3 -> invariant -> certify flow on the models that support it.
# It either prints a result table for one build or compares two builds.
#
# Usage:
#   pdtrav-regress.sh BUILD [EXAMPLES_DIR]
#       Print a result table for BUILD. Fails if any run fails.
#   pdtrav-regress.sh -r REFERENCE BUILD [EXAMPLES_DIR]
#       Also compare BUILD with REFERENCE: the complete output of every run
#       (ignoring timings, dates, host names and build paths) and the
#       invariant files must match. Fails on any difference.
#
# BUILD and REFERENCE are paths to pdtrav executables, for example
# pdtrav/bin/pdtrav. pdtapp is expected in the same directory.
# EXAMPLES_DIR defaults to the examples directory next to this script.
#
# Typical use after changing build flags or compilers: build unmodified
# upstream in a separate checkout, then
#   utils/pdtrav-regress.sh -r /path/to/upstream/pdtrav/bin/pdtrav pdtrav/bin/pdtrav
#
# Environment:
#   TIMEOUT  seconds allowed for each run (default 60)
#
# Exit status: 0 if every run succeeded (and, with -r, every output matches);
# 1 otherwise; 2 on a usage error.
#
# The model set is small. It catches gross breakage such as crashes or wrong
# verdicts, but it is not a substitute for testing on your own designs.

TIMEOUT=${TIMEOUT:-60}

# Models in the examples directory, and the option sets each one is run with.
MODELS="mod3.aag cnt1.aag cnt1e.aag notcnt1.aag toggle.aag s2cfair.aig s2cunfair.aig mult2.aig"
OPTION_SETS=("-f" "-b" "-bmc 5" "-f -verbosity 3" "-strategy ic3")

# Models for which the IC3 -> invariant -> certify flow must succeed. The other
# models are left out on purpose (see utils/examples/README.md): for those where
# IC3 finds a counterexample the invariant is constant false, and pdtapp aborts
# on the empty invariant IC3 writes for s2cfair and s2cunfair.
FLOW_MODELS="mod3.aag mult2.aig"

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
EXAMPLES=${EXAMPLES:-$(dirname "$(realpath "$0")")/examples}
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

failed() { [ "$1" -ne 0 ] || grep -q -E "Caught SIGNAL|FATAL ERROR|Assertion" <<<"$2"; }

# flow BINARY MODEL TAG: run IC3, write the invariant, and certify it with the
# pdtapp that sits next to BINARY. Sets FLOW_OK (1 or 0), FLOW_OUT (the certify
# output) and FLOW_INV (the invariant file).
flow() {
    local bin=$1 model=$2 tag=$3 app rc
    app=$(dirname "$bin")/pdtapp
    FLOW_INV=$WORK/inv-$tag.aig; FLOW_OUT=""; FLOW_OK=0
    rm -f "$FLOW_INV"
    run "$bin" "-strategy ic3 -writeInvar $FLOW_INV" "$model"
    [ "$RC" -eq 0 ] && [ -s "$FLOW_INV" ] || return
    FLOW_OUT=$(cd "$WORK" && timeout "$TIMEOUT" "$app" certify "$model" "$FLOW_INV" 2>&1)
    rc=$?
    # All three checks must pass. "init disproved" (printed for models where IC3
    # finds a counterexample and writes a constant-false invariant) is a failure.
    [ "$rc" -eq 0 ] && grep -q "for init proved" <<<"$FLOW_OUT" &&
        grep -q "invariant proved" <<<"$FLOW_OUT" &&
        grep -q "property proved" <<<"$FLOW_OUT" && FLOW_OK=1
}

# Drop lines that legitimately differ between runs and builds (timings, dates,
# host names, lines made only of decimal numbers such as the per-frame solver
# times), and mask the repository paths that appear in assertion messages and
# the temporary invariant file names that certify prints.
normalise() {
    grep -v -i -E "date|hostname|cpu time|command|time|[0-9] ?s(ec)?\b|memory|MB|scale factor" |
        grep -v -E '^ ?[0-9]+\.[0-9]+( +[0-9]+\.[0-9]+)* *$' |
        sed -e "s#$BUILD_ROOT#ROOT#g" ${REF:+-e "s#$REF_ROOT#ROOT#g"} \
            -e "s#$WORK/inv-[a-z]*\.aig#INVARIANT#g"
}

verdict() {
    grep -m1 -o -E "(Invariant Verification|Verification Result): [A-Z]+" <<<"$1" || echo "-"
}

runs=0; failures=0; diffs=0; skipped=0
printf '%-22s %-17s %-6s %s\n' MODEL OPTIONS STATUS RESULT

for model in $MODELS; do
    path=$EXAMPLES/$model
    if [ ! -f "$path" ]; then
        echo "warning: $path not found, skipping" >&2; skipped=$((skipped + 1)); continue
    fi
    for opts in "${OPTION_SETS[@]}"; do
        runs=$((runs + 1))
        run "$BUILD" "$opts" "$path"; out=$OUT; rc=$RC
        status=ok
        if failed "$rc" "$out"; then status=FAIL; failures=$((failures + 1)); fi
        difflines=""
        if [ -n "$REF" ]; then
            run "$REF" "$opts" "$path"; ref_out=$OUT
            if [ "$(normalise <<<"$out")" != "$(normalise <<<"$ref_out")" ]; then
                [ "$status" = ok ] && status=DIFF
                diffs=$((diffs + 1))
                difflines=$(diff <(normalise <<<"$ref_out") <(normalise <<<"$out") | head -6)
            fi
        fi
        printf '%-22s %-17s %-6s %s\n' "$model" "$opts" "$status" "$(verdict "$out")"
        [ -n "$difflines" ] && sed 's/^/        /' <<<"$difflines"
    done
done

# The IC3 -> invariant -> certify flow.
for model in $FLOW_MODELS; do
    path=$EXAMPLES/$model
    [ -f "$path" ] || { echo "warning: $path not found, skipping" >&2; skipped=$((skipped + 1)); continue; }
    if [ ! -x "$(dirname "$BUILD")/pdtapp" ] || { [ -n "$REF" ] && [ ! -x "$(dirname "$REF")/pdtapp" ]; }; then
        runs=$((runs + 1)); failures=$((failures + 1))
        printf '%-22s %-17s %-6s %s\n' "$model" "ic3 + certify" FAIL "pdtapp not found next to the executable"
        continue
    fi
    runs=$((runs + 1))
    flow "$BUILD" "$path" build; ok=$FLOW_OK; out=$FLOW_OUT
    status=ok
    if [ "$ok" -ne 1 ]; then status=FAIL; failures=$((failures + 1)); fi
    difflines=""
    if [ -n "$REF" ]; then
        flow "$REF" "$path" ref
        if [ "$FLOW_OK" -ne "$ok" ] || ! cmp -s "$WORK/inv-build.aig" "$WORK/inv-ref.aig" ||
           [ "$(normalise <<<"$out")" != "$(normalise <<<"$FLOW_OUT")" ]; then
            [ "$status" = ok ] && status=DIFF
            diffs=$((diffs + 1))
            difflines="invariant or certify output differs from the reference"
        fi
    fi
    result="certified"; [ "$ok" -eq 1 ] || result="not certified"
    printf '%-22s %-17s %-6s %s\n' "$model" "ic3 + certify" "$status" "$result"
    [ -n "$difflines" ] && echo "        $difflines"
done

echo
summary="runs: $runs, failed: $failures"
[ -n "$REF" ] && summary="$summary, differing from reference: $diffs"
[ "$skipped" -gt 0 ] && summary="$summary, skipped: $skipped"
echo "$summary"
[ "$failures" -eq 0 ] && [ "$diffs" -eq 0 ] && [ "$runs" -gt 0 ]
