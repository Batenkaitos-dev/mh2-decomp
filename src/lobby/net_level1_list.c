/*
 * Level 1's list (gLobbyNet.list1).
 * - The three levels' selected and current entries (NetList_Select,
 *   NetList_Enter, NetList_Leave).
 * - NetLevel1ListTask, which fetches the list: 6301's count, then, entry
 *   by entry, the properties the caller's flags ask for.
 * - The count's request (6301) and, for each property of an entry, the
 *   request that asks the server for it, the reply that stores it in the
 *   entry, and the getter the menus read it with. In order: the name
 *   (6302), the values: users, those out on a quest and the maximum
 *   (6303), the state byte (6304), the text (6308), the colour (630D),
 *   an unknown byte (630B) and the four group-info bytes (631B). Each
 *   reply handler also takes the server's notification of the same
 *   command.
 * Network code: it stays byte for byte, as the whole build does.
 */
#include "common.h"
#include "lobby_net.h"
#include "m2c_macros.h"
#include "libc.h"

/* the selection byte of level 0-2 */
void NetList_Select(s32 levelArg, u16 id)
{
    s32 level;

    level = levelArg & 0xFF;
    switch (level) {
    case 0:
        gLobbyNet.sel[0] = id;
        break;
    case 1:
        gLobbyNet.sel[1] = id;
        break;
    case 2:
        gLobbyNet.sel[2] = id;
        break;
    }
}

/* Clears the entry the player is in at a level (0-2): the player left it. */
void NetList_Leave(u8 level)
{
    switch (level) {
    case 0:
        gLobbyNet.cur[0] = 0;
        break;
    case 1:
        gLobbyNet.cur[1] = 0;
        break;
    case 2:
        gLobbyNet.cur[2] = 0;
        break;
    }
}

/* Makes the selected entry of a level (0-2) the one the player is in. */
void NetList_Enter(u8 level)
{
    switch (level) {
    case 0:
        gLobbyNet.cur[0] = gLobbyNet.sel[0];
        break;
    case 1:
        gLobbyNet.cur[1] = gLobbyNet.sel[1];
        break;
    case 2:
        gLobbyNet.cur[2] = gLobbyNet.sel[2];
        break;
    }
}

s32 LobbyNet_GetListReplies(u16 *nReplies)
{
    *nReplies = gLobbyNet.nReplies;
    return 0;
}

/* starts task 3: the second level's list, with the information flags asks for
 * (NET_ENTRY_*: VALUES 6303, STATE 6304, NAME 6302, TEXT 6308, LOCKED 630B,
 * COLOR 630D, GROUP_INFO 631B, KIND 630E; NET_LEVEL1_REQS leaves out LOCKED) */
s32 NetLevel1ListTask_Start(s32 unused, s32 flags, void (*onDone)(NetMsg, NetMsg *))
{
    NetTask *netTask = &gLobbyNet.task[NET_TASK_LEVEL1_LIST];

    if (netTask->active == 0) {
        memset(netTask, 0, sizeof(NetTask));
        netTask->active = 1;
        netTask->onDone = onDone;
        netTask->func = NetLevel1ListTask_Run;
        netTask->reqFlags = flags & NET_LEVEL1_REQS;
        return 0;
    }
    return -1;
}

/*
 * The task's frame function: runs the current step, or ends the task when
 * it is done or failed.
 */
void NetLevel1ListTask_Run(s32 task)
{
    NetTask *netTask = &gLobbyNet.task[NET_TASK_LEVEL1_LIST];

    if (netTask->active != 0) {
        switch (netTask->state) {
        case NET_TASK_RUNNING:
            NetLevel1ListTask_Steps(netTask);
            break;
        case NET_TASK_DONE:
            NetLevel1ListTask_Done(netTask);
            break;
        case NET_TASK_FAILED:
            NetLevel1ListTask_Failed(netTask);
            break;
        }
    }
}

/* Runs the task's current step. */
void NetLevel1ListTask_Steps(NetTask *task)
{
    switch (task->step) {
    case 0:
        NetLevel1ListTask_Step0(task);
        return;
    case 1:
        NetLevel1ListTask_Step1(task);
        return;
    case 2:
        NetLevel1ListTask_Step2(task);
        return;
    case 3:
        NetLevel1ListTask_Step3(task);
        return;
    case 4:
        NetLevel1ListTask_Step4(task);
        return;
    case 5:
        NetLevel1ListTask_Step5(task);
    default:
        return;
    }
}

