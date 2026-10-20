/*
 * The third-level entry records (gLobbyNet.recs2) and the entry's users.
 * It opens with 660C's reply (the end of task 7's settings) and the
 * notifications of users entering and leaving an entry (6503, 6502).
 * NetLevel2RecsTask (task 6) fetches the records in three stages: stage 0
 * asks 6603 for the count and 6607 for each record's sub-entry count;
 * stage 1 asks 6601, 6602 and 660F what may be sent; stage 2 asks 6604,
 * 6606 and 6605 for each record, then 6608 and 660E for each sub-entry.
 * Each field's request follows, with its packet and reply; then 6405 (an
 * entry's byte), 640A (the users of an entry), 6409, 6412-6414, 6415 (the
 * entry's four bytes) and the notification 6505 (a kick).
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
#include "m2c_macros.h"

void NetLevel2RecsTask_Run(s32 task);
void NetLevel2RecsTask_State0(NetTask *t);
void NetLevel2RecsTask_State1(NetTask *t);
void NetLevel2RecsTask_State2(NetTask *t);
void NetLevel2RecsTask_Done(NetTask *t);
void NetLevel2RecsTask_Failed(NetTask *t);
void NetLevel2RecsTask_S0Step0(NetTask *t);
void NetLevel2RecsTask_S0Step1(NetTask *t);
void NetLevel2RecsTask_S0Step2(NetTask *t);
void NetLevel2RecsTask_S0Step3(NetTask *t);
void NetLevel2RecsTask_S0Step4(NetTask *t);
void NetLevel2RecsTask_S0Step5(NetTask *t);
void NetLevel2RecsTask_S1Step0(NetTask *t);
void NetLevel2RecsTask_S1Step1(NetTask *t);
void NetLevel2RecsTask_S1Step2(NetTask *t);
void NetLevel2RecsTask_S1Step3(NetTask *t);
void NetLevel2RecsTask_S1Step4(NetTask *t);
void NetLevel2RecsTask_S1Step5(NetTask *t);
void NetLevel2RecsTask_S1Step6(NetTask *t);
void NetLevel2RecsTask_S2Step0(NetTask *t);
void NetLevel2RecsTask_S2Step1(NetTask *t);
void NetLevel2RecsTask_S2Step2(NetTask *t);
void NetLevel2RecsTask_S2Step3(NetTask *t);
void NetLevel2RecsTask_S2Step4(NetTask *t);
void NetLevel2RecsTask_S2Step5(NetTask *t);
void LobbyNotify_UserInOut(u8 kind);
char *strcpy(char *, const char *);
s32 NetReq_CountFree(void);
u8 *LobbyMsg_ReadU8(void *dst);
u8 *LobbyMsg_ReadU16U8(void *a, void *b);
void LobbyMsg_ReadUsers(u8 *n, NetUserRec *users);
s32 LobbyReq_Level2RecName(u16 id, u8 i, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_Level2RecChoiceCount(u16 id, u8 i, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_Level2RecDefault(u16 id, u8 i, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_Level2RecEditable(u16 id, u8 i, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_Level2RecChoiceName(u16 id, u8 i, u8 j, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_Level2RecChoiceValues(u16 id, u8 i, u8 j, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_Level2TextEditable(u16 id, void (*onReply)(NetMsg, NetMsg *));
void NetLevel2RecsTask_OnReply(NetMsg m, NetMsg *pm);
void NetLevel2RecsTask_OnRecReply(NetMsg m, NetMsg *pm);
s32 LobbyReq_Level2RecCount(u16 id, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyPkt_Level2RecCount(u16 id);
s32 LobbyReq_Level2NameEditable(u16 id, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyPkt_Level2NameEditable(u16 id);
s32 LobbyReq_Level2PassEditable(s32 id, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyPkt_Level2PassEditable(u16 id);
s32 LobbyPkt_Level2Locked(u16 id);
s32 LobbyPkt_Level2Users(u16 id);
void LobbyReply_Level2Users_Users(void);
u8 *LobbyMsg_ReadU8Str(void *a, void *b);
u8 *LobbyMsg_ReadU8U8(void *a, void *b);
u8 *LobbyMsg_ReadU16U16(void *a, void *b);
u8 *LobbyMsg_ReadU8U8Str(void *a, void *b, void *c);
s32 memcmp(void *a, void *b, u32 n);  /* memcmp */
s32 LobbyPkt_Level2RecName(u16 id, u8 i);
s32 LobbyPkt_Level2RecChoiceCount(u16 id, u8 i);
s32 LobbyPkt_Level2RecDefault(u16 id, u8 i);
s32 LobbyPkt_Level2RecEditable(u16 id, u8 i);
s32 LobbyPkt_Level2RecChoiceName(u16 id, u8 i, u8 j);
s32 LobbyPkt_Level2RecChoiceValues(u16 id, u8 i, u8 j);
void LobbyMsg_ReadU8U8Rest(u8 *i, u8 *j, u8 **p);
s32 LobbyPkt_Level2Members(u16 id);
s32 LobbyPkt_Level2TextEditable(u16 id);
void LobbyMsg_ReadErrText(void);
s32 LobbyPkt_Level2Quest(u16 id, u8 *c);

