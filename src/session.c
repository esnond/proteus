#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <time.h>

#include "bridge.h"
#include "config.h"
#include "detector.h"
#include "events.h"
#include "ipc.h"
#include "journal.h"
#include "motor.h"
#include "rate.h"
#include "reflex.h"
#include "session.h"

/* journal event codes, see docs/schemas.md */
enum {
    JEV_NONE = 0, JEV_BURST, JEV_LEGIT,
    JEV_DET_ARMED, JEV_DET_FIRED, JEV_DET_COOL
};
enum { CAUSE_NONE = 0, CAUSE_ENDO = 1, CAUSE_DET = 2 };

static long rss_kb(void)
{
    FILE* f = fopen("/proc/self/statm", "r");
    long total, resident = 0;
    if (!f)
        return 0;
    if (fscanf(f, "%ld %ld", &total, &resident) != 2)
        resident = 0;
    fclose(f);
    return resident * 4;   /* pages to kb on a 4k page machine */
}

static double cpu_ms(void)
{
    struct rusage ru;
    if (getrusage(RUSAGE_SELF, &ru) != 0)
        return 0.0;
    return (ru.ru_utime.tv_sec + ru.ru_stime.tv_sec) * 1000.0 +
           (ru.ru_utime.tv_usec + ru.ru_stime.tv_usec) / 1000.0;
}

/* the status line contract of proteusctl status, one packet per request */
static void status_line(char* buf, int n, const session_stats_t* st,
                        const char* det_state)
{
    snprintf(buf, (size_t)n,
             "port=%d det=%s ser=%.3f oct=%.3f moves_a=%ld moves_b=%ld "
             "moves_ab=%ld det_firings=%ld t_ms=%.0f rss_kb=%ld "
             "cpu_ms=%.0f\n",
             st->port, det_state, st->ser, st->oct, st->moves_a,
             st->moves_b, st->moves_ab, st->det_firings, st->t_ms,
             st->rss_kb, st->cpu_ms);
}

