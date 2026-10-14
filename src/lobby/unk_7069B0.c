/*
 * NETWORK: requests 6311 and 6892, the connection's start and reset, the
 * player's own user record, and the server's 6101 and 6103 requests.
 */
#include "common.h"
#include "lobby_net.h"


extern char D_008385A0[];
extern u8 gLobby6138Data[];
void LobbyMsg_ReadU16U8b2(void *a, void *b);

char *strcpy(char *dst, const char *src);      /* strcpy */
char *strncpy(void *, void *, u32); /* strncpy */
s32 LobbyPkt_EntryFlag(u16 kind, u16 arg1);
s32 LobbyPkt_6892(void);
void LobbyNet_Reset(void);
u8 LobbyNet_GetCharaCount(void);
void LobbySrvReq_6101(void);
void LobbySrvReq_6103(void);
void LobbyPkt_6101(void);
void LobbyPkt_6103(void);
void LobbyPkt_6132(void);
s32 LobbyPkt_600E(void);
void LobbyPkt_61A0(void);

/* 6311 */
s32 LobbyPkt_6311(void)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x6F) & 0xFFFF;
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 6311 */
void LobbyReply_6311(void)
{
    Lobby_RouteMessage(0);
}

s32 LobbyReq_6892(void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_6892();
        return i;
    }
    return -1;
}

s32 LobbyNet_Get6892(Net6892 *dst)
{
    *dst = gLobbyNet.x6892;
    return 0;
}

/* the reply to 6892 and its notification */
void LobbyReply_6892(void)
{
    u8 *p;
    s32 i;
    s32 n;

    if (gLobbyNet.replyErr == 0) {
        p = LobbyMsg_GetU32(&gLobbyNet.x6892.time, gLobbyRecvData);
        p = LobbyMsg_GetU32(&gLobbyNet.x6892.dayTicks, p);
        p = LobbyMsg_GetU32(&gLobbyNet.x6892.day, p);
        p = LobbyMsg_GetU32(&gLobbyNet.x6892.season, p);
        p = LobbyMsg_GetU32(&gLobbyNet.x6892.n, p);
        n = gLobbyNet.x6892.n;
        for (i = 0; i < n; i++) {
            p = LobbyMsg_GetU32(&gLobbyNet.x6892.list[i], p);
        }
        LobbyMsg_GetU32(&gLobbyNet.x6892.unk44, p);
    }
    Lobby_RouteMessage(0x2F);
}

/* 6891 */
s32 LobbyPkt_6892(void)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x117) & 0xFFFF;
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

s32 LobbyReq_630E(u16 v, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_EntryFlag(1, v);
        return i;
    }
    return -1;
}

/* the reply to 630E and its notification: a room's id and a byte */
void LobbyReply_630E(void)
{
    u16 id;
    u8 v;

    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU16U8b2(&id, &v);
        gLobbyNet.list1[id - 1].id = id;
        gLobbyNet.list1[id - 1].kind = v;
        gLobbyNet.list1[id - 1].flags |= 0x400;
    }
    Lobby_RouteMessage(0x35);
}

/* resets the connection's state */
void LobbyNet_Reset(void)
{
    gLobbyNet.unk8 = 0;
    gLobbyNet.connected = 0;
    memset(gLobbyNet.req, 0, 0xE00);
    memset(gLobbyNet.task, 0, 0x280);
    memset(gLobbyNet.unk65D6, 0, 0xA);
    memset(gLobby6138Data, 0, 0x28);
}

/* starts the connection: -1 if task 0 is already active */
/* starts the connection (task 0) with the login parameters; -1 if it is active already */
s32 NetTask0_Start(NetLoginParams *p, void (*onDone)(NetMsg, NetMsg *))
{
    NetLoginParams params = *p;

    if (gLobbyNet.task[0].active == 0) {
        gLobbyNet.login = params;
        gLobbyNet.unk14 = 0;
        gLobbyNet.connected = 1;
        gLobbyNet.rttCount = 0;
        gLobbyNet.rttSum = 0;
        memset(&gLobbyNet.me, 0, 0x11C);
        memset(gLobbyNet.charas, 0, 0x470);
        gLobbyNet.rxLen = 0;
        gLobbyNet.rxSize = 0x1000;
        gLobbyNet.rxBuf = gLobbyNet.buf38C0C;
        memset(&gLobbyNet.x6105, 0, 0x1004);
        memset(gLobbyNet.buf38C0C, 0, 0x2000);
        gLobbyNet.task[0].onDone = onDone;
        gLobbyNet.task[0].active = 1;
        gLobbyNet.task[0].state = 0;
        gLobbyNet.task[0].func = NULL;
        return 0;
    }
    return -1;
}

s32 LobbyNet_Set6103(Net6103 *src)
{
    gLobbyNet.x6103 = *src;
    return 0;
}

/* sets the player's own record: id (NULL: none), name and text */
s32 LobbyNet_SetMe(char *id, char *name, u8 *text)
{
    u32 n;

    memset(&gLobbyNet.me, 0, 0x11C);
    strncpy(gLobbyNet.me.name, name, strlen(name));
    memcpy(gLobbyNet.me.text, text, 0x100);
    if (id == NULL) {
        strncpy(gLobbyNet.charas[3].id, D_008385A0, 6);
        memset(gLobbyNet.me.id, 0, 8);
    } else {
        strncpy(gLobbyNet.charas[3].id, id, 6);
        strncpy(gLobbyNet.me.id, id, 6);
    }
    n = strlen(name);
    strncpy(gLobbyNet.charas[3].name, name, n);
    gLobbyNet.charas[3].name[n] = 0;
    LobbyPkt_6132();
    return 0;
}

u8 LobbyNet_GetCharaCount(void)
{
    return gLobbyNet.nCharas;
}

s32 LobbyNet_GetCharaId(u8 i, char *dst)
{
    strcpy(dst, gLobbyNet.charas[i].id);
    return 0;
}

s32 LobbyNet_GetCharaName(u8 i, char *dst)
{
    strcpy(dst, gLobbyNet.charas[i].name);
    return 0;
}

s32 LobbyNet_GetCharaText(u8 i, u8 *dst)
{
    memcpy(dst, gLobbyNet.charas[i].text, 0x100);
    return 0;
}

s32 LobbyNet_GetMyCharaId(char *dst)
{
    strcpy(dst, gLobbyNet.charas[3].id);
    return 0;
}

s32 LobbyNet_GetMyCharaName(char *dst)
{
    strcpy(dst, gLobbyNet.charas[3].name);
    return 0;
}

/* the server's request 6101: the key for the blobs */
void LobbySrvReq_6101(void)
{
    LobbyMsg_GetU16(&gLobbyNet.key, gLobbyRecvData);
    LobbyPkt_6101();
    LobbyPkt_61A0();
}

/* the server's request 6103 */
void LobbySrvReq_6103(void)
{
    if (gLobbyNet.login.type != 0) {
        LobbyPkt_600E();
        return;
    }
    LobbyPkt_6103();
}

/* unk_7069B0's .rodata (0x8385A0-0x8385A8) */
#pragma rodata on
char D_008385A0[0x8] = {
#include "lobby/008385A0.inc"
};
#pragma rodata off
