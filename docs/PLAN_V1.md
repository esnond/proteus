# PLAN_V1 — from r3 tail to v1.0, token-constrained

status at plan time: master 38411ca, r2.5 shipped, r3 tail fixes done
(ci, gcore, h7 auth ban, ipc oversized, blue_team agent). idle 0.96% at
`--idle-divisor 20`. no h1-h6 registered or run. no t13-t17. no blue
team report beyond the t7 smoke test. paper.md exists as v1.2-preprint.

## rules of the run

1. sequential agents only, one at a time, logged in docs/AGENT_LOG.md.
2. main agent does every mechanical edit and every test run. agents
   judge, attack, verify, and write their own report; they do not run
   `make test`, `sudo`, or long sweeps.
3. every agent prompt names the files to read (max 6) and asks for a
   bounded answer (<= 40 lines, structured). no repo-wide exploration.
4. noisy work (sweeps, atlas, stress) runs through nohup/tmux with
   output to results/<name>.log; the main context reads tail/grep only.
5. token ledger: after every agent call the main writes the call's token
   cost and the phase total into docs/STATE.md. measured 2026-10-29: one
   small methodologist call cost 577k and a smoke-test call 1.06M (cache
   reads dominate), so the original 300k cap is not reachable with this
   harness. revised rule: at most two agent calls per phase, each <= 600k,
   everything else done inline; a call over 600k is recorded and no
   further calls run that phase.
6. /compact at every phase boundary and at 60% context, whichever first.
7. green gate before every commit: make test + ubsan + determinism cmp.
8. no history rewrite. push is owner-only.

phase budgets are caps, not targets. if a phase crosses its cap the
prioritizer decides what to drop; never drop the top of PRIORITY.

| phase | cap (agent tokens) | owner gate before start |
|---|---|---|
| A idle | 550k | push + chown + bench validate |
| B science | 800k | A green |
| C red/blue | 600k | B verdicts published |
| D release | 450k | C closed |
| E v1.0 | 200k | D green |
| total | 2.6M | |

## precondition (owner, 0 agent tokens)

```
sudo chown -R esnond:esnond ~/proteus
git push origin master
sudo tools/validate_r2.sh
```

phase A does not start until the bench run shows 1.4 and 5.2 green.

## PHASE A — idle < 0.5% (must-have, priority 1)

goal: event-driven tick, no periodic timer, `--idle-divisor` removed.

1. methodologist (80k): register h8_idle before code. criterion: idle cpu
   < 0.5% over 600 s, wake p99 < 1 ms, PARTY REPLAY cmp identical, zero
   dropped network events under a 1000 ev/s burst. falsification: any
   one fails.
2. architect (200k): poll() over listener + ipc socket + a timeout that
   only the reaper and the biological tick need; no timerfd unless poll
   cannot express it; remove the flag and its docs. files:
   src/session.c, src/proteusd.c, src/config.h, README.md.
3. main (0): build, measure 600 s idle, wake probe, determinism, make
   test. numbers into docs/STATE.md.
4. adversary (120k): race on wake vs accept, burst 1000 ev/s event loss,
   idle->burst latency. reads src/session.c, src/sensor.c only.
5. blue_team (120k): new surface check (fd leak, missed wake, reaper
   starvation). same files.
6. memory (30k): STATE rows + manifest.
7. editor (60k): README idle section, remove flag from docs.

exit: all four numbers; adversarial+blue reports clean or residuals
published. commit `feat(r3): event-driven tick`.

## PHASE B — science h1-h6 (priority 2: h2, then h1)

1. methodologist (120k): register h1-h6 with success/falsification, held
   out seeds (dev 1-50, holdout 51-100), power analysis, multiple
   comparison correction. docs/hypotheses.md only.
2. architect (250k): missing harness only: tools/bio_vs_random.py (h2),
   tools/economic_cost.py (h4), tools/sustained_attack.py (h5); wire
   existing atlas_runner/atlas_analyze (h1), learnability.py (h3),
   observer_* (h6). no new journal columns; if one is unavoidable, stop
   and route through csv-contract first.
