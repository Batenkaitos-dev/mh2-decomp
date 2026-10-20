#ifndef SPRITE_H
#define SPRITE_H

#include "common.h"

/*
 * A textured rectangle as Sprite_Draw (main) draws it, 0x14 bytes; also
 * the lobby browser's Prim_Sprite. The texture rectangle
 * is two corners (u0, v0)-(u1, v1); the screen one is a corner and a size.
 */
typedef struct Sprite {
    s16 x, y, w, h;
    u32 color;                  /* 0x08: 0xAABBGGRR */
    s16 u0, v0, u1, v1;         /* 0x0C */
} Sprite;

/*
 * A draw slot of sub_main's pools (0x20 bytes): gDrawSlotPool, gDrawSlotsB and
 * gHudDrawHooks (their lengths below). DrawSlot_Alloc
 * takes a free one (it sets draw to 1 until the owner sets its callback) and
 * returns its index or -1, DrawSlot_Get gives its address and DrawSlot_Free
 * clears it (DrawSlotB_* for the second pool; DrawSlotB_Free keeps owner and
 * part). Its owner copies its position into pos and puts it into an ordering
 * table every frame (OT_AddByDepth(gDrawOtSorted, slot, DRAW_OT_SORTED_LEN, 0)), whose walk
 * calls draw with the slot.
 */
#define DRAW_SLOT_POOL_LEN 0x200    /* gDrawSlotPool's slots */
#define DRAW_SLOTS_B_LEN 0x100      /* gDrawSlotsB's slots */
#define HUD_DRAW_HOOKS_LEN 4        /* gHudDrawHooks' slots */

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

/* The ordering tables the frame walks: each entry heads a list of
 * draw slots (DrawSlot.next) that OT_AddByDepth (by depth) and OT_AddAt (at an index) add to.
 * FreeList_Init sets them up and Task_RunList walks each in the frame's draw order
 *; the one-entry lists are 8 bytes apart. */
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
extern u32 gDrawOt32[32];       /* 0x6A9B80: OT_AddByDepth(gDrawOt32, slot, DRAW_OT32_LEN, ..) */
extern u32 gDrawOtSorted[64];   /* 0x6A9C00: OT_AddByDepth(gDrawOtSorted, slot, DRAW_OT_SORTED_LEN, ..) */

/* The entries of the depth-sorted ordering tables: OT_AddByDepth's length argument, which
 * spreads the view depths over that many lists. */
#define DRAW_OT32_LEN 0x20          /* gDrawOt32 */
#define DRAW_OT_SORTED_LEN 0x40     /* gDrawOtSorted */

/* A texture coordinate pair (texels). */
typedef struct UV {
    s16 u;
    s16 v;
} UV;

/* An icon's texture rectangle in gIconUVs (indexed by Icon.kind): two
 * corners, as TexRect's uv0 and uv1 (Draw_Icons). */
typedef struct IconUV {
    UV uv0;
    UV uv1;
} IconUV;

/* A textured rectangle (TexRect_DrawRotated, TexRect_DrawRotatedAt; 0x14 bytes). */
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
