# proteus, closing report, 2026-10-30

**status: closed** at tag `v1.0.1`. the repository is frozen; no further
runs are planned. everything below is written for whoever opens this repo
next, including me in six months.

## what it is

a deterministic C99 simulator of a 279-neuron *C. elegans* connectome
(graded potentials, short-term plasticity, serotonin/octopamine/dopamine,
body physics) running inside a network daemon, where TCP port moves and
firewall bans are a readout of that nervous system's state instead of a
seeded PRNG. ~3.6k lines of layer code over ~3.5k lines of vendored
engine, 16 registered hypotheses, three schedule modes, bitwise party
replay.

## what holds

- **the loop closes.** a scan burst drives ASH, ASH drives the body, the
  body drives AVA/AVB, command activity moves the port. no policy layer.
- **determinism is real.** same seed, same event file, same binary,
  journal identical byte for byte (one pair verified at seed 42; the 100 party replay was not run).
- **detection works at a frozen threshold.** 50.0 derived from a false
  alarm budget: 8 false alarms across 10 seeds × 600 s; burst detection
  8/10 any firing (Wilson 95% CI [0.490, 0.943]) but only 3/10 inside 3 s of stimulus onset (Wilson [0.108, 0.603]): the early fires precede the stimulus.
- **one metric spans both fields.** *L*, observations until the next
  action is predicted with p ≥ 0.9, is reported for worm behaviour and
  for the defense schedule alike.

## what was refuted (kept on purpose)

1. **biology is not more random than a PRNG.** at 64 symbols the ceiling
   is 6 bits per move and a PRNG already reaches it. the L metric
   saturates for almost uniform sources, so "0/100 vs 0/100" measured the
   metric, not a tie.
2. **the PRNG arm is broken.** `DEST_RANDOM`'s xorshift64\* seed is
   recoverable from **3 observed ports** in 0.6 s over 2 × 10⁵ candidates;
   the recovered seed replays all 80 moves exactly
   (`tools/seed_recovery_attack.py`).
3. **the secret permutation buys nothing.** a fixed bijection of the
   output alphabet is absorbed by any learner; measured accuracy is
   identical before and after relabeling. it also created the live/
   headless divergence, which invalidated measurements of the live
   schedule based on journals.
4. **the bio readout is rank one**, a mean of six scalars, giving 16
   distinct ports and 3.672 bits/move, *more* predictable than the PRNG
   under a held out order-1 attacker (0.185 vs 0.000).
5. **h2 was falsified mid-run**: the destination was a counter, not the
   connectome. the honest follow-up (h9) is what produced items 1–4.

## open defects at close

listed in `docs/RELEASE_V1.md`: seed recovery, live/headless divergence,
rank one readout, move rate mean 97.5 per 600 s, 95% CI [79.7, 115.3], below the preregistered 100-300
band (recorded as a miss, never run again to fit).

## value, honestly

as a **security** artifact: modest. no crypto proof, no external
validation, and the headline is a negative result.

as a **computational neuroscience** artifact: stronger than it looks. a
connectome simulator written from scratch, closed onto a real socket,
fully replayable, with preregistration and published refutations. that
combination is rare and is the reason to keep the repo rather than delete
it.

the missing ingredient for a real result was never more code: it was one
external user, one comparison against an established simulator
(Brian2/NEURON), or one experiment where coupling measurably lowers an
attacker's success. none of those happened.

## if someone continues

in this order, and nothing else:

1. **E0 hygiene** (`docs/PLAN_V11.md` §4): manifest, estimators corrected
   for bias, ≥500 moves per party, ≥30 seeds. without this no number in
   the repo is publishable.
2. **h13 coupling**, the only claim a PRNG cannot make. KL divergence
   between the next move distribution with and without an active probe,
   paired seeds, control arm with the detector removed.
3. **the reservoir benchmark**, if the neuroscience side is the goal:
   connectome vs random reservoir on NARMA / memory capacity. no ports,
   no security, directly comparable to published results.

if coupling comes out at zero, and it may, publish that, close the
security line, and keep the engine as a teaching and ablation testbed.

## revival

```bash
make && make test
./proteusd -s 1001 -d 600 -e tests/fixtures/burst.csv --dest-mode bio -o /tmp/p.csv
tools/render_demo.py /tmp/p.csv -o /tmp/p.svg
tools/seed_recovery_attack.py /tmp/p.csv --k 3   # breaks DEST_RANDOM
```

## data not in git

`results/sustained_journals/`, 40 party journals, 158 MB, gitignored.
keep them locally; they are the raw input for any reanalysis.

## the one sentence

the organism was never the randomness source; it was the control loop,
and the loop is what the next person should test.
