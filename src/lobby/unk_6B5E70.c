#include "common.h"
#include "sprite.h"
#include "save.h"
#include "em.h"
#include "lobby_vobj.h"
#pragma optimization_level 4
void DrawSlot_Free(s32);

/* prototypes */
void ModelObj_SpawnOnScaled(u8 *owner, s32 kind, f32 scale);
void ModelObj_Init(VObj *o);
void ModelObj_Run(VObj *o);
void ModelObj_End(VObj *o);
void ModelObj_Free(VObj *o);
/* end of prototypes */


s32 Cond_Check(u16 id);
s32 Items_HaveEnough(u16 item, s32 n);
void LobbySe_Play(s32 se);
s32 Menu_WrapUpDown(s32 cursor, s32 count);
extern u16 gMineDropsPick1[];
extern u16 gMineDropsPick2[];
extern u16 gMineDropsPick3[];
extern u16 gMineDropsPick1Open[];
extern u16 gMineDropsPick2Open[];
extern u16 gMineDropsPick3Open[];
extern u16 gMineDropsPick3Flag[];
extern s16 gMineBonusByLevel[];
u16 Rand_Next(s32 stream);
s32 Mine_IsOpen(void);
void Items_Take(s32 item, s32 n);
s32 Save_TestEventFlag(s32 flag);
u8 Village_FacilityLevel(s32 k);

void Mine_Dig(PickMenu *m)
{
    u16 *tbl;
    u16 item = m->items[m->cursor];
    s16 bonus;
    s32 r;

    Items_Take(item, 1);
    switch (item) {
    case 0x96:
        gSave.mine.digs = (Rand_Next(1) & 1) + 1;
        if (Mine_IsOpen() == 1) {
            tbl = gMineDropsPick1Open;
        } else {
            tbl = gMineDropsPick1;
        }
        break;
    case 0x97:
        gSave.mine.digs = Rand_Next(1) % 4 + 1;
        if (Mine_IsOpen() == 1) {
            tbl = gMineDropsPick2Open;
        } else {
            tbl = gMineDropsPick2;
        }
        break;
    case 0x98:
        gSave.mine.digs = Rand_Next(1) % 5 + 2;
        if (Mine_IsOpen() == 1) {
            tbl = gMineDropsPick3Open;
        } else if (Save_TestEventFlag(0x89) == 0) {
            tbl = gMineDropsPick3;
        } else {
            tbl = gMineDropsPick3Flag;
        }
        break;
    }
    gSave.mine.active = 1;
    r = tbl[Rand_Next(1) % 5];
    bonus = gSave.mine.bonus;
    gSave.mine.find = r;
    if (bonus == 0) {
        gSave.mine.bonus = gMineBonusByLevel[(u8)Village_FacilityLevel(0)];
    }
}

s32 PickMenu_Input(PickMenu *m, u16 pad)
{
    if (pad & 0x20) {
        if (m->count != 0) {
            LobbySe_Play(8);
            return 0;
        }
    } else if (pad & 0x40) {
        LobbySe_Play(4);
        return 3;
    } else if (pad & 0x200) {
        if (m->count != 0) {
            m->toggle ^= 1;
            LobbySe_Play(8);
        }
    } else {
        m->cursor = Menu_WrapUpDown(m->cursor, m->count);
    }
    return 2;
}

void Gfx_DrawLine(s16 x, s16 y, s16 w, s16 h, s32 color);
void ItemInfo_Draw(s16 x, s16 y, s16 w, u8 a3, s32 item, u16 t1, s8 t2, u32 color);
void Text_PrintfColored(const char *fmt, ...);
void Menu_DrawItemCountRow(s16 x, s16 y, u16 item, u32 color);
/* lobby_text.h declares it s16; this unit read it as an int (an implicit
 * declaration: Mine_DrawPickaxes sign-extends the result). */
