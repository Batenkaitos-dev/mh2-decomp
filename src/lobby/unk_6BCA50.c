#include "common.h"
#include "sprite.h"
#include "lobby_text.h"
#include "em.h"
#include "m2c_macros.h"
#include "lobby_vobj.h"
#include "lobby_globals.h"
#pragma optimization_level 4
void *memset(void *, s32, u32);

/* prototypes */
void VFlock_Init(VObj *o);
void VFlock_Run(VObj *o);
void VFlock_End(void *arg0);
void VFlock_Free(VObj *o);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
void DrawSlot_Free(s32);
/* end of prototypes */

s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);
void VBird_Update(VFlock *w, VBird *b, s32 i);

void Key_LerpVec3(f32 *k, f32 *out, f32 t);
void Mtx_FromScaleRotTrans(f32 *scale, f32 *rot, f32 *trans, u16 order, f32 *m);
void Mtx_Concat3x3(f32 *m, f32 *a);
void Color_EvalKeys(s32 *k, s32 t, u32 *out);
s32 Effect_DrawModel(void *mdl, f32 *a1, s32 color, u16 flags, void *a4);
s32 Gs_SetState(u8 reg, u32 v);
extern f32 gCamMatrix[];
extern u8 gVMuzzleScaleKeys0[];
extern u8 gVMuzzleColorKeys0[];
extern u8 gVMuzzleScaleKeys1[];
extern u8 gVMuzzleColorKeys1[];
extern u8 gVMuzzleScaleKeys2[];
extern u8 gVMuzzleColorKeys2[];
void VMuzzle_Draw(DrawSlot *slot)
{
    VObj *o = slot->owner;
    Vec3f scale;
    Vec3f rot;
    u8 *scurve;
    u32 color;
    f32 m[16];
    VObjData *w = (VObjData *)o->buf;
    Model *mdl = gCommonModels[0];
    ModelMat *tex;
    ModelMatSet *set;
    u8 *ccurve;
    u16 flags = 0;

    if (mdl != NULL && mdl->active != 0 && o->visible != 0) {
        tex = mdl->mats;
        switch (o->type) {
        case 0:
            scurve = gVMuzzleScaleKeys0;
            ccurve = gVMuzzleColorKeys0;
            set = &mdl->matSets[18];
            flags |= 2;
            break;
        case 1:
            scurve = gVMuzzleScaleKeys1;
            ccurve = gVMuzzleColorKeys1;
            set = &mdl->matSets[1];
            flags |= 2;
            break;
        case 2:
            scurve = gVMuzzleScaleKeys2;
            ccurve = gVMuzzleColorKeys2;
            set = &mdl->matSets[6];
            break;
        }
        rot.x = 0.0f;
        rot.y = 0.0f;
        rot.z = 6.2831855f * *(s32 *)w / 65536.0f;
        Key_LerpVec3((f32 *) scurve, (f32 *) &scale, o->timer);
        Mtx_FromScaleRotTrans((f32 *) &scale.x, (f32 *) &rot.x, slot->pos, 6, (f32 *) m);
        Mtx_Concat3x3(m, gCamMatrix);
        Color_EvalKeys((s32 *) ccurve, (f32)o->timer, &color);
        Gs_SetState(0x6C, 0);
        Effect_DrawModel(set, m, color, flags, tex);
        Gs_SetState(0x6C, 1);
    }
}

#include "stageobj.h"
#include "lobby_menu.h"
void Vec_Normalize(f32 *v);
void Vec_Copy(f32 *dst, f32 *src);
void VMuzzle_Update(VObj *o);
extern f32 gCamMatrix[];

void VMuzzle_Spawn(f32 *pos)
{
    f32 p[3];
    f32 d[3];
    u8 i;
    StageObj *o;

    d[0] = -gCamMatrix[8];
    d[1] = -gCamMatrix[9];
    d[2] = -gCamMatrix[10];
    Vec_Normalize(d);
    p[0] = pos[0] + 75.0f * d[0];
    p[1] = pos[1] + 75.0f * d[1];
    p[2] = pos[2] + 75.0f * d[2];
    for (i = 0; i < 3; i++) {
        o = StageObj_Alloc(1);
        if (o != NULL) {
            o->kind = 0x26;
            o->type = i;
            o->update = VMuzzle_Update;
            Vec_Copy(o->pos, p);
        }
    }
}

