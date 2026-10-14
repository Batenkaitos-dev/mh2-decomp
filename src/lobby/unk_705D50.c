/*
 * NETWORK: task 8, the settings of the second-level entry the player is in
 * (a name, an 8-character password and a text: 6315, 6316, 6317 where the
 * entry allows them, 6318's four bytes, then 6311). unk_705FB0.c and
 * unk_706230.c have its steps.
 */
#include "common.h"
#include "lobby_net.h"

void NetTask8_Run(s32 task);
void NetTask8_Steps(NetTask *t);
void NetTask8_Done(NetTask *t);
void NetTask8_Failed(NetTask *t);
void NetTask8_Step0(NetTask *t);
void NetTask8_Step1(NetTask *t);
void NetTask8_Step2(NetTask *t);
void NetTask8_Step3(NetTask *t);
void NetTask8_Step4(NetTask *t);

/* starts task 8 with the settings to send */
s32 NetTask8_Start(NetEntrySettings *p, void (*onDone)(NetMsg, NetMsg *))
{
    NetEntrySettings s = *p;
    NetTask *t = &gLobbyNet.task[8];

    if (t->active == 0) {
        memset(t, 0, sizeof(NetTask));
        gLobbyNet.settings = s;
        t->active = 1;
        t->onDone = onDone;
        t->func = NetTask8_Run;
        return 0;
    }
    return -1;
}

void NetTask8_Run(s32 task)
{
    NetTask *t = &gLobbyNet.task[8];

    if (t->active != 0) {
        switch (t->state) {
        case 0:
            NetTask8_Steps(t);
            break;
        case 1:
            NetTask8_Done(t);
            break;
        case 2:
            NetTask8_Failed(t);
            break;
        }
    }
}

void NetTask8_Steps(NetTask *t)
{
    switch (t->step) {
    case 0:
        NetTask8_Step0(t);
        break;
    case 1:
        NetTask8_Step1(t);
        break;
    case 2:
        NetTask8_Step2(t);
        break;
    case 3:
        NetTask8_Step3(t);
        break;
    case 4:
        NetTask8_Step4(t);
        break;
    }
}

void NetTask8_Done(NetTask *t)
{
    NetMsg m;

    t->active = 0;
    t->state = 0;
    m.kind = 0;
    t->onDone(m, &m);
}

void NetTask8_Failed(NetTask *t)
{
    NetMsg m;

    t->active = 0;
    t->state = 0;
    m.kind = -1;
    t->onDone(m, &m);
}
