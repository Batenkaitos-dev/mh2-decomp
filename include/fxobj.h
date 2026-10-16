#ifndef FXOBJ_H
#define FXOBJ_H

#include "common.h"
#include "sprite.h"
#include "em.h"
#include "player.h"

/*
 * sub_main's effect objects for the fishing, the lure, the countdown and the sparks (sub_main
 * unk_3E0EC0.c, unk_3E3D00.c): views of a stage object (StageObj, include/stageobj.h; 0x40
 * bytes) with these kinds' own names (the per-kind views of CONTRIBUTING, "Fakematches").
 * The update at +0x20 takes the view.
 */

/* A spark of an effect (0x34 bytes; the emitter's `sparks` array). */
typedef struct Spark {
    s16 id;                     /* 0x00: DrawSlot_Alloc, -1 none */
    s16 unk2;
    f32 unk4[3];                /* 0x04: from the key track gSparkFxKeys[k] (Key_LerpVec3) */
    f32 vel[3];                 /* 0x10: the offset from the emitter */
    u32 color;                  /* 0x1C: when unk2E is set */
    u16 unk20;                  /* 0x20: an angle, turned by unk22 every frame */
    u16 unk22;
    f32 unk24;                  /* 0x24: the emitter's scale */
    struct DrawSlot *obj;       /* 0x28: DrawSlot_Get(id) */
    s16 unk2C;                  /* 0x2C: its frame; negative: not started yet */
    u16 unk2E;                  /* 0x2E: nonzero: no second track */
    f32 unk30;                  /* 0x30: from the key track gSparkFxKeys[k + 1] (Key_LerpFloat) */
} Spark;
typedef char Spark_size_check[(sizeof(Spark) == 0x34) ? 1 : -1];

/* A spark emitter (effect kind 8: Effect08_Spawn, Effect08_SpawnAtEm; update SparkFx_Update). */
typedef struct SparkEmitter {
    u8 active;                  /* 0x00 */
    u8 visible;                 /* 0x01: cleared when its life is over (SparkFx_Run) */
    u8 kind;                    /* 0x02: 8 */
    u8 type;                    /* 0x03: 0-7, row of gSparkFxCounts */
    u8 state;                   /* 0x04: 0 start, 1 run, 2 end, 3 free (SparkFx_Update) */
    u8 pad5[2];
    u8 tint;                    /* 0x07: 0 white, 1 orange, else beige (Effect08_Spawn) */
    s16 timer;                  /* 0x08: frames run (SparkFx_Run ends it past the type's life) */
    u8 padA[0xA];
    s32 unk14;
    Spark *sparks;              /* 0x18 */
    u8 blk;                     /* 0x1C */
    u8 nblk;                    /* 0x1D */
    u8 ownerIndex;              /* 0x1E */
    u8 keep;                    /* 0x1F */
    void (*update)(struct SparkEmitter *); /* 0x20: SparkFx_Update */
    f32 pos[3];                 /* 0x24 */
    f32 scale;                  /* 0x30 */
    Em *owner;                  /* 0x34: the monster it follows (type 6) */
    u8 pad38[8];
} SparkEmitter;
typedef char SparkEmitter_size_check[(sizeof(SparkEmitter) == 0x40) ? 1 : -1];

/* An effect object of the fishing code (kinds 0x16 lure, 0x17 fish, 0x2C countdown; StageObj_Alloc,
 * StageObj_AllocTail): update at +0x20, a draw slot at +0x38. */
typedef struct FxObj {
    u8 active;                  /* 0x00 */
    u8 visible;                 /* 0x01 */
    u8 kind;                    /* 0x02 */
    u8 type;                    /* 0x03: the variant (the lure's updates switch on it; Fish_FindLure skips type 1) */
    u8 state;                   /* 0x04: 0 init, 1 run, 2 end, 3 free */
    u8 step;                    /* 0x05 */
    u8 phase;                   /* 0x06: within the step */
    u8 claimed;                 /* 0x07: the lure: 1 while a fish goes for it (Fish_FindLure), 0 when it lets go */
    s16 timer;                  /* 0x08 */
    s16 angle;                  /* 0x0A: the lure's heading (16-bit turn, the player's rot[1]); the countdown's time (Countdown_Draw) */
    u8 pad0C[0x14 - 0x0C];
    s32 unk14;                  /* 0x14 */
    void *work;                 /* 0x18: FishWork, LureWork */
    u8 blk;                     /* 0x1C */
    u8 nblk;                    /* 0x1D */
    u8 ownerIndex;              /* 0x1E: the lure's player (Player.index & 3: gLureColors); 0x80: set while a bite pulls it under (Lure_Update) */
    u8 keep;                    /* 0x1F */
    void (*update)(struct FxObj *);         /* 0x20 */
    f32 pos[3];                 /* 0x24 */
    f32 scale;                  /* 0x30 */
    Player *owner;              /* 0x34: the lure's player */
    DrawSlot *slot;             /* 0x38 */
    s16 slotId;                 /* 0x3C */
} FxObj;
typedef char FxObj_size_check[(sizeof(FxObj) == 0x40) ? 1 : -1];

/* The work of a fish (effect kind 0x17, Effect17_Spawn): it swims around home within radius,
 * bites the lure (kind 0x16, Lure_Spawn) and waits. */
typedef struct FishWork {
    f32 home[3];                /* 0x00 */
    f32 scale[3];               /* 0x0C: grows to its kind's in step 0 */
    f32 radius;                 /* 0x18 */
    f32 dist;                   /* 0x1C: to the lure (Fish_FindLure) */
    f32 accel;                  /* 0x20: added to the speed (object +0x30) while it bites */
    u8 pad24[2];
    s16 nibbles;                /* 0x26: nibbles left before the bite */
    FxObj *lure;                /* 0x28 */
    u8 *table;                  /* 0x2C: {weight, kind} pairs, weight 0xFF ends */
    s32 heading;                /* 0x30 */
    s32 turn;                   /* 0x34 */
    s32 unk38;                  /* 0x38 */
    s32 angles[3];              /* 0x3C */
    s8 kind;                    /* 0x48 */
    s8 unk49;                   /* 0x49: bright (1) or dim */
} FishWork;
typedef char FishWork_size_check[(sizeof(FishWork) == 0x4C) ? 1 : -1];

/* The work of a lure (effect kind 0x16, Lure_Spawn). */
typedef struct LureWork {
    s16 mode;                   /* 0x00 */
    s16 unk02;                  /* 0x02 */
    f32 vel[3];                 /* 0x04 */
    f32 target[3];              /* 0x10 */
    f32 unk1C;                  /* 0x1C */
    f32 gravity;                /* 0x20 */
    f32 scale;                  /* 0x24 */
    u8 pad28[2];
    u8 line;                    /* 0x2A: the line twitches (Fish_StepState_Unk) */
    u8 frame;                   /* 0x2B: its texture frame */
} LureWork;
typedef char LureWork_size_check[(sizeof(LureWork) == 0x2C) ? 1 : -1];

/* A fish kind's record (20 bytes, Fish_GetKind). */
typedef struct FishKind {
    u16 item;                   /* 0x0: to Player.gotItemId when caught */
    s8 unk2;                    /* 0x2: base of Player.fishTimer */
    s8 nibbles;                 /* 0x3: at most this many nibbles */
    u16 model;                  /* 0x4: entry of model set 0 */
    u16 color;                  /* 0x6: row of gFishColors */
    f32 scale[3];               /* 0x8 */
} FishKind;
typedef char FishKind_size_check[(sizeof(FishKind) == 0x14) ? 1 : -1];

#endif
