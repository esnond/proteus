#include <errno.h>
#include <math.h>
#include <poll.h>
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
#include "nft.h"
#include "seccomp.h"
#include "sensor.h"
#include "trust.h"
#include "session.h"

/* journal event codes, see docs/schemas.md */
enum {
    JEV_NONE = 0, JEV_BURST, JEV_LEGIT,
    JEV_DET_ARMED, JEV_DET_FIRED, JEV_DET_COOL,
    JEV_NET_SCAN, JEV_NET_SESSION, JEV_BAN
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
             "moves_ab=%ld det_firings=%ld scans=%ld sessions=%ld "
             "bans=%ld nft=%d t_ms=%.0f rss_kb=%ld cpu_ms=%.0f trust=%ld\n",
             st->port, det_state, st->ser, st->oct, st->moves_a,
             st->moves_b, st->moves_ab, st->det_firings, st->net_scans,
             st->net_sessions, st->bans, st->nft_up, st->t_ms,
             st->rss_kb,
             st->cpu_ms, st->trust);
}

/* the one place that mirrors a ban into userspace and the kernel: the
   set is the visible record, the drop rule is the enforcement */
static void ban_source(session_stats_t* st, nft_t* nf, sensor_t* sn,
                       uint32_t ip, double t_ms)
{
    sensor_ban(sn, ip, t_ms);
    if (nf) {
        if (nft_set_ban(nf, NFT_TABLE_NAME, NFT_SET_BANNED, ip) != 0 ||
            nft_rule_add_drop(nf, NFT_TABLE_NAME, NFT_CHAIN_IN, ip, 0) != 0) {
            fprintf(stderr, "nft: queue failed at t=%.2f\n", t_ms);
        } else if (nft_commit(nf) != 0) {
            fprintf(stderr, "nft: commit failed at t=%.2f: %s\n",
                    t_ms, strerror(errno));
        }
    }
    st->bans++;
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
    sensor_t* sn = NULL;
    trust_t* tr = NULL;
    nft_t* nf = NULL;
    uint32_t last_scan_ip = 0;
    double t_ms = 0.0;
    double last_tick_real = 0.0;
    double last_net_wall = 0.0;   /* 0: quiet since start */
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
    if (motor_init(&mo, cfg->bind_addr, cfg->listen) != 0) {
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
    if (cfg->listen && (sn = sensor_init()) == NULL) {
        fprintf(stderr, "sensor: init failed\n");
        rc = 1;
    }
    tr = trust_init();
    if (!tr) {
        fprintf(stderr, "trust: init failed\n");
        rc = 1;
    }
    if (cfg->nft) {
        /* the real firewall backend; without cap_net_admin it fails and
           the daemon says so, the userspace ban list carries on */
        const char* step = "open";
        int bad = 0;
        nf = nft_open();
        if (nf == NULL) {
            bad = 1;
        } else if (nft_table_create(nf, NFT_TABLE_NAME) != 0) {
            step = "table";
            bad = 1;
        } else if (nft_chain_add(nf, NFT_TABLE_NAME, NFT_CHAIN_PRE,
                                 P_NF_INET_PRE_ROUTING, -300,
                                 P_NF_ACCEPT) != 0) {
            step = "chain prerouting";
            bad = 1;
        } else if (nft_chain_add(nf, NFT_TABLE_NAME, NFT_CHAIN_TRUST,
                                 P_NF_INET_LOCAL_IN, -300,
                                 P_NF_ACCEPT) != 0) {
            step = "chain trust";
            bad = 1;
        } else if (nft_chain_add(nf, NFT_TABLE_NAME, NFT_CHAIN_IN,
                                 P_NF_INET_LOCAL_IN, 0,
                                 P_NF_ACCEPT) != 0) {
            step = "chain input";
            bad = 1;
        } else if (nft_set_add(nf, NFT_TABLE_NAME, NFT_SET_BANNED) != 0) {
            step = "set";
            bad = 1;
        } else if (nft_rule_add_mark_accept(nf, NFT_TABLE_NAME,
                                            NFT_CHAIN_TRUST,
                                            TRUST_MARK) != 0) {
            step = "mark rule";
            bad = 1;
        } else if (nft_commit(nf) != 0) {
            step = "commit";
            bad = 1;
        }
        if (bad) {
            fprintf(stderr, "nft: backend unavailable at %s (%s), "
                    "userspace bans only\n", step, strerror(errno));
            nft_close(nf);
            nf = NULL;
        }
    }
    stats->nft_up = nf != NULL;
    if (cfg->seccomp && seccomp_install() != 0) {
        fprintf(stderr, "seccomp: refused to run without a filter\n");
        nft_close(nf);
        sensor_close(sn);
        trust_close(tr);
        motor_close(&mo);
        bridge_free(&br);
        return 1;
    }

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
            /* pace the tick loop at one wall clock tick. while the network
               has been quiet for IDLE_WINDOW_MS the period stretches by
               idle_divisor: the biological clock slows down (documented
               trade-off), but the listener stays in poll so a SYN wakes
               the loop at once, not after the stretched period */
            double cur = now.tv_sec + now.tv_nsec * 1e-9;
            int idle = cfg->idle_divisor > 1 &&
                       (cur - last_net_wall) * 1000.0 >= IDLE_WINDOW_MS;
            double period = (double)TICK_MS * (idle ? cfg->idle_divisor : 1)
                            / 1000.0;
            double due = last_tick_real + period;
            if (cur < due) {
                double wait = due - cur;
                if (idle && mo.fd >= 0) {
                    struct pollfd pf;
                    pf.fd = mo.fd;
                    pf.events = POLLIN;
                    pf.revents = 0;
                    poll(&pf, 1, (int)(wait * 1000.0));
                } else {
                    struct timespec slp;
                    slp.tv_sec = (time_t)wait;
                    slp.tv_nsec = (long)((wait - (double)slp.tv_sec) * 1e9);
                    nanosleep(&slp, NULL);
                }
            }
        }
        if (ipc || cfg->realtime) {
            clock_gettime(CLOCK_MONOTONIC, &now);
            last_tick_real = now.tv_sec + now.tv_nsec * 1e-9;
        }

        if (sn) {
            int any_scan = 0, any_sess = 0;
            uint32_t auth_ban = 0;
            sensor_tick(sn, mo.fd, t_ms, tr, &last_scan_ip, &any_scan,
                        &any_sess, &auth_ban);
            trust_reap(tr, t_ms);
            if (auth_ban && !sensor_trusted(sn, auth_ban, t_ms) &&
                sensor_is_banned(sn, auth_ban, t_ms) == 0) {
                /* credential stuffing is standard security, not the
                   connectome path: the immune scope bans the source */
                ban_source(stats, nf, sn, auth_ban, t_ms);
                ev = ev > JEV_BAN ? ev : JEV_BAN;
            }
            if (any_scan) {
                /* a real scan shaped connection pushes the nociceptor
                   and the density counter, same channels as r0 events */
                bridge_ash(&br, 1.0f);
                rate_add(&r_burst, t_ms);
                stats->net_scans++;
                ev = ev > JEV_NET_SCAN ? ev : JEV_NET_SCAN;
            }
            if (any_sess) {
                rate_add(&r_legit, t_ms);
                stats->net_sessions++;
                ev = ev > JEV_NET_SESSION ? ev : JEV_NET_SESSION;
            }
            if (any_scan || any_sess) {
                struct timespec nw;
                clock_gettime(CLOCK_MONOTONIC, &nw);
                last_net_wall = nw.tv_sec + nw.tv_nsec * 1e-9;
            }
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

        if ((cause & CAUSE_DET) && sn && last_scan_ip &&
            !sensor_trusted(sn, last_scan_ip, t_ms) &&
            sensor_is_banned(sn, last_scan_ip, t_ms) == 0) {
            ban_source(stats, nf, sn, last_scan_ip, t_ms);
            ev = ev > JEV_BAN ? ev : JEV_BAN;
        }

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
            stats->trust = trust_active(tr);
            stats->det_state = det.state;
            stats->ser = ser;
            stats->oct = oct;
            stats->rss_kb = rss_kb();
            stats->cpu_ms = cpu_ms();
            stats->t_ms = t_ms;
            status_line(stat, sizeof(stat), stats,
                        det_state_name(det.state));
            ipc_tick(ipc, tr, t_ms, stat, line);
        }
    }
    sensor_close(sn);
    trust_close(tr);
    nft_close(nf);
    ipc_close(ipc);
    motor_close(&mo);
    bridge_free(&br);
    return rc;
}
