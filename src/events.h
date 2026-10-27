#ifndef PROTEUS_EVENTS_H
#define PROTEUS_EVENTS_H

#include <stdint.h>

/*
 * event schedule, the external history that drives the nervous system.
 * frozen csv schema: t_ms,kind,amp. one comment line carries the version,
 * the next carries the column names. rows must be sorted by t_ms.
 * r0 implements kind burst only: a scan burst on closed ports, amplitude
 * is the normalized burst density.
 */
#define EVENTS_MAX 4096

typedef struct {
    double t_ms;
    float  amp;      /* ash envelope amplitude 0..1 */
} event_t;

typedef struct {
    event_t ev[EVENTS_MAX];
    int     n;
    int     cursor;  /* next unfired event */
} schedule_t;

/* parse a schedule file, -1 on any malformed row or unsorted time */
int    schedule_load(schedule_t* sc, const char* path);
/* apply all events due in [t_lo, t_hi): returns the last amplitude written
 * and sets *fired, 0 when nothing was due */
float  schedule_due(schedule_t* sc, double t_lo, double t_hi, int* fired);

#endif
