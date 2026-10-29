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

/* ava activation 0..1 above which the reversal reflex fires a port move.
 * set once in r0 from the ten seed calibration (baseline peak vs burst
 * plateau), frozen afterwards, never refitted to a green gate */
#define AVA_MOVE_THR     0.75f

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
 * noise (0.4 mv per sqrt ms), mirrors readme limitations */
#define NOISE_SCALE      1.0f

/* event loop (r3): on wake the accumulator joins wall time and the loop
 * steps the simulation once per TICK_MS of it, clamped to
 * TICK_CATCHUP_MAX steps per wake. quiet time therefore runs at most
 * TICK_CATCHUP_MAX ticks per IDLE_POLL_MS, which bounds idle cpu near
 * TICK_CATCHUP_MAX * tick_cost / IDLE_POLL_MS (~0.5% at 0.46 ms/tick),
 * and a burst still brings the empty ticks the detector statistics were
 * calibrated on. the reaper shares the poll deadline. */
#define TICK_CATCHUP_MAX 10
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

/* matched filter threshold, units are summed ava deviations over the one
 * second window. set once from the false alarm budget on withheld seeds
 * 101..110 (no events, 6000 s model time total): the lowest calibration
 * grid threshold with false alarms within the budget of one per ten
 * minutes. curve and per seed tables: results/detector_threshold.csv.
 * never refitted to a green gate */
#define DET_THRESHOLD    30.0f

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
