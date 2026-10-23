#ifndef LOBBY_STATE_H
#define LOBBY_STATE_H

#include "common.h"
#include "menu_task.h"
#include "item.h"

/* Who may enter a player's house (gHouseAccess, HunterCard.houseAccess). */
#define HOUSE_ACCESS_CLOSED     0   /* no one */
#define HOUSE_ACCESS_OPEN       1   /* anyone */
#define HOUSE_ACCESS_PASSWORD   2   /* those who give the house's password */

/* A hunter card (216 bytes): what a player shows the others. HunterCard_Build
 * makes the local player's from the save and the player, the lobby relays it
 * as the first 216 bytes of a NetUser's 256-byte text (gPlayerText is the
 * local one), HunterCard_Sanitize clamps a received one and HunterCard_Apply
 * applies it to gPlayers[idx]. */
typedef struct HunterCard {
    /* 0x00 */ u16 hr;
    /* 0x02 */ u8 onlineMark;       /* the hunter's online mark (below), 0-8; the local hunter's is gOnlineMark */
    /* 0x03 */ u8 weaponClass;
    /* 0x04 */ s32 hairColor;       /* SaveData.hairColor, Player.hairColor */
    /* 0x08 */ u8 questStatus;      /* Player.questStatus */
    /* 0x09 */ u8 hairStyle;        /* SaveData.hairStyle, Player.hairStyle */
    /* 0x0A */ u8 gender;           /* 0 or 1: Player.gender */
    /* 0x0B */ u8 voice;            /* SaveData.voice, Player.voice (0-14) */
    /* 0x0C */ u8 symbolColor;      /* SaveData.symbolColor once a style is earned (styleBits), else SYMBOL_COLOR_NONE; Player_SetColorWord */
    /* 0x0D */ u8 charType;         /* SaveData.charType, Player.charType (0-3) */
    /* 0x0E */ u8 title;            /* SaveData.title (below HUNTER_TITLE_COUNT) */
    /* 0x0F */ u8 friendLimit;      /* SaveData.friendLimit, 0 or 1 (1: no friend requests): gFriendLimitTexts[it] (MemberCard_DrawFriendLimit) */
    /* 0x10 */ u8 comment[0x62];    /* SaveData.comment (Chat_FilterWords on a received one) */
    /* 0x72 */ s16 versusWins;      /* SaveData.versusWins */
    /* 0x74 */ EquipItem weapon;
    /* 0x80 */ EquipItem armour[6];
    /* 0xC8 */ u8 padC8;
    /* 0xC9 */ u8 houseLevel;       /* Village_FacilityLevel(VILLAGE_FACILITY_HOUSE), 0-2: the house's area is AREA_ONLINE_HOUSE_0 + it */
    /* 0xCA */ u8 poogieOutfit;     /* SaveData.poogieOutfit when the Poogie has a name (poogieName), else POOGIE_OUTFIT_NONE (at most 4):
                                       the town loads POOGIE_MODEL_OUTFIT_0 + it (Town_EnterArea) */
    /* 0xCB */ u8 houseAccess;      /* gHouseAccess: HOUSE_ACCESS_* */
    /* 0xCC */ u8 housePass[9];     /* gHousePassword: the house's password */
    /* 0xD5 */ u8 boxLevel;         /* Village_FacilityLevel(5): the box's pages - 1 (at most 4) */
    /* 0xD6 */ u8 arenaRole;        /* in an arena quest (at most 4): 1 posted it (arena_cup.c), 2-4 QuestCounterMenu.kind;
                                       Quest_DrawArenaTeams groups the members by it, 1 and 3 open ArenaMenu */
    /* 0xD7 */ u8 chatToLobby;      /* gChatToLobby: 1 while the hunter's chat goes to the lobby's chat (gChatWindowSel CHAT_SEND_LOBBY) in a town entry; the members' names window blinks a sprite for it */
} HunterCard;
/* The parts of the card Town_UpdateTextParts sends alone when they change
 * (HunterCard_SendPart): their offsets in the card. */
#define CARD_PART_SYMBOL_COLOR 12   /* HunterCard.symbolColor */
#define CARD_PART_TITLE 14          /* HunterCard.title */
#define CARD_PART_FRIEND_LIMIT 15   /* HunterCard.friendLimit */
#define CARD_PART_CHAT_TO_LOBBY 215 /* HunterCard.chatToLobby */
/* HunterCard.symbolColor's value drawn cycling through the colours (Color_Cycle8)
 * instead of one of gPlayerColorWords (the name tags, the member list). */
#define SYMBOL_COLOR_CYCLE 0xB
#define SYMBOL_COLOR_NONE 0xFF      /* HunterCard.symbolColor: no style earned */
typedef char HunterCard_size_check[(sizeof(HunterCard) == 0xD8) ? 1 : -1];

/* HunterCard.onlineMark: the hunter's online mark, one of ONLINE_MARK_COUNT
 * statuses the others see beside the name and on the card, each with its icon
 * (ICON_ONLINE_MARK + it) and its line of the online mark window (OnlineMark_*;
 * the local hunter's is gOnlineMark). */
#define ONLINE_MARK_NONE  0
#define ONLINE_MARK_COUNT 9
#define ONLINE_MARK_HELP_LINE 5 /* HELP_PAGE_STATUS's line of mark 0; one line per mark */

/* a user's text as the lobby relays it (256 bytes): a HunterCard and 40
 * unused bytes. NetUser.text holds one as bytes (NetUser is byte-aligned in
 * LobbyState); gPlayerText is the local player's. */
typedef union UserText {
    HunterCard card;
    u8 raw[0x100];
} UserText;
typedef char UserText_size_check[(sizeof(UserText) == 0x100) ? 1 : -1];

/* a user: id (6 characters used), name and a 256-byte text (the player's
 * own at gLobbyNet.me, set by LobbyNet_SetMe): a HunterCard in its first
 * 216 bytes */
#define NET_USER_ID_MATCH 6  /* the bytes of NetUser.id the lobby compares to tell members apart */
#define LOBBY_USER_ID_LEN 8     /* a user id's bytes (gMyUserId, gAskUserId, a friend's or a blocked user's id, a row of gScene.userIds, the quest sync's k2.userId) */
#define LOBBY_USER_NAME_LEN 16  /* a user name's bytes as the friend and block lists keep it; one more with its terminator */
/* Player.questStatus's flags above the quest's category (QuestFlags_GetCategory) */
#define QUEST_STATUS_SET_OUT 0x20   /* the player has set out (Town_UpdateMode) */
#define QUEST_STATUS_TAKEN 0x40     /* the quest is taken, not paid (QuestDesk_Accept) */
#define QUEST_STATUS_PAID 0x80      /* the quest is taken and paid */
#define QUEST_STATUS_CATEGORY 0x0F  /* the low four bits: the quest's category (QuestFlags_GetCategory, 0-4) */
/* Where a member is (NetUser_State, NetUser_StateByName). */
#define NET_USER_HERE    0     /* its player has an area (and always offline) */
#define NET_USER_NO_AREA 1     /* its player has no area yet */
#define NET_USER_ABSENT  2     /* no member in the slot, or none with the id */
#define LOBBY_MEMBER_COUNT 10 /* the town's members: LobbyState.members, gNetUsers, a level-1 entry's users */
#define LOBBY_ROOM_PLAYERS 4  /* the quest room's players: LobbyState.players */
#define LOBBY_TOWN_PLAYERS 10 /* the town's players: LobbyState.modelLoading and lookPending (FileQueue_Cancel) */
#define LOBBY_ROOM_USER_COUNT 4 /* the users of a level-2 entry (a quest room) the lobby copies */
/* The town member whose NetUser lies `off` bytes into LobbyState.members (off
 * is the slot times sizeof(NetUser)), and its hunter card, reached by that
 * byte offset from gLobby. The member list reads its members this way, as
 * the game's code does: indexing LobbyState.members by the slot compiles
 * differently there (its loops step the offset beside the slot, and the
 * friend question forms the id's and the name's addresses from one base,
 * gLobby moved on by the slot's NetUser). */
