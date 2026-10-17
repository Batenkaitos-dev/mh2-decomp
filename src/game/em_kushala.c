/*
 * Monster family: Kushala Daora, Lunastra and Teostra (EM_KIND_KUSHALA_DAORA,
 * EM_KIND_RUSTED_KUSHALA_DAORA, EM_KIND_LUNASTRA and EM_KIND_TEOSTRA). The
 * code from the family's setup function (EmKushala_Setup, called by the
 * per-kind dispatch Em_TypeSetup) to the next family's. The state functions
 * take the monster and the work area its state dispatcher passes on (EmWork);
 * Em.step is the step within the current state. See include/em.h.
 */
#include "common.h"
#include "scene.h"
#include "quest.h"
#include "em.h"
#include "player.h"
#include "stageobj.h"
#include "objattach.h"
#include "area.h"
/* prototypes of callees (round 13) */
void EmKushala_MotionSounds(void *arg0, EmWork *work);
void EmKushala_MotionSoundsTesca(Em *em, EmWork *work);
s32 EmKind38_MoveByVelocity(Em *em);


/* prototypes */
void EmKushala_ChangeState(Em *em, u16 group, u16 state, u8 mode);
StageObj *StageObj_K1C_CreateAtYaw(f32 *pos, u16 v, u8 k, f32 scale);
void StageObj_K0F_CreateOnEm(Em *em, u8 k, u8 c, f32 scale);
void EmKushala_RunGroup0(Em *em, EmWork *work);
void EmKushala_RunGroup1(Em *em, EmWork *work);
void EmKushala_State0_04(Em *em, EmWork *work, s32 variant);
void EmKushala_State0_0F(Em *em, EmWork *work);
void EmKushala_State0_1B(Em *em, EmWork *work);
void EmKushala_State1_03(Em *em, EmWork *work);
void EmKushala_State1_05(Em *em, EmWork *work, s32 variant);
void EmKushala_State1_15(Em *em, EmWork *work, s32 variant);
void EmKushala_State1_19(Em *em, EmWork *work);
void EmKushala_State3_0F(Em *em, EmWork *work, s32 variant);
void EmKushala_State3_18(Em *em, EmWork *work);
void EmKushala_RunGroup3(Em *em, EmWork *work);
void EmKushala_StartCurves(Em *em, u8 kind);
u8 EmKushala_UpdateCurves(Em *em);
s8 EmMine_CreateTimed0(Em *em, f32 *pos);
void EmShakalaka_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void Em_BeginStatus4(Em *em);
void Em_BeginStatus5(Em *em);
void Em_EndStatus2(Em *em);
void Em_EndStatus4(Em *em);
void Em_EndStatus5(Em *em);
void Em_PlaySeAtBoneOnFrame(Em *em, s32 frame, s32 se, s32 bone);
void Em_SetLayerMotion(Em *em, s32 motion, s32 blend, s32 frame, s32 layer);
void Em_SetMotion(Em *em, s32 motion, s32 a, s32 b);
void Em_SetMotionRow(Em *em, s32 row, s32 arg2, s32 arg3);
/* unit's own declaration: with the definition's void (Em *em, s32 group, s32 state, u8 mode) (game/unk_701810.c), EmKind38_ChangeState: 3 of 96 words differ, EmShakalaka_ChangeState: 3 of 82 words differ ... */
void Em_SetStateSync(Em *em, u16 group, u16 state, u8 mode);
u16 Math_AngleXZ(f32 *from, f32 *to);
f32 Math_Sin(f32 x);
void Player_ClearFlag(void *, u32 flag);
void Player_SetFlag(void *obj, u32 flag);
s8 QuestEm_Appear(Em *em);
void EmImpactAttack_CreateAt(Em *em, f32 *pos, s8 kind);
void EmBomb_Create(Em *, s8);
/* unit's own declaration: with the definition's void (Em *em, s32 bone, u8 kind, u8 c, u16 a, u16 b) (game/unk_6E0650.c), EmKushala_CreateBreathAtBone: 3 of 27 words differ */
void KushalaBreath_CreateAtBone(Em *, s32, u8, u8, s32, s32);
void KushalaAttack_Create(Em *, s32);
void EmEffectAttack_CreateChain(Em *, s32, s32, s32, s32);
void ShakalakaAttack_Create(Em *, s8);
void TeostraAttack_Create(Em *, s32);
ObjAttach *BulldromeAttack_Create(Em *em, s8 kind);
/* unit's own declaration: with the definition's void (Em *em, u8 kind) (game/unk_6EB590.c), EmKushala_State3_05: 1 of 79 words differ */
void TeostraBlastRing_Create(Em *, s32);
void Em_SelectSightRow(Em *em, u8 idx);
void Em_MoveRotated(Em *em, void *src);
void Em_MoveRotatedAccel(Em *em, void *ang);
void Em_SetStateRand(Em *em, s32 arg1, s32 arg2);
void Em_LoadReactionSet(Em *em, u8 k);
void Em_ComputeGoal2(Em *em, f32 *target);
void Em_GetPitchToGoal(Em *em, s32 *out, f32 a, f32 b);
void Em_ClearLowHpNeed(Em *em);
void Em_SpawnStatus4Effect(Em *em, s32 a);
void Em_SetHitReactMode(Em *em, u8 a);
/* unit's own declaration: with the definition's s32 (Em *em, s16 n, s16 max, s16 interval) (game/unk_708850.c), EmKushala_State0_09: 61 of 104 words differ */
s32 Em_RegenHp(Em *em, s32 n, s32 max, s32 interval);
void Em_PlaySeAtBoneVol(Em *em, s32 se, s32 bone, s32 vol);
void Em_PlaySeAtBoneVolOnFrame(Em *em, s32 frame, s32 se, s32 bone, s32 vol);
void Em_PlaySeOnFrame(Em *em, s32 frame, s32 se);
void Em_UpdateFlight(Em *em, s32 flags, EmFlyParam *p);
s32 EmList_FindKindNear(Em *em, u16 kind, s32 mode, f32 dist);
void Em_SetSpeedScaled(Em *em, u16 flags, f32 x, f32 y);
s32 Em_StepToGoal(Em *em, u16 step, u8 flags);
u8 Em_TurnToGoal(Em *em, u16 step, u8 flags);
void Em_StartTurnToTarget(Em *em, EmTurnParams *t);
s32 Em_TurnStep(Em *em, EmTurnParams *t);
void Em_RestartScript(Em *em);
void Em_TargetPlayerInRange(Em *);
s32 EmKushala_WindActive(Em *);
void EmKushala_Init(Em *);
void EmKushala_ToIdle(Em *);
void EmKushala_State2_01(Em *, EmWork *, s32);
void EmKushala_State2_08(Em *, EmWork *);
void EmKushala_State2_09(Em *, EmWork *);
void EmKushala_State2_0A(Em *, EmWork *);
void EmKushala_State2_0F(Em *, EmWork *, s32);
void EmKushala_StepForward(Em *, EmWork *);
void EmKushala_State6_00(Em *, EmWork *);
void EmKushala_State6_01(Em *, EmWork *);
void EmKushala_State6_02(Em *);
void EmKushala_State6_03(Em *, EmWork *);
void EmKushala_State6_04_Captured(Em *);
void EmKushala_State6_05(Em *, EmWork *);
void EmKushala_State6_06(Em *, EmWork *);
void EmKushala_State6_07(Em *);
void EmKushala_State5_00_Hunted(Em *, EmWork *);
void EmKushala_State5_02_HuntedFalling(Em *, EmWork *);
s32 EmKushala_GroundEffectStateInWind(Em *);
void EmKushala_MotionEffects(Em *, EmWork *);
void EmKushala_CreateAttack(Em *em, s32 arg1);
void EmKushala_SpawnEffect02AtBone27(Em *);
void EmKind38_UpdateLoopSe(Em *em, EmWork *work);
void EmKind38_Init(Em *);
void EmKind38_State2_00(Em *, s16);
void EmKind38_State2_01(Em *);
void EmKind38_State2_04(Em *);
void EmKind38_State2_05(Em *);
void EmKind38_State2_06(Em *, s32);
void EmKind38_State2_08(Em *);
void EmKind38_State2_0A(Em *);
void EmKind38_State3_00(Em *, u8);
void EmKind38_State3_02(Em *);
void EmKind38_State5_00_Hunted(Em *, u8);
void EmKind38_State5_02(Em *);
void EmKind38_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void EmShakalaka_Init(Em *);
void EmShakalaka_State0_02(Em *, s32);
void EmShakalaka_State0_08(Em *);
void EmShakalaka_State0_09(Em *);
void EmShakalaka_State1_03(Em *, s32);
void EmShakalaka_State1_06(Em *, s32);
void EmShakalaka_State1_08(Em *, s32);
void EmShakalaka_State1_0D(Em *, s8);
void EmShakalaka_State3_03(Em *, s32);
void EmShakalaka_State4_01(Em *);
void EmShakalaka_State5_00_Hunted(Em *);
void EmShakalaka_State5_01_Hunted(Em *);
void EmShakalaka_State5_02(Em *);
void EmShakalaka_MotionEffects(Em *, EmWork *);
void EmFxTri_Spawn(Em *em, s16 kind);
void StageObj_K13_Create(void *arg, s16 v, u8 k);
void EmRangedAttack_CreateAtBone2(Em *, s32, s32, s32, s32, s32, s32);
/* unit's own declaration: with the definition's void (Em *em, s16 k, s16 c, f32 scale) (game/unk_874B90.c), EmKushala_SpawnK14NearGround: 21 of 50 words differ */
void StageObj_K14_Create(Em *em, s32, s32, f32);
/* unit's own declaration: with the definition's void (Em *em, u8 k, s16 v, s16 t, f32 y, f32 scale) (game/unk_874B90.c), EmKushala_State4_0C: 66 of 98 words differ */
void StageObj_Stun_CreateEm(Em *, s32, s32, s32, f32, f32);
StageObj *StageObj_K1C_CreateOnEm(Em *em, u8 k, f32 scale);
/* unit's own declaration: with the definition's void (Em *em, u8 k, u16 w, s16 v, f32 scale) (game/unk_880D50.c), EmKushala_State3_0F: 1 of 316 words differ */
void StageObj_K1C_CreateOnEmB(Em *em, s8, s16, u16, f32);
void StageObj_K27_Create(Em *em);
s32 Sound_RandomSe(s32 id, s16 lo, s32 alt, s16 n);
s32 EmMotion_PassedFrameScaled(void *p, f32 frame, s32 layer);
s32 EmMotion_ReachedFrame(void *obj, s32 a, f32 frame);
s32 EmMotion_InFrameRange(void *em, s32 layer, f32 from, f32 to);
void Vec_RotateY(f32 *v, f32 angle);
f32 Vec_Distance(f32 *a, f32 *b);
void Vec_Copy(f32 *out, f32 *in);
f32 *Mtx_FromAnglesXYZ(s32 *angles, f32 *m);
void Vec_Add(f32 *, f32 *, f32 *);
f32 Vec_DistanceXZ(f32 *a, f32 *b);
s32 QuestEm_CanAppear(Em *em);
void Player_Integrate(void *);
void Player_ClearVelocity(void *);
void Player_ClearAccel2(void *);
/* the calls that mask the value again (u8 or & 0xFF) cast it to s32 first: MWCC keeps the game's andi that way */
u8 Player_InCurrentArea(void *obj);
void Effect01_Spawn(void *obj, s16 type);
void Effect0D_SpawnOnGround(f32 *pos, s32 a, f32 scale);
/* unit's own declaration: with the definition's void (void *obj, s16 bone, f32 *out) (sub_main/unk_3DDBA0.c), EmKushala_SpawnGroundEffect: 11 of 31 words differ, EmShakalaka_SpawnGroundEffect: 11 of 31 words differ ... */
void Em_BonePos(void *obj, s32 a, f32 *out);
extern EmFlyParam EmKushala_FlyParam;
extern EmFlyParam EmKushala_FlyParam2;
extern EmTurnParams EmKushala_TurnParams[1];
extern u16 EmKind38_Effect02Chance[];
void Vec_Set(f32 *out, f32 x, f32 y, f32 z);
extern f32 EmKind38_HeightRange[][2];
void Em_ClearPoison(Em *em);
void Em_SetCaptured(Em *em, s32 a);
void Em_BeginStatus1(Em *em);
void QuestEm_Captured(Em *em);
void Em_SpawnLyingPuffs(Em *em, s32 bone, f32 *pos, f32 scale);
EmBone *Em_Bone(void *obj, s16 bone);
void Mtx_GetTrans(f32 *out, f32 *m);
void Vec_RotateSelf(f32 *v, f32 *m);
void Effect02_SpawnAt(f32 *pos, s32 a, u16 b);
void StageObj_K10_CreateAtScaled(f32 *pos, u8 k, f32 scale);
f32 *Mtx_FromAnglesYXZ(s32 *angles, f32 *m);
void Vec_Rotate(f32 *out, f32 *in, f32 *mtx);
f32 Math_Absf(f32 x);
void Em_BeginStatus2(Em *em);
extern u16 EmKushala_IdleTbl[];
Em *QuestEm_Spawn(QuestEm *q);
f32 Math_Sqrtf(f32 x);
f32 Math_Atan2(f32 y, f32 x);
extern f32 EmKind38_SpawnOfs[];
extern f32 EmShakalaka_NearDist[];
u16 Rand_Next(s32 stream);
void QuestEm_Hunted(Em *em);
void EmMine_Drop(Em *em, f32 *pos);
void EmKind38_PlaySeAtBone2(Em *em, s32 arg1);
s32 Save_TestEventFlag(s32);
u16 Area_IdForCurrentTime(u16);
typedef struct { f32 x, y, z; } EmKVec;
extern EmKVec EmShakalaka_State3_03Vel[];
extern EmKVec D_008CA324[];
extern EmKVec D_008CA328[];
s32 Motion_ReachedFrame(void *, s32, f32);
void EmKind38_ClampVelocity(Em *em, f32 *lim);
extern f32 EmKind38_VelLimits[];
void EmKind38_KeepHeight(Em *em, u8 k);
extern EmKVec EmKind38_State2_08Accel[];
extern f32 D_008CA26C[];
void EmMine_Remove(Em *em);
s32 EmMine_Update(Em *em);
s8 EmMine_Create(Em *em, s32 idx);
void CamShake_ReqAIf(Em *obj, s32 i);
void Em_ApplyPartBreaks(Em *em);
void Em_FlagPartBreak(Em *em, s32 arg1);
void Em_InitParts(Em *em);
s32 Em_ComputeMaxHp(Em *em, s16 idx);
void EmKind38_InitHover(Em *em, EmWork *work);
extern f32 gStartPos[];
extern f32 D_43F0A4[];
extern f32 D_43F0A8[];
typedef struct { f32 x, y; } EmKPair;
extern EmKPair EmKind38_State3Pts[][2];
extern s32 EmKushala_State3_0FTimers[][2];
extern s32 D_008CA214[][2];
void *EmKind38_CreateAttack(Em *em);
void Vec_RotateX(f32 *v, f32 angle);
void Vec_Scale(f32 *out, f32 *in, f32 s);
void StageObj_K0E_CreateScaled(f32 *pos, s16 k, f32 scale);
void Explosion_CreateAt(f32 *pos, s32 kind, u16 area, u8 arg3);
void StageObj_K15_Create(Em *em, u8 k);
void Effect02_SpawnForOwner(void *owner, s32 ang, s32 type, s32 sub, f32 *pos, f32 scale);
void Effect0D_SpawnAt(f32 *pos, s32 a, f32 scale);
void Mtx_Copy(f32 *dst, f32 *src);
void Quest_TriggerEvent(Em *em);
extern f32 gAreaGroundY[];
u16 Area_DayId(u16 id);
void Hud_TriggerGlow(Em *em);
void Em_EnterNextArea(Em *);
void Em_ComputeGoal(Em *em, f32 *target);
s32 Area_ContainsPos(s32, f32 *);
void Em_AreaExitPoint(Em *em, f32 *out);
AreaRecord *Stage_GetAreaRecord(u16 x);
struct EmIdEntry;
f32 *Em_AreaEntryValue(Em *em, struct EmIdEntry *entry);
void GmEm_ResetSize(Em *em);
void EmShakalaka_RunState(Em *em);
u8 Em_UpdateDamage(Em *em, u8 *out);
void Em_SetStatus2Timer(Em *em);
void Em_SetStatus4Timer(Em *em);
void Em_SetStatus5Timer(Em *em);
void Em_ResetPartHp(Em *em, u32 i);
void Em_UpdatePendingScript(Em *em);
void Em_ClearHitReactMode(Em *em);
void StageObj_K11_CreateAtScaled(f32 *pos, s32 v, s16 k, f32 scale);
void Em_SetPartScaled(Em *em, s32 bit);
void QuestEm_LoadPersistentHp(Em *em);
u8 Quest_FindSpecialMonster(u8 id);
s32 Em_PartBroken(Em *em, s32 arg1);
void StageObj_K09_Create(Em *);
s32 Quest_GetTime(s32 which);
s32 Em_UpdateCombatTimer(Em *em);
s32 func_0083B8F0(Em *em);
void EmKind38_RunState(Em *em);
void EmKind38_SteerHover(Em *em, EmWork *work);
void EmKind38_ApplyHover(Em *em, EmWork *work);
void Em_EndStatus5(Em *em);
s32 EmKushala_WindActive2(Em *);
void EmKushala_BreakParts(Em *em, s32 apply);
void EmKushala_RunState(Em *em, s32 arg1);
void Em_RaisePlayerAreaTriggerLax(Em *em);
u16 Map_DayId(u16);
void Em_CheckLowHp(Em *em, f32 ratio);
void Em_DecayNeed2(Em *em);
void Em_DecayNeed1(Em *em);
void Em_DecayNeed3(Em *em);
void Em_SetStatus1Timer(Em *em);
void Em_CheckRageByHp(Em *em, s16 n);
s32 Em_DamageResultIsReaction(Em *em, s32 arg1);
void CamReq_ForMonster(Em *em);
s32 Quest_IsRunning(s32 ending);
/* end of prototypes */

void EmKushala_Setup(Em *em)
{
}

void EmKushala_SpawnEffects(Em *em)
{
    StageObj_K13_Create(em, 0x22, 0);
    Effect01_Spawn((Player *) em, 0);
    if (em->type == 0x36 || em->type == 0x3C) {
        StageObj_K1C_CreateOnEm(em, 0xC, em->scaleX);
    } else if (em->type == 0x40 || em->type == 0x41) {
        StageObj_K1C_CreateOnEm(em, 0x1D, em->scaleX);
    }
}

void EmKushala_Init(Em *em)
{
    EmKushalaWork *w = (EmKushalaWork *)&em->work;

    Em_SetPartScaled(em, 3);
    Em_SetPartScaled(em, 4);
    if (gQuest.stage == 0) {
        em->rotY = 0x8000;
        switch (gScene.area) {
        case 0:
            em->posX = 8000.0f + 1500.0f * (u32) em->slotNo;
            em->posY = 0.0f;
            em->posZ = 6000.0f;
            break;
        case AREA_SNOWY_MOUNTAINS_NIGHT_9:
            em->posX = 7900.0f + 1500.0f * (u32) em->slotNo;
            em->posY = 1050.0f;
            em->posZ = 13900.0f;
            break;
        case AREA_JUNGLE_NIGHT_5:
            em->posX = 10000.0f + 1500.0f * (u32) em->slotNo;
            em->posY = 200.0f;
            em->posZ = 8000.0f;
            break;
        case AREA_JUNGLE_NIGHT_7:
            em->posX = 9900.0f + 1500.0f * (u32) em->slotNo;
            em->posY = 300.0f;
            em->posZ = 10350.0f;
            break;
        case AREA_DESERT_NIGHT_11:
            em->posX = 10000.0f + 1500.0f * (u32) em->slotNo;
            em->posY = 0.0f;
            em->posZ = 5700.0f;
            em->rotY = 0x4000;
            break;
        case AREA_VOLCANO_NIGHT_9:
            em->posX = 14300.0f + 1500.0f * (u32) em->slotNo;
            em->posY = 0.0f;
            em->posZ = 7100.0f;
            break;
        case AREA_FOREST_HILLS_4:
            em->posX = 8500.0f;
            em->posY = 0.0f;
            em->posZ = 10000.0f;
            em->rotY = 0x5000;
            break;
        case AREA_FOREST_HILLS_5:
            em->posX = 10500.0f + 1500.0f * (u32) em->slotNo;
            em->posY = 0.0f;
            em->posZ = 9500.0f;
            break;
        default:
            em->posX = 5000.0f + 1500.0f * (u32) em->slotNo;
            em->posY = 0.0f;
            em->posZ = 9000.0f;
            break;
        }
    }
    Em_SetMotionRow(em, 1, 0, 0);
    em->stance = 0;
    w->unk00A = 0;
    if ((Quest_FindSpecialMonster(em->type) & 0xFF) == 0xFF) {
        switch (gQuest.stage) {
        case 0x138C:
        case 0x138F:
            em->hp = 2000;
            em->hpMax = 4000;
            break;
        default:
            em->hpMax = em->hp = Em_ComputeMaxHp(em, 0);
            break;
        }
    } else {
        QuestEm_LoadPersistentHp(em);
    }
    em->scriptPending = 1;
    em->blind = 0;
    em->unk_InitFlag = 1;
    em->aimAngle = 0x4000;
    em->spawnParam = 0;
    w->windOn = 0;
    w->pitchStep = 0x100;
    w->turnSpeed = 0x200;
    w->rollSpeed = 0x100;
    w->moveAngBY = 0x2000;
    w->moveAngBX = 0;
    w->unk01B = 0;
    em->meshShown[1] = 0;
    if (Em_PartBroken(em, 2) == 0) {
        StageObj_K09_Create(em);
    } else {
        em->fxFlags |= 1;
        if (em->type == 0x36 || em->type == 0x3C) {
            em->partBreakCount = 1;
        } else {
            em->partBreakCount = 2;
        }
    }
}

void EmKushala_Start(Em *em)
{
    EmKushala_Init(em);
    if (em->type == 0x36 || em->type == 0x3C) {
        switch (gQuest.stage) {
        case 0x138A:
            if (Save_TestEventFlag(0x9D) == 0) {
                em->area = Area_IdForCurrentTime(AREA_JUNGLE_3);
                em->posX = 7146.0f;
                em->posY = 200.0f;
                em->posZ = 11034.0f;
                em->rotY = 0x5800;
            }
            break;
        }
    } else {
        switch (gQuest.stage) {
        case 0x138F:
            if (Save_TestEventFlag(0xA0) == 0) {
                em->area = Area_IdForCurrentTime(AREA_TOWER_0x7C);
                em->posX = 11600.0f;
                em->posY = -20.0f;
                em->posZ = 13000.0f;
                em->rotY = 0x6800;
            }
            break;
        case 0x430:
        case 0x431:
            if (Save_TestEventFlag(0xA3) == 0) {
                em->area = Area_IdForCurrentTime(AREA_VOLCANO_7);
                em->posX = 9329.0f;
                em->posY = 3000.0f;
                em->posZ = 9973.0f;
                em->rotY = 0xFC00;
            }
            break;
        }
    }
    EmKushala_ChangeState(em, 0, 1, 0);
}

void EmKushala_Reinit(Em *em)
{
    u8 type;

    EmKushala_Init(em);
    type = em->type;
    if ((type == 1) || (type == 0x2A) || (type == 0x25)) {
        em->rotY = 0x8A76;
        em->posX = 11948.9f;
        em->posY = 1010.0f;
        em->posZ = 15923.8f;
        return;
    }
    em->rotY = 0x4861;
    em->posX = 3743.2f;
    em->posY = 1010.0f;
    em->posZ = 11415.0f;
}

void EmKushala_ToIdle(Em *em)
{
    u16 *t = EmKushala_IdleTbl;
    s32 found = 0;

    switch (em->stance) {
    case 2:
        em->scriptPending = 1;
        EmKushala_ChangeState(em, 2, 2, 0);
        break;
    case 1:
        em->scriptPending = 1;
        EmKushala_ChangeState(em, 0, 2, 0);
        break;
    case 0:
    default:
        if (em->stateGroup != 0 || em->state != 0x17) {
            if (em->combat == 0) {
                for (; *t != 0xFFFF; t++) {
                    if (em->motionId[0] == *t) {
                        found = 1;
                        break;
                    }
                }
            }
        }
        if (found) {
            EmKushala_ChangeState(em, 0, 0x17, 0);
        } else {
            em->scriptPending = 1;
            EmKushala_ChangeState(em, 0, 1, 0);
        }
        break;
    }
}

void EmKushala_EndState(Em *em, s32 unused1, s32 unused2)
{
    EmKushala_ToIdle(em);
}

void EmKushala_EndState2(Em *em, u8 next)
{
    s32 k;

    k = next & 0xFF;
    em->stance = 2;
    em->scriptPending = 1;
    em->motionSpeed = 1.0f;
    switch (k) {
    case 0:
        Em_SetStateRand(em, 2, 2);
        break;
    case 1:
        Em_SetStateRand(em, 2, 3);
        break;
    }
}

void EmKushala_BreakParts(Em *em, s32 apply)
{
    u8 k;

    if (em->type == 0x40 || em->type == 0x41) {
        k = em->flinchPart;
        switch (k) {
        case 0:
            if (em->hp < (s16) (0.6f * em->hpMax) && em->parts[k].state >= 2 && (em->partFlags[k] & 0x20)) {
                Em_FlagPartBreak(em, 0);
            }
            break;
        case 5:
            if (em->parts[k].state >= 2) {
                Em_FlagPartBreak(em, 1);
            }
            break;
        }
    } else {
        k = em->flinchPart;
        switch (k) {
        case 0:
            if (em->hp < (s16) (0.6f * em->hpMax) && em->parts[k].state >= 2) {
                Em_FlagPartBreak(em, 0);
            }
            break;
        case 5:
            if (em->parts[k].state >= 2) {
                Em_FlagPartBreak(em, 1);
            }
            break;
        }
    }
    if (apply != 0) {
        Em_ApplyPartBreaks(em);
    }
}

void EmKushala_CreateBreathAtBone(Em *em, s32 bone, u8 kind, u8 arg3, s32 arg4, s32 arg5)
{
    if (gScene.map == MAP_SNOWY_MOUNTAINS || gScene.map == MAP_SNOWY_MOUNTAINS_NIGHT) {
        switch (kind) {
        case 0:
        case 2:
            arg3 += 3;
            break;
        case 1:
            arg3 += 1;
            break;
        }
    }
    KushalaBreath_CreateAtBone(em, bone, kind, arg3, arg4, arg5);
}

void EmKushala_State0_00(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        if (em->motionId[0] != 0x3E9) {
            Em_SetLayerMotion(em, 0x3E9, 0xA, 0, 0);
        }
        if (em->motionId[1] != 0x4B1) {
            Em_SetLayerMotion(em, 0x4B1, 0xA, 0, 1);
        }
        if (em->motionId[2] != 0x579) {
            Em_SetLayerMotion(em, 0x579, 0xA, 0, 2);
        }
        break;
    case 1:
        if (--em->timer <= 0) {
            em->scriptPending = 1;
        }
        break;
    }
}

void EmKushala_State0_01(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 1, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
        case 2:
            if (em->remote == 0) {
                em->step++;
                EmKushala_EndState(em, 0, 0);
            }
        }
        break;
    }
}

void EmKushala_State0_02(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0xB, 0, 0);
        em->stance = 1;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKushala_State0_03(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x1F, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKushala_State0_04(Em *em, EmWork *work, s32 variant)
{
    f32 pos[3];
    f32 ofs[3];
    f32 m[16];

    Em_SetHitReactMode(em, EM_HIT_REACT_NO_FLINCH);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x23, 0, 0);
        em->timer = 3;
        break;
    case 1:
        if ((EmMotion_PassedFrameScaled(em, 56.0f, 0) || EmMotion_PassedFrameScaled(em, 102.0f, 0)) && variant == 0 && ((s32)Player_InCurrentArea(em) & 0xFF)) {
            StageObj_K15_Create(em, 1);
            Mtx_Copy(m, (f32 *)Em_Bone(em, 0x25));
            ofs[0] = 0.0f;
            ofs[1] = -45.0f;
            ofs[2] = 165.0f;
            Vec_RotateSelf(ofs, m);
            Mtx_GetTrans(pos, m);
            pos[0] += ofs[0];
            pos[1] += ofs[1];
            pos[2] += ofs[2];
            Effect02_SpawnForOwner(em, 0, 9, 0, pos, 1.0f);
            Effect0D_SpawnAt(pos, 0x14, 4.0f);
        }
        if (em->motion[0].playing == 0 && --em->timer <= 0) {
            em->step++;
            if (variant == 0) {
                EmKushala_ChangeState(em, 0, 5, 4);
            } else {
                EmKushala_ChangeState(em, 0, 0x1A, 4);
            }
        }
        break;
    case 2:
        if (em->remote == 0) {
            if (variant == 0) {
                EmKushala_ChangeState(em, 0, 5, 4);
            } else {
                EmKushala_ChangeState(em, 0, 0x1A, 4);
            }
        }
        break;
    }
}

void EmKushala_State0_05(Em *em, EmWork *work, s32 variant)
{
    u8 type;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x24, 0, 0);
        type = em->type;
        if (((type == 0x36) || (type == 0x3C)) && (variant == 0)) {
            EmMine_CreateTimed0(em, &em->posX);
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKushala_State0_06(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x22, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKushala_State0_07(Em *em, EmWork *work, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x46, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKushala_State0_08(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x46, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKushala_State0_09(Em *em)
{
    u8 type;

    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x26, 0, 0);
        Em_LoadReactionSet(em, 1);
        em->timer = 0x2328;
        break;
    case 1:
        type = em->type;
        if ((type == 0x36) || (type == 0x3C)) {
            Em_RegenHp(em, 1, (s32) (0.4f * (f32) em->hpMax), 8);
        } else {
            Em_RegenHp(em, 1, (s32) (0.6f * (f32) em->hpMax), 8);
        }
        if (--em->timer <= 0) {
            em->step++;
            Em_ClearLowHpNeed(em);
            EmKushala_ChangeState(em, 0, 0xA, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmKushala_ChangeState(em, 0, 0xA, 4);
        }
        break;
    }
}

void EmKushala_State0_0A(Em *em)
{
    u8 type;

    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x27, 0, 0);
        Em_LoadReactionSet(em, 0);
        type = em->type;
        if ((type == 0x40) || (type == 0x41)) {
            EmMine_CreateTimed0(em, &em->posX);
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            if (em->combat == 1) {
                em->step++;
                Em_SetMotion(em, 0x1E, 0, 0);
                break;
            }
            em->blind = 0;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->blind = 0;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKushala_State0_0B(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x47, 0, 0);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            EmKushala_ChangeState(em, 0, 0xC, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmKushala_ChangeState(em, 0, 0xC, 4);
        }
        break;
    }
}

void EmKushala_State0_0C(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x42, 0, 0);
        Em_LoadReactionSet(em, 0);
        em->blind = 2;
        em->timer = 2;
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            if (--em->timer <= 0) {
                em->step++;
                Em_SetMotion(em, 0x43, 0, 0);
                break;
            }
        }
        break;
    case 2:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            EmKushala_ChangeState(em, 0, 8, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmKushala_ChangeState(em, 0, 8, 4);
        }
        break;
    }
}

void EmKushala_State0_0D(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_NO_FLINCH);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x23, 0, 0);
        Em_SelectSightRow(em, 1);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_ChangeState(em, 0, 0xE, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmKushala_ChangeState(em, 0, 0xE, 4);
        }
        break;
    }
}

