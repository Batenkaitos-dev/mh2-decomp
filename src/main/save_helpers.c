/*
 * Small helpers over the save data: the HR gates, the training quests'
 * par times and records, the monster rates' snapshot, the quest record
 * (gQuestRecord, sent to the lobby server after a quest), the season and
 * the time of day, the monsters' rate groups (Rate_*), the next quest's
 * settings (NextQuest_*)
 * and the quest id ranges, the colour editor's panel (ColorEdit_*), the
 * awards and the rate flags (Award_Grant, RateFlag_*) and the monster
 * notes (MonsterNote_*), with the tables they read.
 */
#include "common.h"
#include "save.h"
#include "prim.h"
#include "sprite.h"
#include "quest.h"
#include "award.h"
#include "chat_log.h"
#include "render_state.h"
#include "area_draw.h"
#include "model.h"
#include "text.h"
#include "scene.h"
#include "font.h"
#include "libc.h"

/* The monster rates Rate_TakeSnapshot copies out: the first of
 * SaveData.rateBase. */
#define RATE_SNAPSHOT_COUNT 74
extern TeamEntry *gMonsterNoteMarks[];
extern NextQuest gNextQuest;        /* sub_main: the quest to start next */

/* Five groups of monster kinds (gRateGroups), each with a range of values:
 * the save data keeps a value per kind (+0x2808) and a trend and level
 * byte per kind (+0x28C8). */
typedef struct MonsterRateEntry {
    s16 weight;
    s16 kind;
} MonsterRateEntry;

typedef struct MonsterRateGroup {
    s32 lo;
    s32 hi;
    s32 count;
    MonsterRateEntry entry[8];
} MonsterRateGroup;

extern MonsterRateGroup gRateGroups[RATE_GROUPS];
extern u8 gRateListsInit[RATE_GROUPS][RATE_LIST_LEN];
extern u16 gFrameTick;        /* sub_main: the frame counter */
extern u8 gSeasonFlags;         /* sub_main 0x5D4910 */
extern s32 gLoadLastVSync;      /* sub_main 0x5D4A50: gVSyncCount when the loader last ran a frame */
extern u8 gQuestSeasonFlags;    /* sub_main 0x5D87A9: the quest's season (Quest_GetSeason) */

void NextQuest_SetFlyer(s8 arg0);
s32 Award_Grant(s32 arg0);
u16 Map_Group(u16, s32); /* unit's own declaration: the definition (sub_main) is u16 Map_Group(u16 map), which ignores a1; QuestRecord_Fill passes a1 = v, and its call compiles as the original's only with that second argument */
s32 Quest_GetOutcome(s32 withLoss); /* the definition (sub_main) is s32 Quest_GetOutcome(s32 withLoss): the call here passes a second argument */
/* gDojoParTimes' entry: a training quest and its target time. */
typedef struct DojoParTime {
    u16 questId;        /* 0 ends the table */
    u16 minutes;
} DojoParTime;

extern DojoParTime gDojoParTimes[];
extern u8 gLobbyRateStates[];
extern QuestRecord gQuestRecord;
extern u16 gQuestRecordIds[];
extern s32 gRateSnapshot[];

/* 1 when every quest of HR gate n (0-2: HR 11, 17 and 31) is cleared. */
s32 Save_HrGateCleared(s32 n)
{
    u16 *questId = gHrGateQuests[n];

    while (*questId != 0) {
        if (Save_IsQuestCleared(*questId) == 0) {
            return 0;
        }
        questId++;
    }
    return 1;
}

/*
 * The training quests: gDojoParTimes pairs a quest id with a target time in
 * minutes; the best score and time (in frames) of each are in sub_main.
 */
u16 Dojo_GetParMinutes(u16 id)
{
    s16 i;

    for (i = 0; gDojoParTimes[i].questId != 0; i++) {
        if (id == gDojoParTimes[i].questId) {
            return gDojoParTimes[i].minutes;
        }
    }
    return 0;
}

/* The index of training quest id in gDojoParTimes, or -1. */
s32 Dojo_FindQuest(u16 id)
{
    s16 i;

    for (i = 0; gDojoParTimes[i].questId != 0; i++) {
        if (id == gDojoParTimes[i].questId) {
            return i;
        }
    }
    return -1;
}

/* The best score of training quest id, 0 if it isn't one. */
s32 Dojo_GetBestScore(u16 id)
{
    s16 i;

    for (i = 0; gDojoParTimes[i].questId != 0; i++) {
        if (id == gDojoParTimes[i].questId) {
            return gSave.dojoScore[i];
        }
    }
    return 0;
}

