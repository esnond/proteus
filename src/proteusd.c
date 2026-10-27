/* proteusd: the defense daemon, r0 shape. one connectome, one moving
   listener, one party journal. events arrive from a schedule file; the
   live wire body (nftables, real peers) lands at r2 behind the same
   sensorimotor tables. exit codes: 0 ok, 1 usage or init, 2 non finite. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bridge.h"
#include "config.h"
#include "events.h"
#include "journal.h"
#include "motor.h"
#include "reflex.h"

static void usage(FILE* out)
{
    fprintf(out,
        "proteusd - bio stochastic moving defense, r0\n"
        "  -s SEED      noise realization, same seed same bytes\n"
        "  -d SECONDS   simulated session length, default 10\n"
        "  -e FILE      event schedule csv (t_ms,kind,amp), optional\n"
        "  -o FILE      party journal, default stdout\n");
}

static int run_session(uint64_t seed, double duration_s,
                       const char* events_path, FILE* out)
{
    bridge_t br;
    schedule_t sc;
    motor_t mo;
    reflex_t rf;
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
    journal_open(&jr, out, seed, n_events);

    while (t_ms < duration_s * 1000.0 && rc == 0) {
        int fired = 0;
        int code = 0;
        float amp = schedule_due(&sc, t_ms, t_ms + TICK_MS, &fired);
        float ser, oct, dop, ava, avb;

        if (fired) {
            bridge_ash(&br, amp);
            code = 1;
        }
        bridge_steps(&br, TICK_MS);
        t_ms = bridge_time(&br);
        ava = bridge_ava(&br);
        avb = bridge_avb(&br);
        bridge_modulators(&br, &ser, &oct, &dop);
        if (reflex_update(&rf, ava, t_ms)) {
            if (motor_move(&mo) < 0) {
                fprintf(stderr, "motor: window exhausted at t=%.2f\n", t_ms);
                rc = 1;
                break;
            }
            code = code ? 3 : 2;
        }
        if (!isfinite(ava) || !isfinite(avb) || !isfinite(ser) ||
            !isfinite(oct) || !isfinite(dop)) {
            fprintf(stderr, "journal: non finite state at t=%.2f\n", t_ms);
            rc = 2;
            break;
        }
        journal_row(&jr, t_ms, ava, avb, bridge_ash_trace(&br),
                    ser, oct, dop, mo.port, code);
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
