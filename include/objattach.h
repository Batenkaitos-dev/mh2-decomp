#ifndef OBJATTACH_H
#define OBJATTACH_H

#include "common.h"

/*
 * An object of sub_main's second object pool (0x40 objects of 0xD4 bytes at
 * 0x5B70D0; ObjAttach_Alloc, ObjAttach_Free), attached to a player or a
 * monster: the game overlay's effects and the like. The struct is generated
 * from include/objattach.spec (tools/gen_struct.py): add fields there.
 */

struct ObjAttach;
/* An object's per-frame update and draw functions (ObjAttach_UpdateAll, ObjAttach_DrawAll). */
typedef void (*ObjAttachFn)(struct ObjAttach *o);
typedef struct ObjAttach {
    u8 active;                  /* 0x000: nonzero while the object is in use */
    u8 visible;                 /* 0x001 */
    u8 kind;                    /* 0x002: set by the creator (0x21: EffEm33_Init) */
    u8 type;                    /* 0x003: the variant within the kind (the creator's argument; the updates switch on it) */
    u8 state;                   /* 0x004: 0 set up, 1 run, 2 end, 3 free (the update functions switch on it) */
    u8 step;                    /* 0x005: as Em.step: the step within the update (switch on it, counted up; 0 at set up) */
    u8 stateVar0;               /* 0x006: as Em.stateVar0: per-kind scratch byte (switch on it, counters, 0xFF none) */
    u8 stateVar1;               /* 0x007: as Em.stateVar1: a second per-kind scratch byte (0xFF/0xFE marks, counters) */
    u8 unk008;                  /* 0x008 */
    u8 keep;                    /* 0x009: not freed by ObjAttach_FreeUnkept */
    u8 idx;                     /* 0x00A: the index of the player or monster it follows (gEms, gPlayers) */
    u8 hitPhase;                /* 0x00B: the hit window: 0 off, 1 waiting hitDelay frames, 2 open for hitFrames (ObjAttach_UpdateTimers; Obj_StartMotionOwned restarts it); cleared when the owner motion changes (func_006C6D40, Eff01_Run, Eff04_Run end the object at 0) */
    struct ObjAttach *prev;     /* 0x00C */
    struct ObjAttach *next;     /* 0x010 */
    ObjAttachFn draw;           /* 0x014: drawn every frame while it, its owner and the object are visible (ObjAttach_DrawAll); NULL: none */
    u8 *buf;                    /* 0x018: its blocks of the object buffer */
    u8 blk;                     /* 0x01C */
    u8 nblk;                    /* 0x01D */
    u8 unk01E;                  /* 0x01E */
    u8 unk01F[0x020 - 0x01F];
    ObjAttachFn update;         /* 0x020: run every frame once delay is 0 (ObjAttach_UpdateAll) */
    s32 rotX;                   /* 0x024: its rotation (angles; the creators copy the owner's, Em +0xA0/+0xA4/+0xA8) */
    s32 rotY;                   /* 0x028 */
    s32 rotZ;                   /* 0x02C */
    f32 pos[3];                 /* 0x030: the position */
    f32 pos2[3];                /* 0x03C: the second point of the hit capsule: Capsule_FromPoints(pos, pos2, hitList, .., hitScale) (ObjAttach_HitObjects, ObjAttach_HitPlayer); Eff00_AttachTransform sets it from a bone matrix, Eff03_Setup from pos */
    f32 velX;                   /* 0x048: velocity x: Eff03/Eff06/BowgunShot add it to pos; Vec_RotateY(&velX) and Vec_DirToAngles use velX..velZ as a vector */
    f32 velY;                   /* 0x04C: velocity y (see velX) */
    f32 velZ;                   /* 0x050: velocity z (see velX) */
    f32 homingTurn;             /* 0x054: homing: the turn toward the target, grown each frame (unk_6D3B50.c); zeroed at creation by the other kinds */
    f32 gravity;                /* 0x058: added to velY each frame (Eff06, EffEm33: EffEm33_Val058ByType[type], -1.0 / -1.5) */
    f32 homingSpeed;            /* 0x05C: homing: added to the speed, grown each frame (unk_6D3B50.c); zeroed at creation by the other kinds */
    u8 hitDelay;                /* 0x060: frames before the hit window opens: the Setup functions compute it from the owner motion frames (Eff00_Setup: k - motionFrame / 2 - 1); ObjAttach_UpdateTimers counts it down in hitPhase 1 */
    u8 hitFrames;               /* 0x061: frames the hit window stays open (99: long; Eff04 0x3C): ObjAttach_UpdateTimers counts it in hitPhase 2 */
    u8 power;                   /* 0x062: the attack power: ObjAttach_DamageEm (dmgf = it), ObjAttach_DamagePlayer (it * atk); Eff06_CalcPower, BowgunShot_ApplyKind set it */
    u8 attackKind;              /* 0x063: the attack kind, copied to the target lastAttackKind (ObjAttach_DamageEm/DamagePlayer): 7 paints (Em.painted), 8 and 0x10 special cases, 0xA hits only state-3 players (ObjAttach_HitPlayer); ObjAttach_HitObjects/HitAll switch on it */
    u8 unk064[0x065 - 0x064];
    u8 attrs;                   /* 0x065: attribute bits ORed into the target hitAttrs (ObjAttach_DamageEm, ObjAttach_DamagePlayer): 0x80 cannot be guarded; 1 tested by ObjAttach_HitAll, 0x10 by ObjAttach_DamagePlayer */
    s8 hitYawOfs;               /* 0x066: degrees added to the object's yaw for the target's hitAngle, unless attrs 0x10 (ObjAttach_SetHitHeading) */
    u8 unk067[0x068 - 0x067];
    u8 hitForce;                /* 0x068: copied to the target's hitForce (ObjAttach_DamagePlayer, ObjAttach_DamageEm) */
    u8 hitMarks;                /* 0x069: mark bits ORed into the target's partFlags: 1 a critical hit (with flag 0x40), 2, 8; on a player 4 fire, 0x10 water, 0x40 thunder, 0x20 ice, 0x80 dragon pick the resistance (ObjAttach_DamagePlayer) */
    u8 hitListNo;               /* 0x06A: the index into the kind tables hitList is taken from (hitList = EffBlangonga27_Vals088[it] and the like, Setup functions) */
    u8 hitSe;                   /* 0x06B: the hit sound kind: 0xFF none; on a player 1 SE 0x20, others 0x21 (ObjAttach_DamagePlayer); on a monster by kind (ObjAttach_DamageEm) */
    u16 elementFlags;           /* 0x06C: the element bits: 0x10 fire, 0x20 water, 0x40 thunder, 0x80 ice, 0x200 dragon take elementPower (ObjAttach_DamageEm); the status bits 1 status 2, 2 poison, 4 status 4, 8 status 1, 0x100 mash buildup add elementPower to the target's hit amounts, 0x400, 0x1000 and 0x800 set status 6, 7 and 8 (Em_ApplyHitStatus); ObjAttach_HitEffects picks the hit effect from them; BowgunShot_SetKind sets them */
    u8 elementPower;            /* 0x06E: the element value for elementFlags (ObjAttach_DamageEm: fire = it, ...) */
    u8 hitFx;                   /* 0x06F: low 6 bits: the hit effect kind (StageObj_K10_CreateOnEmAt, Effect02_Spawn in ObjAttach_HitEffects); 0x80 a critical hit, 0x40 a critical miss (the debug prints of ObjAttach_DamageEm) */
    u8 hitStop;                 /* 0x070: the hit stop: the hitter hitStopAdd (times 3 in some cases) and hitSlowTimer, the target hitStopAdd (ObjAttach_DamageEm, ObjAttach_DamagePlayer) */
    u8 swordGaugeAdd;           /* 0x071: long sword: the spirit gauge gained by the hit, times gSwordGaugeHitFactors[hitKind] (LongSword_AddGauge in ObjAttach_DamageEm) */
    u8 multiHit;                /* 0x072: low 4 bits: the hits left; high 4 bits: the delay before the next (ObjAttach_StepHitTimer_Unk reopens the hit window with a new hitId) */
    u8 unk073[0x074 - 0x073];
    u8 stun;                    /* 0x074: the stun amount: the target status5Hit (ObjAttach_DamageEm), stun (ObjAttach_DamagePlayer) */
    u8 unk075[0x078 - 0x075];
    u16 unk078;                 /* 0x078: cleared by ObjAttach_Free */
    u8 isEm;                    /* 0x07A: 0: follows a player, else a monster */
    u8 delay;                   /* 0x07B: frames before update runs */
    s16 ownerMotion;            /* 0x07C: the owner's motionId[0] when the object was made (func_006C6D40); the hit window closes when the owner's motion differs; some kinds count frames in it (EffEm08: the creator's bone, moved to its buffer at set up) */
    s16 aux;                    /* 0x07E: per-kind value: a bone of the owner (Em_GetBone, f06_BoneLaunchPos), a monster kind (EmSlot_FindKind), a frame counter */
    s16 unk080;                 /* 0x080 */
    u8 unk082[0x088 - 0x082];
    struct HitShape *hitList;   /* 0x088: the attack hit shapes (HitShape, include/capsule.h), a list ended by bone -1, read with pos/pos2 (ObjAttach_HitObjects, ObjAttach_HitPlayer: no hit while NULL); Eff10_Vals08C[type] and the kind tables */
    struct HitShape *hurtList;  /* 0x08C: the shapes other objects hit (HitShape, include/capsule.h) (ObjAttach_HitObjects: elist = e->hurtList, skipped while NULL) */
    u8 unk090[0x094 - 0x090];
    void *owner;                /* 0x094: the player or monster it belongs to (EffEm33_Init) */
    u8 unk098[0x09C - 0x098];
    struct Em *targetEm;        /* 0x09C: the monster it locked on or stuck to (found in range in unk_6D3B50.c), NULL: none */
    struct HitShape *targetPart;/* 0x0A0: the hit shape of targetEm it hit (ObjAttach_DamageEm): its bone and position */
    u8 unk0A4[0x0B4 - 0x0A4];
    u8 wasHit;                  /* 0x0B4: 1 once another object hit it (ObjAttach_HitObjects sets it and skips it after); Eff10_Run tests it */
    u8 hitId;                   /* 0x0B5: the attack id: an object is not hit twice by one id (compared with hitIds in ObjAttach_HitAll; stored there by the damage functions) */
    u8 unk0B6[0x0B8 - 0x0B6];
    u16 randBits;               /* 0x0B8: random bits from the owner stateRand (Em/Player), read by shifts (Eff06, BowgunShot, EffKushala39) */
    u8 unk0BA[0x0BC - 0x0BA];
    struct DrawSlot *drawSlotBuf;/* 0x0BC: the draw slot of drawSlot (DrawSlot_Get/DrawSlotB_Get(drawSlot)), NULL: none; drawn at its pos (EffEm07_Run) */
    s16 drawSlot;               /* 0x0C0: the draw slot, -1: none (DrawSlot_Alloc/DrawSlotB_Alloc in the Setup functions) */
    u8 unk0C2[0x0C4 - 0x0C2];
    void *hitBy;                /* 0x0C4: set when another object hit it: gPlayers or gEms by the hitter's isEm, at its own idx (the game's indexing), NULL when the hitter's unk008 is 0xFF (ObjAttach_HitObjects) */
    f32 hitScale;               /* 0x0C8: the hit capsule scale: Capsule_FromPoints(pos, pos2, hitList, .., it) (1.0 for the object hit) */
    u16 yaw;                    /* 0x0CC: its yaw (the owner's rotation Y, Em/Player +0xA4, when made; +0x8000 turns it round) */
    u16 area;                   /* 0x0CE: the area it is in (the owner's +0x680); compared with the current area, gScene.unk014 */
    u8 hitKind;                 /* 0x0D0: the hitter's hitKind at the hit (ObjAttach_DamageEm), 0 on a player (ObjAttach_DamagePlayer); ObjAttach_HitEffects picks the effect by it */
    u8 side;                    /* 0x0D1: the owner side (Em/Player side, Eff00_Create, Eff09_Create; copied to spawned objects): ObjAttach_DamagePlayer Side_Match(t->side, it) */
    u8 powerPct;                /* 0x0D2: nonzero: power is scaled by this percentage on a hit (ObjAttach damage, unk_3BE060.c); 0xFA from unk_6D29F0.c */
    u8 hitSpan;                 /* 0x0D3: Eff05 type 0x47 (Lao-Shan's): twice its hitFrames (Eff05_CreateEx's last argument; Eff05_Setup halves it); 0 from the other Eff05 creators */
} ObjAttach;
typedef char ObjAttach_size_check[(sizeof(ObjAttach) == 0xD4) ? 1 : -1];

