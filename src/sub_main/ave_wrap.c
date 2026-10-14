/*
 * NETWORK: EE side of Capcom_AVE_TCP_Wrapper, the IOP's SIF RPC server
 * 0x01270030. Every call fills the shared 0x400-byte buffer gAveWrapBuf
 * (words at 0x00-0x08, the payload from 0x1C), sends it with a function
 * number and gets the reply back into the same buffer: a halfword result
 * at 0x18 (negative = error) and results in the payload. Both sizes are
 * rounded up to 16 bytes. The semaphore gAveWrapSema serialises callers.
 * Must stay byte-identical for online play.
 */
#include "common.h"
#include "sif.h"
#include "ave.h"

typedef struct AveConfigArgs {
    s16 unk0;           /* 0x000 */
    s16 unk2;           /* 0x002 */
    char str4[0x100];   /* 0x004 */
    char str104[0x100]; /* 0x104 */
    char str204[0x40];  /* 0x204 */
    char str244[0x40];  /* 0x244 */
    char str284[0x40];  /* 0x284 */
    char str2C4[0x40];  /* 0x2C4 */
    s16 unk304[5];      /* 0x304 */
} AveConfigArgs;

typedef struct AveInfo {
    s32 w[11];
} AveInfo;

typedef union AvePayload {
    s32 w[0xF9];
    s16 h[0x1F2];
    u16 uh[0x1F2];
    char s[0x3E4];
    AveConfigArgs config;
    AveInfo info;
} AvePayload;

typedef struct AveWrapBuf {
    s32 arg[3];         /* 0x00 */
    u8 unkC[0xC];
    s32 result;         /* 0x18: a halfword result code, or a length */
    u8 data[0x3E4];     /* 0x1C: AvePayload */
} AveWrapBuf;

extern s32 gAveWrapSema;
extern s16 D_0059BB18;
extern s16 gAveWrapNoWait;
extern SifClientData gAveWrapClient;
extern AveWrapBuf gAveWrapBuf;

#define ALIGN16(x) (((x) % 16 != 0) ? (16 - (x) % 16) + (x) : (x))

void *memset(void *p, s32 c, u32 n);
void *memcpy(void *dst, void *src, u32 n);
char *strcpy(char *dst, char *src);            /* strcpy */
char *strncpy(char *dst, char *src, u32 n);     /* strncpy */
void SyncDCache(void *start, void *end);           /* SyncDCache */

s16 AveWrap_CallAsync(s32 fno, s32 ssize, s32 rsize);
s32 AveWrap_IsIdle(void);
void AveWrap_Lock(void);
void AveWrap_Unlock(void);

s32 AveWrap_Bind(void)
{
    SemaParam sp;
    volatile s32 i;

    if (gAveWrapSema < 0) {
        memset(&sp, 0, sizeof(sp));
        sp.initCount = 1;
        sp.maxCount = 1;
        gAveWrapSema = CreateSema(&sp);
    }
    gAveWrapNoWait = 0;
    D_0059BB18 = 0;
    for (;;) {
        if (sceSifBindRpc(&gAveWrapClient, 0x1270030, 0) >= 0 && gAveWrapClient.serve != NULL) {
            break;
        }
        for (i = 0x10000; i != 0; i--) {
        }
    }
    return 0;
}

s32 AveWrap_Unbind(void)
{
    memset(&gAveWrapClient, 0, sizeof(SifClientData));
    return 0;
}

s16 AveWrap_Call(s32 fno, s32 ssize, s32 rsize)
{
    if (gAveWrapNoWait != 0) {
        return AveWrap_CallAsync(fno, ssize, rsize);
    }
    SyncDCache(&gAveWrapBuf, &gAveWrapBuf.data[sizeof(gAveWrapBuf.data) - 1]);
    rsize = ALIGN16(rsize);
    ssize = ALIGN16(ssize);
    if (sceSifCallRpc(&gAveWrapClient, fno, 0, &gAveWrapBuf, ssize, &gAveWrapBuf, rsize, NULL, NULL) < 0) {
        return -0x21;
    }
    return *(s16 *)&gAveWrapBuf.result;
}

s16 AveWrap_CallAsync(s32 fno, s32 ssize, s32 rsize)
{
    if (AveWrap_IsIdle() == 0) {
        return -0x21;
    }
    SyncDCache(&gAveWrapBuf, &gAveWrapBuf.data[sizeof(gAveWrapBuf.data) - 1]);
    rsize = ALIGN16(rsize);
    ssize = ALIGN16(ssize);
    if (sceSifCallRpc(&gAveWrapClient, fno, 1, &gAveWrapBuf, ssize, &gAveWrapBuf, rsize, NULL, NULL) < 0) {
        return -0x21;
    }
    return 0;
}

