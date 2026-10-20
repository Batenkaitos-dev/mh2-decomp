/*
 * The third level's list (gLobbyNet.list2): NetLevel2ListTask (task 4),
 * which asks 6401 for the entries' count and then, per entry, the
 * properties its flags ask for; and each property's request with its
 * packet, its reply and its getter: 6403 the count, 6404 the state byte,
 * 6402 the name, 640D the text, 640B the limits, 650A the colour, 650C
 * the posted quest. Then two requests about an entry's quest: 6509 (the
 * quest is accepted) and 650B (a quest is posted).
 * Network code: it stays byte for byte, as the whole build does.
 */
#include "common.h"
#include "lobby_net.h"
#include "em.h"

void NetList_ReadValues(NetListEntry *e);
s32 LobbyPkt_ListCount(u16 kind);
s32 LobbyPkt_EntryValues(u16 kind, u16 id);
void LobbyMsg_ReadU16(void *a0);
void NetLevel2ListTask_Step1Reply(NetMsg m, NetMsg *pm);
void NetLevel2ListTask_Step4Reply(NetMsg m, NetMsg *pm);
s32 LobbyReq_Level2Count(void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_Level2Values(u16 id, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_Level2State(u16 id, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_Level2Name(u16 id, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_Level2Text(u16 id, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_Level2Limits(u16 id, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_Level2Color(u16 id, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_Level2PostedQuest(u16 id, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_Level2Locked(u16 id, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_Level2Members(u16 id, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_Level2Kind(u16 arg0, void (*onReply)(NetMsg, NetMsg *));
s32 NetReq_CountFree(void);
void NetReq_ClearAll(void);
void NetLevel2ListTask_Step1(NetTask *t);
void NetLevel2ListTask_Step2(NetTask *t);
void NetLevel2ListTask_Step3(NetTask *t);
void NetLevel2ListTask_Step4(NetTask *t);
void NetLevel2ListTask_Run(s32 task);
void NetLevel2ListTask_Steps(NetTask *t);
void NetLevel2ListTask_Done(NetTask *t);
void NetLevel2ListTask_Failed(NetTask *t);
void NetLevel2ListTask_Step0(NetTask *t);
void NetLevel2ListTask_Step5(NetTask *t);
char *strcpy(char *dst, const char *src);      /* strcpy */
s32 LobbyPkt_EntryName(u16 kind, u16 id);
s32 LobbyPkt_EntryState(u16 kind, u16 id);
s32 LobbyPkt_EntryText(u16 kind, u16 id);
s32 LobbyPkt_Level2Limits(u16 id);
void LobbyMsg_ReadU16Str(void *a, void *b);
void LobbyMsg_ReadU16U8b(void *a, void *b);
void LobbyMsg_ReadU16Str2(void *a, void *b);
u8 *LobbyMsg_ReadU16U32(void *a, void *b);
s32 LobbyPkt_Level2Color(u16 id);
s32 LobbyPkt_Level2PostedQuest(u16 id);
s32 LobbyPkt_QuestAccept(u32 v);
s32 LobbyPkt_QuestPost(void *data, u16 n);
/* prototypes */
void Lobby_RouteMessage(s32);
/* end of prototypes */

s32 NetLevel2ListTask_Start(s32 unused, s32 flags, void (*onDone)(NetMsg, NetMsg *))
{
    NetTask *netTask = &gLobbyNet.task[NET_TASK_LEVEL2_LIST];

    if (netTask->active == 0) {
        memset(netTask, 0, sizeof(NetTask));
        netTask->active = 1;
        netTask->onDone = onDone;
        netTask->func = NetLevel2ListTask_Run;
        netTask->reqFlags = flags & NET_LEVEL2_REQS;
        return 0;
    }
    return -1;
}

/*
 * The task's frame function: runs the current step, or ends the task when
 * it is done or failed.
 */
void NetLevel2ListTask_Run(s32 task)
{
    NetTask *netTask = &gLobbyNet.task[NET_TASK_LEVEL2_LIST];

    if (netTask->active != 0) {
        switch (netTask->state) {
        case NET_TASK_RUNNING:
            NetLevel2ListTask_Steps(netTask);
            break;
        case NET_TASK_DONE:
            NetLevel2ListTask_Done(netTask);
            break;
        case NET_TASK_FAILED:
            NetLevel2ListTask_Failed(netTask);
            break;
        }
    }
}

/* Runs the task's current step. */
void NetLevel2ListTask_Steps(NetTask *task)
{
    switch (task->step) {
    case 0:
        NetLevel2ListTask_Step0(task);
        return;
    case 1:
        NetLevel2ListTask_Step1(task);
        return;
    case 2:
        NetLevel2ListTask_Step2(task);
        return;
    case 3:
        NetLevel2ListTask_Step3(task);
        return;
    case 4:
        NetLevel2ListTask_Step4(task);
        return;
    case 5:
        NetLevel2ListTask_Step5(task);
    default:
        return;
    }
}

/* Ends the task: onDone gets kind 0. */
void NetLevel2ListTask_Done(NetTask *task)
{
    NetMsg msg;

    task->active = 0;
    task->state = NET_TASK_RUNNING;
    task->step = 0;
    msg.kind = 0;
    task->onDone(msg, &msg);
}

/* Ends the task: onDone gets kind -1. */
void NetLevel2ListTask_Failed(NetTask *task)
{
    NetMsg msg;

    task->active = 0;
    task->state = NET_TASK_RUNNING;
    task->step = 0;
    msg.kind = -1;
    task->onDone(msg, &msg);
}

/*
 * Step 0: counts the properties to ask for each entry (the set bits of
 * reqFlags), from entry 1, and clears the replies and the list.
 */
void NetLevel2ListTask_Step0(NetTask *task)
{
    u32 flags;
    u32 i;

    task->step++;
    task->idx = 1;
    task->pos = 1;
    task->reply = NET_REPLY_NONE;
    flags = task->reqFlags;
    task->reqCount = 0;
    for (i = 0; i < 0x20; i++) {
        if (flags & 1) {
            task->reqCount++;
        }
        flags >>= 1;
    }
    gLobbyNet.nReplies = 0;
    gLobbyNet.count2 = 0;
    memset(gLobbyNet.list2, 0, sizeof(gLobbyNet.list2));
}

/* task 4, step 1: 6401 (the count) */
void NetLevel2ListTask_Step1(NetTask *task)
{
    if (NetReq_CountFree() > 0) {
        task->step++;
        LobbyReq_Level2Count(NetLevel2ListTask_Step1Reply);
    }
}

/* task 4, step 2: 6401's reply; tells onDone how many replies to expect (kind 2) */
void NetLevel2ListTask_Step2(NetTask *task)
{
    NetMsg msg;

    if (task->reply == NET_REPLY_OK) {
        task->step++;
        task->reply = NET_REPLY_NONE;
        gLobbyNet.nReplies = gLobbyNet.count2 * task->reqCount;
        msg.kind = 2;
        msg.flag = NET_PROGRESS_LIST_COUNT;
        task->onDone(msg, &msg);
    } else if (task->reply == NET_REPLY_ERROR) {
        task->state = NET_TASK_FAILED;
        task->step = 0;
    }
}

/* task 4, step 3 */
void NetLevel2ListTask_Step3(NetTask *task)
{
    task->step++;
    task->reply = NET_REPLY_NONE;
    if (gLobbyNet.count2 == 0) {
        task->state = NET_TASK_DONE;
        task->step = 0;
    }
}

/* task 4, step 4: the requests of one entry (bits: 1 6403, 2 6404, 4 6402,
 * 0x20 640B, 8 640D, 0x10 6405, 0x40 6409, 0x80 650A, 0x100 650C, 0x400 6418) */
void NetLevel2ListTask_Step4(NetTask *task)
{
    u32 id;

    if (task->reply == NET_REPLY_ERROR) {
        task->state = NET_TASK_FAILED;
        NetReq_ClearAll();
        return;
    }
    id = task->pos;
    if (NetReq_CountFree() >= task->reqCount) {
        if (task->reqFlags & NET_ENTRY_VALUES) {
            LobbyReq_Level2Values(id, NetLevel2ListTask_Step4Reply);
        }
        if (task->reqFlags & NET_ENTRY_STATE) {
            LobbyReq_Level2State(id, NetLevel2ListTask_Step4Reply);
        }
        if (task->reqFlags & NET_ENTRY_NAME) {
            LobbyReq_Level2Name(id, NetLevel2ListTask_Step4Reply);
        }
        if (task->reqFlags & NET_ENTRY_LIMITS) {
            LobbyReq_Level2Limits(id, NetLevel2ListTask_Step4Reply);
        }
        if (task->reqFlags & NET_ENTRY_TEXT) {
            LobbyReq_Level2Text(id, NetLevel2ListTask_Step4Reply);
        }
        if (task->reqFlags & NET_ENTRY_LOCKED) {
            LobbyReq_Level2Locked(id, NetLevel2ListTask_Step4Reply);
        }
        if (task->reqFlags & NET_ENTRY_MEMBERS) {
            LobbyReq_Level2Members(id, NetLevel2ListTask_Step4Reply);
        }
        if (task->reqFlags & NET_ENTRY_COLOR) {
            LobbyReq_Level2Color(id, NetLevel2ListTask_Step4Reply);
        }
        if (task->reqFlags & NET_ENTRY_POSTED_QUEST) {
            LobbyReq_Level2PostedQuest(id, NetLevel2ListTask_Step4Reply);
        }
        if (task->reqFlags & NET_ENTRY_KIND) {
            LobbyReq_Level2Kind(id, NetLevel2ListTask_Step4Reply);
        }
        task->pos++;
        if (id + 1 > gLobbyNet.count2 || id + 1 > 5) {
            task->step++;
        }
    }
}

/* task 4, step 5: done once every entry has all the information asked for */
void NetLevel2ListTask_Step5(NetTask *task)
{
    s32 ok = 1;
    u32 j;
    u32 i = 0;

    for (j = 0; j < 5; j++) {
        i++;
        if (i > gLobbyNet.count2) {
            break;
        }
        if ((task->reqFlags & gLobbyNet.list2[i - 1].flags) != task->reqFlags) {
            ok = 0;
            break;
        }
    }
    if (task->reply == NET_REPLY_ERROR) {
        task->state = NET_TASK_FAILED;
        task->step = 0;
        NetReq_ClearAll();
    } else if (ok) {
        task->state = NET_TASK_DONE;
        task->step = 0;
    }
}

/* task 4's reply callback for 6401 */
void NetLevel2ListTask_Step1Reply(NetMsg msg, NetMsg *msgRef)
{
    NetTask *netTask = &gLobbyNet.task[NET_TASK_LEVEL2_LIST];

    if (msg.kind == 0) {
        netTask->reply = NET_REPLY_OK;
    } else {
        netTask->reply = NET_REPLY_ERROR;
    }
}

/* task 4's reply callback for an entry's requests: each reply is reported to onDone (kind 2) */
void NetLevel2ListTask_Step4Reply(NetMsg msg, NetMsg *msgRef)
{
    NetTask *netTask = &gLobbyNet.task[NET_TASK_LEVEL2_LIST];
    NetMsg n;

    if (msg.kind == 0) {
        n.kind = 2;
        n.flag = NET_PROGRESS_LIST_REPLY;
        netTask->onDone(n, &n);
        netTask->reply = NET_REPLY_OK;
    } else {
        netTask->reply = NET_REPLY_ERROR;
    }
}

/* 6401 (the count) */
s32 LobbyReq_Level2Count(void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_ListCount(2);
        return i;
    }
    return -1;
}

s32 NetList2_GetCount(u16 *out)
{
    *out = gLobbyNet.count2;
    return 0;
}

/* the reply to 6401: u16 count */
void LobbyReply_Level2Count(void)
{
    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU16(&gLobbyNet.count2);
    }
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/* 6403 for entry id */
s32 LobbyReq_Level2Values(u16 id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_EntryValues(2, id);
        return i;
    }
    return -1;
}

/* The users in entry id and how many of them are out on a quest;
 * returns 0. */
s32 NetList2_GetUsers(u16 id, u16 *userCount, u16 *questUserCount)
{
    *userCount = gLobbyNet.list2[id - 1].users;
    *questUserCount = gLobbyNet.list2[id - 1].questUsers;
    return 0;
}

/* the reply to 6403 and its notification; route 0x1E */
void LobbyReply_Level2Values(void)
{
    if (gLobbyNet.replyErr == 0) {
        NetList_ReadValues(gLobbyNet.list2);
    }
    Lobby_RouteMessage(NET_ROUTE_LEVEL2_VALUES);
}

/* 6404 for entry id */
s32 LobbyReq_Level2State(u16 id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_EntryState(2, id);
        return i;
    }
    return -1;
}

s32 NetList2_GetByte(u16 id, u8 *out)
{
    *out = gLobbyNet.list2[id - 1].state;
    return 0;
}

/* the reply to 6404 and its notification: u16 id, u8; route 0x1F */
void LobbyReply_Level2State(void)
{
    u16 id;
    u8 value;

    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU16U8b(&id, &value);
        gLobbyNet.list2[id - 1].id = id;
        gLobbyNet.list2[id - 1].state = value;
        gLobbyNet.lastId = id;
        gLobbyNet.list2[id - 1].flags |= NET_ENTRY_STATE;
    }
    Lobby_RouteMessage(NET_ROUTE_LEVEL2_STATE);
}

/* 6402 for entry id */
s32 LobbyReq_Level2Name(u16 id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_EntryName(2, id);
        return i;
    }
    return -1;
}

s32 NetList2_GetName(u16 id, char *dst)
{
    strcpy(dst, gLobbyNet.list2[id - 1].name);
    return 0;
}

/* the reply to 6402 and its notification: u16 id, string; route 0x1D */
void LobbyReply_Level2Name(void)
{
    char name[0x10C];
    u16 id;

    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU16Str(&id, name);
        gLobbyNet.list2[id - 1].id = id;
        strcpy(gLobbyNet.list2[id - 1].name, name);
        gLobbyNet.lastId = id;
        gLobbyNet.list2[id - 1].flags |= NET_ENTRY_NAME;
    }
    Lobby_RouteMessage(NET_ROUTE_LEVEL2_NAME);
}

/* 640D for entry id */
s32 LobbyReq_Level2Text(u16 id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_EntryText(2, id);
        return i;
    }
    return -1;
}

s32 NetList2_GetText(u16 id, char *dst)
{
    strcpy(dst, gLobbyNet.list2[id - 1].text);
    return 0;
}

/* the reply to 640D and its notification: u16 id, string; route 0x20 */
void LobbyReply_Level2Text(void)
{
    char text[0x10C];
    u16 id;

    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU16Str2(&id, text);
        gLobbyNet.list2[id - 1].id = id;
        strcpy(gLobbyNet.list2[id - 1].text, text);
        gLobbyNet.list2[id - 1].flags |= NET_ENTRY_TEXT;
    }
    Lobby_RouteMessage(NET_ROUTE_LEVEL2_TEXT);
}

/* 640B for entry id */
s32 LobbyReq_Level2Limits(u16 id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_Level2Limits(id);
        return i;
    }
    return -1;
}

/* The values 640B's reply gave entry id: the most users, then four values
 * no code reads yet; returns 0. */
s32 NetList2_GetLimits(u16 id, u16 *maxUserCount, u16 *value1, u16 *value2, u16 *value3, u16 *value4)
{
    *maxUserCount = gLobbyNet.list2[id - 1].maxUsers;
    *value1 = gLobbyNet.list2[id - 1].unk12;
    *value2 = gLobbyNet.list2[id - 1].unk14;
    *value3 = gLobbyNet.list2[id - 1].unk16;
    *value4 = gLobbyNet.list2[id - 1].unk18;
    return 0;
}

/* 640B (entry id): the entry values */
s32 LobbyPkt_Level2Limits(u16 id)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL2_LIMITS);
    LobbyPkt_PutU16(&gLobbyPkt, id);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 640B and its notification: u16 id, u16 x5; route 0x21 */
