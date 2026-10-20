/*
 * The boot ELF's 2D and 3D primitives (Prim_*: lines, rectangles,
 * gradients, triangles and sprites, in screen space, as floats and in 3D),
 * the wait for the frame's flip, and the power-off thread (PowerOff_*).
 */
#include "common.h"
#include "dma.h"
#include "prim.h"
#include "sif.h"
#include "sprite.h"
#include "hw_regs.h"

/* prototypes */
s32 Sound_Update(void);
s32 func_00162040(void);
void func_00162188(void);
void Sys_FlipTick(void);
extern s32 gSysField;
extern s32 gSysFlipAt;
extern s32 gSysVblanks;
extern s32 gSysFlipPending;
/* end of prototypes */

extern DmaQueue gDmaQueue;
extern s32 gSysFrameMode;
extern volatile f32 gSysZMax;    /* reloaded after each packet store */
extern s32 gSysOffsetX;
extern s32 gSysOffsetY;
extern s32 gSysScrollY;
extern s32 gSysNoDraw;
extern s32 gRenderFlags;
extern s32 gDispHeight;
extern s32 gDispWidth;
extern f32 gViewClipH;
extern f32 gViewClipW;
extern f32 gViewCenterY;
extern f32 gViewCenterX;
/* Each Prim_ function's packet head (DMA tag, GIF tag, PRIM), copied to the scratchpad */
extern u128 gPrimLinePacket[];
extern u128 gPrimRectPacket[];
extern u128 gPrimGradRectPacket[];
extern u128 gSpritePacket[];
extern u128 gPrimTriPacket[];
extern u128 gPrimTexTriPacket[];
extern u128 gPrimLineFPacket[];
extern u128 gPrimTexTriFPacket[];
extern u128 gPrimRectFPacket[];
extern u128 gPrimSpriteFPacket[];
extern u128 gPrimRect3DPacket[];
extern u128 gPrimSprite3DPacket[];
extern u128 gPrimTexTri3DPacket[];
extern f32 gMtxProjScreen[];
extern f32 gMtxSlots[];
extern f32 gMtxView[];

void *Packet_Alloc(u32 size, s32 align);
s32 DmaQueue_AddChain(s32 mode, u32 entry, u32 *last, DmaQueue *q);
void Mem_CopyQwords(void *src, void *dst, s32 qwc);
s16 Gs_ScreenX(s16 x);
s16 Gs_ScreenY(s16 y);
f32 Gs_ScreenXf(f32 x);
f32 Gs_ScreenYf(f32 y);
f32 Gs_DepthToZ(f32 z);
u32 Gs_ColorToGs(u32 color, s32 mode);
void Mtx_Mul(f32 *dst, f32 *a, f32 *b);
void Vec_Project(f32 *out, f32 *v, f32 *m);

/* Sony's kernel thread parameters */
struct ThreadParam {
    s32 status;
    void *entry;
    void *stack;
    s32 stackSize;
    void *gpReg;
    s32 initPriority;
    s32 currentPriority;
    u32 attr;
    u32 option;
    s32 waitType;
    s32 waitId;
    s32 wakeupCount;
};

extern s32 gSysExiting;
extern volatile s32 gPowerOffStat;
extern u8 gPowerOffStack[];
extern u8 _gp[];
s32 CreateThread(struct ThreadParam *p);
s32 GetThreadId(void);
s32 ChangeThreadPriority(s32 tid, s32 prio);
s32 iSignalSema(s32 sema);
s32 func_00114C60(s32 tid, s32 arg);
s32 func_00118378(void (*func)(s32), s32 arg);
s32 sceSifSearchModuleByName(char *name);
s32 sceDevctl(char *name, s32 cmd, void *arg, u32 arglen, void *buf, u32 buflen);
s32 sceCdPowerOff(volatile s32 *stat);
void PowerOff_Thread(s32 sema);
void PowerOff_Callback(s32 sema);

/*
 * The end-of-frame wait, called by Sys_FlipFrame. Runs Sound_Update and
 * the CRI (ADX) server call. When the frame ended in time (gSysVblanks at
 * most gSysFlipAt) it stores the GS CSR's FIELD bit in gSysField, requests
 * the flip (Sys_FlipTick) and keeps calling a CRI library routine until
 * gSysFlipPending is cleared at the VBlank. A late frame requests the flip
 * and makes that call once, without waiting.
 */
void Sys_WaitFlip(void)
{
    Sound_Update();
    func_00162040();
    if (gSysVblanks <= gSysFlipAt) {
        gSysField = (s32) ((s64) ((((u64) GS_CSR >> 0xD) & 1) << 0x20) >> 0x20);
        Sys_FlipTick();
        do {
            func_00162188();
        } while (gSysFlipPending != 0);
        return;
    }
    Sys_FlipTick();
    func_00162188();
}

/*
 * Sends a primitive's packet (built at the scratchpad by the Prim_
 * functions): the DMA tag's low 15 bits give the qwords after the tag.
 * Sets the tag's top bit, writes the VIF codes FLUSHA (0x13) and DIRECT
 * (0x51, with the GIF data's qword count) into the tag's upper words,
 * copies the packet into the frame's packet buffer (Packet_Alloc) and
 * adds the copy to gDmaQueue's chain. Returns 1.
 */
s32 Prim_SendPacket(u32 *packet)
{
    s32 qwc;
    u32 *queued;

    qwc = (packet[0] & 0x7FFF) + 1;
    queued = Packet_Alloc(qwc << 4, 16);
    packet[0] |= 0x80000000;
    packet[2] = 0x13000000;
    packet[3] = (qwc - 1) | 0x51000000;
    Mem_CopyQwords(packet, queued, qwc);
    DmaQueue_AddChain(0, ((u32)queued & 0x0FFFFFFF) | 0x40000000, queued, &gDmaQueue);
    return 1;
}