s32 AveWrap_IsIdle(void)
{
    return (s16)!sceSifCheckStatRpc(&gAveWrapClient);
}

void AveWrap_Lock(void)
{
    WaitSema(gAveWrapSema);
}

void AveWrap_Unlock(void)
{
    SignalSema(gAveWrapSema);
}

s16 AveIf_Up(s32 a, s32 b)
{
    s16 r;

    AveWrap_Lock();
    gAveWrapBuf.arg[0] = a;
    gAveWrapBuf.arg[1] = b;
    r = AveWrap_Call(5, 0x20, 0x20);
    AveWrap_Unlock();
    return r;
}

s16 AveIf_Down(void)
{
    s16 r;

    AveWrap_Lock();
    r = AveWrap_Call(6, 0x20, 0x20);
    AveWrap_Unlock();
    return r;
}

s16 AveIf_SetAddress(s32 a, s32 b, s32 c)
{
    AvePayload *p;
    s16 r;

    AveWrap_Lock();
    p = (AvePayload *)gAveWrapBuf.data;
    p->w[0] = a;
    p->w[1] = b;
    p->w[2] = c;
    r = AveWrap_Call(1, 0x2C, 0x20);
    AveWrap_Unlock();
    return r;
}

s16 AveIf_ClearAddress(void)
{
    s16 r;

    AveWrap_Lock();
    r = AveWrap_Call(2, 0x20, 0x20);
    AveWrap_Unlock();
    return r;
}

s16 AveTcp_SetOption(s16 a, s16 b, s16 len, void *src)
{
    AvePayload *p;
    s16 r;

    AveWrap_Lock();
    p = (AvePayload *)gAveWrapBuf.data;
    p->h[0] = a;
    p->h[1] = b;
    p->h[2] = len;
    memcpy(&p->h[3], src, len);
    r = AveWrap_Call(3, 0x126, 0x20);
    AveWrap_Unlock();
    return r;
}

s16 AveTcp_GetOption(s16 a, s16 b, s16 len, void *dst)
{
    s16 r;
    AvePayload *p;

    AveWrap_Lock();
    p = (AvePayload *)gAveWrapBuf.data;
    p->h[0] = a;
    p->h[1] = b;
    p->h[2] = len;
    r = AveWrap_Call(4, 0x126, 0x126);
    if (r < 0) {
        AveWrap_Unlock();
        return r;
    }
    if (r <= len) {
        memcpy(dst, &p->h[3], r);
    }
    AveWrap_Unlock();
    return r;
}

s16 AveTcp_Open(s32 addr, u16 port, u16 unk)
{
    AvePayload *p;
    s16 r;

    AveWrap_Lock();
    p = (AvePayload *)gAveWrapBuf.data;
    p->w[0] = addr;
    p->h[2] = port;
    p->h[3] = unk;
    r = AveWrap_Call(7, 0x28, 0x20);
    AveWrap_Unlock();
    return r;
}

s16 AveTcp_Abort(s16 sock)
{
    AvePayload *p;
    s16 r;

    AveWrap_Lock();
    p = (AvePayload *)gAveWrapBuf.data;
    p->h[0] = sock;
    r = AveWrap_Call(0xD, 0x22, 0x20);
    AveWrap_Unlock();
    return r;
}

s16 AveTcp_Status(s16 sock, u16 *a, u16 *b, u16 *c)
{
    s16 r;
    AvePayload *p;

    AveWrap_Lock();
    p = (AvePayload *)gAveWrapBuf.data;
    p->h[0] = sock;
    r = AveWrap_Call(0xF, 0x2A, 0x2A);
    if (r < 0) {
        AveWrap_Unlock();
        return r;
    }
    *a = p->uh[1];
    *b = p->uh[3];
    *c = p->uh[4];
    AveWrap_Unlock();
    return r;
}

/*
 * The 0x3CA clamp is an if/else (`n = len` in the else keeps the `b` past
 * the clamp), and the delay between sends steps i before the 7 nops of an
 * inline asm (which also keeps both loops unrotated).
 */
