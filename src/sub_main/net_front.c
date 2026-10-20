/*
 * The network stack's front, under the lobby:
 * - the BSD-style socket calls the HTTP library uses (net_socket,
 *   net_connect, net_recv, net_send, net_close, net_gethostbyname), over
 *   AVE's TCP and DNS, and the host caches;
 * - the network devices: the IOP's USB device list (gNetDevs, NetDev_*)
 *   and the IRX drivers (Net_LoadDrivers, Net_StopDevice), started by
 *   Net_Start;
 * - the connection state machines NetMsg_Update runs from gNetMsgState
 *   (Net_ConnectPppStep, Net_ConnectLanStep, Net_Disconnect);
 * - the DNAS check (Dnas_Step) and the transfers through the DNAS install
 *   library (NetXfer_Step);
 * - the reset of the network settings (NetCfg_Reset).
 * The states, errors and kinds are in net_tcp.h. Networking code stays
 * byte-identical.
 */
#include "common.h"
#include "em.h"
#include "sif.h"
#include "yn.h"                /* NetDevice, gNetDevs */
#include "net_tcp.h"

/* The game built this file at optimization level 4. */
#pragma optimization_level 4

/* prototypes */
s32 NetTcp_Init(void);
void NetSock_Lock(void);
void NetSock_Unlock(void);
s32 net_inet_addr(s8 *);
s32 NetDev_MatchesDefault(s8 *vendor, s8 *product);

s32 Net_DisconnectStep(s8 *st, s16 *t);
void FlushCache(s32 operation);
s32 Dnas_Step(s8 *, s16 *, s16 *, s32);
s32 strcmp(s8 *a, s8 *b);
void strncpy(char *dst, const char *src, u32 n);
u8 sceScfGetLanguage(void);
void * memset(void *p, s32 c, u32 n);
s32 * net_gethostbyname(s8 *);
extern NetDevice gNetDevNames[];
extern s8 D_0046A8E0;
extern s8 D_0046A8F0;
extern s8 D_0046A900;
extern s8 D_0046A910;
extern u8 D_0046A918;
extern u8 D_0046A928;
extern s32 gHostCacheNext;
extern s32 gNetHErrno;
extern s32 gNetErrno;
extern s32 gNetHostCacheNext;
extern s16 gNetPppLineMode;
extern s32 gNetXferResult;
extern s32 gDnasResult;
extern s32 gNetPppStatus;
extern u8 gNetTcpStatus;
extern s32 gNetSock;
extern NetMsgState gNetMsgState;
extern s8 *gNetOptList;
extern s8 *D_0059D684;
extern s8 *D_0059D688;
extern s8 *D_0059D68C;
extern u8 *D_0059D690;
extern u8 *D_0059D694;
extern s32 D_0059D698;
extern s32 D_0059D69C;
extern s8 gNetOptLatency;
extern s8 gNetOptLatencyKey;
extern s8 gNetOptMaxRetries;
extern s8 gNetOptMaxRetriesKey;
extern u8 D_0059D700;
extern s32 D_0059D800;
extern s32 D_0059D808;
extern s32 gNetSema;
extern s32 D_0059D818;
extern u8 D_0059D820;
extern u8 D_0059D8A0;
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
extern s16 D_0059DCF0;
extern s32 D_0059DCF8;
extern u8 D_0059DD00;
extern s8 D_0059DE00;
extern s8 gNetDriverType;
extern s32 gNetDevCount;
extern s32 gNetDevSeenCount;
extern NetDevice *gNetDevCur;   /* the device in use (in gNetDevs) */
/* end of prototypes */

/* A struct hostent. */
typedef struct HostEnt {
    s8 *h_name;                 /* 0x00 */
    s8 **h_aliases;             /* 0x04 */
    s32 h_addrtype;             /* 0x08 */
    s32 h_length;               /* 0x0C */
    s8 **h_addr_list;           /* 0x10 */
} HostEnt;

/* A slot of gethostbyname's cache (8 of 0x134 bytes at gHostCache, gHostCacheNext the next one). */
typedef struct HostSlot {
    s8 name[0x100];             /* 0x000 */
    s32 addr;                   /* 0x100 */
    u8 pad104[0xC];
    s8 *addrList[4];            /* 0x110 */
    HostEnt ent;                /* 0x120 */
} HostSlot;

extern HostSlot gHostCache[8];

/* The host name cache (8 entries, a ring: gNetHostCacheNext the next one). */
typedef struct HostEntry {
    s8 name[0x100];             /* 0x000 */
    s32 addr;                   /* 0x100 */
} HostEntry;

extern HostEntry gNetHostCache[8];

s32 atoi(s8 *s);             /* atoi */
u32 strlen(s8 *s);

/* The vendor and product names of the known network devices (gNetDevNames, gNetDefaultDev). */
#pragma rodata on
char gNetUsbLanVendor[0xC] = {
#include "sub_main/0046A508.inc"
};
char gNetUsbLanProduct1[0xC] = {
#include "sub_main/0046A518.inc"
};
char gNetUsbLanProduct2[0xC] = {
#include "sub_main/0046A528.inc"
};
char gNetAdaptorVendor[0x8] = {
#include "sub_main/0046A538.inc"
};
char gNetAdaptorProduct[0x20] = {
#include "sub_main/0046A540.inc"
};
#pragma rodata off

/*
 * inet_addr(): the dotted-quad address in str, in network order. Returns 0 when str is not
 * one: a run of more than 3 digits, a fourth dot, or any other character.
 */
s32 net_inet_addr(s8 *str)
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

/* htons(): x's low 16 bits with the two bytes swapped. */
s32 net_htons(s32 host16)
{
    s32 val;

    val = host16 & 0xFFFF;
    return (((val << 8) & 0xFF00) | ((val >> 8) & 0xFF)) & 0xFFFF;
}

/*
 * inet_aton(): *out = net_inet_addr(str). Returns 1 when that is non-zero, else 0.
 */
s32 net_inet_aton(char *str, s32 *out)
{
    *out = net_inet_addr((s8 *)str);
    return *out != 0;
}

/* The socket layer's errno (gNetErrno), for the HTTP library. */
s32 * net_errno_ptr(void)
{
    return &gNetErrno;
}

/* The resolver's h_errno (gNetHErrno), for the HTTP library. */
s32 * net_h_errno_ptr(void)
{
    return &gNetHErrno;
}

extern s32 gNetSockHandles[];
extern s32 gNetSockClose[8];

/*
 * Asks for socket fd (0-7) to be cancelled: sets gNetSockClose[fd] to 1, and the next
 * wait in net_connect, net_recv or net_gethostbyname fails with ECONNABORTED. Returns 0,
 * or -1 for a bad or free socket.
 */
s32 net_cancel(s32 fd)
{
    if (fd < 0 || fd >= NET_SOCK_MAX) {
        return -1;
    }
    NetSock_Lock();
    if (gNetSockHandles[fd] == NET_SOCK_OPENED) {
        gNetSockClose[fd] = 1;
        NetSock_Unlock();
        return 0;
    }
    NetSock_Unlock();
    if (gNetSockHandles[fd] < 0) {
        return -1;
    }
    NetSock_Lock();
    gNetSockClose[fd] = 1;
    NetSock_Unlock();
    return 0;
}

/* An unused socket-API stub: always -1. */
s32 net_unused_fail(void)
{
    return -1;
}

void NetSock_Lock(void);
void NetSock_Unlock(void);
extern s32 gNetErrno;
extern s32 gNetSockHandles[];

/*
 * socket(): takes a free slot of gNetSockHandles and marks it opened (-2). domain is
 * ignored. Returns the slot, or -1 with EINVAL (type other than stream) or ENOMEM
 * (all 8 slots in use).
 */
s32 net_socket(s32 domain, s32 type)
{
    s32 i;

    if (type != 1) {
        gNetErrno = NET_EINVAL;
        return -1;
    }
    NetSock_Lock();
    for (i = 0; i < NET_SOCK_MAX; i++) {
        if (gNetSockHandles[i] == NET_SOCK_FREE) {
            break;
        }
    }
    if (i == NET_SOCK_MAX) {
        gNetErrno = NET_ENOMEM;
        NetSock_Unlock();
        return -1;
    }
    gNetSockHandles[i] = NET_SOCK_OPENED;
    NetSock_Unlock();
    return i;
}

