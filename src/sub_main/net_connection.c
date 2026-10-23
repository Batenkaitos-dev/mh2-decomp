/*
 * The network connection: the resolver's set-up (Net_InitDns) and host
 * lookups by steps (Net_ResolveHost, with its cache), then the state
 * machines NetMsg_Update runs from gNetMsgState, one frame at a time:
 * the PPP connection (Net_ConnectPppStep, a modem or PPPoE), the
 * Ethernet one (Net_ConnectLanStep, static or DHCP) and the disconnect
 * (Net_Disconnect, Net_DisconnectStep). gNetMsgHandlers, this file's
 * data, lists the three handlers. The states, errors and kinds are in
 * net_tcp.h. Networking code stays byte-identical.
 */

#include "common.h"
#include "yn.h"                /* NetDevice, gNetDevs */
#include "net_tcp.h"
#include "arena.h"
#include "net_connection.h"

/* The game built this file at optimization level 4. */
#pragma optimization_level 4

void strncpy(char *dst, const char *src, u32 n);
void * memset(void *p, s32 c, u32 n);
extern s32 gNetHostCacheNext;
extern s16 gNetPppLineMode;
extern s32 gNetPppStatus;
extern u8 gNetTcpStatus;
extern s32 gNetSock;
extern NetMsgState gNetMsgState;
extern s32 gNetGateway;
extern s32 gNetNetmask;
extern s32 gNetIpAddr;
extern u8 gNetPppLineModeFirst;
extern u8 gNetPhyMode;
extern u8 gNetAuthKey;
extern u8 gNetAuthName;
extern s32 gNetDns2;
extern s32 gNetDns1;
extern s8 gNetDomain;
extern s8 gNetPppAuthText;
extern u8 gNetPppConfBuf;
extern u8 gNetPppFlagSaved;
extern s8 gNetLanMode;
extern NetDevice *gNetDevCur;   /* the device in use (in gNetDevs) */
/* The host name cache (8 entries, a ring: gNetHostCacheNext the next one). */
typedef struct HostEntry {
    s8 name[256];             /* 0x000 */
    s32 addr;                   /* 0x100 */
} HostEntry;

extern HostEntry gNetHostCache[8];
s32 atoi(s8 *s);             /* atoi */
u32 strlen(s8 *s);
/* A copy of net_inet_addr (inet_addr): the address in dotted form (in network order), 0 when str is not one (a run of
 * more than 3 digits, 4 dots or another character). */
s32 net_inet_addr2(s8 *str)
{
    s32 dots = 0;
    s32 digits = 0;
    s8 *cursor;
    u32 i;
    s32 ch;
    u8 oct0;
    u8 oct1;
    u8 oct2;
    u8 oct3;

    for (cursor = str; (ch = *cursor); cursor++) {
        if (ch >= '0' && ch <= '9') {
            if (++digits == 4) {
                return 0;
            }
        } else if (ch == '.') {
            if (++dots == 4) {
                return 0;
            }
            digits = 0;
        } else {
            return 0;
        }
    }
    i = 0;
    oct0 = atoi(str);
    for (; i < strlen(str) && str[i] != '.'; i++) {
    }
    if (i < strlen(str)) {
        i++;
    }
    oct1 = atoi(str + i);
    for (; i < strlen(str) && str[i] != '.'; i++) {
    }
    if (i < strlen(str)) {
        i++;
    }
    oct2 = atoi(str + i);
    for (; i < strlen(str) && str[i] != '.'; i++) {
    }
    if (i < strlen(str)) {
        i++;
    }
    oct3 = atoi(str + i);
    return ((oct3 << 24) & 0xFF000000) | (((oct2 << 16) & 0xFF0000) | ((oct0 & 0xFF) | ((oct1 << 8) & 0xFF00)));
}

/* Empties Net_ResolveHost's cache (gNetHostCache). Returns 0. */
s32 Net_ClearHostCache(void)
{
    memset(gNetHostCache, 0, 2080U);
    gNetHostCacheNext = 0;
    return 0;
}

extern s16 gNetDnsReq;
s32 strncmp(s8 *a, s8 *b, s32 n);
s32 NetDns_Request(s8 *name);

/* A step of a host lookup (state *st): 0 takes the address from net_inet_addr2 or the cache, else
 * starts a DNS request (gNetDnsReq) for state 1; 1 polls it and caches the answer. 1 with the
 * address in *out, 0 while waiting, -1 on an error. */
