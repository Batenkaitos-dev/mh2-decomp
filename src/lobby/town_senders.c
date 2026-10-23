/*
 * The town's set-up and its senders.
 *
 * The set-up: the town's periods (their defaults and the server's), the
 * town's reset as it is entered, the areas' numbering in the position
 * messages (Area_GetSetIndex), and the chat lines the player sends.
 *
 * The senders, one function per town message (TOWN_MSG_*, lobby_net.h):
 * the player's position, state, look, facing, area and name; the house
 * Poogie's position, the kitchen's messages, the arena quest's teams and
 * opening, the town's tune and the song requests; the records another
 * member asks for; the seats asked for, given, taken and left (the house's,
 * the tables' and the contest's); the contest itself; the
 * items one player holds out to another; and the newcomer's question to the
 * others (TOWN_MSG_SYNC_ASK).
 *
 * The file ends with the three ways a town message goes out: to every
 * member (TownMsg_Send), to one member by its user id (TownMsg_SendTo), or
 * to the player itself (TownMsg_SendSelf), which hands it straight to the
 * receiver, TownMsg_Recv. Each copies the command and its data into one
 * buffer (gTownMsgCmd, then gTownMsgData).
 *
 * The file's data is gMsgAreaIds, then the equipment shop's lists
 * (gEquipShopLists: by the shop's kind, then by row).
 */
#include "common.h"
#include "lobby_state.h"
#include "m2c_macros.h"
#include "lobby_globals.h"
#include "lobby_net.h"
#include "lobby_menu.h"
#include "lobby_text.h"
#include "player.h"
#include "em.h"
#include "scene.h"
#include "save.h"
#include "quest.h"
#include "chat_log.h"
#include "lobby_labels.h"
#include "area.h"
#include "menu_task.h"
#include "engine_math.h"
#include "game_util.h"
#include "hunter_card.h"
#include "item_shop.h"
#include "lobby_queries.h"
#include "lobby_world.h"
#include "mem_util.h"
#include "my_house.h"
#include "quest_desk.h"
#include "records_lists.h"
#include "town_player_states.h"

extern s16 gMenuOffsetY;
extern s16 gMenuOffsetX;
extern M2C_UNK gClockWobble;
extern M2C_UNK gTownMode;
extern M2C_UNK *gLobbyMenu;
extern M2C_UNK gLobbyMenuData;

void *memcpy(void *dst, const void *src, u32 n);
extern u16 gTownMoveFrames;
extern u16 gRecruitFrames;
extern u16 gFriendCheckFrames;
extern u16 gAreaSearchFrames;
extern u8 gEmSizeRanges[];

/*
 * Sets the town's periods to their defaults, in frames: 20 for the
 * position messages (gTownMoveFrames, also the other players' walk time),
 * 5400 (90 s) for the friend check and the recruiting, 1800 (30 s) for the
 * area search. Lobby_ApplyServerTimers replaces them with the server's.
 */
void Lobby_ResetTimers(void)
{
    gTownMoveFrames = 20;
    gFriendCheckFrames = 5400;
    gAreaSearchFrames = 1800;
    gRecruitFrames = 5400;
}

/*
 * Takes the server's town periods from gLobbyWorkBuf, each one only when
 * the server gave it (nonzero), and the server's monster size ranges
 * (gEmSizeRanges). The last four are read through a pointer to 0x6F14,
 * which the game's code builds in two additions.
 */
void Lobby_ApplyServerTimers(void)
{
    u8 *work = gLobbyWorkBuf;
    u8 *settings = (u8 *)((s32)work + 28404) + 32;

    if (*(u16 *)(work + LOBBY_WORK_TOWN_MOVE_FRAMES) != 0) {
        gTownMoveFrames = *(u16 *)(work + LOBBY_WORK_TOWN_MOVE_FRAMES);
    }
    if (*(u16 *)(settings + (LOBBY_WORK_RECRUIT_FRAMES - 28436)) != 0) {
        gRecruitFrames = *(u16 *)(settings + (LOBBY_WORK_RECRUIT_FRAMES - 28436));
    }
    if (*(u16 *)(settings + (LOBBY_WORK_FRIEND_CHECK_FRAMES - 28436)) != 0) {
        gFriendCheckFrames = *(u16 *)(settings + (LOBBY_WORK_FRIEND_CHECK_FRAMES - 28436));
    }
    if (*(u16 *)(settings + (LOBBY_WORK_AREA_SEARCH_FRAMES - 28436)) != 0) {
        gAreaSearchFrames = *(u16 *)(settings + (LOBBY_WORK_AREA_SEARCH_FRAMES - 28436));
    }
    memcpy(gEmSizeRanges, settings + (LOBBY_WORK_EM_SIZE_RANGES - 28436), 10);
}

/*
 * Resets the town as it is entered: clears the town's state (the 3944
 * bytes from gTownMode) and the clock's wobble, points gLobby and
 * gLobbyMenu at their data, sets the menus' offsets to 0, resets the lobby
 * menu, and builds the player's hunter card when the local player is
 * active.
 */
void Town_Reset(void)
{
    u8 localPlayer;

    Mem_Fill(&gTownMode, 0, 3944);
    Mem_Fill(&gClockWobble, 0, 6);
    gMenuOffsetY = 0;
    gLobby = &gLobbyState;
    gLobbyMenu = &gLobbyMenuData;
    gMenuOffsetX = 0;
    LobbyMenu_Init();
    localPlayer = gScene.localPlayer;
    if (gPlayers[localPlayer].active != 0) {
        HunterCard_Build(&gPlayerText);
    }
}

/* The number the position messages give area `id`: its index in
 * gMsgAreaIds, the town's areas; 0 when it isn't one of them. */
u8 Area_GetSetIndex(u16 id)
{
    u16 i;

    for (i = 0; i < TOWN_MSG_AREAS; i++) {
        if (gMsgAreaIds[i] == id) {
            return i;
        }
    }
    return 0;
}

char *strcpy(void *dst, const void *src);
extern s8 gChatTargets;
extern u8 gChatClassColors[];
extern char gMyName[17];
extern u8 gMyUserId[LOBBY_USER_ID_LEN];

/*
 * Sends a chat line (online, under 64 bytes). It is said to everyone as a
 * balloon (LobbyNet_SendChatSay, CHAT_MSG_BALLOON) unless
 * LobbyState.chatToTargets is set; then it is told to each user of
 * gChatTargets and shown to the player as a ChatMsg of its own, in the
 * colour of class `color`. The message is built as bytes, at a ChatMsg's
 * offsets.
 */
