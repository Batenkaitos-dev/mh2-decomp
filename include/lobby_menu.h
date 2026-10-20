#ifndef LOBBY_MENU_H
#define LOBBY_MENU_H

#include "common.h"
#include "menu_task.h"
#include "sprite.h"
#include "item.h"
#include "lobby_state.h"

/*
 * The village and town menus (lobby overlay).
 *
 * gTownMode (0x8BDC10) says which menu runs (Town_UpdateMode calls its
 * update); every menu keeps its state in the shared work area gMenuWork
 * (0x8BDC20, cleared when a menu opens: 0xF00 bytes by DojoMenu_Open), which
 * each menu's file views as its own struct (EquipShop, DojoMenu, PoogieMenu,
 * RecordMenu, PrizeMenu ...). A menu's draw callbacks are set with
 * LobbyMenu_SetDraw (gLobbyMenu's layers 0-3).
 *
 * Structs used by more than one file live here.
 */

/*
 * gTownMode: the town's menu mode. Town_UpdateMode runs the update of the
 * mode set (a switch on it); a menu sets its mode when it opens and
 * TOWN_MODE_FREE when it closes. Named by the update each mode runs; the
 * modes not listed here are steps of the switch itself (dialogs and their
 * waits).
 */
#define MENU_WORK_SIZE 0xF00  /* gMenuWork's bytes, cleared when a menu opens */

enum TownMode {
    TOWN_MODE_FREE = 0,             /* no menu: the player walks the town */
    TOWN_MODE_QUEST_COUNTER = 0x01, /* QuestCounter_Step */
    TOWN_MODE_ITEM_SHOP = 0x02,     /* ItemShop_Update */
    TOWN_MODE_PC_CARD = 0x03,       /* PcCard_Update */
    TOWN_MODE_TALK = 0x04,          /* a talk with a townsperson (set as the talk starts) */
    TOWN_MODE_SET_OUT_PAID = 0x06,  /* the quest's payer sets out (yes to SET_OUT_ASK or SET_OUT_ASK_FULL): LobbyStep_SetOut, then Quest_SendPaid and SET_OUT */
    TOWN_MODE_SET_OUT = 0x07,       /* the quest is paid (LobbyStep_SetOut done): the player is on the way out; lock-on is off */
    TOWN_MODE_EQUIP_SHOP = 0x08,    /* EquipShop_Update */
    TOWN_MODE_WORKSHOP = 0x09,      /* Workshop_Update */
    TOWN_MODE_COMBINE_SHOP = 0x0A,  /* CombShop_Update */
    TOWN_MODE_EXIT_ASK = 0x0C,      /* the town's exit spot (TOWN_SPOT_EXIT) asks whether to leave: then EXIT_YES_NO */
    TOWN_MODE_EXIT_YES_NO = 0x0D,   /* the leave question's yes or no: yes EXIT, no FREE */
    TOWN_MODE_EXIT = 0x0E,          /* Town_ModeDisconnect: the town is left for the village */
    TOWN_MODE_GAME_MENU = 0x0B,     /* GameMenu_Update */
    TOWN_MODE_POOGIE_PET = 0x10,    /* PoogieNaming_Update */
    TOWN_MODE_POOGIE_MENU = 0x11,   /* PoogieMenu_Update */
    TOWN_MODE_POOGIE_CARE = 0x12,   /* PoogieMenu_UpdateCare */
    TOWN_MODE_HOUSE_PET = 0x13,     /* HousePet_Update */
    TOWN_MODE_SAVE = 0x14,          /* TownSave_Update */
    TOWN_MODE_ROOM_LEAVE = 0x16,    /* a message is up and the quest room is left next: too few members (Town_CheckQuestParty), or sent out (Lobby_HandleLeaveRequest) */
    TOWN_MODE_ROOM_LEAVING = 0x17,  /* LobbyStep_Level2Leave, then MESSAGE_WAIT */
    TOWN_MODE_MESSAGE_WAIT = 0x18,  /* a message waits for PAD_CIRCLE, then back to FREE */
    TOWN_MODE_SEAT_WAIT = 0x19,     /* a table seat is asked of the host (TownPc_HandleInput); TownPc_TakeSeat sits down */
    TOWN_MODE_KEYBOARD = 0x1A,      /* the chat keyboard is open: back to FREE when it closes */
    TOWN_MODE_SET_OUT_JOIN = 0x1B,  /* a member who didn't pay sets out at the gate: LobbyStep_SetOut (LOBBY_TEXT_SENDING shown), then SET_OUT */
    TOWN_MODE_BOX_OPEN = 0x1C,      /* the box spot opened the box menu (BoxMenu_Open): BOX_MENU next frame */
    TOWN_MODE_BOX_MENU = 0x1D,      /* BoxMenu_Update; the stick turns the player meanwhile (TownPc_TurnByPad) */
    TOWN_MODE_SET_OUT_ASK = 0x1E,   /* the payer at the gate with members missing: set out? (the cursor on no); yes SET_OUT_PAID */
    TOWN_MODE_DEPART = 0x1F,        /* the player walks out to the taken quest: the stream fades out, then DEPART_FADE */
    TOWN_MODE_DEPART_FADE = 0x20,   /* once the fade ends: DEPARTED, with the quest's HUD started */
    TOWN_MODE_DEPARTED = 0x21,      /* the departure's end: LobbyScreen_Run leaves the lobby for the quest (returns 1) */
    TOWN_MODE_SCENE = 0x22,         /* set while a scene or the town tour runs (LobbyScreen_RunTour; an area event started by LobbyArea_StartEvent) */
    TOWN_MODE_QUIT = 0x23,          /* the game menu's quit (GameMenu_Quit): the stream fades out, then QUIT_FADE */
    TOWN_MODE_QUIT_FADE = 0x24,     /* once the fade ends: QUIT_DONE */
    TOWN_MODE_QUIT_DONE = 0x25,     /* the quit's end: LobbyScreen_Run returns -1 and the network scene ends the lobby */
    TOWN_MODE_SLEEP = 0x26,         /* the bed spot's sleep (TOWN_PC_REST, TOWN_PC_REST_LONG): their state sets FREE when it ends */
    TOWN_MODE_SET_OUT_CANCEL_ASK = 0x27, /* the gate again after a member's set-out: cancel it? yes SET_OUT_CANCEL, no SET_OUT */
    TOWN_MODE_SET_OUT_CANCEL = 0x28, /* LobbyStep_SetOutLeave (LOBBY_TEXT_ROOM_LEAVING shown), then FREE; on a failure SET_OUT_MESSAGE_WAIT */
    TOWN_MODE_SET_OUT_MESSAGE_WAIT = 0x29, /* the gate's message waits for PAD_CIRCLE, then SET_OUT */
    TOWN_MODE_SET_OUT_ASK_FULL = 0x2A, /* the payer at the gate with the room full: set out? (the cursor on yes); yes SET_OUT_PAID */
    TOWN_MODE_HOUSE_ACCESS = 0x2B,  /* HouseAccess_Update */
    TOWN_MODE_BAIT_MENU = 0x2C,     /* BaitMenu_Update */
    TOWN_MODE_PRICE_INFO = 0x2D,    /* PriceInfo_Update */
    TOWN_MODE_WAIT_CANCEL = 0x2E,   /* TownMode_WaitCancel: back to FREE on PAD_CROSS */
    TOWN_MODE_GALLERY = 0x2F,       /* Gallery_Run */
    TOWN_MODE_CONTEST = 0x30,       /* Contest_Update */
    TOWN_MODE_POOGIE_OUTSIDE = 0x31, /* PoogieMenu_UpdateOutside */
    TOWN_MODE_DRINK_CONTEST = 0x32, /* DrinkContest_Update */
    TOWN_MODE_MINE = 0x33,          /* Mine_UpdateMenu */
    TOWN_MODE_ARENA_CUP = 0x34,     /* ArenaCup_Update */
    TOWN_MODE_QUEST_DESK = 0x35,    /* QuestDesk_Update */
    TOWN_MODE_CONTEST_SEAT_WAIT = 0x36, /* a contest seat is asked of the host (TownPc_HandleInput) */
    TOWN_MODE_CONTEST_SEATED = 0x37, /* Contest_TakeSeat has seated the player */
    TOWN_MODE_SONG_MENU = 0x38,     /* SongMenu_Update */
    TOWN_MODE_GATE_MESSAGE_WAIT = 0x39, /* the gate's refusal (LOBBY_TEXT_GATE_*) waits for PAD_CIRCLE, then FREE */
    TOWN_MODE_AREA_MENU = 0x3A,     /* AreaMenu_Update */
    TOWN_MODE_MOVE_MENU = 0x3B,     /* MoveMenu_Update */
    TOWN_MODE_HAIR_MENU = 0x3C,     /* HairMenu_Update */
    TOWN_MODE_AREA_BOARD = 0x3D,    /* AreaBoard_Update */
    TOWN_MODE_GROUP_BOARD = 0x3E,   /* GroupBoard_Update */
    TOWN_MODE_DOJO = 0x3F,          /* DojoMenu_Update */
    TOWN_MODE_QUEST_PARTY = 0x40,   /* Town_CheckQuestParty, once the quest is paid */
    TOWN_MODE_ARENA_MENU = 0x41,    /* ArenaMenu_Update */
    TOWN_MODE_OFFER_MENU = 0x43,    /* OfferMenu_Update */
    TOWN_MODE_ELDER_FORECAST = 0x44, /* ElderForecast_Update */
    TOWN_MODE_OFFICIAL_DESK = 0x45, /* OfficialDesk_Update */
    TOWN_MODE_RECORDS = 0x46,       /* Records_Update */
    TOWN_MODE_INGREDIENTS = 0x47,   /* IngredientList_Update */
    TOWN_MODE_KITCHEN = 0x48,       /* Kitchen_Update */
    TOWN_MODE_KITCHEN_GUEST = 0x49, /* KitchenGuest_Update */
    TOWN_MODE_VILLAGE_BOARD = 0x4A, /* VillageBoard_Update */
    TOWN_MODE_TOWN_GUIDE = 0x4B,    /* TownGuide_Update */
    TOWN_MODE_SHIP_MENU = 0x4C,     /* ShipMenu_Update */
    TOWN_MODE_DIG = 0x4D,           /* a player digs at a Poogie dig spot (TownPc_StateDig); back to FREE when done */
    TOWN_MODE_NPC_GIFT = 0x4E,      /* NpcGift_Update */
    TOWN_MODE_MEAL_SCENE = 0x4F     /* MealScene_Update */
};

/*
 * LobbySe_Play's ids: the lobby's menu sounds. LobbySe_Play maps each to a
 * sound of a bank (Sound_PlaySeFlat); those named here play the system
 * bank's SE_SYS_* (sound.h), or (WINDOW_IN, WINDOW_OUT) the sounds of slot
 * 1 the menus play as their window slides in and out. The others stay
 * numbers until their uses show their roles.
 */
enum LobbySe {
    LOBBY_SE_CONFIRM_ALT = 0x00,    /* SE_SYS_CONFIRM, as LOBBY_SE_CONFIRM */
    LOBBY_SE_SCROLL = 0x01,         /* SE_SYS_SCROLL: a cursor, a page or a count changes */
    LOBBY_SE_CURSOR = 0x02,         /* slot 1's sound 0x90: a list's cursor moves (Menu_WrapUpDownSe, in the workshop's counters) */
    LOBBY_SE_NOTICE = 0x03,         /* SE_SYS_NOTICE */
    LOBBY_SE_CANCEL = 0x04,         /* SE_SYS_CANCEL: back */
    LOBBY_SE_REFUSE_ALT = 0x06,     /* SE_SYS_REFUSE, as LOBBY_SE_REFUSE */
    LOBBY_SE_MAIL = 0x07,           /* slot 1's sound 0x80: a short mail arrives in an empty mail box */
    LOBBY_SE_CONFIRM = 0x08,        /* SE_SYS_CONFIRM: a choice is confirmed */
    LOBBY_SE_REFUSE = 0x09,         /* SE_SYS_REFUSE: the choice can't be taken */
    LOBBY_SE_MONEY = 0x0A,          /* slot 1's sound 0x83: money changes hands (a purchase, a fee, a gift of money) */
    LOBBY_SE_OPEN = 0x0B,           /* SE_SYS_OPEN */
    LOBBY_SE_TALK = 0x0E,           /* a townsperson's talk starts (Talk_Start), a pet's menu opens */
    LOBBY_SE_SAVE_MENU = 0x0D,      /* slot 6's sound 0x40: the save menu opens over the bed's menu (TownNetMenu_Update) */
    LOBBY_SE_OPEN_ALT = 0x10,       /* SE_SYS_OPEN, as LOBBY_SE_OPEN */
    LOBBY_SE_MENU_OPEN = 0x11,      /* slot 1's sound 0x81: a counter's or a shop's menu opens */
    LOBBY_SE_EQUIP = 0x12,          /* slot 1's sound 0x89: a new or changed piece is put on (the workshop's counters) */
    LOBBY_SE_PARTY_READY = 0x14,    /* slot 1's sound 0x8C: the quest room's members reach the quest's least number, or the party is complete */
    LOBBY_SE_WINDOW_IN = 0x16,      /* slot 1's sound 0x8D: a menu's window slides in as it opens */
    LOBBY_SE_WINDOW_OUT = 0x17,     /* slot 1's sound 0x8E: a screen is left, its window slides out */
    LOBBY_SE_GALLERY_SHOT = 0x19,   /* the shooting gallery: a shot is fired */
    LOBBY_SE_GALLERY_DOWN = 0x1A,   /* the shooting gallery: a target is downed */
    LOBBY_SE_STAMP = 0x18,          /* slot 1's sound 0x8F: an entry of a level list is chosen and stamped (the area list) */
    LOBBY_SE_SELECT = 0x1B,         /* slot 1's sound 0x91: an entry is taken and its page or question opens (the workshop) */
    /* The list menus' sounds, slot 1's 0xA0-0xA4 (the price board's, the
     * quest counters'). */
    LOBBY_SE_LIST_OPEN = 0x2C,      /* a list menu opens */
    LOBBY_SE_LIST_CURSOR = 0x2D,    /* the list's cursor or page moves */
    LOBBY_SE_LIST_SELECT = 0x2E,    /* circle takes a line */
    LOBBY_SE_LIST_ENTRY = 0x2F,     /* a line's entry opens (a wanted entry's card) */
    LOBBY_SE_LIST_BACK = 0x30,      /* cross: back */
    LOBBY_SE_GALLERY_WALL = 0x37,   /* the shooting gallery: a shot hits a wall */
    LOBBY_SE_GALLERY_SWING = 0x38,  /* the shooting gallery: a target is hit but stays up */
    LOBBY_SE_PAGE = 0x3C,           /* slot 1's sound 0x98: the area list's page changes */
    LOBBY_SE_DESCRIPTION = 0x39,    /* SE_SYS_DESCRIPTION: a description shows or hides */
    LOBBY_SE_DECO_ATTACH = 0x3A,    /* the decorations counter: a decoration is attached */
    LOBBY_SE_DECO_REMOVE = 0x3B     /* the decorations counter: a decoration is removed */
};

/* A window's slide (WinSlide_Step, called once a frame): pos goes from
 * `from` towards `to` in 5 steps of `step`, overshoots `to` by 6, then
 * settles on it; WinSlide_Step returns 0 on the frame it settles, 2 before.
 * Most menus keep one at the head of their work area (gMenuWork) or of a
 * window inside it. */
typedef struct WinSlide {
    s16 pos;        /* 0x0: the window's x (or y) */
    s16 timer;      /* 0x2: steps left */
    s16 step;       /* 0x4: added each frame: (to - from) / 5 */
    s8 state;       /* 0x6: 0 start, 1 sliding, 2 settle */
} WinSlide;
typedef char WinSlide_size_check[(sizeof(WinSlide) == 8) ? 1 : -1];

/* What a menu screen's input returns (and WinSlide_Step: MENU_DONE once
 * settled, MENU_RUNNING before). */
#define MENU_DONE       0   /* finished (a yes/no: yes) */
#define MENU_UNDONE     1   /* finished without doing it (a user not added: FriendList_AddInput) */
#define MENU_RUNNING    2   /* still running */
#define MENU_CANCEL     3   /* left with cancel (a yes/no: no) */
#define MENU_CLOSE      4   /* left with PAD_START: the whole game menu closes */

/* LobbyMenu_MoveCursor's modes: the keys that move the cursor, and
 * whether it wraps round at the ends. */
#define MOVE_CURSOR_V_WRAP  0   /* up and down, wrapping */
#define MOVE_CURSOR_H_WRAP  1   /* left and right, wrapping */
#define MOVE_CURSOR_V       2   /* up and down, stopping at the ends */
#define MOVE_CURSOR_H       3   /* left and right, stopping at the ends */

/* HunterInfo_Open's flags: what the hunter info window offers besides the
 * hunter's details (HunterInfo_Update) */
#define HUNTER_INFO_MAIL    0x1 /* send a short mail */
#define HUNTER_INFO_FRIEND  0x2 /* add to the friend list */
#define HUNTER_INFO_JOIN    0x4 /* join the hunter */

/* A village resident's gift (gMenuWork while TOWN_MODE_NPC_GIFT runs):
 * NpcGift_Open fills it from the resident's gift tables, NpcGift_Update
 * gives it and clears the resident's record. */
typedef struct NpcGiftItem {
    u16 item;
    s16 count;
} NpcGiftItem;
typedef struct NpcGift {
    /* 0x0 */ NpcGiftItem give;     /* copied from the gift tables (gNpcGiftsByKind, gNpcMilestoneGifts) */
    /* 0x4 */ s16 record;           /* the resident's record in gSave.gifts */
    /* 0x6 */ s8 step;              /* NPC_GIFT_STEP_* */
} NpcGift;
enum NpcGiftStep {
    NPC_GIFT_STEP_TALK = 0,     /* the resident's words run */
    NPC_GIFT_STEP_GIVE = 1      /* the item goes into the pouch, then the gift closes */
};

/* The steps of the friend list's and the block list's add question
 * (LobbyMenu.subStep; FriendList_AddInput, BlockList_AddInput). */
enum UserAskStep {
    USER_ASK_FIRST = 0,     /* the first question */
    USER_ASK_CONFIRM = 1,   /* listed already or new: the second question */
    USER_ASK_DONE = 2,      /* the user is listed: PAD_CIRCLE ends it */
    USER_ASK_ENDED = 3      /* nothing added: PAD_CIRCLE ends it */
};

/* The steps of the question that takes a user off the mail block list
 * (LobbyMenu.subStep; BlockList_RemoveInput). */
