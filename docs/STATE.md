# STATE.md — r2.5 immune scope + hardening

branch: r2.5-immune
anchor: tag r2-bench-anchor at 19de9e4 (bench-validated r2)
updated: 2026-10-29

## per-item rows

| id | status | evidence | commit | next action |
|---|---|---|---|---|
| phase0 backup + anchor | done | tag r2-bench-anchor, docs/ROLLBACK.md, results/manifest_phase0.txt | 8e952bc | push (owner, no creds in agent env) |
| neato scrub (HX-9) | done (no-op) | docs/NEATO_INVENTORY.md: zero hits, 4 scope notes kept | 8e952bc | none |
| f1.1 side-channel criterion | RETRACTED, not reproducible | 4 fresh runs FAIL (d +0.32, +0.35, -0.87, -0.38) then 1 PASS (d -0.14): the probe is unstable and measures kernel RTT on the public port. validate 5.1 now reports SKIP + raw data; the tool uses a per-run tmp dir (bench died on /tmp/sd_42.err EACCES) | this commit | redesign: server-side response or same-port probe |
| f1.2 gcore gap | done (bench script) | tools/bench_gcore_test.sh, tools/dump_audit.sh; sandbox has no dumper | b298bb0 | `sudo tools/bench_gcore_test.sh` |
| f1.3 idle <1% | done (opt-in flag) | `--idle-divisor 20`: cpu 0.79%, wake p99 0.95 ms, sim clock 20x slower while quiet | bbe6b6e | bench 300 s confirmation |
| f1.4 FP/ROC | partial, published | tools/roc_sweep.sh: live 0/1600 FP, 4/4 det, upper bound 18.8/10k (needs 6000) | 011b790 | `LEGIT_SECS=60 tools/roc_sweep.sh 10` |
| immune scope core | done | src/trust.c, docs/immune_scope.md: token live with pid, revoked on exit; trusted source not banned | 6247849 | bench t4/t6 |
| red team t1-t12 | done/first pass | results/red_team/REPORT.md: 8 pass, 2 documented, 2 bench | 378c5cf | bench t4/t6, third pass after fixes |
| standards audit | done with exceptions | docs/STANDARDS_AUDIT.md: session_run 307 lines justified | pending commit | r3 refactor |
| full suite | running | /tmp/r25.log, results/r2_validation (bench) | pending commit | re-run on bench |
| ci netns job | fixed, unverified in agent env | ci.yml runs the job under sudo, script skips loudly when unprivileged, builds proteusd | this commit | watch the next Actions run |
| bench gcore glob | fixed | gcore writes memory_dump.PID, the old glob only matched *.core; stale cores removed first | this commit | `sudo tools/bench_gcore_test.sh` |
| sudo artifact ownership | fixed | validate_r2.sh and bench_gcore_test.sh chown the tree back to SUDO_UID at exit | this commit | owner: one-time `sudo chown -R esnond:esnond ~/proteus` for the current root owned build |
| h7 auth failure ban | done, immune scope | connectome path failed on the bench after mixed traffic (oct pegged, ava collapsed, det_score 16-20 < 30); replaced by a direct immune-scope ban: seed 8/23/12 ttb 610-620 ms, legit 3/3 accepted, cause 0 + event 8 | this commit | bench test_1_4 |
| blue_team agent | created, smoke tested | ~/.zcode/agents/blue_team.md (10th agent); test run validated t7, found the oversized variant | this commit | restart the session so the agent type is discovered |
| ipc oversized command | fixed | MSG_TRUNC + length check; regression test fails pre-fix (status blocked) and passes post-fix | this commit | residual: 8 legit `log` subscribers still fill the table |
| v1.0 execution plan | written | docs/PLAN_V1.md: 5 phases, per-phase token caps 550/800/600/450/200k, all skills and 10 agents mapped | this commit | start phase A after the owner gate |
| h8_idle registration | done | docs/hypotheses.md h8; four numeric criteria; live-mode `-d` must become wall seconds (design point recorded) | this commit | implement event-driven tick |
| token ledger A | 577k / cap 550k | methodologist call 577k for a 25-line block; plan rule revised to at most two calls per phase, 600k each, rest inline | - | no further agent calls this phase |
| bench rerun bb051b8 | 18 pass / 1 fail / 1 skip | 1.4 green (ban 610 ms, source 10.0.0.5), 5.2 dump audit clean, 5.1 skip with reason; only 4.1 idle red: 1.03% at div 20 vs <1%, runs range 0.66-1.03% | bb051b8 | phase A decision |
| phase A accumulator loop | implemented, gate green | docs/architecture_research.md: idle 0.67% at 10.8 ticks/s, scan ban fires (det_score 30.43, density 7.8%), auth ban 670 ms, wall-clock security timers, ipc pending-fd race closed | this commit | bench: rerun validate (4.1 new shape) |
| phase A event-driven tick | reverted, falsified | idle 0.010% but det_score 6.25 after 121 scan events, sensor test banned nobody; empty ticks are part of the input model. data in docs/hypotheses.md h8 | (uncommitted, reverted) | owner decision: retune detector, 20x engine, or drop target |
| bench validate 02baf77 | 18 pass / 0 fail / 2 skip | 1.4 ban 680 ms; 5.2 dump clean; 4.1 SKIP (wake probe insufficient, base cpu 8.81% window); 5.1 SKIP retracted criterion. owner ran sudo tools/validate_r2.sh | 02baf77 | 4.1 needs the h8 instrument for wake p99; SKIP is not a PASS, recorded |
| h1-h6 registration | done, before runs | docs/hypotheses.md r3 phase B: h1-h6 mapped to г1-г7 + atlas_spec; seeds 101..110 cal / 1001..1100 train / 2001..2100 held out; alpha 0.0083 per h, power > 0.99 at d >= 0.5; г3 NOT_RUN (no population harness) | 051555a | runs |
| phase B train run | running | atlas_runner bio 1001..1100 -> g1_g7_runner (flat/seeded/random + g1-g7 analysis); smoke: 30 s/party, 208-214 moves, all 3 conditions verified; ~400 parties ~4 h wall, checkpoint+resume | this commit | verdicts after the chain; then held-out validate 2001..2100 |
| h5 sustained tool | written, registered first | tools/sustained_attack.py: 20 train seeds paired, dose-matched sustained schedule frozen in the tool, wilcoxon + CI + d; run launched | this commit | verdict into hypotheses_results |

