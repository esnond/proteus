#ifndef PROTEUS_IPC_H
#define PROTEUS_IPC_H

#include <stddef.h>

#include "trust.h"

/*
 * status and log channel of the daemon, unix socket, seqpacket. a client
 * sends one command packet, "status" gets one status packet and a close,
 * "log", "log json" or "log quiet" subscribes to the journal stream,
 * "register <64 hex>" issues an immune scope token to the owner/root.
 * peers are checked through so_peercred: the socket owner or a member of
 * the socket group passes, members of the group need no root.
 */
#define IPC_CLIENTS_MAX 8
#define IPC_CMD_MAX     32

typedef struct ipc ipc_t;

ipc_t* ipc_init(const char* path, long group);
/* accept new peers, read commands, publish one journal line to log
 * subscribers */
void   ipc_tick(ipc_t* ipc, trust_t* tr, double now_ms,
                const char* status_line, const char* line);
void   ipc_close(ipc_t* ipc);

#endif
