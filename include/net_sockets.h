#ifndef NET_SOCKETS_H
#define NET_SOCKETS_H

#include "common.h"

/*
 * The BSD-style socket calls the HTTP library uses, over AVE's TCP and DNS
 * (net_socket, net_connect, net_recv, net_send, net_close,
 * net_gethostbyname, net_inet_addr), with their cache of looked-up hosts and
 * the socket semaphores that Net_InitSockets creates.
 */

s32 net_inet_addr(s8 *str);
s32 *net_gethostbyname(s8 *name);
void net_clear_host_cache(void);
void Net_InitSockets(void);
extern s32 gNetSockSema;

#endif
