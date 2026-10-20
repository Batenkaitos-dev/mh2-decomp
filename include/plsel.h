#ifndef PLSEL_H
#define PLSEL_H

#include "common.h"
#include "item.h"              /* ItemStack */
#include "sprite.h"            /* Sprite (Sprite_Draw) */
#include "arena_em.h"
#include "quest.h"             /* gQuest (QuestState) */
#include "window.h"            /* ListPanel, ListWin, Panel, Frame */
#include "chat_log.h"          /* ChatLine (PlselSelWork.log) */

/* The player select overlay (plsel.bin). */


/* The rewards list (PlselWork.rewList, 0x1A64 bytes up to noticeCount): text
 * entries of 0x4C bytes from +0x1DC. */
typedef struct PlselRewEntry {
    s8 flag;                    /* 0x00: shown */
    u8 unk01[0x4C - 0x01];
} PlselRewEntry;

typedef struct PlselList {
    s32 goal[3];                /* 0x000: the three goals' rewards */
    s32 huntReward;             /* 0x00C: the hunting reward (the categories but 12 and 13; x2 with catReward[12], x1.5 with [13]) */
    s32 fastClear;              /* 0x010: 1: cleared within the par time (doubles rewardTotal) */
    s32 rewardTotal;            /* 0x014: the goals and the hunting reward */
    s32 clearTime;              /* 0x018: the quest time (frames; gQuestStats +0x14) */
    s32 catReward[17];          /* 0x01C: per hunting category (17; Plsel_CategoryIsShared splits some) */
    s32 kill[0x4A];             /* 0x060: per monster group */
    s32 killTotal;              /* 0x188: the kill rewards' total */
    s32 payout;                 /* 0x18C: (itemTotal + rewardTotal) * the rate percent (Save_AddPoints) */
    s32 zenny;                  /* 0x190: the money (Save_GetPoints, + payout) */
    s32 payoutCopy;             /* 0x194: payout as computed (not read in the C) */
    s32 screen;                 /* 0x198: 0 the rewards, 1, 2 (Plsel_RewardListDraw) */
    s32 pageCount[3];           /* 0x19C: pages per screen */
    s32 pageCur;                /* 0x1A8: the page shown */
    s32 pages[5];               /* 0x1AC: kind of each page */
    s32 topLine;                /* 0x1C0: the first line shown */
    s32 unk1C4;
    s32 lineCount;              /* 0x1C8: lines written */
    s32 linesShown;             /* 0x1CC: lines shown at once (8, 12) */
    s32 scrollable;             /* 1: the page scrolls (arrows drawn) */
    s32 frame;                  /* 0x1D4: frame counter */
    s32 arrowBob;               /* 0x1D8: the arrows' offset (toggles every 4 frames) */
    PlselRewEntry ent[80];      /* 0x1DC */
    ItemStack pouch[20];   /* 0x199C: the pouch at the quest's end */
    s32 val[20];                /* 0x19EC: what each slot sells for */
    s32 itemTotal;              /* 0x1A3C: the items' sell total */
    s32 parTime;                /* 0x1A40: the par time in frames (Dojo_GetParMinutes * 1800) */
    s32 unk1A44;                /* 0x1A44 */
    s32 revealLine;             /* 0x1A48: the last entry revealed (one every 20 frames) */
    s32 newBestScore;           /* 0x1A4C: 1: a new best score (Dojo_SetBestScore) */
    s32 doneShown;              /* 0x1A50: set in the last step */
    s32 newBestTime;            /* 0x1A54: 1: a new best time (Dojo_SetBestTime) */
    s32 newBestTime2;           /* 0x1A58: set with newBestTime (not read in the C) */
    s32 markMonsterLine;        /* 0x1A5C: the line marked on the monsters page (-1 none) */
    s32 markHuntLine;           /* 0x1A60: the line marked on the hunting page (-1 none) */
} PlselList;

/* Its work area (include/plsel.spec lists the fields). t[n] in the comments:
 * the results screen's format text that prints the field (Plsel_BuildResultText
 * reads them from sub_main's texts). */