void EmKushala_State0_0E(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x24, 0, 0);
        Em_SelectSightRow(em, 0);
        Em_LoadReactionSet(em, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKushala_State0_0F(Em *em, EmWork *work)
{
    u16 t;
    u32 d;
    s32 a;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        if (em->motionId[0] == 0x407) {
            Em_SetMotion(em, 0x20, 0xA, 0x1E);
        } else {
            Em_SetMotion(em, 0x20, 0, 0);
        }
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->noReact = 1;
        break;
    case 1:
        a = em->rotY;
        t = em->aimAngle;
        d = (t - (a & 0xFFFF)) & 0xFFFF;
        if (((d + 0x200) & 0xFFFF) < 0x400U) {
            em->rotY = t;
        } else if (d < 0x8000U) {
            em->rotY = (a + 0x200) & 0xFFFF;
        } else {
            em->rotY = (a - 0x200) & 0xFFFF;
        }
        if (EmMotion_ReachedFrame(em, 0, 60.0f) != 0) {
            em->step++;
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->noReact = 0;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKushala_State0_12(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0xF, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKushala_State0_13(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x1F, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKushala_State0_15(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x47, 0, 0);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            EmKushala_ChangeState(em, 0, 0x16, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmKushala_ChangeState(em, 0, 0x16, 4);
        }
        break;
    }
}

void EmKushala_State0_16(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x42, 0, 0);
        Em_LoadReactionSet(em, 0);
        em->blind = 2;
        em->timer = 2;
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            if (--em->timer <= 0) {
                em->step++;
                Em_SetMotion(em, 0x43, 0, 0);
                break;
            }
        }
        break;
    case 2:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            EmKushala_ChangeState(em, 0, 8, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmKushala_ChangeState(em, 0, 8, 4);
        }
        break;
    }
}

void EmKushala_State0_17(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 0;
        Em_SetMotion(em, 0x11, 0, 0x92);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKushala_State0_1B(Em *em, EmWork *work)
{
    u16 t;
    u32 d;
    s32 a;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        if (em->motionId[0] == 0x407) {
            Em_SetMotion(em, 0x20, 0xA, 0x1E);
        } else {
            Em_SetMotion(em, 0x20, 0, 0);
        }
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->noReact = 1;
        break;
    case 1:
        a = em->rotY;
        t = em->aimAngle;
        d = (t - (a & 0xFFFF)) & 0xFFFF;
        if (((d + 0x200) & 0xFFFF) < 0x400U) {
            em->rotY = t;
        } else if (d < 0x8000U) {
            em->rotY = (a + 0x200) & 0xFFFF;
        } else {
            em->rotY = (a - 0x200) & 0xFFFF;
        }
        if (EmMotion_ReachedFrame(em, 0, 60.0f) != 0) {
            em->step++;
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->noReact = 0;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKushala_State1_00(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 2, 0, 0);
        break;
    case 1:
        if (Em_StepToGoal(em, 0x40, 0) == 1) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKushala_State1_01(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x25, 0, 0);
        break;
    case 1:
        if (Em_StepToGoal(em, 0x40, 0) == 1) {
            if (em->combat == 0) {
                em->step++;
                Em_SetMotion(em, 0x11, 0, 0x92);
                break;
            }
            EmKushala_EndState(em, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKushala_State1_02(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_StartTurnToTarget(em, &EmKushala_TurnParams[0]);
        break;
    case 1:
        if (Em_TurnStep(em, &EmKushala_TurnParams[0]) == 1) {
            em->step++;
            Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKushala_State1_03(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        work->turnSpeed = (em->aimAngle - em->rotY) & 0xFFFF;
        if (work->turnSpeed <= 0x238E || work->turnSpeed >= 0xDC72) {
            Em_SetMotion(em, 7, 0, 0);
        } else if (work->turnSpeed >= 0x8000) {
            Em_SetMotion(em, 9, 0, 0);
        } else {
            Em_SetMotion(em, 8, 0, 0);
        }
        if (work->turnSpeed >= 0x8000) {
            work->turnSpeed -= 0x10000;
        }
        break;
    case 1:
        switch (em->motionId[0]) {
        case 0x3EF:
        case 0x3F1:
        case 0x3F0:
            if (EmMotion_InFrameRange(em, 0, 16.0f, 54.0f)) {
                em->rotY = (em->rotY + work->turnSpeed / 19) & 0xFFFF;
            }
            break;
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKushala_State1_04(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 4, 0, 0);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x14, 0, 0);
        }
        
    case 2:
        if (Em_StepToGoal(em, 0x40, 0) == 1) {
            em->step = 3;
            Em_SetMotion(em, 0x18, 0, 0);
        }
        break;
    case 3:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKushala_State1_05(Em *em, EmWork *work, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x15, 0, 0);
        break;
    case 1:
        switch (variant) {
        case 1:
            em->rotY += 0x5B;
            break;
        case 2:
            em->rotY += 0xE3;
            break;
        case 3:
            em->rotY -= 0x5B;
            break;
        case 4:
            em->rotY -= 0xE3;
            break;
        }
        if (Em_StepToGoal(em, 0x40, 0) == 1) {
            em->step++;
            Em_SetMotion(em, 0x18, 0, 0);
            break;
        }
        if (em->motionId[0] == 0x3FD && EmMotion_PassedFrameScaled(em, 12.0f, 0)) {
            EmKushala_CreateAttack(em, 8);
        }
        if (em->motion[0].playing == 0) {
            if (em->motionId[0] == 0x3FD) {
                Em_SetMotion(em, 0x16, 0, 0);
            } else if (em->motionId[0] == 0x3FE) {
                Em_SetMotion(em, 0x17, 0, 0);
            }
        }
        break;
    case 2:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKushala_State1_06(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x19, 0, 0);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x1A, 0, 0);
        }
        break;
    case 2:
        if (em->motion[1].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x1F, 0, 0);
        }
        break;
    case 3:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKushala_State1_07(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x19, 0, 0);
        em->stance = 0;
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            em->stance = 1;
            Em_SetMotion(em, 0x1B, 0, 0);
        }
        break;
    case 2:
        if (em->motion[1].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0xB, 0, 0x1C);
        }
        break;
    case 3:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKushala_State1_14(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 0;
        if (em->motionId[0] == 0x3E9) {
            Em_SetMotion(em, 0x1E, 0, 0);
            break;
        }
        em->step++;
        Em_SetMotion(em, 0xA, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0xA, 0, 0);
        }
        break;
    case 2:
        if (EmMotion_ReachedFrame(em, 0, 48.0f) != 0) {
            em->step++;
            em->stance = 1;
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKushala_State1_15(Em *em, EmWork *work, s32 variant)
{
    f32 ofs[3];
    f32 out[3];
    s32 ang[3];
    f32 m[16];
    s32 t;
    s32 d;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        if (variant == 0) {
            Em_SetMotion(em, 0x13, 0, 0);
        } else {
            Em_SetMotion(em, 0x14, 0, 0);
        }
        Player_ClearVelocity(em);
        em->stance = 1;
        break;
    case 1:
        if (EmMotion_InFrameRange(em, 0, 0.0f, 88.0f)) {
            ang[0] = 0;
            ang[1] = em->rotY;
            ang[2] = 0;
            if (variant == 0) {
                ofs[0] = 50.0f;
            } else {
                ofs[0] = -50.0f;
            }
            ofs[1] = 0.0f;
            ofs[2] = 0.0f;
            Mtx_FromAnglesYXZ(ang, m);
            Vec_Rotate(out, ofs, m);
            em->posX += out[0];
            em->posZ += out[2];
            if (variant == 0) {
                t = ((Math_AngleXZ(&em->posX, em->goal) & 0xFFFF) + 0xC000) & 0xFFFF;
            } else {
                t = ((Math_AngleXZ(&em->posX, em->goal) & 0xFFFF) + 0x4000) & 0xFFFF;
            }
            d = ((t & 0xFFFF) - em->rotY) & 0xFFFF;
            if (d <= 0x8000) {
                if (d <= 0x173) {
                    em->rotY += d;
                } else {
                    em->rotY += 0x174;
                }
            } else {
                if (d > 0xFE8C) {
                    em->rotY += d;
                } else {
                    em->rotY -= 0x174;
                }
            }
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKushala_State1_17(Em *em, EmWork *work, s32 variant)
{
    u8 flags;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0xB, 0, 0);
        em->stance = 1;
        em->velX = 0.0f;
        em->velY = 0.0f;
        em->velZ = 30.0f;
        Player_ClearAccel2(em);
        if (variant == 1) {
            em->speedBonus = 0x32;
        }
        break;
    case 1:
        work->moveAngX = 0;
        work->moveAngY = em->rotY;
        if (variant == 1) {
            flags = 4;
            work->moveAngY = (work->moveAngY + 0x7FFF + 1) & 0xFFFF;
        } else {
            flags = 0;
        }
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
        em->goalDist -= em->velZ;
        if (em->hasGoal != 0) {
            Em_TurnToGoal(em, 0x40, flags);
        }
        if (em->goalDist <= 0.0f) {
            em->step++;
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKushala_State1_18(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0xD, 0, 0);
        EmKushala_StartCurves(em, 1U);
        em->stance = 1;
        break;
    case 1:
        EmKushala_UpdateCurves(em);
        em->goalDist -= em->velZ;
        if ((em->motionId[0] == 0x3F5) && (EmMotion_PassedFrameScaled(em, 58.0f, 0U) != 0)) {
            EmKushala_CreateAttack(em, 9);
            EmKushala_CreateAttack(em, 0x1A);
        }
        if (em->motion[0].playing == 0) {
            if (em->goalDist <= 0.0f) {
                em->step += 2;
                Em_SetMotion(em, 0xE, 0, 0);
                EmKushala_StartCurves(em, 2U);
                break;
            }
            em->step++;
            em->velX = 0.0f;
            em->velY = 0.0f;
            em->velZ = 80.0f;
            Player_ClearAccel2(em);
            Em_SetMotion(em, 0xC, 0, 0);
        }
        break;
    case 2:
        work->moveAngX = 0;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
        em->goalDist -= em->velZ;
        if (Em_StepToGoal(em, 0x40, 0) == 1) {
            em->step++;
            Em_SetMotion(em, 0xE, 0, 0);
            EmKushala_StartCurves(em, 2U);
        }
        break;
    case 3:
        EmKushala_UpdateCurves(em);
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKushala_State1_19(Em *em, EmWork *work)
{
    s32 d;
    s32 dd;
    s32 a;
    u16 t;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 1;
        em->lookAtTarget = 0;
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        work->turnSpeed = (em->aimAngle - em->rotY) & 0xFFFF;
        if (work->turnSpeed <= 0x4000 || work->turnSpeed >= 0xC000) {
            Em_SetMotion(em, 0xB, 0, 0);
            em->timer = 0x1F;
        } else {
            Em_SetMotion(em, 0x12, 0, 0);
            em->timer = 0x1B;
        }
        if (work->turnSpeed >= 0x8000) {
            work->turnSpeed -= 0x10000;
        }
        break;
    case 1:
        if (em->motion[0].blend == 0) {
            d = work->turnSpeed / em->timer;
            a = em->rotY;
            t = em->aimAngle;
            dd = (t - a) & 0xFFFF;
            if (((dd + d) & 0xFFFF) < d * 2) {
                em->rotY = t & 0xFFFF;
            } else {
                em->rotY = (a + d) & 0xFFFF;
            }
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKushala_State1_1A(Em *em, EmWork *work, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 1;
        Em_SetMotion(em, 0x11, 0, 0);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 70.0f) != 0) {
            em->step++;
            em->stance = 0;
        }
        break;
    case 2:
        if (variant == 0) {
            if (em->motion[0].playing == 0) {
                em->step++;
                EmKushala_EndState(em, 0, 0);
                break;
            }
        } else if (EmMotion_ReachedFrame(em, 0, 146.0f) != 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKushala_State1_1B(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x12, 0, 0);
        em->lookAtTarget = 0;
        em->stance = 1;
        break;
    case 1:
        if ((EmMotion_ReachedFrame(em, 0, 6.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 54.0f) == 0)) {
            em->rotY -= 0x555;
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKushala_State2_00(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        work->flyGuard = 0;
        if (em->motionId[0] == 0x3E9) {
            Em_SetMotion(em, 0x1E, 0, 0);
            break;
        }
        em->step++;
        Em_SetMotion(em, 0xA, 0, 0);
        EmKushala_StartCurves(em, 0U);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0xA, 0, 0);
            EmKushala_StartCurves(em, 0U);
        }
        break;
    case 2:
        if (EmMotion_PassedFrameScaled(em, 48.0f, 0U) != 0) {
            em->step++;
            em->stance = 2;
            EmKushala_UpdateCurves(em);
        }
        break;
    case 3:
        EmKushala_UpdateCurves(em);
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState2(em, 0U);
        }
        break;
    }
}

void EmKushala_State2_01(Em *em, EmWork *work, s32 variant)
{
    f32 h;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        em->rotX = 0;
        em->rotZ = 0;
        Em_SetMotion(em, 0xB, 0, 0);
        Player_ClearVelocity(em);
        em->velY = -10.0f;
        h = em->floorY;
        h = em->posY - (1000.0f + h);
        if (h > 0.0f) {
            do {
                em->velY -= 0.75f;
                h += em->velY;
            } while (h > 0.0f && !(em->velY < -200.0f));
        }
        work->flyGuard = 0;
        work->moveAngX = 0;
        work->moveAngZ = 0;
        break;
    case 1:
        work->moveAngX = 0;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
        if (em->velY < -10.0f) {
            em->velY += 0.75f;
        }
        if (!(em->floorY < em->posY)) {
            em->velY = 0.0f;
            em->posY = em->floorY;
        }
        if (em->motion[0].playing == 0 && em->velY == 0.0f) {
            em->step++;
            Em_SetMotion(em, 0x11, 0, 0);
            em->rotX = 0;
            em->rotZ = 0;
            em->stance = 0;
            CamShake_ReqAIf(em, 1);
        }
        break;
    case 2:
        if (variant == 0) {
            if (em->motion[0].playing == 0) {
                em->step++;
                EmKushala_EndState(em, 0, 0);
            }
        } else if (EmMotion_ReachedFrame(em, 0, 146.0f)) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
    if (em->posY < em->floorY) {
        em->posY = em->floorY;
    }
}

void EmKushala_State2_02(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        Em_SetMotion(em, 0xB, 0, 0);
        work->flyGuard = 0;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState2(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmKushala_State2_03(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        Em_SetMotion(em, 0xC, 0, 0);
        Player_ClearAccel2(em);
        em->velY = 0.0f;
        em->velX = 0.0f;
        work->flyGuard = 1;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState2(em, 1U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmKushala_State2_04(Em *em, EmWork *work)
{
    f32 dist;
    f32 floorY;
    s32 timer;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        work->turnSpeed = 0x100;
        Em_SetMotion(em, 0xB, 0, 0);
        work->flyGuard = 0;
        break;
    case 1:
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 1);
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        work->moveAngX = 0;
        Em_MoveRotated(em, &work->moveAngX);
        dist = Vec_DistanceXZ(&em->posX, em->goal);
        timer = em->timer - 1;
        em->timer = timer;
        if ((timer <= 0) || (dist <= (10.0f * em->velZ))) {
            em->step++;
            Player_ClearVelocity(em);
            EmKushala_EndState2(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmKushala_State2_05(Em *em, EmWork *work)
{
    f32 dist;
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        work->turnSpeed = 0x100;
        Em_SetMotion(em, 0xB, 0, 0);
        work->flyGuard = 0;
        break;
    case 1:
        work->moveAngY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        work->moveAngZ = 0;
        Em_GetPitchToGoal(em, &work->moveAngX, 0.0f, 0.0f);
        Em_MoveRotated(em, &work->moveAngX);
        dist = Vec_Distance((f32 *) &em->posX, (f32 *) em->goal);
        if (--em->timer > 0) {
            if (dist <= (10.0f * em->velZ)) {
                goto block_9;
            }
        } else {
block_9:
            em->step++;
            Player_ClearVelocity(em);
            EmKushala_EndState2(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmKushala_State2_06(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        Em_SetMotion(em, 0x12, 0, 0);
        em->lookAtTarget = 0;
        work->flyGuard = 0;
        break;
    case 1:
        if ((EmMotion_ReachedFrame(em, 0, 6.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 54.0f) == 0)) {
            em->rotY -= 0x555;
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState2(em, 2U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmKushala_State2_07(Em *em, EmWork *work)
{
    f32 floorY;
    s32 done;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        Em_SetMotion(em, 0xB, 0, 0);
        work->turnSpeed = 0x180;
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        work->flyGuard = 0;
        em->timer = 0x96;
        break;
    case 1:
        done = Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 1) & 0xFF;
        if (--em->timer > 0) {
            if (done != 0) {
                goto block_9;
            }
        } else {
block_9:
            em->step++;
            EmKushala_EndState2(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmKushala_State2_08(Em *em, EmWork *work)
{
    u16 t;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        Player_ClearVelocity(em);
        Em_SetMotion(em, 0xD, 0, 0);
        EmKushala_StartCurves(em, 1);
        Em_AreaExitPoint(em, (f32 *) em->goal);
        work->flyGuard = 1;
        break;
    case 1:
        EmKushala_UpdateCurves(em);
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0xC, 0, 0);
            Player_ClearVelocity(em);
            em->velZ = 80.0f;
        }
        break;
    case 2:
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 0x4E1, &EmKushala_FlyParam);
        work->moveAngX = em->rotX;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
        if (!Area_ContainsPos(em->area, &em->posX)) {
            em->step++;
        }
        break;
    case 3:
        em->step++;
        break;
    case 4:
        em->step++;
        em->timer = 600;
        Em_EnterNextArea(em);
        t = *(u16 *) &em->destArea;
        if (em->nextArea == t || t == AREA_NONE) {
            Hud_TriggerGlow(em);
            EmKushala_ChangeState(em, 2, 9, 1);
        } else {
            if (em->remote == 0) {
                em->nextArea = t;
                em->goalNo = t;
                em->goalKind = 3;
                Em_ComputeGoal(em, em->goal);
            }
            EmKushala_ChangeState(em, 2, 0xA, 1);
            Hud_TriggerGlow(em);
        }
        break;
    case 5:
        if (em->remote == 0) {
            em->step = 4;
        }
        break;
    }
    if (em->posY < em->floorY) {
        em->posY = em->floorY;
    }
}

void EmKushala_State2_09(Em *em, EmWork *work)
{
    f32 d;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        work->turnSpeed = 0x100;
        Em_SetMotion(em, 0xC, 0, 0);
        Player_ClearVelocity(em);
        em->velZ = 80.0f;
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->rotY = em->aimAngle;
        *(u16 *) &em->destArea = AREA_NONE;
        work->flyGuard = 1;
        break;
    case 1:
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 0xE5, &EmKushala_FlyParam);
        work->moveAngX = em->rotX;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
        d = Vec_DistanceXZ(&em->posX, em->goal);
        em->timer--;
        if (d <= 1000.0f || em->timer < 0) {
            em->step++;
            Em_SetMotion(em, 0xE, 0, 0);
            EmKushala_StartCurves(em, 2);
        }
        break;
    case 2:
        EmKushala_UpdateCurves(em);
        if (em->rotX != 0) {
            if (em->rotX < 0x8000) {
                em->rotX -= work->pitchStep;
            } else {
                em->rotX += work->pitchStep;
            }
            em->rotX = (u16) em->rotX;
        }
        if (em->rotZ != 0) {
            if (em->rotZ < 0x8000) {
                em->rotZ -= work->rollSpeed;
            } else {
                em->rotZ += work->rollSpeed;
            }
            em->rotZ = (u16) em->rotZ;
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 600;
            EmKushala_ChangeState(em, 2, 1, 1);
        }
        break;
    case 3:
        if (em->remote == 0) {
            em->timer = 600;
            EmKushala_ChangeState(em, 2, 1, 1);
        }
        break;
    }
    if (em->posY < em->floorY) {
        em->posY = em->floorY;
    }
}

void EmKushala_State2_0A(Em *em, EmWork *work)
{
    AreaRecord *area;
    f32 *pt;
    u16 next;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        work->turnSpeed = 0x100;
        Em_SetMotion(em, 0xC, 0, 0);
        Player_ClearVelocity(em);
        em->velZ = 80.0f;
        area = Stage_GetAreaRecord(em->area);
        em->goal[1] = area->bottom;
        pt = Em_AreaEntryValue(em, *em->tables);
        if (pt == NULL) {
            em->goal[0] = area->sizeX / 2.0f;
            em->goal[2] = area->sizeZ / 2.0f;
        } else {
            em->goal[0] = pt[0];
            em->goal[2] = pt[2];
        }
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->rotY = em->aimAngle;
        work->flyGuard = 1;
        em->timer = 900;
        break;
    case 1:
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 0xE5, &EmKushala_FlyParam);
        work->moveAngX = em->rotX;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
        if (Vec_DistanceXZ(&em->posX, em->goal) <= 1000.0f || --em->timer <= 0) {
            em->step++;
            em->timer = 900;
            Em_AreaExitPoint(em, (f32 *) em->goal);
        }
        break;
    case 2:
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 0xE1, &EmKushala_FlyParam);
        work->moveAngX = em->rotX;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
        if (Area_ContainsPos(em->area, &em->posX) == 0 || --em->timer <= 0) {
            em->step++;
        }
        break;
    case 3:
        em->step++;
        em->timer = 600;
        Em_EnterNextArea(em);
        next = *(u16 *)&em->destArea;
        if (em->nextArea == next || next == AREA_NONE) {
            Hud_TriggerGlow(em);
            EmKushala_ChangeState(em, 2, 9, 1);
        } else {
            if (em->remote == 0) {
                em->nextArea = next;
                em->goalNo = next;
                em->goalKind = 3;
                Em_ComputeGoal(em, em->goal);
            }
            EmKushala_ChangeState(em, 2, 0xA, 1);
            Hud_TriggerGlow(em);
        }
        break;
    case 4:
        if (em->remote == 0) {
            em->step = 3;
        }
        break;
    }
    if (em->posY < em->floorY) {
        em->posY = em->floorY;
    }
}

void EmKushala_State2_0B(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        if (em->motionId[0] != 0x3F5) {
            Em_SetMotion(em, 0xD, 0, 0);
            EmKushala_StartCurves(em, 1U);
        } else {
            em->step++;
        }
        work->flyGuard = 1;
        break;
    case 1:
        EmKushala_UpdateCurves(em);
        if (em->motion[0].playing == 0) {
            em->step++;
            em->velX = 0.0f;
            em->velY = 0.0f;
            Player_ClearAccel2(em);
            em->velZ = 80.0f;
            Em_SetMotion(em, 0xC, 0, 0);
        }
        break;
    case 2:
        if (!(Vec_DistanceXZ(&em->posX, em->goal) <= 1000.0f)) {
            if (--em->timer <= 0) {
                goto block_17;
            }
        } else {
block_17:
            em->step++;
            EmKushala_ChangeState(em, 2, 0xC, 1);
        }
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 0x121, &EmKushala_FlyParam2);
        work->moveAngX = em->rotX;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
        break;
    case 3:
        if (em->remote == 0) {
            EmKushala_ChangeState(em, 2, 0xC, 1);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmKushala_State2_0C(Em *em, EmWork *work)
{
    f32 floorY;
    s32 rotX;
    s32 rotZ;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0xE, 0, 0);
        work->flyGuard = 1;
        EmKushala_StartCurves(em, 2U);
        break;
    case 1:
        EmKushala_UpdateCurves(em);
        rotX = em->rotX;
        if (rotX != 0) {
            if (rotX < 0x8000) {
                em->rotX = rotX - work->pitchStep;
            } else {
                em->rotX = rotX + work->pitchStep;
            }
            em->rotX = (s32) (u16) em->rotX;
        }
        rotZ = em->rotZ;
        if (rotZ != 0) {
            if (rotZ < 0x8000) {
                em->rotZ = rotZ - work->rollSpeed;
            } else {
                em->rotZ = rotZ + work->rollSpeed;
            }
            em->rotZ = (s32) (u16) em->rotZ;
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState2(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmKushala_State2_0D(Em *em, EmWork *work)
{
    f32 floorY;
    s32 t;
    u8 n;
    u8 stateVar1;

    switch (em->step) {
    case 0:
        em->stance = 2;
        Em_SetMotion(em, 0xC, 0, 0);
        em->step++;
        em->stateVar1 = 0;
        em->lookAtTarget = 0;
        em->velX = 0.0f;
        em->velY = 0.0f;
        Player_ClearAccel2(em);
        if (em->velZ <= 80.0f) {
            em->velZ = 80.0f;
        }
        work->flyGuard = 1;
        break;
    case 1:
        stateVar1 = em->stateVar1;
        if ((stateVar1 == 0) && (em->motion[0].playing == 0)) {
            em->stateVar1 = stateVar1 + 1;
            Em_SetMotion(em, 0xE, 0, 0);
        }
        if ((Vec_DistanceXZ(&em->posX, em->goal) <= 3000.0f) || (t = em->timer - 1, em->timer = t, (t <= 0))) {
            n = ((EmKushalaWork *)work)->legsLeft - 1;
            ((EmKushalaWork *)work)->legsLeft = n;
            if ((n & 0xFF) <= 0) {
                EmKushala_EndState2(em, 1U);
            } else {
                em->curGoalNo++;
                em->curGoalNo &= 3;
                Em_ComputeGoal2(em, em->goal);
                em->timer = (s32) ((1.5f * Vec_DistanceXZ(&em->posX, em->goal)) / 50.0f);
                Em_SetStateRand(em, 2, 0xD);
            }
        }
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 0xE1, &EmKushala_FlyParam);
        work->moveAngX = em->rotX;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmKushala_State2_0F(Em *em, EmWork *work, s32 variant)
{
    s32 go = 0;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        work->turnSpeed = 0x100;
        Em_SetMotion(em, 0xB, 0, 0);
        work->flyGuard = 0;
        Player_ClearVelocity(em);
        em->velY = 10.0f;
        break;
    case 1:
        Em_MoveRotated(em, &em->rotX);
        if (!(em->posY <= 500.0f + em->floorY) && em->velY < 100.0f) {
            em->velY += 0.75f;
        }
        switch (variant) {
        case 0:
            if (!(em->posY < gAreaGroundY[em->area])) {
                go = 1;
            }
            break;
        case 1:
            switch (Area_DayId(em->area)) {
            case AREA_BATTLE_TOWN_1:
                if (!(em->posY < 2200.0f)) {
                    go = 1;
                }
                break;
            case AREA_SNOWY_MOUNTAINS_3:
            case AREA_SNOWY_MOUNTAINS_6:
            default:
                if (!(em->posY < 4500.0f)) {
                    go = 1;
                }
                break;
            }
            break;
        case 2:
            if (!(em->posY < em->goal[1])) {
                go = 1;
            }
            break;
        }
        if (go) {
            em->step++;
            Player_ClearVelocity(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState2(em, 0);
        }
        break;
    }
    if (em->posY < em->floorY) {
        em->posY = em->floorY;
    }
}

void EmKushala_State3_00(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x37, 0, 0);
        break;
    case 1:
        if ((EmMotion_ReachedFrame(em, 0, 40.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 60.0f) == 0)) {
            em->rotY += 0x666;
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKushala_State3_01(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x38, 0, 0);
        break;
    case 1:
        if ((EmMotion_ReachedFrame(em, 0, 40.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 60.0f) == 0)) {
            em->rotY -= 0x666;
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKushala_State3_02(Em *em)
{
    u8 type;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x2E, 0, 0);
        break;
    case 1:
        type = em->type;
        if ((type == 0x36) || (type == 0x3C)) {
            if (EmMotion_PassedFrameScaled(em, 74.0f, 0U) != 0) {
                StageObj_K1C_CreateOnEmB(em, 0xA, 0, 0, 1.0f);
                EmKushala_CreateAttack(em, 0x1B);
            }
            if (EmMotion_PassedFrameScaled(em, 78.0f, 0U) != 0) {
                EmKushala_CreateBreathAtBone(em, 0x25, 0, 0, 0, 0);
            }
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKushala_State3_03(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 1;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x2F, 0, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 104.0f, 0U) != 0) {
            EmEffectAttack_CreateChain(em, 0xF, 0, 0, 0);
        }
        if (em->motion[0].playing == 0) {
            if (--em->timer <= 0) {
                em->step++;
                Em_SetMotion(em, 0x30, 0, 0);
                break;
            }
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKushala_State3_04(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x2B, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKushala_State3_05(Em *em, EmWork *work, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 0x64;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x3B, 0, 0);
        em->timer = 0x5A;
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 56.0f, 0U) != 0) {
            StageObj_K27_Create(em);
        }
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x3C, 0, 0);
        }
        break;
    case 2:
        if (EmMotion_PassedFrameScaled(em, 36.0f, 0U) != 0) {
            EmKushala_SpawnEffect02AtBone27(em);
            TeostraBlastRing_Create(em, variant);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKushala_State3_06(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x2A, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKushala_State3_08(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 0x64;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x29, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x21, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKushala_State3_09(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x35, 0, 0);
        em->stance = 1;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKushala_StepForward(Em *em, EmWork *work)
{
    f32 ofs[3];
    f32 out[3];
    s32 ang[3];
    f32 mtx[16];

    ang[0] = 0;
    ang[1] = work->moveAngY;
    ang[2] = 0;
    if (em->type == 0x40 || em->type == 0x41) {
        Vec_Set(ofs, 0.0f, 0.0f, 20.0f);
    } else {
        Vec_Set(ofs, 0.0f, 0.0f, 15.0f);
    }
    Mtx_FromAnglesYXZ(ang, mtx);
    Vec_Rotate(out, ofs, mtx);
    em->posX += out[0];
    em->posZ += out[2];
}

void EmKushala_State3_0A(Em *em, EmWork *work, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 1;
        Em_SetMotion(em, 0x31, 0, 0);
        em->stance = 1;
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            if (--em->timer <= 0) {
                em->step++;
                em->timer = 1;
                Em_SetMotion(em, 0x32, 0, 0);
                work->moveAngY = em->rotY;
                work->moveAngX = 0;
                break;
            }
        }
        break;
    case 2:
        switch (variant) {
        case 1:
            em->rotY = (s32) ((f32) work->moveAngY + (8192.0f * Math_Sin((6.2831855f * (f32) work->moveAngX) / 360.0f)));
            work->moveAngX += 5;
            
        case 0:
            EmKushala_StepForward(em, work);
            break;
        case 2:
            em->rotY += 0x1CA;
            break;
        }
        if (em->motion[0].playing == 0) {
            if (--em->timer <= 0) {
                em->step++;
                Em_SetMotion(em, 0x28, 0, 0);
                break;
            }
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKushala_State3_0B(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 0xC8;
        Em_SetMotion(em, 0x34, 0, 0);
        em->lookAtTarget = 0;
        em->stance = 1;
        Player_ClearVelocity(em);
        if (em->goalDist < 300.0f) {
            em->goalDist = 300.0f;
        }
        if (!(em->goalDist <= 1000.0f)) {
            em->goalDist = 1000.0f;
        }
        em->velZ = em->goalDist / 20.0f;
        break;
    case 1:
        work->moveAngX = 0;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
        em->goalDist -= em->velZ;
        if (EmMotion_ReachedFrame(em, 0, 40.0f) != 0) {
            em->step++;
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKushala_State3_0C(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x36, 0, 0);
        em->stance = 1;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKushala_State3_0D(Em *em, EmWork *work, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 1;
        em->lookAtTarget = 0;
        if (variant == 0) {
            Em_SetMotion(em, 0x39, 0, 0);
            break;
        }
        Em_SetMotion(em, 0x3A, 0, 0);
        break;
    case 1:
        if (EmMotion_InFrameRange(em, 0, 50.0f, 80.0f) != 0) {
            if (variant == 0) {
                em->rotY += 0x444;
            } else {
                em->rotY -= 0x444;
            }
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKushala_State3_0F(Em *em, EmWork *work, s32 variant)
{
    s8 *w;
    s32 k;

    switch (em->step) {
    case 0:
        em->step++;
        em->timer = EmKushala_State3_0FTimers[variant & 0xF][0];
        Em_SetMotion(em, 0x31, 0, 0);
        if (variant & 0x10) {
            em->stance = 2;
        } else {
            em->stance = 1;
        }
        break;
    case 1:
        k = variant & 0xF;
        if (k == 0) {
            if (EmMotion_PassedFrameScaled(em, 40.0f, 0)) {
                em->step++;
                em->timer = D_008CA214[k][0];
                Em_SetMotion(em, 0x33, 0, 0);
            }
        } else if (EmMotion_PassedFrameScaled(em, 106.0f, 0) && --em->timer <= 0) {
            em->step++;
            em->timer = D_008CA214[k][0];
            Em_SetMotion(em, 0x33, 0, 0);
        }
        break;
    case 2:
        if (EmMotion_PassedFrameScaled(em, 20.0f, 0) && (em->type == 0x36 || em->type == 0x3C)) {
            switch (variant & 0xF) {
            case 0:
                StageObj_K1C_CreateOnEmB(em, 0xA, 0x2000, 0, 1.0f);
                EmKushala_CreateBreathAtBone(em, 0x25, 2, 1, 0x2000, 0);
                break;
            case 1:
                switch (em->timer) {
                case 3:
                    StageObj_K1C_CreateOnEmB(em, 0x12, 0x1555, 0xF1C8, 1.0f);
                    EmKushala_CreateBreathAtBone(em, 0x25, 1, 0, 0x1555, 0xF1C8);
                    break;
                case 2:
                    StageObj_K1C_CreateOnEmB(em, 0x12, 0x11C7, 0, 1.0f);
                    EmKushala_CreateBreathAtBone(em, 0x25, 1, 0, 0x11C7, 0);
                    break;
                case 1:
                    StageObj_K1C_CreateOnEmB(em, 0x12, 0x1555, 0xE39, 1.0f);
                    EmKushala_CreateBreathAtBone(em, 0x25, 1, 0, 0x1555, 0xE39);
                    break;
                }
                break;
            case 2:
                StageObj_K1C_CreateOnEmB(em, 0xA, 0x2000, 0, 1.0f);
                EmKushala_CreateBreathAtBone(em, 0x25, 2, 2, 0x2000, 0);
                break;
            case 3:
                switch (em->timer) {
                case 3:
                    w = (s8 *) work;
                    if (em->remote == 0 && w[1] < 10) {
                        w[1]++;
                        if (w[1] >= 10) {
                            Quest_TriggerEvent(em);
                        }
                        gScene.fortStage = w[1];
                    }
                case 1:
                case 2:
                    StageObj_K1C_CreateOnEmB(em, 0xA, 0x11C7, 0, 1.0f);
                    EmKushala_CreateBreathAtBone(em, 0x25, 1, 0, 0x11C7, 0);
                    break;
                }
                break;
            case 4:
                StageObj_K1C_CreateOnEmB(em, 0xA, 0x11C7, 0, 1.0f);
                EmKushala_CreateBreathAtBone(em, 0x25, 1, 0, 0x11C7, 0);
                break;
            }
        }
        if (EmMotion_PassedFrameScaled(em, 46.0f, 0) && --em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x2D, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            if (variant & 0x10) {
                EmKushala_EndState2(em, 0);
            } else {
                EmKushala_EndState(em, 0, 0);
            }
        }
        break;
    }
    if (em->posY < em->floorY) {
        em->posY = em->floorY;
    }
}

void EmKushala_State3_18(Em *em, EmWork *work)
{
    EmKushalaWork *w = (EmKushalaWork *)work;

    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 100;
        em->stance = 0;
        em->lookAtTarget = 0;
        em->timer = 150;
        if (em->motionId[0] == 0x3E9) {
            Em_SetMotion(em, 0x1E, 0, 0);
        } else {
            em->step++;
            Em_SetMotion(em, 0x3B, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x3B, 0, 0);
        }
        break;
    case 2:
        if (EmMotion_PassedFrameScaled(em, 56.0f, 0)) {
            StageObj_K27_Create(em);
        }
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x3C, 0, 0);
        }
        break;
    case 3:
        if (EmMotion_PassedFrameScaled(em, 36.0f, 0)) {
            EmKushala_SpawnEffect02AtBone27(em);
            TeostraBlastRing_Create(em, 3);
            if (em->remote == 0 && w->fortHits <= 9) {
                w->fortHits++;
                if (w->fortHits >= 10) {
                    Quest_TriggerEvent(em);
                }
                gScene.fortStage = w->fortHits;
            }
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            if (em->combat == 1) {
                EmKushala_EndState(em, 0, 0);
            } else {
                Em_SetMotion(em, 0x11, 0, 0x92);
            }
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKushala_State4_00(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x3D, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKushala_State4_01(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x49, 0, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 104.0f, 0U) != 0) {
            em->step++;
            Em_SetMotion(em, 0x3E, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKushala_State4_02(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 1;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x3F, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKushala_State4_03(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x49, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x42, 0, 0x18);
            em->timer = 2;
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            if (--em->timer <= 0) {
                em->step++;
                Em_SetMotion(em, 0x43, 0, 0);
                break;
            }
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKushala_State4_04(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x40, 0, 0);
        if (!(em->velY <= -50.0f)) {
            em->velY = -50.0f;
        }
        em->accelX = 0.0f;
        em->accelY = -10.0f;
        em->accelZ = 0.0f;
        work->moveAngX = 0;
        work->moveAngZ = 0;
        break;
    case 1:
        work->moveAngY = em->rotY;
        Em_MoveRotatedAccel(em, &work->moveAngX);
        if (em->posY <= em->floorY) {
            em->step++;
            em->posY = em->floorY;
            em->stance = 0;
            Em_SetMotion(em, 0x41, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x42, 0, 0);
            em->timer = 2;
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            if (--em->timer <= 0) {
                em->step++;
                Em_SetMotion(em, 0x43, 0, 0);
                break;
            }
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    }
}

void EmKushala_State4_05(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_STATUS);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x45, 0, 0);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmKushala_ChangeState(em, 0, 7, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmKushala_ChangeState(em, 0, 7, 4);
        }
        break;
    }
    Em_SpawnStatus4Effect(em, 2);
}

void EmKushala_State4_06(Em *em)
{
    f32 turnF;
    u32 turn;

    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x4A, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x4B, 0, 0);
        }
        break;
    case 2:
        if (EmMotion_InFrameRange(em, 0, 0.0f, 104.0f) != 0) {
            turnF = 630.0f * em->motionSpeed;
            turn = (u32) turnF;
            em->rotY -= (s32) turn & 0xFFFF;
        }
        if (em->motion[0].playing == 0) {
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKushala_State4_07(Em *em)
{
    EmKushala_EndState(em, 0, 0);
}

void EmKushala_State4_08(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x40, 0, 0);
        if (!(em->velY <= -50.0f)) {
            em->velY = -50.0f;
        }
        em->accelX = 0.0f;
        em->accelY = -10.0f;
        em->accelZ = 0.0f;
        work->moveAngX = 0;
        work->moveAngZ = 0;
        break;
    case 1:
        work->moveAngY = em->rotY;
        Em_MoveRotatedAccel(em, &work->moveAngX);
        if (em->posY <= em->floorY) {
            em->step++;
            em->posY = em->floorY;
            em->stance = 0;
            Em_SetMotion(em, 0x41, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x42, 0, 0);
            em->timer = 2;
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            if (--em->timer <= 0) {
                em->step++;
                Em_SetMotion(em, 0x43, 0, 0);
                break;
            }
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    }
}

void EmKushala_State4_09(Em *em, EmWork *work)
{
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x40, 0, 0);
        Em_RestartScript(em);
        if (!(em->velY <= -50.0f)) {
            em->velY = -50.0f;
        }
        em->accelX = 0.0f;
        em->accelY = -10.0f;
        em->accelZ = 0.0f;
        work->moveAngX = 0;
        work->moveAngZ = 0;
        break;
    case 1:
        work->moveAngY = em->rotY;
        Em_MoveRotatedAccel(em, &work->moveAngX);
        if (em->posY <= em->floorY) {
            em->step++;
            em->posY = em->floorY;
            em->stance = 0;
            Em_SetMotion(em, 0x41, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x42, 0, 0);
            em->timer = 2;
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            if (--em->timer <= 0) {
                em->step++;
                Em_SetMotion(em, 0x43, 0, 0);
                break;
            }
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    }
}

void EmKushala_State4_0A(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x3D, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKushala_State4_0B(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x41, 0, 0);
        Player_ClearVelocity(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x42, 0, 0);
            em->timer = 2;
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            if (--em->timer <= 0) {
                em->step++;
                Em_SetMotion(em, 0x43, 0, 0);
                break;
            }
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    }
}

void EmKushala_State4_0C(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x49, 0, 0);
        StageObj_Stun_CreateEm(em, 1, 0x25, (s32) ((f32) em->timer + (em->motion[0].frameCount / 2.0f)), 150.0f, 3.0f);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x42, 0, 0x18);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x43, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_ChangeState(em, 0, 0x18, 4);
        }
        break;
    case 4:
        if (em->remote == 0) {
            EmKushala_ChangeState(em, 0, 0x18, 4);
        }
        break;
    }
}

void EmKushala_State4_0D(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x19, 0, 0x28);
        em->stance = 0;
        em->holdTimer = 0x32;
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            em->stance = 1;
            Em_SetMotion(em, 0x1B, 0, 0);
        }
        break;
    case 2:
        if (em->motion[1].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0xB, 0, 0x1C);
        }
        break;
    case 3:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKushala_State4_0E(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x19, 0, 0x28);
        em->holdTimer = 0x32;
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x1A, 0, 0);
        }
        break;
    case 2:
        if (em->motion[1].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x1F, 0, 0);
        }
        break;
    case 3:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKushala_State6_00(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x49, 0, 0);
        work->flyGuard = 0;
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 104.0f, 0)) {
            em->step++;
            Em_SetMotion(em, 0x3E, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0xA, 0, 0);
            EmKushala_StartCurves(em, 0);
        }
        break;
    case 3:
        if (EmMotion_PassedFrameScaled(em, 48.0f, 0)) {
            em->step++;
            em->stance = 2;
            EmKushala_UpdateCurves(em);
        }
        break;
    case 4:
        EmKushala_UpdateCurves(em);
        if (em->motion[0].playing == 0) {
            em->step++;
            work->turnSpeed = 0x100;
            Em_SetMotion(em, 0xB, 0, 0);
            work->flyGuard = 0;
            Player_ClearVelocity(em);
            em->velY = 10.0f;
            Em_SetCombat(em, 0, 0);
        }
        break;
    case 5:
        Em_MoveRotated(em, &em->rotX);
        if (!(em->posY <= 500.0f + em->floorY) && em->velY < 30.0f) {
            em->velY += 0.75f;
        }
        if (!(em->posY < 20000.0f)) {
            em->step++;
            Player_ClearVelocity(em);
            em->velZ = 10.0f;
        }
        break;
    case 6:
        Em_MoveRotated(em, &em->rotX);
        break;
    }
    if (em->posY < em->floorY) {
        em->posY = em->floorY;
    }
}

void EmKushala_State6_01(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        work->turnSpeed = 0x100;
        Em_SetMotion(em, 0xB, 0, 0);
        work->flyGuard = 0;
        Player_ClearVelocity(em);
        em->velY = 10.0f;
        Em_SetCombat(em, 0, 0);
        break;
    case 1:
        Em_MoveRotated(em, &em->rotX);
        if (!(em->posY <= 500.0f + em->floorY) && em->velY < 30.0f) {
            em->velY += 0.75f;
        }
        if (!(em->posY < 20000.0f)) {
            em->step++;
            Player_ClearVelocity(em);
            em->velZ = 10.0f;
        }
        break;
    case 2:
        Em_MoveRotated(em, &em->rotX);
        break;
    }
    if (em->posY < em->floorY) {
        em->posY = em->floorY;
    }
}

void EmKushala_State6_02(Em *em)
{
    f32 d;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        GmEm_ResetSize(em);
        em->lookAtTarget = 0;
        em->stance = 0;
        Em_SetMotion(em, 1, 0, 0);
        em->timer = 270;
        Player_ClearVelocity(em);
        em->posX = 9344.0f;
        em->posY = 305.0f;
        em->posZ = 9973.0f;
        em->rotY = 0xA400;
        em->goal[0] = 8061.0f;
        em->goal[1] = 314.0f;
        em->goal[2] = 8920.0f;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 2, 0, 0);
            em->timer = 1800;
        }
        break;
    case 2:
        d = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer <= 0 || d <= 100.0f) {
            em->step++;
            Em_SetMotion(em, 2, 0, 0);
            em->timer = 1800;
            em->posX = 12338.0f;
            em->posY = 291.0f;
            em->posZ = 11023.0f;
            em->rotY = 0xB000;
            em->goal[0] = 9100.0f;
            em->goal[1] = 310.0f;
            em->goal[2] = 9683.0f;
        }
        break;
    case 3:
        d = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer <= 0 || d <= 100.0f) {
            em->step++;
            Em_SetMotion(em, 0x22, 0, 0);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x20, 0, 0);
            em->rotY = 0x3000;
        }
        break;
    case 5:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x1F, 0, 0);
        }
        break;
    case 6:
        if (em->motion[0].playing == 0 && Save_TestEventFlag(0x9E) == 1) {
            em->step++;
            em->senseDelay = 0;
            em->noHitTimer = 0;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKushala_State6_03(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        GmEm_ResetSize(em);
        em->lookAtTarget = 0;
        em->stance = 2;
        Em_SetMotion(em, 0xB, 0, 0);
        em->timer = 300;
        work->flyGuard = 0;
        Player_ClearVelocity(em);
        em->velY = 10.0f;
        em->posX = 9947.0f;
        em->posY = -3000.0f;
        em->posZ = 8326.0f;
        em->rotY = 0x8000;
        em->goal[0] = 9947.0f;
        em->goal[1] = 0.0f;
        em->goal[2] = 8326.0f;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
        }
        break;
    case 2:
        Em_MoveRotated(em, &em->rotX);
        if (!(em->posY < em->goal[1])) {
            em->step++;
            em->timer = 170;
            em->posX = 9749.0f;
            em->posY = 0.0f;
            em->posZ = 8665.0f;
            em->rotY = 0x8000;
        }
        break;
    case 3:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x11, 0, 0);
            em->posX = 9855.0f;
            em->posY = 0.0f;
            em->posZ = 4996.0f;
            em->rotY = 0x8000;
        }
        break;
    case 4:
        if (EmMotion_ReachedFrame(em, 0, 146.0f)) {
            em->step++;
            Em_SetMotion(em, 0x3C, 0, 0x64);
        }
        break;
    case 5:
        if (EmMotion_ReachedFrame(em, 0, 170.0f)) {
            em->step++;
            Em_SetMotion(em, 0x3C, 0, 0x64);
        }
        break;
    case 6:
        if (EmMotion_ReachedFrame(em, 0, 200.0f)) {
            em->step++;
            Em_SetMotion(em, 0x1F, 0, 0);
        }
        break;
    case 7:
        if (em->motion[0].playing == 0 && Save_TestEventFlag(0x9F) == 1) {
            em->step++;
            em->senseDelay = 0;
            em->noHitTimer = 0;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKushala_State6_04_Captured(Em *em)
{
    f32 pos[3];

    Em_SetCaptured(em, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x61, 0, 0);
        Em_LoadReactionSet(em, 1);
        Em_BeginStatus1(em);
        Em_ClearPoison(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            QuestEm_Captured(em);
        }
        break;
    case 2:
        break;
    }
    pos[1] = 10.0f;
    pos[2] = 140.0f;
    pos[0] = 0.0f;
    Em_SpawnLyingPuffs(em, 0x22, pos, 1.6f);
}

