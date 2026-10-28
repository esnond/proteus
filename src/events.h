#ifndef PROTEUS_EVENTS_H
#define PROTEUS_EVENTS_H

#include <stdint.h>

/*
 * event schedule, the external history that drives the nervous system.
 * frozen csv schema: t_ms,kind,amp. one comment line carries the version,
 * the next carries the column names. rows must be sorted by t_ms.
 * kind burst: a scan burst on closed ports, amplitude is the normalized
 * burst density, drives the ash nociceptor.
 * kind legit: a completed legitimate session, drives the serotonin arm.
 */
#define EVENTS_MAX 4096

enum { EVK_BURST = 0, EVK_LEGIT = 1 };

typedef struct {
    double t_ms;
    int    kind;     /* evk_burst or evk_legit */
    float  amp;      /* ash envelope amplitude 0..1, burst only */
} event_t;

typedef struct {
    event_t ev[EVENTS_MAX];
    int     n;
    int     cursor;  /* next unfired event */
} schedule_t;

/* parse a schedule file, -1 on any malformed row or unsorted time */
int    schedule_load(schedule_t* sc, const char* path);

#endif
