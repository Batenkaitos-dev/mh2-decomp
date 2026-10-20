/*
 * The Rathalos family: Rathian, Rathalos and their subspecies (Pink and
 * Gold Rathian, Silver and Azure Rathalos): the code from the family's
 * first function, EmRathalos_MotionSounds, to the Fatalis family's first.
 *
 * A Rathalos runs a state machine (include/em_rathalos.h names the states
 * and motions; include/em.h the groups): EmRathalos_Update reads the hits
 * (Em_UpdateDamage) and picks reactions, then EmRathalos_RunState calls the
 * group's dispatcher (EmRathalos_RunIdleState ... RunVersusState), which
 * calls one function per state with the monster and its work area
 * (EmWork; the flying families' view is EmFlyerWork). A state function
 * switches on Em.step; EmRathalos_ChangeState starts the next state through
 * the group's Set function, and EmRathalos_EndState / EndAirState hand the
 * choice back to the monster script. Flights move along curves
 * (EmRathalos_StartCurves, UpdateCurves) and Em_UpdateFlight;
 * EmRathalos_MotionEffects adds each motion's effects and sounds.
 */
#include "common.h"
#include "scene.h"
#include "quest.h"
#include "mine_point.h"
#include "em.h"
#include "em_rathalos.h"
#include "player.h"
#include "objattach.h"
#include "stageobj.h"
#include "area.h"
/* prototypes of callees */
void Em_SetPartScaled(Em *em, s32 bit);
void EmTail_Create(Em *);
void Em_AreaExitPoint(Em *em, f32 *out);
s32 Area_ContainsPos(s32, f32 *);
void Em_EnterNextArea(Em *);
struct EmIdEntry;
f32 *Em_AreaEntryValue(Em *em, struct EmIdEntry *entry);
u16 Rand_Next(s32 stream);

s8 EmMine_CreateTimed0(Em *em, f32 *pos);
void EmRathalos_ChangeState(Em *em, u16 group, u16 state, u8 mode);
/* unit's own declaration: the amount is declared s32, as the callers pass it; with the definition's s16 the compiler narrows the amount to 16 bits before each call (EmRathalos_EatNeed1, EmRathalos_EatNeed2), which the game's code doesn't do */
void Em_AddHp(Em *em, s32 n);
void Em_BeginStatus1(Em *em);
void Em_PlaySeAtBone(Em *em, s32 se, s32 bone);
void Em_PlaySeAtBoneOnFrame(Em *em, s32 frame, s32 se, s32 bone);
void Em_SetLayerMotion(Em *em, s32 motion, s32 blend, s32 frame, s32 layer);
void Em_SetMotion(Em *em, s32 motion, s32 a, s32 b);
u16 Math_AngleXZ(f32 *from, f32 *to);
void Player_ClearFlag(void *, u32 flag);
void Player_SetFlag(void *obj, u32 flag);
void QuestEm_Captured(Em *em);
void RathalosAttack_Create(Em *, s8);
void EmRangedAttack_CreateAtBone(Em *em, s32 a, s32 b, s32 c, s32 d, s32 e);
void Em_SelectSightRow(Em *em, u8 idx);
void Em_MoveRotated(Em *em, void *src);
void Em_MoveRotatedAccel(Em *em, void *ang);
void Em_SetStateRand(Em *em, s32 arg1, s32 arg2);
void Em_LoadReactionSet(Em *em, u8 k);
void Em_ComputeGoal2(Em *em, f32 *target);
void Em_GetPitchToGoal(Em *em, s32 *out, f32 a, f32 b);
s32 Em_GetTargetPos(Em *em, u8 idx, f32 *out);
void Em_ClearNeed1State(Em *em);
void Em_ClearNeed2State(Em *em);
void Em_SpawnStatus4Effect(Em *em, s32 a);
void Em_AddNeed1(Em *em, s32 n);
void Em_AddNeed2(Em *em, s32 n);
void Em_SetHitReactMode(Em *em, u8 a);
void Em_SetTrapTimeGoal(Em *em);
void Em_ClearPoison(Em *em);
void Em_SetHold2(Em *em);
void Em_SetCaptured(Em *em, s32 a);
void Em_PlayCommonSeAtBoneOnFrame(Em *em, s32 frame, s32 se, s32 bone);
void Em_PlaySeAtBoneVolOnLayerFrame(Em *em, s32 frame, s32 se, s32 bone, s32 flag, s32 vol);
void Em_UpdateFlight(Em *em, s32 flags, EmFlyParam *p);
s32 Em_TurnToTargetMotion(Em *em, u16 range, u16 motFront, u16 motLeft, u16 motRight);
s32 Em_StepToGoal(Em *em, u16 step, u8 flags);
u8 Em_TurnToGoal(Em *em, u16 step, u8 flags);
void Em_StartTurnToTarget(Em *em, EmTurnParams *t);
s32 Em_TurnStep(Em *em, EmTurnParams *t);
void Em_RestartScript(Em *em);
void EmRathalos_Init(Em *);
void EmRathalos_TurnToGoal(Em *, EmWork *);
void EmRathalos_TakeOff(Em *, EmWork *);
void EmRathalos_Land(Em *, EmWork *);
void EmRathalos_HopToGoal(Em *, EmWork *);
void EmRathalos_FlyOutOfArea(Em *em, EmWork *work);
void EmRathalos_FlyToGoalAndLand(Em *, EmWork *);
void EmRathalos_FlyOutOfAreaAlt(Em *em, EmWork *work);
void EmRathalos_FlyIntoArea(Em *, EmWork *);
void EmRathalos_StepForward(Em *, s32);
void EmRathalos_QuestIntro(Em *, EmWork *);
void EmRathalos_Captured(Em *em);
void EmRathalos_StoryEvent(Em *);
void EmRathalos_VersusEatBait(Em *em, EmWork *work);
void EmRathalos_VersusPoseC(Em *, EmWork *);
void EmRathalos_Die(Em *em, EmWork *work);
void EmRathalos_DieHeld(Em *em, EmWork *work);
void EmRathalos_DieFalling(Em *em, EmWork *work);
void EmRathalos_RunIdleState(Em *em, EmWork *work);
void EmRathalos_RunAttackState(Em *em, EmWork *work);
void PartFx_SpawnForEmScaled(Em *em, s32 a, s32 b, f32 scale);
void EmRathalos_MotionSounds(Em *em, EmWork *work);
void ElementBurst_CreateOnEm(Em *em, u8 k, u8 c, f32 scale);
void EmRathalos_MotionEffects(Em *em, EmFlyerWork *work);
void EmRathalos_SpawnLandingDust(Em *em);
void EmRathalos_SpawnK14Type0AEvery5(Em *em);
void EmRathalos_MoveByVelYCurve(Em *em, s32 kind);
void EmRathalos_StartCurves(Em *em, u8 kind);
u8 EmRathalos_UpdateCurves(Em *em);
void AttackBurst_CreateAtBone(Em *em, s16 bone, s16 k, u16 c);
void AlertMark_Create(void *owner, s16 v, u8 k);
void DustCloud_Create(Em *em, s16 k, s16 c, f32 scale);
/* unit's own declaration: the arguments are declared s32, as the callers pass them; with the definition's u8 and s16 parameters the compiler narrows them before the call (EmRathalos_Stun, EmRathalos_StunDowned), which the game's code doesn't do */
void StunStars_CreateEm(Em *, s32, s32, s32, f32, f32);
s32 Sound_RandomSe(s32 id, s16 lo, s32 alt, s16 n);
s32 EmMotion_PassedFrameScaled(void *p, f32 frame, s32 layer);
s32 EmMotion_ReachedFrame(void *obj, s32 a, f32 frame);
s32 EmMotion_InFrameRange(void *em, s32 layer, f32 from, f32 to);
f32 Vec_Distance(f32 *a, f32 *b);
void CamShake_ReqAIf(Em *obj, s32 i);
f32 Vec_DistanceXZ(f32 *a, f32 *b);
s32 Versus_BothDefeated(void);
s32 QuestEm_CanAppear(Em *em);
void Player_ClearVelocity(void *);
void Player_ClearAccel2(void *);
void Motion_InitTime(void *p, s16 step);
void Shadow_Spawn(void *obj, s16 type);
extern f32 gAreaGroundY[];
extern EmFlyParam EmRathalos_FlyParam;
extern EmFlyParam EmRathalos_FlyParam2;
extern EmTurnParams EmRathalos_TurnParams[2];
void QuestEm_Hunted(Em *em);
void Em_BonePos(void *obj, s16 bone, f32 *out);
void EmRathalos_SetIdleState(Em *em, u16 state, u8 mode);
void EmRathalos_SetMoveState(Em *em, u16 state, u8 mode);
void EmRathalos_SetFlyState(Em *em, u16 state, u8 mode);
void EmRathalos_SetAttackState(Em *em, u16 state, u8 mode);
/* unit's own declaration: group and state are declared u16; with the definition's s32 the callers (EmRathalos_ChangeState, EmRathalos_SetIdleState ...) pass the state without the 16-bit mask the game's code applies */
void Em_SetStateSync(Em *em, u16 group, u16 state, u8 mode);
void Em_SetSpeedScaled(Em *em, u16 flags, f32 x, f32 y);
void Em_ComputeGoal(Em *em, f32 *target);
void Em_TypeSetup(Em *em);
void Em_SetTargetAngle(Em *em);
void Em_StartRage(Em *em);
void Em_StartPendingRage(Em *em);
s32 Game_IsOnline(void);
void Vec_Set(f32 *out, f32 x, f32 y, f32 z);
void EmRathalos_SetEscapePoint(Em *em);
void Em_TargetPlayerInRange(Em *);
void Versus_MarkDefeat(Em *);
s8 QuestEm_Appear(Em *em);
void Em_ResetToBasePose(Em *em);
void Em_BreakPart(Em *em, s32 part);
void Em_EndAllStatus(Em *em);
void Em_BeginStatus2(Em *em);
void Em_BeginStatus4(Em *em);
void Em_BeginStatus5(Em *em);
void Em_EndStatus1(Em *em);
void Em_EndStatus2(Em *em);
void Em_EndStatus4(Em *em);
void Em_EndStatus5(Em *em);
void Em_CheckRageByHp(Em *em, s16 n);
void Em_ResetNotice(Em *em);
void Em_RefillNeed3(Em *em);
void Em_StartGoalAction(Em *em, s32 arg1);
void Em_ClearPart(Em *em, u16 part);
s16 Player_IsState(void *obj, u8 state, u8 sub);
void VersusBait_Create(Player *p, Em *em, u8 k, s32 unused, f32 *pos);
void Versus_RouteEmByTeam(Em *em);
s32 Versus_DefeatCamera(Em *pl);
void Em_SpawnLyingPuffs(Em *em, s32 bone, f32 *pos, f32 scale);
extern EmCurveKey *EmRathalos_MoveVelYCurves[];
s8 EmMine_Create(Em *em, s32 idx);
s32 EmMine_Update(Em *em);
void EmMine_Remove(Em *em);
/* A position copied whole (EmRathalos_CarveThrowPt): the original copies it as one struct. */
extern Vec3f EmRathalos_CarveThrowPt __attribute__((aligned(8)));
u16 Math_Heading(f32 x1, f32 z1, f32 x2, f32 z2);
void Em_SetMotionRow(Em *em, s32 row, s32 arg2, s32 arg3);
s32 Em_ComputeMaxHp(Em *em, s16 idx);
/* unit's own declaration: the arguments are declared s32, as the callers pass them; with the definition's s16 parameters the compiler narrows them before the call (EmRathalos_Rest, EmRathalos_Status2LieDown), which the game's code doesn't do */
s32 Em_RegenHp(Em *em, s32 n, s32 max, s32 interval);
void Em_ClearLowHpNeed(Em *em);
s32 Save_TestEventFlag(s32);
u16 Area_IdForCurrentTime(u16);
s8 Em_NearestActionToBone(Em *em, s32 bone, f32 *pos);
void GmEm_ResetSize(Em *em);
void CamShake_ReqC(Em *em, s32 a);
void Em_AimAtTarget(Em *em);
void Em_GetPitchToTarget(Em *em, s32 *out, f32 a, f32 b);
f32 Math_Sqrtf(f32 x);
void Em_RaisePlayerAreaTriggerLax(Em *em);
u16 Area_DayId(u16 id);
u8 Em_UpdateDamage(Em *em, u8 *out);
void Em_UpdatePendingScript(Em *em);
void Em_ClearHitReactMode(Em *em);
void Em_SetStatus4Timer(Em *em);
void Em_SetStatus2Timer(Em *em);
void Em_DecayNeed3(Em *em);
void Em_CheckCarryRequest(Em *em);
void EmRathalos_RunState(Em *em, s32 arg1);
s32 Em_UpdateCombatTimer(Em *em);
void Em_SetTrapTime(Em *em, u8 *p);
void Em_SetStatus1Timer(Em *em);
void Em_SetStatus5Timer(Em *em);
void Em_ResetKindTimer(Em *em);
s32 Em_IsHeld(Em *em);
u16 Map_DayId(u16);
void Em_CheckLowHp(Em *em, f32 ratio);
void Em_DecayNeed2(Em *em);
void Em_DecayNeed1Only(Em *em);
void Em_DecayNeed1(Em *em);
void EmHermitaur_ChangeState(Em *em, u16 group, u16 state, u8 mode);
s32 EmRathalos_ReachedTarget(Em *em);
void EmRathalos_EndSwoop(Em *em, s32 arg1);
void EmRathalos_StepPitch(Em *em, s32 mode, f32 height);
AreaRecord *Stage_GetAreaRecord(u16 x);
void Area_ToWorld(s32 area, f32 *pos, f32 *out);
u8 Em_AllPlayersHere(Em *em);
s32 Obj_SameArea(void *obj, Player *b);
s32 EmRathalos_ReachedPoint(Em *em);
void PartFx_SpawnForEm(Em *em, s32 a, s32 b);
f32 *Mtx_FromAnglesYXZ(s32 *angles, f32 *m);
void Vec_Rotate(f32 *out, f32 *in, f32 *mtx);
void Em_PlaySeTypeAtBoneInFrames(Em *em, s32 frame, s32 frame2, s32 se, s32 bone, s32 vol, s32 a);
void Em_PlaySeTypeAtBoneOnFrame(Em *em, s32 frame, s32 se, s32 bone);
/* end of prototypes */

/*
 * The Rathalos family's motion sounds (called by EmRathalos_MotionEffects
 * when the motion changes): per motion, the sounds to play at bones on
 * given frames (Em_PlaySeAtBoneOnFrame(em, frame, sound, bone)). It lies
 * in the Cephadrome range, before Rathalos's setup function.
 */
void EmRathalos_MotionSounds(Em *em, EmWork *work)
{
    s16 motion;

    motion = work->sndMotion;
    switch (motion) {
    case EM_MOTION_ID(0, 0x2):
        Em_PlaySeAtBoneOnFrame(em, 6, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 3, 0x1A);
        break;
    case EM_MOTION_ID(0, RATH_MOT_WALK):
        Em_PlaySeAtBoneOnFrame(em, 0x34, 1, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x74, 1, 0x1A);
        break;
    case EM_MOTION_ID(0, 0x4):
        Em_PlaySeAtBoneOnFrame(em, 0x28, 6, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 6, 0x1A);
        break;
    case EM_MOTION_ID(0, RATH_MOT_TURN_RIGHT):
        Em_PlaySeAtBoneOnFrame(em, 0x19, 1, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 1, 0x1A);
        break;
    case EM_MOTION_ID(0, RATH_MOT_TURN_LEFT):
        Em_PlaySeAtBoneOnFrame(em, 0x19, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 1, 0x14);
        break;
    case EM_MOTION_ID(0, 0x7):
        Em_PlaySeAtBoneOnFrame(em, 6, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 1, 0x14);
        break;
    case EM_MOTION_ID(0, 0x8):
        Em_PlaySeAtBoneOnFrame(em, 6, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 1, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 1, 0x1A);
        break;
    case EM_MOTION_ID(0, RATH_MOT_RUN_STOP):
        Em_PlaySeAtBoneOnFrame(em, 4, 6, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 6, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 4, 0x1A);
        break;
    case EM_MOTION_ID(0, RATH_MOT_LIMP):
        Em_PlaySeTypeAtBoneOnFrame(em, 6, 0x59, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 6, 4, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 0, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 0x12, 0x14);
        break;
    case EM_MOTION_ID(0, RATH_MOT_DESCEND):
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0xB, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x42, 0xB, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x7A, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x7E, 0xB, 0xC);
        break;
    case EM_MOTION_ID(0, RATH_MOT_GLIDE):
        Em_PlaySeAtBoneOnFrame(em, 6, 0xC, 6);
        Em_PlaySeAtBoneOnFrame(em, 4, 0xC, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 0xC, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0xC, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x1A, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 6, 0x19, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x19, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0x1A, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 0x19, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 0x19, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0x1A, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 0x19, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 0x19, 6);
        break;
    case EM_MOTION_ID(0, 0xD):
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0xF, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0xF, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0xF, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 0xF, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x42, 0xF, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0xF, 0xC);
        break;
    case EM_MOTION_ID(0, RATH_MOT_FLY):
    case EM_MOTION_ID(0, 0x55):
    case EM_MOTION_ID(0, 0x56):
    case EM_MOTION_ID(0, 0x58):
    case EM_MOTION_ID(0, 0x59):
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x1A, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0x19, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0x19, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 0x19, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 0x19, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x19, 6);
        break;
    case EM_MOTION_ID(0, RATH_MOT_HOVER):
        Em_PlaySeAtBoneOnFrame(em, 4, 0xC, 6);
        Em_PlaySeAtBoneOnFrame(em, 8, 0xC, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 0xB, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x8E, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x8A, 0xB, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0xD4, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0xD8, 0xB, 0xC);
        break;
    case EM_MOTION_ID(0, RATH_MOT_RUN):
        Em_PlaySeTypeAtBoneOnFrame(em, 0x12, 0x4F, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 6, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 6, 0x1A);
        break;
    case EM_MOTION_ID(0, RATH_MOT_TAKE_OFF):
        Em_PlaySeAtBoneOnFrame(em, 0x28, 7, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0xD, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x52, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x56, 0xB, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x88, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x8C, 0xB, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0xB6, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0xBA, 0xB, 0xC);
        break;
    case EM_MOTION_ID(0, RATH_MOT_TOUCH_DOWN):
        Em_PlaySeAtBoneOnFrame(em, 2, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 6, 0xB, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 4, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 4, 1, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x60, 0, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 7, 9);
        break;
    case EM_MOTION_ID(0, 0x1D):
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 2, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0xE, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 8, 0xE, 6);
        break;
    case EM_MOTION_ID(0, RATH_MOT_LIE):
        Em_PlaySeTypeAtBoneOnFrame(em, 0x24, 0x57, 0x23);
        break;
    case EM_MOTION_ID(0, RATH_MOT_ROAR):
        Em_PlaySeAtBoneOnFrame(em, 0x32, 2, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 6, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x54, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x68, 4, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x7C, 0x11, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0xCC, 6, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0xD2, 2, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0xFE, 0x12, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x120, 2, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 0xF, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0xF, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x80, 0xF, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x84, 0xF, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x122, 0xE, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x126, 0xE, 6);
        Em_PlaySeAtBoneOnFrame(em, 2, 0x15, 0);
        Em_PlaySeAtBoneOnFrame(em, 2, 0x23, 0x23);
        Em_PlaySeTypeAtBoneOnFrame(em, 0x30, 0x49, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x80, 0x22, 0x23);
        Em_PlaySeTypeAtBoneOnFrame(em, 0xB8, 0x4C, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x120, 0x22, 0x23);
        break;
    case EM_MOTION_ID(0, 0x24):
        Em_PlaySeTypeAtBoneOnFrame(em, 0x24, 0x4C, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 0, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0x15, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0xC, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0xC, 6);
        Em_PlaySeAtBoneOnFrame(em, 0xA6, 0, 0x1A);
        break;
    case EM_MOTION_ID(0, RATH_MOT_GET_UP):
        Em_PlaySeTypeAtBoneOnFrame(em, 0x1E, 0x48, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0xC, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 2, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 1, 0x1A);
        break;
    case EM_MOTION_ID(0, RATH_MOT_COLLAPSE):
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 0x57, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 0xA4, 9, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x90, 3, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x88, 0xE, 6);
        Em_PlaySeAtBoneOnFrame(em, 0xB2, 4, 0x22);
        break;
    case EM_MOTION_ID(0, RATH_MOT_LIE_DOWN):
        Em_PlaySeTypeAtBoneOnFrame(em, 2, 0x57, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 2, 0x17, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 3, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x66, 0x10, 0x1A);
        break;
    case EM_MOTION_ID(0, RATH_MOT_LEAP):
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 0xD, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x66, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 0xD, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x88, 0xA, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x8C, 0xA, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 2, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 4, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 5, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 7, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x70, 0x1A, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x74, 0x1B, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x78, 0x1B, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x8E, 0x1A, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x94, 0x1B, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x96, 0x1B, 6);
        Em_PlaySeAtBoneOnFrame(em, 0xB0, 0x1A, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0xB4, 0x1B, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0xB8, 0x1B, 6);
        Em_PlaySeAtBoneOnFrame(em, 0xB0, 0x1A, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0xD6, 0x1B, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0xDA, 0x1B, 6);
        break;
    case EM_MOTION_ID(0, RATH_MOT_LEAP_LAND):
        Em_PlaySeTypeAtBoneOnFrame(em, 0xC, 0x4F, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 2, 0xF, 6);
        Em_PlaySeAtBoneOnFrame(em, 6, 0xF, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0xE, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0xE, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 0xA, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0xA, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x5C, 0xA, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x60, 0xA, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0x11, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0x10, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 6, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 8, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x5C, 6, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x70, 6, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x8E, 6, 0x1A);
        break;
    case EM_MOTION_ID(0, RATH_MOT_TAIL_SPIN_B):
        Em_PlaySeTypeAtBoneOnFrame(em, 8, 0x4B, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0xF, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x14, 0x2A);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 0, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 1, 0x1A);
        break;
    case EM_MOTION_ID(0, RATH_MOT_TAIL_SPIN):
        Em_PlaySeTypeAtBoneOnFrame(em, 8, 0x4B, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0xF, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x14, 0x2A);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 1, 0x14);
        break;
    case EM_MOTION_ID(0, RATH_MOT_SWOOP_DIVE):
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 0xB, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 6, 0x1A, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x1B, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0x1B, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 0x1A, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 0x1B, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x1B, 6);
        break;
    case EM_MOTION_ID(0, RATH_MOT_SWOOP_CLAW):
        Em_PlaySeAtBoneOnFrame(em, 0x22, 0x5B, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 8, 0xD, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x36, 0xD, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x62, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x66, 0xB, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x98, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x9C, 0xB, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x1C, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x1D, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 8, 0x1D, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 0x1E, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 0x1D, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 0x1D, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 0x1E, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 0x1D, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 0x1D, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x5E, 0x1C, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x5E, 0x1B, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x62, 0x1B, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x7C, 0x1A, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x7C, 0x1D, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x80, 0x1D, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x9A, 0x1A, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x9A, 0x1D, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x9E, 0x1D, 6);
        break;
    case EM_MOTION_ID(0, RATH_MOT_AIR_FIREBALL):
        Em_PlaySeAtBoneOnFrame(em, 6, 0x18, 0x23);
        Em_PlaySeTypeAtBoneOnFrame(em, 6, 0x51, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0xF, 6);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0xF, 0xC);
        break;
    case EM_MOTION_ID(0, RATH_MOT_FIREBALL):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x12, 0);
        if ((em->stateGroup != 8) || (em->state != 3)) {
            Em_PlaySeAtBoneOnFrame(em, 0x50, 0x18, 0x23);
        }
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0x28, 0x23);
        Em_PlaySeTypeAtBoneOnFrame(em, 0x4A, 0x51, 0x23);
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
    case EM_MOTION_ID(0, RATH_MOT_AIR_FIREBALL_RISE):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x48, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 0xF, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0xF, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x74, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x78, 0xB, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0xBE, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0xC2, 0xB, 0xC);
        break;
    case EM_MOTION_ID(0, RATH_MOT_FIREBALL_B):
        Em_PlaySeTypeAtBoneOnFrame(em, 0x14, 0x56, 0x23);
        Em_PlaySeTypeAtBoneOnFrame(em, 0x4E, 0x51, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 0x18, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x78, 0x18, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xA4, 0x18, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x12, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x13, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 0xF, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 0xF, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0xFA, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x12A, 0x12, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x11E, 0x13, 0);
        break;
    case EM_MOTION_ID(0, RATH_MOT_ALERT_IDLE):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x13, 0x2A);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 0x10, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0x12, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 6, 0x1A);
        Em_PlaySeTypeAtBoneOnFrame(em, 0xA0, 0x56, 0x23);
        Em_PlaySeTypeAtBoneOnFrame(em, 0xE6, 0x5D, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xEE, 0x16, 0);
        break;
    case EM_MOTION_ID(0, RATH_MOT_BACK_HOP):
        Em_PlaySeTypeAtBoneOnFrame(em, 0x16, 0x56, 0x23);
        Em_PlaySeTypeAtBoneOnFrame(em, 0x48, 0x51, 0x23);
        if ((em->stateGroup == 3) && (em->state == 0x10)) {
            Em_PlaySeAtBoneOnFrame(em, 0x52, 0x18, 0x23);
        }
        Em_PlaySeAtBoneOnFrame(em, 6, 0x14, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 0xF, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 0xF, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x6E, 0xB, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0xAC, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0xB0, 0xB, 0xC);
        break;
    case EM_MOTION_ID(0, RATH_MOT_AIR_DIVE):
        Em_PlaySeTypeAtBoneOnFrame(em, 0x50, 0x49, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 0x30, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0xF, 6);
        Em_PlaySeAtBoneOnFrame(em, 8, 0xF, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 0xF, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0xF, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x42, 0xF, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 0xF, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x5C, 0xF, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 0xF, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x70, 0xF, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x74, 0xF, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x92, 0xF, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x96, 0xF, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0xBC, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0xB8, 0xB, 0xC);
        break;
    case EM_MOTION_ID(0, RATH_MOT_TURN_IN_PLACE):
        Em_PlaySeTypeAtBoneOnFrame(em, 0x10, 0x56, 0x23);
        Em_PlaySeTypeAtBoneInFrames(em, 0x6E, 0x118, 0x5C, 0x23, 7, 0);
        Em_PlaySeAtBoneOnFrame(em, 2, 0x12, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 4, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 4, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 5, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x78, 4, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x108, 4, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x14, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0xC, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 0xC, 6);
        break;
    case EM_MOTION_ID(0, 0x37):
        Em_PlaySeAtBoneOnFrame(em, 8, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 8, 8, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 5, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 1, 0x14);
        break;
    case EM_MOTION_ID(0, RATH_MOT_BACK_HOP_LAND):
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x12, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x28, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 8, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 2, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 4, 0xD, 0xC);
        break;
    case EM_MOTION_ID(0, RATH_MOT_FEED):
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 6, 0x22, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x68, 3, 0x1A);
        break;
    case EM_MOTION_ID(0, 0x3C):
        Em_PlaySeTypeAtBoneOnFrame(em, 6, 0x53, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 0x13, 0x2A);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 6, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0x11, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x74, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x94, 3, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0xC4, 3, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 0xC, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0xD, 0xC);
        break;
    case EM_MOTION_ID(0, 0x3D):
        Em_PlaySeTypeAtBoneOnFrame(em, 6, 0x53, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x12, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 6, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 4, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x6E, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0xAC, 3, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x5A, 0x15, 0);
        break;
    case EM_MOTION_ID(0, RATH_MOT_SHAKE_OFF):
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 0x4F, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x56, 0x1F, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 4, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 8, 0xD, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x62, 0x13, 0x2A);
        Em_PlaySeAtBoneOnFrame(em, 0x7E, 3, 0x1A);
        break;
    case EM_MOTION_ID(0, 0x3F):
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 0x52, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 0x1F, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0, 0x1A);
        break;
    case EM_MOTION_ID(0, 0x40):
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 0x52, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x7A, 0, 0x1A);
        break;
    case EM_MOTION_ID(0, 0x41):
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 0x54, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xA8, 0x2A, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x27, 0x2C);
        Em_PlaySeAtBoneOnFrame(em, 0xFC, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x130, 0x27, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 4, 0x2C);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0x12, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 6, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 0x10, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0xD2, 3, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x142, 3, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 9, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 9, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 0x12, 0);
        Em_PlaySeAtBoneOnFrame(em, 0xA2, 0x12, 0);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0xC, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0xC, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x11E, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x122, 0xD, 0xC);
        break;
    case EM_MOTION_ID(0, 0x42):
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 0x52, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x13, 0);
        break;
    case EM_MOTION_ID(0, 0x43):
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0x2D, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0x17, 0);
        break;
    case EM_MOTION_ID(0, RATH_MOT_DIE):
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 2, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 4, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0xDC, 0x10, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x108, 9, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x108, 7, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x114, 0x17, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x180, 0x16, 0);
        Em_PlaySeTypeAtBoneOnFrame(em, 0x18, 0x52, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x8C, 0x28, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xB4, 0x2C, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x13A, 0x2F, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x174, 0xF, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x176, 0xE, 0xC);
        break;
    case EM_MOTION_ID(0, RATH_MOT_KNOCK_DOWN):
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 0x52, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x13, 0x2A);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 9, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0x11, 0);
        break;
    case EM_MOTION_ID(0, RATH_MOT_DOWN):
    case EM_MOTION_ID(0, 0x4B):
        Em_PlaySeAtBoneOnFrame(em, 8, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0x31, 0x23);
        break;
    case EM_MOTION_ID(0, RATH_MOT_DOWN_GET_UP):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 1, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 1, 0x1A);
        break;
    case EM_MOTION_ID(0, 0x48):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 1, 0x14);
        break;
    case EM_MOTION_ID(0, 0x49):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x1E, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x1D, 6);
        Em_PlaySeAtBoneOnFrame(em, 8, 0x1D, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x2A, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0x12, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x10, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 9, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 7, 0);
        break;
    case EM_MOTION_ID(0, 0x4A):
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 0x52, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x13, 0x2A);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 9, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0x11, 0);
        break;
    case EM_MOTION_ID(0, 0x4C):
        Em_PlaySeTypeAtBoneOnFrame(em, 0x10, 0x54, 0x23);
        Em_PlaySeTypeAtBoneOnFrame(em, 0x74, 0x5A, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 1, 0x1A);
        break;
    case EM_MOTION_ID(0, RATH_MOT_FALL):
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 0x53, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0xB, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 0x14, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 0x1C, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 0x1B, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x5C, 0x1B, 0xC);
        break;
    case EM_MOTION_ID(0, RATH_MOT_FALL_LAND):
        Em_PlaySeAtBoneOnFrame(em, 0xA, 9, 0);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 7, 0x2A);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x11, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 0x2C, 0x23);
        break;
    case EM_MOTION_ID(0, RATH_MOT_HEAD_DOWN):
        Em_PlaySeAtBoneOnFrame(em, 0x26, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x24, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0x22, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x66, 0x22, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xA4, 0x22, 0x23);
        break;
    case EM_MOTION_ID(0, 0x50):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x16, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 0x17, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 0x2D, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xC8, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 3, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x140, 3, 0x1A);
        break;
    case EM_MOTION_ID(0, 0x51):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x16, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 0x17, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 0x2D, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xC8, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x140, 3, 0x14);
        break;
    case EM_MOTION_ID(0, RATH_MOT_DIE_FALLING):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0);
        Em_PlaySeTypeAtBoneOnFrame(em, 0x32, 0x54, 0x23);
        break;
    case EM_MOTION_ID(0, 0x53):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x17, 0);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x10, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 0, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 0, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x80, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0xC6, 0, 0x14);
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 0x53, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xA8, 0x27, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xA2, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 0xA6, 0xD, 0xC);
        break;
    case EM_MOTION_ID(0, 0x54):
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0x2A, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0xD, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 0xA, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0xE, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 0xE, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 0xA, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 0xC, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x5C, 0xC, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 5, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x56, 3, 0x14);
        break;
    case EM_MOTION_ID(0, RATH_MOT_SWOOP):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x1A, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 8, 0x1B, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x1B, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 0x1C, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 0x1C, 0x22);
        break;
    case EM_MOTION_ID(0, RATH_MOT_LEVEL_OUT):
        Em_PlaySeTypeAtBoneOnFrame(em, 0x30, 0x51, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0xD, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 0xD, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x7C, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x80, 0xB, 0xC);
        break;
    case EM_MOTION_ID(0, RATH_MOT_CHARGE_TRIP):
        Em_PlaySeAtBoneOnFrame(em, 0x10, 6, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 6, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0xB0, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0xD6, 3, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x108, 1, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x130, 1, 0x1A);
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 0x5B, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 9, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x5C, 0x10, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x8C, 0x11, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 0xF, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0xF, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x10A, 0xC, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x10E, 0xC, 0xC);
        break;
    case EM_MOTION_ID(0, RATH_MOT_FLIP):
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 1, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 0xD, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 0x14, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 0x13, 0);
        Em_PlaySeTypeAtBoneOnFrame(em, 0x54, 0x4F, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xA0, 0xF, 6);
        Em_PlaySeAtBoneOnFrame(em, 0xA4, 0xF, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0xD2, 0xC, 6);
        Em_PlaySeAtBoneOnFrame(em, 0xD6, 0xC, 0xC);
        break;
    case EM_MOTION_ID(0, RATH_MOT_FLIP_FALL):
        Em_PlaySeAtBoneOnFrame(em, 8, 0xA, 6);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0xA, 0xC);
        break;
    case EM_MOTION_ID(0, RATH_MOT_FLIP_LAND):
        Em_PlaySeAtBoneOnFrame(em, 8, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 8, 8, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x68, 5, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 1, 0x14);
        break;
    case EM_MOTION_ID(0, RATH_MOT_TRAPPED):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x10, 0);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x12, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 9, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x27, 0x23);
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 0x52, 0x23);
        break;
    case EM_MOTION_ID(0, RATH_MOT_STRUGGLE):
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0x12, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 0x12, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0xC, 6);
        Em_PlaySeAtBoneOnFrame(em, 8, 0xC, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x28, 0x23);
        Em_PlaySeTypeAtBoneOnFrame(em, 0x28, 0x53, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0);
        break;
    case EM_MOTION_ID(0, RATH_MOT_DOWNED):
        Em_PlaySeAtBoneOnFrame(em, 0x22, 0x16, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 0x16, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x62, 4, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 3, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x2A, 0x23);
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 0x53, 0x23);
        break;
    case EM_MOTION_ID(0, RATH_MOT_DOWNED_GET_UP):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 6);
        Em_PlaySeAtBoneOnFrame(em, 8, 0x16, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 3, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x50, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0x20, 0x23);
        
    case EM_MOTION_ID(0, RATH_MOT_IDLE):
    default:
        break;
    }
}

