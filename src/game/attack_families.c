/*
 * The attack-object families: PlacedItem (a bomb or trap a player sets
 * down), PitfallTrap, EmBomb (the Melynx's bombs), StageShot (the stage's
 * guns and thrown rocks), BowArrow (the bow's arrows), Boomerang,
 * VespoidAttack and the monsters' melee families (<Monster>Attack).
 * Each family is a kind of ObjAttach (enum
 * ObjAttachKind) on sub_main's object list: its Create fills the object and
 * its Update, which ObjAttach_UpdateAll calls every frame, switches on
 * ObjAttach.state (enum ObjAttachState) to its Setup, Run, Hit, End and Free;
 * its type picks a row of its tables (the hit's power, kind, element and
 * shapes).
 */
#include "common.h"
#include "item.h"
#include "stageobj.h"
#include "em.h"
#include "objattach.h"
#include "em_blangonga.h"
#include "player.h"
#include "sprite.h"
#include "capsule.h"
#include "area.h"

/* prototypes */
void ShotFlash_CreateAtAngles(f32 *pos, s32 kind, u16 a, u16 b);
s32 Side_Match(u8 a, u8 b, u8 none);
void PartFx_SpawnAt(f32 *pos, s32 kind, f32 scale);
extern s16 StageShot_TimeLimit[];
extern f32 StageShot_CannonPtsFortress5[][3];
extern u16 StageShot_CannonAngsFortress5[];
extern f32 StageShot_CannonPtsCastle1[][3];
extern u16 StageShot_CannonAngsCastle1[];
extern f32 StageShot_CannonPtsBattleTown1[][3];
extern u16 StageShot_CannonAngsBattleTown1[];
void StageShot_PlaySound(ObjAttach *o, s16 n);
extern void *StageShot_RowsPtr;
extern HitShape *StageShot_HitShapes[];
extern f32 StageShot_StartVels[][3];
extern f32 StageShot_Gravities[];
extern f32 StageShot_RockSpotVels[];
extern f32 StageShot_CannonSpread[];
void StageShot_Draw(DrawSlot *d);
extern f32 StageShot_RockSpotsFortress4[][3];
extern u16 StageShot_RockAngsFortress4[];
extern f32 StageShot_RockSpotsFortress3[][3];
extern u16 StageShot_RockAngsFortress3[];
extern f32 StageShot_RockSpotsFortress2[][3];
extern u16 StageShot_RockAngsFortress2[];
f32 Math_Sin(f32 x);
void PitfallTrap_DrawModel(ModelMatSet *set, f32 *m);
void PitfallTrap_PlaySound(ObjAttach *o, s16 kind);
void DustCloud_CreateAtYaw(f32 *pos, s32 kind, u16 r, f32 scale);
/* unit's own declaration: the angle declared u16 and the kind s32 as the callers pass them; with the definition's s32 angle and s16 kind the calls compile differently */
void AttackBurst_CreateAtScaled(f32 *pos, u16 r, s32 kind, f32 scale);
f32 Em_TrapReach(u16 type, f32 dist);
void ElementBurst_Create(f32 *pos, s32 kind, u16 ang, void *owner, f32 scale);
void HitSpark_SpawnRandomAngle(u16 a, u16 b, u8 type, f32 *pos);
void Mtx_Identity(f32 *m);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpVec3(f32 *k, f32 *out, f32 t);
void Color_EvalKeys(s32 *k, s32 t, u32 *out);
void Mtx_FromScaleRotTrans(f32 *scale, f32 *rot, f32 *trans, u16 order, f32 *m);
s32 Effect_DrawModelAdd(void *list, f32 *v, void *base);
s32 Effect_DrawModel(void *mdl, f32 *a1, s32 color, u16 flags, void *a4);
extern u8 PlacedItem_BasePartScaleKeys[];
extern u8 PlacedItem_RaisedPartScaleKeys[];
extern u8 PlacedItem_SpinPartScaleKeys[];
extern u8 PlacedItem_BasePartColorKeys[];
extern u8 PlacedItem_RaisedPartColorKeys[];
extern s16 PlacedItem_PartModels[];
u16 Area_DayId(u16 id);
extern s16 *EmBlangonga_FallListsSnowy3[];
extern s16 *EmBlangonga_FallListsByZ[];
extern f32 EmBlangonga_FallPointsSnowy3[][3];
extern f32 EmBlangonga_FallPointsSnowy8[][3];
s32 EmMotion_ReachedFrame(void *obj, s32 a, f32 frame);
s32 EmMotion_InFrameRange(void *obj, s32 a, f32 from, f32 to);
s32 ObjAttach_RollBatBack(ObjAttach *o, u16 x);
void Vec_Diff(f32 *out, f32 *a, f32 *b);
u16 BowArrow_MulAddShift(u16 arg0, u16 arg1, s32 arg2, s32 arg3);
f32 BowArrow_BitsToSignedFrac(u16 value, u16 bits);
f32 Math_Sqrtf(f32 x);
void Vec_Add(f32 *, f32 *, f32 *);
void Mtx_Copy(f32 *dst, f32 *src);
void BowArrow_ApplyPower(ObjAttach *o);
f32 BowArrow_BitsToFrac(u16 arg0, s32 arg1);
void BowArrow_ApplyEvent(ObjAttach *o);
void BowArrow_Draw(DrawSlot *d);
void Vec_RotateSelf(f32 *v, f32 *m);
EmBone *Em_GetBone(void *obj, s16 bone);
void Mtx_GetTrans(f32 *out, f32 *m);
void Vec_RotateX(f32 *v, f32 angle);
extern void *BowArrow_RowsPtr;
extern HitShape *BowArrow_HitShapes[];
extern f32 BowArrow_ChargeFlight[][6];
extern f32 BowArrow_ChargePowerScale[];
extern f32 BowArrow_ChargeElementScale[];
extern f32 BowArrow_ChargeCoatingPower[];
extern u8 BowArrow_LevelScale[];
u8 PlacedItem_RunTrap(ObjAttach *o);
void PlayerFx_CreateAtDefault(f32 *src, u8 k, f32 scale);
void PlacedItem_FollowBone(ObjAttach *o);
void PlacedItem_Free(ObjAttach *o);
void PlacedItem_InitTrapParts(ObjAttach *o);
extern s16 PlacedItem_Motions[];
extern void *PlacedItem_RowsPtr;
extern HitShape *PlacedItem_Shapes[];
void PlacedItem_DrawMain(DrawSlot *d);
s32 Em_PlaySeChecked(Em *em, s32 id, s32 a, void *pos, s32 b, s32 c);
s32 Obj_InCurrentArea(void *obj);
void Em_GetBonePos(void *obj, s16 bone, f32 *out);
void Boomerang_Update(ObjAttach *o);
s32 EmMelynx_BombState(Em *em);
s32 EmShakalaka_BombState(Em *em);
f32 *Mtx_FromAnglesYXZ(s32 *angles, f32 *m);
void Vec_Set(f32 *out, f32 x, f32 y, f32 z);
f32 AreaGrid2_GetFloorY(f32 *pos);
void Explosion_CreateAt(f32 *pos, s32 kind, u16 area, u8 arg3);
void ItemProp_Create(f32 *pos, s16 v, s32 k, u8 c);
void Explosion_CreateOwned(Player *owner, f32 *pos, s32 kind);
extern s16 BowArrow_EventHitFx[];
extern f32 BowArrow_EventPowerScale[];
extern s16 BowArrow_EventHitSe[];
void Em_BonePos(void *obj, s16 bone, f32 *out);
u16 Rand_Next(s32 stream);
void PlacedItem_Draw(DrawSlot *d);
void PitfallTrap_Update(ObjAttach *o);
s32 Player_IsLocal(void *obj);
void Vec_RotateY(f32 *v, f32 angle);
void Boomerang_Draw(DrawSlot *d);
extern HitShape *Boomerang_HitShapes;
void PlacedItem_Run(ObjAttach *o);
void GraviosAttack_Update(ObjAttach *o);
void BlangongaAttack_Update(ObjAttach *o);
void StageShot_Update(ObjAttach *o);
void HermitaurAttack_Update(ObjAttach *o);
s32 StageShot_FindSpot(f32 *from, f32 *pos, u16 *ang);
s16 DrawSlotB_Alloc(void);
DrawSlot *DrawSlotB_Get(s32 i);
void ObjAttach_StartAttack(ObjAttach *, s32 a, void *b);
void PitfallTrap_Draw(DrawSlot *d);
extern void *PitfallTrap_RowsPtr;
extern HitShape *PitfallTrap_Val088;
extern s16 *BowArrow_EventTbls[];
EmBone *Em_Bone(void *obj, s16 bone);
void EmBomb_Draw(DrawSlot *d);
s16 DrawSlot_Alloc(void);
DrawSlot *DrawSlot_Get(s32 i);
void EmBomb_Update(ObjAttach *o);
void EmBomb_Follow(ObjAttach *o, Em *em);
void EmBomb_InitTimers(ObjAttach *o);
void Vec_Rotate(f32 *out, f32 *v, f32 *m);
void BowArrow_Update(ObjAttach *o);
/* The bow arrow's block of the object buffer (BowArrow_Create; ObjAttach.buf). */
typedef struct {
    u8 bounced;         /* 0x00: 1 once BowArrow_BounceStep ran (BowArrow_Draw then stops turning it along its velocity) */
    u8 event;           /* 0x01: the next entry of the kind's event table (BowArrow_StepEvents), 0xFF: past the last */
    u8 pad02[2];
    f32 power;          /* 0x04: the object's power at set up (BowArrow_ApplyEvent scales it) */
    f32 attackFactor;   /* 0x08: the player's attackFactor */
    u8 ammoType;        /* 0x0C: the player's ammoType (BowArrow_ApplyCoating) */
    u8 charge;          /* 0x0D: the player's chargeLevel & 3: the row of BowArrow_ChargeFlight and the scales */
    u8 doneFrames;      /* 0x0E: frames counted once event is 0xFF */
    u8 skills;          /* 0x0F: 1 SKILL_STATUS_PLUS (element x1.125), 2 skill 0xBA (range x2), 4 skill 0xB9 (range x0.5) */
    u16 elementFlags;   /* 0x10: the weapon's element bits (Player_WeaponElementFlags) */
    u8 elementLevel;    /* 0x12: the weapon's element value */
    u8 crit;            /* 0x13: the affinity roll (BowArrow_Fire): 1 a critical hit (x1.25, HITFX_CRITICAL), 2 a weak one (x0.5, HITFX_WEAK) */
    s16 shot;           /* 0x14: the shot's level: the index into BowArrow_LevelScale */
} BowArrowBuf;

void BowArrow_InitAttack(BowArrowBuf *b, Player *pl);
void BowArrow_SkillFlags(BowArrowBuf *b, Player *pl);
extern s16 *BowArrow_ShotParams[];
extern u16 BowArrow_SpreadScale[];
extern s16 BowArrow_SpreadDirs[];
void Mtx_Translation(f32 *m, f32 x, f32 y, f32 z);
void Vec_DirToAngles(f32 *dir, s16 *pitch, s16 *yaw);
void Mtx_RotateX(f32 *m, f32 a);
void Mtx_RotateY(f32 *m, f32 a);
s16 Player_GetWeaponStat(Player *p, u8 kind);
u16 Math_Heading(f32 x1, f32 z1, f32 x2, f32 z2);
extern void *Boomerang_RowsPtr;
void Mtx_Scale(f32 *m, f32 x, f32 y, f32 z);
void Mtx_RotateXYZ(f32 *m, f32 x, f32 y, f32 z);
void Mtx_SetTrans(f32 *m, f32 x, f32 y, f32 z);
extern s16 EmBomb_Models[];
void VespoidAttack_Draw(ObjAttach *o);
extern HitShape *VespoidAttack_HitShapes[];
extern void *VespoidAttack_RowsPtr;
extern void *HermitaurAttack_RowsPtr;
void EmBomb_RunHeld(ObjAttach *o);
void EmBomb_RunMelynxThrown(ObjAttach *o);
void EmBomb_RunShakalakaThrown(ObjAttach *o);
void FlashObj_Remove(EmObj *obj);
s32 EmBlangonga_IsSmallKind(Em *em);
extern HitShape *BlangongaAttack_HitShapes[];
extern void *BlangongaAttack_RowsPtr;
u8 BowArrow_BounceStep(ObjAttach *o);
void BowArrow_Move(ObjAttach *o);
void BowArrow_Fall(ObjAttach *o);
s32 EmMotion_PassedFrameScaled(void *p, f32 frame, s32 layer);
extern HitShape *HermitaurAttack_HitShapes[];
extern HitShape *KutkuAttack_HitShapes[];
extern void *KutkuAttack_RowsPtr;
s32 FlashObj_Add(EmObj *obj);
s32 Model_Draw(u32 h, s32 flags);
void ObjAttach_Free(ObjAttach *p);
s32 Player_HasSkill(Player *p, u8 skill);
void VespoidAttack_Run(ObjAttach *o);
void VespoidAttack_End(ObjAttach *o);
void VespoidAttack_Free(ObjAttach *o);
void PitfallTrap_End(ObjAttach *o);
void PitfallTrap_Free(ObjAttach *o);
void VelocipreyAttack_Setup(ObjAttach *o);
void VelocipreyAttack_Run(ObjAttach *o);
void VelocipreyAttack_End(ObjAttach *o);
void VelocipreyAttack_Free(ObjAttach *o);
void EmBomb_End(ObjAttach *o);
void EmBomb_Free(ObjAttach *o);
void DromeAttack_Setup(ObjAttach *o);
void DromeAttack_Run(ObjAttach *o);
void DromeAttack_End(ObjAttach *o);
void DromeAttack_Free(ObjAttach *o);
void KhezuAttack_Setup(ObjAttach *o);
void KhezuAttack_Run(ObjAttach *o);
void KhezuAttack_End(ObjAttach *o);
void KhezuAttack_Free(ObjAttach *o);
void KutkuAttack_End(ObjAttach *o);
void KutkuAttack_Free(ObjAttach *o);
void DiablosAttack_Setup(ObjAttach *);
void DiablosAttack_Run(ObjAttach *o);
void DiablosAttack_End(ObjAttach *o);
void DiablosAttack_Free(ObjAttach *o);
void GraviosAttack_Setup(ObjAttach *o);
void GraviosAttack_End(ObjAttach *o);
void GraviosAttack_Free(ObjAttach *o);
void PlesiothAttack_Setup(ObjAttach *o);
void PlesiothAttack_Run(ObjAttach *o);
void PlesiothAttack_End(ObjAttach *o);
void PlesiothAttack_Free(ObjAttach *o);
void CephadromeAttack_Setup(ObjAttach *o);
void CephadromeAttack_Run(ObjAttach *o);
void CephadromeAttack_End(ObjAttach *o);
void CephadromeAttack_Free(ObjAttach *o);
void StageShot_Hit(ObjAttach *o);
void StageShot_End(ObjAttach *o);
void StageShot_Free(ObjAttach *o);
void CephalosAttack_Setup(ObjAttach *o);
void CephalosAttack_Run(ObjAttach *o);
void CephalosAttack_End(ObjAttach *o);
void CephalosAttack_Free(ObjAttach *o);
void BowArrow_End(ObjAttach *o);
void BowArrow_Free(ObjAttach *o);
void GarugaAttack_Setup(ObjAttach *o);
void GarugaAttack_Run(ObjAttach *o);
void GarugaAttack_End(ObjAttach *o);
void GarugaAttack_Free(ObjAttach *o);
void HermitaurAttack_End(ObjAttach *o);
void HermitaurAttack_Free(ObjAttach *o);
void BlangongaAttack_End(ObjAttach *o);
void BlangongaAttack_Free(ObjAttach *o);
void Boomerang_Run(ObjAttach *o);
void Boomerang_End(ObjAttach *o);
void Boomerang_Free(ObjAttach *o);
s32 Gs_SetState(u8 reg, u32 v);
void Vec_Copy(f32 *out, f32 *in);
f32 AreaGrid2_GetFloorYFiltered(f32 *);
void Draw_ApplyStates(u32);
void Draw_SetDefaultState(void);
void Gs_SetLightList(ModelMat *mats, ModelMatSet *set);
void DrawSlot_Free(s32);
void DrawSlotB_Free(s32);
void ObjAttach_Move(ObjAttach *);
void ObjAttach_MoveAccel(ObjAttach *);
extern s16 gBowgunClipShots[][4];
void BowArrow_Fire(Player *pl, s16 shot);
void PlayerFx_CreateOnPlayerDefault(Player *p, u8 k, f32 scale);
f32 Vec_Distance(f32 *a, f32 *b);
void CamShake_SetAEverywhere(s32);
extern f32 gCamPos;
void FireBlast_CreateScaled(f32 *pos, s16 k, f32 scale);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);
void StageShot_CamShake(ObjAttach *o);
void HitSpark_SpawnScaled(s16 ang, u8 type, u8 sub, f32 *pos, f32 scale);
void PartFx_SpawnIfHere(void *o, f32 *pos, s32 type, f32 scale);
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);
u8 Player_InCurrentArea(void *obj);
ObjAttach *ObjAttach_Alloc(s32 n);
void DromeAttack_Update(ObjAttach *o);
void KhezuAttack_Update(ObjAttach *o);
void KutkuAttack_Update(ObjAttach *o);
void DiablosAttack_Update(ObjAttach *o);
void PlesiothAttack_Update(ObjAttach *o);
void CephadromeAttack_Update(ObjAttach *o);
void CephalosAttack_Update(ObjAttach *o);
void GarugaAttack_Update(ObjAttach *o);
f32 Math_Absf(f32 x);
s32 Player_TestSetFlags(void *, s32 mask);
void ObjAttach_StartOwnedAttack(ObjAttach *, s32 a, s32 b, void *c);
extern HitShape *DiablosAttack_HitShapes[];
extern void *DiablosAttack_RowsPtr;
extern HitShape *PlesiothAttack_HitShapes[];
extern void *PlesiothAttack_RowsPtr;
extern HitShape *CephadromeAttack_HitShapes[];
extern void *CephadromeAttack_RowsPtr;
extern HitShape *CephalosAttack_HitShapes[];
extern void *CephalosAttack_RowsPtr;
extern HitShape *GarugaAttack_HitShapes[];
extern void *GarugaAttack_RowsPtr;
extern HitShape *VelocipreyAttack_HitShapes[];
extern void *VelocipreyAttack_RowsPtr;
extern HitShape *DromeAttack_HitShapes[];
extern void *DromeAttack_RowsPtr;
extern HitShape *KhezuAttack_HitShapes[];
extern void *KhezuAttack_RowsPtr;
extern HitShape *GraviosAttack_HitShapes[];
extern void *GraviosAttack_RowsPtr;
void VespoidAttack_Create(Em *em, s8 kind);
void VelocipreyAttack_Create(Em *em, s8 kind);
void VespoidAttack_Update(ObjAttach *o);
void VelocipreyAttack_Update(ObjAttach *o);
void KutkuAttack_Setup(ObjAttach *o);
void HermitaurAttack_Run(ObjAttach *o);
/* A row of BowArrow_Kinds (0x20 bytes): one kind of arrow shot. */
typedef struct {
    s16 keepSpeed;      /* 0x00: 0, arrow i of a shot flies at speed * (1 - 0.2 i) */
    s16 eventTbl;       /* 0x02: the index into BowArrow_EventTbls, -1 none */
    f32 speed;          /* 0x04: the start speed */
    f32 speedRand;      /* 0x08: the random part of the speed */
    f32 gravity;        /* 0x0C */
    f32 gravityRand;    /* 0x10: the random part of the gravity */
    f32 lift;           /* 0x14: the rise per frame, by the horizontal speed */
    f32 sway;           /* 0x18: the side drift of the first 20 frames */
    s16 straightFrames; /* 0x1C: the frames flown without gravity */
    u8 hitMode;         /* 0x1E: 2 pierces count times; 1 rides the monster's bone */
    u8 count;           /* 0x1F */
} BowArrowKind;
extern BowArrowKind BowArrow_Kinds[];
void BowArrow_Run(ObjAttach *o);
void BowArrow_Hit(ObjAttach *o);
void BlangongaAttack_Setup(ObjAttach *o);
void KutkuAttack_Run(ObjAttach *o);
void EmBomb_Run(ObjAttach *o);
void VespoidAttack_Setup(ObjAttach *o);
void HermitaurAttack_Setup(ObjAttach *o);
void Boomerang_Hit(ObjAttach *o);
void Player_WeaponElementFlags(void *pl, u16 *flags, s8 *level);
void BowArrow_Fire(Player *pl, s16 shot);
void EmBomb_Setup(ObjAttach *o);
void PitfallTrap_Setup(ObjAttach *o);
void Boomerang_Setup(ObjAttach *o);
void GraviosAttack_Run(ObjAttach *o);
void BowArrow_Setup(ObjAttach *o);
void BowArrow_Deflect(ObjAttach *o);
void BlangongaAttack_Run(ObjAttach *o);
void PitfallTrap_Run(ObjAttach *o);
void StageShot_Setup(ObjAttach *o);
void StageShot_Run(ObjAttach *o);
s8 BowArrow_StepEvents(ObjAttach *o);
/* end of prototypes */

/* A part of the trap effect PlacedItem: three from PlacedItemBuf.parts, a DrawSlotB slot each (PlacedItem_Draw). */
typedef struct {
    s16 drawSlot;       /* 0x00: its DrawSlotB index */
    s16 kind;           /* 0x02: its index 0..2, the part's kind (PlacedItem_RunTrap, PlacedItem_Draw switch on it) */
    DrawSlot *slot;     /* 0x04: DrawSlotB_Get(drawSlot), NULL: none */
    s16 timer;          /* 0x08: frames, from a negative wait; drawn while above 0 */
    s16 frame;          /* 0x0A: the colour key frame (up to 9) */
    u16 angle;          /* 0x0C: turned by 0x2AAB a frame */
    u8 pad0E[2];
} PlacedItemPart;

/* The trap effect PlacedItem's block of the object buffer. */
typedef struct {
    u8 phase;           /* 0x00: PlacedItem_RunTrap switches on it */
    u8 unk01;           /* 0x01 */
    u8 unk02;           /* 0x02 */
    u8 unk03;           /* 0x03 */
    Em *em;             /* 0x04: the monster whose bone it follows (PlacedItem_FollowBone) */
    f32 ofs[3];         /* 0x08: its offset in the bone's frame */
    s16 bone;           /* 0x14 */
    u8 pad16[2];
    PlacedItemPart parts[3]; /* 0x18 */
} PlacedItemBuf;

/*
 * Sets up the holding trap's (type 4) three parts in PlacedItemBuf and clears
 * phase. Each part gets its index as its kind and a DrawSlotB slot drawn by
 * PlacedItem_Draw at the trap's position (part 1 five higher). Part 2 also
 * gets a random angle. A part with no free slot gets slot NULL.
 */
void PlacedItem_InitTrapParts(ObjAttach *o)
{
    PlacedItemBuf *buf = (PlacedItemBuf *) o->buf;
    PlacedItemPart *trapPart;
    s16 i;

    buf->phase = 0;
    buf->unk01 = 0;
    buf->unk02 = 0;
    buf->unk03 = 0;
    trapPart = buf->parts;
    for (i = 0; i < 3; i++, trapPart++) {
        trapPart->kind = i;
        trapPart->timer = 0;
        trapPart->frame = 0;
        trapPart->drawSlot = DrawSlotB_Alloc();
        switch (trapPart->kind) {
        case 0:
        case 1:
        default:
            trapPart->angle = 0;
            break;
        case 2:
            trapPart->angle = Rand_Next(1);
            break;
        }
        if (trapPart->drawSlot != -1) {
            trapPart->slot = DrawSlotB_Get(trapPart->drawSlot);
            trapPart->slot->owner = o;
            trapPart->slot->part = i;
            trapPart->slot->draw = PlacedItem_Draw;
            Vec_Copy(trapPart->slot->pos, o->pos);
            switch (trapPart->kind) {
            case 1:
                trapPart->slot->pos[1] += 5.0f;
                break;
            }
        } else {
            trapPart->slot = NULL;
        }
    }
}

/*
 * Sets a placed item up by type. The barrels (0, 1, 6) get a hurt shape and
 * ATKF_SHIELDED, so a hit sets them off. Types 2 and 3 get a hit shape that
 * hits no monster. The holding trap (4) gets a hit shape that hits no player;
 * it starts hidden, with its set effect, its sound and three parts. The stuck
 * bomb (5) may hit its owner, follows its bone, and with the bomb skill
 * (stateVar0 1) has 1.5 times the power, at most 0xFF. Then it takes a
 * DrawSlotB slot for PlacedItem_DrawMain, or frees the item.
 */
void PlacedItem_Setup(ObjAttach *o)
{
    u16 boosted;

    o->state++;
    o->visible = 1;
    o->draw = NULL;
    o->ownerIndex = 0xFF;
    o->hitPhase = 0;
    o->didHit = 0;
    o->targetEm = 0;
    o->targetPart = 0;
    if (PlacedItem_Motions[o->type] != -1) {
        ObjAttach_StartOwnedAttack(o, (s32) o->owner, PlacedItem_Motions[o->type], &PlacedItem_RowsPtr);
    }
    switch (o->type) {
    case PLACED_LARGE_BARREL_BOMB:
    case PLACED_LARGE_BARREL_BOMB_B:
    case PLACED_BOMB:
        o->hitList = 0;
        o->hurtList = PlacedItem_Shapes[o->type];
        Player_TestSetFlags(o, ATKF_SHIELDED);
        break;
    case PLACED_PLAYER_HIT_A:
    case PLACED_PLAYER_HIT_B:
    default:
        o->hitList = PlacedItem_Shapes[o->type];
        o->hurtList = 0;
        Player_TestSetFlags(o, ATKF_NO_EM);
        break;
    case PLACED_TRAP:
        o->hitList = PlacedItem_Shapes[o->type];
        o->hurtList = 0;
        Player_TestSetFlags(o, ATKF_NO_PLAYER);
        o->rotY = 0;
        o->visible = 0;
        if (gScene.area == o->area) {
            PlayerFx_CreateAtDefault(o->pos, 5, 1.0f);
            Sound_PlaySeAtPosChecked(1, 0x89, 0, o->pos, 1, 0);
        }
        PlacedItem_InitTrapParts(o);
        break;
    case PLACED_MONSTER_BOMB:
        o->hitList = PlacedItem_Shapes[o->type];
        o->hurtList = 0;
        Player_TestSetFlags(o, ATKF_HITS_OWNER);
        PlacedItem_FollowBone(o);
        o->aux = 0;
        switch (o->stateVar0) {
        case 1:
            boosted = 1.5f * o->power;
            if (boosted > 0xFF) {
                boosted = 0xFF;
            }
            o->power = boosted;
            break;
        }
        break;
    }
    o->ownerMotion = 0;
    o->drawSlot = DrawSlotB_Alloc();
    if (o->drawSlot != -1) {
        o->drawSlotBuf = DrawSlotB_Get(o->drawSlot);
        o->drawSlotBuf->owner = o;
        o->drawSlotBuf->draw = PlacedItem_DrawMain;
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
    } else {
        PlacedItem_Free(o);
    }
}

/*
 * The holding trap's frame (PlacedItem_Run, type 4); returns 1 once the trap
 * is spent, else 0. The trap appears after 2 frames. buf->phase 0: armed. It
 * catches the monster its hit touched (didHit), or a monster within
 * Em_TrapReach(type, 500) whose holdTimer2 is above 0 and that is not defeated
 * or in an event, and makes the StageObj_K0F catch effect. Phase 1: waits up
 * to 60 frames for the monster's holdTimer2. Phase 2: holds until holdTimer2
 * runs out; once the monster is defeated or in an event, it stops counting in
 * gScene.placed2. Every frame it turns, steps its three parts (part 2
 * restarts every 20 frames, after a random wait of 30-45 frames except while
 * holding) and queues their draw slots.
 */
u8 PlacedItem_RunTrap(ObjAttach *o)
{
    PlacedItemBuf *buf = (PlacedItemBuf *) o->buf;
    PlacedItemPart *part = buf->parts;
    Em *target = o->targetEm;
    Em *em;
    s16 i;
    u8 found = 0;
    f32 dist;

    if (o->ownerMotion > 2) {
        o->visible = 1;
    }
    switch (buf->phase) {
    case 0:
        if (o->didHit != 0) {
            found = 1;
        } else {
            for (em = gEms, i = 0; i < EM_COUNT; i++, em++) {
                if (em->active != 0 && em->area == o->area && (u32) (em->stateGroup - EM_GROUP_DEFEAT) > 1 && (s8) em->holdTimer2 > 0) {
                    dist = Vec_Distance(o->pos, &em->posX);
                    if (dist <= Em_TrapReach(em->type, 500.0f)) {
                        o->targetEm = em;
                        target = em;
                        found = 2;
                        break;
                    }
                }
            }
        }
        if (found) {
            o->hitPhase = 0;
            o->aux = 0;
            if (target == NULL) {
                return 1;
            }
            buf->phase++;
            if (o->area == gScene.area) {
                ElementBurst_Create(o->pos, 7, o->rotY, 0, 1.0f);
            }
        }
        break;
    case 1:
        if (++o->aux > 60) {
            o->aux = 0;
            return 1;
        }
        if (target == NULL) {
            return 1;
        }
        if (target->active == 0) {
            return 1;
        }
        if ((s8) target->holdTimer2 != 0) {
            buf->phase++;
        }
        break;
    case 2:
        if (target == NULL) {
            return 1;
        }
        if (target->active == 0) {
            return 1;
        }
        if (o->stateVar1 == 2 && (target->stateGroup == EM_GROUP_EVENT || target->stateGroup == EM_GROUP_DEFEAT)) {
            (gScene.placed2)--;
            o->stateVar1 = 0;
        }
        if ((s8) target->holdTimer2 <= 0) {
            return 1;
        }
        break;
    }
    o->rotY += 0x1555;
    for (i = 0; i < 3; i++, part++) {
        switch (part->kind) {
        case 0:
        case 1:
        default:
            if (part->timer >= 2) {
                part->timer = 0;
            }
            if (part->frame >= 9) {
                part->frame = 9;
            }
            break;
        case 2:
            if (part->timer >= 20) {
                if (buf->phase == 2) {
                    part->timer = 0;
                } else {
                    part->timer = -((Rand_Next(1) & 0xF) + 30);
                }
            }
            if (part->timer == 0 && o->area == gScene.area) {
                Sound_PlaySeAtPosChecked(1, 0x8A, 0, o->pos, 1, 0);
            }
            part->angle += 0x2AAB;
            break;
        }
        (part->timer)++;
        (part->frame)++;
        if (part->slot != NULL) {
            switch (part->kind) {
            case 2:
                OT_AddByDepth(gDrawOt32, part->slot, 0x20U, 0);
                break;
            default:
                OT_AddByDepth(gDrawOtSorted, part->slot, 0x40U, 0);
                break;
            }
        }
    }
    return 0;
}

/*
 * Places the stuck bomb (type 5) on its monster: PlacedItemBuf.ofs turned by
 * the matrix of bone buf->bone of buf->em, added to the bone's position, 50
 * higher.
 */
void PlacedItem_FollowBone(ObjAttach *o)
{
    PlacedItemBuf *buf = (PlacedItemBuf *) o->buf;
    f32 rotOfs[3];
    f32 *boneMtx;

    boneMtx = (f32 *)Em_Bone(buf->em, buf->bone);
    Vec_Rotate(rotOfs, buf->ofs, boneMtx);
    o->pos[0] = boneMtx[12] + rotOfs[0];
    o->pos[1] = 50.0f + (boneMtx[13] + rotOfs[1]);
    o->pos[2] = boneMtx[14] + rotOfs[2];
}

