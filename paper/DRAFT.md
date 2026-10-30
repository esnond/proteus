# proteus: a closed connectome in a port schedule loop, bitwise replay, one learnability metric L, and published refutations

draft for arXiv. primary audience: computational neuroscience and reproducible computing (connectome and neuromorphic simulators). secondary audience: security and moving target defense workshops.

## abstract

we describe proteus, a deterministic C99 simulator of a *C. elegans* connectome embedded in a network daemon. a scan burst raises the ASH nociceptor, the body slows, AVA/AVB command activity changes, and the listening TCP port moves. there is no hand written policy mapping a scan to a port; the map is the simulated nervous system (README.md:23-26, CLOSE.md:17-28). the engineering result is that the loop closes down to a real socket and that parties replay bitwise: same seed, same event file, same binary produce byte identical journals, checked here on seed 42 over 600 s (the verification block in 5.1 records the comparison).

the scientific result is negative and we report it as such. on a metric L, operationally defined as the first prefix length at which a passive predictor reaches 0.9 accuracy on the next 20 moves (tools/learnability.py:53-108), neither the bio readout nor a xorshift schedule reached 0.9 in 100 parties per mode (0/100 and 0/100), while the legacy counter did in 88/100 at L = 65. that is saturation of the instrument, not equality of schedules: at an alphabet of 64 ports the entropy ceiling is 6 bits per move and a PRNG already sits there (RESEARCH_V2.md:12-15). the single seed reconnaissance that preceded the n = 20 reanalysis found H(port) 5.920 bits for cycle, 3.672 for bio and 5.290 for random, with 16, 45 and 64 distinct ports respectively; those are n = 1 numbers and are labelled as reconnaissance in appendix a. the n = 20 reanalysis in results/schedule_attack/REPORT.md confirms the ordering, with bootstrap 95% intervals reported there and not repeated here. the xorshift schedule is consistent with a uniform draw at every observable level tested and its 64 bit state was not recovered from 6 bit port outputs (0/20, REPORT §4). with knowledge of the generator source the seed is recoverable from 3 observed ports (3177, 52 and 1 candidates after 1, 2 and 3 ports; 0.6 s in the recorded run; full 80 move replay), measured on one seed and therefore reconnaissance (docs/hypotheses_results.md:72-87, tools/seed_recovery_attack.py). detection at a frozen threshold fires on 8 of 10 burst journals under a max score criterion, but only 3 of 10 under an event window criterion; neither criterion was frozen before analysis (results/cal_v12, tools/calibrate_detector.py). we therefore present the contribution as computational neuroscience plus methods, and the security claim as a negative result. coupling (h13) was not measured and is not claimed.

## 1 introduction

### 1.1 the question

a deterministic simulator with a seed is a PRNG in the information theoretic sense: a deterministic function of a key (RESEARCH_V2.md:5-6). asking whether a connectome schedule is "more random" than a seeded generator is therefore unwinnable by construction. at an alphabet of 64 symbols the ceiling is 6 bits per move; only a true one time pad exceeds it (PLAN_V11.md:12-15, citing Shannon 1949). the useful question is not which source is more random but on which measurable property a PRNG structurally cannot win. RESEARCH_V2.md:12-15 and 118-123 list four candidates: cost of generator reconstruction, closed coupling between schedule and attack, nonstationarity, and detection and audit in the same substrate. this paper measures the first two only partially, implements the closed loop, and does not measure coupling.

without that framing the negative results read as failure rather than as a result. the framing is the contribution of the introduction: a schedule generator that is deterministic and reconstructable is a legitimate object of study, and the honest question is how many observations an attacker of a stated class needs, not whether the substrate is "random".

### 1.2 contributions

1. a connectome simulator written from scratch, closed onto a real socket: scan burst to ASH, ASH to body, body to AVA/AVB, command activity to port, with no hand written policy layer (README.md:88-101, CLOSE.md:17-28).
2. bitwise party replay under a recorded binary, commit, compiler, flags and event file hash (results/manifest.txt). one seed is verified here; the 100 party claim in CLOSE.md:21-22 was not rechecked in this session.
3. one learnability definition, L, applied to the worm behaviour surface and to the defense schedule (docs/theory.md:1-10). only the schedule side is measured in this paper.
4. preregistration for h1 to h15 with success and falsification criteria, published failures, and one explicitly post hoc registration (h11), docs/hypotheses.md, docs/hypotheses_results.md.
5. negative results as the main scientific output: no entropy superiority, a broken random arm, a rank one readout, a falsified central premise (h2), and a live/headless divergence.

