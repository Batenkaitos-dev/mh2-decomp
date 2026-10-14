/*
 * main 0x186d80-0x189f90
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

s32 Prim_SendPacket(u32 *pkt)
{
    s32 qwc;
    u32 *p;

    qwc = (pkt[0] & 0x7FFF) + 1;
    p = Packet_Alloc(qwc << 4, 16);
    pkt[0] |= 0x80000000;
    pkt[2] = 0x13000000;
    pkt[3] = (qwc - 1) | 0x51000000;
    Mem_CopyQwords(pkt, p, qwc);
    DmaQueue_AddChain(0, ((u32)p & 0x0FFFFFFF) | 0x40000000, p, &gDmaQueue);
    return 1;
}

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

s32 Prim_DrawLine(PrimLine *l)
{
    u64 *p;
    u32 c;
    s16 x, y;

    if (gSysNoDraw != 0) {
        return 1;
    }
    Mem_CopyQwords(gPrimLinePacket, (void *)SCRATCHPAD, 3);
    p = (u64 *)(SCRATCHPAD + 0x28);
    c = Gs_ColorToGs(l->color, 1);
    *p++ = GS_RGBAQ((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF, (c >> 24) & 0xFF, 1);
    x = Gs_ScreenX(l->x0 * ((f32)gDispWidth / 640.0f));
    y = Gs_ScreenY(l->y0);
    if (Prim_InViewClip((s32)x, (s32)y, 1.0f) == 0) {
        return 0;
    }
    *p++ = GS_XYZ((gSysOffsetX + x) << 4, (gSysOffsetY + y) << 4, (u32)gSysZMax);
    x = Gs_ScreenX(l->x1 * ((f32)gDispWidth / 640.0f));
    y = Gs_ScreenY(l->y1);
    if (Prim_InViewClip((s32)x, (s32)y, 1.0f) == 0) {
        return 0;
    }
    *p++ = GS_XYZ((gSysOffsetX + x) << 4, (gSysOffsetY + y) << 4, (u32)gSysZMax);
    Prim_SendPacket((u32 *)SCRATCHPAD);
    return 1;
}

s32 Prim_FillRect(PrimLine *r)
{
    u64 *p;
    s32 x0, y0, x1, y1;
    s32 ox, oy;
    f32 scale;
    u32 c;

    if (gSysNoDraw != 0) {
        return 1;
    }
    ox = ((0x1000 - gDispWidth) / 2) << 4;
    oy = ((0x1000 - gDispHeight) / 2) << 4;
    scale = (f32)gDispWidth / 640.0f;
    x0 = 16.0f * (r->x0 * scale);
    y0 = r->y0 << 4;
    x1 = 16.0f * (r->x1 * scale);
    y1 = r->y1 << 4;
    if (gSysFrameMode == 0) {
        y0 >>= 1;
        y1 >>= 1;
    }
    x0 += ox;
    x1 += ox;
    y0 += oy;
    y1 += oy;
    Mem_CopyQwords(gPrimRectPacket, (void *)SCRATCHPAD, 3);
    p = (u64 *)(SCRATCHPAD + 0x28);
    c = Gs_ColorToGs(r->color, 1);
    *p++ = GS_RGBAQ((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF, (c >> 24) & 0xFF, 1);
    *p++ = GS_XYZ(x0, y0, (u32)gSysZMax);
    *p++ = GS_XYZ(x1, y1, (u32)gSysZMax);
    Prim_SendPacket((u32 *)SCRATCHPAD);
    return 1;
}

s32 Prim_FillGradRect(PrimGradRect *r)
{
    u64 *p;
    s32 x0, x1, y0, y1;
    u64 left, bottom;
    s32 ox, oy;
    f32 scale;
    u32 c;

    if (gSysNoDraw != 0) {
        return 1;
    }
    Mem_CopyQwords(gPrimGradRectPacket, (void *)SCRATCHPAD, 3);
    ox = ((0x1000 - gDispWidth) / 2) << 4;
    oy = ((0x1000 - gDispHeight) / 2) << 4;
    scale = (f32)gDispWidth / 640.0f;
    x0 = 16.0f * (r->x0 * scale);
    y0 = r->y0 << 4;
    x1 = 16.0f * (r->x1 * scale);
    y1 = r->y1 << 4;
    if (gSysFrameMode == 0) {
        y0 >>= 1;
        y1 >>= 1;
    }
    x0 += ox;
    x1 += ox;
    y0 += oy;
    y1 += oy;
    p = (u64 *)(SCRATCHPAD + 0x28);
    c = Gs_ColorToGs(r->color[0], 1);
    *p++ = GS_RGBAQ((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF, (c >> 24) & 0xFF, 1);
    left = x0;
    *p++ = GS_XYZ(left, y0, (u32)gSysZMax);
    c = Gs_ColorToGs(r->color[1], 1);
    *p++ = GS_RGBAQ((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF, (c >> 24) & 0xFF, 1);
    *p++ = GS_XYZ(x1, y0, (u32)gSysZMax);
    c = Gs_ColorToGs(r->color[2], 1);
    *p++ = GS_RGBAQ((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF, (c >> 24) & 0xFF, 1);
    bottom = (u64)y1 << 16;
    *p++ = left | bottom | ((u64)(u32)gSysZMax << 32);
    c = Gs_ColorToGs(r->color[3], 1);
    *p++ = GS_RGBAQ((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF, (c >> 24) & 0xFF, 1);
    *p++ = (u64)x1 | bottom | ((u64)(u32)gSysZMax << 32);
    Prim_SendPacket((u32 *)SCRATCHPAD);
    return 1;
}

s32 Sprite_Draw(Sprite *s)
{
    u64 *p;
    s32 x0;
    s32 y0;
    s32 x1;
    s32 y1;
    s32 w;
    s32 h;
    s32 u1;
    s32 u0;
    s32 v0;
    s32 v1;
    s32 ox;
    s32 oy;
    f32 scale;
    u32 c;

    if (gSysNoDraw != 0) {
        return 1;
    }
    ox = ((0x1000 - gDispWidth) / 2) << 4;
    oy = ((0x1000 - gDispHeight) / 2) << 4;
    scale = (f32)gDispWidth / 640.0f;
    x0 = 16.0f * (s->x * scale);
    y0 = s->y << 4;
    w = 16.0f * (s->w * scale);
    h = s->h << 4;
    if (gSysFrameMode == 0) {
        y0 >>= 1;
        h >>= 1;
    }
    x0 += ox;
    x1 = x0 + w;
    y0 += oy;
    y1 = y0 + h;
    Mem_CopyQwords(gSpritePacket, (void *)SCRATCHPAD, 3);
    p = (u64 *)(SCRATCHPAD + 0x28);
    c = Gs_ColorToGs(s->color, 0);
    *p++ = GS_RGBAQ((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF, (c >> 24) & 0xFF, 1);
    u0 = s->u0 << 4;
    u1 = s->u1 << 4;
    v0 = s->v0 << 4;
    v1 = s->v1 << 4;
    if (!(gRenderFlags & 0x10000)) {
        u0 += 8;
        v0 += 8;
        u1 += 8;
        v1 += 8;
    }
    *p++ = GS_UV(u0, v0);
    *p++ = GS_XYZ(x0, y0, (u32)gSysZMax);
    *p++ = GS_UV(u1, v1);
    *p++ = GS_XYZ(x1, y1, (u32)gSysZMax);
    Prim_SendPacket((u32 *)SCRATCHPAD);
    return 1;
}

s32 Prim_FillTri(PrimTri *t)
{
    u64 *p;
    s32 x0, x1, x2, y0, y1, y2;
    s32 ox, oy;
    f32 scale;
    u32 c;

    if (gSysNoDraw != 0) {
        return 1;
    }
    Mem_CopyQwords(gPrimTriPacket, (void *)SCRATCHPAD, 3);
    ox = ((0x1000 - gDispWidth) / 2) << 4;
    oy = ((0x1000 - gDispHeight) / 2) << 4;
    scale = (f32)gDispWidth / 640.0f;
    x0 = 16.0f * (t->x0 * scale);
    y0 = t->y0 << 4;
    x1 = 16.0f * (t->x1 * scale);
    y1 = t->y1 << 4;
    x2 = 16.0f * (t->x2 * scale);
    y2 = t->y2 << 4;
    if (gSysFrameMode == 0) {
        y0 >>= 1;
        y1 >>= 1;
        y2 >>= 1;
    }
    x0 += ox;
    x1 += ox;
    x2 += ox;
    y0 += oy;
    y1 += oy;
    y2 += oy;
    p = (u64 *)(SCRATCHPAD + 0x28);
    c = Gs_ColorToGs(t->color, 1);
    *p++ = GS_RGBAQ((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF, (c >> 24) & 0xFF, 1);
    *p++ = GS_XYZ(x0, y0, (u32)gSysZMax);
    *p++ = GS_XYZ(x1, y1, (u32)gSysZMax);
    *p++ = GS_XYZ(x2, y2, (u32)gSysZMax);
    Prim_SendPacket((u32 *)SCRATCHPAD);
    return 1;
}

s32 Prim_DrawTexTri(PrimTexTri *t)
{
    u64 *p;
    s32 x0, y0, x1, y1, x2, y2;
    s32 u1, u0, v0, v1, u2, v2;
    s32 ox, oy;
    f32 scale;
    u32 c;

    if (gSysNoDraw != 0) {
        return 1;
    }
    ox = ((0x1000 - gDispWidth) / 2) << 4;
    oy = ((0x1000 - gDispHeight) / 2) << 4;
    scale = (f32)gDispWidth / 640.0f;
    x0 = 16.0f * (t->x0 * scale);
    y0 = t->y0 << 4;
    x1 = 16.0f * (t->x1 * scale);
    y1 = t->y1 << 4;
    x2 = 16.0f * (t->x2 * scale);
    y2 = t->y2 << 4;
    if (gSysFrameMode == 0) {
        y0 >>= 1;
        y1 >>= 1;
        y2 >>= 1;
    }
    x0 += ox;
    y0 += oy;
    x1 += ox;
    y1 += oy;
    x2 += ox;
    y2 += oy;
    Mem_CopyQwords(gPrimTexTriPacket, (void *)SCRATCHPAD, 3);
    p = (u64 *)(SCRATCHPAD + 0x28);
    c = Gs_ColorToGs(t->color, 0);
    *p++ = GS_RGBAQ((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF, (c >> 24) & 0xFF, 1);
    u0 = t->u0 << 4;
    u1 = t->u1 << 4;
    u2 = t->u2 << 4;
    v0 = t->v0 << 4;
    v1 = t->v1 << 4;
    v2 = t->v2 << 4;
    if (!(gRenderFlags & 0x10000)) {
        u0 += 8;
        v0 += 8;
        u1 += 8;
        v1 += 8;
        u2 += 8;
        v2 += 8;
    }
    *p++ = GS_UV(u0, v0);
    *p++ = GS_XYZ(x0, y0, (u32)gSysZMax);
    *p++ = GS_UV(u1, v1);
    *p++ = GS_XYZ(x1, y1, (u32)gSysZMax);
    *p++ = GS_UV(u2, v2);
    *p++ = GS_XYZ(x2, y2, (u32)gSysZMax);
    Prim_SendPacket((u32 *)SCRATCHPAD);
    return 1;
}

s32 Prim_DrawLineF(PrimLineF *l)
{
    u64 *p;
    f32 x, y, z;
    u32 c;

    if (gSysNoDraw != 0) {
        return 1;
    }
    Mem_CopyQwords(gPrimLineFPacket, (void *)SCRATCHPAD, 3);
    p = (u64 *)(SCRATCHPAD + 0x28);
    c = Gs_ColorToGs(l->color, 1);
    *p++ = GS_RGBAQ((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF, (c >> 24) & 0xFF, *(u32 *)&l->v[0].q);
    x = Gs_ScreenXf(l->v[0].x);
    y = Gs_ScreenYf(l->v[0].y);
    z = Gs_DepthToZ(l->v[0].z);
    if (Prim_InGsRange((s32)x, (s32)y, z) == 0) {
        return 0;
    }
    *p++ = GS_XYZ((s32)(gSysOffsetX + x) << 4, (s32)(gSysOffsetY + y) << 4, (u32)z);
    x = Gs_ScreenXf(l->v[1].x);
    y = Gs_ScreenYf(l->v[1].y);
    z = Gs_DepthToZ(l->v[1].z);
    if (Prim_InGsRange((s32)x, (s32)y, z) == 0) {
        return 0;
    }
    *p++ = GS_XYZ((s32)(gSysOffsetX + x) << 4, (s32)(gSysOffsetY + y) << 4, (u32)z);
    Prim_SendPacket((u32 *)SCRATCHPAD);
    return 1;
}

s32 Prim_DrawTexTriF(PrimTexTriF *t)
{
    u64 *p;
    f32 x, y, z;
    f32 s, tq;
    u32 c;

    if (gSysNoDraw != 0) {
        return 1;
    }
    Mem_CopyQwords(gPrimTexTriFPacket, (void *)SCRATCHPAD, 3);
    p = (u64 *)(SCRATCHPAD + 0x28);
    c = Gs_ColorToGs(t->color[0], 0);
    *p++ = GS_RGBAQ((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF, (c >> 24) & 0xFF, *(u32 *)&t->v[0].q);
    s = t->st[0][0] * t->v[0].q;
    tq = t->st[0][1] * t->v[0].q;
    *p++ = GS_ST(*(u32 *)&s, *(u32 *)&tq);
    x = Gs_ScreenXf(t->v[0].x);
    y = Gs_ScreenYf(t->v[0].y);
    z = Gs_DepthToZ(t->v[0].z);
    *p++ = GS_XYZ((s32)(gSysOffsetX + x) << 4, (s32)(gSysOffsetY + y) << 4, (u32)z);
    c = Gs_ColorToGs(t->color[1], 0);
    *p++ = GS_RGBAQ((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF, (c >> 24) & 0xFF, *(u32 *)&t->v[1].q);
    s = t->st[1][0] * t->v[1].q;
    tq = t->st[1][1] * t->v[1].q;
    *p++ = GS_ST(*(u32 *)&s, *(u32 *)&tq);
    x = Gs_ScreenXf(t->v[1].x);
    y = Gs_ScreenYf(t->v[1].y);
    z = Gs_DepthToZ(t->v[1].z);
    *p++ = GS_XYZ((s32)(gSysOffsetX + x) << 4, (s32)(gSysOffsetY + y) << 4, (u32)z);
    c = Gs_ColorToGs(t->color[2], 0);
    *p++ = GS_RGBAQ((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF, (c >> 24) & 0xFF, *(u32 *)&t->v[2].q);
    s = t->st[2][0] * t->v[2].q;
    tq = t->st[2][1] * t->v[2].q;
    *p++ = GS_ST(*(u32 *)&s, *(u32 *)&tq);
    x = Gs_ScreenXf(t->v[2].x);
    y = Gs_ScreenYf(t->v[2].y);
    z = Gs_DepthToZ(t->v[2].z);
    *p++ = GS_XYZ((s32)(gSysOffsetX + x) << 4, (s32)(gSysOffsetY + y) << 4, (u32)z);
    Prim_SendPacket((u32 *)SCRATCHPAD);
    return 1;
}

s32 Prim_FillRectF(PrimLineF *r)
{
    u64 *p;
    f32 x, y, z;
    u32 c;

    if (gSysNoDraw != 0) {
        return 1;
    }
    Mem_CopyQwords(gPrimRectFPacket, (void *)SCRATCHPAD, 3);
    p = (u64 *)(SCRATCHPAD + 0x28);
    c = Gs_ColorToGs(r->color, 1);
    *p++ = GS_RGBAQ((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF, (c >> 24) & 0xFF, *(u32 *)&r->v[0].q);
    x = Gs_ScreenXf(r->v[0].x * ((f32)gDispWidth / 640.0f));
    y = Gs_ScreenYf(r->v[0].y);
    z = Gs_DepthToZ(r->v[0].z);
    *p++ = GS_XYZ((s32)(gSysOffsetX + x) << 4, (s32)(gSysOffsetY + y) << 4, (u32)z);
    x = Gs_ScreenXf(r->v[1].x * ((f32)gDispWidth / 640.0f));
    y = Gs_ScreenYf(r->v[1].y);
    z = Gs_DepthToZ(r->v[1].z);
    *p++ = GS_XYZ((s32)(gSysOffsetX + x) << 4, (s32)(gSysOffsetY + y) << 4, (u32)z);
    Prim_SendPacket((u32 *)SCRATCHPAD);
    return 1;
}

/* FAKEMATCH: the (s32) around the y word's `(s32)(...) << 4` has no purpose (the
 * value is s32 already), but only with it does MWCC compute y first and still
 * `or` x into y as the game does; without it, 2 words off (the `or` operands).
 * The scaled x goes through y (y = x * (w / 640.0f), then Gs_ScreenX(y)) so the
 * product lands in f0 as in the game. */
