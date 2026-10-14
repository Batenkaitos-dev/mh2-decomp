/*
 * NETWORK: the second level's entry requests 6304, 6308 and 630D: senders,
 * replies and the getters of what they filled in (gLobbyNet.list1).
 */
#include "common.h"
#include "lobby_net.h"

char *strcpy(char *dst, const char *src);      /* strcpy */
s32 LobbyPkt_EntryByte(u16 kind, u16 arg1);
s32 LobbyPkt_EntryText(u16 kind, u16 arg1);
s32 LobbyPkt_630D(s32 arg0); /* unit's own declaration: with the definition's s32 (u16 id) (lobby/unk_703B90.c), LobbyReq_630D 1 of 29 words differ */
void LobbyMsg_ReadU16U8b(void *a, void *b);
void LobbyMsg_ReadU16Str2(void *a, void *b);

/* 6304 for entry id */
s32 LobbyReq_6304(u16 id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_EntryByte(1, id);
        return i;
    }
    return -1;
}

s32 NetList1_GetByte(u16 id, u8 *out)
{
    *out = gLobbyNet.list1[id - 1].state;
    return 0;
}

/* the reply to 6304 and its notification: u16 id, u8; route 0x16 */
void LobbyReply_6304(void)
{
    u16 id;
    u8 v;

    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU16U8b(&id, &v);
        gLobbyNet.list1[id - 1].id = id;
        gLobbyNet.list1[id - 1].state = v;
        gLobbyNet.lastId = id;
        gLobbyNet.list1[id - 1].flags |= 2;
    }
    Lobby_RouteMessage(0x16);
}

/* 6308 for entry id */
s32 LobbyReq_6308(u16 id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_EntryText(1, id);
        return i;
    }
    return -1;
}

s32 NetList1_GetText(u16 id, char *dst)
{
    strcpy(dst, gLobbyNet.list1[id - 1].text);
    return 0;
}

/* the reply to 6308 and its notification: u16 id, string; route 0x17 */
void LobbyReply_6308(void)
{
    char text[0x10C];
    u16 id;

    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU16Str2(&id, text);
        gLobbyNet.list1[id - 1].id = id;
        strcpy(gLobbyNet.list1[id - 1].text, text);
        gLobbyNet.list1[id - 1].flags |= 8;
    }
    Lobby_RouteMessage(0x17);
}

/* 630D for entry id */
s32 LobbyReq_630D(s32 id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_630D(id);
        return i;
    }
    return -1;
}

s32 NetList1_Get630D(u16 id, s32 *out)
{
    *out = gLobbyNet.list1[id - 1].color;
    return 0;
}
