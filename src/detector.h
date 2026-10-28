#ifndef PROTEUS_DETECTOR_H
#define PROTEUS_DETECTOR_H

#include "config.h"

/*
 * threat detector: a matched filter over the ava trace. the score is the
 * sum of deviations from a slow adapted baseline over the dense one second
 * window. the threshold is set by the false alarm budget on withheld
 * seeds, never by the effect size. states: armed, fired, cooling.
 */
typedef enum {
    DET_ARMED = 0,
    DET_FIRED,
    DET_COOL
} det_state_t;

typedef struct {
    float  win[DET_WINDOW_TICKS];
    int    fill;            /* ticks stored, caps at window size */
    int    head;
    int    started;         /* baseline seeded from the first ava value */
    float  baseline;
    float  score;           /* current window sum of deviations */
    int    state;           /* det_state_t */
    double cool_until_ms;
    int    firings;
} detector_t;

void  det_init(detector_t* d);
/* one tick of ava; returns 1 on the firing edge, 2 when cooling starts,
 * 3 when re armed, 0 otherwise, those become journal events */
int   det_update(detector_t* d, float act_ava, double t_ms);
const char* det_state_name(int state);

#endif