/* the reply to 660C */
void LobbyReply_Level2SettingsEnd(void)
{
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/* the user who entered or left last */
s32 LobbyNet_GetUserInOut(NetUser *dst)
{
    *dst = gLobbyNet.userInOut;
    return 0;
}

/* the notification 6503: a user entered the third-level entry */
void LobbyNotify_Level2UserIn(void)
{
    LobbyNotify_UserInOut(2);
}

/* the notification 6502: a user left it */
void LobbyNotify_Level2UserOut(void)
{
    LobbyNotify_UserInOut(3);
}

/* task 6: fetches the records of third-level entry id */
s32 NetLevel2RecsTask_Start(u16 id, void (*onDone)(NetMsg, NetMsg *))
{
    NetTask *netTask = &gLobbyNet.task[NET_TASK_LEVEL2_RECS];

    if (netTask->active == 0) {
        memset(netTask, 0, sizeof(NetTask));
        netTask->active = 1;
        netTask->onDone = onDone;
        netTask->func = NetLevel2RecsTask_Run;
        netTask->arg = id;
        return 0;
    }
    return -1;
}

/* task 6: its dispatcher (three stages of steps) */
void NetLevel2RecsTask_Run(s32 task)
{
    NetTask *netTask = &gLobbyNet.task[NET_TASK_LEVEL2_RECS];

    if (netTask->active != 0) {
        switch (netTask->state) {
        case RECS_STAGE_COUNTS:
            NetLevel2RecsTask_State0(netTask);
            break;
        case RECS_STAGE_SENDABLE:
            NetLevel2RecsTask_State1(netTask);
            break;
        case RECS_STAGE_FIELDS:
            NetLevel2RecsTask_State2(netTask);
            break;
        case NET_TASK_END_DONE:
            NetLevel2RecsTask_Done(netTask);
            break;
        case NET_TASK_END_FAILED:
            NetLevel2RecsTask_Failed(netTask);
            break;
        }
    }
}

/* task 6, stage 0: what may be sent and the count */
void NetLevel2RecsTask_State0(NetTask *task)
{
    switch (task->step) {
    case 0:
        NetLevel2RecsTask_S0Step0(task);
        return;
    case 1:
        NetLevel2RecsTask_S0Step1(task);
        return;
    case 2:
        NetLevel2RecsTask_S0Step2(task);
        return;
    case 3:
        NetLevel2RecsTask_S0Step3(task);
        return;
    case 4:
        NetLevel2RecsTask_S0Step4(task);
        return;
    case 5:
        NetLevel2RecsTask_S0Step5(task);
    default:
        return;
    }
}

/* task 6, stage 1 */
void NetLevel2RecsTask_State1(NetTask *task)
{
    switch (task->step) {
    case 0:
        NetLevel2RecsTask_S1Step0(task);
        break;
    case 1:
        NetLevel2RecsTask_S1Step1(task);
        break;
    case 2:
        NetLevel2RecsTask_S1Step2(task);
        break;
    case 3:
        NetLevel2RecsTask_S1Step3(task);
        break;
    case 4:
        NetLevel2RecsTask_S1Step4(task);
        break;
    case 5:
        NetLevel2RecsTask_S1Step5(task);
        break;
    case 6:
        NetLevel2RecsTask_S1Step6(task);
        break;
    }
}

/* task 6, stage 2 */
void NetLevel2RecsTask_State2(NetTask *task)
{
    switch (task->step) {
    case 0:
        NetLevel2RecsTask_S2Step0(task);
        return;
    case 1:
        NetLevel2RecsTask_S2Step1(task);
        return;
    case 2:
        NetLevel2RecsTask_S2Step2(task);
        return;
    case 3:
        NetLevel2RecsTask_S2Step3(task);
        return;
    case 4:
        NetLevel2RecsTask_S2Step4(task);
        return;
    case 5:
        NetLevel2RecsTask_S2Step5(task);
    default:
        return;
    }
}

/* task 6 done */
void NetLevel2RecsTask_Done(NetTask *task)
{
    NetMsg msg;

    task->active = 0;
    task->state = NET_TASK_RUNNING;
    msg.kind = 0;
    task->onDone(msg, &msg);
}

/* task 6 failed */
void NetLevel2RecsTask_Failed(NetTask *task)
{
    NetMsg msg;

    task->active = 0;
    task->state = NET_TASK_RUNNING;
    msg.kind = -1;
    task->onDone(msg, &msg);
}

/* stage 0, step 0: clears the records */
void NetLevel2RecsTask_S0Step0(NetTask *task)
{
    task->step++;
    task->idx = 1;
    task->pos = 1;
    task->reply = NET_REPLY_NONE;
    gLobbyNet.nReplies = 0;
    memset(&gLobbyNet.recs2, 0, sizeof(NetRecs2));
}

/* 6603: the count */
void NetLevel2RecsTask_S0Step1(NetTask *task)
{
    if (NetReq_CountFree() > 0) {
        task->step++;
        LobbyReq_Level2RecCount(task->arg, NetLevel2RecsTask_OnReply);
    }
}

/* Waits for 6603's reply: with no records, on to stage 1 (RECS_STAGE_SENDABLE);
 * else the next step. An error fails the task. */
void NetLevel2RecsTask_S0Step2(NetTask *task)
{
    if (task->reply == NET_REPLY_OK) {
        task->reply = NET_REPLY_NONE;
        if (gLobbyNet.recs2.n == 0) {
            task->state = RECS_STAGE_SENDABLE;
            task->step = 0;
        } else {
            task->step++;
        }
    } else if (task->reply == NET_REPLY_ERROR) {
        task->state = NET_TASK_END_FAILED;
        task->step = 0;
    }
}

/* Starts the loop over the records at record 0. */
void NetLevel2RecsTask_S0Step3(NetTask *task)
{
    task->step++;
    task->idx = 0;
    task->pos = 0;
}

/* 6607 for each record, one a frame */
void NetLevel2RecsTask_S0Step4(NetTask *task)
{
    u32 i = task->pos;

    if (NetReq_CountFree() > 0) {
        LobbyReq_Level2RecChoiceCount(task->arg, i, NetLevel2RecsTask_OnRecReply);
        task->pos++;
        if (i + 1 >= gLobbyNet.recs2.n) {
            task->step++;
        }
    }
}

/* waits for every record's 6607 reply, then clears the flags */
void NetLevel2RecsTask_S0Step5(NetTask *task)
{
    s32 ok = 1;
    u32 i;
    s32 k;

    for (i = 0; i < gLobbyNet.recs2.n; i++) {
        if ((gLobbyNet.recs2.rec[i].flags & 0xF) != 4) {
            ok = 0;
            break;
        }
    }
    if (ok) {
        task->state++;
        task->step = 0;
        task->idx = 0;
        for (k = 0; k < 32; k++) {
            gLobbyNet.recs2.rec[k].flags = 0;
        }
    }
}

/* stage 1, step 0: tells onDone how many replies are coming */
void NetLevel2RecsTask_S1Step0(NetTask *task)
{
    NetMsg msg;
    s32 sum = 0;
    s32 i;
    u8 n;

    task->step++;
    n = gLobbyNet.recs2.n;
    for (i = 0; i < n; i++) {
        sum += gLobbyNet.recs2.rec[i].subCount;
    }
    gLobbyNet.nReplies = n * 3 + 2 + sum * 2;
    msg.kind = 2;
    msg.flag = NET_PROGRESS_LIST_COUNT;
    task->onDone(msg, &msg);
}

/* 6601: may the name be sent */
void NetLevel2RecsTask_S1Step1(NetTask *task)
{
    if (NetReq_CountFree() > 0) {
        task->step++;
        LobbyReq_Level2NameEditable(task->arg, NetLevel2RecsTask_OnReply);
    }
}

/* Waits for the reply to the previous step's request, then tells onDone of
 * the progress (kind 2, NET_PROGRESS_LIST_REPLY) and goes on. */
void NetLevel2RecsTask_S1Step2(NetTask *task)
{
    NetMsg msg;

    if (task->reply != NET_REPLY_NONE) {
        task->step++;
        task->reply = NET_REPLY_NONE;
        msg.kind = 2;
        msg.flag = NET_PROGRESS_LIST_REPLY;
        task->onDone(msg, &msg);
    }
}

/* 6602: may the password be sent */
void NetLevel2RecsTask_S1Step3(NetTask *task)
{
    if (NetReq_CountFree() > 0) {
        task->step++;
        LobbyReq_Level2PassEditable(task->arg, NetLevel2RecsTask_OnReply);
    }
}

/* Waits for the reply to the previous step's request, then tells onDone of
 * the progress (kind 2, NET_PROGRESS_LIST_REPLY) and goes on. */
void NetLevel2RecsTask_S1Step4(NetTask *task)
{
    NetMsg msg;

    if (task->reply != NET_REPLY_NONE) {
        task->step++;
        task->reply = NET_REPLY_NONE;
        msg.kind = 2;
        msg.flag = NET_PROGRESS_LIST_REPLY;
        task->onDone(msg, &msg);
    }
}

/* 660F: may the text be sent */
void NetLevel2RecsTask_S1Step5(NetTask *task)
{
    if (NetReq_CountFree() > 0) {
        task->step++;
        LobbyReq_Level2TextEditable(task->arg, NetLevel2RecsTask_OnReply);
    }
}

/* Waits for stage 1's last reply: with no records the task is done; else
 * on to stage 2, telling onDone of the progress. */
void NetLevel2RecsTask_S1Step6(NetTask *task)
{
    NetMsg msg;

    if (task->reply != NET_REPLY_NONE) {
        task->reply = NET_REPLY_NONE;
        if (gLobbyNet.recs2.n == 0) {
            task->state = NET_TASK_END_DONE;
            task->step = 0;
        } else {
            task->state++;
            task->step = 0;
            msg.kind = 2;
            msg.flag = NET_PROGRESS_LIST_REPLY;
            task->onDone(msg, &msg);
        }
    }
}

/* Starts the loop over the records at record 0. */
void NetLevel2RecsTask_S2Step0(NetTask *task)
{
    task->step++;
    task->idx = 0;
    task->pos = 0;
}

/* stage 2: 6604, 6606 and 6605 for each record */
void NetLevel2RecsTask_S2Step1(NetTask *task)
{
    u32 i = task->pos;

    if (NetReq_CountFree() >= 3) {
        LobbyReq_Level2RecName(task->arg, i, NetLevel2RecsTask_OnRecReply);
        LobbyReq_Level2RecDefault(task->arg, i, NetLevel2RecsTask_OnRecReply);
        LobbyReq_Level2RecEditable(task->arg, i, NetLevel2RecsTask_OnRecReply);
        task->pos++;
        if (i + 1 >= gLobbyNet.recs2.n) {
            task->step++;
        }
    }
}

/* waits for the three replies of every record, then clears the sub-entry flags */
void NetLevel2RecsTask_S2Step2(NetTask *task)
{
    s32 ok = 1;
    u32 i;
    s32 k;

    for (i = 0; i < gLobbyNet.recs2.n; i++) {
        if ((gLobbyNet.recs2.rec[i].flags & 0xF) != 0xB) {
            ok = 0;
            break;
        }
    }
    if (ok) {
        task->step++;
        task->idx = 0;
        for (k = 0; k < 32; k++) {
            memset(gLobbyNet.recs2.rec[k].subFlags, 0, 0x20);
        }
    }
}

void NetLevel2RecsTask_S2Step3(NetTask *task)
{
    task->step++;
    task->pos = 0;
}

/* 6608 and 660E for each sub-entry of record unk8 */
void NetLevel2RecsTask_S2Step4(NetTask *task)
{
    u32 j = task->pos;

    if (gLobbyNet.recs2.rec[task->idx].subCount == 0) {
        task->state = NET_TASK_END_DONE;
        task->step = 0;
    } else if (NetReq_CountFree() >= 2) {
        LobbyReq_Level2RecChoiceName(task->arg, task->idx, j, NetLevel2RecsTask_OnRecReply);
        LobbyReq_Level2RecChoiceValues(task->arg, task->idx, j, NetLevel2RecsTask_OnRecReply);
        task->pos++;
        if (j + 1 >= gLobbyNet.recs2.rec[task->idx].subCount) {
            task->step++;
        }
    }
}

/* waits for the sub-entries of record unk8, then goes on with the next record */
void NetLevel2RecsTask_S2Step5(NetTask *task)
{
    s32 ok = 1;
    s32 j;

    for (j = 0; j < gLobbyNet.recs2.rec[task->idx].subCount; j++) {
        if ((gLobbyNet.recs2.rec[task->idx].subFlags[j] & 3) != 3) {
            ok = 0;
        }
    }
    if (ok) {
        task->idx++;
        if (task->idx >= gLobbyNet.recs2.n) {
            task->state = NET_TASK_END_DONE;
            task->step = 0;
        } else {
            task->step = 3;
        }
    }
}

/* the replies to task 6's requests */
void NetLevel2RecsTask_OnReply(NetMsg msg, NetMsg *msgRef)
{
    NetTask *netTask = &gLobbyNet.task[NET_TASK_LEVEL2_RECS];

    if (msg.kind == 0) {
        netTask->reply = NET_REPLY_OK;
    } else {
        netTask->reply = NET_REPLY_ERROR;
    }
}

/* the replies to the per-record requests: one more reply for onDone */
void NetLevel2RecsTask_OnRecReply(NetMsg msg, NetMsg *msgRef)
{
    NetTask *netTask = &gLobbyNet.task[NET_TASK_LEVEL2_RECS];
    NetMsg progress;

    if (msg.kind == 0) {
        progress.kind = 2;
        progress.flag = NET_PROGRESS_LIST_REPLY;
        netTask->onDone(progress, &progress);
        netTask->reply = NET_REPLY_OK;
    } else {
        netTask->reply = NET_REPLY_ERROR;
    }
}

/* 6603 for third-level entry id */
s32 LobbyReq_Level2RecCount(u16 id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_Level2RecCount(id);
        return i;
    }
    return -1;
}

