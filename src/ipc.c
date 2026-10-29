/* the status and log channel. so_peercred needs struct ucred, a gnu
   extension; the proteus spec mandates the check, the define lives here
   and nowhere else. */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include "ipc.h"
#include "config.h"
#include "trust.h"

enum { CMD_PENDING = -1, CMD_STATUS, CMD_LOG, CMD_LOG_JSON, CMD_LOG_QUIET,
       CMD_REGISTER, CMD_UNKNOWN };

struct ipc {
    int fd;
    int client[IPC_CLIENTS_MAX];
    int mode[IPC_CLIENTS_MAX];   /* cmd enum per client */
    char path[108];
    long group;
};

/* supplementary groups of the peer process live in its proc status, the
   primary gid alone would miss every real group member */
static int in_group(long pid, long gid)
{
    FILE* f;
    char path[64];
    char buf[2048];
    const char* key = "Groups:";
    char* p;

    snprintf(path, sizeof(path), "/proc/%ld/status", pid);
    f = fopen(path, "r");
    if (!f)
        return 0;
    while (fgets(buf, sizeof(buf), f)) {
        if (strncmp(buf, key, strlen(key)) == 0)
            break;
    }
    fclose(f);
    if (strncmp(buf, key, strlen(key)) != 0)
        return 0;
    p = buf + strlen(key);
    while (*p) {
        char* end;
        long g = strtol(p, &end, 10);
        if (end == p)
            break;
        if (g == gid)
            return 1;
        p = end;
    }
    return 0;
}

static int allowed(struct ucred cr, long group)
{
    /* the socket owner needs no root and no group, group members pass
       through their supplementary groups, root passes as root */
    if (cr.uid == 0)
        return 1;
    if (cr.uid == geteuid())
        return 1;
    if (group < 0)
        return 0;
    if (cr.gid == (gid_t)group)
        return 1;
    return in_group(cr.pid, group);
}

ipc_t* ipc_init(const char* path, long group)
{
    ipc_t* ipc = calloc(1, sizeof(ipc_t));
    struct sockaddr_un sa;
    int i;

    if (!ipc)
        return NULL;
    ipc->fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (ipc->fd < 0) {
        free(ipc);
        return NULL;
    }
    memset(&sa, 0, sizeof(sa));
    sa.sun_family = AF_UNIX;
    snprintf(ipc->path, sizeof(ipc->path), "%s", path);
    snprintf(sa.sun_path, sizeof(sa.sun_path), "%s", path);
    unlink(path);
    if (bind(ipc->fd, (struct sockaddr*)&sa, sizeof(sa)) != 0 ||
        (group >= 0 && chown(path, -1, (gid_t)group) != 0) ||
        chmod(path, 0660) != 0 ||
        listen(ipc->fd, IPC_CLIENTS_MAX) != 0) {
        close(ipc->fd);
        unlink(path);
        free(ipc);
        return NULL;
    }
    ipc->group = group;
    for (i = 0; i < IPC_CLIENTS_MAX; i++) {
        ipc->client[i] = -1;
        ipc->mode[i] = CMD_UNKNOWN;
    }
    {
        int flags = fcntl(ipc->fd, F_GETFL, 0);
        fcntl(ipc->fd, F_SETFL, flags | O_NONBLOCK);
    }
    return ipc;
}

static int parse_cmd(const char* buf, ssize_t n)
{
    if (n <= 0)
        return CMD_UNKNOWN;
    if (strncmp(buf, "status", 6) == 0 && n >= 6)
        return CMD_STATUS;
    if (strncmp(buf, "register", 8) == 0)
        return CMD_REGISTER;
    if (strncmp(buf, "log", 3) == 0) {
        const char* rest = buf + 3;
        if (strstr(rest, "json"))
            return CMD_LOG_JSON;
        if (strstr(rest, "quiet"))
            return CMD_LOG_QUIET;
        return CMD_LOG;
    }
    return CMD_UNKNOWN;
}

