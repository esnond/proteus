#ifndef PROTEUS_CONFIG_H
#define PROTEUS_CONFIG_H

/*
 * proteus layer constants. engine biophysics live in src/petri/config.h.
 * units are written next to every value.
 */

/* protection tick, ms of simulated time per journal row */
#define TICK_MS          10

/* schema versions, bumped on any breaking column or semantics change */
#define JOURNAL_VERSION  1
#define EVENTS_VERSION   1

/* tcp port window for the moving listener, the motor range the worm walks */
#define PORT_BASE        40000
#define PORT_WINDOW      64

/* ---- destination mode (h9, registered 2026-10-29 docs/hypotheses.md) ----
 * cycle:  legacy counter, port = base + n mod window (falsified as a
 *         schedule: strictly learnable after one window pass, L=65).
 * bio:    destination index is a frozen linear readout of the connectome
 *         state at the move tick, equal weights, no tuning freedom:
 *         index = floor(window * mean(ava, avb, ash, ser, oct, dop)),
 *         clamped to window-1. symmetric to the detector's read of AVA:
 *         an encoding of organism state into body position, no goals.
 * random: uniform draw from a proteus-side xorshift seeded with the run
 *         seed, independent of the engine rng (the engine stream must
 *         stay identical across modes; only the destination differs). */
#define DEST_CYCLE       0
#define DEST_BIO         1
#define DEST_RANDOM      2

/* connectome wiring: the petri v1.2 default is the full varshney sheet
 * (279 somatic neurons), vendored with the engine. loaded relative to
 * the working directory; missing file fails the start loudly rather
 * than silently falling back to the builtin legacy wiring. */
#define VARSHNEY_CSV     "src/petri/data/varshney_full.csv"

/* ---- h10: reversal trigger, engine native (petri v1.2) ----
 * the varshney wiring holds ava saturated (mean 0.988 measured); the
 * r0 ava-edge readout is dead. the engine's own reversal definition
 * (petri metrics.c): signed body speed below the threshold sustained
 * over the confirmation window is one reversal episode. constants
 * cited from the engine, never tuned. */
#define REVERSAL_SPEED_MMS   0.02f
#define REVERSAL_CONFIRM_MS  500

/* refractory period between port moves, ms, approx: one reversal episode
 * is one move, not a move every tick while ava stays high */
#define MOVE_COOLDOWN_MS 2000

/* analysis window after burst onset used by proteusctl reflex, ms, covers
 * the burst and most of the ash envelope decay */
#define RESP_WINDOW_MS   2000

/* nociceptor envelope ceiling, scan burst density is normalized to this */
#define ASH_AMP_MAX      1.0f

/* membrane noise multiplier passed to the engine noise channel, the engine
 * ships it at zero (wild type loader) which would collapse every seed into
 * the same trajectory. assumption: 1.0, the natural scale of the engine
 * noise (0.4 mv per sqrt ms), mirrors readme limitations. h10 check: the
 * ava saturation under varshney is the wiring regime, not noise (upstream
 * pegs ava at shipped noise 0 too), so 1.0 stands */
#define NOISE_SCALE      1.0f

/* event loop (r3): on wake the accumulator joins wall time and the loop
 * steps the simulation once per TICK_MS of it, clamped to
 * TICK_CATCHUP_MAX steps per wake. quiet time therefore runs at most
 * TICK_CATCHUP_MAX ticks per IDLE_POLL_MS, which bounds idle cpu near
 * TICK_CATCHUP_MAX * tick_cost / IDLE_POLL_MS (~0.5% at 0.46 ms/tick),
 * and a burst still brings the empty ticks the detector statistics were
 * calibrated on. the reaper shares the poll deadline. h10: the varshney
 * core costs ~1.25 ms/tick (was 0.46), so the clamp drops 10 -> 6 to
 * hold the idle bound under 0.8 percent (6 x 1.25 ms / 1000 ms), part
 * of the declared engine bump, recalibration runs under this value */
#define TICK_CATCHUP_MAX 6
#define IDLE_POLL_MS     1000

/* ---- r1: threat detector ---- */

/* dense matched filter window, ms of simulated time, r1 spec */
#define DET_WINDOW_MS    1000
#define DET_WINDOW_TICKS (DET_WINDOW_MS / TICK_MS)

/* baseline adaptation tau of the detector, ms. assumption: far above the
 * reversal cycle (about 1.5 s), far below the session length */
#define DET_BASE_TAU_MS  30000

/* detector cooling after a firing, ms. assumption: one order above the
 * port move refractory so one detection is one response */
#define DET_COOL_MS      10000

/* matched filter threshold, units are summed command-pool deviations
 * over the one second window. r1 set 30.0 from the false alarm budget on
 * withheld seeds 101..110 (no events, 6000 s model time total); h10
 * (2026-10-29) moved the channel to avb for the petri v1.2 wiring and
 * re-derived the threshold by the same frozen procedure: 50.0, the
 * lowest grid threshold with false alarms within the budget (8 FAs of
 * 10 allowed, 10 seeds x 600 s; detection 8/10 burst journals, wilson
 * ci [0.44, 0.97]; journals results/cal_v12/). never refitted to a
 * green gate */
#define DET_THRESHOLD    50.0f

/* ---- r1: modulator injection ---- */

/* sliding window for the scan density and the legit share, s, r1 spec */
#define MOD_WINDOW_S     60

/* octopamine injection rate per unit scan density, concentration per s of
 * simulated time at one scan event per second. the engine pulls the
 * concentration back with its own tau (5 s), so the equilibrium at
 * density d is rate * d * 5. chosen so the planned density range
 * 0.5..4 hz covers the engine working range (0.1..0.8), not from any
 * move rate result */
#define OCT_INJ_PER_HZ   0.040f

/* serotonin injection rate at a full legitimate share, concentration per
 * s of simulated time, equilibrium rate * share * 6 (engine tau). chosen
 * so a full legit share settles near 0.7, not from any move rate result */
#define SER_INJ_PER_SHARE 0.120f

#endif

/* ban ttl, ms of session time */
#define BAN_TTL_MS 60000