s32 Prim_DrawSpriteF(PrimSpriteF *sp)
{
    u64 *p;
    f32 x, y;
    f32 s, t;
    u32 c;

    if (gSysNoDraw != 0) {
        return 1;
    }
    Mem_CopyQwords(gPrimSpriteFPacket, (void *)SCRATCHPAD, 3);
    p = (u64 *)(SCRATCHPAD + 0x28);
    c = Gs_ColorToGs(sp->color, 0);
    *p++ = GS_RGBAQ((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF, (c >> 24) & 0xFF, *(u32 *)&sp->v[0].q);
    s = sp->st[0][0] * sp->v[0].q;
    t = sp->st[0][1] * sp->v[0].q;
    *p++ = GS_ST(*(u32 *)&s, *(u32 *)&t);
    x = sp->v[0].x;
    y = x * ((f32)gDispWidth / 640.0f);
    x = Gs_ScreenX(y);
    if (gSysFrameMode == 0) {
        y = 0.5f * (sp->v[0].y - gSysScrollY);
    } else {
        y = sp->v[0].y - gSysScrollY;
    }
    *p++ = GS_XYZ((s32)(gSysOffsetX + x) << 4, (s32)((s32)(gSysOffsetY + y) << 4),
                  (u32)(-0.5f * (sp->v[0].z - 1.0f) * gSysZMax));
    s = sp->st[1][0] * sp->v[1].q;
    t = sp->st[1][1] * sp->v[1].q;
    *p++ = GS_ST(*(u32 *)&s, *(u32 *)&t);
    x = sp->v[1].x;
    y = x * ((f32)gDispWidth / 640.0f);
    x = Gs_ScreenX(y);
    if (gSysFrameMode == 0) {
        y = 0.5f * (sp->v[1].y - gSysScrollY);
    } else {
        y = sp->v[1].y - gSysScrollY;
    }
    *p++ = GS_XYZ((s32)(gSysOffsetX + x) << 4, (s32)((s32)(gSysOffsetY + y) << 4),
                  (u32)(-0.5f * (sp->v[1].z - 1.0f) * gSysZMax));
    Prim_SendPacket((u32 *)SCRATCHPAD);
    return 1;
}

s32 Prim_DrawRect3D(PrimRect3D *r)
{
    f32 v[4];
    f32 m[16];
    u64 *p;
    f32 x, y, z;
    u32 c;

    if (gSysNoDraw != 0) {
        return 1;
    }
    Mem_CopyQwords(gPrimRect3DPacket, (void *)SCRATCHPAD, 3);
    Mtx_Mul(m, gMtxSlots, gMtxView);
    Mtx_Mul(m, m, gMtxProjScreen);
    p = (u64 *)(SCRATCHPAD + 0x28);
    Vec_Project(v, r->p[0], m);
    v[3] = 1.0f / v[3];
    c = Gs_ColorToGs(r->color, 1);
    *p++ = GS_RGBAQ((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF, (c >> 24) & 0xFF, *(u32 *)&v[3]);
    x = Gs_ScreenXf(v[0]);
    y = Gs_ScreenYf(v[1]);
    z = Gs_DepthToZ(v[2]);
    *p++ = GS_XYZ((s32)(gSysOffsetX + x) << 4, (s32)(gSysOffsetY + y) << 4, (u32)z);
    Vec_Project(v, r->p[1], m);
    v[3] = 1.0f / v[3];
    x = Gs_ScreenXf(v[0]);
    y = Gs_ScreenYf(v[1]);
    z = Gs_DepthToZ(v[2]);
    *p++ = GS_XYZ((s32)(gSysOffsetX + x) << 4, (s32)(gSysOffsetY + y) << 4, (u32)z);
    Prim_SendPacket((u32 *)SCRATCHPAD);
    return 1;
}

s32 Prim_DrawSprite3D(PrimSprite3D *r)
{
    f32 s, t;
    f32 v[4];
    f32 m[16];
    u64 *p;
    f32 x, y, z;
    u32 c;

    if (gSysNoDraw != 0) {
        return 1;
    }
    Mem_CopyQwords(gPrimSprite3DPacket, (void *)SCRATCHPAD, 3);
    Mtx_Mul(m, gMtxSlots, gMtxView);
    Mtx_Mul(m, m, gMtxProjScreen);
    p = (u64 *)(SCRATCHPAD + 0x28);
    c = Gs_ColorToGs(r->color, 0);
    Vec_Project(v, r->p[0], m);
    v[3] = 1.0f / v[3];
    *p++ = GS_RGBAQ((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF, (c >> 24) & 0xFF, *(u32 *)&v[3]);
    s = r->st[0][0] * v[3];
    t = r->st[0][1] * v[3];
    *p++ = GS_ST(*(u32 *)&s, *(u32 *)&t);
    x = Gs_ScreenXf(v[0]);
    y = Gs_ScreenYf(v[1]);
    z = Gs_DepthToZ(v[2]);
    *p++ = GS_XYZ((s32)(gSysOffsetX + x) << 4, (s32)(gSysOffsetY + y) << 4, (u32)z);
    Vec_Project(v, r->p[1], m);
    v[3] = 1.0f / v[3];
    s = r->st[1][0] * v[3];
    t = r->st[1][1] * v[3];
    *p++ = GS_ST(*(u32 *)&s, *(u32 *)&t);
    x = Gs_ScreenXf(v[0]);
    y = Gs_ScreenYf(v[1]);
    z = Gs_DepthToZ(v[2]);
    *p++ = GS_XYZ((s32)(gSysOffsetX + x) << 4, (s32)(gSysOffsetY + y) << 4, (u32)z);
    Prim_SendPacket((u32 *)SCRATCHPAD);
    return 1;
}

s32 Prim_DrawTexTri3D(PrimTexTri3D *r)
{
    f32 s, t;
    f32 v[4];
    f32 m[16];
    u64 *p;
    f32 x, y, z;
    u32 c;

    if (gSysNoDraw != 0) {
        return 1;
    }
    Mem_CopyQwords(gPrimTexTri3DPacket, (void *)SCRATCHPAD, 3);
    Mtx_Mul(m, gMtxSlots, gMtxView);
    Mtx_Mul(m, m, gMtxProjScreen);
    p = (u64 *)(SCRATCHPAD + 0x28);
    c = Gs_ColorToGs(r->color[0], 0);
    Vec_Project(v, r->p[0], m);
    v[3] = 1.0f / v[3];
    *p++ = GS_RGBAQ((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF, (c >> 24) & 0xFF, *(u32 *)&v[3]);
    s = r->st[0][0] * v[3];
    t = r->st[0][1] * v[3];
    *p++ = GS_ST(*(u32 *)&s, *(u32 *)&t);
    x = Gs_ScreenXf(v[0]);
    y = Gs_ScreenYf(v[1]);
    z = Gs_DepthToZ(v[2]);
    *p++ = GS_XYZ((s32)(gSysOffsetX + x) << 4, (s32)(gSysOffsetY + y) << 4, (u32)z);
    c = Gs_ColorToGs(r->color[1], 0);
    Vec_Project(v, r->p[1], m);
    v[3] = 1.0f / v[3];
    *p++ = GS_RGBAQ((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF, (c >> 24) & 0xFF, *(u32 *)&v[3]);
    s = r->st[1][0] * v[3];
    t = r->st[1][1] * v[3];
    *p++ = GS_ST(*(u32 *)&s, *(u32 *)&t);
    x = Gs_ScreenXf(v[0]);
    y = Gs_ScreenYf(v[1]);
    z = Gs_DepthToZ(v[2]);
    *p++ = GS_XYZ((s32)(gSysOffsetX + x) << 4, (s32)(gSysOffsetY + y) << 4, (u32)z);
    c = Gs_ColorToGs(r->color[2], 0);
    Vec_Project(v, r->p[2], m);
    v[3] = 1.0f / v[3];
    *p++ = GS_RGBAQ((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF, (c >> 24) & 0xFF, *(u32 *)&v[3]);
    s = r->st[2][0] * v[3];
    t = r->st[2][1] * v[3];
    *p++ = GS_ST(*(u32 *)&s, *(u32 *)&t);
    x = Gs_ScreenXf(v[0]);
    y = Gs_ScreenYf(v[1]);
    z = Gs_DepthToZ(v[2]);
    *p++ = GS_XYZ((s32)(gSysOffsetX + x) << 4, (s32)(gSysOffsetY + y) << 4, (u32)z);
    Prim_SendPacket((u32 *)SCRATCHPAD);
    return 1;
}

s32 PowerOff_Init(void)
{
    struct ThreadParam tp;
    SemaParam sp;
    s32 sema;

    sp.maxCount = 1;
    sp.initCount = 0;
    sp.option = 0;
    sema = CreateSema(&sp);
    ChangeThreadPriority(GetThreadId(), 2);
    tp.stackSize = 0x2000;
    tp.gpReg = _gp;
    tp.entry = PowerOff_Thread;
    tp.stack = gPowerOffStack;
    tp.initPriority = 1;
    func_00114C60(CreateThread(&tp), sema);
    func_00118378(PowerOff_Callback, sema);
    return 1;
}

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

void PowerOff_Callback(s32 sema)
{
    iSignalSema(sema);
}

/* unk_186D80's .data (0x1BABB0-0x1BAFE0) */
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
