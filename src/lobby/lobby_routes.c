/*
 * The lobby's route handlers: what the client does with the notifications
 * the lobby server sends on its own (LobbyRoute_*). LobbyRoute_Register
 * registers one handler per route (Net_SetHandler, lobby_net.h's
 * NET_ROUTE_*), and Lobby_RouteMessage calls it with the message's header;
 * each handler fetches the message's body (LobbyNet_Get*) and applies it:
 * - the session: the server closing it or sending the player out, a
 *   departure cancelled, the quest party complete (LobbyRoute_PartyReady,
 *   which copies the party to sub_main for the quest);
 * - the chat: lines (their colour digits checked), tells, short mails and
 *   town messages (TownMsg_Recv);
 * - the level lists' entries (gLobbyLevel0Entries, gLobbyLevel1Entries,
 *   gLobbyLevel2Entries): a name, a state, the user counts ... of one
 *   entry, by the id of the last message (LobbyNet_GetLastId);
 * - the members: users coming into and leaving the town (gNetUsers, the
 *   host's id kept) and the quest room (LobbyState.players), their hunter
 *   cards, the server's kicks out of each level.
 * Most handlers do nothing while the lobby is being left (LOBBY_PHASE_EXIT).
 */
#include "common.h"
#include "lobby_state.h"
#include "card.h"
#include "lobby_menu.h"
#include "net_mode.h"
#include "em.h"
#include "lobby_net.h"
#include "scene.h"
#include "player.h"
#include "lobby_globals.h"
#include "save.h"
#include "syssettings.h"
#include "chat_log.h"
#include "lobby_text.h"
#include "libc_ctype.h"
#include "quest.h"
#include "sound.h"
#include "area_draw.h"
#include "lobby_routes.h"
#include "chat_phrases.h"
#include "hunter_card.h"
#include "item_shop.h"
#include "lobby_queries.h"
#include "lobby_steps.h"
#include "lobby_util.h"
#include "my_house.h"
#include "town_player_activities.h"
#include "town_player_states.h"
#include "lobby_labels.h"

s32 LobbyNet_GetErrText(char *);
extern NetUser gNetUsers[10];
void *memset(void *dst, s32 c, u32 n);
void *memcpy(void *dst, const void *src, u32 n);
extern char gPlayerIds[];       /* sub_main's copies of the quest's four players: their ids (LOBBY_USER_ID_LEN each), */
extern char gMemberNames[];     /* their names (LOBBY_USER_NAME_LEN + 1 each) */
extern char gPlayerCardTexts[];     /* and their hunter cards' texts (256 bytes each) */

/* a received mail (LobbyNet_GetShortMail copies it out) */
typedef struct MailMsg {
    /* 0x00 */ char id[8];
    /* 0x08 */ char name[20];
    /* 0x1C */ char text[128];
} MailMsg;

extern MailEntry gMailBox[MAIL_BOX_LEN];
extern BlockedUser gMailBlockList[MAIL_BLOCK_LIST_LEN];
s32 strcmp(char *s, char *word);
extern u8 gChatClassColors[];
extern u8 gLobbyLevel1Sel;      /* the level-1 entry (the town) the player is in; 0: none */
extern s8 gTownSeats[3];
extern char gSongRequesterId[8];
extern u16 gLobbyQuestId;       /* the quest taken */
extern s8 gArenaCupJoined;
extern s8 gArenaCupJoinValue;
extern u8 gChatBlockList[];     /* the chat's muted ids (CHAT_BLOCK_LIST_LEN records) */
extern s8 gCtypeTable[];
extern u8 gChatDigitColors[8];
s32 memcmp(void *a, void *b, s32 n);
extern u8 gLobbyLevel2Sel;      /* the level-2 entry (the quest room) the player is in */
extern s8 gQuestMinMembers;
extern s32 gTownMode;
s32 NetList2_GetMembers(u16 id, u16 *a, u16 *b); /* unit's own declaration: with a u16 id, as LobbyRoute_Level2Members passes it; with the definition's s32 id the call compiles differently */
void Chat_SystemMsg(s32 msg, char *arg);
extern LevelEntry gLobbyLevel1Entries[];
extern LevelEntry gLobbyLevel0Entries[];
/* The game's own mismatch: LobbyRoute_Level0Name passes the entry's name
 * where the definition takes the reply handler, which NetReq_OnReply calls:
 * a native build must not call it. */
s32 LobbyReq_Level0Name(u16 id, void *name); /* unit's own declaration: a name pointer, as the route passes it */
s32 NetList0_GetUsers(u16 id, u16 *a, u16 *b);

/* Registers the lobby's route handlers, one per route (Net_SetHandler);
 * the lobby's entry calls it once it is connected. */