/*
 * The family's setup (Em_TypeSetup's entry for the Rathalos kinds): nothing
 * to do.
 */
void EmRathalos_Setup(Em *em)
{
}

/*
 * Creates the family's attached objects: a stage object of kind 0x13 (type
 * 0x22) and attached effect 0.
 */
void EmRathalos_SpawnEffects(void *em)
{
    AlertMark_Create(em, 0x22, 0);
    Shadow_Spawn((Player *) em, 0);
}

/*
 * Sets a Rathalos up: the scaled parts 3, 4 and 8; in the quest-less stage
 * (gQuest.stage 0) a fixed start position per area, 1500 apart per slot;
 * the idle motion, full hp from the size, the flight work's turn and roll
 * speeds; and, the first time, hides mesh 1 and creates the stage object of
 * kind 9.
 */
void EmRathalos_Init(Em *em)
{
    EmFlyerWork *w = (EmFlyerWork *)&em->work;
    s16 maxHp;

    Em_SetPartScaled(em, 3);
    Em_SetPartScaled(em, 4);
    Em_SetPartScaled(em, 8);
    if (gQuest.stage == 0) {
        em->rotY = 0x8000;
        switch (gScene.area) {
        case 0:
            em->posX = 8000.0f + 1500.0f * (f32) em->slotNo;
            em->posY = 0.0f;
            em->posZ = 6000.0f;
            break;
        case AREA_SNOWY_MOUNTAINS_NIGHT_9:
            em->posX = 7900.0f + 1500.0f * (f32) em->slotNo;
            em->posY = 1050.0f;
            em->posZ = 13900.0f;
            break;
        case AREA_JUNGLE_NIGHT_5:
            em->posX = 10000.0f + 1500.0f * (f32) em->slotNo;
            em->posY = 200.0f;
            em->posZ = 8000.0f;
            break;
        case AREA_JUNGLE_NIGHT_7:
            em->posX = 9900.0f + 1500.0f * (f32) em->slotNo;
            em->posY = 300.0f;
            em->posZ = 10350.0f;
            break;
        case AREA_DESERT_NIGHT_11:
            em->posX = 10000.0f + 1500.0f * (f32) em->slotNo;
            em->posY = 0.0f;
            em->posZ = 5700.0f;
            em->rotY = 0x4000;
            break;
        case AREA_VOLCANO_NIGHT_9:
            em->posX = 14300.0f + 1500.0f * (f32) em->slotNo;
            em->posY = 0.0f;
            em->posZ = 7100.0f;
            break;
        case AREA_FOREST_HILLS_4:
            em->posX = 9750.0f + 1500.0f * (f32) em->slotNo;
            em->posY = 300.0f;
            em->posZ = 7750.0f;
            break;
        case AREA_FOREST_HILLS_5:
            em->posX = 10500.0f + 1500.0f * (f32) em->slotNo;
            em->posY = 0.0f;
            em->posZ = 9500.0f;
            break;
        default:
            em->posX = 5000.0f + 1500.0f * (f32) em->slotNo;
            em->posY = 0.0f;
            em->posZ = 9000.0f;
            break;
        }
    }
    Em_SetMotionRow(em, 1, 0, 0);
    em->stance = EM_STANCE_GROUND;
    w->fxMotion = 0;
    maxHp = Em_ComputeMaxHp(em, 0);
    em->hp = maxHp;
    em->hpMax = maxHp;
    em->scriptPending = 1;
    em->blind = 0;
    em->unk_InitFlag = 1;
    em->aimAngle = 0x4000;
    w->pitchStep = 0x100;
    ((EmWork *)w)->turnSpeed = 0x200;
    w->rollSpeed = 0x100;
    w->moveAngBY = 0x2000;
    w->moveAngBX = 0;
    if ((em->fxFlags &= 1) == 0) {
        switch (em->type) {
        case EM_KIND_RATHIAN:
        case EM_KIND_GOLD_RATHIAN:
        case EM_KIND_PINK_RATHIAN:
        case EM_KIND_RATHALOS:
        case EM_KIND_SILVER_RATHALOS:
        case EM_KIND_AZURE_RATHALOS:
            em->meshShown[1] = 0;
            EmTail_Create(em);
            break;
        }
    }
}

/*
 * Starts a Rathalos: Init, then in two story quests per pair (Rathalos
 * kinds in stages QUEST_ID_RATHALOS_SCENE_1/QUEST_ID_RATHALOS_SCENE_2, Rathian kinds in QUEST_ID_RATHIAN_SCENE_1/QUEST_ID_RATHIAN_SCENE_2) places it in a
 * far area until the quest's event flag is set; then the idle state.
 */
void EmRathalos_Start(Em *em)
{
    EmRathalos_Init(em);
    switch (gQuest.stage) {
    case QUEST_ID_RATHALOS_SCENE_1:
    case QUEST_ID_RATHALOS_SCENE_2:
        if (em->type == EM_KIND_RATHALOS || em->type == EM_KIND_SILVER_RATHALOS || em->type == EM_KIND_AZURE_RATHALOS) {
            if (Save_TestEventFlag(EVENT_FLAG_RATHALOS_SCENE) == 0) {
                em->area = Area_IdForCurrentTime(AREA_FOREST_HILLS_3);
                em->posX = 20000.0f;
                em->posY = 2000.0f;
                em->posZ = 12000.0f;
            }
        }
        break;
    case QUEST_ID_RATHIAN_SCENE_1:
    case QUEST_ID_RATHIAN_SCENE_2:
        if (em->type == EM_KIND_RATHIAN || em->type == EM_KIND_GOLD_RATHIAN || em->type == EM_KIND_PINK_RATHIAN) {
            if (Save_TestEventFlag(EVENT_FLAG_RATHIAN_SCENE) == 0) {
                em->area = Area_IdForCurrentTime(AREA_JUNGLE_3);
                em->posX = 14350.0f;
                em->posY = 0.0f;
                em->posZ = 9830.0f;
            }
        }
        break;
    }
    EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE, 0);
}

/* Sets the monster up again (Init) without changing its state. */
void EmRathalos_Reinit(Em *em)
{
    EmRathalos_Init(em);
}

/*
 * Ends a ground or air state: asks the script for the next one
 * (scriptPending) and goes back to RATH_IDLE (RATH_IDLE_ALERT when toAlert
 * is 1), or to the hover when in the air.
 */
void EmRathalos_EndState(Em *em, s16 toAlert, s32 unused)
{
    em->scriptPending = 1;
    if (em->stance == EM_STANCE_GROUND) {
        if (toAlert == 1) {
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE_ALERT, 0);
            return;
        }
        EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE, 0);
        return;
    }
    EmRathalos_ChangeState(em, EM_GROUP_FLY, RATH_FLY_HOVER, 0);
}

/*
 * Ends an air state with a short flight: hover once (next 0) or glide once
 * (next 1), the script picking what follows.
 */
void EmRathalos_EndAirState(Em *em, u8 next)
{
    s32 k;

    k = next & 0xFF;
    em->scriptPending = 1;
    em->motionSpeed = 1.0f;
    switch (k) {
    case 0:
        EmRathalos_ChangeState(em, EM_GROUP_FLY, RATH_FLY_HOVER_ONCE, 0);
        break;
    case 1:
        EmRathalos_ChangeState(em, EM_GROUP_FLY, RATH_FLY_GLIDE_ONCE, 0);
        break;
    }
}

/*
 * Restarts a motion layer's idle loop (layer 0, 1 or 2) once its motion has
 * ended.
 */
void EmRathalos_RestartLayerMotion(Em *em, s32 layer)
{
    if (em->motion[layer].playing == 0) {
        switch (layer) {
        case 0:
            Em_SetLayerMotion(em, EM_MOTION_ID(0, RATH_MOT_IDLE), 10, 0, 0);
            break;
        case 1:
            Em_SetLayerMotion(em, EM_MOTION_ID(1, RATH_MOT_IDLE), 10, 0, 1);
            break;
        case 2:
            Em_SetLayerMotion(em, EM_MOTION_ID(2, RATH_MOT_IDLE), 10, 0, 2);
            break;
        }
    }
}

/*
 * Puts a layer back to its idle loop when it plays one of the two layer
 * motions that end in it.
 */
void EmRathalos_LayerMotionToIdle(Em *em, s32 layer)
{
    switch (layer) {
    case 0:
        if (em->motionId[0] == EM_MOTION_ID(0, 0x1D)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(0, RATH_MOT_IDLE), 0xA, 0, 0);
        }
        if (em->motionId[0] == EM_MOTION_ID(0, RATH_MOT_ALERT_IDLE)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(0, RATH_MOT_IDLE), 0xA, 0, 0);
        }
        break;
    case 1:
        if (em->motionId[1] == EM_MOTION_ID(1, 0x1D)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(1, RATH_MOT_IDLE), 0xA, 0, 1);
        }
        if (em->motionId[1] == EM_MOTION_ID(1, RATH_MOT_ALERT_IDLE)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(1, RATH_MOT_IDLE), 0xA, 0, 1);
        }
        break;
    case 2:
        if (em->motionId[2] == EM_MOTION_ID(2, 0x1D)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(2, RATH_MOT_IDLE), 0xA, 0, 2);
        }
        if (em->motionId[2] == EM_MOTION_ID(2, RATH_MOT_ALERT_IDLE)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(2, RATH_MOT_IDLE), 0xA, 0, 2);
        }
        break;
    }
}

/*
 * RATH_IDLE_STAND: keeps the three layers on their idle loops until the
 * timer runs out, then asks the script for the next state.
 */
void EmRathalos_Stand(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        if (em->motionId[0] != EM_MOTION_ID(0, RATH_MOT_IDLE)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(0, RATH_MOT_IDLE), 0xA, 0, 0);
        }
        if (em->motionId[1] != EM_MOTION_ID(1, RATH_MOT_IDLE)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(1, RATH_MOT_IDLE), 0xA, 0, 1);
        }
        if (em->motionId[2] != EM_MOTION_ID(2, RATH_MOT_IDLE)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(2, RATH_MOT_IDLE), 0xA, 0, 2);
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
 * RATH_IDLE: plays motion 0x1D, then the idle loop, then ends the state;
 * the state every action falls back to on the ground.
 */
void EmRathalos_Idle(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x1D, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_IDLE, 0, 0);
        }
        break;
    case 2:
        if ((em->remote == 0) && (em->motion[0].playing == 0)) {
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * RATH_IDLE_ACTION_A: plays motion 0x17 once, the layers kept on their idle
 * loops.
 */
void EmRathalos_IdleActionA(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x17, 0, 0);
        EmRathalos_LayerMotionToIdle(em, 0);
        EmRathalos_LayerMotionToIdle(em, 1);
        break;
    case 1:
        if (em->motion[2].playing == 0) {
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
    EmRathalos_RestartLayerMotion(em, 0);
    EmRathalos_RestartLayerMotion(em, 1);
}

/* RATH_IDLE_ACTION_B: motion 0x17 with a sound at a bone. */
void EmRathalos_IdleActionB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x17, 0, 0);
        EmRathalos_LayerMotionToIdle(em, 0);
        EmRathalos_LayerMotionToIdle(em, 1);
        break;
    case 1:
        if (em->motion[2].playing == 0) {
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
    EmRathalos_RestartLayerMotion(em, 0);
    EmRathalos_RestartLayerMotion(em, 1);
    Em_PlaySeAtBoneVolOnLayerFrame(em, 0x4E, 0x17, 0x2A, 2, 0xD);
}

/*
 * RATH_IDLE_ACTION_C: the idle motion once, the layers kept on their idle
 * loops.
 */
void EmRathalos_IdleActionC(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_IDLE, 0, 0);
        EmRathalos_LayerMotionToIdle(em, 1);
        EmRathalos_LayerMotionToIdle(em, 2);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
    EmRathalos_RestartLayerMotion(em, 1);
    EmRathalos_RestartLayerMotion(em, 2);
}

/*
 * RATH_IDLE_ACTION_D: motion 0x18 with a sound, layers 0 and 2 on their
 * idle loops.
 */
void EmRathalos_IdleActionD(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x18, 0, 0);
        Em_SetLayerMotion(em, EM_MOTION_ID(0, RATH_MOT_IDLE), 0xA, 0, 0);
        Em_SetLayerMotion(em, EM_MOTION_ID(2, RATH_MOT_IDLE), 0xA, 0, 2);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
    EmRathalos_RestartLayerMotion(em, 0);
    EmRathalos_RestartLayerMotion(em, 2);
    Em_PlaySeAtBoneVolOnLayerFrame(em, 0x20, 0x2F, 0x23, 1, 0xD);
}

/* RATH_IDLE_ACTION_E: motion 0x19 with a sound. */
void EmRathalos_IdleActionE(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x19, 0, 0);
        EmRathalos_LayerMotionToIdle(em, 0);
        EmRathalos_LayerMotionToIdle(em, 2);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
    EmRathalos_RestartLayerMotion(em, 0);
    EmRathalos_RestartLayerMotion(em, 2);
    Em_PlaySeAtBoneVolOnLayerFrame(em, 0x20, 0x20, 0x23, 1, 0xD);
    Em_PlaySeAtBoneVolOnLayerFrame(em, 0xA0, 0x17, 0x23, 1, 0xD);
}

/* RATH_IDLE_ACTION_F: motion 0x1A with a sound. */
void EmRathalos_IdleActionF(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x1A, 0, 0);
        EmRathalos_LayerMotionToIdle(em, 0);
        EmRathalos_LayerMotionToIdle(em, 2);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
    EmRathalos_RestartLayerMotion(em, 0);
    EmRathalos_RestartLayerMotion(em, 2);
    Em_PlaySeAtBoneVolOnLayerFrame(em, 4, 0x2D, 0x23, 1, 0xD);
}

/*
 * RATH_IDLE_ACTION_G: motion 0x1B with a sound, and attached effect 1 at
 * frame 2.
 */
void EmRathalos_IdleActionG(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x1B, 0, 0);
        EmRathalos_LayerMotionToIdle(em, 0);
        EmRathalos_LayerMotionToIdle(em, 1);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 26.0f, 2U) != 0) {
            RathalosAttack_Create(em, 0x29);
        }
        if (EmMotion_PassedFrameScaled(em, 90.0f, 2U) != 0) {
            RathalosAttack_Create(em, 0x29);
        }
        if (em->motion[2].playing == 0) {
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
    EmRathalos_RestartLayerMotion(em, 0);
    EmRathalos_RestartLayerMotion(em, 1);
    Em_PlaySeAtBoneVolOnLayerFrame(em, 0x10, 0x16, 0x2A, 2, 0xD);
}

/* RATH_IDLE_PAIR_A: motion 0x50 or 0x51 (stateRand's bit 0) once. */
void EmRathalos_IdlePairA(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        if (em->stateRand & 1) {
            Em_SetMotion(em, 0x50, 0, 0);
            break;
        }
        Em_SetMotion(em, 0x51, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * RATH_IDLE_TURN_TO_GOAL: turns in place to the goal, 0x200 a frame, with
 * RATH_MOT_TURN_IN_PLACE.
 */
void EmRathalos_IdleTurnToGoal(Em *em)
{
    u16 aimAngle;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_TURN_IN_PLACE, 0, 0);
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        break;
    case 1:
        aimAngle = em->aimAngle;
        if ((u32) ((((aimAngle - *(u16 *)&em->rotY) & 0xFFFF) + 0x200) & 0xFFFF) < 0x400U) {
            em->rotY = (s32) aimAngle;
        }
        if (EmMotion_ReachedFrame(em, 0, 60.0f) != 0) {
            em->step++;
        }
        break;
    case 2:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

/* RATH_IDLE_ACTION_H: motion 0x1C with a sound. */
void EmRathalos_IdleActionH(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x1C, 0, 0);
        EmRathalos_LayerMotionToIdle(em, 0);
        EmRathalos_LayerMotionToIdle(em, 2);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
    EmRathalos_RestartLayerMotion(em, 0);
    EmRathalos_RestartLayerMotion(em, 2);
    Em_PlaySeAtBoneVolOnLayerFrame(em, 0x10, 0x21, 0x23, 1, 0xD);
    Em_PlaySeAtBoneVolOnLayerFrame(em, 0x72, 0x1F, 0x23, 1, 0xD);
}

/* RATH_IDLE_ACTION_I: motion 0x1D once. */
void EmRathalos_IdleActionI(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x1D, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

/* RATH_IDLE_ACTION_J: motion 0x14 with a sound. */
void EmRathalos_IdleActionJ(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x14, 0, 0);
        EmRathalos_LayerMotionToIdle(em, 0);
        EmRathalos_LayerMotionToIdle(em, 2);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
    EmRathalos_RestartLayerMotion(em, 0);
    EmRathalos_RestartLayerMotion(em, 2);
    Em_PlaySeAtBoneVolOnLayerFrame(em, 2, 0x20, 0x23, 1, 0xD);
    Em_PlaySeAtBoneVolOnLayerFrame(em, 0x32, 0x1F, 0x23, 1, 0xD);
}

/*
 * RATH_IDLE_EAT_NEED2: lowers its head and feeds, gaining hp and refilling
 * need 2's gauge (Em_AddNeed2) while it feeds; makes a timed mining point at
 * the spot.
 */
void EmRathalos_EatNeed2(Em *em)
{
    u8 kind;

    Em_SetHitReactMode(em, EM_HIT_REACT_NO_FLINCH);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_HEAD_DOWN, 0, 0);
        Em_SelectSightRow(em, 1);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 56.0f, 0U) || EmMotion_PassedFrameScaled(em, 108.0f, 0U) || EmMotion_PassedFrameScaled(em, 126.0f, 0U)
            || EmMotion_PassedFrameScaled(em, 138.0f, 0U) || EmMotion_PassedFrameScaled(em, 170.0f, 0U)) {
            Em_PlaySeAtBone(em, 0x26, 0x23);
            DustCloud_Create(em, 7, 0, 5.0f);
        }
        if ((EmMotion_PassedFrameScaled(em, 210.0f, 0U) != 0) || (EmMotion_PassedFrameScaled(em, 218.0f, 0U) != 0)) {
            DustCloud_Create(em, 7, 0, 2.5f);
        }
        if (EmMotion_PassedFrameScaled(em, 52.0f, 0U) != 0) {
            DustCloud_Create(em, 8, 0, 1.0f);
        }
        if ((EmMotion_PassedFrameScaled(em, 106.0f, 0U) != 0) || (EmMotion_PassedFrameScaled(em, 130.0f, 0U) != 0)) {
            DustCloud_Create(em, 6, 0, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 124.0f, 0U) != 0) {
            DustCloud_Create(em, 6, 1, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 170.0f, 0U) != 0) {
            DustCloud_Create(em, 0xF, 0, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 192.0f, 0U) != 0) {
            DustCloud_Create(em, 0x10, 0, 1.0f);
        }
        if ((em->motion[0].playing == 0) && ((em->step += 1, Em_SetMotion(em, RATH_MOT_FEED, 0, 0), Em_AddHp(em, (s32) (0.05f * (f32) em->hpMax)), kind = em->type, (kind == EM_KIND_RATHIAN)) || (kind == EM_KIND_GOLD_RATHIAN) || (kind == EM_KIND_PINK_RATHIAN))) {
            EmMine_CreateTimed0(em, &em->posX);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SelectSightRow(em, 0);
            Em_LoadReactionSet(em, 0);
            Em_ClearNeed2State(em);
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
    Em_AddNeed2(em, 0xDE);
}

/* RATH_IDLE_PAIR_B: motion 0x50, the idle motion, 0x51, the idle motion. */
void EmRathalos_IdlePairB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x50, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_IDLE, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].blend == 0) {
            em->step++;
            Em_SetMotion(em, 0x51, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_IDLE, 0, 0);
        }
        break;
    case 4:
        if (em->motion[0].blend == 0) {
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

/* RATH_IDLE_PAIR_C: as RATH_IDLE_PAIR_B with 0x51 first. */
void EmRathalos_IdlePairC(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x51, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_IDLE, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].blend == 0) {
            em->step++;
            Em_SetMotion(em, 0x50, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_IDLE, 0, 0);
        }
        break;
    case 4:
        if (em->motion[0].blend == 0) {
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * RATH_IDLE_ALERT: the alert idle (RATH_MOT_ALERT_IDLE) while it fights the
 * hunters.
 */
void EmRathalos_AlertIdle(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_ALERT_IDLE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * RATH_IDLE_REST: lies down and rests, regaining hp (Em_RegenHp) until
 * EM_DMG_HIT_LYING ends it (RATH_IDLE_REST_END); a hit wakes it
 * too.
 */
void EmRathalos_Rest(Em *em, EmWork *work)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_LIE_DOWN, 0, 0);
        Em_LoadReactionSet(em, 1);
        if (em->type == EM_KIND_RATHIAN || em->type == EM_KIND_GOLD_RATHIAN || em->type == EM_KIND_PINK_RATHIAN) {
            em->timer = 9000;
        } else {
            em->timer = 7200;
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_LIE, 0, 0);
        }
        break;
    case 2:
        if (em->type == EM_KIND_RATHIAN || em->type == EM_KIND_GOLD_RATHIAN || em->type == EM_KIND_PINK_RATHIAN) {
            Em_RegenHp(em, 1, (s32) (0.6f * (f32) em->hpMax), 3);
        } else {
            Em_RegenHp(em, 1, (s32) (0.7f * (f32) em->hpMax), 3);
        }
        if (--em->timer <= 0) {
            em->step++;
            Em_ClearLowHpNeed(em);
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE_REST_END, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE_REST_END, 4);
        }
        break;
    }
}

/* RATH_IDLE_EAT_START: lowers its head, then goes on with RATH_IDLE_EAT. */
void EmRathalos_EatStart(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_NO_FLINCH);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_HEAD_DOWN, 0, 0);
        Em_SelectSightRow(em, 1);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 200.0f, 0U) != 0) {
            em->step++;
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE_EAT, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE_EAT, 4);
        }
        break;
    }
}

/* RATH_IDLE_EAT: feeds once (RATH_MOT_FEED) and ends the state. */
void EmRathalos_Eat(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SelectSightRow(em, 0);
        Em_LoadReactionSet(em, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_FEED, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * RATH_IDLE_STATUS2_COLLAPSE: status 2 began: falls over and lies until it
 * ends (RATH_IDLE_STATUS2_END).
 */
void EmRathalos_Status2Collapse(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_COLLAPSE, 0, 0);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_LIE, 0, 0);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE_STATUS2_END, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE_STATUS2_END, 4);
        }
        break;
    }
}

/*
 * RATH_IDLE_STATUS2_LIE_DOWN: status 2 began: lies down and lies, regaining
 * hp, until it ends.
 */
void EmRathalos_Status2LieDown(Em *em, EmWork *work)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_LIE_DOWN, 0, 0);
        Em_LoadReactionSet(em, 1);
        if (em->type == EM_KIND_RATHIAN || em->type == EM_KIND_GOLD_RATHIAN || em->type == EM_KIND_PINK_RATHIAN) {
            em->timer = 12600;
        } else {
            em->timer = 9000;
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_LIE, 0, 0);
        }
        break;
    case 2:
        if (em->type == EM_KIND_RATHIAN || em->type == EM_KIND_GOLD_RATHIAN || em->type == EM_KIND_PINK_RATHIAN) {
            Em_RegenHp(em, 1, (s32) (0.6f * (f32) em->hpMax), 3);
        } else {
            Em_RegenHp(em, 1, (s32) (0.7f * (f32) em->hpMax), 3);
        }
        if (--em->timer <= 0) {
            em->step++;
            Em_ClearLowHpNeed(em);
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE_STATUS2_END, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE_STATUS2_END, 4);
        }
        break;
    }
}

/* RATH_IDLE_ACTION_K: motion 0x50 once. */
void EmRathalos_IdleActionK(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x50, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * RATH_IDLE_REST_END: gets up after resting (a timed mining point appears
 * where it lay), then shakes off.
 */
void EmRathalos_RestEnd(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, RATH_MOT_GET_UP, 0, 0);
        Em_LoadReactionSet(em, 0);
        em->blind = 2;
        if (em->type == EM_KIND_RATHALOS || em->type == EM_KIND_SILVER_RATHALOS || em->type == EM_KIND_AZURE_RATHALOS) {
            EmMine_CreateTimed0(em, &em->posX);
        }
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE_SHAKE_OFF, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE_SHAKE_OFF, 4);
        }
        break;
    }
}

/* RATH_IDLE_STATUS2_END: gets up after status 2, then shakes off. */
void EmRathalos_Status2End(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, RATH_MOT_GET_UP, 0, 0);
        Em_LoadReactionSet(em, 0);
        em->blind = 2;
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE_SHAKE_OFF, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE_SHAKE_OFF, 4);
        }
        break;
    }
}

/* RATH_IDLE_SHAKE_OFF: shakes off (RATH_MOT_SHAKE_OFF) and ends the state. */
void EmRathalos_ShakeOff(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, RATH_MOT_SHAKE_OFF, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * RATH_IDLE_EAT_NEED1: lowers its head and feeds, gaining hp and refilling
 * need 1's gauge (Em_AddNeed1).
 */
void EmRathalos_EatNeed1(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_NO_FLINCH);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_HEAD_DOWN, 0, 0);
        Em_SelectSightRow(em, 1);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_FEED, 0, 0);
            Em_AddHp(em, (s32) (0.05f * (f32) em->hpMax));
            Em_AddNeed1(em, em->need1Max);
            Em_ClearNeed1State(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SelectSightRow(em, 0);
            Em_LoadReactionSet(em, 0);
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * RATH_IDLE_STATUS1_COLLAPSE: status 1 began: falls over and lies until it
 * ends (RATH_IDLE_STATUS1_END).
 */
void EmRathalos_Status1Collapse(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_COLLAPSE, 0, 0);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_LIE, 0, 0);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE_STATUS1_END, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE_STATUS1_END, 4);
        }
        break;
    }
}

/* RATH_IDLE_STATUS1_END: gets up after status 1, then shakes off. */
void EmRathalos_Status1End(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, RATH_MOT_GET_UP, 0, 0);
        Em_LoadReactionSet(em, 0);
        em->blind = 2;
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE_SHAKE_OFF, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE_SHAKE_OFF, 4);
        }
        break;
    }
}

/*
 * RATH_IDLE_STATUS2_DOWNED: status 2 in a hold: lies downed, with the
 * periodic effect at bone 0x22, until it ends.
 */
void EmRathalos_Status2Downed(Em *em, EmWork *work)
{
    f32 pos[3];

    em->holdTimer = 5;
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_DOWNED, 0, 0);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_STATUS2_DOWNED_END, 3);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_STATUS2_DOWNED_END, 3);
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

/* RATH_IDLE_STATUS1_DOWNED: status 1 in a hold, as RATH_IDLE_STATUS2_DOWNED. */
void EmRathalos_Status1Downed(Em *em, EmWork *work)
{
    f32 pos[3];

    em->holdTimer = 5;
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_DOWNED, 0, 0);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_STATUS1_DOWNED_END, 3);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_STATUS1_DOWNED_END, 3);
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
 * The shake-off after status 4, the stun or the second hold (variant 0, 1,
 * 2: RATH_IDLE_STATUS4_SHAKE_OFF to RATH_IDLE_HOLD2_SHAKE_OFF).
 */
