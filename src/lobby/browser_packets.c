/*
 * The lobby browser's GS packets for a sprite (Prim_Sprite), a filled rectangle (Prim_Rect) and a
 * flat triangle (Prim_Tri): each copies its template packet (gPrimSpriteTmpl, gPrimRectTmpl,
 * gPrimTriTmpl) to the scratchpad, fills in the colour, the coordinates and the texture's, and
 * sends it. They are the lobby's copies of main's own sprite and rectangle draws, and read main's
 * display and render state as those do.
 */
#include "common.h"
#include "browser.h"
#include "html.h"
#include "lobby_globals.h"
#include "hw_regs.h"
#include "render_state.h"
#include "syswork.h"
#include "gs_tex_regs.h"

/* Compiled at optimization level 4, as the HTML parser that follows it. */
#pragma optimization_level 4

extern s32 gSysFrameMode;
/* main's display and render state, which the primitives read as main's
 * own draws (Sprite_Draw, Prim_FillRect) do. */
#define gPrimFrame (gSysFrameMode)          /* FRAME_MODE_*: FRAME_MODE_FIELD draws at half height */
extern s32 gSysFbWidth;
#define gPrimScreenW (gSysFbWidth)
extern s32 gSysFbLines;
#define gPrimScreenH (gSysFbLines)
extern f32 gSysZMax;
#define gPrimZ (*(volatile f32 *)&gSysZMax)
extern s32 gSysNoDraw;
#define gPrimOff (gSysNoDraw)
extern u32 gRenderFlags;
#define gPrimFlags (gRenderFlags)
#define GS_RGBAQ(r, g, b, a, q)     ((u64)(r) | ((u64)(g) << 8) | ((u64)(b) << 16) | ((u64)(a) << 24) | ((u64)(q) << 32))
#define GS_XYZ(x, y, z) ((u64)(x) | ((u64)(y) << 16) | ((u64)(z) << 32))
#define GS_UV(u, v) ((u64)(u) | ((u64)(v) << 16))
void Mem_CopyQwords(void *src, void *dst, s32 qwc);  /* main: copies quadwords */
extern u64 gPrimSpriteTmpl[];                         /* the packets' templates: Prim_Sprite's */
extern u64 gPrimRectTmpl[];                         /* Prim_Rect's */
extern u64 gPrimTriTmpl[];                         /* Prim_Tri's */

/*
 * Draws a textured sprite with the texture bound before: the screen
 * corner and size of sprite, in the 640-wide coordinates centred in the
 * GS's 4096-unit space (in sixteenths), halved in height on a field
 * frame, and its texel rectangle. Half a texel is added to the texture
 * coordinates when the textures are filtered (bilinear), and none with
 * point sampling (RF_TEX_NEAREST in gRenderFlags, which RS_TEX_FILTER
 * sets). sprite->color tints it.
 */
void Prim_Sprite(Sprite *sprite)
{
    u64 *gifData;
    s32 left;
    s32 top;
    s32 right;
    s32 bottom;
    s32 width;
    s32 height;
    s32 originX;
    s32 originY;
    f32 scale;
    u32 gsColor;

    if (gPrimOff != 0) {
        return;
    }
    originX = ((4096 - gPrimScreenW) / 2) << 4;
    originY = ((4096 - gPrimScreenH) / 2) << 4;
    scale = (f32)gPrimScreenW / 640.0f;
    left = 16.0f * (sprite->x * scale);
    top = sprite->y << 4;
    width = 16.0f * (sprite->w * scale);
    height = sprite->h << 4;
    if (gPrimFrame == FRAME_MODE_FIELD) {
        top >>= 1;
        height >>= 1;
    }
    left += originX;
    right = left + width;
    top += originY;
    bottom = top + height;
    Mem_CopyQwords(gPrimSpriteTmpl, (void *)SCRATCHPAD, 3);
    gifData = (u64 *)(SCRATCHPAD + 24);
    *gifData++ = 0x535310;  /* the GIF tag's register list: RGBAQ, UV, XYZ2, UV, XYZ2 */
    gifData++;
    gsColor = Gs_ColorToGs(sprite->color, 0);
    *gifData++ = GS_RGBAQ((gsColor >> 16) & 0xFF, (gsColor >> 8) & 0xFF, gsColor & 0xFF, (gsColor >> 24) & 0xFF, 1);
    if ((gPrimFlags & RF_TEX_NEAREST) == RF_TEX_NEAREST) {
        *gifData++ = GS_UV(sprite->u0 << 4, sprite->v0 << 4);
        *gifData++ = GS_XYZ(left, top, (u32)gPrimZ);
        *gifData++ = GS_UV(sprite->u1 << 4, sprite->v1 << 4);
        *gifData++ = GS_XYZ(right, bottom, (u32)gPrimZ);
    } else {
        *gifData++ = GS_UV((sprite->u0 << 4) + 8, (sprite->v0 << 4) + 8);
        *gifData++ = GS_XYZ(left, top, (u32)gPrimZ);
        *gifData++ = GS_UV((sprite->u1 << 4) + 8, (sprite->v1 << 4) + 8);
        *gifData++ = GS_XYZ(right, bottom, (u32)gPrimZ);
    }
    Prim_SendPacket((u32 *)SCRATCHPAD);
}