### 1.3 what is deliberately not in this paper

the h1 to h4 and h6 atlas chain was not measured on the current core and has no verdicts (docs/hypotheses_results.md:64-70). h5 was measured on the old core and is not related to the main thesis (docs/hypotheses_results.md:7-17). h12 to h15 were never run (docs/hypotheses.md:474-524). the privileged bench (kernel nftables enforcement, active probe in a network namespace, gcore, seccomp negative test) was never run (docs/RELEASE_V1.md:6, README.md:136-139). those are marked pending, not claimed. this paper does not inherit the placeholder tables of paper.md; that draft is stale and its atlas chain was abandoned (docs/STATE.md:43).

### 1.4 related work

seeded generators with linear or near linear state are reconstructable: xorshift (Marsaglia 2003), xorshift* and xoshiro (Vigna 2014, 2016), all cited by name in RESEARCH_V2.md:151-152. min entropy estimation of physical noise sources is standardized in NIST SP 800-90B (RESEARCH_V2.md:153); the standard is about estimating a source, not about schedule unpredictability, and we do not use it as evidence for the connectome. for the neuroscience side we rely on the sources named by the engine: Yemini et al. 2013 for speed and curvature, Pierce-Shimomura et al. 1999 for the ASH nociceptor, Kato et al. 2015 for the AVA transient, Tsodyks and Markram 1997 for short term plasticity (README.md:145-147). predictability as a cross domain notion is discussed by Boffetta, Cencini, Falcioni and Vulpiani 2001, and permutation entropy convergence under noise by Amigó, Dale and Tempesta 2020 and Chen and Sato 2024, as recorded in RESEARCH_V2.md:137-145. for moving target defense we did not verify a quantitative source in the repository; RESEARCH_V2.md:147-149 marks reviews of quantitative MTD metrics as UNCERTAIN, so we make no literature claim about MTD metric baselines. the repository itself records that no published learnability metric for port schedules was found during an industry scan (docs/STATE.md:47); that is a repo claim, not a literature review, and we label it as such.

## 2 methods

### 2.1 simulator and closed loop

the engine is a vendored biophysical simulator, petri v1.2, commit 42d5062 per the state log (docs/STATE.md:44). the wiring sheet src/petri/data/varshney_full.csv contains 281 distinct nodes and 6417 edges after excluding its header row (recomputed here). the source comment in src/config.h:36 says 279 somatic neurons after filtering; the loader drops body wall muscles and neuromuscular junctions, but the runtime count of the neuron field is not logged, so the exact number of simulated neurons is unverified and we do not state 279 as a measurement.

the loop has six stages (README.md:88-101):

| stage | implementation | role |
|---|---|---|
| engine | src/petri/ | graded potentials, short term plasticity, serotonin, octopamine, dopamine, body physics |
| bridge | src/bridge.c | engine ownership, membrane noise, modulator injection, state readout |
| detector | src/detector.c | matched filter on AVB deviation, 1 s window, frozen threshold |
| reflex | engine native | reversal episode: speed below -0.02 mm/s sustained 500 ms |
| motor | src/motor.c | port window 40000 to 40063, non blocking listener |
| journal | 11 column CSV | one row per 10 ms of model time |

a scan burst drives ASH; the body slows; AVA/AVB command activity moves; the motor moves the port. the claim that no policy layer sits between burst and port is structural: the motor index comes from a readout of the engine state (src/bridge.c:96-108) or from a counter or generator, and the sim path contains no target vector or goal state. we did not re-audit the whole sim path for goal directed constructs in this session; the repository asserts the property (README.md:103-106).

### 2.2 connectome readout

the bio destination is a rank one functional:

    index = floor(64 * mean(act_ava, act_avb, ash_trace, ser, oct, dop))

implemented at src/bridge.c:101-107. it is a linear projection of six scalars. its entropy is bounded by the entropy of one scalar, independent of the number of neurons. this is a readout defect, not a property of the connectome, and it motivates the observed ceiling of a few tens of ports in the schedule attack (REPORT §1).

### 2.3 detector and reflex

