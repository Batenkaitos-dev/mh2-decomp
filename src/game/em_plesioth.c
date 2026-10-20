/*
 * The Plesioth family (EM_KIND_PLESIOTH, EM_KIND_GREEN_PLESIOTH): the code
 * from the family's first function, EmPlesioth_MotionSounds, to the
 * trader's.
 *
 * A Plesioth runs a state machine (include/em_plesioth.h names the states
 * and motions; include/em.h the groups). It lives in water and on land:
 * EM_STANCE_UNDER under water, where the group of the flying families
 * (EM_GROUP_FLY) holds its swimming states, and EM_STANCE_GROUND ashore.
 * EmPlesioth_Update moves it between the two by the water's depth
 * (EmPlesioth_OverDeepWater), reads the hits (Em_UpdateDamage) and picks
 * reactions, then EmPlesioth_RunState calls the group's dispatcher
 * (EmPlesioth_RunIdleState ... RunEventState), which calls one function per
 * state with the monster and its work area (EmWork; the family's view is
 * em.h's EmPlesiothWork). A state function switches on Em.step;
 * EmPlesioth_ChangeState starts the next state through the group's Set
 * function, and EmPlesioth_EndGroundState / EndSwimState hand the choice
 * back to the monster script. A hunter's bait can hook it under water and
 * pull it ashore (EmPlesioth_WaitForBait, HookedPulled, PulledAshore).
 * Leaps follow curves (EmPlesioth_StartCurves, UpdateCurves); the fins wave
 * and fold by key tables (EmPlesioth_UpdateFins); EmPlesioth_MotionEffects
 * adds each motion's attacks, splashes and effects.
 */
#include "common.h"
#include "scene.h"
#include "quest.h"
#include "em.h"
#include "em_plesioth.h"
#include "em_cephadrome.h"
#include "player.h"
#include "area.h"
s32 Sound_RandomSe(s32 id, s16 lo, s32 alt, s16 n);

void EmPlesioth_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void Em_BeginStatus1(Em *em);
void Em_BeginStatus2(Em *em);
void Em_EndStatus1(Em *em);
void Em_EndStatus2(Em *em);
void Em_EndStatus4(Em *em);
void Em_EndStatus5(Em *em);
void Em_PlaySeAtBone(Em *em, s32 se, s32 bone);
void Em_PlaySeAtBoneOnFrame(Em *em, s32 frame, s32 se, s32 bone);
void Em_SetLayerMotion(Em *em, s32 motion, s32 blend, s32 frame, s32 layer);
void Em_SetMotion(Em *em, s32 motion, s32 a, s32 b);
/* unit's own declaration: declared here as the callers use it; with the definition's void (Em *em, s32 group, s32 state, u8 mode), the calls in EmApceros_ChangeState, EmPlesioth_ChangeState and others compile differently */
void Em_SetStateSync(Em *em, u16 group, u16 state, u8 mode);
u16 Math_AngleXZ(f32 *from, f32 *to);
void Player_ClearFlag(void *, u32 flag);
void Player_SetFlag(void *obj, u32 flag);
void QuestEm_Captured(Em *em);
void QuestEm_Hunted(Em *em);
u16 Rand_Next(s32 stream);
void EmRangedAttack_CreateAtBoneTimed(Em *, s32, s32, s32, s32, s32, s32);
void Em_MoveRotated(Em *em, void *src);
void Em_MoveRotatedAccel(Em *em, void *ang);
void Em_SetStateRand(Em *em, s32 arg1, s32 arg2);
void Em_ComputeGoal2(Em *em, f32 *target);
void Em_GetPitchToGoal(Em *em, s32 *out, f32 a, f32 b);
void Em_ClearLowHpNeed(Em *em);
void Em_SpawnStatus4Effect(Em *em, s32 a);
void Em_SetHitReactMode(Em *em, u8 a);
void Em_SpawnLyingPuffs(Em *em, s32 bone, f32 *pos, f32 scale);
void Em_SetTrapTimeGoal(Em *em);
void Em_ApplyAreaValues(Em *em);
void Em_ClearPoison(Em *em);
void Em_SetHold2(Em *em);
void Em_SetCaptured(Em *em, s32 a);
void Em_PlaySeTypeAtBoneInFrames(Em *em, s32 frame, s32 frame2, s32 se, s32 bone, s32 vol, s32 a);
void Em_PlaySeTypeAtBoneOnFrame(Em *em, s32 frame, s32 se, s32 bone);
void Em_PlaySeAtBoneVolOnFrame(Em *em, s32 frame, s32 se, s32 bone, s32 vol);
s32 Em_StepToGoal(Em *em, u16 step, u8 flags);
u8 Em_TurnToGoal(Em *em, u16 step, u8 flags);
void Em_StartTurnToTarget(Em *em, EmTurnParams *t);
s32 Em_TurnStep(Em *em, EmTurnParams *t);
void Em_RestartScript(Em *em);
void EmPlesioth_Init(Em *);
void EmPlesioth_SetStance(Em *, u8, u8);
void EmPlesioth_LeapOutOfWater(Em *, EmWork *);
void EmPlesioth_Captured(Em *, EmWork *);
void EmPlesioth_EntranceScene(Em *, EmWork *);
void EmPlesioth_Die(Em *, EmWork *);
void EmPlesioth_DieHeld(Em *, EmWork *);
void EmPlesioth_DieUnder(Em *, EmWork *);
void EmPlesioth_DieAshore(Em *, EmWork *);
void EmPlesioth_RunIdleState(Em *em, EmWork *work);
void EmPlesioth_RunSwimState(Em *, EmWork *);
void EmPlesioth_RunReactState(Em *em, EmWork *work);
void EmPlesioth_MotionEffects(Em *, EmWork *);
void EmPlesioth_UpdateFins(Em *em, EmWork *work);
void EmPlesioth_SpawnWakeEvery4(Em *em, f32 scale);
void EmPlesioth_SpawnSurfaceSplash(Em *em, f32 scale);
void EmPlesioth_SpawnWakeEvery8(Em *em, f32 scale);
void EmPlesioth_StartCurves(Em *em, u8 kind);
u8 EmPlesioth_UpdateCurves(Em *);
s32 EmPlesioth_OverDeepWater(Em *em, u8 arg1);
void AlertMark_Create(void *owner, s16 v, u8 k);
void DustCloud_Create(Em *em, s16 k, s16 c, f32 scale);
/* unit's own declaration: declared here as the callers use it; with the definition's void (Em *em, u8 k, s16 v, s16 t, f32 y, f32 scale), the calls in EmPlesioth_HeldStatus compile differently */
void StunStars_CreateEm(Em *, s32, s32, s32, f32, f32);
s32 EmMotion_PassedFrameScaled(void *p, f32 frame, s32 layer);
s32 EmMotion_ReachedFrame(void *obj, s32 a, f32 frame);
s32 EmMotion_InFrameRange(void *em, s32 layer, f32 from, f32 to);
void CamShake_ReqAIf(Em *obj, s32 i);
void Vec_Set(f32 *out, f32 x, f32 y, f32 z);
f32 Vec_DistanceXZ(f32 *a, f32 *b);
s32 QuestEm_CanAppear(Em *em);
void Player_ClearVelocity(void *);
void Player_ClearAccel2(void *obj);
void Motion_InitTime(void *p, s16 step);
u8 Player_InCurrentArea(void *obj);
void Shadow_Spawn(void *obj, s16 type);
void Effect08_Spawn(f32 *pos, s32 k, s32 b, f32 scale);
extern EmTurnParams EmPlesioth_TurnParams[3];
void Em_TargetPlayerInRange(Em *);
void Versus_MarkDefeat(Em *);
s8 QuestEm_Appear(Em *em);
void Em_ResetToBasePose(Em *em);
void Em_BreakPart(Em *em, s32 part);
void Em_EndAllStatus(Em *em);
void Em_BeginStatus4(Em *em);
void Em_BeginStatus5(Em *em);
void Em_LoadReactionSet(Em *em, u8 k);
void Em_SetSpeedScaled(Em *em, u16 flags, f32 x, f32 y);
void Em_CheckRageByHp(Em *em, s16 n);
void Em_ComputeGoal(Em *em, f32 *target);
void Em_TypeSetup(Em *em);
void Em_SetTargetAngle(Em *em);
s32 Em_GetTargetPos(Em *em, u8 idx, f32 *out);
void Em_StartRage(Em *em);
void Em_StartPendingRage(Em *em);
void Em_ResetNotice(Em *em);
void Em_RefillNeed3(Em *em);
void Em_StartGoalAction(Em *em, s32 arg1);
void EmPlesioth_SetMoveState(Em *em, u16 state, u8 mode);
void EmPlesioth_SetSwimState(Em *em, u16 state, u8 mode);
void EmPlesioth_SetAttackState(Em *em, u16 state, u8 mode);
u8 EmPlesioth_NextAreaPoint(Em *em, f32 *out);
void EmPlesioth_SetIdleState(Em *em, u16 state, u8 mode);
void Em_ClearPart(Em *em, u16 part);
void Em_AddHp(Em *em, s16 n);
void Em_AddNeed1(Em *em, s32 n);
void Em_AddNeed2(Em *em, s32 n);
void Em_ClearNeed1State(Em *em);
void Em_ClearNeed2State(Em *em);
/* unit's own declaration: declared here as the callers use it; with the definition's s16 (void *obj, u8 state, u8 sub), the calls in EmTrader_TurnToYaw and EmPlesioth_WaitForBait compile differently */
s32 Player_IsState(void *obj, s32 group, s32 state);
void MinePoint_Remove(s32);
void Em_SetMotionRow(Em *em, s32 row, s32 arg2, s32 arg3);
void Em_SetPartScaled(Em *em, s32 bit);
s32 Em_ComputeMaxHp(Em *em, s16 idx);
void EmTail_Create(Em *);
void EmPlesioth_LeapIntoWater(Em *em, EmWork *work, u8 variant);
void EmPlesioth_KnockDown(Em *em, EmWork *work, s16 variant);
void EmPlesioth_FallFromAir(Em *em, EmWork *work);
void EmPlesioth_FallBlinded(Em *em, EmWork *work);
void EmPlesioth_PulledAshore(Em *em, EmWork *work);
void EmPlesioth_Status1Under(Em *em, EmWork *work);
void EmPlesioth_LandFlop(Em *em, EmWork *work, u8 variant);
s8 EmMine_Create(Em *em, s32 idx);
s32 EmMine_Update(Em *em);
void EmMine_Remove(Em *em);
extern EmCurveKey *EmPlesioth_VelYCurves[];
extern EmCurveKey *EmPlesioth_SpeedCurves[];
extern u16 EmPlesioth_FinFoldTarget[];
s32 Obj_SameArea(void *obj, Player *b);
void Vec_Rotate(f32 *out, f32 *in, f32 *mtx);
struct EmIdEntry;
f32 *Em_AreaEntryValue(Em *em, struct EmIdEntry *entry);
s32 Em_UpdateCombatTimer(Em *em);
u8 Em_UpdateDamage(Em *em, u8 *out);
void Em_SetStatus4Timer(Em *em);
void Em_SetStatus2Timer(Em *em);
void Em_UpdatePendingScript(Em *em);
void Em_ClearHitReactMode(Em *em);
u16 Area_DayId(u16 id);
void Em_HoldPosNoPlayers(Em *em, u16 a, u16 b);
void Em_SetTrapTime(Em *em, u8 *p);
void Em_SetStatus1Timer(Em *em);
void Em_SetStatus5Timer(Em *em);
void Em_ResetKindTimer(Em *em);
void EmCephadrome_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void EmPlesioth_RunState(Em *em, EmWork *work);
s32 Player_IsLocal(void *obj);
s32 Lure_PlayerHasMonsterBait(void *pl);
void Vib_StartLocal(void *port, s32 i);
f32 *Mtx_FromAnglesYXZ(s32 *angles, f32 *m);
void CamReq_PlesiothScene(void *arg);
extern s16 gEmCombatInit[];
s8 EmMine_CreateTimed0(Em *em, f32 *pos);
void PartFx_SpawnForEmScaled(Em *em, s32 a, s32 b, f32 scale);
void PlesiothAttack_Create(Em *em, s8 kind);
void EmPlesioth_MotionSounds(Em *em, EmWork *work);
void EmPlesioth_SetFinMode(Em *em, EmWork *work, s32 mode);
void EmPlesioth_SpawnLandingDust(Em *em);
/* end of prototypes */

/*
 * The motions' sounds: switches on work->prevMotion (the layer-0 motion id
 * EmPlesioth_MotionEffects keeps) and plays sounds at bones on given frames
 * (Em_PlaySeAtBoneOnFrame; some picked at random by Sound_RandomSe).
 */
void EmPlesioth_MotionSounds(Em *em, EmWork *work)
{
    s16 prevMotion;

    prevMotion = work->prevMotion;
    switch (prevMotion) {
    case EM_MOTION_ID(0, PLES_MOT_IDLE):
        Em_PlaySeAtBoneOnFrame(em, 0x40, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xC8, Sound_RandomSe(0x21, 2, 0x21, 2), 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x19A, Sound_RandomSe(0x21, 2, 0x21, 2), 0x23);
        break;
    case EM_MOTION_ID(0, PLES_MOT_WALK):
        Em_PlaySeAtBoneOnFrame(em, 0x34, 1, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x74, 1, 0x1A);
        break;
    case EM_MOTION_ID(0, PLES_MOT_RUN):
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x56, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 6, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 6, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 6, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 6, 0x1A);
        break;
    case EM_MOTION_ID(0, PLES_MOT_TURN_RIGHT):
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 1, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 1, 0x1A);
        break;
    case EM_MOTION_ID(0, PLES_MOT_TURN_LEFT):
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 1, 0x14);
        break;
    case EM_MOTION_ID(0, PLES_MOT_LIMP):
        Em_PlaySeAtBoneOnFrame(em, 6, 0x59, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 6, 4, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 0, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 0x12, 0x14);
        break;
    case EM_MOTION_ID(0, 0xB):
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0xB, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x42, 0xB, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x7A, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x7E, 0xB, 0xC);
        break;
    case EM_MOTION_ID(0, 0x12):
        Em_PlaySeAtBoneOnFrame(em, 0x28, 8, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0xD, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x52, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x56, 0xB, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x88, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x8C, 0xB, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0xB6, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0xBA, 0xB, 0xC);
        break;
    case EM_MOTION_ID(0, 0x13):
        Em_PlaySeAtBoneOnFrame(em, 2, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 6, 0xB, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 4, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 4, 1, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 9, 0);
        break;
    case EM_MOTION_ID(0, PLES_MOT_IDLE_ACTION):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x1F, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 0x22, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xAA, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xD6, 0x22, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x114, 0x1F, 0x23);
        break;
    case EM_MOTION_ID(0, PLES_MOT_REST):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x55, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xB6, 0x55, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x16C, 0x55, 0x23);
        break;
    case EM_MOTION_ID(0, PLES_MOT_REST_START):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x57, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x8E, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x17, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x13, 0);
        break;
    case EM_MOTION_ID(0, PLES_MOT_REST_END):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x1F, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0x55, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x7E, 3, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0xB6, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0x16, 0);
        break;
    case EM_MOTION_ID(0, PLES_MOT_STATUS_LIE):
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0x57, 0x23);
        break;
    case EM_MOTION_ID(0, PLES_MOT_LAND_ATTACK_A):
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0x24, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0x22, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 0, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0x15, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0xC, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0xC, 6);
        Em_PlaySeAtBoneOnFrame(em, 0xA6, 0, 0x1A);
        break;
    case EM_MOTION_ID(0, PLES_MOT_STATUS_GET_UP):
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0xC, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 2, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 1, 0x1A);
        break;
    case EM_MOTION_ID(0, PLES_MOT_STATUS_COLLAPSE):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x57, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 0xA4, 9, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x90, 3, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x88, 0xE, 6);
        Em_PlaySeAtBoneOnFrame(em, 0xB2, 4, 0x22);
        break;
    case EM_MOTION_ID(0, PLES_MOT_SPIN):
        Em_PlaySeAtBoneOnFrame(em, 8, 0x23, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0xF, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x14, 0x2A);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 1, 0x14);
        break;
    case EM_MOTION_ID(0, PLES_MOT_BREATH):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x12, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 0x18, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0x28, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 0x51, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 0xD, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0xB4, 0x13, 6);
        Em_PlaySeAtBoneOnFrame(em, 0xBB, 0x13, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0xF0, 0x12, 0);
        break;
    case EM_MOTION_ID(0, 0x30):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x22, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 0x22, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 0x22, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x72, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 1, 0x14);
        break;
    case EM_MOTION_ID(0, PLES_MOT_FLINCH):
        Em_PlaySeAtBoneOnFrame(em, 6, 0x53, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 0x13, 0x2A);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 6, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0x11, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x74, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x94, 3, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 0xC, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0xD, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0xC2, 3, 0x1A);
        break;
    case EM_MOTION_ID(0, PLES_MOT_FLINCH_BLIND):
        Em_PlaySeAtBoneOnFrame(em, 6, 0x53, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x12, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 6, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 4, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 3, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0xAC, 3, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x5A, 0x15, 0);
        break;
    case EM_MOTION_ID(0, PLES_MOT_SHAKE_OFF):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x4F, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x56, 0x1F, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 4, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 8, 0xD, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x62, 0x13, 0x2A);
        Em_PlaySeAtBoneOnFrame(em, 0x7E, 3, 0x1A);
        break;
    case EM_MOTION_ID(0, PLES_MOT_FLINCH_C):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x52, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 0x1F, 0x23);
        break;
    case EM_MOTION_ID(0, PLES_MOT_FLINCH_D):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x52, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x78, 3, 0x1A);
        break;
    case EM_MOTION_ID(0, PLES_MOT_FLINCH_B):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x52, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x13, 0);
        break;
    case EM_MOTION_ID(0, 0x43):
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0x55, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0x17, 0);
        break;
    case EM_MOTION_ID(0, PLES_MOT_DIE):
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x54, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 8, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x14, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x30, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x2E, 0x23);
        break;
    case EM_MOTION_ID(0, PLES_MOT_FALL_A):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x52, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x13, 0x2A);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 9, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0x11, 0);
        break;
    case EM_MOTION_ID(0, PLES_MOT_LIE_A):
        Em_PlaySeAtBoneOnFrame(em, 0x20, 0x1D, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 0x1C, 0);
        break;
    case EM_MOTION_ID(0, PLES_MOT_GET_UP_A):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0);
        break;
    case EM_MOTION_ID(0, PLES_MOT_FALL_B):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x52, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x13, 0x2A);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 9, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0x11, 0);
        break;
    case EM_MOTION_ID(0, PLES_MOT_LIE_DOWN):
        Em_PlaySeAtBoneOnFrame(em, 0x20, 0x1D, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 0x1C, 0);
        break;
    case EM_MOTION_ID(0, PLES_MOT_STAND_STILL):
        Em_PlaySeAtBoneOnFrame(em, 0x10, 0x5A, 0x23);
        break;
    case EM_MOTION_ID(0, PLES_MOT_TRAPPED):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x10, 0);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x12, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 9, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x27, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x52, 0x23);
        break;
    case EM_MOTION_ID(0, PLES_MOT_TRAP_STRUGGLE):
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0x12, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 0x12, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0xC, 6);
        Em_PlaySeAtBoneOnFrame(em, 8, 0xC, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x50, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0);
        break;
    case EM_MOTION_ID(0, PLES_MOT_TRAPPED_LIE):
        Em_PlaySeAtBoneOnFrame(em, 0x22, 0x16, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 0x16, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x62, 4, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 3, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x52, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x2B, 0x23);
        break;
    case EM_MOTION_ID(0, PLES_MOT_WAKE_IN_TRAP):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 6);
        Em_PlaySeAtBoneOnFrame(em, 8, 0x16, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 3, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x50, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0x20, 0x23);
        break;
    case EM_MOTION_ID(0, PLES_MOT_SWIM_START):
        Em_PlaySeAtBoneOnFrame(em, 4, 1, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0x2A);
        break;
    case EM_MOTION_ID(0, PLES_MOT_SWIM):
        break;
    case EM_MOTION_ID(0, PLES_MOT_SWIM_FAST):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x24, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 0x15, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0x15, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0x17, 0x2A);
        break;
    case EM_MOTION_ID(0, PLES_MOT_SWIM_RISE):
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0x56, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x15, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 0x30, 0x2A);
        break;
    case EM_MOTION_ID(0, PLES_MOT_SWIM_DIVE):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x15, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x30, 0x2A);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0x30, 0x2A);
        break;
    case EM_MOTION_ID(0, PLES_MOT_SWIM_TURN_LEFT):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0);
        break;
    case EM_MOTION_ID(0, PLES_MOT_SWIM_TURN_RIGHT):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0);
        break;
    case EM_MOTION_ID(0, PLES_MOT_LEAP_ATTACK):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x56, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0x30, 0x2A);
        break;
    case EM_MOTION_ID(0, PLES_MOT_DEAD_FLOAT):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x17, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x53, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x7A, 0x16, 0);
        break;
    case EM_MOTION_ID(0, PLES_MOT_SURFACE_BREATH_C):
        Em_PlaySeAtBoneOnFrame(em, 0x74, 0x1A, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x28, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 0x51, 0x23);
        break;
    case EM_MOTION_ID(0, 0x71):
        break;
    case EM_MOTION_ID(0, PLES_MOT_SWIM_CIRCLE_RIGHT):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0x2A);
        break;
    case EM_MOTION_ID(0, PLES_MOT_SWIM_CIRCLE_LEFT):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0x2A);
        break;
    case EM_MOTION_ID(0, PLES_MOT_SURFACE_BREATH_A):
        Em_PlaySeAtBoneOnFrame(em, 0x74, 0x18, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x28, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 0x51, 0x23);
        break;
    case EM_MOTION_ID(0, PLES_MOT_SURFACE_BREATH_B):
        Em_PlaySeAtBoneOnFrame(em, 0x74, 0x18, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x28, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 0x51, 0x23);
        break;
    case EM_MOTION_ID(0, 0x76):
        break;
    case EM_MOTION_ID(0, PLES_MOT_LEAP):
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0x27, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 8, 0xD, 0xC);
        break;
    case EM_MOTION_ID(0, PLES_MOT_LEAP_CROUCH):
        Em_PlaySeAtBoneOnFrame(em, 8, 0x33, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x12, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x15, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x14, 0);
        break;
    case EM_MOTION_ID(0, PLES_MOT_FLOP_A):
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0x10, 0);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 9, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 8, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0x12, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x20, 0x23);
        break;
    case EM_MOTION_ID(0, PLES_MOT_BEACHED):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x35, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x35, 0);
        break;
    case EM_MOTION_ID(0, PLES_MOT_GET_UP):
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0x12, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0x15, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x36, 0, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 4, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 1);
        break;
    case EM_MOTION_ID(0, PLES_MOT_FLOP_B):
        Em_PlaySeAtBoneOnFrame(em, 8, 0x33, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x12, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x15, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x14, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x36, 9, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0x1D, 0);
        break;
    case EM_MOTION_ID(0, PLES_MOT_TUMBLE):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x53, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x5A, 0x30, 0x2A);
        break;
    case EM_MOTION_ID(0, PLES_MOT_DIE_LYING):
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x54, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 8, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x14, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x2E, 0x23);
        break;
    case EM_MOTION_ID(0, PLES_MOT_LIE_FLAP):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x2C, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x10, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 9, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0x23);
        break;
    case EM_MOTION_ID(0, PLES_MOT_LIE_FLAP_B):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x52, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x10, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 9, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0x23);
        break;
    case EM_MOTION_ID(0, PLES_MOT_SWIM_STATUS_FLOAT):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x86, 0x17, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x86, 0x16, 0x23);
        break;
    case EM_MOTION_ID(0, PLES_MOT_SWIM_RECOVER):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x17, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x6E, 0x15, 0x23);
        break;
    case EM_MOTION_ID(0, PLES_MOT_HOOKED):
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0x17, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0x16, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0x16, 0x22);
        break;
    case EM_MOTION_ID(0, PLES_MOT_RUN_STOP):
        Em_PlaySeAtBoneOnFrame(em, 0xA, 6, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x1F, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x12, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x15, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x14, 0);
        break;
    case EM_MOTION_ID(0, PLES_MOT_SWIM_SURFACED):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x34, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x30, 0);
        break;
    case EM_MOTION_ID(0, PLES_MOT_SWIM_BOTTOMED):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x34, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x30, 0);
        break;
    case EM_MOTION_ID(0, PLES_MOT_LAND_ATTACK_B):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x12, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 3, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x6E, 4, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0xD8, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x108, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x15, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 9, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x1F, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x54, 0x16, 0);
        break;
    case EM_MOTION_ID(0, PLES_MOT_SIDE_LIE):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x57, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x122, 0x55, 0x23);
        break;
    case EM_MOTION_ID(0, PLES_MOT_STATUS4_LYING):
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x53, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x78, 0x52, 0x23);
        
        break;
    }
}

/*
 * The family setup (the Em_TypeSetup entry for both Plesioth kinds):
 * nothing to do.
 */
void EmPlesioth_Setup(Em *em)
{
}

/*
 * Callbacks slots 0 and 1: creates the alert mark (AlertMark_Create type
 * 0x22) and attached effect 0, as EmRathalos_SpawnEffects.
 */
void EmPlesioth_SpawnEffects(void *em)
{
    AlertMark_Create(em, 0x22, 0);
    Shadow_Spawn((Player *) em, 0);
}

/*
 * Sets a Plesioth up (Start, Reinit): idle motion row 1, scaled parts 3 and
 * 4, full hp from the size (Em_ComputeMaxHp), the swim depth 670 below the
 * surface (surfaceDepth), the work defaults and the four fins still; the
 * first time (fxFlags bit clear) hides mesh 1 and creates the stage object
 * of kind 9. Same role as EmRathalos_Init.
 */
void EmPlesioth_Init(Em *em)
{
    EmPlesiothWork *w = (EmPlesiothWork *)&em->work;

    s16 maxHp;
    u8 fxFlags;

    Em_SetMotionRow(em, PLES_MOT_IDLE, 0, 0);
    Em_SetPartScaled(em, 3);
    Em_SetPartScaled(em, 4);
    w->unk010 = 0;
    maxHp = Em_ComputeMaxHp(em, 0);
    em->hp = maxHp;
    em->hpMax = maxHp;
    em->scriptPending = 1;
    em->blind = 0;
    em->unk_InitFlag = 1;
    em->noAlert = 0;
    em->painted = 0;
    *(f32 *) &em->surfaceDepth = 670.0f;
    w->unk00A = 0x4000;
    w->unk028 = 0x100;
    w->unk02C = 0x200;
    w->unk030 = 0x100;
    w->unk018 = 0x2000;
    w->unk014 = 0;
    w->unk034 = 0;
    w->unk035 = 0;
    w->unk036 = 0;
    w->finMode = 0;
    w->finWait[0] = 0;
    w->finState[0][0] = 0;
    w->finState[0][1] = 0;
    w->finRot[0].fold = 0;
    w->finRot[0].wave = 0;
    w->finWait[1] = 0;
    w->finState[1][0] = 0;
    w->finState[1][1] = 0;
    w->finRot[1].fold = 0;
    w->finRot[1].wave = 0;
    w->finWait[2] = 0;
    w->finState[2][0] = 0;
    w->finState[2][1] = 0;
    w->finRot[2].fold = 0;
    w->finRot[2].wave = 0;
    w->finWait[3] = 0;
    w->finState[3][0] = 0;
    w->finState[3][1] = 0;
    w->finRot[3].fold = 0;
    w->finRot[3].wave = 0;
    fxFlags = em->fxFlags & 1;
    em->fxFlags = fxFlags;
    if (fxFlags == 0) {
        switch (em->type) {
        case EM_KIND_PLESIOTH:
        case EM_KIND_GREEN_PLESIOTH:
            em->meshShown[1] = 0;
            EmTail_Create(em);
            break;
        }
    }
}

