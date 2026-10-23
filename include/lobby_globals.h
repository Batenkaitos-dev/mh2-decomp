#ifndef LOBBY_GLOBALS_H
#define LOBBY_GLOBALS_H

/*
 * sub_main globals the lobby used to read through raw address casts
 * (`*(u8 *)0x6ADCEC`). Fields of the scene state (gScene, scene.h), the
 * save data (gSave, save.h) and the pads (gPadState, pad.h) are named as
 * members (the files include those headers; a file that views gSave as
 * `u8 gSave[]` uses byte offsets); sub_main's scalars by their sub_main.txt
 * names; the rest are defined in config/symbols/lobby.txt (section "sub_main
 * globals named for the lobby") under their addresses unless named. Declared
 * with the files' own declaration or the type most accesses use; other
 * accesses cast the address (`*(s8 *)&gFieldControl`).
 * Where a name changed a function's code (MWCC shares one `lui` between raw
 * addresses in a 64 KB page, and assumes a named global is not aliased by
 * stores through pointers), the function keeps the raw cast.
 */

#include "common.h"
#include "net_tcp.h"

/* sub_main.txt names */
extern u16 gFrameTick;
extern u8 gPortalPage;
/* gPortalPage: which page of the KDDI portal LobbyKddi_PortalStep opens. */
#define PORTAL_PAGE_TOP 0   /* the portal's top page (its fixed URL) */
#define PORTAL_PAGE_BACK 1  /* the MMBB data's f4 URL: set once the portal has led on to the lobby */
#define PORTAL_PAGE_NEXT 2  /* the MMBB data's next URL, after the lobby was left with a disconnection */
#define PORTAL_PAGE_QUIT 3  /* the next URL too, after the quit menu's choice of the portal (LOBBY_QUIT_TO_PORTAL) */
extern s16 gHelpLine;
extern u8 gHelpShow;
extern u8 *gLoadBuf;
extern u8 gCameraPreset;
extern s32 gNetSock;