/* 6603: u16 id */
s32 LobbyPkt_Level2RecCount(u16 id)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL2_REC_COUNT);
    LobbyPkt_PutU16(&gLobbyPkt, id);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 6603: u8, the count */
void LobbyReply_Level2RecCount(void)
{
    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU8(&gLobbyNet.recs2.n);
    }
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/* 6601 for third-level entry id */
s32 LobbyReq_Level2NameEditable(u16 id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_Level2NameEditable(id);
        return i;
    }
    return -1;
}

/* 6601: u16 id */
s32 LobbyPkt_Level2NameEditable(u16 id)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL2_NAME_EDITABLE);
    LobbyPkt_PutU16(&gLobbyPkt, id);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 6601: u8, whether the name may be sent */
void LobbyReply_Level2NameEditable(void)
{
    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU8(&gLobbyNet.recs2.sendName);
    }
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/* 6602 for third-level entry id */
s32 LobbyReq_Level2PassEditable(s32 id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_Level2PassEditable(id);
        return i;
    }
    return -1;
}

/* the reply to 6602: u8, whether the password may be sent */
void LobbyReply_Level2PassEditable(void)
{
    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU8(&gLobbyNet.recs2.sendPass);
    }
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/* 6602: u16 id */
s32 LobbyPkt_Level2PassEditable(u16 id)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL2_PASS_EDITABLE);
    LobbyPkt_PutU16(&gLobbyPkt, id);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* 6405 for third-level entry id */
