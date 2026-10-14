/*
 * NETWORK: the third-level entry records of task 6 (gLobbyNet.recs2), one
 * request per field: 6604 name, 6607 sub-entry count, 6606, 6605 (whether
 * task 7 may send the record's byte), and per sub-entry 6608 name and 660E
 * values; also 6409, 6412/6413/6414 and 660F.
 */
#include "common.h"
#include "lobby_net.h"
char *strcpy(char *, const char *);

u8 *LobbyMsg_ReadU8Str(void *a, void *b);
u8 *LobbyMsg_ReadU8U8(void *a, void *b);
u8 *LobbyMsg_ReadU16U16(void *a, void *b);
u8 *LobbyMsg_ReadU8U8Str(void *a, void *b, void *c);
s32 memcmp(void *a, void *b, u32 n);  /* memcmp */
s32 LobbyPkt_6604(u16 id, u8 i);
s32 LobbyPkt_6607(u16 id, u8 i);
s32 LobbyPkt_6606(u16 id, u8 i);
s32 LobbyPkt_6605(u16 id, u8 i);
s32 LobbyPkt_6608(u16 id, u8 i, u8 j);
s32 LobbyPkt_660E(u16 id, u8 i, u8 j);
void LobbyMsg_ReadU8U8Rest(u8 *i, u8 *j, u8 **p);
s32 LobbyPkt_6412(u16 id);
s32 LobbyPkt_660F(u16 id);

/* 6604: u16 entry, u8 record */
s32 LobbyPkt_6604(u16 id, u8 i)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0xB2);
    LobbyPkt_PutU16(&gLobbyPkt, id);
    LobbyPkt_PutU8(&gLobbyPkt, i);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 6604: u8 record, string name */
void LobbyReply_6604(void)
{
    char name[0x4C];
    u8 i;

    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU8Str(&i, name);
        strcpy(gLobbyNet.recs2.rec[i].name, name);
        gLobbyNet.recs2.rec[i].flags |= 1;
    }
    Lobby_RouteMessage(0);
}

/* 6607 for record i */
s32 LobbyReq_6607(u16 id, u8 i, void (*onReply)(NetMsg, NetMsg *))
{
    s32 k;

    k = NetReq_Alloc(1, NULL, onReply);
    if (k != -1) {
        gLobbyNet.req[k].seq = LobbyPkt_6607(id, i);
        return k;
    }
    return -1;
}

/* 6607: u16 entry, u8 record */
s32 LobbyPkt_6607(u16 id, u8 i)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0xB8);
    LobbyPkt_PutU16(&gLobbyPkt, id);
    LobbyPkt_PutU8(&gLobbyPkt, i);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 6607: u8 record, u8 sub-entries */
void LobbyReply_6607(void)
{
    u8 i;
    u8 v;

    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU8U8(&i, &v);
        gLobbyNet.recs2.rec[i].m = v;
        gLobbyNet.recs2.rec[i].flags |= 4;
    }
    Lobby_RouteMessage(0);
}

/* 6606 for record i */
s32 LobbyReq_6606(u16 id, u8 i, void (*onReply)(NetMsg, NetMsg *))
{
    s32 k;

    k = NetReq_Alloc(1, NULL, onReply);
    if (k != -1) {
        gLobbyNet.req[k].seq = LobbyPkt_6606(id, i);
        return k;
    }
    return -1;
}

/* 6606: u16 entry, u8 record */
s32 LobbyPkt_6606(u16 id, u8 i)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0xB6);
    LobbyPkt_PutU16(&gLobbyPkt, id);
    LobbyPkt_PutU8(&gLobbyPkt, i);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 6606: u8 record, u8 */
void LobbyReply_6606(void)
{
    u8 i;
    u8 v;

    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU8U8(&i, &v);
        gLobbyNet.recs2.rec[i].unk44 = v;
        gLobbyNet.recs2.rec[i].flags |= 2;
    }
    Lobby_RouteMessage(0);
}