/*
 * Starts a Plesioth (the per-kind start in gm_em): Init, in the quest-less
 * stage (gQuest.stage 0) a fixed spot 1500 apart per slot facing 0x8000;
 * then under water (EM_STANCE_UNDER) in PLES_SWIM_IDLE.
 */
void EmPlesioth_Start(Em *em)
{
    f32 fslot;
    u8 slotNo;

    EmPlesioth_Init(em);
    if (gQuest.stage == 0) {
        em->rotY = 0x8000;
        slotNo = em->slotNo;
        fslot = (f32) (u32) slotNo;
        em->posX = 5000.0f + (1500.0f * fslot);
        em->posY = 0.0f;
        em->posZ = 9000.0f;
        em->stance = EM_STANCE_UNDER;
        EmPlesioth_ChangeState(em, EM_GROUP_FLY, PLES_SWIM_IDLE, EM_SYNC_LOCAL);
        return;
    }
    em->stance = EM_STANCE_UNDER;
    EmPlesioth_ChangeState(em, EM_GROUP_FLY, PLES_SWIM_IDLE, EM_SYNC_LOCAL);
}

/* Callbacks slot 5: Init again without changing the state. */
void EmPlesioth_Reinit(Em *em)
{
    EmPlesioth_Init(em);
}

/*
 * Ends a state on land: speed 1, stance ground, no look-at, asks the script
 * for the next state (scriptPending) and goes to PLES_IDLE.
 */
void EmPlesioth_EndGroundState(Em *em)
{
    em->motionSpeed = 1.0f;
    em->stance = EM_STANCE_GROUND;
    em->lookAtTarget = 0;
    em->scriptPending = 1;
    EmPlesioth_ChangeState(em, EM_GROUP_IDLE, PLES_IDLE, EM_SYNC_LOCAL);
}

/*
 * Ends a state in the water: speed 1, stance under water, no look-at, asks
 * the script (scriptPending) and goes to PLES_SWIM_IDLE.
 */
void EmPlesioth_EndSwimState(Em *em)
{
    em->motionSpeed = 1.0f;
    em->stance = EM_STANCE_UNDER;
    em->lookAtTarget = 0;
    em->scriptPending = 1;
    EmPlesioth_ChangeState(em, EM_GROUP_FLY, PLES_SWIM_IDLE, EM_SYNC_LOCAL);
}

/*
 * Sets Em.stance and loads the matching sense row (Em_LoadReactionSet): row
 * 0 on land, row 2 under water (stance 4), row 1 whenever lying is set (the
 * status-lying and resting states pass 1).
 */
void EmPlesioth_SetStance(Em *em, u8 mode, u8 flag)
{
    em->stance = mode;
    if (flag == 0) {
        switch (mode) {
        case 0: case 1: default:
            Em_LoadReactionSet(em, 0);
            break;
        case 4:
            Em_LoadReactionSet(em, 2);
            break;
        }
    } else {
        Em_LoadReactionSet(em, 1);
    }
}

/*
 * Returns 1 when no other active, visible monster in the area of player p
 * has taken a bait (Em.hooked) or is being pulled out of the water (group 4
 * state 0xF, State4_0F); 0 otherwise. Used by PLES_SWIM_BAIT_WAIT before
 * letting the monster bite.
 */
s32 EmPlesioth_NoOtherHookedInArea(Em *em, Player *player)
{
    Em *other;
    s16 i;

    for (other = gEms, i = 0; i < 20; i++, other++) {
        if (other != em && other->active && other->visible && (Obj_SameArea(other, player) & 0xFF)) {
            if (other->hooked != 0 || (other->stateGroup == EM_GROUP_REACT && other->state == 0xF)) {
                return 0;
            }
        }
    }
    return 1;
}

/*
 * PLES_IDLE_STAND: puts the three layers on their idle loops
 * (EM_MOTION_ID(layer, 1)) and stands until the timer runs out, then asks
 * the script for the next state (scriptPending); never ends by itself.
 */
void EmPlesioth_IdleStand(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        if (em->motionId[0] != EM_MOTION_ID(0, PLES_MOT_IDLE)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(0, PLES_MOT_IDLE), 0xA, 0, 0);
        }
        if (em->motionId[1] != EM_MOTION_ID(0, PLES_MOT_SWIM_START)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(1, PLES_MOT_IDLE), 0xA, 0, 1);
        }
        if (em->motionId[2] != EM_MOTION_ID(1, PLES_MOT_IDLE)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(2, PLES_MOT_IDLE), 0xA, 0, 2);
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
 * PLES_IDLE: on land, the idle loops of the three layers once, then
 * EndGroundState (back to PLES_IDLE with the script asked); the state every
 * land action falls back to.
 */
void EmPlesioth_Idle(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmPlesioth_SetStance(em, 0, 0);
        em->lookAtTarget = 0;
        if (em->motionId[0] != EM_MOTION_ID(0, PLES_MOT_IDLE)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(0, PLES_MOT_IDLE), 0, 0, 0);
        }
        if (em->motionId[1] != EM_MOTION_ID(1, PLES_MOT_IDLE)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(1, PLES_MOT_IDLE), 0, 0, 1);
        }
        if (em->motionId[2] != EM_MOTION_ID(2, PLES_MOT_IDLE)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(2, PLES_MOT_IDLE), 0, 0, 2);
        }
        break;
    case 1:
        if ((em->remote == 0) && (em->motion[0].playing == 0)) {
            em->step++;
            EmPlesioth_EndGroundState(em);
        }
        break;
    }
}

/*
 * PLES_IDLE_ACTION: plays motion 0x19, then the idle motion once, then ends
 * on land.
 */
void EmPlesioth_IdleAction(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmPlesioth_SetStance(em, 0, 0);
        em->lookAtTarget = 0;
        Em_SetMotion(em, PLES_MOT_IDLE_ACTION, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, PLES_MOT_IDLE, 0, 0x5C);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmPlesioth_EndGroundState(em);
        }
        break;
    }
}

/*
 * PLES_IDLE_LAND_FLOP_A (variant 0, motion 0x79) and PLES_IDLE_LAND_FLOP_B
 * (variant 1, motion 0x7B): after a leap onto land, flops down, lies
 * beached on its side (EM_STANCE_FAMILY, motion 0x7A), gets back up (motion
 * 0x7C, stance ground) and ends on land.
 */
void EmPlesioth_LandFlop(Em *em, EmWork *work, u8 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmPlesioth_SetStance(em, 0, 0);
        em->lookAtTarget = 0;
        switch (variant) {
        default:
            Em_SetMotion(em, PLES_MOT_FLOP_A, 0, 0);
            break;
        case 1:
            Em_SetMotion(em, PLES_MOT_FLOP_B, 0, 0);
            break;
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stance = EM_STANCE_FAMILY;
            Em_SetMotion(em, PLES_MOT_BEACHED, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, PLES_MOT_GET_UP, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmPlesioth_EndGroundState(em);
        }
        break;
    }
}

/*
 * PLES_IDLE_STATUS2_COLLAPSE (variant 0) and PLES_IDLE_STATUS1_COLLAPSE_B
 * (variant 1): lying, unseeing and woken by a hit (EM_HIT_REACT_WAKE),
 * collapses (0x26) and lies (0x22) until the status timer ends, then
 * PLES_IDLE_STATUS2_END or PLES_IDLE_STATUS1_END_B. The second parameter is
 * the unused work pointer.
 */
void EmPlesioth_StatusCollapse(Em *em, s32 work, s32 variant)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        EmPlesioth_SetStance(em, 0, 1);
        em->lookAtTarget = 0;
        Em_SetMotion(em, PLES_MOT_STATUS_COLLAPSE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, PLES_MOT_STATUS_LIE, 0, 0);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            if (variant == 0) {
                EmPlesioth_ChangeState(em, EM_GROUP_IDLE, PLES_IDLE_STATUS2_END, EM_SYNC_OWNER_FLAGGED);
                break;
            }
            EmPlesioth_ChangeState(em, EM_GROUP_IDLE, PLES_IDLE_STATUS1_END_B, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 3:
        if (em->remote == 0) {
            if (variant == 0) {
                EmPlesioth_ChangeState(em, EM_GROUP_IDLE, PLES_IDLE_STATUS2_END, EM_SYNC_OWNER_FLAGGED);
                break;
            }
            EmPlesioth_ChangeState(em, EM_GROUP_IDLE, PLES_IDLE_STATUS1_END_B, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * PLES_IDLE_STATUS2_END, PLES_IDLE_STATUS1_END, PLES_IDLE_STATUS1_END_B:
 * gets up from lying under a status (motion 0x25, blind meanwhile), then
 * PLES_IDLE_SHAKE_OFF.
 */
void EmPlesioth_StatusGetUp(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmPlesioth_SetStance(em, 0, 0);
        Em_SetMotion(em, PLES_MOT_STATUS_GET_UP, 0, 0);
        em->blind = 2;
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            EmPlesioth_ChangeState(em, EM_GROUP_IDLE, PLES_IDLE_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmPlesioth_ChangeState(em, EM_GROUP_IDLE, PLES_IDLE_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * PLES_IDLE_SHAKE_OFF, PLES_IDLE_STATUS4_SHAKE_OFF,
 * PLES_IDLE_HOLD2_SHAKE_OFF, PLES_IDLE_STUN_SHAKE_OFF: plays motion 0x3E
 * once, then ends on land.
 */
void EmPlesioth_ShakeOff(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmPlesioth_SetStance(em, 0, 0);
        Em_SetMotion(em, PLES_MOT_SHAKE_OFF, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmPlesioth_EndGroundState(em);
        }
        break;
    }
}

/*
 * PLES_IDLE_STATUS2_TRAPPED: under status 2 while held (holdTimer kept at
 * 5) after the pitfall trap hit it directly: lies in motion 0x61 with lying
 * puffs and a sound every 135 frames until the timer ends, then react state
 * 0x13 (which ends status 2).
 */
void EmPlesioth_Status2Trapped(Em *em, EmWork *work)
{
    f32 pos[3];

    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->holdTimer = 5;
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        EmPlesioth_SetStance(em, 0, 1);
        em->lookAtTarget = 0;
        Em_SetMotion(em, PLES_MOT_TRAPPED_LIE, 0, 0);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmPlesioth_ChangeState(em, EM_GROUP_REACT, PLES_REACT_STATUS2_TRAPPED_END, EM_SYNC_OWNER);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmPlesioth_ChangeState(em, EM_GROUP_REACT, PLES_REACT_STATUS2_TRAPPED_END, EM_SYNC_OWNER);
        }
        break;
    }
    pos[0] = 0.0f;
    pos[1] = 10.0f;
    pos[2] = 140.0f;
    Em_SpawnLyingPuffs(em, 0x22, pos, 1.6f);
    if (em->timer % 135 == 0) {
        Em_PlaySeAtBone(em, 0x57, 0x23);
    }
}

/*
 * PLES_IDLE_STATUS1_TRAPPED: as PLES_IDLE_STATUS2_TRAPPED under status 1,
 * ending in react state 0x14 (which ends status 1).
 */
void EmPlesioth_Status1Trapped(Em *em, EmWork *work)
{
    f32 pos[3];

    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->holdTimer = 5;
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        EmPlesioth_SetStance(em, 0, 1);
        em->lookAtTarget = 0;
        Em_SetMotion(em, PLES_MOT_TRAPPED_LIE, 0, 0);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmPlesioth_ChangeState(em, EM_GROUP_REACT, PLES_REACT_STATUS1_TRAPPED_END, EM_SYNC_OWNER);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmPlesioth_ChangeState(em, EM_GROUP_REACT, PLES_REACT_STATUS1_TRAPPED_END, EM_SYNC_OWNER);
        }
        break;
    }
    pos[0] = 0.0f;
    pos[1] = 10.0f;
    pos[2] = 140.0f;
    Em_SpawnLyingPuffs(em, 0x22, pos, 1.6f);
    if (em->timer % 135 == 0) {
        Em_PlaySeAtBone(em, 0x57, 0x23);
    }
}

/*
 * PLES_IDLE_STATUS1_COLLAPSE: status 1 standing on land (EM_DMG_CAPTURED,
 * not held): collapses (0x26), lies (0x22) until the timer ends, then
 * PLES_IDLE_STATUS1_END.
 */
void EmPlesioth_Status1Collapse(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        EmPlesioth_SetStance(em, 0, 1);
        em->lookAtTarget = 0;
        Em_SetMotion(em, PLES_MOT_STATUS_COLLAPSE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, PLES_MOT_STATUS_LIE, 0, 0);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            EmPlesioth_ChangeState(em, EM_GROUP_IDLE, PLES_IDLE_STATUS1_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmPlesioth_ChangeState(em, EM_GROUP_IDLE, PLES_IDLE_STATUS1_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * PLES_IDLE_STATUS2_COLLAPSE_BEACHED: status 2 while beached (stance
 * EM_STANCE_FAMILY): collapses onto its side (0x4B, then 0x80) and lies
 * (0x89) until the timer ends, then PLES_IDLE_STATUS2_END_BEACHED.
 */
void EmPlesioth_Status2CollapseBeached(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        EmPlesioth_SetStance(em, 1, 1);
        em->lookAtTarget = 0;
        Em_SetMotion(em, PLES_MOT_LIE_DOWN, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, PLES_MOT_LIE_FLAP, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, PLES_MOT_SIDE_LIE, 0, 0);
        }
        break;
    case 3:
        if (--em->timer <= 0) {
            em->step++;
            EmPlesioth_ChangeState(em, EM_GROUP_IDLE, PLES_IDLE_STATUS2_END_BEACHED, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 4:
        if (em->remote == 0) {
            EmPlesioth_ChangeState(em, EM_GROUP_IDLE, PLES_IDLE_STATUS2_END_BEACHED, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * PLES_IDLE_STATUS2_END_BEACHED, PLES_IDLE_STATUS1_END_BEACHED: gets up
 * from its side (motion 0x7C, stance ground, blind meanwhile), then
 * PLES_IDLE_SHAKE_OFF.
 */
void EmPlesioth_StatusGetUpBeached(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmPlesioth_SetStance(em, 0, 0);
        Em_SetMotion(em, PLES_MOT_GET_UP, 0, 0);
        em->blind = 2;
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            EmPlesioth_ChangeState(em, EM_GROUP_IDLE, PLES_IDLE_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmPlesioth_ChangeState(em, EM_GROUP_IDLE, PLES_IDLE_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * PLES_IDLE_STATUS1_COLLAPSE_BEACHED: as Status2CollapseBeached under
 * status 1, then PLES_IDLE_STATUS1_END_BEACHED.
 */
void EmPlesioth_Status1CollapseBeached(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        EmPlesioth_SetStance(em, 0, 1);
        em->lookAtTarget = 0;
        Em_SetMotion(em, PLES_MOT_LIE_DOWN, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, PLES_MOT_LIE_FLAP, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, PLES_MOT_SIDE_LIE, 0, 0);
        }
        break;
    case 3:
        if (--em->timer <= 0) {
            em->step++;
            EmPlesioth_ChangeState(em, EM_GROUP_IDLE, PLES_IDLE_STATUS1_END_BEACHED, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 4:
        if (em->remote == 0) {
            EmPlesioth_ChangeState(em, EM_GROUP_IDLE, PLES_IDLE_STATUS1_END_BEACHED, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * PLES_IDLE_REST: lies down (0x20) and rests (0x1F) for 1800 frames,
 * regaining 1 hp a frame; at the end full hp and the low-hp need cleared
 * (Em_ClearLowHpNeed); then PLES_IDLE_REST_END (a hit wakes it there
 * early).
 */
void EmPlesioth_Rest(Em *em)
{
    s16 hpMax;
    s16 hp;

    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        EmPlesioth_SetStance(em, 0, 1);
        em->lookAtTarget = 0;
        Em_SetMotion(em, PLES_MOT_REST_START, 0, 0);
        em->timer = 0x708;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, PLES_MOT_REST, 0, 0);
        }
        break;
    case 2:
        hpMax = em->hpMax;
        hp = em->hp;
        if (hp < hpMax) {
            em->hp = hp + 1;
        } else {
            em->hp = hpMax;
        }
        if (--em->timer <= 0) {
            em->step++;
            em->hp = em->hpMax;
            Em_ClearLowHpNeed(em);
            EmPlesioth_ChangeState(em, EM_GROUP_IDLE, PLES_IDLE_REST_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmPlesioth_ChangeState(em, EM_GROUP_IDLE, PLES_IDLE_REST_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * PLES_IDLE_REST_END: gets up from the rest (motion 0x21, blind meanwhile),
 * then PLES_IDLE_SHAKE_OFF.
 */
void EmPlesioth_RestEnd(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmPlesioth_SetStance(em, 0, 0);
        Em_SetMotion(em, PLES_MOT_REST_END, 0, 0);
        em->blind = 2;
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            EmPlesioth_ChangeState(em, EM_GROUP_IDLE, PLES_IDLE_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmPlesioth_ChangeState(em, EM_GROUP_IDLE, PLES_IDLE_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * PLES_MOVE_WALK: walks (motion 3) to the goal (Em_StepToGoal), or for the
 * state timer when there is none, then ends on land.
 */
void EmPlesioth_Walk(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        EmPlesioth_SetStance(em, 0, 0);
        Em_SetMotion(em, PLES_MOT_WALK, 0, 0);
        break;
    case 1:
        if (em->hasGoal != 0) {
            if (Em_StepToGoal(em, 0x40, 1) == 1) {
                em->step++;
                EmPlesioth_EndGroundState(em);
                break;
            }
        } else {
            if (--em->timer <= 0) {
                em->step++;
                EmPlesioth_EndGroundState(em);
            }
        }
        break;
    }
}

/*
 * PLES_MOVE_RUN: runs (motion 4) to 300 short of the goal (or for the
 * timer), plays the stop (0x85) and ends on land.
 */
void EmPlesioth_RunAndStop(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        EmPlesioth_SetStance(em, 0, 0);
        Em_SetMotion(em, PLES_MOT_RUN, 0, 0);
        break;
    case 1:
        if (em->hasGoal != 0) {
            if (Em_StepToGoal(em, 0x40, 1) == 1) {
                em->step++;
                Em_SetMotion(em, PLES_MOT_RUN_STOP, 0, 0);
                break;
            }
        } else {
            if (--em->timer <= 0) {
                em->step++;
                Em_SetMotion(em, PLES_MOT_RUN_STOP, 0, 0);
            }
            break;
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmPlesioth_EndGroundState(em);
        }
        break;
    }
}

/*
 * PLES_MOVE_TURN: turns on land toward the target with the land turn table
 * (EmPlesioth_TurnParams[0]: walk, turn left and right), then clears
 * EM_FLAG_NO_ROOT_MOTION and ends on land.
 */
void EmPlesioth_TurnToTarget(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        EmPlesioth_SetStance(em, 0, 0);
        Em_StartTurnToTarget(em, &EmPlesioth_TurnParams[0]);
        break;
    case 1:
        if (Em_TurnStep(em, &EmPlesioth_TurnParams[0]) == 1) {
            em->step++;
            Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
            EmPlesioth_EndGroundState(em);
        }
        break;
    }
}

/*
 * PLES_MOVE_LIMP: limps (motion 0xA) to the goal or for the timer, then
 * ends on land; PLES_MOVE_WALK becomes this below a fifth of max hp.
 */
void EmPlesioth_Limp(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        EmPlesioth_SetStance(em, 0, 0);
        Em_SetMotion(em, PLES_MOT_LIMP, 0, 0);
        break;
    case 1:
        if (em->hasGoal != 0) {
            if (Em_StepToGoal(em, 0x40, 1) == 1) {
                em->step++;
                EmPlesioth_EndGroundState(em);
                break;
            }
        } else {
            if (--em->timer <= 0) {
                em->step++;
                EmPlesioth_EndGroundState(em);
            }
        }
        break;
    }
}

/*
 * PLES_SWIM_IDLE, PLES_SWIM_IDLE_B: hovers under water (motion 0x64, stance
 * under water) once, then EndSwimState (back here with the script asked);
 * the start state and the state every swim action falls back to.
 */
void EmPlesioth_SwimIdle(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, PLES_MOT_SWIM_IDLE, 0, 0);
        EmPlesioth_SetStance(em, 4, 0);
        break;
    case 1:
        if ((em->remote == 0) && (em->motion[0].playing == 0)) {
            em->step++;
            EmPlesioth_EndSwimState(em);
        }
        break;
    }
}

/*
 * PLES_SWIM_IDLE_AREA: as SwimIdle, and applies the area values on entry
 * (Em_ApplyAreaValues).
 */
void EmPlesioth_SwimIdleApplyArea(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, PLES_MOT_SWIM_IDLE, 0, 0);
        EmPlesioth_SetStance(em, 4, 0);
        Em_ApplyAreaValues(em);
        break;
    case 1:
        if ((em->remote == 0) && (em->motion[0].playing == 0)) {
            em->step++;
            EmPlesioth_EndSwimState(em);
        }
        break;
    }
}

/* PLES_SWIM_NONE: does nothing (empty state). */
void EmPlesioth_SwimNone(void)
{
}

/*
 * PLES_SWIM_TURN: turns under water toward the target
 * (EmPlesioth_TurnParams[1]: swim, swim turn left and right), then ends in
 * the water.
 */
void EmPlesioth_SwimTurn(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        EmPlesioth_SetStance(em, 4, 0);
        Em_StartTurnToTarget(em, &EmPlesioth_TurnParams[1]);
        break;
    case 1:
        if (Em_TurnStep(em, &EmPlesioth_TurnParams[1]) == 1) {
            em->step++;
            EmPlesioth_EndSwimState(em);
        }
        break;
    }
}

/*
 * PLES_SWIM_FORWARD: swims toward the goal pitched up or down to it (0x65
 * accelerating to 50, then 0x66), ending at the goal or once goalDist is
 * used up.
 */
void EmPlesioth_SwimForward(Em *em, EmWork *work)
{

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        EmPlesioth_SetStance(em, 4, 0);
        Em_SetMotion(em, PLES_MOT_SWIM_START, 0, 0);
        Player_ClearVelocity(em);
        em->velZ = 5.0f;
        em->accelZ = 1.0f;
        break;
    case 1:
        Em_GetPitchToGoal(em, (s32 *) &work->moveAngBX, 0.0f, 0.0f);
        work->moveAngBY = em->rotY;
        work->moveAngBZ = 0;
        Em_MoveRotated(em, &work->moveAngBX);
        if (!(em->velZ < 50.0f)) {
            em->velZ = 50.0f;
        }
        if (Em_StepToGoal(em, 0x40, 2) == 1) {
            EmPlesioth_EndSwimState(em);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, PLES_MOT_SWIM, 0, 0);
        }
        break;
    case 2:
        Em_GetPitchToGoal(em, (s32 *) &work->moveAngBX, 0.0f, 0.0f);
        work->moveAngBY = em->rotY;
        work->moveAngBZ = 0;
        Em_MoveRotated(em, &work->moveAngBX);
        if (!(em->velZ < 50.0f)) {
            em->velZ = 50.0f;
        }
        em->goalDist -= em->velZ;
        if (em->goalDist <= 0.0f) {
            EmPlesioth_EndSwimState(em);
        }
        break;
    }
}

/*
 * PLES_SWIM_DASH: fast swim (0x67) toward the goal from motion frame 30 at
 * up to 100, with the surface wake effects 2 and 4, ending at the goal.
 */
void EmPlesioth_SwimDash(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        EmPlesioth_SetStance(em, 4, 0);
        Em_SetMotion(em, PLES_MOT_SWIM_FAST, 0, 0);
        Player_ClearVelocity(em);
        em->velZ = 30.0f;
        em->accelZ = 10.0f;
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 30.0f) != 0) {
            Em_GetPitchToGoal(em, (s32 *) &work->moveAngBX, 0.0f, 0.0f);
            work->moveAngBY = em->rotY;
            work->moveAngBZ = 0;
            Em_MoveRotated(em, &work->moveAngBX);
            if (!(em->velZ < 100.0f)) {
                em->velZ = 100.0f;
            }
            if (Em_StepToGoal(em, 0x40, 2) == 1) {
                em->step++;
                EmPlesioth_EndSwimState(em);
            }
            EmPlesioth_SpawnWakeEvery4(em, 8.0f);
            EmPlesioth_SpawnWakeEvery8(em, 8.0f);
        }
        break;
    }
}

/*
 * PLES_SWIM_CURVE: plays motion 0x6E moved by curve set 0 (StartCurves,
 * UpdateCurves) and ends in the water; SetGroup2State turns this state into
 * PLES_SWIM_DIVE, so it never runs through ChangeState.
 */
void EmPlesioth_SwimCurve(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        EmPlesioth_SetStance(em, 4, 0);
        Em_SetMotion(em, PLES_MOT_SWIM_CURVE, 0, 0);
        EmPlesioth_StartCurves(em, 0U);
        break;
    case 1:
        if (EmPlesioth_UpdateCurves(em)) {
            em->step++;
            EmPlesioth_EndSwimState(em);
        }
        break;
    }
}

/*
 * PLES_SWIM_RISE: rises (0x68) straight up to the water surface, splashes
 * (surface effect 5), plays the surfacing end (0x86) and ends in the water.
 */
void EmPlesioth_SwimRise(Em *em, EmWork *work)
{
    f32 surfaceY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        EmPlesioth_SetStance(em, 4, 0);
        Em_SetMotion(em, PLES_MOT_SWIM_RISE, 0, 0);
        Player_ClearVelocity(em);
        em->velZ = 20.0f;
        em->velY = 2.0f;
        em->accelY = 4.0f;
        work->moveAngBX = 0;
        work->moveAngBZ = 0;
        break;
    case 1:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, &work->moveAngBX);
        if (!(em->velY <= 50.0f)) {
            em->velY = 50.0f;
            em->accelY = 0.0f;
        }
        surfaceY = em->surfaceY;
        if (!(em->posY <= surfaceY)) {
            em->posY = surfaceY;
            em->step++;
            Em_SetMotion(em, PLES_MOT_SWIM_SURFACED, 0, 0);
            EmPlesioth_SpawnSurfaceSplash(em, 8.0f);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmPlesioth_EndSwimState(em);
        }
        break;
    }
}

/*
 * PLES_SWIM_DIVE: sinks (0x69) down to the floor, plays the bottom end
 * (0x87) and ends in the water.
 */
void EmPlesioth_SwimDive(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        EmPlesioth_SetStance(em, 4, 0);
        Em_SetMotion(em, PLES_MOT_SWIM_DIVE, 0, 0);
        Player_ClearVelocity(em);
        em->velZ = 20.0f;
        em->velY = -2.0f;
        em->accelY = -4.0f;
        work->moveAngBX = 0;
        work->moveAngBZ = 0;
        break;
    case 1:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, &work->moveAngBX);
        if (em->velY < -50.0f) {
            em->velY = -50.0f;
            em->accelY = 0.0f;
        }
        floorY = em->floorY;
        if (em->posY < floorY) {
            em->posY = floorY;
            em->step++;
            Em_SetMotion(em, PLES_MOT_SWIM_BOTTOMED, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmPlesioth_EndSwimState(em);
        }
        break;
    }
}

/*
 * PLES_SWIM_ROUND_TURN: turns toward the next area point
 * (EmPlesioth_TurnParams[2]), then PLES_SWIM_ROUND_NEXT; SetGroup2State
 * picks it for a round whose point is not ahead.
 */
void EmPlesioth_SwimRoundTurn(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        EmPlesioth_SetStance(em, 4, 0);
        Em_StartTurnToTarget(em, &EmPlesioth_TurnParams[2]);
        break;
    case 1:
        if (Em_TurnStep(em, &EmPlesioth_TurnParams[2]) == 1) {
            em->step++;
            EmPlesioth_ChangeState(em, EM_GROUP_FLY, PLES_SWIM_ROUND_NEXT, EM_SYNC_OWNER);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmPlesioth_ChangeState(em, EM_GROUP_FLY, PLES_SWIM_ROUND_NEXT, EM_SYNC_OWNER);
        }
        break;
    }
}

/*
 * PLES_SWIM_ROUND_NEXT: fast swim (0x67) around the four area points in
 * rising order (curGoalNo + 1), each leg ended within 300 or by a timer,
 * with surface wakes; ends in the water after roundLegsLeft legs (4).
 */
void EmPlesioth_SwimRoundNext(Em *em, EmWork *work)
{
    u8 legsLeft;

    switch (em->step) {
    case 0:
        EmPlesioth_SetStance(em, 4, 0);
        Em_SetMotion(em, PLES_MOT_SWIM_FAST, 0, 0);
        em->step++;
        em->stateVar1 = 0;
        em->lookAtTarget = 0;
        em->velX = 0.0f;
        em->velY = 0.0f;
        Player_ClearAccel2(em);
        if (em->velZ <= 50.0f) {
            em->velZ = 50.0f;
        }
        break;
    case 1:
        if (Vec_DistanceXZ(&em->posX, em->goal) <= 300.0f || --em->timer <= 0) {
            legsLeft = ((EmPlesiothWork *)work)->roundLegsLeft - 1;
            ((EmPlesiothWork *)work)->roundLegsLeft = legsLeft;
            if ((legsLeft & 0xFF) <= 0) {
                em->step++;
                EmPlesioth_EndSwimState(em);
            } else {
                em->curGoalNo++;
                em->curGoalNo &= 3;
                Em_ComputeGoal2(em, em->goal);
                em->timer = (s32) ((1.5f * Vec_DistanceXZ(&em->posX, em->goal)) / 50.0f);
                Em_SetStateRand(em, EM_GROUP_FLY, PLES_SWIM_ROUND_NEXT);
            }
        }
        Em_TurnToGoal(em, 0x200, 0);
        Em_GetPitchToGoal(em, (s32 *) &work->moveAngBX, 0.0f, 0.0f);
        work->moveAngBY = em->rotY;
        work->moveAngBZ = 0;
        Em_MoveRotated(em, (s32 *) &work->moveAngBX);
        EmPlesioth_SpawnWakeEvery4(em, 8.0f);
        EmPlesioth_SpawnWakeEvery8(em, 8.0f);
        break;
    }
}

/* PLES_SWIM_ROUND_PREV: as SwimRoundNext in falling order (curGoalNo - 1). */
void EmPlesioth_SwimRoundPrev(Em *em, EmWork *work)
{
    u8 legsLeft;

    switch (em->step) {
    case 0:
        EmPlesioth_SetStance(em, 4, 0);
        Em_SetMotion(em, PLES_MOT_SWIM_FAST, 0, 0);
        em->step++;
        em->stateVar1 = 0;
        em->lookAtTarget = 0;
        em->velX = 0.0f;
        em->velY = 0.0f;
        Player_ClearAccel2(em);
        if (em->velZ <= 50.0f) {
            em->velZ = 50.0f;
        }
        break;
    case 1:
        if (Vec_DistanceXZ(&em->posX, em->goal) <= 300.0f || --em->timer <= 0) {
            legsLeft = ((EmPlesiothWork *)work)->roundLegsLeft - 1;
            ((EmPlesiothWork *)work)->roundLegsLeft = legsLeft;
            if ((legsLeft & 0xFF) <= 0) {
                em->step++;
                EmPlesioth_EndSwimState(em);
            } else {
                em->curGoalNo -= 1;
                em->curGoalNo &= 3;
                Em_ComputeGoal2(em, em->goal);
                em->timer = (s32) ((1.5f * Vec_DistanceXZ(&em->posX, em->goal)) / 50.0f);
                Em_SetStateRand(em, EM_GROUP_FLY, PLES_SWIM_ROUND_PREV);
            }
        }
        Em_TurnToGoal(em, 0x200, 0);
        Em_GetPitchToGoal(em, (s32 *) &work->moveAngBX, 0.0f, 0.0f);
        work->moveAngBY = em->rotY;
        work->moveAngBZ = 0;
        Em_MoveRotated(em, (s32 *) &work->moveAngBX);
        EmPlesioth_SpawnWakeEvery4(em, 8.0f);
        EmPlesioth_SpawnWakeEvery8(em, 8.0f);
        break;
    }
}

/*
 * PLES_SWIM_CIRCLE_RIGHT: swims a circle for 132 frames at speed 40 (motion
 * 0x72), the heading rising 0x1F0 a frame (the right side in the turn
 * tables), then ends in the water.
 */
void EmPlesioth_SwimCircleRight(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        EmPlesioth_SetStance(em, 4, 0);
        Em_SetMotion(em, PLES_MOT_SWIM_CIRCLE_RIGHT, 0, 0);
        em->timer = 0x84;
        Player_ClearVelocity(em);
        em->velZ = 40.0f;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmPlesioth_EndSwimState(em);
        }
        em->rotY += 0x1F0;
        em->rotY = (s32) (u16) em->rotY;
        Em_GetPitchToGoal(em, (s32 *) &work->moveAngBX, 0.0f, 0.0f);
        work->moveAngBY = em->rotY;
        work->moveAngBZ = 0;
        Em_MoveRotated(em, &work->moveAngBX);
        break;
    }
}