## known gaps (no hiding)

1. ~~brute force with an auth payload is classified as a session~~ fixed
   in h7: a failed `PROTEUS AUTH` line raises the nociceptor and a burst
   source becomes the ban target. residual: only clients that speak the
   auth line are counted, a stuffer that sends arbitrary payload stays a
   session. detector reach is also seed dependent (seed 23 does not ban
   even a dense silent scan, max det_score 29.35).
2. the nft mark fast path does not fire for a marked local client
   (counter 0); tier A rests on the token path.
3. FP<=5/10k is not yet proven: 0/1600 gives an upper bound of 18.8/10k.
4. push is owner-only: the agent environment has no git credentials.
5. the journal header written by the binary says `journal v1` while
   docs/schemas.md declares v2 (columns match v2); fix deferred to
   phase D because a header change invalidates old golden journals.
6. bench 4.1 idle reports SKIP (wake probe insufficient), base cpu
   8.81% in its window; sandbox evidence for the accumulator loop is
   0.67% at 10.8 ticks/s; the SKIP must not be cited as a PASS.
7. ASan cannot start in the agent sandbox ("ASan runtime does not come
   first in initial library list"); ubsan and the strict build are green,
   ASan is a bench item.

## how to resume

read the rows above, then `/tmp/r25.log` or results/r2_validation/report.md.
