#ifndef MONSTER_EFFECTS_H
#define MONSTER_EFFECTS_H

#include "common.h"
#include "sprite.h"
#include "em.h"
#include "stageobj.h"

/*
 * Two kinds of stage objects: MonsterFx, the visual effects of the monsters'
 * moves and attacks, and AreaFlyer, a small thing flying around a point of
 * some areas. A MonsterFx chain (types 0 and 11) keeps its links
 * (MonsterFxLink) trailing behind a bone; the parts of the other types
 * (MonsterFxPiece) play the steps of their type's tables. Here are the kinds'
 * creators, state handlers and draws.
 */

/* a link of a MonsterFx chain (types 0 and 11): 100 bytes each, in the object's blocks */
typedef struct MonsterFxLink {
    s16 slot;                   /* 0x00: its draw slot */
    s16 kind;                   /* 0x02: 0 the first part, 1 the second, others follow */
    f32 pos[3];                 /* 0x04 */
    s16 frame;                  /* 0x10: 8 - its index; a frame (texture scroll) */
    s16 releaseFrame;                  /* 0x12: -1 - its index */
    u16 tiltX;                  /* 0x14: extra angles of the other parts (type 11) */
    u16 tiltY;                  /* 0x16 */
    u16 rotZ;                  /* 0x18: its z angle (random at set-up and at release: MonsterFx_RandomSpin) */
    s16 spinZ;                  /* 0x1A: its speed: 182 for the first part; -256..255 */
    f32 scale;                  /* 0x1C: the object's, + 0.2 per part */
    DrawSlot *node;                /* 0x20 */
    f32 sc[3];                  /* 0x24: a scale from a curve (1.0 at first) */
    s32 color;                  /* 0x30: from a curve, -1 at first */
    f32 base[2][3];             /* 0x34: the last two base points */
    f32 dir[2][3];              /* 0x4C: the last two directions */
} MonsterFxLink;

/* a part of a MonsterFx object of the other types (64 bytes each): it plays the steps of its
 * type's tables */
typedef struct MonsterFxPiece {
    s16 slot;                   /* 0x00: its draw slot */
    u8 idx;                     /* 0x02: the entry of the step table, 255: done */
    u8 sub;                     /* 0x03 */
    f32 pos[3];                 /* 0x04 */
    u32 color;                  /* 0x10 */
    f32 sc[3];                  /* 0x14: its scale from its scale curve (x, y and, unless flatScale, z) */
    f32 scale;                  /* 0x20 */
    DrawSlot *node;                /* 0x24 */
    s16 wait;                   /* 0x28: its frame in the current step (negative: frames still to wait) */
    u16 rotX;                  /* 0x2A: angles (x, y, z) */
    u16 rotY;                  /* 0x2C */
    u16 rotZ;                  /* 0x2E */
    s16 spinX;                  /* 0x30: their speeds */
    s16 spinY;                  /* 0x32 */
    s16 spinZ;                  /* 0x34 */
    u8 useColorKeys;                   /* 0x36 */
    u8 flatScale;                   /* 0x37 */
    f32 speed;                  /* 0x38 */
    f32 riseSpeed;                  /* 0x3C */
} MonsterFxPiece;

void MonsterFx_Setup(StageObj *obj);
void MonsterFx_SetupChain(StageObj *obj);
void MonsterFx_SetupSeq(StageObj *obj);
void MonsterFx_Run(StageObj *obj);
void MonsterFx_RunChain(StageObj *obj);
void MonsterFx_RunSeq(StageObj *obj);
void MonsterFx_End(StageObj *obj);
void MonsterFx_EndChain(StageObj *obj, s16 linkCount);
void MonsterFx_EndSeq(StageObj *obj, s16 partCount);
void MonsterFx_Free(StageObj *obj);
void MonsterFx_Draw(DrawSlot *node);
void MonsterFx_DrawChain(DrawSlot *node);
void MonsterFx_DrawSeq(DrawSlot *node);
void MonsterFx_FollowBone(StageObj *obj, MonsterFxLink *w, MonsterFxLink *prev, f32 *boneMtx, f32 *dir, s16 first);
void MonsterFx_RandomSpin(MonsterFxLink *w);
void MonsterFx_StartKushalaRingStep(StageObj *obj, MonsterFxPiece *w);
s32 MonsterFx_AllPartsDone(StageObj *obj);
void MonsterFx_RestartKushalaRing(StageObj *obj);
void MonsterFx_NextDustBurstStep(StageObj *obj, MonsterFxPiece *w);
void MonsterFx_StartDustBurstStep(StageObj *obj, MonsterFxPiece *w);
f32 MonsterFx_GrowFactor(StageObj *obj);
void MonsterFx_NextDustKickStep(StageObj *obj, MonsterFxPiece *w);
void MonsterFx_StartDustKickStep(StageObj *obj, MonsterFxPiece *w);
void MonsterFx_PlaceBehindOwner(StageObj *obj);
void MonsterFx_StartTescaAuraStep(StageObj *obj, MonsterFxPiece *w);
void MonsterFx_RestartTescaAuraBursts(StageObj *obj);
s32 MonsterFx_TescaAuraBurstsDone(StageObj *obj);
void MonsterFx_GetOwnerBone(StageObj *obj, f32 *out);
s32 MonsterFx_OwnerActive(Em *em);
void MonsterFx_Jitter(MonsterFxPiece *w, f32 radius);
void StageObj_RandomDistinct(s16 range, s16 count, s16 *out);
StageObj *MonsterFx_Alloc(u8 fxType);
StageObj *MonsterFx_CreateOnEm(Em *em, u8 fxType, f32 scale);
void MonsterFx_CreateOnEmVariant(Em *em, u8 fxType, u8 variant, f32 scale);
void MonsterFx_CreateAt(f32 *point, u8 fxType, f32 size);
void MonsterFx_CreateMoving(f32 *pos, f32 *dir, u8 fxType, f32 scale);
StageObj *MonsterFx_CreateAtYaw(f32 *pos, u16 yaw, u8 fxType, f32 scale);
void MonsterFx_CreateAtAngled(f32 *pos, u16 pitch, u16 yaw, u8 fxType, f32 scale);
void MonsterFx_CreateOnEmAngled(Em *em, u8 fxType, u16 pitch, s16 yawOffset, f32 scale);
void AreaFlyer_Setup(StageObj *obj);
void AreaFlyer_Run(StageObj *obj);
void AreaFlyer_End(StageObj *obj);
void AreaFlyer_Free(StageObj *obj);
void AreaFlyer_Draw(DrawSlot *node);
void AreaFlyer_GetCenter(f32 *out, u8 idx);
void AreaFlyer_Create(u8 point);

#endif
