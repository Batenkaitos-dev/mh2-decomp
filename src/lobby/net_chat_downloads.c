/*
 * The lobby protocol's commands for chat and downloads.
 * Network code: it stays byte for byte, as the whole build does.
 * - Chat (67xx): a say (6714), chat messages (6702, 670C), town messages
 *   (6708, 670E) and the two sends to one user (670B, 670D).
 * - Downloads (68xx): NetDlListTask (the files the server offers: 6881,
 *   then 6883 for each) and NetDlFileTask (one file, fetched 0x200 bytes at
 *   a time with 6882).
 * - The town data's slots (6884, refetched on 6885's notice), the save
 *   summary (6148, 6149), 68A0/68A1, the server's name (68C0) and the
 *   server's 6138/6139.
 */
#include "common.h"
#include "lobby_net.h"


extern void *gLobby68C0Buf;
extern u16 gLobbyServerNameSize;
extern u8 gLobby6138Data[];
extern u8 gLobby6149Byte;

void LobbyPkt_ChatSay(s32 arg0, u16 arg1, u8 arg2, u8 arg3);
void LobbyPkt_TownMsg(s32 arg0, u16 arg1);
s32 LobbyPkt_ChatTell(u8 arg0, void *arg1, s32 arg2, u16 arg3);
s32 LobbyPkt_TownMsgTo(void *arg0, s32 arg1, u16 arg2);
void NetDlListTask_Run(s32 task);
void NetDlListTask_Steps(NetTask *t);
void NetDlListTask_Done(NetTask *t);
void NetDlListTask_Failed(NetTask *t);
void NetDlListTask_Step0(NetTask *t);
void NetDlListTask_Step1(NetTask *t);
void NetDlListTask_Step2(NetTask *t);
void NetDlListTask_Step3(NetTask *t);
void NetDlListTask_OnReply(NetMsg m, NetMsg *pm);
void NetDlFileTask_Run(s32 task);
void NetDlFileTask_Steps(NetTask *t);
void NetDlFileTask_Done(NetTask *t);
void NetDlFileTask_Failed(NetTask *t);
void NetDlFileTask_Step0(NetTask *t);
void NetDlFileTask_Step1(NetTask *t);
void NetDlFileTask_Step2(NetTask *t);
void NetDlFileTask_OnReply(NetMsg m, NetMsg *pm);
s32 LobbyReq_DlData(u8 arg0, s32 arg1, s32 arg2, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyPkt_DlData(u8 arg0, s32 arg1, s32 arg2);
s32 LobbyReq_DlList(void (*onReply)(NetMsg, NetMsg *));
s32 LobbyPkt_DlList(void);
s32 LobbyReq_DlFileInfo(u8 arg0, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyPkt_DlFileInfo(u8 arg0);
s32 LobbyPkt_SaveSummaryBegin(u8 arg0);
s32 LobbyPkt_SaveSummaryPart(u8 arg0, u8 arg1, s32 arg2, u16 arg3);
s32 LobbyPkt_TownData(u8 arg0);
void Bits16_Set(u16 *arg0, s32 arg1);
s32 LobbyReq2_TownData(u8 i, void (*onReply)(NetMsg, NetMsg *));
void LobbyNotify_TownDataChangedOnReply(NetMsg m, NetMsg *pm);
s32 LobbyPkt_ServerName(void *arg0);
s32 NetReq_TakeFixed(s32 i, void (*onTick)(s32), void (*onReply)(NetMsg, NetMsg *));
void LobbyMsg_ReadErrText(void);
void Net_CallHandler(s32 i, s32 b);
s32 NetReq_CountFree(void);
s32 NetReq_FindReply(void);

void LobbyNet_SendChatSay(s32 text, u16 len, u8 option, u8 kind)
{
    LobbyPkt_ChatSay(text, len, option, kind);
}

void LobbyNet_GetChatMessage(NetChatMessage *dst)
{
    *dst = gLobbyNet.chatMessage;
}

/* the notification 6714 (sent): blob, u8, u8 */
void LobbyPkt_ChatSay(s32 text, u16 len, u8 option, u8 kind)
{
    LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_CHAT_SAY);
    LobbyPkt_PutBlob(&gLobbyPkt, (void *)text, len);
    LobbyPkt_PutU8(&gLobbyPkt, option);
    LobbyPkt_PutU8(&gLobbyPkt, kind);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
}

/* the notification 6702: a user (blob id 8, blob name 0x10, blob text 0x100,
 * u8 x4, u8) into gLobbyNet.chatMessage; route 0xB */
void LobbyNotify_ChatMessage(void)
{
    memset(&gLobbyNet.chatMessage, 0, 0x124);
    LobbyMsg_GetU8(&gLobbyNet.chatMessage.kind, LobbyMsg_GetU8(gLobbyNet.chatMessage.r.color + 3, LobbyMsg_GetU8(gLobbyNet.chatMessage.r.color + 2, LobbyMsg_GetU8(gLobbyNet.chatMessage.r.color + 1, LobbyMsg_GetU8(gLobbyNet.chatMessage.r.color, LobbyMsg_GetBlob(gLobbyNet.chatMessage.r.u.text, 0x100, LobbyMsg_GetBlob(gLobbyNet.chatMessage.r.u.name, 0x10, LobbyMsg_GetBlob(&gLobbyNet.chatMessage, 8, gLobbyRecvData))))))));
    Lobby_RouteMessage(NET_ROUTE_CHAT_MESSAGE);
}

void LobbyNet_SendTownMsg(s32 src, u16 n)
{
    LobbyPkt_TownMsg(src, n);
}

void LobbyNet_GetTownMsg(NetTownMsg *dst)
{
    *dst = gLobbyNet.townMsg;
}

/* the notification 6708 (sent): a chat message (blob) */
void LobbyPkt_TownMsg(s32 src, u16 n)
{
    LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_TOWN_MSG);
    LobbyPkt_PutBlob(&gLobbyPkt, (void *)src, n);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
}

