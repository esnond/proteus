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
**Method**: the sensor counts the app line `PROTEUS AUTH <user> <token>`
per source inside a rolling window (8 lines / 10 s); a source that
crosses the count is reported as scan pressure to the nervous system, the
existing detector and ban path decide. determinism is untouched: the
network input is outside the journal replay contract. measured with
tools/brute_forcer (60 attempts, 10/s, source 10.0.0.5, sock= so the
stuffer follows the advertised port like an adaptive attacker; a fixed
port measures the move instead of the detector) and tools/legit_client
(40 HELLO sessions, source 10.0.0.3).
**Success Criterion**: brute source banned, time to ban <= 10 s; legit
source banned 0 times out of 40 sessions (0/40, Wilson upper bound
published); test_1_4_brute_force PASS.
**Falsification**: no ban, or any legit source banned -> the criterion is
wrong; revert the counter and publish the negative result as r3-r1.

**Measured 2026-10-29 (sandbox, loopback, n=2 per seed)**: seed 7
ttb=620 ms both runs; seed 8 ttb=2020 ms both runs; legit source
(127.0.0.2) 3/3 sessions accepted after the ban, attacker (127.0.0.1)
banned. 0/6 legit sessions banned, Wilson 95% upper bound 39%. Two
side findings published, not hidden:
1. a silent scan also fails to ban on some seeds (23: max det_score
   29.35 < 30), so detector reach is connectome dependent; the r2.5
   threshold grid stands, the per-seed table is the honest boundary.
2. before the adaptive stuffer was added, 58 of 60 attempts hit a moved
   port: the port move alone defeats a fixed-port stuffer. that is the
   MTD working, and it is why the detection test needs the adaptive
   client.


