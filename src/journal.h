#ifndef PROTEUS_JOURNAL_H
#define PROTEUS_JOURNAL_H

#include <stdint.h>
#include <stdio.h>

/*
 * party journal, the frozen per tick record. csv contract: header lives in
 * one comment line, columns documented in docs/schemas.md, decimal dot, no
 * scientific notation. the journal is part of the determinism gate: same
 * seed plus the same event history must give bitwise identical bytes.
 * schema v2 adds det_score and cause, v1 readers are not compatible, see
 * docs/schemas.md.
 *
 * columns:
 *   t_ms       simulated time, ms
 *   act_ava    ava transmitter release, 0..1
 *   act_avb    avb transmitter release, 0..1
 *   ash_trace  nociceptor envelope, 0..1
 *   ser        serotonin, 0..1
 *   oct        octopamine, 0..1
 *   dop        dopamine, 0..1
 *   det_score  detector matched filter sum over the one second window
 *   port       current listening port
 *   cause      0 none, 1 endogenous reversal, 2 detector, 3 both
 *   event      0 none, 1 burst, 2 legit, 3 det_armed, 4 det_fired,
 *              5 det_cool
 */

#define JOURNAL_COLS 11

typedef struct {
    FILE* out;
} journal_t;

void journal_open(journal_t* j, FILE* out, uint64_t seed, int n_events);
void journal_row(journal_t* j, double t_ms, float act_ava, float act_avb,
                 float ash_trace, float ser, float oct, float dop,
                 float det_score, int port, int cause, int event);

#endif
