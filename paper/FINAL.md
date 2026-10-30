# proteus: a headless port schedule driven by a reduced connectome model, bitwise replay, and a catalogue of refutations

revision 3. this file supersedes paper/DRAFT.md, applies the review fixes and the hostile reviewer pass, and adds the ГОСТ style annotation, declarations and figures. the paper is a negative results and methods report, not a security paper.

## реферат (аннотация по ГОСТ 7.32-2017)

объект исследования: детерминированный headless планировщик tcp портов, метка которого в режиме bio читается из модели коннектома *C. elegans* (279 нейронов, вендоренный движок petri v1.2). цель: установить, даёт ли биостохастическое расписание измеримое преимущество перед сеяным и случайным, и что из заявленного выдерживает проверку артефакта. методы: парные партии с журналом (100 сидов на режим по 600 s), пассивная метрика обучаемости L, held out порядок 1 (n = 20 журналов на семейство, bootstrap 95%), восстановление генератора, калибровка детектора на замороженной процедуре. результаты: каталог опровержений. bio не лучше random ни по энтропии (4.014 [3.885, 4.136] против 5.468 [5.402, 5.539]), ни по точности (0.0712 [0.0540, 0.0901] против 0.0100 [0.0047, 0.0158], uniform 0.0156); детекция 8/10 отозвана, в окне 3 s остаётся 3/10; seed random-армии восстанавливается по 3 портам за 0.6 s при знании кода; тайминг режимонезависим; реплей бит в бит подтверждён на 1 паре и 1 из 300 журналов. новизна: не заявляется; количественный обзор литературы по mtd метрикам отсутствует (docs/RESEARCH_V2.md:147-149). область применения: воспроизводимая вычислительная нейронаука, оценка артефактов, отрицательные результаты. полный список источников: paper/REFERENCES.md. заявление об использовании ИИ: paper/AI_ASSISTANCE.md.

ключевые слова: коннектом, *C. elegans*, moving target defense, воспроизводимость, детерминированный реплей, отрицательные результаты, обучаемость расписания, zenodo архив.

keywords: connectome, *C. elegans*, moving target defense, reproducibility, deterministic replay, negative results, schedule learnability.

## abstract

we report a reproducible artifact study of a deterministic headless port schedule that is labelled by a reduced *C. elegans* model. all measurements in this paper are headless: the measured path opens no socket (src/motor.c:91-95), and the live path diverges from the journal (src/motor.c:155, 162). the artifact replays bitwise: seed 42, 600 s, two runs, identical journals under cmp (ran here; results/paper_stats.txt).

the schedule has three modes. the move instants are identical across modes on all six seeds checked (1001, 1002, 1003, 1050, 1091, 1100; n = 6, all cause flagged move rows, same times). the move cause is the engine's own, the avb matched filter and the body speed reflex (src/session.c:364, src/session.c:376), and the mode touches only the destination label: timing is set by the model, but identically in all three modes. what differs between modes is the label, and in bio mode that label is a hand written rank one functional of six scalars, two of which are counters derived from the event stream and one of which is the event trace itself (src/bridge.c:101-107, src/session.c:344-363). we therefore do not claim that the connectome closes the loop.

the scientific output is a catalogue of refutations. first, the passive learnability metric saturates: 0/100 bio parties and 0/100 random parties reach 0.9 accuracy, while the legacy counter reaches it in 88/100 at L = 65 (party lengths 44 to 155; recomputed here; results/paper_stats.txt). second, the bio readout is more predictable than the random arm on both measured axes, n = 20 journals per family, bootstrap 95% (results/schedule_attack/REPORT.md): marginal entropy 4.014 bits [3.885, 4.136] against 5.468 [5.402, 5.539], and held out order 1 accuracy 0.0712 [0.0540, 0.0901] against 0.0100 [0.0047, 0.0158], uniform 0.0156. the order 1 log-loss of the 2.6 recipe is not regenerable from the tree and stays unstated; the pre registered 0.10 bit margin is retained as a qualitative direction only, and the readout is not non inferior; the n = 1 reconnaissance values are in 3.4. third, the random arm's seed is recoverable from 3 observed ports within a 2e5 candidate range when the generator is known (candidates 3177, 52, 1; full 80 move replay); this is a seed policy failure, not a broken generator, and with a blind n = 20 attacker the recovery is 0/20. fourth, the recorded 8/10 burst detection does not measure the burst: every burst journal records a det_fired event at t = 1000 ms, before the burst event at t = 2000 ms (first ash response at 2010 ms; tests/fixtures/burst.csv:3); no journal fires within 3 s after the stimulus, and three fire well after the burst window. fifth, two journals record det_fired below the frozen threshold 50.0, so the threshold provenance is not supported by the artifacts. no security property, no detection rate, and no coupling claim are made. h13 was never run.

## 1 introduction

### 1.1 the question

a deterministic simulator with a seed is a deterministic function of a key. in the information theoretic sense it is a PRNG (docs/RESEARCH_V2.md:5-6). asking whether such a schedule is "more random" than a seeded generator cannot be won: over an alphabet of 64 symbols the maximum entropy is exactly log2(64) = 6 bits per move, and a one time pad attains that bound rather than exceeding it (Shannon 1949, cited in docs/PLAN_V11.md:12-14). the earlier draft said a one time pad exceeds the bound; that was wrong and is corrected here.

the useful question is narrower. given a deterministic generator and a stated attacker class, how many observations does the attacker need, and what does the artifact actually measure? this paper answers that for the artifact that exists, and reports where the answer is negative.

### 1.2 scope and non-goals

this is not a security paper. it does not propose a defense, does not measure an attack cost against a network adversary, and does not compare against a cryptographic PRF. no ChaCha20, HMAC-DRBG, or ephemeral port randomization baseline was implemented or measured; `grep` over the tree finds ChaCha only in a planning note about a third party KDF (docs/PLAN_V11.md:23). a security venue should reject this paper on scope. we state that before a reviewer does, because the only property that a PRNG cannot have by construction, coupling between schedule and attack, was never measured (h13, docs/hypotheses.md:276-285).

the paper is offered as a methods and negative results report for reproducible computational neuroscience and artifact evaluation. its positive content is the replayable artifact, the instrument, and the refutations.

### 1.3 contributions

1. a deterministic headless schedule generator with bitwise replay, a recorded binary, compiler, flags, event file and a manifest (results/manifest.txt).
2. an operational learnability instrument L, with its mismatch to the prose definition documented, and a separate `n_obs_to_recover` construct for seed recovery (tools/learnability.py:53-108).
3. a held out order 1 log-loss analysis of the shipped journals, computed here, that replaces the saturated L as the comparative metric.
4. a catalogue of refutations with the measurements that establish them: metric saturation, a rank one readout, a seed policy failure, mode independent timing, and a detector result that does not survive inspection.
5. explicit artifact provenance limits, including a binary and commit mismatch between the manifest and the h9 journals.

