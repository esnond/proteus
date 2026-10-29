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

static double mono_s(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
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
    double acc_ms = 0.0;          /* wall time waiting to be simulated */
    double wall_stop = 0.0;
    int live;
    int pending = 0;              /* catch-up ticks owed by the last wake */
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
    if (motor_init(&mo, cfg->bind_addr, cfg->listen, cfg->seed) != 0) {
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

    /* a live daemon is event driven with a clamped accumulator, the
       game-loop pattern: wall time accumulates while the loop sleeps in
       poll and each wake steps the simulation once per TICK_MS of it,
       capped at TICK_CATCHUP_MAX steps. quiet time therefore runs at
       most TICK_CATCHUP_MAX ticks per IDLE_POLL_MS (idle cpu ~0.5%), and
       a burst still brings the empty ticks the detector statistics were
       calibrated on. `-d` is wall seconds in this mode; headless mode
       keeps sim seconds and stays bitwise replayable. */
    live = cfg->realtime && sn != NULL;
    if (live) {
        wall_stop = mono_s() + cfg->duration_s;
        last_tick_real = mono_s();
    }

    while (rc == 0 && (live ? mono_s() < wall_stop
                            : t_ms < cfg->duration_s * 1000.0)) {
        char line[JOURNAL_LINE_MAX];
        char stat[256];
        int ev = JEV_NONE;
        int cause = CAUSE_NONE;
        float ser, oct, dop, ava, avb;
        float density, share;
        double sec_ms;
        struct timespec now;

        if (ipc || cfg->realtime)
            clock_gettime(CLOCK_MONOTONIC, &now);
        if (pending > 0) {
            /* catch-up tick owed by the last wake: no poll, no pacing */
            pending--;
        } else if (live) {
            /* block until a peer, the control channel or the reaper
               deadline needs the loop */
            struct pollfd pf[1 + 1 + IPC_CLIENTS_MAX];
            int nfd = 0;
            double cur;
            if (mo.fd >= 0) {
                pf[nfd].fd = mo.fd;
                pf[nfd].events = POLLIN;
                pf[nfd].revents = 0;
                nfd++;
            }
            if (ipc)
                nfd += ipc_poll_fds(ipc, pf + nfd,
                                    (int)(sizeof(pf) / sizeof(pf[0])) - nfd);
            poll(pf, (nfds_t)nfd, IDLE_POLL_MS);
            clock_gettime(CLOCK_MONOTONIC, &now);
            cur = now.tv_sec + now.tv_nsec * 1e-9;
            acc_ms += (cur - last_tick_real) * 1000.0;
            last_tick_real = cur;
            /* the game-loop clamp: never spend more than
               TICK_CATCHUP_MAX ticks catching up, the excess wall time
               is dropped and the biological clock runs slow while quiet */
            if (acc_ms > (double)TICK_MS * (double)TICK_CATCHUP_MAX)
                acc_ms = (double)TICK_MS * (double)TICK_CATCHUP_MAX;
            pending = (int)(acc_ms / (double)TICK_MS);
            acc_ms -= (double)pending * (double)TICK_MS;
            trust_reap(tr, mono_s() * 1000.0);
        } else if (cfg->realtime) {
            /* realtime without a listener (scheduled events only) keeps
               the wall clock cadence */
            double due = last_tick_real + (double)TICK_MS / 1000.0;
            double cur = mono_s();
            if (cur < due) {
                double wait = due - cur;
                struct timespec slp;
                slp.tv_sec = (time_t)wait;
                slp.tv_nsec = (long)((wait - (double)slp.tv_sec) * 1e9);
                nanosleep(&slp, NULL);
            }
            clock_gettime(CLOCK_MONOTONIC, &now);
            last_tick_real = now.tv_sec + now.tv_nsec * 1e-9;
        } else if (ipc) {
            last_tick_real = now.tv_sec + now.tv_nsec * 1e-9;
        }

        /* immune scope timers run on the wall clock: bans, token ttl and
           the reaper are security properties and must not stretch with
           the simulation clock, which the accumulator slows while quiet.
           headless mode has no sensor and keeps sim time. */
        sec_ms = live ? mono_s() * 1000.0 : t_ms;

        if (sn) {
            int any_scan = 0, any_sess = 0;
            uint32_t auth_ban = 0;
            sensor_tick(sn, mo.fd, sec_ms, tr, &last_scan_ip, &any_scan,
                        &any_sess, &auth_ban);
            trust_reap(tr, sec_ms);
            if (auth_ban && !sensor_trusted(sn, auth_ban, sec_ms) &&
                sensor_is_banned(sn, auth_ban, sec_ms) == 0) {
                /* credential stuffing is standard security, not the
                   connectome path: the immune scope bans the source */
                ban_source(stats, nf, sn, auth_ban, sec_ms);
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

        /* detector state transitions become journal events; h10: the
         * matched filter reads avb, the only oscillating command cell
         * under the varshney wiring (ava is constitutively saturated) */
        {
            int d = det_update(&det, avb, t_ms);
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

        if (reflex_update(&rf, bridge_speed(&br), t_ms))
            cause |= CAUSE_ENDO;

        if ((cause & CAUSE_DET) && sn && last_scan_ip &&
            !sensor_trusted(sn, last_scan_ip, t_ms) &&
            sensor_is_banned(sn, last_scan_ip, t_ms) == 0) {
            ban_source(stats, nf, sn, last_scan_ip, sec_ms);
            ev = ev > JEV_BAN ? ev : JEV_BAN;
        }

        if (cause) {
            int mv;
            if (cfg->dest_mode == DEST_BIO)
                mv = motor_move_to(&mo, bridge_dest_index(&br));
            else if (cfg->dest_mode == DEST_RANDOM)
                mv = motor_move_to(&mo, motor_next_random(&mo));
            else
                mv = motor_move(&mo);
            if (mv < 0) {
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
            ipc_tick(ipc, tr, sec_ms, stat, line);
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
