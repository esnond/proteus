#!/bin/sh
# r2 attack scenarios, the privileged part of the gate: real netns, real
# nftables, the four wire level cases of the spec. runs on the ci runner
# and on any bench with cap_net_admin. reports csv to stdout.
set -e
cd "$(dirname "$0")/.."
# iproute2 keeps namespace handles under /run/netns, which only root can
# create. an unprivileged caller gets a skip, not a red job: the bench
# runs `sudo tools/validate_r2.sh`, the ci runner runs this under sudo.
if [ "$(id -u)" -ne 0 ]; then
    echo "SKIP: netns needs root, run: sudo sh tests/run_attack_scenarios.sh"
    exit 0
fi
make -s proteusd tools

sh tools/netns_setup.sh
trap 'sh tools/netns_teardown.sh' EXIT

HOST_IP=10.0.0.1
GUEST_IP=10.0.0.2
DUR=12
mkdir -p results

echo "scenario,metric,value"

# proteusd inside the namespace, real nftables on, the attacker and the
# legit client run from the host side of the veth pair
ip netns exec proteus_test sh -c \
    "./proteusd -s 7 -d $DUR -r --listen --nft -o results/atk_scenarios.csv & sleep 1"

run_scan() {
    ./scanner "ip=$GUEST_IP" first_port=40000 ports=1 rate=6 duration=4 \
        > results/scan_s.csv 2>/dev/null || true
}

# scenario 1: mass scan from the guest, expect detection and ban
run_scan
SCAN_ROWS=$(grep -c scan results/scan_s.csv || true)
echo "s1_mass_scan,packets_sent,$SCAN_ROWS"

# scenario 2: legit client from a clean address, expect acceptance
./legit_client "ip=$GUEST_IP" port=40000 sessions=3 hold_ms=200 gap_ms=100 \
    > results/legit_s.csv 2>/dev/null || true
LEGIT_OK=$(grep -c data_sent results/legit_s.csv || true)
echo "s2_legit,accepted_sessions,$LEGIT_OK"

# scenario 3: adaptive attacker reads the port from the journal, the
# prediction quality is the r3 atlas business, here: the port moved at
# least once while under observation
MOVES=$(grep -c ',[123],$' results/atk_scenarios.csv 2>/dev/null || true)
echo "s3_adaptive,port_moves_observed,$MOVES"

# scenario 4: flood, the daemon must stay responsive, measured by the
# number of sessions the legit client still completes under load
( ./brute_forcer "ip=$GUEST_IP" port=40000 attempts=40 rate=15 \
    > results/brute_s.csv 2>/dev/null || true ) &
BR=$!
./legit_client "ip=$GUEST_IP" port=40000 sessions=5 hold_ms=100 gap_ms=100 \
    > results/legit_flood.csv 2>/dev/null || true
wait $BR 2>/dev/null || true
FLOOD_OK=$(grep -c data_sent results/legit_flood.csv || true)
echo "s4_flood,legit_throughput,$FLOOD_OK"

# the ban verification lives in the atk journal, cause and event
# columns carry it
echo "journal_entries,$(wc -l < results/atk_scenarios.csv)"
echo DONE