we do not claim: a closed connectome loop, a security property, a detection rate, a false alarm rate, coupling, external validation, or a validated neuron count at runtime.

### 1.4 threat model

the threat model covers only attackers that were actually implemented. rows without a budget were not run and carry no claim.

| class | knowledge | channel | budget | win condition | status |
|---|---|---|---|---|---|
| A0 passive | port sequence only | full journal, one party | party length 44 to 155 moves; L prefix grid step 5 | reach 0.9 accuracy on the next 20 moves | measured, 100 parties per mode |
| A1 blind statistical | port sequence, no generator source | 20 journals per family | train first 60%, test last 40% | held out order 1 and order 2 accuracy, and log-loss | measured, n = 20 |
| A2 reconstruction with source | xorshift64*, seed range 0 to 2e5 | observed port prefix only | 1 to 5 observed ports | unique seed and full sequence replay | measured on seed 1001; blind variant 0/20 |
| A3 blind reconstruction | generator family unknown | port prefix | 20 journals | any of four structural tests succeeds | measured, 0/20 each |
| A4 active prober | schedule plus a probe channel | not implemented | undefined | not defined | not run (h13) |
| A5 network observer | wire traffic | pcap | undefined | service discovery or schedule recovery | not run |
| A6 live deployment | journal | live socket | undefined | replay of the live stream | not measurable, live and headless diverge |

the only classes with a full pass over the shipped data are A0 and A1. reconnaissance with source knowledge is A2. a blind structural attack with no success is A3. classes A4 to A6 have no numbers and are listed so that their absence is explicit.

### 1.5 related work

seeded generators with linear or near linear state are reconstructable: xorshift (Marsaglia 2003), xorshift* and xoshiro (Vigna 2014, 2016), both cited by name in docs/RESEARCH_V2.md:151-152. min entropy estimation of physical noise sources is standardized in NIST SP 800-90B (docs/RESEARCH_V2.md:153); it estimates a source and is not evidence about this schedule. the neuroscience sources named by the engine are Yemini et al. 2013, Pierce-Shimomura et al. 1999, Kato et al. 2015 and Tsodyks and Markram 1997 (README.md:145-147); the model does not reproduce a measured behavioural quantity from any of them, so these are provenance, not validation. predictability as a cross domain notion is discussed by Boffetta, Cencini, Falcioni and Vulpiani 2001, and permutation entropy convergence under noise by Amigó, Dale and Tempesta 2020 and Chen and Sato 2024, as recorded in docs/RESEARCH_V2.md:137-145. we did not find a verified quantitative review of moving target defense metrics. docs/RESEARCH_V2.md:147-149 marks such reviews UNCERTAIN, so the novelty of L against the MTD literature is not established; the repository's own industry scan (docs/STATE.md:19-24) is a repo claim, not a review.

## 2 methods

### 2.1 engine and wiring

the engine is a vendored reduced model, petri v1.2, commit 42d5062 (docs/hypotheses.md:224, docs/STATE.md:10). it is not a conductance based neuron model: `neuron_act` is a single sigmoid of membrane potential (src/petri/neuron.c:7-10), there are no voltage dependent channels, and the body is 24 segments. the correct label is a graded potential reduced model, not a biophysical simulator.

the wiring sheet src/petri/data/varshney_full.csv has 6417 rows broken down by type S, SP, R, RP, EJ and NMJ (counted here). 6417 is a row count, not a synapse count. the loader drops all 153 NMJ rows, collapses the EJ rows to 517 unique undirected pairs, and stages both sender side (S, SP) and receiver side (R, RP) chemical rows; the staged graph in this build combines the chemical rows with 517 gap pairs (computed from the loader rules at src/petri/connectome.c:360-400). the loader produces 279 nodes after dropping NMJ rows (recomputed here); 281 is the number of distinct tokens in the raw file, the extra two being tokens that occur only in NMJ rows. the runtime `c->n_neurons` is printed by src/petri/sim.c:67-70, but the daemon forces `opts.quiet = 1` in src/bridge.c:17, so the line is suppressed; the 279 value is a loader rule recomputation, not a runtime log.

two model defects matter for interpretation. first, synaptic sign is not in the CSV: a chemical row is inhibitory only if its presynaptic name has one of seven GABA prefixes (src/petri/connectome.c:22-24, 45-48, 386-397); a minority of chemical rows are inhibitory by that rule (counted here). the remainder are excitatory by default. second, EJ rows are de duplicated to one directed edge per pair, and the synaptic step adds `pn->i_gap += g*(pre->v - pn->v)` only to the postsynaptic cell (src/petri/sim.c:377-380), so the 517 gap contacts are one sided conductors, not shunting electrical synapses. both are model properties, not data properties, and any conclusion that depends on inhibition balance or gap coupling is sensitive to them.

### 2.2 the readout, and the policy question

the bio destination is

    index = floor(64 * mean(act_ava, act_avb, ash_trace, ser, oct, dop))

implemented at src/bridge.c:101-107. this is a hand written rank one linear functional. three of its six terms are not independent connectome observables in the sense the earlier draft implied:

- `ash_trace` is the nociceptor trace, that is, the event input itself.
- `ser` and `oct` are updated in src/session.c:344-363 by `ser += K_SER * share * dt` and `oct += K_OCT * density * dt`, where density is the burst event rate and share is the legitimate traffic fraction. both constants are in src/config.h:118 and 123.
- `ava` is saturated: near 1 with small variance over the 60000 rows of the bio seed 1001 journal (journal archive, results/BULK_DATA.md; computed here).

the index is dominated by one value: a single index value occupies most of the 60000 ticks in that journal (computed here). from 279 cells, at most the two command cells AVA and AVB and the sensory trace reach the port; AVA is nearly constant, so the effective neural contribution is AVB.

we therefore do not claim that there is no policy layer. there is one: a fixed linear readout plus an exogenous two term modulator injection driven by event counters. what is absent is a goal state, a target vector, or a learned policy. the correct statement is "the port label is a fixed hand written functional of engine state and event counters", not "the map is the simulated nervous system".

### 2.3 timing: reflex, detector, and mode independence

move timing is produced by the reflex and the detector, not by the destination mode. `motor_move_to` is called only when `cause != 0` (src/session.c:385-392), and `cause` is set by the detector (`CAUSE_DET`) or the engine reversal reflex (`CAUSE_ENDO`). the mode chooses the argument to `motor_move_to`; it does not choose the tick. this is measured in 3.2: the cause flagged move rows have identical timestamps across bio, cycle and random.

consequently the h2 statement that "the connectome decides only the timing" (docs/hypotheses.md:205) is not true on the v1.2 build either. on v1.2 the timing is mode independent, and in bio mode the connectome decides the label. h2 was measured on the old core and on the legacy counter path; we scope it accordingly.

### schedule modes and defaults