void Chat_SendText(s32 unused, void *text, u8 color)
{
    u8 buf[304];
    s32 len;
    s32 i;
    u8 cls;
    s8 *target;

    if (Game_IsOnline() == 0) {
        return;
    }
    len = Lobby_Strlen(text);
    if (len < 64) {
        if (gLobby->chatToTargets == 0) {
            LobbyNet_SendChatSay((s32) text, len & 0xFFFF, 0, CHAT_MSG_BALLOON);
            return;
        }
        cls = color;
        for (i = 0, target = &gChatTargets; i < LOBBY_CHAT_TARGETS; i++, target += LOBBY_USER_ID_LEN) {
            if (*target != 0) {
                LobbyReq_ChatTell(cls, target, (s32) text, len, Chat_OnSendError);
            }
        }
        memcpy(&buf[0], gMyUserId, LOBBY_USER_ID_LEN);    /* id */
        memcpy(&buf[8], gMyName, 16);                    /* name */
        strcpy(&buf[28], text);                           /* text */
        buf[286] = 0;                                     /* color[1] */
        buf[285] = 0;                                     /* color[0] */
        buf[287] = gChatClassColors[color];               /* color[2] */
        buf[288] = CHAT_MSG_BALLOON;                      /* kind */
        Chat_Receive(buf);
    }
}

void sprintf(void *dst, void *src);

/* A chat tell's reply (LobbyReq_ChatTell): when it failed (kind -1), the
 * player gets a system line that the chat couldn't be sent, under the
 * system's name. */
void Chat_OnSendError(NetMsg reply, NetMsg *replyPtr)
{
    u8 buf[304];

    if (reply.kind == -1) {
        sprintf(&buf[28], gLobbyEventTexts[LOBBY_EVENT_CHAT_SEND_ERROR]);
        strcpy(&buf[0], gMemberCountTexts[MEMBER_COUNT_SYSTEM_NAME]);
        strcpy(&buf[8], gMemberCountTexts[MEMBER_COUNT_SYSTEM_NAME]);
        buf[287] = 6;
        buf[286] = 6;
        buf[285] = 6;
        buf[288] = CHAT_MSG_BALLOON;
        Chat_Receive(buf);
    }
}

/* Sends a line to the town's chat logs (online, under 64 bytes):
 * LobbyNet_SendChatSay with CHAT_MSG_LOG. */
void Chat_SendLogText(s32 unused, s32 text)
{
    s32 len;

    if (Game_IsOnline() != 0) {
        len = Lobby_Strlen((char *) text);
        if (len < 64) {
            LobbyNet_SendChatSay(text, len & 0xFFFF, 0, CHAT_MSG_LOG);
        }
    }
}

extern f32 gTownSentPos[3];

/*
 * True when the player's position needn't be sent: it is under 5 units
 * from the last one sent (gTownSentPos), the player is alone, the town mode
 * holds it in place (TOWN_MODE_EXIT, TOWN_MODE_AREA_MENU, TOWN_MODE_AREA_CHANGE), or it
 * is in a group-0 state that does (the drunk's slump and getting up, its
 * idle, state 74).
 */
s32 TownMsg_PositionUnchanged(s16 *pos)
{
    f32 probe[3];
    Player *pl = &gPlayers[gScene.localPlayer];

    probe[0] = pos[0];
    probe[1] = gTownSentPos[1];
    probe[2] = pos[1];
    if (!(5.0f <= Vec_Distance(probe, gTownSentPos))) {
        return 1;
    }
    if (gScene.playerCount == 1) {
        return 1;
    }
    switch (gTownMode) {
    case TOWN_MODE_EXIT:
    case TOWN_MODE_AREA_MENU:
    case 21:
        return 1;
    }
    if (pl->stateGroup == 0) {
        switch (pl->state) {
        case TOWN_PC_DRUNK_COLLAPSE:
        case 74:
        case TOWN_PC_DRUNK_GET_UP:
        case TOWN_PC_DRUNK_IDLE:
            return 1;
        }
    }
    return 0;
}

extern s8 gLobbyScreenMode;
extern s8 gTownMoveTimer;
extern s8 D_008BEB59;
extern s16 gTownSentArea;
/*
 * The player's position message, every gTownMoveFrames frames in the
 * town (LOBBY_SCREEN_TOWN): TOWN_MSG_WALK when the position changed enough
 * (TownMsg_PositionUnchanged) or the player is stopping
 * (TOWN_PC_RUN_STOP); the position and area sent are kept.
 */
void TownMsg_SendPosition(Player *unused0)
{
    TownPosMsg msg;
    Player *pl = &gPlayers[gScene.localPlayer];

    if (--gTownMoveTimer <= 0 && gLobbyScreenMode == LOBBY_SCREEN_TOWN) {
        msg.x = pl->pos[0];
        msg.z = pl->pos[2];
        msg.area = Area_GetSetIndex(pl->area);
        if ((s8)TownMsg_PositionUnchanged(&msg.x) == 0 || pl->state == TOWN_PC_RUN_STOP) {
            D_008BEB59 = 0;
            gTownMoveTimer = gTownMoveFrames;
            gTownSentPos[0] = msg.x;
            gTownSentPos[2] = msg.z;
            gTownSentArea = msg.area;
            TownMsg_Send(5, TOWN_MSG_WALK, &msg);
        }
    }
}

u8 Village_FacilityLevel(s32 k);
extern u8 gPoogieLeft;

/*
 * Tells member `member` where the house's Poogie stands (TOWN_MSG_POOGIE_POS):
 * the Poogie `poogie`'s x and z, or with none its spot in the house by the
 * house's level; (0, 0) when the Poogie has no name yet, is out on an
 * errand or has left.
 */
void TownMsg_SendPoogiePos(Em *poogie, void *member)
{
    s16 pos[4];

    if (poogie == NULL) {
        if (*(s8 *)&gSave.poogieName[0] == 0 || gPoogieErrand == 1 || gPoogieLeft == 1) {
            pos[0] = 0;
            pos[1] = 0;
        } else {
            switch (Village_FacilityLevel(VILLAGE_FACILITY_HOUSE) & 0xFF) {
            case 0:
                pos[0] = 2652;
                pos[1] = 2405;
                break;
            case 1:
                pos[0] = 2803;
                pos[1] = 2475;
                break;
            case 2:
                pos[0] = 2255;
                pos[1] = 3076;
                break;
            }
        }
    } else {
        pos[0] = poogie->posX;
        pos[1] = poogie->posZ;
    }
    TownMsg_SendTo(5, TOWN_MSG_POOGIE_POS, pos, member);
}

/* Sends a kitchen message (TOWN_MSG_KITCHEN) to member `member`, or to every
 * member with none; its length by its kind (enum KitchenMsg, the first
 * byte). */
void TownMsg_SendKitchen(u8 *msg, void *member)
{
    u8 len;

    switch (msg[0]) {
    case KITCHEN_MSG_OPEN:
    case KITCHEN_MSG_DONE:
        len = 1;
        break;
    case KITCHEN_MSG_ROUTE:
        len = 2;
        break;
    case KITCHEN_MSG_CHOSEN:
        len = 17;
        break;
    }
    if (member != NULL) {
        TownMsg_SendTo(len, TOWN_MSG_KITCHEN, msg, member);
    } else {
        TownMsg_Send(len, TOWN_MSG_KITCHEN, msg);
    }
}