s32 LobbyReq_Level2Locked(u16 id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_Level2Locked(id);
        return i;
    }
    return -1;
}

/* the byte 6405 gave entry id */
s32 NetList2_GetLocked(u16 id, u8 *out)
{
    *out = gLobbyNet.list2[id - 1].locked;
    return 0;
}

/* 6405: u16 id */
s32 LobbyPkt_Level2Locked(u16 id)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL2_LOCKED);
    LobbyPkt_PutU16(&gLobbyPkt, id);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 6405 and its notification: u16 id, u8; route 0x22 */
void LobbyReply_Level2Locked(void)
{
    u8 value;
    u16 id;

    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU16U8(&id, &value);
        gLobbyNet.list2[id - 1].locked = value;
        gLobbyNet.list2[id - 1].flags |= NET_ENTRY_LOCKED;
        gLobbyNet.lastId = id;
    }
    Lobby_RouteMessage(NET_ROUTE_LEVEL2_LOCKED);
}

/* 640A: the users of third-level entry id */
s32 LobbyReq_Level2Users(u16 id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    memset(gLobbyNet.users2, 0, sizeof(gLobbyNet.users2));
    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_Level2Users(id);
        return i;
    }
    return -1;
}

/* user i of 640A's reply */
s32 NetList2_GetUser(u16 i, char *id, char *name, u8 *text)
{
    strcpy(id, gLobbyNet.users2[i].u.id);
    strcpy(name, gLobbyNet.users2[i].u.name);
    memcpy(text, gLobbyNet.users2[i].u.text, 0x100);
    return 0;
}

