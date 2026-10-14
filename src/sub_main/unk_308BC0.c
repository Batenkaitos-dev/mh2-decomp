#include "common.h"
#include "em.h"
#include "sif.h"
#include "yn.h"                /* NetDevice, gNetDevs */

/* The game built this file at optimization level 4. */
#pragma optimization_level 4

/* prototypes */
s32 NetTcp_Init(void);
void NetSock_Lock(void);
void NetSock_Unlock(void);
s32 net_inet_addr(s8 *);
s32 NetDev_MatchesDefault(s8 *vendor, s8 *product);
/* Called with 2 and with 8 arguments: no prototype. */
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
extern s16 D_0059D4B0;
extern s32 gNetXferResult;
extern s32 gDnasResult;
extern s32 gNetPppStatus;
extern u8 gNetTcpStatus;
extern s32 gNetSock;
extern u8 gNetMsgState;
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
extern u8 D_0059D8C0;
extern u8 gNetPhyMode;
extern u8 gNetAuthKey;
extern u8 gNetAuthName;
extern s32 gNetDns2;
extern s32 gNetDns1;
extern s8 gNetDomain;
extern s8 D_0059DBE0;
extern u8 D_0059DBF0;
extern u8 D_0059DCE0;
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

/* inet_addr(): the address in dotted form (in network order), 0 when str is not one (a run of
 * more than 3 digits, 4 dots or another character). */
/* unk_308BC0's .rodata (0x46A508-0x46A560) */
#pragma rodata on
char D_0046A508[0xC] = {
#include "sub_main/0046A508.inc"
};
char D_0046A518[0xC] = {
#include "sub_main/0046A518.inc"
};
char D_0046A528[0xC] = {
#include "sub_main/0046A528.inc"
};
char D_0046A538[0x8] = {
#include "sub_main/0046A538.inc"
};
char D_0046A540[0x20] = {
#include "sub_main/0046A540.inc"
};
#pragma rodata off

s32 net_inet_addr(s8 *str)
{
    s32 dots = 0;
    s32 run = 0;
    s8 *p;
    u32 i;
    s32 c0;
    u8 a;
    u8 b;
    u8 c;
    u8 d;

    for (p = str; (c0 = *p); p++) {
        if (c0 >= '0' && c0 <= '9') {
            if (++run == 4) {
                return 0;
            }
        } else if (c0 == '.') {
            if (++dots == 4) {
                return 0;
            }
            run = 0;
        } else {
            return 0;
        }
    }
    i = 0;
    a = atoi(str);
    for (; i < strlen(str) && str[i] != '.'; i++) {
    }
    if (i < strlen(str)) {
        i++;
    }
    b = atoi(str + i);
    for (; i < strlen(str) && str[i] != '.'; i++) {
    }
    if (i < strlen(str)) {
        i++;
    }
    c = atoi(str + i);
    for (; i < strlen(str) && str[i] != '.'; i++) {
    }
    if (i < strlen(str)) {
        i++;
    }
    d = atoi(str + i);
    return ((d << 24) & 0xFF000000) | (((c << 16) & 0xFF0000) | ((a & 0xFF) | ((b << 8) & 0xFF00)));
}

s32 net_htons(s32 x)
{
    s32 v;

    v = x & 0xFFFF;
    return (((v << 8) & 0xFF00) | ((v >> 8) & 0xFF)) & 0xFFFF;
}

s32 net_inet_aton(char *s, s32 *out)
{
    *out = net_inet_addr((s8 *)s);
    return *out != 0;
}

s32 * net_errno_ptr(void)
{
    return &gNetErrno;
}

s32 * net_h_errno_ptr(void)
{
    return &gNetHErrno;
}

extern s32 gNetSockHandles[];
extern s32 gNetSockClose[8];

/* Marks socket s (0-7) for closing (gNetSockClose[s] = 1); 0, or -1 for a bad or free socket. */
s32 net_cancel(s32 s)
{
    if (s < 0 || s >= 8) {
        return -1;
    }
    NetSock_Lock();
    if (gNetSockHandles[s] == -2) {
        gNetSockClose[s] = 1;
        NetSock_Unlock();
        return 0;
    }
    NetSock_Unlock();
    if (gNetSockHandles[s] < 0) {
        return -1;
    }
    NetSock_Lock();
    gNetSockClose[s] = 1;
    NetSock_Unlock();
    return 0;
}

s32 func_00308F30(void)
{
    return -1;
}

void NetSock_Lock(void);
void NetSock_Unlock(void);
extern s32 gNetErrno;
extern s32 gNetSockHandles[];

