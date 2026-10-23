#ifndef AVE_H
#define AVE_H

#include "common.h"

/*
 * NETWORK: the EE side of the AVE TCP wrapper, an IOP module reached by
 * SIF RPC. Each Ave* function fills the shared AveWrapBuf, sends it as
 * one AVE_FN_* call and reads the reply back from the same buffer.
 */

/*
 * The IOP wrapper's call numbers (AveWrap_Call's and AveWrap_CallAsync's
 * first argument), each named for the wrapper function that sends it.
 */
#define AVE_FN_IF_SET_ADDRESS        0x01
#define AVE_FN_IF_CLEAR_ADDRESS      0x02
#define AVE_FN_TCP_SET_OPTION        0x03
#define AVE_FN_TCP_GET_OPTION        0x04
#define AVE_FN_IF_UP                 0x05
#define AVE_FN_IF_DOWN               0x06
#define AVE_FN_TCP_OPEN              0x07
#define AVE_FN_TCP_ABORT             0x0D
#define AVE_FN_TCP_STATUS            0x0F
#define AVE_FN_TCP_SEND              0x10
#define AVE_FN_TCP_RECV              0x13
#define AVE_FN_TCP_DELETE            0x16
#define AVE_FN_ROUTE_ADD             0x1E
#define AVE_FN_ROUTE_DELETE          0x1F
#define AVE_FN_PPP_INIT              0x21
#define AVE_FN_PPP_TERM              0x22
#define AVE_FN_PPP_CONNECT           0x23
#define AVE_FN_PPP_DISCONNECT        0x24
#define AVE_FN_PPP_GET_STATUS        0x25
#define AVE_FN_PPP_GET_LINK_STATE    0x26
#define AVE_FN_PPP_SET_OPTION        0x27
#define AVE_FN_ETH_INIT              0x29
#define AVE_FN_ETH_TERM              0x2A
#define AVE_FN_ETH_START             0x2B
#define AVE_FN_ETH_IS_LINK_UP        0x2C
#define AVE_FN_ETH_STOP              0x2D
#define AVE_FN_DHCP_INIT             0x2E
#define AVE_FN_DHCP_TERM             0x2F
#define AVE_FN_DHCP_GET_DNS          0x30
#define AVE_FN_DHCP_GET_ROUTER       0x31
#define AVE_FN_DHCP_START            0x36
#define AVE_FN_DHCP_GET_ADDRESS      0x37
#define AVE_FN_DHCP_GET_STATE        0x38
#define AVE_FN_DHCP_RELEASE          0x3A
#define AVE_FN_DNS_INIT              0x3B
#define AVE_FN_DNS_TERM              0x3C
#define AVE_FN_DNS_REQUEST           0x3D
#define AVE_FN_DNS_FREE              0x3E
#define AVE_FN_DNS_LOOKUP            0x3F
#define AVE_FN_IF_POLL_EVENT         0x46
#define AVE_FN_IF_GET_EVENT          0x47
#define AVE_FN_IF_ACK_EVENT          0x48
#define AVE_FN_IF_GET_OPTION         0x4A

#define AVE_WRAP_RPC_ID 0x01270030  /* the SIF RPC server of the IOP wrapper (AveWrap_Bind) */
#define AVE_ERR_RPC (-0x21)         /* the RPC call failed, or a call is still running */
#define AVE_DNS_PENDING (-2)        /* AveDns_LookUp: no answer yet */
#define AVE_TCP_STATE_NONE (-1)     /* AveTcp_Status's state -1 */
#define AVE_TCP_CHUNK_MAX 0x3CA     /* the most bytes one AveTcp_Send piece or AveTcp_Recv call carries */