/*
 * PLES_SWIM_CIRCLE_LEFT: as SwimCircleRight with motion 0x73 and the
 * heading falling (left).
 */
void EmPlesioth_SwimCircleLeft(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        EmPlesioth_SetStance(em, 4, 0);
        Em_SetMotion(em, PLES_MOT_SWIM_CIRCLE_LEFT, 0, 0);
        em->timer = 0x84;
        Player_ClearVelocity(em);
        em->velZ = 40.0f;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmPlesioth_EndSwimState(em);
        }
        em->rotY -= 0x1F0;
        em->rotY = (s32) (u16) em->rotY;
        Em_GetPitchToGoal(em, (s32 *) &work->moveAngBX, 0.0f, 0.0f);
        work->moveAngBY = em->rotY;
        work->moveAngBZ = 0;
        Em_MoveRotated(em, &work->moveAngBX);
        break;
    }
}

/*
 * PLES_SWIM_LEAP: leaps out of the water (0x77; airborne, splash on
 * leaving), lower in Jungle area 4; falling back over deep water it
 * re-enters with a splash and glides 1000 forward (0x67), over land it
 * lands on the floor and goes to PLES_IDLE_LAND_FLOP_A.
 */
void EmPlesioth_LeapFromWater(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0x0:
        em->step++;
        em->lookAtTarget = 0;
        EmPlesioth_SetStance(em, 4, 0);
        Player_ClearVelocity(em);
        Em_SetMotion(em, PLES_MOT_LEAP, 0, 0);
        em->velY = 130.0f;
        if ((Area_DayId(em->area) & 0xFFFF) == AREA_JUNGLE_4) {
            em->velY *= 0.8f;
        }
        em->velZ = 70.0f;
        em->accelY = -4.0f;
        work->moveAngBX = 0;
        work->moveAngBZ = 0;
        break;
    case 0x1:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, (s32 *) &work->moveAngBX);
        if (!(em->posY < em->surfaceY)) {
            if (em->stance == EM_STANCE_UNDER) {
                EmPlesioth_SpawnSurfaceSplash(em, 8.0f);
            }
            em->stance = EM_STANCE_AIR;
        }
        if (em->velY < 0.0f) {
            if (EmPlesioth_OverDeepWater(em, 1U) == 1) {
                if (em->posY < em->surfaceY) {
                    em->step++;
                    em->posY = em->surfaceY;
                    em->stance = EM_STANCE_UNDER;
                    em->velY = 0.0f;
                    em->accelY = 0.0f;
                    Em_SetMotion(em, PLES_MOT_SWIM_FAST, 0xA, 0);
                    em->goalDist = 1000.0f;
                    em->timer = 0x3C;
                    em->accelZ = (em->goalDist - em->timer * em->velZ) / ((em->timer * em->timer) / 2);
                    EmPlesioth_SpawnSurfaceSplash(em, 8.0f);
                    break;
                }
            } else if (em->posY < em->floorY) {
                em->step = 0x63;
                em->posY = em->floorY;
                EmPlesioth_ChangeState(em, EM_GROUP_IDLE, PLES_IDLE_LAND_FLOP_A, EM_SYNC_OWNER_FLAGGED);
                break;
            }
        }
        break;
    case 0x2:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, (s32 *) &work->moveAngBX);
        em->goalDist -= em->velZ;
        if ((em->goalDist <= 0.0f) || (em->velZ <= 0.0f)) {
            EmPlesioth_EndSwimState(em);
        }
        break;
    case 0x63:
        if (em->remote == 0) {
            em->posY = em->floorY;
            EmPlesioth_ChangeState(em, EM_GROUP_IDLE, PLES_IDLE_LAND_FLOP_A, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * PLES_SWIM_SURFACE_FLOP: at the surface, plays the flop (0x79) with a
 * splash and ends in the water from motion frame 10.
 */
void EmPlesioth_SurfaceFlop(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        EmPlesioth_SetStance(em, 4, 0);
        Em_SetMotion(em, PLES_MOT_FLOP_A, 0, 0);
        Player_ClearVelocity(em);
        em->posY = em->surfaceY;
        EmPlesioth_SpawnSurfaceSplash(em, 8.0f);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 10.0f, 0U) != 0) {
            em->step++;
            EmPlesioth_EndSwimState(em);
        }
        break;
    }
}

/*
 * PLES_SWIM_LEAP_IN (variant 0) and PLES_SWIM_LEAP_IN_TO_GOAL (variant 1):
 * from land, crouches (0x78) and leaps (0x77): variant 0 a fixed leap and a
 * 1000 glide after entering, variant 1 faces the goal and lands on it in 60
 * frames; over land it lands in PLES_IDLE_LAND_FLOP_A.
 */
void EmPlesioth_LeapIntoWater(Em *em, EmWork *work, u8 variant)
{
    switch (em->step) {
    case 0x0:
        em->step++;
        em->lookAtTarget = 0;
        EmPlesioth_SetStance(em, 0, 0);
        Em_SetMotion(em, PLES_MOT_LEAP_CROUCH, 0, 0);
        Player_ClearVelocity(em);
        switch (variant) {
        case 1:
            em->rotY = Math_AngleXZ(&em->posX, em->goal);
            break;
        }
        break;
    case 0x1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stance = EM_STANCE_AIR;
            Em_SetMotion(em, PLES_MOT_LEAP, 0, 0);
            em->accelY = -4.0f;
            switch (variant) {
            default:
                em->velY = 50.0f;
                em->velZ = 100.0f;
                break;
            case 1:
                em->velY = 60.0f * (-0.5f * em->accelY);
                em->velZ = Vec_DistanceXZ(em->goal, &em->posX) / 60.0f;
                break;
            }
        }
        break;
    case 0x2:
        work->moveAngBX = 0;
        work->moveAngBY = em->rotY;
        work->moveAngBZ = 0;
        Em_MoveRotatedAccel(em, (s32 *) &work->moveAngBX);
        if (em->posY <= em->surfaceY) {
            em->stance = EM_STANCE_UNDER;
        }
        if (em->velY < 0.0f) {
            if ((variant != 0 && EmPlesioth_OverDeepWater(em, 0) == 1) || (variant == 0 && EmPlesioth_OverDeepWater(em, 1) == 1)) {
                if (em->posY < em->surfaceY) {
                    em->step++;
                    em->posY = em->surfaceY;
                    em->stance = EM_STANCE_UNDER;
                    em->velY = 0.0f;
                    em->accelY = 0.0f;
                    Em_SetMotion(em, PLES_MOT_SWIM_FAST, 0xA, 0);
                    if (variant != 1) {
                        em->goalDist = 1000.0f;
                    } else {
                        em->goalDist = 0.0f;
                    }
                    em->timer = 0x3C;
                    em->accelZ = (em->goalDist - em->timer * em->velZ) / ((em->timer * em->timer) / 2);
                    EmPlesioth_SpawnSurfaceSplash(em, 8.0f);
                }
            } else if (em->posY < em->floorY) {
                em->step = 0x63;
                em->posY = em->floorY;
                EmPlesioth_ChangeState(em, EM_GROUP_IDLE, PLES_IDLE_LAND_FLOP_A, EM_SYNC_OWNER_FLAGGED);
            }
        }
        break;
    case 0x3:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, (s32 *) &work->moveAngBX);
        em->goalDist -= em->velZ;
        if ((em->goalDist <= 0.0f) || (em->velZ <= 0.0f)) {
            EmPlesioth_EndSwimState(em);
        }
        break;
    case 0x63:
        if (em->remote == 0) {
            em->posY = em->floorY;
            EmPlesioth_ChangeState(em, EM_GROUP_IDLE, PLES_IDLE_LAND_FLOP_A, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * PLES_SWIM_BAIT_WAIT: hovers (0x64) for 120 frames by the target; if the
 * local target holds a monster bait (no other monster hooked in the area)
 * the line shakes (fishTimer, vibration); when that player reels
 * (PL_ACT_FISH_REEL) in the same area out of combat, takes the bait
 * (hooked, hookedBy) and goes to PLES_SWIM_HOOKED, else ends in the water.
 */
void EmPlesioth_WaitForBait(Em *em, EmWork *work)
{
    Player *target = &gPlayers[em->targetNo];

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        EmPlesioth_SetStance(em, 4, 0);
        Em_SetMotion(em, PLES_MOT_SWIM_IDLE, 0, 0);
        em->hooked = 0;
        em->timer = 120;
        em->noReact = 1;
        if (Player_IsLocal(target) == 1 && Lure_PlayerHasMonsterBait(target) == 1 && EmPlesioth_NoOtherHookedInArea(em, target) == 1) {
            target->fishTimer = em->timer;
            Vib_StartLocal(target, 2);
        }
        break;
    case 1:
        if (em->remote == 0 && em->combat == 0 && target->active && Player_IsLocal(target) == 1 && (s16) Player_IsState(target, 0, 0x53) && Obj_SameArea(em, target) && Lure_PlayerHasMonsterBait(target) == 1) {
            if (EmPlesioth_NoOtherHookedInArea(em, target)) {
                em->step++;
                em->hooked = 1;
                em->hookedBy = em->targetNo;
                EmPlesioth_ChangeState(em, EM_GROUP_FLY, PLES_SWIM_HOOKED, EM_SYNC_SEND);
                break;
            }
        } else if (--em->timer > 0 && em->combat != 1) {
            break;
        }
        em->step++;
        EmPlesioth_EndSwimState(em);
        break;
    }
}

/*
 * PLES_SWIM_HOOKED: on the line (0x84, guarded, no reactions), is pulled in
 * 195 frames to 700 ahead of the reeling player, with splashes, vibration
 * and the Plesioth camera scene for that player; then react state 0xF
 * (pulled out of the water).
 */
void EmPlesioth_HookedPulled(Em *em, EmWork *work)
{
    Player *target = &gPlayers[em->targetNo];
    f32 ahead[3];
    f32 aheadRotated[3];
    s32 playerAngles[3];
    f32 mtx[16];

    em->guardTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        EmPlesioth_SetStance(em, 4, 0);
        Em_SetMotion(em, PLES_MOT_HOOKED, 0, 0);
        em->hooked = 1;
        em->hookedBy = em->targetNo;
        em->noReact = 1;
        em->timer = 195;
        ahead[0] = 0.0f;
        ahead[1] = 0.0f;
        ahead[2] = 700.0f;
        playerAngles[0] = 0;
        playerAngles[1] = target->rot[1];
        playerAngles[2] = 0;
        Mtx_FromAnglesYXZ(playerAngles, mtx);
        Vec_Rotate(aheadRotated, ahead, mtx);
        em->goal[0] = target->pos[0] + aheadRotated[0];
        em->goal[1] = target->pos[1] + aheadRotated[1];
        em->goal[2] = target->pos[2] + aheadRotated[2];
        Player_ClearVelocity(em);
        em->rotY = Math_AngleXZ(&em->posX, em->goal);
        em->velY = (em->goal[1] - em->posY) / (f32) em->timer;
        em->velZ = Vec_DistanceXZ(&em->posX, em->goal) / (f32) em->timer;
        work->moveAngBX = 0;
        work->moveAngBZ = 0;
        EmPlesioth_SpawnSurfaceSplash(em, 8.0f);
        Vib_StartLocal(target, 2);
        if (Player_IsLocal(target) == 1) {
            CamReq_PlesiothScene(em);
        }
        break;
    case 1:
        work->moveAngBY = em->rotY;
        Em_MoveRotated(em, (s32 *) &work->moveAngBX);
        if (em->timer == 60 || em->timer == 120 || em->timer == 180 || em->timer == 240 || em->timer == 300) {
            EmPlesioth_SpawnSurfaceSplash(em, 8.0f);
        }
        if (em->timer == 30 || em->timer == 60 || em->timer == 90 || em->timer == 120 || em->timer == 150 ||
            em->timer == 180 || em->timer == 210 || em->timer == 240 || em->timer == 270 || em->timer == 300) {
            Vib_StartLocal(target, 2);
        }
        if (--em->timer <= 0) {
            em->step++;
            EmPlesioth_ChangeState(em, EM_GROUP_REACT, PLES_REACT_PULLED_ASHORE, EM_SYNC_OWNER);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmPlesioth_ChangeState(em, EM_GROUP_REACT, PLES_REACT_PULLED_ASHORE, EM_SYNC_OWNER);
        }
        break;
    }
}

/*
 * PLES_SWIM_STATUS2_COLLAPSE: status 2 under water: restarts the script,
 * jolts up through the surface (0x7E) and falls back, floats (0x82) with
 * lying puffs and a sound every 135 frames until the timer ends, then
 * PLES_SWIM_STATUS2_END; a hit wakes it early.
 */
void EmPlesioth_SwimStatus2Collapse(Em *em, EmWork *work)
{
    f32 puffOfs[3];
    f32 puffOfs2[3];

    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, PLES_MOT_TUMBLE, 0, 0);
        Player_ClearVelocity(em);
        em->velY = 60.0f;
        em->accelY = -2.75f;
        work->moveAngBX = 0;
        work->moveAngBZ = 0;
        em->blind = 2;
        break;
    case 1:
        work->moveAngBY = em->rotY;
        Em_MoveRotated(em, (s32 *) &work->moveAngBX);
        Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
        em->blind = 2;
        if (!(em->posY <= em->surfaceY)) {
            em->stance = EM_STANCE_AIR;
            em->step++;
            EmPlesioth_SpawnSurfaceSplash(em, 8.0f);
        }
        break;
    case 2:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, (s32 *) &work->moveAngBX);
        Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
        em->blind = 2;
        if (em->posY <= em->surfaceY) {
            em->step++;
            em->stance = EM_STANCE_UNDER;
            EmPlesioth_SpawnSurfaceSplash(em, 8.0f);
        }
        break;
    case 3:
        Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
        em->blind = 2;
        if (em->posY < em->surfaceY) {
            em->step++;
            em->lookAtTarget = 0;
            em->stance = EM_STANCE_UNDER;
            Em_SetMotion(em, PLES_MOT_SWIM_STATUS_FLOAT, 0, 0);
        }
        break;
    case 4:
        Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
        em->blind = 2;
        if (--em->timer <= 0) {
            em->step++;
            em->blind = 0;
            EmPlesioth_ChangeState(em, EM_GROUP_FLY, PLES_SWIM_STATUS2_END, EM_SYNC_OWNER_FLAGGED);
        }
        puffOfs[0] = -150.0f;
        puffOfs[1] = 50.0f;
        puffOfs[2] = 140.0f;
        Em_SpawnLyingPuffs(em, 0x22, puffOfs, 1.6f);
        if (em->timer % 135 == 0) {
            Em_PlaySeAtBone(em, 0x57, 0x23);
        }
        break;
    case 5:
        if (em->remote == 0) {
            EmPlesioth_ChangeState(em, EM_GROUP_FLY, PLES_SWIM_STATUS2_END, EM_SYNC_OWNER_FLAGGED);
        }
        puffOfs2[0] = -150.0f;
        puffOfs2[1] = 50.0f;
        puffOfs2[2] = 140.0f;
        Em_SpawnLyingPuffs(em, 0x22, puffOfs2, 1.6f);
        if (em->timer % 135 == 0) {
            Em_PlaySeAtBone(em, 0x57, 0x23);
        }
        break;
    }
}

/*
 * PLES_SWIM_STATUS1_COLLAPSE: as SwimStatus2Collapse under status 1, then
 * PLES_SWIM_STATUS1_END.
 */
void EmPlesioth_SwimStatus1Collapse(Em *em, EmWork *work)
{
    f32 puffOfs[3];
    f32 puffOfs2[3];

    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, PLES_MOT_TUMBLE, 0, 0);
        Player_ClearVelocity(em);
        em->velY = 60.0f;
        em->accelY = -2.75f;
        work->moveAngBX = 0;
        work->moveAngBZ = 0;
        EmPlesioth_SpawnSurfaceSplash(em, 8.0f);
        break;
    case 1:
        work->moveAngBY = em->rotY;
        Em_MoveRotated(em, (s32 *) &work->moveAngBX);
        if (!(em->posY <= em->surfaceY)) {
            em->stance = EM_STANCE_AIR;
            em->step++;
            EmPlesioth_SpawnSurfaceSplash(em, 8.0f);
        }
        break;
    case 2:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, (s32 *) &work->moveAngBX);
        if (em->posY <= em->surfaceY) {
            em->step++;
            em->stance = EM_STANCE_UNDER;
        }
        break;
    case 3:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, (s32 *) &work->moveAngBX);
        if (em->posY < em->surfaceY) {
            em->step++;
            em->lookAtTarget = 0;
            em->stance = EM_STANCE_UNDER;
            Em_SetMotion(em, PLES_MOT_SWIM_STATUS_FLOAT, 0, 0);
            Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
            em->blind = 2;
        }
        break;
    case 4:
        Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
        em->blind = 2;
        if (--em->timer <= 0) {
            em->step++;
            em->blind = 0;
            EmPlesioth_ChangeState(em, EM_GROUP_FLY, PLES_SWIM_STATUS1_END, EM_SYNC_OWNER_FLAGGED);
        }
        puffOfs[0] = -150.0f;
        puffOfs[1] = 50.0f;
        puffOfs[2] = 140.0f;
        Em_SpawnLyingPuffs(em, 0x22, puffOfs, 1.6f);
        if (em->timer % 135 == 0) {
            Em_PlaySeAtBone(em, 0x57, 0x23);
        }
        break;
    case 5:
        if (em->remote == 0) {
            EmPlesioth_ChangeState(em, EM_GROUP_FLY, PLES_SWIM_STATUS1_END, EM_SYNC_OWNER_FLAGGED);
        }
        puffOfs2[0] = -150.0f;
        puffOfs2[1] = 50.0f;
        puffOfs2[2] = 140.0f;
        Em_SpawnLyingPuffs(em, 0x22, puffOfs2, 1.6f);
        if (em->timer % 135 == 0) {
            Em_PlaySeAtBone(em, 0x57, 0x23);
        }
        break;
    }
}

/*
 * PLES_SWIM_STATUS4_RECOVER: recovers in the water (0x83) after status 4
 * under water (State4_12), then ends in the water.
 */
void EmPlesioth_SwimStatus4Recover(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        EmPlesioth_SetStance(em, 4, 0);
        Em_SetMotion(em, PLES_MOT_SWIM_RECOVER, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmPlesioth_EndSwimState(em);
        }
        break;
    }
}

/* PLES_SWIM_TO_IDLE: goes straight to PLES_SWIM_IDLE. */
void EmPlesioth_SwimToIdle(Em *em)
{
    EmPlesioth_ChangeState(em, EM_GROUP_FLY, PLES_SWIM_IDLE, EM_SYNC_OWNER_FLAGGED);
}

/*
 * PLES_SWIM_TRAP_ESCAPE: when the trap time runs out (State4_08,
 * State4_0D): on land, releases the hold (holdTimer, trapHit 0, no
 * reactions), falls onto its side (0x4B, 0x80), gets up (0x7C) and ends on
 * land.
 */
void EmPlesioth_TrapEscape(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, PLES_MOT_LIE_DOWN, 0, 0);
        EmPlesioth_SetStance(em, 0, 0);
        em->noReact = 1;
        em->holdTimer = 0;
        em->trapHit = 0;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, PLES_MOT_LIE_FLAP, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, PLES_MOT_GET_UP, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmPlesioth_EndGroundState(em);
        }
        break;
    }
}

/*
 * PLES_SWIM_STATUS2_END, PLES_SWIM_STATUS1_END: recovers in the water
 * (0x83, blind meanwhile), then ends in the water.
 */
void EmPlesioth_SwimStatusRecover(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        EmPlesioth_SetStance(em, 4, 0);
        em->blind = 2;
        Em_SetMotion(em, PLES_MOT_SWIM_RECOVER, 0, 0);
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            EmPlesioth_EndSwimState(em);
        }
        break;
    }
}

/*
 * PLES_ATK_LAND_A: on land (stance 0) plays PLES_MOT_LAND_ATTACK_A once
 * (its motion effects make attack row 1 at frame 50), then back to land
 * idle (EmPlesioth_EndGroundState).
 */
void EmPlesioth_LandAttackA(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        EmPlesioth_SetStance(em, 0, 0);
        Em_SetMotion(em, PLES_MOT_LAND_ATTACK_A, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmPlesioth_EndGroundState(em);
        }
        break;
    }
}

/*
 * PLES_ATK_BREATH: on land plays PLES_MOT_BREATH and at frame 78 fires the
 * ranged attack from bone 0x22 (EmRangedAttack_CreateAtBoneTimed, yaw
 * 0xF1C8, the yaw Kushala uses for its breath); back to land idle.
 * SetGroup3State sets EmPlesiothWork.unk013 to 1 on entry.
 */
void EmPlesioth_Breath(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        EmPlesioth_SetStance(em, 0, 0);
        Em_SetMotion(em, PLES_MOT_BREATH, 0, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 78.0f, 0U) != 0) {
            EmRangedAttack_CreateAtBoneTimed(em, 0x22, 1, 0, 0x444, 0xF1C8, (s32) (11.0f * em->motionSpeed));
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmPlesioth_EndGroundState(em);
        }
        break;
    }
}

/*
 * PLES_ATK_SPIN: on land plays PLES_MOT_SPIN twice while turning rotY by
 * -0x200 a frame (attack row 0x10 at frame 30, dust when enraged), then
 * back to land idle.
 */
