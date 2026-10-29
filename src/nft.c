/* nftables backend over libmnl, no libnftnl. the protocol constants
   live in nft_abi.h, the kernel freezes them. every mutator queues a
   netlink message, nft_commit ships the queue as one atomic batch and
   drains exactly the acks it asked for. the messages are built the same
   way libnftnl builds them: (subsys << 8) | type for nftables messages,
   raw NFNL_MSG_BATCH_BEGIN/END for the batch delimiters, res_id zero on
   normal messages, NLM_F_CREATE so a restart over an existing table is
   idempotent instead of EEXIST. */
#include <errno.h>
#include <linux/netfilter/nfnetlink.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include "nft.h"
#include "nft_abi.h"

#include <libmnl/libmnl.h>

#define NFT_BATCH_MAX 8192
#define NFT_ACK_WAIT_MS 2000

struct nft {
    struct mnl_socket* nl;
    char    buf[NFT_BATCH_MAX];
    size_t  len;
    uint32_t seq;
    int     pending;   /* messages in the batch that want an ack */
};

/* reserve one nftables message header at the queue tail */
static struct nlmsghdr* msg_open(nft_t* n, uint16_t type, int family,
                                 uint32_t res_id, uint16_t flags,
                                 size_t* off)
{
    struct nlmsghdr* nlh;
    struct nfgenmsg* nfg;
    if (n->len + 256 > sizeof(n->buf))
        return NULL;
    *off = n->len;
    nlh = mnl_nlmsg_put_header(n->buf + n->len);
    nlh->nlmsg_type = (uint16_t)((PROTEUS_NFNL_SUBSYS_NFTABLES << 8) | type);
    nlh->nlmsg_flags = (uint16_t)(NLM_F_REQUEST | NLM_F_ACK | flags);
    nlh->nlmsg_seq = ++n->seq;
    nfg = mnl_nlmsg_put_extra_header(nlh, sizeof(struct nfgenmsg));
    nfg->nfgen_family = (uint8_t)family;
    nfg->version = 0;
    nfg->res_id = htons((uint16_t)res_id);
    n->pending++;
    return nlh;
}

static void msg_close(nft_t* n, size_t off, struct nlmsghdr* nlh)
{
    n->len = off + nlh->nlmsg_len;
}

/* batch delimiters carry the raw type and the subsystem in res_id */
static void put_batch(nft_t* n, uint16_t type)
{
    struct nlmsghdr* nlh;
    struct nfgenmsg* nfg;
    if (n->len + 64 > sizeof(n->buf))
        return;
    nlh = mnl_nlmsg_put_header(n->buf + n->len);
    nlh->nlmsg_type = type;
    nlh->nlmsg_flags = NLM_F_REQUEST;
    nlh->nlmsg_seq = ++n->seq;
    nfg = mnl_nlmsg_put_extra_header(nlh, sizeof(struct nfgenmsg));
    nfg->nfgen_family = AF_UNSPEC;
    nfg->version = 0;
    nfg->res_id = htons(PROTEUS_NFNL_SUBSYS_NFTABLES);
    n->len += nlh->nlmsg_len;
}

nft_t* nft_open(void)
{
    nft_t* n = calloc(1, sizeof(nft_t));
    struct timeval tv;
    if (!n)
        return NULL;
    n->nl = mnl_socket_open(NETLINK_NETFILTER);
    if (!n->nl || mnl_socket_bind(n->nl, 0, MNL_SOCKET_AUTOPID) != 0) {
        if (n->nl)
            mnl_socket_close(n->nl);
        free(n);
        return NULL;
    }
    /* a batch that never gets its final ack must not hang the daemon */
    tv.tv_sec = NFT_ACK_WAIT_MS / 1000;
    tv.tv_usec = (NFT_ACK_WAIT_MS % 1000) * 1000;
    setsockopt(mnl_socket_get_fd(n->nl), SOL_SOCKET, SO_RCVTIMEO,
               &tv, sizeof(tv));
    n->len = 0;
    n->pending = 0;
    put_batch(n, NFNL_MSG_BATCH_BEGIN);
    return n;
}

void nft_close(nft_t* n)
{
    if (!n)
        return;
    if (n->nl)
        mnl_socket_close(n->nl);
    free(n);
}

int nft_table_create(nft_t* n, const char* name)
{
    size_t off;
    struct nlmsghdr* nlh;
    if (!n)
        return -1;
    nlh = msg_open(n, P_NFT_MSG_NEWTABLE, P_NFPROTO_INET, 0, NLM_F_CREATE,
                   &off);
    if (!nlh)
        return -1;
    mnl_attr_put_strz(nlh, P_NFTA_TABLE_NAME, name);
    mnl_attr_put_u32(nlh, P_NFTA_TABLE_FLAGS, htonl(0));
    msg_close(n, off, nlh);
    return 0;
}

