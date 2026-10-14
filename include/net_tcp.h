#ifndef NET_TCP_H
#define NET_TCP_H

#include "common.h"

/* sub_main's TCP layer (net_tcp.c) */

/* the connection's state, NetTcp_Status (AveTcp_Status) fills it: the lobby's gNetTcpStatus */
typedef struct NetTcpStatus {
    /* 0x0 */ s16 state;                /* 4: connected */
    /* 0x2 */ u16 unk2;
    /* 0x4 */ u16 sendSpace;            /* the lobby reads nothing while it is 0x400 or less */
    /* 0x6 */ u16 recvLen;              /* bytes waiting to be read */
} NetTcpStatus;

#endif