#define LOBBY_MEMBER_AT(off) (((LobbyState *)((u8 *)gLobby + (off)))->members[0])
#define LOBBY_MEMBER_CARD_AT(off) ((HunterCard *)LOBBY_MEMBER_AT(off).text)

/* the patch announced at login (6121), as LobbyNet_GetPatchInfo copies it out */
typedef struct NetPatchInfo {
    /* 0x00 */ u32 size;
    /* 0x04 */ char version[0x10];  /* 10 characters */
    /* 0x14 */ char tag[8];         /* 4 characters */
} NetPatchInfo;

typedef struct NetUser {
    /* 0x000 */ char id[8];
    /* 0x008 */ char name[0x14];
    /* 0x01C */ u8 text[0x100];
} NetUser;

/*
 * The lobby's state (gLobbyState, 14752 bytes): the screens' modes and steps,
 * the players, the error text of the last failed request. gLobby always
 * points to it (Town_Reset, Lobby_ResetState and NetSetup_ResetLobby set it), and
 * the code reaches it through that pointer; the village's code reads it too.
 * Most fields are still named by offset.
 */
/* The town data (6884) fills the four slots of gTownQuestLists's buffer:
 * slot 0 starts with the town event, slot 1 holds the monster rate values
 * (gLobbyRateValues), slot 3 the elder dragon raid (gElderRaidEvent). */
#define TOWN_DATA_SLOT_SIZE 0x100
#define LOBBY_RATE_COUNT 0x4A       /* the monster rate values: gLobbyRateValues (u16), gRateSnapshot (s32) */

/* A timed event of the lobby world (24 bytes; LobbyWorld.events):
 * LobbyWorld_RunEvents calls func once the world's time reaches the
 * event's; LobbyWorld_ScheduleEvents takes a list of them. */
typedef struct LobbyWorldEvent {
    /* 0x00 */ s32 active;
    /* 0x04 */ s32 year;
    /* 0x08 */ s32 season;
    /* 0x0C */ s32 day;
    /* 0x10 */ u32 tick;            /* added after the days are scaled */
    /* 0x14 */ void (*func)(void);
} LobbyWorldEvent;

/* The elder dragon raid the server announces (gElderRaidEvent: slot 3 of the
 * server's 6884 data, which Lobby_ApplyTownData turns into two world events). */
typedef struct ElderRaidEvent {
    /* 0x00 */ struct {
        s32 year;
        s32 season;
        s32 day;
        u32 tick;
    } when[2];                      /* [0] calls ElderRaid_SetState, [1] ElderRaid_CheckChanged */
    /* 0x20 */ s16 id;              /* the raid: its town movies play once per id (SaveData.raidStartMovieId, raidEndMovieId) */
    /* 0x22 */ s16 quests[3];       /* the official desk's raid quests */
    /* 0x28 */ s8 noMovie;          /* 1: ElderRaid_SetState sets elderRaidState 2, and LobbyTown_WantsMovie plays nothing */
    /* 0x29 */ s8 talkArg2;         /* the two values the forecast's raid line fills in (ElderForecast_SayRaid) */
    /* 0x2A */ s8 talkArg1;
} ElderRaidEvent;

/* LobbyState.movieState: the elder dragon raid's town movie to play */
#define RAID_MOVIE_NONE 0
#define RAID_MOVIE_START 1          /* the raid has begun: LobbyMovie_Prepare(0) */
#define RAID_MOVIE_END 2            /* it is over: LobbyMovie_Prepare(1) */

/* LobbyState.phase: the lobby's screens, gLobbyScreens' order (Lobby_Tick runs one a frame). */
enum LobbyPhase {
    LOBBY_PHASE_ENTRY = 0,          /* LobbyEntry_Run */
    LOBBY_PHASE_AREA_LIST = 1,      /* AreaList_Run: the server's areas (level 0) */
    LOBBY_PHASE_VILLAGE = 2,        /* LobbyVillage_Run: an area entered, the player alone in the village */
    LOBBY_PHASE_TOWN = 3,           /* LobbyTown_Run */
    LOBBY_PHASE_ROOM = 4,           /* LobbyRoom_Run */
    LOBBY_PHASE_EXIT = 5,           /* LobbyExit_Run: leaving the lobby server (Lobby_Disconnect) */
    LOBBY_PHASE_ROOM_CLOSE = 6,     /* LobbyRoom_RunClose */
    LOBBY_PHASE_ENTER = 7           /* LobbyEnter_Run */
};

/* LobbyState.mode in LOBBY_PHASE_ENTRY: the login's steps
 * (gLobbyEntrySteps). The login task's progress (LobbyLogin_OnProgress)
 * picks the next one from what the server answered. */
enum LobbyEntryMode {
    LOBBY_ENTRY_START = 0,          /* the client's details, then the login task */
    LOBBY_ENTRY_TERMS = 1,          /* the terms of use's page (LobbyWeb_Run) */
    LOBBY_ENTRY_NOTICE = 2,         /* the server's notice before the login goes on */
    LOBBY_ENTRY_PATCH = 3,          /* a patch is downloaded (LobbyPatch_Install) */
    LOBBY_ENTRY_ID_SELECT = 4,      /* the character's choice (IdMenu_Run) */
    LOBBY_ENTRY_ERROR = 5,          /* the server's error text, then the choice again */
    LOBBY_ENTRY_SUMMARY = 6,        /* the login summary and the hunter card */
    LOBBY_ENTRY_INFO = 7,           /* the server's information text */
    LOBBY_ENTRY_FETCH = 8,          /* the lobby's data is fetched, then the lobby is entered */
    LOBBY_ENTRY_WEB_PAGE = 9,       /* a web page the server names */
    LOBBY_ENTRY_MODES = 10
};

/* LobbyEnter_Run's steps (LobbyState.phaseStep in LOBBY_PHASE_ENTER). */
enum LobbyEnterStep {
    LOBBY_ENTER_STEP_START = 0,
    LOBBY_ENTER_STEP_PLACE = 1,         /* LobbyStep_CurrentPlace */
    LOBBY_ENTER_STEP_AREAS = 2,         /* LobbyStep_Level0List */
    LOBBY_ENTER_STEP_LEVEL1_LIST = 3,   /* LobbyStep_Level1List */
    LOBBY_ENTER_STEP_LEVEL1_USERS = 4,  /* LobbyStep_Level1Users */
    LOBBY_ENTER_STEP_QUESTS = 5,        /* LobbyStep_DownloadQuests */
    LOBBY_ENTER_STEP_TOWN = 6           /* LobbyScreen_EnterSeq(LOBBY_ENTER_TOWN_MEMBERS) */
};

/*
 * gLobbyScreenMode: the lobby screen's mode, the index of its three tables
 * of steps (gLobbySceneSteps, gLobbyScreenModes, gLobbyScreenSteps). The
 * town runs in LOBBY_SCREEN_TOWN; moving to another area goes through
 * LEAVE_AREA and ENTER_AREA back to TOWN, and an area's camera tour
 * through TOUR.
 */
enum LobbyScreenMode {
    LOBBY_SCREEN_START = 0,         /* LobbyScreen_Start: the first fade, then offline the village's entry */
    LOBBY_SCREEN_ENTER_AREA = 1,    /* LobbyScreen_EnterArea */
    LOBBY_SCREEN_TOWN = 2,          /* Town_Frame: the player walks the town */
    LOBBY_SCREEN_LEAVE_AREA = 3,    /* LobbyScreen_LeaveArea */
    LOBBY_SCREEN_TOUR = 4           /* LobbyScreen_RunTour: a camera tour (LobbyArea_StartEvent) */
};

/*
 * How the lobby screen is entered: LobbyScreen_EnterSeq's and
 * LobbyScreen_FadeOut's mode. Modes 1-3 are LobbyState.enterSeq, set by
 * LobbyVillage_Begin, _BeginStay and _BeginReturn; the town's are chosen by
 * LobbyState.groupJoin.
 */
