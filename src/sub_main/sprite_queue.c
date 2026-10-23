/*
 * The 2D drawing helpers: the sprite queue (Sprite_Queue and Sprite_RotateVerts, then
 * Prim_DrawAll and Prim_Draw, which draw the frame's queued primitives; sprite_queue.h), the
 * rotated textured rectangles (TexRect_*), and filled and outlined rectangles (Gfx_DrawLine,
 * Gfx_DrawRectFrame).
 */
#include "common.h"
#include "sprite.h"
#include "chat_log.h"
#include "quest.h"
#include "player.h"
#include "em.h"
#include "scene.h"
#include "syssettings.h"
#include "objhead.h"
#include "objattach.h"
#include "obj.h"
#include "window.h"
#include "render_state.h"
#include "area.h"
#include "hud.h"
#include "adx.h"
#include "sound.h"
#include "bgm.h"
#include "sprite_queue.h"
#include "lobby_state.h"
#include "area_draw.h"
#include "kbd.h"
#include "save.h"
#include "text.h"
#include "village_cond.h"
#include "camera.h"
#include "engine_math.h"
#include "vec_math.h"

/* The height of a screen pixel to its width (14 / 15), which the rotated
 * rectangles correct for. */
#define SCREEN_PIXEL_ASPECT 0.93333334f
/* A textured triangle (prim.h's PrimTexTri) with its texture corners as UV
 * pairs: the rotated rectangles copy a corner with one word move. */
typedef struct TexTri {
    s16 x0;                     /* 0x00 */
    s16 y0;                     /* 0x02 */
    s16 x1;                     /* 0x04 */
    s16 y1;                     /* 0x06 */
    s16 x2;                     /* 0x08 */
    s16 y2;                     /* 0x0A */
    s32 color;                  /* 0x0C */
    UV uv0;                     /* 0x10 */
    UV uv1;                     /* 0x14 */
    UV uv2;                     /* 0x18 */
} TexTri;
extern s32 gTexHandles[];
extern u8 gViewMatrix[];
struct MtxSlot;
void *memcpy(void *d, void *s, u32 n);

/*
 * Turns the n points v about the point c by the sprite's rotation (rot, in
 * degrees); for SPR_BILLBOARD, also by the inverse of the view matrix, so
 * that the sprite faces the camera.
 */
void Sprite_RotateVerts(f32 (*verts)[3], f32 *center, s32 count, SprDef *spr)
{
    f32 rotated[4];
    f32 offset[4];
    f32 rotMtx[4][4];
    f32 baseMtx[4][4];
    s32 i;

    if (spr->flags & SPR_BILLBOARD) {
        Mtx_Inverse((f32 *) baseMtx, (f32 *) gViewMatrix);
    } else {
        Mtx_Identity((f32 *) baseMtx);
    }
    Mtx_Identity((f32 *) rotMtx);
    Mtx_SetRotZYX((f32 *) rotMtx, 6.2831855f * spr->rot[0] / 360.0f, 6.2831855f * spr->rot[1] / 360.0f, 6.2831855f * spr->rot[2] / 360.0f);
    Mtx_Mul((f32 *) rotMtx, (f32 *) rotMtx, (f32 *) baseMtx);
    for (i = 0; i < count; i++, verts++) {
        offset[0] = (*verts)[0] - center[0];
        offset[1] = (*verts)[1] - center[1];
        offset[2] = (*verts)[2] - center[2];
        Mtx_RotateVec((f32 *) rotMtx, (f32 *) offset, (f32 *) rotated);
        (*verts)[0] = rotated[0] + center[0];
        (*verts)[1] = rotated[1] + center[1];
        (*verts)[2] = rotated[2] + center[2];
    }
}

/*
 * Queues the sprite s as a primitive in gPrims, unless the queue is full.
 * Its rectangle runs from the anchor pos by size * scale, by the alignment
 * and the flips (in the world kinds y grows upward); its colour is rgb with
 * alpha. A PRIM_SPRITE_3D with PRIM_ROTATED (always for SPR_BILLBOARD,
 * whose rotation starts at 0) is turned about pos (Sprite_RotateVerts) and
 * queued as two triangles, the second only when there is room for it.
 */
