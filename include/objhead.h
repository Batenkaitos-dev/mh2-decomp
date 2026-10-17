#ifndef OBJHEAD_H
#define OBJHEAD_H

#include "common.h"
#include "motion.h"

/* The object header Player (player.h) and Em (em.h) share: the same fields
 * at the same offsets, up to 0x72C. sub_main's object helpers read only
 * this part, and both the hunters' code and the monsters' code call them:
 * Player_InCurrentArea, Obj_InCurrentArea, Obj_SameArea, Player_ClearVelocity,
 * Player_ClearAccel2, Player_Integrate, Player_IsState, Player_IsLocal,
 * Player_RequestState and others. The names are Player's; a comment gives
 * Em's name where it differs, or says that Em has no name there yet. Only
 * the fields both structs agree on, and the ones the shared helpers read,
 * are listed; the rest is padding. The object helpers and the lobby's
 * actor core (unk_7949D0.c) take `void *obj` and read it through
 * `ObjHead *o = obj;` (the round 25 ruling). Every offset here is checked
 * with offsetof against Player's field of the same name and Em's. */
typedef struct ObjHead {
    u8 active;                      /* 0x000 */
    u8 visible;                     /* 0x001 */
    u8 weaponType;                  /* 0x002: Em.type */
    u8 isGunner;                    /* 0x003: Em.subType */
    u8 unk004;                      /* 0x004: Em: unnamed; with step, stateVar0, stateVar1 cleared as one word (Actor_ClearStateWork) */
    u8 step;                        /* 0x005 */
    u8 stateVar0;                   /* 0x006 */
    u8 stateVar1;                   /* 0x007 */
    u32 timer;                      /* 0x008 */
    u16 index;                      /* 0x00C */
    u16 targetYaw;                  /* 0x00E */
    u8 isEm;                        /* 0x010 */
    u8 gender;                      /* 0x011 */
    u8 weaponOut;                   /* 0x012 */
    u8 pad013[0x014 - 0x013];
    u8 stateGroup;                  /* 0x014 */
    u8 state;                       /* 0x015 */
    u8 prevStateGroup;              /* 0x016 */
    u8 prevState;                   /* 0x017 */
    u8 hitKind;                     /* 0x018 */
    u8 hitPending;                  /* 0x019 */
    u8 pad01A[0x01E - 0x01A];
    u8 ownTask;                     /* 0x01E: as Em.ownTask and Player.ownTask: 0 when the game overlay runs the object, 1 when another runner does (the lobby NPCs) */
    u8 pad01F[0x078 - 0x01F];
    u16 attackFlags;                /* 0x078: ATKF_* bits (objattach.h), as ObjAttach.attackFlags (Player_TestFlags, Player_TestSetFlags) */
    u8 pad07A[0x0A0 - 0x07A];
    s32 rot[3];                     /* 0x0A0: Em.rotX..rotZ */
    f32 pos[3];                     /* 0x0AC: Em.posX..posZ */
    f32 scale[3];                   /* 0x0B8: Em.scaleX..scaleZ */
    u8 pad0C4[0x154 - 0x0C4];
    MotionLayer motion[4];          /* 0x154: Em.motion, Player.motion */
    u8 pad294[0x29C - 0x294];
    u16 motionId[4];                /* 0x29C */
    s16 motionFrame[4];             /* 0x2A4 */
    s16 motionBlend[4];             /* 0x2AC */
    u16 unk2B4;                     /* 0x2B4: Em: unnamed */
    u8 pad2B6[0x2B8 - 0x2B6];
    u8 unk2B8;                      /* 0x2B8 */
    u8 pad2B9[0x2BC - 0x2B9];
    u8 motionStarted[4];            /* 0x2BC: Em: unnamed */
    u16 motionLayerCount;           /* 0x2C0 */
    s16 hp;                         /* 0x2C2 */
    u8 pad2C4[0x390 - 0x2C4];
    u8 action;                      /* 0x390: Em.stance */
    u8 pad391[0x392 - 0x391];
    u8 unk392;                      /* 0x392: Em: unnamed */
    u8 pad393[0x398 - 0x393];
    u32 flags;                      /* 0x398 */
    u32 stateFlags;                 /* 0x39C: Em: unnamed */
    u16 unk3A0;                     /* 0x3A0: Em: unnamed */
    u16 stateRand;                  /* 0x3A2 */
    u32 motionTime;                 /* 0x3A4 */
    u8 pad3A8[0x3B4 - 0x3A8];
    void *target;                   /* 0x3B4 */
    f32 vel[3];                     /* 0x3B8: Em.velX..velZ */
    f32 accel[3];                   /* 0x3C4: Em.accelX..accelZ */
    void *handlers;      /* 0x3D0 */
    u8 hitDelay;                    /* 0x3D4: Player.hitDelay; Em: unnamed */
    u8 hitFrames;                   /* 0x3D5: Player.hitFrames; Em: unnamed */
    u8 pad3D6[0x3F0 - 0x3D6];
    u16 hitAngle;                   /* 0x3F0 */
    u8 pad3F2[0x3F4 - 0x3F2];
    u16 lastAttackKind;             /* 0x3F4 */
    u8 pad3F6[0x3F8 - 0x3F6];
    u8 unk3F8;                      /* 0x3F8: Em.lookAtTarget */
    u8 hitIds[16];                  /* 0x3F9 */
    u8 side;                        /* 0x409 */
    u8 hitStopAdd;                  /* 0x40A */
    u8 hitStop;                     /* 0x40B */
    u8 pad40C[0x40E - 0x40C];
    u16 noHitTimer;                 /* 0x40E */
    u16 noPushTimer;                /* 0x410 */
    u8 hitIdNext;                   /* 0x412 */
    u8 pad413[0x414 - 0x413];
    u8 hitPhase;                    /* 0x414: Player.hitPhase; Em: unk414 */
    u8 pad415[0x417 - 0x415];
    u8 unk417;                      /* 0x417 */
    u8 pad418[0x430 - 0x418];
    u16 unk430;                     /* 0x430: Em: unnamed */
    u8 pad432[0x434 - 0x432];
    u8 unk434;                      /* 0x434: Em: unnamed */
    u8 pad435[0x4BA - 0x435];
    u16 unk4BA;                     /* 0x4BA: Em: unnamed (EmWork) */
    u8 pad4BC[0x4C1 - 0x4BC];
    u8 unk4C1;                      /* 0x4C1: Em: unnamed (EmWork) */
    u8 pad4C2[0x4C4 - 0x4C2];
    u8 unk4C4;                      /* 0x4C4: Em: unnamed (EmWork) */
    u8 pad4C5[0x4E8 - 0x4C5];
    s16 modelCount;                 /* 0x4E8: model slots used (Model_Alloc) */
    s16 modelFirst;                 /* 0x4EA: the first model slot */
    struct Model *model;            /* 0x4EC: the skeleton: bones and their matrices (include/model.h) */
    u8 pad4F0[0x52C - 0x4F0];
    struct DrawSlot *drawSlot;      /* 0x52C */
    s16 drawSlotId;                 /* 0x530 */
    u8 painted;                     /* 0x532 */
    u8 carry;                       /* 0x533 */
    u8 ammoType;                    /* 0x534 */
    u8 savedAmmoType;               /* 0x535 */
    u8 ammoMenuOpen;                /* 0x536 */
    u8 pad537[0x53A - 0x537];
    s16 paintTimer;                 /* 0x53A */
    u8 pad53C[0x574 - 0x53C];
    f32 floorY;                     /* 0x574 */
    u8 pad578[0x588 - 0x578];
    u8 rideState;                   /* 0x588 */
    u8 pad589[0x596 - 0x589];
    u8 pushPriority;                /* 0x596 */
    u8 pad597[0x653 - 0x597];
    u8 stateRequest;                /* 0x653 */
    u8 pad654[0x66C - 0x654];
    u8 partAnimOn[4];               /* 0x66C: Em: unnamed */
    u16 partAnimMotion[4];          /* 0x670: Em: unnamed */
    u16 partAnimWeight[4];          /* 0x678: Em: unnamed */
    u16 area;                       /* 0x680 */
    u8 areaMoveState;               /* 0x682 */
    u8 pad683[0x684 - 0x683];
    u16 nextArea;                   /* 0x684 */
    u8 pad686[0x698 - 0x686];
    u32 wallHitDirs;                /* 0x698 */
    u8 pad69C[0x6A0 - 0x69C];
    f32 mapPos[3];                  /* 0x6A0 */
    u8 pad6AC[0x6B0 - 0x6AC];
    u8 aimMode;                     /* 0x6B0 */
    u8 pad6B1[0x6DE - 0x6B1];
    s16 hpMax;                      /* 0x6DE */
    u8 pad6E0[0x6E4 - 0x6E0];
    f32 alpha;                      /* 0x6E4 */
    u8 pad6E8[0x6EC - 0x6E8];
    void *contact;         /* 0x6EC */
    struct ObjAttach *contactAttach;/* 0x6F0 */
    u8 pad6F4[0x706 - 0x6F4];
    s16 poison;                     /* 0x706: Em.poisonBuildup */
    u8 pad708[0x70A - 0x708];
    s16 poisonTimer;                /* 0x70A */
    u8 pad70C[0x722 - 0x70C];
    u8 hideShadow;                  /* 0x722: Em.hideShadow (Player.hideShadow): nonzero: Effect01_Draw skips the shadow */
    u8 pad723[0x724 - 0x723];
    f32 attackFactor;               /* 0x724 */
    f32 defenseFactor;              /* 0x728: Em.damageFactor */
} ObjHead; /* 0x72C */

typedef char ObjHead_size_check[(sizeof(ObjHead) == 0x72C) ? 1 : -1];

#endif
