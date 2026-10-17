#ifndef SAVE_H
#define SAVE_H

#include "common.h"
#include "item.h"
#include "arena_em.h"
#include "chredit.h"            /* CHARA_NAME_LEN */

/*
 * The character's save data (gSave, 0x5D4A60): the equipment box and the
 * equipped items, a pouch, the hunting records, the items obtained, and
 * the equipment flags. unk_3B4C80.c holds most of the functions that
 * read and write it.
 */
/* SaveData.equip and SaveData.equipIdx: the equipped items in the save's order: the weapon
 * first, then the armour types (item.h's enum EquipType) EQUIP_LEGS, EQUIP_HEAD ... EQUIP_WAIST. */
enum SaveEquipSlot {
    SAVE_EQUIP_WEAPON = 0,
    SAVE_EQUIP_LEGS = 1,
    SAVE_EQUIP_HEAD = 2,
    SAVE_EQUIP_BODY = 3,
    SAVE_EQUIP_ARMS = 4,
    SAVE_EQUIP_WAIST = 5,
    SAVE_EQUIP_COUNT = 6
};
#define BOX_NONE    0xFFFF      /* no box slot (SaveData.equipIdx, Box_FindFreeSlot, Box_AddItem) */

/* The ids of each equipment table, as many as its flag bytes in SaveData (Equip_IsValidId). */
#define ARMOR_ID_COUNT  300
#define BLADE_ID_COUNT  640
#define GUN_ID_COUNT    200

/* The bits of an item's flag byte (SaveData.armorFlags, bladeFlags, gunFlags), by bit number
 * (Equip_SetFlag, Equip_TestFlag). */
#define EQUIP_FLAG_OWNED    0   /* has been in the box (Box_AddItem, Box_SetSlot) */
#define EQUIP_FLAG_MADE     1   /* has been made: the lobby's forge sets it with EQUIP_FLAG_NEW the
                                 * first time */
#define EQUIP_FLAG_NEW      2   /* the new mark, which blinks in the lists (Save_ClearNewMarks) */

/* A gun's EquipItem.level: the upgrade level in bits 0-3 (Box_GunLevelUp, at most 4) and the
 * options above it. The barrel excludes the silencer and the shield (Box_SetGunBarrel); the
 * silencer is a light bowgun's, the shield a heavy bowgun's; the scope goes with either. */
#define GUN_LEVEL_MASK      0x0F
#define GUN_OPT_MASK        0xF0
#define GUN_OPT_SILENCER    0x10    /* light bowguns: model part 2; changes the monster's hit
                                     * reaction (LightBowgun_HasSilencer) */
#define GUN_OPT_BARREL      0x20    /* model part 1 (Bowgun_HasBarrel) */
#define GUN_OPT_SCOPE       0x40    /* the aim view zooms (Bowgun_HasScope) */
#define GUN_OPT_SHIELD      0x80    /* heavy bowguns: model part 2; guards (HeavyBowgun_HasShield) */

/* The stat kinds of EquipView_GetStat and Player_GetEquipStat (unk_3B4C80.c), over the set
 * in gEquipShown. The resistances are summed over the five armour pieces, in Player.resist's
 * order (the dragon resistance, Player.resist[4], is the last kind); the element and status
 * values are the weapon's (a blade's only: Weapon_ElementValue, Weapon_StatusValue), by
 * BladeData.elementType 1-4 and statusType 1-3. The weapon's dragon element has no kind
 * here: Player_GetWeaponStat's WSTAT_* (player.h) reads it. */
enum EquipStatKind {
    EQUIP_STAT_ATTACK = 0,          /* a blade's attack, or a gun's with its level and options */
    EQUIP_STAT_DEFENSE = 1,         /* the armour's (Armor_GetDefense) plus the weapon's */
    EQUIP_STAT_RES_FIRE = 2,
    EQUIP_STAT_RES_WATER = 3,
    EQUIP_STAT_RES_THUNDER = 4,
    EQUIP_STAT_RES_ICE = 5,
    EQUIP_STAT_FIRE = 6,            /* element type 1 */
    EQUIP_STAT_WATER = 7,           /* element type 2 */
    EQUIP_STAT_THUNDER = 8,         /* element type 3 */
    EQUIP_STAT_ICE = 9,             /* element type 4 */
    EQUIP_STAT_POISON = 10,         /* status type 1 (WSTAT_POISON) */
    EQUIP_STAT_STATUS_TYPE2 = 11,   /* status type 2 (WSTAT_STATUS_TYPE2) */
    EQUIP_STAT_STATUS_TYPE3 = 12,   /* status type 3 (WSTAT_STATUS_TYPE3) */
    EQUIP_STAT_RES_DRAGON = 13
};

