# hypotheses_results — r3 phase B verdicts

registered 2026-10-29 in docs/hypotheses.md; this file records outcomes
as measured, negative results included. seeds: training 1001..1100;
held-out 2001..2100 not yet run for anything below.

## h5 sustained_load_fatigue — NOT FALSIFIED

n=20 paired (1001..1020), dose-matched sustained schedule frozen in the
tool before the run. det fires: burst 20 vs sustained 19; per-party mean
diff -0.05, 95% CI [-0.20, 0.10], dz = -0.13, wilcoxon p = 0.564.
mean det_score last 300 s: diff +0.001, dz = 0.01, p = 0.622.
the bench desensitization (mixed traffic left det_score 16-20, h7
amendment) is an ORDER/mixed-input effect, not a sustained-rate effect.
data: results/sustained/sustained_results.json. note: this verdict was
measured on the OLD engine core; re-measured on v1.2 is pending under
h10's new threshold.

## h9 destination_readout — CONFIRMED on the main criterion, 2 criteria
## failed as registered (measurement defects, not schedule behavior)

engine: petri v1.2 core, varshney wiring, vitals in docs/STATE.md.
300 parties, 100 seeds x 3 dest modes, same burst input, same binary.

| family | parties where a passive markov observer reached P>=0.9 | L |
|---|---|---|
| bio (connectome readout) | 0 / 100 | censored at 500 (never reached) |
| random (proteus xorshift) | 0 / 100 | censored at 500 (never reached) |
| cycle (legacy counter) | 88 / 100 | 65 observations (median) |

bio vs cycle censored L: wilcoxon p = 6.6e-21, cohen d = 3.81.
bio vs random: indistinguishable at this instrument's resolution (both
censored; the party gives 35..132 observations, the cap is 500).

criterion 1 (bio L>=500 in >=95%): PASS, 100% — with the censoring
caveat: "never reached 0.9 within the available observations", not
"infinite L".
criterion 2 (cycle L=65 in >=95%): FAIL as registered, 88%. all 100
cycle journals are pure ascending cycles (0 violations); the 12%
misses are parties shorter than 76 moves, where the crossing point
lies beyond the data. measurement censoring, not schedule change.
criterion 3 (n_moves n.s. bio vs cycle): FAIL as registered. root
cause found: the bio readout can return the SAME port on consecutive
moves (13% no-op moves), so the port-change count (70 vs 82) undercounts
motor events; motor events are IDENTICAL per seed (cause tallies 81/67/
75...) — timing invariance holds exactly. a registered h9a amendment
(no-op elimination) is the correct follow-up; these data are reported
as-is, nothing re-run to make a gate green.

## h10 engine_bump_coupling — criteria: 1 FAIL (marginal), 2 PASS

reflex := engine-native reversal episode (speed < -0.02 mm/s sustained
500 ms; constants from petri metrics.c). detector channel := avb; r1
matched filter design kept. frozen threshold 50.0 (lowest grid value
with FA within budget: 8 FAs of 10 allowed on seeds 101..110, 6000 s
model time; 30: 25, 40: 16, 50: 8, 55: 3, 60: 1, 70: 0). detection
8/10 burst journals, wilson 95% CI [0.44, 0.97], published as outcome.
criterion 1 (quiet move rate in [100, 300]/600 s): FAIL at the mean —
98 [63..153]. the failure mode the band guarded (dead coupling, no
moves) is excluded by the data; recorded as-is, owner escalation noted.
criterion 2 (FA budget met): PASS. burst detection measured: 8/10.
data: results/cal_v12/, journals in results/journals_v12/.

## h1-h4, h6 — NOT MEASURED on the current core

the g1-g7 chain (old core) was abandoned mid-run by owner order when
the engine bump landed; its bio+flat falsification data (h2) is retained
under results/atlas_journals and results/hypotheses (g1 partial). h1-h4
and h6 must be re-run on the v1.2 core after h9a; their registrations
stand unchanged. no verdicts are claimed for them.

## honest summary

the central question of the project — "is the biological schedule
harder to learn than trivial schedules, at equal detection" — is
answered YES against the counter cycle (d = 3.81) and EQUAL to uniform
random at this instrument's resolution. biology does not beat a PRNG
at unpredictability; it matches it while adding scan response, audit
and replay. that is the publishable claim.