s32 Net_ResolveHost(s8 *step, s16 *timer, s32 *out, s8 *name)
{
    s32 addr;
    s32 i;
    HostEntry *entry;

    switch (*step) {
    case 0:
        *out = net_inet_addr2(name);
        if (*out != 0) {
            *step = 0;
            *timer = 0;
            return 1;
        }
        for (i = 0, entry = gNetHostCache; i < 8; i++, entry++) {
            if (strncmp(entry->name, name, 256) == 0) {
                *step = 0;
                *timer = 0;
                *out = entry->addr;
                return 1;
            }
        }
        gNetDnsReq = NetDns_Request(name);
        if (gNetDnsReq == -1) {
            return -1;
        }
        (*step)++;
        *timer = 0;
        break;
    case 1:
        switch (NetDns_LookUp(gNetDnsReq, &addr)) {
        case -3:
            return 0;
        default:
        case -2:
        case -1:
            NetDns_Free(gNetDnsReq);
            *step = 0;
            *timer = 0;
            return -1;
        case 1:
            NetDns_Free(gNetDnsReq);
            *step = 0;
            *timer = 0;
            *out = addr;
            entry = &gNetHostCache[gNetHostCacheNext];
            strncpy((char *)entry->name, (char *)name, 256);
            entry->addr = addr;
            gNetHostCacheNext++;
            gNetHostCacheNext %= 8;
            return 1;
        }
        break;
    }
    return 0;
}

extern s32 gNetDhcpBuf[2];

/* NETWORK: starts the resolver (NetDns_Init) with the domain name gNetDomain and the DNS servers
 * gNetDns1 / gNetDns2, or where they are 0 those from PPP (NetPpp_GetInfo) or DHCP
 * (NetDhcp_GetDns, LAN mode 1), then clears the host cache (Net_ClearHostCache). */
void Net_InitDns(void)
{
    s32 pppInfo[8];
    s32 dns1;
    s32 dns2;
    s32 autoDns1 = 0;
    s32 autoDns2 = 0;

    if (gNetDevCur == 0) {
        return;
    }
    if (gNetDns1 == 0 || gNetDns2 == 0) {
        switch (gNetDevCur->type) {
        case NET_DEV_MODEM:
            NetPpp_GetInfo(pppInfo);
            autoDns1 = pppInfo[2];
            autoDns2 = pppInfo[3];
            break;
        case NET_DEV_ETHERNET:
            switch (gNetLanMode) {
            case 2:
                NetPpp_GetInfo(pppInfo);
                autoDns1 = pppInfo[2];
                autoDns2 = pppInfo[3];
                break;
            case 0:
                break;
            case 1:
                NetDhcp_GetDns((char *)&gNetDomain, gNetDhcpBuf);
                autoDns1 = gNetDhcpBuf[0];
                autoDns2 = gNetDhcpBuf[1];
                break;
            }
            break;
        }
    }
    dns1 = gNetDns1 ? gNetDns1 : autoDns1;
    dns2 = gNetDns2 ? gNetDns2 : autoDns2;
    NetDns_Init((char *)&gNetDomain, dns1, dns2);
    Net_ClearHostCache();
}

/*
 * Clears the connect state machine (gNetMsgState), the PPP status and the TCP status, and
 * forgets the lobby socket (gNetSock = -1).
 */
void Net_ClearState(void)
{
    memset(&gNetMsgState, 0, sizeof(NetMsgState));
    memset(&gNetPppStatus, 0, 20U);
    memset(&gNetTcpStatus, 0, 8U);
    gNetSock = -1;
}

/*
 * Runs one step of the current connect or disconnect handler on gNetMsgState's fields.
 * Returns the handler's result.
 */
s32 NetMsg_Update(void)
{
    return NetMsg_Dispatch(&gNetMsgState.handler, &gNetMsgState.step, &gNetMsgState.discStep,
                           &gNetMsgState.timer, &gNetMsgState.cancelHold, &gNetMsgState.retries,
                           &gNetMsgState.retries2, &gNetMsgState.err);
}

/*
 * For an Ethernet device, checks for a link event; if there is one and the interface accepts
 * it, the event is stored in the device's status. Returns 0 if all is well (always 0 for
 * other device types), -1 when the event can't be read or acknowledged (status becomes -1)
 * or the poll fails.
 */
s32 NetDev_CheckLink(void)
{
    s32 event[1];
    s32 status[1];
    s32 res;

    if (gNetDevCur->type != NET_DEV_ETHERNET) {
        return 0;
    }
    res = NetIf_PollEvent((s32 *) event);
    switch (res) {
    case 0:
        return 0;
    case 1:
        if ((NetIf_GetEvent(0, (s32 *) status) > 0) && (NetIf_GetState() == 0) && (NetIf_AckEvent(status[0]) == 0)) {
            gNetDevCur->status = status[0];
            return 0;
        }
        gNetDevCur->status = -1;
    default:
        return -1;
    }
}

