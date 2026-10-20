/*
 * The readers that take a reply's fields from gLobbyRecvData (a u16, then
 * a string or a byte), and level 0's entry kind (620B): its request,
 * getter and reply. Network code: it stays byte for byte, as the whole
 * build does.
 */
#include "common.h"
#include "lobby_net.h"

s32 LobbyPkt_EntryKind(u16 level, u16 id);

/* Reads a reply's u16 into dst. */
void LobbyMsg_ReadU16(void *dst)
{
    LobbyMsg_GetU16(dst, gLobbyRecvData);
}

/* Reads a reply's u16 and the string after it. */
void LobbyMsg_ReadU16Str(void *num, void *str)
{
    LobbyMsg_GetString(str, LobbyMsg_GetU16(num, gLobbyRecvData));
}

/* Reads a reply's u16 and the byte after it. */
void LobbyMsg_ReadU16U8b(void *num, void *byte)
{
    LobbyMsg_GetU8(byte, LobbyMsg_GetU16(num, gLobbyRecvData));
}

/* The same as LobbyMsg_ReadU16Str. */
void LobbyMsg_ReadU16Str2(void *num, void *str)
{
    LobbyMsg_GetString(str, LobbyMsg_GetU16(num, gLobbyRecvData));
}

/* The same as LobbyMsg_ReadU16U8b. */
void LobbyMsg_ReadU16U8b2(void *num, void *byte)
{
    LobbyMsg_GetU8(byte, LobbyMsg_GetU16(num, gLobbyRecvData));
}

/*
 * Sends 620B as a request; onReply gets its reply. Returns the request's
 * slot, -1 if none is free.
 */
s32 LobbyReq_Level0Kind(u16 id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 req;

    req = NetReq_Alloc(1, 0, onReply);
    if (req != -1) {
        gLobbyNet.req[req].seq = LobbyPkt_EntryKind(0, id);
        return req;
    }
    return -1;
}

/* The kind of level-0 entry id (620B's reply); returns 0. */
s32 NetList0_GetKind(u16 id, u8 *out)
{
    *out = gLobbyNet.list0[(id & 0xFFFF) - 1].kind;
    return 0;
}

/* the reply to 620B and its notification: u16 id, u8; route 0x34 */
void LobbyReply_Level0Kind(void)
{
    u16 id;
    u8 value;

    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU16U8b2(&id, &value);
        gLobbyNet.list0[id - 1].id = id;
        gLobbyNet.list0[id - 1].kind = value;
        gLobbyNet.list0[id - 1].flags |= NET_ENTRY_KIND;
    }
    Lobby_RouteMessage(NET_ROUTE_LEVEL0_KIND);
}