enum LobbyEnterMode {
    LOBBY_ENTER_VILLAGE_OFFLINE = 0, /* offline (LobbyScreen_Start): the player alone, at AREA_HUB_0x89, or AREA_HUB_0x57 while away from the village */
    LOBBY_ENTER_VILLAGE = 1,        /* the village from the network's lists: the player alone, at AREA_ONLINE_VILLAGE */
    LOBBY_ENTER_VILLAGE_STAY = 2,   /* nothing reloaded: the player placed again in its area */
    LOBBY_ENTER_VILLAGE_RETURN = 3, /* back from the town: at a set spot of AREA_ONLINE_VILLAGE; the fade out clears the other players */
    LOBBY_ENTER_TOWN = 4,           /* the town, the screen kept: AREA_HUB_0x84 */
    LOBBY_ENTER_TOWN_MEMBERS = 5,   /* the town with its members (after a quest, the town's movie, a group's join): AREA_HUB_0x84 for a group, else the area left for the quest (gQuestReturnArea) */
    LOBBY_ENTER_MEMBERS = 6         /* the members spawned, no area set (no caller seen) */
};

/* LobbyScreen_EnterSeq's result besides LOBBY_STEP_DONE and _BUSY: done,
 * and an area's one-time scene or tour has started (LOBBY_SCREEN_TOUR next). */
#define LOBBY_ENTER_TOURING 3

/* gLobbyStartPosIdx: where the player stands once an area is entered or a
 * tour ends; 1 to 5 are gLobbyStartPos's entries, set by the tours. */
#define LOBBY_START_POS_HERE 0      /* where the player is */
#define LOBBY_START_POS_AREA (-1)   /* the area's own start (LobbyArea_StartEvent places the player) */
#define LOBBY_START_POS_TAVERN 1    /* after the tavern's tours */
#define LOBBY_START_POS_SMITHY 2    /* after the smithy's */
#define LOBBY_START_POS_PORT 3      /* after the port's */
#define LOBBY_START_POS_VILLAGE 4   /* after TOWN_TOUR_VILLAGE_LEVEL_2 */
#define LOBBY_START_POS_RETURN 5    /* after TOWN_TOUR_VILLAGE_RETURN */
#define LOBBY_START_POS_COUNT 6

/* gLobbyStartPos: the start positions (entry 0 is filled with the
 * player's position before a tour). */
typedef struct LobbyStartPos {
    Vec3f pos[LOBBY_START_POS_COUNT];
} LobbyStartPos;

/* An area's entry point (gSpawnPoints and the town's own): where the
 * player starts, and the facing. AreaMenu_GoTo and Town_PlaceLocalPlayer
 * read them; the village's own entries get a quarter turn added to the
 * facing. 16 bytes. */
typedef struct SpawnPt {
    Vec3f pos;
    u16 angle;
    u16 pad;
} SpawnPt;

/* Lobby_Disconnect's reasons (LobbyState.quitReason, and LobbyState.mode in
 * LOBBY_PHASE_EXIT). Reasons 0-3 are the player's own leaving, which
 * LobbyExit_RunPlanned runs, each going on to its next screen; 4-8 end the
 * session, from Lobby_Tick and the request steps' callers (LOBBY_QUIT_FAILED),
 * and LobbyExit_RunCut shows the reason before going back to the network
 * menu. */
#define LOBBY_QUIT_RECONNECT 0      /* to connect again through the network scene (NetScene_ToPortal): a group joined on its server, the move menu, the first login */
#define LOBBY_QUIT_TO_GMSUB 1       /* the quit menu's first choice, the entry's cancel: the network scene ends with the boot set to BOOT_MODE_GMSUB */
#define LOBBY_QUIT_TO_PORTAL 2      /* the quit menu's third choice, the end of the web pages: back to the portal (PORTAL_PAGE_QUIT) */
#define LOBBY_QUIT_TO_GMSUB_KEEP 3  /* the quit menu's second choice: as LOBBY_QUIT_TO_GMSUB, with BOOT_MODE_GMSUB_KEEP */
#define LOBBY_QUIT_IDLE 4           /* no input for too long (LobbyState.idleTimer) */
#define LOBBY_QUIT_FAILED 5         /* a request step failed (its result neither done nor busy): LobbyExit_ShowReason shows the same text as for LOBBY_QUIT_LINK_LOST */
#define LOBBY_QUIT_SERVER_CLOSE 6   /* the server closed the session (6003) */
#define LOBBY_QUIT_LINK_LOST 7      /* the connection failed, or no message for too long */
#define LOBBY_QUIT_SENT_OUT 8       /* the server sent the player out (6007): that server is marked tried */

#define LOBBY_SEQ_FAILED 99         /* LobbyState.seqStep, step, subStep or taskStep: the sequence's request failed */

/* LobbyState.linkState. */
#define LINK_STATE_UP 0         /* the connection holds (the state's reset value) */
#define LINK_STATE_DOWN 1       /* the lobby server's connection is lost (Lobby_MarkDisconnected, with connected 0) */
#define LINK_STATE_WEB_BACK 2   /* back in the lobby from the web pages (LobbyWeb_Run) */

/*
 * The request steps' results (LobbyStep_* and the like): the caller runs a
 * step once a frame until it returns DONE or FAILED.
 */
#define LOBBY_STEP_DONE 0
#define LOBBY_STEP_FAILED 1
#define LOBBY_STEP_BUSY 2

/* LobbyState.roomCloseKind: how the room (LOBBY_PHASE_ROOM) closed; read
 * by Lobby_HandleLeaveRequest */
#define ROOM_CLOSE_WITH_TEXT 1      /* the party fetch failed, or the server's route 5 message: the error text is shown */
#define ROOM_CLOSE_QUIET 2          /* closed with the error text kept but not shown */

/* LobbyState.level0Kicked, level1Kicked and level2Kicked: a leave of that
 * level is pending (0: none); Lobby_HandleLeaveRequest acts on it. */
#define LEAVE_SENT_OUT 1            /* the server sent the player out (its kick route) */
#define LEAVE_BEGUN 2               /* the player's own leave began (LobbyVillage_BeginLeave, LobbyTown_BeginLeave) */

/* LobbyState.serverClose: the server ended the session (0: it didn't);
 * Lobby_Tick then quits the lobby. */
#define SERVER_CLOSE_CLOSED 1       /* the server closed the session (6003): LOBBY_QUIT_SERVER_CLOSE */
#define SERVER_CLOSE_SENT_OUT 2     /* the server sent the player out of the lobby (6007): LOBBY_QUIT_SENT_OUT */

/* LobbyState.mode in LOBBY_PHASE_TOWN (LobbyTown_Run). */
#define LOBBY_TOWN_ENTER 0          /* the town is entered, then runs (LobbyTown_Enter) */
#define LOBBY_TOWN_LEAVE 1          /* the town is left for the village (LobbyTown_Leave) */

/* LobbyState.step in LOBBY_PHASE_TOWN's mode LOBBY_TOWN_ENTER (LobbyTown_Enter). */
enum LobbyTownStep {
    LOBBY_TOWN_STEP_PLACE = 0,      /* the place becomes the town */
    LOBBY_TOWN_STEP_FADE_OUT = 1,   /* the menus fade out (LobbyScreen_FadeOut) */
    LOBBY_TOWN_STEP_ENTER = 2,      /* the town's screen comes in (LobbyScreen_EnterSeq) */
    LOBBY_TOWN_STEP_RUN = 3,        /* the town runs (LobbyScene_Run) */
    LOBBY_TOWN_STEP_MOVIE = 4,      /* a raid's movie plays (LobbyTown_StartMovie) */
    LOBBY_TOWN_STEP_REENTER = 5     /* back into the town after it */
};