typedef struct PlselWork {
    s32 resMode;                /* 0x000: the results page up: 0 the money pages, 1 the hunter rank points, 3 the size records, 4 the arena monster (RES_MODE_*) */
    s32 moneyTotal;             /* 0x004: the money paid (all four sums; Save_AddMoney); t[4] */
    s32 questMoney;             /* 0x008: the quest reward: fees and goal rewards (t[1], t[20]) */
    s32 monsterMoney;           /* 0x00C: the monsters' trade price total (t[2], t[11]) */
    s32 extraMoney;             /* 0x010: added to the total; not set here */
    s32 itemMoney;              /* 0x014: the items sold (t[3], t[24]) */
    s32 questMoneyCut;          /* 0x018: 1: the goal reward differs from the quest's, or the 60% cut online */
    s32 monsterMoneyCut;        /* 0x01C: 1: online with others, or the 60% cut (monsterMoney * 60 / 100) */
    s32 mustOrder;              /* 0x020: 1: a must-order quest (the sub goals fold into the main one) */
    s32 mainDone;               /* 0x024: the main goal is done (QuestGoal_IsDone(0)) */
    s32 subADone;               /* 0x028: sub goal A is done */
    s32 subBDone;               /* 0x02C: sub goal B is done */
    u16 recordGroups[0x4A];     /* 0x030: monster groups with a new record (Plsel_AddRecordGroup) */
    u16 largestNew[0x4A];       /* 0x0C4: per group: 1 or 2 when the largest record is new */
    u16 smallestNew[0x4A];      /* 0x158: the same for the smallest */
    s32 recordGroupCount;       /* 0x1EC: entries of recordGroups */
    s32 recordGroupCur;         /* 0x1F0: the recordGroups entry shown */
    u16 newRecord;              /* 0x1F4: 1: some size record is new */
    u16 oldLargest;             /* 0x1F6: the shown group's largest record before (a size percent) */
    u16 oldSmallest;            /* 0x1F8: its smallest record before */
    u16 newLargest;             /* 0x1FA: its largest size of the quest (0: none) */
    u16 newSmallest;            /* 0x1FC: its smallest size of the quest */
    u8 unk1FE[0x200 - 0x1FE];
    f32 oldLargestCm;           /* 0x200: oldLargest in cm (Plsel_BuildResultText) */
    f32 oldSmallestCm;          /* 0x204: oldSmallest in cm */
    f32 newLargestCm;           /* 0x208: newLargest in cm (t[29]) */
    f32 newSmallestCm;          /* 0x20C: newSmallest in cm (t[30]) */
    u16 shownGroup;             /* 0x210: the monster group shown */
    s8 unk0212;                 /* 0x212 */
    s8 unk0213;                 /* 0x213 */
    u8 unk214[0x33C - 0x214];
    u16 largestBefore[0x4A];    /* 0x33C: largest record per group before the quest */
    u16 smallestBefore[0x4A];   /* 0x3D0: smallest record per group before the quest */
    u16 huntedByGroup[0x4A];    /* 0x464: monsters hunted in the quest, by kind group (gQuest.hunted) */
    u16 capturedByGroup[0x4A];  /* 0x4F8: monsters captured, by kind group (gQuest.captured) */
    u16 huntedLeft[0x4A];       /* 0x58C: copies of huntedByGroup, used up by the hunt goals' bonuses */
    u16 capturedLeft[0x4A];     /* 0x620: copies of capturedByGroup, the same */
    s32 page;                   /* 0x6B4: the results page (Plsel_BuildResultText's kind); the menus' cursor */
    s32 pageCount;              /* 0x6B8: the results pages */
    s32 timeout;                /* 0x6BC: frames until the screen moves on by itself (0xE10: a minute) */
    s8 closeState;              /* 0x6C0: 0 open, 1 close asked, 2 closing (PlselRes_Close) */
    s8 closeTimer;              /* 0x6C1: frames left of the closing */
    s8 unk06C2;                 /* 0x6C2 */
    s8 menuStep;                /* 0x6C3: the record slot menu's step (Plsel_SaveRecordMenu) */
    s8 recordSlot;              /* 0x6C4: the record slot chosen */
    s8 help1;                   /* 0x6C5: help line 1 (0: none; gArenaEmPenHelp) */
    s8 help2;                   /* 0x6C6: help line 2 */
    u8 unk6C7[0x6C8 - 0x6C7];
    s32 recordCount;            /* 0x6C8: arena records saved (gSave.arenaEms[].unkA != 0, at most 3) */
    s32 friendChanged;          /* 0x6CC: the arena monster's friendliness changed (t[39] lit) */
    s32 strengthChanged;        /* 0x6D0: its strength changed (t[40] lit) */
    s32 sizeChanged;            /* 0x6D4: its size changed (t[41] lit) */
    ArenaEm arenaBefore;        /* 0x6D8: the arena monster before the fight (t[37]) */
    s32 fee;                    /* 0x6F4: the contract fee back (t[15]) */
    s32 wtdFee;                 /* 0x6F8: the WTD contract fee (t[16]) */
    f32 feeRate;                /* 0x6FC: fee multiplier (2.0 or 1.0; str[1] shown when not 1) */
    f32 wtdFeeRate;             /* 0x700: wtdFee multiplier (2.0 when wtdDouble) */
    s32 mainMoney;              /* 0x704: the main goal's reward (t[17]; t[52] must-order) */
    s32 subAMoney;              /* 0x708: sub goal A's reward (t[18]) */
    s32 subBMoney;              /* 0x70C: sub goal B's reward (t[19]) */
    s32 deaths;                 /* 0x710: the local player's deaths (t[47]) */
    s32 hrpTotal;               /* 0x714: hunter rank points in all (t[48]; Hr_AddPoints) */
    s32 hrpDeaths;              /* 0x718: the deaths' points (deaths * -20) */
    s32 hrpMain;                /* 0x71C: the main goal's points (t[44]; t[53] must-order; t[51] failed) */
    s32 hrpSubA;                /* 0x720: sub goal A's points (t[45]) */
    s32 hrpSubB;                /* 0x724: sub goal B's points (t[46]) */
    u8 unk728[0x72C - 0x728];
    s32 rankBefore;             /* 0x72C: hunter rank before (Save_RankFromPoints) */
    s32 rankAfter;              /* 0x730: hunter rank after (t[50]) */
    s32 rankChange;             /* 0x734: 1: rank up, 2: down */
    s32 hrPoints;               /* 0x738: gSave.hrPoints after (t[49]) */
    s32 hrpResult;              /* 0x73C: Hr_AddPoints' result (which note to show) */
    u8 unk740[0x748 - 0x740];
    s32 unk0748;                /* 0x748 */
    s32 scroll;                 /* 0x74C: the first line shown on the scrolling pages */
    s32 lineCount;              /* 0x750: text lines written */
    s32 firstLine;              /* 0x754: first item line shown (page 3, seven lines) */
    char lines[80][0x4C];       /* 0x758: text lines (Plsel_BuildResultText) */
    PlselList rewList;          /* 0x1F18: the rewards list */
    s32 noticeCount;            /* 0x397C: entries of notices */
    s32 noticePage;             /* 0x3980: the notices page shown (three a page) */
    u16 notices[0x39];          /* 0x3984: notices to show: unlocks (Plsel_QueueFlagUnlocks), 0x3nn, 0x400, 0x500 */
    u8 unk39F6[0x39F8 - 0x39F6];
    s32 itemLines;              /* 0x39F8: item lines (Plsel_TallyRewards) */
    s32 awardCount;             /* 0x39FC: entries of awards (at most 0x20) */
    u16 awards[0x20];           /* 0x3A00: awards to show (Plsel_QueueAward) */
    u16 questLargest[0x50];     /* 0x3A40: largest size of the quest, per kind then per group */
    u16 questSmallest[0x50];    /* 0x3AE0: smallest size of the quest */
    u8 lowRankCut;              /* 0x3B80: 1: rank over 30 on a quest under 5 stars: 60% of the points and money */
    u8 wtdDouble;               /* 0x3B81: 1: the WTD fee counts double (Plsel_SumRecordRewards) */
    u8 rateNoticeQueued;        /* 0x3B82: 1 once the notice that the rates are open is queued */
    u8 clearedBefore;           /* 0x3B83: a drill: whether its quest was cleared before this clear (its notice comes on the first) */
    s32 pointRate;              /* 0x3B84: the points' percent (Plsel_ResetPointRate) */
} PlselWork;
typedef char PlselWork_size_check[(sizeof(PlselWork) == 0x3B88) ? 1 : -1];

