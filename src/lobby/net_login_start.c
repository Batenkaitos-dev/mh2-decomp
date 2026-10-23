/*
 * The connection's reset and start (NetLoginTask_Start), the player's own
 * user record, and the server's 6101 and 6103 requests.
 * Network code: it stays byte for byte, as the whole build does.
 */
#include "common.h"
#include "lobby_net.h"
#include "libc.h"


extern char gNoUserId[];
extern u8 gLobby6138Data[];

/* resets the connection's state */
void LobbyNet_Reset(void)
{
    gLobbyNet.unk8 = 0;
    gLobbyNet.connected = 0;
    memset(gLobbyNet.req, 0, sizeof(gLobbyNet.req));
    memset(gLobbyNet.task, 0, sizeof(gLobbyNet.task));
    memset(gLobbyNet.unk65D6, 0, 10);
    memset(gLobby6138Data, 0, 40);
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
        gLobbyNet.rxSize = 4096;
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

    memset(&gLobbyNet.me, 0, 284);
    strncpy(gLobbyNet.me.name, name, strlen(name));
    memcpy(gLobbyNet.me.text, text, 256);
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
    memcpy(dst, gLobbyNet.charas[i].text, 256);
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
char gNoUserId[8] = {
#include "lobby/008385A0.inc"
};
#pragma rodata off
