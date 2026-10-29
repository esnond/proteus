#!/bin/sh
# r2 netns setup: an isolated network namespace for the defense tests,
# the host is never touched. requires root or cap_net_admin; when the
# environment denies it the caller must print SKIP, not fail silently.
set -e
NS="${1:-proteus_test}"
# iproute2 stores namespace handles under /run/netns. on a normal root
# bench that directory is writable; inside a user namespace it is owned
# by the initial root, so mount a private tmpfs over it when needed
if [ ! -w /run/netns ]; then
    mkdir -p /run/netns 2>/dev/null || true
    mount -t tmpfs tmpfs /run/netns 2>/dev/null || true
fi
# a leftover namespace from an interrupted run would block the setup
# and every daemon start after it
for p in $(ip netns pids "$NS" 2>/dev/null); do
    kill "$p" 2>/dev/null || true
done
ip netns del "$NS" 2>/dev/null || true
ip link del veth0 2>/dev/null || true
ip netns add "$NS"
ip link add veth0 type veth peer name veth1
ip link set veth1 netns "$NS"
ip addr add 10.0.0.1/24 dev veth0
ip link set veth0 up
# the guest address lives on the veth peer, not on lo: with it on lo the
# arp from the host finds no interface and the veth path stays dead
ip netns exec "$NS" ip addr add 10.0.0.2/24 dev veth1
ip netns exec "$NS" ip link set veth1 up
ip netns exec "$NS" ip link set lo up
echo "netns $NS ready: host 10.0.0.1, guest 10.0.0.2"
