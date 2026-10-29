#include <stdlib.h>
#include <string.h>
#include <time.h>
/* scanner: a port scan generator. connects to the target ports and
   closes immediately without sending anything, the shape a syn sweep of
   the proteus window has from the daemon side. csv to stdout. */
#include <unistd.h>

#include "tools_common.h"

int main(int argc, char** argv)
{
    const char* ip = "127.0.0.1";
    const char* src = NULL;
    int first = 40000, count = 64;
    double rate = 10.0, duration = 10.0, t0;
    long sent = 0;
    int i;

    for (i = 1; i < argc; i++) {
        if (!strncmp(argv[i], "ip=", 3))
            ip = argv[i] + 3;
        else if (!strncmp(argv[i], "src=", 4))
            src = argv[i] + 4;
        else if (!strncmp(argv[i], "first_port=", 11))
            first = atoi(argv[i] + 11);
        else if (!strncmp(argv[i], "ports=", 6))
            count = atoi(argv[i] + 6);
        else if (!strncmp(argv[i], "rate=", 5))
            rate = strtod(argv[i] + 5, NULL);
        else if (!strncmp(argv[i], "duration=", 9))
            duration = strtod(argv[i] + 9, NULL);
        else
            return args_usage(argv[0], "ip,first_port,ports,rate,duration"),
                   1;
    }
    if (count < 1 || rate <= 0.0 || duration <= 0.0)
        return args_usage(argv[0], "ip,first_port,ports,rate,duration"), 1;
    csv_header();
    t0 = now_ms_wall();
    while (now_ms_wall() - t0 < duration * 1000.0) {
        int port = first + (int)(sent % (long)count);
        int fd = tcp_connect_src(ip, port, 300, src);
        double due = t0 + sent * 1000.0 / rate;
        if (fd >= 0) {
            csv_row(t0, "scan", ip, port, "connected");
            close(fd);
        } else
            csv_row(t0, "scan", ip, port, "refused");
        sent++;
        if (now_ms_wall() < due) {
            struct timespec slp;
            double wait = due - now_ms_wall();
            slp.tv_sec = 0;
            slp.tv_nsec = (long)(wait * 1e6);
            nanosleep(&slp, NULL);
        }
    }
    return 0;
}
