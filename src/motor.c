/* sockets are outside the c99 skill whitelist; the proteus spec puts the
   network body on tcp and later nftables, that approval lives here */
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "motor.h"
#include "config.h"

static int bind_listen(int port)
{
    struct sockaddr_in sa;
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    int actual;
    socklen_t len = sizeof(actual);

    if (fd < 0)
        return -1;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    sa.sin_port = htons((uint16_t)port);
    if (bind(fd, (struct sockaddr*)&sa, sizeof(sa)) != 0 ||
        listen(fd, 16) != 0 ||
        getsockname(fd, (struct sockaddr*)&sa, &len) != 0)
        goto fail;
    actual = ntohs(sa.sin_port);
    if (actual != port)
        goto fail;
    return fd;
fail:
    close(fd);
    return -1;
}

int motor_init(motor_t* m)
{
    memset(m, 0, sizeof(*m));
    m->fd = -1;
    /* the worm enters the window at its head, one walk step per bind */
    while (m->binds < PORT_WINDOW) {
        int port = PORT_BASE + m->binds;
        m->fd = bind_listen(port);
        if (m->fd >= 0) {
            m->port = port;
            m->binds++;
            return 0;
        }
        m->binds++;
    }
    return -1;
}

int motor_move(motor_t* m)
{
    int tried;

    if (m->fd >= 0) {
        close(m->fd);
        m->fd = -1;
        m->port = 0;
    }
    for (tried = 0; tried < PORT_WINDOW; tried++) {
        int port = PORT_BASE + m->binds % PORT_WINDOW;
        m->binds++;
        m->fd = bind_listen(port);
        if (m->fd >= 0) {
            m->port = port;
            m->moves++;
            return m->port;
        }
    }
    return -1;
}

void motor_close(motor_t* m)
{
    if (m->fd >= 0)
        close(m->fd);
    m->fd = -1;
    m->port = 0;
}
