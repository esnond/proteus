# standards audit, r2.5

| skill | check | result |
|---|---|---|
| c99-posix-strict | `-std=c99 -pedantic -Wall -Wextra -Werror` on every build | clean, including trust.c |
| clean-c | functions <= 60 lines | **exception**: `session_run` 307 lines (existing before). justified: the tick loop is one ordered state machine (sensor, rate, detector, reflex, ban, move, journal); splitting scatters the ordering invariant. protuesd main 67, cmd_party 87: small overruns, noted |
| clean-c | snake_case, UPPER_CASE macros | ok |
| suckless-core | no abstraction used < 3 times | trust module is used by ipc, sensor, session, proteusctl (>=3); no config files added |
| determinism-gate | bitwise replay | untouched: trust/registration never enters the sim path; party replay green |
| emergent-behavior | no goal-directed constructs in behavior code | ok; token logic lives in ipc/sensor/trust, never in neuron/sim |
| csv-contract | journal header frozen | unchanged; auth events do not enter the journal (status field `trust=` only) |
| git-discipline | atomic conventional commits | one commit per phase item |
| reproducibility-manifest | artifacts covered | results/manifest_phase0.txt; r2.5 manifest in phase 5 |
| headless-first | claims reproducible headless | idle/throughput/determinism are headless; network claims carry the netns script |
| statistical-rigor | mean, CI, n, effect size | secret dependence n=210/210, KS p=0.66, d=0.057; ROC live FP 0/1600 with Wilson bound 18.8/10k stated as insufficient |
| literature-grounding | no new biological numbers | none added |
| night-shift-ops | long runs via scripts with logs | validate/roc/red_team all write logs under results/ or /tmp |

## known exceptions, published

1. `session_run` over the 60-line limit. a refactor is scheduled for r3,
   after the trust layer is exercised on the bench; doing it now would
   move the tick ordering while the suite is the only safety net.
2. the mark fast path does not fire in this environment (counter 0). the
   rule stays (kernel semantics may differ on the bench) and the token
   path is the enforced tier.
3. t6 token-timing and t4 foreign-uid are bench items with fixed methods.