typedef struct SockAddr {
    u8 len;                     /* 0x0 */
    u8 family;                  /* 0x1 */
    u16 port;                   /* 0x2 */
    u32 addr;                   /* 0x4 */
} SockAddr;
s16 AveTcp_Open(s32 addr, u16 port, u16 a);
s16 AveTcp_Status(s16 sock, u16 *a, u16 *b, u16 *c);
s32 DelayThread(s32 usec);

/*
 * connect(): opens an AVE TCP connection for socket fd and polls it every 20 ms (at most
 * 3000 times) while it is busy. Returns 0 once it is connected, or -1 with errno EBADF,
 * EHOSTUNREACH (open failed), ECONNABORTED (net_cancel) or ECONNRESET.
 */
s32 net_connect(s32 fd, SockAddr *addr, s32 addrLen)
{
    s16 state;
    u16 sendSpace;
    s16 recvLen;
    s32 handle;
    s16 i;

    state = 0;
    if (fd < 0 || fd >= NET_SOCK_MAX) {
        gNetErrno = NET_EBADF;
        return -1;
    }
    if (gNetSockHandles[fd] != NET_SOCK_OPENED) {
        gNetErrno = NET_EBADF;
        return -1;
    }
    handle = AveTcp_Open(addr->addr, addr->port, 0);
    if (handle < 0) {
        gNetErrno = NET_EHOSTUNREACH;
        return -1;
    }
    NetSock_Lock();
    gNetSockHandles[fd] = handle;
    NetSock_Unlock();
    i = 0;
    while (i++ < NET_POLL_TRIES) {
        NetSock_Lock();
        if (gNetSockClose[fd] == 1) {
            gNetSockClose[fd] = -1;
            gNetErrno = NET_ECONNABORTED;
            NetSock_Unlock();
            return -1;
        }
        NetSock_Unlock();
        if (AveTcp_Status(gNetSockHandles[fd], (u16 *)&state, &sendSpace, (u16 *)&recvLen) < 0) {
            gNetErrno = NET_ECONNRESET;
            return -1;
        }
        if (state != 2 && state != 3) {
            break;
        }
        DelayThread(NET_POLL_USEC);
    }
    if (state != 4) {
        gNetErrno = NET_ECONNRESET;
        return -1;
    }
    return 0;
}

s32 strncmp(s8 *a, s8 *b, s32 n);
s16 AveDns_Request(s8 *name);
s16 AveDns_Free(s16 id);
s16 AveDns_LookUp(s16 id, s32 *addr);
s32 DelayThread(s32 usec);

/* gethostbyname(): from the cache, or a DNS request polled every 20 ms (until it answers, fails
 * or a socket is closed) whose answer is cached; the hostent, or NULL with h_errno (gNetHErrno). */
s32 *net_gethostbyname(s8 *name)
{
    s16 i;
    s16 dnsId;
    s16 res;
    s16 k;
    HostSlot *slot;

    NetSock_Lock();
    for (i = 0, slot = gHostCache; i < 8; i++, slot++) {
        if (strncmp(slot->name, name, 0x100) == 0) {
            goto found;
        }
    }
    slot = &gHostCache[gHostCacheNext];
    dnsId = AveDns_Request(name);
    switch (dnsId) {
    case -0x21:
    case -1:
        gNetHErrno = -1;
        goto fail;
    }
    NetSock_Unlock();
    for (;;) {
        NetSock_Lock();
        for (k = 0; k < NET_SOCK_MAX; k++) {
            if (gNetSockClose[k] == 1) {
                gNetSockClose[k] = -1;
                AveDns_Free(dnsId);
                gNetHErrno = NET_ECONNABORTED;
                goto fail;
            }
        }
        res = AveDns_LookUp(dnsId, &slot->addr);
        switch (res) {
        case -6:
        case -1:
            AveDns_Free(dnsId);
            gNetHErrno = 1;
            goto fail;
        case -2:
            break;
        }
        if (0 < res) {
            AveDns_Free(dnsId);
            NetSock_Unlock();
            break;
        }
        NetSock_Unlock();
        DelayThread(NET_POLL_USEC);
    }
    NetSock_Lock();
    strncpy((char *)slot->name, (char *)name, 0x100);
    gHostCacheNext++;
    gHostCacheNext %= 8;
found:
    slot->ent.h_aliases = NULL;
    slot->ent.h_addrtype = 2;
    slot->ent.h_length = 4;
    slot->addrList[0] = (s8 *)&slot->addr;
    slot->ent.h_addr_list = slot->addrList;
    NetSock_Unlock();
    return (s32 *)&slot->ent;
fail:
    NetSock_Unlock();
    return NULL;
}

/*
 * The address of host, in network order: a dotted address directly, otherwise a DNS lookup
 * (net_gethostbyname). Returns -1 when the lookup fails.
 */
s32 net_resolve(s8 *host)
{
    HostEnt *hent;
    s32 inetAddr;
    u8 *addrBytes;

    inetAddr = net_inet_addr(host);
    if (inetAddr != 0) {
        return inetAddr;
    }
    hent = (HostEnt *)net_gethostbyname(host);
    if (hent == NULL) {
        return -1;
    }
    addrBytes = (u8 *)hent->h_addr_list[0];
    return ((addrBytes[3] << 0x18) & 0xFF000000) | (((addrBytes[2] << 0x10) & 0xFF0000) | (addrBytes[0] | ((addrBytes[1] << 8) & 0xFF00)));
}

s16 AveTcp_Status(s16 sock, u16 *a, u16 *b, u16 *c);
s16 AveTcp_Recv(s16 sock, void *buf, s16 len);
s32 DelayThread(s32 usec);

/*
 * recv(): polls every 20 ms until data is waiting or the peer has closed, then reads at most
 * len bytes. Returns the count (0 at end of stream), or -1 with errno EBADF, ECONNABORTED
 * (net_cancel) or ECONNRESET.
 */
s32 net_recv(s32 fd, void *buf, u32 len)
{
    s16 state;
    u16 sendSpace;
    s16 avail;
    s16 i;
    s16 got;

    state = 0;
    if (fd < 0 || fd >= NET_SOCK_MAX) {
        gNetErrno = NET_EBADF;
        return -1;
    }
    if (gNetSockHandles[fd] < 0) {
        gNetErrno = NET_EBADF;
        return -1;
    }
    for (;;) {
        NetSock_Lock();
        if (gNetSockClose[fd] == 1) {
            gNetSockClose[fd] = -1;
            gNetErrno = NET_ECONNABORTED;
            NetSock_Unlock();
            return -1;
        }
        NetSock_Unlock();
        i = 0;
        while (i++ < NET_POLL_TRIES) {
            if (AveTcp_Status(gNetSockHandles[fd], (u16 *)&state, &sendSpace, (u16 *)&avail) < 0) {
                gNetErrno = NET_ECONNRESET;
                return -1;
            }
            if (state != 2 && state != 3) {
                break;
            }
            DelayThread(NET_POLL_USEC);
        }
        if (state == 5) {
            if (avail == 0) {
                break;
            }
        } else if (state == -1) {
            if (avail == 0) {
                break;
            }
        } else if (state != 4) {
            gNetErrno = NET_ECONNRESET;
            return -1;
        }
        if (0 < avail) {
            break;
        }
        DelayThread(NET_POLL_USEC);
    }
    if (avail == 0) {
        got = 0;
    } else {
        if (avail < len) {
            len = avail;
        }
        got = AveTcp_Recv(gNetSockHandles[fd], buf, len);
        if (got < 0) {
            gNetErrno = NET_ECONNRESET;
            return -1;
        }
        if ((state == 5 || state == -1) && got == len && got == avail) {
            gNetErrno = NET_ECONNRESET;
        }
    }
    return got;
}