3. main (0): nohup the runs (night-shift-ops), collect results/*.json.
4. schedule-attacker (80k): attack h3/h6 with the produced schedules.
5. interdisciplinary (80k): literature check for h2 (Fouad 2018, Gray
   2005), flag any overclaim.
6. adversary (120k): attack h2 metrics and the random baseline.
7. blue_team (120k): statistical validity of every verdict (CI, effect
   size, multiple comparisons, cherry-picking).
8. memory (30k): verdict rows per hypothesis.
9. editor (100k): docs/hypotheses_results.md, ablation tables
   (ablation-interpreter), paper verdict section.

exit: every h1-h6 has confirmed / refuted-with-data / inconclusive, with
numbers. negative results published.

## PHASE C — red team t13-t17 + blue team (priority 4)

1. adversary (200k): t13 predictability, t14 sustained fatigue, t15 tick
   race, t16 immune bypass, t17 nft bypass. results/red_team/REPORT_r3.md.
2. blue_team (200k): validate each, propose minimal fix, verify by run.
3. architect (150k): implement accepted mitigations, one commit each.
4. adversary (100k): last pass on the fixed tree, closure or residual.
5. memory + editor (80k): STATE rows, threat_model.md.

exit: every t has verdict + mitigation or documented residual.

## PHASE D — release r4 (priority 6)

1. methodologist (100k): paper skeleton, 6 attacker models, honest
   limitations, venue decision (workshop/arXiv unless h2 is strong).
2. architect (150k): install target, systemd unit review, proteusctl
   status polish, CITATION.cff placeholder, LICENSE check.
3. editor (150k): paper draft, README v1.0, CHANGELOG, release notes.
4. seam-auditor (80k): journal/nft seam, manifest completeness.
5. memory (30k): manifests, STATE final rows.

exit: paper draft complete with the h2 verdict; packaging green;
reproducibility manifest present.

## PHASE E — final validation and v1.0 (priority 7)

1. main (0): full gate, bench validate, ROC to 6000, ASan on bench.
2. adversary (60k) + blue_team (60k): final pass, only if C/D changed
   code since their last look.
3. prioritizer (30k): go/no-go against the definition of done.
4. editor (50k): final README/CHANGELOG, DOI placeholder.
5. owner: `git tag v1.0 && git push --tags`, keep r2.5-anchor forever.

## definition of done for v1.0

- idle < 0.5% measured 600 s, wake p99 < 1 ms, determinism identical.
- h1-h6 verdicts published, h2 answered honestly (better/same/worse).
- t1-t17 closed, narrowed, or published as residual.
- make test, ubsan, ASan (bench), bench validate green.
- paper draft with formal threat model + verdicts.
- manifest with compiler, flags, binary sha256, commit.
- BENCH_HANDOFF.md lists every root-only command.

## skills map (all active skills, where each binds)

| skill | phase / use |
|---|---|
| emergent-behavior | A, B: tick and detector changes must not encode goals |
| determinism-gate | A, E: replay cmp after tick change |
| proteus-seam | C, D: journal/nft seam audit |
| scientific-methodology | B, D: registration before runs, verdicts |
| c99-posix-strict | A, B, C: every edited .c/.h |
| erlang-otp-strict | N/A: repo has no Erlang/OTP component; state it in D |
| biological-validation | B: hypotheses registered before running |
| statistical-rigor | B: every number mean + CI + n + effect size |
| reproducibility-manifest | D, E: manifest at release |
| suckless-core | A, D: no new abstraction unless used 3x |
| clean-c | A, B, C: touched functions |
| git-discipline | every commit: atomic, typed, green gate |
| headless-first | B: every claim from headless runs |
| csv-contract | B: only if a column change is unavoidable |
| literature-grounding | B, D: primary sources for every number |
| night-shift-ops | B, C: long runs |
| sweep-runner | B: seed sweeps |
| ablation-interpreter | B: h1 tables |
| caveman | every reply |

## agents map (all ten, sequential)

methodologist (A,B,D), architect (A,B,C,D), adversary (A,B,C,E),
blue_team (A,B,C,E), schedule-attacker (B), interdisciplinary (B),
seam-auditor (D), memory (A,B,C,D), editor (A,B,D,E),
prioritizer (phase gates and budget triage).

## stop rules

- can a phase not meet its cap? prioritizer cuts the lowest item, never
  the top. stopping at a STATE checkpoint is not surrender.
- an agent that returns no evidence is rerun once with a narrower prompt;
  a second empty answer means continue without it and say so.
- owner-only blockers (push, chown, bench root checks) go to
  docs/BENCH_HANDOFF.md; the phase continues with the next item.