void LobbyRoute_Register(void)
{
    Net_SetHandler(NET_ROUTE_PARTY_READY, LobbyRoute_PartyReady);
    Net_SetHandler(NET_ROUTE_SERVER_CLOSE, LobbyRoute_ServerClose);
    Net_SetHandler(NET_ROUTE_CHAT_MESSAGE, LobbyRoute_ChatMessage);
    Net_SetHandler(NET_ROUTE_CHAT_TELL, LobbyRoute_ChatTell);
    Net_SetHandler(NET_ROUTE_SHORT_MAIL, LobbyRoute_ShortMail);
    Net_SetHandler(NET_ROUTE_DEPART_CANCEL, LobbyRoute_DepartCancel);
    Net_SetHandler(NET_ROUTE_DEPART_ERROR, LobbyRoute_DepartError);
    Net_SetHandler(NET_ROUTE_SERVER_LEAVE, LobbyRoute_ServerLeave);
    Net_SetHandler(NET_ROUTE_LEVEL0_VALUES, LobbyRoute_Level0Values);
    Net_SetHandler(NET_ROUTE_LEVEL0_NAME, LobbyRoute_Level0Name);
    Net_SetHandler(NET_ROUTE_LEVEL0_STATE, LobbyRoute_Level0State);
    Net_SetHandler(NET_ROUTE_LEVEL1_NAME, LobbyRoute_Level1Name);
    Net_SetHandler(NET_ROUTE_LEVEL1_STATE, LobbyRoute_Level1State);
    Net_SetHandler(NET_ROUTE_LEVEL1_COLOR, LobbyRoute_Level1Color);
    Net_SetHandler(NET_ROUTE_LEVEL1_VALUES, LobbyRoute_Level1Values);
    Net_SetHandler(NET_ROUTE_LEVEL1_TEXT, LobbyRoute_Level1Text);
    Net_SetHandler(NET_ROUTE_LEVEL1_GROUP_INFO, LobbyRoute_Level1GroupInfo);
    Net_SetHandler(NET_ROUTE_LEVEL1_USER_IN, LobbyRoute_Level1UserIn);
    Net_SetHandler(NET_ROUTE_LEVEL1_USER_OUT, LobbyRoute_Level1UserOut);
    Net_SetHandler(NET_ROUTE_LEVEL2_NAME, LobbyRoute_Level2Name);
    Net_SetHandler(NET_ROUTE_LEVEL2_STATE, LobbyRoute_Level2State);
    Net_SetHandler(NET_ROUTE_LEVEL2_COLOR, LobbyRoute_Level2Color);
    Net_SetHandler(NET_ROUTE_LEVEL2_VALUES, LobbyRoute_Level2Values);
    Net_SetHandler(NET_ROUTE_LEVEL2_LIMITS, LobbyRoute_Level2Limits);
    Net_SetHandler(NET_ROUTE_LEVEL2_LOCKED, LobbyRoute_Level2Locked);
    Net_SetHandler(NET_ROUTE_LEVEL2_TEXT, LobbyRoute_Level2Text);
    Net_SetHandler(NET_ROUTE_LEVEL2_POSTED_QUEST, LobbyRoute_Level2PostedQuest);
    Net_SetHandler(NET_ROUTE_LEVEL0_KICK, LobbyRoute_Level0Kick);
    Net_SetHandler(NET_ROUTE_LEVEL1_KICK, LobbyRoute_Level1Kick);
    Net_SetHandler(NET_ROUTE_LEVEL2_KICK, LobbyRoute_Level2Kick);
    Net_SetHandler(NET_ROUTE_LEVEL2_USER_IN, LobbyRoute_Level2UserIn);
    Net_SetHandler(NET_ROUTE_LEVEL2_USER_OUT, LobbyRoute_Level2UserOut);
    Net_SetHandler(NET_ROUTE_LEVEL2_MEMBERS, LobbyRoute_Level2Members);
    Net_SetHandler(NET_ROUTE_TOWN_MSG, LobbyRoute_TownMsg);
    Net_SetHandler(NET_ROUTE_USER_CARD, LobbyRoute_UserCard);
    Net_SetHandler(NET_ROUTE_USER_CARD_PART, LobbyRoute_UserCardPart);
    Net_SetHandler(NET_ROUTE_TOWN_DATA, LobbyRoute_TownData);
}

/* The town's data (6884): applied unless the lobby is being left. */
void LobbyRoute_TownData(NetMsg msg, NetMsg *msgRef)
{
    if (gLobby->phase != LOBBY_PHASE_EXIT) {
        Lobby_ApplyTownData();
    }
}

/*
 * The quest's party is complete (6910): marks it (playersListed), clears
 * the menu's draw, tells the town the player's area is none and clears the
 * player's quest status and arena role on the hunter card (sent again).
 * LOBBY_SE_PARTY_READY sounds unless the player paid the quest. The quest
 * room's four players (their ids, names and card texts) are copied to
 * sub_main's copies, which the quest uses.
 */
