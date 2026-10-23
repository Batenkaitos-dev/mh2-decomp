/*
 * Entering a third-level entry (6406, with its password), leaving it
 * (6501) and opening one (6407); then NetLevel2SettingsTask (task 7),
 * which sends the settings of the third-level entry the player is in
 * (gLobbyNet.settings2): 6609 the name, 660A the password and 6610 the
 * text (each only if the server allowed it: gLobbyNet.recs2.send*), 660B
 * one byte per record, 6415 four bytes, then 660C, each with its request
 * and reply.
 * Network code: it stays byte for byte, as the whole build does.
 */
#include "common.h"
#include "lobby_net.h"
#include "item.h"
#include "scene.h"
#include "em.h"
#include "player.h"
#include "save.h"
#include "pad.h"

struct LobbyPkt;

/* the reply to 6406: the third-level entry was entered */
void LobbyReply_Level2Enter(void)
{
    if (gLobbyNet.replyErr == 0) {
        NetList_Enter(2);
    }
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/* 6406: enters third-level entry id (with its password) */
s32 LobbyReq_Level2Enter(u16 id, char *pass, u8 value, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        NetList_Select(2, id);
        gLobbyNet.req[i].seq = LobbyPkt_Level2Enter(id, pass, value);
        return i;
    }
    return -1;
}

/* 6406: u16 id, blob password, u8 */
s32 LobbyPkt_Level2Enter(u16 id, char *pass, u8 value)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL2_ENTER);
    LobbyPkt_PutU16(&gLobbyPkt, id);
    if (pass == NULL) {
        LobbyPkt_PutBlob(&gLobbyPkt, "", 0);
    } else {
        LobbyPkt_PutBlob(&gLobbyPkt, pass, strlen(pass));
    }
    LobbyPkt_PutU8(&gLobbyPkt, value);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* 6501: leaves the third-level entry */
s32 LobbyReq_Level2Leave(void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_Leave(2);
        return i;
    }
    return -1;
}

/* the reply to 6501 */
void LobbyReply_Level2Leave(void)
{
    if (gLobbyNet.replyErr == 0) {
        NetList_Leave(2);
    }
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/* 6407: enters third-level entry id */
s32 LobbyReq_Level2Open(u16 id, u8 value, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        NetList_Select(2, id);
        gLobbyNet.req[i].seq = LobbyPkt_Level2Open(id, value);
        return i;
    }
    return -1;
}

/* 6407: u16 id, u8 */
s32 LobbyPkt_Level2Open(u16 id, u8 value)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL2_OPEN);
    LobbyPkt_PutU16(&gLobbyPkt, id);
    LobbyPkt_PutU8(&gLobbyPkt, value);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 6407 */