void EmRathalos_ShakeOffAfterStatus(Em *em, EmWork *work, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, RATH_MOT_SHAKE_OFF, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

/* RATH_MOVE_WALK: walks to the goal (Em_StepToGoal) and ends there. */
void EmRathalos_Walk(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_WALK, 0, 0);
        break;
    case 1:
        if (Em_StepToGoal(em, 0x40, 0) == 1) {
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

/* RATH_MOVE_END: ends the state. */
void EmRathalos_MoveEnd(Em *em, EmWork *work)
{
    EmRathalos_EndState(em, 0, 0);
}

/* RATH_MOVE_END2: ends the state. */
void EmRathalos_MoveEnd2(Em *em, EmWork *work)
{
    EmRathalos_EndState(em, 0, 0);
}

/*
 * RATH_MOVE_TURN: turns to the target with the family's turn parameters
 * (Em_StartTurnToTarget, Em_TurnStep).
 */
void EmRathalos_Turn(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_StartTurnToTarget(em, &EmRathalos_TurnParams[0]);
        break;
    case 1:
        if (Em_TurnStep(em, &EmRathalos_TurnParams[0]) == 1) {
            em->step++;
            Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

/* RATH_MOVE_RUN (and RATH_MOVE_RUN_SLOW): runs to the goal. */
void EmRathalos_Run(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_RUN, 0, 0);
        break;
    case 1:
        if (Em_StepToGoal(em, 0x40, 0) == 1) {
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

/* RATH_MOVE_TURN_IN_COMBAT: the turn to the target while it fights. */
void EmRathalos_TurnInCombat(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_StartTurnToTarget(em, &EmRathalos_TurnParams[1]);
        break;
    case 1:
        if (Em_TurnStep(em, &EmRathalos_TurnParams[1]) == 1) {
            em->step++;
            Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

/* RATH_MOVE_LIMP: limps to the goal (low hp). */
void EmRathalos_Limp(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_LIMP, 0, 0);
        break;
    case 1:
        if (Em_StepToGoal(em, 0x40, 1) == 1) {
            em->timer = 1;
        }
        if (--em->timer <= 0) {
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * RATH_MOVE_TURN_TO_GOAL (and the two rage turns): turns in place to the
 * goal, 0x200 a frame, then ends the state toward the alert idle.
 */
void EmRathalos_TurnToGoal(Em *em, EmWork *work)
{
    u16 aimAngle;
    u32 diff;
    s32 rotY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_TURN_IN_PLACE, 0, 0);
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        break;
    case 1:
        rotY = em->rotY;
        aimAngle = em->aimAngle;
        diff = (aimAngle - (rotY & 0xFFFF)) & 0xFFFF;
        if ((u32) ((diff + 0x200) & 0xFFFF) < 0x400U) {
            em->rotY = (s32) aimAngle;
        } else if (diff < 0x8000U) {
            em->rotY = (rotY + 0x200) & 0xFFFF;
        } else {
            em->rotY = (rotY - 0x200) & 0xFFFF;
        }
        if (EmMotion_ReachedFrame(em, 0, 60.0f) != 0) {
            em->step++;
        }
        break;
    case 2:
        if (em->motion[1].playing == 0) {
            EmRathalos_EndState(em, 1, 0);
        }
        break;
    }
}

/*
 * RATH_FLY_TAKE_OFF: takes off (airborne from frame 38) along the family's
 * take-off curve (0xF for the Rathalos kinds, 2 for the Rathian kinds),
 * then picks a short flight.
 */
void EmRathalos_TakeOff(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_TAKE_OFF, 0, 0);
        if (em->type == EM_KIND_RATHALOS || em->type == EM_KIND_SILVER_RATHALOS || em->type == EM_KIND_AZURE_RATHALOS) {
            EmRathalos_StartCurves(em, 0xF);
        } else {
            EmRathalos_StartCurves(em, 2);
        }
        work->flyGuard = 0;
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 38.0f, 0U) != 0) {
            em->step++;
            em->stance = EM_STANCE_AIR;
            EmRathalos_UpdateCurves(em);
        }
        break;
    case 2:
        if (EmRathalos_UpdateCurves(em)) {
            em->step++;
            EmRathalos_EndAirState(em, 0U);
        }
        break;
    }
}

/*
 * RATH_FLY_LAND: descends from the hover (velY eased toward the floor),
 * then lands with a camera shake.
 */
void EmRathalos_Land(Em *em, EmWork *work)
{
    f32 dropLeft;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        em->rotX = 0;
        em->rotZ = 0;
        Em_SetMotion(em, RATH_MOT_HOVER, 0, 0);
        Player_ClearVelocity(em);
        em->velY = -10.0f;
        dropLeft = em->floorY;
        dropLeft = em->posY - (1000.0f + dropLeft);
        if (dropLeft > 0.0f) {
            do {
                em->velY -= 0.75f;
                dropLeft += em->velY;
            } while (dropLeft > 0.0f && !(em->velY < -200.0f));
        }
        work->flyGuard = 0;
        work->moveAngX = 0;
        work->moveAngZ = 0;
        break;
    case 1:
        work->moveAngY = em->rotY;
        Em_MoveRotated(em, &work->moveAngX);
        if (em->velY < -10.0f) {
            em->velY += 0.75f;
        }
        if (!((630.0f + em->floorY) <= em->posY) && ((EmMotion_PassedFrameScaled(em, 10.0f, 0U) != 0) || (EmMotion_PassedFrameScaled(em, 86.0f, 0U) != 0) || (EmMotion_PassedFrameScaled(em, 160.0f, 0U) != 0))) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_DESCEND, 0, 0);
            em->velY = (em->floorY - em->posY) / 30.0f;
            if (!(em->velY < 0.0f)) {
                em->velY = -10.0f;
            }
            em->timer = 0x1E;
        }
        break;
    case 2:
        work->moveAngX = 0;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
        if (--em->timer < 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_TOUCH_DOWN, 0, 0);
            em->posY = em->floorY;
            em->rotX = 0;
            em->rotZ = 0;
            em->stance = EM_STANCE_GROUND;
            CamShake_ReqAIf(em, 1);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
    dropLeft = em->floorY;
    if (em->posY < dropLeft) {
        em->posY = dropLeft;
    }
}

/*
 * RATH_FLY_HOVER: hovers, turning to the goal, until the timer runs out,
 * then lands; at timer 300 it asks for a short flight instead.
 */
void EmRathalos_Hover(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        work->turnSpeed = 0x100;
        Em_SetMotion(em, RATH_MOT_HOVER, 0, 0);
        work->flyGuard = 0;
        break;
    case 1:
        EmRathalos_MoveByVelYCurve(em, 1U);
        if (--em->timer <= 0) {
            em->step++;
            EmRathalos_ChangeState(em, EM_GROUP_FLY, RATH_FLY_LAND, 1);
        }
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 1);
        if (em->timer == 0x12C) {
            EmRathalos_EndAirState(em, 0U);
        }
        break;
    case 2:
        EmRathalos_MoveByVelYCurve(em, 1U);
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 1);
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * RATH_FLY_HOP_TO_GOAL: takes off turning to the goal, flies the curve, and
 * lands; the landing keeps fxMotion's after-landing count (150).
 */
void EmRathalos_HopToGoal(Em *em, EmWork *work)
{
    f32 velY;
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_TAKE_OFF, 0, 0);
        Player_ClearVelocity(em);
        if (em->type == EM_KIND_RATHALOS || em->type == EM_KIND_SILVER_RATHALOS || em->type == EM_KIND_AZURE_RATHALOS) {
            EmRathalos_StartCurves(em, 0xF);
        } else {
            EmRathalos_StartCurves(em, 2);
        }
        work->flyGuard = 0;
        break;
    case 1:
        if ((EmMotion_ReachedFrame(em, 0, 50.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 114.0f) == 0)) {
            Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 1);
        }
        if (EmMotion_ReachedFrame(em, 0, 30.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            EmRathalos_UpdateCurves(em);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
        }
        break;
    case 2:
        if ((EmRathalos_UpdateCurves(em) & 0xFF) && (em->posY <= (800.0f + em->floorY))) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_DESCEND, 0, 0);
            Player_ClearVelocity(em);
            velY = (em->floorY - em->posY) / 30.0f;
            em->velY = velY;
            if (!(velY < 0.0f)) {
                em->velY = -10.0f;
            }
        }
        break;
    case 3:
        work->moveAngX = 0;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
        if (em->posY <= em->floorY) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_TOUCH_DOWN, 0, 0);
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            CamShake_ReqAIf(em, 2);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            work->fxMotion = 0x96;
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * RATH_FLY_HOP_FORWARD: a short hop forward (velZ 20) along curve 3,
 * landing again.
 */
