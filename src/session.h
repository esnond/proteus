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
    int         listen;        /* r2: accept network peers on the port */
    const char* bind_addr;     /* r2: listener address, default loopback */
    int         nft;           /* r2: mirror the bans into nftables */
    int         seccomp;       /* r2: install the syscall whitelist */
    int         idle_divisor;  /* r2.5: slow the tick while quiet, 1 = off */
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
    long   net_scans;      /* r2: scan shaped connections */
    long   net_sessions;   /* r2: peer sessions with payload */
    long   trust;          /* r2.5: live registration tokens */
    long   bans;           /* r2: ips banned by the detector */
    int    nft_up;         /* r2: backend state, 0 userspace only */
} session_stats_t;

/* run the session, 0 on success, 1 init failure, 2 non finite state */
int session_run(const session_cfg_t* cfg, session_stats_t* stats);

#endif
