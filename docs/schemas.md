# schemas

frozen data contracts of proteus. any breaking change bumps the version
in the first line of the file it changes and gets a dated entry here.

## party journal, v2 (2026-10-28)

first line: `# proteus journal v2 seed=<n> events=<n>`, second line is
the column header as a comment. one row per protection tick (10 ms of
simulated time). decimal dot, no scientific notation. same seed plus the
same event schedule gives bitwise identical bytes on any machine with
the build flags from the makefile.

columns:

| column | unit or domain |
|---|---|
| t_ms | simulated time, ms |
| act_ava | ava transmitter release, 0..1 |
| act_avb | avb transmitter release, 0..1 |
| ash_trace | nociceptor envelope, 0..1 |
| ser | serotonin, 0..1, includes the r1 injection |
| oct | octopamine, 0..1, includes the r1 injection |
| dop | dopamine, 0..1 |
| det_score | detector matched filter, sum of ava deviations from the adapted baseline over the dense one second window |
| port | current listening port |
| cause | 0 none, 1 endogenous reversal, 2 detector firing, 3 both |
| event | 0 none, 1 burst, 2 legit, 3 det_armed, 4 det_fired, 5 det_cool |

when several events land in one tick the journal carries the highest
code. v1 (r0, commit e38df49) had 9 columns and no det_score/cause, v1
files stay in the r0 history, tools read v2 only.

## event schedule, v2 (2026-10-28)

first line: `# proteus events v2`, second line: `# t_ms,kind,amp`. rows
sorted by t_ms, no duplicates enforced.

| kind | meaning | amp |
|---|---|---|
| burst | scan burst on closed ports | ash envelope amplitude 0..1 |
| legit | completed legitimate session | unused, write 0.0 |

v1 had kind burst only; legit is the r1 addition.

## detector contract (r1)

state machine armed → fired → cool → armed. the baseline ema seeds from
the first ava reading, a zero start would paint the boot transient as a
giant deviation. the threshold DET_THRESHOLD in src/config.h is frozen
from the false alarm budget on withheld seeds 101..110 (no events,
6000 s model time): the lowest calibration grid threshold with false
alarms within one per ten minutes. frozen value 30.0, detection 7/10,
wilson 95% ci [0.40, 0.89]; false alarms 1.00 per 600 s, 95% ci
[0.38, 1.62]. full record: results/cal_101_110.log,
results/detector_threshold.csv. the baseline tau and the cooling time
are r1 design constants, assumptions are documented in src/config.h.