/* 640A: u16 id */
s32 LobbyPkt_Level2Users(u16 id)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL2_USERS);
    LobbyPkt_PutU16(&gLobbyPkt, id);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 640A */
void LobbyReply_Level2Users(void)
{
    if (gLobbyNet.replyErr == 0) {
        LobbyReply_Level2Users_Users();
    }
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/* 640A's users: into users2 */
void LobbyReply_Level2Users_Users(void)
{
    memset(gLobbyNet.users2, 0, sizeof(gLobbyNet.users2));
    LobbyMsg_ReadUsers(&gLobbyNet.nUsers2, gLobbyNet.users2);
}

/*
 * Sends 6604 as a request; onReply gets its reply. Returns the request's
 * slot, -1 if none is free.
 */
s32 LobbyReq_Level2RecName(u16 id, u8 i, void (*onReply)(NetMsg, NetMsg *))
{
    s32 req;

    req = NetReq_Alloc(1, 0, onReply);
    if (req != -1) {
        gLobbyNet.req[req].seq = LobbyPkt_Level2RecName(id, i);
        return req;
    }
    return -1;
}

/* 6604: u16 entry, u8 record */
s32 LobbyPkt_Level2RecName(u16 id, u8 i)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL2_REC_NAME);
    LobbyPkt_PutU16(&gLobbyPkt, id);
    LobbyPkt_PutU8(&gLobbyPkt, i);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 6604: u8 record, string name */