extern NetUser gNetUsers[];

/*
 * An arena quest's team (28 bytes, a gSave.arenaEms record) and its
 * item: once the quest is paid, the player keeps them as the first side
 * (gNextQuest.teams[0], .teamItems[0]); before, they go to the players
 * whose card's arena role is 1, the quest's poster (TOWN_MSG_ARENA_TEAM).
 */
void TownMsg_SendArenaTeam(void *team, s32 item)
{
    u8 buf[32];
    s8 i;
    s32 off;

    memcpy(buf, team, 28);
    memcpy(buf + 28, &item, 4);
    if (gLobby->questPaid != 0) {
        memcpy(&gNextQuest.teams[0], team, 28);
        gNextQuest.teamItems[0] = item;
    } else {
        for (i = 0, off = 0; i < 4; i++, off += sizeof(NetUser)) {
            if (LOBBY_PLAYER_CARD_AT(off)->arenaRole == 1) {
                TownMsg_SendTo(32, TOWN_MSG_ARENA_TEAM, buf, LOBBY_PLAYER_AT(off).id);
            }
        }
    }
}

/*
 * The poster's last step of an arena quest's set-up: sends both teams and
 * items (TOWN_MSG_ARENA_TEAMS) to each player, noting in gNextQuest.sideIds
 * which side each one is on by its card's arena role, then the sides
 * (TOWN_MSG_ARENA_SIDES) to each player.
 */
void TownMsg_SendArenaTeams(void)
{
    u8 buf[64];
    s32 sideOff;
    s8 j;
    s8 i;
    s32 off;
    s8 k;
    LobbyState *slot; /* gLobby moved by off (or sideOff): its players[0] is player i */

    memcpy(buf, gNextQuest.teams, 56);
    memcpy(buf + 56, gNextQuest.teamItems, 8);
    for (i = 0, off = 0; i < 4; i++, off += sizeof(NetUser)) {
        slot = (LobbyState *)((u8 *)gLobby + off);
        if (slot->players[0].id[0] != 0) {
            TownMsg_SendTo(64, TOWN_MSG_ARENA_TEAMS, buf, slot->players[0].id);
            k = Lobby_FindMember((u8 *)LOBBY_PLAYER_AT(off).id);
            memcpy(gNextQuest.sideIds[((HunterCard *)gNetUsers[k].text)->arenaRole - 1], LOBBY_PLAYER_AT(off).id, 6);
        }
    }
    for (j = 0, sideOff = 0; j < 4; j++, sideOff += sizeof(NetUser)) {
        slot = (LobbyState *)((u8 *)gLobby + sideOff);
        if (slot->players[0].id[0] != 0) {
            TownMsg_SendTo(24, TOWN_MSG_ARENA_SIDES, gNextQuest.sideIds, slot->players[0].id);
        }
    }
}

/* Tells member `member` the town's tune and its position (TOWN_MSG_BGM). */
void TownMsg_SendBgmTo(s32 unused, void *member)
{
    s32 tune[2];

    tune[0] = LobbyBgm_GetTrack();
    tune[1] = LobbyBgm_GetPos();
    TownMsg_SendTo(8, TOWN_MSG_BGM, tune, member);
}

/* Starts tune `track` from its start in the town: sends it to every member
 * and to the player itself (TOWN_MSG_BGM). */
void TownMsg_SendBgm(s32 track)
{
    s32 tune[2];

    tune[0] = track;
    tune[1] = 0;
    TownMsg_Send(8, TOWN_MSG_BGM, tune);
    TownMsg_SendSelf(8, TOWN_MSG_BGM, tune);
}

extern u8 gSongRequesterId[];
void *memset(void *, s32, u32);

/* Asks the host to play song `song` (TOWN_MSG_SONG_ASK), to every member
 * and to the player itself. */
void TownMsg_SendSongAsk(s32 song)
{
    s32 data;

    data = song;
    TownMsg_Send(4, TOWN_MSG_SONG_ASK, &data);
    TownMsg_SendSelf(4, TOWN_MSG_SONG_ASK, &data);
}

/* The host's answer to a song request (TOWN_MSG_SONG_ANSWER), to every
 * member and to the player itself: `busy` (the first byte), the song and
 * the asker's id. */
void TownMsg_SendSongAnswer(void *askerId, s32 song, u8 busy)
{
    u8 buf[16];

    buf[0] = busy;
    memcpy(&buf[1], &song, 4);
    memcpy(&buf[5], askerId, LOBBY_USER_ID_LEN);
    TownMsg_Send(13, TOWN_MSG_SONG_ANSWER, buf);
    TownMsg_SendSelf(13, TOWN_MSG_SONG_ANSWER, (s32 *)buf);
}

/* Forgets who asked for the song being played (gSongRequesterId). */
void LobbyBgm_ClearState(void)
{
    memset(gSongRequesterId, 0, LOBBY_USER_ID_LEN);
}

/* Tells every member that the player went into the house of the member
 * whose user id `ownerId` is (TOWN_MSG_HOUSE_VISIT). */
void TownMsg_SendHouseVisit(void *ownerId)
{
    TownMsg_Send(LOBBY_USER_ID_LEN, TOWN_MSG_HOUSE_VISIT, ownerId);
}

/* Tells every member to put the player at its x and z at once
 * (TOWN_MSG_PLACE). */
void TownMsg_SendPlace(Player *pl)
{
    s16 pos[4];

    pos[0] = pl->pos[0];
    pos[1] = pl->pos[2];
    TownMsg_Send(4, TOWN_MSG_PLACE, pos);
}

/* TOWN_MSG_PLACE with the origin (0, 0): sent as the player enters an
 * area, before the area and the real place. */
void TownMsg_SendPlaceOrigin(void)
{
    s16 pos[4];

    pos[0] = 0;
    pos[1] = 0;
    TownMsg_Send(4, TOWN_MSG_PLACE, pos);
}

struct Player;

/* Tells every member that the player is no longer drunk (TOWN_MSG_SOBER). */
void TownMsg_SendSober(struct Player *pl)
{
    TownMsg_Send(0, TOWN_MSG_SOBER, 0);
}

void Season_Set(u8); /* unit's own declaration: the definition takes an s32; declared with a u8, as TownMsg_SendStatus passes the season, since with the definition's type the call compiles differently */
void Daytime_Set(s32 a);
void NextQuest_SetFlyer(s8);
extern s8 gQuestIsOfficial;
/* TOWN_MSG_QUEST_INFO's data (26 bytes sent). */
typedef struct TownQuestInfoMsg {
    TownEntry wanted[WANTED_ENTRIES];   /* the season's wanted list of the hall (none for an official quest) */
    u8 seasonFlags;                     /* gSeasonFlags */
    s8 rareRoll;                        /* Lobby_EnterRollRare */
} TownQuestInfoMsg;