void Sprite_Queue(SprDef *spr)
{
    PrimPoint corner0;
    PrimPoint corner1;
    PrimPoint center;
    PrimPoint corners[4];
    Prim *prim = gPrims;
    PrimSpriteF *fillQuad;
    PrimSpriteF *spriteQuad;
    PrimTri3DView *tri;
    u32 color;
    s32 rgb;
    f32 sign;
    f32 len;
    f32 uLeft;
    f32 uRight;
    f32 vTop;
    f32 vBottom;

    if (gPrimCount != PRIM_QUEUE_MAX) {
        prim += gPrimCount;
        prim->tex = spr->tex;
        prim->z = spr->pos[2];
        prim->flags = spr->primFlags & PRIM_FLAGS_MASK;
        if (spr->flags & SPR_FILTER) {
            prim->flags |= PRIM_FILTER;
        }
        if (spr->flags & SPR_ADDITIVE) {
            prim->additive = 1;
        } else {
            prim->additive = 0;
        }
        if ((prim->flags & PRIM_IS_3D) == 0) {
            sign = 1.0f;
        } else {
            sign = -1.0f;
        }
        switch (spr->flags & SPR_ALIGN_X_MASK) {
        case 0:
        case 1:
            if (spr->flags & SPR_FLIP_X) {
                corner0.x = spr->pos[0] - spr->size[0] * spr->scale[0];
                corner1.x = spr->pos[0];
            } else {
                corner0.x = spr->pos[0];
                corner1.x = corner0.x + spr->size[0] * spr->scale[0];
            }
            break;
        case SPR_ALIGN_CENTER:
            len = spr->size[0] * spr->scale[0];
            corner0.x = spr->pos[0] - len / 2.0f;
            corner1.x = corner0.x + len;
            break;
        case SPR_ALIGN_END:
            if (spr->flags & SPR_FLIP_X) {
                corner0.x = spr->pos[0];
                corner1.x = corner0.x + spr->size[0] * spr->scale[0];
            } else {
                corner0.x = spr->pos[0] - spr->size[0] * spr->scale[0];
                corner1.x = spr->pos[0];
            }
            break;
        }
        switch ((spr->flags & SPR_ALIGN_Y_MASK) >> SPR_ALIGN_Y_SHIFT) {
        case 0:
        case 1:
            if (spr->flags & SPR_FLIP_Y) {
                corner0.y = spr->pos[1] + sign * (-1.0f * (spr->size[1] * spr->scale[1]));
                corner1.y = spr->pos[1];
            } else {
                corner0.y = spr->pos[1];
                corner1.y = corner0.y + sign * (spr->size[1] * spr->scale[1]);
            }
            break;
        case SPR_ALIGN_CENTER:
            len = spr->size[1] * spr->scale[1];
            corner0.y = spr->pos[1] + sign * (-1.0f * (len / 2.0f));
            corner1.y = corner0.y + sign * len;
            break;
        case SPR_ALIGN_END:
            if (spr->flags & SPR_FLIP_Y) {
                corner0.y = spr->pos[1];
                corner1.y = corner0.y + sign * (spr->size[1] * spr->scale[1]);
            } else {
                corner0.y = spr->pos[1] + sign * (-1.0f * (spr->size[1] * spr->scale[1]));
                corner1.y = spr->pos[1];
            }
            break;
        }
        corner0.z = corner1.z = spr->pos[2];
        rgb = spr->rgb;
        color = rgb | (u32)(255.0f * spr->alpha) << 24;
        switch (prim->flags & PRIM_KIND_MASK) {
        case PRIM_FILL_2D:
            fillQuad = &prim->data.sprite;
            fillQuad->v[0].x = Math_Floor(corner0.x);
            fillQuad->v[0].y = Math_Floor(corner0.y);
            fillQuad->v[0].z = corner0.z;
            fillQuad->v[0].q = 1.0f;
            fillQuad->v[1].x = Math_Floor(corner1.x);
            fillQuad->v[1].y = Math_Floor(corner1.y);
            fillQuad->v[1].z = corner1.z;
            fillQuad->v[1].q = 1.0f;
            fillQuad->color = color;
            break;
        case PRIM_FILL_3D:
            prim->data.rect.p0 = corner0;
            prim->data.rect.p1 = corner1;
            prim->data.rect.color = color;
            break;
        case PRIM_SPRITE_2D:
            spriteQuad = &prim->data.sprite;
            spriteQuad->v[0].x = Math_Floor(corner0.x);
            spriteQuad->v[0].y = Math_Floor(corner0.y);
            spriteQuad->v[0].z = corner0.z;
            spriteQuad->v[0].q = 1.0f;
            spriteQuad->v[1].x = Math_Floor(corner1.x);
            spriteQuad->v[1].y = Math_Floor(corner1.y);
            spriteQuad->v[1].z = corner1.z;
            spriteQuad->v[1].q = 1.0f;
            spriteQuad->color = color;
            if (spr->flags & SPR_FLIP_X) {
                uLeft = spr->uv[2];
                uRight = spr->uv[0];
            } else {
                uLeft = spr->uv[0];
                uRight = spr->uv[2];
            }
            if (spr->flags & SPR_FLIP_Y) {
                vTop = spr->uv[3];
                vBottom = spr->uv[1];
            } else {
                vTop = spr->uv[1];
                vBottom = spr->uv[3];
            }
            spriteQuad->st[0][0] = uLeft;
            spriteQuad->st[0][1] = vTop;
            spriteQuad->st[1][0] = uRight;
            spriteQuad->st[1][1] = vBottom;
            break;
        case PRIM_SPRITE_3D:
            if (spr->flags & SPR_FLIP_X) {
                uLeft = spr->uv[2];
                uRight = spr->uv[0];
            } else {
                uLeft = spr->uv[0];
                uRight = spr->uv[2];
            }
            if (spr->flags & SPR_FLIP_Y) {
                vTop = spr->uv[3];
                vBottom = spr->uv[1];
            } else {
                vTop = spr->uv[1];
                vBottom = spr->uv[3];
            }
            if (spr->flags & SPR_BILLBOARD) {
                if (!(spr->primFlags & PRIM_ROTATED)) {
                    spr->primFlags |= PRIM_ROTATED;
                    spr->rot[2] = 0.0f;
                    spr->rot[1] = 0.0f;
                    spr->rot[0] = 0.0f;
                }
            }
            if (spr->primFlags & PRIM_ROTATED) {
                tri = &prim->data.tri;
                center.x = spr->pos[0];
                center.y = spr->pos[1];
                center.z = spr->pos[2];
                corners[0] = corner0;
                corners[1] = corner1;
                corners[1].y = corner0.y;
                corners[2] = corner0;
                corners[2].y = corner1.y;
                corners[3] = corner1;
                Sprite_RotateVerts((f32 (*)[3])corners, &center.x, 4, spr);
                tri->v[0] = corners[0];
                tri->v[1] = corners[1];
                tri->v[2] = corners[2];
                tri->color[0] = tri->color[1] = tri->color[2] = color;
                tri->st[0][0] = uLeft;
                tri->st[0][1] = vTop;
                tri->st[1][0] = uRight;
                tri->st[1][1] = vTop;
                tri->st[2][0] = uLeft;
                tri->st[2][1] = vBottom;
                if (gPrimCount != PRIM_QUEUE_MAX - 1) {
                    memcpy(prim + 1, prim, sizeof(Prim));
                    gPrimCount++;
                    prim[1].data.tri.v[0] = corners[1];
                    prim[1].data.tri.v[1] = corners[2];
                    prim[1].data.tri.v[2] = corners[3];
                    prim[1].data.tri.st[0][0] = uRight;
                    prim[1].data.tri.st[0][1] = vTop;
                    prim[1].data.tri.st[1][0] = uLeft;
                    prim[1].data.tri.st[1][1] = vBottom;
                    prim[1].data.tri.st[2][0] = uRight;
                    prim[1].data.tri.st[2][1] = vBottom;
                }
            } else {
                prim->data.rect.p0 = corner0;
                prim->data.rect.p1 = corner1;
                prim->data.rect.color = color;
                prim->data.rect.st[0] = uLeft;
                prim->data.rect.st[1] = vTop;
                prim->data.rect.st[2] = uRight;
                prim->data.rect.st[3] = vBottom;
            }
            break;
        }
        gPrimCount++;
    }
}