void LobbyRoute_PartyReady(NetMsg msg, NetMsg *msgRef)
{
    s8 i;
    char *id;
    s32 offset;
    char *name;
    char *text;

    gLobby->playersListed = 1;
    LobbyMenu_SetDraw(0, 0, NULL, 0);
    TownMsg_SendArea(0);
    gPlayers[gScene.localPlayer].questStatus = 0;
    gPlayerText.card.arenaRole = 0;
    HunterCard_SendChanged();
    if (gLobby->questPaid == 0) {
        LobbySe_Play(LOBBY_SE_PARTY_READY);
    }
    for (i = 0, id = gPlayerIds, offset = 0, name = gMemberNames, text = gPlayerCardTexts;
         i < LOBBY_ROOM_USER_COUNT; i++) {
        memset(id, 0, LOBBY_USER_ID_LEN);
        memcpy(id, LOBBY_PLAYER_AT(offset).id, LOBBY_USER_ID_LEN);
        memcpy(name, LOBBY_PLAYER_AT(offset).name, LOBBY_USER_NAME_LEN + 1);
        memcpy(text, LOBBY_PLAYER_AT(offset).text, sizeof(gLobby->players[0].text));
        id += LOBBY_USER_ID_LEN;
        offset += sizeof(NetUser);
        name += LOBBY_USER_NAME_LEN + 1;
        text += sizeof(gLobby->players[0].text);
    }
}

/* The server closes the session (6003): its error text is kept, and
 * Lobby_Tick quits the lobby (SERVER_CLOSE_CLOSED). */
void LobbyRoute_ServerClose(NetMsg msg, NetMsg *msgRef)
{
    LobbyState *lobby = gLobby;

    if (lobby->phase != LOBBY_PHASE_EXIT) {
        lobby->serverClose = SERVER_CLOSE_CLOSED;
        LobbyNet_GetErrText(gLobby->errText);
    }
}

/* The departure for the quest is cancelled (6004): in the quest room, the
 * room closes with the error text shown. The first test is redundant; the
 * game's code makes both. */
void LobbyRoute_DepartCancel(NetMsg msg, NetMsg *msgRef)
{
    u8 phase;

    phase = gLobby->phase;
    if ((phase != LOBBY_PHASE_EXIT) && (phase == LOBBY_PHASE_ROOM)) {
        gLobby->roomCloseKind = ROOM_CLOSE_WITH_TEXT;
    }
}

/* The same with the server's text (6005): in the quest room, the text is
 * kept and the room closes without showing it. */
void LobbyRoute_DepartError(NetMsg msg, NetMsg *msgRef)
{
    u8 phase;
    LobbyState *lobby = gLobby;

    phase = lobby->phase;
    if (phase != LOBBY_PHASE_EXIT && phase == LOBBY_PHASE_ROOM) {
        LobbyNet_GetErrText(lobby->errText);
        gLobby->roomCloseKind = ROOM_CLOSE_QUIET;
    }
}

/* The server sends the player out of the lobby (6007): its text is kept,
 * and Lobby_Tick quits the lobby (SERVER_CLOSE_SENT_OUT). */
void LobbyRoute_ServerLeave(NetMsg msg, NetMsg *msgRef)
{
    LobbyState *lobby = gLobby;

    if (lobby->phase != LOBBY_PHASE_EXIT) {
        lobby->serverClose = SERVER_CLOSE_SENT_OUT;
        LobbyNet_GetErrText(gLobby->errText);
    }
}

/* A chat line's colour digit ('0'-'7') as its colour (gChatDigitColors),
 * CHAT_DIGIT_NONE for anything else. */
static u8 ChatMsg_ColorDigit(u8 digit)
{
    if (digit == 0 || !((gCtypeTable + 1)[digit] & CTYPE_DIGIT)) {
        digit = CHAT_DIGIT_NONE;
    } else {
        digit = gChatDigitColors[(digit - '0') & 7];
    }
    return digit;
}

/*
 * A chat line (6702). A chat-log line from a muted sender (gChatBlockList)
 * is dropped. The three colour digits become colours: a missing first one
 * is 0, and a chat-log line's first one is dropped (CHAT_DIGIT_NONE); a
 * missing second or third one is 0. A line without its first digit that
 * comes in a town while a raid's movie plays (or as the town comes back
 * after it) is answered with the town-state check's line, unless
 * it is that line. The line then goes to the log or a balloon
 * (Chat_Receive).
 */
void LobbyRoute_ChatMessage(NetMsg msg, NetMsg *msgRef)
{
    ChatMsg line;
    u8 *nameColor;
    u8 *textColor;
    u8 *kind;
    u8 *firstColor;

    memset(&line, 0, sizeof(line));
    LobbyNet_GetChatMessage((struct NetChatMessage *)&line);
    kind = &line.kind;
    if (*kind == CHAT_MSG_LOG && IdList_Find((s8 *) gChatBlockList, CHAT_BLOCK_LIST_LEN, &line) != -1) {
        return;
    }
    firstColor = &line.color[0];
    *firstColor = ChatMsg_ColorDigit(*firstColor);
    nameColor = &line.color[1];
    *nameColor = ChatMsg_ColorDigit(*nameColor);
    textColor = &line.color[2];
    *textColor = ChatMsg_ColorDigit(*textColor);
    if (*firstColor == CHAT_DIGIT_NONE) {
        *firstColor = 0;
        if (*kind != CHAT_MSG_LOG && gLobbyLevel1Sel != 0) {
            if ((gLobby->step == LOBBY_TOWN_STEP_MOVIE || gLobby->step == LOBBY_TOWN_STEP_REENTER) &&
                gLobby->phase == LOBBY_PHASE_TOWN &&
                strcmp(line.text, gLobbyEventTexts[LOBBY_EVENT_TOWN_CHECKING]) != 0) {
                Chat_SendText(gScene.localPlayer, gLobbyEventTexts[LOBBY_EVENT_TOWN_CHECKING], 0);
            }
        }
    } else if (*kind == CHAT_MSG_LOG) {
        *firstColor = CHAT_DIGIT_NONE;
    }
    if (*nameColor == CHAT_DIGIT_NONE) {
        *nameColor = 0;
    }
    if (*textColor == CHAT_DIGIT_NONE) {
        *textColor = 0;
    }
    Chat_Receive((u8 *) &line);
}