/* the notification 6708 (received): blob id (8), blob message (0x300); route 0xD */
void LobbyNotify_TownMsg(void)
{
    LobbyMsg_GetBlob(gLobbyNet.townMsg.text, 0x300, LobbyMsg_GetBlob(&gLobbyNet.townMsg, 8, gLobbyRecvData));
    Lobby_RouteMessage(NET_ROUTE_TOWN_MSG);
}

/*
 * Sends 670B as a request; onReply gets its reply. Returns the request's
 * slot, -1 if none is free.
 */
s32 LobbyReq_ChatTell(u8 value, void *src, s32 text, u16 n, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_ChatTell(value, src, text, n);
        return i;
    }
    return -1;
}

/* 670B: blob id (6), blob text, u8 */
s32 LobbyPkt_ChatTell(u8 value, void *src, s32 text, u16 n)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_CHAT_TELL) & 0xFFFF;
    LobbyPkt_PutBlob(&gLobbyPkt, src, 6);
    LobbyPkt_PutBlob(&gLobbyPkt, (void *)text, n);
    LobbyPkt_PutU8(&gLobbyPkt, value);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 670B */
void LobbyReply_ChatTell(void)
{
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/* the notification 670C: the same user record as 6702's; route 0xC */
void LobbyNotify_ChatTellMessage(void)
{
    memset(&gLobbyNet.chatMessage, 0, 0x124);
    LobbyMsg_GetU8(&gLobbyNet.chatMessage.kind, LobbyMsg_GetU8(gLobbyNet.chatMessage.r.color + 3, LobbyMsg_GetU8(gLobbyNet.chatMessage.r.color + 2, LobbyMsg_GetU8(gLobbyNet.chatMessage.r.color + 1, LobbyMsg_GetU8(gLobbyNet.chatMessage.r.color, LobbyMsg_GetBlob(gLobbyNet.chatMessage.r.u.text, 0x100, LobbyMsg_GetBlob(gLobbyNet.chatMessage.r.u.name, 0x10, LobbyMsg_GetBlob(&gLobbyNet.chatMessage, 8, gLobbyRecvData))))))));
    Lobby_RouteMessage(NET_ROUTE_CHAT_TELL);
}

/*
 * Sends 670D as a request; onReply gets its reply. Returns the request's
 * slot, -1 if none is free.
 */
s32 LobbyReq_TownMsgTo(void *src, void *data, u16 n, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_TownMsgTo(src, (s32) data, n);
        return i;
    }
    return -1;
}