/* Draws the frame's queued primitives in order, with the camera's view
 * matrix and an identity model matrix, then restores the render states. */
void Prim_DrawAll(void)
{
    s32 i;
    Prim *prim = gPrims;
    f32 identity[4][4];

    Camera_SetPreset(0);
    MtxSlot_Store(33, (struct MtxSlot *) gViewMatrix);
    Mtx_Identity((f32 *) identity);
    Gs_SetState(RS_MTX_SLOT(0), (u32)identity);
    Gs_SetState(RS_ALPHA_REF, 0);
    for (i = 0; i < gPrimCount; i++, prim++) {
        Prim_Draw(prim);
    }
    Gfx_ResetRenderStates(1);
}

/* Draws one queued primitive: its filter and blend, then its kind's draw
 * function. A textured one is skipped while its texture is not loaded. */
void Prim_Draw(Prim *prim)
{
    Gfx_SetTexFilter((prim->flags & PRIM_FILTER) / PRIM_FILTER);
    if (prim->additive != 0) {
        Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_ONE);
    } else {
        Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
    }
    switch (prim->flags & PRIM_KIND_MASK) {
    case PRIM_FILL_2D:
        Prim_FillRectF(&prim->data.fill);
        break;
    case PRIM_FILL_3D:
        Prim_DrawRect3D(&prim->data.fill3d);
        break;
    case PRIM_SPRITE_2D:
        if (gTexHandles[prim->tex] != 0) {
            Tex_Prepare(1, prim->tex);
            Tex_Bind(prim->tex);
            Prim_DrawSpriteF(&prim->data.sprite);
        }
        break;
    case PRIM_SPRITE_3D:
        if (gTexHandles[prim->tex] != 0) {
            Tex_Prepare(1, prim->tex);
            Tex_Bind(prim->tex);
            if (prim->flags & PRIM_ROTATED) {
                Prim_DrawTexTri3D(&prim->data.tri3d);
            } else {
                Prim_DrawSprite3D(&prim->data.sprite3d);
            }
        }
        break;
    }
}