/*
 * A placed item's frame. Every item ends after 9000 frames. A barrel (0, 1,
 * 6) bursts once hit (wasHit): it makes a StageObj_K0E and an Explosion of
 * kind 2 (type 0) or 0x16 (type 1), both passing the bomb skill in stateVar0,
 * or of kind 0x21 with its own effects and sound (type 6). Types 2 and 3 keep
 * their hit window open, and the trap runs PlacedItem_RunTrap. The stuck bomb
 * follows its bone for 224 frames, then bursts once (hitDelay 5, a sound and
 * a StageObj_K0E) and hides; it ends when its hit window closes. Then it
 * queues its draw slot at its position.
 */
void PlacedItem_Run(ObjAttach *o)
{
    if (++o->ownerMotion > 9000) {
        o->hitPhase = 0;
        o->state = ATTACH_END;
        return;
    }
    switch (o->type) {
    case PLACED_LARGE_BARREL_BOMB:
    case PLACED_LARGE_BARREL_BOMB_B:
    case PLACED_BOMB:
        if (o->wasHit != 0) {
            o->state = ATTACH_END;
            if (o->type == PLACED_LARGE_BARREL_BOMB) {
                if (gScene.area == o->area) {
                    FireBlast_CreateScaled(o->pos, 0, 2.0f);
                }
                Explosion_CreateAt(o->pos, EXPLOSION_LARGE_BARREL_BOMB, o->area, o->stateVar0);
            } else if (o->type == PLACED_LARGE_BARREL_BOMB_B) {
                if (gScene.area == o->area) {
                    FireBlast_CreateScaled(o->pos, 0, 3.0f);
                }
                Explosion_CreateAt(o->pos, EXPLOSION_LARGE_BARREL_BOMB_B, o->area, o->stateVar0);
            } else {
                if (gScene.area == o->area) {
                    PlayerFx_CreateAtDefault(o->pos, 0xD, 1.0f);
                    PlayerFx_CreateAtDefault(o->pos, 0xE, 1.0f);
                    Sound_PlaySeAtPosChecked(1, 0x93, 0, o->pos, 1, 0);
                }
                Explosion_CreateAt(o->pos, EXPLOSION_PLACED_BOMB, o->area, 0);
            }
        }
        break;
    case PLACED_PLAYER_HIT_A:
    case PLACED_PLAYER_HIT_B:
    default:
        o->hitFrames = 99;
        break;
    case PLACED_TRAP:
        if (PlacedItem_RunTrap(o) == 1) {
            o->hitPhase = 0;
            o->state = ATTACH_END;
            return;
        }
        o->hitFrames = 99;
        break;
    case PLACED_MONSTER_BOMB:
        if (o->ownerMotion <= 224) {
            PlacedItem_FollowBone(o);
            o->hitDelay = 99;
        } else if (o->aux == 0) {
            o->hitDelay = 5;
            if (o->area == gScene.area) {
                Sound_PlaySeAtPosChecked(1, 0x4F, 0, o->pos, 15, 0);
                FireBlast_CreateScaled(o->pos, 0, 2.0f);
            }
            o->aux++;
            o->visible = 0;
        }
        if (o->hitPhase == 0) {
            o->state = ATTACH_END;
            return;
        }
        break;
    }
    if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
    }
}

/*
 * ATTACH_HIT: the placed item hit something this frame. The trap and the
 * stuck bomb (4, 5) run on (PlacedItem_Run). The others end, type 2 with a
 * small StageObj_K0E puff, and queue their draw slot.
 */
void PlacedItem_Hit(ObjAttach *o)
{
    switch (o->type) {
    case PLACED_LARGE_BARREL_BOMB:
    case PLACED_LARGE_BARREL_BOMB_B:
    case PLACED_PLAYER_HIT_B:
    case PLACED_BOMB:
    default:
        o->state = ATTACH_END;
        break;
    case PLACED_PLAYER_HIT_A:
        o->state = ATTACH_END;
        if (o->area == gScene.area) {
            FireBlast_CreateScaled(o->pos, 0, 1.0f);
        }
        break;
    case PLACED_TRAP:
    case PLACED_MONSTER_BOMB:
        PlacedItem_Run(o);
        return;
    }
    if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
    }
}

/*
 * The holding trap's end (PlacedItem_End, type 4): a small StageObj_K0E puff
 * and sound 0x4D in the current area, then frees the DrawSlotB slots of its
 * three parts.
 */
void PlacedItem_EndBurst(ObjAttach *o)
{
    s16 i;
    PlacedItemPart *part;

    part = ((PlacedItemBuf *) o->buf)->parts;
    if (o->area == gScene.area) {
        FireBlast_CreateScaled(o->pos, 0, 0.5f);
        Sound_PlaySeAtPosChecked(1, 0x4D, 0, o->pos, 1, 0);
    }
    for (i = 0; i < 3; i++) {
        if (*(s32 *)&part->slot != 0) {
            DrawSlotB_Free(part->drawSlot);
        }
        part++;
    }
}

/*
 * Hides the placed item, frees its draw slot (and the trap's parts,
 * PlacedItem_EndBurst) and frees it on the next update.
 */
void PlacedItem_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = ATTACH_FREE;
    o->visible = 0;
    if (o->drawSlotBuf != NULL) {
        DrawSlotB_Free(o->drawSlot);
    }
    switch (o->type) {
    case PLACED_TRAP:
        PlacedItem_EndBurst(o);
        break;
    }
}

/*
 * Takes the item off the local player's count (gScene.placed1 when stateVar1
 * is 1, placed2 when it is 2) and frees it.
 */
void PlacedItem_Free(ObjAttach *o)
{
    switch (o->stateVar1) {
    case 1:
        (gScene.placed1)--;
        break;
    case 2:
        (gScene.placed2)--;
        break;
    }
    ObjAttach_Free(o);
}

/*
 * Draws model set mdl with matrix m, lit by the materials tex, then restores
 * the default draw state.
 */
void PlacedItem_DrawModel(ModelMatSet *mdl, f32 *mtx, ModelMat *tex)
{
    Gs_SetState(0x1A, (s32) mtx);
    if (mdl != NULL && mdl->id != -1) {
        Gs_SetLightList(tex, mdl);
        Draw_ApplyStates(mdl->flags);
        Model_Draw(mdl->id, 0);
    }
    Draw_SetDefaultState();
}

/*
 * Draws the placed item's model from gCommonModels[0] while it is visible in
 * the current area, turned by rotY at pos. The barrels use set 55 at scale 3
 * (types 0, 5), 4.5 (1) or 2.1 (6); type 6 shows the set's material 1, the
 * others hide it. Types 2, 3 and 4 use the trap set 116.
 */
void PlacedItem_DrawMain(DrawSlot *slot)
{
    f32 mtx[16];
//{
    ObjAttach *o = slot->owner;
    ModelMat *tex;
    ModelMatSet *model;
    Model *mdl;
//}

    if (gScene.area == o->area && o->visible != 0) {
        mdl = gCommonModels[0];
        if (mdl != NULL && mdl->active != 0) {
            tex = mdl->mats;
            switch (o->type) {
            case PLACED_LARGE_BARREL_BOMB:
            case PLACED_MONSTER_BOMB:
                Mtx_Scale(mtx, 3.0f, 3.0f, 3.0f);
                Mtx_RotateY(mtx, (6.2831855f * o->rotY) / 65536.0f);
                model = &mdl->matSets[55];
                {
                    s32 i;
                    s32 *ids;
                    ModelMat *mat;
                    i = 0;
                    if (i < model->count) {
                        ids = (s32 *) model;
                        do {
                            mat = &tex[ids[2]];
                            if (i == 1) {
                                mat->alpha = 0.0f;
                            }
                            i++;
                            ids++;
                        } while (i < model->count);
                    }
                }
                break;
            case PLACED_LARGE_BARREL_BOMB_B:
                Mtx_Scale(mtx, 4.5f, 4.5f, 4.5f);
                Mtx_RotateY(mtx, (6.2831855f * o->rotY) / 65536.0f);
                model = &mdl->matSets[55];
                {
                    s32 *ids;
                    s32 i;
                    ModelMat *mat;
                i = 0;
                    if (i < model->count) {
                        ids = (s32 *) model;
                        do {
                            mat = &tex[ids[2]];
                            if (i == 1) {
                                mat->alpha = 0.0f;
                            }
                            i++;
                            ids++;
                        } while (i < model->count);
                    }
                }
                break;
            case PLACED_PLAYER_HIT_A:
            case PLACED_PLAYER_HIT_B:
                Mtx_Identity(mtx);
                Mtx_RotateY(mtx, (6.2831855f * o->rotY) / 65536.0f);
                model = &mdl->matSets[116];
                break;
            case PLACED_TRAP:
                Mtx_Scale(mtx, 1.0f, 1.0f, 1.0f);
                Mtx_RotateY(mtx, (6.2831855f * o->rotY) / 65536.0f);
                model = &mdl->matSets[116];
                break;
            case PLACED_BOMB:
                Mtx_Scale(mtx, 2.1f, 2.1f, 2.1f);
                Mtx_RotateY(mtx, (6.2831855f * o->rotY) / 65536.0f);
                model = &mdl->matSets[55];
                {
                    s32 *ids;
                    s32 i;
                    ModelMat *mat;
                i = 0;
                    if (i < model->count) {
                        ids = (s32 *) model;
                        do {
                            mat = &tex[ids[2]];
                            if (i == 1) {
                                mat->alpha = 1.0f;
                            }
                            i++;
                            ids++;
                        } while (i < model->count);
                    }
                }
                break;
            }
            Mtx_SetTrans(mtx, o->pos[0], o->pos[1], o->pos[2]);
            PlacedItem_DrawModel(model, mtx, tex);
        }
    }
}

/*
 * Draws one of the holding trap's parts (slot->part) while its timer is above
 * 0, with model PlacedItem_PartModels[kind] of gCommonModels[0]. Parts 0 and 1
 * lie flat, scaled and coloured by their key tables over timer and frame.
 * Part 2 is drawn additive with a scrolling texture, scaled by its key table
 * (three times as tall while the trap holds) and turned by its angle.
 */
void PlacedItem_Draw(DrawSlot *slot)
{
    s32 col;
    f32 scale[3];
    f32 rot[3];
    f32 mtx[16];
    f32 texMtx[16];
    ObjAttach *o = slot->owner;
    PlacedItemBuf *trapBuf = (PlacedItemBuf *) o->buf;
    PlacedItemPart *trapPart = &trapBuf->parts[slot->part];
    ModelMat *tex;
    u16 flags = 0;
    u16 order;
    u8 lit = 1;
    Model *set;
    ModelMatSet *mdl;

    if (gScene.area == o->area && o->visible != 0 && trapPart->timer > 0) {
        set = gCommonModels[0];
        if (set != NULL && set->active != 0) {
            tex = set->mats;
            switch (trapPart->kind) {
            case 0:
                Key_LerpVec2((f32 *) PlacedItem_BasePartScaleKeys, scale, trapPart->timer);
                Color_EvalKeys((s32 *) PlacedItem_BasePartColorKeys, trapPart->frame, (u32 *) &col);
                rot[0] = -1.5707964f;
                order = 1;
                flags |= 2;
                break;
            case 1:
                Key_LerpVec2((f32 *) PlacedItem_RaisedPartScaleKeys, scale, trapPart->timer);
                Color_EvalKeys((s32 *) PlacedItem_RaisedPartColorKeys, trapPart->frame, (u32 *) &col);
                rot[0] = -1.5707964f;
                order = 1;
                flags |= 2;
                break;
            case 2:
                Mtx_Translation(texMtx, 0.0f, 0.375f * (trapPart->timer / 20.0f), 0.0f);
                Gs_SetState(0x19, (s32) texMtx);
                Key_LerpVec3((f32 *) PlacedItem_SpinPartScaleKeys, scale, trapPart->timer);
                if (trapBuf->phase == 2) {
                    scale[1] *= 3.0f;
                }
                col = -1;
                rot[1] = (6.2831855f * trapPart->angle) / 65536.0f;
                order = 2;
                flags |= 4;
                lit = 0;
                break;
            default:
                return;
            }
            Mtx_FromScaleRotTrans(scale, rot, slot->pos, order, mtx);
            mdl = &set->matSets[PlacedItem_PartModels[trapPart->kind]];
            if (lit == 0) {
                Gs_SetState(0x67, -1);
                Effect_DrawModelAdd(mdl, mtx, tex);
            } else {
                Gs_SetState(0x6C, 0);
                Effect_DrawModel(mdl, mtx, col, flags, tex);
            }
            Gs_SetState(0x6C, 1);
        }
    }
}

/*
 * Makes a VespoidAttack for em with row kind of VespoidAttack_Rows (its
 * type), if a player is in em's area. Clears em's hitPending and contactHit.
 */
void VespoidAttack_Create(Em *em, s8 kind)
{
    ObjAttach *o;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((o = ObjAttach_Alloc(0)) != NULL) {
            o->kind = ATTACH_VESPOID_ATTACK;
            o->type = kind;
            o->update = VespoidAttack_Update;
            o->idx = em->index;
            o->isEm = em->isEm;
            o->ownerMotion = em->motionId[0];
            o->owner = em;
            o->yaw = em->rotY;
            o->rotX = em->rotX;
            o->rotY = em->rotY;
            o->rotZ = em->rotZ;
            o->side = *(u8 *) &em->side;
            em->hitPending = 0;
            em->contactHit = 0;
        }
    }
}

/* The VespoidAttack's per-frame update, by ObjAttach.state. */
void VespoidAttack_Update(ObjAttach *o)
{
    switch (o->state) {
    case ATTACH_SETUP:
        VespoidAttack_Setup(o);
        break;
    case ATTACH_RUN:
        VespoidAttack_Run(o);
        break;
    case ATTACH_END:
        VespoidAttack_End(o);
        break;
    case ATTACH_HIT:
        VespoidAttack_Run(o);
        break;
    case 4:
        VespoidAttack_End(o);
        break;
    case ATTACH_DEFLECTED:
        VespoidAttack_End(o);
        break;
    case ATTACH_DEFLECTED_AURA:
        VespoidAttack_End(o);
        break;
    case 7:
        VespoidAttack_End(o);
        break;
    case ATTACH_FREE:
        VespoidAttack_Free(o);
        
    default:
        break;
    }
}

/*
 * Sets the attack up from its row: the hit shapes, ATKF_OWNER_CONTACT and the
 * empty VespoidAttack_Draw. hitDelay is counted from the monster's motion.
 */
void VespoidAttack_Setup(ObjAttach *o)
{
    Em *em = &gEms[o->idx];
    s32 blendFrames;

    o->state++;
    o->visible = 1;
    o->draw = VespoidAttack_Draw;
    Player_TestSetFlags(o, ATKF_OWNER_CONTACT);
    o->wasHit = 0;
    ObjAttach_StartOwnedAttack(o, (s32) em, o->type, &VespoidAttack_RowsPtr);
    o->hitList = VespoidAttack_HitShapes[o->hitListNo];
    o->hurtList = 0;
    o->area = em->area;
    if (o->hitDelay != 0) {
        blendFrames = Math_Absf(em->motionBlend[0] % 100);
        o->hitDelay += blendFrames - (s32) Math_Absf(em->motionFrame[0]) - 1;
        if ((s8) o->hitDelay < 0) {
            o->hitDelay = 0;
        }
    }
    o->drawSlotBuf = NULL;
}

/*
 * Ends the attack when the monster is gone, its motion changes or the hit
 * window closes; types 4 and 10 keep their window open.
 */
void VespoidAttack_Run(ObjAttach *o)
{
    Em *em = &gEms[o->idx];

    switch (o->type) {
    case 4:
    case 10:
        o->hitFrames = 99;
        break;
    }
    if (em->active == 0) {
        o->hitPhase = 0;
        o->state = ATTACH_END;
        return;
    }
    if (em->motionId[0] != o->ownerMotion || o->hitPhase == 0) {
        o->hitPhase = 0;
        o->state = ATTACH_END;
    }
    if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
    }
}

/* Hides the attack, frees its draw slot and frees it on the next update. */
void VespoidAttack_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = ATTACH_FREE;
    o->visible = 0;
    if (o->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) o->drawSlot);
    }
}

/* Frees the object (ObjAttach_Free). */
void VespoidAttack_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

/* The attack's draw function: it draws nothing. */
void VespoidAttack_Draw(ObjAttach *o)
{
}

/*
 * Sets the player's second trap (Item_UseMisc, items 0x54 and 0x299) 80 ahead
 * of pl in pl's area; kind is its type (0). A trap of the local player counts
 * in gScene.placed2 (stateVar1 1).
 */
void PitfallTrap_Create(Player *pl, s8 kind)
{
    ObjAttach *o;
    f32 ofs[3];

    if ((o = ObjAttach_Alloc(0)) != NULL) {
        o->kind = ATTACH_PITFALL_TRAP;
        o->update = PitfallTrap_Update;
        o->type = kind;
        ofs[0] = 0.0f;
        ofs[1] = 0.0f;
        ofs[2] = 80.0f;
        Vec_RotateY(ofs, (6.2831855f * pl->rot[1]) / 65536.0f);
        o->pos[0] = pl->pos[0] + ofs[0];
        o->pos[1] = pl->pos[1] + ofs[1];
        o->pos[2] = pl->pos[2] + ofs[2];
        o->owner = pl;
        o->idx = pl->index;
        o->area = pl->area;
        o->keep = 1;
        if (Player_IsLocal(pl)) {
            (gScene.placed2)++;
            o->stateVar1 = 1;
        }
    }
}

/*
 * The PitfallTrap's per-frame update, by ObjAttach.state; ATTACH_HIT and
 * state 4 run on.
 */
void PitfallTrap_Update(ObjAttach *o)
{
    switch (o->state) {
    case ATTACH_SETUP:
        PitfallTrap_Setup(o);
        break;
    case ATTACH_RUN:
        PitfallTrap_Run(o);
        break;
    case ATTACH_END:
        PitfallTrap_End(o);
        break;
    case ATTACH_HIT:
        PitfallTrap_Run(o);
        break;
    case 4:
        PitfallTrap_Run(o);
        break;
    case ATTACH_DEFLECTED:
        PitfallTrap_End(o);
        break;
    case ATTACH_DEFLECTED_AURA:
        PitfallTrap_End(o);
        break;
    case 7:
        PitfallTrap_End(o);
        break;
    case ATTACH_FREE:
        PitfallTrap_Free(o);
        
    default:
        break;
    }
}

/*
 * Sets the trap up: its row (type + 1, by ObjAttach_StartAttack), its hit
 * shapes and no attack flags. It clears its phase (stateVar0), frame count
 * (ownerMotion) and aux. Then it takes a DrawSlotB slot for PitfallTrap_Draw,
 * or frees the trap.
 */
void PitfallTrap_Setup(ObjAttach *o)
{
    o->state++;
    o->visible = 1;
    o->draw = NULL;
    o->step = 0;
    o->stateVar0 = 0;
    o->ownerMotion = 0;
    o->aux = 0;
    o->isEm = 0;
    Player_TestSetFlags(o, 0);
    ObjAttach_StartAttack(o, o->type + 1, &PitfallTrap_RowsPtr);
    o->hitList = PitfallTrap_Val088;
    o->hurtList = 0;
    o->drawSlot = DrawSlotB_Alloc();
    if (o->drawSlot != -1) {
        o->drawSlotBuf = DrawSlotB_Get(o->drawSlot);
        o->drawSlotBuf->owner = o;
        o->drawSlotBuf->draw = PitfallTrap_Draw;
    } else {
        PitfallTrap_Free(o);
    }
}

/*
 * The trap's frame, by phase (stateVar0); ownerMotion counts the frames. 0:
 * being set: smoke puffs and ticks, at frame 55 three stage effects and a
 * sound, at 60 ready. 1: opens (aux counts for PitfallTrap_Draw) until frame
 * 121. 2: armed until frame 9121. It catches the monster its hit touched
 * (setting Em.trapHit to 6), or a monster within Em_TrapReach(type, 700) whose
 * holdTimer is above 0 and that is not defeated or in an event. 3: 5 frames,
 * then phase 6 if nothing was caught (step 0). 4: up to 60 frames for the
 * monster's holdTimer. 5: holds until holdTimer runs out; once the monster is
 * defeated or in an event, it stops counting in gScene.placed2. 6: 5 frames,
 * then ATTACH_END. Every frame it queues its draw slot.
 */
void PitfallTrap_Run(ObjAttach *o)
{
    Em *target = o->targetEm;
    Em *em;
    s16 i;
    u8 found = 0;
    f32 dist;

    o->ownerMotion++;
    switch (o->stateVar0) {
    case 0:
        if (o->ownerMotion < 55) {
            if (o->area == gScene.area) {
                if ((o->ownerMotion & 7) == 0) {
                    HitSpark_SpawnRandomAngle(0xC000, 0, 2, o->pos);
                }
                if ((o->ownerMotion & 0xF) == 0) {
                    PitfallTrap_PlaySound(o, 0);
                }
            }
        } else if (o->ownerMotion == 55 && o->area == gScene.area) {
            DustCloud_CreateAtYaw(o->pos, 12, Rand_Next(1), 0.8f);
            AttackBurst_CreateAtScaled(o->pos, Rand_Next(1), 10, 0.4f);
            DustCloud_CreateAtYaw(o->pos, 20, Rand_Next(1), 2.0f);
            PitfallTrap_PlaySound(o, 1);
        }
        if (o->ownerMotion >= 60) {
            o->stateVar0++;
            PitfallTrap_PlaySound(o, 2);
        }
        break;
    case 1:
        o->aux++;
        o->hitFrames = 99;
        if (o->ownerMotion > 121) {
            o->stateVar0++;
        }
        break;
    case 2:
        if (o->ownerMotion >= 9121) {
            o->hitPhase = 0;
            o->state = ATTACH_END;
            return;
        }
        o->hitFrames = 99;
        if (o->didHit != 0) {
            found = 1;
        } else {
            for (em = gEms, i = 0; i < EM_COUNT; i++, em++) {
                if (em->active != 0 && em->area == o->area && (u32) (em->stateGroup - EM_GROUP_DEFEAT) > 1 && (s8) em->holdTimer > 0) {
                    dist = Vec_Distance(o->pos, &em->posX);
                    if (dist <= Em_TrapReach(em->type, 700.0f)) {
                        o->targetEm = em;
                        target = em;
                        found = 2;
                        break;
                    }
                }
            }
        }
        if (found) {
            if (target == NULL) {
                o->step = 0;
            } else {
                if (found == 1) {
                    target->trapHit = 6;
                }
                o->step = 1;
            }
            o->stateVar0++;
            o->aux = 0;
            o->hitPhase = 0;
        }
        break;
    case 3:
        if (++o->aux > 5) {
            if (o->step == 0) {
                o->stateVar0 = 6;
            } else {
                o->stateVar0++;
            }
            o->aux = 0;
        }
        break;
    case 4:
        if (++o->aux > 60) {
            o->hitPhase = 0;
            o->aux = 0;
            o->stateVar0 = 6;
            break;
        }
        if (target == NULL) {
            o->step = 0;
            o->stateVar0 = 6;
            o->hitPhase = 0;
            o->aux = 0;
            break;
        }
        if (target->active == 0) {
            o->step = 0;
            o->stateVar0 = 6;
            o->hitPhase = 0;
            o->aux = 0;
            break;
        }
        if ((s8) target->holdTimer != 0) {
            o->hitPhase = 0;
            o->aux = 0;
            o->stateVar0++;
        }
        break;
    case 5:
        if (target == NULL) {
            o->step = 0;
            o->stateVar0 = 6;
            o->hitPhase = 0;
            o->aux = 0;
            break;
        }
        if (target->active == 0) {
            o->step = 0;
            o->stateVar0 = 6;
            o->hitPhase = 0;
            o->aux = 0;
            break;
        }
        if (o->stateVar1 != 0 && (target->stateGroup == EM_GROUP_EVENT || target->stateGroup == EM_GROUP_DEFEAT)) {
            (gScene.placed2)--;
            o->stateVar1 = 0;
        }
        if ((s8) target->holdTimer <= 0) {
            o->hitPhase = 0;
            o->aux = 0;
            o->stateVar0++;
        }
        break;
    case 6:
        if (++o->aux > 5) {
            o->hitPhase = 0;
            o->state = ATTACH_END;
            return;
        }
        break;
    }
    o->drawSlotBuf->pos[0] = o->pos[0];
    o->drawSlotBuf->pos[1] = o->pos[1];
    o->drawSlotBuf->pos[2] = o->pos[2];
    OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
}

/*
 * Hides the trap, frees its DrawSlotB slot and frees it on the next update.
 */
void PitfallTrap_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = ATTACH_FREE;
    o->visible = 0;
    DrawSlotB_Free(o->drawSlot);
}

/*
 * Takes the trap off gScene.placed2 if it still counts there (stateVar1) and
 * frees it.
 */
void PitfallTrap_Free(ObjAttach *o)
{
    if (o->stateVar1 != 0) {
        gScene.placed2--;
    }
    ObjAttach_Free(o);
}

/* Draws model set `set` with matrix m, without lights. */
void PitfallTrap_DrawModel(ModelMatSet *set, f32 *mtx)
{
    Gs_SetState(0x1A, (s32) mtx);
    if (set != NULL && set->id != -1) {
        Draw_ApplyStates(set->flags);
        Model_Draw(set->id, 0);
    }
}

/*
 * Draws the trap in the current area. Its base (set 116 of gCommonModels[0])
 * is drawn until it catches (phase below 3). The area's pit models are
 * gAreaFxModels[area][0] and the two after it, in gAreaSubModels[0]. The
 * first two sink and flatten as aux counts in phases 1-2, with a scrolled
 * texture, and shrink in phase 3. The third grows in phase 3, stays in phases
 * 4-5 and shrinks in phase 6.
 */
void PitfallTrap_Draw(DrawSlot *slot)
{
    f32 mtx[16];
    f32 texMtx[16];
    ObjAttach *o = slot->owner;
    Model *areaModel = gAreaSubModels[0];
    ModelMat *tex;
    Model *commonModel = gCommonModels[0];
    s16 idx;
    f32 texOfsY;
    f32 scaleX;
    f32 scaleZ;
    f32 scaleY;
    s32 frames;

    if (o->area == gScene.area && areaModel != NULL && areaModel->active != 0) {
        switch (o->stateVar0) {
        case 0:
            break;
        case 1:
        case 2:
            frames = o->aux;
            scaleX = 1.0f;
            scaleZ = scaleX;
            if (frames < 15) {
                texOfsY = scaleX - 0.3f * (frames / 15.0f);
            } else if (frames < 30) {
                texOfsY = 0.7f - 0.2f * ((frames - 15) / 15.0f);
            } else {
                texOfsY = 0.5f;
            }
            if (frames > 61) {
                scaleY = 0.16f;
            } else if (frames > 15) {
                scaleY = 1.0f - 0.84f * Math_Sin((1.5707964f * (frames - 15)) / 46.0f);
            } else {
                scaleY = 1.0f;
            }
            break;
        case 3:
            frames = o->aux;
            scaleX = 1.0f - frames / 5.0f;
            scaleY = 0.16f * scaleX;
            texOfsY = 0.5f;
            scaleZ = scaleX;
            break;
        case 4:
        case 5:
            scaleX = 0.0f;
            scaleY = scaleX;
            scaleZ = scaleX;
            texOfsY = scaleX;
            break;
        case 6:
            frames = 5 - o->aux;
            scaleY = 0.0f;
            scaleZ = scaleY;
            texOfsY = scaleY;
            scaleX = 1.0f - frames / 5.0f;
            break;
        }
        idx = gAreaFxModels[o->area][0];
        Gs_SetState(0x67, -1);
        if (o->stateVar0 < 3) {
            tex = commonModel->mats;
            Gs_SetState(0x60, 0x80);
            Mtx_Identity(mtx);
            Mtx_SetTrans(mtx, o->pos[0], o->pos[1], o->pos[2]);
            Gs_SetLightList(tex, &commonModel->matSets[116]);
            PitfallTrap_DrawModel(&commonModel->matSets[116], mtx);
        }
        if (o->stateVar0 != 4 && o->stateVar0 != 5 && o->stateVar0 != 6) {
            Gs_SetState(0x60, 0);
            Mtx_Scale(mtx, scaleX, scaleY, scaleZ);
            Mtx_SetTrans(mtx, o->pos[0], o->pos[1], o->pos[2]);
            Mtx_Translation(texMtx, 0.0f, texOfsY, 0.0f);
            Gs_SetState(0x19, (s32) texMtx);
            PitfallTrap_DrawModel(&areaModel->matSets[idx], mtx);
            Mtx_Scale(mtx, scaleX, scaleY, scaleZ);
            Mtx_SetTrans(mtx, o->pos[0], 2.0f + o->pos[1], o->pos[2]);
            Gs_SetState(0x19, (s32) texMtx);
            PitfallTrap_DrawModel(&areaModel->matSets[idx] + 1, mtx);
        }
        Gs_SetState(0x60, 0x80);
        if ((u32) (o->stateVar0 - 3) <= 2 || o->stateVar0 == 6) {
            Mtx_Scale(mtx, 1.0f - scaleX, 1.0f - scaleX, 1.0f - scaleX);
            Mtx_SetTrans(mtx, o->pos[0], o->pos[1], o->pos[2]);
            PitfallTrap_DrawModel(&areaModel->matSets[idx] + 2, mtx);
        }
        Draw_SetDefaultState();
    }
}

/*
 * Plays the trap's sound `kind` at the trap, in the current area only: 0 the
 * setting tick (0x25), 1 the opening (0x26), 2 ready (0x46).
 */
void PitfallTrap_PlaySound(ObjAttach *o, s16 kind)
{
    if (o->area == gScene.area) {
        switch (kind) {
        case 0:
            Sound_PlaySeAtPosChecked(1, 0x25, 0, o->pos, 1, 0);
            break;
        case 1:
            Sound_PlaySeAtPosChecked(1, 0x26, 0, o->pos, 1, 0);
            break;
        case 2:
            Sound_PlaySeAtPosChecked(1, 0x46, 0, o->pos, 1, 0);
            break;
        }
    }
}

/*
 * Makes a VelocipreyAttack for em (the Velociprey family's states) with row kind of VelocipreyAttack_Rows, if a player is in em's
 * area. Clears em's hitPending and contactHit.
 */
void VelocipreyAttack_Create(Em *em, s8 kind)
{
    ObjAttach *o;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((o = ObjAttach_Alloc(0)) != NULL) {
            o->kind = ATTACH_VELOCIPREY_ATTACK;
            o->type = kind;
            o->update = VelocipreyAttack_Update;
            o->idx = em->index;
            o->isEm = em->isEm;
            o->ownerMotion = em->motionId[0];
            o->owner = em;
            o->yaw = em->rotY;
            o->rotX = em->rotX;
            o->rotY = em->rotY;
            o->rotZ = em->rotZ;
            o->side = *(u8 *) &em->side;
            em->hitPending = 0;
            em->contactHit = 0;
        }
    }
}

/* The VelocipreyAttack's per-frame update, by ObjAttach.state. */
void VelocipreyAttack_Update(ObjAttach *o)
{
    switch (o->state) {
    case ATTACH_SETUP:
        VelocipreyAttack_Setup(o);
        break;
    case ATTACH_RUN:
        VelocipreyAttack_Run(o);
        break;
    case ATTACH_END:
        VelocipreyAttack_End(o);
        break;
    case ATTACH_HIT:
        VelocipreyAttack_Run(o);
        break;
    case 4:
        VelocipreyAttack_End(o);
        break;
    case ATTACH_DEFLECTED:
        VelocipreyAttack_End(o);
        break;
    case ATTACH_DEFLECTED_AURA:
        VelocipreyAttack_End(o);
        break;
    case 7:
        VelocipreyAttack_End(o);
        break;
    case ATTACH_FREE:
        VelocipreyAttack_Free(o);
        
    default:
        break;
    }
}