/* The best time of training quest id in frames, 0 if it isn't one. */
s32 Dojo_GetBestTime(u16 id)
{
    s16 i;

    for (i = 0; gDojoParTimes[i].questId != 0; i++) {
        if (id == gDojoParTimes[i].questId) {
            return gSave.dojoTime[i];
        }
    }
    return 0;
}

/* Clears the monster rates' snapshot and the season flags. */
void Rate_ClearSnapshot(void)
{
    s32 i;

    for (i = 0; i < RATE_SNAPSHOT_COUNT; i++) {
        gRateSnapshot[i] = 0;
    }
    gSeasonFlags = 0;
}

s32 Rate_GetSnapshot(s32 i)
{
    return gRateSnapshot[i];
}

s32 Rate_Get(s32 i)
{
    return gSave.rateBase[i];
}

/* Copies the monster rates of the save data out: the first
 * RATE_SNAPSHOT_COUNT values (as s32) and every kind's trend and level
 * bits. */
void Rate_TakeSnapshot(void)
{
    SaveData *save = &gSave;
    s32 i;

    for (i = 0; i < RATE_SNAPSHOT_COUNT; i++) {
        gRateSnapshot[i] = save->rateBase[i];
    }
    memcpy(gLobbyRateStates, save->rateState, sizeof(save->rateState));
}

/* Clears gQuestRecord; returns it (memset's result). */
void *QuestRecord_Clear(void)
{
    return memset(&gQuestRecord, 0, sizeof(QuestRecord));
}

/*
 * Fills gQuestRecord for the results from sub_main's state: the quest, its
 * phase, map group, flyer, tier and time. The counts and the hunted and
 * captured tables (kept up during the quest) are cleared unless the first
 * player in the quest is the local one and the quest's goal is met.
 * Kept for the match:
 * - the pointer to the quest's map (info + 0x20, QuestInfo.map read as
 *   a u16) taken before the stores, and the empty check of the tier (0 to
 *   3; any two-test condition gives the same code), give the game's
 *   registers and load order;
 * - the time is stored through a cast of the field (`*(s32 *)&...elapsed`,
 *   which aliases like a raw address): stored as the field, the stores are
 *   scheduled differently.
 */
void QuestRecord_Fill(void)
{
    QuestRecord *rec = &gQuestRecord;
    s32 elapsed;
    u16 questId;
    u8 phase;
    u8 *info;
    s8 tier;
    s8 flyer;
    u8 *map;
    u8 playerCount;
    s32 i;
    s32 j;

    elapsed = gQuest.elapsed;
    questId = gQuest.stage;
    phase = gScene.questPhase;
    info = (u8 *)gQuest.info;
    map = info + 32;
    *(s32 *)&gQuestRecord.elapsed = elapsed;
    gQuestRecord.questId = questId;
    gQuestRecord.phase = phase;
    gQuestRecord.mapGroup = Map_Group(*(u16 *)map, elapsed);
    tier = gQuest.header->tier;
    flyer = gQuest.flyer;
    if (tier < 0 || tier > 3) { /* empty: for the match */
    }
    playerCount = gScene.playerCount;
    gQuestRecord.tier = tier;
    gQuestRecord.flyer = flyer;
    for (i = 0; i < playerCount; i++) {
        if (gScene.inQuest[i] == 1) {
            break;
        }
    }
    if (i != gScene.localPlayer || Quest_GetOutcome(1) != 1) {
        for (j = 0; j < QUEST_RECORD_IDS; j++) {
            rec->counts[j] = 0;
        }
        for (i = 0; i < QUEST_RECORD_KINDS; i++) {
            rec->hunted[i] = 0;
            rec->captured[i] = 0;
        }
    }
}

/* Counts a clear of quest id in gQuestRecord.counts, if gQuestRecordIds
 * lists it (the count stops at 255). */
void QuestRecord_Count(u16 id)
{
    s32 i;
    u16 *listed = gQuestRecordIds;

    for (i = 0; i < QUEST_RECORD_IDS; i++, listed++) {
        if (*listed == 0) {
            break;
        }
        if (*listed == id) {
            if (gQuestRecord.counts[i] < 255) {
                gQuestRecord.counts[i]++;
            }
            break;
        }
    }
}

/* The season (SEASON_*), from sub_main's season bits (Season_Set sets one). */
s32 Season_Get(void)
{
    u8 flags = gSeasonFlags;
    s32 season;

    if (flags & (1 << SEASON_WARM)) {
        season = SEASON_WARM;
    } else if (flags & (1 << SEASON_COLD)) {
        season = SEASON_COLD;
    } else {
        season = SEASON_BREEDING;
    }
    return season;
}

