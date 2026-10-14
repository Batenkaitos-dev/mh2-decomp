#ifndef STAGEOBJ_H
#define STAGEOBJ_H

#include "common.h"
#include "model.h"

/*
 * An object of sub_main's third object list (StageObj_Alloc(nblk) allocates
 * one; StageObj_UpdateAll runs its update, +0x20, every frame), which the game
 * overlay uses for the objects that live in the stage (src/game/unk_868110.c
 * .. unk_88E760.c). Its header up to 0x1F is laid out like Obj's
 * (include/obj.h). The struct is generated from include/stageobj.spec
 * (tools/gen_struct.py): add fields there.
 */
struct StageObj;
/* An object update callback (StageObj_UpdateAll runs it every frame with the object). */
typedef void (*StageObjFn)(struct StageObj *o);

typedef struct StageObj {
    u8 active;                  /* 0x000: nonzero while the object is in use */
    u8 visible;                 /* 0x001 */
    u8 kind;                    /* 0x002: set by the creator (0xC, 0x11, 0x1B...) */
    u8 type;                    /* 0x003: the variant within the kind (the updates switch on it) */
    u8 state;                   /* 0x004: the update's state */
    u8 var0;                    /* 0x005: per-kind scratch byte (counters, flags) */
    u8 var1;                    /* 0x006: per-kind scratch byte (a bone index for Em_GetBonePos, an angle << 8 ...) */
    u8 var2;                    /* 0x007: per-kind scratch byte (0xFF: none in many kinds; an angle << 8 ...) */
    s16 timer;                  /* 0x008: a frame counter */
    s16 angle;                  /* 0x00A: a 16-bit angle in most kinds (Vec_RotateY, Mtx_RotateY, StageObj_K1C_CreateAtYaw; yaw from Vec_DirToAngles); a bone index in some */
    struct StageObj *prev;      /* 0x00C */
    struct StageObj *next;      /* 0x010 */
    void *draw;                 /* 0x014 */
    u8 *buf;                    /* 0x018: its blocks of the object buffer (the kind's data) */
    u8 blk;                     /* 0x01C */
    u8 nblk;                    /* 0x01D: the block count given to StageObj_Alloc */
    u8 ownerIndex;              /* 0x01E: the owner's index (Player.index, Em.index; compared with gScene.localPlayer) */
    u8 keep;                    /* 0x01F: not freed by StageObj_FreeUnkept */
    StageObjFn update;          /* 0x020: run every frame (StageObj_UpdateAll) */
    f32 pos[3];                 /* 0x024: the position */
    f32 scale;                  /* 0x030: the size the creator gives (1.0f by default) */
    void *owner;                /* 0x034: the monster, player or object it follows (NULL: none), in some kinds */
    struct DrawSlot *node;      /* 0x038: its draw slot, DrawSlot_Get(slot), when the kind draws one (DrawSlot: include/sprite.h) */
    s16 slot;                   /* 0x03C: its draw slot (DrawSlot_Alloc; -1: none was free), freed by the end state (DrawSlot_Free) */
    u16 angle2;                 /* 0x03E: a second 16-bit angle (pitch from Vec_DirToAngles) in some kinds; an area in others */
} StageObj;
typedef char StageObj_size_check[(sizeof(StageObj) == 0x40) ? 1 : -1];

/* sub_main: allocates an object with nblk blocks of the object buffer (+0x18), NULL when none is free */
StageObj *StageObj_Alloc(s32 nblk);
/* sub_main: frees an object (its blocks too) */
void StageObj_Free(StageObj *o);

/* a part of a kind 0x1B object (0x40 bytes each, in its blocks; src/game/unk_874B90.c, unk_880D50.c) */
typedef struct Obj1BPart {
    u16 rot[3];                 /* 0x00 */
    s16 t;                      /* 0x06: its frame */
    f32 fall;                   /* 0x08: its fall speed */
    f32 scale[3];               /* 0x0C: from a curve by its frame */
    f32 pos[3];                 /* 0x18 */
    f32 vel[3];                 /* 0x24 */
    u8 model;                   /* 0x30: its model of the common set */
    u8 alpha;                   /* 0x31 */
    s16 life;                   /* 0x32: its frames */
    f32 ground;                 /* 0x34: type 0: a floor height */
    struct DrawSlot *node;      /* 0x38 */
    s16 slot;                   /* 0x3C */
    u8 unk3E[0x40 - 0x3E];
} Obj1BPart;

#endif /* STAGEOBJ_H */