void EmRathalos_HopForward(Em *em, EmWork *work)
{
    f32 floorY;
    s32 ended;

    ended = 0;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_TAKE_OFF, 0, 0);
        EmRathalos_StartCurves(em, 3);
        em->velZ = 20.0f;
        work->flyGuard = 0;
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 30.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            ended = EmRathalos_UpdateCurves(em) & 0xFF;
        }
        if ((ended != 0) && (em->posY <= em->floorY)) {
            Em_SetMotion(em, RATH_MOT_TOUCH_DOWN, 0, 0);
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            CamShake_ReqAIf(em, 2);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            work->fxMotion = 0x96;
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/* RATH_FLY_HOP_BACK: a short hop backward (velZ -20). */
void EmRathalos_HopBack(Em *em, EmWork *work)
{
    f32 floorY;
    s32 ended;

    ended = 0;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_TAKE_OFF, 0, 0);
        EmRathalos_StartCurves(em, 3);
        em->velZ = -20.0f;
        work->flyGuard = 0;
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 30.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            ended = EmRathalos_UpdateCurves(em) & 0xFF;
        }
        if ((ended != 0) && (em->posY <= em->floorY)) {
            Em_SetMotion(em, RATH_MOT_TOUCH_DOWN, 0, 0);
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            CamShake_ReqAIf(em, 2);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            work->fxMotion = 0x96;
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * RATH_FLY_ROUTE: flies through the goal route (legsLeft legs, the goal
 * number stepping 0-3), then glides once.
 */
void EmRathalos_FlyRoute(Em *em, EmWork *work)
{
    f32 floorY;
    s32 timerLeft;
    u8 n;
    u8 stateVar1;

    switch (em->step) {
    case 0:
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, RATH_MOT_GLIDE, 0, 0);
        em->step++;
        em->stateVar1 = 0;
        em->lookAtTarget = 0;
        em->velX = 0.0f;
        em->velY = 0.0f;
        Player_ClearAccel2(em);
        if (em->velZ <= 50.0f) {
            em->velZ = 50.0f;
        }
        work->flyGuard = 1;
        break;
    case 1:
        stateVar1 = em->stateVar1;
        if ((stateVar1 == 0) && (em->motion[0].playing == 0)) {
            em->stateVar1 = stateVar1 + 1;
            Em_SetMotion(em, RATH_MOT_FLY, 0, 0);
        }
        if ((Vec_DistanceXZ(&em->posX, em->goal) <= 3000.0f) || (timerLeft = em->timer - 1, em->timer = timerLeft, (timerLeft <= 0))) {
            n = ((EmFlyerWork *)work)->legsLeft - 1;
            ((EmFlyerWork *)work)->legsLeft = n;
            if ((n & 0xFF) <= 0) {
                EmRathalos_EndAirState(em, 1U);
            } else {
                em->curGoalNo++;
                em->curGoalNo &= 3;
                Em_ComputeGoal2(em, em->goal);
                em->timer = (s32) ((1.5f * Vec_DistanceXZ(&em->posX, em->goal)) / 50.0f);
                Em_SetStateRand(em, EM_GROUP_FLY, RATH_FLY_ROUTE);
            }
        }
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 0xE1, &EmRathalos_FlyParam);
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
 * RATH_FLY_HOVER_TO_GOAL: hovers toward the goal, pitching at it, until it
 * is near or the timer runs out.
 */
void EmRathalos_HoverToGoal(Em *em, EmWork *work)
{
    f32 dist;
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        work->turnSpeed = 0x100;
        Em_SetMotion(em, RATH_MOT_HOVER, 0, 0);
        work->flyGuard = 0;
        break;
    case 1:
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 1);
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_GetPitchToGoal(em, &work->moveAngX, 0.0f, 0.0f);
        Em_MoveRotated(em, &work->moveAngX);
        EmRathalos_MoveByVelYCurve(em, 1U);
        dist = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer <= 0 || dist <= 10.0f * em->velZ) {
            em->step++;
            Player_ClearVelocity(em);
            EmRathalos_EndAirState(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void Hud_TriggerGlow(Em *em);
/*
 * RATH_FLY_OUT_OF_AREA: rises, flies to the area's exit point and out; then
 * flies to the goal in the next area (RATH_FLY_INTO_AREA) or, when it
 * stays, to a landing point (RATH_FLY_TO_GOAL_AND_LAND).
 */
void EmRathalos_FlyOutOfArea(Em *em, EmWork *work)
{
    u16 next;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, RATH_MOT_HOVER, 0, 0);
        work->flyGuard = 0;
        work->moveAngX = 0;
        work->moveAngZ = 0;
        Player_ClearVelocity(em);
        em->velY = 20.0f;
        Em_AreaExitPoint(em, (f32 *) em->goal);
        break;
    case 1:
        Em_MoveRotated(em, &em->rotX);
        if (em->posY > 500.0f + em->floorY && em->velY < 100.0f) {
            em->velY += 0.75f;
        }
        if (!(em->posY < em->goal[1])) {
            em->step++;
            em->stance = EM_STANCE_AIR;
            em->lookAtTarget = 0;
            Player_ClearVelocity(em);
            em->velZ = 50.0f;
            Em_SetMotion(em, RATH_MOT_GLIDE, 0, 0);
            work->flyGuard = 1;
        }
        break;
    case 2:
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 0x4E1, &EmRathalos_FlyParam);
        work->moveAngX = em->rotX;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
        if (Area_ContainsPos(em->area, &em->posX) == 0) {
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
        next = *(u16 *)&em->destArea;
        if (em->nextArea == next || next == AREA_NONE) {
            Hud_TriggerGlow(em);
            EmRathalos_ChangeState(em, EM_GROUP_FLY, RATH_FLY_TO_GOAL_AND_LAND, 1);
        } else {
            if (em->remote == 0) {
                em->nextArea = next;
                em->goalNo = next;
                em->goalKind = 3;
                Em_ComputeGoal(em, em->goal);
            }
            EmRathalos_ChangeState(em, EM_GROUP_FLY, RATH_FLY_INTO_AREA, 1);
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

/* RATH_FLY_TO_GOAL_AND_LAND: flies to the goal, then lands. */
void EmRathalos_FlyToGoalAndLand(Em *em, EmWork *work)
{
    f32 goalDist;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        work->turnSpeed = 0x100;
        Em_SetMotion(em, RATH_MOT_HOVER, 0, 0);
        Player_ClearVelocity(em);
        em->velZ = 80.0f;
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->rotY = (s32) em->aimAngle;
        *(u16 *)&em->destArea = AREA_NONE;
        work->flyGuard = 1;
        break;
    case 1:
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 0xE5, &EmRathalos_FlyParam);
        work->moveAngX = em->rotX;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
        goalDist = Vec_DistanceXZ(&em->posX, em->goal);
        em->timer--;
        if (goalDist <= 1000.0f || em->timer < 0) {
            em->step++;
            em->timer = 600;
            EmRathalos_ChangeState(em, EM_GROUP_FLY, RATH_FLY_LAND, 1);
        }
        break;
    case 2:
        if (em->remote == 0) {
            em->timer = 0x258;
            EmRathalos_ChangeState(em, EM_GROUP_FLY, RATH_FLY_LAND, 1);
        }
        break;
    }
    if (em->posY < em->floorY) {
        em->posY = em->floorY;
    }
}

/*
 * RATH_FLY_OUT_OF_AREA_ALT: as RATH_FLY_OUT_OF_AREA, but once it has
 * entered the next area (Em_EnterNextArea) the area id is overwritten:
 * AREA_JUNGLE_NIGHT_6 if the area entered is AREA_SNOWY_MOUNTAINS_NIGHT_9,
 * else AREA_SNOWY_MOUNTAINS_NIGHT_9. Seen: no code in this file enters
 * this state.
 */
void EmRathalos_FlyOutOfAreaAlt(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        work->turnSpeed = 0x100;
        Em_SetMotion(em, RATH_MOT_HOVER, 0, 0);
        em->stance = EM_STANCE_AIR;
        Player_ClearVelocity(em);
        Em_AreaExitPoint(em, (f32 *) em->goal);
        work->flyGuard = 1;
        break;
    case 1:
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 1);
        EmRathalos_MoveByVelYCurve(em, 1U);
        em->posY += 100.0f;
        if (!(em->posY < em->goal[1])) {
            em->step++;
        }
        break;
    case 2:
        em->step++;
        em->velZ = 50.0f;
        Em_SetMotion(em, RATH_MOT_GLIDE, 0, 0);
        
    case 3:
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 0x4E1, &EmRathalos_FlyParam);
        work->moveAngX = em->rotX;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
        if (Area_ContainsPos((s32) em->area, &em->posX) == 0) {
            em->step++;
        }
        break;
    case 4:
        em->step++;
        Em_EnterNextArea(em);
        Hud_TriggerGlow(em);
        if (em->area == AREA_SNOWY_MOUNTAINS_NIGHT_9) {
            em->area = AREA_JUNGLE_NIGHT_6;
        } else {
            em->area = AREA_SNOWY_MOUNTAINS_NIGHT_9;
        }
        EmRathalos_EndAirState(em, 1U);
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/* RATH_FLY_GLIDE_TO_GOAL: glides to the goal (at most velZ 100), then lands. */
void EmRathalos_GlideToGoal(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_GLIDE, 0, 0);
        if (!(em->velZ <= 100.0f)) {
            em->velZ = 100.0f;
        }
        work->flyGuard = 1;
        break;
    case 1:
        if (Vec_DistanceXZ(&em->posX, em->goal) <= 1000.0f) {
            if (!(em->velZ <= 50.0f)) {
                em->velZ = 50.0f;
            }
            em->step++;
            em->timer = 0x12C;
            EmRathalos_ChangeState(em, EM_GROUP_FLY, RATH_FLY_LAND, 1);
        } else {
            em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
            em->aimAngle = em->aimAngle - em->rotY;
            Em_UpdateFlight(em, 0xE1, &EmRathalos_FlyParam);
            work->moveAngX = em->rotX;
            work->moveAngY = em->rotY;
            work->moveAngZ = 0;
            Em_MoveRotated(em, &work->moveAngX);
            if (--em->timer <= 0) {
                em->step++;
                em->timer = 0x12C;
                EmRathalos_ChangeState(em, EM_GROUP_FLY, RATH_FLY_LAND, 1);
            }
        }
        break;
    case 2:
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 0xE1, &EmRathalos_FlyParam);
        work->moveAngX = em->rotX;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
        if (--em->timer <= 0) {
            em->timer = 0x12C;
            if (em->remote == 0) {
                EmRathalos_ChangeState(em, EM_GROUP_FLY, RATH_FLY_LAND, 1);
            }
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * RATH_FLY_BACK_HOP: hops backward (airborne from frame 76) and lands with
 * ground effects and a camera shake.
 */
void EmRathalos_BackHop(Em *em)
{
    f32 floorY;
    s32 ended;

    ended = 0;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_BACK_HOP, 0, 0);
        EmRathalos_StartCurves(em, 0xE);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 76.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            ended = EmRathalos_UpdateCurves(em) & 0xFF;
        }
        if ((ended != 0) && (em->posY <= em->floorY)) {
            Em_SetMotion(em, RATH_MOT_BACK_HOP_LAND, 0, 0);
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            EmRathalos_SpawnLandingDust(em);
            CamShake_ReqAIf(em, 2);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * RATH_FLY_INTO_AREA: flies into the new area toward its entry point (or
 * its middle), then to the exit point and on, as RATH_FLY_OUT_OF_AREA.
 */
void EmRathalos_FlyIntoArea(Em *em, EmWork *work)
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
        Em_SetMotion(em, RATH_MOT_HOVER, 0, 0);
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
        Em_UpdateFlight(em, 0xE5, &EmRathalos_FlyParam);
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
        Em_UpdateFlight(em, 0xE1, &EmRathalos_FlyParam);
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
            EmRathalos_ChangeState(em, EM_GROUP_FLY, RATH_FLY_TO_GOAL_AND_LAND, 1);
        } else {
            if (em->remote == 0) {
                em->nextArea = next;
                em->goalNo = next;
                em->goalKind = 3;
                Em_ComputeGoal(em, em->goal);
            }
            EmRathalos_ChangeState(em, EM_GROUP_FLY, RATH_FLY_INTO_AREA, 1);
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

/* RATH_FLY_HOVER_ONCE: hovers for one motion, then picks the next air state. */
void EmRathalos_HoverOnce(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, RATH_MOT_HOVER, 0, 0);
        work->flyGuard = 0;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmRathalos_EndAirState(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/* RATH_FLY_GLIDE_ONCE: glides for one motion, then picks the next air state. */
void EmRathalos_GlideOnce(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, RATH_MOT_GLIDE, 0, 0);
        Player_ClearAccel2(em);
        em->velY = 0.0f;
        em->velX = 0.0f;
        work->flyGuard = 1;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmRathalos_EndAirState(em, 1U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/* RATH_FLY_HOVER_TURN: hovers turning to the goal (150 frames at most). */
void EmRathalos_HoverTurn(Em *em, EmWork *work)
{
    f32 floorY;
    s32 done;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, RATH_MOT_HOVER, 0, 0);
        work->turnSpeed = 0x180;
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        work->flyGuard = 0;
        em->timer = 0x96;
        break;
    case 1:
        EmRathalos_MoveByVelYCurve(em, 1U);
        done = Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 1) & 0xFF;
        if (--em->timer <= 0 || done != 0) {
            em->step++;
            EmRathalos_EndAirState(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * RATH_FLY_ROUTE_B: flies the goal route as RATH_FLY_ROUTE, keeping a glide
 * already running.
 */
void EmRathalos_FlyRouteB(Em *em, EmWork *work)
{
    f32 floorY;
    u16 motionId;
    u8 stateVar1;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        em->velX = 0.0f;
        em->velY = 0.0f;
        Player_ClearAccel2(em);
        if (em->velZ < 50.0f) {
            em->stateVar1 = 0;
            em->velZ = 50.0f;
            Em_SetMotion(em, RATH_MOT_GLIDE, 0, 0);
        } else {
            em->stateVar1 = 1;
            motionId = em->motionId[0];
            if ((motionId != EM_MOTION_ID(0, RATH_MOT_GLIDE)) && (motionId != EM_MOTION_ID(0, RATH_MOT_FLY))) {
                Em_SetMotion(em, RATH_MOT_GLIDE, 0, 0);
            }
        }
        work->flyGuard = 1;
        break;
    case 1:
        stateVar1 = em->stateVar1;
        if ((stateVar1 == 0) && (em->motion[0].playing == 0)) {
            em->stateVar1 = stateVar1 + 1;
            Em_SetMotion(em, RATH_MOT_FLY, 0, 0);
        }
        if (Vec_DistanceXZ(&em->posX, em->goal) <= 1000.0f || --em->timer <= 0) {
            em->curGoalNo++;
            em->curGoalNo &= 3;
            Em_ComputeGoal2(em, em->goal);
            em->timer = (s32) ((1.5f * Vec_DistanceXZ(&em->posX, em->goal)) / 50.0f) + 0x96;
            em->step++;
            Em_SetStateRand(em, EM_GROUP_FLY, RATH_FLY_ROUTE_B);
        }
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 0x321, &EmRathalos_FlyParam2);
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
 * RATH_FLY_LEVEL_OUT_AND_LAND: levels pitch and roll, hovers for 300
 * frames, then lands.
 */
void EmRathalos_LevelOutAndLand(Em *em, EmWork *work)
{
    f32 floorY;
    s32 rotX;
    s32 rotZ;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_LEVEL_OUT, 0, 0);
        EmRathalos_StartCurves(em, 9);
        work->flyGuard = 0;
        break;
    case 1:
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
        if (EmRathalos_UpdateCurves(em) & 0xFF) {
            em->step++;
            Player_ClearVelocity(em);
            Em_SetMotion(em, RATH_MOT_HOVER, 0, 0);
            em->timer = 0x12C;
            EmRathalos_ChangeState(em, EM_GROUP_FLY, RATH_FLY_LAND, 1);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->timer = 0x12C;
            Em_SetMotion(em, RATH_MOT_HOVER, 0, 0);
            if (em->remote == 0) {
                EmRathalos_ChangeState(em, EM_GROUP_FLY, RATH_FLY_LAND, 1);
            }
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * RATH_FLY_GLIDE_TO_GOAL_B: glides to the goal with the second flight
 * parameters (EmRathalos_FlyParam2), then glides once.
 */
void EmRathalos_GlideToGoalB(Em *em, EmWork *work)
{
    f32 floorY;
    u16 motionId;
    u8 stateVar1;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        em->velX = 0.0f;
        em->velY = 0.0f;
        Player_ClearAccel2(em);
        if (em->velZ < 50.0f) {
            em->stateVar1 = 0;
            em->velZ = 50.0f;
            Em_SetMotion(em, RATH_MOT_GLIDE, 0, 0);
        } else {
            em->stateVar1 = 1;
            motionId = em->motionId[0];
            if ((motionId != EM_MOTION_ID(0, RATH_MOT_GLIDE)) && (motionId != EM_MOTION_ID(0, RATH_MOT_FLY))) {
                Em_SetMotion(em, RATH_MOT_GLIDE, 0, 0);
            }
        }
        work->flyGuard = 1;
        break;
    case 1:
        stateVar1 = em->stateVar1;
        if ((stateVar1 == 0) && (em->motion[0].playing == 0)) {
            em->stateVar1 = stateVar1 + 1;
            Em_SetMotion(em, RATH_MOT_FLY, 0, 0);
        }
        if (Vec_DistanceXZ(&em->posX, em->goal) <= 1000.0f || --em->timer <= 0) {
            em->step++;
            EmRathalos_EndAirState(em, 1U);
        }
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 0x121, &EmRathalos_FlyParam2);
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

/* RATH_FLY_END: ends the state. */
void EmRathalos_EndFlight(Em *em)
{
    EmRathalos_EndState(em, 0, 0);
}

/*
 * RATH_FLY_HOVER_STRAIGHT_TO_GOAL: hovers straight at the goal in three
 * dimensions.
 */
void EmRathalos_HoverStraightToGoal(Em *em, EmWork *work)
{
    f32 dist;
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        work->turnSpeed = 0x100;
        Em_SetMotion(em, RATH_MOT_HOVER, 0, 0);
        work->flyGuard = 0;
        break;
    case 1:
        work->moveAngY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        work->moveAngZ = 0;
        Em_GetPitchToGoal(em, &work->moveAngX, 0.0f, 0.0f);
        Em_MoveRotated(em, &work->moveAngX);
        dist = Vec_Distance((f32 *) &em->posX, (f32 *) em->goal);
        if (--em->timer <= 0 || dist <= 10.0f * em->velZ) {
            em->step++;
            Player_ClearVelocity(em);
            EmRathalos_EndAirState(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * RATH_FLY_LEVEL_OUT: levels pitch and roll along curve 9, then picks the
 * next air state.
 */
void EmRathalos_LevelOut(Em *em, EmWork *work)
{
    f32 floorY;
    s32 rotX;
    s32 rotZ;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_LEVEL_OUT, 0, 0);
        EmRathalos_StartCurves(em, 9);
        work->flyGuard = 0;
        break;
    case 1:
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
        if (EmRathalos_UpdateCurves(em) & 0xFF) {
            em->step++;
            Player_ClearVelocity(em);
            EmRathalos_EndAirState(em, 0U);
        }
        break;
    case 2:
        EmRathalos_UpdateCurves(em);
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * RATH_FLY_ESCAPE_UPWARD: breaks out of a trap: takes off (holdTimer
 * cleared, no reactions), then lands.
 */
void EmRathalos_EscapeUpward(Em *em, EmWork *work)
{
    f32 floorY;
    f32 velY;
    f32 y;

    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, RATH_MOT_TAKE_OFF, 0xA, 0x1E);
        EmRathalos_StartCurves(em, 2);
        em->stance = EM_STANCE_AIR;
        em->holdTimer = 0;
        em->trapHit = 0;
        em->noReact = 1;
        break;
    case 1:
        EmRathalos_UpdateCurves(em);
        if (em->motion[0].playing == 0) {
            em->step++;
        }
        break;
    case 2:
        if ((EmRathalos_UpdateCurves(em) & 0xFF) && (em->posY <= (800.0f + em->floorY))) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_DESCEND, 0, 0);
            Player_ClearVelocity(em);
            floorY = em->floorY;
            if (em->posY < floorY) {
                em->posY = floorY;
            }
            velY = (em->floorY - em->posY) / 30.0f;
            em->velY = velY;
            if (!(velY < 0.0f)) {
                em->velY = -10.0f;
            }
        }
        break;
    case 3:
        work->moveAngX = 0;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
        if (em->posY <= em->floorY) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_TOUCH_DOWN, 0, 0);
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            CamShake_ReqAIf(em, 2);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            work->fxMotion = 0x96;
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
    y = em->floorY;
    if (em->posY < y) {
        em->posY = y;
    }
}

/* RATH_FLY_HOVER_RISE: hovers upward to 1000 above the goal. */
void EmRathalos_HoverRise(Em *em, EmWork *work)
{
    f32 pos[3];
    f32 floorY;
    s32 timer;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, RATH_MOT_HOVER, 0, 0);
        work->flyGuard = 0;
        work->moveAngX = 0;
        work->moveAngZ = 0;
        Player_ClearVelocity(em);
        em->velY = 20.0f;
        break;
    case 1:
        work->moveAngY = Math_AngleXZ(&em->posX, pos) & 0xFFFF;
        Em_MoveRotated(em, &work->moveAngX);
        timer = em->timer - 1;
        em->timer = timer;
        if ((timer <= 0) || !(em->posY <= (1000.0f + em->goal[1]))) {
            em->step++;
            Player_ClearVelocity(em);
            EmRathalos_EndAirState(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/* RATH_ATK_TURN_AND_STARE: turns to the target, then plays motion 0x24. */
void EmRathalos_TurnAndStare(Em *em)
{
    switch (em->step) {
    case 0:
        if (Em_TurnToTargetMotion(em, 0xE38, 3, 6, 5) != 0) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->lookAtTarget = 0;
            Em_SetMotion(em, 0x24, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * RATH_ATK_LEAP: turns to the target, leaps at it along curve 5 (goalDist),
 * and lands with ground effects near the end.
 */
void EmRathalos_Leap(Em *em, EmWork *work)
{
    f32 floorY;
    u8 stateVar0;

    switch (em->step) {
    case 0:
        stateVar0 = em->stateVar0;
        switch (stateVar0) {
        case 0:
            em->stateVar0 = stateVar0 + 1;
            em->stance = EM_STANCE_GROUND;
            
        case 1:
            if (Em_TurnToTargetMotion(em, 0xE38, 3, 6, 5) != 0) {
                em->lookAtTarget = 0;
                em->step++;
                Em_SetMotion(em, RATH_MOT_LEAP, 0, 0);
                EmRathalos_StartCurves(em, 5);
            }
        }
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 60.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            EmRathalos_UpdateCurves(em);
            em->step++;
        }
        break;
    case 2:
        em->goalDist -= em->velZ;
        EmRathalos_UpdateCurves(em);
        if (em->goalDist <= 0.0f) {
            em->step++;
            em->accelY = -10.0f;
        }
        if (em->goalDist <= 500.0f) {
            EmRathalos_SpawnK14Type0AEvery5(em);
        }
        break;
    case 3:
        work->moveAngX = 0;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotatedAccel(em, &work->moveAngX);
        if (em->goalDist <= 500.0f) {
            EmRathalos_SpawnK14Type0AEvery5(em);
        }
        if (em->posY <= em->floorY) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            Em_SetMotion(em, RATH_MOT_LEAP_LAND, 0, 0);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/* RATH_ATK_STARE: motion 0x24 with the body bent toward the target. */
void EmRathalos_Stare(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 1;
        Em_SetMotion(em, 0x24, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->lookAtTarget = 0;
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * RATH_ATK_FIREBALL (and RATH_ATK_FIREBALL_LOW and _HIGH): turns to the
 * target and breathes a fireball (two projectiles from bone 0x24 at frame
 * 78), countdown times.
 */
void EmRathalos_Fireball(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        break;
    case 1:
        if (Em_TurnToTargetMotion(em, 0xE38, 3, 6, 5) != 0) {
            em->step++;
            em->lookAtTarget = 0;
            Em_SetMotion(em, RATH_MOT_FIREBALL, 0, 0);
        }
        break;
    case 2:
        if (EmMotion_ReachedFrame(em, 0, 154.0f) != 0 && EmMotion_ReachedFrame(em, 0, 276.0f) == 0) {
            if (!(gScene.frame & 3)) {
                DustCloud_Create((Em *) em, 0x1A, (u16) Rand_Next(1) & 1, 1.0f);
            }
        } else if (EmMotion_PassedFrameScaled(em, 78.0f, 0U) != 0) {
            AttackBurst_CreateAtBone(em, 0x24, 1, 0);
            AttackBurst_CreateAtBone(em, 0x24, 2, 0);
            EmRangedAttack_CreateAtBone(em, 0x22, 0, 0, 0, 0);
        }
        if (em->motion[0].playing == 0) {
            em->lookAtTarget = 0;
            em->step++;
            if (--((EmFlyerWork *)work)->countdown <= 0) {
                EmRathalos_EndState(em, 0, 0);
            } else {
                Em_SetTargetAngle(em);
                Em_SetStateRand(em, EM_GROUP_ATTACK, RATH_ATK_FIREBALL);
            }
        }
        break;
    }
}

/* RATH_ATK_ROAR: roars at the target. */
void EmRathalos_Roar(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_ROAR, 0, 0);
        Motion_InitTime(em, 0);
        em->lookAtTarget = 1;
        RathalosAttack_Create(em, 1);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

/* RATH_ATK_TAIL_SPIN: spins in place twice (rotY - 0x200 a frame). */
void EmRathalos_TailSpin(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_TAIL_SPIN, 0, 0);
        Motion_InitTime(em, 0);
        break;
    case 1:
        em->rotY -= 0x200;
        if (em->motion[1].playing == 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_TAIL_SPIN, 0, 0);
        }
        break;
    case 2:
        em->rotY -= 0x200;
        if (em->motion[1].playing == 0) {
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * RATH_ATK_LIE_AND_RISE: turns to the target, plays RATH_MOT_LIE and at
 * frame 30 motion 0x30.
 */
void EmRathalos_LieAndRise(Em *em)
{
    u8 stateVar0;

    switch (em->step) {
    case 0:
        stateVar0 = em->stateVar0;
        switch (stateVar0) {
        case 0:
            em->stateVar0 = stateVar0 + 1;
            em->stance = EM_STANCE_GROUND;
            em->lookAtTarget = 0;
            
        case 1:
            if (Em_TurnToTargetMotion(em, 0xE38, 3, 6, 5) != 0) {
                em->step++;
                em->lookAtTarget = 0;
                Em_SetMotion(em, RATH_MOT_LIE, 0, 0);
            }
            break;
        }
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 30.0f) != 0) {
            em->step++;
            Em_SetMotion(em, 0x30, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * RATH_ATK_SWOOP_THEN_LEAVE: flies at the target and swoops on it
 * (RATH_MOT_SWOOP_DIVE, RATH_MOT_SWOOP, the claw), pulling up from walls,
 * then climbs away to the escape point and ends the swoop.
 */
void EmRathalos_SwoopThenLeave(Em *em, EmWork *work)
{
    f32 targetWorldPos[3];
    f32 pos[3];
    f32 targetPosB[3];
    AreaRecord *area;
    Player *player;

    area = Stage_GetAreaRecord(em->area);
    if (work->wallTimer != 0) {
        if (--work->wallTimer <= 0) {
            work->wallTimer = 0;
        }
    }
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        work->wallTimer = 0;
        break;
    case 1:
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 0xD1, &EmRathalos_FlyParam);
        if (100.0f < em->velZ) {
            em->velZ -= 2.0f;
        } else if (Vec_DistanceXZ(&em->posX, em->goal) <= 3000.0f) {
            em->step++;
            em->timer = 600;
        }
        work->moveAngX = em->rotX;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
        if (--em->timer <= 0) {
            EmRathalos_EndSwoop(em, (s32) work);
        }
        break;
    case 2:
        if (em->targetNo == -1) {
            EmRathalos_EndSwoop(em, (s32) work);
            break;
        }
        if (80.0f < em->velZ) {
            em->velZ -= 1.0f;
        }
        player = &gPlayers[em->targetNo];
        if ((Obj_SameArea(em, player) & 0xFF) == 0 && player->active != 0) {
            EmRathalos_EndSwoop(em, (s32) work);
            break;
        }
        Em_GetTargetPos(em, em->targetNo, pos);
        Area_ToWorld(player->area, pos, targetWorldPos);
        em->aimAngle = Math_AngleXZ(em->mapPos, targetWorldPos);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 0xD1, &EmRathalos_FlyParam);
        if (em->aimAngle <= 0x800 || em->aimAngle >= 0xF800) {
            if (!(4000.0f < Vec_DistanceXZ(&em->posX, pos))) {
                EmRathalos_EndSwoop(em, (s32) work);
            } else {
                em->step++;
                work->turnSpeed = 0x100;
                Em_SetMotion(em, RATH_MOT_SWOOP_DIVE, 0, 0);
                em->timer = 600;
            }
        }
        work->moveAngX = em->rotX;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
        if (--em->timer <= 0 || Em_AllPlayersHere(em) == 0) {
            EmRathalos_EndSwoop(em, (s32) work);
        }
        break;
    case 3:
        if (em->rotZ != 0) {
            if (em->rotZ <= 0x8000) {
                em->rotZ -= work->rollSpeed;
            } else {
                em->rotZ += work->rollSpeed;
            }
        }
        if (EmMotion_ReachedFrame(em, 0, 46.0f) && 50.0f < em->velZ) {
            em->velZ -= 0.1f;
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_SWOOP, 0, 0);
            em->velZ = 100.0f;
            em->timer = 600;
        }
        player = &gPlayers[em->targetNo];
        if ((Obj_SameArea(em, player) & 0xFF) == 0 && player->active != 0) {
            EmRathalos_EndSwoop(em, (s32) work);
            break;
        }
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 2);
        work->moveAngX = em->rotX;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0xF;
        Em_MoveRotated(em, &work->moveAngX);
        em->posY -= 0.1f;
        if (--em->timer <= 0 || Em_AllPlayersHere(em) == 0) {
            EmRathalos_EndSwoop(em, (s32) work);
        }
        break;
    case 4:
        if (em->posY <= 1000.0f + area->bottom) {
            em->step++;
            em->timer = 600;
        }
        if (EmRathalos_ReachedTarget(em) && em->step == 4) {
            em->step++;
            em->timer = 600;
        }
        if (em->motionId[0] == EM_MOTION_ID(0, RATH_MOT_SWOOP_CLAW)) {
            EmRathalos_StepPitch(em, 2, 0.0f);
        } else {
            EmRathalos_StepPitch(em, 0, -200.0f);
        }
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 2);
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        if (work->wallTimer == 0) {
            Em_GetPitchToTarget(em, &work->moveAngX, -3000.0f, 0.0f);
        }
        if (em->wallHitDirs != 0) {
            work->moveAngX = 0;
            work->wallTimer = 30;
        }
        Em_MoveRotated(em, &work->moveAngX);
        if (em->wallHitDirs != 0) {
            work->moveAngX = 0;
            if (em->velY < 0.0f) {
                em->posY -= em->velY;
            }
            em->posY += 30.0f;
        }
        if (--em->timer <= 0 || Em_AllPlayersHere(em) == 0) {
            EmRathalos_EndSwoop(em, (s32) work);
        }
        break;
    case 5:
        EmRathalos_ReachedTarget(em);
        if (em->motionId[0] == EM_MOTION_ID(0, RATH_MOT_SWOOP_CLAW) && em->motion[0].playing == 0) {
            em->step = 6;
            Em_SetMotion(em, RATH_MOT_FLY, 0, 0);
            EmRathalos_SetEscapePoint(em);
        }
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 2);
        if (work->wallTimer == 0) {
        EmRathalos_StepPitch(em, 2, 100.0f);
        if (em->motionId[0] == EM_MOTION_ID(0, RATH_MOT_SWOOP_CLAW) && EmMotion_ReachedFrame(em, 0, 76.0f)) {
            Em_GetPitchToTarget(em, &work->moveAngX, 300.0f, 0.0f);
        } else if (em->targetNo != -1) {
            Em_GetTargetPos(em, em->targetNo, targetPosB);
            if (!(200.0f + targetPosB[1] < em->posY)) {
                Em_GetPitchToTarget(em, &work->moveAngX, 50.0f, 0.0f);
            } else {
                Em_GetPitchToTarget(em, &work->moveAngX, -2000.0f, 0.0f);
            }
        } else {
            Em_GetPitchToTarget(em, &work->moveAngX, -2000.0f, 0.0f);
        }
        }
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        if (em->wallHitDirs != 0) {
            work->moveAngX = 0;
            work->wallTimer = 30;
        }
        Em_MoveRotated(em, &work->moveAngX);
        if (em->wallHitDirs != 0) {
            work->moveAngX = 0;
            if (em->velY < 0.0f) {
                em->posY -= em->velY;
            }
            em->posY += 30.0f;
        }
        if (--em->timer <= 0 || Em_AllPlayersHere(em) == 0) {
            EmRathalos_EndSwoop(em, (s32) work);
        }
        break;
    case 6:
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        EmRathalos_StepPitch(em, 1, 0.0f);
        Em_UpdateFlight(em, 0xD1, &EmRathalos_FlyParam);
        work->moveAngX = em->rotX;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
        if (1300.0f + area->bottom <= em->posY) {
            em->step++;
            em->timer = 300;
        }
        break;
    case 7:
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        EmRathalos_StepPitch(em, 2, 0.0f);
        Em_UpdateFlight(em, 0xD1, &EmRathalos_FlyParam);
        work->moveAngX = em->rotX;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
        if (Vec_DistanceXZ(&em->posX, em->goal) <= 3000.0f) {
            EmRathalos_SetEscapePoint(em);
        }
        if ((em->rotX == 0 && em->rotZ == 0) || --em->timer <= 0) {
            em->rotX = 0;
            em->rotZ = 0;
            EmRathalos_EndSwoop(em, (s32) work);
        }
        break;
    }
    if (em->posY < em->floorY) {
        em->posY = em->floorY;
    }
}

/*
 * RATH_ATK_AIR_FIREBALL: rises in the hover, breathes a fireball down
 * (frame 6 of RATH_MOT_AIR_FIREBALL), and hovers again.
 */
void EmRathalos_AirFireball(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_AIR_FIREBALL_RISE, 0, 0);
        EmRathalos_StartCurves(em, 0xD);
        break;
    case 1:
        Em_PlaySeAtBoneOnFrame(em, 0x78, 0x20, 0x23);
        Em_TurnToGoal(em, (s32) ((EmFlyerWork *)work)->turnSpeed, 2);
        EmRathalos_UpdateCurves(em);
        if (EmMotion_PassedFrameScaled(em, 60.0f, 0U) != 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_AIR_FIREBALL, 0, 0);
            EmRathalos_StartCurves(em, 0x10);
        }
        break;
    case 2:
        Em_PlaySeAtBoneOnFrame(em, 0x78, 0x20, 0x23);
        Em_TurnToGoal(em, (s32) ((EmFlyerWork *)work)->turnSpeed, 2);
        EmRathalos_UpdateCurves(em);
        if (EmMotion_PassedFrameScaled(em, 6.0f, 0U) != 0) {
            AttackBurst_CreateAtBone(em, 0x24, 1, 0);
            AttackBurst_CreateAtBone(em, 0x24, 2, 0);
            EmRangedAttack_CreateAtBone(em, 0x22, 0, 1, 0x1E94, 0);
        }
        if (EmMotion_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_HOVER, 2, 0x22);
        }
        break;
    case 3:
        EmRathalos_MoveByVelYCurve(em, 1U);
        if (em->motion[0].playing == 0) {
            em->step++;
            EmRathalos_EndAirState(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * RATH_ATK_AIR_FIREBALL_B: as RATH_ATK_AIR_FIREBALL with the other flash
 * effect.
 */
void EmRathalos_AirFireballB(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_AIR_FIREBALL_RISE, 0, 0);
        EmRathalos_StartCurves(em, 0xD);
        break;
    case 1:
        Em_PlaySeAtBoneOnFrame(em, 0x78, 0x20, 0x23);
        Em_TurnToGoal(em, (s32) ((EmFlyerWork *)work)->turnSpeed, 2);
        EmRathalos_UpdateCurves(em);
        if (EmMotion_PassedFrameScaled(em, 60.0f, 0U) != 0) {
            em->step++;
            em->step++;
            Em_SetMotion(em, RATH_MOT_AIR_FIREBALL, 0, 0);
            EmRathalos_StartCurves(em, 0x10);
        }
        break;
    case 2:
    case 3:
        EmRathalos_UpdateCurves(em);
        Em_TurnToGoal(em, (s32) ((EmFlyerWork *)work)->turnSpeed, 2);
        if (EmMotion_PassedFrameScaled(em, 6.0f, 0U) != 0) {
            AttackBurst_CreateAtBone(em, 0x24, 1, 0);
            AttackBurst_CreateAtBone(em, 0x24, 2, 0);
            EmRangedAttack_CreateAtBone(em, 0x22, 0, 2, 0x1E94, 0);
        }
        if (EmMotion_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_HOVER, 2, 0x22);
        }
        break;
    case 4:
        EmRathalos_MoveByVelYCurve(em, 1U);
        if (em->motion[0].playing == 0) {
            em->step++;
            EmRathalos_EndAirState(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * RATH_ATK_AIR_DIVE: from the hover, turns to the target and lunges at it
 * (speed from the distance left), then hovers again; lands if the target is
 * not in front in time.
 */
void EmRathalos_AirDive(Em *em, EmWork *work)
{
    f32 pos[4];
    f32 speed;
    f32 dist;
    s32 n;
    EmFlyerWork *flyWork = (EmFlyerWork *)work;

    switch (em->step) {
    case 0:
        em->stance = EM_STANCE_AIR;
        Vec_Set(flyWork->startPos, em->posX, em->posY, em->posZ);
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 2);
        Em_AimAtTarget(em);
        em->lookAtTarget = 0;
        if (em->aimAngle <= 0x100 || em->aimAngle >= 0xFF00) {
            em->step = 2;
            Em_SetMotion(em, RATH_MOT_AIR_DIVE, 0, 0);
            Player_ClearVelocity(em);
            em->timer = 45;
        } else {
            em->step++;
            em->timer = 120;
            if (em->motionId[0] != EM_MOTION_ID(0, RATH_MOT_HOVER)) {
                Em_SetMotion(em, RATH_MOT_HOVER, 0, 0);
            }
        }
        break;
    case 1:
        EmRathalos_MoveByVelYCurve(em, 1);
        if (--em->timer <= 0) {
            em->step = 0x62;
            em->timer = 300;
            EmRathalos_ChangeState(em, EM_GROUP_FLY, RATH_FLY_LAND, 1);
            break;
        }
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 2);
        Em_AimAtTarget(em);
        if (em->aimAngle <= 0x100 || em->aimAngle >= 0xFF00) {
            em->step = 2;
            Em_SetMotion(em, RATH_MOT_AIR_DIVE, 0, 0);
            Player_ClearVelocity(em);
            em->timer = 45;
        }
        break;
    case 2:
        em->step++;
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 2);
        break;
    case 3:
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 2);
        em->targetDist = Vec_DistanceXZ(&em->posX, em->goal) - 300.0f;
        if (em->targetDist < 0.0f) {
            em->targetDist = 0.0f;
        }
        em->velZ = 100.0f;
        em->accelZ = 10.0f;
        n = (s32) ((Math_Sqrtf(2.0f * em->accelZ * em->targetDist + em->velZ * em->velZ) - em->velZ) / em->accelZ) + 1;
        if (--em->timer <= n) {
            em->step++;
        }
        break;
    case 4:
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 2);
        if (em->motion[0].blend != 0) {
            break;
        }
        if (EmMotion_ReachedFrame(em, 0, 90.0f) == 0) {
            if (em->timer < 5 && em->timer != 0) {
                em->accelZ = 0.0f;
                em->targetDist = Vec_DistanceXZ(&em->posX, em->goal) - 300.0f;
                if (em->targetDist < 0.0f) {
                    em->targetDist = 0.0f;
                }
                speed = em->targetDist / (f32) em->timer--;
                if (1.2f * em->velZ >= speed && speed <= 0.8f * em->velZ) {
                    em->velZ = speed;
                } else if (em->velZ < speed) {
                    em->velZ *= 1.2f;
                } else {
                    em->velZ *= 0.8f;
                }
            }
            work->moveAngY = em->rotY;
            work->moveAngZ = 0;
            Em_GetPitchToTarget(em, &work->moveAngX, -200.0f, -300.0f);
            Em_MoveRotatedAccel(em, &work->moveAngX);
            Em_GetTargetPos(em, em->targetNo, pos);
            speed = pos[1];
            if (em->posY < speed) {
                em->posY = speed;
            }
            if (em->posY < em->floorY) {
                em->posY = em->floorY;
            }
        }
        if (EmMotion_ReachedFrame(em, 0, 120.0f) != 0) {
            em->step++;
            Player_ClearVelocity(em);
            em->timer = 55;
        }
        break;
    case 5:
        dist = Vec_Distance((f32 *) &em->posX, flyWork->startPos);
        work->moveAngX = 0;
        work->moveAngY = Math_AngleXZ(&em->posX, flyWork->startPos) & 0xFFFF;
        work->moveAngZ = 0;
        if (em->timer != 0) {
            em->velZ = dist / (f32) em->timer;
            em->velY = (flyWork->startPos[1] - em->posY) / (f32) em->timer--;
            Em_MoveRotated(em, &work->moveAngX);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            Player_ClearVelocity(em);
            EmRathalos_EndAirState(em, 0);
        }
        break;
    case 0x62:
    case 0x63:
        if (--em->timer <= 0) {
            em->timer = 300;
            if (em->remote == 0) {
                EmRathalos_ChangeState(em, EM_GROUP_FLY, RATH_FLY_LAND, 1);
            }
        }
        break;
    }
}

/*
 * RATH_ATK_ROAR_AT_POINT: looks at the action point nearest bone 0x23 and
 * roars at it, then motion 0x30.
 */
void EmRathalos_RoarAtPoint(Em *em, EmWork *work)
{
    f32 pos[3];
    s32 hasPoint;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 1;
        em->lookAtPoint = 1;
        break;
    case 1:
        hasPoint = Em_NearestActionToBone(em, 0x23, em->lookPoint);
        Em_BonePos(em, 0x23, pos);
        if (!(150.0f < Vec_Distance((f32 *) pos, (f32 *) em->lookPoint))) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_ROAR, 0, 0);
            break;
        }
        if (hasPoint == 0) {
            em->lookAtTarget = 0;
            em->lookAtPoint = 0;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    case 2:
        if (em->motion[1].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x30, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->lookAtTarget = 0;
            em->lookAtPoint = 0;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

/* RATH_ATK_SINGLE_A: motion 0x53 once. */
void EmRathalos_SingleAttackA(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x53, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

/* RATH_ATK_SINGLE_B: motion 0x38 once. */
void EmRathalos_SingleAttackB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x38, 0, 0);
        
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

/* RATH_ATK_CHARGE: runs at the goal and ends there. */
void EmRathalos_Charge(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_RUN, 0, 0);
        break;
    case 1:
        if (Em_StepToGoal(em, 0x80, 1) == 1) {
            em->timer = 1;
        }
        if (--em->timer <= 0) {
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * RATH_ATK_BACK_HOP_FIREBALL (variant 0) and its two turning variants (1:
 * rotY + 0x666, 2: rotY - 0x666 a frame, for 20 frames): hops backward
 * breathing a fireball at frame 90, and lands.
 */
void EmRathalos_BackHopFireball(Em *em, EmWork *work, s32 variant)
{
    f32 floorY;
    s32 ended;
    u8 step;

    step = em->step;
    ended = 0;
    switch (step) {
    case 0:
        em->step = step + 1;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_BACK_HOP, 0, 0);
        EmRathalos_StartCurves(em, 8);
        break;
    case 1:
        if (EmMotion_InFrameRange(em, 0, 0.0f, 20.0f) != 0) {
            switch (variant) {
            case 0:
                break;
            case 1:
                em->rotY += 0x666;
                break;
            case 2:
                em->rotY -= 0x666;
                break;
            }
        }
        if (EmMotion_PassedFrameScaled(em, 90.0f, 0U) != 0) {
            AttackBurst_CreateAtBone(em, 0x24, 1, 0);
            AttackBurst_CreateAtBone(em, 0x24, 2, 0);
            EmRangedAttack_CreateAtBone(em, 0x22, 0, 2, 0x2222, 0);
        }
        if (EmMotion_ReachedFrame(em, 0, 76.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            ended = EmRathalos_UpdateCurves(em) & 0xFF;
        }
        if ((ended != 0) && (em->posY <= em->floorY)) {
            Em_SetMotion(em, RATH_MOT_BACK_HOP_LAND, 0, 0);
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            EmRathalos_SpawnLandingDust(em);
            CamShake_ReqAIf(em, 2);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step = step + 1;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/* RATH_ATK_AIR_FIREBALL_DOUBLE: two fireballs from the hover. */
void EmRathalos_AirFireballDouble(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_AIR_FIREBALL_RISE, 0, 0);
        EmRathalos_StartCurves(em, 0xDU);
        break;
    case 1:
        Em_PlaySeAtBoneOnFrame(em, 0x78, 0x20, 0x23);
        EmRathalos_UpdateCurves(em);
        Em_TurnToGoal(em, (s32) ((EmFlyerWork *)work)->turnSpeed, 2);
        if (EmMotion_PassedFrameScaled(em, 60.0f, 0U) != 0) {
            em->step++;
            em->step++;
            Em_SetMotion(em, RATH_MOT_AIR_FIREBALL, 0, 0);
            EmRathalos_StartCurves(em, 0x10U);
        }
        break;
    case 3:
    case 2:
        EmRathalos_UpdateCurves(em);
        Em_TurnToGoal(em, (s32) ((EmFlyerWork *)work)->turnSpeed, 2);
        if (EmMotion_PassedFrameScaled(em, 6.0f, 0U) != 0) {
            AttackBurst_CreateAtBone(em, 0x24, 1, 0);
            AttackBurst_CreateAtBone(em, 0x24, 2, 0);
            EmRangedAttack_CreateAtBone(em, 0x22, 0, 2, 0x1E94, 0);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_AIR_FIREBALL, 4, 0);
        }
        break;
    case 4:
        EmRathalos_UpdateCurves(em);
        Em_TurnToGoal(em, (s32) ((EmFlyerWork *)work)->turnSpeed, 2);
        if (EmMotion_PassedFrameScaled(em, 6.0f, 1U) != 0) {
            AttackBurst_CreateAtBone(em, 0x24, 1, 0);
            AttackBurst_CreateAtBone(em, 0x24, 2, 0);
            EmRangedAttack_CreateAtBone(em, 0x22, 0, 2, 0x1E94, 0);
        }
        if (EmMotion_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_HOVER, 2, 0x22);
        }
        break;
    case 5:
        EmRathalos_MoveByVelYCurve(em, 1U);
        if (em->motion[1].playing == 0) {
            em->step++;
            EmRathalos_EndAirState(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * RATH_ATK_CHARGE_TRIP: runs at the goal, and within 500 of it ends the
 * charge with RATH_MOT_CHARGE_TRIP.
 */
void EmRathalos_ChargeTrip(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_RUN, 0, 0);
        break;
    case 1:
        Em_StepToGoal(em, 0x40, 1);
        if (em->goalDist <= 500.0f) {
            em->timer = 1;
        }
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_CHARGE_TRIP, 0, 0);
            RathalosAttack_Create(em, 0x24);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

/* RATH_ATK_ROAR_B: the roar without its attached effect. */
void EmRathalos_RoarB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_ROAR, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * RATH_ATK_SOMERSAULT: jumps up (airborne from frame 94), turns over and
 * lands with ground effects and a camera shake.
 */
void EmRathalos_Somersault(Em *em)
{
    f32 floorY;
    s32 ended;

    ended = 0;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_FLIP, 0, 0);
        EmRathalos_StartCurves(em, 0xA);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 94.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            ended = EmRathalos_UpdateCurves(em) & 0xFF;
        }
        if ((ended != 0) && (em->posY <= (372.0f + em->floorY))) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_FLIP_FALL, 0, 0);
            EmRathalos_StartCurves(em, 0xB);
        }
        break;
    case 2:
        EmRathalos_UpdateCurves(em);
        if (em->posY <= em->floorY) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_FLIP_LAND, 0, 0);
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            EmRathalos_SpawnLandingDust(em);
            CamShake_ReqAIf(em, 2);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * RATH_ATK_SWOOP_THEN_LAND: the swoop of RATH_ATK_SWOOP_THEN_LEAVE, ending
 * on the ground (RATH_MOT_LEAP_LAND).
 */
void EmRathalos_SwoopThenLand(Em *em, EmWork *work)
{
    f32 targetWorldPos[3];
    f32 pos[3];
    f32 targetPosB[3];
    AreaRecord *area;
    s8 idx;

    area = Stage_GetAreaRecord(em->area);
    if (work->wallTimer != 0) {
        if (--work->wallTimer <= 0) {
            work->wallTimer = 0;
        }
    }
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        work->wallTimer = 0;
        break;
    case 1:
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 0xD1, &EmRathalos_FlyParam);
        if (100.0f < em->velZ) {
            em->velZ -= 2.0f;
        } else if (Vec_DistanceXZ(&em->posX, em->goal) <= 3000.0f) {
            em->step++;
            em->timer = 600;
        }
        work->moveAngX = em->rotX;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
        if (--em->timer <= 0) {
            EmRathalos_EndSwoop(em, (s32) work);
        }
        break;
    case 2:
        if (em->targetNo == -1) {
            EmRathalos_EndSwoop(em, (s32) work);
            break;
        }
        if (80.0f < em->velZ) {
            em->velZ -= 1.0f;
        }
        idx = em->targetNo;
        Em_GetTargetPos(em, idx, pos);
        Area_ToWorld(((Player *) &gPlayers[idx])->area, pos, targetWorldPos);
        em->aimAngle = Math_AngleXZ(em->mapPos, targetWorldPos);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 0xD1, &EmRathalos_FlyParam);
        if (em->aimAngle <= 0x800 || em->aimAngle >= 0xF800) {
            if (!(4000.0f < Vec_DistanceXZ(&em->posX, pos))) {
                EmRathalos_EndSwoop(em, (s32) work);
            } else {
                em->step++;
                work->turnSpeed = 0x100;
                Em_SetMotion(em, RATH_MOT_SWOOP_DIVE, 0, 0);
                em->timer = 600;
            }
        }
        work->moveAngX = em->rotX;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
        if (--em->timer <= 0 || Em_AllPlayersHere(em) == 0) {
            EmRathalos_EndSwoop(em, (s32) work);
        }
        break;
    case 3:
        if (em->rotZ != 0) {
            if (em->rotZ <= 0x8000) {
                em->rotZ -= work->rollSpeed;
            } else {
                em->rotZ += work->rollSpeed;
            }
        }
        if (EmMotion_ReachedFrame(em, 0, 46.0f)) {
            EmRathalos_StepPitch(em, 0, -200.0f);
            if (50.0f < em->velZ) {
                em->velZ -= 0.1f;
            }
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_SWOOP, 0, 0);
            em->velZ = 100.0f;
            em->timer = 600;
        }
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 2);
        work->moveAngX = em->rotX;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0xF;
        Em_MoveRotated(em, &work->moveAngX);
        em->posY -= 0.1f;
        if (--em->timer <= 0 || Em_AllPlayersHere(em) == 0) {
            EmRathalos_EndSwoop(em, (s32) work);
        }
        break;
    case 4:
        if (em->posY <= 1000.0f + area->bottom) {
            em->step++;
            em->timer = 600;
        }
        if (EmRathalos_ReachedTarget(em) && em->step == 4) {
            em->step++;
            em->timer = 600;
        }
        if (em->motionId[0] == EM_MOTION_ID(0, RATH_MOT_SWOOP_CLAW)) {
            EmRathalos_StepPitch(em, 2, 0.0f);
        } else {
            EmRathalos_StepPitch(em, 0, -200.0f);
        }
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 2);
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        if (work->wallTimer == 0) {
            Em_GetPitchToTarget(em, &work->moveAngX, -3000.0f, 0.0f);
        }
        if (em->wallHitDirs != 0) {
            work->moveAngX = 0;
            work->wallTimer = 30;
        }
        Em_MoveRotated(em, &work->moveAngX);
        if (em->wallHitDirs != 0) {
            work->moveAngX = 0;
            if (em->velY < 0.0f) {
                em->posY -= em->velY;
            }
            em->posY += 30.0f;
        }
        if (--em->timer <= 0 || Em_AllPlayersHere(em) == 0) {
            EmRathalos_EndSwoop(em, (s32) work);
        }
        break;
    case 5:
        EmRathalos_ReachedTarget(em);
        if (em->motionId[0] == EM_MOTION_ID(0, RATH_MOT_SWOOP_CLAW) && em->motion[0].playing == 0) {
            em->step = 6;
            em->accelX = 0.0f;
            em->accelZ = -10.0f;
            em->accelY = -10.0f;
        }
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 2);
        if (work->wallTimer == 0) {
        EmRathalos_StepPitch(em, 2, 100.0f);
        if (em->motionId[0] == EM_MOTION_ID(0, RATH_MOT_SWOOP_CLAW) && EmMotion_ReachedFrame(em, 0, 76.0f)) {
            Em_GetPitchToTarget(em, &work->moveAngX, 50.0f, 0.0f);
        } else if (em->targetNo != -1) {
            Em_GetTargetPos(em, em->targetNo, targetPosB);
            if (!(200.0f + targetPosB[1] < em->posY)) {
                Em_GetPitchToTarget(em, &work->moveAngX, 50.0f, 0.0f);
            } else {
                Em_GetPitchToTarget(em, &work->moveAngX, -2000.0f, 0.0f);
            }
        } else {
            Em_GetPitchToTarget(em, &work->moveAngX, -2000.0f, 0.0f);
        }
        }
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        if (em->wallHitDirs != 0) {
            work->moveAngX = 0;
            work->wallTimer = 30;
        }
        Em_MoveRotated(em, &work->moveAngX);
        if (em->wallHitDirs != 0) {
            work->moveAngX = 0;
            if (em->velY < 0.0f) {
                em->posY -= em->velY;
            }
            em->posY += 30.0f;
        }
        if (--em->timer <= 0 || Em_AllPlayersHere(em) == 0) {
            EmRathalos_EndSwoop(em, (s32) work);
        }
        break;
    case 6:
        if (em->velZ < 0.0f) {
            em->velZ = 0.0f;
            em->accelZ = 0.0f;
        }
        work->moveAngX = 0;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotatedAccel(em, &work->moveAngX);
        if (em->posY <= em->floorY) {
            em->step++;
            em->rotX = 0;
            em->rotZ = 0;
            em->posY = em->floorY;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, RATH_MOT_LEAP_LAND, 0, 0);
        }
        break;
    case 7:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
    if (em->posY < em->floorY) {
        em->posY = em->floorY;
    }
}