/* Ends the task: onDone gets kind 0. */
void NetLevel1ListTask_Done(NetTask *task)
{
    NetMsg msg;

    task->active = 0;
    task->state = NET_TASK_RUNNING;
    msg.kind = 0;
    task->onDone(msg, &msg);
}

/* Ends the task: onDone gets kind -1. */
void NetLevel1ListTask_Failed(NetTask *task)
{
    NetMsg msg;

    task->active = 0;
    task->state = NET_TASK_RUNNING;
    msg.kind = -1;
    task->onDone(msg, &msg);
}

/* task 3, step 0: counts the requests per entry, clears the list */
void NetLevel1ListTask_Step0(NetTask *task)
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
    gLobbyNet.count1 = 0;
    memset(gLobbyNet.list1, 0, 17820);
}

/* task 3, step 1: 6301 (the count) */
void NetLevel1ListTask_Step1(NetTask *task)
{
    if (NetReq_CountFree() > 0) {
        task->step++;
        LobbyReq_Level1Count(NetLevel1ListTask_Step1Reply);
    }
}

/* task 3, step 2: 6301's reply; tells onDone how many replies to expect (kind 2) */
void NetLevel1ListTask_Step2(NetTask *task)
{
    NetMsg msg;

    if (task->reply == NET_REPLY_OK) {
        task->step++;
        task->reply = NET_REPLY_NONE;
        gLobbyNet.nReplies = gLobbyNet.count1 * task->reqCount;
        msg.kind = 2;
        msg.flag = NET_PROGRESS_LIST_COUNT;
        task->onDone(msg, &msg);
    } else if (task->reply == NET_REPLY_ERROR) {
        task->state = NET_TASK_FAILED;
        task->step = 0;
    }
}

/* task 3, step 3 */
void NetLevel1ListTask_Step3(NetTask *task)
{
    task->step++;
    task->reply = NET_REPLY_NONE;
    if (gLobbyNet.count1 == 0) {
        task->state = NET_TASK_DONE;
        task->step = 0;
    }
}

/* task 3, step 4: the requests of one entry, when enough request slots are free */
void NetLevel1ListTask_Step4(NetTask *task)
{
    u32 id;

    if (task->reply & 0x80) {
        task->state = NET_TASK_FAILED;
        NetReq_ClearAll();
        return;
    }
    id = task->pos;
    if (NetReq_CountFree() >= task->reqCount) {
        if (task->reqFlags & NET_ENTRY_VALUES) {
            LobbyReq_Level1Values(id, NetLevel1ListTask_Step4Reply);
        }
        if (task->reqFlags & NET_ENTRY_STATE) {
            LobbyReq_Level1State(id, NetLevel1ListTask_Step4Reply);
        }
        if (task->reqFlags & NET_ENTRY_NAME) {
            LobbyReq_Level1Name(id, NetLevel1ListTask_Step4Reply);
        }
        if (task->reqFlags & NET_ENTRY_TEXT) {
            LobbyReq_Level1Text(id, NetLevel1ListTask_Step4Reply);
        }
        if (task->reqFlags & NET_ENTRY_LOCKED) {
            LobbyReq_630B(id, NetLevel1ListTask_Step4Reply);
        }
        if (task->reqFlags & NET_ENTRY_COLOR) {
            LobbyReq_Level1Color(id, NetLevel1ListTask_Step4Reply);
        }
        if (task->reqFlags & NET_ENTRY_GROUP_INFO) {
            LobbyReq_Level1GroupInfo(id, NetLevel1ListTask_Step4Reply);
        }
        if (task->reqFlags & NET_ENTRY_KIND) {
            LobbyReq_Level1Kind(id, NetLevel1ListTask_Step4Reply);
        }
        task->pos++;
        if (id + 1 > gLobbyNet.count1 || id + 1 > 45) {
            task->step++;
        }
    }
}

/* task 3, step 5: done once every entry has all the information asked for */
void NetLevel1ListTask_Step5(NetTask *task)
{
    s32 ok = 1;
    u32 j;
    u32 i = 0;

    for (j = 0; j < 45; j++) {
        i++;
        if (i > gLobbyNet.count1) {
            break;
        }
        if ((task->reqFlags & gLobbyNet.list1[i - 1].flags) != task->reqFlags) {
            ok = 0;
            break;
        }
    }
    if (task->reply & 0x80) {
        task->state = NET_TASK_FAILED;
        task->step = 0;
        NetReq_ClearAll();
    } else if (ok) {
        task->state = NET_TASK_DONE;
        task->step = 0;
    }
}