s16 AveTcp_Status(s16 sock, u16 *a, u16 *b, u16 *c);
s16 AveTcp_Send(s16 sock, u8 *data, s16 len);
s32 DelayThread(s32 usec);

/*
 * send(): polls every 20 ms (at most 3000 times) while the socket is busy (state 2 or 3),
 * then sends len bytes. Returns len, or -1 with errno EBADF or ECONNRESET.
 */
s64 net_send(s32 fd, void *data, u32 len)
{
    s16 state;
    u16 sendSpace;
    s16 recvLen;
    s16 i;
    s32 *handle;

    state = 0;
    if (fd < 0 || fd >= NET_SOCK_MAX) {
        gNetErrno = NET_EBADF;
        return -1;
    }
    handle = &gNetSockHandles[fd];
    if (*handle < 0) {
        gNetErrno = NET_EBADF;
        return -1;
    }
    i = 0;
    while (i++ < NET_POLL_TRIES) {
        if (AveTcp_Status(*handle, (u16 *)&state, &sendSpace, (u16 *)&recvLen) < 0) {
            gNetErrno = NET_ECONNRESET;
            return -1;
        }
        if (state != 2 && state != 3) {
            break;
        }
        DelayThread(NET_POLL_USEC);
    }
    if (AveTcp_Send(*handle, data, len) < 0) {
        gNetErrno = NET_ECONNRESET;
        return -1;
    }
    return len;
}

s16 AveTcp_Abort(s16 sock);
s16 AveTcp_Delete(s16 sock);

/*
 * close(): aborts and deletes fd's AVE TCP connection, and just frees a socket that never
 * connected. Returns 0, or -1 with EBADF for a bad or free socket.
 */
s32 net_close(s32 fd)
{
    s32 *handle;

    if (fd < 0 || fd >= NET_SOCK_MAX) {
        gNetErrno = NET_EBADF;
        return -1;
    }
    NetSock_Lock();
    handle = &gNetSockHandles[fd];
    if (*handle == NET_SOCK_OPENED) {
        *handle = -1;
        gNetSockClose[fd] = -1;
        NetSock_Unlock();
        return 0;
    }
    NetSock_Unlock();
    if (*handle < 0) {
        gNetErrno = NET_EBADF;
        return -1;
    }
    NetSock_Lock();
    AveTcp_Abort(gNetSockHandles[fd]);
    AveTcp_Delete(gNetSockHandles[fd]);
    gNetSockHandles[fd] = NET_SOCK_FREE;
    gNetSockClose[fd] = -1;
    NetSock_Unlock();
    return 0;
}

/* An unused socket-API stub: always 0. */
s32 net_unused_ok1(void)
{
    return 0;
}

/* An unused socket-API stub: always 0. */
s32 net_unused_ok2(void)
{
    return 0;
}

/* Empties net_gethostbyname's cache (gHostCache), holding the socket lock. */
void net_clear_host_cache(void)
{
    NetSock_Lock();
    memset(gHostCache, 0, 0x9A0U);
    gHostCacheNext = 0;
    NetSock_Unlock();
}

extern s32 gNetSockSema;          /* the sockets' lock */
extern s32 gNetSockWaitSema;
extern s32 gNetSockClose[8];

/* Creates the socket semaphores once (a lock, and one at 0) and clears the socket tables. */
void Net_InitSockets(void)
{
    SemaParam semaParam;

    if (gNetSockSema < 0) {
        memset(&semaParam, 0, sizeof(semaParam));
        semaParam.initCount = 1;
        semaParam.maxCount = 1;
        gNetSockSema = CreateSema(&semaParam);
    }
    if (gNetSockWaitSema < 0) {
        memset(&semaParam, 0, sizeof(semaParam));
        semaParam.maxCount = 1;
        semaParam.initCount = 0;
        gNetSockWaitSema = CreateSema(&semaParam);
    }
    memset(gNetSockHandles, 0xFF, sizeof(gNetSockClose));
    memset(gNetSockClose, 0xFF, sizeof(gNetSockClose));
    gNetErrno = 0;
    gNetHErrno = 0;
}

s32 NetPpp_GetStatus(NetConnInfo *out);
extern s16 gUsbKbReady;

/*
 * The device in use, for the lobby: its vendor and product ids, whether the USB keyboard
 * is ready, and for modem-type devices (types 2 and 3) the link speed / 100 (otherwise 0).
 */
void Net_GetDeviceState(s16 *outVendor, s16 *outProduct, s16 *outKbReady, s16 *outSpeed)
{
    NetConnInfo info;

    *outVendor = gNetDevCur->vendorId;
    *outProduct = gNetDevCur->productId;
    *outKbReady = gUsbKbReady & 1;
    *outSpeed = 0;
    switch (gNetDevCur->type) {
    case NET_DEV_MODEM:
    case NET_DEV_PPP_OTHER:
        NetPpp_GetStatus(&info);
        *outSpeed = info.speed / 100;
        break;
    }
}

s32 sceSifStopModule(s32 id, s32 argc, char *argv, s32 *result);   /* stop a module */
s32 sceSifUnloadModule(s32 id);                                       /* unload a module */

/* Stops and unloads IRX module id, retrying each step until it succeeds (as Yn_UnloadModule). */
void Irx_Unload(s32 modId)
{
    s32 result;
    s32 ret;

    while ((ret = sceSifStopModule(modId, 0, 0, &result)) < 0) {
        __asm__ volatile ("nop\n nop\n nop\n nop\n nop\n nop\n nop\n nop");
    }
    if (0 <= ret) {
        while (sceSifUnloadModule(modId) < 0) {
            __asm__ volatile ("nop\n nop\n nop\n nop\n nop\n nop\n nop\n nop");
        }
    }
}

s32 sceSifLoadModule(char *path, s32 argc, char *argv);
s32 sceSifLoadStartModule(char *path, s32 argc, char *argv, void *buf);

/*
 * Loads the IRX module at path with its argument block (argc bytes at argv). If startResult
 * is given, the module is also started and its start result goes there. Retries forever when
 * tries is 0, otherwise at most tries times.
 */
void Net_LoadIrx(s32 path, s32 argc, char *argv, s32 startResult, s32 tries)
{
    s32 i;

    if (startResult == 0) {
        if (tries == 0) {
            while (sceSifLoadModule((char *)path, argc, argv) < 0) {
                __asm__ volatile ("nop\n nop\n nop\n nop\n nop\n nop\n nop\n nop");
            }
        } else {
            for (i = 0; i < tries; i++) {
                if (0 <= sceSifLoadModule((char *)path, argc, argv)) {
                    i = 0;
                    break;
                }
                __asm__ volatile ("nop\n nop\n nop\n nop\n nop\n nop\n nop");
            }
        }
    } else {
        if (tries == 0) {
            while (sceSifLoadStartModule((char *)path, argc, argv, (void *)startResult) < 0) {
                __asm__ volatile ("nop\n nop\n nop\n nop\n nop\n nop\n nop\n nop");
            }
        } else {
            for (i = 0; i < tries; i++) {
                if (0 <= sceSifLoadStartModule((char *)path, argc, argv, (void *)startResult)) {
                    i = 0;
                    break;
                }
                __asm__ volatile ("nop\n nop\n nop\n nop\n nop\n nop\n nop");
            }
        }
    }
}

extern s32 gNetDevRpcBuf;
extern SifClientData gNetDevRpc;

/*
 * Calls function fno of the device-search RPC server, with gNetDevRpcBuf (0x40 bytes) as
 * both send and receive buffer, after writing back the data cache. Returns the call's result,
 * or -1.
 */
s32 NetDev_CallRpc(s32 fno)
{
    s32 r;

    FlushCache(0);
    r = sceSifCallRpc(&gNetDevRpc, fno, 0, (s32 *) &gNetDevRpcBuf, 0x40, (s32 *) &gNetDevRpcBuf, 0x40, 0, 0);
    if (r < 0) {
        return -1;
    }
    return r;
}