enum BlockRemoveStep {
    BLOCK_REMOVE_ASK = 0,   /* the question */
    BLOCK_REMOVE_DONE = 1,  /* the user is taken off: PAD_CIRCLE ends it */
    BLOCK_REMOVE_KEPT = 2   /* the user stays listed: PAD_CIRCLE ends it */
};

/* A window's shake (Win_Shake): the offsets the draw adds for 5 frames. */
typedef struct WinShake {
    s16 dx;
    s16 dy;
    s16 timer;
    s8 state;
} WinShake;

/* Equipment indexes (u16 each), a set of six (sub_main data D_5D6BE6). */
typedef struct EquipIdx {
    u16 idx[6];
} EquipIdx;

/* A prize of the shooting gallery: an item and its points. */
typedef struct PrizeEntry {
    u16 item;
    u16 points;
} PrizeEntry;

/* A target setting of the shooting gallery (gGalleryCourseCfgs[course] lists them, the next one
 * applying from its points): two target kinds and how many of each. */
typedef struct ShootCfg {
    s16 points;     /* 0x0: the points from which it applies (< 0: the end of the list) */
    s8 kindA;       /* 0x2 */
    s8 nA;          /* 0x3 */
    s8 kindB;       /* 0x4 */
    s8 nB;          /* 0x5 */
} ShootCfg;
typedef char ShootCfg_size_check[(sizeof(ShootCfg) == 6) ? 1 : -1];

/*
 * gMenuWork while the bed's menu runs (TOWN_MODE_SAVE: TownSave_Open,
 * TownSave_Update, TownSave_Draw). Its three lines are the save, a rest
 * to the next half of the day and a sleep to the next day; the rest and
 * the sleep cost money when offline.
 */
enum BedLine {
    BED_LINE_SAVE = 0,      /* the save menu (TownNetMenu_Update) */
    BED_LINE_REST = 1,      /* the clock jumps to the end of the morning or of the afternoon */
    BED_LINE_SLEEP = 2,     /* the clock jumps to the end of the day */
    BED_LINE_COUNT = 3
};

/* TownSaveWork.step: TownSave_Update's steps. */
enum TownSaveStep {
    TOWN_SAVE_CHOOSE = 0,       /* the cursor on the three lines (online: straight to the save) */
    TOWN_SAVE_ASK = 1,          /* the save: offline, ask first; online, open the save menu */
    TOWN_SAVE_ASK_ANSWER = 2,   /* the question's yes/no */
    TOWN_SAVE_SAVING = 3,       /* the save menu runs (TownNetMenu_Update) */
    TOWN_SAVE_PAY_ANSWER = 4,   /* the rest's or the sleep's price: its yes/no */
    TOWN_SAVE_LYING_DOWN = 5,   /* paid: the hunter goes to bed, then the clock jumps */
    TOWN_SAVE_GETTING_UP = 6,   /* the menu has closed; the hunter gets up when the timer ends */
    TOWN_SAVE_DONE = 7
};

typedef struct TownSaveWork {
    /* 0x00 */ u8 step;             /* enum TownSaveStep */
    /* 0x01 */ u8 unk1;
    /* 0x02 */ s8 line;             /* the cursor: enum BedLine */
    /* 0x03 */ u8 unk3;
    /* 0x04 */ s16 timer;           /* frames left of the hunter's walk to or from the bed */
    /* 0x06 */ u8 unk6[2];
    /* 0x08 */ s32 prices[BED_LINE_COUNT]; /* each line's price (TownSave_Open) */
} TownSaveWork;

#define GALLERY_PRIZE_COUNT     13      /* the prizes of a list */
#define GALLERY_CHOSEN_MAX      0x28    /* prizes that can be chosen at once */
#define GALLERY_COURSE_COUNT    6
#define GALLERY_JINGLE_START    0x28    /* Jingle_Play: a round starts */
#define GALLERY_JINGLE_END      0x29    /* Jingle_Play: the time or the shots run out */
#define GALLERY_BONUS_POINTS    3500    /* a round's points that earn the Poogie's outfit 1 (EVENT_FLAG_POOGIE_OUTFIT_1) once */
#define GALLERY_AWARD_POINTS    4000    /* course 5's points that grant AWARD_GALLERY_COURSE_5 */

/* PrizeMenu.step: Gallery_Run's steps. */
enum GalleryStep {
    GALLERY_STEP_OPEN = 0,          /* marks the courses too dear, puts the window off screen */
    GALLERY_STEP_SLIDE_IN = 1,      /* the window slides in, then the courses (or the prizes) */
    GALLERY_STEP_COURSE = 2,        /* the course menu: PAD_CIRCLE pays and starts, PAD_SQUARE shows the prizes */
    GALLERY_STEP_COUNTDOWN = 3,     /* the countdown, then the board and its targets */
    GALLERY_STEP_SHOOT = 4,         /* the round */
    GALLERY_STEP_ROUND_END = 5,     /* a pause, then the score */
    GALLERY_STEP_SCORE = 6,         /* the score counts up (PAD_CIRCLE skips it); the event flag and the award */
    GALLERY_STEP_RESULT = 7,        /* the result window */
    GALLERY_STEP_PRIZES = 8,        /* the prize exchange */
    GALLERY_STEP_QUIT_ASK = 9,      /* the question whether to leave */
    GALLERY_STEP_CLOSE = 10         /* Gallery_Close */
};

/* PrizeMenu.shootStep: Gallery_ShootInput's steps. */
enum GalleryShootStep {
    GALLERY_SHOOT_READY = 0,
    GALLERY_SHOOT_AIM = 1,          /* aiming; PAD_R3 fires */
    GALLERY_SHOOT_FIRED = 2,
    GALLERY_SHOOT_IN_FLIGHT = 3     /* until no shot flies */
};

/* The shooting gallery's work area (gMenuWork): Gallery_Run's steps, the round,
 * its draw and the prize exchange. */
typedef struct PrizeMenu {
    /* 0x000 */ u8 step;                /* Gallery_Run's step (below 6: Gallery_BeforeShooting) */
    /* 0x001 */ u8 shootStep;           /* Gallery_ShootInput step: 0 start, 1 aim (pad 1 fires), 2, 3 the shot flying */
    /* 0x002 */ s8 course;              /* the course (gGalleryCourseTexts) */
    /* 0x003 */ s8 cursor;
    /* 0x004 */ s32 time;               /* seconds left (gGalleryCourseTimes[course]; Gallery_ShootInput counts it down) */
    /* 0x008 */ u8 details;             /* the prize's ItemInfo_Draw mode (pad 0x200 toggles it) */
    /* 0x009 */ u8 unk9[3];
    /* 0x00C */ struct StageObj *targets;   /* the targets' StageObj (Gallery_AddObj mode 0) */
    /* 0x010 */ struct StageObj *shots[10]; /* the shots in flight (Gallery_AddObj) */
    /* 0x038 */ f32 sideOffset[3];       /* (-500, 0, 0) turned by the camera's matrix at the round's start; not read in C */
    /* 0x044 */ u16 yaw;                /* the aim, left/right (Gallery_Aim; CamAim_Set) */
    /* 0x046 */ u16 pitch;              /* the aim, up/down */
    /* 0x048 */ s16 timer;              /* frames: the countdown, then to the next second of time */
    /* 0x04A */ s16 recoil;             /* the gun's kick: 30 when it fires, down 5 a frame while the shot flies */
    /* 0x04C */ PrizeEntry *list;       /* the 13 prizes */
    /* 0x050 */ u8 locked[0xE];         /* 1: the prize can't be chosen (too dear, no room in the pouch, all taken); drawn grey */
    /* 0x05E */ u16 chosen[GALLERY_CHOSEN_MAX]; /* the chosen prizes' items, given by Gallery_TakePrize */
    /* 0x0AE */ u8 chosenCount;         /* entries of chosen[] */
    /* 0x0AF */ u8 tooDear[7];          /* per course: 1 when its fee is above the points (Gallery_MarkTooDearCourses) */
    /* 0x0B6 */ s16 points;             /* the points earned */
    /* 0x0B8 */ s8 shotsLeft;           /* gGalleryCourseShots[course] at the start */
    /* 0x0B9 */ u8 shotsLive;           /* the shots in flight (Gallery_CountShots): the round ends at 0 */
    /* 0x0BA */ u8 padBA[2];
    /* 0x0BC */ f32 muzzle[3];          /* the gun's muzzle (Gallery_DrawGunDebug, GalleryMuzzle_Spawn) */
    /* 0x0C8 */ ShootCfg *cfg;          /* the target setting in use (gGalleryCourseCfgs[course] onwards) */
    /* 0x0CC */ u8 hitsB;               /* kindB targets hit under cfg */
    /* 0x0CD */ u8 shortClears;         /* targets cleared with fewer than nB kindB hits (2: the next cfg) */
    /* 0x0CE */ u8 restart;             /* 1: the next refill takes the course's first cfg */
    /* 0x0CF */ u8 lastCfg;             /* 1 past the cfg list's end */
    /* 0x0D0 */ u8 slots[15];           /* each target's kind (0: none; Gallery_PlaceTargets) */
    /* 0x0DF */ u8 order[0x11];         /* the 15 slots shuffled */
    /* 0x0F0 */ f32 spawnDir[3];        /* the targets' direction (GalleryBoard_Spawn) */
    /* 0x0FC */ u8 prizeView;           /* 1: the course menu shows the prizes (pad 0x200) */
    /* 0x0FD */ u8 kindHits[4];         /* the hits per target kind of the course's level */
    /* 0x101 */ u8 page;
    /* 0x102 */ u8 stacksChecked;       /* 1 once Gallery_MarkFullStacks ran: the prize window says the pouch is full */
    /* 0x103 */ u8 allTaken;            /* 1 once the 13 prizes are taken (Gallery_MarkAllTaken) */
    /* 0x104 */ WinSlide win;
    /* 0x10C */ u8 unk10C;
    /* 0x10D */ u8 toPrizes;            /* 1 after a paid course's score: the window opens on the prizes */
    /* 0x10E */ u8 played;              /* 1 once a course is paid for and started: Gallery_TakePrize then charges its fee */
    /* 0x10F */ u8 playedCourse;        /* that course (gGalleryCourseFees) */
    /* 0x110 */ u8 pouchFull;           /* 1 once the course menu said the pouch is full */
    /* 0x111 */ u8 skipped;             /* 1 when the score was skipped (pad 0x20) */
    /* 0x112 */ u8 pad112[2];
} PrizeMenu;
typedef char PrizeMenu_size_check[(sizeof(PrizeMenu) == 0x114) ? 1 : -1];

/* Gallery_AddObj's modes: what the object is to the gallery. */
#define GALLERY_OBJ_TARGETS     0   /* the target board (PrizeMenu.targets) */
#define GALLERY_OBJ_SHOT        1   /* a shot in flight (PrizeMenu.shots) */

/* An item list window: the shop's buy window over a townsperson's stock, its
 * sell window over the pouch, and the combine shop's lists. A page holds
 * SHOP_PAGE_ROWS items. */
#define SHOP_PAGE_ROWS 7
typedef struct ItemListEnt {
    u16 id;
    u16 price;      /* the price of one: buying (Shop_GetBuyPrice) or selling (Item_GetSellPrice) */
} ItemListEnt;
typedef struct ItemListWin {
    WinSlide win;   /* 0x00 */
    ItemListEnt ent[SHOP_PAGE_ROWS];
    s32 count;      /* 0x24: the count chosen (Shop_BuyAmountInput) */
    u32 color;      /* 0x28 */
    u16 kind;       /* 0x2C: the list's use (Shop_StartBuy) */
    s16 page;       /* 0x2E */
    s16 pages;      /* 0x30 */
    s16 infoPage;   /* 0x32: the ItemInfo_Draw page (attr), cycles 0..2 */
    s16 cur;        /* 0x34 */
    s16 n;          /* 0x36 */
    s16 backdrop;   /* 0x38: the picture behind the buy window, by the townsperson (Shop_StartBuy, Shop_DrawBuy) */
    s8 state;       /* 0x3A: enum ShopListStep */
    u8 pad3B;
    s8 infoMode;    /* 0x3C: the ItemInfo_Draw mode */
    s8 pouchHasRoom; /* 0x3D: the pouch has a free slot (SavePouch_HasFreeSlot): the buy prompt says so */
} ItemListWin;

/* The shop counter menu (gMenuWork, declared as bytes: read through a
   (ShopMenu *) cast MWCC keeps the address in a register, as the game does). */
typedef struct ShopMenu {
    WinSlide win;       /* 0x00 */
    ItemListWin list;   /* 0x08 */
    u8 *person;         /* 0x48: the townsperson's Em slot (ItemShop_Open) */
    char **lines;       /* 0x4C */
    s16 items[4];       /* 0x50: what each line does (enum ShopMenuLine) */
    u32 color;          /* 0x58 */
    s16 cur;            /* 0x5C */
    s16 n;              /* 0x5E */
    u16 npc;            /* 0x60: the townsperson's id (NpcWork id) */
    s8 state;           /* 0x62: enum ShopMenuStep */
} ShopMenu;

/* An entry of a townsperson's conditional stock table (Shop_BuildStock):
 * the townsperson sells `items` (a list ending with item 0) when
 * Cond_Check(cond) holds. The first entry whose condition holds is used. */
typedef struct ShopStockCond {
    u16 cond;
    u16 *items;
} ShopStockCond;

/* ShopMenu.state: the shop counter's steps (ItemShop_Update). */
enum ShopMenuStep {
    SHOP_MENU_OPEN = 0,         /* sets the menu's lines (ItemShop_SetRows) */
    SHOP_MENU_CHOOSE = 1,       /* the menu */
    SHOP_MENU_BUY = 2,          /* the buy window (Shop_BuyStep) */
    SHOP_MENU_SELL = 3,         /* the sell window (Shop_SellStep) */
    SHOP_MENU_TALK_END = 4      /* the townsperson's closing talk, then the menu closes */
};

/* ShopMenu.items: what a line of the shop's menu does. */
enum ShopMenuLine {
    SHOP_LINE_BUY = 1,
    SHOP_LINE_SELL = 2,
    SHOP_LINE_GALLERY = 3,      /* the shooting gallery (NPC_GALLERY_KEEPER's menu) */
    SHOP_LINE_OFFERS = 4        /* the townsperson's offers (OfferMenu_Open) */
};

/* ItemListWin.state: the shop's buy and sell windows' steps (Shop_BuyStep,
 * Shop_SellStep). */
enum ShopListStep {
    SHOP_STEP_SLIDE_IN = 0,     /* the window slides in */
    SHOP_STEP_LIST = 1,         /* the list: an item is chosen */
    SHOP_STEP_COUNT = 2,        /* how many */
    SHOP_STEP_CONFIRM = 3       /* the price, then yes or no */
};

/* Icon_DrawMode's arrows beside a count that can go up or down. */
#define ICON_ARROW_DOWN 0x28
#define ICON_ARROW_UP 0x29

/* A hair style of the hair menu (menu 0x3C, the hair-style change): HairMenuWork.list.hair,
 * copied from D_007EB8C0 by HairMenu_LoadPage. */
typedef struct HairStyle {
    /* 0x00 */ s16 id;              /* -1: only the colour changes */
    /* 0x02 */ s16 cost;
    /* 0x04 */ char name[0x40];     /* name + 0x20: the one for gSave.gender != 1 */
} HairStyle;

/* an entry of the colour menu (func_006F2E80, func_006F3370): HairMenuWork.list.color */
typedef struct ColorEntry {
    /* 0x00 */ s16 equip;
    /* 0x02 */ s16 rank;            /* the hunter rank it needs */
    /* 0x04 */ u8 pad4[0x40];
} ColorEntry;

/* gMenuWork while the hairdresser's menu (HairMenu_*) or the colour menu
 * (unk_6F2E80.c) is open: the two share it, and list holds either's page */
struct Em;
typedef struct HairMenuWork {
    /* 0x000 */ WinSlide win;
    /* 0x008 */ WinSlide win2;
    /* 0x010 */ struct Em *npc;         /* the hairdresser (states 0x97-0x9B) */
    /* 0x014 */ union {
        u8 raw[6][0x44];
        HairStyle hair[6];           /* the hair menu's page (D_007EB8C0 records) */
        ColorEntry color[6];         /* the colour menu's page */
    } list;
    /* 0x1AC */ s16 sel;
    /* 0x1AE */ s16 count;
    /* 0x1B0 */ s16 page;
    /* 0x1B2 */ s16 pages;
    /* 0x1B4 */ s16 equip;
    /* 0x1B6 */ s16 repeat;
    /* 0x1B8 */ u8 r;
    /* 0x1B9 */ u8 g;
    /* 0x1BA */ u8 b;
    /* 0x1BB */ u8 unk1BB;
    /* 0x1BC */ s16 preset;
    /* 0x1BE */ s16 row;
    /* 0x1C0 */ s16 timer;              /* frames before the hairdresser starts */
    /* 0x1C2 */ s8 toggle;
    /* 0x1C3 */ s8 step;                /* HairMenu_Update's step */
    /* 0x1C4 */ s8 state;
    /* 0x1C5 */ s8 withNpc;             /* HairMenu_Open's second argument: 1 at the hairdresser */
    /* 0x1C6 */ s8 unk1C6;              /* set once her motion 0x43D reaches frame 118 */
} HairMenuWork;

/* gMenuWork during the arm wrestling contest: the talk (Contest_*, unk_6F7400.c) and the
 * match (unk_6F7AA0.c). */
typedef struct ContestMenu {
    /* 0x00 */ u8 state;
    /* 0x01 */ u8 phase;                /* the match: 3 playing, 5 result */
    /* 0x02 */ u8 step;
    /* 0x03 */ u8 next;                 /* the state after the message */
    /* 0x04 */ u8 unk4[2];
    /* 0x06 */ s8 timer;
    /* 0x07 */ s8 result;               /* 0 won, 1 draw, 2 lost (Minigame_DrawResult, TownMsg_SendContestResult) */
    /* 0x08 */ void *spot;              /* the player's spot, put back at the end */
    /* 0x0C */ s32 power[2];            /* per player, +4 per button press */
    /* 0x14 */ s8 toggle;               /* the button to press next: 0 circle, 1 cross */
    /* 0x15 */ u8 unk15[7];
    /* 0x1C */ struct Player *who;      /* the opponent Contest_Open was given (its index: gNetUsers) */
    /* 0x20 */ s16 timeLeft;            /* frames (Lobby_DrawTimeLeft) */
    /* 0x22 */ s16 tick;
    /* 0x24 */ s8 level;                /* gContestLevel (offline), the row of gContestOpponents; below 7 the talk is gContestTalkLow */
    /* 0x25 */ s8 wait;                 /* frames to the opponent next press */
    /* 0x26 */ u8 unk26[2];
    /* 0x28 */ f32 scale[2];            /* Contest_DrawButtons */
    /* 0x30 */ s16 bump;
    /* 0x32 */ s8 done;                 /* 1 once it ended (func_006F7A00) */
    /* 0x33 */ u8 unk33;
    /* 0x34 */ s8 *msg;                 /* its messages: gContestTalkLow (level below 7) or gContestTalkTop */
    /* 0x38 */ u8 round;                /* 1 on the first match, 2 after */
    /* 0x39 */ u8 fxSpawned;            /* Effect2C_Spawn done */
} ContestMenu;
typedef char ContestMenu_size_check[(sizeof(ContestMenu) == 0x3C) ? 1 : -1];