/* The work area's pointer, a sub_main variable. */
extern void *gOverlayWorkPtr; /* sub_main: the work pointer the overlays share */
#define gPlselWork (*(PlselWork **)&gOverlayWorkPtr)

/* plsel.c's view of the same work area (include/plsel_sel.spec). */
typedef struct PlselSelWork {
    u8 kbdOpen;                 /* 0x000: 1 while the chat keyboard is open (Plsel_ChatOpenKbd, Plsel_ChatUpdate) */
    u8 logHead;                 /* 0x001: the next chat log entry (wraps at 64; Plsel_AddChatLog) */
    u8 logCount;                /* 0x002: chat log entries (at most 0x40) */
    u8 unk003[0x004 - 0x003];
    s32 sendWait;               /* 0x004: frames before the next message may be sent (0x3C after one; Kbd_SetSubmitWait) */
    u8 logScroll;               /* 0x008: counts messages added past 11 log lines (at most 0x3F) */
    u8 unk009[0x00B - 0x009];
    ChatLine log[64];           /* 0x00B: the chat log, a ring (logHead, logCount) */
    u8 unk13CB[0x13D4 - 0x13CB];
    u8 newMsg;                  /* 0x13D4: 1: a message was sent or came in */
    u8 newMsgMine;              /* 0x13D5: 1: it was the local player's */
    u8 unk13D6[0x13D8 - 0x13D6];
    s16 logShowTimer;           /* 0x13D8: frames the chat log stays shown (0x12C after a message) */
    u8 unk13DA[0x13DC - 0x13DA];
} PlselSelWork;
typedef char PlselSelWork_size_check[(sizeof(PlselSelWork) == 0x13DC) ? 1 : -1];