/* task 3's reply callback for 6301 */
void NetLevel1ListTask_Step1Reply(NetMsg msg, NetMsg *msgRef)
{
    NetTask *netTask = &gLobbyNet.task[NET_TASK_LEVEL1_LIST];

    if (msg.kind == 0) {
        netTask->reply |= 1;
    } else {
        netTask->reply |= 0x80;
    }
}

/* task 3's reply callback for an entry's requests: each reply is reported to onDone (kind 2) */
void NetLevel1ListTask_Step4Reply(NetMsg msg, NetMsg *msgRef)
{
    NetTask *netTask = &gLobbyNet.task[NET_TASK_LEVEL1_LIST];
    NetMsg n;

    if (msg.kind == 0) {
        netTask->reply |= 1;
        n.kind = 2;
        n.flag = NET_PROGRESS_LIST_REPLY;
        netTask->onDone(n, &n);
    } else {
        netTask->reply |= 0x80;
    }
}

/*
 * Sends 6301 as a request; onReply gets its reply. Returns the request's
 * slot, -1 if none is free.
 */
s32 LobbyReq_Level1Count(void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_ListCount(1);
        return i;
    }
    return -1;
}

s32 NetList1_GetCount(u16 *count1)
{
    *count1 = gLobbyNet.count1;
    return 0;
}

/* the reply to 6301: u16 count */
void LobbyReply_Level1Count(void)
{
    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU16(&gLobbyNet.count1);
    }
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/* 6302 for entry id */
s32 LobbyReq_Level1Name(u16 id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_EntryName(1, id);
        return i;
    }
    return -1;
}

s32 NetList1_GetName(u16 id, char *dst)
{
    strcpy(dst, gLobbyNet.list1[id - 1].name);
    return 0;
}

/* the reply to 6302 and its notification: u16 id, string; route NET_ROUTE_LEVEL1_NAME */
void LobbyReply_Level1Name(void)
{
    char name[76];
    u16 id;

    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU16Str(&id, name);
        gLobbyNet.list1[id - 1].id = id;
        strcpy(gLobbyNet.list1[id - 1].name, name);
        gLobbyNet.lastId = id;
        gLobbyNet.list1[id - 1].flags |= NET_ENTRY_NAME;
    }
    Lobby_RouteMessage(NET_ROUTE_LEVEL1_NAME);
}

/* 6303 for entry id */
s32 LobbyReq_Level1Values(u16 id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_EntryValues(1, id);
        return i;
    }
    return -1;
}

/* The users of level-1 entry id, and how many of them are out on a quest; returns 0. */
s32 NetList1_GetUsers(u16 id, u16 *users, u16 *questUsers)
{
    *users = gLobbyNet.list1[id - 1].users;
    *questUsers = gLobbyNet.list1[id - 1].questUsers;
    return 0;
}

s32 NetList1_GetMaxUsers(u16 id, u16 *out)
{
    *out = gLobbyNet.list1[id - 1].maxUsers;
    return 0;
}

/* The reply to 6303: the entry's values into gLobbyNet.list1. */
void LobbyReply_Level1Values(void)
{
    if (gLobbyNet.replyErr == 0) {
        NetList_ReadValues(gLobbyNet.list1);
    }
    Lobby_RouteMessage(NET_ROUTE_LEVEL1_VALUES);
}

/* 6304 for entry id */
s32 LobbyReq_Level1State(u16 id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_EntryState(1, id);
        return i;
    }
    return -1;
}

s32 NetList1_GetByte(u16 id, u8 *out)
{
    *out = gLobbyNet.list1[id - 1].state;
    return 0;
}

/* the reply to 6304 and its notification: u16 id, u8; route NET_ROUTE_LEVEL1_STATE */
void LobbyReply_Level1State(void)
{
    u16 id;
    u8 value;

    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU16U8b(&id, &value);
        gLobbyNet.list1[id - 1].id = id;
        gLobbyNet.list1[id - 1].state = value;
        gLobbyNet.lastId = id;
        gLobbyNet.list1[id - 1].flags |= NET_ENTRY_STATE;
    }
    Lobby_RouteMessage(NET_ROUTE_LEVEL1_STATE);
}

/* 6308 for entry id */
s32 LobbyReq_Level1Text(u16 id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_EntryText(1, id);
        return i;
    }
    return -1;
}

s32 NetList1_GetText(u16 id, char *dst)
{
    strcpy(dst, gLobbyNet.list1[id - 1].text);
    return 0;
}

