# ATLAS OF PARTIES — Phase 2 (r3) Specification

**Status**: SPECIFICATION (not code). Implementation follows architect gate.

---

## 1. What "party" means in proteus context

A **party** is one deterministic, headless simulation run of the C. elegans connectome engine (petri, vendored) driving the TCP port move schedule.

- One party = one seed + one event schedule (v2) + one binary build.
- Duration: 600 s simulated time (60,000 ticks at 10 ms/tick).
- Journal: `party journal v2` (schemas.md) — 14 columns, one row per tick.
- Reproducibility: **PARTY REPLAY** — same seed + same event schedule + same binary = bitwise identical journal (validated by `cmp -s`).
- No wall-clock, no live network, no `-r` flag in science path. `xorshift64*` only. `-ffp-contract=off`.

---

## 2. Causal attribution: candidate driver cells/states per port move

Every port move (journal `port` column change) must be attributed to connectome dynamics. The **causal atlas** enumerates which neurons / modulators / internal states are candidates for driving each move.

### 2.1 Driver taxonomy (frozen from connectome)

| Driver class | Candidate cells / variables | Mechanism (emergent-behavior compliant) |
|---|---|---|
| **Reversal command** | `AVA`, `AVB`, `AVD`, `AVG` | Transmitter release (`act_ava`, `act_avb`) + coupling to body bend |
| **Nociception trigger** | `ASH`, `ADL`, `FLP` | `ash_trace` envelope → AVA/AVB via synaptic weights |
| **Modulatory gain** | `SER` (serotonin), `OCT` (octopamine), `DOP` (dopamine) | Continuous gain/tau modulation of command interneurons |
| **Detector coupling** | `det_score` (matched filter on AVA deviation) | Endogenous: detector fires when AVA deviation exceeds adapted baseline |
| **Endogenous reversal** | Spontaneous AVA/AVB phase flip | Intrinsic dynamics + noise, no external event |

### 2.2 Attribution vector per move

For each port move at tick `t`, compute a **14-element attribution vector** (frozen length, one per driver candidate):

```
[ava_amp, avb_amp, avd_amp, avg_amp, ash_trace, ser, oct, dop,
 det_score_delta, det_state, phase_ava, phase_avb, coupling_ava_avb, noise_floor]
```

- `ava_amp` = `act_ava` at move tick (transmitter release)
- `det_score_delta` = `det_score[t] - det_score[t-1]`
- `det_state` = 0 armed, 1 fired, 2 cool (from detector contract)
- `phase_ava` = instantaneous phase of AVA oscillator (Hilbert or zero-crossing)
- `coupling_ava_avb` = signed synaptic weight AVA→AVB × AVB activity
- `noise_floor` = residual after subtracting deterministic drivers (quantifies stochasticity)

**Rule**: No `if (detector) port_move` in science path. Port move emerges from AVA/AVB dynamics; detector is a *readout* of AVA deviation, not a causal switch.

---

## 3. Divergence flags: mirror causal-atlas method

Two halves of the simulation = **first half** (ticks 0..29,999, 0–300 s) and **second half** (ticks 30,000..59,999, 300–600 s). The **causal atlas** is built independently on each half.

### 3.1 Per-half causal model

For each half `h ∈ {1,2}`:
- Fit a **sparse logistic regression** (or mutual information ranking) predicting `port_move ∈ {0,1}` from the 14-driver attribution vector.
- Regularization: L1 (lasso) with `lambda` chosen by 5-fold CV *within the half*.
- Output: coefficient vector `β_h` (14 elements) + selected driver set `S_h = {i : β_h[i] ≠ 0}`.

### 3.2 Divergence metrics (computed per party)

| Flag | Formula | Interpretation |
|---|---|---|
| **Jaccard divergence** | `J = 1 - |S_1 ∩ S_2| / |S_1 ∪ S_2|` | Driver set instability |
| **Coefficient cosine distance** | `D_cos = 1 - (β_1·β_2) / (||β_1|| ||β_2||)` | Directional shift in driver weights |
| **Top-3 rank correlation** | `ρ = spearman(rank(β_1[top3]), rank(β_2[top3]))` | Stability of dominant drivers |
| **Attribution mass shift** | `M = Σ_i |β_1[i] - β_2[i]| / (Σ_i |β_1[i]| + |β_2[i]|)` | Total redistribution |