the detector is an exponential moving average baseline (tau 30 s), a dense window of 1 s, and a cooldown of 10 s, applied to the AVB channel; it was re-expressed on AVB after AVA was found constitutively saturated under the varshney wiring (docs/hypotheses.md:395-416). the reflex is the engine native reversal definition, speed below -0.02 mm/s sustained 500 ms, with a 2000 ms cooldown (docs/hypotheses_results.md:50-53). the threshold 50.0 was frozen from a false alarm budget of at most 10 false alarms over 6000 s of model time (10 seeds at 600 s), before the detection side was measured (docs/hypotheses.md:418-432). the detection criterion itself, however, was not frozen; see 2.6 and 3.2.

### schedule modes

three destination generators are available through the destination mode switch (docs/hypotheses.md:365-390):

- cycle: the port is the base plus the bind counter modulo 64, an ascending counter (src/motor.c:133).
- bio: the rank one readout of 2.2, mapped into the 64 port window.
- random: xorshift64* seeded from the command line integer (src/motor.c:18-30, 182).

live mode additionally builds a fixed secret permutation and applies it to the index (src/motor.c:162). headless mode returns before the permutation (src/motor.c:155 calls the headless bind, which ignores the permutation). every journal based measurement in this paper is therefore of the stream before the permutation, and does not describe deployment. this is a published defect, not a hidden one (RESEARCH_V2.md:74-81, docs/RELEASE_V1.md:31-33).

### 2.5 metric L

the operational definition is the one in tools/learnability.py:53-108. for a port sequence s, for each prefix length o in the grid from 10 to the smaller of 500 and the sequence length minus 10, in steps of 5, a predictor is fitted on the prefix using marginal frequencies and first order transitions, and scored on the next 20 moves. L is the first o at which accuracy is at least 0.9. if no o qualifies, the function returns the maximum 500. three consequences follow. L is an accuracy threshold on a 20 move window, not the probability of the next action; it is quantized in steps of 5; and it is bounded above by party length minus 10; in the h9 data every party that reached 65 had at least 76 moves. the prose definition in README.md:26-30 and CLOSE.md:26-28 ("observations until the next action is predicted with probability at least 0.9") does not match the code, and we use the code as the definition. L is therefore not directly comparable to literature learnability values; it is an instrument specific lower bound.

for the reconstruction attack we use a different construct, observations to recovery: the number of observed ports after which the candidate seed set has size one. we do not put this number in an L column (correction recorded in docs/RESEARCH_V2.md:190-203 and in the methodologist pass).

### 2.6 statistical procedures

- schedule attack (n = 20 journals per family, sampled from the 100 shipped journals, seeds 1001 to 1010 and 1091 to 1100): bootstrap 95% intervals, 4000 resamples (results/schedule_attack/REPORT.md:3-5).
- proportions: we computed Wilson and Clopper-Pearson intervals and name which is which. for 8/10 the Clopper-Pearson interval is [0.44, 0.97]; the Wilson bounds are not printed here because the repository record does not contain them.
- counts: for 8 false alarms over 6000 s we report the rate 0.8 per 600 s; a Poisson interval was computed but its bounds are not printed because the repository record does not contain them.
- moves per party: the repository records a mean near 98 per 600 s (README.md:39); the brackets in that line are a range, not a confidence interval, and a recomputation from the shipped journals gives a mean below 100 with an interval that overlaps 100.
- censoring: when a party never reaches 0.9, we report the fraction of parties that reached it and the party length distribution. the sentinel 500 is not an estimate of L.
- multiplicity: family wise alpha 0.05 with per hypothesis 0.0083 was registered for the h1 to h6 atlas block (docs/hypotheses.md:241-245). the h9, h10 and schedule attack measurements were not corrected for multiplicity; every metric in the exploratory schedule attack is reported, and we do not select a winner. we flag this as a preregistration gap, not as a corrected analysis.

## 3 results

### 3.1 closure, determinism and replay

the loop is closed in the sense that the journal shows cause codes for endogenous reversal (cause 1) and detector firing (cause 2) on the same rows that change the port. in the 10 calibration journals, every port change is accompanied by a nonzero cause code (1, 2 or 3). for example, results/cal_v12/fa_101.csv has 152 port changes, 150 cause 1 rows and 3 cause 2 rows; the one cause flagged tick without a change is a no op. this is consistent with the motor being driven by the reflex, not by a timer.

determinism: two runs with seed 42, 600 s, tests/fixtures/burst.csv produced identical journals (the comparison is recorded in 5.1). the current binary hash matches the manifest at commit 2cf9b1f6 (results/manifest.txt:2-6). the h9 journals were produced by a different binary at commit 4c5eb5d (results/journals_v12/bio/run_summary.txt:3-4). we regenerated one of the 300 shipped journals (bio, seed 1001) with the current binary and it matched byte for byte. one of 300 is checked, not all 300.

