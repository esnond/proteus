# proteus: a headless port schedule labelled by a reduced connectome readout, bitwise replay, and a catalogue of refutations

revision 5, draft, artifact evaluation not ready. this file supersedes the earlier drafts and applies the review fixes, the hostile reviewer pass and the methodologist edits, and adds the ГОСТ style annotation, declarations, figures, the evidence and provenance inventories, the discussion, the conclusion and the hypothesis register. the paper is a negative results and methods report, not a security paper. the header keeps the file name FINAL.md because the run's output path is fixed; the status line, not the name, carries the draft status. numeric parity with paper/FINAL_RU.md is checked by tools/check_paper_parity.py.

## реферат (аннотация по ГОСТ 7.32-2017)

объект исследования: детерминированный headless планировщик tcp портов, у которого тик задают рефлекс скорости тела и детектор, а метку в режиме bio задаёт сокращённый readout из шести скаляров модели коннектома *C. elegans* (279 нейронов, вендоренный движок petri v1.2). цель: установить, даёт ли метка из readout измеримое преимущество перед сеяным xorshift64* и случайным, и что из заявленного выдерживает проверку артефакта. методы: парные партии с журналом (100 сидов на режим по 600 s), пассивная метрика обучаемости L, held out порядок 1 (n = 20 журналов на семейство, bootstrap 95%), order 0 baseline, восстановление генератора, калибровка детектора на замороженной процедуре. результаты: каталог опровержений. порядок 1 log-loss даёт 5.851 у bio против 6.025 у random, парный интервал [-0.221, -0.129], эффект 0.174 бита; order 0 baseline даёт 5.058 против 6.364, uniform 6.000; маргинальная H 4.014 [3.885, 4.136] против 5.468 [5.402, 5.539] конфаундирована алфавитом и служит диагностикой. детекция 8/10 отозвана: firmware det_fired 0/10 после 2010 ms, raw пересечения порога 2/10, а записанные 3/10 относятся к t0 = 1000 ms до стимула. seed xorshift64* восстанавливается по 3 портам в диапазоне 200001 кандидатов за under one second при знании кода; четыре структурных теста дают 0/20 каждый, blind seed recovery не реализована. тайминг режимонезависим на 100/100 сидах. реплей бит в бит подтверждён на 1 паре; все 300 журналов совпадают с манифестом, 1 из 300 регенерирован текущим бинарником. readout есть достаточное узкое место, вклад коннектома не проверен. новизна: не заявляется; обзоры метрик mtd существуют [cho et al., 2020; sengupta et al., 2020; cai et al., 2016], но их таблицы метрик не прочитаны, проверены только аннотации, статус UNCERTAIN. область применения: воспроизводимая вычислительная нейронаука, оценка артефактов, отрицательные результаты. полный список источников: paper/REFERENCES.md. заявление об использовании ИИ: paper/AI_ASSISTANCE.md.

ключевые слова: коннектом, *C. elegans*, moving target defense, воспроизводимость, детерминированный реплей, отрицательные результаты, обучаемость расписания, zenodo архив.

keywords: connectome, *C. elegans*, moving target defense, reproducibility, deterministic replay, negative results, schedule learnability.

## abstract

we report a reproducible artifact study of a deterministic headless port schedule whose label is produced by a reduced *C. elegans* readout. all measurements in this paper are headless: the measured path opens no socket (src/motor.c:91-95), and the live path diverges from the journal (src/motor.c:155, 162). the artifact replays bitwise: seed 42, 600 s, two runs, identical journals under cmp (ran here; results/paper_stats.txt). all 300 h9 journals match the manifest hashes, and 1 of 300 was regenerated with the current binary (3.1).

the schedule has three modes. the tick is set by the body speed reflex and the detector, not by the mode. the cause flagged move timestamps are identical across modes on 100 of 100 seeds (checked here with the 5.2 snippet); the six seed table in 3.2 is a sample. the move cause is the engine's own, the avb matched filter and the body speed reflex (src/session.c:364, src/session.c:376), and the mode touches only the destination label. what differs between modes is the label, and in bio mode that label is a hand written rank one functional of six scalars: ash_trace is the event input, ser and oct are event counters, dop is a modulator, and only ava and avb are command cells (src/bridge.c:101-107, src/session.c:344-363). four of the six terms are therefore not independent neural observables. we do not claim that the connectome closes the loop.

the scientific output is a catalogue of refutations. first, the passive learnability metric saturates: 0/100 bio parties and 0/100 random parties reach 0.9 accuracy, while the legacy counter reaches it in 88/100 at L = 65 (cycle party lengths 58 to 155 in the table convention; recomputed here; results/paper_stats.txt). second, the bio label is more predictable than the xorshift64* arm under the order 1 held out log-loss: 5.851 bits per move against 6.025, paired bootstrap 95% interval [-0.221, -0.129], effect 0.174 bits (n = 20 journals per family; computed here; docs/RESEARCH_V3.md). an order 0 held out baseline with the same split and smoothing gives 5.058 against 6.364, and uniform over 64 symbols is 6.000. the marginal entropy comparison 4.014 [3.885, 4.136] against 5.468 [5.402, 5.539] is confounded by alphabet size and is reported as a diagnostic only. the analysis is exploratory: split, smoothing and alpha were chosen after the data. third, the xorshift64* seed is recoverable from 3 observed ports over 200001 candidates when the generator is known (candidates 3177, 52, 1; full 80 move replay; under one second on the review host); this is a seed policy failure, not a broken generator. no blind seed recovery attack was implemented: the 0/20 result is four structural tests, 0/20 each. fourth, the recorded 8/10 does not establish burst detection: every burst journal records a firmware det_fired event at t = 1000 ms, before the burst event at t = 2000 ms (first ash response at 2010 ms; tests/fixtures/burst.csv:3); 0 of 10 journals record firmware det_fired within 3 s after 2010 ms, and the recorded 3/10 in window is an artifact of t0 = 1000 ms. 2 of 10 journals have raw det_score crossings at or above 50 after 2010 ms, but the offline state machine started its cooldown earlier, at 850 ms and 800 ms, so the crossings are masked. fifth, two journals record det_fired below the frozen threshold 50.0, so the threshold provenance is not supported by the artifacts. no security property, no detection rate, and no coupling claim are made. h13 was never run.

## 1 introduction

### 1.1 the question

a deterministic simulator with a seed is a keyed deterministic generator. whether it behaves as a PRNG is an empirical question for a stated attacker (docs/RESEARCH_V2.md:5-6). asking whether such a schedule is "more random" than a seeded generator cannot be won: over an alphabet of 64 symbols the maximum entropy is exactly log2(64) = 6 bits per move, and a one time pad attains that bound rather than exceeding it (Shannon 1949, cited in docs/PLAN_V11.md:12-14). the earlier draft said a one time pad exceeds the bound; that was wrong and is corrected here.

the useful question is narrower. given a deterministic generator and a stated attacker class, how many observations does the attacker need, and what does the artifact actually measure? this paper answers that for the artifact that exists, and reports where the answer is negative.

### 1.2 scope and non-goals

this is not a security paper. it does not propose a defense, does not measure an attack cost against a network adversary, and does not compare against a cryptographic PRF. no CSPRNG arm was implemented and no ChaCha20, HMAC-DRBG, or ephemeral port randomization baseline was measured; `grep` over the tree finds ChaCha only in a planning note about a third party KDF (docs/PLAN_V11.md:23). the seeded arm measured here is xorshift64*, not a generic CSPRNG. a security venue should reject this paper on scope. we state that before a reviewer does. a pure function of a fixed key cannot depend on the attacker's observations; a PRNG with external input can. coupling between schedule and attack was never measured (h13, docs/hypotheses.md:276-285).

the paper is offered as a methods and negative results report for reproducible computational neuroscience and artifact evaluation. its positive content is the replayable artifact, the instrument, and the refutations.

### 1.3 contributions

1. a deterministic headless schedule generator with bitwise replay, a recorded binary, compiler, flags, event file and a manifest (results/manifest.txt).
2. an operational learnability instrument L, with its mismatch to the prose definition documented, and a separate `n_obs_to_recover` construct for seed recovery (tools/learnability.py:53-108; implemented in tools/seed_recovery_attack.py).
3. a held out order 1 log-loss analysis of the shipped journals, computed here, that replaces the saturated L as the comparative metric.
4. a catalogue of refutations with the measurements that establish them: metric saturation, a rank one readout, a seed policy failure, mode independent timing, and a detector result that does not survive inspection.
5. explicit artifact provenance limits, including a binary and commit mismatch between the manifest and the h9 journals.

we do not claim: a closed connectome loop, a security property, a detection rate, a false alarm rate, coupling, external validation, or a validated neuron count at runtime.

the evidence inventory is below. every row names the section that carries the measurement; a count without an interval is marked, and n = 1 rows carry no interval.

| claim | n | value | interval | status | source |
|---|---|---|---|---|---|
| timing is mode independent | 100 seeds | all cause flagged move timestamps identical across modes | exact equality | supported | 3.2, 5.2 |
| marginal H, bio vs random | 20 journals | 4.014 vs 5.468 bits | [3.885, 4.136] vs [5.402, 5.539] | diagnostic, confounded by alphabet | results/schedule_attack/REPORT.md |
| held out order 1 accuracy, bio vs random | 20 journals | 0.0712 vs 0.0100 | [0.0540, 0.0901] vs [0.0047, 0.0158] | exploratory, confounded by smoothing | results/schedule_attack/REPORT.md |
| order 0 held out log-loss | 20 journals | 5.058 bio, 6.306 cycle, 6.364 random | uniform 6.000 | exploratory | 2.6, docs/RESEARCH_V3.md |
| held out order 1 log-loss, 2.6 recipe | 20 journals | 5.851 bio, 4.869 cycle, 6.025 random | bio minus random [-0.221, -0.129] | exploratory | 5.2, docs/RESEARCH_V3.md |
| h9 cycle learns at 0.9 | 100 parties | 88/100 at L = 65 | count | exploratory | 3.3 |
| h9 bio and random | 100 parties each | 0/100 reach 0.9 | sentinel 500 for 212/300 | exploratory, censored | 3.3 |
| seed recovery, xorshift64* arm | 1 seed | 3177, 52, 1 candidates at 1, 2, 3 ports; under one second; 200001 candidates; 80 moves | none, single seed | reconnaissance | 3.6 N2 |
| blind reconstruction tests | 20 journals | four structural tests, 0/20 each | count | supported as absent evidence; no blind seed recovery implemented | 3.4, 3.6 N2 |
| detector after stimulus | 10 journals | firmware det_fired 0/10 in 3 s after 2010 ms; raw crossings 2/10 | count | supported | 3.5 |
| recorded detection | 10 journals | 8/10 any firing, 3/10 in window | Wilson [0.490, 0.943]; [0.108, 0.603] | retracted as detection rate | 3.5 |
| quiet move rate | 10 journals | 97.5 moves per 600 s | [79.7, 115.3] | recorded miss | 3.5 |
| replay | 1 pair | seed 42, 600 s, cmp identical | exact | supported | 3.1 |
| h9 journals against manifest | 300 journals | 300/300 sha256 match | none | supported | 3.1, 5.3 |
| readout rank | n = 1 and n = 20 | 16 distinct ports n = 1; 21.7 [19.8, 23.5] n = 20 | as listed | supported; connectome contribution untested | 3.4, 3.6 N3 |

