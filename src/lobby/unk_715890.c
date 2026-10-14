/*
 * NETWORK: users entering and leaving a third-level entry (6503, 6502), and
 * task 6, which fetches the third-level entry records (gLobbyNet.recs2:
 * 6601/6602/660F what may be sent, 6603 the count, then per record
 * 6604-6608 and 660E).
 */
#include "common.h"
#include "lobby_net.h"

typedef struct LobbyRec11C {
    u8 b[0x11C];
} LobbyRec11C;

void NetTask6_Run(s32 task);
void NetTask6_State0(NetTask *t);
void NetTask6_State1(NetTask *t);
void NetTask6_State2(NetTask *t);
void NetTask6_Done(NetTask *t);
void NetTask6_Failed(NetTask *t);
void NetTask6_S0Step0(NetTask *t);
void NetTask6_S0Step1(NetTask *t);
void NetTask6_S0Step2(NetTask *t);
void NetTask6_S0Step3(NetTask *t);
void NetTask6_S0Step4(NetTask *t);
void NetTask6_S0Step5(NetTask *t);
void NetTask6_S1Step0(NetTask *t);
void NetTask6_S1Step1(NetTask *t);
void NetTask6_S1Step2(NetTask *t);
void NetTask6_S1Step3(NetTask *t);
void NetTask6_S1Step4(NetTask *t);
void NetTask6_S1Step5(NetTask *t);
void NetTask6_S1Step6(NetTask *t);
void NetTask6_S2Step0(NetTask *t);
void NetTask6_S2Step1(NetTask *t);
void NetTask6_S2Step2(NetTask *t);
void NetTask6_S2Step3(NetTask *t);
void NetTask6_S2Step4(NetTask *t);
void NetTask6_S2Step5(NetTask *t);
void LobbyNotify_UserInOut(u8 kind);

/* the reply to 660C */
void LobbyReply_660C(void)
{
    Lobby_RouteMessage(0);
}

/* the user who entered or left last */
s32 LobbyNet_GetUserInOut(NetUser *dst)
{
    *dst = gLobbyNet.userInOut;
    return 0;
}

/* the notification 6503: a user entered the third-level entry */
void LobbyNotify_6503(void)
{
    LobbyNotify_UserInOut(2);
}

/* the notification 6502: a user left it */
void LobbyNotify_6502(void)
{
    LobbyNotify_UserInOut(3);
}

/* task 6: fetches the records of third-level entry id */
s32 NetTask6_Start(u16 id, void (*onDone)(NetMsg, NetMsg *))
{
    NetTask *t = &gLobbyNet.task[6];

    if (t->active == 0) {
        memset(t, 0, sizeof(NetTask));
        t->active = 1;
        t->onDone = onDone;
        t->func = NetTask6_Run;
        t->arg = id;
        return 0;
    }
    return -1;
}

/* task 6: its dispatcher (three stages of steps) */
void NetTask6_Run(s32 task)
{
    NetTask *t = &gLobbyNet.task[6];

    if (t->active != 0) {
        switch (t->state) {
        case 0:
            NetTask6_State0(t);
            break;
        case 1:
            NetTask6_State1(t);
            break;
        case 2:
            NetTask6_State2(t);
            break;
        case 10:
            NetTask6_Done(t);
            break;
        case 11:
            NetTask6_Failed(t);
            break;
        }
    }
}

/* task 6, stage 0: what may be sent and the count */
void NetTask6_State0(NetTask *t)
{
    switch (t->step) {
    case 0:
        NetTask6_S0Step0(t);
        return;
    case 1:
        NetTask6_S0Step1(t);
        return;
    case 2:
        NetTask6_S0Step2(t);
        return;
    case 3:
        NetTask6_S0Step3(t);
        return;
    case 4:
        NetTask6_S0Step4(t);
        return;
    case 5:
        NetTask6_S0Step5(t);
    default:
        return;
    }
}

/* task 6, stage 1 */
void NetTask6_State1(NetTask *t)
{
    switch (t->step) {
    case 0:
        NetTask6_S1Step0(t);
        break;
    case 1:
        NetTask6_S1Step1(t);
        break;
    case 2:
        NetTask6_S1Step2(t);
        break;
    case 3:
        NetTask6_S1Step3(t);
        break;
    case 4:
        NetTask6_S1Step4(t);
        break;
    case 5:
        NetTask6_S1Step5(t);
        break;
    case 6:
        NetTask6_S1Step6(t);
        break;
    }
}

/* task 6, stage 2 */
void NetTask6_State2(NetTask *t)
{
    switch (t->step) {
    case 0:
        NetTask6_S2Step0(t);
        return;
    case 1:
        NetTask6_S2Step1(t);
        return;
    case 2:
        NetTask6_S2Step2(t);
        return;
    case 3:
        NetTask6_S2Step3(t);
        return;
    case 4:
        NetTask6_S2Step4(t);
        return;
    case 5:
        NetTask6_S2Step5(t);
    default:
        return;
    }
}

/* task 6 done */
void NetTask6_Done(NetTask *t)
{
    NetMsg m;

    t->active = 0;
    t->state = 0;
    m.kind = 0;
    t->onDone(m, &m);
}

/* task 6 failed */
void NetTask6_Failed(NetTask *t)
{
    NetMsg m;

    t->active = 0;
    t->state = 0;
    m.kind = -1;
    t->onDone(m, &m);
}