/*
 * The blocks of the object buffer (ObjAttach.buf) of kinds that more than one
 * file reads. A kind read in one file has its view in that file.
 */

/* A row of gArrowParams (game unk_6BB9D0.c, 33 rows): a bow arrow kind (Eff06, the type of the object). */
typedef struct ArrowParam {
    u8 kind;                    /* 0x00: how it flies (Eff06_Init and Eff06_Fly switch on it: 2 from a bone, scattering; 4 ...) */
    u8 model;                   /* 0x01: its mesh set of gCommonModels[0] (0x62: a spinning one; 0xFF: none) */
    s8 blend;                   /* 0x02: the row of Eff06_BlendStates */
    u8 life;                    /* 0x03: the row of Eff06_LifeLimit (frames), 0xFF: none */
    s16 powerTbl;               /* 0x04: the row of Eff06_PowerTbls (power by distance), -1: none */
    u16 unk06;                  /* 0x06 */
    f32 speed;                  /* 0x08: the launch speed, times the level row's (Eff06_Init) */
    f32 speedRand;              /* 0x0C: the random part of the speed */
    f32 gravity;                /* 0x10: ObjAttach.gravity, negated */
    f32 gravityRand;            /* 0x14: the random part of the gravity */
    f32 wobbleY;                /* 0x18: kinds 0 and 1: a random vertical push, times the horizontal speed (Eff06_Fly) */
    f32 wobbleX;                /* 0x1C: kinds 0 and 1: a random sideways push in the first 20 frames, times the level row's +0x10 */
    s16 boostFrames;            /* 0x20: frames it still accelerates (MoveObj_AddVelocity while ownerMotion is below it) */
    u8 hitKind;                 /* 0x22: Eff06_Fly and Eff06_Hit switch on it */
    u8 count;                   /* 0x23: the hits, or the row of Eff06_ScatterParams */
} ArrowParam;
typedef char ArrowParam_size_check[(sizeof(ArrowParam) == 0x24) ? 1 : -1];