/* 670D: blob id (6), blob text */
s32 LobbyPkt_TownMsgTo(void *src, s32 data, u16 n)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_TOWN_MSG_TO) & 0xFFFF;
    LobbyPkt_PutBlob(&gLobbyPkt, src, 6);
    LobbyPkt_PutBlob(&gLobbyPkt, (void *)data, n);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 670D */
void LobbyReply_TownMsgTo(void)
{
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/* the notification 670E: blob id (8), blob message (0x300); route 0xD */
void LobbyNotify_TownMsgToRecv(void)
{
    LobbyMsg_GetBlob(gLobbyNet.townMsg.text, 0x300, LobbyMsg_GetBlob(&gLobbyNet.townMsg, 8, gLobbyRecvData));
    Lobby_RouteMessage(NET_ROUTE_TOWN_MSG);
}

/* the reply to 6181 */
void LobbyReply_6181(void)
{
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/* the reply to 6188 */
void LobbyReply_6188(void)
{
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/* starts task 13: the files offered for download (6881, then 6883 for each) */
s32 NetDlListTask_Start(void (*onDone)(NetMsg, NetMsg *))
{
    NetTask *netTask = &gLobbyNet.task[NET_TASK_DL_LIST];

    if (netTask->active == 0) {
        memset(netTask, 0, sizeof(NetTask));
        gLobbyNet.dlCount = 0;
        memset(gLobbyNet.dlFiles, 0, 0x100);
        netTask->active = 1;
        netTask->onDone = onDone;
        netTask->func = NetDlListTask_Run;
        return 0;
    }
    return -1;
}

/*
 * The task's frame function: runs the current step, or ends the task when
 * it is done or failed.
 */
void NetDlListTask_Run(s32 task)
{
    NetTask *netTask = &gLobbyNet.task[NET_TASK_DL_LIST];

    if (netTask->active != 0) {
        switch (netTask->state) {
        case NET_TASK_RUNNING:
            NetDlListTask_Steps(netTask);
            break;
        case NET_TASK_DONE:
            NetDlListTask_Done(netTask);
            break;
        case NET_TASK_FAILED:
            NetDlListTask_Failed(netTask);
            break;
        }
    }
}

/* Runs the task's current step. */
void NetDlListTask_Steps(NetTask *task)
{
    switch (task->step) {
    case 0:
        NetDlListTask_Step0(task);
        break;
    case 1:
        NetDlListTask_Step1(task);
        break;
    case 2:
        NetDlListTask_Step2(task);
        break;
    case 3:
        NetDlListTask_Step3(task);
        break;
    }
}

/* Ends the task: onDone gets kind 0. */
void NetDlListTask_Done(NetTask *task)
{
    NetMsg msg;

    task->active = 0;
    task->state = NET_TASK_RUNNING;
    msg.kind = 0;
    task->onDone(msg, &msg);
}

/* Ends the task: onDone gets kind -1. */
void NetDlListTask_Failed(NetTask *task)
{
    NetMsg msg;

    task->active = 0;
    task->state = NET_TASK_RUNNING;
    msg.kind = -1;
    task->onDone(msg, &msg);
}

/* task 13, step 0: 6881 */
void NetDlListTask_Step0(NetTask *task)
{
    task->step++;
    task->reply = NET_REPLY_NONE;
    LobbyReq_DlList(NetDlListTask_OnReply);
}

/* task 13, step 1: 6881's reply; nothing offered is a failure */
void NetDlListTask_Step1(NetTask *task)
{
    if (task->reply == NET_REPLY_OK) {
        task->reply = NET_REPLY_NONE;
        if (gLobbyNet.dlCount == 0) {
            task->state = NET_TASK_FAILED;
        } else {
            task->step++;
            task->idx = 0;
            task->pos = 0;
            task->reqCount = 0;
        }
    } else if (task->reply == NET_REPLY_ERROR) {
        task->state = NET_TASK_FAILED;
    }
}

/* task 13, step 2: 6883 for each file, one a frame while a request slot is free */
void NetDlListTask_Step2(NetTask *task)
{
    u8 i = task->pos;

    if (NetReq_CountFree() > 0) {
        LobbyReq_DlFileInfo(i, NetDlListTask_OnReply);
        task->pos++;
        if ((u8)(i + 1) >= gLobbyNet.dlCount) {
            task->step++;
        }
    }
}

/* task 13, step 3: the last reply */
void NetDlListTask_Step3(NetTask *task)
{
    if (task->reply == NET_REPLY_OK) {
        task->reply = NET_REPLY_NONE;
        if (gLobbyNet.dlCount == 0) {
            task->state = NET_TASK_FAILED;
        } else {
            task->state = NET_TASK_DONE;
        }
    } else if (task->reply == NET_REPLY_ERROR) {
        task->state = NET_TASK_FAILED;
    }
}

/* task 13's reply callback */
void NetDlListTask_OnReply(NetMsg msg, NetMsg *msgRef)
{
    if (msg.kind == 0) {
        gLobbyNet.task[NET_TASK_DL_LIST].reply = NET_REPLY_OK;
    } else {
        gLobbyNet.task[NET_TASK_DL_LIST].reply = NET_REPLY_ERROR;
    }
}

/* the files offered: their number and table */
s32 LobbyNet_GetDlFiles(u8 *count, NetDlFile **files)
{
    *count = gLobbyNet.dlCount;
    *files = gLobbyNet.dlFiles;
    return 0;
}

/* starts task 14: downloads file `file` (size bytes) into buf with 6882 */
s32 NetDlFileTask_Start(u8 file, u8 *buf, s32 size, void (*onDone)(NetMsg, NetMsg *))
{
    NetTask *netTask = &gLobbyNet.task[NET_TASK_DL_FILE];

    if (netTask->active == 0) {
        memset(netTask, 0, sizeof(NetTask));
        gLobbyNet.dlFile = file;
        gLobbyNet.dlBuf = buf;
        gLobbyNet.dlSize = size;
        gLobbyNet.dlGot = 0;
        netTask->active = 1;
        netTask->onDone = onDone;
        netTask->func = NetDlFileTask_Run;
        return 0;
    }
    return -1;
}

/*
 * The task's frame function: runs the current step, or ends the task when
 * it is done or failed.
 */
void NetDlFileTask_Run(s32 task)
{
    NetTask *netTask = &gLobbyNet.task[NET_TASK_DL_FILE];

    if (netTask->active != 0) {
        switch (netTask->state) {
        case NET_TASK_RUNNING:
            NetDlFileTask_Steps(netTask);
            break;
        case NET_TASK_DONE:
            NetDlFileTask_Done(netTask);
            break;
        case NET_TASK_FAILED:
            NetDlFileTask_Failed(netTask);
            break;
        }
    }
}

/* Runs the task's current step. */
void NetDlFileTask_Steps(NetTask *task)
{
    switch (task->step) {
    case 0:
        NetDlFileTask_Step0(task);
        break;
    case 1:
        NetDlFileTask_Step1(task);
        break;
    case 2:
        NetDlFileTask_Step2(task);
        break;
    }
}

/* Ends the task: onDone gets kind 0. */
void NetDlFileTask_Done(NetTask *task)
{
    NetMsg msg;

    task->active = 0;
    task->state = NET_TASK_RUNNING;
    msg.kind = 0;
    task->onDone(msg, &msg);
}

/* Ends the task: onDone gets kind -1. */
void NetDlFileTask_Failed(NetTask *task)
{
    NetMsg msg;

    task->active = 0;
    task->state = NET_TASK_RUNNING;
    msg.kind = -1;
    task->onDone(msg, &msg);
}

/* task 14, step 0: nothing to fetch is a failure */
void NetDlFileTask_Step0(NetTask *task)
{
    if (gLobbyNet.dlSize != 0) {
        task->step++;
        task->reply = NET_REPLY_NONE;
        task->pos = 0;
        return;
    }
    task->state = NET_TASK_FAILED;
    task->step = 0;
}

/* task 14, step 1: 6882 for the next 0x200 bytes, one a frame while a request slot is free */
void NetDlFileTask_Step1(NetTask *task)
{
    u32 off;
    u32 size;
    u32 n;

    if (NetReq_CountFree() > 0) {
        off = task->pos;
        size = gLobbyNet.dlSize;
        if (off + 0x200 < size) {
            task->pos = off + 0x200;
            n = 0x200;
        } else {
            task->step++;
            n = size - off;
        }
        LobbyReq_DlData(gLobbyNet.dlFile, off, n, NetDlFileTask_OnReply);
    }
}

/* task 14, step 2: done once every byte arrived */
void NetDlFileTask_Step2(NetTask *task)
{
    if (task->reply == NET_REPLY_OK) {
        if (gLobbyNet.dlSize <= gLobbyNet.dlGot) {
            task->state = NET_TASK_DONE;
            task->step = 0;
        }
    } else if (task->reply & 0x80) {
        task->reply = NET_REPLY_NONE;
        task->state = NET_TASK_FAILED;
        task->step = 0;
    }
}

/* task 14's reply callback */
void NetDlFileTask_OnReply(NetMsg msg, NetMsg *msgRef)
{
    NetTask *netTask = &gLobbyNet.task[NET_TASK_DL_FILE];

    if (msg.kind == 0) {
        netTask->reply |= 1;
    } else {
        netTask->reply |= 0x80;
    }
}

/*
 * Sends 6882 as a request; onReply gets its reply. Returns the request's
 * slot, -1 if none is free.
 */
s32 LobbyReq_DlData(u8 value, s32 off, s32 n, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_DlData(value, off, n);
        return i;
    }
    return -1;
}

/* the reply to 6882: u8 file, u32 offset, blob (u32 size) into the download buffer */
void LobbyReply_DlData(void)
{
    u32 off;
    u32 n;
    u8 x;
    u8 *cur;
    u8 *dst;

    if (gLobbyNet.replyErr == 0) {
        cur = LobbyMsg_GetU32(&n, LobbyMsg_GetU32(&off, LobbyMsg_GetU8(&x, gLobbyRecvData)));
        dst = gLobbyNet.dlBuf + off;
        gLobbyNet.dlGot += n;
        LobbyMsg_GetBlob(dst, n, cur);
    }
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/* 6882: u8 file, u32 offset, u32 length */
s32 LobbyPkt_DlData(u8 value, s32 off, s32 n)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_DL_DATA) & 0xFFFF;
    LobbyPkt_PutU8(&gLobbyPkt, value);
    LobbyPkt_PutU32(&gLobbyPkt, off);
    LobbyPkt_PutU32(&gLobbyPkt, n);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/*
 * Sends 6881 as a request; onReply gets its reply. Returns the request's
 * slot, -1 if none is free.
 */
s32 LobbyReq_DlList(void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_DlList();
        return i;
    }
    return -1;
}

/* the reply to 6881: u8 count, a u32 for each file */
void LobbyReply_DlList(void)
{
    s32 i;
    NetDlFile *file = gLobbyNet.dlFiles;
    u8 n;
    u8 *cur;

    if (gLobbyNet.replyErr == 0) {
        cur = LobbyMsg_GetU8(&n, gLobbyRecvData);
        gLobbyNet.dlCount = n;
        for (i = 0; i < n; i++) {
            cur = LobbyMsg_GetU32(&file->size, cur);
            file++;
        }
    }
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/* 6881 */
s32 LobbyPkt_DlList(void)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_DL_LIST) & 0xFFFF;
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/*
 * Sends 6883 as a request; onReply gets its reply. Returns the request's
 * slot, -1 if none is free.
 */
s32 LobbyReq_DlFileInfo(u8 value, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_DlFileInfo(value);
        return i;
    }
    return -1;
}

