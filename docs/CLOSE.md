# proteus, closing report, 2026-10-30

**status: closed** at tag `v1.0.1`. the repository is frozen; no further
runs are planned. everything below is written for whoever opens this repo
next, including me in six months.

## what it is

a deterministic C99 simulator of a 279-neuron *C. elegans* connectome
(graded potentials, short-term plasticity, serotonin/octopamine/dopamine,
body physics) running inside a headless network daemon. the move tick is
set by the body speed reflex and the detector; in bio mode the TCP port
label is a hand written six-scalar readout of engine state, not a
closed-loop motor command. ~3.6k lines of layer code over ~3.5k lines of
vendored engine, 16 registered hypotheses, three schedule modes, bitwise
party replay.

## what holds

- **the loop does not close.** timing is identical across bio, cycle and
  random on 100 of 100 seeds; the readout includes the event trace and
  two event counters. there is a hand written policy layer.
- **determinism is real.** same seed, same event file, same binary,
  journal identical byte for byte (one pair verified at seed 42; all 300
  h9 journals match the manifest hashes).
- **detection does not establish the burst.** threshold 50.0 comes from a
  false alarm budget: 8 false alarms across 10 seeds x 600 s, no interval
  because the events are not independent. every journal records firmware
  det_fired at 1000 ms, before the burst; 0 of 10 record it within 3 s
  after 2010 ms; the recorded 3/10 is an artifact of t0 = 1000 ms and the
  8/10 is retracted. 2 of 10 journals have raw det_score crossings after
  2010 ms, masked by an earlier cooldown.
- **one metric spans both fields.** *L*, observations until the next
  action is predicted with p ≥ 0.9, is reported for worm behaviour and
  for the defense schedule alike; it is a sample complexity threshold,
  not an attack success probability.

## what was refuted (kept on purpose)

1. **biology is not more random than a PRNG.** at 64 symbols the ceiling
   is 6 bits per move and a PRNG already reaches it. the L metric
   saturates for almost uniform sources, so "0/100 vs 0/100" measured the
   metric, not a tie.
2. **the xorshift64\* arm is broken.** `DEST_RANDOM`'s xorshift64\* seed is
   recoverable from **3 observed ports** in under one second over 200001
   candidates; the recovered seed replays all 80 moves exactly
   (`tools/seed_recovery_attack.py`). no CSPRNG baseline was implemented,
   and no blind seed-recovery attack was implemented: the 0/20 result is
   four structural tests, 0/20 each.
3. **the secret permutation buys nothing.** a fixed bijection of the
   output alphabet is absorbed by any learner; measured accuracy is
   identical before and after relabeling. it also created the live/
   headless divergence, which invalidated measurements of the live
   schedule based on journals.
4. **the bio readout is rank one**, a mean of six scalars, giving 16
   distinct ports and 3.672 bits/move in the n = 1 reconnaissance. the
   n = 20 order 1 log-loss is 5.851 for bio against 6.025 for random,
   paired interval [-0.221, -0.129]; the marginal H comparison is
   confounded by alphabet size. the readout is a sufficient bottleneck;
   the connectome contribution is untested.
5. **h2 was falsified mid-run**: the destination was a counter, not the
   connectome. the honest follow-up (h9) is what produced items 1 to 4.

## open defects at close

listed in `docs/RELEASE_V1.md`: seed recovery, live/headless divergence,
rank one readout, move rate mean 97.5 per 600 s, 95% CI [79.7, 115.3], below the preregistered 100-300
band (recorded as a miss, never run again to fit).

## value, honestly

as a **security** artifact: modest. no crypto proof, no external
validation, and the headline is a negative result.

as a **computational neuroscience** artifact: stronger than it looks. a
connectome simulator written from scratch, replayable bitwise, with
preregistration and published refutations. the measured path is headless
and the live path diverges from the journal; the loop is not closed. that
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