/*
 * A line told to the player alone (670C): its colour digits become colours
 * (a missing first or second one 0, a missing third one the sender's class
 * colour, gChatClassColors). It is answered as in LobbyRoute_ChatMessage
 * during a raid's movie, then received (Chat_Receive).
 */
void LobbyRoute_ChatTell(NetMsg msg, NetMsg *msgRef)
{
    ChatMsg line;
    u8 *firstColor;
    u8 *nameColor;
    u8 *textColor;

    memset(&line, 0, sizeof(line));
    LobbyNet_GetChatMessage((struct NetChatMessage *)&line);
    firstColor = &line.color[0];
    *firstColor = ChatMsg_ColorDigit(*firstColor);
    nameColor = &line.color[1];
    *nameColor = ChatMsg_ColorDigit(*nameColor);
    textColor = &line.color[2];
    *textColor = ChatMsg_ColorDigit(*textColor);
    if (*firstColor == CHAT_DIGIT_NONE) {
        *firstColor = 0;
    }
    if (*nameColor == CHAT_DIGIT_NONE) {
        *nameColor = 0;
    }
    if (*textColor == CHAT_DIGIT_NONE) {
        *textColor = gChatClassColors[line.cls];
    }
    if (line.kind != CHAT_MSG_LOG && gLobbyLevel1Sel != 0) {
        if ((gLobby->step == LOBBY_TOWN_STEP_MOVIE || gLobby->step == LOBBY_TOWN_STEP_REENTER) &&
            gLobby->phase == LOBBY_PHASE_TOWN &&
            strcmp(line.text, gLobbyEventTexts[LOBBY_EVENT_TOWN_CHECKING]) != 0) {
            Chat_SendText(gScene.localPlayer, gLobbyEventTexts[LOBBY_EVENT_TOWN_CHECKING], 0);
        }
    }
    Chat_Receive((u8 *) &line);
}

/*
 * A short mail arrived (6705). It is dropped when its sender is on the
 * mail block list (gMailBlockList). Otherwise the mail box (gMailBox, the
 * newest first) moves down one and the mail goes first, its text filtered
 * (Chat_FilterWords). LOBBY_SE_MAIL sounds when the box was empty, the
 * player's model is up and no raid's movie plays.
 */
void LobbyRoute_ShortMail(NetMsg msg, NetMsg *msgRef)
{
    s32 dest;
    s32 count;
    MailMsg mail;
    s32 i;
    BlockedUser *blocked;
    MailEntry *entry;
    LobbyState *lobby;
    s32 phase;

    memset(&mail, 0, sizeof(mail));
    LobbyNet_GetShortMail(mail.id, (u8 *) mail.name, (u8 *) mail.text);
    for (i = 0, blocked = gMailBlockList; i < MAIL_BLOCK_LIST_LEN; i++, blocked++) {
        if (memcmp(blocked, mail.id, LOBBY_USER_ID_LEN) == 0) {
            return;
        }
    }
    count = 0;
    for (i = 0, entry = gMailBox; i < MAIL_BOX_LEN; i++, entry++) {
        if (entry->unread != 0) {
            count++;
        }
    }
    lobby = gLobby;
    if (lobby->modelLoaded[gScene.localPlayer] != 0) {
        phase = lobby->phase;
        if ((phase != LOBBY_PHASE_TOWN || lobby->step != LOBBY_TOWN_STEP_MOVIE) &&
            (phase != LOBBY_PHASE_VILLAGE || lobby->mode != LOBBY_VILLAGE_MOVIE) && count == 0) {
            LobbySe_Play(LOBBY_SE_MAIL);
        }
    }
    /* count is the mail each one moves from */
    for (count = MAIL_BOX_LEN - 2, dest = MAIL_BOX_LEN - 1; dest > 0; count--, dest--) {
        memcpy(&gMailBox[dest], &gMailBox[count], sizeof(MailEntry));
    }
    memset(&gMailBox[0], 0, sizeof(MailEntry));
    memcpy(gMailBox[0].id, mail.id, LOBBY_USER_ID_LEN);
    memcpy(gMailBox[0].name, mail.name, LOBBY_USER_NAME_LEN);
    memcpy(gMailBox[0].text, mail.text, sizeof(gMailBox[0].text));
    Chat_FilterWords(gMailBox[0].text);
    gMailBox[0].unread = 1;
}

/* A level-0 entry's (an area's) name (6204). */
void LobbyRoute_Level0Name(NetMsg msg, NetMsg *msgRef)
{
    u16 id;

    LobbyNet_GetLastId(&id);
    LobbyReq_Level0Name(id, gLobbyLevel0Entries[id - 1].name);
}