/* RATH_ATK_AIR_FIREBALL_DOUBLE_B: as RATH_ATK_AIR_FIREBALL_DOUBLE. */
void EmRathalos_AirFireballDoubleB(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_AIR_FIREBALL_RISE, 0, 0);
        EmRathalos_StartCurves(em, 0xDU);
        break;
    case 1:
        Em_PlaySeAtBoneOnFrame(em, 0x78, 0x20, 0x23);
        EmRathalos_UpdateCurves(em);
        Em_TurnToGoal(em, (s32) ((EmFlyerWork *)work)->turnSpeed, 2);
        if (EmMotion_PassedFrameScaled(em, 60.0f, 0U) != 0) {
            em->step++;
            em->step++;
            Em_SetMotion(em, RATH_MOT_AIR_FIREBALL, 0, 0);
            EmRathalos_StartCurves(em, 0x10U);
        }
        break;
    case 3:
    case 4:
    case 2:
        EmRathalos_UpdateCurves(em);
        Em_TurnToGoal(em, (s32) ((EmFlyerWork *)work)->turnSpeed, 2);
        if (EmMotion_PassedFrameScaled(em, 6.0f, 1U) != 0) {
            AttackBurst_CreateAtBone(em, 0x24, 1, 0);
            AttackBurst_CreateAtBone(em, 0x24, 2, 0);
            EmRangedAttack_CreateAtBone(em, 0x22, 0, 2, 0x1E94, 0);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_AIR_FIREBALL, 4, 0);
        }
        break;
    case 5:
        EmRathalos_UpdateCurves(em);
        Em_TurnToGoal(em, (s32) ((EmFlyerWork *)work)->turnSpeed, 2);
        if (EmMotion_PassedFrameScaled(em, 6.0f, 1U) != 0) {
            AttackBurst_CreateAtBone(em, 0x24, 1, 0);
            AttackBurst_CreateAtBone(em, 0x24, 2, 0);
            EmRangedAttack_CreateAtBone(em, 0x22, 0, 2, 0x1E94, 0);
        }
        if (EmMotion_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_HOVER, 2, 0x22);
        }
        break;
    case 6:
        EmRathalos_MoveByVelYCurve(em, 1U);
        if (em->motion[1].playing == 0) {
            em->step++;
            EmRathalos_EndAirState(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * RATH_ATK_FIREBALL_B: turns and breathes the second ground fireball
 * (RATH_MOT_FIREBALL_B).
 */
void EmRathalos_FireballB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        break;
    case 1:
        if (Em_TurnToTargetMotion(em, 0xE38, 3, 6, 5) != 0) {
            em->step++;
            em->lookAtTarget = 0;
            Em_SetMotion(em, RATH_MOT_FIREBALL_B, 0, 0);
        }
        break;
    case 2:
        if ((EmMotion_ReachedFrame(em, 0, 256.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 316.0f) == 0) && !(gScene.frame & 3)) {
            DustCloud_Create((Em *) em, 0x1A, 1, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 78.0f, 0U) != 0) {
            AttackBurst_CreateAtBone(em, 0x24, 1, 0);
            AttackBurst_CreateAtBone(em, 0x24, 2, 0);
            EmRangedAttack_CreateAtBone(em, 0x22, 0, 0, 0x5B0, 0);
        }
        if (EmMotion_PassedFrameScaled(em, 120.0f, 0U) != 0) {
            AttackBurst_CreateAtBone(em, 0x24, 1, 0);
            AttackBurst_CreateAtBone(em, 0x24, 2, 0);
            EmRangedAttack_CreateAtBone(em, 0x22, 0, 0, 0x4FA, 0xF1C8);
        }
        if (EmMotion_PassedFrameScaled(em, 164.0f, 0U) != 0) {
            AttackBurst_CreateAtBone(em, 0x24, 1, 0);
            AttackBurst_CreateAtBone(em, 0x24, 2, 0);
            EmRangedAttack_CreateAtBone(em, 0x22, 0, 0, 0x444, 0x9F5);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            em->lookAtTarget = 0;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

/* RATH_ATK_FIREBALL_LOW: the fireball at a target below it. */
void EmRathalos_FireballLow(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        break;
    case 1:
        if (Em_TurnToTargetMotion(em, 0xE38, 3, 6, 5) != 0) {
            em->step++;
            em->lookAtTarget = 0;
            Em_SetMotion(em, RATH_MOT_FIREBALL, 0, 0);
        }
        break;
    case 2:
        if (EmMotion_ReachedFrame(em, 0, 154.0f) != 0 && EmMotion_ReachedFrame(em, 0, 276.0f) == 0) {
            if (!(gScene.frame & 3)) {
                DustCloud_Create((Em *) em, 0x1A, (u16) Rand_Next(1) & 1, 1.0f);
            }
        } else if (EmMotion_PassedFrameScaled(em, 78.0f, 0U) != 0) {
            AttackBurst_CreateAtBone(em, 0x24, 1, 0);
            AttackBurst_CreateAtBone(em, 0x24, 2, 0);
            EmRangedAttack_CreateAtBone(em, 0x22, 0, 0, 0xE39, 0);
        }
        if (em->motion[0].playing == 0) {
            em->lookAtTarget = 0;
            em->step++;
            if (--((EmFlyerWork *)work)->countdown <= 0) {
                EmRathalos_EndState(em, 0, 0);
            } else {
                Em_SetTargetAngle(em);
                Em_SetStateRand(em, EM_GROUP_ATTACK, RATH_ATK_FIREBALL_LOW);
            }
        }
        break;
    }
}

/* RATH_ATK_FIREBALL_HIGH: the fireball at a target above it. */
void EmRathalos_FireballHigh(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        break;
    case 1:
        if (Em_TurnToTargetMotion(em, 0xE38, 3, 6, 5) != 0) {
            em->step++;
            em->lookAtTarget = 0;
            Em_SetMotion(em, RATH_MOT_FIREBALL, 0, 0);
        }
        break;
    case 2:
        if (EmMotion_ReachedFrame(em, 0, 154.0f) != 0 && EmMotion_ReachedFrame(em, 0, 276.0f) == 0) {
            if (!(gScene.frame & 3)) {
                DustCloud_Create((Em *) em, 0x1A, (u16) Rand_Next(1) & 1, 1.0f);
            }
        } else if (EmMotion_PassedFrameScaled(em, 78.0f, 0U) != 0) {
            AttackBurst_CreateAtBone(em, 0x24, 1, 0);
            AttackBurst_CreateAtBone(em, 0x24, 2, 0);
            EmRangedAttack_CreateAtBone(em, 0x22, 0, 0, 0xF1C8, 0);
        }
        if (em->motion[0].playing == 0) {
            em->lookAtTarget = 0;
            em->step++;
            if (--((EmFlyerWork *)work)->countdown <= 0) {
                EmRathalos_EndState(em, 0, 0);
            } else {
                Em_SetTargetAngle(em);
                Em_SetStateRand(em, EM_GROUP_ATTACK, RATH_ATK_FIREBALL_HIGH);
            }
        }
        break;
    }
}

/*
 * RATH_ATK_TAIL_SPIN_B: spins in place twice the other way (rotY + 0x200 a
 * frame).
 */
void EmRathalos_TailSpinB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_TAIL_SPIN_B, 0, 0);
        Motion_InitTime(em, 0);
        break;
    case 1:
        em->rotY += 0x200;
        if (em->motion[1].playing == 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_TAIL_SPIN_B, 0, 0);
        }
        break;
    case 2:
        em->rotY += 0x200;
        if (em->motion[1].playing == 0) {
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

/* RATH_ATK_RUN_AND_STOP: runs at the goal and stops (RATH_MOT_RUN_STOP). */
void EmRathalos_RunAndStop(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_RUN, 0, 0);
        break;
    case 1:
        Em_StepToGoal(em, 0x40, 1);
        if (em->goalDist <= 500.0f) {
            em->timer = 1;
        }
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_RUN_STOP, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * Moves the monster 10 forward along its yaw (the air fireballs that
 * advance).
 */
void EmRathalos_StepForward(Em *em, s32 unused)
{
    f32 forward[4];
    f32 out[4];
    s32 ang[4];
    f32 mtx[16];

    ang[0] = 0;
    ang[1] = em->rotY;
    ang[2] = 0;
    Vec_Set(forward, 0.0f, 0.0f, 10.0f);
    Mtx_FromAnglesYXZ(ang, mtx);
    Vec_Rotate(out, forward, mtx);
    em->posX += out[0];
    em->posZ += out[2];
}

/* RATH_ATK_AIR_FIREBALL_FORWARD: the air fireball while moving forward. */
void EmRathalos_AirFireballForward(Em *em, s32 work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_AIR_FIREBALL_RISE, 0, 0);
        EmRathalos_StartCurves(em, 0xD);
        break;
    case 1:
        Em_PlaySeAtBoneOnFrame(em, 0x78, 0x20, 0x23);
        EmRathalos_UpdateCurves(em);
        EmRathalos_StepForward(em, work);
        if (EmMotion_PassedFrameScaled(em, 60.0f, 0U) != 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_AIR_FIREBALL, 0, 0);
            EmRathalos_StartCurves(em, 0x10);
        }
        break;
    case 2:
        Em_PlaySeAtBoneOnFrame(em, 0x78, 0x20, 0x23);
        EmRathalos_UpdateCurves(em);
        EmRathalos_StepForward(em, work);
        if (EmMotion_PassedFrameScaled(em, 6.0f, 0U) != 0) {
            AttackBurst_CreateAtBone(em, 0x24, 1, 0);
            AttackBurst_CreateAtBone(em, 0x24, 2, 0);
            EmRangedAttack_CreateAtBone(em, 0x22, 0, 1, 0x2D83, 0);
        }
        if (EmMotion_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_HOVER, 2, 0x22);
        }
        break;
    case 3:
        EmRathalos_MoveByVelYCurve(em, 1U);
        if (em->motion[0].playing == 0) {
            em->step++;
            EmRathalos_EndAirState(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * RATH_ATK_AIR_FIREBALL_DOUBLE_FORWARD: two air fireballs while moving
 * forward.
 */
void EmRathalos_AirFireballDoubleForward(Em *em, s32 work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_AIR_FIREBALL_RISE, 0, 0);
        EmRathalos_StartCurves(em, 0xDU);
        break;
    case 1:
        Em_PlaySeAtBoneOnFrame(em, 0x78, 0x20, 0x23);
        EmRathalos_UpdateCurves(em);
        EmRathalos_StepForward(em, work);
        if (EmMotion_PassedFrameScaled(em, 60.0f, 0U) != 0) {
            em->step++;
            em->step++;
            Em_SetMotion(em, RATH_MOT_AIR_FIREBALL, 0, 0);
            EmRathalos_StartCurves(em, 0x10U);
        }
        break;
    case 3:
    case 4:
    case 2:
        EmRathalos_UpdateCurves(em);
        EmRathalos_StepForward(em, work);
        if (EmMotion_PassedFrameScaled(em, 6.0f, 1U) != 0) {
            AttackBurst_CreateAtBone(em, 0x24, 1, 0);
            AttackBurst_CreateAtBone(em, 0x24, 2, 0);
            EmRangedAttack_CreateAtBone(em, 0x22, 0, 2, 0x2D83, 0);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_AIR_FIREBALL, 4, 0);
        }
        break;
    case 5:
        EmRathalos_UpdateCurves(em);
        EmRathalos_StepForward(em, work);
        if (EmMotion_PassedFrameScaled(em, 6.0f, 1U) != 0) {
            AttackBurst_CreateAtBone(em, 0x24, 1, 0);
            AttackBurst_CreateAtBone(em, 0x24, 2, 0);
            EmRangedAttack_CreateAtBone(em, 0x22, 0, 2, 0x2D83, 0);
        }
        if (EmMotion_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_HOVER, 2, 0x22);
        }
        break;
    case 6:
        EmRathalos_MoveByVelYCurve(em, 1U);
        if (em->motion[1].playing == 0) {
            em->step++;
            EmRathalos_EndAirState(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * RATH_ATK_SOMERSAULT_DOUBLE: a somersault, motion 0x37, and a second
 * somersault.
 */
void EmRathalos_SomersaultDouble(Em *em)
{
    f32 floorY;
    s32 ended;
    u8 step;

    step = em->step;
    ended = 0;
    switch (step) {
    case 0:
        em->step = step + 1;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_FLIP, 0, 0);
        EmRathalos_StartCurves(em, 0xAU);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 94.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            ended = EmRathalos_UpdateCurves(em) & 0xFF;
        }
        if ((ended != 0) && (em->posY <= (372.0f + em->floorY))) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_FLIP_FALL, 0, 0);
            EmRathalos_StartCurves(em, 0xBU);
        }
        break;
    case 2:
        EmRathalos_UpdateCurves(em);
        if (em->posY <= em->floorY) {
            em->step++;
            Em_SetMotion(em, 0x37, 0, 0);
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            EmRathalos_SpawnLandingDust(em);
            CamShake_ReqAIf(em, 2);
        }
        break;
    case 3:
        if (EmMotion_InFrameRange(em, 0, 50.0f, 70.0f) != 0) {
            Em_TurnToGoal(em, 0x100, 2);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_FLIP, 2, 0x46);
            EmRathalos_StartCurves(em, 0xAU);
        }
        break;
    case 4:
        if (EmMotion_ReachedFrame(em, 0, 94.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            ended = EmRathalos_UpdateCurves(em) & 0xFF;
        }
        if ((ended != 0) && (em->posY <= (372.0f + em->floorY))) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_FLIP_FALL, 0, 0);
            EmRathalos_StartCurves(em, 0xBU);
        }
        break;
    case 5:
        EmRathalos_UpdateCurves(em);
        if (em->posY <= em->floorY) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_FLIP_LAND, 0, 0);
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            EmRathalos_SpawnLandingDust(em);
            CamShake_ReqAIf(em, 2);
        }
        break;
    case 6:
        if (em->motion[0].playing == 0) {
            em->step = step + 1;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/* RATH_REACT_FLINCH: flinches (motion 0x3C); the script restarts. */
void EmRathalos_Flinch(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x3C, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

/* RATH_REACT_FLINCH_B: flinches (motion 0x42). */
void EmRathalos_FlinchB(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x42, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

/* RATH_REACT_FLINCH_C: flinches (motion 0x3F). */
void EmRathalos_FlinchC(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x3F, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

/* RATH_REACT_FLINCH_D: flinches (motion 0x40). */
void EmRathalos_FlinchD(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x40, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * RATH_REACT_PART_BREAK: the once-only reaction (motion 0x41), then
 * RATH_REACT_PART_BREAK_END, which breaks part 3.
 */
void EmRathalos_PartBreak(Em *em, EmWork *work)
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
        Em_SetMotion(em, 0x41, 0, 0);
        break;
    case 1:
        if ((EmMotion_PassedFrameScaled(em, 6.0f, 0U) != 0) && ((s8) em->meshShown[1] != 0)) {
            em->meshShown[1] = 0;
        }
        if ((EmMotion_ReachedFrame(em, 0, 52.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 122.0f) == 0)) {
            turnF = 936.0f * em->motionSpeed;
            turn = (u32) turnF;
            em->rotY -= (s32) turn & 0xFFFF;
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            em->rotY -= 8;
            EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_PART_BREAK_END, 3);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_PART_BREAK_END, 3);
        }
        break;
    }
}

/* RATH_REACT_KNOCK_DOWN: is knocked down, lies and gets up. */
void EmRathalos_KnockDown(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, RATH_MOT_KNOCK_DOWN, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 0;
            Em_SetMotion(em, RATH_MOT_DOWN, 0, 0);
        }
        break;
    case 2:
        em->timer++;
        if (em->timer >= 0x78) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_DOWN_GET_UP, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmRathalos_EndState(em, 0, 0);
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
        }
        break;
    }
}

/* RATH_REACT_END_AIR: glides once. */
void EmRathalos_ReactEndAir(Em *em, EmWork *work)
{
    EmRathalos_EndAirState(em, 0x1);
}

/* RATH_REACT_END_AIR_B: glides once. */
void EmRathalos_ReactEndAirB(Em *em, EmWork *work)
{
    EmRathalos_EndAirState(em, 0x1);
}

/* RATH_REACT_FALL: falls out of the air, hits the floor and gets up. */
void EmRathalos_Fall(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, RATH_MOT_FALL, 0, 0);
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
            Em_SetMotion(em, RATH_MOT_FALL_LAND, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_DOWN_GET_UP, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmRathalos_EndState(em, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    }
}

/* RATH_REACT_FALL_B: as RATH_REACT_FALL. */
void EmRathalos_FallB(Em *em, EmWork *work)
{
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, RATH_MOT_FALL, 0, 0);
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
            Em_SetMotion(em, RATH_MOT_FALL_LAND, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_DOWN_GET_UP, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmRathalos_EndState(em, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    }
}

/* RATH_REACT_FLINCH_E: flinches (motion 0x3D). */
void EmRathalos_FlinchE(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x3D, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

/* RATH_REACT_STATUS4: status 4 (motion 0x4C, no flinch), then shakes it off. */
void EmRathalos_Status4(Em *em, EmWork *work)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_STATUS);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x4C, 0, 0);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE_STATUS4_SHAKE_OFF, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE_STATUS4_SHAKE_OFF, 4);
        }
        break;
    }
    Em_SpawnStatus4Effect(em, 2);
}

/* RATH_REACT_TRAPPED: caught (RATH_MOT_TRAPPED, no flinch), then struggles. */
void EmRathalos_Trapped(Em *em, EmWork *work)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_NO_FLINCH);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, RATH_MOT_TRAPPED, 0, 0);
        em->noReact = 1;
        em->blind = 0;
        break;
    case 1:
        if ((s8) em->holdTimer < 5) {
            em->holdTimer = 5;
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_TRAPPED_STRUGGLE, 3);
        }
        break;
    case 2:
        if ((s8) em->holdTimer < 5) {
            em->holdTimer = 5;
        }
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_TRAPPED_STRUGGLE, 3);
        }
        break;
    }
}

/*
 * RATH_REACT_TRAPPED_STRUGGLE: struggles three times, then flies out
 * (RATH_FLY_ESCAPE_UPWARD).
 */
void EmRathalos_TrappedStruggle(Em *em, EmWork *work)
{
    em->holdTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, RATH_MOT_STRUGGLE, 0, 0);
        em->noReact = 1;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            Em_SetMotion(em, RATH_MOT_STRUGGLE, 0, 0);
        }
        if (--em->trapTimer <= 0) {
            em->step++;
            EmRathalos_ChangeState(em, EM_GROUP_FLY, RATH_FLY_ESCAPE_UPWARD, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, EM_GROUP_FLY, RATH_FLY_ESCAPE_UPWARD, 4);
        }
        if (em->motion[0].playing == 0) {
            Em_SetMotion(em, RATH_MOT_STRUGGLE, 0, 0);
        }
        break;
    }
}

/*
 * RATH_REACT_STATUS4_DOWNED: status 4 in a hold: lies downed with the
 * periodic effect until it ends.
 */
void EmRathalos_Status4Downed(Em *em, EmWork *work)
{
    em->holdTimer = 5;
    Em_SetHitReactMode(em, EM_HIT_REACT_STATUS);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, RATH_MOT_DOWNED, 0, 0);
        Em_RestartScript(em);
        em->noReact = 1;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetTrapTimeGoal(em);
            EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_STATUS4_DOWNED_END, 3);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_STATUS4_DOWNED_END, 3);
        }
        break;
    }
    Em_SpawnStatus4Effect(em, 2);
}

/* RATH_REACT_PART_BREAK_END: ends the state. */
void EmRathalos_PartBreakEnd(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * RATH_REACT_KNOCK_DOWN_PART3: the reaction to a hit on the broken part 3
 * (motions 0x4A, 0x4B, 0x48).
 */
void EmRathalos_KnockDownPart3(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x4A, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 0;
            Em_SetMotion(em, 0x4B, 0, 0);
        }
        break;
    case 2:
        em->timer++;
        if (em->timer >= 0x78) {
            em->step++;
            Em_SetMotion(em, 0x48, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmRathalos_EndState(em, 0, 0);
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
        }
        break;
    }
}

/* RATH_REACT_STATUS4_DOWNED_END: struggles twice, then flies out. */
void EmRathalos_Status4DownedEnd(Em *em, EmWork *work)
{
    em->holdTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, RATH_MOT_STRUGGLE, 0, 0);
        em->noReact = 1;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            Em_SetMotion(em, RATH_MOT_STRUGGLE, 0, 0);
        }
        if (--em->trapTimer <= 0) {
            em->step++;
            EmRathalos_ChangeState(em, EM_GROUP_FLY, RATH_FLY_ESCAPE_UPWARD, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, EM_GROUP_FLY, RATH_FLY_ESCAPE_UPWARD, 4);
        }
        break;
    }
}

/*
 * RATH_REACT_STATUS2_DOWNED_END: gets up from the downed lying, then the
 * trap struggle.
 */
void EmRathalos_Status2DownedEnd(Em *em, EmWork *work)
{
    em->holdTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_DOWNED_GET_UP, 0, 0);
        em->blind = 2;
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetTrapTimeGoal(em);
            em->blind = 0;
            EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_TRAPPED_STRUGGLE, 3);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_TRAPPED_STRUGGLE, 3);
        }
        break;
    }
}

/*
 * RATH_REACT_STATUS1_DOWNED_END: as RATH_REACT_STATUS2_DOWNED_END after
 * status 1.
 */
void EmRathalos_Status1DownedEnd(Em *em, EmWork *work)
{
    em->holdTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_DOWNED_GET_UP, 0, 0);
        em->blind = 2;
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetTrapTimeGoal(em);
            em->blind = 0;
            EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_TRAPPED_STRUGGLE, 3);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_TRAPPED_STRUGGLE, 3);
        }
        break;
    }
}

/*
 * RATH_REACT_STUN: knocked down with the stun's stage object
 * (StunStars_CreateEm) until the stun (EM_STATUS_STUN) ends, then the
 * shake-off.
 */
void EmRathalos_Stun(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, RATH_MOT_KNOCK_DOWN, 0, 0);
        StunStars_CreateEm(em, 1, 0x22, (s32) ((f32) em->timer + (em->motion[0].frameCount / 2.0f)), 150.0f, 3.0f);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_DOWN, 0, 0);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_DOWN_GET_UP, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE_STUN_SHAKE_OFF, 4);
        }
        break;
    case 4:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE_STUN_SHAKE_OFF, 4);
        }
        break;
    }
}

/*
 * RATH_REACT_STUN_DOWNED: the stun in a hold: lies downed with the stun's
 * object.
 */
void EmRathalos_StunDowned(Em *em, EmWork *work)
{
    em->holdTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, RATH_MOT_DOWNED, 0, 0);
        Em_RestartScript(em);
        em->noReact = 1;
        StunStars_CreateEm(em, 1, 0x22, (s32) ((f32) em->timer + (em->motion[0].frameCount / 2.0f)), 150.0f, 3.0f);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetTrapTimeGoal(em);
            EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_STUN_DOWNED_END, 3);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_STUN_DOWNED_END, 3);
        }
        break;
    }
}

/* RATH_REACT_STUN_DOWNED_END: struggles twice, then flies out. */
void EmRathalos_StunDownedEnd(Em *em, EmWork *work)
{
    em->holdTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, RATH_MOT_STRUGGLE, 0, 0);
        em->noReact = 1;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            Em_SetMotion(em, RATH_MOT_STRUGGLE, 0, 0);
        }
        if (--em->trapTimer <= 0) {
            em->step++;
            EmRathalos_ChangeState(em, EM_GROUP_FLY, RATH_FLY_ESCAPE_UPWARD, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, EM_GROUP_FLY, RATH_FLY_ESCAPE_UPWARD, 4);
        }
        break;
    }
}

/*
 * RATH_REACT_HOLD2: held by the second hold (Em_SetHold2), the players
 * flagged while it lasts, then the shake-off.
 */
void EmRathalos_Hold2(Em *em, EmWork *work)
{
    Em_SetHold2(em);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x3C, 0, 0);
        Player_SetFlag((Player *) em, EM_FLAG_NO_ROOT_MOTION);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 52.0f) != 0) {
            em->step++;
            Em_SetMotion(em, 0x4C, 0xC, 0);
            Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE_HOLD2_SHAKE_OFF, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE_HOLD2_SHAKE_OFF, 4);
        }
        break;
    }
}

/*
 * RATH_EVENT_INTRO: the story quests' opening: flies in to the area's
 * point, swoops, lands, roars and feeds, the camera following.
 */
void EmRathalos_QuestIntro(Em *em, EmWork *work)
{
    f32 pos[3];
    f32 off[3];
    f32 out[3];
    s32 ang[3];
    f32 offsetB[3];
    f32 rotatedB[3];
    s32 anglesB[3];
    f32 mtx[16];
    f32 mtxB[16];
    Em *otherEm;
    s8 i;

    Stage_GetAreaRecord(em->area);
    for (i = 0, otherEm = gEms; i < 20; i++, otherEm++) {
        if ((otherEm->type == 0xC || otherEm->type == EM_KIND_POPO) && otherEm->active != 0 && otherEm->variant == 0) {
            goto found;
        }
    }
    otherEm = NULL;
found:
    if (otherEm == NULL) {
        Vec_Set(pos, 10000.0f, 0.0f, 10000.0f);
    } else {
        *(Em **) &otherEm->unk898[0] = em;
        Vec_Set(pos, otherEm->posX, otherEm->posY, otherEm->posZ);
    }
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        em->velZ = 100.0f;
        em->posX = 20000.0f;
        em->posY = 2000.0f;
        em->posZ = 12000.0f;
        em->goal[0] = 26500.0f;
        em->goal[1] = 2000.0f;
        em->goal[2] = 19000.0f;
        em->rotY = Math_AngleXZ(&em->posX, em->goal);
        GmEm_ResetSize(em);
        Em_SetMotion(em, RATH_MOT_FLY, 0, 0);
        break;
    case 1:
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 0xD5, &EmRathalos_FlyParam);
        if (Vec_DistanceXZ(&em->posX, em->goal) <= 1000.0f) {
            em->step++;
            em->timer = 900;
        }
        work->moveAngX = em->rotX;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
        break;
    case 2:
        if (80.0f < em->velZ) {
            em->velZ -= 1.0f;
        }
        em->aimAngle = Math_AngleXZ(&em->posX, pos);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 0xD1, &EmRathalos_FlyParam);
        if ((em->aimAngle <= 0x800 || em->aimAngle >= 0xF800) && 4000.0f < Vec_DistanceXZ(&em->posX, pos)) {
            em->step++;
            work->turnSpeed = 0x100;
            Em_SetMotion(em, RATH_MOT_SWOOP_DIVE, 0, 0);
            em->timer = 1800;
        }
        work->moveAngX = em->rotX;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
        break;
    case 3:
        if (em->rotZ != 0) {
            if (em->rotZ <= 0x8000) {
                em->rotZ -= work->rollSpeed;
            } else {
                em->rotZ += work->rollSpeed;
            }
        }
        if (EmMotion_ReachedFrame(em, 0, 46.0f) && 50.0f < em->velZ) {
            em->velZ -= 0.1f;
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_SWOOP, 0, 0);
            em->velZ = 100.0f;
            em->timer = 1800;
        }
        Vec_Set(em->goal, pos[0], pos[1], pos[2]);
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 0);
        work->moveAngX = em->rotX;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0xF;
        Em_MoveRotated(em, &work->moveAngX);
        em->posY -= 1.0f;
        break;
    case 4:
        if (em->posY <= 1000.0f) {
            em->step++;
            em->timer = 1800;
        }
        if (EmRathalos_ReachedPoint(em) && em->step == 4) {
            em->step++;
            em->timer = 1800;
        }
        Vec_Set(em->goal, pos[0], pos[1], pos[2]);
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 0);
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_GetPitchToGoal(em, &work->moveAngX, -3000.0f, 0.0f);
        Em_MoveRotated(em, &work->moveAngX);
        break;
    case 5:
        EmRathalos_ReachedPoint(em);
        Vec_Set(em->goal, pos[0], pos[1], pos[2]);
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 0);
        if (!(200.0f + em->goal[1] < em->posY)) {
            Em_GetPitchToGoal(em, &work->moveAngX, 0.0f, 0.0f);
        } else {
            Em_GetPitchToGoal(em, &work->moveAngX, -2000.0f, 0.0f);
        }
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
        if (Vec_DistanceXZ(&em->posX, pos) <= 600.0f ||
            (em->motionId[0] == EM_MOTION_ID(0, RATH_MOT_SWOOP_CLAW) && (EmMotion_ReachedFrame(em, 0, 52.0f) || em->motion[0].playing == 0))) {
            em->step++;
            em->posY = em->floorY;
            em->rotX = 0;
            em->rotZ = 0;
            em->posY = em->floorY;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, RATH_MOT_LEAP_LAND, 4, 0);
            if (otherEm != NULL) {
                ang[0] = 0;
                ang[1] = em->rotY;
                ang[2] = 0;
                Vec_Set(off, 0.0f, 0.0f, 600.0f);
                Mtx_FromAnglesYXZ(ang, mtx);
                Vec_Rotate(out, off, mtx);
                otherEm->posX = em->posX + out[0];
                otherEm->posZ = em->posZ + out[2];
                if (otherEm->stateGroup != EM_GROUP_EVENT || otherEm->state != 1) {
                    Em_SetStateRand(otherEm, 6, 1);
                    otherEm->rotY = (em->rotY - 0x4000) & 0xFFFF;
                }
            }
        }
        break;
    case 6:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_ROAR, 0, 0);
            if (otherEm != NULL) {
                Em_SetStateRand(otherEm, 6, 2);
            }
        }
        if (otherEm != NULL) {
            anglesB[0] = 0;
            anglesB[1] = em->rotY;
            anglesB[2] = 0;
            Vec_Set(offsetB, 0.0f, 0.0f, 600.0f);
            Mtx_FromAnglesYXZ(anglesB, mtxB);
            Vec_Rotate(rotatedB, offsetB, mtxB);
            otherEm->posX = em->posX + rotatedB[0];
            otherEm->posZ = em->posZ + rotatedB[2];
            otherEm->noPushTimer = 5;
        }
        break;
    case 7:
        if (EmMotion_PassedFrameScaled(em, 60.0f, 0) || EmMotion_PassedFrameScaled(em, 202.0f, 0)) {
            PartFx_SpawnForEm(em, 0x1C, 6);
        }
        if (EmMotion_PassedFrameScaled(em, 52.0f, 0) || EmMotion_PassedFrameScaled(em, 208.0f, 0)) {
            PartFx_SpawnForEm(em, 0x16, 6);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_HEAD_DOWN, 0xA, 0);
            if (otherEm != NULL) {
                Em_SetStateRand(otherEm, 6, 3);
            }
        }
        break;
    case 8:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_FEED, 0, 0);
        }
        break;
    case 9:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_TURN_IN_PLACE, 0, 0);
            if (otherEm != NULL) {
                otherEm->motionSpeed = 0.0f;
            }
        }
        break;
    case 10:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
    if (em->posY < em->floorY) {
        em->posY = em->floorY;
    }
}

/*
 * RATH_EVENT_CAPTURED: status 1 in holdTimer's hold: lies downed; once
 * QuestEm_CanAppear allows it, the capture (QuestEm_Captured) and
 * RATH_DEFEAT_REAPPEAR.
 */
void EmRathalos_Captured(Em *em)
{
    f32 pos[3];

    Em_SetCaptured(em, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->stateVar0 = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_DOWNED, 0, 0);
        Em_LoadReactionSet(em, 1);
        Em_BeginStatus1(em);
        Em_ClearPoison(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            QuestEm_Captured(em);
            em->timer = 1800;
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
                EmRathalos_ChangeState(em, EM_GROUP_DEFEAT, RATH_DEFEAT_REAPPEAR, 4);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    }
    if (em->step < 3) {
        pos[0] = 0.0f;
        pos[1] = 10.0f;
        pos[2] = 140.0f;
        Em_SpawnLyingPuffs(em, 0x22, pos, 1.6f);
        if (++em->stateVar0 % 135 == 0) {
            Em_PlaySeAtBone(em, 0x57, 0x23);
        }
    }
}

/*
 * RATH_EVENT_CAPTURED_HOLD2: the capture in the second hold, collapsing
 * first.
 */
void EmRathalos_CapturedHold2(Em *em)
{
    s32 timer;
    s32 timerLeft;

    Em_SetCaptured(em, 1);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_COLLAPSE, 0, 0);
        Em_LoadReactionSet(em, 1);
        Em_BeginStatus1(em);
        Em_ClearPoison(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_LIE, 0, 0);
            QuestEm_Captured(em);
            em->timer = 0x708;
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
        timerLeft = em->timer;
        if (timerLeft > 0) {
            em->timer = timerLeft - 1;
        }
        timer = em->timer;
        if (timer <= 0) {
            if (em->remote == 0) {
                EmRathalos_ChangeState(em, EM_GROUP_DEFEAT, RATH_DEFEAT_REAPPEAR, 4);
                break;
            }
        } else {
            em->alpha = (f32) timer / 150.0f;
        }
        break;
    }
}

/*
 * RATH_EVENT_STORY: the Rathian kinds' story scene (started through
 * EmHermitaur_ChangeState in stages QUEST_ID_RATHIAN_SCENE_1/QUEST_ID_RATHIAN_SCENE_2): walks, runs at the point
 * and trips, the camera shaking.
 */
void EmRathalos_StoryEvent(Em *em)
{
    f32 goalDistXZ;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        em->fieldCollide = 0;
        em->posX = 14350.0f;
        em->posY = 0.0f;
        em->posZ = 9830.0f;
        em->goal[0] = 11000.0f;
        em->goal[1] = 0.0f;
        em->goal[2] = 10870.0f;
        em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        GmEm_ResetSize(em);
        Em_SetMotion(em, RATH_MOT_IDLE, 0, 0);
        em->timer = 120;
        em->stateVar0 = 0;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_WALK, 0, 0);
            em->timer = 900;
        }
        break;
    case 2:
        goalDistXZ = Vec_DistanceXZ(&em->posX, em->goal);
        em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        if (--em->timer <= 0 || goalDistXZ <= 150.0f) {
            em->step++;
            em->timer = 100;
            Em_SetMotion(em, 0x51, 0, 0);
            em->timer = 90;
            em->posX = 8551.0f;
            em->posY = 0.0f;
            em->posZ = 9386.0f;
            em->rotY = 0xB000;
        } else if (EmMotion_PassedFrameScaled(em, 32.0f, 0U) != 0) {
            CamShake_ReqC(em, 2);
        } else if (EmMotion_PassedFrameScaled(em, 88.0f, 0U) != 0) {
            CamShake_ReqC(em, 2);
        }
        break;
    case 3:
        if (--em->timer <= 0) {
            em->step++;
            em->timer = 100;
            Em_SetMotion(em, 0x50, 0, 0);
        }
        break;
    case 4:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotionRow(em, 0x33, 0, 0);
            em->posX = 8384.0f;
            em->posY = 0.0f;
            em->posZ = 9396.0f;
            em->rotY = 0x5000;
        }
        break;
    case 5:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_RUN, 0, 0);
            em->posX = 7826.0f;
            em->posY = 0.0f;
            em->posZ = 10289.0f;
            em->goal[0] = 10550.0f;
            em->goal[1] = 0.0f;
            em->goal[2] = 9650.0f;
            em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        }
        break;
    case 6:
        goalDistXZ = Vec_DistanceXZ(&em->posX, em->goal);
        em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        if (--em->timer <= 0 || goalDistXZ <= 300.0f) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_CHARGE_TRIP, 0, 0);
        }
        break;
    case 7:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_ALERT_IDLE, 0, 0);
        }
        break;
    case 8:
        if (Save_TestEventFlag(EVENT_FLAG_RATHIAN_SCENE) == 1) {
            em->step++;
            em->senseDelay = 0;
            em->noHitTimer = 0;
            em->fieldCollide = 1;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * RATH_VS_EAT_BAIT: at the versus match's end, after the defeat camera,
 * eats the bait thrown from the winner's side (VersusBait_Create), then
 * routes the monster by team.
 */
void EmRathalos_VersusEatBait(Em *em, EmWork *work)
{
    Vec3f throwPoint;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        if (Versus_DefeatCamera(em) == 1) {
            em->step++;
            Em_SetMotion(em, 0x1A, 0, 0);
            Em_SetLayerMotion(em, EM_MOTION_ID(0, RATH_MOT_IDLE), 0, 0, 0);
            Em_SetLayerMotion(em, EM_MOTION_ID(0, RATH_MOT_IDLE), 0, 0, 2);
            em->timer = 30;
        }
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0) {
            throwPoint = EmRathalos_CarveThrowPt;
            VersusBait_Create(&gPlayers[(u8) em->side - 1], em, 0, 0, &throwPoint.x);
        }
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_HEAD_DOWN, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_FEED, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 30;
            Em_SetMotion(em, RATH_MOT_ALERT_IDLE, 0, 0);
        }
        break;
    case 4:
        if (--em->timer <= 0) {
            Versus_RouteEmByTeam(em);
        }
        break;
    }
}