void LobbyReply_Level2RecName(void)
{
    char name[0x4C];
    u8 i;

    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU8Str(&i, name);
        strcpy(gLobbyNet.recs2.rec[i].name, name);
        gLobbyNet.recs2.rec[i].flags |= NET_ENTRY_VALUES;
    }
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/* 6607 for record i */
s32 LobbyReq_Level2RecChoiceCount(u16 id, u8 i, void (*onReply)(NetMsg, NetMsg *))
{
    s32 k;

    k = NetReq_Alloc(1, NULL, onReply);
    if (k != -1) {
        gLobbyNet.req[k].seq = LobbyPkt_Level2RecChoiceCount(id, i);
        return k;
    }
    return -1;
}

/* 6607: u16 entry, u8 record */
s32 LobbyPkt_Level2RecChoiceCount(u16 id, u8 i)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL2_REC_CHOICE_COUNT);
    LobbyPkt_PutU16(&gLobbyPkt, id);
    LobbyPkt_PutU8(&gLobbyPkt, i);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 6607: u8 record, u8 sub-entries */
void LobbyReply_Level2RecChoiceCount(void)
{
    u8 i;
    u8 value;

    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU8U8(&i, &value);
        gLobbyNet.recs2.rec[i].subCount = value;
        gLobbyNet.recs2.rec[i].flags |= NET_ENTRY_NAME;
    }
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/* 6606 for record i */
s32 LobbyReq_Level2RecDefault(u16 id, u8 i, void (*onReply)(NetMsg, NetMsg *))
{
    s32 k;

    k = NetReq_Alloc(1, NULL, onReply);
    if (k != -1) {
        gLobbyNet.req[k].seq = LobbyPkt_Level2RecDefault(id, i);
        return k;
    }
    return -1;
}

/* 6606: u16 entry, u8 record */
s32 LobbyPkt_Level2RecDefault(u16 id, u8 i)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL2_REC_DEFAULT);
    LobbyPkt_PutU16(&gLobbyPkt, id);
    LobbyPkt_PutU8(&gLobbyPkt, i);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 6606: u8 record, u8 */
void LobbyReply_Level2RecDefault(void)
{
    u8 i;
    u8 value;

    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU8U8(&i, &value);
        gLobbyNet.recs2.rec[i].defaultChoice = value;
        gLobbyNet.recs2.rec[i].flags |= NET_ENTRY_STATE;
    }
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/* 6605 for record i */
s32 LobbyReq_Level2RecEditable(u16 id, u8 i, void (*onReply)(NetMsg, NetMsg *))
{
    s32 k;

    k = NetReq_Alloc(1, NULL, onReply);
    if (k != -1) {
        gLobbyNet.req[k].seq = LobbyPkt_Level2RecEditable(id, i);
        return k;
    }
    return -1;
}

/* 6605: u16 entry, u8 record */
s32 LobbyPkt_Level2RecEditable(u16 id, u8 i)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL2_REC_EDITABLE);
    LobbyPkt_PutU16(&gLobbyPkt, id);
    LobbyPkt_PutU8(&gLobbyPkt, i);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 6605: u8 record, u8 */
