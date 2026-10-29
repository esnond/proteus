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
5. ASan cannot start in the agent sandbox ("ASan runtime does not come
   first in initial library list"); ubsan and the strict build are green,
   ASan is a bench item.

## how to resume

read the rows above, then `/tmp/r25.log` or results/r2_validation/report.md.