s16 AveTcp_Send(s16 sock, u8 *data, s16 len)
{
    volatile s32 i;
    s16 r;
    AvePayload *p;
    s16 n;

    AveWrap_Lock();
    p = (AvePayload *)gAveWrapBuf.data;
    r = 0;
    p->h[0] = sock;
    while (0 < len) {
        if (len > 0x3CA) {
            n = 0x3CA;
        } else {
            n = len;
        }
        p->h[1] = n;
        memcpy(&p->h[2], data, n);
        gAveWrapBuf.arg[0] = 0;
        gAveWrapBuf.arg[1] = 0;
        r = AveWrap_Call(0x10, n + 0x26, n + 0x26);
        if (r < 0) {
            AveWrap_Unlock();
            return r;
        }
        len -= n;
        data += n;
        if (len <= 0) {
            break;
        }
        i = 0;
        while (i < 10000) {
            i++;
            __asm__ volatile ("nop\n nop\n nop\n nop\n nop\n nop\n nop");
        }
    }
    AveWrap_Unlock();
    return r;
}

s16 AveTcp_Recv(s16 sock, void *buf, s16 len)
{
    s16 r;
    AvePayload *p;

    if (len <= 0) {
        return 0;
    }
    AveWrap_Lock();
    if (len > 0x3CA) {
        len = 0x3CA;
    }
    p = (AvePayload *)gAveWrapBuf.data;
    p->h[0] = sock;
    p->h[1] = len;
    r = AveWrap_Call(0x13, len + 0x26, len + 0x26);
    if (r < 0) {
        AveWrap_Unlock();
        return r;
    }
    if (gAveWrapBuf.result <= len) {
        len = gAveWrapBuf.result;
    }
    if (0 < len) {
        memcpy(buf, &p->h[2], len);
    }
    AveWrap_Unlock();
    return len;
}

s16 AveTcp_Delete(s16 sock)
{
    AvePayload *p;
    s16 r;

    AveWrap_Lock();
    p = (AvePayload *)gAveWrapBuf.data;
    p->h[0] = sock;
    r = AveWrap_Call(0x16, 0x22, 0x20);
    AveWrap_Unlock();
    return r;
}

s16 AveRoute_Add(s32 a, s32 b)
{
    AvePayload *p;
    s16 r;

    AveWrap_Lock();
    p = (AvePayload *)gAveWrapBuf.data;
    p->w[0] = a;
    p->w[1] = b;
    r = AveWrap_Call(0x1E, 0x28, 0x20);
    AveWrap_Unlock();
    return r;
}

s16 AveRoute_Delete(s32 a, s32 b)
{
    AvePayload *p;
    s16 r;

    AveWrap_Lock();
    p = (AvePayload *)gAveWrapBuf.data;
    p->w[0] = a;
    p->w[1] = b;
    r = AveWrap_Call(0x1F, 0x28, 0x20);
    AveWrap_Unlock();
    return r;
}

s16 AvePpp_Init(void)
{
    s16 r;

    AveWrap_Lock();
    r = AveWrap_Call(0x21, 0x20, 0x20);
    AveWrap_Unlock();
    return r;
}

s16 AvePpp_Term(void)
{
    s16 r;

    AveWrap_Lock();
    r = AveWrap_Call(0x22, 0x20, 0x20);
    AveWrap_Unlock();
    return r;
}

s16 AvePpp_Connect(AveConfig *c)
{
    AvePayload *p;
    s16 r;

    AveWrap_Lock();
    gAveWrapBuf.arg[0] = c->unk1;
    p = (AvePayload *)gAveWrapBuf.data;
    gAveWrapBuf.arg[1] = c->unk26;
    gAveWrapBuf.arg[2] = c->unk27;
    memset(p, 0, sizeof(AveConfigArgs));
    p->config.unk0 = c->unk0;
    strcpy(p->config.str4, c->str4);
    strcpy(p->config.str104, c->str8);
    strcpy(p->config.str204, c->strC);
    strcpy(p->config.str244, c->str10);
    strcpy(p->config.str284, c->str14);
    p->config.unk2 = c->unk2;
    strcpy(p->config.str2C4, c->str18);
    p->config.unk304[0] = c->unk1C;
    p->config.unk304[1] = c->unk1E;
    p->config.unk304[2] = c->unk20;
    p->config.unk304[3] = c->unk22;
    p->config.unk304[4] = c->unk24;
    r = AveWrap_Call(0x23, 0x32E, 0x20);
    AveWrap_Unlock();
    return r;
}

s16 AvePpp_Disconnect(void)
{
    s16 r;

    AveWrap_Lock();
    r = AveWrap_Call(0x24, 0x20, 0x20);
    AveWrap_Unlock();
    return r;
}