int nft_chain_add(nft_t* n, const char* table, const char* chain,
                  int hook, int priority, int policy)
{
    size_t off;
    struct nlmsghdr* nlh;
    struct nlattr* hook_nest;
    if (!n)
        return -1;
    nlh = msg_open(n, P_NFT_MSG_NEWCHAIN, P_NFPROTO_INET, 0, NLM_F_CREATE,
                   &off);
    if (!nlh)
        return -1;
    mnl_attr_put_strz(nlh, P_NFTA_CHAIN_TABLE, table);
    mnl_attr_put_strz(nlh, P_NFTA_CHAIN_NAME, chain);
    hook_nest = mnl_attr_nest_start(nlh, P_NFTA_CHAIN_HOOK);
    mnl_attr_put_u32(nlh, P_NFTA_HOOK_HOOKNUM, htonl((uint32_t)hook));
    mnl_attr_put_u32(nlh, P_NFTA_HOOK_PRIORITY, htonl((uint32_t)priority));
    mnl_attr_nest_end(nlh, hook_nest);
    mnl_attr_put_u32(nlh, P_NFTA_CHAIN_POLICY, htonl((uint32_t)policy));
    msg_close(n, off, nlh);
    return 0;
}

static void put_meta(struct nlmsghdr* nlh, int dreg, int key)
{
    struct nlattr* elem = mnl_attr_nest_start(nlh, P_NFTA_LIST_ELEM);
    struct nlattr* data;
    mnl_attr_put_strz(nlh, P_NFTA_EXPR_NAME, P_EXPR_META);
    data = mnl_attr_nest_start(nlh, P_NFTA_EXPR_DATA);
    mnl_attr_put_u32(nlh, P_NFTA_META_DREG, htonl((uint32_t)dreg));
    mnl_attr_put_u32(nlh, P_NFTA_META_KEY, htonl((uint32_t)key));
    mnl_attr_nest_end(nlh, data);
    mnl_attr_nest_end(nlh, elem);
}

/* payload load into a register, then the compare closes the match */
static void put_payload(struct nlmsghdr* nlh, int dreg, int base,
                        int offset, int len)
{
    struct nlattr* elem = mnl_attr_nest_start(nlh, P_NFTA_LIST_ELEM);
    struct nlattr* data;
    mnl_attr_put_strz(nlh, P_NFTA_EXPR_NAME, P_EXPR_PAYLOAD);
    data = mnl_attr_nest_start(nlh, P_NFTA_EXPR_DATA);
    mnl_attr_put_u32(nlh, P_NFTA_PAYLOAD_DREG, htonl((uint32_t)dreg));
    mnl_attr_put_u32(nlh, P_NFTA_PAYLOAD_BASE, htonl((uint32_t)base));
    mnl_attr_put_u32(nlh, P_NFTA_PAYLOAD_OFFSET, htonl((uint32_t)offset));
    mnl_attr_put_u32(nlh, P_NFTA_PAYLOAD_LEN, htonl((uint32_t)len));
    mnl_attr_nest_end(nlh, data);
    mnl_attr_nest_end(nlh, elem);
}

static void put_cmp(struct nlmsghdr* nlh, int sreg, const void* value,
                    int vlen)
{
    struct nlattr* elem = mnl_attr_nest_start(nlh, P_NFTA_LIST_ELEM);
    struct nlattr* data;
    struct nlattr* dn;
    mnl_attr_put_strz(nlh, P_NFTA_EXPR_NAME, P_EXPR_CMP);
    data = mnl_attr_nest_start(nlh, P_NFTA_EXPR_DATA);
    mnl_attr_put_u32(nlh, P_NFTA_CMP_SREG, htonl((uint32_t)sreg));
    mnl_attr_put_u32(nlh, P_NFTA_CMP_OP, htonl(P_NFT_CMP_EQ));
    /* CMP_DATA wraps only the value, the name/sreg/op are siblings */
    dn = mnl_attr_nest_start(nlh, P_NFTA_CMP_DATA);
    mnl_attr_put(nlh, P_NFTA_DATA_VALUE, (uint16_t)vlen, value);
    mnl_attr_nest_end(nlh, dn);
    mnl_attr_nest_end(nlh, data);
    mnl_attr_nest_end(nlh, elem);
}

/* immediate verdict: IMMEDIATE_DATA -> DATA_VERDICT -> VERDICT_CODE,
   the extra DATA_VERDICT level is what nft_data_init parses */