void EmKushala_State6_05(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        GmEm_ResetSize(em);
        em->lookAtTarget = 0;
        em->stance = 2;
        Em_SetMotion(em, 0xB, 0, 0);
        em->timer = 200;
        work->flyGuard = 0;
        Player_ClearVelocity(em);
        em->velY = -10.0f;
        em->posX = 9329.0f;
        em->posY = 3000.0f;
        em->posZ = 9973.0f;
        em->rotY = 0xFC00;
        em->goal[0] = 9329.0f;
        em->goal[1] = 100.0f;
        em->goal[2] = 9973.0f;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
        }
        break;
    case 2:
        Em_MoveRotated(em, &em->rotX);
        if (em->posY <= em->goal[1]) {
            em->step++;
            em->timer = 170;
            em->posX = 9329.0f;
            em->posY = 100.0f;
            em->posZ = 9973.0f;
            em->rotY = 0xFC00;
        }
        break;
    case 3:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotionRow(em, 0x11, 0, 0);
            em->posX = 9329.0f;
            em->posY = 0.0f;
            em->posZ = 9973.0f;
            em->rotY = 0xFC00;
        }
        break;
    case 4:
        if (EmMotion_ReachedFrame(em, 0, 146.0f)) {
            em->step++;
            Em_SetMotionRow(em, 0x3C, 0, 0);
            em->stance = 0;
        }
        break;
    case 5:
        if (EmMotion_PassedFrameScaled(em, 36.0f, 0)) {
            EmKushala_SpawnEffect02AtBone27(em);
        }
        if (EmMotion_ReachedFrame(em, 0, 200.0f)) {
            em->step++;
            Em_SetMotionRow(em, 0x2F, 0, 0);
        }
        break;
    case 6:
        if (EmMotion_PassedFrameScaled(em, 104.0f, 0)) {
            EmEffectAttack_CreateChain(em, 0xF, 0, 0, 0);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x30, 0, 0);
        }
        break;
    case 7:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x1F, 0, 0);
        }
        break;
    case 8:
        if (Save_TestEventFlag(0xA3) == 1) {
            em->step++;
            em->senseDelay = 0;
            em->noHitTimer = 0;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKushala_State6_06(Em *em, EmWork *work)
{
    f32 d;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        GmEm_ResetSize(em);
        em->lookAtTarget = 0;
        em->stance = 2;
        Em_SetMotion(em, 0xC, 0, 0);
        work->flyGuard = 0;
        Player_ClearVelocity(em);
        em->velZ = 80.0f;
        em->posX = 7146.0f;
        em->posY = 2000.0f;
        em->posZ = 11034.0f;
        em->rotY = 0x5800;
        em->goal[0] = 9740.0f;
        em->goal[1] = 2000.0f;
        em->goal[2] = 9360.0f;
        em->timer = 180;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            em->timer = 300;
        }
        break;
    case 2:
        Em_MoveRotated(em, &em->rotX);
        d = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer <= 0 || d <= 100.0f) {
            em->step++;
            em->posX = 9740.0f;
            em->posY = 2000.0f;
            em->posZ = 9360.0f;
            em->rotY = 0x3000;
            em->goal[0] = 13806.0f;
            em->goal[1] = 2000.0f;
            em->goal[2] = 11045.0f;
            em->timer = 300;
        }
        break;
    case 3:
        Em_MoveRotated(em, &em->rotX);
        d = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer <= 0 || d <= 100.0f) {
            em->step++;
            Em_SetMotion(em, 0xB, 0, 0);
            em->posX = 13917.0f;
            em->posY = 2000.0f;
            em->posZ = 10375.0f;
            em->rotY = 0xB000;
            Player_ClearVelocity(em);
            em->velY = -10.0f;
        }
        break;
    case 4:
        Em_MoveRotated(em, &em->rotX);
        if (em->posY <= 500.0f) {
            em->step++;
            Em_SetMotion(em, 0xB, 0, 0);
            em->posX = 13917.0f;
            em->posY = 2000.0f;
            em->posZ = 10375.0f;
            em->rotY = 0xB000;
            Player_ClearVelocity(em);
            em->velY = -10.0f;
        }
        break;
    case 5:
        Em_MoveRotated(em, &em->rotX);
        if (em->posY <= 500.0f) {
            em->step++;
            Em_SetMotion(em, 0xD, 0, 0);
            EmKushala_StartCurves(em, 1);
            em->posX = 13690.0f;
            em->posY = 200.0f;
            em->posZ = 10845.0f;
            em->rotY = 0xB000;
        }
        break;
    case 6:
        EmKushala_UpdateCurves(em);
        if (em->motion[0].playing == 0) {
            em->step++;
            em->velX = 0.0f;
            em->velY = 0;
            Player_ClearAccel2(em);
            em->velZ = 80.0f;
            Em_SetMotion(em, 0xC, 0, 0);
            em->rotY = 0xA400;
            em->goal[0] = 8488.0f;
            em->goal[1] = 200.0f;
            em->goal[2] = 9339.0f;
            em->rotY = Math_AngleXZ(&em->posX, em->goal);
            em->timer = 300;
        }
        break;
    case 7:
        Em_MoveRotated(em, &em->rotX);
        d = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer <= 0 || d <= 100.0f) {
            em->step++;
            Em_SetMotion(em, 0xE, 0, 0);
            EmKushala_StartCurves(em, 2);
        }
        break;
    case 8:
        EmKushala_UpdateCurves(em);
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0xB, 0, 0);
        }
        break;
    case 9:
        if (Save_TestEventFlag(0x9D) == 1) {
            em->senseDelay = 0;
            em->noHitTimer = 0;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKushala_State6_07(Em *em)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        GmEm_ResetSize(em);
        em->lookAtTarget = 0;
        em->stance = 0;
        Em_SetMotion(em, 0x26, 0, 0);
        em->timer = 180;
        Player_ClearVelocity(em);
        em->posX = 11600.0f;
        em->posY = -20.0f;
        em->posZ = 13000.0f;
        em->rotY = 0x6800;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            em->timer = 2;
        }
        break;
    case 2:
        if (em->motion[0].playing == 0 && --em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x27, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x3B, 0, 0);
            em->timer = 2;
            em->posX = 10018.0f;
            em->posY = 0.0f;
            em->posZ = 11470.0f;
            em->rotY = 0xB000;
        }
        break;
    case 4:
        if (EmMotion_PassedFrameScaled(em, 56.0f, 0)) {
            StageObj_K27_Create(em);
        }
        if (em->motion[0].playing == 0 && --em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x3C, 0, 0x64);
        }
        break;
    case 5:
        if (EmMotion_ReachedFrame(em, 0, 230.0f)) {
            em->step++;
        }
        break;
    case 6:
        if (Save_TestEventFlag(0xA0) == 1) {
            em->step++;
            em->senseDelay = 0;
            em->noHitTimer = 0;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKushala_State5_00_Hunted(Em *em, EmWork *work)
{
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x44, 0, 0);
        em->lookAtTarget = 0;
        em->stance = 3;
        ((EmKushalaWork *)work)->windOn = 0;
        QuestEm_Hunted(em);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 414.0f, 0)) {
            CamShake_ReqAIf(em, 2);
        }
        if (em->motion[0].playing == 0) {
            em->timer = 0x708;
            em->step++;
            EmMine_Create(em, 0);
            em->fieldCollide = 0;
        }
        break;
    case 2:
        EmMine_Update(em);
        if (QuestEm_CanAppear(em) && --em->timer <= 0) {
            em->step++;
            em->timer = 150;
            EmMine_Remove(em);
        }
        break;
    case 3:
        if (em->timer > 0) {
            em->timer--;
        }
        if (em->timer <= 0) {
            if (em->remote == 0) {
                EmKushala_ChangeState(em, 5, 3, 4);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    }
}

void EmKushala_State5_02_HuntedFalling(Em *em, EmWork *work)
{
    s32 t;

    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x40, 0, 0);
        ((EmKushalaWork *)work)->windOn = 0;
        if (!(em->velY <= -50.0f)) {
            em->velY = -50.0f;
        }
        em->accelY = -10.0f;
        work->moveAngX = 0;
        work->moveAngZ = 0;
        break;
    case 1:
        work->moveAngY = em->rotY;
        Em_MoveRotatedAccel(em, &work->moveAngX);
        if (em->posY <= em->floorY) {
            em->step++;
            em->posY = em->floorY;
            Em_SetMotion(em, 0x41, 0, 0);
            Player_ClearVelocity(em);
            em->stance = 3;
            QuestEm_Hunted(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->timer = 1800;
            em->step++;
            EmMine_Create(em, 0);
            em->fieldCollide = 0;
        }
        break;
    case 4:
        EmMine_Update(em);
        if (QuestEm_CanAppear(em) != 0) {
            if (--em->timer <= 0) {
                em->step++;
                EmMine_Remove(em);
                em->timer = 150;
            }
        }
        break;
    case 5:
        if (em->timer > 0) {
            em->timer--;
        }
        t = em->timer;
        if (t <= 0) {
            if (em->remote == 0) {
                EmKushala_ChangeState(em, 5, 3, 4);
            }
        } else {
            em->alpha = (f32) t / 150.0f;
        }
        break;
    case 0x63:
        if (em->motion[0].playing == 0) {
            em->stance = 0;
            EmKushala_EndState(em, 0, 0);
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
        }
        break;
    }
}

void EmKushala_State5_03(Em *em)
{
    switch (em->step) {
    case 0:
        if (QuestEm_CanAppear(em) == 1) {
            EmKushala_ChangeState(em, 0, 1, 0);
            break;
        }
        em->taskPhase++;
    case 1:
        break;
    }
}

void EmKushala_RunGroup0(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmKushala_State0_00(em);
        break;
    case 1:
        EmKushala_State0_01(em);
        break;
    case 2:
        EmKushala_State0_02(em);
        break;
    case 3:
        EmKushala_State0_03(em);
        break;
    case 4:
        EmKushala_State0_04(em, work, 0);
        break;
    case 5:
        EmKushala_State0_05(em, work, 0);
        break;
    case 6:
        EmKushala_State0_06(em);
        break;
    case 7:
        EmKushala_State0_07(em, work, 0);
        break;
    case 8:
        EmKushala_State0_08(em);
        break;
    case 9:
        EmKushala_State0_09(em);
        break;
    case 0xA:
        EmKushala_State0_0A(em);
        break;
    case 0xB:
        EmKushala_State0_0B(em);
        break;
    case 0xC:
        EmKushala_State0_0C(em);
        break;
    case 0xD:
        EmKushala_State0_0D(em);
        break;
    case 0xE:
        EmKushala_State0_0E(em);
        break;
    case 0xF:
        EmKushala_State0_0F(em, work);
        break;
    case 0x10:
        EmKushala_State0_0F(em, work);
        break;
    case 0x11:
        EmKushala_State0_0F(em, work);
        break;
    case 0x12:
        EmKushala_State0_12(em);
        break;
    case 0x13:
        EmKushala_State0_13(em);
        break;
    case 0x14:
        EmKushala_State0_0F(em, work);
        break;
    case 0x15:
        EmKushala_State0_15(em);
        break;
    case 0x16:
        EmKushala_State0_16(em);
        break;
    case 0x17:
        EmKushala_State0_17(em);
        break;
    case 0x18:
        EmKushala_State0_07(em, work, 1);
        break;
    case 0x19:
        EmKushala_State0_04(em, work, 1);
        break;
    case 0x1A:
        EmKushala_State0_05(em, work, 1);
        break;
    case 0x1B:
        EmKushala_State0_1B(em, work);
        break;
    }
}

void EmKushala_RunGroup1(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmKushala_State1_00(em);
        break;
    case 1:
        EmKushala_State1_01(em);
        break;
    case 2:
        EmKushala_State1_02(em);
        break;
    case 3:
        EmKushala_State1_03(em, work);
        break;
    case 4:
        EmKushala_State1_04(em);
        break;
    case 5:
        EmKushala_State1_05(em, work, 0);
        break;
    case 6:
        EmKushala_State1_06(em);
        break;
    case 7:
        EmKushala_State1_07(em);
        break;
    case 8:
        EmKushala_State1_05(em, work, 0);
        break;
    case 9:
        EmKushala_State1_05(em, work, 1);
        break;
    case 0xA:
        EmKushala_State1_05(em, work, 2);
        break;
    case 0xB:
        EmKushala_State1_05(em, work, 3);
        break;
    case 0xC:
        EmKushala_State1_05(em, work, 4);
        break;
    case 0x14:
        EmKushala_State1_14(em);
        break;
    case 0x15:
        EmKushala_State1_15(em, work, 0);
        break;
    case 0x16:
        EmKushala_State1_15(em, work, 1);
        break;
    case 0x17:
        EmKushala_State1_17(em, work, 0);
        break;
    case 0x18:
        EmKushala_State1_18(em, work);
        break;
    case 0x19:
        EmKushala_State1_19(em, work);
        break;
    case 0x1A:
        EmKushala_State1_1A(em, work, 0);
        break;
    case 0x1B:
        EmKushala_State1_1B(em);
        break;
    case 0x1C:
        EmKushala_State1_17(em, work, 0);
        break;
    case 0x1D:
        EmKushala_State1_17(em, work, 1);
        break;
    case 0x1E:
        EmKushala_State1_1A(em, work, 1);
        break;
    }
}

void EmKushala_RunGroup2(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmKushala_State2_00(em, work);
        break;
    case 1:
        EmKushala_State2_01(em, work, 0);
        break;
    case 2:
        EmKushala_State2_02(em, work);
        break;
    case 3:
        EmKushala_State2_03(em, work);
        break;
    case 4:
        EmKushala_State2_04(em, work);
        break;
    case 5:
        EmKushala_State2_05(em, work);
        break;
    case 6:
        EmKushala_State2_06(em, work);
        break;
    case 7:
        EmKushala_State2_07(em, work);
        break;
    case 8:
        EmKushala_State2_08(em, work);
        break;
    case 9:
        EmKushala_State2_09(em, work);
        break;
    case 10:
        EmKushala_State2_0A(em, work);
        break;
    case 11:
        EmKushala_State2_0B(em, work);
        break;
    case 12:
        EmKushala_State2_0C(em, work);
        break;
    case 13:
        EmKushala_State2_0D(em, work);
        break;
    case 14:
        EmKushala_State2_01(em, work, 1);
        break;
    case 15:
        EmKushala_State2_0F(em, work, 0);
        break;
    case 16:
        EmKushala_State2_0F(em, work, 1);
        break;
    case 17:
        EmKushala_State2_0F(em, work, 2);
        
    default:
        break;
    }
}

void EmKushala_RunGroup3(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmKushala_State3_00(em);
        break;
    case 1:
        EmKushala_State3_01(em);
        break;
    case 2:
        EmKushala_State3_02(em);
        break;
    case 3:
        EmKushala_State3_03(em);
        break;
    case 4:
        EmKushala_State3_04(em);
        break;
    case 5:
        EmKushala_State3_05(em, work, 0);
        break;
    case 6:
        EmKushala_State3_06(em);
        break;
    case 7:
        EmKushala_State3_05(em, work, 1);
        break;
    case 8:
        EmKushala_State3_08(em);
        break;
    case 9:
        EmKushala_State3_09(em);
        break;
    case 0xA:
        EmKushala_State3_0A(em, work, 0);
        break;
    case 0xB:
        EmKushala_State3_0B(em, work);
        break;
    case 0xC:
        EmKushala_State3_0C(em);
        break;
    case 0xD:
        EmKushala_State3_0D(em, work, 0);
        break;
    case 0xE:
        EmKushala_State3_0D(em, work, 1);
        break;
    case 0xF:
        EmKushala_State3_0F(em, work, 0);
        break;
    case 0x10:
        EmKushala_State3_0F(em, work, 1);
        break;
    case 0x11:
        EmKushala_State3_0F(em, work, 2);
        break;
    case 0x12:
        EmKushala_State3_0F(em, work, 16);
        break;
    case 0x13:
        EmKushala_State3_0F(em, work, 17);
        break;
    case 0x14:
        EmKushala_State3_0F(em, work, 18);
        break;
    case 0x15:
        EmKushala_State3_0A(em, work, 1);
        break;
    case 0x16:
        EmKushala_State3_0A(em, work, 2);
        break;
    case 0x17:
        EmKushala_State3_0F(em, work, 19);
        break;
    case 0x18:
        EmKushala_State3_18(em, work);
        break;
    case 0x19:
        EmKushala_State3_0F(em, work, 20);
        break;
    case 0x1A:
        EmKushala_State3_05(em, work, 2);
        break;
    case 0x1B:
        EmKushala_State3_05(em, work, 3);
        break;
    }
}

void EmKushala_RunGroup4(Em *em, EmWork *work)
{
    switch (em->state) {
    default:
        EmKushala_State4_00(em);
        break;
    case 0:
        EmKushala_State4_00(em);
        break;
    case 1:
        EmKushala_State4_01(em);
        break;
    case 2:
        EmKushala_State4_02(em);
        break;
    case 3:
        EmKushala_State4_03(em);
        break;
    case 4:
        EmKushala_State4_04(em, work);
        break;
    case 5:
        EmKushala_State4_05(em);
        break;
    case 6:
        EmKushala_State4_06(em);
        break;
    case 7:
        EmKushala_State4_07(em);
        break;
    case 8:
        EmKushala_State4_08(em, work);
        break;
    case 9:
        EmKushala_State4_09(em, work);
        break;
    case 10:
        EmKushala_State4_0A(em);
        break;
    case 11:
        EmKushala_State4_0B(em);
        break;
    case 12:
        EmKushala_State4_0C(em);
        break;
    case 13:
        EmKushala_State4_0D(em);
        break;
    case 14:
        EmKushala_State4_0E(em);
        break;
    case 15:
        EmKushala_State4_00(em);
        break;
    }
}