s16 AvePpp_GetStatus(AveInfo *out)
{
    AvePayload *p;
    s16 r;

    AveWrap_Lock();
    r = AveWrap_Call(0x25, 0x20, 0x4C);
    if (r < 0) {
        AveWrap_Unlock();
        return r;
    }
    p = (AvePayload *)gAveWrapBuf.data;
    *out = p->info;
    AveWrap_Unlock();
    return r;
}

s16 AvePpp_GetStatus26_Unk(s32 *out)
{
    AvePayload *p;
    s16 r;

    AveWrap_Lock();
    r = AveWrap_Call(0x26, 0x20, 0x4C);
    if (r < 0) {
        AveWrap_Unlock();
        return r;
    }
    p = (AvePayload *)gAveWrapBuf.data;
    *out = p->w[0];
    AveWrap_Unlock();
    return r;
}

s16 AvePpp_SetOption(s16 a, s16 len, void *src)
{
    AvePayload *p;
    s16 r;

    AveWrap_Lock();
    p = (AvePayload *)gAveWrapBuf.data;
    p->h[0] = a;
    p->h[1] = len;
    memcpy(&p->h[2], src, len);
    r = AveWrap_Call(0x27, 0x124, 0x20);
    AveWrap_Unlock();
    return r;
}

s16 AveEth_Init(void)
{
    s16 r;

    AveWrap_Lock();
    r = AveWrap_Call(0x29, 0x20, 0x20);
    AveWrap_Unlock();
    return r;
}

s16 AveEth_Term(void)
{
    s16 r;

    AveWrap_Lock();
    r = AveWrap_Call(0x2A, 0x20, 0x20);
    AveWrap_Unlock();
    return r;
}

s16 AveEth_Start(s16 a)
{
    s16 r;

    AveWrap_Lock();
    gAveWrapBuf.arg[0] = a;
    r = AveWrap_Call(0x2B, 0x20, 0x20);
    AveWrap_Unlock();
    return r;
}

s16 AveEth_IsLinkUp(void)
{
    s16 r;

    AveWrap_Lock();
    r = AveWrap_Call(0x2C, 0x20, 0x20);
    AveWrap_Unlock();
    return r;
}

s16 AveEth_Stop(void)
{
    s16 r;

    AveWrap_Lock();
    r = AveWrap_Call(0x2D, 0x20, 0x20);
    AveWrap_Unlock();
    return r;
}

s16 AveDhcp_Init(void)
{
    s16 r;

    AveWrap_Lock();
    r = AveWrap_Call(0x2E, 0x20, 0x20);
    AveWrap_Unlock();
    return r;
}

s16 AveDhcp_Term(void)
{
    s16 r;

    AveWrap_Lock();
    r = AveWrap_Call(0x2F, 0x20, 0x20);
    AveWrap_Unlock();
    return r;
}

s16 AveDhcp_GetDns(char *dst, s32 *a, s32 *b)
{
    s16 r;
    AvePayload *p;

    AveWrap_Lock();
    r = AveWrap_Call(0x30, 0x20, 0x128);
    if (r < 0) {
        AveWrap_Unlock();
        return r;
    }
    p = (AvePayload *)gAveWrapBuf.data;
    strcpy(dst, p->s);
    *a = p->w[0x40];
    *b = p->w[0x41];
    AveWrap_Unlock();
    return r;
}

s16 AveDhcp_GetRouter(s32 *out)
{
    AvePayload *p;
    s16 r;

    AveWrap_Lock();
    r = AveWrap_Call(0x31, 0x20, 0x24);
    if (r < 0) {
        AveWrap_Unlock();
        return r;
    }
    p = (AvePayload *)gAveWrapBuf.data;
    *out = p->w[0];
    AveWrap_Unlock();
    return r;
}

s16 AveDhcp_Start(s32 a, s32 b)
{
    AvePayload *p;
    s16 r;

    AveWrap_Lock();
    p = (AvePayload *)gAveWrapBuf.data;
    p->w[0] = a;
    p->w[1] = b;
    r = AveWrap_Call(0x36, 0x28, 0x20);
    AveWrap_Unlock();
    return r;
}

s16 AveDhcp_GetAddress(s32 *a, s32 *b, s32 *c)
{
    AvePayload *p;
    s16 r;

    AveWrap_Lock();
    r = AveWrap_Call(0x37, 0x20, 0x2C);
    if (r < 0) {
        AveWrap_Unlock();
        return r;
    }
    p = (AvePayload *)gAveWrapBuf.data;
    *a = p->w[0];
    *b = p->w[1];
    *c = p->w[2];
    AveWrap_Unlock();
    return r;
}