static void put_verdict(struct nlmsghdr* nlh, int code)
{
    struct nlattr* elem = mnl_attr_nest_start(nlh, P_NFTA_LIST_ELEM);
    struct nlattr* data;
    struct nlattr* vd;
    struct nlattr* dv;
    mnl_attr_put_strz(nlh, P_NFTA_EXPR_NAME, P_EXPR_IMMEDIATE);
    data = mnl_attr_nest_start(nlh, P_NFTA_EXPR_DATA);
    mnl_attr_put_u32(nlh, P_NFTA_IMMEDIATE_DREG,
                     htonl(P_NFT_REG_VERDICT));
    vd = mnl_attr_nest_start(nlh, P_NFTA_IMMEDIATE_DATA);
    dv = mnl_attr_nest_start(nlh, P_NFTA_DATA_VERDICT);
    mnl_attr_put_u32(nlh, P_NFTA_VERDICT_CODE, htonl((uint32_t)code));
    mnl_attr_nest_end(nlh, dv);
    mnl_attr_nest_end(nlh, vd);
    mnl_attr_nest_end(nlh, data);
    mnl_attr_nest_end(nlh, elem);
}

static int rule_add(nft_t* n, int verdict, uint32_t src_ip_be,
                    uint16_t dst_port)
{
    size_t off;
    struct nlmsghdr* nlh;
    struct nlattr* exprs;
    if (!n)
        return -1;
    nlh = msg_open(n, P_NFT_MSG_NEWRULE, P_NFPROTO_INET, 0,
                   (uint16_t)(NLM_F_CREATE | NLM_F_APPEND), &off);
    if (!nlh)
        return -1;
    mnl_attr_put_strz(nlh, P_NFTA_RULE_TABLE, NFT_TABLE_NAME);
    mnl_attr_put_strz(nlh, P_NFTA_RULE_CHAIN, NFT_CHAIN_IN);
    exprs = mnl_attr_nest_start(nlh, P_NFTA_RULE_EXPRESSIONS);
    put_payload(nlh, P_NFT_REG_1, P_NFT_PAYLOAD_NETWORK_HDR, 12, 4);
    put_cmp(nlh, P_NFT_REG_1, &src_ip_be, 4);
    if (dst_port) {
        uint16_t be = htons(dst_port);
        put_payload(nlh, P_NFT_REG_2, P_NFT_PAYLOAD_TRANSP_HDR, 2, 2);
        put_cmp(nlh, P_NFT_REG_2, &be, 2);
    }
    put_verdict(nlh, verdict);
    mnl_attr_nest_end(nlh, exprs);
    msg_close(n, off, nlh);
    return 0;
}

int nft_rule_add_drop(nft_t* n, const char* table, const char* chain,
                      uint32_t src_ip_be, uint16_t dst_port)
{
    (void)table;
    (void)chain;
    return rule_add(n, P_NF_DROP, src_ip_be, dst_port);
}

int nft_rule_add_accept(nft_t* n, const char* table, const char* chain,
                        uint32_t src_ip_be, uint16_t dst_port)
{
    (void)table;
    (void)chain;
    return rule_add(n, P_NF_ACCEPT, src_ip_be, dst_port);
}

/* immune scope fast path: packets carrying the local socket mark are
 * accepted in the prerouting chain, which runs before the ban chain.
 * the mark is local metadata (SO_MARK needs CAP_NET_ADMIN), it cannot
 * arrive from the wire. meta values are host order in registers. */
int nft_rule_add_mark_accept(nft_t* n, const char* table, const char* chain,
                             uint32_t mark)
{
    size_t off;
    struct nlmsghdr* nlh;
    struct nlattr* exprs;
    if (!n)
        return -1;
    nlh = msg_open(n, P_NFT_MSG_NEWRULE, P_NFPROTO_INET, 0,
                   (uint16_t)(NLM_F_CREATE | NLM_F_APPEND), &off);
    if (!nlh)
        return -1;
    mnl_attr_put_strz(nlh, P_NFTA_RULE_TABLE, table);
    mnl_attr_put_strz(nlh, P_NFTA_RULE_CHAIN, chain);
    exprs = mnl_attr_nest_start(nlh, P_NFTA_RULE_EXPRESSIONS);
    put_meta(nlh, P_NFT_REG_1, P_NFT_META_MARK);
    put_cmp(nlh, P_NFT_REG_1, &mark, 4);
    put_verdict(nlh, P_NF_ACCEPT);
    mnl_attr_nest_end(nlh, exprs);
    msg_close(n, off, nlh);
    return 0;
}