/* 6883: u8 file */
s32 LobbyPkt_DlFileInfo(u8 value)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_DL_FILE_INFO) & 0xFFFF;
    LobbyPkt_PutU8(&gLobbyPkt, value);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 6883: u8 file, u32, u32 */
void LobbyReply_DlFileInfo(void)
{
    u32 size;
    u32 stamp;
    u8 i;

    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_GetU32(&stamp, LobbyMsg_GetU32(&size, LobbyMsg_GetU8(&i, gLobbyRecvData)));
        gLobbyNet.dlFiles[i].size = size;
        gLobbyNet.dlFiles[i].stamp = stamp;
    }
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/* the notification 6010: an error text; route 0x2D */
void LobbyNotify_ServerMessage(void)
{
    LobbyMsg_ReadErrText();
    Lobby_RouteMessage(NET_ROUTE_SERVER_MESSAGE);
}

/* the reply to 68A0: u8 n (at most 3), four u8 */
void LobbyReply_68A0(void)
{
    u8 *dst;
    u8 n;
    u8 *cur;

    if (gLobbyNet.replyErr == 0) {
        cur = LobbyMsg_GetU8(&n, gLobbyRecvData);
        if (n > 3) {
            n = 3;
        }
        dst = (u8 *)gLobbyNet.x68A0;
        gLobbyNet.x68A0n = n;
        cur = LobbyMsg_GetU8(dst, cur);
        cur = LobbyMsg_GetU8(dst + 1, cur);
        cur = LobbyMsg_GetU8(dst + 2, cur);
        LobbyMsg_GetU8(dst + 3, cur);
    }
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/* the reply to 68A1: a string */
void LobbyReply_68A1(void)
{
    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_GetString(gLobbyNet.x68A1Str, gLobbyRecvData);
    }
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/*
 * Sends 6148 as a request; onReply gets its reply. Returns the request's
 * slot, -1 if none is free.
 */
s32 LobbyReq_SaveSummaryBegin(u8 value, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_SaveSummaryBegin(value);
        return i;
    }
    return -1;
}

