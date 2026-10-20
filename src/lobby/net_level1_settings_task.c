/*
 * NetLevel1SettingsTask: sends the settings of the level-1 entry the player
 * is in (a name, an 8-character password and a text: 6315, 6316, 6317
 * where the entry allows them, 6318's four bytes, then 6311). Its starter,
 * dispatcher and step switch, its steps 0-4 and the steps' reply
 * callbacks. Network code: it stays byte for byte, as the whole build does.
 */
#include "common.h"
#include "lobby_net.h"

void NetLevel1SettingsTask_Run(s32 task);
void NetLevel1SettingsTask_Steps(NetTask *t);
void NetLevel1SettingsTask_Done(NetTask *t);
void NetLevel1SettingsTask_Failed(NetTask *t);
void NetLevel1SettingsTask_Step0(NetTask *t);
void NetLevel1SettingsTask_Step1(NetTask *t);
void NetLevel1SettingsTask_Step2(NetTask *t);
void NetLevel1SettingsTask_Step3(NetTask *t);
void NetLevel1SettingsTask_Step4(NetTask *t);
s32 LobbyReq_Level1SetName(char *s, u16 n, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_Level1SetPass(s8 *arg0, u16 arg1, void (*arg2)(NetMsg, NetMsg *));
s32 LobbyReq_Level1SetText(s8 *arg0, u16 arg1, void (*arg2)(NetMsg, NetMsg *));
s32 LobbyReq_Level1SetGroupInfo(u16 arg0, u8 *arg1, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_Level1SettingsEnd(void (*onReply)(NetMsg, NetMsg *));
void NetLevel1SettingsTask_Step1Reply1(NetMsg m, NetMsg *pm);
void NetLevel1SettingsTask_Step1Reply2(NetMsg m, NetMsg *pm);
void NetLevel1SettingsTask_Step1Reply4(NetMsg m, NetMsg *pm);
void NetLevel1SettingsTask_Step3Reply(NetMsg m, NetMsg *pm);
void NetLevel1SettingsTask_Step1Reply8(NetMsg m, NetMsg *pm);
s32 NetReq_CountFree(void);

/* starts task 8 with the settings to send */
s32 NetLevel1SettingsTask_Start(NetEntrySettings *src, void (*onDone)(NetMsg, NetMsg *))
{
    NetEntrySettings copy = *src;
    NetTask *netTask = &gLobbyNet.task[NET_TASK_LEVEL1_SETTINGS];

    if (netTask->active == 0) {
        memset(netTask, 0, sizeof(NetTask));
        gLobbyNet.settings = copy;
        netTask->active = 1;
        netTask->onDone = onDone;
        netTask->func = NetLevel1SettingsTask_Run;
        return 0;
    }
    return -1;
}

/*
 * The task's frame function: runs the current step, or ends the task when
 * it is done or failed.
 */
void NetLevel1SettingsTask_Run(s32 task)
{
    NetTask *netTask = &gLobbyNet.task[NET_TASK_LEVEL1_SETTINGS];

    if (netTask->active != 0) {
        switch (netTask->state) {
        case NET_TASK_RUNNING:
            NetLevel1SettingsTask_Steps(netTask);
            break;
        case NET_TASK_DONE:
            NetLevel1SettingsTask_Done(netTask);
            break;
        case NET_TASK_FAILED:
            NetLevel1SettingsTask_Failed(netTask);
            break;
        }
    }
}

/* Runs the task's current step. */
void NetLevel1SettingsTask_Steps(NetTask *task)
{
    switch (task->step) {
    case 0:
        NetLevel1SettingsTask_Step0(task);
        break;
    case 1:
        NetLevel1SettingsTask_Step1(task);
        break;
    case 2:
        NetLevel1SettingsTask_Step2(task);
        break;
    case 3:
        NetLevel1SettingsTask_Step3(task);
        break;
    case 4:
        NetLevel1SettingsTask_Step4(task);
        break;
    }
}

/* Ends the task: onDone gets kind 0. */
void NetLevel1SettingsTask_Done(NetTask *task)
{
    NetMsg msg;

    task->active = 0;
    task->state = NET_TASK_RUNNING;
    msg.kind = 0;
    task->onDone(msg, &msg);
}

/* Ends the task: onDone gets kind -1. */
void NetLevel1SettingsTask_Failed(NetTask *task)
{
    NetMsg msg;

    task->active = 0;
    task->state = NET_TASK_RUNNING;
    msg.kind = -1;
    task->onDone(msg, &msg);
}

/* task 8, step 0 */
void NetLevel1SettingsTask_Step0(NetTask *task)
{
    task->step++;
    gLobbyNet.nReplies = 0;
    task->idx = 1;
    task->pos = 1;
    task->reply = NET_REPLY_NONE;
}

/* task 8, step 1: the settings the entry allows (its 6312-6314 values are 1);
 * the others count as answered */
void NetLevel1SettingsTask_Step1(NetTask *task)
{
    if (NetReq_CountFree() >= 4) {
        task->step++;
        if (gLobbyNet.settings.name[0] != 0 && gLobbyNet.rec[gLobbyNet.cur[1] - 1].v[0] == 1) {
            LobbyReq_Level1SetName(gLobbyNet.settings.name, strlen(gLobbyNet.settings.name), NetLevel1SettingsTask_Step1Reply1);
        } else {
            task->reply |= 1;
        }
        if (gLobbyNet.settings.pass[0] != 0 && gLobbyNet.rec[gLobbyNet.cur[1] - 1].v[1] == 1) {
            LobbyReq_Level1SetPass((s8 *) gLobbyNet.settings.pass, strlen(gLobbyNet.settings.pass), NetLevel1SettingsTask_Step1Reply2);
        } else {
            task->reply |= 2;
        }
        if (gLobbyNet.settings.text[0] != 0 && gLobbyNet.rec[gLobbyNet.cur[1] - 1].v[2] == 1) {
            LobbyReq_Level1SetText((s8 *) gLobbyNet.settings.text, strlen(gLobbyNet.settings.text), NetLevel1SettingsTask_Step1Reply4);
        } else {
            task->reply |= 4;
        }
        LobbyReq_Level1SetGroupInfo(gLobbyNet.cur[1], gLobbyNet.settings.groupInfo, NetLevel1SettingsTask_Step1Reply8);
    }
}

/* task 8, step 2: all four answered */
void NetLevel1SettingsTask_Step2(NetTask *task)
{
    if (task->reply == 0xF) {
        task->step++;
        task->reply = NET_REPLY_NONE;
    } else if (task->reply & 0x80) {
        task->state = NET_TASK_FAILED;
        task->step = 0;
        task->reply = NET_REPLY_NONE;
    }
}

/* task 8, step 3: 6311 */
void NetLevel1SettingsTask_Step3(NetTask *task)
{
    if (NetReq_CountFree() > 0) {
        task->step++;
        LobbyReq_Level1SettingsEnd(NetLevel1SettingsTask_Step3Reply);
    }
}

/*
 * Step 4: waits for 6311's reply. Any reply, an error too, ends the task
 * as done: the error test comes after the nonzero test, so it never holds.
 */
void NetLevel1SettingsTask_Step4(NetTask *task)
{
    s32 replyBits = task->reply;

    if (replyBits != 0) {
        task->state++;
        task->step = 0;
        task->reply = NET_REPLY_NONE;
    } else if ((u8)replyBits & 0x80) {
        task->state = NET_TASK_FAILED;
        task->step = 0;
        task->reply = NET_REPLY_NONE;
    }
}

/*
 * Step 1's reply callback: sets bit 1 of the task's reply, or 0x80 when
 * the reply reports an error.
 */
void NetLevel1SettingsTask_Step1Reply1(NetMsg msg, NetMsg *msgRef)
{
    NetTask *netTask = &gLobbyNet.task[NET_TASK_LEVEL1_SETTINGS];

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
void NetLevel1SettingsTask_Step1Reply2(NetMsg msg, NetMsg *msgRef)
{
    NetTask *netTask = &gLobbyNet.task[NET_TASK_LEVEL1_SETTINGS];

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
void NetLevel1SettingsTask_Step1Reply4(NetMsg msg, NetMsg *msgRef)
{
    NetTask *netTask = &gLobbyNet.task[NET_TASK_LEVEL1_SETTINGS];

    if (msg.kind == 0) {
        netTask->reply |= 4;
    } else {
        netTask->reply |= 0x80;
    }
}

/*
 * Step 3's reply callback: sets bit 4 of the task's reply, or 0x80 when
 * the reply reports an error.
 */
void NetLevel1SettingsTask_Step3Reply(NetMsg msg, NetMsg *msgRef)
{
    NetTask *netTask = &gLobbyNet.task[NET_TASK_LEVEL1_SETTINGS];

    if (msg.kind == 0) {
        netTask->reply |= 4;
    } else {
        netTask->reply |= 0x80;
    }
}

/*
 * Step 1's reply callback: sets bit 8 of the task's reply, or 0x80 when
 * the reply reports an error.
 */
void NetLevel1SettingsTask_Step1Reply8(NetMsg msg, NetMsg *msgRef)
{
    NetTask *netTask = &gLobbyNet.task[NET_TASK_LEVEL1_SETTINGS];

    if (msg.kind == 0) {
        netTask->reply |= 8;
    } else {
        netTask->reply |= 0x80;
    }
}