#define gPlselSelWork (*(PlselSelWork **)&gOverlayWorkPtr)
#define PLSEL_CHAT_LOG_LEN 64       /* PlselSelWork.log's entries, a ring: the indices wrap with & (PLSEL_CHAT_LOG_LEN - 1) */
#define PLSEL_CHAT_ROW_LEN 0x16     /* the text bytes of a chat log row; a two-byte character may end one byte past it, and such a row is drawn narrower */

/* The list panels and windows of the overlay are window.h's ListPanel, ListWin and Panel. */

/* A preset hunter's equipment (14 bytes): the rows of gPlselPresets[set]
 * (four per set, Plsel_CurPreset) and of the stage records gTrainingPresets[stage]
 * (Plsel_ApplyStageEquip). The armour ids go to Player.equip's armour slots,
 * EQUIP_HEAD to EQUIP_WAIST and EQUIP_LEGS (Plsel_SetupSavedPlayer; the names
 * gArmorHeadNames to gArmorLegsNames). */
typedef struct PlselPreset {
    u16 weaponType;             /* 0x0: the weapon's EquipItem type (EQUIP_BLADE or EQUIP_GUN) */
    u16 weaponId;               /* 0x2 */
    u16 weaponLevel;            /* 0x4 */
    u8 head;                    /* 0x6: armour ids, of types EQUIP_HEAD, EQUIP_BODY, EQUIP_ARMS, EQUIP_WAIST and EQUIP_LEGS */
    u8 body;                    /* 0x7 */
    u8 arms;                    /* 0x8 */
    u8 waist;                   /* 0x9 */
    u8 legs;                    /* 0xA */
    u8 startItems;              /* 0xB: the start item list in the set (gStartItemLists[startItems + set * 4]) */
    u16 pointRatePct;           /* 0xC: the points' percent (QuestSync.pointRatePct) */
} PlselPreset;
typedef char PlselPreset_size_check[(sizeof(PlselPreset) == 0xE) ? 1 : -1];

/* A preset player's look (16 bytes, gPlselPresetPlayers: Plsel_SetupPresetPlayer). */
typedef struct PlselPresetPlayer {
    u8 gender;                  /* 0x0 */
    u8 face;                    /* 0x1: the face (Player.equip[EQUIP_SLOT_FACE].id) */
    u8 hairStyle;               /* 0x2 */
    u8 unk3;                    /* 0x3 */
    s32 hairColor;              /* 0x4 */
    u8 head;                    /* 0x8: armour ids, as PlselPreset's */
    u8 body;                    /* 0x9 */
    u8 arms;                    /* 0xA */
    u8 waist;                   /* 0xB */
    u8 legs;                    /* 0xC */
    u8 unkD[0x10 - 0xD];
} PlselPresetPlayer;
typedef char PlselPresetPlayer_size_check[(sizeof(PlselPresetPlayer) == 0x10) ? 1 : -1];

/* The select screen (plsel.c). */
#define PLSEL_PREVIEW_PLAYER 5      /* gPlayers' slot of the select screen's preview hunter (Plsel_SetCamera aims at it) */
#define PLSEL_SET_PRESETS 4         /* the preset hunters of a set (gPlselPresets[set], gQuestSync.pick 0-3) */
#define PLSEL_PRESET_SETS 17        /* the sets: gPlselPresetStages' rows (Plsel_PresetSetForStage) */
#define PLSEL_SET_NONE 0xFF         /* gQuestSync.builtSet: no preview players built yet */
#define PLSEL_TEX_BG 0xA3           /* the texture handles of the overlay's pack: the background */
#define PLSEL_TEX_PANEL 0xA4        /* the bottom panel */
#define PLSEL_TEX_HEADER 0xA5       /* the header strips (Plsel_DrawHeader's rows) */
#define PLSEL_TEX_ICONS 0x144       /* the system icons' pack */
#define PLSEL_TEX_WEAPON_ICONS 0x155 /* the weapon-class and equipment icons */

/* plsel_entry's steps (TaskSlot.mode). */
enum PlselEntryMode {
    PLSEL_ENTRY_SETUP = 0,      /* Plsel_Setup */
    PLSEL_ENTRY_WAIT = 1,       /* Plsel_WaitPlayersReady */
    PLSEL_ENTRY_FINISH = 2,     /* Plsel_Finish */
    PLSEL_ENTRY_DEBUG = 3       /* Plsel_DebugMenu */
};