/* json escape of one journal line: the frozen csv columns become one
   object, no strings inside, no escaping needed */
static void json_line(const char* line, char* out, size_t outn)
{
    double t, ava, avb, ash, ser, oct, dop, score;
    int port, cause, ev;
    if (sscanf(line, "%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%d,%d,%d",
               &t, &ava, &avb, &ash, &ser, &oct, &dop, &score,
               &port, &cause, &ev) != 11) {
        snprintf(out, outn, "{\"bad\":1}\n");
        return;
    }
    snprintf(out, outn,
             "{\"t_ms\":%.2f,\"act_ava\":%.4f,\"act_avb\":%.4f,"
             "\"ash_trace\":%.4f,\"ser\":%.4f,\"oct\":%.4f,\"dop\":%.4f,"
             "\"det_score\":%.3f,\"port\":%d,\"cause\":%d,\"event\":%d}\n",
             t, ava, avb, ash, ser, oct, dop, score, port, cause, ev);
}

/* parse and act on one command packet. returns the mode to store, or -1
   when the peer was answered and closed here (registration) or dropped
   (unknown/oversized). */
static int classify(int c, char* cmd, ssize_t n,
                    trust_t* tr, double now_ms)
{
    int m = parse_cmd(cmd, n);
    struct ucred cr;
    socklen_t len = sizeof(cr);

    if (m == CMD_REGISTER) {
        /* registration is stricter than status: owner or root only,
           never a plain socket-group member */
        char token[65];
        const char* prof = cmd + 8;
        char reply[96];
        int ok = 0;
        uid_t euid = geteuid();

        while (*prof == ' ')
            prof++;
        if (getsockopt(c, SOL_SOCKET, SO_PEERCRED, &cr, &len) == 0 &&
            (cr.uid == 0 || cr.uid == euid) &&
            trust_register(tr, cr.pid, cr.uid, *prof ? prof : NULL,
                           now_ms, token, sizeof(token)))
            ok = 1;
        if (ok)
            snprintf(reply, sizeof(reply), "token=%s ttl_ms=%d\n",
                     token, (int)TRUST_TOKEN_TTL_MS);
        else
            snprintf(reply, sizeof(reply), "denied\n");
        send(c, reply, strlen(reply), 0);
        close(c);
        return -1;
    }
    if (m == CMD_UNKNOWN) {
        /* an unknown or oversized command must not occupy a slot: the
           peer never gets a reply, so holding it is a leak */
        close(c);
        return -1;
    }
    return m;
}

/* first packet of a peer that connected before it sent. 1 = mode stored,
   0 = still waiting, -1 = slot released */
static int pending_read(ipc_t* ipc, int i, trust_t* tr, double now_ms)
{
    char cmd[IPC_CMD_MAX];
    ssize_t n = recv(ipc->client[i], cmd, sizeof(cmd) - 1,
                     MSG_DONTWAIT | MSG_TRUNC);
    int m;

    if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
        return 0;
    if (n <= 0 || n > (ssize_t)sizeof(cmd) - 1) {
        close(ipc->client[i]);
        ipc->client[i] = -1;
        return -1;
    }
    cmd[n] = '\0';
    m = classify(ipc->client[i], cmd, n, tr, now_ms);
    if (m < 0) {
        ipc->client[i] = -1;
        return -1;
    }
    ipc->mode[i] = m;
    return 1;
}

/* the listener plus every peer still waiting for its first packet: the
   event loop polls these so a late send is serviced, not dropped */
int ipc_poll_fds(const ipc_t* ipc, struct pollfd* pf, int max)
{
    int n = 0, i;

    if (!ipc || max < 1)
        return 0;
    pf[n].fd = ipc->fd;
    pf[n].events = POLLIN;
    pf[n].revents = 0;
    n++;
    for (i = 0; i < IPC_CLIENTS_MAX && n < max; i++) {
        if (ipc->client[i] >= 0 && ipc->mode[i] == CMD_PENDING) {
            pf[n].fd = ipc->client[i];
            pf[n].events = POLLIN;
            pf[n].revents = 0;
            n++;
        }
    }
    return n;
}