/* A level-0 entry's user counts and its other value (6205). */
void LobbyRoute_Level0Values(NetMsg msg, NetMsg *msgRef)
{
    u16 id;
    LevelEntry *entry;

    LobbyNet_GetLastId(&id);
    entry = &gLobbyLevel0Entries[id - 1];
    NetList0_GetUsers(id, &entry->users, &entry->questUsers);
    NetList0_GetValue10(id, &gLobbyLevel0Entries[id - 1].unk14);
}

/* A level-0 entry's state (6206). */
void LobbyRoute_Level0State(NetMsg msg, NetMsg *msgRef)
{
    u16 id;

    LobbyNet_GetLastId(&id);
    NetList0_GetByte(id, &gLobbyLevel0Entries[id - 1].state);
}

/* A level-1 entry's (a town's) user counts (6303). */
void LobbyRoute_Level1Values(NetMsg msg, NetMsg *msgRef)
{
    u16 id;
    LevelEntry *entry;

    LobbyNet_GetLastId(&id);
    entry = &gLobbyLevel1Entries[id - 1];
    NetList1_GetUsers(id, &entry->users, &entry->questUsers);
}

/* A level-1 entry's recruiting text (6308). */
void LobbyRoute_Level1Text(NetMsg msg, NetMsg *msgRef)
{
    u16 id;

    LobbyNet_GetLastId(&id);
    NetList1_GetText(id, gLobbyLevel1Entries[id - 1].text);
}

/* A level-1 entry's group values (631B: the group limit, the quest target). */
void LobbyRoute_Level1GroupInfo(NetMsg msg, NetMsg *msgRef)
{
    u16 id;

    LobbyNet_GetLastId(&id);
    NetList1_GetGroupInfo(id, gLobbyLevel1Entries[id - 1].groupInfo);
}

/* A level-1 entry's name (6302). */
void LobbyRoute_Level1Name(NetMsg msg, NetMsg *msgRef)
{
    u16 id;

    LobbyNet_GetLastId(&id);
    NetList1_GetName(id, gLobbyLevel1Entries[id - 1].name);
}

/* A level-1 entry's state (6304). */
void LobbyRoute_Level1State(NetMsg msg, NetMsg *msgRef)
{
    u16 id;

    LobbyNet_GetLastId(&id);
    NetList1_GetByte(id, &gLobbyLevel1Entries[id - 1].state);
}

/* A level-1 entry's name colour (630D). */
void LobbyRoute_Level1Color(NetMsg msg, NetMsg *msgRef)
{
    u16 id;

    LobbyNet_GetLastId(&id);
    NetList1_GetColor(id, &gLobbyLevel1Entries[id - 1].color);
}

/*
 * A user entered the town (6411). The lowest user id present is the
 * town's host (LobbyState.hostId). The user goes into the first free slot
 * of gNetUsers unless it is there already, and the chat announces it
 * (LOBBY_EVENT_USER_COMING, with the id when names are hidden) out of the
 * server's first level.
 */
void LobbyRoute_Level1UserIn(NetMsg msg, NetMsg *msgRef)
{
    NetUser user;
    s32 i;
    NetUser *entry;

    memset(&user, 0, sizeof(user));
    LobbyNet_GetUserInOut(&user);
    if (gLobby->hostId[0] == 0 || 0 > memcmp(gLobby->hostId, user.id, LOBBY_USER_ID_LEN)) {
        memset(gLobby->hostId, 0, LOBBY_USER_ID_LEN);
        memcpy(gLobby->hostId, user.id, LOBBY_USER_ID_LEN);
    }
    for (i = 0, entry = gNetUsers; i < LOBBY_MEMBER_COUNT; i++, entry++) {
        if (memcmp(entry, user.id, LOBBY_USER_ID_LEN) == 0) {
            return;
        }
    }
    for (i = 0, entry = gNetUsers; i < LOBBY_MEMBER_COUNT; i++, entry++) {
        if (entry->id[0] == 0) {
            memcpy(&gNetUsers[i], &user, sizeof(NetUser));
            break;
        }
    }
    if (Lobby_GetPlace() != LOBBY_PLACE_LEVEL0 && user.name[0] != 0) {
        if (gChatHideNames != 0) {
            Chat_SystemMsg(LOBBY_EVENT_USER_COMING, user.id);
        } else {
            Chat_SystemMsg(LOBBY_EVENT_USER_COMING, user.name);
        }
    }
}

/*
 * A user left the town (6410). Its slot of gNetUsers is announced in the
 * chat (LOBBY_EVENT_USER_LEFT_TOWN) and cleared, its player put at the
 * origin, the town's action spots, ready bits and slot ids reset. A seated
 * player leaves its table (a hall's or the house's town message), the
 * seats are freed, a contest the player was in ends, and the player's
 * target is dropped if it was that user. When the user was the host, the
 * lowest id left becomes the host. The song's requester is forgotten if it
 * was that user.
 */