/* LobbyState.mode in LOBBY_PHASE_VILLAGE (LobbyVillage_Run). */
enum LobbyVillageMode {
    LOBBY_VILLAGE_START = 0,        /* the fade begins */
    LOBBY_VILLAGE_FETCH_TOWNS = 1,  /* the area's towns and quests are fetched (LobbyVillage_FetchTowns) */
    LOBBY_VILLAGE_ENTER = 2,        /* the village's screen comes in (LobbyScreen_EnterSeq) */
    LOBBY_VILLAGE_RUN = 3,          /* the village runs (LobbyScene_Run) */
    LOBBY_VILLAGE_LEAVE_COUNTDOWN = 4, /* LobbyLevel0_LeaveCountdown */
    LOBBY_VILLAGE_JOIN_TOWN = 5,    /* the town of the group joined is entered (LobbyLevel1_Enter) */
    LOBBY_VILLAGE_MOVIE = 6         /* a raid's movie plays (LobbyTown_StartMovie) */
};

/* A player of the quest room (LobbyState.players), reached by its byte
 * offset (the slot times sizeof(NetUser)) from gLobby, as the room's
 * routes do: indexing players by the slot compiles differently there (the
 * loops step the offset beside the slot). */
#define LOBBY_PLAYER_AT(off) (((LobbyState *)((u8 *)gLobby + (off)))->players[0])
#define LOBBY_PLAYER_CARD_AT(off) ((HunterCard *)LOBBY_PLAYER_AT(off).text)

/* LobbyState.place: the level of the lobby server the player is in. */
enum LobbyPlace {
    LOBBY_PLACE_LEVEL0 = 0,         /* the server's first level: after leaving a town */
    LOBBY_PLACE_LEVEL1 = 1,         /* a town (level 1): set once its entry is entered */
    LOBBY_PLACE_LEVEL2 = 2,         /* a quest room (level 2) */
    LOBBY_PLACE_NONE = 9            /* no level: the lobby's own screens set it when they start */
};

/*
 * LobbyState.awaitReply: the tag of the request whose reply is awaited.
 * A request step sets its tag when it sends the request, and its reply
 * handler acts only while the tag is still its own, then clears it to
 * LOBBY_AWAIT_NONE. The tags belong to the steps, not to the commands:
 * LOBBY_AWAIT_USERS serves several user requests, and two steps that start
 * the level-2 records task use two tags. Named by the step that sets each.
 */
enum LobbyAwait {
    LOBBY_AWAIT_NONE = 0,
    LOBBY_AWAIT_CHARA_SELECT_TEXT = 0x01, /* LobbyStep_CharaSelectText */
    LOBBY_AWAIT_SERVER_NAME = 0x02,       /* LobbyStep_ServerName */
    LOBBY_AWAIT_CURRENT_PLACE = 0x03,     /* LobbyStep_CurrentPlace */
    LOBBY_AWAIT_HUNTER_CARD = 0x04,       /* LobbyStep_HunterCard */
    LOBBY_AWAIT_LOGIN_INFO = 0x05,        /* LobbyStep_LoginInfo */
    LOBBY_AWAIT_CHAT_COLOR_MODE = 0x06,   /* LobbyStep_ChatColorMode */
    LOBBY_AWAIT_TOWN_DATA = 0x07,         /* LobbyStep_TownData */
    LOBBY_AWAIT_TOWN_CLOCK = 0x08,        /* LobbyStep_TownClock */
    LOBBY_AWAIT_QUEST_RECORD = 0x09,      /* LobbyStep_QuestRecord */
    LOBBY_AWAIT_SUMMARY_BEGIN = 0x0A,     /* LobbyLogin_SendSummary: the save summary's start */
    LOBBY_AWAIT_SUMMARY_PART = 0x0B,      /* LobbyLogin_SendSummary: its parts */
    LOBBY_AWAIT_LEVEL0_LIST = 0x0C,       /* LobbyStep_Level0List */
    LOBBY_AWAIT_ENTER_LEVELS = 0x0E,      /* LobbyLogin_EnterEntry */
    LOBBY_AWAIT_LEVEL1_ENTER = 0x0F,      /* LobbyStep_Level1Enter */
    LOBBY_AWAIT_LEVEL0_LEAVE = 0x10,      /* LobbyLevel0_Leave, LobbyLevel0_LeaveCountdown */
    LOBBY_AWAIT_USERS = 0x11,             /* a level-1 entry's users; a user's profile, card, status or search */
    LOBBY_AWAIT_LEVEL1_OPEN = 0x14,       /* LobbyStep_Level1Open */
    LOBBY_AWAIT_LEVEL1_SETTINGS = 0x16,   /* LobbyStep_Level1Settings */
    LOBBY_AWAIT_LEVEL1_SET_COLOR = 0x17,  /* LobbyStep_Level1SetColor */
    LOBBY_AWAIT_LEVEL1_SEARCH = 0x18,     /* LobbyLevel1_Search */
    LOBBY_AWAIT_LEVEL1_SEARCH_INFO = 0x19, /* LobbyLevel1_FetchInfo */
    LOBBY_AWAIT_USER_FIND_BY_NAME = 0x1C, /* LobbyUser_FindByName */
    LOBBY_AWAIT_LEVEL1_LIST = 0x1D,       /* LobbyStep_Level1List: the list task */
    LOBBY_AWAIT_LEVEL1_ALLOWED = 0x1E,    /* LobbyStep_Level1List: the allowed task */
    LOBBY_AWAIT_DL_LIST = 0x1F,           /* LobbyStep_DownloadQuests: the list task */
    LOBBY_AWAIT_DL_FILE = 0x20,           /* LobbyStep_DownloadQuests, LobbyStep_Download: a file */
    LOBBY_AWAIT_LEVEL2_LIST = 0x22,       /* LobbyLevel2_FetchList */
    LOBBY_AWAIT_LEVEL2_OPEN = 0x23,       /* LobbyLevel2_EnterFree */
    LOBBY_AWAIT_LEVEL2_RECS = 0x24,       /* LobbyLevel2_FetchRecords */
    LOBBY_AWAIT_LEVEL2_SETTINGS = 0x25,   /* LobbyLevel2_SendSettings */
    LOBBY_AWAIT_QUEST_POST = 0x26,        /* LobbyLevel2_SendQuestPost */
    LOBBY_AWAIT_LEVEL2_ENTRY_RECS = 0x27, /* LobbyStep_Level2Entry: the records task */
    LOBBY_AWAIT_LEVEL2_ENTRY_USERS = 0x28, /* LobbyStep_Level2Entry: the users */
    LOBBY_AWAIT_LEVEL2_ENTER = 0x29,      /* LobbyLevel2_EnterWithPass */
    LOBBY_AWAIT_LEVEL1_LEAVE = 0x2A,      /* LobbyStep_Level1Leave */
    LOBBY_AWAIT_LEVEL2_LEAVE = 0x2C,      /* LobbyStep_Level2Leave */
    LOBBY_AWAIT_QUEST_ACCEPT = 0x2D,      /* LobbyLevel2_SendQuestAccept */
    LOBBY_AWAIT_SET_OUT = 0x2F,           /* LobbyStep_SetOut */
    LOBBY_AWAIT_SET_OUT_LEAVE = 0x30,     /* LobbyStep_SetOutLeave */
    LOBBY_AWAIT_PARTY = 0x31,             /* LobbyRoom_FetchParty */
    LOBBY_AWAIT_ROOM_LOBBY_LEAVE = 0x32,  /* LobbyRoom_SendLobbyLeave */
    LOBBY_AWAIT_SHORT_MAIL = 0x34,        /* LobbyStep_ShortMailSend */
    LOBBY_AWAIT_LOBBY_LEAVE = 0x35        /* LobbyLeave_Run */
};
#define LOBBY_RECV_TIMEOUT 3600     /* frames: a minute without a message (LobbyState.recvTimer) */
#define LOBBY_MENU_TIMEOUT 3600     /* frames: a minute (LobbyState.timer, LobbyMenu_StartTimer) */
#define LOBBY_IDLE_TIMEOUT 108000   /* frames: an hour without input (LobbyState.idleTimer) */
#define LOBBY_CLOCK_NOON 43200      /* gLobbyTime, the lobby's clock in seconds of the day: noon */
#define LOBBY_DAY_SECONDS 86400    /* the lobby world's day in seconds (gLobbyWorld's clock; gTownDayTicks when unset) */
#define LOBBY_HOUR_SECONDS 3600