void ipc_tick(ipc_t* ipc, trust_t* tr, double now_ms,
              const char* status_line, const char* line)
{
    int i;

    for (;;) {
        int c = accept(ipc->fd, NULL, NULL);
        struct ucred cr;
        socklen_t len = sizeof(cr);
        char cmd[IPC_CMD_MAX];
        ssize_t n;
        int m;
        if (c < 0)
            break;
        if (getsockopt(c, SOL_SOCKET, SO_PEERCRED, &cr, &len) != 0 ||
            !allowed(cr, ipc->group)) {
            close(c);
            continue;
        }
        /* MSG_TRUNC reports the real datagram length, so an oversized
           command is rejected instead of being cut down to a valid
           prefix ("log " + 4 kB would otherwise become a subscription
           that holds the slot forever) */
        n = recv(c, cmd, sizeof(cmd) - 1, MSG_DONTWAIT | MSG_TRUNC);
        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            /* the peer connected before it sent. a wake on the connect
               used to close it here; keep it and let poll watch the fd
               until the packet arrives */
            for (i = 0; i < IPC_CLIENTS_MAX; i++)
                if (ipc->client[i] < 0)
                    break;
            if (i == IPC_CLIENTS_MAX) {
                close(c);
                continue;
            }
            ipc->client[i] = c;
            ipc->mode[i] = CMD_PENDING;
            continue;
        }
        if (n <= 0 || n > (ssize_t)sizeof(cmd) - 1) {
            close(c);
            continue;
        }
        cmd[n] = '\0';
        m = classify(c, cmd, n, tr, now_ms);
        if (m < 0)
            continue;
        for (i = 0; i < IPC_CLIENTS_MAX; i++) {
            if (ipc->client[i] < 0) {
                ipc->client[i] = c;
                ipc->mode[i] = m;
                break;
            }
        }
        if (i == IPC_CLIENTS_MAX)
            close(c);
    }

    for (i = 0; i < IPC_CLIENTS_MAX; i++) {
        int c = ipc->client[i];
        if (c < 0)
            continue;
        if (ipc->mode[i] == CMD_PENDING) {
            if (pending_read(ipc, i, tr, now_ms) <= 0)
                continue;
            c = ipc->client[i];
        }
        if (ipc->mode[i] == CMD_STATUS) {
            send(c, status_line, strlen(status_line), 0);
            close(c);
            ipc->client[i] = -1;
            ipc->mode[i] = CMD_UNKNOWN;
        } else if (ipc->mode[i] == CMD_LOG && line) {
            if (send(c, line, strlen(line), 0) < 0 && errno != EAGAIN) {
                close(c);
                ipc->client[i] = -1;
            }
        } else if (line) {
            char jbuf[512];
            if (ipc->mode[i] == CMD_LOG_JSON)
                json_line(line, jbuf, sizeof(jbuf));
            else if (ipc->mode[i] == CMD_LOG_QUIET) {
                int cause, ev;
                if (sscanf(line, "%*f,%*f,%*f,%*f,%*f,%*f,%*f,%*f,%*d,%d,%d",
                           &cause, &ev) != 2 || (cause == 0 && ev == 0))
                    continue;
                snprintf(jbuf, sizeof(jbuf), "%s", line);
            } else
                continue;
            if (send(c, jbuf, strlen(jbuf), 0) < 0 && errno != EAGAIN) {
                close(c);
                ipc->client[i] = -1;
            }
        }
    }
}

void ipc_close(ipc_t* ipc)
{
    int i;
    if (!ipc)
        return;
    for (i = 0; i < IPC_CLIENTS_MAX; i++)
        if (ipc->client[i] >= 0)
            close(ipc->client[i]);
    if (ipc->fd >= 0) {
        close(ipc->fd);
        unlink(ipc->path);
    }
    free(ipc);
}