/* Plsel_SelectTask's steps (TaskSlot.mode). */
enum PlselSelMode {
    PLSEL_SEL_INIT = 0,         /* Plsel_SelectInit */
    PLSEL_SEL_SOUNDS = 1,       /* Plsel_SelectWaitSound */
    PLSEL_SEL_START_CHOOSE = 2, /* Plsel_SelectStartChoose */
    PLSEL_SEL_SETUP_PLAYERS = 3, /* Plsel_SelectSetupPlayers */
    PLSEL_SEL_CHOOSE = 4,       /* Plsel_StateChoose */
    PLSEL_SEL_READY = 5         /* Plsel_StateReady */
};

/* Plsel_StateChoose's sub-steps (TaskSlot.step). */
enum PlselChooseStep {
    PLSEL_CHOOSE_BROWSE = 0,    /* left and right pick a preset hunter */
    PLSEL_CHOOSE_RETURN = 1,    /* waits for the fade, then back to PLSEL_SEL_START_CHOOSE (no setter seen) */
    PLSEL_CHOOSE_MENU = 2,      /* the player menu is open (Plsel_OpenPlayerMenu) */
    PLSEL_CHOOSE_CONFIRM = 3,   /* the yes/no question (TaskSlot.flag: 0 yes, 1 no) */
    PLSEL_CHOOSE_TAKEN = 5      /* set with PLSEL_SEL_READY once the preset is taken */
};

/* The player menu's entries (Plsel_DrawPlayerMenuList; HudState.fieldMenuCursor). */
enum PlselMenuEntry {
    PLSEL_MENU_POUCH,           /* 0: the pouch (sub_main's PouchMenu) */
    PLSEL_MENU_STATUS,          /* 1: the personal status */
    PLSEL_MENU_EQUIPMENT,       /* 2: the equipment status */
    PLSEL_MENU_QUEST,           /* 3: the quest's information */
    PLSEL_MENU_STAGE,           /* 4: the stage page (Plsel_DrawStagePage) */
    PLSEL_MENU_COUNT
};

/* The gallery's quest scenes and town tours (Galex_Task). */
#define GALEX_QUEST_ID 0x3AB1       /* gNextQuestId of an entry with no quest of its own: the gallery's own stage file (Galex_SetupQuest) */
#define GALEX_PAIRS_END 0xFF        /* ends gGalexEntryPairs' first list */
#define GALEX_TOUR_FACILITY_ROW 12  /* gGalexEntryPairs' row of the town tours' facility levels */

/* gGalexQuests' records (46 of them): per gallery entry (its
 * GAL_ENTRY_MASK value), the quest it plays and for how long. */
typedef struct GalQuest {
    u16 questId;                /* 0x0: -> gNextQuestId; 0: the gallery's own stage file (GALEX_QUEST_ID) */
    u16 unk2;                   /* not read by plsel */
    u8 season;                  /* 0x4: Season_Set's argument */
    u8 unk5;                    /* not read by plsel */
    u16 limit;                  /* 0x6: the time limit in frames (0: none) */
} GalQuest;

/* Galex_QuestTask's steps (TaskSlot.mode). */
enum GalexQuestMode {
    GALEX_QUEST_LOAD,           /* 0: loads the game overlay */
    GALEX_QUEST_SETUP,          /* 1: the quest, its area, players and monsters */
    GALEX_QUEST_SOUNDS,         /* 2: the sound banks (SceneState.subStep 0-20) */
    GALEX_QUEST_START,          /* 3: the area, the scene event's camera, the fade-in */
    GALEX_QUEST_FADE_IN,        /* 4 */
    GALEX_QUEST_PLAY,           /* 5: until the time limit, PAD_CROSS or the event's end */
    GALEX_QUEST_FADE_OUT,       /* 6 */
    GALEX_QUEST_END             /* 7: back to select */
};

/* Galex_Task's steps (TaskSlot.mode). */
enum GalexTourMode {
    GALEX_TOUR_SETUP,           /* 0: loads the lobby overlay, sets up the save and the town */
    GALEX_TOUR_ENTER,           /* 1: runs the lobby's screen steps until it is in the town */
    GALEX_TOUR_PLAY,            /* 2: until the time limit, PAD_CROSS or the camera's end */
    GALEX_TOUR_FADE_OUT,        /* 3 */
    GALEX_TOUR_END              /* 4: back to select */
};

/* The ending (ending.c). */
#define ENDING_OBJ_SLOTS 16         /* EndWork.slots: the objects at a time */
#define ENDING_TEX_FIRST 0xA3       /* the ending's first texture handle in gTexHandles (gOvlTex) */
#define ENDING_TEX_COUNT 26         /* the endtex partition's textures */
#define ENDING_ROLL_PIC_TEX 0xBC    /* the handle of the roll pictures' texture */