void LobbyReply_Level2Limits(void)
{
    NetListEntry *entry;
    u16 id;
    u8 *cur;

    if (gLobbyNet.replyErr == 0) {
        cur = LobbyMsg_GetU16(&id, gLobbyRecvData);
        entry = &gLobbyNet.list2[id - 1];
        cur = LobbyMsg_GetU16(&entry->maxUsers, cur);
        cur = LobbyMsg_GetU16(&entry->unk12, cur);
        cur = LobbyMsg_GetU16(&entry->unk16, cur);
        cur = LobbyMsg_GetU16(&entry->unk14, cur);
        LobbyMsg_GetU16(&entry->unk18, cur);
        entry->flags |= NET_ENTRY_LIMITS;
    }
    Lobby_RouteMessage(NET_ROUTE_LEVEL2_LIMITS);
}

/* 650A for entry id */
s32 LobbyReq_Level2Color(u16 id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_Level2Color(id);
        return i;
    }
    return -1;
}

/* the value 650A gave entry id */
s32 Net_GetLevel2Color(u16 id, s32 *out)
{
    *out = gLobbyNet.list2[id - 1].color;
    return 0;
}

/* 650A (entry id) */
s32 LobbyPkt_Level2Color(u16 id)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL2_COLOR);
    LobbyPkt_PutU16(&gLobbyPkt, id);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 650A and its notification: u16 id, u32; route 0x23 */