/* config/symbols/lobby.txt */
extern void *gPadPtr;
extern char *gInfoMenuLabels[]; /* sub_main area_draw.c: the info menu labels (the town info prints [0]) */
extern s32 gActiveSkillsTitle;
extern s32 gPouchFullTakeText;
extern char *gItemNames[];
extern u16 gKbdHeldKey; /* the first key held in sub_main's copy of the USB keyboard's report (gKbdSettings.keys[0]) */
extern NetMsgState gNetMsgState; /* the connect state machine's state */
extern s16 gNetCancelHold;
extern s16 gNetRetries;
extern u8 gNetProgress;
extern s16 gLobbyCommonSeBank;
extern s8 gChatHidden;
extern u8 gChatHideNames;
extern u8 gChatToAll;
extern s8 gHelpPage;
extern u8 gHelpColumn;
extern s8 gPauseMenuReq;
extern u16 gChatWinTimer;
extern u8 gChatWinOpen;
extern u8 gChatWindowSel;
extern u8 gChatSendTo;
extern u8 gChatWinState;
extern u8 gChatWinHold;
extern s32 gRewardMoney;
extern s32 gQuestQNetReadyArg;
extern u32 gSrvQNetPeerTimeout;
extern u32 gSrvQNetMaxStall;
extern u8 gElderForecastHeard;
extern u16 gQuestDeskShown[2][12]; /* the quest board's shown quest ids: [1] in AREA_ELDER_HALL, [0] elsewhere */
extern u16 gQuestOfferId;
extern u8 gQuestAcceptDay;
extern u8 gLobbyLastPm;
extern u8 gLobbyLastSeason;
extern u8 gMemberNamesOffSaved;
extern u8 gOnlineMark;
extern u8 gLobbyServerCount;
extern s8 gPortalExit;
/* gPortalExit: how the portal's browser ended. */
#define PORTAL_EXIT_LEAVE 0         /* left (the menu's leave line, the leave dialog) */
#define PORTAL_EXIT_GAME_MENU 1     /* for the game's menu (Portal_GameMenuStep) */
#define PORTAL_EXIT_CANCEL 2        /* cancelled (Portal_CancelStep) */
extern s8 gProxyMode;
extern u16 gQuestReturnArea;
extern u8 D_5CAA40;
extern u8 gServerTriedCount;
extern u8 gLobbyConnDone;
extern s8 gLobbyNetError;
extern u8 gPoogieErrand;
extern u8 gSeasonFlags;
extern u8 gQuestRecordPhase;
extern u32 gQuestRecordElapsed;
extern s32 gQuestTimeLeft;
extern s32 gQuestReward;
extern struct QuestInfo *gQuestInfo;
extern f32 gCamDir[3];
extern struct NextQuest gNextQuest; /* quest.h's NextQuest: the quest to start next */
extern s16 D_6ADADE;
extern s16 D_6ADAFA;
extern u8 D_6ADB0C[];
extern s16 D_6ADB14;
extern s8 gOnlineFlag;
extern u8 gFieldControl; /* suggested name gFieldControl: 1 = the hunter takes pad input in the field (Field_ControlEnabled); the lobby clears it while a menu or talk runs */
extern u8 gSoundLoading;
extern s8 gSysBusy;
extern s8 gGmSubLoaded;
extern u8 *gMemAreas;
extern u8 *gObjBufBlocks;            /* the object buffer: 256 blocks of 512 bytes (StageObj_Alloc) */
extern struct ModelMatSet *gMatSets;     /* 384 material sets of 140 bytes (sub_main model_pool.c) */
extern struct Obj50 *gMaterials;     /* 1024 materials of 80 bytes (sub_main model_pool.c) */
extern u8 *gLobbyWorkBuf;
#define LOBBY_WORK_TOWN_MOVE_FRAMES 0x7A90 /* u16 in gLobbyWorkBuf: the server's period of the town's position messages, 0 for the default (Lobby_ApplyServerTimers) */
#define LOBBY_WORK_RECRUIT_FRAMES 0x7A92 /* u16: the server's recruiting period, likewise */
#define LOBBY_WORK_FRIEND_CHECK_FRAMES 0x7A96 /* u16: the server's friend check period, likewise */
#define LOBBY_WORK_AREA_SEARCH_FRAMES 0x7A9A /* u16: the server's area search period, likewise */
#define LOBBY_WORK_EM_SIZE_RANGES 0x7AA0 /* 10 bytes: the server's monster size ranges, copied to gEmSizeRanges */
#define LOBBY_WORK_MEAL_GIFT 0x7A98 /* u16 in gLobbyWorkBuf: online, the money a diner may be given after a dish with others (Kitchen_Update) */
#define LOBBY_WORK_QNET_READY_ARG 0x7A94 /* u16 in gLobbyWorkBuf: when nonzero, QuestDesk_Accept copies it to gQuestQNetReadyArg online */
extern void *gSaveBuf;
extern u8 gBootMode;
extern f32 gFps;
extern u8 *gLbguildFree;
extern s32 gCardMenuTex;
extern s32 gLoadLettersTex;
extern s32 D_6AE3F8;
extern s32 gOvlTex;
extern s32 D_6AEFA8;

/*
 * A wanted entry of the town's lists (gTownQuestLists): posted when its three
 * values are all nonzero (Wanted_IsSet); the low byte of v[1] is the wanted
 * monster's kind (Wanted_MatchesQuest), and v[0] 3 never matches a quest.
 * AREA_TAVERN and AREA_ELDER_HALL use the first and the second set of three
 * lists, one list per season (LobbyWorld_GetSeason).
 */
/* An offer townsperson (gOfferNpcs, ended by id 0). */
typedef struct OfferNpc {
    s8 kind;            /* 0-2: the townsperson's group, which takes offers in turn (Offer_PickKind) */
    u8 defs[4];         /* gRequestDefs indexes: the first two always, the last two once condition 21 holds */
    u8 pad5;
    u16 id;             /* the townsperson (an NPC id) */
} OfferNpc;

