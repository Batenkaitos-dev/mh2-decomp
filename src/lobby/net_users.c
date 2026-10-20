/*
 * The lobby protocol's commands about other users and searches. Network
 * code: it stays byte for byte, as the whole build does.
 * - Short mail: one sent to a user (6704), one received (its
 *   notification, and the server's 6706).
 * - A user's profile (6713) and status (6707), the chat's colour mode
 *   (670F).
 * - The searches, each answered in pages: a reply carries the total, the
 *   first record's index, the count and whether it is the last page, and
 *   the next page is asked for from count + start (gLobbyNet.searchReq is
 *   the request it reuses). Users by conditions (6709) or by name (6711),
 *   6416's 16-byte records, level 1's entries (6319) and one entry's
 *   details (631A).
 * - The party's ready notice and the quest's payment (the last two).
 */
#include "common.h"
#include "lobby_net.h"
#include "lobby_globals.h"

u8 * LobbyMsg_GetString(void *dst, u8 *src);
u8 * LobbyMsg_GetU32(void *dst, u8 *src);
void LobbyReply_UserProfile(void);
void LobbySrvReq_6706(void);
s32 LobbyNet_GetUserSearch(NetUserSearchFound **out);
void LobbyNet_GetUserStatus(u16 *arg0, u16 *arg1, u16 *arg2, u8 *arg3);
void LobbyReply_UserStatus(void);
void LobbyReply_6417(void);
s32 LobbyNet_GetUserFindByName(NetUserFindByNameFound **out);
s32 LobbyNet_GetLevel1Search(u32 **arg0);
void LobbyReply_Level1SearchInfo(void);
s32 LobbyPkt_QuestPaid(void);
char *strcpy(void *dst, const void *src);
void Lobby_RouteMessage(s32 a);
s32 LobbyPkt_ShortMailSend(void *id, char *text);
s32 LobbyPkt_UserProfile(void *id);
s32 LobbyPkt_UserSearch(NetUserSearchQuery query);
s32 LobbyPkt_UserSearchNext(u8 arg0);
s32 LobbyPkt_6416Next(u8 arg0);
s32 LobbyPkt_UserStatus(void *id);
s32 LobbyPkt_ChatColorMode(u8 arg0);
s32 LobbyPkt_UserFindByName(NetBlob name);
s32 LobbyPkt_UserFindByNameNext(u8 arg0);
s32 LobbyPkt_Level1Search(NetLevel1SearchQuery query);
s32 LobbyPkt_Level1SearchNext(u8 arg0);
s32 LobbyPkt_Level1SearchInfo(u8 arg0);
void LobbyNotify_ShortMailRead(void);
void LobbyReply_UserProfileRead(void);
void LobbyReply_UserStatusRead(void);
void LobbyReply_6417Read(void);
void LobbyReply_Level1SearchInfoRead(void);

/* The notification of a short mail: sender's id, name and text into
 * gLobbyNet.shortMail. */
void LobbyNotify_ShortMail(void)
{
    LobbyNotify_ShortMailRead();
    Lobby_RouteMessage(NET_ROUTE_SHORT_MAIL);
}

/* 6704: a short mail to user id: blob id (6), blob text */
s32 LobbyReq_ShortMailSend(void *id, char *text, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_ShortMailSend(id, text);
        return i;
    }
    return -1;
}

/* Copies the last short mail's sender id, name and text (each if not NULL). */
void LobbyNet_GetShortMail(char *id, u8 *name, u8 *text)
{
    if (id != 0) {
        strcpy((void *) id, gLobbyNet.shortMail.id);
    }
    if (name != NULL) {
        strcpy(name, gLobbyNet.shortMail.name);
    }
    if (text != NULL) {
        strcpy(text, gLobbyNet.shortMail.text);
    }
}

/* The reply to 6704: completes its request. */
void LobbyReply_ShortMailSend(void)
{
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/* Sends 6704; returns its sequence number. */
s32 LobbyPkt_ShortMailSend(void *id, char *text)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_SHORT_MAIL_SEND);
    LobbyPkt_PutBlob(&gLobbyPkt, id, 6);
    LobbyPkt_PutBlob(&gLobbyPkt, text, strlen(text));
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* Reads a short mail: strings id, name, text. */
void LobbyNotify_ShortMailRead(void)
{
    LobbyMsg_GetString(gLobbyNet.shortMail.text, LobbyMsg_GetString(gLobbyNet.shortMail.name, LobbyMsg_GetString(gLobbyNet.shortMail.id, gLobbyRecvData)));
}

