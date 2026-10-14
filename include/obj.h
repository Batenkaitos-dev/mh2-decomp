#ifndef OBJ_H
#define OBJ_H

#include "common.h"

/*
 * An object of sub_main's first object pool (0x80 objects of 0x40 bytes at
 * 0x5B4FB0; Obj_Alloc, Obj_Free), which the game overlay uses for its
 * short-lived objects: an update function run every frame steps the state
 * in +0x4. The struct is generated from include/obj.spec
 * (tools/gen_struct.py): add fields there.
 */
struct Obj;
typedef void (*ObjFn)(struct Obj *o);
typedef struct Obj {
    u8 active;                  /* 0x000: nonzero while the object is in use */
    u8 visible;                 /* 0x001 */
    u8 kind;                    /* 0x002: set by the creator */
    u8 type;                    /* 0x003: the variant within the kind (the creator's argument; the updates switch on it) */
    u8 state;                   /* 0x004: 0 set up, 1 run, 2 end, 3 free (the update functions switch on it) */
    u8 var0;                    /* 0x005: per-kind scratch byte (counters, flags; += var1 in some) */
    u8 var1;                    /* 0x006: per-kind scratch byte */
    u8 var2;                    /* 0x007: per-kind scratch byte (a random lifetime: Rand_Next % 10 + 55 ...) */
    s16 timer;                  /* 0x008: a frame counter in many kinds (--timer <= 0; & 0x1FF / 512.0f) */
    s16 var3;                   /* 0x00A: per-kind s16: a second counter (to unk01E[0]) or an angle (/ 65536.0f) */
    struct Obj *prev;           /* 0x00C */
    struct Obj *next;           /* 0x010 */
    ObjFn draw;                 /* 0x014: drawn every frame while it is visible (Obj_DrawAll); NULL: none */
    void *buf;                  /* 0x018: its blocks of the object buffer (the kind's data: a struct of its own per kind) */
    u8 blk;                     /* 0x01C */
    u8 nblk;                    /* 0x01D */
    u8 var4;                    /* 0x01E: per-kind scratch byte: a player index, a position-table index (0xFF none), a random lifetime */
    u8 keep;                    /* 0x01F: not freed by Obj_FreeUnkept */
    f32 pos[3];                 /* 0x020 */
    ObjFn update;               /* 0x02C: run every frame (Obj_UpdateAll) */
    f32 fvar;                   /* 0x030: per-kind f32: a scale, an alpha, a speed or a velocity, a spacing */
    void *arg;                  /* 0x034: per-kind pointer: the creator's argument (the thrown item's player; cleared by K14) */
    struct DrawSlot *ds;        /* 0x038: its draw slot, DrawSlot_Get(slot) (a DrawSlot; OT_AddByDepth adds it) */
    s16 slot;                   /* 0x03C: its draw slot (DrawSlot_Alloc; -1: none was free), in the kinds that draw through one */
    s16 var5;                   /* 0x03E: per-kind s16: a frame counter (K13 to 1200, the season banner), the notice banner's message */
} Obj;
typedef char Obj_size_check[(sizeof(Obj) == 0x40) ? 1 : -1];

#endif /* OBJ_H */
