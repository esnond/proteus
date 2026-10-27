#include <stdio.h>
#include <string.h>

#include "events.h"
#include "config.h"

int schedule_load(schedule_t* sc, const char* path)
{
    FILE* f = fopen(path, "r");
    char line[256];

    memset(sc, 0, sizeof(*sc));
    if (!f)
        return -1;
    while (fgets(line, sizeof(line), f)) {
        char kind[16];
        double t;
        float amp;
        if (line[0] == '#' || line[0] == '\n')
            continue;
        if (sc->n >= EVENTS_MAX) {
            fclose(f);
            return -1;
        }
        if (sscanf(line, "%lf,%15[^,],%f", &t, kind, &amp) != 3) {
            fclose(f);
            return -1;
        }
        if (strcmp(kind, "burst") != 0 || amp < 0.0f || amp > ASH_AMP_MAX) {
            fclose(f);
            return -1;
        }
        if (sc->n > 0 && t < sc->ev[sc->n - 1].t_ms) {
            fclose(f);
            return -1;
        }
        sc->ev[sc->n].t_ms = t;
        sc->ev[sc->n].amp = amp;
        sc->n++;
    }
    fclose(f);
    return 0;
}

float schedule_due(schedule_t* sc, double t_lo, double t_hi, int* fired)
{
    float amp = 0.0f;

    *fired = 0;
    while (sc->cursor < sc->n && sc->ev[sc->cursor].t_ms < t_hi) {
        if (sc->ev[sc->cursor].t_ms >= t_lo) {
            amp = sc->ev[sc->cursor].amp;
            *fired = 1;
        }
        sc->cursor++;
    }
    return amp;
}