/* The connection settings NetPpp_Connect builds and AvePpp_Connect sends. */
typedef struct AveConfig {
    u8 unk0;            /* 0x00: always 1 */
    u8 phyMode;         /* 0x01: the Ethernet link mode (gNetPhyMode); sent as arg[0] */
    s16 authFlag;       /* 0x02: the dial settings' flag, 0 or 1; NetPpp_TryOtherFlag swaps it on a redial */
    char *chat0;        /* 0x04: the modem's first chat script */
    char *chat1;        /* 0x08: the modem's second chat script */
    char *confText;     /* 0x0C: a text after the chat scripts, always empty */
    char *authName;     /* 0x10: the account name */
    char *authKey;      /* 0x14: the account's password */
    char *authText;     /* 0x18: the authentication text, emptied unless the kind is 1 */
    s16 modemOpt[5];    /* 0x1C: the five modem options, each mapped to the wrapper's values */
    u8 isPppoe;         /* 0x26: 1 for PPPoE; sent as arg[1] */
    u8 lineMode;        /* 0x27: the line mode, 0 to 3 sent as 0, 1, 2, 4; sent as arg[2] */
} AveConfig;

/* AvePpp_Connect's payload: AveConfig with its strings copied in. */
typedef struct AveConfigArgs {
    s16 unk0;               /* 0x000: AveConfig.unk0 */
    s16 authFlag;           /* 0x002 */
    char chat0[0x100];      /* 0x004 */
    char chat1[0x100];      /* 0x104 */
    char confText[0x40];    /* 0x204 */
    char authName[0x40];    /* 0x244 */
    char authKey[0x40];     /* 0x284 */
    char authText[0x40];    /* 0x2C4 */
    s16 modemOpt[5];        /* 0x304 */
} AveConfigArgs;

/* The DNS part of AveInfo. */
typedef struct AveInfoSub {
    s32 unk0;
    s32 unk4;
    s32 unk8;               /* 0x08: copied out by NetPpp_GetInfo; nothing reads it */
    s32 unkC;               /* 0x0C: the same */
    s32 primaryDns;         /* 0x10 */
    s32 secondaryDns;       /* 0x14 */
} AveInfoSub;

/* The connection's status, AvePpp_GetStatus's reply. */
typedef struct AveInfo {
    s32 unk0;               /* 0x00: copied out; nothing reads it */
    s32 localAddr;          /* 0x04: the address the connection was given */
    s16 phase;              /* 0x08: 0-9, 4 = up */
    s16 unkA;
    s32 speed;              /* 0x0C: the link speed (bps) */
    s16 unk10;              /* 0x10: copied out; nothing reads it */
    s16 error;              /* 0x12: why the connection went down, 0-13 */
    AveInfoSub sub;         /* 0x14 */
} AveInfo;

/* The payload of a call (AveWrapBuf.data): one member for each call's layout. */
typedef union AvePayload {
    struct {                /* AveIf_SetAddress, and AveDhcp_GetAddress's reply */
        s32 address;
        s32 netmask;
        s32 broadcast;
    } ifAddr;
    struct {                /* AveTcp_SetOption, AveTcp_GetOption */
        s16 sock;
        s16 option;
        s16 len;
        u8 data[0x100];
    } tcpOpt;
    struct {                /* AveTcp_Open */
        s32 addr;
        u16 port;
        u16 openArg;
    } tcpOpen;
    struct {                /* AveTcp_Status: the socket, and the reply's fields */
        s16 sock;
        u16 state;
        u16 unk4;
        u16 sendSpace;
        u16 recvLen;
    } tcpStatus;
    struct {                /* AveTcp_Send, AveTcp_Recv, and the socket of AveTcp_Abort and AveTcp_Delete */
        s16 sock;
        s16 len;
        u8 data[AVE_TCP_CHUNK_MAX];
    } tcpIo;
    struct {                /* AveRoute_Add, AveRoute_Delete */
        s32 dest;
        s32 gateway;
    } route;
    AveConfigArgs pppConfig;    /* AvePpp_Connect */
    AveInfo pppStatus;          /* AvePpp_GetStatus's reply */
    s32 linkState;              /* AvePpp_GetLinkState's reply */
    struct {                /* AvePpp_SetOption */
        s16 option;
        s16 len;
        u8 data[0x100];
    } pppOpt;
    struct {                /* AveDns_Init, and AveDhcp_GetDns's reply */
        char domain[0x100];
        s32 primaryDns;
        s32 secondaryDns;
    } dns;
    s32 router;                 /* AveDhcp_GetRouter's reply */
    struct {                /* AveDhcp_Start, AveDhcp_Release */
        s32 address;
        s32 dhcpArg;
    } dhcp;
    char dnsName[0x200];        /* AveDns_Request */
    struct {                /* AveDns_Free, AveDns_LookUp */
        s16 id;
        s16 pad2;
        s32 addr;           /* the reply's address */
    } dnsLookUp;
    struct {                /* AveIf_GetEvent's reply */
        s32 status;
        s32 eventArgA;
        s32 eventArgB;
    } ifEvent;
    struct {                /* AveIf_GetOption */
        s32 option;
        s32 len;
        u8 data[0x100];
    } ifOpt;
    u8 raw[0x3E4];
} AvePayload;