void EmPlesioth_SpinAttack(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmPlesioth_SetStance(em, 0, 0);
        em->lookAtTarget = 0;
        Em_SetMotion(em, PLES_MOT_SPIN, 0, 0);
        Motion_InitTime(em, 0);
        break;
    case 1:
        em->rotY -= 0x200;
        if (em->motion[1].playing == 0) {
            em->step++;
            Em_SetMotion(em, PLES_MOT_SPIN, 0, 0);
        }
        break;
    case 2:
        em->rotY -= 0x200;
        if (em->motion[1].playing == 0) {
            em->step++;
            EmPlesioth_EndGroundState(em);
        }
        break;
    }
}

/*
 * PLES_ATK_SURFACE_BREATH_A: under water (stance 4) plays
 * PLES_MOT_SURFACE_BREATH_A: a splash at the surface at frame 30
 * (SpawnSurfaceEffect5), the ranged attack from bone 0x22 at frame 114;
 * then back to swimming (EmPlesioth_EndSwimState).
 */
void EmPlesioth_SurfaceBreathA(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, PLES_MOT_SURFACE_BREATH_A, 0, 0);
        EmPlesioth_SetStance(em, 4, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 114.0f, 0U) != 0) {
            EmRangedAttack_CreateAtBoneTimed(em, 0x22, 1, 0, 0x4FA, 0, (s32) (46.0f * em->motionSpeed));
        }
        if (EmMotion_PassedFrameScaled(em, 30.0f, 0U) != 0) {
            EmPlesioth_SpawnSurfaceSplash(em, 8.0f);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmPlesioth_EndSwimState(em);
        }
        break;
    }
}

/*
 * PLES_ATK_SURFACE_BREATH_B: as SurfaceBreathA with motion
 * PLES_MOT_SURFACE_BREATH_B and the same ranged attack arguments; nothing
 * in the code tells the two apart beyond the motion.
 */
void EmPlesioth_SurfaceBreathB(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, PLES_MOT_SURFACE_BREATH_B, 0, 0);
        EmPlesioth_SetStance(em, 4, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 114.0f, 0U) != 0) {
            EmRangedAttack_CreateAtBoneTimed(em, 0x22, 1, 0, 0x4FA, 0, (s32) (46.0f * em->motionSpeed));
        }
        if (EmMotion_PassedFrameScaled(em, 30.0f, 0U) != 0) {
            EmPlesioth_SpawnSurfaceSplash(em, 8.0f);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmPlesioth_EndSwimState(em);
        }
        break;
    }
}

/*
 * PLES_ATK_SURFACE_BREATH_C: as SurfaceBreathA with motion
 * PLES_MOT_SURFACE_BREATH_C, the ranged attack at frame 118 with other
 * arguments (1, 1, 0x71C instead of 1, 0, 0x4FA); back to swimming.
 */
void EmPlesioth_SurfaceBreathC(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, PLES_MOT_SURFACE_BREATH_C, 0, 0);
        EmPlesioth_SetStance(em, 4, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 118.0f, 0U) != 0) {
            EmRangedAttack_CreateAtBoneTimed(em, 0x22, 1, 1, 0x71C, 0, (s32) (30.0f * em->motionSpeed));
        }
        if (EmMotion_PassedFrameScaled(em, 30.0f, 0U) != 0) {
            EmPlesioth_SpawnSurfaceSplash(em, 8.0f);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmPlesioth_EndSwimState(em);
        }
        break;
    }
}

/*
 * PLES_ATK_LEAP: from under water leaps forward and up (velZ and velY 100,
 * gravity -5, PLES_MOT_LEAP, attack row 0xE at frame 10), airborne (stance
 * 2) above the surface; on falling back into the water plays
 * PLES_MOT_SWIM_FAST, slides 1000 forward (decelerating over 60 frames) and
 * returns to swimming; if it comes down on land (after 300 frames, or the
 * motion ends before it leaves the water) it goes to land idle state 0/4
 * (the flop on landing).
 */
void EmPlesioth_LeapOutOfWater(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0x0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, PLES_MOT_LEAP_ATTACK, 0, 0);
        EmPlesioth_SetStance(em, 4, 0);
        Player_ClearVelocity(em);
        em->velZ = 100.0f;
        em->velY = 100.0f;
        em->accelY = -5.0f;
        work->moveAngBX = 0;
        work->moveAngBZ = 0;
        em->timer = 0x12C;
        break;
    case 0x1:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, (s32 *) &work->moveAngBX);
        if (!(em->posY < em->surfaceY)) {
            em->step++;
            em->stance = EM_STANCE_AIR;
        }
        if (em->motion[0].playing == 0) {
            em->step = 0x63;
            EmPlesioth_EndSwimState(em);
        }
        break;
    case 0x2:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, (s32 *) &work->moveAngBX);
        floorY = em->floorY;
        if (em->posY < floorY) {
            em->posY = floorY;
            em->accelY = 0.0f;
        }
        if (em->posY <= em->surfaceY) {
            em->step++;
            em->stance = EM_STANCE_UNDER;
            em->posY = em->surfaceY;
            em->velY = 0.0f;
            em->accelY = 0.0f;
            Em_SetMotion(em, PLES_MOT_SWIM_FAST, 0xA, 0);
            em->goalDist = 1000.0f;
            em->timer = 0x3C;
            em->accelZ = (em->goalDist - em->timer * em->velZ) / ((em->timer * em->timer) / 2);
            EmPlesioth_SpawnSurfaceSplash(em, 8.0f);
            break;
        }
        if (--em->timer <= 0) {
            em->step = 0x63;
            em->posY = em->floorY;
            EmPlesioth_ChangeState(em, EM_GROUP_IDLE, PLES_IDLE_LAND_FLOP_A, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 0x3:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, (s32 *) &work->moveAngBX);
        em->goalDist -= em->velZ;
        if ((em->goalDist <= 0.0f) || (em->velZ <= 0.0f)) {
            em->step++;
            EmPlesioth_EndSwimState(em);
        }
        break;
    case 0x63:
        if (em->remote == 0) {
            em->posY = em->floorY;
            EmPlesioth_ChangeState(em, EM_GROUP_IDLE, PLES_IDLE_LAND_FLOP_A, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * PLES_ATK_LAND_B: on land plays PLES_MOT_LAND_ATTACK_B once (attack row
 * 0x16 at frame 54), then back to land idle.
 */
void EmPlesioth_LandAttackB(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        EmPlesioth_SetStance(em, 0, 0);
        Em_SetMotion(em, PLES_MOT_LAND_ATTACK_B, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmPlesioth_EndGroundState(em);
        }
        break;
    }
}

/*
 * PLES_REACT_FLINCH: a flinch of parts 0 and 7 on land
 * (EM_DMG_PART_FLINCH), or EM_DMG_HIT_MODE3 out of the air: plays
 * PLES_MOT_FLINCH (restarting motion 1 when layers 1 and 2 stop) and
 * returns to land idle.
 */
void EmPlesioth_Flinch(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, PLES_MOT_FLINCH, 0, 0);
        break;
    case 1:
        if (em->motion[em->stateVar1].playing == 0) {
            em->step++;
            EmPlesioth_EndGroundState(em);
            break;
        }
        if (em->motion[1].playing == 0) {
            Em_SetMotion(em, PLES_MOT_IDLE, 0, 0);
        }
        if (em->motion[2].playing == 0) {
            Em_SetMotion(em, PLES_MOT_IDLE, 0, 0);
        }
        break;
    }
}

/*
 * PLES_REACT_FLINCH_B: a flinch of an unbroken part 3 or 4 on land:
 * PLES_MOT_FLINCH_B, then land idle.
 */
void EmPlesioth_FlinchB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, PLES_MOT_FLINCH_B, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmPlesioth_EndGroundState(em);
        }
        break;
    }
}

/*
 * PLES_REACT_FLINCH_C: a flinch of parts 5 and 6 on land:
 * PLES_MOT_FLINCH_C, then land idle.
 */
void EmPlesioth_FlinchC(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, PLES_MOT_FLINCH_C, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmPlesioth_EndGroundState(em);
        }
        break;
    }
}

/*
 * PLES_REACT_FLINCH_D: a flinch of parts 1 and 2 on land:
 * PLES_MOT_FLINCH_D, then land idle.
 */
void EmPlesioth_FlinchD(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, PLES_MOT_FLINCH_D, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmPlesioth_EndGroundState(em);
        }
        break;
    }
}

/*
 * PLES_REACT_FLINCH_LYING: a part flinch while lying on land (stance 1):
 * PLES_MOT_LIE_DOWN, PLES_MOT_LIE_FLAP, PLES_MOT_GET_UP, then land idle.
 */
void EmPlesioth_FlinchLying(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        EmPlesioth_SetStance(em, 0, 0);
        Em_RestartScript(em);
        Em_SetMotion(em, PLES_MOT_LIE_DOWN, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, PLES_MOT_LIE_FLAP, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, PLES_MOT_GET_UP, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmPlesioth_EndGroundState(em);
        }
        break;
    }
}

/*
 * PLES_REACT_KNOCK_DOWN (variant 1), PLES_REACT_KNOCK_DOWN_PART3 (0),
 * PLES_REACT_STUN_A (3), PLES_REACT_STUN_B (2): falls over, lies and gets
 * up; odd variants use FALL_A, LIE_A, LIE_FLAP, GET_UP_A, even ones FALL_B,
 * LIE_DOWN, LIE_FLAP_B, GET_UP. The stun variants show stun stars at bone
 * 0x22, lie until em->timer runs out and end in land idle state 0/0x16
 * (which ends the stun); the others end in land idle.
 */
void EmPlesioth_KnockDown(Em *em, EmWork *work, s16 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, (s16) ((variant & 1) == 0 ? 0x4A : 0x45), 0, 0);
        switch (variant) {
        case 2:
        case 3:
            StunStars_CreateEm(em, 1, 0x22, em->timer + em->motion[0].frameCount / 2.0f, 150.0f, 3.0f);
            break;
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, (s16) ((variant & 1) == 0 ? 0x4B : 0x46), 0, 0);
            em->stateVar1 = 0;
        }
        break;
    case 2:
        switch (variant) {
        case 2:
        case 3:
            if (--em->timer <= 0) {
                em->stateVar1 = 1;
            }
            break;
        default:
            if (em->motion[0].playing == 0) {
                em->stateVar1 = 1;
            }
            break;
        }
        if (em->stateVar1) {
            em->step++;
            Em_SetMotion(em, (s16) ((variant & 1) == 0 ? 0x81 : 0x80), 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 0;
            Em_SetMotion(em, (s16) ((variant & 1) == 0 ? 0x7C : 0x47), 0, 0);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
            switch (variant) {
            case 2:
            case 3:
                em->step++;
                EmPlesioth_ChangeState(em, EM_GROUP_IDLE, PLES_IDLE_STUN_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
                break;
            default:
                EmPlesioth_EndGroundState(em);
                break;
            }
        }
        break;
    case 5:
        if (em->remote == 0) {
            EmPlesioth_ChangeState(em, EM_GROUP_IDLE, PLES_IDLE_STUN_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * PLES_REACT_FALL: EM_DMG_STATUS_IN_AIR, or a flinch while airborne (stance
 * 2): falls under gravity -10 with PLES_MOT_TUMBLE; landing in deep water
 * returns to swimming (EndState2), on land goes to PLES_REACT_LAND.
 */
void EmPlesioth_FallFromAir(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, PLES_MOT_TUMBLE, 0, 0);
        EmPlesioth_SetStance(em, 2, 0);
        em->accelX = 0.0f;
        em->accelY = -10.0f;
        em->accelZ = 0.0f;
        work->moveAngBX = 0;
        work->moveAngBZ = 0;
        break;
    case 1:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, &work->moveAngBX);
        if (!(em->posY < em->surfaceY)) {
            em->stance = EM_STANCE_AIR;
        } else {
            em->stance = EM_STANCE_UNDER;
        }
        if (EmPlesioth_OverDeepWater(em, 1) == 1) {
            if (em->posY < em->surfaceY) {
                em->stance = EM_STANCE_UNDER;
                em->posY = em->surfaceY;
                EmPlesioth_EndSwimState(em);
            }
        } else if (em->posY < em->floorY) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            EmPlesioth_ChangeState(em, EM_GROUP_REACT, PLES_REACT_LAND, EM_SYNC_OWNER);
        }
        break;
    case 2:
        if (em->remote == 0) {
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            EmPlesioth_ChangeState(em, EM_GROUP_REACT, PLES_REACT_LAND, EM_SYNC_OWNER);
        }
        break;
    }
}

/*
 * PLES_REACT_STATUS4_LYING: status 4 (EM_DMG_STATUS4) while lying on land
 * (stance 1); begins status 4 on entry: LIE_DOWN, LIE_FLAP, then
 * PLES_MOT_STATUS4_LYING with the status 4 effect until em->timer runs out,
 * then PLES_REACT_STATUS4_LYING_END.
 */
void EmPlesioth_Status4Lying(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_STATUS);
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, PLES_MOT_LIE_DOWN, 0, 0);
        EmPlesioth_SetStance(em, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, PLES_MOT_LIE_FLAP, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, PLES_MOT_STATUS4_LYING, 0, 0);
        }
        break;
    case 3:
        if (--em->timer <= 0) {
            em->step++;
            EmPlesioth_ChangeState(em, EM_GROUP_REACT, PLES_REACT_STATUS4_LYING_END, EM_SYNC_OWNER);
        }
        Em_SpawnStatus4Effect(em, 2);
        break;
    case 4:
        Em_SpawnStatus4Effect(em, 2);
        if (em->remote == 0) {
            EmPlesioth_ChangeState(em, EM_GROUP_REACT, PLES_REACT_STATUS4_LYING_END, EM_SYNC_OWNER);
        }
        break;
    }
}

/*
 * PLES_REACT_STATUS4_HELD_END (8, ends status 4 on entry) and
 * PLES_REACT_STUN_HELD_END (0x1B, ends the stun): held (holdTimer 5 each
 * frame), loops PLES_MOT_TRAP_STRUGGLE until trapTimer runs out, then swim
 * state 2/0x17 (the escape from the trap, on land).
 */
void EmPlesioth_HeldStatusStruggle(Em *em)
{
    em->holdTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, PLES_MOT_TRAP_STRUGGLE, 0, 0);
        em->noReact = 1;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            Em_SetMotion(em, PLES_MOT_TRAP_STRUGGLE, 0, 0);
        }
        if (--em->trapTimer <= 0) {
            em->step++;
            em->trapHit = 0;
            Em_SetMotion(em, PLES_MOT_TRAP_STRUGGLE, 0, 0);
            EmPlesioth_ChangeState(em, EM_GROUP_FLY, PLES_SWIM_TRAP_ESCAPE, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if ((em->motion[0].playing == 0) && (em->remote == 0)) {
            Em_SetMotion(em, PLES_MOT_TRAP_STRUGGLE, 0, 0);
            EmPlesioth_ChangeState(em, EM_GROUP_FLY, PLES_SWIM_TRAP_ESCAPE, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * PLES_REACT_FALL_BLIND: as FallFromAir but blinded (Em.blind 2 every
 * frame); no C code enters it.
 */
void EmPlesioth_FallBlinded(Em *em, EmWork *work)
{
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, PLES_MOT_TUMBLE, 0, 0);
        EmPlesioth_SetStance(em, 2, 0);
        em->accelX = 0.0f;
        em->accelY = -10.0f;
        em->accelZ = 0.0f;
        work->moveAngBX = 0;
        work->moveAngBZ = 0;
        break;
    case 1:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, &work->moveAngBX);
        if (!(em->posY < em->surfaceY)) {
            em->stance = EM_STANCE_AIR;
        } else {
            em->stance = EM_STANCE_UNDER;
        }
        if (EmPlesioth_OverDeepWater(em, 1) == 1) {
            if (em->posY < em->surfaceY) {
                em->stance = EM_STANCE_UNDER;
                em->posY = em->surfaceY;
                EmPlesioth_EndSwimState(em);
            }
        } else if (em->posY < em->floorY) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            EmPlesioth_ChangeState(em, EM_GROUP_REACT, PLES_REACT_LAND, EM_SYNC_OWNER);
        }
        break;
    case 2:
        if (em->remote == 0) {
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            EmPlesioth_ChangeState(em, EM_GROUP_REACT, PLES_REACT_LAND, EM_SYNC_OWNER);
        }
        break;
    }
}

/*
 * PLES_REACT_FLINCH_BLIND: blinded (Em.blind 2) on land, plays
 * PLES_MOT_FLINCH_BLIND once, then land idle; no C code enters it.
 */
void EmPlesioth_FlinchBlinded(Em *em)
{
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, PLES_MOT_FLINCH_BLIND, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmPlesioth_EndGroundState(em);
        }
        break;
    }
}

/*
 * PLES_REACT_STATUS4: status 4 on land (stance 0); begins status 4 on
 * entry: PLES_MOT_STAND_STILL with the status 4 effect until em->timer runs
 * out, then land idle state 0/0x14 (which ends status 4).
 */
void EmPlesioth_Status4(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_STATUS);
    switch (em->step) {
    case 0:
        em->step++;
        EmPlesioth_SetStance(em, 0, 0);
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, PLES_MOT_STAND_STILL, 0, 0);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmPlesioth_ChangeState(em, EM_GROUP_IDLE, PLES_IDLE_STATUS4_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmPlesioth_ChangeState(em, EM_GROUP_IDLE, PLES_IDLE_STATUS4_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
    Em_SpawnStatus4Effect(em, 2);
}

/*
 * PLES_REACT_TRAPPED: EM_DMG_ATTACK9 or ATTACK6 outside a hold (Update sets
 * the trap time first; ChangeState ends all statuses): PLES_MOT_TRAPPED,
 * held (holdTimer 50, kept at 5 or more), no flinch, then
 * PLES_REACT_TRAPPED_STRUGGLE.
 */
void EmPlesioth_Trapped(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_NO_FLINCH);
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, PLES_MOT_TRAPPED, 0, 0);
        em->noReact = 1;
        em->holdTimer = 0x32;
        em->blind = 0;
        break;
    case 1:
        if ((s8) em->holdTimer < 5) {
            em->holdTimer = 5;
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmPlesioth_ChangeState(em, EM_GROUP_REACT, PLES_REACT_TRAPPED_STRUGGLE, EM_SYNC_OWNER);
        }
        break;
    case 2:
        if ((s8) em->holdTimer < 5) {
            em->holdTimer = 5;
        }
        if (em->remote == 0) {
            EmPlesioth_ChangeState(em, EM_GROUP_REACT, PLES_REACT_TRAPPED_STRUGGLE, EM_SYNC_OWNER);
        }
        break;
    }
}

/*
 * PLES_REACT_TRAPPED_STRUGGLE: held, loops PLES_MOT_TRAP_STRUGGLE until
 * trapTimer runs out, then swim state 2/0x17 (escape from the trap). Same
 * code as HeldStatusStruggle.
 */
void EmPlesioth_TrappedStruggle(Em *em)
{
    em->holdTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, PLES_MOT_TRAP_STRUGGLE, 0, 0);
        em->noReact = 1;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            Em_SetMotion(em, PLES_MOT_TRAP_STRUGGLE, 0, 0);
        }
        if (--em->trapTimer <= 0) {
            em->step++;
            em->trapHit = 0;
            Em_SetMotion(em, PLES_MOT_TRAP_STRUGGLE, 0, 0);
            EmPlesioth_ChangeState(em, EM_GROUP_FLY, PLES_SWIM_TRAP_ESCAPE, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if ((em->motion[0].playing == 0) && (em->remote == 0)) {
            Em_SetMotion(em, PLES_MOT_TRAP_STRUGGLE, 0, 0);
            EmPlesioth_ChangeState(em, EM_GROUP_FLY, PLES_SWIM_TRAP_ESCAPE, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * PLES_REACT_STATUS4_HELD (variant 0, status 4 while held; begins status 4)
 * and PLES_REACT_STUN_HELD (variant 1, the stun while held, begins it, stun
 * stars at bone 0x22): held, lies with PLES_MOT_LIE_HELD until em->timer
 * runs out, sets the trap time goal, then PLES_REACT_STATUS4_HELD_END or
 * PLES_REACT_STUN_HELD_END. The second argument is unused (RunGroup4 passes
 * work).
 */
void EmPlesioth_HeldStatus(Em *em, s32 work, s32 variant)
{
    s32 variantByte;

    variantByte = variant & 0xFF;
    if (variantByte == 0) {
        Em_SetHitReactMode(em, EM_HIT_REACT_STATUS);
    }
    em->holdTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        EmPlesioth_SetStance(em, 0, 0);
        Em_SetMotion(em, PLES_MOT_TRAPPED_LIE, 0, 0);
        em->noReact = 1;
        Em_RestartScript(em);
        if (variantByte == 1) {
            StunStars_CreateEm(em, 1, 0x22, (s32) ((f32) em->timer + (em->motion[0].frameCount / 2.0f)), 150.0f, 3.0f);
        }
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetTrapTimeGoal(em);
            if (variantByte == 1) {
                EmPlesioth_ChangeState(em, EM_GROUP_REACT, PLES_REACT_STUN_HELD_END, EM_SYNC_OWNER);
            } else {
                EmPlesioth_ChangeState(em, EM_GROUP_REACT, PLES_REACT_STATUS4_HELD_END, EM_SYNC_OWNER);
            }
        }
        break;
    case 2:
        if (em->remote == 0) {
            if (variantByte == 1) {
                EmPlesioth_ChangeState(em, EM_GROUP_REACT, PLES_REACT_STUN_HELD_END, EM_SYNC_OWNER);
            } else {
                EmPlesioth_ChangeState(em, EM_GROUP_REACT, PLES_REACT_STATUS4_HELD_END, EM_SYNC_OWNER);
            }
        }
        break;
    }
    if (variantByte != 1) {
        Em_SpawnStatus4Effect(em, 2);
    }
}

/*
 * PLES_REACT_PULLED_ASHORE: after the bait reel (State2_12): ChangeState
 * breaks part 2 and aims at goal 1; jumps out of the water in a 30-frame
 * arc to the goal (PLES_MOT_TUMBLE, clears Em.hooked, enters combat), lands
 * (PLES_MOT_LAND_FLOP), makes a mine point and loses 100 hp (dying there
 * goes to PLES_DEFEAT_DIE_ASHORE), lies 300 frames, flaps, gets up, checks
 * rage and returns to land idle.
 */
void EmPlesioth_PulledAshore(Em *em, EmWork *work)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_NO_FLINCH);
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        EmPlesioth_SetStance(em, 4, 0);
        Player_ClearVelocity(em);
        Em_SetMotion(em, PLES_MOT_TUMBLE, 0, 0);
        em->rotY = Math_AngleXZ(&em->posX, em->goal);
        em->hooked = 0;
        em->timer = 30;
        em->accelY = -10.0f;
        em->velY = (em->goal[1] - em->posY) / (f32) em->timer - (em->accelY * (f32) em->timer) / 2.0f;
        em->velZ = Vec_DistanceXZ(&em->posX, em->goal) / (f32) em->timer;
        Em_SetCombat(em, 1, gEmCombatInit[em->type]);
        work->moveAngBX = 0;
        work->moveAngBZ = 0;
        Em_RestartScript(em);
        em->blind = 0;
        break;
    case 1:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, (s32 *) &work->moveAngBX);
        if (!(em->posY < em->surfaceY)) {
            em->step++;
            em->stance = EM_STANCE_AIR;
            EmPlesioth_SpawnSurfaceSplash(em, 8.0f);
        }
        break;
    case 2:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, (s32 *) &work->moveAngBX);
        if (em->velY < 0.0f) {
            if (em->posY < em->floorY) {
                em->step++;
                em->stance = EM_STANCE_GROUND;
                em->posY = em->floorY;
                Em_SetMotion(em, PLES_MOT_FLOP_A, 0, 0);
            }
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            EmMine_CreateTimed0(em, &em->posX);
            em->hp -= 100;
            if (em->remote == 0 && em->hp <= 0) {
                em->hp = 0;
                em->step = 0x63;
                EmPlesioth_ChangeState(em, EM_GROUP_DEFEAT, PLES_DEFEAT_DIE_ASHORE, EM_SYNC_OWNER);
                break;
            }
            em->step++;
            Em_SetMotion(em, PLES_MOT_LIE_DOWN, 0, 0);
            em->timer = 300;
        }
        break;
    case 4:
        if (--em->timer <= 0) {
            em->step++;
        }
        break;
    case 5:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, PLES_MOT_LIE_FLAP, 0, 0);
        }
        break;
    case 6:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, PLES_MOT_GET_UP, 0, 0);
        }
        break;
    case 7:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_CheckRageByHp(em, em->rageThreshold);
            EmPlesioth_EndGroundState(em);
        }
        break;
    }
}

/*
 * PLES_REACT_LAND: came down on land from a fall: PLES_MOT_LAND_FLOP,
 * PLES_MOT_GET_UP, then land idle.
 */
void EmPlesioth_LandAfterFall(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        EmPlesioth_SetStance(em, 0, 0);
        Em_SetMotion(em, PLES_MOT_FLOP_A, 0xC, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, PLES_MOT_GET_UP, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmPlesioth_EndGroundState(em);
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
            Player_ClearVelocity(em);
        }
        break;
    }
}

/*
 * PLES_REACT_THROWN_OUT: under water (stance 4), EM_DMG_ATTACK5,
 * EM_DMG_ATTACK5_FLINCH or a part flinch: shoots up out of the water (velY
 * 60, PLES_MOT_TUMBLE, splashes), falls back; deep water returns to
 * swimming, land goes to PLES_REACT_LAND.
 */
void EmPlesioth_ThrownOutOfWater(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, PLES_MOT_TUMBLE, 0, 0);
        Player_ClearVelocity(em);
        em->velY = 60.0f;
        em->accelY = -2.75f;
        work->moveAngBX = 0;
        work->moveAngBZ = 0;
        break;
    case 1:
        work->moveAngBY = em->rotY;
        Em_MoveRotated(em, &work->moveAngBX);
        if (!(em->posY <= em->surfaceY)) {
            em->stance = EM_STANCE_AIR;
            em->step++;
            EmPlesioth_SpawnSurfaceSplash(em, 8.0f);
        }
        break;
    case 2:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, &work->moveAngBX);
        if (em->posY <= em->surfaceY) {
            em->step++;
            em->stance = EM_STANCE_UNDER;
            EmPlesioth_SpawnSurfaceSplash(em, 8.0f);
        }
        break;
    case 3:
        if (EmPlesioth_OverDeepWater(em, 1U) == 1) {
            if (em->posY <= em->surfaceY) {
                em->stance = EM_STANCE_UNDER;
                em->posY = em->surfaceY;
                EmPlesioth_EndSwimState(em);
                break;
            }
            work->moveAngBY = em->rotY;
            Em_MoveRotatedAccel(em, &work->moveAngBX);
            if (em->stance == EM_STANCE_AIR) {
                if (!(em->posY <= em->surfaceY)) {
                    em->stance = EM_STANCE_AIR;
                    break;
                }
                em->stance = EM_STANCE_UNDER;
                break;
            }
        } else if (em->posY < em->floorY) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            EmPlesioth_ChangeState(em, EM_GROUP_REACT, PLES_REACT_LAND, EM_SYNC_OWNER);
        }
        break;
    case 4:
        if (em->remote == 0) {
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            EmPlesioth_ChangeState(em, EM_GROUP_REACT, PLES_REACT_LAND, EM_SYNC_OWNER);
        }
        break;
    }
}

