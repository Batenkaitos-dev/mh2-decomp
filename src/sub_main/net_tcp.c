/*
 * NETWORK: "layer A" over the AVE wrapper (ave_wrap.c), 0x3075C0-0x308BC0:
 * a connection state gNetIfState (set to 1 by NetIf_Up, 2 by
 * NetPpp_Connect/NetEth_Start, 3 by NetPpp_SetConnected/NetEth_SetConnected, 4 by
 * NetPpp_SetClosed/NetEth_ClearAddress, 5 by NetPpp_Disconnect/NetEth_Stop, 0 by
 * NetIf_Down), the error state gNetIfError polled by NetIf_PollError, the
 * modem chat scripts (NetModem_GetChat), and the NetTcp_* calls used by lobby
 * and gm_sub. AVE results pass through NetTcp_Result unchanged. The file
 * is complete: code, .data (the chat scripts, 0x405180-0x4053A8), .rodata
 * (the jump tables of NetTcp_Status, NetPpp_Connect and NetPpp_GetStatus,
 * 0x46A450-0x46A508) and .bss (0x59BF80-0x59C1B0). Must stay byte-identical
 * for online play.
 */
#include "common.h"
#include "sif.h"
#include "ave.h"
#include "net_tcp.h"

typedef struct NetAddr {
    s32 addr;
    u16 port;
    u16 unk6;
} NetAddr;

/* The game's connection settings, as NetPpp_Connect reads them. */
typedef struct NetModemCfg {
    s16 unk0;           /* 0-6 */
    s16 unk2;
    s16 unk4;
    s16 unk6;
    s16 unk8;
} NetModemCfg;

typedef struct NetAuthCfg {
    s16 unk0;
    s16 unk2;
    char *str4;
} NetAuthCfg;

typedef struct NetDialCfg {
    NetModemCfg *modem; /* 0x00 */
    NetAuthCfg *auth;   /* 0x04 */
    s16 *line;          /* 0x08 */
    char *strC;
    char *str10;
    char *str14;
    char *str18;
    char *str1C;
} NetDialCfg;

typedef struct NetDialArg {
    NetDialCfg *cfg;    /* 0x00 */
    s16 unk4;
    s8 unk6;
    s8 unk7;
} NetDialArg;


typedef struct NetConnInfo {
    s32 unk0;
    s32 unk4;
    s32 speed;          /* 0x08: the link speed (bps) */
    s16 phase;          /* 0x0C: 4 = up */
    s16 unkE;
    s16 error;          /* 0x10: why it went down (12, 13...) */
} NetConnInfo;

typedef struct AveInfoSub {
    s32 unk0;
    s32 unk4;
    s32 unk8;
    s32 unkC;
    s32 dns1;           /* 0x10 */
    s32 dns2;           /* 0x14 */
} AveInfoSub;

typedef struct AveInfo {
    s32 unk0;
    s32 unk4;
    s16 phase;          /* 0x08: 0-9, 4 = up */
    s16 unkA;
    s32 speed;          /* 0x0C: the link speed (bps) */
    s16 unk10;
    s16 error;          /* 0x12: 0-13 */
    AveInfoSub sub;     /* 0x14 */
} AveInfo;

/*
 * The file's .bss (0x59BF80-0x59C1B0). MWCC emits uninitialized variables
 * last-declared first, so they are declared from the highest address down.
 * gNetIfStarted is seven flags (one byte each, set and cleared in pairs of
 * calls below).
 */
char gModemInitChat[0x100];
char gModemChat2[0x100];
u8 gNetIfStarted[7];
s32 gNetIfError;
s32 gNetIfNoErrorCheck;
s32 gNetIfState;
s32 gNetPendingEvent;
u8 gNetEventPending;
/* The modem chat scripts (the file's .data, 0x405180-0x4053A8). */
static char D_00405180[0x100] = {
#include "sub_main/00405180.inc"
};
static char D_00405280[0x100] = {
#include "sub_main/00405280.inc"
};
static char D_00405380[33] = {
#include "sub_main/00405380.inc"
};
extern s32 gNetSockSema;

