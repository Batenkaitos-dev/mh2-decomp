/*
 * The patch the server sends at login: 6121 announces it (its tag,
 * version, size and byte sum), 6122 carries it in parts appended at
 * gLobbyNet.patchDst, and 6125 says it is whole; its byte sum then
 * decides what the login task's onDone hears (NET_PROGRESS_PATCH_OK or
 * NET_PROGRESS_PATCH_BAD).
 * Network code: it stays byte for byte, as the whole build does.
 */
#include "common.h"
#include "lobby_net.h"


s32 strncpy(s32, s32, u32);             /* strncpy */

/* the notification 6121 at login, while the login task runs: a patch follows */
void LobbyNotify_PatchHeader(void)
{
    if (gLobbyNet.task[NET_TASK_LOGIN].active != 0) {
        if (gLobbyNet.flag == LOBBY_FLAG_NOTIFY) {
            NetPatch_ReadHeader();
            gLobbyNet.unk1290 = 0;
            gLobbyNet.unk1294 = 0;
            gLobbyNet.patchDst = gLobbyNet.login.patchBuf;
        } else if (gLobbyNet.flag == LOBBY_FLAG_REPLY) {
            return;
        }
    }
}

/* 6121's payload: string (a 4-character tag and 10 characters of version),
 * u16, u32 size, u32 checksum */
void NetPatch_ReadHeader(void)
{
    u8 buf[24];
    u8 *cur;

    memset(buf, 0, 24);
    cur = LobbyMsg_GetString(buf, gLobbyRecvData);
    cur = LobbyMsg_GetU16(&gLobbyNet.patchUnk, cur);
    cur = LobbyMsg_GetU32(&gLobbyNet.patchSize, cur);
    LobbyMsg_GetU32(&gLobbyNet.patchSum, cur);
    memset(gLobbyNet.patchTag, 0, 8);
    memcpy(gLobbyNet.patchTag, buf, 4);
    memset(gLobbyNet.patchVersion, 0, 16);
    memcpy(gLobbyNet.patchVersion, buf + 4, 10);
}

/* the notification 6122: a part of the patch */
void LobbyNotify_PatchData(void)
{
    if (gLobbyNet.task[NET_TASK_LOGIN].active != 0) {
        NetPatch_ReadData();
    }
}

/* 6122's payload: u16, u16 n, n bytes appended to the patch */
void NetPatch_ReadData(void)
{
    u16 n;
    u16 x;
    u8 *cur;

    cur = LobbyMsg_GetU16(&n, LobbyMsg_GetU16(&x, gLobbyRecvData));
    LobbyMsg_GetBytes(gLobbyNet.patchDst, cur, n);
    gLobbyNet.patchDst += n;
}

/* the server's request 6124: u16, answered with the same u16 */
void LobbySrvReq_PatchAck(void)
{
    u16 value;

    if (gLobbyNet.task[NET_TASK_LOGIN].active != 0) {
        LobbyMsg_RecvU16(&value);
        LobbyPkt_PatchAck(value);
    }
}

/* the reply to 6124: u16 */
s32 LobbyPkt_PatchAck(u16 value)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_PATCH_ACK_ANSWER);
    LobbyPkt_PutU16(&gLobbyPkt, value);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the notification 6123 */
void LobbyNotify_6123(void)
{
}

/* the server's request 6125: the patch is whole; its checksum decides
 * (the login task's onDone: NET_PROGRESS_PATCH_OK; a bad sum: kind -1,
 * NET_PROGRESS_PATCH_BAD) */
void LobbySrvReq_PatchEnd(void)
{
    NetMsg msg;

    if (gLobbyNet.task[NET_TASK_LOGIN].active != 0) {
        if (NetPatch_CheckSum(gLobbyNet.login.patchBuf, gLobbyNet.patchSize, gLobbyNet.patchSum)) {
            msg.kind = 0;
            msg.flag = NET_PROGRESS_PATCH_OK;
            gLobbyNet.task[NET_TASK_LOGIN].onDone(msg, &msg);
        } else {
            msg.kind = -1;
            msg.flag = NET_PROGRESS_PATCH_BAD;
            gLobbyNet.task[NET_TASK_LOGIN].onDone(msg, &msg);
        }
    }
}

/* the reply to 6125 */
s32 LobbyNet_SendPatchEnd(void)
{
    LobbyPkt_PatchEnd();
    return 0;
}

/* Sends 6125; returns its sequence number. */
s32 LobbyPkt_PatchEnd(void)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_PATCH_END_ANSWER) & 0xFFFF;
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the patch's size, version and tag */
s32 LobbyNet_GetPatchInfo(NetPatchInfo *info)
{
    memset(info, 0, 28);
    strncpy((s32)info->version, (s32)gLobbyNet.patchVersion, 10);
    strncpy((s32)info->tag, (s32)gLobbyNet.patchTag, 4);
    info->size = gLobbyNet.patchSize;
    return 0;
}

/* 1 if the bytes of buf add up to sum */
s32 NetPatch_CheckSum(u8 *buf, u32 n, u32 sum)
{
    u32 i;
    u32 total = 0;

    for (i = 0; i < n; i++) {
        total += *buf++;
    }
    return sum == total;
}
