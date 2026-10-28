#ifndef PROTEUS_SESSION_H
#define PROTEUS_SESSION_H

#include <stdint.h>
#include <stdio.h>

/*
 * one protection session: the tick loop, the detector, the modulator
 * injection, the port walk and the journal. shared by proteusd and by
 * proteusctl party, which replays a session in its own process to verify
 * a saved journal bitwise.
 */
typedef struct {
    uint64_t    seed;
    double      duration_s;
    const char* events_path;   /* NULL for a quiet session */
    FILE*       journal_out;
    int         realtime;      /* pace ticks at wall clock, ms per tick */
    const char* socket_path;    /* status and log ipc, NULL disables */
    long        socket_group;  /* ipc group id, -1 keeps owner only */
} session_cfg_t;

typedef struct {
    int    port;
    int    det_state;      /* det_state_t */
    long   moves_a;        /* endogenous reversal moves */
    long   moves_b;        /* detector moves */
    long   moves_ab;       /* both in one tick */
    long   det_firings;
    long   rss_kb;
    double cpu_ms;
    double t_ms;
    float  ser;            /* serotonin, 0..1, with r1 injection */
    float  oct;            /* octopamine, 0..1, with r1 injection */
} session_stats_t;

/* run the session, 0 on success, 1 init failure, 2 non finite state */
int session_run(const session_cfg_t* cfg, session_stats_t* stats);

#endif
