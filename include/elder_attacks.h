#ifndef ELDER_ATTACKS_H
#define ELDER_ATTACKS_H

#include "common.h"
#include "sprite.h"
#include "em.h"
#include "objattach.h"

/*
 * Four attack-object families, as kinds of ObjAttach: KushalaBreath (Kushala
 * Daora's breath, which leaves an Explosion by its size) and the body attacks
 * of Kushala Daora, Shen Gaoren and Yama Tsukami. Each family has its Create,
 * Update (by ObjAttach.state), Setup, Run, End and Free; the breath keeps its
 * particles in its object buffer (KushalaBreathBuf, KushalaBreathPart).
 */

/* KushalaBreath's object buffer (o->buf) and its particles (KushalaBreath_PartCounts[type] of them). */
typedef struct KushalaBreathPart {
    f32 pos[3];                 /* 0x00 */
    f32 scale[3];               /* 0x0C: Key_LerpVec2/3 of KushalaBreath_ScaleKeys, scaled by the buffer's size */
    f32 vel[3];                 /* 0x18 */
    DrawSlot *ds;               /* 0x24: its draw slot (part = its index) */
    f32 unk28;                  /* 0x28 */
    u16 rot[3];                 /* 0x2C: X, Y, Z (kind 2); Z spins (KushalaBreath_SpinPart) */
    s16 timer;                  /* 0x32: starts at or below 0; reset at 0, moves while positive */
    s16 slot;                   /* 0x34 */
    s8 kind;                    /* 0x36: 0, 1 (on the floor) or 2 */
    u8 active;                  /* 0x37 */
    u8 alpha;                   /* 0x38: 255 * Key_LerpFloat of KushalaBreath_AlphaKeys */
    u8 flags;                   /* 0x39: 4: spins forward (KushalaBreath_SpinPart), 2: the second model (kind 1) */
    u8 pad3A[2];
} KushalaBreathPart;

typedef struct KushalaBreathBuf {
    f32 vec[3];                 /* 0x00: Key_LerpVec3's value, scaled by KushalaBreath_SizeScale[size] */
    f32 height;                 /* 0x0C */
    u16 ang;                    /* 0x10 */
    s16 spin;                   /* 0x12: the bone at creation; then the Z spin (+1820 a frame) */
    u8 size;                    /* 0x14: index into KushalaBreath_SizeScale */
    u8 pad15[3];
    KushalaBreathPart part[1];           /* 0x18 */
} KushalaBreathBuf;

void KushalaBreath_Update(ObjAttach *o);
void KushalaBreath_Setup(ObjAttach *o);
void KushalaBreath_Run(ObjAttach *o);
void KushalaBreath_SpinPart(ObjAttach *o, KushalaBreathPart *part, u16 step);
void KushalaBreath_DrawPart(ObjAttach *o, KushalaBreathPart *particle);
void KushalaBreath_End(ObjAttach *o);
void KushalaBreath_Free(ObjAttach *o);
void KushalaBreath_Draw(DrawSlot *drawSlot);
void KushalaBreath_Impact(ObjAttach *o, s32 unused, f32 *pos);
void KushalaAttack_Create(Em *em, s32 kind);
void KushalaAttack_Update(ObjAttach *o);
void KushalaAttack_Setup(ObjAttach *o);
void KushalaAttack_Run(ObjAttach *o);
void KushalaAttack_End(ObjAttach *o);
void KushalaAttack_Free(ObjAttach *o);
void ShenGaorenAttack_Create(Em *em, s8 kind);
void ShenGaorenAttack_Update(ObjAttach *o);
void ShenGaorenAttack_Setup(ObjAttach *o);
void ShenGaorenAttack_Run(ObjAttach *o);
void ShenGaorenAttack_End(ObjAttach *o);
void ShenGaorenAttack_Free(ObjAttach *o);
void YamaTsukamiAttack_Create(Em *em, s8 kind);
void YamaTsukamiAttack_Update(ObjAttach *o);
void YamaTsukamiAttack_Setup(ObjAttach *o);
void YamaTsukamiAttack_Run(ObjAttach *o);
void YamaTsukamiAttack_End(ObjAttach *o);
void YamaTsukamiAttack_Free(ObjAttach *o);

#endif
