# atlas of parties: phase 2 (r3) specification

**status**: specification (not code). implementation follows architect gate.

---

## 1. what "party" means in proteus context

A **party** is one deterministic, headless simulation run of the C. elegans connectome engine (petri, vendored) driving the TCP port move schedule.

- one party = one seed + one event schedule (v2) + one binary build.
- duration: 600 s simulated time (60,000 ticks at 10 ms/tick).
- journal: `party journal v2` (schemas.md), 14 columns, one row per tick.
- reproducibility: **party replay**: same seed + same event schedule + same binary = bitwise identical journal (validated by `cmp -s`).
- no wall-clock, no live network, no `-r` flag in science path. `xorshift64*` only. `-ffp-contract=off`.

---

## 2. causal attribution: candidate driver cells and states per port move

Every port move (journal `port` column change) must be attributed to connectome dynamics. The **causal atlas** enumerates which neurons / modulators / internal states are candidates for driving each move.

### 2.1 driver taxonomy (frozen from connectome)

| driver class | candidate cells / variables | mechanism (emergent behavior compliant) |
|---|---|---|
| **reversal command** | `AVA`, `AVB`, `AVD`, `AVG` | transmitter release (`act_ava`, `act_avb`) + coupling to body bend |
| **nociception trigger** | `ASH`, `ADL`, `FLP` | `ash_trace` envelope → AVA/AVB via synaptic weights |
| **modulatory gain** | `SER` (serotonin), `OCT` (octopamine), `DOP` (dopamine) | continuous gain/tau modulation of command interneurons |
| **detector coupling** | `det_score` (matched filter on AVA deviation) | endogenous: detector fires when AVA deviation exceeds adapted baseline |
| **endogenous reversal** | spontaneous AVA/AVB phase flip | intrinsic dynamics + noise, no external event |

### 2.2 attribution vector per move

For each port move at tick `t`, compute a **14-element attribution vector** (frozen length, one per driver candidate):

```
[ava_amp, avb_amp, avd_amp, avg_amp, ash_trace, ser, oct, dop,
 det_score_delta, det_state, phase_ava, phase_avb, coupling_ava_avb, noise_floor]
```

- `ava_amp` = `act_ava` at move tick (transmitter release)
- `det_score_delta` = `det_score[t] - det_score[t-1]`
- `det_state` = 0 armed, 1 fired, 2 cool (from detector contract)
- `phase_ava` = instantaneous phase of AVA oscillator (Hilbert or zero crossing)
- `coupling_ava_avb` = signed synaptic weight AVA→AVB × AVB activity
- `noise_floor` = residual after subtracting deterministic drivers (quantifies stochasticity)

**rule**: no `if (detector) port_move` in science path. port move emerges from AVA/AVB dynamics; detector is a *readout* of AVA deviation, not a causal switch.

---

## 3. divergence flags: mirror causal atlas method

Two halves of the simulation = **first half** (ticks 0..29,999, 0–300 s) and **second half** (ticks 30,000..59,999, 300–600 s). The **causal atlas** is built independently on each half.

### 3.1 per-half causal model

For each half `h ∈ {1,2}`:
- fit a **sparse logistic regression** (or mutual information ranking) predicting `port_move ∈ {0,1}` from the 14-driver attribution vector.
- regularization: L1 (lasso) with `lambda` chosen by 5-fold CV *within the half*.
- output: coefficient vector `β_h` (14 elements) + selected driver set `S_h = {i : β_h[i] ≠ 0}`.

### 3.2 divergence metrics (computed per party)

| flag | formula | interpretation |
|---|---|---|
| **jaccard divergence** | `J = 1 - |S_1 ∩ S_2| / |S_1 ∪ S_2|` | driver set instability |
| **coefficient cosine distance** | `D_cos = 1 - (β_1·β_2) / (||β_1|| ||β_2||)` | directional shift in driver weights |
| **top-3 rank correlation** | `ρ = spearman(rank(β_1[top3]), rank(β_2[top3]))` | stability of dominant drivers |
| **attribution mass shift** | `M = Σ_i |β_1[i] - β_2[i]| / (Σ_i |β_1[i]| + |β_2[i]|)` | total redistribution |

### 3.3 divergence flag thresholds (registered pre-experiment)

| flag | threshold (α=0.05, bonferroni for 4 flags) | action |
|---|---|---|
| `J > 0.5` | significant driver set change | report "DRIVER_SET_SHIFT" |
| `D_cos > 0.3` | significant weight redistribution | report "WEIGHT_SHIFT" |
| `ρ < 0.5` | top drivers reorder | report "RANK_SHIFT" |
| `M > 0.4` | mass moved between halves | report "MASS_SHIFT" |

**multiple comparison correction**: bonferroni (4 flags) → per-flag α = 0.0125.

---

## 4. success criterion (quantitative)

### 4.1 primary criterion (attribution coverage)

> **≥ 80% of port moves** in a party are attributable to identified drivers with **p < 0.05** (permutation test, 1000 permutations per move), after bonferroni correction for number of moves in the party.

- attribution test: shuffle driver labels 1000×, recompute attribution vector correlation with actual move; p = fraction of shuffles ≥ observed.
- report: mean attribution coverage across parties ± 95% CI, n = number of parties.

### 4.2 secondary criterion (divergence quantification)

> **reconstruction sensitive set** = parties where ≥1 divergence flag triggers.
> report: fraction of parties with divergence flags ± 95% wilson CI, n = total parties.
> **power caveat**: if power < 0.8 to detect J ≥ 0.3 at α=0.0125, label "underpowered" in results.

### 4.3 tertiary criterion (learnability measure L)