/*
 * The quest information as the party sets out: sets the time of day from
 * the town's clock and the season from the town's, keeps them as the last
 * seen, takes the season's wanted list of the hall (AREA_TAVERN's, else
 * the elder hall's; none for an official quest) and rolls the rare chance;
 * keeps them for the quest (NextQuest_SetHuntGoals, NextQuest_SetFlyer) and
 * sends them to every member (TOWN_MSG_QUEST_INFO). The lists are read
 * through a byte pointer, at the season's offset in each.
 */
void TownMsg_SendStatus(void)
{
    TownQuestInfoMsg msg;
    u8 *lists = (u8 *)&gTownQuestLists;
    s32 season;

    if (LobbyTime_IsAm() == 0) {
        Daytime_Set(DAYTIME_SET_NIGHT);
    } else {
        Daytime_Set(DAYTIME_SET_DAY);
    }
    season = LobbyWorld_GetSeason();
    Season_Set(season);
    gLobbyLastSeason = season;
    gLobbyLastPm = LobbyTime_IsPm();
    if (gScene.area == AREA_TAVERN) {
        memcpy(msg.wanted, lists + season * sizeof(msg.wanted) + 4, sizeof(msg.wanted));     /* .a[season] */
    } else {
        memcpy(msg.wanted, lists + season * sizeof(msg.wanted) + 76, sizeof(msg.wanted));  /* .b[season] */
    }
    if (gQuestIsOfficial != 0) {
        memset(msg.wanted, 0, sizeof(msg.wanted));
    }
    msg.seasonFlags = gSeasonFlags;
    msg.rareRoll = Lobby_EnterRollRare();
    NextQuest_SetHuntGoals((u8 *)msg.wanted);
    NextQuest_SetFlyer(msg.rareRoll);
    TownMsg_Send(26, TOWN_MSG_QUEST_INFO, &msg);
}

/* True when the player is alone in the town. */
s32 Town_IsAlone(s16 *unused0)
{
    return gScene.playerCount == 1;
}

/* The player's state after it changed (TownMsg_SendState): not sent for
 * the plain states. */
void TownMsg_SendStateChange(void *unused)
{
    TownMsg_SendState(unused, 0);
}

/* The player's state, sent whatever it is (TownMsg_SendState). */
void TownMsg_SendStateNow(Player *pl)
{
    TownMsg_SendState(pl, 1);
}

extern u8 gTakeSpotId;

/* TOWN_MSG_ACTOR's data (10 bytes sent). */
typedef struct TownActMsg {
    s16 x;
    s16 z;
    u8 rot;         /* the facing's high byte */
    u8 spot;        /* the seat or action spot held (gTakeSpotId, the spot's type) */
    u8 stateGroup;
    u8 state;
    u8 drunk;       /* Player.drunk */
} TownActMsg;

void *memset(void *dst, s32 c, u32 n);
extern u8 gTownSentRot;
extern u8 gTownSentItem;
extern u8 gTownSentState0;
extern u8 gTownSentState1;
extern u8 gTownSentDrunk;

/*
 * The player's state, position, facing and drunkenness to every member
 * (TOWN_MSG_ACTOR), unless the player is alone; what is sent is kept
 * (gTownSent*). In group 0: the plain states (idle, walking, running,
 * turning, the drunk's idle) are sent only with `flag`; sitting down sends
 * where the player goes, and the seats and spot states the seat taken;
 * standing up and leaving a meal give up the action spot (in a town hall)
 * or the house seat; the drunk's slump and getting up keep the position
 * last sent.
 */
void TownMsg_SendState(void *unused, u8 flag)
{
    TownActMsg msg;
    Player *pl = &gPlayers[gScene.localPlayer];

    msg.stateGroup = pl->stateGroup;
    msg.state = pl->state;
    msg.rot = (u16)(pl->targetYaw >> 8);
    msg.drunk = pl->drunk;
    if (pl->stateGroup == 0) {
        switch (pl->state) {
        case TOWN_PC_RUN_STOP:
        case TOWN_PC_WALK:
        case 2:
        case TOWN_PC_RUN:
        case TOWN_PC_IDLE:
        case TOWN_PC_RUN_LOOP:
        case TOWN_PC_TURN_AROUND:
        case TOWN_PC_DRUNK_IDLE:
            if (flag == 0) {
                return;
            }
        default:
            msg.x = pl->pos[0];
            msg.z = pl->pos[2];
            break;
        case TOWN_PC_SIT_DOWN:
            msg.x = pl->moveTo[0];
            msg.z = pl->moveTo[2];
            msg.spot = gTakeSpotId;
            break;
        case TOWN_PC_SEAT_POSE_RIGHT:
        case TOWN_PC_SEAT_DRINK:
        case TOWN_PC_SEAT_CROSS:
        case TOWN_PC_SEAT_POSE_LEFT:
        case TOWN_PC_SIT_DOWN_SEATED:
        case TOWN_PC_SEAT_POSE_DOWN:
        case TOWN_PC_SEAT_POSE_UP:
        case TOWN_PC_SEAT_POSE_LEFT_END:
        case TOWN_PC_SEAT_POSE_UP_END:
        case TOWN_PC_SEAT_POSE_DOWN_END:
        case TOWN_PC_SEAT_POSE_RIGHT_END:
        case TOWN_PC_MEAL_SERVED:
        case TOWN_PC_KITCHEN_ORDER:
            msg.x = pl->pos[0];
            msg.z = pl->pos[2];
            msg.spot = gTakeSpotId;
            break;
        case TOWN_PC_STAND_UP:
        case TOWN_PC_MEAL_RISE:
        case TOWN_PC_MEAL_COLLAPSE:
        case TOWN_PC_MEAL_RISE_GOOD:
            msg.x = pl->pos[0];
            msg.z = pl->pos[2];
            msg.spot = *(u16 *)pl->spot;
            if (Area_IsTownHall(gScene.area) == 1) {
                ActSpot_Release(msg.spot);
            } else {
                u8 *slots = (u8 *)(pl->index + (s32)gLobby);   /* LobbyState.memberSlot, at the player's index */
                if (slots[5432] - '0' == gScene.localPlayer) {
                gLobby->houseSeatBits = gLobby->houseSeatBits & ~(1 << *(u16 *)pl->spot);
                memset(&gLobby->houseSeatIds[*(u16 *)pl->spot * 8], 0, 4);
                }
            }
            break;
        case 74:
        case TOWN_PC_DRUNK_COLLAPSE:
        case TOWN_PC_DRUNK_GET_UP:
            msg.x = gTownSentPos[0];
            msg.z = gTownSentPos[2];
            break;
        }
    } else {
        msg.x = pl->pos[0];
        msg.z = pl->pos[2];
    }
    if ((s8)Town_IsAlone(&msg.x) == 0) {
        gTownMoveTimer = gTownMoveFrames;
        gTownSentPos[0] = msg.x;
        gTownSentPos[2] = msg.z;
        gTownSentState0 = msg.stateGroup;
        gTownSentState1 = msg.state;
        gTownSentRot = msg.rot;
        gTownSentDrunk = msg.drunk;
        gTownSentItem = msg.spot;
        TownMsg_Send(10, TOWN_MSG_ACTOR, &msg);
    }
}

