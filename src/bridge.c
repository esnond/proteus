#include <stdlib.h>
#include <string.h>

#include "bridge.h"
#include "config.h"

int bridge_init(bridge_t* br, uint64_t seed)
{
    sim_opts_t opts;

    br->sim = malloc(sizeof(sim_t));
    if (!br->sim)
        return -1;
    memset(&opts, 0, sizeof(opts));
    opts.seed = seed;
    opts.headless = 1;
    opts.quiet = 1;
    if (sim_init(br->sim, &opts) != 0) {
        free(br->sim);
        br->sim = NULL;
        return -1;
    }
    {
        /* the wild type loader leaves noise_scale at zero, the defense
         * needs the stochastic channel on: without it every seed gives
         * the same trajectory and the schedule has no biological source */
        int i;
        for (i = 0; i < br->sim->ns.n; i++)
            br->sim->ns.cells[i].noise_scale = NOISE_SCALE;
    }
    {
        /* engine init leaves the body skeleton at zero, so the first
           world step sees the head at (0,0) inside the contact band and
           the worm inherits a phantom wall tap that burns the ash
           vesicle pool at t 0. prime the edge detectors so the session
           starts from a stateless body, real wall contact later still
           fires through the normal 0 to 1 edge */
        br->sim->prev_contact = 1;
        br->sim->prev_nose_contact = 1;
    }
    return 0;
}

void bridge_free(bridge_t* br)
{
    if (!br->sim)
        return;
    free(br->sim);
    br->sim = NULL;
}

void bridge_ash(bridge_t* br, float amp)
{
    if (amp < 0.0f)
        amp = 0.0f;
    if (amp > ASH_AMP_MAX)
        amp = ASH_AMP_MAX;
    br->sim->ash_trace = amp;
}

float bridge_ash_trace(const bridge_t* br)
{
    return br->sim->ash_trace;
}

void bridge_steps(bridge_t* br, int n)
{
    int i;
    for (i = 0; i < n; i++)
        sim_step(br->sim);
}

float bridge_ava(const bridge_t* br)
{
    int i = br->sim->idx_ava;
    if (i < 0)
        return 0.0f;
    return neuron_act(&br->sim->ns.cells[i]);
}

float bridge_avb(const bridge_t* br)
{
    int i = br->sim->idx_avb;
    if (i < 0)
        return 0.0f;
    return neuron_act(&br->sim->ns.cells[i]);
}

void bridge_modulators(const bridge_t* br, float* ser, float* oct, float* dop)
{
    *ser = br->sim->mod.ser;
    *oct = br->sim->mod.oct;
    *dop = br->sim->mod.dop;
}

double bridge_time(const bridge_t* br)
{
    return br->sim->t_ms;
}