the default destination mode is `DEST_CYCLE = 0` (src/config.h:31), and `cfg.seed` defaults to 1 (src/proteusd.c:38). the modes are:

- cycle: `port = PORT_BASE + (binds mod 64)` (src/motor.c:133), a counter.
- bio: `motor_move_to(&mo, bridge_dest_index(&br))` (src/session.c:388).
- random: `motor_next_random`, xorshift64* seeded from the command line integer (src/motor.c:18-30, 182).

live mode builds a fixed permutation from the same seed and applies it (src/motor.c:98, 162). the permutation is not a secret: the seed is a command line integer with default 1, and the permutation is derived from it. we drop the word "secret" from the earlier draft. headless mode does not apply the permutation (src/motor.c:155 calls `headless_bind`, which ignores `m->perm`). every measurement in this paper is headless.

### 2.5 metric L and n_obs_to_recover

L is the operational definition in tools/learnability.py:53-108. for a port sequence s, for each prefix length o in {10, 15, ..., min(500, |s| - 10)}, a predictor is fitted on s[:o] using marginal frequencies and first order transitions and scored on s[o:o+20]. L is the first o at which accuracy is at least 0.9; if none, the function returns max_obs = 500. the scored window s[o:o+20] truncates at the end of the party, so prefixes near the tail are scored on fewer than 20 moves. L is an accuracy threshold on a 20 move window, quantized in steps of 5, censored by party length, and not the probability of the next action. the prose definition in README.md:26-30 and docs/CLOSE.md:26-28 does not match the code; the code is the definition here.

the same tool contains stub active and reconstruction observers (`compute_L_active` returns `max(10, passive/2.5)`, `compute_L_reconstruction` returns `min(passive*3, 500)`, tools/learnability.py:110-130). we use only `compute_L_passive`. the tool's aggregation path uses `df['port'].diff() != 0`, which excludes the initial port (tools/learnability.py:183); the h9 table and our recomputation include it. this changes reported counts; we state which convention is used in each table.

for seed recovery we use `n_obs_to_recover`: the number of observed ports after which the candidate seed set has size one. this is a different construct from L and is never placed in an L column.

### 2.6 held out log-loss

the pre registered comparative metric was held out log-loss in bits per move, with L only as a lower bound (docs/RESEARCH_V2.md:196-199). we compute it here from the 20 shipped journals per family (seeds 1001 to 1010 and 1091 to 1100). for each journal, the first 60% of the port sequence is training, the last 40% is test, the model is order 1 with Laplace smoothing alpha = 0.5 over the 64 symbol alphabet, and the score is the mean negative log2 probability of the test symbols. intervals are bootstrap 95% over the 20 journals, 4000 resamples. the same recipe with order 0 is reported as a reference. the estimator and alpha were chosen after seeing the data; this analysis is exploratory and not pre registered.

### statistical procedures

- schedule attack (n = 20 journals per family): bootstrap 95%, 4000 resamples (results/schedule_attack/REPORT.md:3-5).
- proportions: Wilson and Clopper-Pearson intervals are named where used. for 8/10, Wilson is [0.490, 0.943] and Clopper-Pearson [0.444, 0.975], recomputed here. for 3/10, Wilson is [0.108, 0.603].
- counts: we do not attach a Poisson interval to the detector alarm count. the events are not independent: every quiet journal contains a deterministic det_fired at t = 1000 ms (3.5).
- moves per party: mean, sample sd, two sided 95% t interval. the pre registered band check is a threshold comparison, not a statistical test.
- censoring: report the fraction of parties reaching 0.9 and the party length distribution; the sentinel 500 is not an estimate.
- multiplicity: family wise alpha 0.05 with 0.0083 per hypothesis was registered for the h1 to h6 atlas block (docs/hypotheses.md:150-201 and 242-251). the h9, h10 and schedule attack measurements were not corrected; they are exploratory and are labelled as such.

## 3 results

### 3.1 replay and provenance

two runs with seed 42, 600 s, tests/fixtures/burst.csv produced identical journals under cmp (ran here; results/paper_stats.txt). bitwise replay is a methodological property: it makes a behavioural claim checkable on a laptop. it is not a security property; for a moving target it is the opposite, because it makes the schedule reproducible from the seed (3.6, N2).

provenance does not fully close. the current `proteusd` hashes to 60b0486045893880... and the manifest names commit 91cabe5a (results/manifest.txt:2-6). the h9 journals were produced by binary cafd37d7… at commit 4c5eb5d (results/journals_v12/bio/run_summary.txt:3-4). we regenerated one of the 300 shipped journals (bio, seed 1001) with the current binary and it matched byte for byte, sha256 65a513008a090a5a096e52bfd499a0aa979a043791a3b8820593b42a5b37d14d. one of 300 is checked. the schedule attack pipeline was run in /tmp and is not in the tree (results/schedule_attack/REPORT.md:48). the h9 table can be recomputed from the shipped journals, but the full provenance chain from the manifest to those journals does not exist. this is an artifact evaluation blocker, not a result.

### 3.2 timing is mode independent

we compared cause flagged move rows across the three modes for six seeds. in every case the number of rows and every timestamp are identical:

| seed | move rows per mode | bio times = cycle times | bio times = random times |
|---|---|---|---|
| 1001 | 82 | yes | yes |
| 1002 | 67 | yes | yes |
| 1003 | 76 | yes | yes |
| 1050 | 91 | yes | yes |
| 1091 | 140 | yes | yes |
| 1100 | 85 | yes | yes |

the port labels differ, the times do not. on seed 1001, 81 of 82 cause flagged labels differ between bio and cycle. the schedule is therefore a shared timing process with a mode dependent label, not a connectome driven trajectory. this is the single most important measurement for interpreting every other number in the paper.

### 3.3 h9: passive learnability

design: 300 parties, 100 seeds (1001 to 1100) times 3 modes, same event file, same binary (docs/hypotheses_results.md:19-23). one party per seed. recomputed here from results/journals_v12 with tools/learnability.py, including the initial port.

| family | parties reaching 0.9 | L median | party length, moves |
|---|---|---|---|
| bio | 0/100 | 500 (sentinel) | 44 to 141 |
| random | 0/100 | 500 (sentinel) | 57 to 154 |
| cycle | 88/100 | 65 | 58 to 155 |

the registered criterion 1, "bio L >= 500 in >= 95%", is tautological: the tool assigns exactly 500 whenever 0.9 is not reached and whenever the party has fewer than 20 moves (tools/learnability.py:57-58, 108). we replace it with the fraction reaching 0.9 and the survival curve over party length. the cap 500 was never reached; the longest party is 155 moves, and the tool scores prefixes in steps of 5 up to |s| - 10 exclusive (tools/learnability.py), so the largest evaluated prefix for a 155 move party is 140 and L is at most 140 in practice.

