/*
 * NETWORK: task 5 (gLobbyNet.task[5]): its starter, dispatcher and step
 * switch; the steps are in unk_7055A0.c.
 */
#include "common.h"
#include "lobby_net.h"

void NetTask5_Run(s32 task);
void NetTask5_Steps(NetTask *t);
void NetTask5_Done(NetTask *t);
void NetTask5_Failed(NetTask *t);
void NetTask5_Step0(NetTask *t);
void NetTask5_Step1(NetTask *t);
void NetTask5_Step2(NetTask *t);

/* starts task 5; -1 if it is already running */
s32 NetTask5_Start(u16 n, void (*onDone)(NetMsg, NetMsg *))
{
    NetTask *t = &gLobbyNet.task[5];

    if (t->active == 0) {
        memset(t, 0, sizeof(NetTask));
        t->active = 1;
        t->onDone = onDone;
        t->func = NetTask5_Run;
        t->arg = n;
        return 0;
    }
    return -1;
}

void NetTask5_Run(s32 task)
{
    NetTask *t = &gLobbyNet.task[5];

    if (t->active != 0) {
        switch (t->state) {
        case 0:
            NetTask5_Steps(t);
            break;
        case 1:
            NetTask5_Done(t);
            break;
        case 2:
            NetTask5_Failed(t);
            break;
        }
    }
}

void NetTask5_Steps(NetTask *t)
{
    switch (t->step) {
    case 0:
        NetTask5_Step0(t);
        break;
    case 1:
        NetTask5_Step1(t);
        break;
    case 2:
        NetTask5_Step2(t);
        break;
    }
}

void NetTask5_Done(NetTask *t)
{
    NetMsg m;

    t->active = 0;
    t->state = 0;
    m.kind = 0;
    t->onDone(m, &m);
}

void NetTask5_Failed(NetTask *t)
{
    NetMsg m;

    t->active = 0;
    t->state = 0;
    m.kind = -1;
    t->onDone(m, &m);
}
