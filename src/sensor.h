#ifndef PROTEUS_SENSOR_H
#define PROTEUS_SENSOR_H

#include <stdint.h>

#include "trust.h"

/*
 * r2 network sensor: non blocking accept on the moving listener, peer
 * classification by payload. a connection that sends data inside the
 * handshake window is a legitimate session, one that closes silently or
 * stays silent is a scan. the daemon feeds both into the nervous system:
 * scans push the ash envelope and the scan rate counter, sessions feed
 * the legit share. the userspace ban list is always active, the nft
 * backend mirrors it when it is up. the network input is not part of the
 * bitwise determinism contract: journals recorded with --listen record
 * the observed input but cannot be replayed.
 */
#define SENSOR_HANDSHAKE_MS 300
#define SESSION_MAX_MS      10000
#define MAX_PEERS 16
#define MAX_BANS 64
#define SENSOR_BUF 128
#define SENSOR_TRUST_MS 60000.0
#define SENSOR_TRUST_IPS 8
/* repeated failed auth from one source is credential stuffing, an
   application level attack. the count is immune scope state, the ban
   still comes from the connectome detector. */
#define SENSOR_FAIL_MS   10000.0
#define SENSOR_FAIL_MAX  8
#define SENSOR_FAIL_IPS  16

typedef struct {
    uint32_t ip;          /* network byte order */
    double   until_ms;    /* ban expiry, wall clock of the session */
} ban_t;

typedef struct {
    uint32_t ip;          /* a source that presented a live token */
    double   until_ms;
} trust_ip_t;

typedef struct {
    uint32_t ip;
    int      count;       /* failed auth lines inside the window */
    double   first_ms;
} fail_t;

typedef struct {
    int      fd;
    uint32_t ip;
    double   born_ms;
    int      fed;         /* payload seen: legitimate session */
} peer_t;

typedef struct sensor sensor_t;

sensor_t* sensor_init(void);
/* wire in one engine tick: accept on fd, classify, expire. event codes:
 * 1 scan event, 2 session event. *last_scan_ip carries the newest
 * scanning peer */
int  sensor_tick(sensor_t* sn, int fd, double t_ms, trust_t* tr,
                 uint32_t* last_scan_ip, int* any_scan, int* any_session);
/* 1 when the source presented a live registration token recently */
int  sensor_trusted(const sensor_t* sn, uint32_t ip, double t_ms);
/* ban an ip for the ttl, 1 when the list changed */
int  sensor_ban(sensor_t* sn, uint32_t ip, double t_ms);
int  sensor_is_banned(const sensor_t* sn, uint32_t ip, double t_ms);
void sensor_close(sensor_t* sn);

#endif