/* 6148: u8 */
s32 LobbyPkt_SaveSummaryBegin(u8 value)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_SAVE_SUMMARY_BEGIN) & 0xFFFF;
    LobbyPkt_PutU8(&gLobbyPkt, value);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 6148: u8 */
void LobbyReply_SaveSummaryBegin(void)
{
    gLobby6149Byte = 0;
    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_GetU8(&gLobby6149Byte, gLobbyRecvData);
    }
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/*
 * Sends 6149 as a request; onReply gets its reply. Returns the request's
 * slot, -1 if none is free.
 */
s32 LobbyReq_SaveSummaryPart(u8 value, u8 subStep2, s32 src, u16 n, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_SaveSummaryPart(value, subStep2, src, n);
        return i;
    }
    return -1;
}

/* 6149: u8, u8, blob */
s32 LobbyPkt_SaveSummaryPart(u8 value, u8 subStep2, s32 src, u16 n)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_SAVE_SUMMARY_PART) & 0xFFFF;
    LobbyPkt_PutU8(&gLobbyPkt, value);
    LobbyPkt_PutU8(&gLobbyPkt, subStep2);
    LobbyPkt_PutBlob(&gLobbyPkt, (void *)src, n);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 6149: u8 */
void LobbyReply_SaveSummaryPart(void)
{
    gLobby6149Byte = 0;
    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_GetU8(&gLobby6149Byte, gLobbyRecvData);
    }
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