/*
 * Sets the attack up from its row: the hit shapes and ATKF_OWNER_CONTACT.
 * hitDelay is counted from the monster's motion (type 3's from motion layer
 * 1).
 */
void VelocipreyAttack_Setup(ObjAttach *o)
{
    Em *em = &gEms[o->idx];
    s32 blendFrames;
    s32 startFrame;

    o->state++;
    o->visible = 1;
    o->draw = NULL;
    Player_TestSetFlags(o, ATKF_OWNER_CONTACT);
    ObjAttach_StartOwnedAttack(o, (s32) em, o->type, &VelocipreyAttack_RowsPtr);
    o->hitList = VelocipreyAttack_HitShapes[o->hitListNo];
    o->hurtList = 0;
    o->area = em->area;
    if (o->hitDelay != 0) {
        if (o->type == 3) {
            blendFrames = Math_Absf(em->motionBlend[1] % 100);
            startFrame = Math_Absf(em->motionFrame[1]);
        } else {
            blendFrames = Math_Absf(em->motionBlend[0] % 100);
            startFrame = Math_Absf(em->motionFrame[0]);
        }
        o->hitDelay += blendFrames - startFrame - 1;
        if ((s8) o->hitDelay < 0) {
            o->hitDelay = 0;
        }
    }
    o->drawSlotBuf = NULL;
}

/*
 * Ends the attack when the monster's motion changes or the hit window closes.
 */
void VelocipreyAttack_Run(ObjAttach *o)
{
    Em *em = &gEms[o->idx];

    if (em->motionId[0] != o->ownerMotion || o->hitPhase == 0) {
        o->hitPhase = 0;
        o->state = ATTACH_END;
    }
    if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
    }
}

/* Hides the attack, frees its draw slot and frees it on the next update. */
void VelocipreyAttack_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = ATTACH_FREE;
    o->visible = 0;
    if (o->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) o->drawSlot);
    }
}

/* Frees the object (ObjAttach_Free). */
void VelocipreyAttack_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

/*
 * Makes an EmBomb of type kind at em's bones (EmBomb_Follow), in em's area.
 * Types 0-3 are the Melynx's bombs (EmMelynx_*), 4 and 5 the Shakalaka's
 * (EmShakalaka_ThrowBomb). Clears em's hitPending and contactHit.
 */
void EmBomb_Create(Em *em, s8 kind)
{
    ObjAttach *o;

    if ((o = ObjAttach_Alloc(0)) != NULL) {
        o->kind = ATTACH_EM_BOMB;
        o->type = kind;
        o->update = EmBomb_Update;
        o->idx = em->index;
        o->isEm = em->isEm;
        o->owner = em;
        o->yaw = em->rotY;
        o->area = em->area;
        EmBomb_Follow(o, em);
        o->keep = 0;
        em->hitPending = 0;
        em->contactHit = 0;
        o->side = *(u8 *) &em->side;
    }
}

/*
 * Puts the bomb on em: type 4 at bone 6, the others midway between bones 6
 * and 9, 20 higher (type 1: 20 times its size). It takes em's rotation.
 */
void EmBomb_Follow(ObjAttach *o, Em *em)
{
    f32 bonePos[3];
    f32 otherBonePos[3];

    if (o->type == 4) {
        Em_BonePos(em, 6, bonePos);
        o->pos[0] = bonePos[0];
        o->pos[1] = bonePos[1];
        o->pos[2] = bonePos[2];
    } else {
        Em_BonePos(em, 6, bonePos);
        Em_BonePos(em, 9, otherBonePos);
        o->pos[0] = (bonePos[0] + otherBonePos[0]) / 2.0f;
        switch (o->type) {
        case 1:
            o->pos[1] = 20.0f * o->velX + (bonePos[1] + otherBonePos[1]) / 2.0f;
            break;
        default:
            o->pos[1] = 20.0f + (bonePos[1] + otherBonePos[1]) / 2.0f;
            break;
        }
        o->pos[2] = (bonePos[2] + otherBonePos[2]) / 2.0f;
    }
    o->rotX = em->rotX;
    o->rotY = em->rotY;
    o->rotZ = em->rotZ;
}

/*
 * The EmBomb's per-frame update, by ObjAttach.state. Any state after
 * ATTACH_RUN ends it.
 */
void EmBomb_Update(ObjAttach *o)
{
    switch (o->state) {
    case ATTACH_SETUP:
        EmBomb_Setup(o);
        break;
    case ATTACH_RUN:
        EmBomb_Run(o);
        break;
    case ATTACH_END:
        EmBomb_End(o);
        break;
    case ATTACH_HIT:
        EmBomb_End(o);
        break;
    case 4:
        EmBomb_End(o);
        break;
    case ATTACH_DEFLECTED:
        EmBomb_End(o);
        break;
    case ATTACH_DEFLECTED_AURA:
        EmBomb_End(o);
        break;
    case 7:
        EmBomb_End(o);
        break;
    case ATTACH_FREE:
        EmBomb_Free(o);
        
    default:
        break;
    }
}

/*
 * Sets the bomb's counters. aux (the blink gap) is 340. ownerMotion (the
 * frames it grows in) is 10 for types 4 and 5, else 30. velX (its size) is 0.
 */
void EmBomb_InitTimers(ObjAttach *o)
{
    o->hitPhase = 0;
    o->aux = 0x154;
    if (o->type == 4 || o->type == 5) {
        o->ownerMotion = 10;
    } else {
        o->ownerMotion = 30;
    }
    o->velX = 0.0f;
}

/*
 * Types 0 and 1, the bomb a Melynx holds (EmMelynx_HoldBomb, EmMelynx_HoldBigBomb). It grows
 * to size 1 (type 1: 3); at frame 60 its fuse is lit, then it smokes, ticks
 * every 15 frames and blinks ever faster. At frame 340 it bursts in the
 * current area: a StageObj_K0E and an Explosion of kind 0 (type 1: kind 2).
 * If the Melynx lets go first (EmMelynx_BombState not 1), it vanishes
 * while still growing, or bursts at once after that.
 */
void EmBomb_RunHeld(ObjAttach *o)
{
    Em *em = o->owner;

    switch (o->step) {
    case 0:
        if (o->type == 1) {
            o->velX += 0.1f;
        } else {
            o->velX += 0.033333335f;
        }
        if (--o->ownerMotion <= 0) {
            o->step++;
            if (o->type == 1) {
                o->velX = 3.0f;
            } else {
                o->velX = 1.0f;
            }
        }
        if (EmMelynx_BombState(em) != 1) {
            o->visible ^= 1;
            o->state++;
            return;
        }
        break;
    case 1:
        if (o->ownerMotion >= 60) {
            o->step++;
            o->stateVar0 = 15;
            if (o->area == gScene.area) {
                Sound_PlaySeAtPosChecked(1, 0x24, 0, o->pos, 1, 0);
            }
        }
    case 2:
        if (++o->ownerMotion >= 340 || EmMelynx_BombState(em) != 1) {
            if (o->area == gScene.area) {
                FireBlast_CreateScaled(o->pos, 0, 1.0f);
                if (o->type == 1) {
                    Explosion_CreateAt(o->pos, EXPLOSION_LARGE_BARREL_BOMB, o->area, 0);
                } else {
                    Explosion_CreateAt(o->pos, EXPLOSION_BOMB, o->area, 0);
                }
            }
            o->visible ^= 1;
            o->state++;
            return;
        }
        if (o->ownerMotion >= 320) {
            o->visible ^= 1;
            break;
        }
        if (o->stateVar1 == 0) {
            if (!(0.8f * o->aux <= 320 - o->ownerMotion)) {
                o->visible ^= 1;
                if (320 - o->ownerMotion > 2) {
                    o->stateVar1 = 2;
                }
            }
        } else if (--o->stateVar1 == 0) {
            o->visible ^= 1;
            o->aux = 320 - o->ownerMotion;
        }
        if (o->area == gScene.area && o->ownerMotion >= 60) {
            if (o->ownerMotion & 1) {
                HitSpark_SpawnRandomAngle(0xC000, 0, 2, o->pos);
            }
            if (--o->stateVar0 <= 0) {
                o->stateVar0 = 15;
                Sound_PlaySeAtPosChecked(1, 0x25, 0, o->pos, 1, 0);
            }
        }
        break;
    }
    EmBomb_Follow(o, em);
}

/*
 * Types 2 and 3, the bomb a Melynx throws (EmMelynx_ThrowBomb: type 2 in
 * state 4, else 3). It grows at em's bones. Once let go
 * (EmMelynx_BombState 0), it flies forward (type 2: 10 a frame and 20
 * up; type 3: 15 and 25; gravity -1). It bursts (Explosion kind 0) at the
 * floor or after 120 frames. On any other result it vanishes.
 */
void EmBomb_RunMelynxThrown(ObjAttach *o)
{
    Em *em = o->owner;
    f32 localVel[3];
    f32 move[3];
    s32 ang[3];
    f32 mat[16];
    f32 floorY;

    switch (o->step) {
    case 0:
        o->velX += 0.033333335f;
        if (--o->ownerMotion <= 0) {
            o->step++;
            o->velX = 1.0f;
        }
        if (EmMelynx_BombState(em) != 1) {
            o->visible ^= 1;
            o->state++;
        } else {
            EmBomb_Follow(o, em);
        }
        break;
    case 1:
        switch (EmMelynx_BombState(em)) {
        case 0:
            o->step++;
            if (o->type == 2) {
                o->velZ = 10.0f;
                o->velY = 20.0f;
            } else {
                o->velZ = 15.0f;
                o->velY = 25.0f;
            }
            o->gravity = -1.0f;
            o->ownerMotion = 120;
            goto follow;
        default:
            o->visible ^= 1;
            o->state++;
            break;
        case 1:
        follow:
            EmBomb_Follow(o, em);
            break;
        }
        break;
    case 2:
        ang[0] = 0;
        ang[1] = o->rotY;
        ang[2] = 0;
        Mtx_FromAnglesYXZ(ang, mat);
        Vec_Set(localVel, 0.0f, 0.0f, o->velZ);
        Vec_Rotate(move, localVel, mat);
        o->pos[0] += move[0];
        o->pos[2] += move[2];
        o->pos[1] += o->velY;
        o->velY += o->gravity;
        floorY = AreaGrid2_GetFloorY(o->pos);
        if (--o->ownerMotion <= 0 || o->pos[1] < floorY) {
            if (o->area == gScene.area) {
                FireBlast_CreateScaled(o->pos, 0, 1.0f);
                Explosion_CreateAt(o->pos, EXPLOSION_BOMB, o->area, 0);
            }
            o->visible ^= 1;
            o->state++;
            return;
        }
        break;
    }
}

/*
 * Types 4 and 5, the bomb a Shakalaka throws (EmShakalaka_ThrowBomb). It
 * works as EmBomb_RunMelynxThrown but asks EmShakalaka_BombState and grows over 10
 * frames. Type 5 flies flat and fast (20 forward, 10 up) and bursts with a
 * StageObj_K0C and an Explosion of kind 0x1A owned by em. Type 4 (15, 25)
 * bursts with kind 0.
 */
void EmBomb_RunShakalakaThrown(ObjAttach *o)
{
    Em *em = o->owner;
    f32 localVel[3];
    f32 move[3];
    s32 ang[3];
    f32 mat[16];
    f32 floorY;

    switch (o->step) {
    case 0:
        o->velX += 0.05f;
        if (--o->ownerMotion <= 0) {
            o->step++;
            o->velX = 1.0f;
        }
        if (EmShakalaka_BombState(em) != 1) {
            o->visible ^= 1;
            o->state++;
        } else {
            EmBomb_Follow(o, em);
        }
        break;
    case 1:
        switch (EmShakalaka_BombState(em)) {
        case 0:
            o->step++;
            if (o->type == 5) {
                o->velZ = 20.0f;
                o->velY = 10.0f;
            } else {
                o->velZ = 15.0f;
                o->velY = 25.0f;
            }
            o->gravity = -1.0f;
            o->ownerMotion = 120;
            goto follow;
        default:
            o->visible ^= 1;
            o->state++;
            break;
        case 1:
        follow:
            EmBomb_Follow(o, em);
            break;
        }
        break;
    case 2:
        ang[0] = 0;
        ang[1] = o->rotY;
        ang[2] = 0;
        Mtx_FromAnglesYXZ(ang, mat);
        Vec_Set(localVel, 0.0f, 0.0f, o->velZ);
        Vec_Rotate(move, localVel, mat);
        o->pos[0] += move[0];
        o->pos[2] += move[2];
        o->pos[1] += o->velY;
        o->velY += o->gravity;
        floorY = AreaGrid2_GetFloorY(o->pos);
        if (--o->ownerMotion <= 0 || o->pos[1] < floorY) {
            if (o->area == gScene.area) {
                if (o->type == 5) {
                    ItemProp_Create(o->pos, 0, ITEMPROP_CLOUD, 4);
                    Explosion_CreateOwned((Player *)em, o->pos, EXPLOSION_THROWN_CLOUD_B);
                } else {
                    FireBlast_CreateScaled(o->pos, 0, 1.0f);
                    Explosion_CreateAt(o->pos, EXPLOSION_BOMB, o->area, 0);
                }
            }
            o->visible ^= 1;
            o->state++;
            return;
        }
        break;
    }
}

/*
 * Sets the bomb up (EmBomb_InitTimers), takes a DrawSlot for EmBomb_Draw and
 * runs its first frame. Frees the bomb if no slot is free.
 */
void EmBomb_Setup(ObjAttach *o)
{
    o->state++;
    o->visible = 1;
    o->draw = NULL;
    EmBomb_InitTimers(o);
    o->drawSlot = DrawSlot_Alloc();
    if (o->drawSlot != -1) {
        o->drawSlotBuf = DrawSlot_Get(o->drawSlot);
        o->drawSlotBuf->owner = o;
        o->drawSlotBuf->draw = EmBomb_Draw;
        EmBomb_Run(o);
    } else {
        ObjAttach_Free(o);
    }
}

/*
 * Runs the bomb by type (0-1, 2-3, 4-5) and queues its draw slot at its
 * position.
 */
void EmBomb_Run(ObjAttach *o)
{
    switch (o->type) {
    case 0:
    case 1:
    default:
        EmBomb_RunHeld(o);
        break;
    case 2:
    case 3:
        EmBomb_RunMelynxThrown(o);
        break;
    case 4:
    case 5:
        EmBomb_RunShakalakaThrown(o);
        break;
    }
    if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
    }
}

/* Hides the bomb, frees its draw slot and frees it on the next update. */
void EmBomb_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = ATTACH_FREE;
    o->visible = 0;
    if (o->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) o->drawSlot);
    }
}

/* Frees the object (ObjAttach_Free). */
void EmBomb_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

/*
 * Draws the bomb while visible in the current area: model set
 * EmBomb_Models[type] of gCommonModels[0], at size velX, turned by rotY. For
 * the barrel set (0x37) it hides material 1.
 */
void EmBomb_Draw(DrawSlot *slot)
{
    f32 mtx[16];
    ObjAttach *o = slot->owner;
    Model *set = gCommonModels[0];
    ModelMat *tex;
    ModelMatSet *mdl;
    s16 idx;
    s32 i;
    s32 *ids;
    ModelMat *mat;

    if (o->visible != 0 && o->area == gScene.area && set != NULL && set->active != 0) {
        tex = set->mats;
        Mtx_Scale(mtx, o->velX, o->velX, o->velX);
        Mtx_SetTrans(mtx, slot->pos[0], slot->pos[1], slot->pos[2]);
        Mtx_RotateXYZ(mtx, 0.0f, (6.2831855f * o->rotY) / 65536.0f, 0.0f);
        idx = EmBomb_Models[o->type];
        mdl = &set->matSets[idx];
        if (idx == 0x37) {
            i = 0;
            if (i < mdl->count) {
                ids = (s32 *) mdl;
                do {
                    mat = &tex[ids[2]];
                    if (i == 1) {
                        mat->alpha = 0.0f;
                    }
                    i++;
                    ids++;
                } while (i < mdl->count);
            }
        }
        Gs_SetState(0x1A, (s32) mtx);
        if (mdl != NULL && mdl->id != -1) {
            Gs_SetLightList(tex, mdl);
            Draw_ApplyStates(mdl->flags);
            Model_Draw(mdl->id, 0);
        }
        Draw_SetDefaultState();
    }
}

/*
 * Makes a DromeAttack for em (Velocidrome, Gendrome, Iodrome)
 * with row kind of DromeAttack_Rows, if a player is in em's area. Clears em's
 * hitPending and contactHit.
 */
void DromeAttack_Create(Em *em, s8 kind)
{
    ObjAttach *o;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((o = ObjAttach_Alloc(0)) != NULL) {
            o->kind = ATTACH_DROME_ATTACK;
            o->type = kind;
            o->update = DromeAttack_Update;
            o->idx = em->index;
            o->isEm = em->isEm;
            o->ownerMotion = em->motionId[0];
            o->owner = em;
            o->yaw = em->rotY;
            o->rotX = em->rotX;
            o->rotY = em->rotY;
            o->rotZ = em->rotZ;
            o->side = *(u8 *) &em->side;
            em->hitPending = 0;
            em->contactHit = 0;
        }
    }
}

/* The DromeAttack's per-frame update, by ObjAttach.state. */
void DromeAttack_Update(ObjAttach *o)
{
    switch (o->state) {
    case ATTACH_SETUP:
        DromeAttack_Setup(o);
        break;
    case ATTACH_RUN:
        DromeAttack_Run(o);
        break;
    case ATTACH_END:
        DromeAttack_End(o);
        break;
    case ATTACH_HIT:
        DromeAttack_Run(o);
        break;
    case 4:
        DromeAttack_End(o);
        break;
    case ATTACH_DEFLECTED:
        DromeAttack_End(o);
        break;
    case ATTACH_DEFLECTED_AURA:
        DromeAttack_End(o);
        break;
    case 7:
        DromeAttack_End(o);
        break;
    case ATTACH_FREE:
        DromeAttack_Free(o);
        
    default:
        break;
    }
}

/*
 * Sets the attack up from its row: the hit shapes and ATKF_OWNER_CONTACT.
 * hitDelay is counted from the monster's motion (type 3's from motion layer
 * 1).
 */
void DromeAttack_Setup(ObjAttach *o)
{
    Em *em = &gEms[o->idx];
    s32 blendFrames;
    s32 startFrame;

    o->state++;
    o->visible = 1;
    o->draw = NULL;
    Player_TestSetFlags(o, ATKF_OWNER_CONTACT);
    ObjAttach_StartOwnedAttack(o, (s32) em, o->type, &DromeAttack_RowsPtr);
    o->hitList = DromeAttack_HitShapes[o->hitListNo];
    o->hurtList = 0;
    o->area = em->area;
    if (o->hitDelay != 0) {
        if (o->type == 3) {
            blendFrames = Math_Absf(em->motionBlend[1] % 100);
            startFrame = Math_Absf(em->motionFrame[1]);
        } else {
            blendFrames = Math_Absf(em->motionBlend[0] % 100);
            startFrame = Math_Absf(em->motionFrame[0]);
        }
        o->hitDelay += blendFrames - startFrame - 1;
        if ((s8) o->hitDelay < 0) {
            o->hitDelay = 0;
        }
    }
    o->drawSlotBuf = NULL;
}

/*
 * Ends the attack when the monster's motion changes or the hit window closes.
 */
void DromeAttack_Run(ObjAttach *o)
{
    Em *em = &gEms[o->idx];

    if (em->motionId[0] != o->ownerMotion || o->hitPhase == 0) {
        o->hitPhase = 0;
        o->state = ATTACH_END;
    }
    if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
    }
}

/* Hides the attack, frees its draw slot and frees it on the next update. */
void DromeAttack_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = ATTACH_FREE;
    o->visible = 0;
    if (o->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) o->drawSlot);
    }
}

/* Frees the object (ObjAttach_Free). */
void DromeAttack_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

/*
 * Makes a KhezuAttack for em (Khezu and Red Khezu, EmKhezu_MotionEffects)
 * with row kind of KhezuAttack_Rows, if a player is in em's area. Clears em's
 * hitPending and contactHit.
 */
void KhezuAttack_Create(Em *em, s8 kind)
{
    ObjAttach *o;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((o = ObjAttach_Alloc(0)) != NULL) {
            o->kind = ATTACH_KHEZU_ATTACK;
            o->type = kind;
            o->update = KhezuAttack_Update;
            o->idx = em->index;
            o->isEm = em->isEm;
            o->ownerMotion = em->motionId[0];
            o->owner = em;
            o->yaw = em->rotY;
            o->rotX = em->rotX;
            o->rotY = em->rotY;
            o->rotZ = em->rotZ;
            o->side = *(u8 *) &em->side;
            em->hitPending = 0;
            em->contactHit = 0;
        }
    }
}

/* The KhezuAttack's per-frame update, by ObjAttach.state. */
void KhezuAttack_Update(ObjAttach *o)
{
    switch (o->state) {
    case ATTACH_SETUP:
        KhezuAttack_Setup(o);
        break;
    case ATTACH_RUN:
        KhezuAttack_Run(o);
        break;
    case ATTACH_END:
        KhezuAttack_End(o);
        break;
    case ATTACH_HIT:
        KhezuAttack_Run(o);
        break;
    case 4:
        KhezuAttack_End(o);
        break;
    case ATTACH_DEFLECTED:
        KhezuAttack_End(o);
        break;
    case ATTACH_DEFLECTED_AURA:
        KhezuAttack_End(o);
        break;
    case 7:
        KhezuAttack_End(o);
        break;
    case ATTACH_FREE:
        KhezuAttack_Free(o);
        
    default:
        break;
    }
}

/*
 * Sets the attack up from its row: the hit shapes and ATKF_OWNER_CONTACT.
 * hitDelay is counted from the monster's motion (type 3's from motion layer
 * 1).
 */
void KhezuAttack_Setup(ObjAttach *o)
{
    Em *em = &gEms[o->idx];
    s32 blendFrames;
    s32 startFrame;

    o->state++;
    o->visible = 1;
    o->draw = NULL;
    Player_TestSetFlags(o, ATKF_OWNER_CONTACT);
    ObjAttach_StartOwnedAttack(o, (s32) em, o->type, &KhezuAttack_RowsPtr);
    o->hitList = KhezuAttack_HitShapes[o->hitListNo];
    o->hurtList = 0;
    o->area = em->area;
    if (o->hitDelay != 0) {
        if (o->type == 3) {
            blendFrames = Math_Absf(em->motionBlend[1] % 100);
            startFrame = Math_Absf(em->motionFrame[1]);
        } else {
            blendFrames = Math_Absf(em->motionBlend[0] % 100);
            startFrame = Math_Absf(em->motionFrame[0]);
        }
        o->hitDelay += blendFrames - startFrame - 1;
        if ((s8) o->hitDelay < 0) {
            o->hitDelay = 0;
        }
    }
    o->drawSlotBuf = NULL;
}

/*
 * Ends the attack when the monster's motion changes or the hit window closes;
 * types 29 and 31 keep their window open.
 */
void KhezuAttack_Run(ObjAttach *o)
{
    Em *em = &gEms[o->idx];

    switch (o->type) {
    case 29:
    case 31:
        o->hitFrames = 99;
        break;
    }
    if (em->motionId[0] != o->ownerMotion || o->hitPhase == 0) {
        o->hitPhase = 0;
        o->state = ATTACH_END;
    }
    if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
    }
}

/* Hides the attack, frees its draw slot and frees it on the next update. */
void KhezuAttack_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = ATTACH_FREE;
    o->visible = 0;
    if (o->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) o->drawSlot);
    }
}

/* Frees the object (ObjAttach_Free). */
void KhezuAttack_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

/*
 * Makes a KutkuAttack for em (EmKutku_MotionEffects, EmKutku_ChargeTrip) with
 * row kind of KutkuAttack_Rows, if a player is in em's area. Clears em's
 * hitPending and contactHit.
 */
void KutkuAttack_Create(Em *em, s8 kind)
{
    ObjAttach *o;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((o = ObjAttach_Alloc(0)) != NULL) {
            o->kind = ATTACH_KUTKU_ATTACK;
            o->type = kind;
            o->update = KutkuAttack_Update;
            o->idx = em->index;
            o->isEm = em->isEm;
            o->ownerMotion = em->motionId[0];
            o->owner = em;
            o->yaw = em->rotY;
            o->rotX = em->rotX;
            o->rotY = em->rotY;
            o->rotZ = em->rotZ;
            o->side = *(u8 *) &em->side;
            em->hitPending = 0;
            em->contactHit = 0;
        }
    }
}

/*
 * Makes a KutkuAttack as KutkuAttack_Create does, but with one block of the
 * object buffer (ObjAttach_Alloc(1)) to hold the flash's EmObj. Its only
 * caller makes type 22, the flash (EmKutku_MotionEffects).
 */
void KutkuAttack_CreateFlash(Em *em, s8 kind)
{
    ObjAttach *o;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((o = ObjAttach_Alloc(1)) != NULL) {
            o->kind = ATTACH_KUTKU_ATTACK;
            o->type = kind;
            o->update = KutkuAttack_Update;
            o->idx = em->index;
            o->isEm = em->isEm;
            o->ownerMotion = em->motionId[0];
            o->owner = em;
            o->yaw = em->rotY;
            o->rotX = em->rotX;
            o->rotY = em->rotY;
            o->rotZ = em->rotZ;
            o->side = *(u8 *) &em->side;
            em->hitPending = 0;
            em->contactHit = 0;
        }
    }
}

/* The KutkuAttack's per-frame update, by ObjAttach.state. */
void KutkuAttack_Update(ObjAttach *o)
{
    switch (o->state) {
    case ATTACH_SETUP:
        KutkuAttack_Setup(o);
        break;
    case ATTACH_RUN:
        KutkuAttack_Run(o);
        break;
    case ATTACH_END:
        KutkuAttack_End(o);
        break;
    case ATTACH_HIT:
        KutkuAttack_Run(o);
        break;
    case 4:
        KutkuAttack_End(o);
        break;
    case ATTACH_DEFLECTED:
        KutkuAttack_End(o);
        break;
    case ATTACH_DEFLECTED_AURA:
        KutkuAttack_End(o);
        break;
    case 7:
        KutkuAttack_End(o);
        break;
    case ATTACH_FREE:
        KutkuAttack_Free(o);
        
    default:
        break;
    }
}

/*
 * Sets the attack up from its row: the hit shapes and ATKF_OWNER_CONTACT.
 * hitDelay is counted from the monster's motion. Type 22 also adds its buffer
 * as a flash (FlashObj_Add) at the monster, radius 2000, for 5 frames.
 */
void KutkuAttack_Setup(ObjAttach *o)
{
    Em *em = &gEms[o->idx];
    s32 blendFrames;
    EmObj *flash;

    o->state++;
    o->visible = 1;
    o->draw = NULL;
    Player_TestSetFlags(o, ATKF_OWNER_CONTACT);
    ObjAttach_StartOwnedAttack(o, (s32) em, o->type, &KutkuAttack_RowsPtr);
    o->hitList = KutkuAttack_HitShapes[o->hitListNo];
    o->hurtList = 0;
    o->area = em->area;
    if (o->hitDelay != 0) {
        blendFrames = Math_Absf(em->motionBlend[0] % 100);
        o->hitDelay += blendFrames - (s32) Math_Absf(em->motionFrame[0]) - 1;
        if ((s8) o->hitDelay < 0) {
            o->hitDelay = 0;
        }
    }
    o->drawSlotBuf = NULL;
    if (o->type == 22) {
        flash = (EmObj *) o->buf;
        Vec_Copy(flash->pos, &em->posX);
        flash->radius = 2000.0f;
        flash->goalArg = 0;
        flash->timer = 5;
        flash->area = o->area;
        FlashObj_Add(flash);
    }
}

/*
 * Ends the attack when the monster's motion changes or the hit window closes;
 * types 19, 23 and 28 keep their window open. Type 22 refills its flash's
 * EmObj.timer every frame and removes the flash when it ends.
 */
void KutkuAttack_Run(ObjAttach *o)
{
    Em *em = &gEms[o->idx];

    switch (o->type) {
    case 19:
    case 23:
    case 28:
        o->hitFrames = 99;
        break;
    }
    if (o->type == 22) {
        o->buf[0x14] = 5;
    }
    if (em->motionId[0] != o->ownerMotion || o->hitPhase == 0) {
        o->hitPhase = 0;
        o->state = ATTACH_END;
        if (o->type == 22) {
            FlashObj_Remove((EmObj *)o->buf);
        }
    }
    if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
    }
}

/* Hides the attack, frees its draw slot and frees it on the next update. */
void KutkuAttack_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = ATTACH_FREE;
    o->visible = 0;
    if (o->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) o->drawSlot);
    }
}

/* Frees the object (ObjAttach_Free). */
void KutkuAttack_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

/*
 * Makes a DiablosAttack for em (Diablos, Monoblos and their subspecies,
 * EmDiablos_MotionEffects) with row kind of DiablosAttack_Rows, if a player
 * is in em's area. Clears em's hitPending and contactHit.
 */
void DiablosAttack_Create(Em *em, s8 kind)
{
    ObjAttach *o;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((o = ObjAttach_Alloc(0)) != NULL) {
            o->kind = ATTACH_DIABLOS_ATTACK;
            o->type = kind;
            o->update = DiablosAttack_Update;
            o->idx = em->index;
            o->isEm = em->isEm;
            o->ownerMotion = em->motionId[0];
            o->owner = em;
            o->yaw = em->rotY;
            o->rotX = em->rotX;
            o->rotY = em->rotY;
            o->rotZ = em->rotZ;
            o->side = *(u8 *) &em->side;
            em->hitPending = 0;
            em->contactHit = 0;
        }
    }
}

/* The DiablosAttack's per-frame update, by ObjAttach.state. */
void DiablosAttack_Update(ObjAttach *o)
{
    switch (o->state) {
    case ATTACH_SETUP:
        DiablosAttack_Setup(o);
        break;
    case ATTACH_RUN:
        DiablosAttack_Run(o);
        break;
    case ATTACH_END:
        DiablosAttack_End(o);
        break;
    case ATTACH_HIT:
        DiablosAttack_Run(o);
        break;
    case 4:
        DiablosAttack_End(o);
        break;
    case ATTACH_DEFLECTED:
        DiablosAttack_End(o);
        break;
    case ATTACH_DEFLECTED_AURA:
        DiablosAttack_End(o);
        break;
    case 7:
        DiablosAttack_End(o);
        break;
    case ATTACH_FREE:
        DiablosAttack_Free(o);
        
    default:
        break;
    }
}

/*
 * Sets the attack up from its row: the hit shapes and ATKF_OWNER_CONTACT.
 * hitDelay is counted from the monster's motion.
 */
void DiablosAttack_Setup(ObjAttach *o)
{
    Em *em = &gEms[o->idx];
    s32 blendFrames;

    o->state++;
    o->visible = 1;
    o->draw = NULL;
    Player_TestSetFlags(o, ATKF_OWNER_CONTACT);
    ObjAttach_StartOwnedAttack(o, (s32) em, o->type, &DiablosAttack_RowsPtr);
    o->hitList = DiablosAttack_HitShapes[o->hitListNo];
    o->hurtList = 0;
    o->area = em->area;
    if (o->hitDelay != 0) {
        blendFrames = Math_Absf(em->motionBlend[0] % 100);
        o->hitDelay += blendFrames - (s32) Math_Absf(em->motionFrame[0]) - 1;
        if ((s8) o->hitDelay < 0) {
            o->hitDelay = 0;
        }
    }
    o->drawSlotBuf = NULL;
}

