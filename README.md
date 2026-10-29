# Proteus

**Bio-stochastic moving target defense driven by a living *C. elegans* connectome.**

The defense schedule (TCP port moves, firewall bans) is a function of a nervous system's internal state — nociception (ASH), command interneurons (AVA/AVB), neuromodulators (serotonin, octopamine, dopamine) — not a seeded PRNG.

**Status:** r2 sandbox-validated, r3 atlas + g1-g7 infrastructure built, r6 paper draft + packaging ready. Privileged bench (nftables kernel, g4 active probe, g6 transfer) pending.

---

## Quick Start

```bash
# Build
make

# Test (all green in sandbox)
make test

# Run daemon (headless, no network)
./proteusd -s 1 -d 10 --listen --socket /tmp/p.sock -o journal.csv

# Query status
./proteusctl status --socket /tmp/p.sock
```

## Architecture

| Component | Description |
|-----------|-------------|
| **petri** (vendored) | *C. elegans* connectome engine, graded potential, STSP, modulators |
| **bridge** | Engine ownership, noise on, edge priming, modulator injection |
| **detector** | Matched filter on AVA deviation (1s window, threshold 30.0 from FA budget) |
| **motor** | Deterministic port walk (40000–40063), O_NONBLOCK listener |
| **sensor** | Non-blocking accept, payload classification, userspace ban list (TTL) |
| **nft** | Hand-built netlink over libmnl (table `proteus`, set `banned`) |
| **seccomp** | 31 syscalls from static binary strace, BPF filter |
| **IPC** | SOCK_SEQPACKET, SO_PEERCRED, group auth |

## Stages

| Stage | Status | Description |
|-------|--------|-------------|
| R0 | ✅ | Skeleton, libpetri link, minimal reflex, 10 seed gate |
| R1 | ✅ | Detector + modulator reactivity, journal v2, IPC |
| R2 | ✅ (sandbox) | nftables + validation suite (6 phases) |
| R2-val | ⚠️ | Needs privileged bench for nft enforcement |
| R3 | 🔨 | Atlas of parties + g1–g7 hypotheses (infra built) |
| R4 | 📋 | Ablation studies (planned) |
| R5 | 📋 | Chess theater (deferred to R6) |
| R6 | 📝 | Packaging + paper draft (this version) |

## Key Results (Sandbox)

| Metric | Value | Note |
|--------|-------|------|
| Detector threshold | 30.0 | Frozen, FA budget 1/600s |
| Detection (fixture) | 7/10 | Wilson CI [0.40, 0.89] |
| Detection (wire, low-and-slow) | 1/3 | Published boundary |
| Ban hold rate | 100% | 0 sessions after ban row |
| Idle CPU | 7% | Realtime tick cost (r3 opt target <1%) |
| Throughput (headless) | ~250 sess/s | No -r flag |
| Throughput (realtime) | ~33 sess/s | With nanosleep |
| RSS | ~3.5 MB | Within 16 MB ceiling |
| Determinism | ✅ | Party replay bitwise-identical |

## Paper

See `paper.md` for:
- Formal threat model (6 attacker classes A1–A6)
- Learnability metric L
- Registered hypotheses g1–g7 with falsification criteria
- Honest limitations (evidence-backed)

## Packaging

```bash
# Install (requires root)
sudo make install

# systemd
systemctl daemon-reload
systemctl enable --now proteusd

# Version
make version
```

## Validation

```bash
# Full suite (requires root, netns, nftables, strace)
sudo tools/validate_r2.sh

# Quick pass
BENCH_DUR=120 DET_DUR=120 tools/validate_r2.sh

# Atlas (r3, headless, ~42h for 250 parties)
make atlas

# g1-g7 hypotheses (r3, headless, ~72h)
make phase3
```

## Reproducibility

Every experiment emits `manifest.txt` with binary SHA256, compiler, CFLAGS, seed range, schedule hash, output file hashes.

```bash
# Verify a party replay
./proteusd -s 42 -d 600 -e tests/fixtures/burst.csv --headless > journal.csv
```

## License

ISC — see `LICENSE`.

## Citation

If you use Proteus in research, please cite the preprint (arXiv forthcoming) and the primary sources for the connectome engine:
- Yemini et al. 2013 (speed/curvature)
- Pierce-Shimomura et al. 1999 (ASH nociceptor)
- Kato et al. 2015 (AVA transient)
- Tsodyks & Markram 1997 (STSP)