void EmKushala_RunGroup5(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmKushala_State5_00_Hunted(em, work);
        break;
    case 1:
        EmKushala_State5_00_Hunted(em, work);
        break;
    case 2:
        EmKushala_State5_02_HuntedFalling(em, work);
        break;
    case 3:
        EmKushala_State5_03(em);
        break;
    }
}

void EmKushala_RunGroup6(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmKushala_State6_00(em, work);
        break;
    case 1:
        EmKushala_State6_01(em, work);
        break;
    case 2:
        EmKushala_State6_02(em);
        break;
    case 3:
        EmKushala_State6_03(em, work);
        break;
    case 4:
        EmKushala_State6_04_Captured(em);
        break;
    case 5:
        EmKushala_State6_05(em, work);
        break;
    case 6:
        EmKushala_State6_06(em, work);
        break;
    case 7:
        EmKushala_State6_07(em);
        
    default:
        break;
    }
}

void EmKushala_Update(Em *em)
{
    u8 buf[4];
    f32 ratio;
    EmKushalaWork *w = (EmKushalaWork *)&em->work;
    s32 ev;
    u16 k;

    Em_UpdateCombatTimer(em);
    Em_RaisePlayerAreaTriggerLax(em);
    if (em->type == 0x36 || em->type == 0x3C) {
        ratio = 0.12f;
        em->hitzoneSet = 0;
        em->windTimer = 0;
        if (EmKushala_WindActive(em)) {
            em->hitzoneSet = 1;
            em->windTimer = 2;
        }
    } else {
        if (em->type == 0x40) {
            ratio = 0.15f;
        } else {
            ratio = 0.2f;
        }
        em->hitzoneSet = 0;
        em->windTimer = 0;
        if (EmKushala_WindActive2(em)) {
            em->hitzoneSet = 1;
            em->windTimer = 2;
        }
    }
    k = Map_DayId(gScene.map);
    if (em->noAlert != 1 && em->rage == 0 && k != 8 && k != 9 && k != 0xA) {
        if (em->type == 0x36 || em->type == 0x3C) {
            switch (k) {
            case MAP_JUNGLE:
                Em_CheckLowHp(em, ratio);
                Em_DecayNeed2(em);
                Em_DecayNeed1(em);
                Em_DecayNeed3(em);
                break;
            case MAP_DESERT:
                Em_DecayNeed2(em);
                Em_CheckLowHp(em, ratio);
                break;
            case MAP_SNOWY_MOUNTAINS:
                Em_CheckLowHp(em, ratio);
                break;
            }
        } else {
            switch (k) {
            case MAP_DESERT:
                Em_CheckLowHp(em, ratio);
                break;
            case MAP_VOLCANO:
                Em_CheckLowHp(em, ratio);
                Em_DecayNeed1(em);
                break;
            case MAP_SWAMP:
                Em_CheckLowHp(em, ratio);
                break;
            }
        }
    }
    ev = Em_UpdateDamage(em, buf) & 0xFF;
    switch (ev) {
    case 0:
    case 9:
    case 14:
    case 15:
    case 16:
        break;
    case 1:
    case 2:
        switch (gQuest.stage) {
        case 0x138A:
        case 0x138B:
        case 0x138D:
        case 0x138E:
            if (em->stateGroup != 6 && em->stateGroup != 5) {
                em->scriptPending = 0;
                if (em->stance == 2) {
                    EmKushala_ChangeState(em, 6, 1, 3);
                } else {
                    if (gScene.area == em->area) {
                        CamReq_ForMonster(em);
                    }
                    EmKushala_ChangeState(em, 6, 0, 3);
                }
            }
            break;
        default:
            if (em->stance == 2) {
                EmKushala_ChangeState(em, 5, 2, 3);
            } else {
                EmKushala_ChangeState(em, 5, 0, 3);
            }
            break;
        }
        break;
    case 3:
        if (em->stance == 2) {
            EmKushala_ChangeState(em, 4, 8, 3);
        } else if (em->stance == 1) {
            EmKushala_ChangeState(em, 4, 0xB, 3);
        } else {
            EmKushala_ChangeState(em, 4, 3, 3);
        }
        break;
    case 4:
    case 18:
        if (em->stance == 0) {
            if (em->type == 0x36 || em->type == 0x3C) {
                EmKushala_ChangeState(em, 4, 0xD, 3);
            } else {
                EmKushala_ChangeState(em, 4, 0xE, 3);
            }
        }
        break;
    case 5:
        if ((em->stateGroup != 4 || em->state != 4) && (em->stateGroup != 4 || em->state != 8)) {
            EmKushala_ChangeState(em, 4, 4, 3);
        }
        break;
    case 6:
        if ((em->stateGroup != 4 || em->state != 5) && (em->stateGroup != 4 || em->state != 4) &&
            (em->stateGroup != 4 || em->state != 8)) {
            Em_SetStatus4Timer(em);
            EmKushala_ChangeState(em, 4, 5, 3);
        }
        break;
    case 7:
        if ((em->stateGroup != 0 || em->state != 0x15) && (em->stateGroup != 4 || em->state != 4) &&
            (em->stateGroup != 4 || em->state != 8)) {
            Em_SetStatus1Timer(em);
            EmKushala_ChangeState(em, 0, 0x15, 3);
        }
        break;
    case 8:
        if ((em->stateGroup != 0 || em->state != 0xB) && (em->stateGroup != 4 || em->state != 4) &&
            (em->stateGroup != 4 || em->state != 8)) {
            Em_SetStatus2Timer(em);
            EmKushala_ChangeState(em, 0, 0xB, 3);
        }
        break;
    case 10:
        switch (em->state) {
        case 9:
            EmKushala_ChangeState(em, 0, 0xA, 3);
            em->scriptPending = 0;
            Em_CheckRageByHp(em, em->rageThreshold);
            break;
        case 0xB:
            EmKushala_ChangeState(em, 0, 0xC, 3);
            em->scriptPending = 0;
            break;
        case 0x15:
            EmKushala_ChangeState(em, 0, 0x16, 3);
            em->scriptPending = 0;
            break;
        }
        break;
    case 11:
        if (em->type == 0x36 || em->type == 0x3C) {
            EmKushala_ChangeState(em, 4, 6, 3);
        } else if (em->partBreakCount > 0) {
            EmKushala_ChangeState(em, 4, 6, 3);
        } else {
            EmKushala_ChangeState(em, 4, 0xF, 3);
        }
        break;
    case 12:
        if (em->stance == 2) {
            if (em->flinchPart == 0) {
                EmKushala_ChangeState(em, 4, 8, 3);
            } else {
                EmKushala_ChangeState(em, 4, 4, 3);
            }
        } else if (em->stance == 1) {
            if (em->flinchPart == 0) {
                EmKushala_ChangeState(em, 4, 0xB, 3);
            } else {
                EmKushala_ChangeState(em, 4, 2, 3);
            }
        } else {
            EmKushala_BreakParts(em, 1);
            switch (em->flinchPart) {
            case 0:
                EmKushala_ChangeState(em, 4, 3, 3);
                break;
            case 5:
            case 1:
            case 2:
                EmKushala_ChangeState(em, 4, 0, 3);
                break;
            default:
                if (em->parts[em->flinchPart].state >= 2) {
                    EmKushala_ChangeState(em, 4, 1, 3);
                } else {
                    EmKushala_ChangeState(em, 4, 0, 3);
                }
                break;
            }
        }
        break;
    case 13:
        if (em->stance != 2) {
            EmKushala_ChangeState(em, 4, 0, 3);
            em->scriptPending = 0;
        }
        break;
    case 17:
        if ((em->stateGroup != 4 || em->state != 0xC) && (em->stateGroup != 4 || em->state != 4) &&
            (em->stateGroup != 4 || em->state != 8)) {
            Em_SetStatus5Timer(em);
            EmKushala_ChangeState(em, 4, 0xC, 3);
        }
        break;
    }
    if (Em_DamageResultIsReaction(em, ev) == 1) {
        EmKushala_BreakParts(em, 0);
    }
    switch (gQuest.stage) {
    case 0x430:
    case 0x431:
        if ((em->type == 0x40 || em->type == 0x41) && Area_DayId(em->area) == AREA_VOLCANO_7 && Save_TestEventFlag(0xA3) == 0) {
            if (Area_DayId(gScene.area) == AREA_VOLCANO_7 && gScene.eventRunning == 1 && em->stateGroup != 6) {
                EmKushala_ChangeState(em, 6, 5, 1);
            }
            break;
        }
        goto normal;
    case 0x138A:
        if ((em->type == 0x36 || em->type == 0x3C) && Area_DayId(em->area) == AREA_JUNGLE_3 && Save_TestEventFlag(0x9D) == 0) {
            if (Area_DayId(gScene.area) == AREA_JUNGLE_3 && gScene.eventRunning == 1 && em->stateGroup != 6) {
                EmKushala_ChangeState(em, 6, 6, 1);
            }
            break;
        }
        goto normal;
    case 0x138C:
        if ((em->type == 0x36 || em->type == 0x3C) && Area_DayId(em->area) == AREA_SNOWY_MOUNTAINS_6 && Save_TestEventFlag(0x9E) == 0) {
            if (Area_DayId(gScene.area) == AREA_SNOWY_MOUNTAINS_6 && gScene.eventRunning == 1 && em->stateGroup != 6) {
                EmKushala_ChangeState(em, 6, 2, 1);
            }
            break;
        }
        goto normal;
    case 0x138D:
        if ((em->type == 0x40 || em->type == 0x41) && Area_DayId(em->area) == AREA_VOLCANO_8 && Save_TestEventFlag(0x9F) == 0) {
            if (Area_DayId(gScene.area) == AREA_VOLCANO_8 && gScene.eventRunning == 1 && em->stateGroup != 6) {
                EmKushala_ChangeState(em, 6, 3, 1);
            }
            break;
        }
        goto normal;
    case 0x138F:
        if ((em->type == 0x40 || em->type == 0x41) && Area_DayId(em->area) == AREA_TOWER_0x7C && Save_TestEventFlag(0xA0) == 0) {
            if (Area_DayId(gScene.area) == AREA_TOWER_0x7C && gScene.eventRunning == 1 && em->stateGroup != 6) {
                EmKushala_ChangeState(em, 6, 7, 1);
            }
            break;
        }
    default:
    normal:
        if (Quest_IsRunning(0) == 1 && em->stateGroup != 6 && em->stateGroup != 5) {
            em->scriptPending = 0;
            if (em->stance == 2) {
                EmKushala_ChangeState(em, 6, 1, 0);
            } else {
                if (gScene.area == em->area) {
                    CamReq_ForMonster(em);
                }
                EmKushala_ChangeState(em, 6, 0, 0);
            }
        }
        Em_UpdatePendingScript(em);
        break;
    }
    Em_ClearHitReactMode(em);
    EmKushala_RunState(em, (s32) w);
    if (em->stateRequest != 0) {
        EmKushala_RunState(em, (s32) w);
        em->stateRequest = 0;
    }
    if (em->stateGroup == 2 && (s8) w->flyGuard == 1) {
        em->guardTimer = 2;
    }
}

void EmKushala_RunState(Em *em, s32 work)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->stateGroup) {
    case 0:
        EmKushala_RunGroup0(em, (EmWork *) work);
        break;
    case 1:
        EmKushala_RunGroup1(em, (EmWork *) work);
        break;
    case 2:
        EmKushala_RunGroup2(em, (EmWork *) work);
        break;
    case 3:
        EmKushala_RunGroup3(em, (EmWork *) work);
        break;
    case 4:
        EmKushala_RunGroup4(em, (EmWork *) work);
        break;
    case 5:
        EmKushala_RunGroup5(em, (EmWork *) work);
        break;
    case 6:
        EmKushala_RunGroup6(em, (EmWork *) work);
        break;
    case 7:
        EmKushala_RunGroup6(em, (EmWork *) work);
        
    default:
        break;
    }
}

s32 EmKushala_GroundEffectStateInWind(Em *em)
{
    switch (em->stateGroup) {
    case 0:
        switch (em->state) {
        case 2:
        case 3:
        case 0x13:
            return 1;
        }
        break;
    case 1:
        switch (em->state) {
        case 3:
        case 5:
        case 8:
        case 0x17:
        case 0x19:
        case 0x1B:
        case 0x1C:
        case 0x1D:
            return 1;
        }
        break;
    case 3:
        switch (em->state) {
        case 2:
        case 0xF:
        case 0x10:
        case 0x11:
            return 1;
        }
        break;
    }
    return 0;
}

void EmKushala_SpawnGroundEffect(Em *em, s32 a1, s32 a2, f32 scale)
{
    f32 pos[3];

    if (Player_InCurrentArea(em)) {
        Em_BonePos(em, a1, pos);
        pos[1] = em->floorY;
        Effect0D_SpawnOnGround(pos, a2, scale * em->scaleX);
    }
}

void EmKushala_SpawnGroundEffectNearGround(Em *em, s32 arg1, s32 arg2, s32 flag, f32 scale)
{
    if (EmKushala_WindActive(em) == 1) {
        if (EmKushala_GroundEffectStateInWind(em) != 1) {
            goto block_3;
        }
    } else {
block_3:
        if (em->posY <= (1000.0f + em->floorY)) {
            EmKushala_SpawnGroundEffect(em, arg1, arg2, scale);
            if (flag != 0) {
                EmImpactAttack_CreateAt(em, (f32 *) &em->posX, 1U);
            }
        }
    }
}

void EmKushala_SpawnK14NearGround(Em *em, s32 k, s32 c, s32 flag, f32 scale)
{
    if (EmKushala_WindActive(em) == 1) {
        if (EmKushala_GroundEffectStateInWind(em) != 1) {
            goto block_3;
        }
    } else {
block_3:
        if (em->posY <= (1000.0f + em->floorY)) {
            StageObj_K14_Create((Em *) em, k, c, scale);
            if (flag != 0) {
                EmImpactAttack_CreateAt(em, (f32 *) &em->posX, 1U);
            }
        }
    }
}

static void EmKushala_MotionEffectsDefault(Em *em)
{
}

void EmKushala_SpawnK1CInMotions(Em *em, s16 motion)
{
    switch (motion) {
    case 0x3E9:
    case 0x3EA:
    case 0x3EB:
    case 0x3ED:
    case 0x3EE:
    case 0x3EF:
    case 0x3F3:
    case 0x3F7:
    case 0x400:
    case 0x402:
    case 0x406:
    case 0x407:
    case 0x40A:
    case 0x40D:
    case 0x413:
    case 0x42E:
    case 0x433:
        if (em->rage != 0 && gScene.frame % 45 == 0) {
            StageObj_K1C_CreateOnEm(em, 0xD, 2.0f * em->scaleX);
        }
        break;
    }
}

void Explosion_CreateOwned(Player *owner, f32 *pos, s32 kind);
void EmKushala_MotionEffects(Em *em, EmWork *work)
{
    f32 v3[3];
    f32 v2[3];
    f32 v1[3];
    f32 v0[3];

    if (em->motionId[0] != work->fxMotion) {
        work->fxMotion = em->motionId[0];
    }
    if ((em->type == 0x36) || (em->type == 0x3C)) {
        EmKushala_MotionSounds(em, work);
    } else {
        EmKushala_MotionSoundsTesca(em, work);
    }
    EmKushala_SpawnK1CInMotions(em, (s32) work->fxMotion);
    switch (work->fxMotion) {
    case 0x3F0:
        if (EmMotion_PassedFrameScaled(em, 8.0f, 0) != 0) {
            EmKushala_SpawnGroundEffect(em, 0x1F, 3, 8.0f * em->scaleX);
        }
        if (EmMotion_PassedFrameScaled(em, 25.0f, 0) != 0) {
            EmKushala_SpawnGroundEffect(em, 0x19, 3, 8.0f * em->scaleX);
            break;
        }
        break;
    case 0x3F1:
        if (EmMotion_PassedFrameScaled(em, 8.0f, 0) != 0) {
            EmKushala_SpawnGroundEffect(em, 0x19, 3, 8.0f * em->scaleX);
        }
        if (EmMotion_PassedFrameScaled(em, 25.0f, 0) != 0) {
            EmKushala_SpawnGroundEffect(em, 0x1F, 3, 8.0f * em->scaleX);
            break;
        }
        break;
    case 0x3F2:
        if (EmMotion_PassedFrameScaled(em, 46.0f, 0) != 0) {
            EmKushala_SpawnK14NearGround(em, 1, 0, 0, 1.5f * em->scaleX);
            break;
        }
        break;
    case 0x3F3:
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            EmKushala_SpawnK14NearGround(em, 1, 0, 0, 1.5f * em->scaleX);
            break;
        }
        break;
    case 0x3F5:
        if (EmMotion_PassedFrameScaled(em, 60.0f, 0) != 0) {
            EmKushala_SpawnK14NearGround(em, 1, 0, 0, 1.5f * em->scaleX);
            break;
        }
        break;
    case 0x3F6:
        if ((EmMotion_PassedFrameScaled(em, 26.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 76.0f, 0) != 0)) {
            EmKushala_SpawnK14NearGround(em, 9, 0, 0, em->scaleX);
            break;
        }
        break;
    case 0x3F9:
        if (EmMotion_PassedFrameScaled(em, 8.0f, 0) != 0) {
            EmKushala_SpawnGroundEffectNearGround(em, 1, 7, 0, 10.0f * em->scaleX);
        }
        if (EmMotion_PassedFrameScaled(em, 66.0f, 0) != 0) {
            EmKushala_SpawnGroundEffectNearGround(em, 1, 0xC, 0, 12.0f * em->scaleX);
        }
        if (EmMotion_PassedFrameScaled(em, 70.0f, 0) != 0) {
            EmImpactAttack_CreateAt(em, (f32 *) &em->posX, 2);
            break;
        }
        break;
    case 0x3FA:
        if (EmMotion_PassedFrameScaled(em, 8.0f, 0) != 0) {
            EmKushala_SpawnK14NearGround(em, 1, 0, 0, em->scaleX);
            break;
        }
        break;
    case 0x3FB:
        if (EmMotion_PassedFrameScaled(em, 4.0f, 0) != 0) {
            EmKushala_SpawnK14NearGround(em, 1, 0, 0, em->scaleX);
            break;
        }
        break;
    case 0x3FC:
        if (EmMotion_PassedFrameScaled(em, 4.0f, 0) != 0) {
            EmKushala_SpawnGroundEffectNearGround(em, 1, 7, 0, 10.0f * em->scaleX);
            break;
        }
        break;
    case 0x3FD:
        if (EmMotion_PassedFrameScaled(em, 18.0f, 0) != 0) {
            EmKushala_SpawnGroundEffect(em, 8, 3, 8.0f * em->scaleX);
        }
        if (EmMotion_PassedFrameScaled(em, 36.0f, 0) != 0) {
            EmKushala_SpawnGroundEffect(em, 0xD, 3, 8.0f * em->scaleX);
            break;
        }
        break;
    case 0x3FE:
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            StageObj_K14_Create((Em *) em, 2, 0x1A, 1.5f * em->scaleX);
            break;
        }
        break;
    case 0x3FF:
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            StageObj_K14_Create((Em *) em, 2, 0x1A, 1.5f * em->scaleX);
            break;
        }
        break;
    case 0x401:
        if (EmMotion_PassedFrameScaled(em, 46.0f, 0) != 0) {
            StageObj_K14_Create((Em *) em, 2, 0x1B, 2.0f * em->scaleX);
            break;
        }
        break;
    case 0x402:
        if (EmMotion_PassedFrameScaled(em, 16.0f, 0) != 0) {
            EmKushala_CreateAttack(em, 0xB);
        }
        if (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) {
            EmKushala_SpawnGroundEffect(em, 1, 7, 10.0f * em->scaleX);
            break;
        }
        break;
    case 0x403:
        if (EmMotion_PassedFrameScaled(em, 24.0f, 0) != 0) {
            StageObj_K14_Create((Em *) em, 1, 0, em->scaleX);
            break;
        }
        break;
    case 0x408:
        if ((em->type == 0x36) || (em->type == 0x3C)) {
            if (EmMotion_PassedFrameScaled(em, 62.0f, 0) != 0) {
                EmKushala_CreateAttack(em, 0x1C);
                StageObj_K0F_CreateOnEm(em, 5, 3, 1.0f);
            }
            if ((EmKushala_WindActive(em) == 1) && (EmMotion_PassedFrameScaled(em, 186.0f, 0) != 0)) {
                StageObj_K1C_CreateOnEm(em, 0x10, em->scaleX);
                EmImpactAttack_CreateAt(em, (f32 *) &em->posX, 4);
                break;
            }
        } else {
            if (EmMotion_PassedFrameScaled(em, 62.0f, 0) != 0) {
                EmKushala_CreateAttack(em, 0x1C);
                StageObj_K0F_CreateOnEm(em, 5, 3, 1.0f);
            }
            if ((EmKushala_WindActive2(em) == 1) && (EmMotion_PassedFrameScaled(em, 186.0f, 0) != 0)) {
                StageObj_K1C_CreateOnEm(em, 0x10, em->scaleX);
                EmImpactAttack_CreateAt(em, (f32 *) &em->posX, 4);
                break;
            }
        }
        break;
    case 0x409:
        if (((em->type == 0x40) || (em->type == 0x41)) && (EmMotion_PassedFrameScaled(em, 16.0f, 0) != 0)) {
            EmKushala_SpawnGroundEffect(em, 1, 7, 10.0f * em->scaleX);
            break;
        }
        break;
    case 0x40B:
        break;
    case 0x40E:
        if (EmMotion_ReachedFrame(em, 0, 220.0f) != 0) {
            v1[0] = 30.0f;
            v1[1] = 30.0f;
            v1[2] = 200.0f;
            Em_SpawnLyingPuffs(em, 0x25, v1, 1.6f);
            break;
        }
        break;
    case 0x410:
        if (((em->type == 0x40) || (em->type == 0x41)) && (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0)) {
            EmKushala_SpawnK14NearGround(em, 1, 0, 0, em->scaleX);
            break;
        }
        break;
    case 0x411:
        if ((em->type == 0x40) || (em->type == 0x41)) {
            if (EmMotion_PassedFrameScaled(em, 20.0f, 0) != 0) {
                EmKushala_SpawnK14NearGround(em, 1, 0x1D, 0, em->scaleX);
            }
            if (EmMotion_PassedFrameScaled(em, 24.0f, 0) != 0) {
                EmKushala_SpawnK14NearGround(em, 1, 0x19, 0, em->scaleX);
            }
            if (EmMotion_PassedFrameScaled(em, 18.0f, 0) != 0) {
                EmKushala_CreateAttack(em, 0x1D);
                break;
            }
        }
        break;
    case 0x412:
        if ((em->type == 0x40) || (em->type == 0x41)) {
            if (EmMotion_PassedFrameScaled(em, 40.0f, 0) != 0) {
                StageObj_K14_Create((Em *) em, 2, 0x1E, 1.5f * em->scaleX);
            }
            if (EmMotion_PassedFrameScaled(em, 32.0f, 0) != 0) {
                EmKushala_CreateAttack(em, 0x1E);
                break;
            }
        }
        break;
    case 0x413:
        if ((em->type == 0x40) || (em->type == 0x41)) {
            if (EmMotion_PassedFrameScaled(em, 122.0f, 0) != 0) {
                EmKushala_CreateAttack(em, 5);
            }
            if (EmMotion_PassedFrameScaled(em, 124.0f, 0) != 0) {
                StageObj_K14_Create((Em *) em, 2, 0x20, 2.0f * em->scaleX);
            }
            if (EmMotion_PassedFrameScaled(em, 162.0f, 0) != 0) {
                EmKushala_SpawnGroundEffect(em, 0x12, 0xC, 8.0f * em->scaleX);
                break;
            }
        }
        break;
    case 0x414:
        if (EmMotion_PassedFrameScaled(em, 32.0f, 0) != 0) {
            EmKushala_CreateAttack(em, 5);
            break;
        }
        break;
    case 0x415:
        if (EmMotion_PassedFrameScaled(em, 32.0f, 0) != 0) {
            EmKushala_CreateAttack(em, 6);
            break;
        }
        break;
    case 0x416:
        if (EmMotion_PassedFrameScaled(em, 60.0f, 0) != 0) {
            EmKushala_CreateAttack(em, 0xC);
            break;
        }
        break;
    case 0x417:
        if (EmMotion_PassedFrameScaled(em, 90.0f, 0) != 0) {
            EmKushala_CreateAttack(em, 0x11);
            break;
        }
        break;
    case 0x419:
        if ((EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 60.0f, 0) != 0)) {
            EmKushala_SpawnK14NearGround(em, 1, 0, 0, em->scaleX);
            break;
        }
        break;
    case 0x41A:
        if ((em->type == 0x36) || (em->type == 0x3C)) {
            if (EmMotion_PassedFrameScaled(em, 24.0f, 0) != 0) {
                u8 area = gScene.map;

                if (area == AREA_FORTRESS_4 || area == AREA_FOREST_HILLS_0) {
                    EmRangedAttack_CreateAtBone2(em, 0x25, 0x12, 1, 0x2000, 0, (s32) (131.0f * em->motionSpeed));
                } else {
                    EmRangedAttack_CreateAtBone2(em, 0x25, 0x12, 0, 0x2000, 0, (s32) (131.0f * em->motionSpeed));
                }
            }
        } else if (EmMotion_PassedFrameScaled(em, 24.0f, 0) != 0) {
            EmEffectAttack_CreateChain(em, 0xF, 0, 0x2000, 0);
            break;
        }
        break;
    case 0x41B:
        break;
    case 0x41C:
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            EmKushala_SpawnGroundEffectNearGround(em, 1, 7, 0, 10.0f * em->scaleX);
        }
        if ((EmMotion_PassedFrameScaled(em, 52.0f, 0) != 0) && Player_InCurrentArea(em) && (em->posY <= (1000.0f + em->floorY))) {
            v2[0] = 0.0f;
            v2[1] = 0.0f;
            v2[2] = 400.0f * em->scaleZ;
            Vec_RotateY(v2, (6.2831855f * (f32) em->rotY) / 65536.0f);
            v3[0] = em->posX + v2[0];
            v3[1] = em->floorY;
            v3[2] = em->posZ + v2[2];
            StageObj_K1C_CreateAtYaw(v3, (u16) em->rotY, 0x11, 1.2f);
            Explosion_CreateOwned((Player *)em, v3, 0x1E);
            break;
        }
        break;
    case 0x41D:
        if (EmMotion_PassedFrameScaled(em, 50.0f, 0) != 0) {
            EmKushala_CreateAttack(em, 2);
        }
        if (EmMotion_PassedFrameScaled(em, 56.0f, 0) != 0) {
            EmKushala_SpawnGroundEffect(em, 1, 0xC, 8.0f * em->scaleX);
            break;
        }
        break;
    case 0x41E:
        if (EmMotion_PassedFrameScaled(em, 42.0f, 0) != 0) {
            EmKushala_CreateAttack(em, 0x12);
        }
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            EmKushala_SpawnK14NearGround(em, 1, 0, 0, em->scaleX);
        }
        if (EmMotion_PassedFrameScaled(em, 64.0f, 0) != 0) {
            EmKushala_SpawnGroundEffectNearGround(em, 1, 7, 0, 10.0f * em->scaleX);
            break;
        }
        break;
    case 0x41F:
        if (EmMotion_PassedFrameScaled(em, 48.0f, 0) != 0) {
            EmKushala_CreateAttack(em, 3);
        }
        if (EmMotion_PassedFrameScaled(em, 54.0f, 0) != 0) {
            EmKushala_CreateAttack(em, 0xD);
        }
        if (EmMotion_PassedFrameScaled(em, 60.0f, 0) != 0) {
            EmKushala_SpawnGroundEffect(em, 8, 4, 6.0f * em->scaleX);
        }
        if (EmMotion_PassedFrameScaled(em, 58.0f, 0) != 0) {
            StageObj_K14_Create((Em *) em, 2, 0x1E, 1.5f * em->scaleX);
            break;
        }
        break;
    case 0x420:
        if (EmMotion_PassedFrameScaled(em, 48.0f, 0) != 0) {
            EmKushala_CreateAttack(em, 4);
        }
        if (EmMotion_PassedFrameScaled(em, 54.0f, 0) != 0) {
            EmKushala_CreateAttack(em, 0xF);
        }
        if (EmMotion_PassedFrameScaled(em, 60.0f, 0) != 0) {
            EmKushala_SpawnGroundEffect(em, 0xD, 4, 6.0f * em->scaleX);
        }
        if (EmMotion_PassedFrameScaled(em, 70.0f, 0) != 0) {
            StageObj_K14_Create((Em *) em, 3, 0x1C, 1.5f * em->scaleX);
            break;
        }
        break;
    case 0x421:
        if (EmMotion_PassedFrameScaled(em, 50.0f, 0) != 0) {
            EmKushala_CreateAttack(em, 0x13);
        }
        if (EmMotion_PassedFrameScaled(em, 52.0f, 0) != 0) {
            EmKushala_CreateAttack(em, 0x14);
        }
        if (EmMotion_PassedFrameScaled(em, 82.0f, 0) != 0) {
            EmKushala_CreateAttack(em, 0x15);
        }
        if (EmMotion_PassedFrameScaled(em, 6.0f, 0) != 0) {
            EmKushala_SpawnGroundEffect(em, 1, 7, 10.0f * em->scaleX);
        }
        if (EmMotion_PassedFrameScaled(em, 48.0f, 0) != 0) {
            StageObj_K14_Create((Em *) em, 3, 0x1F, 1.5f * em->scaleX);
            break;
        }
        break;
    case 0x422:
        if (EmMotion_PassedFrameScaled(em, 50.0f, 0) != 0) {
            EmKushala_CreateAttack(em, 0x16);
        }
        if (EmMotion_PassedFrameScaled(em, 52.0f, 0) != 0) {
            EmKushala_CreateAttack(em, 0x17);
        }
        if (EmMotion_PassedFrameScaled(em, 82.0f, 0) != 0) {
            EmKushala_CreateAttack(em, 0x18);
        }
        if (EmMotion_PassedFrameScaled(em, 6.0f, 0) != 0) {
            EmKushala_SpawnGroundEffectNearGround(em, 1, 7, 0, 10.0f * em->scaleX);
        }
        if (EmMotion_PassedFrameScaled(em, 60.0f, 0) != 0) {
            StageObj_K14_Create((Em *) em, 2, 0x1F, 1.5f * em->scaleX);
            break;
        }
        break;
    case 0x426:
        if (EmMotion_PassedFrameScaled(em, 20.0f, 0) != 0) {
            EmKushala_SpawnGroundEffect(em, 1, 7, 8.0f * em->scaleX);
            break;
        }
        break;
    case 0x427:
        if ((EmMotion_PassedFrameScaled(em, 22.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 70.0f, 0) != 0)) {
            EmKushala_SpawnK14NearGround(em, 1, 0, 0, em->scaleX);
            break;
        }
        break;
    case 0x429:
        if (EmMotion_PassedFrameScaled(em, 16.0f, 0) != 0) {
            EmKushala_SpawnK14NearGround(em, 0xB, 0, 0, em->scaleX);
            break;
        }
        break;
    case 0x42A:
        if (EmMotion_PassedFrameScaled(em, 60.0f, 0) != 0) {
            EmKushala_SpawnGroundEffect(em, 1, 0xC, 8.0f * em->scaleX);
            break;
        }
        break;
    case 0x42C:
        if (EmMotion_PassedFrameScaled(em, 420.0f, 0) != 0) {
            StageObj_K14_Create((Em *) em, 0, 0, em->scaleX);
            break;
        }
        break;
    case 0x42F:
        if (EmMotion_PassedFrameScaled(em, 190.0f, 0) != 0) {
            StageObj_K14_Create((Em *) em, 0, 0, em->scaleX);
        }
        if (EmMotion_ReachedFrame(em, 0, 228.0f) != 0) {
            v0[0] = -30.0f;
            v0[1] = 30.0f;
            v0[2] = 200.0f;
            Em_SpawnLyingPuffs(em, 0x25, v0, 1.6f);
            break;
        }
        break;
    case 0x431:
        if (EmMotion_PassedFrameScaled(em, 134.0f, 0) != 0) {
            StageObj_K14_Create((Em *) em, 0, 0, em->scaleX);
            break;
        }
        break;
    case 0x432:
        if (EmMotion_PassedFrameScaled(em, 60.0f, 0) != 0) {
            StageObj_K14_Create((Em *) em, 0, 0, em->scaleX);
            break;
        }
        break;
    default:
        EmKushala_MotionEffectsDefault(em);
        break;
    }
}

