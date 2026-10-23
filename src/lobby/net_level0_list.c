/*
 * Level 0's list (gLobbyNet.list0) and its task (NET_TASK_LEVEL0_LIST):
 * 6203's count, then for each entry the requests the caller's flags ask
 * for; the entry requests' senders, replies and getters; entering an
 * entry (6207), leaving it (6306) and the server's kick (6307). Then the
 * readers that take a reply's fields from gLobbyRecvData (a u16, then a
 * string or a byte), which the three levels' lists share, and the last
 * of level 0's entry requests, the entry's kind (620B): its request,
 * getter and reply.
 * Network code: it stays byte for byte, as the whole build does.
 */
#include "common.h"
#include "lobby_net.h"
#include "libc.h"

s32 LobbyReq_Level0Name(u16 id, void (*onReply)(NetMsg, NetMsg *));

/* starts the level-0 list task (NET_TASK_LEVEL0_LIST), with the information flags asks for
 * (NET_ENTRY_*: VALUES 6205, STATE 6206, NAME 6204, TEXT 620A, KIND 620B) */
s32 NetLevel0ListTask_Start(s32 unused, s32 flags, void (*onDone)(NetMsg, NetMsg *))
{
    NetTask *netTask = &gLobbyNet.task[NET_TASK_LEVEL0_LIST];

    if (netTask->active == 0) {
        memset(netTask, 0, sizeof(NetTask));
        netTask->active = 1;
        netTask->onDone = onDone;
        netTask->func = NetLevel0ListTask_Run;
        netTask->reqFlags = flags & NET_LEVEL0_REQS;
        return 0;
    }
    return -1;
}

/*
 * The task's frame function: runs the current step, or ends the task when
 * it is done or failed.
 */
void NetLevel0ListTask_Run(s32 task)
{
    NetTask *netTask = &gLobbyNet.task[NET_TASK_LEVEL0_LIST];

    if (netTask->active != 0) {
        switch (netTask->state) {
        case NET_TASK_RUNNING:
            NetLevel0ListTask_Steps(netTask);
            break;
        case NET_TASK_DONE:
            NetLevel0ListTask_Done(netTask);
            break;
        case NET_TASK_FAILED:
            NetLevel0ListTask_Failed(netTask);
            break;
        }
    }
}

/* Runs the task's current step. */
void NetLevel0ListTask_Steps(NetTask *task)
{
    switch (task->step) {
    case 0:
        NetLevel0ListTask_Step0(task);
        return;
    case 1:
        NetLevel0ListTask_Step1(task);
        return;
    case 2:
        NetLevel0ListTask_Step2(task);
        return;
    case 3:
        NetLevel0ListTask_Step3(task);
        return;
    case 4:
        NetLevel0ListTask_Step4(task);
        return;
    case 5:
        NetLevel0ListTask_Step5(task);
    default:
        return;
    }
}

/* Ends the task: onDone gets kind 0. */
void NetLevel0ListTask_Done(NetTask *task)
{
    NetMsg msg;

    task->active = 0;
    task->state = NET_TASK_RUNNING;
    msg.kind = 0;
    task->onDone(msg, &msg);
}

/* Ends the task: onDone gets kind -1. */
void NetLevel0ListTask_Failed(NetTask *task)
{
    NetMsg msg;

    task->active = 0;
    task->state = NET_TASK_RUNNING;
    msg.kind = -1;
    task->onDone(msg, &msg);
}

/* the level-0 list task, step 0: counts the requests per entry, clears the list */
void NetLevel0ListTask_Step0(NetTask *task)
{
    u32 flags;
    u32 i;

    task->step++;
    task->idx = 1;
    task->pos = 1;
    task->reply = NET_REPLY_NONE;
    flags = task->reqFlags;
    task->reqCount = 0;
    for (i = 0; i < 32; i++) {
        if (flags & 1) {
            task->reqCount++;
        }
        flags >>= 1;
    }
    gLobbyNet.nReplies = 0;
    gLobbyNet.count0 = 0;
    memset(gLobbyNet.list0, 0, 9900);
}