### 3.2 detection at a frozen threshold

the frozen threshold is 50.0, the lowest value in the tool grid at which the false alarm budget is met (results/cal_v12, recomputed with tools/calibrate_detector.py):

| threshold | false alarms over 6000 s | detections, event window |
|---|---|---|
| 45 | 12 | 0/10 |
| 50.0 | 8 | 3/10 |
| 60 | 1 | 0/10 |
| 70 | 0 | 0/10 |

the false alarm statement is a count, not a proportion of seeds: 8 false alarms across 10 seeds times 600 s, rate 0.8 per 600 s; the Poisson bounds are not printed because the repository record does not contain them. the often quoted "8/10 seeds" is the wrong denominator.

detection is criterion dependent. with the criterion "maximum detector score at or above 50.0 anywhere in the 600 s journal", 8 of 10 burst journals fire. with the criterion "a firing inside the first 3 s after the first event at 1000 ms", 3 of 10 fire. five of the eight max score firings occur before the first event, at 920, 930, 850, 800 and 970 ms (results/cal_v12/det_burst_101, 104, 105, 107, 108). the proportions are 8/10 with Clopper-Pearson [0.44, 0.97], and 3/10; the corresponding Wilson and Poisson bounds are not printed here because the repository record does not contain them. neither criterion was registered before the analysis; both are reported and we make no detection rate claim. the earlier r1 artifact results/detector_threshold.csv (threshold 30.0, 7/10, Wilson [0.40, 0.89]) is stale, contradicts h10 and src/config.h, and is excluded from this paper.

quiet move rate: 10 calibration journals, seeds 101 to 110, 600 s each. the repository records a mean near 98 per 600 s (README.md:39, docs/hypotheses_results.md:58-61). the bracketed range in that line is not a confidence interval. the mean is below the preregistered band of 100 to 300 per 600 s; a recomputation from the shipped journals gives a mean below 100 and an interval that overlaps 100, so this is a recorded miss, not a statistical rejection. the repository root also contains older calibration journals with a different move count; those are from a different build, are not the h10 measurement, and are not used here.

### 3.3 schedule learnability: h9

design: 300 parties, 100 seeds (1001 to 1100) times 3 destination modes, same burst input, same binary (docs/hypotheses_results.md:19-23). one party per seed. table entries recomputed here from results/journals_v12 with tools/learnability.py, taking the port sequence to include the initial port as the h9 analysis does.

| family | parties reaching 0.9 | L, median | party length, moves |
|---|---|---|---|
| bio | 0/100 | 500 (sentinel) | 45 to 142 |
| random | 0/100 | 500 (sentinel) | 58 to 155 |
| cycle | 88/100 | 65 | 59 to 156 |

the registered criterion 1 ("bio L at least 500 in 95%") is tautological as written, because the tool assigns exactly 500 whenever 0.9 is not reached and also whenever the party has fewer than 20 moves. we replace it with the fraction of parties reaching 0.9 (0/100) and the party length distribution. the sentinel cap 500 was never reached by any party: the longest is 156 moves, so L is bounded by the sequence length minus 10 in practice.

we do not report Cohen d or a Wilcoxon p for bio against cycle. the bio vector is constant at the sentinel 500 and the random vector is also constant, so a test would describe the sentinel, not a schedule effect. "bio and random are indistinguishable" is also not a statistical statement; the correct statement is that both failed to reach 0.9 and the test is impossible under this instrument at these party lengths.

criterion 2 failed as registered: 88/100, not 95%. all 12 misses have party length 59 to 75 moves; all hits have at least 76 moves. the crossing at 65 lies beyond the data when the party is shorter than 76, so this is measurement censoring, not a schedule change. criterion 3 failed as registered: the bio readout returns the same port on consecutive moves, so port change counts undercount motor events. the recorded no-op fraction is 13% (docs/hypotheses_results.md:44); a recomputation from the shipped bio journals gives a slightly higher fraction, and the denominator differs between the two counts. motor event counts per seed are identical across modes; timing is scenario driven.

### 3.4 schedule attack at n = 20

the 20 journal reanalysis samples seeds 1001 to 1010 and 1091 to 1100 from each family. all intervals are bootstrap 95% with 4000 resamples and are reported in results/schedule_attack/REPORT.md §1 to §3; we cite the report for the exact bounds and do not reprint values that this draft's numeric audit cannot trace to the four source files it covers.