/* unit's own declaration: with the definition's s16 (char *s, s16 x, s16 w) (lobby/unk_790EC0.c), Mine_DrawPickaxes: 11 of 176 words differ */
s32 Text_CenterX(char *s, s16 x, s32 size);
void Menu_DrawLabelBar(s16 x, s16 y, s16 w, u32 color, char *label, u8 t1, u8 t2);
void Menu_DrawLine(s16 x, s16 y0, s16 w, s16 y1, s8 flag);
void Win_Draw(s16 x, s16 y, s32 w, s32 h, u32 c1, u32 c2, s32 k); /* unit's own declaration: with the definition's void (s32 x, s32 y, s32 w, s32 h, s32 c1, s32 c2, s16 kind) (lobby/unk_790EC0.c), Mine_DrawPickaxes 6 of 176 words differ */
void Win_DrawText(s16 x, s16 y, s16 w, s16 h, u32 c1, u32 c2, char *s);
void Font_SetSpacing(u8 a, u8 b);
void Font_SetPos(s16 x, s16 y);
void Font_SetColor(s32 color);
extern char *gPickaxeSelectTexts[];
extern char *gPickaxeGiveTexts[];
extern char *D_0080C1EC;

void Mine_DrawPickaxes(PickMenu *m, s16 x, s16 y)
{
    s16 cy = y;
    s16 i;
    u16 item;
    u16 sel;

    Win_Draw(x, y, 0x154, 0x120, 0xBDB59F6C, 0xBB402010, 8);
    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(0x118, 0x12, 0xC8, 0xFFB0B000, gPickaxeSelectTexts[0], 2, 1);
    if (m->count == 0) {
        Font_SetSpacing(0x12, 0x12);
        Font_SetColor(0x802B7F7F);
        Font_SetPos(Text_CenterX(D_0080C1EC, x + 0xAA, 0x12), cy + 0x5A);
        Text_PrintfColored(D_0080C1EC);
        return;
    }
    Font_SetColor(0x807F7F7F);
    cy += 0x1E;
    Font_SetSpacing(0x14, 0x14);
    for (i = 0; i < 7; i++) {
        if (i <= m->count) {
            item = m->items[i];
            if (i == m->cursor) {
                sel = item;
                Menu_DrawLine(x + 0xA, cy - 1, 0x140, 0x18, 0);
            }
            Menu_DrawItemCountRow(x + 0x3C, cy, item, 0x807F7F7F);
        }
        cy += 0x16;
    }
    Gfx_DrawLine(x + 0x14, cy + 2, 0x12C, 2, 0xBDB59F6C);
    ItemInfo_Draw((s16)(x + 0x3C) - 0x32, cy + 4, 0x154, m->toggle, sel, 0, -1, 0x807F7F7F);
    Win_DrawText(0x118, 0x164, 0x154, 0x50, 0xBDB59F6C, 0xC0101010, gPickaxeGiveTexts[0]);
}

extern u8 gMenuWork[];
void Text_SetSize(s32 size);
void Mine_DrawPickaxes(PickMenu *m, s16 x, s16 y);
void Mine_DrawMenu(MenuTask *t)
{
    u8 *m = (u8 *)gMenuWork;

    Text_SetSize(t->textSize);
    switch (m[0x19]) {
    case 1:
    case 2:
        Mine_DrawPickaxes((PickMenu *)m, *(s16 *)m, 0x3A);
        break;
    }
}

void ModelObj_SpawnOn(void *owner, s32 kind)
{
    ModelObj_SpawnOnScaled((void *) owner, kind, 1.0f);
}

void ModelObj_Update(VObj *o);
void Vec_Copy(f32 *dst, f32 *src);
void ModelObj_SpawnAt(f32 *pos, s32 kind, s32 k7, f32 scale)
{
    VObj *o = StageObj_Alloc(1);

    if (o != NULL) {
        o->kind = 0x19;
        o->update = ModelObj_Update;
        o->draw = 0;
        o->owner = NULL;
        o->type = kind;
        o->var2 = k7;
        o->node = NULL;
        o->scale = scale;
        Vec_Copy((f32 *) &o->pos, pos);
    }
}

void ModelObj_SpawnOnScaled(u8 *owner, s32 kind, f32 scale)
{
    VObj *o = StageObj_Alloc(1);

    if (o != NULL) {
        o->kind = 0x19;
        o->update = ModelObj_Update;
        o->draw = 0;
        o->owner = owner;
        o->type = kind;
        o->angle = *(s32 *)(owner + 0xA4);
        o->node = NULL;
        o->scale = scale;
    }
}

void ModelObj_Update(VObj *o)
{
    u8 state;

    state = o->state;
    switch (state) {
    case 0:
        ModelObj_Init(o);
        break;
    case 1:
        ModelObj_Run(o);
        break;
    case 2:
        ModelObj_End(o);
        break;
    case 3:
        ModelObj_Free(o);
        break;
    }
}