/* The desk a quest is posted from (MsgQuestPost.desk). */
enum QuestPostDesk {
    QUEST_POST_DESK_QUEST = 0,      /* the quest desk (QuestDesk_Post) */
    QUEST_POST_DESK_OFFICIAL = 1,   /* OfficialDesk */
    QUEST_POST_DESK_DOJO = 2,       /* the dojo */
    QUEST_POST_DESK_ARENA_CUP = 3   /* the arena cup's desk */
};

/* A quest posted to the lobby: message 650B (LobbyLevel2_SendQuestPost sends its 0x18 bytes;
 * Quest_PostStep). The posting menus keep it in their work area: OfficialDeskMenu,
 * the quest desk (QuestDesk.head), ArenaCupMenu at 0x0C, DojoMenu at 0x4C. */
typedef struct MsgQuestPost {
    /* 0x00 */ u16 quest;           /* the quest id */
    /* 0x02 */ s16 money;           /* the quest desk's wanted fee (QuestDesk.wantedFee) */
    /* 0x04 */ u16 area;            /* gScene.area */
    /* 0x06 */ s8 desk;             /* the desk posted from (QUEST_POST_DESK_*) */
    /* 0x07 */ char userId[7];      /* gMyUserId (6 bytes): LobbyLevel2_SendQuestPost fills it */
    /* 0x0E */ char name[9];        /* gMyName (8 bytes): LobbyLevel2_SendQuestPost fills it */
    /* 0x17 */ u8 pad17;
} MsgQuestPost;
typedef char MsgQuestPost_size_check[(sizeof(MsgQuestPost) == 0x18) ? 1 : -1];

/* The head of gMenuWork that the quest posting menus share (OfficialDeskMenu, QuestDesk):
 * Quest_PostStep's step and the posting it sends. */
typedef struct QuestPostHead {
    /* 0x00 */ u8 unk0[7];
    /* 0x07 */ s8 postStep;         /* Quest_PostStep's: 0 the quest's download, 1 the room list, 2 the entry, 3 the records, 4 the settings, 5 the posting (0x650B), 6, 7 0x6509; 8 an error shown */
    /* 0x08 */ u8 unk8[4];
    /* 0x0C */ MsgQuestPost req;         /* the posting (LobbyLevel2_SendQuestPost) */
} QuestPostHead;
typedef char QuestPostHead_size_check[(sizeof(QuestPostHead) == 0x24) ? 1 : -1];

/* A level-2 entry's posted quest as 650C returns it (0x20 bytes; Net_GetLevel2PostedQuest
 * copies it into gPostedQuests[id - 1]): the poster's MsgQuestPost and 8 bytes more. */
typedef struct PostedQuest {
    /* 0x00 */ u16 quest;           /* the quest id (0: none) */
    /* 0x02 */ u16 money;           /* the fee to join (gQuestBoardRows[BOARD_ROW_WANTED]; QuestCounter_CheckQuest compares it with gSave.money) */
    /* 0x04 */ u16 area;            /* the poster's area (the counter's own when gScene.area) */
    /* 0x06 */ u8 desk;             /* MsgQuestPost.desk: 3 the arena cup (gQuestBoardRows[BOARD_ROW_FREE][it]) */
    /* 0x07 */ char userId[7];      /* the poster's (MsgQuestPost) */
    /* 0x0E */ char name[9];
    /* 0x17 */ u8 pad17[0x20 - 0x17];
} PostedQuest;
typedef char PostedQuest_size_check[(sizeof(PostedQuest) == 0x20) ? 1 : -1];
extern PostedQuest gPostedQuests[5];

/* The drinking contest (gMenuWork while it runs: DrinkContest_*, Minigame_Update; unk_6F8CC0.c) */
typedef struct Minigame {
    void *npc;                  /* 0x00: the opponent NPC (DrinkContest_Open's npc; its +0x438 is its NpcWork) */
    u8 step;                    /* 0x04 */
    u8 mode;                    /* 0x05 */
    u8 playStep;                /* 0x06: mode 3's step: 0 keys, 1 drinking, 2 knocked down, 3 getting up, 4 the hit keys flying off */
    u8 unk7;                    /* 0x07 */
    s16 wait;                   /* 0x08: frames: from 90 to -9 before the start (mode 2), before the result (mode 7); Minigame_DrawResult's t */
    u8 level;                   /* 0x0A: the row of gMinigameLevelC8, gMinigameRollCounts, gMinigameRollWeights (DrinkContest_Open sets 4; from 4 the counts come from countTable) */
    u8 count;                   /* 0x0B */
    u8 dir[7];                  /* 0x0C */
    u8 phase[7];                /* 0x13 */
    u8 timer[7];                /* 0x1A */
    u8 pad21[3];
    s32 vx[7];                  /* 0x24 */
    s32 vy[7];                  /* 0x40 */
    f32 x[7];                   /* 0x5C */
    f32 y[7];                   /* 0x78 */
    u8 frame[8];                /* 0x94 */
    f32 scale[7];               /* 0x9C */
    u8 lastDir;                 /* 0xB8: the direction of the last run of keys (4: none yet) */
    u8 keyNext;                 /* 0xB9: the key to press next */
    s8 result;                  /* 0xBA: 0 the player won, 1 lost, 2 neither (the online end) */
    u8 hits;                    /* 0xBB: keys hit this round */
    u8 keysLeft;                /* 0xBC: keys still to press (plus count at each round) */
    u8 missed;                  /* 0xBD: 1 after a wrong key: the round ends */
    u8 playerMugs;              /* 0xBE: the player's mugs left (10; DrinkContest_DrawCounts) */
    s8 downWait;                /* 0xBF: frames knocked down (0x78) */
    u8 down;                    /* 0xC0: 1 while knocked down */
    u8 misses;                  /* 0xC1: missed rounds: the third knocks the player down */
    u8 inputFrames;             /* 0xC2: frames of input this round (quick: within 30 * quickSecs) */
    u8 npcMugs;                 /* 0xC3: the NPC's mugs left (10; DrinkContest_DrawCounts) */
    u8 inputOn;                 /* 0xC4: 1 once DrinkContest_Draw shows the keys: input counts from then */
    u8 padC5[3];
    f32 quickSecs;              /* 0xC8: gMinigameLevelC8[level]: a round within this many seconds drinks faster */
    s8 endWait;                 /* 0xCC: frames before the end (mode 5) */
    s8 unkCD;                   /* 0xCD */
    s8 shakeVel;                /* 0xCE: the missed key's shake (DrinkContest_ShakeIcon) */
    u8 keysShown;               /* 0xCF: 1 from the first round: DrinkContest_Draw shows the keys */
    u8 unkD0;                   /* 0xD0 */
    u8 cursor;                  /* 0xD1: the opening menu's cursor (DrinkContest_Update step 0) */
    s8 menuWait;                /* 0xD2: frames since the opening menu opened (input from 6) */
    u8 talkNext;                /* 0xD3: the talk step after the current line (DrinkContest_Update step 4: 3 close, 5 the win's lines, 6, 7) */
    u8 running;                 /* 0xD4: 1 from the end of the opening talk to the result (DrinkContest_IsOn) */
    u8 padD5[3];
    u8 *countTable;             /* 0xD8: 16 key counts (DrinkContest_GetLevel by the wins), read at random from level 4 */
} Minigame;
typedef char Minigame_size_check[(sizeof(Minigame) == 0xDC) ? 1 : -1];

/* The drop talk's state for a taken quest (QuestDesk_Cancel): the quest desk's
 * and the arena cup's desk's. */
typedef struct QuestDeskCancel {
    s8 step;        /* QuestDesk_Cancel's step */
    s8 hideSheet;   /* 1 hides the quest sheet (the draws return early) */
} QuestDeskCancel;

#define ARENA_CUP_WIN_X 0x104        /* the form's and the monster check's window x when shown */
#define ARENA_CUP_RECORD_X 0x140     /* the record's */
#define ARENA_CUP_WIN_X_OUT 0x280    /* the windows' x off the screen's right edge */

/* ArenaCupMenu.step: ArenaCup_Update's steps. */
enum ArenaCupStep {
    ARENA_CUP_STEP_START = 0,       /* a held quest's drop asked, or the first list, or the cup not open */
    ARENA_CUP_STEP_CHOOSE = 1,      /* the first list (ArenaCup_ChooseQuest) */
    ARENA_CUP_STEP_POST = 2,        /* the entry (ArenaCup_Post) */
    ARENA_CUP_STEP_DROP = 3,        /* the held quest's drop talk (QuestDesk_Cancel) */
    ARENA_CUP_STEP_BYE = 4,         /* the clerk's last line, then ArenaCup_Close */
    ARENA_CUP_STEP_RECORD_IN = 5,   /* the record slides in */
    ARENA_CUP_STEP_RECORD = 6,      /* the record, until PAD_CROSS */
    ARENA_CUP_STEP_MONSTERS_IN = 7, /* the monster check slides in */
    ARENA_CUP_STEP_MONSTERS = 8,    /* the monster check (ArenaCup_MonsterInput) */
    ARENA_CUP_STEP_REFUSED = 9,     /* the entry refused with a talk, then CHOOSE */
    ARENA_CUP_STEP_NOT_OPEN = 10,   /* the cup isn't open yet (event flag 0xC4): a talk, then BYE */
    ARENA_CUP_STEP_TERMS_UNMET = 11 /* the quest's order terms aren't met: a dialog until PAD_CIRCLE, then BYE */
};

/* ArenaCupMenu.choice: the first list's entries (gArenaCupTexts' last three). */
enum ArenaCupChoice {
    ARENA_CUP_ENTER = 0,            /* the entry */
    ARENA_CUP_MONSTERS = 1,         /* the kept monsters' check */
    ARENA_CUP_RECORD = 2,           /* the cup's total record */
    ARENA_CUP_CHOICE_COUNT = 3
};

/* The clerk's lines (Talk_Say) at the arena cup's desk. */
enum ArenaCupTalk {
    ARENA_CUP_TALK_BYE = 0x1,
    ARENA_CUP_TALK_NO_MONSTER = 0x8,    /* no monster is kept */
    ARENA_CUP_TALK_ENTERED = 0xB,
    ARENA_CUP_TALK_ASK_DROP = 0xC,      /* a quest is held: drop it? */
    ARENA_CUP_TALK_NOT_OPEN = 0xF,
    ARENA_CUP_TALK_NOT_ENTERED = 0x11,  /* the entry left or failed */
    ARENA_CUP_TALK_MORNING = 0x12       /* the cup runs in the afternoon (LobbyTime_IsPm) */
};

#define ARENA_CUP_MONSTER_X 0x11C    /* a kept monster's details window x when shown */

/* ArenaCupMenu.subStep while ArenaCup_MonsterInput runs. */
enum ArenaMonsterStep {
    ARENA_MON_LIST = 0,             /* the kept monsters (ArenaCupMenu.monster) */
    ARENA_MON_MENU_IN = 1,          /* the monster's menu slides in (ArenaCupMenu.entryWin) */
    ARENA_MON_MENU_OUT = 2,         /* and out */
    ARENA_MON_MENU = 3,             /* the monster's menu (enum ArenaMonsterAction) */
    ARENA_MON_DETAILS_IN = 4,       /* the details slide in (ArenaCupMenu.monsterWin) */
    ARENA_MON_DETAILS_OUT = 5,      /* and out */
    ARENA_MON_DETAILS = 6,          /* the details, until PAD_CROSS */
    ARENA_MON_RENAMING = 7,         /* ArenaCup_RenameMonster */
    ARENA_MON_RELEASE_ASK = 8       /* the release's yes/no */
};

/* ArenaCupMenu.cursor in the monster's menu. */
enum ArenaMonsterAction {
    ARENA_MON_SHOW_DETAILS = 0,
    ARENA_MON_RENAME = 1,
    ARENA_MON_RELEASE = 2,
    ARENA_MON_ACTION_COUNT = 3
};

/* Arena_DrawMonsterList's modes. */
enum ArenaListMode {
    ARENA_LIST_EMPTY = 0,           /* no monster is kept: the panel says so */
    ARENA_LIST_CURSOR = 1,          /* the list with the cursor's bar */
    ARENA_LIST_NO_CURSOR = 2        /* the list alone (a monster's menu or details are up) */
};

/* ArenaCupMenu.subStep while ArenaCup_Post runs. */
enum ArenaPostStep {
    ARENA_POST_FETCH_LIST = 0,      /* the level-2 server's quest list (LobbyLevel2_FetchList) */
    ARENA_POST_ENTER_ROOM = 1,      /* a free quest room entered (LobbyLevel2_EnterFree) */
    ARENA_POST_FETCH_RECORDS = 2,   /* LobbyLevel2_FetchRecords */
    ARENA_POST_SLIDE_IN = 3,        /* the entry form slides in */
    ARENA_POST_FORM = 4,            /* the entry form (ArenaCup_PostInput) */
    ARENA_POST_SEND_SETTINGS = 5,   /* LobbyLevel2_SendSettings */
    ARENA_POST_SEND_POST = 6,       /* LobbyLevel2_SendQuestPost */
    ARENA_POST_SEND_ACCEPT = 7,     /* LobbyLevel2_SendQuestAccept, then the quest is taken */
    ARENA_POST_ERROR = 8,           /* a server error's dialog, until PAD_CIRCLE */
    ARENA_POST_LEAVE_ROOM = 9       /* the form left: the quest room is left (LobbyStep_Level2Leave) */
};

/* The arena cup's entry form's rows (ArenaCupMenu.cursor while ArenaCup_PostInput
 * runs). The three settings are the room's records 1 to 3 (gLobbyRecs.rec), each
 * shown only when the quest list's arenaFormRows bit for it is set
 * (ArenaCup_IsFormRowOpen); the password's row only while the password is on. */
enum ArenaFormRow {
    ARENA_FORM_PASS_ON = 0,         /* the password on or off (gLobbyRecs.usePass) */
    ARENA_FORM_PASSWORD = 1,        /* the password (gLobbyRecs.pass) */
    ARENA_FORM_SETTING_1 = 2,       /* gLobbyRecs.rec[1] */
    ARENA_FORM_SETTING_2 = 3,       /* gLobbyRecs.rec[2]: when shown, its first choice sets the least members (rec[0], gQuestMinMembers) to 1, any other to 3 */
    ARENA_FORM_SETTING_3 = 4,       /* gLobbyRecs.rec[3] */
    ARENA_FORM_DECIDE = 5,
    ARENA_FORM_ROWS = 6
};

/* The arena cup's desk (ArenaCup_Open, gMenuWork; update ArenaCup_Update, draw
 * ArenaCup_Draw and ArenaCup_Draw2). */
typedef struct ArenaCupMenu {
    /* 0x00 */ u8 unk0[0xC];
    /* 0x0C */ MsgQuestPost req;         /* the posting (ArenaCup_Post) */
    /* 0x24 */ WinSlide win;        /* the form, the record, the monster check */
    /* 0x2C */ WinSlide entryWin;   /* the entry form (ArenaCup_DrawMonsterMenu) */
    /* 0x34 */ WinSlide monsterWin; /* a monster's details (ArenaCup_DrawMonster) */
    /* 0x3C */ QuestDeskCancel cancel; /* the drop talk's state (QuestDesk_Cancel) */
    /* 0x3E */ u8 unk3E[2];
    /* 0x40 */ s32 season;          /* LobbyWorld_GetSeason, at ArenaCup_Open */
    /* 0x44 */ s8 step;             /* ArenaCup_Update's (enum ArenaCupStep) */
    /* 0x45 */ s8 subStep;          /* the step of the function step runs */
    /* 0x46 */ s8 renameStep;       /* ArenaCup_RenameMonster's */
    /* 0x47 */ s8 postStep;         /* ArenaCup_PostInput's */
    /* 0x48 */ u8 unk48[2];
    /* 0x4A */ s16 choice;          /* the first list's cursor (enum ArenaCupChoice) */
    /* 0x4C */ s16 monster;         /* gSave.arenaEms' index */
    /* 0x4E */ s16 cursor;          /* the form's line, the cup kind (ArenaCup_IsFormRowOpen) */
    /* 0x50 */ u8 unk50[2];
    /* 0x52 */ s8 dropAsked;        /* 1 once the drop of a held quest is asked (ArenaCup_Update); nothing reads it */
    /* 0x53 */ s8 monsterCount;     /* the monsters kept in gSave.arenaEms */
} ArenaCupMenu;
typedef char ArenaCupMenu_size_check[(sizeof(ArenaCupMenu) == 0x54) ? 1 : -1];

/* What each of the official desk's three rows lists (OfficialDeskMenu.row). */
enum OfficialRow {
    OFFICIAL_ROW_EVENTS = 0,        /* the downloaded event quests */
    OFFICIAL_ROW_GATE = 1,          /* the next quest of the HR gate this desk offers */
    OFFICIAL_ROW_RAID = 2,          /* the elder dragon raid's quests, while a raid runs */
    OFFICIAL_ROW_NONE = 3           /* nothing: the row can't be chosen */
};
#define OFFICIAL_ROWS 3
#define OFFICIAL_ROW_QUESTS 9       /* the quests a row lists */

/* OfficialDeskMenu.state (OfficialDesk_Update). */
enum OfficialDeskStep {
    OFFICIAL_STEP_OPEN = 0,         /* the rows window slides in */
    OFFICIAL_STEP_ROWS = 1,         /* the row (OfficialDesk_ChooseRow) */
    OFFICIAL_STEP_ROWS_OUT = 2,     /* the rows window slides out */
    OFFICIAL_STEP_CHOOSE = 3,       /* the quest sheet (OfficialDesk_ChooseQuest) */
    OFFICIAL_STEP_CHECK = 4,        /* the checks before the quest is taken (OfficialDesk_CheckOrder) */
    OFFICIAL_STEP_POST = 5,         /* the room settings, then the posting (OfficialDesk_Post) */
    OFFICIAL_STEP_STAMP = 6,        /* the stamp lands on the sheet, then the quest is accepted */
    OFFICIAL_STEP_ACCEPTED = 7,     /* the clerk's words, then the desk closes */
    OFFICIAL_STEP_ROWS_BACK = 8,    /* after a cancel: the rows window slides back in */
    OFFICIAL_STEP_LEAVE = 9         /* the clerk's goodbye, then the desk closes */
};