/*
 * The dial-up (PPP) settings Net_ConnectPppStep fills for NetPpp_Connect
 * (gNetPppConf): the records NetPpp_Connect reads, with dial.modem, dial.auth
 * and arg.cfg pointing back into the same object.
 */
typedef struct NetPppConf {
    /* 0x00 */ NetAuthCfg auth;     /* auth.flag: NetPpp_TryOtherFlag toggles it */
    /* 0x08 */ NetModemCfg opts;    /* 4, 0, 0, 1, 1 */
    /* 0x12 */ u8 pad12[6];
    /* 0x18 */ NetDialCfg dial;
    /* 0x38 */ NetDialArg arg;      /* what NetPpp_Connect gets */
} NetPppConf;
extern NetPppConf gNetPppConf;

/*
 * Before a redial after PPP error 2: sets gNetPppConf.auth.flag to the value gNetPppFlagSaved
 * does not hold. Returns 0 when it changed it, -1 when the other value was already in use
 * (both failed).
 */
s32 NetPpp_TryOtherFlag(void)
{
    if (gNetPppFlagSaved == 1) {
        if (gNetPppConf.auth.flag == 1) {
            gNetPppConf.auth.flag = 0;
            return 0;
        }
        goto block_6;
    }
    if (gNetPppConf.auth.flag == 0) {
        gNetPppConf.auth.flag = 1;
        return 0;
    }
block_6:
    return -1;
}

/*
 * Saves the gNetPppConf.auth.flag a connection worked with in gNetPppFlagSaved, so the next
 * dial starts with it. Returns 0.
 */
s32 NetPpp_SaveFlag(void)
{
    gNetPppFlagSaved = (u8) gNetPppConf.auth.flag;
    return 0;
}

/*
 * Before a redial after PPP error 5: moves gNetPppLineMode on (1, 3, 2, 0, then 1).
 * Returns 0, or -1 once it is back at gNetPppLineModeFirst (every mode failed).
 */
s32 NetPpp_NextLineMode(void)
{
    switch (gNetPppLineMode) {
    case 1:
        gNetPppLineMode = 3;
        break;
    case 3:
        gNetPppLineMode = 2;
        break;
    case 2:
        gNetPppLineMode = 0;
        break;
    case 0:
        gNetPppLineMode = 1;
        break;
    }
    return -(gNetPppLineModeFirst == gNetPppLineMode);
}

typedef s32 (*NetMsgFn)(s8 *handler, s8 *step, s8 *discStep, s16 *timer, u16 *cancelHold, u16 *retries, u16 *retries2, s8 *err);
extern NetMsgFn gNetMsgHandlers[];

#pragma rodata on
char D_0046A860[33] = {
#include "sub_main/0046A860.inc"
};
#pragma rodata off

/*
 * Logs (Net_DebugLog, compiled out) and runs the connection handler gNetMsgHandlers[*handler]
 * (NET_HANDLER_*), passing it gNetMsgState's step, disconnect step, timer, cancel count, two
 * retry counts and error code. Returns the handler's value (0 busy, NET_STEP_*).
 */
s32 NetMsg_Dispatch(s8 *handler, s8 *step, s8 *discStep, s16 *timer, u16 *cancelHold, u16 *retries, u16 *retries2, s8 *err)
{
    Net_DebugLog(1, 27, 1, D_0046A860, *timer, *handler, *step, *discStep, *err);
    return gNetMsgHandlers[*handler](handler, step, discStep, timer, cancelHold, retries, retries2, err);
}

typedef struct NetProgress {
    u8 cur;
    u8 max;
} NetProgress;
extern NetProgress gNetProgress;
extern s32 D_0059D61C;
extern s32 gNetIfOpt8;
/* unit's own declaration: declared as this file's callers use it; with the definition's s16 (s32 mode, s32 kind), Net_ConnectLanStep and Net_ConnectPppStep compile differently */
s32 NetIf_Up(s32 mode, s32 kind);

/* Stops the connection with error e: hands over to the disconnect handler
 * with its state and counters cleared. */
#define NET_RESET(e)  \
    *err = (e);       \
    *handler = NET_HANDLER_DISCONNECT;         \
    *step = 0;          \
    *discStep = 0;         \
    *timer = 0;           \
    *cancelHold = 0;          \
    *retries = 0;          \
    *retries2 = 0

#pragma rodata on
#pragma explicit_zero_data on
char D_0046A888[1] = {
#include "sub_main/0046A888.inc"
};
#pragma explicit_zero_data reset
#pragma rodata off