int nft_set_add(nft_t* n, const char* table, const char* name)
{
    size_t off;
    struct nlmsghdr* nlh;
    if (!n)
        return -1;
    nlh = msg_open(n, P_NFT_MSG_NEWSET, P_NFPROTO_INET, 0, NLM_F_CREATE,
                   &off);
    if (!nlh)
        return -1;
    mnl_attr_put_strz(nlh, P_NFTA_SET_TABLE, table);
    mnl_attr_put_strz(nlh, P_NFTA_SET_NAME, name);
    /* key type: any value outside the reserved mask reads as
       NFT_DATA_VALUE in the kernel, ipv4 keys are 4 bytes */
    mnl_attr_put_u32(nlh, P_NFTA_SET_FLAGS, htonl(0));
    mnl_attr_put_u32(nlh, P_NFTA_SET_KEY_TYPE, htonl(P_NFT_DATA_IPV4));
    mnl_attr_put_u32(nlh, P_NFTA_SET_KEY_LEN, htonl(4));
    mnl_attr_put_u32(nlh, P_NFTA_SET_ID, htonl(NFT_SET_ID));
    msg_close(n, off, nlh);
    return 0;
}

static int set_elem(nft_t* n, int msg, const char* table,
                    const char* name, uint32_t ip_be, uint16_t flags)
{
    size_t off;
    struct nlmsghdr* nlh;
    struct nlattr* elems;
    struct nlattr* elem;
    struct nlattr* key;
    if (!n)
        return -1;
    nlh = msg_open(n, (uint16_t)msg, P_NFPROTO_INET, 0, flags, &off);
    if (!nlh)
        return -1;
    /* table, set and the transaction set id sit at message level */
    mnl_attr_put_strz(nlh, P_NFTA_SET_ELEM_LIST_TABLE, table);
    mnl_attr_put_strz(nlh, P_NFTA_SET_ELEM_LIST_SET, name);
    mnl_attr_put_u32(nlh, P_NFTA_SET_ELEM_LIST_SET_ID, htonl(NFT_SET_ID));
    elems = mnl_attr_nest_start(nlh, P_NFTA_SET_ELEM_LIST_ELEMENTS);
    elem = mnl_attr_nest_start(nlh, P_NFTA_LIST_ELEM);
    key = mnl_attr_nest_start(nlh, P_NFTA_SET_ELEM_KEY);
    mnl_attr_put(nlh, P_NFTA_DATA_VALUE, 4, &ip_be);
    mnl_attr_nest_end(nlh, key);
    mnl_attr_nest_end(nlh, elem);
    mnl_attr_nest_end(nlh, elems);
    msg_close(n, off, nlh);
    return 0;
}

int nft_set_ban(nft_t* n, const char* table, const char* name,
                uint32_t ip_be)
{
    return set_elem(n, P_NFT_MSG_NEWSETELEM, table, name, ip_be,
                    NLM_F_CREATE);
}

int nft_set_unban(nft_t* n, const char* table, const char* name,
                  uint32_t ip_be)
{
    return set_elem(n, P_NFT_MSG_DELSETELEM, table, name, ip_be, 0);
}

/* drop any unread acks so a failed batch cannot poison the next one */
static void drain_acks(nft_t* n)
{
    char rcv[MNL_SOCKET_BUFFER_SIZE];
    int fd = mnl_socket_get_fd(n->nl);
    while (recv(fd, rcv, sizeof(rcv), MSG_DONTWAIT) > 0)
        ;
}

int nft_commit(nft_t* n)
{
    char rcv[MNL_SOCKET_BUFFER_SIZE];
    int acks = 0;
    int want;
    int rc = 0;

    if (!n || n->len == 0)
        return 0;
    want = n->pending;
    put_batch(n, NFNL_MSG_BATCH_END);
    if (mnl_socket_sendto(n->nl, n->buf, n->len) < 0) {
        n->len = 0;
        n->pending = 0;
        return -1;
    }
    n->len = 0;
    n->pending = 0;
    put_batch(n, NFNL_MSG_BATCH_BEGIN);
    /* drain exactly the acks the batch asked for */
    while (acks < want) {
        int ret = mnl_socket_recvfrom(n->nl, rcv, sizeof(rcv));
        struct nlmsghdr* nlh;
        if (ret <= 0) {
            rc = -1;
            break;
        }
        nlh = (struct nlmsghdr*)rcv;
        while (mnl_nlmsg_ok(nlh, (unsigned int)ret)) {
            if (nlh->nlmsg_type == NLMSG_ERROR) {
                const struct nlmsgerr* e =
                    (const struct nlmsgerr*)mnl_nlmsg_get_payload(nlh);
                if (e->error != 0) {
                    errno = -e->error;
                    rc = -1;
                    break;
                }
                acks++;
            } else if (nlh->nlmsg_type == NLMSG_DONE) {
                acks++;
            }
            nlh = mnl_nlmsg_next(nlh, &ret);
        }
        if (rc != 0)
            break;
    }
    if (rc != 0)
        drain_acks(n);
    return rc;
}
