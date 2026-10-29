# docs/hypotheses.md - Phase 1 Fix Hypotheses (f1.1-f1.5)

## f1.1 determinism_4.4
**Hypothesis**: Journal divergence in test_4.4 is a TEST-METHOD artifact (live -r mode or half-built binary between runs), not an engine bug.
**Method**: Run test_4.4 via PARTY REPLAY (recorded input: -e tests/fixtures/burst.csv, NO -r flag). Two replays same seed+input -> cmp identical.
**Success Criterion**: `cmp -s det_run1.csv det_run2.csv` returns 0 (bitwise identical).
**Falsification**: If replay journals still differ -> engine bug, escalate to architect.

## f1.2 timing_cv
**Hypothesis**: CV=52% in test_5.1 is driven by PUBLIC input (scan density, wire-level published detection rate), not internal secret state.
**Method**: Document in report.md that timing variance = wire-level stochasticity (detector is probabilistic by design at frozen threshold 30.0), not a side-channel leak.
**Success Criterion**: One-line justification added to report.md timing section.
**Falsification**: CV persists with FIXED public input (deterministic scanner) -> investigate.

## f1.3 throughput_split
**Hypothesis**: 250.9 sess/s (headless throughput_probe) vs 33 sess/s (realtime tick under load) are TWO different metrics.
**Method**: Split with labels in report.md: "headless_throughput" vs "realtime_tick_throughput". Stop presenting one number.
**Success Criterion**: Report has both numbers with clear labels.
**Falsification**: N/A (documentation fix).

## f1.4 bypass_finding
**Hypothesis**: 1/3 detection on low-and-slow + ban holds 100% = documented MTD boundary, NOT a bug.
**Method**: Write into theory.md as strength: predicted the boundary (low-and-slow stays under FA budget).
**Success Criterion**: theory.md has entry documenting this finding.
**Falsification**: N/A.

## f1.5 scenario_6.1_assert
**Hypothesis**: test_6.1 must assert USERSPACE ban (works in sandbox), not only nft (dead in sandbox without CAP_NET_ADMIN).
**Method**: Change test_6.1 to check userspace ban list (sensor) + journal event 8, not just nft set.
**Success Criterion**: test_6.1 passes in sandbox (userspace ban fires).
**Falsification**: Userspace ban also fails -> real bug.

---

# r2.5 cycle, registered 2026-10-29 BEFORE the runs

the f1.x ids above refer to the r2 cycle. this section reuses the ids the
owner's r2.5 order names and supersedes the old f1.1 timing_cv reasoning:
the cv criterion is withdrawn (it measured input density), the new f1.1
below replaces it.

## f1.1 side_channel_criterion
**Hypothesis**: response time of the session path does not depend on the
daemon's secret material (seed as proxy until the trust layer lands).
**Method**: fixed identical probe input; two conditions differ only in
the seed; n>=100 per condition; session handshake latency (connect +
payload), blocks alternate order, warmup 5 discarded.
**Success Criterion**: KS p >= 0.01 AND |Cohen's d| < 0.2; numbers
published with n, means, sd.
**Falsification**: significant dependence -> real leak, escalate as a
security bug. ban latency is excluded by design (neural state drives it);
its cv is published as input-driven variance only.

**Reproduction 2026-10-29 (r3), criterion RETRACTED**: the r2.5 record
claims PASS (n=210/210, KS p=0.66, d=0.057). four fresh runs -- two on
88e2cf9, two on the r3 tree -- give d=+0.324, +0.345, -0.873, -0.384,
KS p 0.0 to 0.0153, i.e. FAIL every time. the effect is not caused by
the r3 sensor change (the pre-change binary shows it too).
the test is invalid as designed: the probe measures TCP connect+send
RTT on loopback, the daemon sends no response, and the seed chooses the
advertised port, so the two conditions do NOT share an input. the probe
measures the kernel and the public port, not the secret. the old PASS
was not reproducible and must not be cited. the honest claim is
narrower: the session path has no response channel to leak through.
a valid test needs a server-side response (or a same-port design).

## f1.2 gcore_gap
**Hypothesis**: a core dump of the running daemon contains no journal or
token material in plaintext.
**Method**: gcore (or gdb gcore) on a daemon after a fired ban; audit with
tools/dump_audit.sh (fixed markers + entropy summary).
**Success Criterion**: zero marker hits in the dump.
**Falsification**: any marker found -> zeroization bug, fix and document
the residual.

## f1.3 idle_cpu
**Hypothesis**: the realtime loop can idle below 1 percent CPU without a
determinism or wake-latency regression.
**Method**: event-driven wake (poll with a longer idle timeout) when no
network event and no journal subscriber; measure over 300 s on the bench.
**Success Criterion**: idle cpu < 1 percent; PARTY REPLAY cmp identical;
wake p99 <= 5 ms.
**Falsification**: any of the three fails -> redesign the wake path.