s32 AveWrap_Bind(void);
/* the calls cast to s16: this unit's code takes the value as s16 */
s32 AveWrap_IsIdle(void);
s32 AveWrap_Unbind(void);
s16 AveIf_Up(s32 a, s32 b);
s16 AveIf_Down(void);
s16 AveIf_SetAddress(s32 a, s32 b, s32 c);
s16 AveIf_ClearAddress(void);
s16 AveTcp_SetOption(s16 sock, s16 opt, s16 len, void *src);
s16 AveTcp_GetOption(s16 sock, s16 opt, s16 len, void *dst);
s16 AveTcp_Open(s32 addr, u16 port, u16 unk);
s16 AveTcp_Status(s16 sock, u16 *a, u16 *b, u16 *c);
s16 AveTcp_Abort(s16 sock);
s16 AveTcp_Send(s16 sock, u8 *data, s16 len);
s16 AveTcp_Recv(s16 sock, void *buf, s16 len);
s16 AveTcp_Delete(s16 sock);
s16 AveRoute_Add(s32 a, s32 b);
s16 AveRoute_Delete(s32 a, s32 b);
s16 AvePpp_Init(void);
s16 AvePpp_Term(void);
s16 AvePpp_Connect(AveConfig *c);
s16 AvePpp_Disconnect(void);
s16 AvePpp_GetStatus(AveInfo *out);
s16 AvePpp_GetStatus26_Unk(s32 *out);
/* unit's own declaration: with the definition's s16 (s16 a, s16 len, void *src) (sub_main/ave_wrap.c), NetPpp_SetOption1: 10 of 11 words differ */
s32 AvePpp_SetOption(s16 a, s16 len, void *src);
s16 AveEth_Init(void);
s16 AveEth_Term(void);
s16 AveEth_Start(s16 a);
s16 AveEth_IsLinkUp(void);
s16 AveEth_Stop(void);
s16 AveDhcp_Init(void);
s16 AveDhcp_Term(void);
s16 AveDhcp_GetDns(char *name, s32 *a, s32 *b);
s16 AveDhcp_GetRouter(s32 *out);
s16 AveDhcp_Start(s32 a, s32 b);
s16 AveDhcp_GetAddress(s32 *a, s32 *b, s32 *c);
s16 AveDhcp_GetState(void);
s16 AveDhcp_Release(s32 a, s32 b);
s16 AveDns_Init(char *server, s32 a, s32 b);
s16 AveDns_Term(void);
s16 AveDns_Request(char *name);
s16 AveDns_Free(s16 a);
s16 AveDns_LookUp(s16 id, s32 *addr);
s16 AveIf_PollEvent_Unk(s32 *out);
s16 AveIf_GetEvent_Unk(s32 a, s32 *b, s32 *c, s32 *d);
s16 AveIf_AckEvent_Unk(s32 a);
s16 AveIf_GetOption_Unk(s32 a, s32 len, void *dst);
s32 NetDhcp_GetState(void);
s32 NetIf_PollEvent_Unk(s32 *out);
s32 NetPpp_GetStatus(NetConnInfo *out);
char *strcat(char *dst, char *src);
s32 sprintf(char *dst, char *fmt, ...);

s32 NetTcp_Init(void)
{
    gNetEventPending = 0;
    gNetPendingEvent = 0;
    gNetIfState = 0;
    gNetIfNoErrorCheck = 0;
    AveWrap_Bind();
    return 0;
}

s32 NetTcp_Term(void)
{
    gNetEventPending = 0;
    gNetPendingEvent = 0;
    gNetIfState = 0;
    gNetIfNoErrorCheck = 0;
    AveWrap_Unbind();
    return 0;
}

s32 NetTcp_Result(s32 r)
{
    return r;
}

s32 NetTcp_IsIdle(void)
{
    return (s16)AveWrap_IsIdle();
}

s32 NetIf_IsConnected(void)
{
    return gNetIfState == 3;
}

s32 NetIf_GetState(void)
{
    return gNetIfState;
}

s32 NetIf_SetErrorCheck(s32 on)
{
    return gNetIfNoErrorCheck = !on;
}