/*
 * RATH_VS_POSE_A: motion 0x3D, then waits for both sides
 * (RATH_VS_WAIT_BOTH).
 */
void EmRathalos_VersusPoseA(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x3D, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_IDLE, 0, 0);
            EmRathalos_ChangeState(em, EM_GROUP_VERSUS, RATH_VS_WAIT_BOTH, 1);
        }
        break;
    }
}

/* RATH_VS_POSE_B: motion 0x36, then waits for both sides. */
void EmRathalos_VersusPoseB(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, RATH_MOT_TURN_IN_PLACE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_IDLE, 0, 0);
            EmRathalos_ChangeState(em, EM_GROUP_VERSUS, RATH_VS_WAIT_BOTH, 1);
        }
        break;
    }
}

/*
 * RATH_VS_POSE_C: a somersault for the Rathian kinds, a fireball for the
 * Rathalos kinds, then waits for both sides.
 */
void EmRathalos_VersusPoseC(Em *em, EmWork *work)
{
    s32 done = 0;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    if (em->type == EM_KIND_RATHIAN || em->type == EM_KIND_GOLD_RATHIAN || em->type == EM_KIND_PINK_RATHIAN) {
        switch (em->step) {
        case 0:
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->lookAtTarget = 0;
            Em_SetMotion(em, RATH_MOT_FLIP, 0, 0);
            EmRathalos_StartCurves(em, 0xA);
            break;
        case 1:
            if (EmMotion_ReachedFrame(em, 0, 94.0f) != 0) {
                em->stance = EM_STANCE_AIR;
                done = EmRathalos_UpdateCurves(em);
            }
            if (done != 0 && em->posY <= 372.0f + em->floorY) {
                em->step++;
                Em_SetMotion(em, RATH_MOT_FLIP_FALL, 0, 0);
                EmRathalos_StartCurves(em, 0xB);
            }
            break;
        case 2:
            EmRathalos_UpdateCurves(em);
            if (em->posY <= em->floorY) {
                em->step++;
                Em_SetMotion(em, RATH_MOT_FLIP_LAND, 0, 0);
                em->stance = EM_STANCE_GROUND;
                em->posY = em->floorY;
                EmRathalos_SpawnLandingDust(em);
                CamShake_ReqAIf(em, 2);
            }
            break;
        case 3:
            if (em->motion[0].playing == 0) {
                em->step++;
                Em_SetMotion(em, RATH_MOT_IDLE, 0, 0);
                EmRathalos_ChangeState(em, EM_GROUP_VERSUS, RATH_VS_WAIT_BOTH, 1);
            }
            break;
        }
        if (em->posY < em->floorY) {
            em->posY = em->floorY;
        }
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->lookAtTarget = 0;
            Em_SetMotion(em, RATH_MOT_FIREBALL, 0, 0);
            break;
        case 1:
            if (em->motion[1].playing == 0) {
                em->step++;
                Em_SetMotion(em, RATH_MOT_IDLE, 0, 0);
                EmRathalos_ChangeState(em, EM_GROUP_VERSUS, RATH_VS_WAIT_BOTH, 1);
            }
            break;
        }
    }
}

/*
 * RATH_VS_WAIT_BOTH: idles until both sides are defeated
 * (Versus_BothDefeated), then RATH_VS_END.
 */
void EmRathalos_VersusWaitBoth(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        if (em->motionId[0] != EM_MOTION_ID(0, RATH_MOT_IDLE)) {
            Em_SetMotion(em, RATH_MOT_IDLE, 0, 0);
        }
        
    case 1:
        if (Versus_BothDefeated() == 1) {
            em->step++;
            EmRathalos_ChangeState(em, EM_GROUP_VERSUS, RATH_VS_END, 1);
        }
        break;
    }
}

/* RATH_VS_END: idles 90 frames, then ends the state. */
void EmRathalos_VersusEnd(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 0x5A;
        if (em->motionId[0] != EM_MOTION_ID(0, RATH_MOT_IDLE)) {
            Em_SetMotion(em, RATH_MOT_IDLE, 0, 0);
        }
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            em->senseDelay = 0;
            em->noHitTimer = 0;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * RATH_DEFEAT_DIE: falls dead (QuestEm_Hunted), leaves a carving mining
 * point while it lies, then disappears or reappears (RATH_DEFEAT_REAPPEAR).
 */
void EmRathalos_Die(Em *em, EmWork *work)
{
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, RATH_MOT_DIE, 0, 0);
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
                em->timer = 150;
                EmMine_Remove(em);
            }
        }
        break;
    case 3:
        if (em->timer > 0) {
            em->timer--;
        }
        if (em->timer <= 0) {
            if (em->remote == 0) {
                EmRathalos_ChangeState(em, EM_GROUP_DEFEAT, RATH_DEFEAT_REAPPEAR, 4);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    case 99:
        if (em->motion[0].playing == 0) {
            em->stance = EM_STANCE_GROUND;
            EmRathalos_EndState(em, 0, 0);
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
        }
        break;
    }
}

/* RATH_DEFEAT_DIE_HELD: dies lying downed in a hold, as RATH_DEFEAT_DIE. */
void EmRathalos_DieHeld(Em *em, EmWork *work)
{
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, RATH_MOT_DOWNED, 0, 0);
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_DEAD;
        QuestEm_Hunted(em);
        em->holdTimer = 5;
        break;
    case 1:
        em->holdTimer = 5;
        if (em->motion[0].playing == 0) {
            em->step++;
            EmMine_Create(em, 0);
            em->fieldCollide = 0;
            em->timer = 1800;
        }
        break;
    case 2:
        EmMine_Update(em);
        em->holdTimer = 5;
        if (QuestEm_CanAppear(em) != 0) {
            if (--em->timer <= 0) {
                em->step++;
                em->holdTimer = 0;
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
                EmRathalos_ChangeState(em, EM_GROUP_DEFEAT, RATH_DEFEAT_REAPPEAR, 4);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    case 99:
        if (em->motion[0].playing == 0) {
            em->stance = EM_STANCE_GROUND;
            EmRathalos_EndState(em, 0, 0);
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
        }
        break;
    }
}

/*
 * RATH_DEFEAT_DIE_FALLING: dies in the air: falls, hits the floor, and lies
 * as RATH_DEFEAT_DIE.
 */
void EmRathalos_DieFalling(Em *em, EmWork *work)
{
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_FALL, 0, 0);
        if (em->velY > -50.0f) {
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
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, RATH_MOT_FALL_LAND, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    case 2:
        if (EmMotion_PassedFrameScaled(em, 60.0f, 0) != 0) {
            em->step++;
            em->timer = 0;
            Em_SetMotion(em, RATH_MOT_DIE_FALLING, 0, 0);
            em->stance = EM_STANCE_DEAD;
            QuestEm_Hunted(em);
        }
        break;
    case 3:
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
        if (em->timer <= 0) {
            if (em->remote == 0) {
                EmRathalos_ChangeState(em, EM_GROUP_DEFEAT, RATH_DEFEAT_REAPPEAR, 4);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    case 99:
        if (em->motion[0].playing == 0) {
            em->stance = EM_STANCE_GROUND;
            EmRathalos_EndState(em, 0, 0);
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
        }
        break;
    }
}

/*
 * RATH_DEFEAT_REAPPEAR: when the quest lets it appear again, flies in and
 * lands.
 */
void EmRathalos_Reappear(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        if (QuestEm_CanAppear(em) == 1) {
            em->posY = gAreaGroundY[em->area];
            em->stance = EM_STANCE_AIR;
            EmRathalos_ChangeState(em, EM_GROUP_FLY, RATH_FLY_LAND, 0);
            break;
        }
        em->taskPhase++;
    case 1:
        break;
    }
}

/* Runs the current group-0 state (EM_GROUP_IDLE). */
void EmRathalos_RunIdleState(Em *em, EmWork *work)
{
    switch (em->state) {
    case RATH_IDLE_STAND:
        EmRathalos_Stand(em);
        break;
    case RATH_IDLE:
        EmRathalos_Idle(em);
        break;
    case RATH_IDLE_ACTION_A:
        EmRathalos_IdleActionA(em);
        break;
    case RATH_IDLE_ACTION_B:
        EmRathalos_IdleActionB(em);
        break;
    case RATH_IDLE_ACTION_C:
        EmRathalos_IdleActionC(em);
        break;
    case RATH_IDLE_ACTION_D:
        EmRathalos_IdleActionD(em);
        break;
    case RATH_IDLE_ACTION_E:
        EmRathalos_IdleActionE(em);
        break;
    case RATH_IDLE_ACTION_F:
        EmRathalos_IdleActionF(em);
        break;
    case RATH_IDLE_ACTION_G:
        EmRathalos_IdleActionG(em);
        break;
    case RATH_IDLE_PAIR_A:
        EmRathalos_IdlePairA(em);
        break;
    case RATH_IDLE_TURN_TO_GOAL:
        EmRathalos_IdleTurnToGoal(em);
        break;
    case RATH_IDLE_ACTION_H:
        EmRathalos_IdleActionH(em);
        break;
    case RATH_IDLE_ACTION_I:
        EmRathalos_IdleActionI(em);
        break;
    case RATH_IDLE_ACTION_J:
        EmRathalos_IdleActionJ(em);
        break;
    case RATH_IDLE_EAT_NEED2:
        EmRathalos_EatNeed2(em);
        break;
    case RATH_IDLE_PAIR_B:
        EmRathalos_IdlePairB(em);
        break;
    case RATH_IDLE_PAIR_C:
        EmRathalos_IdlePairC(em);
        break;
    case RATH_IDLE_ALERT:
        EmRathalos_AlertIdle(em);
        break;
    case RATH_IDLE_REST:
        EmRathalos_Rest(em, work);
        break;
    case RATH_IDLE_EAT_START:
        EmRathalos_EatStart(em);
        break;
    case RATH_IDLE_STATUS2_COLLAPSE:
        EmRathalos_Status2Collapse(em);
        break;
    case RATH_IDLE_STATUS2_LIE_DOWN:
        EmRathalos_Status2LieDown(em, work);
        break;
    case RATH_IDLE_ACTION_K:
        EmRathalos_IdleActionK(em);
        break;
    case RATH_IDLE_REST_END:
        EmRathalos_RestEnd(em, work);
        break;
    case RATH_IDLE_STATUS2_END:
        EmRathalos_Status2End(em);
        break;
    case RATH_IDLE_SHAKE_OFF:
        EmRathalos_ShakeOff(em);
        break;
    case RATH_IDLE_EAT_NEED1:
        EmRathalos_EatNeed1(em);
        break;
    case RATH_IDLE_STATUS1_COLLAPSE:
        EmRathalos_Status1Collapse(em);
        break;
    case RATH_IDLE_STATUS1_END:
        EmRathalos_Status1End(em);
        break;
    case RATH_IDLE_STATUS2_DOWNED:
        EmRathalos_Status2Downed(em, work);
        break;
    case RATH_IDLE_STATUS1_DOWNED:
        EmRathalos_Status1Downed(em, work);
        break;
    case RATH_IDLE_STATUS4_SHAKE_OFF:
        EmRathalos_ShakeOffAfterStatus(em, work, 0);
        break;
    case RATH_IDLE_STUN_SHAKE_OFF:
        EmRathalos_ShakeOffAfterStatus(em, work, 1);
        break;
    case RATH_IDLE_HOLD2_SHAKE_OFF:
        EmRathalos_ShakeOffAfterStatus(em, work, 2);
        break;
    case RATH_IDLE_EAT:
        EmRathalos_Eat(em);
        break;
    }
}

/* Runs the current group-1 state (EM_GROUP_MOVE). */
void EmRathalos_RunMoveState(Em *em, EmWork *work)
{
    switch (em->state) {
    case RATH_MOVE_WALK:
        EmRathalos_Walk(em, work);
        break;
    case RATH_MOVE_END:
        EmRathalos_MoveEnd(em, work);
        break;
    case RATH_MOVE_END2:
        EmRathalos_MoveEnd2(em, work);
        break;
    case RATH_MOVE_TURN:
        EmRathalos_Turn(em, work);
        break;
    case RATH_MOVE_RUN:
        EmRathalos_Run(em, work);
        break;
    case RATH_MOVE_TURN_IN_COMBAT:
        EmRathalos_TurnInCombat(em, work);
        break;
    case RATH_MOVE_LIMP:
        EmRathalos_Limp(em, work);
        break;
    case RATH_MOVE_TURN_TO_GOAL:
        EmRathalos_TurnToGoal(em, work);
        break;
    case RATH_MOVE_RUN_SLOW:
        EmRathalos_Run(em, work);
        break;
    case RATH_MOVE_TURN_RAGE:
        EmRathalos_TurnToGoal(em, work);
        break;
    case RATH_MOVE_TURN_PENDING_RAGE:
        EmRathalos_TurnToGoal(em, work);
        
    default:
        break;
    }
}

/* Runs the current group-2 state (EM_GROUP_FLY). */
void EmRathalos_RunFlyState(Em *em, EmWork *work)
{
    switch (em->state) {
    case RATH_FLY_TAKE_OFF:
        EmRathalos_TakeOff(em, work);
        break;
    case RATH_FLY_LAND:
        EmRathalos_Land(em, work);
        break;
    case RATH_FLY_HOVER:
        EmRathalos_Hover(em, work);
        break;
    case RATH_FLY_HOP_TO_GOAL:
        EmRathalos_HopToGoal(em, work);
        break;
    case RATH_FLY_HOP_FORWARD:
        EmRathalos_HopForward(em, work);
        break;
    case RATH_FLY_HOP_BACK:
        EmRathalos_HopBack(em, work);
        break;
    case RATH_FLY_ROUTE:
        EmRathalos_FlyRoute(em, work);
        break;
    case RATH_FLY_HOVER_TO_GOAL:
        EmRathalos_HoverToGoal(em, work);
        break;
    case RATH_FLY_OUT_OF_AREA:
        EmRathalos_FlyOutOfArea(em, work);
        break;
    case RATH_FLY_TO_GOAL_AND_LAND:
        EmRathalos_FlyToGoalAndLand(em, work);
        break;
    case RATH_FLY_OUT_OF_AREA_ALT:
        EmRathalos_FlyOutOfAreaAlt(em, work);
        break;
    case RATH_FLY_GLIDE_TO_GOAL:
        EmRathalos_GlideToGoal(em, work);
        break;
    case RATH_FLY_BACK_HOP:
        EmRathalos_BackHop(em);
        break;
    case RATH_FLY_INTO_AREA:
        EmRathalos_FlyIntoArea(em, work);
        break;
    case RATH_FLY_HOVER_ONCE:
        EmRathalos_HoverOnce(em, work);
        break;
    case RATH_FLY_GLIDE_ONCE:
        EmRathalos_GlideOnce(em, work);
        break;
    case RATH_FLY_HOVER_TURN:
        EmRathalos_HoverTurn(em, work);
        break;
    case RATH_FLY_ROUTE_B:
        EmRathalos_FlyRouteB(em, work);
        break;
    case RATH_FLY_LEVEL_OUT_AND_LAND:
        EmRathalos_LevelOutAndLand(em, work);
        break;
    case RATH_FLY_GLIDE_TO_GOAL_B:
        EmRathalos_GlideToGoalB(em, work);
        break;
    case RATH_FLY_END:
        EmRathalos_EndFlight(em);
        break;
    case RATH_FLY_HOVER_STRAIGHT_TO_GOAL:
        EmRathalos_HoverStraightToGoal(em, work);
        break;
    case RATH_FLY_LEVEL_OUT:
        EmRathalos_LevelOut(em, work);
        break;
    case RATH_FLY_ESCAPE_UPWARD:
        EmRathalos_EscapeUpward(em, work);
        break;
    case RATH_FLY_HOVER_RISE:
        EmRathalos_HoverRise(em, work);
        
    default:
        break;
    }
}

/* Runs the current group-3 state (EM_GROUP_ATTACK). */
void EmRathalos_RunAttackState(Em *em, EmWork *work)
{
    switch (em->state) {
    case RATH_ATK_TURN_AND_STARE:
        EmRathalos_TurnAndStare(em);
        break;
    case RATH_ATK_LEAP:
        EmRathalos_Leap(em, work);
        break;
    case RATH_ATK_STARE:
        EmRathalos_Stare(em);
        break;
    case RATH_ATK_FIREBALL:
        EmRathalos_Fireball(em, work);
        break;
    case RATH_ATK_ROAR:
        EmRathalos_Roar(em);
        break;
    case RATH_ATK_TAIL_SPIN:
        EmRathalos_TailSpin(em);
        break;
    case RATH_ATK_LIE_AND_RISE:
        EmRathalos_LieAndRise(em);
        break;
    case RATH_ATK_SWOOP_THEN_LEAVE:
        EmRathalos_SwoopThenLeave(em, work);
        break;
    case RATH_ATK_AIR_FIREBALL:
        EmRathalos_AirFireball(em, work);
        break;
    case RATH_ATK_AIR_FIREBALL_B:
        EmRathalos_AirFireballB(em, work);
        break;
    case RATH_ATK_AIR_DIVE:
        EmRathalos_AirDive(em, work);
        break;
    case RATH_ATK_ROAR_AT_POINT:
        EmRathalos_RoarAtPoint(em, work);
        break;
    case RATH_ATK_SINGLE_A:
        EmRathalos_SingleAttackA(em);
        break;
    case RATH_ATK_SINGLE_B:
        EmRathalos_SingleAttackB(em);
        break;
    case RATH_ATK_CHARGE:
        EmRathalos_Charge(em);
        break;
    case RATH_ATK_BACK_HOP_FIREBALL:
        EmRathalos_BackHopFireball(em, work, 0);
        break;
    case RATH_ATK_AIR_FIREBALL_DOUBLE:
        EmRathalos_AirFireballDouble(em, work);
        break;
    case RATH_ATK_CHARGE_TRIP:
        EmRathalos_ChargeTrip(em);
        break;
    case RATH_ATK_ROAR_B:
        EmRathalos_RoarB(em);
        break;
    case RATH_ATK_SOMERSAULT:
        EmRathalos_Somersault(em);
        break;
    case RATH_ATK_SWOOP_THEN_LAND:
        EmRathalos_SwoopThenLand(em, work);
        break;
    case RATH_ATK_AIR_FIREBALL_DOUBLE_B:
        EmRathalos_AirFireballDoubleB(em, work);
        break;
    case RATH_ATK_FIREBALL_B:
        EmRathalos_FireballB(em);
        break;
    case RATH_ATK_FIREBALL_LOW:
        EmRathalos_FireballLow(em, work);
        break;
    case RATH_ATK_FIREBALL_HIGH:
        EmRathalos_FireballHigh(em, work);
        break;
    case RATH_ATK_TAIL_SPIN_B:
        EmRathalos_TailSpinB(em);
        break;
    case RATH_ATK_RUN_AND_STOP:
        EmRathalos_RunAndStop(em);
        break;
    case RATH_ATK_AIR_FIREBALL_FORWARD:
        EmRathalos_AirFireballForward(em, (s32) work);
        break;
    case RATH_ATK_AIR_FIREBALL_DOUBLE_FORWARD:
        EmRathalos_AirFireballDoubleForward(em, (s32) work);
        break;
    case RATH_ATK_SOMERSAULT_DOUBLE:
        EmRathalos_SomersaultDouble(em);
        break;
    case RATH_ATK_BACK_HOP_FIREBALL_LEFT:
        EmRathalos_BackHopFireball(em, work, 1);
        break;
    case RATH_ATK_BACK_HOP_FIREBALL_RIGHT:
        EmRathalos_BackHopFireball(em, work, 2);
        break;
    }
}

/* Runs the current group-4 state (EM_GROUP_REACT). */
void EmRathalos_RunReactState(Em *em, EmWork *work)
{
    switch (em->state) {
    case RATH_REACT_FLINCH:
        EmRathalos_Flinch(em, work);
        break;
    case RATH_REACT_FLINCH_B:
        EmRathalos_FlinchB(em, work);
        break;
    case RATH_REACT_FLINCH_C:
        EmRathalos_FlinchC(em, work);
        break;
    case RATH_REACT_FLINCH_D:
        EmRathalos_FlinchD(em, work);
        break;
    case RATH_REACT_PART_BREAK:
        EmRathalos_PartBreak(em, work);
        break;
    case RATH_REACT_KNOCK_DOWN:
        EmRathalos_KnockDown(em, work);
        break;
    case RATH_REACT_END_AIR:
        EmRathalos_ReactEndAir(em, work);
        break;
    case RATH_REACT_END_AIR_B:
        EmRathalos_ReactEndAirB(em, work);
        break;
    case RATH_REACT_FALL:
        EmRathalos_Fall(em, work);
        break;
    case RATH_REACT_FALL_B:
        EmRathalos_FallB(em, work);
        break;
    case RATH_REACT_FLINCH_E:
        EmRathalos_FlinchE(em, work);
        break;
    case RATH_REACT_STATUS4:
        EmRathalos_Status4(em, work);
        break;
    case RATH_REACT_TRAPPED:
        EmRathalos_Trapped(em, work);
        break;
    case RATH_REACT_TRAPPED_STRUGGLE:
        EmRathalos_TrappedStruggle(em, work);
        break;
    case RATH_REACT_STATUS4_DOWNED:
        EmRathalos_Status4Downed(em, work);
        break;
    case RATH_REACT_PART_BREAK_END:
        EmRathalos_PartBreakEnd(em, work);
        break;
    case RATH_REACT_KNOCK_DOWN_PART3:
        EmRathalos_KnockDownPart3(em, work);
        break;
    case RATH_REACT_STATUS4_DOWNED_END:
        EmRathalos_Status4DownedEnd(em, work);
        break;
    case RATH_REACT_STATUS2_DOWNED_END:
        EmRathalos_Status2DownedEnd(em, work);
        break;
    case RATH_REACT_STATUS1_DOWNED_END:
        EmRathalos_Status1DownedEnd(em, work);
        break;
    case RATH_REACT_STUN:
        EmRathalos_Stun(em, work);
        break;
    case RATH_REACT_STUN_DOWNED:
        EmRathalos_StunDowned(em, work);
        break;
    case RATH_REACT_STUN_DOWNED_END:
        EmRathalos_StunDownedEnd(em, work);
        break;
    case RATH_REACT_HOLD2:
        EmRathalos_Hold2(em, work);
        
    default:
        break;
    }
}

/* Runs the current group-5 state (EM_GROUP_DEFEAT). */
void EmRathalos_RunDefeatState(Em *em, EmWork *work)
{
    switch (em->state) {
    case RATH_DEFEAT_DIE:
        EmRathalos_Die(em, work);
        break;
    case RATH_DEFEAT_DIE_HELD:
        EmRathalos_DieHeld(em, work);
        break;
    case RATH_DEFEAT_DIE_FALLING:
        EmRathalos_DieFalling(em, work);
        break;
    case RATH_DEFEAT_REAPPEAR:
        EmRathalos_Reappear(em, work);
        break;
    }
}

/* Runs the current group-6 or group-7 state (EM_GROUP_EVENT). */
void EmRathalos_RunEventState(Em *em, EmWork *work)
{
    switch (em->state) {
    case RATH_EVENT_INTRO:
        EmRathalos_QuestIntro(em, work);
        break;
    case RATH_EVENT_CAPTURED:
        EmRathalos_Captured(em);
        break;
    case RATH_EVENT_CAPTURED_HOLD2:
        EmRathalos_CapturedHold2(em);
        break;
    case RATH_EVENT_STORY:
        EmRathalos_StoryEvent(em);
        break;
    }
}

/* Runs the current group-8 state (EM_GROUP_VERSUS). */
void EmRathalos_RunVersusState(Em *em, EmWork *work)
{
    switch (em->state) {
    case RATH_VS_EAT_BAIT:
        EmRathalos_VersusEatBait(em, work);
        break;
    case RATH_VS_POSE_A:
        EmRathalos_VersusPoseA(em, work);
        break;
    case RATH_VS_POSE_B:
        EmRathalos_VersusPoseB(em, work);
        break;
    case RATH_VS_POSE_C:
        EmRathalos_VersusPoseC(em, work);
        break;
    case RATH_VS_WAIT_BOTH:
        EmRathalos_VersusWaitBoth(em, work);
        break;
    case RATH_VS_END:
        EmRathalos_VersusEnd(em, work);
        break;
    }
}

/*
 * The family's frame: the combat timer and the needs (by area kind), then
 * Em_UpdateDamage's result picks a reaction state (see EM_GROUP_REACT's
 * enum); the story quests' area events; otherwise the pending script, then
 * the current state (twice when a state was requested). In flight states it
 * keeps guardTimer at 2.
 */
void EmRathalos_Update(Em *em)
{
    u8 buf[4];
    EmFlyerWork *w = (EmFlyerWork *)&em->work;
    f32 ratio;

    Em_UpdateCombatTimer(em);
    Em_RaisePlayerAreaTriggerLax(em);
    if (em->type == EM_KIND_RATHIAN || em->type == EM_KIND_GOLD_RATHIAN || em->type == EM_KIND_PINK_RATHIAN) {
        ratio = 0.1f;
    } else {
        ratio = 0.3f;
    }
    if (em->noAlert != 1 && em->rage == 0) {
        switch (Map_DayId(gScene.map)) {
        case MAP_FOREST_HILLS:
            if (em->questKey == 1) {
                if (em->type == EM_KIND_RATHIAN || em->type == EM_KIND_GOLD_RATHIAN || em->type == EM_KIND_PINK_RATHIAN) {
                    Em_CheckLowHp(em, ratio);
                    Em_DecayNeed3(em);
                } else {
                    Em_DecayNeed2(em);
                }
                Em_DecayNeed1Only(em);
            } else {
                Em_CheckLowHp(em, ratio);
                Em_DecayNeed2(em);
                Em_DecayNeed1(em);
                Em_DecayNeed3(em);
            }
            break;
        case MAP_JUNGLE:
            Em_CheckLowHp(em, ratio);
            Em_DecayNeed2(em);
            Em_DecayNeed1(em);
            Em_DecayNeed3(em);
            Em_CheckCarryRequest(em);
            break;
        case MAP_DESERT:
            Em_CheckCarryRequest(em);
        case MAP_SWAMP:
            Em_CheckLowHp(em, ratio);
            Em_DecayNeed2(em);
            Em_DecayNeed1Only(em);
            break;
        }
    }
    if (w->fxMotion != 0) {
        w->fxMotion -= 1;
    }
    switch (Em_UpdateDamage(em, buf) & 0xFF) {
    case EM_DMG_NONE:
    case EM_DMG_FLASH:
    case EM_DMG_HIT:
    case EM_DMG_ATTACK5:
    case EM_DMG_ATTACK5_FLINCH:
        break;
    case EM_DMG_KILLED:
    case EM_DMG_DEAD:
        if (em->stance == EM_STANCE_AIR) {
            EmRathalos_ChangeState(em, EM_GROUP_DEFEAT, RATH_DEFEAT_DIE_FALLING, 3);
        } else if ((s8) em->holdTimer != 0) {
            EmRathalos_ChangeState(em, EM_GROUP_DEFEAT, RATH_DEFEAT_DIE_HELD, 3);
        } else {
            EmRathalos_ChangeState(em, EM_GROUP_DEFEAT, RATH_DEFEAT_DIE, 3);
        }
        break;
    case EM_DMG_ATTACK9:
    case EM_DMG_ATTACK6:
        if ((s8) em->holdTimer == 0) {
            Em_SetTrapTime(em, buf);
            Em_SetTrapTimeGoal(em);
            EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_TRAPPED, 3);
        }
        break;
    case EM_DMG_STATUS_IN_AIR:
        if (em->stateGroup != EM_GROUP_REACT || em->state != RATH_REACT_FALL) {
            EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_FALL, 3);
        }
        break;
    case EM_DMG_STATUS4:
        if ((s8) em->holdTimer != 0) {
            Em_SetStatus4Timer(em);
            EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_STATUS4_DOWNED, 3);
        } else if ((em->stateGroup != EM_GROUP_REACT || em->state != RATH_REACT_STATUS4) && (em->stateGroup != EM_GROUP_REACT || em->state != RATH_REACT_FALL)) {
            Em_SetStatus4Timer(em);
            EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_STATUS4, 3);
        }
        break;
    case EM_DMG_CAPTURED:
        if (Em_IsHeld(em) == 1) {
            Em_SetStatus1Timer(em);
            if ((s8) em->holdTimer != 0) {
                EmRathalos_ChangeState(em, EM_GROUP_EVENT, RATH_EVENT_CAPTURED, 3);
                break;
            }
            if ((s8) em->holdTimer2 != 0) {
                EmRathalos_ChangeState(em, EM_GROUP_EVENT, RATH_EVENT_CAPTURED_HOLD2, 3);
                break;
            }
        }
        if ((em->stateGroup != EM_GROUP_IDLE || em->state != RATH_IDLE_STATUS1_COLLAPSE) && (em->stateGroup != EM_GROUP_REACT || em->state != RATH_REACT_FALL)) {
            Em_SetStatus1Timer(em);
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE_STATUS1_COLLAPSE, 3);
        }
        break;
    case EM_DMG_STATUS2:
        if ((s8) em->holdTimer != 0) {
            Em_SetStatus2Timer(em);
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE_STATUS2_DOWNED, 3);
        } else if ((s8) em->holdTimer2 != 0) {
            Em_SetStatus2Timer(em);
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE_STATUS2_COLLAPSE, 3);
        } else if ((em->stateGroup != EM_GROUP_IDLE || em->state != RATH_IDLE_STATUS2_COLLAPSE) && (em->stateGroup != EM_GROUP_REACT || em->state != RATH_REACT_FALL)) {
            Em_SetStatus2Timer(em);
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE_STATUS2_COLLAPSE, 3);
        }
        break;
    case EM_DMG_HIT_LYING:
        switch (em->state) {
        case RATH_IDLE_REST:
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE_REST_END, 3);
            em->scriptPending = 0;
            Em_CheckRageByHp(em, em->rageThreshold);
            break;
        case RATH_IDLE_STATUS2_COLLAPSE:
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE_STATUS2_END, 3);
            em->scriptPending = 0;
            break;
        case RATH_IDLE_STATUS1_COLLAPSE:
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE_STATUS1_END, 3);
            em->scriptPending = 0;
            break;
        case RATH_IDLE_STATUS2_DOWNED:
            EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_STATUS2_DOWNED_END, 3);
            em->scriptPending = 0;
            break;
        case RATH_IDLE_STATUS1_DOWNED:
            EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_STATUS1_DOWNED_END, 3);
            em->scriptPending = 0;
            break;
        }
        break;
    case EM_DMG_PART8_DOWN:
        EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_PART_BREAK, 3);
        break;
    case EM_DMG_PART_FLINCH:
        if (em->stance == EM_STANCE_AIR) {
            EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_FALL, 3);
        } else {
            switch (em->flinchPart) {
            case 0:
            case 7:
                EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_FLINCH, 3);
                break;
            case 5:
            case 6:
                EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_FLINCH_C, 3);
                break;
            case 1:
            case 2:
                EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_FLINCH_D, 3);
                break;
            case 3:
            case 4:
            default:
                if (em->parts[em->flinchPart].state >= 2) {
                    if (em->flinchPart != 3) {
                        EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_KNOCK_DOWN, 3);
                    } else {
                        EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_KNOCK_DOWN_PART3, 3);
                    }
                } else {
                    EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_FLINCH_B, 3);
                }
                break;
            }
        }
        break;
    case EM_DMG_HIT_MODE3:
        if (em->stance != EM_STANCE_AIR) {
            EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_FLINCH, 3);
            em->scriptPending = 0;
        }
        break;
    case EM_DMG_STATUS5:
        if ((s8) em->holdTimer != 0) {
            Em_SetStatus5Timer(em);
            EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_STUN_DOWNED, 3);
        } else if ((em->stateGroup != EM_GROUP_REACT || em->state != RATH_REACT_STUN) && (em->stateGroup != EM_GROUP_REACT || em->state != RATH_REACT_FALL)) {
            Em_SetStatus5Timer(em);
            EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_STUN, 3);
        }
        break;
    case EM_DMG_ATTACK13:
        Em_ResetKindTimer(em);
        EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_HOLD2, 3);
        break;
    }
    switch (gQuest.stage) {
    case QUEST_ID_RATHALOS_SCENE_1:
    case QUEST_ID_RATHALOS_SCENE_2:
        if ((em->type == EM_KIND_RATHALOS || em->type == EM_KIND_SILVER_RATHALOS || em->type == EM_KIND_AZURE_RATHALOS) && Area_DayId(em->area) == AREA_FOREST_HILLS_3 && Save_TestEventFlag(EVENT_FLAG_RATHALOS_SCENE) == 0) {
            if (Area_DayId(gScene.area) == AREA_FOREST_HILLS_3 && gScene.eventRunning == 1 && em->stateGroup != EM_GROUP_EVENT) {
                EmRathalos_ChangeState(em, EM_GROUP_EVENT, RATH_EVENT_INTRO, 1);
            }
            break;
        }
        goto normal;
    case QUEST_ID_RATHIAN_SCENE_1:
    case QUEST_ID_RATHIAN_SCENE_2:
        if ((em->type == EM_KIND_RATHIAN || em->type == EM_KIND_GOLD_RATHIAN || em->type == EM_KIND_PINK_RATHIAN) && Area_DayId(em->area) == AREA_JUNGLE_3 && Save_TestEventFlag(EVENT_FLAG_RATHIAN_SCENE) == 0) {
            if (Area_DayId(gScene.area) == AREA_JUNGLE_3 && gScene.eventRunning == 1 && em->stateGroup != EM_GROUP_EVENT) {
                EmHermitaur_ChangeState(em, 6, 6, 1);
            }
            break;
        }
    default:
    normal:
        Em_UpdatePendingScript(em);
        break;
    }
    Em_ClearHitReactMode(em);
    EmRathalos_RunState(em, (s32) w);
    if (em->stateRequest != 0) {
        EmRathalos_RunState(em, (s32) w);
        em->stateRequest = 0;
    }
    if (em->stateGroup == EM_GROUP_FLY && (s8) w->flyGuard == 1) {
        em->guardTimer = 2;
    }
}

