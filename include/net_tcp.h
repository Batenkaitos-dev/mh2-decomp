#ifndef NET_TCP_H
#define NET_TCP_H

#include "common.h"

/* sub_main's TCP layer (net_if.c) */

/*
 * The dial-up settings NetPpp_Connect reads. Net_ConnectPppStep lays them
 * out in gNetPppConf: NetAuthCfg at its start, the modem options after
 * it, then NetDialCfg and NetDialArg.
 */
typedef struct NetModemCfg {
    s16 opt[5];         /* the five modem options; opt[0] is 0-6 */
} NetModemCfg;

typedef struct NetAuthCfg {
    s16 flag;           /* 0 or 1; NetPpp_TryOtherFlag swaps it */
    s16 authKind;       /* NET_PPP_AUTH_* */
    char *authText;
} NetAuthCfg;

typedef struct NetDialCfg {
    NetModemCfg *modem; /* 0x00 */
    NetAuthCfg *auth;   /* 0x04 */
    s16 *lineMode;      /* 0x08: gNetPppLineMode */
    char *chat0;        /* 0x0C: the modem's chat scripts (NetModem_GetChat) */
    char *chat1;        /* 0x10 */
    char *confText;     /* 0x14: a text after the chat scripts, always empty */
    char *authName;     /* 0x18 */
    char *authKey;      /* 0x1C */
} NetDialCfg;

typedef struct NetDialArg {
    NetDialCfg *cfg;    /* 0x00 */
    s16 unk4;           /* set to 0; NetPpp_Connect doesn't read it */
    s8 isPppoe;         /* 0x06: gNetLanMode == NET_LAN_PPPOE */
    s8 phyMode;         /* 0x07: gNetPhyMode */
} NetDialArg;

/* The connection's status as NetPpp_GetStatus gives it. */
typedef struct NetConnInfo {
    s32 localAddr;      /* 0x00: the address the connection was given */
    s32 unk4;           /* copied from AVE's status; nothing reads it */
    s32 speed;          /* 0x08: the link speed (bps) */
    s16 phase;          /* 0x0C: NET_PPP_PHASE_* */
    s16 unkE;           /* copied from AVE's status; nothing reads it */
    s16 error;          /* 0x10: why it went down (NET_PPP_ERR_*) */
} NetConnInfo;

/* A host to connect to (NetTcp_Open). */
typedef struct NetAddr {
    s32 addr;
    u16 port;
    u16 openArg;        /* sent with them to AveTcp_Open; seen: 0 */
} NetAddr;

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
#define NET_TCP_STATE_ERROR 11 /* AVE's state AVE_TCP_STATE_NONE; nothing reads it */

/* The connect and disconnect state machine's state (gNetMsgState, 0x12
 * bytes): NetMsg_Update passes its fields to the handler of the phase,
 * gNetMsgHandlers[phase]; Net_ClearState clears it. */
typedef struct NetMsgState {
    /* 0x00 */ u8 unk0;                 /* the lobby sets 4 here; nothing in sub_main reads it */
    /* 0x01 */ s8 handler;              /* NET_HANDLER_*: which state machine runs */
    /* 0x02 */ s8 step;                 /* its state */
    /* 0x03 */ s8 discStep;             /* Net_DisconnectStep's state */
    /* 0x04 */ s16 timer;
    /* 0x06 */ u16 cancelHold;           /* raised by the lobby while Start is held; over NET_CANCEL_AT cancels */
    /* 0x08 */ u16 retries;              /* redials left */
    /* 0x0A */ u16 retries2;             /* redials left after PPP error 12 */
    /* 0x0C */ u8 progress[2];          /* gNetProgress: the steps done and their count */
    /* 0x0E */ s8 err;                  /* the error the lobby reports */
    /* 0x0F */ u8 pad0F[3];
} NetMsgState;
typedef char NetMsgState_size_check[(sizeof(NetMsgState) == 0x12) ? 1 : -1];

/* The socket layer's errno values (gNetErrno), in newlib's numbering. */
#define NET_EBADF 9                 /* a bad or free socket */
#define NET_ENOMEM 0xC              /* no free socket */
#define NET_EINVAL 0x16             /* not a stream socket */
#define NET_ECONNRESET 0x68
#define NET_ECONNABORTED 0x71       /* cancelled by net_cancel */
#define NET_EHOSTUNREACH 0x76       /* the AVE TCP open failed */

/* The sockets (gNetSockHandles: the AVE TCP handle, or one of these). */
#define NET_SOCK_MAX 8
#define NET_SOCK_FREE -1
#define NET_SOCK_OPENED -2          /* made by net_socket, not connected yet */
#define NET_POLL_TRIES 3000         /* the polls a socket call waits at most */
#define NET_POLL_USEC 20000         /* 20 ms between polls */

/* NetDevice.type. */
#define NET_DEV_ETHERNET 1          /* a LAN adaptor (AN986, LANEGG or SMAP driver) */
#define NET_DEV_MODEM 2             /* a modem, through PPP */
#define NET_DEV_PPP_OTHER 3         /* PPP like a modem, with no driver loaded here */

/* NetDevice.driver: which IRX drives a device. */
#define NET_DRV_AN986 1
#define NET_DRV_CXTMDM 2
#define NET_DRV_OSTMDM 4
#define NET_DRV_SMAP 5              /* the network adaptor */
#define NET_DRV_LANEGG 6