s32 NetIf_PollError(s32 mode, s32 check)
{
    NetConnInfo info;
    s32 tmp;

    if (gNetIfError != 0) {
        return gNetIfError;
    }
    if (gNetIfNoErrorCheck != 0) {
        return gNetIfError;
    }
    if (NetTcp_IsIdle() == 0) {
        return gNetIfError;
    }
    switch (mode) {
    case 1:
        if (check != 0 && NetDhcp_GetState() < 0) {
            gNetIfError = 1;
            break;
        }
        switch (NetIf_PollEvent_Unk(&tmp)) {
        case 0:
            break;
        case 1:
            gNetIfError = 2;
            break;
        }
        break;
    case 2:
    case 3:
        if (NetPpp_GetStatus(&info) < 0) {
            gNetIfError = 1;
        } else if (info.phase != 4) {
            gNetIfError = 2;
        }
        break;
    }
    return gNetIfError;
}

s32 NetIf_GetError(void)
{
    return gNetIfError;
}

s16 NetIf_Up(s32 mode, s32 kind)
{
    s16 r;

    switch (mode) {
    case 3:
    case 2:
        switch (kind) {
        case 5:
            r = AveIf_Up(0, 1);
            break;
        default:
            r = AveIf_Up(0, 4);
            break;
        }
        break;
    case 1:
        switch (kind) {
        case 1:
        case 6:
            r = AveIf_Up(1, 5);
            break;
        default:
            r = AveIf_Up(0, 2);
            break;
        }
        break;
    }
    gNetIfStarted[0] = 1;
    gNetIfState = 1;
    return r;
}

s16 NetIf_Down(void)
{
    s16 r = AveIf_Down();

    gNetIfStarted[0] = 0;
    gNetIfState = 0;
    gNetIfError = 0;
    return r;
}

s32 NetTcp_Open(NetAddr *a)
{
    return NetTcp_Result(AveTcp_Open(a->addr, a->port, a->unk6));
}

s32 NetTcp_GetOptions(s32 sock, s32 *out)
{
    s32 r;

    r = AveTcp_GetOption(sock, 0x2008, 4, out);
    r = NetTcp_Result(r);
    if (r < 0) {
        return r;
    }
    r = AveTcp_GetOption(sock, 0x2001, 4, out + 1);
    r = NetTcp_Result(r);
    return r;
}

s32 NetTcp_SetOptions(s32 sock, s32 *in)
{
    s32 r;

    r = AveTcp_SetOption(sock, 0x2008, 4, in);
    r = NetTcp_Result(r);
    if (r < 0) {
        return r;
    }
    r = AveTcp_SetOption(sock, 0x2001, 4, in + 1);
    r = NetTcp_Result(r);
    return r;
}

/* The connection's state and two more fields from the AVE wrapper; AVE's
 * state -1 becomes 11, and anything unknown 7. */
s32 NetTcp_Status(s32 sock, NetTcpStatus *st)
{
    s32 r = NetTcp_Result(AveTcp_Status(sock, (u16 *)&st->state, &st->sendSpace, &st->recvLen));

    if (r < 0) {
        return r;
    }
    switch (st->state) {
    case 0:
        st->state = 0;
        break;
    case 1:
        st->state = 1;
        break;
    case 2:
        st->state = 2;
        break;
    case 3:
        st->state = 3;
        break;
    case 4:
        st->state = 4;
        break;
    case 5:
        st->state = 5;
        break;
    case 6:
        st->state = 6;
        break;
    default:
        st->state = 7;
        break;
    case 8:
        st->state = 8;
        break;
    case 9:
        st->state = 9;
        break;
    case 10:
        st->state = 10;
        break;
    case -1:
        st->state = 11;
        break;
    }
    return r;
}

s32 NetTcp_Recv(s32 sock, void *buf, s16 len)
{
    return NetTcp_Result(AveTcp_Recv(sock, buf, len));
}

s32 NetTcp_Send(s32 sock, void *data, s16 len)
{
    s32 r = NetTcp_Result(AveTcp_Send(sock, data, len));

    if (r < 0) {
        return r;
    }
    if (r == 0) {
        r = len;
    }
    return r;
}