/*
 * Ends the attack when the monster's motion changes or the hit window closes;
 * type 14 keeps its window open.
 */
void DiablosAttack_Run(ObjAttach *o)
{
    Em *em = &gEms[o->idx];

    switch (o->type) {
    case 14:
        o->hitFrames = 99;
        break;
    }
    if (em->motionId[0] != o->ownerMotion || o->hitPhase == 0) {
        o->hitPhase = 0;
        o->state = ATTACH_END;
    }
    if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
    }
}

/* Hides the attack, frees its draw slot and frees it on the next update. */
void DiablosAttack_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = ATTACH_FREE;
    o->visible = 0;
    if (o->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) o->drawSlot);
    }
}

/* Frees the object (ObjAttach_Free). */
void DiablosAttack_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

/*
 * Makes a GraviosAttack for em (Gravios, Black Gravios, Basarios:
 * EmGravios_MotionEffects, EmGravios_Gas) with row kind of
 * GraviosAttack_Rows, if a player is in em's area. Types 4 and 22 face
 * backward (yaw + 0x8000). Clears em's hitPending and contactHit.
 */
void GraviosAttack_Create(Em *em, s8 kind)
{
    ObjAttach *o;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((o = ObjAttach_Alloc(0)) != NULL) {
            o->kind = ATTACH_GRAVIOS_ATTACK;
            o->type = kind;
            o->update = GraviosAttack_Update;
            o->idx = em->index;
            o->isEm = em->isEm;
            o->ownerMotion = em->motionId[0];
            o->owner = em;
            o->yaw = em->rotY;
            switch (o->type) {
            case 4:
            case 22:
                *(u16 *) &o->yaw += 0x8000;
                break;
            }
            o->rotX = em->rotX;
            o->rotY = em->rotY;
            o->rotZ = em->rotZ;
            o->side = *(u8 *) &em->side;
            em->hitPending = 0;
            em->contactHit = 0;
        }
    }
}

/* The GraviosAttack's per-frame update, by ObjAttach.state. */
void GraviosAttack_Update(ObjAttach *o)
{
    switch (o->state) {
    case ATTACH_SETUP:
        GraviosAttack_Setup(o);
        break;
    case ATTACH_RUN:
        GraviosAttack_Run(o);
        break;
    case ATTACH_END:
        GraviosAttack_End(o);
        break;
    case ATTACH_HIT:
        GraviosAttack_Run(o);
        break;
    case 4:
        GraviosAttack_End(o);
        break;
    case ATTACH_DEFLECTED:
        GraviosAttack_End(o);
        break;
    case ATTACH_DEFLECTED_AURA:
        GraviosAttack_End(o);
        break;
    case 7:
        GraviosAttack_End(o);
        break;
    case ATTACH_FREE:
        GraviosAttack_Free(o);
        
    default:
        break;
    }
}

/*
 * Sets the attack up from its row: the hit shapes and ATKF_OWNER_CONTACT.
 * hitDelay is counted from the monster's motion (type 3's from motion layer
 * 1).
 */
void GraviosAttack_Setup(ObjAttach *o)
{
    Em *em = &gEms[o->idx];
    s32 blendFrames;
    s32 startFrame;

    o->state++;
    o->visible = 1;
    o->draw = NULL;
    Player_TestSetFlags(o, ATKF_OWNER_CONTACT);
    ObjAttach_StartOwnedAttack(o, (s32) em, o->type, &GraviosAttack_RowsPtr);
    o->hitList = GraviosAttack_HitShapes[o->hitListNo];
    o->hurtList = 0;
    o->area = em->area;
    if (o->hitDelay != 0) {
        if (o->type == 3) {
            blendFrames = Math_Absf(em->motionBlend[1] % 100);
            startFrame = Math_Absf(em->motionFrame[1]);
        } else {
            blendFrames = Math_Absf(em->motionBlend[0] % 100);
            startFrame = Math_Absf(em->motionFrame[0]);
        }
        o->hitDelay += blendFrames - startFrame - 1;
        if ((s8) o->hitDelay < 0) {
            o->hitDelay = 0;
        }
    }
    o->drawSlotBuf = NULL;
}

/*
 * Types 11 and 27 (motion 0x3F9: 11 for Gravios and Black Gravios, 27 for
 * Basarios) keep their window at 60 frames while the monster plays 0x3F9.
 * Types 12 and 14 (EmGravios_Gas) keep theirs at 18 frames while it
 * plays 0x457. These four end only when the window closes or the monster is
 * in group 4 or 5. The others end when the motion changes or the window
 * closes.
 */
void GraviosAttack_Run(ObjAttach *o)
{
    Em *em = &gEms[o->idx];

    switch (o->type) {
    case 11:
    case 27:
        if (em->motionId[0] == EM_MOTION_ID(0, 0x11)) {
            o->hitFrames = 60;
        }
        break;
    case 12:
    case 14:
        if (em->motionId[0] == EM_MOTION_ID(0, 0x6F)) {
            o->hitFrames = 18;
        }
        break;
    }
    if (o->type == 11 || o->type == 27) {
        if (o->hitPhase == 0 || em->stateGroup == EM_GROUP_REACT || em->stateGroup == EM_GROUP_DEFEAT) {
            o->hitPhase = 0;
            o->state = ATTACH_END;
            return;
        }
    } else if (o->type == 12 || o->type == 14) {
        if (o->hitPhase == 0 || em->stateGroup == EM_GROUP_REACT || em->stateGroup == EM_GROUP_DEFEAT) {
            o->hitPhase = 0;
            o->state = ATTACH_END;
        }
    } else {
        if (em->motionId[0] != o->ownerMotion || o->hitPhase == 0) {
            o->hitPhase = 0;
            o->state = ATTACH_END;
        }
    }
    if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
    }
}

/* Hides the attack, frees its draw slot and frees it on the next update. */
void GraviosAttack_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = ATTACH_FREE;
    o->visible = 0;
    if (o->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) o->drawSlot);
    }
}

/* Frees the object (ObjAttach_Free). */
void GraviosAttack_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

/*
 * Makes a PlesiothAttack for em (Plesioth and Green Plesioth,
 * EmPlesioth_MotionEffects) with row kind of PlesiothAttack_Rows, if a player
 * is in em's area. Clears em's hitPending and contactHit.
 */
void PlesiothAttack_Create(Em *em, s8 kind)
{
    ObjAttach *o;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((o = ObjAttach_Alloc(0)) != NULL) {
            o->kind = ATTACH_PLESIOTH_ATTACK;
            o->type = kind;
            o->update = PlesiothAttack_Update;
            o->idx = em->index;
            o->isEm = em->isEm;
            o->ownerMotion = em->motionId[0];
            o->owner = em;
            o->yaw = em->rotY;
            o->rotX = em->rotX;
            o->rotY = em->rotY;
            o->rotZ = em->rotZ;
            o->side = *(u8 *) &em->side;
            em->hitPending = 0;
            em->contactHit = 0;
        }
    }
}

/* The PlesiothAttack's per-frame update, by ObjAttach.state. */
void PlesiothAttack_Update(ObjAttach *o)
{
    switch (o->state) {
    case ATTACH_SETUP:
        PlesiothAttack_Setup(o);
        break;
    case ATTACH_RUN:
        PlesiothAttack_Run(o);
        break;
    case ATTACH_END:
        PlesiothAttack_End(o);
        break;
    case ATTACH_HIT:
        PlesiothAttack_Run(o);
        break;
    case 4:
        PlesiothAttack_End(o);
        break;
    case ATTACH_DEFLECTED:
        PlesiothAttack_End(o);
        break;
    case ATTACH_DEFLECTED_AURA:
        PlesiothAttack_End(o);
        break;
    case 7:
        PlesiothAttack_End(o);
        break;
    case ATTACH_FREE:
        PlesiothAttack_Free(o);
        
    default:
        break;
    }
}

/*
 * Sets the attack up from its row: the hit shapes and ATKF_OWNER_CONTACT;
 * hitDelay is counted from the monster's motion.
 */
void PlesiothAttack_Setup(ObjAttach *o)
{
    Em *em = &gEms[o->idx];
    s32 blend;

    o->state++;
    o->visible = 1;
    o->draw = NULL;
    Player_TestSetFlags(o, ATKF_OWNER_CONTACT);
    ObjAttach_StartOwnedAttack(o, (s32) em, o->type, &PlesiothAttack_RowsPtr);
    o->hitList = PlesiothAttack_HitShapes[o->hitListNo];
    o->hurtList = 0;
    o->area = em->area;
    if (o->hitDelay != 0) {
        blend = Math_Absf(em->motionBlend[0] % 100);
        o->hitDelay += blend - (s32) Math_Absf(em->motionFrame[0]) - 1;
        if ((s8) o->hitDelay < 0) {
            o->hitDelay = 0;
        }
    }
    o->drawSlotBuf = NULL;
}

/*
 * Ends the attack when the monster's motion changes or the hit window closes;
 * types 14, 19, 23 and 24 keep their window open.
 */
void PlesiothAttack_Run(ObjAttach *o)
{
    Em *em = &gEms[o->idx];

    switch (o->type) {
    case 14:
    case 19:
    case 23:
    case 24:
        o->hitFrames = 99;
        break;
    }
    if (em->motionId[0] != o->ownerMotion || o->hitPhase == 0) {
        o->hitPhase = 0;
        o->state = ATTACH_END;
    }
    if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
    }
}

/* Hides the attack, frees its draw slot and frees it on the next update. */
void PlesiothAttack_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = ATTACH_FREE;
    o->visible = 0;
    if (o->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) o->drawSlot);
    }
}

/* Returns the object to the pool. */
void PlesiothAttack_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

/*
 * Makes a CephadromeAttack, a hit zone of Cephadrome's, for em with the row
 * kind of CephadromeAttack_Rows, if a player is in em's area
 * (EmCephadrome_CreateAttack). Clears em's hitPending and contactHit.
 */
void CephadromeAttack_Create(Em *em, s32 kind)
{
    ObjAttach *o;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((o = ObjAttach_Alloc(0)) != NULL) {
            o->kind = ATTACH_CEPHADROME_ATTACK;
            o->type = kind;
            o->update = CephadromeAttack_Update;
            o->idx = em->index;
            o->isEm = em->isEm;
            o->ownerMotion = em->motionId[0];
            o->owner = em;
            o->yaw = em->rotY;
            o->rotX = em->rotX;
            o->rotY = em->rotY;
            o->rotZ = em->rotZ;
            o->side = *(u8 *) &em->side;
            em->hitPending = 0;
            em->contactHit = 0;
        }
    }
}

/* The CephadromeAttack's per-frame update, by ObjAttach.state. */
void CephadromeAttack_Update(ObjAttach *o)
{
    switch (o->state) {
    case ATTACH_SETUP:
        CephadromeAttack_Setup(o);
        break;
    case ATTACH_RUN:
        CephadromeAttack_Run(o);
        break;
    case ATTACH_END:
        CephadromeAttack_End(o);
        break;
    case ATTACH_HIT:
        CephadromeAttack_Run(o);
        break;
    case 4:
        CephadromeAttack_End(o);
        break;
    case ATTACH_DEFLECTED:
        CephadromeAttack_End(o);
        break;
    case ATTACH_DEFLECTED_AURA:
        CephadromeAttack_End(o);
        break;
    case 7:
        CephadromeAttack_End(o);
        break;
    case ATTACH_FREE:
        CephadromeAttack_Free(o);
        
    default:
        break;
    }
}

/*
 * Sets the attack up from its row: the hit shapes and ATKF_OWNER_CONTACT;
 * hitDelay is counted from the monster's motion.
 */
void CephadromeAttack_Setup(ObjAttach *o)
{
    Em *em = &gEms[o->idx];
    s32 blend;

    o->state++;
    o->visible = 1;
    o->draw = NULL;
    Player_TestSetFlags(o, ATKF_OWNER_CONTACT);
    ObjAttach_StartOwnedAttack(o, (s32) em, o->type, &CephadromeAttack_RowsPtr);
    o->hitList = CephadromeAttack_HitShapes[o->hitListNo];
    o->hurtList = 0;
    o->area = em->area;
    if (o->hitDelay != 0) {
        blend = Math_Absf(em->motionBlend[0] % 100);
        o->hitDelay += blend - (s32) Math_Absf(em->motionFrame[0]) - 1;
        if ((s8) o->hitDelay < 0) {
            o->hitDelay = 0;
        }
    }
    o->drawSlotBuf = NULL;
}

/*
 * Ends the attack when the monster's motion changes or the hit window closes;
 * types 14, 19, 23, 24 and 26 keep their window open.
 */
void CephadromeAttack_Run(ObjAttach *o)
{
    Em *em = &gEms[o->idx];

    switch (o->type) {
    case 14:
    case 19:
    case 23:
    case 24:
    case 26:
        o->hitFrames = 99;
        break;
    }
    if (em->motionId[0] != o->ownerMotion || o->hitPhase == 0) {
        o->hitPhase = 0;
        o->state = ATTACH_END;
    }
    if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
    }
}

/* Hides the attack, frees its draw slot and frees it on the next update. */
void CephadromeAttack_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = ATTACH_FREE;
    o->visible = 0;
    if (o->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) o->drawSlot);
    }
}

/* Returns the object to the pool. */
void CephadromeAttack_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

/*
 * Makes pl's stage shot of type kind. Type 0 is the mounted gun: two shots,
 * 36.2 to either side, aimed by pl->aimPitchPct, the second 3 frames late.
 * Type 1 is the cannon: it starts at the area's cannon spot pl->stateRand,
 * with a random spread from randBits, and is freed outside its three areas.
 * Type 7 is the arena strike, on pl's side and 45 frames late. Skill 0xC9
 * sets powerPct 110 for types 0 and 1.
 */
void StageShot_CreateForPlayer(Player *pl, u8 kind)
{
    f32 ofs[3];
    s16 count;
    ObjAttach *o;
    u8 powerPercent = 0;
    f32 (*pts)[3];
    u16 *angs;
    s16 i;
    s8 aim;
    f32 turn;

    switch (kind) {
    case STAGESHOT_MOUNTED_GUN:
        count = 2;
        if (Player_HasSkill(pl, 0xC9) == 1) {
            powerPercent = 110;
        }
        break;
    case STAGESHOT_CANNON:
        count = 1;
        if (Player_HasSkill(pl, 0xC9) == 1) {
            powerPercent = 110;
        }
        break;
    default:
        count = 1;
        break;
    }
    for (i = 0; i < count; i++) {
        if ((o = ObjAttach_Alloc(0)) != NULL) {
            o->kind = ATTACH_STAGE_SHOT;
            o->type = kind;
            o->update = StageShot_Update;
            o->idx = pl->index;
            o->isEm = pl->isEm;
            o->owner = pl;
            o->area = pl->area;
            o->stateVar0 = pl->stateRand;
            o->randBits = (pl->stateRand & 0xFF00) >> 8;
            o->stateVar1 = 0;
            o->ownerMotion = 0;
            o->powerPct = powerPercent;
            switch (kind) {
            case STAGESHOT_MOUNTED_GUN:
                o->pos[0] = pl->pos[0];
                o->pos[1] = pl->pos[1];
                o->pos[2] = pl->pos[2];
                aim = pl->aimPitchPct;
                if (aim >= 0) {
                    turn = 65536.0f;
                    o->rotX = -(u16) (s32) (0.5f + (turn * (0.01f * (45.0f * aim))) / 360.0f);
                } else {
                    turn = 65536.0f;
                    o->rotX = -(u16) (s32) (0.5f + (turn * (0.01f * (35.0f * aim))) / 360.0f);
                }
                o->rotY = pl->rot[1];
                if (i == 0) {
                    ofs[0] = 36.2f;
                } else {
                    ofs[0] = -36.2f;
                    o->ownerMotion = -3;
                }
                ofs[1] = 100.0f;
                ofs[2] = 75.0f;
                Vec_RotateY(ofs, (6.2831855f * o->rotY) / turn);
                o->pos[0] += ofs[0];
                o->pos[1] += ofs[1];
                o->pos[2] += ofs[2];
                break;
            case STAGESHOT_CANNON:
                switch (Area_DayId(o->area)) {
                case AREA_FORTRESS_5:
                    pts = StageShot_CannonPtsFortress5;
                    angs = StageShot_CannonAngsFortress5;
                    break;
                case AREA_CASTLE_1:
                    pts = StageShot_CannonPtsCastle1;
                    angs = StageShot_CannonAngsCastle1;
                    break;
                case AREA_BATTLE_TOWN_1:
                    pts = StageShot_CannonPtsBattleTown1;
                    angs = StageShot_CannonAngsBattleTown1;
                    break;
                default:
                    ObjAttach_Free(o);
                    return;
                }
                Vec_Copy(o->pos, pts[o->stateVar0]);
                o->rotX = 0xF05C;
                o->rotY = angs[o->stateVar0];
                ofs[0] = 0.0f;
                ofs[1] = 50.0f;
                ofs[2] = 255.0f;
                Vec_RotateX(ofs, (6.2831855f * o->rotX) / 65536.0f);
                Vec_RotateY(ofs, (6.2831855f * o->rotY) / 65536.0f);
                o->pos[0] += ofs[0];
                o->pos[1] += ofs[1];
                o->pos[2] += ofs[2];
                break;
            case STAGESHOT_ARENA_STRIKE:
                o->side = pl->side;
                o->ownerMotion = -45;
                break;
            }
            o->yaw = o->rotY;
        }
    }
}

/*
 * Makes a StageShot of type kind with no owner at pos in area, facing ang
 * (for AreaWeapon_Run and CastleGate_Run). Type 2 makes four objects and
 * type 8 two; stateVar1, their index, picks each one's hit shapes.
 */
void StageShot_CreateAt(f32 *pos, u8 kind, u8 area, s32 ang)
{
    s16 count;
    s16 i;
    ObjAttach *o;

    switch (kind) {
    case STAGESHOT_AREA_BURST:
        count = 4;
        break;
    case STAGESHOT_AREA_PAIR:
        count = 2;
        break;
    default:
        count = 1;
        break;
    }
    for (i = 0; i < count; i++) {
        if ((o = ObjAttach_Alloc(0)) != NULL) {
            o->kind = ATTACH_STAGE_SHOT;
            o->type = kind;
            o->update = StageShot_Update;
            o->idx = 0;
            o->isEm = 0;
            o->owner = NULL;
            o->yaw = ang;
            o->pos[0] = pos[0];
            o->pos[1] = pos[1];
            o->pos[2] = pos[2];
            o->area = area;
            o->rotX = 0;
            o->rotY = (u16) ang;
            o->stateVar0 = 0;
            o->stateVar1 = i;
            o->ownerMotion = 0;
        }
    }
}

/*
 * Makes Lao-Shan Lung's falling rock (type kind, 4) at the current area's
 * spot near em (StageShot_FindSpot), if a player is in em's area. spot
 * (stateVar1) picks its added velocity in StageShot_RockSpotVels.
 */
void StageShot_CreateAtSpot(Em *em, s32 kind, s32 spot)
{
    u16 ang;
    f32 pos[3];
    ObjAttach *o;

    if (Player_InCurrentArea(em) & 0xFF) {
        if (StageShot_FindSpot(&em->posX, pos, &ang) != 0) {
            if ((o = ObjAttach_Alloc(0)) != NULL) {
                o->kind = ATTACH_STAGE_SHOT;
                o->type = kind;
                o->update = StageShot_Update;
                o->idx = 0;
                o->isEm = 1;
                o->owner = em;
                o->yaw = ang;
                o->pos[0] = pos[0];
                o->pos[1] = pos[1];
                o->pos[2] = pos[2];
                o->area = em->area;
                o->rotX = 0;
                o->rotY = ang;
                o->stateVar0 = 0;
                o->stateVar1 = spot;
                o->ownerMotion = 0;
            }
        }
    }
}

/*
 * Makes em's StageShot of type kind at pos, if a player is in em's area
 * (Blangonga's falling objects, type 6). It starts order * 20 frames late.
 */
void StageShot_CreateAtPos(Em *em, f32 *pos, s8 kind, u8 order)
{
    ObjAttach *o;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((o = ObjAttach_Alloc(0)) != NULL) {
            o->kind = ATTACH_STAGE_SHOT;
            o->type = kind;
            o->update = StageShot_Update;
            o->idx = 0;
            o->isEm = 1;
            o->owner = em;
            o->yaw = em->rotY;
            o->pos[0] = pos[0];
            o->pos[1] = pos[1];
            o->pos[2] = pos[2];
            o->area = em->area;
            o->rotX = 0;
            o->rotY = 0;
            o->stateVar0 = 0;
            o->ownerMotion = -order * 20;
        }
    }
}

/*
 * The StageShot's per-frame update, by ObjAttach.state; ATTACH_HIT goes to
 * StageShot_Hit.
 */
void StageShot_Update(ObjAttach *o)
{
    switch (o->state) {
    case ATTACH_SETUP:
        StageShot_Setup(o);
        break;
    case ATTACH_RUN:
        StageShot_Run(o);
        break;
    case ATTACH_END:
        StageShot_End(o);
        break;
    case ATTACH_HIT:
        StageShot_Hit(o);
        break;
    case 4:
        StageShot_End(o);
        break;
    case ATTACH_DEFLECTED:
        StageShot_End(o);
        break;
    case ATTACH_DEFLECTED_AURA:
        StageShot_End(o);
        break;
    case 7:
        StageShot_End(o);
        break;
    case ATTACH_FREE:
        StageShot_Free(o);
        
    default:
        break;
    }
}

/*
 * Sets the shot up from row type + 1 of StageShot_Rows (an ownerless shot
 * takes it without an owner). Types 4 and 6 also hit players, the others only
 * monsters. Types 2 and 8 offset their hit shapes by stateVar1. The velocity
 * (StageShot_StartVels) and gravity (StageShot_Gravities) come by type, plus
 * the rock's spot velocity or the cannon's random spread, turned by rotX and
 * rotY. Plays the set-up sound and takes a draw slot (none for types 2, 3 and
 * 5; it frees itself when none is free); the rock shakes the camera.
 */
void StageShot_Setup(ObjAttach *o)
{
    Em *owner = o->owner;

    o->state++;
    o->visible = 1;
    o->draw = NULL;
    if (o->type == STAGESHOT_LAOSHAN_ROCK || o->type == STAGESHOT_FALLING_OBJECT) {
        Player_TestSetFlags(o, 0);
    } else {
        Player_TestSetFlags(o, ATKF_NO_PLAYER);
    }
    o->aux = 0;
    if (owner == NULL) {
        ObjAttach_StartAttack(o, o->type + 1, &StageShot_RowsPtr);
    } else {
        ObjAttach_StartOwnedAttack(o, (s32) owner, o->type + 1, &StageShot_RowsPtr);
    }
    switch (o->type) {
    case STAGESHOT_AREA_BURST:
    case STAGESHOT_AREA_PAIR:
        o->hitList = StageShot_HitShapes[o->hitListNo + o->stateVar1];
        o->hurtList = 0;
        break;
    default:
        o->hitList = StageShot_HitShapes[o->hitListNo];
        o->hurtList = 0;
        break;
    }
    Vec_Copy(o->pos2, o->pos);
    o->velX = StageShot_StartVels[o->type][0];
    o->velY = StageShot_StartVels[o->type][1];
    o->velZ = StageShot_StartVels[o->type][2];
    o->homingTurn = 0.0f;
    o->gravity = StageShot_Gravities[o->type];
    o->homingSpeed = 0.0f;
    switch (o->type) {
    case STAGESHOT_LAOSHAN_ROCK:
        o->velX += StageShot_RockSpotVels[o->stateVar1 & 3];
        o->velY += StageShot_RockSpotVels[(o->stateVar1 & 3) + 1];
        o->velZ += StageShot_RockSpotVels[(o->stateVar1 & 3) + 2];
        break;
    case STAGESHOT_CANNON:
        o->velX += StageShot_CannonSpread[o->randBits & 3];
        o->velY += 0.4f * StageShot_CannonSpread[(o->randBits >> 2) & 3];
        o->velZ += StageShot_CannonSpread[(o->randBits >> 4) & 3];
        o->gravity += 0.2f * StageShot_CannonSpread[(o->randBits >> 6) & 3];
        break;
    }
    Vec_RotateX(&o->velX, (6.2831855f * o->rotX) / 65536.0f);
    Vec_RotateY(&o->velX, (6.2831855f * o->rotY) / 65536.0f);
    StageShot_PlaySound(o, 0);
    switch (o->type) {
    case STAGESHOT_MOUNTED_GUN:
    case STAGESHOT_CANNON:
    case STAGESHOT_LAOSHAN_ROCK:
    case STAGESHOT_FALLING_OBJECT:
    case STAGESHOT_ARENA_STRIKE:
    default:
        o->drawSlot = DrawSlot_Alloc();
        if (o->drawSlot != -1) {
            o->drawSlotBuf = DrawSlot_Get(o->drawSlot);
            o->drawSlotBuf->owner = o;
            o->drawSlotBuf->draw = StageShot_Draw;
            Vec_Copy(o->drawSlotBuf->pos, o->pos);
            if (o->type != STAGESHOT_CANNON && o->type != STAGESHOT_MOUNTED_GUN && o->type != STAGESHOT_ARENA_STRIKE) {
                OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
            }
            if (o->type == STAGESHOT_LAOSHAN_ROCK && o->area == gScene.area) {
                StageShot_CamShake(o);
            }
        } else {
            ObjAttach_Free(o);
        }
        break;
    case STAGESHOT_AREA_BURST:
    case STAGESHOT_AREA_PULSE:
    case STAGESHOT_AREA_DROP:
        o->drawSlotBuf = NULL;
        break;
    }
}

/*
 * Counts ownerMotion (the frames; negative is a start delay) and ends the
 * shot past StageShot_TimeLimit[type]; except type 7, an open hit window
 * stays open. Types 0 and 1 fire after 5 or 10 frames (sound, StageObj_K12
 * effects) and end at the floor. Types 2, 3 and 8 move only for 4 frames; 3
 * reopens its hit every 25 frames, 8 once after 20. The rock (4) crashes 50
 * above the floor (shake, sound, dust), 6 falls to the floor, and 7 rises
 * under the first visible monster of another side for 4 frames, holds for 15,
 * then sinks.
 */
void StageShot_Run(ObjAttach *o)
{
    Em *target;
    f32 groundY;
    s16 i;
    s16 useOt32 = 1;

    o->ownerMotion++;
    Vec_Copy(o->pos2, o->pos);
    if (o->type != STAGESHOT_ARENA_STRIKE && o->hitFrames != 0) {
        o->hitFrames = 99;
    }
    if (o->ownerMotion > StageShot_TimeLimit[o->type]) {
        o->hitFrames = 0;
        o->hitPhase = 0;
        o->state = ATTACH_END;
        return;
    }
    switch (o->type) {
    case STAGESHOT_MOUNTED_GUN:
    default:
        if (o->ownerMotion < 5) {
            return;
        }
        if (o->ownerMotion < 13) {
            if (o->ownerMotion == 5) {
                StageShot_PlaySound(o, 1);
                if (o->area == gScene.area) {
                    ShotFlash_CreateAtAngles(o->pos, 10, o->rotX, o->rotY);
                    ShotFlash_CreateAtAngles(o->pos, 11, o->rotX, o->rotY);
                }
            }
            ObjAttach_Move(o);
        } else {
            ObjAttach_MoveAccel(o);
        }
        if (o->ownerMotion > 5 && o->pos[1] <= AreaGrid2_GetFloorYFiltered(o->pos)) {
            o->hitFrames = 0;
            o->hitPhase = 0;
            o->state = ATTACH_END;
        }
        break;
    case STAGESHOT_CANNON:
        switch (o->stateVar1) {
        case 0:
            if (o->ownerMotion > 10) {
                o->stateVar1++;
                StageShot_PlaySound(o, 1);
                if (o->area == gScene.area) {
                    ShotFlash_CreateAtAngles(o->pos, 10, o->rotX, o->rotY);
                    ShotFlash_CreateAtAngles(o->pos, 11, o->rotX, o->rotY);
                }
            }
            return;
        case 1:
            ObjAttach_MoveAccel(o);
            if (o->ownerMotion > 15 && o->pos[1] <= AreaGrid2_GetFloorYFiltered(o->pos)) {
                o->hitFrames = 0;
                o->hitPhase = 0;
                o->state = ATTACH_END;
            }
            break;
        }
        break;
    case STAGESHOT_AREA_BURST:
        if (o->ownerMotion <= 4) {
            ObjAttach_MoveAccel(o);
        }
        break;
    case STAGESHOT_AREA_PULSE:
        if (++o->aux >= 25) {
            ObjAttach_StartAttack(o, o->type + 1, &StageShot_RowsPtr);
            o->aux = 0;
        }
        if (o->ownerMotion <= 4) {
            ObjAttach_MoveAccel(o);
        }
        break;
    case STAGESHOT_AREA_PAIR:
        if (o->stateVar0 == 0 && ++o->aux >= 20) {
            ObjAttach_StartAttack(o, o->type + 1, &StageShot_RowsPtr);
            o->aux = 0;
            o->stateVar0++;
        }
        if (o->ownerMotion <= 4) {
            ObjAttach_MoveAccel(o);
        }
        break;
    case STAGESHOT_LAOSHAN_ROCK:
        ObjAttach_MoveAccel(o);
        if (o->ownerMotion > 5) {
            if (o->ownerMotion == 7) {
                StageShot_PlaySound(o, 1);
            }
            groundY = 50.0f + AreaGrid2_GetFloorYFiltered(o->pos);
            if (o->pos[1] <= groundY) {
                o->pos[1] = groundY;
                if (o->area == gScene.area) {
                    StageShot_CamShake(o);
                    StageShot_PlaySound(o, 2);
                    HitSpark_SpawnScaled(0, 0xB, 0, o->pos, 2.0f);
                    HitSpark_SpawnScaled(0, 0xB, 1, o->pos, 2.0f);
                    PartFx_SpawnIfHere(o->owner, o->pos, 0x22, 16.0f);
                }
                o->hitFrames = 0;
                o->hitPhase = 0;
                o->state = ATTACH_END;
            }
        }
        break;
    case STAGESHOT_FALLING_OBJECT:
        if (o->ownerMotion > 0) {
            ObjAttach_MoveAccel(o);
            if (o->ownerMotion > 5 && o->pos[1] <= AreaGrid2_GetFloorYFiltered(o->pos)) {
                o->hitFrames = 0;
                o->hitPhase = 0;
                o->state = ATTACH_END;
            }
        }
        useOt32 = 0;
        break;
    case STAGESHOT_ARENA_STRIKE:
        if (o->ownerMotion <= 0) {
            return;
        }
        switch (o->stateVar1) {
        case 0:
            if (o->ownerMotion == 1) {
                for (i = 0, target = gEms; i < 20; i++, target++) {
                    if (target->active != 0 && target->visible != 0 && Side_Match(o->side, *(u8 *) &target->side, 1) == 0) {
                        o->pos[0] = target->posX;
                        o->pos[1] = target->floorY;
                        o->pos[2] = target->posZ;
                        o->rotY = target->rotY;
                        StageShot_PlaySound(o, 1);
                        if (o->area == gScene.area) {
                            PartFx_SpawnAt(o->pos, 7, 3.0f);
                        }
                        break;
                    }
                }
                if (i == 20) {
                    o->hitFrames = 0;
                    o->hitPhase = 0;
                    o->state = ATTACH_END;
                    return;
                }
            }
            ObjAttach_MoveAccel(o);
            if (++o->aux >= 4) {
                o->stateVar1++;
                o->aux = 0;
            }
            break;
        case 1:
            if (++o->aux >= 15) {
                o->stateVar1++;
                o->aux = 0;
                StageShot_PlaySound(o, 2);
            }
            break;
        case 2:
            o->pos[1] -= 60.0f;
            break;
        }
        break;
    case STAGESHOT_AREA_DROP:
        break;
    }
    if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        if (useOt32 == 0) {
            OT_AddByDepth(gDrawOtSorted, o->drawSlotBuf, 0x40U, 0);
        } else {
            OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
        }
    }
}