/* The room settings' lines (the posting menus' cursor). */
enum RoomSetting {
    ROOM_SETTING_MEMBERS = 0,       /* the least members (gQuestMinMembers) */
    ROOM_SETTING_PASS_ON = 1,       /* the password on or off (gRoomPassOn) */
    ROOM_SETTING_PASSWORD = 2,      /* the password (gRoomPassword); skipped while it is off */
    ROOM_SETTING_DECIDE = 3
};
#define ROOM_SETTINGS 4
#define ROOM_MIN_MEMBERS_CHOICES 4  /* gQuestMinMembers' values, 0 to 3 */

/* A line of the dojo's point exchange: an item and its price in points
 * (gSave.points). A stock is a list of them ended by item 0. */
typedef struct PointShopLine {
    /* 0x0 */ u16 item;
    /* 0x2 */ u16 pad;
    /* 0x4 */ s32 price;
} PointShopLine;
/* The point exchange's six stocks (gPointShopStocks): PointShop_LoadPage
 * picks one by the game's mode and progress. */
enum PointShopStock {
    POINT_SHOP_OFFLINE = 0,             /* offline, the dojo's facility below level 2 */
    POINT_SHOP_OFFLINE_LV2 = 1,         /* offline, the facility at level 2 */
    POINT_SHOP_OFFLINE_CLEARED = 2,     /* offline, every offline drill cleared */
    POINT_SHOP_ONLINE = 3,              /* online, before Cond_Check(0x64) holds */
    POINT_SHOP_ONLINE_HR31 = 4,         /* online, once Cond_Check(0x64) holds (HR 31) */
    POINT_SHOP_ONLINE_CLEARED = 5,      /* online, the online chain cleared */
    POINT_SHOP_STOCKS = 6
};

/* The dojo's menu (DojoMenu.mode): the instructor's greeting, the offline
 * choice, the drills menu, the training menu, the goodbye. */
enum DojoMode {
    DOJO_MODE_GREET = 0,
    DOJO_MODE_CHOICE = 1,       /* offline: the drills, the training or the instructor's offers */
    DOJO_MODE_DRILLS = 2,       /* online, the menu opens here */
    DOJO_MODE_TRAINING = 3,
    DOJO_MODE_BYE = 4           /* the instructor's last words, then DojoMenu_Close */
};

/* The offline choice's lines (DojoMenu.choices): the drills only once
 * Cond_Check(10) holds, the offers only when the instructor has some. */
enum DojoChoice {
    DOJO_CHOICE_DRILLS = 0,
    DOJO_CHOICE_TRAINING = 1,
    DOJO_CHOICE_OFFERS = 2      /* OfferMenu_Open */
};

/* The drills menu's steps (DojoMenu.step and .next in DOJO_MODE_DRILLS). */
enum DrillsStep {
    DRILLS_STEP_CATEGORY = 0,           /* the categories: drills, the point exchange, the rules */
    DRILLS_STEP_KIND = 1,               /* hunts or arena */
    DRILLS_STEP_TALK = 2,               /* the instructor's words before the list */
    DRILLS_STEP_SLIDE_OUT = 3,          /* the menu's window slides out, then .next */
    DRILLS_STEP_QUESTS = 4,             /* the drills' quest sheet */
    DRILLS_STEP_BACK_FROM_SHOP = 5,     /* 5 to 7: the window slides back in, then .next */
    DRILLS_STEP_BACK_FROM_RULES = 6,
    DRILLS_STEP_BACK_FROM_QUESTS = 7,
    DRILLS_STEP_SHOP = 8,               /* the point exchange (PointShop_Input) */
    DRILLS_STEP_RULES = 9               /* the rules' pages */
};

/* The drills menu's categories (DojoMenu.cat in DOJO_MODE_DRILLS). */
enum DrillsCat {
    DRILLS_CAT_QUESTS = 0,
    DRILLS_CAT_EXCHANGE = 1,
    DRILLS_CAT_RULES = 2,
    DRILLS_CATS = 3
};

/* The drills' kinds (DojoMenu.drillKind), by the quest's map. */
enum DrillKind {
    DRILL_KIND_HUNT = 0,        /* any map but the two arenas */
    DRILL_KIND_ARENA = 1,       /* MAP_GREAT_ARENA and MAP_ARENA */
    DRILL_KINDS = 2
};

/* DojoMenu.state while the drills' categories are up. */
#define DRILLS_CATS_SLIDE_IN 0
#define DRILLS_CATS_INPUT 1

/* The training menu's steps (DojoMenu.step and .next in DOJO_MODE_TRAINING). */
enum TrainingStep {
    TRAINING_STEP_SLIDE_IN = 0,
    TRAINING_STEP_CATEGORY = 1,
    TRAINING_STEP_TALK = 2,             /* no step leads here */
    TRAINING_STEP_QUESTS = 3,           /* a training list's quest sheet */
    TRAINING_STEP_BASICS = 4,           /* the basics' pages */
    TRAINING_STEP_SLIDE_OUT = 5,        /* the window slides out, then .next */
    TRAINING_STEP_SLIDE_BACK = 6        /* the window slides back in, then .next */
};

/* The training menu's categories (DojoMenu.cat in DOJO_MODE_TRAINING): the
 * basics' pages, then three lists of training quests (gDojoTrainingQuests*);
 * the fourth shows once Cond_Check(3) holds. */
enum TrainingCat {
    TRAINING_CAT_BASICS = 0,
    TRAINING_CAT_QUESTS_1 = 1,
    TRAINING_CAT_QUESTS_2 = 2,
    TRAINING_CAT_QUESTS_3 = 3
};

/* The quest sheet's steps (DojoMenu.state, DojoMenu_QuestInput). */
enum DojoQuestStep {
    DOJO_QUEST_LIST = 0,
    DOJO_QUEST_TALK = 1,        /* the instructor's words on the quest picked */
    DOJO_QUEST_POST = 2,        /* online: the posting (DojoMenu_PostOnline) */
    DOJO_QUEST_STAMP = 3,       /* the stamp falls, then the quest is taken */
    DOJO_QUEST_TAKEN = 4,       /* a quest is already taken: the instructor says so */
    DOJO_QUEST_TERMS = 5,       /* the quest's order terms aren't met */
    DOJO_QUEST_RESET = 6        /* the question to clear the best score and time */
};

/* The point exchange's steps (DojoMenu.state, PointShop_Input). */
enum DojoShopStep {
    DOJO_SHOP_LIST = 0,
    DOJO_SHOP_COUNT = 1,        /* how many to take */
    DOJO_SHOP_CONFIRM = 2       /* the price's yes or no */
};

/* The online posting's steps (DojoMenu.postStep, DojoMenu_PostOnline). */
enum DojoPostStep {
    DOJO_POST_FETCH_LIST = 0,
    DOJO_POST_ENTER = 1,                /* enter the free quest level */
    DOJO_POST_FETCH_RECORDS = 2,
    DOJO_POST_SETTINGS = 3,             /* the room's settings (DojoMenu_SettingsInput) */
    DOJO_POST_SEND_SETTINGS = 4,
    DOJO_POST_SEND = 5,                 /* post DojoMenu.req */
    DOJO_POST_ACCEPT = 6,
    DOJO_POST_LEAVE = 7,                /* the settings were left: leave the level */
    DOJO_POST_ERROR = 8                 /* the server's error shows until circle closes the menu */
};

/* DojoMenu.settingsStep. */
#define DOJO_SETTINGS_LIST 0
#define DOJO_SETTINGS_PASSWORD 1        /* the password's keyboard */

#define DOJO_BASICS_PAGES 10            /* the basics' pages (gDojoBasicsPages) */
#define DOJO_RULES_TOPICS_ALL 11        /* the rules' topics; the last only online with Cond_Check(0x64), or offline at facility level 2 */
#define POINT_SHOP_MAX_QTY 99           /* the most of a line taken at once */

#define DOJO_LIST_MAX 12        /* the quests a list holds */
#define POINT_SHOP_ROWS 7       /* the exchange's lines on a page */

/* gMenuWork while the dojo's menu runs (TOWN_MODE_DOJO): DojoMenu_Open
 * fills it at the instructor, DojoMenu_Update runs it each frame. */
typedef struct DojoMenu {
    /* 0x00 */ WinSlide win;
    /* 0x08 */ WinShake shake;          /* the window's shake (Win_Shake) when a drill is stamped */
    /* 0x10 */ PointShopLine *shopStock;    /* the point exchange's stock (one of gPointShopStocks) */
    /* 0x14 */ PointShopLine shopLines[POINT_SHOP_ROWS]; /* the exchange's page shown */
    /* 0x4C */ MsgQuestPost req;        /* the online posting (LobbyLevel2_SendQuestPost) */
    /* 0x64 */ struct Em *npc;          /* the instructor */
    /* 0x68 */ s32 season;              /* LobbyWorld_GetSeason() at the opening */
    /* 0x6C */ s32 isPm;                /* LobbyTime_IsPm() at the opening */
    /* 0x70 */ u16 list[DOJO_LIST_MAX]; /* the quests listed */
    /* 0x88 */ s16 choices[4];          /* what each line of the first choice does (DOJO_CHOICE_*) */
    /* 0x90 */ s16 settingsCursor;      /* the posting settings' cursor (ROOM_SETTING_*) */
    /* 0x92 */ s16 choiceCursor;
    /* 0x94 */ s16 cat;                 /* the cursor in the categories */
    /* 0x96 */ s16 drillKind;           /* DRILL_KIND_* */
    /* 0x98 */ s16 cats;                /* the training categories: 3, or 4 once Cond_Check(3) holds */
    /* 0x9A */ s16 choiceCount;
    /* 0x9C */ s16 page;                /* the sheet's page, the rules' topic, the basics' page or the exchange's page */
    /* 0x9E */ s16 pages;               /* how many of them */
    /* 0xA0 */ s16 rulesPage;           /* the page within the rules' topic */
    /* 0xA2 */ s16 rulesPages;
    /* 0xA4 */ s16 questCount;
    /* 0xA6 */ s16 questCursor;
    /* 0xA8 */ s16 shopCount;           /* the lines on the exchange's page */
    /* 0xAA */ s16 shopQty;             /* how many to take */
    /* 0xAC */ s16 shopCursor;
    /* 0xAE */ s8 mode;                 /* DOJO_MODE_* */
    /* 0xAF */ s8 step;                 /* DRILLS_STEP_* or TRAINING_STEP_*, by the mode */
    /* 0xB0 */ s8 state;                /* the open screen's step: QUEST_STEP_*, SHOP_STEP_*, or 0 (sliding in) and 1 */
    /* 0xB1 */ s8 postStep;             /* DOJO_POST_* (DojoMenu_PostOnline) */
    /* 0xB2 */ s8 settingsStep;         /* 0 the settings, 1 the password's keyboard */
    /* 0xB3 */ s8 next;                 /* the step after a window slide */
    /* 0xB4 */ s8 stamped;              /* the stamp is drawn on the sheet */
    /* 0xB5 */ s8 showInfo;             /* PAD_SQUARE shows or hides the item's description (ItemInfo_Draw's mode) */
    /* 0xB6 */ s8 questAccepted;        /* the drill was taken (QuestDesk_Accept) */
    /* 0xB7 */ s8 questPicked;          /* a drill was picked and waits for its answer */
} DojoMenu;

/*
 * The Poogie's naming (gMenuWork while TOWN_MODE_POOGIE_PET runs:
 * PoogieNaming_Open, PoogieNaming_Update): the first petting of the
 * unnamed Poogie, then NPC_NAMING_TALKER's talk and the keyboard.
 */
typedef struct PoogieNaming {
    /* 0x0 */ u8 step;              /* NAMING_* */
    /* 0x1 */ u8 talk;              /* NAMING_TALK_*: which line the talk is at */
    /* 0x2 */ u8 pad2[2];
    /* 0x4 */ struct Em *poogie;
} PoogieNaming;
enum PoogieNamingStep {
    NAMING_WAIT = 0,            /* until the Poogie is free, then the petting starts */
    NAMING_PETTING = 1,
    NAMING_REACTION = 2,        /* the player's motion ends: a pleased Poogie goes on to the talk, else the naming closes */
    NAMING_ANGRY = 3,           /* the Poogie ran off: the naming stays open, the town mode is left as it is */
    NAMING_TALK = 4,
    NAMING_CLOSE = 5,
    NAMING_KEYBOARD = 6         /* the name is typed (Kbd_RunFiltered) */
};
enum PoogieNamingTalk {
    NAMING_TALK_ASK = 0,        /* the first question: yes goes on to the keyboard, no asks again */
    NAMING_TALK_NAME_IT = 1,    /* then the keyboard */
    NAMING_TALK_NAMED = 2,      /* the line with the name: yes ends the talk, no opens the keyboard again */
    NAMING_TALK_LAST = 3,       /* a closing line, then the naming closes */
    NAMING_TALK_ASK_AGAIN = 4,  /* yes: the keyboard; no: the refusal */
    NAMING_TALK_REFUSED = 5     /* the naming closes with the Poogie unnamed */
};

/*
 * A pet's menu (gMenuWork while TOWN_MODE_POOGIE_MENU, _POOGIE_OUTSIDE,
 * _POOGIE_CARE or _HOUSE_PET runs). The Poogie's menu offers petting,
 * feeding and (offline) the outfits; the menus outside the house and
 * of the care pet petting and feeding; the house pet only a petting,
 * kept apart in the house fields.
 */
#define PET_FEED_ROWS           7       /* the feeding list's rows a page */
#define PET_INPUT_DELAY         6       /* frames a step waits before it takes PAD_CIRCLE (PoogieMenu.inputDelay) */
#define POOGIE_OUTFIT_COUNT     5       /* outfit 0, always owned, and the four of EVENT_FLAG_POOGIE_OUTFIT_0..3 */
/* An outfit of the Poogie (gPoogieOutfits): only the name is read. */
typedef struct PoogieOutfit {
    /* 0x00 */ char name[0x18];
    /* 0x18 */ s32 eventFlag;       /* the event flag that gives it; -1 for outfit 0 */
    /* 0x1C */ s32 model;           /* its model (the town loads model 0xD + SaveData.poogieOutfit) */
} PoogieOutfit;
typedef struct PoogieMenu {
    /* 0x00 */ u8 step;             /* PET_STEP_* */
    /* 0x01 */ u8 feedStep;         /* PET_FEED_*: the feeding list */
    /* 0x02 */ u8 eatStep;          /* PoogieMenu_Feed: 0 the pet comes, 1 it eats */
    /* 0x03 */ u8 findStep;         /* FIND_*: the errand's find handed over (POOGIE_ERRAND_BACK) */
    /* 0x04 */ u8 choice;           /* PET_CHOICE_*: the choice list's cursor */
    /* 0x05 */ u8 outfitRow;        /* the outfit list's cursor, among the outfits owned */
    /* 0x06 */ u8 pad6[2];
    /* 0x08 */ struct Em *pet;            /* the Poogie or the care pet */
    /* 0x0C */ u8 owned[POOGIE_OUTFIT_COUNT]; /* 1: the outfit is owned (outfit 0 always) */
    /* 0x11 */ u8 pad11[3];
    /* 0x14 */ char *outfitNames[POOGIE_OUTFIT_COUNT]; /* the outfit list: the names of the outfits owned */
    /* 0x28 */ u8 outfitCount;
    /* 0x29 */ s8 petResult;        /* the petting's end (PETTING_*), -1 before one */
    /* 0x2A */ WinSlide win;        /* the feeding list's window, sliding in */
    /* 0x32 */ u8 pad32[2];
    /* 0x34 */ s32 items[POUCH_SLOTS]; /* the pouch's ingredients (Kitchen_IsIngredient) */
    /* 0x84 */ s32 prices[POUCH_SLOTS]; /* their prices: the points eating one adds to the pet's counter */
    /* 0xD4 */ u8 rows;             /* the list's rows, PET_FEED_ROWS a page */
    /* 0xD5 */ u8 cursor;           /* the row on the page */
    /* 0xD6 */ u8 chosenRow;        /* the row taken: the item is items[chosenRow + page * PET_FEED_ROWS] */
    /* 0xD7 */ s8 page;
    /* 0xD8 */ s8 pages;
    /* 0xD9 */ u8 showInfo;         /* PAD_SQUARE shows or hides the item's description (ItemInfo_Draw's mode) */
    /* 0xDA */ s16 eatTimer;        /* frames until the food is eaten */
    /* 0xDC */ u8 errandEarned;     /* two very joyful pettings in a row: the Poogie leaves on its errand */
    /* 0xDD */ s8 inputDelay;       /* frames since the step began, held at 6: the buttons wait for it; also the find's notice time */
    /* 0xDE */ u8 houseStep;        /* HOUSE_PET_* */
    /* 0xDF */ u8 padDF;
    /* 0xE0 */ struct Em *housePet;
    /* 0xE4 */ s8 houseResult;      /* the house pet's petting end (PETTING_*) */
    /* 0xE5 */ u8 padE5;
    /* 0xE6 */ s16 bars[4];         /* the debug editor's petting bars (PET_BAR_*), copied from gPetDebugBars */
    /* 0xEE */ u8 debugBar;         /* the bar the editor changes */
} PoogieMenu;
enum PetStep {
    PET_STEP_CHOOSE = 0,        /* the choice list */
    PET_STEP_RUN = 1,           /* the choice runs: the petting, the feeding list or the outfit list */
    PET_STEP_CLOSE = 2,         /* the reactions end, then the menu closes */
    PET_STEP_ANGRY = 3,         /* the pet ran off: the menu stays until the town takes it down */
    PET_STEP_SEND_OFF = 4,      /* the reaction ends: the pet follows, goes back to its route, or (the errand) leaves */
    PET_STEP_DIG = 5,           /* the counter filled: the pet goes digging */
    PET_STEP_DEBUG_BARS = 6,    /* the debug editor of the petting bars (PET_DEBUG_BARS) */
    PET_STEP_CARE_DUG = 7,      /* the care pet's petting after its dig */
    PET_STEP_CARE_DIG_AGAIN = 8, /* a very joyful one sends it digging again */
    PET_STEP_DUG = 10,          /* the Poogie's petting after its dig, outside the house */
    PET_STEP_COUNTER = 11       /* the petting's points were added: a full counter sends it digging */
};
enum PetChoice {
    PET_CHOICE_PET = 0,
    PET_CHOICE_FEED = 1,
    PET_CHOICE_OUTFIT = 2,      /* the Poogie's menu offline only */
    PET_CHOICE_DUG = 10         /* no choice: the petting after a dig (PET_STEP_DUG) */
};
enum PetFeedStep {
    PET_FEED_SLIDE = 0,         /* the window slides in */
    PET_FEED_LIST = 1,
    PET_FEED_EAT = 2,           /* PoogieMenu_Feed */
    PET_FEED_REACT = 3          /* the care pet: its reaction plays */
};
enum PoogieFindStep {
    FIND_JOY = 0,               /* the Poogie's great joy and jingle */
    FIND_FACE = 1,              /* it turns to the player */
    FIND_WAIT_IDLE = 2,
    FIND_GIVE = 3,              /* the find goes into the pouch */
    FIND_CLOSE = 4              /* after the notice's time the menu closes */
};
enum HousePetStep {
    HOUSE_PET_START = 0,
    HOUSE_PET_PETTING = 1,
    HOUSE_PET_CLOSE = 2,
    HOUSE_PET_ANGRY = 3,
    HOUSE_PET_PLEASED = 9       /* a very joyful petting: its notice, the first time an outfit */
};