extern u8 gMenuWork[];
extern u8 gMailBlockList[];
s32 IdList_Count(s8 *p, u8 n);
s32 WinSlide_Step(WinSlide *m, s16 from, s16 to);
/* The mail block list menu (gMenuWork while BlockList_* run). */
typedef struct BlockListMenu {
    WinSlide win;
    s16 sel;
    s16 count;
    s8 state;                   /* 2: the delete box (BlockList_DrawDelete) */
} BlockListMenu;
void BlockList_Draw(MenuTask *t);
void BlockList_Open(void)
{
    BlockListMenu *m = (BlockListMenu *)gMenuWork;

    memset(m, 0, 0xF00);
    m->count = IdList_Count((s8 *) gMailBlockList, 8);
    WinSlide_Step(&m->win, 0x280, 0x154);
    LobbyMenu_SetDraw(BlockList_Draw, 0, 0, 0);
    gFieldControl = 0;
}

void BlockList_Close(void)
{
    gFieldControl = 1;
    LobbyMenu_SetDraw(0, 0, NULL, 0);
}

typedef struct Entry5C3910 {
    u8 unk0[8];
    u8 name[16];
} Entry5C3910;
void LobbySe_Play(s32 se);
s32 Menu_WrapUpDown(s32 cur, s32 n);
s32 BlockList_RemoveInput(u16 pad);
void BlockList_AskRemove(void *e, u8 *name);
s32 BlockList_Update(u16 pad)
{
    BlockListMenu *m = (BlockListMenu *)gMenuWork;
    s32 state;

    switch (state = m->state) {
    case 0:
        switch (WinSlide_Step(&m->win, 0x280, 0x154)) {
        case 0:
            m->state++;
            break;
        }
        break;
    case 1:
        if (pad & 0x20) {
            if (m->count != 0) {
                m->state = state + 1;
                LobbySe_Play(8);
            }
        } else if (pad & 0x40) {
            BlockList_Close();
            LobbySe_Play(4);
            return 3;
        } else if (pad & 0x8000) {
            BlockList_Close();
            return 4;
        } else {
            m->sel = Menu_WrapUpDown(m->sel, m->count);
        }
        break;
    case 2:
        if (pad & 0x20) {
            m->state = state + 1;
            BlockList_AskRemove(&((Entry5C3910 *)gMailBlockList)[m->sel], ((Entry5C3910 *)gMailBlockList)[m->sel].name);
            LobbySe_Play(8);
        } else if (pad & 0x8000) {
            BlockList_Close();
            return 4;
        } else if (pad & 0x40) {
            m->state = state - 1;
            LobbySe_Play(4);
        }
        break;
    case 3:
        switch (BlockList_RemoveInput(pad)) {
        case 3:
            m->state--;
            break;
        case 0:
            m->count = IdList_Count((s8 *) gMailBlockList, 8);
            if (m->count > 0 && m->sel >= m->count) {
                m->sel = m->count - 1;
            }
            m->state = 1;
            break;
        case 4:
            BlockList_Close();
            return 4;
        }
        break;
    }
    return 2;
}

