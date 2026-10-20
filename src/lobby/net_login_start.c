/*
 * Requests 6311 and 6892, the connection's start (NetLoginTask_Start) and
 * reset, the player's own user record, and the server's 6101 and 6103
 * requests. Network code: it stays byte for byte, as the whole build does.
 */
#include "common.h"
#include "lobby_net.h"


extern char gNoUserId[];
extern u8 gLobby6138Data[];
void LobbyMsg_ReadU16U8b2(void *a, void *b);

char *strcpy(char *dst, const char *src);      /* strcpy */
char *strncpy(void *, void *, u32); /* strncpy */
s32 LobbyPkt_EntryKind(u16 kind, u16 arg1);
s32 LobbyPkt_TownClock(void);
void LobbyNet_Reset(void);
u8 LobbyNet_GetCharaCount(void);
void LobbySrvReq_LoginAuth(void);
void LobbySrvReq_ClientInfo(void);
void LobbyPkt_LoginAuth(void);
void LobbyPkt_ClientInfo(void);
void LobbyPkt_CharaSelect(void);
s32 LobbyPkt_Ping(void);
void LobbyPkt_LoginParams(void);

/* 6311 */
s32 LobbyPkt_Level1SettingsEnd(void)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL1_SETTINGS_END) & 0xFFFF;
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 6311 */
void LobbyReply_Level1SettingsEnd(void)
{
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/*
 * Sends 6892 as a request; onReply gets its reply. Returns the request's
 * slot, -1 if none is free.
 */
s32 LobbyReq_TownClock(void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_TownClock();
        return i;
    }
    return -1;
}

s32 LobbyNet_GetTownClock(NetTownClock *dst)
{
    *dst = gLobbyNet.townClock;
    return 0;
}

/* the reply to 6892 and its notification */
void LobbyReply_TownClock(void)
{
    u8 *cur;
    s32 i;
    s32 n;

    if (gLobbyNet.replyErr == 0) {
        cur = LobbyMsg_GetU32(&gLobbyNet.townClock.time, gLobbyRecvData);
        cur = LobbyMsg_GetU32(&gLobbyNet.townClock.dayTicks, cur);
        cur = LobbyMsg_GetU32(&gLobbyNet.townClock.day, cur);
        cur = LobbyMsg_GetU32(&gLobbyNet.townClock.season, cur);
        cur = LobbyMsg_GetU32(&gLobbyNet.townClock.n, cur);
        n = gLobbyNet.townClock.n;
        for (i = 0; i < n; i++) {
            cur = LobbyMsg_GetU32(&gLobbyNet.townClock.list[i], cur);
        }
        LobbyMsg_GetU32(&gLobbyNet.townClock.year, cur);
    }
    Lobby_RouteMessage(NET_ROUTE_TOWN_CLOCK);
}

/* 6891 */
s32 LobbyPkt_TownClock(void)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_TOWN_CLOCK) & 0xFFFF;
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/*
 * Sends 630E as a request; onReply gets its reply. Returns the request's
 * slot, -1 if none is free.
 */
s32 LobbyReq_Level1Kind(u16 value, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_EntryKind(1, value);
        return i;
    }
    return -1;
}

/* the reply to 630E and its notification: a room's id and a byte */
void LobbyReply_Level1Kind(void)
{
    u16 id;
    u8 value;

    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU16U8b2(&id, &value);
        gLobbyNet.list1[id - 1].id = id;
        gLobbyNet.list1[id - 1].kind = value;
        gLobbyNet.list1[id - 1].flags |= NET_ENTRY_KIND;
    }
    Lobby_RouteMessage(NET_ROUTE_LEVEL1_KIND);
}

/* resets the connection's state */
void LobbyNet_Reset(void)
{
    gLobbyNet.unk8 = 0;
    gLobbyNet.connected = 0;
    memset(gLobbyNet.req, 0, sizeof(gLobbyNet.req));
    memset(gLobbyNet.task, 0, sizeof(gLobbyNet.task));
    memset(gLobbyNet.unk65D6, 0, 0xA);
    memset(gLobby6138Data, 0, 0x28);
}

/* starts the connection (NetLoginTask) with the login parameters; -1 if it is active already */
s32 NetLoginTask_Start(NetLoginParams *src, void (*onDone)(NetMsg, NetMsg *))
{
    NetLoginParams params = *src;

    if (gLobbyNet.task[NET_TASK_LOGIN].active == 0) {
        gLobbyNet.login = params;
        gLobbyNet.unk14 = 0;
        gLobbyNet.connected = 1;
        gLobbyNet.rttCount = 0;
        gLobbyNet.rttSum = 0;
        memset(&gLobbyNet.me, 0, sizeof(gLobbyNet.me));
        memset(gLobbyNet.charas, 0, sizeof(gLobbyNet.charas));
        gLobbyNet.rxLen = 0;
        gLobbyNet.rxSize = 0x1000;
        gLobbyNet.rxBuf = gLobbyNet.rxStore;
        memset(&gLobbyNet.loginNotice, 0, sizeof(gLobbyNet.loginNotice));
        memset(gLobbyNet.rxStore, 0, sizeof(gLobbyNet.rxStore));
        gLobbyNet.task[NET_TASK_LOGIN].onDone = onDone;
        gLobbyNet.task[NET_TASK_LOGIN].active = 1;
        gLobbyNet.task[NET_TASK_LOGIN].state = NET_TASK_RUNNING;
        gLobbyNet.task[NET_TASK_LOGIN].func = NULL;
        return 0;
    }
    return -1;
}

s32 LobbyNet_SetClientInfo(NetClientInfo *src)
{
    gLobbyNet.clientInfo = *src;
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
        strncpy(gLobbyNet.charas[3].id, gNoUserId, 6);
        memset(gLobbyNet.me.id, 0, 8);
    } else {
        strncpy(gLobbyNet.charas[3].id, id, 6);
        strncpy(gLobbyNet.me.id, id, 6);
    }
    n = strlen(name);
    strncpy(gLobbyNet.charas[3].name, name, n);
    gLobbyNet.charas[3].name[n] = 0;
    LobbyPkt_CharaSelect();
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
void LobbySrvReq_LoginAuth(void)
{
    LobbyMsg_GetU16(&gLobbyNet.key, gLobbyRecvData);
    LobbyPkt_LoginAuth();
    LobbyPkt_LoginParams();
}

/* the server's request 6103 */
void LobbySrvReq_ClientInfo(void)
{
    if (gLobbyNet.login.type != 0) {
        LobbyPkt_Ping();
        return;
    }
    LobbyPkt_ClientInfo();
}

/* the file's .rodata: the user id used when the player has none */
#pragma rodata on
char gNoUserId[0x8] = {
#include "lobby/008385A0.inc"
};
#pragma rodata off