/* Player `pl`'s state, position and look to member `member`
 * (TOWN_MSG_ACTOR_TO, the answer to TOWN_MSG_SYNC_ASK), once it has a
 * position; with the seat it holds, or the action spot it gives up as it
 * stands. */
void TownMsg_SendActor(Player *pl, void *member)
{
    TownActMsg msg;

    if (pl->pos[0] != 0.0f || pl->pos[1] != 0.0f || pl->pos[2] != 0.0f) {
        msg.x = pl->pos[0];
        msg.z = pl->pos[2];
        msg.stateGroup = pl->stateGroup;
        msg.state = pl->state;
        msg.rot = (u16)(pl->targetYaw >> 8);
        msg.drunk = pl->drunk;
        if (TownPc_IsSeatedState(pl->stateGroup, pl->state) == 1) {
            msg.spot = gTakeSpotId;
        } else if (pl->stateGroup == 0 && pl->state == TOWN_PC_STAND_UP) {
            msg.spot = *(u16 *)pl->spot;
            ActSpot_Release(msg.spot);
        }
        TownMsg_SendTo(10, TOWN_MSG_ACTOR_TO, &msg, member);
    }
}

/* The player's facing to every member (TOWN_MSG_FACING). */
void TownMsg_SendFacing(Player *pl)
{
    TownMsg_Send(2, TOWN_MSG_FACING, &pl->rot[1]);
}

u8 Area_GetSetIndex(u16 id);

/* The area the player moved to, to every member (TOWN_MSG_AREA): its
 * number (Area_GetSetIndex) and the player's user id. */
void TownMsg_SendArea(u16 id)
{
    u8 buf[16];
    u8 *lobby = (u8 *)gLobby;
    s8 slot = lobby[gScene.localPlayer + 5432] - '0';   /* LobbyState.memberSlot */

    buf[0] = Area_GetSetIndex(id);
    memcpy(&buf[1], gNetUsers[slot].id, LOBBY_USER_ID_LEN);
    TownMsg_Send(9, TOWN_MSG_AREA, buf);
}

/* TownMsg_SendArea to member `member` alone. */
void TownMsg_SendAreaTo(void *member, u16 id)
{
    u8 buf[16];
    u8 *lobby = (u8 *)gLobby;
    s8 slot = lobby[gScene.localPlayer + 5432] - '0';

    buf[0] = Area_GetSetIndex(id);
    memcpy(&buf[1], gNetUsers[slot].id, LOBBY_USER_ID_LEN);
    TownMsg_SendTo(9, TOWN_MSG_AREA, buf, member);
}

/* The player's name to every member as it arrives (TOWN_MSG_NAME_NOTICE),
 * then its state. */
void TownMsg_SendNameNotice(void)
{
    u8 buf[32];

    memcpy(buf, gMyName, 16);
    TownMsg_Send(16, TOWN_MSG_NAME_NOTICE, buf);
    TownMsg_SendStateNow(&gPlayers[gScene.localPlayer]);
}

/* Asks for one kind of records (TownMsg_SendRecords' kinds;
 * TOWN_MSG_RECORDS_ASK) from the user in the player's member slot. */
void TownMsg_SendRecordsAsk(u8 kind)
{
    u8 data = kind;
    u8 *lobby = (u8 *)gLobby;

    TownMsg_SendTo(1, TOWN_MSG_RECORDS_ASK, &data, gNetUsers[(s8)(lobby[gScene.localPlayer + 5432] - '0')].id);
}

/*
 * Sends one kind of the player's records to member `member`: 2 the monsters'
 * largest and smallest sizes and their hunted and captured counts, 3 the
 * items seen, 4 and 5 the weapons' and the armour's tallies, 6 the titles,
 * 10 and 12-17 the equipment made (packed bits) by type; the other kinds
 * send nothing.
 */
void TownMsg_SendRecords(void *member, u8 kind)
{
    u8 big[1280];
    u8 small[48];
    s32 i;
    u8 *out;

    switch (kind) {
    case 4:
        Records_TallyWeapons((s32 *) small);
        TownMsg_SendTo(40, TOWN_MSG_RECORDS_WEAPON_TALLY, small, member);
        break;
    case 5:
        Records_TallyArmor((s32 *) small);
        TownMsg_SendTo(40, TOWN_MSG_RECORDS_ARMOR_TALLY, small, member);
        break;
    case 3:
        TownMsg_SendTo(128, TOWN_MSG_RECORDS_ITEMS, (u8 *)gSave.itemBits, member);
        break;
    case 6:
        TownMsg_SendTo(32, TOWN_MSG_RECORDS_TITLES, gSave.titles, member);
        break;
    case 2:
        out = big;
        for (i = 0; i < 74; i++) {
            *out++ = gSave.sizeMax[i];
        }
        TownMsg_SendTo(74, TOWN_MSG_RECORDS_SIZE_MAX, big, member);
        out = big;
        for (i = 0; i < 74; i++) {
            *out++ = gSave.sizeMin[i];
        }
        TownMsg_SendTo(74, TOWN_MSG_RECORDS_SIZE_MIN, big, member);
        memcpy(big, gSave.hunted, 256);
        for (i = 0; i < 74; i++) {
            ((u16 *)big)[i] += gSave.captured[i];
        }
        TownMsg_SendTo(148, TOWN_MSG_RECORDS_HUNTED, gSave.hunted, member);
        break;
    case 10:
        Bits_Pack(big, gSave.armorFlags[4], 640);
        TownMsg_SendTo(80, TOWN_MSG_RECORDS_LEGS, big, member);
        break;
    case 12:
        Bits_Pack(big, gSave.armorFlags[0], 640);
        TownMsg_SendTo(80, TOWN_MSG_RECORDS_HEAD, big, member);
        break;
    case 13:
        Bits_Pack(big, gSave.armorFlags[1], 640);
        TownMsg_SendTo(80, TOWN_MSG_RECORDS_BODY, big, member);
        break;
    case 14:
        Bits_Pack(big, gSave.armorFlags[2], 640);
        TownMsg_SendTo(80, TOWN_MSG_RECORDS_ARMS, big, member);
        break;
    case 15:
        Bits_Pack(big, gSave.armorFlags[3], 640);
        TownMsg_SendTo(80, TOWN_MSG_RECORDS_WAIST, big, member);
        break;
    case 16:
        Bits_Pack(big, gSave.bladeFlags, 640);
        TownMsg_SendTo(80, TOWN_MSG_RECORDS_BLADES, big, member);
        break;
    case 17:
        Bits_Pack(big, gSave.gunFlags, 640);
        TownMsg_SendTo(80, TOWN_MSG_RECORDS_GUNS, big, member);
        break;
    case 0:
    case 1:
    case 7:
    case 8:
    case 9:
    case 11:
        break;
    }
}

