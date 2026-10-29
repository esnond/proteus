#!/bin/sh
# bench_gcore_test: close the gcore gap on a root bench.
#
# sudo tools/bench_gcore_test.sh
#
# installs gdb when neither gcore nor gdb is present, starts the daemon,
# makes it fire a ban, dumps the core, audits it with tools/dump_audit.sh.
# exit 0 = audit clean, 1 = leak or tool failure.
set -e
cd "$(dirname "$0")/.."
D=${DUMP_DIR:-results/gcore}
mkdir -p "$D"
# running under sudo builds and writes as root; hand the tree back to the
# invoking user or the next non-root make fails on the root owned archive
if [ -n "${SUDO_UID:-}" ]; then
    trap 'chown -R "$SUDO_UID:$SUDO_GID" "$D" . 2>/dev/null || true' EXIT
fi
# stale cores from an earlier run would let a failed dump look clean
rm -f "$D"/memory_dump.* "$D"/*.core
if ! command -v gcore >/dev/null 2>&1 && ! command -v gdb >/dev/null 2>&1; then
    echo "installing gdb"
    apt-get update -qq
    apt-get install -y -qq gdb
fi
make -s proteusd
./proteusd -s 5 -d 90 -r --listen --socket /tmp/gcore_test.sock \
    -o "$D/journal.csv" 2>"$D/daemon.log" &
PD=$!
sleep 5
./scanner ip=127.0.0.1 first_port=40000 ports=64 rate=50 duration=10 \
    > "$D/scanner.csv" 2>&1 || true
sleep 2
if command -v gcore >/dev/null 2>&1; then
    gcore -o "$D/memory_dump" "$PD" >> "$D/daemon.log" 2>&1 || true
else
    gdb -p "$PD" -batch -ex "gcore $D/memory_dump.core" \
        >> "$D/daemon.log" 2>&1 || true
fi
kill "$PD" 2>/dev/null || true
CORE=$(ls -t "$D"/memory_dump.* "$D"/*.core 2>/dev/null | head -n 1 || true)
if [ -z "$CORE" ] || [ ! -s "$CORE" ]; then
    echo "FAIL: no core produced, see $D/daemon.log"
    exit 1
fi
sh tools/dump_audit.sh "$CORE" "# proteus journal"
