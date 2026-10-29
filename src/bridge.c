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
    /* petri v1.2 core: full varshney wiring, the file must exist or the
     * start fails loudly (no silent legacy fallback) */
    opts.connectome_path = VARSHNEY_CSV;
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
    /* v1.2 note: the r0 phantom-tap priming is removed pending a
       fresh check on the varshney wiring (h10 diagnostics) */
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

float bridge_speed(const bridge_t* br)
{
    return br->sim->body.speed;
}

int bridge_dest_index(const bridge_t* br)
{
    float mean;
    int idx;

    mean = (bridge_ava(br) + bridge_avb(br) + bridge_ash_trace(br) +
            br->sim->mod.ser + br->sim->mod.oct + br->sim->mod.dop) / 6.0f;
    if (!(mean > 0.0f))
        return 0;
    idx = (int)(mean * (float)PORT_WINDOW);
    if (idx >= PORT_WINDOW)
        idx = PORT_WINDOW - 1;
    return idx;
}

double bridge_time(const bridge_t* br)
{
    return br->sim->t_ms;
}