/*
 * On a hit: types 2, 3, 7 and 8 go on (StageShot_Run); the rock (4) crashes
 * as on landing and ends; the others end.
 */
void StageShot_Hit(ObjAttach *o)
{
    u8 type;

    type = o->type;
    switch (type) {
    case 0x0:
    default:
        StageShot_End(o);
        break;
    case 2:
    case 3:
    case 7:
    case 8:
        StageShot_Run(o);
        break;
    case 4:
        if (o->area == gScene.area) {
            StageShot_CamShake(o);
            HitSpark_SpawnScaled(0, 0xB, 0, o->pos, 2.0f);
            HitSpark_SpawnScaled(0, 0xB, 1, o->pos, 2.0f);
            PartFx_SpawnIfHere(o->owner, o->pos, 0x22, 16.0f);
        }
        o->hitFrames = 0;
        o->hitPhase = 0;
        StageShot_End(o);
        break;
    }
}

/* Hides the shot, frees its draw slot and frees it on the next update. */
void StageShot_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = ATTACH_FREE;
    o->visible = 0;
    if (o->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) o->drawSlot);
    }
}

/* Returns the object to the pool. */
void StageShot_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

/*
 * Draws model mdl with matrix mtx and the lights of tex (nothing when mdl is
 * NULL or has no id), then restores the default draw state.
 */
void StageShot_DrawModel(ModelMatSet *mdl, f32 *mtx, ModelMat *tex)
{
    Gs_SetState(0x1A, (s32) mtx);
    if (mdl != NULL && mdl->id != -1) {
        Gs_SetLightList(tex, mdl);
        Draw_ApplyStates(mdl->flags);
        Model_Draw(mdl->id, 0);
    }
    Draw_SetDefaultState();
}

/*
 * Draws a StageShot in the current area by type: 0 the gun's shot model of
 * Fortress 5, Castle 1 and Battle Town 1 and 3, turned along its velocity; 1
 * the cannon's shot (common model 133); 4 the rock of Fortress 2-4; 6 the
 * falling object of Snowy Mountains 3 and 8; 7 the Great Arena's object. The
 * other types draw nothing.
 */
void StageShot_Draw(DrawSlot *slot)
{
    f32 mtx[16];
    f32 texMtx[16];
    u16 pitch;
    u16 yaw;
    ObjAttach *o = slot->owner;
    ModelMat *tex;
    ModelMatSet *mdl;

    if (gScene.area != o->area) {
        return;
    }
    switch (o->type) {
    case STAGESHOT_MOUNTED_GUN:
    {
        Model *bank = gAreaSubModels[0];

        if (bank == NULL) {
            return;
        }
        if (bank->active == 0) {
            return;
        }
        Mtx_Scale(mtx, 2.0f, 2.0f, 1.0f);
        tex = bank->mats;
        switch (o->area) {
        case AREA_FORTRESS_5:
        case AREA_FORTRESS_NIGHT_5:
            mdl = &bank->matSets[4];
            break;
        case AREA_CASTLE_1:
            mdl = &bank->matSets[13];
            break;
        case AREA_BATTLE_TOWN_1:
        case AREA_BATTLE_TOWN_NIGHT_1:
            mdl = &bank->matSets[4];
            break;
        case AREA_BATTLE_TOWN_3:
        case AREA_BATTLE_TOWN_NIGHT_3:
            mdl = &bank->matSets[2];
            break;
        default:
            return;
        }
        }
        Vec_DirToAngles(&o->velX, (s16 *) &pitch, (s16 *) &yaw);
        Mtx_RotateX(mtx, (6.2831855f * pitch) / 65536.0f);
        Mtx_RotateY(mtx, (6.2831855f * yaw) / 65536.0f);
        break;
    case STAGESHOT_CANNON:
    {
        Model *bank = gCommonModels[0];

        if (bank == NULL) {
            return;
        }
        if (bank->active == 0) {
            return;
        }
        Mtx_Identity(mtx);
        tex = bank->mats;
        mdl = &bank->matSets[133];
        Mtx_Translation(texMtx, 0.0f, 0.0f, 0.0f);
        Gs_SetState(0x19, (s32) texMtx);
        break;
    }
    case STAGESHOT_LAOSHAN_ROCK:
    {
        Model *bank = gAreaSubModels[0];

        if (bank == NULL) {
            return;
        }
        if (bank->active == 0) {
            return;
        }
        Mtx_Identity(mtx);
        tex = bank->mats;
        switch (o->area) {
        case AREA_FORTRESS_4:
        case AREA_FORTRESS_2:
            mdl = &bank->matSets[2];
            break;
        case AREA_FORTRESS_NIGHT_4:
            mdl = &bank->matSets[3];
            break;
        case AREA_FORTRESS_3:
        case AREA_FORTRESS_NIGHT_3:
            mdl = &bank->matSets[24];
            break;
        case AREA_FORTRESS_NIGHT_2:
            mdl = &bank->matSets[3];
            break;
        default:
            return;
        }
        break;
    }
    case STAGESHOT_FALLING_OBJECT:
    {
        Model *bank = gAreaSubModels[0];

        if (bank == NULL) {
            return;
        }
        if (bank->active == 0) {
            return;
        }
        Mtx_Scale(mtx, 1.7f, 1.0f, 1.7f);
        switch (o->area) {
        case AREA_SNOWY_MOUNTAINS_3:
        default:
        case AREA_SNOWY_MOUNTAINS_NIGHT_3:
            mdl = &bank->matSets[4];
            break;
        case AREA_SNOWY_MOUNTAINS_8:
        case AREA_SNOWY_MOUNTAINS_NIGHT_8:
            mdl = &bank->matSets[7];
            break;
        }
        tex = bank->mats;
        Gs_SetState(0x67, -1);
        break;
    }
    case STAGESHOT_ARENA_STRIKE:
    {
        Model *bank = gAreaSubModels[0];

        if (bank == NULL) {
            return;
        }
        if (bank->active == 0) {
            return;
        }
        Mtx_Identity(mtx);
        tex = bank->mats;
        switch (o->area) {
        case AREA_GREAT_ARENA_NIGHT_1:
        case AREA_GREAT_ARENA_1:
            mdl = &bank->matSets[22];
            break;
        default:
            return;
        }
        break;
    }
    default:
        return;
    }
    Mtx_SetTrans(mtx, slot->pos[0], slot->pos[1], slot->pos[2]);
    StageShot_DrawModel(mdl, mtx, tex);
}

/*
 * Plays the shot's sound for event (0 set up, 1 fired or appearing, 2 landing
 * or striking) if it is in the current area. STAGESHOT_LAOSHAN_ROCK plays
 * through the Lao-Shan Lung present (Em_PlaySeChecked); STAGESHOT_AREA_DROP
 * and STAGESHOT_FALLING_OBJECT have none.
 */
void StageShot_PlaySound(ObjAttach *o, s16 event)
{
    Em *em = gEms;
    s16 i;

    if (o->area != gScene.area) {
        return;
    }
    switch (o->type) {
    case STAGESHOT_MOUNTED_GUN:
        switch (event) {
        case 0:
            Sound_PlaySeAtPosChecked(7, 0x38, 0, o->pos, 7, 0);
            break;
        case 1:
            Sound_PlaySeAtPosChecked(7, 0x30, 0, o->pos, 7, 0);
            break;
        }
        break;
    case STAGESHOT_CANNON:
        switch (event) {
        case 0:
            break;
        case 1:
            Sound_PlaySeAtPosChecked(7, 0x32, 0, o->pos, 7, 0);
            break;
        }
        break;
    case STAGESHOT_AREA_BURST:
    case STAGESHOT_AREA_PULSE:
    case STAGESHOT_AREA_PAIR:
        switch (event) {
        case 0:
            Sound_PlaySeAtPosChecked(7, 0x34, 0, o->pos, 7, 0);
            break;
        }
        break;
    case STAGESHOT_LAOSHAN_ROCK:
        for (i = 0; i < EM_COUNT; i++, em++) {
            if (em->active && em->visible && (em->type == EM_KIND_LAO_SHAN_LUNG || em->type == EM_KIND_ASHEN_LAO_SHAN_LUNG)) {
                break;
            }
        }
        switch (event) {
        case 0:
            Em_PlaySeChecked(em, 0x2C, 0, o->pos, 3, 0);
            break;
        case 1:
            Em_PlaySeChecked(em, 0x2D, 0, o->pos, 3, 0);
            break;
        case 2:
            Em_PlaySeChecked(em, 0x2B, 0, o->pos, 3, 0);
            break;
        }
        break;
    case STAGESHOT_AREA_DROP:
    case STAGESHOT_FALLING_OBJECT:
        break;
    case STAGESHOT_ARENA_STRIKE:
        switch (event) {
        case 1:
            Sound_PlaySeAtPosChecked(7, 0x34, 0, o->pos, 7, 0);
            break;
        case 2:
            Sound_PlaySeAtPosChecked(7, 0x36, 0, o->pos, 7, 0);
            break;
        }
        break;
    }
}

/*
 * Finds the falling-rock spot of the current area (Fortress 2, 3 or 4) whose
 * box holds from, copies its position to pos and its angle to ang, and
 * returns 1. Returns 0 when there is none.
 */
s32 StageShot_FindSpot(f32 *from, f32 *pos, u16 *ang)
{
    s16 i;

    switch (Area_DayId(gScene.area)) {
    case AREA_FORTRESS_4:
        if (from[0] > 12000.0f && from[0] < 16000.0f && from[2] > 20500.0f && from[2] < 23000.0f) {
            i = 0;
        } else if (from[0] > 12000.0f && from[0] < 16000.0f && from[2] > 26000.0f && from[2] < 29000.0f) {
            i = 1;
        } else if (from[0] > 16500.0f && from[0] < 19000.0f && from[2] > 28000.0f && from[2] < 32000.0f) {
            i = 2;
        } else {
            return 0;
        }
        Vec_Copy(pos, StageShot_RockSpotsFortress4[i]);
        *ang = StageShot_RockAngsFortress4[i];
        return 1;
    case AREA_FORTRESS_3:
        if (from[0] > 14000.0f && from[0] < 17000.0f && from[2] > 18000.0f && from[2] < 25000.0f) {
            i = 0;
        } else if (from[0] > 17000.0f && from[0] < 20000.0f && from[2] > 22000.0f && from[2] < 29000.0f) {
            i = 1;
        } else {
            return 0;
        }
        Vec_Copy(pos, StageShot_RockSpotsFortress3[i]);
        *ang = StageShot_RockAngsFortress3[i];
        return 1;
    case AREA_FORTRESS_2:
        if (from[0] > 9000.0f && from[0] < 13000.0f && from[2] > 21000.0f && from[2] < 26000.0f) {
            i = 0;
        } else if (from[0] > 14000.0f && from[0] < 18000.0f && from[2] > 21000.0f && from[2] < 26000.0f) {
            i = 1;
        } else if (from[0] > 20000.0f && from[0] < 25000.0f && from[2] > 20000.0f && from[2] < 25000.0f) {
            i = 2;
        } else {
            return 0;
        }
        Vec_Copy(pos, StageShot_RockSpotsFortress2[i]);
        *ang = StageShot_RockAngsFortress2[i];
        return 1;
    default:
        return 0;
    }
}

/*
 * Shakes the camera by the shot's distance from it: strength 5 within 500,
 * down to 0 within 3000, none beyond.
 */
void StageShot_CamShake(ObjAttach *o)
{
    f32 dist;

    dist = Vec_Distance(o->pos, (f32 *) &gCamPos);
    if (dist < 500.0f) {
        CamShake_SetAEverywhere(5);
        return;
    }
    if (dist < 1000.0f) {
        CamShake_SetAEverywhere(4);
        return;
    }
    if (dist < 1500.0f) {
        CamShake_SetAEverywhere(3);
        return;
    }
    if (dist < 2000.0f) {
        CamShake_SetAEverywhere(2);
        return;
    }
    if (dist < 2500.0f) {
        CamShake_SetAEverywhere(1);
        return;
    }
    if (dist < 3000.0f) {
        CamShake_SetAEverywhere(0);
    }
}

/*
 * The heavy slams' falling objects (EmBlangonga_Slam's variant 1,
 * EmBlangonga_RajangComboSlam): in the two areas that have them (by Area_DayId),
 * picks two of the area's points and makes a type-6 StageShot object fall at
 * each. In the first area each object has its own list of points
 * (EmBlangonga_FallListsSnowy3); in the second one list serves both, chosen by
 * the monster's z (EmBlangonga_FallListsByZ), and the second pick skips
 * the first. A list is a count, then indices into the area's points.
 */
void EmBlangonga_DropFallingObjects(Em *em)
{
    s16 pick[2];
    s16 used[2];
    s16 **tabs;
    s16 *tab;
    s16 mode;
    s16 i;
    f32 (*pts)[3];
    s16 j;
    s16 skipped;

    switch (Area_DayId(gScene.area)) {
    case AREA_SNOWY_MOUNTAINS_3:
        mode = 1;
        tabs = EmBlangonga_FallListsSnowy3;
        pts = EmBlangonga_FallPointsSnowy3;
        break;
    case AREA_SNOWY_MOUNTAINS_8:
        mode = 0;
        tab = EmBlangonga_FallListsByZ[(s16) ((em->posZ <= 9000.0f) ^ 1)];
        tabs = EmBlangonga_FallListsByZ;
        pts = EmBlangonga_FallPointsSnowy8;
        break;
    default:
        return;
    }
    for (i = 0; i < 2; i++) {
        if (mode == 1) {
            tab = tabs[i];
        }
        pick[i] = (em->stateRand >> i) % (tab[0] - i);
        if (mode == 0) {
            for (j = 0; j < i; j++) {
                used[j] = 0;
            }
        again:
            skipped = 0;
            for (j = 0; j < i; j++) {
                if (used[j] == 0 && pick[j] <= pick[i]) {
                    used[j] = 1;
                    skipped++;
                }
            }
            if (skipped > 0) {
                pick[i] += skipped;
                goto again;
            }
        }
        StageShot_CreateAtPos(em, pts[tab[pick[i] + 1]], STAGESHOT_FALLING_OBJECT, i);
    }
}

/*
 * Makes a CephalosAttack, a hit zone of Cephalos's, for em with the row kind
 * of CephalosAttack_Rows, if a player is in em's area
 * (EmCephadrome_CreateAttack). Clears em's hitPending and contactHit.
 */
void CephalosAttack_Create(Em *em, s32 kind)
{
    ObjAttach *o;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((o = ObjAttach_Alloc(0)) != NULL) {
            o->kind = ATTACH_CEPHALOS_ATTACK;
            o->type = kind;
            o->update = CephalosAttack_Update;
            o->idx = em->index;
            o->isEm = em->isEm;
            o->ownerMotion = em->motionId[0];
            o->owner = em;
            o->yaw = em->rotY;
            o->rotX = em->rotX;
            o->rotY = em->rotY;
            o->rotZ = em->rotZ;
            o->side = *(u8 *) &em->side;
            em->hitPending = 0;
            em->contactHit = 0;
        }
    }
}

/* The CephalosAttack's per-frame update, by ObjAttach.state. */
void CephalosAttack_Update(ObjAttach *o)
{
    switch (o->state) {
    case ATTACH_SETUP:
        CephalosAttack_Setup(o);
        break;
    case ATTACH_RUN:
        CephalosAttack_Run(o);
        break;
    case ATTACH_END:
        CephalosAttack_End(o);
        break;
    case ATTACH_HIT:
        CephalosAttack_Run(o);
        break;
    case 4:
        CephalosAttack_End(o);
        break;
    case ATTACH_DEFLECTED:
        CephalosAttack_End(o);
        break;
    case ATTACH_DEFLECTED_AURA:
        CephalosAttack_End(o);
        break;
    case 7:
        CephalosAttack_End(o);
        break;
    case ATTACH_FREE:
        CephalosAttack_Free(o);
        
    default:
        break;
    }
}

/*
 * Sets the attack up from its row: the hit shapes and ATKF_OWNER_CONTACT;
 * hitDelay is counted from the monster's motion.
 */
void CephalosAttack_Setup(ObjAttach *o)
{
    Em *em = &gEms[o->idx];
    s32 blend;

    o->state++;
    o->visible = 1;
    o->draw = NULL;
    Player_TestSetFlags(o, ATKF_OWNER_CONTACT);
    ObjAttach_StartOwnedAttack(o, (s32) em, o->type, &CephalosAttack_RowsPtr);
    o->hitList = CephalosAttack_HitShapes[o->hitListNo];
    o->hurtList = 0;
    o->area = em->area;
    if (o->hitDelay != 0) {
        blend = Math_Absf(em->motionBlend[0] % 100);
        o->hitDelay += blend - (s32) Math_Absf(em->motionFrame[0]) - 1;
        if ((s8) o->hitDelay < 0) {
            o->hitDelay = 0;
        }
    }
    o->drawSlotBuf = NULL;
}

/*
 * Ends the attack when the monster's motion changes or the hit window closes;
 * types 14, 19, 23, 24 and 26 keep their window open.
 */
void CephalosAttack_Run(ObjAttach *o)
{
    Em *em = &gEms[o->idx];

    switch (o->type) {
    case 14:
    case 19:
    case 23:
    case 24:
    case 26:
        o->hitFrames = 99;
        break;
    }
    if (em->motionId[0] != o->ownerMotion || o->hitPhase == 0) {
        o->hitPhase = 0;
        o->state = ATTACH_END;
    }
    if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
    }
}

/* Hides the attack, frees its draw slot and frees it on the next update. */
void CephalosAttack_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = ATTACH_FREE;
    o->visible = 0;
    if (o->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) o->drawSlot);
    }
}

/* Returns the object to the pool. */
void CephalosAttack_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

/*
 * Fires the bow's shot through BowArrow_Fire. The shot level comes from
 * gBowgunClipShots, at the bow's clipRow and pl->chargeLevel. A full charge
 * (3) adds the StageObj_K29 effect of kind 3.
 */
void Bow_FireArrows(Player *pl, s32 unused)
{

    if ((s8) pl->chargeLevel == 3) {
        PlayerFx_CreateOnPlayerDefault(pl, 3, 1.0f);
    }
    BowArrow_Fire(pl, gBowgunClipShots[gGunData[pl->equip[6].id].clipRow][(s8) pl->chargeLevel & 3]);
}

/*
 * Makes one arrow of a shot for pl. kind is its type (a BowArrow_Kinds row),
 * angle its yaw, idx its place in the shot (stateVar1; it also varies
 * randBits), crit 1 a critical hit or 2 a weak one, shot the shot level.
 * Fills its buffer from the player: charge, attackFactor, coating, element
 * and skills.
 */
void BowArrow_Create(Player *pl, u16 angle, u8 kind, u8 idx, u8 crit, s16 shot)
{
    ObjAttach *o;
    BowArrowBuf *buf;

    if ((o = ObjAttach_Alloc(1)) != NULL) {
        o->kind = ATTACH_BOW_ARROW;
        o->type = kind;
        o->update = BowArrow_Update;
        o->idx = pl->index;
        o->isEm = pl->isEm;
        o->owner = pl;
        o->area = pl->area;
        o->rotX = 0;
        o->rotY = angle;
        o->stateVar1 = idx;
        o->side = pl->side;
        o->randBits = pl->stateRand * (idx * 2 + 1);
        buf = (BowArrowBuf *) o->buf;
        buf->charge = (s8) pl->chargeLevel & 3;
        BowArrow_InitAttack(buf, pl);
        BowArrow_SkillFlags(buf, pl);
        buf->crit = crit;
        buf->shot = shot;
    }
}

/*
 * Fires the arrows of shot level shot: one per kind in
 * BowArrow_ShotParams[shot] (ended by -1), fanned by BowArrow_SpreadDirs
 * times the shot's BowArrow_SpreadScale (levels 4-7 spread wider). One
 * affinity roll (WSTAT_AFFINITY) gives the first three arrows a critical (1)
 * or weak (2) hit.
 */
void BowArrow_Fire(Player *pl, s16 shot)
{
    u8 i;
    u8 crit;
    s16 *kinds;
    u16 spread;
    u8 arrowCrit;
    s16 affinity;

    kinds = BowArrow_ShotParams[shot];
    i = 0;

    switch (shot) {
    default:
        spread = 0;
        break;
    case 4:
        spread = 1;
        break;
    case 5:
        spread = 2;
        break;
    case 6:
        spread = 3;
        break;
    case 7:
        spread = 4;
        break;
    }
    crit = 0;
    affinity = Player_GetWeaponStat(pl, WSTAT_AFFINITY);
    if (affinity < 0) {
        if (pl->stateRand % 100 < -affinity) {
            crit = 2;
        }
    } else {
        if (affinity > pl->stateRand % 100) {
            crit = 1;
        }
    }
    for (; *kinds != -1; kinds++, i++) {
        if (i <= 2) {
            arrowCrit = crit;
        } else {
            arrowCrit = 0;
        }
        BowArrow_Create(pl, BowArrow_SpreadScale[spread] * BowArrow_SpreadDirs[i], *kinds, i, arrowCrit, shot);
    }
}

/*
 * The arrow's per-frame update, by ObjAttach.state: ATTACH_HIT goes to
 * BowArrow_Hit, ATTACH_DEFLECTED and ATTACH_DEFLECTED_AURA to
 * BowArrow_Deflect.
 */
void BowArrow_Update(ObjAttach *o)
{
    switch (o->state) {
    case ATTACH_SETUP:
        BowArrow_Setup(o);
        break;
    case ATTACH_RUN:
        BowArrow_Run(o);
        break;
    case ATTACH_END:
        BowArrow_End(o);
        break;
    case ATTACH_HIT:
        BowArrow_Hit(o);
        break;
    case 4:
        BowArrow_End(o);
        break;
    case ATTACH_DEFLECTED:
        BowArrow_Deflect(o);
        break;
    case ATTACH_DEFLECTED_AURA:
        BowArrow_Deflect(o);
        break;
    case 7:
        BowArrow_End(o);
        break;
    case ATTACH_FREE:
        BowArrow_Free(o);
        
    default:
        break;
    }
}

/*
 * Sets the arrow up from row type + 1 of BowArrow_Rows and gives it its power
 * (BowArrow_ApplyPower). Status coatings are not deflected by wind (no
 * ATKF_WIND_DEFLECTS). It starts at the owner's bone 14, aimed along it plus
 * rotX/rotY. Speed and gravity come from its BowArrowKind and
 * BowArrow_ChargeFlight row with a random part, doubled or halved by the
 * range skills. Takes a draw slot and runs its first frame.
 */
void BowArrow_Setup(ObjAttach *o)
{
    BowArrowBuf *arrowBuf = (BowArrowBuf *) o->buf;
    BowArrowKind *row = &BowArrow_Kinds[o->type];
    Em *pl = o->owner;
    f32 dir[3];
    u16 pitch;
    u16 heading;
    void *bone;
    f32 *chargeRow;
    f32 speedRand;
    f32 gravityRand;

    o->state++;
    o->visible = 1;
    o->draw = NULL;
    ObjAttach_StartOwnedAttack(o, (s32) pl, o->type + 1, &BowArrow_RowsPtr);
    BowArrow_ApplyPower(o);
    switch (o->elementFlags) {
    case 2:
    case 1:
    case 4:
        Player_TestSetFlags(o, ATKF_SHELL_DEFLECTS | ATKF_SHOT);
        break;
    default:
        Player_TestSetFlags(o, ATKF_SHELL_DEFLECTS | ATKF_SHOT | ATKF_WIND_DEFLECTS);
        break;
    }
    arrowBuf->power = o->power;
    o->hitList = BowArrow_HitShapes[o->hitListNo];
    o->hurtList = 0;
    o->step = 0;
    o->stateVar0 = 0;
    arrowBuf->bounced = 0;
    arrowBuf->event = 0;
    o->ownerMotion = 0;
    dir[0] = 1.0f;
    dir[1] = 0.0f;
    dir[2] = 0.0f;
    bone = Em_GetBone(pl, 14);
    Vec_RotateSelf(dir, bone);
    Mtx_GetTrans(o->pos, bone);
    Vec_DirToAngles(dir, (s16 *) &pitch, (s16 *) &heading);
    o->rotX += pitch;
    o->rotY += heading;
    o->yaw = o->rotY;
    chargeRow = BowArrow_ChargeFlight[arrowBuf->charge];
    speedRand = row->speedRand * chargeRow[1];
    gravityRand = row->gravityRand * chargeRow[3];
    if (arrowBuf->skills & 2) {
        speedRand *= 2.0f;
        gravityRand *= 2.0f;
    }
    if (arrowBuf->skills & 4) {
        speedRand *= 0.5f;
        gravityRand *= 0.5f;
    }
    o->velX = 0.0f;
    o->velY = 0.0f;
    o->velZ = row->speed + chargeRow[0] + speedRand * BowArrow_BitsToFrac(o->randBits, 16);
    switch (row->keepSpeed) {
    case 0:
        o->velZ *= 1.0f - 0.2f * o->stateVar1;
        break;
    }
    Vec_RotateX(&o->velX, (6.2831855f * o->rotX) / 65536.0f);
    Vec_RotateY(&o->velX, (6.2831855f * o->rotY) / 65536.0f);
    o->homingTurn = 0.0f;
    o->gravity = -(row->gravity + chargeRow[2]) + gravityRand * BowArrow_BitsToFrac(o->randBits >> 1, 15);
    o->homingSpeed = 0.0f;
    Vec_Copy(o->pos2, o->pos);
    if (row->eventTbl != -1) {
        BowArrow_ApplyEvent(o);
    }
    o->drawSlot = DrawSlot_Alloc();
    if (o->drawSlot != -1) {
        o->drawSlotBuf = DrawSlot_Get(o->drawSlot);
        o->drawSlotBuf->owner = o;
        o->drawSlotBuf->draw = BowArrow_Draw;
        BowArrow_Run(o);
    } else {
        o->drawSlotBuf = NULL;
    }
}

/*
 * One flying frame of an arrow. After a hit, a hitMode 2 (piercing) arrow
 * restarts its row 2 frames later at 70% speed; a hitMode 1 arrow rides
 * targetEm's bone aux until ownerMotion reaches count. Adds the kind's random
 * sway (first 20 frames) and lift, flies straight for straightFrames, then
 * with gravity, and ends at the floor. Steps the kind's events
 * (BowArrow_StepEvents, BowArrow_ApplyEvent).
 */
void BowArrow_Move(ObjAttach *o)
{
    BowArrowBuf *arrowBuf = (BowArrowBuf *) o->buf;
    BowArrowKind *row = &BowArrow_Kinds[o->type];
    f32 *chargeRow = BowArrow_ChargeFlight[arrowBuf->charge];
    f32 drift[3];
    f32 bonePos[3];
    f32 ofs[3];
    f32 mat[16];
    f32 len;
    f32 sway;
    Em *em;

    Vec_Copy(o->pos2, o->pos);
    o->hitFrames = 99;
    if (o->stateVar0 > 0) {
        switch (row->hitMode) {
        case 1:
            if (!(o->ownerMotion < row->count)) {
                o->state = ATTACH_END;
                return;
            }
            em = o->targetEm;
            if (em == NULL) {
                o->state = ATTACH_END;
                return;
            }
            if (em->taskPhase == 3 || em->active == 0) {
                o->state = ATTACH_END;
                return;
            }
            Mtx_Copy(mat, (f32 *)Em_GetBone(em, o->aux));
            Mtx_GetTrans(bonePos, mat);
            Vec_Rotate(ofs, &o->velX, mat);
            o->pos[0] = ofs[0] + bonePos[0];
            o->pos[1] = ofs[1] + bonePos[1];
            o->pos[2] = ofs[2] + bonePos[2];
            return;
        case 2:
            if (o->aux != 0) {
                if (--o->aux == 0) {
                    ObjAttach_StartOwnedAttack(o, (s32) o->owner, o->type + 1, &BowArrow_RowsPtr);
                    BowArrow_ApplyPower(o);
                    o->hitList = BowArrow_HitShapes[0];
                    o->hurtList = 0;
                    o->velX *= 0.7f;
                    o->velY *= 0.7f;
                    o->velZ *= 0.7f;
                    BowArrow_ApplyEvent(o);
                }
            }
            break;
        }
    }
    len = Math_Sqrtf(o->velX * o->velX + o->velZ * o->velZ);
    if (o->ownerMotion <= 19 && BowArrow_MulAddShift(o->randBits, o->ownerMotion, 0x3045, 1) <= 0x1FFF) {
        sway = row->sway * chargeRow[5];
        if (arrowBuf->skills & 2) {
            sway *= 2.0f;
        }
        if (arrowBuf->skills & 4) {
            sway *= 0.5f;
        }
        drift[0] = sway * BowArrow_BitsToSignedFrac(BowArrow_MulAddShift(o->randBits, o->ownerMotion, 0x3121, 4), 6);
    } else {
        drift[0] = 0.0f;
    }
    drift[1] = len * (row->lift + chargeRow[4]) * BowArrow_BitsToFrac(BowArrow_MulAddShift(o->randBits, o->ownerMotion, 0x147F, 2), 14);
    drift[2] = 0.0f;
    Vec_RotateX(drift, (6.2831855f * o->rotX) / 65536.0f);
    Vec_RotateY(drift, (6.2831855f * o->rotY) / 65536.0f);
    Vec_Add(&o->velX, &o->velX, drift);
    if (o->ownerMotion < row->straightFrames) {
        ObjAttach_Move(o);
    } else {
        ObjAttach_MoveAccel(o);
    }
    if (o->pos[1] <= AreaGrid2_GetFloorYFiltered(o->pos)) {
        o->hitFrames = 0;
        o->state = ATTACH_END;
        return;
    }
    if (BowArrow_StepEvents(o)) {
        BowArrow_ApplyEvent(o);
        if (arrowBuf->event == 0xFF) {
            arrowBuf->doneFrames = 0;
        }
    }
}

/*
 * Moves the arrow: a bounced one (stateVar1 0xFE) tumbles
 * (BowArrow_BounceStep). Otherwise it ends after 100 frames or 3 frames past
 * its last event; a batted-back arrow (stateVar1 0xFF) flies by
 * BowArrow_Fall, the others by BowArrow_Move. Moves its draw slot.
 */
void BowArrow_Run(ObjAttach *o)
{
    BowArrowBuf *buf = (BowArrowBuf *) o->buf;

    if (o->stateVar1 == 0xFE) {
        if (BowArrow_BounceStep(o) != 0) {
            o->state = ATTACH_END;
            return;
        }
    } else {
        if (++o->ownerMotion > 100) {
            o->hitFrames = 0;
            o->state = ATTACH_END;
            return;
        }
        if (o->stateVar1 != 0xFF) {
            if (buf->event == 0xFF) {
                if (++buf->doneFrames > 3) {
                    o->hitFrames = 0;
                    o->state = ATTACH_END;
                    return;
                }
            }
            BowArrow_Move(o);
        } else {
            BowArrow_Fall(o);
        }
    }
    if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
    }
}

/* Hides the arrow, frees its draw slot and frees it on the next update. */
void BowArrow_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = ATTACH_FREE;
    o->visible = 0;
    if (o->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) o->drawSlot);
    }
}

/* Returns the object to the pool. */
void BowArrow_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

/*
 * On a hit: a batted-back arrow ends. A piercing arrow (hitMode 2) counts the
 * hit and flies on, reopening its window 2 frames later while hits are left;
 * the others end.
 */