/* Draws the textured rectangle r turned by angle (ANGLE_TURN to a full turn)
 * about its centre. */
void TexRect_DrawRotated(TexRect *rect, s32 angle)
{
    TexRect_DrawRotatedAt(rect, rect->x + (s16)(0.5f * rect->w), rect->y + (s16)(0.5f * rect->h), angle);
}

/*
 * Draws the textured rectangle r turned by angle (ANGLE_TURN to a full turn)
 * about the point (cx, cy), as two textured triangles. The turn is made
 * with the heights scaled to square pixels (by 1 / SCREEN_PIXEL_ASPECT, and
 * back after), so the shape keeps its proportions; r's height is restored
 * on return.
 */
void TexRect_DrawRotatedAt(TexRect *rect, s16 centerX, s16 centerY, u32 angle)
{
    f32 offset[3];
    TexTri tri;
    s16 height = rect->h;
    f32 rad;

    rect->h = height / SCREEN_PIXEL_ASPECT;
    offset[2] = 0.0f;
    offset[0] = rect->x - centerX;
    offset[1] = rect->y - centerY;
    rad = 6.2831855f * angle / 65536.0f;
    Vec_RotateZ(offset, rad);
    tri.x0 = centerX + offset[0];
    tri.y0 = centerY + SCREEN_PIXEL_ASPECT * offset[1];
    offset[0] = rect->x + rect->w - centerX;
    offset[1] = rect->y + rect->h - centerY;
    Vec_RotateZ(offset, rad);
    tri.x1 = centerX + offset[0];
    tri.y1 = centerY + SCREEN_PIXEL_ASPECT * offset[1];
    offset[0] = rect->x + rect->w - centerX;
    offset[1] = rect->y - centerY;
    Vec_RotateZ(offset, rad);
    tri.x2 = centerX + offset[0];
    tri.y2 = centerY + SCREEN_PIXEL_ASPECT * offset[1];
    tri.color = rect->color;
    tri.uv0 = rect->uv0;
    tri.uv1 = rect->uv1;
    tri.uv2.u = rect->uv1.u;
    tri.uv2.v = rect->uv0.v;
    Prim_DrawTexTri((struct PrimTexTri *)&tri);
    offset[0] = rect->x - centerX;
    offset[1] = rect->y + rect->h - centerY;
    Vec_RotateZ(offset, rad);
    tri.x2 = centerX + offset[0];
    tri.y2 = centerY + SCREEN_PIXEL_ASPECT * offset[1];
    tri.color = rect->color;
    tri.uv0 = rect->uv0;
    tri.uv1 = rect->uv1;
    tri.uv2.u = rect->uv0.u;
    tri.uv2.v = rect->uv1.v;
    Prim_DrawTexTri((struct PrimTexTri *)&tri);
    rect->h = height;
}