void Menu_DrawLine(s16 x, s16 y, s32 w, s32 h, s32 sel); /* unit's own declaration: with the definition's void (s16 x0, s16 y0, s16 x1, s16 y1, s8 flag) (lobby/unk_790EC0.c), BlockList_DrawDelete 43 of 67 words differ */
void Win_Draw(s16 x, s16 y, s32 w, s32 h, u32 c1, u32 c2, s32 k);
void Font_SetPos(s16 x, s16 y);
void Font_SetColor(s32 color);
void Text_PrintfColored(char *fmt, ...);
void Gfx_DrawLine(s16 x, s16 y, s16 w, s16 h, s32 color);
void Text_Printf(const char *fmt, ...);
void Sjis_Widen(char *src, char *dst);
s32 sprintf(char *buf, const char *fmt, ...);
void Icon_Draw(s16 x, s16 y, s16 w, s16 h, u32 color, s32 i);
extern char D_00825F20[];
extern char *D_0080C204;
extern char *D_0080C4A4;
extern char *D_0080C4AC;
extern char *D_0080D37C;
void BlockList_DrawList(void *mp, s16 x, s16 y)
{
    BlockListMenu *m = mp;
    s16 cy = y;
    s16 i;
    u8 hl;
    char buf[32];
    char wide[32];

    Win_Draw(x, y, 0x118, 0x122, 0xBDB59F6C, 0xBB402010, 8);
    Gfx_DrawLine(x + 8, y + 0xC, 0x108, 0x14, 0x80B59F6C);
    Font_SetColor(0x807F7F28);
    Font_SetPos(x + 0xA, (s16)(y + 0xE) - 1);
    Text_PrintfColored(D_0080C204);
    cy += 0x24;
    m->count = IdList_Count((s8 *) gMailBlockList, 8);
    if (m->count > 0) {
        if (m->sel >= m->count) {
            m->sel = m->count - 1;
        }
    } else {
        Font_SetPos(Text_CenterX(D_0080D37C, x + 0x8C, 0x12), cy + 0x46);
        Font_SetColor(0x802B7F7F);
        Text_Printf(D_00825F20, D_0080D37C);
        return;
    }
    Font_SetPos(x + 0x26, cy);
    Text_PrintfColored(D_00825F20, D_0080C4A4);
    Font_SetPos(x + 0x8C, cy);
    Text_PrintfColored(D_00825F20, D_0080C4AC);
    Font_SetColor(0x807F7F7F);
    for (i = 0; i < m->count; i++) {
        cy += 0x14;
        if (m->sel == i) {
            switch (m->state) {
            case 1:
                hl = 0;
                break;
            default:
                hl = 1;
                break;
            }
            Menu_DrawLine(x + 8, cy - 1, 0x108, 0x14, (s8)hl);
        }
        Font_SetPos(x + 0x26, cy);
        Text_PrintfColored(D_00825F20, ((Entry5C3910 *)gMailBlockList)[i].name);
        Font_SetPos(x + 0x8C, cy);
        sprintf(buf, D_00825F20, &((Entry5C3910 *)gMailBlockList)[i]);
        Sjis_Widen(buf, wide);
        Text_PrintfColored(D_00825F20, wide);
        Icon_Draw(x + 0xA, cy - 1, 0x16, 0x14, -1, 0x55);
        Icon_Draw(x + 0xA, cy - 1, 0x16, 0x14, -1, 0x58);
    }
}

void Menu_DrawLine(s16 x, s16 y, s32 w, s32 h, s32 sel); /* unit's own declaration: with the definition's void (s16 x0, s16 y0, s16 x1, s16 y1, s8 flag) (lobby/unk_790EC0.c), BlockList_DrawDelete 43 of 67 words differ */
void Win_Draw(s16 x, s16 y, s32 w, s32 h, u32 c1, u32 c2, s32 k);
void Font_SetPos(s16 x, s16 y);
void Font_SetColor(s32 color);
void Text_PrintfColored(char *fmt, ...);
extern char *D_0080C51C;
void BlockList_DrawDelete(void *m, s16 x, s16 y)
{
    s32 ty;
    s32 sel;
    s32 x0;
    s32 x1;

    x0 = x - 0x6E;
    Win_Draw(x0, y, 0x6E, 0x26, 0xBDB59F6C, 0xBB402010, 3);
    Font_SetColor(0x807F7F7F);
    switch (((s8 *)m)[0xC]) {
    case 2:
        sel = 0;
        break;
    default:
        sel = 1;
        break;
    }
    ty = y;
    x1 = (s16)x0 + 8;
    Menu_DrawLine(x1, (s16)(ty + 8) - 1, 0x5E, 0x14, sel);
    Font_SetPos(x1, ty + 8);
    Text_PrintfColored(D_0080C51C);
}

extern u8 gMenuWork[];
void Text_SetSize(s32 size);
void Font_SetSpacing(u8 a, u8 b);
void BlockList_DrawList(void *m, s16 x, s16 y);
void BlockList_DrawDelete(void *m, s16 a, s16 b);
void BlockList_Draw(MenuTask *t)
{
    BlockListMenu *m = (BlockListMenu *)gMenuWork;

    Text_SetSize(t->textSize);
    Font_SetSpacing(0x12, 0x12);
    BlockList_DrawList(m, m->win.pos, 0x36);
    switch (m->state) {
    case 2:
        BlockList_DrawDelete(m, m->win.pos, 0x36);
        break;
    }
}