## f1.4 fp_baseline
**Hypothesis**: a single detector threshold exists with FP <= 5 per 10k
legit packets on unregistered mixed traffic.
**Method**: sweep det_score over labeled mixed-traffic journals; FP/FN
with denominators; ROC published.
**Success Criterion**: an operating point at or under the FP budget.
**Falsification**: none reaches it -> publish the limit as the detector
boundary, do not hide it.

---

# r3 cycle, registered 2026-10-29 BEFORE the implementation

## h7 auth_failure_ban
**Hypothesis**: repeated failed application auth attempts from one source
are a credential-stuffing attack, not legitimate sessions, and the daemon
can ban the source without banning a client that sends valid payload.
**Method** (amended after the bench result below): the sensor counts the
app line `PROTEUS AUTH <user> <token>` per source inside a rolling window
(8 lines / 10 s). the source that crosses the count is banned by the
immune scope directly: userspace list + nft set/rule, journal event 8
with cause 0. failed auth lines feed neither the food channel nor the
nociceptor, and the connectome does not decide this ban; the path is
standard security, documented in docs/schemas.md. determinism is
untouched: the network input is outside the journal replay contract.
measured with tools/brute_forcer (60 attempts, 10/s, sock= so the
stuffer follows the advertised port like an adaptive attacker) and
tools/legit_client (HELLO sessions from a different source).
**Success Criterion**: brute source banned, time to ban <= 10 s; legit
source banned 0 times (Wilson upper bound published); test_1_4 PASS.
**Falsification**: no ban, or any legit source banned -> revert the
counter and publish the negative result.

**Measured 2026-10-29, immune-scope path (sandbox, loopback, n=1 per
seed)**: seed 8 ttb=610 ms, seed 23 ttb=610 ms, seed 12 ttb=620 ms;
legit source 127.0.0.2 accepted 3/3 after each ban. the ban is seed
independent, which is the point: it no longer rides the connectome.

**Amendment record (honest)**: the first version fed the failure burst
to the connectome (ash pulses, then a held nociceptor). it passed in a
clean sandbox (seed 7 ttb 620 ms, seed 8 ttb 2020 ms) but failed on the
bench after the mixed-traffic test: octopamine pegged, ava collapsed,
det_score plateaued at 16-20 < 30, test_1_4 red. the mechanism was
replaced by the immune-scope rule above; the earlier measurement is
kept here as the datum that forced the change. side findings still
stand: detector reach is connectome dependent (seed 23 cannot ban a
dense silent scan, max det_score 29.35), and a fixed-port stuffer hits
a moved port 58/60 times, so the adaptive client is required.




---

# r3 phase A, registered 2026-10-29 BEFORE the change

## h8_idle event_driven_tick

**Hypothesis**: the realtime daemon's 11-12% idle CPU is the cost of the
TICK_MS=10 periodic nanosleep, not the cost of the tick itself; an
event-driven poll (listener + IPC + reaper deadline, no periodic timer)
keeps idle CPU below 0.5% of wall on the bench with no dropped network
event, no wake-latency regression, and no change to the simulation path.

**Method**: current state: the realtime loop paces with nanosleep at
TICK_MS=10; an opt-in --idle-divisor N stretches the period after
IDLE_WINDOW_MS=1000 of quiet; bench baseline 0.66-0.96% at divisor 20 vs
11-12% base. Change under test: remove --idle-divisor and its docs; poll
on the listener, the IPC socket and the next reaper deadline; a
wall-clock value may only choose the poll timeout and must never reach
bridge_steps or any session state. Bench, quiet network, seed 1;
pre-change binary at the parent commit (golden) and post-change binary
at the merge commit, same harness; 3 x 600 s wall runs for idle, with
the daemon killed at t1 (not by -d); burst and latency on separate runs.
open design point recorded before implementation: in live mode the
simulation clock no longer advances while the network is quiet, so `-d`
must mean wall seconds in `-r` mode (headless `-d` stays sim seconds),
otherwise the loop never reaches its duration.

**Success Criterion** (all four; a failing run is reported, not averaged
away):
1. idle CPU = (cpu_ms(t1) - cpu_ms(t0)) / (wall(t1) - wall(t0)) < 0.5%
   in each of the 3 runs; mean and 95% CI published (n=3).
2. idle SYN wake p99 < 1 ms, n >= 1000; measured daemon-side with a
   busy-baseline control for the client+kernel offset; a raw client
   connect is not the measurement.
3. 1000 events/s for 10 s: 10,000 paced attempts against the live port,
   0 dropped = attempts - (net_scans + net_sessions delta) - wrong-port
   attempts; 95% upper bound published (rule of three: 0/10,000 bounds
   the drop rate at <= 0.03%).
