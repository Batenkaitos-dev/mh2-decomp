#ifndef PRIM_H
#define PRIM_H

/*
 * The boot ELF's 2D primitives (src/main/unk_186D80.c). Each one copies a
 * GIF packet template (DMA END tag, GIF tag in REGLIST mode, PRIM) to the
 * scratchpad at 0x70000000, writes its register values after it and sends
 * the packet with Prim_SendPacket.
 */
#include "common.h"

/* 64-bit GS register values. */
#define GS_RGBAQ(r, g, b, a, q) \
    ((u64)(r) | ((u64)(g) << 8) | ((u64)(b) << 16) | ((u64)(a) << 24) | ((u64)(q) << 32))
#define GS_XYZ(x, y, z) ((u64)(x) | ((u64)(y) << 16) | ((u64)(z) << 32))
#define GS_UV(u, v) ((u64)(u) | ((u64)(v) << 16))
#define GS_ST(s, t) ((u64)(s) | ((u64)(t) << 32))

/* A line (Prim_DrawLine) or a filled rectangle (Prim_FillRect) in screen
 * coordinates, 0x0C bytes. */
typedef struct PrimLine {
    s16 x0;                     /* 0x00 */
    s16 y0;                     /* 0x02 */
    s16 x1;                     /* 0x04 */
    s16 y1;                     /* 0x06 */
    u32 color;                  /* 0x08: 0xAARRGGBB */
} PrimLine;

/* A rectangle with a colour per corner (Prim_FillGradRect), 0x18 bytes. */
typedef struct PrimGradRect {
    s16 x0;                     /* 0x00 */
    s16 y0;                     /* 0x02 */
    s16 x1;                     /* 0x04 */
    s16 y1;                     /* 0x06 */
    u32 color[4];               /* 0x08: top left, top right, bottom left, bottom right */
} PrimGradRect;

/* A flat triangle (Prim_FillTri), 0x10 bytes. */
typedef struct PrimTri {
    s16 x0;                     /* 0x00 */
    s16 y0;                     /* 0x02 */
    s16 x1;                     /* 0x04 */
    s16 y1;                     /* 0x06 */
    s16 x2;                     /* 0x08 */
    s16 y2;                     /* 0x0A */
    u32 color;                  /* 0x0C */
} PrimTri;

/* A textured triangle (Prim_DrawTexTri), texel coordinates, 0x1C bytes. */
typedef struct PrimTexTri {
    s16 x0;                     /* 0x00 */
    s16 y0;                     /* 0x02 */
    s16 x1;                     /* 0x04 */
    s16 y1;                     /* 0x06 */
    s16 x2;                     /* 0x08 */
    s16 y2;                     /* 0x0A */
    u32 color;                  /* 0x0C */
    s16 u0;                     /* 0x10 */
    s16 v0;                     /* 0x12 */
    s16 u1;                     /* 0x14 */
    s16 v1;                     /* 0x16 */
    s16 u2;                     /* 0x18 */
    s16 v2;                     /* 0x1A */
} PrimTexTri;

/* A vertex in screen coordinates with depth (z 1.0 is the near plane) and
 * the q of the texture coordinates. */
typedef struct PrimVtx {
    f32 x;                      /* 0x00 */
    f32 y;                      /* 0x04 */
    f32 z;                      /* 0x08 */
    f32 q;                      /* 0x0C */
} PrimVtx;

/* A line (Prim_DrawLineF) or a filled rectangle (Prim_FillRectF), 0x24 bytes. */
typedef struct PrimLineF {
    PrimVtx v[2];               /* 0x00 */
    u32 color;                  /* 0x20 */
} PrimLineF;

/* A textured rectangle (Prim_DrawSpriteF), 0x34 bytes. */
typedef struct PrimSpriteF {
    PrimVtx v[2];               /* 0x00 */
    u32 color;                  /* 0x20 */
    f32 st[2][2];               /* 0x24 */
} PrimSpriteF;

/* A textured triangle with a colour per vertex (Prim_DrawTexTriF), 0x54 bytes. */
typedef struct PrimTexTriF {
    PrimVtx v[3];               /* 0x00 */
    u32 color[3];               /* 0x30 */
    f32 st[3][2];               /* 0x3C */
} PrimTexTriF;

/* The same in world coordinates, projected by the view matrices. */
typedef struct PrimRect3D {
    f32 p[2][3];                /* 0x00 */
    u32 color;                  /* 0x18 */
} PrimRect3D;

typedef struct PrimSprite3D {
    f32 p[2][3];                /* 0x00 */
    u32 color;                  /* 0x18 */
    f32 st[2][2];               /* 0x1C */
} PrimSprite3D;

typedef struct PrimTexTri3D {
    f32 p[3][3];                /* 0x00 */
    u32 color[3];               /* 0x24 */
    f32 st[3][2];               /* 0x30 */
} PrimTexTri3D;

#endif /* PRIM_H */