/*
 * The buffer of every call, sent to the wrapper and overwritten by its
 * reply. A call's sizes count the 28 bytes before the payload.
 */
typedef struct AveWrapBuf {
    s32 arg[3];             /* 0x00: the call's argument words */
    u8 unkC[0xC];
    union {
        s32 len;            /* AveTcp_Recv: the bytes received */
        s16 code;           /* the call's result: negative is an error */
    } result;               /* 0x18 */
    AvePayload data;        /* 0x1C */
} AveWrapBuf;

s32 AveWrap_Bind(void);
s32 AveWrap_Unbind(void);
s16 AveWrap_CallAsync(s32 funcNo, s32 sendSize, s32 recvSize);
s32 AveWrap_IsIdle(void);
void AveWrap_Lock(void);
void AveWrap_Unlock(void);
s16 AveIf_Up(s32 ifFlag, s32 ifKind);
s16 AveIf_Down(void);
s16 AveIf_SetAddress(s32 address, s32 netmask, s32 broadcast);
s16 AveIf_ClearAddress(void);
s16 AveTcp_SetOption(s16 sock, s16 option, s16 len, void *src);
s16 AveTcp_GetOption(s16 sock, s16 option, s16 len, void *dst);
s16 AveTcp_Open(s32 addr, u16 port, u16 openArg);
s16 AveTcp_Abort(s16 sock);
s16 AveTcp_Status(s16 sock, u16 *state, u16 *sendSpace, u16 *recvLen);
s16 AveTcp_Send(s16 sock, u8 *data, s16 len);
s16 AveTcp_Recv(s16 sock, void *buf, s16 len);
s16 AveTcp_Delete(s16 sock);
s16 AveRoute_Add(s32 dest, s32 gateway);
s16 AveRoute_Delete(s32 dest, s32 gateway);
s16 AvePpp_Init(void);
s16 AvePpp_Term(void);
s16 AvePpp_Connect(AveConfig *settings);
s16 AvePpp_Disconnect(void);
s16 AvePpp_GetStatus(AveInfo *out);
s16 AvePpp_GetLinkState(s32 *out);
s16 AveEth_Init(void);
s16 AveEth_Term(void);
s16 AveEth_Start(s16 phyMode);
s16 AveEth_IsLinkUp(void);
s16 AveEth_Stop(void);
s16 AveDhcp_Init(void);
s16 AveDhcp_Term(void);
s16 AveDhcp_GetDns(char *domain, s32 *primaryDns, s32 *secondaryDns);
s16 AveDhcp_GetRouter(s32 *out);
s16 AveDhcp_Start(s32 address, s32 dhcpArg);
s16 AveDhcp_GetAddress(s32 *address, s32 *netmask, s32 *broadcast);
s16 AveDhcp_GetState(void);
s16 AveDhcp_Release(s32 address, s32 dhcpArg);
s16 AveDns_Init(char *domain, s32 primaryDns, s32 secondaryDns);
s16 AveDns_Term(void);
s16 AveDns_Free(s16 id);
s16 AveDns_LookUp(s16 id, s32 *addr);
s16 AveIf_PollEvent(s32 *out);
s16 AveIf_GetEvent(s32 selector, s32 *status, s32 *eventArgA, s32 *eventArgB);
s16 AveIf_AckEvent(s32 status);
s16 AveIf_GetOption(s32 option, s32 len, void *dst);

#endif /* AVE_H */