void LobbyRoute_Level1UserOut(NetMsg msg, NetMsg *msgRef)
{
    NetUser user;
    s32 i;
    s32 found;
    Player *self;
    NetUser *entry;
    Player *pl;
    NetUser *leaving;
    NetUser *other;
    s32 j;

    LobbyNet_GetUserInOut(&user);
    found = -1;
    self = &gPlayers[gScene.localPlayer];
    for (i = 0, entry = gNetUsers; i < LOBBY_MEMBER_COUNT; i++, entry++) {
        if (memcmp(entry, user.id, LOBBY_USER_ID_LEN) == 0) {
            if (Lobby_GetPlace() != LOBBY_PLACE_LEVEL0) {
                leaving = &gNetUsers[i];
                if (leaving->id[0] != 0) {
                    if (gChatHideNames != 0) {
                        Chat_SystemMsg(LOBBY_EVENT_USER_LEFT_TOWN, leaving->id);
                    } else {
                        Chat_SystemMsg(LOBBY_EVENT_USER_LEFT_TOWN, leaving->name);
                    }
                }
            }
            found = i;
            pl = &gPlayers[i];
            pl->pos[0] = pl->pos[1] = pl->pos[2] = 0.0f;
            pl->moveTo[0] = pl->moveTo[1] = pl->moveTo[2] = 0.0f;
            pl->rot[1] = pl->targetYaw = 0;
            ActSpot_ClearAll();
            gLobby->houseSeatBits = 0;
            gLobby->memberSlot[i] = 0;
            memset(gLobby->houseSeatIds, 0, sizeof(gLobby->houseSeatIds));
            if (TownPc_IsSeatedState(pl->stateGroup, pl->state) == 1) {
                if (Area_IsTownHall(gScene.area) == 1) {
                    TownMsg_SendSeatTaken();
                } else if (Area_IsOnlineHouse(gScene.area) == 1) {
                    TownMsg_SendHouseSeatTaken();
                }
            }
            gTownSeats[0] = gTownSeats[1] = gTownSeats[2] = 0;
            if (TownPc_IsInContest(&gPlayers[gScene.localPlayer]) == 1) {
                TownMsg_SendContestSeatTaken();
            }
            if (self->target != NULL && self->target == pl) {
                self->target = NULL;
            }
            break;
        }
    }
    if (memcmp(gLobby->hostId, user.id, LOBBY_USER_ID_LEN) == 0) {
        gLobby->hostId[0] = 0;
        for (j = 0, other = gNetUsers; j < LOBBY_MEMBER_COUNT; j++, other++) {
            if (other->id[0] != 0) {
                if (gLobby->hostId[0] == 0 || 0 > memcmp(gLobby->hostId, other->id, LOBBY_USER_ID_LEN)) {
                    memset(gLobby->hostId, 0, LOBBY_USER_ID_LEN);
                    memcpy(gLobby->hostId, other->id, LOBBY_USER_ID_LEN);
                }
            }
        }
    }
    if (found >= 0) {
        memset(&gNetUsers[found], 0, sizeof(NetUser));
    }
    if (memcmp(gSongRequesterId, user.id, LOBBY_USER_ID_LEN) == 0) {
        memset(gSongRequesterId, 0, LOBBY_USER_ID_LEN);
    }
}

/* A level-2 entry's (a quest room's) name (6402). */
void LobbyRoute_Level2Name(NetMsg msg, NetMsg *msgRef)
{
    u16 id;

    LobbyNet_GetLastId(&id);
    NetList2_GetName(id, gLobbyLevel2Entries[id - 1].name);
}

/* A level-2 entry's state (6404): a closed room (state 1) is cleared with
 * its posted quest. */
void LobbyRoute_Level2State(NetMsg msg, NetMsg *msgRef)
{
    u16 id;

    LobbyNet_GetLastId(&id);
    NetList2_GetByte(id, &gLobbyLevel2Entries[id - 1].state);
    if (gLobbyLevel2Entries[id - 1].state == 1) {
        memset(&gLobbyLevel2Entries[id - 1], 0, sizeof(LevelEntry));
        memset(&gPostedQuests[id - 1], 0, sizeof(PostedQuest));
    }
}

/* A level-2 entry's colour (650A). */
void LobbyRoute_Level2Color(NetMsg msg, NetMsg *msgRef)
{
    u16 id;

    LobbyNet_GetLastId(&id);
    Net_GetLevel2Color(id, &gLobbyLevel2Entries[id - 1].color);
}

/* A level-2 entry's user counts (6403). */
void LobbyRoute_Level2Values(NetMsg msg, NetMsg *msgRef)
{
    u16 id;
    LevelEntry *entry;

    LobbyNet_GetLastId(&id);
    entry = &gLobbyLevel2Entries[id - 1];
    NetList2_GetUsers(id, &entry->users, &entry->questUsers);
}

/* A level-2 entry's limits (640B): the most users, and four more values
 * no code reads. */
void LobbyRoute_Level2Limits(NetMsg msg, NetMsg *msgRef)
{
    u16 id;
    LevelEntry *entry;

    LobbyNet_GetLastId(&id);
    entry = &gLobbyLevel2Entries[id - 1];
    NetList2_GetLimits(id, &entry->maxUsers, &entry->unkA[0], &entry->unkA[1], &entry->unkA[2], &entry->unkA[3]);
}

/* Whether a level-2 entry has a password (6405). */
void LobbyRoute_Level2Locked(NetMsg msg, NetMsg *msgRef)
{
    u16 id;

    LobbyNet_GetLastId(&id);
    NetList2_GetLocked(id, &gLobbyLevel2Entries[id - 1].locked);
}

