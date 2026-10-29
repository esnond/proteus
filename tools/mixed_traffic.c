/* mixed_traffic: attacks and legitimate traffic from one process, the
   shape the detection experiments need: a scan stream plus short legit
   sessions interleaved on one timer loop. csv to stdout. */
#include <poll.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#include "tools_common.h"

#define HELLO "PROTEUS HELLO\n"
#define MAX_OPEN 16

typedef struct {
    int    fd;
    double deadline;
    int    port;
} sess_t;

int main(int argc, char** argv)
{
    const char* ip = "127.0.0.1";
    const char* src = NULL;
    const char* scan_src = NULL;
    const char* sock = NULL;
    int port = 40000, i;
    double duration = 10.0, scan_rate = 5.0, legit_rate = 1.0, t0;
    double next_scan, next_legit;
    sess_t open[MAX_OPEN];
    int n_open = 0;
    long scans = 0, sessions = 0;

    for (i = 1; i < argc; i++) {
        if (!strncmp(argv[i], "ip=", 3))
            ip = argv[i] + 3;
        else if (!strncmp(argv[i], "src=", 4))
            src = argv[i] + 4;
        else if (!strncmp(argv[i], "scan_src=", 9))
            scan_src = argv[i] + 9;
        else if (!strncmp(argv[i], "port=", 5))
            port = atoi(argv[i] + 5);
        else if (!strncmp(argv[i], "duration=", 9))
            duration = strtod(argv[i] + 9, NULL);
        else if (!strncmp(argv[i], "scan_rate=", 10))
            scan_rate = strtod(argv[i] + 10, NULL);
        else if (!strncmp(argv[i], "legit_rate=", 11))
            legit_rate = strtod(argv[i] + 11, NULL);
        else if (!strncmp(argv[i], "sock=", 5))
            sock = argv[i] + 5;
        else
            return args_usage(argv[0],
                              "ip,port,duration,scan_rate,legit_rate,sock,scan_src"), 1;
    }
    if (duration <= 0.0 || scan_rate < 0.0 || legit_rate < 0.0)
        return args_usage(argv[0],
                          "ip,port,duration,scan_rate,legit_rate,sock,scan_src"), 1;
    csv_header();
    t0 = now_ms_wall();
    next_scan = t0;
    next_legit = t0;
    while (now_ms_wall() - t0 < duration * 1000.0) {
        double now = now_ms_wall();
        if (scan_rate > 0.0 && now >= next_scan) {
            int fd = tcp_connect_src(ip, port, 300, scan_src);
            if (fd >= 0) {
                csv_row(t0, "scan", ip, port, "connected");
                close(fd);
            } else
                csv_row(t0, "scan", ip, port, "refused");
            scans++;
            next_scan = t0 + scans * 1000.0 / scan_rate;
        }
        if (legit_rate > 0.0 && now >= next_legit) {
            int lp = live_port(sock);
            int target = lp > 0 ? lp : port;
            sessions++;
            if (n_open < MAX_OPEN) {
                int fd = tcp_connect_src(ip, target, 300, src);
                if (fd >= 0) {
                    send(fd, HELLO, sizeof(HELLO) - 1, 0);
                    open[n_open].fd = fd;
                    open[n_open].deadline = now + 400.0;
                    open[n_open].port = target;
                    n_open++;
                    csv_row(t0, "session", ip, target, "data_sent");
                } else
                    csv_row(t0, "session", ip, target, "refused");
            }
            next_legit = t0 + sessions * 1000.0 / legit_rate;
        }
        for (i = n_open - 1; i >= 0; i--) {
            if (now_ms_wall() >= open[i].deadline) {
                close(open[i].fd);
                open[i] = open[n_open - 1];
                n_open--;
            }
        }
        {
            struct timespec slp;
            slp.tv_sec = 0;
            slp.tv_nsec = 2000000;   /* 2 ms loop */
            nanosleep(&slp, NULL);
        }
    }
    return 0;
}