/* Sets the season bits to season (SEASON_*): bit season of the low three. */
void Season_Set(s32 season)
{
    gSeasonFlags = (u8) (gSeasonFlags & ~7);
    gSeasonFlags = (u8) (gSeasonFlags | ((1 << (season & 0xFF)) & 0xFF));
}

/* The current quest's season (SEASON_*; the same bits in the quest's data). */
s32 Quest_GetSeason(void)
{
    u8 flags = gQuestSeasonFlags;
    s32 season;

    if (flags & (1 << SEASON_WARM)) {
        season = SEASON_WARM;
    } else if (flags & (1 << SEASON_COLD)) {
        season = SEASON_COLD;
    } else {
        season = SEASON_BREEDING;
    }
    return season;
}

/* Sets the quest's season: gQuestSeasonFlags gets the bit 1 << season. */
void Quest_SetSeason(u8 season)
{
    gQuestSeasonFlags = 1 << (season & 0xFF);
}

/*
 * The time of day from gSeasonFlags: DAYTIME_DAY when bit DAYTIME_SET_DAY
 * is set, DAYTIME_NIGHT when bit DAYTIME_SET_NIGHT is, else DAYTIME_DAY.
 */
u8 Daytime_Get(void)
{
    u8 flags = gSeasonFlags;

    if (flags & (1 << DAYTIME_SET_DAY)) {
        return DAYTIME_DAY;
    }
    if (flags & (1 << DAYTIME_SET_NIGHT)) {
        return DAYTIME_NIGHT;
    }
    return DAYTIME_DAY;
}

/*
 * Sets the time of day: clears both time bits of gSeasonFlags and sets
 * bit time (DAYTIME_SET_DAY or DAYTIME_SET_NIGHT).
 */
void Daytime_Set(s32 time)
{
    gSeasonFlags = (u8) (gSeasonFlags & ~((1 << DAYTIME_SET_DAY) | (1 << DAYTIME_SET_NIGHT)));
    gSeasonFlags = (u8) (gSeasonFlags | ((1 << (time & 0xFF)) & 0xFF));
}

/*
 * Sets up a new save's monster rates: copies gRateListsInit into
 * save->rateLists and gives each of the RATE_KINDS monster kinds the
 * middle of its rate group's range (lo to hi) in save->rateBase, 0 for a
 * kind in no group. Seen: the character creation (select).
 */
void Rate_InitSave(SaveData *save)
{
    s16 i;
    s8 group;

    memcpy(save->rateLists, ((u8 *)gRateListsInit), sizeof(save->rateLists));
    for (i = 0; i < RATE_KINDS; i++) {
        group = Rate_GroupOf(i);
        if (group != RATE_GROUP_NONE) {
            save->rateBase[i] = (gRateGroups[group].hi + gRateGroups[group].lo) / 2;
        } else {
            save->rateBase[i] = 0;
        }
    }
}

/* Adds a monster kind to its group's list of RATE_LIST_LEN (the first free slot; the
 * next slot is cleared as the end marker) and uses up one free count. Both callers pass a
 * second argument (a count), which it does not read. */
void Rate_ListAdd(s16 kind, s32 unused)
{
    SaveData *save = &gSave;
    s16 i;
    s8 group = Rate_GroupOf(kind);

    if (group != RATE_GROUP_NONE) {
        for (i = 0; i < RATE_LIST_LEN; i++) {
            if (save->rateLists[group][i] == 0) {
                (&save->rateLists[group][0])[(i + 1) % RATE_LIST_LEN] = 0;
                if (kind <= 0 || kind >= RATE_KINDS) {
                    /* Kept for the match: an empty range check. It compiles to
                     * nothing, but the compiler orders the loop as the
                     * original does only with it. */
                }
                (&save->rateLists[group][0])[i] = kind;
                if (0 < save->rateFree) {
                    save->rateFree--;
                }
                return;
            }
        }
    }
}

/* The rate group (0-4) of a monster kind, RATE_GROUP_NONE when it is in none. */
s8 Rate_GroupOf(s16 kind)
{
    s8 group;
    s8 i;
    MonsterRateGroup *groupDef;

    for (group = 0; group < RATE_GROUPS; group++) {
        groupDef = &gRateGroups[group];
        for (i = 0; i < groupDef->count; i++) {
            if (kind == groupDef->entry[i].kind) {
                return group;
            }
        }
    }
    return RATE_GROUP_NONE;
}