/*
 * A petting is a timing game: the frames the player holds the pose
 * (Player.cookTimer) against four bars, in frames (gPetBars, the care
 * pet's gPetBarsCare, the debug editor's PoogieMenu.bars).
 */
enum PetBar {
    PET_BAR_WAIT = 0,           /* the frames before the joy */
    PET_BAR_JOY = 1,            /* the joy's frames; past them the pet is angered */
    PET_BAR_GREAT_JOY = 2,      /* the great joy's frames, centred in the joy's */
    PET_BAR_ANGER = 3           /* after the joy, the frames before the petting ends by itself */
};
/* The petting's result so far, kept in Player.cookTimer's neighbour
 * Player.cookResult (the spit roast's field). */
enum PetResult {
    PET_RESULT_NONE = 0,        /* too early */
    PET_RESULT_LATE = 1,        /* still too early after 180 frames */
    PET_RESULT_JOY = 2,
    PET_RESULT_ANGER = 3,
    PET_RESULT_GREAT_JOY = 4
};
/* Poogie_UpdatePetting's returns. */
enum PettingEnd {
    PETTING_NOTHING = 0,        /* ended too early: no reaction */
    PETTING_ANGRY = 1,          /* the pet ran off (PET_STATE_ANGRY) */
    PETTING_PLEASED = 2,        /* joy or great joy (PET_STATE_JOY, PET_STATE_GREAT_JOY) */
    PETTING_RUNNING = 3
};
/* Poogie_NotifyMood's events. */
enum PetNotice {
    PET_NOTICE_COUNTER = 0,     /* points added: how near the counter is to its limit */
    PET_NOTICE_STREAK = 1,      /* a very joyful petting in the Poogie's menu (gPoogiePetStreak) */
    PET_NOTICE_HOUSE_PET = 2,   /* the house pet's great joy */
    PET_NOTICE_DELIGHTED = 3
};
/* The pets' jingles (Jingle_Play). */
#define PET_JINGLE_START        4       /* a petting starts */
#define PET_JINGLE_JOY          5
#define PET_JINGLE_NOTHING      0xE
#define PET_JINGLE_ANGRY        0x10
#define PET_DEBUG_BARS          0x139   /* Debug_IsOn: the Poogie's menu edits the petting bars */

/*
 * The official desk's menu (OfficialDesk_Open; gMenuWork while
 * TOWN_MODE_OFFICIAL_DESK runs; OfficialDesk_Update, drawn by
 * OfficialDesk_Draw and OfficialDesk_Draw2): three rows, each a list of
 * quests, then the chosen quest's sheet and the room settings.
 */
typedef struct OfficialDeskMenu {
    /* 0x00 */ u8 unk0[7];
    /* 0x07 */ s8 postStep;         /* Quest_PostStep's (QuestPostHead) */
    /* 0x08 */ u8 unk8[4];
    /* 0x0C */ MsgQuestPost req;    /* the posting (Quest_PostStep) */
    /* 0x24 */ struct QuestInfo *quest; /* the quest chosen (Quest_GetDiscInfo) */
    /* 0x28 */ WinSlide win;        /* the rows window (its x slides) */
    /* 0x30 */ WinShake shake;      /* Win_Shake: dx, dy; its timer is the stamp's fall */
    /* 0x38 */ struct Em *npc;      /* the clerk */
    /* 0x3C */ s8 row[OFFICIAL_ROWS]; /* enum OfficialRow: row i lists i, or OFFICIAL_ROW_NONE */
    /* 0x3F */ u8 pad3F;
    /* 0x40 */ s16 cursor;          /* the row */
    /* 0x42 */ u16 list[OFFICIAL_ROWS][OFFICIAL_ROW_QUESTS]; /* by enum OfficialRow: quest ids, 0 ends */
    /* 0x78 */ s16 listCursor;      /* the cursor in the row's list */
    /* 0x7A */ s16 listCount;       /* the quests in the chosen row */
    /* 0x7C */ s16 sheetPage;       /* QUEST_SHEET_* */
    /* 0x7E */ s16 menuCursor;      /* the room settings' line: enum RoomSetting */
    /* 0x80 */ s8 state;            /* enum OfficialDeskStep */
    /* 0x81 */ s8 subStep;          /* the step within state */
    /* 0x82 */ s8 roomStep;         /* the room settings: 0 the lines, 1 the password's keyboard */
} OfficialDeskMenu;
typedef char OfficialDeskMenu_size_check[(sizeof(OfficialDeskMenu) == 0x84) ? 1 : -1];

/* The id search (IdSearch_Open, gMenuWork; IdSearch_Update): a hunter id typed on
 * the keyboard, then that hunter's info (HunterInfo_Open). */
typedef struct IdSearchMenu {
    /* 0x00 */ WinSlide win;
    /* 0x08 */ u8 unk08[0x28];
    /* 0x30 */ char id[8];          /* the hunter id typed (ID_SEARCH_ID_CHARS characters, Kbd_Run) */
    /* 0x38 */ s16 row;             /* enum IdSearchRow */
    /* 0x3A */ u8 pad3A[6];
    /* 0x40 */ s8 step;             /* enum IdSearchStep */
    /* 0x41 */ s8 inputStep;        /* enum IdInputStep */
} IdSearchMenu;
typedef char IdSearchMenu_size_check[(sizeof(IdSearchMenu) == 0x42) ? 1 : -1];
#define ID_SEARCH_ID_CHARS 6        /* a hunter id's characters, as typed and compared */

/* IdSearchMenu.step */
enum IdSearchStep {
    ID_SEARCH_FORM = 0,             /* IdSearch_InputId */
    ID_SEARCH_HUNTER = 1,           /* the hunter's info (HunterInfo_Update) */
    ID_SEARCH_NOTICE = 2,           /* a notice; circle closes the search (nothing sets it) */
};

/* IdSearchMenu.inputStep */
enum IdInputStep {
    ID_INPUT_SLIDE_IN = 0,
    ID_INPUT_ROWS = 1,
    ID_INPUT_KEYBOARD = 2,
    ID_INPUT_NOTICE = 3,            /* the id is too short or the player's own; circle goes back */
};

/* IdSearchMenu.row */
enum IdSearchRow {
    ID_SEARCH_ROW_ID = 0,
    ID_SEARCH_ROW_START = 1,
    ID_SEARCH_ROWS = 2,
};

/* The name search (NameSearch_Open, gMenuWork; NameSearch_Update): a name typed on
 * the keyboard, then the pages of the hunters found by it, and a found
 * hunter's info. */
struct NetUserFindByNameRec;
#define NAME_SEARCH_ROWS_PER_PAGE 10    /* the hunters a results page lists */
#define NAME_SEARCH_NAME_CHARS 8        /* the name's characters at most, as typed */
typedef struct NameSearchMenu {
    /* 0x00 */ WinSlide listWin;    /* the results (NameSearch_ShowResults slides it in) */
    /* 0x08 */ WinSlide formWin;    /* the form (NameSearch_Open slides it in) */
    /* 0x10 */ struct NetUserFindByNameRec *rows[NAME_SEARCH_ROWS_PER_PAGE]; /* the page's hunters */
    /* 0x38 */ char name[0x10];     /* the name typed (Chat_OpenKbd; empty: none yet) */
    /* 0x48 */ s16 row;             /* the form's row: enum NameSearchRow */
    /* 0x4A */ s16 cursor;          /* the cursor in rows */
    /* 0x4C */ s16 page;
    /* 0x4E */ s16 pageCount;
    /* 0x50 */ s16 count;           /* the hunters on the page */
    /* 0x52 */ s8 step;             /* enum NameSearchStep */
    /* 0x53 */ s8 inputStep;        /* enum NameInputStep on the form, enum NameResultsStep on the results */
} NameSearchMenu;
typedef char NameSearchMenu_size_check[(sizeof(NameSearchMenu) == 0x54) ? 1 : -1];

/* NameSearchMenu.step */
enum NameSearchStep {
    NAME_SEARCH_FORM = 0,           /* NameSearch_InputName */
    NAME_SEARCH_FINDING = 1,        /* LobbyUser_FindByName runs */
    NAME_SEARCH_RESULTS = 2,        /* NameSearch_ResultsInput */
    NAME_SEARCH_HUNTER = 3,         /* a found hunter's info (HunterInfo_Update) */
    NAME_SEARCH_FAILED = 4,         /* the search failed: the server's notice until circle */
};

/* NameSearchMenu.inputStep on the form */
enum NameInputStep {
    NAME_INPUT_SLIDE_IN = 0,
    NAME_INPUT_ROWS = 1,
    NAME_INPUT_KEYBOARD = 2,
    NAME_INPUT_NOTICE = 3,          /* no name was typed; circle goes back */
};

/* NameSearchMenu.inputStep on the results */
enum NameResultsStep {
    NAME_RESULTS_SLIDE_IN = 0,
    NAME_RESULTS_LIST = 1,
};

/* NameSearchMenu.row */
enum NameSearchRow {
    NAME_SEARCH_ROW_NAME = 0,
    NAME_SEARCH_ROW_START = 1,
    NAME_SEARCH_ROWS = 2,
};

/*
 * The area board's work (TOWN_MODE_AREA_BOARD; gMenuWork from AreaBoard_Open):
 * the board of an area that holds rooms lists the area's level-1 entries;
 * an open one is joined after its members are shown, a free one is opened
 * as the player's own with a recruiting text. The group board's work
 * (GroupBoardMenu) shares its layout up to listWin, since the join, the text
 * picker and the member panel run on both.
 */
#define AREA_BOARD_ROWS_PER_PAGE 10     /* the entries on a page of the list */
#define AREA_BOARD_MEMBERS_PER_PAGE 5   /* the members on a page of the members panel */
#define AREA_BOARD_MEMBERS 10           /* the members an entry's users fill (two pages) */
#define RECRUIT_TEXTS_PER_PAGE 11       /* the recruiting texts on a page of the text picker */
#define AREA_BOARD_PICK_FREE 1          /* AreaBoard_ListInput: a free entry was chosen (MENU_DONE: an open one) */
typedef struct AreaBoardMenu {
    /* 0x000 */ WinSlide joinWin;       /* the members panel AreaBoard_Join slides in from the right */
    /* 0x008 */ NetUser members[AREA_BOARD_MEMBERS]; /* the joined entry's users (LobbyStep_FetchLevel1Users); id[0] 0: none */
    /* 0xB20 */ s16 memberPage;         /* the members panel's page */
    /* 0xB22 */ s16 memberPages;        /* 1 or 2 */
    /* 0xB24 */ s16 memberCursor;       /* the cursor in the members */
    /* 0xB26 */ s16 memberCount;        /* the members on the page */
    /* 0xB28 */ s8 memberView;          /* enum MemberViewMode */
    /* 0xB29 */ s8 step;                /* enum AreaJoinStep (AreaBoard_Join) */
    /* 0xB2A */ s8 joinStep;            /* enum AreaJoinInputStep (AreaBoard_JoinInput) */
    /* 0xB2B */ u8 padB2B;
    /* 0xB2C */ s8 textSlots[16];    /* the room slots whose recruiting texts the picker shows (AreaBoard_LoadPage) */
    /* 0xB3C */ s16 textCount;          /* the texts in textSlots */
    /* 0xB3E */ s16 textCursor;         /* the cursor in textSlots */
    /* 0xB40 */ s16 textPage;
    /* 0xB42 */ s16 textPages;
    /* 0xB44 */ WinSlide listWin;       /* the entry list (slides in from the left) */
    /* 0xB4C */ WinSlide panelWin;      /* the recruit form's panel */
    /* 0xB54 */ WinShake stampShake;    /* the panel's shake under the stamp (Win_Shake) */
    /* 0xB5C */ s8 state;               /* enum AreaBoardState (AreaBoard_Update) */
    /* 0xB5D */ s8 recruitStep;         /* enum AreaRecruitStep (AreaBoard_RecruitInput) */
    /* 0xB5E */ u8 padB5E;
    /* 0xB5F */ s8 noticeNext;          /* the state circle returns to from AREA_BOARD_NOTICE */
    /* 0xB60 */ s32 entryNo;            /* the level-1 entry chosen, from 1 (gLobbyLevel1Entries[entryNo - 1]) */
    /* 0xB64 */ s16 listPage;           /* the entry list's page */
    /* 0xB66 */ s16 listPages;
    /* 0xB68 */ s16 formTimer;          /* frames left on the recruit form (gRecruitFrames at its opening) */
    /* 0xB6A */ s16 rowCursor;          /* the cursor on a page of the list */
    /* 0xB6C */ s16 row;                /* the recruit form's row: enum AreaRecruitRow */
    /* 0xB6E */ u16 stampSeed;          /* the stamp's random seed (Menu_DrawStamp) */
    /* 0xB70 */ char text[0x20];        /* the recruiting text chosen (AreaBoard_RecruitInput, AreaBoard_DrawRecruitForm) */
    /* 0xB90 */ s8 showForm;            /* the panel shows the recruit form (a free entry opened), else the joined entry's members */
    /* 0xB91 */ s8 stamped;             /* the stamp is drawn */
} AreaBoardMenu;
typedef char AreaBoardMenu_size_check[(sizeof(AreaBoardMenu) == 0xB94) ? 1 : -1];

/* AreaBoardMenu.state */
enum AreaBoardState {
    AREA_BOARD_SLIDE_IN = 0,        /* the list slides in */
    AREA_BOARD_SLIDE_OUT = 1,       /* the list slides out, then AreaBoard_Close */
    AREA_BOARD_LIST = 2,            /* AreaBoard_ListInput */
    AREA_BOARD_JOIN = 3,            /* AreaBoard_Join */
    AREA_BOARD_RECRUIT = 4,         /* AreaBoard_RecruitInput */
    AREA_BOARD_STAMP = 5,           /* the panel shakes under the stamp */
    AREA_BOARD_ENTER_TOWN = 6,      /* LobbyStep_DownloadQuests, then the town */
    AREA_BOARD_LEAVE = 7,           /* AreaBoard_LeaveEntry */
    AREA_BOARD_NOTICE = 8,          /* a failure's notice; circle goes to noticeNext */
};

/* AreaBoardMenu.step */
enum AreaJoinStep {
    AREA_JOIN_FETCH = 0,            /* LobbyStep_FetchLevel1Users */
    AREA_JOIN_SLIDE_IN = 1,
    AREA_JOIN_SLIDE_OUT = 2,        /* backed out: the panel leaves */
    AREA_JOIN_MEMBERS = 3,          /* AreaBoard_JoinInput */
    AREA_JOIN_ENTER = 4,            /* LobbyStep_EnterLevel1 */
};

/* AreaBoardMenu.joinStep */
enum AreaJoinInputStep {
    AREA_JOIN_INPUT_PAGE = 0,       /* the hints; left and right turn the member page */
    AREA_JOIN_INPUT_MEMBERS = 1,    /* the cursor over the members */
    AREA_JOIN_INPUT_HUNTER_CARD = 2, /* HunterInfo_Update */
    AREA_JOIN_INPUT_ASK = 3,        /* the join question (Menu_YesNoInput) */
};

/* AreaBoardMenu.recruitStep */
enum AreaRecruitStep {
    AREA_RECRUIT_OPEN = 0,          /* LobbyStep_Level1Open */
    AREA_RECRUIT_SLIDE_IN = 1,
    AREA_RECRUIT_FORM = 2,          /* the rows */
    AREA_RECRUIT_PICK_TEXT = 3,     /* the text picker */
    AREA_RECRUIT_SEND_COLOR = 4,    /* LobbyStep_Level1SetColor */
    AREA_RECRUIT_SEND_TEXT = 5,     /* LobbyStep_Level1Settings */
    AREA_RECRUIT_REFRESH = 6,       /* LobbyStep_RefreshLevel1Users */
    AREA_RECRUIT_NO_TEXT = 7,       /* no text picked: a notice, circle back to the form */
};

/* AreaBoardMenu.row */
enum AreaRecruitRow {
    AREA_RECRUIT_ROW_TEXT = 0,      /* circle opens the text picker */
    AREA_RECRUIT_ROW_CONFIRM = 1,   /* circle sends */
    AREA_RECRUIT_ROWS = 2,
};

/* gLobby->subStep2 in AreaBoard_LeaveEntry */
enum AreaLeaveStep {
    AREA_LEAVE_RELEASE = 0,         /* LobbyStep_Level1Leave */
    AREA_LEAVE_RELIST = 1,          /* LobbyStep_Level1List */
};

/* What a menu's member panel shows (AreaBoardMenu.memberView, GroupBoardMenu.memberView):
 * GroupBoard_DrawButtonHints draws the square's hint only at MEMBER_VIEW_HINTS. */
enum MemberViewMode {
    MEMBER_VIEW_HINTS = 0,          /* the button hints, the square's (the member details) with them */
    MEMBER_VIEW_CURSOR = 1,         /* the cursor over the members (also the group details' location page) */
    MEMBER_VIEW_HUNTER_CARD = 2,    /* a member's hunter card is open (HunterInfo_Open) */
};

/* A new group's settings as the group board sends them (GroupBoard_SendSettings, which
 * the group board runs on GroupBoardMenu.join). */
typedef struct RoomJoin {
    /* 0x0 */ s16 textIdx;          /* the recruiting text chosen: an index into the menu's textSlots */
    /* 0x2 */ s8 groupInfo[4];      /* sent by LobbyStep_Level1Settings: [0] the limit (GROUP_LIMIT_*), [1] the target quest (RoomData.quests), [2] and [3] zero */
    /* 0x6 */ s8 step;              /* enum RoomJoinStep (GroupBoard_SendSettings) */
} RoomJoin;