/* EndWork.state (Ending_Update). */
enum EndingState {
    ENDING_START,               /* 0: fades in and prepares the music stream */
    ENDING_MUSIC,               /* 1: starts the music */
    ENDING_ROLL,                /* 2: the roll (Ending_UpdateRoll) until its end, a skip or timeLeft */
    ENDING_MESSAGE,             /* 3: the closing message, messageTimer frames */
    ENDING_DONE,                /* 4: fading out after the message */
    ENDING_SKIPPED,             /* 5: fading out after a skip or the time-out */
    ENDING_RESTART = 99         /* fades in, then ENDING_MUSIC (nothing in plsel sets it) */
};

/* EndWork.fadePhase: the fading picture's. */
enum EndingFadePhase {
    ENDING_FADE_IN,             /* 0: alpha up by fadeStep */
    ENDING_FADE_HOLD,           /* 1: until near the step's end */
    ENDING_FADE_OUT             /* 2: alpha down, then the next step */
};

/* EndRoll.type. */
enum EndingRollType {
    ENDING_ROLL_TEXT,           /* 0: a line of text */
    ENDING_ROLL_ICON,           /* 1: an animated icon */
    ENDING_ROLL_TITLE,          /* 2: a title in the big letters */
    ENDING_ROLL_PICTURE,        /* 3: a picture */
    ENDING_ROLL_SPAWN           /* 4: objects started as it comes into view (Ending_SpawnObjects) */
};

/* EndObj.moveKind (Ending_MoveObjects). */
enum EndingMove {
    ENDING_MOVE_STAY,           /* 0: 300 frames in place */
    ENDING_MOVE_RISE_WITH_ROLL, /* 1: up with the roll, until off the top */
    ENDING_MOVE_BOB,            /* 2: down and up, 150 frames each */
    ENDING_MOVE_WALK_LATE,      /* 3: walks after 180 frames, until off the left edge */
    ENDING_MOVE_WALK,           /* 4: walks at once */
    ENDING_MOVE_WALK_SOON,      /* 5: walks after 90 frames */
    ENDING_MOVE_FALL,           /* 6: falls until below the screen */
    ENDING_MOVE_FLY,            /* 7: flies across, bobbing every 30 frames */
    ENDING_MOVE_SINK,           /* 8: down */
    ENDING_MOVE_RISE            /* 9: up */
};

/* The quest results screen's own objects (the overlay's .bss and data). */
/* One entry of gPlselBonus's recs (8 bytes). */
typedef struct PlselRec {
    s16 type;
    s16 id;
    s16 count;                  /* goal multiples met, monsters hunted, items delivered */
    s16 reward;                 /* the money per count (the delivery's whole reward) */
} PlselRec;

/* gPlselBonus: 0xD0 bytes of the file's .bss. */
typedef struct PlselBonus {
    u8 unk00[0x08];
    s32 rewardMoney;            /* 0xD7BD98: gRewardMoney at the results' start */
    u8 unk0C[0x04];
    u8 itemsTaken;              /* 0xD7BDA0: 1 when Plsel_TakeSaleItems took a stack */
    u8 hasBonusRecs;            /* 0xD7BDA1: 1 when Plsel_CheckTargetBonus filled a record */
    u8 sizeCheckAfter;          /* 0xD7BDA2: the size records are checked from the group after it; only cleared */
    u8 unk13[0x16 - 0x13];
    struct {
        u16 item;
        s16 count;
    } items[20];                /* 0xD7BDA6 */
    u16 deliveredItem;          /* 0xD7BDF6: the delivery record's item */
    s16 deliveredCount;         /* 0xD7BDF8: and its count */
    PlselRec recs[8];           /* 0xD7BDFA */
    u8 unkAA[0xD0 - 0xAA];
} PlselBonus;

/* A countdown and its flags (gPlselItemBox). */
typedef struct PlselItemBoxMenu {
    u8 state;                   /* IBOX_LIST, IBOX_ENTRY, IBOX_DONE */
    u8 cursor;                  /* the entry: IBOX_ENTRY_BOX, _SELL, _FINISH */
    u8 subStep;                 /* in the box: IBOX_BOX_*; in the sale: IBOX_SELL_* */
    u8 infoWindow;              /* the description shown: IBOX_INFO_* */
    u8 sold;                    /* nonzero: the sale entry ends the menu at once (only cleared here) */
    u8 entryCount;              /* the entries the cursor moves over */
    u8 boxCursor;               /* the supply box slot: column | row << 3 */
    u8 pouchCursor;             /* the pouch slot */
    s16 timeLeft;               /* frames; the menu closes at 0 */
    s8 noRoom;                  /* 0: the pouch can't take the box item (its line shows); -1: it can */
    u8 blinkTimer;              /* counts up after a swap; the next-page icon blinks with it */
    u8 sellChoice;              /* the sale's question: 0 yes, 1 no */
    u8 unkD[3];
} PlselItemBoxMenu;