s32 LobbyNet_GetSaveSummaryPart(u8 *out)
{
    *out = gLobby6149Byte;
    return 0;
}

/* 6884 for slot i: its reply goes to dst (at most max bytes) */
s32 LobbyReq_TownData(u8 i, u8 *dst, s32 max, void (*onReply)(NetMsg, NetMsg *))
{
    s32 k;
    s32 seq;

    k = NetReq_Alloc(1, NULL, onReply);
    if (k != -1) {
        gLobbyNet.x6884Buf = dst;
        gLobbyNet.townDataMax = max;
        seq = LobbyPkt_TownData(i);
        gLobbyNet.req[k].seq = seq;
        gLobbyNet.req[k].unk1A = 0;
        return k;
    }
    return -1;
}

/* 6884: u8 slot */
s32 LobbyPkt_TownData(u8 value)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_TOWN_DATA) & 0xFFFF;
    LobbyPkt_PutU8(&gLobbyPkt, value);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

void Bits16_Set(u16 *slotMask, s32 i)
{
    *slotMask |= (1 << (i & 0xFFFF)) & 0xFFFF;
}

/* the reply to 6884: u8 slot, blob; for a request of the first list, or a
 * slot of the second (gLobbyNet.req2) */
void LobbyReply_TownData(void)
{
    u8 i;
    s32 k;
    u8 *cur;

    gLobbyNet.townDataSlot = 0;
    if (NetReq_FindReply() != -1) {
        if (gLobbyNet.replyErr == 0) {
            LobbyMsg_GetBlob(gLobbyNet.x6884Buf, gLobbyNet.slotMax, LobbyMsg_GetU8(&gLobbyNet.townDataSlot, gLobbyRecvData));
        }
    } else if (gLobbyNet.replyErr == 0) {
        for (k = 0; k < 0x10; k++) {
            if (gLobbyNet.echo == gLobbyNet.req2[k].seq) {
                cur = LobbyMsg_GetU8(&i, gLobbyRecvData);
                if (gLobbyNet.slotBuf[i] != NULL) {
                    LobbyMsg_GetBlob(gLobbyNet.slotBuf[i], gLobbyNet.slotMax, cur);
                    Bits16_Set((u16 *)&gLobbyNet.slotMask, i);
                }
                break;
            }
        }
    }
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

s32 LobbyNet_GetTownData(u8 *slot)
{
    *slot = gLobbyNet.townDataSlot;
    return 0;
}

/* registers the buffer of slot i (the notification 6885 fills it) */
s32 LobbyNet_SetSlotBuf(u8 i, char *buf, s32 n)
{
    *(s32 *)((u8 *)gLobbyNet.slotBuf + ((i & 0xFF) * 4)) = (s32) buf;
    gLobbyNet.slotMax = n;
    gLobbyNet.unk4A912 = 0;
    gLobbyNet.slotMask = 0;
    memset(gLobbyNet.req2, 0, 0x1C0);
    return 0;
}

/* 6884 for slot i on gLobbyNet.req2[i] */
s32 LobbyReq2_TownData(u8 i, void (*onReply)(NetMsg, NetMsg *))
{
    NetReq_TakeFixed(i, NULL, onReply);
    gLobbyNet.req2[i].seq = LobbyPkt_TownData(i);
    return 1;
}

/* 6884's reply after 6885's notice: calls the town data's route handler when it came. */
void LobbyNotify_TownDataChangedOnReply(NetMsg msg, NetMsg *msgRef)
{
    if (msg.kind == 0) {
        Net_CallHandler(NET_ROUTE_TOWN_DATA, 0);
    }
}

/* the notification 6885: u16, a bit for each slot to fetch (6884) */
void LobbyNotify_TownDataChanged(void)
{
    u16 mask = 0;
    s32 i;

    LobbyMsg_GetU16(&mask, gLobbyRecvData);
    if (mask != 0) {
        gLobbyNet.slotMask = 0;
    }
    for (i = 0; i < 0x10; i++) {
        if (((mask >> i) & 1) && gLobbyNet.slotBuf[i] != NULL) {
            LobbyReq2_TownData(i, LobbyNotify_TownDataChangedOnReply);
        }
    }
}

/*
 * Sends 68C0 as a request; onReply gets its reply. Returns the request's
 * slot, -1 if none is free.
 */
s32 LobbyReq_ServerName(void *src, void *buf, s32 size, void (*onReply)(NetMsg, NetMsg *))
{
    s32 req;

    req = NetReq_Alloc(1, 0, onReply);
    if (req != -1) {
        gLobby68C0Buf = buf;
        gLobbyServerNameSize = (s16) size;
        gLobbyNet.req[req].seq = LobbyPkt_ServerName(src);
        return req;
    }
    return -1;
}

/* 68C0: blob (0xC) */
s32 LobbyPkt_ServerName(void *src)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_SERVER_NAME) & 0xFFFF;
    LobbyPkt_PutBlob(&gLobbyPkt, src, 0xC);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 68C0: a blob */