/*
 * 1 when (x, y) lies within gViewClipW and gViewClipH of the view centre
 * (gViewCenterX, gViewCenterY) and z is between 0 and gSysZMax, else 0.
 * Prim_DrawLine's test for each end.
 */
static s32 Prim_InViewClip(f32 x, f32 y, f32 z)
{
    if (gViewCenterX + gViewClipW < x) {
        return 0;
    }
    if (gViewCenterX - gViewClipW > x) {
        return 0;
    }
    if (gViewCenterY + gViewClipH < y) {
        return 0;
    }
    if (gViewCenterY - gViewClipH > y) {
        return 0;
    }
    if (z < 0.0f) {
        return 0;
    }
    if (gSysZMax < z) {
        return 0;
    }
    return 1;
}

/*
 * 1 when z is between 0 and gSysZMax and (x, y), moved by gSysOffsetX
 * and gSysOffsetY, falls inside the GS's 4096 x 4096 coordinate space,
 * else 0. Prim_DrawLineF's test for each end.
 */
static s32 Prim_InGsRange(f32 x, f32 y, f32 z)
{
    if (z < 0.0f) {
        return 0;
    }
    if (gSysZMax < z) {
        return 0;
    }
    if (-(f32)gSysOffsetX > x) {
        return 0;
    }
    if (4096.0f - (f32)gSysOffsetX <= x) {
        return 0;
    }
    if (-(f32)gSysOffsetY > y) {
        return 0;
    }
    if (4096.0f - (f32)gSysOffsetY <= y) {
        return 0;
    }
    return 1;
}

/*
 * Draws a line from (x0, y0) to (x1, y1) of line, in the 640-wide screen
 * coordinates (x scaled to gDispWidth), in line->color (0xAARRGGBB; only
 * the alpha is converted to the GS's range). Returns 0, drawing nothing,
 * when an end falls outside the view (Prim_InViewClip), else 1; also 1
 * when gSysNoDraw is set.
 */
s32 Prim_DrawLine(PrimLine *line)
{
    u64 *gifData;
    u32 gsColor;
    s16 x, y;

    if (gSysNoDraw != 0) {
        return 1;
    }
    Mem_CopyQwords(gPrimLinePacket, (void *)SCRATCHPAD, 3);
    gifData = (u64 *)(SCRATCHPAD + 0x28);
    gsColor = Gs_ColorToGs(line->color, 1);
    *gifData++ = GS_RGBAQ((gsColor >> 16) & 0xFF, (gsColor >> 8) & 0xFF, gsColor & 0xFF, (gsColor >> 24) & 0xFF, 1);
    x = Gs_ScreenX(line->x0 * ((f32)gDispWidth / 640.0f));
    y = Gs_ScreenY(line->y0);
    if (Prim_InViewClip((s32)x, (s32)y, 1.0f) == 0) {
        return 0;
    }
    *gifData++ = GS_XYZ((gSysOffsetX + x) << 4, (gSysOffsetY + y) << 4, (u32)gSysZMax);
    x = Gs_ScreenX(line->x1 * ((f32)gDispWidth / 640.0f));
    y = Gs_ScreenY(line->y1);
    if (Prim_InViewClip((s32)x, (s32)y, 1.0f) == 0) {
        return 0;
    }
    *gifData++ = GS_XYZ((gSysOffsetX + x) << 4, (gSysOffsetY + y) << 4, (u32)gSysZMax);
    Prim_SendPacket((u32 *)SCRATCHPAD);
    return 1;
}

/*
 * Fills the rectangle (x0, y0)-(x1, y1) of rect in the 640-wide screen
 * coordinates: x is scaled to gDispWidth, the display is centred in the
 * GS's 4096-pixel space (originX, originY, in 1/16 pixels) and y is
 * halved in field mode (gSysFrameMode 0). rect->color is 0xAARRGGBB; only
 * its alpha is converted. Returns 1.
 */
s32 Prim_FillRect(PrimLine *rect)
{
    u64 *gifData;
    s32 x0, y0, x1, y1;
    s32 originX, originY;
    f32 scale;
    u32 gsColor;

    if (gSysNoDraw != 0) {
        return 1;
    }
    originX = ((0x1000 - gDispWidth) / 2) << 4;
    originY = ((0x1000 - gDispHeight) / 2) << 4;
    scale = (f32)gDispWidth / 640.0f;
    x0 = 16.0f * (rect->x0 * scale);
    y0 = rect->y0 << 4;
    x1 = 16.0f * (rect->x1 * scale);
    y1 = rect->y1 << 4;
    if (gSysFrameMode == 0) {
        y0 >>= 1;
        y1 >>= 1;
    }
    x0 += originX;
    x1 += originX;
    y0 += originY;
    y1 += originY;
    Mem_CopyQwords(gPrimRectPacket, (void *)SCRATCHPAD, 3);
    gifData = (u64 *)(SCRATCHPAD + 0x28);
    gsColor = Gs_ColorToGs(rect->color, 1);
    *gifData++ = GS_RGBAQ((gsColor >> 16) & 0xFF, (gsColor >> 8) & 0xFF, gsColor & 0xFF, (gsColor >> 24) & 0xFF, 1);
    *gifData++ = GS_XYZ(x0, y0, (u32)gSysZMax);
    *gifData++ = GS_XYZ(x1, y1, (u32)gSysZMax);
    Prim_SendPacket((u32 *)SCRATCHPAD);
    return 1;
}

