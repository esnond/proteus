#include <errno.h>
#include <fcntl.h>
#include <linux/audit.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <unistd.h>

#ifdef HAS_SECCOMP_PROFILE
#include "seccomp_profile.h"
#endif

#ifndef HAS_SECCOMP_PROFILE
int seccomp_install(void)
{
    /* no profile compiled in, the honest refusal: a whitelist must come
       from a strace of the real daemon, tools/seccomp_gen.sh makes one */
    fprintf(stderr, "seccomp: no profile compiled in, run"
                    " tools/seccomp_gen.sh and make SECCOMP=1\n");
    return -1;
}
#else
/* the filter shape follows seccomp(2): an arch gate, an x32 gate, then
 * one compare and one allow verdict per syscall, kill on fallthrough.
 * jt and jf are skip counts after the compare, mixing them up kills
 * allowed calls first. */
static const struct sock_filter seccomp_head[] = {
    BPF_STMT(BPF_LD | BPF_W | BPF_ABS,
             offsetof(struct seccomp_data, arch)),
    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, AUDIT_ARCH_X86_64, 1, 0),
    BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_KILL),
    BPF_STMT(BPF_LD | BPF_W | BPF_ABS,
             offsetof(struct seccomp_data, nr)),
    BPF_JUMP(BPF_JMP | BPF_JGE | BPF_K, 0x40000000, 0, 1),
    BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_KILL),
};

static const struct sock_filter seccomp_tail[] = {
    BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_KILL),
};

int seccomp_install(void)
{
    /* the profile carries its own element count, a zero sentinel can
     * not work: __NR_read is syscall number zero */
    struct sock_filter prog[SECCOMP_ALLOW_N * 2 + 8];
    struct sock_fprog fprog;
    size_t i, k = 0;

    for (i = 0; i < sizeof(seccomp_head) / sizeof(seccomp_head[0]); i++)
        prog[k++] = seccomp_head[i];
    for (i = 0; i < SECCOMP_ALLOW_N; i++) {
        /* every allowed number, an allow verdict right after the
         * compare, miss walks to the next compare */
        prog[k++] = (struct sock_filter)
            BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, seccomp_allow[i], 0, 1);
        prog[k++] = (struct sock_filter)
            BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW);
    }
    for (i = 0; i < sizeof(seccomp_tail) / sizeof(seccomp_tail[0]); i++)
        prog[k++] = seccomp_tail[i];

    fprog.len = (unsigned short)k;
    fprog.filter = prog;
    if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) != 0)
        return -1;
    return prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &fprog) == 0 ? 0
                                                                   : -1;
}
#endif
