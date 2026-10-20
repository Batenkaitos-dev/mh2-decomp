/*
 * The quest party (69xx) and going out on a quest (6504). Network code:
 * it stays byte for byte, as the whole build does.
 * - 6504 tells the server the player goes out (1) or is back (0).
 * - The party task (NET_TASK_PARTY) has no steps: its starter sends 6911,
 *   and each reply sends the next request. 6911 gives the number of
 *   players, 6912 a byte, then 6913 and 6917 for each player (1..count:
 *   id, name, text...), 6915 the room's name, 6914 a text, and 6916 the
 *   address (IPv4, port) of the quest server to connect to. The data goes
 *   to gLobbyNet.party (NetParty) and gLobbyNet.partyAddr.
 * - The reply to 640E, a level-2 command of unknown meaning, comes first.
 */
#include "common.h"
#include "lobby_net.h"

s32 LobbyPkt_SetOut(u8 value);
void Lobby_RouteMessage(s32);
s32 LobbyPkt_PartyCount(void);
extern u8 gNetPartyFetchIdx;           /* the server asked for next (1-based) */
u8 *LobbyMsg_ReadU8(void *dst);
void LobbyMsg_ReadErrText(void);
void NetPartyTask_Finish(NetMsg m);
void LobbyPkt_PartySelf(u8 arg0);
void LobbyPkt_PartyMember(u8 arg0);
void LobbyPkt_PartyMemberValues(u8 arg0);
void LobbyPkt_PartyText(void);
void LobbyPkt_PartyRoom(void);
void LobbyPkt_PartyServer(void);
s32 LobbyPkt_PartyFail(void);
u16 Swap16(s32);

/* The reply to 640E: completes its request. */
void LobbyReply_640E(void)
{
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/*
 * Sends 6504 as a request; onReply gets its reply. Returns the request's
 * slot, -1 if none is free.
 */
s32 LobbyReq_SetOut(u8 value, void (*onReply)(NetMsg, NetMsg *))
{
    s32 req;

    req = NetReq_Alloc(1, 0, onReply);
    if (req != -1) {
        gLobbyNet.req[req].seq = LobbyPkt_SetOut(value);
        return req;
    }
    return -1;
}

/* Sends 6504 (a byte); returns its sequence number. */
s32 LobbyPkt_SetOut(u8 value)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_SET_OUT) & 0xFFFF;
    LobbyPkt_PutU8(&gLobbyPkt, value);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* The reply to 6504: completes its request. */