/*
 * Fills dev for the IOP's device (vendor, product): copies type, driver and names from the
 * matching gNetDevNames row, plus the ids and status. Returns 0, or -1 for an unknown device,
 * with dev set to all 0xFF (unused).
 */
s32 NetDev_Identify(NetDevice *dev, s32 vendor, s32 product, s32 rpcWord2, s32 devStatus)
{
    NetDevice *row;
    s32 rowVendor;

    row = gNetDevNames;
loop_1:
    rowVendor = row->vendorId;
    if (rowVendor != 0) {
        if ((rowVendor == vendor) && (row->productId == product)) {
            dev->type = row->type;
            dev->driver = row->driver;
            dev->vendorId = vendor;
            dev->productId = product;
            dev->status = devStatus;
            dev->vendor = row->vendor;
            dev->product = row->product;
            return 0;
        }
        row++;
        goto loop_1;
    }
    memset(dev, 0xFF, sizeof(NetDevice));
    return -1;
}

/*
 * When no device is in use, clears the device list and sets both counts to -1, so the next
 * NetDev_Refresh reads it again. Only the first 0x20 bytes of gNetDevs are filled with 0xFF.
 */
void NetDev_ClearList(void)
{
    if (gNetDevCur == 0) {
        memset(&gNetDevs, 0xFF, 0x20U);
        gNetDevSeenCount = -1;
        gNetDevCount = -1;
    }
}

extern NetDevice gNetDefaultDev[];

/*
 * Whether vendor/product are the network adaptor's names (gNetDevAdaptor[0]). 0 for NULL
 * names.
 */
s32 NetDev_MatchesDefault(s8 *vendor, s8 *product)
{
    if (vendor == 0 || product == 0) {
        return 0;
    }
    return strcmp(vendor, (s8 *)gNetDefaultDev[0].vendor) == 0 && strcmp(product, (s8 *)gNetDefaultDev[0].product) == 0;
}

/*
 * Whether vendor/product name a device this game supports: a gNetDevNames row or the
 * network adaptor. 0 for NULL names.
 */
s32 NetDev_IsKnownName(s8 *vendor, s8 *product)
{
    NetDevice *known;

    if ((vendor == 0) || (product == 0)) {
        return 0;
    }
    known = gNetDevNames;
loop_4:
    if (known->vendorId != 0) {
        if ((strcmp(vendor, (s8 *)known->vendor) == 0) && (strcmp(product, (s8 *)known->product) == 0)) {
            return 1;
        }
        known++;
        goto loop_4;
    }
    return NetDev_MatchesDefault(vendor, product) != 0;
}

extern char D_0046A560[];
extern char D_0046A580[];
extern char D_0046A5A0[];
extern s32 gNetStopReq;
extern s32 gNetIrxUsbsrch;
void Net_LoadIrx(s32 mod, s32 argc, char *argv, s32 buf, s32 tries);
s32 sceSifSearchModuleByName(char *name);
s32 NetTcp_Term(void);
s32 NetDev_Refresh(void);
void SifRpc_BindWait(SifClientData *cd, u32 id);

INCLUDE_RODATA("asm/sub_main/nonmatchings/net_front", D_0046A560);
INCLUDE_RODATA("asm/sub_main/nonmatchings/net_front", D_0046A580);
INCLUDE_RODATA("asm/sub_main/nonmatchings/net_front", D_0046A5A0);

/*
 * Releases the Ethernet device in use (sets gNetStopReq). For the LANEGG or SMAP driver
 * kinds it unloads the AVE TCP wrapper module, stops the TCP layer, unloads the driver
 * module, reloads the USB device searcher, binds its RPC server again, forgets the device and
 * re-reads the device list. Returns 0, or -1 for modem-type devices and the AN986 driver kind.
 */
s32 Net_StopDevice(void)
{
    s32 modId;

    gNetStopReq = 1;
    switch (gNetDevCur->type) {
    case NET_DEV_MODEM:
        return -1;
    case NET_DEV_ETHERNET:
        switch (gNetDevCur->driver) {
        case NET_DRV_AN986:
            return -1;
        }
        if (0 <= (modId = sceSifSearchModuleByName(D_0046A560))) {
            Irx_Unload(modId);
        }
        modId = -1;
        NetTcp_Term();
        switch (gNetDevCur->driver) {
        case NET_DRV_LANEGG:
            modId = sceSifSearchModuleByName(D_0046A580);
            break;
        case NET_DRV_SMAP:
            modId = sceSifSearchModuleByName(D_0046A5A0);
            break;
        }
        if (0 <= modId) {
            Irx_Unload(modId);
        }
        Net_LoadIrx(gNetIrxUsbsrch, 0, 0, 0, 0);
        SifRpc_BindWait(&gNetDevRpc, 0x1270010);
        gNetDevCur = 0;
        NetDev_ClearList();
        NetDev_Refresh();
        break;
    case NET_DEV_PPP_OTHER:
        return -1;
    }
    return 0;
}

extern char D_0046A5C0[];
extern char D_0046A5E0[];
extern char D_0046A5E8[];
extern s32 gNetIrxAn986;
extern s32 gNetIrxLanegg;
extern s32 gNetIrxSmap;
extern s32 gNetIrxAvewrape;
void Net_LoadIrx(s32 mod, s32 argc, char *argv, s32 buf, s32 tries);
s32 sceSifSearchModuleByName(char *name);
s32 Net_Init(void);

INCLUDE_RODATA("asm/sub_main/nonmatchings/net_front", D_0046A5C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/net_front", D_0046A5E0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/net_front", D_0046A5E8);

/*
 * Unloads the USB device searcher if it is loaded. For an Ethernet device, loads its driver
 * (AN986, LANEGG or SMAP, with a thread-priority argument) and the Ethernet AVE wrapper, then
 * Net_Init. Always returns 1.
 */
s32 Net_LoadDriverType1(void)
{
    s32 modId;

    if (0 <= (modId = sceSifSearchModuleByName(D_0046A5C0))) {
        Irx_Unload(modId);
    }
    switch (gNetDevCur->type) {
    case NET_DEV_MODEM:
        break;
    case NET_DEV_ETHERNET:
        switch (gNetDevCur->driver) {
        case NET_DRV_AN986:
            Net_LoadIrx(gNetIrxAn986, 8, D_0046A5E0, 0, 0);
            break;
        case NET_DRV_LANEGG:
            Net_LoadIrx(gNetIrxLanegg, 8, D_0046A5E0, 0, 0);
            break;
        case NET_DRV_SMAP:
            Net_LoadIrx(gNetIrxSmap, 9, D_0046A5E8, 0, 0);
            break;
        }
        Net_LoadIrx(gNetIrxAvewrape, 0, 0, 0, 0);
        Net_Init();
        break;
    case NET_DEV_PPP_OTHER:
        break;
    }
    return 1;
}

extern s32 gNetIrxCxtmdm;
extern s32 gNetIrxOstmdmA;
extern s32 gNetIrxAveppp;
extern s32 gNetIrxAvewrapm;
extern s32 gNetIrxAvedhcp;
extern s32 gNetIrxDevglue;
extern char D_0046A5F8[];
extern s8 gNetDriverType;

INCLUDE_RODATA("asm/sub_main/nonmatchings/net_front", D_0046A5F8);

/*
 * Loads the support modules for the device in use, unless gNetStopReq is set. First unloads
 * the USB device searcher. A modem gets its modem driver (kind 2 or 4), AVE PPP and the modem
 * AVE wrapper, then Net_Init (gNetDriverType = 2). An Ethernet device gets the device glue,
 * AVE DHCP and AVE PPP modules (gNetDriverType = 1). Always returns 1.
 */
