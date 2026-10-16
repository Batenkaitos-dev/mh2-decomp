/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"
#include "save.h"
#include "prim.h"
#include "sprite.h"
#include "quest.h"
extern u16 *gMonsterNoteMarks[];

/* Five groups of monster kinds (0x1BB2A0), each with a range of values:
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
void NetQuest_SetUnk4C(s8 arg0);
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
void SaveFlag_Set(s32 n);
s32 SaveFlag_Test(s32 n);    /* sub_main: 1 when the save has award n */

void Quest_SetSeason(u8 season) {
    gQuestSeasonFlags = 1 << (season & 0xFF);
}

u8 Daytime_Get(void)
{
    u8 f = gSeasonFlags;

    if (f & 8) {
        return 0;
    }
    if (f & 0x10) {
        return 1;
    }
    return 0;
}

void Daytime_Set(s32 time)
{
    gSeasonFlags = (u8) (gSeasonFlags & ~0x18);
    gSeasonFlags = (u8) (gSeasonFlags | ((1 << (time & 0xFF)) & 0xFF));
}

void Rate_InitSave(SaveData *save)
{
    s16 i;
    s8 g;

    memcpy(save->rateLists, ((u8 *)gRateListsInit), 0x50);
    for (i = 0; i < 0x60; i++) {
        g = Rate_GroupOf(i);
        if (g != -1) {
            save->rateBase[i] = (gRateGroups[g].hi + gRateGroups[g].lo) / 2;
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
    s8 g = Rate_GroupOf(kind);

    if (g != -1) {
        for (i = 0; i < 0x10; i++) {
            if (save->rateLists[g][i] == 0) {
                (&save->rateLists[g][0])[(i + 1) % 16] = 0;
                if (kind <= 0 || kind >= 0x60) {
                    /* empty: probably a range check compiled out (the
                     * schedule needs it; found with decomp-permuter) */
                }
                (&save->rateLists[g][0])[i] = kind;
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
    s8 g;
    s8 i;
    MonsterRateGroup *p;

    for (g = 0; g < 5; g++) {
        p = &gRateGroups[g];
        for (i = 0; i < p->count; i++) {
            if (kind == p->entry[i].kind) {
                return g;
            }
        }
    }
    return -1;
}

/* Sets the rate value of a monster kind and its state byte: the trend
 * (bits 3-4: 2 down, 1 up, 0 unchanged) and the level (bits 0-2: which
 * sixth of its group's range the value is in). */
void Rate_Set(s32 kind, s32 val)
{
    SaveData *save = &gSave;
    u16 old;
    u8 trend;
    u8 st;
    s8 grp;
    MonsterRateGroup *g;
    u16 step;
    s8 lv;

    old = gSave.rateBase[kind];
    gSave.rateBase[kind] = val;
    if (old > gSave.rateBase[kind]) {
        trend = 2;
    } else if (save->rateBase[kind] > old) {
        trend = 1;
    } else {
        trend = 0;
    }
    st = trend * 8;
    grp = Rate_GroupOf(kind);
    g = &gRateGroups[grp];
    step = (g->hi - g->lo) / 6;
    for (lv = 1; lv < 7; lv++) {
        if (val < g->lo + step * lv || lv == 6) {
            st |= (u8)(lv - 1);
            break;
        }
    }
    save->rateState[kind] = st;
}

extern s8 D_006ADB4C;
void NetQuest_SetUnk4C(s8 v) {
    D_006ADB4C = v;
}

void NetQuest_SetInfo(u8 *info) {
    memcpy(&D_6ADB30, (void *)info, 0x18);
}

extern s16 gNextQuestId;
void NetQuest_SetStage(u16 id) {
    gNextQuestId = id;
}

s32 QuestId_IsStandard(s32 id) {
    return (id & 0xFFFF) < 0x2711;
}

s32 Quest_IsDownloadId(s32 id) {
    return (id & 0xFFFF) >= 0xC350;
}

void ColorEdit_DrawFrame(s16 x, s16 y, s16 w, s16 h, s16 border)
{
    Rect r;
    s32 b;

    r.x0 = x;
    r.y0 = y;
    r.x1 = x + w;
    r.y1 = y + h;
    r.color = -1;
    Prim_FillRect((struct PrimLine *)&r);
    b = border;
    r.x0 = x + b;
    r.y0 = y + b;
    r.x1 = x + w - b;
    r.y1 -= b;
    r.color = 0xFF010101;
    Prim_FillRect((struct PrimLine *)&r);
}

/* The colour editor of the character edit (select): the panel (mode 0),
 * the R, G and B bars, the 16 swatches and the labels. */
extern u32 gPresetColors[16];              /* the swatches' colours */
void Gfx_SetState63(s32 a);                   /* sub_main: sprite texture setup */
void Tex_Prepare(s32 n, s32 tex);
void Tex_Bind(s32 i);
s32 Sprite_Draw(Sprite *s);
s32 Prim_FillGradRect(PrimGradRect *r);
void Font_SetSpacing(u8 a, u8 b);                /* sub_main: text size */
void Font_SetPos(s16 x, s16 y);              /* text position */
void Font_SetColor(s32 color);                   /* text colour */
void Text_PrintfColored(const char *fmt, ...);  /* sub_main's Text_PrintfColored */

/* ly is declared `= 0` (a defensive initialiser: a constant in a declaration is created at
 * the top), which puts the label loop's counter and ly in s0/s1 as in the original (without
 * it, the other way round; round 17's permuter found the same with `ly = 0; spr.v0 = ly;`). The invariants are written inside
 * the loops on purpose: that gives the original's preheaders placed after the function's
 * end. Two earlier permuter finds: py (&rc.y0), as the original reuses the swatch loop's
 * hoisted &rc.y0 in the selected swatch's rectangle, and the bar loop's y computed in ly
 * (shared with the label loop) before ty. */
void ColorEdit_Draw(u8 mode, s16 x, s16 y, u8 r, u8 g, u8 b, u8 sel, u8 pal)
{
    PrimGradRect gr;
    Sprite spr;
    Rect rc;
    s16 i;
    u8 v;
    s32 col;
    s32 ly = 0;
    s16 ty;
    s16 bx;
    s16 *py;

    if (mode == 0) {
        Gfx_SetState63(1);
        Tex_Prepare(1, 0xA7);
        Tex_Bind(0xA7);
        spr.color = -1;
        spr.x = x;
        spr.y = y;
        spr.w = 0x144;
        spr.h = 0xDE;
        spr.u0 = 0;
        spr.v0 = 0;
        spr.u1 = 0xA0;
        spr.v1 = 0xA8;
        Sprite_Draw(&spr);
    }
    for (i = 0; i < 3; i++) {
        bx = x + 0x73;
        ly = y + (i * 0x1D + 0x26);
        ty = ly;
        if (i == sel) {
            ColorEdit_DrawFrame(bx - 2, ty - 2, 0xA9, 0x16, 2);
        }
        switch (i) {
        case 0:
            v = r;
            gr.color[0] = 0xFF400101;
            gr.color[1] = 0xFFFF0101;
            break;
        case 1:
            v = g;
            gr.color[0] = 0xFF014001;
            gr.color[1] = 0xFF01FF01;
            break;
        case 2:
            v = b;
            gr.color[0] = 0xFF010140;
            gr.color[1] = 0xFF0101FF;
            break;
        }
        gr.x0 = bx;
        gr.y0 = ty;
        gr.y1 = gr.y0 + 0x12;
        gr.x1 = bx + (s16)(0.64705884f * v);
        gr.color[2] = gr.color[0];
        gr.color[3] = gr.color[1];
        Prim_FillGradRect(&gr);
    }
    for (i = 0; i < 16; i++) {
        bx = x + (i * 14 + 0x34);
        ty = y + 0x96;
        if (sel != 3 || pal != i) {
            rc.x0 = bx;
            rc.y0 = ty;
            rc.x1 = rc.x0 + 14;
            rc.y1 = rc.y0 + 14;
            rc.color = gPresetColors[i];
            Prim_FillRect((struct PrimLine *)&rc);
        }
    }
    py = &rc.y0;
    if (sel == 3) {
        bx = x + (s32)(pal * 14 + 0x34) - 3;
        ty = y + 0x92;
        ColorEdit_DrawFrame(bx - 2, ty - 2, 0x14, 0x1A, 2);
        rc.x0 = bx;
        rc.y0 = ty;
        rc.x1 = rc.x0 + 0x10;
        rc.y1 = *py + 0x16;
        rc.color = gPresetColors[pal];
        Prim_FillRect((struct PrimLine *)&rc);
    }
    Font_SetSpacing(0x12, 0x12);
    for (i = 0; i < 5; i++) {
        spr.x = x + 0x16;
        ly = y + (i * 0x1D + 0x26);
        spr.y = ly - 2;
        if (i == 4) {
            spr.y += 0x12;
        }
        spr.w = 0x18;
        spr.h = 0x18;
        spr.u0 = 0xF;
        spr.v0 = 0;
        spr.u1 = 0;
        spr.v1 = 0xF;
        if (i == sel) {
            if (mode == 0) {
                col = 0xFF32DA5D;
                spr.color = 0xFFFB7103;
            } else {
                col = 0xFF32DA5D;
                spr.color = 0xFFC0C0C0;
            }
            if (gFrameTick & 0x10) {
                spr.x -= 4;
            }
        } else if (mode == 0) {
            col = 0xFF2A2120;
            spr.color = 0xFF342B2A;
        } else {
            col = 0xFFC0C0C0;
            spr.color = 0xFF342B2A;
        }
        Tex_Prepare(1, 0x16B);
        Tex_Bind(0x16B);
        Sprite_Draw(&spr);
        Font_SetColor(col);
        if (i == 4) {
            Font_SetPos(x + 0x34, ly + 0x12);
        } else {
            Font_SetPos(x + 0x34, ly);
        }
        switch (i) {
        case 0:
            Text_PrintfColored("R(%03d)", r);
            break;
        case 1:
            Text_PrintfColored("G(%03d)", g);
            break;
        case 2:
            Text_PrintfColored("B(%03d)", b);
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
s32 Award_Grant(s32 n) {
    if (SaveFlag_Test(n) == 0) {
        SaveFlag_Set(n);
        Notice_PushItem((char *) D_00446A24);
        return 1;
    }
    return 0;
}

s32 RateFlag_SetAndNotify(s32 styleFlag);
s32 RateFlag_Set(s32 bit, s32 flag)
{
    SaveData *save = &gSave;
    s32 m = 1 << ((u8)bit & 0xF);

    if (gSave.rateFlags & m) {    /* save->rateFlags: read through the save pointer, it is scheduled differently */
        return 0;
    }
    save->rateFlags |= m;
    if ((save->rateFlags & 0xFFF) == 0x7FF) {
        if ((u8)flag == 0) {
            if (SaveFlag_Test(0x7A) == 0) {
                SaveFlag_Set(0x7A);
            }
            RateFlag_Set(0xB, 0);
        } else {
            Award_Grant(0x7A);
            RateFlag_SetAndNotify(0xB);
        }
    }
    return Save_TestEventFlag(0xB6);
}

s32 RateFlag_SetQuiet(u8 bit)
{
    SaveData *save = &gSave;
    s32 m = 1 << (bit & 0xF);

    if (gSave.rateFlags & m) {    /* save->rateFlags: read through the save pointer, it is scheduled differently */
        return 0;
    }
    save->rateFlags |= m;
    if ((save->rateFlags & 0xFFF) == 0x7FF) {
        RateFlag_SetQuiet(0xB);
    }
    return 1;
}

extern s32 D_00446A28;
s32 RateFlag_SetAndNotify(s32 styleFlag)
{
    if (RateFlag_Set(styleFlag, 1) == 1) {
        Notice_PushItem((char *) D_00446A28);
        return 1;
    }
    return 0;
}

s32 MonsterNote_Index(u16 id)
{
    s32 i;
    u16 *p;
    s32 r;

    r = 0xFF;
    for (i = 0, p = gMonsterNoteIds; i < 0x49; i++, p++) {
        if (*p == 0) {
            break;
        }
        if (id == *p) {
            r = i;
            break;
        }
    }
    return r;
}

u8 MonsterNote_GetFlags(u16 id)
{
    SaveData *save = &gSave;
    s32 i = MonsterNote_Index(id);
    s32 v;

    if (i == 0xFF) {
        return 0;
    }
    i *= 4;
    v = (save->unk3688[i >> 3] >> (i & 7)) & 0xF;
    return v + v + 1;
}

/* The flags of every entry of the list gMonsterGroups[id] (ended by 0xFF), or'd. */
u8 MonsterNote_GetListFlags(u16 id)
{
    u8 f = 0;
    u8 *p = gMonsterGroups[id];
    u8 c;

    while ((c = *p) != 0xFF) {
        f |= MonsterNote_GetFlags(c);
        p++;
    }
    return f;
}

/* Records that entry id (an id of gMonsterNoteIds) reached val: when val is the
 * k-th value (k = 1-4) of its list at gMonsterNoteMarks, bit k - 1 of the entry's
 * four bits in the save data is set. 1 when val is in the list. */
s32 MonsterNote_Record(u16 id, u16 val)
{
    SaveData *save = &gSave;
    s32 i;
    u8 k;
    u16 *p;

    if (val == 0) {
        return 0;
    }
    i = MonsterNote_Index(id);
    if (i == 0xFF) {
        return 0;
    }
    p = gMonsterNoteMarks[i];
    for (k = 0; k < 5; k++, p += 4) {
        if (*p == val) {
            if (k == 0) {
                return 1;
            }
            i *= 4;
            save->unk3688[i >> 3] |= 1 << ((i & 7) + k - 1);
            return 1;
        }
    }
    return 0;
}

void Quest_ClearVersusDefeat(void) {
    gQuest.versusDefeat = 0;
}

/* unk_18DA50's .data (0x1BB2A0-0x1BB9E0) */
extern u32 D_0024C150[];
extern u32 D_001BB900[];
MonsterRateGroup gRateGroups[5] = {
#include "main/001BB2A0.inc"
};
u32 gRateListsInit[20] = {
#include "main/001BB380.inc"
};
u32 D_001BB3D0[28] = {
#include "main/001BB3D0.inc"
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
