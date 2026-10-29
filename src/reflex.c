#include "reflex.h"
#include "config.h"

void reflex_init(reflex_t* r)
{
    r->in_rev = 0;
    r->start_ms = 0.0;
    r->last_move_ms = -2.0 * MOVE_COOLDOWN_MS;
    r->fired = 0;
}

int reflex_update(reflex_t* r, float speed, double t_ms)
{
    if (speed < REVERSAL_SPEED_MMS) {
        if (!r->in_rev) {
            r->in_rev = 1;
            r->start_ms = t_ms;
        } else if (t_ms - r->start_ms >= (double)REVERSAL_CONFIRM_MS &&
                   t_ms - r->last_move_ms >= (double)MOVE_COOLDOWN_MS) {
            r->last_move_ms = t_ms;
            r->fired++;
            return 1;
        }
        return 0;
    }
    r->in_rev = 0;
    return 0;
}