/* Sets the rate value of a monster kind and its state byte: the trend
 * (bits 3-4: RATE_TREND_*) and the level (bits 0-2: which
 * sixth of its group's range the value is in). */
void Rate_Set(s32 kind, s32 value)
{
    SaveData *save = &gSave;
    u16 old;
    u8 trend;
    u8 state;
    s8 groupIndex;
    MonsterRateGroup *group;
    u16 step;
    s8 level;

    old = gSave.rateBase[kind];
    gSave.rateBase[kind] = value;
    if (old > gSave.rateBase[kind]) {
        trend = RATE_TREND_DOWN;
    } else if (save->rateBase[kind] > old) {
        trend = RATE_TREND_UP;
    } else {
        trend = RATE_TREND_SAME;
    }
    state = trend * 8;
    groupIndex = Rate_GroupOf(kind);
    group = &gRateGroups[groupIndex];
    step = (group->hi - group->lo) / 6;
    for (level = 1; level < 7; level++) {
        if (value < group->lo + step * level || level == 6) {
            state |= (u8)(level - 1);
            break;
        }
    }
    save->rateState[kind] = state;
}

/*
 * Sets gNextQuest.flyer: whether the area's flyer appears in the quest.
 * Seen: Lobby_EnterRollRare's roll (1 in about 9), or the roll a town
 * status message brings.
 */
void NextQuest_SetFlyer(s8 rareRoll)
{
    gNextQuest.flyer = rareRoll;
}

/* Copies the quest's three hunting goals into gNextQuest.huntGoals: the
 * town's wanted monsters of the season, or the ones a town message brings. */
void NextQuest_SetHuntGoals(u8 *info)
{
    memcpy(gNextQuest.huntGoals, (void *)info, sizeof(gNextQuest.huntGoals));
}

extern s16 gNextQuestId;
/* Sets the quest to start, gNextQuestId (gNextQuest.stage), to id. */
void NextQuest_SetStage(u16 id)
{
    gNextQuestId = id;
}

/* 1 when the low 16 bits of id are at most QUEST_ID_STANDARD_MAX (a standard quest). */
s32 QuestId_IsStandard(s32 id)
{
    return (id & 0xFFFF) <= QUEST_ID_STANDARD_MAX;
}

/* 1 when the low 16 bits of id are QUEST_ID_DOWNLOADED or more (a download quest). */
s32 Quest_IsDownloadId(s32 id)
{
    return (id & 0xFFFF) >= QUEST_ID_DOWNLOADED;
}

/*
 * Draws the colour editor's selection frame: a white rectangle at (x, y)
 * of width by height, with a near-black one inset by border pixels on
 * every side drawn over it, leaving a white border.
 */
void ColorEdit_DrawFrame(s16 x, s16 y, s16 width, s16 height, s16 border)
{
    PrimLine rect;
    s32 inset;

    rect.x0 = x;
    rect.y0 = y;
    rect.x1 = x + width;
    rect.y1 = y + height;
    rect.color = -1;
    Prim_FillRect(&rect);
    inset = border;
    rect.x0 = x + inset;
    rect.y0 = y + inset;
    rect.x1 = x + width - inset;
    rect.y1 -= inset;
    rect.color = 0xFF010101;
    Prim_FillRect(&rect);
}

/*
 * Draws the colour editor at (x, y): in mode 0 (the character edit) its
 * panel sprite first; mode 1 is the lobby's. Three bars show red, green
 * and blue (a gradient as long as the value, 165 pixels at 255), then the
 * 16 swatches of gPresetColors, then five labels, each with an arrow
 * sprite: the R, G and B values and two entries of gSkillNames. selRow
 * is the selected row (0-2 the bars, 3 the swatches, 4 the last label):
 * its bar gets a frame, its label's arrow is highlighted and blinks with
 * gFrameTick. With selRow 3 the swatch swatchIndex is drawn larger in a
 * frame.
 * Kept for the match: rowY is declared = 0, which puts the label loop's
 * counter and rowY in the same registers as the game (without it they
 * swap). The loop invariants are written inside the loops, which places
 * the loops' set-up code after the function's end as the game does.
 * swatchTopPtr points at swatchRect.y0, as the game reuses the swatch
 * loop's &swatchRect.y0 for the selected swatch's rectangle, and the bar
 * loop's y is computed in rowY (shared with the label loop) before topY.
 */