/*
 * Fills the rectangle (x0, y0)-(x1, y1) of rect as Prim_FillRect does,
 * with a colour per corner (color[0] top left, [1] top right, [2] bottom
 * left, [3] bottom right) shaded between them. Returns 1.
 */
s32 Prim_FillGradRect(PrimGradRect *rect)
{
    u64 *gifData;
    s32 x0, x1, y0, y1;
    u64 left, bottom;
    s32 originX, originY;
    f32 scale;
    u32 gsColor;

    if (gSysNoDraw != 0) {
        return 1;
    }
    Mem_CopyQwords(gPrimGradRectPacket, (void *)SCRATCHPAD, 3);
    originX = ((0x1000 - gDispWidth) / 2) << 4;
    originY = ((0x1000 - gDispHeight) / 2) << 4;
    scale = (f32)gDispWidth / 640.0f;
    x0 = 16.0f * (rect->x0 * scale);
    y0 = rect->y0 << 4;
    x1 = 16.0f * (rect->x1 * scale);
    y1 = rect->y1 << 4;
    if (gSysFrameMode == 0) {
        y0 >>= 1;
        y1 >>= 1;
    }
    x0 += originX;
    x1 += originX;
    y0 += originY;
    y1 += originY;
    gifData = (u64 *)(SCRATCHPAD + 0x28);
    gsColor = Gs_ColorToGs(rect->color[0], 1);
    *gifData++ = GS_RGBAQ((gsColor >> 16) & 0xFF, (gsColor >> 8) & 0xFF, gsColor & 0xFF, (gsColor >> 24) & 0xFF, 1);
    left = x0;
    *gifData++ = GS_XYZ(left, y0, (u32)gSysZMax);
    gsColor = Gs_ColorToGs(rect->color[1], 1);
    *gifData++ = GS_RGBAQ((gsColor >> 16) & 0xFF, (gsColor >> 8) & 0xFF, gsColor & 0xFF, (gsColor >> 24) & 0xFF, 1);
    *gifData++ = GS_XYZ(x1, y0, (u32)gSysZMax);
    gsColor = Gs_ColorToGs(rect->color[2], 1);
    *gifData++ = GS_RGBAQ((gsColor >> 16) & 0xFF, (gsColor >> 8) & 0xFF, gsColor & 0xFF, (gsColor >> 24) & 0xFF, 1);
    bottom = (u64)y1 << 16;
    *gifData++ = left | bottom | ((u64)(u32)gSysZMax << 32);
    gsColor = Gs_ColorToGs(rect->color[3], 1);
    *gifData++ = GS_RGBAQ((gsColor >> 16) & 0xFF, (gsColor >> 8) & 0xFF, gsColor & 0xFF, (gsColor >> 24) & 0xFF, 1);
    *gifData++ = (u64)x1 | bottom | ((u64)(u32)gSysZMax << 32);
    Prim_SendPacket((u32 *)SCRATCHPAD);
    return 1;
}

/*
 * Draws a textured rectangle with the texture bound before (Tex_Bind): the
 * screen corner (x, y) of sprite and its size w by h, in the 640-wide
 * coordinates placed as Prim_FillRect places them, and the texel
 * rectangle (u0, v0)-(u1, v1). Half a texel is added to the texture
 * coordinates unless gRenderFlags has bit 0x10000. sprite->color tints it,
 * each channel converted to the GS's range (0x80 is 1.0). Returns 1.
 */
s32 Sprite_Draw(Sprite *sprite)
{
    u64 *gifData;
    s32 left;
    s32 top;
    s32 right;
    s32 bottom;
    s32 width;
    s32 height;
    s32 texRight;
    s32 texLeft;
    s32 texTop;
    s32 texBottom;
    s32 originX;
    s32 originY;
    f32 scale;
    u32 gsColor;

    if (gSysNoDraw != 0) {
        return 1;
    }
    originX = ((0x1000 - gDispWidth) / 2) << 4;
    originY = ((0x1000 - gDispHeight) / 2) << 4;
    scale = (f32)gDispWidth / 640.0f;
    left = 16.0f * (sprite->x * scale);
    top = sprite->y << 4;
    width = 16.0f * (sprite->w * scale);
    height = sprite->h << 4;
    if (gSysFrameMode == 0) {
        top >>= 1;
        height >>= 1;
    }
    left += originX;
    right = left + width;
    top += originY;
    bottom = top + height;
    Mem_CopyQwords(gSpritePacket, (void *)SCRATCHPAD, 3);
    gifData = (u64 *)(SCRATCHPAD + 0x28);
    gsColor = Gs_ColorToGs(sprite->color, 0);
    *gifData++ = GS_RGBAQ((gsColor >> 16) & 0xFF, (gsColor >> 8) & 0xFF, gsColor & 0xFF, (gsColor >> 24) & 0xFF, 1);
    texLeft = sprite->u0 << 4;
    texRight = sprite->u1 << 4;
    texTop = sprite->v0 << 4;
    texBottom = sprite->v1 << 4;
    if (!(gRenderFlags & 0x10000)) {
        texLeft += 8;
        texTop += 8;
        texRight += 8;
        texBottom += 8;
    }
    *gifData++ = GS_UV(texLeft, texTop);
    *gifData++ = GS_XYZ(left, top, (u32)gSysZMax);
    *gifData++ = GS_UV(texRight, texBottom);
    *gifData++ = GS_XYZ(right, bottom, (u32)gSysZMax);
    Prim_SendPacket((u32 *)SCRATCHPAD);
    return 1;
}

