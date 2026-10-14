/*
 * NETWORK, the login (task 0): the reply to the ping 600E, the notification
 * 6131 (the account's characters), the reply to 6132, and 6190.
 */
#include "common.h"
#include "lobby_net.h"

typedef struct LobbyRec11C {
    u8 b[0x11C];
} LobbyRec11C;

s32 LobbyNet_GetMyCharaId(char *dst);
s32 LobbyNet_GetMyCharaName(char *dst);
void LobbyPkt_6103(void);
s32 LobbyPkt_6190(void *src, u16 n);
s32 LobbyPkt_600E(void);
s32 NetLogin_ReadCharas(void);
s32 LobbyReply_6132Read(void);
void LobbyPkt_61A1(void);
void LobbyMsg_ReadErrText(void);

/* the reply to 600E (a ping, four times): the frames each took, summed x16,
 * then a quarter of the sum goes out with 6103's reply; then 61A1 */
void LobbyReply_600E(void)
{
    gLobbyNet.rttSum += (gLobbyNet.frames * 0x10) & 0xFFFF;
    gLobbyNet.rttCount += 1;
    if ((s32)gLobbyNet.rttCount < 4) {
        LobbyPkt_600E();
        return;
    }
    gLobbyNet.x6103.state[4] = (s16)((u32)gLobbyNet.rttSum >> 2);
    LobbyPkt_6103();
    LobbyPkt_61A1();
}

/* the notification 6131: the account's characters (task 0's onDone: kind 0, 1) */
void LobbyNotify_6131(void)
{
    NetMsg m;

    if (gLobbyNet.task[0].active != 0) {
        NetLogin_ReadCharas();
        m.kind = 0;
        m.flag = 1;
        gLobbyNet.task[0].onDone(m, &m);
    }
}

/* the reply to 6132: the id the server gave the character (onDone: kind 0, 2;
 * an error: kind -1, 7) */
void LobbyReply_6132(void)
{
    NetMsg m;

    if (gLobbyNet.task[0].active != 0) {
        if (gLobbyNet.replyErr == 0) {
            LobbyReply_6132Read();
            LobbyNet_GetMyCharaId(gLobbyNet.self.id);
            LobbyNet_GetMyCharaName(gLobbyNet.self.name);
            m.kind = 0;
            m.flag = 2;
            gLobbyNet.task[0].onDone(m, &m);
        } else {
            LobbyMsg_ReadErrText();
            m.kind = -1;
            m.flag = 7;
            gLobbyNet.task[0].onDone(m, &m);
        }
    }
}

/* 6190: a blob */
s32 LobbyReq_6190(void *src, u16 n, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_6190(src, n);
        return i;
    }
    return -1;
}

/* the reply to 6190 */
void LobbyReply_6190(void)
{
    Lobby_RouteMessage(0);
}

s32 LobbyNet_Get6191(NetUser *dst)
{
    *dst = gLobbyNet.x6191;
    return 0;
}