void ColorEdit_Draw(u8 mode, s16 x, s16 y, u8 red, u8 green, u8 blue, u8 selRow, u8 swatchIndex)
{
    PrimGradRect barRect;
    Sprite sprite;
    PrimLine swatchRect;
    s16 i;
    u8 channelValue;
    s32 textColor;
    s32 rowY = 0;
    s16 topY;
    s16 leftX;
    s16 *swatchTopPtr;

    if (mode == 0) {
        Gfx_SetTexFilter(GFX_TEX_BILINEAR);
        Tex_Prepare(1, 167);
        Tex_Bind(167);
        sprite.color = -1;
        sprite.x = x;
        sprite.y = y;
        sprite.w = 324;
        sprite.h = 222;
        sprite.u0 = 0;
        sprite.v0 = 0;
        sprite.u1 = 160;
        sprite.v1 = 168;
        Sprite_Draw(&sprite);
    }
    for (i = 0; i < 3; i++) {
        leftX = x + 115;
        rowY = y + (i * 29 + 38);
        topY = rowY;
        if (i == selRow) {
            ColorEdit_DrawFrame(leftX - 2, topY - 2, 169, 22, 2);
        }
        switch (i) {
        case 0:
            channelValue = red;
            barRect.color[0] = 0xFF400101;
            barRect.color[1] = 0xFFFF0101;
            break;
        case 1:
            channelValue = green;
            barRect.color[0] = 0xFF014001;
            barRect.color[1] = 0xFF01FF01;
            break;
        case 2:
            channelValue = blue;
            barRect.color[0] = 0xFF010140;
            barRect.color[1] = 0xFF0101FF;
            break;
        }
        barRect.x0 = leftX;
        barRect.y0 = topY;
        barRect.y1 = barRect.y0 + 18;
        barRect.x1 = leftX + (s16)(0.64705884f * channelValue);
        barRect.color[2] = barRect.color[0];
        barRect.color[3] = barRect.color[1];
        Prim_FillGradRect(&barRect);
    }
    for (i = 0; i < 16; i++) {
        leftX = x + (i * 14 + 52);
        topY = y + 150;
        if (selRow != 3 || swatchIndex != i) {
            swatchRect.x0 = leftX;
            swatchRect.y0 = topY;
            swatchRect.x1 = swatchRect.x0 + 14;
            swatchRect.y1 = swatchRect.y0 + 14;
            swatchRect.color = gPresetColors[i];
            Prim_FillRect(&swatchRect);
        }
    }
    swatchTopPtr = &swatchRect.y0;
    if (selRow == 3) {
        leftX = x + (s32)(swatchIndex * 14 + 52) - 3;
        topY = y + 146;
        ColorEdit_DrawFrame(leftX - 2, topY - 2, 20, 26, 2);
        swatchRect.x0 = leftX;
        swatchRect.y0 = topY;
        swatchRect.x1 = swatchRect.x0 + 16;
        swatchRect.y1 = *swatchTopPtr + 22;
        swatchRect.color = gPresetColors[swatchIndex];
        Prim_FillRect(&swatchRect);
    }
    Font_SetSpacing(18, 18);
    for (i = 0; i < 5; i++) {
        sprite.x = x + 22;
        rowY = y + (i * 29 + 38);
        sprite.y = rowY - 2;
        if (i == 4) {
            sprite.y += 18;
        }
        sprite.w = 24;
        sprite.h = 24;
        sprite.u0 = 15;
        sprite.v0 = 0;
        sprite.u1 = 0;
        sprite.v1 = 15;
        if (i == selRow) {
            if (mode == 0) {
                textColor = 0xFF32DA5D;
                sprite.color = 0xFFFB7103;
            } else {
                textColor = 0xFF32DA5D;
                sprite.color = 0xFFC0C0C0;
            }
            if (gFrameTick & 0x10) {
                sprite.x -= 4;
            }
        } else if (mode == 0) {
            textColor = 0xFF2A2120;
            sprite.color = 0xFF342B2A;
        } else {
            textColor = 0xFFC0C0C0;
            sprite.color = 0xFF342B2A;
        }
        Tex_Prepare(1, 363);
        Tex_Bind(363);
        Sprite_Draw(&sprite);
        Font_SetColor(textColor);
        if (i == 4) {
            Font_SetPos(x + 52, rowY + 18);
        } else {
            Font_SetPos(x + 52, rowY);
        }
        switch (i) {
        case 0:
            Text_PrintfColored("R(%03d)", red);
            break;
        case 1:
            Text_PrintfColored("G(%03d)", green);
            break;
        case 2:
            Text_PrintfColored("B(%03d)", blue);
            break;
        case 3:
            Text_PrintfColored("%s", gSkillNames[82]);
            break;
        case 4:
            Text_PrintfColored("%s", gSkillNames[83]);
            break;
        }
    }
}