extern s16 gNetPppPhase;
extern s16 gNetPppError;

/*
 * One frame of the PPP connection (a modem, or PPPoE on an Ethernet device). For Ethernet
 * in static or DHCP mode it hands over to Net_ConnectLanStep (NET_HANDLER_LAN). Fills
 * gNetPppConf, brings the interface up, dials (NetPpp_Connect) and follows the PPP phase,
 * raising gNetProgress. For PPP errors 1, 2, 5 and 12 it redials (the retry counts, the
 * other flag, the next line mode). Any other failure, or *cancelHold past NET_CANCEL_AT,
 * goes through NET_RESET: the error in *err and on to Net_Disconnect. Returns 0 while
 * busy, else NET_STEP_CONNECTED, NET_STEP_STOPPED (no device) or NET_STEP_REDIAL.
 */
s32 Net_ConnectPppStep(s8 *handler, s8 *step, s8 *discStep, s16 *timer, s16 *cancelHold, s16 *retries, s16 *retries2, s8 *err)
{
    char *chat[2];
    s32 pppOpt1;

    if (NetDev_CheckLink() < 0) {
        NET_RESET(NET_ERR_NO_DEVICE);
        return 0;
    }
    switch (*step) {
    case 0:
        if (gNetDevCur == 0) {
            *err = NET_ERR_NO_DEVICE;
            return NET_STEP_STOPPED;
        }
        gNetProgress.max = 10;
        gNetProgress.cur = 0;
        *discStep = 0;
        *timer = 0;
        *cancelHold = 0;
        *retries = 3;
        *retries2 = 2;
        if (gNetDevCur->type == NET_DEV_ETHERNET) {
            switch (gNetLanMode) {
            case NET_LAN_PPPOE:
                *step = 10;
                break;
            case NET_LAN_DHCP:
            case NET_LAN_STATIC:
                *handler = NET_HANDLER_LAN;
                break;
            }
        } else {
            *step = 10;
        }
        return 0;
    case 10:
        NetModem_GetChat(gNetDevCur->type, gNetDevCur->driver, 0, 2, chat);
        *step = 1;
        gNetPppConf.arg.cfg = &gNetPppConf.dial;
        gNetPppConf.dial.modem = &gNetPppConf.opts;
        gNetPppConf.dial.auth = &gNetPppConf.auth;
        gNetPppConf.dial.lineMode = &gNetPppLineMode;
        gNetPppConf.dial.confText = (char *)&gNetPppConfBuf;
        gNetPppConf.dial.authName = (char *)&gNetAuthName;
        gNetPppConf.dial.authKey = (char *)&gNetAuthKey;
        gNetPppConf.opts.opt[0] = 4;
        gNetPppConf.arg.unk4 = 0;
        gNetPppConf.opts.opt[1] = 0;
        gNetPppConf.opts.opt[2] = 0;
        gNetPppConf.opts.opt[3] = 1;
        gNetPppConf.opts.opt[4] = 1;
        gNetPppConf.arg.isPppoe = gNetLanMode == NET_LAN_PPPOE;
        gNetPppConf.arg.phyMode = gNetPhyMode;
        gNetPppConf.dial.chat0 = chat[0];
        gNetPppConf.dial.chat1 = chat[1];
        gNetPppConf.auth.flag = gNetPppFlagSaved;
        if (gNetPppAuthText == 0) {
            gNetPppConf.auth.authKind = 0;
            gNetPppConf.auth.authText = D_0046A888;
        } else {
            gNetPppConf.auth.authKind = 1;
            gNetPppConf.auth.authText = (char *)&gNetPppAuthText;
        }
        memset(&gNetPppLineMode, 0, 16);
        gNetPppLineMode = gNetPppLineModeFirst;
        return 0;
    case 1:
        if (0 < (*timer)--) {
            return 0;
        }
        *timer = 0;
        gNetProgress.cur = 2;
        if (NetIf_Up(gNetDevCur->type, gNetDevCur->driver) < 0) {
            NET_RESET(NET_ERR_FAILED);
            return 0;
        }
        (*step)++;
        return 0;
    case 2:
        gNetProgress.cur = 4;
        if (NetPpp_Init() < 0) {
            NET_RESET(NET_ERR_FAILED);
            return 0;
        }
        pppOpt1 = 10;
        NetPpp_SetOption1(&pppOpt1);
        (*step)++;
        *timer = 0;
        return 0;
    case 3:
        gNetProgress.cur = 6;
        if (gNetDevCur->type == NET_DEV_MODEM || gNetDevCur->type == NET_DEV_PPP_OTHER) {
            gNetDevCur->status = NetPpp_GetLinkState();
            if (gNetDevCur->status < 0) {
                NET_RESET(NET_ERR_NO_DEVICE);
                return 0;
            }
        }
        if (NetPpp_Connect(&gNetPppConf.arg) < 0) {
            NET_RESET(NET_ERR_FAILED);
            return 0;
        }
        memset(&gNetPppStatus, 0, 20);
        *step += 2;
        *timer = 0;
        return 0;
    case 4:
        (*step)++;
        return 0;
    case 5:
        if (*cancelHold > NET_CANCEL_AT) {
            NET_RESET(NET_ERR_CANCELED);
            return 0;
        }
        if (*timer % 30 == 0) {
            gNetProgress.cur = (gNetProgress.cur < gNetProgress.max) ? gNetProgress.cur + 1 : gNetProgress.max;
        }
        (*timer)++;
        if (NetPpp_GetStatus((NetConnInfo *)&gNetPppStatus) < 0) {
            NET_RESET(NET_ERR_FAILED);
            return 0;
        }
        switch (gNetPppPhase) {
        case 1:
            gNetProgress.max = 12;
            if (gNetProgress.cur < 11) {
                gNetProgress.cur = 11;
            }
            break;
        case 2:
            gNetProgress.max = 55;
            if (gNetProgress.cur < 12) {
                gNetProgress.cur = 12;
            }
            break;
        case 3:
            gNetProgress.max = 70;
            if (gNetProgress.cur < 55) {
                gNetProgress.cur = 55;
            }
            break;
        case NET_PPP_PHASE_UP:
            gNetProgress.max = 80;
            if (gNetProgress.cur < 70) {
                gNetProgress.cur = 70;
            }
            (*step)++;
            break;
        case NET_PPP_PHASE_DOWN:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
            switch (gNetPppError) {
            case NET_PPP_ERR_FLAG:
                if (NetPpp_TryOtherFlag() == 0) {
                    *step = 99;
                } else {
                    NET_RESET(NET_ERR_PPP_FLAG);
                }
                break;
            case NET_PPP_ERR_BUSY:
                if (--(*retries2) == 0) {
                    NET_RESET(NET_ERR_PPP_LINE);
                } else {
                    *step = 99;
                }
                break;
            case NET_PPP_ERR_RETRY:
                if (--(*retries) == 0) {
                    NET_RESET(NET_ERR_FAILED);
                } else {
                    *step = 99;
                }
                break;
            case NET_PPP_ERR_LINEMODE:
                if (NetPpp_NextLineMode() == 0) {
                    *step = 99;
                } else {
                    NET_RESET(NET_ERR_FAILED);
                }
                break;
            case 3:
                NET_RESET(NET_ERR_PPP_E3);
                break;
            case 6:
                NET_RESET(NET_ERR_PPP_E6);
                break;
            case 13:
                NET_RESET(NET_ERR_PPP_LINE);
                break;
            case 10:
                NET_RESET(NET_ERR_PPP_E10);
                break;
            default:
                NET_RESET(NET_ERR_FAILED);
                break;
            }
            return 0;
        default:
            NET_RESET(NET_ERR_FAILED);
            break;
        }
        return 0;
    case 6:
        gNetProgress.max = 100;
        gNetProgress.cur = 100;
        (*step)++;
        NetPpp_SaveFlag();
        Net_InitDns();
        NetPpp_SetConnected(0);
        gNetIpAddr = gNetPppStatus;
        return 0;
    case 7:
        *handler = NET_HANDLER_PPP;
        *step = 0;
        *timer = 0;
        *cancelHold = 0;
        *retries = 0;
        *retries2 = 0;
        return NET_STEP_CONNECTED;
    case 99:
        (*step)++;
        *discStep = 0;
        *timer = 0;
        return 0;
    case 100:
        if (Net_DisconnectStep(discStep, timer) != 0) {
            gNetProgress.cur = 0;
            gNetProgress.max = 10;
            *step = 1;
            *discStep = 0;
            *timer = 30;
            *cancelHold = 0;
            return NET_STEP_REDIAL;
        }
        return 0;
    default:
        return 0;
    }
}

