#ifndef PROTEUS_REFLEX_H
#define PROTEUS_REFLEX_H

/*
 * reversal reflex: a rising edge of ava across the threshold fires one
 * port move, with a refractory period so one reversal episode is one
 * move. the mapping is the r0 row of the motor table: ava above threshold
 * is a reversal, a reversal moves the port.
 */
typedef struct {
    float  prev_act;
    double last_move_ms;
    int    fired;
} reflex_t;

void reflex_init(reflex_t* r);
/* returns 1 when the reflex fires a port move this tick */
int  reflex_update(reflex_t* r, float act_ava, double t_ms);

#endif