/*
 * Grants award awardId: when the save does not have it yet, sets it
 * (Save_SetTitle), pushes the award notice (NOTICE_MSG_AWARD) and returns
 * 1; returns 0 when the save already has it.
 */
s32 Award_Grant(s32 awardId)
{
    if (Save_HasTitle(awardId) == 0) {
        Save_SetTitle(awardId);
        Notice_PushItem(gNoticeBannerTexts[NOTICE_MSG_AWARD]);
        return 1;
    }
    return 0;
}

/*
 * Sets rate flag bit (0-15) in gSave.rateFlags; returns 0 when it was
 * set already. When that completes flags 0 to 10, AWARD_RATE_FLAGS is
 * granted and RATE_FLAG_ALL set: with notify 0 quietly (Save_SetTitle, then
 * RateFlag_Set(RATE_FLAG_ALL, 0)), else with notices (Award_Grant, then
 * RateFlag_SetAndNotify(RATE_FLAG_ALL)). After setting the flag it returns
 * Save_TestEventFlag(182).
 */
s32 RateFlag_Set(s32 bit, s32 notify)
{
    SaveData *save = &gSave;
    s32 mask = 1 << ((u8)bit & 0xF);

    if (gSave.rateFlags & mask) {    /* save->rateFlags: read through the save pointer, it is scheduled differently */
        return 0;
    }
    save->rateFlags |= mask;
    if ((save->rateFlags & RATE_FLAGS_MASK) == RATE_FLAGS_FIRST_11) {
        if ((u8)notify == 0) {
            if (Save_HasTitle(AWARD_RATE_FLAGS) == 0) {
                Save_SetTitle(AWARD_RATE_FLAGS);
            }
            RateFlag_Set(RATE_FLAG_ALL, 0);
        } else {
            Award_Grant(AWARD_RATE_FLAGS);
            RateFlag_SetAndNotify(RATE_FLAG_ALL);
        }
    }
    return Save_TestEventFlag(182);
}

/*
 * Sets rate flag bit (0-15) in gSave.rateFlags without awards or
 * notices, and RATE_FLAG_ALL when flags 0 to 10 are then all set. Returns 1
 * when the flag was newly set, 0 when it was set already.
 */
s32 RateFlag_SetQuiet(u8 bit)
{
    SaveData *save = &gSave;
    s32 mask = 1 << (bit & 0xF);

    if (gSave.rateFlags & mask) {    /* save->rateFlags: read through the save pointer, it is scheduled differently */
        return 0;
    }
    save->rateFlags |= mask;
    if ((save->rateFlags & RATE_FLAGS_MASK) == RATE_FLAGS_FIRST_11) {
        RateFlag_SetQuiet(RATE_FLAG_ALL);
    }
    return 1;
}

/*
 * RateFlag_Set(styleFlag, 1); when that returns 1, pushes the rate
 * notice (NOTICE_MSG_RATE_FLAG) and returns 1, else returns 0.
 */
s32 RateFlag_SetAndNotify(s32 styleFlag)
{
    if (RateFlag_Set(styleFlag, 1) == 1) {
        Notice_PushItem(gNoticeBannerTexts[NOTICE_MSG_RATE_FLAG]);
        return 1;
    }
    return 0;
}

/*
 * The index of monster kind id in gMonsterNoteIds (at most
 * MONSTER_NOTE_IDS_MAX entries, ended by 0), or MONSTER_NOTE_NONE when it
 * is not there.
 */
s32 MonsterNote_Index(u16 id)
{
    s32 i;
    u16 *idPtr;
    s32 index;

    index = MONSTER_NOTE_NONE;
    for (i = 0, idPtr = gMonsterNoteIds; i < MONSTER_NOTE_IDS_MAX; i++, idPtr++) {
        if (*idPtr == 0) {
            break;
        }
        if (id == *idPtr) {
            index = i;
            break;
        }
    }
    return index;
}

/*
 * The monster note flags of monster kind id: 0 when it has no note,
 * else 1 with the entry's four recorded marks (MonsterNote_Record) in
 * bits 1-4.
 */