s32 Sound_PlaySeAtPosChecked(s32, s32, s32, void *, s32, s32);
static void VBird_PlaySe(s32 se, VBird *b, s32 c)
{
    Sound_PlaySeAtPosChecked(6, se, 0, b, c, 0);
}

void VFlock_Update(StageObj *o)
{
    u8 state;

    state = o->state;
    switch (state) {
    case 0:
        VFlock_Init(o);
        break;
    case 1:
        VFlock_Run(o);
        break;
    case 2:
        VFlock_End(o);
        break;
    case 3:
        VFlock_Free(o);
        break;
    }
}

extern VBirdAnim gVBirdAnims[];

f32 Math_Cos(f32 a);
f32 Math_Sin(f32 a);
u16 Rand_Next(s32 stream);
static void VBird_StartArrival(VBird *e, f32 s)
{
    f32 a;
    f32 dx;
    f32 dz;
    f32 v;

    e->ang = Rand_Next(1) % 8;
    e->dest.x = e->pos.x;
    e->dest.y = 20.0f + e->pos.y;
    e->dest.z = e->pos.z;
    a = (6.2831855f * (u16)(((e->ang + 4) & 7) << 13)) / 65536.0f;
    dx = 8000.0f * Math_Sin(a);
    dz = 8000.0f * Math_Cos(a);
    e->pos.y += 4000.0f;
    e->pos.x += dx;
    e->pos.z += dz;
    e->vel.z = -15.0f * s;
    v = 20.0f * s;
    e->vel.x = v;
    e->vel.y = v;
    if (dx > 0.0f) {
        e->vel.x *= -1.0f;
    }
    if (dz > 0.0f) {
        e->vel.y *= -1.0f;
    }
}

static void VBird_SetAnim(VBird *m, s32 k)
{
    s8 *p;

    m->anim = k;
    m->mesh = gVBirdAnims[k].mesh;
    m->list = gVBirdAnims[k].list;
    m->idx = 0;
    if (m->list == NULL) {
        m->part = 0;
        m->frames = -1;
    } else {
        p = m->list + m->idx * 2;
        m->part = p[0];
        m->frames = p[1];
    }
}

static void VBird_Animate(VBird *m)
{
    s8 *p;

    if (m->frames != -1) {
        if (m->frames > 0) {
            m->frames--;
        } else {
            p = m->list;
            m->idx++;
            p += m->idx * 2;
            if (p[0] == -1) {
                m->idx = p[1];
            }
            m->part = p[0];
            m->frames = p[1];
            if (m->part == -1) {
                if (m->anim == 4) {
                    VBird_SetAnim(m, 2);
                    m->state = 2;
                    m->step = 0;
                    m->unk37 = 0;
                } else if (m->anim == 3) {
                    VBird_SetAnim(m, 0);
                    m->state = 0;
                    m->step = 0;
                    m->unk37 = 0;
                    m->timer = 0x3C;
                } else {
                    m->part = 0;
                }
            }
        }
    }
}

extern Vec3f gVBirdSpots[];
static void VBird_PickSpot(s8 *w, s32 i)
{
    VBird *e = (VBird *)(w + i * 0x4C + 0xC);
    s32 n;
    s32 j;
    f32 *s;

retry:
    n = Rand_Next(1) % 16;
    for (j = 0; j < 6; j++) {
        if (j != i && n == w[j]) {
            goto retry;
        }
    }
    w[i] = n;
    s = (f32 *)&gVBirdSpots[n];
    e->pos.x = *s++;
    e->pos.y = *s++;
    e->pos.z = *s;
    e->ang = 0;
    e->unk3C = 20.0f;
    e->state = 0;
    e->step = 0;
    e->timer = (Rand_Next(1) % 5 + 1) * 30;
    e->unk20 = 30;
    e->dir = Rand_Next(1) % 8;
    e->ang = (u16)e->dir << 13;
    VBird_SetAnim(e, 0);
}

