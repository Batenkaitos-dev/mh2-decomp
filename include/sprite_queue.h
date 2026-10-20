#ifndef SPRITE_QUEUE_H
#define SPRITE_QUEUE_H

/*
 * The sprite queue: Sprite_Queue turns a sprite's description (SprDef) into
 * one or two queued primitives (Prim) in gPrims, and Prim_DrawAll draws the
 * frame's queue, in order, after the scene.
 */
#include "common.h"
#include "prim.h"

#define PRIM_QUEUE_MAX      0x100   /* the primitives gPrims holds */

/* SprDef.flags. */
#define SPR_ALIGN_X_MASK    0x0003  /* where pos[0] is on the sprite: SPR_ALIGN_* */
#define SPR_ALIGN_Y_MASK    0x000C  /* the same for pos[1], shifted by SPR_ALIGN_Y_SHIFT */
#define SPR_ALIGN_Y_SHIFT   2
#define SPR_ALIGN_START     0       /* (and 1) the left or top edge */
#define SPR_ALIGN_CENTER    2       /* the centre */
#define SPR_ALIGN_END       3       /* the right or bottom edge */
#define SPR_FLIP_X          0x0010  /* mirrored: the anchor's side and the u's swapped */
#define SPR_FLIP_Y          0x0020  /* the same for the y and the v's */
#define SPR_FILTER          0x1000  /* filtered texture (PRIM_FILTER) */
#define SPR_BILLBOARD       0x2000  /* turned to face the camera (by the inverse of the view matrix) */
#define SPR_ADDITIVE        0x4000  /* drawn additively (Prim.additive) */

/* Prim.flags, and SprDef.primFlags's low nibble. */
#define PRIM_FLAGS_MASK     0x0F    /* the bits of SprDef.primFlags Sprite_Queue copies */
#define PRIM_KIND_MASK      0x03
#define PRIM_FILL_2D        0       /* a filled rectangle on the screen (Prim_FillRectF) */
#define PRIM_FILL_3D        1       /* a filled rectangle in the world (Prim_DrawRect3D) */
#define PRIM_SPRITE_2D      2       /* a textured rectangle on the screen (Prim_DrawSpriteF) */
#define PRIM_SPRITE_3D      3       /* a textured rectangle in the world (Prim_DrawSprite3D) */
#define PRIM_ROTATED        0x04    /* PRIM_SPRITE_3D as a rotated triangle (Prim_DrawTexTri3D): two Prims per sprite */
#define PRIM_FILTER         0x08    /* filtered texture (Gfx_SetTexFilter) */
#define PRIM_IS_3D          0x01    /* the world kinds: their y grows upward */

/* A sprite to queue (Sprite_Queue), 0x4C bytes. */
typedef struct SprDef {
    s32 tex;                    /* 0x00: the texture slot */
    f32 pos[3];                 /* 0x04: the anchor (SPR_ALIGN_*), z the depth */
    f32 size[2];                /* 0x10 */
    f32 scale[2];               /* 0x18: size is multiplied by it */
    f32 uv[4];                  /* 0x20: u0, v0, u1, v1 */
    f32 alpha;                  /* 0x30: 0.0 to 1.0 */
    s32 primFlags;              /* 0x34: PRIM_* (low nibble); Sprite_Queue sets PRIM_ROTATED for SPR_BILLBOARD */
    s32 flags;                  /* 0x38: SPR_* */
    u32 rgb : 24;               /* 0x3C: the colour; alpha gives the top byte */
    u32 pad3C : 8;
    f32 rot[3];                 /* 0x40: degrees, about x, y and z (PRIM_ROTATED) */
} SprDef;

/* A point (x, y, z): the corners Sprite_Queue copies into the primitives
 * as whole records. */
typedef struct PrimPoint {
    f32 x;
    f32 y;
    f32 z;
} PrimPoint;

/* Prim.data of PRIM_FILL_3D and of PRIM_SPRITE_3D (prim.h's PrimSprite3D,
 * with its corners as PrimPoints). */
typedef struct PrimRect3DView {
    PrimPoint p0;               /* 0x00 */
    PrimPoint p1;               /* 0x0C */
    u32 color;                  /* 0x18 */
    f32 st[4];                  /* 0x1C: u0, v0, u1, v1 */
} PrimRect3DView;

/* Prim.data of PRIM_SPRITE_3D | PRIM_ROTATED (prim.h's PrimTexTri3D, with
 * its corners as PrimPoints). */
typedef struct PrimTri3DView {
    PrimPoint v[3];             /* 0x00 */
    u32 color[3];               /* 0x24 */
    f32 st[3][2];               /* 0x30 */
} PrimTri3DView;

/* A queued primitive, 0x50 bytes (gPrims). */
typedef struct Prim {
    u8 flags;                   /* 0x00: PRIM_* */
    u8 additive;                /* 0x01: nonzero: added to the screen; else alpha blended */
    u16 tex;                    /* 0x02: the texture slot (PRIM_SPRITE_*) */
    f32 z;                      /* 0x04: the depth */
    union {                     /* 0x08: the draw function's record, by kind */
        PrimLineF fill;         /* PRIM_FILL_2D */
        PrimSpriteF sprite;     /* PRIM_SPRITE_2D; Sprite_Queue fills PRIM_FILL_2D through it too */
        PrimRect3D fill3d;      /* PRIM_FILL_3D */
        PrimSprite3D sprite3d;  /* PRIM_SPRITE_3D */
        PrimTexTri3D tri3d;     /* PRIM_SPRITE_3D | PRIM_ROTATED */
        PrimRect3DView rect;    /* the world kinds' corners as Sprite_Queue fills them */
        PrimTri3DView tri;      /* the same for the rotated triangle */
    } data;
} Prim;

extern Prim *gPrims;            /* PRIM_QUEUE_MAX primitives (gSysSettings.prims) */
extern s16 gPrimCount;          /* the primitives queued this frame */

#endif /* SPRITE_QUEUE_H */
