#ifndef LOBBY_MENU_H
#define LOBBY_MENU_H

#include "common.h"
#include "menu_task.h"
#include "lobby_state.h"

/*
 * The village and town menus (lobby overlay).
 *
 * gTownMode (D_008BDC10) says which menu runs (Town_UpdateMode calls its
 * update); every menu keeps its state in the shared work area gMenuWork
 * (D_008BDC20, cleared when a menu opens: 0xF00 bytes by DojoMenu_Open), which
 * each menu's file views as its own struct (EquipShop, DojoMenu, PoogieMenu,
 * RecordMenu, PrizeMenu ...). A menu's draw callbacks are set with
 * LobbyMenu_SetDraw (gLobbyMenu's layers 0-3).
 *
 * Structs used by more than one file live here.
 */

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

typedef struct SkillView {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ s8 unk4;
    /* 0x05 */ u8 unk5[0xB];
    /* 0x10 */ u8 unk10;
    /* 0x11 */ s8 unk11;
} SkillView;

/* A prize of the shooting gallery (unk_70E2A0.c): an item and its points. */
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

/* The shooting gallery's work area (gMenuWork): Gallery_Run's steps, the round (unk_70CC10.c),
 * its draw and the prize exchange (unk_70E2A0.c). */
typedef struct PrizeMenu {
    /* 0x000 */ u8 step;                /* Gallery_Run's step (below 6: Gallery_BeforeShooting) */
    /* 0x001 */ u8 shootStep;           /* Gallery_ShootInput step: 0 start, 1 aim (pad 1 fires), 2, 3 the shot flying */
    /* 0x002 */ s8 course;              /* the course (gGalleryCourseTexts) */
    /* 0x003 */ s8 cursor;
    /* 0x004 */ s32 time;               /* seconds left (gGalleryCourseTimes[course]; Gallery_ShootInput counts it down) */
    /* 0x008 */ u8 details;             /* the prize's ItemInfo_Draw mode (pad 0x200 toggles it) */
    /* 0x009 */ u8 unk9[3];
    /* 0x00C */ struct StageObj *targets;   /* the targets' VObj (Gallery_AddObj mode 0) */
    /* 0x010 */ struct StageObj *shots[10]; /* the shots in flight (Gallery_AddObj) */
    /* 0x038 */ f32 unk38[3];
    /* 0x044 */ u16 yaw;                /* the aim, left/right (Gallery_Aim; CamAim_Set) */
    /* 0x046 */ u16 pitch;              /* the aim, up/down */
    /* 0x048 */ s16 timer;              /* frames: the countdown, then to the next second of time */
    /* 0x04A */ s16 unk4A;
    /* 0x04C */ PrizeEntry *list;       /* the 13 prizes */
    /* 0x050 */ u8 full[0xE];           /* a prize the pouch can't take more of */
    /* 0x05E */ u16 items[0x28];        /* the prizes chosen */
    /* 0x0AE */ u8 count;
    /* 0x0AF */ u8 tooDear[7];          /* per course: 1 when its fee is above the points (Gallery_MarkTooDearCourses) */
    /* 0x0B6 */ s16 points;             /* the points earned */
    /* 0x0B8 */ s8 shotsLeft;           /* gGalleryCourseShots[course] at the start */
    /* 0x0B9 */ u8 shotsLive;           /* the shots in flight (Gallery_CountShots): the round ends at 0 */
    /* 0x0BA */ u8 padBA[2];
    /* 0x0BC */ f32 muzzle[3];          /* the gun's muzzle (Gallery_DrawGunDebug, VMuzzle_Spawn) */
    /* 0x0C8 */ ShootCfg *cfg;          /* the target setting in use (gGalleryCourseCfgs[course] onwards) */
    /* 0x0CC */ u8 hitsB;               /* kindB targets hit under cfg */
    /* 0x0CD */ u8 shortClears;         /* targets cleared with fewer than nB kindB hits (2: the next cfg) */
    /* 0x0CE */ u8 restart;             /* 1: the next refill takes the course's first cfg */
    /* 0x0CF */ u8 lastCfg;             /* 1 past the cfg list's end */
    /* 0x0D0 */ u8 slots[15];           /* each target's kind (0: none; Gallery_PlaceTargets) */
    /* 0x0DF */ u8 order[0x11];         /* the 15 slots shuffled */
    /* 0x0F0 */ f32 spawnDir[3];        /* the targets' direction (VObj_Spawn) */
    /* 0x0FC */ u8 prizeView;           /* 1: the course menu shows the prizes (pad 0x200) */
    /* 0x0FD */ u8 kindHits[4];         /* the hits per target kind of the course's level */
    /* 0x101 */ u8 page;
    /* 0x102 */ u8 checked;
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

/* An item list window (the pouch's pages): 4-byte entries from +8. */
typedef struct ItemListEnt {
    u16 id;
    u16 unk2;
} ItemListEnt;
typedef struct ItemListWin {
    WinSlide win;   /* 0x00 */
    ItemListEnt ent[7];
    s32 count;      /* 0x24: the count chosen (Shop_BuyAmountInput) */
    u32 color;      /* 0x28 */
    u16 kind;       /* 0x2C: the list's use (Shop_StartBuy) */
    s16 page;       /* 0x2E */
    s16 pages;      /* 0x30 */
    s16 infoPage;   /* 0x32: the ItemInfo_Draw page (attr), cycles 0..2 */
    s16 cur;        /* 0x34 */
    s16 n;          /* 0x36 */
    s16 unk38;      /* 0x38 */
    s8 state;       /* 0x3A: Shop_BuyStep's step */
    u8 pad3B;
    s8 infoMode;    /* 0x3C: the ItemInfo_Draw mode */
    s8 unk3D;       /* 0x3D: the pouch has a free slot */
} ItemListWin;

/* The shop counter menu (gMenuWork, declared as bytes: read through a
   (ShopMenu *) cast MWCC keeps the address in a register, as the game does). */
typedef struct ShopMenu {
    WinSlide win;       /* 0x00 */
    ItemListWin list;   /* 0x08 */
    u8 *person;         /* 0x48: the townsperson's Em slot (ItemShop_Open) */
    char **lines;       /* 0x4C */
    s16 items[4];       /* 0x50: what each line does */
    u32 color;          /* 0x58 */
    s16 cur;            /* 0x5C */
    s16 n;              /* 0x5E */
    u16 npc;            /* 0x60: the townsperson's id (NpcWork id) */
    s8 state;           /* 0x62 */
} ShopMenu;

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
    /* 0x07 */ s8 result;               /* 0 won, 1 draw, 2 lost (Minigame_DrawResult, TownMsg_Send21) */
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

/* A quest posted to the lobby: message 650B (LobbyLevel2_Send650B sends its 0x18 bytes;
 * Quest_PostStep). The quest desks keep it in their work area: SlotMenu, the contest
 * desk (unk_719A30.c), ArenaCupMenu at 0x0C, DojoMenu (unk_6CB690.c) at 0x4C. */
typedef struct Msg650B {
    /* 0x00 */ u16 quest;           /* the quest id */
    /* 0x02 */ s16 money;           /* the contest desk's reward (gRewardMoney - fee); 0 */
    /* 0x04 */ u16 area;            /* gScene.area */
    /* 0x06 */ s8 desk;             /* the desk: 0 contest, 1 OfficialDesk, 2 dojo, 3 arena cup */
    /* 0x07 */ char userId[7];      /* gMyUserId (6 bytes): LobbyLevel2_Send650B fills it */
    /* 0x0E */ char name[9];        /* gMyName (8 bytes): LobbyLevel2_Send650B fills it */
    /* 0x17 */ u8 pad17;
} Msg650B;
typedef char Msg650B_size_check[(sizeof(Msg650B) == 0x18) ? 1 : -1];

/* The head of gMenuWork that the quest posting menus share (SlotMenu, unk_719A30.c's VMenu):
 * Quest_PostStep's step and the posting it sends. */
typedef struct QuestPostHead {
    /* 0x00 */ u8 unk0[7];
    /* 0x07 */ s8 postStep;         /* Quest_PostStep's: 0 the quest's download, 1 the room list, 2 the entry, 3 the records, 4 the settings, 5 the posting (0x650B), 6, 7 0x6509; 8 an error shown */
    /* 0x08 */ u8 unk8[4];
    /* 0x0C */ Msg650B req;         /* the posting (LobbyLevel2_Send650B) */
} QuestPostHead;
typedef char QuestPostHead_size_check[(sizeof(QuestPostHead) == 0x24) ? 1 : -1];

/* A level-2 entry's posted quest as 650C returns it (0x20 bytes; Net_Get650C
 * copies it into gPostedQuests[id - 1]): the poster's Msg650B and 8 bytes more. */
typedef struct PostedQuest {
    /* 0x00 */ u16 quest;           /* the quest id (0: none) */
    /* 0x02 */ u16 money;           /* the fee to join (gWantedFeeTexts; QuestCounter_CheckQuest compares it with gSave.money) */
    /* 0x04 */ u16 area;            /* the poster's area (the counter's own when gScene.area) */
    /* 0x06 */ u8 desk;             /* Msg650B.desk: 3 the arena cup (D_0080C19C[it]) */
    /* 0x07 */ char userId[7];      /* the poster's (Msg650B) */
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

/* The arena cup's desk (ArenaCup_Open, gMenuWork; update ArenaCup_Update, draw
 * ArenaCup_Draw and ArenaCup_Draw2). */
typedef struct ArenaCupMenu {
    /* 0x00 */ u8 unk0[0xC];
    /* 0x0C */ Msg650B req;         /* the posting (ArenaCup_Post) */
    /* 0x24 */ WinSlide win;        /* the form, the record, the monster check */
    /* 0x2C */ WinSlide entryWin;   /* the entry form (ArenaCup_DrawEntryForm) */
    /* 0x34 */ WinSlide monsterWin; /* a monster's details (ArenaCup_DrawMonster) */
    /* 0x3C */ s8 cancel[2];        /* QuestDesk_Cancel's state: its step, 1 once cancelled */
    /* 0x3E */ u8 unk3E[2];
    /* 0x40 */ s32 season;          /* LobbyWorld_GetSeason, at ArenaCup_Open */
    /* 0x44 */ s8 step;             /* ArenaCup_Update's */
    /* 0x45 */ s8 subStep;          /* the step of the function step runs */
    /* 0x46 */ s8 renameStep;       /* ArenaCup_RenameMonster's */
    /* 0x47 */ s8 postStep;         /* ArenaCup_PostInput's */
    /* 0x48 */ u8 unk48[2];
    /* 0x4A */ s16 choice;          /* the first list: 0 enter, 1 records, 2 monsters */
    /* 0x4C */ s16 monster;         /* gSave.arenaEms' index */
    /* 0x4E */ s16 cursor;          /* the form's line, the cup kind (ArenaCup_IsKindOpen) */
    /* 0x50 */ u8 unk50[2];
    /* 0x52 */ s8 unk52;            /* 1 once a quest is held (ArenaCup_Update) */
    /* 0x53 */ s8 monsterCount;     /* the monsters kept in gSave.arenaEms */
} ArenaCupMenu;
typedef char ArenaCupMenu_size_check[(sizeof(ArenaCupMenu) == 0x54) ? 1 : -1];

/* The menu opened by OfficialDesk_Open (work area gMenuWork, id 0x45; update
 * OfficialDesk_Draw, draw OfficialDesk_Draw2): three rows, each with a list. */
typedef struct SlotMenu {
    /* 0x00 */ u8 unk0[7];
    /* 0x07 */ s8 postStep;         /* Quest_PostStep's (QuestPostHead) */
    /* 0x08 */ u8 unk8[4];
    /* 0x0C */ Msg650B req;         /* the posting (Quest_PostStep) */
    /* 0x24 */ struct QuestInfo *quest;         /* the quest chosen (Quest_GetDiscInfo) */
    /* 0x28 */ WinSlide win;
    /* 0x30 */ WinShake shake;       /* Win_Shake: dx, dy */
    /* 0x38 */ u8 *npc;
    /* 0x3C */ s8 row[3];           /* 0-2: shown, 3: not */
    /* 0x3F */ u8 unk3F;
    /* 0x40 */ s16 cursor;          /* the row */
    /* 0x42 */ u16 list[3][9];
    /* 0x78 */ s16 listCursor;      /* the cursor in the list */
    /* 0x7A */ s16 unk7A;
    /* 0x7C */ s16 unk7C;
    /* 0x7E */ s16 menuCursor;      /* the posting menu item (0-3, the password item 2 skipped without gRoomPassOn) */
    /* 0x80 */ s8 state;
    /* 0x81 */ s8 subStep;          /* the step within state */
    /* 0x82 */ s8 unk82;
} SlotMenu;
typedef char SlotMenu_size_check[(sizeof(SlotMenu) == 0x84) ? 1 : -1];

/* The name search (NameSearch_Open, gMenuWork; NameSearch_Update): the name typed, then
 * the pages of the hunters found. */
struct Net6711Rec;
typedef struct NameSearchMenu {
    /* 0x00 */ WinSlide win;
    /* 0x08 */ WinSlide win2;       /* NameSearch_Open slides it in */
    /* 0x10 */ struct Net6711Rec *rows[10]; /* the page's entries */
    /* 0x38 */ s8 unk38;            /* Chat_OpenKbd fills it (0: none) */
    /* 0x39 */ u8 unk39[0xF];
    /* 0x48 */ s16 unk48;
    /* 0x4A */ s16 cursor;
    /* 0x4C */ s16 page;
    /* 0x4E */ s16 pageCount;
    /* 0x50 */ s16 count;
    /* 0x52 */ s8 unk52;
    /* 0x53 */ s8 state;
} NameSearchMenu;
typedef char NameSearchMenu_size_check[(sizeof(NameSearchMenu) == 0x54) ? 1 : -1];

/* The room entry menu (gMenuWork; AreaBoard_LoadPage lists the rooms at 0xB2C) */
typedef struct RoomMenu {
    /* 0x000 */ WinSlide win;
    /* 0x008 */ u8 unk8[5][0x11C];     /* the members of a room (LobbyStep_630AUsers); [0]: nonzero if used, +8 the name */
    /* 0x594 */ s8 unk594;
    /* 0x595 */ u8 pad595[0xB20 - 0x595];
    /* 0xB20 */ s16 page;               /* the page */
    /* 0xB22 */ s16 pages;              /* the pages */
    /* 0xB24 */ s16 cursor;         /* the cursor in the members */
    /* 0xB26 */ s16 shown;          /* the members on the page */
    /* 0xB28 */ s8 unkB28;
    /* 0xB29 */ s8 step;
    /* 0xB2A */ s8 joinStep;        /* the step of AreaBoard_JoinInput */
    /* 0xB2B */ u8 padB2B;
    /* 0xB2C */ s8 list[16];         /* the rooms shown (AreaBoard_LoadPage) */
    /* 0xB3C */ s16 listCount;      /* the rooms in list */
    /* 0xB3E */ s16 listCursor;     /* the cursor in list */
    /* 0xB40 */ s16 unkB40;
    /* 0xB42 */ s16 unkB42;
    /* 0xB44 */ WinSlide listWin;      /* the room list (slides in from the left) */
    /* 0xB4C */ WinSlide win3;
    /* 0xB54 */ WinShake shake;
    /* 0xB5C */ s8 state;              /* AreaBoard_Update */
    /* 0xB5D */ s8 recruitStep;     /* the step of AreaBoard_RecruitInput */
    /* 0xB5E */ u8 padB5E;
    /* 0xB5F */ s8 unkB5F;
    /* 0xB60 */ s32 slot;               /* the room (AreaBoard_Join) */
    /* 0xB64 */ s16 unkB64;
    /* 0xB66 */ s16 unkB66;
    /* 0xB68 */ s16 unkB68;
    /* 0xB6A */ s16 slotCursor;     /* the cursor in the page of 10 (AreaBoard_ListInput) */
    /* 0xB6C */ s16 yesNo;          /* the recruit box choice (0 yes) */
    /* 0xB6E */ u16 unkB6E;
    /* 0xB70 */ char text[0x20];      /* the recruiting text (AreaBoard_RecruitInput, AreaBoard_DrawRecruit) */
    /* 0xB90 */ s8 unkB90;
    /* 0xB91 */ s8 unkB91;
} RoomMenu;
typedef char RoomMenu_size_check[(sizeof(RoomMenu) == 0xB94) ? 1 : -1];

/* A mail (0x92 bytes): the mail box's (gMailBox, sub_main 0x5C0540: the last 8, newest
 * first), the one being written (ShortMailMenu, sent as 6704 by LobbyStep_6704, kept in
 * gShortMailMsg). */
typedef struct MailEntry {
    /* 0x00 */ u8 used;
    /* 0x01 */ char id[8];          /* the sender (the addressee in ShortMailMenu) */
    /* 0x09 */ char name[0x11];
    /* 0x1A */ char text[0x78];
} MailEntry;
typedef char MailEntry_size_check[(sizeof(MailEntry) == 0x92) ? 1 : -1];

/* The short mail form (gShortMailMenu, 0x4B4 bytes at 0x8BCD20: ShortMail_Open, ShortMail_Step;
 * not in gMenuWork, so another menu can stay open under it). */
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

/* The level-2 list screen's work area (gMenuWork, unk_7BF560.c; LevelList_Begin): the entries grouped
 * by kind, up to 7 per group */
typedef struct LevelGroup {
    /* 0x00 */ s8 n;
    /* 0x01 */ s8 kind;
    /* 0x02 */ u8 pad2[2];
    /* 0x04 */ s32 ids[7];
} LevelGroup;
typedef struct LevelList {
    /* 0x00 */ s16 sel;                 /* the group of the entry the player is in */
    /* 0x02 */ s16 n;                   /* groups */
    /* 0x04 */ s16 sub;                 /* its index in the group */
    /* 0x06 */ s16 wait;                /* the slide-in counter (LevelList_WaitAnim; the panels slide while it is >= 0) */
    /* 0x08 */ s16 shake;               /* frames of the list's shake once an entry is chosen */
    /* 0x0A */ s16 shakeX;              /* the shake's offsets (LevelList_DrawText) */
    /* 0x0C */ s16 shakeY;
    /* 0x0E */ s16 unkE;
    /* 0x10 */ s32 count[4];            /* entries per kind */
    /* 0x20 */ LevelGroup g[20];
} LevelList;
typedef char LevelList_size_check[(sizeof(LevelList) == 0x2A0) ? 1 : -1];

/* The quest counter (lobby unk_76DB40.c, unk_770410.c): the room's members, its windows and
 * its steps (QuestCounter_Step). */
typedef struct QuestCounterMenu {
    /* 0x000 */ NetUser members[4];     /* the room's members (LobbyStep_Level2Entry, Quest_DrawArenaTeams) */
    /* 0x470 */ WinSlide questWin;      /* QuestCounter_DrawQuestWindow */
    /* 0x478 */ WinSlide kindWin;       /* QuestCounter_ChooseKind's (QuestCounter_DrawTeamChoice) */
    /* 0x480 */ WinSlide board[5];      /* QuestCounter_OpenWindows, QuestCounter_DrawRecruit */
    /* 0x4A8 */ s32 passKind;           /* LobbyLevel2_EnterWithPass's: 0, or the kind chosen */
    /* 0x4AC */ s16 delay;              /* QuestCounter_OpenWindows' frames between windows */
    /* 0x4AE */ s8 step;                /* QuestCounter_Step's */
    /* 0x4AF */ s8 subStep;             /* the step of the current choice or window opening */
    /* 0x4B0 */ u8 pad4B0;
    /* 0x4B1 */ s8 returnStep;          /* the step after the message (step 0x11) */
    /* 0x4B2 */ u8 pad4B2[2];
    /* 0x4B4 */ s8 kind;                /* 2..4, QuestCounter_ChooseKind's choice (D_5CA9F6) */
    /* 0x4B5 */ s8 cursor;
    /* 0x4B6 */ s8 quest;               /* the posted quest chosen (gPostedQuests index) */
    /* 0x4B7 */ s8 page;                /* the quest sheet's page */
    /* 0x4B8 */ s8 pages;               /* 2 for an arena quest (kind 3), else 4 */
    /* 0x4B9 */ u8 pad4B9[3];
    /* 0x4BC */ s32 questId;            /* the chosen quest's id (gLobbyQuestId) */
} QuestCounterMenu;
typedef char QuestCounterMenu_size_check[(sizeof(QuestCounterMenu) == 0x4C0) ? 1 : -1];

/* The quest desk's work area (gMenuWork, lobby unk_71C390.c and unk_7196F0.c): the quest board, the info index and
 * the guide pages (QuestDesk_*, QuestBoard_Draw). */
typedef struct QuestDesk {
    /* 0x00 */ u8 unk0[0x24];
    /* 0x24 */ struct QuestInfo *quest;     /* a quest shown instead of the list's */
    /* 0x28 */ s16 panelX;           /* the info panel's x */
    /* 0x2A */ u8 unk2A[0x38 - 0x2A];
    /* 0x38 */ s16 boardX;           /* the board's place (QuestBoard_Draw at +0x118, +0x3A) */
    /* 0x3A */ s16 boardY;
    /* 0x3C */ s16 unk3C;
    /* 0x3E */ u8 unk3E[0x40 - 0x3E];
    /* 0x40 */ s16 boardOfs;         /* added to both */
    /* 0x42 */ u8 unk42[0x44 - 0x42];
    /* 0x44 */ s16 unk44;
    /* 0x46 */ u8 unk46[2];
    /* 0x48 */ s8 unk48[2];
    /* 0x4A */ s16 kinds[5];         /* the index's entries (gInfoIndexTexts) */
    /* 0x54 */ s16 count;
    /* 0x56 */ u16 forecast[5];      /* the elder's forecast (ElderForecast_DrawPanel) */
    /* 0x60 */ s16 sel;
    /* 0x62 */ s16 unk62;
    /* 0x64 */ s16 page;              /* 0 overview, 1 terms, 2 details */
    /* 0x66 */ s16 unk66;
    /* 0x68 */ s16 unk68;
    /* 0x6A */ s8 mode;
    /* 0x6B */ s8 unk6B;
    /* 0x6C */ s8 unk6C;
    /* 0x6D */ s8 unk6D;
    /* 0x6E */ s16 kindSel;           /* the entry of kinds[] at the cursor */
    /* 0x70 */ s16 list;
    /* 0x72 */ s16 unk72;
    /* 0x74 */ s8 season;           /* LobbyWorld_GetSeason (QuestDesk_Open): gSeasonNames' index, the frame's colours (gDecoFrameColors) */
    /* 0x75 */ s8 pm;               /* LobbyTime_IsPm (QuestDesk_Open): gDaytimeNames' index */
    /* 0x76 */ s8 unk76;            /* 1 at townsperson 0x4F's desk (QuestDesk_Open) */
    /* 0x77 */ s8 unk77;
    /* 0x78 */ u16 ids[1][12];
} QuestDesk;
typedef char QuestDesk_size_check[(sizeof(QuestDesk) == 0x90) ? 1 : -1];

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
    RoomMenu room;
    LevelList levelList;
    Minigame drink;
    QuestCounterMenu questCounter;
    u8 raw[0xF00];
} MenuWork;
typedef char MenuWork_size_check[(sizeof(MenuWork) == 0xF00) ? 1 : -1];

#endif /* LOBBY_MENU_H */