/* the level-0 list task, step 1: 6203 (the count) */
void NetLevel0ListTask_Step1(NetTask *task)
{
    if (NetReq_CountFree() > 0) {
        task->step++;
        LobbyReq_Level0Count(NetLevel0ListTask_Step1Reply);
    }
}

/* the level-0 list task, step 2: 6203's reply; tells onDone how many replies to expect (kind 2) */
void NetLevel0ListTask_Step2(NetTask *task)
{
    NetMsg msg;

    if (task->reply == NET_REPLY_OK) {
        task->step++;
        task->reply = NET_REPLY_NONE;
        gLobbyNet.nReplies = gLobbyNet.count0 * task->reqCount;
        msg.kind = 2;
        msg.flag = NET_PROGRESS_LIST_COUNT;
        task->onDone(msg, &msg);
    } else if (task->reply == NET_REPLY_ERROR) {
        task->state = NET_TASK_FAILED;
        task->step = 0;
    }
}

/* the level-0 list task, step 3 */
void NetLevel0ListTask_Step3(NetTask *task)
{
    task->step++;
    task->reply = NET_REPLY_NONE;
    if (gLobbyNet.count0 == 0) {
        task->state = NET_TASK_DONE;
        task->step = 0;
    }
}

/* the level-0 list task, step 4: the requests of one entry, when enough request slots are free */
void NetLevel0ListTask_Step4(NetTask *task)
{
    u32 id;

    if (task->reply == NET_REPLY_ERROR) {
        task->state = NET_TASK_FAILED;
        task->step = 0;
        NetReq_ClearAll();
        return;
    }
    id = task->pos;
    if (NetReq_CountFree() >= task->reqCount) {
        if (task->reqFlags & NET_ENTRY_VALUES) {
            LobbyReq_Level0Values(id, NetLevel0ListTask_Step4Reply);
        }
        if (task->reqFlags & NET_ENTRY_STATE) {
            LobbyReq_Level0State(id, NetLevel0ListTask_Step4Reply);
        }
        if (task->reqFlags & NET_ENTRY_NAME) {
            LobbyReq_Level0Name(id, NetLevel0ListTask_Step4Reply);
        }
        if (task->reqFlags & NET_ENTRY_TEXT) {
            LobbyReq_Level0Text(id, NetLevel0ListTask_Step4Reply);
        }
        if (task->reqFlags & NET_ENTRY_KIND) {
            LobbyReq_Level0Kind(id, NetLevel0ListTask_Step4Reply);
        }
        task->pos++;
        if (id + 1 > gLobbyNet.count0 || id + 1 > 25) {
            task->step++;
        }
    }
}

/* the level-0 list task, step 5: done once every entry has all the information asked for */
void NetLevel0ListTask_Step5(NetTask *task)
{
    s32 ok = 1;
    u32 j;
    u32 i = 0;

    for (j = 0; j < 25; j++) {
        i++;
        if (i > gLobbyNet.count0) {
            break;
        }
        if ((task->reqFlags & gLobbyNet.list0[i - 1].flags) != task->reqFlags) {
            ok = 0;
            break;
        }
    }
    if (task->reply == NET_REPLY_ERROR) {
        task->state = NET_TASK_FAILED;
        NetReq_ClearAll();
    } else if (ok) {
        task->state = NET_TASK_DONE;
        task->step = 0;
    }
}

/* the level-0 list task's reply callback for 6203 */
void NetLevel0ListTask_Step1Reply(NetMsg msg, NetMsg *msgRef)
{
    NetTask *netTask = &gLobbyNet.task[NET_TASK_LEVEL0_LIST];

    if (msg.kind == 0) {
        netTask->reply = NET_REPLY_OK;
    } else {
        netTask->reply = NET_REPLY_ERROR;
    }
}

