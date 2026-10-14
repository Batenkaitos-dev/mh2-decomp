/*
 * sub_main 0x37AC80-0x37B570: the area's animation timers and the matrices
 * of animated area model parts.
 * - Area_UpdateTimers runs the timers of gAreaState: state 0 (Area_StartTimers)
 *   clears the frame counter (frame) and moves to state 1; state 1
 *   (Area_Update) adds 1 to frame and 0x2E to spin every frame.
 * - The others build one matrix each for the part drawer Area_DrawProp,
 *   which reads a command byte per part (record +2) and a data pointer
 *   (+4); the flags byte (+3) is passed on: 0x4 reverses the motion, 0x40
 *   rotates about X, 0x80 about Z (default Y). Object matrices go to
 *   Gs_SetState(0x1A, m) (the first slot of the matrix palette), texture
 *   matrices to Gs_SetState(0x19, m).
 *   Command -> function: 1 ADD0, 2 B000, 3 B150, 4 ACE0, 5 AD20, 6 AEF0,
 *   7 AFC0, 8 B370, 9 B290, 0xA B3C0, 0xB B420, 0xC B2F0.
 */
#include "common.h"
#include "area_anim.h"
#include "area.h"

void Area_StartTimers(AreaState *s);
void Area_Update(AreaState *s);
void Mtx_Scale(f32 *m, f32 x, f32 y, f32 z);   /* m = scale */
void Mtx_Translation(f32 *m, f32 x, f32 y, f32 z);   /* m = translation */
void Mtx_SetTrans(f32 *m, f32 x, f32 y, f32 z);   /* sets m's translation */
void Mtx_RotateX(f32 *m, f32 a);                 /* rotates m about X */
void Mtx_RotateY(f32 *m, f32 a);                 /* ... about Y */
void Mtx_RotateZ(f32 *m, f32 a);                 /* ... about Z */
f32 Math_Sin(f32 x);                           /* sin */
f32 Math_Floor(f32 x);                           /* floorf */
/* The value is a u32 (as in unk_36BE70.c): with a `void *` parameter, MWCC
 * loads the two arguments in the other order for a local matrix. */
s32 Gs_SetState(u8 reg, u32 v);

void Area_UpdateTimers(void)
{
    AreaState *s = &gAreaState;

    switch (s->timerState) {
    case 0:
        Area_StartTimers(s);
        break;
    case 1:
        Area_Update(s);
        break;
    }
}

void AreaAnim_Translate(void *m, f32 *pos)
{
    Mtx_Translation(m, pos[0], pos[1], pos[2]);
    Gs_SetState(0x1A, (u32)m);
}

void AreaAnim_TranslateRotate(void *m, AnimRot *p, u8 flags)
{
    Mtx_Translation(m, p->pos[0], p->pos[1], p->pos[2]);
    if (flags & 0x40) {
        Mtx_RotateX(m, p->angle);
    } else if (flags & 0x80) {
        Mtx_RotateZ(m, p->angle);
    } else {
        Mtx_RotateY(m, p->angle);
    }
    Gs_SetState(0x1A, (u32)m);
}

void AreaAnim_TranslateSpin(void *m, AnimSpin *p, u8 flags)
{
    AreaState *s = &gAreaState;
    u16 t;
    f32 a;

    Mtx_Translation(m, p->pos[0], p->pos[1], p->pos[2]);
    if (flags & 4) {
        t = -s->frame;
    } else {
        t = s->frame;
    }
    a = (6.2831855f * ((t & (0xFFFF >> p->shift)) << p->shift)) / 65536.0f;
    if (flags & 0x40) {
        Mtx_RotateX(m, a);
    } else if (flags & 0x80) {
        Mtx_RotateZ(m, a);
    } else {
        Mtx_RotateY(m, a);
    }
    Gs_SetState(0x1A, (u32)m);
}