void EmKushala_UpdateMotionEffects(Em *em)
{
    EmKushalaWork *w = (EmKushalaWork *)&em->work;

    switch (w->fxStep) {
    case 0:
        w->fxStep++;
        break;
    case 1:
        EmKushala_MotionEffects(em, (EmWork *) w);
        break;
    }
}

void EmKushala_CreateAttack(Em *em, s32 kind)
{
    u8 type;

    type = em->type;
    if ((type == 0x36) || (type == 0x3C)) {
        KushalaAttack_Create(em, kind);
        return;
    }
    TeostraAttack_Create(em, kind);
}

void EmKushala_SpawnEffect02AtBone27(Em *em)
{
    f32 pos[3];
    f32 ofs[3];
    void *bone;

    if ((s32)Player_InCurrentArea(em) & 0xFF) {
        bone = Em_Bone(em, 0x27);
        Mtx_GetTrans(pos, bone);
        ofs[0] = 0.0f;
        ofs[1] = -25.0f;
        ofs[2] = 60.0f;
        Vec_RotateSelf(ofs, bone);
        pos[0] = pos[0] + ofs[0];
        pos[1] = pos[1] + ofs[1];
        pos[2] = pos[2] + ofs[2];
        Effect02_SpawnAt(pos, 0, em->rotY);
        StageObj_K10_CreateAtScaled(pos, 6, 1.0f);
    }
}

void EmKushala_TableNop(void)
{
}

s32 EmKushala_AllowHit8(Em *em)
{
    if (em->stance != 2 && em->stance != 1) {
        if (em->type == 0x36 || em->type == 0x3C) {
            if (em->hp <= (s16) (0.3f * em->hpMax) && em->partBreakCount == 0) {
                return 1;
            }
        } else {
            if (em->hp <= (s16) (0.4f * em->hpMax) && em->partBreakCount <= 1) {
                return 1;
            }
        }
    }
    return 0;
}

/* Kind 0x38's own meanings of the work area's bytes (EmWork, include/em.h, has the shared ones). */
typedef struct EmKind38Work {
    u8 fxStep;                  /* 0x000: UpdateMotionEffects: 0, then 1 */
    u8 unk001[0x006 - 0x001];
    s16 stayTimer;              /* 0x006: Start: 1800 when spawned with spawnParam bit 0, else -1 (none); Update counts it down, and at 0 a state of groups 0 to 2 changes to group 5 */
    f32 bobPos;                 /* 0x008: the hover's bob: offset (InitHover: 15 to 24, its sign by the height above the floor), speed, step (SteerHover, ApplyHover) */
    f32 bobVel;                 /* 0x00C */
    f32 bobAccel;               /* 0x010 */
    f32 bobGainGoal;            /* 0x014: the gain ApplyHover eases bobGain to while stance is 2 (the states: 1.0, 0.6 or 0) */
    f32 bobGain;                /* 0x018: ApplyHover: posY += bobVel * bobGain */
    f32 baseScale;              /* 0x01C: Init: bodyScale; State2_0A grows the scale to it, State3_02 pulses the scale around it */
    u8 unk020[0x038 - 0x020];
    s16 seTimer;                /* 0x038: UpdateLoopSe: frames of the looped sound (played at 0, wraps at 10 or 11) */
    u8 seKind;                  /* 0x03A: UpdateLoopSe: the looped sound (1, 2; else none); Init: 0 */
    u8 unk03B;                  /* 0x03B: State3_00: 1 during its step, then 0 (no C reads) */
    u8 unk03C;                  /* 0x03C: State3_00, State3_02: 1, then 0 (RunState clears it; func_0083BA70 returns it) */
    u8 unk03D[0x03E - 0x03D];
    s16 scaleFrames;            /* 0x03E: State2_0A: the timer's first value (the scale grows over it) */
    s16 pulseAng;               /* 0x040: State3_02: an angle stepped by 0x777, the scale's sine */
    u8 unk042[0x100 - 0x042];
} EmKind38Work;
typedef char EmKind38Work_size_check[(sizeof(EmKind38Work) == sizeof(EmWork)) ? 1 : -1];

void EmKind38_UpdateLoopSe(Em *em, EmWork *work)
{
    EmKind38Work *w = (EmKind38Work *)work;

    switch (w->seKind) {
    case 1:
        if (w->seTimer == 0) {
            Em_PlaySeAtBoneVol(em, 0, 2, 3);
        }
        (w->seTimer)++;
        if (w->seTimer >= 10) {
            w->seTimer = 0;
        }
        break;
    case 2:
        if (w->seTimer == 0) {
            Em_PlaySeAtBoneVol(em, 1, 2, 3);
        }
        (w->seTimer)++;
        if (w->seTimer >= 11) {
            w->seTimer = 0;
        }
        break;
    default:
        w->seTimer = 0;
        break;
    }
}

void EmKind38_Init(Em *em)
{
    EmKind38Work *w = (EmKind38Work *)&em->work;
    s16 hp;

    if (gQuest.stage == 0 && !(em->spawnParam & 1)) {
        if (gScene.area == AREA_SNOWY_MOUNTAINS_NIGHT_9) {
            em->posX = 9500.0f + 150.0f * (em->slotNo / 8);
            em->posY = 0.0f;
            em->posZ = 9200.0f + 150.0f * (em->slotNo & 7);
        } else {
            em->posX = 150.0f * em->slotNo + gStartPos[gScene.area * 3];
            em->posY = D_43F0A4[gScene.area * 3];
            em->posZ = D_43F0A8[gScene.area * 3];
        }
    }
    Em_SetMotionRow(em, 1, 0, 0);
    Player_ClearVelocity(em);
    hp = Em_ComputeMaxHp(em, 0);
    em->hp = hp;
    em->hpMax = hp;
    w->baseScale = em->bodyScale;
    em->scriptPending = 1;
    em->blind = 0;
    w->unk03B = 0;
    w->unk03C = 0;
    w->seKind = 0;
    w->seTimer = 0;
    Em_InitParts(em);
    EmKind38_InitHover(em, (EmWork *) w);
    em->remote = 0;
}

void EmKind38_Start(Em *em)
{
    EmKind38Work *w = (EmKind38Work *)&em->work;

    EmKind38_Init(em);
    if (em->spawnParam & 1) {
        em->stance = 2;
        w->stayTimer = 0x708;
        em->scriptPending = 0;
        EmKind38_ChangeState(em, 2, 0xA, 0);
        return;
    }
    em->stance = 0;
    w->stayTimer = -1;
    EmKind38_ChangeState(em, 0, 1, 0);
}

void EmKind38_Reinit(Em *em)
{
    EmKind38_Init(em);
}

void EmKind38_EndState(Em *em)
{
    ((EmKind38Work *)&em->work)->unk03B = 0;
    em->scriptPending = 1;
    em->stance = 0;
    EmKind38_ChangeState(em, 0, 1, 0);
}

void EmKind38_EndState2(Em *em)
{
    ((EmKind38Work *)&em->work)->unk03B = 0;
    em->scriptPending = 1;
    em->stance = 2;
    EmKind38_ChangeState(em, 2, 3, 0);
}

void EmKind38_State0_00(void)
{
}

void EmKind38_State0_01(Em *em)
{
    switch (em->step) {
    case 0:
        em->stance = 0;
        em->step++;
        em->timer = ((em->stateRand & 3) * 0xF) + 0x1E;
        break;
    case 1:
        if (--em->timer <= 0) {
            EmKind38_EndState(em);
        }
        break;
    }
}

void EmKind38_RunGroup0(Em *em)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->state) {
    case 0:
        EmKind38_State0_00();
        break;
    case 1:
        EmKind38_State0_01(em);
        break;
    }
}

void EmKind38_State1_00(void)
{
}

void EmKind38_RunGroup1(Em *em)
{
    switch (em->state) {
    case 0:
        EmKind38_State1_00();
        break;
    }
}

void EmKind38_State2_00(Em *em, s16 variant)
{
    EmKind38Work *w = (EmKind38Work *)&em->work;
    f32 lim;

    w->seKind = 1;
    switch (em->step) {
    case 0:
        em->stance = 2;
        em->step++;
        w->bobGainGoal = 0.6f;
        em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        if (variant == 0) {
            em->accelZ = 0.345f;
            Em_SetSpeedScaled(em, 0, 300.0f, 200.0f);
            if (!(em->goalDist <= 800.0f)) {
                em->goalDist = 800.0f;
            }
        } else {
            em->accelZ = 0.555f;
            Em_SetSpeedScaled(em, 0, 300.0f, 100.0f);
            if (!(em->goalDist <= 1200.0f)) {
                em->goalDist = 1200.0f;
            }
        }
        break;
    case 1:
        EmKind38_KeepHeight(em, 0);
        if (variant == 0) {
            lim = 8.0f;
        } else {
            lim = 10.0f;
        }
        if (!(em->velZ <= lim)) {
            em->velZ = lim;
        }
        if (Em_StepToGoal(em, 0x20, 2) == 1) {
            em->accelZ = 0.0625f * -em->velZ;
            em->accelY = 0.0f;
            em->step++;
        }
        break;
    case 2:
        em->velY *= 0.7f;
        if (em->velZ - em->accelZ <= 0.0f) {
            w->bobGainGoal = 1.0f;
            Player_ClearVelocity(em);
            EmKind38_EndState2(em);
            return;
        }
        break;
    }
    EmKind38_MoveByVelocity(em);
}

void EmKind38_State2_01(Em *em)
{
    EmKind38Work *w = (EmKind38Work *)&em->work;

    w->seKind = 1;
    switch (em->step) {
    case 0:
        em->stance = 2;
        em->step++;
        w->bobGainGoal = 0.6f;
        em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        em->goalDist = -800.0f;
        em->accelZ = -0.421f;
        break;
    case 1:
        if (em->velZ < -6.4f) {
            em->velZ = -6.4f;
        }
        if (!((em->goalDist -= em->velZ) < 0.0f)) {
            em->accelZ = 0.0625f * -em->velZ;
            em->step++;
        }
        break;
    case 2:
        if (!(em->velZ - em->accelZ < 0.0f)) {
            w->bobGainGoal = 1.0f;
            Player_ClearVelocity(em);
            EmKind38_EndState2(em);
            return;
        }
        break;
    }
    EmKind38_MoveByVelocity(em);
}

void EmKind38_State2_03(Em *em)
{
    ((EmKind38Work *)&em->work)->seKind = 1;
    switch (em->step) {
    case 0:
        em->stance = 2;
        em->step++;
        em->timer = ((em->stateRand & 3) * 0x1E) + 0x1E;
        break;
    case 1:
        em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        if (--em->timer <= 0) {
            em->step++;
            EmKind38_EndState2(em);
        }
        break;
    }
}

void EmKind38_State2_04(Em *em)
{
    em->unk472 = 1;
    switch (em->step) {
    case 0:
        em->step++;
        if (em->stance == 0) {
            Player_ClearVelocity(em);
            em->timer = 10;
            break;
        }
        em->timer = 0;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            em->stance = 2;
            em->timer = (em->stateRand & 3) * 8 + 0x1E;
            em->accelY = 0.777f;
            em->accelZ = 0.55f;
        }
        break;
    case 2:
        EmKind38_ClampVelocity(em, EmKind38_VelLimits);
        if (--em->timer > 0) {
            break;
        }
        em->step++;
        em->accelZ = 0.0f;
        em->accelY = 0.0f;
    case 3:
        em->velY *= 0.876f;
        em->velZ *= 0.876f;
        if (Math_Absf(em->velY) < 0.05f) {
            em->step++;
            em->velZ = 0.0f;
            em->velY = 0.0f;
            EmKind38_EndState2(em);
            return;
        }
        break;
    }
    EmKind38_MoveByVelocity(em);
}

void EmKind38_State2_05(Em *em)
{
    em->unk472 = 1;
    switch (em->step) {
    case 0:
        if (em->stance == 0) {
            em->accelY = -0.5f;
        } else {
            em->step++;
            em->timer = 5;
        }
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            em->timer = (em->stateRand & 3) * 8 + 0x1E;
            em->accelY = -0.777f;
            em->accelZ = 0.55f;
        }
        break;
    case 2:
        EmKind38_ClampVelocity(em, EmKind38_VelLimits);
        if (--em->timer > 0) {
            break;
        }
        em->step++;
        em->accelZ = 0.0f;
        em->accelY = 0.0f;
    case 3:
        em->velY *= 0.876f;
        em->velZ *= 0.876f;
        if (Math_Absf(em->velY) < 0.05f) {
            em->step++;
            em->velZ = 0.0f;
            em->velY = 0.0f;
            EmKind38_EndState2(em);
            return;
        }
        break;
    }
    if ((s16) EmKind38_MoveByVelocity(em)) {
        em->stance = 0;
        em->accelY = 0.0f;
        em->velY = 0.0f;
        em->accelZ = 0.0f;
        em->velZ = 0.0f;
        EmKind38_EndState(em);
    }
}

void EmKind38_State2_06(Em *em, s32 dir)
{
    EmKind38Work *w = (EmKind38Work *)&em->work;
    s32 turn;

    w->seKind = 1;
    em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
    if (dir == 0) {
        turn = -0x3800;
    } else {
        turn = 0x3800;
    }
    em->rotY += turn;
    switch (em->step) {
    case 0:
        em->stance = 2;
        em->step++;
        w->bobGainGoal = 0.6f;
        em->timer = 0x5A;
        em->accelZ = 0.333f;
        break;
    case 1:
        EmKind38_KeepHeight(em, 0);
        if (em->velZ < -6.4f) {
            em->velZ = -6.4f;
        }
        if (--em->timer <= 0) {
            em->accelZ = 0.0625f * -em->velZ;
            em->accelY = 0.0f;
            em->step++;
        }
        break;
    case 2:
        em->velY *= 0.7f;
        if (em->velZ - em->accelZ <= 0.0f) {
            w->bobGainGoal = 1.0f;
            Player_ClearVelocity(em);
            EmKind38_EndState2(em);
            return;
        }
        break;
    }
    EmKind38_MoveByVelocity(em);
}

void EmKind38_State2_08(Em *em)
{
    em->unk472 = 1;
    switch (em->step) {
    case 0:
        em->stance = 2;
        em->step++;
        Player_ClearVelocity(em);
        em->stateVar0 = em->stateRand & 3;
        em->timer = 5;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            em->timer = (em->stateRand & 3) * 4 + 0xF;
            Vec_Copy(&em->accelX, &EmKind38_State2_08Accel[em->stateVar0].x);
        }
        break;
    case 2:
        EmKind38_ClampVelocity(em, D_008CA26C);
        if (--em->timer > 0) {
            break;
        }
        em->step++;
        em->accelZ = 0.0f;
        em->accelX = 0.0f;
    case 3:
        em->velX *= 0.876f;
        em->velZ *= 0.876f;
        if (Math_Absf(em->velZ) + Math_Absf(em->velZ) < 0.03f) {
            em->step++;
            em->velZ = 0.0f;
            em->velX = 0.0f;
            EmKind38_EndState2(em);
            return;
        }
        break;
    }
    EmKind38_MoveByVelocity(em);
}

void EmKind38_State2_09(void)
{
}

void EmKind38_State2_0A(Em *em)
{
    EmKind38Work *w = (EmKind38Work *)&em->work;
    f32 s;

    switch (em->step) {
    case 0:
        em->noReact = 1;
        em->stance = 2;
        em->step++;
        w->bobGainGoal = 0.0f;
        w->scaleFrames = em->timer = (em->stateRand & 3) * 3 + 0xF;
        em->rotX += (em->stateRand & 0xF00) + 0x600;
        em->rotY += ((em->stateRand >> 4) & 0xF00) - 0x780;
        Player_ClearVelocity(em);
        em->velZ = 50.0f + 7.0f * ((em->stateRand >> 2) & 3);
        Vec_RotateX(&em->velX, (6.2831855f * (f32) em->rotX) / 65536.0f);
        em->noPushTimer = 5;
        EmKind38_PlaySeAtBone2(em, 2);
        break;
    case 1:
        em->noPushTimer = 5;
        w->seKind = 1;
        s = w->baseScale * (w->scaleFrames - em->timer) / w->scaleFrames;
        em->scaleX = em->scaleY = em->scaleZ = s;
        if (--em->timer <= 0) {
            em->step++;
            Vec_Scale(&em->accelX, &em->velX, -0.025f);
            em->scaleX = em->scaleY = em->scaleZ = w->baseScale;
            w->bobGainGoal = 1.0f;
        }
        break;
    case 2:
        w->seKind = 1;
        if (em->velZ - em->accelZ <= 0.0f) {
            em->step++;
            em->noReact = 0;
            Player_ClearVelocity(em);
            EmKind38_EndState2(em);
            return;
        }
        break;
    }
    EmKind38_MoveByVelocity(em);
}

void EmKind38_RunGroup2(Em *em)
{
    switch (em->state) {
    case 0:
        EmKind38_State2_00(em, 0);
        break;
    case 1:
        EmKind38_State2_01(em);
        break;
    case 2:
        EmKind38_State2_00(em, 1);
        break;
    case 3:
        EmKind38_State2_03(em);
        break;
    case 4:
        EmKind38_State2_04(em);
        break;
    case 5:
        EmKind38_State2_05(em);
        break;
    case 6:
        EmKind38_State2_06(em, 0);
        break;
    case 7:
        EmKind38_State2_06(em, 1);
        break;
    case 8:
        EmKind38_State2_08(em);
        break;
    case 9:
        EmKind38_State2_09();
        break;
    case 10:
        EmKind38_State2_0A(em);
        
    default:
        break;
    }
}

void EmKind38_State3_00(Em *em, u8 k)
{
    EmKind38Work *w = (EmKind38Work *)&em->work;
    EmKPair *p;

    w->unk03C = 1;
    w->seKind = 2;
    switch (em->step) {
    case 0:
        em->stance = 2;
        em->step++;
        Player_ClearVelocity(em);
        em->timer = 10;
        w->bobGainGoal = 1.0f;
        break;
    case 1:
        EmKind38_KeepHeight(em, 1);
        if (--em->timer <= 0) {
            em->step++;
            w->unk03B = 1;
            EmKind38_CreateAttack(em);
            em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
            em->accelY = 0.0f;
            p = &EmKind38_State3Pts[em->variant][k];
            Em_SetSpeedScaled(em, 0, 500.0f, -400.0f);
            if (!(em->goalDist <= p->x)) {
                em->goalDist = p->x;
            }
            em->accelZ = p->y;
            w->bobGainGoal = 0.0f;
            em->accelY = 0.0f;
        }
        break;
    case 2:
        em->velY *= 0.6f;
        if ((em->goalDist -= em->velZ) < 0.0f) {
            w->unk03B = 0;
            w->unk03C = 0;
            em->accelZ = 0.025f * -em->velZ;
            em->step++;
        }
        break;
    case 3:
        w->unk03C = 0;
        if (em->velZ - em->accelZ <= 0.0f) {
            w->bobGainGoal = 1.0f;
            Player_ClearVelocity(em);
            EmKind38_EndState2(em);
            return;
        }
        break;
    }
    EmKind38_MoveByVelocity(em);
}

void * EmKind38_CreateAttack(Em *em)
{
    s8 kind;

    kind = 0;
    if ((em->stateRand & 0x1F) < (s32) EmKind38_Effect02Chance[em->variant]) {
        kind = 2;
    }
    return BulldromeAttack_Create(em, kind);
}

void EmKind38_State3_02(Em *em)
{
    EmKind38Work *w = (EmKind38Work *)&em->work;
    f32 pos[3];
    f32 s;

    w->unk03C = 1;
    w->seKind = 2;
    switch (em->step) {
    case 0:
        em->stance = 2;
        em->step++;
        Player_ClearVelocity(em);
        em->timer = 20;
        w->bobGainGoal = 1.0f;
        w->pulseAng = 0;
        break;
    case 1:
        EmKind38_KeepHeight(em, 1);
        if (--em->timer <= 0) {
            em->step++;
            em->accelZ = 0.4f;
            em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
            Em_SetSpeedScaled(em, 0, 500.0f, -50.0f);
            if (!(em->goalDist <= 1600.0f)) {
                em->goalDist = 1600.0f;
            }
            w->bobGainGoal = 0.0f;
        }
        break;
    case 2:
        EmKind38_KeepHeight(em, 1);
        if (!(em->velZ <= 9.6f)) {
            em->velZ = 9.6f;
        }
        if (Em_StepToGoal(em, 0x20, 2) == 1) {
            if ((s32)Player_InCurrentArea(em) & 0xFF) {
                Mtx_GetTrans(pos, em->model->bones[2].world);
                StageObj_K0E_CreateScaled(pos, 0, 1.0f);
                Explosion_CreateAt(pos, 0x22, em->area, 0);
            }
            EmKind38_ChangeState(em, 5, 1, 3);
            return;
        }
        break;
    }
    s = w->baseScale * (1.0f + 0.4f * Math_Sin(9.58738019e-05f * w->pulseAng));
    em->scaleX = em->scaleY = em->scaleZ = s;
    w->pulseAng += 0x777;
    EmKind38_MoveByVelocity(em);
}

void EmKind38_RunGroup3(Em *em)
{
    switch (em->state) {
    case 0:
        EmKind38_State3_00(em, 0U);
        break;
    case 1:
        EmKind38_State3_00(em, 1U);
        break;
    case 2:
        EmKind38_State3_02(em);
        break;
    }
}

void EmKind38_State4_00(Em *em)
{
    switch (em->step) {
    case 0:
        em->stance = 2;
        em->step++;
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmKind38_EndState2(em);
        }
        break;
    }
}

void EmKind38_State4_02(void)
{
}

void EmKind38_State4_03(Em *em)
{
    switch (em->step) {
    case 0:
        em->stance = 0;
        em->step++;
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmKind38_EndState(em);
        }
        break;
    }
}

void EmKind38_RunGroup4(Em *em)
{
    switch (em->state) {
    case 0:
        EmKind38_State4_00(em);
        break;
    case 1:
        EmKind38_State4_00(em);
        break;
    case 2:
        EmKind38_State4_02();
        break;
    case 3:
        EmKind38_State4_03(em);
        break;
    }
}

void EmKind38_State5_00_Hunted(Em *em, u8 variant)
{
    f32 pos[3];

    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        QuestEm_Hunted(em);
        em->step++;
        if (variant == 0) {
            em->stateVar0 = 0x3C;
        } else {
            em->stateVar0 = 0;
            em->alpha = 0.0f;
        }
        if (em->variant == 0) {
            Vec_Copy(pos, &em->posX);
            pos[1] = em->floorY;
            EmMine_Drop(em, pos);
        }
        break;
    case 1:
        em->hideShadowHold = 2;
        em->hideShadow = 2;
        if (em->stateVar0 != 0) {
            em->stateVar0--;
            if (em->stateVar0 == 0) {
                EmKind38_PlaySeAtBone2(em, 3);
            }
        } else if ((em->alpha -= 0.016666668f) <= 0.0f) {
            em->visible = 0;
            em->alpha = 0.0f;
            em->timer = 150;
            em->step++;
            em->painted = 0;
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            EmKind38_ChangeState(em, 5, 2, 4);
        }
        break;
    }
}

void EmKind38_State5_02(Em *em)
{
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        if (QuestEm_CanAppear(em) == 1) {
            em->posY += 300.0f;
            em->alpha = 0.0f;
            EmKind38_ChangeState(em, 7, 0, 0);
        } else {
            em->taskPhase++;
        }
        break;
    case 1:
        break;
    }
}

void EmKind38_RunGroup5(Em *em)
{
    em->noHitTimer = 0xA;
    em->noPushTimer = 0xA;
    switch (em->state) {
    case 0:
        EmKind38_State5_00_Hunted(em, 0U);
        break;
    case 1:
        EmKind38_State5_00_Hunted(em, 1U);
        break;
    case 2:
        EmKind38_State5_02(em);
        break;
    }
}

void EmKind38_State6_00(void)
{
}

void EmKind38_RunGroup6(Em *em)
{
    switch (em->state) {
    case 0:
        EmKind38_State6_00();
        break;
    }
}

void EmKind38_State7_00(Em *em)
{

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        Player_ClearVelocity(em);
        em->velY = -10.0f;
        em->timer = 0x1E;
        break;
    case 1:
        em->alpha += 0.033333335f;
        if (!(em->alpha <= 1.0f)) {
            em->alpha = 1.0f;
        }
        Player_Integrate(em);
        if (em->posY < em->floorY) {
            em->posY = em->floorY;
        } else if (!(em->posY <= (1000.0f + em->floorY))) {
            em->posY = em->posY - 20.0f;
        }
        if (--em->timer <= 0) {
            em->timer = 0;
            em->step++;
            em->senseDelay = 0;
            EmKind38_EndState2(em);
            em->alpha = 1.0f;
        }
        break;
    }
}

void EmKind38_RunGroup7(Em *em)
{
    em->noHitTimer = 0xA;
    em->noPushTimer = 0xA;
    em->senseDelay = 5;
    switch (em->state) {
    case 0:
        EmKind38_State7_00(em);
        break;
    }
}

void EmKind38_Update(Em *em)
{
    EmKind38Work *w = (EmKind38Work *)&em->work;
    u8 buf[4];

    em->motionSpeed = 1.0f;
    Em_UpdateCombatTimer(em);
    switch (Em_UpdateDamage(em, buf) & 0xFF) {
    case 0:
    case 3:
    case 4:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 13:
    case 15:
    case 16:
    case 18:
        break;
    case 1:
    case 2:
        if (em->stateGroup != 5) {
            EmKind38_ChangeState(em, 5, 0, 3);
            if (func_0083B8F0(em) == 1) {
                EmKind38_PlaySeAtBone2(em, 4);
            }
        }
        em->scriptPending = 0;
        break;
    case 5:
        if (func_0083B8F0(em) == 1) {
            EmKind38_ChangeState(em, 5, 0, 3);
            EmKind38_PlaySeAtBone2(em, 4);
        } else if (em->stateGroup != 4 || em->state != 1) {
            EmKind38_ChangeState(em, 4, 1, 3);
            EmKind38_PlaySeAtBone2(em, 5);
        }
        em->scriptPending = 0;
        break;
    case 12:
    case 14:
        if (func_0083B8F0(em) == 1) {
            EmKind38_ChangeState(em, 5, 0, 3);
            EmKind38_PlaySeAtBone2(em, 4);
        } else {
            if (em->stance == 2) {
                EmKind38_ChangeState(em, 4, 0, 3);
            } else {
                EmKind38_ChangeState(em, 4, 3, 3);
            }
            EmKind38_PlaySeAtBone2(em, 5);
        }
        em->scriptPending = 0;
        break;
    case 17:
        Em_EndStatus5(em);
        break;
    }
    if (w->stayTimer > 0) {
        (w->stayTimer)--;
    }
    if (w->stayTimer == 0) {
        switch (em->stateGroup) {
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
            break;
        case 0:
        case 1:
        case 2:
            EmKind38_ChangeState(em, 5, 0, 3);
            em->scriptPending = 0;
            break;
        }
    }
    Em_UpdatePendingScript(em);
    Em_ClearHitReactMode(em);
    EmKind38_RunState(em);
    if (em->stateRequest != 0) {
        EmKind38_RunState(em);
        em->stateRequest = 0;
    }
    EmKind38_SteerHover(em, (EmWork *) w);
    EmKind38_ApplyHover(em, (EmWork *) w);
}

void EmKind38_RunState(Em *em)
{
    ((EmKind38Work *)&em->work)->unk03C = 0;
    em->unk472 = 0;
    switch (em->stateGroup) {
    case 0:
        EmKind38_RunGroup0(em);
        break;
    case 1:
        EmKind38_RunGroup1(em);
        break;
    case 2:
        EmKind38_RunGroup2(em);
        break;
    case 3:
        EmKind38_RunGroup3(em);
        break;
    case 4:
        EmKind38_RunGroup4(em);
        break;
    case 5:
        EmKind38_RunGroup5(em);
        break;
    case 6:
        EmKind38_RunGroup6(em);
        break;
    case 7:
        EmKind38_RunGroup7(em);
        
    default:
        break;
    }
}

void EmKind38_MotionEffects(Em *em, EmWork *work)
{
    s16 *p = (s16 *) work;

    if (em->motionId[0] != p[0x21]) {
        p[0x21] = em->motionId[0];
    }
    EmKind38_UpdateLoopSe(em, work);
}

void EmKind38_UpdateMotionEffects(Em *em)
{
    EmKind38Work *w = (EmKind38Work *)&em->work;

    switch (w->fxStep) {
    case 0:
        w->fxStep++;
        break;
    case 1:
        EmKind38_MotionEffects(em, (EmWork *) w);
        break;
    }
}

void EmKind38_SpawnEffects(void *em)
{
    EmFxTri_Spawn(em, 2);
    Effect01_Spawn((Player *) em, 0);
}

void EmKind38_TableNop(void)
{
}

void EmKind38_ChangeState(Em *em, u16 group, u16 state, u8 mode)
{
    f32 f;

    em->unk473 = 0;
    f = ((EmKind38Work *)&em->work)->baseScale;
    em->scaleZ = f;
    em->scaleY = f;
    em->scaleX = f;
    switch (group) {
    case 2:
        switch (state) {
        case 0:
        case 1:
        case 2:
        case 6:
        case 7:
            Em_ComputeGoal2(em, em->goal);
            break;
        case 10:
            em->noReact = 1;
            em->scaleZ = 0.0f;
            em->scaleY = 0.0f;
            em->scaleX = 0.0f;
            break;
        }
        break;
    case 3:
        switch (state) {
        case 0:
        case 1:
        case 2:
            Em_ComputeGoal2(em, em->goal);
            break;
        }
        break;
    case 5:
        if (state == 2 && QuestEm_CanAppear(em) == 1) {
            QuestEm_Appear(em);
        }
        break;
    case 0:
    case 1:
    case 4:
    case 6:
        break;
    }
    Em_SetStateSync(em, group, state, mode);
}

void EmKind38_InitHover(Em *em, EmWork *work)
{
    u16 r;
    f32 s;
    f32 ground;
    f32 y;
    EmKind38Work *h = (EmKind38Work *)work;

    ground = em->floorY;
    r = em->stateRand;
    y = em->posY;
    s = -15 - ((r >> 8) & 3) * 3;
    if (!(y <= 300.0f + ground)) {
        s = -s;
    } else if (!(y <= 60.0f + ground) && (r & 0x8000)) {
        s = -s;
    }
    h->bobPos = s;
    h->bobAccel = 0.0f;
    h->bobVel = 0.0f;
    h->bobGainGoal = 1.0f;
    h->bobGain = 0.0f;
}

void EmKind38_SteerHover(Em *em, EmWork *work)
{
    EmKind38Work *h = (EmKind38Work *)work;
    f32 d;

    if (!(h->bobPos * h->bobAccel < 0.0f)) {
        if (!(h->bobVel <= 7.5f)) {
            h->bobVel = 7.5f;
        }
        if (h->bobVel < -7.5f) {
            h->bobVel = -7.5f;
        }
        h->bobAccel = 0.512f + 0.007f * ((em->stateRand >> 8) & 0x1F);
        d = h->bobVel * h->bobVel / (2.0f * (50.0f - Math_Absf(h->bobPos)));
        d *= 1.05f;
        if (h->bobAccel < d) {
            h->bobAccel = d;
        }
        if (!(h->bobPos + h->bobVel <= 0.0f)) {
            h->bobAccel = -h->bobAccel;
        }
    }
}