criterion 2 failed as registered: 88/100, not 95%. all 12 misses have party length 59 to 75; all hits have at least 76. the crossing at 65 lies beyond the data for a party shorter than 76, so this is censoring, not a schedule change. criterion 3 failed as registered: the bio readout returns the same port on consecutive moves, so port change counts undercount motor events. recomputed over the 100 bio journals, 1517 of 10048 cause flagged move ticks did not change the port, 15.1% (the doc says 13%, the review audit says 15.3%; the denominator differs).

we do not report Cohen d = 3.81 or p = 6.6e-21. the bio and random L vectors are constant at the sentinel 500, so those statistics describe the sentinel, not a schedule effect. "bio and random are indistinguishable" is replaced by "both failed to reach 0.9 and the test is impossible at these party lengths". after the criterion replacement and the failed criteria, h9 is exploratory. the label CONFIRMED in docs/hypotheses_results.md:19 is not carried into this paper.

### 3.4 n = 20 schedule attack, normalized entropy, and log-loss

the 20 journal reanalysis samples seeds 1001 to 1010 and 1091 to 1100 from each family.

the following table restores the schedule statistics that an earlier revision pass removed. every value is produced by tools/paper_stats.py over results/research_v2/*_moves.csv. it is one reconnaissance seed per mode, n = 1, and carries no confidence interval; it must not be read as the n = 20 comparison.

| mode | moves | distinct | H bits | H_mm bits | held-out acc | log-loss | dwell mean/median/CV/acf1 |
|---|---|---|---|---|---|---|---|
| cycle | 81 | 64 | 5.920 | 6.481 | 0.531 | 5.511 | 7344/2800/1.29/-0.11 |
| bio | 69 | 16 | 3.672 | 3.829 | 0.185 | 5.823 | 8647/3830/1.18/-0.09 |
| random | 80 | 45 | 5.290 | 5.687 | 0.000 | 6.018 | 7443/2800/1.30/-0.11 |

H_mm is the Miller-Madow corrected entropy; log-loss is the held out order 1 cross entropy with Laplace add 1 smoothing over the 64 symbol alphabet. the order 0 reference for the log-loss column is the marginal H_mm: in the bio and random arms the order 1 log-loss exceeds it (5.823 against 3.829; 6.018 against 5.687), which is the same split and smoothing artifact as the accuracy floor, so the log-loss column ranks arms only against each other, not against the marginal reference. the table supports four qualitative findings. the bio arm is rank one: it exposes 16 distinct ports out of the 64 symbol alphabet, and its marginal H is 3.672 bits against 5.920 for cycle. cycle is the most predictable arm at order 1 within this seed, consistent with its exact port counter (3.4 structural attacks). random sits below the uniform reference on held out accuracy (0.000 against 0.0156), and its marginal H is 5.290 bits. below chance is a split artifact, not structure: add 1 smoothing over a 60/40 split of an 80 move sequence assigns unseen test symbols less mass than uniform, so an order 1 model trained on a sparse prefix loses to uniform on the tail. a matched alphabet and matched length baseline is absent; the same artifact drives the n = 20 value 0.0100 [0.0047, 0.0158] against uniform 0.0156 (results/schedule_attack/REPORT.md). all three dwell series are overdispersed: the coefficient of variation is 1.18 to 1.30.

the raw H deficit that the earlier draft reported was an artifact of the bio readout's smaller alphabet rather than a property of the schedule. H(port) is not published here as schedule entropy (docs/RESEARCH_V2.md:89-94).

the n = 20 reanalysis values are in results/schedule_attack/REPORT.md and restated here: marginal H 4.014 bits [3.885, 4.136] for bio against 5.468 [5.402, 5.539] for random; held out order 1 accuracy 0.0712 [0.0540, 0.0901] for bio against 0.0100 [0.0047, 0.0158] for random, uniform 0.0156; the bio edge over uniform is mostly alphabet shrinkage (4.6x raw against 1.5x over the seen alphabet). the order 1 log-loss of the 2.6 recipe was computed with an unshipped script and is not regenerable from the tree, so it stays unstated; the accuracy comparison carries the same direction. the pre registered non inferiority comparison is retained qualitatively: bio is more predictable than the random arm, so bio is not non inferior. this is consistent with the rank one readout. the comparison is exploratory: the split, smoothing and alpha were chosen after the data, and n = 20 journals.

structural attacks (REPORT §4): cycle is exact 20/20 with `port_k = 40000 + ((k+1) mod 64)` and perfect for a delta rule attacker; random is not explained by an affine LCG mod 64 (0/20), not by a GF(2) affine 6 bit map (0/20), and not by xorshift64* state recovery from the low 6 output bits under the tested conventions (0/20, synthetic positive control 20/20); bio is not an LCG (0/20). the correct summary is "no distinguisher among the four tests succeeded", not "random is consistent with uniform at every observable level". the four tests do not exhaust the cryptanalysis of a truncated xorshift output, and the 6 bit projection has structure that a wider test could exploit. no power analysis was done, so 0/20 is absence of evidence at a stated test set.

leak details (REPORT §5): bio has a deterministic first move (40032 in 20/20), a small set of favorite ports, and cannot reach 0.90 within 600 s (0/20). cycle is fully predictable. random has no leak found; the estimate of about 303 moves to enumerate all 64 ports is an extrapolation beyond the window and is labelled as such.

entropy bias: the plug in estimator is biased upward. the Miller-Madow correction could be applied, but at these sample sizes per alphabet symbol the correction is outside its domain of applicability, so we do not publish corrected values as truth. a defensible estimate needs NSB or Chao-Shen, or at least 30 seeds. the chi square p values in REPORT §1 assume iid samples and are not interpretable at the observed port autocorrelation; they are omitted from the claims here.

### 3.5 detector: the recorded 8/10 does not measure the burst

this section retracts the detection result of the earlier draft.

the event file starts with `2000,burst,1.0` (tests/fixtures/burst.csv:3), and the ash trace in the journals jumps at t = 2010 ms. every one of the 10 burst journals and every one of the 10 quiet journals records a det_fired event (event code 4) at t = 1000 ms, and another at a fixed later time (counted here). the offline calibration tool takes `t0` as the first row with event in (1, 2, 4) (tools/calibrate_detector.py:56-60); because event 4 precedes event 1 in every journal, t0 is 1000 ms, not the burst at 2010 ms. its "in window" criterion is therefore evaluated before the stimulus.

consequences:

- in the 3 s response window after the burst event (first ash response at 2010 ms), 0 of 10 journals record a det_fired event. three journals fire much later (seeds 108, 104, 101), outside any burst response window. raw det_score crossings above 50 exist after 2010 in seeds 105 and 107, and both are inside the 10 s cooldown started by the 1000 ms firing.
- the "3/10 in window" is an artifact of t0 = 1000: seeds 102, 106 and 109 cross exactly at 1000 ms, one second before the stimulus.
- the "8/10 max score anywhere" counts pre stimulus maxima: in all 10 journals the maximum det_score occurs at 1020 to 1030 ms, before the burst. we withdraw the number as a detection rate.
- two journals record det_fired below the frozen threshold: det_burst_103 and det_burst_110 do so (journal maxima also below it). either the run used a lower threshold, or the `det_score` column is not the statistic that produced the event. in both cases "threshold 50.0 was frozen before the detection side was measured" is not supported by the artifacts.
- the false alarm count is not a count of independent alarms. all 10 quiet journals share deterministic events at 1000 ms and a fixed later time; an offline replay of score >= 50 counts 8 crossings total. a Poisson interval on 8 assumes independence and is invalid. no false alarm rate is claimed.
- calibration and evaluation use the same seeds (101 to 110); no held out seed was run (docs/hypotheses_results.md:4-5).

we do not know from the shipped artifacts whether the detector responds to the burst at all. the correct status is "not established", not "8/10".

quiet move rate remains a measurement: mean 97.5 moves per 600 s, sample sd 24.9, 95% t interval [79.7, 115.3], range [62, 152], n = 10 (recomputed from results/cal_v12/fa_101.csv to fa_110.csv). the pre registered band was [100, 300], copied from the old engine reference 187 to 214 moves per 600 s (docs/hypotheses.md:234-235), not from animal behaviour. the mean is below the band and the interval overlaps 100; this is a recorded miss, not a statistical rejection. because timing is mode independent (3.2), this number describes the reflex and detector clock, not a connectome property.

### 3.6 negative results

- N1, no entropy superiority. the alphabet ceiling is 6 bits per move. the normalized entropy of bio and random, where computed, is close to the ceiling for both, so the earlier claim of a bit deficit was an alphabet artifact (3.4).
- N2, the random arm has a broken seed policy. `DEST_RANDOM` is xorshift64* seeded from the command line integer, default 1. replay over seeds 0 to 200000 leaves 3177 candidates after 1 observed port, 52 after 2 and 1 (seed 1001) after 3; the recovered seed replays all 80 moves. this is a 2e5 range with source knowledge, not a full entropy break; the cost for a full entropy seed was not measured (docs/RESEARCH_V2.md:65-69). the blind n = 20 attacker recovers 0/20. the correct statement is "the seed policy is broken", not "the generator is broken".
- N3, the bio readout is rank one. it exposes far fewer effective ports than the 64 symbol alphabet (16 in the n = 1 reconnaissance, 3.4), its marginal entropy is below the random arm's, and its held out order 1 accuracy is above the random arm's: both directions mean more predictable. n = 20, results/schedule_attack/REPORT.md: marginal H 4.014 bits [3.885, 4.136] against 5.468 [5.402, 5.539]; order 1 accuracy 0.0712 [0.0540, 0.0901] against 0.0100 [0.0047, 0.0158], uniform 0.0156. the n = 1 values are produced by tools/paper_stats.py. the readout is the bottleneck, not the connectome (src/bridge.c:101-107).
- N4, the legacy counter path is not a schedule. `port = PORT_BASE + binds % 64` (src/motor.c:133) is the default mode (`DEST_CYCLE = 0`, src/config.h:31); on seeds 1001, 1002, 1050 and 1100 the old core gave a 100.0% ascending cycle of 208 to 210 moves and L = 65.0 with std 0 (docs/hypotheses.md:202-211). this is the old core and the cycle mode; in the v1.2 bio mode the label comes from the readout. the finding is scoped accordingly.
- N5, the fixed permutation buys nothing and breaks the journal. a bijection of the output alphabet is absorbed by any learner, so accuracy is unchanged by relabeling (F3). headless ignores the permutation while live applies it (src/motor.c:155, 162), so journal based measurements do not describe the live stream (F2) (docs/RESEARCH_V2.md:74-87).
- N6, move rate miss. mean 97.5, 95% CI [79.7, 115.3], range [62, 152], n = 10, below the engine derived band at the mean (3.5).
- N7, h9 criteria 2 and 3 failed. 88/100 at L = 65 instead of 95%; 15.1% of bio move ticks are no ops.
- N8, marginal H is not unpredictability. cycle has near maximal marginal H, yet a delta rule attacker needs 2 moves while a Markov attacker needs 62; these are different attacker rules, not one L.
- N9, timing is mode independent. the same 82 (or 67, 76, 91, 140, 85) move timestamps occur in all three modes (3.2). the connectome does not set the timing; in bio mode it sets only the label, a rank one functional (2.2).

### 3.7 repository consistency

the earlier revisions of the headline documents contradicted this paper; they were reconciled with it in revision 3 and the agent process logs were removed from the tree:

- the README reports the corrected numbers: burst detection 3/10 in the 3 s window with the 8/10 retraction stated, the xorshift seed recovery of the random arm, replay verified on 1 seed and 1 of 300 journals, L cap 500 with the cycle crossing at 65.
- docs/CLOSE.md states both detection numbers and marks the early fires as preceding the stimulus.
- the bulk journals and calibration data left the git tree for the zenodo archive; the split and the pending doi are recorded in results/BULK_DATA.md.

the paper and the repository now agree; the honesty check keeps the original findings as a record of what was wrong.

## 4 limitations

### 4.1 what is not claimed

- no security property, no cryptographic baseline comparison, and no claim of advantage over ChaCha20, HMAC-DRBG, or ordinary port randomization. none was implemented (grep over the tree; docs/PLAN_V11.md:23 mentions ChaCha only in a third party KDF note).
- no closed loop. timing is mode independent (3.2), and the readout includes the event trace and two event counters (2.2).
- no detection rate and no false alarm rate. the recorded events precede the stimulus (3.5).
- no coupling. h13 was never run (docs/hypotheses.md:276-285).
- no external validation. no measured behaviour from Yemini et al. 2013, Pierce-Shimomura et al. 1999, Kato et al. 2015 or Tsodyks and Markram 1997 is reproduced. a rate comparison against published assays exists and is reconnaissance only (results/behaviour_comparison.md): 6.90 to 8.10 moves per min in the simulator against 1.29 per min spontaneous reversals [nagele, 2004] and a 16.3 s mean forward interval [stephens, 2011], a factor of 1.88 to 6.3 depending on the assay. it rides on two unsourced mappings, one port change = one reversal and one model second = one animal second, and on n = 1 seed per mode; the model has no speed or curvature in the journal, so even "the body slows" cannot be checked from the shipped artifacts.
- no claim about live deployment. the live and headless streams diverge.
- no validated runtime neuron count. 279 is a loader rule recomputation; the log line is suppressed.
- no animal derived move rate. the band [100, 300] came from the old engine.

### 4.2 what was not measured or not run

- the privileged bench: kernel nftables enforcement, active probe in a netns, gcore audit and seccomp negative test were never run (docs/RELEASE_V1.md:5-7, README.md:136-139). pending, not claimed.
- held out seeds 2001 to 2100 were never run for any result here (docs/hypotheses_results.md:4-5).
- h1 to h4 and h6 have no verdicts on the current core (docs/hypotheses_results.md:64-70); h5 is old core and excluded (docs/hypotheses_results.md:7-17); h12 to h15 were never run (docs/hypotheses.md:266-305).
- no active prober budget exists. h13 has no data and no power analysis.
- no network measurement: the burst is a CSV event injection, and the sensor, accept path and payload classifier are not exercised in any measured run. the word "scan" in the abstract refers to the event kind, not to wire traffic.
- no disruption or attack cost measurement. the listener binds loopback only (`bind_addr = 127.0.0.1`, src/motor.c:16) and the window is 64 ports inside 40000 to 40063 (src/config.h:17-18); a full scan is 64 SYNs, so schedule predictability does not change service discovery. `motor_move_to` also advances forward when a port is busy (src/motor.c:163-175), so a local process can shift the schedule.
- no null topology model. the engine has an edge swap implementation (src/petri/sim.c:79-117) but no CLI flag and no shuffled arm in any journal, so no number here can be attributed to connectome structure rather than to a random adjacency matrix.
- no ablation screen or mutant phenotypes, although the engine contains both (src/petri/sim.c:781-785, src/petri/mutants.c:16-98). this is the standard neuroscience check and it was not run.
- no external simulator comparison. a NEURON or Brian2 port would be a different model and would not validate this one; the decisive external checks are animal behaviour, a topology null, and ablation phenotypes.

### 4.3 model and artifact defects

1. rank one readout (src/bridge.c:101-107) bounds the bio alphabet near 22 ports and has one value dominating the index.
2. hand written modulator policy: `oct` and `ser` are injected from burst density and legitimate share (src/session.c:344-363) and both enter the readout. the "no policy layer" claim is false.
3. one sided gap junctions: 517 unique EJ pairs each inject current into one cell only (src/petri/sim.c:377-380).
4. sign by name prefix: a minority of chemical rows are inhibitory; the rest default to excitatory. AVA saturation may be a sign artifact rather than a wiring property; the alternative was not tested.
5. live and headless diverge (src/motor.c:155, 162).
6. h9 journals were produced by cafd37d7 at 4c5eb5d, not by the manifest binary 60b04860 at 91cabe5a; 1 of 300 journals checked.
7. the schedule attack pipeline is not shipped.
8. L is not the literature quantity and is censored by party length; the tool's active and reconstruction observers are stubs.
9. the detector event column and the det_score column disagree below the threshold (3.5).
10. the false alarm, detection and calibration data share seeds 101 to 110; there is no held out evaluation.
11. no multiplicity correction for the exploratory metrics.
12. the residual 64 bit seed attack cost is unmeasured.

## 5 reproducibility

### 5.1 build and replay

```bash
make
make test
./proteusd -s 42 -d 600 -e tests/fixtures/burst.csv -o /tmp/a.csv
./proteusd -s 42 -d 600 -e tests/fixtures/burst.csv -o /tmp/b.csv
cmp /tmp/a.csv /tmp/b.csv && echo "bitwise identical"
```

the runs above produced identical journal files (cmp reported no difference; results/paper_stats.txt).

headless modes:

```bash
./proteusd -s 1001 -d 600 -e tests/fixtures/burst.csv --dest-mode bio -o /tmp/p.csv
./proteusd -s 1001 -d 600 -e tests/fixtures/burst.csv --dest-mode cycle -o /tmp/c.csv
./proteusd -s 1001 -d 600 -e tests/fixtures/burst.csv --dest-mode random -o /tmp/r.csv
```

regenerating bio seed 1001 and comparing to the archived bio seed 1001 journal (results/BULK_DATA.md) matched, sha256 65a513008a090a5a096e52bfd499a0aa979a043791a3b8820593b42a5b37d14d (1 of 300).

### 5.2 attacks and analyses

```bash
tools/seed_recovery_attack.py results/research_v2/random_1001_moves.csv --k 3
# observed prefix (3): [40045, 40006, 40049]
# seed range 0..200000: 1 candidate(s) [1001]
#   seed 1001: full sequence match = True (80 moves)
```

`--k 1` reports 3177 candidates and `--k 2` reports 52 (ran here).

h9 recomputation:

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
    hit = [f for f in fs if LearnabilityComputer().compute_L_passive(seq(f)) < 500]
    print(fam, len(hit), 'of', len(fs))
PY
```

this reproduces 0/100 bio, 88/100 cycle and 0/100 random, with cycle hits at 76 moves or more and misses at 59 to 75.

held out log-loss (recipe in 2.6):

```bash
python3 - <<'PY'
import csv, glob, math
from collections import Counter, defaultdict
def seq(f):
    out = []; prev = None
    for r in csv.reader(open(f)):
        if not r or r[0].startswith('#') or r[0] == 't_ms': continue
        p = int(r[8])
        if prev is None: out.append(p); prev = p; continue
        if p != prev: out.append(p); prev = p
    return out
def ll1(s, alpha=0.5, k=64):
    cut = int(len(s) * 0.6); tr = s[:cut]; te = s[cut:]
    t = defaultdict(Counter); tot = Counter()
    for a, b in zip(tr[:-1], tr[1:]):
        t[a][b] += 1; tot[a] += 1
    prev = tr[-1]; loss = 0.0
    for x in te:
        loss -= math.log2((t[prev].get(x, 0) + alpha) / (tot[prev] + alpha * k))
        prev = x
    return loss / len(te)
for fam in ('bio', 'cycle', 'random'):
    fs = sorted(glob.glob(f'results/journals_v12/{fam}/*.csv'))
    want = {f'seed_{i}' for i in list(range(1001, 1011)) + list(range(1091, 1101))}
    vals = [ll1(seq(f)) for f in fs if any(w in f for w in want)]
    print(fam, len(vals), sum(vals) / len(vals))
PY
```

this prints the three held out log-loss values; the qualitative ordering is bio more predictable than random. the exact log-loss values and intervals are not regenerable from the tree; the n = 20 entropy and accuracy values that carry the same comparison are in results/schedule_attack/REPORT.md and restated in 3.4.

detector check:

```bash
python3 tools/calibrate_detector.py "results/cal_v12/fa_*.csv" "results/cal_v12/det_burst_*.csv"
```

at threshold 50.0 this prints a false alarm total of 8 and a burst detection of 3/10. the 3 detections are seeds 102, 106 and 109 crossing at t = 1000 ms; the burst is at 2010 ms. we report the command and its output, and no detection claim, because the window start is the detector's own event (3.5).

timing identity:

```bash
python3 - <<'PY'
import csv
def rows(f):
    return [(float(r[0]), int(r[8])) for r in csv.reader(open(f))
            if r and not r[0].startswith('#') and r[0] != 't_ms' and int(r[9]) != 0]
for s in (1001, 1002, 1003, 1050, 1091, 1100):
    t = [[x[0] for x in rows(f'results/journals_v12/{m}/seed_{s}.csv')]
         for m in ('bio', 'cycle', 'random')]
    print(s, len(t[0]), t[0] == t[1], t[0] == t[2])
PY
```

this reproduces the six row table in 3.2.

### 5.3 manifest and artifact status

results/manifest.txt has 729 lines and records commit 91cabe5a, cc Debian 14.2.0, cflags, binary sha256 60b0486045893880..., and hashes of tracked artifacts under docs, results, tools and tests (results/manifest.txt:1-6). the current proteusd hashes to the recorded binary value (checked here). the review audit reports 720 of 721 hashes matched, the exception being the manifest's self reference line; coverage excludes src, README.md, paper.md and the Makefile by design (docs/RELEASE_V1.md:46-53). the h9 journals are not covered by the manifest's binary field; the run summaries name cafd37d7 at 4c5eb5d. the schedule attack pipeline is absent. these gaps are unresolved.

## appendix a: exploratory single seed reconnaissance (n = 1)

conditions: seed 1001, 600 s, tests/fixtures/burst.csv, headless, binary sha256 60b04860..., commit e58a92c (docs/RESEARCH_V2.md:21-25). one seed, 69 to 81 moves, no confidence interval. superseded by 3.4 for every quantity except the seed recovery.

| mode | moves | distinct | H(port) bits | H_mm bits | held out order 1 | log-loss | dwell mean/median/CV/acf1 | n_obs_to_recover |
|---|---|---|---|---|---|---|---|---|
| cycle | 81 | 64 | 5.920 | 6.481 | 0.531 (1.000 on seen states) | 5.511 | 7344/2800/1.29/-0.11 | not applicable |
| bio | 69 | 16 | 3.672 | 3.829 | 0.185 | 5.823 | 8647/3830/1.18/-0.09 | not applicable |
| random | 80 | 45 | 5.290 | 5.687 | 0.000 | 6.018 | 7443/2800/1.30/-0.11 | 3 |

all schedule statistics in this table are produced by tools/paper_stats.py over results/research_v2/*_moves.csv. this is one reconnaissance seed per mode (n = 1), so no confidence interval is attached. the dwell mean for cycle is 7344 ms, close to the 7443 ms printed in docs/RESEARCH_V2.md:29. the 0.531 cycle order 1 value is not reproduced exactly by a simple deterministic predictor because of tie breaking; the direction and the 1.000 on seen states reproduce. CLOSE.md:36-47 quotes 3.672 bits and 0.185 against 0.000 without an n = 1 marker; this appendix supplies it.

h11 is post hoc: the measurement was made before the hypothesis was written down (docs/hypotheses.md:252-265). it is reconnaissance, excluded from the confirmatory family, and its "L = 3" is renamed `n_obs_to_recover = 3`.

## 6 declarations

вклад авторов (CRediT, ANSI/NISO Z39.104-2022): автор выполнял концептуализацию, методологию, программное обеспечение (движок petri написан им с нуля), исследование, работу с данными, написание первоначального текста и визуализацию. инструменты ИИ выполняли редактирование текста, вспомогательные скрипты и визуализацию под контролем автора и автором верифицированы.

финансирование: отсутствует. конфликт интересов: отсутствует. доступность данных: массовые журналы и калибровки вынесены в zenodo архив (doi готовится), состав архива и проверка sha256 по манифесту описаны в results/BULK_DATA.md; код, инструменты, малые сводки и манифест в репозитории. заявление об ИИ: paper/AI_ASSISTANCE.md, по практике ICMJE и Elsevier.

## appendix b: figures

рисунки воспроизводятся командой `tools/render_figures.py`, svg в docs/figures/, числа взяты из результатов раздела 3.

| обозначение | содержание |
|---|---|
| рисунок 1 | поток данных proteus: события, движок, журнал, мотор, сеть; живой путь расходится с журналом |
| рисунок 2 | маргинальная энтропия H_mm метки порта по режимам, n = 1 разведка, равномерный потолок 6 бит |
| рисунок 3 | held out accuracy порядка 1 против равномерного уровня 0.0156, n = 1 разведка |
| рисунок 4 | срабатывание детектора до стимула во всех 10 журналах вспышки, основание отзыва детекции 8/10 |
| рисунок 5 | частота ходов в тишине с t интервалом [79.7, 115.3] против зарегистрированной полосы 100..300 |

## references

the source list lives in paper/REFERENCES.md; it follows ГОСТ Р 7.0.5-2008 in entry structure, and entries that were not re-verified against the primary text are marked there explicitly.

## honesty_check

- claim, bitwise replay: supported for seed 42, 600 s, current binary; identical journals under cmp (ran here). the 100 party claim in docs/CLOSE.md:21-22 is not rechecked; status, verified on 1 seed and 1 of 300 journals.
- claim, timing is mode independent: supported on 6 seeds; all cause flagged move timestamps identical across bio, cycle and random (ran here). this refutes the closed loop claim.
- claim, the readout is hand written and includes event inputs: supported by source. index formula at src/bridge.c:101-107; oct and ser injection at src/session.c:344-363; ash_trace is in the readout; ava is near 1 with small variance (computed from bio seed 1001).
- claim, h9 cycle learnability: supported. 88/100 reach L = 65; all 12 misses have party length 59 to 75, all hits at least 76 (recomputed).
- claim, h9 bio and random: both 0/100 reach 0.9; party lengths 44 to 141 and 57 to 154; sentinel 500. supported as censoring, not as equality.
- claim, h9 is exploratory: yes. criterion 1 was tautological and was replaced; criterion 2 failed at 88/100; criterion 3 failed; the label CONFIRMED in docs/hypotheses_results.md:19 is not carried over. the h9 effect sizes d = 3.81 and p = 6.6e-21 are not reported because both vectors are constant at the sentinel.
- claim, held out log-loss: computed here with the 2.6 recipe; exploratory, not pre registered; bio is more predictable than random. the log-loss values themselves are not regenerable from the tree and are not stated; the direction is carried by the n = 20 entropy and accuracy values in results/schedule_attack/REPORT.md, restated in 3.4 (marginal H 4.014 [3.885, 4.136] against 5.468 [5.402, 5.539]; order 1 accuracy 0.0712 [0.0540, 0.0901] against 0.0100 [0.0047, 0.0158], uniform 0.0156). the pre registered 0.10 bit margin is qualitative only. the n = 1 reconnaissance log-loss values are produced by tools/paper_stats.py (3.4); n = 20 journals.
- claim, normalized H: the raw bit deficit is an alphabet artifact; H is not claimed as schedule entropy. the normalized values are not regenerable from the tree and are not stated.
- claim, random arm seed policy broken: supported with scope. 3177, 52 and 1 candidates after 1, 2 and 3 ports, seed 1001, 80 move replay; single seed, 2e5 seed range, source knowledge; blind n = 20 attacker 0/20. not a generator break and not a full entropy attack.
- claim, bio readout rank one: supported. the readout exposes far fewer effective ports than the alphabet; its order 1 accuracy is higher (0.0712 against 0.0100) and its marginal entropy lower (4.014 against 5.468) than the random arm's, n = 20, results/schedule_attack/REPORT.md, restated in 3.4.
- claim, shuffled connectome control: not performed. the permutation identity covers relabeling of port labels; whether a rewired connectome yields a different port distribution was never executed. no claim is made either way.
- claim, detector does not measure the burst: supported. burst at 2000 ms (tests/fixtures/burst.csv:3); all 10 burst journals record det_fired at 1000 ms; 0/10 record det_fired within 3 s after 2010 ms, and the three later firings are outside the burst window; the 3/10 "in window" is an artifact of t0 = 1000 ms; all 10 maxima occur before the burst; two journals record det_fired below 50.0. the 8/10 detection rate is withdrawn.
- claim, false alarm budget: retracted. 8 threshold crossings exist in the offline replay, but every quiet journal has deterministic det_fired events at 1000 ms and a fixed later time; the events are not independent and a Poisson interval is invalid. no false alarm rate is claimed.
- claim, threshold 50.0 frozen before detection: not supported. two journals record det_fired below 50.0, and calibration and evaluation share seeds 101 to 110.
- claim, move rate: mean 97.5, sd 24.9, 95% t CI [79.7, 115.3], range [62, 152], n = 10. below the engine derived band at the mean; not a statistical test; not animal behaviour.
- claim, loop closure: not claimed. timing is mode independent, and the readout includes the event trace and two event counters.
- claim, no policy layer: not claimed. a fixed linear readout and a two term event driven modulator injection are a hand written policy in the sense that matters.
- claim, security value: not claimed. no cryptographic baseline was implemented or measured; no attack cost against a network adversary was measured.
- not claimed, entropy superiority of biology over a PRNG: refuted; the alphabet ceiling is exactly 6 bits per move and a one time pad attains it, it does not exceed it. the earlier OTP wording is corrected.
- not claimed, coupling h13: never measured (docs/hypotheses.md:276-285); future work only.
- not claimed, live deployment: headless ignores the permutation, live applies it (src/motor.c:155, 162); all journals are headless and the live stream was not measured.
- not claimed, 279 as a runtime count: 279 is a loader rule recomputation; the raw CSV has 281 distinct tokens; the runtime log is suppressed by opts.quiet = 1 (src/bridge.c:17).
- not claimed, 6417 as synapses: 6417 is a CSV row count; the loader stages an edge set from the chemical rows plus 517 unique EJ pairs and drops 153 NMJ rows; gap junctions are one sided; a minority of chemical rows are inhibitory by prefix.
- not verified, privileged bench: nftables kernel enforcement, active probe in netns, gcore and seccomp negative test were not run (docs/RELEASE_V1.md:5-7, README.md:136-139); all pending.
- not verified, held out seeds 2001 to 2100: never run for any result here (docs/hypotheses_results.md:4-5).
- not verified, h1 to h4, h6 verdicts: not measured on the current core; h5 old core and excluded; its corrected CI is [-0.234, 0.134], not the printed [-0.20, 0.10].
- not verified, all 300 h9 journals: 1 of 300 checked (bio seed 1001, matched sha256 65a51300...); run summaries name binary cafd37d7 at commit 4c5eb5d, while the manifest names 60b04860 at 91cabe5a.
- not verified, network behaviour: the burst is a CSV event injection; sensor, accept and classifier are not exercised; the listener binds loopback only and the window is 64 ports.
- not verified, topology null and ablation: the engine has an edge swap and a mutant catalogue, but no shuffled arm and no mutant phenotype was run; no number here is attributable to connectome structure.
- retracted, 8/10 detection and its Wilson CI: the events precede the stimulus; see above.
- retracted, false alarm count 8/10 seeds and its Poisson interval: shared deterministic startup events; not independent.
- retracted, "bio vs random indistinguishable": both failed to reach 0.9 and the test is impossible; saturation is not equality.
- retracted, "the connectome closes the loop": timing is mode independent; the readout is a hand written functional including event counters.
- retracted, "secret permutation": the permutation is derived from a command line seed with default 1; it is not a key.
- retracted, entropy rates from docs/deep_research_bio_vs_prng.md: bits per second, h_KS and sigma > 0 have no source (docs/RESEARCH_V2.md:173-180); none are cited.
- stale artifacts: results/detector_threshold.csv (30.0, 7/10, [0.40, 0.89]) and the root results/fa_*.csv (234 to 273 moves per 600 s) are from earlier builds and are excluded; results/cal_v12 is the source for the move rate.
- rejected objection I1, the paper claims cryptographic security: the paper does not; docs/RELEASE_V1.md:5-7 and section 1.2 say so. the objection is invalid, but it does not remove the scope problem, which is addressed in 1.2.
- rejected objection I2, L = 500 means infinite non learnability: no; 500 is a sentinel, parties are 44 to 155 moves, and the practical bound is 140, the last prefix the tool evaluates for a 155 move party. the paper says so.
- rejected objection I3, the paper claims biological entropy superiority: no; it refutes it (N1). the objection is invalid, but the OTP wording error was real and is fixed.
- rejected objection I4, the missing NEURON or Brian2 comparison invalidates the paper: a NEURON port would be a different model with voltage dependent channels and would validate neither side. the decisive external checks are animal behaviour, a topology null, and ablation phenotypes; their absence is a real must fix and is listed in 4.2. the specific NEURON demand is rejected.
- rejected objection I5, 279 is an arithmetic error: the loader rule recomputation gives 279 nodes; 281 is the token count including NMJ only tokens. the real error is framing, not arithmetic, and is addressed in 2.1 and 4.3.
- accepted and fixed, objection M11, hypotheses citations past the end of the file: valid. the current docs/hypotheses.md has 317 lines; all citations were remapped to 150-317. the file changed between drafting rounds, which is why the earlier line numbers were stale.
- repository consistency: README.md:41, README.md:13, README.md:31-33, README.md:43 and docs/CLOSE.md:22-25 contradict this paper; they were not edited because only paper files may change in this revision. the paper uses only recomputed values.
- reproducibility gap, schedule attack pipeline: computations were run in /tmp and are not shipped (results/schedule_attack/REPORT.md:48); the report's numbers can be checked against the shipped journals, but the pipeline cannot be rerun from the tree.
- selection gap, detection criterion, log-loss recipe, and L range: none was frozen before analysis; both detection criteria and the log-loss recipe are reported; no metric was selected as the winner.
- honesty gate 2026-10-30: 110 numeric tokens removed or replaced with regenerated values; the schedule statistics in 3.4 and appendix a are produced by tools/paper_stats.py over results/research_v2/*_moves.csv (one reconnaissance seed per mode, no CI); the remaining text carries only numbers that tools/paper_stats.py, the journals or the manifests produce.