void LobbyReply_Level2RecEditable(void)
{
    u8 i;
    u8 value;

    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU8U8(&i, &value);
        gLobbyNet.recs2.rec[i].editable = value;
        gLobbyNet.recs2.rec[i].flags |= NET_ENTRY_TEXT;
    }
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/* 6608 for sub-entry j of record i */
s32 LobbyReq_Level2RecChoiceName(u16 id, u8 i, u8 j, void (*onReply)(NetMsg, NetMsg *))
{
    s32 k;

    k = NetReq_Alloc(1, NULL, onReply);
    if (k != -1) {
        gLobbyNet.req[k].seq = LobbyPkt_Level2RecChoiceName(id, i, j);
        return k;
    }
    return -1;
}

/* 6608: u16 entry, u8 record, u8 sub-entry */
s32 LobbyPkt_Level2RecChoiceName(u16 id, u8 i, u8 j)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL2_REC_CHOICE_NAME);
    LobbyPkt_PutU16(&gLobbyPkt, id);
    LobbyPkt_PutU8(&gLobbyPkt, i);
    LobbyPkt_PutU8(&gLobbyPkt, j);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 6608: u8 record, u8 sub-entry, string name */
void LobbyReply_Level2RecChoiceName(void)
{
    u8 i;
    u8 j;
    char name[0x4C];

    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU8U8Str(&i, &j, name);
        strcpy(gLobbyNet.recs2.rec[i].subName[j], name);
        gLobbyNet.recs2.rec[i].subFlags[j] |= 1;
    }
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/* 660E for sub-entry j of record i */
s32 LobbyReq_Level2RecChoiceValues(u16 id, u8 i, u8 j, void (*onReply)(NetMsg, NetMsg *))
{
    s32 k;

    k = NetReq_Alloc(1, NULL, onReply);
    if (k != -1) {
        gLobbyNet.req[k].seq = LobbyPkt_Level2RecChoiceValues(id, i, j);
        return k;
    }
    return -1;
}

/* 660E: u16 entry, u8 record, u8 sub-entry */
s32 LobbyPkt_Level2RecChoiceValues(u16 id, u8 i, u8 j)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL2_REC_CHOICE_VALUES);
    LobbyPkt_PutU16(&gLobbyPkt, id);
    LobbyPkt_PutU8(&gLobbyPkt, i);
    LobbyPkt_PutU8(&gLobbyPkt, j);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 660E: u8 record, u8 sub-entry, u8 n, n three-byte values */
void LobbyReply_Level2RecChoiceValues(void)
{
    u8 i;
    u8 j;
    u8 *cur;
    s32 k;
    s32 n;

    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU8U8Rest(&i, &j, &cur);
        n = *cur++;
        for (k = 0; k < n; k++) {
            gLobbyNet.recs2.rec[i].subData[j][k][0] = cur[0];
            gLobbyNet.recs2.rec[i].subData[j][k][1] = cur[1];
            gLobbyNet.recs2.rec[i].subData[j][k][2] = cur[2];
            cur += 3;
        }
        gLobbyNet.recs2.rec[i].subN[j] = n;
        gLobbyNet.recs2.rec[i].subFlags[j] |= 2;
    }
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/* reads u8 i, u8 j of the message received; *p: what follows */
void LobbyMsg_ReadU8U8Rest(u8 *i, u8 *j, u8 **rest)
{
    *rest = LobbyMsg_GetU8(j, LobbyMsg_GetU8(i, gLobbyRecvData));
}

/* the reply to 6409: u16, u16 */
void LobbyReply_6409(void)
{
    u16 x;

    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU16U16(&x, &gLobbyNet.x6409);
    }
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/* 6412 for third-level entry id */
s32 LobbyReq_Level2Members(u16 id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_Level2Members(id);
        return i;
    }
    return -1;
}

/* The two values 6412 gave entry id: the room's members and a second value
 * no code reads yet; returns 0. */
s32 NetList2_GetMembers(s32 id, u16 *memberCount, u16 *extra)
{
    if (id != 0) {
        *memberCount = gLobbyNet.list2[(u16)id - 1].roomMembers;
        *extra = gLobbyNet.list2[(u16)id - 1].unk1C;
    }
    return 0;
}