void LobbyReply_Level2Open(void)
{
    if (gLobbyNet.replyErr == 0) {
        NetList_Enter(2);
    }
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/* task 7: sends these settings for the third-level entry the player is in */
s32 NetLevel2SettingsTask_Start(NetEntrySettings2 settings, void (*onDone)(NetMsg, NetMsg *))
{
    NetTask *netTask = &gLobbyNet.task[NET_TASK_LEVEL2_SETTINGS];

    if (netTask->active == 0) {
        memset(netTask, 0, sizeof(NetTask));
        gLobbyNet.settings2 = settings;
        netTask->active = 1;
        netTask->onDone = onDone;
        netTask->func = NetLevel2SettingsTask_Run;
        return 0;
    }
    return -1;
}

/* task 7: its dispatcher */
void NetLevel2SettingsTask_Run(s32 task)
{
    NetTask *netTask = &gLobbyNet.task[NET_TASK_LEVEL2_SETTINGS];

    if (netTask->active != 0) {
        switch (netTask->state) {
        case NET_TASK_RUNNING:
            NetLevel2SettingsTask_Steps(netTask);
            break;
        case NET_TASK_END_DONE:
            NetLevel2SettingsTask_Done(netTask);
            break;
        case NET_TASK_END_FAILED:
            NetLevel2SettingsTask_Failed(netTask);
            break;
        }
    }
}

/* task 7: its steps */
void NetLevel2SettingsTask_Steps(NetTask *task)
{
    switch (task->step) {
    case 0:
        NetLevel2SettingsTask_Step0(task);
        break;
    case 1:
        NetLevel2SettingsTask_Step1(task);
        break;
    case 2:
        NetLevel2SettingsTask_Step2(task);
        break;
    case 3:
        NetLevel2SettingsTask_Step3(task);
        break;
    case 4:
        NetLevel2SettingsTask_Step4(task);
        break;
    case 5:
        NetLevel2SettingsTask_Step5(task);
        break;
    case 6:
        NetLevel2SettingsTask_Step6(task);
        break;
    case 7:
        NetLevel2SettingsTask_Step7(task);
        break;
    case 8:
        NetLevel2SettingsTask_Step8(task);
        break;
    case 9:
        NetLevel2SettingsTask_Step9(task);
        break;
    case 10:
        NetLevel2SettingsTask_Step10(task);
        break;
    case 11:
        NetLevel2SettingsTask_Step11(task);
        break;
    case 12:
        NetLevel2SettingsTask_Step12(task);
        break;
    }
}

/* task 7 done */
void NetLevel2SettingsTask_Done(NetTask *task)
{
    NetMsg msg;

    task->active = 0;
    task->state = NET_TASK_RUNNING;
    msg.kind = 0;
    task->onDone(msg, &msg);
}

/* task 7 failed */
void NetLevel2SettingsTask_Failed(NetTask *task)
{
    NetMsg msg;

    task->active = 0;
    task->state = NET_TASK_RUNNING;
    msg.kind = -1;
    task->onDone(msg, &msg);
}

void NetLevel2SettingsTask_Step0(NetTask *task)
{
    task->step++;
    task->reply = NET_REPLY_NONE;
}

/* 6609: the name, if there is one and the server allows it */
void NetLevel2SettingsTask_Step1(NetTask *task)
{
    if (gLobbyNet.settings2.name[0] != 0 && gLobbyNet.recs2.sendName == 1) {
        if (NetReq_CountFree() > 0) {
            task->step++;
            LobbyReq_Level2SetName(gLobbyNet.settings2.name, NetLevel2SettingsTask_OnReply);
        }
    } else {
        task->step = 3;
    }
}

/* Waits for the reply to the previous step's request: on OK the next step,
 * on an error the task fails. */
void NetLevel2SettingsTask_Step2(NetTask *task)
{
    if (task->reply == NET_REPLY_OK) {
        task->step++;
        task->reply = NET_REPLY_NONE;
    } else if (task->reply == NET_REPLY_ERROR) {
        task->state = NET_TASK_END_FAILED;
        task->step = 0;
    }
}

/* 660A: the password */
void NetLevel2SettingsTask_Step3(NetTask *task)
{
    if (gLobbyNet.settings2.pass[0] != 0 && gLobbyNet.recs2.sendPass == 1) {
        if (NetReq_CountFree() > 0) {
            task->step++;
            LobbyReq_Level2SetPass(gLobbyNet.settings2.pass, NetLevel2SettingsTask_OnReply);
        }
    } else {
        task->step = 5;
    }
}

/* Waits for the reply to the previous step's request: on OK the next step,
 * on an error the task fails. */
void NetLevel2SettingsTask_Step4(NetTask *task)
{
    if (task->reply == NET_REPLY_OK) {
        task->step++;
        task->reply = NET_REPLY_NONE;
    } else if (task->reply == NET_REPLY_ERROR) {
        task->state = NET_TASK_END_FAILED;
        task->step = 0;
    }
}

/* 660B: the byte of each record the server allows */
void NetLevel2SettingsTask_Step5(NetTask *task)
{
    s32 i;

    task->step++;
    for (i = 0; i < gLobbyNet.recs2.n; i++) {
        if (gLobbyNet.recs2.rec[i].editable == 1) {
            LobbyPkt_Level2SetRecChoice(i, gLobbyNet.settings2.b[i]);
        }
    }
}

void NetLevel2SettingsTask_Step6(NetTask *task)
{
    task->step++;
}

/* 6610: the text */
void NetLevel2SettingsTask_Step7(NetTask *task)
{
    if (gLobbyNet.settings2.text[0] != 0 && gLobbyNet.recs2.sendText == 1) {
        if (NetReq_CountFree() > 0) {
            task->step++;
            LobbyReq_Level2SetText(gLobbyNet.settings2.text, strlen(gLobbyNet.settings2.text), NetLevel2SettingsTask_OnReply);
        }
    } else {
        task->step = 9;
    }
}

/* Waits for the reply to the previous step's request: on OK the next step,
 * on an error the task fails. */
void NetLevel2SettingsTask_Step8(NetTask *task)
{
    if (task->reply == NET_REPLY_OK) {
        task->step++;
        task->reply = NET_REPLY_NONE;
    } else if (task->reply == NET_REPLY_ERROR) {
        task->state = NET_TASK_END_FAILED;
        task->step = 0;
    }
}

/* 6415: the four bytes */
void NetLevel2SettingsTask_Step9(NetTask *task)
{
    if (NetReq_CountFree() > 0) {
        task->step++;
        LobbyReq_Level2Quest(gLobbyNet.cur[2], gLobbyNet.settings2.c, NetLevel2SettingsTask_OnReply);
    }
}

/* Waits for the reply to the previous step's request: on OK the next step,
 * on an error the task fails. */
void NetLevel2SettingsTask_Step10(NetTask *task)
{
    if (task->reply == NET_REPLY_OK) {
        task->step++;
        task->reply = NET_REPLY_NONE;
    } else if (task->reply == NET_REPLY_ERROR) {
        task->state = NET_TASK_END_FAILED;
        task->step = 0;
    }
}

/* 660C */
void NetLevel2SettingsTask_Step11(NetTask *task)
{
    if (NetReq_CountFree() > 0) {
        task->step++;
        LobbyReq_Level2SettingsEnd(NetLevel2SettingsTask_OnReply);
    }
}

/* Waits for the reply to the previous step's request: on OK the next step,
 * on an error the task fails. */
void NetLevel2SettingsTask_Step12(NetTask *task)
{
    if (task->reply == NET_REPLY_OK) {
        task->state = NET_TASK_END_DONE;
        task->step = 0;
    } else if (task->reply == NET_REPLY_ERROR) {
        task->state = NET_TASK_END_FAILED;
        task->step = 0;
    }
}

/* the replies to task 7's requests */
void NetLevel2SettingsTask_OnReply(NetMsg msg, NetMsg *msgRef)
{
    NetTask *netTask = &gLobbyNet.task[NET_TASK_LEVEL2_SETTINGS];

    if (msg.kind == 0) {
        netTask->reply = NET_REPLY_OK;
    } else {
        netTask->reply = NET_REPLY_ERROR;
    }
}

/* 660B: u8 record, u8 */
s32 LobbyPkt_Level2SetRecChoice(u8 i, u8 value)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL2_SET_REC_CHOICE);
    LobbyPkt_PutU8(&gLobbyPkt, i);
    LobbyPkt_PutU8(&gLobbyPkt, value);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 660B */
