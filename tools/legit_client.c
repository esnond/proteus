#include <stdlib.h>
#include <string.h>
#include <time.h>
/* legit_client: a legitimate session generator. connects, sends the
   hello line, holds the session for the requested time, closes. the
   daemon reads the payload and reads food, the csv records every
   session. */
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include "tools_common.h"

#define HELLO "PROTEUS HELLO\n"

/* the service registers itself: the token is bound to this process, so a
   short lived helper cannot register on its behalf */
static int self_register(const char* path, char* tok, size_t tok_n)
{
    struct sockaddr_un sa;
    char reply[128];
    int fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    ssize_t n;
    if (fd < 0)
        return 0;
    memset(&sa, 0, sizeof(sa));
    sa.sun_family = AF_UNIX;
    snprintf(sa.sun_path, sizeof(sa.sun_path), "%s", path);
    if (connect(fd, (struct sockaddr*)&sa, sizeof(sa)) != 0 ||
        send(fd, "register", 8, 0) != 8) {
        close(fd);
        return 0;
    }
    n = recv(fd, reply, sizeof(reply) - 1, 0);
    close(fd);
    if (n <= 0)
        return 0;
    reply[n] = '\0';
    if (strncmp(reply, "token=", 6) != 0 || tok_n < 65)
        return 0;
    memcpy(tok, reply + 6, 64);
    tok[64] = '\0';
    return 1;
}

int main(int argc, char** argv)
{
    const char* ip = "127.0.0.1";
    const char* src = NULL;
    const char* token = NULL;
    const char* mark = NULL;
    const char* regsock = NULL;
    char regtok[65] = "";
    int port = 40000, sessions = 10;
    double hold_ms = 500.0, gap_ms = 500.0, t0;
    int i;

    for (i = 1; i < argc; i++) {
        if (!strncmp(argv[i], "ip=", 3))
            ip = argv[i] + 3;
        else if (!strncmp(argv[i], "src=", 4))
            src = argv[i] + 4;
        else if (!strncmp(argv[i], "port=", 5))
            port = atoi(argv[i] + 5);
        else if (!strncmp(argv[i], "sessions=", 9))
            sessions = atoi(argv[i] + 9);
        else if (!strncmp(argv[i], "hold_ms=", 8))
            hold_ms = strtod(argv[i] + 8, NULL);
        else if (!strncmp(argv[i], "gap_ms=", 7))
            gap_ms = strtod(argv[i] + 7, NULL);
        else if (!strncmp(argv[i], "token=", 6))
            token = argv[i] + 6;
        else if (!strncmp(argv[i], "mark=", 5))
            mark = argv[i] + 5;
        else if (!strncmp(argv[i], "regsock=", 8))
            regsock = argv[i] + 8;
        else
            return args_usage(argv[0],
                              "ip,port,sessions,hold_ms,gap_ms,token,mark,regsock"),
                   1;
    }
    if (sessions < 1 || hold_ms < 0.0 || gap_ms < 0.0)
        return args_usage(argv[0], "ip,port,sessions,hold_ms,gap_ms"), 1;
    if (regsock && self_register(regsock, regtok, sizeof(regtok)))
        token = regtok;
    csv_header();
    t0 = now_ms_wall();
    for (i = 0; i < sessions; i++) {
        int fd = tcp_connect_src(ip, port, 1000, src);
        if (fd < 0) {
            csv_row(t0, "session", ip, port, "refused");
        } else {
            char hello[96];
            const char* payload = HELLO;
            size_t plen = sizeof(HELLO) - 1;
            ssize_t n;
            if (mark) {
                int m = (int)strtol(mark, NULL, 0);
                setsockopt(fd, SOL_SOCKET, SO_MARK, &m, sizeof(m));
            }
            if (token) {
                snprintf(hello, sizeof(hello), "PROTEUS TOKEN %s\n", token);
                payload = hello;
                plen = strlen(hello);
            }
            n = send(fd, payload, plen, 0);
            csv_row(t0, "session", ip, port,
                    n == (ssize_t)plen ? "data_sent" : "timeout");
            {
                struct timespec slp;
                slp.tv_sec = (time_t)(hold_ms / 1000.0);
                slp.tv_nsec = (long)((hold_ms
                                      - (double)slp.tv_sec * 1000.0) * 1e6);
                nanosleep(&slp, NULL);
            }
            close(fd);
        }
        {
            struct timespec slp;
            slp.tv_sec = (time_t)(gap_ms / 1000.0);
            slp.tv_nsec = (long)((gap_ms
                                  - (double)slp.tv_sec * 1000.0) * 1e6);
            nanosleep(&slp, NULL);
        }
    }
    return 0;
}
