/*
 * NETWORK: the 69xx requests of the server list (task 9, unk_70A230.c), the
 * patch the server sends at login (6121 announces it, 6122 carries it, 6125
 * has its byte sum checked), and the first level (62xx): task 2 (6203's
 * count, then per entry the requests its flags ask for), the entry
 * requests' senders, replies and getters (gLobbyNet.list0), 6207 (enter),
 * 6306 (leave).
 */
#include "common.h"
#include "lobby_net.h"


char *strcpy(char *dst, const char *src);      /* strcpy */
s32 strncpy(s32, s32, u32);             /* strncpy */
u16 Swap16(s32);
void NetList_ReadValues(NetListEntry *e);
s32 LobbyPkt_ListCount(u16 kind);
s32 LobbyPkt_EntryName(u16 kind, u16 id);
s32 LobbyPkt_EntryValues(u16 kind, u16 id);
s32 LobbyPkt_EntryByte(u16 kind, u16 id);
s32 LobbyPkt_EntryText(u16 kind, u16 id);
s32 LobbyPkt_Leave(u16 kind);
void NetList_Leave(u8 level);
void NetList_Enter(u8 level);
void NetPatch_ReadHeader(void);
void NetPatch_ReadData(void);
s32 LobbyPkt_6124(u16 arg0);
s32 LobbyPkt_6125(void);
s32 NetPatch_CheckSum(u8 *buf, u32 n, u32 sum);
void NetTask2_Run(s32 task);
void NetTask2_Steps(NetTask *t);
void NetTask2_Done(NetTask *t);
void NetTask2_Failed(NetTask *t);
void NetTask2_Step0(NetTask *t);
void NetTask2_Step1(NetTask *t);
void NetTask2_Step2(NetTask *t);
void NetTask2_Step3(NetTask *t);
void NetTask2_Step4(NetTask *t);
void NetTask2_Step5(NetTask *t);
void NetTask2_Step1Reply(NetMsg m, NetMsg *pm);
void NetTask2_Step4Reply(NetMsg m, NetMsg *pm);
s32 LobbyReq_6203(void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_6204(u16 id, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_6205(u16 id, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_6206(u16 id, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_620A(u16 id, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_620B(u16 id, void (*onReply)(NetMsg, NetMsg *)); /* unit's own declaration: with the definition's s32 (s32 v, void (*onReply)(NetMsg, NetMsg *)) (lobby/unk_70BFE0.c), NetTask2_Step4 1 of 81 words differ */
void LobbyMsg_ReadU16Str(void *a, void *b);
void LobbyMsg_ReadU16Str2(void *a, void *b);
u8 *LobbyMsg_RecvU16(void *dst);
u8 *LobbyMsg_ReadU16U8(void *a, void *b);
void LobbyMsg_ReadErrText(void);
s32 NetReq_CountFree(void);
void NetReq_ClearAll(void);

/* 6911 */
s32 LobbyPkt_6911(void)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0xDF) & 0xFFFF;
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* 6912: u8 */
void LobbyPkt_6912(u8 v)
{
    LobbyPkt_BeginCmd(&gLobbyPkt, 0xE1);
    LobbyPkt_PutU8(&gLobbyPkt, v);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
}

/* 6913: u8 server */
void LobbyPkt_6913(u8 v)
{
    LobbyPkt_BeginCmd(&gLobbyPkt, 0xE5);
    LobbyPkt_PutU8(&gLobbyPkt, v);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
}

/* 6917: u8 server */
void LobbyPkt_6917(u8 v)
{
    LobbyPkt_BeginCmd(&gLobbyPkt, 0xE7);
    LobbyPkt_PutU8(&gLobbyPkt, v);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
}

/* 6914 */
void LobbyPkt_6914(void)
{
    LobbyPkt_BeginCmd(&gLobbyPkt, 0xE3);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
}

/* 6915 */
void LobbyPkt_6915(void)
{
    LobbyPkt_BeginCmd(&gLobbyPkt, 0xEA);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
}

/* 6916 */
void LobbyPkt_6916(void)
{
    LobbyPkt_BeginCmd(&gLobbyPkt, 0xEC);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
}

/* the notification 6918 (sent when task 9 fails) */
s32 LobbyPkt_6918(void)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0xE9) & 0xFFFF;
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

s32 LobbyNet_GetParty(NetParty *dst)
{
    *dst = gLobbyNet.party;
    return 0;
}

/* gLobbyNet.partyAddr and partyPort, byte by byte: as fields the code
 * differs (the base address is kept in a register) */

/* the address 6916 gave: the IPv4 address (as a little-endian word) and the port.
 * Each byte is masked to its place (gLobbyNet.partyAddr[0] & 0xFF too: without it MWCC loads
 * gLobbyNet.partyAddr[1] first). */
void LobbyNet_GetPartyAddr(s32 *addr, s16 *port)
{
    *addr = (gLobbyNet.partyAddr[0] & 0xFF) | ((gLobbyNet.partyAddr[1] << 8) & 0xFF00) | ((gLobbyNet.partyAddr[2] << 16) & 0xFF0000) |
            ((gLobbyNet.partyAddr[3] << 24) & 0xFF000000);
    *port = Swap16((gLobbyNet.partyPort[1] + (gLobbyNet.partyPort[0] << 8)) & 0xFFFF);
}
/* the notification 6121 at login: a patch follows (task 0) */
void LobbyNotify_6121(void)
{
    if (gLobbyNet.task[0].active != 0) {
        if (gLobbyNet.flag == 0x10) {
            NetPatch_ReadHeader();
            gLobbyNet.unk1290 = 0;
            gLobbyNet.unk1294 = 0;
            gLobbyNet.patchDst = gLobbyNet.login.patchBuf;
        } else if (gLobbyNet.flag == 2) {
            return;
        }
    }
}

/* 6121's payload: string (a 4-character tag and 10 characters of version),
 * u16, u32 size, u32 checksum */
void NetPatch_ReadHeader(void)
{
    u8 buf[0x18];
    u8 *p;

    memset(buf, 0, 0x18);
    p = LobbyMsg_GetString(buf, gLobbyRecvData);
    p = LobbyMsg_GetU16(&gLobbyNet.patchUnk, p);
    p = LobbyMsg_GetU32(&gLobbyNet.patchSize, p);
    LobbyMsg_GetU32(&gLobbyNet.patchSum, p);
    memset(gLobbyNet.patchTag, 0, 8);
    memcpy(gLobbyNet.patchTag, buf, 4);
    memset(gLobbyNet.patchVersion, 0, 0x10);
    memcpy(gLobbyNet.patchVersion, buf + 4, 10);
}

/* the notification 6122: a part of the patch */
void LobbyNotify_6122(void)
{
    if (gLobbyNet.task[0].active != 0) {
        NetPatch_ReadData();
    }
}

/* 6122's payload: u16, u16 n, n bytes appended to the patch */
void NetPatch_ReadData(void)
{
    u16 n;
    u16 x;
    u8 *p;

    p = LobbyMsg_GetU16(&n, LobbyMsg_GetU16(&x, gLobbyRecvData));
    LobbyMsg_GetBytes(gLobbyNet.patchDst, p, n);
    gLobbyNet.patchDst += n;
}

/* the server's request 6124: u16, answered with the same u16 */
void LobbySrvReq_6124(void)
{
    u16 v;

    if (gLobbyNet.task[0].active != 0) {
        LobbyMsg_RecvU16(&v);
        LobbyPkt_6124(v);
    }
}

/* the reply to 6124: u16 */
s32 LobbyPkt_6124(u16 v)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0xFE);
    LobbyPkt_PutU16(&gLobbyPkt, v);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the notification 6123 */
void LobbyNotify_6123(void)
{
}

/* the server's request 6125: the patch is whole; its checksum decides
 * (task 0's onDone: kind 0, 3; a bad sum: kind -1, 9) */
void LobbySrvReq_6125(void)
{
    NetMsg m;

    if (gLobbyNet.task[0].active != 0) {
        if (NetPatch_CheckSum(gLobbyNet.login.patchBuf, gLobbyNet.patchSize, gLobbyNet.patchSum)) {
            m.kind = 0;
            m.flag = 3;
            gLobbyNet.task[0].onDone(m, &m);
        } else {
            m.kind = -1;
            m.flag = 9;
            gLobbyNet.task[0].onDone(m, &m);
        }
    }
}

/* the reply to 6125 */
s32 LobbyNet_Send6125(void)
{
    LobbyPkt_6125();
    return 0;
}

s32 LobbyPkt_6125(void)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x100) & 0xFFFF;
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the patch's size, version and tag */
s32 LobbyNet_GetPatchInfo(NetPatchInfo *info)
{
    memset(info, 0, 0x1C);
    strncpy((s32)info->version, (s32)gLobbyNet.patchVersion, 0xA);
    strncpy((s32)info->tag, (s32)gLobbyNet.patchTag, 4);
    info->size = gLobbyNet.patchSize;
    return 0;
}

/* 1 if the bytes of buf add up to sum */
s32 NetPatch_CheckSum(u8 *buf, u32 n, u32 sum)
{
    u32 i;
    u32 s = 0;

    for (i = 0; i < n; i++) {
        s += *buf++;
    }
    return sum == s;
}

/* starts task 2: the first level's list, with the information flags asks for
 * (bits: 1 6205, 2 6206, 4 6204, 8 620A, 0x400 620B) */
s32 NetTask2_Start(s32 unused, s32 flags, void (*onDone)(NetMsg, NetMsg *))
{
    NetTask *t = &gLobbyNet.task[2];

    if (t->active == 0) {
        memset(t, 0, sizeof(NetTask));
        t->active = 1;
        t->onDone = onDone;
        t->func = NetTask2_Run;
        t->reqFlags = flags & 0x40F;
        return 0;
    }
    return -1;
}

void NetTask2_Run(s32 task)
{
    NetTask *t = &gLobbyNet.task[2];

    if (t->active != 0) {
        switch (t->state) {
        case 0:
            NetTask2_Steps(t);
            break;
        case 1:
            NetTask2_Done(t);
            break;
        case 2:
            NetTask2_Failed(t);
            break;
        }
    }
}

void NetTask2_Steps(NetTask *t)
{
    switch (t->step) {
    case 0:
        NetTask2_Step0(t);
        return;
    case 1:
        NetTask2_Step1(t);
        return;
    case 2:
        NetTask2_Step2(t);
        return;
    case 3:
        NetTask2_Step3(t);
        return;
    case 4:
        NetTask2_Step4(t);
        return;
    case 5:
        NetTask2_Step5(t);
    default:
        return;
    }
}

void NetTask2_Done(NetTask *t)
{
    NetMsg m;

    t->active = 0;
    t->state = 0;
    m.kind = 0;
    t->onDone(m, &m);
}

void NetTask2_Failed(NetTask *t)
{
    NetMsg m;

    t->active = 0;
    t->state = 0;
    m.kind = -1;
    t->onDone(m, &m);
}

/* task 2, step 0: counts the requests per entry, clears the list */
void NetTask2_Step0(NetTask *t)
{
    u32 flags;
    u32 i;

    t->step++;
    t->idx = 1;
    t->pos = 1;
    t->reply = 0;
    flags = t->reqFlags;
    t->reqCount = 0;
    for (i = 0; i < 0x20; i++) {
        if (flags & 1) {
            t->reqCount++;
        }
        flags >>= 1;
    }
    gLobbyNet.nReplies = 0;
    gLobbyNet.count0 = 0;
    memset(gLobbyNet.list0, 0, 0x26AC);
}

/* task 2, step 1: 6203 (the count) */
void NetTask2_Step1(NetTask *t)
{
    if (NetReq_CountFree() > 0) {
        t->step++;
        LobbyReq_6203(NetTask2_Step1Reply);
    }
}

/* task 2, step 2: 6203's reply; tells onDone how many replies to expect (kind 2) */
void NetTask2_Step2(NetTask *t)
{
    NetMsg m;

    if (t->reply == 1) {
        t->step++;
        t->reply = 0;
        gLobbyNet.nReplies = gLobbyNet.count0 * t->reqCount;
        m.kind = 2;
        m.flag = 10;
        t->onDone(m, &m);
    } else if (t->reply == 2) {
        t->state = 2;
        t->step = 0;
    }
}

/* task 2, step 3 */
void NetTask2_Step3(NetTask *t)
{
    t->step++;
    t->reply = 0;
    if (gLobbyNet.count0 == 0) {
        t->state = 1;
        t->step = 0;
    }
}

/* task 2, step 4: the requests of one entry, when enough request slots are free */
void NetTask2_Step4(NetTask *t)
{
    u32 id;

    if (t->reply == 2) {
        t->state = 2;
        t->step = 0;
        NetReq_ClearAll();
        return;
    }
    id = t->pos;
    if (NetReq_CountFree() >= t->reqCount) {
        if (t->reqFlags & 1) {
            LobbyReq_6205(id, NetTask2_Step4Reply);
        }
        if (t->reqFlags & 2) {
            LobbyReq_6206(id, NetTask2_Step4Reply);
        }
        if (t->reqFlags & 4) {
            LobbyReq_6204(id, NetTask2_Step4Reply);
        }
        if (t->reqFlags & 8) {
            LobbyReq_620A(id, NetTask2_Step4Reply);
        }
        if (t->reqFlags & 0x400) {
            LobbyReq_620B(id, NetTask2_Step4Reply);
        }
        t->pos++;
        if (id + 1 > gLobbyNet.count0 || id + 1 > 0x19) {
            t->step++;
        }
    }
}

/* task 2, step 5: done once every entry has all the information asked for */
void NetTask2_Step5(NetTask *t)
{
    s32 ok = 1;
    u32 j;
    u32 i = 0;

    for (j = 0; j < 0x19; j++) {
        i++;
        if (i > gLobbyNet.count0) {
            break;
        }
        if ((t->reqFlags & gLobbyNet.list0[i - 1].flags) != t->reqFlags) {
            ok = 0;
            break;
        }
    }
    if (t->reply == 2) {
        t->state = 2;
        NetReq_ClearAll();
    } else if (ok) {
        t->state = 1;
        t->step = 0;
    }
}

/* task 2's reply callback for 6203 */
void NetTask2_Step1Reply(NetMsg m, NetMsg *pm)
{
    NetTask *t = &gLobbyNet.task[2];

    if (m.kind == 0) {
        t->reply = 1;
    } else {
        t->reply = 2;
    }
}

/* task 2's reply callback for an entry's requests: each reply is reported to onDone (kind 2) */
void NetTask2_Step4Reply(NetMsg m, NetMsg *pm)
{
    NetTask *t = &gLobbyNet.task[2];
    NetMsg n;

    if (m.kind == 0) {
        t->reply = 1;
        n.kind = 2;
        n.flag = 11;
        t->onDone(n, &n);
    } else {
        t->reply = 2;
    }
}

/* 6203 (the count) */
s32 LobbyReq_6203(void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_ListCount(0);
        return i;
    }
    return -1;
}

