# Changelog

All notable changes to Proteus.

## [v1.2-preprint] - 2026-10-29
### Added
- r3 Atlas of Parties infrastructure (tools/atlas_*)
- r3 g1-g7 observer models (passive, active, reconstruction)
- Formal threat model with 6 attacker classes (A1-A6)
- Learnability metric L with bootstrap CI, Cohen's d, power analysis
- hypotheses.md with registered falsification criteria
- theory.md updated with r2 findings (bypass boundary, timing CV justification)
- validate_r2.sh: honest report template, test_6_1 userspace assert
- systemd unit (packaging/proteusd.service)
- paper.md draft with limitations section

### Fixed
- r2 sensor test flakiness: duration 150s, startup sleep 2s, post-scan sleep 1s
- test_4_4_determinism: uses PARTY REPLAY (-e fixture, no -r)
- seccomp: __NR_read=0 sentinel fixed (SECCOMP_ALLOW_N explicit count)
- seccomp_gen: static trace target, format-independent parser, unistd_64.h validation

### Changed
- throughput reported as TWO metrics: headless (~250 sess/s) vs realtime tick (~33 sess/s)
- timing CV=52% documented as wire-level stochasticity, not side-channel leak
- bypass finding 1/3 detection + 100% ban hold = documented MTD boundary

### Security
- seccomp profile: 31 syscalls from static binary strace
- nftables: hand-built netlink (libmnl, no libnftnl)
- IPC: SOCK_SEQPACKET + SO_PEERCRED + group auth

## [v1.1-r2-validated] - 2026-10-28
### Added
- r2 nftables integration (libmnl netlink)
- r2 validation suite (tools/validate_r2.sh, 6 phases)
- traffic generators (scanner, legit_client, brute_forcer, mixed_traffic)
- seccomp mechanism (tools/seccomp_gen.sh)
- benchmarks (idle, throughput, latency, determinism)
- adversary phase (timing, state leakage, resource exhaustion, bypass)
- netns test infrastructure (setup/teardown scripts)

### Fixed
- Blocking accept() → O_NONBLOCK listener
- Journal not flushed in realtime → setvbuf(_IOLBF)
- Phantom wall tap at t=0 → edge detector priming
- noise_scale=0 → set to 1.0 in bridge

## [v1.0-r1] - 2026-10-27
### Added
- Threat detector (matched filter on AVA)
- Modulator reactivity (octopamine ← scan density, serotonin ← legit share)
- Journal v2 + events v2 (frozen schemas)
- IPC (proteusctl status/log/party/detector)
- Detector calibration (seeds 101..110, FA budget 1/600s, threshold 30.0)
- Move curve (hunger > fed 10/10 seeds at 0.5/1/2/4 Hz)

## [v0.9-r0] - 2026-10-26
### Added
- Repository skeleton, libpetri vendored (b436adb)
- Minimal reflex: ASH → AVA → port move
- Ten seed response test
- First commit
