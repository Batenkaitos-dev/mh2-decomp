/*
 * Monster family: Kushala Daora, its rusted form, Lunastra and Teostra
 * (EmKushala_*; include/em_kushala.h names their motions and states): elder
 * dragons that walk, hover low (stance 1) and fly, and raise a wind
 * (Kushala Daora) or an aura (Lunastra and Teostra, "Tesca" in the code)
 * that changes their hitzones. EmKushala_Update turns the damage into
 * reactions and runs the state functions through the Run*State
 * dispatchers. EmKushala_ChangeState, the Set*State functions, the wind and
 * the flight curves come earlier in the game's code, before Shen Gaoren's,
 * and have a file of their own. The file starts with the motion sounds
 * (EmKushala_MotionSounds, MotionSoundsTesca). The state functions take the
 * monster and its work area (EmKushalaWork, the family's view of EmWork);
 * Em.step is the step within the current state.
 */
#include "common.h"
#include "scene.h"
#include "quest.h"
#include "em.h"
#include "em_kushala.h"
#include "player.h"
#include "stageobj.h"
#include "objattach.h"
#include "area.h"
/* The functions this file calls */
void EmKushala_MotionSounds(void *arg0, EmWork *work);
void EmKushala_MotionSoundsTesca(Em *em, EmWork *work);

/* prototypes */
void EmKushala_ChangeState(Em *em, u16 group, u16 state, u8 mode);
StageObj *MonsterFx_CreateAtYaw(f32 *pos, u16 v, u8 k, f32 scale);
void ElementBurst_CreateOnEm(Em *em, u8 k, u8 c, f32 scale);
void EmKushala_RunIdleState(Em *em, EmWork *work);
void EmKushala_RunMoveState(Em *em, EmWork *work);
void EmKushala_EatNeedStart(Em *em, EmWork *work, s32 variant);
void EmKushala_Roar(Em *em, EmWork *work);
void EmKushala_RoarB(Em *em, EmWork *work);
void EmKushala_TurnAlert(Em *em, EmWork *work);
void EmKushala_Run(Em *em, EmWork *work, s32 variant);
void EmKushala_SideDash(Em *em, EmWork *work, s32 variant);
void EmKushala_HoverTurn(Em *em, EmWork *work);
void EmKushala_BreathAttack(Em *em, EmWork *work, s32 variant);
void EmKushala_BlastRingFort(Em *em, EmWork *work);
void EmKushala_RunAttackState(Em *em, EmWork *work);
void EmKushala_StartCurves(Em *em, u8 kind);
u8 EmKushala_UpdateCurves(Em *em);
s8 EmMine_CreateTimed0(Em *em, f32 *pos);
void Em_EndStatus5(Em *em);
void Em_PlaySeAtBoneOnFrame(Em *em, s32 frame, s32 se, s32 bone);
void Em_SetLayerMotion(Em *em, s32 motion, s32 blend, s32 frame, s32 layer);
void Em_SetMotion(Em *em, s32 motion, s32 a, s32 b);
void Em_SetMotionRow(Em *em, s32 row, s32 arg2, s32 arg3);
u16 Math_AngleXZ(f32 *from, f32 *to);
f32 Math_Sin(f32 x);
void Player_ClearFlag(void *, u32 flag);
void EmImpactAttack_CreateAt(Em *em, f32 *pos, s8 kind);
/* unit's own declaration: the last two arguments are declared s32; with the definition's u16 the compiler masks them to 16 bits before the jump to it, which EmKushala_CreateBreathAtBone doesn't do */
void KushalaBreath_CreateAtBone(Em *, s32, u8, u8, s32, s32);
void KushalaAttack_Create(Em *, s32);
void EmEffectAttack_CreateChain(Em *, s32, s32, s32, s32);
void TeostraAttack_Create(Em *, s32);
/* unit's own declaration: the kind is declared s32; with the definition's u8 the compiler masks it to a byte before the call, which EmKushala_BlastRing doesn't do */
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
/* unit's own declaration: the arguments are declared s32; with the definition's s16 the compiler narrows them before the call, which EmKushala_Rest doesn't do */
s32 Em_RegenHp(Em *em, s32 n, s32 max, s32 interval);
void Em_PlaySeAtBoneVolOnFrame(Em *em, s32 frame, s32 se, s32 bone, s32 vol);
void Em_UpdateFlight(Em *em, s32 flags, EmFlyParam *p);
s32 Em_StepToGoal(Em *em, u16 step, u8 flags);
u8 Em_TurnToGoal(Em *em, u16 step, u8 flags);
void Em_StartTurnToTarget(Em *em, EmTurnParams *t);
s32 Em_TurnStep(Em *em, EmTurnParams *t);
void Em_RestartScript(Em *em);
void Em_TargetPlayerInRange(Em *);
s32 EmKushala_WindActive(Em *);
void EmKushala_Init(Em *);
void EmKushala_ToIdle(Em *);
void EmKushala_Land(Em *, EmWork *, s32);
void EmKushala_LeaveArea(Em *, EmWork *);
void EmKushala_EnterAreaAndLand(Em *, EmWork *);
void EmKushala_CrossArea(Em *, EmWork *);
void EmKushala_RiseToHeight(Em *, EmWork *, s32);
void EmKushala_StepForward(Em *, EmWork *);
void EmKushala_Leave(Em *, EmWork *);
void EmKushala_LeaveFromAir(Em *, EmWork *);
void EmKushala_SceneWalkIn(Em *);
void EmKushala_SceneRise(Em *, EmWork *);
void EmKushala_Captured(Em *);
void EmKushala_SceneDescend(Em *, EmWork *);
void EmKushala_SceneFlyIn(Em *, EmWork *);
void EmKushala_SceneWake(Em *);
void EmKushala_Die(Em *, EmWork *);
void EmKushala_DieFalling(Em *, EmWork *);
s32 EmKushala_StateHasWindRing(Em *);
void EmKushala_MotionEffects(Em *, EmWork *);
void EmKushala_CreateAttack(Em *em, s32 arg1);
void EmKushala_SpawnBlastBurst(Em *);
void AlertMark_Create(void *arg, s16 v, u8 k);
void EmRangedAttack_CreateAtBoneTimed(Em *, s32, s32, s32, s32, s32, s32);
/* unit's own declaration: the arguments are declared s32; with the definition's s16 the compiler narrows them before the call, which EmKushala_SpawnK14NearGround doesn't do */
void DustCloud_Create(Em *em, s32, s32, f32);
/* unit's own declaration: the arguments are declared s32; with the definition's u8 and s16 the compiler narrows them before the call, which EmKushala_Stun doesn't do */
void StunStars_CreateEm(Em *, s32, s32, s32, f32, f32);
StageObj *MonsterFx_CreateOnEm(Em *em, u8 k, f32 scale);
/* unit's own declaration: the third argument is declared s16 and the fourth u16, as the callers pass them: with the definition's u16 and s16 the yaw 0xF1C8 of EmKushala_BreathAttack would be passed sign-extended */
void MonsterFx_CreateOnEmAngled(Em *em, s8, s16, u16, f32);
void TeostraBlastCharge_Create(Em *em);
s32 Sound_RandomSe(s32 id, s16 lo, s32 alt, s16 n);
s32 EmMotion_PassedFrameScaled(void *p, f32 frame, s32 layer);
s32 EmMotion_ReachedFrame(void *obj, s32 a, f32 frame);
s32 EmMotion_InFrameRange(void *em, s32 layer, f32 from, f32 to);
void Vec_RotateY(f32 *v, f32 angle);
f32 Vec_Distance(f32 *a, f32 *b);
f32 Vec_DistanceXZ(f32 *a, f32 *b);
s32 QuestEm_CanAppear(Em *em);
void Player_ClearVelocity(void *);
void Player_ClearAccel2(void *);
/* the calls that mask the value again (u8 or & 0xFF) cast it to s32 first: MWCC keeps the game's andi that way */
u8 Player_InCurrentArea(void *obj);
void Shadow_Spawn(void *obj, s16 type);
void PartFx_SpawnOnGround(f32 *pos, s32 a, f32 scale);
/* unit's own declaration: the bone is declared s32; with the definition's s16 the compiler narrows it before the call, which the ground-effect functions don't do */
void Em_BonePos(void *obj, s32 a, f32 *out);
extern EmFlyParam EmKushala_FlyParam;
extern EmFlyParam EmKushala_FlyParam2;
extern EmTurnParams EmKushala_TurnParams[1];
void Vec_Set(f32 *out, f32 x, f32 y, f32 z);
void Em_ClearPoison(Em *em);
void Em_SetCaptured(Em *em, s32 a);
void Em_BeginStatus1(Em *em);
void QuestEm_Captured(Em *em);
void Em_SpawnLyingPuffs(Em *em, s32 bone, f32 *pos, f32 scale);
EmBone *Em_Bone(void *obj, s16 bone);
void Mtx_GetTrans(f32 *out, f32 *m);
void Vec_RotateSelf(f32 *v, f32 *m);
void HitSpark_SpawnAt(f32 *pos, s32 a, u16 b);
void HitSplash_CreateAtScaled(f32 *pos, u8 k, f32 scale);
f32 *Mtx_FromAnglesYXZ(s32 *angles, f32 *m);
void Vec_Rotate(f32 *out, f32 *in, f32 *mtx);
extern u16 EmKushala_IdleTbl[];
void QuestEm_Hunted(Em *em);
s32 Save_TestEventFlag(s32);
u16 Area_IdForCurrentTime(u16);
void EmMine_Remove(Em *em);
s32 EmMine_Update(Em *em);
s8 EmMine_Create(Em *em, s32 idx);
void CamShake_ReqAIf(Em *obj, s32 i);
void Em_ApplyPartBreaks(Em *em);
void Em_FlagPartBreak(Em *em, s32 arg1);
s32 Em_ComputeMaxHp(Em *em, s16 idx);
typedef struct { f32 x, y; } EmKPair;
extern s32 EmKushala_BreathTimers[][2];
extern s32 EmKushala_BreathCounts[][2];
void FallingDebris_Create(Em *em, u8 k);
void HitSpark_SpawnForOwner(void *owner, s32 ang, s32 type, s32 sub, f32 *pos, f32 scale);
void PartFx_SpawnAt(f32 *pos, s32 a, f32 scale);
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
u8 Em_UpdateDamage(Em *em, u8 *out);
void Em_SetStatus2Timer(Em *em);
void Em_SetStatus4Timer(Em *em);
void Em_SetStatus5Timer(Em *em);
void Em_UpdatePendingScript(Em *em);
void Em_ClearHitReactMode(Em *em);
void Em_SetPartScaled(Em *em, s32 bit);
void QuestEm_LoadPersistentHp(Em *em);
u8 Quest_FindSpecialMonster(u8 id);
s32 Em_PartBroken(Em *em, s32 arg1);
void EmTail_Create(Em *);
s32 Em_UpdateCombatTimer(Em *em);
void Em_EndStatus5(Em *em);
s32 EmKushala_WindActiveTesca(Em *);
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
s32 Quest_GetOutcome(s32 withLoss);
void Em_PlaySeTypeAtBoneInFrames(Em *em, s32 frame, s32 frame2, s32 se, s32 bone, s32 vol, s32 a);
void Em_PlaySeTypeAtBoneOnFrame(Em *em, s32 frame, s32 se, s32 bone);
/* end of prototypes */

void EmKushala_MotionSounds(void *em, EmWork *work)
{
    s16 motion;

    motion = work->fxMotion;
    switch (motion) {
    case 0x3EA:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x24, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x4E, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x84, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xB2, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3E, 4, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x62, 4, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x96, 4, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xC4, 4, 8);
        break;
    case 0x3ED:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xC, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x28, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x42, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 3, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x24, 3, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x40, 4, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x48, 4, 0xD);
        break;
    case 0x3EE:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xC, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x28, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x42, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 3, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x24, 3, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x40, 4, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x48, 4, 8);
        break;
    case 0x3EF:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x16, 2);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x26, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x22, 3, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x30, 3, 8);
        break;
    case 0x3F0:
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 0x32, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x15, 0x25);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2C, 0x15, 0x11);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 3, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x28, 2, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2C, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x26, 5, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2A, 3, 8);
        break;
    case 0x3F1:
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 0x32, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x15, 0x25);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2C, 0x15, 0x11);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 3, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x28, 2, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2C, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x26, 5, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2A, 3, 0xD);
        break;
    case 0x3F2:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x32, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x26, 0x27, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2C, 0x14, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x14, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x26, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xE, 4, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1A, 3, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x20, 0xF, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x24, 0xF, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x28, 0x16, 2);
        break;
    case 0x3F3:
        Em_PlaySeAtBoneOnFrame((Em *) em, 2, 0xB, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 0xB, 0x32);
        break;
    case 0x3F4:
    case 0x404:
    case 0x405:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xD, 0x1A, 0x25);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x12, 0x19, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x17, 0x19, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2C, 0x19, 0x25);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3A, 0x19, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x41, 0x19, 0x2E);
        break;
    case 0x3F5:
        Em_PlaySeTypeAtBoneOnFrame((Em *) em, 0x28, 0x4C, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 2, 0xF, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 0xF, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x32, 0x1C, 0x25);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x34, 0x1D, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3A, 0x1D, 0x2E);
        break;
    case 0x3F6:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xC, 0xD, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x10, 0xD, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x38, 0xB, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3C, 0xB, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2C, 0x15, 0x11);
        break;
    case 0x3F7:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x28, 0xE, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2C, 0xE, 0x32);
        break;
    case 0x3F8:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x26, 0xE, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2A, 0xE, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 2, 0x1A, 0x25);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 0x19, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 0x19, 0x2E);
        break;
    case 0x3F9:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3C, 0x32, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x4A, 7, 2);
        Em_PlaySeAtBoneOnFrame((Em *) em, 2, 0xF, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 0xF, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x36, 0xD, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3A, 0xD, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x46, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x42, 2, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x48, 5, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x44, 4, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x7C, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x8E, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x92, 5, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x8A, 4, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xAE, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xC4, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xC2, 3, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xB8, 4, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x4C, 0x16, 2);
        break;
    case 0x3FA:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x31, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 0x14, 0x25);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0xD, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 0xD, 0x32);
        break;
    case 0x3FB:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x32, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 0x14, 0x25);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0xB, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 0xB, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0xD, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 0xD, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x16, 2);
        break;
    case 0x3FC:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x32, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 0x14, 0x25);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0xB, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 0xB, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0xD, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 0xD, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x16, 2);
        break;
    case 0x3FD:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x18, 0x26, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA, 2, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x20, 2, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x32, 2, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3E, 2, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 5, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x30, 5, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x38, 5, 0xD);
        break;
    case 0x3FE:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x10, 0x28, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1A, 2, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3E, 2, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x20, 5, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 2, 0x15, 0xD);
        break;
    case 0x3FF:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x14, 2, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1C, 2, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x22, 5, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 2, 0x15, 0xD);
        break;
    case 0x400:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x31, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1A, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x28, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1E, 3, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2C, 4, 0xD);
        break;
    case 0x401:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x31, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x28, 0x23, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x10, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1E, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x14, 4, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x28, 0xF, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2C, 0xF, 0x32);
        break;
    case 0x402:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1C, 7, 2);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1A, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1E, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 5, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x18, 3, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1C, 0xC, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x20, 0xC, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x24, 0x16, 2);
        break;
    case 0x403:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xE, 0xD, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x14, 0xD, 0x32);
        break;
    case 0x406:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x24, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2A, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x18, 5, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x24, 3, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 0xE, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1A, 0xE, 0x32);
        break;
    case 0x407:
        Em_PlaySeTypeAtBoneOnFrame((Em *) em, 0xA, 0x47, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x14, 2, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x70, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x14, 4, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0xE, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA, 0xE, 0x2E);
        break;
    case 0x408:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x20, 0x26);
        Em_PlaySeTypeAtBoneInFrames((Em *) em, 0x3C, 0xBE, 0x4A, 0x26, 7, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 0xF, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA, 0xF, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x60, 0xE, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x64, 0xE, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xBC, 8, 2);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xB8, 9, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xC0, 2, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xF2, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xF8, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x12, 4, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 3, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xF6, 3, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xBA, 0x16, 2);
        break;
    case 0x40A:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x31, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x8A, 0x32, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x16, 2);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x12, 2, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x9E, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xCA, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x10, 5, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1E, 4, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xB4, 3, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x84, 4, 0xD);
        break;
    case 0x40B:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x36, 0x3A, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x64, 0x39, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1A, 0x32, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x14, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x14, 4, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1A, 4, 8);
        break;
    case 0x40C:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x3B, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x14, 4, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1A, 4, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x28, 1, 0x1A);
        break;
    case 0x40D:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x2F, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x54, 0x2F, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x40, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x60, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x80, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x36, 4, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x58, 3, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 0x12, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x10, 0x16, 2);
        break;
    case 0x40E:
        Em_PlaySeTypeAtBoneOnFrame((Em *) em, 4, 0x56, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xE0, 0x3B, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x20, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x6A, 0x10, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1E, 3, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x46, 4, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x7C, 0xE, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x80, 0xE, 0x2E);
        break;
    case 0x40F:
        Em_PlaySeTypeAtBoneOnFrame((Em *) em, 4, 0x63, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x46, 0x32, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x4A, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x6E, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x84, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x62, 3, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x80, 4, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x30, 0xE, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x34, 0xE, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x6C, 0x17, 2);
        break;
    case 0x416:
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 0x20, 0x26);
        Em_PlaySeTypeAtBoneOnFrame((Em *) em, 0x40, 0x4C, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x4A, 0x18, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1E, 0xE, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x22, 0xE, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x40, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x46, 2, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x8A, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x9C, 0, 0x1F);
        break;
    case 0x419:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x20, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 2, 0xC, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 0xC, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x30, 0xB, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x34, 0xB, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x6C, 0xB, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x70, 0xB, 0x2E);
        break;
    case 0x41A:
        Em_PlaySeTypeAtBoneInFrames((Em *) em, 0x16, 0x11C, 0x49, 0x26, 7, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 2, 0xB, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 0xB, 0x2E);
        break;
    case 0x41B:
        Em_PlaySeTypeAtBoneOnFrame((Em *) em, 0x16, 0x4C, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x12, 0x18, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 0xE, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA, 0xE, 0x2E);
        break;
    case 0x410:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x12, 0xE, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 0xE, 0x2E);
        break;
    case 0x415:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x14, 0xE, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x18, 0xE, 0x2E);
        break;
    case 0x41C:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x25, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x36, 0x23, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 2, 0xE, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 0xE, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2A, 0xB, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2E, 0xB, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3A, 0x3C, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3E, 9, 0x1F);
        break;
    case 0x41D:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x26, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x32, 0x27, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 2, 0xE, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 0xE, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2A, 0xF, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2E, 0xF, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x30, 0x15, 0x11);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x38, 9, 0x11);
        break;
    case 0x41E:
        Em_PlaySeTypeAtBoneOnFrame((Em *) em, 4, 0x47, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x32, 0x23, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 2, 0xA, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 0xA, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2A, 0xF, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2E, 0xF, 0x2E);
        break;
    case 0x41F:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x31, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x28, 0x27, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x7E, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x30, 0x15, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x76, 2, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x12, 3, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3C, 5, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x70, 3, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x46, 5, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x80, 3, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x48, 0x15, 0x11);
        break;
    case 0x420:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x31, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x28, 0x27, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x7E, 9, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2C, 0x15, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x40, 0x15, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x76, 2, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x12, 3, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3C, 5, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x70, 3, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x46, 5, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x80, 3, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x4A, 0x15, 0x11);
        break;
    case 0x421:
    case 0x422:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x26, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2C, 0x28, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x24, 0x14, 0x25);
        Em_PlaySeAtBoneOnFrame((Em *) em, 2, 0xD, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 0xD, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2A, 0xF, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2E, 0xF, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x80, 0xC, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x84, 0xC, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x38, 0x16, 2);
        break;
    case 0x425:
        Em_PlaySeTypeAtBoneOnFrame((Em *) em, 4, 0x53, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x38, 0x23, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x16, 2);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xE, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x14, 3, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x58, 4, 8);
        break;
    case 0x426:
        Em_PlaySeTypeAtBoneOnFrame((Em *) em, 4, 0x54, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1C, 7, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xE, 2, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x62, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x14, 2, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x5C, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x52, 3, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x68, 3, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 0xE, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1A, 0xE, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1C, 0x16, 2);
        break;
    case 0x427:
        Em_PlaySeTypeAtBoneOnFrame((Em *) em, 4, 0x53, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xE, 0xD, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x12, 0xD, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3A, 0xB, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3E, 0xB, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x6E, 0xB, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x72, 0xB, 0x32);
        break;
    case 0x428:
        Em_PlaySeTypeAtBoneOnFrame((Em *) em, 6, 0x54, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 0xC, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA, 0xC, 0x32);
        break;
    case 0x429:
        Em_PlaySeTypeAtBoneOnFrame((Em *) em, 6, 0x53, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 9, 0x25);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x18, 7, 2);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1A, 0xF, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x14, 0x17, 2);
        break;
    case 0x42A:
        Em_PlaySeTypeAtBoneOnFrame((Em *) em, 0x18, 0x57, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x26, 0x10, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1A, 0x17, 0x26);
        break;
    case 0x42B:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x16, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 2, 0x12, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x5A, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3A, 2, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x56, 3, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x62, 3, 0xD);
        break;
    case 0x42C:
        Em_PlaySeTypeAtBoneOnFrame((Em *) em, 4, 0x55, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xDA, 0x2F, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x11A, 0x2E, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16E, 0x2C, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x60, 0x17, 2);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xC0, 0x16, 2);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x11A, 0x17, 2);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1E, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2C, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xC, 3, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1C, 4, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x190, 0x10, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x19C, 7, 2);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1B6, 7, 0x25);
        break;
    case 0x42D:
        Em_PlaySeTypeAtBoneOnFrame((Em *) em, 0xC, 0x58, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xC, 0x16, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1A, 0x12, 0x1A);
        break;
    case 0x42E:
        Em_PlaySeTypeAtBoneOnFrame((Em *) em, 0x1E, 0x4F, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1E, 0x16, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 0xD, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1A, 0xD, 0x32);
        break;
    case 0x42F:
        Em_PlaySeTypeAtBoneOnFrame((Em *) em, 4, 0x56, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x46, 0x17, 2);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x10A, 0x3B, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x14, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x9C, 0x12, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3A, 0x12, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA0, 0x10, 2);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xB8, 7, 2);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x30, 0xE, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x34, 0xE, 0x32);
        break;
    case 0x431:
        Em_PlaySeTypeAtBoneOnFrame((Em *) em, 4, 0x55, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x2E, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x18, 0x17, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x10, 0xD, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x14, 0xD, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x40, 0xC, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x44, 0xC, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x88, 0xE, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x58, 3, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x6E, 0x12, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x86, 7, 2);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x96, 5, 0x25);
        break;
    case 0x432:
        Em_PlaySeTypeAtBoneOnFrame((Em *) em, 4, 0x53, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA, 0x16, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x38, 7, 2);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x4C, 5, 0x25);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 0xD, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xC, 0xD, 0x32);
        break;
    case 0x433:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x32, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xC, 0x17, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0xE, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x56, 0xE, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x5A, 0xE, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x7A, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x8A, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x54, 4, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x80, 4, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x5E, 4, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x8A, 0, 0xD);
        
    case 0x3E9:
    default:
        break;
    }
}

