#!/bin/sh
# roc_sweep: generate labeled runs for the detector score ROC.
# sudo not needed; the daemon binds loopback only.
#
# tools/roc_sweep.sh [runs] [outdir]
set -u
cd "$(dirname "$0")/.."
RUNS=${1:-10}
OUT=${2:-results/roc}
# the FP bound needs ~6000 legit sessions for 3/n <= 5 per 10k
LEGIT_SECS=${LEGIT_SECS:-60}
mkdir -p "$OUT"
rm -f "$OUT"/legit_*.csv "$OUT"/attack_*.csv
: > "$OUT/counts.csv"
make -s proteusd proteusctl mixed_traffic scanner || exit 1
i=0
while [ "$i" -lt "$RUNS" ]; do
    i=$((i + 1))
    rm -f /tmp/roc.sock
    ./proteusd -s $((700 + i)) -d 40 -r --listen --socket /tmp/roc.sock \
        -o "$OUT/legit_$i.csv" 2>/dev/null &
    P=$!
    sleep 1.5
    ./mixed_traffic ip=127.0.0.1 port=40000 duration="$LEGIT_SECS" \
        sock=/tmp/roc.sock scan_rate=0 legit_rate=20 \
        > "$OUT/legit_traffic_$i.csv" 2>&1 || true
    n=$(grep -c ',data_sent$' "$OUT/legit_traffic_$i.csv" || true)
    echo "legit,$i,${n:-0}" >> "$OUT/counts.csv"
    kill "$P" 2>/dev/null; wait "$P" 2>/dev/null

    rm -f /tmp/roc.sock
    ./proteusd -s $((700 + i)) -d 40 -r --listen --socket /tmp/roc.sock \
        -o "$OUT/attack_$i.csv" 2>/dev/null &
    P=$!
    sleep 1.5
    ./scanner ip=127.0.0.1 first_port=40000 ports=64 rate=50 duration=10 \
        > /dev/null 2>&1 || true
    sleep 1
    kill "$P" 2>/dev/null; wait "$P" 2>/dev/null
done
python3 tools/roc_sweep.py "$OUT" "${FP_BUDGET:-5}"