/*
 * PLES_REACT_STATUS4_UNDER: status 4 under water: thrown up out of the
 * water like ThrownOutOfWater, falls back and floats at the surface
 * (PLES_MOT_FLOAT) with the status 4 effect until em->timer runs out, then
 * swim state 2/0x15 (which ends status 4).
 */
void EmPlesioth_Status4Under(Em *em, EmWork *work)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_STATUS);
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, PLES_MOT_TUMBLE, 0, 0);
        Player_ClearVelocity(em);
        em->velY = 60.0f;
        em->accelY = -2.75f;
        work->moveAngBX = 0;
        work->moveAngBZ = 0;
        em->noReact = 1;
        break;
    case 1:
        work->moveAngBY = em->rotY;
        Em_MoveRotated(em, (s32 *) &work->moveAngBX);
        if (!(em->posY <= em->surfaceY)) {
            em->stance = EM_STANCE_AIR;
            em->step++;
            EmPlesioth_SpawnSurfaceSplash(em, 8.0f);
            break;
        }
    default:
        break;
    case 2:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, (s32 *) &work->moveAngBX);
        if (em->posY <= em->surfaceY) {
            em->step++;
            em->stance = EM_STANCE_UNDER;
            EmPlesioth_SpawnSurfaceSplash(em, 8.0f);
        }
        break;
    case 3:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, (s32 *) &work->moveAngBX);
        if (em->posY < em->surfaceY) {
            em->step++;
            em->posY = em->surfaceY;
            em->lookAtTarget = 0;
            em->stance = EM_STANCE_UNDER;
            Em_SetMotion(em, PLES_MOT_SWIM_STATUS_FLOAT, 0, 0);
        }
        break;
    case 4:
        if (--em->timer <= 0) {
            em->step++;
            EmPlesioth_ChangeState(em, EM_GROUP_FLY, PLES_SWIM_STATUS4_RECOVER, EM_SYNC_OWNER_FLAGGED);
        }
        Em_SpawnStatus4Effect(em, 2);
        break;
    case 5:
        if (em->remote == 0) {
            EmPlesioth_ChangeState(em, EM_GROUP_FLY, PLES_SWIM_STATUS4_RECOVER, EM_SYNC_OWNER_FLAGGED);
        }
        Em_SpawnStatus4Effect(em, 2);
        break;
    }
}

/*
 * PLES_REACT_STATUS2_TRAPPED_END (0x13, ends status 2) and
 * PLES_REACT_STATUS1_TRAPPED_END (0x14, ends status 1): from the
 * lying-in-trap idle states 0/8 and 0/0xA (their timer, or
 * EM_DMG_HIT_LYING): held and blinded, plays PLES_MOT_WAKE_IN_TRAP, sets
 * the trap time goal, then PLES_REACT_TRAPPED_STRUGGLE.
 */
void EmPlesioth_WakeInTrap(Em *em)
{
    em->holdTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        EmPlesioth_SetStance(em, 0, 0);
        em->lookAtTarget = 0;
        Em_SetMotion(em, PLES_MOT_WAKE_IN_TRAP, 0, 0);
        em->blind = 2;
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            Em_SetTrapTimeGoal(em);
            EmPlesioth_ChangeState(em, EM_GROUP_REACT, PLES_REACT_TRAPPED_STRUGGLE, EM_SYNC_OWNER);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmPlesioth_ChangeState(em, EM_GROUP_REACT, PLES_REACT_TRAPPED_STRUGGLE, EM_SYNC_OWNER);
        }
        break;
    }
}

/*
 * PLES_REACT_STATUS4_LYING_END: ends status 4 on entry; still lying (stance
 * 1) gets up (PLES_MOT_GET_UP), then land idle.
 */
void EmPlesioth_Status4LyingEnd(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, PLES_MOT_GET_UP, 0, 0);
        EmPlesioth_SetStance(em, 1, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmPlesioth_EndGroundState(em);
        }
        break;
    }
}

/*
 * PLES_REACT_STATUS1_UNDER: result 7 (EM_DMG_CAPTURED) outside a hold while
 * under water; begins status 1 on entry: thrown up out of the water, falls
 * back and floats at the surface (PLES_MOT_FLOAT) blinded, with lying puffs
 * and sound 0x57 every 135 frames, until em->timer runs out or
 * EM_DMG_HIT_LYING, then swim state 2/0x19 (which ends status 1).
 */
void EmPlesioth_Status1Under(Em *em, EmWork *work)
{
    f32 puffOfs[3];
    f32 puffOfs2[3];

    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, PLES_MOT_TUMBLE, 0, 0);
        Player_ClearVelocity(em);
        em->velY = 60.0f;
        em->accelY = -2.75f;
        work->moveAngBX = 0;
        work->moveAngBZ = 0;
        EmPlesioth_SpawnSurfaceSplash(em, 8.0f);
        break;
    case 1:
        work->moveAngBY = em->rotY;
        Em_MoveRotated(em, (s32 *) &work->moveAngBX);
        if (!(em->posY <= em->surfaceY)) {
            em->stance = EM_STANCE_AIR;
            em->step++;
            EmPlesioth_SpawnSurfaceSplash(em, 8.0f);
        }
        break;
    case 2:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, (s32 *) &work->moveAngBX);
        if (em->posY <= em->surfaceY) {
            em->step++;
            em->stance = EM_STANCE_UNDER;
        }
        break;
    case 3:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, (s32 *) &work->moveAngBX);
        if (em->posY < em->surfaceY) {
            em->step++;
            em->lookAtTarget = 0;
            em->stance = EM_STANCE_UNDER;
            Em_SetMotion(em, PLES_MOT_SWIM_STATUS_FLOAT, 0, 0);
            em->blind = 2;
            Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
        }
        break;
    case 4:
        Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
        em->blind = 2;
        if (--em->timer <= 0) {
            em->step++;
            em->blind = 0;
            EmPlesioth_ChangeState(em, EM_GROUP_FLY, PLES_SWIM_STATUS1_END, EM_SYNC_OWNER_FLAGGED);
        }
        puffOfs[0] = -150.0f;
        puffOfs[1] = 50.0f;
        puffOfs[2] = 140.0f;
        Em_SpawnLyingPuffs(em, 0x22, puffOfs, 1.6f);
        if (em->timer % 135 == 0) {
            Em_PlaySeAtBone(em, 0x57, 0x23);
        }
        break;
    case 5:
        if (em->remote == 0) {
            EmPlesioth_ChangeState(em, EM_GROUP_FLY, PLES_SWIM_STATUS1_END, EM_SYNC_OWNER_FLAGGED);
        }
        puffOfs2[0] = -150.0f;
        puffOfs2[1] = 50.0f;
        puffOfs2[2] = 140.0f;
        Em_SpawnLyingPuffs(em, 0x22, puffOfs2, 1.6f);
        if (em->timer % 135 == 0) {
            Em_PlaySeAtBone(em, 0x57, 0x23);
        }
        break;
    }
}

/*
 * PLES_REACT_HOLD2: EM_DMG_ATTACK13; ends all statuses on entry; the second
 * hold every frame (Em_SetHold2): PLES_MOT_FLINCH without root motion up to
 * frame 52, then PLES_MOT_STAND_STILL until em->timer runs out, then land
 * idle state 0/0x15 (which clears holdTimer2).
 */
void EmPlesioth_Hold2(Em *em)
{
    Em_SetHold2(em);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, PLES_MOT_FLINCH, 0, 0);
        Player_SetFlag((Player *) em, EM_FLAG_NO_ROOT_MOTION);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 52.0f) != 0) {
            em->step++;
            Em_SetMotion(em, PLES_MOT_STAND_STILL, 0xC, 0);
            Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            EmPlesioth_ChangeState(em, EM_GROUP_IDLE, PLES_IDLE_HOLD2_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmPlesioth_ChangeState(em, EM_GROUP_IDLE, PLES_IDLE_HOLD2_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * PLES_EVENT_CAPTURED: captured in a hold (result 7 with holdTimer set;
 * ChangeState begins status 1): Em_SetCaptured(em, 0) every frame, lies
 * (PLES_MOT_LIE_HELD) with lying puffs and sound 0x57 every 135 frames,
 * reports the capture (QuestEm_Captured), waits em->timer once
 * QuestEm_CanAppear, fades out over 150 frames, then PLES_DEFEAT_GONE.
 */
void EmPlesioth_Captured(Em *em, EmWork *work)
{
    f32 pos[3];

    Em_SetCaptured(em, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, PLES_MOT_TRAPPED_LIE, 0, 0);
        Em_LoadReactionSet(em, 1);
        Em_ClearPoison(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            QuestEm_Captured(em);
        }
        break;
    case 2:
        if (QuestEm_CanAppear(em) != 0) {
            if (--em->timer <= 0) {
                em->step++;
                em->timer = 150;
            }
        }
        break;
    case 3:
        if (em->timer > 0) {
            em->timer--;
        }
        if (em->timer <= 0) {
            if (em->remote == 0) {
                EmPlesioth_ChangeState(em, EM_GROUP_DEFEAT, PLES_DEFEAT_GONE, EM_SYNC_OWNER_FLAGGED);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    }
    pos[0] = 0.0f;
    pos[1] = 10.0f;
    pos[2] = 140.0f;
    Em_SpawnLyingPuffs(em, 0x22, pos, 1.6f);
    if (++em->timer % 135 == 0) {
        Em_PlaySeAtBone(em, 0x57, 0x23);
    }
}

/*
 * PLES_EVENT_ENTRANCE: a scripted entrance at fixed positions: floats
 * (PLES_MOT_FLOAT), PLES_MOT_SURFACE_WAKE, then swims a fixed route of
 * goals (SWIM, SWIM_FAST, two circling turns with SWIM_TURN, a leap with
 * PLES_MOT_LEAP), waits 210 frames, does a surface breath
 * (PLES_MOT_SURFACE_BREATH_B, ranged attack at frame 114) and returns to
 * swimming. Keeps senseDelay and noHitTimer at 5. No C code enters it.
 */
void EmPlesioth_EntranceScene(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->posX = 6694.0f;
        em->posY = -550.0f;
        em->posZ = 17459.0f;
        em->goal[0] = 10412.0f;
        em->goal[1] = -550.0f;
        em->goal[2] = 12820.0f;
        em->rotX = 0;
        em->rotY = Math_AngleXZ(&em->posX, em->goal);
        em->rotZ = 0;
        em->goalDist = Vec_DistanceXZ(&em->posX, em->goal);
        Player_ClearVelocity(em);
        Em_SetMotion(em, PLES_MOT_SWIM_STATUS_FLOAT, 0, 0);
        work->moveAngBX = 0;
        work->moveAngBZ = 0;
        em->timer = 180;
        em->fieldCollide = 0;
        break;
    case 1:
        em->fieldCollide = 1;
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, PLES_MOT_SWIM_RECOVER, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->posX = 6649.0f;
            em->posY = -550.0f;
            em->posZ = 17459.0f;
            em->goal[0] = 10412.0f;
            em->goal[1] = -550.0f;
            em->goal[2] = 12820.0f;
            em->rotX = 0;
            em->rotY = Math_AngleXZ(&em->posX, em->goal);
            em->rotZ = 0;
            em->goalDist = Vec_DistanceXZ(&em->posX, em->goal);
            Em_SetMotion(em, PLES_MOT_SWIM, 0, 0);
            work->moveAngBX = 0;
            work->moveAngBZ = 0;
            em->timer = 30;
        }
        break;
    case 3:
        if (--em->timer <= 0) {
            em->step++;
            em->velZ = 30.0f;
        }
        break;
    case 4:
        work->moveAngBY = em->rotY;
        Em_MoveRotated(em, (s32 *) &work->moveAngBX);
        em->goalDist -= em->velZ;
        if (em->goalDist <= 0.0f) {
            em->step++;
            Player_ClearVelocity(em);
            em->posX = 15251.0f;
            em->posY = -230.0f;
            em->posZ = 13531.0f;
            em->goal[0] = 14094.0f;
            em->goal[1] = -230.0f;
            em->goal[2] = 13301.0f;
            em->rotX = 0;
            em->rotY = Math_AngleXZ(&em->posX, em->goal);
            em->rotZ = 0;
            em->goalDist = Vec_DistanceXZ(&em->posX, em->goal);
            work->moveAngBX = 0;
            work->moveAngBZ = 0;
            em->velZ = 20.0f;
        }
        EmPlesioth_SpawnWakeEvery4(em, 8.0f);
        break;
    case 5:
        work->moveAngBY = em->rotY;
        Em_MoveRotated(em, (s32 *) &work->moveAngBX);
        em->goalDist -= em->velZ;
        if (em->goalDist <= 0.0f) {
            em->step++;
            Player_ClearVelocity(em);
            em->goal[0] = 10191.0f;
            em->goal[1] = -200.0f;
            em->goal[2] = 12523.0f;
            em->rotX = 0;
            em->rotY = Math_AngleXZ(&em->posX, em->goal);
            em->rotZ = 0;
            em->goalDist = Vec_DistanceXZ(&em->posX, em->goal);
            em->velZ = 50.0f;
            Em_SetMotion(em, PLES_MOT_SWIM_FAST, 0, 0);
        }
        EmPlesioth_SpawnWakeEvery4(em, 8.0f);
        break;
    case 6:
        work->moveAngBY = em->rotY;
        Em_MoveRotated(em, (s32 *) &work->moveAngBX);
        em->goalDist -= em->velZ;
        if (em->goalDist <= 0.0f) {
            em->step++;
            Player_ClearVelocity(em);
            em->posX = 10679.0f;
            em->posY = -550.0f;
            em->posZ = 11603.0f;
            em->goal[0] = 10000.0f;
            em->goal[1] = -550.0f;
            em->goal[2] = 8560.0f;
            em->rotX = 0;
            em->rotY = Math_AngleXZ(&em->posX, em->goal) - 0x3000;
            em->rotZ = 0;
            Em_SetMotion(em, PLES_MOT_SWIM_CIRCLE_RIGHT, 0, 0);
            work->moveAngBX = 0;
            work->moveAngBZ = 0;
            em->timer = 90;
            em->velZ = 60.0f;
        }
        EmPlesioth_SpawnWakeEvery4(em, 8.0f);
        break;
    case 7:
        em->rotY += 0x100;
        em->rotY = (u16) em->rotY;
        work->moveAngBX = 0;
        work->moveAngBY = em->rotY;
        work->moveAngBZ = 0;
        Em_MoveRotated(em, (s32 *) &work->moveAngBX);
        if (--em->timer <= 0) {
            em->step++;
            Player_ClearVelocity(em);
            em->posX = 5000.0f;
            em->posY = -550.0f;
            em->posZ = 7950.0f;
            em->goal[0] = 5570.0f;
            em->goal[1] = -550.0f;
            em->goal[2] = 8900.0f;
            em->rotX = 0;
            em->rotY = Math_AngleXZ(&em->posX, em->goal) - 0x3000;
            em->rotZ = 0;
            Em_SetMotion(em, PLES_MOT_SWIM_CIRCLE_RIGHT, 0, 0);
            work->moveAngBX = 0;
            work->moveAngBZ = 0;
            em->timer = 45;
            em->velZ = 20.0f;
        }
        EmPlesioth_SpawnWakeEvery4(em, 8.0f);
        break;
    case 8:
        em->rotY += 0x180;
        em->rotY = (u16) em->rotY;
        work->moveAngBX = 0;
        work->moveAngBY = em->rotY;
        work->moveAngBZ = 0;
        Em_MoveRotated(em, (s32 *) &work->moveAngBX);
        if (--em->timer <= 0) {
            em->step++;
            em->goal[0] = 6600.0f;
            em->goal[1] = -550.0f;
            em->goal[2] = 9330.0f;
            em->rotX = 0;
            em->rotY = Math_AngleXZ(&em->posX, em->goal);
            em->rotZ = 0;
            em->goalDist = Vec_DistanceXZ(&em->posX, em->goal);
            Em_SetMotion(em, PLES_MOT_LEAP_ATTACK, 0, 0);
            em->velZ = 50.0f;
        }
        EmPlesioth_SpawnWakeEvery4(em, 8.0f);
        break;
    case 9:
        work->moveAngBY = em->rotY;
        Em_MoveRotated(em, (s32 *) &work->moveAngBX);
        em->goalDist -= em->velZ;
        if (em->goalDist <= 0.0f) {
            em->step++;
            em->goal[0] = 9120.0f;
            em->goal[1] = -550.0f;
            em->goal[2] = 10400.0f;
            em->rotX = 0;
            em->rotY = Math_AngleXZ(&em->posX, em->goal);
            em->rotZ = 0;
            em->goalDist = Vec_DistanceXZ(&em->posX, em->goal);
            Em_SetMotion(em, PLES_MOT_SWIM_FAST, 0, 0);
            em->velZ = 30.0f;
            em->velY = -5.0f;
        }
        EmPlesioth_SpawnWakeEvery4(em, 8.0f);
        break;
    case 10:
        work->moveAngBY = em->rotY;
        Em_MoveRotated(em, (s32 *) &work->moveAngBX);
        em->goalDist -= em->velZ;
        if (em->goalDist <= 0.0f) {
            em->step++;
            Em_SetMotion(em, PLES_MOT_IDLE, 0, 0);
            Player_ClearVelocity(em);
            em->timer = 210;
        }
        EmPlesioth_SpawnWakeEvery4(em, 8.0f);
        break;
    case 11:
        if (--em->timer <= 0) {
            em->step++;
            em->posX = 9100.0f;
            em->posY = -600.0f;
            em->posZ = 10350.0f;
            em->goal[0] = 11131.0f;
            em->goal[1] = -600.0f;
            em->goal[2] = 11228.0f;
            em->rotX = 0;
            em->rotY = Math_AngleXZ(&em->posX, em->goal);
            em->rotZ = 0;
            Em_SetMotion(em, PLES_MOT_SURFACE_BREATH_B, 0, 0);
        }
        EmPlesioth_SpawnWakeEvery4(em, 8.0f);
        break;
    case 12:
        if (EmMotion_PassedFrameScaled(em, 114.0f, 0)) {
            EmRangedAttack_CreateAtBoneTimed(em, 0x22, 1, 0, 0x71C, 0, (s32) (46.0f * em->motionSpeed));
        }
        if (EmMotion_PassedFrameScaled(em, 30.0f, 0)) {
            EmPlesioth_SpawnSurfaceSplash(em, 8.0f);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, PLES_MOT_IDLE, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    case 13:
        em->step++;
        EmPlesioth_EndSwimState(em);
        break;
    }
}

/*
 * PLES_EVENT_CAPTURED_HOLD2: captured in the second hold (result 7 with
 * holdTimer2 set): Em_SetCaptured(em, 1), reports the capture, begins
 * status 1, collapses (PLES_MOT_COLLAPSE) and lies (PLES_MOT_LIE_STATUS);
 * 1800 frames after QuestEm_CanAppear fades out over 150 frames, then
 * PLES_DEFEAT_GONE.
 */
void EmPlesioth_CapturedHold2(Em *em, EmWork *work)
{
    s32 timerLeft;
    s32 timerBefore;

    Em_SetCaptured(em, 1);
    switch (em->step) {
    case 0:
        em->step++;
        QuestEm_Captured(em);
        em->timer = 0x708;
        Em_BeginStatus1(em);
        Em_ClearPoison(em);
        em->stance = EM_STANCE_GROUND;
        EmPlesioth_SetStance(em, 0, 1);
        em->lookAtTarget = 0;
        Em_SetMotion(em, PLES_MOT_STATUS_COLLAPSE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, PLES_MOT_STATUS_LIE, 0, 0);
        }
        break;
    case 2:
        if (QuestEm_CanAppear(em) != 0) {
            if (--em->timer <= 0) {
                em->step++;
                em->timer = 0x96;
                break;
            }
        }
        break;
    case 3:
        timerBefore = em->timer;
        if (timerBefore > 0) {
            em->timer = timerBefore - 1;
        }
        timerLeft = em->timer;
        if (timerLeft <= 0) {
            if (em->remote == 0) {
                EmPlesioth_ChangeState(em, EM_GROUP_DEFEAT, PLES_DEFEAT_GONE, EM_SYNC_OWNER_FLAGGED);
                break;
            }
        } else {
            em->alpha = (f32) timerLeft / 150.0f;
        }
        break;
    }
}

/*
 * PLES_DEFEAT_DIE: killed on land (EM_DMG_KILLED or DEAD, not held, not
 * under water): out of combat, stance 3, QuestEm_Hunted, PLES_MOT_DIE
 * (camera shake at frame 212), then a carve mine point for 1800 frames once
 * QuestEm_CanAppear, fades out over 150 frames, then PLES_DEFEAT_GONE.
 */
void EmPlesioth_Die(Em *em, EmWork *work)
{
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, PLES_MOT_DIE, 0, 0);
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_DEAD;
        QuestEm_Hunted(em);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 212.0f, 0) != 0) {
            CamShake_ReqAIf(em, 2);
        }
        if (em->motion[0].playing == 0) {
            em->timer = 1800;
            em->step++;
            EmMine_Create(em, 0);
            em->fieldCollide = 0;
        }
        break;
    case 2:
        EmMine_Update(em);
        if (QuestEm_CanAppear(em) != 0) {
            if (--em->timer <= 0) {
                em->step++;
                EmMine_Remove(em);
                em->timer = 150;
            }
        }
        break;
    case 3:
        if (em->timer > 0) {
            em->timer--;
        }
        if (em->timer <= 0) {
            if (em->remote == 0) {
                EmPlesioth_ChangeState(em, EM_GROUP_DEFEAT, PLES_DEFEAT_GONE, EM_SYNC_OWNER_FLAGGED);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    }
}

/*
 * PLES_DEFEAT_DIE_HELD: killed while held: as Die with PLES_MOT_LIE_HELD
 * and a 450-frame mine point.
 */
void EmPlesioth_DieHeld(Em *em, EmWork *work)
{
    em->holdTimer = 5;
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, PLES_MOT_TRAPPED_LIE, 0, 0);
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_DEAD;
        QuestEm_Hunted(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->timer = 450;
            em->step++;
            EmMine_Create(em, 0);
            em->fieldCollide = 0;
        }
        break;
    case 2:
        EmMine_Update(em);
        if (QuestEm_CanAppear(em) != 0) {
            if (--em->timer <= 0) {
                em->step++;
                EmMine_Remove(em);
                em->timer = 150;
            }
        }
        break;
    case 3:
        if (em->timer > 0) {
            em->timer--;
        }
        if (em->timer <= 0) {
            if (em->remote == 0) {
                EmPlesioth_ChangeState(em, EM_GROUP_DEFEAT, PLES_DEFEAT_GONE, EM_SYNC_OWNER_FLAGGED);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    }
}

/*
 * PLES_DEFEAT_DIE_UNDER: killed under water (stance 4): thrown up out of
 * the water (PLES_MOT_TUMBLE), falls back, sinks and rises to the surface
 * (PLES_MOT_DEAD_FLOAT, motion frozen at the surface), QuestEm_Hunted, a
 * mine point, bobs at the surface, fades out, then PLES_DEFEAT_GONE.
 */
void EmPlesioth_DieUnder(Em *em, EmWork *work)
{
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, PLES_MOT_TUMBLE, 0, 0);
        Player_ClearVelocity(em);
        em->velY = 60.0f;
        em->accelY = -2.75f;
        work->moveAngBX = 0;
        work->moveAngBZ = 0;
        break;
    case 1:
        work->moveAngBY = em->rotY;
        Em_MoveRotated(em, &work->moveAngBX);
        if (!(em->posY <= em->surfaceY)) {
            em->step++;
            em->stance = EM_STANCE_AIR;
            EmPlesioth_SpawnSurfaceSplash(em, 8.0f);
        }
        break;
    case 2:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, &work->moveAngBX);
        if (em->posY <= em->surfaceY) {
            em->step++;
            em->stance = EM_STANCE_UNDER;
            EmPlesioth_SpawnSurfaceSplash(em, 8.0f);
        }
        break;
    case 3:
        if (em->posY < em->surfaceY) {
            em->step++;
            em->stance = EM_STANCE_UNDER;
            em->accelY = 1.0f;
            Em_SetMotion(em, PLES_MOT_DEAD_FLOAT, 0, 0);
            QuestEm_Hunted(em);
            *(f32 *) &em->surfaceDepth = 444.3f;
        } else {
            work->moveAngBY = em->rotY;
            Em_MoveRotatedAccel(em, &work->moveAngBX);
            if (em->stance == EM_STANCE_AIR) {
                if (!(em->posY < em->surfaceY)) {
                    em->stance = EM_STANCE_AIR;
                } else {
                    em->stance = EM_STANCE_UNDER;
                }
            }
        }
        break;
    case 4:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, &work->moveAngBX);
        if (!(em->velY < 5.0f)) {
            em->velY = 5.0f;
            em->accelY = 0.0f;
        }
        if (!(em->posY < em->surfaceY)) {
            em->posY = em->surfaceY;
            *(s32 *) &em->motionSpeed = 0;
            em->step++;
            em->stateVar0 = 0;
            EmMine_Create(em, 0);
            em->fieldCollide = 0;
        }
        break;
    case 5:
        switch (em->stateVar0) {
        case 0:
            em->stateVar0++;
            *(f32 *) &em->velY = -0.5f;
            em->timer = (Rand_Next(1) & 3) * 10;
            break;
        case 1:
            work->moveAngBY = em->rotY;
            Em_MoveRotated(em, &work->moveAngBX);
            if (--em->timer <= 0) {
                em->stateVar0++;
                *(f32 *) &em->velY = 1.0f;
            }
            break;
        case 2:
            work->moveAngBY = em->rotY;
            Em_MoveRotated(em, &work->moveAngBX);
            if (!(em->posY < em->surfaceY)) {
                em->posY = em->surfaceY;
                em->stateVar0 = 0;
            }
            break;
        }
        EmMine_Update(em);
        if (QuestEm_CanAppear(em) != 0) {
            if (--em->timer <= 0) {
                em->step++;
                EmMine_Remove(em);
                em->timer = 150;
            }
        }
        break;
    case 6:
        if (em->timer > 0) {
            em->timer--;
        }
        if (em->timer <= 0) {
            if (em->remote == 0) {
                EmPlesioth_ChangeState(em, EM_GROUP_DEFEAT, PLES_DEFEAT_GONE, EM_SYNC_OWNER_FLAGGED);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    }
}

/*
 * PLES_DEFEAT_DIE_ASHORE: killed by the 100 hp of PulledAshore: lies down,
 * flaps, dies lying (PLES_MOT_DIE_LYING), a 450-frame mine point, fades
 * out, then PLES_DEFEAT_GONE.
 */