s32 Net_LoadDrivers(void)
{
    s32 modId;

    if (gNetStopReq != 0) {
        return 1;
    }
    if (0 <= (modId = sceSifSearchModuleByName(D_0046A5C0))) {
        Irx_Unload(modId);
    }
    switch (gNetDevCur->type) {
    case NET_DEV_MODEM:
        switch (gNetDevCur->driver) {
        case NET_DRV_CXTMDM:
            Net_LoadIrx(gNetIrxCxtmdm, 0, 0, 0, 0);
            break;
        case NET_DRV_OSTMDM:
            Net_LoadIrx(gNetIrxOstmdmA, 0, 0, 0, 0);
            break;
        }
        Net_LoadIrx(gNetIrxAveppp, 1, D_0046A5F8, 0, 0);
        Net_LoadIrx(gNetIrxAvewrapm, 0, 0, 0, 0);
        Net_Init();
        gNetDriverType = 2;
        break;
    case NET_DEV_ETHERNET:
        Net_LoadIrx(gNetIrxDevglue, 1, D_0046A5F8, 0, 0);
        Net_LoadIrx(gNetIrxAvedhcp, 0, 0, 0, 0);
        Net_LoadIrx(gNetIrxAveppp, 1, D_0046A5F8, 0, 0);
        gNetDriverType = 1;
        break;
    case NET_DEV_PPP_OTHER:
        break;
    }
    return 1;
}

extern s32 D_0059CBC4;
extern s32 D_0059CBC8;
extern s32 D_0059CBCC;

/*
 * Re-reads the device list from the IOP when no device is in use and the IOP's count has
 * changed (RPC 0). For each of the 32 slots it asks RPC 1 for the device, then fills
 * gNetDevs[i] (NetDev_Identify) or marks it unused. gNetDevCount also counts the extra slots
 * 32-34 that are in use. Returns 1 if the list was re-read, else 0.
 */
s32 NetDev_Refresh(void)
{
    u32 count;
    s32 i;
    NetDevice *dev;
    s32 *pStatus;
    s32 *pRpcWord2;
    s32 *pProductId;

    if (gNetDevCur != 0) {
        return 0;
    }
    NetDev_CallRpc(0);
    count = gNetDevRpcBuf;
    if (count != gNetDevSeenCount) {
        i = 0;
        dev = gNetDevs;
        pStatus = &D_0059CBCC;
        pRpcWord2 = &D_0059CBC8;
        pProductId = &D_0059CBC4;
        for (; i < 32; i++, dev++) {
            gNetDevRpcBuf = i;
            NetDev_CallRpc(1);
            if (gNetDevRpcBuf == -1) {
                memset(dev, 0xFF, sizeof(NetDevice));
            } else {
                NetDev_Identify(dev, gNetDevRpcBuf, *pProductId, *pRpcWord2, *pStatus);
            }
        }
        gNetDevSeenCount = count;
        gNetDevCount = count;
        for (i = 32; i < 35; i++) {
            if (gNetDevs[i].type != -1) {
                gNetDevCount++;
            }
        }
        return 1;
    }
    return 0;
}

/* Binds an RPC client to server id, retrying until the server answers. */
void SifRpc_BindWait(SifClientData *client, u32 serverId)
{
    volatile s32 i;

    for (;;) {
        if (sceSifBindRpc(client, serverId, 0) >= 0 && client->serve != NULL) {
            break;
        }
        for (i = 0x10000; i != 0; i--) {
        }
    }
}

/* Starts the TCP layer (NetTcp_Init). Returns 0. */
s32 Net_Init(void)
{
    NetTcp_Init();
    return 0;
}

extern s32 gNetStopReq;
extern s32 D_0059CC30;
extern s32 gNetIrxAvetcp;
extern s32 gNetIrxUsbsrch;
extern char D_0046A5F8[];
extern char D_0046A600[];
void Net_LoadIrx(s32 mod, s32 argc, char *argv, s32 buf, s32 tries);
s32 sceSifSearchModuleByName(char *name);
void NetCfg_Reset(void);
s32 NetDev_Refresh(void);

INCLUDE_RODATA("asm/sub_main/nonmatchings/net_front", D_0046A600);
INCLUDE_RODATA("asm/sub_main/nonmatchings/net_front", D_0046A610);
INCLUDE_RODATA("asm/sub_main/nonmatchings/net_front", D_0046A640);
INCLUDE_RODATA("asm/sub_main/nonmatchings/net_front", D_0046A670);
INCLUDE_RODATA("asm/sub_main/nonmatchings/net_front", D_0046A6A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/net_front", D_0046A6D0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/net_front", D_0046A700);
INCLUDE_RODATA("asm/sub_main/nonmatchings/net_front", D_0046A730);
INCLUDE_RODATA("asm/sub_main/nonmatchings/net_front", D_0046A760);
INCLUDE_RODATA("asm/sub_main/nonmatchings/net_front", D_0046A790);
INCLUDE_RODATA("asm/sub_main/nonmatchings/net_front", D_0046A7C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/net_front", D_0046A7F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/net_front", D_0046A820);
INCLUDE_RODATA("asm/sub_main/nonmatchings/net_front", D_0046A848);

/*
 * Starts the network. Resets the settings and device list, loads the AVE TCP module, and adds
 * the network adaptor as gNetDevs[34] (status 0x21) when the expansion-bay driver is already
 * loaded. Then loads the USB device searcher, binds its RPC server and reads the device list.
 * Returns 0.
 */
s32 Net_Start(void)
{
    NetCfg_Reset();
    NetDev_ClearList();
    gNetStopReq = 0;
    D_0059CC30 = 0;
    Net_LoadIrx(gNetIrxAvetcp, 1, D_0046A5F8, 0, 0);
    if (0 <= sceSifSearchModuleByName(D_0046A600)) {
        gNetDevs[34].type = gNetDefaultDev[0].type;
        gNetDevs[34].driver = gNetDefaultDev[0].driver;
        gNetDevs[34].vendorId = gNetDefaultDev[0].vendorId;
        gNetDevs[34].productId = gNetDefaultDev[0].productId;
        gNetDevs[34].status = 0x21;
        gNetDevs[34].vendor = gNetDefaultDev[0].vendor;
        gNetDevs[34].product = gNetDefaultDev[0].product;
    }
    Net_LoadIrx(gNetIrxUsbsrch, 0, 0, 0, 0);
    SifRpc_BindWait(&gNetDevRpc, 0x1270010);
    NetDev_Refresh();
    return 0;
}

extern u8 *gLockrockKey;              /* the 8-byte key */

/* Obfuscates len bytes of in into out: each byte xored with the key byte (i & 7) and with
 * salt + seed + i; returns the sum of the input bytes & 0x7FFF, -1 without buffers. */
s32 Net_LockrockObfuscate(u8 *out, u8 *in, u8 seed, s32 len, s32 salt)
{
    s32 i;
    s32 sum = 0;

    if (out == NULL || in == NULL) {
        return -1;
    }
    for (i = 0; i < len; i++) {
        sum += *in;
        *out++ = *in++ ^ gLockrockKey[i & 7] ^ (salt + (seed + i));
    }
    return sum & 0x7FFF;
}

/* Writes the n lowest decimal digits of v at out (no terminator). */
static s32 Str_FormatDigits(char *out, s32 value, s32 width)
{
    s32 i;

    out += width - 1;
    for (i = 0; i < width; i++) {
        *out-- = value % 10 + '0';
        value /= 10;
    }
    return 0;
}

/* The number in the first n characters (digits only; others are skipped). */
static s32 Str_ParseDigits(s8 *str, s32 len)
{
    s32 value = 0;
    s32 i;

    for (i = 0; i < len; i++, str++) {
        if (*str >= '0' && *str <= '9') {
            value *= 10;
            value += *str - '0';
        }
    }
    return value;
}

/* The reply to a lobby challenge: out = the two 4-digit halves of ch plus key, as two 5-digit
 * numbers (10 characters); -1 without a buffer or a challenge. */
s32 Net_ChallengeReply(char *out, s8 *challenge, u16 key)
{
    s32 half0;
    s32 half1;

    if (out == NULL || challenge == NULL) {
        return -1;
    }
    half0 = key + Str_ParseDigits(challenge, 4);
    half1 = key + Str_ParseDigits(challenge + 4, 4);
    Str_FormatDigits(out, half0, 5);
    Str_FormatDigits(out + 5, half1, 5);
    out[10] = 0;
    return 0;
}

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
    memset(gNetHostCache, 0, 0x820U);
    gNetHostCacheNext = 0;
    return 0;
}

