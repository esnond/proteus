# release v1.0.0 — 2026-10-30

## what this release is

a research prototype, packaged so results can be cited and re-run. it is
not a hardened security product. the privileged bench (kernel nftables
enforcement, active-probe measurement) has not been run.

included:

- vendored `C. elegans` connectome engine v1.2 (279 neurons, graded
  potentials, STSP, modulators, body physics), with its own tests and the
  varshney wiring sheet;
- detection and reflex layer (matched filter on AVB, engine-native
  reversal episodes), journal with bitwise party replay;
- three destination generators: `cycle`, `bio`, `random` (`--dest-mode`);
- pre-registered hypotheses h1–h15 with success and falsification criteria
  (`docs/hypotheses.md`), verdicts including failures and one post-hoc
  disclosure (`docs/hypotheses_results.md`);
- deep-research report with a working reconstruction attack and explicit
  retractions of unsourced numbers (`docs/RESEARCH_V2.md`);
- v1.1 plan with four provable claims and a frozen experiment budget
  (`docs/PLAN_V11.md`);
- demo figure generator (`tools/render_demo.py`, stdlib only).

## known defects, open at this tag

1. `DEST_RANDOM` is broken: the xorshift64\* seed is recoverable from 3
   observed ports (`tools/seed_recovery_attack.py`, 0.6 s over a 2 × 10⁵
   candidate space).
2. live and headless paths diverge: `headless_bind` never applies the
   secret permutation (`src/motor.c`), so every journalled science run
   describes a different stream than deployment.
3. the bio readout is rank one (`floor(64 · mean of six scalars)`), which
   caps it at 16 distinct ports in practice.
4. move rate 98 per 600 s sits below the pre-registered 100–300 band
   (recorded as a miss, not re-run to fit).

defects 1–3 are scheduled for removal in v1.1 (`docs/PLAN_V11.md` §3).

## what is not claimed

no entropy superiority of biology over a PRNG. no cryptographic security
proof. no claim that has not been measured in the sandbox.

## verification

`results/manifest.txt` records the commit, compiler, flags, binary hash
and the hashes of every tracked artifact. party replay:

    ./proteusd -s 42 -d 600 -e tests/fixtures/burst.csv -o a.csv
    ./proteusd -s 42 -d 600 -e tests/fixtures/burst.csv -o b.csv
    cmp a.csv b.csv
