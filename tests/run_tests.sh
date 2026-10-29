#!/bin/sh
# proteus r0 ci gate. engineering invariants only: strict build, bitwise
# determinism, frozen journal schema, port move mechanics, finite values,
# ubsan. the scientific r0 verdict (ava response to a scan burst) is
# measured here and published in results/reflex_r0.log with both the
# paired statistical measure and the behavioral significance caveat.
set -e
cd "$(dirname "$0")/.."
# the same libmnl resolution as the makefile: pkg-config first, the
# local deps build second
NFT_CFLAGS=$(pkg-config --cflags libmnl 2>/dev/null || true)
NFT_LIBS=$(pkg-config --libs libmnl 2>/dev/null || true)
if [ -z "$NFT_CFLAGS" ] && [ -f "$HOME/deps/usr/lib/libmnl.a" ]; then
    NFT_CFLAGS="-I$HOME/deps/usr/include"
    NFT_LIBS="-L$HOME/deps/usr/lib -lmnl"
fi
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
    grep -q '^# t_ms,act_ava,act_avb,ash_trace,ser,oct,dop,det_score,port,cause,event$' || {
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
awk -F, 'NR>2 && $10 != 0 { ok = 1 } END { exit !ok }' \
    "$TMP/move.csv" || { echo "FAIL: no port move fired"; exit 1; }
awk -F, 'NR>2 && ($9 < 40000 || $9 >= 40064) { bad = 1 } END { exit bad }' \
    "$TMP/move.csv" || { echo "FAIL: port outside the window"; exit 1; }
echo "PASS: port move fires, ports stay in the window ($MODE)"

echo "== detector measurement, ten seeds =="
for s in 1 2 3 4 5 6 7 8 9 10; do
    ./proteusd -s "$s" -d 8 -e tests/fixtures/burst.csv -o "$TMP/burst_$s.csv"
done
# run from the journal directory so the published log carries no
# throwaway temp paths, the report stays byte identical between runs
ROOT="$PWD"
(cd "$TMP" && "$ROOT/proteusctl" detector burst_*.csv) \
    | tee results/detector_r1.log
grep -q '^detected=' results/detector_r1.log \
    || { echo "FAIL: detector produced no detection line"; exit 1; }
echo "PASS: detector report published in results/detector_r1.log"

echo "== party bitwise replay =="
./proteusctl party 7 -d 6 -e tests/fixtures/burst.csv --against "$TMP/det_a.csv" \
    || { echo "FAIL: party replay differs"; exit 1; }
echo "PASS: party replay identical"

echo "== ipc status and memory gate =="
GID=$(id -g)
./proteusd -s 3 -d 5 -r --socket "$TMP/st.sock" --group "$GID" \
    -e tests/fixtures/burst.csv -o "$TMP/st.csv" &
PD=$!
sleep 2
./proteusctl status --socket "$TMP/st.sock" | tee "$TMP/status.txt"
wait $PD
grep -q '^port=4' "$TMP/status.txt" || { echo "FAIL: status has no port"; exit 1; }
RSS=$(grep -o 'rss_kb=[0-9]*' "$TMP/status.txt" | cut -d= -f2)
if [ "$RSS" -gt 16384 ]; then
    echo "FAIL: resident memory ${RSS} kb above the 16 mb ceiling"
    exit 1
fi
echo "PASS: status serves, rss ${RSS} kb within the 16 mb ceiling"

echo "== detector report =="
./proteusctl detector "$TMP/move.csv" | grep -q '^threshold=' \
    || { echo "FAIL: detector report broken"; exit 1; }
echo "PASS: detector report prints"

echo "== r2 sensor: targeted scan then ban =="
# the targeted attacker reads the current port from the status channel
# and sweeps it; the daemon moves the port on its own reversals, so the
# attacker re reads the port every round
make -s tools
./proteusd -s 4 -d 150 -r --listen --socket "$TMP/s1.sock" \
    -o "$TMP/s1.csv" &
PD=$!
sleep 2
n=0
while [ "$n" -lt 25 ]; do
    sleep 0.5
    P=$(./proteusctl status --socket "$TMP/s1.sock" \
        | grep -o 'port=[0-9]*' | cut -d= -f2)
    [ -n "$P" ] || break
    timeout 4 ./scanner ip=127.0.0.1 first_port="$P" ports=1 rate=6 \
        duration=2 > /dev/null 2>&1
    # allow sensor tick + detector to process the scan and emit ban event
    sleep 1
    grep -q ',8$' "$TMP/s1.csv" && break
    # the ava adaptation needs its two seconds back before the next
    # round, dense sweeps drive the response below the threshold
    sleep 2.5
    n=$((n + 1))
done
m=0
while kill -0 "$PD" 2>/dev/null && [ "$m" -lt 30 ]; do
    sleep 0.5
    m=$((m + 1))
done
kill -9 "$PD" 2>/dev/null || true
wait "$PD" 2>/dev/null || true
grep -q ',6$' "$TMP/s1.csv" || { echo "FAIL: sensor saw no scan"; exit 1; }
grep -q ',8$' "$TMP/s1.csv" || { echo "FAIL: detector banned nobody"; exit 1; }
echo "PASS: scan detected and ip banned (userspace)"

echo "== r2 sensor: a fed session that ends is not a scan =="
# the ban path attributes the newest scan source. a session that sent
# payload and then closed must not become that source, or mixed traffic
# bans the legitimate client instead of the scanner.
./proteusd -s 12 -d 20 -r --listen --socket "$TMP/s2.sock" \
    -o "$TMP/s2.csv" &
PD=$!
sleep 2
# the worm may move between the port read and the connect, retry
TRIES=0
while [ "$TRIES" -lt 3 ]; do
    P=$(./proteusctl status --socket "$TMP/s2.sock" \
        | grep -o 'port=[0-9]*' | cut -d= -f2)
    P=${P:-40000}
    if python3 - "$P" <<'PYEOF'
import socket, sys, time
s = socket.create_connection(("127.0.0.1", int(sys.argv[1])), timeout=2)
s.sendall(b"PROTEUS HELLO\n")
time.sleep(0.5)
s.close()
PYEOF
    then
        break
    fi
    TRIES=$((TRIES + 1))
    sleep 1
done
sleep 1
kill -9 "$PD" 2>/dev/null || true
wait "$PD" 2>/dev/null || true
if ! grep -q ',7$' "$TMP/s2.csv"; then
    echo "FAIL: the fed session produced no session event"; exit 1
fi
if grep -q ',6$' "$TMP/s2.csv"; then
    echo "FAIL: session eof counted as a scan"; exit 1
fi
echo "PASS: session event only, no scan event on eof"

echo "== r2.5 immune scope: register, live pid, revoke on exit =="
rm -f "$TMP/s3.sock"
./proteusd -s 21 -d 25 -r --listen --socket "$TMP/s3.sock" \
    -o /dev/null 2>/dev/null &
PD=$!
sleep 2
P=$(./proteusctl status --socket "$TMP/s3.sock" \
    | grep -o 'port=[0-9]*' | cut -d= -f2)
P=${P:-40000}
./legit_client ip=127.0.0.1 port="$P" sessions=1 hold_ms=1500 gap_ms=0 \
    regsock="$TMP/s3.sock" > /dev/null 2>&1 &
LG=$!
sleep 0.4
TR=$(./proteusctl status --socket "$TMP/s3.sock" | grep -o 'trust=[0-9]*')
wait "$LG" 2>/dev/null || true
sleep 0.8
TR2=$(./proteusctl status --socket "$TMP/s3.sock" | grep -o 'trust=[0-9]*')
kill "$PD" 2>/dev/null || true; wait "$PD" 2>/dev/null || true
if [ "$TR" != "trust=1" ]; then
    echo "FAIL: live service not registered ($TR)"; exit 1
fi
if [ "$TR2" != "trust=0" ]; then
    echo "FAIL: token survived the process ($TR2)"; exit 1
fi
echo "PASS: token live with the pid, revoked when it exits"

echo "== r2 netns =="
if unshare -n true 2>/dev/null || ip netns add __probe 2>/dev/null; then
    ip netns del __probe 2>/dev/null || true
    sh tools/netns_setup.sh
    ip netns exec proteus_test sh -c "cd $PWD && make -s proteusd && ./proteusd -s 5 -d 3 -o /dev/null" &
    wait $! || { echo "FAIL: daemon inside netns"; sh tools/netns_teardown.sh; exit 1; }
    sh tools/netns_teardown.sh
    echo "PASS: daemon runs inside the namespace"
else
    echo "SKIP: netns unavailable in this sandbox, no cap_net_admin,"
    echo "the privileged bench is the acceptance gate for the real"
    echo "firewall rules, see tools/netns_setup.sh requirements"
fi

echo "== ubsan =="
cc -std=c99 -pedantic -Wall -Wextra -O1 -g -fsanitize=undefined \
   -ffp-contract=off -D_DEFAULT_SOURCE -Isrc -Isrc/petri ${NFT_CFLAGS} \
   -o "$TMP/proteusd_ubsan" src/proteusd.c \
   src/bridge.c src/detector.c src/events.c src/ipc.c src/journal.c \
   src/motor.c src/nft.c src/rate.c src/reflex.c src/seccomp.c \
   src/sensor.c \
   src/session.c src/trust.c \
   src/petri/body.c src/petri/connectome.c src/petri/gcamp.c \
   src/petri/metrics.c src/petri/modulators.c src/petri/mutants.c \
   src/petri/neuron.c src/petri/plasticity.c src/petri/rng.c \
   src/petri/sim.c src/petri/world.c src/petri/evolution.c -lm \
   ${NFT_LIBS}
"$TMP/proteusd_ubsan" -s 5 -d 5 -e tests/fixtures/burst.csv \
    -o "$TMP/ub.csv" 2>"$TMP/ub.log"
if grep -qE "runtime error|warning" "$TMP/ub.log"; then
    echo "FAIL: ubsan flagged undefined behavior"; cat "$TMP/ub.log"; exit 1
fi
echo "PASS: ubsan clean"

echo ""
echo "ALL TESTS GREEN"
