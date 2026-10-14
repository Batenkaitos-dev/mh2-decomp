/*
 * NETWORK: the third level's entry requests 6404, 6402, 640D and 640B:
 * senders, replies and getters (gLobbyNet.list2).
 */
#include "common.h"
#include "lobby_net.h"

char *strcpy(char *dst, const char *src);      /* strcpy */
s32 LobbyPkt_EntryName(u16 kind, u16 id);
s32 LobbyPkt_EntryByte(u16 kind, u16 id);
s32 LobbyPkt_EntryText(u16 kind, u16 id);
s32 LobbyPkt_640B(u16 id);
void LobbyMsg_ReadU16Str(void *a, void *b);
void LobbyMsg_ReadU16U8b(void *a, void *b);
void LobbyMsg_ReadU16Str2(void *a, void *b);

/* 6404 for entry id */
s32 LobbyReq_6404(u16 id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_EntryByte(2, id);
        return i;
    }
    return -1;
}

s32 NetList2_GetByte(u16 id, u8 *out)
{
    *out = gLobbyNet.list2[id - 1].state;
    return 0;
}

/* the reply to 6404 and its notification: u16 id, u8; route 0x1F */
void LobbyReply_6404(void)
{
    u16 id;
    u8 v;

    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU16U8b(&id, &v);
        gLobbyNet.list2[id - 1].id = id;
        gLobbyNet.list2[id - 1].state = v;
        gLobbyNet.lastId = id;
        gLobbyNet.list2[id - 1].flags |= 2;
    }
    Lobby_RouteMessage(0x1F);
}

/* 6402 for entry id */
s32 LobbyReq_6402(u16 id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_EntryName(2, id);
        return i;
    }
    return -1;
}

s32 NetList2_GetName(u16 id, char *dst)
{
    strcpy(dst, gLobbyNet.list2[id - 1].name);
    return 0;
}

/* the reply to 6402 and its notification: u16 id, string; route 0x1D */
void LobbyReply_6402(void)
{
    char name[0x10C];
    u16 id;

    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU16Str(&id, name);
        gLobbyNet.list2[id - 1].id = id;
        strcpy(gLobbyNet.list2[id - 1].name, name);
        gLobbyNet.lastId = id;
        gLobbyNet.list2[id - 1].flags |= 4;
    }
    Lobby_RouteMessage(0x1D);
}

/* 640D for entry id */
s32 LobbyReq_640D(u16 id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_EntryText(2, id);
        return i;
    }
    return -1;
}

s32 NetList2_GetText(u16 id, char *dst)
{
    strcpy(dst, gLobbyNet.list2[id - 1].text);
    return 0;
}

/* the reply to 640D and its notification: u16 id, string; route 0x20 */
void LobbyReply_640D(void)
{
    char text[0x10C];
    u16 id;

    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU16Str2(&id, text);
        gLobbyNet.list2[id - 1].id = id;
        strcpy(gLobbyNet.list2[id - 1].text, text);
        gLobbyNet.list2[id - 1].flags |= 8;
    }
    Lobby_RouteMessage(0x20);
}

/* 640B for entry id */
s32 LobbyReq_640B(u16 id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_640B(id);
        return i;
    }
    return -1;
}

/* the values 640B's reply gave entry id */
s32 NetList2_Get640B(u16 id, u16 *a, u16 *b, u16 *c, u16 *d, u16 *e)
{
    *a = gLobbyNet.list2[id - 1].maxUsers;
    *b = gLobbyNet.list2[id - 1].unk12;
    *c = gLobbyNet.list2[id - 1].unk14;
    *d = gLobbyNet.list2[id - 1].unk16;
    *e = gLobbyNet.list2[id - 1].unk18;
    return 0;
}