void LobbyReply_Level2Color(void)
{
    s32 value;
    u16 id;

    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU16U32(&id, &value);
        gLobbyNet.list2[id - 1].id = id;
        gLobbyNet.list2[id - 1].color = value;
        gLobbyNet.lastId = id;
        gLobbyNet.list2[id - 1].flags |= NET_ENTRY_COLOR;
    }
    Lobby_RouteMessage(NET_ROUTE_LEVEL2_COLOR);
}

/* 650C for entry id */
s32 LobbyReq_Level2PostedQuest(u16 id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_Level2PostedQuest(id);
        return i;
    }
    return -1;
}

/* the 0x20 bytes 650C gave entry id */
s32 Net_GetLevel2PostedQuest(u16 id, u8 *dst)
{
    s32 i;

    for (i = 0; i < 0x20; i++) {
        dst[i] = gLobbyNet.list2[id - 1].postedQuest[i];
    }
    return 0;
}

/* 650C (entry id) */
s32 LobbyPkt_Level2PostedQuest(u16 id)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL2_POSTED_QUEST);
    LobbyPkt_PutU16(&gLobbyPkt, id);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 650C and its notification: u16 id, blob (0x20 bytes); route 0x31 */
void LobbyReply_Level2PostedQuest(void)
{
    u16 id;
    u8 *cur;

    if (gLobbyNet.replyErr == 0) {
        cur = LobbyMsg_GetU16(&id, gLobbyRecvData);
        LobbyMsg_GetBlob(gLobbyNet.list2[id - 1].postedQuest, 0x20, cur);
        gLobbyNet.list2[id - 1].id = id;
        gLobbyNet.lastId = id;
        gLobbyNet.list2[id - 1].flags |= NET_ENTRY_POSTED_QUEST;
    }
    Lobby_RouteMessage(NET_ROUTE_LEVEL2_POSTED_QUEST);
}

/* 6509 (u32) */
s32 LobbyReq_QuestAccept(u32 value, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_QuestAccept(value);
        return i;
    }
    return -1;
}

/* Sends 6509; returns its sequence number. */
s32 LobbyPkt_QuestAccept(u32 value)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_QUEST_ACCEPT);
    LobbyPkt_PutU32(&gLobbyPkt, value);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 6509 */
void LobbyReply_QuestAccept(void)
{
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/* 650B (a blob) */
s32 LobbyReq_QuestPost(void *data, u16 n, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_QuestPost(data, n);
        return i;
    }
    return -1;
}

/* Sends 650B; returns its sequence number. */
s32 LobbyPkt_QuestPost(void *data, u16 n)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_QUEST_POST);
    LobbyPkt_PutBlob(&gLobbyPkt, data, n);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* The reply to 650B: completes its request. */
void LobbyReply_QuestPost(void)
{
    Lobby_RouteMessage(NET_ROUTE_NONE);
}