void EmKushala_MotionSoundsTesca(Em *em, EmWork *work)
{
    s16 motion;

    motion = work->fxMotion;
    switch (motion) {
    case 0x3EA:
        Em_PlaySeAtBoneOnFrame(em, 0x24, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x84, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0xB2, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 4, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x62, 4, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x96, 4, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0xC4, 4, 8);
        break;
    case 0x3ED:
        Em_PlaySeAtBoneOnFrame(em, 8, 0x32, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x42, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 3, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 3, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 4, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 4, 0xD);
        break;
    case 0x3EE:
        Em_PlaySeAtBoneOnFrame(em, 8, 0x32, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x42, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 3, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 3, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 4, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 4, 8);
        break;
    case 0x3EF:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x3C, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 3, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 3, 8);
        break;
    case 0x3F0:
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x3C, 2, 0x3C, 2), 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x36, 2, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 3, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 4, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 4, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 3, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0xE, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x36, 0xE, 0x32);
        break;
    case 0x3F1:
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x3C, 2, 0x3C, 2), 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x36, 2, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 3, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 4, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 4, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 3, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0xE, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x36, 0xE, 0x32);
        break;
    case 0x3F2:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x3C, 0x26);
        Em_PlaySeTypeAtBoneOnFrame(em, 0x26, 0x5B, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x14, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 4, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 3, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 0xF, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0xF, 0x32);
        break;
    case 0x3F3:
        Em_PlaySeAtBoneOnFrame(em, 2, 0xB, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 6, 0xB, 0x32);
        break;
    case 0x3F4:
    case 0x404:
    case 0x405:
        Em_PlaySeAtBoneOnFrame(em, 0xD, 0x1A, 0x25);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0x19, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x17, 0x19, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x19, 0x25);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 0x19, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x41, 0x19, 0x2E);
        break;
    case 0x3F5:
        Em_PlaySeTypeAtBoneOnFrame(em, 0x28, 0x4B, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 2, 0xF, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 6, 0xF, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x1C, 0x25);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 0x1D, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 0x1D, 0x2E);
        break;
    case 0x3F6:
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 0x51, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0xD, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 0xD, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0xB, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0xB, 0x32);
        break;
    case 0x3F7:
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 0x31, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0x3E, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 0xE, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0xE, 0x32);
        break;
    case 0x3F8:
        Em_PlaySeAtBoneOnFrame(em, 0x26, 0xE, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 0xE, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 2, 0x1A, 0x25);
        Em_PlaySeAtBoneOnFrame(em, 6, 0x19, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 8, 0x19, 0x2E);
        break;
    case 0x3F9:
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 7, 2);
        Em_PlaySeAtBoneOnFrame(em, 2, 0xF, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 6, 0xF, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x36, 0xD, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 0xD, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x42, 2, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 5, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 4, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x7C, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x8E, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x92, 5, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x8A, 4, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0xAE, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0xC4, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0xC2, 3, 8);
        Em_PlaySeAtBoneOnFrame(em, 0xB8, 4, 0xD);
        break;
    case 0x3FA:
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 0x51, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 8, 0x14, 0x25);
        Em_PlaySeAtBoneOnFrame(em, 4, 0xD, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 8, 0xD, 0x32);
        break;
    case 0x3FD:
        Em_PlaySeAtBoneOnFrame(em, 0xA, 2, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 2, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 2, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 2, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 5, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 5, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 5, 0xD);
        break;
    case 0x3FE:
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 2, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 2, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 5, 8);
        Em_PlaySeAtBoneOnFrame(em, 2, 0x15, 0xD);
        break;
    case 0x3FF:
        Em_PlaySeAtBoneOnFrame(em, 0x14, 2, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 2, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 5, 8);
        Em_PlaySeAtBoneOnFrame(em, 2, 0x15, 0xD);
        break;
    case 0x400:
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 3, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 4, 0xD);
        break;
    case 0x401:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x36, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 4, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 0xF, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0xF, 0x32);
        break;
    case 0x402:
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 7, 2);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 5, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 3, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 0xC, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 0xC, 0x32);
        break;
    case 0x403:
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0xD, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0xD, 0x32);
        break;
    case 0x406:
        Em_PlaySeAtBoneOnFrame(em, 0x20, 0x3D, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 4, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 3, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0xE, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0xE, 0x32);
        break;
    case 0x407:
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x33, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 0x32, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 2, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x70, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 4, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 4, 0xE, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0xE, 0x2E);
        break;
    case 0x408:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x25, 0x26);
        Em_PlaySeTypeAtBoneInFrames(em, 0x38, 0xAA, 0x4A, 0x26, 7, 0);
        Em_PlaySeAtBoneOnFrame(em, 6, 0xF, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0xF, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x60, 0xE, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 0xE, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0xBC, 8, 2);
        Em_PlaySeAtBoneOnFrame(em, 0xB8, 2, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0xC0, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0xF2, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0xF8, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 4, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 3, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0xF6, 3, 0xD);
        break;
    case 0x40A:
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0x29, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 0x3E, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x78, 0x36, 2);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 2, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x9E, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0xCA, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 5, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 4, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0xB4, 3, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x84, 4, 0xD);
        break;
    case 0x40B:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x3C, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x39, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 0x3A, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x82, 0x3B, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0xA8, 0x23, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 4, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 4, 8);
        break;
    case 0x40C:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x32, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 4, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 4, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 1, 0x1A);
        break;
    case 0x40D:
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 0x55, 0x26);
        Em_PlaySeTypeAtBoneOnFrame(em, 0x54, 0x55, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0x12, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 0x12, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x80, 0x12, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x60, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x80, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x36, 4, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 3, 8);
        break;
    case 0x40E:
        Em_PlaySeAtBoneOnFrame(em, 0x50, 0x26, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0xE0, 0x36, 0x26);
        if (em->stateGroup == 6) {
            if (em->state != 7) {
                goto block_49;
            }
        default:
            break;
        }
block_49:
        Em_PlaySeAtBoneOnFrame(em, 0x20, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 0x10, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 3, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 4, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x7C, 0xE, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x80, 0xE, 0x2E);
        break;
    case 0x40F:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x32, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 0x3B, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 0x3E, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x6E, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x84, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x62, 3, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x80, 4, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0xE, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 0xE, 0x2E);
        break;
    case 0x411:
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 0x5B, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 6, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 8, 3, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0x14, 2);
        break;
    case 0x409:
        Em_PlaySeTypeAtBoneOnFrame(em, 0xA, 0x51, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x15, 0x25);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 9, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 7, 2);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 2, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 4, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 3, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 3, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 4, 8);
        break;
    case 0x412:
        Em_PlaySeTypeAtBoneOnFrame(em, 0xA, 0x4F, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x6E, 0x31, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 6, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0x15, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x7E, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x84, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 0xE, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 0xE, 0x2E);
        break;
    case 0x413:
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 0x4D, 0x26);
        Em_PlaySeTypeAtBoneOnFrame(em, 0xB8, 0x5E, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x78, 0x15, 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x98, 0x15, 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0xD4, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0xE4, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 4, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 3, 0xD);
        break;
    case 0x417:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x3C, 0x26);
        Em_PlaySeTypeAtBoneOnFrame(em, 0x32, 0x48, 0x26);
        Em_PlaySeTypeAtBoneOnFrame(em, 0x62, 0x49, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 0x41, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x36, 0x12, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 2, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x68, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 3, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 3, 0xD);
        break;
    case 0x418:
        Em_PlaySeTypeAtBoneOnFrame(em, 6, 0x51, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 0x33, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0xE, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0xE, 0x2E);
        break;
    case 0x419:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x20, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 2, 0xC, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 6, 0xC, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0xB, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 0xB, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 0xB, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x70, 0xB, 0x2E);
        break;
    case 0x41A:
        Em_PlaySeTypeAtBoneInFrames(em, 0x16, 0x11C, 0x49, 0x26, 7, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0x41, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 2, 0xB, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 6, 0xB, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 0xB, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 0xB, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x8C, 0xB, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x90, 0xB, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0xCA, 0xB, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0xCE, 0xB, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x10C, 0xB, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x110, 0xB, 0x2E);
        break;
    case 0x410:
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 0x5B, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0xE, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0xE, 0x2E);
        break;
    case 0x41D:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x36, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 0x29, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 2, 0xE, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 6, 0xE, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 0xF, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 0xF, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 0x15, 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 9, 0x11);
        break;
    case 0x41F:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x31, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 0x27, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x5A, 0x3C, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x7E, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0x15, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x76, 2, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 3, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 5, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x70, 3, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 5, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x80, 3, 0xD);
        break;
    case 0x420:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x31, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 0x27, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x5A, 0x3C, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x7E, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0x15, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x76, 2, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 3, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 5, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x70, 3, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 5, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x80, 3, 8);
        break;
    case 0x423:
        Em_PlaySeAtBoneOnFrame(em, 2, 0x20, 0x26);
        Em_PlaySeTypeAtBoneOnFrame(em, 2, 0x4D, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 0x3E, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0x32, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 4, 0xB, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 8, 0xB, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 0xC, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 0xC, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 0xC, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x68, 0xC, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 0x24, 2);
        Em_PlaySeAtBoneVolOnFrame(em, 0x3E, Sound_RandomSe(0x43, 1, 0x44, 1), 0x2E, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x46, Sound_RandomSe(0x45, 2, 0x43, 1), 0x2E, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x4A, Sound_RandomSe(0x44, 4, 0x45, 2), 0x2E, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x4E, Sound_RandomSe(0x43, 2, 0x44, 2), 0x2E, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x58, Sound_RandomSe(0x45, 2, 0x43, 1), 0x2E, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x5E, Sound_RandomSe(0x43, 4, 0x44, 1), 0x2E, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x6C, Sound_RandomSe(0x45, 2, 0x43, 2), 0x2E, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x74, Sound_RandomSe(0x44, 3, 0x44, 2), 0x2E, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x7C, Sound_RandomSe(0x45, 3, 0x45, 4), 0x2E, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x84, Sound_RandomSe(0x43, 3, 0x44, 3), 0x2E, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x90, Sound_RandomSe(0x43, 4, 0x45, 2), 0x2E, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x44, Sound_RandomSe(0x45, 1, 0x43, 1), 0x32, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x4A, Sound_RandomSe(0x45, 1, 0x43, 1), 0x32, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x54, Sound_RandomSe(0x44, 1, 0x43, 3), 0x32, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x5A, Sound_RandomSe(0x45, 2, 0x43, 2), 0x32, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x62, Sound_RandomSe(0x45, 1, 0x44, 1), 0x32, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x6C, Sound_RandomSe(0x43, 1, 0x43, 2), 0x32, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x70, Sound_RandomSe(0x45, 2, 0x43, 2), 0x32, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x76, Sound_RandomSe(0x45, 2, 0x44, 2), 0x32, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x7C, Sound_RandomSe(0x45, 4, 0x43, 3), 0x32, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x84, Sound_RandomSe(0x45, 2, 0x45, 4), 0x32, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x92, Sound_RandomSe(0x43, 4, 0x44, 2), 0x32, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x40, Sound_RandomSe(0x44, 1, 0x45, 2), 0x11, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x48, Sound_RandomSe(0x45, 1, 0x44, 1), 0x11, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x50, Sound_RandomSe(0x44, 1, 0x45, 2), 0x11, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x5A, Sound_RandomSe(0x44, 2, 0x44, 1), 0x11, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x62, Sound_RandomSe(0x43, 4, 0x45, 2), 0x11, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x68, Sound_RandomSe(0x44, 2, 0x44, 2), 0x11, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x6E, Sound_RandomSe(0x43, 2, 0x45, 2), 0x11, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x78, Sound_RandomSe(0x44, 2, 0x44, 2), 0x11, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x86, Sound_RandomSe(0x43, 3, 0x45, 4), 0x11, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x8C, Sound_RandomSe(0x44, 4, 0x43, 2), 0x11, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x94, Sound_RandomSe(0x43, 3, 0x45, 4), 0x11, 4);
        break;
    case 0x424:
        Em_PlaySeTypeAtBoneInFrames(em, 0x58, 0x104, 0x50, 0x26, 7, 0);
        Em_PlaySeAtBoneOnFrame(em, 2, 0x33, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0x3A, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x3F, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0xC4, 0x3C, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 4, 0xD, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 8, 0xD, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 0xF, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0xF, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0xD2, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0xE2, 1, 0x1F);
        break;
    case 0x425:
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 0x53, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0x23, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 3, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 4, 8);
        break;
    case 0x426:
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 0x54, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 0x29, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 7, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 2, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x62, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 2, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x5C, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x52, 3, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x68, 3, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0xE, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0xE, 0x32);
        break;
    case 0x427:
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 0x53, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0xD, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0xD, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 0xB, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 0xB, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x6E, 0xB, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x72, 0xB, 0x32);
        break;
    case 0x428:
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 0x52, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 6, 0xC, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0xC, 0x32);
        break;
    case 0x429:
        Em_PlaySeAtBoneOnFrame(em, 8, 9, 0x25);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 7, 2);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0xF, 0x32);
        break;
    case 0x42A:
        Em_PlaySeTypeAtBoneOnFrame(em, 0x18, 0x57, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 0x10, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x17, 0x26);
        break;
    case 0x42B:
        Em_PlaySeAtBoneOnFrame(em, 0x10, 0x29, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 2, 0x12, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x5A, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 2, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x56, 3, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x62, 3, 0xD);
        break;
    case 0x42C:
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 0x57, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 0x30, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0xAA, 0x3B, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0xEE, 0x2D, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x11A, 0x26, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 3, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 4, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x190, 0x10, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x19C, 7, 2);
        Em_PlaySeAtBoneOnFrame(em, 0x1B6, 7, 0x25);
        break;
    case 0x42D:
        Em_PlaySeTypeAtBoneOnFrame(em, 0xC, 0x58, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x12, 0x1A);
        break;
    case 0x42E:
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 0x51, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0x33, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 0x3B, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0xD, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0xD, 0x32);
        break;
    case 0x42F:
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 0x55, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 0x3C, 0x26);
        Em_PlaySeTypeAtBoneOnFrame(em, 0x3E, 0x4E, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x10E, 0x36, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x9C, 0x12, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 0x12, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0xA0, 0x10, 2);
        Em_PlaySeAtBoneOnFrame(em, 0xB8, 7, 2);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0xE, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 0xE, 0x32);
        break;
    case 0x431:
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 0x54, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x26, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 0x2A, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 0xD, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0xD, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 0xC, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 0xC, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x88, 0xE, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 3, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x6E, 0x12, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x86, 5, 2);
        Em_PlaySeAtBoneOnFrame(em, 0x96, 9, 0x25);
        break;
    case 0x432:
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 0x54, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 5, 2);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 9, 0x25);
        Em_PlaySeAtBoneOnFrame(em, 8, 0xD, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0xD, 0x32);
        break;
    case 0x433:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x3C, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 0x33, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 4, 0xE, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x56, 0xE, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x5A, 0xE, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x7A, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x8A, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x54, 4, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x80, 4, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x5E, 4, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x8A, 0, 0xD);
        break;
    case 0x3E9:
        break;
    }
}

/*
 * The family's per-kind setup hook (Em_TypeSetup calls it for the four
 * kinds): nothing to set up.
 */
void EmKushala_Setup(Em *em)
{
}

/*
 * Callback-table slots 0 and 1: creates the monster's stage object and
 * effect, then its aura object (StageObj_K1C kind 0xC for Kushala Daora,
 * 0x1D for Lunastra and Teostra).
 */
void EmKushala_SpawnEffects(Em *em)
{
    AlertMark_Create(em, 0x22, 0);
    Shadow_Spawn((Player *) em, 0);
    if (em->type == EM_KIND_KUSHALA_DAORA || em->type == EM_KIND_RUSTED_KUSHALA_DAORA) {
        MonsterFx_CreateOnEm(em, 0xC, em->scaleX);
    } else if (em->type == EM_KIND_LUNASTRA || em->type == EM_KIND_TEOSTRA) {
        MonsterFx_CreateOnEm(em, 0x1D, em->scaleX);
    }
}