void LobbyReply_ServerName(void)
{
    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_GetBlob(gLobby68C0Buf, gLobbyServerNameSize, gLobbyRecvData);
    }
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/* the replies to 6144, 6145, 6146 */
void LobbyReply_6144(void)
{
}

void LobbyReply_6145(void)
{
}

void LobbyReply_6146(void)
{
}

/* the reply to the server's 6138: 0x5678, blob (0xF), u8 0, u8 0, u8, eight u16;
 * or only the flag byte 0xFF */
void LobbyPkt_6138(void)
{
    Net6138Answer *answer = (Net6138Answer *)gLobby6138Data;

    LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_6138_ANSWER);
    if (answer->unk10 != 0) {
        LobbyPkt_PutU16(&gLobbyPkt, 0x5678);
        LobbyPkt_PutBlob(&gLobbyPkt, answer, 0xF);
        LobbyPkt_PutU8(&gLobbyPkt, 0);
        LobbyPkt_PutU8(&gLobbyPkt, 0);
        LobbyPkt_PutU8(&gLobbyPkt, answer->unk10);
        LobbyPkt_PutU16(&gLobbyPkt, answer->unk14[0]);
        LobbyPkt_PutU16(&gLobbyPkt, answer->unk14[1]);
        LobbyPkt_PutU16(&gLobbyPkt, answer->unk14[2]);
        LobbyPkt_PutU16(&gLobbyPkt, answer->unk14[3]);
        LobbyPkt_PutU16(&gLobbyPkt, answer->unk14[4]);
        LobbyPkt_PutU16(&gLobbyPkt, answer->unk14[5]);
        LobbyPkt_PutU16(&gLobbyPkt, answer->unk14[6]);
        LobbyPkt_PutU16(&gLobbyPkt, answer->unk14[7]);
    } else {
        LobbyPkt_SetError(&gLobbyPkt, 0xFF);
    }
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
}

