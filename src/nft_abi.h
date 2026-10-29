#ifndef PROTEUS_NFT_ABI_H
#define PROTEUS_NFT_ABI_H

/*
 * vendored subset of the nftables netlink abi, values from the uapi
 * header include/uapi/linux/nf_tables.h of linux 6.6 plus
 * netfilter/nfnetlink.h which ships in libc dev. the full header is not
 * available in this environment (libnftnl and linux-libc-dev without
 * nftables both absent), these are the stable generation one constants
 * the wrapper needs. changes here must mirror the abi, they are frozen
 * by the kernel for compatibility.
 */

/* netfilter/nfnetlink.h */
#define PROTEUS_NFNL_SUBSYS_NFTABLES 10
#define PROTEUS_NFNL_MSG_BATCH_BEGIN 16
#define PROTEUS_NFNL_MSG_BATCH_END   17

enum {
    P_NFT_MSG_NEWTABLE   = 0,
    P_NFT_MSG_NEWCHAIN   = 3,
    P_NFT_MSG_NEWRULE    = 6,
    P_NFT_MSG_NEWSET     = 9,
    P_NFT_MSG_NEWSETELEM = 12,
    P_NFT_MSG_DELSETELEM = 14
};

enum {
    P_NFTA_TABLE_NAME  = 1,
    P_NFTA_TABLE_FLAGS = 2
};

enum {
    P_NFTA_CHAIN_TABLE  = 1,
    P_NFTA_CHAIN_NAME   = 3,
    P_NFTA_CHAIN_HOOK   = 4,
    P_NFTA_CHAIN_POLICY = 5
};

enum {
    P_NFTA_HOOK_HOOKNUM   = 1,
    P_NFTA_HOOK_PRIORITY  = 2
};

enum {
    P_NFTA_RULE_TABLE       = 1,
    P_NFTA_RULE_CHAIN       = 2,
    P_NFTA_RULE_EXPRESSIONS = 4
};

enum {
    P_NFTA_LIST_ELEM = 1
};

enum {
    P_NFTA_EXPR_NAME = 1,
    P_NFTA_EXPR_DATA = 2
};

enum {
    P_NFTA_PAYLOAD_DREG   = 1,
    P_NFTA_PAYLOAD_BASE   = 2,
    P_NFTA_PAYLOAD_OFFSET = 3,
    P_NFTA_PAYLOAD_LEN    = 4
};

enum {
    P_NFTA_META_DREG = 1,
    P_NFTA_META_KEY  = 2
};

enum {
    P_NFTA_CMP_SREG = 1,
    P_NFTA_CMP_OP   = 2,
    P_NFTA_CMP_DATA = 3
};

enum {
    P_NFTA_DATA_VALUE   = 1,
    P_NFTA_DATA_VERDICT = 2
};

enum {
    P_NFTA_VERDICT_CODE = 1
};

enum {
    P_NFTA_IMMEDIATE_DREG = 1,
    P_NFTA_IMMEDIATE_DATA = 2
};

enum {
    P_NFTA_SET_TABLE     = 1,
    P_NFTA_SET_NAME      = 2,
    P_NFTA_SET_FLAGS     = 3,
    P_NFTA_SET_KEY_TYPE  = 4,
    P_NFTA_SET_KEY_LEN   = 5,
    P_NFTA_SET_ID        = 10
};

enum {
    P_NFTA_SET_ELEM_LIST_TABLE     = 1,
    P_NFTA_SET_ELEM_LIST_SET       = 2,
    P_NFTA_SET_ELEM_LIST_ELEMENTS  = 3,
    P_NFTA_SET_ELEM_LIST_SET_ID    = 4
};

/* the set id used inside one transaction, must match between the set
   creation and the element list that references it */
#define NFT_SET_ID 1u

enum {
    P_NFTA_SET_ELEM_KEY = 1
};

/* expression names, null terminated strings on the wire */
#define P_EXPR_META    "meta"
#define P_EXPR_PAYLOAD "payload"
#define P_EXPR_CMP     "cmp"
#define P_EXPR_IMMEDIATE "immediate"

/* meta keys and payload bases */
#define P_NFT_META_L4PROTO        16
#define P_NFT_META_MARK            3
#define P_NFT_PAYLOAD_NETWORK_HDR 1
#define P_NFT_PAYLOAD_TRANSP_HDR  2
#define P_NFT_CMP_EQ              0
#define P_NFT_REG_VERDICT         0
#define P_NFT_REG_1               1
#define P_NFT_REG_2               2

/* verdict codes from linux/netfilter.h */
#define P_NF_DROP   0
#define P_NF_ACCEPT 1

/* nfgenmsg families */
#define P_NFPROTO_INET   1
#define P_NFPROTO_BRIDGE 0

/* hooks */
#define P_NF_INET_PRE_ROUTING 0
#define P_NF_INET_LOCAL_IN    1

/* set data types */
#define P_NFT_DATA_U32    0
#define P_NFT_DATA_IPV4   7

#endif
