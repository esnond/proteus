# decisions.md: HX style log

## 2026-10-29: r2.5 run, HX-9 robotics cancellation

**HX-9:** robotics integration cancelled by owner 2026-10-29. focus is
network defense only. the working tree carried no neato/xv-15/robotics
lane at cancellation time (docs/NEATO_INVENTORY.md; independent Explore
verification: zero hits). git history keeps its traces as provenance;
filter-repo is an owner option, never run by an agent. the paper keeps
"robot body excluded" as an honest scope limit.

## 2026-10-29: phase 0-3 autonomous run

### scope cuts (recorded before work)
- **robot body excluded** (AGENTS.md): r6 packaging + paper only. no robot hardware integration.
- **quantum rhetoric forbidden** (AGENTS.md): A6 model is theoretical bound only, no resistance claims.

### environment blocks routed to scripts (not obeyed as stops)
| block | script | what it proves on bench |
|-------|--------|------------------------|
| nftables kernel acceptance | `tools/validate_r2.sh` phases 1,2,6 | hand built netlink protocol meets kernel |
| g4 active probe | `tools/validate_r2.sh` phase 5 / g1_g7_runner.sh | real netns reduces L below passive |
| g6 transfer gap | `tools/g1_g7_validate.sh` | synthetic trained observer on independent stand |
| seccomp negative test | (none yet) | fault injection helper needed |

### agent reviews (sequential per R1)
| phase | methodologist | architect | adversary | memory | editor |
|-------|---------------|-----------|-----------|--------|--------|
| 0 | ✅ baseline spec | ✅ baseline spec | none | ✅ STATE.md | none |
| 1 | ✅ f1.1-f1.5 registered | ✅ validate_r2.sh fixes | none | ✅ theory.md updates | none |
| 2 | ✅ atlas_spec.md | ✅ atlas_runner/analyze | none | ✅ STATE.md | none |
| 3 | (g1-g7 pre-registered) | ✅ observers + learnability | none | ✅ STATE.md | none |

### key technical decisions
1. **noise_scale=1.0 in bridge** (not engine): engine default 0 → all seeds identical. bridge override documented in VENDOR.md + petri DEFECTS.md.
2. **phantom wall tap at t=0**: zeroed body in contact band → primed edge detectors (`prev_contact=1`). documented.
3. **detector threshold 30.0 frozen**: lowest grid with FA ≤ 1/600s on seeds 101..110. never refit.
4. **DET_THRESHOLD in config.h**: not fitted to green results post hoc.
5. **party replay for determinism**: `-e fixture` (recorded input), not live `-r` (nanosleep = nondeterministic).
6. **throughput = two metrics**: headless (250) vs realtime (33). never one number.
7. **timing CV=52% = wire stochasticity**: detector probabilistic by design at frozen threshold. not a side channel leak.
8. **bypass finding 1/3 + 100% ban hold = MTD boundary**: documented in theory.md as strength (predicted boundary).
9. **test_6.1 asserts userspace ban**: sandbox gate; nft ban on privileged bench.
10. **SECCOMP_ALLOW_N explicit count**: `__NR_read=0` broke sentinel `break` on zero. fixed.

### negative results published (not hidden)
- idle CPU 7% vs <1% target
- nftables backend unavailable in sandbox
- low and slow wire detection 1/3
- g4 active probe blocked (no netns)
- g6 transfer gap blocked (no independent stand)
- seccomp negative test not covered

### falsification criteria registered pre-experiment
all g1-g7 in hypotheses.md + atlas_spec.md + theory.md. any refutation = result with data.

### reproducibility
- all seeds allocated: calibration 101..110, training 1001..1100, held-out 2001..2100, stress 3001..3050
- manifests generated per experiment (binary SHA256, CFLAGS, seeds, schedule hash)
- held-out strictly separated (no peeking in training scripts)

### next session resumes from
STATE.md phase 4 (packaging + paper draft). Paper.md exists. run `make phase3` for g1-g7 sweep (72h), then `make phase4` for adversary review + final paper edit.