/* gNetLanMode, as the settings give it. */
#define NET_LAN_STATIC 0
#define NET_LAN_DHCP 1
#define NET_LAN_PPPOE 2

/* NetMsgState.handler: the state machine NetMsg_Update runs. */
#define NET_HANDLER_PPP 0           /* Net_ConnectPppStep, which hands a static or DHCP LAN over */
#define NET_HANDLER_LAN 1           /* Net_ConnectLanStep */
#define NET_HANDLER_DISCONNECT 2    /* Net_Disconnect */

/* What a handler returns (0 while it is busy). */
#define NET_STEP_CONNECTED 2
#define NET_STEP_STOPPED 4          /* the lobby shows NetMsgState.err */
#define NET_STEP_REDIAL 5           /* after a disconnect, the connection starts again */

/* NetMsgState.err: why the connection stopped. */
#define NET_ERR_PPP_LINE -2         /* PPP error 12 once the redials are spent, or 13 */
#define NET_ERR_PPP_FLAG -3         /* PPP error 2 with both flag values tried */
#define NET_ERR_PPP_E3 -5           /* PPP error 3 */
#define NET_ERR_NO_DEVICE -0x51     /* no device, the link lost, or no modem link */
#define NET_ERR_FAILED -0x52        /* a driver call failed */
#define NET_ERR_IF_OPT8 -0x53       /* interface option 8 missing or 0 */
#define NET_ERR_DHCP -0x54          /* DHCP failed 3 times */
#define NET_ERR_PPP_E6 -0x55        /* PPP error 6 */
#define NET_ERR_CANCELED -0x56      /* cancelHold passed NET_CANCEL_AT */
#define NET_ERR_PPP_E10 -0x57       /* PPP error 10 */
#define NET_ERR_NO_LINK -0x58       /* no Ethernet link after NET_LINK_WAIT frames, 3 times */
#define NET_ERR_DHCP_TIMEOUT -0x59  /* DHCP failed with its progress at the cap */
#define NET_CANCEL_AT 30            /* the cancel count that stops a connection or a DNAS check */
#define NET_LINK_WAIT 450           /* frames: 15 s for the Ethernet link */

/* gNetPppError: why AVE's PPP went down, where the connection handles it. */
#define NET_PPP_ERR_RETRY 1         /* redial while retries last */
#define NET_PPP_ERR_FLAG 2          /* redial with the other flag (NetPpp_TryOtherFlag) */
#define NET_PPP_ERR_LINEMODE 5      /* redial with the next line mode (NetPpp_NextLineMode) */
#define NET_PPP_ERR_BUSY 12         /* redial while retries2 last */

/* gNetPppPhase. */
#define NET_PPP_PHASE_DOWN 0
#define NET_PPP_PHASE_UP 4
#define NET_PPP_PHASE_OTHER 8       /* a phase AVE gave that NetPpp_GetStatus doesn't know */
#define NET_PPP_ERR_OTHER 11        /* a PPP error NetPpp_GetStatus doesn't know */

/* NetPppConf.authKind: whether the authentication text is sent */
#define NET_PPP_AUTH_NONE 0         /* an empty text */
#define NET_PPP_AUTH_TEXT 1         /* gNetPppAuthText */

/* gNetIfState: the interface's state, which NetIf_GetState returns */
#define NET_IF_DOWN 0               /* NetTcp_Init, NetIf_Down */
#define NET_IF_UP 1                 /* NetIf_Up */
#define NET_IF_CONNECTING 2         /* NetPpp_Connect, NetEth_Start */
#define NET_IF_CONNECTED 3          /* NetPpp_SetConnected, NetEth_SetConnected */
#define NET_IF_CLOSED 4             /* NetPpp_SetClosed, NetEth_ClearAddress */
#define NET_IF_STOPPING 5           /* NetPpp_Disconnect, NetEth_Stop */

/* gNetIfError: why NetIf_PollError saw the connection fail */
#define NET_IF_ERR_QUERY 1          /* the DHCP or PPP status query failed */
#define NET_IF_ERR_LINK_LOST 2      /* an interface event, or the PPP phase no longer up */

/* gNetIfStarted's flags: which parts are started, so that each is stopped once */
#define NET_STARTED_IF 0            /* NetIf_Up, NetIf_Down */
#define NET_STARTED_PPP 1           /* NetPpp_Init, NetPpp_Term */
#define NET_STARTED_ETH 2           /* NetEth_Init, NetEth_Term */
#define NET_STARTED_CONNECTED 3     /* connected, until closed */
#define NET_STARTED_DNS 4           /* NetDns_Init, NetDns_Term */
#define NET_STARTED_ROUTE 5         /* NetRoute_Add, NetRoute_Delete */
#define NET_STARTED_DHCP 6          /* NetDhcp_Init, NetDhcp_Term */
#define NET_STARTED_COUNT 7

/* The NetDns_* results */
#define NET_DNS_ERROR (-1)          /* AVE_ERR_RPC or AVE's -1 */
#define NET_DNS_FAILED (-2)         /* NetDns_LookUp: the name has no answer; the caller gives up */
#define NET_DNS_PENDING (-3)        /* NetDns_LookUp: no answer yet */

/* Dnas_Step's kind, and gDnasResult when the check was cancelled. */
#define DNAS_KIND_AUTH 0            /* the authentication at login */
#define DNAS_KIND_INST 1            /* the check before a patch download */
#define DNAS_RESULT_CANCELED 0x7F   /* cancelled, or the disc did not get ready */

#endif