extern u8 gLobbyLevel2Sel[];

/* Tells each player of the room that the arena quest opens
 * (TOWN_MSG_ARENA_OPEN), with the level-2 choice. */
void TownMsg_SendArenaOpen(void)
{
    s8 i;
    s32 off;
    LobbyState *slot;   /* gLobby moved by off: its players[0] is player i */

    for (i = 0, off = 0; i < 4; i++, off += sizeof(NetUser)) {
        slot = (LobbyState *)((u8 *)gLobby + off);
        if (slot->players[0].id[0] != 0) {
            TownMsg_SendTo(1, TOWN_MSG_ARENA_OPEN, gLobbyLevel2Sel, slot->players[0].id);
        }
    }
}

/* Asks for the house seat of the player's action spot (its type)
 * (TOWN_MSG_HOUSE_SEAT_ASK), of the user in the player's member slot. */
void TownMsg_SendHouseSeatAsk(Player *pl)
{
    u8 *lobby = (u8 *)gLobby;
    u8 seat = *(u16 *)pl->spot;

    TownMsg_SendTo(2, TOWN_MSG_HOUSE_SEAT_ASK, &seat, gNetUsers[(s8)(lobby[gScene.localPlayer + 5432] - '0')].id);
}

/* The player left house seat gTakeSpotId (TOWN_MSG_HOUSE_SEAT_FREE), to
 * every member and to the player itself, with its user id; only while it
 * is in a house (MyHouse_LocalSlot). */
void TownMsg_SendHouseSeatFree(void *pl)
{
    u8 buf[9];
    s8 i;

    memset(buf, 0, 9);
    buf[0] = gTakeSpotId;
    i = MyHouse_LocalSlot();
    if (i != -1) {
        memcpy(&buf[1], gNetUsers[i].id, LOBBY_USER_ID_LEN);
        TownMsg_Send(9, TOWN_MSG_HOUSE_SEAT_FREE, buf);
        TownMsg_SendSelf(9, TOWN_MSG_HOUSE_SEAT_FREE, buf);
    }
}

/* The house owner's answer to a seat request (TOWN_MSG_HOUSE_SEAT_ANSWER),
 * to every member and to the player itself: yes or no, the seat and the
 * asker's id. */
void TownMsg_SendHouseSeatAnswer(char *askerId, u8 answer, u8 seat)
{
    u8 buf[16];

    memset(buf, 0, 11);
    buf[0] = answer;
    buf[1] = seat;
    strcpy(&buf[2], askerId);
    TownMsg_Send(11, TOWN_MSG_HOUSE_SEAT_ANSWER, buf);
    TownMsg_SendSelf(11, TOWN_MSG_HOUSE_SEAT_ANSWER, buf);
}

/* The player sat in house seat gTakeSpotId (TOWN_MSG_HOUSE_SEAT_TAKEN), to
 * every member and to the player itself, with its user id; only while it
 * is in a house. */
void TownMsg_SendHouseSeatTaken(void)
{
    u8 buf[9];
    s8 i;

    memset(buf, 0, 9);
    buf[0] = gTakeSpotId;
    i = MyHouse_LocalSlot();
    if (i != -1) {
        memcpy(&buf[1], gNetUsers[i].id, LOBBY_USER_ID_LEN);
        TownMsg_Send(9, TOWN_MSG_HOUSE_SEAT_TAKEN, buf);
        TownMsg_SendSelf(9, TOWN_MSG_HOUSE_SEAT_TAKEN, buf);
    }
}

extern s8 gContestSeatId;

/* Asks the host for the contest seat the player stands at
 * (TOWN_MSG_CONTEST_SEAT_ASK, to every member): the seat's id, the action
 * spot's type. */
void TownMsg_SendContestSeatAsk(Player *pl)
{
    s8 seat;

    seat = (s8)pl->spot->type;
    TownMsg_Send(2, TOWN_MSG_CONTEST_SEAT_ASK, &seat);
}

/* The player left contest seat gContestSeatId (TOWN_MSG_CONTEST_SEAT_FREE),
 * to every member and to the player itself. */
void TownMsg_SendContestSeatFree(Player *unused)
{
    TownMsg_Send(1, TOWN_MSG_CONTEST_SEAT_FREE, &gContestSeatId);
    TownMsg_SendSelf(1, TOWN_MSG_CONTEST_SEAT_FREE, &gContestSeatId);
}

/* The host's answer to a contest seat request (TOWN_MSG_CONTEST_SEAT_ANSWER),
 * to every member and to the player itself: whether it is granted, the
 * seat, and the asker's user id. */
void TownMsg_SendContestSeatAnswer(char *askerId, u8 granted, u8 seat)
{
    u8 buf[16];

    memset(buf, 0, 11);
    buf[0] = granted;
    buf[1] = seat;
    strcpy(&buf[2], askerId);
    TownMsg_Send(11, TOWN_MSG_CONTEST_SEAT_ANSWER, buf);
    TownMsg_SendSelf(11, TOWN_MSG_CONTEST_SEAT_ANSWER, (s8 *)buf);
}

/* The player sat in contest seat gContestSeatId
 * (TOWN_MSG_CONTEST_SEAT_TAKEN), to every member and to the player itself. */
void TownMsg_SendContestSeatTaken(void)
{
    TownMsg_Send(1, TOWN_MSG_CONTEST_SEAT_TAKEN, &gContestSeatId);
    TownMsg_SendSelf(1, TOWN_MSG_CONTEST_SEAT_TAKEN, &gContestSeatId);
}

/* The host opens a contest once both seats are taken
 * (TOWN_MSG_CONTEST_OPEN): each contestant gets the other's user id. */
void TownMsg_SendContestOpen(void *firstId, void *secondId)
{
    TownMsg_SendTo(8, TOWN_MSG_CONTEST_OPEN, secondId, firstId);
    TownMsg_SendTo(8, TOWN_MSG_CONTEST_OPEN, firstId, secondId);
}

/* Starts the contest at member `toId` (TOWN_MSG_CONTEST_START); the one byte
 * sent is never set. */
void TownMsg_SendContestStart(s32 *toId)
{
    s8 data;

    TownMsg_SendTo(1, TOWN_MSG_CONTEST_START, &data, toId);
}