/* LobbyState.elderRaidState (ElderRaid_SetState): the elder raid. */
#define ELDER_RAID_OFF 0
#define ELDER_RAID_ON 1             /* on, with its movie */
#define ELDER_RAID_ON_NO_MOVIE 2    /* on, without its movie (ElderRaidEvent.noMovie) */
/* LobbyState.townEvent, the town's event from the server's town data: 1 to
 * 3 change the item shop's stock; the sale halves the shops' prices online. */
#define TOWN_EVENT_SALE 4
#define LOBBY_CHAT_TARGETS 7        /* gChatTargets: the user ids (LOBBY_USER_ID_LEN each) a chat line is told to */

/* The group join's fields at LobbyState's start (groupJoin to land),
 * cleared together to drop a join. */
#define LOBBY_GROUP_JOIN_SIZE 0x1C

/* gNetEntryMode: how the network scene came to the lobby's login. */
#define LOGIN_ENTRY_FIRST 0         /* the first entry: the character is chosen, then the lobby is left to connect again */
#define LOGIN_ENTRY_SERVER 1        /* that second connection, to the server chosen (the server list screen) */
#define LOGIN_ENTRY_AGAIN 2         /* the lobby was entered before (LobbyEntry_Fetch sets it) */
#define LOGIN_ENTRY_QUEST_RETURN 3  /* back after a quest (the quest room's leave): the save menu runs first, the player goes back to the area left */

/* gLoginState in the login's connection (LobbyLogin_RunConnect). Each
 * stage starts at 0; the scene task and the connection's steps clear it
 * when they move the login to another stage. */
enum LoginConnectState {
    LOGIN_START = 0,            /* moves on to the network screen */
    LOGIN_NET_SCREEN = 1,       /* the network screen (NetScreen_Run) */
    LOGIN_CONNECT = 2,          /* the lobby server's connection (NetScreen_ConnectStep); the network screen's first step sets it */
    LOGIN_CONNECTED = 3,        /* set by the connection once connected; the stage ends in the same frame */
    LOGIN_CONN_FAILED = 10,     /* the failed connection's message, then the portal */
    LOGIN_PORTAL = 20           /* the portal's pages (LobbyKddi_PortalStep); no code seen sets it */
};

/* gLoginState in the leave (LobbyLogin_Leave). */
enum LoginLeaveState {
    LOGIN_LEAVE_PORTAL = 0,     /* the portal's step ends */
    LOGIN_LEAVE_DISCONNECT = 1, /* the socket closes (LobbyNet_DisconnectStep); NetScene_ToDisconnect starts here */
    LOGIN_LEAVE_CHOOSE = 2,     /* gNetErrorCode chooses the exit */
    LOGIN_LEAVE_EXIT = 3,       /* the exit runs (enum LoginLeaveExit, in gNetConnStep) */
    LOGIN_LEAVE_DONE = 4        /* returns 1; the room's leave and NetScene_ToNetFileCheck start here */
};

/* LobbyLogin_Leave's exits (gNetConnStep at LOGIN_LEAVE_EXIT). */
enum LoginLeaveExit {
    LEAVE_EXIT_NET_ERROR = 0,   /* the line error's message, then the set-up's settings */
    LEAVE_EXIT_SETTINGS = 1,    /* the set-up's settings */
    LEAVE_EXIT_RELEASE = 2,     /* the network textures released */
    LEAVE_EXIT_CONN_FAILED = 3  /* the failed connection's message, then the portal */
};