s32 NetList0_GetCount(u16 *out)
{
    *out = gLobbyNet.count0;
    return 0;
}

/* the reply to 6203: u16 count */
void LobbyReply_6203(void)
{
    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_RecvU16(&gLobbyNet.count0);
    }
    Lobby_RouteMessage(0);
}

/* 6204 for entry id */
s32 LobbyReq_6204(u16 id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_EntryName(0, id);
        return i;
    }
    return -1;
}

s32 NetList0_GetName(u16 id, char *dst)
{
    strcpy(dst, gLobbyNet.list0[id - 1].name);
    return 0;
}

/* the reply to 6204: u16 id, string; route 0x10 */
void LobbyReply_6204(void)
{
    char name[0x4C];
    u16 id;

    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU16Str(&id, name);
        gLobbyNet.list0[id - 1].id = id;
        strcpy(gLobbyNet.list0[id - 1].name, name);
        gLobbyNet.lastId = id;
        gLobbyNet.list0[id - 1].flags |= 4;
    }
    Lobby_RouteMessage(0x10);
}

/* 6205 for entry id */
s32 LobbyReq_6205(u16 id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_EntryValues(0, id);
        return i;
    }
    return -1;
}

s32 NetList0_GetUsers(u16 id, u16 *a, u16 *b)
{
    *a = gLobbyNet.list0[id - 1].users;
    *b = gLobbyNet.list0[id - 1].questUsers;
    return 0;
}