/* The player's contest power, to the other contestant `toId`
 * (TOWN_MSG_CONTEST_POWER). */
void TownMsg_SendContestPower(void *toId, s32 power)
{
    s32 data;

    data = power;
    TownMsg_SendTo(4, TOWN_MSG_CONTEST_POWER, &data, toId);
}

/* The contest's result, to member `toId` and to the player itself
 * (TOWN_MSG_CONTEST_RESULT). */
void TownMsg_SendContestResult(s32 *toId, s8 result)
{
    s8 data;

    data = result;
    TownMsg_SendTo(1, TOWN_MSG_CONTEST_RESULT, &data, toId);
    TownMsg_SendSelf(1, TOWN_MSG_CONTEST_RESULT, &data);
}

/* Asks the host for the table seat the player stands at
 * (TOWN_MSG_SEAT_ASK, to every member): the action spot's type. */
void TownMsg_SendSeatAsk(Player *pl)
{
    s8 seat;

    seat = (s8)pl->spot->type;
    TownMsg_Send(2, TOWN_MSG_SEAT_ASK, &seat);
}

/* The player left table seat gTakeSpotId (TOWN_MSG_SEAT_FREE), to every
 * member and to the player itself. */
void TownMsg_SendSeatFree(Player *unused)
{
    TownMsg_Send(1, TOWN_MSG_SEAT_FREE, &gTakeSpotId);
    TownMsg_SendSelf(1, TOWN_MSG_SEAT_FREE, &gTakeSpotId);
}

/* The host's answer to a table seat request (TOWN_MSG_SEAT_ANSWER), to
 * every member and to the player itself: whether it is granted, the seat,
 * and the asker's user id. */
void TownMsg_SendSeatAnswer(char *askerId, u8 granted, u8 seat)
{
    u8 buf[16];

    memset(buf, 0, 11);
    buf[0] = granted;
    buf[1] = seat;
    strcpy(&buf[2], askerId);
    TownMsg_Send(11, TOWN_MSG_SEAT_ANSWER, buf);
    TownMsg_SendSelf(11, TOWN_MSG_SEAT_ANSWER, (s8 *)buf);
}

/* The player sat in table seat gTakeSpotId (TOWN_MSG_SEAT_TAKEN), to every
 * member and to the player itself. */
void TownMsg_SendSeatTaken(void)
{
    TownMsg_Send(1, TOWN_MSG_SEAT_TAKEN, &gTakeSpotId);
    TownMsg_SendSelf(1, TOWN_MSG_SEAT_TAKEN, &gTakeSpotId);
}

/* The items the player holds out (TOWN_MSG_OFFER) to every member: the
 * item, the count and the taker's user id (Player.offerItem, offerCount,
 * offerTo). */
void TownMsg_SendOfferAll(Player *pl)
{
    OfferMsg msg;

    memcpy(msg.id, gLobby->members[pl->offerTo].id, LOBBY_USER_ID_LEN);
    msg.item = pl->offerItem;
    msg.count = pl->offerCount;
    TownMsg_Send(14, TOWN_MSG_OFFER, &msg);
}

/* The same offer to member `toId` alone. */
void TownMsg_SendOffer(Player *pl, void *toId)
{
    OfferMsg msg;

    memcpy(msg.id, gLobby->members[pl->offerTo].id, LOBBY_USER_ID_LEN);
    msg.item = pl->offerItem;
    msg.count = pl->offerCount;
    TownMsg_SendTo(14, TOWN_MSG_OFFER, &msg, toId);
}

/* The taker asks the giver `toId` for the offered items
 * (TOWN_MSG_OFFER_TAKE): the item, the count and the user id, as offered. */
void TownMsg_SendOfferTake(void *toId, Player *pl)
{
    OfferMsg msg;

    memcpy(msg.id, gLobby->members[pl->offerTo].id, LOBBY_USER_ID_LEN);
    msg.item = pl->offerItem;
    msg.count = pl->offerCount;
    TownMsg_SendTo(14, TOWN_MSG_OFFER_TAKE, &msg, toId);
}

/* The giver's answer to the taker `toId` (TOWN_MSG_OFFER_RESULT): the local
 * player's offer, and whether it was refused. */
void TownMsg_SendOfferResult(void *toId, u8 refused)
{
    OfferResultMsg msg;
    Player *pl = &gPlayers[gScene.localPlayer];

    memcpy(msg.id, gLobby->members[pl->offerTo].id, LOBBY_USER_ID_LEN);
    msg.item = pl->offerItem;
    msg.count = pl->offerCount;
    msg.refused = refused;
    TownMsg_SendTo(14, TOWN_MSG_OFFER_RESULT, &msg, toId);
}

/* The player's offer is over (TOWN_MSG_OFFER_END, no data). */
void TownMsg_SendOfferEnd(void)
{
    TownMsg_Send(0, TOWN_MSG_OFFER_END, 0);
}

/* A newcomer asks every member where it stands (TOWN_MSG_SYNC_ASK): the
 * number of the player's area (Area_GetSetIndex). */
void TownMsg_SendSyncAsk(void)
{
    s8 area;

    area = Area_GetSetIndex(gScene.area);
    TownMsg_Send(2, TOWN_MSG_SYNC_ASK, &area);
}

/* The same question to member `slot` alone, if that slot holds a user
 * (its name is set). */
void TownMsg_SendSyncAskTo(u8 slot)
{
    s8 area;

    area = Area_GetSetIndex(gScene.area);
    if (gNetUsers[slot].name[0] != 0) {
        TownMsg_SendTo(2, TOWN_MSG_SYNC_ASK, &area, gNetUsers[slot].id);
    }
}

extern s8 gTownMsgCmd;
extern u8 gTownMsgData[];

/* Sends a town message to every member: the command byte (gTownMsgCmd),
 * then `len` bytes of data (gTownMsgData). It goes only online, in a town
 * or a room (Lobby_GetPlace not LOBBY_PLACE_LEVEL0), in the lobby phases
 * LOBBY_PHASE_VILLAGE, _TOWN and _ENTER, and, but for TOWN_MSG_AREA, while
 * the quest's party isn't being listed (Lobby_SendHeld).
 *
 * Two forms are kept for the match. The goto puts the sending after the
 * early return, where the game's code has it; nested in the tests, it
 * compiles in another order. The (s32) cast keeps the compiler from reusing
 * the masked length across the copy: the game's code masks it again after
 * the call. */
void TownMsg_Send(u8 len, u8 cmd, void *data)
{
    u8 phase;

    if (Game_IsOnline() != 0 && Lobby_GetPlace() != LOBBY_PLACE_LEVEL0) {
        if (cmd == TOWN_MSG_AREA || Lobby_SendHeld() != 1) {
            phase = gLobby->phase;
            if (phase == LOBBY_PHASE_VILLAGE || phase == LOBBY_PHASE_TOWN || phase == LOBBY_PHASE_ENTER) {
                goto send;
            }
        }
        return;
send:
        gTownMsgCmd = cmd;
        if ((s32)len != 0) {
            Mem_Copy(gTownMsgData, data, len);
        }
        LobbyNet_SendTownMsg((s32)&gTownMsgCmd, (u8)(len + 1));
    }
}

