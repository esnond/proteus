#!/bin/sh
# proteus r0 ci gate. engineering invariants only: strict build, bitwise
# determinism, frozen journal schema, port move mechanics, finite values,
# ubsan. the scientific r0 verdict (ava response to a scan burst) is
# measured here and published in results/reflex_r0.log with both the
# paired statistical measure and the behavioral significance caveat.
set -e
cd "$(dirname "$0")/.."
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
mkdir -p results

echo "== build strict =="
make clean >/dev/null
make > "$TMP/build.log" 2>&1 || { tail -30 "$TMP/build.log"; echo "FAIL: make"; exit 1; }
if grep -qE "warning:|error:" "$TMP/build.log"; then
    echo "FAIL: build has warnings"
    grep -E "warning:|error:" "$TMP/build.log" | head -5
    exit 1
fi
echo "PASS: clean build"

echo "== determinism, with events =="
./proteusd -s 7 -d 6 -e tests/fixtures/burst.csv -o "$TMP/det_a.csv"
./proteusd -s 7 -d 6 -e tests/fixtures/burst.csv -o "$TMP/det_b.csv"
[ -s "$TMP/det_a.csv" ] || { echo "FAIL: determinism run produced no rows"; exit 1; }
cmp "$TMP/det_a.csv" "$TMP/det_b.csv"
echo "PASS: bitwise identical party journals"

echo "== determinism, baseline =="
./proteusd -s 9 -d 6 -o "$TMP/db_a.csv"
./proteusd -s 9 -d 6 -o "$TMP/db_b.csv"
cmp "$TMP/db_a.csv" "$TMP/db_b.csv"
echo "PASS: baseline identical"

echo "== journal schema freeze =="
sed -n 2p "$TMP/det_a.csv" | \
    grep -q '^# t_ms,act_ava,act_avb,ash_trace,ser,oct,dop,port,event$' || {
    echo "FAIL: journal header drifted from the frozen contract"; exit 1; }
echo "PASS: journal header frozen"

echo "== nan guard =="
if grep -qi 'nan\|inf' "$TMP/det_a.csv"; then
    echo "FAIL: non finite values in journal"; exit 1
fi
echo "PASS: finite values"

echo "== port move =="
# the spec keeps network layer tests in a private netns, the host is never
# configured. when the sandbox denies unshare, the check degrades to a
# loopback bind inside our own process and says so out loud. proteusd
# binds 127.0.0.1 only.
if unshare -n true 2>/dev/null; then
    NS="unshare -n"
    MODE="netns"
else
    NS=""
    MODE="loopback fallback, netns unavailable, no host config touched"
    echo "NOTE: $MODE"
fi
$NS sh -c "ip link set lo up 2>/dev/null; ./proteusd -s 3 -d 8 -e tests/fixtures/burst.csv -o '$TMP/move.csv'"
grep -qE ',[23]$' "$TMP/move.csv" || { echo "FAIL: no port move fired"; exit 1; }
awk -F, 'NR>2 && ($8 < 40000 || $8 >= 40064) { bad = 1 } END { exit bad }' \
    "$TMP/move.csv" || { echo "FAIL: port outside the window"; exit 1; }
echo "PASS: port move fires, ports stay in the window ($MODE)"

echo "== reflex measurement, ten seeds =="
for s in 1 2 3 4 5 6 7 8 9 10; do
    ./proteusd -s "$s" -d 8 -o "$TMP/base_$s.csv"
    ./proteusd -s "$s" -d 8 -e tests/fixtures/burst.csv -o "$TMP/burst_$s.csv"
done
PAIRS=""
for s in 1 2 3 4 5 6 7 8 9 10; do
    PAIRS="$PAIRS $TMP/base_$s.csv $TMP/burst_$s.csv"
done
# shellcheck disable=SC2086
./proteusctl reflex $PAIRS | tee results/reflex_r0.log
COUNT=$(grep -o 'count_pos=[0-9]*' results/reflex_r0.log | cut -d= -f2)
if [ "$COUNT" -lt 8 ]; then
    echo "FAIL: r0 exit criterion not met, $COUNT of 10 seeds"
    exit 1
fi
echo "PASS: paired ava response distinguishable in $COUNT of 10 seeds"
echo "NOTE: the paired measure is microvolt scale, the behavioral reversal"
echo "readout is absent in the current engine calibration, see readme"
echo "limitations and results/reflex_r0.log before citing this pass"

echo "== ubsan =="
cc -std=c99 -pedantic -Wall -Wextra -O1 -g -fsanitize=undefined \
   -ffp-contract=off -D_DEFAULT_SOURCE -Isrc -Isrc/petri \
   -o "$TMP/proteusd_ubsan" src/proteusd.c \
   src/bridge.c src/events.c src/journal.c src/motor.c src/reflex.c \
   src/petri/body.c src/petri/connectome.c src/petri/gcamp.c \
   src/petri/metrics.c src/petri/modulators.c src/petri/mutants.c \
   src/petri/neuron.c src/petri/plasticity.c src/petri/rng.c \
   src/petri/sim.c src/petri/world.c src/petri/evolution.c -lm
"$TMP/proteusd_ubsan" -s 5 -d 5 -e tests/fixtures/burst.csv \
    -o "$TMP/ub.csv" 2>"$TMP/ub.log"
if grep -qE "runtime error|warning" "$TMP/ub.log"; then
    echo "FAIL: ubsan flagged undefined behavior"; cat "$TMP/ub.log"; exit 1
fi
echo "PASS: ubsan clean"

echo ""
echo "ALL TESTS GREEN"