/*
 * Draws the textured rectangle r turned twice: by angle1 about (cx, cy),
 * then by angle2 about (cx2, cy2), as TexRect_DrawRotatedAt does (a hand
 * turning on a dial that turns too, say).
 */
void TexRect_DrawRotated2(TexRect *rect, s16 centerX, s16 centerY, s16 pivotX, s16 pivotY, u32 angle, u32 pivotAngle)
{
    f32 offset[3];
    TexTri tri;
    s16 height = rect->h;
    f32 rad;
    f32 pivotRad;

    rect->h = height / SCREEN_PIXEL_ASPECT;
    offset[2] = 0.0f;
    offset[0] = rect->x - centerX;
    offset[1] = rect->y - centerY;
    rad = 6.2831855f * angle / 65536.0f;
    Vec_RotateZ(offset, rad);
    tri.x0 = centerX + offset[0];
    tri.y0 = centerY + SCREEN_PIXEL_ASPECT * offset[1];
    offset[0] = tri.x0 - pivotX;
    offset[1] = tri.y0 - pivotY;
    pivotRad = 6.2831855f * pivotAngle / 65536.0f;
    Vec_RotateZ(offset, pivotRad);
    tri.x0 = pivotX + offset[0];
    tri.y0 = pivotY + SCREEN_PIXEL_ASPECT * offset[1];
    offset[0] = rect->x + rect->w - centerX;
    offset[1] = rect->y + rect->h - centerY;
    Vec_RotateZ(offset, rad);
    tri.x1 = centerX + offset[0];
    tri.y1 = centerY + SCREEN_PIXEL_ASPECT * offset[1];
    offset[0] = tri.x1 - pivotX;
    offset[1] = tri.y1 - pivotY;
    Vec_RotateZ(offset, pivotRad);
    tri.x1 = pivotX + offset[0];
    tri.y1 = pivotY + SCREEN_PIXEL_ASPECT * offset[1];
    offset[0] = rect->x + rect->w - centerX;
    offset[1] = rect->y - centerY;
    Vec_RotateZ(offset, rad);
    tri.x2 = centerX + offset[0];
    tri.y2 = centerY + SCREEN_PIXEL_ASPECT * offset[1];
    offset[0] = tri.x2 - pivotX;
    offset[1] = tri.y2 - pivotY;
    Vec_RotateZ(offset, pivotRad);
    tri.x2 = pivotX + offset[0];
    tri.y2 = pivotY + SCREEN_PIXEL_ASPECT * offset[1];
    tri.color = rect->color;
    tri.uv0 = rect->uv0;
    tri.uv1 = rect->uv1;
    tri.uv2.u = rect->uv1.u;
    tri.uv2.v = rect->uv0.v;
    Prim_DrawTexTri((struct PrimTexTri *)&tri);
    offset[0] = rect->x - centerX;
    offset[1] = rect->y + rect->h - centerY;
    Vec_RotateZ(offset, rad);
    tri.x2 = centerX + offset[0];
    tri.y2 = centerY + SCREEN_PIXEL_ASPECT * offset[1];
    offset[0] = tri.x2 - pivotX;
    offset[1] = tri.y2 - pivotY;
    Vec_RotateZ(offset, pivotRad);
    tri.x2 = pivotX + offset[0];
    tri.y2 = pivotY + SCREEN_PIXEL_ASPECT * offset[1];
    tri.color = rect->color;
    tri.uv0 = rect->uv0;
    tri.uv1 = rect->uv1;
    tri.uv2.u = rect->uv0.u;
    tri.uv2.v = rect->uv1.v;
    Prim_DrawTexTri((struct PrimTexTri *)&tri);
    rect->h = height;
}