/* 6605 for record i */
s32 LobbyReq_6605(u16 id, u8 i, void (*onReply)(NetMsg, NetMsg *))
{
    s32 k;

    k = NetReq_Alloc(1, NULL, onReply);
    if (k != -1) {
        gLobbyNet.req[k].seq = LobbyPkt_6605(id, i);
        return k;
    }
    return -1;
}

/* 6605: u16 entry, u8 record */
s32 LobbyPkt_6605(u16 id, u8 i)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0xB4);
    LobbyPkt_PutU16(&gLobbyPkt, id);
    LobbyPkt_PutU8(&gLobbyPkt, i);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 6605: u8 record, u8 */
void LobbyReply_6605(void)
{
    u8 i;
    u8 v;

    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU8U8(&i, &v);
        gLobbyNet.recs2.rec[i].unk42 = v;
        gLobbyNet.recs2.rec[i].flags |= 8;
    }
    Lobby_RouteMessage(0);
}

/* 6608 for sub-entry j of record i */
s32 LobbyReq_6608(u16 id, u8 i, u8 j, void (*onReply)(NetMsg, NetMsg *))
{
    s32 k;

    k = NetReq_Alloc(1, NULL, onReply);
    if (k != -1) {
        gLobbyNet.req[k].seq = LobbyPkt_6608(id, i, j);
        return k;
    }
    return -1;
}

/* 6608: u16 entry, u8 record, u8 sub-entry */
s32 LobbyPkt_6608(u16 id, u8 i, u8 j)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0xBA);
    LobbyPkt_PutU16(&gLobbyPkt, id);
    LobbyPkt_PutU8(&gLobbyPkt, i);
    LobbyPkt_PutU8(&gLobbyPkt, j);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 6608: u8 record, u8 sub-entry, string name */
void LobbyReply_6608(void)
{
    u8 i;
    u8 j;
    char name[0x4C];

    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU8U8Str(&i, &j, name);
        strcpy(gLobbyNet.recs2.rec[i].subName[j], name);
        gLobbyNet.recs2.rec[i].subFlags[j] |= 1;
    }
    Lobby_RouteMessage(0);
}

/* 660E for sub-entry j of record i */
s32 LobbyReq_660E(u16 id, u8 i, u8 j, void (*onReply)(NetMsg, NetMsg *))
{
    s32 k;

    k = NetReq_Alloc(1, NULL, onReply);
    if (k != -1) {
        gLobbyNet.req[k].seq = LobbyPkt_660E(id, i, j);
        return k;
    }
    return -1;
}

/* 660E: u16 entry, u8 record, u8 sub-entry */
s32 LobbyPkt_660E(u16 id, u8 i, u8 j)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0xBC);
    LobbyPkt_PutU16(&gLobbyPkt, id);
    LobbyPkt_PutU8(&gLobbyPkt, i);
    LobbyPkt_PutU8(&gLobbyPkt, j);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 660E: u8 record, u8 sub-entry, u8 n, n three-byte values */
void LobbyReply_660E(void)
{
    u8 i;
    u8 j;
    u8 *p;
    s32 k;
    s32 n;

    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU8U8Rest(&i, &j, &p);
        n = *p++;
        for (k = 0; k < n; k++) {
            gLobbyNet.recs2.rec[i].subData[j][k][0] = p[0];
            gLobbyNet.recs2.rec[i].subData[j][k][1] = p[1];
            gLobbyNet.recs2.rec[i].subData[j][k][2] = p[2];
            p += 3;
        }
        gLobbyNet.recs2.rec[i].subN[j] = n;
        gLobbyNet.recs2.rec[i].subFlags[j] |= 2;
    }
    Lobby_RouteMessage(0);
}

/* reads u8 i, u8 j of the message received; *p: what follows */
void LobbyMsg_ReadU8U8Rest(u8 *i, u8 *j, u8 **p)
{
    *p = LobbyMsg_GetU8(j, LobbyMsg_GetU8(i, gLobbyRecvData));
}