/* The bow arrow's draw part (Eff06Buf.part; Eff06_PartAlloc). */
typedef struct Eff06Part {
    struct DrawSlot *slot;      /* 0x00: DrawSlot_Get(drawSlot), NULL: none */
    s16 drawSlot;               /* 0x04 */
    s16 unk06;                  /* 0x06 */
    s16 timer;                  /* 0x08: counted up by Eff06_PartRun */
    s16 unk0A;                  /* 0x0A */
} Eff06Part;

/* The bow arrow Eff06's block (Eff06_Create and Eff06_Init in game unk_6BB9D0.c; game unk_6C7AE0.c). */
typedef struct Eff06Buf {
    u8 bounced;                 /* 0x00: 1 once Eff06_BounceStep ran (Eff06_DrawModel then stops turning it along its velocity) */
    u8 stage;                   /* 0x01: the entry of Eff06_PowerTbls[powerTbl] reached (Eff06_UpdatePowerStage), 0xFF: past the last */
    u8 flyStep;                 /* 0x02: Eff06_Fly switches on it */
    u8 unk03;                   /* 0x03: the hit number (o->hitList = Eff06_Vals088[it]), also counted down by Eff06_Fly */
    f32 drawPos[3];             /* 0x04: copied to the draw slot's pos (Eff06_Run, Eff06_Hit) */
    f32 power;                  /* 0x10: the object's power at set up (Eff06_ApplyKind scales it) */
    s16 partTimer;              /* 0x14: Eff06_PartRun's frames */
    u8 phase;                   /* 0x16: Eff06_Fly, Eff06_Hit, Eff06_State5 and Eff06_DrawModel switch on it */
    u8 unk17;                   /* 0x17 */
    f32 attackFactor;           /* 0x18: the player's attackFactor */
    f32 scale;                  /* 0x1C: the charge scale (Eff06_Create's; Eff06_Scatter passes it on): the power is attackFactor times it */
    u8 *levelRow;               /* 0x20: the bow's level row of gGunLevelRows (24 bytes: f32 values at +4 to +0x14) */
    u8 level;                   /* 0x24: the bow's level (low 4 bits; 0x10 and 0x20 add the D_006AEE04 and D_0045EB04/24 speeds) */
    u8 skills;                  /* 0x25: Eff06_InitSkills: 1, 2, 4, 8 power skills; 0x10 range x2, 0x20 range x0.5 */
    u8 crit;                    /* 0x26: 1 a critical (Player_GetWeaponStat 8, by chance), 2 a weak hit */
    u8 unk27;                   /* 0x27: 0 when the bow's Equip_GetClass is 1, else 1 */
    Eff06Part part;             /* 0x28 */
} Eff06Buf;