/* The caps and gates of the save's counters. */
#define MONEY_MAX           9999999 /* SaveData.money (Save_AddMoney) */
#define HR_POINTS_MAX       9999999 /* SaveData.hrPoints (Save_AddHrPoints) */
#define POINTS_MAX          9999999 /* SaveData.points (Save_AddPoints) */
#define HR_MAX              999     /* the highest hunter rank (SaveData.hr) */
#define HR_TABLE_MAX        50      /* gHrRankPoints lists ranks 0-50; gHrPointSteps the ranks above */
#define HR_BAND_RANKS       50      /* the ranks of each gHrPointSteps band (one step per rank) */
#define RECORD_COUNT_MAX    9999    /* a monster's hunted or captured count, and their group sums */

#define BOX_PAGE_SLOTS      100     /* the slots of each page of the equipment box (Box_GetCapacity) */
#define ITEM_BITS_WORDS     32      /* SaveData.itemBits: one bit per item id */
#define EQUIP_LIST_END      9999    /* ends the id lists of the equipment collection awards */

/* Save_BuildSummary's and Save_SummarySize's modes (unk_3B4C80.c; the lobby's unk_7C45D0.c
 * calls them). */
#define SAVE_SUMMARY_EQUIPPED   1   /* the equipped items (EquipSummary) */
#define SAVE_SUMMARY_BOX        2   /* SUMMARY_BOX_SLOTS box slots from page * SUMMARY_BOX_SLOTS (BoxPage) */
#define SUMMARY_BOX_SLOTS       25

/* A monster that keeps its health between quests (SaveData.persistentEms). */
typedef struct PersistentEm {
    s16 hp;                     /* 0: none saved (QuestEm_LoadPersistentHp starts it full) */
    u16 partsBroken;            /* Em.partsBroken, gQuest.parts[] */
} PersistentEm;

/* A village offer (SaveData.offers, 0x14 bytes), one per offer NPC id. */
typedef struct OfferRec {
    u8 unk0[0xC];
    u8 unkC;                    /* which of the entry's 3 titles/texts */
    s8 unkD;                    /* nonzero: in use */
    u8 unkE[2];
    s16 unk10;                  /* index into gOfferTable */
    u8 unk12[2];
} OfferRec;

/*
 * The village mine (SaveData.mine, 0x5D824E): the item found, a bonus, the
 * digs left (1-2, 1-4 or 2-6 by pickaxe) and a state. Village_QuestTick
 * counts bonus down by digs once a quest while active is 1 (then 2), and
 * raises the village facility 0 when it reaches 0.
 */
typedef struct MineState {
    u16 find;                   /* 0x0: given to the pouch, then cleared with digs and active (unk_6B5300.c) */
    s16 bonus;                  /* 0x2 */
    u8 digs;                    /* 0x4 */
    u8 active;                  /* 0x5 */
} MineState;

/* A saved equipment set (SaveData.equipSets): box slots, 0xFFFF none. */
typedef struct EquipSet {
    u16 idx[6];
    u8 name[0x14];              /* 0xC: the set name (EquipSet_RenameInput) */
} EquipSet;

/* A village timer (SaveData.villageTimers): set from a milestone's time by
 * Village_ProcessEventFlags, counted down by Village_QuestTick. */
typedef struct VillageTimer {
    s32 left;                   /* counted down each quest, cleared at 0 */
    s16 unk4;
    s16 state;                  /* 1, 2: set by Village_ProcessEventFlags */
} VillageTimer;

/* SaveData.npcRecsA: one per entry of the lobby's gVillageNpcRecsA. */
typedef struct VillageNpcRecA {
    u16 id;
    s8 kind;                    /* 0-4 (Village_ProcessEventFlags) */
    s8 state;                   /* 0 free, 1 random kind, 2 set kind */
} VillageNpcRecA;