#define WANTED_SEASONS  3   /* the lists per hub, one per season */
#define WANTED_ENTRIES  3   /* the entries per list */
#define WANTED_KIND_MONSTER 1   /* TownEntry.v[0]: a monster to hunt (v[1] its id) */
#define WANTED_KIND_ITEM    3   /* TownEntry.v[0]: an item to deliver (v[1] its id); never matches a quest */

typedef struct TownEntry {
    /* 0x0 */ s16 v[3];
    /* 0x6 */ s16 unk6;
} TownEntry;

/* gTownQuestLists (0x8BECA0) and sub_main's copy gTownQuestListsCopy (the
 * lobby keeps the lists from the copy when the day and the list are the same).
 * The lobby also lends it to request 6884 as four 256-byte reply buffers,
 * past its 208 bytes. */
typedef struct TownQuestLists {
    /* 0x00 */ u8 townEvent;        /* the town data's (6884) slot 0 starts here: LobbyState.townEvent */
    /* 0x01 */ u8 pad1[3];
    /* 0x04 */ TownEntry a[WANTED_SEASONS][WANTED_ENTRIES];   /* AREA_TAVERN's wanted entries: [season][entry] */
    /* 0x4C */ TownEntry b[WANTED_SEASONS][WANTED_ENTRIES];   /* AREA_ELDER_HALL's (online only): [season][entry] */
    /* 0x94 */ u16 quests[2][3][5]; /* quest ids, 5 per list: [0] below hunter rank 41, [1] from 41 on (ElderForecast_Load) */
} TownQuestLists;
typedef char TownQuestLists_size_check[(sizeof(TownQuestLists) == 0xD0) ? 1 : -1];

extern TownQuestLists gTownQuestLists;
extern TownQuestLists gTownQuestListsCopy;


/* A message of the table gLobbyMsgTexts (152 of them; LobbyText_Open opens
 * one): its dialog kind with the flag LOBBY_MSG_DIM (the screen dimmed
 * behind the box), and its text. */
typedef struct LobbyMsgDef {
    /* 0x0 */ u16 flags;
    /* 0x2 */ u16 pad2;
    /* 0x4 */ char *text;
} LobbyMsgDef;

typedef char LobbyMsgDef_size_check[(sizeof(LobbyMsgDef) == 0x8) ? 1 : -1];

/* A user on the mail block list (gMailBlockList, MAIL_BLOCK_LIST_LEN of
 * them; the save keeps a copy). The block list page shows the name and the
 * id; IdList_Count and IdList_Find read the id. */
typedef struct BlockedUser {
    u8 userId[8];   /* the user's id (a string) */
    u8 name[16];    /* the user's name */
} BlockedUser;
#define MAIL_BLOCK_LIST_LEN 8

/* A user on the friend list (gFriendList, FRIEND_LIST_LEN of them; the save
 * keeps a copy). The first two fields are those of a BlockedUser; an entry
 * whose id starts with 0 is free. */
typedef struct FriendEntry {
    u8 userId[8];   /* the user's id (a string) */
    u8 name[16];    /* the user's name */
    u8 unk18[0x18];
} FriendEntry;
#define FRIEND_LIST_LEN 50

/* gFriendCheckTimers: the friend list's pages' timers (frames); when one runs
 * out, that page's statuses go back to FRIEND_STATUS_UNKNOWN, to be asked
 * again. Lobby_Tick runs the first FRIEND_PAGES of them. */
#define FRIEND_CHECK_TIMERS 12
#define FRIEND_TIMER_OFF 0xFFFF     /* a page's timer stopped */

/* A friend's connection status (gFriendStatus, one per gFriendList entry):
 * the server's status byte (LobbyNet_GetUserStatus), or what the friend
 * list sets while it asks. Named by the label each one draws. */
enum FriendStatus {
    FRIEND_STATUS_UNKNOWN = -1, /* not asked yet, or being asked */
    FRIEND_STATUS_ONLINE = 0,   /* in the lobby */
    FRIEND_STATUS_ON_QUEST = 1, /* on a quest */
    FRIEND_STATUS_OFFLINE = 2   /* not found on the server */
};

#endif