### 3.3 Divergence flag thresholds (registered pre-experiment)

| Flag | Threshold (α=0.05, Bonferroni for 4 flags) | Action |
|---|---|---|
| `J > 0.5` | Significant driver set change | Report "DRIVER_SET_SHIFT" |
| `D_cos > 0.3` | Significant weight redistribution | Report "WEIGHT_SHIFT" |
| `ρ < 0.5` | Top drivers reorder | Report "RANK_SHIFT" |
| `M > 0.4` | Mass moved between halves | Report "MASS_SHIFT" |

**Multiple comparison correction**: Bonferroni (4 flags) → per-flag α = 0.0125.

---

## 4. Success criterion (quantitative)

### 4.1 Primary criterion (attribution coverage)

> **≥ 80% of port moves** in a party are attributable to identified drivers with **p < 0.05** (permutation test, 1000 permutations per move), after Bonferroni correction for number of moves in the party.

- Attribution test: shuffle driver labels 1000×, recompute attribution vector correlation with actual move; p = fraction of shuffles ≥ observed.
- Report: mean attribution coverage across parties ± 95% CI, n = number of parties.

### 4.2 Secondary criterion (divergence quantification)

> **Reconstruction-sensitive set** = parties where ≥1 divergence flag triggers.
> Report: fraction of parties with divergence flags ± 95% Wilson CI, n = total parties.
> **Power caveat**: If power < 0.8 to detect J ≥ 0.3 at α=0.0125, label "underpowered" in results.

### 4.3 Tertiary criterion (learnability measure L)

> Compute **L** (theory.md г5) on each half: minimum observations for observer model to predict next port move with P ≥ 0.9.
> Report: `L_1`, `L_2`, gap `ΔL = |L_1 - L_2|` ± 95% CI across parties.

---

## 5. Falsification experiment

**Registered prediction**: "Bio-stochastic schedule (living connectome) yields lower learnability L than seeded schedule, and divergence flags are rare (J < 0.5 in ≥ 90% of parties)."

**Falsification experiment** (runs headless, bitwise-reproducible):

1. **Condition A (bio)**: 100 parties, seeds 1001..1100, event schedule = `tests/fixtures/burst.csv` (r1 fixture).
2. **Condition B (seeded)**: 100 parties, seeds 1001..1100, event schedule = `tests/fixtures/seeded_schedule.csv` (deterministic port moves every 5 s, no connectome).
3. **Condition C (random)**: 100 parties, seeds 1001..1100, event schedule = `tests/fixtures/random_schedule.csv` (uniform random port moves, matched rate to Condition A).

**Falsification criteria** (any ONE refutes the prediction):
- Median `L_bio` ≥ median `L_seeded` (bio not harder to learn than seeded).
- Median `L_bio` ≥ median `L_random` (bio not harder to learn than random).
- Fraction of bio parties with `J ≥ 0.5` > 10% (divergence common).
- Attribution coverage < 80% in > 20% of bio parties.

**Statistical test**: Wilcoxon rank-sum (L), Fisher's exact (flag fractions), all Bonferroni-corrected for 3 comparisons (α = 0.0167). Effect sizes: Cohen's d for L, odds ratio for fractions.

---

## 6. Held-out seed plan (biological-validation)

### 6.1 Seed allocation

| Set | Seeds | Purpose | Size |
|---|---|---|---|
| **Calibration** | 101..110 | Detector threshold (already frozen, r1) | 10 |
| **Training** | 1001..1100 | Atlas construction, divergence thresholds | 100 |
| **Held-out validation** | 2001..2100 | Final validation, no peeking | 100 |
| **Stress** | 3001..3050 | Adversary: edge cases, long runs | 50 |

### 6.2 Held-out protocol

- **No analysis code touches held-out seeds until final validation.**
- Validation script `tools/atlas_validate.sh` takes seed list as argument; training script `tools/atlas_train.sh` forbidden from reading held-out journals.
- Report: all metrics on held-out set with same CI/effect-size standards.
- If held-out performance drops > 15% relative to training (attribution coverage, L gap), report as **negative result** with power analysis.

---

## 7. CSV contract (frozen columns)

