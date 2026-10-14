#ifndef SPRITE_H
#define SPRITE_H

#include "common.h"

/*
 * A textured rectangle as Sprite_Draw (main) draws it, 0x14 bytes; also
 * the lobby browser's Prim_Sprite (unk_741900.c). The texture rectangle
 * is two corners (u0, v0)-(u1, v1); the screen one is a corner and a size.
 * Other views of it: unk_3549F0.c (the UVs as Vec2s pairs).
 */
typedef struct Sprite {
    s16 x, y, w, h;
    u32 color;                  /* 0x08: 0xAABBGGRR */
    s16 u0, v0, u1, v1;         /* 0x0C */
} Sprite;

/*
 * A draw slot of sub_main's pools (0x20 bytes): gDrawSlotPool (0x200),
 * gDrawSlotsB (0x100) and gHudDrawHooks (4), in unk_376C00.c. DrawSlot_Alloc
 * takes a free one (it sets draw to 1 until the owner sets its callback) and
 * returns its index or -1, DrawSlot_Get gives its address and DrawSlot_Free
 * clears it (DrawSlotB_* for the second pool; DrawSlotB_Free keeps owner and
 * part). Its owner copies its position into pos and puts it into an ordering
 * table every frame (OT_AddByDepth(gDrawOtSorted, slot, 0x40, 0)), whose walk
 * calls draw with the slot.
 */
struct DrawSlot;
/* A draw slot's callback: called with the slot (the ordering table walk). */
typedef void (*DrawSlotFn)(struct DrawSlot *slot);

typedef struct DrawSlot {
    u32 next;                   /* 0x00: the ordering table link: the next node's address | 1 (TagList_Push, OT_InsertSorted), a tagged word, not a plain pointer */
    f32 key;                    /* 0x04: the view depth (OT_AddByDepth) */
    f32 pos[3];                 /* 0x08: where it is drawn; its depth is taken from it */
    DrawSlotFn draw;            /* 0x14: called with the slot; nonzero while allocated (DrawSlot_Alloc sets 1) */
    union {
        void *owner;            /* 0x18: the object it draws (a StageObj, Obj, Em, ObjAttach...) */
        s32 player;             /* 0x18: the player draw callbacks (DrawCb_LocalPlayer...): a gPlayers index */
    };
    s32 part;                   /* 0x1C: the index of the owner's part it draws (o->buf + part) */
} DrawSlot;
typedef char DrawSlot_size_check[(sizeof(DrawSlot) == 0x20) ? 1 : -1];

/* The ordering tables the frame walks (sub_main 0x6A9AD0-0x6A9D00): each entry heads a list of
 * draw slots (DrawSlot.next) that OT_AddByDepth (by depth) and OT_AddAt (at an index) add to.
 * FreeList_Init sets them up (unk_368F10.c) and Task_RunList walks each in the frame's draw order
 * (unk_36BE70.c); the one-entry lists are 8 bytes apart. */
extern u32 gDrawListText0a[1];  /* 0x6A9AD0 */
extern u32 gDrawListText6[1];   /* 0x6A9AD8 */
extern u32 gDrawListSky[1];     /* 0x6A9AE0 */
extern u32 gDrawListText5[1];   /* 0x6A9AE8 */
extern u32 gDrawListText4[1];   /* 0x6A9AF0 */
extern u32 gDrawListText2[1];   /* 0x6A9AF8 */
extern u32 gDrawListText1[1];   /* 0x6A9B00 */
extern u32 gDrawListText0b[1];  /* 0x6A9B08 */
extern u32 gDrawListStage[1];   /* 0x6A9B10 */
extern u32 gDrawOt8[8];         /* 0x6A9B20 */
extern u32 gDrawOtText3[16];    /* 0x6A9B40 */
extern u32 gDrawOt32[32];       /* 0x6A9B80: OT_AddByDepth(gDrawOt32, slot, 0x20, ..) */
extern u32 gDrawOtSorted[64];   /* 0x6A9C00: OT_AddByDepth(gDrawOtSorted, slot, 0x40, ..) */

/* A texture coordinate pair (texels). */
typedef struct UV {
    s16 u;
    s16 v;
} UV;

/* A textured rectangle (TexRect_DrawRotated, TexRect_DrawRotatedAt: sub_main unk_363550.c; 0x14 bytes). */
typedef struct TexRect {
    s16 x;                      /* 0x00 */
    s16 y;                      /* 0x02 */
    s16 w;                      /* 0x04 */
    s16 h;                      /* 0x06 */
    s32 color;                  /* 0x08 */
    UV uv0;                     /* 0x0C */
    UV uv1;                     /* 0x10 */
} TexRect;

typedef char TexRect_size_check[(sizeof(TexRect) == 0x14) ? 1 : -1];

/* An icon to draw (Draw_Icons, 8 bytes). */
typedef struct Icon {
    s16 x;
    s16 y;
    s16 size;
    u8 kind;                    /* index into gIconUVs */
    u8 alpha;
} Icon;

#endif /* SPRITE_H */