/* A part of a village object drawn as a model (0x34 bytes; ModelObj_DrawPart). */
typedef struct VModelPart {
    /* 0x00 */ s16 slot;
    /* 0x02 */ s16 kind;
    /* 0x04 */ Vec3f size;
    /* 0x10 */ Vec3f pos;
    /* 0x1C */ f32 alpha;
    /* 0x20 */ u8 unk20[4];
    /* 0x24 */ u16 ang;
    /* 0x26 */ s16 unk26;
    /* 0x28 */ f32 scale;
    /* 0x2C */ DrawSlot *draw;
    /* 0x30 */ s16 unk30;
    /* 0x32 */ u16 unk32;
} VModelPart;

EmBone *Em_GetBone(void *obj, s16 bone);
void Mtx_Copy(f32 *dst, f32 *src);
void Vec_RotateSelf(f32 *v, f32 *m);
s16 DrawSlot_Alloc(void);
DrawSlot *DrawSlot_Get(s32 i);
void ModelObj_DrawPart(DrawSlot *slot);
extern s16 gModelObjPartCounts[];
extern s16 D_007DC4A0[];

/* Init (state 0) of the model objects (type 0x19): places the object at a
 * bone of its owner (kinds 0-2, 6, 8) and gives each of its gModelObjPartCounts[kind]
 * parts a draw slot drawn by ModelObj_DrawPart. */
void ModelObj_Init(VObj *o)
{
    f32 m[4][4];
    Vec3f v;
    VModelPart *p = (VModelPart *)o->buf;
    s16 n;
    s16 i;

    o->state++;
    o->var0 = 0;
    o->var1 = 0;
    o->visible = 1;
    o->draw = 0;
    o->timer = 0;
    n = gModelObjPartCounts[o->type];
    switch (o->type) {
    case 0:
        o->angle = Rand_Next(1);
    case 1:
        Mtx_Copy(m[0], (f32 *)Em_GetBone(o->owner, 10));
        v.x = 0.0f;
        v.y = 50.0f;
        v.z = 25.0f;
        Vec_RotateSelf(&v.x, m[0]);
        o->pos[0] = m[3][0] + v.x;
        o->pos[1] = m[3][1] + v.y;
        o->pos[2] = m[3][2] + v.z;
        break;
    case 2:
        if (o->owner != NULL) {
            Mtx_Copy(m[0], (f32 *)Em_GetBone(o->owner, 11));
            v.x = -26.4f;
            v.y = 2.8f;
            v.z = 7.7f;
            Vec_RotateSelf(&v.x, m[0]);
            o->pos[0] = m[3][0] + v.x;
            o->pos[1] = m[3][1] + v.y;
            o->pos[2] = m[3][2] + v.z;
        }
        break;
    case 6:
        Mtx_Copy(m[0], (f32 *)Em_GetBone(o->owner, 18));
        v.x = -8.0f;
        v.y = 0.0f;
        v.z = 70.0f;
        Vec_RotateSelf(&v.x, m[0]);
        o->pos[0] = m[3][0] + v.x;
        o->pos[1] = m[3][1] + v.y;
        o->pos[2] = m[3][2] + v.z;
        break;
    case 8:
        Mtx_Copy(m[0], (f32 *)Em_GetBone(o->owner, 20));
        v.x = 0.0f;
        v.y = 40.0f;
        v.z = 0.0f;
        Vec_RotateSelf(&v.x, m[0]);
        o->pos[0] = m[3][0] + v.x;
        o->pos[1] = m[3][1] + v.y;
        o->pos[2] = m[3][2] + v.z;
        break;
    }
    for (i = 0; i < n; p++, i++) {
        p->unk30 = 0;
        p->scale = o->scale;
        Vec_Copy((f32 *) &p->pos, (f32 *) &o->pos);
        p->kind = i;
        switch (o->type) {
        case 0:
            p->unk30 = D_007DC4A0[i] - 10;
            switch (p->kind) {
            case 0:
                p->ang = 0;
                p->unk26 = 0;
                break;
            case 1:
                p->ang = 0;
                p->unk26 = 0;
                p->scale *= 3.0f;
                break;
            case 2:
                p->ang = Rand_Next(1);
                p->unk26 = (Rand_Next(1) & 0x3ff) - 0x200;
                break;
            case 3:
                p->ang = Rand_Next(1);
                p->unk26 = (Rand_Next(1) & 0xFF) - 0x80;
                p->scale *= 3.0f;
                break;
            case 4:
            case 5:
            case 6:
                p->ang = Rand_Next(1);
                p->unk26 = (Rand_Next(1) & 0x3ff) - 0x200;
                p->scale *= 0.5f + 0.0003f * ((Rand_Next(1) & 0x3ff));
                break;
            }
            break;
        case 1:
            p->ang = 0;
            p->unk26 = (Rand_Next(1) & 0x7ff) - 0x400;
            p->scale *= 0.8f + 0.00040000002f * ((Rand_Next(1) & 0x3ff));
            p->unk32 = Rand_Next(1);
            break;
        case 2:
            p->ang = Rand_Next(1);
            p->unk26 = (Rand_Next(1) & 0x1ff) - 0x100;
            p->unk32 = Rand_Next(1);
            break;
        case 6:
            switch (p->kind) {
            case 3:
                p->ang = 0x4000;
                break;
            case 1:
            default:
                p->ang = 0;
                break;
            }
            p->unk26 = 0;
            break;
        case 7:
            p->unk30 = -i * 5 - 10;
            p->ang = 0x2000;
            p->unk26 = 0;
            break;
        case 8:
            p->unk30 = 0;
            p->ang = 0;
            p->unk26 = 0;
            p->size.x = 0.3f;
            p->size.y = 0.3f;
            p->size.z = 0.3f;
            break;
        }
        p->slot = DrawSlot_Alloc();
        if (p->slot != -1) {
            p->draw = DrawSlot_Get(p->slot);
            p->draw->owner = o;
            p->draw->part = i;
            p->draw->draw = ModelObj_DrawPart;
        } else {
            p->draw = NULL;
        }
    }
}