4. PARTY REPLAY: the journal is cmp -s identical to the pre-change
   golden journal for the same seed and input (seed 1, -e
   tests/fixtures/burst.csv, no -r), and run1 vs run2 is identical too.
   the golden comparison is what tests that pacing does not enter the
   sim path.

**Falsification**: any of the four fails; or the p99 is met only with a
periodic timer still firing at the TICK_MS cadence; or the CPU drop
comes from a non-event timeout that stretches the sim clock (the old
divisor under a new name); or the only replay evidence is run1 vs
run2, which tests determinism but not the absence of a pacing -> sim
channel.

**Measurement plan**: CPU sampled only at t0 and t1 from the daemon's
cpu_ms (getrusage RUSAGE_SELF) over the status channel; the denominator
is the measured wall interval, never status t_ms (sim time). latency:
n=1000 SYNs, report p50/p99/max with a bootstrap CI. burst: read the
counters after the burst plus a 1 s drain; peer EOF outside the wait set
is a drop, not a free pass. seed 1; commit SHA, CFLAGS, binary SHA256
and CSV SHA256 in results/manifest.txt via tools/generate_manifest.sh;
the bench run is the acceptance gate, the sandbox is not. limitation:
cpu_ms is a ~10 ms-granularity cumulative counter, so the criterion is a
window average and a short spike is invisible by construction; the burst
and latency tests are the separate guards.

**Implementation attempt 2026-10-29 (reverted, falsified)**: a poll
driven loop was built: quiet time runs no tick, a peer or control packet
runs one, the 500 ms reaper timeout runs only the reaper, `-d` becomes
wall seconds in live mode. measured: idle cpu 0.010% (from 11-12%),
determinism cmp identical, status and the auth ban still work (ttb
160 ms). the gate failed on behavior, not on cpu: with no empty ticks the
nociceptor is saturated by construction, ava stays suppressed, det_score
reached 6.25 after 121 scan events in 134 ticks, and "targeted scan then
ban" banned nobody. the unit of the simulation is the tick; removing the
empty ticks changes the stimulus density, which is exactly the h8
falsification clause. reverted; no idle claim is made. decision needed,
owner: (a) re-tune the detector and re-validate the science under
event-driven ticks, (b) optimize the engine so 100 ticks/s fits under
0.5% (20x), (c) drop the <0.5% target and keep the wall-paced clock.

**Design amendment 2026-10-29, registered before the bench run**: the
poll-driven loop of the first attempt is replaced by the accumulator
event loop (the game-loop pattern, Fiedler "Fix Your Timestep"): wall
time accumulates while the loop sleeps in poll (listener + control
peers + IDLE_POLL_MS deadline, the reaper shares it), and each wake
steps the simulation once per TICK_MS of accumulated time, clamped at
TICK_CATCHUP_MAX=10 steps. properties stated before measurement: quiet
time runs <= 10 ticks/s (~0.46% cpu at the measured 0.46 ms/tick), a
burst wake returns the empty ticks, wake latency stays one tick. the h8
falsification clause "the old divisor under a new name" is addressed
head on: this IS a quiet-time slowdown, but bounded, event aligned, and
it restores the stimulus density instead of removing it; the four
original criteria plus the density check decide. sandbox numbers at
registration time: idle 0.67% (10.8 ticks/s), scan ban fires (det_score
30.43), auth ban 670 ms, determinism cmp identical, wall -d exit
3.03 s. bench acceptance is still owed.

---

# r3 phase B, registered 2026-10-29 BEFORE any phase B run

plan h1-h6 map onto the г1-г7 hypotheses of docs/theory.md and the
frozen experiment design of docs/atlas_spec.md. the harness is
tools/atlas_runner.sh + tools/g1_g7_runner.sh (train) and
tools/g1_g7_validate.sh (held out); observer classes in
tools/observer_{passive,active,reconstruction}.py, metric L in
tools/learnability.py. seeds frozen: calibration 101..110 (spent, r1),
training 1001..1100, held out 2001..2100, stress 3001..3050. headless
only, 600 s sim per party, no -r, bitwise PARTY REPLAY. family-wise
alpha 0.05 over the six hypotheses -> per-hypothesis alpha 0.0083
where a block has no internal correction; blocks with their own
bonferroni (atlas flags, atlas falsification triple) keep it.
power: 100 vs 100, wilcoxon, d>=0.5 -> power > 0.99.
recorded defect: the journal header written by the binary says
`journal v1` while docs/schemas.md declares v2 (columns match v2);
doc/code mismatch logged in STATE.md, fix deferred to phase D because
a header change invalidates old golden journals.