/* A level-2 entry's text (640D). */
void LobbyRoute_Level2Text(NetMsg msg, NetMsg *msgRef)
{
    u16 id;

    LobbyNet_GetLastId(&id);
    NetList2_GetText(id, gLobbyLevel2Entries[id - 1].text);
}

/* A level-2 entry's posted quest (650C, gPostedQuests). */
void LobbyRoute_Level2PostedQuest(NetMsg msg, NetMsg *msgRef)
{
    u16 id;

    LobbyNet_GetLastId(&id);
    Net_GetLevel2PostedQuest(id, (u8 *)&gPostedQuests[id - 1]);
}

/*
 * The quest room's member count (6412 and 6506) into
 * LobbyState.roomMembers. With a paid quest that needs more than one
 * member: reaching the quest's least number (gQuestMinMembers) is
 * announced (LOBBY_EVENT_ALL_READY, LOBBY_SE_PARTY_READY) unless the payer
 * is setting out; falling below it again is announced as the entry
 * cancelled.
 */
void LobbyRoute_Level2Members(NetMsg msg, NetMsg *msgRef)
{
    s32 id = gLobbyLevel2Sel;
    u16 unused;
    s32 wasReady;

    wasReady = 0;
    if (gLobby->roomMembers + 1 == gQuestMinMembers + 1) {
        wasReady = 1;
    }
    NetList2_GetMembers(id, &gLobby->roomMembers, &unused);
    if (gTownMode != TOWN_MODE_SET_OUT_PAID && gQuestMinMembers + 1 > 1 &&
        gLobby->roomMembers == gQuestMinMembers && gLobby->questPaid != 0) {
        Chat_SystemMsg(LOBBY_EVENT_ALL_READY, NULL);
        LobbySe_Play(LOBBY_SE_PARTY_READY);
    }
    if (wasReady == 1 && gQuestMinMembers + 1 > 1 && gQuestMinMembers + 1 > gLobby->roomMembers + 1 &&
        gLobby->questPaid != 0) {
        Chat_SystemMsg(LOBBY_EVENT_ENTRY_CANCELLED, NULL);
    }
}

/*
 * A user entered the quest room (6503): it takes the first free one of
 * the room's players (LobbyState.players) and is announced in the chat
 * (LOBBY_EVENT_MEMBER_JOINED) out of the server's first level. With a paid
 * arena cup quest, gArenaCupJoined and gArenaCupJoinValue are set (no
 * reader of them is known). The common bank's sound 148 plays.
 */
void LobbyRoute_Level2UserIn(NetMsg msg, NetMsg *msgRef)
{
    NetUser user;
    s32 i;
    u8 *slotBase;
    s32 offset;

    memset(&user, 0, sizeof(user));
    LobbyNet_GetUserInOut(&user);
    for (i = 0, slotBase = (u8 *)gLobby; i < LOBBY_ROOM_USER_COUNT; i++, slotBase += sizeof(NetUser)) {
        if (((LobbyState *)slotBase)->players[0].id[0] == 0) {
            offset = i * sizeof(NetUser);
            memset(&LOBBY_PLAYER_AT(offset), 0, sizeof(NetUser));
            memcpy(LOBBY_PLAYER_AT(offset).id, user.id, LOBBY_USER_ID_LEN);
            memcpy(LOBBY_PLAYER_AT(offset).name, user.name, LOBBY_USER_NAME_LEN);
            memcpy(LOBBY_PLAYER_AT(offset).text, user.text, sizeof(user.text));
            break;
        }
    }
    if (Lobby_GetPlace() != LOBBY_PLACE_LEVEL0 && user.name[0] != 0) {
        if (gChatHideNames != 0) {
            Chat_SystemMsg(LOBBY_EVENT_MEMBER_JOINED, user.id);
        } else {
            Chat_SystemMsg(LOBBY_EVENT_MEMBER_JOINED, user.name);
        }
    }
    if (gLobby->questPaid != 0 && gLobbyQuestId == QUEST_ID_ARENA_CUP) {
        gArenaCupJoined = 1;
        gArenaCupJoinValue = 44;
    }
    Sound_PlaySeFlat(SE_SLOT_COMMON, 148, 0);
}

/*
 * A user left the quest room (6502): until the party is complete, its
 * player is announced (LOBBY_EVENT_MEMBER_LEFT) and cleared, and the
 * players after it move up one.
 */