/*
 * Fills the triangle (x0, y0), (x1, y1), (x2, y2) of tri in one colour,
 * the coordinates placed as Prim_FillRect places them; only the colour's
 * alpha is converted. Returns 1.
 */
s32 Prim_FillTri(PrimTri *tri)
{
    u64 *gifData;
    s32 x0, x1, x2, y0, y1, y2;
    s32 originX, originY;
    f32 scale;
    u32 gsColor;

    if (gSysNoDraw != 0) {
        return 1;
    }
    Mem_CopyQwords(gPrimTriPacket, (void *)SCRATCHPAD, 3);
    originX = ((0x1000 - gDispWidth) / 2) << 4;
    originY = ((0x1000 - gDispHeight) / 2) << 4;
    scale = (f32)gDispWidth / 640.0f;
    x0 = 16.0f * (tri->x0 * scale);
    y0 = tri->y0 << 4;
    x1 = 16.0f * (tri->x1 * scale);
    y1 = tri->y1 << 4;
    x2 = 16.0f * (tri->x2 * scale);
    y2 = tri->y2 << 4;
    if (gSysFrameMode == 0) {
        y0 >>= 1;
        y1 >>= 1;
        y2 >>= 1;
    }
    x0 += originX;
    x1 += originX;
    x2 += originX;
    y0 += originY;
    y1 += originY;
    y2 += originY;
    gifData = (u64 *)(SCRATCHPAD + 0x28);
    gsColor = Gs_ColorToGs(tri->color, 1);
    *gifData++ = GS_RGBAQ((gsColor >> 16) & 0xFF, (gsColor >> 8) & 0xFF, gsColor & 0xFF, (gsColor >> 24) & 0xFF, 1);
    *gifData++ = GS_XYZ(x0, y0, (u32)gSysZMax);
    *gifData++ = GS_XYZ(x1, y1, (u32)gSysZMax);
    *gifData++ = GS_XYZ(x2, y2, (u32)gSysZMax);
    Prim_SendPacket((u32 *)SCRATCHPAD);
    return 1;
}

/*
 * Draws the triangle of tri, placed as Prim_FillTri places it, with the
 * bound texture: the texel coordinates (u0, v0), (u1, v1), (u2, v2) take
 * half a texel more unless gRenderFlags has bit 0x10000, and tri->color
 * tints it (each channel converted). Returns 1.
 */
s32 Prim_DrawTexTri(PrimTexTri *tri)
{
    u64 *gifData;
    s32 x0, y0, x1, y1, x2, y2;
    s32 u1, u0, v0, v1, u2, v2;
    s32 originX, originY;
    f32 scale;
    u32 gsColor;

    if (gSysNoDraw != 0) {
        return 1;
    }
    originX = ((0x1000 - gDispWidth) / 2) << 4;
    originY = ((0x1000 - gDispHeight) / 2) << 4;
    scale = (f32)gDispWidth / 640.0f;
    x0 = 16.0f * (tri->x0 * scale);
    y0 = tri->y0 << 4;
    x1 = 16.0f * (tri->x1 * scale);
    y1 = tri->y1 << 4;
    x2 = 16.0f * (tri->x2 * scale);
    y2 = tri->y2 << 4;
    if (gSysFrameMode == 0) {
        y0 >>= 1;
        y1 >>= 1;
        y2 >>= 1;
    }
    x0 += originX;
    y0 += originY;
    x1 += originX;
    y1 += originY;
    x2 += originX;
    y2 += originY;
    Mem_CopyQwords(gPrimTexTriPacket, (void *)SCRATCHPAD, 3);
    gifData = (u64 *)(SCRATCHPAD + 0x28);
    gsColor = Gs_ColorToGs(tri->color, 0);
    *gifData++ = GS_RGBAQ((gsColor >> 16) & 0xFF, (gsColor >> 8) & 0xFF, gsColor & 0xFF, (gsColor >> 24) & 0xFF, 1);
    u0 = tri->u0 << 4;
    u1 = tri->u1 << 4;
    u2 = tri->u2 << 4;
    v0 = tri->v0 << 4;
    v1 = tri->v1 << 4;
    v2 = tri->v2 << 4;
    if (!(gRenderFlags & 0x10000)) {
        u0 += 8;
        v0 += 8;
        u1 += 8;
        v1 += 8;
        u2 += 8;
        v2 += 8;
    }
    *gifData++ = GS_UV(u0, v0);
    *gifData++ = GS_XYZ(x0, y0, (u32)gSysZMax);
    *gifData++ = GS_UV(u1, v1);
    *gifData++ = GS_XYZ(x1, y1, (u32)gSysZMax);
    *gifData++ = GS_UV(u2, v2);
    *gifData++ = GS_XYZ(x2, y2, (u32)gSysZMax);
    Prim_SendPacket((u32 *)SCRATCHPAD);
    return 1;
}

/*
 * Draws a line between line's two vertices, given in screen coordinates
 * with depth (Gs_ScreenXf, Gs_ScreenYf, Gs_DepthToZ). Returns 0, drawing
 * nothing, when a vertex falls outside the GS's range (Prim_InGsRange),
 * else 1.
 */
