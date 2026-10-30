# hypotheses_results: r3 phase b verdicts

registered 2026-10-29 in docs/hypotheses.md; this file records outcomes
as measured, negative results included. seeds: training 1001..1100;
held-out 2001..2100 not yet run for anything below.

## h5 sustained_load_fatigue: not falsified

n=20 paired (1001..1020), dose matched sustained schedule frozen in the
tool before the run. det fires: burst 20 vs sustained 19; per party mean
diff -0.05, 95% CI [-0.234, 0.134], dz = -0.13, wilcoxon p = 0.564 (CI recomputed from sustained results; the earlier [-0.20, 0.10] did not reproduce).
mean det_score last 300 s: diff +0.001, dz = 0.01, p = 0.622.
the bench desensitization (mixed traffic left det_score 16-20, h7
amendment) is an order/mixed-input effect, not a sustained-rate effect.
data: results/sustained/sustained_results.json. note: this verdict was
measured on the old engine core; re-measured on v1.2 is pending under
h10's new threshold.

## h9 destination_readout: confirmed on the main criterion, 2 criteria
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
censored; the party gives 44..155 observations and the cap of 500 was never reached).

criterion 1 (bio L>=500 in >=95%): pass, 100%, with the censoring
caveat: "never reached 0.9 within the available observations", not
"infinite L".
criterion 2 (cycle L=65 in >=95%): fail as registered, 88%. all 100
cycle journals are pure ascending cycles (0 violations); the 12%
misses are parties shorter than 76 moves, where the crossing point
lies beyond the data. measurement censoring, not schedule change.
criterion 3 (n_moves n.s. bio vs cycle): fail as registered. root
cause found: the bio readout can return the same port on consecutive
moves (15.1% no-op moves over 100 bio parties), so the port-change count undercounts
motor events; motor events are identical per seed (cause tallies 81/67/
75...): timing invariance holds exactly. a registered h9a amendment
(no-op elimination) is the correct follow-up; these data are reported
as is, nothing re-run to make a gate green.

## h10 engine_bump_coupling, criteria: 1 fail (marginal), 2 pass

reflex := engine-native reversal episode (speed < -0.02 mm/s sustained
500 ms; constants from petri metrics.c). detector channel := avb; r1
matched filter design kept. frozen threshold 50.0 (lowest grid value
with FA within budget: 8 FAs of 10 allowed on seeds 101..110, 6000 s
model time; 30: 25, 40: 16, 50: 8, 55: 3, 60: 1, 70: 0). detection
8/10 burst journals show any firing (wilson 95% CI [0.490, 0.943]) but only 3/10 fire inside the 3 s window after stimulus onset (wilson [0.108, 0.603]); the five early fires precede the stimulus and are not detections.
criterion 1 (quiet move rate in [100, 300]/600 s): fail at the mean,
mean 97.5, 95% CI [79.7, 115.3], range [62, 152]. the failure mode the band guarded (dead coupling, no
moves) is excluded by the data; recorded as is, owner escalation noted.
criterion 2 (FA budget met): pass, 8 alarms over 6000 s. burst detection measured: 8/10 any firing, 3/10 inside the window.
data: results/cal_v12/, journals in results/journals_v12/.

## h1-h4, h6: not measured on the current core

the g1-g7 chain (old core) was abandoned mid-run by owner order when
the engine bump landed; its bio+flat falsification data (h2) is retained
under results/atlas_journals and results/hypotheses (g1 partial). h1-h4
and h6 must be re-run on the v1.2 core after h9a; their registrations
stand unchanged. no verdicts are claimed for them.

## h11 reconstruction_attacker: confirmed (reconnaissance, post-hoc registration)

DEST_RANDOM is seed recoverable far below the h9 cap. measured on seed
1001, 600 s, tests/fixtures/burst.csv, binary sha256 60b04860…, commit
e58a92c: a replay search over seeds 0..200000 leaves 3177 candidates
after 1 observed port, 52 after 2, and exactly 1 (seed 1001) after 3;
0.6 s. the recovered seed reproduces all 80 moves exactly.
reproducer: tools/seed_recovery_attack.py. sequences:
results/research_v2/*_moves.csv. same run, headless: cycle H(port)
5.920 bits but order-1 deterministic (accuracy 1.000 on states seen in
training, L=65); bio 16 distinct ports, H=3.672 bits, held-out order-1
0.185; random 45 distinct, H=5.290, held-out order-1 0.000: and L=3
against reconstruction. therefore: the h9 result "random L >= 500"
holds only inside the weak attacker class used there; against a
generator reconstruction attacker random is the weakest of the three.
see docs/RESEARCH_V2.md.

## honest summary

the central question of the project, "is the biological schedule
harder to learn than trivial schedules, at equal detection", is
answered yes against the counter cycle (d = 3.81) and equal to uniform
random at this instrument's resolution. biology does not beat a PRNG
at unpredictability; it matches it while adding scan response, audit
and replay. that is the publishable claim.

correction 2026-10-30: the "equal to uniform random" half of the last
paragraph is superseded by h11. at the instrument used here random is
not equal, it is strictly weaker (L = 3 by seed recovery); the "equal"
statement was an artifact of the attacker class. the "biology does not
beat a PRNG at unpredictability" half stands and is now supported by
the bio readout measurement (3.672 bits/move, held-out 0.185).