s16 DrawSlot_Alloc(void);
DrawSlot *DrawSlot_Get(s32 i);
void VFlock_Draw(DrawSlot *slot);
extern s32 gBirdColors[];
void VFlock_Init(VObj *o)
{
    VFlock *w;
    VBird *e;
    s32 i;
    DrawSlot *d;

    o->slot = DrawSlot_Alloc();
    if (o->slot == -1) {
        o->state = 3;
        return;
    }
    w = (VFlock *)o->buf;
    d = DrawSlot_Get(o->slot);
    o->node = d;
    d->owner = (VObj *)w;
    d->draw = VFlock_Draw;
    *(Vec3f *)d->pos = *(Vec3f *)o->pos;
    o->state = 1;
    o->var0 = 0;
    w->scale = 1.2f;
    for (i = 0; i < 8; i++) {
        w->spot[i] = -1;
    }
    for (e = w->bird, i = 0; i < 6; i++, e++) {
        VBird_PickSpot(w->spot, i);
        if (Rand_Next(1) % 8 == 2) {
            e->color = gBirdColors[Rand_Next(1) % 3];
        } else {
            e->color = -1;
        }
    }
}

f32 Vec_DistanceXZ(f32 *a, f32 *b);
f32 Math_Absf(f32 x);
u16 Math_Heading(f32 x, f32 z, f32 a, f32 b);
s32 LobbyWorld_GetSeason(void);
void VBird_Update(VFlock *w, VBird *b, s32 i)
{
    Vec3f t;
    f32 d;
    Vec3f *p;

    switch (b->state) {
    case 0:
        switch (b->step) {
        case 0:
            if (b->unk20 != 0) {
                b->unk20--;
            } else {
                b->unk20 = 30;
                if (!(Rand_Next(1) & 1)) {
                    b->dir = Rand_Next(1) & 0xF;
                    b->ang = (u16)b->dir << 12;
                    break;
                }
            }
            if (b->timer != 0) {
                b->timer--;
            } else if (Rand_Next(1) % 10 <= 2 && b->ang > 0 && b->ang <= 0x5FFF) {
                VBird_SetAnim(b, 4);
                b->unk37++;
            } else {
                b->timer = (Rand_Next(1) % 5 + 1) * 30;
            }
            break;
        case 1:
            break;
        }
        break;
    case 1:
        switch (b->step) {
        case 0:
            if (b->called == 0) {
                t.x = b->dest.x;
                t.y = b->dest.y;
                t.z = b->dest.z;
                d = Vec_DistanceXZ((f32 *) &t, (f32 *) &b->pos);
                d = Math_Absf(d) / Math_Absf(b->vel.x);
                if ((s32)d <= 0x2A) {
                    VBird_PlaySe(0xE7, b, 0x13);
                    b->called = 1;
                }
            }
            if (Math_Absf(b->dest.x - b->pos.x) <= 20.0f) {
                b->pos.x = b->dest.x;
            } else {
                b->pos.x += b->vel.x;
            }
            if (Math_Absf(b->dest.z - b->pos.z) <= 20.0f) {
                b->pos.z = b->dest.z;
            } else {
                b->pos.z += b->vel.y;
            }
            if (Math_Absf(b->dest.y - b->pos.y) <= 20.0f) {
                b->pos.y = b->dest.y;
            } else {
                b->pos.y += b->vel.z;
            }
            if (b->pos.x == b->dest.x && b->pos.z == b->dest.z && b->pos.y == b->dest.y) {
                VBird_SetAnim(b, 2);
                b->vel.z = -4.0f;
                b->timer = 12;
                b->step++;
            } else {
                b->ang = Math_Heading(b->dest.x, b->dest.z, b->pos.x, b->pos.z);
                b->ang += 0x4000;
            }
            break;
        case 1:
            b->timer--;
            b->pos.y += b->vel.z;
            if (b->vel.z < -0.1f) {
                b->vel.z += 0.3f;
            } else {
                b->vel.z = -0.1f;
            }
            p = &gVBirdSpots[w->spot[i]];
            if (p->y > b->pos.y) {
                b->pos.y = p->y;
            }
            if (b->timer == 0) {
                b->pos.y = p->y;
                VBird_SetAnim(b, 3);
                b->step++;
            }
            break;
        case 2:
            break;
        }
        break;
    case 2:
        switch (b->step) {
        case 0:
            b->vel.x = 0.5f;
            b->vel.y = 0.5f;
            b->vel.z = 4.0f * w->scale;
            b->accel = 0.3f * w->scale;
            b->climb = 0.0f;
            b->timer = 6;
            b->step++;
            VBird_PlaySe(0xE6, b, 0x13);
        case 1:
            b->timer--;
            b->pos.y += b->vel.z;
            b->pos.x += b->vel.x * Math_Sin((6.2831855f * b->ang) / 65536.0f);
            b->pos.z += b->vel.y * Math_Cos((6.2831855f * b->ang) / 65536.0f);
            if (b->vel.z > 0.1f) {
                b->vel.z -= 0.3f * w->scale;
            } else {
                b->vel.z = 0.1f;
            }
            if (b->vel.z < 15.0f) {
                b->vel.z += b->accel;
            }
            if (b->vel.y < 40.0f) {
                b->vel.y += 0.2f * w->scale;
            }
            if (b->vel.x < 40.0f) {
                b->vel.x += 0.2f * w->scale;
            }
            if (b->timer != 0) {
                break;
            }
            VBird_SetAnim(b, 1);
            b->step++;
        case 2:
            b->pos.x += b->vel.x * Math_Sin((6.2831855f * b->ang) / 65536.0f);
            b->pos.z += b->vel.y * Math_Cos((6.2831855f * b->ang) / 65536.0f);
            b->pos.y += b->vel.z;
            if (b->vel.z < 15.0f) {
                b->vel.z += b->accel;
            }
            if (b->vel.y < 40.0f) {
                b->vel.y += 1.5f * w->scale;
            }
            if (b->vel.x < 40.0f) {
                b->vel.x += 1.5f * w->scale;
            }
            b->climb += b->vel.z;
            if (b->climb >= 4000.0f) {
                w->spot[i] = -1;
                VBird_PickSpot(w->spot, i);
                b->step = 0;
                b->unk37 = 0;
                b->called = 0;
                b->color = gBirdColors[LobbyWorld_GetSeason()];
                b->state = 1;
                VBird_SetAnim(b, 1);
                VBird_StartArrival(b, w->scale);
                if (Rand_Next(1) % 8 == 2) {
                    b->color = gBirdColors[Rand_Next(1) % 3];
                } else {
                    b->color = -1;
                }
            }
            break;
        }
        break;
    }
    VBird_Animate(b);
}