typedef struct LobbyState {
    /* 0x0000 */ s8 groupJoin;      /* set on joining a group (Group_Join, the group board): the level lists go on by themselves */
    /* 0x0001 */ char groupId[0x10]; /* the group joined: its id (memcpy 16 bytes from the group, LobbySrv_Index) */
    /* 0x0011 */ u8 unk11[3];
    /* 0x0014 */ s32 area;          /* the group's area (Group_Join; LobbyLogin_EnterEntry) */
    /* 0x0018 */ s32 land;          /* the group's land (Group_Join; LobbyLogin_EnterEntry) */
    /* 0x001C */ u8 idCursor;       /* the id menu's cursor kept (LobbyEntry_IdSelect) */
    /* 0x001D */ u8 idMenuPrompt;   /* the id menu's opening prompt (LobbyMenu.promptStep): 0, or 1 and 2 after the login's errors 7 and 8 */
    /* 0x001E */ s8 hostId[8];          /* the user id of the town host: it answers the TownMsg requests when it equals myId */
    /* 0x0026 */ char charaIds[4][8];   /* the four character ids (LobbyNet_GetCharaId; IdMenu_Redraw, LobbyEntry_IdSelect) */
    /* 0x0046 */ u8 charaNames[0x44]; /* four names of 17 bytes (LobbyNet_GetCharaName; IdMenu_DrawText) */
    /* 0x008A */ u8 charaTexts[0x400]; /* four texts of 256 bytes (LobbyNet_GetCharaText; HunterCard_Sanitize) */
    /* 0x048A */ u8 myId[8];            /* this player's user id (login; gMyUserId keeps a copy) */
    /* 0x0492 */ u8 myName[0x10];   /* this player's character name (LobbyNet_GetMyCharaName; gMyName keeps a copy) */
    /* 0x04A2 */ u8 unk4A2[4];
    /* 0x04A6 */ u8 myText[0x100];  /* this player's text, 256 bytes (gPlayerText; Lobby_UpdatePlayerText) */
    /* 0x05A6 */ NetUser players[LOBBY_ROOM_PLAYERS];
    /* 0x0A16 */ NetUser members[LOBBY_MEMBER_COUNT];   /* the town's members (the village: LobbyMember_GetCard, LobbyMember_InMyParty) */
    /* 0x152E */ u8 pcLoaded[0xA];        /* per player: 1 once its model is loaded (TownPc_LoadModel); 0: the town updates skip it; cleared when it leaves */
    /* 0x1538 */ char memberSlot[0xA];     /* per member: '0' + the player slot it is in; read as u8 */
    /* 0x1542 */ u8 unk1542[0x20];
    /* 0x1562 */ u16 houseSeatBits;     /* the player's house's seats taken, one bit per seat (TownMsg_OnHouseSeatTaken, TOWN_MSG_HOUSE_SEAT_*) */
    /* 0x1564 */ u8 houseSeatIds[0x80];   /* the user id (LOBBY_USER_ID_LEN bytes) of whoever sits in each of the 16 house seats; cleared when the seat is freed */
    /* 0x15E4 */ u8 loginNotice;        /* the 6105 text (LobbyNet_GetLoginNotice fills 4100 bytes from here: NetLoginNotice.unk0) */
    /* 0x15E5 */ u8 pad15E5;
    /* 0x15E6 */ s16 loginNoticeSecs;   /* NetLoginNotice.unk2: the countdown shown with the text, in seconds (read as u16 and s16) */
    /* 0x15E8 */ u8 loginNoticeText[0x1000]; /* NetLoginNotice.text (LobbyDialog_OpenCountdown; copied to errText) */
    /* 0x25E8 */ u8 loginInfo[4];     /* 614C's reply (LobbyNet_GetLoginInfo fills 4100 bytes from here: NetLoginInfo.unk0) */
    /* 0x25EC */ u8 loginInfoText[0x1000]; /* NetLoginInfo.text (LobbyDialog_OpenWait) */
    /* 0x35EC */ NetPatchInfo patchInfo; /* the patch the server announced at login (LobbyEntry_Patch) */
    /* 0x3608 */ char errText[0x300];   /* the server's error text (LobbyNet_GetErrText) */
    /* 0x3908 */ s32 timer;             /* frames left of a menu's wait (LOBBY_MENU_TIMEOUT); below 0, LOBBY_QUIT_FAILED */
    /* 0x390C */ s32 recvTimer;         /* frames left without a message (from LOBBY_RECV_TIMEOUT) before LOBBY_QUIT_LINK_LOST */
    /* 0x3910 */ s32 idleTimer;         /* frames left without input (from LOBBY_IDLE_TIMEOUT) before LOBBY_QUIT_IDLE */
    /* 0x3914 */ s32 countdown;          /* frames, shown in seconds (/30) */
    /* 0x3918 */ s32 waitTimer;     /* frames left on an entry screen (8, 0x708: LobbyEntry_Info) */
    /* 0x391C */ s32 questList;     /* the town quest list (256 bytes of gTownQuestLists) filled (LobbyStep_TownData) */
    /* 0x3920 */ s32 unk3920;
    /* 0x3924 */ s32 townEvent;     /* the town event from 6884 (slot 0 first byte, Lobby_ApplyTownData): 1 to 3 change the shop stock (Shop_BuildStock), 4 halves CombShop prices; tested by the Talk_Check functions */
    /* 0x3928 */ s32 place;         /* the level the player is in, enum LobbyPlace (Lobby_SetPlace, Lobby_GetPlace) */
    /* 0x392C */ s32 enterSeq;      /* the village entry, LOBBY_ENTER_VILLAGE, _STAY or _RETURN (LobbyVillage_Begin*; LobbyScreen_EnterSeq) */
    /* 0x3930 */ u16 roomMembers;       /* the other members of the room: +1 is compared with the room size, below gQuestMinMembers the quest is disbanded */
    /* 0x3932 */ u16 buttons;           /* the menu buttons of this frame (LobbyPad_Read, LobbyPad_Get) */
    /* 0x3934 */ s16 seqStep;            /* a sequence's step (LOBBY_SEQ_FAILED: failed) */
    /* 0x3936 */ u8 phase;             /* the lobby phase (5: leaving, Lobby_Disconnect) */
    /* 0x3937 */ u8 phaseStep;           /* phase > phaseStep > mode > step > subStep: the levels */
    /* 0x3938 */ u8 mode;                /* of the lobby's state machines (Lobby_Disconnect clears them) */
    /* 0x3939 */ u8 step;
    /* 0x393A */ u8 subStep;             /* LOBBY_SEQ_FAILED: failed */
    /* 0x393B */ u8 subStep2;           /* the level below subStep (also a reply loop part counter) */
    /* 0x393C */ u8 spareStep0;         /* cleared with the steps above whenever a phase starts; nothing reads it */
    /* 0x393D */ u8 spareStep1;         /* the same */
    /* 0x393E */ s8 taskStep;       /* the step of the LobbyStep_* sequences that run a network task (LOBBY_SEQ_FAILED: failed) */
    /* 0x393F */ s8 unk393F;
    /* 0x3940 */ u8 unk3940;            /* cleared when a room's records are fetched (LobbyLevel2_FetchRecords); nothing reads it */
    /* 0x3941 */ u8 unk3941;
    /* 0x3942 */ u8 pageDataChanged;   /* LobbyKddi_WaitPage: the web page changed its data (no reader seen) */
    /* 0x3943 */ u8 webStep;              /* the step of LobbyWeb_Run (switch on it, counted up) */
    /* 0x3944 */ u8 linkState;      /* 0 at reset, LINK_STATE_*: LobbyEntry_Terms tests LINK_STATE_WEB_BACK, Lobby_Tick LINK_STATE_DOWN */
    /* 0x3945 */ u8 awaitReply;            /* the tag of the request whose reply is awaited (enum LobbyAwait) */
    /* 0x3946 */ u8 quitReason;        /* the reason given to Lobby_Disconnect */
    /* 0x3947 */ u8 partyCount;     /* the room's party list count (LobbyRoom_FetchParty) */
    /* 0x3948 */ u8 questPaid;            /* 1: a quest was accepted with its fee paid */
    /* 0x3949 */ s8 areaLoaded;     /* 1 once the area's models are loaded (Town_EnterArea, TownPc_LoadAllModels; Town_FreeArea clears it) */
    /* 0x394A */ u8 modelLoaded[0xA]; /* per player: 1 once TownPc_LoadModel or Player_ApplyLook set the models up */
    /* 0x3954 */ u8 modelLoading[0xA]; /* per player: 1 while TownPc_LoadModel loads the models (TownPc_IsLoaded waits) */
    /* 0x395E */ s8 connected;           /* 1: on the lobby server (Lobby_Tick checks its timeouts) */
    /* 0x395F */ s8 level0Kicked;      /* a leave request, LEAVE_*: LEAVE_SENT_OUT by the server's 6307 (LobbyRoute_Level0Kick), LEAVE_BEGUN by LobbyVillage_BeginLeave */
    /* 0x3960 */ s8 level1Kicked;      /* the same for 640C (LobbyRoute_Level1Kick; LEAVE_BEGUN: LobbyTown_BeginLeave) */
    /* 0x3961 */ s8 level2Kicked;      /* the same for 6505 (LobbyRoute_Level2Kick; LEAVE_BEGUN: LobbyTown_BeginLeave) */
    /* 0x3962 */ s8 playersListed;  /* 1 once the quest's party is complete (LobbyRoute_PartyReady); Lobby_SendHeld returns it */
    /* 0x3963 */ s8 roomCloseKind;   /* the room has closed: ROOM_CLOSE_* (0: open) */
    /* 0x3964 */ s8 serverClose;    /* the server ended the session, SERVER_CLOSE_* (set out of LOBBY_PHASE_EXIT) */
    /* 0x3965 */ s8 dlQuestsReady;      /* 1: the downloaded quests can be used (Quest_IsDownloadId checks skip them while 0) */
    /* 0x3966 */ u8 dlQuestCount;       /* the downloaded quests (count) */
    /* 0x3967 */ u8 chatToTargets;      /* 1: chat lines are told to gChatTargets alone; 0: said to everyone (Chat_SendToTargets, Chat_SendText) */
    /* 0x3968 */ u8 unk3968;            /* seen: 1 once the lobby screen's area or models are loaded (LobbyScreen_LoadArea, LobbyScreen_LoadModels), 0 on entering it online; no reader seen */
    /* 0x3969 */ u8 returnToArea;       /* 1: entering the town goes back to the area left for a quest (LobbyEnter_Begin's message names it, gQuestReturnArea) */
    /* 0x396A */ u8 questTaken;            /* 1 while a quest is taken */
    /* 0x396B */ u8 setOutOn;    /* 1 after 6504 (LobbyStep_SetOutDone), 0 after its leave (LobbyStep_SetOutLeaveDone) */
    /* 0x396C */ u8 mealEaten;      /* 1 after a meal (Kitchen_EatDish, Meal_ApplyEffects; ShipMenu_Update clears it) */
    /* 0x396D */ u8 unk396D;            /* seen: cleared on entering the lobby screen online; no other use */
    /* 0x396E */ u8 inLevel;            /* 1 while place is a level (LOBBY_PLACE_LEVEL0-2), 0 at LOBBY_PLACE_NONE (Lobby_SetPlace); no reader seen */
    /* 0x396F */ u8 elderRaidState; /* ELDER_RAID_*: ElderRaid_SetState turns the raid on, with or without its movie (ElderRaidEvent.noMovie); ELDER_RAID_OFF once over */
    /* 0x3970 */ u8 movieState;     /* RAID_MOVIE_*: the raid's town movie waiting to play (LobbyTown_WantsMovie, LobbyTown_StartMovie) */
    /* 0x3971 */ u8 pad3971;
    /* 0x3972 */ s16 petPoints[2];     /* the pets' points for the day, by enum PetArea: the Poogie's
                                         * feeding (PoogieMenu_Feed) and the care pet's (PoogieMenu_UpdateCare) */
    /* 0x3976 */ s16 petPointsMax[2];  /* the points' limits, by enum PetArea (Poogie_DailyReset rolls them) */
    /* 0x397A */ u8 roomName[0xF];      /* the room's name (6915; LobbyRoom_FetchParty), 15 bytes: the
                                         * game copies 16 bytes in (LobbyRoom_FetchParty), so the name's terminator
                                         * lands on player 0's byte below, and 15 out */
    /* 0x3989 */ u8 lookPending[0x17];  /* per player: nonzero while its look is to be applied: 2 changed (TownPc_RefreshChanged), 0 after Player_ApplyLook; a byte per player (10 used), read as s8 (TownPc_UpdateAll);
                                         * LobbyTown_Enter clears the 10 */
} LobbyState;
typedef char LobbyState_size_check[(sizeof(LobbyState) == 0x39A0) ? 1 : -1];