s16 AveDhcp_GetState(void)
{
    s16 r;

    AveWrap_Lock();
    r = AveWrap_Call(0x38, 0x20, 0x20);
    AveWrap_Unlock();
    return r;
}

s16 AveDhcp_Release(s32 a, s32 b)
{
    AvePayload *p;
    s16 r;

    AveWrap_Lock();
    p = (AvePayload *)gAveWrapBuf.data;
    p->w[0] = a;
    p->w[1] = b;
    r = AveWrap_Call(0x3A, 0x28, 0x20);
    AveWrap_Unlock();
    return r;
}

s16 AveDns_Init(char *server, s32 a, s32 b)
{
    AvePayload *p;
    s16 r;

    AveWrap_Lock();
    p = (AvePayload *)gAveWrapBuf.data;
    if (server != NULL) {
        strcpy(p->s, server);
    } else {
        p->s[0] = 0;
    }
    p->w[0x40] = a;
    p->w[0x41] = b;
    r = AveWrap_Call(0x3B, 0x128, 0x20);
    AveWrap_Unlock();
    return r;
}

s16 AveDns_Term(void)
{
    s16 r;

    AveWrap_Lock();
    r = AveWrap_Call(0x3C, 0x20, 0x20);
    AveWrap_Unlock();
    return r;
}

s16 AveDns_Request(char *name)
{
    AvePayload *p;
    s16 r;

    AveWrap_Lock();
    p = (AvePayload *)gAveWrapBuf.data;
    strncpy(p->s, name, 0x200);
    p->s[0x1FF] = 0;
    r = AveWrap_Call(0x3D, 0x220, 0x20);
    AveWrap_Unlock();
    return r;
}

s16 AveDns_Free(s16 a)
{
    AvePayload *p;
    s16 r;

    AveWrap_Lock();
    p = (AvePayload *)gAveWrapBuf.data;
    p->h[0] = a;
    r = AveWrap_Call(0x3E, 0x22, 0x20);
    AveWrap_Unlock();
    return r;
}

s16 AveDns_LookUp(s16 id, s32 *addr)
{
    s16 r;
    AvePayload *p;

    AveWrap_Lock();
    p = (AvePayload *)gAveWrapBuf.data;
    p->h[0] = id;
    r = AveWrap_Call(0x3F, 0x28, 0x28);
    if (0 <= r) {
        *addr = p->w[1];
    }
    AveWrap_Unlock();
    return r;
}

s16 AveIf_PollEvent_Unk(s32 *out)
{
    s16 r;

    AveWrap_Lock();
    r = AveWrap_Call(0x46, 0x20, 0x20);
    if (0 <= r) {
        *out = gAveWrapBuf.arg[0];
    }
    AveWrap_Unlock();
    return r;
}

s16 AveIf_GetEvent_Unk(s32 a, s32 *b, s32 *c, s32 *d)
{
    AvePayload *p;
    s16 r;

    AveWrap_Lock();
    gAveWrapBuf.arg[0] = a;
    r = AveWrap_Call(0x47, 0x20, 0x32C);
    p = (AvePayload *)gAveWrapBuf.data;
    *b = p->w[0];
    *c = p->w[1];
    *d = p->w[2];
    AveWrap_Unlock();
    return r;
}

s16 AveIf_AckEvent_Unk(s32 a)
{
    s16 r;

    AveWrap_Lock();
    gAveWrapBuf.arg[0] = a;
    r = AveWrap_Call(0x48, 0x20, 0x20);
    AveWrap_Unlock();
    return r;
}

s16 AveIf_GetOption_Unk(s32 a, s32 len, void *dst)
{
    s16 r;
    AvePayload *p;

    AveWrap_Lock();
    p = (AvePayload *)gAveWrapBuf.data;
    p->w[0] = a;
    p->w[1] = len;
    r = AveWrap_Call(0x4A, 0x128, 0x128);
    if (r < 0) {
        AveWrap_Unlock();
        return r;
    }
    if (r <= len) {
        memcpy(dst, &p->w[2], r);
    }
    AveWrap_Unlock();
    return r;
}

/* ave_wrap's .data (0x405178-0x405180) */
s32 gAveWrapSema = {
#include "sub_main/00405178.inc"
};
