#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "tools_common.h"

double now_ms_wall(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

int tcp_connect(const char* ip, int port, int timeout_ms)
{
    return tcp_connect_src(ip, port, timeout_ms, NULL);
}

int tcp_connect_src(const char* ip, int port, int timeout_ms,
                    const char* src_ip)
{
    struct sockaddr_in sa;
    struct pollfd pf;
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    int err = 0;
    socklen_t len = sizeof(err);
    if (fd < 0)
        return -1;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons((uint16_t)port);
    if (inet_pton(AF_INET, ip, &sa.sin_addr) != 1)
        goto fail;
    if (src_ip) {
        /* bind the outgoing address for the multi source scenarios */
        struct sockaddr_in sa_src;
        memset(&sa_src, 0, sizeof(sa_src));
        sa_src.sin_family = AF_INET;
        if (inet_pton(AF_INET, src_ip, &sa_src.sin_addr) != 1 ||
            bind(fd, (struct sockaddr*)&sa_src, sizeof(sa_src)) != 0)
            goto fail;
    }
    {
        int flags = fcntl(fd, F_GETFL, 0);
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }
    if (connect(fd, (struct sockaddr*)&sa, sizeof(sa)) != 0 &&
        errno != EINPROGRESS)
        goto fail;
    pf.fd = fd;
    pf.events = POLLOUT;
    if (poll(&pf, 1, timeout_ms) != 1)
        goto fail;
    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len) != 0 || err != 0)
        goto fail;
    return fd;
fail:
    close(fd);
    return -1;
}

void csv_header(void)
{
    printf(TOOLS_CSV_HEADER "\n");
}

void csv_row(double t0, const char* event, const char* ip, int port,
             const char* result)
{
    printf("%.0f,%s,%s,%d,%s\n", now_ms_wall() - t0, event, ip, port,
           result);
}

void args_usage(const char* prog, const char* keys)
{
    fprintf(stderr, "usage: %s key=value ... keys: %s\n", prog, keys);
}
