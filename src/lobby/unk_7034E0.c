/*
 * NETWORK: the second level's entry requests 6302 (the name) and 6303 (the
 * values): senders, replies and getters (gLobbyNet.list1).
 */
#include "common.h"
#include "lobby_net.h"

char *strcpy(char *dst, const char *src);      /* strcpy */
s32 LobbyPkt_EntryValues(u16 kind, u16 id);
s32 LobbyPkt_EntryName(u16 kind, u16 id);
void LobbyMsg_ReadU16Str(void *a, void *b);

/* 6302 for entry id */
s32 LobbyReq_6302(u16 id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_EntryName(1, id);
        return i;
    }
    return -1;
}

s32 NetList1_GetName(u16 id, char *dst)
{
    strcpy(dst, gLobbyNet.list1[id - 1].name);
    return 0;
}

/* the reply to 6302 and its notification: u16 id, string; route 0x14 */
void LobbyReply_6302(void)
{
    char name[0x4C];
    u16 id;

    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU16Str(&id, name);
        gLobbyNet.list1[id - 1].id = id;
        strcpy(gLobbyNet.list1[id - 1].name, name);
        gLobbyNet.lastId = id;
        gLobbyNet.list1[id - 1].flags |= 4;
    }
    Lobby_RouteMessage(0x14);
}

/* 6303 for entry id */
s32 LobbyReq_6303(u16 id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_EntryValues(1, id);
        return i;
    }
    return -1;
}

s32 NetList1_GetUsers(u16 id, u16 *a, u16 *b)
{
    *a = gLobbyNet.list1[id - 1].users;
    *b = gLobbyNet.list1[id - 1].questUsers;
    return 0;
}

s32 NetList1_GetMaxUsers(u16 id, u16 *out)
{
    *out = gLobbyNet.list1[id - 1].maxUsers;
    return 0;
}
