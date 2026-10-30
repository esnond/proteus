# schedule learnability attack: journals_v12

data: /home/esnond/proteus/results/journals_v12/{bio,cycle,random}/seed_{1001..1010,1091..1100}.csv
sample 20/100 journals per family; 60000 rows @ 10 ms = 600 s window; port space 40000..40063 (n_ports=64).
n_transitions = port column changes: bio 87.2, cycle 99.4, random 98.9 (mean, n=20 journals). all CI = bootstrap 95%, 4000 resamples, n=20 unless stated.

## 1. passive frequency
| family | moves mean [CI] | distinct ports | H bits [CI] | uniform-null H | H deficit [CI] | top-1 share [CI] | chi2 vs uniform p |
|---|---|---|---|---|---|---|---|
| bio | 87.2 [77.9, 97.4] | 21.7 [19.8, 23.5] | 4.014 [3.885, 4.136] | 5.350 | -1.339 [-1.442, -1.241] | 0.139 [0.124, 0.156] | 0.000 |
| cycle | 99.4 [89.5, 110.2] | 64 | 5.941 [5.931, 5.952] | 5.461 | +0.526 [0.469, 0.586] | 0.0223 [0.0207, 0.0239] | 1.000 |
| random | 98.9 [89.2, 109.6] | 50.7 [48.4, 53.1] | 5.468 [5.402, 5.539] | 5.446 | +0.029 [-0.019, 0.071] | 0.0496 [0.0460, 0.0533] | 0.585 [0.454, 0.711] |
uniform-null H = mean of 1000 uniform-64 draws at each journal's n (finite-sample reference, not a CI). bio pooled top ports: 40031 12.10% [10.65, 13.71], 40030 11.30% [9.89, 12.87], top-2 23.39% [21.47, 25.44], n=1744 moves.

## 2. periodicity / autocorrelation
| family | port ACF(1) [CI] | max abs ACF lag1-16 | share of +1 steps [CI] | interval mean s [CI] | interval CV [CI] | interval ACF(1) [CI] | Ljung-Box p(16) | port spectral peak/median |
|---|---|---|---|---|---|---|---|---|
| bio | 0.354 [0.286, 0.413] | 0.390 [0.350, 0.428] | 0.099 [0.084, 0.115] | 7.28 [6.55, 8.04] | 1.302 [1.246, 1.360] | -0.002 [-0.042, 0.038] | 0.264 [0.141, 0.396] | 3.35 [2.98, 3.74] |
| cycle | 0.914 [0.910, 0.918] | 0.914 [0.910, 0.918] | 1.000 [1.000, 1.000] | 6.32 [5.71, 6.93] | 1.389 [1.320, 1.466] | -0.011 [-0.043, 0.023] | 0.418 [0.280, 0.559] | 18.10 [16.91, 19.39] |
| random | -0.044 [-0.089, 0.001] | 0.212 [0.184, 0.244] | 0.017 [0.012, 0.022] | 6.36 [5.75, 6.95] | 1.389 [1.320, 1.466] | -0.010 [-0.043, 0.024] | 0.404 [0.259, 0.544] | 2.62 [2.49, 2.75] |
inter-move intervals are memoryless in all families (ljung-box p>0.26, lag-1 ACF 0); CV~1.3 means heavy-tailed, not periodic. cycle port power is a monotone ramp trend (period 77 moves = window), not recurrence. port autocorrelation in bio is favorite port recurrence, not a period.

## 3. held-out markov prediction (train first 60%, test last 40%, n=20)
| family | order-1 acc [CI] | order-2 acc [CI] | uniform 1/64 | ratio order-1 | order-2 minus order-1 [CI] | obs to 0.90 (Markov) | obs to 0.90 (delta rule) |
|---|---|---|---|---|---|---|---|
| bio | 0.0712 [0.0540, 0.0901] | 0.0768 [0.0603, 0.0948] | 0.0156 | 4.6x | +0.0056 [0.0000, 0.0150] | >87 (0/20) | >87 (0/20) |
| cycle | 0.7188 [0.5891, 0.8368] | 0.7188 [0.5891, 0.8368] | 0.0156 | 46.0x | 0.0000 | 62 moves (14/20), >99 (6/20) | 2 moves (20/20) |
| random | 0.0100 [0.0047, 0.0158] | 0.0089 [0.0037, 0.0148] | 0.0156 | 0.64x | -0.0011 [-0.0034, 0.0000] | >99 (0/20) | >99 (0/20) |
obs-to-0.90 = first prefix length whose model predicts the next 20 moves with >=0.90 accuracy. bio order-1 is 1.54x the uniform baseline over the observed alphabet (1/21.7), so its edge is mostly alphabet shrinkage. cycle order-1/2 needs 62 moves because symbols do not repeat until the counter wraps; a delta rule attacker (predict prev+1 mod 64) reaches 0.90 after 2 moves, 20/20.

## 4. generator reconstruction
| family | test | result (n=20 journals) |
|---|---|---|
| cycle | port_k = 40000 + ((k+1) mod 64) | exact for 20/20; LCG (a=1,c=1) perfect 20/20; first move 40001 in 20/20 |
| random | affine LCG mod 64 | perfect 0/20; best (a,c) explains 0.080 [0.074, 0.085], chance level |
| random | GF(2) affine 6-bit map | consistent 0/20 |
| random | xorshift64* state recovery from low-6 output | inconsistent 0/20 (rank-64 GF(2) system, strides 1-16, offsets 0-3, state and low6(product) extraction); synthetic xorshift64* positive control 20/20 |
| bio | affine LCG mod 64 | perfect 0/20; best explains 0.143 [0.128, 0.160] (favorite-port pairing, not a generator) |
random is therefore consistent with an i.i.d. uniform draw over 64 ports at every observable level (H, chi2, ACF, LCG/Galois fits). the 6-bit port projection does not expose the 64-bit xorshift state under tested conventions.

## 5. leak / learnability verdict
| family | property that leaked | minimum observations | observations_to_90pct | vs uniform | verdict |
|---|---|---|---|---|---|
| bio | deterministic first move 40032 (20/20); favorite ports 40031/40030 = 23.4% of moves; H 1.34 bits below uniform | 0 for move 1; 8.3 [7.3, 9.4] active probes to first favorite hit; top-2 hit in 4.3 [3.9, 4.7] probes | >87 moves (>600 s); best 7.7% | 4.6x (order-1) / 1.5x vs seen alphabet | claim_refuted (leaks prefix and favorites, cannot reach 0.90) |
| cycle | constant +1 counter mod 64 from 40000 | 2 moves, 3.52 s [3.11, 4.02] from journal start | 2 moves | 64x (delta), 46x (order-1) | claim_refuted (fully predictable) |
| random | none found | >99 moves within window; ~303 moves estimated to enumerate all 64 (coupon collector, estimate beyond window; 41.4 [40.3, 42.5] distinct seen by move 64) | >99 moves | 0.64x | claim_supported |

notes: for a fixed seed the move-instant sets are 97-100% shared across families (bio vs cycle/random), so timing is scenario-driven and family-independent; the port draw itself carries the family difference. all figures are within the 600 s window; the random enumeration figure is the only extrapolation and is labelled an estimate. no repo files other than this report were written; computations are in /tmp (main_attack.py, attack2.py, attack3.py, attack4.py, final_summary.py).