u8 MonsterNote_GetFlags(u16 id)
{
    SaveData *save = &gSave;
    s32 bitPos = MonsterNote_Index(id);
    s32 marks;

    if (bitPos == MONSTER_NOTE_NONE) {
        return 0;
    }
    bitPos *= 4;
    marks = (save->noteMarks[bitPos >> 3] >> (bitPos & 7)) & 0xF;
    return marks + marks + 1;
}

/* The flags of every entry of the list gMonsterGroups[id] (ended by 0xFF), or'd. */
u8 MonsterNote_GetListFlags(u16 id)
{
    u8 flags = 0;
    u8 *entry = gMonsterGroups[id];
    u8 noteId;

    while ((noteId = *entry) != 255) {
        flags |= MonsterNote_GetFlags(noteId);
        entry++;
    }
    return flags;
}

/*
 * Records that monster kind id reached value: when value is the k-th
 * value (k 1-4) of its list in gMonsterNoteMarks, bit k - 1 of the
 * entry's four bits in the save data is set. Returns 1 when value is in
 * the list (the 0th value records nothing), 0 when it is not, when value
 * is 0 or when the kind has no note.
 */
s32 MonsterNote_Record(u16 id, u16 value)
{
    SaveData *save = &gSave;
    s32 bitPos;
    u8 k;
    u16 *marks;

    if (value == 0) {
        return 0;
    }
    bitPos = MonsterNote_Index(id);
    if (bitPos == MONSTER_NOTE_NONE) {
        return 0;
    }
    marks = (u16 *)gMonsterNoteMarks[bitPos];
    for (k = 0; k < 5; k++, marks += 4) {
        if (*marks == value) {
            if (k == 0) {
                return 1;
            }
            bitPos *= 4;
            save->noteMarks[bitPos >> 3] |= 1 << ((bitPos & 7) + k - 1);
            return 1;
        }
    }
    return 0;
}

/* Clears gQuest.versusDefeat. */
void Quest_ClearVersusDefeat(void)
{
    gQuest.versusDefeat = 0;
}

/* Maps the byte at +0x19 of a record (0-12) to 0-9: pairs 0-1, 2-3, 4-5,
 * 6-7 give 0-3, then 8-12 give 4, 5, 6, 8, 9; anything else 9. No code
 * calls it, and no use shows what the byte is. */
s8 SaveRecord_KindIndex_Unk(u8 *record)
{
    switch ((s8)record[25]) {
    case 0:
    case 1:
        return 0;
    case 2:
    case 3:
        return 1;
    case 4:
    case 5:
        return 2;
    case 6:
    case 7:
        return 3;
    case 8:
        return 4;
    case 9:
        return 5;
    case 10:
        return 6;
    case 11:
        return 8;
    case 12:
        return 9;
    }
    return 9;
}

/* This file's initialised data. */
u32 gPresetColors[16] = {
#include "main/001BB140.inc"
};
u16 gHrGateQuests_0[8] = {
#include "main/001BB180.inc"
};
u16 gHrGateQuests_1[8] = {
#include "main/001BB190.inc"
};
u16 gHrGateQuests_2[8] = {
#include "main/001BB1A0.inc"
};
u16 gHrGateQuests_3[8] = {
#include "main/001BB1B0.inc"
};
u16 gHrGateQuests_4[8] = {
#include "main/001BB1C0.inc"
};
u16 gHrGateQuests_5[8] = {
#include "main/001BB1D0.inc"
};
u16 *gHrGateQuests[6] = {
    gHrGateQuests_0, gHrGateQuests_1, gHrGateQuests_2, gHrGateQuests_3,
    gHrGateQuests_4, gHrGateQuests_5,
};
#pragma explicit_zero_data on
u32 D_001BB1F8[2] = {
    0, 0,
};
#pragma explicit_zero_data reset
DojoParTime gDojoParTimes[24] = {
#include "main/001BB200.inc"
};
u32 D_001BB260[8] = {
#include "main/001BB260.inc"
};
u16 gQuestRecordIds[16] = {
#include "main/001BB280.inc"
};
extern u32 D_0024C150[];
MonsterRateGroup gRateGroups[RATE_GROUPS] = {
#include "main/001BB2A0.inc"
};
/* a new save's rateLists (Rate_InitSave copies it): per rate group, its
 * monster kinds */
u8 gRateListsInit[RATE_GROUPS][RATE_LIST_LEN] = {
#include "main/001BB380.inc"
};
/* The last quest id of each group of quest ids, 0 ending the list: Quest_Init
 * (sub_main) picks the group's table of stage files by it, the lobby's
 * Quest_GetDiscInfo the group's quest list. */