the reanalysis confirms the reconnaissance ordering. bio is far from uniform: its port entropy is about 1.3 bits below the uniform null, its two most frequent ports carry about 23.4% of moves, and its first move is 40032 in 20/20 journals. its held out order 1 accuracy is above the 1/64 uniform rate, but most of that edge is shrinkage of the observed alphabet, not structure an attacker can use: the predictor still does not reach 0.9 within 87 moves (0/20). cycle is deterministic: a counter or delta rule explains 20/20 journals, the delta rule reaches 0.9 after 2 moves (20/20), and a Markov attacker needs 62 moves (14/20). random is consistent with a uniform draw: an affine LCG mod 64 explains 0/20 (best fit near 0.08), a GF(2) affine 6 bit map 0/20, and xorshift64* state recovery from low 6 bit outputs 0/20, with a synthetic positive control at 20/20; the best fit for bio is near 0.14 and is consistent with favorite port pairing, not a generator. inter move intervals are memoryless in all families: the Ljung-Box test does not reject, lag 1 interval autocorrelation is near zero, and the coefficient of variation is between 1.3 and 1.4 (REPORT §2). an estimate of about 303 moves to enumerate all 64 ports is an extrapolation beyond the 600 s window and is labelled as such.

entropy bias: the plug in estimator is biased upward. the Miller-Madow correction would be 0.56 bits for cycle and 0.40 for random, and smaller for bio because its observed alphabet is smaller; these follow the formula recorded at RESEARCH_V2.md:190-193. at a sample to alphabet ratio near 1.3 the correction is outside its domain of applicability, so we do not publish corrected values as truth; we state the plug in values and the direction of bias. a defensible estimate needs NSB or Chao-Shen, or at least 30 seeds (RESEARCH_V2.md:188-193).

### 3.5 negative results and open defects

these are the contribution, not footnotes.

- N1, no entropy superiority. at alphabet 64 the ceiling is 6 bits per move. the L saturated for both bio and random (0/100 each), which measured the instrument, not a tie (README.md:54-57, CLOSE.md:32-35).
- N2, the random arm is reconstructable. the random destination is xorshift64* seeded from the command line. replay over seeds 0 to 200000 leaves 3177 candidates after 1 observed port, 52 after 2, and 1 (seed 1001) after 3; the recovered seed replays all 80 moves. the recorded attack time is 0.6 s (README.md:58-60, docs/hypotheses_results.md:72-87); a rerun on this machine took about one second. this is one seed, reconnaissance, and the construct is observations to recovery = 3, not L. the n = 20 journal attacker that does not know the generator recovers 0/20, so the attack requires knowledge of the generator code (REPORT §4).
- N3, the bio readout is rank one. the schedule attack finds a few tens of distinct ports and an entropy deficit of about 1.3 bits against the uniform null, n = 20 (REPORT §1); the single seed reconnaissance gave 16 ports and 3.672 bits (appendix a). this is a readout defect, not a connectome property (src/bridge.c:96-108).
- N4, h2 is falsified. the destination is not chosen by the connectome: the port is the base plus the bind counter modulo the window, a pure ascending counter (src/motor.c:133). on seeds 1001, 1002, 1050 and 1100 the sequence is a 100.0% ascending cycle of 208 to 210 moves, and L = 65.0 with standard deviation 0 in 100/100 bio and 100/100 flat parties (docs/hypotheses.md:338-363).
- N5, the secret permutation buys nothing and breaks the journal. a fixed bijection of the output alphabet is absorbed by any learner, so measured accuracy is identical before and after relabeling (F3). separately, headless ignores the permutation while live applies it (src/motor.c:155, 162), so journal based measurements of the live schedule are invalid (F2) (RESEARCH_V2.md:74-87).
- N6, move rate miss. the recorded mean is near 98 per 600 s, below the preregistered 100 to 300 band at the mean; a recomputed interval overlaps 100, so this is not a statistical rejection (docs/hypotheses_results.md:58-61).
- N7, h9 criteria 2 and 3 failed as registered. 88/100 at L = 65 instead of 95%, with the 12 misses all shorter than 76 moves; and the recorded no-op fraction is 13%. data are published as is and nothing was rerun to fit the gate (docs/hypotheses_results.md:35-48).
- N8, marginal H is not unpredictability. cycle has H(port) 5.920 bits out of 6 on the single seed reconnaissance, yet a delta rule attacker needs 2 moves and a Markov attacker 62; H(port) must not be published as schedule entropy (RESEARCH_V2.md:89-94, REPORT §3).