s32 Prim_DrawLineF(PrimLineF *line)
{
    u64 *gifData;
    f32 x, y, z;
    u32 gsColor;

    if (gSysNoDraw != 0) {
        return 1;
    }
    Mem_CopyQwords(gPrimLineFPacket, (void *)SCRATCHPAD, 3);
    gifData = (u64 *)(SCRATCHPAD + 0x28);
    gsColor = Gs_ColorToGs(line->color, 1);
    *gifData++ = GS_RGBAQ((gsColor >> 16) & 0xFF, (gsColor >> 8) & 0xFF, gsColor & 0xFF, (gsColor >> 24) & 0xFF, *(u32 *)&line->v[0].q);
    x = Gs_ScreenXf(line->v[0].x);
    y = Gs_ScreenYf(line->v[0].y);
    z = Gs_DepthToZ(line->v[0].z);
    if (Prim_InGsRange((s32)x, (s32)y, z) == 0) {
        return 0;
    }
    *gifData++ = GS_XYZ((s32)(gSysOffsetX + x) << 4, (s32)(gSysOffsetY + y) << 4, (u32)z);
    x = Gs_ScreenXf(line->v[1].x);
    y = Gs_ScreenYf(line->v[1].y);
    z = Gs_DepthToZ(line->v[1].z);
    if (Prim_InGsRange((s32)x, (s32)y, z) == 0) {
        return 0;
    }
    *gifData++ = GS_XYZ((s32)(gSysOffsetX + x) << 4, (s32)(gSysOffsetY + y) << 4, (u32)z);
    Prim_SendPacket((u32 *)SCRATCHPAD);
    return 1;
}

/*
 * Draws tri's three screen-space vertices (as Prim_DrawLineF converts
 * them) as a triangle with the bound texture, a colour per vertex and
 * perspective texture coordinates: each vertex's st times its q (texS,
 * texT), with q in the colour's Q. No range test. Returns 1.
 */
s32 Prim_DrawTexTriF(PrimTexTriF *tri)
{
    u64 *gifData;
    f32 x, y, z;
    f32 texS, texT;
    u32 gsColor;

    if (gSysNoDraw != 0) {
        return 1;
    }
    Mem_CopyQwords(gPrimTexTriFPacket, (void *)SCRATCHPAD, 3);
    gifData = (u64 *)(SCRATCHPAD + 0x28);
    gsColor = Gs_ColorToGs(tri->color[0], 0);
    *gifData++ = GS_RGBAQ((gsColor >> 16) & 0xFF, (gsColor >> 8) & 0xFF, gsColor & 0xFF, (gsColor >> 24) & 0xFF, *(u32 *)&tri->v[0].q);
    texS = tri->st[0][0] * tri->v[0].q;
    texT = tri->st[0][1] * tri->v[0].q;
    *gifData++ = GS_ST(*(u32 *)&texS, *(u32 *)&texT);
    x = Gs_ScreenXf(tri->v[0].x);
    y = Gs_ScreenYf(tri->v[0].y);
    z = Gs_DepthToZ(tri->v[0].z);
    *gifData++ = GS_XYZ((s32)(gSysOffsetX + x) << 4, (s32)(gSysOffsetY + y) << 4, (u32)z);
    gsColor = Gs_ColorToGs(tri->color[1], 0);
    *gifData++ = GS_RGBAQ((gsColor >> 16) & 0xFF, (gsColor >> 8) & 0xFF, gsColor & 0xFF, (gsColor >> 24) & 0xFF, *(u32 *)&tri->v[1].q);
    texS = tri->st[1][0] * tri->v[1].q;
    texT = tri->st[1][1] * tri->v[1].q;
    *gifData++ = GS_ST(*(u32 *)&texS, *(u32 *)&texT);
    x = Gs_ScreenXf(tri->v[1].x);
    y = Gs_ScreenYf(tri->v[1].y);
    z = Gs_DepthToZ(tri->v[1].z);
    *gifData++ = GS_XYZ((s32)(gSysOffsetX + x) << 4, (s32)(gSysOffsetY + y) << 4, (u32)z);
    gsColor = Gs_ColorToGs(tri->color[2], 0);
    *gifData++ = GS_RGBAQ((gsColor >> 16) & 0xFF, (gsColor >> 8) & 0xFF, gsColor & 0xFF, (gsColor >> 24) & 0xFF, *(u32 *)&tri->v[2].q);
    texS = tri->st[2][0] * tri->v[2].q;
    texT = tri->st[2][1] * tri->v[2].q;
    *gifData++ = GS_ST(*(u32 *)&texS, *(u32 *)&texT);
    x = Gs_ScreenXf(tri->v[2].x);
    y = Gs_ScreenYf(tri->v[2].y);
    z = Gs_DepthToZ(tri->v[2].z);
    *gifData++ = GS_XYZ((s32)(gSysOffsetX + x) << 4, (s32)(gSysOffsetY + y) << 4, (u32)z);
    Prim_SendPacket((u32 *)SCRATCHPAD);
    return 1;
}

/*
 * Fills the rectangle between rect's two screen-space vertices: x is
 * scaled to gDispWidth before Gs_ScreenXf, y and the depth are converted
 * as Prim_DrawLineF converts them. No range test. Returns 1.
 */
