#ifndef PROTEUS_SECCOMP_H
#define PROTEUS_SECCOMP_H

/*
 * syscall whitelist loader. the allowed set is generated from a real
 * strace of the daemon by tools/seccomp_gen.sh into
 * src/seccomp_profile.h (built with make SECCOMP=1), never from
 * imagination. seccomp bounds the syscall surface of the daemon, it
 * does not protect against signals, an insider with the same uid can
 * still kill the process.
 * returns 0 when the filter is installed.
 */
int seccomp_install(void);

#endif
