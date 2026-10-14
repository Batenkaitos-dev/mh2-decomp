#include "common.h"
#include "em.h"
#include "lobby_net.h"

/* prototypes */
/* end of prototypes */

extern char D_00838598[];
char *strcpy(char *dst, const char *src);      /* strcpy */

s32 LobbyPkt_630A(s32 arg0);

s32 LobbyPkt_ListCount(u16 kind)
{
    s32 seq;

    switch (kind) {
    case 0:
        seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x3D);
        break;
    case 1:
        seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x54);
        break;
    case 2:
        seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x8A);
        break;
    }
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

s32 LobbyPkt_EntryName(u16 kind, u16 v)
{
    s32 seq;

    switch (kind) {
    case 0:
        seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x3F);
        break;
    case 1:
        seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x56);
        break;
    case 2:
        seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x8C);
        break;
    }
    LobbyPkt_PutU16(&gLobbyPkt, v);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

s32 LobbyPkt_EntryValues(u16 kind, u16 v)
{
    s32 seq;

    switch (kind) {
    case 0:
        seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x41);
        break;
    case 1:
        seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x59);
        break;
    case 2:
        seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x8F);
        break;
    }
    LobbyPkt_PutU16(&gLobbyPkt, v);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

s32 LobbyPkt_EntryByte(u16 kind, u16 v)
{
    s32 seq;

    switch (kind) {
    case 0:
        seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x44);
        break;
    case 1:
        seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x5C);
        break;
    case 2:
        seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x92);
        break;
    }
    LobbyPkt_PutU16(&gLobbyPkt, v);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

s32 LobbyPkt_EntryText(u16 kind, u16 v)
{
    s32 seq;

    switch (kind) {
    case 0:
        seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x47);
        break;
    case 1:
        seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x5F);
        break;
    case 2:
        seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x95);
        break;
    }
    LobbyPkt_PutU16(&gLobbyPkt, v);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

s32 LobbyPkt_6305(u16 v, char *s)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x50);
    LobbyPkt_PutU16(&gLobbyPkt, v);
    if (s == NULL) {
        LobbyPkt_PutBlob(&gLobbyPkt, D_00838598, 0);
    } else {
        LobbyPkt_PutBlob(&gLobbyPkt, s, strlen(s));
    }
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

s32 LobbyPkt_Leave(u16 kind)
{
    s32 seq;

    switch (kind) {
    case 0:
        seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x4A);
        break;
    case 1:
        seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x52);
        break;
    case 2:
        seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x87);
        break;
    }
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

s32 LobbyPkt_EntryFlag(u16 kind, u16 v)
{
    s32 seq;

    switch (kind) {
    case 0:
        seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x4D);
        break;
    case 1:
        seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x82);
        break;
    case 2:
        seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0xD4);
        break;
    }
    LobbyPkt_PutU16(&gLobbyPkt, v);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* reads a u16 id, then n u16 into out */
void LobbyMsg_ReadU16Array(u16 *id, u16 n, u16 *out)
{
    u8 *p;
    s32 i;

    p = LobbyMsg_GetU16(id, gLobbyRecvData);
    for (i = 0; i < n; i++) {
        p = LobbyMsg_GetU16(out, p);
        out++;
    }
}

/* the reply to 640F and its notification: u16 x3; route 0x1C */
void LobbyReply_640F(void)
{
    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_GetU16(&gLobbyNet.x640F[2], LobbyMsg_GetU16(&gLobbyNet.x640F[1], LobbyMsg_GetU16(&gLobbyNet.x640F[0], gLobbyRecvData)));
    }
    Lobby_RouteMessage(0x1C);
}

s32 LobbyReq_630A(s32 v, void (*onReply)(NetMsg, NetMsg *))
{
    s32 req;

    req = NetReq_Alloc(1, 0, onReply);
    if (req != -1) {
        memset(gLobbyNet.users, 0, 0xB40);
        gLobbyNet.req[req].seq = LobbyPkt_630A(v);
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

/* unk_7046A0's .rodata (0x838598-0x8385A0) */
#pragma rodata on
#pragma explicit_zero_data on
char D_00838598[0x8] = {
#include "lobby/00838598.inc"
};
#pragma explicit_zero_data reset
#pragma rodata off
