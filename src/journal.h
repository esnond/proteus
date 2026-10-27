#ifndef PROTEUS_JOURNAL_H
#define PROTEUS_JOURNAL_H

#include <stdint.h>
#include <stdio.h>

/*
 * party journal, the frozen per tick record. csv contract: header lives in
 * one comment line, columns documented in readme, decimal dot, no scientific
 * notation. the journal is part of the determinism gate: same seed plus the
 * same event history must give bitwise identical bytes.
 *
 * columns:
 *   t_ms       simulated time, ms
 *   act_ava    ava transmitter release, 0..1
 *   act_avb    avb transmitter release, 0..1
 *   ash_trace  nociceptor envelope, 0..1
 *   ser        serotonin, 0..1
 *   oct        octopamine, 0..1
 *   dop        dopamine, 0..1
 *   port       current listening port
 *   event      0 none, 1 burst, 2 port move, 3 burst and move
 */

typedef struct {
    FILE* out;
} journal_t;

/* write the version and schema lines, then one row per tick */
void journal_open(journal_t* j, FILE* out, uint64_t seed, int n_events);
void journal_row(journal_t* j, double t_ms, float act_ava, float act_avb,
                 float ash_trace, float ser, float oct, float dop,
                 int port, int event_code);

#endif