> compute **L** (theory.md г5) on each half: minimum observations for observer model to predict next port move with P ≥ 0.9.
> report: `L_1`, `L_2`, gap `ΔL = |L_1 - L_2|` ± 95% CI across parties.

---

## 5. falsification experiment

**registered prediction**: "bio-stochastic schedule (living connectome) yields lower learnability L than seeded schedule, and divergence flags are rare (J < 0.5 in ≥ 90% of parties)."

**falsification experiment** (runs headless, bitwise reproducible):

1. **condition A (bio)**: 100 parties, seeds 1001..1100, event schedule = `tests/fixtures/burst.csv` (r1 fixture).
2. **condition B (seeded)**: 100 parties, seeds 1001..1100, event schedule = `tests/fixtures/seeded_schedule.csv` (deterministic port moves every 5 s, no connectome).
3. **condition C (random)**: 100 parties, seeds 1001..1100, event schedule = `tests/fixtures/random_schedule.csv` (uniform random port moves, matched rate to condition A).

**falsification criteria** (any one refutes the prediction):
- median `L_bio` ≥ median `L_seeded` (bio not harder to learn than seeded).
- median `L_bio` ≥ median `L_random` (bio not harder to learn than random).
- fraction of bio parties with `J ≥ 0.5` > 10% (divergence common).
- attribution coverage < 80% in > 20% of bio parties.

**statistical test**: wilcoxon rank sum (L), fisher's exact (flag fractions), all bonferroni corrected for 3 comparisons (α = 0.0167). effect sizes: cohen's d for L, odds ratio for fractions.

---

## 6. held-out seed plan (biological validation)

### 6.1 seed allocation

| set | seeds | purpose | size |
|---|---|---|---|
| **calibration** | 101..110 | detector threshold (already frozen, r1) | 10 |
| **training** | 1001..1100 | atlas construction, divergence thresholds | 100 |
| **held-out validation** | 2001..2100 | final validation, no peeking | 100 |
| **stress** | 3001..3050 | adversary: edge cases, long runs | 50 |

### 6.2 held-out protocol

- **no analysis code touches held-out seeds until final validation.**
- validation script `tools/atlas_validate.sh` takes seed list as argument; training script `tools/atlas_train.sh` forbidden from reading held-out journals.
- report: all metrics on held-out set with same CI and effect size standards.
- if held-out performance drops > 15% relative to training (attribution coverage, L gap), report as **negative result** with power analysis.

---

## 7. CSV contract (frozen columns)

### 7.1 `results/atlas_parties.csv`, one row per party

| column | type | description |
|---|---|---|
| `party_id` | int | sequential 1..N |
| `seed` | uint64 | xorshift64* seed |
| `schedule_hash` | string | SHA256 of event schedule file |
| `binary_hash` | string | SHA256 of proteus binary |
| `n_moves` | int | total port moves in 600 s |
| `attribution_coverage` | float | fraction of moves with p<0.05 attribution |
| `jaccard_div` | float | jaccard divergence J (0..1) |
| `cosine_div` | float | cosine distance D_cos (0..1) |
| `rank_corr` | float | spearman ρ on top-3 drivers (-1..1) |
| `mass_shift` | float | attribution mass shift M (0..1) |
| `flags_triggered` | string | comma separated: DRIVER_SET_SHIFT,WEIGHT_SHIFT,... |
| `L_half1` | float | learnability L first half (observations) |
| `L_half2` | float | learnability L second half |
| `delta_L` | float | \|L_1 - L_2\| |
| `duration_s` | float | wall clock seconds (for budget) |

### 7.2 `results/atlas_moves.csv`, one row per port move

| column | type | description |
|---|---|---|
| `party_id` | int | foreign key to atlas_parties |
| `tick` | int | journal tick (0..59999) |
| `t_ms` | int | simulated time ms |
| `port_from` | int | previous port |
| `port_to` | int | new port |
| `cause_code` | int | journal cause (0..3) |
| `attribution_p` | float | permutation p-value for this move |
| `top_driver` | string | name of driver with max \|β\| |
| `top_driver_weight` | float | β value for top driver |
| `half` | int | 1 or 2 |

### 7.3 `results/atlas_drivers.csv`, per-half driver coefficients

| column | type | description |
|---|---|---|
| `party_id` | int | foreign key |
| `half` | int | 1 or 2 |
| `driver` | string | one of 14 frozen driver names |
| `coefficient` | float | β from lasso (0 if not selected) |
| `selected` | int | 1 if in S_h, 0 otherwise |
| `mi_score` | float | mutual information with port_move (bits) |

---

## 8. reproducibility manifest requirements

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

## 9. limitations (grows only)

- attribution is correlational (sparse regression on observational simulation data); causal claims require ablation (r4+).
- 14-driver vector assumes petri connectome topology is fixed; upstream petri changes invalidate atlas.
- held-out seeds 2001..2100 untested at specification time; power analysis assumes effect size d≥0.5.
- divergence flags assume stationarity within half; non-stationarity within half not modeled.
- learnability L measured against synthetic observer classes; real attacker capability unknown.

---

## 10. implementation gates (for architect)

1. **determinism-gate**: `tools/atlas_runner.sh` runs headless, `xorshift64*`, `-ffp-contract=off`, no wall-clock.
2. **emergent-behavior**: attribution computation reads journal only; no `if (detector) move` in analysis.
3. **scientific-methodology**: all thresholds registered above; no post-hoc threshold tuning.
4. **statistical-rigor**: every reported number → mean, 95% CI, n, effect size. bonferroni for 4 divergence flags + 3 falsification comparisons.
5. **reproducibility-manifest**: manifest generated per seed batch.
6. **biological-validation**: held-out seeds 2001..2100 strictly separated.

---

**end of specification.** implementation proceeds only after architect review and adversary stress confirm.