/* the level-0 list task's reply callback for an entry's requests: each reply is reported to onDone (kind 2) */
void NetLevel0ListTask_Step4Reply(NetMsg msg, NetMsg *msgRef)
{
    NetTask *netTask = &gLobbyNet.task[NET_TASK_LEVEL0_LIST];
    NetMsg n;

    if (msg.kind == 0) {
        netTask->reply = NET_REPLY_OK;
        n.kind = 2;
        n.flag = NET_PROGRESS_LIST_REPLY;
        netTask->onDone(n, &n);
    } else {
        netTask->reply = NET_REPLY_ERROR;
    }
}

/* 6203 (the count) */
s32 LobbyReq_Level0Count(void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_ListCount(0);
        return i;
    }
    return -1;
}

s32 NetList0_GetCount(u16 *out)
{
    *out = gLobbyNet.count0;
    return 0;
}

/* the reply to 6203: u16 count */
void LobbyReply_Level0Count(void)
{
    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_RecvU16(&gLobbyNet.count0);
    }
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/* 6204 for entry id */
s32 LobbyReq_Level0Name(u16 id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_EntryName(0, id);
        return i;
    }
    return -1;
}

s32 NetList0_GetName(u16 id, char *dst)
{
    strcpy(dst, gLobbyNet.list0[id - 1].name);
    return 0;
}

/* the reply to 6204: u16 id, string; route NET_ROUTE_LEVEL0_NAME */
void LobbyReply_Level0Name(void)
{
    char name[76];
    u16 id;

    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU16Str(&id, name);
        gLobbyNet.list0[id - 1].id = id;
        strcpy(gLobbyNet.list0[id - 1].name, name);
        gLobbyNet.lastId = id;
        gLobbyNet.list0[id - 1].flags |= NET_ENTRY_NAME;
    }
    Lobby_RouteMessage(NET_ROUTE_LEVEL0_NAME);
}

/* 6205 for entry id */
s32 LobbyReq_Level0Values(u16 id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_EntryValues(0, id);
        return i;
    }
    return -1;
}

/* The users of level-0 entry id, and how many of them are out on a quest;
 * returns 0. */
s32 NetList0_GetUsers(u16 id, u16 *users, u16 *questUsers)
{
    *users = gLobbyNet.list0[id - 1].users;
    *questUsers = gLobbyNet.list0[id - 1].questUsers;
    return 0;
}

s32 NetList0_GetMaxUsers(u16 id, u16 *out)
{
    *out = gLobbyNet.list0[id - 1].maxUsers;
    return 0;
}

s32 NetList0_GetValue10(u16 id, u16 *out)
{
    *out = gLobbyNet.list0[id - 1].unk10;
    return 0;
}

/* the reply to 6205 and its notification; route NET_ROUTE_LEVEL0_VALUES */
void LobbyReply_Level0Values(void)
{
    if (gLobbyNet.replyErr == 0) {
        NetList_ReadValues(gLobbyNet.list0);
    }
    Lobby_RouteMessage(NET_ROUTE_LEVEL0_VALUES);
}

/* 6206 for entry id */
s32 LobbyReq_Level0State(u16 id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_EntryState(0, id);
        return i;
    }
    return -1;
}

s32 NetList0_GetByte(u16 id, u8 *out)
{
    *out = gLobbyNet.list0[id - 1].state;
    return 0;
}

/* the reply to 6206 and its notification: u16 id, u8; route NET_ROUTE_LEVEL0_STATE */
void LobbyReply_Level0State(void)
{
    u16 id;
    u8 value;

    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU16U8(&id, &value);
        gLobbyNet.list0[id - 1].id = id;
        gLobbyNet.list0[id - 1].state = value;
        gLobbyNet.lastId = id;
        gLobbyNet.list0[id - 1].flags |= NET_ENTRY_STATE;
    }
    Lobby_RouteMessage(NET_ROUTE_LEVEL0_STATE);
}

