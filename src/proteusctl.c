/* proteusctl: the measurement and control cli, r1. four subcommands:
   status, log, party, detector. party replays a session in this process
   and verifies a saved journal bitwise; detector reports the frozen
   threshold and the detection or false alarm side of given journals. */
#define _GNU_SOURCE
#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "config.h"
#include "session.h"

#define JROWS 400000
#define DEFAULT_SOCK "/tmp/proteusd.sock"
#define RECV_BUF 256

static void usage(FILE* out)
{
    fprintf(out,
        "proteusctl - control and measurement, r1\n"
        "  status  [--socket P]                 one status packet\n"
        "  log     [--quiet|--json] [--socket P] live journal stream\n"
        "  party SEED [-d SEC] [-e SCHED] --against JOURNAL\n"
        "                                       bitwise replay check\n"
        "  detector FA1..N BURST1..M             fa rate and detections\n");
}

/* ---- ipc plumbing ---- */

static int connect_sock(const char* path)
{
    struct sockaddr_un sa;
    int fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (fd < 0)
        return -1;
    memset(&sa, 0, sizeof(sa));
    sa.sun_family = AF_UNIX;
    snprintf(sa.sun_path, sizeof(sa.sun_path), "%s", path);
    if (connect(fd, (struct sockaddr*)&sa, sizeof(sa)) != 0) {
        close(fd);
        return -1;
    }
    return fd;
}

static int cmd_status(const char* path)
{
    char buf[RECV_BUF];
    int fd = connect_sock(path);
    ssize_t n;
    if (fd < 0) {
        fprintf(stderr, "proteusctl: cannot connect %s: %s\n", path,
                strerror(errno));
        return 1;
    }
    if (send(fd, "status", 6, 0) < 0) {
        fprintf(stderr, "proteusctl: send failed\n");
        close(fd);
        return 1;
    }
    n = recv(fd, buf, sizeof(buf) - 1, 0);
    if (n <= 0) {
        fprintf(stderr, "proteusctl: no status packet\n");
        close(fd);
        return 1;
    }
    buf[n] = '\0';
    fputs(buf, stdout);
    close(fd);
    return 0;
}

static int cmd_log(const char* path, const char* mode)
{
    char buf[RECV_BUF];
    char cmd[32];
    int fd = connect_sock(path);
    ssize_t n;
    if (fd < 0) {
        fprintf(stderr, "proteusctl: cannot connect %s: %s\n", path,
                strerror(errno));
        return 1;
    }
    snprintf(cmd, sizeof(cmd), "log%s", mode);
    if (send(fd, cmd, strlen(cmd), 0) < 0) {
        fprintf(stderr, "proteusctl: send failed\n");
        close(fd);
        return 1;
    }
    for (;;) {
        n = recv(fd, buf, sizeof(buf) - 1, 0);
        if (n <= 0)
            break;
        buf[n] = '\0';
        fputs(buf, stdout);
        fflush(stdout);
    }
    close(fd);
    return 0;
}

/* ---- party: bitwise replay ---- */

static int read_file(const char* path, char** out, size_t* len)
{
    FILE* f = fopen(path, "rb");
    size_t cap = 1 << 16, n = 0;
    char* buf;
    if (!f)
        return -1;
    buf = malloc(cap);
    if (!buf) {
        fclose(f);
        return -1;
    }
    for (;;) {
        size_t r;
        if (n == cap) {
            char* nb;
            cap *= 2;
            nb = realloc(buf, cap);
            if (!nb) {
                free(buf);
                fclose(f);
                return -1;
            }
            buf = nb;
        }
        r = fread(buf + n, 1, cap - n, f);
        if (r == 0)
            break;
        n += r;
    }
    fclose(f);
    *out = buf;
    *len = n;
    return 0;
}

static int cmd_party(int argc, char** argv)
{
    session_cfg_t cfg;
    session_stats_t stats;
    const char* against = NULL;
    const char* sched = NULL;
    double duration = 0.0;
    uint64_t seed;
    char* want = NULL;
    char* got = NULL;
    size_t want_len = 0, got_len = 0, got_cap = 1 << 16;
    FILE* tmp = tmpfile();
    long i, at = -1;
    int rc = 0;

    memset(&cfg, 0, sizeof(cfg));
    seed = strtoull(argv[0], NULL, 10);
    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-d") && i + 1 < argc)
            duration = strtod(argv[++i], NULL);
        else if (!strcmp(argv[i], "-e") && i + 1 < argc)
            sched = argv[++i];
        else if (!strcmp(argv[i], "--against") && i + 1 < argc)
            against = argv[++i];
        else
            return usage(stderr), 1;
    }
    if (!against || !tmp)
        return usage(stderr), 1;
    cfg.seed = seed;
    cfg.duration_s = duration > 0.0 ? duration : 10.0;
    cfg.events_path = sched;
    cfg.journal_out = tmp;
    if (session_run(&cfg, &stats) != 0) {
        fprintf(stderr, "proteusctl: replay failed\n");
        fclose(tmp);
        return 1;
    }
    fflush(tmp);
    if (read_file(against, &want, &want_len) != 0) {
        fprintf(stderr, "proteusctl: cannot read %s\n", against);
        fclose(tmp);
        return 1;
    }
    got = malloc(got_cap);
    if (!got) {
        fclose(tmp);
        return 1;
    }
    rewind(tmp);
    for (;;) {
        size_t r = fread(got + got_len, 1, got_cap - got_len, tmp);
        got_len += r;
        if (got_len < got_cap)
            break;
        {
            char* nb = realloc(got, got_cap * 2);
            if (!nb) {
                rc = 1;
                break;
            }
            got_cap *= 2;
            got = nb;
        }
    }
    fclose(tmp);
    for (i = 0;
         i < (long)(want_len < got_len ? want_len : got_len); i++) {
        if (want[i] != got[i]) {
            at = i;
            break;
        }
    }
    if (at < 0 && want_len != got_len)
        at = (long)(want_len < got_len ? want_len : got_len);
    if (at < 0)
        printf("party seed %llu: identical, %zu bytes\n",
               (unsigned long long)seed, want_len);
    else {
        printf("party seed %llu: DIFFER at byte %ld\n",
               (unsigned long long)seed, at);
        rc = 1;
    }
    free(want);
    free(got);
    return rc;
}

