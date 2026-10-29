#ifndef PROTEUS_REFLEX_H
#define PROTEUS_REFLEX_H

/*
 * reversal reflex, h10: the engine's own reversal definition. a
 * reversal episode is signed body speed below REVERSAL_SPEED_MMS
 * sustained REVERSAL_CONFIRM_MS (petri metrics.c); the port move fires
 * once when the episode confirms, refractory MOVE_COOLDOWN_MS so one
 * episode is one move. the r0 ava-edge readout is retired with the
 * legacy wiring (ava is constitutively saturated under varshney).
 */
typedef struct {
    int    in_rev;      /* speed currently below threshold */
    double start_ms;    /* episode start */
    double last_move_ms;
    int    fired;
} reflex_t;

void reflex_init(reflex_t* r);
/* feed the signed body speed; returns 1 when the reflex fires a port
 * move this tick */
int  reflex_update(reflex_t* r, float speed, double t_ms);

#endif
