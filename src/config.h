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

#endif
