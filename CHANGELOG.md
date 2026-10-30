# changelog

all notable changes to proteus.

## [v1.2-preprint] - 2026-10-29
### added
- r3 atlas of parties infrastructure (tools/atlas_*)
- r3 g1-g7 observer models (passive, active, reconstruction)
- formal threat model with 6 attacker classes (A1-A6)
- learnability metric L with bootstrap CI, Cohen's d, power analysis
- hypotheses.md with registered falsification criteria
- theory.md updated with r2 findings (bypass boundary, timing CV justification)
- validate_r2.sh: honest report template, test_6_1 userspace assert
- systemd unit (packaging/proteusd.service)
- paper.md draft with limitations section

### fixed
- r2 sensor test flakiness: duration 150s, startup sleep 2s, post-scan sleep 1s
- test_4_4_determinism: uses PARTY REPLAY (-e fixture, no -r)
- seccomp: __NR_read=0 sentinel fixed (SECCOMP_ALLOW_N explicit count)
- seccomp_gen: static trace target, format-independent parser, unistd_64.h validation

### changed
- throughput reported as two metrics: headless (~250 sess/s) vs realtime tick (~33 sess/s)
- timing CV=52% documented as wire-level stochasticity, not side-channel leak
- bypass finding 1/3 detection + 100% ban hold = documented MTD boundary

### security
- seccomp profile: 31 syscalls from static binary strace
- nftables: netlink built by hand (libmnl, no libnftnl)
- IPC: SOCK_SEQPACKET + SO_PEERCRED + group auth

## [v1.1-r2-validated] - 2026-10-28
### added
- r2 nftables integration (libmnl netlink)
- r2 validation suite (tools/validate_r2.sh, 6 phases)
- traffic generators (scanner, legit_client, brute_forcer, mixed_traffic)
- seccomp mechanism (tools/seccomp_gen.sh)
- benchmarks (idle, throughput, latency, determinism)
- adversary phase (timing, state leakage, resource exhaustion, bypass)
- netns test infrastructure (setup/teardown scripts)

### fixed
- blocking accept() → O_NONBLOCK listener
- journal not flushed in realtime → setvbuf(_IOLBF)
- phantom wall tap at t=0 → edge detector priming
- noise_scale=0 → set to 1.0 in bridge

## [v1.0-r1] - 2026-10-27
### added
- threat detector (matched filter on AVA)
- modulator reactivity (octopamine ← scan density, serotonin ← legit share)
- journal v2 + events v2 (frozen schemas)
- IPC (proteusctl status/log/party/detector)
- detector calibration (seeds 101..110, FA budget 1/600s, threshold 30.0)
- move curve (hunger > fed 10/10 seeds at 0.5/1/2/4 Hz)

## [v0.9-r0] - 2026-10-26
### added
- repository skeleton, libpetri vendored (b436adb)
- minimal reflex: ASH → AVA → port move
- ten seed response test
- first commit