void LobbyReply_SetOut(void)
{
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/* Starts the party task (no steps): clears gLobbyNet.party and sends 6911. */
s32 NetPartyTask_Start(void (*onDone)(NetMsg, NetMsg *))
{
    if (gLobbyNet.task[NET_TASK_PARTY].active == 0) {
        memset(&gLobbyNet.party, 0, sizeof(NetParty));
        LobbyPkt_PartyCount();
        gLobbyNet.task[NET_TASK_PARTY].onDone = onDone;
        gLobbyNet.task[NET_TASK_PARTY].active = 1;
        gLobbyNet.task[NET_TASK_PARTY].state = NET_TASK_RUNNING;
        gLobbyNet.task[NET_TASK_PARTY].func = NULL;
        return 0;
    }
    return -1;
}

/* the reply to 6911: the number of players; then 6912 */
void LobbyReply_PartyCount(void)
{
    NetMsg msg;

    if (gLobbyNet.task[NET_TASK_PARTY].active != 0 && gLobbyNet.flag == LOBBY_FLAG_REPLY) {
        if (gLobbyNet.replyErr == 0) {
            LobbyMsg_ReadU8(&gLobbyNet.party.count);
            LobbyPkt_PartySelf(0);
        } else {
            msg.kind = -1;
            LobbyMsg_ReadErrText();
            NetPartyTask_Finish(msg);
        }
    }
}

/* the reply to 6912: a byte (less one); then 6913 for server 1 */
void LobbyReply_PartySelf(void)
{
    u8 value;

    if (gLobbyNet.task[NET_TASK_PARTY].active != 0 && gLobbyNet.flag == LOBBY_FLAG_REPLY) {
        if (gLobbyNet.replyErr == 0) {
            LobbyMsg_ReadU8(&value);
            if (value != 0) {
                value--;
            }
            gNetPartyFetchIdx = 1;
            gLobbyNet.party.unk1 = value;
            LobbyPkt_PartyMember(1);
        } else {
            NetMsg msg;

            msg.kind = -1;
            LobbyMsg_ReadErrText();
            NetPartyTask_Finish(msg);
        }
    }
}

/* the reply to 6913: u8 server, u8, four strings, u8; then the next server's
 * 6913, or 6917 for server 1 */
void LobbyReply_PartyMember(void)
{
    NetParty *partyRec;
    u8 *cur;

    if (gLobbyNet.task[NET_TASK_PARTY].active != 0 && gLobbyNet.flag != LOBBY_FLAG_NOTIFY) {
        if (gLobbyNet.flag == LOBBY_FLAG_REPLY) {
            if (gLobbyNet.replyErr == 0) {
                u8 i;

                partyRec = &gLobbyNet.party;
                cur = LobbyMsg_GetU8(&i, gLobbyRecvData);
                cur = LobbyMsg_GetU8(&partyRec->members[i - 1].b156, cur);
                cur = LobbyMsg_GetString(partyRec->members[i - 1].id, cur);
                cur = LobbyMsg_GetString(partyRec->members[i - 1].name, cur);
                cur = LobbyMsg_GetString(partyRec->members[i - 1].text, cur);
                cur = LobbyMsg_GetString(partyRec->members[i - 1].s11C, cur);
                LobbyMsg_GetU8(&partyRec->members[i - 1].b155, cur);
                partyRec->members[i - 1].index = i;
            } else {
                NetMsg msg;

                msg.kind = -1;
                LobbyMsg_ReadErrText();
                NetPartyTask_Finish(msg);
                return;
            }
        }
        gNetPartyFetchIdx++;
        if (gNetPartyFetchIdx <= gLobbyNet.party.count) {
            LobbyPkt_PartyMember(gNetPartyFetchIdx);
        } else {
            gNetPartyFetchIdx = 1;
            LobbyPkt_PartyMemberValues(1);
        }
    }
}

/* the reply to 6917: u8 server, u16, u32 x5; then the next server 6917, or 6915 */
void LobbyReply_PartyMemberValues(void)
{
    NetParty *partyRec;
    u8 *cur;

    if (gLobbyNet.task[NET_TASK_PARTY].active != 0 && gLobbyNet.flag != LOBBY_FLAG_NOTIFY) {
        if (gLobbyNet.flag == LOBBY_FLAG_REPLY) {
            if (gLobbyNet.replyErr == 0) {
                u8 i;

                partyRec = &gLobbyNet.party;
                cur = LobbyMsg_GetU8(&i, gLobbyRecvData);
                cur = LobbyMsg_GetU16(&partyRec->members[i - 1].h13C, cur);
                cur = LobbyMsg_GetU32(&partyRec->members[i - 1].w[0], cur);
                cur = LobbyMsg_GetU32(&partyRec->members[i - 1].w[1], cur);
                cur = LobbyMsg_GetU32(&partyRec->members[i - 1].w[2], cur);
                cur = LobbyMsg_GetU32(&partyRec->members[i - 1].w[3], cur);
                LobbyMsg_GetU32(&partyRec->members[i - 1].w[4], cur);
                partyRec->members[i - 1].index = i;
            } else {
                NetMsg msg;

                msg.kind = -1;
                LobbyMsg_ReadErrText();
                NetPartyTask_Finish(msg);
                return;
            }
        }
        gNetPartyFetchIdx++;
        if (gNetPartyFetchIdx <= gLobbyNet.party.count) {
            LobbyPkt_PartyMemberValues(gNetPartyFetchIdx);
        } else {
            LobbyPkt_PartyRoom();
        }
    }
}

/* the reply to 6915: string (0x10), u16; then 6914 */
void LobbyReply_PartyRoom(void)
{
    NetParty *partyRec;
    NetMsg msg;

    if (gLobbyNet.task[NET_TASK_PARTY].active != 0 && gLobbyNet.flag != LOBBY_FLAG_NOTIFY) {
        if (gLobbyNet.flag == LOBBY_FLAG_REPLY) {
            if (gLobbyNet.replyErr == 0) {
                partyRec = &gLobbyNet.party;
                LobbyMsg_GetU16(&partyRec->unk12, LobbyMsg_GetString(partyRec->name, gLobbyRecvData));
            } else {
                msg.kind = -1;
                LobbyMsg_ReadErrText();
                NetPartyTask_Finish(msg);
                return;
            }
        }
        LobbyPkt_PartyText();
    }
}

/* the reply to 6914: a string; then 6916 */
void LobbyReply_PartyText(void)
{
    NetMsg msg;

    if (gLobbyNet.task[NET_TASK_PARTY].active != 0 && gLobbyNet.flag != LOBBY_FLAG_NOTIFY) {
        if (gLobbyNet.flag == LOBBY_FLAG_REPLY) {
            if (gLobbyNet.replyErr == 0) {
                LobbyMsg_GetString(gLobbyNet.party.text, gLobbyRecvData);
            } else {
                msg.kind = -1;
                LobbyMsg_ReadErrText();
                NetPartyTask_Finish(msg);
                return;
            }
        }
        LobbyPkt_PartyServer();
    }
}

/* the reply to 6916: the address to connect to (two strings: IPv4, port);
 * the party task is done */
void LobbyReply_PartyServer(void)
{
    NetMsg msg;

    if (gLobbyNet.task[NET_TASK_PARTY].active != 0) {
        if (gLobbyNet.flag == LOBBY_FLAG_REPLY) {
            if (gLobbyNet.replyErr == 0) {
                msg.kind = 0;
                LobbyMsg_GetString(gLobbyNet.partyPort, LobbyMsg_GetString(gLobbyNet.partyAddr, gLobbyRecvData));
            } else {
                msg.kind = -1;
                LobbyMsg_ReadErrText();
                NetPartyTask_Finish(msg);
                return;
            }
        }
        msg.kind = 0;
        NetPartyTask_Finish(msg);
    }
}

/* ends the party task (a failure sends 6918 first) */
void NetPartyTask_Finish(NetMsg msg)
{
    void (*onDone)(NetMsg, NetMsg *);

    if (msg.kind == -1) {
        LobbyPkt_PartyFail();
    }
    onDone = gLobbyNet.task[NET_TASK_PARTY].onDone;
    if (onDone != NULL) {
        gLobbyNet.task[NET_TASK_PARTY].active = 0;
        gLobbyNet.task[NET_TASK_PARTY].state = NET_TASK_RUNNING;
        onDone(msg, &msg);
    }
}

/* 6911 */
s32 LobbyPkt_PartyCount(void)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_PARTY_COUNT) & 0xFFFF;
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* 6912: u8 */
void LobbyPkt_PartySelf(u8 value)
{
    LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_PARTY_SELF);
    LobbyPkt_PutU8(&gLobbyPkt, value);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
}

/* 6913: u8 server */
void LobbyPkt_PartyMember(u8 value)
{
    LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_PARTY_MEMBER);
    LobbyPkt_PutU8(&gLobbyPkt, value);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
}

/* 6917: u8 server */
void LobbyPkt_PartyMemberValues(u8 value)
{
    LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_PARTY_MEMBER_VALUES);
    LobbyPkt_PutU8(&gLobbyPkt, value);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
}

/* 6914 */
void LobbyPkt_PartyText(void)
{
    LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_PARTY_TEXT);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
}

/* 6915 */
void LobbyPkt_PartyRoom(void)
{
    LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_PARTY_ROOM);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
}

/* 6916 */
void LobbyPkt_PartyServer(void)
{
    LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_PARTY_SERVER);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
}

/* the notification 6918 (sent when the party task fails) */
s32 LobbyPkt_PartyFail(void)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_PARTY_FAIL) & 0xFFFF;
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