void Key_LerpVec3(f32 *k, f32 *out, f32 t);
void Key_LerpF32(f32 *k, f32 *out, f32 t);
s16 Cook_GetTime(void);
void Vec_RotateY(f32 *v, f32 a);
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);
extern f32 gCamMatrix[];
extern u16 gModelObjLimits[];
extern s16 gModelObjKeyIndex[];
extern s16 gModelObjSteps[];
extern u16 gModelObjDurations[];
extern u16 *D_007DCA60[];
extern void *gModelObjKeys[];

/* Run (state 1) of the model objects: animates each part from its key curves
 * (gModelObjKeys). At level 3. The duration is read as tbl[i] and the angle as
 * i * 0x5555 (MWCC creates those walkers itself, after the loop guard, as in
 * the game; walkers of our own were set before it). */
#pragma optimization_level 3
void ModelObj_Run(VObj *o)
{
    f32 m[4][4];
    Vec3f v;
    Vec3f w;
    VModelPart *p = (VModelPart *)o->buf;
    s16 n = gModelObjPartCounts[o->type];
    u16 lim;
    s16 key;
    s16 step;
    u16 dur;
    u16 *tbl;
    s16 i;
    s32 k;
    void *curve;

    if (o->type == 8) {
        lim = Cook_GetTime();
    } else {
        lim = gModelObjLimits[o->type];
    }
    key = gModelObjKeyIndex[o->type];
    step = gModelObjSteps[o->type];
    dur = gModelObjDurations[o->type];
    tbl = D_007DCA60[o->type];
    if (++o->timer >= lim) {
        o->state++;
        o->visible = 0;
        return;
    }
    if (o->type == 6) {
        if (*(u8 *)o->owner == 0) {
            o->state++;
            o->visible = 0;
            return;
        }
        Mtx_Copy(m[0], (f32 *)Em_GetBone(o->owner, 18));
        v.x = -8.0f;
        v.y = 0.0f;
        v.z = 70.0f;
        Vec_RotateSelf(&v.x, m[0]);
        o->pos[0] = m[3][0] + v.x;
        o->pos[1] = m[3][1] + v.y;
        o->pos[2] = m[3][2] + v.z;
    } else if (o->type == 7) {
        if (*(u8 *)o->owner == 0) {
            o->state++;
            o->visible = 0;
            return;
        }
    } else if (o->type == 8) {
        if (*(u8 *)o->owner == 0) {
            o->state++;
            o->visible = 0;
            return;
        }
        Mtx_Copy(m[0], (f32 *)Em_GetBone(o->owner, 20));
        v.x = 0.0f;
        v.y = 40.0f;
        v.z = 0.0f;
        Vec_RotateSelf(&v.x, m[0]);
        o->pos[0] = m[3][0] + v.x;
        o->pos[1] = m[3][1] + v.y;
        o->pos[2] = m[3][2] + v.z;
    }
    for (i = 0; i < n; i++, p++) {
        if (tbl != NULL) {
            dur = tbl[i];
        } else {
            key = gModelObjKeyIndex[o->type];
        }
        if (++p->unk30 <= 0) {
            key += step;
            continue;
        }
        if (p->unk30 == 1) {
            if (o->type == 7) {
                Mtx_Copy(m[0], (f32 *)Em_GetBone(o->owner, 18));
                v.x = -8.0f + ((Rand_Next(1) & 0x1F) - 16);
                v.y = (Rand_Next(1) & 0xF) - 8;
                v.z = 70.0f + ((Rand_Next(1) & 0x3F) - 32);
                Vec_RotateSelf(&v.x, m[0]);
                p->pos.x = m[3][0] + v.x;
                p->pos.y = m[3][1] + v.y;
                p->pos.z = m[3][2] + v.z;
            }
        } else if (dur < p->unk30) {
            key += step;
            continue;
        }
        v.x = 0.0f;
        v.y = 0.0f;
        v.z = 0.0f;
        switch (o->type) {
        case 0:
            k = key;
            curve = gModelObjKeys[k];
            Key_LerpVec3(curve, &p->size.x, p->unk30);
            Key_LerpVec3(gModelObjKeys[(s16)(k + 1)], &w.x, p->unk30);
            switch (p->kind) {
            default:
                Vec_RotateY((f32 *) &w, 6.2831855f * o->angle / 65536.0f);
                break;
            case 4:
            case 5:
            case 6:
                Vec_RotateY((f32 *) &w, 6.2831855f * (o->angle + i * 0x5555) / 65536.0f);
                break;
            }
            v.x += w.x;
            v.y += w.y;
            v.z += w.z;
            key += 3;
            curve = gModelObjKeys[(s16)(k + 2)];
            Key_LerpF32(curve, &p->alpha, p->unk30);
            break;
        case 1:
            k = key;
            curve = gModelObjKeys[k];
            Key_LerpVec3(curve, &p->size.x, p->unk30);
            key += 2;
            Key_LerpVec3(gModelObjKeys[(s16)(k + 1)], &w.x, p->unk30);
            Vec_RotateY((f32 *) &w, 6.2831855f * o->angle / 65536.0f);
            v.x = p->unk30 * (0.030000001f * ((p->unk32 & 0x3FF) - 0x200)) / dur;
            v.y = 0.0f;
            v.z = 0.0f;
            Vec_RotateSelf(&v.x, gCamMatrix);
            v.x += w.x;
            v.y += w.y;
            v.z += w.z;
            break;
        case 2:
            k = key;
            curve = gModelObjKeys[k];
            Key_LerpVec3(curve, &p->size.x, p->unk30);
            curve = gModelObjKeys[(s16)(k + 1)];
            Key_LerpVec3(curve, &w.x, p->unk30);
            Vec_RotateY((f32 *) &w, 6.2831855f * o->angle / 65536.0f);
            key += 3;
            Key_LerpF32(gModelObjKeys[(s16)(k + 2)], &p->alpha, p->unk30);
            v.x = p->unk30 * (0.010000001f * ((p->unk32 & 0x3FF) - 0x200)) / dur;
            v.y = 0.0f;
            v.z = 0.0f;
            Vec_RotateSelf(&v.x, gCamMatrix);
            v.x += w.x;
            v.y += w.y;
            v.z += w.z;
            v.x *= o->scale;
            v.y *= o->scale;
            v.z *= o->scale;
            break;
        case 7:
            curve = gModelObjKeys[key++];
            Key_LerpVec3(curve, &p->size.x, p->unk30);
            v.z = 5.0f * i;
            Vec_RotateSelf(&v.x, gCamMatrix);
            break;
        case 6:
            Vec_Copy((f32 *) &p->pos, (f32 *) &o->pos);
            k = key;
            curve = gModelObjKeys[k];
            Key_LerpVec3(curve, &p->size.x, p->unk30);
            key += 2;
            Key_LerpF32(gModelObjKeys[(s16)(k + 1)], &p->alpha, p->unk30);
            v.z = 5.0f * i;
            Vec_RotateSelf(&v.x, gCamMatrix);
            break;
        case 8:
            Vec_Copy((f32 *) &p->pos, (f32 *) &o->pos);
            break;
        }
        p->ang += p->unk26;
        if (p->draw != NULL) {
            p->draw->pos[0] = p->pos.x + v.x;
            p->draw->pos[1] = p->pos.y + v.y;
            p->draw->pos[2] = p->pos.z + v.z;
            OT_AddByDepth(gDrawOtSorted, p->draw, 0x40, 0);
        }
    }
}
#pragma optimization_level reset