void VFlock_Run(VObj *o)
{
    VFlock *w;
    VBird *b;
    s32 i;

    w = (VFlock *)o->buf;
    i = 0;
    b = w->bird;
    do {
        VBird_Update(w, b, i);
        i += 1;
        b += 1;
    } while (i < 6);
    OT_AddByDepth(gDrawOt32, o->node, 0x20, 0);
}

void VFlock_End(void *o)
{
    *(u8 *)((u8 *)o + 4) = (u8) (*(u8 *)((u8 *)o + 4) + 1);
    DrawSlot_Free(*(s16 *)((u8 *)o + 0x3C));
}

/* unk_6BCA50's .data (0x7DD030-0x7DD2B0) */
u8 gVMuzzleScaleKeys0[64] = {
#include "lobby/007DD030.inc"
};
u8 gVMuzzleColorKeys0[32] = {
#include "lobby/007DD070.inc"
};
u8 gVMuzzleScaleKeys1[48] = {
#include "lobby/007DD090.inc"
};
u8 gVMuzzleColorKeys1[32] = {
#include "lobby/007DD0C0.inc"
};
u8 gVMuzzleScaleKeys2[96] = {
#include "lobby/007DD0E0.inc"
};
u8 gVMuzzleColorKeys2[48] = {
#include "lobby/007DD140.inc"
};
s32 gBirdColors[4] = {
#include "lobby/007DD170.inc"
};
u32 gVBirdAnims_3[4] = {
#include "lobby/007DD180.inc"
};
u32 gVBirdAnims_4[4] = {
#include "lobby/007DD190.inc"
};
u32 gVBirdAnims_1[4] = {
#include "lobby/007DD1A0.inc"
};
u32 gVBirdAnims_2[4] = {
#include "lobby/007DD1B0.inc"
};
VBirdAnim gVBirdAnims[6] = {
#include "lobby/007DD1C0.inc"
};
Vec3f gVBirdSpots[16] = {
#include "lobby/007DD1F0.inc"
};

/* unk_6BCA50's .rodata (0x825F20-0x825F28) */
#pragma rodata on
char D_00825F20[0x8] = {
#include "lobby/00825F20.inc"
};
#pragma rodata off
