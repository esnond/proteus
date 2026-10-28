#include <string.h>

#include "detector.h"
#include "config.h"

void det_init(detector_t* d)
{
    memset(d, 0, sizeof(*d));
    d->state = DET_ARMED;
}

/*
 * the score is the sum of ava deviations from the slow baseline over the
 * dense window. the baseline is an ema, so a reversal episode shows as a
 * positive hump only while it outlives the adaptation, which is the
 * matched filter property the r1 spec asks for.
 */
static float window_score(detector_t* d)
{
    float sum = 0.0f;
    int i;
    for (i = 0; i < d->fill; i++)
        sum += d->win[i] - d->baseline;
    return sum;
}

int det_update(detector_t* d, float act_ava, double t_ms)
{
    int ev = 0;

    /* the baseline seeds from the first ava reading: starting from zero
     * would paint the boot transient as a giant positive deviation and
     * spend the first cooling period on a phantom */
    if (!d->started) {
        d->started = 1;
        d->baseline = act_ava;
    }
    d->baseline += (act_ava - d->baseline) *
                   ((float)TICK_MS / (float)DET_BASE_TAU_MS);
    d->win[d->head] = act_ava;
    d->head = (d->head + 1) % DET_WINDOW_TICKS;
    if (d->fill < DET_WINDOW_TICKS)
        d->fill++;
    d->score = window_score(d);

    if (d->state == DET_ARMED) {
        if (d->fill >= DET_WINDOW_TICKS && d->score >= DET_THRESHOLD) {
            d->state = DET_FIRED;
            d->cool_until_ms = t_ms + (double)DET_COOL_MS;
            d->firings++;
            ev = 2;   /* det_fired */
        }
    } else if (d->state == DET_FIRED) {
        d->state = DET_COOL;
        ev = 1;       /* det_cool, one tick after the fire */
    } else if (t_ms >= d->cool_until_ms) {
        d->state = DET_ARMED;
        ev = 3;       /* det_armed */
    }
    return ev;
}

const char* det_state_name(int state)
{
    switch (state) {
    case DET_ARMED: return "armed";
    case DET_FIRED: return "fired";
    case DET_COOL:  return "cool";
    }
    return "?";
}
