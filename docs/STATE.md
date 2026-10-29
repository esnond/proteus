# STATE.md — r2.5 immune scope + hardening

branch: r2.5-immune
anchor: tag r2-bench-anchor at 19de9e4 (bench-validated r2)
updated: 2026-10-29

## per-item rows

| id | status | evidence | commit | next action |
|---|---|---|---|---|
| phase0 backup + anchor | done | tag r2-bench-anchor, docs/ROLLBACK.md, results/manifest_phase0.txt | 8e952bc | push (owner, no creds in agent env) |
| neato scrub (HX-9) | done (no-op) | docs/NEATO_INVENTORY.md: zero hits, 4 scope notes kept | 8e952bc | none |
| f1.1 side-channel criterion | done | tools/secret_dependence.py: n=210/210, KS p=0.66, d=0.057, PASS | b298bb0 | bench re-run with ADV_REPEATS=20 |
| f1.2 gcore gap | done (bench script) | tools/bench_gcore_test.sh, tools/dump_audit.sh; sandbox has no dumper | b298bb0 | `sudo tools/bench_gcore_test.sh` |
| f1.3 idle <1% | done (opt-in flag) | `--idle-divisor 20`: cpu 0.79%, wake p99 0.95 ms, sim clock 20x slower while quiet | bbe6b6e | bench 300 s confirmation |
| f1.4 FP/ROC | partial, published | tools/roc_sweep.sh: live 0/1600 FP, 4/4 det, upper bound 18.8/10k (needs 6000) | 011b790 | `LEGIT_SECS=60 tools/roc_sweep.sh 10` |
| immune scope core | done | src/trust.c, docs/immune_scope.md: token live with pid, revoked on exit; trusted source not banned | 6247849 | bench t4/t6 |
| red team t1-t12 | done/first pass | results/red_team/REPORT.md: 8 pass, 2 documented, 2 bench | 378c5cf | bench t4/t6, third pass after fixes |
| standards audit | done with exceptions | docs/STANDARDS_AUDIT.md: session_run 307 lines justified | pending commit | r3 refactor |
| full suite | running | /tmp/r25.log, results/r2_validation (bench) | pending commit | re-run on bench |

## known gaps (no hiding)

1. brute force with an auth payload is classified as a session; the
   detector has no auth-failure input, so test 1.4 fails honestly.
2. the nft mark fast path does not fire for a marked local client
   (counter 0); tier A rests on the token path.
3. FP<=5/10k is not yet proven: 0/1600 gives an upper bound of 18.8/10k.
4. push is owner-only: the agent environment has no git credentials.

## how to resume

read the rows above, then `/tmp/r25.log` or results/r2_validation/report.md.