/*
 * Draws a filled rectangle from (x0, y0) to (x1, y1) of rect, placed as
 * Prim_Sprite places a sprite, in rect->color.
 */
void Prim_Rect(PrimLine *rect)
{
    u64 *gifData;
    s32 left;
    s32 top;
    s32 right;
    s32 bottom;
    s32 originX;
    s32 originY;
    f32 scale;
    u32 gsColor;

    if (gPrimOff != 0) {
        return;
    }
    originX = ((4096 - gPrimScreenW) / 2) << 4;
    originY = ((4096 - gPrimScreenH) / 2) << 4;
    scale = (f32)gPrimScreenW / 640.0f;
    left = 16.0f * (rect->x0 * scale);
    top = rect->y0 << 4;
    right = 16.0f * (rect->x1 * scale);
    bottom = rect->y1 << 4;
    if (gPrimFrame == FRAME_MODE_FIELD) {
        top >>= 1;
        bottom >>= 1;
    }
    left += originX;
    right += originX;
    top += originY;
    bottom += originY;
    Mem_CopyQwords(gPrimRectTmpl, (void *)SCRATCHPAD, 3);
    gifData = (u64 *)(SCRATCHPAD + 40);
    gsColor = Gs_ColorToGs(rect->color, 1);
    *gifData++ = GS_RGBAQ((gsColor >> 16) & 0xFF, (gsColor >> 8) & 0xFF, gsColor & 0xFF, (gsColor >> 24) & 0xFF, 1);
    *gifData++ = GS_XYZ(left, top, (u32)gPrimZ);
    *gifData++ = GS_XYZ(right, bottom, (u32)gPrimZ);
    Prim_SendPacket((u32 *)SCRATCHPAD);
}

/*
 * Draws a filled triangle of tri's three corners, placed as Prim_Sprite
 * places a sprite, in tri->color. Returns 1.
 */
s32 Prim_Tri(PrimTri *tri)
{
    u64 *gifData;
    s32 x0;
    s32 x1;
    s32 y0;
    s32 y1;
    s32 x2;
    s32 y2;
    s32 originX;
    s32 originY;
    f32 scale;
    u32 gsColor;

    if (gPrimOff != 0) {
        return 1;
    }
    Mem_CopyQwords(gPrimTriTmpl, (void *)SCRATCHPAD, 3);
    gifData = (u64 *)(SCRATCHPAD + 40);
    gsColor = Gs_ColorToGs(tri->color, 1);
    *gifData++ = GS_RGBAQ((gsColor >> 16) & 0xFF, (gsColor >> 8) & 0xFF, gsColor & 0xFF, (gsColor >> 24) & 0xFF, 1);
    originX = ((4096 - gPrimScreenW) / 2) << 4;
    originY = ((4096 - gPrimScreenH) / 2) << 4;
    scale = (f32)gPrimScreenW / 640.0f;
    x0 = 16.0f * (tri->x0 * scale);
    y0 = tri->y0 << 4;
    x1 = 16.0f * (tri->x1 * scale);
    y1 = tri->y1 << 4;
    x2 = 16.0f * (tri->x2 * scale);
    y2 = tri->y2 << 4;
    if (gPrimFrame == FRAME_MODE_FIELD) {
        y0 >>= 1;
        y1 >>= 1;
        y2 >>= 1;
    }
    x0 += originX;
    y0 += originY;
    x1 += originX;
    x2 += originX;
    y1 += originY;
    y2 += originY;
    *gifData++ = GS_XYZ(x0, y0, (u32)gPrimZ);
    *gifData++ = GS_XYZ(x1, y1, (u32)gPrimZ);
    *gifData++ = GS_XYZ(x2, y2, (u32)gPrimZ);
    Prim_SendPacket((u32 *)SCRATCHPAD);
    return 1;
}

/* The packets' templates. */
u64 gPrimSpriteTmpl[10] = {
#include "lobby/007EEE30.inc"
};
u64 gPrimRectTmpl[8] = {
#include "lobby/007EEE80.inc"
};
u64 gPrimTriTmpl[10] = {
#include "lobby/007EEEC0.inc"
};
