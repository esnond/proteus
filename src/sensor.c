#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "config.h"
#include "sensor.h"

struct sensor {
    int    listener;
    peer_t peers[MAX_PEERS];
    int    n_peers;
    ban_t  bans[MAX_BANS];
    int    n_bans;
    trust_ip_t tips[SENSOR_TRUST_IPS];
    int    n_tips;
    fail_t fails[SENSOR_FAIL_IPS];
    int    n_fails;
};

sensor_t* sensor_init(void)
{
    return calloc(1, sizeof(sensor_t));
}

int sensor_trusted(const sensor_t* sn, uint32_t ip, double t_ms)
{
    int i;
    for (i = 0; i < sn->n_tips; i++)
        if (sn->tips[i].ip == ip && t_ms < sn->tips[i].until_ms)
            return 1;
    return 0;
}

static void trust_ip(sensor_t* sn, uint32_t ip, double t_ms)
{
    int i;
    for (i = 0; i < sn->n_tips; i++) {
        if (sn->tips[i].ip == ip) {
            sn->tips[i].until_ms = t_ms + SENSOR_TRUST_MS;
            return;
        }
    }
    if (sn->n_tips < SENSOR_TRUST_IPS) {
        sn->tips[sn->n_tips].ip = ip;
        sn->tips[sn->n_tips].until_ms = t_ms + SENSOR_TRUST_MS;
        sn->n_tips++;
    }
}

/* one failed auth line: count per source inside a rolling window. a
   source that crosses the burst threshold is returned to the caller as
   a ban request; this is standard security, the connectome is not in
   this path. one recv counts once, stuffing is one line per
   connection. */
static void auth_fail(sensor_t* sn, uint32_t ip, double t_ms,
                      uint32_t* ban_ip)
{
    fail_t* f = NULL;
    int i;

    if (sensor_trusted(sn, ip, t_ms))
        return;
    for (i = 0; i < sn->n_fails; i++) {
        if (sn->fails[i].ip == ip) {
            f = &sn->fails[i];
            break;
        }
    }
    if (!f && sn->n_fails < SENSOR_FAIL_IPS) {
        f = &sn->fails[sn->n_fails++];
        f->ip = ip;
        f->count = 0;
        f->first_ms = t_ms;
    }
    if (!f) {
        /* table full: reuse the oldest window */
        int oldest = 0;
        for (i = 1; i < sn->n_fails; i++)
            if (sn->fails[i].first_ms < sn->fails[oldest].first_ms)
                oldest = i;
        f = &sn->fails[oldest];
        f->ip = ip;
        f->count = 0;
        f->first_ms = t_ms;
    }
    if (t_ms - f->first_ms >= SENSOR_FAIL_MS) {
        f->count = 0;
        f->first_ms = t_ms;
    }
    if (f->count < SENSOR_FAIL_MAX)
        f->count++;
    if (f->count >= SENSOR_FAIL_MAX && ban_ip) {
        *ban_ip = ip;
        f->count = 0;
        f->first_ms = t_ms;
    }
}

static int ip_index(const ban_t* bans, int n, uint32_t ip)
{
    int i;
    for (i = 0; i < n; i++)
        if (bans[i].ip == ip)
            return i;
    return -1;
}

int sensor_is_banned(const sensor_t* sn, uint32_t ip, double t_ms)
{
    int i;
    if (sensor_trusted(sn, ip, t_ms))
        return 0;
    i = ip_index(sn->bans, sn->n_bans, ip);
    return i >= 0 && t_ms < sn->bans[i].until_ms;
}

int sensor_ban(sensor_t* sn, uint32_t ip, double t_ms)
{
    int i = ip_index(sn->bans, sn->n_bans, ip);
    if (i >= 0) {
        sn->bans[i].until_ms = t_ms + (double)BAN_TTL_MS;
        return 0;
    }
    if (sn->n_bans >= MAX_BANS) {
        /* drop the oldest ban, the list is small and circular */
        int oldest = 0;
        for (i = 1; i < sn->n_bans; i++)
            if (sn->bans[i].until_ms < sn->bans[oldest].until_ms)
                oldest = i;
        sn->bans[oldest] = sn->bans[--sn->n_bans];
    }
    sn->bans[sn->n_bans].ip = ip;
    sn->bans[sn->n_bans].until_ms = t_ms + (double)BAN_TTL_MS;
    sn->n_bans++;
    return 1;
}

/* drop expired bans, close peers past the handshake window without
   payload (silent scan), return codes through the out params */
