#ifndef PROTEUS_BRIDGE_H
#define PROTEUS_BRIDGE_H

#include <stdint.h>

#include "petri/sim.h"

/*
 * bridge: owns one petri sim. proteus feeds sensory currents and reads the
 * command pool, the engine is never modified. injection goes through the
 * native nociceptor envelope (sim_t.ash_trace), the same path petri drives
 * from nose contact, so adaptation and stsp habituation come from the
 * engine, not from us.
 */
typedef struct {
    sim_t* sim;   /* heap, sim_t is well over a megabyte */
} bridge_t;

int    bridge_init(bridge_t* br, uint64_t seed);
void   bridge_free(bridge_t* br);
/* set the nociceptor envelope amplitude 0..1, decays with the engine ash
 * time constant, 2500 ms */
void   bridge_ash(bridge_t* br, float amp);
/* current nociceptor envelope after engine decay */
float  bridge_ash_trace(const bridge_t* br);
/* advance the engine by n ode steps, dt is 1 ms inside petri */
void   bridge_steps(bridge_t* br, int n);
/* command pool readout, transmitter release 0..1 */
float  bridge_ava(const bridge_t* br);
float  bridge_avb(const bridge_t* br);
/* modulator concentrations 0..1 */
void   bridge_modulators(const bridge_t* br, float* ser, float* oct,
                         float* dop);
/* engine time, ms */
double bridge_time(const bridge_t* br);

#endif
