/*
 * NETWORK: task 9, the quest party (69xx), driven by its replies: 6911 gives
 * the number of players, 6912 a byte, then 6913 and 6917 for each player
 * (1..count: id, name, text...), 6915 the room's name, 6914 a text, and 6916
 * the address (IPv4, port) of the quest server to connect to. The data goes
 * to gLobbyNet.party (NetParty) and gLobbyNet.partyAddr (the address).
 */
#include "common.h"
#include "lobby_net.h"

extern u8 gNetPartyFetchIdx;           /* the server asked for next (1-based) */
u8 *LobbyMsg_ReadU8(void *dst);
void LobbyMsg_ReadErrText(void);
void NetTask9_Finish(NetMsg m);
void LobbyPkt_6912(u8 arg0);
void LobbyPkt_6913(u8 arg0);
void LobbyPkt_6917(u8 arg0);
void LobbyPkt_6914(void);
void LobbyPkt_6915(void);
void LobbyPkt_6916(void);
s32 LobbyPkt_6918(void);

/* the reply to 6911: the number of players; then 6912 */
void LobbyReply_6911(void)
{
    NetMsg m;

    if (gLobbyNet.task[9].active != 0 && gLobbyNet.flag == 2) {
        if (gLobbyNet.replyErr == 0) {
            LobbyMsg_ReadU8(&gLobbyNet.party.count);
            LobbyPkt_6912(0);
        } else {
            m.kind = -1;
            LobbyMsg_ReadErrText();
            NetTask9_Finish(m);
        }
    }
}

/* the reply to 6912: a byte (less one); then 6913 for server 1 */
void LobbyReply_6912(void)
{
    u8 v;

    if (gLobbyNet.task[9].active != 0 && gLobbyNet.flag == 2) {
        if (gLobbyNet.replyErr == 0) {
            LobbyMsg_ReadU8(&v);
            if (v != 0) {
                v--;
            }
            gNetPartyFetchIdx = 1;
            gLobbyNet.party.unk1 = v;
            LobbyPkt_6913(1);
        } else {
            NetMsg m;

            m.kind = -1;
            LobbyMsg_ReadErrText();
            NetTask9_Finish(m);
        }
    }
}

/* the reply to 6913: u8 server, u8, four strings, u8; then the next server's
 * 6913, or 6917 for server 1 */
void LobbyReply_6913(void)
{
    NetParty *d;
    u8 *p;

    if (gLobbyNet.task[9].active != 0 && gLobbyNet.flag != 0x10) {
        if (gLobbyNet.flag == 2) {
            if (gLobbyNet.replyErr == 0) {
                u8 i;

                d = &gLobbyNet.party;
                p = LobbyMsg_GetU8(&i, gLobbyRecvData);
                p = LobbyMsg_GetU8(&d->members[i - 1].b156, p);
                p = LobbyMsg_GetString(d->members[i - 1].id, p);
                p = LobbyMsg_GetString(d->members[i - 1].name, p);
                p = LobbyMsg_GetString(d->members[i - 1].text, p);
                p = LobbyMsg_GetString(d->members[i - 1].s11C, p);
                LobbyMsg_GetU8(&d->members[i - 1].b155, p);
                d->members[i - 1].index = i;
            } else {
                NetMsg m;

                m.kind = -1;
                LobbyMsg_ReadErrText();
                NetTask9_Finish(m);
                return;
            }
        }
        gNetPartyFetchIdx++;
        if (gNetPartyFetchIdx <= gLobbyNet.party.count) {
            LobbyPkt_6913(gNetPartyFetchIdx);
        } else {
            gNetPartyFetchIdx = 1;
            LobbyPkt_6917(1);
        }
    }
}

/* the reply to 6917: u8 server, u16, u32 x5; then the next server 6917, or 6915 */
void LobbyReply_6917(void)
{
    NetParty *d;
    u8 *p;

    if (gLobbyNet.task[9].active != 0 && gLobbyNet.flag != 0x10) {
        if (gLobbyNet.flag == 2) {
            if (gLobbyNet.replyErr == 0) {
                u8 i;

                d = &gLobbyNet.party;
                p = LobbyMsg_GetU8(&i, gLobbyRecvData);
                p = LobbyMsg_GetU16(&d->members[i - 1].h13C, p);
                p = LobbyMsg_GetU32(&d->members[i - 1].w[0], p);
                p = LobbyMsg_GetU32(&d->members[i - 1].w[1], p);
                p = LobbyMsg_GetU32(&d->members[i - 1].w[2], p);
                p = LobbyMsg_GetU32(&d->members[i - 1].w[3], p);
                LobbyMsg_GetU32(&d->members[i - 1].w[4], p);
                d->members[i - 1].index = i;
            } else {
                NetMsg m;

                m.kind = -1;
                LobbyMsg_ReadErrText();
                NetTask9_Finish(m);
                return;
            }
        }
        gNetPartyFetchIdx++;
        if (gNetPartyFetchIdx <= gLobbyNet.party.count) {
            LobbyPkt_6917(gNetPartyFetchIdx);
        } else {
            LobbyPkt_6915();
        }
    }
}

/* the reply to 6915: string (0x10), u16; then 6914 */
void LobbyReply_6915(void)
{
    NetParty *d;
    NetMsg m;

    if (gLobbyNet.task[9].active != 0 && gLobbyNet.flag != 0x10) {
        if (gLobbyNet.flag == 2) {
            if (gLobbyNet.replyErr == 0) {
                d = &gLobbyNet.party;
                LobbyMsg_GetU16(&d->unk12, LobbyMsg_GetString(d->name, gLobbyRecvData));
            } else {
                m.kind = -1;
                LobbyMsg_ReadErrText();
                NetTask9_Finish(m);
                return;
            }
        }
        LobbyPkt_6914();
    }
}

/* the reply to 6914: a string; then 6916 */
void LobbyReply_6914(void)
{
    NetMsg m;

    if (gLobbyNet.task[9].active != 0 && gLobbyNet.flag != 0x10) {
        if (gLobbyNet.flag == 2) {
            if (gLobbyNet.replyErr == 0) {
                LobbyMsg_GetString(gLobbyNet.party.text, gLobbyRecvData);
            } else {
                m.kind = -1;
                LobbyMsg_ReadErrText();
                NetTask9_Finish(m);
                return;
            }
        }
        LobbyPkt_6916();
    }
}

/* the reply to 6916: the address to connect to (two strings: IPv4, port);
 * task 9 is done */
void LobbyReply_6916(void)
{
    NetMsg m;

    if (gLobbyNet.task[9].active != 0) {
        if (gLobbyNet.flag == 2) {
            if (gLobbyNet.replyErr == 0) {
                m.kind = 0;
                LobbyMsg_GetString(gLobbyNet.partyPort, LobbyMsg_GetString(gLobbyNet.partyAddr, gLobbyRecvData));
            } else {
                m.kind = -1;
                LobbyMsg_ReadErrText();
                NetTask9_Finish(m);
                return;
            }
        }
        m.kind = 0;
        NetTask9_Finish(m);
    }
}

/* ends task 9 (a failure sends 6918 first) */
void NetTask9_Finish(NetMsg m)
{
    void (*onDone)(NetMsg, NetMsg *);

    if (m.kind == -1) {
        LobbyPkt_6918();
    }
    onDone = gLobbyNet.task[9].onDone;
    if (onDone != NULL) {
        gLobbyNet.task[9].active = 0;
        gLobbyNet.task[9].state = 0;
        onDone(m, &m);
    }
}
