/*
 * The three levels' shared request builders (a count, an entry's name,
 * values, state, text and kind, leaving, each for level 0, 1 or 2), the
 * sender of 6305 (entering a level-1 entry), 640F's reply and 630A (the
 * users of a level-1 entry). Network code: it stays byte for byte, as the
 * whole build does.
 */
#include "common.h"
#include "em.h"
#include "lobby_net.h"
#include "lobby_cmd.h"
#include "m2c_macros.h"

extern char gNoPassword[];
char *strcpy(char *dst, const char *src);      /* strcpy */
s32 LobbyPkt_Level1Users(u16 id);

/*
 * Sends the count request of a level (0-2: 6203, 6301, 6401); returns its
 * sequence number.
 */
s32 LobbyPkt_ListCount(u16 level)
{
    s32 seq;

    switch (level) {
    case 0:
        seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL0_COUNT);
        break;
    case 1:
        seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL1_COUNT);
        break;
    case 2:
        seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL2_COUNT);
        break;
    }
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/*
 * Asks for the name of entry id of a level (0-2: 6204, 6302, 6402);
 * returns the sequence number.
 */
s32 LobbyPkt_EntryName(u16 level, u16 id)
{
    s32 seq;

    switch (level) {
    case 0:
        seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL0_NAME);
        break;
    case 1:
        seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL1_NAME);
        break;
    case 2:
        seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL2_NAME);
        break;
    }
    LobbyPkt_PutU16(&gLobbyPkt, id);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/*
 * Asks for the values (users ..) of entry id of a level (0-2: 6205, 6303,
 * 6403); returns the sequence number.
 */
s32 LobbyPkt_EntryValues(u16 level, u16 id)
{
    s32 seq;

    switch (level) {
    case 0:
        seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL0_VALUES);
        break;
    case 1:
        seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL1_VALUES);
        break;
    case 2:
        seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL2_VALUES);
        break;
    }
    LobbyPkt_PutU16(&gLobbyPkt, id);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/*
 * Asks for the state of entry id of a level (0-2: 6206, 6304, 6404);
 * returns the sequence number.
 */
s32 LobbyPkt_EntryState(u16 level, u16 id)
{
    s32 seq;

    switch (level) {
    case 0:
        seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL0_STATE);
        break;
    case 1:
        seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL1_STATE);
        break;
    case 2:
        seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL2_STATE);
        break;
    }
    LobbyPkt_PutU16(&gLobbyPkt, id);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/*
 * Asks for the text of entry id of a level (0-2: 620A, 6308, 640D);
 * returns the sequence number.
 */
s32 LobbyPkt_EntryText(u16 level, u16 id)
{
    s32 seq;

    switch (level) {
    case 0:
        seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL0_TEXT);
        break;
    case 1:
        seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL1_TEXT);
        break;
    case 2:
        seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL2_TEXT);
        break;
    }
    LobbyPkt_PutU16(&gLobbyPkt, id);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/*
 * Enters level-1 entry id (6305) with its password, or with the empty blob
 * when there is none; returns the sequence number.
 */
s32 LobbyPkt_Level1Enter(u16 id, char *password)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL1_ENTER);
    LobbyPkt_PutU16(&gLobbyPkt, id);
    if (password == NULL) {
        LobbyPkt_PutBlob(&gLobbyPkt, gNoPassword, 0);
    } else {
        LobbyPkt_PutBlob(&gLobbyPkt, password, strlen(password));
    }
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/*
 * Leaves the entry the player is in at a level (0-2: 6306, 6408, 6501);
 * returns the sequence number.
 */
s32 LobbyPkt_Leave(u16 level)
{
    s32 seq;

    switch (level) {
    case 0:
        seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL0_LEAVE);
        break;
    case 1:
        seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL1_LEAVE);
        break;
    case 2:
        seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL2_LEAVE);
        break;
    }
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/*
 * Asks for the kind of entry id of a level (0-2: 620B, 630E, 6418);
 * returns the sequence number.
 */
s32 LobbyPkt_EntryKind(u16 level, u16 id)
{
    s32 seq;

    switch (level) {
    case 0:
        seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL0_KIND);
        break;
    case 1:
        seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL1_KIND);
        break;
    case 2:
        seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL2_KIND);
        break;
    }
    LobbyPkt_PutU16(&gLobbyPkt, id);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* reads a u16 id, then n u16 into out */
void LobbyMsg_ReadU16Array(u16 *id, u16 n, u16 *out)
{
    u8 *cur;
    s32 i;

    cur = LobbyMsg_GetU16(id, gLobbyRecvData);
    for (i = 0; i < n; i++) {
        cur = LobbyMsg_GetU16(out, cur);
        out++;
    }
}

/* the reply to 640F and its notification: u16 x3; route 0x1C */
void LobbyReply_640F(void)
{
    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_GetU16(&gLobbyNet.x640F[2], LobbyMsg_GetU16(&gLobbyNet.x640F[1], LobbyMsg_GetU16(&gLobbyNet.x640F[0], gLobbyRecvData)));
    }
    Lobby_RouteMessage(NET_ROUTE_640F);
}

/*
 * Sends 630A as a request; onReply gets its reply. Returns the request's
 * slot, -1 if none is free.
 */
s32 LobbyReq_Level1Users(u16 id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 req;

    req = NetReq_Alloc(1, 0, onReply);
    if (req != -1) {
        memset(gLobbyNet.users, 0, sizeof(gLobbyNet.users));
        gLobbyNet.req[req].seq = LobbyPkt_Level1Users(id);
        return req;
    }
    return -1;
}

/* user i of 630A's reply: id, name and text */
s32 LobbyNet_GetUser(u16 i, char *id, char *name, u8 *text)
{
    strcpy(id, gLobbyNet.users[i].u.id);
    strcpy(name, gLobbyNet.users[i].u.name);
    memcpy(text, gLobbyNet.users[i].u.text, 0x100);
    return 0;
}

/* the file's .rodata: the empty password blob */
#pragma rodata on
#pragma explicit_zero_data on
char gNoPassword[0x8] = {
#include "lobby/00838598.inc"
};
#pragma explicit_zero_data reset
#pragma rodata off

/* Sends 630A for level-1 entry id; returns its sequence number. */
s32 LobbyPkt_Level1Users(u16 id)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL1_USERS) & 0xFFFF;
    LobbyPkt_PutU16(&gLobbyPkt, id);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}
