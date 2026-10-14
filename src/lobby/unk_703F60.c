/*
 * NETWORK: the reply to 631B (and its notification): four bytes of a second
 * level entry; the sender of 6305 (enter a second-level entry).
 */
#include "common.h"
#include "lobby_net.h"

void NetList_Select(s32, s32); /* unit's own declaration: with the definition's void (s32 arg0, u16 id) (lobby/unk_700AF0.c), LobbyReq_6305 2 of 34 words differ */
s32 LobbyPkt_6305(s32 arg0, char *s);

/* the reply to 631B and its notification: u16 id, u8 x4; route 0x33 */
void LobbyReply_631B(void)
{
    u16 id;
    u8 *p;

    if (gLobbyNet.replyErr == 0) {
        p = LobbyMsg_GetU16(&id, gLobbyRecvData);
        p = LobbyMsg_GetU8(&gLobbyNet.list1[id - 1].groupInfo[0], p);
        p = LobbyMsg_GetU8(&gLobbyNet.list1[id - 1].groupInfo[1], p);
        p = LobbyMsg_GetU8(&gLobbyNet.list1[id - 1].groupInfo[2], p);
        LobbyMsg_GetU8(&gLobbyNet.list1[id - 1].groupInfo[3], p);
        gLobbyNet.list1[id - 1].id = id;
        gLobbyNet.lastId = id;
        gLobbyNet.list1[id - 1].flags |= 0x200;
    }
    Lobby_RouteMessage(0x33);
}

/* 6305: enters second-level entry id (with a password, NULL for none) */
s32 LobbyReq_6305(s32 id, char *pass, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        NetList_Select(1, id);
        gLobbyNet.req[i].seq = LobbyPkt_6305(id, pass);
        return i;
    }
    return -1;
}