/* 6713: blob id (6); the reply fills gLobbyNet.userProfile */
s32 LobbyReq_UserProfile(void *id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    memset(&gLobbyNet.userProfile, 0, sizeof(gLobbyNet.userProfile));
    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_UserProfile(id);
        return i;
    }
    return -1;
}

/* Copies the profile 6713's reply brought. */
void LobbyNet_GetUserProfile(NetUserProfile *dst)
{
    *dst = gLobbyNet.userProfile;
}

/* The reply to 6713: the profile read unless an error. */
void LobbyReply_UserProfile(void)
{
    if (gLobbyNet.replyErr == 0) {
        LobbyReply_UserProfileRead();
    }
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/* Sends 6713; returns its sequence number. */
s32 LobbyPkt_UserProfile(void *id)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_USER_PROFILE);
    LobbyPkt_PutBlob(&gLobbyPkt, id, 6);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* 6713's reply: a user's details (NetUserProfile) into gLobbyNet.userProfile */
void LobbyReply_UserProfileRead(void)
{
    NetUserProfile *profile;
    u8 *cur;
    s32 i;

    memset(&gLobbyNet.userStatus, 0, 0x20A);
    memset(&gLobbyNet.userProfile, 0, sizeof(gLobbyNet.userProfile));
    profile = &gLobbyNet.userProfile;
    cur = LobbyMsg_GetBlob(profile->s0, 0x10, gLobbyRecvData);
    cur = LobbyMsg_GetBlob(profile->s14, 0xC, cur);
    cur = LobbyMsg_GetU16(&profile->h3B8, cur);
    cur = LobbyMsg_GetU16(&profile->h3BA, cur);
    cur = LobbyMsg_GetU8(&profile->b3C8, cur);
    cur = LobbyMsg_GetBlob(profile->s24, 0x100, cur);
    cur = LobbyMsg_GetBlob(profile->s128, 0x100, cur);
    cur = LobbyMsg_GetU32(&profile->w3C4, cur);
    cur = LobbyMsg_GetBlob(profile->s22C, 0x40, cur);
    cur = LobbyMsg_GetU8(&profile->b3C9, cur);
    cur = LobbyMsg_GetBlob(profile->s270, 0x40, cur);
    cur = LobbyMsg_GetBlob(profile->s2B4, 0x100, cur);
    cur = LobbyMsg_GetU8(&profile->b3CA, cur);
    for (i = 0; i < 4; i++) {
        cur = LobbyMsg_GetU8(&profile->b3C0[i], cur);
    }
    cur = LobbyMsg_GetU16(&profile->h3BC, cur);
    LobbyMsg_GetU16(&profile->h3BE, cur);
}

/* The server's request 6706: a short mail without the sender's id (name,
 * text into gLobbyNet.shortMail). */
void LobbySrvReq_6706(void)
{
    LobbyMsg_GetString(gLobbyNet.shortMail.text, LobbyMsg_GetString(gLobbyNet.shortMail.name, gLobbyRecvData));
    Lobby_RouteMessage(NET_ROUTE_6706);
}

/* 6709: searches users (its pages fill gLobbyNet.userSearchFound.users) */
s32 LobbyReq_UserSearch(NetUserSearchQuery query, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    memset(&gLobbyNet.userSearchFound, 0, sizeof(NetUserSearchFound));
    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_UserSearch(query);
        gLobbyNet.searchReq = i;
        return i;
    }
    return -1;
}

/* Points out at the users the search found; returns 0. */
s32 LobbyNet_GetUserSearch(NetUserSearchFound **out)
{
    *out = &gLobbyNet.userSearchFound;
    return 0;
}