### 1.4 threat model

the threat model covers only attackers that were actually implemented. rows without a budget were not run and carry no claim.

| class | knowledge | channel | budget | win condition | status |
|---|---|---|---|---|---|
| A0 passive | port sequence only | full journal, one party | party length 44 to 155 moves; L prefix grid step 5 | reach 0.9 accuracy on the next 20 moves | measured, 100 parties per mode |
| A1 blind statistical | port sequence, no generator source | 20 journals per family | train first 60%, test last 40% | held out order 1 and order 2 accuracy, and log-loss | measured, n = 20 |
| A2 reconstruction with source | xorshift64*, seed range 0 to 200000 | observed port prefix only | 1 to 5 observed ports | unique seed and full sequence replay | measured on seed 1001 |
| A3 blind reconstruction | generator family unknown | port prefix | 20 journals | any of four structural tests succeeds | measured, 0/20 each; no blind seed-recovery attack implemented |
| A4 active prober | schedule plus a probe channel | not implemented | undefined | not defined | not run (h13) |
| A5 network observer | wire traffic | pcap | undefined | service discovery or schedule recovery | not run |
| A6 live deployment | journal | live socket | undefined | replay of the live stream | not measurable, live and headless diverge |

the only classes with a full pass over the shipped data are A0 and A1. reconnaissance with source knowledge is A2. A3 is four structural tests with no success, not a blind seed-recovery attack; no such attack was implemented. classes A4 to A6 have no numbers and are listed so that their absence is explicit.

### 1.5 related work

seeded generators with linear or near linear state are reconstructable: xorshift (Marsaglia 2003), xorshift* and xoshiro (Vigna 2014, 2016), both cited by name in docs/RESEARCH_V2.md:151-152 and listed in paper/REFERENCES.md. min entropy estimation of physical noise sources is standardized in NIST SP 800-90B (docs/RESEARCH_V2.md:153); it estimates a source and is not evidence about this schedule.

the closest published problem is ephemeral port randomization. rfc 6056 [larsen, gont, 2011, recommendations for transport-protocol port randomization] names the exact failure mode of the cycle arm: a single global variable that tracks the last port makes the next port easy to predict. the rfc recommends randomization over the range from 1024 upward and reports a collision rate at most 0.3%. the cycle arm is a deterministic counter with no key, so it reproduces the rfc warning on a different substrate; it is not a new finding. [man, zhou, qian, 2021, dns cache poisoning attack: resurrections with side channels] show that network side channels can derandomize an ephemeral port even when it is randomized. the A2 result is the same lesson at the generator level: the xorshift64* seed is recoverable from 3 observed ports over 200001 candidates when the generator is known. this paper does not measure a network side channel; classes A5 and A6 were not run.

port hopping is the closest published design. [lee, thing, 2004, port hopping for resilient networks] vary the port as a function of time and a shared secret. [luo, wang, cai, 2014, effectiveness of port hopping as a moving target defense] introduce an urn model and quantify attacker success in terms of port pool size, number of probes, vulnerable services and hopping frequency. this is the closest construct to L, but L is a sample complexity threshold and not a success probability. [hari, dohi, 2010, sensitivity analysis of random port hopping] include the defender's communication success rate; this paper reports no legitimate traffic success rate, so a cost term is missing. [jafarian, al-shaer, duan, 2012, openflow random host mutation; jafarian, al-shaer, duan, 2015, an effective address mutation approach for disrupting reconnaissance attacks] treat mutation as a reconnaissance countermeasure with unpredictability, mutation rate and adaptation as design variables; in this artifact the mutation rate is fixed by the reflex and detector and is mode independent, and the readout is rank one, so both variables are capped before the connectome enters.

learning an mtd schedule is addressed directly by [moghaddam, yang, thapa, camtepe, kim, 2024, mtd in plain sight: hiding network behavior in moving target defenses], who show that an attacker who knows the mtd window can increase success, that a trigger leaks enough traffic to be detected, and that mimic triggers can deceive the attacker. the timing identity in 3.2 is a window leak of this kind. [mani, haliem, bhargava et al., 2023, machine learning based resilience testing of an address randomization cyber defense] use an LSTM predictor and report varied effectiveness; the order 1 Markov attacker here is weaker. 0.0712 is the measured accuracy of this attacker; a stronger attacker could do better, so it is not an upper bound for the class of learning attackers. [zhuang, deloach, ou, 2014, towards a theory of moving target defense] formalize the entropy hypothesis: greater configuration entropy makes an mtd more effective. N8 is a counterexample on this artifact. [carroll, crouse, fulp, berenhaut, 2014, analysis of network address shuffling as a moving target defense] report a success probability in terms of network size, scan quantity, vulnerable systems and shuffle frequency; L contains the observation count and the attacker class but not the vulnerable population or the scan rate.

three surveys cover mtd metrics: [cho, sharma, alavizadeh et al., 2020, toward proactive, adaptive defense: a survey on moving target defense], [sengupta, chowdhary, sabur et al., 2020, a survey of moving target defenses for network security] and [cai, wang, hu, wang, 2016, moving target defense: state of the art and characteristics]. the earlier claim that no quantitative review exists is corrected. only the abstracts of these three surveys were read; their metric tables were not read, so any claim about their specific metric lists is UNCERTAIN. no published metric identical to L was found in the verified sources, and docs/RESEARCH_V3.md records that as UNCERTAIN; absence in the sources read is not proof of absence, so novelty of L is not claimed.

the neuroscience sources named by the engine are Yemini et al. 2013, Pierce-Shimomura et al. 1999, Kato et al. 2015 and Tsodyks and Markram 1997 (README.md, the citation section); the model does not reproduce a measured behavioural quantity from any of them, so these are provenance, not validation. predictability as a cross domain notion is discussed by Boffetta, Cencini, Falcioni and Vulpiani 2001, and permutation entropy convergence under noise by Amigó, Dale and Tempesta 2020 and Chen and Sato 2024, as recorded in docs/RESEARCH_V2.md:137-145. the repository's own industry scan (docs/STATE.md:19-24) is a repo claim, not a review. the full source records for the new citations are in docs/RESEARCH_V3.md:126-148 and were merged into paper/REFERENCES.md in revision 5.

## 2 methods

### 2.1 engine and wiring

the engine is a vendored reduced model, petri v1.2, commit 42d5062, from the upstream repository recorded in src/petri/VENDOR.md (also docs/hypotheses.md:224, docs/STATE.md:10). it is not a conductance based neuron model: `neuron_act` is a single sigmoid of membrane potential (src/petri/neuron.c:7-10), there are no voltage dependent channels, and the body is 24 segments. the correct label is a graded potential reduced model, not a biophysical simulator. `bridge_init` raises `noise_scale` to 1.0 at runtime (src/bridge.c:22-29), because the wild type loader leaves it at zero; this is a runtime adjustment, not a CSV property, and it is part of the methods.

the wiring sheet src/petri/data/varshney_full.csv has 6417 data rows plus a header and 6418 lines broken down by type S, SP, R, RP, EJ and NMJ (counted here). 6417 is a row count, not a synapse count. the loader drops all 153 NMJ rows, collapses the EJ rows to 517 unique undirected pairs, and stages both sender side (S, SP) and receiver side (R, RP) chemical rows; the staged graph in this build combines the chemical rows with 517 gap pairs (computed from the loader rules at src/petri/connectome.c:360-400). the loader produces 279 nodes after dropping NMJ rows (recomputed here); 281 is the number of distinct tokens in the raw file, the extra two being tokens that occur only in NMJ rows. VC6 is a real neuron that occurs only in the NMJ row at line 6205 and is dropped with the 153 NMJ rows; it is absent from the loaded graph. the runtime `c->n_neurons` is printed by src/petri/sim.c:67-70, but the daemon forces `opts.quiet = 1` in src/bridge.c:17, so the line is suppressed; the 279 value is a loader rule recomputation, not a runtime log.

two model defects matter for interpretation. first, synaptic sign is not in the CSV: the file has four columns and no sign column, so a chemical row is inhibitory only if its presynaptic name has one of seven GABA prefixes (src/petri/connectome.c:22-24, 45-48, 386-397); 203 of 5233 chemical rows, 3.9%, are inhibitory by that rule (counted here). the prefix rule is the effective sign rule, and the remainder are excitatory by default. second, EJ rows are de duplicated to one directed edge per pair, and the synaptic step adds `pn->i_gap += g*(pre->v - pn->v)` only to the postsynaptic cell (src/petri/sim.c:377-380), so the 517 gap contacts are one sided conductors, not shunting electrical synapses. both are model properties, not data properties, and any conclusion that depends on inhibition balance or gap coupling is sensitive to them.

### 2.2 the readout, and the policy question

the bio destination is

    index = floor(64 * mean(act_ava, act_avb, ash_trace, ser, oct, dop))

implemented at src/bridge.c:101-107. this is a hand written rank one linear functional. four of its six terms are not independent connectome observables in the sense the earlier draft implied:

- `ash_trace` is the nociceptor trace, that is, the event input itself.
- `ser` and `oct` are updated in src/session.c:344-363 by `ser += K_SER * share * dt` and `oct += K_OCT * density * dt`, where density is the burst event rate and share is the legitimate traffic fraction. both constants are in src/config.h:118 and 123.
- `dop` is a modulator scalar read from `sim->mod.dop`, not a neuron activation.
- `ava` is saturated: near 1 with small variance over the 60000 rows of the bio seed 1001 journal (journal archive, results/BULK_DATA.md; computed here).