extern s16 gNetDnsReq;
s32 net_inet_addr2(s8 *name);
s32 strncmp(s8 *a, s8 *b, s32 n);
s32 NetDns_Request(s8 *name);
s32 NetDns_Free(s16 id);
s32 NetDns_LookUp(s16 id, s32 *addr);

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
            if (strncmp(entry->name, name, 0x100) == 0) {
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
            strncpy((char *)entry->name, (char *)name, 0x100);
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
s32 NetDns_Init(char *server, s32 a, s32 b);
s32 NetPpp_GetInfo(s32 *out);
s32 NetDhcp_GetDns(char *name, s32 *b);

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
    memset(&gNetPppStatus, 0, 0x14U);
    memset(&gNetTcpStatus, 0, 8U);
    gNetSock = -1;
}

s32 NetMsg_Dispatch(s8 *handler, s8 *step, s8 *discStep, s16 *timer, u16 *cancelHold, u16 *retries, u16 *retries2, s8 *err);

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

s32 NetIf_GetState(void);
s32 NetIf_PollEvent(s32 *out);
s32 NetIf_GetEvent(s32 a, s32 *b);
s32 NetIf_AckEvent(s32 a);

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

void Net_DebugLog(s32 a, s32 b, s32 c, char *fmt, ...);
typedef s32 (*NetMsgFn)(s8 *handler, s8 *step, s8 *discStep, s16 *timer, u16 *cancelHold, u16 *retries, u16 *retries2, s8 *err);
extern NetMsgFn gNetMsgHandlers[];
extern char D_0046A860[];

INCLUDE_RODATA("asm/sub_main/nonmatchings/net_front", D_0046A860);

/*
 * Logs (Net_DebugLog, compiled out) and runs the connection handler gNetMsgHandlers[*handler]
 * (NET_HANDLER_*), passing it gNetMsgState's step, disconnect step, timer, cancel count, two
 * retry counts and error code. Returns the handler's value (0 busy, NET_STEP_*).
 */
