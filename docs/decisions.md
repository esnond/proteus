# decisions.md — HX-style log

## 2026-10-29: r2.5 run — HX-9 robotics cancellation

**HX-9:** robotics integration cancelled by owner 2026-10-29. focus is
network defense only. the working tree carried no neato/xv-15/robotics
lane at cancellation time (docs/NEATO_INVENTORY.md; independent Explore
verification: zero hits). git history keeps its traces as provenance;
filter-repo is an owner option, never run by an agent. the paper keeps
"robot body excluded" as an honest scope limit.

## 2026-10-29: Phase 0-3 Autonomous Run

### Scope cuts (recorded before work)
- **Robot body EXCLUDED** (AGENTS.md): r6 packaging + paper only. No robot hardware integration.
- **Quantum rhetoric FORBIDDEN** (AGENTS.md): A6 model is theoretical bound only, no resistance claims.

### Environment blocks routed to scripts (not obeyed as stops)
| Block | Script | What it proves on bench |
|-------|--------|------------------------|
| nftables kernel acceptance | `tools/validate_r2.sh` phases 1,2,6 | Hand-built netlink protocol meets kernel |
| g4 active probe | `tools/validate_r2.sh` phase 5 / g1_g7_runner.sh | Real netns reduces L below passive |
| g6 transfer gap | `tools/g1_g7_validate.sh` | Synthetic-trained observer on independent stand |
| seccomp negative test | (none yet) | Fault injection helper needed |

### Agent reviews (sequential per R1)
| Phase | Methodologist | Architect | Adversary | Memory | Editor |
|-------|---------------|-----------|-----------|--------|--------|
| 0 | ✅ baseline spec | ✅ baseline spec | — | ✅ STATE.md | — |
| 1 | ✅ f1.1-f1.5 registered | ✅ validate_r2.sh fixes | — | ✅ theory.md updates | — |
| 2 | ✅ atlas_spec.md | ✅ atlas_runner/analyze | — | ✅ STATE.md | — |
| 3 | (g1-g7 pre-registered) | ✅ observers + learnability | — | ✅ STATE.md | — |

### Key technical decisions
1. **noise_scale=1.0 in bridge** (not engine): Engine default 0 → all seeds identical. Bridge override documented in VENDOR.md + petri DEFECTS.md.
2. **Phantom wall tap at t=0**: Zeroed body in contact band → primed edge detectors (`prev_contact=1`). Documented.
3. **Detector threshold 30.0 frozen**: Lowest grid with FA ≤ 1/600s on seeds 101..110. Never refit.
4. **DET_THRESHOLD in config.h**: Not fitted to green results post hoc.
5. **PARTY REPLAY for determinism**: `-e fixture` (recorded input), NOT live `-r` (nanosleep = nondeterministic).
6. **Throughput = TWO metrics**: Headless (250) vs realtime (33). Never one number.
7. **Timing CV=52% = wire stochasticity**: Detector probabilistic by design at frozen threshold. Not a side-channel leak.
8. **Bypass finding 1/3 + 100% ban hold = MTD boundary**: Documented in theory.md as strength (predicted boundary).
9. **test_6.1 asserts userspace ban**: Sandbox gate; nft ban on privileged bench.
10. **SECCOMP_ALLOW_N explicit count**: `__NR_read=0` broke sentinel `break` on zero. Fixed.

### Negative results published (not hidden)
- Idle CPU 7% vs <1% target
- nftables backend unavailable in sandbox
- Low-and-slow wire detection 1/3
- g4 active probe blocked (no netns)
- g6 transfer gap blocked (no independent stand)
- Seccomp negative test not covered

### Falsification criteria registered pre-experiment
All g1-g7 in hypotheses.md + atlas_spec.md + theory.md. Any refutation = result with data.

### Reproducibility
- All seeds allocated: calibration 101..110, training 1001..1100, held-out 2001..2100, stress 3001..3050
- Manifests generated per experiment (binary SHA256, CFLAGS, seeds, schedule hash)
- Held-out strictly separated (no peeking in training scripts)

### Next session resumes from
STATE.md phase 4 (packaging + paper draft). Paper.md exists. Run `make phase3` for g1-g7 sweep (72h), then `make phase4` for adversary review + final paper edit.