/*
 * Sets the monster up: parts, the idle motion, the ground stance, the wind
 * off and the flight defaults. At the quest's start it places the monster
 * by area; hp is fixed in two story quests, else Em_ComputeMaxHp (or the
 * kept hp of a monster that comes back). A part 2 already broken is counted
 * in partBreakCount.
 */
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
    Em_SetMotionRow(em, KUSH_MOT_IDLE, 0, 0);
    em->stance = EM_STANCE_GROUND;
    w->windFrames = 0;
    if ((Quest_FindSpecialMonster(em->type) & 0xFF) == 0xFF) {
        switch (gQuest.stage) {
        case QUEST_ID_KUSHALA_SNOW_SCENE:
        case QUEST_ID_LUNASTRA_TEOSTRA_TOWER_SCENE:
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
        EmTail_Create(em);
    } else {
        em->fxFlags |= 1;
        if (em->type == EM_KIND_KUSHALA_DAORA || em->type == EM_KIND_RUSTED_KUSHALA_DAORA) {
            em->partBreakCount = 1;
        } else {
            em->partBreakCount = 2;
        }
    }
}

/*
 * The start hook: runs EmKushala_Init, then in the story quests holds the
 * monster at its scene's spot until the scene's event flag is set, and goes
 * to KUSH_IDLE.
 */