/*
 * Kushala Daora's effect 39's block (EffKushala39_Setup and EffKushala39_Run in game
 * unk_6EB590.c, EffKushala39_Draw in game unk_6EC680.c). A piece spawns the next one
 * (EffKushala39_Spawn, up to 7) at its origin when its first phase ends.
 */
typedef struct EffKushala39Buf {
    u8 phase;                   /* 0x00: 0, 1, 2 (EffKushala39_Life[phase] frames each); EffKushala39_Draw's model and colour keys */
    u8 pad01[2];
    u8 seDone;                  /* 0x03: 1 once phase 0's end played its sound (Em_PlaySeChecked 0x24) */
    s16 rotZ;                   /* 0x04: the model's turn about z (random at set up and when phase 2 starts) */
    s16 spin;                   /* 0x06: rotZ's step per frame in phase 2 */
    f32 rise;                   /* 0x08: pos[1]'s step per frame in phase 2 */
    f32 scale;                  /* 0x0C: the model's x and y scale (1.0 to 1.2) */
    f32 origin[3];              /* 0x10: where it was made (the owner's bone 1 for the first piece); the next piece spawns there */
} EffKushala39Buf;
typedef char EffKushala39Buf_size_check[(sizeof(EffKushala39Buf) == 0x1C) ? 1 : -1];

#endif /* OBJATTACH_H */
