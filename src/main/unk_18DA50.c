/*
 * Small helpers over the save data: the season and the time of day, the
 * monsters' rate groups (Rate_*), the network quest's settings, the colour
 * editor's panel (ColorEdit_*), the awards and the rate flags (Award_Grant,
 * RateFlag_*) and the monster notes (MonsterNote_*).
 */
#include "common.h"
#include "m2c_macros.h"
#include "save.h"
#include "prim.h"
#include "sprite.h"
#include "quest.h"
#include "award.h"
extern u16 *gMonsterNoteMarks[];

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

typedef struct Rect {
    s16 x0;
    s16 y0;
    s16 x1;
    s16 y1;
    s32 color;
} Rect;

extern MonsterRateGroup gRateGroups[5];
extern u32 gRateListsInit[];
extern u16 gMonsterNoteIds[];
extern u8 *gMonsterGroups[];
extern u16 gFrameTick;        /* sub_main: the frame counter */
extern char *gSkillNames[];    /* sub_main */
extern u8 D_6ADB30;
extern u8 gSeasonFlags;         /* sub_main 0x5D4910 */
extern s32 gLoadLastVSync;      /* sub_main 0x5D4A50: gVSyncCount when the loader last ran a frame */
extern u8 gQuestSeasonFlags;    /* sub_main 0x5D87A9: the quest's season (Quest_GetSeason) */

void Quest_SetSeason(u8 arg0);
void NetQuest_SetRareRoll(s8 arg0);
void NetQuest_SetInfo(u8 *arg0);
void NetQuest_SetStage(u16 arg0);
s32 QuestId_IsStandard(s32 arg0);
s32 Quest_IsDownloadId(s32 arg0);
s32 Award_Grant(s32 arg0);
s32 RateFlag_Set(s32 bit, s32 flag);
s32 Save_TestEventFlag(s32 n);
void Quest_ClearVersusDefeat(void);
s8 Rate_GroupOf(s16 kind);
s32 MonsterNote_Index(u16 id);
u8 MonsterNote_GetFlags(u16 id);
s32 RateFlag_SetQuiet(u8 bit);
s32 Prim_FillRect(struct PrimLine *r);
void *memcpy(void *dst, const void *src, u32 n);
u8 Notice_PushItem(char *item);
void Save_SetTitle(s32 n);
s32 Save_HasTitle(s32 n);    /* sub_main: 1 when the save has award n */

/* Sets the quest's season: gQuestSeasonFlags gets the bit 1 << season. */
void Quest_SetSeason(u8 season) {
    gQuestSeasonFlags = 1 << (season & 0xFF);
}

/*
 * The time of day from gSeasonFlags: DAYTIME_DAY (0) when bit 3 is set,
 * DAYTIME_NIGHT (1) when bit 4 is set, else 0.
 */
u8 Daytime_Get(void)
{
    u8 flags = gSeasonFlags;

    if (flags & 8) {
        return 0;
    }
    if (flags & 0x10) {
        return 1;
    }
    return 0;
}

/*
 * Sets the time of day: clears bits 3 and 4 of gSeasonFlags and sets bit
 * time (DAYTIME_SET_DAY 3 or DAYTIME_SET_NIGHT 4).
 */
void Daytime_Set(s32 time)
{
    gSeasonFlags = (u8) (gSeasonFlags & ~0x18);
    gSeasonFlags = (u8) (gSeasonFlags | ((1 << (time & 0xFF)) & 0xFF));
}

/*
 * Sets up a new save's monster rates: copies gRateListsInit (0x50 bytes)
 * into save->rateLists and gives each of the 0x60 monster kinds the
 * middle of its rate group's range (lo to hi) in save->rateBase, 0 for a
 * kind in no group. Seen: the character creation (select).
 */
void Rate_InitSave(SaveData *save)
{
    s16 i;
    s8 group;

    memcpy(save->rateLists, ((u8 *)gRateListsInit), 0x50);
    for (i = 0; i < 0x60; i++) {
        group = Rate_GroupOf(i);
        if (group != -1) {
            save->rateBase[i] = (gRateGroups[group].hi + gRateGroups[group].lo) / 2;
        } else {
            save->rateBase[i] = 0;
        }
    }
}

/* Adds a monster kind to its group's list of 16 (the first free slot; the
 * next slot is cleared as the end marker) and uses up one free count. Both callers pass a
 * second argument (a count), which it does not read. */