## 4 limitations

### 4.1 what is not claimed

- no entropy superiority of biology over a PRNG; the opposite is established at the alphabet ceiling (README.md:54-57).
- no cryptographic security proof and no claim of a hardened product (docs/RELEASE_V1.md:5-7, 41-44).
- no detection rate claim: the 8/10 and 3/10 numbers are criterion dependent and neither criterion was preregistered.
- no coupling claim. h13 was never run (docs/hypotheses.md:489-502); it is future work only.
- no external validation. no comparison against Brian2 or NEURON was made, and no external user tested the system (CLOSE.md:68-71).
- no claim about live deployment: the permutation divergence means the journals describe a different stream than the daemon emits (N5).
- no claim about 279 neurons as a measured count; the runtime count is not logged.

### 4.2 what was not measured or not run

- the privileged bench: kernel nftables enforcement, active probe in a netns, gcore audit and seccomp negative test were never run (docs/RELEASE_V1.md:6, README.md:136-139). pending, not claimed.
- held out seeds 2001 to 2100 were never run for any result in this paper (docs/hypotheses_results.md:4-5).
- h1 to h4 and h6 have no verdicts on the current core (docs/hypotheses_results.md:64-70); h5 was measured on the old core and is excluded (docs/hypotheses_results.md:7-17); h12 to h15 were never run (docs/hypotheses.md:474-524).
- the atlas chain and the threat model in paper.md are placeholders with no numbers; the atlas chain was abandoned (docs/STATE.md:43).
- external reconstruction cost with a full entropy seed was not measured; RESEARCH_V2.md:65-69 marks it UNCERTAIN.

### 4.3 defects that limit the measurements

1. rank one readout (src/bridge.c:101-107) bounds the bio alphabet to a few tens of ports.
2. live and headless diverge (src/motor.c:155, 162); all journal results describe the pre permutation stream.
3. the h9 journals were produced by a binary at commit 4c5eb5d, not by the manifest binary at commit 2cf9b1f6. one journal of 300 was regenerated and matched; the other 299 were not checked.
4. the detection criterion and the L search range were not frozen before analysis. this is a selection gap: a max anywhere criterion and an event window criterion give 8/10 and 3/10. results/detector_threshold.csv is stale and contradicts the frozen threshold.
5. L as implemented is not the literature quantity; it is a 20 move accuracy threshold quantized by 5 and censored by party length.
6. the 300 party h9 design has one party per seed, so the fraction reaching 0.9 has a binomial n of 100 parties but no seed level replication of party length.
7. no multiplicity correction was applied to the exploratory schedule metrics.
8. the false alarm count and the detection proportion have different denominators and should not be combined.

## 5 reproducibility

### 5.1 build and run

```bash
make                       # cc -std=c99 -pedantic -Wall -Wextra -Werror -O2
make test                  # unit and replay determinism
./proteusd -s 42 -d 600 -e tests/fixtures/burst.csv -o /tmp/a.csv
./proteusd -s 42 -d 600 -e tests/fixtures/burst.csv -o /tmp/b.csv
cmp /tmp/a.csv /tmp/b.csv && echo "bitwise identical"
sha256sum /tmp/a.csv /tmp/b.csv
```

the two runs above produced identical files. recorded output of the check run for this draft:

```text
bitwise identical
```

the current binary hash is recorded in results/manifest.txt as the manifest binary at commit 2cf9b1f6; the shipped h9 journals name a different binary at commit 4c5eb5d (results/journals_v12/bio/run_summary.txt:3-4). regenerating the bio seed 1001 journal with the current binary and comparing it to results/journals_v12/bio/seed_1001.csv matched, which is 1 of 300 checked.

### 5.2 attacks and analyses

```bash
tools/seed_recovery_attack.py results/research_v2/random_1001_moves.csv --k 3
```

the command reported one candidate, seed 1001, and a full sequence match over 80 moves.

the same command with k = 1 reports 3177 candidates and with k = 2 reports 52 (ran here). the recorded wall time is 0.6 s; a rerun on this machine took about one second, and timing is machine dependent.

detector calibration:

```bash
python3 tools/calibrate_detector.py "results/cal_v12/fa_*.csv" "results/cal_v12/det_burst_*.csv"
```

at threshold 50.0 this prints 8 false alarms over 6000 s and 3 detections of 10 under the event window criterion. the 8/10 max score criterion was computed separately from the same journals; the five pre trigger times are listed in 3.2.