/* the reply to 6409: u16, u16 */
void LobbyReply_6409(void)
{
    u16 x;

    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU16U16(&x, &gLobbyNet.x6409);
    }
    Lobby_RouteMessage(0);
}

/* 6412 for third-level entry id */
s32 LobbyReq_6412(u16 id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_6412(id);
        return i;
    }
    return -1;
}

/* the two values 6412 gave entry id */
s32 NetList2_Get6412(s32 id, u16 *a, u16 *b)
{
    if (id != 0) {
        *a = gLobbyNet.list2[(u16)id - 1].roomMembers;
        *b = gLobbyNet.list2[(u16)id - 1].unk1C;
    }
    return 0;
}

/* the reply to 6412 and its notification: u16 id, u16, u16; route 0x26 */
void LobbyReply_6412(void)
{
    u16 id;
    u8 *p;

    if (gLobbyNet.replyErr == 0) {
        p = LobbyMsg_GetU16(&id, gLobbyRecvData);
        p = LobbyMsg_GetU16(&gLobbyNet.list2[id - 1].roomMembers, p);
        LobbyMsg_GetU16(&gLobbyNet.list2[id - 1].unk1C, p);
        gLobbyNet.list2[id - 1].flags |= 0x40;
        gLobbyNet.lastId = id;
    }
    Lobby_RouteMessage(0x26);
}

/* the same two values for the third-level entry the player is in */
void LobbyNotify_6506(void)
{
    s32 id;
    u8 *p;

    if (gLobbyNet.replyErr == 0) {
        id = gLobbyNet.cur[2];
        p = LobbyMsg_GetU16(&gLobbyNet.list2[id - 1].roomMembers, gLobbyRecvData);
        LobbyMsg_GetU16(&gLobbyNet.list2[id - 1].unk1C, p);
        gLobbyNet.lastId = id;
    }
    Lobby_RouteMessage(0x26);
}

/* 6412: u16 id */
s32 LobbyPkt_6412(u16 id)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0xD9);
    LobbyPkt_PutU16(&gLobbyPkt, id);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 6413: u16, u8 n, n x (blob id, u8): the byte of each user of
 * users2 with that id */
void LobbyReply_6413(void)
{
    u8 n;
    u8 v;
    u16 x;
    char id[8];
    s32 k;
    s32 i;
    u8 *p;

    if (gLobbyNet.replyErr == 0) {
        p = LobbyMsg_GetU8(&n, LobbyMsg_GetU16(&x, gLobbyRecvData));
        if (n) {
            for (i = 0; i < n; i++) {
                memset(id, 0, 8);
                p = LobbyMsg_GetU8(&v, LobbyMsg_GetBlob(id, 8, p));
                for (k = 0; k < 10; k++) {
                    if (memcmp(id, gLobbyNet.users2[k].u.id, 8) == 0) {
                        gLobbyNet.users2[k].unk11C[0] = v;
                    }
                }
            }
        }
    }
    Lobby_RouteMessage(0);
}

/* the notification 6414: u16 entry, blob id, u8: one user of the entry the
 * player is in; route 0x27 */
void LobbyNotify_6414(void)
{
    u16 id;
    u8 *p;
    NetUserRec *u;

    if (gLobbyNet.replyErr == 0) {
        memset(&gLobbyNet.user6414, 0, sizeof(NetUserRec));
        p = LobbyMsg_GetU16(&id, gLobbyRecvData);
        if (id != gLobbyNet.cur[2]) {
            return;
        }
        u = gLobbyNet.users2;
        gLobbyNet.nUsers2 = 1;
        LobbyMsg_GetU8(u->unk11C, LobbyMsg_GetBlob(u->u.id, 8, p));
    }
    Lobby_RouteMessage(0x27);
}

/* 660F for third-level entry id */
s32 LobbyReq_660F(u16 id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_660F(id);
        return i;
    }
    return -1;
}

/* 660F: u16 id */
s32 LobbyPkt_660F(u16 id)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0xAE);
    LobbyPkt_PutU16(&gLobbyPkt, id);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}