s32 NetList0_GetMaxUsers(u16 id, u16 *out)
{
    *out = gLobbyNet.list0[id - 1].maxUsers;
    return 0;
}

s32 NetList0_GetValue10(u16 id, u16 *out)
{
    *out = gLobbyNet.list0[id - 1].unk10;
    return 0;
}

/* the reply to 6205 and its notification; route 0x11 */
void LobbyReply_6205(void)
{
    if (gLobbyNet.replyErr == 0) {
        NetList_ReadValues(gLobbyNet.list0);
    }
    Lobby_RouteMessage(0x11);
}

/* 6206 for entry id */
s32 LobbyReq_6206(u16 id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_EntryByte(0, id);
        return i;
    }
    return -1;
}

s32 NetList0_GetByte(u16 id, u8 *out)
{
    *out = gLobbyNet.list0[id - 1].state;
    return 0;
}

/* the reply to 6206 and its notification: u16 id, u8; route 0x12 */
void LobbyReply_6206(void)
{
    u16 id;
    u8 v;

    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU16U8(&id, &v);
        gLobbyNet.list0[id - 1].id = id;
        gLobbyNet.list0[id - 1].state = v;
        gLobbyNet.lastId = id;
        gLobbyNet.list0[id - 1].flags |= 2;
    }
    Lobby_RouteMessage(0x12);
}