learnability recomputation:

```bash
python3 - <<'PY'
import csv, glob, sys
sys.path.insert(0, 'tools')
from learnability import LearnabilityComputer
def seq(p):
    out = []; prev = None
    for r in csv.reader(open(p)):
        if not r or r[0].startswith('#') or r[0] == 't_ms': continue
        x = int(r[8])
        if prev is None: out.append(x); prev = x; continue
        if x != prev: out.append(x); prev = x
    return out
for fam in ('bio', 'cycle', 'random'):
    fs = sorted(glob.glob(f'results/journals_v12/{fam}/*.csv'))
    hit = [len(seq(f)) for f in fs if LearnabilityComputer().compute_L_passive(seq(f)) < 500]
    print(fam, len(hit), 'of', len(fs), 'party lengths', min(hit), max(hit))
PY
```

this reproduced 0/100 bio, 88/100 cycle and 0/100 random, with cycle hits at 76 moves or more and misses at 59 to 75 moves.

### 5.3 manifest and artifact status

results/manifest.txt records commit 2cf9b1f6, the compiler, flags, the binary hash, and hashes of tracked artifacts under docs, results, tools and tests (results/manifest.txt:1-6). the current binary hash matches the recorded value (checked here). the repository audit reports 720 of 721 hashes matched, the exception being the manifest's self reference line; coverage excludes src, README.md, paper.md and the Makefile by design (docs/RELEASE_V1.md:46-53). the cross check here adds: journals_v12 run summaries record a different binary and commit, which the manifest does not encode. a reader who wants to reproduce the h9 table must use that binary or accept the one journal check reported above.

the schedule attack computations were run in /tmp and the scripts are not shipped; results/schedule_attack/REPORT.md:48 says so. this is a reproducibility gap: the report's numbers can be checked against the journals, as done here for h9, but the attack pipeline is not in the tree.

## appendix a: exploratory single seed reconnaissance (n = 1)

conditions: seed 1001, 600 s, tests/fixtures/burst.csv, headless, the binary and commit recorded in docs/RESEARCH_V2.md:21-25. these numbers are reconnaissance, not estimates: one seed, 69 to 81 moves, no confidence interval. they are superseded by the n = 20 reanalysis in 3.4, which is cited but whose exact interval bounds are left in the report.

| mode | moves | distinct | H(port) bits | held out order 1 | dwell mean ms | observations to recovery |
|---|---|---|---|---|---|---|
| cycle | 81 | 64 | 5.920 | 0.531 (1.000 on seen states) | 7443 | not applicable |
| bio | 69 | 16 | 3.672 | 0.185 | 8647 | not applicable |
| random | 80 | 45 | 5.290 | 0.000 | 7443 | 3 |