over the 60000 rows of bio seed 1001 the Pearson correlation of the port label with each term is 0.578 for dop, 0.465 for ash_trace, 0.154 for ser, -0.133 for oct, 0.065 for avb and 0.029 for ava. the ranges are ava mean 0.993, sd 0.015, min 0.701; avb mean 0.792, sd 0.335; dop mean 0.030. the label therefore tracks the two non-neural terms dop and ash_trace more strongly than it tracks either command cell. the top port in seed 1001 is 40031 with 8705 of 60000 ticks, share 0.145; the pooled n = 20 top-1 move share is 0.139 [0.124, 0.156], and the top two ports carry 23.4% of moves (results/schedule_attack/REPORT.md; computed here). the index mass is concentrated, and from 279 cells only the two command cells AVA and AVB and the sensory trace reach the port; AVA is nearly constant and AVB is weakly correlated, so the effective neural contribution is small.

we therefore do not claim that there is no policy layer. there is one: a fixed linear readout plus an exogenous two term modulator injection driven by event counters. what is absent is a goal state, a target vector, or a learned policy. the correct statement is "the port label is a fixed hand written functional of engine state and event counters", not "the map is the simulated nervous system".

### 2.3 timing: reflex, detector, and mode independence

move timing is produced by the reflex and the detector, not by the destination mode. `motor_move_to` is called only when `cause != 0` (src/session.c:385-392), and `cause` is set by the detector (`CAUSE_DET`) or the engine reversal reflex (`CAUSE_ENDO`). the mode chooses the argument to `motor_move_to`; it does not choose the tick. this is measured in 3.2: the cause flagged move rows have identical timestamps across bio, cycle and random on 100 of 100 seeds, and the six seed table there is a sample.

consequently the h2 statement that "the connectome decides only the timing" (docs/hypotheses.md:205) is not true on the v1.2 build either. on v1.2 the timing is mode independent, and in bio mode the connectome decides the label. h2 was measured on the old core and on the legacy counter path; we scope it accordingly.

### 2.4 schedule modes and defaults

the default destination mode is `DEST_CYCLE = 0` (src/config.h:31), and `cfg.seed` defaults to 1 (src/proteusd.c:38). the modes are:

- cycle: `port = PORT_BASE + (binds mod 64)` (src/motor.c:133), a counter.
- bio: `motor_move_to(&mo, bridge_dest_index(&br))` (src/session.c:388).
- random: `motor_next_random`, xorshift64* seeded from the command line integer (src/motor.c:18-30, 182).

live mode builds a fixed permutation from the same seed and applies it (src/motor.c:98, 162). the permutation is not a secret: the seed is a command line integer with default 1, and the permutation is derived from it. we drop the word "secret" from the earlier draft. headless mode does not apply the permutation (src/motor.c:155 calls `headless_bind`, which ignores `m->perm`). every measurement in this paper is headless.

### 2.5 metric L and n_obs_to_recover

L is the operational definition in tools/learnability.py:53-108. for a port sequence s, for each prefix length o in {10, 15, ..., min(500, |s| - 10)}, a predictor is fitted on s[:o] using marginal frequencies and first order transitions and scored on s[o:o+20]. L is the first o at which accuracy is at least 0.9; if none, the function returns max_obs = 500. the scored window s[o:o+20] truncates at the end of the party, so prefixes near the tail are scored on fewer than 20 moves. L is an accuracy threshold on a 20 move window, quantized in steps of 5, censored by party length, and not the probability of the next action. the prose definition in docs/CLOSE.md:26-28 (the pre revision 3 README carried the same wording) does not match the code; the code is the definition here. L is a sample complexity threshold at 0.9 accuracy, not an attack success probability; the published success probability metrics are a different slice (see the discussion). the party length convention matters: tools/paper_stats.py counts port changes and excludes the initial port, while the h9 recomputation in 3.3 and 5.2 feeds the tool the sequence that includes the initial port. the table in 3.3 therefore reports cycle 58 to 155 and the L text reports 59 to 156 for the same parties.

the same tool contains stub active and reconstruction observers (`compute_L_active` returns `max(10, passive/2.5)`, `compute_L_reconstruction` returns `min(passive*3, 500)`, tools/learnability.py:110-130). we use only `compute_L_passive`. the tool's aggregation path uses `df['port'].diff() != 0`, which excludes the initial port (tools/learnability.py:183); our L recomputation in 3.3 and 5.2 includes it, while the party length table is produced by tools/paper_stats.py and excludes it. this changes reported counts; we state which convention is used in each table.

for seed recovery we use `n_obs_to_recover`: the number of observed ports after which the candidate seed set has size one. it is implemented as `n_obs_to_recover` in tools/seed_recovery_attack.py and run with `--nobs`; it is a different construct from L and is never placed in an L column.

### 2.6 held out log-loss

the pre registered comparative metric was held out log-loss in bits per move, with L only as a lower bound (docs/RESEARCH_V2.md:196-199). we compute it here from the 20 shipped journals per family (seeds 1001 to 1010 and 1091 to 1100). for each journal, the first 60% of the port sequence is training, the last 40% is test, the model is order 1 with Laplace smoothing alpha = 0.5 over the 64 symbol alphabet, and the score is the mean negative log2 probability of the test symbols. the interval for the bio minus random difference is bootstrap 95% over the 20 journals, 4000 resamples of paired differences. an order 0 held-out reference with the same split and smoothing gives 5.058 for bio, 6.306 for cycle and 6.364 for random; uniform over 64 symbols is 6.000. the H_mm values in 3.4 are plug-in marginal entropies and are not order 0 held-out scores. the estimator and alpha were chosen after seeing the data; this analysis is exploratory and not pre registered. the 60/40 split does not control non-stationarity (the ava transient and the ash burst); accuracy by time quartile was not computed.

### 2.7 statistical procedures

- schedule attack (n = 20 journals per family): bootstrap 95%, 4000 resamples (results/schedule_attack/REPORT.md:3-5).
- proportions: Wilson and Clopper-Pearson intervals are named where used. for 8/10, Wilson is [0.490, 0.943] and Clopper-Pearson [0.444, 0.975], recomputed here. for 3/10, Wilson is [0.108, 0.603].
- counts: we do not attach a Poisson interval to the detector alarm count. the events are not independent: every quiet journal contains a deterministic det_fired at t = 1000 ms (3.5).
- moves per party: mean, sample sd, two sided 95% t interval. the pre registered band check is a threshold comparison, not a statistical test.
- censoring: report the fraction of parties reaching 0.9 and the party length distribution; the sentinel 500 is not an estimate.
- multiplicity: family wise alpha 0.05 with 0.0083 per hypothesis was registered for the h1 to h6 atlas block (docs/hypotheses.md:150-201 and 242-251). the h9, h10 and schedule attack measurements were not corrected; they are exploratory and are labelled as such.

## 3 results

### 3.1 replay and provenance

two runs with seed 42, 600 s, tests/fixtures/burst.csv produced identical journals under cmp (ran here; results/paper_stats.txt). bitwise replay is a methodological property: it makes a behavioural claim checkable on a laptop. it is not a security property; for a moving target it is the opposite, because it makes the schedule reproducible from the seed (3.6, N2).

provenance does not fully close. the current worktree commit is 7b67dfce, the current `proteusd` hashes to 60b0486045893880..., and the binary hash matches the manifest value; the manifest names commit 91cabe5a (results/manifest.txt:2-6). the h9 journals were produced by binary cafd37d7… at commit 4c5eb5d (results/journals_v12/bio/run_summary.txt:3-4). all 300 h9 journal csv files match the manifest sha256 entries (checked here with `sha256sum -c`); one of the 300 (bio, seed 1001) was regenerated with the current binary and matched byte for byte, sha256 65a513008a090a5a096e52bfd499a0aa979a043791a3b8820593b42a5b37d14d. the manifest covers file integrity, not the binary that produced the files, so the binary and commit mismatch remains. the schedule attack pipeline was run in /tmp and is not in the tree (results/schedule_attack/REPORT.md:48). the h9 table can be recomputed from the shipped journals, but the full provenance chain from the manifest binary to those journals does not exist. this is an artifact evaluation blocker, not a result.

the provenance inventory is below. an ellipsis marks a truncated hash that is given in full in the cited file.

| artifact | binary sha256 | commit | dataset |
|---|---|---|---|
| current worktree | 60b0486045893880... | 7b67dfce | this tree |
| manifest | 60b0486045893880... | 91cabe5a | results/manifest.txt |
| h9 journals | cafd37d7... | 4c5eb5d | results/journals_v12 |
| reconnaissance n = 1 | 60b0486045893880... | e58a92c | results/research_v2 |
| replay check | 60b0486045893880... | 7b67dfce | seed 42, 600 s |

### 3.2 timing is mode independent

we compared cause flagged move rows across the three modes for all 100 seeds. on every seed the timestamps are identical; the table reports six seeds as a sample:

| seed | move rows per mode | bio times = cycle times | bio times = random times |
|---|---|---|---|
| 1001 | 82 | yes | yes |
| 1002 | 67 | yes | yes |
| 1003 | 76 | yes | yes |
| 1050 | 91 | yes | yes |
| 1091 | 140 | yes | yes |
| 1100 | 85 | yes | yes |

the port labels differ, the times do not. the same identity holds on all 100 seeds (checked with the 5.2 snippet); the cause flagged row count ranges from 59 to 156 across the 100 seeds, and the table above is a sample of six. on seed 1001, 81 of 82 cause flagged labels differ between bio and cycle. the schedule is therefore a shared timing process with a mode dependent label, not a connectome driven trajectory. this is the single most important measurement for interpreting every other number in the paper.

### 3.3 h9: passive learnability

design: 300 parties, 100 seeds (1001 to 1100) times 3 modes, same event file, same binary (docs/hypotheses_results.md:19-23). one party per seed. recomputed here from results/journals_v12 with tools/learnability.py, including the initial port.

| family | parties reaching 0.9 | L median | party length, moves |
|---|---|---|---|
| bio | 0/100 | 500 (sentinel) | 44 to 141 |
| random | 0/100 | 500 (sentinel) | 57 to 154 |
| cycle | 88/100 | 65 | 58 to 155 |

the party length column uses the tools/paper_stats.py convention, which excludes the initial port; the L recomputation includes it. under the table convention the 12 cycle misses have 58 to 74 moves and the hits have at least 75; under the include-initial convention used for L the misses are 59 to 75 and the hits at least 76. the minimum 58 in the table is therefore a miss, and the 76 boundary belongs to the include-initial sequence.

the registered criterion 1, "bio L >= 500 in >= 95%", is tautological: the tool assigns exactly 500 whenever 0.9 is not reached and whenever the party has fewer than 20 moves (tools/learnability.py:57-58, 108). we replace it with the fraction reaching 0.9 and the survival curve over party length. the sentinel 500 was returned for 212 of 300 parties; the cap was never evaluated, and the longest party is 155 moves in the table convention and 156 in the include-initial convention. the tool scores prefixes in steps of 5 up to |s| - 10 exclusive (tools/learnability.py), so the largest evaluated prefix for the 156 move party is 145; 145 is the practical bound for the longest include-initial party, and 140 applies only to the 155 move table convention.

