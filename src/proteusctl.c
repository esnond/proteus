/* proteusctl: control and measurement cli. r0 ships the reflex gate
   statistic: paired ava response of burst runs against baseline runs over
   ten seeds, the r0 exit criterion. the gate decision (count_pos >= 8)
   belongs to the test script, the tool only measures. */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#define JROWS 200000

/* student t, two sided 95 percent, df 9, fixed by the ten seed gate */
#define T95_DF9 2.262

typedef struct {
    double   t_ms[JROWS];
    float    ava[JROWS];
    int      event[JROWS];
    int      n;
    uint64_t seed;
} rec_t;

static int load_rec(rec_t* r, const char* path)
{
    FILE* f = fopen(path, "r");
    char line[256];

    memset(r, 0, sizeof(*r));
    if (!f)
        return -1;
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#') {
            const char* s = strstr(line, "seed=");
            if (s)
                r->seed = strtoull(s + 5, NULL, 10);
            continue;
        }
        if (r->n >= JROWS) {
            fclose(f);
            return -1;
        }
        {
            double t;
            float ava;
            int port, ev;
            if (sscanf(line, "%lf,%f,%*f,%*f,%*f,%*f,%*f,%d,%d",
                       &t, &ava, &port, &ev) != 4) {
                fclose(f);
                return -1;
            }
            r->t_ms[r->n] = t;
            r->ava[r->n] = ava;
            r->event[r->n] = ev;
            r->n++;
        }
    }
    fclose(f);
    return 0;
}

/* mean act_ava over [t0, t0 + window], -1 when the record does not cover it */
static double window_mean(const rec_t* r, double t0)
{
    double sum = 0.0;
    int n = 0, i;
    for (i = 0; i < r->n; i++) {
        if (r->t_ms[i] >= t0 && r->t_ms[i] <= t0 + (double)RESP_WINDOW_MS) {
            sum += r->ava[i];
            n++;
        }
    }
    return n ? sum / n : -1.0;
}

static double burst_onset(const rec_t* r)
{
    int i;
    for (i = 0; i < r->n; i++)
        if (r->event[i] >= 1)
            return r->t_ms[i];
    return -1.0;
}

int main(int argc, char** argv)
{
    static double delta[10];
    int pairs, i, count_pos = 0;
    double mean, var = 0.0, sd, dz, half;

    if (argc < 2 || strcmp(argv[1], "reflex") != 0 || argc != 22 ||
        (argc - 2) / 2 != 10) {
        fprintf(stderr,
                "usage: proteusctl reflex BASE1 BURST1 ... BASE10 BURST10\n"
                "  ten baseline and burst journal pairs, the r0 gate\n");
        return 1;
    }
    pairs = 10;
    printf("# proteus reflex v1 window_ms=%d\n", RESP_WINDOW_MS);
    printf("# seed,delta\n");
    for (i = 0; i < pairs; i++) {
        static rec_t base, burst;
        double t0, mb, mr;
        if (load_rec(&base, argv[2 + 2 * i]) != 0 ||
            load_rec(&burst, argv[3 + 2 * i]) != 0) {
            fprintf(stderr, "proteusctl: cannot parse pair %d\n", i + 1);
            return 1;
        }
        t0 = burst_onset(&burst);
        if (t0 < 0.0) {
            fprintf(stderr, "proteusctl: burst %d fired no event\n", i + 1);
            return 1;
        }
        mb = window_mean(&base, t0);
        mr = window_mean(&burst, t0);
        if (mb < 0.0 || mr < 0.0) {
            fprintf(stderr, "proteusctl: pair %d does not cover the window\n",
                    i + 1);
            return 1;
        }
        delta[i] = mr - mb;
        if (delta[i] > 0.0)
            count_pos++;
        printf("%llu,%.4f\n", (unsigned long long)burst.seed, delta[i]);
    }
    mean = 0.0;
    for (i = 0; i < pairs; i++)
        mean += delta[i];
    mean /= pairs;
    for (i = 0; i < pairs; i++)
        var += (delta[i] - mean) * (delta[i] - mean);
    sd = var / (pairs - 1);
    sd = sd > 0.0 ? sqrt(sd) : 0.0;
    dz = sd > 0.0 ? mean / sd : 0.0;
    half = T95_DF9 * sd / 3.16227766;   /* sqrt(10) */
    printf("summary,n=%d,count_pos=%d,mean_delta=%.4f,ci95_lo=%.4f,"
           "ci95_hi=%.4f,dz=%.2f\n",
           pairs, count_pos, mean, mean - half, mean + half, dz);
    return 0;
}
