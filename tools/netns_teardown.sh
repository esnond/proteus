#!/bin/sh
# r2 netns teardown, the mirror of netns_setup.sh
set -e
NS="${1:-proteus_test}"
# ip netns del does not kill the processes inside, orphaned daemons
# would keep their namespace alive invisibly
for p in $(ip netns pids "$NS" 2>/dev/null); do
    kill "$p" 2>/dev/null || true
done
sleep 0.5
for p in $(ip netns pids "$NS" 2>/dev/null); do
    kill -9 "$p" 2>/dev/null || true
done
ip netns del "$NS" 2>/dev/null || true
ip link del veth0 2>/dev/null || true
echo "netns $NS removed"
