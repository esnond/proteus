#ifndef PROTEUS_RATE_H
#define PROTEUS_RATE_H

#include "config.h"

/*
 * sliding event counters over a fixed window, one cell per second. used
 * for the scan density and the legitimate session share that drive the
 * modulator injection. deterministic, no rng.
 */
typedef struct {
    int    cells[MOD_WINDOW_S];
    int    last_sec;     /* second index the cells were built for */
    double total;        /* events ever seen */
} rate_t;

void  rate_init(rate_t* r);
void  rate_add(rate_t* r, double t_ms);
/* events per second over the window at time t_ms, rolls stale cells */
float rate_hz(rate_t* r, double t_ms);

#endif
