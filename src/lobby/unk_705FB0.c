/*
 * NETWORK: task 8's steps 0-3 (unk_705D50.c): the settings requests.
 */
#include "common.h"
#include "lobby_net.h"

s32 LobbyReq_6315(char *s, u16 n, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_6316(s8 *arg0, u16 arg1, void (*arg2)(NetMsg, NetMsg *));
s32 LobbyReq_6317(s8 *arg0, u16 arg1, void (*arg2)(NetMsg, NetMsg *));
s32 LobbyReq_6318(u16 arg0, u8 *arg1, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_6311(void (*onReply)(NetMsg, NetMsg *));
void NetTask8_Step1Reply1(NetMsg m, NetMsg *pm);
void NetTask8_Step1Reply2(NetMsg m, NetMsg *pm);
void NetTask8_Step1Reply4(NetMsg m, NetMsg *pm);
void NetTask8_Step3Reply(NetMsg m, NetMsg *pm);
void NetTask8_Step1Reply8(NetMsg m, NetMsg *pm);
s32 NetReq_CountFree(void);

/* task 8, step 0 */
void NetTask8_Step0(NetTask *t)
{
    t->step++;
    gLobbyNet.nReplies = 0;
    t->idx = 1;
    t->pos = 1;
    t->reply = 0;
}

/* task 8, step 1: the settings the entry allows (its 6312-6314 values are 1);
 * the others count as answered */
void NetTask8_Step1(NetTask *t)
{
    if (NetReq_CountFree() >= 4) {
        t->step++;
        if (gLobbyNet.settings.name[0] != 0 && gLobbyNet.rec[gLobbyNet.cur[1] - 1].v[0] == 1) {
            LobbyReq_6315(gLobbyNet.settings.name, strlen(gLobbyNet.settings.name), NetTask8_Step1Reply1);
        } else {
            t->reply |= 1;
        }
        if (gLobbyNet.settings.pass[0] != 0 && gLobbyNet.rec[gLobbyNet.cur[1] - 1].v[1] == 1) {
            LobbyReq_6316((s8 *) gLobbyNet.settings.pass, strlen(gLobbyNet.settings.pass), NetTask8_Step1Reply2);
        } else {
            t->reply |= 2;
        }
        if (gLobbyNet.settings.text[0] != 0 && gLobbyNet.rec[gLobbyNet.cur[1] - 1].v[2] == 1) {
            LobbyReq_6317((s8 *) gLobbyNet.settings.text, strlen(gLobbyNet.settings.text), NetTask8_Step1Reply4);
        } else {
            t->reply |= 4;
        }
        LobbyReq_6318(gLobbyNet.cur[1], gLobbyNet.settings.b, NetTask8_Step1Reply8);
    }
}

/* task 8, step 2: all four answered */
void NetTask8_Step2(NetTask *t)
{
    if (t->reply == 0xF) {
        t->step++;
        t->reply = 0;
    } else if (t->reply & 0x80) {
        t->state = 2;
        t->step = 0;
        t->reply = 0;
    }
}

/* task 8, step 3: 6311 */
void NetTask8_Step3(NetTask *t)
{
    if (NetReq_CountFree() > 0) {
        t->step++;
        LobbyReq_6311(NetTask8_Step3Reply);
    }
}