/* 620A for entry id */
s32 LobbyReq_Level0Text(u16 id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_EntryText(0, id);
        return i;
    }
    return -1;
}

s32 NetList0_GetText(u16 id, char *dst)
{
    strcpy(dst, gLobbyNet.list0[id - 1].text);
    return 0;
}

/* the reply to 620A and its notification: u16 id, string; route NET_ROUTE_LEVEL0_TEXT */
void LobbyReply_Level0Text(void)
{
    char text[268];
    u16 id;

    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU16Str2(&id, text);
        gLobbyNet.list0[id - 1].id = id;
        strcpy(gLobbyNet.list0[id - 1].text, text);
        gLobbyNet.list0[id - 1].flags |= NET_ENTRY_TEXT;
    }
    Lobby_RouteMessage(NET_ROUTE_LEVEL0_TEXT);
}

/* the reply to 6207: the first-level entry was entered */
void LobbyReply_Level0Enter(void)
{
    if (gLobbyNet.replyErr == 0) {
        NetList_Enter(0);
    }
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/* 6306: leaves the first-level entry */
s32 LobbyReq_Level0Leave(void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_Leave(0);
        return i;
    }
    return -1;
}

/* the reply to 6306 */
void LobbyReply_Level0Leave(void)
{
    if (gLobbyNet.replyErr == 0) {
        NetList_Leave(0);
    }
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/* the notification 6307: an error text; route 8 */
void LobbyNotify_Level0Kick(void)
{
    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadErrText();
        Lobby_RouteMessage(NET_ROUTE_LEVEL0_KICK);
    }
}

/* Reads a reply's u16 into dst. */
void LobbyMsg_ReadU16(void *dst)
{
    LobbyMsg_GetU16(dst, gLobbyRecvData);
}

/* Reads a reply's u16 and the string after it. */
void LobbyMsg_ReadU16Str(void *num, void *str)
{
    LobbyMsg_GetString(str, LobbyMsg_GetU16(num, gLobbyRecvData));
}

/* Reads a reply's u16 and the byte after it. */
void LobbyMsg_ReadU16U8b(void *num, void *byte)
{
    LobbyMsg_GetU8(byte, LobbyMsg_GetU16(num, gLobbyRecvData));
}

/* The same as LobbyMsg_ReadU16Str. */
void LobbyMsg_ReadU16Str2(void *num, void *str)
{
    LobbyMsg_GetString(str, LobbyMsg_GetU16(num, gLobbyRecvData));
}

/* The same as LobbyMsg_ReadU16U8b. */
void LobbyMsg_ReadU16U8b2(void *num, void *byte)
{
    LobbyMsg_GetU8(byte, LobbyMsg_GetU16(num, gLobbyRecvData));
}

/*
 * Sends 620B as a request; onReply gets its reply. Returns the request's
 * slot, -1 if none is free.
 */
s32 LobbyReq_Level0Kind(u16 id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 req;

    req = NetReq_Alloc(1, 0, onReply);
    if (req != -1) {
        gLobbyNet.req[req].seq = LobbyPkt_EntryKind(0, id);
        return req;
    }
    return -1;
}

/* The kind of level-0 entry id (620B's reply); returns 0. */
s32 NetList0_GetKind(u16 id, u8 *out)
{
    *out = gLobbyNet.list0[(id & 0xFFFF) - 1].kind;
    return 0;
}

/* the reply to 620B and its notification: u16 id, u8; route NET_ROUTE_LEVEL0_KIND */
void LobbyReply_Level0Kind(void)
{
    u16 id;
    u8 value;

    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU16U8b2(&id, &value);
        gLobbyNet.list0[id - 1].id = id;
        gLobbyNet.list0[id - 1].kind = value;
        gLobbyNet.list0[id - 1].flags |= NET_ENTRY_KIND;
    }
    Lobby_RouteMessage(NET_ROUTE_LEVEL0_KIND);
}