s32 NetMsg_Dispatch(s8 *handler, s8 *step, s8 *discStep, s16 *timer, u16 *cancelHold, u16 *retries, u16 *retries2, s8 *err)
{
    Net_DebugLog(1, 0x1B, 1, D_0046A860, *timer, *handler, *step, *discStep, *err);
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
s32 NetEth_Init(void);
s32 NetEth_SetAddress(s32 *p);
s32 NetEth_Start(s32 a);
s32 NetEth_IsLinkUp(void);
s32 NetEth_SetConnected(s32 unused0);
s32 NetIf_GetOption8(void *buf);
s32 NetRoute_Add(s32 a, s32 b);
s32 NetDhcp_Init(void);
s32 NetDhcp_Term(void);
s32 NetDhcp_Start(s32 a, s32 b);
s32 NetDhcp_GetState(void);
s32 NetDhcp_GetAddress(s32 *p);
s32 NetDhcp_GetRouter(s32 *out);

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

INCLUDE_RODATA("asm/sub_main/nonmatchings/net_front", D_0046A888);

extern char D_0046A888[];
extern s16 gNetPppPhase;
extern s16 gNetPppError;
s32 NetPpp_Init(void);
s16 NetPpp_SetOption1(void *src);
s32 NetModem_GetChat(s32 mode, s32 kind, s32 a2, s32 a3, char **out);
s32 NetPpp_GetLinkState(void);
s32 NetPpp_Connect(NetDialArg *arg);
s32 NetPpp_SetConnected(s32 unused0);

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
        memset(&gNetPppLineMode, 0, 0x10);
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
        memset(&gNetPppStatus, 0, 0x14);
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
                    *step = 0x63;
                } else {
                    NET_RESET(NET_ERR_PPP_FLAG);
                }
                break;
            case NET_PPP_ERR_BUSY:
                if (--(*retries2) == 0) {
                    NET_RESET(NET_ERR_PPP_LINE);
                } else {
                    *step = 0x63;
                }
                break;
            case NET_PPP_ERR_RETRY:
                if (--(*retries) == 0) {
                    NET_RESET(NET_ERR_FAILED);
                } else {
                    *step = 0x63;
                }
                break;
            case NET_PPP_ERR_LINEMODE:
                if (NetPpp_NextLineMode() == 0) {
                    *step = 0x63;
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
    case 0x63:
        (*step)++;
        *discStep = 0;
        *timer = 0;
        return 0;
    case 0x64:
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
                *step = 0x63;
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
                *step = 0x63;
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
                *step = 0x63;
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
    case 0x63:
        (*step)++;
        *discStep = 0;
        *timer = 0;
        return 0;
    case 0x64:
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

extern char D_0046A8C0[];
extern s16 gNetPppPhase;
extern s32 D_0059D61C;
s32 NetDns_Term(void);
s16 NetIf_Down(void);
s32 NetPpp_SetClosed(s32 unused0);
s32 NetPpp_Disconnect(s32 unused0);
s32 NetPpp_Term(void);
s32 NetEth_ClearAddress(s32 unused0);
s32 NetEth_Stop(void);
s32 NetEth_Term(void);
s32 NetRoute_Delete(s32 a, s32 b);
s32 NetDhcp_Term(void);
s32 NetDhcp_GetState(void);
s32 NetDhcp_Release(s32 *p, s32 b);
s32 NetTcp_Abort(s32 sock);
s32 NetTcp_Delete(s32 *sock);

INCLUDE_RODATA("asm/sub_main/nonmatchings/net_front", D_0046A8C0);

/* NETWORK: the disconnect steps (state *st, timer *t): closes the socket (gNetSock), stops the
 * device by its type and returns 1 when done. */
s32 Net_DisconnectStep(s8 *state, s16 *timer)
{
    s16 left;
    s32 pppResult;

    Net_DebugLog(1, 0x1A, 1, D_0046A8C0);
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

/*
 * The result of the last DNAS install call made by NetXfer_Step or NetXfer_Step2
 * (gNetXferResult), negative on error.
 */
s32 NetXfer_GetResult(void)
{
    return gNetXferResult;
}

extern s32 gNetXferSize;
s32 func_AE2518(void);
s32 func_AE8638(s32 a, s32 b, s32 *out);
s32 func_AE8688(s32 a, s32 b);
void func_AE2570(void);

/*
 * One step of passing size bytes at buf through the DNAS install library (the DNAS_INS
 * overlay must be loaded). States: 0 start, 10 get the output size (failed if over
 * maxSize), 11 process, 15 close, 0x63 failed, 0x64 done. Returns the output size
 * (gNetXferSize) when done, -1 on failure (the cause in gNetXferResult), 0 while busy.
 * timer is not used.
 */
s32 NetXfer_Step(s8 *state, s16 *timer, u8 *buf, s32 size, s32 maxSize)
{
    switch (*state) {
    case 0:
        *state = 10;
        gNetXferSize = 0;
        gNetXferResult = func_AE2518();
        if (gNetXferResult < 0) {
            *state = 0x63;
        }
        break;
    case 10:
        (*state)++;
        gNetXferResult = func_AE8638(size, (s32) buf, &gNetXferSize);
        if (gNetXferResult < 0) {
            *state = 0x63;
        } else if (gNetXferSize > maxSize) {
            *state = 0x63;
        }
        break;
    case 11:
        *state = 15;
        gNetXferResult = func_AE8688(size, gNetXferSize);
        if (gNetXferResult < 0) {
            *state = 0x63;
        }
        break;
    case 15:
        *state = 0x64;
        func_AE2570();
        break;
    case 0x63:
        *state = 0;
        func_AE2570();
        return -1;
    case 0x64:
        *state = 0;
        return gNetXferSize;
    }
    return 0;
}

s32 func_AE2588(s32 a, s32 b, s32 *out);
s32 func_AE2EC8(s32 a, s32 b);

/*
 * NetXfer_Step with the library's other pair of calls (for the patch records). In state
 * 11 the game passes a local it never set as the first argument.
 */
s32 NetXfer_Step2(s8 *state, s16 *timer, u8 *buf, s32 size, s32 maxSize)
{
    volatile s32 handle;

    switch (*state) {
    case 0:
        *state = 10;
        gNetXferSize = 0;
        gNetXferResult = func_AE2518();
        if (gNetXferResult < 0) {
            *state = 0x63;
        }
        break;
    case 10:
        (*state)++;
        gNetXferResult = func_AE2588(size, (s32) buf, &gNetXferSize);
        if (gNetXferResult < 0) {
            *state = 0x63;
        } else if (gNetXferSize > maxSize) {
            *state = 0x63;
        }
        break;
    case 11:
        *state = 15;
        /* the game reads a stack word it never set: h is the word at sp+0x3C, nothing here writes it */
        gNetXferResult = func_AE2EC8(handle, gNetXferSize);
        if (gNetXferResult < 0) {
            *state = 0x63;
        }
        break;
    case 15:
        *state = 0x64;
        func_AE2570();
        break;
    case 0x63:
        *state = 0;
        func_AE2570();
        return -1;
    case 0x64:
        *state = 0;
        return gNetXferSize;
    }
    return 0;
}

/*
 * The result of the last DNAS call (gDnasResult): the error the lobby shows after
 * Dnas_Step fails; DNAS_RESULT_CANCELED when cancelled or the disc did not get ready.
 */
s32 Dnas_GetError(void)
{
    return gDnasResult;
}

/*
 * One step of the DNAS authentication at login (Dnas_Step's DNAS_KIND_AUTH, which also
 * restarts the sockets and clears the host cache). Returns 1 when done, -1 on failure,
 * else 0.
 */
s32 Dnas_AuthStep(s8 *state, s16 *cancel, s16 *percent)
{
    return Dnas_Step(state, cancel, percent, DNAS_KIND_AUTH);
}

/*
 * One step of the DNAS check before a patch download (Dnas_Step's DNAS_KIND_INST).
 * Returns 1 when done, -1 on failure, else 0.
 */
s32 Dnas_StateMachine(s8 *state, s16 *cancel, s16 *percent)
{
    return Dnas_Step(state, cancel, percent, DNAS_KIND_INST);
}

extern u16 gDnasWait;
extern char gDnasProxyHost[];
extern s32 gDnasFixedDns;
extern s32 gDnasStatusPrev[4];
extern s32 D_004054D0;
extern s32 D_004054E0;
extern s8 gProxyMode;
extern char D_005C3518[];
extern char D_005C3627[];
s32 NetIf_SetErrorCheck(s32 on);
s32 sceCdDiskReady(s32 a, s32 b);
s32 sceCdStandby(s32 a, s32 b);
s32 sceCdSync(s32 a, s32 b);
s32 memcmp(void *a, void *b, s32 n);
void *memcpy(void *dst, const void *src, u32 n);
s32 func_AC99D0(void *p, s32 a, s32 b, s32 c, s32 d);
s32 func_AC9A08(void);
s32 func_AC9D78(void);
s32 func_ACA070(void);
s32 func_ACA188(s32 *out);
s32 func_ACA1E8(s32 a, char *host, u16 port);
s32 func_ACA4E0(void);

typedef struct DnasParam {
    s32 unk0;
    s32 *passPhrase;
    s32 passPhraseSize;
    s32 *authData;
    s32 authDataSize;
} DnasParam;

extern u8 *gPadInputPtr;
/*
 * One step of the DNAS check (state *st). Gets the disc ready, starts the DNAS library
 * with the pass phrase and the authentication data, passes on the HTTP or HTTPS proxy,
 * starts the check of the kind (DNAS_KIND_*), then polls its status, raising *percent to
 * 100. *cancel at NET_CANCEL_AT or more aborts (the lobby raises it while Start is held).
 * With a modem and Select+Triangle+Square held on pad 1 at the start, the check uses two
 * built-in DNS servers (gDnasFixedDns) and the normal resolver comes back after.
 * Returns 1 when done, -1 on failure (the cause in gDnasResult), 0 while busy.
 */
s32 Dnas_Step(s8 *state, s16 *cancel, s16 *percent, s32 kind)
{
    DnasParam authInfo;
    s32 status[4];
    char *colon;

    switch (*state) {
    case 0:
        gDnasResult = 0;
        gDnasFixedDns = 0;
        if (gNetDevCur->type == NET_DEV_MODEM && *(s32 *)(gPadInputPtr + 8) == 0x4300) {
            gDnasFixedDns = 1;
        }
        if (gDnasFixedDns != 0) {
            NetDns_Term();
        }
        NetIf_SetErrorCheck(0);
        *state = 0x10;
        *percent = 0;
        gDnasWait = 0;
        break;
    case 0xF:
        sceCdDiskReady(0, 3);
        *state = 1;
        *percent = 3;
        gDnasWait = 0;
        break;
    case 0x10:
        gDnasWait++;
        *percent = 1;
        if (sceCdStandby(1, 3) == 1 || gDnasWait > 300) {
            *state = 0x11;
            gDnasWait = 0;
            break;
        }
        if (*cancel >= NET_CANCEL_AT) {
            gDnasResult = DNAS_RESULT_CANCELED;
            *state = 0;
            NetIf_SetErrorCheck(1);
            if (gDnasFixedDns != 0) {
                *state = 0x62;
                break;
            }
            return -1;
        }
        break;
    case 0x11:
        gDnasWait++;
        *percent = 2;
        if (sceCdSync(1, 3) == 0 || gDnasWait > 300) {
            *state = 0x12;
            gDnasWait = 0;
        }
        break;
    case 0x12:
        gDnasWait++;
        *percent = 3;
        if (sceCdDiskReady(1, 3) == 2 || gDnasWait > 300) {
            *state = 1;
            gDnasWait = 0;
            break;
        }
        if (*cancel >= NET_CANCEL_AT) {
            gDnasResult = DNAS_RESULT_CANCELED;
            *state = 0;
            NetIf_SetErrorCheck(1);
            if (gDnasFixedDns != 0) {
                *state = 0x62;
                break;
            }
            return -1;
        }
        break;
    case 1:
        if (gDnasFixedDns != 0) {
            NetDns_Init((char *)&gNetDomain, 0x79F3D5CA, 0x59F3D5CA);
        }
        if (kind == DNAS_KIND_AUTH) {
            Net_InitSockets();
            net_clear_host_cache();
        }
        (*state)++;
        *percent = 5;
        break;
    case 2:
        memset(gDnasStatusPrev, 0, 0x10);
        memset(&authInfo, 0, 0x14);
        authInfo.unk0 = 1;
        authInfo.authData = &D_004054E0;
        authInfo.authDataSize = 0x10000;
        authInfo.passPhrase = &D_004054D0;
        authInfo.passPhraseSize = 8;
        gDnasResult = func_AC99D0(&authInfo, 0x30, 0, 0, 0);
        if (gDnasResult < 0) {
            *state = 0x63;
        } else {
            (*state)++;
            *percent = 10;
        }
        break;
    case 3:
        if (gProxyMode != 0) {
            gDnasProxyHost[0x10F] = 0;
            if ((s8)(s32)gProxyMode & 2) {
                strncpy(gDnasProxyHost, D_005C3518, 0x10F);
            } else {
                strncpy(gDnasProxyHost, D_005C3627, 0x10F);
            }
            colon = gDnasProxyHost + strlen((s8 *)gDnasProxyHost) - 1;
            do {
                if (*colon == ':') {
                    break;
                }
                colon--;
            } while (gDnasProxyHost < colon);
            if (gDnasProxyHost < colon) {
                *colon = 0;
                func_ACA1E8(1, gDnasProxyHost, atoi((s8 *)colon + 1));
            }
        }
        if (gDnasResult < 0) {
            *state = 0x63;
        } else {
            (*state)++;
            *percent = 20;
        }
        break;
    case 4:
        switch (kind) {
        case DNAS_KIND_AUTH:
            gDnasResult = func_ACA4E0();
            break;
        case DNAS_KIND_INST:
            gDnasResult = func_AC9A08();
            break;
        default:
            while (1) {
            }
        }
        if (gDnasResult < 0) {
            *state = 0x63;
        } else {
            *state = 0xA;
            *percent = 30;
        }
        break;
    case 0xA:
        memset(status, 0, 0x10);
        gDnasResult = func_ACA188(status);
        if (gDnasResult < 0) {
            *state = 0x63;
        } else if (memcmp(gDnasStatusPrev, status, 0x10) != 0) {
            if (*percent < 0x50) {
                *percent += 10;
            }
            if (status[0] == 5) {
                memcpy(gDnasStatusPrev, status, 0x10);
                *state = 0x64;
            } else if (status[0] == 6 || status[0] == 7) {
                gDnasResult = status[1];
                *state = 0x63;
            } else {
                goto same;
            }
        } else {
        same:
            memcpy(gDnasStatusPrev, status, 0x10);
            if (*cancel >= NET_CANCEL_AT) {
                gDnasResult = func_ACA070();
                *state = 0x5A;
            }
        }
        break;
    case 0x5A:
        memset(status, 0, 0x10);
        gDnasResult = func_ACA188(status);
        if (status[0] == 2) {
            gDnasResult = DNAS_RESULT_CANCELED;
            *state = 0x63;
        }
        break;
    case 0x63:
        *state = 0;
        func_AC9D78();
        NetIf_SetErrorCheck(1);
        if (gDnasFixedDns != 0) {
            NetDns_Term();
            *state = 0x62;
            break;
        }
        return -1;
    case 0x62:
        *state = 0;
        Net_InitDns();
        return -1;
    case 0x64:
        *state = 0x7F;
        gDnasResult = func_AC9D78();
        NetIf_SetErrorCheck(1);
        if (gDnasFixedDns != 0) {
            NetDns_Term();
            *state = 0x65;
        }
        if (*percent < 0x5A) {
            *percent = 0x5A;
        }
        break;
    case 0x65:
        *state = 0x7F;
        Net_InitDns();
        break;
    case 0x7F:
        *state = 0;
        if (*percent < 100) {
            *percent = 100;
        }
        return 1;
    }
    return 0;
}

s32 NetTcp_GetOptions(s32 sock, s32 *out);
s32 NetTcp_SetOptions(s32 sock, s32 *in);

/*
 * Reads socket sock's two AVE TCP options, then sets option 0x2008 to 1 and option
 * 0x2001 to 0. The lobby applies it to gNetSock.
 */
void NetTcp_SetOption(s32 sock)
{
    s32 opts[2];

    NetTcp_GetOptions(sock, opts);
    opts[0] = 1;
    opts[1] = 0;
    NetTcp_SetOptions(sock, opts);
}

INCLUDE_RODATA("asm/sub_main/nonmatchings/net_front", D_0046A8E0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/net_front", D_0046A8F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/net_front", D_0046A900);
INCLUDE_RODATA("asm/sub_main/nonmatchings/net_front", D_0046A910);
INCLUDE_RODATA("asm/sub_main/nonmatchings/net_front", D_0046A918);
INCLUDE_RODATA("asm/sub_main/nonmatchings/net_front", D_0046A928);

/*
 * Clears the network settings: no device in use (gNetDevCur NULL, gNetDevs filled with
 * 0xFF, both device counts -1, gNetDriverType 0), static LAN mode, the PPP, address, DNS
 * and authentication settings zeroed, the system language saved, gNetSema -1. Rebuilds
 * gNetOptList: the max-retries and initial-latency keys and values are copied into their
 * buffers, the DVD-init pair is pointed at directly, then two NULLs.
 */
void NetCfg_Reset(void)
{
    gNetDevCur = NULL;
    memset((void *) &gNetDevs, 0xFF, 0x3D4U);
    gNetDriverType = 0;
    gNetDevSeenCount = -1;
    gNetDevCount = -1;
    D_0059DE00 = 0;
    memset((void *) &D_0059DD00, 0, 0x100U);
    D_0059DCF8 = 0;
    D_0059DCF0 = 0;
    gNetLanMode = 0;
    gNetPppFlagSaved = 0;
    memset((void *) &gNetPppConfBuf, 0, 0xF0U);
    memset((void *) &gNetPppAuthText, 0, 0x10U);
    memset((void *) &gNetDomain, 0, 0x100U);
    gNetDns1 = 0;
    gNetDns2 = 0;
    memset((void *) &gNetAuthName, 0, 0x100U);
    memset((void *) &gNetAuthKey, 0, 0x100U);
    gNetPhyMode = 0;
    gNetPppLineModeFirst = 0;
    gNetIpAddr = 0;
    gNetNetmask = 0;
    gNetGateway = 0;
    D_0059D8A0 = sceScfGetLanguage();
    memset((void *) &D_0059D820, 0, 0x80U);
    D_0059D818 = 0;
    gNetSema = -1;
    D_0059D800 = -1;
    D_0059D808 = 0;
    memset((void *) &D_0059D700, 0, 0x100U);
    strncpy((char *) &gNetOptMaxRetriesKey, (const char *) &D_0046A8E0, 0x20U);
    strncpy((char *) &gNetOptMaxRetries, (const char *) &D_0046A8F0, 0x10U);
    strncpy((char *) &gNetOptLatencyKey, (const char *) &D_0046A900, 0x20U);
    strncpy((char *) &gNetOptLatency, (const char *) &D_0046A910, 0x10U);
    D_0059D698 = 0;
    gNetOptList = &gNetOptMaxRetriesKey;
    D_0059D684 = &gNetOptMaxRetries;
    D_0059D688 = &gNetOptLatencyKey;
    D_0059D68C = &gNetOptLatency;
    D_0059D690 = &D_0046A918;
    D_0059D694 = &D_0046A928;
    D_0059D69C = 0;
}

/* This file's initialised data. */
extern char gNetUsbLanVendor[];
extern char gNetUsbLanProduct1[];
extern char gNetUsbLanProduct2[];
extern char gNetAdaptorVendor[];
extern char gNetAdaptorProduct[];
extern u32 D_0046A610[];
extern u32 D_0046A640[];
extern u32 D_0046A670[];
extern u32 D_0046A6A0[];
extern u32 D_0046A6D0[];
extern u32 D_0046A700[];
extern u32 D_0046A730[];
extern u32 D_0046A760[];
extern u32 D_0046A790[];
extern u32 D_0046A7C0[];
extern u32 D_0046A7F0[];
extern u32 D_0046A820[];
extern u32 D_0046A848[];
s32 gNetSockWaitSema = {
#include "sub_main/004053A8.inc"
};
s32 gNetSockSema = {
#include "sub_main/004053B0.inc"
};
NetDevice gNetDevNames[3] = {
#include "sub_main/004053C0.inc"
};
NetDevice gNetDefaultDev[2] = {
#include "sub_main/00405420.inc"
};
s32 gNetIrxCxtmdm = (s32)D_0046A610;
s32 gNetIrxOstmdmA = (s32)D_0046A640;
s32 gNetIrxAvetcp = (s32)D_0046A670;
s32 gNetIrxAveppp = (s32)D_0046A6A0;
s32 gNetIrxAvewrapm = (s32)D_0046A6D0;
s32 gNetIrxUsbsrch = (s32)D_0046A700;
s32 gNetIrxAvewrape = (s32)D_0046A730;
s32 gNetIrxAvedhcp = (s32)D_0046A760;
s32 gNetIrxDevglue = (s32)D_0046A790;
s32 gNetIrxAn986 = (s32)D_0046A7C0;
s32 gNetIrxLanegg = (s32)D_0046A7F0;
s32 gNetIrxSmap = (s32)D_0046A820;
u8 *gLockrockKey = (u8 *)D_0046A848;
NetMsgFn gNetMsgHandlers[4] = {
    (NetMsgFn)Net_ConnectPppStep, (NetMsgFn)Net_ConnectLanStep, (NetMsgFn)Net_Disconnect, NULL,
};
INCLUDE_DATA("build/blobs/sub_main", D_004054D0);
INCLUDE_DATA("build/blobs/sub_main", D_004054E0);