/* 6709: u8, u8 n, n conditions (u8 type: 1 a 6-byte id, 2 a string, 3-5 a byte, 6 two bytes) */
s32 LobbyPkt_UserSearch(NetUserSearchQuery query)
{
    s32 seq;
    s32 i;
    u8 n;
    u8 type;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_USER_SEARCH);
    LobbyPkt_PutU8(&gLobbyPkt, query.kind);
    n = query.n;
    LobbyPkt_PutU8(&gLobbyPkt, n);
    for (i = 0; i < n; i++) {
        type = query.cond[i].type;
        LobbyPkt_PutU8(&gLobbyPkt, type);
        switch (type) {
        case 0:
            break;
        case 1:
            LobbyPkt_PutBlob(&gLobbyPkt, query.cond[i].data, 6);
            break;
        case 2:
            LobbyPkt_PutBlob(&gLobbyPkt, query.cond[i].data, query.cond[i].n);
            break;
        case 3:
            LobbyPkt_PutU8(&gLobbyPkt, query.cond[i].data[0]);
            break;
        case 4:
            LobbyPkt_PutU8(&gLobbyPkt, query.cond[i].data[0]);
            break;
        case 5:
            LobbyPkt_PutU8(&gLobbyPkt, query.cond[i].data[0]);
            break;
        case 6:
            LobbyPkt_PutU8(&gLobbyPkt, query.cond[i].data[0]);
            LobbyPkt_PutU8(&gLobbyPkt, query.cond[i].data[1]);
            break;
        }
    }
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* Sends 6709; returns its sequence number. */
s32 LobbyPkt_UserSearchNext(u8 value)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_USER_SEARCH_NEXT);
    LobbyPkt_PutU8(&gLobbyPkt, value);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 6709 (and 670A): a page of users */
void LobbyReply_UserSearch(void)
{
    u8 total;
    u8 start;
    u8 count;
    u8 last;
    NetUser *user;
    u8 *cur;
    s32 i;

    if (gLobbyNet.replyErr == 0) {
        cur = LobbyMsg_GetU8(&total, gLobbyRecvData);
        cur = LobbyMsg_GetU8(&start, cur);
        cur = LobbyMsg_GetU8(&count, cur);
        cur = LobbyMsg_GetU8(&last, cur);
        user = &gLobbyNet.userSearchFound.users[start];
        for (i = 0; i < count; i++) {
            cur = LobbyMsg_GetBlob(user->id, sizeof(user->id), cur);
            cur = LobbyMsg_GetBlob(user->name, 0x10, cur);
            cur = LobbyMsg_GetBlob(user->text, sizeof(user->text), cur);
            user++;
        }
        if (last == 0) {
            gLobbyNet.req[gLobbyNet.searchReq].seq = LobbyPkt_UserSearchNext(count + start);
            return;
        }
        gLobbyNet.userSearchFound.total = total;
    }
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/* Sends 6416; returns its sequence number. */
s32 LobbyPkt_6416Next(u8 value)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_6416_NEXT);
    LobbyPkt_PutU8(&gLobbyPkt, value);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 6416: a page of 16-byte records */
void LobbyReply_6416(void)
{
    u8 total;
    u8 start;
    u8 count;
    u8 last;
    Net6416Rec *rec;
    u8 *cur;
    s32 i;

    if (gLobbyNet.replyErr == 0) {
        cur = LobbyMsg_GetU8(&total, gLobbyRecvData);
        cur = LobbyMsg_GetU8(&start, cur);
        cur = LobbyMsg_GetU8(&count, cur);
        cur = LobbyMsg_GetU8(&last, cur);
        rec = &gLobbyNet.found6416.list[start];
        for (i = 0; i < count; i++) {
            cur = LobbyMsg_GetU8(&rec->b[0], cur);
            cur = LobbyMsg_GetU8(&rec->b[1], cur);
            cur = LobbyMsg_GetU8(&rec->b[2], cur);
            cur = LobbyMsg_GetU8(&rec->b[3], cur);
            cur = LobbyMsg_GetU16(&rec->h[0], cur);
            cur = LobbyMsg_GetU16(&rec->h[1], cur);
            cur = LobbyMsg_GetU16(&rec->h[2], cur);
            cur = LobbyMsg_GetU16(&rec->h[3], cur);
            cur = LobbyMsg_GetU8(&rec->c, cur);
            rec++;
        }
        if (last == 0) {
            gLobbyNet.req[gLobbyNet.searchReq].seq = LobbyPkt_6416Next(count + start);
            return;
        }
        gLobbyNet.found6416.total = total;
    }
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/* 6707: a user's status: blob id (6) */
s32 LobbyReq_UserStatus(void *id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_UserStatus(id);
        return i;
    }
    return -1;
}

/* Copies the status 6707's reply brought: three u16 and a byte. */
void LobbyNet_GetUserStatus(u16 *a, u16 *b, u16 *c, u8 *d)
{
    *a = gLobbyNet.userStatus.a;
    *b = gLobbyNet.userStatus.b;
    *c = gLobbyNet.userStatus.c;
    *d = gLobbyNet.userStatus.d;
}

