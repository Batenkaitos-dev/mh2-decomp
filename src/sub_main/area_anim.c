/*
 * The area's animation timers and the matrices of animated area model
 * parts.
 * - Area_UpdateTimers runs the timers of gAreaState: state 0 (Area_StartTimers)
 *   clears the frame counter (frame) and moves to state 1; state 1
 *   (Area_Update) adds 1 to frame and 46 to spin every frame.
 * - The others build one matrix each for the part drawer Area_DrawProp,
 *   which reads a command byte per part (record +2, AREA_ANIM_CMD_* in
 *   area_anim.h) and a data pointer (+4); the flags byte (+3) is passed on
 *   (AREA_ANIM_REVERSE, AREA_ANIM_ROT_X, AREA_ANIM_ROT_Z; about Y without
 *   them). Object matrices go to Gs_SetState(RS_MTX_SLOT(0), mtx) (the first slot of
 *   the matrix palette), texture matrices to Gs_SetState(RS_UV_MTX, mtx).
 */
#include "common.h"
#include "area_anim.h"
#include "area.h"
#include "render_state.h"
#include "engine_math.h"

/* The value is a u32, as the other model code declares it: with a `void *`
 * parameter, the compiler loads the two arguments in the other order for a
 * local matrix. */

/* Steps gAreaState's timers: starts them, then runs them (see above). */
void Area_UpdateTimers(void)
{
    AreaState *areaState = &gAreaState;

    switch (areaState->timerState) {
    case 0:
        Area_StartTimers(areaState);
        break;
    case 1:
        Area_Update(areaState);
        break;
    }
}

/* AREA_ANIM_CMD_TRANSLATE: the part moved to pos. */
void AreaAnim_Translate(void *mtx, f32 *pos)
{
    Mtx_Translation(mtx, pos[0], pos[1], pos[2]);
    Gs_SetState(RS_MTX_SLOT(0), (u32)mtx);
}

/* AREA_ANIM_CMD_ROTATE: moved and turned by a fixed angle. */
void AreaAnim_TranslateRotate(void *mtx, AnimRot *anim, u8 flags)
{
    Mtx_Translation(mtx, anim->pos[0], anim->pos[1], anim->pos[2]);
    if (flags & AREA_ANIM_ROT_X) {
        Mtx_RotateX(mtx, anim->angle);
    } else if (flags & AREA_ANIM_ROT_Z) {
        Mtx_RotateZ(mtx, anim->angle);
    } else {
        Mtx_RotateY(mtx, anim->angle);
    }
    Gs_SetState(RS_MTX_SLOT(0), (u32)mtx);
}

/*
 * AREA_ANIM_CMD_SPIN: moved, and turning with the area's frame counter: a
 * turn every ANGLE_TURN >> shift frames, backwards with AREA_ANIM_REVERSE.
 */
void AreaAnim_TranslateSpin(void *mtx, AnimSpin *anim, u8 flags)
{
    AreaState *areaState = &gAreaState;
    u16 tick;
    f32 angle;

    Mtx_Translation(mtx, anim->pos[0], anim->pos[1], anim->pos[2]);
    if (flags & AREA_ANIM_REVERSE) {
        tick = -areaState->frame;
    } else {
        tick = areaState->frame;
    }
    angle = (6.2831855f * ((tick & (0xFFFF >> anim->shift)) << anim->shift)) / 65536.0f;
    if (flags & AREA_ANIM_ROT_X) {
        Mtx_RotateX(mtx, angle);
    } else if (flags & AREA_ANIM_ROT_Z) {
        Mtx_RotateZ(mtx, angle);
    } else {
        Mtx_RotateY(mtx, angle);
    }
    Gs_SetState(RS_MTX_SLOT(0), (u32)mtx);
}

/* AREA_ANIM_CMD_SCALE_ROTATE: scaled, moved and turned by a fixed angle. */
void AreaAnim_ScaleTranslateRotate(void *mtx, AnimRotScale *anim, u8 flags)
{
    Mtx_Scale(mtx, anim->scale[0], anim->scale[1], anim->scale[2]);
    Mtx_SetTrans(mtx, anim->rot->pos[0], anim->rot->pos[1], anim->rot->pos[2]);
    if (flags & AREA_ANIM_ROT_X) {
        Mtx_RotateX(mtx, anim->rot->angle);
    } else if (flags & AREA_ANIM_ROT_Z) {
        Mtx_RotateZ(mtx, anim->rot->angle);
    } else {
        Mtx_RotateY(mtx, anim->rot->angle);
    }
    Gs_SetState(RS_MTX_SLOT(0), (u32)mtx);
}

/* AREA_ANIM_CMD_TRANSLATE_XZ: moved to (x, 0, z). */
void AreaAnim_TranslateXZ(void *mtx, f32 *posXZ)
{
    Mtx_Translation(mtx, posXZ[0], 0.0f, posXZ[1]);
    Gs_SetState(RS_MTX_SLOT(0), (u32)mtx);
}

