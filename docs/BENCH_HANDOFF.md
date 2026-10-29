# BENCH_HANDOFF.md — sudo checklist for owner

## What this unlocks
Running these commands on a privileged bench (real hardware, CAP_NET_ADMIN, netns) upgrades the empirical story:
- **r2 validation**: from "sandbox-passed with env blocks" → "bench-validated"
- **g4 active probe**: from "blocked" → measured L_active vs L_passive on real netns
- **g6 transfer gap**: from "blocked" → synthetic-trained observer on independent stand
- **Paper**: from "preprint with caveats" → "bench-complete, main-track eligible if g7 holds"

---

## 1. r2 Full Validation (30-45 min)

```bash
cd /home/esnond/proteus
git pull
sudo tools/validate_r2.sh
```

**What it proves:**
- Phase 1: nftables backend AVAILABLE (kernel accepts hand-built netlink), bans persist across daemon restart
- Phase 2: nft set persistence + rule correctness (kernel ruleset matches daemon intent)
- Phase 6: Full scenario A legit → B scan ban (nft) → C legit survives → D brute ban

**Exit criteria:** `report.md` shows `VERDICT: r2 validated, proceed to r3` (all CRITICAL green, IMPORTANT > 80%)

---

## 2. g4 Active Probe on Real Netns (15 min)

```bash
cd /home/esnond/proteus
sudo ip netns add proteus_bench
sudo sh tools/netns_setup.sh proteus_bench
# Inside netns: run proteusd with --nft --addr 10.0.0.2
# From host: run active observer probes against 10.0.0.2
# Measure L_active vs L_passive
sudo sh tools/netns_teardown.sh proteus_bench
```

**What it proves:** Whether probing reduces learnability below passive bound (g4 thesis) on real kernel path.

---

## 3. g6 Transfer Gap on Independent Stand (1 h)

```bash
# Requires: second machine / VM with different kernel / hardware
# Train observer on proteus journals (this repo)
# Test on independent C. elegans sim stand (different params)
# Measure transfer gap = L_stand - L_proteus
```

**What it proves:** Model-collapse caveat (g6) — synthetic-trained observer generalization.

---

## 4. Full Phase 3 Sweep (72 h, can run overnight)

```bash
cd /home/esnond/proteus
git pull
make phase3
```

**What it produces:**
- `results/hypotheses/g1_g7_results.csv` (training 1001..1100)
- `results/hypotheses_validate/g1_g7_results.csv` (held-out 2001..2100)
- Manifests with SHA256

**If g7 holds** (L_bio ≥ 10×L_seeded, legit disruption < 1%): paper is main-track eligible.

---

## 5. Push Results

```bash
cd /home/esnond/proteus
git add results/r2_validation/ results/hypotheses/ results/hypotheses_validate/
git commit -m "bench: r2 validated + g1-g7 results"
git tag v1.2
git push --force-with-lease origin v1.2
git push --force-with-lease origin v1.2-autonomous
```

---

## What each result upgrades

| Bench result | Paper claim upgraded |
|--------------|---------------------|
| r2 VERDICT=validated | "nftables enforcement works" (not just userspace) |
| g4 L_active < L_passive | "active probing reduces learnability" (not just synthetic) |
| g6 transfer gap > 0 | "model-collapse caveat quantified" (not just assumed) |
| g7 L_bio ≥ 10×L_seeded + disruption < 1% | Central thesis confirmed → main-track submission |

---

## If bench unavailable

The paper.md, atlas, g1-g7 infrastructure, and sandbox-validated r2 are **complete and honest**. The limitations section explicitly states what's unvalidated. This is a valid workshop/arXiv contribution as-is.

**No silent "needs owner" items** — every blocker has a ready script above. Run what you can, publish what you get.

---

## r2.5 additions (2026-10-29)

four bench-only items, each a ready command. no item is "needs owner".

| item | command | what upgrades |
|---|---|---|
| gcore gap (t5.2) | `sudo tools/bench_gcore_test.sh` | installs gdb, dumps a live daemon, audits markers |
| foreign uid (t4) | `sudo -u nobody ./proteusctl register --socket /tmp/rt.sock` | expect `denied`; proves registration is owner/root only |
| token timing (t6) | `tools/roc_sweep.sh` style: valid vs invalid token, KS p>=0.01, \|d\|<0.2 | no timing side channel on the check |
| FP budget (f1.4) | `LEGIT_SECS=60 tools/roc_sweep.sh 10` | ~12000 legit sessions, 95 percent upper bound <=5 per 10k |
| idle target (f1.3) | `tools/validate_r2.sh` phase 4.1 | idle clock `--idle-divisor 20`: cpu <1 percent, wake p99 <=5 ms |
| full suite | `sudo tools/validate_r2.sh` | r2.5 report with the corrected tests |

expected sandbox numbers to diff against: 18 pass / 1 fail (brute force,
known gap) / 1 skip; idle clock 0.79 percent at divisor 20; secret
dependence p=0.66 d=0.057; register -> trust=1 while the pid lives,
trust=0 within 800 ms after it exits.