/* 620A for entry id */
s32 LobbyReq_620A(u16 id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_EntryText(0, id);
        return i;
    }
    return -1;
}

s32 NetList0_GetText(u16 id, char *dst)
{
    strcpy(dst, gLobbyNet.list0[id - 1].text);
    return 0;
}

/* the reply to 620A and its notification: u16 id, string; route 0x13 */
void LobbyReply_620A(void)
{
    char text[0x10C];
    u16 id;

    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU16Str2(&id, text);
        gLobbyNet.list0[id - 1].id = id;
        strcpy(gLobbyNet.list0[id - 1].text, text);
        gLobbyNet.list0[id - 1].flags |= 8;
    }
    Lobby_RouteMessage(0x13);
}

/* the reply to 6207: the first-level entry was entered */
void LobbyReply_6207(void)
{
    if (gLobbyNet.replyErr == 0) {
        NetList_Enter(0);
    }
    Lobby_RouteMessage(0);
}

/* 6306: leaves the first-level entry */
s32 LobbyReq_6306(void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_Leave(0);
        return i;
    }
    return -1;
}

/* the reply to 6306 */
void LobbyReply_6306(void)
{
    if (gLobbyNet.replyErr == 0) {
        NetList_Leave(0);
    }
    Lobby_RouteMessage(0);
}

/* the notification 6307: an error text; route 8 */
void LobbyNotify_6307(void)
{
    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadErrText();
        Lobby_RouteMessage(8);
    }
}