void EmKind38_ApplyHover(Em *em, EmWork *work)
{
    EmKind38Work *h = (EmKind38Work *)work;

    if (em->stance == 2) {
        h->bobGain = h->bobGain + 0.25f * (h->bobGainGoal - h->bobGain);
    } else {
        h->bobGain = 0.0f;
    }
    h->bobVel += h->bobAccel;
    h->bobPos += h->bobVel;
    em->posY += h->bobVel * h->bobGain;
    if (em->posY < em->floorY) {
        em->posY = em->floorY;
    }
}

s32 EmKind38_MoveByVelocity(Em *em)
{
    f32 vel[3];
    f32 *pos;
    f32 floorY;
    f32 *out;

    out = &em->velX;
    Vec_Add((f32 *) out, (f32 *) out, &em->accelX);
    Vec_Copy(vel, &em->velX);
    Vec_RotateY(vel, (6.2831855f * (f32) em->rotY) / 65536.0f);
    pos = &em->posX;
    Vec_Add(pos, pos, vel);
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
        return -1;
    }
    return 0;
}

void EmKind38_ClampVelocity(Em *em, f32 *max)
{
    f32 m;

    m = max[0];
    if (Math_Absf(em->velX) > m) {
        em->velX = (em->velX > 0.0f) ? m : -m;
        em->accelX = 0.0f;
    }
    m = max[1];
    if (Math_Absf(em->velY) > m) {
        em->velY = (em->velY > 0.0f) ? m : -m;
        em->accelY = 0.0f;
    }
    m = max[2];
    if (Math_Absf(em->velZ) > m) {
        em->velZ = (em->velZ > 0.0f) ? m : -m;
        em->accelZ = 0.0f;
    }
}

void EmKind38_KeepHeight(Em *em, u8 k)
{
    f32 *t = EmKind38_HeightRange[k];

    if (em->posY < 20.0f + em->goal[1]) {
        em->accelY = t[0];
    } else if (em->posY > 60.0f + em->goal[1]) {
        em->accelY = -t[0];
    } else {
        em->accelY = 0.0f;
        em->velY *= 0.8f;
    }
    if (em->velY > t[1]) {
        em->velY = t[1];
    }
    if (em->velY < -t[1]) {
        em->velY = -t[1];
    }
}

s32 func_0083B8F0(Em *em)
{
    if ((em->stateGroup != 5) && (em->partFlags[0] & 0x20)) {
        return 1;
    }
    return 0;
}

void EmKind38_SpawnAtEm(Em *em)
{
    QuestEm q;
    f32 v[3];
    Em *m;
    f32 *mtx;

    if (em->area == gScene.area) {
        q.kind = 0x38;
        q.variant = 1;
        q.hp = -1;
        q.spawnParam = 1;
        q.size = 100;
        q.level = em->level;
        m = QuestEm_Spawn(&q);
        if (m != NULL) {
            mtx = (f32 *)Em_Bone(em, 0x22);
            Vec_RotateSelf(EmKind38_SpawnOfs, mtx);
            Vec_Add(&m->posX, v, mtx + 12);
            v[2] = Math_Sqrtf(mtx[8] * mtx[8] + mtx[10] * mtx[10]);
            m->rotX = (u16) (s32) (10430.378f * Math_Atan2(-mtx[9], v[2]));
            m->rotY = (u16) (s32) (10430.378f * Math_Atan2(mtx[8], mtx[10]));
            m->rotZ = 0;
        }
    }
}

u8 func_0083BA70(Em *em)
{
    return ((EmKind38Work *)&em->work)->unk03C;
}

void EmKind38_PlaySeAtBone2(Em *em, s32 se)
{
    Em_PlaySeAtBoneVol(em, se, 0x2, 0x3);
}

/* Shakalaka's own meanings of the work area's bytes (EmWork, include/em.h, has the shared ones). */
typedef struct EmShakalakaWork {
    u8 fxStep;                  /* 0x000: UpdateMotionEffects: 0, then 1 */
    u8 unk001[0x00C - 0x001];
    u16 unk00C;                 /* 0x00C: Init: 0; Update counts it down */
    s16 unk00E;                 /* 0x00E: State3_01: 1 during its step, then 0; Init: 0 */
    u8 unk010[0x014 - 0x010];
    f32 home[3];                /* 0x014: Init: the spawn position (posX, 0, posZ); State1_08 turns to it */
    u8 unk020[0x050 - 0x020];
    s32 accelAngX;              /* 0x050: angles X, Y, Z for Em_MoveRotatedAccel (State4_04, State5_01_Hunted: 0, rotY, 0) */
    f32 accelAngY;              /* 0x054: rotY converted to a float (the game's own store; Em_MoveRotatedAccel reads the word as an angle) */
    s32 accelAngZ;              /* 0x058 */
    u8 unk05C[0x05D - 0x05C];
    u8 unk05D;                  /* 0x05D: Init: 0 */
    u8 unk05E[0x100 - 0x05E];
} EmShakalakaWork;
typedef char EmShakalakaWork_size_check[(sizeof(EmShakalakaWork) == sizeof(EmWork)) ? 1 : -1];

void EmShakalaka_MotionSounds(Em *em, EmWork *work)
{
    s16 motion;

    motion = work->fxMotion;
    switch (motion) {
    case 0x3E9:
        Em_PlaySeOnFrame(em, 0xC, Sound_RandomSe(0x24, 1, 0x24, 1));
        break;
    case 0x3EA:
        Em_PlaySeOnFrame(em, 0x10, 3);
        Em_PlaySeOnFrame(em, 0x16, 0x1D);
        Em_PlaySeOnFrame(em, 0x28, 0x12);
        break;
    case 0x3EB:
        Em_PlaySeOnFrame(em, 8, Sound_RandomSe(0x1C, 3, 0x1C, 2));
        Em_PlaySeOnFrame(em, 0x40, Sound_RandomSe(0x24, 2, 0x24, 2));
        Em_PlaySeOnFrame(em, 0x7A, Sound_RandomSe(0x1C, 2, 0x1C, 0));
        Em_PlaySeOnFrame(em, 0x98, Sound_RandomSe(0x24, 1, 0x24, 1));
        break;
    case 0x3EC:
        Em_PlaySeOnFrame(em, 8, 1);
        Em_PlaySeOnFrame(em, 0x18, 2);
        Em_PlaySeOnFrame(em, 0x28, 1);
        Em_PlaySeOnFrame(em, 0x38, 2);
        Em_PlaySeOnFrame(em, 0x46, 1);
        Em_PlaySeOnFrame(em, 0x58, 2);
        Em_PlaySeOnFrame(em, 0x64, 1);
        Em_PlaySeOnFrame(em, 0x72, 2);
        Em_PlaySeOnFrame(em, 0x2E, Sound_RandomSe(0x1A, 3, 0x1A, 2));
        Em_PlaySeOnFrame(em, 0x58, Sound_RandomSe(0x27, 3, 0x27, 2));
        break;
    case 0x3ED:
        Em_PlaySeOnFrame(em, 0xA, 1);
        Em_PlaySeOnFrame(em, 0x24, 2);
        Em_PlaySeOnFrame(em, 0x3A, 1);
        Em_PlaySeOnFrame(em, 0x53, 2);
        Em_PlaySeOnFrame(em, 0x10, Sound_RandomSe(0x1C, 2, 0x1C, 2));
        Em_PlaySeOnFrame(em, 0x3C, Sound_RandomSe(0x24, 1, 0x24, 0));
        break;
    case 0x3EE:
        Em_PlaySeOnFrame(em, 8, 0);
        Em_PlaySeOnFrame(em, 0x1C, 0);
        break;
    case 0x3EF:
        Em_PlaySeOnFrame(em, 8, 3);
        Em_PlaySeOnFrame(em, 0x10, 0x26);
        Em_PlaySeOnFrame(em, 0x1A, 1);
        Em_PlaySeOnFrame(em, 0x1E, 2);
        Em_PlaySeOnFrame(em, 0x2E, 1);
        Em_PlaySeOnFrame(em, 0x3E, 2);
        break;
    case 0x3F0:
        Em_PlaySeOnFrame(em, 8, 3);
        Em_PlaySeOnFrame(em, 0xC, 0x26);
        Em_PlaySeOnFrame(em, 0x1A, 1);
        Em_PlaySeOnFrame(em, 0x1E, 2);
        Em_PlaySeOnFrame(em, 0x2E, 1);
        Em_PlaySeOnFrame(em, 0x3E, 2);
        break;
    case 0x3F1:
        Em_PlaySeOnFrame(em, 0xE, 0x14);
        Em_PlaySeOnFrame(em, 0x32, 0x15);
        Em_PlaySeOnFrame(em, 0x58, 0x16);
        Em_PlaySeOnFrame(em, 0x14, 2);
        Em_PlaySeOnFrame(em, 0xA0, 1);
        break;
    case 0x3F2:
        Em_PlaySeOnFrame(em, 4, 0x1F);
        Em_PlaySeOnFrame(em, 0x28, 0x20);
        Em_PlaySeOnFrame(em, 0x4E, 0x21);
        Em_PlaySeOnFrame(em, 0xC, 2);
        Em_PlaySeOnFrame(em, 0x14, 1);
        break;
    case 0x3F3:
        Em_PlaySeOnFrame(em, 0x2E, 0x22);
        Em_PlaySeOnFrame(em, 0x44, 0x23);
        Em_PlaySeOnFrame(em, 0xE, 2);
        Em_PlaySeOnFrame(em, 0x14, 1);
        break;
    case 0x3F4:
        Em_PlaySeOnFrame(em, 0xA, 1);
        Em_PlaySeOnFrame(em, 0x1E, 2);
        break;
    case 0x3F5:
        Em_PlaySeOnFrame(em, 0xC, 1);
        Em_PlaySeOnFrame(em, 0x1A, 2);
        Em_PlaySeOnFrame(em, 0x2A, 1);
        Em_PlaySeOnFrame(em, 0x38, 2);
        Em_PlaySeOnFrame(em, 0x44, 1);
        Em_PlaySeOnFrame(em, 0x52, 2);
        Em_PlaySeOnFrame(em, 0x5E, 1);
        Em_PlaySeOnFrame(em, 0x6A, 2);
        Em_PlaySeOnFrame(em, 0x72, 1);
        Em_PlaySeOnFrame(em, 0x7E, 2);
        Em_PlaySeOnFrame(em, 0x12, 5);
        Em_PlaySeOnFrame(em, 0x32, 6);
        Em_PlaySeOnFrame(em, 0x52, 5);
        Em_PlaySeOnFrame(em, 0x72, 6);
        Em_PlaySeOnFrame(em, 8, Sound_RandomSe(0x2D, 2, 0x2D, 1));
        Em_PlaySeOnFrame(em, 0x44, Sound_RandomSe(0x2D, 1, 0x2D, 2));
        Em_PlaySeOnFrame(em, 0x82, Sound_RandomSe(0x2D, 1, 0x2D, 1));
        break;
    case 0x3F6:
        Em_PlaySeOnFrame(em, 4, 0xB);
        Em_PlaySeOnFrame(em, 0x20, 0x1E);
        Em_PlaySeOnFrame(em, 0x24, 7);
        Em_PlaySeOnFrame(em, 0x44, 0x12);
        break;
    case 0x3FD:
        Em_PlaySeOnFrame(em, 8, 0x28);
        Em_PlaySeOnFrame(em, 0x14, 1);
        Em_PlaySeOnFrame(em, 0x18, 5);
        Em_PlaySeOnFrame(em, 0x56, 2);
        break;
    case 0x3FF:
        Em_PlaySeOnFrame(em, 0xA, 0x29);
        Em_PlaySeOnFrame(em, 0x3A, Sound_RandomSe(0x2D, 3, 0x31, 2));
        Em_PlaySeOnFrame(em, 0x18, 5);
        Em_PlaySeOnFrame(em, 0x2C, 6);
        break;
    case 0x400:
        Em_PlaySeOnFrame(em, 2, 0x1B);
        Em_PlaySeOnFrame(em, 6, 9);
        Em_PlaySeOnFrame(em, 0x1E, 0xA);
        Em_PlaySeOnFrame(em, 0x6E, 0x11);
        break;
    case 0x401:
        Em_PlaySeOnFrame(em, 6, 0x17);
        Em_PlaySeOnFrame(em, 0x10, 0xF);
        Em_PlaySeOnFrame(em, 0x26, 0xF);
        Em_PlaySeOnFrame(em, 0x42, 0xF);
        Em_PlaySeOnFrame(em, 0x5E, 0xF);
        Em_PlaySeOnFrame(em, 0x7A, 0xF);
        break;
    case 0x402:
        Em_PlaySeOnFrame(em, 0x2C, 0xE);
        Em_PlaySeOnFrame(em, 0x2C, 0x27);
        Em_PlaySeOnFrame(em, 0x56, 0x32);
        Em_PlaySeOnFrame(em, 0x5A, 0xD);
        Em_PlaySeOnFrame(em, 0x70, 0x12);
        break;
    case 0x403:
        Em_PlaySeOnFrame(em, 2, 0x28);
        Em_PlaySeOnFrame(em, 8, 5);
        break;
    case 0x404:
        Em_PlaySeOnFrame(em, 2, 0x2A);
        Em_PlaySeOnFrame(em, 4, 6);
        break;
    case 0x405:
        Em_PlaySeOnFrame(em, 2, 8);
        Em_PlaySeOnFrame(em, 4, 0x12);
        Em_PlaySeOnFrame(em, 0x38, 0x17);
        Em_PlaySeOnFrame(em, 0x62, 0x18);
        Em_PlaySeOnFrame(em, 0x64, 7);
        break;
    case 0x407:
        Em_PlaySeOnFrame(em, 6, 1);
        Em_PlaySeOnFrame(em, 0x18, 2);
        Em_PlaySeOnFrame(em, 0x28, 1);
        Em_PlaySeOnFrame(em, 0x3A, 2);
        Em_PlaySeOnFrame(em, 0x46, 1);
        Em_PlaySeOnFrame(em, 0x58, 2);
        Em_PlaySeOnFrame(em, 0x64, 1);
        Em_PlaySeOnFrame(em, 0x72, 2);
        Em_PlaySeOnFrame(em, 0x7E, 1);
        Em_PlaySeOnFrame(em, 0x20, Sound_RandomSe(0x30, 3, 0x30, 2));
        Em_PlaySeOnFrame(em, 0x56, Sound_RandomSe(0x1A, 1, 0x1A, 2));
        break;
    case 0x424:
        Em_PlaySeOnFrame(em, 8, 0x1B);
        Em_PlaySeOnFrame(em, 0x16, 1);
        Em_PlaySeOnFrame(em, 0x22, 2);
        Em_PlaySeOnFrame(em, 0x46, 0x24);
        break;
    case 0x425:
        Em_PlaySeOnFrame(em, 2, 0x2F);
        Em_PlaySeOnFrame(em, 0x12, 0x10);
        Em_PlaySeOnFrame(em, 0x38, 4);
        Em_PlaySeOnFrame(em, 0x58, 0x10);
        Em_PlaySeOnFrame(em, 0x74, 0xA);
        break;
    case 0x427:
        Em_PlaySeOnFrame(em, 2, 4);
        Em_PlaySeOnFrame(em, 0x1E, 0x17);
        Em_PlaySeOnFrame(em, 0x26, 3);
        Em_PlaySeOnFrame(em, 0x32, 0x12);
        break;
    case 0x428:
        Em_PlaySeOnFrame(em, 0x1A, 0x13);
        Em_PlaySeOnFrame(em, 0x24, 4);
        Em_PlaySeOnFrame(em, 0x96, 0x24);
        Em_PlaySeOnFrame(em, 0x50, 1);
        Em_PlaySeOnFrame(em, 0x60, 2);
        Em_PlaySeOnFrame(em, 0x76, 1);
        Em_PlaySeOnFrame(em, 0x90, 2);
        break;
    case 0x429:
        Em_PlaySeOnFrame(em, 4, 0x19);
        Em_PlaySeOnFrame(em, 0x64, Sound_RandomSe(0x19, 2, 0x19, 2));
        break;
    case 0x42A:
        Em_PlaySeOnFrame(em, 6, 0x1A);
        Em_PlaySeOnFrame(em, 0xC, 0x13);
        Em_PlaySeOnFrame(em, 0x2E, 0x11);
        break;
    case 0x42B:
        Em_PlaySeOnFrame(em, 0xC, 0x1D);
        Em_PlaySeOnFrame(em, 0x78, 0x1D);
        Em_PlaySeOnFrame(em, 0xCA, 0x10);
        Em_PlaySeOnFrame(em, 0xE6, 0x11);
        break;
    case 0x42C:
        Em_PlaySeOnFrame(em, 0xA, 0);
        Em_PlaySeOnFrame(em, 0x1C, 0);
        Em_PlaySeOnFrame(em, 0x2A, 0);
        Em_PlaySeOnFrame(em, 0x3E, 0);
        Em_PlaySeOnFrame(em, 0x4A, 0);
        Em_PlaySeOnFrame(em, 0x5C, 0);
        Em_PlaySeOnFrame(em, 0x68, 0);
        Em_PlaySeOnFrame(em, 0x76, 0);
        Em_PlaySeOnFrame(em, 0x80, 0);
        Em_PlaySeOnFrame(em, 0x8C, 0);
        Em_PlaySeOnFrame(em, 0x96, 0);
        Em_PlaySeOnFrame(em, 0xA2, 0);
        Em_PlaySeOnFrame(em, 0xAE, 0);
        Em_PlaySeOnFrame(em, 0xC0, 3);
        Em_PlaySeOnFrame(em, 0xC8, 0x2B);
        Em_PlaySeOnFrame(em, 0xD6, 0xC);
        Em_PlaySeOnFrame(em, 0xDE, 0x2C);
        break;
    case 0x42D:
        Em_PlaySeOnFrame(em, 4, 0x2E);
        break;
    case 0x42E:
        Em_PlaySeOnFrame(em, 2, 0x10);
        Em_PlaySeOnFrame(em, 0xE, 0x2F);
        Em_PlaySeOnFrame(em, 0x18, 0xA);
        break;
    }
}

void EmShakalaka_Init(Em *em)
{
    EmShakalakaWork *w = (EmShakalakaWork *)&em->work;
    f32 total;
    f32 p;

    if (gQuest.stage == 0) {
        if (gScene.area == AREA_SNOWY_MOUNTAINS_NIGHT_9 || gScene.area == AREA_FOREST_HILLS_4) {
            em->posX = 9500.0f + 150.0f * (em->slotNo / 8);
            em->posY = 0.0f;
            em->posZ = 9200.0f + 150.0f * (em->slotNo & 7);
        } else {
            em->posX = 2400.0f + 150.0f * (u32) em->slotNo;
            em->posY = 0.0f;
            em->posZ = 3600.0f;
        }
    }
    em->blind = 0;
    em->remote = 0;
    Em_SetMotionRow(em, 1, 0, 0);
    em->stance = 0;
    em->hpMax = em->hp = Em_ComputeMaxHp(em, 0);
    w->unk00E = 0;
    w->unk00C = 0;
    w->home[0] = em->posX;
    w->home[1] = 0.0f;
    w->home[2] = em->posZ;
    w->unk05D = 0;
    em->painted = 0;
    switch (em->variant) {
    case 1:
        EmFxTri_Spawn(em, 3);
    case 0:
        total = Quest_GetTime(1);
        p = (100.0f * Quest_GetTime(0)) / total;
        if (p < 0.0f) {
            p = 0.0f;
        } else if (!(p <= 100.0f)) {
            p = 100.0f;
        }
        if (p < 33.0f) {
            if (Rand_Next(1) % 100 <= 29) {
                em->spawnParam = 2;
            } else {
                em->spawnParam = Rand_Next(1) & 1;
            }
        } else if (p < 66.0f) {
            if (Rand_Next(1) % 100 <= 69) {
                em->spawnParam = 2;
            } else {
                em->spawnParam = Rand_Next(1) & 1;
            }
        } else if (Rand_Next(1) % 100 <= 89) {
            em->spawnParam = 2;
        } else {
            em->spawnParam = Rand_Next(1) & 1;
        }
        break;
    case 3:
        EmFxTri_Spawn(em, 4);
        break;
    case 2:
        break;
    }
    Em_InitParts(em);
}

void EmShakalaka_Start(Em *em)
{
    EmShakalaka_Init(em);
    switch (em->variant) {
    case 0:
    case 1:
        Em_SetMotionRow(em, 0xF, 0, 0);
        EmShakalaka_ChangeState(em, 0, 8, 0);
        break;
    case 2:
    case 3:
        Em_SetMotionRow(em, 1, 0, 0);
        EmShakalaka_ChangeState(em, 0, 1, 0);
        break;
    default:
        EmShakalaka_ChangeState(em, 1, 8, 0);
        break;
    }
}

void EmShakalaka_Reinit(Em *em)
{
    EmShakalaka_Init(em);
}

s32 EmShakalaka_BombState(Em *em)
{
    EmKushalaWork *w = (EmKushalaWork *)&em->work;

    if (em->stance != 0 || (u32) (em->stateGroup - 4) <= 2 || em->active == 0) {
        return -1;
    }
    return w->unk00E != 0;
}

void EmShakalaka_CheckTargetArea(Em *em)
{
    s8 t;

    if (em->timer > 0xF) {
        t = em->targetNo;
        if (t == -1) {
            em->timer = 0xF;
        } else if (em->area != ((Player *) &gPlayers[t])->area) {
            em->timer = 0xF;
        }
    }
}

void EmShakalaka_ChangeState(Em *em, u16 group, u16 state, u8 mode)
{
    em->unk446 = 0;
    switch (group) {
    case 0:
        em->stance = 0;
        switch (state) {
        case 8:
            em->scriptPending = 0;
            em->senseDelay = 5;
            break;
        case 11:
            Em_EndStatus5(em);
            break;
        }
        break;
    case 1:
        em->stance = 0;
        Em_ComputeGoal2(em, em->goal);
        break;
    case 3:
        em->stance = 0;
        break;
    case 4:
        em->stance = 0;
        switch (state) {
        case 5:
            em->stance = 0;
            Em_BeginStatus5(em);
            break;
        }
        break;
    case 5:
        if (state == 2 && QuestEm_CanAppear(em) == 1) {
            QuestEm_Appear(em);
        }
        break;
    case 2:
    case 6:
    case 7:
    case 8:
        break;
    }
    Em_SetStateSync(em, group, state, mode);
}

void EmShakalaka_EndState(Em *em)
{
    em->motionSpeed = 1.0f;
    em->scriptPending = 1;
    EmShakalaka_ChangeState(em, 0, 1, 0);
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
}

s32 Angle_StepToward(s32 *angle, s32 *target, s32 step)
{
    s32 diff;
    s32 d;

    *target = (u16) *target;
    diff = (*target - *angle) & 0xFFFF;
    if (diff >= 0x8000) {
        d = -diff & 0xFFFF;
    } else {
        d = diff;
    }
    if (d < step) {
        step = d;
    }
    if (diff >= 0x8000) {
        *angle -= step;
    } else {
        *angle += step;
    }
    return step;
}

void EmShakalaka_State0_00(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        if (em->motionId[0] != 0x3E9) {
            Em_SetMotion(em, 1, 0, 0);
        }
        em->timer = (em->stateRand & 0xF) + 5;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->scriptPending = 1;
        }
        break;
    }
}

void EmShakalaka_State0_01(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        if (em->motionId[0] != 0x3E9) {
            Em_SetMotion(em, 1, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmShakalaka_EndState(em);
        }
        break;
    }
}

void EmShakalaka_State0_02(Em *em, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        switch (variant) {
        case 0:
        default:
            Em_SetMotion(em, 2, 0, 0);
            break;
        case 1:
            Em_SetMotion(em, 3, 0, 0);
            break;
        case 2:
            Em_SetMotion(em, 6, 0, 0);
            em->timer = 0x5A;
            break;
        }
        break;
    case 1:
        switch (variant) {
        case 0:
        default:
        case 1:
            if (em->motion[0].playing == 0) {
                EmShakalaka_EndState(em);
            }
            break;
        case 2:
            if (--em->timer <= 0) {
                EmShakalaka_EndState(em);
            }
            break;
        }
        break;
    }
}

void EmShakalaka_State0_05(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 9, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmShakalaka_EndState(em);
        }
        break;
    }
}

void EmShakalaka_State0_06(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0xA, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmShakalaka_EndState(em);
        }
        break;
    }
}

void EmShakalaka_State0_07(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0xB, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmShakalaka_EndState(em);
        }
        break;
    }
}

void EmShakalaka_State0_08(Em *em)
{
    Player *p;
    s32 i;

    em->hideShadowHold = 2;
    em->hideShadow = 2;
    em->stateArmor = 1;
    em->noPushTimer = 0xA;
    em->senseDelay = 5;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0xF, 0, 0);
        break;
    case 1:
        p = gPlayers;
        if (em->spawnParam == 2) {
            em->noHitTimer = 0xA;
            em->noPushTimer = 0xA;
            break;
        }
        for (i = 0; i < 4; i++, p++) {
            if (p->active != 0 && p->area == em->area && p->visible != 0
                && Vec_Distance((f32 *) &em->posX, (f32 *) p->pos) < EmShakalaka_NearDist[em->spawnParam]) {
                EmShakalaka_ChangeState(em, 3, 6, 4);
            }
        }
        break;
    }
}

void EmShakalaka_State0_09(Em *em)
{
    f32 pos[3];

    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x43, 0, 0);
        Em_BeginStatus2(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x3E, 0, 0);
        }
        break;
    case 2:
        pos[2] = 10.0f;
        pos[0] = 0.0f;
        pos[1] = 0.0f;
        Em_SpawnLyingPuffs(em, 0xA, pos, 0.6f);
        if (--em->timer <= 0) {
            em->step++;
            EmShakalaka_ChangeState(em, 0, 0xA, 4);
        }
        break;
    }
}

void EmShakalaka_State0_0A(Em *em)
{
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x3F, 0, 0);
        Em_EndStatus2(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmShakalaka_EndState(em);
        }
        break;
    }
}

void EmShakalaka_State0_0B(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x3F, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmShakalaka_EndState(em);
        }
        break;
    }
}

void EmShakalaka_RunGroup0(Em *em)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->state) {
    case 0:
        EmShakalaka_State0_00(em);
        break;
    case 1:
        EmShakalaka_State0_01(em);
        break;
    case 2:
        EmShakalaka_State0_02(em, 0);
        break;
    case 3:
        EmShakalaka_State0_02(em, 1);
        break;
    case 4:
        EmShakalaka_State0_02(em, 2);
        break;
    case 5:
        EmShakalaka_State0_05(em);
        break;
    case 6:
        EmShakalaka_State0_06(em);
        break;
    case 7:
        EmShakalaka_State0_07(em);
        break;
    case 8:
        EmShakalaka_State0_08(em);
        break;
    case 9:
        EmShakalaka_State0_09(em);
        break;
    case 10:
        EmShakalaka_State0_0A(em);
        break;
    case 11:
        EmShakalaka_State0_0B(em);
        
    default:
        break;
    }
}

void EmShakalaka_State1_00(Em *em, s32 variant)
{
    f32 range;
    s32 t;
    s32 k2;
    u32 k;

    switch (em->step) {
    case 0:
        em->step++;
        k = variant & 0xFF;
        em->timer = 0x12C;
        switch (k) {
        case 6:
            em->timer = 0xD2;
            Em_SetMotion(em, 0xC, 0, 0);
            break;
        case 0x0:
        default:
            Em_SetMotion(em, 0xC, 0, 0);
            break;
        case 1:
            Em_SetMotion(em, 4, 0, 0);
            break;
        case 2:
        case 5:
        case 4:
            Em_SetMotion(em, 0xD, 0, 0);
            break;
        case 3:
            Em_SetMotion(em, 0xD, 0, 0);
            em->speedBonus = 0x32;
        }
        break;
    case 1:
        k2 = variant & 0xFF;
        switch (k2) {
        case 6:
            range = 700.0f;
            break;
        case 5:
            range = 600.0f;
            break;
        case 4:
            range = 300.0f;
            break;
        default:
            range = 200.0f;
            break;
        }
        if ((Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) < range) || (t = em->timer - 1, em->timer = t, (t <= 0))) {
            EmShakalaka_EndState(em);
            break;
        }
        em->targetAngle = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        Angle_StepToward(&em->rotY, &em->targetAngle, 0x800);
        Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
        EmShakalaka_CheckTargetArea(em);
        break;
    }
}

void EmShakalaka_State1_03(Em *em, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 4;
    case 1:
        em->step++;
        switch (Rand_Next(1) & 1) {
        case 0:
        default:
            Em_SetMotion(em, 7, 0, 0);
            break;
        case 1:
            Em_SetMotion(em, 8, 0, 0);
            break;
        }
        em->targetAngle = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            if (--em->timer <= 0) {
                EmShakalaka_EndState(em);
            } else {
                em->step = 1;
            }
        } else {
            em->targetAngle = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
            Angle_StepToward(&em->rotY, &em->targetAngle, 0x800);
            Mtx_FromAnglesXYZ((s32 *) &em->rotX, (f32 *) em->rotMtx);
            EmShakalaka_CheckTargetArea(em);
        }
        break;
    }
}

void EmShakalaka_State1_05(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->targetAngle = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        Em_SetMotion(em, 0xC, 0, 0);
        Player_SetFlag((Player *) em, EM_FLAG_NO_ROOT_MOTION);
        break;
    case 1:
        if (Angle_StepToward(&em->rotY, &em->targetAngle, 0x800) < 0x800) {
            Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
            EmShakalaka_EndState(em);
        }
        break;
    }
    Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
}

void EmShakalaka_State1_06(Em *em, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 0x12C;
        switch (variant & 0xFF) {
        case 0:
        default:
            Em_SetMotion(em, 0xC, 0, 0);
            break;
        case 1:
            Em_SetMotion(em, 0xD, 0, 0);
            break;
        }
        em->targetAngle = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        break;
    case 1:
        if (Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) < 100.0f || --em->timer <= 0) {
            EmShakalaka_EndState(em);
        } else {
            Angle_StepToward(&em->rotY, &em->targetAngle, 0x800);
            Mtx_FromAnglesXYZ((s32 *) &em->rotX, (f32 *) em->rotMtx);
        }
        break;
    }
}

void EmShakalaka_State1_08(Em *em, s32 variant)
{
    EmShakalakaWork *w = (EmShakalakaWork *)&em->work;

    switch (em->step) {
    case 0:
        em->step++;
        em->timer = (em->stateRand & 0x7F) + 0x78;
        switch (variant & 0xFF) {
        case 0:
        default:
            Em_SetMotion(em, 0xC, 0, 0);
            break;
        case 1:
            Em_SetMotion(em, 4, 0, 0);
            break;
        }
        break;
    case 1:
        if (em->timer > 0) {
            em->timer--;
        }
        if (em->remote == 0 && em->timer <= 0) {
            EmShakalaka_EndState(em);
        } else {
            em->targetAngle = Math_AngleXZ(&em->posX, w->home) & 0xFFFF;
            Angle_StepToward(&em->rotY, &em->targetAngle, 0x100);
            Mtx_FromAnglesXYZ((s32 *) &em->rotX, (f32 *) em->rotMtx);
        }
        break;
    }
}

void EmShakalaka_State1_0D(Em *em, s8 variant)
{
    Player *p = &gPlayers[em->targetNo];
    f32 pos[3] = {0};

    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 150;
        Em_SetMotion(em, 0xD, 0, 0);
        if (variant & 1) {
            em->speedBonus = 0x32;
        }
        break;
    case 1:
        if (variant <= 1) {
            pos[2] = -600.0f;
        } else if (variant <= 3) {
            pos[0] = 600.0f;
        } else {
            pos[0] = -600.0f;
        }
        Vec_RotateY(pos, (6.2831855f * (f32) p->rot[1]) / 65536.0f);
        pos[0] += p->pos[0];
        pos[1] += p->pos[1];
        pos[2] += p->pos[2];
        if (Vec_Distance((f32 *) &em->posX, (f32 *) pos) < 300.0f || --em->timer <= 0) {
            EmShakalaka_EndState(em);
        } else {
            em->targetAngle = Math_AngleXZ(&em->posX, pos) & 0xFFFF;
            Angle_StepToward(&em->rotY, &em->targetAngle, 0x800);
            Mtx_FromAnglesXYZ((s32 *) &em->rotX, (f32 *) em->rotMtx);
            EmShakalaka_CheckTargetArea(em);
        }
        break;
    }
}