/* 6139: 0x7654, bytes */
s32 LobbyPkt_QuestRecord(void *src, s32 n)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_QUEST_RECORD) & 0xFFFF;
    LobbyPkt_PutU16(&gLobbyPkt, 0x7654);
    LobbyPkt_PutBytes2(&gLobbyPkt, src, n & 0xFFFF);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* reads 6417's reply (NetInfo6417) into gLobbyNet.x6417 */
void LobbyReply_6417Read(void)
{
    NetInfo6417 *info = (NetInfo6417 *)&gLobbyNet.x6417;
    u16 n;
    u8 *cur;
    s32 i;

    cur = LobbyMsg_GetU16(&info->unk0, gLobbyRecvData);
    cur = LobbyMsg_GetU16(&info->unk2, cur);
    cur = LobbyMsg_GetU8(&info->unk4[0], cur);
    cur = LobbyMsg_GetU8(&info->unk4[1], cur);
    cur = LobbyMsg_GetU8(&info->unk4[2], cur);
    cur = LobbyMsg_GetU8(&info->unk4[3], cur);
    cur = LobbyMsg_GetU16(&n, cur);
    cur = LobbyMsg_GetU16(&info->unkA, cur);
    cur = LobbyMsg_GetU8(&info->unk10D, cur);
    if (n > 4) {
        n = 4;
    }
    info->n = n;
    cur = LobbyMsg_GetString(info->text, cur);
    for (i = 0; i < n; i++) {
        cur = LobbyMsg_GetString(info->members[i].id, cur);
        cur = LobbyMsg_GetString(info->members[i].name, cur);
        cur = LobbyMsg_GetU8(&info->members[i].unk19, cur);
        cur = LobbyMsg_GetU8(&info->members[i].unk1A, cur);
    }
}

/* reads 631A's reply (NetLevel1SearchInfo) into gLobbyNet.level1SearchInfo */
void LobbyReply_Level1SearchInfoRead(void)
{
    s32 i;
    NetLevel1SearchInfo *info = (NetLevel1SearchInfo *)&gLobbyNet.level1SearchInfo;
    u16 n;
    u8 buf[0x10];
    u8 *cur;

    cur = LobbyMsg_GetString(info->serverId, gLobbyRecvData);
    cur = LobbyMsg_GetU16(&info->level0Id, cur);
    cur = LobbyMsg_GetU16(&info->level1Id, cur);
    cur = LobbyMsg_GetString(info->level0Name, cur);
    cur = LobbyMsg_GetString(info->level1Text, cur);
    cur = LobbyMsg_GetU16(&info->unkA0[0], cur);
    cur = LobbyMsg_GetU16(&info->unkA0[1], cur);
    cur = LobbyMsg_GetU16(&info->unkA0[2], cur);
    cur = LobbyMsg_GetU16(&info->unkA0[3], cur);
    cur = LobbyMsg_GetU8(&info->groupInfo[0], cur);
    cur = LobbyMsg_GetU8(&info->groupInfo[1], cur);
    cur = LobbyMsg_GetU8(&info->groupInfo[2], cur);
    cur = LobbyMsg_GetU8(&info->groupInfo[3], cur);
    cur = LobbyMsg_GetString(info->textB0, cur);
    cur = LobbyMsg_GetU16(&n, cur);
    cur = LobbyMsg_GetU16(&info->unkAA, cur);
    cur = LobbyMsg_GetU16(&info->unkAC, cur);
    if (n > 10) {
        n = 10;
    }
    info->n = n;
    for (i = 0; i < n; i++) {
        cur = LobbyMsg_GetString(info->users[i].id, cur);
        cur = LobbyMsg_GetString(info->users[i].name, cur);
        cur = LobbyMsg_GetBlob(buf, 0x10, cur);
        info->users[i].hr = (buf[1] << 8) | buf[0];
        info->users[i].icon = buf[3];
    }
}