/* RoomJoin.step */
enum RoomJoinStep {
    ROOM_JOIN_SEND_COLOR = 0,       /* LobbyStep_Level1SetColor */
    ROOM_JOIN_SEND_SETTINGS = 1,    /* LobbyStep_Level1Settings */
    ROOM_JOIN_REFRESH = 2,          /* LobbyStep_RefreshLevel1Users */
};

/* A mail (0x92 bytes): the mail box's (gMailBox, sub_main 0x5C0540: the last 8, newest
 * first), the one being written (ShortMailMenu, sent as 6704 by LobbyStep_ShortMailSend, kept in
 * gShortMailMsg). */
typedef struct MailEntry {
    /* 0x00 */ u8 used;
    /* 0x01 */ char id[8];          /* the sender (the addressee in ShortMailMenu) */
    /* 0x09 */ char name[0x11];
    /* 0x1A */ char text[0x78];
} MailEntry;
typedef char MailEntry_size_check[(sizeof(MailEntry) == 0x92) ? 1 : -1];
#define MAIL_BOX_LEN 8              /* gMailBox's mails, the newest first */

/* The short mail form (gShortMailMenu, 0x4B4 bytes at 0x8BCD20: ShortMail_Open, ShortMail_Step;
 * not in gMenuWork, so another menu can stay open under it). */
/* ShortMail_Open's flags (ShortMailMenu.flags). */
#define SHORT_MAIL_TYPE_ID   1  /* the addressee's id is typed in: the form has three rows */
#define SHORT_MAIL_TO_HUNTER 4  /* the addressee is given: two rows */
#define SHORT_MAIL_SAVED     8  /* with no id given, the form starts from the last mail (gShortMailMsg); three rows */
typedef struct ShortMailMenu {
    /* 0x000 */ WinSlide win;
    /* 0x008 */ MenuTask layer[2];  /* its own draw tasks (OT_AddAt): ShortMail_Draw, ShortMail_Draw2 */
    /* 0x048 */ MailEntry mail;     /* the mail written */
    /* 0x0DA */ u8 padDA[2];
    /* 0x0DC */ u8 details[0x3CC];  /* the addressee's (LobbyUser_FetchDetails) */
    /* 0x4A8 */ s32 flags;          /* ShortMail_Open's: 8 the last mail again, 4 and 9 ... */
    /* 0x4AC */ s16 sel;            /* the form's row */
    /* 0x4AE */ s16 count;          /* its rows */
    /* 0x4B0 */ s16 sel2;           /* the action cursor (ShortMail_DrawActions) */
    /* 0x4B2 */ s8 step;            /* ShortMail_Step */
    /* 0x4B3 */ s8 state;           /* ShortMail_Input */
} ShortMailMenu;
typedef char ShortMailMenu_size_check[(sizeof(ShortMailMenu) == 0x4B4) ? 1 : -1];

/*
 * The area list (LOBBY_PHASE_AREA_LIST): the server's first level, whose
 * entries the game calls areas (level 0: gLobbyLevel0Entries). Its work is
 * gMenuWork (AreaList_Begin clears it): the entries grouped by kind into
 * pages of up to AREA_GROUP_LEN, a kind's entries past that on a page of
 * their own.
 */
#define AREA_KINDS 4            /* LevelEntry.kind of an area: 1 to 4 */
#define AREA_KIND_GROUPS 4      /* the kind whose areas hold groups (the group board); the others hold rooms (the area board) */
#define AREA_GROUP_LEN 7        /* the entries of one page */
#define AREA_GROUP_MAX 20       /* pages */
#define AREA_LIST_SLIDE_FRAMES 6 /* AreaList.wait's start: the panels' slide in */
#define AREA_LIST_SHAKE_FRAMES 5 /* AreaList.shake's start: the stamp's shake */
typedef struct AreaGroup {
    /* 0x00 */ s8 count;            /* its entries */
    /* 0x01 */ s8 kind;             /* their kind, from 0 (LevelEntry.kind - 1) */
    /* 0x02 */ u8 pad2[2];
    /* 0x04 */ s32 ids[AREA_GROUP_LEN]; /* their level-0 ids, from 1 */
} AreaGroup;
typedef struct AreaList {
    /* 0x00 */ s16 group;               /* the cursor's page */
    /* 0x02 */ s16 groupCount;          /* pages */
    /* 0x04 */ s16 index;               /* the cursor's entry on its page */
    /* 0x06 */ s16 wait;                /* the slide-in counter (AreaList_WaitSlide; the panels slide while it is >= 0) */
    /* 0x08 */ s16 shake;               /* frames left of the stamp's shake once an area is chosen */
    /* 0x0A */ s16 shakeX;              /* the shake's offsets (AreaList_DrawList) */
    /* 0x0C */ s16 shakeY;
    /* 0x0E */ s16 stampSeed;           /* the stamp's random pattern (Menu_DrawStamp) */
    /* 0x10 */ s32 kindCounts[AREA_KINDS]; /* the areas of each kind grouped so far */
    /* 0x20 */ AreaGroup groups[AREA_GROUP_MAX];
} AreaList;
typedef char AreaList_size_check[(sizeof(AreaList) == 0x2A0) ? 1 : -1];

/* AreaList_Run's steps (LobbyState.phaseStep). */
enum AreaListStep {
    AREA_LIST_SETUP = 0,            /* AreaList_Init */
    AREA_LIST_FETCH = 1,            /* LobbyStep_Level0List, then AreaList_Group */
    AREA_LIST_SLIDE_IN = 2,         /* AreaList_WaitSlide */
    AREA_LIST_INPUT = 3,            /* AreaList_Input */
    AREA_LIST_STAMP = 4,            /* the chosen area's stamp shakes */
    AREA_LIST_ENTER = 5,            /* LobbyLogin_EnterEntry enters the chosen area */
    AREA_LIST_QUIT_ASK = 6,         /* the quit question (QuitMenu_Update) */
    AREA_LIST_MESSAGE = 7,          /* a refusal's message waits for PAD_CIRCLE */
    AREA_LIST_ENTER_FAILED = 8      /* the server's error text waits for PAD_CIRCLE; then the list is fetched again */
};

/*
 * The move menu (TOWN_MODE_MOVE_MENU), in gMenuWork: a question whether to
 * move to another area or leave the lobby, then the areas' pages to choose
 * from, grouped as the area list groups them (AreaList_Group). Three panels
 * slide in from the right: the server's, the player's server name and the
 * areas' page (MoveMenu_DrawBack, MoveMenu_DrawFront).
 */
typedef struct MoveMenu {
    /* 0x000 */ AreaList list;      /* group: the page shown; index: the cursor; shake, shakeX, shakeY, stampSeed: the chosen area's stamp */
    /* 0x2A0 */ s8 step;            /* enum MoveMenuStep */
    /* 0x2A1 */ s8 pick;            /* enum MovePickStep (MoveMenu_PickInput) */
    /* 0x2A2 */ s16 slideStep;      /* the panels' slide: pixels per frame (MOVE_MENU_SLIDE_STEP) */
    /* 0x2A4 */ s16 slide;          /* frames of the slide left; below 0 the panels are in place */
    /* 0x2A6 */ u8 pad2A6[2];
    /* 0x2A8 */ s16 choice;         /* the question's cursor (MOVE_CHOICE_*) */
} MoveMenu;
typedef char MoveMenu_size_check[(sizeof(MoveMenu) == 0x2AC) ? 1 : -1];

#define MOVE_MENU_SLIDE_STEP 0x35   /* MoveMenu.slideStep: pixels per frame */

/* MoveMenu.step */
enum MoveMenuStep {
    MOVE_MENU_ASK = 0,              /* the question (LOBBY_TEXT_MOVE_ASK) */
    MOVE_MENU_PICK = 1,             /* the areas' pages (MoveMenu_PickInput) */
    MOVE_MENU_LEAVE_ASK = 2,        /* leave the lobby? (LOBBY_TEXT_LEAVE_ASK) */
    MOVE_MENU_QUIT = 3              /* the quit menu (QuitMenu_Update); nothing in the move menu sets it */
};

/* MoveMenu.choice: the question's lines */
#define MOVE_CHOICE_AREA 0          /* move to another area */
#define MOVE_CHOICE_LEAVE 1         /* leave the lobby */

/* MoveMenu.pick: the area pages' steps (MoveMenu_PickInput) */
enum MovePickStep {
    MOVE_PICK_SLIDE_IN = 0,         /* the panels slide in */
    MOVE_PICK_INPUT = 1,            /* the cursor moves over the pages */
    MOVE_PICK_STAMP = 2,            /* the chosen area's stamp shakes */
    MOVE_PICK_ENTER = 3,            /* the screen fades out and the area is entered */
    MOVE_PICK_MESSAGE = 99          /* a refusal's message waits for PAD_CIRCLE */
};

/*
 * The village board (TOWN_MODE_VILLAGE_BOARD), in gMenuWork: the
 * categories, a page of the chosen category's titles, and an article.
 */
#define VILLAGE_BOARD_CATEGORY_COUNT 5  /* gVillageBoardArticles' lists */
#define VILLAGE_BOARD_PAGE_ROWS 8       /* titles on a page */
#define VILLAGE_BOARD_X_OUT 0x280       /* the board window's x off the screen's right */
#define VILLAGE_BOARD_X_IN 0x11C        /* and in place */
#define VILLAGE_BOARD_ARTICLE_Y_OUT -0x140 /* the article window's y above the screen */
#define VILLAGE_BOARD_ARTICLE_Y_IN 0x50 /* and in place */
typedef struct VillageBoardMenu {
    /* 0x00 */ WinSlide boardWin;   /* the board's window, sliding in x */
    /* 0x08 */ WinSlide articleWin; /* the article's window, sliding in y */
    /* 0x10 */ s32 rows[VILLAGE_BOARD_PAGE_ROWS]; /* the page's articles, as indexes in the category */
    /* 0x30 */ s16 page;
    /* 0x32 */ s16 pageCount;
    /* 0x34 */ s16 rowCount;        /* the page's titles */
    /* 0x36 */ s16 category;        /* the chosen category, 0 to VILLAGE_BOARD_CATEGORY_COUNT - 1 */
    /* 0x38 */ s16 cursor;          /* the cursor's row on the page */
    /* 0x3A */ s16 categoryFirst;   /* the read-mark number of the category's first article (the articles of the categories before it) */
    /* 0x3C */ s8 step;             /* enum VillageBoardStep */
} VillageBoardMenu;
/* An article of the village board (gVillageBoardArticles). */
typedef struct VillageBoardArticle {
    /* 0x0 */ s32 cond;             /* a Cond_Check id that must be met (0: always) */
    /* 0x4 */ s32 eventFlag;        /* an event flag that must be set (0: none) */
    /* 0x8 */ char *title;
    /* 0xC */ char *text;
} VillageBoardArticle;
/* VillageBoardMenu.step (VillageBoard_Update) */
enum VillageBoardStep {
    VILLAGE_BOARD_SLIDE_IN = 0,     /* the board's window slides in */
    VILLAGE_BOARD_CATEGORIES = 1,   /* the categories (VillageBoard_TopInput) */
    VILLAGE_BOARD_TITLES = 2,       /* a page of titles (VillageBoard_ListInput) */
    VILLAGE_BOARD_ARTICLE_IN = 3,   /* the article's window slides in */
    VILLAGE_BOARD_ARTICLE = 4,      /* the article (VillageBoard_ArticleInput) */
    VILLAGE_BOARD_SLIDE_OUT = 5     /* the board's window slides out, then the board closes */
};

/* AreaList_Input's modes (LobbyState.mode at AREA_LIST_INPUT). */
#define AREA_LIST_MODE_LIST 0       /* the list takes the pad */
#define AREA_LIST_MODE_FRIENDS 1    /* the friend list is open (FriendList_Update) */

/* The quest counter's work, in gMenuWork: the room's members, its windows and its steps
 * (QuestCounter_Step; its draws read it too). */
typedef struct QuestCounterMenu {
    /* 0x000 */ NetUser members[4];     /* the room's members (LobbyStep_Level2Entry, Quest_DrawArenaTeams) */
    /* 0x470 */ WinSlide questWin;      /* QuestCounter_DrawQuestWindow */
    /* 0x478 */ WinSlide kindWin;       /* QuestCounter_ChooseArenaRole's (QuestCounter_DrawTeamChoice) */
    /* 0x480 */ WinSlide board[5];      /* QuestCounter_OpenWindows, QuestCounter_DrawBoard */
    /* 0x4A8 */ s32 joinRole;           /* LobbyLevel2_EnterWithPass's value: 0, or the arena cup role chosen */
    /* 0x4AC */ s16 delay;              /* QuestCounter_OpenWindows' frames between windows */
    /* 0x4AE */ s8 step;                /* enum QuestCounterStep */
    /* 0x4AF */ s8 subStep;             /* the step of the current choice or window opening */
    /* 0x4B0 */ u8 pad4B0;
    /* 0x4B1 */ s8 returnStep;          /* the step after QUEST_COUNTER_MESSAGE (enum QuestCounterStep) */
    /* 0x4B2 */ u8 pad4B2[2];
    /* 0x4B4 */ s8 arenaRole;           /* 2..4: the arena cup role chosen (QuestCounter_ChooseArenaRole; copied to gPlayerText.card.arenaRole) */
    /* 0x4B5 */ s8 cursor;              /* the team choice's cursor, 0-2 */
    /* 0x4B6 */ s8 quest;               /* the posted quest chosen (gPostedQuests index) */
    /* 0x4B7 */ s8 page;                /* the quest sheet's page */
    /* 0x4B8 */ s8 pages;               /* 2 for an arena cup quest (PostedQuest.desk 3), else 4 */
    /* 0x4B9 */ u8 pad4B9[3];
    /* 0x4BC */ s32 questId;            /* the chosen quest's id (gLobbyQuestId) */
} QuestCounterMenu;
typedef char QuestCounterMenu_size_check[(sizeof(QuestCounterMenu) == 0x4C0) ? 1 : -1];

/* The quest counter's steps (QuestCounterMenu.step; QuestCounter_Step). */
enum QuestCounterStep {
    QUEST_COUNTER_START = 0,        /* the sending message, then the room list */
    QUEST_COUNTER_FETCH_LIST = 1,   /* LobbyLevel2_FetchList: the five rooms */
    QUEST_COUNTER_OPEN_BOARD = 2,   /* QuestCounter_OpenWindows */
    QUEST_COUNTER_CHOOSE_QUEST = 3, /* QuestCounter_ChooseQuest */
    QUEST_COUNTER_FETCH_ROOM = 4,   /* LobbyStep_Level2Entry: the room's settings and members */
    QUEST_COUNTER_OPEN_QUEST = 5,   /* the quest window slides in */
    QUEST_COUNTER_VIEW_QUEST = 6,   /* QuestCounter_ViewQuest */
    QUEST_COUNTER_ASK_JOIN = 7,     /* the talk asks whether to join (Talk_Update) */
    QUEST_COUNTER_CHECK = 8,        /* QuestCounter_CheckQuest */
    QUEST_COUNTER_PASSWORD = 9,     /* QuestCounter_InputPassword: a locked room */
    QUEST_COUNTER_PREPARE = 10,     /* a downloaded quest's file, or the arena role */
    QUEST_COUNTER_CHOOSE_ROLE = 11, /* QuestCounter_ChooseArenaRole */
    QUEST_COUNTER_ENTER = 12,       /* LobbyLevel2_EnterWithPass, then QuestDesk_Accept */
    QUEST_COUNTER_ACCEPTED = 13,    /* the accepted message waits for PAD_CIRCLE */
    QUEST_COUNTER_DONE = 14,        /* closes the counter, the quest taken */
    QUEST_COUNTER_TALK_END = 15,    /* the talk's last line, then CLOSE */
    QUEST_COUNTER_CLOSE = 16,       /* closes the counter */
    QUEST_COUNTER_MESSAGE = 17,     /* a message waits for PAD_CIRCLE, then returnStep */
    QUEST_COUNTER_LEAVE_ROOM = 20   /* the room's quest changed: LobbyStep_Level2Leave */
};

/* The quest desk's entries (QuestDesk.entries; QuestDesk_HasEntry says
 * which are offered). */
enum QuestDeskEntry {
    QUEST_DESK_GUIDE = 0,       /* the field guide: the open map groups */
    QUEST_DESK_FORECAST = 1,    /* the elder's forecast */
    QUEST_DESK_BOARD = 2,       /* the quest board: the hub's shown quests */
    QUEST_DESK_BY_MAP = 3,      /* the quests by map group */
    QUEST_DESK_BY_TYPE = 4,     /* the quests by type */
    QUEST_DESK_ENTRY_COUNT = 5
};
/* Quest_CheckOrder's results: whether the taken quest can set out now. */
#define QUEST_ORDER_OK 0          /* it can */
#define QUEST_ORDER_WRONG_TIME 1  /* not in the town's season or time of day */
#define QUEST_ORDER_TERMS 2       /* its order terms aren't met (Quest_CheckOrderTerms) */
#define QUEST_DESK_LISTS 12     /* QuestDesk.lists: the lists QuestDesk_BuildLists can fill */
#define QUEST_LIST_LEN 12       /* a quest list's ids at most */
#define QUEST_MAP_GROUP_COUNT 7 /* gQuestMapGroups' entries */

/* The quest sheet's pages (QuestDesk.page, QuestBoard_Draw). */
#define QUEST_SHEET_OVERVIEW 0  /* Quest_DrawOverview */
#define QUEST_SHEET_TERMS 1     /* Quest_DrawTerms */
#define QUEST_SHEET_DETAILS 2   /* Quest_DrawDetails */

/* A quest's texts (Quest_GetText's second argument), as the quest sheet
 * prints them beside their labels. */
#define QUEST_TEXT_TITLE 0
#define QUEST_TEXT_MAIN_TARGET 1
#define QUEST_TEXT_SUB_TARGET_A 2
#define QUEST_TEXT_SUB_TARGET_B 3
#define QUEST_TEXT_SUCCESS 4    /* the success conditions */
#define QUEST_TEXT_FAILURE 5    /* the failure conditions */
#define QUEST_TEXT_CLIENT 6
#define QUEST_TEXT_DETAILS 7

/* The menus' text colours (Font_SetColor: 0x80 alpha, then blue, green and
 * red, 0x7F full). */
#define MENU_INK_WHITE 0x807F7F7F        /* a frame's headings */
#define QUEST_SHEET_INK 0x80101010       /* the paper sheets' text: near black */
#define QUEST_SHEET_LABEL_INK 0x80301010 /* the sheets' labels: dark blue */

/* The quest lists the desks offer (gQuestLists, built by QuestList_Build):
 * two sets of up to QUEST_SET_LEN quest ids, 0 ending a set. Set 1 is
 * NPC_QUEST_DESK_2's (online), set 0 every other desk's. The quest board
 * shows up to QUEST_BOARD_LEN of a set's quests (gQuestDeskShown). */