/*
 * One frame of the Ethernet connection, static or DHCP: brings the interface and Ethernet
 * up, sets the address (static) or waits for a DHCP lease, adds the default route, starts
 * the resolver and checks interface option 8. A link down for NET_LINK_WAIT frames, or a
 * DHCP failure, redials up to 3 times. Failures and *cancelHold past NET_CANCEL_AT go
 * through NET_RESET (the error in *err, on to Net_Disconnect). Returns 0 while busy,
 * NET_STEP_CONNECTED or NET_STEP_REDIAL.
 */
s32 Net_ConnectLanStep(s8 *handler, s8 *step, s8 *discStep, s16 *timer, s16 *cancelHold, s16 *retries, s16 *retries2, s8 *err)
{
    s32 dhcpState;

    if (NetDev_CheckLink() < 0) {
        NET_RESET(NET_ERR_NO_DEVICE);
        return 0;
    }
    switch (*step) {
    case 0:
        (*step)++;
        *discStep = 0;
        *timer = 0;
        *cancelHold = 0;
        *retries = 0;
        *retries2 = 0;
        return 0;
    case 1:
        if (0 < (*timer)--) {
            return 0;
        }
        *timer = 0;
        gNetProgress.max = 10;
        gNetProgress.cur = 0;
        (*step)++;
        return 0;
    case 2:
        gNetProgress.cur = 2;
        if (NetIf_Up(gNetDevCur->type, gNetDevCur->driver) < 0) {
            NET_RESET(NET_ERR_FAILED);
            return 0;
        }
        (*step)++;
        return 0;
    case 3:
        gNetProgress.cur = 6;
        if (NetEth_Init() < 0) {
            NET_RESET(NET_ERR_FAILED);
            return 0;
        }
        switch (gNetLanMode) {
        case NET_LAN_DHCP:
            *step = 20;
            break;
        case NET_LAN_STATIC:
            *step = 10;
            break;
        }
        return 0;
    case 10:
        gNetProgress.max = 20;
        if (gNetProgress.cur < 10) {
            gNetProgress.cur = 10;
        }
        gNetDhcpBuf[0] = gNetIpAddr;
        D_0059D61C = gNetNetmask;
        if (NetEth_SetAddress(gNetDhcpBuf) < 0) {
            NET_RESET(NET_ERR_FAILED);
            return 0;
        }
        (*step)++;
        return 0;
    case 11:
        if (NetEth_Start(gNetPhyMode) < 0) {
            NET_RESET(NET_ERR_FAILED);
            return 0;
        }
        (*step)++;
        *timer = 0;
        return 0;
    case 12:
        if (*cancelHold > NET_CANCEL_AT) {
            NET_RESET(NET_ERR_CANCELED);
            return 0;
        }
        if (*timer % 30 == 0) {
            gNetProgress.cur = (gNetProgress.cur < gNetProgress.max) ? gNetProgress.cur + 1 : gNetProgress.max;
        }
        (*timer)++;
        if (NetEth_IsLinkUp() != 0) {
            gNetProgress.max = 70;
            if (gNetProgress.cur < 20) {
                gNetProgress.cur = 20;
            }
            (*step)++;
            *timer = 0;
            *retries = 0;
            return 0;
        }
        if (*timer > NET_LINK_WAIT) {
            if (*retries >= 3) {
                NET_RESET(NET_ERR_NO_LINK);
            } else {
                *step = 99;
                (*retries)++;
            }
        }
        return 0;
    case 13:
        if (*cancelHold > NET_CANCEL_AT) {
            NET_RESET(NET_ERR_CANCELED);
            return 0;
        }
        *timer = 0;
        gNetProgress.max = 100;
        if (gNetProgress.cur < 70) {
            gNetProgress.cur = 70;
        }
        Net_InitDns();
        if (NetRoute_Add(0, gNetGateway) < 0) {
            NET_RESET(NET_ERR_FAILED);
            return 0;
        }
        *step = 30;
        return 0;
    case 20:
        gNetProgress.max = 20;
        if (gNetProgress.cur < 11) {
            gNetProgress.cur = 11;
        }
        if (NetEth_Start(gNetPhyMode) < 0) {
            NET_RESET(NET_ERR_FAILED);
            return 0;
        }
        (*step)++;
        *timer = 0;
        return 0;
    case 21:
        if (*cancelHold > NET_CANCEL_AT) {
            NET_RESET(NET_ERR_CANCELED);
            return 0;
        }
        if (*timer % 30 == 0) {
            gNetProgress.cur = (gNetProgress.cur < gNetProgress.max) ? gNetProgress.cur + 1 : gNetProgress.max;
        }
        (*timer)++;
        if (NetEth_IsLinkUp() != 0) {
            gNetProgress.max = 60;
            if (gNetProgress.cur < 20) {
                gNetProgress.cur = 20;
            }
            (*step)++;
            *timer = 0;
            return 0;
        }
        if (*timer > NET_LINK_WAIT) {
            if (*retries >= 3) {
                NET_RESET(NET_ERR_NO_LINK);
            } else {
                *step = 99;
                (*retries)++;
            }
        }
        return 0;
    case 22:
        if (NetDhcp_Init() < 0) {
            NET_RESET(NET_ERR_FAILED);
            return 0;
        }
        if (NetDhcp_Start(0, 5) < 0) {
            NET_RESET(NET_ERR_FAILED);
            return 0;
        }
        (*step)++;
        *timer = 0;
        return 0;
    case 23:
        if (*cancelHold > NET_CANCEL_AT) {
            NetDhcp_Term();
            NET_RESET(NET_ERR_CANCELED);
            return 0;
        }
        if (*timer % 30 == 0) {
            gNetProgress.cur = (gNetProgress.cur < gNetProgress.max) ? gNetProgress.cur + 1 : gNetProgress.max;
        }
        (*timer)++;
        dhcpState = NetDhcp_GetState();
        if (dhcpState < 0) {
            if (*retries >= 3) {
                NetDhcp_Term();
                if (gNetProgress.cur == 60) {
                    NET_RESET(NET_ERR_DHCP_TIMEOUT);
                } else {
                    NET_RESET(NET_ERR_DHCP);
                }
            } else {
                NetDhcp_Term();
                *step = 99;
                (*retries)++;
            }
            return 0;
        }
        switch (dhcpState) {
        case 3:
            gNetProgress.max = 70;
            if (gNetProgress.cur < 60) {
                gNetProgress.cur = 60;
            }
            NetDhcp_GetAddress(gNetDhcpBuf);
            gNetIpAddr = gNetDhcpBuf[0];
            gNetNetmask = D_0059D61C;
            NetEth_SetAddress(gNetDhcpBuf);
            (*step)++;
            *timer = 0;
            break;
        }
        return 0;
    case 24:
        *timer = 0;
        gNetProgress.max = 100;
        if (gNetProgress.cur < 70) {
            gNetProgress.cur = 70;
        }
        Net_InitDns();
        if (NetDhcp_GetRouter(gNetDhcpBuf) < 0) {
            NET_RESET(NET_ERR_FAILED);
            return 0;
        }
        gNetGateway = gNetDhcpBuf[0];
        if (NetRoute_Add(0, gNetGateway) < 0) {
            NET_RESET(NET_ERR_FAILED);
            return 0;
        }
        *step = 30;
        return 0;
    case 30:
        NetEth_SetConnected(0);
        gNetIfOpt8 = 0;
        if (NetIf_GetOption8(&gNetIfOpt8) < 0 || gNetIfOpt8 == 0) {
            NET_RESET(NET_ERR_IF_OPT8);
            return 0;
        }
        gNetProgress.max = 100;
        gNetProgress.cur = 100;
        (*step)++;
        *discStep = 0;
        *timer = 0;
        *cancelHold = 0;
        *retries = 0;
        *retries2 = 0;
        return 0;
    case 31:
        *handler = NET_HANDLER_PPP;
        *step = 0;
        *timer = 0;
        *cancelHold = 0;
        *retries = 0;
        *retries2 = 0;
        return NET_STEP_CONNECTED;
    case 99:
        (*step)++;
        *discStep = 0;
        *timer = 0;
        return 0;
    case 100:
        if (Net_DisconnectStep(discStep, timer) != 0) {
            gNetProgress.cur = 0;
            gNetProgress.max = 10;
            *step = 1;
            *discStep = 0;
            *timer = 30;
            *cancelHold = 0;
            return NET_STEP_REDIAL;
        }
        return 0;
    default:
        return 0;
    }
}