s32 Prim_FillRectF(PrimLineF *rect)
{
    u64 *gifData;
    f32 x, y, z;
    u32 gsColor;

    if (gSysNoDraw != 0) {
        return 1;
    }
    Mem_CopyQwords(gPrimRectFPacket, (void *)SCRATCHPAD, 3);
    gifData = (u64 *)(SCRATCHPAD + 0x28);
    gsColor = Gs_ColorToGs(rect->color, 1);
    *gifData++ = GS_RGBAQ((gsColor >> 16) & 0xFF, (gsColor >> 8) & 0xFF, gsColor & 0xFF, (gsColor >> 24) & 0xFF, *(u32 *)&rect->v[0].q);
    x = Gs_ScreenXf(rect->v[0].x * ((f32)gDispWidth / 640.0f));
    y = Gs_ScreenYf(rect->v[0].y);
    z = Gs_DepthToZ(rect->v[0].z);
    *gifData++ = GS_XYZ((s32)(gSysOffsetX + x) << 4, (s32)(gSysOffsetY + y) << 4, (u32)z);
    x = Gs_ScreenXf(rect->v[1].x * ((f32)gDispWidth / 640.0f));
    y = Gs_ScreenYf(rect->v[1].y);
    z = Gs_DepthToZ(rect->v[1].z);
    *gifData++ = GS_XYZ((s32)(gSysOffsetX + x) << 4, (s32)(gSysOffsetY + y) << 4, (u32)z);
    Prim_SendPacket((u32 *)SCRATCHPAD);
    return 1;
}

/*
 * Draws a textured rectangle between sprite's two vertices with
 * perspective texture coordinates (st times q). x is scaled to gDispWidth
 * and placed by Gs_ScreenX; y has gSysScrollY taken off and is halved in
 * field mode (gSysFrameMode 0); the depth is converted as Gs_DepthToZ
 * does. Returns 1.
 * Kept for the match: the (s32) around the y word's (s32)(...) << 4 has
 * no effect (the value is s32 already), but only with it does the
 * compiler compute y first and then or x into it as the game does. The
 * scaled x goes through y (y = x * scale, then Gs_ScreenX(y)) so the
 * product is in the same register as in the game.
 */
s32 Prim_DrawSpriteF(PrimSpriteF *sprite)
{
    u64 *gifData;
    f32 x, y;
    f32 texS, texT;
    u32 gsColor;

    if (gSysNoDraw != 0) {
        return 1;
    }
    Mem_CopyQwords(gPrimSpriteFPacket, (void *)SCRATCHPAD, 3);
    gifData = (u64 *)(SCRATCHPAD + 0x28);
    gsColor = Gs_ColorToGs(sprite->color, 0);
    *gifData++ = GS_RGBAQ((gsColor >> 16) & 0xFF, (gsColor >> 8) & 0xFF, gsColor & 0xFF, (gsColor >> 24) & 0xFF, *(u32 *)&sprite->v[0].q);
    texS = sprite->st[0][0] * sprite->v[0].q;
    texT = sprite->st[0][1] * sprite->v[0].q;
    *gifData++ = GS_ST(*(u32 *)&texS, *(u32 *)&texT);
    x = sprite->v[0].x;
    y = x * ((f32)gDispWidth / 640.0f);
    x = Gs_ScreenX(y);
    if (gSysFrameMode == 0) {
        y = 0.5f * (sprite->v[0].y - gSysScrollY);
    } else {
        y = sprite->v[0].y - gSysScrollY;
    }
    *gifData++ = GS_XYZ((s32)(gSysOffsetX + x) << 4, (s32)((s32)(gSysOffsetY + y) << 4),
                  (u32)(-0.5f * (sprite->v[0].z - 1.0f) * gSysZMax));
    texS = sprite->st[1][0] * sprite->v[1].q;
    texT = sprite->st[1][1] * sprite->v[1].q;
    *gifData++ = GS_ST(*(u32 *)&texS, *(u32 *)&texT);
    x = sprite->v[1].x;
    y = x * ((f32)gDispWidth / 640.0f);
    x = Gs_ScreenX(y);
    if (gSysFrameMode == 0) {
        y = 0.5f * (sprite->v[1].y - gSysScrollY);
    } else {
        y = sprite->v[1].y - gSysScrollY;
    }
    *gifData++ = GS_XYZ((s32)(gSysOffsetX + x) << 4, (s32)((s32)(gSysOffsetY + y) << 4),
                  (u32)(-0.5f * (sprite->v[1].z - 1.0f) * gSysZMax));
    Prim_SendPacket((u32 *)SCRATCHPAD);
    return 1;
}

/*
 * Fills a rectangle between two world-space corners (rect->p[0], p[1]):
 * each is projected (Vec_Project) by worldToScreen, the product of
 * gMtxSlots, gMtxView and gMtxProjScreen, and placed as Prim_DrawLineF
 * places its vertices; the colour's Q is 1 / w of the first corner. Only
 * the colour's alpha is converted. Returns 1.
 */
s32 Prim_DrawRect3D(PrimRect3D *rect)
{
    f32 projected[4];
    f32 worldToScreen[16];
    u64 *gifData;
    f32 x, y, z;
    u32 gsColor;

    if (gSysNoDraw != 0) {
        return 1;
    }
    Mem_CopyQwords(gPrimRect3DPacket, (void *)SCRATCHPAD, 3);
    Mtx_Mul(worldToScreen, gMtxSlots, gMtxView);
    Mtx_Mul(worldToScreen, worldToScreen, gMtxProjScreen);
    gifData = (u64 *)(SCRATCHPAD + 0x28);
    Vec_Project(projected, rect->p[0], worldToScreen);
    projected[3] = 1.0f / projected[3];
    gsColor = Gs_ColorToGs(rect->color, 1);
    *gifData++ = GS_RGBAQ((gsColor >> 16) & 0xFF, (gsColor >> 8) & 0xFF, gsColor & 0xFF, (gsColor >> 24) & 0xFF, *(u32 *)&projected[3]);
    x = Gs_ScreenXf(projected[0]);
    y = Gs_ScreenYf(projected[1]);
    z = Gs_DepthToZ(projected[2]);
    *gifData++ = GS_XYZ((s32)(gSysOffsetX + x) << 4, (s32)(gSysOffsetY + y) << 4, (u32)z);
    Vec_Project(projected, rect->p[1], worldToScreen);
    projected[3] = 1.0f / projected[3];
    x = Gs_ScreenXf(projected[0]);
    y = Gs_ScreenYf(projected[1]);
    z = Gs_DepthToZ(projected[2]);
    *gifData++ = GS_XYZ((s32)(gSysOffsetX + x) << 4, (s32)(gSysOffsetY + y) << 4, (u32)z);
    Prim_SendPacket((u32 *)SCRATCHPAD);
    return 1;
}