criterion 2 failed as registered: 88/100, not 95%. all 12 misses have 59 to 75 moves and all hits at least 76 in the include-initial convention used for L; in the table convention these are 58 to 74 and at least 75. the crossing at 65 lies beyond the data for a party shorter than 76, so this is censoring, not a schedule change. criterion 3 failed as registered: the bio readout returns the same port on consecutive moves, so port change counts undercount motor events. recomputed over the 100 bio journals, 1517 of 10048 cause flagged move ticks did not change the port, 15.1% (docs/hypotheses_results.md:44 records 15.1%).

we do not report Cohen d = 3.81 or p = 6.6e-21. docs/hypotheses_results.md:31 computes them for bio versus cycle, not for bio versus random; cycle is not constant at the sentinel (88/100 at L = 65, 12/100 censored at 500). the refusal reason is censoring by party length and the impossibility of estimating a schedule effect, not constancy of both vectors. "bio and random are indistinguishable" is replaced by "both failed to reach 0.9 and the test is impossible at these party lengths". after the criterion replacement and the failed criteria, h9 is exploratory. the label CONFIRMED in docs/hypotheses_results.md:19 is not carried into this paper.

### 3.4 n = 20 schedule attack, marginal entropy, and log-loss

the 20 journal reanalysis samples seeds 1001 to 1010 and 1091 to 1100 from each family. the sample is two contiguous blocks chosen before the attack, not a random sample; the other 80 seeds are not analysed by the attack pipeline. a sensitivity check over all 100 seeds with the same estimators gives marginal H mean and sd 3.963 and 0.416 for bio, 5.943 and 0.027 for cycle and 5.477 and 0.151 for random, and order 1 accuracy mean and sd 0.0983 and 0.1057, 0.7533 and 0.2867, 0.0164 and 0.0192 (computed here; docs/RESEARCH_V3.md).