static int expire(sensor_t* sn, double t_ms, uint32_t* last_scan_ip,
                  int* any_scan)
{
    int i, ev = 0;
    for (i = sn->n_bans - 1; i >= 0; i--)
        if (t_ms >= sn->bans[i].until_ms)
            sn->bans[i] = sn->bans[--sn->n_bans];
    for (i = sn->n_tips - 1; i >= 0; i--)
        if (t_ms >= sn->tips[i].until_ms)
            sn->tips[i] = sn->tips[--sn->n_tips];
    for (i = sn->n_peers - 1; i >= 0; i--) {
        peer_t* p = &sn->peers[i];
        if (p->fed) {
            /* a held session with a vanished client must not occupy a
               peer slot forever */
            if (t_ms - p->born_ms >= (double)SESSION_MAX_MS) {
                close(p->fd);
                sn->peers[i] = sn->peers[--sn->n_peers];
            }
            continue;
        }
        if (t_ms - p->born_ms >= (double)SENSOR_HANDSHAKE_MS) {
            *last_scan_ip = p->ip;
            *any_scan = 1;
            ev = 1;
            close(p->fd);
            sn->peers[i] = sn->peers[--sn->n_peers];
        }
    }
    return ev;
}

/* drain one peer socket, close on eof, feed the ash counter on payload */
static void drain(sensor_t* sn, peer_t* p, double t_ms, trust_t* tr,
                  uint32_t* last_scan_ip, int* any_scan, int* any_session,
                  uint32_t* ban_ip)
{
    char buf[SENSOR_BUF];
    ssize_t n = recv(p->fd, buf, sizeof(buf), MSG_DONTWAIT);
    if (n > 0) {
        /* payload proves an application client, not a silent port
           sweep; keep reading until the peer closes. a live registration
           token in the first line puts the source in the immune scope:
           its sessions never count as scans and the ban path skips it.
           a failed auth line is not food and not scan pressure: it only
           feeds the failure counter, which asks for an immune-scope ban
           when the source crosses the burst threshold. */
        int auth = n >= 13 && !memcmp(buf, "PROTEUS AUTH ", 13);
        p->fed = 1;
        if (auth) {
            auth_fail(sn, p->ip, t_ms, ban_ip);
            return;
        }
        *any_session = 1;
        if (tr && n >= 14 + 64 && !memcmp(buf, "PROTEUS TOKEN ", 14)) {
            char hex[65];
            memcpy(hex, buf + 14, 64);
            hex[64] = '\0';
            if (trust_check(tr, hex, t_ms))
                trust_ip(sn, p->ip, t_ms);
        }
        return;
    }
    if (n == 0) {
        /* eof: a scan if the peer never sent payload, otherwise a
           legitimate session that ended and must not be counted as
           scan pressure nor become the ban target */
        close(p->fd);
        if (!p->fed) {
            *last_scan_ip = p->ip;
            *any_scan = 1;
        }
        *p = sn->peers[--sn->n_peers];
        return;
    }
    /* eagain: still waiting, the handshake window expires in expire() */
    (void)0;
}

int sensor_tick(sensor_t* sn, int fd, double t_ms, trust_t* tr,
                uint32_t* last_scan_ip, int* any_scan, int* any_session,
                uint32_t* ban_ip)
{
    int ev = 0;
    int i;

    *ban_ip = 0;
    ev |= expire(sn, t_ms, last_scan_ip, any_scan);
    for (;;) {
        struct sockaddr_in sa;
        socklen_t len = sizeof(sa);
        int cfd = accept(fd, (struct sockaddr*)&sa, &len);
        peer_t* p;
        uint32_t ip;
        if (cfd < 0)
            break;
        ip = sa.sin_addr.s_addr;
        if (sensor_is_banned(sn, ip, t_ms) || sn->n_peers >= MAX_PEERS) {
            close(cfd);
            continue;
        }
        p = &sn->peers[sn->n_peers++];
        p->fd = cfd;
        p->ip = ip;
        p->born_ms = t_ms;
        p->fed = 0;
        drain(sn, p, t_ms, tr, last_scan_ip, any_scan, any_session,
              ban_ip);
    }
    for (i = 0; i < sn->n_peers; i++)
        drain(sn, &sn->peers[i], t_ms, tr, last_scan_ip,
              any_scan, any_session, ban_ip);
    if (*any_scan)
        ev |= 1;
    if (*any_session)
        ev |= 2;
    return ev;
}

void sensor_close(sensor_t* sn)
{
    int i;
    if (!sn)
        return;
    for (i = 0; i < sn->n_peers; i++)
        close(sn->peers[i].fd);
    free(sn);
}