void EmShakalaka_State1_13(Em *em)
{
    s32 t;

    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 0x12C;
        Em_SetMotion(em, 0xD, 0, 0);
        Em_SetSpeedScaled(em, 2, 500.0f, -400.0f);
        break;
    case 1:
        if ((Em_StepToGoal(em, 0x40, 0) == 1) || (t = em->timer - 1, em->timer = t, (t <= 0))) {
            em->step++;
            Em_SetMotion(em, 0x18, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x40, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmShakalaka_EndState(em);
        }
        break;
    }
}

void EmShakalaka_RunGroup1(Em *em)
{
    switch (em->state) {
    case 0:
        EmShakalaka_State1_00(em, 0);
        break;
    case 1:
        EmShakalaka_State1_00(em, 1);
        break;
    case 2:
        EmShakalaka_State1_00(em, 2);
        break;
    case 3:
        EmShakalaka_State1_03(em, 0);
        break;
    case 4:
        EmShakalaka_State1_03(em, 1);
        break;
    case 5:
        EmShakalaka_State1_05(em);
        break;
    case 6:
        EmShakalaka_State1_06(em, 0);
        break;
    case 7:
        EmShakalaka_State1_06(em, 1);
        break;
    case 8:
        EmShakalaka_State1_08(em, 0);
        break;
    case 9:
        EmShakalaka_State1_08(em, 1);
        break;
    case 10:
        EmShakalaka_State1_00(em, 3);
        break;
    case 11:
        EmShakalaka_State1_00(em, 4);
        break;
    case 12:
        EmShakalaka_State1_00(em, 5);
        break;
    case 13:
        EmShakalaka_State1_0D(em, 0);
        break;
    case 14:
        EmShakalaka_State1_0D(em, 1);
        break;
    case 15:
        EmShakalaka_State1_0D(em, 2);
        break;
    case 16:
        EmShakalaka_State1_0D(em, 3);
        break;
    case 17:
        EmShakalaka_State1_0D(em, 4);
        break;
    case 18:
        EmShakalaka_State1_0D(em, 5);
        break;
    case 19:
        EmShakalaka_State1_13(em);
        break;
    case 20:
        EmShakalaka_State1_00(em, 6);
        
    default:
        break;
    }
}

void EmShakalaka_RunGroup2(Em *em)
{
    EmShakalaka_State0_00(em);
}

void EmShakalaka_State3_00(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x15, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmShakalaka_EndState(em);
        }
        break;
    }
}

void EmShakalaka_State3_01(Em *em, s32 variant)
{
    EmShakalakaWork *w = (EmShakalakaWork *)&em->work;

    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x19, 2, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x1A, 0, 0);
        }
        break;
    case 2:
        if (EmMotion_PassedFrameScaled(em, 45.0f, 0U) != 0) {
            em->step++;
            w->unk00E = 1;
            if (!(variant & 0xFF)) {
                EmBomb_Create(em, 4);
                break;
            }
            EmBomb_Create(em, 5);
        }
        break;
    case 3:
        if (EmMotion_PassedFrameScaled(em, 94.0f, 0U) != 0) {
            em->step++;
            w->unk00E = 0;
        }
        
    case 4:
        if (em->motion[0].playing == 0) {
            EmShakalaka_EndState(em);
        }
        break;
    }
}

void EmShakalaka_State3_02(Em *em)
{
    f32 posY;

    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0xA, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x16, 0, 0);
            break;
        }
    default:
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stance = 2;
            Em_SetMotion(em, 0x1B, 0, 0);
            Player_ClearVelocity(em);
            em->velY = 30.0f;
            em->accelY = -2.5f;
            em->velZ = 6.0f;
            ShakalakaAttack_Create(em, 4);
        }
        break;
    case 3:
        Em_MoveRotatedAccel(em, &em->rotX);
        posY = em->posY;
        if ((posY < em->prevPosY) && (posY < 160.0f)) {
            em->step++;
            Em_SetMotion(em, 0x1C, 0, 0);
        }
        break;
    case 4:
        Em_MoveRotatedAccel(em, &em->rotX);
        if (em->posY < (80.0f + em->floorY)) {
            em->stance = 0;
            em->posY = em->floorY;
            Player_ClearVelocity(em);
            em->step++;
            Em_SetMotion(em, 0x1D, 0, 0);
        }
        break;
    case 5:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stance = 0;
            em->posY = em->floorY;
            EmShakalaka_EndState(em);
        }
        break;
    }
}

void EmShakalaka_State3_03(Em *em, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x17, 0, 0);
        break;
    case 1:
        if (Motion_ReachedFrame(em, 0, 8.0f)) {
            em->step++;
            em->stance = 2;
            Player_ClearVelocity(em);
            em->velY = EmShakalaka_State3_03Vel[variant & 0xFF].x;
            em->accelY = D_008CA324[variant & 0xFF].x;
            em->velZ = D_008CA328[variant & 0xFF].x;
            ShakalakaAttack_Create(em, 2);
        }
        break;
    case 2:
        Em_MoveRotatedAccel(em, &em->rotX);
        if (em->posY <= em->floorY) {
            em->step++;
            em->stance = 0;
            em->posY = em->floorY;
            Em_SetMotion(em, 0x18, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            EmShakalaka_EndState(em);
        }
        break;
    }
}

void EmShakalaka_State3_06(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->noReact = 1;
        Em_SetMotion(em, 0xE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmShakalaka_EndState(em);
        }
        break;
    }
}

void EmShakalaka_RunGroup3(Em *em)
{
    switch (em->state) {
    case 0:
        EmShakalaka_State3_00(em);
        break;
    case 1:
        EmShakalaka_State3_01(em, 0);
        break;
    case 2:
        EmShakalaka_State3_02(em);
        break;
    case 3:
        EmShakalaka_State3_03(em, 0);
        break;
    case 4:
        EmShakalaka_State3_03(em, 1);
        break;
    case 5:
        EmShakalaka_State3_03(em, 2);
        break;
    case 6:
        EmShakalaka_State3_06(em);
        break;
    case 7:
        EmShakalaka_State3_01(em, 1);
        
    default:
        break;
    }
}

void EmShakalaka_State4_00(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x3C, 0, 0);
        em->rotY = em->hitAngle + 0x7FFF + 1;
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmShakalaka_EndState(em);
        }
        break;
    }
}

void EmShakalaka_State4_01(Em *em)
{
    f32 m[16];
    s32 ang[3];

    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x3D, 0, 0);
        ang[0] = 0;
        em->rotY = ang[1] = em->hitAngle + 0x8000;
        ang[2] = 0;
        Mtx_FromAnglesXYZ((s32 *) ang, m);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stance = 0;
            Em_SetMotion(em, 0x3E, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x3F, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            EmShakalaka_EndState(em);
        }
        break;
    }
}

void EmShakalaka_State4_02(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x3C, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmShakalaka_EndState(em);
        }
        break;
    }
}

void EmShakalaka_State4_03(Em *em)
{
    s32 timer;

    timer = em->timer;
    if (timer > 0) {
        em->timer = timer - 1;
        Em_SpawnStatus4Effect(em, 2);
        Em_SetHitReactMode(em, EM_HIT_REACT_STATUS);
    }
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x41, 0, 0);
        em->rotY = em->hitAngle + 0x7FFF + 1;
        Em_BeginStatus4(em);
        em->noReact = 1;
        Em_RestartScript(em);
        break;
    case 1:
        if (em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x42, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x40, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            Em_EndStatus4(em);
            em->noReact = 0;
            EmShakalaka_EndState(em);
        }
        break;
    }
}

void EmShakalaka_State4_04(Em *em)
{
    EmShakalakaWork *w = (EmShakalakaWork *)&em->work;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x45, 0, 0);
        Player_ClearVelocity(em);
        if (!(em->velY <= -5.0f)) {
            em->velY = -5.0f;
        }
        em->accelX = 0.0f;
        em->accelY = -1.0f;
        em->accelZ = 0.0f;
        w->accelAngX = 0;
        w->accelAngZ = 0;
        break;
    case 1:
        w->accelAngY = (f32) em->rotY;
        Em_MoveRotatedAccel(em, &w->accelAngX);
        if (em->posY <= em->floorY) {
            em->step++;
            em->posY = em->floorY;
            em->stance = 0;
            Em_SetMotion(em, 0x46, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x40, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmShakalaka_EndState(em);
            Player_ClearVelocity(em);
        }
        break;
    }
}

void EmShakalaka_State4_05(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x43, 0, 0);
        StageObj_Stun_CreateEm(em, 1, 0, *(s16 *)((u8 *)em + 8), 90.0f, 1.0f);
        break;
    case 1:
        em->timer -= 1;
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x3E, 0, 0);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            EmShakalaka_ChangeState(em, 0, 0xB, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmShakalaka_ChangeState(em, 0, 0xB, 4);
        }
        break;
    }
}

void EmShakalaka_RunGroup4(Em *em)
{
    switch (em->state) {
    case 0:
        EmShakalaka_State4_00(em);
        break;
    case 1:
        EmShakalaka_State4_01(em);
        break;
    case 2:
        EmShakalaka_State4_02(em);
        break;
    case 3:
        EmShakalaka_State4_03(em);
        break;
    case 4:
        EmShakalaka_State4_04(em);
        break;
    case 5:
        EmShakalaka_State4_05(em);
        
    default:
        break;
    }
}

void EmShakalaka_State5_00_Hunted(Em *em)
{
    f32 m[16];
    s32 ang[3];

    em->noHitTimer = 10;
    em->noPushTimer = 10;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->posY = em->floorY;
        Em_SetMotion(em, 0x44, 0, 0);
        ang[1] = em->hitAngle + 0x8000;
        ang[2] = 0;
        Mtx_FromAnglesXYZ((s32 *) ang, m);
        QuestEm_Hunted(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 0x960;
            if (em->variant == 3) {
                EmMine_Create(em, 1);
            } else {
                EmMine_Create(em, 0);
            }
        }
        break;
    case 2:
        if (EmMine_Update(em) <= 0 || --em->timer <= 0) {
            em->step++;
            em->timer = 100;
            EmMine_Remove(em);
        }
        break;
    case 3:
        em->motionSpeed = 0.0f;
        em->timer--;
        if (em->timer <= 99) {
            em->alpha = em->timer / 100.0f;
        }
        if (em->timer <= 0) {
            em->visible = 0;
            EmShakalaka_ChangeState(em, 5, 2, 4);
        }
        break;
    }
}

void EmShakalaka_State5_01_Hunted(Em *em)
{
    EmShakalakaWork *w = (EmShakalakaWork *)&em->work;
    f32 m[16];
    s32 ang[3];

    em->noHitTimer = 10;
    em->noPushTimer = 10;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x45, 0, 0);
        Player_ClearVelocity(em);
        if (!(em->velY <= -5.0f)) {
            em->velY = -5.0f;
        }
        em->accelX = 0.0f;
        em->accelY = -1.0f;
        *(s32 *) &em->accelZ = 0;
        w->accelAngX = 0;
        w->accelAngZ = 0;
        QuestEm_Hunted(em);
        break;
    case 1:
        w->accelAngY = em->rotY;
        Em_MoveRotatedAccel(em, &w->accelAngX);
        if (em->posY <= em->floorY) {
            em->step++;
            em->posY = em->floorY;
            em->stance = 0;
            Em_SetMotion(em, 0x46, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x40, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x44, 0, 0);
            ang[1] = em->hitAngle + 0x8000;
            ang[2] = 0;
            Mtx_FromAnglesXYZ((s32 *) ang, m);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 0x960;
            if (em->variant == 3) {
                EmMine_Create(em, 1);
            } else {
                EmMine_Create(em, 0);
            }
        }
        break;
    case 5:
        if (EmMine_Update(em) <= 0 || --em->timer <= 0) {
            em->step++;
            em->timer = 100;
            EmMine_Remove(em);
        }
        break;
    case 6:
        em->motionSpeed = 0.0f;
        em->timer--;
        if (em->timer <= 99) {
            em->alpha = em->timer / 100.0f;
        }
        if (em->timer <= 0) {
            em->visible = 0;
            EmShakalaka_ChangeState(em, 5, 2, 4);
        }
        break;
    }
}

void EmShakalaka_State5_02(Em *em)
{
    switch (em->step) {
    case 0:
        if (QuestEm_CanAppear(em) == 1) {
            switch (em->variant) {
            case 0:
            case 1:
                Em_SetMotionRow(em, 0xF, 0, 0);
                EmShakalaka_ChangeState(em, 0, 8, 0);
                break;
            case 2:
            case 3:
                Em_SetMotionRow(em, 1, 0, 0);
                EmShakalaka_ChangeState(em, 0, 1, 0);
                break;
            default:
                EmShakalaka_ChangeState(em, 1, 8, 0);
                break;
            }
        } else {
            em->taskPhase++;
        }
        break;
    case 1:
        break;
    }
}

void EmShakalaka_RunGroup5(Em *em)
{
    switch (em->state) {
    case 0:
        EmShakalaka_State5_00_Hunted(em);
        break;
    case 1:
        EmShakalaka_State5_01_Hunted(em);
        break;
    case 2:
        EmShakalaka_State5_02(em);
        break;
    }
}

void EmShakalaka_State6_00(void)
{
}

void EmShakalaka_RunGroup6(Em *em)
{
    switch (em->state) {
    case 0:
        EmShakalaka_State6_00();
        break;
    }
}

void EmShakalaka_Update(Em *em)
{
    EmShakalakaWork *w = (EmShakalakaWork *)&em->work;
    u8 buf[3];

    if (w->unk00C != 0) {
        (w->unk00C)--;
    }
    switch (Em_UpdateDamage(em, buf) & 0xFF) {
    case 0:
    case 3:
    case 4:
    case 7:
    case 9:
    case 11:
    case 15:
    case 16:
    case 18:
        break;
    case 1:
    case 2:
        if (em->stance == 2) {
            EmShakalaka_ChangeState(em, 5, 1, 3);
        } else {
            EmShakalaka_ChangeState(em, 5, 0, 3);
        }
        break;
    case 5:
        if (em->stateGroup != 4 || em->state != 4) {
            EmShakalaka_ChangeState(em, 4, 4, 3);
        }
        break;
    case 6:
        if (em->stateGroup != 4 || em->state != 4) {
            Em_SetStatus4Timer(em);
            EmShakalaka_ChangeState(em, 4, 3, 3);
        }
        break;
    case 8:
        if (em->stateGroup != 4 || em->state != 4) {
            Em_SetStatus2Timer(em);
            EmShakalaka_ChangeState(em, 0, 9, 3);
        }
        break;
    case 10:
        em->blind = 0;
        Em_EndStatus2(em);
        EmShakalaka_ChangeState(em, 4, 1, 3);
        break;
    case 12:
    case 13:
        EmShakalaka_ChangeState(em, 4, 1, 3);
        Em_ResetPartHp(em, 0);
        break;
    case 14:
        if (em->stance == 2) {
            EmShakalaka_ChangeState(em, 4, 1, 3);
        } else {
            EmShakalaka_ChangeState(em, 4, 0, 3);
        }
        em->scriptPending = 0;
        break;
    case 17:
        if ((em->stateGroup != 4 || em->state != 4) && (em->stateGroup != 4 || em->state != 5)) {
            Em_SetStatus5Timer(em);
            EmShakalaka_ChangeState(em, 4, 5, 3);
        }
        break;
    }
    Em_UpdatePendingScript(em);
    Em_ClearHitReactMode(em);
    EmShakalaka_RunState(em);
    if (em->stateRequest != 0) {
        EmShakalaka_RunState(em);
        em->stateRequest = 0;
    }
}

void EmShakalaka_RunState(Em *em)
{
    switch (em->stateGroup) {
    case 0:
        EmShakalaka_RunGroup0(em);
        break;
    case 1:
        EmShakalaka_RunGroup1(em);
        break;
    case 2:
        EmShakalaka_RunGroup2(em);
        break;
    case 3:
        EmShakalaka_RunGroup3(em);
        break;
    case 4:
        EmShakalaka_RunGroup4(em);
        break;
    case 5:
        EmShakalaka_RunGroup5(em);
        break;
    case 6:
        EmShakalaka_RunGroup6(em);
        break;
    case 7:
        EmShakalaka_RunGroup0(em);
        
    default:
        break;
    }
}

static void EmShakalaka_MotionEffectsDefault(Em *em)
{
}

void EmShakalaka_SpawnGroundEffect(Em *em, s32 a1, s32 a2, f32 scale)
{
    f32 pos[3];

    if (Player_InCurrentArea(em)) {
        Em_BonePos(em, a1, pos);
        pos[1] = em->floorY;
        Effect0D_SpawnOnGround(pos, a2, scale * em->scaleX);
    }
}

void EmShakalaka_SpawnGroundEffectOffset(Em *em, s32 a1, s32 a2, f32 *ofs, f32 scale)
{
    f32 pos[3];
    f32 v[3];

    if (Player_InCurrentArea(em)) {
        Vec_Copy(v, ofs);
        Vec_RotateY(v, (6.2831855f * (f32) em->rotY) / 65536.0f);
        Em_BonePos(em, a1, pos);
        pos[1] = em->floorY;
        pos[0] = pos[0] + v[0];
        pos[1] = pos[1] + v[1];
        pos[2] = pos[2] + v[2];
        Effect0D_SpawnOnGround(pos, a2, scale * em->scaleX);
    }
}

void EmShakalaka_MotionEffects(Em *em, EmWork *work)
{
    f32 pos[3];
    f32 v[3];

    if (em->motionId[0] != work->fxMotion) {
        work->fxMotion = em->motionId[0];
    }
    EmShakalaka_MotionSounds(em, work);
    switch (work->fxMotion) {
    case 0x3EF:
    case 0x3F0:
        if (EmMotion_PassedFrameScaled(em, 8.0f, 0) != 0) {
            EmShakalaka_SpawnGroundEffect(em, 1, 0x14, 2.0f);
        }
        break;
    case 0x3FD:
        if (EmMotion_PassedFrameScaled(em, 24.0f, 0) != 0) {
            ShakalakaAttack_Create(em, 1);
        }
        break;
    case 0x3F5:
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            ShakalakaAttack_Create(em, 3);
        }
        break;
    case 0x3F6:
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            ShakalakaAttack_Create(em, 5);
        }
        if (EmMotion_PassedFrameScaled(em, 20.0f, 0) != 0) {
            EmShakalaka_SpawnGroundEffect(em, 1, 0x14, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 70.0f, 0) != 0) {
            EmShakalaka_SpawnGroundEffect(em, 1, 4, 2.0f);
        }
        break;
    case 0x400:
        if (EmMotion_PassedFrameScaled(em, 8.0f, 0) != 0) {
            EmShakalaka_SpawnGroundEffect(em, 0xA, 0x14, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 42.0f, 0) != 0) {
            EmShakalaka_SpawnGroundEffect(em, 0xA, 4, 2.0f);
        }
        break;
    case 0x405:
        if ((EmMotion_PassedFrameScaled(em, 4.0f, 0) != 0) && Player_InCurrentArea(em)) {
            v[0] = 0.0f;
            v[1] = 0.0f;
            v[2] = 55.0f * em->scaleZ;
            EmShakalaka_SpawnGroundEffectOffset(em, 1, 0x28, v, 0.2f);
            EmShakalaka_SpawnGroundEffectOffset(em, 1, 0xE, v, 0.4f);
            Vec_RotateY(v, (6.2831855f * (f32) em->rotY) / 65536.0f);
            Em_BonePos(em, 1, pos);
            pos[1] = em->floorY;
            pos[0] += v[0];
            pos[1] += v[1];
            pos[2] += v[2];
            StageObj_K11_CreateAtScaled(pos, *(u16 *) &em->rotY, 0xA, 0.2f * em->scaleX);
            EmImpactAttack_CreateAt(em, (f32 *) pos, 0xD);
        }
        break;
    case 0x425:
        if ((EmMotion_PassedFrameScaled(em, 28.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 60.0f, 0) != 0)) {
            EmShakalaka_SpawnGroundEffect(em, 1, 0x14, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 96.0f, 0) != 0) {
            EmShakalaka_SpawnGroundEffect(em, 1, 4, 2.0f);
        }
        break;
    case 0x42A:
        if (EmMotion_PassedFrameScaled(em, 48.0f, 0) != 0) {
            EmShakalaka_SpawnGroundEffect(em, 0xA, 4, 2.0f);
        }
        break;
    case 0x42B:
        if (EmMotion_PassedFrameScaled(em, 206.0f, 0) != 0) {
            EmShakalaka_SpawnGroundEffect(em, 0xA, 4, 2.0f);
        }
        break;
    case 0x42C:
        if ((EmMotion_PassedFrameScaled(em, 16.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 46.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 76.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 106.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 132.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 152.0f, 0) != 0)) {
            v[0] = 0.0f;
            v[1] = 0.0f;
            v[2] = 55.0f * em->scaleZ;
            EmShakalaka_SpawnGroundEffectOffset(em, 1, 4, v, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 218.0f, 0) != 0) {
            EmShakalaka_SpawnGroundEffect(em, 0xA, 7, 0.5f);
        }
        break;
    case 0x42E:
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            EmShakalaka_SpawnGroundEffect(em, 3, 4, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 48.0f, 0) != 0) {
            EmShakalaka_SpawnGroundEffect(em, 3, 7, 0.5f);
        }
        break;
    default:
        EmShakalaka_MotionEffectsDefault(em);
        break;
    }
}

void EmShakalaka_UpdateMotionEffects(Em *em)
{
    EmShakalakaWork *w = (EmShakalakaWork *)&em->work;

    switch (w->fxStep) {
    case 0:
        w->fxStep++;
        break;
    case 1:
        EmShakalaka_MotionEffects(em, (EmWork *) w);
        break;
    }
}

