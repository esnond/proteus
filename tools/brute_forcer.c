#include <stdlib.h>
#include <string.h>
#include <time.h>
/* brute_forcer: ssh style credential stuffing. rapid short sessions
   with an auth payload and a wrong token, the shape of a brute force
   attack against the port the attacker already knows. sock= makes the
   stuffer follow the advertised port, an adaptive attacker; without it
   the run measures the move instead of the detector. csv to stdout. */
#include <sys/socket.h>
#include <unistd.h>

#include "tools_common.h"

#define AUTH "PROTEUS AUTH root wrong\n"

int main(int argc, char** argv)
{
    const char* ip = "127.0.0.1";
    const char* src = NULL;
    const char* sock = NULL;
    int port = 40000, attempts = 100;
    double rate = 20.0, t0;
    long n = 0;

    for (int i = 1; i < argc; i++) {
        if (!strncmp(argv[i], "ip=", 3))
            ip = argv[i] + 3;
        else if (!strncmp(argv[i], "src=", 4))
            src = argv[i] + 4;
        else if (!strncmp(argv[i], "sock=", 5))
            sock = argv[i] + 5;
        else if (!strncmp(argv[i], "port=", 5))
            port = atoi(argv[i] + 5);
        else if (!strncmp(argv[i], "attempts=", 9))
            attempts = atoi(argv[i] + 9);
        else if (!strncmp(argv[i], "rate=", 5))
            rate = strtod(argv[i] + 5, NULL);
        else
            return args_usage(argv[0], "ip,port,attempts,rate,sock"), 1;
    }
    if (attempts < 1 || rate <= 0.0)
        return args_usage(argv[0], "ip,port,attempts,rate,sock"), 1;
    csv_header();
    t0 = now_ms_wall();
    while (n < attempts) {
        int target = port;
        int fd;
        double due;
        if (sock) {
            int lp = live_port(sock);
            if (lp > 0)
                target = lp;
        }
        fd = tcp_connect_src(ip, target, 300, src);
        due = t0 + n * 1000.0 / rate;
        if (fd < 0) {
            csv_row(t0, "brute", ip, target, "refused");
        } else {
            send(fd, AUTH, sizeof(AUTH) - 1, 0);
            csv_row(t0, "brute", ip, target, "data_sent");
            close(fd);
        }
        n++;
        if (now_ms_wall() < due) {
            struct timespec slp;
            double wait = due - now_ms_wall();
            slp.tv_sec = (time_t)(wait / 1000.0);
            slp.tv_nsec = (long)((wait - (double)slp.tv_sec * 1000.0) * 1e6);
            nanosleep(&slp, NULL);
        }
    }
    return 0;
}
