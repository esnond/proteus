#!/bin/sh
# r2 benchmarks: idle and under attack resource use, legit throughput
# and connect latency. thresholds from the r2 spec: idle under 1 percent
# cpu and 10 mb rss, under attack under 10 percent and 20 mb, throughput
# above 100 sessions per second, latency under 100 ms. writes
# results/benchmark.csv. the loopback bench runs without privileges.
set -e
cd "$(dirname "$0")/.."
make -s tools proteusd
mkdir -p results
OUT=results/benchmark.csv
echo "scenario,cpu_percent,rss_kb,throughput_cps,connect_ms_avg" > "$OUT"

read_status() {
    ./proteusctl status --socket /tmp/bench.sock
}

# part one, resource use: the same daemon idle and under a mass scan
./proteusd -s 1 -d 14 -r --listen --socket /tmp/bench.sock -o /dev/null &
PD=$!
sleep 2
S=$(read_status)
RSS=$(echo "$S" | grep -o 'rss_kb=[0-9]*' | cut -d= -f2)
CPU=$(echo "$S" | grep -o 'cpu_ms=[0-9]*' | cut -d= -f2)
T_MS=$(echo "$S" | grep -o 't_ms=[0-9]*' | cut -d= -f2)
echo "idle,$(python3 -c "print(f'{$CPU / max(1, $T_MS) * 100:.2f}')"),$RSS,0,0" \
    >> "$OUT"

./scanner ip=127.0.0.1 first_port=40000 ports=64 rate=50 duration=4 \
    > /dev/null 2>&1 &
SC=$!
sleep 4
S=$(read_status)
RSS=$(echo "$S" | grep -o 'rss_kb=[0-9]*' | cut -d= -f2)
CPU=$(echo "$S" | grep -o 'cpu_ms=[0-9]*' | cut -d= -f2)
T_MS=$(echo "$S" | grep -o 't_ms=[0-9]*' | cut -d= -f2)
echo "under_attack,$(python3 -c "print(f'{$CPU / max(1, $T_MS) * 100:.2f}')"),$RSS,0,0" \
    >> "$OUT"
wait "$SC" 2>/dev/null || true
kill "$PD" 2>/dev/null || true
wait "$PD" 2>/dev/null || true

# part two, throughput and latency: a fresh daemon, back to back short
# sessions with payload, the probe reads the live port from the status
# channel because the worm moves the port on its own
./proteusd -s 2 -d 12 -r --listen --socket /tmp/bench.sock -o /dev/null &
PD=$!
sleep 1
PORT=$(read_status | grep -o 'port=[0-9]*' | cut -d= -f2)
PORT=${PORT:-40000}
export PORT
python3 - <<'PYEOF' | tail -1 >> "$OUT"
# the probe runs 8 threads: one python loop cannot saturate the daemon
import os, socket, time
from concurrent.futures import ThreadPoolExecutor

def burst(_):
    n, lat = 0, []
    t0 = time.monotonic()
    while time.monotonic() - t0 < 3.0:
        c = time.monotonic()
        try:
            s = socket.create_connection(("127.0.0.1", int(os.environ["PORT"])), timeout=0.5)
        except OSError:
            break
        s.sendall(b"PROTEUS HELLO\n")
        lat.append((time.monotonic() - c) * 1000)
        s.close()
        n += 1
    return n, lat

wall = time.monotonic()
with ThreadPoolExecutor(max_workers=8) as ex:
    res = list(ex.map(burst, range(8)))
el = max(time.monotonic() - wall, 0.001)
n = sum(r[0] for r in res)
lat = [x for r in res for x in r[1]]
print(f"throughput,0,0,{n/el:.1f},{sum(lat)/max(1, len(lat)):.2f}")
PYEOF
kill "$PD" 2>/dev/null || true
wait "$PD" 2>/dev/null || true

cat "$OUT"
echo DONE