### 7.1 `results/atlas_parties.csv` — one row per party

| Column | Type | Description |
|---|---|---|
| `party_id` | int | Sequential 1..N |
| `seed` | uint64 | xorshift64* seed |
| `schedule_hash` | string | SHA256 of event schedule file |
| `binary_hash` | string | SHA256 of proteus binary |
| `n_moves` | int | Total port moves in 600 s |
| `attribution_coverage` | float | Fraction of moves with p<0.05 attribution |
| `jaccard_div` | float | Jaccard divergence J (0..1) |
| `cosine_div` | float | Cosine distance D_cos (0..1) |
| `rank_corr` | float | Spearman ρ on top-3 drivers (-1..1) |
| `mass_shift` | float | Attribution mass shift M (0..1) |
| `flags_triggered` | string | Comma-separated: DRIVER_SET_SHIFT,WEIGHT_SHIFT,... |
| `L_half1` | float | Learnability L first half (observations) |
| `L_half2` | float | Learnability L second half |
| `delta_L` | float | \|L_1 - L_2\| |
| `duration_s` | float | Wall-clock seconds (for budget) |

### 7.2 `results/atlas_moves.csv` — one row per port move

| Column | Type | Description |
|---|---|---|
| `party_id` | int | Foreign key to atlas_parties |
| `tick` | int | Journal tick (0..59999) |
| `t_ms` | int | Simulated time ms |
| `port_from` | int | Previous port |
| `port_to` | int | New port |
| `cause_code` | int | Journal cause (0..3) |
| `attribution_p` | float | Permutation p-value for this move |
| `top_driver` | string | Name of driver with max \|β\| |
| `top_driver_weight` | float | β value for top driver |
| `half` | int | 1 or 2 |

### 7.3 `results/atlas_drivers.csv` — per-half driver coefficients

| Column | Type | Description |
|---|---|---|
| `party_id` | int | Foreign key |
| `half` | int | 1 or 2 |
| `driver` | string | One of 14 frozen driver names |
| `coefficient` | float | β from lasso (0 if not selected) |
| `selected` | int | 1 if in S_h, 0 otherwise |
| `mi_score` | float | Mutual information with port_move (bits) |

---

## 8. Reproducibility manifest requirements

Every `results/atlas_*/` directory **must** contain `manifest.txt` generated by `tools/generate_manifest.sh` with:

```
# Generated: [UTC timestamp]
# Binary: proteus [version]
# Binary SHA256: [hash]
# Compiler: [cc --version]
# CFLAGS: [exact flags from Makefile]
# Seed range: [min]..[max]
# Duration: 600
# Schedule: [schedule_file] [SHA256]

results/atlas_parties.csv: [SHA256]
results/atlas_moves.csv: [SHA256]
results/atlas_drivers.csv: [SHA256]
```

Verification: `./proteus --seed N --events schedule.csv --duration 600 --headless > journal.csv` → `tools/atlas_analyze.py journal.csv` → compare CSV hashes.

---

## 9. Limitations (grows only)

- Attribution is correlational (sparse regression on observational simulation data); causal claims require ablation (r4+).
- 14-driver vector assumes petri connectome topology is fixed; upstream petri changes invalidate atlas.
- Held-out seeds 2001..2100 untested at specification time; power analysis assumes effect size d≥0.5.
- Divergence flags assume stationarity within half; non-stationarity within half not modeled.
- Learnability L measured against synthetic observer classes; real attacker capability unknown.

---

## 10. Implementation gates (for architect)

1. **determinism-gate**: `tools/atlas_runner.sh` runs headless, `xorshift64*`, `-ffp-contract=off`, no wall-clock.
2. **emergent-behavior**: Attribution computation reads journal only; no `if (detector) move` in analysis.
3. **scientific-methodology**: All thresholds registered above; no post-hoc threshold tuning.
4. **statistical-rigor**: Every reported number → mean, 95% CI, n, effect size. Bonferroni for 4 divergence flags + 3 falsification comparisons.
5. **reproducibility-manifest**: Manifest generated per seed batch.
6. **biological-validation**: Held-out seeds 2001..2100 strictly separated.

---

**End of specification.** Implementation proceeds only after architect review and adversary stress-confirm.