/*
 * The lobby screens' menu state (gLobbyMenu): the screen and its step, a
 * cursor, a page of pages, a countdown. A file that uses it declares
 * `extern LobbyMenu *gLobbyMenu;` itself (some still declare it u8 *).
 */
typedef struct LobbyMenu {
    /* 0x00 */ MenuTask *layer[8];            /* sub_main draw tasks gLobbyMenuLayer0..gLobbyMenuLayer7 (32 bytes apart): +0x14 the
                                         * draw callback, +0x18 the text size (LobbyMenu_Init, LobbyMenu_SetDraw) */
    /* 0x20 */ u8 unk20[4];
    /* 0x24 */ s16 timer;               /* frames left (screen changes, fades) */
    /* 0x26 */ u8 unk26[2];
    /* 0x28 */ s16 page;
    /* 0x2A */ s16 pages;
    /* 0x2C */ u8 unk2C[2];
    /* 0x2E */ u8 screen;               /* the menu's page (gIdMenuScreens[screen]) */
    /* 0x2F */ u8 step;
    /* 0x30 */ s8 unk30;
    /* 0x31 */ u8 subStep;             /* a step within the screen step (switch, then ++) */
    /* 0x32 */ u8 unk32;
    /* 0x33 */ u8 promptStep;       /* a third step byte: PROMPT_*: the closing prompt runs a countdown, waits for a button, then the screen goes on (LobbyLevel0_LeaveCountdown, LobbyTown_Leave); for the server choice menu, NetScreen_Step1 sets 1 to show its introductory message first (SrvMenu_SlideIn) */
    /* 0x34 */ u8 unk34;
    /* 0x35 */ u8 cursor;
    /* 0x36 */ u8 unk36[2];
    /* 0x38 */ u8 quitYesNo;        /* the quit question's answer: 0 yes (SrvMenu_Quit, Menu_SetYesNo) */
    /* 0x39 */ u8 redraw;               /* 1: redraw the texts (IdMenu_Redraw clears it) */
    /* 0x3A */ u8 unk3A[2];
    /* 0x3C */ u8 yesNo;               /* the yes/no cursor: 0 yes, 1 no (Menu_SetYesNo) */
    /* 0x3D */ u8 hideLayers;       /* nonzero: the frame queues no menu layers (the screens skip LobbyMenu_QueueLayers) */
    /* 0x3E */ s16 slideTimer;      /* frames left of a slide or wait (SrvMenu_Start's slide-in: 6, KitchenGuest_Update) */
    /* 0x40 */ u8 unk40[4];             /* LobbyMenu_Init clears 68 bytes */
} LobbyMenu;

/* LobbyMenu.promptStep in a closing prompt (the server's text shown with a
 * countdown before a screen is left). */
#define PROMPT_COUNTDOWN 0         /* the countdown runs (LobbyState.countdown) */
#define PROMPT_PRESS 1             /* it ran out: the prompt waits for PAD_CIRCLE */
#define PROMPT_DONE 2              /* pressed: the screen goes on */
typedef char LobbyMenu_size_check[(sizeof(LobbyMenu) == 0x44) ? 1 : -1];

/* The third level's entry records as the screens keep them (gLobbyRecs, 169301 bytes at
 * 0x8C0CA0; LobbyLevel2_FetchRecords copies them from gLobbyNet.recs2, NetRecs2): the room's
 * settings (password, the member count, the arena cup's kinds) are the records' values. */
typedef struct LobbyRec {
    /* 0x0000 */ u8 flags;
    /* 0x0001 */ char name[0x41];
    /* 0x0042 */ u8 subCount;       /* how many sub-entries (subName, subN, subData) are used */
    /* 0x0043 */ u8 editable;       /* NetRec2.editable */
    /* 0x0044 */ u8 defaultChoice;  /* NetRec2.defaultChoice */
    /* 0x0045 */ u8 cur;            /* the value chosen (task 7 sends it) */
    /* 0x0046 */ u8 pad46[0x22];
    /* 0x0068 */ char subName[0x20][0x41];
    /* 0x0888 */ u8 subN[0x20];
    /* 0x08A8 */ u8 subData[0x20][0x20][3];
} LobbyRec;
typedef char LobbyRec_size_check[(sizeof(LobbyRec) == 0x14A8) ? 1 : -1];

typedef struct LobbyRecs {
    /* 0x00000 */ u8 sendName;
    /* 0x00001 */ u8 sendPass;
    /* 0x00002 */ char pass[9];     /* the room's password (LobbyMenu_KbdEntry) */
    /* 0x0000B */ u8 padB[0x47];
    /* 0x00052 */ u8 usePass;       /* 1: a password is set (gNoneSetTexts) */
    /* 0x00053 */ u8 pad53;
    /* 0x00054 */ u8 n;             /* records */
    /* 0x00055 */ LobbyRec rec[32]; /* rec[0]: the member count (gMemberCountTexts); rec[1..3]: the arena cup's */
} LobbyRecs;
typedef char LobbyRecs_size_check[(sizeof(LobbyRecs) == 0x29555) ? 1 : -1];

/* The town player's state groups (Actor_SetState's group): 0 the town's
 * actions (enum TownPcState), 1 the gestures (gTownGestureStates). */
#define TOWN_PC_GROUP_ACTION    0
#define TOWN_PC_GROUP_GESTURE   1

/* The town player's gesture state (group TOWN_PC_GROUP_GESTURE) after the
 * player.h gesture ids (PL_GESTURE_*): a held gesture left with the stick
 * goes back to idle through it. */
#define TOWN_GESTURE_END        0x11

/* TownPc_MoveKind's answer: how the player moves this frame. */
#define TOWN_MOVE_NONE          0   /* it doesn't */
#define TOWN_MOVE_WALK          1   /* the stick tilted 40 or more */
#define TOWN_MOVE_WALK_FULL     3   /* the stick fully tilted (120 or more) */
#define TOWN_MOVE_DASH          5   /* PAD_R2 held while moving, standing, the weapon sheathed */

/* The kind of a town's action spot (ActionSpot.kind): what the action
 * button does there (TownPc_HandleInput). The spot's type is its number:
 * the seat's index, the dig spot's, the contest seat's, the area the door
 * leads to. */
enum TownSpotKind {
    TOWN_SPOT_SEAT = 1,             /* a tavern seat; offline in the afternoon it opens the drinking contest */
    TOWN_SPOT_FISHING = 2,          /* the fishing spot: the bait menu */
    TOWN_SPOT_QUEST_COUNTER = 3,    /* the quest counter (from hunter rank 1) */
    TOWN_SPOT_EXIT = 4,             /* the town's exit: leave the town? (TOWN_MODE_EXIT_ASK) */
    TOWN_SPOT_SAVE = 5,             /* the save menu */
    TOWN_SPOT_BOX = 6,              /* the box menu */
    TOWN_SPOT_BED = 8,              /* the bed: the player walks there and rests (TOWN_MODE_SLEEP) */
    TOWN_SPOT_MEAL = 9,             /* the kitchen's seat, until a meal is eaten */
    TOWN_SPOT_HOUSE_SEAT = 10,      /* a seat in the house, asked of the house's owner */
    TOWN_SPOT_PRICE_BOARD = 12,     /* the price board (PriceInfo_Open) */
    TOWN_SPOT_POOGIE_DIG = 13,      /* a Poogie dig spot (TOWN_PC_POOGIE_DIG) */
    TOWN_SPOT_CONTEST = 14,         /* an arm-wrestling seat */
    TOWN_SPOT_VILLAGE_BOARD = 15,   /* the village board */
    TOWN_SPOT_BOARD = 16,           /* the area board, or the group board when the level has groups */
    TOWN_SPOT_GATE = 17,            /* the quest gate: sets out on the quest taken */
    TOWN_SPOT_MOVE = 23,            /* the move menu */
    TOWN_SPOT_AREA_DOOR = 24,       /* a door to the area its type names (AreaMenu_Open) */
    TOWN_SPOT_HOUSE_ACCESS = 25,    /* the house's access setting */
    TOWN_SPOT_RECORDS = 26          /* the records menu */
};

