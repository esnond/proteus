#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "trust.h"

typedef struct {
    uint8_t  token[TRUST_TOKEN_LEN];
    char     profile_hex[65];
    pid_t    pid;
    uid_t    uid;
    double   expiry_ms;
} entry_t;

struct trust {
    entry_t e[TRUST_TOKENS_MAX];
    int     n;
    double  last_reap_ms;
};

static const char HEX[] = "0123456789abcdef";

static int hex_val(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

static int valid_profile(const char* hex)
{
    size_t i;
    if (!hex)
        return 1;   /* no profile: empty hash is allowed */
    if (strlen(hex) != 64)
        return 0;
    for (i = 0; i < 64; i++)
        if (hex_val(hex[i]) < 0)
            return 0;
    return 1;
}

static int random_bytes(uint8_t* out, size_t n)
{
    /* getrandom is in the seccomp whitelist; the profile carries it */
    ssize_t got = syscall(SYS_getrandom, out, n, 0);
    return got == (ssize_t)n;
}

static void hex_encode(const uint8_t* in, size_t n, char* out)
{
    size_t i;
    for (i = 0; i < n; i++) {
        out[2 * i] = HEX[in[i] >> 4];
        out[2 * i + 1] = HEX[in[i] & 15];
    }
    out[2 * n] = '\0';
}

/* constant time compare of two same length byte strings */
static int ct_equal(const uint8_t* a, const uint8_t* b, size_t n)
{
    volatile uint8_t d = 0;
    size_t i;
    for (i = 0; i < n; i++)
        d |= (uint8_t)(a[i] ^ b[i]);
    return d == 0;
}

static int pid_alive(pid_t pid)
{
    char path[64];
    struct stat st;
    if (pid <= 0)
        return 0;
    snprintf(path, sizeof(path), "/proc/%ld", (long)pid);
    return stat(path, &st) == 0;
}

trust_t* trust_init(void)
{
    return calloc(1, sizeof(trust_t));
}

int trust_register(trust_t* t, pid_t pid, uid_t uid,
                   const char* profile_hex, double now_ms, char* out_hex,
                   size_t out_n)
{
    entry_t* e;
    if (!t || t->n >= TRUST_TOKENS_MAX || out_n < 65)
        return 0;
    if (!valid_profile(profile_hex) || !pid_alive(pid))
        return 0;
    e = &t->e[t->n];
    if (!random_bytes(e->token, sizeof(e->token)))
        return 0;
    if (profile_hex)
        snprintf(e->profile_hex, sizeof(e->profile_hex), "%s", profile_hex);
    e->pid = pid;
    e->uid = uid;
    e->expiry_ms = now_ms + TRUST_TOKEN_TTL_MS;
    t->n++;
    hex_encode(e->token, sizeof(e->token), out_hex);
    return 1;
}

int trust_check(const trust_t* t, const char* hex, double now_ms)
{
    uint8_t want[TRUST_TOKEN_LEN];
    int i, j;
    if (!t || !hex || strlen(hex) != 64)
        return 0;
    for (i = 0; i < 64; i++) {
        int v = hex_val(hex[i]);
        if (v < 0)
            return 0;
        want[i / 2] = (uint8_t)((i % 2 == 0) ? (v << 4) : (want[i / 2] | v));
    }
    for (j = 0; j < t->n; j++) {
        const entry_t* e = &t->e[j];
        if (now_ms >= e->expiry_ms || !pid_alive(e->pid))
            continue;
        if (ct_equal(want, e->token, TRUST_TOKEN_LEN))
            return 1;
    }
    return 0;
}

void trust_reap(trust_t* t, double now_ms)
{
    int i;
    if (!t || now_ms - t->last_reap_ms < TRUST_REAP_MS)
        return;
    t->last_reap_ms = now_ms;
    for (i = t->n - 1; i >= 0; i--) {
        if (now_ms >= t->e[i].expiry_ms || !pid_alive(t->e[i].pid)) {
            memset(&t->e[i], 0, sizeof(entry_t));
            t->e[i] = t->e[t->n - 1];
            memset(&t->e[t->n - 1], 0, sizeof(entry_t));
            t->n--;
        }
    }
}

int trust_active(const trust_t* t)
{
    return t ? t->n : 0;
}

void trust_close(trust_t* t)
{
    if (!t)
        return;
    memset(t, 0, sizeof(*t));
    free(t);
}