s32 NetTcp_Abort(s32 sock)
{
    return NetTcp_Result(AveTcp_Abort(sock));
}

s32 NetTcp_Delete(s32 *sock)
{
    s32 r = 0;
    s16 ret;

    if (0 <= *sock) {
        ret = AveTcp_Delete(*sock);
        *sock = -1;
        r = NetTcp_Result(ret);
    } else {
        *sock = -1;
    }
    return r;
}

s32 NetDns_Init(char *server, s32 a, s32 b)
{
    s32 r = AveDns_Init(server, a, b);

    switch (r) {
    case -0x21:
    case -1:
        r = -1;
        break;
    }
    gNetIfStarted[4] = 1;
    return r;
}

s32 NetDns_Term(void)
{
    s32 r = AveDns_Term();

    switch (r) {
    case -0x21:
    case -1:
        r = -1;
        break;
    }
    gNetIfStarted[4] = 0;
    return r;
}

s32 NetDns_Request(char *name)
{
    s32 r = AveDns_Request(name);

    switch (r) {
    case -0x21:
    case -1:
        r = -1;
        break;
    }
    return r;
}

s32 NetDns_Free(s16 id)
{
    s32 r = AveDns_Free(id);

    switch (r) {
    case -0x21:
    case -1:
        r = -1;
        break;
    }
    return r;
}

s32 NetDns_LookUp(s16 id, s32 *addr)
{
    s32 r = AveDns_LookUp(id, addr);

    switch (r) {
    case -0x21:
        r = -1;
        break;
    case -1:
        r = -1;
        break;
    case -6:
        r = -2;
        break;
    case -2:
        r = -3;
        break;
    }
    return r;
}

s32 NetPpp_Init(void)
{
    s16 r = AvePpp_Init();

    gNetIfStarted[1] = 1;
    return r;
}

s16 NetPpp_SetOption1(void *src)
{
    return AvePpp_SetOption(1, 4, src);
}

s32 NetPpp_GetInfo(s32 *out)
{
    AveInfo info;
    AveInfoSub *sub;
    s16 r;

    r = AvePpp_GetStatus(&info);
    if (r < 0) {
        return r;
    }
    out[0] = info.unk4;
    out[1] = info.unk0;
    sub = &info.sub;
    out[2] = sub->dns1;
    out[3] = sub->dns2;
    out[4] = sub->unk8;
    out[5] = sub->unkC;
    return r;
}

s32 NetModem_GetChat(s32 mode, s32 kind, s32 a2, s32 a3, char **out)
{
    switch (mode) {
    case 2:
        gModemInitChat[0] = 0;
        switch (kind) {
        case 2:
        case 5:
            strcat(gModemInitChat, D_00405180);
            break;
        case 4:
            strcat(gModemInitChat, D_00405280);
            break;
        default:
            sprintf(gModemInitChat, D_00405380);
            break;
        }
        gModemChat2[0] = 0;
        out[0] = gModemInitChat;
        out[1] = gModemChat2;
        break;
    case 3:
        break;
    case 1:
        gModemInitChat[0] = 0;
        gModemChat2[0] = 0;
        out[0] = gModemInitChat;
        out[1] = gModemChat2;
        break;
    }
    return 0;
}

s32 NetPpp_GetLinkState_Unk(void)
{
    s32 tmp;

    if (AvePpp_GetStatus26_Unk(&tmp) == 1) {
        return tmp;
    }
    return -1;
}

/* Connects: turns the game's connection settings into the AVE wrapper's
 * (AvePpp_Connect) and sets the connection state to 2. */