the following table restores the schedule statistics that an earlier revision pass removed. every value is produced by tools/paper_stats.py over results/research_v2/*_moves.csv. it is one reconnaissance seed per mode, n = 1, and carries no confidence interval; it must not be read as the n = 20 comparison. the log-loss column here is add 1 smoothing with the `zip(test, test[1:])` scoring loop of tools/paper_stats.py, n = 1; the n = 20 log-loss in 3.4 and 5.2 is alpha 0.5 over all test symbols, 2.6 recipe, n = 20. the two log-loss columns are not comparable.

| mode | moves | distinct | H bits | plug-in H_mm bits | held-out acc | log-loss (add 1, paper_stats.py, n = 1) | dwell mean/median/CV/acf1 |
|---|---|---|---|---|---|---|---|
| cycle | 81 | 64 | 5.920 | 6.481 | 0.531 | 5.511 | 7344/2800/1.29/-0.11 |
| bio | 69 | 16 | 3.672 | 3.829 | 0.185 | 5.823 | 8647/3830/1.18/-0.09 |
| random | 80 | 45 | 5.290 | 5.687 | 0.000 | 6.018 | 7443/2800/1.30/-0.11 |

H_mm is the Miller-Madow corrected plug-in marginal entropy. the plug-in estimator is biased downward, and the Miller-Madow correction adds (k-1)/(2 n ln 2); at these sample sizes the corrected value can exceed the 6 bit alphabet ceiling (6.481 for cycle), so H_mm is a diagnostic of finite-sample bias, not an estimate of schedule entropy. H_mm is not an order 0 held-out log-loss: it uses the full sequence, while log-loss is the held out order 1 cross entropy with Laplace add 1 smoothing over the 64 symbol alphabet on a 60/40 split. an order 0 held-out reference with the same split and smoothing gives 5.058 for bio, 6.306 for cycle and 6.364 for random; uniform is 6.000. the n = 1 log-loss column ranks arms only against each other, not against H_mm. the table supports four qualitative findings. the bio arm is rank one: it exposes 16 distinct ports out of the 64 symbol alphabet, and its marginal H is 3.672 bits against 5.920 for cycle. cycle is the most predictable arm at order 1 within this seed, consistent with its exact port counter (3.4 structural attacks). random sits below the uniform reference on held out accuracy (0.000 against 0.0156), and its marginal H is 5.290 bits. below chance is a split artifact, not structure: add 1 smoothing over a 60/40 split of an 80 move sequence assigns unseen test symbols less mass than uniform, so an order 1 model trained on a sparse prefix loses to uniform on the tail. a matched alphabet and matched length baseline is absent; the same artifact drives the n = 20 value 0.0100 [0.0047, 0.0158] against uniform 0.0156 (results/schedule_attack/REPORT.md). all three dwell series are overdispersed: the coefficient of variation is 1.18 to 1.30.

the raw H deficit that the earlier draft reported was an artifact of the bio readout's smaller alphabet rather than a property of the schedule. H(port) is not published here as schedule entropy (docs/RESEARCH_V2.md:89-94).

the n = 20 reanalysis values are in results/schedule_attack/REPORT.md and restated here: marginal H 4.014 bits [3.885, 4.136] for bio against 5.468 [5.402, 5.539] for random, diagnostic only because H is confounded by alphabet size; held out order 1 accuracy 0.0712 [0.0540, 0.0901] for bio against 0.0100 [0.0047, 0.0158] for random, uniform 0.0156; the bio edge over uniform is mostly alphabet shrinkage (4.6x raw against 1.5x over the seen alphabet). the paired difference is 0.0612 with bootstrap 95% interval [0.0407, 0.0832], and the paired marginal H difference is [-1.519, -1.321]. the order 1 log-loss of the 2.6 recipe is regenerable from the shipped journals with the 5.2 command: 5.851 for bio, 4.869 for cycle and 6.025 for random, with a paired bootstrap 95% interval for the bio minus random difference of [-0.221, -0.129], effect 0.174 bits (4000 resamples; computed here, docs/RESEARCH_V3.md). the order 0 held out baseline with the same split and smoothing is 5.058 for bio and 6.364 for random, uniform 6.000. the original /tmp pipeline is not shipped; these point values and the intervals were recomputed here. the log-loss comparison carries the direction; the accuracy and H comparisons are exploratory and confounded, the first by smoothing and the second by alphabet size. the pre registered non inferiority comparison is retained qualitatively: bio is more predictable than the random arm, so bio is not non inferior. this is consistent with the rank one readout. the comparison is exploratory: the split, smoothing and alpha were chosen after the data, and n = 20 journals.

the n = 20 move counts are 87.2 [77.9, 97.4] for bio, 99.4 [89.5, 110.2] for cycle and 98.9 [89.2, 109.6] for random. port ACF(1) is 0.354 [0.286, 0.413], 0.914 [0.910, 0.918] and -0.044 [-0.089, 0.001]; the interval CV is 1.302, 1.389 and 1.389; Ljung-Box p(16) is 0.264, 0.418 and 0.404, so all p values are above 0.26 and the intervals are not periodic. the distinct port count is 21.7 [19.8, 23.5] for bio and 50.7 [48.4, 53.1] for random, and the top two ports carry 23.4% of bio moves. the first bio move is 40032 in 20/20 journals; it lies in the 60% training prefix, not in the held out 40% tail, so it contributes no held out hit. the order 2 accuracy adds +0.0056 with interval [0.0000, 0.0150] over order 1 in bio; the interval includes zero at its endpoint, so the gain is not distinguishable from zero, and the point estimate is not called a gain. the 303 move estimate for enumerating all 64 ports is a coupon collector extrapolation beyond the 600 s window and is labelled as such.

structural attacks (REPORT §4): cycle is exact 20/20 with `port_k = 40000 + ((k+1) mod 64)` and perfect for a delta rule attacker; random is not explained by an affine LCG mod 64 (0/20), not by a GF(2) affine 6 bit map (0/20), and not by xorshift64* state recovery from the low 6 output bits under the tested conventions (0/20, synthetic positive control 20/20); bio is not an LCG (0/20). the four failing tests are the random affine LCG, the random GF(2) affine map, the random xorshift64* state recovery and the bio LCG check, 0/20 each; cycle is the positive control at 20/20. no blind seed-recovery attack was implemented. the correct summary is "no distinguisher among the four tests succeeded", not "random is consistent with uniform at every observable level". the four tests do not exhaust the cryptanalysis of a truncated xorshift output, and the 6 bit projection has structure that a wider test could exploit. no power analysis was done, so 0/20 is absence of evidence at a stated test set.

leak details (REPORT §5): bio has a deterministic first move (40032 in 20/20), a small set of favorite ports, and cannot reach 0.90 within 600 s (0/20). cycle is fully predictable. random has no leak found; the estimate of about 303 moves to enumerate all 64 ports is an extrapolation beyond the window and is labelled as such.

entropy bias: the plug-in estimator is biased downward, and the Miller-Madow correction adds (k-1)/(2 n ln 2). for cycle at n = 81 and k = 64 the correction is 0.56 bits. at these sample sizes per alphabet symbol the correction is outside its domain of applicability, and the corrected value can exceed the 6 bit alphabet ceiling (6.481 for cycle), so H_mm is a diagnostic of finite-sample bias, not an entropy estimate and not a truth value. a defensible estimate needs NSB or Chao-Shen, or at least 30 seeds. the chi square p values in REPORT §1 assume iid samples and are not interpretable at the observed port autocorrelation; they are omitted from the claims here.

### 3.5 detector: the recorded 8/10 does not establish burst detection

this section retracts the detection result of the earlier draft.

the event file starts with `2000,burst,1.0` (tests/fixtures/burst.csv:3), and the ash trace in the journals jumps at t = 2010 ms. every one of the 10 burst journals and every one of the 10 quiet journals records a det_fired event (event code 4) at t = 1000 ms, and another at a fixed later time (counted here). the offline calibration tool takes `t0` as the first row with event in (1, 2, 4) (tools/calibrate_detector.py:56-60); because event 4 precedes event 1 in every journal, t0 is 1000 ms, not the burst at 2010 ms. its "in window" criterion is therefore evaluated before the stimulus.

consequences:

- in the 3 s response window after the burst event (first ash response at 2010 ms), 0 of 10 journals record a firmware det_fired event. three journals fire much later (seeds 108, 104, 101), outside any burst response window. the first raw det_score crossings above 50 in seeds 105 and 107 occur at 850 ms and 800 ms, before the recorded firmware det_fired event at 1000 ms, so the offline state machine starts its 10 s cooldown there, not at the 1000 ms event. raw det_score crossings at or above 50 after 2010 ms exist in 2 of 10 journals (34 samples in seed 105, 97 in seed 107), but they fall inside that earlier cooldown. the firmware det_fired event at 1000 ms and the offline raw-score state machine are two different objects, and each sentence below names which one it uses.
- the "3/10 in window" is an artifact of t0 = 1000: seeds 102, 106 and 109 cross exactly at 1000 ms, one second before the stimulus.
- the "8/10 max score anywhere" counts pre stimulus maxima: in all 10 journals the maximum det_score occurs at 1020 to 1030 ms, before the burst. we withdraw the number as a detection rate.
- two journals record det_fired below the frozen threshold: det_burst_103 and det_burst_110 do so (journal maxima also below it). either the run used a lower threshold, or the `det_score` column is not the statistic that produced the event. in both cases "threshold 50.0 was frozen before the detection side was measured" is not supported by the artifacts.
- the shipped calibration tool on seeds 101 to 110 records false alarm counts over 6000 s as 30: 25, 40: 16, 45: 12, 50: 8, 60: 1 and 70: 0 at thresholds 30, 40, 45, 50, 60 and 70. the 55: 3 entry in docs/hypotheses_results.md:56 is a docs record and does not reproduce with the shipped tool. these are counts, and no interval is attached.
- the false alarm count is not a count of independent alarms. all 10 quiet journals share deterministic events at 1000 ms and a fixed later time; an offline replay of score >= 50 counts 8 crossings total. a Poisson interval on 8 assumes independence and is invalid. no false alarm rate is claimed.
- calibration and evaluation use the same seeds (101 to 110); no held out seed was run (docs/hypotheses_results.md:4-5).

we do not know from the shipped artifacts whether the detector responds to the burst at all. the correct status is "not established", not "8/10".

quiet move rate remains a measurement: mean 97.5 moves per 600 s, sample sd 24.9, 95% t interval [79.7, 115.3], range [62, 152], n = 10 (recomputed from results/cal_v12/fa_101.csv to fa_110.csv). the pre registered band was [100, 300], copied from the old engine reference 187 to 214 moves per 600 s (docs/hypotheses.md:234-235), not from animal behaviour. the mean is below the band and the interval overlaps 100; this is a recorded miss, not a statistical rejection. because timing is mode independent (3.2), this number describes the reflex and detector clock, not a connectome property.

### 3.6 negative results

- N1, no entropy superiority. the alphabet ceiling is 6 bits per move, and a one time pad attains it rather than exceeding it. no normalized entropy is defined or reported; the raw marginal H of bio is lower than random's (4.014 against 5.468 bits), and the earlier claim of a bit deficit was an alphabet artifact (3.4).
- N2, the xorshift64* arm has a broken seed policy. `DEST_RANDOM` is xorshift64* seeded from the command line integer, default 1. replay over seeds 0 to 200000, 200001 candidates, leaves 3177 candidates after 1 observed port, 52 after 2 and 1 (seed 1001) after 3; the recovered seed replays all 80 moves, and the run takes under one second on the review host. this is a 200001 candidate range with source knowledge, not a full entropy break; the cost for a full entropy seed was not measured (docs/RESEARCH_V2.md:65-69). the 0/20 result is four structural tests, 0/20 each; no blind seed-recovery attack was implemented. the correct statement is "the seed policy is broken", not "the generator is broken".
- N3, the bio readout is rank one. it exposes 16 distinct ports in the n = 1 reconnaissance and 21.7 [19.8, 23.5] in the n = 20 reanalysis, against 50.7 [48.4, 53.1] for random; the observed distinct count is 21.7, the structural bound is 64. the lower marginal H of bio (4.014 against 5.468 bits) is expected from the smaller effective alphabet and is diagnostic, not independent evidence. the evidence is the held out order 1 accuracy 0.0712 measured for this attacker against 0.0100, with the caveat that the raw 4.6x edge is 1.5x over the seen alphabet (3.4). n = 20, results/schedule_attack/REPORT.md; order 1 accuracy 0.0712 [0.0540, 0.0901] against 0.0100 [0.0047, 0.0158], uniform 0.0156, paired interval [0.0407, 0.0832]. the n = 1 values are produced by tools/paper_stats.py. the readout is a sufficient bottleneck; the connectome contribution is untested, because no topology null or ablation was run (4.2).
- N4, the legacy counter path is not a schedule. `port = PORT_BASE + binds % 64` (src/motor.c:133) is the default mode (`DEST_CYCLE = 0`, src/config.h:31); on seeds 1001, 1002, 1050 and 1100 the old core gave a 100.0% ascending cycle of 208 to 210 moves and L = 65.0 with std 0 (docs/hypotheses.md:202-211). this is the old core and the cycle mode; in the v1.2 bio mode the label comes from the readout. the finding is scoped accordingly.
- N5, the fixed permutation buys nothing and breaks the journal. a bijection of the output alphabet is absorbed by a learner that sees only the permuted sequence and has no label prior; this is measured on n = 1, and a secret permutation was not tested (F3). headless ignores the permutation while live applies it (src/motor.c:155, 162), so journal based measurements do not describe the live stream (F2) (docs/RESEARCH_V2.md:74-87).
- N6, move rate miss. mean 97.5, 95% CI [79.7, 115.3], range [62, 152], n = 10, below the engine derived band at the mean (3.5).
- N7, h9 criteria 2 and 3 failed. 88/100 at L = 65 instead of 95%; 15.1% of bio move ticks are no ops.
- N8, marginal H is not unpredictability. cycle has near maximal plug-in marginal H, 5.941 [5.931, 5.952], yet a delta rule attacker needs 2 moves while a Markov attacker needs 62 moves in 14/20 journals and more than 99 in 6/20; these are different attacker rules, not one L.
- N9, timing is mode independent. the same 82 (or 67, 76, 91, 140, 85) move timestamps occur in all three modes (3.2). the connectome does not set the timing; in bio mode it sets only the label, a rank one functional (2.2).

### 3.7 repository consistency

the headline documents were reconciled with this paper in revision 5. README.md, README_EN.md, CITATION.cff and docs/CLOSE.md now state that the connectome does not close the loop, that the tick is set by the reflex and the detector, that 3/10 is an artifact of t0 = 1000 ms, that the 8/10 is retracted and that the Byar interval is not claimed. paper.md is marked superseded by this file, and its historical 7/10 and living connectome claims are labelled as such. docs/hypotheses_results.md now carries the shipped detector grid 45:12 and the 0/10 firmware result. paper/REFERENCES.md now contains the 14 mtd records plus Boffetta 2001 and Vigna 2016. the document set checked for this section is README.md, README_EN.md, CITATION.cff, paper.md, docs/CLOSE.md, docs/hypotheses_results.md, docs/RESEARCH_V2.md, docs/RESEARCH_V3.md, docs/RELEASE_V1.md, docs/STANDARDS.md, results/BULK_DATA.md, results/paper_stats.txt, results/schedule_attack/REPORT.md, results/manifest.txt, paper/REFERENCES.md, tools/check_paper_claims.py, tools/paper_stats.py and tools/seed_recovery_attack.py.

the remaining inconsistencies are explicit:

- results/manifest.txt was not regenerated; its audit is in 5.3 and the mismatch is unresolved.
- the bulk journals and calibration data are intended for a zenodo archive; the doi is pending and the archive is not publicly available, so no artifact reproducibility claim is made until it resolves (results/BULK_DATA.md).
- docs/RELEASE_V1.md and docs/STANDARDS.md were not rewritten; they are cited as provenance and status records, not as current claims.
- the schedule attack pipeline remains unshipped (results/schedule_attack/REPORT.md:48).

this paper therefore claims consistency only for the documents named in the first paragraph, not for the whole tree.

## discussion: relation to prior work

the measurements above are interpretable only against the moving target defense and port randomization literature, and this section states which published construct each result addresses and which it does not. the material is drawn from docs/RESEARCH_V3.md, which records the primary sources checked on 2026-10-30.

the table below maps the negative results to prior work and to the metric each prior work uses.

| negative result | prior work | prior metric | this artifact |
|---|---|---|---|
| N1 entropy ceiling | larsen, gont 2011; zhuang, deloach, ou 2014 | prediction chance; configuration entropy | 6 bit ceiling; a one time pad attains it, not exceeds it; H does not order attacker effort |
| N2 seed policy | man, zhou, qian 2021; larsen, gont 2011 | side channel derandomization; counter prediction | 3 ports, 200001 candidates, under one second, xorshift64* known |
| N3 rank one readout | jafarian, al-shaer, duan 2015 | unpredictability and mutation rate | 21.7 distinct ports; 0.0712 measured order 1 accuracy; connectome contribution untested |
| N4 cycle not a schedule | larsen, gont 2011 | sequential port prediction | default counter; delta rule 2 moves, 20/20 |
| N5 permutation neutral | jafarian, al-shaer, duan 2012 | mutation and adaptation | relabeling absorbed; live and headless diverge |
| N6 move rate | hari, dohi 2010 | communication success rate | 97.5 per 600 s, no legit success rate |
| N7 h9 criteria | luo, wang, cai 2014 | success probability given probes | L censored; 88/100 at L = 65; 15.1% no-op |
| N8 H not unpredictability | zhuang, deloach, ou 2014; carroll et al. 2014 | entropy hypothesis; success probability | cycle plug-in H 5.941 [5.931, 5.952]; delta rule 2, Markov 62 in 14/20 |
| N9 timing leak | moghaddam et al. 2024 | mtd window detection | identical timestamps across modes on 100/100 seeds |
| detector | moghaddam et al. 2024 | trigger detection | firmware det_fired 1000 ms before burst 2000 ms; raw 2/10 after 2010 ms |
| behaviour | nagele 2004; stephens 2011 | reversal rate; forward interval | 6.90 to 8.10 per min, factor 5.4 to 6.3 for reversal rate, 1.88 to 2.22 for forward interval, n = 1 |

three comparisons need a boundary. first, the closest published construct to L is the urn model of [luo, wang, cai, 2014], which reports the probability of attacker success as a function of port pool size, number of probes, vulnerable services and hopping frequency. L is the first prefix length at which a 0.9 accuracy threshold is crossed; it is a sample complexity threshold, not a probability, and it carries no vulnerable population or scan rate. [carroll, crouse, fulp, berenhaut, 2014] also report a success probability given network size, scan quantity, vulnerable systems and shuffle frequency. this paper measures a different slice. second, [hari, dohi, 2010] include the defender's communication success rate; no legitimate traffic success rate was measured here, so the cost side is missing. third, [mani, haliem, bhargava et al., 2023] use an LSTM predictor; the attacker here is order 1 Markov, so 0.0712 is the measured accuracy of this attacker; a stronger attacker could do better, and the value is not an upper bound for the class of learning attackers.

the strongest theoretical target is the mtd entropy hypothesis of [zhuang, deloach, ou, 2014], which states that greater configuration entropy makes an mtd more effective. N8 is a counterexample on this artifact: cycle has plug-in marginal H 5.941 [5.931, 5.952] bits, yet a delta rule needs 2 moves and a Markov attacker needs 62 moves in 14/20 journals and more than 99 in 6/20. configuration entropy does not order attacker effort. the cycle result also reproduces the failure case named by rfc 6056 [larsen, gont, 2011]: a global counter makes the next port easy to predict. the cycle arm is the default mode (`DEST_CYCLE = 0`), so the artifact ships the failure mode the rfc warns about.

the timing identity is the mtd window leak described by [moghaddam, yang, thapa, camtepe, kim, 2024]: the move instants are identical across modes, so an observer learns when the next move can occur without learning the port. the detector retraction is a second instance: every journal records det_fired at 1000 ms, before the burst at 2000 ms, so the trigger is a fixed observable.

the behaviour comparison is reconnaissance, not validation. the simulator's 6.90 to 8.10 moves per min sit against 1.29 spontaneous reversals per min [nagele, metz, crowder, 2004] and a 16.3 s mean forward interval [stephens, bueno de mesquita, ryu, bialek, 2011]. the ratio is 5.4 to 6.3 for the reversal rate and 1.88 to 2.22 for the 16.3 s forward interval; the earlier text mixed the two into one 1.88 to 6.3 range. it rides on two unsourced mappings, one port change = one reversal and one model second = one animal second, and on n = 1 seed per mode; it is not a validation of the model.

the literature does not contain a metric identical to L in the verified sources; docs/RESEARCH_V3.md records this as UNCERTAIN and does not claim novelty. three surveys cover mtd metrics [cho, sharma, alavizadeh et al., 2020; sengupta, chowdhary, sabur et al., 2020; cai, wang, hu, wang, 2016], so the earlier statement that no quantitative review exists is corrected. only their abstracts were read; their metric tables were not read, so specific metric-list claims are UNCERTAIN. what remains absent is a published metric that uses the first prefix length at a 0.9 accuracy threshold, not a review.

## 4 limitations

### 4.1 what is not claimed

- no security property, no cryptographic baseline comparison, and no claim of advantage over ChaCha20, HMAC-DRBG, or ordinary port randomization. none was implemented (grep over the tree; docs/PLAN_V11.md:23 mentions ChaCha only in a third party KDF note).
- no closed loop. timing is mode independent (3.2), and the readout includes the event trace and two event counters (2.2). the readout is a sufficient bottleneck; the connectome contribution is untested because no topology null or ablation was run.
- no detection rate and no false alarm rate. the recorded events precede the stimulus (3.5).
- no coupling. h13 was never run (docs/hypotheses.md:276-285).
- no external validation. no measured behaviour from Yemini et al. 2013, Pierce-Shimomura et al. 1999, Kato et al. 2015 or Tsodyks and Markram 1997 is reproduced. a rate comparison against published assays exists and is reconnaissance only (results/behaviour_comparison.md): 6.90 to 8.10 moves per min in the simulator against 1.29 per min spontaneous reversals [nagele, 2004] and a 16.3 s mean forward interval [stephens, 2011]; the ratio is 5.4 to 6.3 for the reversal rate and 1.88 to 2.22 for the forward interval. it rides on two unsourced mappings, one port change = one reversal and one model second = one animal second, and on n = 1 seed per mode; the model has no speed or curvature in the journal, so even "the body slows" cannot be checked from the shipped artifacts.
- no claim about live deployment. the live and headless streams diverge.
- no validated runtime neuron count. 279 is a loader rule recomputation; the log line is suppressed.
- no animal derived move rate. the band [100, 300] came from the old engine.
- no legitimate traffic success rate, unlike [hari, dohi, 2010], and no vulnerable population or scan rate, unlike [carroll et al., 2014]. the schedule attack reports move counts and prediction accuracy, not a defender cost term.
- no LSTM attacker. the implemented attacker is order 1 Markov, weaker than [mani et al., 2023]; 0.0712 is the measured accuracy of this attacker, and a stronger attacker could do better.

### 4.2 what was not measured or not run

- the privileged bench: kernel nftables enforcement, active probe in a netns, gcore audit and seccomp negative test were never run (docs/RELEASE_V1.md:5-7, docs/BENCH_PENDING.md). pending, not claimed.
- held out seeds 2001 to 2100 were never run for any result here (docs/hypotheses_results.md:4-5).
- the n = 30 entropy sweep is incomplete: cycle has partial output, while bio and random are empty. no number from results/entropy_n30 is published until the run finishes.
- h1 to h4 and h6 have no verdicts on the current core (docs/hypotheses_results.md:64-70); h5 is old core and excluded (docs/hypotheses_results.md:7-17); h12 to h15 were never run (docs/hypotheses.md:266-305, with the amendment at 306-316).
- no active prober budget exists. h13 has no data and no power analysis.
- no network measurement: the burst is a CSV event injection, and the sensor, accept path and payload classifier are not exercised in any measured run. the word "scan" in the abstract refers to the event kind, not to wire traffic.
- no disruption or attack cost measurement. the listener binds loopback only (`bind_addr = 127.0.0.1`, src/motor.c:16) and the window is 64 ports inside 40000 to 40063 (src/config.h:17-18). the 64 port window caps a blind scan at 64 SYNs; a schedule aware attacker can reduce this to one SYN per move, so predictability changes the cost, not the possibility, of discovery. `motor_move_to` also advances forward when a port is busy (src/motor.c:163-175), so a local process can shift the schedule.
- no null topology model. the engine has an edge swap implementation (src/petri/sim.c:79-117) but no CLI flag and no shuffled arm in any journal, so no number here can be attributed to connectome structure rather than to a random adjacency matrix. the readout is a sufficient bottleneck; the connectome contribution is untested.
- no ablation screen or mutant phenotypes, although the engine contains both (src/petri/sim.c:781-785, src/petri/mutants.c:16-98). this is the standard neuroscience check and it was not run.
- no external simulator comparison. a NEURON or Brian2 port would be a different model and would not validate this one; the decisive external checks are animal behaviour, a topology null, and ablation phenotypes.

### 4.3 model and artifact defects

1. rank one readout (src/bridge.c:101-107): the observed distinct port count is 21.7 [19.8, 23.5], against a structural bound of 64; the index mass is concentrated in the top two ports (23.4% of moves).
2. hand written modulator policy: `oct` and `ser` are injected from burst density and legitimate share (src/session.c:344-363) and both enter the readout. the "no policy layer" claim is false.
3. one sided gap junctions: 517 unique EJ pairs each inject current into one cell only (src/petri/sim.c:377-380).
4. sign by name prefix: 203 of 5233 chemical rows, 3.9%, are inhibitory by the prefix rule; the CSV has no sign column, so the rest default to excitatory. VC6 occurs only in an NMJ row and is dropped, 153 NMJ rows in all. AVA saturation may be a sign artifact rather than a wiring property; the alternative was not tested.
5. live and headless diverge (src/motor.c:155, 162).
6. h9 journals were produced by cafd37d7 at 4c5eb5d, not by the manifest binary 60b04860 at 91cabe5a; all 300 journals match the manifest sha256, and 1 of 300 was regenerated with the current binary.
7. the schedule attack pipeline is not shipped.
8. L is not the literature quantity and is censored by party length; the tool's active and reconstruction observers are stubs.
9. the detector event column and the det_score column disagree below the threshold (3.5).
10. the false alarm, detection and calibration data share seeds 101 to 110; there is no held out evaluation.
11. no multiplicity correction for the exploratory metrics.
12. the residual 64 bit seed attack cost is unmeasured.

### 4.4 research program, proposal only

the following tests are registered in docs/hypotheses.md and docs/PLAN_V11.md but were not run; no number from them is reported here.

- C1 non-inferiority on held out log-loss against a seeded generator at matched alphabet and attacker, TOST margin 0.10 bit, n = 30 parties.
- C2 coupling: KL between next move distributions with and without an active probe, 0.1 bit threshold, bootstrap CI excluding zero, n = 30.
- C3 non-stationarity: held out accuracy drop from the first to the last quartile, 0.05 absolute, n = 30.
- C4 deployment irreproducibility: seed free key, MAC chain and key commitment; the 3 port attack must fail.
- h12 readout bandwidth, h13 active coupling, h14 nonstationarity and h15 permutation null are registered with success and falsification criteria and have no data. h12 requires H(next|hist) of at least 5.0 bits per move and order 1 accuracy at most 0.05 in at least 27 of 30 seeds; h15 requires TOST with margin 0.01. h13 is the property a pure function of a fixed key cannot have; it was never measured.

## 5 reproducibility

### 5.1 build and replay

```bash
make
make test
./proteusd -s 42 -d 600 -e tests/fixtures/burst.csv -o /tmp/a.csv
./proteusd -s 42 -d 600 -e tests/fixtures/burst.csv -o /tmp/b.csv
cmp /tmp/a.csv /tmp/b.csv && echo "bitwise identical"
```

the runs above produced identical journal files (cmp reported no difference; results/paper_stats.txt). `make test` ran here and printed ALL TESTS GREEN; the netns arm skipped because the sandbox has no cap_net_admin, and that skip is reported, not counted as a pass. all 300 h9 journal csv files match the manifest sha256 (5.3).

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

`--k 1` reports 3177 candidates and `--k 2` reports 52 (ran here). the range 0 to 200000 holds 200001 candidates. the `--k 3` run took 0.876 s on the review host, so the paper says under one second; the value is hardware dependent and is not a fixed constant. `tools/seed_recovery_attack.py results/research_v2/random_1001_moves.csv --nobs` returns `n_obs_to_recover = 3, seed 1001` in 3.690 s (ran here).

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

this reproduces 0/100 bio, 88/100 cycle and 0/100 random, with cycle hits at 76 moves or more and misses at 59 to 75 in the include-initial convention; the 3.3 table excludes the initial port, so its cycle range is 58 to 155 and its misses are 58 to 74.

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

this prints the three held out order 1 log-loss values: 5.851 for bio, 4.869 for cycle and 6.025 for random (rounded to three decimals; computed here and recorded in docs/RESEARCH_V3.md). the order is bio more predictable than random. the point values are regenerable from the tree with this command; the original /tmp pipeline is not shipped, and the paired journal bootstrap 95% interval for the bio minus random difference is [-0.221, -0.129], effect 0.174 bits (4000 resamples; computed here, docs/RESEARCH_V3.md). the same command with an order 0 model, marginal frequencies from the training split and alpha 0.5, gives 5.058 for bio, 6.306 for cycle and 6.364 for random; uniform over 64 symbols is 6.000. the n = 20 entropy and accuracy values that carry the same comparison are in results/schedule_attack/REPORT.md and restated in 3.4.

detector check:

```bash
python3 tools/calibrate_detector.py "results/cal_v12/fa_*.csv" "results/cal_v12/det_burst_*.csv"
```

at threshold 50.0 this prints a false alarm total of 8 and a burst detection of 3/10. the 3 detections are seeds 102, 106 and 109 crossing at t = 1000 ms; the burst is at 2010 ms. the same run prints false alarm counts 45:12, 50:8, 60:1 and 70:0; there is no 55 row, so the 55:3 entry in docs/hypotheses_results.md:56 is a docs record and not a shipped result. we report the command and its output, and no detection claim, because the window start is the detector's own event (3.5).

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

this reproduces the six row table in 3.2; run over seeds 1001 to 1100 it reproduces the 100/100 identity (ran here).

### 5.3 manifest and artifact status

results/manifest.txt has 732 lines and 724 sha256 entries and records commit 91cabe5a, cc Debian 14.2.0, cflags and binary sha256 60b0486045893880... (results/manifest.txt:1-6). tools/generate_manifest.sh:33-35 hashes `git ls-files` over results, docs, tools and tests only; src, README.md, paper.md and the Makefile are outside that set. the current proteusd hashes to the recorded binary value (checked here). a `sha256sum -c` run after the revision 5 edits reports 697 OK, 11 mismatched and 16 missing entries. all 300 h9 journal csv files are among the OK entries (300/300 checked here). the mismatches are docs/CLOSE.md, docs/PLAN_V11.md, docs/RESEARCH_V2.md, docs/STATE.md, docs/hypotheses_results.md, results/manifest.txt (the self reference line), results/paper_stats.txt, results/schedule_attack/REPORT.md, tools/check_paper_claims.py, tools/paper_stats.py and tools/seed_recovery_attack.py. the missing files are docs/AGENT_LOG.md, docs/BENCH_HANDOFF.md, docs/NEATO_INVENTORY.md, docs/PLAN_V1.md, docs/ROLLBACK.md, docs/STANDARDS_AUDIT.md, docs/architecture_research.md, docs/atlas_spec.md, docs/decisions.md, docs/deep_research_bio_vs_prng.md, docs/immune_scope.md, docs/r2.5_validation_2026-10-29.md, docs/r2_results.md, docs/r2_validation_2026-10-29.md, docs/roadmap.md and docs/roc_2026-10-29.md. the manifest also does not list results/BULK_DATA.md, results/behaviour_comparison.md, docs/RESEARCH_V3.md, docs/HISTORY.md or docs/STANDARDS.md. the h9 journals match the manifest hashes but were produced by a different binary; the run summaries name cafd37d7 at 4c5eb5d, the manifest names 60b04860 at 91cabe5a. the schedule attack pipeline is absent. these gaps are unresolved; the manifest predates the removal of the agent logs. results/BULK_DATA.md is now synchronized with the 732 and 724 counts and with the 697/11/16 audit.

## conclusion

the artifact yields one negative result and one methods contribution. the negative result: a schedule whose label is read from a reduced connectome readout does not beat the xorshift64* arm on the order 1 held out log-loss (5.851 against 6.025 bits, paired interval [-0.221, -0.129], effect 0.174 bits; order 0 baseline 5.058 against 6.364, uniform 6.000). the marginal H comparison 4.014 against 5.468 is confounded by alphabet size and is diagnostic. the timing is mode independent on 100/100 seeds, the readout is a sufficient bottleneck and the connectome contribution is untested, and the xorshift64* seed policy is recoverable from 3 ports over 200001 candidates. the methods contribution: a bitwise replayable artifact, the L instrument with its mismatch documented, and a catalogue of refutations with traceable numbers. no security property, detection rate, false alarm rate, coupling or external validation is claimed.

## appendix a: exploratory single seed reconnaissance (n = 1)

conditions: seed 1001, 600 s, tests/fixtures/burst.csv, headless, binary sha256 60b04860..., commit e58a92c (docs/RESEARCH_V2.md:21-25). one seed, 69 to 81 moves, no confidence interval. superseded by 3.4 for every quantity except the seed recovery.

| mode | moves | distinct | H(port) bits | plug-in H_mm bits | held out order 1 (paper_stats.py) | log-loss (add 1, paper_stats.py, n = 1) | dwell mean/median/CV/acf1 | n_obs_to_recover |
|---|---|---|---|---|---|---|---|---|
| cycle | 81 | 64 | 5.920 | 6.481 | 0.531 (1.000 on seen states) | 5.511 | 7344/2800/1.29/-0.11 | not applicable |
| bio | 69 | 16 | 3.672 | 3.829 | 0.185 | 5.823 | 8647/3830/1.18/-0.09 | not applicable |
| random | 80 | 45 | 5.290 | 5.687 | 0.000 | 6.018 | 7443/2800/1.30/-0.11 | 3 |

all schedule statistics in this table are produced by tools/paper_stats.py over results/research_v2/*_moves.csv. this is one reconnaissance seed per mode (n = 1), so no confidence interval is attached. the log-loss column uses add 1 smoothing and the `zip(test, test[1:])` scoring loop of tools/paper_stats.py; it is not comparable to the n = 20 alpha 0.5 log-loss in 3.4. the dwell mean for cycle is 7344 ms, recomputed by tools/paper_stats.py; the 7443 ms printed in docs/RESEARCH_V2.md:29 is stale or a typo and does not reproduce under this convention. the 0.531 cycle order 1 value is not reproduced exactly by a simple deterministic predictor because of tie breaking; the direction and the 1.000 on seen states reproduce. CLOSE.md:36-47 quotes 3.672 bits and 0.185 against 0.000 without an n = 1 marker; this appendix supplies it. the n_obs_to_recover value 3 is produced by tools/seed_recovery_attack.py --nobs, not by tools/learnability.py.

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

## appendix c: hypothesis register

statuses are as of the freeze; no new run was made for this paper.

| hypothesis | status | source |
|---|---|---|
| h1 attribution atlas | not measured on the current core | docs/hypotheses_results.md:64-70 |
| h2 connectome decides what | falsified on the old core: the destination was a counter | docs/hypotheses.md:202-211 |
| h3 flat vs bio | not measured on the current core | docs/hypotheses_results.md:64-70 |
| h4 economic threshold | not measured on the current core | docs/hypotheses_results.md:64-70 |
| h5 sustained load fatigue | not falsified on the old core; excluded here | docs/hypotheses_results.md:7-17 |
| h6 observer models | not measured on the current core | docs/hypotheses_results.md:64-70 |
| h7 auth failure ban | supported in the sandbox; privileged bench not run | docs/hypotheses.md:105-121 |
| h8 idle event driven tick | implementation attempt reverted; bench acceptance owed | docs/hypotheses.md:123-148 |
| h9 destination readout | exploratory: criterion 1 tautological, criteria 2 and 3 failed | 3.3 |
| h10 engine bump coupling | partial: threshold frozen, move rate band missed | docs/hypotheses_results.md:50-62 |
| h11 reconstruction attacker | confirmed as post hoc reconnaissance | docs/hypotheses_results.md:72-87 |
| h12 readout bandwidth | not run | docs/hypotheses.md:266-275 |
| h13 active coupling | not run | docs/hypotheses.md:276-285 |
| h14 nonstationarity | not run | docs/hypotheses.md:286-295 |
| h15 permutation null | not run | docs/hypotheses.md:296-305; the amendment 306-316 applies to h9 and h12 to h15 |

## references

the source list lives in paper/REFERENCES.md; it follows ГОСТ Р 7.0.5-2008 in entry structure, and entries that were not re-verified against the primary text are marked there explicitly. the sources added in this revision are recorded in docs/RESEARCH_V3.md:126-148 and are not yet merged into paper/REFERENCES.md.

## honesty_check

- claim, bitwise replay: supported for seed 42, 600 s, current binary; identical journals under cmp (ran here). `make test` ran here and printed ALL TESTS GREEN; the netns arm skipped for lack of cap_net_admin and is reported, not counted as a pass. all 300 h9 journal csv files match the manifest sha256; 1 of 300 was regenerated with the current binary.
- claim, timing is mode independent: supported on 100 of 100 seeds; all cause flagged move timestamps identical across bio, cycle and random (ran here with the 5.2 snippet). this refutes the closed loop claim.
- claim, the readout is hand written and includes event inputs: supported by source. index formula at src/bridge.c:101-107; oct and ser injection at src/session.c:344-363; ash_trace is in the readout; dop is a modulator scalar; ava is near 1 with small variance (computed from bio seed 1001). four of six terms are not independent neural observables; the label correlates 0.578 with dop, 0.465 with ash, 0.065 with avb and 0.029 with ava.
- claim, h9 cycle learnability: supported. 88/100 reach L = 65; in the include-initial convention used for L all 12 misses have 59 to 75 moves and all hits at least 76; in the table convention these are 58 to 74 and at least 75 (recomputed).
- claim, h9 bio and random: both 0/100 reach 0.9; party lengths 44 to 141 and 57 to 154 in the table convention, 45 to 142 and 58 to 155 including the initial port; the sentinel 500 was returned for 212 of 300 parties and the cap was never evaluated. supported as censoring, not as equality.
- claim, h9 is exploratory: yes. criterion 1 was tautological and was replaced; criterion 2 failed at 88/100; criterion 3 failed; the label CONFIRMED in docs/hypotheses_results.md:19 is not carried over. the h9 effect sizes d = 3.81 and p = 6.6e-21 are not reported: docs/hypotheses_results.md:31 compares bio with cycle, cycle is not constant at the sentinel (88/100 at L = 65, 12/100 censored), and the L vectors are censored by party length, so a schedule effect is not estimable.
- claim, held out log-loss: computed here with the 2.6 recipe; exploratory, not pre registered; bio is more predictable than the xorshift64* arm. the n = 20 point values are 5.851 bio, 4.869 cycle and 6.025 random, and the paired bootstrap 95% interval for the bio minus random difference is [-0.221, -0.129], effect 0.174 bits; they are regenerable with the 5.2 command and recorded in docs/RESEARCH_V3.md. the original /tmp pipeline is not shipped. the order 0 held out baseline is 5.058 bio, 6.306 cycle and 6.364 random, uniform 6.000. the marginal H comparison 4.014 [3.885, 4.136] against 5.468 [5.402, 5.539] is confounded by alphabet size and is diagnostic; the order 1 accuracy 0.0712 [0.0540, 0.0901] against 0.0100 [0.0047, 0.0158] is exploratory and confounded by smoothing; the paired accuracy interval is [0.0407, 0.0832] and the paired H interval is [-1.519, -1.321]. the pre registered 0.10 bit margin is qualitative only. the n = 1 reconnaissance log-loss values use add 1 smoothing in tools/paper_stats.py and are not comparable to the n = 20 alpha 0.5 recipe (3.4); n = 20 journals.
- claim, normalized H: no normalized H is defined or reported; the raw bit deficit is an alphabet artifact and H is not claimed as schedule entropy. H_mm is a plug-in diagnostic, not a normalized or order 0 held-out estimate.
- claim, xorshift64* arm seed policy broken: supported with scope. 3177, 52 and 1 candidates after 1, 2 and 3 ports, seed 1001, 80 move replay, 200001 candidate range, under one second on the review host; single seed, source knowledge; the 0/20 result is four structural tests, 0/20 each, and no blind seed-recovery attack was implemented. not a generator break and not a full entropy attack.
- claim, bio readout rank one: supported. the readout exposes 21.7 [19.8, 23.5] distinct ports against 50.7 [48.4, 53.1] for random; its order 1 accuracy is 0.0712 against 0.0100, measured for this attacker. its lower marginal entropy (4.014 against 5.468) is expected from the smaller alphabet and is diagnostic, not independent evidence. the readout is a sufficient bottleneck; the connectome contribution is untested. n = 20, results/schedule_attack/REPORT.md, restated in 3.4.
- claim, shuffled connectome control: not performed. the permutation identity covers relabeling of port labels; whether a rewired connectome yields a different port distribution was never executed. no claim is made either way.
- claim, detector does not establish burst detection: supported. burst at 2000 ms (tests/fixtures/burst.csv:3); all 10 burst journals record firmware det_fired at 1000 ms; 0/10 record firmware det_fired within 3 s after 2010 ms; 2/10 have raw det_score crossings after 2010 ms (34 and 97 samples), masked by an earlier cooldown; the 3/10 "in window" is an artifact of t0 = 1000 ms; all 10 maxima occur before the burst; two journals record det_fired below 50.0. the 8/10 detection rate is withdrawn.
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
- not claimed, 6417 as synapses: 6417 is a CSV row count; the loader stages an edge set from the chemical rows plus 517 unique EJ pairs and drops 153 NMJ rows; gap junctions are one sided; 203 of 5233 chemical rows are inhibitory by prefix, 3.9%, and VC6 is dropped with the NMJ rows.
- not verified, privileged bench: nftables kernel enforcement, active probe in netns, gcore and seccomp negative test were not run (docs/RELEASE_V1.md:5-7, README.md:136-139); all pending.
- not verified, held out seeds 2001 to 2100: never run for any result here (docs/hypotheses_results.md:4-5).
- not verified, h1 to h4, h6 verdicts: not measured on the current core; h5 is old core and excluded, and its printed interval is not used.
- verified, all 300 h9 journals: 300 of 300 csv files match the manifest sha256; 1 of 300 (bio seed 1001) was regenerated with the current binary and matched sha256 65a51300...; run summaries name binary cafd37d7 at commit 4c5eb5d, while the manifest names 60b04860 at 91cabe5a, so file integrity and binary provenance are separate claims.
- not verified, network behaviour: the burst is a CSV event injection; sensor, accept and classifier are not exercised; the listener binds loopback only and the window is 64 ports.
- not verified, topology null and ablation: the engine has an edge swap and a mutant catalogue, but no shuffled arm and no mutant phenotype was run; no number here is attributable to connectome structure. the readout is a sufficient bottleneck; the connectome contribution is untested.
- retracted, 8/10 detection and its Wilson CI: the events precede the stimulus; see above.
- retracted, false alarm count 8/10 seeds and its Poisson interval: shared deterministic startup events; not independent.
- retracted, "bio vs random indistinguishable": both failed to reach 0.9 and the test is impossible; saturation is not equality.
- retracted, "the connectome closes the loop": timing is mode independent; the readout is a hand written functional including event counters.
- retracted, "secret permutation": the permutation is derived from a command line seed with default 1; it is not a key.
- retracted, entropy rates from docs/deep_research_bio_vs_prng.md: bits per second, h_KS and sigma > 0 have no source (docs/RESEARCH_V2.md:173-180); none are cited.
- stale artifacts: results/detector_threshold.csv and the root results/fa_*.csv are from earlier builds and are excluded; results/cal_v12 is the source for the move rate.
- rejected objection I1, the paper claims cryptographic security: the paper does not; docs/RELEASE_V1.md:5-7 and section 1.2 say so. the objection is invalid, but it does not remove the scope problem, which is addressed in 1.2.
- rejected objection I2, L = 500 means infinite non learnability: no; 500 is a sentinel, returned for 212 of 300 parties, parties are 44 to 141 for bio, 57 to 154 for random and 58 to 155 for cycle in the table convention, and the practical bound is 145, the last prefix the tool evaluates for the 156 move include-initial party; 140 applies only to the 155 move table convention. the cap 500 was never evaluated. the paper says so.
- rejected objection I3, the paper claims biological entropy superiority: no; it refutes it (N1). the objection is invalid, but the OTP wording error was real and is fixed.
- rejected objection I4, the missing NEURON or Brian2 comparison invalidates the paper: a NEURON port would be a different model with voltage dependent channels and would validate neither side. the decisive external checks are animal behaviour, a topology null, and ablation phenotypes; their absence is a real must fix and is listed in 4.2. the specific NEURON demand is rejected.
- rejected objection I5, 279 is an arithmetic error: the loader rule recomputation gives 279 nodes; 281 is the token count including NMJ only tokens. the real error is framing, not arithmetic, and is addressed in 2.1 and 4.3.
- accepted and fixed, objection M11, hypotheses citations past the end of the file: valid. the current docs/hypotheses.md has 317 lines; all citations were remapped to 150-317. the file changed between drafting rounds, which is why the earlier line numbers were stale.
- repository consistency: README.md, README_EN.md, CITATION.cff and docs/CLOSE.md were reconciled in revision 5; the legacy preprint paper.md and the superseded paper/DRAFT.md were removed at close: closed loop, valid 3/10, Byar interval, 7/10 and living connectome claims were removed or marked superseded. results/BULK_DATA.md was synchronized to 732 lines and 724 hashes. docs/RELEASE_V1.md and docs/STANDARDS.md were not rewritten and are cited as status records. results/manifest.txt was not regenerated (3.7, 5.3).
- reproducibility gap, schedule attack pipeline: computations were run in /tmp and are not shipped (results/schedule_attack/REPORT.md:48); the report's numbers can be checked against the shipped journals, but the pipeline cannot be rerun from the tree.
- selection gap, detection criterion, log-loss recipe, and L range: none was frozen before analysis; both detection criteria and the log-loss recipe are reported; no metric was selected as the winner.
- honesty gate 2026-10-30: 110 numeric tokens removed or replaced with regenerated values; the schedule statistics in 3.4 and appendix a are produced by tools/paper_stats.py over results/research_v2/*_moves.csv (one reconnaissance seed per mode, no CI); the n = 20 log-loss values are recomputed with the 5.2 command and recorded in docs/RESEARCH_V3.md; the remaining text carries only numbers that tools/paper_stats.py, the journals, the manifests or docs/RESEARCH_V3.md produce.
- revision 4, 2026-10-30, accepted edits: the abstract and 3.7 no longer claim 3/10 is valid or that the repository agrees; 3.3 names the party length convention and the bio vs cycle referent of d = 3.81 and p = 6.6e-21; 3.4 and 2.6 rename H_mm a plug-in diagnostic and report the n = 20 log-loss values; 5.3 reports the current manifest audit; 3.5 gives the first raw crossings at 850 ms and 800 ms; appendix a marks 7443 ms stale; 2.2 gives the top index share 0.139 against 0.0156; N1 drops normalized entropy; N3 drops lower H as independent evidence.
- revision 5, accepted edits: title and abstract now say the tick is set by the reflex and detector and the label by a hand written readout; H is removed from the proof and replaced by the paired order 1 log-loss with an order 0 baseline and uniform 6.000; 0/20 is called four structural tests with no blind seed recovery; the readout is a sufficient bottleneck and the connectome contribution is untested; the top share is 8705 of 60000 in seed 1001 and 0.139 [0.124, 0.156] pooled; the detector wording is "does not establish burst detection" with 0/10 firmware and 2/10 raw crossings; the Markov attacker is 62 moves in 14/20 and more than 99 in 6/20; the n = 1 and n = 20 log-loss columns are labelled and declared incomparable; the practical prefix bound is 145 for the 156 move party; four of six readout terms are non-neural; the wiring has 203 of 5233 inhibitory rows and VC6 is dropped; service discovery is a cost change, not a possibility change; the citations are merged into paper/REFERENCES.md; n_obs_to_recover is implemented in tools/seed_recovery_attack.py; README, README_EN, CITATION.cff, paper.md and docs/CLOSE.md were reconciled; results/BULK_DATA.md is 732 and 724; the seed recovery run is 200001 candidates and under one second; the order 2 gain is not distinguishable from zero; the bijection claim is scoped; 0.0712 is a measured accuracy; the bio alphabet is 21.7 observed against a structural bound of 64; the PRNG definition is keyed generator; the behaviour factors are split; the RU evidence inventory is one inventory with one status vocabulary; the surveys caveat is in the abstract and 1.5; the gate has a number-context mode; the paper is marked draft.
- revision 5, not done: the order 0 baseline is computed but not cross checked by an independent implementation; the n = 30 entropy sweep, the privileged bench, a topology null and an ablation remain unrun; the schedule attack pipeline is unshipped; results/manifest.txt was not regenerated; the zenodo doi is pending and the archive is not available; external sources were not re-verified in this pass.
- rejected objection R1, rename the files to remove "final" and mark them draft: the run fixes the output paths paper/FINAL.md and paper/FINAL_RU.md. the files are marked "draft, artifact evaluation not ready" in the header, and the status line carries the draft status; renaming would break the run's outputs. the substantive part, the draft marking, is applied.
- rejected objection R2, the deterministic first move 40032 leaks held-out accuracy: the first move lies in the 60% training prefix, not in the held out 40% tail, so it contributes no held out hit. it remains a leak for an observer of the full journal, and 3.4 states that. a first move baseline is not needed for the held out score.
- rejected objection R3, h15 runs to line 316: the h15 section ends at line 305; 306-316 is the amendment, which applies to h9 and h12 to h15. the citation gives both ranges.
- not run in this pass, scope of the check: the external bibliographic records were not re-opened against the publisher pages; only the repository records and the existing RESEARCH_V3 verification were used. the review does not count as an external source verification.