void EmKushala_Start(Em *em)
{
    EmKushala_Init(em);
    if (em->type == EM_KIND_KUSHALA_DAORA || em->type == EM_KIND_RUSTED_KUSHALA_DAORA) {
        switch (gQuest.stage) {
        case QUEST_ID_KUSHALA_JUNGLE_SCENE:
            if (Save_TestEventFlag(EVENT_FLAG_KUSHALA_JUNGLE_SCENE) == 0) {
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
        case QUEST_ID_LUNASTRA_TEOSTRA_TOWER_SCENE:
            if (Save_TestEventFlag(EVENT_FLAG_LUNASTRA_TEOSTRA_TOWER_SCENE) == 0) {
                em->area = Area_IdForCurrentTime(AREA_TOWER_0x7C);
                em->posX = 11600.0f;
                em->posY = -20.0f;
                em->posZ = 13000.0f;
                em->rotY = 0x6800;
            }
            break;
        case QUEST_ID_LUNASTRA_TEOSTRA_VOLCANO_7_SCENE_1:
        case QUEST_ID_LUNASTRA_TEOSTRA_VOLCANO_7_SCENE_2:
            if (Save_TestEventFlag(EVENT_FLAG_LUNASTRA_TEOSTRA_VOLCANO_7_SCENE) == 0) {
                em->area = Area_IdForCurrentTime(AREA_VOLCANO_7);
                em->posX = 9329.0f;
                em->posY = 3000.0f;
                em->posZ = 9973.0f;
                em->rotY = 0xFC00;
            }
            break;
        }
    }
    EmKushala_ChangeState(em, EM_GROUP_IDLE, KUSH_IDLE, EM_SYNC_LOCAL);
}

/*
 * Callback slot 5: runs EmKushala_Init, then puts the monster at a fixed
 * spot. The kind test names Rathian kinds, which never run this function,
 * so the family always takes the second spot.
 */
void EmKushala_Reinit(Em *em)
{
    u8 type;

    EmKushala_Init(em);
    type = em->type;
    if ((type == EM_KIND_RATHIAN) || (type == EM_KIND_GOLD_RATHIAN) || (type == EM_KIND_PINK_RATHIAN)) {
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

/*
 * Picks the idle for the stance: KUSH_FLY_HOVER in the air, KUSH_IDLE_HOVER
 * in the hover; on the ground KUSH_IDLE_SETTLE out of combat after a motion
 * of EmKushala_IdleTbl, else KUSH_IDLE with the script asked for its next
 * state.
 */
void EmKushala_ToIdle(Em *em)
{
    u16 *idleEntry = EmKushala_IdleTbl;
    s32 found = 0;

    switch (em->stance) {
    case EM_STANCE_AIR:
        em->scriptPending = 1;
        EmKushala_ChangeState(em, EM_GROUP_FLY, KUSH_FLY_HOVER, EM_SYNC_LOCAL);
        break;
    case EM_STANCE_FAMILY:
        em->scriptPending = 1;
        EmKushala_ChangeState(em, EM_GROUP_IDLE, KUSH_IDLE_HOVER, EM_SYNC_LOCAL);
        break;
    case EM_STANCE_GROUND:
    default:
        if (em->stateGroup != EM_GROUP_IDLE || em->state != KUSH_IDLE_SETTLE) {
            if (em->combat == 0) {
                for (; *idleEntry != 0xFFFF; idleEntry++) {
                    if (em->motionId[0] == *idleEntry) {
                        found = 1;
                        break;
                    }
                }
            }
        }
        if (found) {
            EmKushala_ChangeState(em, EM_GROUP_IDLE, KUSH_IDLE_SETTLE, EM_SYNC_LOCAL);
        } else {
            em->scriptPending = 1;
            EmKushala_ChangeState(em, EM_GROUP_IDLE, KUSH_IDLE, EM_SYNC_LOCAL);
        }
        break;
    }
}

/* Ends a state through EmKushala_ToIdle; the two extra arguments are unused. */
void EmKushala_EndState(Em *em, s32 unused1, s32 unused2)
{
    EmKushala_ToIdle(em);
}

/*
 * Ends a state in the air: next 0 goes to KUSH_FLY_HOVER, 1 to
 * KUSH_FLY_HOVER_GUARDED, any other value only asks the script for its next
 * state.
 */
void EmKushala_EndFlyState(Em *em, u8 next)
{
    s32 k;

    k = next & 0xFF;
    em->stance = EM_STANCE_AIR;
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

/*
 * Marks the part breaks a flinch earns: part 0 below 60% hp once it is
 * damaged (Lunastra and Teostra also need partFlags 0x20), part 5 once
 * damaged. With apply set, applies them (Em_ApplyPartBreaks).
 */
void EmKushala_BreakParts(Em *em, s32 apply)
{
    u8 k;

    if (em->type == EM_KIND_LUNASTRA || em->type == EM_KIND_TEOSTRA) {
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

/*
 * Creates a breath attack at a bone (KushalaBreath_CreateAtBone); on the
 * snowy mountains the size is larger (+3 for breath kinds 0 and 2, +1 for
 * kind 1).
 */
void EmKushala_CreateBreathAtBone(Em *em, s32 bone, u8 kind, u8 size, s32 pitch, s32 yaw)
{
    if (gScene.map == MAP_SNOWY_MOUNTAINS || gScene.map == MAP_SNOWY_MOUNTAINS_NIGHT) {
        switch (kind) {
        case 0:
        case 2:
            size += 3;
            break;
        case 1:
            size += 1;
            break;
        }
    }
    KushalaBreath_CreateAtBone(em, bone, kind, size, pitch, yaw);
}

/*
 * KUSH_IDLE_STAND: the idle motion on all three layers until Em.timer runs
 * out, then asks the script.
 */
void EmKushala_Stand(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        if (em->motionId[0] != EM_MOTION_ID(0, KUSH_MOT_IDLE)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(0, KUSH_MOT_IDLE), 0xA, 0, 0);
        }
        if (em->motionId[1] != EM_MOTION_ID(1, KUSH_MOT_IDLE)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(1, KUSH_MOT_IDLE), 0xA, 0, 1);
        }
        if (em->motionId[2] != EM_MOTION_ID(2, KUSH_MOT_IDLE)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(2, KUSH_MOT_IDLE), 0xA, 0, 2);
        }
        break;
    case 1:
        if (--em->timer <= 0) {
            em->scriptPending = 1;
        }
        break;
    }
}

/*
 * KUSH_IDLE: plays the idle motion once, then ends the state; a remote copy
 * waits for the owner.
 */
void EmKushala_Idle(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_IDLE, 0, 0);
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

/*
 * KUSH_IDLE_HOVER: one hover cycle in stance 1, then ends the state
 * (EmKushala_ToIdle comes back here while hovering).
 */
void EmKushala_HoverIdle(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_HOVER, 0, 0);
        em->stance = EM_STANCE_FAMILY;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/* KUSH_IDLE_ALERT: plays the alert motion once, then ends the state. */
void EmKushala_AlertIdle(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_ALERT, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * KUSH_IDLE_EAT_NEED1_START and KUSH_IDLE_EAT_NEED2_START (variant 0, 1):
 * lowers the head without flinching; variant 0 makes the bite effects at
 * the mouth at frames 56 and 102 when a player is in the area. Then
 * KUSH_IDLE_EAT_NEED1 or KUSH_IDLE_EAT_NEED2.
 */
void EmKushala_EatNeedStart(Em *em, EmWork *work, s32 variant)
{
    f32 pos[3];
    f32 ofs[3];
    f32 boneMtx[16];

    Em_SetHitReactMode(em, EM_HIT_REACT_NO_FLINCH);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_EAT_START, 0, 0);
        em->timer = 3;
        break;
    case 1:
        if ((EmMotion_PassedFrameScaled(em, 56.0f, 0) || EmMotion_PassedFrameScaled(em, 102.0f, 0)) && variant == 0 && ((s32)Player_InCurrentArea(em) & 0xFF)) {
            FallingDebris_Create(em, 1);
            Mtx_Copy(boneMtx, (f32 *)Em_Bone(em, 0x25));
            ofs[0] = 0.0f;
            ofs[1] = -45.0f;
            ofs[2] = 165.0f;
            Vec_RotateSelf(ofs, boneMtx);
            Mtx_GetTrans(pos, boneMtx);
            pos[0] += ofs[0];
            pos[1] += ofs[1];
            pos[2] += ofs[2];
            HitSpark_SpawnForOwner(em, 0, 9, 0, pos, 1.0f);
            PartFx_SpawnAt(pos, 0x14, 4.0f);
        }
        if (em->motion[0].playing == 0 && --em->timer <= 0) {
            em->step++;
            if (variant == 0) {
                EmKushala_ChangeState(em, EM_GROUP_IDLE, KUSH_IDLE_EAT_NEED1, EM_SYNC_OWNER_FLAGGED);
            } else {
                EmKushala_ChangeState(em, EM_GROUP_IDLE, KUSH_IDLE_EAT_NEED2, EM_SYNC_OWNER_FLAGGED);
            }
        }
        break;
    case 2:
        if (em->remote == 0) {
            if (variant == 0) {
                EmKushala_ChangeState(em, EM_GROUP_IDLE, KUSH_IDLE_EAT_NEED1, EM_SYNC_OWNER_FLAGGED);
            } else {
                EmKushala_ChangeState(em, EM_GROUP_IDLE, KUSH_IDLE_EAT_NEED2, EM_SYNC_OWNER_FLAGGED);
            }
        }
        break;
    }
}

/*
 * KUSH_IDLE_EAT_NEED1 and KUSH_IDLE_EAT_NEED2: the eating motion, then ends
 * the state; Kushala Daora's variant 0 also drops a timed mine point where
 * it stands.
 */
void EmKushala_EatNeed(Em *em, EmWork *work, s32 variant)
{
    u8 type;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_EAT, 0, 0);
        type = em->type;
        if (((type == EM_KIND_KUSHALA_DAORA) || (type == EM_KIND_RUSTED_KUSHALA_DAORA)) && (variant == 0)) {
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

/* KUSH_IDLE_ACTION_A: plays its motion once, then ends the state. */
void EmKushala_IdleActionA(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_IDLE_ACTION_A, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * KUSH_IDLE_STATUS4_SHAKE_OFF and KUSH_IDLE_STUN_SHAKE_OFF: shakes itself,
 * then ends the state.
 */
void EmKushala_StatusShakeOff(Em *em, EmWork *work, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, KUSH_MOT_SHAKE_OFF, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/* KUSH_IDLE_SHAKE_OFF: the same shake, after a status ends. */
void EmKushala_ShakeOff(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, KUSH_MOT_SHAKE_OFF, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * KUSH_IDLE_REST: lies down and regains 1 hp every 8 frames up to 40% of
 * its maximum (60% for Lunastra and Teostra) for 9000 frames, a hit waking
 * it; then KUSH_IDLE_REST_END.
 */
void EmKushala_Rest(Em *em)
{
    u8 type;

    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_REST, 0, 0);
        Em_LoadReactionSet(em, 1);
        em->timer = 0x2328;
        break;
    case 1:
        type = em->type;
        if ((type == EM_KIND_KUSHALA_DAORA) || (type == EM_KIND_RUSTED_KUSHALA_DAORA)) {
            Em_RegenHp(em, 1, (s32) (0.4f * (f32) em->hpMax), 8);
        } else {
            Em_RegenHp(em, 1, (s32) (0.6f * (f32) em->hpMax), 8);
        }
        if (--em->timer <= 0) {
            em->step++;
            Em_ClearLowHpNeed(em);
            EmKushala_ChangeState(em, EM_GROUP_IDLE, KUSH_IDLE_REST_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmKushala_ChangeState(em, EM_GROUP_IDLE, KUSH_IDLE_REST_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * KUSH_IDLE_REST_END: gets up (Lunastra and Teostra drop a timed mine
 * point); in combat it flaps its wings before the state ends.
 */
void EmKushala_RestEnd(Em *em)
{
    u8 type;

    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, KUSH_MOT_REST_END, 0, 0);
        Em_LoadReactionSet(em, 0);
        type = em->type;
        if ((type == EM_KIND_LUNASTRA) || (type == EM_KIND_TEOSTRA)) {
            EmMine_CreateTimed0(em, &em->posX);
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            if (em->combat == 1) {
                em->step++;
                Em_SetMotion(em, KUSH_MOT_WING_FLAP, 0, 0);
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

/*
 * KUSH_IDLE_STATUS2_COLLAPSE: collapses under status 2 and lies until its
 * timer runs out, then KUSH_IDLE_STATUS2_END.
 */
void EmKushala_Status2Collapse(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_COLLAPSE, 0, 0);
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
            EmKushala_ChangeState(em, EM_GROUP_IDLE, KUSH_IDLE_STATUS2_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmKushala_ChangeState(em, EM_GROUP_IDLE, KUSH_IDLE_STATUS2_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * KUSH_IDLE_STATUS2_END: lies a moment longer, gets up, then
 * KUSH_IDLE_SHAKE_OFF.
 */
void EmKushala_Status2End(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, KUSH_MOT_DOWN, 0, 0);
        Em_LoadReactionSet(em, 0);
        em->blind = 2;
        em->timer = 2;
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            if (--em->timer <= 0) {
                em->step++;
                Em_SetMotion(em, KUSH_MOT_DOWN_GET_UP, 0, 0);
                break;
            }
        }
        break;
    case 2:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            EmKushala_ChangeState(em, EM_GROUP_IDLE, KUSH_IDLE_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmKushala_ChangeState(em, EM_GROUP_IDLE, KUSH_IDLE_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * KUSH_IDLE_EAT_START: lowers the head without flinching (sight row 1,
 * reaction set 1), then KUSH_IDLE_EAT.
 */
void EmKushala_EatStart(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_NO_FLINCH);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_EAT_START, 0, 0);
        Em_SelectSightRow(em, 1);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_ChangeState(em, EM_GROUP_IDLE, KUSH_IDLE_EAT, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmKushala_ChangeState(em, EM_GROUP_IDLE, KUSH_IDLE_EAT, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * KUSH_IDLE_EAT: the eating motion with the normal sight and reactions,
 * then ends the state.
 */
void EmKushala_Eat(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_EAT, 0, 0);
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

/*
 * The roars (KUSH_IDLE_WIND_ROAR to _C and KUSH_IDLE_WIND_ROAR_OR_SWEEP):
 * turns toward the goal until frame 60 of the roar (from the alert stance
 * it starts at frame 30), then lets the motion end.
 */
void EmKushala_Roar(Em *em, EmWork *work)
{
    u16 targetYaw;
    u32 yawLeft;
    s32 yaw;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        if (em->motionId[0] == EM_MOTION_ID(0, KUSH_MOT_ALERT)) {
            Em_SetMotion(em, KUSH_MOT_ROAR, 0xA, 0x1E);
        } else {
            Em_SetMotion(em, KUSH_MOT_ROAR, 0, 0);
        }
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->noReact = 1;
        break;
    case 1:
        yaw = em->rotY;
        targetYaw = em->aimAngle;
        yawLeft = (targetYaw - (yaw & 0xFFFF)) & 0xFFFF;
        if (((yawLeft + 0x200) & 0xFFFF) < 0x400U) {
            em->rotY = targetYaw;
        } else if (yawLeft < 0x8000U) {
            em->rotY = (yaw + 0x200) & 0xFFFF;
        } else {
            em->rotY = (yaw - 0x200) & 0xFFFF;
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

/* KUSH_IDLE_ACTION_B: plays its motion once, then ends the state. */
void EmKushala_IdleActionB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_IDLE_ACTION_B, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/* KUSH_IDLE_ALERT_B: the same as EmKushala_AlertIdle. */
void EmKushala_AlertIdleB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_ALERT, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * KUSH_IDLE_STATUS1_COLLAPSE: collapses under status 1 and lies until its
 * timer runs out, then KUSH_IDLE_STATUS1_END.
 */
void EmKushala_Status1Collapse(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_COLLAPSE, 0, 0);
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
            EmKushala_ChangeState(em, EM_GROUP_IDLE, KUSH_IDLE_STATUS1_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmKushala_ChangeState(em, EM_GROUP_IDLE, KUSH_IDLE_STATUS1_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * KUSH_IDLE_STATUS1_END: lies a moment longer, gets up, then
 * KUSH_IDLE_SHAKE_OFF.
 */
void EmKushala_Status1End(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, KUSH_MOT_DOWN, 0, 0);
        Em_LoadReactionSet(em, 0);
        em->blind = 2;
        em->timer = 2;
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            if (--em->timer <= 0) {
                em->step++;
                Em_SetMotion(em, KUSH_MOT_DOWN_GET_UP, 0, 0);
                break;
            }
        }
        break;
    case 2:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            EmKushala_ChangeState(em, EM_GROUP_IDLE, KUSH_IDLE_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmKushala_ChangeState(em, EM_GROUP_IDLE, KUSH_IDLE_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * KUSH_IDLE_SETTLE: plays the end of the landing motion (from frame 146),
 * then ends the state.
 */
void EmKushala_Settle(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, KUSH_MOT_LAND, 0, 0x92);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * KUSH_IDLE_ROAR: the same roar as EmKushala_Roar; entering it leaves the
 * wind as it is.
 */
void EmKushala_RoarB(Em *em, EmWork *work)
{
    u16 targetYaw;
    u32 yawLeft;
    s32 yaw;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        if (em->motionId[0] == EM_MOTION_ID(0, KUSH_MOT_ALERT)) {
            Em_SetMotion(em, KUSH_MOT_ROAR, 0xA, 0x1E);
        } else {
            Em_SetMotion(em, KUSH_MOT_ROAR, 0, 0);
        }
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->noReact = 1;
        break;
    case 1:
        yaw = em->rotY;
        targetYaw = em->aimAngle;
        yawLeft = (targetYaw - (yaw & 0xFFFF)) & 0xFFFF;
        if (((yawLeft + 0x200) & 0xFFFF) < 0x400U) {
            em->rotY = targetYaw;
        } else if (yawLeft < 0x8000U) {
            em->rotY = (yaw + 0x200) & 0xFFFF;
        } else {
            em->rotY = (yaw - 0x200) & 0xFFFF;
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

/* KUSH_MOVE_WALK: walks to the goal (Em_StepToGoal), then ends the state. */
void EmKushala_Walk(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_WALK, 0, 0);
        break;
    case 1:
        if (Em_StepToGoal(em, 0x40, 0) == 1) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * KUSH_MOVE_LIMP: limps to the goal; out of combat it then settles with the
 * end of the landing motion.
 */
void EmKushala_Limp(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_LIMP, 0, 0);
        break;
    case 1:
        if (Em_StepToGoal(em, 0x40, 0) == 1) {
            if (em->combat == 0) {
                em->step++;
                Em_SetMotion(em, KUSH_MOT_LAND, 0, 0x92);
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

/*
 * KUSH_MOVE_TURN: turns toward the target with EmKushala_TurnParams[0],
 * then ends the state.
 */
void EmKushala_Turn(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
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

/*
 * KUSH_MOVE_TURN_ALERT: picks the small or a big turn by the angle to the
 * goal and turns rotY by a 19th of it a frame over frames 16-54.
 */
void EmKushala_TurnAlert(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        work->turnSpeed = (em->aimAngle - em->rotY) & 0xFFFF;
        if (work->turnSpeed <= 0x238E || work->turnSpeed >= 0xDC72) {
            Em_SetMotion(em, KUSH_MOT_TURN_SMALL, 0, 0);
        } else if (work->turnSpeed >= 0x8000) {
            Em_SetMotion(em, KUSH_MOT_TURN_BIG_LEFT, 0, 0);
        } else {
            Em_SetMotion(em, KUSH_MOT_TURN_BIG_RIGHT, 0, 0);
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

/* KUSH_MOVE_WALK_B: a start motion, then walks to the goal and stops. */
void EmKushala_WalkB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_WALK_B_START, 0, 0);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_SIDE_DASH_B, 0, 0);
        }
        
    case 2:
        if (Em_StepToGoal(em, 0x40, 0) == 1) {
            em->step = 3;
            Em_SetMotion(em, KUSH_MOT_RUN_STOP, 0, 0);
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

/*
 * The runs (KUSH_MOVE_RUN, KUSH_MOVE_RUN_PAST and its arcs): runs to the
 * goal with a contact attack from frame 12; variants 1-4 curve, turning
 * rotY by a fixed step a frame; then stops.
 */
void EmKushala_Run(Em *em, EmWork *work, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_RUN_START, 0, 0);
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
            Em_SetMotion(em, KUSH_MOT_RUN_STOP, 0, 0);
            break;
        }
        if (em->motionId[0] == EM_MOTION_ID(0, KUSH_MOT_RUN_START) && EmMotion_PassedFrameScaled(em, 12.0f, 0)) {
            EmKushala_CreateAttack(em, 8);
        }
        if (em->motion[0].playing == 0) {
            if (em->motionId[0] == EM_MOTION_ID(0, KUSH_MOT_RUN_START)) {
                Em_SetMotion(em, KUSH_MOT_RUN_B, 0, 0);
            } else if (em->motionId[0] == EM_MOTION_ID(0, KUSH_MOT_RUN_B)) {
                Em_SetMotion(em, KUSH_MOT_RUN_C, 0, 0);
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

/*
 * KUSH_MOVE_STOMP: rears up, stomps (its attack comes from
 * EmKushala_MotionEffects) and takes the alert stance.
 */
void EmKushala_Stomp(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_REAR_UP, 0, 0);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_STOMP, 0, 0);
        }
        break;
    case 2:
        if (em->motion[1].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_ALERT, 0, 0);
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

/* KUSH_MOVE_LIFT_TO_HOVER: rears up and lifts into the hover (stance 1). */
void EmKushala_LiftToHover(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_REAR_UP, 0, 0);
        em->stance = EM_STANCE_GROUND;
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            em->stance = EM_STANCE_FAMILY;
            Em_SetMotion(em, KUSH_MOT_LIFT_TO_HOVER, 0, 0);
        }
        break;
    case 2:
        if (em->motion[1].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_HOVER, 0, 0x1C);
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

/*
 * KUSH_MOVE_TAKE_OFF_TO_HOVER: flaps first if it was idle, then takes off
 * into the hover, stance 1 from frame 48.
 */
void EmKushala_TakeOffToHover(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        if (em->motionId[0] == EM_MOTION_ID(0, KUSH_MOT_IDLE)) {
            Em_SetMotion(em, KUSH_MOT_WING_FLAP, 0, 0);
            break;
        }
        em->step++;
        Em_SetMotion(em, KUSH_MOT_TAKE_OFF, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_TAKE_OFF, 0, 0);
        }
        break;
    case 2:
        if (EmMotion_ReachedFrame(em, 0, 48.0f) != 0) {
            em->step++;
            em->stance = EM_STANCE_FAMILY;
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

/*
 * KUSH_MOVE_SIDE_DASH_A and _B (variant 0, 1): slides sideways 50 a frame
 * for 88 frames while turning to keep the goal at its side.
 */
void EmKushala_SideDash(Em *em, EmWork *work, s32 variant)
{
    f32 ofs[3];
    f32 out[3];
    s32 ang[3];
    f32 rotMtx[16];
    s32 targetYaw;
    s32 yawDiff;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        if (variant == 0) {
            Em_SetMotion(em, KUSH_MOT_SIDE_DASH_A, 0, 0);
        } else {
            Em_SetMotion(em, KUSH_MOT_SIDE_DASH_B, 0, 0);
        }
        Player_ClearVelocity(em);
        em->stance = EM_STANCE_FAMILY;
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
            Mtx_FromAnglesYXZ(ang, rotMtx);
            Vec_Rotate(out, ofs, rotMtx);
            em->posX += out[0];
            em->posZ += out[2];
            if (variant == 0) {
                targetYaw = ((Math_AngleXZ(&em->posX, em->goal) & 0xFFFF) + 0xC000) & 0xFFFF;
            } else {
                targetYaw = ((Math_AngleXZ(&em->posX, em->goal) & 0xFFFF) + 0x4000) & 0xFFFF;
            }
            yawDiff = ((targetYaw & 0xFFFF) - em->rotY) & 0xFFFF;
            if (yawDiff <= 0x8000) {
                if (yawDiff <= 0x173) {
                    em->rotY += yawDiff;
                } else {
                    em->rotY += 0x174;
                }
            } else {
                if (yawDiff > 0xFE8C) {
                    em->rotY += yawDiff;
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

/*
 * KUSH_MOVE_HOVER_FORWARD, _PAST and KUSH_MOVE_HOVER_BACKWARD (variant 1:
 * backwards, facing away): hovers to the goal at speed 30.
 */
void EmKushala_HoverForward(Em *em, EmWork *work, s32 variant)
{
    u8 flags;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_HOVER, 0, 0);
        em->stance = EM_STANCE_FAMILY;
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

/*
 * KUSH_MOVE_HOVER_DASH: a fast flight in the hover with attacks at frame
 * 58, a long leg at speed 80 if the goal is far, then the brake.
 */
void EmKushala_HoverDash(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_DASH_START, 0, 0);
        EmKushala_StartCurves(em, 1U);
        em->stance = EM_STANCE_FAMILY;
        break;
    case 1:
        EmKushala_UpdateCurves(em);
        em->goalDist -= em->velZ;
        if ((em->motionId[0] == EM_MOTION_ID(0, KUSH_MOT_DASH_START)) && (EmMotion_PassedFrameScaled(em, 58.0f, 0U) != 0)) {
            EmKushala_CreateAttack(em, 9);
            EmKushala_CreateAttack(em, 0x1A);
        }
        if (em->motion[0].playing == 0) {
            if (em->goalDist <= 0.0f) {
                em->step += 2;
                Em_SetMotion(em, KUSH_MOT_DASH_END, 0, 0);
                EmKushala_StartCurves(em, 2U);
                break;
            }
            em->step++;
            em->velX = 0.0f;
            em->velY = 0.0f;
            em->velZ = 80.0f;
            Player_ClearAccel2(em);
            Em_SetMotion(em, KUSH_MOT_FLY_FAST, 0, 0);
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
            Em_SetMotion(em, KUSH_MOT_DASH_END, 0, 0);
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

/*
 * KUSH_MOVE_HOVER_TURN: turns to the goal in the hover, spread over 31
 * frames (a small angle) or 27 (a large one with the hover turn motion).
 */
void EmKushala_HoverTurn(Em *em, EmWork *work)
{
    s32 turnStep;
    s32 yawLeft;
    s32 yaw;
    u16 targetYaw;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_FAMILY;
        em->lookAtTarget = 0;
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        work->turnSpeed = (em->aimAngle - em->rotY) & 0xFFFF;
        if (work->turnSpeed <= 0x4000 || work->turnSpeed >= 0xC000) {
            Em_SetMotion(em, KUSH_MOT_HOVER, 0, 0);
            em->timer = 0x1F;
        } else {
            Em_SetMotion(em, KUSH_MOT_HOVER_TURN, 0, 0);
            em->timer = 0x1B;
        }
        if (work->turnSpeed >= 0x8000) {
            work->turnSpeed -= 0x10000;
        }
        break;
    case 1:
        if (em->motion[0].blend == 0) {
            turnStep = work->turnSpeed / em->timer;
            yaw = em->rotY;
            targetYaw = em->aimAngle;
            yawLeft = (targetYaw - yaw) & 0xFFFF;
            if (((yawLeft + turnStep) & 0xFFFF) < turnStep * 2) {
                em->rotY = targetYaw & 0xFFFF;
            } else {
                em->rotY = (yaw + turnStep) & 0xFFFF;
            }
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * KUSH_MOVE_LAND and KUSH_MOVE_LAND_QUICK (variant 1, which ends at frame
 * 146): sets down from the hover, on the ground from frame 70.
 */
void EmKushala_LandFromHover(Em *em, EmWork *work, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_FAMILY;
        Em_SetMotion(em, KUSH_MOT_LAND, 0, 0);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 70.0f) != 0) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
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

/*
 * KUSH_MOVE_HOVER_SPIN: one full turn on the spot in the hover (rotY -0x555
 * a frame over frames 6-54).
 */
void EmKushala_HoverSpin(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, KUSH_MOT_HOVER_TURN, 0, 0);
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_FAMILY;
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

/*
 * KUSH_FLY_TAKE_OFF: flaps first if it was idle, then takes off with the
 * lift curve; in the air (stance 2) from frame 48.
 */
void EmKushala_TakeOff(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        work->flyGuard = 0;
        if (em->motionId[0] == EM_MOTION_ID(0, KUSH_MOT_IDLE)) {
            Em_SetMotion(em, KUSH_MOT_WING_FLAP, 0, 0);
            break;
        }
        em->step++;
        Em_SetMotion(em, KUSH_MOT_TAKE_OFF, 0, 0);
        EmKushala_StartCurves(em, 0U);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_TAKE_OFF, 0, 0);
            EmKushala_StartCurves(em, 0U);
        }
        break;
    case 2:
        if (EmMotion_PassedFrameScaled(em, 48.0f, 0U) != 0) {
            em->step++;
            em->stance = EM_STANCE_AIR;
            EmKushala_UpdateCurves(em);
        }
        break;
    case 3:
        EmKushala_UpdateCurves(em);
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndFlyState(em, 0U);
        }
        break;
    }
}

/*
 * KUSH_FLY_LAND and KUSH_FLY_LAND_IN_COMBAT (variant 1, which ends at frame
 * 146): descends on a planned vertical speed, touches down (stance 0, a
 * camera shake) and goes to the idle.
 */
void EmKushala_Land(Em *em, EmWork *work, s32 variant)
{
    f32 excessHeight;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        em->rotX = 0;
        em->rotZ = 0;
        Em_SetMotion(em, KUSH_MOT_HOVER, 0, 0);
        Player_ClearVelocity(em);
        em->velY = -10.0f;
        excessHeight = em->floorY;
        excessHeight = em->posY - (1000.0f + excessHeight);
        if (excessHeight > 0.0f) {
            do {
                em->velY -= 0.75f;
                excessHeight += em->velY;
            } while (excessHeight > 0.0f && !(em->velY < -200.0f));
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
            Em_SetMotion(em, KUSH_MOT_LAND, 0, 0);
            em->rotX = 0;
            em->rotZ = 0;
            em->stance = EM_STANCE_GROUND;
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

/* KUSH_FLY_HOVER: one hover cycle in the air, then asks the script again. */
void EmKushala_Hover(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, KUSH_MOT_HOVER, 0, 0);
        work->flyGuard = 0;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndFlyState(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * KUSH_FLY_HOVER_GUARDED: hovers in place with the flight guard set (hits
 * can't flinch or kill it).
 */
void EmKushala_HoverGuarded(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, KUSH_MOT_FLY_FAST, 0, 0);
        Player_ClearAccel2(em);
        em->velY = 0.0f;
        em->velX = 0.0f;
        work->flyGuard = 1;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndFlyState(em, 1U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * KUSH_FLY_DRIFT_TO_GOAL: drifts level toward the goal at speed 20, turning
 * to it, until within ten frames' travel of it or out of time.
 */
void EmKushala_DriftToGoal(Em *em, EmWork *work)
{
    f32 dist;
    f32 floorY;
    s32 timer;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        work->turnSpeed = 0x100;
        Em_SetMotion(em, KUSH_MOT_HOVER, 0, 0);
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
            EmKushala_EndFlyState(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * KUSH_FLY_DRIFT_TO_GOAL_PITCHED: the same drift, facing the goal and
 * pitched toward it, by the distance in 3D.
 */
void EmKushala_DriftToGoalPitched(Em *em, EmWork *work)
{
    f32 dist;
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        work->turnSpeed = 0x100;
        Em_SetMotion(em, KUSH_MOT_HOVER, 0, 0);
        work->flyGuard = 0;
        break;
    case 1:
        work->moveAngY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        work->moveAngZ = 0;
        Em_GetPitchToGoal(em, &work->moveAngX, 0.0f, 0.0f);
        Em_MoveRotated(em, &work->moveAngX);
        dist = Vec_Distance((f32 *) &em->posX, (f32 *) em->goal);
        if (--em->timer <= 0 || dist <= (10.0f * em->velZ)) {
            em->step++;
            Player_ClearVelocity(em);
            EmKushala_EndFlyState(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * KUSH_FLY_SPIN_IN_PLACE: one full turn on the spot in the air, then asks
 * the script again.
 */
void EmKushala_SpinInPlace(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, KUSH_MOT_HOVER_TURN, 0, 0);
        em->lookAtTarget = 0;
        work->flyGuard = 0;
        break;
    case 1:
        if ((EmMotion_ReachedFrame(em, 0, 6.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 54.0f) == 0)) {
            em->rotY -= 0x555;
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndFlyState(em, 2U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * KUSH_FLY_TURN_TO_GOAL: hovers and turns to the goal for at most 150
 * frames.
 */
void EmKushala_TurnToGoal(Em *em, EmWork *work)
{
    f32 floorY;
    s32 done;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, KUSH_MOT_HOVER, 0, 0);
        work->turnSpeed = 0x180;
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        work->flyGuard = 0;
        em->timer = 0x96;
        break;
    case 1:
        done = Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 1) & 0xFF;
        if (--em->timer <= 0 || done != 0) {
            em->step++;
            EmKushala_EndFlyState(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * KUSH_FLY_LEAVE_AREA: dashes out through the area's exit
 * (Em_AreaExitPoint) and into the next area, then
 * KUSH_FLY_ENTER_AREA_AND_LAND if that is the goal's area, else
 * KUSH_FLY_CROSS_AREA.
 */
void EmKushala_LeaveArea(Em *em, EmWork *work)
{
    u16 destAreaId;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Player_ClearVelocity(em);
        Em_SetMotion(em, KUSH_MOT_DASH_START, 0, 0);
        EmKushala_StartCurves(em, 1);
        Em_AreaExitPoint(em, (f32 *) em->goal);
        work->flyGuard = 1;
        break;
    case 1:
        EmKushala_UpdateCurves(em);
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_FLY_FAST, 0, 0);
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
        destAreaId = *(u16 *) &em->destArea;
        if (em->nextArea == destAreaId || destAreaId == AREA_NONE) {
            Hud_TriggerGlow(em);
            EmKushala_ChangeState(em, EM_GROUP_FLY, KUSH_FLY_ENTER_AREA_AND_LAND, EM_SYNC_SEND);
        } else {
            if (em->remote == 0) {
                em->nextArea = destAreaId;
                em->goalNo = destAreaId;
                em->goalKind = 3;
                Em_ComputeGoal(em, em->goal);
            }
            EmKushala_ChangeState(em, EM_GROUP_FLY, KUSH_FLY_CROSS_AREA, EM_SYNC_SEND);
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

/*
 * KUSH_FLY_ENTER_AREA_AND_LAND: flies to its point in the new area, brakes
 * and levels out, then lands.
 */
void EmKushala_EnterAreaAndLand(Em *em, EmWork *work)
{
    f32 goalDistXZ;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        work->turnSpeed = 0x100;
        Em_SetMotion(em, KUSH_MOT_FLY_FAST, 0, 0);
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
        goalDistXZ = Vec_DistanceXZ(&em->posX, em->goal);
        em->timer--;
        if (goalDistXZ <= 1000.0f || em->timer < 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_DASH_END, 0, 0);
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
            EmKushala_ChangeState(em, EM_GROUP_FLY, KUSH_FLY_LAND, EM_SYNC_SEND);
        }
        break;
    case 3:
        if (em->remote == 0) {
            em->timer = 600;
            EmKushala_ChangeState(em, EM_GROUP_FLY, KUSH_FLY_LAND, EM_SYNC_SEND);
        }
        break;
    }
    if (em->posY < em->floorY) {
        em->posY = em->floorY;
    }
}

/*
 * KUSH_FLY_CROSS_AREA: flies through an area on the way, from its entry
 * point to its exit, then on as EmKushala_LeaveArea does.
 */
void EmKushala_CrossArea(Em *em, EmWork *work)
{
    AreaRecord *area;
    f32 *entryPoint;
    u16 next;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        work->turnSpeed = 0x100;
        Em_SetMotion(em, KUSH_MOT_FLY_FAST, 0, 0);
        Player_ClearVelocity(em);
        em->velZ = 80.0f;
        area = Stage_GetAreaRecord(em->area);
        em->goal[1] = area->bottom;
        entryPoint = Em_AreaEntryValue(em, *em->tables);
        if (entryPoint == NULL) {
            em->goal[0] = area->sizeX / 2.0f;
            em->goal[2] = area->sizeZ / 2.0f;
        } else {
            em->goal[0] = entryPoint[0];
            em->goal[2] = entryPoint[2];
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
            EmKushala_ChangeState(em, EM_GROUP_FLY, KUSH_FLY_ENTER_AREA_AND_LAND, EM_SYNC_SEND);
        } else {
            if (em->remote == 0) {
                em->nextArea = next;
                em->goalNo = next;
                em->goalKind = 3;
                Em_ComputeGoal(em, em->goal);
            }
            EmKushala_ChangeState(em, EM_GROUP_FLY, KUSH_FLY_CROSS_AREA, EM_SYNC_SEND);
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

/*
 * KUSH_FLY_DASH_TO_GOAL: a fast flight to the goal (Em_UpdateFlight), then
 * KUSH_FLY_DASH_BRAKE within 1000 of it or out of time.
 */
void EmKushala_DashToGoal(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        if (em->motionId[0] != EM_MOTION_ID(0, KUSH_MOT_DASH_START)) {
            Em_SetMotion(em, KUSH_MOT_DASH_START, 0, 0);
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
            Em_SetMotion(em, KUSH_MOT_FLY_FAST, 0, 0);
        }
        break;
    case 2:
        if (Vec_DistanceXZ(&em->posX, em->goal) <= 1000.0f || --em->timer <= 0) {
            em->step++;
            EmKushala_ChangeState(em, EM_GROUP_FLY, KUSH_FLY_DASH_BRAKE, EM_SYNC_SEND);
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
            EmKushala_ChangeState(em, EM_GROUP_FLY, KUSH_FLY_DASH_BRAKE, EM_SYNC_SEND);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * KUSH_FLY_DASH_BRAKE: brakes out of the dash, levelling pitch and roll,
 * then hovers.
 */
void EmKushala_BrakeDash(Em *em, EmWork *work)
{
    f32 floorY;
    s32 rotX;
    s32 rotZ;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_DASH_END, 0, 0);
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
            EmKushala_EndFlyState(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * KUSH_FLY_DASH_CIRCUIT: flies a lap of the area's four points, legsLeft
 * counting the legs, then KUSH_FLY_HOVER_GUARDED.
 */
void EmKushala_DashCircuit(Em *em, EmWork *work)
{
    f32 floorY;
    s32 framesLeft;
    u8 n;
    u8 stateVar1;

    switch (em->step) {
    case 0:
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, KUSH_MOT_FLY_FAST, 0, 0);
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
            Em_SetMotion(em, KUSH_MOT_DASH_END, 0, 0);
        }
        if ((Vec_DistanceXZ(&em->posX, em->goal) <= 3000.0f) || (framesLeft = em->timer - 1, em->timer = framesLeft, (framesLeft <= 0))) {
            n = ((EmKushalaWork *)work)->legsLeft - 1;
            ((EmKushalaWork *)work)->legsLeft = n;
            if ((n & 0xFF) <= 0) {
                EmKushala_EndFlyState(em, 1U);
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

/*
 * KUSH_FLY_RISE_TO_AREA_HEIGHT, _FIXED_HEIGHT and _GOAL_HEIGHT (variants
 * 0-2): climbs until it reaches the area's ground height, a height fixed
 * per area, or the goal's height.
 */
void EmKushala_RiseToHeight(Em *em, EmWork *work, s32 variant)
{
    s32 reached = 0;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        work->turnSpeed = 0x100;
        Em_SetMotion(em, KUSH_MOT_HOVER, 0, 0);
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
                reached = 1;
            }
            break;
        case 1:
            switch (Area_DayId(em->area)) {
            case AREA_BATTLE_TOWN_1:
                if (!(em->posY < 2200.0f)) {
                    reached = 1;
                }
                break;
            case AREA_SNOWY_MOUNTAINS_3:
            case AREA_SNOWY_MOUNTAINS_6:
            default:
                if (!(em->posY < 4500.0f)) {
                    reached = 1;
                }
                break;
            }
            break;
        case 2:
            if (!(em->posY < em->goal[1])) {
                reached = 1;
            }
            break;
        }
        if (reached) {
            em->step++;
            Player_ClearVelocity(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndFlyState(em, 0);
        }
        break;
    }
    if (em->posY < em->floorY) {
        em->posY = em->floorY;
    }
}

/*
 * KUSH_ATK_TURN_SWEEP_A: a half turn on the ground (rotY +0x666 a frame
 * over frames 40-59) with attacks at frames 48 and 54.
 */
void EmKushala_TurnSweepA(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_TURN_SWEEP_A, 0, 0);
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

/* KUSH_ATK_TURN_SWEEP_B: the mirrored sweep. */
void EmKushala_TurnSweepB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_TURN_SWEEP_B, 0, 0);
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

/*
 * KUSH_ATK_STRIKE_BREATH: an attack at frame 60; Kushala Daora adds a gust
 * (stage object and attack at frame 74, a breath from bone 0x25 at frame
 * 78).
 */
void EmKushala_StrikeBreath(Em *em)
{
    u8 type;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_STRIKE_BREATH, 0, 0);
        break;
    case 1:
        type = em->type;
        if ((type == EM_KIND_KUSHALA_DAORA) || (type == EM_KIND_RUSTED_KUSHALA_DAORA)) {
            if (EmMotion_PassedFrameScaled(em, 74.0f, 0U) != 0) {
                MonsterFx_CreateOnEmAngled(em, 0xA, 0, 0, 1.0f);
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

/*
 * KUSH_ATK_BREATH_CHAIN: a chain of fire from the mouth (EMFX_BREATH_CHAIN)
 * at frame 104, then its end motion.
 */
void EmKushala_BreathChain(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 1;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_BREATH_CHAIN, 0, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 104.0f, 0U) != 0) {
            EmEffectAttack_CreateChain(em, EMFX_BREATH_CHAIN, 0, 0, 0);
        }
        if (em->motion[0].playing == 0) {
            if (--em->timer <= 0) {
                em->step++;
                Em_SetMotion(em, KUSH_MOT_BREATH_CHAIN_END, 0, 0);
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

/*
 * KUSH_ATK_TESCA_ACTION_A: plays its motion; the attacks and effects are
 * Lunastra's and Teostra's (EmKushala_MotionEffects).
 */
void EmKushala_TescaActionA(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_TESCA_ACTION_A, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * The blast rings (KUSH_ATK_BLAST_RING_A to _D; variant: the ring type):
 * charges for 90 frames, then blasts, with the ring
 * (TeostraBlastRing_Create) at frame 36.
 */
void EmKushala_BlastRing(Em *em, EmWork *work, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 0x64;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_BLAST_CHARGE, 0, 0);
        em->timer = 0x5A;
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 56.0f, 0U) != 0) {
            TeostraBlastCharge_Create(em);
        }
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_BLAST, 0, 0);
        }
        break;
    case 2:
        if (EmMotion_PassedFrameScaled(em, 36.0f, 0U) != 0) {
            EmKushala_SpawnBlastBurst(em);
            TeostraBlastRing_Create(em, variant);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * KUSH_ATK_TESCA_ACTION_B: plays its motion; the effects are Lunastra's and
 * Teostra's.
 */
void EmKushala_TescaActionB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_TESCA_ACTION_B, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * KUSH_ATK_TESCA_ACTION_C: plays its motion and the one after it; the
 * effects are Lunastra's and Teostra's.
 */
void EmKushala_TescaActionC(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 0x64;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_TESCA_ACTION_C, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_TESCA_ACTION_C_END, 0, 0);
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

/* KUSH_ATK_HOVER_ACTION_A: plays its motion in the hover (stance 1 at once). */
void EmKushala_HoverActionA(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_HOVER_ACTION_A, 0, 0);
        em->stance = EM_STANCE_FAMILY;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * Moves the monster 15 forward along the work area's moveAngY (20 for
 * Lunastra and Teostra).
 */
void EmKushala_StepForward(Em *em, EmWork *work)
{
    f32 ofs[3];
    f32 out[3];
    s32 ang[3];
    f32 mtx[16];

    ang[0] = 0;
    ang[1] = work->moveAngY;
    ang[2] = 0;
    if (em->type == EM_KIND_LUNASTRA || em->type == EM_KIND_TEOSTRA) {
        Vec_Set(ofs, 0.0f, 0.0f, 20.0f);
    } else {
        Vec_Set(ofs, 0.0f, 0.0f, 15.0f);
    }
    Mtx_FromAnglesYXZ(ang, mtx);
    Vec_Rotate(out, ofs, mtx);
    em->posX += out[0];
    em->posZ += out[2];
}

/*
 * The breath streams (KUSH_ATK_BREATH_STREAM_ADVANCE, _WEAVE, _SPIN;
 * variants 0-2) in the hover: winds up, then breathes a stream while
 * stepping forward, weaving 45 degrees each way, or spinning in place.
 */
void EmKushala_BreathStream(Em *em, EmWork *work, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 1;
        Em_SetMotion(em, KUSH_MOT_BREATH_WINDUP, 0, 0);
        em->stance = EM_STANCE_FAMILY;
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            if (--em->timer <= 0) {
                em->step++;
                em->timer = 1;
                Em_SetMotion(em, KUSH_MOT_BREATH_STREAM, 0, 0);
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
                Em_SetMotion(em, KUSH_MOT_BREATH_STREAM_END, 0, 0);
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

/*
 * KUSH_ATK_LUNGE_BLAST: lunges at the goal (300 to 1000 away, over 20
 * frames) and makes an explosion ahead at frame 52.
 */
void EmKushala_LungeBlast(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 0xC8;
        Em_SetMotion(em, KUSH_MOT_LUNGE, 0, 0);
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_FAMILY;
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

/* KUSH_ATK_HOVER_ACTION_B: plays its motion in the hover. */
void EmKushala_HoverActionB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_HOVER_ACTION_B, 0, 0);
        em->stance = EM_STANCE_FAMILY;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * KUSH_ATK_HOVER_SWEEP_A and _B (variant 0, 1): a half turn in the hover
 * over frames 50-80 with three attacks.
 */
void EmKushala_HoverSweep(Em *em, EmWork *work, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_FAMILY;
        em->lookAtTarget = 0;
        if (variant == 0) {
            Em_SetMotion(em, KUSH_MOT_HOVER_SWEEP_A, 0, 0);
            break;
        }
        Em_SetMotion(em, KUSH_MOT_HOVER_SWEEP_B, 0, 0);
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

/*
 * The breaths (KUSH_ATK_BREATH_QUICK to KUSH_ATK_AIR_BREATH_SINGLE):
 * variant & 0xF picks the pattern (EmKushala_BreathTimers,
 * EmKushala_BreathCounts), 0x10 means in the air. Winds up, breathes
 * (Kushala Daora's breath objects), finishes, and counts a hit on the fort
 * in the triple.
 */
void EmKushala_BreathAttack(Em *em, EmWork *work, s32 variant)
{
    EmKushalaWork *w;
    s32 k;

    switch (em->step) {
    case 0:
        em->step++;
        em->timer = EmKushala_BreathTimers[variant & 0xF][0];
        Em_SetMotion(em, KUSH_MOT_BREATH_WINDUP, 0, 0);
        if (variant & 0x10) {
            em->stance = EM_STANCE_AIR;
        } else {
            em->stance = EM_STANCE_FAMILY;
        }
        break;
    case 1:
        k = variant & 0xF;
        if (k == 0) {
            if (EmMotion_PassedFrameScaled(em, 40.0f, 0)) {
                em->step++;
                em->timer = EmKushala_BreathCounts[k][0];
                Em_SetMotion(em, KUSH_MOT_BREATH, 0, 0);
            }
        } else if (EmMotion_PassedFrameScaled(em, 106.0f, 0) && --em->timer <= 0) {
            em->step++;
            em->timer = EmKushala_BreathCounts[k][0];
            Em_SetMotion(em, KUSH_MOT_BREATH, 0, 0);
        }
        break;
    case 2:
        if (EmMotion_PassedFrameScaled(em, 20.0f, 0) && (em->type == EM_KIND_KUSHALA_DAORA || em->type == EM_KIND_RUSTED_KUSHALA_DAORA)) {
            switch (variant & 0xF) {
            case 0:
                MonsterFx_CreateOnEmAngled(em, 0xA, 0x2000, 0, 1.0f);
                EmKushala_CreateBreathAtBone(em, 0x25, 2, 1, 0x2000, 0);
                break;
            case 1:
                switch (em->timer) {
                case 3:
                    MonsterFx_CreateOnEmAngled(em, 0x12, 0x1555, 0xF1C8, 1.0f);
                    EmKushala_CreateBreathAtBone(em, 0x25, 1, 0, 0x1555, 0xF1C8);
                    break;
                case 2:
                    MonsterFx_CreateOnEmAngled(em, 0x12, 0x11C7, 0, 1.0f);
                    EmKushala_CreateBreathAtBone(em, 0x25, 1, 0, 0x11C7, 0);
                    break;
                case 1:
                    MonsterFx_CreateOnEmAngled(em, 0x12, 0x1555, 0xE39, 1.0f);
                    EmKushala_CreateBreathAtBone(em, 0x25, 1, 0, 0x1555, 0xE39);
                    break;
                }
                break;
            case 2:
                MonsterFx_CreateOnEmAngled(em, 0xA, 0x2000, 0, 1.0f);
                EmKushala_CreateBreathAtBone(em, 0x25, 2, 2, 0x2000, 0);
                break;
            case 3:
                switch (em->timer) {
                case 3:
                    w = (EmKushalaWork *) work;
                    if (em->remote == 0 && w->fortHits < 10) {
                        w->fortHits++;
                        if (w->fortHits >= 10) {
                            Quest_TriggerEvent(em);
                        }
                        gScene.fortStage = w->fortHits;
                    }
                case 1:
                case 2:
                    MonsterFx_CreateOnEmAngled(em, 0xA, 0x11C7, 0, 1.0f);
                    EmKushala_CreateBreathAtBone(em, 0x25, 1, 0, 0x11C7, 0);
                    break;
                }
                break;
            case 4:
                MonsterFx_CreateOnEmAngled(em, 0xA, 0x11C7, 0, 1.0f);
                EmKushala_CreateBreathAtBone(em, 0x25, 1, 0, 0x11C7, 0);
                break;
            }
        }
        if (EmMotion_PassedFrameScaled(em, 46.0f, 0) && --em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_BREATH_FINISH, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            if (variant & 0x10) {
                EmKushala_EndFlyState(em, 0);
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

/*
 * KUSH_ATK_BLAST_RING_FORT: the blast ring (type 3) with a 150-frame
 * charge; each blast counts a hit on the fort (at 10 the quest's event).
 */
void EmKushala_BlastRingFort(Em *em, EmWork *work)
{
    EmKushalaWork *w = (EmKushalaWork *)work;

    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 100;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        em->timer = 150;
        if (em->motionId[0] == EM_MOTION_ID(0, KUSH_MOT_IDLE)) {
            Em_SetMotion(em, KUSH_MOT_WING_FLAP, 0, 0);
        } else {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_BLAST_CHARGE, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_BLAST_CHARGE, 0, 0);
        }
        break;
    case 2:
        if (EmMotion_PassedFrameScaled(em, 56.0f, 0)) {
            TeostraBlastCharge_Create(em);
        }
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_BLAST, 0, 0);
        }
        break;
    case 3:
        if (EmMotion_PassedFrameScaled(em, 36.0f, 0)) {
            EmKushala_SpawnBlastBurst(em);
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
                Em_SetMotion(em, KUSH_MOT_LAND, 0, 0x92);
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

/*
 * KUSH_REACT_FLINCH (and KUSH_REACT_FIRST_BREAK_FLINCH): a flinch on the
 * ground, then ends the state.
 */
void EmKushala_Flinch(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, KUSH_MOT_FLINCH, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * KUSH_REACT_FLINCH_HEAVY: topples, recovers from frame 104, then ends the
 * state.
 */
void EmKushala_FlinchHeavy(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, KUSH_MOT_TOPPLE, 0, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 104.0f, 0U) != 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_TOPPLE_RECOVER, 0, 0);
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

/* KUSH_REACT_FLINCH_HOVER: a flinch in the hover, staying in stance 1. */
void EmKushala_FlinchHover(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_FAMILY;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, KUSH_MOT_FLINCH_HOVER, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * KUSH_REACT_KNOCK_DOWN: topples, lies for two passes of the lying motion,
 * gets up.
 */
void EmKushala_KnockDown(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, KUSH_MOT_TOPPLE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_DOWN, 0, 0x18);
            em->timer = 2;
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            if (--em->timer <= 0) {
                em->step++;
                Em_SetMotion(em, KUSH_MOT_DOWN_GET_UP, 0, 0);
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

/* KUSH_REACT_FALL: falls out of the air, lands, lies and gets up. */
void EmKushala_Fall(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, KUSH_MOT_FALL, 0, 0);
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
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, KUSH_MOT_FALL_LAND, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_DOWN, 0, 0);
            em->timer = 2;
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            if (--em->timer <= 0) {
                em->step++;
                Em_SetMotion(em, KUSH_MOT_DOWN_GET_UP, 0, 0);
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

/*
 * KUSH_REACT_STATUS4: holds the status motion, its effect every 32 frames,
 * until the status ends, then KUSH_IDLE_STATUS4_SHAKE_OFF.
 */
void EmKushala_Status4(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_STATUS);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, KUSH_MOT_STATUS4, 0, 0);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmKushala_ChangeState(em, EM_GROUP_IDLE, KUSH_IDLE_STATUS4_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmKushala_ChangeState(em, EM_GROUP_IDLE, KUSH_IDLE_STATUS4_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
    Em_SpawnStatus4Effect(em, 2);
}

/*
 * KUSH_REACT_PART_BREAK: can't be hit for a moment, then turns once around
 * in the part-break motion.
 */
void EmKushala_PartBreak(Em *em)
{
    f32 turnF;
    u32 turn;

    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, KUSH_MOT_PART_BREAK, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_PART_BREAK_TURN, 0, 0);
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

/* KUSH_REACT_RETURN: ends the state at once. */
void EmKushala_ReactReturn(Em *em)
{
    EmKushala_EndState(em, 0, 0);
}

/* KUSH_REACT_AIR_KNOCK_DOWN: the same fall as EmKushala_Fall. */
void EmKushala_AirKnockDown(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, KUSH_MOT_FALL, 0, 0);
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
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, KUSH_MOT_FALL_LAND, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_DOWN, 0, 0);
            em->timer = 2;
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            if (--em->timer <= 0) {
                em->step++;
                Em_SetMotion(em, KUSH_MOT_DOWN_GET_UP, 0, 0);
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

/* KUSH_REACT_FLASH_FALL: the fall, blinded every frame. */
void EmKushala_FlashFall(Em *em, EmWork *work)
{
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_FALL, 0, 0);
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
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, KUSH_MOT_FALL_LAND, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_DOWN, 0, 0);
            em->timer = 2;
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            if (--em->timer <= 0) {
                em->step++;
                Em_SetMotion(em, KUSH_MOT_DOWN_GET_UP, 0, 0);
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

/* KUSH_REACT_FLASH_FLINCH: the same flinch as EmKushala_Flinch. */
void EmKushala_FlashFlinch(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, KUSH_MOT_FLINCH, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * KUSH_REACT_HOVER_KNOCK_DOWN: drops from the hover onto the ground, lies
 * and gets up.
 */
void EmKushala_HoverKnockDown(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, KUSH_MOT_FALL_LAND, 0, 0);
        Player_ClearVelocity(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_DOWN, 0, 0);
            em->timer = 2;
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            if (--em->timer <= 0) {
                em->step++;
                Em_SetMotion(em, KUSH_MOT_DOWN_GET_UP, 0, 0);
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

/*
 * KUSH_REACT_STUN: makes the stun's stage object, topples and lies until
 * the stun ends, gets up, then KUSH_IDLE_STUN_SHAKE_OFF.
 */
void EmKushala_Stun(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, KUSH_MOT_TOPPLE, 0, 0);
        StunStars_CreateEm(em, 1, 0x25, (s32) ((f32) em->timer + (em->motion[0].frameCount / 2.0f)), 150.0f, 3.0f);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_DOWN, 0, 0x18);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_DOWN_GET_UP, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_ChangeState(em, EM_GROUP_IDLE, KUSH_IDLE_STUN_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 4:
        if (em->remote == 0) {
            EmKushala_ChangeState(em, EM_GROUP_IDLE, KUSH_IDLE_STUN_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/* KUSH_REACT_HOLD_RISE: held for 50 frames, then rears up into the hover. */
void EmKushala_HoldRise(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_REAR_UP, 0, 0x28);
        em->stance = EM_STANCE_GROUND;
        em->holdTimer = 0x32;
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            em->stance = EM_STANCE_FAMILY;
            Em_SetMotion(em, KUSH_MOT_LIFT_TO_HOVER, 0, 0);
        }
        break;
    case 2:
        if (em->motion[1].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_HOVER, 0, 0x1C);
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

/*
 * KUSH_REACT_HOLD_SETTLE: held for 50 frames, then rears up and settles on
 * the ground in the alert stance.
 */
void EmKushala_HoldSettle(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_REAR_UP, 0, 0x28);
        em->holdTimer = 0x32;
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_STOMP, 0, 0);
        }
        break;
    case 2:
        if (em->motion[1].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_ALERT, 0, 0);
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

/*
 * KUSH_EVENT_LEAVE: staggers, takes off and climbs to height 20000, leaving
 * combat; the state never ends.
 */
void EmKushala_Leave(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_TOPPLE, 0, 0);
        work->flyGuard = 0;
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 104.0f, 0)) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_TOPPLE_RECOVER, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_TAKE_OFF, 0, 0);
            EmKushala_StartCurves(em, 0);
        }
        break;
    case 3:
        if (EmMotion_PassedFrameScaled(em, 48.0f, 0)) {
            em->step++;
            em->stance = EM_STANCE_AIR;
            EmKushala_UpdateCurves(em);
        }
        break;
    case 4:
        EmKushala_UpdateCurves(em);
        if (em->motion[0].playing == 0) {
            em->step++;
            work->turnSpeed = 0x100;
            Em_SetMotion(em, KUSH_MOT_HOVER, 0, 0);
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

/*
 * KUSH_EVENT_LEAVE_FROM_AIR: the climb of EmKushala_Leave, started in the
 * air.
 */
void EmKushala_LeaveFromAir(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        work->turnSpeed = 0x100;
        Em_SetMotion(em, KUSH_MOT_HOVER, 0, 0);
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

/*
 * KUSH_EVENT_SCENE_WALK_IN: the scene in AREA_SNOWY_MOUNTAINS_6: placed by
 * hand, it walks in over two camera cuts, turns and waits for the scene's
 * event flag.
 */
void EmKushala_SceneWalkIn(Em *em)
{
    f32 goalDist;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        GmEm_ResetSize(em);
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, KUSH_MOT_IDLE, 0, 0);
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
            Em_SetMotion(em, KUSH_MOT_WALK, 0, 0);
            em->timer = 1800;
        }
        break;
    case 2:
        goalDist = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer <= 0 || goalDist <= 100.0f) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_WALK, 0, 0);
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
        goalDist = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer <= 0 || goalDist <= 100.0f) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_IDLE_ACTION_A, 0, 0);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_ROAR, 0, 0);
            em->rotY = 0x3000;
        }
        break;
    case 5:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_ALERT, 0, 0);
        }
        break;
    case 6:
        if (em->motion[0].playing == 0 && Save_TestEventFlag(EVENT_FLAG_KUSHALA_SNOW_SCENE) == 1) {
            em->step++;
            em->senseDelay = 0;
            em->noHitTimer = 0;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * KUSH_EVENT_SCENE_RISE: the scene in AREA_VOLCANO_8: rises from below,
 * lands, roars and waits for the scene's event flag.
 */
void EmKushala_SceneRise(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        GmEm_ResetSize(em);
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, KUSH_MOT_HOVER, 0, 0);
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
            Em_SetMotion(em, KUSH_MOT_LAND, 0, 0);
            em->posX = 9855.0f;
            em->posY = 0.0f;
            em->posZ = 4996.0f;
            em->rotY = 0x8000;
        }
        break;
    case 4:
        if (EmMotion_ReachedFrame(em, 0, 146.0f)) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_BLAST, 0, 0x64);
        }
        break;
    case 5:
        if (EmMotion_ReachedFrame(em, 0, 170.0f)) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_BLAST, 0, 0x64);
        }
        break;
    case 6:
        if (EmMotion_ReachedFrame(em, 0, 200.0f)) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_ALERT, 0, 0);
        }
        break;
    case 7:
        if (em->motion[0].playing == 0 && Save_TestEventFlag(EVENT_FLAG_LUNASTRA_TEOSTRA_VOLCANO_8_SCENE) == 1) {
            em->step++;
            em->senseDelay = 0;
            em->noHitTimer = 0;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * KUSH_EVENT_CAPTURED: held and unhittable, lies captured (QuestEm_Captured
 * once) with the lying puffs.
 */
void EmKushala_Captured(Em *em)
{
    f32 pos[3];

    Em_SetCaptured(em, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_DOWNED, 0, 0);
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

/*
 * KUSH_EVENT_SCENE_DESCEND: the scene in AREA_VOLCANO_7: descends, lands,
 * blasts and breathes the chain of fire, then waits for the scene's event
 * flag.
 */
void EmKushala_SceneDescend(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        GmEm_ResetSize(em);
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, KUSH_MOT_HOVER, 0, 0);
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
            Em_SetMotionRow(em, KUSH_MOT_LAND, 0, 0);
            em->posX = 9329.0f;
            em->posY = 0.0f;
            em->posZ = 9973.0f;
            em->rotY = 0xFC00;
        }
        break;
    case 4:
        if (EmMotion_ReachedFrame(em, 0, 146.0f)) {
            em->step++;
            Em_SetMotionRow(em, KUSH_MOT_BLAST, 0, 0);
            em->stance = EM_STANCE_GROUND;
        }
        break;
    case 5:
        if (EmMotion_PassedFrameScaled(em, 36.0f, 0)) {
            EmKushala_SpawnBlastBurst(em);
        }
        if (EmMotion_ReachedFrame(em, 0, 200.0f)) {
            em->step++;
            Em_SetMotionRow(em, KUSH_MOT_BREATH_CHAIN, 0, 0);
        }
        break;
    case 6:
        if (EmMotion_PassedFrameScaled(em, 104.0f, 0)) {
            EmEffectAttack_CreateChain(em, EMFX_BREATH_CHAIN, 0, 0, 0);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_BREATH_CHAIN_END, 0, 0);
        }
        break;
    case 7:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_ALERT, 0, 0);
        }
        break;
    case 8:
        if (Save_TestEventFlag(EVENT_FLAG_LUNASTRA_TEOSTRA_VOLCANO_7_SCENE) == 1) {
            em->step++;
            em->senseDelay = 0;
            em->noHitTimer = 0;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * KUSH_EVENT_SCENE_FLY_IN: the scene in AREA_JUNGLE_3: glides in, descends
 * and hovers, then waits for the scene's event flag.
 */
void EmKushala_SceneFlyIn(Em *em, EmWork *work)
{
    f32 goalDist;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        GmEm_ResetSize(em);
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, KUSH_MOT_FLY_FAST, 0, 0);
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
        goalDist = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer <= 0 || goalDist <= 100.0f) {
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
        goalDist = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer <= 0 || goalDist <= 100.0f) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_HOVER, 0, 0);
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
            Em_SetMotion(em, KUSH_MOT_HOVER, 0, 0);
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
            Em_SetMotion(em, KUSH_MOT_DASH_START, 0, 0);
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
            Em_SetMotion(em, KUSH_MOT_FLY_FAST, 0, 0);
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
        goalDist = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer <= 0 || goalDist <= 100.0f) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_DASH_END, 0, 0);
            EmKushala_StartCurves(em, 2);
        }
        break;
    case 8:
        EmKushala_UpdateCurves(em);
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_HOVER, 0, 0);
        }
        break;
    case 9:
        if (Save_TestEventFlag(EVENT_FLAG_KUSHALA_JUNGLE_SCENE) == 1) {
            em->senseDelay = 0;
            em->noHitTimer = 0;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * KUSH_EVENT_SCENE_WAKE: the scene in AREA_TOWER_0x7C: lies, gets up and
 * blasts, then waits for the scene's event flag.
 */
void EmKushala_SceneWake(Em *em)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        GmEm_ResetSize(em);
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, KUSH_MOT_REST, 0, 0);
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
            Em_SetMotion(em, KUSH_MOT_REST_END, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_BLAST_CHARGE, 0, 0);
            em->timer = 2;
            em->posX = 10018.0f;
            em->posY = 0.0f;
            em->posZ = 11470.0f;
            em->rotY = 0xB000;
        }
        break;
    case 4:
        if (EmMotion_PassedFrameScaled(em, 56.0f, 0)) {
            TeostraBlastCharge_Create(em);
        }
        if (em->motion[0].playing == 0 && --em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_BLAST, 0, 0x64);
        }
        break;
    case 5:
        if (EmMotion_ReachedFrame(em, 0, 230.0f)) {
            em->step++;
        }
        break;
    case 6:
        if (Save_TestEventFlag(EVENT_FLAG_LUNASTRA_TEOSTRA_TOWER_SCENE) == 1) {
            em->step++;
            em->senseDelay = 0;
            em->noHitTimer = 0;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * KUSH_DEFEAT_DIE: the death motion (a camera shake at frame 414),
 * QuestEm_Hunted, the carcass's mine point for 1800 frames, then a
 * 150-frame fade and KUSH_DEFEAT_REAPPEAR.
 */
void EmKushala_Die(Em *em, EmWork *work)
{
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, KUSH_MOT_DIE, 0, 0);
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_DEAD;
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
                EmKushala_ChangeState(em, EM_GROUP_DEFEAT, KUSH_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    }
}

/*
 * KUSH_DEFEAT_DIE_FALLING: falls to the floor, then dies as EmKushala_Die
 * does.
 */
void EmKushala_DieFalling(Em *em, EmWork *work)
{
    s32 savedTimer;

    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_FALL, 0, 0);
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
            Em_SetMotion(em, KUSH_MOT_FALL_LAND, 0, 0);
            Player_ClearVelocity(em);
            em->stance = EM_STANCE_DEAD;
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
        savedTimer = em->timer;
        if (savedTimer <= 0) {
            if (em->remote == 0) {
                EmKushala_ChangeState(em, EM_GROUP_DEFEAT, KUSH_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
            }
        } else {
            em->alpha = (f32) savedTimer / 150.0f;
        }
        break;
    case 0x63:
        if (em->motion[0].playing == 0) {
            em->stance = EM_STANCE_GROUND;
            EmKushala_EndState(em, 0, 0);
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
        }
        break;
    }
}

/*
 * KUSH_DEFEAT_REAPPEAR: comes back to the idle if the quest lets it appear
 * again (QuestEm_CanAppear), else ends the task.
 */
void EmKushala_Reappear(Em *em)
{
    switch (em->step) {
    case 0:
        if (QuestEm_CanAppear(em) == 1) {
            EmKushala_ChangeState(em, EM_GROUP_IDLE, KUSH_IDLE, EM_SYNC_LOCAL);
            break;
        }
        em->taskPhase++;
    case 1:
        break;
    }
}

/*
 * Runs the current group 0 (idle) state's function; some functions serve
 * several states, told apart by a variant.
 */
void EmKushala_RunIdleState(Em *em, EmWork *work)
{
    switch (em->state) {
    case KUSH_IDLE_STAND:
        EmKushala_Stand(em);
        break;
    case KUSH_IDLE:
        EmKushala_Idle(em);
        break;
    case KUSH_IDLE_HOVER:
        EmKushala_HoverIdle(em);
        break;
    case KUSH_IDLE_ALERT:
        EmKushala_AlertIdle(em);
        break;
    case KUSH_IDLE_EAT_NEED1_START:
        EmKushala_EatNeedStart(em, work, 0);
        break;
    case KUSH_IDLE_EAT_NEED1:
        EmKushala_EatNeed(em, work, 0);
        break;
    case KUSH_IDLE_ACTION_A:
        EmKushala_IdleActionA(em);
        break;
    case KUSH_IDLE_STATUS4_SHAKE_OFF:
        EmKushala_StatusShakeOff(em, work, 0);
        break;
    case KUSH_IDLE_SHAKE_OFF:
        EmKushala_ShakeOff(em);
        break;
    case KUSH_IDLE_REST:
        EmKushala_Rest(em);
        break;
    case KUSH_IDLE_REST_END:
        EmKushala_RestEnd(em);
        break;
    case KUSH_IDLE_STATUS2_COLLAPSE:
        EmKushala_Status2Collapse(em);
        break;
    case KUSH_IDLE_STATUS2_END:
        EmKushala_Status2End(em);
        break;
    case KUSH_IDLE_EAT_START:
        EmKushala_EatStart(em);
        break;
    case KUSH_IDLE_EAT:
        EmKushala_Eat(em);
        break;
    case KUSH_IDLE_WIND_ROAR:
        EmKushala_Roar(em, work);
        break;
    case KUSH_IDLE_WIND_ROAR_B:
        EmKushala_Roar(em, work);
        break;
    case KUSH_IDLE_WIND_ROAR_C:
        EmKushala_Roar(em, work);
        break;
    case KUSH_IDLE_ACTION_B:
        EmKushala_IdleActionB(em);
        break;
    case KUSH_IDLE_ALERT_B:
        EmKushala_AlertIdleB(em);
        break;
    case KUSH_IDLE_WIND_ROAR_OR_SWEEP:
        EmKushala_Roar(em, work);
        break;
    case KUSH_IDLE_STATUS1_COLLAPSE:
        EmKushala_Status1Collapse(em);
        break;
    case KUSH_IDLE_STATUS1_END:
        EmKushala_Status1End(em);
        break;
    case KUSH_IDLE_SETTLE:
        EmKushala_Settle(em);
        break;
    case KUSH_IDLE_STUN_SHAKE_OFF:
        EmKushala_StatusShakeOff(em, work, 1);
        break;
    case KUSH_IDLE_EAT_NEED2_START:
        EmKushala_EatNeedStart(em, work, 1);
        break;
    case KUSH_IDLE_EAT_NEED2:
        EmKushala_EatNeed(em, work, 1);
        break;
    case KUSH_IDLE_ROAR:
        EmKushala_RoarB(em, work);
        break;
    }
}

/* Runs the current group 1 (move) state's function. */
void EmKushala_RunMoveState(Em *em, EmWork *work)
{
    switch (em->state) {
    case KUSH_MOVE_WALK:
        EmKushala_Walk(em);
        break;
    case KUSH_MOVE_LIMP:
        EmKushala_Limp(em);
        break;
    case KUSH_MOVE_TURN:
        EmKushala_Turn(em);
        break;
    case KUSH_MOVE_TURN_ALERT:
        EmKushala_TurnAlert(em, work);
        break;
    case KUSH_MOVE_WALK_B:
        EmKushala_WalkB(em);
        break;
    case KUSH_MOVE_RUN:
        EmKushala_Run(em, work, 0);
        break;
    case KUSH_MOVE_STOMP:
        EmKushala_Stomp(em);
        break;
    case KUSH_MOVE_LIFT_TO_HOVER:
        EmKushala_LiftToHover(em);
        break;
    case KUSH_MOVE_RUN_PAST:
        EmKushala_Run(em, work, 0);
        break;
    case KUSH_MOVE_RUN_PAST_ARC_A:
        EmKushala_Run(em, work, 1);
        break;
    case KUSH_MOVE_RUN_PAST_ARC_B:
        EmKushala_Run(em, work, 2);
        break;
    case KUSH_MOVE_RUN_PAST_ARC_C:
        EmKushala_Run(em, work, 3);
        break;
    case KUSH_MOVE_RUN_PAST_ARC_D:
        EmKushala_Run(em, work, 4);
        break;
    case KUSH_MOVE_TAKE_OFF_TO_HOVER:
        EmKushala_TakeOffToHover(em);
        break;
    case KUSH_MOVE_SIDE_DASH_A:
        EmKushala_SideDash(em, work, 0);
        break;
    case KUSH_MOVE_SIDE_DASH_B:
        EmKushala_SideDash(em, work, 1);
        break;
    case KUSH_MOVE_HOVER_FORWARD:
        EmKushala_HoverForward(em, work, 0);
        break;
    case KUSH_MOVE_HOVER_DASH:
        EmKushala_HoverDash(em, work);
        break;
    case KUSH_MOVE_HOVER_TURN:
        EmKushala_HoverTurn(em, work);
        break;
    case KUSH_MOVE_LAND:
        EmKushala_LandFromHover(em, work, 0);
        break;
    case KUSH_MOVE_HOVER_SPIN:
        EmKushala_HoverSpin(em);
        break;
    case KUSH_MOVE_HOVER_FORWARD_PAST:
        EmKushala_HoverForward(em, work, 0);
        break;
    case KUSH_MOVE_HOVER_BACKWARD:
        EmKushala_HoverForward(em, work, 1);
        break;
    case KUSH_MOVE_LAND_QUICK:
        EmKushala_LandFromHover(em, work, 1);
        break;
    }
}

/* Runs the current group 2 (fly) state's function. */
void EmKushala_RunFlyState(Em *em, EmWork *work)
{
    switch (em->state) {
    case KUSH_FLY_TAKE_OFF:
        EmKushala_TakeOff(em, work);
        break;
    case KUSH_FLY_LAND:
        EmKushala_Land(em, work, 0);
        break;
    case KUSH_FLY_HOVER:
        EmKushala_Hover(em, work);
        break;
    case KUSH_FLY_HOVER_GUARDED:
        EmKushala_HoverGuarded(em, work);
        break;
    case KUSH_FLY_DRIFT_TO_GOAL:
        EmKushala_DriftToGoal(em, work);
        break;
    case KUSH_FLY_DRIFT_TO_GOAL_PITCHED:
        EmKushala_DriftToGoalPitched(em, work);
        break;
    case KUSH_FLY_SPIN_IN_PLACE:
        EmKushala_SpinInPlace(em, work);
        break;
    case KUSH_FLY_TURN_TO_GOAL:
        EmKushala_TurnToGoal(em, work);
        break;
    case KUSH_FLY_LEAVE_AREA:
        EmKushala_LeaveArea(em, work);
        break;
    case KUSH_FLY_ENTER_AREA_AND_LAND:
        EmKushala_EnterAreaAndLand(em, work);
        break;
    case KUSH_FLY_CROSS_AREA:
        EmKushala_CrossArea(em, work);
        break;
    case KUSH_FLY_DASH_TO_GOAL:
        EmKushala_DashToGoal(em, work);
        break;
    case KUSH_FLY_DASH_BRAKE:
        EmKushala_BrakeDash(em, work);
        break;
    case KUSH_FLY_DASH_CIRCUIT:
        EmKushala_DashCircuit(em, work);
        break;
    case KUSH_FLY_LAND_IN_COMBAT:
        EmKushala_Land(em, work, 1);
        break;
    case KUSH_FLY_RISE_TO_AREA_HEIGHT:
        EmKushala_RiseToHeight(em, work, 0);
        break;
    case KUSH_FLY_RISE_TO_FIXED_HEIGHT:
        EmKushala_RiseToHeight(em, work, 1);
        break;
    case KUSH_FLY_RISE_TO_GOAL_HEIGHT:
        EmKushala_RiseToHeight(em, work, 2);
        
    default:
        break;
    }
}

/* Runs the current group 3 (attack) state's function. */
void EmKushala_RunAttackState(Em *em, EmWork *work)
{
    switch (em->state) {
    case KUSH_ATK_TURN_SWEEP_A:
        EmKushala_TurnSweepA(em);
        break;
    case KUSH_ATK_TURN_SWEEP_B:
        EmKushala_TurnSweepB(em);
        break;
    case KUSH_ATK_STRIKE_BREATH:
        EmKushala_StrikeBreath(em);
        break;
    case KUSH_ATK_BREATH_CHAIN:
        EmKushala_BreathChain(em);
        break;
    case KUSH_ATK_TESCA_ACTION_A:
        EmKushala_TescaActionA(em);
        break;
    case KUSH_ATK_BLAST_RING_A:
        EmKushala_BlastRing(em, work, 0);
        break;
    case KUSH_ATK_TESCA_ACTION_B:
        EmKushala_TescaActionB(em);
        break;
    case KUSH_ATK_BLAST_RING_B:
        EmKushala_BlastRing(em, work, 1);
        break;
    case KUSH_ATK_TESCA_ACTION_C:
        EmKushala_TescaActionC(em);
        break;
    case KUSH_ATK_HOVER_ACTION_A:
        EmKushala_HoverActionA(em);
        break;
    case KUSH_ATK_BREATH_STREAM_ADVANCE:
        EmKushala_BreathStream(em, work, 0);
        break;
    case KUSH_ATK_LUNGE_BLAST:
        EmKushala_LungeBlast(em, work);
        break;
    case KUSH_ATK_HOVER_ACTION_B:
        EmKushala_HoverActionB(em);
        break;
    case KUSH_ATK_HOVER_SWEEP_A:
        EmKushala_HoverSweep(em, work, 0);
        break;
    case KUSH_ATK_HOVER_SWEEP_B:
        EmKushala_HoverSweep(em, work, 1);
        break;
    case KUSH_ATK_BREATH_QUICK:
        EmKushala_BreathAttack(em, work, 0);
        break;
    case KUSH_ATK_BREATH_FAN:
        EmKushala_BreathAttack(em, work, 1);
        break;
    case KUSH_ATK_BREATH_FULL:
        EmKushala_BreathAttack(em, work, 2);
        break;
    case KUSH_ATK_AIR_BREATH_QUICK:
        EmKushala_BreathAttack(em, work, 16);
        break;
    case KUSH_ATK_AIR_BREATH_FAN:
        EmKushala_BreathAttack(em, work, 17);
        break;
    case KUSH_ATK_AIR_BREATH_FULL:
        EmKushala_BreathAttack(em, work, 18);
        break;
    case KUSH_ATK_BREATH_STREAM_WEAVE:
        EmKushala_BreathStream(em, work, 1);
        break;
    case KUSH_ATK_BREATH_STREAM_SPIN:
        EmKushala_BreathStream(em, work, 2);
        break;
    case KUSH_ATK_AIR_BREATH_TRIPLE:
        EmKushala_BreathAttack(em, work, 19);
        break;
    case KUSH_ATK_BLAST_RING_FORT:
        EmKushala_BlastRingFort(em, work);
        break;
    case KUSH_ATK_AIR_BREATH_SINGLE:
        EmKushala_BreathAttack(em, work, 20);
        break;
    case KUSH_ATK_BLAST_RING_C:
        EmKushala_BlastRing(em, work, 2);
        break;
    case KUSH_ATK_BLAST_RING_D:
        EmKushala_BlastRing(em, work, 3);
        break;
    }
}

/*
 * Runs the current group 4 (react) state's function; a state it doesn't
 * list runs the flinch.
 */
void EmKushala_RunReactState(Em *em, EmWork *work)
{
    switch (em->state) {
    default:
        EmKushala_Flinch(em);
        break;
    case KUSH_REACT_FLINCH:
        EmKushala_Flinch(em);
        break;
    case KUSH_REACT_FLINCH_HEAVY:
        EmKushala_FlinchHeavy(em);
        break;
    case KUSH_REACT_FLINCH_HOVER:
        EmKushala_FlinchHover(em);
        break;
    case KUSH_REACT_KNOCK_DOWN:
        EmKushala_KnockDown(em);
        break;
    case KUSH_REACT_FALL:
        EmKushala_Fall(em, work);
        break;
    case KUSH_REACT_STATUS4:
        EmKushala_Status4(em);
        break;
    case KUSH_REACT_PART_BREAK:
        EmKushala_PartBreak(em);
        break;
    case KUSH_REACT_RETURN:
        EmKushala_ReactReturn(em);
        break;
    case KUSH_REACT_AIR_KNOCK_DOWN:
        EmKushala_AirKnockDown(em, work);
        break;
    case KUSH_REACT_FLASH_FALL:
        EmKushala_FlashFall(em, work);
        break;
    case KUSH_REACT_FLASH_FLINCH:
        EmKushala_FlashFlinch(em);
        break;
    case KUSH_REACT_HOVER_KNOCK_DOWN:
        EmKushala_HoverKnockDown(em);
        break;
    case KUSH_REACT_STUN:
        EmKushala_Stun(em);
        break;
    case KUSH_REACT_HOLD_RISE:
        EmKushala_HoldRise(em);
        break;
    case KUSH_REACT_HOLD_SETTLE:
        EmKushala_HoldSettle(em);
        break;
    case KUSH_REACT_FIRST_BREAK_FLINCH:
        EmKushala_Flinch(em);
        break;
    }
}

/* Runs the current group 5 (defeat) state's function. */
void EmKushala_RunDefeatState(Em *em, EmWork *work)
{
    switch (em->state) {
    case KUSH_DEFEAT_DIE:
        EmKushala_Die(em, work);
        break;
    case KUSH_DEFEAT_DIE_B:
        EmKushala_Die(em, work);
        break;
    case KUSH_DEFEAT_DIE_FALLING:
        EmKushala_DieFalling(em, work);
        break;
    case KUSH_DEFEAT_REAPPEAR:
        EmKushala_Reappear(em);
        break;
    }
}

/* Runs the current group 6 or 7 (event) state's function. */
void EmKushala_RunEventState(Em *em, EmWork *work)
{
    switch (em->state) {
    case KUSH_EVENT_LEAVE:
        EmKushala_Leave(em, work);
        break;
    case KUSH_EVENT_LEAVE_FROM_AIR:
        EmKushala_LeaveFromAir(em, work);
        break;
    case KUSH_EVENT_SCENE_WALK_IN:
        EmKushala_SceneWalkIn(em);
        break;
    case KUSH_EVENT_SCENE_RISE:
        EmKushala_SceneRise(em, work);
        break;
    case KUSH_EVENT_CAPTURED:
        EmKushala_Captured(em);
        break;
    case KUSH_EVENT_SCENE_DESCEND:
        EmKushala_SceneDescend(em, work);
        break;
    case KUSH_EVENT_SCENE_FLY_IN:
        EmKushala_SceneFlyIn(em, work);
        break;
    case KUSH_EVENT_SCENE_WAKE:
        EmKushala_SceneWake(em);
        
    default:
        break;
    }
}

/*
 * The family's per-frame update: the wind's hitzones and marker, the needs
 * and the low-hp check (outside the arenas), Em_UpdateDamage's result as a
 * reaction, a status or a death, the story scenes by quest stage, then the
 * state (twice when a state change is pending).
 */
void EmKushala_Update(Em *em)
{
    u8 buf[4];
    f32 ratio;
    EmKushalaWork *w = (EmKushalaWork *)&em->work;
    s32 damage;
    u16 k;

    Em_UpdateCombatTimer(em);
    Em_RaisePlayerAreaTriggerLax(em);
    if (em->type == EM_KIND_KUSHALA_DAORA || em->type == EM_KIND_RUSTED_KUSHALA_DAORA) {
        ratio = 0.12f;
        em->hitzoneSet = 0;
        em->windTimer = 0;
        if (EmKushala_WindActive(em)) {
            em->hitzoneSet = 1;
            em->windTimer = 2;
        }
    } else {
        if (em->type == EM_KIND_LUNASTRA) {
            ratio = 0.15f;
        } else {
            ratio = 0.2f;
        }
        em->hitzoneSet = 0;
        em->windTimer = 0;
        if (EmKushala_WindActiveTesca(em)) {
            em->hitzoneSet = 1;
            em->windTimer = 2;
        }
    }
    k = Map_DayId(gScene.map);
    if (em->noAlert != 1 && em->rage == 0 && k != 8 && k != 9 && k != 0xA) {
        if (em->type == EM_KIND_KUSHALA_DAORA || em->type == EM_KIND_RUSTED_KUSHALA_DAORA) {
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
    damage = Em_UpdateDamage(em, buf) & 0xFF;
    switch (damage) {
    case EM_DMG_NONE:
    case EM_DMG_FLASH:
    case EM_DMG_HIT:
    case EM_DMG_ATTACK5:
    case EM_DMG_ATTACK5_FLINCH:
        break;
    case EM_DMG_KILLED:
    case EM_DMG_DEAD:
        switch (gQuest.stage) {
        case QUEST_ID_KUSHALA_JUNGLE_SCENE:
        case 0x138B:
        case QUEST_ID_LUNASTRA_TEOSTRA_VOLCANO_8_SCENE:
        case 0x138E:
            if (em->stateGroup != EM_GROUP_EVENT && em->stateGroup != EM_GROUP_DEFEAT) {
                em->scriptPending = 0;
                if (em->stance == EM_STANCE_AIR) {
                    EmKushala_ChangeState(em, EM_GROUP_EVENT, KUSH_EVENT_LEAVE_FROM_AIR, EM_SYNC_OWNER);
                } else {
                    if (gScene.area == em->area) {
                        CamReq_ForMonster(em);
                    }
                    EmKushala_ChangeState(em, EM_GROUP_EVENT, KUSH_EVENT_LEAVE, EM_SYNC_OWNER);
                }
            }
            break;
        default:
            if (em->stance == EM_STANCE_AIR) {
                EmKushala_ChangeState(em, EM_GROUP_DEFEAT, KUSH_DEFEAT_DIE_FALLING, EM_SYNC_OWNER);
            } else {
                EmKushala_ChangeState(em, EM_GROUP_DEFEAT, KUSH_DEFEAT_DIE, EM_SYNC_OWNER);
            }
            break;
        }
        break;
    case EM_DMG_ATTACK9:
        if (em->stance == EM_STANCE_AIR) {
            EmKushala_ChangeState(em, EM_GROUP_REACT, KUSH_REACT_AIR_KNOCK_DOWN, EM_SYNC_OWNER);
        } else if (em->stance == EM_STANCE_FAMILY) {
            EmKushala_ChangeState(em, EM_GROUP_REACT, KUSH_REACT_HOVER_KNOCK_DOWN, EM_SYNC_OWNER);
        } else {
            EmKushala_ChangeState(em, EM_GROUP_REACT, KUSH_REACT_KNOCK_DOWN, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_ATTACK6:
    case EM_DMG_ATTACK13:
        if (em->stance == EM_STANCE_GROUND) {
            if (em->type == EM_KIND_KUSHALA_DAORA || em->type == EM_KIND_RUSTED_KUSHALA_DAORA) {
                EmKushala_ChangeState(em, EM_GROUP_REACT, KUSH_REACT_HOLD_RISE, EM_SYNC_OWNER);
            } else {
                EmKushala_ChangeState(em, EM_GROUP_REACT, KUSH_REACT_HOLD_SETTLE, EM_SYNC_OWNER);
            }
        }
        break;
    case EM_DMG_STATUS_IN_AIR:
        if ((em->stateGroup != EM_GROUP_REACT || em->state != KUSH_REACT_FALL) && (em->stateGroup != EM_GROUP_REACT || em->state != KUSH_REACT_AIR_KNOCK_DOWN)) {
            EmKushala_ChangeState(em, EM_GROUP_REACT, KUSH_REACT_FALL, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_STATUS4:
        if ((em->stateGroup != EM_GROUP_REACT || em->state != KUSH_REACT_STATUS4) && (em->stateGroup != EM_GROUP_REACT || em->state != KUSH_REACT_FALL) &&
            (em->stateGroup != EM_GROUP_REACT || em->state != KUSH_REACT_AIR_KNOCK_DOWN)) {
            Em_SetStatus4Timer(em);
            EmKushala_ChangeState(em, EM_GROUP_REACT, KUSH_REACT_STATUS4, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_CAPTURED:
        if ((em->stateGroup != EM_GROUP_IDLE || em->state != KUSH_IDLE_STATUS1_COLLAPSE) && (em->stateGroup != EM_GROUP_REACT || em->state != KUSH_REACT_FALL) &&
            (em->stateGroup != EM_GROUP_REACT || em->state != KUSH_REACT_AIR_KNOCK_DOWN)) {
            Em_SetStatus1Timer(em);
            EmKushala_ChangeState(em, EM_GROUP_IDLE, KUSH_IDLE_STATUS1_COLLAPSE, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_STATUS2:
        if ((em->stateGroup != EM_GROUP_IDLE || em->state != KUSH_IDLE_STATUS2_COLLAPSE) && (em->stateGroup != EM_GROUP_REACT || em->state != KUSH_REACT_FALL) &&
            (em->stateGroup != EM_GROUP_REACT || em->state != KUSH_REACT_AIR_KNOCK_DOWN)) {
            Em_SetStatus2Timer(em);
            EmKushala_ChangeState(em, EM_GROUP_IDLE, KUSH_IDLE_STATUS2_COLLAPSE, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_HIT_LYING:
        switch (em->state) {
        case 9:
            EmKushala_ChangeState(em, EM_GROUP_IDLE, KUSH_IDLE_REST_END, EM_SYNC_OWNER);
            em->scriptPending = 0;
            Em_CheckRageByHp(em, em->rageThreshold);
            break;
        case 0xB:
            EmKushala_ChangeState(em, EM_GROUP_IDLE, KUSH_IDLE_STATUS2_END, EM_SYNC_OWNER);
            em->scriptPending = 0;
            break;
        case 0x15:
            EmKushala_ChangeState(em, EM_GROUP_IDLE, KUSH_IDLE_STATUS1_END, EM_SYNC_OWNER);
            em->scriptPending = 0;
            break;
        }
        break;
    case EM_DMG_PART8_DOWN:
        if (em->type == EM_KIND_KUSHALA_DAORA || em->type == EM_KIND_RUSTED_KUSHALA_DAORA) {
            EmKushala_ChangeState(em, EM_GROUP_REACT, KUSH_REACT_PART_BREAK, EM_SYNC_OWNER);
        } else if (em->partBreakCount > 0) {
            EmKushala_ChangeState(em, EM_GROUP_REACT, KUSH_REACT_PART_BREAK, EM_SYNC_OWNER);
        } else {
            EmKushala_ChangeState(em, EM_GROUP_REACT, KUSH_REACT_FIRST_BREAK_FLINCH, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_PART_FLINCH:
        if (em->stance == EM_STANCE_AIR) {
            if (em->flinchPart == 0) {
                EmKushala_ChangeState(em, EM_GROUP_REACT, KUSH_REACT_AIR_KNOCK_DOWN, EM_SYNC_OWNER);
            } else {
                EmKushala_ChangeState(em, EM_GROUP_REACT, KUSH_REACT_FALL, EM_SYNC_OWNER);
            }
        } else if (em->stance == EM_STANCE_FAMILY) {
            if (em->flinchPart == 0) {
                EmKushala_ChangeState(em, EM_GROUP_REACT, KUSH_REACT_HOVER_KNOCK_DOWN, EM_SYNC_OWNER);
            } else {
                EmKushala_ChangeState(em, EM_GROUP_REACT, KUSH_REACT_FLINCH_HOVER, EM_SYNC_OWNER);
            }
        } else {
            EmKushala_BreakParts(em, 1);
            switch (em->flinchPart) {
            case 0:
                EmKushala_ChangeState(em, EM_GROUP_REACT, KUSH_REACT_KNOCK_DOWN, EM_SYNC_OWNER);
                break;
            case 5:
            case 1:
            case 2:
                EmKushala_ChangeState(em, EM_GROUP_REACT, KUSH_REACT_FLINCH, EM_SYNC_OWNER);
                break;
            default:
                if (em->parts[em->flinchPart].state >= 2) {
                    EmKushala_ChangeState(em, EM_GROUP_REACT, KUSH_REACT_FLINCH_HEAVY, EM_SYNC_OWNER);
                } else {
                    EmKushala_ChangeState(em, EM_GROUP_REACT, KUSH_REACT_FLINCH, EM_SYNC_OWNER);
                }
                break;
            }
        }
        break;
    case EM_DMG_HIT_MODE3:
        if (em->stance != EM_STANCE_AIR) {
            EmKushala_ChangeState(em, EM_GROUP_REACT, KUSH_REACT_FLINCH, EM_SYNC_OWNER);
            em->scriptPending = 0;
        }
        break;
    case EM_DMG_STATUS5:
        if ((em->stateGroup != EM_GROUP_REACT || em->state != KUSH_REACT_STUN) && (em->stateGroup != EM_GROUP_REACT || em->state != KUSH_REACT_FALL) &&
            (em->stateGroup != EM_GROUP_REACT || em->state != KUSH_REACT_AIR_KNOCK_DOWN)) {
            Em_SetStatus5Timer(em);
            EmKushala_ChangeState(em, EM_GROUP_REACT, KUSH_REACT_STUN, EM_SYNC_OWNER);
        }
        break;
    }
    if (Em_DamageResultIsReaction(em, damage) == 1) {
        EmKushala_BreakParts(em, 0);
    }
    switch (gQuest.stage) {
    case QUEST_ID_LUNASTRA_TEOSTRA_VOLCANO_7_SCENE_1:
    case QUEST_ID_LUNASTRA_TEOSTRA_VOLCANO_7_SCENE_2:
        if ((em->type == EM_KIND_LUNASTRA || em->type == EM_KIND_TEOSTRA) && Area_DayId(em->area) == AREA_VOLCANO_7 && Save_TestEventFlag(EVENT_FLAG_LUNASTRA_TEOSTRA_VOLCANO_7_SCENE) == 0) {
            if (Area_DayId(gScene.area) == AREA_VOLCANO_7 && gScene.eventRunning == 1 && em->stateGroup != EM_GROUP_EVENT) {
                EmKushala_ChangeState(em, EM_GROUP_EVENT, KUSH_EVENT_SCENE_DESCEND, EM_SYNC_SEND);
            }
            break;
        }
        goto normal;
    case QUEST_ID_KUSHALA_JUNGLE_SCENE:
        if ((em->type == EM_KIND_KUSHALA_DAORA || em->type == EM_KIND_RUSTED_KUSHALA_DAORA) && Area_DayId(em->area) == AREA_JUNGLE_3 && Save_TestEventFlag(EVENT_FLAG_KUSHALA_JUNGLE_SCENE) == 0) {
            if (Area_DayId(gScene.area) == AREA_JUNGLE_3 && gScene.eventRunning == 1 && em->stateGroup != EM_GROUP_EVENT) {
                EmKushala_ChangeState(em, EM_GROUP_EVENT, KUSH_EVENT_SCENE_FLY_IN, EM_SYNC_SEND);
            }
            break;
        }
        goto normal;
    case QUEST_ID_KUSHALA_SNOW_SCENE:
        if ((em->type == EM_KIND_KUSHALA_DAORA || em->type == EM_KIND_RUSTED_KUSHALA_DAORA) && Area_DayId(em->area) == AREA_SNOWY_MOUNTAINS_6 && Save_TestEventFlag(EVENT_FLAG_KUSHALA_SNOW_SCENE) == 0) {
            if (Area_DayId(gScene.area) == AREA_SNOWY_MOUNTAINS_6 && gScene.eventRunning == 1 && em->stateGroup != EM_GROUP_EVENT) {
                EmKushala_ChangeState(em, EM_GROUP_EVENT, KUSH_EVENT_SCENE_WALK_IN, EM_SYNC_SEND);
            }
            break;
        }
        goto normal;
    case QUEST_ID_LUNASTRA_TEOSTRA_VOLCANO_8_SCENE:
        if ((em->type == EM_KIND_LUNASTRA || em->type == EM_KIND_TEOSTRA) && Area_DayId(em->area) == AREA_VOLCANO_8 && Save_TestEventFlag(EVENT_FLAG_LUNASTRA_TEOSTRA_VOLCANO_8_SCENE) == 0) {
            if (Area_DayId(gScene.area) == AREA_VOLCANO_8 && gScene.eventRunning == 1 && em->stateGroup != EM_GROUP_EVENT) {
                EmKushala_ChangeState(em, EM_GROUP_EVENT, KUSH_EVENT_SCENE_RISE, EM_SYNC_SEND);
            }
            break;
        }
        goto normal;
    case QUEST_ID_LUNASTRA_TEOSTRA_TOWER_SCENE:
        if ((em->type == EM_KIND_LUNASTRA || em->type == EM_KIND_TEOSTRA) && Area_DayId(em->area) == AREA_TOWER_0x7C && Save_TestEventFlag(EVENT_FLAG_LUNASTRA_TEOSTRA_TOWER_SCENE) == 0) {
            if (Area_DayId(gScene.area) == AREA_TOWER_0x7C && gScene.eventRunning == 1 && em->stateGroup != EM_GROUP_EVENT) {
                EmKushala_ChangeState(em, EM_GROUP_EVENT, KUSH_EVENT_SCENE_WAKE, EM_SYNC_SEND);
            }
            break;
        }
    default:
    normal: /* a story stage whose scene doesn't apply joins the default here */
        if (Quest_GetOutcome(0) == 1 && em->stateGroup != EM_GROUP_EVENT && em->stateGroup != EM_GROUP_DEFEAT) {
            em->scriptPending = 0;
            if (em->stance == EM_STANCE_AIR) {
                EmKushala_ChangeState(em, EM_GROUP_EVENT, KUSH_EVENT_LEAVE_FROM_AIR, EM_SYNC_LOCAL);
            } else {
                if (gScene.area == em->area) {
                    CamReq_ForMonster(em);
                }
                EmKushala_ChangeState(em, EM_GROUP_EVENT, KUSH_EVENT_LEAVE, EM_SYNC_LOCAL);
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
    if (em->stateGroup == EM_GROUP_FLY && (s8) w->flyGuard == 1) {
        em->guardTimer = 2;
    }
}

/*
 * Remembers the current state as the previous one, then runs the state's
 * group dispatcher (group 7 shares group 6's).
 */
void EmKushala_RunState(Em *em, s32 work)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->stateGroup) {
    case EM_GROUP_IDLE:
        EmKushala_RunIdleState(em, (EmWork *) work);
        break;
    case EM_GROUP_MOVE:
        EmKushala_RunMoveState(em, (EmWork *) work);
        break;
    case EM_GROUP_FLY:
        EmKushala_RunFlyState(em, (EmWork *) work);
        break;
    case EM_GROUP_ATTACK:
        EmKushala_RunAttackState(em, (EmWork *) work);
        break;
    case EM_GROUP_REACT:
        EmKushala_RunReactState(em, (EmWork *) work);
        break;
    case EM_GROUP_DEFEAT:
        EmKushala_RunDefeatState(em, (EmWork *) work);
        break;
    case EM_GROUP_EVENT:
        EmKushala_RunEventState(em, (EmWork *) work);
        break;
    case EM_GROUP_EVENT2:
        EmKushala_RunEventState(em, (EmWork *) work);
        
    default:
        break;
    }
}

/*
 * Returns 1 in the ground states (groups 0, 1 and 3) where the wind's ring
 * replaces the footfall dust.
 */
s32 EmKushala_StateHasWindRing(Em *em)
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

/*
 * Puts the dust effect at a bone, at floor height, when a player is in the
 * area.
 */
void EmKushala_SpawnGroundEffect(Em *em, s32 bone, s32 effectKind, f32 scale)
{
    f32 pos[3];

    if (Player_InCurrentArea(em)) {
        Em_BonePos(em, bone, pos);
        pos[1] = em->floorY;
        PartFx_SpawnOnGround(pos, effectKind, scale * em->scaleX);
    }
}

/*
 * Puts the dust at a bone within 1000 of the floor, unless the wind's ring
 * replaces it; with withImpact also an impact attack at the monster.
 */
void EmKushala_SpawnDustNearGround(Em *em, s32 bone, s32 effectKind, s32 flag, f32 scale)
{
    if (EmKushala_WindActive(em) != 1 || EmKushala_StateHasWindRing(em) != 1) {
        if (em->posY <= (1000.0f + em->floorY)) {
            EmKushala_SpawnGroundEffect(em, bone, effectKind, scale);
            if (flag != 0) {
                EmImpactAttack_CreateAt(em, (f32 *) &em->posX, 1U);
            }
        }
    }
}

/*
 * The same as EmKushala_SpawnDustNearGround with the ground stage object
 * (StageObj_K14) instead of the dust.
 */
void EmKushala_SpawnK14NearGround(Em *em, s32 k, s32 variant, s32 flag, f32 scale)
{
    if (EmKushala_WindActive(em) != 1 || EmKushala_StateHasWindRing(em) != 1) {
        if (em->posY <= (1000.0f + em->floorY)) {
            DustCloud_Create((Em *) em, k, variant, scale);
            if (flag != 0) {
                EmImpactAttack_CreateAt(em, (f32 *) &em->posX, 1U);
            }
        }
    }
}

/* The default of EmKushala_MotionEffects: nothing. */
static void EmKushala_MotionEffectsNop(Em *em)
{
}

/*
 * While enraged, every 45 frames puts an aura puff on the monster in its
 * moving and idle motions.
 */
void EmKushala_SpawnRageAura(Em *em, s16 motion)
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
            MonsterFx_CreateOnEm(em, 0xD, 2.0f * em->scaleX);
        }
        break;
    }
}

void Explosion_CreateOwned(Player *owner, f32 *pos, s32 kind);
/*
 * Each frame: keeps the current motion in fxMotion, plays its sounds
 * (EmKushala_MotionSounds or EmKushala_MotionSoundsTesca), the rage aura,
 * then the motion's dust, stage objects and attacks at their frames.
 */
void EmKushala_MotionEffects(Em *em, EmWork *work)
{
    f32 burstPos[3];
    f32 aheadOfs[3];
    f32 puffOfsA[3];
    f32 puffOfsB[3];

    if (em->motionId[0] != work->fxMotion) {
        work->fxMotion = em->motionId[0];
    }
    if ((em->type == EM_KIND_KUSHALA_DAORA) || (em->type == EM_KIND_RUSTED_KUSHALA_DAORA)) {
        EmKushala_MotionSounds(em, work);
    } else {
        EmKushala_MotionSoundsTesca(em, work);
    }
    EmKushala_SpawnRageAura(em, (s32) work->fxMotion);
    switch (work->fxMotion) {
    case EM_MOTION_ID(0, KUSH_MOT_TURN_BIG_RIGHT):
        if (EmMotion_PassedFrameScaled(em, 8.0f, 0) != 0) {
            EmKushala_SpawnGroundEffect(em, 0x1F, 3, 8.0f * em->scaleX);
        }
        if (EmMotion_PassedFrameScaled(em, 25.0f, 0) != 0) {
            EmKushala_SpawnGroundEffect(em, 0x19, 3, 8.0f * em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_TURN_BIG_LEFT):
        if (EmMotion_PassedFrameScaled(em, 8.0f, 0) != 0) {
            EmKushala_SpawnGroundEffect(em, 0x19, 3, 8.0f * em->scaleX);
        }
        if (EmMotion_PassedFrameScaled(em, 25.0f, 0) != 0) {
            EmKushala_SpawnGroundEffect(em, 0x1F, 3, 8.0f * em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_TAKE_OFF):
        if (EmMotion_PassedFrameScaled(em, 46.0f, 0) != 0) {
            EmKushala_SpawnK14NearGround(em, 1, 0, 0, 1.5f * em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_HOVER):
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            EmKushala_SpawnK14NearGround(em, 1, 0, 0, 1.5f * em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_DASH_START):
        if (EmMotion_PassedFrameScaled(em, 60.0f, 0) != 0) {
            EmKushala_SpawnK14NearGround(em, 1, 0, 0, 1.5f * em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_DASH_END):
        if ((EmMotion_PassedFrameScaled(em, 26.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 76.0f, 0) != 0)) {
            EmKushala_SpawnK14NearGround(em, 9, 0, 0, em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_LAND):
        if (EmMotion_PassedFrameScaled(em, 8.0f, 0) != 0) {
            EmKushala_SpawnDustNearGround(em, 1, 7, 0, 10.0f * em->scaleX);
        }
        if (EmMotion_PassedFrameScaled(em, 66.0f, 0) != 0) {
            EmKushala_SpawnDustNearGround(em, 1, 0xC, 0, 12.0f * em->scaleX);
        }
        if (EmMotion_PassedFrameScaled(em, 70.0f, 0) != 0) {
            EmImpactAttack_CreateAt(em, (f32 *) &em->posX, 2);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_HOVER_TURN):
        if (EmMotion_PassedFrameScaled(em, 8.0f, 0) != 0) {
            EmKushala_SpawnK14NearGround(em, 1, 0, 0, em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_SIDE_DASH_A):
        if (EmMotion_PassedFrameScaled(em, 4.0f, 0) != 0) {
            EmKushala_SpawnK14NearGround(em, 1, 0, 0, em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_SIDE_DASH_B):
        if (EmMotion_PassedFrameScaled(em, 4.0f, 0) != 0) {
            EmKushala_SpawnDustNearGround(em, 1, 7, 0, 10.0f * em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_RUN_START):
        if (EmMotion_PassedFrameScaled(em, 18.0f, 0) != 0) {
            EmKushala_SpawnGroundEffect(em, 8, 3, 8.0f * em->scaleX);
        }
        if (EmMotion_PassedFrameScaled(em, 36.0f, 0) != 0) {
            EmKushala_SpawnGroundEffect(em, 0xD, 3, 8.0f * em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_RUN_B):
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 2, 0x1A, 1.5f * em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_RUN_C):
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 2, 0x1A, 1.5f * em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_REAR_UP):
        if (EmMotion_PassedFrameScaled(em, 46.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 2, 0x1B, 2.0f * em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_STOMP):
        if (EmMotion_PassedFrameScaled(em, 16.0f, 0) != 0) {
            EmKushala_CreateAttack(em, 0xB);
        }
        if (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) {
            EmKushala_SpawnGroundEffect(em, 1, 7, 10.0f * em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_LIFT_TO_HOVER):
        if (EmMotion_PassedFrameScaled(em, 24.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 1, 0, em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_ROAR):
        if ((em->type == EM_KIND_KUSHALA_DAORA) || (em->type == EM_KIND_RUSTED_KUSHALA_DAORA)) {
            if (EmMotion_PassedFrameScaled(em, 62.0f, 0) != 0) {
                EmKushala_CreateAttack(em, 0x1C);
                ElementBurst_CreateOnEm(em, 5, 3, 1.0f);
            }
            if ((EmKushala_WindActive(em) == 1) && (EmMotion_PassedFrameScaled(em, 186.0f, 0) != 0)) {
                MonsterFx_CreateOnEm(em, 0x10, em->scaleX);
                EmImpactAttack_CreateAt(em, (f32 *) &em->posX, 4);
                break;
            }
        } else {
            if (EmMotion_PassedFrameScaled(em, 62.0f, 0) != 0) {
                EmKushala_CreateAttack(em, 0x1C);
                ElementBurst_CreateOnEm(em, 5, 3, 1.0f);
            }
            if ((EmKushala_WindActiveTesca(em) == 1) && (EmMotion_PassedFrameScaled(em, 186.0f, 0) != 0)) {
                MonsterFx_CreateOnEm(em, 0x10, em->scaleX);
                EmImpactAttack_CreateAt(em, (f32 *) &em->posX, 4);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_TESCA_ACTION_C_END):
        if (((em->type == EM_KIND_LUNASTRA) || (em->type == EM_KIND_TEOSTRA)) && (EmMotion_PassedFrameScaled(em, 16.0f, 0) != 0)) {
            EmKushala_SpawnGroundEffect(em, 1, 7, 10.0f * em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_EAT_START):
        break;
    case EM_MOTION_ID(0, KUSH_MOT_REST):
        if (EmMotion_ReachedFrame(em, 0, 220.0f) != 0) {
            puffOfsA[0] = 30.0f;
            puffOfsA[1] = 30.0f;
            puffOfsA[2] = 200.0f;
            Em_SpawnLyingPuffs(em, 0x25, puffOfsA, 1.6f);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_BREATH_STREAM_END):
        if (((em->type == EM_KIND_LUNASTRA) || (em->type == EM_KIND_TEOSTRA)) && (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0)) {
            EmKushala_SpawnK14NearGround(em, 1, 0, 0, em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_TESCA_ACTION_C):
        if ((em->type == EM_KIND_LUNASTRA) || (em->type == EM_KIND_TEOSTRA)) {
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
    case EM_MOTION_ID(0, KUSH_MOT_TESCA_ACTION_B):
        if ((em->type == EM_KIND_LUNASTRA) || (em->type == EM_KIND_TEOSTRA)) {
            if (EmMotion_PassedFrameScaled(em, 40.0f, 0) != 0) {
                DustCloud_Create((Em *) em, 2, 0x1E, 1.5f * em->scaleX);
            }
            if (EmMotion_PassedFrameScaled(em, 32.0f, 0) != 0) {
                EmKushala_CreateAttack(em, 0x1E);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_TESCA_ACTION_A):
        if ((em->type == EM_KIND_LUNASTRA) || (em->type == EM_KIND_TEOSTRA)) {
            if (EmMotion_PassedFrameScaled(em, 122.0f, 0) != 0) {
                EmKushala_CreateAttack(em, 5);
            }
            if (EmMotion_PassedFrameScaled(em, 124.0f, 0) != 0) {
                DustCloud_Create((Em *) em, 2, 0x20, 2.0f * em->scaleX);
            }
            if (EmMotion_PassedFrameScaled(em, 162.0f, 0) != 0) {
                EmKushala_SpawnGroundEffect(em, 0x12, 0xC, 8.0f * em->scaleX);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, 0x2C):
        if (EmMotion_PassedFrameScaled(em, 32.0f, 0) != 0) {
            EmKushala_CreateAttack(em, 5);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_BREATH_FINISH):
        if (EmMotion_PassedFrameScaled(em, 32.0f, 0) != 0) {
            EmKushala_CreateAttack(em, 6);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_STRIKE_BREATH):
        if (EmMotion_PassedFrameScaled(em, 60.0f, 0) != 0) {
            EmKushala_CreateAttack(em, 0xC);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_BREATH_CHAIN):
        if (EmMotion_PassedFrameScaled(em, 90.0f, 0) != 0) {
            EmKushala_CreateAttack(em, 0x11);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_BREATH_WINDUP):
        if ((EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 60.0f, 0) != 0)) {
            EmKushala_SpawnK14NearGround(em, 1, 0, 0, em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_BREATH_STREAM):
        if ((em->type == EM_KIND_KUSHALA_DAORA) || (em->type == EM_KIND_RUSTED_KUSHALA_DAORA)) {
            if (EmMotion_PassedFrameScaled(em, 24.0f, 0) != 0) {
                u8 map = gScene.map;

                if (map == MAP_SNOWY_MOUNTAINS || map == MAP_SNOWY_MOUNTAINS_NIGHT) {
                    EmRangedAttack_CreateAtBoneTimed(em, 0x25, 0x12, 1, 0x2000, 0, (s32) (131.0f * em->motionSpeed));
                } else {
                    EmRangedAttack_CreateAtBoneTimed(em, 0x25, 0x12, 0, 0x2000, 0, (s32) (131.0f * em->motionSpeed));
                }
            }
        } else if (EmMotion_PassedFrameScaled(em, 24.0f, 0) != 0) {
            EmEffectAttack_CreateChain(em, EMFX_BREATH_CHAIN, 0, 0x2000, 0);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_BREATH):
        break;
    case EM_MOTION_ID(0, KUSH_MOT_LUNGE):
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            EmKushala_SpawnDustNearGround(em, 1, 7, 0, 10.0f * em->scaleX);
        }
        if ((EmMotion_PassedFrameScaled(em, 52.0f, 0) != 0) && Player_InCurrentArea(em) && (em->posY <= (1000.0f + em->floorY))) {
            aheadOfs[0] = 0.0f;
            aheadOfs[1] = 0.0f;
            aheadOfs[2] = 400.0f * em->scaleZ;
            Vec_RotateY(aheadOfs, (6.2831855f * (f32) em->rotY) / 65536.0f);
            burstPos[0] = em->posX + aheadOfs[0];
            burstPos[1] = em->floorY;
            burstPos[2] = em->posZ + aheadOfs[2];
            MonsterFx_CreateAtYaw(burstPos, (u16) em->rotY, 0x11, 1.2f);
            Explosion_CreateOwned((Player *)em, burstPos, EXPLOSION_KUSHALA_GROUND_BLAST);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_HOVER_ACTION_A):
        if (EmMotion_PassedFrameScaled(em, 50.0f, 0) != 0) {
            EmKushala_CreateAttack(em, 2);
        }
        if (EmMotion_PassedFrameScaled(em, 56.0f, 0) != 0) {
            EmKushala_SpawnGroundEffect(em, 1, 0xC, 8.0f * em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_HOVER_ACTION_B):
        if (EmMotion_PassedFrameScaled(em, 42.0f, 0) != 0) {
            EmKushala_CreateAttack(em, 0x12);
        }
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            EmKushala_SpawnK14NearGround(em, 1, 0, 0, em->scaleX);
        }
        if (EmMotion_PassedFrameScaled(em, 64.0f, 0) != 0) {
            EmKushala_SpawnDustNearGround(em, 1, 7, 0, 10.0f * em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_TURN_SWEEP_A):
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
            DustCloud_Create((Em *) em, 2, 0x1E, 1.5f * em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_TURN_SWEEP_B):
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
            DustCloud_Create((Em *) em, 3, 0x1C, 1.5f * em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_HOVER_SWEEP_A):
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
            DustCloud_Create((Em *) em, 3, 0x1F, 1.5f * em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_HOVER_SWEEP_B):
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
            EmKushala_SpawnDustNearGround(em, 1, 7, 0, 10.0f * em->scaleX);
        }
        if (EmMotion_PassedFrameScaled(em, 60.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 2, 0x1F, 1.5f * em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_TOPPLE_RECOVER):
        if (EmMotion_PassedFrameScaled(em, 20.0f, 0) != 0) {
            EmKushala_SpawnGroundEffect(em, 1, 7, 8.0f * em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_FLINCH_HOVER):
        if ((EmMotion_PassedFrameScaled(em, 22.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 70.0f, 0) != 0)) {
            EmKushala_SpawnK14NearGround(em, 1, 0, 0, em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_FALL_LAND):
        if (EmMotion_PassedFrameScaled(em, 16.0f, 0) != 0) {
            EmKushala_SpawnK14NearGround(em, 0xB, 0, 0, em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_DOWN):
        if (EmMotion_PassedFrameScaled(em, 60.0f, 0) != 0) {
            EmKushala_SpawnGroundEffect(em, 1, 0xC, 8.0f * em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_DIE):
        if (EmMotion_PassedFrameScaled(em, 420.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 0, 0, em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_COLLAPSE):
        if (EmMotion_PassedFrameScaled(em, 190.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 0, 0, em->scaleX);
        }
        if (EmMotion_ReachedFrame(em, 0, 228.0f) != 0) {
            puffOfsB[0] = -30.0f;
            puffOfsB[1] = 30.0f;
            puffOfsB[2] = 200.0f;
            Em_SpawnLyingPuffs(em, 0x25, puffOfsB, 1.6f);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_TOPPLE):
        if (EmMotion_PassedFrameScaled(em, 134.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 0, 0, em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_PART_BREAK):
        if (EmMotion_PassedFrameScaled(em, 60.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 0, 0, em->scaleX);
            break;
        }
        break;
    default:
        EmKushala_MotionEffectsNop(em);
        break;
    }
}

/*
 * Callback slot 3: skips the first frame, then runs EmKushala_MotionEffects
 * every frame.
 */
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

/*
 * Creates the family's contact attack of the given kind (KushalaAttack for
 * Kushala Daora, TeostraAttack for Lunastra and Teostra).
 */
void EmKushala_CreateAttack(Em *em, s32 kind)
{
    u8 type;

    type = em->type;
    if ((type == EM_KIND_KUSHALA_DAORA) || (type == EM_KIND_RUSTED_KUSHALA_DAORA)) {
        KushalaAttack_Create(em, kind);
        return;
    }
    TeostraAttack_Create(em, kind);
}

/*
 * The blast's flash: when a player is in the area, an effect and a stage
 * object in front of bone 0x27.
 */
void EmKushala_SpawnBlastBurst(Em *em)
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
        HitSpark_SpawnAt(pos, 0, em->rotY);
        HitSplash_CreateAtScaled(pos, 6, 1.0f);
    }
}

/* Callback slot 2: nothing. */
void EmKushala_TableNop(void)
{
}

/*
 * Callback slot 6: returns 1 when part 8 may go down: on the ground, at 30%
 * hp or less with no break yet (Lunastra and Teostra: 40% or less, at most
 * one break).
 */
s32 EmKushala_AllowHit8(Em *em)
{
    if (em->stance != EM_STANCE_AIR && em->stance != EM_STANCE_FAMILY) {
        if (em->type == EM_KIND_KUSHALA_DAORA || em->type == EM_KIND_RUSTED_KUSHALA_DAORA) {
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

/* The file's initialised data, in the game's order */
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
s32 EmKushala_BreathTimers[6][2] = {
#include "game/008CA210.inc"
};
void *EmKushala_Callbacks[8] = {
    EmKushala_SpawnEffects, EmKushala_SpawnEffects, EmKushala_TableNop, EmKushala_UpdateMotionEffects,
    GmEm_ReturnZero, EmKushala_Reinit, EmKushala_AllowHit8, NULL,
};