/* SaveData.npcRecsB: one per entry of the lobby's gVillageNpcRecsB. */
typedef struct VillageNpcRecB {
    u16 id;
    s8 flag;
    u8 pad;
} VillageNpcRecB;

typedef struct SaveData {
    u8 used;                    /* 0x0: nonzero: the slot holds a hunter (SaveMenu_SlotUsed, the save menus, plsel) */
    u8 gender;                  /* 0x1: 0 male, else female (see ArmorData.flags) */
    u8 face;                    /* 0x2: the face (ChrEdit.face, Chara_MakeSave); + 1 is Player.equip[1].id (plsel) */
    u8 hairStyle;               /* 0x3: the hair style (HairMenu_Apply, HairMenu_IsUnlocked; StageModels_LoadEquip(0xFF, style, gender)); Player +0x30E */
    s32 hairColor;              /* 0x4: the hair colour (HairMenu_Apply, HairMenu_ColorInput); Player +0x580 */
    s8 name[CHARA_NAME_LEN];    /* 0x8: the hunter's name (Player +0x82C) */
    u8 pad1A[2];
    PersistentEm persistentEms[16]; /* 0x1C: health and broken parts of the monsters that keep them between quests (QuestEm_SavePersistentHp, QuestEm_LoadPersistentHp, DrinkContest_HpStage), one per kind of gPersistentEmKinds (0x1BB9C8; QuestEm_LoadPersistentHp) */
    u32 hrPoints;               /* 0x5C: hunter rank points, at most 9,999,999 */
    s32 money;                  /* 0x60: at most 9,999,999 (flag 0x1E at 2,000,000) */
    u16 flags[44];              /* 0x64: event flags, one bit each (Save_SetEventFlag) */
    EquipItem box[500];         /* 0xBC: the equipment box, 100 per box page owned */
    ItemStack itemBox[500];     /* 0x182C: the item box, as many slots */
    u32 clearedQuests[16];      /* 0x1FFC: a bit per quest of gClearListQuests, set when its main goal is done (Save_MarkQuestCleared, Save_IsQuestCleared: the clear marks) */
    u32 playTime;               /* 0x203C: seconds, at most 35999999 (Lobby_CountPlayTime; Save_BuildSummary, CardMenu_DrawSlots) */
    ItemStack pouch[20];        /* 0x2040 */
    EquipItem equip[SAVE_EQUIP_COUNT]; /* 0x2090: the equipped items (enum SaveEquipSlot), copied from the box by Equip_Refresh */
    u8 voice;                   /* 0x20D8: Player.voice */
    u8 modeSelChoice;           /* 0x20D9: mode select's last choice, where its cursor starts (gm_main.c, GMSUB_MODE_SEL_*); a new character starts offline (Chara_MakeSave) */
    u8 pad20DA;
    u8 recipeBits[30];          /* 0x20DB: combination recipes known, one bit per list bit (0xED; D_005D6B3B, combine.c) */
    u8 pad20F9[0x211B - 0x20F9];
    u8 noteKnown[7];            /* 0x211B: the monster notes known, one bit per note (0x34; MonsterNote_SetKnown) */
    u8 pad2122;
    u8 comment[0x62];           /* 0x2123: the hunter's comment, a text of up to 0x60 bytes (the profile page: Chat_OpenKbd, Kbd_Run, Chat_FilterWords; the lobby copies it) */
    u8 pad2185;
    u16 equipIdx[SAVE_EQUIP_COUNT]; /* 0x2186: box index of each equipped item (enum SaveEquipSlot), BOX_NONE: none */
    u16 randSaved[32];          /* 0x2192: gRandSaved as saved (Rand_NextSaved); Save_LoadSlot sets random stream 2 (gRandSeeds[2]) from [0] */
    u16 sizeMax[0x80];          /* 0x21D2, per monster: largest size (percent, Monster_SizeFromPercent) */
    u16 sizeMin[0x80];          /* 0x22D2, per monster: smallest size (0: none yet) */
    u16 hunted[0x80];           /* 0x23D2, per monster: number hunted, at most 9999 */
    u16 captured[0x80];         /* 0x24D2, per monster: number captured, at most 9999 */
    u8 pad25D2[0x25DC - 0x25D2];
    u8 facilityLevel[8];        /* 0x25DC: per village facility, its level (Village_FacilityLevel; plsel raises them) */
    EquipSet equipSets[10];     /* 0x25E4: the saved equipment sets (the lobby's box menu, unk_760C70.c, unk_7683D0.c) */
    u8 unk2724;                 /* 0x2724: compared with 0x14 (the lobby's unk_7A8690.c) */
    u8 pad2725;
    u16 hr;                     /* 0x2726: hunter rank (from hrPoints, Save_RankFromPoints) */
    u8 season;                  /* 0x2728: the lobby world's season (Season_Set) */
    u8 day;                     /* 0x2729: the lobby world's day (gTownDay) */
    u8 pad272A[2];
    s32 time;                   /* 0x272C: the lobby clock (gLobbyTime) */
    u8 townDays;                /* 0x2730: copied to and from the lobby's save summary .days (unk_7D44B0.c) */
    u8 pad2731[0x2734 - 0x2731];
    u32 itemBits[ITEM_BITS_WORDS]; /* 0x2734: items obtained, one bit per item id (0x399) */
    /* 0x25DC: u8 D_005D703C[] (Village_FacilityLevel). */
    u16 rateFlags;              /* 0x27B4: the monster rates' flags (main unk_18DA50.c tests a bit per rate group); plsel sets it after a quest */
    u8 symbolColor;             /* 0x27B6: the symbol colour (gSymbolColorNames[it], StatusPage_DrawSymbolColor; Player_SetColorWord sets Player.style from it); gStyleSymbolColors[it] like styleFlag (Save_PickBestCounter) */
    u8 rateFree;                /* 0x27B7: the monster rates (main unk_18DA50.c): the free count */
    u8 rateLists[5][0x10];      /* 0x27B8: per rate group (gRateGroups), its monster kinds */
    u16 rateBase[0x60];         /* 0x2808: per monster kind, its rate value */
    u8 rateState[0x60];         /* 0x28C8: per monster kind, its trend and level bits */
    ArenaEm arenaEms[3];        /* 0x2928: the monsters kept for the arena: unkA the kind (nonzero: in use), unkE the name (ArenaMenu_*, ArenaCup_*), Versus_UpdateRecords, Quest_UpdateSaveRecords */
    u16 versusWins;             /* 0x297C: versus quests won (unk_39C290.c, at most 9999; award 0x8D at 50) */
    u16 versusLosses;           /* 0x297E: versus quests lost (at most 9999; award 0x8E at 50) */
    u32 versusMinutes;          /* 0x2980: the versus quests' time in minutes (gQuest.elapsed / 1800 added, at most 59999; Time_FormatMinSec) */
    s32 points;                 /* 0x2984: points (printed %7dP): added by unk_3B4C80.c (at most 9,999,999), spent in the lobby's point shop (unk_6CB690.c) */
    u32 mealPoints;             /* 0x2988: the village meal points: level * 12 per meal while below 0x136 (Meal_ApplyEffects); ready at 300 (Village_MealPointsReady); cleared by Kitchen_Update */
    u8 pad298C[0x2994 - 0x298C];
    u8 charType;                /* 0x2994: the TYPE panel's choice, 0-3 (ChrEdit.type, Chara_MakeSave); Player.charType, HunterCard.charType */
    u8 pad2995[0x2998 - 0x2995];
    u32 questCount;             /* 0x2998: quests counted (Records_CountQuest), at most 9999 */
    OfferRec offers[16];        /* 0x299C: the village offers, one per offer NPC id (the lobby's unk_6E7100.c) */
    u16 offerQuests[16];        /* 0x2ADC: the quest of each offer, 0 none (the lobby's unk_6E7100.c, unk_75E3B0.c) */
    u32 offerPoints[16];        /* 0x2AFC: points per offer NPC (the lobby's Offer_AddPoints, by Offer_IndexOf; Offer_CheckPointFlags every 24 and 32; reset above 2000) */
    u32 offerKindPoints[4];     /* 0x2B3C: the same points per NPC kind (gOfferNpcs[].kind), [3] the total; Village_CheckMilestone compares them before and after; reset above 100000 */
    u32 offerMilePoints[4];     /* 0x2B4C: points towards the next milestone of gOfferMilestonesA/B, per kind, [3] the total; reset above 2000 */
    s32 questKills;             /* 0x2B5C: Records_AddQuestKills, at most 9999 */
    u32 monsterFlags;           /* 0x2B60: bits of gMonsterFlagBits, set by Records_UpdateMonsterFlags (the lobby's Monster_IsHunted) */
    s32 dojoScore[40];          /* 0x2B64: per course: the best score (Dojo_GetBestScore) */
    u32 drillQuests;            /* 0x2C04: the drill quests played, cleared or not, at most 9999 (Records_CountFlagged) */
    s32 dojoTime[32];           /* 0x2C08: per course: the best time (Dojo_GetBestTime) */
    s32 pspLinkCount;           /* 0x2C88: links with MH Portable, at most 9,999,999 (PspLink_Count) */
    u32 pspLinkFlags;           /* 0x2C8C: what the MH Portable data had reached (PspLink_SetFlags, PspLink_HasFlag) */
    s16 stylePoints[3];         /* 0x2C90: points per play style: 0 captures (x5), 1 sub-goals (+3), 2 hunts (x2) (Save_AddCounterPoints); the best one picks the style (Save_PickBestCounter) */
    u8 styleFlag;               /* 0x2C96: gStyleSymbolColors[style], the rate flag set (RateFlag_SetAndNotify, RateFlag_SetQuiet) */
    u8 styleBits;               /* 0x2C97: bit style + 1 per style earned, 0 none yet (Save_PickBestCounter; the lobby tests it) */
    u8 poogieName[0x2CAA - 0x2C98];   /* 0x2C98: the Poogie's name (text), empty until named (PoogiePet_Open; then PoogieMenu_Init in the online house); drawn over the Poogie (kind 0x14, unk_78F070.c) */
    s8 poogieOutfit;            /* 0x2CAA: the Poogie's outfit (HunterCard.poogieOutfit; the town loads model 0xD + it); PoogieMenu_LastCursor counts owned outfits up to it + 1 */
    u8 unk2CAB;                 /* 0x2CAB: bits 0-2, one per kind (the lobby's unk_6E7100.c sets them, clears all at 7) */
    u16 recordCounts[48];       /* 0x2CAC: counters, at most 9999 (Records_UpdateCounters; plsel's awards: Plsel_CheckAwards); [17..47] at 0x2CCE: [32] and [33] raised by the lobby's unk_6C4E40.c, [35] by unk_6F8CC0.c, [40] set by unk_7B3140.c */
    u16 unk2D0C[24];            /* 0x2D0C: bits, one per id (n / 16, n % 16) (the lobby's unk_6E7100.c) */
    /* One flag byte per equipment id and type (Save_GetEquipFlags); bit 0 is set
     * when an item is put in the box, bit 2 marks it new (Save_ClearNewMarks). */
    u8 armorFlags[5][ARMOR_ID_COUNT]; /* 0x2D3C: EQUIP_FLAG_* by armour id, for the types EQUIP_HEAD, BODY, ARMS, WAIST and LEGS */
    u8 bladeFlags[BLADE_ID_COUNT]; /* 0x3318: EQUIP_FLAG_* by blade weapon id */
    u8 gunFlags[GUN_ID_COUNT];  /* 0x3598: EQUIP_FLAG_* by gun id */
    u8 titles[0x20];            /* 0x3660: a bit per hunter title earned (gHunterTitles; Save_SetTitle, Save_HasTitle;
                                 * the lobby sends it and tallies it, lobby_vobj.h's titleBits) */
    u8 pad3680[0x3688 - 0x3680];
    u8 unk3688[0x3695 - 0x3688]; /* 0x3688: 4 bits per entry of 0x1BB990 (main unk_18DA50.c, which indexes it as 0x20 bytes) */
    u8 title;                   /* 0x3695: the hunter's title (a gHunterTitles index; chosen in the lobby's unk_6B0570.c; compared with a quest entry byte in unk_7A44D0.c) */
    s16 unk3696;                /* 0x3696: the lobby (unk_6FE760.c, unk_6FF940.c) */
    s16 unk3698;                /* 0x3698: the lobby (unk_6FF8F0.c, unk_6FF940.c) */
    u8 days[3][3][8];           /* 0x369A: three days of quests (the lobby's world clock, unk_7B91F0.c) */
    s16 rotationDays[3];        /* 0x36E2: per monster kind of the rotation, the days until it picks again (the lobby's Rotation_Update, Rotation_RandomDays) */
    s8 unk36E8;                 /* 0x36E8: tested with condition 0x4F (unk_6BF390.c, unk_71B880.c, unk_71C390.c, unk_7B91F0.c) */
    u8 unk36E9;                 /* 0x36E9: a counter, compared with 50 (unk_7A8690.c), raised by a menu value (unk_6E3BF0.c) */
    u8 unk36EA[0x36EC - 0x36EA];
    VillageTimer villageTimers[7]; /* 0x36EC: one per milestone of the lobby's gVillageMilestones (Village_ProcessEventFlags, Village_QuestTick) */
    u8 unk3724[0x376C - 0x3724];
    VillageNpcRecA npcRecsA[16]; /* 0x376C: one per entry of the lobby's table gVillageNpcRecsA (Village_ProcessEventFlags) */
    VillageNpcRecB npcRecsB[16]; /* 0x37AC: one per entry of the lobby's table gVillageNpcRecsB (Village_ClearNpcRec) */
    u8 lastPersistentEm;        /* 0x37EC: persistentEms index of the last one saved (QuestEm_SavePersistentHp; DrinkContest_HpStage) */
    u8 npcRecCount;             /* 0x37ED: entries of npcRecsA filled (SaveRecView.count) */
    MineState mine;             /* 0x37EE: the village mine (the lobby's Mine_*; Village_QuestTick) */
    u8 pad37F4[2];
    u8 restriction;             /* 0x37F6: the member card restriction, 0 or 1 (SaveOpt37F6_Input; HunterCard.restriction, gRestrictTexts) */
    u8 pad37F7;
} SaveData;                     /* 0x37F8 bytes: Save_LoadSlot copies it whole */

