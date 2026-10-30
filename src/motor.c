/* sockets are outside the c99 skill whitelist; the proteus spec puts the
   network body on tcp and later nftables, that approval lives here */
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/random.h>

#include "motor.h"
#include "config.h"

static const char* bind_addr = "127.0.0.1";

static uint64_t rr_next(uint64_t* s)
{
    uint64_t x = *s;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    *s = x;
    return x * 2685821657736338717ULL;
}

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
    if (inet_pton(AF_INET, bind_addr, &sa.sin_addr) != 1)
        sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    sa.sin_port = htons((uint16_t)port);
    if (bind(fd, (struct sockaddr*)&sa, sizeof(sa)) != 0 ||
        listen(fd, 16) != 0 ||
        getsockname(fd, (struct sockaddr*)&sa, &len) != 0)
        goto fail;
    actual = ntohs(sa.sin_port);
    if (actual != port)
        goto fail;
    {
        /* the tick loop accepts from this socket, it must not block
           when the backlog is empty */
        int flags = fcntl(fd, F_GETFL, 0);
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }
    return fd;
fail:
    close(fd);
    return -1;
}

static void perm_init(motor_t* m, uint64_t seed)
{
    /* Fisher-Yates shuffle of 0..63 using xorshift seeded with seed */
    for (int i = 0; i < 64; i++)
        m->perm[i] = (uint8_t)i;
    uint64_t s = seed ^ 0xBF58476D1CE4E5B9ULL;
    if (!s) s = 0x9E3779B97F4A7C15ULL;
    for (int i = 63; i > 0; i--) {
        s ^= s >> 12;
        s ^= s << 25;
        s ^= s >> 27;
        s *= 2685821657736338717ULL;
        int j = (int)(s % (uint64_t)(i + 1));
        uint8_t tmp = m->perm[i];
        m->perm[i] = m->perm[j];
        m->perm[j] = tmp;
    }
    m->perm_ready = 1;
}

int motor_init(motor_t* m, const char* addr, int live, uint64_t seed)
{
    memset(m, 0, sizeof(*m));
    if (addr)
        bind_addr = addr;
    m->fd = -1;
    m->live = live ? 1 : 0;
    m->rr = seed ^ 0x9E3779B97F4A7C15ULL;
    if (!m->rr)
        m->rr = 88172645463325252ULL;
    if (!m->live) {
        /* headless replay: no socket, the port schedule must not
           depend on which host ports are free right now */
        m->port = PORT_BASE;
        return 0;
    }
    /* live mode: generate secret permutation for DEST_BIO obfuscation */
    perm_init(m, seed);
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

static int headless_bind(motor_t* m, int index)
{
    m->binds++;
    m->port = PORT_BASE + index % PORT_WINDOW;
    m->moves++;
    return m->port;
}

int motor_move(motor_t* m)
{
    int tried;

    if (m->fd >= 0) {
        close(m->fd);
        m->fd = -1;
        m->port = 0;
    }
    if (!m->live)
        return headless_bind(m, m->binds);
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

int motor_move_to(motor_t* m, int index)
{
    int tried;

    if (m->fd >= 0) {
        close(m->fd);
        m->fd = -1;
        m->port = 0;
    }
    if (!m->live)
        return headless_bind(m, index);
    index %= PORT_WINDOW;
    if (index < 0)
        index += PORT_WINDOW;
    /* h9a amendment: no-op elimination — if secret permutation maps
     * back to current port, advance forward until port changes */
    if (m->perm_ready)
        index = m->perm[index];
    for (tried = 0; tried < PORT_WINDOW; tried++) {
        int port = PORT_BASE + (index + tried) % PORT_WINDOW;
        if (m->port == port) {
            /* no-op: destination equals current port, advance */
            continue;
        }
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

int motor_next_random(motor_t* m)
{
    return (int)(rr_next(&m->rr) % (uint64_t)PORT_WINDOW);
}

void motor_close(motor_t* m)
{
    if (m->fd >= 0)
        close(m->fd);
    m->fd = -1;
    m->port = 0;
}
