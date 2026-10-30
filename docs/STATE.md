# release state and provenance

> historical note, closed 2026-10-30 at tag v1.0.1 and maintained only as
> provenance. the closing state and handoff live in docs/CLOSE.md; the
> research program (not started) is docs/PLAN_V11.md.

## engine provenance

| item | value | evidence |
|---|---|---|
| petri v1.2 core | vendored at commit 42d5062 | varshney wiring, 279 neurons from csv, avb slow command; 1.25 ms/tick (was 0.46); TICK_CATCHUP_MAX 6 holds idle below 0.8% |
| dest modes | `--dest-mode {cycle,bio,random}` | bio = frozen mean readout of 6 state vars; random = proteus xorshift, engine rng untouched; replay green |
| detector channel | avb | ava saturated (mean 0.988, wiring regime, not noise); DET_THRESHOLD 50.0 frozen on seeds 101..110 |

## industry scan (a repo claim, not a review)

the owner-ordered scan found only mechanical open-loop randomization among
public competitors: sdn port shuffling, Morphisec, WireGuard address
hopping, Rlaph 2025. no published learnability metric for port schedules
was found; the closest items are an LSTM-based test (Mani 2023), 802.15.4
key cracking and a Cai unpredictability formalism. the stated
differentiators are the L benchmark, the closed-loop schedule, host
specific bio schedules and the bitwise replayable daemon.

## known gaps (no hiding)

1. only clients that speak the auth line are counted as auth failures; a
   stuffer that sends arbitrary payload stays a session. detector reach is
   seed dependent (seed 23 bans nobody, max det_score 29.35).
2. the nft mark fast path does not fire for a marked local client
   (counter 0); tier A rests on the token path.
3. FP<=5/10k is not proven: 0/1600 gives an upper bound of 18.8/10k.
4. git credentials are not stored in the build environment; pushes are
   made by the owner.
5. the journal header written by the binary says `journal v1` while
   docs/schemas.md declares v2 (columns match v2); deferred because a
   header change invalidates old golden journals.
6. the bench 4.1 idle check reports SKIP (wake probe insufficient); the
   SKIP must not be cited as a PASS.
7. ASan does not start in the sandbox ("ASan runtime does not come first
   in initial library list"); ubsan and the strict build are green, ASan
   is a bench item.