#define QUEST_SETS 2
#define QUEST_SET_LEN 60
#define QUEST_BOARD_LEN 12

/* The kinds of quest list QuestList_Build makes (Quest_IsOffered's and
 * ElderForecast_QuestAppears' listKind); QuestList_Rebuild picks them. */
enum QuestListKind {
    QUEST_LIST_VILLAGE = 0,         /* offline, in the village: set 0, the standard quests */
    QUEST_LIST_ONLINE = 1,          /* online: set 0, the online quests */
    QUEST_LIST_ONLINE_DESK_2 = 2,   /* online: set 1, NPC_QUEST_DESK_2's quests */
    QUEST_LIST_AWAY = 3             /* offline, away from the village (gSave.awayFromVillage): set 0, the standard quests */
};
#define QUEST_DESK_WIN_X 0x118      /* the desk window's x when shown */
#define QUEST_DESK_WIN_Y 0x3A       /* its y */
#define QUEST_SHEET_W 0x140         /* the quest sheets' and the desk window's width */
#define QUEST_SHEET_H 0x168         /* and height */
#define QUEST_FORM_CURSOR_COLOR 0xFF106060  /* the recruit form's arrows and chosen box */
#define QUEST_FORM_BOX_COLOR 0xFF606060     /* its password box otherwise */
#define QUEST_DESK_WIN_X_OUT 0x280  /* its x off the screen's right edge */
#define QUEST_DESK_WANTED_X 0x124   /* the wanted panel's x when shown */
#define QUEST_SHEET_PAGES 3     /* the quest sheet's pages (QuestDesk.page, OfferMenu's, the official desk's): overview, terms, details */

/* QuestDesk_Update's steps (QuestDesk.step); the draws switch on them too. */
enum QuestDeskStep {
    QUEST_DESK_STEP_START = 0,          /* greet, or ask to drop the taken quest */
    QUEST_DESK_STEP_SLIDE_IN = 1,       /* the window slides in */
    QUEST_DESK_STEP_TOP_MENU = 2,       /* the entries (QuestDesk_TopMenu) */
    QUEST_DESK_STEP_CHOOSE_LIST = 3,    /* by map group or by type: the list (QuestDesk_ChooseList) */
    QUEST_DESK_STEP_SLIDE_OUT = 4,      /* the window slides out */
    QUEST_DESK_STEP_CHOOSE_QUEST = 5,   /* the quest sheet (QuestDesk_ChooseQuest) */
    QUEST_DESK_STEP_CHECK_FEE = 6,      /* the order terms and the money (QuestDesk_CheckFee) */
    QUEST_DESK_STEP_POST = 7,           /* online: the room settings and the posting (QuestDesk_Post) */
    QUEST_DESK_STEP_WANTED_STAMP = 8,   /* the wanted stamp shakes */
    QUEST_DESK_STEP_STAMP = 9,          /* the accepted stamp shakes, then the quest is taken */
    QUEST_DESK_STEP_WAIT_TALK = 10,     /* the last line is said */
    QUEST_DESK_STEP_CLOSE = 11,         /* QuestDesk_Close */
    QUEST_DESK_STEP_DROP = 12,          /* the taken quest's drop talk (QuestDesk_Cancel) */
    QUEST_DESK_STEP_GUIDE = 13,         /* the field guide (QuestDesk_GuideInput) */
    QUEST_DESK_STEP_FORECAST = 14,      /* the forecast (QuestDesk_ForecastInput) */
    QUEST_DESK_STEP_SLIDE_BACK = 15     /* the window slides back in */
};

/* The desk's lines (Talk_Say), named by their role. */
enum QuestDeskTalk {
    QUEST_DESK_TALK_GREETING = 0x0,
    QUEST_DESK_TALK_BYE = 0x1,
    QUEST_DESK_TALK_LIST_CHOSEN = 0x2,
    QUEST_DESK_TALK_NO_FEE = 0x5,       /* asks to take a quest without a fee */
    QUEST_DESK_TALK_FEE = 0x6,          /* asks to take a quest, its fee filled in */
    QUEST_DESK_TALK_WANTED_FEE = 0x7,   /* asks to take a wanted quest, its two fees filled in */
    QUEST_DESK_TALK_NO_MONEY = 0x8,
    QUEST_DESK_TALK_LIST_EMPTY = 0x9,
    QUEST_DESK_TALK_ACCEPTED = 0xA,
    QUEST_DESK_TALK_ACCEPTED_MIN_MEMBERS = 0xB, /* online, with a minimum of members set */
    QUEST_DESK_TALK_ASK_DROP = 0xC,
    QUEST_DESK_TALK_ASK_DROP_PAID = 0xD, /* online, the quest paid: asked again */
    QUEST_DESK_TALK_DROPPED = 0xE,
    QUEST_DESK_TALK_DECLINED = 0x10
};

/* The quest desk's work area (gMenuWork while TOWN_MODE_QUEST_DESK runs:
 * QuestDesk_Open clears it): the desk's steps, its entries, the quest lists
 * and the window, the stamps and the posting the draws (QuestDesk_Draw,
 * QuestBoard_Draw and the guide pages) show. */
typedef struct QuestDesk {
    /* 0x000 */ QuestPostHead head;     /* online, the posting (QuestDesk_Post) */
    /* 0x024 */ struct QuestInfo *quest; /* the chosen or taken quest (Quest_GetDiscInfo, Quest_GetInfoAny) */
    /* 0x028 */ WinSlide win;           /* the desk's window, slid in and out */
    /* 0x030 */ u8 unk30[8];
    /* 0x038 */ WinShake stamp;         /* the accepted stamp's shake; its timer is the stamp's frame */
    /* 0x040 */ WinShake wantedStamp;   /* the wanted stamp's, shaken first for a wanted quest */
    /* 0x048 */ QuestDeskCancel cancel; /* the drop talk's state */
    /* 0x04A */ s16 entries[QUEST_DESK_ENTRY_COUNT]; /* the entries offered (QUEST_DESK_*), entryCount of them */
    /* 0x054 */ s16 entryCount;
    /* 0x056 */ u16 forecast[5];        /* the elder's forecast (ElderForecast_Load, ElderForecast_DrawPanel) */
    /* 0x060 */ s16 sel;                /* the row: the quest in the list, the map group in the guide, the room settings' row */
    /* 0x062 */ s16 groupCount;         /* the open map groups (QuestDesk_CountOpenGroups): the guide's rows */
    /* 0x064 */ s16 page;               /* the quest sheet's page: 0 overview, 1 terms, 2 details */
    /* 0x066 */ s16 unk66;
    /* 0x068 */ s16 wantedFee;          /* the wanted fee of the chosen quest (QuestDesk_GetWantedFee), 0 for another */
    /* 0x06A */ s8 step;                /* QuestDesk_Update's step (QUEST_DESK_STEP_*) */
    /* 0x06B */ s8 subStep;             /* the step of the list and quest choices, the fee check and the posting */
    /* 0x06C */ s8 roomStep;            /* QuestDesk_RoomSettings': 0 the rows, 1 the password's keyboard */
    /* 0x06D */ s8 unk6D;               /* the draws skip the quest sheet when it is 1; no code seen sets it */
    /* 0x06E */ s16 entrySel;           /* the entry at the top menu's cursor (an index into entries) */
    /* 0x070 */ s16 list;               /* the list shown (by map group or by type) */
    /* 0x072 */ s16 listCount;          /* the lists QuestDesk_BuildLists filled */
    /* 0x074 */ s8 season;              /* LobbyWorld_GetSeason (QuestDesk_Open): gSeasonNames' index, the frame's colours (gDecoFrameColors) */
    /* 0x075 */ s8 pm;                  /* LobbyTime_IsPm (QuestDesk_Open): gDaytimeNames' index */
    /* 0x076 */ s8 questSet;            /* the quest list set (gQuestLists): 1 at NPC_QUEST_DESK_2's desk, else 0 */
    /* 0x077 */ s8 hidePageNum;         /* 1 hides the sheet's page number (a quest taken up) */
    /* 0x078 */ u16 lists[QUEST_DESK_LISTS][QUEST_LIST_LEN]; /* the quest ids of each list; 0 ends a list */
    /* 0x198 */ s16 savedSel;           /* sel, kept while the room settings use it */
} QuestDesk;
typedef char QuestDesk_size_check[(sizeof(QuestDesk) == 0x19C) ? 1 : -1];

/* The town menus' shared work area (0xF00 bytes at 0x8BDC20): a menu's Open clears it
 * (memset 0xF00) and starts the window slide that most menus keep first (WinSlide_Step).
 * Most units declare it as bytes; these are the menus that live there. */
typedef union MenuWork {
    WinSlide win;
    PrizeMenu prize;
    ShopMenu shop;
    HairMenuWork hair;
    ContestMenu contest;
    NameSearchMenu nameSearch;
    IdSearchMenu idSearch;
    AreaBoardMenu areaBoard;
    AreaList areaList;
    MoveMenu move;
    VillageBoardMenu villageBoard;
    Minigame drink;
    QuestCounterMenu questCounter;
    u8 raw[0xF00];
} MenuWork;
typedef char MenuWork_size_check[(sizeof(MenuWork) == 0xF00) ? 1 : -1];

/* The result of a town minigame (the arm-wrestling contest's and the
 * drinking contest's result field; Minigame_DrawResult draws it). */
enum MinigameResult {
    MINIGAME_WIN,                   /* 0: the player won */
    MINIGAME_LOSE,                  /* 1 */
    MINIGAME_DRAW                   /* 2 */
};

/* The block list page's steps (BlockListMenu.state). */
enum BlockListStep {
    BLOCK_LIST_SLIDE_IN = 0,    /* the window slides in */
    BLOCK_LIST_LIST = 1,        /* the cursor moves over the list */
    BLOCK_LIST_DELETE_BOX = 2,  /* the delete box beside the list */
    BLOCK_LIST_ASK_REMOVE = 3   /* the yes/no on removing the user */
};

/* The block list page's work (gMenuWork while the game menu's mail block list page is open). */
typedef struct BlockListMenu {
    WinSlide win;
    s16 cursor;                 /* the selected row */
    s16 count;                  /* the users listed */
    s8 state;                   /* enum BlockListStep */
} BlockListMenu;

/* The field menu's place info page (gMenuWork while it is open). */
enum PlaceInfoStep {
    PLACE_INFO_STEP_SLIDE_IN = 0,   /* the window slides in */
    PLACE_INFO_STEP_SHOWN = 1       /* in place: PAD_CROSS goes back */
};
typedef struct PlaceInfoMenu {
    WinSlide win;               /* win.pos: the window's x as it slides */
    s8 step;                    /* enum PlaceInfoStep */
} PlaceInfoMenu;
#define PLACE_INFO_X 0x136          /* the window's x once in place */
#define PLACE_INFO_W 0x12C          /* the window's width */
#define PLACE_INFO_H_PLACE 0x44     /* the window's height: the place block alone */
#define PLACE_INFO_H_VILLAGE 0xAA   /* with the village's facilities */
#define PLACE_INFO_H_ONLINE 0xE2    /* with the server, gate and room */
#define ICON_SEASON_FIRST 0x2D      /* Icon_Draw's season marks: this + SEASON_* */
#define ICON_ONLINE_MARK 0x59       /* Icon_Draw's online marks: this + the mark (ONLINE_MARK_*; mark 0 has none) */
#define ICON_OFFER_MARK 0x54        /* the offer mark over a townsperson (NameTags_DrawNpcs) */
#define ICON_SYMBOL_COLOR 0x62      /* the square of a member's symbol colour after its name tag */
#define ICON_WEAPON_CLASS 0x48      /* Icon_DrawWeaponClass: this + the weapon class (HunterCard.weaponClass) */
#define ICON_WEAPON_CLASS_WIDE 0x81 /* the same icons drawn 1.2 times as wide */

/* An entry of the lobby's icon sheet (gIconSprites, Icon_Draw's spr): its
 * texture and its corners on it. */
typedef struct IconSprite {
    /* 0x0 */ s32 tex;      /* the texture handle */
    /* 0x4 */ UV uv0;       /* the icon's corners on it: the draws copy each one whole into a TexRect */
    /* 0x8 */ UV uv1;
} IconSprite;
typedef char IconSprite_size_check[(sizeof(IconSprite) == 0xC) ? 1 : -1];
#define ICON_PANEL_AREAS 0          /* the move menu's areas' panel */
#define ICON_PANEL_SERVER 1         /* a server panel (LobbySrv_DrawServerPanel) */
#define ICON_SERVER_WAIT 0x1B       /* the mark on the server panel while the server's name hasn't arrived */
#define ICON_CURSOR_BAR 0x6F        /* the bar under a list's cursor line (MoveMenu_DrawAreaList) */
#define ICON_UNREAD_MARK 0x63       /* beside a village board title not read yet */
/* LobbySprite_Draw's sprites (entries of gLobbySpriteUVs). */
#define LOBBY_SPRITE_QUEST_TAKEN 9  /* a member's quest is taken (QUEST_STATUS_TAKEN), in its category's colour (gQuestCategoryColors) */
#define LOBBY_SPRITE_QUEST_PAID 10  /* a member's quest is paid (QUEST_STATUS_PAID), likewise */
#define LOBBY_SPRITE_NONE 0x16      /* draws nothing */

/* The forges' armour upgrades (gArmorUpgradeRows): the levels of an upgrade
 * table's row (0 the cost of making the piece, then each upgrade), and the
 * materials an upgrade or a derivation needs at most (the stacks
 * ArmorUpgrade_GetMaterials fills). */
#define ARMOR_UPGRADE_LEVELS 7
#define ARMOR_UPGRADE_MATERIALS 4

/* A recipe of the forge's recipe list (gForgeRecipeList), as the forge
 * menus' recipes give it (ForgeRecipe.type and .id). */
typedef struct RecipeId {
    s32 type;   /* enum EquipType */
    s32 id;     /* the equipment's id; 0 ends the list */
} RecipeId;

/* The materials of an armour upgrade: an item, then its count and its
 * condition (FORGE_COND_*) for each level; level 0 is the cost of making the
 * piece from the one it derives from. A table ends with an item of 0. */
typedef struct ArmorUpgradeCost {
    u8 count;
    u8 cond;
} ArmorUpgradeCost;

typedef struct ArmorUpgradeRow {
    /* 0x0 */ u16 item;
    /* 0x2 */ ArmorUpgradeCost lv[ARMOR_UPGRADE_LEVELS];
} ArmorUpgradeRow;

/*
 * The workshop's menu (TOWN_MODE_WORKSHOP): the first bytes of gMenuWork
 * while it runs. Its top list chooses a counter (the weapon forge, the
 * armour forge, the decorations) or the townsperson's offers; the counter
 * keeps its own work after this header, at gMenuWork + FORGE_WORK_OFFSET,
 * and the armour forge reads shortList too.
 */
typedef struct WorkshopMenu {
    /* 0x0 */ u8 step;              /* enum WorkshopStep */
    /* 0x1 */ u8 choice;            /* the top list's cursor: enum WorkshopChoice */
    /* 0x2 */ s8 hasOffers;         /* 1: the townsperson has offers, listed as the last entry */
    /* 0x3 */ s8 shortList;         /* 1 for NPC_WORKSHOP_HELPER and NPC_WORKSHOP_HELPER_ONLINE: the make menu (gWorkshopTexts from WORKSHOP_TEXT_MAKE_MENU), one entry shorter */
    /* 0x4 */ u16 saved;            /* kept in the townsperson's NpcWorkshopWork.menuSave while the menu is closed */
    /* 0x6 */ s16 count;            /* entries in the top list */
} WorkshopMenu;

/* WorkshopMenu.step (Workshop_Update). */
enum WorkshopStep {
    WORKSHOP_STEP_TOP = 0,          /* the top list */
    WORKSHOP_STEP_COUNTER = 1,      /* the chosen counter's menus */
    WORKSHOP_STEP_FORGE = 2,        /* the forging scene starts */
    WORKSHOP_STEP_FORGE_WAIT = 3,   /* until it ends */
    WORKSHOP_STEP_RESULT = 4,       /* the counter's closing question */
    WORKSHOP_STEP_BYE = 5           /* the townsperson's last line, then the menu closes */
};

/* WorkshopMenu.choice: the top list's entries. */
enum WorkshopChoice {
    WORKSHOP_WEAPONS = 0,
    WORKSHOP_ARMOR = 1,
    WORKSHOP_DECOS = 2,
    WORKSHOP_OFFERS = 3             /* the full list's last entry when the townsperson has offers */
};

#define BAIT_MENU_ROWS 7   /* bait listed on a page */

/* BaitMenu.step: the menu's phases (BaitMenu_Update). */
enum BaitMenuStep {
    BAIT_STEP_GREET = 0,   /* the townsperson's greeting */
    BAIT_STEP_OPEN = 1,    /* the list's window slides in */
    BAIT_STEP_LIST = 2,    /* the bait list */
    BAIT_STEP_FISH = 3,    /* fishing with the chosen bait */
    BAIT_STEP_BYE = 4      /* the townsperson's last line, then the menu closes */
};

/* BaitMenu.fishStep. */
#define BAIT_FISH_READY 0  /* standing at the spot, before a cast */
#define BAIT_FISH_LINE_OUT 1  /* cast: the line is out until the player stands again */

/* The townsperson's lines (Talk_Say). */
#define BAIT_TALK_BYE 0
#define BAIT_TALK_GREET 1
#define BAIT_TALK_NO_BAIT 2
#define BAIT_TALK_OUT_OF_BAIT 3

/* The bait menu at the fishing spot (BaitMenu_*): gMenuWork while
 * TOWN_MODE_BAIT_MENU runs. */
typedef struct BaitMenu {
    /* 0x00 */ struct Em *npc;             /* the townsperson at the fishing spot */
    /* 0x04 */ WinSlide win;
    /* 0x0C */ u16 items[BAIT_MENU_ROWS]; /* the page's bait (BaitMenu_LoadPage) */
    /* 0x1A */ s8 step;             /* enum BaitMenuStep */
    /* 0x1B */ s8 fishStep;         /* BAIT_FISH_READY or BAIT_FISH_LINE_OUT */
    /* 0x1C */ s8 showInfo;         /* square toggles the bait's description (ItemInfo_Draw's mode) */
    /* 0x1D */ u8 pad1D;
    /* 0x1E */ s16 page;
    /* 0x20 */ s16 pages;
    /* 0x22 */ s16 count;           /* bait on this page */
    /* 0x24 */ s16 cursor;
    /* 0x26 */ u16 npcYaw;          /* the townsperson's heading when the fishing starts, turned back to after a catch */
    /* 0x28 */ s32 bait;            /* the chosen bait's item id */
    /* 0x2C */ s32 baitHeld;        /* how many of it are held, counted when chosen and after each catch */
} BaitMenu;