/*
 * The disconnect handler: runs Net_DisconnectStep to the end, then clears the handler,
 * step, timer and counters (*err is kept for the error message). Returns
 * NET_STEP_STOPPED when done, else 0.
 */
s32 Net_Disconnect(s8 *handler, s8 *step, s8 *discStep, s16 *timer, s16 *cancelHold, s16 *retries, s16 *retries2, s32 err)
{
    s8 stepNow;

    stepNow = *step;
    switch (stepNow) {
    case 0:
        *step = stepNow + 1;
        *discStep = 0;
        *timer = 0;
        return 0;
    case 1:
        if (Net_DisconnectStep(discStep, timer) != 0) {
            *handler = NET_HANDLER_PPP;
            *step = 0;
            *discStep = 0;
            *timer = 0;
            *cancelHold = 0;
            *retries = 0;
            *retries2 = 0;
            return NET_STEP_STOPPED;
        }
        return 0;
    default:
        return 0;
    }
}


#pragma rodata on
char D_0046A8C0[26] = {
#include "sub_main/0046A8C0.inc"
};
#pragma rodata off

/* NETWORK: the disconnect steps (state *st, timer *t): closes the socket (gNetSock), stops the
 * device by its type and returns 1 when done. */
s32 Net_DisconnectStep(s8 *state, s16 *timer)
{
    s16 left;
    s32 pppResult;

    Net_DebugLog(1, 26, 1, D_0046A8C0);
    switch (*state) {
    case 0:
        if (NetIf_GetState() == 0) {
            *state = 20;
            *timer = 0;
        } else {
            if (NetIf_GetState() == 3) {
                (*state)++;
                *timer = 0;
                return 0;
            }
            *state = 2;
            *timer = 0;
        }
        return 0;
    case 1:
        NetTcp_Abort(gNetSock);
        NetTcp_Delete(&gNetSock);
        (*state)++;
        *timer = 0;
        return 0;
    case 2:
        if (0 < (*timer)--) {
            return 0;
        }
        *timer = 0;
        if (NetIf_GetState() == 2) {
            *state = 6;
        } else {
            (*state)++;
        }
        return 0;
    case 3:
        NetDns_Term();
        if (gNetDevCur->type == NET_DEV_ETHERNET) {
            switch (gNetLanMode) {
            case NET_LAN_STATIC:
                NetRoute_Delete(0, gNetGateway);
                NetEth_ClearAddress(0);
                *state = 6;
                break;
            case NET_LAN_DHCP:
                NetRoute_Delete(0, gNetGateway);
                NetEth_ClearAddress(0);
                (*state)++;
                break;
            case NET_LAN_PPPOE:
                NetPpp_SetClosed(0);
                *state = 6;
                break;
            }
        } else {
            NetPpp_SetClosed(0);
            *state = 6;
        }
        return 0;
    case 4:
        D_0059D61C = 0;
        gNetDhcpBuf[0] = gNetIpAddr;
        if (NetDhcp_Release(gNetDhcpBuf, 5) != 0) {
            NetDhcp_Term();
            *state += 2;
        } else {
            (*state)++;
            *timer = 300;
        }
        return 0;
    case 5:
        switch (NetDhcp_GetState()) {
        default:
            if (0 < (*timer)--) {
                break;
            }
            *timer = 0;
        case 0:
        case -2:
        case -1:
            NetDhcp_Term();
            (*state)++;
            *timer = 0;
            break;
        }
        return 0;
    case 6:
        switch (gNetDevCur->type) {
        case NET_DEV_MODEM:
        case NET_DEV_PPP_OTHER:
            NetPpp_Disconnect(0);
            *timer = 300;
            (*state)++;
            break;
        case NET_DEV_ETHERNET:
            switch (gNetLanMode) {
            case NET_LAN_PPPOE:
                NetPpp_Disconnect(0);
                *timer = 300;
                (*state)++;
                break;
            case NET_LAN_DHCP:
            case NET_LAN_STATIC:
                NetEth_Stop();
                *timer = 30;
                *state += 2;
                break;
            }
            break;
        }
        return 0;
    case 7:
        (*timer)--;
        left = *timer;
        if (left <= -1) {
            (*state)++;
            *timer = 0;
        } else if (left % 30 == 0) {
            pppResult = NetPpp_GetStatus((NetConnInfo *)&gNetPppStatus);
            if (gNetPppPhase == NET_PPP_PHASE_DOWN || pppResult < 0) {
                (*state)++;
                *timer = 0;
            }
        }
        return 0;
    case 8:
        if (0 < (*timer)--) {
            return 0;
        }
        *timer = 0;
        switch (gNetDevCur->type) {
        case NET_DEV_MODEM:
        case NET_DEV_PPP_OTHER:
            NetPpp_Term();
            break;
        case NET_DEV_ETHERNET:
            switch (gNetLanMode) {
            case NET_LAN_PPPOE:
                NetPpp_Term();
                break;
            case NET_LAN_DHCP:
            case NET_LAN_STATIC:
                NetEth_Term();
                break;
            }
            break;
        }
        (*state)++;
        *timer = 20;
        return 0;
    case 9:
        if (0 < (*timer)--) {
            return 0;
        }
        *timer = 0;
        NetIf_Down();
        (*state)++;
        *timer = 20;
        return 0;
    case 10:
        if (0 < (*timer)--) {
            return 0;
        }
        *timer = 0;
        *state = 20;
        *timer = 0;
        return 0;
    case 20:
        if (0 < (*timer)--) {
            return 0;
        }
        *timer = 0;
        *state = 0;
        *timer = 0;
        return 1;
    default:
        return 0;
    }
}

/* This file's initialised data. */
NetMsgFn gNetMsgHandlers[4] = {
    (NetMsgFn)Net_ConnectPppStep, (NetMsgFn)Net_ConnectLanStep, (NetMsgFn)Net_Disconnect, NULL,
};
