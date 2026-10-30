# Proteus

**A closed-loop network defense whose schedule is a readout of a simulated
*C. elegans* nervous system.**

![port schedule driven by the connectome](docs/demo.svg)

The model organism is not a metaphor here. A 279-neuron connectome —
graded potentials, short-term synaptic plasticity, serotonin / octopamine /
dopamine modulation, and a physical body with muscles and proprioception —
runs continuously inside the daemon. Which TCP port the service listens on,
and when it moves, is a function of that nervous system's internal state.
There is no seeded PRNG in the schedule path.

The engine (`src/petri/`) was written from scratch for this project: it is a
biophysical simulator, not a neural-network library, and it is vendored here
with its own tests, CSV fixtures and documentation.

---

## Why a neuroscientist might care

1. **The loop is closed down to the wire.** ASH nociceptor activity produced
   by a scan burst changes the body, the body changes AVA/AVB command
   activity, and the command activity changes the port. One simulated
   organism, one real socket, no hand-written policy layer in between.
2. **The same metric measures worm and defense.** Behavioural predictability
   and schedule predictability are both reported as *L*: the number of
   observations after which an observer predicts the next action with
   probability ≥ 0.9.
3. **Everything replays bitwise.** Same seed, same event file, same binary →
   byte-identical journal (100 parties × 600 s checked). A behavioural claim
   can be re-run on a laptop and diffed.

## What is actually measured

| Measurement | Value | n | Note |
|---|---|---|---|
| Reversal-triggered move rate | 98 per 600 s [63..153] | 10 seeds | below the pre-registered 100–300 band; recorded as a miss |
| Detector false alarms | 8 / 10 seeds | 10 seeds × 600 s | one per 10 min budget |
| Burst detection | 8 / 10 | 10 journals | Wilson 95% CI [0.44, 0.97] |
| Detector threshold | 50.0 | frozen | lowest grid value meeting the FA budget |
| Schedule learnability, passive Markov attacker | cycle 88/100, bio 0/100, random 0/100 | 100 parties each | L capped at 65; see the caveat below |
| Idle CPU (bound) | ≤ 0.75% | — | 6 catch-up ticks × 1.25 ms per 1000 ms poll |

Per-party entropy figures exist but are single-seed reconnaissance, not
estimates with confidence intervals; they live in `docs/RESEARCH_V2.md`
and must not be quoted as results.

## What this project does not claim

Honest negative results, kept because they are the interesting part:

- **Biology is not more random than a PRNG.** At an alphabet of 64 symbols
  the ceiling is 6 bits per move and a PRNG already reaches it. The
  learnability metric saturates for any near-uniform source, so
  "0/100 vs 0/100" is a property of the metric, not a tie.
- **The random-schedule arm is broken.** Its xorshift64\* seed is
  recoverable from 3 observed ports (0.6 s, 2 × 10⁵ candidate space). This
  is measured and reproducible with `tools/seed_recovery_attack.py`.
- **The current bio readout is rank one** — a mean of six journal scalars —
  so it yields only 16 distinct ports in practice. That is a readout
  defect, not a connectome property.

The four claims that are winnable, and the experiments that would establish
them, are in `docs/PLAN_V11.md`.

## Build and run

```bash
make                       # cc -std=c99 -pedantic -Wall -Wextra -Werror -O2
make test                  # unit + replay determinism

# headless party, writes a journal to stdout
./proteusd -s 1 -d 60 -e tests/fixtures/burst.csv -o /tmp/party.csv

# destination mode: cycle (counter) | bio (connectome readout) | random
./proteusd -s 1 -d 60 -e tests/fixtures/burst.csv --dest-mode bio

# live listener with IPC control socket
./proteusd -s 1 -d 600 --listen --socket /tmp/p.sock -o /tmp/party.csv
./proteusctl status --socket /tmp/p.sock

# redraw the demo figure from any journal
tools/render_demo.py /tmp/party.csv -o docs/demo.svg
```

## Architecture

| Component | Role |
|---|---|
| `src/petri/` | vendored connectome engine, v1.2 — 279 neurons, graded potentials, STSP, modulators, body physics |
| `bridge` | engine ownership, membrane noise, modulator injection, state readout |
| `detector` | matched filter on AVB deviation, 1 s window, frozen threshold |
| `reflex` | engine-native reversal episode (speed < −0.02 mm/s sustained 500 ms) |
| `motor` | port window 40000–40063, non-blocking listener |
| `sensor` | non-blocking accept, payload classification, ban list with wall-clock TTL |
| `nft` | hand-built netlink (libmnl) — table `proteus`, set `banned` |
| `seccomp` | 31-syscall BPF filter generated from a static-binary strace |
| `ipc` | SOCK_SEQPACKET, SO_PEERCRED, group auth |
| `journal` | 11-column CSV, versioned, one row per 10 ms of model time |

Design rules enforced by the build and by review: the simulation is
deterministic and RNG-free outside a seed-derived stream; no wall-clock
time enters the sim path; no goal-directed constructs in the sim path
(behaviour must emerge from the connectome, not from a written policy).

## Reproducibility

Every run records binary SHA256, commit, compiler, flags, seed range and
the event-file hash. Party replay is bitwise:

```bash
./proteusd -s 42 -d 600 -e tests/fixtures/burst.csv -o a.csv
./proteusd -s 42 -d 600 -e tests/fixtures/burst.csv -o b.csv
cmp a.csv b.csv && echo "bitwise identical"
```

## Documents

| File | Contents |
|---|---|
| `docs/theory.md` | the L metric, registered hypotheses г1–г7 |
| `docs/hypotheses.md` | h1–h15 with pre-registered success and falsification criteria |
| `docs/hypotheses_results.md` | verdicts, including failures and one post-hoc disclosure |
| `docs/RESEARCH_V2.md` | deep research: reconstruction attack, retractions, corrections |
| `docs/PLAN_V11.md` | v1.1 program: four provable claims, architecture, budget |
| `docs/STATE.md` | current stage and open decisions |

## Status

**Closed at v1.0.1 (2026-10-30).** The repository is frozen. A closing
report with the measured claims, the refutations, the open defects and the
three things a successor should do first is in `docs/CLOSE.md`.

R0–R2 sandbox-validated. R3 measurement layer complete. Phase C (red/blue
team) and Phase D (packaging) were stopped at close. The privileged bench
(kernel nftables enforcement, active-probe measurements) was never run;
anything requiring it is marked pending, not claimed.

## License and citation

ISC — see `LICENSE`. If you use this in research, cite the repository
(DOI via Zenodo, see `CITATION.cff`) and the connectome engine's primary
sources: Yemini et al. 2013 (speed/curvature), Pierce-Shimomura et al. 1999
(ASH nociceptor), Kato et al. 2015 (AVA transient), Tsodyks & Markram 1997
(STSP).
