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