/* The reply to 6707: the status read unless an error. */
void LobbyReply_UserStatus(void)
{
    if (gLobbyNet.replyErr == 0) {
        LobbyReply_UserStatusRead();
    }
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/* Sends 6707; returns its sequence number. */
s32 LobbyPkt_UserStatus(void *id)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_USER_STATUS);
    LobbyPkt_PutBlob(&gLobbyPkt, id, 6);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* 6707's reply: u16 x3, u8 */
void LobbyReply_UserStatusRead(void)
{
    LobbyMsg_GetU8(&gLobbyNet.userStatus.d, LobbyMsg_GetU16(&gLobbyNet.userStatus.c, LobbyMsg_GetU16(&gLobbyNet.userStatus.b, LobbyMsg_GetU16(&gLobbyNet.userStatus.a, gLobbyRecvData))));
}

/* 670F: the chat's colour mode, a byte */
s32 LobbyReq_ChatColorMode(u8 value, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_ChatColorMode(value);
        return i;
    }
    return -1;
}

/* The reply to 670F: completes its request. */
void LobbyReply_ChatColorMode(void)
{
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/* Sends 670F; returns its sequence number. */
s32 LobbyPkt_ChatColorMode(u8 value)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_CHAT_COLOR_MODE);
    LobbyPkt_PutU8(&gLobbyPkt, value);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* The reply to 6417: read unless an error. */
void LobbyReply_6417(void)
{
    if (gLobbyNet.replyErr == 0) {
        LobbyReply_6417Read();
    }
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/* 6711: u8, blob (its pages fill gLobbyNet.userFindByNameFound.list) */
s32 LobbyReq_UserFindByName(NetBlob name, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    memset(&gLobbyNet.userFindByNameFound, 0, sizeof(NetUserFindByNameFound));
    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_UserFindByName(name);
        gLobbyNet.searchReq = i;
        return i;
    }
    return -1;
}

/* Points out at the users the search by name found; returns 0. */
s32 LobbyNet_GetUserFindByName(NetUserFindByNameFound **out)
{
    *out = &gLobbyNet.userFindByNameFound;
    return 0;
}

/* Sends 6711; returns its sequence number. */
s32 LobbyPkt_UserFindByName(NetBlob name)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_USER_FIND_BY_NAME);
    LobbyPkt_PutU8(&gLobbyPkt, name.kind);
    LobbyPkt_PutBlob(&gLobbyPkt, name.data, name.n);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 6711: a page of users (blob id, blob name, string) */
void LobbyReply_UserFindByName(void)
{
    u8 total;
    u8 start;
    u8 count;
    u8 last;
    NetUserFindByNameRec *user;
    u8 *cur;
    s32 i;

    if (gLobbyNet.replyErr == 0) {
        cur = LobbyMsg_GetU8(&total, gLobbyRecvData);
        cur = LobbyMsg_GetU8(&start, cur);
        cur = LobbyMsg_GetU8(&count, cur);
        cur = LobbyMsg_GetU8(&last, cur);
        user = &gLobbyNet.userFindByNameFound.list[start];
        for (i = 0; i < count; i++) {
            cur = LobbyMsg_GetBlob(user->id, 8, cur);
            cur = LobbyMsg_GetBlob(user->name, 0x10, cur);
            cur = LobbyMsg_GetString(user->text, cur);
            user++;
        }
        if (last == 0) {
            gLobbyNet.req[gLobbyNet.searchReq].seq = LobbyPkt_UserFindByNameNext(count + start);
            return;
        }
        gLobbyNet.userFindByNameFound.total = total;
    }
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/* Sends 6711; returns its sequence number. */
s32 LobbyPkt_UserFindByNameNext(u8 value)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_USER_FIND_BY_NAME_NEXT);
    LobbyPkt_PutU8(&gLobbyPkt, value);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* 6319: a search of the second level's entries */
s32 LobbyReq_Level1Search(NetLevel1SearchQuery query, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_Level1Search(query);
        return i;
    }
    return -1;
}

/* Points out at the u32 6319's reply began with; returns 0. */
s32 LobbyNet_GetLevel1Search(u32 **out)
{
    *out = &gLobbyNet.level1Search;
    return 0;
}