void BowArrow_Hit(ObjAttach *o)
{
    BowArrowKind *row = &BowArrow_Kinds[o->type];

    if (o->stateVar1 == 0xFF) {
        o->state = ATTACH_END;
    } else {
        switch (row->hitMode) {
        case 0:
        default:
            o->state = ATTACH_END;
            break;
        case 1:
            o->state = ATTACH_END;
            break;
        case 2:
            if (++o->stateVar0 < row->count) {
                o->aux = 2;
            } else {
                o->hitPhase = 0;
            }
            o->state = ATTACH_RUN;
            if (++o->ownerMotion > 100) {
                o->hitFrames = 0;
                o->state = ATTACH_END;
                return;
            }
            BowArrow_Move(o);
            break;
        }
    }
    if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
    }
}

/*
 * A deflected arrow (ATTACH_DEFLECTED, ATTACH_DEFLECTED_AURA). Either it
 * bounces off at a random angle (stateVar1 0xFE), or, when ObjAttach_RollBatBack says
 * so, the monster in targetEm bats it back. It then becomes that monster's
 * attack (row 12), flying at a point 100 above its shooter's feet (stateVar1
 * 0xFF), with SE 0x96.
 */
void BowArrow_Deflect(ObjAttach *o)
{
    Em *owner = o->owner;
    Em *batter = o->targetEm;
    f32 aim[3];
    f32 dir[3];
    u16 pitch;
    u16 heading;

    o->step = 0;
    o->stateVar0 = 0;
    o->ownerMotion = 0;
    o->side = 0;
    if (ObjAttach_RollBatBack(o, o->randBits >> o->stateVar1) == 0) {
        o->stateVar1 = 0xFE;
        o->hitPhase = 0;
        Vec_Diff(dir, o->pos2, o->pos);
        Vec_DirToAngles(dir, (s16 *) &pitch, (s16 *) &heading);
        pitch += -(Rand_Next(1) & 0x7FF) - 0x200;
        heading += (Rand_Next(1) & 0x3FFF) - 0x2000;
        o->velX = 0.0f;
        o->velY = 1.0f + (Rand_Next(1) & 0x1F);
        o->velZ = 10.0f + (Rand_Next(1) & 7);
        o->homingTurn = 0.0f;
        o->gravity = -1.8f - (Rand_Next(1) & 0x1F) / 32.0f;
        o->homingSpeed = 0.0f;
        Vec_RotateX(&o->velX, (6.2831855f * pitch) / 65536.0f);
        Vec_RotateY(&o->velX, (6.2831855f * heading) / 65536.0f);
        o->rotX = Rand_Next(1);
        o->rotY = Rand_Next(1);
    } else {
        o->stateVar1 = 0xFF;
        o->idx = batter->index;
        o->isEm = batter->isEm;
        o->owner = batter;
        Player_TestSetFlags(o, 0);
        ObjAttach_StartOwnedAttack(o, (s32) batter, 12, &BowArrow_RowsPtr);
        o->hitList = BowArrow_HitShapes[o->hitListNo];
        o->hurtList = 0;
        Vec_Copy(aim, &owner->posX);
        aim[1] += 100.0f;
        Vec_Diff(dir, aim, o->pos);
        Vec_DirToAngles(dir, (s16 *) &pitch, (s16 *) &heading);
        o->rotX = pitch;
        o->rotY = heading;
        o->yaw = o->rotY;
        o->velX = 0.0f;
        o->velY = 0.0f;
        o->velZ = 100.0f;
        Vec_RotateX(&o->velX, (6.2831855f * o->rotX) / 65536.0f);
        Vec_RotateY(&o->velX, (6.2831855f * o->rotY) / 65536.0f);
        o->homingTurn = 0.0f;
        o->gravity = -1.5f;
        o->homingSpeed = 0.0f;
        Vec_Copy(o->pos2, o->pos);
        if (gScene.area == o->area) {
            Sound_PlaySeAtPosChecked(1, 0x96, 0, o->pos, 6, 0);
        }
    }
    o->state = ATTACH_RUN;
    BowArrow_Run(o);
}

/*
 * The flight of a batted-back arrow: straight for 20 frames, then with
 * gravity; it ends at the floor.
 */
void BowArrow_Fall(ObjAttach *o)
{
    Vec_Copy(o->pos2, o->pos);
    o->hitFrames = 0x63;
    if (o->ownerMotion < 0x14) {
        ObjAttach_Move(o);
    } else {
        ObjAttach_MoveAccel(o);
    }
    if (o->pos[1] <= AreaGrid2_GetFloorYFiltered(o->pos)) {
        o->hitFrames = 0;
        o->state = ATTACH_END;
    }
}

/*
 * One frame of a bounced arrow's tumble: no hit, buf->bounced set, it falls
 * and spins. Returns 1 after 20 frames (the arrow ends), else 0.
 */
u8 BowArrow_BounceStep(ObjAttach *o)
{
    BowArrowBuf *buf = (BowArrowBuf *) o->buf;

    o->hitPhase = 0;
    buf->bounced = 1;
    if (++o->ownerMotion > 20) {
        return 1;
    }
    ObjAttach_MoveAccel(o);
    o->rotX -= 0x400;
    return 0;
}

/*
 * Draws the arrow (common model 149) at its draw slot, in the current area
 * only. It is turned along its velocity, or by rotX/rotY once bounced.
 */
void BowArrow_Draw(DrawSlot *slot)
{
    u16 pitch;
    u16 yaw;
    f32 mtx[16];
    ObjAttach *o;
    BowArrowBuf *buf;
    ModelMat *tex;
    Model *set;
    ModelMatSet *mdl;

    set = gCommonModels[0];
    o = slot->owner;
    buf = (BowArrowBuf *) o->buf;
    if (o->area == gScene.area && set != NULL && set->active != 0) {
        tex = set->mats;
        Mtx_Translation(mtx, slot->pos[0], slot->pos[1], slot->pos[2]);
        mdl = &set->matSets[149];
        if (buf->bounced == 0) {
            Vec_DirToAngles(&o->velX, (s16 *) &pitch, (s16 *) &yaw);
            Mtx_RotateX(mtx, (6.2831855f * pitch) / 65536.0f);
            Mtx_RotateY(mtx, (6.2831855f * yaw) / 65536.0f);
        } else {
            Mtx_RotateX(mtx, (6.2831855f * o->rotX) / 65536.0f);
            Mtx_RotateY(mtx, (6.2831855f * o->rotY) / 65536.0f);
        }
        Gs_SetState(0x1A, (s32) mtx);
        Gs_SetState(0x67, -1);
        Gs_SetState(0x60, 0x80);
        if (mdl != NULL && mdl->id != -1) {
            Gs_SetLightList(tex, mdl);
            Draw_ApplyStates(mdl->flags);
            Model_Draw(mdl->id, 0);
        }
        Draw_SetDefaultState();
    }
}

/*
 * Sets buf->skills from pl's skills: 1 SKILL_STATUS_PLUS (coatings x1.125), 2 skill
 * 0xBA (range x2), 4 skill 0xB9 (range x0.5).
 */
void BowArrow_SkillFlags(BowArrowBuf *buf, Player *pl)
{
    buf->skills = 0;
    if (Player_HasSkill(pl, SKILL_STATUS_PLUS) == 1) {
        buf->skills |= 1;
    }
    if (Player_HasSkill(pl, 0xBA) == 1) {
        buf->skills |= 2;
    }
    if (Player_HasSkill(pl, 0xB9) == 1) {
        buf->skills |= 4;
    }
}

/*
 * Gives the player's weapon element as ELEMF_* bits in *flags and its value
 * in *level (0: none): fire, water, thunder, ice or dragon
 * (Player_GetWeaponStat).
 */
void Player_WeaponElementFlags(void *pl, u16 *flags, s8 *level)
{
    s16 stat;

    *flags = 0;
    *level = 0;
    stat = Player_GetWeaponStat(pl, WSTAT_FIRE);
    if (stat > 0) {
        *flags |= ELEMF_FIRE;
        *level = stat;
    }
    stat = Player_GetWeaponStat(pl, WSTAT_WATER);
    if (stat > 0) {
        *flags |= ELEMF_WATER;
        *level = stat;
    }
    stat = Player_GetWeaponStat(pl, WSTAT_THUNDER);
    if (stat > 0) {
        *flags |= ELEMF_THUNDER;
        *level = stat;
    }
    stat = Player_GetWeaponStat(pl, WSTAT_ICE);
    if (stat > 0) {
        *flags |= ELEMF_ICE;
        *level = stat;
    }
    stat = Player_GetWeaponStat(pl, WSTAT_DRAGON);
    if (stat > 0) {
        *flags |= ELEMF_DRAGON;
        *level = stat;
    }
}

/*
 * Copies pl's attackFactor, ammoType (the loaded coating) and weapon element
 * into the arrow's buffer.
 */
void BowArrow_InitAttack(BowArrowBuf *buf, Player *pl)
{
    buf->attackFactor = pl->attackFactor;
    buf->ammoType = pl->ammoType;
    Player_WeaponElementFlags(pl, &buf->elementFlags, (s8 *) &buf->elementLevel);
}

/*
 * Applies a bow coating to o: 1 power (x1.5 power, only when stabbing), 2
 * poison, 3 ELEMF_EM_STATUS4, 4 ELEMF_EM_STATUS2. stab is 1 for the bow's
 * close stab (PlayerAttack_Setup) and 0 for an arrow, whose hitFx is then
 * 0x18. On the stab a status coating gets elementPower 2, otherwise the
 * weapon's element applies; status values get SKILL_STATUS_PLUS's x1.125 and
 * elements are halved (at least 1).
 */
void BowArrow_ApplyCoating(ObjAttach *o, u8 coating, u8 stab)
{
    f32 value;
    f32 skillScale;

    o->elementFlags = 0;
    o->elementPower = 0;
    switch (coating) {
    case 2:
        o->elementFlags = ELEMF_POISON;
        if (stab == 0) {
            o->hitFx = 0x18;
        }
        break;
    case 3:
        o->elementFlags = ELEMF_EM_STATUS4;
        if (stab == 0) {
            o->hitFx = 0x18;
        }
        break;
    case 4:
        o->elementFlags = ELEMF_EM_STATUS2;
        if (stab == 0) {
            o->hitFx = 0x18;
        }
        break;
    case 1:
        if (stab != 0) {
            value = o->power;
            value *= 1.5f;
            if (value > 255.0f) {
                value = 255.0f;
            }
            o->power = value;
        }
        break;
    }
    if (stab != 0) {
        if (o->elementFlags != 0) {
            o->elementPower = 2;
        } else {
            Player_WeaponElementFlags(o->owner, &o->elementFlags, (s8 *) &o->elementPower);
        }
    }
    if (o->elementPower > 0 && stab != 0) {
        switch (o->elementFlags) {
        case ELEMF_POISON:
        case ELEMF_EM_STATUS4:
        case ELEMF_EM_STATUS2:
            if (Player_HasSkill(o->owner, SKILL_STATUS_PLUS) == 1) {
                skillScale = 1.125f;
            } else {
                skillScale = 1.0f;
            }
            value = o->elementPower * skillScale;
            if (value > 255.0f) {
                value = 255.0f;
            }
            o->elementPower = value;
            break;
        default:
            o->elementPower /= 2;
            break;
        }
        if (o->elementPower == 0) {
            o->elementPower = 1;
        }
    }
}

/*
 * Sets the arrow's element and power at set up (not for attackKind 8). A
 * coating's status power is BowArrow_ChargeCoatingPower[charge] x
 * BowArrow_LevelScale[shot]; with no coating, the weapon's element is scaled
 * by BowArrow_ChargeElementScale. Power is multiplied by attackFactor (x1.5
 * with the power coating) and BowArrow_ChargePowerScale[charge], x1.25 on a
 * critical or x0.5 on a weak hit, clamped to 1-255.
 */
void BowArrow_ApplyPower(ObjAttach *o)
{
    BowArrowBuf *arrowBuf = (BowArrowBuf *) o->buf;
    f32 scale;
    u16 value;

    if (o->attackKind == 8) {
        return;
    }
    BowArrow_ApplyCoating(o, arrowBuf->ammoType, 0);
    if (o->elementFlags == 0) {
        o->elementFlags = arrowBuf->elementFlags;
        o->elementPower = arrowBuf->elementLevel;
        if (o->elementPower != 0) {
            o->hitFx = 0x18;
            value = o->elementPower * BowArrow_ChargeElementScale[arrowBuf->charge];
            if (value == 0) {
                value = 1;
            } else if (value > 0xFF) {
                value = 0xFF;
            }
            o->elementPower = value;
        }
    } else {
        scale = BowArrow_ChargeCoatingPower[arrowBuf->charge];
        if (arrowBuf->skills & 1) {
            scale *= 1.125f;
        }
        value = BowArrow_LevelScale[arrowBuf->shot] * scale;
        if (value > 0xFF) {
            value = 0xFF;
        }
        o->elementPower = value;
    }
    scale = arrowBuf->attackFactor;
    if (arrowBuf->ammoType == 1) {
        scale *= 1.5f;
    }
    if (o->power != 0) {
        scale *= BowArrow_ChargePowerScale[arrowBuf->charge];
        if (arrowBuf->crit == 1) {
            scale *= 1.25f;
            o->hitFx |= HITFX_CRITICAL;
        } else if (arrowBuf->crit == 2) {
            scale *= 0.5f;
            o->hitFx |= HITFX_WEAK;
        }
        value = o->power * scale;
        if (value == 0) {
            value = 1;
        } else if (value > 0xFF) {
            value = 0xFF;
        }
        o->power = value;
    }
}

/*
 * Moves buf->event past the entries of the kind's event table whose frame
 * ownerMotion has passed; a negative entry ends the list (event 0xFF, hit
 * window closed). Returns 1 when an entry passed, 0 otherwise or when the
 * kind has none.
 */
s8 BowArrow_StepEvents(ObjAttach *o)
{
    BowArrowKind *row = &BowArrow_Kinds[o->type];
    BowArrowBuf *buf = (BowArrowBuf *) o->buf;
    s8 ret = 0;
    s16 *tbl;

    if (row->eventTbl == -1) {
        return ret;
    }
    tbl = BowArrow_EventTbls[row->eventTbl];
    for (; buf->event != 0xFF; buf->event++) {
        if (tbl[buf->event] >= o->ownerMotion) {
            break;
        }
        ret = 1;
        if (tbl[buf->event] < 0) {
            buf->event = 0xFF;
            o->hitPhase = 0;
            break;
        }
    }
    return ret;
}

/*
 * Applies event buf->event (0-4) to the arrow. If its hit effect is 10, 12,
 * 14 or 16 it takes BowArrow_EventHitFx and BowArrow_EventHitSe (keeping the
 * critical bits). Its power becomes buf->power x BowArrow_EventPowerScale,
 * clamped to 1-255.
 */
void BowArrow_ApplyEvent(ObjAttach *o)
{
    BowArrowBuf *arrowBuf = (BowArrowBuf *) o->buf;
    f32 newPower;

    if (o->power == 0 || arrowBuf->event > 4 || arrowBuf->event == 0xFF) {
        return;
    }
    switch (o->hitFx & 0x3F) {
    case 10:
    case 12:
    case 14:
    case 16:
        o->hitFx = BowArrow_EventHitFx[arrowBuf->event];
        if (arrowBuf->crit == 1) {
            o->hitFx |= HITFX_CRITICAL;
        } else if (arrowBuf->crit == 2) {
            o->hitFx |= HITFX_WEAK;
        }
        o->hitSe = BowArrow_EventHitSe[arrowBuf->event];
        break;
    }
    if (arrowBuf->power != 0.0f) {
        newPower = arrowBuf->power * BowArrow_EventPowerScale[arrowBuf->event];
        if (!(newPower < 255.0f)) {
            o->power = 255;
        } else if (newPower <= 1.0f) {
            o->power = 1;
        } else {
            o->power = newPower;
        }
    }
}

/*
 * Returns (randBits + ownerMotion * mul) >> shift as 16 bits: a pseudo-random
 * value that changes every frame.
 */
u16 BowArrow_MulAddShift(u16 randBits, u16 ownerMotion, s32 mul, s32 shift)
{
    return ((s32) ((randBits & 0xFFFF) + ((ownerMotion & 0xFFFF) * (mul & 0xFFFF))) >> (shift & 0xFFFF)) & 0xFFFF;
}

/*
 * Returns the low bits bits of randBits, moved to the top of 16 bits, times
 * 0.000015625 (0 to about 1.02).
 */
f32 BowArrow_BitsToFrac(u16 randBits, s32 bits)
{
    s32 shift;

    shift = (0x10 - (bits & 0xFFFF)) & 0xFFFF;
    return 0.000015625f * (f32) ((randBits & 0xFFFF & ((0xFFFF >> shift) & 0xFFFF)) << shift);
}

/* As BowArrow_BitsToFrac but centred on 0 (about -0.51 to 0.51). */
f32 BowArrow_BitsToSignedFrac(u16 value, u16 bits)
{
    u16 shift = 16 - bits;

    return 0.000015625f * (s16) (((value & (u16) (0xFFFF >> shift)) << shift) - 0x8000);
}

/*
 * Gives arrow kind idx's start velocity (0, 0, speed) in vel and its gravity
 * and straightFrames, for the bow's aim arc (BowArc_Draw).
 */
void BowArrow_FlightParams(s16 idx, s32 *vel, f32 *gravity, s16 *straightFrames)
{
    BowArrowKind *row;

    vel[0] = 0;
    vel[1] = 0;
    row = &BowArrow_Kinds[idx];
    *(f32 *) &vel[2] = row->speed;
    *gravity = row->gravity;
    *straightFrames = row->straightFrames;
}

/*
 * Makes a GarugaAttack, a hit zone of Yian Garuga's, for em with the row kind
 * of GarugaAttack_Rows, if a player is in em's area. Clears em's hitPending
 * and contactHit.
 */
void GarugaAttack_Create(Em *em, s8 kind)
{
    ObjAttach *o;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((o = ObjAttach_Alloc(0)) != NULL) {
            o->kind = ATTACH_GARUGA_ATTACK;
            o->type = kind;
            o->update = GarugaAttack_Update;
            o->idx = em->index;
            o->isEm = em->isEm;
            o->ownerMotion = em->motionId[0];
            o->owner = em;
            o->yaw = em->rotY;
            o->rotX = em->rotX;
            o->rotY = em->rotY;
            o->rotZ = em->rotZ;
            o->side = *(u8 *) &em->side;
            em->hitPending = 0;
            em->contactHit = 0;
        }
    }
}

/*
 * The same as GarugaAttack_Create; EmGaruga_MotionEffects makes type 0x16
 * with it.
 */
void GarugaAttack_CreateForEffects(Em *em, s8 kind)
{
    ObjAttach *o;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((o = ObjAttach_Alloc(0)) != NULL) {
            o->kind = ATTACH_GARUGA_ATTACK;
            o->type = kind;
            o->update = GarugaAttack_Update;
            o->idx = em->index;
            o->isEm = em->isEm;
            o->ownerMotion = em->motionId[0];
            o->owner = em;
            o->yaw = em->rotY;
            o->rotX = em->rotX;
            o->rotY = em->rotY;
            o->rotZ = em->rotZ;
            o->side = *(u8 *) &em->side;
            em->hitPending = 0;
            em->contactHit = 0;
        }
    }
}

/* The GarugaAttack's per-frame update, by ObjAttach.state. */
void GarugaAttack_Update(ObjAttach *o)
{
    switch (o->state) {
    case ATTACH_SETUP:
        GarugaAttack_Setup(o);
        break;
    case ATTACH_RUN:
        GarugaAttack_Run(o);
        break;
    case ATTACH_END:
        GarugaAttack_End(o);
        break;
    case ATTACH_HIT:
        GarugaAttack_Run(o);
        break;
    case 4:
        GarugaAttack_End(o);
        break;
    case ATTACH_DEFLECTED:
        GarugaAttack_End(o);
        break;
    case ATTACH_DEFLECTED_AURA:
        GarugaAttack_End(o);
        break;
    case 7:
        GarugaAttack_End(o);
        break;
    case ATTACH_FREE:
        GarugaAttack_Free(o);
        
    default:
        break;
    }
}

/*
 * Sets the attack up from its row: the hit shapes and ATKF_OWNER_CONTACT;
 * hitDelay is counted from the monster's motion.
 */
void GarugaAttack_Setup(ObjAttach *o)
{
    Em *em = &gEms[o->idx];
    s32 blend;

    o->state++;
    o->visible = 1;
    o->draw = NULL;
    Player_TestSetFlags(o, ATKF_OWNER_CONTACT);
    ObjAttach_StartOwnedAttack(o, (s32) em, o->type, &GarugaAttack_RowsPtr);
    o->hitList = GarugaAttack_HitShapes[o->hitListNo];
    o->hurtList = 0;
    o->area = em->area;
    if (o->hitDelay != 0) {
        blend = Math_Absf(em->motionBlend[0] % 100);
        o->hitDelay += blend - (s32) Math_Absf(em->motionFrame[0]) - 1;
        if ((s8) o->hitDelay < 0) {
            o->hitDelay = 0;
        }
    }
    o->drawSlotBuf = NULL;
}

/*
 * Ends the attack when the monster's motion changes or the hit window closes;
 * types 19, 23 and 28 keep their window open.
 */
void GarugaAttack_Run(ObjAttach *o)
{
    Em *em = &gEms[o->idx];

    switch (o->type) {
    case 19:
    case 23:
    case 28:
        o->hitFrames = 99;
        break;
    }
    if (em->motionId[0] != o->ownerMotion || o->hitPhase == 0) {
        o->hitPhase = 0;
        o->state = ATTACH_END;
    }
    if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
    }
}

/* Hides the attack, frees its draw slot and frees it on the next update. */
void GarugaAttack_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = ATTACH_FREE;
    o->visible = 0;
    if (o->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) o->drawSlot);
    }
}

/* Returns the object to the pool. */
void GarugaAttack_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

/*
 * Makes a HermitaurAttack for em with the row kind of HermitaurAttack_Rows,
 * if a player is in em's area. It is a hit zone of the Hermitaur family's
 * (EmHermitaur_*: Hermitaur, Daimyo Hermitaur, Shogun Ceanataur). Type 9
 * faces backward (yaw + 0x8000). Clears em's hitPending and contactHit.
 */
void HermitaurAttack_Create(Em *em, s32 kind)
{
    ObjAttach *o;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((o = ObjAttach_Alloc(0)) != NULL) {
            o->kind = ATTACH_HERMITAUR_ATTACK;
            o->type = kind;
            o->update = HermitaurAttack_Update;
            o->idx = em->index;
            o->isEm = em->isEm;
            o->ownerMotion = em->motionId[0];
            o->owner = em;
            o->yaw = em->rotY;
            if (kind == 9) {
                *(u16 *) &o->yaw += 0x8000;
            }
            o->rotX = em->rotX;
            o->rotY = em->rotY;
            o->rotZ = em->rotZ;
            o->side = *(u8 *) &em->side;
            em->hitPending = 0;
            em->contactHit = 0;
        }
    }
}

/* The HermitaurAttack's per-frame update, by ObjAttach.state. */
void HermitaurAttack_Update(ObjAttach *o)
{
    switch (o->state) {
    case ATTACH_SETUP:
        HermitaurAttack_Setup(o);
        break;
    case ATTACH_RUN:
        HermitaurAttack_Run(o);
        break;
    case ATTACH_END:
        HermitaurAttack_End(o);
        break;
    case ATTACH_HIT:
        HermitaurAttack_Run(o);
        break;
    case 4:
        HermitaurAttack_End(o);
        break;
    case ATTACH_DEFLECTED:
        HermitaurAttack_End(o);
        break;
    case ATTACH_DEFLECTED_AURA:
        HermitaurAttack_End(o);
        break;
    case 7:
        HermitaurAttack_End(o);
        break;
    case ATTACH_FREE:
        HermitaurAttack_Free(o);
        
    default:
        break;
    }
}

/*
 * Sets the attack up from its row (hit shapes, ATKF_OWNER_CONTACT) at the
 * monster's position; type 11 starts on the floor. Hit list 5 grows with the
 * monster (hitScale = scaleX). hitDelay is counted from the monster's motion.
 */
void HermitaurAttack_Setup(ObjAttach *o)
{
    Em *em = &gEms[o->idx];
    s32 blend;

    o->state++;
    o->visible = 1;
    o->draw = NULL;
    Player_TestSetFlags(o, ATKF_OWNER_CONTACT);
    ObjAttach_StartOwnedAttack(o, (s32) em, o->type, &HermitaurAttack_RowsPtr);
    o->hitList = HermitaurAttack_HitShapes[o->hitListNo];
    o->hurtList = 0;
    o->area = em->area;
    switch (o->type) {
    default:
        Vec_Copy(o->pos, &em->posX);
        break;
    case 11:
        Vec_Copy(o->pos, &em->posX);
        o->pos[1] = em->floorY;
        break;
    }
    switch (o->hitListNo) {
    case 5:
        o->hitScale = em->scaleX;
        break;
    default:
        o->hitScale = 1.0f;
        break;
    }
    if (o->hitDelay != 0) {
        blend = Math_Absf(em->motionBlend[0] % 100);
        o->hitDelay += blend - (s32) Math_Absf(em->motionFrame[0]) - 1;
        if ((s8) o->hitDelay < 0) {
            o->hitDelay = 0;
        }
    }
    o->drawSlotBuf = NULL;
}

/*
 * Ends the attack when the monster's motion changes or the window closes;
 * types 9 and 27 (EmHermitaur_WalkAlert) keep it open. Type 11 is Daimyo
 * Hermitaur's leap (EmHermitaur_LeapPress). It stays open in the air motion
 * and at frame 4 of the landing becomes hit list 5 on the floor for 8 frames.
 */
void HermitaurAttack_Run(ObjAttach *o)
{
    u8 kind = o->type;
    Em *em = &gEms[o->idx];

    switch (kind) {
    default:
        if (em->motionId[0] != o->ownerMotion || o->hitPhase == 0) {
            o->hitPhase = 0;
            o->state = ATTACH_END;
        }
        break;
    case 9:
    case 27:
        o->hitFrames = 99;
        if (em->motionId[0] != o->ownerMotion || o->hitPhase == 0) {
            o->hitPhase = 0;
            o->state = ATTACH_END;
        }
        break;
    case 11:
        if (em->motionId[0] == EM_MOTION_ID(0, 0xA)) {
            o->hitFrames = 99;
        } else if (em->motionId[0] == EM_MOTION_ID(0, 0xB)) {
            if (EmMotion_PassedFrameScaled(em, 4.0f, 0) != 0 && o->stateVar0 == 0) {
                o->stateVar0++;
                o->hitListNo = 5;
                o->hitList = HermitaurAttack_HitShapes[o->hitListNo];
                o->hitFrames = 8;
                o->hitScale = em->scaleX;
                Vec_Copy(o->pos, &em->posX);
                o->pos[1] = em->floorY;
            }
        } else {
            o->hitPhase = 0;
            o->state = ATTACH_END;
        }
        if (o->hitPhase == 0) {
            o->hitPhase = 0;
            o->state = ATTACH_END;
        }
        break;
    }
    if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
    }
}

/* Hides the attack, frees its draw slot and frees it on the next update. */
void HermitaurAttack_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = ATTACH_FREE;
    o->visible = 0;
    if (o->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) o->drawSlot);
    }
}

/* Returns the object to the pool. */
void HermitaurAttack_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

/*
 * Makes a BlangongaAttack for em with the row kind of BlangongaAttack_Rows,
 * if a player is in em's area. It is a hit zone of the Blangonga family's
 * (EmBlangonga_*: Blangonga, Congalala, Rajang, Blango, Conga). Types 9, 18,
 * 20 and 30 face backward (yaw + 0x8000). Clears em's hitPending and
 * contactHit.
 */
void BlangongaAttack_Create(Em *em, s32 kind)
{
    ObjAttach *o;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((o = ObjAttach_Alloc(0)) != NULL) {
            o->kind = ATTACH_BLANGONGA_ATTACK;
            o->type = kind;
            o->update = BlangongaAttack_Update;
            o->idx = em->index;
            o->isEm = em->isEm;
            o->ownerMotion = em->motionId[0];
            o->owner = em;
            switch (kind) {
            case 9:
            case 18:
            case 20:
            case 30:
                o->yaw = em->rotY + 0x8000;
                break;
            default:
                o->yaw = em->rotY;
                break;
            }
            o->rotX = em->rotX;
            o->rotY = em->rotY;
            o->rotZ = em->rotZ;
            o->side = *(u8 *) &em->side;
            em->hitPending = 0;
            em->contactHit = 0;
        }
    }
}

/* The BlangongaAttack's per-frame update, by ObjAttach.state. */
void BlangongaAttack_Update(ObjAttach *o)
{
    switch (o->state) {
    case ATTACH_SETUP:
        BlangongaAttack_Setup(o);
        break;
    case ATTACH_RUN:
        BlangongaAttack_Run(o);
        break;
    case ATTACH_END:
        BlangongaAttack_End(o);
        break;
    case ATTACH_HIT:
        BlangongaAttack_Run(o);
        break;
    case 4:
        BlangongaAttack_End(o);
        break;
    case ATTACH_DEFLECTED:
        BlangongaAttack_End(o);
        break;
    case ATTACH_DEFLECTED_AURA:
        BlangongaAttack_End(o);
        break;
    case 7:
        BlangongaAttack_End(o);
        break;
    case ATTACH_FREE:
        BlangongaAttack_Free(o);
        
    default:
        break;
    }
}

/*
 * Sets the attack up from its row: the hit shapes and ATKF_OWNER_CONTACT, and
 * half power for the small kinds (EmBlangonga_IsSmallKind). hitDelay is
 * counted from the monster's motion; type 3 (EmBlangonga_Strike) uses motion
 * layer 1.
 */
void BlangongaAttack_Setup(ObjAttach *o)
{
    Em *em = &gEms[o->idx];
    s32 blend;
    s32 startFrame;

    o->state++;
    o->visible = 1;
    o->draw = NULL;
    Player_TestSetFlags(o, ATKF_OWNER_CONTACT);
    ObjAttach_StartOwnedAttack(o, (s32) em, o->type, &BlangongaAttack_RowsPtr);
    if (EmBlangonga_IsSmallKind(em) == 1) {
        o->power /= 2;
    }
    o->hitList = BlangongaAttack_HitShapes[o->hitListNo];
    o->hurtList = 0;
    o->area = em->area;
    if (o->hitDelay != 0) {
        if (o->type == 3) {
            blend = Math_Absf(em->motionBlend[1] % 100);
            startFrame = Math_Absf(em->motionFrame[1]);
        } else {
            blend = Math_Absf(em->motionBlend[0] % 100);
            startFrame = Math_Absf(em->motionFrame[0]);
        }
        o->hitDelay += blend - startFrame - 1;
        if ((s8) o->hitDelay < 0) {
            o->hitDelay = 0;
        }
    }
    o->drawSlotBuf = NULL;
}

/*
 * Ends the attack when the monster's motion changes or the window closes. The
 * charges' types 15, 29 and 35 stay open. 31 (EmBlangonga_CongalalaCombo)
 * changes hit lists by the combo's frames and at frame 176 closes for the
 * small kinds; 36 (EmBlangonga_RajangComboSlam) changes them by the frames of
 * BLANG_MOT_STRIKE_A. 33 (EmBlangonga_Hop) and 39 (EmBlangonga_RajangCharge)
 * stay open in their motion and last 20 or 26 frames into BLANG_MOT_HOP_LAND
 * or BLANG_MOT_RAJANG_CHARGE_END.
 */
