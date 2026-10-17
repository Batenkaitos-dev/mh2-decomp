#ifndef NET_TCP_H
#define NET_TCP_H

#include "common.h"

/* sub_main's TCP layer (net_tcp.c) */

/* the connection's state, NetTcp_Status (AveTcp_Status) fills it: the lobby's gNetTcpStatus */
typedef struct NetTcpStatus {
    /* 0x0 */ s16 state;                /* NET_TCP_STATE_*: AVE's state, copied */
    /* 0x2 */ u16 unk2;
    /* 0x4 */ u16 sendSpace;            /* the lobby reads nothing while it is 0x400 or less */
    /* 0x6 */ u16 recvLen;              /* bytes waiting to be read */
} NetTcpStatus;

/*
 * NetTcpStatus.state: NetTcp_Status copies AVE's states 0-6 and 8-10, makes
 * AVE's -1 into 11 and anything else into 7. Only one value's meaning is
 * shown by a use: the lobby sends and reads only in state 4.
 */
#define NET_TCP_STATE_CONNECTED 4
#define NET_TCP_STATE_OTHER 7 /* a state AVE gave that NetTcp_Status doesn't know */

#endif
