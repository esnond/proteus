#include "reflex.h"
#include "config.h"

void reflex_init(reflex_t* r)
{
    r->prev_act = 0.0f;
    r->last_move_ms = -2.0 * MOVE_COOLDOWN_MS;
    r->fired = 0;
}

int reflex_update(reflex_t* r, float act_ava, double t_ms)
{
    int rising = act_ava >= AVA_MOVE_THR && r->prev_act < AVA_MOVE_THR;

    r->prev_act = act_ava;
    if (rising && t_ms - r->last_move_ms >= (double)MOVE_COOLDOWN_MS) {
        r->last_move_ms = t_ms;
        r->fired++;
        return 1;
    }
    return 0;
}