/* 6319: blob (0xC), u8, u8 n, then n conditions (u8 type, u8 m, m bytes) */
s32 LobbyPkt_Level1Search(NetLevel1SearchQuery query)
{
    s32 seq;
    s32 i;
    s32 j;
    u8 n;
    u8 condLen;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL1_SEARCH) & 0xFFFF;
    LobbyPkt_PutBlob(&gLobbyPkt, query.id, 0xC);
    LobbyPkt_PutU8(&gLobbyPkt, query.maxFound);
    LobbyPkt_PutU8(&gLobbyPkt, query.n);
    n = query.n;
    for (i = 0; i < n; i++) {
        LobbyPkt_PutU8(&gLobbyPkt, query.cond[i].type);
        condLen = query.cond[i].n;
        LobbyPkt_PutU8(&gLobbyPkt, condLen);
        for (j = 0; j < condLen; j++) {
            LobbyPkt_PutU8(&gLobbyPkt, query.cond[i].values[j]);
        }
    }
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* Sends 6319; returns its sequence number. */
s32 LobbyPkt_Level1SearchNext(u8 value)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL1_SEARCH_NEXT);
    LobbyPkt_PutU8(&gLobbyPkt, value);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 6319: u32, then a page of entries */
void LobbyReply_Level1Search(void)
{
    u32 head;
    u8 total;
    u8 start;
    u8 count;
    u8 last;
    NetLevel1SearchRec *entry;
    u8 *cur;
    s32 i;

    if (gLobbyNet.replyErr == 0) {
        cur = LobbyMsg_GetU32(&head, gLobbyRecvData);
        cur = LobbyMsg_GetU8(&total, cur);
        cur = LobbyMsg_GetU8(&start, cur);
        cur = LobbyMsg_GetU8(&count, cur);
        cur = LobbyMsg_GetU8(&last, cur);
        entry = &gLobbyNet.level1SearchFound.list[start];
        for (i = 0; i < count; i++) {
            cur = LobbyMsg_GetU8(&entry->groupInfo[0], cur);
            cur = LobbyMsg_GetU8(&entry->groupInfo[1], cur);
            cur = LobbyMsg_GetU8(&entry->groupInfo[2], cur);
            cur = LobbyMsg_GetU8(&entry->groupInfo[3], cur);
            cur = LobbyMsg_GetString(entry->serverId, cur);
            cur = LobbyMsg_GetU16(&entry->h118, cur);
            cur = LobbyMsg_GetU16(&entry->h11A, cur);
            cur = LobbyMsg_GetString(entry->text, cur);
            cur = LobbyMsg_GetU16(&entry->users, cur);
            cur = LobbyMsg_GetU16(&entry->maxUsers, cur);
            cur = LobbyMsg_GetU16(&entry->usersInTown, cur);
            entry->index = start + i;
            entry++;
        }
        if (last == 0) {
            gLobbyNet.req[gLobbyNet.searchReq].seq = LobbyPkt_Level1SearchNext(count + start);
            return;
        }
        gLobbyNet.level1Search = head;
        gLobbyNet.level1SearchFound.total = total;
    }
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/* 631A: u8 (the reply fills gLobbyNet.level1SearchInfo) */
s32 LobbyReq_Level1SearchInfo(u8 value, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    memset(&gLobbyNet.level1SearchInfo, 0, sizeof(gLobbyNet.level1SearchInfo));
    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_Level1SearchInfo(value);
        return i;
    }
    return -1;
}

/* Copies the entry's details 631A's reply brought. */
void LobbyNet_GetLevel1SearchInfo(NetLevel1SearchInfo *dst)
{
    *dst = gLobbyNet.level1SearchInfo;
}

/* Sends 631A; returns its sequence number. */
s32 LobbyPkt_Level1SearchInfo(u8 value)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL1_SEARCH_INFO);
    LobbyPkt_PutU8(&gLobbyPkt, value);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* The reply to 631A: read unless an error. */
void LobbyReply_Level1SearchInfo(void)
{
    if (gLobbyNet.replyErr == 0) {
        LobbyReply_Level1SearchInfoRead();
    }
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/* The notification that the quest's party is ready. */
void LobbyNotify_PartyReady(void)
{
    Lobby_RouteMessage(NET_ROUTE_PARTY_READY);
}

/* Sends the quest's payment notice; returns 0. */
s32 LobbyPkt_QuestPaid(void)
{
    LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_QUEST_PAID);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return 0;
}