/* An entry of sub_main's gRecordCountEntries (4 bytes, ends with id 0). */
typedef struct PlselKind {
    u16 id;
    u16 kind;
} PlselKind;

/* One hunt-count award: granted once the hunted count of `monster` (a
 * kind, or a group when byMonster is 0) reaches `count`. byMonster 0xFF
 * ends the list. */
typedef struct PlselHuntAward
{
    u8 byMonster;
    u8 monster;
    u8 count;
    u8 award;
} PlselHuntAward;

/* gPlselItemBox with the object after it (gPlselItemBoxSale), as the shop menu uses
 * them: one 0x20-byte work area. */
typedef struct PlselShop {
    PlselItemBoxMenu t;               /* 0x00 */
    s32 money;                  /* 0x10 */
    s32 flag;                   /* 0x14 */
} PlselShop;

/* The quest results screen (Plsel_ResultsUpdate). Entries of sub_main's
 * gPlselTexts that it uses, by their roles. */
#define PLSEL_TEXT_QUIT_NOTICE 26   /* the five lines of the notice after a quest ended with QUEST_PHASE_QUIT */
#define PLSEL_TEXT_ITEMBOX_TIME 31  /* the item box menu's time left, in seconds (also the arena record menu's) */
#define PLSEL_TEXT_STACK_NAME 32    /* the box item's name */
#define PLSEL_TEXT_STACK_COUNT 33   /* the box item's name and count */
#define PLSEL_TEXT_NO_ROOM 34       /* the pouch can't take the box item */
#define PLSEL_TEXT_RESULT_FMTS 44   /* the results pages' line formats (Plsel_BuildResultText) */
#define PLSEL_TEXT_TITLES_HEADING 69 /* the heading of the hunter titles page */
#define PLSEL_TEXT_NEW_RECORD 78    /* drawn beside a beaten size record */
#define PLSEL_TEXT_NOTICES_HEADING 79 /* the heading of the notices page */
#define PLSEL_TEXT_PROMPT 111       /* the prompt under a page */
#define PLSEL_TEXT_COUNTDOWN 112    /* a page's time left, in seconds */
#define PLSEL_TEXT_REWARD_LIST 120  /* the drill rewards list's texts (24) */
#define PLSEL_TEXT_REWARD_CATEGORIES 144 /* the reward categories' lines (24) */

/* Entries of gHelpTexts_6, the item box menu's page of help texts. */
#define IBOX_TEXT_SELL_YES 4        /* the sale's yes; no follows it (indexed by sellChoice) */
#define IBOX_TEXT_DRILL_ENTRIES 10  /* the menu's entries in a drill */
#define IBOX_TEXT_STACK_VALUE 16    /* the label before a stack's value */
#define IBOX_TEXT_SALE_TOTAL 17     /* the label before the sale's total */
#define IBOX_TEXT_VALUE_FMT 18      /* the format of a value */
#define IBOX_TEXT_MONEY_FMT 19      /* the format of the hunter's money */

/* The steps of the quest results screen (gScene.step in Plsel_ResultsUpdate).
 * A page's steps come in threes: its set-up, its input, and its fade-out. */
enum PlselResStep {
    PLRES_START,                /* 0: the tally; picks the first page */
    PLRES_ARENA_MENU,           /* 1: keep the captured monster as an arena record? */
    PLRES_ARENA_OUT,            /* 2: fades out to the item box menu or the money pages */
    PLRES_ITEMBOX,              /* 3: the item box menu */
    PLRES_ITEMBOX_OUT,          /* 4: fades out to the next page */
    PLRES_MONEY,                /* 5: the money pages */
    PLRES_MONEY_OUT,            /* 6 */
    PLRES_POINTS_START,         /* 7: online, the hunter rank points page */
    PLRES_POINTS,               /* 8 */
    PLRES_POINTS_OUT,           /* 9 */
    PLRES_TITLES_START,         /* 0xA: the hunter titles earned */
    PLRES_TITLES,               /* 0xB */
    PLRES_TITLES_OUT,           /* 0xC */
    PLRES_RECORDS_START,        /* 0xD: the size records */
    PLRES_RECORDS,              /* 0xE */
    PLRES_RECORDS_OUT,          /* 0xF */
    PLRES_NOTICES_START,        /* 0x10: the notices of what was unlocked */
    PLRES_NOTICES,              /* 0x11 */
    PLRES_NOTICES_OUT,          /* 0x12 */
    PLRES_REWARD_LIST_START,    /* 0x13: a drill's rewards list */
    PLRES_REWARD_LIST,          /* 0x14 */
    PLRES_MONSTER,              /* 0x15: after a versus quest, the arena monster page */
    PLRES_MONSTER_OUT,          /* 0x16 */
    PLRES_QUIT_NOTICE,          /* 0x17: the notice after QUEST_PHASE_QUIT */
    PLRES_QUIT_OUT,             /* 0x18 */
    PLRES_EXIT                  /* 0x19: on to the opening, the ending or the quest's end */
};