/*
 * Draws the textured rectangle r: mode 1 as it is, 0 mirrored left to
 * right (as sprites); 2 and 3 as two triangles with the texture turned a
 * quarter turn one way or the other.
 */
void TexRect_Draw(TexRect *rect, s8 mode)
{
    TexTri tri;
    TexRect copy;
    s16 right;
    s16 bottom;
    s16 uLeft;
    s16 uRight;
    s16 left;
    s16 top;
    s16 vTop;
    s16 vBottom;

    if (mode < 2) {
        copy = *rect;
        if (mode == 0) {
            copy.uv0.u = rect->uv1.u;
            copy.uv1.u = rect->uv0.u;
        }
        Sprite_Draw((struct Sprite *)&copy);
        return;
    }
    left = rect->x;
    top = rect->y;
    uLeft = rect->uv0.u;
    uRight = rect->uv1.u;
    vTop = rect->uv0.v;
    vBottom = rect->uv1.v;
    right = left + rect->w;
    bottom = top + rect->h;
    tri.color = rect->color;
    switch (mode) {
    case 2:
        tri.x0 = tri.x1 = left;
        tri.x2 = right;
        tri.y0 = top;
        tri.y1 = tri.y2 = bottom;
        tri.uv0.u = uRight;
        tri.uv1.u = tri.uv2.u = uLeft;
        tri.uv0.v = tri.uv1.v = vTop;
        tri.uv2.v = vBottom;
        Prim_DrawTexTri((struct PrimTexTri *)&tri);
        tri.x0 = tri.x1 = right;
        tri.x2 = left;
        tri.y0 = bottom;
        tri.y1 = tri.y2 = top;
        tri.uv0.u = uLeft;
        tri.uv1.u = tri.uv2.u = uRight;
        tri.uv0.v = tri.uv1.v = vBottom;
        tri.uv2.v = vTop;
        Prim_DrawTexTri((struct PrimTexTri *)&tri);
        break;
    case 3:
        tri.x0 = tri.x1 = left;
        tri.x2 = right;
        tri.y0 = top;
        tri.y1 = tri.y2 = bottom;
        tri.uv0.u = uLeft;
        tri.uv1.u = tri.uv2.u = uRight;
        tri.uv0.v = tri.uv1.v = vBottom;
        tri.uv2.v = vTop;
        Prim_DrawTexTri((struct PrimTexTri *)&tri);
        tri.x0 = tri.x1 = right;
        tri.x2 = left;
        tri.y0 = bottom;
        tri.y1 = tri.y2 = top;
        tri.uv0.u = uRight;
        tri.uv1.u = tri.uv2.u = uLeft;
        tri.uv0.v = tri.uv1.v = vTop;
        tri.uv2.v = vBottom;
        Prim_DrawTexTri((struct PrimTexTri *)&tri);
        break;
    }
}

/* Fills the rectangle of w by h at (x, y) with color (a line when one of
 * them is 0). */
void Gfx_DrawLine(s16 x, s16 y, s16 width, s16 height, s32 color)
{
    PrimLine line;

    line.x0 = (f32)x;
    line.y0 = y;
    line.x1 = (f32)(x + width);
    line.y1 = y + height;
    line.color = color;
    Prim_FillRect(&line);
}

/* Draws the outline of the rectangle of w by h at (x, y) in color, as four
 * lines. */
void Gfx_DrawRectFrame(s16 x, s16 y, s16 width, s16 height, s32 color)
{
    PrimLine line;

    x = (f32)x;
    width = (f32)width;

    line.color = color;
    line.x0 = line.x1 = x;
    line.y0 = y;
    line.y1 = y + height;
    Prim_DrawLine(&line);
    line.x1 = x + width;
    line.x0 = width + x;
    Prim_DrawLine(&line);
    line.y0 = line.y1 = y;
    line.x0 = x;
    Prim_DrawLine(&line);
    line.y0 = line.y1 = height + y;
    Prim_DrawLine(&line);
}