/*
 * Draws a textured rectangle between two world-space corners projected as
 * Prim_DrawRect3D projects them, with perspective texture coordinates
 * (each corner's st over its w) and sprite->color as the tint (each
 * channel converted). Returns 1.
 */
s32 Prim_DrawSprite3D(PrimSprite3D *sprite)
{
    f32 texS, texT;
    f32 projected[4];
    f32 worldToScreen[16];
    u64 *gifData;
    f32 x, y, z;
    u32 gsColor;

    if (gSysNoDraw != 0) {
        return 1;
    }
    Mem_CopyQwords(gPrimSprite3DPacket, (void *)SCRATCHPAD, 3);
    Mtx_Mul(worldToScreen, gMtxSlots, gMtxView);
    Mtx_Mul(worldToScreen, worldToScreen, gMtxProjScreen);
    gifData = (u64 *)(SCRATCHPAD + 0x28);
    gsColor = Gs_ColorToGs(sprite->color, 0);
    Vec_Project(projected, sprite->p[0], worldToScreen);
    projected[3] = 1.0f / projected[3];
    *gifData++ = GS_RGBAQ((gsColor >> 16) & 0xFF, (gsColor >> 8) & 0xFF, gsColor & 0xFF, (gsColor >> 24) & 0xFF, *(u32 *)&projected[3]);
    texS = sprite->st[0][0] * projected[3];
    texT = sprite->st[0][1] * projected[3];
    *gifData++ = GS_ST(*(u32 *)&texS, *(u32 *)&texT);
    x = Gs_ScreenXf(projected[0]);
    y = Gs_ScreenYf(projected[1]);
    z = Gs_DepthToZ(projected[2]);
    *gifData++ = GS_XYZ((s32)(gSysOffsetX + x) << 4, (s32)(gSysOffsetY + y) << 4, (u32)z);
    Vec_Project(projected, sprite->p[1], worldToScreen);
    projected[3] = 1.0f / projected[3];
    texS = sprite->st[1][0] * projected[3];
    texT = sprite->st[1][1] * projected[3];
    *gifData++ = GS_ST(*(u32 *)&texS, *(u32 *)&texT);
    x = Gs_ScreenXf(projected[0]);
    y = Gs_ScreenYf(projected[1]);
    z = Gs_DepthToZ(projected[2]);
    *gifData++ = GS_XYZ((s32)(gSysOffsetX + x) << 4, (s32)(gSysOffsetY + y) << 4, (u32)z);
    Prim_SendPacket((u32 *)SCRATCHPAD);
    return 1;
}

/*
 * Draws a textured triangle of three world-space vertices projected as
 * Prim_DrawRect3D projects them, with a colour per vertex and perspective
 * texture coordinates (each vertex's st over its w). Returns 1.
 */
s32 Prim_DrawTexTri3D(PrimTexTri3D *tri)
{
    f32 texS, texT;
    f32 projected[4];
    f32 worldToScreen[16];
    u64 *gifData;
    f32 x, y, z;
    u32 gsColor;

    if (gSysNoDraw != 0) {
        return 1;
    }
    Mem_CopyQwords(gPrimTexTri3DPacket, (void *)SCRATCHPAD, 3);
    Mtx_Mul(worldToScreen, gMtxSlots, gMtxView);
    Mtx_Mul(worldToScreen, worldToScreen, gMtxProjScreen);
    gifData = (u64 *)(SCRATCHPAD + 0x28);
    gsColor = Gs_ColorToGs(tri->color[0], 0);
    Vec_Project(projected, tri->p[0], worldToScreen);
    projected[3] = 1.0f / projected[3];
    *gifData++ = GS_RGBAQ((gsColor >> 16) & 0xFF, (gsColor >> 8) & 0xFF, gsColor & 0xFF, (gsColor >> 24) & 0xFF, *(u32 *)&projected[3]);
    texS = tri->st[0][0] * projected[3];
    texT = tri->st[0][1] * projected[3];
    *gifData++ = GS_ST(*(u32 *)&texS, *(u32 *)&texT);
    x = Gs_ScreenXf(projected[0]);
    y = Gs_ScreenYf(projected[1]);
    z = Gs_DepthToZ(projected[2]);
    *gifData++ = GS_XYZ((s32)(gSysOffsetX + x) << 4, (s32)(gSysOffsetY + y) << 4, (u32)z);
    gsColor = Gs_ColorToGs(tri->color[1], 0);
    Vec_Project(projected, tri->p[1], worldToScreen);
    projected[3] = 1.0f / projected[3];
    *gifData++ = GS_RGBAQ((gsColor >> 16) & 0xFF, (gsColor >> 8) & 0xFF, gsColor & 0xFF, (gsColor >> 24) & 0xFF, *(u32 *)&projected[3]);
    texS = tri->st[1][0] * projected[3];
    texT = tri->st[1][1] * projected[3];
    *gifData++ = GS_ST(*(u32 *)&texS, *(u32 *)&texT);
    x = Gs_ScreenXf(projected[0]);
    y = Gs_ScreenYf(projected[1]);
    z = Gs_DepthToZ(projected[2]);
    *gifData++ = GS_XYZ((s32)(gSysOffsetX + x) << 4, (s32)(gSysOffsetY + y) << 4, (u32)z);
    gsColor = Gs_ColorToGs(tri->color[2], 0);
    Vec_Project(projected, tri->p[2], worldToScreen);
    projected[3] = 1.0f / projected[3];
    *gifData++ = GS_RGBAQ((gsColor >> 16) & 0xFF, (gsColor >> 8) & 0xFF, gsColor & 0xFF, (gsColor >> 24) & 0xFF, *(u32 *)&projected[3]);
    texS = tri->st[2][0] * projected[3];
    texT = tri->st[2][1] * projected[3];
    *gifData++ = GS_ST(*(u32 *)&texS, *(u32 *)&texT);
    x = Gs_ScreenXf(projected[0]);
    y = Gs_ScreenYf(projected[1]);
    z = Gs_DepthToZ(projected[2]);
    *gifData++ = GS_XYZ((s32)(gSysOffsetX + x) << 4, (s32)(gSysOffsetY + y) << 4, (u32)z);
    Prim_SendPacket((u32 *)SCRATCHPAD);
    return 1;
}