void LobbyReply_Level2SetRecChoice(void)
{
}

/*
 * Sends 6609 as a request; onReply gets its reply. Returns the request's
 * slot, -1 if none is free.
 */
s32 LobbyReq_Level2SetName(char *src, void (*onReply)(NetMsg, NetMsg *))
{
    s32 req;

    req = NetReq_Alloc(1, 0, onReply);
    if (req != -1) {
        gLobbyNet.req[req].seq = LobbyPkt_Level2SetName(src);
        return req;
    }
    return -1;
}

/* Sends 6609; returns its sequence number. */
s32 LobbyPkt_Level2SetName(char *src)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL2_SET_NAME) & 0xFFFF;
    LobbyPkt_PutBlob(&gLobbyPkt, src, strlen(src) & 0xFFFF);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* The reply to 6609: completes its request. */
void LobbyReply_Level2SetName(void)
{
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/*
 * Sends 660A as a request; onReply gets its reply. Returns the request's
 * slot, -1 if none is free.
 */
s32 LobbyReq_Level2SetPass(char *src, void (*onReply)(NetMsg, NetMsg *))
{
    s32 req;

    req = NetReq_Alloc(1, 0, onReply);
    if (req != -1) {
        gLobbyNet.req[req].seq = LobbyPkt_Level2SetPass(src);
        return req;
    }
    return -1;
}

/* Sends 660A; returns its sequence number. */
s32 LobbyPkt_Level2SetPass(char *src)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL2_SET_PASS) & 0xFFFF;
    LobbyPkt_PutBlob(&gLobbyPkt, src, strlen(src) & 0xFFFF);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* The reply to 660A: completes its request. */
void LobbyReply_Level2SetPass(void)
{
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/*
 * Sends 6610 as a request; onReply gets its reply. Returns the request's
 * slot, -1 if none is free.
 */
s32 LobbyReq_Level2SetText(char *src, u16 n, void (*onReply)(NetMsg, NetMsg *))
{
    s32 req;

    req = NetReq_Alloc(1, 0, onReply);
    if (req != -1) {
        gLobbyNet.req[req].seq = LobbyPkt_Level2SetText(src, n);
        return req;
    }
    return -1;
}

/* Sends 6610; returns its sequence number. */
s32 LobbyPkt_Level2SetText(char *src, u16 n)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL2_SET_TEXT) & 0xFFFF;
    LobbyPkt_PutBlob(&gLobbyPkt, src, n);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* The reply to 6610: completes its request. */
void LobbyReply_Level2SetText(void)
{
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/*
 * Sends 660C as a request; onReply gets its reply. Returns the request's
 * slot, -1 if none is free.
 */
s32 LobbyReq_Level2SettingsEnd(void (*onReply)(NetMsg, NetMsg *))
{
    s32 req;

    req = NetReq_Alloc(1, 0, onReply);
    if (req != -1) {
        gLobbyNet.req[req].seq = LobbyPkt_Level2SettingsEnd();
        return req;
    }
    return -1;
}

/* Sends 660C; returns its sequence number. */
s32 LobbyPkt_Level2SettingsEnd(void)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL2_SETTINGS_END) & 0xFFFF;
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 660C */
void LobbyReply_Level2SettingsEnd(void)
{
    Lobby_RouteMessage(NET_ROUTE_NONE);
}