s32 NetPpp_Connect(NetDialArg *arg)
{
    AveConfig c;
    char empty[4];

    empty[0] = 0;
    c.unk0 = 1;
    c.unk26 = arg->unk6;
    c.unk1 = arg->unk7;
    c.str4 = arg->cfg->strC;
    c.str8 = arg->cfg->str10;
    c.strC = arg->cfg->str14;
    c.str10 = arg->cfg->str18;
    c.str14 = arg->cfg->str1C;
    c.str18 = arg->cfg->auth->str4;
    if (c.str18 == NULL) {
        c.str18 = empty;
    }
    switch (arg->cfg->modem->unk0) {
    default:
    case 5:
    case 6:
        c.unk1C = 6;
        break;
    case 4:
        c.unk1C = 5;
        break;
    case 3:
        c.unk1C = 4;
        break;
    case 2:
        c.unk1C = 3;
        break;
    case 1:
        c.unk1C = 2;
        break;
    case 0:
        c.unk1C = 0;
        break;
    }
    switch (arg->cfg->modem->unk2) {
    default:
        c.unk1E = 0;
        break;
    case 1:
        c.unk1E = 1;
        break;
    }
    switch (arg->cfg->modem->unk4) {
    default:
    case 2:
        c.unk20 = 2;
        break;
    case 1:
        c.unk20 = 1;
        break;
    case 0:
        c.unk20 = 0;
        break;
    }
    switch (arg->cfg->modem->unk6) {
    default:
    case 1:
        c.unk22 = 3;
        break;
    case 0:
        c.unk22 = 2;
        break;
    }
    switch (arg->cfg->modem->unk8) {
    default:
    case 1:
        c.unk24 = 1;
        break;
    case 2:
        c.unk24 = 2;
        break;
    case 0:
        c.unk24 = 0;
        break;
    }
    switch (arg->cfg->auth->unk0) {
    default:
        c.unk2 = 0;
        break;
    case 1:
        c.unk2 = 1;
        break;
    }
    if (arg->cfg->auth->unk2 != 1) {
        c.str18[0] = 0;
    }
    switch (*arg->cfg->line) {
    case 0:
        c.unk27 = 0;
        break;
    case 1:
        c.unk27 = 1;
        break;
    case 2:
        c.unk27 = 2;
        break;
    case 3:
        c.unk27 = 4;
        break;
    }
    {
        s16 r = AvePpp_Connect(&c);

        gNetIfState = 2;
        return r;
    }
}

s32 NetPpp_SetConnected(s32 unused0)
{
    gNetIfError = 0;
    gNetIfState = 3;
    gNetIfStarted[3] = 1;
    return 0;
}

s32 NetPpp_SetClosed(s32 unused0)
{
    gNetIfStarted[3] = 0;
    gNetIfState = 4;
    return 0;
}

/* The connection's details from the AVE wrapper (AvePpp_GetStatus); values it
 * doesn't know become 8 and 11. */
s32 NetPpp_GetStatus(NetConnInfo *out)
{
    AveInfo info;
    s32 r = AvePpp_GetStatus(&info);

    if (r < 0) {
        return r;
    }
    out->unk4 = info.unk0;
    out->unk0 = info.unk4;
    switch (info.phase) {
    case 0:
        out->phase = 0;
        break;
    case 1:
        out->phase = 1;
        break;
    case 2:
        out->phase = 2;
        break;
    case 3:
        out->phase = 3;
        break;
    case 4:
        out->phase = 4;
        break;
    case 5:
        out->phase = 5;
        break;
    case 6:
        out->phase = 6;
        break;
    case 7:
        out->phase = 7;
        break;
    default:
        out->phase = 8;
        break;
    case 9:
        out->phase = 9;
        break;
    }
    out->speed = info.speed;
    out->unkE = info.unk10;
    switch (info.error) {
    case 0:
        out->error = 0;
        break;
    case 1:
        out->error = 1;
        break;
    case 2:
        out->error = 2;
        break;
    case 3:
        out->error = 3;
        break;
    case 4:
        out->error = 4;
        break;
    case 5:
        out->error = 5;
        break;
    case 6:
        out->error = 6;
        break;
    case 7:
        out->error = 7;
        break;
    case 8:
        out->error = 8;
        break;
    case 9:
        out->error = 9;
        break;
    case 10:
        out->error = 10;
        break;
    default:
        out->error = 11;
        break;
    case 12:
        out->error = 12;
        break;
    case 13:
        out->error = 13;
        break;
    }
    return r;
}