/* The town's player states (Actor_SetState's group 0; TownPc_RunActionState runs
 * each in its TownPc_State function). The seat poses are picked with the
 * d-pad while seated, and each has an END state back to the seat. */
enum TownPcState {
    TOWN_PC_IDLE = 0x01,
    TOWN_PC_WALK = 0x03,
    TOWN_PC_RUN = 0x04,             /* the run with R2 */
    TOWN_PC_RUN_LOOP = 0x05,        /* the walk's handler, running */
    TOWN_PC_RUN_STOP = 0x06,
    TOWN_PC_SEAT_POSE_DOWN_END = 0x07,
    TOWN_PC_SEAT_POSE_UP_END = 0x08,
    TOWN_PC_SEAT_POSE_RIGHT_END = 0x09,
    TOWN_PC_OFFER_ITEM = 0x0A,      /* hands an item to another member */
    TOWN_PC_TAKE_ITEM_WAIT = 0x0B,  /* waits for the other member's answer */
    TOWN_PC_OFFER_ITEM_END = 0x0C,
    TOWN_PC_TAKE_ITEM = 0x0D,
    TOWN_PC_BED_LIE_DOWN = 0x0E,    /* walks to the bed's spot (TownPc_WalkToBed) and lies down; then TOWN_PC_BED_SLEEP */
    TOWN_PC_BED_SLEEP = 0x0F,       /* lies in bed, breathing, while the bed's menu runs */
    TOWN_PC_BED_GET_UP = 0x10,      /* turns round and gets up; the local player's town mode goes back to free */
    TOWN_PC_TURN_AROUND = 0x11,     /* turns round, then runs */
    TOWN_PC_DRUNK_COLLAPSE = 0x12,  /* the drunk idle's slump after 240 frames */
    TOWN_PC_DRUNK_GET_UP = 0x13,
    TOWN_PC_SIT_DOWN = 0x14,        /* sits on a table seat */
    TOWN_PC_STAND_UP = 0x15,
    TOWN_PC_SEAT_POSE_LEFT_END = 0x16,
    TOWN_PC_BED_NAP = 0x17,         /* the bed's sleep for 360 frames, then TOWN_PC_BED_GET_UP */
    TOWN_PC_MEAL_RISE_GOOD = 0x18,  /* rises after a good meal (TownPc_StateMealRise) */
    TOWN_PC_REST = 0x19,            /* the rest at an action spot of kind 8, short form */
    TOWN_PC_REST_LONG = 0x1A,       /* the same with its own motions, and AREA_HUB_0x57's jingle */
    TOWN_PC_GET_UP_FACE_UP = 0x1B,  /* after TOWN_PC_MEAL_COLLAPSE */
    TOWN_PC_SEAT_POSE_RIGHT = 0x1C,
    TOWN_PC_DRUNK_IDLE = 0x1D,      /* the idle of a drunk player (Player.drunk) */
    TOWN_PC_KITCHEN_ORDER = 0x1E,   /* Kitchen_Open sets it */
    TOWN_PC_THROWN = 0x1F,          /* thrown by a townsperson after TOWN_PC_GRABBED: falls and gets up */
    TOWN_PC_GRABBED = 0x20,         /* a townsperson's grab sets it */
    TOWN_PC_SEAT_DRINK = 0x21,      /* drinks at the seat (square); counts Player.drinkCount */
    TOWN_PC_SEAT_CROSS = 0x22,      /* the seat's cross-button motion */
    TOWN_PC_DRUNK_WALK = 0x23,      /* the walk's handler, drunk */
    TOWN_PC_SEAT_POSE_LEFT = 0x24,
    TOWN_PC_SIT_DOWN_SEATED = 0x25, /* TOWN_PC_SIT_DOWN's handler, already seated */
    TOWN_PC_SEAT_POSE_DOWN = 0x26,
    TOWN_PC_MEAL_RISE = 0x27,       /* rises after a plain meal */
    TOWN_PC_SEAT_POSE_UP = 0x28,
    TOWN_PC_MEAL_SERVED = 0x29,     /* the meal scene sets it when the cooking is done */
    TOWN_PC_MEAL_COLLAPSE = 0x2A,   /* a bad meal: collapses, lies, gets up */
    TOWN_PC_FISH_WAIT = 0x61,       /* fishing: waits with the line out, after the cast */
    TOWN_PC_FISH_CAST = 0x62,       /* fishing: casts the line (the bait menu's confirm); then TOWN_PC_FISH_WAIT */
    TOWN_PC_CONTEST_GRIP = 0x6F,    /* the arm-wrestling match: set on both players as it starts */
    TOWN_PC_CONTEST_SEAT_A = 0x70,  /* a contest seat, by gContestSeatId (Contest_TakeSeat) */
    TOWN_PC_CONTEST_SEAT_B = 0x71,
    TOWN_PC_CONTEST_LOSE = 0x74,    /* set on the loser by both contests */
    TOWN_PC_CONTEST_WIN = 0x75,     /* set on the winner */
    TOWN_PC_GALLERY_AIM = 0x79,     /* holds the shooting gallery's gun */
    TOWN_PC_POOGIE_DIG = 0x80,      /* at the Poogie's dig spot (TownPc_HandleInput) */
    TOWN_PC_DRINK_MISS = 0x8D,      /* the drinking contest: a missed key */
    TOWN_PC_DRINK_LOSE = 0x90,      /* the opponent emptied its mugs first */
    TOWN_PC_DRINK_WIN = 0x91,       /* the player emptied its mugs first */
    TOWN_PC_HAIR_CHAIR = 0x9A,      /* sits in the hair menu's chair */
    TOWN_PC_WALK_TO_HAIR_CHAIR = 0x9C, /* HairMenu_MovePlayer walks the player to the chair */
    TOWN_PC_CONTEST_DRAW = 0xA0     /* the arm-wrestling match: both powers equal */
};

extern LobbyState gLobbyState;
extern UserText gPlayerText;  /* the local player's (sub_main; Lobby_UpdatePlayerText) */
extern LobbyState *gLobby;      /* = &gLobbyState */

void LobbyMenu_StartTimer(void);
s32 LobbyMenu_TickTimer(void);
void LobbyMenu_Init(void);
void Lobby_ClearRequests(void);
s32 Lobby_SendHeld(void);
void Lobby_ResetState(void);
s32 LobbyLevel_SelId(s32 level);
void Lobby_ResetIdleTimer(s32 *timer);
void Lobby_ReceiveWhileWaiting(void);
void Lobby_ClearHost(void);
void LobbyMenu_SetDraw(void *draw0, void *draw1, void *draw2, void *draw3);
void Lobby_DrawBlackLine(void);
void *LobbyLevel2_Entry(s32 idx);
s32 Lobby_HasQuest(void);
void Lobby_SetPlace(s32 place);
s32 Lobby_GetPlace(void);
void LobbyTime_Tick(void);
void LobbyTime_SetClock(u32 time);
f32 LobbyTime_GetDayFrac(void);
f32 LobbyTime_GetDayLen30(void);
s32 LobbyTime_IsAm(void);
s32 LobbyTime_IsPm(void);
void Chat_ClearTargets(void);
void Lobby_LoadTextures(void);

#endif /* LOBBY_STATE_H */