s32 memcmp(void *, void *, s32);

/* Sends a town message to one member, by its user id `toId`; to the player
 * itself (gMyUserId) it goes straight to TownMsg_SendSelf. */
void TownMsg_SendTo(u8 len, u8 cmd, void *data, void *toId)
{
    if (Game_IsOnline() != 0 && Lobby_SendHeld() != 1) {
        gTownMsgCmd = cmd;
        if ((s32)len != 0) {
            Mem_Copy(gTownMsgData, data, len);
        }
        if (memcmp(gMyUserId, toId, LOBBY_USER_ID_LEN) == 0) {
            TownMsg_SendSelf((u8)(len + 1), cmd, data);
            return;
        }
        LobbyReq_TownMsgTo(toId, &gTownMsgCmd, (u8)(len + 1), (void (*)(NetMsg, NetMsg *))TownMsg_OnSent); /* defined (void): it ignores the reply */
    }
}

/* TownMsg_SendTo's reply handler: nothing to do. */
void TownMsg_OnSent(void)
{
}

/* A town message to the player itself: handled at once (TownMsg_Recv) as
 * if received from the player's own user id (gMyUserId). */
void TownMsg_SendSelf(u8 len, u8 cmd, void *data)
{
    if (Game_IsOnline() != 0) {
        gTownMsgCmd = cmd;
        Mem_Copy(gTownMsgData, data, len);
        TownMsg_Recv(gMyUserId, (u8 *)&gTownMsgCmd);
    }
}

/* The file's .data: gMsgAreaIds, then the equipment shop's lists. */
u16 gMsgAreaIds[16] = {
#include "lobby/0080ACD0.inc"
};
s32 gEquipShopLists_0_0[4] = {
#include "lobby/0080ACF0.inc"
};
s32 gEquipShopLists_1_0[4] = {
#include "lobby/0080AD00.inc"
};
s32 gEquipShopLists_0_1[4] = {
#include "lobby/0080AD10.inc"
};
s32 gEquipShopLists_1_1[4] = {
#include "lobby/0080AD20.inc"
};
s32 gEquipShopLists_0_2[4] = {
#include "lobby/0080AD30.inc"
};
s32 gEquipShopLists_1_2[4] = {
#include "lobby/0080AD40.inc"
};
s32 gEquipShopLists_0_3[4] = {
#include "lobby/0080AD50.inc"
};
s32 gEquipShopLists_1_3[4] = {
#include "lobby/0080AD60.inc"
};
s32 gEquipShopLists_0_4[4] = {
#include "lobby/0080AD70.inc"
};
s32 gEquipShopLists_1_4[8] = {
#include "lobby/0080AD80.inc"
};
s32 gEquipShopLists_0_5[4] = {
#include "lobby/0080ADA0.inc"
};
s32 gEquipShopLists_1_5[8] = {
#include "lobby/0080ADB0.inc"
};
s32 gEquipShopLists_2_5[12] = {
#include "lobby/0080ADD0.inc"
};
s32 gEquipShopLists_3_5[10] = {
#include "lobby/0080AE00.inc"
};
s32 gEquipShopLists_0_6[3] = {
#include "lobby/0080AE28.inc"
};
s32 gEquipShopLists_1_6[8] = {
#include "lobby/0080AE40.inc"
};
s32 gEquipShopLists_2_6[12] = {
#include "lobby/0080AE60.inc"
};
s32 gEquipShopLists_3_6[12] = {
#include "lobby/0080AE90.inc"
};
s32 gEquipShopLists_0_7[4] = {
#include "lobby/0080AEC0.inc"
};
s32 gEquipShopLists_1_7[8] = {
#include "lobby/0080AED0.inc"
};
s32 gEquipShopLists_2_7[12] = {
#include "lobby/0080AEF0.inc"
};
s32 gEquipShopLists_3_7[12] = {
#include "lobby/0080AF20.inc"
};
s32 gEquipShopLists_0_8[4] = {
#include "lobby/0080AF50.inc"
};
s32 gEquipShopLists_1_8[12] = {
#include "lobby/0080AF60.inc"
};
s32 gEquipShopLists_2_8[16] = {
#include "lobby/0080AF90.inc"
};
s32 gEquipShopLists_3_8[14] = {
#include "lobby/0080AFD0.inc"
};
s32 gEquipShopLists_0_9[3] = {
#include "lobby/0080B008.inc"
};
s32 gEquipShopLists_1_9[12] = {
#include "lobby/0080B020.inc"
};
s32 gEquipShopLists_2_9[16] = {
#include "lobby/0080B050.inc"
};
s32 gEquipShopLists_3_9[16] = {
#include "lobby/0080B090.inc"
};
s32 *gEquipShopLists_0[12] = {
    gEquipShopLists_0_0, gEquipShopLists_0_1, gEquipShopLists_0_2, gEquipShopLists_0_3,
    gEquipShopLists_0_4, gEquipShopLists_0_5, gEquipShopLists_0_6, gEquipShopLists_0_7,
    gEquipShopLists_0_8, gEquipShopLists_0_9, NULL, NULL,
};
s32 *gEquipShopLists_1[12] = {
    gEquipShopLists_1_0, gEquipShopLists_1_1, gEquipShopLists_1_2, gEquipShopLists_1_3,
    gEquipShopLists_1_4, gEquipShopLists_1_5, gEquipShopLists_1_6, gEquipShopLists_1_7,
    gEquipShopLists_1_8, gEquipShopLists_1_9, NULL, NULL,
};
s32 *gEquipShopLists_2[12] = {
    gEquipShopLists_1_0, gEquipShopLists_1_1, gEquipShopLists_1_2, gEquipShopLists_1_3,
    gEquipShopLists_1_4, gEquipShopLists_2_5, gEquipShopLists_2_6, gEquipShopLists_2_7,
    gEquipShopLists_2_8, gEquipShopLists_2_9, NULL, NULL,
};
s32 *gEquipShopLists_3[12] = {
    gEquipShopLists_1_0, gEquipShopLists_1_1, gEquipShopLists_1_2, gEquipShopLists_1_3,
    gEquipShopLists_1_4, gEquipShopLists_3_5, gEquipShopLists_3_6, gEquipShopLists_3_7,
    gEquipShopLists_3_8, gEquipShopLists_3_9, NULL, NULL,
};
s32 **gEquipShopLists[4] = {
    gEquipShopLists_0, gEquipShopLists_1, gEquipShopLists_2, gEquipShopLists_3,
};