/* the reply to 6308 and its notification: u16 id, string; route NET_ROUTE_LEVEL1_TEXT */
void LobbyReply_Level1Text(void)
{
    char text[268];
    u16 id;

    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU16Str2(&id, text);
        gLobbyNet.list1[id - 1].id = id;
        strcpy(gLobbyNet.list1[id - 1].text, text);
        gLobbyNet.list1[id - 1].flags |= NET_ENTRY_TEXT;
    }
    Lobby_RouteMessage(NET_ROUTE_LEVEL1_TEXT);
}

/* 630D for entry id */
s32 LobbyReq_Level1Color(u16 id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_Level1Color(id);
        return i;
    }
    return -1;
}

s32 NetList1_GetColor(u16 id, s32 *out)
{
    *out = gLobbyNet.list1[id - 1].color;
    return 0;
}

/* 630D: u16 id */
s32 LobbyPkt_Level1Color(u16 id)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL1_COLOR) & 0xFFFF;
    LobbyPkt_PutU16(&gLobbyPkt, id);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 630D and its notification: u16 id, u32; route NET_ROUTE_LEVEL1_COLOR */
void LobbyReply_Level1Color(void)
{
    s32 value;
    u16 id;

    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU16U32(&id, &value);
        gLobbyNet.list1[id - 1].id = id;
        gLobbyNet.list1[id - 1].color = value;
        gLobbyNet.lastId = id;
        gLobbyNet.list1[id - 1].flags |= NET_ENTRY_COLOR;
    }
    Lobby_RouteMessage(NET_ROUTE_LEVEL1_COLOR);
}

/* 630B for entry id */
s32 LobbyReq_630B(u16 id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_630B(id);
        return i;
    }
    return -1;
}

/* 630B: u16 id */
s32 LobbyPkt_630B(u16 id)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_630B) & 0xFFFF;
    LobbyPkt_PutU16(&gLobbyPkt, id);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 630B and its notification: u16 id, u8; route NET_ROUTE_630B */
void LobbyReply_630B(void)
{
    u16 id;
    u8 value;

    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU16U8(&id, &value);
        gLobbyNet.list1[id - 1].locked = value;
        gLobbyNet.list1[id - 1].flags |= NET_ENTRY_LOCKED;
        gLobbyNet.lastId = id;
    }
    Lobby_RouteMessage(NET_ROUTE_630B);
}

/* 631B for entry id */
s32 LobbyReq_Level1GroupInfo(u16 id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_Level1GroupInfo(id);
        return i;
    }
    return -1;
}

/* the address of gLobbyNet.list1[-1].groupInfo: the game adds id * the
 * entry's size to it; reached from gLobbyNet, the offset would be added
 * apart and the code would differ */
extern u8 D_008530C8[];

/* Copies the four group-info bytes of level-1 entry id; returns 0. */
s32 NetList1_GetGroupInfo(u16 id, u8 *out)
{
    u8 *cur = D_008530C8 + id * sizeof(NetListEntry);

    out[0] = cur[0];
    out[1] = cur[1];
    out[2] = cur[2];
    out[3] = cur[3];
    return 0;
}

/* 631B: u16 id */
s32 LobbyPkt_Level1GroupInfo(u16 id)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL1_GROUP_INFO) & 0xFFFF;
    LobbyPkt_PutU16(&gLobbyPkt, id);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 631B and its notification: u16 id, u8 x4; route NET_ROUTE_LEVEL1_GROUP_INFO */
void LobbyReply_Level1GroupInfo(void)
{
    u16 id;
    u8 *cur;

    if (gLobbyNet.replyErr == 0) {
        cur = LobbyMsg_GetU16(&id, gLobbyRecvData);
        cur = LobbyMsg_GetU8(&gLobbyNet.list1[id - 1].groupInfo[0], cur);
        cur = LobbyMsg_GetU8(&gLobbyNet.list1[id - 1].groupInfo[1], cur);
        cur = LobbyMsg_GetU8(&gLobbyNet.list1[id - 1].groupInfo[2], cur);
        LobbyMsg_GetU8(&gLobbyNet.list1[id - 1].groupInfo[3], cur);
        gLobbyNet.list1[id - 1].id = id;
        gLobbyNet.lastId = id;
        gLobbyNet.list1[id - 1].flags |= NET_ENTRY_GROUP_INFO;
    }
    Lobby_RouteMessage(NET_ROUTE_LEVEL1_GROUP_INFO);
}