/*
 * Sets up the power-off handling: creates a semaphore, sets the current
 * thread's priority to 2, creates and starts PowerOff_Thread (priority 1,
 * stack gPowerOffStack of 0x2000 bytes) with the semaphore as its
 * argument, and registers PowerOff_Callback with the library's power-off
 * hook. Called during the IOP set-up when the power-off module is there.
 * Returns 1.
 */
s32 PowerOff_Init(void)
{
    struct ThreadParam threadParam;
    SemaParam semaParam;
    s32 sema;

    semaParam.maxCount = 1;
    semaParam.initCount = 0;
    semaParam.option = 0;
    sema = CreateSema(&semaParam);
    ChangeThreadPriority(GetThreadId(), 2);
    threadParam.stackSize = 0x2000;
    threadParam.gpReg = _gp;
    threadParam.entry = PowerOff_Thread;
    threadParam.stack = gPowerOffStack;
    threadParam.initPriority = 1;
    func_00114C60(CreateThread(&threadParam), sema);
    func_00118378(PowerOff_Callback, sema);
    return 1;
}

/*
 * Waits on sema (signalled by PowerOff_Callback), then sets gSysExiting
 * and shuts the console down: with the hdd module loaded it asks hdd: for
 * its status (devctl 0x4807); on status 3 it switches the DEV9 bay off
 * (dev9x: 0x4402), otherwise it closes pfs: (0x5003) and shuts down the
 * disk (hdd: 0x4806). Without the module it switches DEV9 off. Each of
 * these devctls but the pfs: one is retried until it succeeds; then
 * cdrom0: (0x4328) the same way, then sceCdPowerOff until it succeeds
 * with gPowerOffStat 0, and the thread spins forever.
 */
void PowerOff_Thread(s32 sema)
{
    WaitSema(sema);
    gSysExiting = 1;
    if (sceSifSearchModuleByName("hdd") >= 0) {
        gPowerOffStat = sceDevctl("hdd:", 0x4807, NULL, 0, NULL, 0);
        switch (gPowerOffStat) {
        case 3:
            while (sceDevctl("dev9x:", 0x4402, NULL, 0, NULL, 0) < 0) {
            }
            break;
        default:
            sceDevctl("pfs:", 0x5003, NULL, 0, NULL, 0);
            while (sceDevctl("hdd:", 0x4806, NULL, 0, NULL, 0) < 0) {
            }
            break;
        }
    } else {
        while (sceDevctl("dev9x:", 0x4402, NULL, 0, NULL, 0) < 0) {
        }
    }
    while (sceDevctl("cdrom0:", 0x4328, NULL, 0, NULL, 0) < 0) {
    }
    while (!sceCdPowerOff(&gPowerOffStat) || gPowerOffStat != 0) {
    }
    for (;;) {
    }
}

/* The power-off callback: signals sema to wake PowerOff_Thread. */
void PowerOff_Callback(s32 sema)
{
    iSignalSema(sema);
}

/* This file's initialised data. */
u64 gPrimLinePacket_u64[8] = {
#include "main/001BABB0.inc"
};
u64 gPrimRectPacket_u64[8] = {
#include "main/001BABF0.inc"
};
u64 gPrimGradRectPacket_u64[14] = {
#include "main/001BAC30.inc"
};
u64 gSpritePacket_u64[10] = {
#include "main/001BACA0.inc"
};
u64 gPrimTriPacket_u64[10] = {
#include "main/001BACF0.inc"
};
u64 gPrimTexTriPacket_u64[12] = {
#include "main/001BAD40.inc"
};
u64 gPrimLineFPacket_u64[8] = {
#include "main/001BADA0.inc"
};
u64 gPrimTexTriFPacket_u64[14] = {
#include "main/001BADE0.inc"
};
u64 gPrimRectFPacket_u64[8] = {
#include "main/001BAE50.inc"
};
u64 gPrimSpriteFPacket_u64[10] = {
#include "main/001BAE90.inc"
};
u64 gPrimRect3DPacket_u64[8] = {
#include "main/001BAEE0.inc"
};
u64 gPrimSprite3DPacket_u64[10] = {
#include "main/001BAF20.inc"
};
u64 gPrimTexTri3DPacket_u64[14] = {
#include "main/001BAF70.inc"
};