extern SaveData gSave;

/* A hunter title (sub_main gHunterTitles, by SaveData.title): the records menu lists it under
 * its group's tab (Records_SetTitlePage, Records_TallyTitles). */
typedef struct HunterTitle {
    s8 group;                   /* 0x0: the tab it is listed under, 0 to 4 (Records_SetTitlePage compares it with RecordMenu.sel) */
    u8 unk1;                    /* 0x1: 0 or 1 (no reader seen) */
    char *text;                 /* 0x4 */
} HunterTitle;
typedef char HunterTitle_size_check[(sizeof(HunterTitle) == 0x8) ? 1 : -1];
extern HunterTitle gHunterTitles[148];

/* A monster note's definition: a row of gMonsterNoteDefs (sub_main, 52 rows, one per note), read by
 * the monster notes (MonsterNote_DrawPage), the size records' crowns (SizeRecord_DrawLarge and
 * SizeRecord_DrawSmall; plsel's awards) and the lobby's hunt records (Records_DrawMonsters). The
 * sizes are in percent, as SaveData.sizeMax and sizeMin. */
typedef struct MonsterNoteDef {
    u8 note;                    /* 0x0: the note's number (its bit in SaveData.noteKnown; gEmDescTexts) */
    u8 emType;                  /* 0x1: the monster kind (gMonsterNames; the Record_Group* and MonsterGroup_* tallies; MonsterNote_FindKind) */
    s16 bigCrown;               /* 0x2: the size from which the big crown is drawn, -1: no size records */
    s16 goldCrown;              /* 0x4: the size from which the big crown is gold */
    s16 smallCrown;             /* 0x6: the size up to which the small crown is drawn, -1: none */
    s16 unk08;                  /* 0x8: 0 in every row (no reader seen) */
    u16 knownItem;              /* 0xA: the item whose pickup makes the note known (MonsterNote_OnItemGet) */
    u8 emClass;                 /* 0xC: gEmClassNames index */
    u8 picture;                 /* 0xD: the picture (EntryPicture_GetUV), 0xFF: none (Menu_DrawEntryPicture) */
} MonsterNoteDef;
typedef char MonsterNoteDef_size_check[(sizeof(MonsterNoteDef) == 0xE) ? 1 : -1];
#define MONSTER_NOTE_COUNT 52
extern MonsterNoteDef gMonsterNoteDefs[MONSTER_NOTE_COUNT];

#endif /* SAVE_H */