/*
 * AREA_ANIM_CMD_UV_SCROLL: the texture scrolled in u and v, each through a
 * full repeat every `period` frames (a power of two; 0 still), backwards
 * with AREA_ANIM_REVERSE.
 */
void AreaAnim_UvScroll(UvScroll *anim, u8 flags)
{
    f32 texMtx[4][4];
    AreaState *areaState = &gAreaState;
    f32 uOffset;
    f32 vOffset;

    if (anim->period[0] == 0) {
        uOffset = 0.0f;
    } else {
        uOffset = (f32)(areaState->frame & (anim->period[0] - 1)) / anim->period[0];
        if (flags & AREA_ANIM_REVERSE) {
            uOffset = 1.0f - uOffset;
        }
    }
    if (anim->period[1] == 0) {
        vOffset = 0.0f;
    } else {
        vOffset = (f32)(areaState->frame & (anim->period[1] - 1)) / anim->period[1];
        if (flags & AREA_ANIM_REVERSE) {
            vOffset = 1.0f - vOffset;
        }
    }
    Mtx_Translation((f32 *) texMtx, uOffset, vOffset, 0.0f);
    Gs_SetState(RS_UV_MTX, (u32)texMtx);
}

/* AREA_ANIM_CMD_UV_WAVE: the texture swayed in u and v: base + amp *
 * sin(speed * the frame), the frame taken modulo `period`. */
void AreaAnim_UvWave(UvWave *waves)
{
    f32 texMtx[4][4];
    AreaState *areaState = &gAreaState;
    f32 uOffset;
    f32 vOffset;

    if (waves[0].period == 0) {
        uOffset = 0.0f;
    } else {
        uOffset = waves[0].base + waves[0].amp * Math_Sin((6.2831855f * (waves[0].speed * ((u16)areaState->frame & (waves[0].period - 1)))) / 65536.0f);
    }
    if (waves[1].period == 0) {
        vOffset = 0.0f;
    } else {
        vOffset = waves[1].base + waves[1].amp * Math_Sin((6.2831855f * (waves[1].speed * ((u16)areaState->frame & (waves[1].period - 1)))) / 65536.0f);
    }
    Mtx_Translation((f32 *) texMtx, uOffset, vOffset, 0.0f);
    Gs_SetState(RS_UV_MTX, (u32)texMtx);
}

/* AREA_ANIM_CMD_UV_SCROLL_SPIN: AreaAnim_UvScroll and AreaAnim_TranslateSpin. */
void AreaAnim_UvScrollSpin(void *mtx, AnimScrollSpin *anim, u8 flags)
{
    AreaAnim_UvScroll(anim->uv, flags);
    AreaAnim_TranslateSpin(mtx, anim->spin, flags);
}

/* AREA_ANIM_CMD_UV_STEP: the texture moved in steps: step * ((frame >>
 * shift) & mask) in u and in v (frames of a texture strip). */
void AreaAnim_UvStep(UvStep *steps)
{
    f32 texMtx[4][4];

    Mtx_Translation((f32 *) texMtx, steps[0].step * (f32)(steps[0].mask & (gAreaState.frame >> steps[0].shift)),
                  steps[1].step * (f32)(steps[1].mask & (gAreaState.frame >> steps[1].shift)), 0.0f);
    Gs_SetState(RS_UV_MTX, (u32)texMtx);
}

/* AREA_ANIM_CMD_UV_SCROLL_MOVE: AreaAnim_UvScroll and AreaAnim_Translate. */
void AreaAnim_UvScrollTranslate(void *mtx, AnimScrollPos *anim, u8 flags)
{
    AreaAnim_UvScroll(anim->uv, flags);
    AreaAnim_Translate(mtx, anim->pos);
}

/* AREA_ANIM_CMD_UV_SCROLL_ROTATE: AreaAnim_UvScroll and AreaAnim_TranslateRotate. */
void AreaAnim_UvScrollRotate(void *mtx, AnimScrollRot *anim, u8 flags)
{
    AreaAnim_UvScroll(anim->uv, flags);
    AreaAnim_TranslateRotate(mtx, anim->rot, flags);
}

/*
 * AREA_ANIM_CMD_SPIN_SCROLL: turning about Y with gAreaState.spin (scaled by
 * shift), the texture scrolled in u with the same angle (a full scroll every
 * 30 degrees of turn).
 */
void AreaAnim_SpinScroll(void *mtx, AnimSpin2 *anim)
{
    f32 texMtx[4][4];
    u16 tick;
    f32 uOffset;

    tick = gAreaState.spin << anim->shift;
    uOffset = 0.033333335f * ((360.0f * tick) / 65536.0f);
    uOffset -= Math_Floor(uOffset);
    Mtx_Translation((f32 *) texMtx, uOffset, 0.0f, 0.0f);
    Gs_SetState(RS_UV_MTX, (u32)texMtx);
    AreaAnim_Translate(mtx, anim->pos);
    Mtx_RotateY(mtx, (6.2831855f * tick) / 65536.0f);
    Gs_SetState(RS_MTX_SLOT(0), (u32)mtx);
}