/*
 * Runs the current state through its group's dispatcher, keeping the group
 * and state as the previous ones.
 */
void EmRathalos_RunState(Em *em, s32 work)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->stateGroup) {
    case EM_GROUP_IDLE:
        EmRathalos_RunIdleState(em, (EmWork *) work);
        break;
    case EM_GROUP_MOVE:
        EmRathalos_RunMoveState(em, (EmWork *) work);
        break;
    case EM_GROUP_FLY:
        EmRathalos_RunFlyState(em, (EmWork *) work);
        break;
    case EM_GROUP_ATTACK:
        EmRathalos_RunAttackState(em, (EmWork *) work);
        break;
    case EM_GROUP_REACT:
        EmRathalos_RunReactState(em, (EmWork *) work);
        break;
    case EM_GROUP_DEFEAT:
        EmRathalos_RunDefeatState(em, (EmWork *) work);
        break;
    case EM_GROUP_EVENT:
        EmRathalos_RunEventState(em, (EmWork *) work);
        break;
    case EM_GROUP_EVENT2:
        EmRathalos_RunEventState(em, (EmWork *) work);
        break;
    case EM_GROUP_VERSUS:
        EmRathalos_RunVersusState(em, (EmWork *) work);
        
    default:
        break;
    }
}

/* Requests camera shake `level` when the motion passes frame `frame`. */
void EmRathalos_CamShakeOnFrame(void *em, s32 frame, s32 i)
{
    if (EmMotion_PassedFrameScaled((Em *) em, (f32) frame, 0U) != 0) {
        CamShake_ReqAIf(em, i);
    }
}

static void EmRathalos_MotionEffectsDefault(Em *em)
{
}

/*
 * The effects of each motion: plays its sounds (EmRathalos_MotionSounds)
 * and on given frames creates attached effects, ground effects (stage
 * object kind 0x14; more of them while enraged) and camera shakes, at the
 * frames of the motion on layer 0 (work->sndMotion keeps its id).
 */