/* the reply to 6412 and its notification: u16 id, u16, u16; route 0x26 */
void LobbyReply_Level2Members(void)
{
    u16 id;
    u8 *cur;

    if (gLobbyNet.replyErr == 0) {
        cur = LobbyMsg_GetU16(&id, gLobbyRecvData);
        cur = LobbyMsg_GetU16(&gLobbyNet.list2[id - 1].roomMembers, cur);
        LobbyMsg_GetU16(&gLobbyNet.list2[id - 1].unk1C, cur);
        gLobbyNet.list2[id - 1].flags |= NET_ENTRY_MEMBERS;
        gLobbyNet.lastId = id;
    }
    Lobby_RouteMessage(NET_ROUTE_LEVEL2_MEMBERS);
}

/* the same two values for the third-level entry the player is in */
void LobbyNotify_Level2MembersNotify(void)
{
    s32 id;
    u8 *rest;

    if (gLobbyNet.replyErr == 0) {
        id = gLobbyNet.cur[2];
        rest = LobbyMsg_GetU16(&gLobbyNet.list2[id - 1].roomMembers, gLobbyRecvData);
        LobbyMsg_GetU16(&gLobbyNet.list2[id - 1].unk1C, rest);
        gLobbyNet.lastId = id;
    }
    Lobby_RouteMessage(NET_ROUTE_LEVEL2_MEMBERS);
}

/* 6412: u16 id */
s32 LobbyPkt_Level2Members(u16 id)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL2_MEMBERS);
    LobbyPkt_PutU16(&gLobbyPkt, id);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 6413: u16, u8 n, n x (blob id, u8): the byte of each user of
 * users2 with that id */
void LobbyReply_6413(void)
{
    u8 n;
    u8 value;
    u16 x;
    char id[8];
    s32 k;
    s32 i;
    u8 *cur;

    if (gLobbyNet.replyErr == 0) {
        cur = LobbyMsg_GetU8(&n, LobbyMsg_GetU16(&x, gLobbyRecvData));
        if (n) {
            for (i = 0; i < n; i++) {
                memset(id, 0, 8);
                cur = LobbyMsg_GetU8(&value, LobbyMsg_GetBlob(id, 8, cur));
                for (k = 0; k < 10; k++) {
                    if (memcmp(id, gLobbyNet.users2[k].u.id, 8) == 0) {
                        gLobbyNet.users2[k].color[0] = value;
                    }
                }
            }
        }
    }
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/* the notification 6414: u16 entry, blob id, u8: one user of the entry the
 * player is in; route 0x27 */
void LobbyNotify_6414(void)
{
    u16 id;
    u8 *rest;
    NetUserRec *user;

    if (gLobbyNet.replyErr == 0) {
        memset(&gLobbyNet.user6414, 0, sizeof(NetUserRec));
        rest = LobbyMsg_GetU16(&id, gLobbyRecvData);
        if (id != gLobbyNet.cur[2]) {
            return;
        }
        user = gLobbyNet.users2;
        gLobbyNet.nUsers2 = 1;
        LobbyMsg_GetU8(user->color, LobbyMsg_GetBlob(user->u.id, 8, rest));
    }
    Lobby_RouteMessage(NET_ROUTE_6414);
}

/* 660F for third-level entry id */
s32 LobbyReq_Level2TextEditable(u16 id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_Level2TextEditable(id);
        return i;
    }
    return -1;
}

/* 660F: u16 id */
s32 LobbyPkt_Level2TextEditable(u16 id)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL2_TEXT_EDITABLE);
    LobbyPkt_PutU16(&gLobbyPkt, id);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 660F: u8, whether the text may be sent */
void LobbyReply_Level2TextEditable(void)
{
    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU8(&gLobbyNet.recs2.sendText);
    }
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/* 6415 for third-level entry id */
s32 LobbyReq_Level2Quest(u16 id, u8 *values, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_Level2Quest(id, values);
        return i;
    }
    return -1;
}

/* 6415: u16 id, u8 x4 */
s32 LobbyPkt_Level2Quest(u16 id, u8 *values)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL2_QUEST);
    LobbyPkt_PutU16(&gLobbyPkt, id);
    LobbyPkt_PutU8(&gLobbyPkt, values[0]);
    LobbyPkt_PutU8(&gLobbyPkt, values[1]);
    LobbyPkt_PutU8(&gLobbyPkt, values[2]);
    LobbyPkt_PutU8(&gLobbyPkt, values[3]);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 6415 */
void LobbyReply_Level2Quest(void)
{
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/* the notification 6505: an error text; route 0xA */
void LobbyNotify_Level2Kick(void)
{
    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadErrText();
    }
    Lobby_RouteMessage(NET_ROUTE_LEVEL2_KICK);
}