the H values, distinct counts and move counts reproduce exactly from results/research_v2/*_moves.csv. a simple deterministic predictor did not reproduce the cycle order 1 value 0.531 exactly; tie breaking differs, while the direction and the 1.000 on seen states reproduce. the cycle dwell entry in the document table has been questioned: the shipped move file gives a slightly lower mean, and the difference does not affect any claim. the document level caveat is required: CLOSE.md:36-47 quotes 3.672 bits and 0.185 against 0.000 without an n = 1 marker; this appendix supplies it.

h11 is a post hoc registration: the measurement was made during reconnaissance before the hypothesis was written down (docs/hypotheses.md:453-459). the verdict is published as reconnaissance, not as a preregistered test, and it is excluded from the confirmatory family (docs/PLAN_V11.md:114).

## honesty_check

- main claim, closed loop without a policy layer: supported by structure and journal cause codes; every port change in the 10 calibration journals carries a nonzero cause code (1, 2 or 3). source, README.md:88-101, CLOSE.md:17-28, results/cal_v12/fa_101.csv. not independently audited for goal directed constructs in this session.
- main claim, bitwise replay: supported for seed 42 over 600 s with the current binary; both runs are byte identical and the comparison is in 5.1. the 100 party claim in CLOSE.md:21-22 is documented but not rechecked here; status, verified on one seed, documented on 100.
- main claim, L spans two fields: definition is common (docs/theory.md:1-10), but only the schedule side is measured in this paper. worm behaviour L is not reported.
- main claim, detection at a frozen threshold: weak and criterion dependent. 8/10 max anywhere with Clopper-Pearson [0.44, 0.97]; 3/10 event window. no detection rate is claimed.
- main claim, false alarm budget: supported. 8 false alarms over 6000 s at threshold 50.0, rate 0.8 per 600 s; the Poisson bounds are not printed because the repository record does not contain them.
- main claim, threshold selection: supported for 50.0 as the lowest tool grid value meeting the budget. the 55:3 row in docs/hypotheses_results.md:56 is not in the tool grid and is not used.
- main claim, h9 cycle learnability: supported. 88/100 reach L = 65; all 12 misses have party length 59 to 75, all hits at least 76 (recomputed from results/journals_v12).
- main claim, h9 bio and random learnability: both 0/100 reached 0.9; party lengths 45 to 142 and 58 to 155; sentinel 500. status, supported as censoring, not as equality or as "L at least 500".
- not reported, h9 effect sizes d = 3.81 and p = 6.6e-21: deliberately omitted. the bio and random vectors are constant at the sentinel, so the test describes the sentinel, not a schedule effect.
- main claim, bio readout is rank one: supported. formula at src/bridge.c:101-107; the schedule attack finds a few tens of distinct ports and an entropy deficit of about 1.3 bits against the uniform null, n = 20 (REPORT §1).
- main claim, random schedule consistent with uniform: supported at the observable level. 0/20 LCG, 0/20 GF(2), 0/20 xorshift state recovery with a 20/20 positive control; the report gives the exact entropy and chi square intervals (REPORT §1, §4).
- main claim, random arm reconstructable: supported with a scope limit. 3177, 52 and 1 candidates after 1, 2 and 3 ports, seed 1001, 80 move replay, 0.6 s recorded; single seed reconnaissance; observations to recovery = 3, not L. the blind n = 20 journal attacker recovers 0/20, so knowledge of the generator code is required.
- main claim, bio leaks but cannot reach 0.90: supported. first move 40032 in 20/20, the two favorite ports carry about 23.4% of moves, and 0.90 is not reached within 87 moves (0/20).
- main claim, marginal H is not unpredictability: supported. cycle H 5.920 bits on the single seed reconnaissance with delta rule 2 moves and Markov 62.
- main claim, move rate: weak. the recorded mean is near 98 per 600 s; the bracketed range in the record is not a CI, the mean is below the band and a recomputed interval overlaps 100; this is a recorded miss, not a statistical test.
- not claimed, entropy superiority of biology over a PRNG: refuted; alphabet 64 caps at 6 bits per move.
- not claimed, coupling h13: never measured (docs/hypotheses.md:489-502); future work only.
- not claimed, live deployment properties: headless ignores the permutation, live applies it (src/motor.c:155, 162); journal results describe the pre permutation stream.
- not verified, privileged bench: nftables kernel enforcement, active probe in netns, gcore and seccomp negative test were not run (docs/RELEASE_V1.md:6, README.md:136-139); all pending.
- not verified, held out seeds 2001 to 2100: never run for any result here (docs/hypotheses_results.md:4-5).
- not verified, h1 to h4, h6 verdicts: not measured on the current core; no verdict claimed. h5 measured on the old core and excluded; its corrected interval is in the audit, not reprinted here.
- not verified, 279 neurons as a runtime count: config comment only; the CSV has 281 nodes and 6417 edges after its header; the loader filter count is not logged.
- not verified, all 300 h9 journals against the manifest binary: one of 300 checked (bio seed 1001, matched); run summaries name a different binary and commit than the manifest.
- retracted, entropy rates from docs/deep_research_bio_vs_prng.md: all bits per second and h_KS and sigma above zero numbers have no source and are withdrawn (docs/RESEARCH_V2.md:173-180); none are cited here.
- retracted, secret permutation as a security feature: refuted by F3 and it breaks live/headless by F2; it appears here only as a negative result.
- retracted, "bio vs random indistinguishable": replaced by "both failed to reach 0.9 and the test is impossible"; this is saturation, not equality.
- stale artifacts: results/detector_threshold.csv (30.0, 7/10, [0.40, 0.89]) and the repository root calibration journals are from earlier builds and are excluded; results/cal_v12 is the source for threshold 50.0 and the move rate record.
- reproducibility gap, schedule attack pipeline: computations were run in /tmp and are not shipped (results/schedule_attack/REPORT.md:48); the report's numbers can be checked against the shipped journals, but the pipeline cannot be rerun from the tree.
- selection gap, detection criterion and L search range: neither was frozen before analysis; both detection criteria are reported; no metric was selected as the winner.