s32 EmShakalaka_ScriptQuery(Em *em, u8 flag)
{
    switch (flag) {
    case 1:
        if (EmList_FindKindNear(em, 0x3B, 1, 2000.0f) == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

void EmShakalaka_SpawnEffect01(u8 *em)
{
    Effect01_Spawn((Player *) em, 0);
}

void EmShakalaka_TableNop(void)
{
}

void EmYamaTsukami_StartCurves(Em *em, u8 kind)
{
    EmYamaTsukamiWork *w = (EmYamaTsukamiWork *)&em->work;

    w->curveNone = 0;
    w->curveVelY = 0;
    w->curveSpeed = 0;
    w->curveKind = kind;
    switch (w->curveKind) {
    case 0:
        w->curveVelY = 1;
        w->curveFrame = 0;
        break;
    case 1:
        w->curveVelY = 1;
        w->curveFrame = 0;
        break;
    case 2:
        w->curveVelY = 1;
        w->curveFrame = 0;
        break;
    case 3:
        w->curveVelY = 1;
        w->curveFrame = 0;
        break;
    case 4:
        w->curveSpeed = 1;
        w->curveFrame = 0;
        break;
    case 5:
        w->curveSpeed = 1;
        w->curveFrame = 0;
        break;
    case 6:
        w->curveVelY = 1;
        w->curveFrame = 0;
        break;
    case 7:
        w->curveVelY = 1;
        w->curveFrame = 0;
        break;
    case 8:
        w->curveVelY = 1;
        w->curveFrame = 0;
        break;
    case 9:
        w->curveVelY = 1;
        w->curveFrame = 0;
        break;
    case 10:
        w->curveVelY = 1;
        w->curveFrame = 0;
        break;
    case 11:
        w->curveVelY = 1;
        w->curveFrame = 0;
        break;
    case 12:
        w->curveVelY = 1;
        w->curveFrame = 0;
        break;
    }
    Player_ClearVelocity(em);
}

static s32 EmYamaTsukami_CurveNone(Em *em, EmWork *work)
{
    return 0;
}


extern EmCurveKey *EmYamaTsukami_VelYCurves[];

/* The value of curve work[0x2F] at the frame in the work area (a table of keys): sets
 * em->velY from the key pair around the frame, 2 at the table's end. */
static u8 EmYamaTsukami_CurveVelY(Em *em, EmWork *work)
{
    EmYamaTsukamiWork *w = (EmYamaTsukamiWork *) work;
    EmCurveKey *p;
    f32 frame;
    f32 t;
    s32 i;
    EmCurveKey *tbl;
    u8 ret;

    tbl = EmYamaTsukami_VelYCurves[((EmYamaTsukamiWork *)work)->curveKind];
    ret = 0;
    if (tbl == NULL) {
        return ret;
    }
    frame = w->curveFrame;
    if (frame != 0.0f && frame != 1.0f) {
        i = 1;
        while (i != 0) {
            p = &tbl[i];
            t = p->t;
            if (frame > t && t > 0.0f) {
                i++;
            } else if (t == 0.0f) {
                ret = 2;
                if (em->velY > -50.0f) {
                    em->velY -= 1.0f;
                }
                break;
            } else if (t < 0.0f) {
                w->curveFrame = 0;
                ret = 2;
                em->velY = 0.0f;
                break;
            } else {
                i = 0;
                em->velY = (p->v - p[-1].v) / ((t - p[-1].t) / em->motion[0].step);
            }
        }
    }
    return ret;
}

extern EmCurveKey *EmYamaTsukami_SpeedCurves[];

/* The value of curve work[0x2F] at the frame in the work area: sets em->velZ from the key
 * pair around the frame, 4 at the table's end. */
static u8 EmYamaTsukami_CurveSpeed(Em *em, EmWork *work)
{
    EmYamaTsukamiWork *w = (EmYamaTsukamiWork *) work;
    EmCurveKey *p;
    f32 frame;
    f32 t;
    s32 i;
    EmCurveKey *tbl;
    u8 ret;

    tbl = EmYamaTsukami_SpeedCurves[((EmYamaTsukamiWork *)work)->curveKind];
    ret = 0;
    if (tbl == NULL) {
        return ret;
    }
    frame = w->curveFrame;
    if (frame != 0.0f && frame != 1.0f) {
        i = 1;
        while (i != 0) {
            p = &tbl[i];
            t = p->t;
            if (frame > t && t != 0.0f) {
                i++;
            } else if (t == 0.0f) {
                ret = 4;
                break;
            } else {
                i = 0;
                em->velZ = (p->v - p[-1].v) / ((t - p[-1].t) / em->motion[0].step);
            }
        }
    }
    return ret;
}

/* Runs the family's three curves when their flags in the work area (+0x438) are set, advances
 * the curve frame by the motion's step, and turns toward the target while unk390 is 2. */
u8 EmYamaTsukami_UpdateCurves(Em *em)
{
    EmYamaTsukamiWork *w = (EmYamaTsukamiWork *)&em->work;
    u8 ret = 0;

    if (w->curveNone != 0) {
        ret = EmYamaTsukami_CurveNone(em, (EmWork *) w);
    }
    if (w->curveVelY != 0) {
        ret |= EmYamaTsukami_CurveVelY(em, (EmWork *) w);
    }
    if (w->curveSpeed != 0) {
        ret |= EmYamaTsukami_CurveSpeed(em, (EmWork *) w);
    }
    w->curveFrame += (s16) em->motion[0].step;
    if (em->stance == 2) {
        w->accelAngX = 0;
        w->accelAngY = em->rotY;
        w->accelAngZ = 0;
        Em_MoveRotatedAccel(em, &w->accelAngX);
    }
    return ret;
}

void EmYamaTsukami_SetGroup0State(Em *em, u16 state, u8 mode)
{
    EmYamaTsukamiWork *w = (EmYamaTsukamiWork *)&em->work;

    em->motionSpeed = 1.0f;
    switch ((u16) state) {
    case 4:
        w->easeIdx = 2;
        break;
    }
    Em_SetStateSync(em, 0, state, mode);
}

void EmYamaTsukami_SetGroup1State(Em *em, u16 state, u8 mode)
{
    s32 a;

    em->motionSpeed = 1.0f;
    switch (state) {
    case 0:
        if (em->curGoalKind == 0) {
            em->hasGoal = 0;
            em->goalDist = 500.0f;
        } else {
            em->hasGoal = 1;
            em->goalDist = Vec_DistanceXZ(&em->posX, em->goal);
        }
        break;
    case 1:
        if (em->curGoalKind == 0) {
            em->hasGoal = 0;
            em->goalDist = 500.0f;
        } else {
            em->hasGoal = 1;
            em->goalDist = Vec_DistanceXZ(&em->posX, em->goal);
        }
        a = (u16) (Math_AngleXZ(&em->posX, em->goal) - em->rotY);
        if ((0 <= a && a <= 0x4000) || (a >= 0xC000 && a <= 0xFFFF)) {
            state = 0;
        }
        break;
    case 2:
        break;
    }
    Em_SetStateSync(em, 1, state, mode);
}

void EmYamaTsukami_SetGroup2State(Em *em, u16 state, u8 mode)
{
    EmYamaTsukamiWork *w = (EmYamaTsukamiWork *)&em->work;

    em->motionSpeed = 1.0f;
    switch (state) {
    case 2:
    case 3:
        if (em->remote == 0 && em->curGoalKind == 1 && em->curGoalArg == 0 && em->targetNo == -1) {
            state = 0;
        }
        break;
    case 4:
    case 6:
        break;
    case 17:
        w->easeIdx = 2;
        break;
    case 18:
        w->easeIdx = 2;
        break;
    }
    Em_SetStateSync(em, 2, state, mode);
}

void EmYamaTsukami_SetGroup3State(Em *em, u16 state, u8 mode)
{
    em->motionSpeed = 1.0f;
    Em_SetStateSync(em, 0x3, state, mode);
}

void EmYamaTsukami_SetGroup4State(Em *em, u16 state, u8 mode)
{
    EmYamaTsukamiWork *w = (EmYamaTsukamiWork *)&em->work;

    em->motionSpeed = 1.0f;
    switch (state) {
    case 0:
    case 1:
        w->easeIdx = 0;
        w->unk02E = 0;
        w->unk033 = 30;
        break;
    case 2:
        w->unk02E = 1;
        break;
    case 3:
        w->unk02E = 2;
        break;
    case 4:
        w->easeIdx = 1;
        w->unk02E = 0;
        break;
    case 5:
        w->easeIdx = 0;
        w->unk02E = 0;
        break;
    }
    Em_SetStateSync(em, 4, state, mode);
}

void EmYamaTsukami_ChangeState(Em *em, u16 group, u16 state, u8 mode)
{
    EmYamaTsukamiWork *w = (EmYamaTsukamiWork *)&em->work;

    if (em->remote == 0) {
        Em_TargetPlayerInRange(em);
    } else if (mode == 3 || mode == 4) {
        return;
    }
    if ((group != 0 || state != 1) && (group != 2 || state != 0)) {
        w->unk038 = 0;
    }
    switch (group) {
    case 0:
        EmYamaTsukami_SetGroup0State(em, state, mode);
        break;
    case 1:
        EmYamaTsukami_SetGroup1State(em, state, mode);
        break;
    case 2:
        EmYamaTsukami_SetGroup2State(em, state, mode);
        break;
    case 3:
        EmYamaTsukami_SetGroup3State(em, state, mode);
        break;
    case 4:
        EmYamaTsukami_SetGroup4State(em, state, mode);
        break;
    case 5:
    case 6:
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        break;
    }
}

void EmYamaTsukami_MotionSounds(Em *em, EmWork *work)
{
    s16 motion;

    motion = ((EmYamaTsukamiWork *)work)->unk008;
    switch (motion) {
    case 0x3E9:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x5A, Sound_RandomSe(0xD, 4, 0xF, 4), 3);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 8, Sound_RandomSe(0x65, 4, 0x66, 4), 3, 0x16);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x70, Sound_RandomSe(0x65, 4, 0x67, 4), 3, 0x16);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0xD8, Sound_RandomSe(0x65, 4, 0x66, 4), 3, 0x16);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x140, Sound_RandomSe(0x65, 4, 0x67, 4), 3, 0x16);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xC, Sound_RandomSe(3, 5, 4, 3), 7);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x10, Sound_RandomSe(4, 3, 5, 5), 0xB);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x74, Sound_RandomSe(3, 5, 4, 3), 7);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x78, Sound_RandomSe(4, 3, 5, 5), 0xB);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xDC, Sound_RandomSe(3, 5, 4, 3), 7);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xE0, Sound_RandomSe(4, 3, 5, 5), 0xB);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x144, Sound_RandomSe(3, 5, 4, 3), 7);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x148, Sound_RandomSe(4, 3, 5, 5), 0xB);
        break;
    case 0x3EA:
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 6, Sound_RandomSe(0xD, 4, 0xF, 4), 3, 9);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 6, 0x65, 3, 0x16);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x9E, 0x67, 3, 0x16);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x5C, 0x66, 3, 0x16);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x60, 0x4C, 0x24, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x62, Sound_RandomSe(3, 5, 4, 3), 7);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x68, Sound_RandomSe(4, 3, 5, 5), 0xB);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x5C, Sound_RandomSe(0xD, 4, 0xF, 4), 3, 9);
        break;
    case 0x3EB:
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 6, Sound_RandomSe(0xD, 4, 0xF, 4), 3, 9);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 6, 0x65, 3, 0x16);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x9E, 0x67, 3, 0x16);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x5C, 0x66, 3, 0x16);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x60, 0x4C, 0x24, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x62, Sound_RandomSe(3, 5, 4, 3), 7);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x68, Sound_RandomSe(4, 3, 5, 5), 0xB);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x5C, Sound_RandomSe(0xD, 4, 0xF, 4), 3, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x84, Sound_RandomSe(3, 5, 4, 3), 7);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x88, Sound_RandomSe(4, 3, 5, 5), 0xB);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xF4, Sound_RandomSe(3, 5, 4, 3), 7);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xF8, Sound_RandomSe(4, 3, 5, 5), 0xB);
        break;
    case 0x3EC:
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 6, 0x53, 0x24, 9);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x44, 0x2C, 3, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 0xB, 3);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0xC, 0x67, 3, 0x16);
        break;
    case 0x3ED:
    case 0x3EE:
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0xC, 0x67, 3, 0x16);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x68, 0x66, 3, 0x16);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x6A, 0x2D, 0x10, 0x16);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, Sound_RandomSe(0xC, 4, 0xE, 4), 0x17);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, Sound_RandomSe(0xC, 4, 0xE, 4), 0x1D);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1E, 0x12, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x24, 0x13, 0x20);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x12, Sound_RandomSe(0xC, 4, 0xE, 4), 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2E, 0x13, 0xA);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x24, 0x15, 0xF);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x68, Sound_RandomSe(0xC, 4, 0xE, 4), 0x17);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x7A, Sound_RandomSe(0xC, 4, 0xE, 4), 0x1D);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x82, 0x12, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x88, 0x13, 0x20);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x76, Sound_RandomSe(0xC, 4, 0xE, 4), 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x92, 0x13, 0xA);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x88, 0x15, 0xF);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x18, Sound_RandomSe(3, 5, 4, 3), 7);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1C, Sound_RandomSe(4, 3, 5, 5), 0xB);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x68, Sound_RandomSe(3, 5, 4, 3), 7);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x6C, Sound_RandomSe(4, 3, 5, 5), 0xB);
        break;
    case 0x3EF:
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 6, 0x4C, 0x24, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xC, 0x28, 0x24);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x30, 0x11, 3);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, Sound_RandomSe(3, 5, 4, 3), 7);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xC, Sound_RandomSe(4, 3, 5, 5), 0xB);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x70, Sound_RandomSe(3, 5, 4, 3), 7);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x74, Sound_RandomSe(4, 3, 5, 5), 0xB);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x32, 0x66, 3, 0x16);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x94, 0x65, 3, 0x16);
        break;
    case 0x3F0:
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 4, 0x51, 0x24, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, Sound_RandomSe(0xC, 4, 0xE, 4), 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, Sound_RandomSe(0xC, 4, 0xE, 4), 0x20);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x22, Sound_RandomSe(0xC, 4, 0xE, 4), 0xA);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x26, Sound_RandomSe(0xC, 4, 0xE, 4), 0xF);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0xC, 0x65, 3, 0x16);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x52, 0x65, 3, 0x16);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x54, 0xA, 3);
        break;
    case 0x3F1:
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x4C, 0x4B, 0x24, 9);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x24, 6, 3, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xAC, Sound_RandomSe(0xC, 4, 0xE, 4), 0x17);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xB8, Sound_RandomSe(0xD, 4, 0xF, 4), 0x1D);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xE6, Sound_RandomSe(0xC, 4, 0xE, 4), 8);
        break;
    case 0x3F2:
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 6, 0x4C, 0x24, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 0xB, 3);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x16, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 0x16, 0x20);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x16, 0xA);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 0x16, 0xF);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x10, 0x3F, 0x1A, 5);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x14, 0x3F, 0x20, 5);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x10, 0x3F, 0xA, 5);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x14, 0x3F, 0xF, 5);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x12, 0x13, 0x2A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 0x14, 0x2F);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x42, 0x67, 3, 0x16);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x44, Sound_RandomSe(3, 5, 4, 3), 7);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x48, Sound_RandomSe(4, 3, 5, 5), 0xB);
        break;
    case 0x3F3:
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 4, 0x58, 0x24, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 0xB, 3);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, Sound_RandomSe(3, 5, 4, 3), 7);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xC, Sound_RandomSe(4, 3, 5, 5), 0xB);
        break;
    case 0x3F4:
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 4, 0x46, 0x24, 9);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0xB4, 0x58, 0x24, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xBC, 0x28, 0x24);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0xB4, 0x66, 3, 0x16);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xE, Sound_RandomSe(0xC, 4, 0xE, 4), 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x26, Sound_RandomSe(0xC, 4, 0xE, 4), 0x20);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xD2, 0xA, 0x1D);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x104, 0x13, 0x20);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xF2, Sound_RandomSe(0xC, 4, 0xE, 4), 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3C, 0x13, 0x2A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x58, 0x14, 0x2F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xE, Sound_RandomSe(3, 5, 4, 3), 7);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x12, Sound_RandomSe(4, 3, 5, 5), 0xB);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x84, Sound_RandomSe(3, 5, 4, 3), 7);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x88, Sound_RandomSe(4, 3, 5, 5), 0xB);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xF4, Sound_RandomSe(3, 5, 4, 3), 7);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xF8, Sound_RandomSe(4, 3, 5, 5), 0xB);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xDC, Sound_RandomSe(0xD, 4, 0xF, 4), 0xA);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x10A, Sound_RandomSe(0xD, 4, 0xF, 4), 0xF);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x14, 0x65, 3, 0x16);
        break;
    case 0x3F5:
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 4, 0x52, 0x24, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xE, 0x12, 0x2F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x20, 0x12, 0x2F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xE, Sound_RandomSe(3, 5, 4, 3), 7);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x12, Sound_RandomSe(4, 3, 5, 5), 0xB);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x12, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, Sound_RandomSe(0xD, 4, 0xF, 4), 0x17);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x44, 0x13, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xC, Sound_RandomSe(0xD, 4, 0xF, 4), 0x1D);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x62, 0x15, 0x20);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, Sound_RandomSe(0xD, 4, 0xF, 4), 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x56, 0x13, 0xF);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x12, Sound_RandomSe(0xD, 4, 0xF, 4), 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x60, 0x14, 0xA);
        break;
    case 0x3F6:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xC, Sound_RandomSe(3, 5, 4, 3), 7);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x10, Sound_RandomSe(4, 3, 5, 5), 0xB);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x74, Sound_RandomSe(3, 5, 4, 3), 7);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x78, Sound_RandomSe(4, 3, 5, 5), 0xB);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 8, Sound_RandomSe(0x65, 4, 0x66, 4), 3, 0x16);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x70, Sound_RandomSe(0x65, 4, 0x67, 4), 3, 0x16);
        break;
    case 0x3FC:
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0xC, 0x52, 0x24, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, Sound_RandomSe(0, 5, 1, 3), 7);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, Sound_RandomSe(1, 3, 2, 5), 0xB);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 0x11, 0x24);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x14, 0xB, 0x24);
        break;
    case 0x3FD:
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x34, 0x4C, 0x24, 9);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x48, 6, 3, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, Sound_RandomSe(3, 5, 4, 3), 7);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA, Sound_RandomSe(4, 3, 5, 5), 0xB);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 0xA, 3);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x48, 0x10, 3);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x8E, 0x67, 3, 0x16);
        break;
    case 0x3FE:
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x12, Sound_RandomSe(0x65, 4, 0x66, 4), 3, 0x16);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x12, 0x46, 0x24);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x18, Sound_RandomSe(3, 5, 4, 3), 7);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1C, Sound_RandomSe(4, 3, 5, 5), 0xB);
        break;
    case 0x3FF:
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 4, Sound_RandomSe(0x65, 4, 0x66, 4), 3, 0x16);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x12, 0x46, 0x24, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, Sound_RandomSe(3, 5, 4, 3), 7);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, Sound_RandomSe(4, 3, 5, 5), 0xB);
        break;
    case 0x400:
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 4, 0x47, 0x24, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x32, Sound_RandomSe(0xD, 4, 0xF, 4), 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x44, Sound_RandomSe(0xD, 4, 0xF, 4), 0x20);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 4, Sound_RandomSe(0x65, 4, 0x65, 4), 3, 0x16);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x5C, Sound_RandomSe(0x67, 4, 0x66, 4), 3, 0x16);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x34, Sound_RandomSe(3, 5, 4, 3), 7);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x38, Sound_RandomSe(4, 3, 5, 5), 0xB);
        break;
    case 0x406:
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 4, 0x48, 0x24, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0xA, 0x1D);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x44, 0x13, 0x20);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x40, 0x15, 0x20);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x62, 0xC, 0x17);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA0, 0x13, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x9C, 0x15, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x98, Sound_RandomSe(0, 5, 1, 3), 7);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x9C, Sound_RandomSe(1, 3, 2, 5), 0xB);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x16, Sound_RandomSe(0x65, 4, 0x67, 4), 3, 0x16);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x7A, Sound_RandomSe(0x65, 4, 0x66, 4), 3, 0x16);
        break;
    case 0x407:
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 4, 0x48, 0x24, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 0xA, 0x17);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x44, 0x13, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x40, 0x15, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x62, 0xC, 0x1D);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA0, 0x13, 0x20);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x9C, 0x15, 0x20);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x98, Sound_RandomSe(0, 5, 1, 3), 0xB);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x9C, Sound_RandomSe(1, 3, 2, 5), 7);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x16, Sound_RandomSe(0x65, 4, 0x67, 4), 3, 0x16);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x7A, Sound_RandomSe(0x65, 4, 0x66, 4), 3, 0x16);
        break;
    case 0x408:
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 4, 0x47, 0x24, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0xA, 0x1D);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x6C, 0x10, 0x20);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x68, 0x15, 0x20);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xC8, 0xF, 0x20);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x10, 0x11, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xDC, Sound_RandomSe(3, 5, 4, 3), 7);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xE0, Sound_RandomSe(4, 3, 5, 5), 0xB);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x74, Sound_RandomSe(3, 5, 4, 3), 7);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x78, Sound_RandomSe(4, 3, 5, 5), 0xB);
        break;
    case 0x409:
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 4, 0x47, 0x24, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0xA, 0x17);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x6C, 0x10, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x68, 0x15, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xC8, 0xF, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x10, 0x11, 0x20);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xDC, Sound_RandomSe(3, 5, 4, 3), 7);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xE0, Sound_RandomSe(4, 3, 5, 5), 0xB);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x74, Sound_RandomSe(3, 5, 4, 3), 7);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x78, Sound_RandomSe(4, 3, 5, 5), 0xB);
        break;
    case 0x40A:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, Sound_RandomSe(0xD, 4, 0xF, 4), 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3E, Sound_RandomSe(0xC, 4, 0xE, 4), 0xF);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3A, 0x15, 0xF);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x4E, 0x13, 0xF);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x34, Sound_RandomSe(0xD, 4, 0xF, 4), 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x84, Sound_RandomSe(0xC, 4, 0xE, 4), 0xA);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x80, 0x15, 0xA);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x9C, 0x13, 0xA);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xC2, Sound_RandomSe(0xD, 4, 0xF, 4), 0xA);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xDC, Sound_RandomSe(3, 5, 4, 3), 7);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xE0, Sound_RandomSe(4, 3, 5, 5), 0xB);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x74, Sound_RandomSe(3, 5, 4, 3), 7);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x78, Sound_RandomSe(4, 3, 5, 5), 0xB);
        break;
    case 0x40B:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, Sound_RandomSe(0xD, 4, 0xF, 4), 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3E, Sound_RandomSe(0xC, 4, 0xE, 4), 0xA);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3A, 0x15, 0xA);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x4E, 0x13, 0xA);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x34, Sound_RandomSe(0xD, 4, 0xF, 4), 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x84, Sound_RandomSe(0xC, 4, 0xE, 4), 0xF);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x80, 0x15, 0xF);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x9C, 0x13, 0xF);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xC2, Sound_RandomSe(0xD, 4, 0xF, 4), 0xF);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xDC, Sound_RandomSe(3, 5, 4, 3), 7);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xE0, Sound_RandomSe(4, 3, 5, 5), 0xB);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x74, Sound_RandomSe(3, 5, 4, 3), 7);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x78, Sound_RandomSe(4, 3, 5, 5), 0xB);
        break;
    case 0x40C:
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x16, 0x47, 0x24, 9);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x7A, 0x48, 0x24, 9);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 4, 0x67, 3, 0x16);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0xE4, 0x65, 3, 0x16);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 0xA, 3);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x22, Sound_RandomSe(0xD, 4, 0xF, 4), 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x26, Sound_RandomSe(0xD, 4, 0xF, 4), 0x20);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x9E, 0x17, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA2, 0x17, 0x20);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA0, 0x15, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA4, 0x15, 0x20);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA2, 0x17, 0x2A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x9E, 0x17, 0x2F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xE0, Sound_RandomSe(3, 5, 4, 3), 7);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xE2, Sound_RandomSe(4, 3, 5, 5), 0xB);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xAE, 0x44, 0x1D);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xB8, 9, 0x17);
        break;
    case 0x40D:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x14, Sound_RandomSe(0xD, 4, 0xF, 4), 0x1D);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x64, Sound_RandomSe(0xC, 4, 0xE, 4), 0x20);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x5A, Sound_RandomSe(0xD, 4, 0xF, 4), 0x17);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x16, 0x65, 3, 0x16);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x7A, 0x65, 3, 0x16);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, Sound_RandomSe(3, 5, 4, 3), 7);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, Sound_RandomSe(4, 3, 5, 5), 0xB);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x68, Sound_RandomSe(3, 5, 4, 3), 7);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x6A, Sound_RandomSe(4, 3, 5, 5), 0xB);
        break;
    case 0x40E:
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 6, 0x56, 0x24, 9);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x90, 0x27, 0x24, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, Sound_RandomSe(0xD, 4, 0xF, 4), 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA, Sound_RandomSe(0xD, 4, 0xF, 4), 0x20);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x14, Sound_RandomSe(0xD, 4, 0xF, 4), 0xA);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x18, Sound_RandomSe(0xD, 4, 0xF, 4), 0xF);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x28, 0x67, 3, 0x16);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x90, 0x67, 3, 0x16);
        break;
    case 0x40F:
    case 0x41A:
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 6, 0x58, 0x24, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 0x11, 3);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, Sound_RandomSe(3, 5, 4, 3), 7);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, Sound_RandomSe(4, 3, 5, 5), 0xB);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x26, 0x15, 0x1A, 6);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x2C, 0x15, 0x20, 6);
        break;
    case 0x410:
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x16, 0x4E, 0x24, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x6A, 0x27, 0x24);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xDE, 0x28, 0x24);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x104, 0x28, 0x24);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x88, 0x27, 0x2A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x8E, 0x27, 0x2F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xE0, 0x27, 0x2A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xE4, 0x27, 0x2F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x11C, 0x27, 0x2A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x120, 0x27, 0x2F);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x13A, 0x3C, 0x24, 9);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x102, 0x3C, 0x24, 9);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0xEE, 0x3E, 0x24, 9);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x128, 0x3E, 0x24, 9);
        break;
    case 0x411:
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 6, 0x58, 0x24, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 0x11, 3);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, Sound_RandomSe(3, 5, 4, 3), 7);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, Sound_RandomSe(4, 3, 5, 5), 0xB);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x74, Sound_RandomSe(3, 5, 4, 3), 7);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x78, Sound_RandomSe(4, 3, 5, 5), 0xB);
        break;
    case 0x412:
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 4, 0x4D, 0x24, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3A, 0x16, 0x2A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3A, 0x16, 0x2F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3E, 0x16, 0x2A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3A, 0x16, 0x2F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x4C, 9, 0x2A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x48, 9, 0x2F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 0xA, 3);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 6, Sound_RandomSe(0x66, 4, 0x67, 4), 3, 0x16);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x5E, Sound_RandomSe(3, 5, 4, 3), 7);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x62, Sound_RandomSe(4, 3, 5, 5), 0xB);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x60, 0x65, 3, 0x16);
        break;
    case 0x413:
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 4, 0x4D, 0x24, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x34, 0x16, 0x2A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x38, 0x16, 0x2F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x34, 0x16, 0x2A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x38, 0x16, 0x2F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 0xA, 3);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 6, Sound_RandomSe(0x66, 4, 0x67, 4), 3, 0x16);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3C, Sound_RandomSe(3, 5, 4, 3), 7);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3C, Sound_RandomSe(4, 3, 5, 5), 0xB);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x42, 0x65, 3, 0x16);
        break;
    case 0x414:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x12, 9, 0x2A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x14, 8, 0x2F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x66, Sound_RandomSe(3, 5, 4, 3), 7);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x6A, Sound_RandomSe(4, 3, 5, 5), 0xB);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x10, 0x65, 3, 0x16);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x12, 0xA, 3);
        break;
    case 0x415:
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 4, 0x4C, 0x24, 9);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x82, 0x46, 0x24, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0xB, 3);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x6C, Sound_RandomSe(0xD, 4, 0xF, 4), 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x70, Sound_RandomSe(0xD, 4, 0xF, 4), 0x20);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x3C, 0x67, 3, 0x16);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x98, 0x67, 3, 0x16);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1E, Sound_RandomSe(3, 5, 4, 3), 7);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x20, Sound_RandomSe(4, 3, 5, 5), 0xB);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x8E, Sound_RandomSe(3, 5, 4, 3), 7);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x90, Sound_RandomSe(4, 3, 5, 5), 0xB);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xFE, Sound_RandomSe(3, 5, 4, 3), 7);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x100, Sound_RandomSe(4, 3, 5, 5), 0xB);
        break;
    case 0x416:
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 4, 0x47, 0x24, 9);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x1E, Sound_RandomSe(0x66, 4, 0x66, 4), 3, 0x16);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x10, 3);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x34, Sound_RandomSe(0xC, 4, 0xE, 4), 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x30, Sound_RandomSe(0xC, 4, 0xE, 4), 0x20);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x32, Sound_RandomSe(0xC, 4, 0xE, 4), 0xA);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2E, Sound_RandomSe(0xC, 4, 0xE, 4), 0xF);
        break;
    case 0x417:
        Em_PlaySeAtBoneOnFrame((Em *) em, 2, 0x12, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x12, 0x12, 0x20);
        Em_PlaySeAtBoneOnFrame((Em *) em, 2, 0x14, 0xF);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x12, 0x14, 0x20);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x12, 0x27, 0x2F);
        break;
    case 0x418:
        Em_PlaySeAtBoneOnFrame((Em *) em, 2, 0x13, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x12, 0x13, 0x20);
        Em_PlaySeAtBoneOnFrame((Em *) em, 2, 0x15, 0xF);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x12, 0x15, 0x20);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 4, 0x58, 0x24, 9);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x1E, Sound_RandomSe(0x66, 4, 0x66, 4), 3, 0x16);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x10, 3);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, Sound_RandomSe(3, 5, 4, 3), 7);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA, Sound_RandomSe(4, 3, 5, 5), 0xB);
        break;
    case 0x419:
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 6, 0x56, 0x24, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, Sound_RandomSe(0xD, 4, 0xF, 4), 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA, Sound_RandomSe(0xD, 4, 0xF, 4), 0x20);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x14, Sound_RandomSe(0xD, 4, 0xF, 4), 0xA);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x18, Sound_RandomSe(0xD, 4, 0xF, 4), 0xF);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x8E, 0x28, 0x24, 9);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x28, 0x67, 3, 0x16);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x90, 0x67, 3, 0x16);
        break;
    case 0x41B:
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, Sound_RandomSe(0xC, 4, 0xE, 4), 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA, Sound_RandomSe(0xC, 4, 0xE, 4), 0x20);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xC, Sound_RandomSe(0xC, 4, 0xE, 4), 0xA);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, Sound_RandomSe(0xC, 4, 0xE, 4), 0xF);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x12, 0x2A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 0x14, 0x2F);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x28, 0x66, 3, 0x16);
        break;
    case 0x41C:
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x30, 0x57, 0x24, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0xA, 3);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xD0, 0xB, 3);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x38, Sound_RandomSe(0xD, 4, 0xF, 4), 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3C, Sound_RandomSe(0xD, 4, 0xF, 4), 0x20);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x46, Sound_RandomSe(0xD, 4, 0xF, 4), 0xA);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x4A, Sound_RandomSe(0xD, 4, 0xF, 4), 0xF);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0xA0, 0x31, 0xB, 9);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0xA8, 0x31, 7, 9);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x28, 0x67, 3, 0x16);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x68, 0x65, 3, 0x16);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0xDE, 0x66, 3, 0x16);
        break;
    case 0x41D:
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x2C, 0x47, 0x24, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, Sound_RandomSe(0xD, 4, 0xF, 4), 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA, Sound_RandomSe(0xD, 4, 0xF, 4), 0x20);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xC, Sound_RandomSe(0xD, 4, 0xF, 4), 0xA);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, Sound_RandomSe(0xD, 4, 0xF, 4), 0xF);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x44, 0xC, 3);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x46, 0xB, 3);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x7E, 7, 3, 9);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x28, 0x67, 3, 0x16);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x84, 6, 3, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x86, 8, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x8A, 8, 0x20);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x88, 9, 0xA);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x84, 9, 0xF);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xB4, Sound_RandomSe(0, 5, 1, 3), 7);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xB8, Sound_RandomSe(1, 3, 2, 5), 0xB);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x12C, Sound_RandomSe(3, 5, 4, 3), 7);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x130, Sound_RandomSe(4, 3, 5, 5), 0xB);
        break;
    case 0x41E:
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x44, 0x46, 0x24, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 0xB, 3);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, Sound_RandomSe(0xD, 4, 0xF, 4), 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA, Sound_RandomSe(0xD, 4, 0xF, 4), 0x20);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xC, Sound_RandomSe(0xD, 4, 0xF, 4), 0xA);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, Sound_RandomSe(0xD, 4, 0xF, 4), 0xF);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x54, 0x67, 3, 0x16);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, Sound_RandomSe(3, 5, 4, 3), 7);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, Sound_RandomSe(4, 3, 5, 5), 0xB);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x72, Sound_RandomSe(3, 5, 4, 3), 7);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x76, Sound_RandomSe(4, 3, 5, 5), 0xB);
        break;
    case 0x424:
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 4, 0x49, 0x24, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, Sound_RandomSe(0xD, 6, 0xF, 2), 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA, Sound_RandomSe(0xD, 2, 0xF, 6), 0x20);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 8, Sound_RandomSe(0x66, 4, 0x67, 4), 3, 0x16);
        break;
    case 0x425:
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 4, 0x4A, 0x24, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, Sound_RandomSe(0xC, 4, 0xE, 4), 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA, Sound_RandomSe(0xD, 4, 0xF, 4), 0x20);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x46, 0x11, 3);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 8, Sound_RandomSe(0x66, 4, 0x67, 4), 3, 0x16);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x5C, Sound_RandomSe(0x66, 4, 0x67, 4), 3, 0x16);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x4A, Sound_RandomSe(3, 5, 4, 3), 7);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x4E, Sound_RandomSe(4, 3, 5, 5), 0xB);
        break;
    case 0x426:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x4A, 0x24);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 8, Sound_RandomSe(0x66, 4, 0x67, 4), 3, 0x16);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, Sound_RandomSe(0xE, 4, 0x10, 4), 0x20);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, Sound_RandomSe(0xD, 4, 0xF, 4), 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x54, 0x13, 0x2A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x58, 0x14, 0x2F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x66, 9, 0x2A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x6A, 9, 0x2F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, Sound_RandomSe(3, 5, 4, 3), 7);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1A, Sound_RandomSe(4, 3, 5, 5), 0xB);
        break;
    case 0x427:
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 4, 0x4B, 0x24, 9);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0xDC, 0x46, 0x24, 9);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0xB2, 6, 3, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA2, 0x12, 0x2A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xB0, 9, 0x2A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xB6, 0x14, 0x2F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xC2, 8, 0x2F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x11, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x92, Sound_RandomSe(0xD, 6, 0xF, 2), 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x136, Sound_RandomSe(0xD, 2, 0xF, 6), 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x132, Sound_RandomSe(0xD, 6, 0xF, 2), 0xA);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x136, Sound_RandomSe(0xD, 2, 0xF, 6), 0xF);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xC, 0x11, 0x20);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xF6, Sound_RandomSe(0xD, 6, 0xF, 2), 0x20);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x150, 8, 0x20);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0xC, 0x67, 3, 0x16);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x54, 0x65, 3, 0x16);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0xAC, 0x66, 3, 0x16);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x4A, Sound_RandomSe(3, 5, 4, 3), 7);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x4E, Sound_RandomSe(4, 3, 5, 5), 0xB);
        break;
    case 0x428:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x49, 0x24);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, Sound_RandomSe(0xE, 4, 0x10, 4), 0x20);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 8, Sound_RandomSe(0x66, 4, 0x67, 4), 3, 0x16);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x52, Sound_RandomSe(0xD, 4, 0xF, 4), 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, Sound_RandomSe(3, 5, 4, 3), 7);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xC, Sound_RandomSe(4, 3, 5, 5), 0xB);
        break;
    case 0x429:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x49, 0x24);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, Sound_RandomSe(0xE, 4, 0x10, 4), 0x1A);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 8, Sound_RandomSe(0x66, 4, 0x67, 4), 3, 0x16);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x52, Sound_RandomSe(0xD, 4, 0xF, 4), 0x20);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, Sound_RandomSe(3, 5, 4, 3), 7);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xC, Sound_RandomSe(4, 3, 5, 5), 0xB);
        break;
    case 0x42A:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x49, 0x24);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3E, 0x12, 0x2F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x54, 0x13, 0x2F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, Sound_RandomSe(0xC, 4, 0xE, 4), 3);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0xC, 0x67, 3, 0x16);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x22, Sound_RandomSe(3, 5, 4, 3), 7);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x26, Sound_RandomSe(4, 3, 5, 5), 0xB);
        break;
    case 0x42B:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x4B, 0x24);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, Sound_RandomSe(0xC, 4, 0xE, 4), 0x17);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x18, Sound_RandomSe(0xC, 4, 0xE, 4), 0x1D);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x30, Sound_RandomSe(3, 5, 4, 3), 7);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x34, Sound_RandomSe(4, 3, 5, 5), 0xB);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 8, Sound_RandomSe(0x67, 4, 0x67, 4), 3, 0x16);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x4E, Sound_RandomSe(0x66, 4, 0x67, 4), 3, 0x16);
        
    default:
        break;
    }
}

/* em_kushala's .data (0x8CA100-0x8CA9A8) */
EmFlyParam EmKushala_FlyParam = {
#include "game/008CA100.inc"
};
EmFlyParam EmKushala_FlyParam2 = {
#include "game/008CA160.inc"
};
u16 EmKushala_IdleTbl[24] = {
#include "game/008CA1C0.inc"
};
EmTurnParams EmKushala_TurnParams[1] = {
#include "game/008CA1F0.inc"
};
s32 GmEm_ReturnZero(void); /* gm_sub */
s32 EmKushala_State3_0FTimers[6][2] = {
#include "game/008CA210.inc"
};
void *EmKushala_Callbacks[8] = {
    EmKushala_SpawnEffects, EmKushala_SpawnEffects, EmKushala_TableNop, EmKushala_UpdateMotionEffects,
    GmEm_ReturnZero, EmKushala_Reinit, EmKushala_AllowHit8, NULL,
};
f32 EmKind38_VelLimits[6] = {
#include "game/008CA260.inc"
};
u16 EmKind38_Effect02Chance[4] = {
#include "game/008CA278.inc"
};
EmKPair EmKind38_State3Pts[2][2] = {
#include "game/008CA280.inc"
};
EmKVec EmKind38_State2_08Accel[4] = {
#include "game/008CA2A0.inc"
};
void *EmKind38_Callbacks[8] = {
    EmKind38_SpawnEffects, EmKind38_SpawnEffects, EmKind38_TableNop, EmKind38_UpdateMotionEffects,
    GmEm_ReturnZero, EmKind38_Reinit, NULL, NULL,
};
f32 EmKind38_HeightRange[2][2] = {
#include "game/008CA2F0.inc"
};
f32 EmKind38_SpawnOfs[4] = {
#include "game/008CA300.inc"
};
f32 EmShakalaka_NearDist[4] = {
#include "game/008CA310.inc"
};
EmKVec EmShakalaka_State3_03Vel[4] = {
#include "game/008CA320.inc"
};
void *EmShakalaka_Callbacks[8] = {
    EmShakalaka_SpawnEffect01, EmShakalaka_SpawnEffect01, EmShakalaka_TableNop, EmShakalaka_UpdateMotionEffects,
    EmShakalaka_ScriptQuery, EmShakalaka_Reinit, NULL, NULL,
};
EmCurveKey EmYamaTsukami_VelYCurves_0[10] = {
#include "game/008CA370.inc"
};
EmCurveKey EmYamaTsukami_VelYCurves_1[10] = {
#include "game/008CA3C0.inc"
};
EmCurveKey EmYamaTsukami_VelYCurves_2[12] = {
#include "game/008CA410.inc"
};
EmCurveKey EmYamaTsukami_VelYCurves_3[12] = {
#include "game/008CA470.inc"
};
EmCurveKey EmYamaTsukami_VelYCurves_6[12] = {
#include "game/008CA4D0.inc"
};
EmCurveKey EmYamaTsukami_VelYCurves_7[12] = {
#include "game/008CA530.inc"
};
EmCurveKey EmYamaTsukami_VelYCurves_8[18] = {
#include "game/008CA590.inc"
};
EmCurveKey EmYamaTsukami_VelYCurves_9[18] = {
#include "game/008CA620.inc"
};
EmCurveKey EmYamaTsukami_VelYCurves_10[18] = {
#include "game/008CA6B0.inc"
};
EmCurveKey EmYamaTsukami_VelYCurves_11[18] = {
#include "game/008CA740.inc"
};
EmCurveKey EmYamaTsukami_VelYCurves_12[18] = {
#include "game/008CA7D0.inc"
};
EmCurveKey EmYamaTsukami_SpeedCurves_4[12] = {
#include "game/008CA860.inc"
};
EmCurveKey EmYamaTsukami_SpeedCurves_5[14] = {
#include "game/008CA8C0.inc"
};
EmCurveKey *EmYamaTsukami_VelYCurves[16] = {
    EmYamaTsukami_VelYCurves_0, EmYamaTsukami_VelYCurves_1, EmYamaTsukami_VelYCurves_2, EmYamaTsukami_VelYCurves_3,
    NULL, NULL, EmYamaTsukami_VelYCurves_6, EmYamaTsukami_VelYCurves_7,
    EmYamaTsukami_VelYCurves_8, EmYamaTsukami_VelYCurves_9, EmYamaTsukami_VelYCurves_10, EmYamaTsukami_VelYCurves_11,
    EmYamaTsukami_VelYCurves_12, NULL, NULL, NULL,
};
EmCurveKey *EmYamaTsukami_SpeedCurves[14] = {
    NULL, NULL, NULL, NULL,
    EmYamaTsukami_SpeedCurves_4, EmYamaTsukami_SpeedCurves_5, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL,
};