int session_run(const session_cfg_t* cfg, session_stats_t* stats)
{
    bridge_t br;
    schedule_t sc;
    motor_t mo;
    reflex_t rf;
    detector_t det;
    rate_t r_burst, r_legit;
    journal_t jr;
    ipc_t* ipc = NULL;
    double t_ms = 0.0;
    double last_tick_real = 0.0;
    int n_events = 0;
    int rc = 0;

    memset(stats, 0, sizeof(*stats));
    memset(&sc, 0, sizeof(sc));
    if (cfg->events_path && schedule_load(&sc, cfg->events_path) != 0) {
        fprintf(stderr, "events: cannot parse %s\n", cfg->events_path);
        return 1;
    }
    n_events = sc.n;
    if (bridge_init(&br, cfg->seed) != 0) {
        fprintf(stderr, "bridge: engine init failed\n");
        return 1;
    }
    if (motor_init(&mo) != 0) {
        fprintf(stderr, "motor: no free port in the window\n");
        bridge_free(&br);
        return 1;
    }
    if (cfg->socket_path && (ipc = ipc_init(cfg->socket_path,
                                            cfg->socket_group)) == NULL) {
        fprintf(stderr, "ipc: cannot bind %s\n", cfg->socket_path);
        motor_close(&mo);
        bridge_free(&br);
        return 1;
    }
    reflex_init(&rf);
    det_init(&det);
    rate_init(&r_burst);
    rate_init(&r_legit);
    journal_open(&jr, cfg->journal_out, cfg->seed, n_events);

    while (t_ms < cfg->duration_s * 1000.0 && rc == 0) {
        char line[JOURNAL_LINE_MAX];
        char stat[256];
        int ev = JEV_NONE;
        int cause = CAUSE_NONE;
        float ser, oct, dop, ava, avb;
        float density, share;
        struct timespec now;

        if (ipc || cfg->realtime)
            clock_gettime(CLOCK_MONOTONIC, &now);
        if (cfg->realtime) {
            /* pace the tick loop at one wall clock tick, the live ipc
               channel is only useful in this mode */
            double due = last_tick_real + (double)TICK_MS / 1000.0;
            double cur = now.tv_sec + now.tv_nsec * 1e-9;
            if (cur < due) {
                struct timespec slp;
                slp.tv_sec = 0;
                slp.tv_nsec = (long)((due - cur) * 1e9);
                nanosleep(&slp, NULL);
            }
        }
        if (ipc || cfg->realtime) {
            clock_gettime(CLOCK_MONOTONIC, &now);
            last_tick_real = now.tv_sec + now.tv_nsec * 1e-9;
        }

        while (sc.cursor < sc.n &&
               sc.ev[sc.cursor].t_ms < t_ms + TICK_MS) {
            event_t* e = &sc.ev[sc.cursor++];
            if (e->kind == EVK_BURST) {
                bridge_ash(&br, e->amp);
                rate_add(&r_burst, e->t_ms);
                if (!ev || ev < JEV_BURST)
                    ev = JEV_BURST;
            } else {
                rate_add(&r_legit, e->t_ms);
                if (!ev)
                    ev = JEV_LEGIT;
            }
        }
        bridge_steps(&br, TICK_MS);
        t_ms = bridge_time(&br);
        ava = bridge_ava(&br);
        avb = bridge_avb(&br);
        bridge_modulators(&br, &ser, &oct, &dop);

        /* modulator injection, r1: scan density lifts octopamine, the
         * legit share lifts serotonin. the engine kinetics pull the
         * concentrations back toward the world targets, that is the
         * decay tau; the clamp is the saturation */
        density = rate_hz(&r_burst, t_ms);
        {
            float legit_hz = rate_hz(&r_legit, t_ms);
            float total = density + legit_hz;
            share = total > 0.0f ? legit_hz / total : 0.0f;
        }
        oct += OCT_INJ_PER_HZ * density * ((float)TICK_MS / 1000.0f);
        ser += SER_INJ_PER_SHARE * share * ((float)TICK_MS / 1000.0f);
        if (oct > 1.0f)
            oct = 1.0f;
        if (ser > 1.0f)
            ser = 1.0f;
        br.sim->mod.oct = oct;
        br.sim->mod.ser = ser;

        /* detector state transitions become journal events */
        {
            int d = det_update(&det, ava, t_ms);
            if (d == 2) {
                ev = ev > JEV_DET_FIRED ? ev : JEV_DET_FIRED;
                cause |= CAUSE_DET;
                stats->det_firings++;
            } else if (d == 1) {
                ev = ev > JEV_DET_COOL ? ev : JEV_DET_COOL;
            } else if (d == 3) {
                ev = ev > JEV_DET_ARMED ? ev : JEV_DET_ARMED;
            }
        }

        if (reflex_update(&rf, ava, t_ms))
            cause |= CAUSE_ENDO;

        if (cause) {
            if (motor_move(&mo) < 0) {
                fprintf(stderr, "motor: window exhausted at t=%.2f\n",
                        t_ms);
                rc = 1;
                break;
            }
            if (cause == CAUSE_ENDO)
                stats->moves_a++;
            else if (cause == CAUSE_DET)
                stats->moves_b++;
            else
                stats->moves_ab++;
        }
        if (!isfinite(ava) || !isfinite(avb) || !isfinite(ser) ||
            !isfinite(oct) || !isfinite(dop) || !isfinite(det.score)) {
            fprintf(stderr, "journal: non finite state at t=%.2f\n", t_ms);
            rc = 2;
            break;
        }
        journal_line(line, JOURNAL_LINE_MAX, t_ms, ava, avb,
                     bridge_ash_trace(&br), ser, oct, dop, det.score,
                     mo.port, cause, ev);
        journal_emit(&jr, line);

        if (ipc) {
            stats->port = mo.port;
            stats->det_state = det.state;
            stats->ser = ser;
            stats->oct = oct;
            stats->rss_kb = rss_kb();
            stats->cpu_ms = cpu_ms();
            stats->t_ms = t_ms;
            status_line(stat, sizeof(stat), stats,
                        det_state_name(det.state));
            ipc_tick(ipc, stat, line);
        }
    }
    ipc_close(ipc);
    motor_close(&mo);
    bridge_free(&br);
    return rc;
}