void EmPlesioth_DieAshore(Em *em, EmWork *work)
{
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, PLES_MOT_LIE_DOWN, 0, 0);
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_DEAD;
        QuestEm_Hunted(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, PLES_MOT_LIE_FLAP, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, PLES_MOT_DIE_LYING, 0, 0);
        }
    case 3:
        if (em->motion[0].playing == 0) {
            em->timer = 450;
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
        if (em->timer <= 0) {
            if (em->remote == 0) {
                EmPlesioth_ChangeState(em, EM_GROUP_DEFEAT, PLES_DEFEAT_GONE, EM_SYNC_OWNER_FLAGGED);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    }
}

/*
 * Runs the current group-0 state (EM_GROUP_IDLE). States 4 and 9 share
 * State0_04 (variant 0, 1), 5 and 41 State0_05 (variant 0, 1), 6, 13 and 42
 * State0_06, 7, 20, 21 and 22 State0_07, 15 and 17 State0_0F; states 3, 11
 * and 23-40 have no case.
 */
void EmPlesioth_RunIdleState(Em *em, EmWork *work)
{
    switch (em->state) {
    case PLES_IDLE_STAND:
        EmPlesioth_IdleStand(em, work);
        break;
    case PLES_IDLE:
        EmPlesioth_Idle(em, work);
        break;
    case PLES_IDLE_ACTION:
        EmPlesioth_IdleAction(em, work);
        break;
    case PLES_IDLE_LAND_FLOP_A:
        EmPlesioth_LandFlop(em, work, 0);
        break;
    case PLES_IDLE_STATUS2_COLLAPSE:
        EmPlesioth_StatusCollapse(em, (s32) work, 0);
        break;
    case PLES_IDLE_STATUS2_END:
        EmPlesioth_StatusGetUp(em);
        break;
    case PLES_IDLE_SHAKE_OFF:
        EmPlesioth_ShakeOff(em);
        break;
    case PLES_IDLE_STATUS2_TRAPPED:
        EmPlesioth_Status2Trapped(em, work);
        break;
    case PLES_IDLE_LAND_FLOP_B:
        EmPlesioth_LandFlop(em, work, 1);
        break;
    case PLES_IDLE_STATUS1_TRAPPED:
        EmPlesioth_Status1Trapped(em, work);
        break;
    case PLES_IDLE_STATUS1_COLLAPSE:
        EmPlesioth_Status1Collapse(em);
        break;
    case PLES_IDLE_STATUS1_END:
        EmPlesioth_StatusGetUp(em);
        break;
    case PLES_IDLE_STATUS2_COLLAPSE_BEACHED:
        EmPlesioth_Status2CollapseBeached(em);
        break;
    case PLES_IDLE_STATUS2_END_BEACHED:
        EmPlesioth_StatusGetUpBeached(em);
        break;
    case PLES_IDLE_STATUS1_COLLAPSE_BEACHED:
        EmPlesioth_Status1CollapseBeached(em);
        break;
    case PLES_IDLE_STATUS1_END_BEACHED:
        EmPlesioth_StatusGetUpBeached(em);
        break;
    case PLES_IDLE_REST:
        EmPlesioth_Rest(em);
        break;
    case PLES_IDLE_REST_END:
        EmPlesioth_RestEnd(em);
        break;
    case PLES_IDLE_STATUS4_SHAKE_OFF:
        EmPlesioth_ShakeOff(em);
        break;
    case PLES_IDLE_HOLD2_SHAKE_OFF:
        EmPlesioth_ShakeOff(em);
        break;
    case PLES_IDLE_STUN_SHAKE_OFF:
        EmPlesioth_ShakeOff(em);
        break;
    case PLES_IDLE_STATUS1_COLLAPSE_B:
        EmPlesioth_StatusCollapse(em, (s32) work, 1);
        break;
    case PLES_IDLE_STATUS1_END_B:
        EmPlesioth_StatusGetUp(em);
        break;
    }
}

/*
 * Runs the current group-1 state (EM_GROUP_MOVE): states 0-3, one function
 * each.
 */
void EmPlesioth_RunMoveState(Em *em, EmWork *work)
{
    switch (em->state) {
    case PLES_MOVE_WALK:
        EmPlesioth_Walk(em, work);
        break;
    case PLES_MOVE_RUN:
        EmPlesioth_RunAndStop(em, work);
        break;
    case PLES_MOVE_TURN:
        EmPlesioth_TurnToTarget(em, work);
        break;
    case PLES_MOVE_LIMP:
        EmPlesioth_Limp(em, work);
        break;
    }
}

/*
 * Runs the current group-2 state (EM_GROUP_FLY: for this family swimming
 * under water). States 16 and 26 share State2_10 (variant 0, 1), 24 and 25
 * State2_18, 27 runs State2_00; State2_02 takes no arguments.
 */
void EmPlesioth_RunSwimState(Em *em, EmWork *work)
{
    switch (em->state) {
    case PLES_SWIM_IDLE:
        EmPlesioth_SwimIdle(em);
        break;
    case PLES_SWIM_IDLE_AREA:
        EmPlesioth_SwimIdleApplyArea(em);
        break;
    case PLES_SWIM_NONE:
        EmPlesioth_SwimNone();
        break;
    case PLES_SWIM_TURN:
        EmPlesioth_SwimTurn(em);
        break;
    case PLES_SWIM_FORWARD:
        EmPlesioth_SwimForward(em, work);
        break;
    case PLES_SWIM_DASH:
        EmPlesioth_SwimDash(em, work);
        break;
    case PLES_SWIM_CURVE:
        EmPlesioth_SwimCurve(em);
        break;
    case PLES_SWIM_RISE:
        EmPlesioth_SwimRise(em, work);
        break;
    case PLES_SWIM_DIVE:
        EmPlesioth_SwimDive(em, work);
        break;
    case PLES_SWIM_ROUND_TURN:
        EmPlesioth_SwimRoundTurn(em);
        break;
    case PLES_SWIM_ROUND_NEXT:
        EmPlesioth_SwimRoundNext(em, work);
        break;
    case PLES_SWIM_ROUND_PREV:
        EmPlesioth_SwimRoundPrev(em, work);
        break;
    case PLES_SWIM_CIRCLE_RIGHT:
        EmPlesioth_SwimCircleRight(em, work);
        break;
    case PLES_SWIM_CIRCLE_LEFT:
        EmPlesioth_SwimCircleLeft(em, work);
        break;
    case PLES_SWIM_LEAP:
        EmPlesioth_LeapFromWater(em, work);
        break;
    case PLES_SWIM_SURFACE_FLOP:
        EmPlesioth_SurfaceFlop(em);
        break;
    case PLES_SWIM_LEAP_IN:
        EmPlesioth_LeapIntoWater(em, work, 0);
        break;
    case PLES_SWIM_BAIT_WAIT:
        EmPlesioth_WaitForBait(em, work);
        break;
    case PLES_SWIM_HOOKED:
        EmPlesioth_HookedPulled(em, work);
        break;
    case PLES_SWIM_STATUS2_COLLAPSE:
        EmPlesioth_SwimStatus2Collapse(em, work);
        break;
    case PLES_SWIM_STATUS1_COLLAPSE:
        EmPlesioth_SwimStatus1Collapse(em, work);
        break;
    case PLES_SWIM_STATUS4_RECOVER:
        EmPlesioth_SwimStatus4Recover(em);
        break;
    case PLES_SWIM_TO_IDLE:
        EmPlesioth_SwimToIdle(em);
        break;
    case PLES_SWIM_TRAP_ESCAPE:
        EmPlesioth_TrapEscape(em);
        break;
    case PLES_SWIM_STATUS2_END:
        EmPlesioth_SwimStatusRecover(em);
        break;
    case PLES_SWIM_STATUS1_END:
        EmPlesioth_SwimStatusRecover(em);
        break;
    case PLES_SWIM_LEAP_IN_TO_GOAL:
        EmPlesioth_LeapIntoWater(em, work, 1);
        break;
    case PLES_SWIM_IDLE_B:
        EmPlesioth_SwimIdle(em);
        
    default:
        break;
    }
}

/*
 * Runs the current group-3 state (EM_GROUP_ATTACK): states 0-7, one
 * function each.
 */
void EmPlesioth_RunAttackState(Em *em, EmWork *work)
{
    switch (em->state) {
    case PLES_ATK_LAND_A:
        EmPlesioth_LandAttackA(em, work);
        break;
    case PLES_ATK_BREATH:
        EmPlesioth_Breath(em, work);
        break;
    case PLES_ATK_SPIN:
        EmPlesioth_SpinAttack(em, work);
        break;
    case PLES_ATK_SURFACE_BREATH_A:
        EmPlesioth_SurfaceBreathA(em, work);
        break;
    case PLES_ATK_SURFACE_BREATH_B:
        EmPlesioth_SurfaceBreathB(em, work);
        break;
    case PLES_ATK_SURFACE_BREATH_C:
        EmPlesioth_SurfaceBreathC(em, work);
        break;
    case PLES_ATK_LEAP:
        EmPlesioth_LeapOutOfWater(em, work);
        break;
    case PLES_ATK_LAND_B:
        EmPlesioth_LandAttackB(em, work);
        
    default:
        break;
    }
}

/*
 * Runs the current group-4 state (EM_GROUP_REACT). States 23, 5, 26 and 25
 * share State4_05 (variant 0, 1, 2, 3), 14 and 28 State4_0E (variant 0, 1),
 * 8 and 27 State4_08, 19 and 20 State4_13.
 */
void EmPlesioth_RunReactState(Em *em, EmWork *work)
{
    switch (em->state) {
    case PLES_REACT_FLINCH:
        EmPlesioth_Flinch(em);
        break;
    case PLES_REACT_FLINCH_B:
        EmPlesioth_FlinchB(em);
        break;
    case PLES_REACT_FLINCH_C:
        EmPlesioth_FlinchC(em);
        break;
    case PLES_REACT_FLINCH_D:
        EmPlesioth_FlinchD(em);
        break;
    case PLES_REACT_FLINCH_LYING:
        EmPlesioth_FlinchLying(em);
        break;
    case PLES_REACT_KNOCK_DOWN:
        EmPlesioth_KnockDown(em, work, 1);
        break;
    case PLES_REACT_FALL:
        EmPlesioth_FallFromAir(em, work);
        break;
    case PLES_REACT_STATUS4_LYING:
        EmPlesioth_Status4Lying(em);
        break;
    case PLES_REACT_STATUS4_HELD_END:
        EmPlesioth_HeldStatusStruggle(em);
        break;
    case PLES_REACT_FALL_BLIND:
        EmPlesioth_FallBlinded(em, work);
        break;
    case PLES_REACT_FLINCH_BLIND:
        EmPlesioth_FlinchBlinded(em);
        break;
    case PLES_REACT_STATUS4:
        EmPlesioth_Status4(em);
        break;
    case PLES_REACT_TRAPPED:
        EmPlesioth_Trapped(em);
        break;
    case PLES_REACT_TRAPPED_STRUGGLE:
        EmPlesioth_TrappedStruggle(em);
        break;
    case PLES_REACT_STATUS4_HELD:
        EmPlesioth_HeldStatus(em, (s32) work, 0);
        break;
    case PLES_REACT_PULLED_ASHORE:
        EmPlesioth_PulledAshore(em, work);
        break;
    case PLES_REACT_LAND:
        EmPlesioth_LandAfterFall(em);
        break;
    case PLES_REACT_THROWN_OUT:
        EmPlesioth_ThrownOutOfWater(em, work);
        break;
    case PLES_REACT_STATUS4_UNDER:
        EmPlesioth_Status4Under(em, work);
        break;
    case PLES_REACT_STATUS2_TRAPPED_END:
        EmPlesioth_WakeInTrap(em);
        break;
    case PLES_REACT_STATUS1_TRAPPED_END:
        EmPlesioth_WakeInTrap(em);
        break;
    case PLES_REACT_STATUS4_LYING_END:
        EmPlesioth_Status4LyingEnd(em);
        break;
    case PLES_REACT_STATUS1_UNDER:
        EmPlesioth_Status1Under(em, work);
        break;
    case PLES_REACT_KNOCK_DOWN_PART3:
        EmPlesioth_KnockDown(em, work, 0);
        break;
    case PLES_REACT_HOLD2:
        EmPlesioth_Hold2(em);
        break;
    case PLES_REACT_STUN_A:
        EmPlesioth_KnockDown(em, work, 3);
        break;
    case PLES_REACT_STUN_B:
        EmPlesioth_KnockDown(em, work, 2);
        break;
    case PLES_REACT_STUN_HELD_END:
        EmPlesioth_HeldStatusStruggle(em);
        break;
    case PLES_REACT_STUN_HELD:
        EmPlesioth_HeldStatus(em, (s32) work, 1);
        break;
    }
}

/*
 * Runs the current group-5 state (EM_GROUP_DEFEAT): states 0-3 (hunted,
 * hunted while held, hunted under water and falling, State5_03).
 */
void EmPlesioth_RunDefeatState(Em *em, EmWork *work)
{
    switch (em->state) {
    case PLES_DEFEAT_DIE:
        EmPlesioth_Die(em, work);
        break;
    case PLES_DEFEAT_DIE_HELD:
        EmPlesioth_DieHeld(em, work);
        break;
    case PLES_DEFEAT_DIE_UNDER:
        EmPlesioth_DieUnder(em, work);
        break;
    case PLES_DEFEAT_DIE_ASHORE:
        EmPlesioth_DieAshore(em, work);
        break;
    }
}

/*
 * Runs the current group-6 state (EM_GROUP_EVENT; RunState also uses it for
 * group 7): 0 and 2 captured, 1 the entrance.
 */
void EmPlesioth_RunEventState(Em *em, EmWork *work)
{
    switch (em->state) {
    case PLES_EVENT_CAPTURED:
        EmPlesioth_Captured(em, work);
        break;
    case PLES_EVENT_ENTRANCE:
        EmPlesioth_EntranceScene(em, work);
        break;
    case PLES_EVENT_CAPTURED_HOLD2:
        EmPlesioth_CapturedHold2(em, work);
        break;
    }
}

/*
 * The family's frame: a local monster in task phase 1 stays put while no
 * hunter is in its area (Em_HoldPosNoPlayers; on land except in idle state
 * 1, under water except in swim state 0); otherwise, in the hunters' area,
 * a monster on land over deep water starts swimming (swim state 0) and one
 * under water off deep water goes ashore (idle state 1). Then the combat
 * timer, the reaction Em_UpdateDamage's result picks, the pending script, the current state (twice when a state was requested),
 * and under water posY is kept between floorY and surfaceY.
 */
void EmPlesioth_Update(Em *em)
{
    u8 buf[4];
    u8 *w = &em->work;

    em->moveFrozen = 0;
    if (em->remote == 0 && em->taskPhase == 1) {
        if (em->stance == EM_STANCE_GROUND) {
            Em_HoldPosNoPlayers(em, 0, 1);
        } else if (em->stance == EM_STANCE_UNDER) {
            Em_HoldPosNoPlayers(em, 2, 0);
        }
    } else if (em->area == gScene.area) {
        switch (em->stance) {
        case EM_STANCE_GROUND:
            if (EmPlesioth_OverDeepWater(em, 1) == 1) {
                EmPlesioth_ChangeState(em, EM_GROUP_FLY, PLES_SWIM_IDLE, EM_SYNC_LOCAL);
            }
            break;
        case EM_STANCE_UNDER:
            if (EmPlesioth_OverDeepWater(em, 1) == 0) {
                EmPlesioth_ChangeState(em, EM_GROUP_IDLE, PLES_IDLE, EM_SYNC_LOCAL);
            }
            break;
        }
    }
    Em_UpdateCombatTimer(em);
    switch (Em_UpdateDamage(em, buf) & 0xFF) {
    case EM_DMG_NONE:
    case EM_DMG_FLASH:
    case EM_DMG_PART8_DOWN:
        break;
    case EM_DMG_KILLED:
    case EM_DMG_DEAD:
        if (em->stance != EM_STANCE_AIR) {
            if (em->stance == EM_STANCE_UNDER) {
                EmPlesioth_ChangeState(em, EM_GROUP_DEFEAT, PLES_DEFEAT_DIE_UNDER, EM_SYNC_OWNER);
            } else if ((s8) em->holdTimer != 0) {
                EmPlesioth_ChangeState(em, EM_GROUP_DEFEAT, PLES_DEFEAT_DIE_HELD, EM_SYNC_OWNER);
            } else {
                EmPlesioth_ChangeState(em, EM_GROUP_DEFEAT, PLES_DEFEAT_DIE, EM_SYNC_OWNER);
            }
        }
        break;
    case EM_DMG_ATTACK9:
    case EM_DMG_ATTACK6:
        if ((s8) em->holdTimer == 0) {
            Em_SetTrapTime(em, buf);
            Em_SetTrapTimeGoal(em);
            EmPlesioth_ChangeState(em, EM_GROUP_REACT, PLES_REACT_TRAPPED, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_STATUS5:
        if (em->stance == EM_STANCE_UNDER) {
            Em_EndStatus5(em);
        } else if ((s8) em->holdTimer != 0) {
            Em_SetStatus5Timer(em);
            EmPlesioth_ChangeState(em, EM_GROUP_REACT, PLES_REACT_STUN_HELD, EM_SYNC_OWNER);
        } else if ((em->stateGroup != EM_GROUP_REACT || em->state != PLES_REACT_STUN_A) && (em->stateGroup != EM_GROUP_REACT || em->state != PLES_REACT_STUN_B) && (em->stateGroup != EM_GROUP_REACT || em->state != PLES_REACT_FALL)) {
            Em_SetStatus5Timer(em);
            if ((u16) (em->hitAngle - em->rotY) > 0x8000) {
                EmPlesioth_ChangeState(em, EM_GROUP_REACT, PLES_REACT_STUN_A, EM_SYNC_OWNER);
            } else {
                EmPlesioth_ChangeState(em, EM_GROUP_REACT, PLES_REACT_STUN_B, EM_SYNC_OWNER);
            }
        }
        break;
    case EM_DMG_ATTACK13:
        Em_ResetKindTimer(em);
        EmPlesioth_ChangeState(em, EM_GROUP_REACT, PLES_REACT_HOLD2, EM_SYNC_OWNER);
        break;
    case EM_DMG_ATTACK5:
        if (em->stance == EM_STANCE_UNDER) {
            EmPlesioth_ChangeState(em, EM_GROUP_REACT, PLES_REACT_THROWN_OUT, EM_SYNC_OWNER);
            Em_CheckRageByHp(em, em->rageThreshold);
        }
        break;
    case EM_DMG_STATUS_IN_AIR:
        if (em->stateGroup != EM_GROUP_REACT || em->state != PLES_REACT_FALL) {
            EmPlesioth_ChangeState(em, EM_GROUP_REACT, PLES_REACT_FALL, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_STATUS4:
        if ((s8) em->holdTimer != 0) {
            Em_SetStatus4Timer(em);
            EmPlesioth_ChangeState(em, EM_GROUP_REACT, PLES_REACT_STATUS4_HELD, EM_SYNC_OWNER);
        } else if ((em->stateGroup != EM_GROUP_REACT || em->state != PLES_REACT_STATUS4) && (em->stateGroup != EM_GROUP_REACT || em->state != PLES_REACT_FALL) && (em->stateGroup != EM_GROUP_REACT || em->state != PLES_REACT_STATUS4_LYING) && em->stance == EM_STANCE_FAMILY) {
            Em_SetStatus4Timer(em);
            EmPlesioth_ChangeState(em, EM_GROUP_REACT, PLES_REACT_STATUS4_LYING, EM_SYNC_OWNER);
        } else if ((em->stateGroup != EM_GROUP_REACT || em->state != PLES_REACT_STATUS4) && (em->stateGroup != EM_GROUP_REACT || em->state != PLES_REACT_FALL) && (em->stateGroup != EM_GROUP_REACT || em->state != PLES_REACT_STATUS4_LYING) && em->stance == EM_STANCE_GROUND) {
            Em_SetStatus4Timer(em);
            EmPlesioth_ChangeState(em, EM_GROUP_REACT, PLES_REACT_STATUS4, EM_SYNC_OWNER);
        } else if ((em->stateGroup != EM_GROUP_REACT || em->state != PLES_REACT_PULLED_ASHORE) && (em->stateGroup != EM_GROUP_REACT || em->state != PLES_REACT_FALL) && (em->stateGroup != EM_GROUP_REACT || em->state != PLES_REACT_STATUS4_LYING) && em->stance == EM_STANCE_UNDER) {
            Em_SetStatus4Timer(em);
            EmPlesioth_ChangeState(em, EM_GROUP_REACT, PLES_REACT_STATUS4_UNDER, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_CAPTURED:
        if ((s8) em->holdTimer != 0) {
            Em_SetStatus1Timer(em);
            EmPlesioth_ChangeState(em, EM_GROUP_EVENT, PLES_EVENT_CAPTURED, EM_SYNC_OWNER);
        } else if ((s8) em->holdTimer2 != 0) {
            EmPlesioth_ChangeState(em, EM_GROUP_EVENT, PLES_EVENT_CAPTURED_HOLD2, EM_SYNC_OWNER);
        } else if ((em->stateGroup != EM_GROUP_REACT || em->state != PLES_REACT_STATUS1_UNDER) && (em->stateGroup != EM_GROUP_REACT || em->state != PLES_REACT_FALL) && em->stance == EM_STANCE_UNDER) {
            Em_SetStatus1Timer(em);
            EmPlesioth_ChangeState(em, EM_GROUP_REACT, PLES_REACT_STATUS1_UNDER, EM_SYNC_OWNER);
        } else if ((em->stateGroup != EM_GROUP_IDLE || em->state != PLES_IDLE_STATUS1_COLLAPSE_BEACHED) && (em->stateGroup != EM_GROUP_REACT || em->state != PLES_REACT_FALL) && em->stance == EM_STANCE_FAMILY) {
            Em_SetStatus1Timer(em);
            EmPlesioth_ChangeState(em, EM_GROUP_IDLE, PLES_IDLE_STATUS1_COLLAPSE_BEACHED, EM_SYNC_OWNER);
        } else if ((em->stateGroup != EM_GROUP_IDLE || em->state != PLES_IDLE_STATUS1_COLLAPSE) && (em->stateGroup != EM_GROUP_REACT || em->state != PLES_REACT_FALL) && em->stance == EM_STANCE_GROUND) {
            Em_SetStatus1Timer(em);
            EmPlesioth_ChangeState(em, EM_GROUP_IDLE, PLES_IDLE_STATUS1_COLLAPSE, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_STATUS2:
        if (em->trapHit == 6) {
            Em_SetStatus2Timer(em);
            EmPlesioth_ChangeState(em, EM_GROUP_IDLE, PLES_IDLE_STATUS2_TRAPPED, EM_SYNC_OWNER);
        } else if ((em->stateGroup != EM_GROUP_FLY || em->state != PLES_SWIM_STATUS2_COLLAPSE) && (em->stateGroup != EM_GROUP_REACT || em->state != PLES_REACT_FALL) && em->stance == EM_STANCE_UNDER) {
            Em_SetStatus2Timer(em);
            EmPlesioth_ChangeState(em, EM_GROUP_FLY, PLES_SWIM_STATUS2_COLLAPSE, EM_SYNC_OWNER);
        } else if ((em->stateGroup != EM_GROUP_IDLE || em->state != PLES_IDLE_STATUS2_COLLAPSE_BEACHED) && (em->stateGroup != EM_GROUP_REACT || em->state != PLES_REACT_FALL) && em->stance == EM_STANCE_FAMILY) {
            Em_SetStatus2Timer(em);
            EmPlesioth_ChangeState(em, EM_GROUP_IDLE, PLES_IDLE_STATUS2_COLLAPSE_BEACHED, EM_SYNC_OWNER);
        } else if ((em->stateGroup != EM_GROUP_IDLE || em->state != PLES_IDLE_STATUS2_COLLAPSE) && (em->stateGroup != EM_GROUP_REACT || em->state != PLES_REACT_FALL) && em->stance == EM_STANCE_GROUND) {
            Em_SetStatus2Timer(em);
            EmPlesioth_ChangeState(em, EM_GROUP_IDLE, PLES_IDLE_STATUS2_COLLAPSE, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_HIT_LYING:
        if (em->stateGroup == EM_GROUP_IDLE) {
            switch (em->state) {
            case PLES_IDLE_STATUS2_COLLAPSE:
                EmPlesioth_ChangeState(em, EM_GROUP_IDLE, PLES_IDLE_STATUS2_END, EM_SYNC_OWNER);
                em->scriptPending = 0;
                break;
            case PLES_IDLE_STATUS2_TRAPPED:
                EmPlesioth_ChangeState(em, EM_GROUP_REACT, PLES_REACT_STATUS2_TRAPPED_END, EM_SYNC_OWNER);
                em->scriptPending = 0;
                break;
            case PLES_IDLE_STATUS1_TRAPPED:
                EmPlesioth_ChangeState(em, EM_GROUP_REACT, PLES_REACT_STATUS1_TRAPPED_END, EM_SYNC_OWNER);
                em->scriptPending = 0;
                break;
            case PLES_IDLE_STATUS1_COLLAPSE:
                EmPlesioth_ChangeState(em, EM_GROUP_IDLE, PLES_IDLE_STATUS1_END, EM_SYNC_OWNER);
                em->scriptPending = 0;
                break;
            case PLES_IDLE_STATUS2_COLLAPSE_BEACHED:
                EmPlesioth_ChangeState(em, EM_GROUP_IDLE, PLES_IDLE_STATUS2_END_BEACHED, EM_SYNC_OWNER);
                em->scriptPending = 0;
                break;
            case PLES_IDLE_REST:
                EmPlesioth_ChangeState(em, EM_GROUP_IDLE, PLES_IDLE_REST_END, EM_SYNC_OWNER);
                em->scriptPending = 0;
                break;
            case PLES_IDLE_STATUS1_COLLAPSE_B:
                /* the game's own call into the Cephadrome's ChangeState, with the Cephadrome's state */
                EmCephadrome_ChangeState(em, EM_GROUP_IDLE, CEPH_IDLE_STATUS1_END, EM_SYNC_OWNER);
                em->scriptPending = 0;
                break;
            }
        } else if (em->stateGroup == EM_GROUP_FLY) {
            switch (em->state) {
            case PLES_SWIM_STATUS2_COLLAPSE:
                EmPlesioth_ChangeState(em, EM_GROUP_FLY, PLES_SWIM_STATUS2_END, EM_SYNC_OWNER);
                em->scriptPending = 0;
                break;
            case PLES_SWIM_STATUS1_COLLAPSE:
                EmPlesioth_ChangeState(em, EM_GROUP_FLY, PLES_SWIM_STATUS1_END, EM_SYNC_OWNER);
                em->scriptPending = 0;
                break;
            }
        } else if (em->stateGroup == EM_GROUP_REACT) {
            switch (em->state) {
            case PLES_REACT_STATUS1_UNDER:
                EmPlesioth_ChangeState(em, EM_GROUP_FLY, PLES_SWIM_STATUS1_END, EM_SYNC_OWNER);
                em->scriptPending = 0;
                break;
            }
        }
        break;
    case EM_DMG_ATTACK5_FLINCH:
        if (em->stance == EM_STANCE_UNDER) {
            EmPlesioth_ChangeState(em, EM_GROUP_REACT, PLES_REACT_THROWN_OUT, EM_SYNC_OWNER);
            Em_CheckRageByHp(em, em->rageThreshold);
            break;
        }
    case EM_DMG_PART_FLINCH:
        switch (em->stance) {
        case EM_STANCE_GROUND:
        case EM_STANCE_DEAD:
            switch (em->flinchPart) {
            case 0:
            case 7:
                EmPlesioth_ChangeState(em, EM_GROUP_REACT, PLES_REACT_FLINCH, EM_SYNC_OWNER);
                break;
            case 5:
            case 6:
                EmPlesioth_ChangeState(em, EM_GROUP_REACT, PLES_REACT_FLINCH_C, EM_SYNC_OWNER);
                break;
            case 1:
            case 2:
                EmPlesioth_ChangeState(em, EM_GROUP_REACT, PLES_REACT_FLINCH_D, EM_SYNC_OWNER);
                break;
            case 3:
            case 4:
            default:
                if (em->parts[em->flinchPart].state >= 2) {
                    if (em->flinchPart != 3) {
                        EmPlesioth_ChangeState(em, EM_GROUP_REACT, PLES_REACT_KNOCK_DOWN, EM_SYNC_OWNER);
                    } else {
                        EmPlesioth_ChangeState(em, EM_GROUP_REACT, PLES_REACT_KNOCK_DOWN_PART3, EM_SYNC_OWNER);
                    }
                } else {
                    EmPlesioth_ChangeState(em, EM_GROUP_REACT, PLES_REACT_FLINCH_B, EM_SYNC_OWNER);
                }
                break;
            }
            break;
        case EM_STANCE_FAMILY:
            EmPlesioth_ChangeState(em, EM_GROUP_REACT, PLES_REACT_FLINCH_LYING, EM_SYNC_OWNER);
            break;
        case EM_STANCE_AIR:
            EmPlesioth_ChangeState(em, EM_GROUP_REACT, PLES_REACT_FALL, EM_SYNC_OWNER);
            break;
        case EM_STANCE_UNDER:
            EmPlesioth_ChangeState(em, EM_GROUP_REACT, PLES_REACT_THROWN_OUT, EM_SYNC_OWNER);
            break;
        }
        break;
    case EM_DMG_HIT_MODE3:
        if (em->stance != EM_STANCE_AIR) {
            EmPlesioth_ChangeState(em, EM_GROUP_REACT, PLES_REACT_FLINCH, EM_SYNC_OWNER);
            em->scriptPending = 0;
        }
        break;
    case EM_DMG_HIT:
        if (em->stateGroup == EM_GROUP_FLY && em->state == PLES_SWIM_HOOKED) {
            em->scriptPending = 0;
        }
        break;
    }
    Em_UpdatePendingScript(em);
    Em_ClearHitReactMode(em);
    EmPlesioth_RunState(em, (EmWork *) w);
    if (em->stateRequest != 0) {
        EmPlesioth_RunState(em, (EmWork *) w);
        em->stateRequest = 0;
    }
    if (em->stance == EM_STANCE_UNDER) {
        if (em->posY > em->surfaceY) {
            em->posY = em->surfaceY;
        }
        if (em->posY < em->floorY) {
            em->posY = em->floorY;
        }
    }
}

/*
 * Runs the current state through its group's dispatcher (group 7 through
 * group 6's), keeping the group and state as the previous ones.
 */
void EmPlesioth_RunState(Em *em, EmWork *work)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->stateGroup) {
    case EM_GROUP_IDLE:
        EmPlesioth_RunIdleState(em, work);
        break;
    case EM_GROUP_MOVE:
        EmPlesioth_RunMoveState(em, work);
        break;
    case EM_GROUP_FLY:
        EmPlesioth_RunSwimState(em, work);
        break;
    case EM_GROUP_ATTACK:
        EmPlesioth_RunAttackState(em, work);
        break;
    case EM_GROUP_REACT:
        EmPlesioth_RunReactState(em, work);
        break;
    case EM_GROUP_DEFEAT:
        EmPlesioth_RunDefeatState(em, work);
        break;
    case EM_GROUP_EVENT:
        EmPlesioth_RunEventState(em, work);
        break;
    case EM_GROUP_EVENT2:
        EmPlesioth_RunEventState(em, work);
        
    default:
        break;
    }
}

/*
 * Requests camera shake `level` (CamShake_ReqAIf) when the motion passes
 * frame `frame` (as EmRathalos_CamShakeOnFrame).
 */
void EmPlesioth_CamShakeOnFrame(s32 em, s32 frame, s32 level)
{
    if (EmMotion_PassedFrameScaled((Em *) em, (f32) frame, 0U) != 0) {
        CamShake_ReqAIf((void *) em, level);
    }
}

/* MotionEffects' default case: does nothing. */
static void EmPlesioth_MotionEffectsDefault(Em *em)
{
}

/*
 * The effects of each motion: keeps the layer-0 motion id in
 * work->prevMotion, plays its sounds (EmPlesioth_MotionSounds), sets the
 * fin mode by motion (EmPlesioth_SetFinMode: 1 for the walks, land attacks
 * and flinches, 0 for the turn and most swim motions, 3 for resting, lying
 * and the falls) and on given frames creates the attack volumes
 * (PlesiothAttack_Create), the walk's camera shakes, ground clouds
 * (DustCloud_Create; extra spray while enraged), splashes
 * (PartFx_SpawnForEmScaled) and the lying puffs (Em_SpawnLyingPuffs).
 */
void EmPlesioth_MotionEffects(Em *em, EmWork *work)
{
    f32 puffOfsA[3];
    f32 puffOfsB[3];
    f32 puffOfsC[3];
    f32 puffOfsD[3];

    if (em->motionId[0] != work->prevMotion) {
        work->prevMotion = em->motionId[0];
    }
    EmPlesioth_MotionSounds(em, work);
    switch (work->prevMotion) {
    case EM_MOTION_ID(0, PLES_MOT_IDLE):
        EmPlesioth_SetFinMode(em, work, 3);
        break;
    case EM_MOTION_ID(0, PLES_MOT_WALK):
        EmPlesioth_CamShakeOnFrame((s32) em, 0x34, 1);
        EmPlesioth_CamShakeOnFrame((s32) em, 0x74, 1);
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            PlesiothAttack_Create(em, 2);
        }
        if (EmMotion_PassedFrameScaled(em, 50.0f, 0) != 0) {
            PlesiothAttack_Create(em, 3);
        }
        if (EmMotion_PassedFrameScaled(em, 120.0f, 0) != 0) {
            PlesiothAttack_Create(em, 4);
        }
        EmPlesioth_SetFinMode(em, work, 1);
        break;
    case EM_MOTION_ID(0, PLES_MOT_RUN):
        EmPlesioth_SetFinMode(em, work, 0);
        if (EmMotion_PassedFrameScaled(em, 22.0f, 0) != 0) {
            PlesiothAttack_Create(em, 0x13);
        }
        if (EmMotion_PassedFrameScaled(em, 16.0f, 0) || EmMotion_PassedFrameScaled(em, 56.0f, 0) || EmMotion_PassedFrameScaled(em, 102.0f, 0)) {
            PlesiothAttack_Create(em, 0x14);
        }
        if ((EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 78.0f, 0) != 0)) {
            PlesiothAttack_Create(em, 0x15);
        }
        if ((EmMotion_PassedFrameScaled(em, 18.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 66.0f, 0) != 0)) {
            PartFx_SpawnForEmScaled(em, 0x1A, 3, 4.0f);
        }
        if ((EmMotion_PassedFrameScaled(em, 42.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 90.0f, 0) != 0)) {
            PartFx_SpawnForEmScaled(em, 0x14, 3, 4.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, PLES_MOT_TURN_RIGHT):
        EmPlesioth_SetFinMode(em, work, 1);
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            PlesiothAttack_Create(em, 5);
        }
        if (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) {
            PlesiothAttack_Create(em, 6);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 2.0f, 78.0f) != 0) && !(gScene.frame & 3)) {
            DustCloud_Create(em, 0x1A, 1, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, PLES_MOT_TURN_LEFT):
        EmPlesioth_SetFinMode(em, work, 1);
        if (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) {
            PlesiothAttack_Create(em, 7);
        }
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            PlesiothAttack_Create(em, 8);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 2.0f, 78.0f) != 0) && !(gScene.frame & 3)) {
            DustCloud_Create(em, 0x1A, 0, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, PLES_MOT_LIMP):
        EmPlesioth_SetFinMode(em, work, 1);
        if (EmMotion_PassedFrameScaled(em, 42.0f, 0) != 0) {
            PlesiothAttack_Create(em, 0x11);
        }
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            PlesiothAttack_Create(em, 0x12);
        }
        if (EmMotion_PassedFrameScaled(em, 74.0f, 0) != 0) {
            PlesiothAttack_Create(em, 0x12);
            break;
        }
        break;
    case EM_MOTION_ID(0, 0xB):
        EmPlesioth_SetFinMode(em, work, 1);
        break;
    case EM_MOTION_ID(0, 0x12):
        EmPlesioth_SetFinMode(em, work, 1);
        if (EmMotion_PassedFrameScaled(em, 42.0f, 0) || EmMotion_PassedFrameScaled(em, 102.0f, 0) || EmMotion_PassedFrameScaled(em, 156.0f, 0)) {
            if (em->posY <= (1000.0f + em->floorY)) {
                DustCloud_Create(em, 1, 0, 1.0f);
            }
        }
        break;
    case EM_MOTION_ID(0, 0x13):
        EmPlesioth_SetFinMode(em, work, 1);
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            PlesiothAttack_Create(em, 0xD);
        }
        if (EmMotion_PassedFrameScaled(em, 16.0f, 0) != 0) {
            EmPlesioth_SpawnLandingDust(em);
            break;
        }
        break;
    case EM_MOTION_ID(0, PLES_MOT_IDLE_ACTION):
        EmPlesioth_SetFinMode(em, work, 3);
        break;
    case EM_MOTION_ID(0, PLES_MOT_REST):
        puffOfsA[0] = 0.0f;
        puffOfsA[1] = 10.0f;
        puffOfsA[2] = 140.0f;
        Em_SpawnLyingPuffs(em, 0x22, puffOfsA, 1.6f);
        break;
    case EM_MOTION_ID(0, PLES_MOT_REST_START):
    case EM_MOTION_ID(0, PLES_MOT_REST_END):
        break;
    case EM_MOTION_ID(0, PLES_MOT_STATUS_LIE):
        EmPlesioth_SetFinMode(em, work, 3);
        puffOfsB[0] = 0.0f;
        puffOfsB[1] = 10.0f;
        puffOfsB[2] = 140.0f;
        Em_SpawnLyingPuffs(em, 0x22, puffOfsB, 1.6f);
        break;
    case EM_MOTION_ID(0, PLES_MOT_LAND_ATTACK_A):
        EmPlesioth_SetFinMode(em, work, 3);
        if (EmMotion_PassedFrameScaled(em, 50.0f, 0) != 0) {
            PlesiothAttack_Create(em, 1);
            break;
        }
        break;
    case EM_MOTION_ID(0, PLES_MOT_STATUS_GET_UP):
        EmPlesioth_SetFinMode(em, work, 3);
        break;
    case EM_MOTION_ID(0, PLES_MOT_STATUS_COLLAPSE):
        EmPlesioth_SetFinMode(em, work, 3);
        if (EmMotion_PassedFrameScaled(em, 148.0f, 0) != 0) {
            PlesiothAttack_Create(em, 0xF);
        }
        if (EmMotion_PassedFrameScaled(em, 170.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 2, 6, 4.0f);
        }
        puffOfsC[0] = 0.0f;
        puffOfsC[1] = 10.0f;
        puffOfsC[2] = 140.0f;
        Em_SpawnLyingPuffs(em, 0x22, puffOfsC, 1.6f);
        if (EmMotion_PassedFrameScaled(em, 170.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 2, 6, 4.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, PLES_MOT_SPIN):
        EmPlesioth_SetFinMode(em, work, 1);
        if (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) {
            PlesiothAttack_Create(em, 0x10);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 6.0f, 38.0f) != 0) && !(gScene.frame & 3)) {
            DustCloud_Create(em, 0x1A, 1, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, PLES_MOT_BREATH):
        EmPlesioth_SetFinMode(em, work, 1);
        if (EmMotion_PassedFrameScaled(em, 66.0f, 0) != 0) {
            PlesiothAttack_Create(em, 0x19);
            break;
        }
        break;
    case EM_MOTION_ID(0, 0x30):
        break;
    case EM_MOTION_ID(0, PLES_MOT_FLINCH):
        EmPlesioth_SetFinMode(em, work, 1);
        if (EmMotion_PassedFrameScaled(em, 44.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0x1B, 7, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 62.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0x16, 7, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 80.0f, 0) != 0) {
            DustCloud_Create(em, 1, 0x80, 0.5f);
            break;
        }
        break;
    case EM_MOTION_ID(0, PLES_MOT_FLINCH_BLIND):
        if (EmMotion_PassedFrameScaled(em, 24.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0x1A, 7, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, PLES_MOT_SHAKE_OFF):
        EmPlesioth_SetFinMode(em, work, 3);
        break;
    case EM_MOTION_ID(0, PLES_MOT_FLINCH_C):
        EmPlesioth_SetFinMode(em, work, 1);
        break;
    case EM_MOTION_ID(0, PLES_MOT_FLINCH_D):
        EmPlesioth_SetFinMode(em, work, 1);
        break;
    case EM_MOTION_ID(0, PLES_MOT_FLINCH_B):
        EmPlesioth_SetFinMode(em, work, 1);
        break;
    case EM_MOTION_ID(0, 0x43):
        EmPlesioth_SetFinMode(em, work, 1);
        break;
    case EM_MOTION_ID(0, PLES_MOT_DIE):
        EmPlesioth_SetFinMode(em, work, 0);
        if (EmMotion_PassedFrameScaled(em, 50.0f, 0) != 0) {
            DustCloud_Create(em, 0, 0, 0.8f);
            break;
        }
        break;
    case EM_MOTION_ID(0, PLES_MOT_FALL_A):
        EmPlesioth_SetFinMode(em, work, 1);
        if (EmMotion_PassedFrameScaled(em, 26.0f, 0) != 0) {
            DustCloud_Create(em, 0x12, 0x80, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, PLES_MOT_LIE_A):
        EmPlesioth_SetFinMode(em, work, 3);
        if ((EmMotion_PassedFrameScaled(em, 16.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 66.0f, 0) != 0)) {
            PartFx_SpawnForEmScaled(em, 2, 6, 4.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, PLES_MOT_GET_UP_A):
        EmPlesioth_SetFinMode(em, work, 3);
        break;
    case EM_MOTION_ID(0, PLES_MOT_FALL_B):
        EmPlesioth_SetFinMode(em, work, 1);
        if (EmMotion_PassedFrameScaled(em, 26.0f, 0) != 0) {
            DustCloud_Create(em, 0x12, 0x81, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, PLES_MOT_LIE_DOWN):
        EmPlesioth_SetFinMode(em, work, 3);
        if ((EmMotion_PassedFrameScaled(em, 16.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 66.0f, 0) != 0)) {
            PartFx_SpawnForEmScaled(em, 2, 6, 4.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, PLES_MOT_STAND_STILL):
        EmPlesioth_SetFinMode(em, work, 3);
        break;
    case EM_MOTION_ID(0, PLES_MOT_TRAPPED):
        EmPlesioth_SetFinMode(em, work, 1);
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 4.0f, 60.0f) != 0) && !(gScene.frame & 3)) {
            DustCloud_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, PLES_MOT_TRAP_STRUGGLE):
        EmPlesioth_SetFinMode(em, work, 1);
        if (em->rage != 0 && (EmMotion_InFrameRange(em, 0, 12.0f, 24.0f) || EmMotion_InFrameRange(em, 0, 46.0f, 90.0f)) && !(gScene.frame & 3)) {
            DustCloud_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
        }
        break;
    case EM_MOTION_ID(0, PLES_MOT_TRAPPED_LIE):
    case EM_MOTION_ID(0, PLES_MOT_WAKE_IN_TRAP):
        break;
    case EM_MOTION_ID(0, PLES_MOT_SWIM_START):
        EmPlesioth_SetFinMode(em, work, 0);
        break;
    case EM_MOTION_ID(0, PLES_MOT_SWIM):
        EmPlesioth_SetFinMode(em, work, 0);
        break;
    case EM_MOTION_ID(0, PLES_MOT_SWIM_FAST):
        EmPlesioth_SetFinMode(em, work, 0);
        break;
    case EM_MOTION_ID(0, PLES_MOT_SWIM_RISE):
        EmPlesioth_SetFinMode(em, work, 0);
        break;
    case EM_MOTION_ID(0, PLES_MOT_SWIM_DIVE):
        EmPlesioth_SetFinMode(em, work, 0);
        break;
    case EM_MOTION_ID(0, PLES_MOT_SWIM_TURN_LEFT):
        EmPlesioth_SetFinMode(em, work, 0);
        break;
    case EM_MOTION_ID(0, PLES_MOT_SWIM_TURN_RIGHT):
        EmPlesioth_SetFinMode(em, work, 0);
        break;
    case EM_MOTION_ID(0, PLES_MOT_LEAP_ATTACK):
        EmPlesioth_SetFinMode(em, work, 0);
        if (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0) {
            PlesiothAttack_Create(em, 0xE);
            break;
        }
        break;
    case EM_MOTION_ID(0, PLES_MOT_DEAD_FLOAT):
        EmPlesioth_SetFinMode(em, work, 0);
        break;
    case EM_MOTION_ID(0, PLES_MOT_SURFACE_BREATH_C):
        EmPlesioth_SetFinMode(em, work, 0);
        break;
    case EM_MOTION_ID(0, 0x71):
        EmPlesioth_SetFinMode(em, work, 0);
        break;
    case EM_MOTION_ID(0, PLES_MOT_SWIM_CIRCLE_RIGHT):
        EmPlesioth_SetFinMode(em, work, 0);
        break;
    case EM_MOTION_ID(0, PLES_MOT_SWIM_CIRCLE_LEFT):
        EmPlesioth_SetFinMode(em, work, 0);
        break;
    case EM_MOTION_ID(0, PLES_MOT_SURFACE_BREATH_A):
        EmPlesioth_SetFinMode(em, work, 0);
        break;
    case EM_MOTION_ID(0, PLES_MOT_SURFACE_BREATH_B):
        EmPlesioth_SetFinMode(em, work, 0);
        break;
    case EM_MOTION_ID(0, 0x76):
        EmPlesioth_SetFinMode(em, work, 0);
        break;
    case EM_MOTION_ID(0, PLES_MOT_LEAP):
        EmPlesioth_SetFinMode(em, work, 0);
        if (EmMotion_PassedFrameScaled(em, 50.0f, 0) != 0) {
            PlesiothAttack_Create(em, 0x18);
            break;
        }
        break;
    case EM_MOTION_ID(0, PLES_MOT_LEAP_CROUCH):
        EmPlesioth_SetFinMode(em, work, 0);
        break;
    case EM_MOTION_ID(0, PLES_MOT_FLOP_A):
        EmPlesioth_SetFinMode(em, work, 0);
        if (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0) {
            DustCloud_Create(em, 0, 0x89, 0.7f);
            break;
        }
        break;
    case EM_MOTION_ID(0, PLES_MOT_BEACHED):
        EmPlesioth_SetFinMode(em, work, 0);
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            PlesiothAttack_Create(em, 0x17);
        }
        if ((EmMotion_PassedFrameScaled(em, 24.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 42.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 62.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 82.0f, 0) != 0)) {
            PartFx_SpawnForEmScaled(em, 6, 3, 8.0f + (0.0005f * (f32) (Rand_Next(1) & 0x3FF)));
        }
        if ((EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 32.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 52.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 74.0f, 0) != 0)) {
            PartFx_SpawnForEmScaled(em, 0xC, 3, 8.0f + (0.0005f * (f32) (Rand_Next(1) & 0x3FF)));
            break;
        }
        break;
    case EM_MOTION_ID(0, PLES_MOT_GET_UP):
        EmPlesioth_SetFinMode(em, work, 0);
        break;
    case EM_MOTION_ID(0, PLES_MOT_TUMBLE):
        EmPlesioth_SetFinMode(em, work, 3);
        break;
    case EM_MOTION_ID(0, PLES_MOT_DIE_LYING):
        EmPlesioth_SetFinMode(em, work, 0);
        if (EmMotion_PassedFrameScaled(em, 50.0f, 0) != 0) {
            DustCloud_Create(em, 0, 0, 0.8f);
            break;
        }
        break;
    case EM_MOTION_ID(0, PLES_MOT_LIE_FLAP):
        EmPlesioth_SetFinMode(em, work, 3);
        if (EmMotion_PassedFrameScaled(em, 42.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 2, 6, 4.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, PLES_MOT_LIE_FLAP_B):
        EmPlesioth_SetFinMode(em, work, 3);
        if (EmMotion_PassedFrameScaled(em, 42.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 2, 6, 4.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, PLES_MOT_SWIM_STATUS_FLOAT):
        EmPlesioth_SetFinMode(em, work, 0);
        break;
    case EM_MOTION_ID(0, PLES_MOT_SWIM_RECOVER):
        EmPlesioth_SetFinMode(em, work, 0);
        break;
    case EM_MOTION_ID(0, PLES_MOT_HOOKED):
        EmPlesioth_SetFinMode(em, work, 0);
        break;
    case EM_MOTION_ID(0, PLES_MOT_RUN_STOP):
        EmPlesioth_SetFinMode(em, work, 0);
        if ((EmMotion_PassedFrameScaled(em, 6.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0)) {
            PartFx_SpawnForEmScaled(em, 2, 3, 7.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, PLES_MOT_SWIM_SURFACED):
        EmPlesioth_SetFinMode(em, work, 0);
        break;
    case EM_MOTION_ID(0, PLES_MOT_SWIM_BOTTOMED):
        EmPlesioth_SetFinMode(em, work, 0);
        break;
    case EM_MOTION_ID(0, PLES_MOT_LAND_ATTACK_B):
        EmPlesioth_SetFinMode(em, work, 0);
        if (EmMotion_PassedFrameScaled(em, 54.0f, 0) != 0) {
            PlesiothAttack_Create(em, 0x16);
            break;
        }
        break;
    case EM_MOTION_ID(0, PLES_MOT_SIDE_LIE):
        puffOfsD[0] = 0.0f;
        puffOfsD[1] = 10.0f;
        puffOfsD[2] = 140.0f;
        Em_SpawnLyingPuffs(em, 0x22, puffOfsD, 1.6f);
        break;
    case EM_MOTION_ID(0, PLES_MOT_STATUS4_LYING):
        break;
    default:
        EmPlesioth_MotionEffectsDefault(em);
        break;
    }
}

/*
 * Fin i's fold (finRot[i].fold, a turn about x): in fin modes 2 and 3 folds
 * it to EmPlesioth_FinFoldTarget[i] over 20 frames, in mode 1 opens it back
 * to 0 over 20 frames; mode 0 leaves it as it is.
 */
static void EmPlesioth_UpdateFinFold(Em *em, EmWork *work, s32 i)
{
    EmPlesiothWork *fins = (EmPlesiothWork *) work;
    u8 *foldState;

    /* the cast (a no-op) gives the game's operand order (&p->finState[i][1] swaps an addu) */
    switch (*(foldState = &((u8 (*)[2]) fins->finState)[i][1])) {
    case 0:
        if (fins->finMode == 2 || fins->finMode == 3) {
            if (fins->finRot[i].fold != EmPlesioth_FinFoldTarget[i]) {
                *foldState = 2;
                fins->finCount[i] = 20;
            }
        } else if (fins->finMode == 1) {
            if (fins->finRot[i].fold != 0) {
                *foldState = 1;
                fins->finCount[i] = 20;
            }
        }
        break;
    case 1:
        if (--fins->finCount[i] <= 0) {
            fins->finRot[i].fold = 0;
            *foldState = 0;
        } else {
            fins->finRot[i].fold += (u16) ((0x10000 - fins->finRot[i].fold) / fins->finCount[i]);
        }
        break;
    case 2:
        if (--fins->finCount[i] <= 0) {
            fins->finRot[i].fold = EmPlesioth_FinFoldTarget[i];
            *foldState = 0;
        } else {
            fins->finRot[i].fold -= (u16) ((u16) (0x10000 - (EmPlesioth_FinFoldTarget[i] - fins->finRot[i].fold)) / fins->finCount[i]);
        }
        break;
    }
}

typedef struct EmFinKey {
    u16 f;
    u16 v;
} EmFinKey;

extern EmFinKey *EmPlesioth_FinKeys2[];
extern EmFinKey *EmPlesioth_FinKeys1[];
extern s16 EmPlesioth_FinWait1[];
extern s16 EmPlesioth_FinWait2[];
extern s16 EmPlesioth_FinRewait1[];
extern s16 EmPlesioth_FinRewait2[];

/*
 * Returns the index of the first fin key whose frame is at least f; when it
 * reaches the table's end (a key with frame 0 after the first) it returns
 * that key's index with bit 0x80 set.
 */
static inline u8 EmPlesioth_FinFindKey(EmFinKey *keys, u16 frame)
{
    s32 idx = 0;

    for (;;) {
        if (!(idx != 0) == 0 && keys->f == 0) {
            idx = (u8) (idx | 0x80);
            break;
        }
        if (keys->f < frame) {
            idx = (u8) (idx + 1);
            keys++;
        } else {
            break;
        }
    }
    return idx;
}

#define FIN_S16(base, j) (*(s16 *) ((u8 *) (base) + (j)))

/* j: the first argument (the caller passes em) reused as fin i's byte
   offset into the s16 arrays; only this gives the game's registers. */
static void EmPlesioth_UpdateFinWave(s32 j, EmWork *work, s32 i)
{
    EmPlesiothWork *fins = (EmPlesiothWork *) work;
    s16 keyFrame;
    EmFinKey *keys;
    u8 keyIdx;
    u8 *waveState;

    j = i * 2;
    switch (*(waveState = (u8 *) fins->finState + j)) {
    case 0:
        if (fins->finMode == 2) {
            (*waveState)++;
            FIN_S16(fins->finWait, j) = FIN_S16(EmPlesioth_FinWait2, j);
        } else if (fins->finMode == 1) {
            (*waveState)++;
            FIN_S16(fins->finWait, j) = FIN_S16(EmPlesioth_FinWait1, j);
        }
        break;
    case 1:
        if (--FIN_S16(fins->finWait, j) <= 0) {
            if (fins->finMode == 2) {
                *waveState = 3;
            } else {
                *waveState = 2;
            }
            FIN_S16(fins->finWait, j) = 0;
            FIN_S16(fins->finFrame, j) = 0;
        }
        break;
    case 2:
        keyFrame = FIN_S16(fins->finFrame, j)++;
        keys = EmPlesioth_FinKeys2[i];
        keyIdx = EmPlesioth_FinFindKey(keys, keyFrame);
        if (keyIdx & 0x80) {
            if (fins->finMode == 2) {
                *waveState = 1;
                FIN_S16(fins->finWait, j) = FIN_S16(EmPlesioth_FinRewait2, j);
            } else if (fins->finMode == 1) {
                *waveState = 1;
                FIN_S16(fins->finWait, j) = FIN_S16(EmPlesioth_FinRewait1, j);
            } else {
                *waveState = 0;
            }
            fins->finRot[i].wave = keys[keyIdx & 0x7F].v;
        } else {
            fins->finRot[i].wave += keys[keyIdx].v;
        }
        break;
    case 3:
        keyFrame = FIN_S16(fins->finFrame, j)++;
        keys = EmPlesioth_FinKeys1[i];
        keyIdx = EmPlesioth_FinFindKey(keys, keyFrame);
        if (keyIdx & 0x80) {
            if (fins->finMode == 2) {
                *waveState = 1;
                FIN_S16(fins->finWait, j) = FIN_S16(EmPlesioth_FinRewait2, j);
            } else if (fins->finMode == 1) {
                *waveState = 1;
                FIN_S16(fins->finWait, j) = FIN_S16(EmPlesioth_FinRewait1, j);
            } else {
                *waveState = 0;
            }
            fins->finRot[i].wave = keys[keyIdx & 0x7F].v;
        } else {
            fins->finRot[i].wave += keys[keyIdx].v;
        }
        break;
    }
}

/*
 * Runs the wave and the fold of the four fins (bones 37, 38, 39, 16) for
 * Em_AdjustKindBones.
 */
void EmPlesioth_UpdateFins(Em *em, EmWork *work)
{
    s32 i;

    for (i = 0; i < 4; i++) {
        EmPlesioth_UpdateFinWave((s32) em, work, i);
        EmPlesioth_UpdateFinFold(em, work, i);
    }
}

/*
 * Runs EmPlesioth_MotionEffects and EmPlesioth_UpdateFins once the work
 * area's fxStep is set (the first frame only sets it).
 */
void EmPlesioth_UpdateMotionEffects(Em *em)
{
    EmPlesiothWork *work = (EmPlesiothWork *)&em->work;

    switch (work->fxStep) {
    case 0:
        work->fxStep++;
        break;
    case 1:
        EmPlesioth_MotionEffects(em, (EmWork *) work);
        EmPlesioth_UpdateFins(em, (EmWork *) work);
        break;
    }
}

/*
 * Sets the fin mode: 0 still, 1 waving, 2 waving folded, 3 folded; mode 1
 * becomes 2 at a fifth of hpMax or less, mode 3 becomes 0 above it (the
 * fins fold only when weak).
 */
void EmPlesioth_SetFinMode(Em *em, EmWork *work, s32 mode)
{
    EmPlesiothWork *w = (EmPlesiothWork *)work;

    w->finMode = mode;
    if (w->finMode == 1) {
        if (em->hp <= (s16) (0.2f * em->hpMax)) {
            w->finMode = 2;
        }
    }
    if (w->finMode == 3) {
        if (em->hp > (s16) (0.2f * em->hpMax)) {
            w->finMode = 0;
        }
    }
}

/*
 * Creates the ground effect of kind 0x14 type 0xB (DustCloud_Create) on
 * frame 16 of motion 19 (as EmRathalos_SpawnLandingDust).
 */
void EmPlesioth_SpawnLandingDust(Em *em)
{
    DustCloud_Create((Em *) em, 0xB, 0, 1.0f);
}

/*
 * While the monster is in the hunters' area and within 100 below the water
 * surface, every 4th frame spawns Effect08 type 2 at the surface above it
 * with sound 0x1B; the swimming states call it every frame.
 */
void EmPlesioth_SpawnWakeEvery4(Em *em, f32 scale)
{
    f32 pos[3];
    f32 surfaceY;

    if ((gScene.area == em->area) && !(gScene.frame & 3)) {
        surfaceY = em->surfaceY;
        if (!(em->posY < (surfaceY - 100.0f))) {
            Vec_Set(pos, em->posX, 10.0f + (surfaceY + *(f32 *)&em->surfaceDepth), em->posZ);
            Effect08_Spawn(pos, 2, 0, scale);
            Em_PlaySeAtBone(em, 0x1B, 6);
        }
    }
}

/*
 * In the hunters' area spawns Effect08 type 5 at the water surface above
 * the monster with sound 0x1E: the splash where it breaks the surface
 * (leaping out, diving back in, pulled by a bait).
 */
void EmPlesioth_SpawnSurfaceSplash(Em *em, f32 scale)
{
    f32 pos[3];

    if (gScene.area == em->area) {
        Vec_Set(pos, em->posX, 10.0f + (em->surfaceY + *(f32 *)&em->surfaceDepth), em->posZ);
        Effect08_Spawn(pos, 5, 0, scale);
        Em_PlaySeAtBone(em, 0x1E, 6);
    }
}

/*
 * As SpawnWakeEvery4 with Effect08 type 4, every 8th frame and without a
 * sound; called beside it in the swim states.
 */
void EmPlesioth_SpawnWakeEvery8(Em *em, f32 scale)
{
    f32 pos[3];
    f32 surfaceY;

    if ((gScene.area == em->area) && !(gScene.frame & 7)) {
        surfaceY = em->surfaceY;
        if (!(em->posY < (surfaceY - 100.0f))) {
            Vec_Set(pos, em->posX, 10.0f + (surfaceY + *(f32 *)&em->surfaceDepth), em->posZ);
            Effect08_Spawn(pos, 4, 0, scale);
        }
    }
}

/* Slot 2 of EmPlesioth_Callbacks: does nothing. */
void EmPlesioth_TableNop(void)
{
}

/*
 * Enters idle state `state`: motion speed 1, begins or ends the status of
 * the lying states (status 2: begins 5, 8, 14, ends 6, 15; status 1: begins
 * 10, 12, 16, 41, ends 13, 17, 19, 42; ends status 4 in 20, status 5 in 22;
 * 21 clears holdTimer2), stance ground, then Em_SetStateSync.
 */
void EmPlesioth_SetIdleState(Em *em, u16 state, u8 mode)
{
    em->motionSpeed = 1.0f;
    switch (state) {
    case PLES_IDLE_STATUS2_COLLAPSE:
    case PLES_IDLE_STATUS2_TRAPPED:
    case PLES_IDLE_STATUS2_COLLAPSE_BEACHED:
        Em_BeginStatus2(em);
        break;
    case PLES_IDLE_STATUS1_TRAPPED:
    case PLES_IDLE_STATUS1_COLLAPSE:
    case PLES_IDLE_STATUS1_COLLAPSE_BEACHED:
        Em_BeginStatus1(em);
        break;
    case PLES_IDLE_STATUS2_END:
    case PLES_IDLE_STATUS2_END_BEACHED:
        Em_EndStatus2(em);
        break;
    case PLES_IDLE_STATUS1_END:
    case PLES_IDLE_STATUS1_END_BEACHED:
        Em_EndStatus1(em);
        break;
    case PLES_IDLE_REST_END:
        Em_EndStatus1(em);
        break;
    case PLES_IDLE_STATUS4_SHAKE_OFF:
        Em_EndStatus4(em);
        break;
    case PLES_IDLE_HOLD2_SHAKE_OFF:
        em->holdTimer2 = 0;
        break;
    case PLES_IDLE_STUN_SHAKE_OFF:
        Em_EndStatus5(em);
        break;
    case PLES_IDLE_STATUS1_COLLAPSE_B:
        Em_BeginStatus1(em);
        break;
    case PLES_IDLE_STATUS1_END_B:
        Em_EndStatus1(em);
        break;
    }
    em->stance = EM_STANCE_GROUND;
    Em_SetStateSync(em, EM_GROUP_IDLE, state, mode);
}

/*
 * Enters move state `state` on the ground: state 0 (the walk, speed set 0)
 * runs as state 3 (the weak walk, speed set 6) below a fifth of hpMax;
 * without a goal both get a timer of 90-149 frames (state 3 at most 300);
 * state 1 sets goalDist (the goal's distance less 300, or 1000 without a
 * goal).
 */
void EmPlesioth_SetMoveState(Em *em, u16 state, u8 mode)
{
    f32 dist;

    em->stance = EM_STANCE_GROUND;
    em->motionSpeed = 1.0f;
    switch (state) {
    case PLES_MOVE_WALK:
        /* low on hp it limps: the goto joins that case */
        if (em->hp < (s16) (0.2f * em->hpMax)) {
            state = PLES_MOVE_LIMP;
            goto state3;
        }
        Em_SetSpeedScaled(em, 0, 1000.0f, 500.0f);
        if (em->hasGoal == 0) {
            em->timer = (em->stateRand % 120 + 180) / 2;
        }
        break;
    case PLES_MOVE_RUN:
        if (em->curGoalKind == 0) {
            em->hasGoal = 0;
            em->goalDist = 1000.0f;
        } else {
            em->hasGoal = 1;
            dist = Vec_DistanceXZ(&em->posX, em->goal) - 300.0f;
            em->goalDist = dist;
            if (dist < 0.0f) {
                em->goalDist = 0.0f;
            }
        }
        break;
    case PLES_MOVE_TURN:
        break;
    case PLES_MOVE_LIMP:
    state3:
        Em_SetSpeedScaled(em, 6, em->goalDist, 0.0f);
        if (em->hasGoal == 0) {
            em->timer = (em->stateRand % 120 + 180) / 2;
            if (em->timer > 300) {
                em->timer = 300;
            }
        }
        break;
    }
    Em_SetStateSync(em, EM_GROUP_MOVE, state, mode);
}

/*
 * Enters swim state `state`: a local monster off deep water (OverDeepWater
 * 0) goes to idle state 1 instead (all but 16, 23, 26, 27); begins status 2
 * in 19, status 1 in 20, ends status 4 in 21, status 2 in 24, status 1 in
 * 25; state 6 runs as 8; 10 and 11 (circling the area's four points) set
 * circleLegs 4 and the nearest point as goal, run as 11 when the next point
 * is behind, as 9 when the goal is not within 45 degrees ahead.
 */
void EmPlesioth_SetSwimState(Em *em, u16 state, u8 mode)
{
    EmPlesiothWork *w = (EmPlesiothWork *)&em->work;
    f32 nextPoint[3];
    s32 relYaw;

    em->motionSpeed = 1.0f;
    switch (state) {
    case PLES_SWIM_IDLE:
    case PLES_SWIM_IDLE_AREA:
    case PLES_SWIM_NONE:
    case PLES_SWIM_TURN:
    case PLES_SWIM_RISE:
    case PLES_SWIM_DIVE:
    case PLES_SWIM_ROUND_TURN:
    case PLES_SWIM_CIRCLE_RIGHT:
    case PLES_SWIM_CIRCLE_LEFT:
    case PLES_SWIM_LEAP:
    case PLES_SWIM_SURFACE_FLOP:
    case PLES_SWIM_BAIT_WAIT:
    case PLES_SWIM_HOOKED:
    case PLES_SWIM_STATUS2_COLLAPSE:
    case PLES_SWIM_STATUS1_COLLAPSE:
    case PLES_SWIM_STATUS4_RECOVER:
    case PLES_SWIM_TO_IDLE:
    case PLES_SWIM_STATUS2_END:
    case PLES_SWIM_STATUS1_END:
        switch (state) {
        case PLES_SWIM_STATUS2_COLLAPSE:
            Em_BeginStatus2(em);
            break;
        case PLES_SWIM_STATUS1_COLLAPSE:
            Em_BeginStatus1(em);
            break;
        case PLES_SWIM_STATUS4_RECOVER:
            Em_EndStatus4(em);
            break;
        case PLES_SWIM_STATUS2_END:
            Em_EndStatus2(em);
            break;
        case PLES_SWIM_STATUS1_END:
            Em_EndStatus1(em);
            break;
        }
        if (em->remote == 0 && EmPlesioth_OverDeepWater(em, 0) == 0) {
            EmPlesioth_ChangeState(em, EM_GROUP_IDLE, PLES_IDLE, EM_SYNC_SEND);
            return;
        }
        break;
    case 4:
    case 5:
        if (em->remote == 0 && EmPlesioth_OverDeepWater(em, 0) == 0) {
            EmPlesioth_ChangeState(em, EM_GROUP_IDLE, PLES_IDLE, EM_SYNC_SEND);
            return;
        }
        Em_SetSpeedScaled(em, 0, 1000.0f, 500.0f);
        break;
    case 6:
        if (em->remote == 0 && EmPlesioth_OverDeepWater(em, 0) == 0) {
            EmPlesioth_ChangeState(em, EM_GROUP_IDLE, PLES_IDLE, EM_SYNC_SEND);
            return;
        }
        state = PLES_SWIM_DIVE;
        break;
    case 10:
    case 11:
        if (em->remote == 0 && EmPlesioth_OverDeepWater(em, 0) == 0) {
            EmPlesioth_ChangeState(em, EM_GROUP_IDLE, PLES_IDLE, EM_SYNC_SEND);
            return;
        }
        w->roundLegsLeft = 4;
        if (em->remote == 0) {
            em->goalNo = EmPlesioth_NextAreaPoint(em, nextPoint);
            em->goalKind = 2;
            em->goalArg = 2;
            Em_ComputeGoal(em, em->goal);
            relYaw = (u16) (Math_AngleXZ(&em->posX, em->goal) - em->rotY);
            if ((0 <= relYaw && relYaw <= 0x2000) || (relYaw >= 0xE000 && relYaw <= 0xFFFF)) {
                relYaw = (u16) (Math_AngleXZ(&em->posX, nextPoint) - em->rotY);
                if (relYaw >= 0x4000 && relYaw <= 0xC000 && state == PLES_SWIM_ROUND_NEXT) {
                    state = PLES_SWIM_ROUND_PREV;
                }
                em->timer = 1.5f * (Vec_DistanceXZ(&em->posX, em->goal) / 0.5f) / 50.0f;
            } else {
                state = PLES_SWIM_ROUND_TURN;
            }
        } else {
            Em_ComputeGoal2(em, em->goal);
        }
        break;
    case 16:
    case 23:
    case 26:
    case 27:
        break;
    }
    Em_SetStateSync(em, EM_GROUP_FLY, state, mode);
}

/*
 * Enters attack state `state`: the under-water attacks 3-6 of a local
 * monster off deep water go to idle state 1 instead; state 1 sets work byte
 * 0x13.
 */
void EmPlesioth_SetAttackState(Em *em, u16 state, u8 mode)
{
    EmPlesiothWork *w = (EmPlesiothWork *)&em->work;

    em->motionSpeed = 1.0f;
    switch (state) {
    case PLES_ATK_SURFACE_BREATH_A:
    case PLES_ATK_SURFACE_BREATH_B:
    case PLES_ATK_SURFACE_BREATH_C:
    case PLES_ATK_LEAP:
        if (em->remote == 0 && EmPlesioth_OverDeepWater(em, 0) == 0) {
            EmPlesioth_ChangeState(em, EM_GROUP_IDLE, PLES_IDLE, EM_SYNC_SEND);
            return;
        }
        break;
    case PLES_ATK_BREATH:
        w->unk013 = 1;
        break;
    case PLES_ATK_LAND_A:
    case PLES_ATK_SPIN:
    case PLES_ATK_LAND_B:
        break;
    }
    Em_SetStateSync(em, EM_GROUP_ATTACK, state, mode);
}

/*
 * Moves to (group, state): a local monster first targets a player in range
 * (a remote one ignores sync modes 3 and 4), loads reaction set 0, then the
 * group's Set function; inline for group 4 (statuses begun or ended; state
 * 15 aims at point 1 of table 1 and breaks part 2), 5 (state 4 makes the
 * monster appear again) and 6 (state 0 begins status 1).
 */
void EmPlesioth_ChangeState(Em *em, u16 group, u16 state, u8 mode)
{
    if (em->remote == 0) {
        Em_TargetPlayerInRange(em);
    } else if (mode == EM_SYNC_OWNER || mode == EM_SYNC_OWNER_FLAGGED) {
        return;
    }
    Em_LoadReactionSet(em, 0);
    switch (group) {
    case EM_GROUP_IDLE:
        EmPlesioth_SetIdleState(em, state, mode);
        break;
    case EM_GROUP_MOVE:
        EmPlesioth_SetMoveState(em, state, mode);
        break;
    case EM_GROUP_FLY:
        EmPlesioth_SetSwimState(em, state, mode);
        break;
    case EM_GROUP_ATTACK:
        EmPlesioth_SetAttackState(em, state, mode);
        break;
    case EM_GROUP_REACT:
        switch (state) {
        case PLES_REACT_STATUS4_LYING:
        case PLES_REACT_STATUS4:
        case PLES_REACT_STATUS4_HELD:
        case PLES_REACT_STATUS4_UNDER:
            Em_BeginStatus4(em);
            break;
        case PLES_REACT_STATUS4_HELD_END:
        case PLES_REACT_STATUS4_LYING_END:
            Em_EndStatus4(em);
            break;
        case PLES_REACT_TRAPPED:
        case PLES_REACT_HOLD2:
            Em_EndAllStatus(em);
            break;
        case PLES_REACT_STATUS2_TRAPPED_END:
            Em_EndStatus2(em);
            break;
        case PLES_REACT_STATUS1_TRAPPED_END:
            Em_EndStatus1(em);
            break;
        case PLES_REACT_STATUS1_UNDER:
            Em_BeginStatus1(em);
            break;
        case PLES_REACT_STUN_A:
        case PLES_REACT_STUN_B:
        case PLES_REACT_STUN_HELD:
            Em_BeginStatus5(em);
            break;
        case PLES_REACT_STUN_HELD_END:
            Em_EndStatus5(em);
            break;
        case PLES_REACT_PULLED_ASHORE:
            em->goalKind = 2;
            em->goalArg = 1;
            em->goalNo = 1;
            Em_ComputeGoal(em, em->goal);
            Em_BreakPart(em, 2);
            break;
        }
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        break;
    case EM_GROUP_DEFEAT:
        if (state == 4 && QuestEm_CanAppear(em) == 1) {
            QuestEm_Appear(em);
        }
        Em_SetStateSync(em, group, state, mode);
        break;
    case EM_GROUP_EVENT:
        switch (state) {
        case PLES_EVENT_CAPTURED:
            Em_BeginStatus1(em);
            break;
        }
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        break;
    }
}

/*
 * Starts curve set `kind` of the movement curves: kinds 0-2 the vertical
 * and forward speed curves from frame 0, kind 3 only the vertical one from
 * frame 0x26; clears the velocity.
 */
void EmPlesioth_StartCurves(Em *em, u8 kind)
{
    EmPlesiothWork *w = (EmPlesiothWork *)&em->work;

    w->curveNone = 0;
    w->curveVelY = 0;
    w->curveSpeed = 0;
    w->curveKind = kind;
    switch (w->curveKind) {
    case 0:
        w->curveVelY = 1;
        w->curveSpeed = 1;
        w->curveFrame = 0;
        break;
    case 1:
        w->curveVelY = 1;
        w->curveSpeed = 1;
        w->curveFrame = 0;
        break;
    case 2:
        w->curveVelY = 1;
        w->curveSpeed = 1;
        w->curveFrame = 0;
        break;
    case 3:
        w->curveVelY = 1;
        w->curveFrame = 0x26;
        break;
    }
    Player_ClearVelocity(em);
}

/* The curve step of a set without curves: does nothing and returns 0. */
static s32 EmPlesioth_CurveNone(Em *em, EmWork *work)
{
    return 0;
}

/*
 * The vertical-speed curve (EmPlesioth_VelYCurves[curveKind]): sets velY
 * from the key pair around curveFrame; at the table's end (a key with t 0)
 * eases velY down to -50 and returns 2, else 0.
 */
static u8 EmPlesioth_CurveVelY(Em *em, EmWork *work)
{
    EmCurveKey *key;
    f32 frame;
    f32 keyT;
    s32 i;
    EmCurveKey *tbl;
    u8 ret;

    tbl = EmPlesioth_VelYCurves[((EmPlesiothWork *)work)->curveKind];
    ret = 0;
    if (tbl == NULL) {
        return ret;
    }
    if (em->motion[0].blend != 0) {
        frame = 0.0f;
    } else {
        frame = ((EmPlesiothWork *)work)->curveFrame;
    }
    if (frame != 0.0f && frame != 1.0f) {
        i = 1;
        while (i != 0) {
            key = &tbl[i];
            keyT = key->t;
            if (frame > keyT && keyT != 0.0f) {
                i++;
            } else if (keyT == 0.0f) {
                ret = 2;
                if (em->velY > -50.0f) {
                    em->velY -= 1.0f;
                }
                break;
            } else {
                i = 0;
                em->velY = (key->v - key[-1].v) / ((keyT - key[-1].t) / em->motion[0].step);
            }
        }
    }
    return ret;
}

/*
 * The forward-speed curve (EmPlesioth_SpeedCurves[curveKind]): sets velZ
 * from the key pair around curveFrame; returns 4 at the table's end, else
 * 0.
 */
static u8 EmPlesioth_CurveSpeed(Em *em, EmWork *work)
{
    EmCurveKey *key;
    f32 frame;
    f32 keyT;
    s32 i;
    EmCurveKey *tbl;
    u8 ret;

    tbl = EmPlesioth_SpeedCurves[((EmPlesiothWork *)work)->curveKind];
    ret = 0;
    if (tbl == NULL) {
        return ret;
    }
    if (em->motion[0].blend != 0) {
        frame = 0.0f;
    } else {
        frame = ((EmPlesiothWork *)work)->curveFrame;
    }
    if (frame != 0.0f && frame != 1.0f) {
        i = 1;
        while (i != 0) {
            key = &tbl[i];
            keyT = key->t;
            if (frame > keyT && keyT != 0.0f) {
                i++;
            } else if (keyT == 0.0f) {
                ret = 4;
                break;
            } else {
                i = 0;
                em->velZ = (key->v - key[-1].v) / ((keyT - key[-1].t) / em->motion[0].step);
            }
        }
    }
    return ret;
}

/*
 * Runs the curves whose flags are set, advances curveFrame by the motion's
 * step, and in the air or under water moves the monster along its yaw
 * (Em_MoveRotatedAccel). Returns the curves' end bits (2 vertical, 4
 * forward).
 */
u8 EmPlesioth_UpdateCurves(Em *em)
{
    EmPlesiothWork *w = (EmPlesiothWork *)&em->work;
    u8 ret = 0;

    if (w->curveNone != 0) {
        ret = EmPlesioth_CurveNone(em, (EmWork *) w);
    }
    if (w->curveVelY != 0) {
        ret |= EmPlesioth_CurveVelY(em, (EmWork *) w);
    }
    if (w->curveSpeed != 0) {
        ret |= EmPlesioth_CurveSpeed(em, (EmWork *) w);
    }
    w->curveFrame += (s16) em->motion[0].step;
    if (em->stance == EM_STANCE_AIR || em->stance == EM_STANCE_UNDER) {
        w->moveAngBX = 0;
        w->moveAngBY = em->rotY;
        w->moveAngBZ = 0;
        Em_MoveRotatedAccel(em, &w->moveAngBX);
    }
    return ret;
}

/*
 * Of the area's four points (em->tables[2]), finds the one nearest the
 * monster and writes the following one (index + 1, wrapping) to out.
 * Returns the nearest one's index; without a table writes (1000, 0, 1000)
 * and returns 0.
 */
u8 EmPlesioth_NextAreaPoint(Em *em, f32 *out)
{
    f32 dist[4];
    f32 (*points)[3];
    u32 i;
    f32 (*pt)[3];
    f32 best;
    f32 candidate;
    u8 nearest;
    f32 *next;

    points = (f32 (*)[3]) Em_AreaEntryValue(em, em->tables[2]);
    if (points == NULL) {
        out[0] = 1000.0f;
        out[1] = 0.0f;
        out[2] = 1000.0f;
        return 0;
    }
    pt = points;
    for (i = 0; i < 4; i++) {
        dist[i] = Vec_DistanceXZ(&em->posX, *pt);
        pt++;
    }
    best = dist[0];
    nearest = 0;
    candidate = dist[1];
    if (best > candidate) {
        best = candidate;
        nearest = 1;
    }
    candidate = dist[2];
    if (best > candidate) {
        best = candidate;
        nearest = 2;
    }
    candidate = dist[3];
    if (best > candidate) {
        nearest = 3;
    }
    next = points[(nearest + 1) & 3];
    out[0] = next[0];
    out[1] = next[1];
    out[2] = next[2];
    return nearest;
}

/*
 * Returns 1 when the floor under the monster lies below the water's depth
 * (Em.floorUnderSurface), or, unless floorOnly, when no hunter is in its
 * area; else 0.
 */
s32 EmPlesioth_OverDeepWater(Em *em, u8 floorOnly)
{
    if (em->floorUnderSurface != 0) {
        return 1;
    }
    if (!(floorOnly & 0xFF) && !(Player_InCurrentArea(em) & 0xFF)) {
        return 1;
    }
    return 0;
}

/* The file's initialised data, in the game's order */
EmTurnParams EmPlesioth_TurnParams[3] = {
#include "game/008C6070.inc"
};
u32 EmPlesioth_FinKeys2_0[8] = {
#include "game/008C60D0.inc"
};
u32 EmPlesioth_FinKeys2_1[8] = {
#include "game/008C60F0.inc"
};
u32 EmPlesioth_FinKeys2_2[8] = {
#include "game/008C6110.inc"
};
u32 EmPlesioth_FinKeys2_3[8] = {
#include "game/008C6130.inc"
};
EmFinKey *EmPlesioth_FinKeys2[4] = {
    (EmFinKey *)EmPlesioth_FinKeys2_0, (EmFinKey *)EmPlesioth_FinKeys2_1, (EmFinKey *)EmPlesioth_FinKeys2_2, (EmFinKey *)EmPlesioth_FinKeys2_3,
};
u32 EmPlesioth_FinKeys1_0[12] = {
#include "game/008C6160.inc"
};
u32 EmPlesioth_FinKeys1_1[12] = {
#include "game/008C6190.inc"
};
u32 EmPlesioth_FinKeys1_2[8] = {
#include "game/008C61C0.inc"
};
u32 EmPlesioth_FinKeys1_3[8] = {
#include "game/008C61E0.inc"
};
EmFinKey *EmPlesioth_FinKeys1[4] = {
    (EmFinKey *)EmPlesioth_FinKeys1_0, (EmFinKey *)EmPlesioth_FinKeys1_1, (EmFinKey *)EmPlesioth_FinKeys1_2, (EmFinKey *)EmPlesioth_FinKeys1_3,
};
s16 EmPlesioth_FinWait1[4] = {
#include "game/008C6210.inc"
};
s16 EmPlesioth_FinWait2[4] = {
#include "game/008C6218.inc"
};
s16 EmPlesioth_FinRewait1[4] = {
#include "game/008C6220.inc"
};
s16 EmPlesioth_FinRewait2[4] = {
#include "game/008C6228.inc"
};
s32 GmEm_ReturnZero(void); /* gm_sub */
u16 EmPlesioth_FinFoldTarget[8] = {
#include "game/008C6230.inc"
};
void *EmPlesioth_Callbacks[8] = {
    EmPlesioth_SpawnEffects, EmPlesioth_SpawnEffects, EmPlesioth_TableNop, EmPlesioth_UpdateMotionEffects,
    GmEm_ReturnZero, EmPlesioth_Reinit, NULL, NULL,
};
EmCurveKey EmPlesioth_VelYCurves_0[16] = {
#include "game/008C6260.inc"
};
EmCurveKey EmPlesioth_VelYCurves_1[16] = {
#include "game/008C62E0.inc"
};
EmCurveKey EmPlesioth_VelYCurves_2[12] = {
#include "game/008C6360.inc"
};
extern EmCurveKey EmCephadrome_VelYCurves_3[];
EmCurveKey *EmPlesioth_VelYCurves[4] = {
    EmPlesioth_VelYCurves_0, EmPlesioth_VelYCurves_1, EmPlesioth_VelYCurves_2, EmCephadrome_VelYCurves_3,
};
EmCurveKey EmPlesioth_SpeedCurves_0[16] = {
#include "game/008C63D0.inc"
};
EmCurveKey EmPlesioth_SpeedCurves_1[16] = {
#include "game/008C6450.inc"
};
EmCurveKey EmPlesioth_SpeedCurves_2[11] = {
#include "game/008C64D0.inc"
};
EmCurveKey *EmPlesioth_SpeedCurves[3] = {
    EmPlesioth_SpeedCurves_0, EmPlesioth_SpeedCurves_1, EmPlesioth_SpeedCurves_2,
};