void EmRathalos_MotionEffects(Em *em, EmFlyerWork *work)
{
    f32 boneOffset[3];

    if (em->motionId[0] != work->sndMotion) {
        work->sndMotion = em->motionId[0];
    }
    EmRathalos_MotionSounds(em, (EmWork *) work);
    switch (work->sndMotion) {
    case EM_MOTION_ID(0, RATH_MOT_WALK):
        EmRathalos_CamShakeOnFrame(em, 0x34, 1);
        EmRathalos_CamShakeOnFrame(em, 0x74, 1);
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            RathalosAttack_Create(em, 2);
        }
        if (EmMotion_PassedFrameScaled(em, 50.0f, 0) != 0) {
            RathalosAttack_Create(em, 3);
        }
        if (EmMotion_PassedFrameScaled(em, 120.0f, 0) != 0) {
            RathalosAttack_Create(em, 4);
        }
        break;
    case EM_MOTION_ID(0, 0x4):
        if (EmMotion_PassedFrameScaled(em, 12.0f, 0) != 0) {
            DustCloud_Create(em, 2, 0, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 52.0f, 0) != 0) {
            DustCloud_Create(em, 3, 0, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 92.0f, 0) != 0) {
            DustCloud_Create(em, 2, 0, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, RATH_MOT_TURN_RIGHT):
        EmRathalos_CamShakeOnFrame(em, 0x3C, 1);
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            RathalosAttack_Create(em, 5);
        }
        if (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) {
            RathalosAttack_Create(em, 6);
        }
        if ((em->rage != 0) && (EmMotion_ReachedFrame(em, 0, 2.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 78.0f) == 0) && !(gScene.frame & 3)) {
            DustCloud_Create(em, 0x1A, 1, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, RATH_MOT_TURN_LEFT):
        EmRathalos_CamShakeOnFrame(em, 0x3C, 1);
        if (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) {
            RathalosAttack_Create(em, 7);
        }
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            RathalosAttack_Create(em, 8);
        }
        if ((em->rage != 0) && (EmMotion_ReachedFrame(em, 0, 2.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 78.0f) == 0) && !(gScene.frame & 3)) {
            DustCloud_Create(em, 0x1A, 0, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, 0x7):
        if (EmMotion_PassedFrameScaled(em, 18.0f, 0) != 0) {
            RathalosAttack_Create(em, 9);
        }
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            RathalosAttack_Create(em, 0xA);
        }
        if (em->rage != 0) {
            if ((EmMotion_ReachedFrame(em, 0, 2.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 18.0f) == 0) && !(gScene.frame & 3)) {
                DustCloud_Create(em, 0x1A, 1, 1.0f);
            }
            if ((EmMotion_ReachedFrame(em, 0, 20.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 70.0f) == 0) && !(gScene.frame & 3)) {
                DustCloud_Create(em, 0x1A, 0, 1.0f);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, 0x8):
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            RathalosAttack_Create(em, 0xB);
        }
        if (EmMotion_PassedFrameScaled(em, 18.0f, 0) != 0) {
            RathalosAttack_Create(em, 0xC);
        }
        if (em->rage != 0) {
            if ((EmMotion_ReachedFrame(em, 0, 2.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 18.0f) == 0) && !(gScene.frame & 3)) {
                DustCloud_Create(em, 0x1A, 0, 1.0f);
            }
            if ((EmMotion_ReachedFrame(em, 0, 20.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 70.0f) == 0) && !(gScene.frame & 3)) {
                DustCloud_Create(em, 0x1A, 1, 1.0f);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, RATH_MOT_LIMP):
        if (EmMotion_PassedFrameScaled(em, 42.0f, 0) != 0) {
            RathalosAttack_Create(em, 0x19);
        }
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            RathalosAttack_Create(em, 0x1A);
        }
        if (EmMotion_PassedFrameScaled(em, 74.0f, 0) != 0) {
            RathalosAttack_Create(em, 0x1A);
        }
        break;
    case EM_MOTION_ID(0, RATH_MOT_DESCEND):
        if (EmMotion_PassedFrameScaled(em, 52.0f, 0) || EmMotion_PassedFrameScaled(em, 104.0f, 0) || EmMotion_PassedFrameScaled(em, 162.0f, 0)) {
            if (em->posY <= (1000.0f + em->floorY)) {
                PartFx_SpawnForEmScaled(em, 2, 7, 5.0f);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, 0xD):
        if (((EmMotion_PassedFrameScaled(em, 26.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 52.0f, 0) != 0)) && (em->posY <= (1000.0f + em->floorY))) {
            PartFx_SpawnForEmScaled(em, 1, 7, 5.0f);
        }
        break;
    case EM_MOTION_ID(0, RATH_MOT_FLY):
    case EM_MOTION_ID(0, 0x55):
    case EM_MOTION_ID(0, 0x56):
    case EM_MOTION_ID(0, 0x58):
    case EM_MOTION_ID(0, 0x59):
        if ((em->rage != 0) && (((s32) gScene.frame % 19) == 0)) {
            DustCloud_Create(em, 0x19, ((u16) Rand_Next(1) & 1) + 2, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, RATH_MOT_HOVER):
        if (EmMotion_PassedFrameScaled(em, 8.0f, 0) || EmMotion_PassedFrameScaled(em, 86.0f, 0) || EmMotion_PassedFrameScaled(em, 160.0f, 0)) {
            if (em->posY <= (1000.0f + em->floorY)) {
                DustCloud_Create(em, 1, 0, 1.0f);
            }
        }
        if (em->rage != 0) {
            if (((EmMotion_ReachedFrame(em, 0, 70.0f) != 0) && (EmMotion_PassedFrameScaled(em, 84.0f, 0) == 0)) || ((EmMotion_ReachedFrame(em, 0, 146.0f) != 0) && (EmMotion_PassedFrameScaled(em, 156.0f, 0) == 0)) || ((EmMotion_ReachedFrame(em, 0, 220.0f) != 0) && (EmMotion_PassedFrameScaled(em, 234.0f, 0) == 0))) {
                if (!(gScene.frame & 3)) {
                    DustCloud_Create(em, 0x1A, (u16) Rand_Next(1) & 1, 1.0f);
                    break;
                }
            }
        }
        break;
    case EM_MOTION_ID(0, RATH_MOT_RUN):
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            RathalosAttack_Create(em, 0x18);
        }
        if ((em->rage != 0) && (((s32) gScene.frame % 10) == 0)) {
            DustCloud_Create(em, 0x19, ((u16) Rand_Next(1) & 1) + 2, 1.0f);
        }
        if ((EmMotion_PassedFrameScaled(em, 12.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 78.0f, 0) != 0)) {
            DustCloud_Create(em, 2, 0, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 42.0f, 0) != 0) {
            DustCloud_Create(em, 3, 0, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, RATH_MOT_TAKE_OFF):
        if (EmMotion_PassedFrameScaled(em, 42.0f, 0) || EmMotion_PassedFrameScaled(em, 102.0f, 0) || EmMotion_PassedFrameScaled(em, 156.0f, 0)) {
            if (em->posY <= (1000.0f + em->floorY)) {
                DustCloud_Create(em, 1, 0, 1.0f);
            }
        }
        if ((em->rage != 0) && (EmMotion_PassedFrameScaled(em, 62.0f, 0) != 0)) {
            DustCloud_Create(em, 0x19, 4, 1.0f);
            DustCloud_Create(em, 0x19, 5, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, RATH_MOT_TOUCH_DOWN):
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            RathalosAttack_Create(em, 0xD);
        }
        if (EmMotion_PassedFrameScaled(em, 16.0f, 0) != 0) {
            EmRathalos_SpawnLandingDust(em);
        }
        if ((em->rage != 0) && (EmMotion_ReachedFrame(em, 0, 18.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 44.0f) == 0) && !(gScene.frame & 3)) {
            DustCloud_Create(em, 0x19, ((u16) Rand_Next(1) & 1) + 2, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, 0x1F):
    case EM_MOTION_ID(0, RATH_MOT_LIE):
        boneOffset[1] = 10.0f;
        boneOffset[2] = 140.0f;
        boneOffset[0] = 0.0f;
        Em_SpawnLyingPuffs(em, 0x22, boneOffset, 1.6f);
        break;
    case EM_MOTION_ID(0, 0x24):
        if (EmMotion_PassedFrameScaled(em, 50.0f, 0) != 0) {
            RathalosAttack_Create(em, 1);
        }
        if (EmMotion_PassedFrameScaled(em, 46.0f, 0) != 0) {
            RathalosAttack_Create(em, 0x29);
        }
        break;
    case EM_MOTION_ID(0, RATH_MOT_COLLAPSE):
        if (EmMotion_PassedFrameScaled(em, 148.0f, 0) != 0) {
            RathalosAttack_Create(em, 0xF);
        }
        if (EmMotion_PassedFrameScaled(em, 156.0f, 0) != 0) {
            DustCloud_Create(em, 0, 0, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, RATH_MOT_LEAP):
        if (EmMotion_PassedFrameScaled(em, 60.0f, 0) != 0) {
            RathalosAttack_Create(em, 0x22);
        }
        if ((em->rage != 0) && (EmMotion_ReachedFrame(em, 0, 142.0f) != 0) && (EmMotion_PassedFrameScaled(em, 236.0f, 0) == 0) && !(gScene.frame & 3)) {
            DustCloud_Create(em, 0x1A, (u16) Rand_Next(1) & 1, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, RATH_MOT_LEAP_LAND):
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            RathalosAttack_Create(em, 0x23);
        }
        if ((EmMotion_ReachedFrame(em, 0, 20.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 50.0f) == 0) && !(gScene.frame & 3)) {
            DustCloud_Create(em, 2, 0, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 102.0f, 0) != 0) {
            DustCloud_Create(em, 2, 0, 1.0f);
        }
        if ((EmMotion_ReachedFrame(em, 0, 22.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 38.0f) == 0) && !(gScene.frame & 3)) {
            DustCloud_Create(em, 3, 0, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 62.0f, 0) != 0) {
            DustCloud_Create(em, 3, 0, 1.0f);
        }
        if ((em->rage != 0) && (EmMotion_ReachedFrame(em, 0, 46.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 134.0f) == 0) && !(gScene.frame & 3)) {
            DustCloud_Create(em, 0x1A, (u16) Rand_Next(1) & 1, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, RATH_MOT_TAIL_SPIN_B):
        EmRathalos_CamShakeOnFrame(em, 0x2C, 1);
        EmRathalos_CamShakeOnFrame(em, 0x49, 1);
        if (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) {
            RathalosAttack_Create(em, 0x11);
        }
        if ((em->rage != 0) && (EmMotion_ReachedFrame(em, 0, 6.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 38.0f) == 0) && !(gScene.frame & 3)) {
            DustCloud_Create(em, 0x1A, 0, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, RATH_MOT_TAIL_SPIN):
        EmRathalos_CamShakeOnFrame(em, 0x2C, 1);
        EmRathalos_CamShakeOnFrame(em, 0x49, 1);
        if (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) {
            RathalosAttack_Create(em, 0x11);
        }
        if ((em->rage != 0) && (EmMotion_ReachedFrame(em, 0, 6.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 38.0f) == 0) && !(gScene.frame & 3)) {
            DustCloud_Create(em, 0x1A, 1, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, RATH_MOT_SWOOP_CLAW):
        if (EmMotion_PassedFrameScaled(em, 22.0f, 0) != 0) {
            RathalosAttack_Create(em, 0x14);
            RathalosAttack_Create(em, 0x15);
        }
        if (EmMotion_PassedFrameScaled(em, 20.0f, 0) || EmMotion_PassedFrameScaled(em, 68.0f, 0) || EmMotion_PassedFrameScaled(em, 114.0f, 0)) {
            if (em->posY <= (1000.0f + em->floorY)) {
                DustCloud_Create(em, 9, 0, 1.0f);
                DustCloud_Create(em, 0xE, 0, 1.0f);
            }
        }
        if ((em->rage != 0) && (EmMotion_ReachedFrame(em, 0, 18.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 110.0f) == 0) && !(gScene.frame & 3)) {
            DustCloud_Create(em, 0x1A, (u16) Rand_Next(1) & 1, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, RATH_MOT_AIR_FIREBALL_RISE):
        if ((EmMotion_PassedFrameScaled(em, 92.0f, 0) != 0) && (em->posY <= (1000.0f + em->floorY))) {
            DustCloud_Create(em, 1, 0, 1.0f);
        }
        if ((em->rage != 0) && (EmMotion_ReachedFrame(em, 0, 60.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 132.0f) == 0) && !(gScene.frame & 3)) {
            DustCloud_Create(em, 0x1A, (u16) Rand_Next(1) & 1, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, RATH_MOT_ALERT_IDLE):
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            RathalosAttack_Create(em, 0x1B);
        }
        if (EmMotion_PassedFrameScaled(em, 168.0f, 0) != 0) {
            RathalosAttack_Create(em, 0x1B);
        }
        if (EmMotion_PassedFrameScaled(em, 56.0f, 0) != 0) {
            RathalosAttack_Create(em, 0x1C);
        }
        if (EmMotion_PassedFrameScaled(em, 54.0f, 0) != 0) {
            DustCloud_Create(em, 2, 2, 0.8f);
        }
        if (em->rage != 0) {
            if (EmMotion_InFrameRange(em, 0, 154.0f, 244.0f) || EmMotion_InFrameRange(em, 0, 254.0f, 330.0f)) {
                if (!(gScene.frame & 3)) {
                    DustCloud_Create(em, 0x1A, (u16) Rand_Next(1) & 1, 1.0f);
                    break;
                }
            }
        }
        break;
    case EM_MOTION_ID(0, RATH_MOT_BACK_HOP):
        if (EmMotion_PassedFrameScaled(em, 72.0f, 0) != 0) {
            DustCloud_Create(em, 4, 0, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 80.0f, 0) != 0) {
            DustCloud_Create(em, 1, 0, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 84.0f, 0) != 0) {
            DustCloud_Create(em, 4, 1, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, RATH_MOT_AIR_DIVE):
        if (EmMotion_PassedFrameScaled(em, 90.0f, 0) != 0) {
            RathalosAttack_Create(em, 0x16);
        }
        if (EmMotion_PassedFrameScaled(em, 108.0f, 0) != 0) {
            RathalosAttack_Create(em, 0x17);
        }
        break;
    case EM_MOTION_ID(0, RATH_MOT_TURN_IN_PLACE):
        if (EmMotion_PassedFrameScaled(em, 102.0f, 0) != 0) {
            RathalosAttack_Create(em, 0x12);
        }
        if (EmMotion_PassedFrameScaled(em, 110.0f, 0) != 0) {
            RathalosAttack_Create(em, 0x13);
            ElementBurst_CreateOnEm(em, 5, 4, 1.0f);
        }
        if (em->rage != 0) {
            if (((EmMotion_ReachedFrame(em, 0, 20.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 36.0f) == 0)) || ((EmMotion_ReachedFrame(em, 0, 46.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 168.0f) == 0)) || ((EmMotion_ReachedFrame(em, 0, 172.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 192.0f) == 0)) || ((EmMotion_ReachedFrame(em, 0, 196.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 216.0f) == 0)) || ((EmMotion_ReachedFrame(em, 0, 230.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 238.0f) == 0))) {
                if (!(gScene.frame & 3)) {
                    DustCloud_Create(em, 0x1A, (u16) Rand_Next(1) & 1, 1.0f);
                    break;
                }
            }
        }
        break;
    case EM_MOTION_ID(0, RATH_MOT_BACK_HOP_LAND):
        if (EmMotion_PassedFrameScaled(em, 12.0f, 0) != 0) {
            DustCloud_Create(em, 4, 0, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) {
            DustCloud_Create(em, 5, 0, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 28.0f, 0) != 0) {
            RathalosAttack_Create(em, 0x1D);
        }
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            RathalosAttack_Create(em, 0x25);
        }
        if (EmMotion_PassedFrameScaled(em, 36.0f, 0) != 0) {
            RathalosAttack_Create(em, 0x28);
        }
        break;
    case EM_MOTION_ID(0, 0x3C):
        if (EmMotion_PassedFrameScaled(em, 80.0f, 0) != 0) {
            DustCloud_Create(em, 1, 0x80, 0.5f);
        }
        if (EmMotion_PassedFrameScaled(em, 44.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0x1B, 7, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 62.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0x16, 7, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, 0x3D):
        if (EmMotion_PassedFrameScaled(em, 24.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0x1A, 7, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, 0x41):
        if (EmMotion_PassedFrameScaled(em, 20.0f, 0) != 0) {
            DustCloud_Create(em, 3, 0x80, 1.0f);
        }
        if ((EmMotion_PassedFrameScaled(em, 128.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 164.0f, 0) != 0)) {
            DustCloud_Create(em, 3, 0x80, 0.6f);
        }
        if (EmMotion_PassedFrameScaled(em, 60.0f, 0) != 0) {
            DustCloud_Create(em, 1, 0x80, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, RATH_MOT_DIE):
        if (EmMotion_PassedFrameScaled(em, 268.0f, 0) != 0) {
            DustCloud_Create(em, 0, 0, 0.9f);
        }
        break;
    case EM_MOTION_ID(0, RATH_MOT_KNOCK_DOWN):
        EmRathalos_CamShakeOnFrame(em, 0x1A, 2);
        if (EmMotion_PassedFrameScaled(em, 26.0f, 0) != 0) {
            DustCloud_Create(em, 0x12, 0x80, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, 0x49):
        EmRathalos_CamShakeOnFrame(em, 0x1A, 2);
        if ((EmMotion_ReachedFrame(em, 0, 28.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 52.0f) == 0) && !(gScene.frame & 3)) {
            DustCloud_Create(em, 0xA, 0x80, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, 0x4A):
        EmRathalos_CamShakeOnFrame(em, 0x1A, 2);
        if (EmMotion_PassedFrameScaled(em, 26.0f, 0) != 0) {
            DustCloud_Create(em, 0x12, 0x81, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, RATH_MOT_FALL_LAND):
        if (EmMotion_PassedFrameScaled(em, 8.0f, 0) != 0) {
            DustCloud_Create(em, 0xB, 0, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, RATH_MOT_HEAD_DOWN):
        if (EmMotion_PassedFrameScaled(em, 48.0f, 0) || EmMotion_PassedFrameScaled(em, 106.0f, 0) || EmMotion_PassedFrameScaled(em, 174.0f, 0)) {
            RathalosAttack_Create(em, 0x10);
        }
        break;
    case EM_MOTION_ID(0, 0x54):
        if (EmMotion_PassedFrameScaled(em, 38.0f, 0) || EmMotion_PassedFrameScaled(em, 60.0f, 0) || EmMotion_PassedFrameScaled(em, 78.0f, 0)) {
            PartFx_SpawnForEmScaled(em, 2, 7, 5.0f);
        }
        break;
    case EM_MOTION_ID(0, RATH_MOT_LEVEL_OUT):
        if ((EmMotion_PassedFrameScaled(em, 50.0f, 0) != 0) && (em->posY <= (1000.0f + em->floorY))) {
            DustCloud_Create(em, 9, 0, 1.0f);
            DustCloud_Create(em, 0xE, 0, 1.0f);
        }
        if ((EmMotion_PassedFrameScaled(em, 102.0f, 0) != 0) && (em->posY <= (1000.0f + em->floorY))) {
            DustCloud_Create(em, 9, 0, 1.0f);
            DustCloud_Create(em, 0xE, 0, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, RATH_MOT_CHARGE_TRIP):
        if (EmMotion_PassedFrameScaled(em, 34.0f, 0) != 0) {
            DustCloud_Create(em, 3, 0, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 70.0f, 0) != 0) {
            DustCloud_Create(em, 2, 0, 1.0f);
        }
        if ((EmMotion_ReachedFrame(em, 0, 96.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 120.0f) == 0) && (((s32) gScene.frame % 6) == 0)) {
            DustCloud_Create(em, 0x12, 0, 1.0f);
        }
        if ((em->rage != 0) && (((s32) gScene.frame % 19) == 0)) {
            DustCloud_Create(em, 0x19, ((u16) Rand_Next(1) & 1) + 2, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, RATH_MOT_FLIP):
        if (EmMotion_PassedFrameScaled(em, 108.0f, 0) != 0) {
            RathalosAttack_Create(em, 0x1E);
        }
        if (EmMotion_PassedFrameScaled(em, 104.0f, 0) != 0) {
            DustCloud_Create(em, 1, 0, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 200.0f, 0) != 0) {
            DustCloud_Create(em, 1, 0, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, RATH_MOT_FLIP_LAND):
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            RathalosAttack_Create(em, 0x1F);
        }
        break;
    case EM_MOTION_ID(0, RATH_MOT_TRAPPED):
        if (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0) {
            DustCloud_Create(em, 0xB, 0x80, 1.0f);
        }
        if ((em->rage != 0) && (EmMotion_ReachedFrame(em, 0, 4.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 60.0f) == 0) && !(gScene.frame & 3)) {
            DustCloud_Create(em, 0x1A, (u16) Rand_Next(1) & 1, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, RATH_MOT_STRUGGLE):
        if (em->rage != 0) {
            if (((EmMotion_ReachedFrame(em, 0, 12.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 24.0f) == 0)) || (EmMotion_ReachedFrame(em, 0, 46.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 90.0f) == 0)) {
                if (!(gScene.frame & 3)) {
                    DustCloud_Create(em, 0x1A, (u16) Rand_Next(1) & 1, 1.0f);
                    break;
                }
            }
        }
        break;
    default:
        EmRathalos_MotionEffectsDefault(em);
        break;
    case EM_MOTION_ID(0, RATH_MOT_IDLE):
    case EM_MOTION_ID(0, RATH_MOT_RUN_STOP):
    case EM_MOTION_ID(0, RATH_MOT_GLIDE):
    case EM_MOTION_ID(0, 0x10):
    case EM_MOTION_ID(0, 0x1D):
    case EM_MOTION_ID(0, 0x1E):
    case EM_MOTION_ID(0, RATH_MOT_ROAR):
    case EM_MOTION_ID(0, RATH_MOT_GET_UP):
    case EM_MOTION_ID(0, RATH_MOT_LIE_DOWN):
    case EM_MOTION_ID(0, RATH_MOT_SWOOP_DIVE):
    case EM_MOTION_ID(0, RATH_MOT_AIR_FIREBALL):
    case EM_MOTION_ID(0, RATH_MOT_FIREBALL):
    case EM_MOTION_ID(0, 0x30):
    case EM_MOTION_ID(0, RATH_MOT_FIREBALL_B):
    case EM_MOTION_ID(0, RATH_MOT_FEED):
    case EM_MOTION_ID(0, RATH_MOT_SHAKE_OFF):
    case EM_MOTION_ID(0, 0x3F):
    case EM_MOTION_ID(0, 0x40):
    case EM_MOTION_ID(0, 0x42):
    case EM_MOTION_ID(0, 0x43):
    case EM_MOTION_ID(0, RATH_MOT_DOWN):
    case EM_MOTION_ID(0, RATH_MOT_DOWN_GET_UP):
    case EM_MOTION_ID(0, 0x48):
    case EM_MOTION_ID(0, 0x4B):
    case EM_MOTION_ID(0, 0x4C):
    case EM_MOTION_ID(0, RATH_MOT_FALL):
    case EM_MOTION_ID(0, 0x50):
    case EM_MOTION_ID(0, 0x51):
    case EM_MOTION_ID(0, RATH_MOT_DIE_FALLING):
    case EM_MOTION_ID(0, 0x53):
    case EM_MOTION_ID(0, RATH_MOT_SWOOP):
    case EM_MOTION_ID(0, RATH_MOT_FLIP_FALL):
    case EM_MOTION_ID(0, RATH_MOT_DOWNED):
    case EM_MOTION_ID(0, RATH_MOT_DOWNED_GET_UP):
        break;
    }
}

/*
 * Runs EmRathalos_MotionEffects once the work area's step is set (the first
 * frame only sets it).
 */
void EmRathalos_UpdateMotionEffects(Em *em)
{
    EmFlyerWork *w = (EmFlyerWork *)&em->work;

    switch (w->fxStep) {
    case 0:
        w->fxStep++;
        break;
    case 1:
        EmRathalos_MotionEffects(em, w);
        break;
    }
}

/* Creates the ground effect of kind 0x14 type 0xB (made at the landings). */
void EmRathalos_SpawnLandingDust(Em *em)
{
    DustCloud_Create((Em *) em, 0xB, 0, 1.0f);
}

/* Creates the ground effect of kind 0x14 type 0xA every fifth frame. */
void EmRathalos_SpawnK14Type0AEvery5(Em *em)
{
    if (((s32) gScene.frame % 5) == 0) {
        DustCloud_Create((Em *) em, 0xA, 0, 1.0f);
    }
}

/*
 * During a swoop: 1 once the claw pass runs; starts it
 * (RATH_MOT_SWOOP_CLAW) when the target is within 22 frames' flight. 0
 * without a target.
 */
s32 EmRathalos_ReachedTarget(Em *em)
{
    f32 targetPos[3];
    f32 dist;
    s8 targetNo;

    targetNo = em->targetNo;
    if (targetNo == -1) {
        return 0;
    }
    Em_GetTargetPos(em, targetNo & 0xFF, targetPos);
    dist = Vec_Distance((f32 *) &em->posX, (f32 *) targetPos);
    if (em->motionId[0] == EM_MOTION_ID(0, RATH_MOT_SWOOP_CLAW)) {
        return 1;
    }
    if (dist <= (4.5f + (30.0f * em->velZ))) {
        Em_SetMotion(em, RATH_MOT_SWOOP_CLAW, 0xA, 0);
        return 1;
    }
    return 0;
}

/* As EmRathalos_ReachedTarget, measured to the goal (the intro's swoop). */
s32 EmRathalos_ReachedPoint(Em *em)
{
    f32 dist;

    dist = Vec_Distance((f32 *) &em->posX, (f32 *) em->goal);
    if (em->motionId[0] == EM_MOTION_ID(0, RATH_MOT_SWOOP_CLAW)) {
        return 1;
    }
    if (dist <= (2.4199998f + (22.0f * em->velZ))) {
        Em_SetMotion(em, RATH_MOT_SWOOP_CLAW, 0xA, 0);
        return 1;
    }
    return 0;
}

/* Ends a swoop: step 99, then a glide once (EmRathalos_EndAirState). */
void EmRathalos_EndSwoop(Em *em, s32 unused)
{
    em->step = 99;
    EmRathalos_EndAirState(em, 1);
}

void EmRathalos_TableNop(void)
{
}

/*
 * Whether the once-only reaction (RATH_REACT_PART_BREAK) may run: not in
 * the trapped, downed or downed-end states, not in the air, and not after
 * it ran once.
 */
s32 EmRathalos_AllowPartBreak(Em *em)
{
    u8 stateGroup;

    stateGroup = em->stateGroup;
    if ((stateGroup != EM_GROUP_REACT || em->state != RATH_REACT_TRAPPED)
        && (stateGroup != EM_GROUP_REACT || em->state != RATH_REACT_TRAPPED_STRUGGLE)
        && (stateGroup != EM_GROUP_REACT || em->state != RATH_REACT_STATUS4_DOWNED)
        && (stateGroup != EM_GROUP_REACT || em->state != RATH_REACT_STATUS4_DOWNED_END)
        && (stateGroup != EM_GROUP_REACT || em->state != RATH_REACT_STATUS2_DOWNED_END)
        && (stateGroup != EM_GROUP_REACT || em->state != RATH_REACT_STATUS1_DOWNED_END)
        && em->stance != EM_STANCE_AIR && (s8) em->partBreakCount == 0) {
        return 1;
    }
    return 0;
}

/*
 * Starts a group-0 state: the alert idle instead of the idle in combat, and
 * the statuses and needs each state begins or ends.
 */
void EmRathalos_SetIdleState(Em *em, u16 state, u8 mode)
{
    em->motionSpeed = 1.0f;
    switch (state) {
    case RATH_IDLE:
        if (em->combat == 1) {
            state = RATH_IDLE_ALERT;
        }
        break;
    case RATH_IDLE_REST:
        Em_ResetNotice(em);
        break;
    case RATH_IDLE_STATUS2_COLLAPSE:
        Em_BeginStatus2(em);
        break;
    case RATH_IDLE_STATUS2_LIE_DOWN:
        Em_BeginStatus2(em);
        break;
    case RATH_IDLE_REST_END:
        Em_RefillNeed3(em);
        break;
    case RATH_IDLE_STATUS2_END:
        Em_EndStatus2(em);
        break;
    case RATH_IDLE_STATUS1_COLLAPSE:
        Em_BeginStatus1(em);
        break;
    case RATH_IDLE_STATUS1_END:
        Em_EndStatus1(em);
        break;
    case RATH_IDLE_STATUS2_DOWNED:
        Em_BeginStatus2(em);
        break;
    case RATH_IDLE_STATUS1_DOWNED:
        Em_BeginStatus1(em);
        break;
    case RATH_IDLE_STATUS4_SHAKE_OFF:
        Em_EndStatus4(em);
        break;
    case RATH_IDLE_STUN_SHAKE_OFF:
        Em_EndStatus5(em);
        break;
    case RATH_IDLE_HOLD2_SHAKE_OFF:
        em->holdTimer2 = 0;
        break;
    case RATH_IDLE_EAT:
        Em_StartGoalAction(em, 0x2710);
        break;
    }
    em->stance = EM_STANCE_GROUND;
    Em_SetStateSync(em, EM_GROUP_IDLE, state, mode);
}

/*
 * Starts a group-1 state: the limp below the low-hp ratio, the run in
 * combat, the speed and the walk's goal and timer.
 */
void EmRathalos_SetMoveState(Em *em, u16 state, u8 mode)
{
    f32 lowHpRatio;
    f32 dist;

    em->stance = EM_STANCE_GROUND;
    em->motionSpeed = 1.0f;
    switch (state) {
    case RATH_MOVE_WALK:
    case RATH_MOVE_END2:
        if (em->type == EM_KIND_RATHIAN || em->type == EM_KIND_GOLD_RATHIAN || em->type == EM_KIND_PINK_RATHIAN) {
            lowHpRatio = 0.1f;
        } else {
            lowHpRatio = 0.3f;
        }
        /* low on hp it limps, in combat it runs: the gotos join those cases */
        if (em->hp < (s16) (em->hpMax * lowHpRatio)) {
            state = RATH_MOVE_LIMP;
            goto state6;
        }
        if (state == RATH_MOVE_END2 && em->combat == 1) {
            state = RATH_MOVE_RUN;
            goto state4;
        }
        if (state == RATH_MOVE_END2) {
            state = RATH_MOVE_WALK;
        }
        Em_SetSpeedScaled(em, 0, 1000.0f, 500.0f);
        break;
    case RATH_MOVE_END:
        em->scriptPending = 1;
        return;
    case RATH_MOVE_TURN:
        if (em->combat == 1) {
            state = RATH_MOVE_TURN_IN_COMBAT;
        }
        break;
    case RATH_MOVE_RUN_SLOW:
        em->motionSpeed = 0.7f;
    case RATH_MOVE_TURN_RAGE:
        Em_StartRage(em);
        break;
    case RATH_MOVE_TURN_PENDING_RAGE:
        Em_StartPendingRage(em);
        break;
    case RATH_MOVE_RUN:
    state4:
        Em_SetSpeedScaled(em, 2, 1000.0f, 500.0f);
        break;
    case RATH_MOVE_LIMP:
    state6:
        if (em->curGoalKind == 0) {
            em->hasGoal = 0;
            em->timer = (em->stateRand % 120 + 180) / 2;
        } else {
            em->hasGoal = 1;
            dist = Vec_DistanceXZ(&em->posX, em->goal);
            em->goalDist = dist;
            em->timer = (s32) dist / 7;
        }
        if (em->timer > 300) {
            em->timer = 300;
        }
        break;
    case RATH_MOVE_TURN_IN_COMBAT:
    case RATH_MOVE_TURN_TO_GOAL:
        break;
    }
    Em_SetStateSync(em, EM_GROUP_MOVE, state, mode);
}

/*
 * Starts a group-2 state: the goals and timers of the flights (routes,
 * exits, landing points, the area sequence's point).
 */
void EmRathalos_SetFlyState(Em *em, u16 state, u8 mode)
{
    EmFlyerWork *w = (EmFlyerWork *)&em->work;

    em->motionSpeed = 1.0f;
    switch (state) {
    case RATH_FLY_LAND:
        if (em->remote == 0 && !(em->velZ < 50.0f)) {
            state = RATH_FLY_LEVEL_OUT_AND_LAND;
        }
        break;
    case RATH_FLY_ROUTE:
    case RATH_FLY_ROUTE_B:
        w->legsLeft = 4;
        if (em->remote == 0) {
            em->goalNo = em->stateRand & 3;
            em->goalKind = 2;
            em->goalArg = 2;
            Em_ComputeGoal(em, em->goal);
            em->timer = 1.5f * Vec_DistanceXZ(&em->posX, em->goal) / 50.0f;
        }
        break;
    case RATH_FLY_HOVER_TO_GOAL:
    case RATH_FLY_HOVER_STRAIGHT_TO_GOAL:
        Player_ClearVelocity(em);
        em->velZ = 20.0f;
        em->timer = (s32) (Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) / em->velZ) * 3;
        break;
    case RATH_FLY_INTO_AREA:
        Em_TypeSetup(em);
    case RATH_FLY_OUT_OF_AREA:
        if (em->remote == 0 && em->curGoalKind != 3) {
            EmRathalos_ChangeState(em, EM_GROUP_FLY, RATH_FLY_LAND, 1);
            return;
        }
        break;
    case RATH_FLY_TO_GOAL_AND_LAND:
        if (em->remote == 0) {
            em->goalKind = 2;
            if (em->areaSeqActive != 0) {
                em->goalArg = 3;
                em->goalNo = 0;
            } else {
                em->goalArg = 0;
            }
            Em_ComputeGoal(em, em->goal);
        }
        em->timer = 500;
        Em_TypeSetup(em);
        break;
    case RATH_FLY_GLIDE_TO_GOAL:
        if (em->remote == 0) {
            if (em->areaSeqActive != 0) {
                em->goalKind = 2;
                em->goalArg = 3;
                em->goalNo = 0;
            }
            Em_ComputeGoal(em, em->goal);
        }
        em->timer = 500;
        break;
    case RATH_FLY_GLIDE_TO_GOAL_B:
        w->legsLeft = 1;
        if (em->remote == 0) {
            em->timer = 1.5f * Vec_DistanceXZ(&em->posX, em->goal) / 50.0f;
        }
        break;
    case RATH_FLY_HOVER_RISE:
        if (em->velZ <= 0.0f) {
            em->velZ = 20.0f;
        }
        if (em->remote == 0) {
            em->timer = Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) / em->velZ;
        }
        break;
    case RATH_FLY_TAKE_OFF:
    case RATH_FLY_HOVER:
    case RATH_FLY_HOP_TO_GOAL:
    case RATH_FLY_HOP_FORWARD:
    case RATH_FLY_HOP_BACK:
    case RATH_FLY_OUT_OF_AREA_ALT:
    case RATH_FLY_BACK_HOP:
    case RATH_FLY_HOVER_ONCE:
    case RATH_FLY_GLIDE_ONCE:
    case RATH_FLY_HOVER_TURN:
    case RATH_FLY_LEVEL_OUT_AND_LAND:
    case RATH_FLY_END:
    case RATH_FLY_LEVEL_OUT:
    case RATH_FLY_ESCAPE_UPWARD:
        break;
    }
    Em_SetStateSync(em, EM_GROUP_FLY, state, mode);
}

/*
 * Starts a group-3 state: the target and goal, the fireball's variant by
 * the target's height, the swoops' escape point, the repeat counts.
 */
void EmRathalos_SetAttackState(Em *em, u16 state, u8 mode)
{
    EmFlyerWork *w = (EmFlyerWork *)&em->work;
    f32 targetPos[3];
    f32 pos[3];
    f32 distOrHeight;

    em->motionSpeed = 1.0f;
    switch (state) {
    case RATH_ATK_TURN_AND_STARE:
    case RATH_ATK_TURN_AND_STARE_B:
    case RATH_ATK_LIE_AND_RISE:
        Em_SetTargetAngle(em);
        break;
    case RATH_ATK_TAIL_SPIN:
        if (em->remote == 0 && (Game_IsOnline() == 0 || mode != 0) && (em->stateRand & 1)) {
            state = RATH_ATK_TAIL_SPIN_B;
        }
        break;
    case RATH_ATK_LEAP:
        Em_SetTargetAngle(em);
        if (em->targetNo == -1) {
            em->goalDist = 2500.0f;
            break;
        }
        em->target = &gPlayers[em->targetNo];
        Em_GetTargetPos(em, em->targetNo, targetPos);
        Vec_Set(em->goal, targetPos[0], targetPos[1], targetPos[2]);
        distOrHeight = Vec_DistanceXZ(&em->posX, em->goal);
        em->goalDist = distOrHeight;
        if (distOrHeight > 5000.0f) {
            em->goalDist = 5000.0f;
        } else if (distOrHeight < 2500.0f) {
            em->goalDist = 2500.0f;
        }
        break;
    case RATH_ATK_FIREBALL:
        if (em->remote == 0 && (Game_IsOnline() == 0 || mode != 0) && em->curGoalKind == 1 && em->curGoalArg == 0) {
            if (em->targetNo == -1) {
                w->countdown = 1;
                Em_SetTargetAngle(em);
                break;
            }
            Em_GetTargetPos(em, em->targetNo, pos);
            distOrHeight = em->playerDistXZ[em->targetNo];
            if (distOrHeight <= 1000.0f) {
                distOrHeight = pos[1];
                if (distOrHeight <= em->posY + 50.0f) {
                    state = RATH_ATK_FIREBALL_LOW;
                } else if (distOrHeight <= em->posY + 150.0f) {
                    state = RATH_ATK_FIREBALL;
                } else {
                    state = RATH_ATK_FIREBALL_HIGH;
                }
            } else if (distOrHeight <= 2000.0f) {
                distOrHeight = pos[1];
                if (distOrHeight <= em->posY - 150.0f) {
                    state = RATH_ATK_FIREBALL_LOW;
                } else if (distOrHeight <= em->posY + 300.0f) {
                    state = RATH_ATK_FIREBALL;
                } else {
                    state = RATH_ATK_FIREBALL_HIGH;
                }
            } else {
                distOrHeight = pos[1];
                if (distOrHeight <= em->posY - 200.0f) {
                    state = RATH_ATK_FIREBALL_LOW;
                } else if (distOrHeight <= em->posY + 400.0f) {
                    state = RATH_ATK_FIREBALL;
                } else {
                    state = RATH_ATK_FIREBALL_HIGH;
                }
            }
        }
    case RATH_ATK_FIREBALL_B:
    case RATH_ATK_FIREBALL_LOW:
    case RATH_ATK_FIREBALL_HIGH:
        w->countdown = 1;
        Em_SetTargetAngle(em);
        break;
    case RATH_ATK_SWOOP_THEN_LEAVE:
    case RATH_ATK_SWOOP_THEN_LAND:
        if (em->remote == 0) {
            EmRathalos_SetEscapePoint(em);
            em->timer = 1.5f * Vec_DistanceXZ(&em->posX, em->goal) / 50.0f;
        }
        break;
    case RATH_ATK_AIR_FIREBALL_B:
    case RATH_ATK_AIR_FIREBALL_DOUBLE:
    case RATH_ATK_AIR_FIREBALL_DOUBLE_B:
    case RATH_ATK_AIR_FIREBALL_DOUBLE_FORWARD:
        w->countdown = em->stateRand % 2 + 1;
    case RATH_ATK_AIR_FIREBALL:
    case RATH_ATK_AIR_FIREBALL_FORWARD:
        if (em->targetNo == -1) {
            em->timer = 200;
            EmRathalos_ChangeState(em, EM_GROUP_FLY, RATH_FLY_HOVER, 1);
            return;
        }
        break;
    case RATH_ATK_AIR_DIVE:
        if (em->remote == 0) {
            if (em->targetNo == -1) {
                em->timer = 200;
                EmRathalos_ChangeState(em, EM_GROUP_FLY, RATH_FLY_HOVER, 1);
                return;
            }
            em->target = &gPlayers[em->targetNo];
        } else {
            em->target = &gPlayers[em->targetNo];
        }
        break;
    case RATH_ATK_CHARGE_TRIP:
    case RATH_ATK_RUN_AND_STOP:
        if (em->targetNo == -1) {
            em->timer = 150;
            em->hasGoal = 0;
        } else {
            em->hasGoal = 1;
            distOrHeight = Vec_DistanceXZ(&em->posX, em->goal);
            em->goalDist = distOrHeight - 500.0f;
            em->timer = (s32) (distOrHeight / 30.0f) + 30;
        }
        if (em->timer < 30) {
            em->timer = 30;
        }
        break;
    case RATH_ATK_CHARGE:
        if (em->targetNo == -1) {
            em->timer = 150;
            em->hasGoal = 0;
        } else {
            em->hasGoal = 1;
            distOrHeight = Vec_DistanceXZ(&em->posX, em->goal) + 700.0f;
            em->goalDist = distOrHeight;
            em->timer = (s32) (distOrHeight / 30.0f) + 30;
        }
        break;
    case RATH_ATK_BACK_HOP_FIREBALL:
    case RATH_ATK_BACK_HOP_FIREBALL_LEFT:
    case RATH_ATK_BACK_HOP_FIREBALL_RIGHT:
        if (em->targetNo == -1) {
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE, 1);
            return;
        }
        em->target = &gPlayers[em->targetNo];
        break;
    case RATH_ATK_STARE:
    case RATH_ATK_ROAR:
    case RATH_ATK_ROAR_AT_POINT:
    case RATH_ATK_SINGLE_A:
    case RATH_ATK_SINGLE_B:
    case RATH_ATK_ROAR_B:
    case RATH_ATK_SOMERSAULT:
    case RATH_ATK_TAIL_SPIN_B:
    case RATH_ATK_SOMERSAULT_DOUBLE:
        break;
    }
    Em_SetStateSync(em, EM_GROUP_ATTACK, state, mode);
}

/*
 * Changes the state. On the ground, flight states that need the air become
 * RATH_MOVE_WALK; group 0 to 3 states go through their Set function, the
 * rest straight to Em_SetStateSync, with the statuses and part breaks group
 * 4's states begin or end. Remote copies skip modes 3 and 4.
 */
void EmRathalos_ChangeState(Em *em, u16 group, u16 state, u8 mode)
{
    if (em->remote == 0) {
        Em_TargetPlayerInRange(em);
        if (em->stance == EM_STANCE_GROUND) {
            switch (group) {
            case EM_GROUP_FLY:
                switch (state) {
                case RATH_FLY_LAND:
                case RATH_FLY_HOVER:
                case RATH_FLY_ROUTE:
                case RATH_FLY_HOVER_TO_GOAL:
                case RATH_FLY_OUT_OF_AREA:
                case RATH_FLY_TO_GOAL_AND_LAND:
                case RATH_FLY_OUT_OF_AREA_ALT:
                case RATH_FLY_GLIDE_TO_GOAL:
                case RATH_FLY_INTO_AREA:
                case RATH_FLY_HOVER_ONCE:
                case RATH_FLY_GLIDE_ONCE:
                case RATH_FLY_HOVER_TURN:
                case RATH_FLY_ROUTE_B:
                case RATH_FLY_LEVEL_OUT_AND_LAND:
                case RATH_FLY_GLIDE_TO_GOAL_B:
                case RATH_FLY_HOVER_STRAIGHT_TO_GOAL:
                case RATH_FLY_LEVEL_OUT:
                case RATH_FLY_HOVER_RISE:
                    state = RATH_MOVE_WALK;
                    group = EM_GROUP_MOVE;
                    break;
                case RATH_FLY_TAKE_OFF:
                case RATH_FLY_HOP_TO_GOAL:
                case RATH_FLY_HOP_FORWARD:
                case RATH_FLY_HOP_BACK:
                case RATH_FLY_BACK_HOP:
                case RATH_FLY_END:
                case RATH_FLY_ESCAPE_UPWARD:
                    break;
                }
                break;
            }
        }
    } else if (mode == 3 || mode == 4) {
        return;
    }
    Em_LoadReactionSet(em, 0);
    switch (group) {
    case EM_GROUP_IDLE:
        EmRathalos_SetIdleState(em, state, mode);
        break;
    case EM_GROUP_MOVE:
        EmRathalos_SetMoveState(em, state, mode);
        break;
    case EM_GROUP_FLY:
        EmRathalos_SetFlyState(em, state, mode);
        break;
    case EM_GROUP_ATTACK:
        EmRathalos_SetAttackState(em, state, mode);
        break;
    case EM_GROUP_VERSUS:
        if (state == RATH_VS_WAIT_BOTH) {
            Versus_MarkDefeat(em);
        }
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        break;
    case EM_GROUP_DEFEAT:
        if (state == RATH_DEFEAT_REAPPEAR && QuestEm_CanAppear(em) == 1) {
            QuestEm_Appear(em);
        }
    case EM_GROUP_EVENT:
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        break;
    case EM_GROUP_REACT:
        switch (state) {
        case RATH_REACT_PART_BREAK:
            Em_EndAllStatus(em);
            em->fxFlags |= 1;
            em->partBreakCount++;
            break;
        case RATH_REACT_STATUS4:
            Em_BeginStatus4(em);
            break;
        case RATH_REACT_TRAPPED:
            em->holdTimer = 50;
            Em_EndAllStatus(em);
            break;
        case RATH_REACT_STATUS4_DOWNED:
            Em_BeginStatus4(em);
            break;
        case RATH_REACT_PART_BREAK_END:
            Em_ResetToBasePose(em);
            Em_BreakPart(em, 3);
            break;
        case RATH_REACT_STATUS4_DOWNED_END:
            Em_EndStatus4(em);
            break;
        case RATH_REACT_STATUS2_DOWNED_END:
            Em_EndStatus2(em);
            break;
        case RATH_REACT_STATUS1_DOWNED_END:
            Em_EndStatus1(em);
            break;
        case RATH_REACT_STUN:
            Em_BeginStatus5(em);
            break;
        case RATH_REACT_STUN_DOWNED:
            Em_BeginStatus5(em);
            break;
        case RATH_REACT_STUN_DOWNED_END:
            Em_EndStatus5(em);
            break;
        case RATH_REACT_HOLD2:
            Em_EndAllStatus(em);
            break;
        }
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        break;
    case EM_GROUP_EVENT2:
        break;
    }
}

/*
 * Sets the goal to the escape point of a swoop: of the area's four points
 * (Em_AreaEntryValue's table 2), the one farthest from the target; (1000,
 * 0, 1000) without points. Without a target it flies the route instead.
 */
void EmRathalos_SetEscapePoint(Em *em)
{
    f32 pos[3];
    f32 dist[4];
    f32 (*pts)[3];
    u32 i;
    f32 (*p)[3];
    f32 best;
    f32 candidate;
    u8 k;
    f32 *sel;

    if (em->targetNo == -1) {
        EmRathalos_ChangeState(em, EM_GROUP_FLY, RATH_FLY_ROUTE, 1);
        return;
    }
    pts = (f32 (*)[3]) Em_AreaEntryValue(em, em->tables[2]);
    if (pts == NULL) {
        em->goal[0] = 1000.0f;
        em->goal[1] = 0.0f;
        em->goal[2] = 1000.0f;
        return;
    }
    p = pts;
    Em_GetTargetPos(em, em->targetNo, pos);
    for (i = 0; i < 4; i++) {
        dist[i] = Vec_DistanceXZ(pos, *p);
        p++;
    }
    best = dist[0];
    k = 0;
    candidate = dist[1];
    if (best < candidate) {
        best = candidate;
        k = 1;
    }
    candidate = dist[2];
    if (best < candidate) {
        best = candidate;
        k = 2;
    }
    candidate = dist[3];
    if (best < candidate) {
        k = 3;
    }
    sel = pts[k];
    em->goalKind = 2;
    em->goalArg = 2;
    em->goalNo = k;
    em->curGoalKind = em->goalKind;
    em->curGoalArg = *(u16 *) &em->goalArg;
    em->curGoalNo = *(u16 *) &em->goalNo;
    em->goal[0] = sel[0];
    em->goal[1] = sel[1];
    em->goal[2] = sel[2];
}

/*
 * Steps rotX by the work area's pitchStep toward the pitch of `mode`: 0 at
 * the target's position raised by `height`, 1 nose up (0xD000), 2 level.
 */
void EmRathalos_StepPitch(Em *em, s32 mode, f32 height)
{
    EmWork *work = (EmWork *) &em->work;
    f32 pos[3];
    s32 pitchDelta;

    if (em->targetNo == -1) {
        return;
    }
    switch (mode) {
    case 0:
        Em_GetTargetPos(em, em->targetNo, pos);
        pitchDelta = 0x10000 - Math_Heading(Vec_Distance((f32 *) &em->posX, (f32 *) pos), em->posY, 0.0f, pos[1] + height);
        pitchDelta = (pitchDelta - em->rotX) & 0xFFFF;
        break;
    case 1:
        pitchDelta = (0xD000 - em->rotX) & 0xFFFF;
        break;
    case 2:
        pitchDelta = -em->rotX & 0xFFFF;
        break;
    }
    if (pitchDelta == 0) {
        return;
    }
    if (pitchDelta < 0x8000) {
        if (em->rotX < 0x3000 || em->rotX >= 0xD000) {
            if (pitchDelta <= work->pitchStep) {
                em->rotX += pitchDelta;
            } else {
                em->rotX += work->pitchStep;
            }
        }
    } else {
        if (em->rotX > 0xD000 || em->rotX <= 0x3000) {
            if (pitchDelta >= 0x10000 - work->pitchStep) {
                em->rotX -= 0x10000 - pitchDelta;
            } else {
                em->rotX -= work->pitchStep;
            }
        }
    }
    em->rotX = (u16) em->rotX;
}

/*
 * Sets velY from vertical-speed curve `kind` (EmRathalos_MoveVelYCurves) at
 * the motion's frame, while the motion is not blending.
 */
void EmRathalos_MoveByVelYCurve(Em *em, s32 kind)
{
    EmWork *work = (EmWork *) &em->work;
    EmCurveKey *tbl;
    EmCurveKey *key;
    f32 frame;
    f32 keyFrame;
    s32 i;

    tbl = EmRathalos_MoveVelYCurves[kind];
    if (tbl != NULL && em->motion[0].blend == 0) {
        frame = em->motion[0].frame;
        if (frame == 0.0f || frame == 1.0f) {
            em->velY = tbl->v;
        } else {
            i = 1;
            while (i != 0) {
                key = &tbl[i];
                keyFrame = key->t;
                if (frame > keyFrame && keyFrame != 0.0f) {
                    i++;
                } else if (keyFrame == 0.0f) {
                    break;
                } else {
                    i = 0;
                    em->velY = (key->v - key[-1].v) / ((keyFrame - key[-1].t) / em->motion[0].step);
                }
            }
        }
        work->moveAngX = 0;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
    }
}

/*
 * Starts curve set `index` of the family's movement curves (vertical speed
 * and forward speed).
 */
void EmRathalos_StartCurves(Em *em, u8 kind)
{
    EmFlyerWork *w = (EmFlyerWork *)&em->work;

    w->curveNone = 0;
    w->curveVelY = 0;
    w->curveSpeed = 0;
    w->curveKind = kind;
    switch (w->curveKind) {
    case 2:
    case 15:
        w->curveVelY = 1;
        w->curveFrame = 0x26;
        break;
    case 3:
        w->curveVelY = 1;
        w->curveFrame = 0x1E;
        break;
    case 5:
        w->curveVelY = 1;
        w->curveSpeed = 1;
        w->curveFrame = 0x3C;
        break;
    case 8:
    case 14:
        w->curveVelY = 1;
        w->curveSpeed = 1;
        w->curveFrame = 0x4C;
        break;
    case 9:
        w->curveVelY = 1;
        w->curveSpeed = 1;
        w->curveFrame = 0;
        break;
    case 10:
        w->curveVelY = 1;
        w->curveSpeed = 1;
        w->curveFrame = 0x5C;
        break;
    case 11:
        w->curveVelY = 1;
        w->curveFrame = 0;
        break;
    case 12:
        w->curveVelY = 1;
        w->curveFrame = 0x2C;
        break;
    case 13:
        w->curveVelY = 1;
        w->curveFrame = 0;
        break;
    case 16:
        w->curveVelY = 1;
        w->curveFrame = 0x40;
        break;
    }
    Player_ClearVelocity(em);
}

/* The curve step of a curve set without curves: does nothing and returns 0. */
static s32 EmRathalos_CurveNone(Em *em, EmWork *work)
{
    return 0;
}

extern EmCurveKey *EmRathalos_VelYCurves[];

/*
 * The vertical-speed curve (EmRathalos_VelYCurves[curveKind]): sets velY
 * from the key pair around the work area's curveFrame. Returns 2 at the
 * table's end (a key with t 0 eases velY down to -50; a negative t sets
 * velY 0 and restarts the frame), else 0.
 */
static u8 EmRathalos_CurveVelY(Em *em, EmWork *work)
{
    EmFlyerWork *w = (EmFlyerWork *) work;
    EmCurveKey *key;
    f32 frame;
    f32 keyFrame;
    s32 i;
    EmCurveKey *tbl;
    u8 ret;

    tbl = EmRathalos_VelYCurves[((EmFlyerWork *)work)->curveKind];
    ret = 0;
    if (tbl == NULL) {
        return ret;
    }
    frame = w->curveFrame;
    if (frame != 0.0f && frame != 1.0f) {
        i = 1;
        while (i != 0) {
            key = &tbl[i];
            keyFrame = key->t;
            if (frame > keyFrame && keyFrame > 0.0f) {
                i++;
            } else if (keyFrame == 0.0f) {
                ret = 2;
                if (em->velY > -50.0f) {
                    em->velY -= 1.0f;
                }
                break;
            } else if (keyFrame < 0.0f) {
                w->curveFrame = 0;
                ret = 2;
                em->velY = 0.0f;
                break;
            } else {
                i = 0;
                em->velY = (key->v - key[-1].v) / ((keyFrame - key[-1].t) / em->motion[0].step);
            }
        }
    }
    return ret;
}

extern EmCurveKey *EmRathalos_SpeedCurves[];

/*
 * The forward-speed curve (EmRathalos_SpeedCurves[curveKind]): sets velZ
 * from the key pair around curveFrame; returns 4 at the table's end.
 */
static u8 EmRathalos_CurveSpeed(Em *em, EmWork *work)
{
    EmFlyerWork *w = (EmFlyerWork *) work;
    EmCurveKey *key;
    f32 frame;
    f32 keyFrame;
    s32 i;
    EmCurveKey *tbl;
    u8 ret;

    tbl = EmRathalos_SpeedCurves[((EmFlyerWork *)work)->curveKind];
    ret = 0;
    if (tbl == NULL) {
        return ret;
    }
    frame = w->curveFrame;
    if (frame != 0.0f && frame != 1.0f) {
        i = 1;
        while (i != 0) {
            key = &tbl[i];
            keyFrame = key->t;
            if (frame > keyFrame && keyFrame != 0.0f) {
                i++;
            } else if (keyFrame == 0.0f) {
                ret = 4;
                break;
            } else {
                i = 0;
                em->velZ = (key->v - key[-1].v) / ((keyFrame - key[-1].t) / em->motion[0].step);
            }
        }
    }
    return ret;
}

/*
 * Runs the curves whose flags (curveNone, curveVelY, curveSpeed) are set,
 * advances curveFrame by the motion's step, and in the air moves the
 * monster along its yaw. Returns the curves' end bits (2: vertical speed,
 * 4: forward speed).
 */
u8 EmRathalos_UpdateCurves(Em *em)
{
    EmFlyerWork *w = (EmFlyerWork *)&em->work;
    u8 ret = 0;

    if (w->curveNone != 0) {
        ret = EmRathalos_CurveNone(em, (EmWork *) w);
    }
    if (w->curveVelY != 0) {
        ret |= EmRathalos_CurveVelY(em, (EmWork *) w);
    }
    if (w->curveSpeed != 0) {
        ret |= EmRathalos_CurveSpeed(em, (EmWork *) w);
    }
    w->curveFrame += (s16) em->motion[0].step;
    if (em->stance == EM_STANCE_AIR) {
        w->moveAngX = 0;
        w->moveAngY = em->rotY;
        w->moveAngZ = 0;
        Em_MoveRotatedAccel(em, &w->moveAngX);
    }
    return ret;
}

/* The file's initialised data, in the game's order: the families' tables and curves */
extern u32 EmCephadrome_VelYCurves_3[];
EmFlyParam EmRathalos_FlyParam = {
#include "game/008C9100.inc"
};
EmFlyParam EmRathalos_FlyParam2 = {
#include "game/008C9160.inc"
};
EmTurnParams EmRathalos_TurnParams[2] = {
#include "game/008C91C0.inc"
};
Vec3f EmRathalos_CarveThrowPt = {
#include "game/008C91F8.inc"
};
s32 GmEm_ReturnZero(void); /* gm_sub */
void *EmRathalos_Callbacks[8] = {
    EmRathalos_SpawnEffects, EmRathalos_SpawnEffects, EmRathalos_TableNop, EmRathalos_UpdateMotionEffects,
    GmEm_ReturnZero, EmRathalos_Reinit, EmRathalos_AllowPartBreak, NULL,
};
EmCurveKey EmRathalos_MoveVelYCurves_0[8] = {
#include "game/008C9230.inc"
};
EmCurveKey EmRathalos_MoveVelYCurves_1[20] = {
#include "game/008C9270.inc"
};
EmCurveKey EmRathalos_MoveVelYCurves_3[10] = {
#include "game/008C9310.inc"
};
EmCurveKey EmRathalos_MoveVelYCurves_5[6] = {
#include "game/008C9360.inc"
};
EmCurveKey EmRathalos_MoveVelYCurves_6[10] = {
#include "game/008C9390.inc"
};
EmCurveKey *EmRathalos_MoveVelYCurves[16] = {
    EmRathalos_MoveVelYCurves_0, EmRathalos_MoveVelYCurves_1, NULL, EmRathalos_MoveVelYCurves_3,
    NULL, EmRathalos_MoveVelYCurves_5, EmRathalos_MoveVelYCurves_6, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
};
EmCurveKey EmRathalos_VelYCurves_15[30] = {
#include "game/008C9420.inc"
};
EmCurveKey EmRathalos_VelYCurves_3[10] = {
#include "game/008C9510.inc"
};
EmCurveKey EmRathalos_VelYCurves_5[14] = {
#include "game/008C9560.inc"
};
EmCurveKey EmRathalos_VelYCurves_8[8] = {
#include "game/008C95D0.inc"
};
EmCurveKey EmRathalos_VelYCurves_14[12] = {
#include "game/008C9610.inc"
};
EmCurveKey EmRathalos_VelYCurves_9[18] = {
#include "game/008C9670.inc"
};
EmCurveKey EmRathalos_VelYCurves_10[14] = {
#include "game/008C9700.inc"
};
EmCurveKey EmRathalos_VelYCurves_11[4] = {
#include "game/008C9770.inc"
};
EmCurveKey EmRathalos_VelYCurves_12[8] = {
#include "game/008C9790.inc"
};
EmCurveKey EmRathalos_VelYCurves_13[22] = {
#include "game/008C97D0.inc"
};
EmCurveKey EmRathalos_VelYCurves_16[12] = {
#include "game/008C9880.inc"
};
EmCurveKey *EmRathalos_VelYCurves[20] = {
    NULL, NULL, (EmCurveKey *)EmCephadrome_VelYCurves_3, EmRathalos_VelYCurves_3,
    NULL, EmRathalos_VelYCurves_5, NULL, NULL,
    EmRathalos_VelYCurves_8, EmRathalos_VelYCurves_9, EmRathalos_VelYCurves_10, EmRathalos_VelYCurves_11,
    EmRathalos_VelYCurves_12, EmRathalos_VelYCurves_13, EmRathalos_VelYCurves_14, EmRathalos_VelYCurves_15,
    EmRathalos_VelYCurves_16, NULL, NULL, NULL,
};
EmCurveKey EmRathalos_SpeedCurves_5[14] = {
#include "game/008C9930.inc"
};
EmCurveKey EmRathalos_SpeedCurves_8[8] = {
#include "game/008C99A0.inc"
};
EmCurveKey EmRathalos_SpeedCurves_14[12] = {
#include "game/008C99E0.inc"
};
EmCurveKey EmRathalos_SpeedCurves_9[14] = {
#include "game/008C9A40.inc"
};
EmCurveKey EmRathalos_SpeedCurves_10[12] = {
#include "game/008C9AB0.inc"
};
EmCurveKey *EmRathalos_SpeedCurves[18] = {
    NULL, NULL, NULL, NULL,
    NULL, EmRathalos_SpeedCurves_5, NULL, NULL,
    EmRathalos_SpeedCurves_8, EmRathalos_SpeedCurves_9, EmRathalos_SpeedCurves_10, NULL,
    NULL, NULL, EmRathalos_SpeedCurves_14, NULL,
    NULL, NULL,
};