/* PlselWork.closeState: a page's closing (PlselRes_BeginClose, PlselRes_Close). */
enum PlselResClose {
    PLRES_OPEN,                 /* 0 */
    PLRES_CLOSE_ASKED,          /* 1: the page's input asked to close */
    PLRES_CLOSING               /* 2: closeTimer runs */
};

/* PlselWork.resMode: which results page is up (Plsel_DrawResultPage picks
 * what to add by it). */
enum PlselResMode {
    RES_MODE_MONEY,             /* 0: the money pages */
    RES_MODE_POINTS,            /* 1: the hunter rank points */
    RES_MODE_RECORDS = 3,       /* 3: the size records */
    RES_MODE_ARENA              /* 4: the arena monster */
};

/* PlselWork.page: the page whose lines Plsel_BuildResultText builds. */
enum PlselResPage {
    RES_PAGE_SUMMARY,           /* 0: the money totals */
    RES_PAGE_QUEST,             /* 1: the fee and the goals' rewards */
    RES_PAGE_MONSTERS,          /* 2: the monsters' rewards */
    RES_PAGE_ITEMS,             /* 3: the items sold */
    RES_PAGE_POINTS,            /* 4: the hunter rank points */
    RES_PAGE_RECORDS,           /* 5: a monster group's size records */
    RES_PAGE_ARENA = 7          /* 7: the arena monster */
};
#define RES_MONEY_PAGES 4           /* the money pages, RES_PAGE_SUMMARY to RES_PAGE_ITEMS */
#define PLRES_PAGE_TIMEOUT 0xE10    /* frames a page or menu waits before it goes on by itself: 2 min */
#define PLSEL_GROUP_COUNT 0x4A      /* the monster groups the results count by (PlselWork's per-group arrays) */
#define PLSEL_LIST_END 0xFFFF       /* ends the u16 lists of quests and flags */

/* The item box menu (PlselItemBoxMenu, Plsel_ItemBoxMenu). */
enum PlselItemBoxState {
    IBOX_LIST,                  /* 0: the list of entries */
    IBOX_ENTRY,                 /* 1: inside an entry */
    IBOX_DONE                   /* 2: closed */
};
enum PlselItemBoxEntry {
    IBOX_ENTRY_BOX,             /* 0: take items from the supply box */
    IBOX_ENTRY_SELL,            /* 1: sell what is left */
    IBOX_ENTRY_FINISH           /* 2: finish */
};
enum PlselItemBoxStep {
    IBOX_BOX_PICK = 0,          /* the box: pick a stack */
    IBOX_BOX_SWAP = 1,          /* the box: pick the pouch slot to swap it with */
    IBOX_BOX_SWAPPED = 2,       /* the box: swapped */
    IBOX_SELL_ASK = 0,          /* the sale: the yes/no question */
    IBOX_SELL_DONE = 1          /* the sale: sold */
};
enum PlselItemBoxInfo {
    IBOX_INFO_NONE,             /* 0 */
    IBOX_INFO_BOX,              /* 1: the description of the box item */
    IBOX_INFO_POUCH             /* 2: of the pouch item */
};
#define IBOX_TIME_LIMIT 0xE10       /* the menu's clock, frames: 2 min */
#define IBOX_COLUMNS 8              /* the box grid: 8 columns of 6 rows */
#define IBOX_ROWS 6

/* The arena record menu (Plsel_SaveRecordMenu): PlselWork.menuStep, and
 * PlselWork.page as the first question's cursor. */
enum PlselArenaMenuStep {
    ARENA_MENU_ASK,             /* 0: keep the captured monster, or not */
    ARENA_MENU_PICK_SLOT,       /* 1: the slot to keep it in */
    ARENA_MENU_CONFIRM,         /* 2: overwrite a record (all three in use)? */
    ARENA_MENU_SAVED,           /* 3: kept */
    ARENA_MENU_TIMED_OUT        /* 4: the time ran out: the monster was released */
};
#define ARENA_CHOICE_KEEP 0
#define ARENA_CHOICE_SKIP 1

#endif /* PLSEL_H */