u16 gQuestGroupLastIds[16] = {
#include "main/001BB3D0.inc"
};
/* Versus_Init (sub_main) and the lobby's arena feed: per item (0 ends the
 * table) a value and the team bits. */
TeamEntry gTeamItemBits[10] = {
#include "main/001BB3F0.inc"
};
TeamEntry gMonsterNoteMarks_0[6] = {
#include "main/001BB440.inc"
};
TeamEntry gMonsterNoteMarks_1[6] = {
#include "main/001BB470.inc"
};
TeamEntry gMonsterNoteMarks_2[6] = {
#include "main/001BB4A0.inc"
};
TeamEntry gMonsterNoteMarks_3[6] = {
#include "main/001BB4D0.inc"
};
TeamEntry gMonsterNoteMarks_4[6] = {
#include "main/001BB500.inc"
};
TeamEntry gMonsterNoteMarks_5[6] = {
#include "main/001BB530.inc"
};
TeamEntry gMonsterNoteMarks_6[6] = {
#include "main/001BB560.inc"
};
TeamEntry gMonsterNoteMarks_7[6] = {
#include "main/001BB590.inc"
};
TeamEntry gMonsterNoteMarks_8[6] = {
#include "main/001BB5C0.inc"
};
TeamEntry gMonsterNoteMarks_9[6] = {
#include "main/001BB5F0.inc"
};
TeamEntry gMonsterNoteMarks_10[6] = {
#include "main/001BB620.inc"
};
TeamEntry gMonsterNoteMarks_11[6] = {
#include "main/001BB650.inc"
};
TeamEntry gMonsterNoteMarks_12[6] = {
#include "main/001BB680.inc"
};
TeamEntry gMonsterNoteMarks_13[6] = {
#include "main/001BB6B0.inc"
};
TeamEntry gMonsterNoteMarks_16[6] = {
#include "main/001BB6E0.inc"
};
TeamEntry *gMonsterNoteMarks[26] = {
    gMonsterNoteMarks_0, gMonsterNoteMarks_1, gMonsterNoteMarks_2, gMonsterNoteMarks_3,
    gMonsterNoteMarks_4, gMonsterNoteMarks_5, gMonsterNoteMarks_6, gMonsterNoteMarks_7,
    gMonsterNoteMarks_8, gMonsterNoteMarks_9, gMonsterNoteMarks_10, gMonsterNoteMarks_11,
    gMonsterNoteMarks_12, gMonsterNoteMarks_13, gMonsterNoteMarks_13, gMonsterNoteMarks_13,
    gMonsterNoteMarks_16, gMonsterNoteMarks_3, gMonsterNoteMarks_0, gMonsterNoteMarks_1,
    gMonsterNoteMarks_2, gMonsterNoteMarks_3, gMonsterNoteMarks_8, gMonsterNoteMarks_4,
    gMonsterNoteMarks_5, gMonsterNoteMarks_2,
};
#pragma explicit_zero_data on
u32 D_001BB778[2] = {
    0, 0,
};
#pragma explicit_zero_data reset
u8 gArenaGaugeUpChances_0[5][4] = {
#include "main/001BB780.inc"
};
u8 gArenaGaugeUpChances_1[5][4] = {
#include "main/001BB7A0.inc"
};
u8 gArenaGaugeUpChances_2[5][4] = {
#include "main/001BB7C0.inc"
};
u8 gArenaGaugeUpChances_3[5][4] = {
#include "main/001BB7E0.inc"
};
u8 gArenaGaugeUpChances_4[5][4] = {
#include "main/001BB800.inc"
};
u8 gArenaGaugeUpChances_5[5][4] = {
#include "main/001BB820.inc"
};
u8 gArenaGaugeUpChances_6[5][4] = {
#include "main/001BB840.inc"
};
u8 gArenaGaugeUpChances_7[5][4] = {
#include "main/001BB860.inc"
};
u8 gArenaGaugeUpChances_8[5][4] = {
#include "main/001BB880.inc"
};
u8 gArenaGaugeUpChances_9[5][4] = {
#include "main/001BB8A0.inc"
};
u8 gArenaGaugeUpChances_10[5][4] = {
#include "main/001BB8C0.inc"
};
u8 gArenaGaugeUpChances_12[5][4] = {
#include "main/001BB8E0.inc"
};
u8 gArenaGaugeUpChances_13[5][4] = {
#include "main/001BB900.inc"
};
u8 *gArenaGaugeUpChances[28] = {
#include "main/001BB920.inc"
};
u16 gMonsterNoteIds[40] = {
#include "main/001BB990.inc"
};