void BlangongaAttack_Run(ObjAttach *o)
{
    Em *em = &gEms[o->idx];

    switch (o->type) {
    case 0xF:
    case 0x1D:
    case 0x23:
        o->hitFrames = 99;
        break;
    case 0x1F:
        if (em->motionId[0] == EM_MOTION_ID(0, BLANG_MOT_CONGALALA_COMBO)) {
            if (EmMotion_ReachedFrame(em, 0, 176.0f)) {
                if (EmBlangonga_IsSmallKind(em) == 1) {
                    o->hitPhase = 0;
                } else {
                    o->hitListNo = 23;
                }
            } else if (EmMotion_ReachedFrame(em, 0, 148.0f)) {
                o->hitListNo = 2;
            } else if (EmMotion_ReachedFrame(em, 0, 98.0f)) {
                o->hitListNo = 3;
            } else if (EmMotion_ReachedFrame(em, 0, 52.0f)) {
                o->hitListNo = 2;
            } else {
                o->hitListNo = 3;
            }
        }
        o->hitList = BlangongaAttack_HitShapes[o->hitListNo];
        break;
    case 0x21:
        if (em->motionId[0] == o->ownerMotion) {
            o->hitFrames = 20;
        } else if (em->motionId[0] != EM_MOTION_ID(0, BLANG_MOT_HOP_LAND) || o->hitPhase == 0) {
            o->hitPhase = 0;
            o->state = ATTACH_END;
        }
        return;
    case 0x24:
        o->hitFrames = 99;
        if (em->motionId[0] == EM_MOTION_ID(0, BLANG_MOT_STRIKE_A)) {
            if (!EmMotion_ReachedFrame(em, 0, 34.0f) || EmMotion_InFrameRange(em, 0, 54.0f, 86.0f)) {
                o->hitListNo = 2;
            } else {
                o->hitListNo = 3;
            }
        }
        o->hitList = BlangongaAttack_HitShapes[o->hitListNo];
        break;
    case 0x27:
        if (em->motionId[0] == o->ownerMotion) {
            o->hitFrames = 26;
        } else if (em->motionId[0] != EM_MOTION_ID(0, BLANG_MOT_RAJANG_CHARGE_END) || o->hitPhase == 0) {
            o->hitPhase = 0;
            o->state = ATTACH_END;
        }
        return;
    }
    if (em->motionId[0] != o->ownerMotion || o->hitPhase == 0) {
        o->hitPhase = 0;
        o->state = ATTACH_END;
    }
    if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
    }
}

/* Hides the attack, frees its draw slot and frees it on the next update. */
void BlangongaAttack_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = ATTACH_FREE;
    o->visible = 0;
    if (o->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) o->drawSlot);
    }
}

/* Returns the object to the pool. */
void BlangongaAttack_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

/* The boomerang's ObjAttach buffer. */
typedef struct {
    f32 releaseHeight;  /* 0x00: the hand's height above the feet at the throw */
    f32 localVel[3];    /* 0x04: the velocity in the throw's frame */
    u16 throwYaw;       /* 0x10: the thrower's rotY at the throw */
    u16 returnYaw;      /* 0x12: the heading back to the thrower, set by the hit */
    f32 speed;          /* 0x14 */
    u8 homingFrames;    /* 0x18: the frames of homing turn on the way out */
    u8 wasHit;          /* 0x19: set by Boomerang_Hit */
    u8 pad1A[2];
    f32 hitDist;        /* 0x1C: the distance to the thrower at the hit */
    s16 returnFrame;    /* 0x20: counts the frames of the return */
    s16 returnFrames;   /* 0x22: the return's length, hitDist / 30 */
    f32 returnVelY;     /* 0x24: the return arc's vertical speed */
    u8 spinSe;          /* 0x28: 1 when rotY wraps (plays the spin sound) */
    s8 timer;           /* 0x29: frames to the drop, then of the drop's bend */
    s8 spinDownFrames;  /* 0x2A: a grounded boomerang's spin-down */
    u8 returns;         /* 0x2B: 1, it comes back; 0, it drops */
    u8 grounded;        /* 0x2C: 1 once a dropping boomerang is on the floor */
} BoomerangBuf;

/*
 * Makes the boomerang thrown by thrower at its bone 0x12, if thrower is in
 * the current area. thrower is a player: Player_StateThrowItem casts it. kind
 * (0 standing, 1 walking, 2 running) sets the throw's height. returns is
 * nonzero when the item is not used up, and then it comes back instead of
 * dropping. It flies in the frame of the thrower's yaw (throwYaw).
 */
void Boomerang_Create(Em *thrower, u8 kind, u8 returns)
{
    f32 pos[3];
    ObjAttach *o;
    BoomerangBuf *boomBuf;

    if (Obj_InCurrentArea(thrower)) {
        if ((o = ObjAttach_Alloc(1)) != NULL) {
            o->kind = ATTACH_BOOMERANG;
            o->type = kind;
            o->update = Boomerang_Update;
            o->idx = thrower->index;
            o->isEm = thrower->isEm;
            o->ownerMotion = 0;
            o->owner = thrower;
            o->area = thrower->area;
            boomBuf = (BoomerangBuf *) o->buf;
            o->rotY = 0;
            o->rotZ = 0xF000;
            Em_GetBonePos(o->owner, 0x12, pos);
            o->pos[0] = pos[0];
            o->pos[1] = pos[1];
            o->pos[2] = pos[2];
            o->velX = 0.0f;
            o->velY = 0.0f;
            o->velZ = 0.0f;
            o->side = *(u8 *) &thrower->side;
            boomBuf->throwYaw = thrower->rotY;
            boomBuf->releaseHeight = pos[1] - ((Em *) o->owner)->posY;
            boomBuf->homingFrames = 22;
            boomBuf->localVel[0] = 0.0f;
            switch (o->type) {
            case BOOMERANG_FLAT:
                boomBuf->localVel[1] = 0.0f;
                break;
            case BOOMERANG_RISING:
                boomBuf->localVel[1] = -12.0f;
                break;
            case BOOMERANG_SINKING:
                boomBuf->localVel[1] = 20.0f;
                break;
            }
            boomBuf->localVel[2] = 50.0f;
            boomBuf->wasHit = 0;
            boomBuf->returnFrame = 0;
            boomBuf->spinSe = 0;
            boomBuf->returns = returns;
            boomBuf->grounded = 0;
            boomBuf->spinDownFrames = 15;
        }
    }
}

/*
 * The boomerang's per-frame update, by ObjAttach.state: ATTACH_HIT goes to
 * Boomerang_Hit. Its End, Free and Draw follow KushalaBreath's family.
 */
void Boomerang_Update(ObjAttach *o)
{
    switch (o->state) {
    case ATTACH_SETUP:
        Boomerang_Setup(o);
        break;
    case ATTACH_RUN:
        Boomerang_Run(o);
        break;
    case ATTACH_END:
        Boomerang_End(o);
        break;
    case ATTACH_HIT:
        Boomerang_Hit(o);
        break;
    case 4:
        Boomerang_End(o);
        break;
    case ATTACH_DEFLECTED:
        Boomerang_End(o);
        break;
    case ATTACH_DEFLECTED_AURA:
        Boomerang_End(o);
        break;
    case 7:
        Boomerang_End(o);
        break;
    case ATTACH_FREE:
        Boomerang_Free(o);
        
    default:
        break;
    }
}

/*
 * Sets the boomerang up. One that won't return drops after 13 frames (10 for
 * type 1). It takes row 1 of Boomerang_Rows and Boomerang_HitShapes, and a
 * draw slot (it frees itself when none is free), then runs its first frame.
 */
void Boomerang_Setup(ObjAttach *o)
{
    BoomerangBuf *buf = (BoomerangBuf *) o->buf;

    o->state++;
    o->step = 0;
    o->stateVar0 = 0;
    o->visible = 1;
    o->draw = NULL;
    o->ownerMotion = 0;
    if (buf->returns <= 0) {
        if (o->type == BOOMERANG_RISING) {
            buf->timer = 10;
        } else {
            buf->timer = 13;
        }
    } else {
        buf->timer = 0;
    }
    o->hitPhase = 0;
    ObjAttach_StartOwnedAttack(o, (s32) o->owner, 1, &Boomerang_RowsPtr);
    o->hitList = Boomerang_HitShapes;
    o->hurtList = 0;
    o->drawSlot = DrawSlot_Alloc();
    if (o->drawSlot != -1) {
        o->drawSlotBuf = DrawSlot_Get(o->drawSlot);
        o->drawSlotBuf->owner = o;
        o->drawSlotBuf->draw = Boomerang_Draw;
        Boomerang_Run(o);
    } else {
        ObjAttach_Free(o);
    }
}

/*
 * On a hit: a dropping boomerang that won't return ends. Any other turns
 * back: it restarts its row with no delay, sets wasHit and plans a half
 * circle back to the thrower (hitDist, returnYaw, returnFrames = hitDist /
 * 30, returnVelY), then runs.
 */
void Boomerang_Hit(ObjAttach *o)
{
    BoomerangBuf *boomBuf = (BoomerangBuf *) o->buf;

    if (boomBuf->returns == 0 && o->stateVar0 == 3) {
        Boomerang_End(o);
        return;
    }
    o->state = ATTACH_RUN;
    ObjAttach_StartOwnedAttack(o, (s32) o->owner, 1, &Boomerang_RowsPtr);
    o->hitDelay = 0;
    boomBuf->wasHit = 1;
    boomBuf->hitDist = Vec_Distance(o->pos, &((Em *) o->owner)->posX);
    boomBuf->returnYaw = Math_Heading(o->pos[0], o->pos[2], ((Em *) o->owner)->posX, ((Em *) o->owner)->posZ) + 0x4000;
    boomBuf->returnFrames = boomBuf->hitDist / 30.0f;
    boomBuf->returnVelY = Math_Absf(o->pos[1] - (((Em *) o->owner)->posY + boomBuf->releaseHeight - 30.0f)) / boomBuf->returnFrames;
    Boomerang_Run(o);
}

f32 Math_Cos(f32 x);
f32 Vec_Length(f32 *v);
void Vec_Sub(f32 *out, f32 *a, f32 *b);
void Vec_Normalize(f32 *v);


typedef struct {
    f32 x, y, z;
} BoomerangVec;

/*
 * Moves the boomerang by stateVar0. 0 is the outward curve: localVel turned
 * by throwYaw. It turns back (1) when localVel[2] reaches 0, or drops (3)
 * after timer frames if it won't return. 1 homes back to the thrower's hand
 * height, 2 arcs back after a hit (no more hits), and 3 is the drop: the
 * curve bends down and grows or fades by type, and type 1 slides on the
 * ground for 15 frames. It ends within 80 of the thrower, on the ground or
 * after 180 frames; it spins and plays SE 0x88 once per turn.
 */
void Boomerang_Run(ObjAttach *o)
{
    f32 dir[3];
    f32 tgt[3];
    f32 vel[3];
    BoomerangBuf *boomBuf;
    Em *em;
    f32 curve;
    f32 dy;
    f32 scale;
    f32 targetY;
    f32 len;
    f32 headingAngle;
    u16 pitchStep;
    u16 yawStep;
    u16 heading;
    u16 prevRotY;

    boomBuf = (BoomerangBuf *) o->buf;
    *(BoomerangVec *) o->pos2 = *(BoomerangVec *) o->pos;
    if (boomBuf->wasHit == 1) {
        o->stateVar0 = 2;
        o->hitFrames = 0;
    } else {
        o->hitFrames = 99;
    }
    switch (o->stateVar0) {
    case 0:
        if (boomBuf->localVel[2] <= 0.0f) {
            boomBuf->speed = Vec_Length(&o->velX);
            o->homingTurn = 0.0f;
            o->homingSpeed = 0.0f;
            o->stateVar0 = 1;
            o->velX = 1.0f;
            o->velY = 0.0f;
            o->velZ = 0.0f;
            Vec_RotateY(&o->velX, (6.2831855f * boomBuf->throwYaw) / 65536.0f);
            o->pos[0] += o->velX;
            o->pos[1] += o->velY;
            o->pos[2] += o->velZ;
            o->rotY += 0x3000;
            if (o->drawSlotBuf != NULL) {
                Vec_Copy(o->drawSlotBuf->pos, o->pos);
                OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20, 0);
            }
            return;
        }
        if (boomBuf->returns == 0) {
            if (--boomBuf->timer < 0) {
                o->stateVar0 = 3;
                switch (o->type) {
                case BOOMERANG_FLAT:
                    boomBuf->timer = 6;
                    break;
                case BOOMERANG_RISING:
                    boomBuf->timer = 6;
                    break;
                case BOOMERANG_SINKING:
                    boomBuf->timer = 18;
                    break;
                }
            }
        }
        switch (o->type) {
        case BOOMERANG_FLAT:
            curve = 3.3333333f;
            boomBuf->localVel[1] = 0.0f;
            break;
        case BOOMERANG_RISING:
            boomBuf->localVel[1] += 0.48f;
            curve = 5.0f;
            break;
        case BOOMERANG_SINKING:
            boomBuf->localVel[1] += -0.8f;
            curve = 10.0f;
            break;
        }
        boomBuf->localVel[0] = -1.0f * curve * Math_Cos((6.2831855f * (180.0f * ((50.0f - boomBuf->localVel[2]) / 50.0f))) / 360.0f);
        boomBuf->localVel[2] += -2.0f;
        Vec_Copy(&o->velX, boomBuf->localVel);
        Vec_RotateY(&o->velX, (6.2831855f * boomBuf->throwYaw) / 65536.0f);
        break;
    case 1:
        if (o->ownerMotion++ > 180.0f) {
            o->state = ATTACH_END;
            break;
        }
        em = (Em *) o->owner;
        targetY = em->posY + boomBuf->releaseHeight - 30.0f;
        *(BoomerangVec *) tgt = *(BoomerangVec *) &em->posX;
        tgt[1] = targetY;
        Vec_Sub(dir, tgt, o->pos);
        len = Vec_Length(dir);
        dir[1] = 0.0f;
        Vec_Normalize(dir);
        if (len > 80.0f) {
        if (boomBuf->homingFrames-- <= 0) {
            boomBuf->homingFrames = 0;
            Vec_Copy(vel, &o->velX);
            o->velY = 0.0f;
            len = Vec_Length(&o->velX);
            o->velX = dir[0] * len;
            o->velZ = dir[2] * len;
            o->velY = vel[1];
        } else {
            switch (o->type) {
            case BOOMERANG_FLAT:
                curve = 2.0f;
                break;
            case BOOMERANG_RISING:
                curve = 1.5f;
                break;
            case BOOMERANG_SINKING:
                curve = 1.0f;
                break;
            }
            dir[0] *= (o->homingTurn += 0.15f * curve);
            dir[2] *= (o->homingTurn += 0.15f * curve);
            len = Vec_Length(&o->velX);
            o->velX /= len;
            o->velZ /= len;
            o->velX *= boomBuf->speed + o->homingSpeed;
            o->velZ *= boomBuf->speed + o->homingSpeed;
            o->homingSpeed += 1.5f * curve;
            o->velX += dir[0];
            o->velZ += dir[2];
        }
            dy = o->pos[1] - tgt[1];
            if (dy >= 0.0f) {
                if (o->type == BOOMERANG_FLAT) {
                    o->velY += -0.16f;
                } else {
                    o->velY += -0.8f;
                }
            } else {
                o->velY += 0.48f;
            }
        } else {
            o->state = ATTACH_END;
        }
        break;
    case 2:
        if (o->ownerMotion++ > 180.0f) {
            o->state = ATTACH_END;
            break;
        }
        em = (Em *) o->owner;
        targetY = em->posY + boomBuf->releaseHeight - 30.0f;
        *(BoomerangVec *) tgt = *(BoomerangVec *) &em->posX;
        tgt[1] = targetY;
        Vec_Sub(dir, tgt, o->pos);
        len = Vec_Length(dir);
        dir[1] = 0.0f;
        Vec_Normalize(dir);
        if (len > 80.0f) {
        if (boomBuf->returnFrame++ >= boomBuf->returnFrames) {
            boomBuf->homingFrames = 0;
            len = 0.15f + Vec_Length(&o->velX);
            o->velX = dir[0] * len;
            o->velZ = dir[2] * len;
        } else {
            o->velX = -0.06f * boomBuf->hitDist * Math_Cos((6.2831855f * (180.0f * ((f32) boomBuf->returnFrame / (f32) boomBuf->returnFrames))) / 360.0f);
            o->velZ = -30.0f;
            Vec_RotateY(&o->velX, (6.2831855f * boomBuf->returnYaw) / 65536.0f);
        }
            dy = o->pos[1] - tgt[1];
            if (dy >= 0.0f) {
                o->velY = -1.0f * boomBuf->returnVelY;
            } else {
                o->velY = boomBuf->returnVelY;
            }
        } else {
            o->state = ATTACH_END;
        }
        break;
    case 3:
        if (o->ownerMotion++ > 180.0f) {
            o->state = ATTACH_END;
            return;
        }
        switch (o->type) {
        case BOOMERANG_FLAT:
            pitchStep = 0x250;
            scale = 1.04f;
            yawStep = 0x200;
            break;
        case BOOMERANG_RISING:
            pitchStep = 0x5FF;
            if (o->pos[1] <= 10.0f + AreaGrid2_GetFloorYFiltered(o->pos)) {
                scale = 0.98f;
            } else {
                scale = 1.02f;
            }
            yawStep = 0;
            break;
        case BOOMERANG_SINKING:
            pitchStep = 0x150;
            scale = 1.025f;
            yawStep = 0x150;
            break;
        }
        if (--boomBuf->timer >= 0) {
            heading = Math_Heading(boomBuf->localVel[0], boomBuf->localVel[2], 0.0f, 0.0f);
            headingAngle = (6.2831855f * heading) / 65536.0f;
            Vec_RotateY(boomBuf->localVel, headingAngle);
            if (o->type == BOOMERANG_RISING) {
                Vec_RotateX(boomBuf->localVel, -((6.2831855f * pitchStep) / 65536.0f));
                o->rotZ += 0x200;
            } else {
                Vec_RotateX(boomBuf->localVel, (6.2831855f * pitchStep) / 65536.0f);
            }
            Vec_RotateY(boomBuf->localVel, (6.2831855f * yawStep) / 65536.0f);
            Vec_RotateY(boomBuf->localVel, -1.0f * headingAngle);
        } else {
            boomBuf->timer = 0;
        }
        if (o->pos[1] <= 10.0f + AreaGrid2_GetFloorYFiltered(o->pos)) {
            if (o->type == BOOMERANG_RISING) {
                boomBuf->localVel[1] = 0.0f;
                boomBuf->grounded = 1;
            } else {
                o->state = ATTACH_END;
                return;
            }
        }
        boomBuf->localVel[0] *= scale;
        boomBuf->localVel[1] *= scale;
        boomBuf->localVel[2] *= scale;
        Vec_Copy(&o->velX, boomBuf->localVel);
        Vec_RotateY(&o->velX, (6.2831855f * boomBuf->throwYaw) / 65536.0f);
        break;
    }
    if (o->stateVar0 == 3 && o->type == BOOMERANG_RISING && boomBuf->grounded == 1) {
        if (--boomBuf->spinDownFrames <= 0) {
            boomBuf->spinDownFrames = 0;
            o->state = ATTACH_END;
            return;
        }
    }
    o->pos[0] += o->velX;
    if (Math_Absf(dy) < Math_Absf(o->velY) && o->stateVar0 != 0 && o->stateVar0 != 3) {
        o->pos[1] = tgt[1];
    } else {
        o->pos[1] += o->velY;
    }
    o->pos[2] += o->velZ;
    prevRotY = o->rotY;
    if (o->type == BOOMERANG_RISING && boomBuf->grounded == 1) {
        if (boomBuf->spinDownFrames > 0) {
            o->rotY += (u16) (s32) (0.5f + (65536.0f * (1.1780972f * (boomBuf->spinDownFrames / 15))) / 6.2831855f);
        }
    } else {
        o->rotY += 0x3000;
    }
    if ((u16) o->rotY < (u16) prevRotY) {
        boomBuf->spinSe = 1;
    }
    if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20, 0);
    }
    if (boomBuf->spinSe == 1) {
        Sound_PlaySeAtPosChecked(1, 0x88, 0, o->pos, 1, 0);
        boomBuf->spinSe = 0;
    }
}

