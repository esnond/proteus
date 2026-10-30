# red team report: r2.5 trust layer

generated 2026-10-29T07:29:54Z

| id | verdict | evidence |
|---|---|---|
| t1_token_forgery | PASS | 8 registrations |
| t2_replay_after_death | PASS | registrar exited |
| t12_restart_persistence | PASS | fresh daemon has an empty token table |
| t5_table_exhaustion | PASS | cap honoured at 32 entries |
| t2b_reaper_window | PASS | after the dead registrars: trust=0 (reap period 500 ms) |
| t7_registration_fuzz | PASS | 7 malformed cases |
| t4_foreign_uid | BENCH | needs a second uid: sudo -u nobody ./proteusctl register --socket /tmp/rt.sock must print denied (script: docs/BENCH_HANDOFF.md) |
| t6_token_timing | BENCH | unit harness not in the sandbox tree; method fixed: valid vs invalid token |
| t8_seccomp_growth | PASS | profile regenerated with registration in the trace: 36 syscalls (getrandom |
| t9_mark_bypass | PASS | mark is local metadata |
| t10_detector_blindness | DOCUMENTED | a registered source is exempt by construction within the 60 s trust window; defense is the registration allowlist (owner/root) |
| t11_downgrade | DOCUMENTED | unregistered traffic keeps the f1.4 numbers: 0 bans in 1600 legit sessions |

residual risks: an attacker with the same uid is inside the trust domain;
the mark fast path does not fire in this environment; a registered
service is exempt from the detector by construction.
