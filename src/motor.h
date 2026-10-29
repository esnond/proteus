#ifndef PROTEUS_MOTOR_H
#define PROTEUS_MOTOR_H

#include <stdint.h>

/*
 * motor: the moving tcp listener. legacy walk order is deterministic,
 * the n-th bind takes port base + n mod window. destination modes live
 * in config.h (DEST_*): cycle keeps the counter, bio and random take a
 * destination index from motor_move_to, the journalled port always
 * comes from getsockname in live mode, never from the intent.
 */
typedef struct {
    int      fd;    /* listening socket, -1 when closed */
    int      port;  /* current listening port, 0 when closed */
    int      binds; /* completed binds, drives the legacy walk order */
    int      moves; /* completed moves */
    int      live;  /* 1: bind real sockets, 0: headless port walk only */
    uint64_t rr;    /* xorshift64* state for DEST_RANDOM, engine rng untouched */
} motor_t;

/* bind the first port of the window on addr (dotted quad or 0.0.0.0
 * for the netns bench), -1 on failure. live=0 walks the port schedule
 * without sockets, so a headless replay does not depend on which host
 * ports happen to be free (the determinism contract needs this). seed
 * feeds only the DEST_RANDOM stream */
int  motor_init(motor_t* m, const char* addr, int live, uint64_t seed);
/* close the current listener and open the next port in the legacy
 * cycle, returns the new port or -1 */
int  motor_move(motor_t* m);
/* close the current listener and open base + index mod window; on a
 * live bind collision the walk continues forward with wraparound,
 * returns the new port or -1 */
int  motor_move_to(motor_t* m, int index);
/* next uniform window index from the proteus-side stream */
int  motor_next_random(motor_t* m);
void motor_close(motor_t* m);

#endif