/* The file's data. */
HitShape PlacedItem_Shapes_0[2] = {
#include "game/0089EC80.inc"
};
HitShape PlacedItem_Shapes_1[2] = {
#include "game/0089ECD0.inc"
};
HitShape PlacedItem_Shapes_2[2] = {
#include "game/0089ED20.inc"
};
HitShape PlacedItem_Shapes_4[2] = {
#include "game/0089ED70.inc"
};
HitShape PlacedItem_Shapes_5[2] = {
#include "game/0089EDC0.inc"
};
HitShape PlacedItem_Shapes_6[2] = {
#include "game/0089EE10.inc"
};
HitShape *PlacedItem_Shapes[8] = {
    PlacedItem_Shapes_0, PlacedItem_Shapes_1, PlacedItem_Shapes_2, PlacedItem_Shapes_2,
    PlacedItem_Shapes_4, PlacedItem_Shapes_5, PlacedItem_Shapes_6, 0,
};
u32 PlacedItem_Rows[30] = {
#include "game/0089EE80.inc"
};
void *PlacedItem_RowsPtr = (void *)PlacedItem_Rows;
s16 PlacedItem_Motions[8] = {
#include "game/0089EF00.inc"
};
u8 PlacedItem_BasePartScaleKeys[48] = {
#include "game/0089EF10.inc"
};
u8 PlacedItem_RaisedPartScaleKeys[48] = {
#include "game/0089EF40.inc"
};
u8 PlacedItem_SpinPartScaleKeys[48] = {
#include "game/0089EF70.inc"
};
u8 PlacedItem_BasePartColorKeys[32] = {
#include "game/0089EFA0.inc"
};
u8 PlacedItem_RaisedPartColorKeys[32] = {
#include "game/0089EFC0.inc"
};
s16 PlacedItem_PartModels[8] = {
#include "game/0089EFE0.inc"
};
HitShape VespoidAttack_HitShapes_0[4] = {
#include "game/0089EFF0.inc"
};
HitShape VespoidAttack_HitShapes_1[2] = {
#include "game/0089F090.inc"
};
HitShape VespoidAttack_HitShapes_2[3] = {
#include "game/0089F0E0.inc"
};
HitShape VespoidAttack_HitShapes_3[2] = {
#include "game/0089F160.inc"
};
HitShape VespoidAttack_HitShapes_4[2] = {
#include "game/0089F1B0.inc"
};
HitShape VespoidAttack_HitShapes_5[2] = {
#include "game/0089F200.inc"
};
HitShape VespoidAttack_HitShapes_6[2] = {
#include "game/0089F250.inc"
};
HitShape VespoidAttack_HitShapes_7[3] = {
#include "game/0089F2A0.inc"
};
HitShape VespoidAttack_HitShapes_8[2] = {
#include "game/0089F320.inc"
};
HitShape *VespoidAttack_HitShapes[12] = {
    VespoidAttack_HitShapes_0, VespoidAttack_HitShapes_1, VespoidAttack_HitShapes_2, VespoidAttack_HitShapes_3,
    VespoidAttack_HitShapes_4, VespoidAttack_HitShapes_5, VespoidAttack_HitShapes_6, VespoidAttack_HitShapes_7,
    VespoidAttack_HitShapes_8, 0, 0, 0,
};
u32 VespoidAttack_Rows[120] = {
#include "game/0089F3A0.inc"
};
void *VespoidAttack_RowsPtr = (void *)VespoidAttack_Rows;
HitShape D_0089F590[2] = {
#include "game/0089F590.inc"
};
HitShape *PitfallTrap_Val088 = D_0089F590;
u32 PitfallTrap_Rows[12] = {
#include "game/0089F5F0.inc"
};
void *PitfallTrap_RowsPtr = (void *)PitfallTrap_Rows;
HitShape VelocipreyAttack_HitShapes_0[2] = {
#include "game/0089F630.inc"
};
HitShape VelocipreyAttack_HitShapes_1[2] = {
#include "game/0089F680.inc"
};
HitShape VelocipreyAttack_HitShapes_2[2] = {
#include "game/0089F6D0.inc"
};
HitShape VelocipreyAttack_HitShapes_3[3] = {
#include "game/0089F720.inc"
};
HitShape VelocipreyAttack_HitShapes_4[2] = {
#include "game/0089F7A0.inc"
};
HitShape *VelocipreyAttack_HitShapes[8] = {
    VelocipreyAttack_HitShapes_0, VelocipreyAttack_HitShapes_1, VelocipreyAttack_HitShapes_2, VelocipreyAttack_HitShapes_3,
    VelocipreyAttack_HitShapes_4, 0, 0, 0,
};
u32 VelocipreyAttack_Rows[108] = {
#include "game/0089F810.inc"
};
void *VelocipreyAttack_RowsPtr = (void *)VelocipreyAttack_Rows;
s16 EmBomb_Models[6] = {
#include "game/0089F9C8.inc"
};
HitShape DromeAttack_HitShapes_0[2] = {
#include "game/0089F9E0.inc"
};
HitShape DromeAttack_HitShapes_1[2] = {
#include "game/0089FA30.inc"
};
HitShape DromeAttack_HitShapes_2[2] = {
#include "game/0089FA80.inc"
};
HitShape DromeAttack_HitShapes_3[3] = {
#include "game/0089FAD0.inc"
};
HitShape *DromeAttack_HitShapes[4] = {
    DromeAttack_HitShapes_0, DromeAttack_HitShapes_1, DromeAttack_HitShapes_2, DromeAttack_HitShapes_3,
};
u32 DromeAttack_Rows[84] = {
#include "game/0089FB60.inc"
};
void *DromeAttack_RowsPtr = (void *)DromeAttack_Rows;
HitShape KhezuAttack_HitShapes_0[2] = {
#include "game/0089FCC0.inc"
};
HitShape KhezuAttack_HitShapes_1[2] = {
#include "game/0089FD10.inc"
};
HitShape KhezuAttack_HitShapes_2[2] = {
#include "game/0089FD60.inc"
};
HitShape KhezuAttack_HitShapes_3[3] = {
#include "game/0089FDB0.inc"
};
HitShape KhezuAttack_HitShapes_4[4] = {
#include "game/0089FE30.inc"
};
HitShape KhezuAttack_HitShapes_5[4] = {
#include "game/0089FED0.inc"
};
HitShape KhezuAttack_HitShapes_6[2] = {
#include "game/0089FF70.inc"
};
HitShape KhezuAttack_HitShapes_7[2] = {
#include "game/0089FFC0.inc"
};
HitShape KhezuAttack_HitShapes_8[3] = {
#include "game/008A0010.inc"
};
HitShape KhezuAttack_HitShapes_9[3] = {
#include "game/008A0090.inc"
};
HitShape KhezuAttack_HitShapes_10[2] = {
#include "game/008A0110.inc"
};
HitShape KhezuAttack_HitShapes_11[2] = {
#include "game/008A0160.inc"
};
HitShape KhezuAttack_HitShapes_12[2] = {
#include "game/008A01B0.inc"
};
HitShape KhezuAttack_HitShapes_13[2] = {
#include "game/008A0200.inc"
};
HitShape KhezuAttack_HitShapes_14[4] = {
#include "game/008A0250.inc"
};
HitShape KhezuAttack_HitShapes_15[2] = {
#include "game/008A02F0.inc"
};
HitShape KhezuAttack_HitShapes_16[2] = {
#include "game/008A0340.inc"
};
HitShape KhezuAttack_HitShapes_17[2] = {
#include "game/008A0390.inc"
};
HitShape KhezuAttack_HitShapes_18[3] = {
#include "game/008A03E0.inc"
};
HitShape KhezuAttack_HitShapes_19[2] = {
#include "game/008A0460.inc"
};
HitShape KhezuAttack_HitShapes_20[2] = {
#include "game/008A04B0.inc"
};
HitShape *KhezuAttack_HitShapes[24] = {
    KhezuAttack_HitShapes_0, KhezuAttack_HitShapes_1, KhezuAttack_HitShapes_2, KhezuAttack_HitShapes_3,
    KhezuAttack_HitShapes_4, KhezuAttack_HitShapes_5, KhezuAttack_HitShapes_6, KhezuAttack_HitShapes_7,
    KhezuAttack_HitShapes_8, KhezuAttack_HitShapes_9, KhezuAttack_HitShapes_10, KhezuAttack_HitShapes_11,
    KhezuAttack_HitShapes_12, KhezuAttack_HitShapes_13, KhezuAttack_HitShapes_14, KhezuAttack_HitShapes_15,
    KhezuAttack_HitShapes_16, KhezuAttack_HitShapes_17, KhezuAttack_HitShapes_18, KhezuAttack_HitShapes_19,
    KhezuAttack_HitShapes_20, 0, 0, 0,
};
u32 KhezuAttack_Rows[246] = {
#include "game/008A0560.inc"
};
void *KhezuAttack_RowsPtr = (void *)KhezuAttack_Rows;
HitShape KutkuAttack_HitShapes_0[2] = {
#include "game/008A0940.inc"
};
HitShape KutkuAttack_HitShapes_1[3] = {
#include "game/008A0990.inc"
};
HitShape KutkuAttack_HitShapes_2[3] = {
#include "game/008A0A10.inc"
};
HitShape KutkuAttack_HitShapes_3[3] = {
#include "game/008A0A90.inc"
};
HitShape KutkuAttack_HitShapes_4[3] = {
#include "game/008A0B10.inc"
};
HitShape KutkuAttack_HitShapes_5[4] = {
#include "game/008A0B90.inc"
};
HitShape KutkuAttack_HitShapes_6[2] = {
#include "game/008A0C30.inc"
};
HitShape KutkuAttack_HitShapes_7[2] = {
#include "game/008A0C80.inc"
};
HitShape KutkuAttack_HitShapes_8[3] = {
#include "game/008A0CD0.inc"
};
HitShape KutkuAttack_HitShapes_9[3] = {
#include "game/008A0D50.inc"
};
HitShape KutkuAttack_HitShapes_10[2] = {
#include "game/008A0DD0.inc"
};
HitShape KutkuAttack_HitShapes_11[2] = {
#include "game/008A0E20.inc"
};
HitShape KutkuAttack_HitShapes_12[2] = {
#include "game/008A0E70.inc"
};
HitShape KutkuAttack_HitShapes_13[2] = {
#include "game/008A0EC0.inc"
};
HitShape KutkuAttack_HitShapes_14[3] = {
#include "game/008A0F10.inc"
};
HitShape KutkuAttack_HitShapes_15[2] = {
#include "game/008A0F90.inc"
};
HitShape KutkuAttack_HitShapes_16[2] = {
#include "game/008A0FE0.inc"
};
HitShape KutkuAttack_HitShapes_17[2] = {
#include "game/008A1030.inc"
};
HitShape KutkuAttack_HitShapes_18[4] = {
#include "game/008A1080.inc"
};
HitShape KutkuAttack_HitShapes_19[2] = {
#include "game/008A1120.inc"
};
HitShape KutkuAttack_HitShapes_20[6] = {
#include "game/008A1170.inc"
};
HitShape KutkuAttack_HitShapes_21[5] = {
#include "game/008A1260.inc"
};
HitShape *KutkuAttack_HitShapes[24] = {
    KutkuAttack_HitShapes_0, KutkuAttack_HitShapes_1, KutkuAttack_HitShapes_2, KutkuAttack_HitShapes_3,
    KutkuAttack_HitShapes_4, KutkuAttack_HitShapes_5, KutkuAttack_HitShapes_6, KutkuAttack_HitShapes_7,
    KutkuAttack_HitShapes_8, KutkuAttack_HitShapes_9, KutkuAttack_HitShapes_10, KutkuAttack_HitShapes_11,
    KutkuAttack_HitShapes_12, KutkuAttack_HitShapes_13, KutkuAttack_HitShapes_14, KutkuAttack_HitShapes_15,
    KutkuAttack_HitShapes_16, KutkuAttack_HitShapes_17, KutkuAttack_HitShapes_18, KutkuAttack_HitShapes_19,
    KutkuAttack_HitShapes_20, KutkuAttack_HitShapes_21, 0, 0,
};
u32 KutkuAttack_Rows[228] = {
#include "game/008A1390.inc"
};
void *KutkuAttack_RowsPtr = (void *)KutkuAttack_Rows;
HitShape DiablosAttack_HitShapes_0[2] = {
#include "game/008A1730.inc"
};
HitShape DiablosAttack_HitShapes_1[3] = {
#include "game/008A1780.inc"
};
HitShape DiablosAttack_HitShapes_2[3] = {
#include "game/008A1800.inc"
};
HitShape DiablosAttack_HitShapes_3[3] = {
#include "game/008A1880.inc"
};
HitShape DiablosAttack_HitShapes_4[4] = {
#include "game/008A1900.inc"
};
HitShape DiablosAttack_HitShapes_5[4] = {
#include "game/008A19A0.inc"
};
HitShape DiablosAttack_HitShapes_6[2] = {
#include "game/008A1A40.inc"
};
HitShape DiablosAttack_HitShapes_7[2] = {
#include "game/008A1A90.inc"
};
HitShape DiablosAttack_HitShapes_8[3] = {
#include "game/008A1AE0.inc"
};
HitShape DiablosAttack_HitShapes_9[3] = {
#include "game/008A1B60.inc"
};
HitShape DiablosAttack_HitShapes_10[2] = {
#include "game/008A1BE0.inc"
};
HitShape DiablosAttack_HitShapes_11[4] = {
#include "game/008A1C30.inc"
};
HitShape DiablosAttack_HitShapes_12[3] = {
#include "game/008A1CD0.inc"
};
HitShape DiablosAttack_HitShapes_13[4] = {
#include "game/008A1D50.inc"
};
HitShape DiablosAttack_HitShapes_14[3] = {
#include "game/008A1DF0.inc"
};
HitShape DiablosAttack_HitShapes_15[2] = {
#include "game/008A1E70.inc"
};
HitShape DiablosAttack_HitShapes_16[3] = {
#include "game/008A1EC0.inc"
};
HitShape DiablosAttack_HitShapes_17[3] = {
#include "game/008A1F40.inc"
};
HitShape DiablosAttack_HitShapes_18[4] = {
#include "game/008A1FC0.inc"
};
HitShape DiablosAttack_HitShapes_19[2] = {
#include "game/008A2060.inc"
};
HitShape *DiablosAttack_HitShapes[20] = {
    DiablosAttack_HitShapes_0, DiablosAttack_HitShapes_1, DiablosAttack_HitShapes_2, DiablosAttack_HitShapes_3,
    DiablosAttack_HitShapes_4, DiablosAttack_HitShapes_5, DiablosAttack_HitShapes_6, DiablosAttack_HitShapes_7,
    DiablosAttack_HitShapes_8, DiablosAttack_HitShapes_9, DiablosAttack_HitShapes_10, DiablosAttack_HitShapes_11,
    DiablosAttack_HitShapes_12, DiablosAttack_HitShapes_13, DiablosAttack_HitShapes_14, DiablosAttack_HitShapes_15,
    DiablosAttack_HitShapes_16, DiablosAttack_HitShapes_17, DiablosAttack_HitShapes_18, DiablosAttack_HitShapes_19,
};
u32 DiablosAttack_Rows[204] = {
#include "game/008A2100.inc"
};
void *DiablosAttack_RowsPtr = (void *)DiablosAttack_Rows;
HitShape GraviosAttack_HitShapes_0[2] = {
#include "game/008A2440.inc"
};
HitShape GraviosAttack_HitShapes_1[3] = {
#include "game/008A2490.inc"
};
HitShape GraviosAttack_HitShapes_2[3] = {
#include "game/008A2510.inc"
};
HitShape GraviosAttack_HitShapes_3[3] = {
#include "game/008A2590.inc"
};
HitShape GraviosAttack_HitShapes_4[4] = {
#include "game/008A2610.inc"
};
HitShape GraviosAttack_HitShapes_5[6] = {
#include "game/008A26B0.inc"
};
HitShape GraviosAttack_HitShapes_6[2] = {
#include "game/008A27A0.inc"
};
HitShape GraviosAttack_HitShapes_7[2] = {
#include "game/008A27F0.inc"
};
HitShape GraviosAttack_HitShapes_8[3] = {
#include "game/008A2840.inc"
};
HitShape GraviosAttack_HitShapes_9[3] = {
#include "game/008A28C0.inc"
};
HitShape GraviosAttack_HitShapes_10[2] = {
#include "game/008A2940.inc"
};
HitShape GraviosAttack_HitShapes_11[2] = {
#include "game/008A2990.inc"
};
HitShape GraviosAttack_HitShapes_12[2] = {
#include "game/008A29E0.inc"
};
HitShape GraviosAttack_HitShapes_13[2] = {
#include "game/008A2A30.inc"
};
HitShape GraviosAttack_HitShapes_14[4] = {
#include "game/008A2A80.inc"
};
HitShape GraviosAttack_HitShapes_15[2] = {
#include "game/008A2B20.inc"
};
HitShape GraviosAttack_HitShapes_16[3] = {
#include "game/008A2B70.inc"
};
HitShape GraviosAttack_HitShapes_17[2] = {
#include "game/008A2BF0.inc"
};
HitShape GraviosAttack_HitShapes_18[2] = {
#include "game/008A2C40.inc"
};
HitShape GraviosAttack_HitShapes_19[2] = {
#include "game/008A2C90.inc"
};
HitShape GraviosAttack_HitShapes_20[2] = {
#include "game/008A2CE0.inc"
};
HitShape GraviosAttack_HitShapes_21[2] = {
#include "game/008A2D30.inc"
};
HitShape GraviosAttack_HitShapes_22[2] = {
#include "game/008A2D80.inc"
};
HitShape *GraviosAttack_HitShapes[24] = {
    GraviosAttack_HitShapes_0, GraviosAttack_HitShapes_1, GraviosAttack_HitShapes_2, GraviosAttack_HitShapes_3,
    GraviosAttack_HitShapes_4, GraviosAttack_HitShapes_5, GraviosAttack_HitShapes_6, GraviosAttack_HitShapes_7,
    GraviosAttack_HitShapes_8, GraviosAttack_HitShapes_9, GraviosAttack_HitShapes_10, GraviosAttack_HitShapes_11,
    GraviosAttack_HitShapes_12, GraviosAttack_HitShapes_13, GraviosAttack_HitShapes_14, GraviosAttack_HitShapes_15,
    GraviosAttack_HitShapes_16, GraviosAttack_HitShapes_17, GraviosAttack_HitShapes_18, GraviosAttack_HitShapes_19,
    GraviosAttack_HitShapes_20, GraviosAttack_HitShapes_21, GraviosAttack_HitShapes_22, 0,
};
u32 GraviosAttack_Rows[198] = {
#include "game/008A2E30.inc"
};
void *GraviosAttack_RowsPtr = (void *)GraviosAttack_Rows;
HitShape PlesiothAttack_HitShapes_0[2] = {
#include "game/008A3150.inc"
};
HitShape PlesiothAttack_HitShapes_1[3] = {
#include "game/008A31A0.inc"
};
HitShape PlesiothAttack_HitShapes_2[3] = {
#include "game/008A3220.inc"
};
HitShape PlesiothAttack_HitShapes_3[2] = {
#include "game/008A32A0.inc"
};
HitShape PlesiothAttack_HitShapes_4[4] = {
#include "game/008A32F0.inc"
};
HitShape PlesiothAttack_HitShapes_5[4] = {
#include "game/008A3390.inc"
};
HitShape PlesiothAttack_HitShapes_6[3] = {
#include "game/008A3430.inc"
};
HitShape PlesiothAttack_HitShapes_7[3] = {
#include "game/008A34B0.inc"
};
HitShape PlesiothAttack_HitShapes_8[4] = {
#include "game/008A3530.inc"
};
HitShape PlesiothAttack_HitShapes_9[3] = {
#include "game/008A35D0.inc"
};
HitShape PlesiothAttack_HitShapes_10[2] = {
#include "game/008A3650.inc"
};
HitShape PlesiothAttack_HitShapes_11[2] = {
#include "game/008A36A0.inc"
};
HitShape *PlesiothAttack_HitShapes[12] = {
    PlesiothAttack_HitShapes_0, PlesiothAttack_HitShapes_1, PlesiothAttack_HitShapes_2, PlesiothAttack_HitShapes_3,
    PlesiothAttack_HitShapes_4, PlesiothAttack_HitShapes_5, PlesiothAttack_HitShapes_6, PlesiothAttack_HitShapes_7,
    PlesiothAttack_HitShapes_8, PlesiothAttack_HitShapes_9, PlesiothAttack_HitShapes_10, PlesiothAttack_HitShapes_11,
};
u32 PlesiothAttack_Rows[156] = {
#include "game/008A3720.inc"
};
void *PlesiothAttack_RowsPtr = (void *)PlesiothAttack_Rows;
HitShape CephadromeAttack_HitShapes_0[2] = {
#include "game/008A39A0.inc"
};
HitShape CephadromeAttack_HitShapes_1[3] = {
#include "game/008A39F0.inc"
};
HitShape CephadromeAttack_HitShapes_2[3] = {
#include "game/008A3A70.inc"
};
HitShape CephadromeAttack_HitShapes_3[2] = {
#include "game/008A3AF0.inc"
};
HitShape CephadromeAttack_HitShapes_4[4] = {
#include "game/008A3B40.inc"
};
HitShape CephadromeAttack_HitShapes_5[4] = {
#include "game/008A3BE0.inc"
};
HitShape CephadromeAttack_HitShapes_6[3] = {
#include "game/008A3C80.inc"
};
HitShape CephadromeAttack_HitShapes_7[3] = {
#include "game/008A3D00.inc"
};
HitShape CephadromeAttack_HitShapes_8[4] = {
#include "game/008A3D80.inc"
};
HitShape CephadromeAttack_HitShapes_9[3] = {
#include "game/008A3E20.inc"
};
HitShape CephadromeAttack_HitShapes_10[2] = {
#include "game/008A3EA0.inc"
};
HitShape CephadromeAttack_HitShapes_11[2] = {
#include "game/008A3EF0.inc"
};
HitShape CephadromeAttack_HitShapes_12[2] = {
#include "game/008A3F40.inc"
};
HitShape CephadromeAttack_HitShapes_13[2] = {
#include "game/008A3F90.inc"
};
HitShape CephadromeAttack_HitShapes_14[2] = {
#include "game/008A3FE0.inc"
};
HitShape *CephadromeAttack_HitShapes[16] = {
    CephadromeAttack_HitShapes_0, CephadromeAttack_HitShapes_1, CephadromeAttack_HitShapes_2, CephadromeAttack_HitShapes_3,
    CephadromeAttack_HitShapes_4, CephadromeAttack_HitShapes_5, CephadromeAttack_HitShapes_6, CephadromeAttack_HitShapes_7,
    CephadromeAttack_HitShapes_8, CephadromeAttack_HitShapes_9, CephadromeAttack_HitShapes_10, CephadromeAttack_HitShapes_11,
    CephadromeAttack_HitShapes_12, CephadromeAttack_HitShapes_13, CephadromeAttack_HitShapes_14, 0,
};
u32 CephadromeAttack_Rows[180] = {
#include "game/008A4070.inc"
};
void *CephadromeAttack_RowsPtr = (void *)CephadromeAttack_Rows;
HitShape StageShot_HitShapes_0[2] = {
#include "game/008A4350.inc"
};
HitShape StageShot_HitShapes_1[2] = {
#include "game/008A43A0.inc"
};
HitShape StageShot_HitShapes_2[2] = {
#include "game/008A43F0.inc"
};
HitShape StageShot_HitShapes_3[2] = {
#include "game/008A4440.inc"
};
HitShape StageShot_HitShapes_4[2] = {
#include "game/008A4490.inc"
};
HitShape StageShot_HitShapes_5[2] = {
#include "game/008A44E0.inc"
};
HitShape StageShot_HitShapes_6[2] = {
#include "game/008A4530.inc"
};
HitShape StageShot_HitShapes_7[2] = {
#include "game/008A4580.inc"
};
HitShape StageShot_HitShapes_8[2] = {
#include "game/008A45D0.inc"
};
HitShape StageShot_HitShapes_9[2] = {
#include "game/008A4620.inc"
};
HitShape StageShot_HitShapes_10[2] = {
#include "game/008A4670.inc"
};
HitShape StageShot_HitShapes_11[2] = {
#include "game/008A46C0.inc"
};
HitShape StageShot_HitShapes_12[2] = {
#include "game/008A4710.inc"
};
HitShape *StageShot_HitShapes[16] = {
    StageShot_HitShapes_0, StageShot_HitShapes_1, StageShot_HitShapes_2, StageShot_HitShapes_3,
    StageShot_HitShapes_4, StageShot_HitShapes_5, StageShot_HitShapes_6, StageShot_HitShapes_7,
    StageShot_HitShapes_8, StageShot_HitShapes_9, StageShot_HitShapes_10, StageShot_HitShapes_11,
    StageShot_HitShapes_12, 0, 0, 0,
};
u32 StageShot_Rows[60] = {
#include "game/008A47A0.inc"
};
void *StageShot_RowsPtr = (void *)StageShot_Rows;
f32 StageShot_StartVels[9][3] = {
#include "game/008A48A0.inc"
};
f32 StageShot_Gravities[12] = {
#include "game/008A4910.inc"
};
s16 StageShot_TimeLimit[16] = {
#include "game/008A4940.inc"
};
f32 StageShot_CannonPtsFortress5[2][3] = {
#include "game/008A4960.inc"
};
f32 StageShot_CannonPtsCastle1[2][3] = {
#include "game/008A4980.inc"
};
u16 StageShot_CannonAngsCastle1[4] = {
#include "game/008A4998.inc"
};
f32 StageShot_CannonPtsBattleTown1[2][3] = {
#include "game/008A49A0.inc"
};
u16 StageShot_CannonAngsBattleTown1[4] = {
#include "game/008A49B8.inc"
};
f32 StageShot_CannonSpread[4] = {
#include "game/008A49C0.inc"
};
f32 StageShot_RockSpotsFortress4[3][3] = {
#include "game/008A49D0.inc"
};
u16 StageShot_RockAngsFortress4[4] = {
#include "game/008A49F8.inc"
};
f32 StageShot_RockSpotsFortress3[2][3] = {
#include "game/008A4A00.inc"
};
u16 StageShot_RockAngsFortress3[4] = {
#include "game/008A4A18.inc"
};
f32 StageShot_RockSpotsFortress2[3][3] = {
#include "game/008A4A20.inc"
};
u16 StageShot_RockAngsFortress2[4] = {
#include "game/008A4A48.inc"
};
f32 StageShot_RockSpotVels[12] = {
#include "game/008A4A50.inc"
};
f32 EmBlangonga_FallPointsSnowy3[6][3] = {
#include "game/008A4A80.inc"
};
s16 EmBlangonga_FallListsSnowy3_0[4] = {
#include "game/008A4AC8.inc"
};
s16 EmBlangonga_FallListsSnowy3_1[8] = {
#include "game/008A4AD0.inc"
};
s16 *EmBlangonga_FallListsSnowy3[4] = {
    EmBlangonga_FallListsSnowy3_0, EmBlangonga_FallListsSnowy3_1, NULL, NULL,
};
f32 EmBlangonga_FallPointsSnowy8[6][3] = {
#include "game/008A4AF0.inc"
};
s16 EmBlangonga_FallListsByZ_0[6] = {
#include "game/008A4B38.inc"
};
s16 EmBlangonga_FallListsByZ_1[6] = {
#include "game/008A4B48.inc"
};
s16 *EmBlangonga_FallListsByZ[2] = {
    EmBlangonga_FallListsByZ_0, EmBlangonga_FallListsByZ_1,
};
HitShape CephalosAttack_HitShapes_0[2] = {
#include "game/008A4B60.inc"
};
HitShape CephalosAttack_HitShapes_1[3] = {
#include "game/008A4BB0.inc"
};
HitShape CephalosAttack_HitShapes_2[3] = {
#include "game/008A4C30.inc"
};
HitShape CephalosAttack_HitShapes_3[2] = {
#include "game/008A4CB0.inc"
};
HitShape CephalosAttack_HitShapes_4[4] = {
#include "game/008A4D00.inc"
};
HitShape CephalosAttack_HitShapes_5[4] = {
#include "game/008A4DA0.inc"
};
HitShape CephalosAttack_HitShapes_6[3] = {
#include "game/008A4E40.inc"
};
HitShape CephalosAttack_HitShapes_7[3] = {
#include "game/008A4EC0.inc"
};
HitShape CephalosAttack_HitShapes_8[4] = {
#include "game/008A4F40.inc"
};
HitShape CephalosAttack_HitShapes_9[3] = {
#include "game/008A4FE0.inc"
};
HitShape CephalosAttack_HitShapes_10[2] = {
#include "game/008A5060.inc"
};
HitShape CephalosAttack_HitShapes_11[2] = {
#include "game/008A50B0.inc"
};
HitShape CephalosAttack_HitShapes_12[2] = {
#include "game/008A5100.inc"
};
HitShape CephalosAttack_HitShapes_13[2] = {
#include "game/008A5150.inc"
};
HitShape CephalosAttack_HitShapes_14[2] = {
#include "game/008A51A0.inc"
};
HitShape *CephalosAttack_HitShapes[16] = {
    CephalosAttack_HitShapes_0, CephalosAttack_HitShapes_1, CephalosAttack_HitShapes_2, CephalosAttack_HitShapes_3,
    CephalosAttack_HitShapes_4, CephalosAttack_HitShapes_5, CephalosAttack_HitShapes_6, CephalosAttack_HitShapes_7,
    CephalosAttack_HitShapes_8, CephalosAttack_HitShapes_9, CephalosAttack_HitShapes_10, CephalosAttack_HitShapes_11,
    CephalosAttack_HitShapes_12, CephalosAttack_HitShapes_13, CephalosAttack_HitShapes_14, 0,
};
u32 CephalosAttack_Rows[180] = {
#include "game/008A5230.inc"
};
void *CephalosAttack_RowsPtr = (void *)CephalosAttack_Rows;
HitShape BowArrow_HitShapes_0[2] = {
#include "game/008A5510.inc"
};
HitShape *BowArrow_HitShapes[4] = {
    BowArrow_HitShapes_0, 0, 0, 0,
};
u32 BowArrow_Rows[78] = {
#include "game/008A5570.inc"
};
void *BowArrow_RowsPtr = (void *)BowArrow_Rows;
s16 BowArrow_ShotParams_0[4] = {
#include "game/008A56B0.inc"
};
s16 BowArrow_ShotParams_1[4] = {
#include "game/008A56B8.inc"
};
s16 BowArrow_ShotParams_2[4] = {
#include "game/008A56C0.inc"
};
s16 BowArrow_ShotParams_3[6] = {
#include "game/008A56C8.inc"
};
s16 BowArrow_ShotParams_4[4] = {
#include "game/008A56D8.inc"
};
s16 BowArrow_ShotParams_5[4] = {
#include "game/008A56E0.inc"
};
s16 BowArrow_ShotParams_6[6] = {
#include "game/008A56E8.inc"
};
s16 BowArrow_ShotParams_7[6] = {
#include "game/008A56F8.inc"
};
s16 BowArrow_ShotParams_8[4] = {
#include "game/008A5708.inc"
};
s16 BowArrow_ShotParams_9[4] = {
#include "game/008A5710.inc"
};
s16 BowArrow_ShotParams_10[4] = {
#include "game/008A5718.inc"
};
s16 BowArrow_ShotParams_11[8] = {
#include "game/008A5720.inc"
};
s16 *BowArrow_ShotParams[12] = {
    BowArrow_ShotParams_0, BowArrow_ShotParams_1, BowArrow_ShotParams_2, BowArrow_ShotParams_3,
    BowArrow_ShotParams_4, BowArrow_ShotParams_5, BowArrow_ShotParams_6, BowArrow_ShotParams_7,
    BowArrow_ShotParams_8, BowArrow_ShotParams_9, BowArrow_ShotParams_10, BowArrow_ShotParams_11,
};
u16 BowArrow_SpreadScale[8] = {
#include "game/008A5760.inc"
};
s16 BowArrow_SpreadDirs[8] = {
#include "game/008A5770.inc"
};
BowArrowKind BowArrow_Kinds[11] = {
#include "game/008A5780.inc"
};
s16 BowArrow_EventTbls_0[8] = {
#include "game/008A58E0.inc"
};
s16 BowArrow_EventTbls_1[8] = {
#include "game/008A58F0.inc"
};
s16 BowArrow_EventTbls_2[8] = {
#include "game/008A5900.inc"
};
s16 *BowArrow_EventTbls[4] = {
    BowArrow_EventTbls_0, BowArrow_EventTbls_1, BowArrow_EventTbls_2, NULL,
};
f32 BowArrow_ChargePowerScale[4] = {
#include "game/008A5920.inc"
};
f32 BowArrow_ChargeElementScale[4] = {
#include "game/008A5930.inc"
};
f32 BowArrow_ChargeCoatingPower[4] = {
#include "game/008A5940.inc"
};
f32 BowArrow_ChargeFlight[4][6] = {
#include "game/008A5950.inc"
};
u8 BowArrow_LevelScale[16] = {
#include "game/008A59B0.inc"
};
s16 BowArrow_EventHitFx[8] = {
#include "game/008A59C0.inc"
};
f32 BowArrow_EventPowerScale[6] = {
#include "game/008A59D0.inc"
};
s16 BowArrow_EventHitSe[6] = {
#include "game/008A59E8.inc"
};
HitShape GarugaAttack_HitShapes_0[2] = {
#include "game/008A5A00.inc"
};
HitShape GarugaAttack_HitShapes_1[3] = {
#include "game/008A5A50.inc"
};
HitShape GarugaAttack_HitShapes_2[3] = {
#include "game/008A5AD0.inc"
};
HitShape GarugaAttack_HitShapes_3[3] = {
#include "game/008A5B50.inc"
};
HitShape GarugaAttack_HitShapes_4[4] = {
#include "game/008A5BD0.inc"
};
HitShape GarugaAttack_HitShapes_5[4] = {
#include "game/008A5C70.inc"
};
HitShape GarugaAttack_HitShapes_6[2] = {
#include "game/008A5D10.inc"
};
HitShape GarugaAttack_HitShapes_7[2] = {
#include "game/008A5D60.inc"
};
HitShape GarugaAttack_HitShapes_8[3] = {
#include "game/008A5DB0.inc"
};
HitShape GarugaAttack_HitShapes_9[3] = {
#include "game/008A5E30.inc"
};
HitShape GarugaAttack_HitShapes_10[2] = {
#include "game/008A5EB0.inc"
};
HitShape GarugaAttack_HitShapes_11[2] = {
#include "game/008A5F00.inc"
};
HitShape GarugaAttack_HitShapes_12[2] = {
#include "game/008A5F50.inc"
};
HitShape GarugaAttack_HitShapes_13[2] = {
#include "game/008A5FA0.inc"
};
HitShape GarugaAttack_HitShapes_14[4] = {
#include "game/008A5FF0.inc"
};
HitShape GarugaAttack_HitShapes_15[2] = {
#include "game/008A6090.inc"
};
HitShape GarugaAttack_HitShapes_16[2] = {
#include "game/008A60E0.inc"
};
HitShape GarugaAttack_HitShapes_17[2] = {
#include "game/008A6130.inc"
};
HitShape GarugaAttack_HitShapes_18[4] = {
#include "game/008A6180.inc"
};
HitShape GarugaAttack_HitShapes_19[2] = {
#include "game/008A6220.inc"
};
HitShape GarugaAttack_HitShapes_20[6] = {
#include "game/008A6270.inc"
};
HitShape GarugaAttack_HitShapes_21[5] = {
#include "game/008A6360.inc"
};
HitShape GarugaAttack_HitShapes_22[4] = {
#include "game/008A6430.inc"
};
HitShape *GarugaAttack_HitShapes[24] = {
    GarugaAttack_HitShapes_0, GarugaAttack_HitShapes_1, GarugaAttack_HitShapes_2, GarugaAttack_HitShapes_3,
    GarugaAttack_HitShapes_4, GarugaAttack_HitShapes_5, GarugaAttack_HitShapes_6, GarugaAttack_HitShapes_7,
    GarugaAttack_HitShapes_8, GarugaAttack_HitShapes_9, GarugaAttack_HitShapes_10, GarugaAttack_HitShapes_11,
    GarugaAttack_HitShapes_12, GarugaAttack_HitShapes_13, GarugaAttack_HitShapes_14, GarugaAttack_HitShapes_15,
    GarugaAttack_HitShapes_16, GarugaAttack_HitShapes_17, GarugaAttack_HitShapes_18, GarugaAttack_HitShapes_19,
    GarugaAttack_HitShapes_20, GarugaAttack_HitShapes_21, GarugaAttack_HitShapes_22, 0,
};
u32 GarugaAttack_Rows[240] = {
#include "game/008A6530.inc"
};
void *GarugaAttack_RowsPtr = (void *)GarugaAttack_Rows;
HitShape HermitaurAttack_HitShapes_0[4] = {
#include "game/008A6900.inc"
};
HitShape HermitaurAttack_HitShapes_1[4] = {
#include "game/008A69A0.inc"
};
HitShape HermitaurAttack_HitShapes_2[7] = {
#include "game/008A6A40.inc"
};
HitShape HermitaurAttack_HitShapes_3[4] = {
#include "game/008A6B60.inc"
};
HitShape HermitaurAttack_HitShapes_4[4] = {
#include "game/008A6C00.inc"
};
HitShape HermitaurAttack_HitShapes_5[2] = {
#include "game/008A6CA0.inc"
};
HitShape HermitaurAttack_HitShapes_6[8] = {
#include "game/008A6CF0.inc"
};
HitShape HermitaurAttack_HitShapes_7[9] = {
#include "game/008A6E30.inc"
};
HitShape HermitaurAttack_HitShapes_8[2] = {
#include "game/008A6FA0.inc"
};
HitShape HermitaurAttack_HitShapes_9[5] = {
#include "game/008A6FF0.inc"
};
HitShape HermitaurAttack_HitShapes_10[5] = {
#include "game/008A70C0.inc"
};
HitShape HermitaurAttack_HitShapes_11[9] = {
#include "game/008A7190.inc"
};
HitShape HermitaurAttack_HitShapes_12[2] = {
#include "game/008A7300.inc"
};
HitShape *HermitaurAttack_HitShapes[16] = {
    HermitaurAttack_HitShapes_0, HermitaurAttack_HitShapes_1, HermitaurAttack_HitShapes_2, HermitaurAttack_HitShapes_3,
    HermitaurAttack_HitShapes_4, HermitaurAttack_HitShapes_5, HermitaurAttack_HitShapes_6, HermitaurAttack_HitShapes_7,
    HermitaurAttack_HitShapes_8, HermitaurAttack_HitShapes_9, HermitaurAttack_HitShapes_10, HermitaurAttack_HitShapes_11,
    HermitaurAttack_HitShapes_12, 0, 0, 0,
};
u32 HermitaurAttack_Rows[204] = {
#include "game/008A7390.inc"
};
void *HermitaurAttack_RowsPtr = (void *)HermitaurAttack_Rows;
HitShape BlangongaAttack_HitShapes_0[3] = {
#include "game/008A76D0.inc"
};
HitShape BlangongaAttack_HitShapes_1[4] = {
#include "game/008A7750.inc"
};
HitShape BlangongaAttack_HitShapes_2[2] = {
#include "game/008A77F0.inc"
};
HitShape BlangongaAttack_HitShapes_3[2] = {
#include "game/008A7840.inc"
};
HitShape BlangongaAttack_HitShapes_4[2] = {
#include "game/008A7890.inc"
};
HitShape BlangongaAttack_HitShapes_5[2] = {
#include "game/008A78E0.inc"
};
HitShape BlangongaAttack_HitShapes_6[3] = {
#include "game/008A7930.inc"
};
HitShape BlangongaAttack_HitShapes_7[2] = {
#include "game/008A79B0.inc"
};
HitShape BlangongaAttack_HitShapes_8[4] = {
#include "game/008A7A00.inc"
};
HitShape BlangongaAttack_HitShapes_9[4] = {
#include "game/008A7AA0.inc"
};
HitShape BlangongaAttack_HitShapes_10[3] = {
#include "game/008A7B40.inc"
};
HitShape BlangongaAttack_HitShapes_11[3] = {
#include "game/008A7BC0.inc"
};
HitShape BlangongaAttack_HitShapes_12[2] = {
#include "game/008A7C40.inc"
};
HitShape BlangongaAttack_HitShapes_13[2] = {
#include "game/008A7C90.inc"
};
HitShape BlangongaAttack_HitShapes_14[2] = {
#include "game/008A7CE0.inc"
};
HitShape BlangongaAttack_HitShapes_15[2] = {
#include "game/008A7D30.inc"
};
HitShape BlangongaAttack_HitShapes_16[3] = {
#include "game/008A7D80.inc"
};
HitShape BlangongaAttack_HitShapes_17[2] = {
#include "game/008A7E00.inc"
};
HitShape BlangongaAttack_HitShapes_18[4] = {
#include "game/008A7E50.inc"
};
HitShape BlangongaAttack_HitShapes_19[2] = {
#include "game/008A7EF0.inc"
};
HitShape BlangongaAttack_HitShapes_20[2] = {
#include "game/008A7F40.inc"
};
HitShape BlangongaAttack_HitShapes_21[4] = {
#include "game/008A7F90.inc"
};
HitShape BlangongaAttack_HitShapes_22[2] = {
#include "game/008A8030.inc"
};
HitShape BlangongaAttack_HitShapes_23[2] = {
#include "game/008A8080.inc"
};
HitShape BlangongaAttack_HitShapes_24[3] = {
#include "game/008A80D0.inc"
};
HitShape BlangongaAttack_HitShapes_25[4] = {
#include "game/008A8150.inc"
};
HitShape *BlangongaAttack_HitShapes[28] = {
    BlangongaAttack_HitShapes_0, BlangongaAttack_HitShapes_1, BlangongaAttack_HitShapes_2, BlangongaAttack_HitShapes_3,
    BlangongaAttack_HitShapes_4, BlangongaAttack_HitShapes_5, BlangongaAttack_HitShapes_6, BlangongaAttack_HitShapes_7,
    BlangongaAttack_HitShapes_8, BlangongaAttack_HitShapes_9, BlangongaAttack_HitShapes_10, BlangongaAttack_HitShapes_11,
    BlangongaAttack_HitShapes_12, BlangongaAttack_HitShapes_13, BlangongaAttack_HitShapes_14, BlangongaAttack_HitShapes_15,
    BlangongaAttack_HitShapes_16, BlangongaAttack_HitShapes_17, BlangongaAttack_HitShapes_18, BlangongaAttack_HitShapes_19,
    BlangongaAttack_HitShapes_20, BlangongaAttack_HitShapes_21, BlangongaAttack_HitShapes_22, BlangongaAttack_HitShapes_23,
    BlangongaAttack_HitShapes_24, BlangongaAttack_HitShapes_25, 0, 0,
};
u32 BlangongaAttack_Rows[252] = {
#include "game/008A8260.inc"
};
void *BlangongaAttack_RowsPtr = (void *)BlangongaAttack_Rows;
HitShape D_008A8660[2] = {
#include "game/008A8660.inc"
};
HitShape *Boomerang_HitShapes = D_008A8660;
u32 Boomerang_Rows[12] = {
#include "game/008A86C0.inc"
};
void *Boomerang_RowsPtr = (void *)Boomerang_Rows;
