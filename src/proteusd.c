/* proteusd: the defense daemon, r1 shape. one connectome, one moving
   listener, one party journal. events arrive from a schedule file; the
   live wire body (nftables, real peers) lands at r2 behind the same
   sensorimotor tables. the threat detector is a matched filter over the
   ava trace, the port move joins the endogenous reversal and the
   detector firing, modulator reactivity lands in this stage too.
   exit codes: 0 ok, 1 usage or init, 2 non finite. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bridge.h"
#include "config.h"
#include "detector.h"
#include "events.h"
#include "journal.h"
#include "motor.h"
#include "rate.h"
#include "reflex.h"

static void usage(FILE* out)
{
    fprintf(out,
        "proteusd - bio stochastic moving defense, r1\n"
        "  -s SEED      noise realization, same seed same bytes\n"
        "  -d SECONDS   simulated session length, default 10\n"
        "  -e FILE      event schedule csv (t_ms,kind,amp), optional\n"
        "  -o FILE      party journal, default stdout\n");
}

/* journal event codes, see docs/schemas.md */
enum {
    JEV_NONE = 0, JEV_BURST, JEV_LEGIT,
    JEV_DET_ARMED, JEV_DET_FIRED, JEV_DET_COOL
};
enum { CAUSE_NONE = 0, CAUSE_ENDO = 1, CAUSE_DET = 2 };

static int run_session(uint64_t seed, double duration_s,
                       const char* events_path, FILE* out)
{
    bridge_t br;
    schedule_t sc;
    motor_t mo;
    reflex_t rf;
    detector_t det;
    rate_t r_burst, r_legit;
    journal_t jr;
    double t_ms = 0.0;
    int n_events = 0;
    int rc = 0;

    memset(&sc, 0, sizeof(sc));
    if (events_path && schedule_load(&sc, events_path) != 0) {
        fprintf(stderr, "events: cannot parse %s\n", events_path);
        return 1;
    }
    n_events = sc.n;
    if (bridge_init(&br, seed) != 0) {
        fprintf(stderr, "bridge: engine init failed\n");
        return 1;
    }
    if (motor_init(&mo) != 0) {
        fprintf(stderr, "motor: no free port in the window\n");
        bridge_free(&br);
        return 1;
    }
    reflex_init(&rf);
    det_init(&det);
    rate_init(&r_burst);
    rate_init(&r_legit);
    journal_open(&jr, out, seed, n_events);

    while (t_ms < duration_s * 1000.0 && rc == 0) {
        int ev = JEV_NONE;
        int cause = CAUSE_NONE;
        float ser, oct, dop, ava, avb;
        float density, share;

        while (sc.cursor < sc.n && sc.ev[sc.cursor].t_ms < t_ms + TICK_MS) {
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
            } else if (d == 1) {
                ev = ev > JEV_DET_COOL ? ev : JEV_DET_COOL;
            } else if (d == 3) {
                ev = ev > JEV_DET_ARMED ? ev : JEV_DET_ARMED;
            }
        }

        if (reflex_update(&rf, ava, t_ms))
            cause |= CAUSE_ENDO;

        if (cause && motor_move(&mo) < 0) {
            fprintf(stderr, "motor: window exhausted at t=%.2f\n", t_ms);
            rc = 1;
            break;
        }
        if (!isfinite(ava) || !isfinite(avb) || !isfinite(ser) ||
            !isfinite(oct) || !isfinite(dop) || !isfinite(det.score)) {
            fprintf(stderr, "journal: non finite state at t=%.2f\n", t_ms);
            rc = 2;
            break;
        }
        journal_row(&jr, t_ms, ava, avb, bridge_ash_trace(&br),
                    ser, oct, dop, det.score, mo.port, cause, ev);
    }
    motor_close(&mo);
    bridge_free(&br);
    return rc;
}

int main(int argc, char** argv)
{
    uint64_t seed = 1;
    double duration_s = 10.0;
    const char* events_path = NULL;
    const char* out_path = NULL;
    FILE* out = stdout;
    int i, rc;

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-s") && i + 1 < argc)
            seed = strtoull(argv[++i], NULL, 10);
        else if (!strcmp(argv[i], "-d") && i + 1 < argc)
            duration_s = strtod(argv[++i], NULL);
        else if (!strcmp(argv[i], "-e") && i + 1 < argc)
            events_path = argv[++i];
        else if (!strcmp(argv[i], "-o") && i + 1 < argc)
            out_path = argv[++i];
        else {
            usage(stderr);
            return 1;
        }
    }
    if (duration_s <= 0.0) {
        fprintf(stderr, "proteusd: duration must be positive\n");
        return 1;
    }
    if (out_path) {
        out = fopen(out_path, "w");
        if (!out) {
            fprintf(stderr, "proteusd: cannot write %s\n", out_path);
            return 1;
        }
    }
    rc = run_session(seed, duration_s, events_path, out);
    if (out != stdout)
        fclose(out);
    return rc;
}
