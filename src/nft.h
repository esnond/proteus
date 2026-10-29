#ifndef PROTEUS_NFT_H
#define PROTEUS_NFT_H

#include <stdint.h>

#include "nft_abi.h"

/*
 * nftables backend of the daemon, a thin wrapper over libmnl speaking
 * the netlink protocol directly, libnftnl is not a dependency. every
 * builder appends one message to an internal batch, nft_commit sends the
 * batch as one atomic transaction and reads the acks. the wrapper fails
 * gracefully: nft_open returns an unusable backend when the kernel or
 * the privileges are missing, the daemon then runs the userspace ban
 * list alone, loudly.
 *
 * layout: table "proteus" (family inet), chains "prerouting" and
 * "input", set "banned" for the addresses the detector fired on.
 */
typedef struct nft nft_t;

nft_t* nft_open(void);
void   nft_close(nft_t* n);

/* transaction building, all calls queue into the batch */
int nft_table_create(nft_t* n, const char* name);
int nft_chain_add(nft_t* n, const char* table, const char* chain,
                  int hook, int priority, int policy);
/* drop everything from src_ip toward dst_port (0 means any port) */
int nft_rule_add_drop(nft_t* n, const char* table, const char* chain,
                      uint32_t src_ip_be, uint16_t dst_port);
/* accept rules live on top by priority: a known peer passes first */
int nft_rule_add_accept(nft_t* n, const char* table, const char* chain,
                        uint32_t src_ip_be, uint16_t dst_port);
int nft_rule_add_mark_accept(nft_t* n, const char* table, const char* chain,
                             uint32_t mark);
int nft_set_add(nft_t* n, const char* table, const char* name);
/* add or remove one banned address in the set */
int nft_set_ban(nft_t* n, const char* table, const char* name,
                uint32_t ip_be);
int nft_set_unban(nft_t* n, const char* table, const char* name,
                  uint32_t ip_be);
/* send the queued messages as one atomic batch, 0 on full ack */
int nft_commit(nft_t* n);

/* the fixed names of the layout */
#define NFT_TABLE_NAME    "proteus"
#define NFT_CHAIN_IN      "input"
#define NFT_CHAIN_PRE     "prerouting"
#define NFT_CHAIN_TRUST   "trust"
#define NFT_SET_BANNED    "banned"

#endif
