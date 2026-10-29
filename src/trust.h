#ifndef PROTEUS_TRUST_H
#define PROTEUS_TRUST_H

#include <sys/types.h>
#include <stdint.h>

/*
 * r2.5 immune scope: local registration over the existing unix socket.
 * the peer is authenticated by SO_PEERCRED in ipc.c (owner or root only
 * for registration), gets an ephemeral 32 byte token, and the entry dies
 * with the process (the reaper stats /proc/<pid>). the token is drawn
 * from the kernel CSPRNG, not from the biological rng: the connectome is
 * a schedule generator, not a key source.
 *
 * trust_check is constant time. the token is the app level credential;
 * kernel enforcement uses the SO_MARK fast path (mark needs
 * CAP_NET_ADMIN, which a plain same-uid process does not have).
 */
#define TRUST_TOKENS_MAX   32
#define TRUST_TOKEN_LEN    32
#define TRUST_TOKEN_TTL_MS 600000.0
#define TRUST_REAP_MS      500.0
#define TRUST_MARK         0x50

typedef struct trust trust_t;

trust_t* trust_init(void);
/* 1 on success and out_hex gets 64 hex chars + nul, 0 when the table is
   full or the profile hash is malformed */
int  trust_register(trust_t* t, pid_t pid, uid_t uid,
                    const char* profile_hex, double now_ms, char* out_hex,
                    size_t out_n);
/* 1 when the token is live and its process is alive */
int  trust_check(const trust_t* t, const char* hex, double now_ms);
void trust_reap(trust_t* t, double now_ms);
int  trust_active(const trust_t* t);
void trust_close(trust_t* t);

#endif