/* ---- detector: frozen threshold, fa and detection side ---- */

#define JLINE_MAX 256

typedef struct {
    double t_ms;
    float  score;
    int    event;
} row_t;

static int load_rows(const char* path, row_t* rows)
{
    FILE* f = fopen(path, "r");
    char line[JLINE_MAX];
    int n = 0;
    if (!f)
        return -1;
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#')
            continue;
        if (n >= JROWS) {
            fclose(f);
            return -1;
        }
        if (sscanf(line, "%lf,%*f,%*f,%*f,%*f,%*f,%*f,%f,%*d,%*d,%d",
                   &rows[n].t_ms, &rows[n].score, &rows[n].event) != 3) {
            fclose(f);
            return -1;
        }
        n++;
    }
    fclose(f);
    return n;
}

/* replay the detector state machine from the score column, mirrors
   src/detector.c: cool window then re arm */
static long det_fires(const row_t* rows, int n, double t0, double t1)
{
    long fires = 0;
    int armed = 1;
    double cool_until = -1.0;
    int i;
    for (i = 0; i < n; i++) {
        if (rows[i].t_ms > t1)
            break;
        if (armed) {
            if (rows[i].score >= (float)DET_THRESHOLD) {
                fires++;
                armed = 0;
                cool_until = rows[i].t_ms + (double)DET_COOL_MS;
                if (fires == 1 && t0 < 0.0)
                    t0 = rows[i].t_ms;
            }
        } else if (rows[i].t_ms >= cool_until) {
            armed = 1;
        }
        if (t0 >= 0.0 && rows[i].t_ms > t1)
            break;
    }
    return fires;
}

static int cmd_detector(int argc, char** argv)
{
    static row_t rows[JROWS];
    long fa_total = 0, fa_files = 0, det_pos = 0, det_files = 0;
    double fa_time = 0.0;
    int i;

    printf("threshold=%.1f cool_ms=%d window_ms=%d\n", DET_THRESHOLD,
           DET_COOL_MS, DET_WINDOW_MS);
    for (i = 0; i < argc; i++) {
        int n = load_rows(argv[i], rows);
        int has_events = 0;
        long fires;
        double t0 = -1.0, t_last;
        if (n <= 0) {
            fprintf(stderr, "proteusctl: cannot parse %s\n", argv[i]);
            return 1;
        }
        t_last = rows[n - 1].t_ms;
        for (int j = 0; j < n; j++)
            if (rows[j].event == 1 || rows[j].event == 2) {
                has_events = 1;
                t0 = rows[j].t_ms;
                break;
            }
        if (!has_events) {
            /* a withheld seed journal: false alarm accounting */
            fires = det_fires(rows, n, -1.0, t_last);
            fa_total += fires;
            fa_files++;
            fa_time += t_last;
            printf("fa %s: %ld firings\n", argv[i], fires);
        } else {
            fires = det_fires(rows, n, t0, t0 + 3000.0);
            det_pos += fires > 0 ? 1 : 0;
            det_files++;
            printf("burst %s: %s (%ld firings in the response window)\n",
                   argv[i], fires > 0 ? "detected" : "missed", fires);
        }
    }
    if (fa_files)
        printf("fa_total=%ld fa_rate_per_600s=%.2f over %.0f s\n",
               fa_total, fa_total / fa_time * 600.0, fa_time);
    if (det_files)
        printf("detected=%ld/%ld\n", det_pos, det_files);
    return 0;
}

int main(int argc, char** argv)
{
    const char* sock = DEFAULT_SOCK;

    if (argc < 2) {
        usage(stderr);
        return 1;
    }
    if (!strcmp(argv[1], "status")) {
        int i;
        for (i = 2; i < argc; i++)
            if (!strcmp(argv[i], "--socket") && i + 1 < argc)
                sock = argv[++i];
            else
                return usage(stderr), 1;
        return cmd_status(sock);
    }
    if (!strcmp(argv[1], "log")) {
        const char* mode = "";
        int i;
        for (i = 2; i < argc; i++) {
            if (!strcmp(argv[i], "--socket") && i + 1 < argc)
                sock = argv[++i];
            else if (!strcmp(argv[i], "--quiet"))
                mode = " quiet";
            else if (!strcmp(argv[i], "--json"))
                mode = " json";
            else
                return usage(stderr), 1;
        }
        return cmd_log(sock, mode);
    }
    if (!strcmp(argv[1], "party"))
        return cmd_party(argc - 2, argv + 2);
    if (!strcmp(argv[1], "detector"))
        return cmd_detector(argc - 2, argv + 2);
    usage(stderr);
    return 1;
}