s32 NetPpp_Disconnect(s32 unused0)
{
    s16 r = AvePpp_Disconnect();

    gNetIfState = 5;
    return r;
}

s32 NetPpp_Term(void)
{
    s16 r = AvePpp_Term();

    gNetIfStarted[1] = 0;
    return r;
}

s32 NetEth_Init(void)
{
    s16 r = AveEth_Init();

    gNetIfStarted[2] = 1;
    return r;
}

s32 NetEth_SetAddress(s32 *p)
{
    s32 b = p[1];
    s32 a = p[0];

    return -(AveIf_SetAddress(a, b, a | ~(a & b)) < 0);
}

s32 NetEth_Start(s32 a)
{
    s16 r = AveEth_Start(a);

    gNetIfState = 2;
    return r;
}

s32 NetEth_IsLinkUp(void)
{
    return AveEth_IsLinkUp();
}

s32 NetEth_SetConnected(s32 unused0)
{
    gNetIfError = 0;
    gNetIfState = 3;
    gNetIfStarted[3] = 1;
    return 0;
}

s32 NetEth_ClearAddress(s32 unused0)
{
    AveIf_ClearAddress();
    gNetIfStarted[3] = 0;
    gNetIfState = 4;
    return 0;
}

s32 NetEth_GetOpt8_Unk(void *buf)
{
    return -(AveIf_GetOption_Unk(8, 4, buf) != 4);
}

s32 NetEth_Stop(void)
{
    s16 r = AveEth_Stop();

    gNetIfState = 5;
    return r;
}

s32 NetEth_Term(void)
{
    s16 r = AveEth_Term();

    gNetIfStarted[2] = 0;
    return r;
}

s32 NetRoute_Add(s32 a, s32 b)
{
    s16 r = AveRoute_Add(a, b);

    if (r < 0) {
        return -1;
    }
    gNetIfStarted[5] = 1;
    return r;
}

s32 NetRoute_Delete(s32 a, s32 b)
{
    s16 r = AveRoute_Delete(a, b);

    if (r < 0) {
        return -1;
    }
    gNetIfStarted[5] = 0;
    return r;
}

s32 NetDhcp_Init(void)
{
    s16 r = AveDhcp_Init();

    gNetIfStarted[6] = 1;
    return r;
}

s32 NetDhcp_Term(void)
{
    AveDhcp_Term();
    gNetIfStarted[6] = 0;
    return 0;
}

s32 NetDhcp_Start(s32 a, s32 b)
{
    return AveDhcp_Start(a, b);
}

s32 NetDhcp_GetState(void)
{
    return AveDhcp_GetState();
}

s32 NetDhcp_GetAddress(s32 *p)
{
    s32 out;

    return AveDhcp_GetAddress(p, p + 1, &out);
}

s32 NetDhcp_GetDns(char *name, s32 *b)
{
    return AveDhcp_GetDns(name, b, b + 1);
}

s32 NetDhcp_GetRouter(s32 *out)
{
    return AveDhcp_GetRouter(out);
}

s32 NetDhcp_Release(s32 *p, s32 b)
{
    return AveDhcp_Release(*p, b);
}

s32 NetIf_PollEvent_Unk(s32 *out)
{
    s32 r;

    if (gNetEventPending != 0) {
        *out = gNetPendingEvent;
        return 1;
    }
    r = AveIf_PollEvent_Unk(out);
    if (r != 0) {
        gNetEventPending = 1;
        gNetPendingEvent = *out;
    }
    return r;
}

s32 NetIf_GetEvent_Unk(s32 a, s32 *b)
{
    s32 c;
    s32 d;

    gNetEventPending = 0;
    gNetPendingEvent = 0;
    return AveIf_GetEvent_Unk(a, b, &c, &d);
}

s32 NetIf_AckEvent_Unk(s32 a)
{
    return AveIf_AckEvent_Unk(a);
}

void NetSock_Lock(void)
{
    WaitSema(gNetSockSema);
}

void NetSock_Unlock(void)
{
    SignalSema(gNetSockSema);
}