s32 net_socket(s32 domain, s32 type)
{
    s32 i;

    if (type != 1) {
        gNetErrno = 0x16;
        return -1;
    }
    NetSock_Lock();
    for (i = 0; i < 8; i++) {
        if (gNetSockHandles[i] == -1) {
            break;
        }
    }
    if (i == 8) {
        gNetErrno = 0xC;
        NetSock_Unlock();
        return -1;
    }
    gNetSockHandles[i] = -2;
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

/* connect(): opens a TCP connection for socket s (created by net_socket) and waits (up to 3000 x
 * 20 ms, or until net_close) until it is established; 0, or -1 with errno 9 (EBADF), 0x76
 * (EHOSTUNREACH), 0x71 or 0x68 (ECONNRESET). */
s32 net_connect(s32 s, SockAddr *addr, s32 len)
{
    s16 st;
    u16 b;
    s16 c;
    s32 sock;
    s16 i;

    st = 0;
    if (s < 0 || s >= 8) {
        gNetErrno = 9;
        return -1;
    }
    if (gNetSockHandles[s] != -2) {
        gNetErrno = 9;
        return -1;
    }
    sock = AveTcp_Open(addr->addr, addr->port, 0);
    if (sock < 0) {
        gNetErrno = 0x76;
        return -1;
    }
    NetSock_Lock();
    gNetSockHandles[s] = sock;
    NetSock_Unlock();
    i = 0;
    while (i++ < 3000) {
        NetSock_Lock();
        if (gNetSockClose[s] == 1) {
            gNetSockClose[s] = -1;
            gNetErrno = 0x71;
            NetSock_Unlock();
            return -1;
        }
        NetSock_Unlock();
        if (AveTcp_Status(gNetSockHandles[s], (u16 *)&st, &b, (u16 *)&c) < 0) {
            gNetErrno = 0x68;
            return -1;
        }
        if (st != 2 && st != 3) {
            break;
        }
        DelayThread(20000);
    }
    if (st != 4) {
        gNetErrno = 0x68;
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
    s16 id;
    s16 r;
    s16 k;
    HostSlot *h;

    NetSock_Lock();
    for (i = 0, h = gHostCache; i < 8; i++, h++) {
        if (strncmp(h->name, name, 0x100) == 0) {
            goto found;
        }
    }
    h = &gHostCache[gHostCacheNext];
    id = AveDns_Request(name);
    switch (id) {
    case -0x21:
    case -1:
        gNetHErrno = -1;
        goto fail;
    }
    NetSock_Unlock();
    for (;;) {
        NetSock_Lock();
        for (k = 0; k < 8; k++) {
            if (gNetSockClose[k] == 1) {
                gNetSockClose[k] = -1;
                AveDns_Free(id);
                gNetHErrno = 0x71;
                goto fail;
            }
        }
        r = AveDns_LookUp(id, &h->addr);
        switch (r) {
        case -6:
        case -1:
            AveDns_Free(id);
            gNetHErrno = 1;
            goto fail;
        case -2:
            break;
        }
        if (0 < r) {
            AveDns_Free(id);
            NetSock_Unlock();
            break;
        }
        NetSock_Unlock();
        DelayThread(20000);
    }
    NetSock_Lock();
    strncpy((char *)h->name, (char *)name, 0x100);
    gHostCacheNext++;
    gHostCacheNext %= 8;
found:
    h->ent.h_aliases = NULL;
    h->ent.h_addrtype = 2;
    h->ent.h_length = 4;
    h->addrList[0] = (s8 *)&h->addr;
    h->ent.h_addr_list = h->addrList;
    NetSock_Unlock();
    return (s32 *)&h->ent;
fail:
    NetSock_Unlock();
    return NULL;
}

s32 net_resolve(s8 *host)
{
    s32 *he;
    s32 ret;
    void *addr;

    ret = net_inet_addr(host);
    if (ret != 0) {
        return ret;
    }
    he = net_gethostbyname(host);
    if (he == NULL) {
        return -1;
    }
    addr = **(void ***)((u8 *)he + 0x10);
    return ((*(u8 *)((u8 *)addr + 3) << 0x18) & 0xFF000000) | (((*(u8 *)((u8 *)addr + 2) << 0x10) & 0xFF0000) | (*(u8 *)((u8 *)addr + 0) | ((*(u8 *)((u8 *)addr + 1) << 8) & 0xFF00)));
}

s16 AveTcp_Status(s16 sock, u16 *a, u16 *b, u16 *c);
s16 AveTcp_Recv(s16 sock, void *buf, s16 len);
s32 DelayThread(s32 usec);

/* recv(): waits for data (polling the status every 20 ms, while not closed by net_close) and
 * reads at most len bytes; the count, or -1 with errno 9 (EBADF), 0x71 or 0x68 (ECONNRESET). */
s32 net_recv(s32 s, void *buf, u32 len)
{
    s16 st;
    u16 b;
    s16 avail;
    s16 i;
    s16 r;

    st = 0;
    if (s < 0 || s >= 8) {
        gNetErrno = 9;
        return -1;
    }
    if (gNetSockHandles[s] < 0) {
        gNetErrno = 9;
        return -1;
    }
    for (;;) {
        NetSock_Lock();
        if (gNetSockClose[s] == 1) {
            gNetSockClose[s] = -1;
            gNetErrno = 0x71;
            NetSock_Unlock();
            return -1;
        }
        NetSock_Unlock();
        i = 0;
        while (i++ < 3000) {
            if (AveTcp_Status(gNetSockHandles[s], (u16 *)&st, &b, (u16 *)&avail) < 0) {
                gNetErrno = 0x68;
                return -1;
            }
            if (st != 2 && st != 3) {
                break;
            }
            DelayThread(20000);
        }
        if (st == 5) {
            if (avail == 0) {
                break;
            }
        } else if (st == -1) {
            if (avail == 0) {
                break;
            }
        } else if (st != 4) {
            gNetErrno = 0x68;
            return -1;
        }
        if (0 < avail) {
            break;
        }
        DelayThread(20000);
    }
    if (avail == 0) {
        r = 0;
    } else {
        if (avail < len) {
            len = avail;
        }
        r = AveTcp_Recv(gNetSockHandles[s], buf, len);
        if (r < 0) {
            gNetErrno = 0x68;
            return -1;
        }
        if ((st == 5 || st == -1) && r == len && r == avail) {
            gNetErrno = 0x68;
        }
    }
    return r;
}

s16 AveTcp_Status(s16 sock, u16 *a, u16 *b, u16 *c);
s16 AveTcp_Send(s16 sock, u8 *data, s16 len);
s32 DelayThread(s32 usec);

/* send(): waits (up to 3000 x 20 ms) while the socket is busy (status 2 or 3), then sends len
 * bytes; len, or -1 with errno 9 (EBADF) or 0x68 (ECONNRESET). */
s64 net_send(s32 s, void *data, u32 len)
{
    s16 st;
    u16 b;
    s16 c;
    s16 i;
    s32 *p;

    st = 0;
    if (s < 0 || s >= 8) {
        gNetErrno = 9;
        return -1;
    }
    p = &gNetSockHandles[s];
    if (*p < 0) {
        gNetErrno = 9;
        return -1;
    }
    i = 0;
    while (i++ < 3000) {
        if (AveTcp_Status(*p, (u16 *)&st, &b, (u16 *)&c) < 0) {
            gNetErrno = 0x68;
            return -1;
        }
        if (st != 2 && st != 3) {
            break;
        }
        DelayThread(20000);
    }
    if (AveTcp_Send(*p, data, len) < 0) {
        gNetErrno = 0x68;
        return -1;
    }
    return len;
}

s16 AveTcp_Abort(s16 sock);
s16 AveTcp_Delete(s16 sock);

/* close(): aborts and deletes the TCP socket of s (a socket not yet connected is just freed);
 * errno 9 (EBADF) for a bad or free socket. */
s32 net_close(s32 s)
{
    s32 *p;

    if (s < 0 || s >= 8) {
        gNetErrno = 9;
        return -1;
    }
    NetSock_Lock();
    p = &gNetSockHandles[s];
    if (*p == -2) {
        *p = -1;
        gNetSockClose[s] = -1;
        NetSock_Unlock();
        return 0;
    }
    NetSock_Unlock();
    if (*p < 0) {
        gNetErrno = 9;
        return -1;
    }
    NetSock_Lock();
    AveTcp_Abort(gNetSockHandles[s]);
    AveTcp_Delete(gNetSockHandles[s]);
    gNetSockHandles[s] = -1;
    gNetSockClose[s] = -1;
    NetSock_Unlock();
    return 0;
}

s32 func_00309A70(void)
{
    return 0;
}

s32 func_00309A80(void)
{
    return 0;
}

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
    SemaParam p;

    if (gNetSockSema < 0) {
        memset(&p, 0, sizeof(p));
        p.initCount = 1;
        p.maxCount = 1;
        gNetSockSema = CreateSema(&p);
    }
    if (gNetSockWaitSema < 0) {
        memset(&p, 0, sizeof(p));
        p.maxCount = 1;
        p.initCount = 0;
        gNetSockWaitSema = CreateSema(&p);
    }
    memset(gNetSockHandles, 0xFF, sizeof(gNetSockClose));
    memset(gNetSockClose, 0xFF, sizeof(gNetSockClose));
    gNetErrno = 0;
    gNetHErrno = 0;
}

/* NETWORK: as in net_tcp.c (NetPpp_GetStatus). */
typedef struct NetConnInfo {
    s32 unk0;
    s32 unk4;
    s32 speed;          /* 0x08: the link speed (bps) */
    s16 phase;          /* 0x0C: 4 = up */
    s16 unkE;
    s16 error;          /* 0x10: why it went down */
} NetConnInfo;
s32 NetPpp_GetStatus(NetConnInfo *out);
extern s16 gUsbKbReady;

/* The network device's state: its vendor and product ids, gUsbKbReady bit 0, and for devices 2 and 3 the
 * link speed / 100. */
void Net_GetDeviceState(s16 *a, s16 *b, s16 *c, s16 *d)
{
    NetConnInfo info;

    *a = gNetDevCur->vendorId;
    *b = gNetDevCur->productId;
    *c = gUsbKbReady & 1;
    *d = 0;
    switch (gNetDevCur->type) {
    case 2:
    case 3:
        NetPpp_GetStatus(&info);
        *d = info.speed / 100;
        break;
    }
}

s32 sceSifStopModule(s32 id, s32 argc, char *argv, s32 *result);   /* stop a module */
s32 sceSifUnloadModule(s32 id);                                       /* unload a module */

/* Stops and unloads IRX module id, retrying each step until it succeeds (as Yn_UnloadModule). */
void Irx_Unload(s32 id)
{
    s32 result;
    s32 ret;

    while ((ret = sceSifStopModule(id, 0, 0, &result)) < 0) {
        __asm__ volatile ("nop\n nop\n nop\n nop\n nop\n nop\n nop\n nop");
    }
    if (0 <= ret) {
        while (sceSifUnloadModule(id) < 0) {
            __asm__ volatile ("nop\n nop\n nop\n nop\n nop\n nop\n nop\n nop");
        }
    }
}

s32 sceSifLoadModule(char *path, s32 argc, char *argv);
s32 sceSifLoadStartModule(char *path, s32 argc, char *argv, void *buf);

/* Loads IRX module path (and starts it when buf is given), retrying forever or tries times
 * (as Yn_LoadModule: a 7-nop wait in the tries loops, a dead i = 0 on success). */
void Net_LoadIrx(s32 mod, s32 argc, char *argv, s32 buf, s32 tries)
{
    s32 i;

    if (buf == 0) {
        if (tries == 0) {
            while (sceSifLoadModule((char *)mod, argc, argv) < 0) {
                __asm__ volatile ("nop\n nop\n nop\n nop\n nop\n nop\n nop\n nop");
            }
        } else {
            for (i = 0; i < tries; i++) {
                if (0 <= sceSifLoadModule((char *)mod, argc, argv)) {
                    i = 0;
                    break;
                }
                __asm__ volatile ("nop\n nop\n nop\n nop\n nop\n nop\n nop");
            }
        }
    } else {
        if (tries == 0) {
            while (sceSifLoadStartModule((char *)mod, argc, argv, (void *)buf) < 0) {
                __asm__ volatile ("nop\n nop\n nop\n nop\n nop\n nop\n nop\n nop");
            }
        } else {
            for (i = 0; i < tries; i++) {
                if (0 <= sceSifLoadStartModule((char *)mod, argc, argv, (void *)buf)) {
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

s32 NetDev_Identify(NetDevice *dev, s32 vendor, s32 product, s32 unused, s32 arg4)
{
    NetDevice *ent;
    s32 vid;

    ent = gNetDevNames;
loop_1:
    vid = ent->vendorId;
    if (vid != 0) {
        if ((vid == vendor) && (ent->productId == product)) {
            dev->type = ent->type;
            dev->driver = ent->driver;
            dev->vendorId = vendor;
            dev->productId = product;
            dev->status = arg4;
            dev->vendor = ent->vendor;
            dev->product = ent->product;
            return 0;
        }
        ent++;
        goto loop_1;
    }
    memset(dev, 0xFF, sizeof(NetDevice));
    return -1;
}

void NetDev_ClearList(void)
{
    if (gNetDevCur == 0) {
        memset(&gNetDevs, 0xFF, 0x20U);
        gNetDevSeenCount = -1;
        gNetDevCount = -1;
    }
}

extern NetDevice D_00405420[];

/* Whether the names are the default device's (D_00405420[0], the network adaptor). */
s32 NetDev_MatchesDefault(s8 *vendor, s8 *product)
{
    if (vendor == 0 || product == 0) {
        return 0;
    }
    return strcmp(vendor, (s8 *)D_00405420[0].vendor) == 0 && strcmp(product, (s8 *)D_00405420[0].product) == 0;
}

s32 NetDev_IsKnownName(s8 *vendor, s8 *product)
{
    NetDevice *ent;

    if ((vendor == 0) || (product == 0)) {
        return 0;
    }
    ent = gNetDevNames;
loop_4:
    if (ent->vendorId != 0) {
        if ((strcmp(vendor, (s8 *)ent->vendor) == 0) && (strcmp(product, (s8 *)ent->product) == 0)) {
            return 1;
        }
        ent++;
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

INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_308BC0", D_0046A560);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_308BC0", D_0046A580);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_308BC0", D_0046A5A0);

/* Stops the network (gNetStopReq = 1): for a device of type 1 (not of kind 1), unloads module
 * D_0046A560 and the driver of its kind (6: D_0046A580, 5: D_0046A5A0), reloads gNetIrxUsbsrch,
 * rebinds the RPC server and forgets the device; -1 for devices of type 2, 3 or kind 1. */
s32 Net_StopDevice(void)
{
    s32 id;

    gNetStopReq = 1;
    switch (gNetDevCur->type) {
    case 2:
        return -1;
    case 1:
        switch (gNetDevCur->driver) {
        case 1:
            return -1;
        }
        if (0 <= (id = sceSifSearchModuleByName(D_0046A560))) {
            Irx_Unload(id);
        }
        id = -1;
        NetTcp_Term();
        switch (gNetDevCur->driver) {
        case 6:
            id = sceSifSearchModuleByName(D_0046A580);
            break;
        case 5:
            id = sceSifSearchModuleByName(D_0046A5A0);
            break;
        }
        if (0 <= id) {
            Irx_Unload(id);
        }
        Net_LoadIrx(gNetIrxUsbsrch, 0, 0, 0, 0);
        SifRpc_BindWait(&gNetDevRpc, 0x1270010);
        gNetDevCur = 0;
        NetDev_ClearList();
        NetDev_Refresh();
        break;
    case 3:
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

INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_308BC0", D_0046A5C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_308BC0", D_0046A5E0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_308BC0", D_0046A5E8);

/* Unloads module D_0046A5C0 if loaded; for a device of type 1, loads the driver of its kind (1,
 * 6 or 5) and gNetIrxAvewrape, then Net_Init. */
s32 Net_LoadDriverType1(void)
{
    s32 id;

    if (0 <= (id = sceSifSearchModuleByName(D_0046A5C0))) {
        Irx_Unload(id);
    }
    switch (gNetDevCur->type) {
    case 2:
        break;
    case 1:
        switch (gNetDevCur->driver) {
        case 1:
            Net_LoadIrx(gNetIrxAn986, 8, D_0046A5E0, 0, 0);
            break;
        case 6:
            Net_LoadIrx(gNetIrxLanegg, 8, D_0046A5E0, 0, 0);
            break;
        case 5:
            Net_LoadIrx(gNetIrxSmap, 9, D_0046A5E8, 0, 0);
            break;
        }
        Net_LoadIrx(gNetIrxAvewrape, 0, 0, 0, 0);
        Net_Init();
        break;
    case 3:
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

INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_308BC0", D_0046A5F8);

/* Loads the drivers of the device in use (gNetStopReq must be clear): after unloading module
 * D_0046A5C0, for type 2 the module of its kind (4: gNetIrxOstmdmA, 2: gNetIrxCxtmdm) and the common
 * ones (gNetDriverType = 2), for type 1 gNetIrxDevglue, gNetIrxAvedhcp and gNetIrxAveppp (gNetDriverType = 1);
 * always 1. */
s32 Net_LoadDrivers(void)
{
    s32 id;

    if (gNetStopReq != 0) {
        return 1;
    }
    if (0 <= (id = sceSifSearchModuleByName(D_0046A5C0))) {
        Irx_Unload(id);
    }
    switch (gNetDevCur->type) {
    case 2:
        switch (gNetDevCur->driver) {
        case 2:
            Net_LoadIrx(gNetIrxCxtmdm, 0, 0, 0, 0);
            break;
        case 4:
            Net_LoadIrx(gNetIrxOstmdmA, 0, 0, 0, 0);
            break;
        }
        Net_LoadIrx(gNetIrxAveppp, 1, D_0046A5F8, 0, 0);
        Net_LoadIrx(gNetIrxAvewrapm, 0, 0, 0, 0);
        Net_Init();
        gNetDriverType = 2;
        break;
    case 1:
        Net_LoadIrx(gNetIrxDevglue, 1, D_0046A5F8, 0, 0);
        Net_LoadIrx(gNetIrxAvedhcp, 0, 0, 0, 0);
        Net_LoadIrx(gNetIrxAveppp, 1, D_0046A5F8, 0, 0);
        gNetDriverType = 1;
        break;
    case 3:
        break;
    }
    return 1;
}

extern s32 D_0059CBC4;
extern s32 D_0059CBC8;
extern s32 D_0059CBCC;

/*
 * Refreshes the device list from the IOP when its count changed: for each of the 32 slots,
 * asks for the device (RPC 1) and fills gNetDevs[i] (NetDev_Identify) or clears it; then
 * counts the three extra entries into gNetDevCount. Returns 1 when refreshed.
 * The game keeps the addresses of the reply words (D_0059CBC4..CC) in registers, which
 * only the pointer locals give; count is a u32 (as an s32 the saved registers rotate) and
 * i = 0 comes first.
 */
s32 NetDev_Refresh(void)
{
    u32 count;
    s32 i;
    NetDevice *dev;
    s32 *pc;
    s32 *pb;
    s32 *pa;

    if (gNetDevCur != 0) {
        return 0;
    }
    NetDev_CallRpc(0);
    count = gNetDevRpcBuf;
    if (count != gNetDevSeenCount) {
        i = 0;
        dev = gNetDevs;
        pc = &D_0059CBCC;
        pb = &D_0059CBC8;
        pa = &D_0059CBC4;
        for (; i < 32; i++, dev++) {
            gNetDevRpcBuf = i;
            NetDev_CallRpc(1);
            if (gNetDevRpcBuf == -1) {
                memset(dev, 0xFF, sizeof(NetDevice));
            } else {
                NetDev_Identify(dev, gNetDevRpcBuf, *pa, *pb, *pc);
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
void SifRpc_BindWait(SifClientData *cd, u32 id)
{
    volatile s32 i;

    for (;;) {
        if (sceSifBindRpc(cd, id, 0) >= 0 && cd->serve != NULL) {
            break;
        }
        for (i = 0x10000; i != 0; i--) {
        }
    }
}

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

INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_308BC0", D_0046A600);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_308BC0", D_0046A610);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_308BC0", D_0046A640);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_308BC0", D_0046A670);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_308BC0", D_0046A6A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_308BC0", D_0046A6D0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_308BC0", D_0046A700);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_308BC0", D_0046A730);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_308BC0", D_0046A760);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_308BC0", D_0046A790);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_308BC0", D_0046A7C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_308BC0", D_0046A7F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_308BC0", D_0046A820);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_308BC0", D_0046A848);

/* Starts the network: loads module gNetIrxAvetcp, copies the default device D_00405420[0] into
 * gNetDevs[34] (status 0x21) when module D_0046A600 is already loaded, loads gNetIrxUsbsrch, binds the RPC server 0x1270010. */
s32 Net_Start(void)
{
    NetCfg_Reset();
    NetDev_ClearList();
    gNetStopReq = 0;
    D_0059CC30 = 0;
    Net_LoadIrx(gNetIrxAvetcp, 1, D_0046A5F8, 0, 0);
    if (0 <= sceSifSearchModuleByName(D_0046A600)) {
        gNetDevs[34].type = D_00405420[0].type;
        gNetDevs[34].driver = D_00405420[0].driver;
        gNetDevs[34].vendorId = D_00405420[0].vendorId;
        gNetDevs[34].productId = D_00405420[0].productId;
        gNetDevs[34].status = 0x21;
        gNetDevs[34].vendor = D_00405420[0].vendor;
        gNetDevs[34].product = D_00405420[0].product;
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
static s32 Str_FormatDigits(char *out, s32 v, s32 n)
{
    s32 i;

    out += n - 1;
    for (i = 0; i < n; i++) {
        *out-- = v % 10 + '0';
        v /= 10;
    }
    return 0;
}

/* The number in the first n characters (digits only; others are skipped). */
static s32 Str_ParseDigits(s8 *s, s32 n)
{
    s32 v = 0;
    s32 i;

    for (i = 0; i < n; i++, s++) {
        if (*s >= '0' && *s <= '9') {
            v *= 10;
            v += *s - '0';
        }
    }
    return v;
}

/* The reply to a lobby challenge: out = the two 4-digit halves of ch plus key, as two 5-digit
 * numbers (10 characters); -1 without a buffer or a challenge. */
s32 Net_ChallengeReply(char *out, s8 *ch, u16 key)
{
    s32 a;
    s32 b;

    if (out == NULL || ch == NULL) {
        return -1;
    }
    a = key + Str_ParseDigits(ch, 4);
    b = key + Str_ParseDigits(ch + 4, 4);
    Str_FormatDigits(out, a, 5);
    Str_FormatDigits(out + 5, b, 5);
    out[10] = 0;
    return 0;
}

/* A copy of net_inet_addr (inet_addr): the address in dotted form (in network order), 0 when str is not one (a run of
 * more than 3 digits, 4 dots or another character). */
s32 net_inet_addr2(s8 *str)
{
    s32 dots = 0;
    s32 run = 0;
    s8 *p;
    u32 i;
    s32 c0;
    u8 a;
    u8 b;
    u8 c;
    u8 d;

    for (p = str; (c0 = *p); p++) {
        if (c0 >= '0' && c0 <= '9') {
            if (++run == 4) {
                return 0;
            }
        } else if (c0 == '.') {
            if (++dots == 4) {
                return 0;
            }
            run = 0;
        } else {
            return 0;
        }
    }
    i = 0;
    a = atoi(str);
    for (; i < strlen(str) && str[i] != '.'; i++) {
    }
    if (i < strlen(str)) {
        i++;
    }
    b = atoi(str + i);
    for (; i < strlen(str) && str[i] != '.'; i++) {
    }
    if (i < strlen(str)) {
        i++;
    }
    c = atoi(str + i);
    for (; i < strlen(str) && str[i] != '.'; i++) {
    }
    if (i < strlen(str)) {
        i++;
    }
    d = atoi(str + i);
    return ((d << 24) & 0xFF000000) | (((c << 16) & 0xFF0000) | ((a & 0xFF) | ((b << 8) & 0xFF00)));
}

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
s32 Net_ResolveHost(s8 *st, s16 *timer, s32 *out, s8 *name)
{
    s32 addr;
    s32 i;
    HostEntry *e;

    switch (*st) {
    case 0:
        *out = net_inet_addr2(name);
        if (*out != 0) {
            *st = 0;
            *timer = 0;
            return 1;
        }
        for (i = 0, e = gNetHostCache; i < 8; i++, e++) {
            if (strncmp(e->name, name, 0x100) == 0) {
                *st = 0;
                *timer = 0;
                *out = e->addr;
                return 1;
            }
        }
        gNetDnsReq = NetDns_Request(name);
        if (gNetDnsReq == -1) {
            return -1;
        }
        (*st)++;
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
            *st = 0;
            *timer = 0;
            return -1;
        case 1:
            NetDns_Free(gNetDnsReq);
            *st = 0;
            *timer = 0;
            *out = addr;
            e = &gNetHostCache[gNetHostCacheNext];
            strncpy((char *)e->name, (char *)name, 0x100);
            e->addr = addr;
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
    s32 buf[8];
    s32 x;
    s32 y;
    s32 a = 0;
    s32 b = 0;

    if (gNetDevCur == 0) {
        return;
    }
    if (gNetDns1 == 0 || gNetDns2 == 0) {
        switch (gNetDevCur->type) {
        case 2:
            NetPpp_GetInfo(buf);
            a = buf[2];
            b = buf[3];
            break;
        case 1:
            switch (gNetLanMode) {
            case 2:
                NetPpp_GetInfo(buf);
                a = buf[2];
                b = buf[3];
                break;
            case 0:
                break;
            case 1:
                NetDhcp_GetDns((char *)&gNetDomain, gNetDhcpBuf);
                a = gNetDhcpBuf[0];
                b = gNetDhcpBuf[1];
                break;
            }
            break;
        }
    }
    x = gNetDns1 ? gNetDns1 : a;
    y = gNetDns2 ? gNetDns2 : b;
    NetDns_Init((char *)&gNetDomain, x, y);
    Net_ClearHostCache();
}

void Net_ClearState(void)
{
    memset(&gNetMsgState, 0, 0x12U);
    memset(&gNetPppStatus, 0, 0x14U);
    memset(&gNetTcpStatus, 0, 8U);
    gNetSock = -1;
}

s32 NetMsg_Dispatch(s8 *a, s8 *b, s8 *c, s16 *d, u16 *e, u16 *f, u16 *g, s8 *h);

s32 NetMsg_Update(void)
{
    return NetMsg_Dispatch((s8 *)&gNetMsgState + 1, (s8 *)&gNetMsgState + 2, (s8 *)&gNetMsgState + 3, (s16 *)(&gNetMsgState + 4),
                  (u16 *)(&gNetMsgState + 6), (u16 *)(&gNetMsgState + 8), (u16 *)(&gNetMsgState + 0xA),
                  (s8 *)&gNetMsgState + 0xE);
}

s32 NetIf_GetState(void);
s32 NetIf_PollEvent_Unk(s32 *out);
s32 NetIf_GetEvent_Unk(s32 a, s32 *b);
s32 NetIf_AckEvent_Unk(s32 a);

s32 NetDev_CheckLink(void)
{
    s32 ev[1];
    s32 status[1];
    s32 r;

    if (gNetDevCur->type != 1) {
        return 0;
    }
    r = NetIf_PollEvent_Unk((s32 *) ev);
    switch (r) {
    case 0:
        return 0;
    case 1:
        if ((NetIf_GetEvent_Unk(0, (s32 *) status) > 0) && (NetIf_GetState() == 0) && (NetIf_AckEvent_Unk(status[0]) == 0)) {
            gNetDevCur->status = status[0];
            return 0;
        }
        gNetDevCur->status = -1;
    default:
        return -1;
    }
}

/* the dial-up (PPP) settings Net_ConnectPppStep fills for NetPpp_Connect (0x59D468) */
typedef struct NetPppConf {
    /* 0x00 */ s16 flag;            /* NetPpp_TestFlag_Unk toggles it */
    /* 0x02 */ s16 authKind;        /* 0: authText is the default (D_0046A888), 1: D_0059DBE0 */
    /* 0x04 */ char *authText;
    /* 0x08 */ s16 opt[5];          /* optP's: 4, 0, 0, 1, 1 */
    /* 0x12 */ u8 pad12[6];
    /* 0x18 */ s16 *optP;           /* &opt */
    /* 0x1C */ s16 *flagP;          /* &flag */
    /* 0x20 */ s16 *addrP;          /* &D_0059D4B0 */
    /* 0x24 */ char *chat0;         /* NetModem_GetChat's two strings */
    /* 0x28 */ char *chat1;
    /* 0x2C */ void *p494;          /* &D_0059DBF0 */
    /* 0x30 */ void *authName;      /* &gNetAuthName */
    /* 0x34 */ void *authKey;       /* &gNetAuthKey */
    /* 0x38 */ void *args;          /* &optP: what NetPpp_Connect gets */
    /* 0x3C */ s16 unk3C;
    /* 0x3E */ s8 lanMode2;         /* gNetLanMode == 2 */
    /* 0x3F */ u8 phyMode;          /* gNetPhyMode */
} NetPppConf;
extern NetPppConf gNetPppConf;

s32 NetPpp_TestFlag_Unk(void)
{
    if (D_0059DCE0 == 1) {
        if (gNetPppConf.flag == 1) {
            gNetPppConf.flag = 0;
            return 0;
        }
        goto block_6;
    }
    if (gNetPppConf.flag == 0) {
        gNetPppConf.flag = 1;
        return 0;
    }
block_6:
    return -1;
}

s32 NetPpp_LatchFlag_Unk(void)
{
    D_0059DCE0 = (u8) gNetPppConf.flag;
    return 0;
}

s32 NetPpp_StepState_Unk(void)
{
    switch (D_0059D4B0) {
    case 1:
        D_0059D4B0 = 3;
        break;
    case 3:
        D_0059D4B0 = 2;
        break;
    case 2:
        D_0059D4B0 = 0;
        break;
    case 0:
        D_0059D4B0 = 1;
        break;
    }
    return -(D_0059D8C0 == D_0059D4B0);
}

void Net_DebugLog(s32 a, s32 b, s32 c, char *fmt, ...);
typedef s32 (*NetMsgFn)(s8 *kind, s8 *b, s8 *c, s16 *d, u16 *e, u16 *f, u16 *g, s8 *h);
extern NetMsgFn gNetMsgHandlers[];
extern char D_0046A860[];

INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_308BC0", D_0046A860);

/* Logs a network message (Net_DebugLog, compiled out) and runs its handler gNetMsgHandlers[*kind], returning its value. */
s32 NetMsg_Dispatch(s8 *kind, s8 *b, s8 *c, s16 *d, u16 *e, u16 *f, u16 *g, s8 *h)
{
    Net_DebugLog(1, 0x1B, 1, D_0046A860, *d, *kind, *b, *c, *h);
    return gNetMsgHandlers[*kind](kind, b, c, d, e, f, g, h);
}

typedef struct NetProgress {
    u8 cur;
    u8 max;
} NetProgress;
extern NetProgress gNetProgress;
extern s32 D_0059D61C;
extern s32 D_0059D620;
/* unit's own declaration: with the definition's s16 (s32 mode, s32 kind) (sub_main/net_tcp.c), Net_ConnectLanStep: 668 of 783 words differ, Net_ConnectPppStep: 478 of 693 words differ */
s32 NetIf_Up(s32 mode, s32 kind);
s32 NetEth_Init(void);
s32 NetEth_SetAddress(s32 *p);
s32 NetEth_Start(s32 a);
s32 NetEth_IsLinkUp(void);
s32 NetEth_SetConnected(s32 unused0);
s32 NetEth_GetOpt8_Unk(void *buf);
s32 NetRoute_Add(s32 a, s32 b);
s32 NetDhcp_Init(void);
s32 NetDhcp_Term(void);
s32 NetDhcp_Start(s32 a, s32 b);
s32 NetDhcp_GetState(void);
s32 NetDhcp_GetAddress(s32 *p);
s32 NetDhcp_GetRouter(s32 *out);

#define NET_RESET(e)  \
    *err = (e);       \
    *st0 = 2;         \
    *st = 0;          \
    *sub = 0;         \
    *t = 0;           \
    *c1 = 0;          \
    *c2 = 0;          \
    *c3 = 0

INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_308BC0", D_0046A888);

extern char D_0046A888[];
extern s16 gNetPppPhase;
extern s16 gNetPppError;
s32 NetPpp_Init(void);
s16 NetPpp_SetOption1(void *src);
s32 NetModem_GetChat(s32 mode, s32 kind, s32 a2, s32 a3, char **out);
s32 NetPpp_GetLinkState_Unk(void);
struct NetDialArg;
s32 NetPpp_Connect(struct NetDialArg *arg);
s32 NetPpp_SetConnected(s32 unused0);

/* NETWORK: the dial-up connect steps (modem/PPP, NetPpp_Connect with the settings at gNetPppConf.args),
 * like Net_ConnectLanStep: failures through NET_RESET, 2 when connected, 5 after a reconnect. */
s32 Net_ConnectPppStep(s8 *st0, s8 *st, s8 *sub, s16 *t, s16 *c1, s16 *c2, s16 *c3, s8 *err)
{
    char *buf[2];
    s32 x;

    if (NetDev_CheckLink() < 0) {
        NET_RESET(-0x51);
        return 0;
    }
    switch (*st) {
    case 0:
        if (gNetDevCur == 0) {
            *err = -0x51;
            return 4;
        }
        gNetProgress.max = 10;
        gNetProgress.cur = 0;
        *sub = 0;
        *t = 0;
        *c1 = 0;
        *c2 = 3;
        *c3 = 2;
        if (gNetDevCur->type == 1) {
            switch (gNetLanMode) {
            case 2:
                *st = 10;
                break;
            case 1:
            case 0:
                *st0 = 1;
                break;
            }
        } else {
            *st = 10;
        }
        return 0;
    case 10:
        NetModem_GetChat(gNetDevCur->type, gNetDevCur->driver, 0, 2, buf);
        *st = 1;
        gNetPppConf.args = &gNetPppConf.optP;
        gNetPppConf.optP = &gNetPppConf.opt[0];
        gNetPppConf.flagP = &gNetPppConf.flag;
        gNetPppConf.addrP = &D_0059D4B0;
        gNetPppConf.p494 = &D_0059DBF0;
        gNetPppConf.authName = &gNetAuthName;
        gNetPppConf.authKey = &gNetAuthKey;
        gNetPppConf.opt[0] = 4;
        gNetPppConf.unk3C = 0;
        gNetPppConf.opt[1] = 0;
        gNetPppConf.opt[2] = 0;
        gNetPppConf.opt[3] = 1;
        gNetPppConf.opt[4] = 1;
        gNetPppConf.lanMode2 = gNetLanMode == 2;
        gNetPppConf.phyMode = gNetPhyMode;
        gNetPppConf.chat0 = buf[0];
        gNetPppConf.chat1 = buf[1];
        gNetPppConf.flag = D_0059DCE0;
        if (D_0059DBE0 == 0) {
            gNetPppConf.authKind = 0;
            gNetPppConf.authText = D_0046A888;
        } else {
            gNetPppConf.authKind = 1;
            gNetPppConf.authText = (char *)&D_0059DBE0;
        }
        memset(&D_0059D4B0, 0, 0x10);
        D_0059D4B0 = D_0059D8C0;
        return 0;
    case 1:
        if (0 < (*t)--) {
            return 0;
        }
        *t = 0;
        gNetProgress.cur = 2;
        if (NetIf_Up(gNetDevCur->type, gNetDevCur->driver) < 0) {
            NET_RESET(-0x52);
            return 0;
        }
        (*st)++;
        return 0;
    case 2:
        gNetProgress.cur = 4;
        if (NetPpp_Init() < 0) {
            NET_RESET(-0x52);
            return 0;
        }
        x = 10;
        NetPpp_SetOption1(&x);
        (*st)++;
        *t = 0;
        return 0;
    case 3:
        gNetProgress.cur = 6;
        if (gNetDevCur->type == 2 || gNetDevCur->type == 3) {
            gNetDevCur->status = NetPpp_GetLinkState_Unk();
            if (gNetDevCur->status < 0) {
                NET_RESET(-0x51);
                return 0;
            }
        }
        if (NetPpp_Connect((struct NetDialArg *)&gNetPppConf.args) < 0) {
            NET_RESET(-0x52);
            return 0;
        }
        memset(&gNetPppStatus, 0, 0x14);
        *st += 2;
        *t = 0;
        return 0;
    case 4:
        (*st)++;
        return 0;
    case 5:
        if (*c1 > 30) {
            NET_RESET(-0x56);
            return 0;
        }
        if (*t % 30 == 0) {
            gNetProgress.cur = (gNetProgress.cur < gNetProgress.max) ? gNetProgress.cur + 1 : gNetProgress.max;
        }
        (*t)++;
        if (NetPpp_GetStatus((NetConnInfo *)&gNetPppStatus) < 0) {
            NET_RESET(-0x52);
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
        case 4:
            gNetProgress.max = 80;
            if (gNetProgress.cur < 70) {
                gNetProgress.cur = 70;
            }
            (*st)++;
            break;
        case 0:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
            switch (gNetPppError) {
            case 2:
                if (NetPpp_TestFlag_Unk() == 0) {
                    *st = 0x63;
                } else {
                    NET_RESET(-3);
                }
                break;
            case 12:
                if (--(*c3) == 0) {
                    NET_RESET(-2);
                } else {
                    *st = 0x63;
                }
                break;
            case 1:
                if (--(*c2) == 0) {
                    NET_RESET(-0x52);
                } else {
                    *st = 0x63;
                }
                break;
            case 5:
                if (NetPpp_StepState_Unk() == 0) {
                    *st = 0x63;
                } else {
                    NET_RESET(-0x52);
                }
                break;
            case 3:
                NET_RESET(-5);
                break;
            case 6:
                NET_RESET(-0x55);
                break;
            case 13:
                NET_RESET(-2);
                break;
            case 10:
                NET_RESET(-0x57);
                break;
            default:
                NET_RESET(-0x52);
                break;
            }
            return 0;
        default:
            NET_RESET(-0x52);
            break;
        }
        return 0;
    case 6:
        gNetProgress.max = 100;
        gNetProgress.cur = 100;
        (*st)++;
        NetPpp_LatchFlag_Unk();
        Net_InitDns();
        NetPpp_SetConnected(0);
        gNetIpAddr = gNetPppStatus;
        return 0;
    case 7:
        *st0 = 0;
        *st = 0;
        *t = 0;
        *c1 = 0;
        *c2 = 0;
        *c3 = 0;
        return 2;
    case 0x63:
        (*st)++;
        *sub = 0;
        *t = 0;
        return 0;
    case 0x64:
        if (Net_DisconnectStep(sub, t) != 0) {
            gNetProgress.cur = 0;
            gNetProgress.max = 10;
            *st = 1;
            *sub = 0;
            *t = 30;
            *c1 = 0;
            return 5;
        }
        return 0;
    default:
        return 0;
    }
}

/* NETWORK: the connect steps (state *st; gNetProgress the progress shown, cur rising to max): on a
 * failure NET_RESET stores the error code in *err and sets *st0 to 2; 2 when connected, 5 after a
 * reconnect (states 0x63, 0x64 run the disconnect steps Net_DisconnectStep first). */
s32 Net_ConnectLanStep(s8 *st0, s8 *st, s8 *sub, s16 *t, s16 *c1, s16 *c2, s16 *c3, s8 *err)
{
    s32 r;

    if (NetDev_CheckLink() < 0) {
        NET_RESET(-0x51);
        return 0;
    }
    switch (*st) {
    case 0:
        (*st)++;
        *sub = 0;
        *t = 0;
        *c1 = 0;
        *c2 = 0;
        *c3 = 0;
        return 0;
    case 1:
        if (0 < (*t)--) {
            return 0;
        }
        *t = 0;
        gNetProgress.max = 10;
        gNetProgress.cur = 0;
        (*st)++;
        return 0;
    case 2:
        gNetProgress.cur = 2;
        if (NetIf_Up(gNetDevCur->type, gNetDevCur->driver) < 0) {
            NET_RESET(-0x52);
            return 0;
        }
        (*st)++;
        return 0;
    case 3:
        gNetProgress.cur = 6;
        if (NetEth_Init() < 0) {
            NET_RESET(-0x52);
            return 0;
        }
        switch (gNetLanMode) {
        case 1:
            *st = 20;
            break;
        case 0:
            *st = 10;
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
            NET_RESET(-0x52);
            return 0;
        }
        (*st)++;
        return 0;
    case 11:
        if (NetEth_Start(gNetPhyMode) < 0) {
            NET_RESET(-0x52);
            return 0;
        }
        (*st)++;
        *t = 0;
        return 0;
    case 12:
        if (*c1 > 30) {
            NET_RESET(-0x56);
            return 0;
        }
        if (*t % 30 == 0) {
            gNetProgress.cur = (gNetProgress.cur < gNetProgress.max) ? gNetProgress.cur + 1 : gNetProgress.max;
        }
        (*t)++;
        if (NetEth_IsLinkUp() != 0) {
            gNetProgress.max = 70;
            if (gNetProgress.cur < 20) {
                gNetProgress.cur = 20;
            }
            (*st)++;
            *t = 0;
            *c2 = 0;
            return 0;
        }
        if (*t > 450) {
            if (*c2 >= 3) {
                NET_RESET(-0x58);
            } else {
                *st = 0x63;
                (*c2)++;
            }
        }
        return 0;
    case 13:
        if (*c1 > 30) {
            NET_RESET(-0x56);
            return 0;
        }
        *t = 0;
        gNetProgress.max = 100;
        if (gNetProgress.cur < 70) {
            gNetProgress.cur = 70;
        }
        Net_InitDns();
        if (NetRoute_Add(0, gNetGateway) < 0) {
            NET_RESET(-0x52);
            return 0;
        }
        *st = 30;
        return 0;
    case 20:
        gNetProgress.max = 20;
        if (gNetProgress.cur < 11) {
            gNetProgress.cur = 11;
        }
        if (NetEth_Start(gNetPhyMode) < 0) {
            NET_RESET(-0x52);
            return 0;
        }
        (*st)++;
        *t = 0;
        return 0;
    case 21:
        if (*c1 > 30) {
            NET_RESET(-0x56);
            return 0;
        }
        if (*t % 30 == 0) {
            gNetProgress.cur = (gNetProgress.cur < gNetProgress.max) ? gNetProgress.cur + 1 : gNetProgress.max;
        }
        (*t)++;
        if (NetEth_IsLinkUp() != 0) {
            gNetProgress.max = 60;
            if (gNetProgress.cur < 20) {
                gNetProgress.cur = 20;
            }
            (*st)++;
            *t = 0;
            return 0;
        }
        if (*t > 450) {
            if (*c2 >= 3) {
                NET_RESET(-0x58);
            } else {
                *st = 0x63;
                (*c2)++;
            }
        }
        return 0;
    case 22:
        if (NetDhcp_Init() < 0) {
            NET_RESET(-0x52);
            return 0;
        }
        if (NetDhcp_Start(0, 5) < 0) {
            NET_RESET(-0x52);
            return 0;
        }
        (*st)++;
        *t = 0;
        return 0;
    case 23:
        if (*c1 > 30) {
            NetDhcp_Term();
            NET_RESET(-0x56);
            return 0;
        }
        if (*t % 30 == 0) {
            gNetProgress.cur = (gNetProgress.cur < gNetProgress.max) ? gNetProgress.cur + 1 : gNetProgress.max;
        }
        (*t)++;
        r = NetDhcp_GetState();
        if (r < 0) {
            if (*c2 >= 3) {
                NetDhcp_Term();
                if (gNetProgress.cur == 60) {
                    NET_RESET(-0x59);
                } else {
                    NET_RESET(-0x54);
                }
            } else {
                NetDhcp_Term();
                *st = 0x63;
                (*c2)++;
            }
            return 0;
        }
        switch (r) {
        case 3:
            gNetProgress.max = 70;
            if (gNetProgress.cur < 60) {
                gNetProgress.cur = 60;
            }
            NetDhcp_GetAddress(gNetDhcpBuf);
            gNetIpAddr = gNetDhcpBuf[0];
            gNetNetmask = D_0059D61C;
            NetEth_SetAddress(gNetDhcpBuf);
            (*st)++;
            *t = 0;
            break;
        }
        return 0;
    case 24:
        *t = 0;
        gNetProgress.max = 100;
        if (gNetProgress.cur < 70) {
            gNetProgress.cur = 70;
        }
        Net_InitDns();
        if (NetDhcp_GetRouter(gNetDhcpBuf) < 0) {
            NET_RESET(-0x52);
            return 0;
        }
        gNetGateway = gNetDhcpBuf[0];
        if (NetRoute_Add(0, gNetGateway) < 0) {
            NET_RESET(-0x52);
            return 0;
        }
        *st = 30;
        return 0;
    case 30:
        NetEth_SetConnected(0);
        D_0059D620 = 0;
        if (NetEth_GetOpt8_Unk(&D_0059D620) < 0 || D_0059D620 == 0) {
            NET_RESET(-0x53);
            return 0;
        }
        gNetProgress.max = 100;
        gNetProgress.cur = 100;
        (*st)++;
        *sub = 0;
        *t = 0;
        *c1 = 0;
        *c2 = 0;
        *c3 = 0;
        return 0;
    case 31:
        *st0 = 0;
        *st = 0;
        *t = 0;
        *c1 = 0;
        *c2 = 0;
        *c3 = 0;
        return 2;
    case 0x63:
        (*st)++;
        *sub = 0;
        *t = 0;
        return 0;
    case 0x64:
        if (Net_DisconnectStep(sub, t) != 0) {
            gNetProgress.cur = 0;
            gNetProgress.max = 10;
            *st = 1;
            *sub = 0;
            *t = 30;
            *c1 = 0;
            return 5;
        }
        return 0;
    default:
        return 0;
    }
}

s32 Net_Disconnect(s8 *arg0, s8 *step, s8 *st, s16 *t, s16 *arg4, s16 *arg5, s16 *arg6, s32 arg7)
{
    s8 cur;

    cur = *step;
    switch (cur) {
    case 0:
        *step = cur + 1;
        *st = 0;
        *t = 0;
        return 0;
    case 1:
        if (Net_DisconnectStep(st, t) != 0) {
            *arg0 = 0;
            *step = 0;
            *st = 0;
            *t = 0;
            *arg4 = 0;
            *arg5 = 0;
            *arg6 = 0;
            return 4;
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

INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_308BC0", D_0046A8C0);

/* NETWORK: the disconnect steps (state *st, timer *t): closes the socket (gNetSock), stops the
 * device by its type and returns 1 when done. */
s32 Net_DisconnectStep(s8 *st, s16 *t)
{
    s16 n;
    s32 r;

    Net_DebugLog(1, 0x1A, 1, D_0046A8C0);
    switch (*st) {
    case 0:
        if (NetIf_GetState() == 0) {
            *st = 20;
            *t = 0;
        } else {
            if (NetIf_GetState() == 3) {
                (*st)++;
                *t = 0;
                return 0;
            }
            *st = 2;
            *t = 0;
        }
        return 0;
    case 1:
        NetTcp_Abort(gNetSock);
        NetTcp_Delete(&gNetSock);
        (*st)++;
        *t = 0;
        return 0;
    case 2:
        if (0 < (*t)--) {
            return 0;
        }
        *t = 0;
        if (NetIf_GetState() == 2) {
            *st = 6;
        } else {
            (*st)++;
        }
        return 0;
    case 3:
        NetDns_Term();
        if (gNetDevCur->type == 1) {
            switch (gNetLanMode) {
            case 0:
                NetRoute_Delete(0, gNetGateway);
                NetEth_ClearAddress(0);
                *st = 6;
                break;
            case 1:
                NetRoute_Delete(0, gNetGateway);
                NetEth_ClearAddress(0);
                (*st)++;
                break;
            case 2:
                NetPpp_SetClosed(0);
                *st = 6;
                break;
            }
        } else {
            NetPpp_SetClosed(0);
            *st = 6;
        }
        return 0;
    case 4:
        D_0059D61C = 0;
        gNetDhcpBuf[0] = gNetIpAddr;
        if (NetDhcp_Release(gNetDhcpBuf, 5) != 0) {
            NetDhcp_Term();
            *st += 2;
        } else {
            (*st)++;
            *t = 300;
        }
        return 0;
    case 5:
        switch (NetDhcp_GetState()) {
        default:
            if (0 < (*t)--) {
                break;
            }
            *t = 0;
        case 0:
        case -2:
        case -1:
            NetDhcp_Term();
            (*st)++;
            *t = 0;
            break;
        }
        return 0;
    case 6:
        switch (gNetDevCur->type) {
        case 2:
        case 3:
            NetPpp_Disconnect(0);
            *t = 300;
            (*st)++;
            break;
        case 1:
            switch (gNetLanMode) {
            case 2:
                NetPpp_Disconnect(0);
                *t = 300;
                (*st)++;
                break;
            case 1:
            case 0:
                NetEth_Stop();
                *t = 30;
                *st += 2;
                break;
            }
            break;
        }
        return 0;
    case 7:
        (*t)--;
        n = *t;
        if (n <= -1) {
            (*st)++;
            *t = 0;
        } else if (n % 30 == 0) {
            r = NetPpp_GetStatus((NetConnInfo *)&gNetPppStatus);
            if (gNetPppPhase == 0 || r < 0) {
                (*st)++;
                *t = 0;
            }
        }
        return 0;
    case 8:
        if (0 < (*t)--) {
            return 0;
        }
        *t = 0;
        switch (gNetDevCur->type) {
        case 2:
        case 3:
            NetPpp_Term();
            break;
        case 1:
            switch (gNetLanMode) {
            case 2:
                NetPpp_Term();
                break;
            case 1:
            case 0:
                NetEth_Term();
                break;
            }
            break;
        }
        (*st)++;
        *t = 20;
        return 0;
    case 9:
        if (0 < (*t)--) {
            return 0;
        }
        *t = 0;
        NetIf_Down();
        (*st)++;
        *t = 20;
        return 0;
    case 10:
        if (0 < (*t)--) {
            return 0;
        }
        *t = 0;
        *st = 20;
        *t = 0;
        return 0;
    case 20:
        if (0 < (*t)--) {
            return 0;
        }
        *t = 0;
        *st = 0;
        *t = 0;
        return 1;
    default:
        return 0;
    }
}

s32 NetXfer_GetState_Unk(void)
{
    return gNetXferResult;
}

extern s32 gNetXferSize;
s32 func_AE2518(void);
s32 func_AE8638(s32 a, s32 b, s32 *out);
s32 func_AE8688(s32 a, s32 b);
void func_AE2570(void);

/* A step of an asynchronous transfer (state *st: 0 start, 10 open, 11 run, 15 close, 99 failed,
 * 100 done): returns the size (gNetXferSize) when done, -1 when failed (more than max), 0 while
 * running. */
s32 NetXfer_Step(s8 *st, s16 *unused, u8 *b, s32 a, s32 max)
{
    switch (*st) {
    case 0:
        *st = 10;
        gNetXferSize = 0;
        gNetXferResult = func_AE2518();
        if (gNetXferResult < 0) {
            *st = 0x63;
        }
        break;
    case 10:
        (*st)++;
        gNetXferResult = func_AE8638(a, (s32) b, &gNetXferSize);
        if (gNetXferResult < 0) {
            *st = 0x63;
        } else if (gNetXferSize > max) {
            *st = 0x63;
        }
        break;
    case 11:
        *st = 15;
        gNetXferResult = func_AE8688(a, gNetXferSize);
        if (gNetXferResult < 0) {
            *st = 0x63;
        }
        break;
    case 15:
        *st = 0x64;
        func_AE2570();
        break;
    case 0x63:
        *st = 0;
        func_AE2570();
        return -1;
    case 0x64:
        *st = 0;
        return gNetXferSize;
    }
    return 0;
}

s32 func_AE2588(s32 a, s32 b, s32 *out);
s32 func_AE2EC8(s32 a, s32 b);

/* The same as NetXfer_Step with the other transfer functions; its step 11 passes an
 * uninitialised local (a volatile: the stack word at +0x3C) as the first argument. */
s32 NetXfer_Step2(s8 *st, s16 *unused, u8 *b, s32 a, s32 max)
{
    volatile s32 h;

    switch (*st) {
    case 0:
        *st = 10;
        gNetXferSize = 0;
        gNetXferResult = func_AE2518();
        if (gNetXferResult < 0) {
            *st = 0x63;
        }
        break;
    case 10:
        (*st)++;
        gNetXferResult = func_AE2588(a, (s32) b, &gNetXferSize);
        if (gNetXferResult < 0) {
            *st = 0x63;
        } else if (gNetXferSize > max) {
            *st = 0x63;
        }
        break;
    case 11:
        *st = 15;
        /* the game reads a stack word it never set: h is the word at sp+0x3C, nothing here writes it */
        gNetXferResult = func_AE2EC8(h, gNetXferSize);
        if (gNetXferResult < 0) {
            *st = 0x63;
        }
        break;
    case 15:
        *st = 0x64;
        func_AE2570();
        break;
    case 0x63:
        *st = 0;
        func_AE2570();
        return -1;
    case 0x64:
        *st = 0;
        return gNetXferSize;
    }
    return 0;
}

s32 Dnas_GetError(void)
{
    return gDnasResult;
}

s32 Dnas_AuthStep(s8 *em, s16 *timer, s16 *percent)
{
    return Dnas_Step(em, timer, percent, 0);
}

s32 Dnas_StateMachine(s8 *em, s16 *timer, s16 *percent)
{
    return Dnas_Step(em, timer, percent, 0x1);
}

extern u16 gDnasWait;
extern char gDnasProxyHost[];
extern s32 D_0059D5F0;
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
    s32 *unk4;
    s32 unk8;
    s32 *unkC;
    s32 unk10;
} DnasParam;

extern u8 *gPadInputPtr;
/* NETWORK: the DNAS steps (state *st, arg1 a timeout count, *arg2 progress in percent; arg3 picks
 * func_ACA4E0 (0) or func_AC9A08 (1)): 1 when done, -1 on failure. */
s32 Dnas_Step(s8 *st, s16 *timer, s16 *percent, s32 arg3)
{
    DnasParam prm;
    s32 buf[4];
    char *p;

    switch (*st) {
    case 0:
        gDnasResult = 0;
        D_0059D5F0 = 0;
        if (gNetDevCur->type == 2 && *(s32 *)(gPadInputPtr + 8) == 0x4300) {
            D_0059D5F0 = 1;
        }
        if (D_0059D5F0 != 0) {
            NetDns_Term();
        }
        NetIf_SetErrorCheck(0);
        *st = 0x10;
        *percent = 0;
        gDnasWait = 0;
        break;
    case 0xF:
        sceCdDiskReady(0, 3);
        *st = 1;
        *percent = 3;
        gDnasWait = 0;
        break;
    case 0x10:
        gDnasWait++;
        *percent = 1;
        if (sceCdStandby(1, 3) == 1 || gDnasWait > 300) {
            *st = 0x11;
            gDnasWait = 0;
            break;
        }
        if (*timer >= 30) {
            gDnasResult = 0x7F;
            *st = 0;
            NetIf_SetErrorCheck(1);
            if (D_0059D5F0 != 0) {
                *st = 0x62;
                break;
            }
            return -1;
        }
        break;
    case 0x11:
        gDnasWait++;
        *percent = 2;
        if (sceCdSync(1, 3) == 0 || gDnasWait > 300) {
            *st = 0x12;
            gDnasWait = 0;
        }
        break;
    case 0x12:
        gDnasWait++;
        *percent = 3;
        if (sceCdDiskReady(1, 3) == 2 || gDnasWait > 300) {
            *st = 1;
            gDnasWait = 0;
            break;
        }
        if (*timer >= 30) {
            gDnasResult = 0x7F;
            *st = 0;
            NetIf_SetErrorCheck(1);
            if (D_0059D5F0 != 0) {
                *st = 0x62;
                break;
            }
            return -1;
        }
        break;
    case 1:
        if (D_0059D5F0 != 0) {
            NetDns_Init((char *)&gNetDomain, 0x79F3D5CA, 0x59F3D5CA);
        }
        if (arg3 == 0) {
            Net_InitSockets();
            net_clear_host_cache();
        }
        (*st)++;
        *percent = 5;
        break;
    case 2:
        memset(gDnasStatusPrev, 0, 0x10);
        memset(&prm, 0, 0x14);
        prm.unk0 = 1;
        prm.unkC = &D_004054E0;
        prm.unk10 = 0x10000;
        prm.unk4 = &D_004054D0;
        prm.unk8 = 8;
        gDnasResult = func_AC99D0(&prm, 0x30, 0, 0, 0);
        if (gDnasResult < 0) {
            *st = 0x63;
        } else {
            (*st)++;
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
            p = gDnasProxyHost + strlen((s8 *)gDnasProxyHost) - 1;
            do {
                if (*p == ':') {
                    break;
                }
                p--;
            } while (gDnasProxyHost < p);
            if (gDnasProxyHost < p) {
                *p = 0;
                func_ACA1E8(1, gDnasProxyHost, atoi((s8 *)p + 1));
            }
        }
        if (gDnasResult < 0) {
            *st = 0x63;
        } else {
            (*st)++;
            *percent = 20;
        }
        break;
    case 4:
        switch (arg3) {
        case 0:
            gDnasResult = func_ACA4E0();
            break;
        case 1:
            gDnasResult = func_AC9A08();
            break;
        default:
            while (1) {
            }
        }
        if (gDnasResult < 0) {
            *st = 0x63;
        } else {
            *st = 0xA;
            *percent = 30;
        }
        break;
    case 0xA:
        memset(buf, 0, 0x10);
        gDnasResult = func_ACA188(buf);
        if (gDnasResult < 0) {
            *st = 0x63;
        } else if (memcmp(gDnasStatusPrev, buf, 0x10) != 0) {
            if (*percent < 0x50) {
                *percent += 10;
            }
            if (buf[0] == 5) {
                memcpy(gDnasStatusPrev, buf, 0x10);
                *st = 0x64;
            } else if (buf[0] == 6 || buf[0] == 7) {
                gDnasResult = buf[1];
                *st = 0x63;
            } else {
                goto same;
            }
        } else {
        same:
            memcpy(gDnasStatusPrev, buf, 0x10);
            if (*timer >= 30) {
                gDnasResult = func_ACA070();
                *st = 0x5A;
            }
        }
        break;
    case 0x5A:
        memset(buf, 0, 0x10);
        gDnasResult = func_ACA188(buf);
        if (buf[0] == 2) {
            gDnasResult = 0x7F;
            *st = 0x63;
        }
        break;
    case 0x63:
        *st = 0;
        func_AC9D78();
        NetIf_SetErrorCheck(1);
        if (D_0059D5F0 != 0) {
            NetDns_Term();
            *st = 0x62;
            break;
        }
        return -1;
    case 0x62:
        *st = 0;
        Net_InitDns();
        return -1;
    case 0x64:
        *st = 0x7F;
        gDnasResult = func_AC9D78();
        NetIf_SetErrorCheck(1);
        if (D_0059D5F0 != 0) {
            NetDns_Term();
            *st = 0x65;
        }
        if (*percent < 0x5A) {
            *percent = 0x5A;
        }
        break;
    case 0x65:
        *st = 0x7F;
        Net_InitDns();
        break;
    case 0x7F:
        *st = 0;
        if (*percent < 100) {
            *percent = 100;
        }
        return 1;
    }
    return 0;
}

s32 NetTcp_GetOptions(s32 sock, s32 *out);
s32 NetTcp_SetOptions(s32 sock, s32 *in);

void NetTcp_SetOption(s32 a)
{
    s32 buf[2];

    NetTcp_GetOptions(a, buf);
    buf[0] = 1;
    buf[1] = 0;
    NetTcp_SetOptions(a, buf);
}

INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_308BC0", D_0046A8E0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_308BC0", D_0046A8F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_308BC0", D_0046A900);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_308BC0", D_0046A910);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_308BC0", D_0046A918);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_308BC0", D_0046A928);

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
    D_0059DCE0 = 0;
    memset((void *) &D_0059DBF0, 0, 0xF0U);
    memset((void *) &D_0059DBE0, 0, 0x10U);
    memset((void *) &gNetDomain, 0, 0x100U);
    gNetDns1 = 0;
    gNetDns2 = 0;
    memset((void *) &gNetAuthName, 0, 0x100U);
    memset((void *) &gNetAuthKey, 0, 0x100U);
    gNetPhyMode = 0;
    D_0059D8C0 = 0;
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

/* unk_308BC0's .data (0x4053A8-0x4154E0) */
extern char D_0046A508[];
extern char D_0046A518[];
extern char D_0046A528[];
extern char D_0046A538[];
extern char D_0046A540[];
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
NetDevice D_00405420[2] = {
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