#define INGREDIENT_ROWS 7   /* ingredients listed on a page */

/* KitchenMenu.step in the ingredient list (IngredientList_Update). */
enum IngredientListStep {
    INGREDIENT_STEP_GREET = 0,      /* the cook's greeting */
    INGREDIENT_STEP_PROMPT = 1,     /* the cook's line before the list; cancel says goodbye */
    INGREDIENT_STEP_OPEN = 2,       /* the window slides in */
    INGREDIENT_STEP_LIST = 3,       /* the list */
    INGREDIENT_STEP_SHUT = 4,       /* the window slides out */
    INGREDIENT_STEP_BYE = 5         /* the cook's goodbye, then the list closes */
};

/* The house cook's lines (Talk_Say), in the kitchen and the ingredient list. */
#define COOK_TALK_GREET 0           /* the kitchen: the greeting before a dish */
#define COOK_TALK_BYE 1
#define COOK_TALK_TOO_FEW 2         /* the kitchen: fewer than two ingredients held */
#define COOK_TALK_ASK_LEAVE 3       /* the kitchen: asks whether to leave (said as a question) */
#define COOK_TALK_THANKS 4          /* the kitchen: after the meal */
#define COOK_TALK_SET_COURSE 5      /* the kitchen: the greeting before a set course */
#define COOK_TALK_PARTY 7           /* the kitchen: the diners' line, filled with their number */
#define COOK_TALK_LIST 10           /* the line before the list */
#define COOK_TALK_GREET_FIRST 0xB   /* the first of the greetings */
#define COOK_TALK_GREET_COUNT 15    /* greetings, one picked by gLobbyEnterRand */

#define KITCHEN_DINERS_MAX 4    /* the users at one table, the player included */

/* KitchenMenu.step in the kitchen (Kitchen_Update). */
enum KitchenStep {
    KITCHEN_STEP_SIT = 0,           /* the player sits down (to the motion's frame 30) */
    KITCHEN_STEP_COOK_WALK = 1,     /* the cook walks to the table */
    KITCHEN_STEP_GREET = 2,         /* the cook's greeting (a dish or the set course) */
    KITCHEN_STEP_PARTY = 3,         /* online with others: the diners' line */
    KITCHEN_STEP_LIST_IN = 4,       /* the ingredient list slides in */
    KITCHEN_STEP_PICK_FIRST = 5,    /* the first ingredient */
    KITCHEN_STEP_PICK_SECOND = 6,   /* the second ingredient, which must differ */
    KITCHEN_STEP_CONFIRM_IN = 7,    /* the confirm window slides in */
    KITCHEN_STEP_CONFIRM = 8,       /* yes: the season, the camera and the cooking lesson */
    KITCHEN_STEP_COOKING = 9,       /* until the lesson is done */
    KITCHEN_STEP_EAT = 10,          /* until the player sits again: the meal's effect */
    KITCHEN_STEP_REACT = 11,        /* until the player stands idle: the cook's thanks */
    KITCHEN_STEP_THANKS = 12,       /* that line, then the kitchen closes */
    KITCHEN_STEP_ASK_LEAVE = 13,    /* the cook asks whether to leave: yes says goodbye, no greets again */
    KITCHEN_STEP_BYE = 14,          /* the cook's goodbye; the player stands up */
    KITCHEN_STEP_CLOSE = 15
};

/* The table the meal is served at (KitchenMenu.spot, KitchenMeal.spot):
 * Npc_SetKitchenRoute and CamReq_Kitchen take it. Seats 0-15 are the
 * player's seat (gTakeSpotId). */
enum KitchenSpot {
    KITCHEN_SPOT_SEATS_0_1 = 0,
    KITCHEN_SPOT_SEATS_2_5 = 1,
    KITCHEN_SPOT_SEATS_6_15 = 2,
    KITCHEN_SPOT_AWAY = 3           /* AREA_HUB_0x57, and offline away from the village */
};

/* The kitchen's town message (TownMsg_SendKitchen), by its first byte. */
enum KitchenMsg {
    KITCHEN_MSG_OPEN = 0,           /* to each diner: opens the guest's menu */
    KITCHEN_MSG_ROUTE = 1,          /* to the room, with the spot: the cook walks there */
    KITCHEN_MSG_CHOSEN = 2,         /* to each diner, with the KitchenMeal */
    KITCHEN_MSG_DONE = 3            /* to the room, with the spot: the meal is over */
};

/*
 * The kitchen's menu work in gMenuWork: the kitchen (TOWN_MODE_KITCHEN)
 * and the ingredient list (TOWN_MODE_INGREDIENTS) share its list part. The
 * fields marked "the kitchen" are used by the kitchen alone.
 */
typedef struct KitchenMenu {
    /* 0x00 */ WinSlide win;        /* the list's window (its x slides) */
    /* 0x08 */ WinSlide confirmWin; /* the kitchen: the confirm window (its y slides) */
    /* 0x10 */ struct Em *cook;     /* the kitchen: the house cook (NPC_HOUSE_COOK) */
    /* 0x14 */ struct Player *player; /* the kitchen: the local player */
    /* 0x18 */ u8 dinerIds[KITCHEN_DINERS_MAX][LOBBY_USER_ID_LEN]; /* the kitchen: the diners' user ids, online */
    /* 0x38 */ u8 unk38[0x68 - 0x38];
    /* 0x68 */ s8 season;           /* the kitchen: LobbyWorld_GetSeason when the meal is confirmed */
    /* 0x69 */ u8 pad69;
    /* 0x6A */ u16 items[INGREDIENT_ROWS]; /* the page's ingredients (IngredientList_BuildPage) */
    /* 0x78 */ u16 firstPick;       /* the kitchen: the first ingredient chosen; 0 for a set course */
    /* 0x7A */ u16 secondPick;      /* the kitchen: the second ingredient chosen, or the set course (0-3) */
    /* 0x7C */ u16 rand;            /* the kitchen: a roll drawn at the start, which picks the taste line */
    /* 0x7E */ u8 pad7E[2];
    /* 0x80 */ s32 seatMask;        /* the kitchen: the diners' seats, one bit each */
    /* 0x84 */ s16 diners;          /* the kitchen: the diners, each of whom takes one of each ingredient; 0 in the ingredient list */
    /* 0x86 */ s16 cursor;
    /* 0x88 */ s16 count;           /* ingredients on this page */
    /* 0x8A */ s16 page;
    /* 0x8C */ s16 pages;
    /* 0x8E */ s16 timer;           /* the kitchen: online with others, the frames left to choose */
    /* 0x90 */ u8 setCourse;        /* the kitchen: 1 when the cook serves its set course */
    /* 0x91 */ u8 spot;             /* the kitchen: enum KitchenSpot */
    /* 0x92 */ s8 showInfo;         /* square toggles the ingredient's description (ItemInfo_Draw's mode) */
    /* 0x93 */ s8 step;             /* enum IngredientListStep in the ingredient list, enum KitchenStep in the kitchen */
    /* 0x94 */ s8 backStep;         /* the kitchen: the step a no in the confirm window returns to */
} KitchenMenu;

/* An ingredient of gKitchenIngredients' groups: a group ends at item 0,
 * the table of groups at NULL. */
typedef struct KitchenIngredient {
    /* 0x0 */ u16 item;
    /* 0x2 */ s8 stat[3][2];        /* per season (SEASON_*): the values of the group's two stats (gMealStatKinds) */
} KitchenIngredient;

/* The meal's stats (gMealStatKinds, the rows of gMealBonus). */
enum MealStat {
    MEAL_STAT_STAMINA = 0,
    MEAL_STAT_HEALTH = 1,
    MEAL_STAT_ATTACK = 2,
    MEAL_STAT_DEFENSE = 3,
    MEAL_STAT_NONE = 4              /* gMealStatKinds: the group's second stat is none */
};
#define MEAL_STATS 4

#define MEAL_LEVELS 6       /* a stat change's levels (Kitchen_EffectLevel), gMealBonus's columns */
#define MEAL_DISHES 16      /* the dishes, by (firstPick + secondPick) % 16: gDishScales, gMealNameTexts */
#define MEAL_STRONG_TASTE 21 /* a total gain or loss from which the taste line is the strongest */

/* gMealBonus: by stat, then by level, the bonus a meal gives. Eight zero
 * halfwords follow the table. */
typedef struct MealBonusTable {
    s16 bonus[MEAL_STATS][MEAL_LEVELS];
    s16 unused[8];
} MealBonusTable;

/* The taste lines (gMealTasteTexts' rows, one of four by the meal's roll). */
enum MealTaste {
    MEAL_TASTE_GREAT = 0,           /* gains only, MEAL_STRONG_TASTE or more */
    MEAL_TASTE_GOOD = 1,            /* gains only */
    MEAL_TASTE_MIXED_GOOD = 2,      /* gains above losses */
    MEAL_TASTE_NONE = 3,            /* no change */
    MEAL_TASTE_MIXED_BAD = 4,       /* losses as large as gains or larger */
    MEAL_TASTE_BAD = 5,             /* losses only */
    MEAL_TASTE_AWFUL = 6            /* losses only, MEAL_STRONG_TASTE or more */
};

/* The meal a host chose in the kitchen, as its message to the guests
 * carries it (Kitchen_Broadcast sends it, KitchenGuest_OnMealChosen keeps
 * it in gKitchenGuestMeal). */
typedef struct KitchenMeal {
    /* 0x0 */ s32 seatMask;         /* the seats' bits (NpcScene_Start's meal scene) */
    /* 0x4 */ s16 season;           /* LobbyWorld_GetSeason when chosen (Kitchen_EatDish) */
    /* 0x6 */ u16 firstPick;        /* the first ingredient; 0 for a set course */
    /* 0x8 */ u16 secondPick;       /* the second ingredient, or the set course when firstPick is 0 */
    /* 0xA */ u16 rand;             /* the roll Kitchen_EatDish and Kitchen_EatSetCourse use */
    /* 0xC */ s8 spot;              /* the host's spot, 0-3 (CamReq_Kitchen, Npc_SetKitchenRoute) */
    /* 0xD */ u8 padD[3];
} KitchenMeal;

/* gLobbyMenu->subStep in the kitchen guest's menu (KitchenGuest_Update). */
enum KitchenGuestStep {
    KITCHEN_GUEST_STEP_WAIT = 0,        /* the host chooses; the menu closes when the wait runs out */
    KITCHEN_GUEST_STEP_SLIDE_IN = 1,    /* the meal's window slides in */
    KITCHEN_GUEST_STEP_SHOW = 2,        /* the window is shown */
    KITCHEN_GUEST_STEP_SLIDE_OUT = 3,   /* the window slides out */
    KITCHEN_GUEST_STEP_COOK = 4,        /* the cooking scene */
    KITCHEN_GUEST_STEP_SERVE = 5,       /* until the serve motion: then the guest eats */
    KITCHEN_GUEST_STEP_EAT = 6          /* until the player stands again */
};


/*
 * The offer menu a townsperson opens when talked to (gOfferMenu, run in
 * TOWN_MODE_OFFER_MENU): up to OFFER_MENU_ENTRIES deliveries the townsperson
 * wants (story deliveries copied from gStoryDeliveries, or its village
 * request kept in SaveData.offers) and up to OFFER_MENU_ENTRIES quests it
 * gives.
 */
#define OFFER_MENU_ENTRIES 6
#define DELIVERY_WANTS 3            /* the item stacks a delivery wants */

/* A delivery (0x82 bytes): the items to bring, the points it gives the
 * townsperson, its title and its text. */
typedef struct Delivery {
    /* 0x00 */ ItemStack want[DELIVERY_WANTS];  /* item 0: none */
    /* 0x0C */ s16 points;
    /* 0x0E */ char title[0x1D];
    /* 0x2B */ char text[0x57];
} Delivery;

/* Where an entry of the offer menu comes from (OfferMenu.source and
 * questSource). */
#define OFFER_SRC_STORY 0       /* a story delivery (gStoryDeliveries) or a story quest */
#define OFFER_SRC_VILLAGE 1     /* the townsperson's village request (SaveData.offers) or its quest */

/* What a townsperson offers (OfferMenu_BuildList's result, OfferMenu.has). */
enum OfferHas {
    OFFER_HAS_NONE = 0,
    OFFER_HAS_BOTH = 1,         /* deliveries and quests */
    OFFER_HAS_DELIVERIES = 2,
    OFFER_HAS_QUESTS = 3,
    OFFER_HAS_STORY = 4         /* a probe found a story entry (the mark over the townsperson's head) */
};

/* OfferMenu.step: OfferMenu_Update's steps. */
enum OfferStep {
    OFFER_STEP_TALK = 0,        /* waits for the opening talk */
    OFFER_STEP_TOP = 1,         /* the top menu: deliveries or quests */
    OFFER_STEP_QUESTS = 2,      /* the quest list (OfferMenu_QuestList) */
    OFFER_STEP_DELIVERIES = 3,  /* the delivery list (OfferMenu_DeliveryList) */
    OFFER_STEP_BYE = 4          /* the goodbye talk, then the menu closes */
};

/* OfferMenu.listStep: the steps of both lists. */
enum OfferListStep {
    OFFER_LIST_SLIDE_IN = 0,    /* the window slides in */
    OFFER_LIST_CHOOSE = 1,      /* the cursor moves; circle asks to take the entry */
    OFFER_LIST_CONFIRM = 2,     /* the yes/no prompt */
    OFFER_LIST_STAMP = 3,       /* the window shakes and the sheet is stamped */
    OFFER_LIST_DONE = 4         /* waits for the closing circle */
};

/* OfferMenu_DrawDeliveryPrompt's prompts. */
enum DeliveryPrompt {
    DELIVERY_PROMPT_READY = 0,      /* the items are at hand */
    DELIVERY_PROMPT_MISSING = 1,    /* items are missing */
    DELIVERY_PROMPT_CONFIRM = 2     /* the yes/no to deliver */
};


typedef struct OfferMenu {
    /* 0x000 */ Delivery entries[OFFER_MENU_ENTRIES];
    /* 0x30C */ WinSlide win;
    /* 0x314 */ WinShake shake;     /* the windows' shake (Win_Shake): dx, dy */
    /* 0x31C */ struct Em *npc;     /* the townsperson the menu was built for */
    /* 0x320 */ s16 source[OFFER_MENU_ENTRIES];     /* each delivery's OFFER_SRC_* */
    /* 0x32C */ s16 story[OFFER_MENU_ENTRIES];      /* the gStoryDeliveries index of a story delivery */
    /* 0x338 */ s16 period[OFFER_MENU_ENTRIES];     /* gQuestPeriodTexts' index: 0 story, 1 village */
    /* 0x344 */ s16 tabs[OFFER_MENU_ENTRIES + 1];   /* the delivery sheet's tab labels; 0 ends them */
    /* 0x352 */ s16 questSource[OFFER_MENU_ENTRIES]; /* each quest's OFFER_SRC_* */
    /* 0x35E */ u16 quests[OFFER_MENU_ENTRIES];     /* the quest ids */
    /* 0x36A */ s16 questIsVillage[OFFER_MENU_ENTRIES]; /* 1 for the village's quest */
    /* 0x376 */ s16 questTabs[OFFER_MENU_ENTRIES + 1]; /* each quest's map group (Map_Group): its tab and map strip; 0 ends them */
    /* 0x384 */ s16 topCur;         /* the top menu's cursor: 0 deliveries, 1 quests */
    /* 0x386 */ s16 cur;            /* the list's cursor */
    /* 0x388 */ s16 page;           /* the quest sheet's page (QUEST_SHEET_PAGES) */
    /* 0x38A */ s16 listLen;        /* the length of the list shown: count or questCount */
    /* 0x38C */ u16 stampSeed;      /* a random nonzero seed set on a yes; the stamp shows once it is set */
    /* 0x38E */ u8 pad38E[2];
    /* 0x390 */ s8 listSel;         /* the list shown: 0 deliveries, 1 quests */
    /* 0x391 */ s8 has;             /* enum OfferHas */
    /* 0x392 */ s8 count;           /* the deliveries listed */
    /* 0x393 */ s8 questCount;      /* the quests listed */
    /* 0x394 */ s8 step;            /* enum OfferStep */
    /* 0x395 */ s8 listStep;        /* enum OfferListStep */
    /* 0x396 */ u8 pad396[2];
} OfferMenu;

/* One line of the equipment shop's page. */
typedef struct ShopLine {
    /* 0x00 */ EquipItem item;
    /* 0x0C */ s32 price;
    /* 0x10 */ u8 noBuy;           /* too expensive or the box is full */
    /* 0x11 */ u8 unk11[3];
} ShopLine;
/* The equipment shop (TOWN_MODE_EQUIP_SHOP), in gMenuWork: EquipShop_Open
 * sets it up for the townsperson npc. */
typedef struct EquipShop {
    /* 0x00 */ WinSlide win;
    /* 0x08 */ u8 *npc;             /* the townsperson of the shop (OfferMenu_Open) */
    /* 0x0C */ s8 state;
    /* 0x0D */ s8 canWear;
    /* 0x0E */ u8 unkE[2];
    /* 0x10 */ s8 unk10;
    /* 0x11 */ s8 tab;              /* with kind: the title (gEquipGroupTexts[tab * 5 + kind]) */
    /* 0x12 */ s8 kind;
    /* 0x13 */ u8 unk13[5];
    /* 0x18 */ s16 cursor;
    /* 0x1A */ u8 unk1A[4];
    /* 0x1E */ s16 view;             /* the page shown (EquipShop_Draw) */
    /* 0x20 */ char *msg;
    /* 0x24 */ s16 pages;
    /* 0x26 */ s16 page;
    /* 0x28 */ s16 detailPage;      /* the detail page shown (views 1 to 3), up to detailPages */
    /* 0x2A */ s16 detailPages;
    /* 0x2C */ s16 count;           /* equipment on sale */
    /* 0x2E */ s16 lines;           /* lines on the page */
    /* 0x30 */ ShopLine line[7];
    /* 0xBC */ EquipItem worn;      /* the equipment worn in the slot of the kind on sale (gSave.equip; type set when none) */
    /* 0xC8 */ EquipItem item;
    /* 0xD4 */ EquipItem shown;     /* the item whose model is loaded (EquipShop_TryOn) */
    /* 0xE0 */ s32 price;
    /* 0xE4 */ s8 hasOffers;        /* the townsperson also has deliveries or quests to offer (OfferMenu_BuildList) */
    /* 0xE5 */ s8 selling;
} EquipShop;

#endif /* LOBBY_MENU_H */