void LobbyRoute_Level2UserOut(NetMsg msg, NetMsg *msgRef)
{
    NetUser user;
    s32 found;
    s32 i;

    memset(&user, 0, sizeof(user));
    LobbyNet_GetUserInOut(&user);
    for (i = 0; i < LOBBY_ROOM_USER_COUNT; i++) {
        if (gLobby->playersListed == 0 && memcmp(gLobby->players[i].id, user.id, LOBBY_USER_ID_LEN) == 0) {
            if (Lobby_GetPlace() != LOBBY_PLACE_LEVEL0 && gLobby->players[i].name[0] != 0) {
                if (gChatHideNames != 0) {
                    Chat_SystemMsg(LOBBY_EVENT_MEMBER_LEFT, gLobby->players[i].id);
                } else {
                    Chat_SystemMsg(LOBBY_EVENT_MEMBER_LEFT, gLobby->players[i].name);
                }
            }
            memset(&gLobby->players[i], 0, sizeof(NetUser));
            found = i;
        }
    }
    /* found + 1 is the player each one moves from */
    for (i = found, found++; i < LOBBY_ROOM_USER_COUNT; i++, found++) {
        if (found >= LOBBY_ROOM_USER_COUNT) {
            memset(&gLobby->players[i], 0, sizeof(NetUser));
        } else {
            memcpy(&gLobby->players[i], &gLobby->players[found], sizeof(NetUser));
        }
    }
}

/* A town message (6708, 670E): passed to TownMsg_Recv with its sender's
 * id, unless the lobby is being left, the player is in the quest room or
 * the party is complete. */
void LobbyRoute_TownMsg(NetMsg msg, NetMsg *msgRef)
{
    struct {
        char id[8];
        char text[768];
    } townMsg;
    u8 phase;
    LobbyState *lobby = gLobby;

    phase = lobby->phase;
    if (phase != LOBBY_PHASE_EXIT) {
        if (phase == LOBBY_PHASE_ROOM || lobby->playersListed != 0) {
            return;
        }
        LobbyNet_GetTownMsg((struct NetTownMsg *)&townMsg);
        TownMsg_Recv(townMsg.id, (u8 *) townMsg.text);
    }
}

/* The server sends the player out of the level-0 entry (6307): taken in
 * LOBBY_PHASE_VILLAGE when no leave is pending, with its error text.
 * The first test is redundant; the game's code makes both. */
void LobbyRoute_Level0Kick(NetMsg msg, NetMsg *msgRef)
{
    s8 *leave;
    u8 phase;
    LobbyState *lobby = gLobby;

    phase = lobby->phase;
    if (phase != LOBBY_PHASE_EXIT && phase == LOBBY_PHASE_VILLAGE) {
        leave = &lobby->level0Kicked;
        if (*leave == 0) {
            *leave = LEAVE_SENT_OUT;
            LobbyNet_GetErrText(gLobby->errText);
        }
    }
}

/* Out of the level-1 entry, the town (640C): the same, in the town. */
void LobbyRoute_Level1Kick(NetMsg msg, NetMsg *msgRef)
{
    s8 *leave;
    u8 phase;
    LobbyState *lobby = gLobby;

    phase = lobby->phase;
    if (phase != LOBBY_PHASE_EXIT && phase == LOBBY_PHASE_TOWN) {
        leave = &lobby->level1Kicked;
        if (*leave == 0) {
            *leave = LEAVE_SENT_OUT;
            LobbyNet_GetErrText(gLobby->errText);
        }
    }
}

/* Out of the level-2 entry, the quest room (6505): the same, in the town
 * before the party is complete; the notices waiting are dropped. */
void LobbyRoute_Level2Kick(NetMsg msg, NetMsg *msgRef)
{
    s8 *leave;
    u8 phase;
    LobbyState *lobby = gLobby;

    phase = lobby->phase;
    if (phase != LOBBY_PHASE_EXIT && phase == LOBBY_PHASE_TOWN && lobby->playersListed == 0) {
        leave = &lobby->level2Kicked;
        if (*leave == 0) {
            *leave = LEAVE_SENT_OUT;
            Notice_ResetQueue();
            LobbyNet_GetErrText(gLobby->errText);
        }
    }
}

/* A member's new hunter card (6191): checked (HunterCard_Sanitize) and
 * copied to the member's entry of gNetUsers. */
void LobbyRoute_UserCard(NetMsg msg, NetMsg *msgRef)
{
    NetUser user;
    s8 i;
    NetUser *entry;
    u8 *card;

    LobbyNet_GetUserCard(&user);
    card = user.text;
    HunterCard_Sanitize(card);
    for (i = 0, entry = gNetUsers; i < LOBBY_MEMBER_COUNT; i++, entry++) {
        if (memcmp(entry, user.id, LOBBY_USER_ID_LEN) == 0) {
            memcpy(gNetUsers[i].text, card, sizeof(gNetUsers[i].text));
            return;
        }
    }
}

/* A part of a member's hunter card (6193): n bytes at an offset into the
 * member's card in gNetUsers, which is then checked. */
void LobbyRoute_UserCardPart(NetMsg msg, NetMsg *msgRef)
{
    NetUserCardPart part;
    u8 *card;
    s8 i;
    NetUser *entry;

    LobbyNet_GetUserCardPart(&part);
    for (i = 0, entry = gNetUsers; i < LOBBY_MEMBER_COUNT; i++, entry++) {
        if (memcmp(entry, part.id, LOBBY_USER_ID_LEN) == 0) {
            card = gNetUsers[i].text;
            memcpy(card + part.offset, part.data, part.n);
            HunterCard_Sanitize(card);
            return;
        }
    }
}

/* The colour of each chat colour digit '0'-'7' (ChatMsg_ColorDigit). */
u8 gChatDigitColors[8] = {
#include "lobby/00819700.inc"
};