extern s16 gModelObjPartCounts[];
void ModelObj_End(VObj *o)
{
    u8 *d = o->buf;
    s16 n;
    s16 i;

    o->state++;
    n = gModelObjPartCounts[o->type];
    if (o->node != NULL) {
        DrawSlot_Free(o->slot);
    }
    for (i = 0; i < n; i++, d += 0x34) {
        if (*(s32 *)(d + 0x2C) != 0) {
            DrawSlot_Free(*(s16 *)d);
        }
    }
}

/* unk_6B5E70's .data (0x7DBED0-0x7DCAC0) */
u16 gMineDropsPick1[8] = {
#include "lobby/007DBED0.inc"
};
u16 gMineDropsPick2[8] = {
#include "lobby/007DBEE0.inc"
};
u16 gMineDropsPick3[8] = {
#include "lobby/007DBEF0.inc"
};
u16 gMineDropsPick1Open[8] = {
#include "lobby/007DBF00.inc"
};
u16 gMineDropsPick2Open[8] = {
#include "lobby/007DBF10.inc"
};
u16 gMineDropsPick3Open[8] = {
#include "lobby/007DBF20.inc"
};
u16 gMineDropsPick3Flag[8] = {
#include "lobby/007DBF30.inc"
};
s16 gMineBonusByLevel[16] = {
#include "lobby/007DBF40.inc"
};
u32 gModelObjKeys_0[76] = {
#include "lobby/007DBF60.inc"
};
u32 gModelObjKeys_1[96] = {
#include "lobby/007DC090.inc"
};
u32 gModelObjKeys_2[8] = {
#include "lobby/007DC210.inc"
};
u32 gModelObjKeys_3[12] = {
#include "lobby/007DC230.inc"
};
u32 gModelObjKeys_6[20] = {
#include "lobby/007DC260.inc"
};
u32 gModelObjKeys_9[20] = {
#include "lobby/007DC2B0.inc"
};
u32 gModelObjKeys_12[16] = {
#include "lobby/007DC300.inc"
};
u32 gModelObjKeys_4[12] = {
#include "lobby/007DC340.inc"
};
u32 gModelObjKeys_7[12] = {
#include "lobby/007DC370.inc"
};
u32 gModelObjKeys_10[12] = {
#include "lobby/007DC3A0.inc"
};
u32 gModelObjKeys_13[16] = {
#include "lobby/007DC3D0.inc"
};
u32 gModelObjKeys_5[8] = {
#include "lobby/007DC410.inc"
};
u32 gModelObjKeys_8[8] = {
#include "lobby/007DC430.inc"
};
u32 gModelObjKeys_11[8] = {
#include "lobby/007DC450.inc"
};
u32 gModelObjKeys_14[8] = {
#include "lobby/007DC470.inc"
};
u16 gLobbyEffKind_0[8] = {
#include "lobby/007DC490.inc"
};
s16 D_007DC4A0[8] = {
#include "lobby/007DC4A0.inc"
};
u32 gModelObjKeys_21[20] = {
#include "lobby/007DC4B0.inc"
};
u32 gModelObjKeys_22[16] = {
#include "lobby/007DC500.inc"
};
u32 gModelObjColorKeys[8] = {
#include "lobby/007DC540.inc"
};
u32 gModelObjKeys_23[12] = {
#include "lobby/007DC560.inc"
};
u32 gModelObjKeys_24[12] = {
#include "lobby/007DC590.inc"
};
u32 gModelObjKeys_25[12] = {
#include "lobby/007DC5C0.inc"
};
u32 gModelObjPartColors[4] = {
#include "lobby/007DC5F0.inc"
};
u32 gModelObjKeys_26[20] = {
#include "lobby/007DC600.inc"
};
u32 gModelObjKeys_28[20] = {
#include "lobby/007DC650.inc"
};
u32 gModelObjKeys_30[20] = {
#include "lobby/007DC6A0.inc"
};
u32 gModelObjKeys_34[20] = {
#include "lobby/007DC6F0.inc"
};
u32 gModelObjKeys_36[20] = {
#include "lobby/007DC740.inc"
};
u32 gModelObjKeys_38[20] = {
#include "lobby/007DC790.inc"
};
u32 gModelObjKeys_27[8] = {
#include "lobby/007DC7E0.inc"
};
u32 gModelObjKeys_29[8] = {
#include "lobby/007DC800.inc"
};
u32 gModelObjKeys_31[8] = {
#include "lobby/007DC820.inc"
};
u32 gModelObjKeys_35[8] = {
#include "lobby/007DC840.inc"
};
u32 gModelObjKeys_37[8] = {
#include "lobby/007DC860.inc"
};
u32 gModelObjKeys_39[8] = {
#include "lobby/007DC880.inc"
};
u16 gLobbyEffKind_0_a[8] = {
#include "lobby/007DC8A0.inc"
};
u32 gModelObjKeys_40[24] = {
#include "lobby/007DC8B0.inc"
};
void *gModelObjKeys[44] = {
    (void *)gModelObjKeys_0, (void *)gModelObjKeys_1, (void *)gModelObjKeys_2, (void *)gModelObjKeys_3,
    (void *)gModelObjKeys_4, (void *)gModelObjKeys_5, (void *)gModelObjKeys_6, (void *)gModelObjKeys_7,
    (void *)gModelObjKeys_8, (void *)gModelObjKeys_9, (void *)gModelObjKeys_10, (void *)gModelObjKeys_11,
    (void *)gModelObjKeys_12, (void *)gModelObjKeys_13, (void *)gModelObjKeys_14, (void *)gModelObjKeys_12,
    (void *)gModelObjKeys_13, (void *)gModelObjKeys_14, (void *)gModelObjKeys_12, (void *)gModelObjKeys_13,
    (void *)gModelObjKeys_14, (void *)gModelObjKeys_21, (void *)gModelObjKeys_22, (void *)gModelObjKeys_23,
    (void *)gModelObjKeys_24, (void *)gModelObjKeys_25, (void *)gModelObjKeys_26, (void *)gModelObjKeys_27,
    (void *)gModelObjKeys_28, (void *)gModelObjKeys_29, (void *)gModelObjKeys_30, (void *)gModelObjKeys_31,
    (void *)gModelObjKeys_30, (void *)gModelObjKeys_31, (void *)gModelObjKeys_34, (void *)gModelObjKeys_35,
    (void *)gModelObjKeys_36, (void *)gModelObjKeys_37, (void *)gModelObjKeys_38, (void *)gModelObjKeys_39,
    (void *)gModelObjKeys_40, NULL, NULL, NULL,
};
s16 gModelObjPartCounts[16] = {
#include "lobby/007DC9C0.inc"
};
u16 gModelObjLimits[16] = {
#include "lobby/007DC9E0.inc"
};
s16 gModelObjKeyIndex[16] = {
#include "lobby/007DCA00.inc"
};
s16 gModelObjSteps[16] = {
#include "lobby/007DCA20.inc"
};
u16 gModelObjDurations[16] = {
#include "lobby/007DCA40.inc"
};
/* The per-kind tables (D_007DCA60, 10 kinds) run into the per-kind texture ids
 * unk_6B7830 reads (D_007DCA88, at 8 mod 16, so one object; both are aliases). */
typedef struct LobbyEffKind {
    u16 *tbl[10];
    s16 texId[12];
} LobbyEffKind;
LobbyEffKind gLobbyEffKind = {
#include "lobby/007DCA60.inc"
};
u32 gVObjSwingAmps[8] = {
#include "lobby/007DCAA0.inc"
};
