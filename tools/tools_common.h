#ifndef PROTEUS_TOOLS_COMMON_H
#define PROTEUS_TOOLS_COMMON_H

/*
 * shared helpers of the traffic generators. every generator prints one
 * csv row per event: rel_ms,event,ip,port,result, event in
 * scan,session,brute, result in connected,refused,timeout,data_sent.
 */
#include <stdio.h>
#include <sys/time.h>

#define TOOLS_CSV_HEADER "rel_ms,event,ip,port,result"

double now_ms_wall(void);
/* tcp connect with timeout_ms, fd on success, -1 failure.
 * src_ip may be NULL for the default source address */
int  tcp_connect(const char* ip, int port, int timeout_ms);
int  tcp_connect_src(const char* ip, int port, int timeout_ms,
                     const char* src_ip);
/* advertised port from the status socket, -1 when the channel is down.
 * a client that knows the service follows the move, so a refusal is a
 * rejection rather than a stale port */
int  live_port(const char* path);
void csv_header(void);
void csv_row(double t0, const char* event, const char* ip, int port,
             const char* result);
void args_usage(const char* prog, const char* keys);

#endif
