/*
 * NETWORK: task 8's last step and its reply callbacks; the sender of 6315.
 */
#include "common.h"
#include "lobby_net.h"

s32 LobbyPkt_6315(char *s, u16 n);

/* task 8, step 4: 6311's reply */
void NetTask8_Step4(NetTask *t)
{
    s32 r = t->reply;

    if (r != 0) {
        t->state++;
        t->step = 0;
        t->reply = 0;
    } else if ((u8)r & 0x80) {
        t->state = 2;
        t->step = 0;
        t->reply = 0;
    }
}

void NetTask8_Step1Reply1(NetMsg m, NetMsg *pm)
{
    NetTask *t = &gLobbyNet.task[8];

    if (m.kind == 0) {
        t->reply |= 1;
    } else {
        t->reply |= 0x80;
    }
}

void NetTask8_Step1Reply2(NetMsg m, NetMsg *pm)
{
    NetTask *t = &gLobbyNet.task[8];

    if (m.kind == 0) {
        t->reply |= 2;
    } else {
        t->reply |= 0x80;
    }
}

void NetTask8_Step1Reply4(NetMsg m, NetMsg *pm)
{
    NetTask *t = &gLobbyNet.task[8];

    if (m.kind == 0) {
        t->reply |= 4;
    } else {
        t->reply |= 0x80;
    }
}

void NetTask8_Step3Reply(NetMsg m, NetMsg *pm)
{
    NetTask *t = &gLobbyNet.task[8];

    if (m.kind == 0) {
        t->reply |= 4;
    } else {
        t->reply |= 0x80;
    }
}

void NetTask8_Step1Reply8(NetMsg m, NetMsg *pm)
{
    NetTask *t = &gLobbyNet.task[8];

    if (m.kind == 0) {
        t->reply |= 8;
    } else {
        t->reply |= 0x80;
    }
}

/* 6315: the name */
s32 LobbyReq_6315(char *s, u16 n, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_6315(s, n);
        return i;
    }
    return -1;
}
