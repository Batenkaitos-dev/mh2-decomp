/*
 * NetLevel1AllowedTask: which level-1 settings (the name, the password,
 * the text) each entry lets the player set. Its starter, dispatcher and
 * step switch, then its steps 0-2: for each entry 1..gLobbyNet.count1 (the
 * count 6301's reply gave), the requests 6312, 6313 and 6314 (one u16: the
 * entry), three at a time; their replies (u8 value, u16 entry) fill
 * gLobbyNet.rec[entry - 1] and mark gLobbyNet.flags[0..2][entry - 1].
 * Network code: it stays byte for byte, as the whole build does.
 */
#include "common.h"
#include "lobby_net.h"

void NetLevel1AllowedTask_Run(s32 task);
void NetLevel1AllowedTask_Steps(NetTask *t);
void NetLevel1AllowedTask_Done(NetTask *t);
void NetLevel1AllowedTask_Failed(NetTask *t);
void NetLevel1AllowedTask_Step0(NetTask *t);
void NetLevel1AllowedTask_Step1(NetTask *t);
void NetLevel1AllowedTask_Step2(NetTask *t);
s32 NetReq_CountFree(void);
s32 LobbyReq_Level1NameAllowed(u16 entry, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyPkt_Level1NameAllowed(u16 entry);
s32 LobbyReq_Level1PassAllowed(u16 entry, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyPkt_Level1PassAllowed(u16 entry);
s32 LobbyReq_Level1TextAllowed(u16 entry, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyPkt_Level1TextAllowed(u16 entry);
void NetLevel1AllowedTask_Step1Reply1(NetMsg m, NetMsg *pm);
void NetLevel1AllowedTask_Step1Reply2(NetMsg m, NetMsg *pm);
void NetLevel1AllowedTask_Step1Reply4(NetMsg m, NetMsg *pm);

/* starts task 5; -1 if it is already running */
s32 NetLevel1AllowedTask_Start(u16 n, void (*onDone)(NetMsg, NetMsg *))
{
    NetTask *netTask = &gLobbyNet.task[NET_TASK_LEVEL1_ALLOWED];

    if (netTask->active == 0) {
        memset(netTask, 0, sizeof(NetTask));
        netTask->active = 1;
        netTask->onDone = onDone;
        netTask->func = NetLevel1AllowedTask_Run;
        netTask->arg = n;
        return 0;
    }
    return -1;
}

/*
 * The task's frame function: runs the current step, or ends the task when
 * it is done or failed.
 */
void NetLevel1AllowedTask_Run(s32 task)
{
    NetTask *netTask = &gLobbyNet.task[NET_TASK_LEVEL1_ALLOWED];

    if (netTask->active != 0) {
        switch (netTask->state) {
        case NET_TASK_RUNNING:
            NetLevel1AllowedTask_Steps(netTask);
            break;
        case NET_TASK_DONE:
            NetLevel1AllowedTask_Done(netTask);
            break;
        case NET_TASK_FAILED:
            NetLevel1AllowedTask_Failed(netTask);
            break;
        }
    }
}

/* Runs the task's current step. */
void NetLevel1AllowedTask_Steps(NetTask *task)
{
    switch (task->step) {
    case 0:
        NetLevel1AllowedTask_Step0(task);
        break;
    case 1:
        NetLevel1AllowedTask_Step1(task);
        break;
    case 2:
        NetLevel1AllowedTask_Step2(task);
        break;
    }
}

/* Ends the task: onDone gets kind 0. */
void NetLevel1AllowedTask_Done(NetTask *task)
{
    NetMsg msg;

    task->active = 0;
    task->state = NET_TASK_RUNNING;
    msg.kind = 0;
    task->onDone(msg, &msg);
}

/* Ends the task: onDone gets kind -1. */
void NetLevel1AllowedTask_Failed(NetTask *task)
{
    NetMsg msg;

    task->active = 0;
    task->state = NET_TASK_RUNNING;
    msg.kind = -1;
    task->onDone(msg, &msg);
}

/* Step 0: clears the values and the arrival flags, and starts at entry 1. */
void NetLevel1AllowedTask_Step0(NetTask *task)
{
    task->step++;
    gLobbyNet.nReplies = 0;
    memset(gLobbyNet.rec, 0, 0xB4);
    memset(gLobbyNet.flags, 0, 0x87);
    task->idx = 1;
    task->pos = 1;
    task->reply = NET_REPLY_NONE;
}

/* Step 1: asks for the next entry's three values when three request slots are free. */
void NetLevel1AllowedTask_Step1(NetTask *task)
{
    u32 entry = task->pos;

    if (NetReq_CountFree() >= 3) {
        LobbyReq_Level1NameAllowed(entry, NetLevel1AllowedTask_Step1Reply1);
        LobbyReq_Level1PassAllowed(entry, NetLevel1AllowedTask_Step1Reply2);
        LobbyReq_Level1TextAllowed(entry, NetLevel1AllowedTask_Step1Reply4);
        task->pos++;
        if (entry + 1 > gLobbyNet.count1 || entry + 1 > 0x2D) {
            task->step++;
        }
    }
}

/*
 * Step 2: waits until every entry's three replies came; an error in any
 * fails the task.
 */
void NetLevel1AllowedTask_Step2(NetTask *task)
{
    s32 ok = 1;
    u32 j;
    u32 i = 0;

    for (j = 0; j < 0x2D; j++) {
        i++;
        if (i > gLobbyNet.count1) {
            break;
        }
        if (gLobbyNet.flags[0][i - 1] == 0) {
            ok = 0;
            break;
        }
        if (gLobbyNet.flags[1][i - 1] == 0) {
            ok = 0;
            break;
        }
        if (gLobbyNet.flags[2][i - 1] == 0) {
            ok = 0;
            break;
        }
    }
    if (task->reply & 0x80) {
        task->state = NET_TASK_FAILED;
        task->step = 0;
        task->reply = NET_REPLY_NONE;
    } else if (ok) {
        task->state++;
        task->step = 0;
    }
}

/*
 * Step 1's reply callback: sets bit 1 of the task's reply, or 0x80 when
 * the reply reports an error.
 */
void NetLevel1AllowedTask_Step1Reply1(NetMsg msg, NetMsg *msgRef)
{
    NetTask *netTask = &gLobbyNet.task[NET_TASK_LEVEL1_ALLOWED];

    if (msg.kind == 0) {
        netTask->reply |= 1;
    } else {
        netTask->reply |= 0x80;
    }
}

/*
 * Step 1's reply callback: sets bit 2 of the task's reply, or 0x80 when
 * the reply reports an error.
 */
void NetLevel1AllowedTask_Step1Reply2(NetMsg msg, NetMsg *msgRef)
{
    NetTask *netTask = &gLobbyNet.task[NET_TASK_LEVEL1_ALLOWED];

    if (msg.kind == 0) {
        netTask->reply |= 2;
    } else {
        netTask->reply |= 0x80;
    }
}

/*
 * Step 1's reply callback: sets bit 4 of the task's reply, or 0x80 when
 * the reply reports an error.
 */
void NetLevel1AllowedTask_Step1Reply4(NetMsg msg, NetMsg *msgRef)
{
    NetTask *netTask = &gLobbyNet.task[NET_TASK_LEVEL1_ALLOWED];

    if (msg.kind == 0) {
        netTask->reply |= 4;
    } else {
        netTask->reply |= 0x80;
    }
}

/* the values of entry (clamped to 1..0x2D) */
s32 LobbyNet_GetEntryVals(u16 entry, NetEntryVals *out)
{
    if (entry == 0) {
        entry = 1;
    } else if (entry > 0x2D) {
        entry = 0x2D;
    }
    *out = gLobbyNet.rec[entry - 1];
    return 0;
}

/*
 * Sends 6312 as a request; onReply gets its reply. Returns the request's
 * slot, -1 if none is free.
 */
s32 LobbyReq_Level1NameAllowed(u16 entry, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_Level1NameAllowed(entry);
        return i;
    }
    return -1;
}

/* the reply to 6312: u8 value, u16 entry */
void LobbyReply_Level1NameAllowed(void)
{
    u16 entry;
    u8 value;

    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_GetU16(&entry, LobbyMsg_GetU8(&value, gLobbyRecvData));
        if ((s32)entry < 0x2E) {
            gLobbyNet.rec[entry - 1].v[0] = value;
            gLobbyNet.flags[0][entry - 1] = 1;
        }
    }
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/* Sends 6312; returns its sequence number. */
s32 LobbyPkt_Level1NameAllowed(u16 entry)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL1_NAME_ALLOWED) & 0xFFFF;
    LobbyPkt_PutU16(&gLobbyPkt, entry);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/*
 * Sends 6313 as a request; onReply gets its reply. Returns the request's
 * slot, -1 if none is free.
 */
s32 LobbyReq_Level1PassAllowed(u16 entry, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_Level1PassAllowed(entry);
        return i;
    }
    return -1;
}

