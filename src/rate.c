#include <string.h>

#include "rate.h"
#include "config.h"

void rate_init(rate_t* r)
{
    memset(r, 0, sizeof(*r));
}

/* the cell array rolls forward one second at a time, a cell holds all
 * events of one wall clock second of simulated time */
static void roll(rate_t* r, double t_ms)
{
    int sec = (int)(t_ms / 1000.0);
    int steps = sec - r->last_sec;
    int i;
    if (steps <= 0)
        return;
    if (steps >= MOD_WINDOW_S) {
        memset(r->cells, 0, sizeof(r->cells));
    } else {
        for (i = 0; i < steps; i++) {
            r->cells[(r->last_sec + 1 + i) % MOD_WINDOW_S] = 0;
        }
    }
    r->last_sec = sec;
}

void rate_add(rate_t* r, double t_ms)
{
    int sec = (int)(t_ms / 1000.0);
    roll(r, t_ms);
    r->cells[sec % MOD_WINDOW_S]++;
    r->total += 1.0;
}

float rate_hz(rate_t* r, double t_ms)
{
    int sum = 0, i;
    roll(r, t_ms);
    for (i = 0; i < MOD_WINDOW_S; i++)
        sum += r->cells[i];
    return (float)sum / (float)MOD_WINDOW_S;
}