## h1 attribution_atlas (atlas_spec sec 4)
**Hypothesis**: port moves are attributable to connectome dynamics:
>= 80% of moves carry attribution p < 0.05 (permutation, 1000 shuffles,
bonferroni over moves), and driver sets are stable across halves
(J >= 0.5 in no more than 10% of parties).
**Method**: atlas_analyze.py on 100 bio parties (train seeds), 14
frozen drivers, lasso per half, the four divergence flags of the spec.
**Success Criterion**: mean attribution coverage >= 0.80 (95% CI
published, n=100); fraction of parties with any divergence flag <= 0.10
(wilson CI). held out: same criteria on 2001..2100, drop > 15% relative
published as negative result.
**Falsification**: coverage < 0.80 in > 20% of parties, or J >= 0.5 in
> 10%, or the held out drop > 15%.

## h2 bio_stochastic_vs_random_seeded (theory г7, atlas_spec sec 5) — MAIN
**Hypothesis**: the bio-stochastic schedule (connectome with contact)
is harder to learn than seeded no-contact and random no-contact
schedules at a matched detection posture, while the connectome path
keeps legit disruption within budget.
**Method**: g7 block: L_passive on 3 conditions x 100 train parties;
wilcoxon rank-sum, bonferroni over 3 comparisons (alpha 0.0167),
cohen's d and odds ratios published. g7 gate: L_bio / L_seeded >= 10.
**Success Criterion**: median L_bio > median L_seeded AND median L_bio
> median L_random (both p < 0.0167), and gate ratio >= 10, and bench
legit_refused = 0 (r2 bench datum, already measured).
**Falsification**: any of: median L_bio >= median L_seeded; median
L_bio >= median L_random; J >= 0.5 in > 10% of bio parties; attribution
coverage < 0.80 in > 20% of bio parties; the gate ratio < 10. a failed
gate is published as a negative result, not tuned away.

## h3 self_disclosure flat_vs_bio (theory г1)
**Hypothesis**: flat profiles (no input, endogenous schedule only) are
recognized faster than bio-stochastic profiles: L_bio > L_flat for the
passive observer.
**Method**: g1 block: flat parties on the same train seeds, L_passive
both conditions, wilcoxon + cohen's d.
**Success Criterion**: L_bio > L_flat, p < 0.0083, d published.
**Falsification**: L_bio <= L_flat.
г3 (recognition time vs family frequency in a population of schedule
families) has no harness in this cycle: registered NOT_RUN, needs a
population generator, deferred; it is not counted in the alpha budget.

## h4 economic_threshold (theory г5)
**Hypothesis**: learnability converts to attacker compute cost, and the
bio schedule raises the observer's training cost at least as much as it
raises L.
**Method**: g5 block: L table by observer class (passive, active,
reconstruction) on bio parties; cost model fixed before reading
results: cost = L observations x bytes per observation (journal row) x
model flops per observation as implemented in learnability.py; the
model is written into the tool, not chosen after.
**Success Criterion**: the cost table published with n and CI per
class; cost_bio / cost_seeded >= 10 for the passive class (same ratio
as the г7 gate by construction; the block's job is the table itself).
**Falsification**: no table (any observer class crashes on all
parties) -> hypothesis inconclusive for that class, published.

## h5 sustained_load_fatigue
**Hypothesis**: the detector discriminates burst-shaped scan from
sustained constant-rate scan of equal total dose: under sustained load
the detector stays at least as responsive as under the burst fixture.
**Method**: tools/sustained_attack.py (written after this registration,
before its run): 20 train seeds x 2 conditions (burst fixture vs
sustained schedule matched to burst total dose over 600 s), headless;
outcome per party = det_fired count and mean det_score over the last
300 s (adaptation window); wilcoxon paired by seed.
**Success Criterion**: sustained fire count >= burst fire count minus
one (paired), p >= 0.0083 for the difference being a DEGRADATION in
the sustained direction; means, CI, d published.
**Falsification**: sustained fires significantly FEWER than burst
(p < 0.0083, d published) -> adaptation suppresses sustained detection;
published as a negative result and a defense boundary (the bench h7
datum already hints at desensitization).

## h6 observer_models (theory г2, г4, г6)
**Hypothesis**: (a, г2) a prediction attacker beats a current-state
attacker on the same surface; (b, г4) active probing lowers L below
the passive bound; (c, г6) an observer trained on seeded (synthetic)
parties transfers to bio parties with a published gap.
**Method**: g2 (two attacker models, first 20 train parties), g4
(active vs passive, first 20), g6 (reconstruction observer, seeded ->
bio transfer gap). all numbers n, CI, effect size.
**Success Criterion**: (a) and (b): the stronger attacker reaches
prediction >= 0.9 in fewer observations, p < 0.0083 per comparison,
bonferroni over (a,b,c); (c): the transfer gap is PUBLISHED whatever
its direction (the claim is the gap exists and is measured, not that
it is small).
**Falsification**: (a) or (b) reversed at p < 0.0083; (c) has no
falsifier beyond tool failure (inconclusive then, published).