/* the reply to 6313 */
void LobbyReply_Level1PassAllowed(void)
{
    u16 entry;
    u8 value;

    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_GetU16(&entry, LobbyMsg_GetU8(&value, gLobbyRecvData));
        if ((s32)entry < 0x2E) {
            gLobbyNet.rec[entry - 1].v[1] = value;
            gLobbyNet.flags[1][entry - 1] = 1;
        }
    }
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/* Sends 6313; returns its sequence number. */
s32 LobbyPkt_Level1PassAllowed(u16 entry)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL1_PASS_ALLOWED) & 0xFFFF;
    LobbyPkt_PutU16(&gLobbyPkt, entry);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/*
 * Sends 6314 as a request; onReply gets its reply. Returns the request's
 * slot, -1 if none is free.
 */
s32 LobbyReq_Level1TextAllowed(u16 entry, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_Level1TextAllowed(entry);
        return i;
    }
    return -1;
}

/* the reply to 6314 */
void LobbyReply_Level1TextAllowed(void)
{
    u16 entry;
    u8 value;

    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_GetU16(&entry, LobbyMsg_GetU8(&value, gLobbyRecvData));
        if ((s32)entry < 0x2E) {
            gLobbyNet.rec[entry - 1].v[2] = value;
            gLobbyNet.flags[2][entry - 1] = 1;
        }
    }
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/* Sends 6314; returns its sequence number. */
s32 LobbyPkt_Level1TextAllowed(u16 entry)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL1_TEXT_ALLOWED) & 0xFFFF;
    LobbyPkt_PutU16(&gLobbyPkt, entry);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}
