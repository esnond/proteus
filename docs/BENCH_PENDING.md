# privileged bench: not run, and exactly how to run it

this file exists because the four gaps in paper/FINAL.md named a privileged
bench, an n=30 entropy run, a comparison with published animal behaviour,
and an independent reader. the entropy run and the two documents are done
elsewhere. this one cannot be done from the agent sandbox: `sudo -n` needs a
password and the kernel objects it exercises (`cap_net_admin`, netfilter,
gcore) are not reachable without root.

what follows is the complete procedure, so a machine with root produces the
missing evidence without re-deciding anything.

## what the bench is for

three numbers currently labelled as pending, not measured:

1. kernel enforcement: does the `proteus` nftables set actually drop the
   banned source, and does the ban expire at BAN_TTL_MS wall seconds.
2. active probing: what does an attacker who probes on purpose pay, and does
   the schedule respond to it (h13 coupling, KL divergence).
3. idle cpu and wake latency: the 0.75 percent figure in README is the
   arithmetic bound 6 x 1.25 ms per 1000 ms poll, not a measurement.

## procedure

```sh
cd ~/proteus
make test                      # sandbox suite, must be green first
sudo tools/netns_setup.sh      # namespace, veth pair, 10.0.0.0/24
sudo tools/validate_r2.sh      # six phases, writes results/r2_validation/
sudo tools/bench_gcore_test.sh # dump audit, expects memory_dump.PID only
sudo tools/roc_sweep.sh 10     # false positive upper bound on 10 seeds
```

for the active probe arm, in a second terminal:

```sh
sudo ip netns exec proteus-ns ./proteusd -s 2001 -d 600 --listen \
    --nft --addr 10.0.0.2 -o /tmp/party_probe.csv
sudo ip netns exec proteus-client tools/observer_active.py \
    --profile probe --duration 600 --quiet
```

for the idle measurement, on a quiet party with no events:

```sh
/usr/bin/time -v sudo ip netns exec proteus-ns ./proteusd -s 2002 -d 300 \
    --listen --nft --addr 10.0.0.2 -o /tmp/idle.csv 2> /tmp/idle.time
```

## acceptance criteria, frozen before the run

| arm | criterion | what to keep as evidence |
|---|---|---|
| kernel ban | banned source gets zero completed handshakes after the ban row; set membership present in the kernel for BAN_TTL_MS | `nft list set proteus banned` before, during and after; journal rows with cause and event |
| ban expiry | a source is accepted again only after BAN_TTL_MS wall seconds, not model seconds | two timestamps from the journal and one from the set dump |
| active probe | KL divergence between next-move distributions with and without probing is at least 0.1 bit with a bootstrap 95 percent interval excluding zero, 30 paired seeds | the two move sequences per seed and the estimator's output |
| idle cpu | measured percent, not the bound; report mean and 95 percent interval over 5 runs of 300 s | `/usr/bin/time -v` maximum resident and user plus system seconds |
| gcore | no core file contains the key or the port sequence in clear | `tools/dump_audit.sh` output and file hashes |

## how to report the result

write `results/bench_2026/PROTOCOL.md` with the commit, the binary sha256,
the kernel version, and one row per arm: criterion, measured value, verdict,
and the artifact hash. do not adjust a criterion after seeing the data; if
an arm fails, publish the failure.

## what this bench cannot fix

the paper's other gaps are not bench problems: n=1 reconnaissance numbers
stay n=1 until a multi seed run exists, and a comparison with animal
behaviour is a literature comparison, not an experiment. see
paper/BEHAVIOUR_COMPARISON.md and results/entropy_n30/.