void Rate_ListAdd(s16 kind, s32 unused)
{
    SaveData *save = &gSave;
    s16 i;
    s8 group = Rate_GroupOf(kind);

    if (group != -1) {
        for (i = 0; i < 0x10; i++) {
            if (save->rateLists[group][i] == 0) {
                (&save->rateLists[group][0])[(i + 1) % 16] = 0;
                if (kind <= 0 || kind >= 0x60) {
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

/* The rate group (0-4) of a monster kind, -1 when it is in none. */
s8 Rate_GroupOf(s16 kind)
{
    s8 group;
    s8 i;
    MonsterRateGroup *groupDef;

    for (group = 0; group < 5; group++) {
        groupDef = &gRateGroups[group];
        for (i = 0; i < groupDef->count; i++) {
            if (kind == groupDef->entry[i].kind) {
                return group;
            }
        }
    }
    return -1;
}

/* Sets the rate value of a monster kind and its state byte: the trend
 * (bits 3-4: 2 down, 1 up, 0 unchanged) and the level (bits 0-2: which
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
        trend = 2;
    } else if (save->rateBase[kind] > old) {
        trend = 1;
    } else {
        trend = 0;
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

extern s8 D_006ADB4C;
/*
 * Stores rareRoll for the network quest. Seen: Lobby_EnterRollRare's
 * result, or the byte after the season flags in a town status message.
 */
void NetQuest_SetRareRoll(s8 rareRoll) {
    D_006ADB4C = rareRoll;
}

/* Copies the network quest's 0x18-byte info block from info. */
void NetQuest_SetInfo(u8 *info) {
    memcpy(&D_6ADB30, (void *)info, 0x18);
}

extern s16 gNextQuestId;
/* Sets gNextQuestId to id. */
void NetQuest_SetStage(u16 id) {
    gNextQuestId = id;
}

/* 1 when the low 16 bits of id are at most 10000 (a standard quest). */
s32 QuestId_IsStandard(s32 id) {
    return (id & 0xFFFF) < 0x2711;
}

/* 1 when the low 16 bits of id are 50000 or more (a download quest). */
s32 Quest_IsDownloadId(s32 id) {
    return (id & 0xFFFF) >= 0xC350;
}

/*
 * Draws the colour editor's selection frame: a white rectangle at (x, y)
 * of width by height, with a near-black one inset by border pixels on
 * every side drawn over it, leaving a white border.
 */
void ColorEdit_DrawFrame(s16 x, s16 y, s16 width, s16 height, s16 border)
{
    Rect rect;
    s32 inset;

    rect.x0 = x;
    rect.y0 = y;
    rect.x1 = x + width;
    rect.y1 = y + height;
    rect.color = -1;
    Prim_FillRect((struct PrimLine *)&rect);
    inset = border;
    rect.x0 = x + inset;
    rect.y0 = y + inset;
    rect.x1 = x + width - inset;
    rect.y1 -= inset;
    rect.color = 0xFF010101;
    Prim_FillRect((struct PrimLine *)&rect);
}

/* The colour editor of the character edit (select): the panel (mode 0),
 * the R, G and B bars, the 16 swatches and the labels. */
extern u32 gPresetColors[16];              /* the swatches' colours */
void Gfx_SetTexFilter(s32 a);                   /* sub_main: sprite texture setup */
void Tex_Prepare(s32 n, s32 tex);
void Tex_Bind(s32 i);
s32 Sprite_Draw(Sprite *s);
s32 Prim_FillGradRect(PrimGradRect *r);
void Font_SetSpacing(u8 a, u8 b);                /* sub_main: text size */
void Font_SetPos(s16 x, s16 y);              /* text position */
void Font_SetColor(s32 color);                   /* text colour */
void Text_PrintfColored(const char *fmt, ...);  /* sub_main's Text_PrintfColored */

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
    Rect swatchRect;
    s16 i;
    u8 channelValue;
    s32 textColor;
    s32 rowY = 0;
    s16 topY;
    s16 leftX;
    s16 *swatchTopPtr;

    if (mode == 0) {
        Gfx_SetTexFilter(1);
        Tex_Prepare(1, 0xA7);
        Tex_Bind(0xA7);
        sprite.color = -1;
        sprite.x = x;
        sprite.y = y;
        sprite.w = 0x144;
        sprite.h = 0xDE;
        sprite.u0 = 0;
        sprite.v0 = 0;
        sprite.u1 = 0xA0;
        sprite.v1 = 0xA8;
        Sprite_Draw(&sprite);
    }
    for (i = 0; i < 3; i++) {
        leftX = x + 0x73;
        rowY = y + (i * 0x1D + 0x26);
        topY = rowY;
        if (i == selRow) {
            ColorEdit_DrawFrame(leftX - 2, topY - 2, 0xA9, 0x16, 2);
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
        barRect.y1 = barRect.y0 + 0x12;
        barRect.x1 = leftX + (s16)(0.64705884f * channelValue);
        barRect.color[2] = barRect.color[0];
        barRect.color[3] = barRect.color[1];
        Prim_FillGradRect(&barRect);
    }
    for (i = 0; i < 16; i++) {
        leftX = x + (i * 14 + 0x34);
        topY = y + 0x96;
        if (selRow != 3 || swatchIndex != i) {
            swatchRect.x0 = leftX;
            swatchRect.y0 = topY;
            swatchRect.x1 = swatchRect.x0 + 14;
            swatchRect.y1 = swatchRect.y0 + 14;
            swatchRect.color = gPresetColors[i];
            Prim_FillRect((struct PrimLine *)&swatchRect);
        }
    }
    swatchTopPtr = &swatchRect.y0;
    if (selRow == 3) {
        leftX = x + (s32)(swatchIndex * 14 + 0x34) - 3;
        topY = y + 0x92;
        ColorEdit_DrawFrame(leftX - 2, topY - 2, 0x14, 0x1A, 2);
        swatchRect.x0 = leftX;
        swatchRect.y0 = topY;
        swatchRect.x1 = swatchRect.x0 + 0x10;
        swatchRect.y1 = *swatchTopPtr + 0x16;
        swatchRect.color = gPresetColors[swatchIndex];
        Prim_FillRect((struct PrimLine *)&swatchRect);
    }
    Font_SetSpacing(0x12, 0x12);
    for (i = 0; i < 5; i++) {
        sprite.x = x + 0x16;
        rowY = y + (i * 0x1D + 0x26);
        sprite.y = rowY - 2;
        if (i == 4) {
            sprite.y += 0x12;
        }
        sprite.w = 0x18;
        sprite.h = 0x18;
        sprite.u0 = 0xF;
        sprite.v0 = 0;
        sprite.u1 = 0;
        sprite.v1 = 0xF;
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
        Tex_Prepare(1, 0x16B);
        Tex_Bind(0x16B);
        Sprite_Draw(&sprite);
        Font_SetColor(textColor);
        if (i == 4) {
            Font_SetPos(x + 0x34, rowY + 0x12);
        } else {
            Font_SetPos(x + 0x34, rowY);
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
            Text_PrintfColored("%s", gSkillNames[0x52]);
            break;
        case 4:
            Text_PrintfColored("%s", gSkillNames[0x53]);
            break;
        }
    }
}

extern s32 D_00446A24;
/*
 * Grants award awardId: when the save does not have it yet, sets it
 * (Save_SetTitle), pushes the award notice (Notice_PushItem) and returns
 * 1; returns 0 when the save already has it.
 */
s32 Award_Grant(s32 awardId) {
    if (Save_HasTitle(awardId) == 0) {
        Save_SetTitle(awardId);
        Notice_PushItem((char *) D_00446A24);
        return 1;
    }
    return 0;
}

s32 RateFlag_SetAndNotify(s32 styleFlag);
/*
 * Sets rate flag bit (0-15) in gSave.rateFlags; returns 0 when it was
 * set already. When that completes flags 0 to 10, AWARD_RATE_FLAGS is
 * granted and RATE_FLAG_ALL set: with notify 0 quietly (Save_SetTitle, then
 * RateFlag_Set(RATE_FLAG_ALL, 0)), else with notices (Award_Grant, then
 * RateFlag_SetAndNotify(RATE_FLAG_ALL)). After setting the flag it returns
 * Save_TestEventFlag(0xB6).
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
    return Save_TestEventFlag(0xB6);
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

extern s32 D_00446A28;
/*
 * RateFlag_Set(styleFlag, 1); when that returns 1, pushes the rate
 * notice (Notice_PushItem) and returns 1, else returns 0.
 */
s32 RateFlag_SetAndNotify(s32 styleFlag)
{
    if (RateFlag_Set(styleFlag, 1) == 1) {
        Notice_PushItem((char *) D_00446A28);
        return 1;
    }
    return 0;
}

/*
 * The index of monster kind id in gMonsterNoteIds (at most 0x49 entries,
 * ended by 0), or 0xFF when it is not there.
 */
s32 MonsterNote_Index(u16 id)
{
    s32 i;
    u16 *idPtr;
    s32 index;

    index = 0xFF;
    for (i = 0, idPtr = gMonsterNoteIds; i < 0x49; i++, idPtr++) {
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

    if (bitPos == 0xFF) {
        return 0;
    }
    bitPos *= 4;
    marks = (save->unk3688[bitPos >> 3] >> (bitPos & 7)) & 0xF;
    return marks + marks + 1;
}

/* The flags of every entry of the list gMonsterGroups[id] (ended by 0xFF), or'd. */
u8 MonsterNote_GetListFlags(u16 id)
{
    u8 flags = 0;
    u8 *entry = gMonsterGroups[id];
    u8 noteId;

    while ((noteId = *entry) != 0xFF) {
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
    if (bitPos == 0xFF) {
        return 0;
    }
    marks = gMonsterNoteMarks[bitPos];
    for (k = 0; k < 5; k++, marks += 4) {
        if (*marks == value) {
            if (k == 0) {
                return 1;
            }
            bitPos *= 4;
            save->unk3688[bitPos >> 3] |= 1 << ((bitPos & 7) + k - 1);
            return 1;
        }
    }
    return 0;
}

/* Clears gQuest.versusDefeat. */
void Quest_ClearVersusDefeat(void) {
    gQuest.versusDefeat = 0;
}

/* This file's initialised data. */
extern u32 D_0024C150[];
extern u32 D_001BB900[];
MonsterRateGroup gRateGroups[5] = {
#include "main/001BB2A0.inc"
};
u32 gRateListsInit[20] = {
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
u32 gTeamItemBits[20] = {
#include "main/001BB3F0.inc"
};
u16 D_001BB440[24] = {
#include "main/001BB440.inc"
};
u16 D_001BB470[24] = {
#include "main/001BB470.inc"
};
u16 D_001BB4A0[24] = {
#include "main/001BB4A0.inc"
};
u16 D_001BB4D0[24] = {
#include "main/001BB4D0.inc"
};
u16 D_001BB500[24] = {
#include "main/001BB500.inc"
};
u16 D_001BB530[24] = {
#include "main/001BB530.inc"
};
u16 D_001BB560[24] = {
#include "main/001BB560.inc"
};
u16 D_001BB590[24] = {
#include "main/001BB590.inc"
};
u16 D_001BB5C0[24] = {
#include "main/001BB5C0.inc"
};
u16 D_001BB5F0[24] = {
#include "main/001BB5F0.inc"
};
u16 D_001BB620[24] = {
#include "main/001BB620.inc"
};
u16 D_001BB650[24] = {
#include "main/001BB650.inc"
};
u16 D_001BB680[24] = {
#include "main/001BB680.inc"
};
u16 D_001BB6B0[24] = {
#include "main/001BB6B0.inc"
};
u16 D_001BB6E0[24] = {
#include "main/001BB6E0.inc"
};
u16 *gMonsterNoteMarks[26] = {
    D_001BB440, D_001BB470, D_001BB4A0, D_001BB4D0,
    D_001BB500, D_001BB530, D_001BB560, D_001BB590,
    D_001BB5C0, D_001BB5F0, D_001BB620, D_001BB650,
    D_001BB680, D_001BB6B0, D_001BB6B0, D_001BB6B0,
    D_001BB6E0, D_001BB4D0, D_001BB440, D_001BB470,
    D_001BB4A0, D_001BB4D0, D_001BB5C0, D_001BB500,
    D_001BB530, D_001BB4A0,
};
#pragma explicit_zero_data on
u32 D_001BB778[2] = {
    0, 0,
};
#pragma explicit_zero_data reset
u32 D_001BB780[8] = {
#include "main/001BB780.inc"
};
u32 D_001BB7A0[8] = {
#include "main/001BB7A0.inc"
};
u32 D_001BB7C0[8] = {
#include "main/001BB7C0.inc"
};
u32 D_001BB7E0[8] = {
#include "main/001BB7E0.inc"
};
u32 D_001BB800[8] = {
#include "main/001BB800.inc"
};
u32 D_001BB820[8] = {
#include "main/001BB820.inc"
};
u32 D_001BB840[8] = {
#include "main/001BB840.inc"
};
u32 D_001BB860[8] = {
#include "main/001BB860.inc"
};
u32 D_001BB880[8] = {
#include "main/001BB880.inc"
};
u32 D_001BB8A0[8] = {
#include "main/001BB8A0.inc"
};
u32 D_001BB8C0[8] = {
#include "main/001BB8C0.inc"
};
u32 D_001BB8E0[8] = {
#include "main/001BB8E0.inc"
};
u32 D_001BB900[36] = {
#include "main/001BB900.inc"
};
u16 gMonsterNoteIds[40] = {
#include "main/001BB990.inc"
};
