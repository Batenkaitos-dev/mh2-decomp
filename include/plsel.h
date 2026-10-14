#ifndef PLSEL_H
#define PLSEL_H

#include "common.h"
#include "item.h"              /* ItemStack */
#include "sprite.h"            /* Sprite (Sprite_Draw) */
#include "arena_em.h"
#include "quest.h"             /* gQuest (QuestState) */
#include "window.h"            /* ListPanel, ListWin, Panel, Frame */

/* The player select overlay (plsel.bin, loaded at 0xD60200). */


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
    s32 payout;                 /* 0x18C: (itemTotal + rewardTotal) * the rate percent (Save_AddZenny) */
    s32 zenny;                  /* 0x190: the money (Save_GetZenny, + payout) */
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
 * the results screen format string that prints the field (sub_main D_469EB0,
 * Plsel_BuildResultText). */
typedef struct PlselWork {
    s32 unk0000;                /* 0x000 */
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
    s32 hrpTotal;               /* 0x714: hunter rank points in all (t[48]; AddHrPoints) */
    s32 hrpDeaths;              /* 0x718: the deaths' points (deaths * -20) */
    s32 hrpMain;                /* 0x71C: the main goal's points (t[44]; t[53] must-order; t[51] failed) */
    s32 hrpSubA;                /* 0x720: sub goal A's points (t[45]) */
    s32 hrpSubB;                /* 0x724: sub goal B's points (t[46]) */
    u8 unk728[0x72C - 0x728];
    s32 rankBefore;             /* 0x72C: hunter rank before (Save_RankFromPoints) */
    s32 rankAfter;              /* 0x730: hunter rank after (t[50]) */
    s32 rankChange;             /* 0x734: 1: rank up, 2: down */
    s32 hrPoints;               /* 0x738: gSave.hrPoints after (t[49]) */
    s32 hrpResult;              /* 0x73C: AddHrPoints' result (which note to show) */
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
    u8 unk3B82;                 /* 0x3B82 */
    u8 unk3B83;                 /* 0x3B83 */
    s32 pointRate;              /* 0x3B84: the points' percent (Plsel_ResetPointRate) */
} PlselWork;
typedef char PlselWork_size_check[(sizeof(PlselWork) == 0x3B88) ? 1 : -1];

/* The work area's pointer, a sub_main variable. */
extern void *gOverlayWorkPtr; /* sub_main: the work pointer the overlays share */
#define gPlselWork (*(PlselWork **)&gOverlayWorkPtr)

/* A chat log entry (0x4F bytes, PlselSelWork.log): the message in up to two
 * lines and who sent it (Plsel_AddChatLog, Plsel_DrawChatLog). */
typedef struct PlselChatEntry {
    char lines[2][0x18];        /* 0x00: 22 bytes a line; a line whose byte 0x16 is set (a split two-byte character) is drawn narrower */
    u8 lineCount;               /* 0x30 */
    u8 who;                     /* 0x31: the sender's slot (the "%dP" tag) */
    u8 color[4];                /* 0x32: the sender's colours: [2] the name tag's, [3] the text's (NetUserRec.unk11C; else 0, the slot's twice, the chat class's) */
    char id[8];                 /* 0x36: the sender's user id; empty: no name tag */
    char name[0x11];            /* 0x3E: the sender's name */
} PlselChatEntry;
typedef char PlselChatEntry_size_check[(sizeof(PlselChatEntry) == 0x4F) ? 1 : -1];

/* plsel.c's view of the same work area (include/plsel_sel.spec). */
typedef struct PlselSelWork {
    u8 kbdOpen;                 /* 0x000: 1 while the chat keyboard is open (Plsel_ChatOpenKbd, Plsel_ChatUpdate) */
    u8 logHead;                 /* 0x001: the next chat log entry (wraps at 64; Plsel_AddChatLog) */
    u8 logCount;                /* 0x002: chat log entries (at most 0x40) */
    u8 unk003[0x004 - 0x003];
    s32 sendWait;               /* 0x004: frames before the next message may be sent (0x3C after one; Kbd_SetSubmitWait) */
    u8 logScroll;               /* 0x008: counts messages added past 11 log lines (at most 0x3F) */
    u8 unk009[0x00B - 0x009];
    PlselChatEntry log[64];     /* 0x00B: the chat log, a ring (logHead, logCount) */
    u8 unk13CB[0x13D4 - 0x13CB];
    u8 newMsg;                  /* 0x13D4: 1: a message was sent or came in */
    u8 newMsgMine;              /* 0x13D5: 1: it was the local player's */
    u8 unk13D6[0x13D8 - 0x13D6];
    s16 logShowTimer;           /* 0x13D8: frames the chat log stays shown (0x12C after a message) */
    u8 unk13DA[0x13DC - 0x13DA];
} PlselSelWork;
typedef char PlselSelWork_size_check[(sizeof(PlselSelWork) == 0x13DC) ? 1 : -1];

#define gPlselSelWork (*(PlselSelWork **)&gOverlayWorkPtr)

/* The list panels and windows of the overlay are window.h's ListPanel, ListWin and Panel. */

/* A preset hunter's equipment (14 bytes): the rows of gPlselPresets[set]
 * (four per set, Plsel_CurPreset) and of the stage records D_896D90[stage]
 * (Plsel_ApplyStageEquip). The armour ids go to Player.equip[2..5] and
 * [0] (Plsel_SetupSavedPlayer; the names gArmorHeadNames to gArmorLegsNames). */
typedef struct PlselPreset {
    u16 weaponType;             /* 0x0: the weapon's EquipItem type (6 blade, 7 gun) */
    u16 weaponId;               /* 0x2 */
    u16 weaponLevel;            /* 0x4 */
    u8 head;                    /* 0x6: armour ids, EquipItem types 2, 3, 4, 5 and 0 */
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
    u8 unk1;                    /* 0x1: Player.equip[1].id */
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

#endif /* PLSEL_H */