void AreaAnim_ScaleTranslateRotate(void *m, AnimRotScale *p, u8 flags)
{
    Mtx_Scale(m, p->scale[0], p->scale[1], p->scale[2]);
    Mtx_SetTrans(m, p->rot->pos[0], p->rot->pos[1], p->rot->pos[2]);
    if (flags & 0x40) {
        Mtx_RotateX(m, p->rot->angle);
    } else if (flags & 0x80) {
        Mtx_RotateZ(m, p->rot->angle);
    } else {
        Mtx_RotateY(m, p->rot->angle);
    }
    Gs_SetState(0x1A, (u32)m);
}

void AreaAnim_TranslateXZ(void *m, f32 *p)
{
    Mtx_Translation(m, p[0], 0.0f, p[1]);
    Gs_SetState(0x1A, (u32)m);
}

void AreaAnim_UvScroll(UvScroll *p, u8 flags)
{
    f32 m[4][4];
    AreaState *s = &gAreaState;
    f32 u;
    f32 v;

    if (p->period[0] == 0) {
        u = 0.0f;
    } else {
        u = (f32)(s->frame & (p->period[0] - 1)) / p->period[0];
        if (flags & 4) {
            u = 1.0f - u;
        }
    }
    if (p->period[1] == 0) {
        v = 0.0f;
    } else {
        v = (f32)(s->frame & (p->period[1] - 1)) / p->period[1];
        if (flags & 4) {
            v = 1.0f - v;
        }
    }
    Mtx_Translation((f32 *) m, u, v, 0.0f);
    Gs_SetState(0x19, (u32)m);
}

void AreaAnim_UvWave(UvWave *p)
{
    f32 m[4][4];
    AreaState *s = &gAreaState;
    f32 u;
    f32 v;

    if (p[0].period == 0) {
        u = 0.0f;
    } else {
        u = p[0].base + p[0].amp * Math_Sin((6.2831855f * (p[0].speed * ((u16)s->frame & (p[0].period - 1)))) / 65536.0f);
    }
    if (p[1].period == 0) {
        v = 0.0f;
    } else {
        v = p[1].base + p[1].amp * Math_Sin((6.2831855f * (p[1].speed * ((u16)s->frame & (p[1].period - 1)))) / 65536.0f);
    }
    Mtx_Translation((f32 *) m, u, v, 0.0f);
    Gs_SetState(0x19, (u32)m);
}

void AreaAnim_UvScrollSpin(void *m, AnimScrollSpin *p, u8 flags)
{
    AreaAnim_UvScroll(p->uv, flags);
    AreaAnim_TranslateSpin(m, p->spin, flags);
}

void AreaAnim_UvStep(UvStep *p)
{
    f32 m[4][4];

    Mtx_Translation((f32 *) m, p[0].step * (f32)(p[0].mask & (gAreaState.frame >> p[0].shift)),
                  p[1].step * (f32)(p[1].mask & (gAreaState.frame >> p[1].shift)), 0.0f);
    Gs_SetState(0x19, (u32)m);
}

void AreaAnim_UvScrollTranslate(void *m, AnimScrollPos *p, u8 flags)
{
    AreaAnim_UvScroll(p->uv, flags);
    AreaAnim_Translate(m, p->pos);
}

void AreaAnim_UvScrollRotate(void *m, AnimScrollRot *p, u8 flags)
{
    AreaAnim_UvScroll(p->uv, flags);
    AreaAnim_TranslateRotate(m, p->rot, flags);
}

void AreaAnim_SpinScroll(void *m, AnimSpin2 *p)
{
    f32 mt[4][4];
    u16 t;
    f32 u;

    t = gAreaState.spin << p->shift;
    u = 0.033333335f * ((360.0f * t) / 65536.0f);
    u -= Math_Floor(u);
    Mtx_Translation((f32 *) mt, u, 0.0f, 0.0f);
    Gs_SetState(0x19, (u32)mt);
    AreaAnim_Translate(m, p->pos);
    Mtx_RotateY(m, (6.2831855f * t) / 65536.0f);
    Gs_SetState(0x1A, (u32)m);
}
