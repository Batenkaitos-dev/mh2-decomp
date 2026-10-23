/*
 * The BSD-style socket calls the HTTP library uses (net_socket,
 * net_connect, net_recv, net_send, net_close, net_gethostbyname, with
 * inet_addr, htons and the errno pointers), over AVE's TCP and DNS: eight
 * socket slots, a cache of looked-up hosts, and the socket lock and wait
 * semaphores (this file's data) that Net_InitSockets creates. The states,
 * errors and kinds are in net_tcp.h. Networking code stays
 * byte-identical.
 */
#include "common.h"
#include "sif.h"
#include "net_tcp.h"
#include "ave.h"
#include "net_sockets.h"
#include "libkernl.h"

/* The game built this file at optimization level 4. */
#pragma optimization_level 4

void strncpy(char *dst, const char *src, u32 n);
void * memset(void *p, s32 c, u32 n);
extern s32 gHostCacheNext;
extern s32 gNetHErrno;
extern s32 gNetErrno;
/* end of prototypes */

/* A struct hostent. */
typedef struct HostEnt {
    s8 *h_name;                 /* 0x00 */
    s8 **h_aliases;             /* 0x04 */
    s32 h_addrtype;             /* 0x08 */
    s32 h_length;               /* 0x0C */
    s8 **h_addr_list;           /* 0x10 */
} HostEnt;

/* A slot of gethostbyname's cache (8 of 308 bytes at gHostCache, gHostCacheNext the next one). */
typedef struct HostSlot {
    s8 name[256];             /* 0x000 */
    s32 addr;                   /* 0x100 */
    u8 pad104[12];
    s8 *addrList[4];            /* 0x110 */
    HostEnt ent;                /* 0x120 */
} HostSlot;

extern HostSlot gHostCache[8];

s32 atoi(s8 *s);             /* atoi */
u32 strlen(s8 *s);

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
        if (strncmp(slot->name, name, 256) == 0) {
            goto found;
        }
    }
    slot = &gHostCache[gHostCacheNext];
    dnsId = AveDns_Request(name);
    switch (dnsId) {
    case -33:
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
    strncpy((char *)slot->name, (char *)name, 256);
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
    return ((addrBytes[3] << 24) & 0xFF000000) | (((addrBytes[2] << 16) & 0xFF0000) | (addrBytes[0] | ((addrBytes[1] << 8) & 0xFF00)));
}


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
    memset(gHostCache, 0, 2464U);
    gHostCacheNext = 0;
    NetSock_Unlock();
}

extern s32 gNetSockWaitSema;

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
    memset(gNetSockHandles, 255, sizeof(gNetSockClose));
    memset(gNetSockClose, 255, sizeof(gNetSockClose));
    gNetErrno = 0;
    gNetHErrno = 0;
}

/* This file's initialised data. */
s32 gNetSockWaitSema = {
#include "sub_main/004053A8.inc"
};
s32 gNetSockSema = {
#include "sub_main/004053B0.inc"
};
