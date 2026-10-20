/*
 * Monster family: Khezu and Red Khezu (EM_KIND_KHEZU and EM_KIND_RED_KHEZU),
 * which share one set of state functions (EmKhezu_*). The file starts at the
 * family's first function, EmKhezu_MotionSounds.
 *
 * The family has the shape of every monster family (include/em.h): Init and
 * Start, one function per state, a dispatcher per state group (Run*State)
 * and a function that enters a state (Set*State, ChangeState), the
 * per-frame Update that turns Em_UpdateDamage's result into reactions, and
 * the motion effects and curves. The state functions take the monster and
 * its work area (EmKhezuWork, the family's view of EmWork); Em.step is the
 * step within the current state. include/em_khezu.h names the family's
 * motions and states.
 */
#include "common.h"
#include "scene.h"
#include "quest.h"
#include "em.h"
#include "em_khezu.h"
#include "player.h"
#include "area.h"
/* The functions this file calls */
u16 Area_IdForCurrentTime(u16);
void QuestEm_Hunted(Em *em);

void PartFx_SpawnForEm(Em *em, s32 a, s32 b);
s32 EmMotion_InFrameRange(void *em, s32 layer, f32 from, f32 to);
u16 Rand_Next(s32 stream);
void KhezuAttack_Create(Em *em, s8 kind);
void ThunderSpark_CreateOnEm(Em *em, u8 k);
void ThunderSpark_CreateTimed(Em *em, u8 k, s32 n, f32 f);
void ElementBurst_CreateOnEm(Em *em, u8 k, u8 c, f32 scale);
void EmKhezu_ChangeState(Em *em, u16 group, u16 state, u8 mode);
u8 EmKhezu_UpdateCurves(Em *em);
/* unit's own declaration: declared here as the callers use it; with the definition's void (Em *em, s16 n), the calls in EmKhezu_EatNeed2 and EmKhezu_EatNeed1 compile differently */
void Em_AddHp(Em *em, s32 n);
void Em_BeginStatus1(Em *em);
void Em_BeginStatus2(Em *em);
void Em_BeginStatus4(Em *em);
void Em_EndStatus2(Em *em);
void Em_EndStatus4(Em *em);
void Em_PlaySeAtBone(Em *em, s32 se, s32 bone);
void Em_PlaySeAtBoneOnFrame(Em *em, s32 frame, s32 se, s32 bone);
void Em_SetLayerMotion(Em *em, s32 motion, s32 blend, s32 frame, s32 layer);
void Em_SetMotion(Em *em, s32 motion, s32 a, s32 b);
/* unit's own declaration: declared here as the callers use it; with the definition's void (Em *em, s32 group, s32 state, u8 mode), the calls in EmAnteka_ChangeState, EmKhezu_ChangeState and others compile differently */
void Em_SetStateSync(Em *em, u16 group, u16 state, u8 mode);
u16 Math_AngleXZ(f32 *from, f32 *to);
void Player_ClearFlag(void *, u32 flag);
void Player_SetFlag(void *obj, u32 flag);
void QuestEm_Captured(Em *em);
void EmRangedAttack_CreateAtBone(Em *, s32, s32, s32, s32, s32);
void Em_SelectSightRow(Em *em, u8 idx);
void Em_MoveRotated(Em *em, void *src);
void Em_MoveRotatedAccel(Em *, void *);
void Em_LoadReactionSet(Em *em, u8 k);
void Em_ComputeGoal(Em *em, f32 *target);
void Em_ComputeGoal2(Em *em, f32 *target);
void Em_GetPitchToGoal(Em *em, s32 *out, f32 a, f32 b);
void Em_ClearLowHpNeed(Em *em);
void Em_ClearNeed1State(Em *em);
void Em_ClearNeed2State(Em *em);
void Em_SpawnStatus4Effect(Em *em, s32 a);
void Em_AddNeed1(Em *em, s32 n);
void Em_AddNeed2(Em *em, s32 n);
void Em_SetHitReactMode(Em *em, u8 a);
void Em_SetTrapTimeGoal(Em *em);
/* unit's own declaration: declared here as the callers use it; with the definition's s32 (Em *em, s16 n, s16 max, s16 interval), the calls in EmKhezu_Rest and EmKhezu_Status2LieDown compile differently */
s32 Em_RegenHp(Em *em, s32 n, s32 max, s32 interval);
void Em_ClearPoison(Em *em);
void Em_SetHold2(Em *em);
void Em_SetCaptured(Em *em, s32 a);
void Em_PlaySeTypeAtBoneInFrames(Em *em, s32 frame, s32 frame2, s32 se, s32 bone, s32 vol, s32 a);
void Em_PlaySeAtBoneOnLayerFrame(Em *em, s32 frame, s32 se, s32 bone, s32 flag);
void Em_PlaySeAtBoneVolOnFrame(Em *em, s32 frame, s32 se, s32 bone, s32 vol);
s32 Em_StepToGoal(Em *em, u16 step, u8 flags);
u8 Em_TurnToGoal(Em *em, u16 step, u8 flags);
void Em_StartTurnToTarget(Em *em, EmTurnParams *t);
s32 Em_TurnStep(Em *em, EmTurnParams *t);
void Em_ClearTilt(Em *em, s32 arg1);
void Em_RestartScript(Em *em);
void EmKhezu_Init(Em *);
void EmKhezu_Status2Downed(Em *, EmWork *);
void EmKhezu_Status1Downed(Em *, EmWork *);
void EmKhezu_TurnToGoal(Em *, EmWork *);
void EmKhezu_Land(Em *, EmWork *);
void EmKhezu_FlyOutOfArea(Em *, EmWork *);
void EmKhezu_FlyToGoalAndLand(Em *, EmWork *);
void EmKhezu_FlyIntoArea(Em *, EmWork *);
void EmKhezu_CeilingShots(Em *, EmWork *);
void EmKhezu_StoryScene(Em *, EmWork *);
void EmKhezu_Captured(Em *, EmWork *);
void EmKhezu_VersusEatBait(Em *, EmWork *);
void EmKhezu_Die(Em *em, EmWork *work);
void EmKhezu_DieHeld(Em *, EmWork *);
void EmKhezu_DieFalling(Em *, EmWork *);
void EmKhezu_RunReactState(Em *, EmWork *);
void EmKhezu_MotionSounds(Em *em, struct EmWork *);
void EmKhezu_MotionEffects(Em *, EmKhezuWork *);
void EmKhezu_SpawnLandingImpact(Em *em);
void EmKhezu_MoveByVelYCurve(Em *em, s32 kind);
void EmKhezu_StartCurves(Em *em, u8 kind);
s32 EmKhezu_InSpecialArea(u16 area);
void AlertMark_Create(void *owner, s16 v, u8 k);
void DustCloud_Create(Em *em, s16 k, s16 c, f32 scale);
/* unit's own declaration: declared here as the callers use it; with the definition's void (Em *em, u8 k, s16 v, s16 t, f32 y, f32 scale), the calls in EmKhezu_HeldDowned compile differently */
void StunStars_CreateEm(Em *, s32, s32, s32, f32, f32);
s32 EmMotion_PassedFrameScaled(void *p, f32 frame, s32 layer);
s32 EmMotion_ReachedFrame(void *obj, s32 a, f32 frame);
f32 Vec_Distance(f32 *a, f32 *b);
void CamShake_ReqAIf(Em *obj, s32 i);
void Vec_Set(f32 *out, f32 x, f32 y, f32 z);
f32 Vec_DistanceXZ(f32 *a, f32 *b);
s32 Versus_BothDefeated(void);
s32 QuestEm_CanAppear(Em *em);
void Player_ClearVelocity(void *);
void Shadow_Spawn(void *obj, s16 type);
extern f32 gAreaGroundY[];
extern EmTurnParams EmKhezu_TurnParams[6];
u16 Area_DayId(u16 area);
void Em_TargetPlayerInRange(Em *);
s32 Game_IsOnline(void);
void Versus_MarkDefeat(Em *);
s8 QuestEm_Appear(Em *em);
void Em_ResetToBasePose(Em *em);
void Em_BreakPart(Em *em, s32 part);
void Em_EndAllStatus(Em *em);
void Em_BeginStatus5(Em *em);
void Em_EndStatus1(Em *em);
void Em_EndStatus5(Em *em);
void Em_SetSpeedScaled(Em *em, u16 flags, f32 x, f32 y);
void Em_CheckRageByHp(Em *em, s16 n);
void Em_TypeSetup(Em *em);
void Em_SetTargetAngle(Em *em);
s32 Em_GetTargetPos(Em *em, u8 idx, f32 *out);
void Em_StartRage(Em *em);
void Em_StartPendingRage(Em *em);
void Em_ResetNotice(Em *em);
void Em_RefillNeed3(Em *em);
void Em_StartGoalAction(Em *em, s32 arg1);
void Em_ClearPart(Em *em, u16 part);
void MinePoint_Remove(s32);
void EmKhezu_SetIdleState(Em *em, u16 state, u8 mode);
void EmKhezu_SetMoveState(Em *em, u16 state, u8 mode);
void EmKhezu_SetFlyState(Em *em, u16 state, u8 mode);
void EmKhezu_SetAttackState(Em *em, u16 state, u8 mode);
u8 EmKhezu_NearestCeilingPoint(Em *em, f32 *out);
struct AreaTri;
u8 AreaGrid2_GetCeiling(f32 *pos, f32 *y, struct AreaTri *info);
void Em_SpawnLyingPuffs(Em *em, s32 bone, f32 *pos, f32 scale);
void Em_SetMotionRow(Em *em, s32 row, s32 arg2, s32 arg3);
void Em_SetPartScaled(Em *em, s32 bit);
s32 Em_ComputeMaxHp(Em *em, s16 idx);
void EmTail_Create(Em *);
s8 EmMine_CreateTimed0(Em *em, f32 *pos);
void Vec_Copy(f32 *out, f32 *in);
void VersusBait_Create(Player *p, Em *em, u8 k, s32 unused, f32 *pos);
void Versus_RouteEmByTeam(Em *em);
s32 Versus_DefeatCamera(Em *pl);
extern Vec3f EmKhezu_BaitThrowPt __attribute__((aligned(8)));
extern EmCurveKey *EmKhezu_MoveVelYCurves[];
s8 EmMine_Create(Em *em, s32 idx);
s32 EmMine_Update(Em *em);
void EmMine_Remove(Em *em);
struct EmIdEntry;
f32 *Em_AreaEntryValue(Em *em, struct EmIdEntry *entry);
void Em_PickRoutePoint(Em *em);
void Em_AreaExitPoint(Em *em, f32 *out);
void Em_EnterNextArea(Em *em);
s32 Area_ContainsPos(s32 id, f32 *pos);
void Hud_TriggerGlow(Em *em);
AreaRecord *Stage_GetAreaRecord(u16 x);
s32 Save_TestEventFlag(s32 flag);
void GmEm_ResetSize(Em *em);
s32 Em_SetCombat(Em *em, u8 a, s16 b);
s32 Em_UpdateCombatTimer(Em *em);
void Em_SetStatus4Timer(Em *em);
void Em_SetStatus2Timer(Em *em);
u8 Em_UpdateDamage(Em *em, u8 *out);
void Em_UpdatePendingScript(Em *em);
void Em_ClearHitReactMode(Em *em);
void Em_SetStatus5Timer(Em *em);
void PartFx_SpawnForEmScaled(Em *em, s32 a, s32 b, f32 scale);
void EmKhezu_RunState(Em *em, EmWork *work);
void Em_RaisePlayerAreaTriggerLax(Em *em);
void Em_HoldPosNoPlayers(Em *em, u16 a, u16 b);
void Em_SetTrapTime(Em *em, u8 *p);
void Em_SetStatus1Timer(Em *em);
void Em_ResetKindTimer(Em *em);
s32 Em_IsHeld(Em *em);
u16 Map_DayId(u16);
void Em_CheckLowHp(Em *em, f32 ratio);
void Em_DecayNeed1Only(Em *em);
void Em_DecayNeed1(Em *em);
/* end of prototypes */

void EmKhezu_MotionSounds(Em *em, EmWork *work)
{
    s16 motion;

    motion = work->sndMotion;
    switch (motion) {
    case 0x3E9:
        Em_PlaySeAtBoneOnFrame(em, 0xFA, 0x34, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x172, 0x57, 0x23);
        break;
    case 0x3EB:
        Em_PlaySeAtBoneOnFrame(em, 0x22, 0x35, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 1, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x8A, 1, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0xBC, 1, 0x1A);
        break;
    case 0x3ED:
        Em_PlaySeAtBoneOnFrame(em, 0x18, 1, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 1, 0x1A);
        break;
    case 0x3EE:
        Em_PlaySeAtBoneOnFrame(em, 0x18, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 1, 0x14);
        break;
    case 0x3EF:
        Em_PlaySeAtBoneOnFrame(em, 6, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 1, 0x14);
        break;
    case 0x3F0:
        Em_PlaySeAtBoneOnFrame(em, 6, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 1, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 1, 0x1A);
        break;
    case 0x3F2:
        Em_PlaySeAtBoneOnFrame(em, 6, 0x52, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 6, 4, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 0, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 0x12, 0x14);
        break;
    case 0x3F3:
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0xB, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x42, 0xB, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x7A, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x7E, 0xB, 0xC);
        break;
    case 0x3F7:
        Em_PlaySeAtBoneOnFrame(em, 4, 0xC, 6);
        Em_PlaySeAtBoneOnFrame(em, 8, 0xC, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 0xB, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x8E, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x8A, 0xB, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0xD4, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0xD8, 0xB, 0xC);
        break;
    case 0x3FA:
        Em_PlaySeAtBoneOnFrame(em, 0x28, 9, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0xD, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x52, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x56, 0xB, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x88, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x8C, 0xB, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0xB6, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0xBA, 0xB, 0xC);
        break;
    case 0x3FB:
        Em_PlaySeAtBoneOnFrame(em, 2, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 6, 0xB, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 4, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 4, 1, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 7, 0);
        break;
    case 0x407:
        Em_PlaySeAtBoneOnFrame(em, 2, 0x55, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xB6, 0x55, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x16C, 0x55, 0x23);
        break;
    case 0x408:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x57, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x8E, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x17, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x13, 0);
        break;
    case 0x409:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x57, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0x2D, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x7E, 3, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0xB6, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0x16, 0);
        break;
    case 0x40A:
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0x59, 0x23);
        break;
    case 0x40B:
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0x24, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 0x22, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x15, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0xA2, 0, 0x1A);
        break;
    case 0x40D:
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x48, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0xC, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 2, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 1, 0x1A);
        break;
    case 0x40E:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x57, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 0xA4, 7, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x90, 3, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x88, 0xE, 6);
        Em_PlaySeAtBoneOnFrame(em, 0xB2, 4, 0x22);
        break;
    case 0x40F:
        Em_PlaySeAtBoneOnFrame(em, 2, 0x57, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 2, 0x17, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 3, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x66, 0x10, 0x1A);
        break;
    case 0x413:
        Em_PlaySeAtBoneOnFrame(em, 8, 0x23, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0xF, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x14, 0x2A);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 1, 0x14);
        break;
    case 0x417:
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x74, 0x28, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xA8, 0x51, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xAC, 0x18, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xAC, 0x37, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x16, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0x36, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0x38, 0x23);
        break;
    case 0x41B:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x23, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x72, 0x34, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xBA, 0x34, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x17, 0);
        break;
    case 0x41E:
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 0x5B, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 8, 0, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x88, 0, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0xB6, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0xFA, 0x12, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x138, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x54, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 0xBE, 0x16, 0);
        break;
    case 0x423:
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 6, 0x22, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x68, 3, 0x1A);
        break;
    case 0x424:
        Em_PlaySeAtBoneOnFrame(em, 6, 0x53, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 0x13, 0x2A);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 6, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x90, 0, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0xC, 0xC);
        break;
    case 0x425:
        Em_PlaySeAtBoneOnFrame(em, 6, 0x53, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x12, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 6, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 4, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x6E, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x5A, 0x15, 0);
        Em_PlaySeAtBoneOnFrame(em, 0xAA, 3, 0x1A);
        break;
    case 0x426:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x4F, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x56, 0x1F, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 4, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 8, 0xD, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x62, 0x13, 0x2A);
        Em_PlaySeAtBoneOnFrame(em, 0x7E, 3, 0x1A);
        break;
    case 0x427:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x52, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 0x1F, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0, 0x1A);
        break;
    case 0x428:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x52, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x12, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x7A, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x96, 0, 0x14);
        break;
    case 0x42B:
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0x2D, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0x17, 0);
        break;
    case 0x42C:
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 2, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 4, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0xDC, 0x10, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x108, 7, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x114, 0x17, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x180, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0x52, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x8C, 0x28, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xB4, 0x2C, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x192, 0x2F, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x174, 0xF, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x176, 0xE, 0xC);
        break;
    case 0x42D:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x52, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x13, 0x2A);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 7, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0x11, 0);
        break;
    case 0x42E:
    case 0x433:
        Em_PlaySeAtBoneOnFrame(em, 8, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x47, 0x23);
        break;
    case 0x42F:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 1, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 1, 0x1A);
        break;
    case 0x430:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 1, 0x14);
        break;
    case 0x432:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x52, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x13, 0x2A);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 7, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0x11, 0);
        break;
    case 0x434:
        Em_PlaySeAtBoneOnFrame(em, 0x10, 0x5A, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 0, 0x1A);
        break;
    case 0x435:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x53, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0xB, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 0x14, 0);
        break;
    case 0x436:
        Em_PlaySeAtBoneOnFrame(em, 0xA, 9, 0);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 7, 0x2A);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x11, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 0x54, 0x23);
        break;
    case 0x437:
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x23, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0x22, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x66, 0x22, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xA4, 0x22, 0x23);
        break;
    case 0x43A:
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 0x2A, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0);
        break;
    case 0x43C:
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0xD, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 0xA, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0xE, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 0xE, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 0xA, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 0xC, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x5C, 0xC, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 5, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 0, 0x14);
        break;
    case 0x442:
        Em_PlaySeAtBoneOnFrame(em, 0x28, 0x27, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0xD, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 0xD, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x7C, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x80, 0xB, 0xC);
        break;
    case 0x447:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x10, 0);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x12, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 7, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x4F, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x2A, 0x23);
        break;
    case 0x448:
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0x12, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 0x12, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0xC, 6);
        Em_PlaySeAtBoneOnFrame(em, 8, 0xC, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x50, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0);
        break;
    case 0x449:
        Em_PlaySeAtBoneOnFrame(em, 0x22, 0x16, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 0x16, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x62, 4, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 3, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x2A, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x2B, 0x23);
        break;
    case 0x44A:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 6);
        Em_PlaySeAtBoneOnFrame(em, 8, 0x16, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 3, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x21, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0x20, 0x23);
        break;
    case 0x44C:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x59, 0x23);
        break;
    case 0x44D:
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 1, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 0x11, 0);
        break;
    case 0x44E:
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x62, 0x21, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x42, 1, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 0x16, 0);
        break;
    case 0x44F:
        Em_PlaySeAtBoneOnFrame(em, 0x10, 0x59, 0x23);
        break;
    case 0x450:
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x6E, 0x15, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x5C, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x60, 0xD, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x70, 6, 0x1A);
        break;
    case 0x451:
        Em_PlaySeAtBoneOnFrame(em, 0x40, 0x5B, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x16, 0);
        break;
    case 0x452:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x28, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0x24, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 0x15, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x42, 0x11, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 6, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x42, 0xD, 0xC);
        break;
    case 0x453:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x54, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 7, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x8C, 0x16, 0);
        break;
    case 0x454:
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x24, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 6, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0x11, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 6, 0x14);
        break;
    case 0x455:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x24, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x15, 0);
        break;
    case 0x456:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x14, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 0x21, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xF6, 0x31, 0x23);
        break;
    case 0x457:
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x16, 0);
        break;
    case 0x458:
        Em_PlaySeAtBoneOnFrame(em, 0x28, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x8C, 0x21, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xB4, 0x4F, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xEC, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xB2, 0x15, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x96, 1, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0xB0, 0x12, 0x1A);
        break;
    case 0x459:
        if (em->type == 0xF) {
            Em_PlaySeAtBoneOnFrame(em, 4, 0x20, 0x23);
            Em_PlaySeAtBoneOnFrame(em, 0x40, 0x22, 0x23);
            Em_PlaySeAtBoneOnFrame(em, 0x64, 0x21, 0x23);
            Em_PlaySeAtBoneOnFrame(em, 0x40, 1, 0x14);
            Em_PlaySeAtBoneOnFrame(em, 0x88, 1, 0x14);
            break;
        }
        Em_PlaySeAtBoneOnFrame(em, 0x20, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0xBA, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0x22, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 0x24, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0x23, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x15, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x56, 2, 0x23);
        break;
    case 0x45A:
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 0x28, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x70, 0x56, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 0x14, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 0x15, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x36, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x74, 0x37, 0);
        Em_PlaySeAtBoneOnFrame(em, 0xC2, 0x36, 0);
        Em_PlaySeAtBoneOnFrame(em, 0xC2, 0x37, 0);
        break;
    case 0x45B:
        Em_PlaySeAtBoneOnFrame(em, 0x34, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x16, 0x22);
        break;
    case 0x45D:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x54, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 8, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x92, 0x16, 0);
        break;
    case 0x45E:
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0x1C, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0x1C, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x62, 0x1C, 0x2A);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 0x1B, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x60, 0x1B, 6);
        break;
    case 0x45F:
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0x1B, 6);
        Em_PlaySeAtBoneOnFrame(em, 2, 0x1B, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 0x1C, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x1C, 0x14);
        break;
    case 0x460:
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x1B, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0x1B, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 0x1C, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 0x1C, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 0x1C, 0x14);
        break;
    case 0x461:
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x1B, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0x1B, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 0x1C, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 0x1C, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 0x1C, 0x14);
        break;
    }
}

/*
 * The family's setup (Em_TypeSetup's entry for the Khezu kinds): nothing to
 * do.
 */
void EmKhezu_Setup(Em *em)
{
}

/*
 * Callback slots 0 and 1: creates the stage object of kind 0x13 (type 0x22)
 * and the effect 01 for the monster, as Rathalos does.
 */
void EmKhezu_SpawnEffects(void *em)
{
    AlertMark_Create(em, 0x22, 0);
    Shadow_Spawn((Player *) em, 0);
}

/*
 * Sets a Khezu up: the idle motion, the scaled parts 3 and 4, in the
 * quest-less stage (gQuest.stage 0) a fixed start position per slot, full
 * hp from the size, and the work area's turn speeds (EmKhezuWork.turnSpeed
 * is stored as a word). The first time (Em.fxFlags bit 0 clear) it also
 * hides mesh 1 and creates the kind-9 stage object.
 */
void EmKhezu_Init(Em *em)
{
    EmKhezuWork *w = (EmKhezuWork *)&em->work;

    f32 fslot;
    s16 maxHp;
    u8 slotNo;
    u8 fxFlags;

    Em_SetMotionRow(em, KHEZU_MOT_IDLE, 0, 0);
    Em_SetPartScaled(em, 3);
    Em_SetPartScaled(em, 4);
    if (gQuest.stage == 0) {
        em->rotY = 0x8000;
        slotNo = em->slotNo;
        fslot = (f32) (u32) slotNo;
        em->posX = 10000.0f + (1500.0f * fslot);
        em->posY = 0.0f;
        em->posZ = 10000.0f;
    }
    maxHp = Em_ComputeMaxHp(em, 0);
    em->hp = maxHp;
    em->hpMax = maxHp;
    em->scriptPending = 1;
    em->blind = 0;
    em->unk_InitFlag = 1;
    em->noAlert = 0;
    em->painted = 0;
    w->goalYaw = 0x4000;
    w->unk02C = 0x100;
    *(s32 *)&w->turnSpeed = 0x200;
    w->unk034 = 0x100;
    w->unk01C = 0;
    w->unk044 = 0;
    fxFlags = em->fxFlags & 1;
    em->fxFlags = fxFlags;
    if (fxFlags == 0) {
        switch (em->type) {
        case EM_KIND_KHEZU:
        case EM_KIND_RED_KHEZU:
            em->meshShown[1] = 0;
            EmTail_Create(em);
            break;
        }
    }
}

/*
 * Starts a Khezu: Init, then in QUEST_ID_KHEZU_SCENE, until
 * EVENT_FLAG_KHEZU_SCENE is set, places it at the scene's spot facing its goal; then
 * KHEZU_IDLE on the ground.
 */
void EmKhezu_Start(Em *em)
{
    EmKhezu_Init(em);
    if (em->type == EM_KIND_KHEZU || em->type == EM_KIND_RED_KHEZU) {
        switch (gQuest.stage) {
        case QUEST_ID_KHEZU_SCENE:
            if (Save_TestEventFlag(EVENT_FLAG_KHEZU_SCENE) == 0) {
                em->area = Area_IdForCurrentTime(AREA_SNOWY_MOUNTAINS_3);
                em->posX = 12220.0f;
                em->posY = 0.0f;
                em->posZ = 10119.0f;
                em->goal[0] = 9290.0f;
                em->goal[1] = 0.0f;
                em->goal[2] = 10238.0f;
                em->rotX = 0;
                em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
                em->rotZ = 0;
            }
            break;
        }
        em->stance = EM_STANCE_GROUND;
        EmKhezu_ChangeState(em, EM_GROUP_IDLE, KHEZU_IDLE, EM_SYNC_LOCAL);
    }
}

/* Callback slot 5: Init again, the state kept. */
void EmKhezu_Reinit(Em *em)
{
    EmKhezu_Init(em);
}

/*
 * Ends a ground state: normal motion speed, ground stance, no look-at; the
 * script is asked for the next action and the monster goes back to
 * KHEZU_IDLE.
 */
void EmKhezu_EndState(Em *em)
{
    em->motionSpeed = 1.0f;
    em->stance = EM_STANCE_GROUND;
    em->lookAtTarget = 0;
    em->scriptPending = 1;
    EmKhezu_ChangeState(em, EM_GROUP_IDLE, KHEZU_IDLE, EM_SYNC_LOCAL);
}

/*
 * Ends a ceiling state: air stance, Em.ceilingTimer refreshed; the script
 * is asked and the monster hangs again (KHEZU_FLY_CEIL_HANG).
 */
void EmKhezu_EndCeilingState(Em *em)
{
    em->motionSpeed = 1.0f;
    em->stance = EM_STANCE_AIR;
    em->lookAtTarget = 0;
    em->scriptPending = 1;
    em->ceilingTimer = 5;
    EmKhezu_ChangeState(em, EM_GROUP_FLY, KHEZU_FLY_CEIL_HANG, EM_SYNC_LOCAL);
}

/*
 * Ends a flight state: air stance; the script is asked and the monster
 * hovers once (KHEZU_FLY_HOVER_ONCE).
 */
void EmKhezu_EndFlyState(Em *em)
{
    em->scriptPending = 1;
    em->motionSpeed = 1.0f;
    em->stance = EM_STANCE_AIR;
    em->lookAtTarget = 0;
    EmKhezu_ChangeState(em, EM_GROUP_FLY, KHEZU_FLY_HOVER_ONCE, EM_SYNC_LOCAL);
}

/*
 * Restarts the idle loop on layer `layer` (with a 10-frame blend) once its
 * motion has stopped.
 */
void EmKhezu_RestartLayerMotion(Em *em, s32 layer)
{
    if (em->motion[layer].playing == 0) {
        switch (layer) {
        case 0:
            Em_SetLayerMotion(em, EM_MOTION_ID(0, KHEZU_MOT_IDLE), 10, 0, 0);
            break;
        case 1:
            Em_SetLayerMotion(em, EM_MOTION_ID(1, KHEZU_MOT_IDLE), 10, 0, 1);
            break;
        case 2:
            Em_SetLayerMotion(em, EM_MOTION_ID(2, KHEZU_MOT_IDLE), 10, 0, 2);
            break;
        }
    }
}

/*
 * KHEZU_IDLE_STAND: the idle motion until it ends or 30 frames pass
 * (counted only on the owner's console), then EmKhezu_EndState.
 */
void EmKhezu_Stand(Em *em, EmWork *work)
{
    s32 timeLeft;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KHEZU_MOT_IDLE, 0, 0);
        em->timer = 0x1E;
        break;
    case 1:
        if ((em->remote == 0) && ((em->motion[0].playing == 0) || (timeLeft = em->timer - 1, em->timer = timeLeft, (timeLeft <= 0)))) {
            em->step++;
            EmKhezu_EndState(em);
        }
        break;
    }
}

/*
 * KHEZU_IDLE: the idle motion once, then EmKhezu_EndState, which comes back
 * here: the loop in which the script picks the next action.
 */
void EmKhezu_Idle(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KHEZU_MOT_IDLE, 0, 0);
        break;
    case 1:
        if ((em->remote == 0) && (em->motion[0].playing == 0)) {
            em->step++;
            EmKhezu_EndState(em);
        }
        break;
    }
}

/* KHEZU_IDLE_LOOP_START: its motion on layer 1 once, then KHEZU_IDLE_LOOP. */
void EmKhezu_IdleLoopStart(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KHEZU_MOT_IDLE_LOOP_START, 0, 0);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmKhezu_ChangeState(em, EM_GROUP_IDLE, KHEZU_IDLE_LOOP, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmKhezu_ChangeState(em, EM_GROUP_IDLE, KHEZU_IDLE_LOOP, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * KHEZU_IDLE_LOOP: its motion on layer 1, restarting the state each time
 * the motion ends; only a hit or a change from outside leaves it.
 */
void EmKhezu_IdleLoop(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KHEZU_MOT_IDLE_LOOP, 0, 0);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmKhezu_ChangeState(em, EM_GROUP_IDLE, KHEZU_IDLE_LOOP, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmKhezu_ChangeState(em, EM_GROUP_IDLE, KHEZU_IDLE_LOOP, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/* KHEZU_IDLE_ACTION_A: its motion on layer 1 once, then EmKhezu_EndState. */
void EmKhezu_IdleActionA(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KHEZU_MOT_IDLE_ACTION_A, 0, 0);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmKhezu_EndState(em);
        }
        break;
    }
}

/*
 * KHEZU_IDLE_ACTION_B: its motion on layer 1 once, with layers 0 and 2 kept
 * on their idle loops, then EmKhezu_EndState.
 */
void EmKhezu_IdleActionB(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KHEZU_MOT_IDLE_ACTION_B, 0, 0);
        Em_SetLayerMotion(em, EM_MOTION_ID(0, KHEZU_MOT_IDLE), 0xA, 0, 0);
        Em_SetLayerMotion(em, EM_MOTION_ID(2, KHEZU_MOT_IDLE), 0xA, 0, 2);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmKhezu_EndState(em);
        }
        break;
    }
    EmKhezu_RestartLayerMotion(em, 0);
    EmKhezu_RestartLayerMotion(em, 2);
}

/* KHEZU_IDLE_ACTION_C: its motion on layer 1 once, then EmKhezu_EndState. */
void EmKhezu_IdleActionC(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KHEZU_MOT_IDLE_ACTION_C, 0, 0);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmKhezu_EndState(em);
        }
        break;
    }
}

/* KHEZU_IDLE_WAIT: the idle motion for 30 frames, then EmKhezu_EndState. */
void EmKhezu_Wait(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KHEZU_MOT_IDLE, 0, 0);
        em->timer = 0x1E;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmKhezu_EndState(em);
        }
        break;
    }
}

/*
 * KHEZU_IDLE_WAIT_ALERT: the alert idle for 30 frames, then
 * EmKhezu_EndState.
 */
void EmKhezu_WaitAlert(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KHEZU_MOT_ALERT_IDLE, 0, 0);
        em->timer = 0x1E;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmKhezu_EndState(em);
        }
        break;
    }
}

/*
 * KHEZU_IDLE_TURN_TO_GOAL: the turning motion, rotY set to the goal's
 * heading once within 0x200 of it; after frame 60 it waits for the motion's
 * end, then EmKhezu_EndState.
 */
void EmKhezu_IdleTurnToGoal(Em *em, EmWork *work)
{
    u16 goalYaw;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KHEZU_MOT_TURN_IN_PLACE, 0, 0);
        work->goalYaw = Math_AngleXZ(&em->posX, em->goal);
        break;
    case 1:
        goalYaw = work->goalYaw;
        if ((u32) ((((goalYaw - *(u16 *)&em->rotY) & 0xFFFF) + 0x200) & 0xFFFF) < 0x400U) {
            em->rotY = (s32) goalYaw;
        }
        if (EmMotion_ReachedFrame(em, 0, 60.0f) != 0) {
            em->step++;
        }
        break;
    case 2:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmKhezu_EndState(em);
        }
        break;
    }
}

/*
 * KHEZU_IDLE_ACTION_D: its motion on layer 1 with two sounds at bone 0x23,
 * layers 0 and 2 kept on their idle loops, then EmKhezu_EndState. The test
 * of layer 2 compares it with layer 1's idle id, so layer 2 is always set
 * again (the original's code).
 */
void EmKhezu_IdleActionD(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KHEZU_MOT_IDLE_ACTION_D, 0, 0);
        if (em->motionId[0] != EM_MOTION_ID(0, KHEZU_MOT_IDLE)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(0, KHEZU_MOT_IDLE), 0xA, 0, 0);
        }
        if (em->motionId[2] != EM_MOTION_ID(1, KHEZU_MOT_IDLE)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(2, KHEZU_MOT_IDLE), 0xA, 0, 2);
        }
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmKhezu_EndState(em);
        }
        break;
    }
    EmKhezu_RestartLayerMotion(em, 0);
    EmKhezu_RestartLayerMotion(em, 2);
    Em_PlaySeAtBoneOnLayerFrame(em, 2, 0x20, 0x23, 1);
    Em_PlaySeAtBoneOnLayerFrame(em, 0x4A, 0x1F, 0x23, 1);
}

/*
 * KHEZU_IDLE_EAT_NEED2: lowers the head with bite effects and sounds at set
 * frames, then eats and regains 5% of its hp; the need-2 gauge rises every
 * frame and its state is cleared at the end. Hits don't make it flinch.
 */
void EmKhezu_EatNeed2(Em *em, EmWork *work)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_NO_FLINCH);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KHEZU_MOT_EAT_START, 0, 0);
        Em_SelectSightRow(em, 1);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if ((EmMotion_PassedFrameScaled(em, 56.0f, 0U) != 0) || (EmMotion_PassedFrameScaled(em, 108.0f, 0U) != 0)
            || (EmMotion_PassedFrameScaled(em, 126.0f, 0U) != 0) || (EmMotion_PassedFrameScaled(em, 138.0f, 0U) != 0)
            || (EmMotion_PassedFrameScaled(em, 170.0f, 0U) != 0)) {
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
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KHEZU_MOT_EAT, 0, 0);
            Em_AddHp(em, (s32) (0.05f * (f32) em->hpMax));
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SelectSightRow(em, 0);
            Em_LoadReactionSet(em, 0);
            Em_ClearNeed2State(em);
            EmKhezu_EndState(em);
        }
        break;
    }
    Em_AddNeed2(em, 0xDE);
}

/*
 * KHEZU_IDLE_ALERT (and the two rage roars): the alert idle once, then
 * EmKhezu_EndState.
 */
void EmKhezu_AlertIdle(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KHEZU_MOT_ALERT_IDLE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKhezu_EndState(em);
        }
        break;
    }
}

/*
 * KHEZU_IDLE_EAT_START: lowers the head; at frame 200 of that motion
 * KHEZU_IDLE_EAT takes over, the motion running on.
 */
void EmKhezu_EatStart(Em *em, EmWork *work)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_NO_FLINCH);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KHEZU_MOT_EAT_START, 0, 0);
        Em_SelectSightRow(em, 1);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 200.0f, 0U) != 0) {
            em->step++;
            EmKhezu_ChangeState(em, EM_GROUP_IDLE, KHEZU_IDLE_EAT, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmKhezu_ChangeState(em, EM_GROUP_IDLE, KHEZU_IDLE_EAT, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * KHEZU_IDLE_EAT: lets the head-down motion finish, eats once, puts back
 * the normal sight row and reaction set, then EmKhezu_EndState.
 */
void EmKhezu_Eat(Em *em, EmWork *work)
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
            Em_SetMotion(em, KHEZU_MOT_EAT, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKhezu_EndState(em);
        }
        break;
    }
}

/*
 * KHEZU_IDLE_REST: lies down and rests, blind, a hit waking it
 * (EM_HIT_REACT_WAKE); 1 hp every 12 frames up to half its hp. After 9000
 * frames the low-hp need is cleared and KHEZU_IDLE_REST_END follows.
 */
void EmKhezu_Rest(Em *em, EmWork *work)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KHEZU_MOT_REST_START, 0, 0);
        Em_LoadReactionSet(em, 1);
        em->timer = 0x2328;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KHEZU_MOT_REST, 0, 0);
        }
        break;
    case 2:
        Em_RegenHp(em, 1, (s32) (0.5f * (f32) em->hpMax), 0xC);
        if (--em->timer <= 0) {
            em->step++;
            Em_ClearLowHpNeed(em);
            EmKhezu_ChangeState(em, EM_GROUP_IDLE, KHEZU_IDLE_REST_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmKhezu_ChangeState(em, EM_GROUP_IDLE, KHEZU_IDLE_REST_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * KHEZU_IDLE_STATUS2_COLLAPSE: falls over and lies until the status-2 timer
 * (Em_SetStatus2Timer) runs out, then KHEZU_IDLE_STATUS2_END.
 */
void EmKhezu_Status2Collapse(Em *em, EmWork *work)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KHEZU_MOT_COLLAPSE, 0, 0);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KHEZU_MOT_LIE, 0, 0);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            EmKhezu_ChangeState(em, EM_GROUP_IDLE, KHEZU_IDLE_STATUS2_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmKhezu_ChangeState(em, EM_GROUP_IDLE, KHEZU_IDLE_STATUS2_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * KHEZU_IDLE_STATUS2_LIE_DOWN: lies down under status 2 for at most 9000
 * frames, 1 hp every 5 frames up to half its hp, then
 * KHEZU_IDLE_STATUS2_END.
 */
void EmKhezu_Status2LieDown(Em *em, EmWork *work)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KHEZU_MOT_LIE_DOWN, 0, 0);
        Em_LoadReactionSet(em, 1);
        em->timer = 0x2328;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KHEZU_MOT_LIE, 0, 0);
        }
        break;
    case 2:
        Em_RegenHp(em, 1, (s32) (0.5f * (f32) em->hpMax), 5);
        if (--em->timer <= 0) {
            em->step++;
            EmKhezu_ChangeState(em, EM_GROUP_IDLE, KHEZU_IDLE_STATUS2_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmKhezu_ChangeState(em, EM_GROUP_IDLE, KHEZU_IDLE_STATUS2_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * KHEZU_IDLE_SHAKE_OFF and the three other shake-offs (status 4, the second
 * hold, the stun; EmKhezu_SetIdleState does each one's clean-up): the
 * shake-off motion once, then EmKhezu_EndState.
 */
void EmKhezu_ShakeOff(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, KHEZU_MOT_SHAKE_OFF, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKhezu_EndState(em);
        }
        break;
    }
}

/*
 * KHEZU_IDLE_REST_END: gets up from the rest, still blind, then
 * KHEZU_IDLE_SHAKE_OFF.
 */
void EmKhezu_RestEnd(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, KHEZU_MOT_REST_END, 0, 0);
        Em_LoadReactionSet(em, 0);
        em->blind = 2;
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            EmKhezu_ChangeState(em, EM_GROUP_IDLE, KHEZU_IDLE_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmKhezu_ChangeState(em, EM_GROUP_IDLE, KHEZU_IDLE_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * KHEZU_IDLE_STATUS2_END and KHEZU_IDLE_STATUS1_END: gets up, still blind,
 * then KHEZU_IDLE_SHAKE_OFF.
 */
void EmKhezu_GetUpAfterStatus(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, KHEZU_MOT_GET_UP, 0, 0);
        Em_LoadReactionSet(em, 0);
        em->blind = 2;
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            EmKhezu_ChangeState(em, EM_GROUP_IDLE, KHEZU_IDLE_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmKhezu_ChangeState(em, EM_GROUP_IDLE, KHEZU_IDLE_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * KHEZU_IDLE_EAT_NEED1: lowers the head, then eats: 5% of its hp back, need
 * 1 filled to its maximum and its state cleared; then EmKhezu_EndState.
 */
void EmKhezu_EatNeed1(Em *em, EmWork *work)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_NO_FLINCH);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KHEZU_MOT_EAT_START, 0, 0);
        Em_SelectSightRow(em, 1);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KHEZU_MOT_EAT, 0, 0);
            Em_AddNeed1(em, em->need1Max);
            Em_ClearNeed1State(em);
            Em_AddHp(em, (s32) (0.05f * (f32) em->hpMax));
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SelectSightRow(em, 0);
            Em_LoadReactionSet(em, 0);
            EmKhezu_EndState(em);
        }
        break;
    }
}

/*
 * KHEZU_IDLE_STATUS1_COLLAPSE: falls over and lies until the status-1 timer
 * (Em_SetStatus1Timer) runs out, then KHEZU_IDLE_STATUS1_END.
 */
void EmKhezu_Status1Collapse(Em *em, EmWork *work)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KHEZU_MOT_COLLAPSE, 0, 0);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KHEZU_MOT_LIE, 0, 0);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            EmKhezu_ChangeState(em, EM_GROUP_IDLE, KHEZU_IDLE_STATUS1_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmKhezu_ChangeState(em, EM_GROUP_IDLE, KHEZU_IDLE_STATUS1_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * KHEZU_IDLE_STATUS2_DOWNED: lies in a hold under status 2, blind, with a
 * periodic effect at bone 0x22 and a sound every 135 frames; when the timer
 * runs out, KHEZU_REACT_STATUS2_DOWNED_END.
 */
void EmKhezu_Status2Downed(Em *em, EmWork *work)
{
    f32 pos[3];

    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->holdTimer = 5;
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KHEZU_MOT_DOWNED, 0, 0);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmKhezu_ChangeState(em, EM_GROUP_REACT, KHEZU_REACT_STATUS2_DOWNED_END, EM_SYNC_OWNER);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmKhezu_ChangeState(em, EM_GROUP_REACT, KHEZU_REACT_STATUS2_DOWNED_END, EM_SYNC_OWNER);
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
 * KHEZU_IDLE_STATUS1_DOWNED: as EmKhezu_Status2Downed under status 1; then
 * KHEZU_REACT_STATUS1_DOWNED_END.
 */
void EmKhezu_Status1Downed(Em *em, EmWork *work)
{
    f32 pos[3];

    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->holdTimer = 5;
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KHEZU_MOT_DOWNED, 0, 0);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmKhezu_ChangeState(em, EM_GROUP_REACT, KHEZU_REACT_STATUS1_DOWNED_END, EM_SYNC_OWNER);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmKhezu_ChangeState(em, EM_GROUP_REACT, KHEZU_REACT_STATUS1_DOWNED_END, EM_SYNC_OWNER);
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
 * KHEZU_MOVE_WALK: walks with Em_StepToGoal, to the goal or (without one)
 * for Em.timer frames, then EmKhezu_EndState.
 */
void EmKhezu_Walk(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KHEZU_MOT_WALK, 0, 0);
        break;
    case 1:
        if (Em_StepToGoal(em, 0x40, 1) == 1) {
            em->timer = 1;
        }
        if (--em->timer <= 0) {
            em->step++;
            EmKhezu_EndState(em);
        }
        break;
    }
}

/*
 * KHEZU_MOVE_TURN: turns to the target with the turn row 0, then
 * EmKhezu_EndState.
 */
void EmKhezu_Turn(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_StartTurnToTarget(em, &EmKhezu_TurnParams[0]);
        break;
    case 1:
        if (Em_TurnStep(em, &EmKhezu_TurnParams[0]) == 1) {
            em->step++;
            Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
            EmKhezu_EndState(em);
        }
        break;
    }
}

/*
 * KHEZU_MOVE_TURN_ALERT: turns to the target with the turn row 1, then
 * EmKhezu_EndState.
 */
void EmKhezu_TurnAlert(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_StartTurnToTarget(em, &EmKhezu_TurnParams[1]);
        break;
    case 1:
        if (Em_TurnStep(em, &EmKhezu_TurnParams[1]) == 1) {
            em->step++;
            Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
            EmKhezu_EndState(em);
        }
        break;
    }
}

/* KHEZU_MOVE_LIMP: as EmKhezu_Walk with the limping motion. */
void EmKhezu_Limp(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KHEZU_MOT_LIMP, 0, 0);
        break;
    case 1:
        if (Em_StepToGoal(em, 0x40, 1) == 1) {
            em->timer = 1;
        }
        if (--em->timer <= 0) {
            em->step++;
            EmKhezu_EndState(em);
        }
        break;
    }
}

/*
 * KHEZU_MOVE_TURN_TO_GOAL: turns in place, rotY stepping 0x200 a frame
 * toward the goal's heading until frame 60, then waits for the motion's end
 * and EmKhezu_EndState.
 */
void EmKhezu_TurnToGoal(Em *em, EmWork *work)
{
    u16 goalYaw;
    u32 diff;
    s32 rotY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KHEZU_MOT_TURN_IN_PLACE, 0, 0);
        work->goalYaw = Math_AngleXZ(&em->posX, em->goal);
        break;
    case 1:
        rotY = em->rotY;
        goalYaw = work->goalYaw;
        diff = (goalYaw - (rotY & 0xFFFF)) & 0xFFFF;
        if ((u32) ((diff + 0x200) & 0xFFFF) < 0x400U) {
            em->rotY = (s32) goalYaw;
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
            em->step++;
            EmKhezu_EndState(em);
        }
        break;
    }
}

/*
 * KHEZU_FLY_TAKE_OFF: takes off; airborne from frame 38 with the curves 2,
 * and when they end, EmKhezu_EndFlyState.
 */
void EmKhezu_TakeOff(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KHEZU_MOT_TAKE_OFF, 0, 0);
        EmKhezu_StartCurves(em, 2);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 38.0f, 0U) != 0) {
            em->step++;
            em->stance = EM_STANCE_AIR;
            EmKhezu_UpdateCurves(em);
        }
        break;
    case 2:
        if (EmKhezu_UpdateCurves(em)) {
            em->step++;
            EmKhezu_EndFlyState(em);
        }
        break;
    }
}

/*
 * KHEZU_FLY_LAND: comes down from the air: a descent speed that slows to 10
 * at 1000 above the floor, the hover motion until 630 above it, then the
 * last stretch in 30 frames and the touch-down (ground stance, camera
 * shake), then EmKhezu_EndState.
 */
void EmKhezu_Land(Em *em, EmWork *work)
{
    f32 height;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        em->rotX = 0;
        em->rotZ = 0;
        Em_SetMotion(em, KHEZU_MOT_HOVER, 0, 0);
        Player_ClearVelocity(em);
        em->velY = -10.0f;
        Em_ClearTilt(em, 1);
        height = em->floorY;
        height = em->posY - (1000.0f + height);
        if (height > 0.0f) {
            do {
                em->velY -= 0.75f;
                height += em->velY;
            } while (height > 0.0f && !(em->velY < -200.0f));
        }
        break;
    case 1:
        Em_MoveRotated(em, &em->rotX);
        if (em->velY < -10.0f) {
            em->velY += 0.75f;
        }
        if (!((em->floorY + 630.0f * em->scaleY) <= em->posY) && ((EmMotion_PassedFrameScaled(em, 10.0f, 0U) != 0) || (EmMotion_PassedFrameScaled(em, 86.0f, 0U) != 0) || (EmMotion_PassedFrameScaled(em, 160.0f, 0U) != 0))) {
            em->step++;
            Em_SetMotion(em, KHEZU_MOT_DESCEND, 0, 0);
            em->velY = (em->floorY - em->posY) / 30.0f;
            if (!(em->velY < 0.0f)) {
                em->velY = -10.0f;
            }
            em->timer = 0x1E;
        }
        break;
    case 2:
        Em_MoveRotated(em, &em->rotX);
        if (--em->timer < 0) {
            em->step++;
            Em_SetMotion(em, KHEZU_MOT_TOUCH_DOWN, 0, 0);
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
            EmKhezu_EndState(em);
        }
        break;
    }
    height = em->floorY;
    if (em->posY < height) {
        em->posY = height;
    }
}

/*
 * KHEZU_FLY_HOVER: hovers, bobbing with the height curve 1 and turning to
 * the goal; when Em.timer runs out it asks to land; EmKhezu_EndFlyState
 * when the timer reaches 300.
 */
void EmKhezu_Hover(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        work->moveAngX = 0x100;
        Em_SetMotion(em, KHEZU_MOT_HOVER, 0, 0);
        break;
    case 1:
        EmKhezu_MoveByVelYCurve(em, 1U);
        if (--em->timer <= 0) {
            em->step++;
            EmKhezu_ChangeState(em, EM_GROUP_FLY, KHEZU_FLY_LAND, EM_SYNC_SEND);
        }
        Em_TurnToGoal(em, (s32) (u16) work->moveAngX, 1);
        if (em->timer == 0x12C) {
            EmKhezu_EndFlyState(em);
        }
        break;
    case 2:
        EmKhezu_MoveByVelYCurve(em, 1U);
        Em_TurnToGoal(em, (s32) (u16) work->moveAngX, 1);
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * KHEZU_FLY_HOP_TURN: a hop straight up with the curves 2, turning to the
 * goal between frames 50 and 114 of the take-off, and down again where it
 * took off; then EmKhezu_EndState.
 */
void EmKhezu_HopTurn(Em *em, EmWork *work)
{
    f32 velY;
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KHEZU_MOT_TAKE_OFF, 0, 0);
        Player_ClearVelocity(em);
        EmKhezu_StartCurves(em, 2);
        break;
    case 1:
        if ((EmMotion_ReachedFrame(em, 0, 50.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 114.0f) == 0)) {
            Em_TurnToGoal(em, (s32) ((EmKhezuWork *)work)->turnSpeed, 1);
        }
        if (EmMotion_ReachedFrame(em, 0, 30.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            EmKhezu_UpdateCurves(em);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
        }
        break;
    case 2:
        if ((EmKhezu_UpdateCurves(em) & 0xFF) && (em->posY <= (em->floorY + (800.0f * em->scaleY)))) {
            em->step++;
            Em_SetMotion(em, KHEZU_MOT_DESCEND, 0, 0);
            Player_ClearVelocity(em);
            velY = (em->floorY - em->posY) / 30.0f;
            em->velY = velY;
            if (!(velY < 0.0f)) {
                em->velY = -10.0f;
            }
            Em_ClearTilt(em, 1);
        }
        break;
    case 3:
        Em_MoveRotated(em, &em->rotX);
        if (em->posY <= em->floorY) {
            em->step++;
            Em_SetMotion(em, KHEZU_MOT_TOUCH_DOWN, 0, 0);
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            CamShake_ReqAIf(em, 2);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKhezu_EndState(em);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * KHEZU_FLY_HOP_FORWARD: a hop forward (the curves 3, velZ 20 from frame
 * 30), landing with a camera shake, then EmKhezu_EndState.
 */
void EmKhezu_HopForward(Em *em, EmWork *work)
{
    f32 floorY;
    s32 ended;

    ended = 0;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KHEZU_MOT_TAKE_OFF, 0, 0);
        EmKhezu_StartCurves(em, 3);
        em->velZ = 20.0f;
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 30.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            ended = EmKhezu_UpdateCurves(em) & 0xFF;
        }
        if ((ended != 0) && (em->posY <= em->floorY)) {
            Em_SetMotion(em, KHEZU_MOT_TOUCH_DOWN, 0, 0);
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            CamShake_ReqAIf(em, 2);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKhezu_EndState(em);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/* KHEZU_FLY_HOP_BACK: as EmKhezu_HopForward, backward (velZ -20). */
void EmKhezu_HopBack(Em *em, EmWork *work)
{
    f32 floorY;
    s32 ended;

    ended = 0;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KHEZU_MOT_TAKE_OFF, 0, 0);
        EmKhezu_StartCurves(em, 3);
        em->velZ = -20.0f;
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 30.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            ended = EmKhezu_UpdateCurves(em) & 0xFF;
        }
        if ((ended != 0) && (em->posY <= em->floorY)) {
            Em_SetMotion(em, KHEZU_MOT_TOUCH_DOWN, 0, 0);
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            CamShake_ReqAIf(em, 2);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKhezu_EndState(em);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * KHEZU_FLY_TO_GOAL: hovers toward the goal at its height, turning to it,
 * until it is within ten frames' travel (XZ) or Em.timer runs out; then
 * EmKhezu_EndFlyState.
 */
void EmKhezu_FlyToGoal(Em *em, EmWork *work)
{
    f32 dist;
    f32 floorY;
    s32 timer;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        work->moveAngX = 0x100;
        Em_SetMotion(em, KHEZU_MOT_HOVER, 0, 0);
        Em_ClearTilt(em, 1);
        break;
    case 1:
        Em_TurnToGoal(em, (s32) (u16) work->moveAngX, 1);
        Em_MoveRotated(em, &em->rotX);
        dist = Vec_DistanceXZ(&em->posX, em->goal);
        timer = em->timer - 1;
        em->timer = timer;
        if ((timer <= 0) || (dist <= (10.0f * em->velZ))) {
            em->step++;
            Player_ClearVelocity(em);
            EmKhezu_EndFlyState(em);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * KHEZU_FLY_TO_GOAL_PITCHED: as EmKhezu_FlyToGoal, pitching toward the goal
 * and bobbing with the height curve 1.
 */
void EmKhezu_FlyToGoalPitched(Em *em, EmWork *work)
{
    f32 dist;
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        work->moveAngX = 0x100;
        Em_SetMotion(em, KHEZU_MOT_HOVER, 0, 0);
        break;
    case 1:
        Em_TurnToGoal(em, (s32) (u16) work->moveAngX, 1);
        work->moveAngBZ = em->rotY;
        work->moveAngCX = 0;
        Em_GetPitchToGoal(em, &work->moveAngBY, 0.0f, 0.0f);
        Em_MoveRotated(em, &work->moveAngBY);
        EmKhezu_MoveByVelYCurve(em, 1U);
        dist = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer <= 0 || dist <= (10.0f * em->velZ)) {
            em->step++;
            Player_ClearVelocity(em);
            EmKhezu_EndFlyState(em);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * KHEZU_FLY_OUT_OF_AREA: flies to a route point and the area's exit at
 * speed 40 until it is out of the area, then Em_EnterNextArea;
 * KHEZU_FLY_TO_GOAL_AND_LAND when no further area is wanted,
 * KHEZU_FLY_INTO_AREA otherwise.
 */
void EmKhezu_FlyOutOfArea(Em *em, EmWork *work)
{
    u16 next;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Player_ClearVelocity(em);
        em->velZ = 40.0f;
        Em_SetMotion(em, KHEZU_MOT_HOVER, 0, 0);
        Em_PickRoutePoint(em);
        Em_AreaExitPoint(em, (f32 *) em->goal);
        break;
    case 1:
        Em_TurnToGoal(em, ((EmKhezuWork *)work)->turnSpeed, 0);
        work->moveAngBY = em->rotX;
        work->moveAngBZ = em->rotY;
        work->moveAngCX = 0;
        Em_MoveRotated(em, &work->moveAngBY);
        if (Area_ContainsPos(em->area, &em->posX) == 0) {
            em->step++;
        }
        break;
    case 2:
        em->step++;
        break;
    case 3:
        em->step++;
        em->timer = 600;
        Em_EnterNextArea(em);
        next = *(u16 *)&em->destArea;
        if (em->nextArea == next || next == AREA_NONE) {
            Hud_TriggerGlow(em);
            EmKhezu_ChangeState(em, EM_GROUP_FLY, KHEZU_FLY_TO_GOAL_AND_LAND, EM_SYNC_SEND);
        } else {
            if (em->remote == 0) {
                em->nextArea = next;
                em->goalNo = next;
                em->goalKind = 3;
                Em_ComputeGoal(em, em->goal);
            }
            EmKhezu_ChangeState(em, EM_GROUP_FLY, KHEZU_FLY_INTO_AREA, EM_SYNC_SEND);
            Hud_TriggerGlow(em);
        }
        break;
    case 4:
        if (em->remote == 0) {
            if (--em->timer <= 0) {
                em->step = 3;
            }
        }
        break;
    }
    if (em->posY < em->floorY) {
        em->posY = em->floorY;
    }
}

/*
 * KHEZU_FLY_TO_GOAL_AND_LAND: faces the goal and flies at speed 40 until it
 * is within 500 (XZ) or Em.timer runs out, then asks to land (again after
 * 600 frames online).
 */
void EmKhezu_FlyToGoalAndLand(Em *em, EmWork *work)
{
    f32 dist;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        work->moveAngX = 0x100;
        Em_SetMotion(em, KHEZU_MOT_HOVER, 0, 0);
        Player_ClearVelocity(em);
        em->velZ = 40.0f;
        work->goalYaw = Math_AngleXZ(&em->posX, em->goal);
        em->rotY = work->goalYaw;
        *(u16 *) &em->destArea = AREA_NONE;
        break;
    case 1:
        Em_TurnToGoal(em, (s32) (u16) work->moveAngX, 0);
        work->moveAngBY = em->rotX;
        work->moveAngBZ = em->rotY;
        work->moveAngCX = 0;
        Em_MoveRotated(em, &work->moveAngBY);
        dist = Vec_DistanceXZ(&em->posX, em->goal);
        em->timer--;
        if (dist <= 500.0f || em->timer < 0) {
            em->step++;
            em->timer = 600;
            EmKhezu_ChangeState(em, EM_GROUP_FLY, KHEZU_FLY_LAND, EM_SYNC_SEND);
        }
        break;
    case 2:
        if (em->remote == 0) {
            if (--em->timer <= 0) {
                em->timer = 600;
                EmKhezu_ChangeState(em, EM_GROUP_FLY, KHEZU_FLY_TO_GOAL_AND_LAND, EM_SYNC_SEND);
            }
        }
        break;
    }
}

/*
 * KHEZU_FLY_TO_GOAL_AND_LAND_B: as EmKhezu_FlyToGoalAndLand without facing
 * the goal first; asks to land within 1000 (XZ), again every 300 frames.
 */
void EmKhezu_FlyToGoalAndLandB(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KHEZU_MOT_HOVER, 0, 0);
        if (!(em->velZ <= 40.0f)) {
            em->velZ = 40.0f;
        }
        break;
    case 1:
        if (Vec_DistanceXZ(&em->posX, em->goal) <= 1000.0f) {
            if (!(em->velZ <= 40.0f)) {
                em->velZ = 40.0f;
            }
            em->step++;
            em->timer = 0x12C;
            EmKhezu_ChangeState(em, EM_GROUP_FLY, KHEZU_FLY_LAND, EM_SYNC_SEND);
        } else {
            Em_TurnToGoal(em, (s32) ((EmKhezuWork *)work)->turnSpeed, 0);
            work->moveAngBY = em->rotX;
            work->moveAngBZ = em->rotY;
            work->moveAngCX = 0;
            Em_MoveRotated(em, &work->moveAngBY);
            if (--em->timer <= 0) {
                em->step++;
                EmKhezu_ChangeState(em, EM_GROUP_FLY, KHEZU_FLY_LAND, EM_SYNC_SEND);
            }
        }
        break;
    case 2:
        Em_TurnToGoal(em, (s32) ((EmKhezuWork *)work)->turnSpeed, 0);
        work->moveAngBY = em->rotX;
        work->moveAngBZ = em->rotY;
        work->moveAngCX = 0;
        Em_MoveRotated(em, &work->moveAngBY);
        if (--em->timer <= 0) {
            em->timer = 0x12C;
            if (em->remote == 0) {
                EmKhezu_ChangeState(em, EM_GROUP_FLY, KHEZU_FLY_LAND, EM_SYNC_SEND);
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
 * KHEZU_FLY_RISE_TO_AREA_HEIGHT: climbs straight up (faster above 500 over
 * the floor) until it reaches the area's flight height (gAreaGroundY), then
 * EmKhezu_EndFlyState when the motion ends.
 */
void EmKhezu_RiseToAreaHeight(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, KHEZU_MOT_DESCEND, 0, 0);
        Player_ClearVelocity(em);
        em->velY = 10.0f;
        break;
    case 1:
        Em_MoveRotated(em, &em->rotX);
        if (!(em->posY <= (500.0f + em->floorY))) {
            if (em->velY < 100.0f) {
                em->velY += 0.75f;
            }
        }
        if (!(em->posY < gAreaGroundY[em->area])) {
            em->step++;
            Player_ClearVelocity(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKhezu_EndFlyState(em);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * KHEZU_FLY_INTO_AREA: flies to the middle of the new area, then on to a
 * route point and the exit as EmKhezu_FlyOutOfArea does.
 */
void EmKhezu_FlyIntoArea(Em *em, EmWork *work)
{
    AreaRecord *area;
    f32 dist;
    u16 next;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        work->moveAngX = 0x100;
        Em_SetMotion(em, KHEZU_MOT_HOVER, 0, 0);
        Player_ClearVelocity(em);
        em->velZ = 40.0f;
        area = Stage_GetAreaRecord(em->area);
        em->goal[0] = area->sizeX / 2.0f;
        em->goal[1] = area->bottom;
        em->goal[2] = area->sizeZ / 2.0f;
        work->goalYaw = Math_AngleXZ(&em->posX, em->goal);
        em->rotY = work->goalYaw;
        break;
    case 1:
        Em_TurnToGoal(em, ((EmKhezuWork *)work)->turnSpeed, 0);
        work->moveAngBY = em->rotX;
        work->moveAngBZ = em->rotY;
        work->moveAngCX = 0;
        Em_MoveRotated(em, &work->moveAngBY);
        dist = Vec_DistanceXZ(&em->posX, em->goal);
        em->timer--;
        if (dist <= 500.0f || em->timer < 0) {
            em->step++;
            Em_PickRoutePoint(em);
            Em_AreaExitPoint(em, (f32 *) em->goal);
        }
        break;
    case 2:
        Em_TurnToGoal(em, ((EmKhezuWork *)work)->turnSpeed, 0);
        work->moveAngBY = em->rotX;
        work->moveAngBZ = em->rotY;
        work->moveAngCX = 0;
        Em_MoveRotated(em, &work->moveAngBY);
        if (Area_ContainsPos(em->area, &em->posX) == 0) {
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
            EmKhezu_ChangeState(em, EM_GROUP_FLY, KHEZU_FLY_TO_GOAL_AND_LAND, EM_SYNC_SEND);
        } else {
            if (em->remote == 0) {
                em->nextArea = next;
                em->goalNo = next;
                em->goalKind = 3;
                Em_ComputeGoal(em, em->goal);
            }
            EmKhezu_ChangeState(em, EM_GROUP_FLY, KHEZU_FLY_INTO_AREA, EM_SYNC_SEND);
            Hud_TriggerGlow(em);
        }
        break;
    case 4:
        if (em->remote == 0) {
            if (--em->timer <= 0) {
                em->step = 3;
            }
        }
        break;
    }
    if (em->posY < em->floorY) {
        em->posY = em->floorY;
    }
}

/*
 * KHEZU_FLY_HOVER_ONCE: the hover motion once, bobbing; then
 * EmKhezu_EndFlyState, which comes back here: the flight's loop in which
 * the script picks the next action.
 */
void EmKhezu_HoverOnce(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, KHEZU_MOT_HOVER, 0, 0);
        break;
    case 1:
        EmKhezu_MoveByVelYCurve(em, 1U);
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKhezu_EndFlyState(em);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * KHEZU_FLY_RISE_ABOVE_GOAL: rises at 20 a frame until Em.timer runs out or
 * it is 3000 above the goal, then EmKhezu_EndFlyState. The heading it moves
 * on comes from a position never set (the original's code).
 */
void EmKhezu_RiseAboveGoal(Em *em, EmWork *work)
{
    f32 pos[3];
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, KHEZU_MOT_HOVER, 0, 0);
        work->moveAngBY = 0;
        work->moveAngCX = 0;
        Player_ClearVelocity(em);
        em->velY = 20.0f;
        break;
    case 1:
        work->moveAngBZ = Math_AngleXZ(&em->posX, pos) & 0xFFFF;
        Em_MoveRotated(em, &work->moveAngBY);
        if (--em->timer <= 0 || !(em->posY <= (3000.0f + em->goal[1]))) {
            em->step++;
            Player_ClearVelocity(em);
            EmKhezu_EndFlyState(em);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * KHEZU_FLY_HOVER_TURN: hovers, bobbing, while it turns to the goal; when
 * the turn is done or Em.timer runs out, EmKhezu_EndFlyState.
 */
void EmKhezu_HoverTurn(Em *em, EmWork *work)
{
    f32 floorY;
    s32 done;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, KHEZU_MOT_HOVER, 0, 0);
        work->moveAngX = 0x100;
        work->goalYaw = Math_AngleXZ(&em->posX, em->goal);
        work->goalYaw = work->goalYaw - em->rotY;
        em->timer = 0x12C;
        break;
    case 1:
        EmKhezu_MoveByVelYCurve(em, 1U);
        done = Em_TurnToGoal(em, (s32) (u16) work->moveAngX, 1) & 0xFF;
        if (--em->timer <= 0 || done != 0) {
            em->step++;
            EmKhezu_EndFlyState(em);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/* KHEZU_FLY_EMPTY_A: does nothing (as KHEZU_FLY_EMPTY_B, _C and _D). */
void EmKhezu_FlyEmptyA(void)
{
}

void EmKhezu_FlyEmptyB(void)
{
}

void EmKhezu_FlyEmptyC(void)
{
}

/*
 * KHEZU_FLY_HOP_SLAM: a hop (airborne from frame 44, the curves 12),
 * landing with a camera shake and the landing impact, then
 * EmKhezu_EndState.
 */
void EmKhezu_HopSlam(Em *em, EmWork *work)
{
    f32 floorY;
    s32 ended;

    ended = 0;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KHEZU_MOT_HOP_SLAM, 0, 0);
        EmKhezu_StartCurves(em, 0xC);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 44.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            ended = EmKhezu_UpdateCurves(em) & 0xFF;
        }
        if ((ended != 0) && (em->posY <= em->floorY)) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            EmKhezu_SpawnLandingImpact(em);
            CamShake_ReqAIf(em, 2);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKhezu_EndState(em);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * KHEZU_FLY_TO_GOAL_DIRECT: faces the goal and pitches toward it, flying
 * straight at it until within ten frames' travel or Em.timer runs out; then
 * EmKhezu_EndFlyState.
 */
void EmKhezu_FlyToGoalDirect(Em *em, EmWork *work)
{
    f32 dist;
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        work->moveAngX = 0x100;
        Em_SetMotion(em, KHEZU_MOT_HOVER, 0, 0);
        break;
    case 1:
        work->moveAngBZ = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        work->moveAngCX = 0;
        Em_GetPitchToGoal(em, &work->moveAngBY, 0.0f, 0.0f);
        Em_MoveRotated(em, &work->moveAngBY);
        dist = Vec_Distance((f32 *) &em->posX, (f32 *) em->goal);
        if (--em->timer <= 0 || dist <= (10.0f * em->velZ)) {
            em->step++;
            Player_ClearVelocity(em);
            EmKhezu_EndFlyState(em);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmKhezu_FlyEmptyD(void)
{
}

/*
 * KHEZU_FLY_ESCAPE_TRAP: after the trap's struggle, takes off out of the
 * trap (the hold cleared, Em.noReact set) and comes down again as
 * EmKhezu_HopTurn does; then EmKhezu_EndState.
 */
void EmKhezu_EscapeTrap(Em *em, EmWork *work)
{
    f32 floorY;
    f32 velY;
    f32 y;

    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, KHEZU_MOT_TAKE_OFF, 0xA, 0x1E);
        EmKhezu_StartCurves(em, 2);
        em->stance = EM_STANCE_AIR;
        em->holdTimer = 0;
        em->noReact = 1;
        break;
    case 1:
        EmKhezu_UpdateCurves(em);
        if (em->motion[0].playing == 0) {
            em->step++;
        }
        break;
    case 2:
        if ((EmKhezu_UpdateCurves(em) & 0xFF) && (em->posY <= (em->floorY + (800.0f * em->scaleY)))) {
            em->step++;
            Em_SetMotion(em, KHEZU_MOT_DESCEND, 0, 0);
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
            Em_ClearTilt(em, 1);
        }
        break;
    case 3:
        Em_MoveRotated(em, &em->rotX);
        if (em->posY <= em->floorY) {
            em->step++;
            Em_SetMotion(em, KHEZU_MOT_TOUCH_DOWN, 0, 0);
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            CamShake_ReqAIf(em, 2);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKhezu_EndState(em);
        }
        break;
    }
    y = em->floorY;
    if (em->posY < y) {
        em->posY = y;
    }
}

/*
 * KHEZU_FLY_CEIL_LEAP_UP: leaps from the floor at frame 112 of its motion
 * and rises until it is 580 (scaled) under the ceiling, then takes hold
 * (KHEZU_MOT_CEIL_LATCH) and EmKhezu_EndCeilingState.
 */
void EmKhezu_LeapToCeiling(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, KHEZU_MOT_CEIL_LEAP, 0, 0);
        em->stance = EM_STANCE_GROUND;
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 112.0f, 0U) != 0) {
            em->step++;
            em->stance = EM_STANCE_AIR;
            Player_ClearVelocity(em);
            em->velY = 100.0f;
            em->accelY = -5.0f;
            Em_ClearTilt(em, 1);
        }
        break;
    case 2:
        Em_MoveRotatedAccel(em, &em->rotX);
        if (em->velY < 50.0f) {
            em->velY = 50.0f;
            em->accelY = 0.0f;
        }
        if (!(em->posY <= (em->surfaceY - (580.0f * em->scaleX)))) {
            em->step++;
            em->posY = em->surfaceY;
            Em_SetMotion(em, KHEZU_MOT_CEIL_LATCH, 0, 0);
            em->ceilingTimer = 5;
        }
        break;
    case 3:
        em->ceilingTimer = 5;
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKhezu_EndCeilingState(em);
        }
        break;
    }
}

/*
 * KHEZU_FLY_CEIL_HANG: hangs at the ceiling's height; each time the hang
 * motion ends, EmKhezu_EndCeilingState, which comes back here: the
 * ceiling's loop in which the script picks the next action.
 */
void EmKhezu_HangFromCeiling(Em *em, EmWork *work)
{
    em->ceilingTimer = 5;
    em->posY = em->surfaceY;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        if (em->motionId[0] != EM_MOTION_ID(0, KHEZU_MOT_CEIL_HANG)) {
            Em_SetMotion(em, KHEZU_MOT_CEIL_HANG, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKhezu_EndCeilingState(em);
        }
        break;
    }
}

/*
 * KHEZU_FLY_CEIL_DROP: lets go of the ceiling (moved down 580, scaled, at
 * frame 12, Em.ceilingTimer cleared), falls to the floor and lands; then
 * EmKhezu_EndState.
 */
void EmKhezu_DropFromCeiling(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, KHEZU_MOT_CEIL_DROP, 0, 0);
        Player_ClearVelocity(em);
        em->velY = -50.0f;
        em->accelY = -5.0f;
        Em_ClearTilt(em, 1);
        em->ceilingTimer = 5;
        break;
    case 1:
        em->ceilingTimer = 5;
        if (EmMotion_PassedFrameScaled(em, 12.0f, 0U) != 0) {
            em->step++;
            em->posY -= 580.0f * em->scaleX;
            em->ceilingTimer = 0;
        }
        break;
    case 2:
        Em_MoveRotatedAccel(em, &em->rotX);
        floorY = em->floorY;
        if (em->posY <= floorY) {
            em->posY = floorY;
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, KHEZU_MOT_CEIL_DROP_LAND, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKhezu_EndState(em);
        }
        break;
    }
}

/*
 * KHEZU_FLY_CEIL_TURN: turns on the ceiling with the turn row 2, then hangs
 * again.
 */
void EmKhezu_TurnOnCeiling(Em *em, EmWork *work)
{
    em->ceilingTimer = 5;
    em->posY = em->surfaceY;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_StartTurnToTarget(em, &EmKhezu_TurnParams[2]);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 8.0f) != 0) {
            em->step++;
        case 2:
            if (Em_TurnStep(em, &EmKhezu_TurnParams[2]) == 1) {
                em->step++;
                Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
                EmKhezu_EndCeilingState(em);
            }
        }
        break;
    }
}

/*
 * KHEZU_FLY_CEIL_CRAWL: crawls on the ceiling to the goal with
 * Em_StepToGoal, then hangs again.
 */
void EmKhezu_CrawlOnCeiling(Em *em, EmWork *work)
{
    em->ceilingTimer = 5;
    em->posY = em->surfaceY;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KHEZU_MOT_CEIL_CRAWL, 0, 0);
        break;
    case 1:
        if (Em_StepToGoal(em, 0x40, 0) == 1) {
            em->step++;
            EmKhezu_EndCeilingState(em);
        }
        break;
    }
}

/*
 * KHEZU_FLY_CEIL_CIRCUIT: crawls to the next of the area's four ceiling
 * points (reached within 300, or after Em.timer frames, counted while it is
 * farther); while EmKhezuWork.legsLeft counts down it goes on to the
 * following point, then hangs again.
 */
void EmKhezu_CrawlCircuit(Em *em, EmWork *work)
{
    s8 n;

    em->ceilingTimer = 5;
    em->posY = em->surfaceY;
    switch (em->step) {
    case 0:
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, KHEZU_MOT_CEIL_CRAWL, 0, 0);
        em->step++;
        em->stateVar1 = 0;
        em->lookAtTarget = 0;
        em->timer = 0x12C;
        break;
    case 1:
        if (Vec_DistanceXZ(&em->posX, em->goal) <= 300.0f || --em->timer <= 0) {
            n = ((EmKhezuWork *)work)->legsLeft - 1;
            ((EmKhezuWork *)work)->legsLeft = n;
            if (n <= 0) {
                em->step++;
                EmKhezu_EndCeilingState(em);
            } else {
                em->curGoalNo++;
                em->curGoalNo &= 3;
                Em_ComputeGoal2(em, em->goal);
                EmKhezu_ChangeState(em, EM_GROUP_FLY, KHEZU_FLY_CEIL_CIRCUIT, EM_SYNC_LOCAL);
            }
        }
        Em_TurnToGoal(em, 0x200, 0);
        break;
    }
}

/*
 * KHEZU_FLY_CEIL_CIRCUIT_REVERSE: as EmKhezu_CrawlCircuit, round the points
 * the other way.
 */
void EmKhezu_CrawlCircuitReverse(Em *em, EmWork *work)
{
    s8 n;

    em->ceilingTimer = 5;
    em->posY = em->surfaceY;
    switch (em->step) {
    case 0:
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, KHEZU_MOT_CEIL_CRAWL, 0, 0);
        em->step++;
        em->stateVar1 = 0;
        em->lookAtTarget = 0;
        em->timer = 0x12C;
        break;
    case 1:
        if (Vec_DistanceXZ(&em->posX, em->goal) <= 300.0f || --em->timer <= 0) {
            n = ((EmKhezuWork *)work)->legsLeft - 1;
            ((EmKhezuWork *)work)->legsLeft = n;
            if (n <= 0) {
                em->step++;
                EmKhezu_EndCeilingState(em);
            } else {
                em->curGoalNo -= 1;
                em->curGoalNo &= 3;
                Em_ComputeGoal2(em, em->goal);
                EmKhezu_ChangeState(em, EM_GROUP_FLY, KHEZU_FLY_CEIL_CIRCUIT_REVERSE, EM_SYNC_LOCAL);
            }
        }
        Em_TurnToGoal(em, 0x200, 0);
        break;
    }
}

/*
 * KHEZU_FLY_CEIL_TURN_TO_CIRCUIT: turns with the turn row 3 (root motion
 * off), then KHEZU_FLY_CEIL_CIRCUIT.
 */
void EmKhezu_TurnToCircuit(Em *em, EmWork *work)
{
    em->ceilingTimer = 5;
    em->posY = em->surfaceY;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Player_SetFlag((Player *) em, EM_FLAG_NO_ROOT_MOTION);
        Em_StartTurnToTarget(em, &EmKhezu_TurnParams[3]);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 8.0f) != 0) {
            em->step++;
        case 2:
            if (Em_TurnStep(em, &EmKhezu_TurnParams[3]) == 1) {
                em->step++;
                em->timer = 0x12C;
                Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
                EmKhezu_ChangeState(em, EM_GROUP_FLY, KHEZU_FLY_CEIL_CIRCUIT, EM_SYNC_SEND);
                break;
            }
        } else {
        }
        break;
    case 3:
        if (--em->timer <= 0) {
            em->timer = 0x12C;
            if (em->remote == 0) {
                EmKhezu_ChangeState(em, EM_GROUP_FLY, KHEZU_FLY_CEIL_CIRCUIT, EM_SYNC_SEND);
            }
        }
        break;
    }
}

/*
 * KHEZU_FLY_CEIL_CRAWL_TO_ZONE: crawls to the ceiling point of the target's
 * zone; there it takes the target as its goal and turns again
 * (KHEZU_FLY_CEIL_TURN_TO_ZONE).
 */
void EmKhezu_CrawlToTargetZone(Em *em, EmWork *work)
{
    em->ceilingTimer = 5;
    em->posY = em->surfaceY;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KHEZU_MOT_CEIL_CRAWL, 0, 0);
        break;
    case 1:
        if (Em_StepToGoal(em, 0x40, 0) == 1) {
            em->step++;
            em->goalKind = 1;
            em->goalArg = 0;
            em->goalNo = (s16) em->targetNo;
            em->curGoalKind = em->goalKind;
            em->curGoalArg = *(u16 *) &em->goalArg;
            em->curGoalNo = *(u16 *) &em->goalNo;
            Em_ComputeGoal(em, em->goal);
            em->timer = 0x12C;
            EmKhezu_ChangeState(em, EM_GROUP_FLY, KHEZU_FLY_CEIL_TURN_TO_ZONE, EM_SYNC_SEND);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->timer = 0x12C;
            if (em->remote == 0) {
                EmKhezu_ChangeState(em, EM_GROUP_FLY, KHEZU_FLY_CEIL_TURN_TO_ZONE, EM_SYNC_SEND);
            }
        }
        break;
    }
}

/*
 * KHEZU_FLY_CEIL_TURN_TO_ZONE: turns with the turn row 4, then
 * KHEZU_FLY_CEIL_CRAWL_TO_ZONE.
 */
void EmKhezu_TurnToTargetZone(Em *em, EmWork *work)
{
    em->ceilingTimer = 5;
    em->posY = em->surfaceY;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_StartTurnToTarget(em, &EmKhezu_TurnParams[4]);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 8.0f) != 0) {
            em->step++;
        case 2:
            if (Em_TurnStep(em, &EmKhezu_TurnParams[4]) == 1) {
                em->step++;
                Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
                em->timer = 0x12C;
                EmKhezu_ChangeState(em, EM_GROUP_FLY, KHEZU_FLY_CEIL_CRAWL_TO_ZONE, EM_SYNC_SEND);
                break;
            }
        } else {
        }
        break;
    case 3:
        if (--em->timer <= 0) {
            em->timer = 0x12C;
            if (em->remote == 0) {
                EmKhezu_ChangeState(em, EM_GROUP_FLY, KHEZU_FLY_CEIL_CRAWL_TO_ZONE, EM_SYNC_SEND);
            }
        }
        break;
    }
}

/*
 * KHEZU_FLY_CEIL_TURN_TO_CRAWL: turns to the goal with the turn row 5, then
 * KHEZU_FLY_CEIL_CRAWL.
 */
void EmKhezu_TurnToCrawl(Em *em, EmWork *work)
{
    em->ceilingTimer = 5;
    em->posY = em->surfaceY;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_StartTurnToTarget(em, &EmKhezu_TurnParams[5]);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 8.0f) != 0) {
            em->step++;
        case 2:
            if (Em_TurnStep(em, &EmKhezu_TurnParams[5]) == 1) {
                em->step++;
                Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
                em->timer = 0x12C;
                EmKhezu_ChangeState(em, EM_GROUP_FLY, KHEZU_FLY_CEIL_CRAWL, EM_SYNC_SEND);
                break;
            }
        } else {
        }
        break;
    case 3:
        if (--em->timer <= 0) {
            em->timer = 0x12C;
            if (em->remote == 0) {
                EmKhezu_ChangeState(em, EM_GROUP_FLY, KHEZU_FLY_CEIL_CRAWL, EM_SYNC_SEND);
            }
        }
        break;
    }
}

/*
 * KHEZU_ATK_LEAP: a leap at the target: on the floor for the first 78
 * frames, then airborne with the curves 0xE until they end and the floor is
 * reached; lands, then EmKhezu_EndState.
 */
void EmKhezu_Leap(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KHEZU_MOT_LEAP, 0, 0);
        EmKhezu_StartCurves(em, 0xE);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 78.0f, 0U) != 0) {
            em->step++;
            em->stance = EM_STANCE_AIR;
            EmKhezu_UpdateCurves(em);
        }
        break;
    case 2:
        if ((EmKhezu_UpdateCurves(em) & 0xFF) && (em->posY <= em->floorY)) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, KHEZU_MOT_LEAP_LAND, 0, 0);
        }
        break;
    case 3:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmKhezu_EndState(em);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * KHEZU_ATK_CEIL_SHOTS: on the ceiling, its motion fires a shot from the
 * head every 16 frames (EmKhezu_MotionEffects); at the end a mine point is
 * left on the floor below (EmMine_CreateTimed0), then
 * EmKhezu_EndCeilingState.
 */
void EmKhezu_CeilingShots(Em *em, EmWork *work)
{
    f32 pos[3];

    em->ceilingTimer = 5;
    em->posY = em->surfaceY;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KHEZU_MOT_CEIL_SHOTS, 0, 0);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            Vec_Copy(pos, &em->posX);
            pos[1] = em->floorY;
            EmMine_CreateTimed0(em, pos);
            em->step++;
            Em_SetMotion(em, KHEZU_MOT_CEIL_SHOTS_END, 0, 0);
        }
        break;
    case 2:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmKhezu_EndCeilingState(em);
        }
        break;
    }
}

/*
 * KHEZU_ATK_ACTION_A: its motion once (impact effects near its end), then
 * EmKhezu_EndState.
 */
void EmKhezu_AttackActionA(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KHEZU_MOT_ATK_ACTION_A, 0, 0);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmKhezu_EndState(em);
        }
        break;
    }
}

/* KHEZU_ATK_ACTION_B: its motion once, then EmKhezu_EndState. */
void EmKhezu_AttackActionB(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KHEZU_MOT_ATK_ACTION_B, 0, 0);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmKhezu_EndState(em);
        }
        break;
    }
}

/*
 * KHEZU_ATK_ACTION_C: its motion once (Red Khezu adds sparks), then
 * EmKhezu_EndState.
 */
void EmKhezu_AttackActionC(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KHEZU_MOT_ATK_ACTION_C, 0, 0);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmKhezu_EndState(em);
        }
        break;
    }
}

/*
 * KHEZU_ATK_SHOT_FAN: charges, then at frame 176 fires three shots from the
 * head: one straight ahead and two to the sides, wide (0xE39, about 20
 * degrees) when Em.stateRand & 0x30, narrow (0x71C) otherwise; then
 * KHEZU_MOT_ATK_RECOVER and EmKhezu_EndState.
 */
void EmKhezu_ShotFan(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KHEZU_MOT_SHOT_FAN, 0, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 176.0f, 0U) != 0) {
            EmRangedAttack_CreateAtBone(em, 0x22, 6, 0, 0, 0);
            if (em->stateRand & 0x30) {
                EmRangedAttack_CreateAtBone(em, 0x22, 6, 0, 0, 0xE39);
                EmRangedAttack_CreateAtBone(em, 0x22, 6, 0, 0, 0xF1C8);
            } else {
                EmRangedAttack_CreateAtBone(em, 0x22, 6, 0, 0, 0x71C);
                EmRangedAttack_CreateAtBone(em, 0x22, 6, 0, 0, 0xF8E5);
            }
        }
        if (EmMotion_PassedFrameScaled(em, 254.0f, 0U) != 0) {
            em->step++;
            Em_SetMotion(em, KHEZU_MOT_ATK_RECOVER, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKhezu_EndState(em);
        }
        break;
    }
}

/*
 * KHEZU_ATK_SPIN: the spin motion twice, rotY falling 0x200 a frame, then
 * EmKhezu_EndState.
 */
void EmKhezu_Spin(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KHEZU_MOT_SPIN, 0, 0);
        break;
    case 1:
        em->rotY -= 0x200;
        if (em->motion[1].playing == 0) {
            em->step++;
            Em_SetMotion(em, KHEZU_MOT_SPIN, 0, 0);
        }
        break;
    case 2:
        em->rotY -= 0x200;
        if (em->motion[1].playing == 0) {
            em->step++;
            EmKhezu_EndState(em);
        }
        break;
    }
}

/*
 * KHEZU_ATK_ACTION_D: its motion and KHEZU_MOT_ATK_RECOVER, then
 * EmKhezu_EndState.
 */
void EmKhezu_AttackActionD(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KHEZU_MOT_ATK_ACTION_D, 0, 0);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            Em_SetMotion(em, KHEZU_MOT_ATK_RECOVER, 0, 0);
        }
        break;
    case 2:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmKhezu_EndState(em);
        }
        break;
    }
}

/*
 * KHEZU_ATK_LEAP_QUICK: as EmKhezu_Leap, airborne from frame 30 (the curves
 * 0xF).
 */
void EmKhezu_LeapQuick(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KHEZU_MOT_LEAP_QUICK, 0, 0);
        EmKhezu_StartCurves(em, 0xF);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 30.0f, 0U) != 0) {
            em->step++;
            em->stance = EM_STANCE_AIR;
            EmKhezu_UpdateCurves(em);
        }
        break;
    case 2:
        if ((EmKhezu_UpdateCurves(em) & 0xFF) && (em->posY <= em->floorY)) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, KHEZU_MOT_LEAP_LAND, 0, 0);
        }
        break;
    case 3:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmKhezu_EndState(em);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * KHEZU_REACT_FLINCH: the script restarts; the flinch motion, then
 * EmKhezu_EndState.
 */
void EmKhezu_Flinch(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, KHEZU_MOT_FLINCH, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKhezu_EndState(em);
        }
        break;
    }
}

/*
 * KHEZU_REACT_CEIL_FALL: knocked off the ceiling: as
 * EmKhezu_DropFromCeiling, falling with Em_MoveRotatedAccel; then
 * EmKhezu_EndState.
 */
void EmKhezu_CeilingFall(Em *em)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, KHEZU_MOT_CEIL_DROP, 0, 0);
        Player_ClearVelocity(em);
        em->velY = -50.0f;
        em->accelY = -5.0f;
        Em_ClearTilt(em, 1);
        em->ceilingTimer = 5;
        Em_RestartScript(em);
        break;
    case 1:
        em->ceilingTimer = 5;
        if (EmMotion_PassedFrameScaled(em, 12.0f, 0U) != 0) {
            em->step++;
            em->posY -= 580.0f * em->scaleX;
            em->ceilingTimer = 0;
        }
        break;
    case 2:
        Em_MoveRotatedAccel(em, &em->rotX);
        floorY = em->floorY;
        if (em->posY <= floorY) {
            em->posY = floorY;
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, KHEZU_MOT_CEIL_DROP_LAND, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKhezu_EndState(em);
        }
        break;
    }
}

/* KHEZU_REACT_FLINCH_B: the second flinch motion, then EmKhezu_EndState. */
void EmKhezu_FlinchB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, KHEZU_MOT_FLINCH_B, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKhezu_EndState(em);
        }
        break;
    }
}

/* KHEZU_REACT_FLINCH_C: the third flinch motion, then EmKhezu_EndState. */
void EmKhezu_FlinchC(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, KHEZU_MOT_FLINCH_C, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKhezu_EndState(em);
        }
        break;
    }
}

/*
 * KHEZU_REACT_KNOCK_DOWN_PART3 (variant 0), KHEZU_REACT_KNOCK_DOWN (1),
 * KHEZU_REACT_STUN (2) and KHEZU_REACT_STUN_B (3): knocked down on one side
 * (the B motions for variants 0 and 2, the A motions for 1 and 3). A flinch
 * lies 120 frames and ends with EmKhezu_EndState; the stun makes the stun's
 * stage object, lies for Em.timer (Em_SetStatus5Timer) and ends in
 * KHEZU_IDLE_STUN_SHAKE_OFF.
 */
void EmKhezu_KnockDown(Em *em, EmWork *work, u8 variant)
{
    s32 k;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        k = variant;
        if ((k & 1) == 0) {
            Em_SetMotion(em, KHEZU_MOT_KNOCK_DOWN_B, 0, 0);
        } else {
            Em_SetMotion(em, KHEZU_MOT_KNOCK_DOWN_A, 0, 0);
        }
        switch (k) {
        case 2:
        case 3:
            StunStars_CreateEm(em, 1, 0x22, (f32) em->timer + em->motion[0].frameCount / 2.0f, 150.0f, 3.0f);
            break;
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            k = variant;
            em->step++;
            if ((k & 1) == 0) {
                Em_SetMotion(em, KHEZU_MOT_DOWN_B, 0, 0);
            } else {
                Em_SetMotion(em, KHEZU_MOT_DOWN_A, 0, 0);
            }
            switch (k) {
            case 2:
            case 3:
                break;
            case 0:
            case 1:
            default:
                em->timer = 120;
                break;
            }
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            if ((variant & 1) == 0) {
                Em_SetMotion(em, KHEZU_MOT_GET_UP_B, 0, 0);
            } else {
                Em_SetMotion(em, KHEZU_MOT_GET_UP_A, 0, 0);
            }
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
            switch (variant) {
            case 2:
            case 3:
                em->step++;
                EmKhezu_ChangeState(em, EM_GROUP_IDLE, KHEZU_IDLE_STUN_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
                break;
            default:
                EmKhezu_EndState(em);
                break;
            }
        }
        break;
    case 4:
        if (em->remote == 0) {
            EmKhezu_ChangeState(em, EM_GROUP_IDLE, KHEZU_IDLE_STUN_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * KHEZU_REACT_FALL: falls out of the air, lands, gets up, then
 * EmKhezu_EndState.
 */
void EmKhezu_Fall(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, KHEZU_MOT_FALL, 0, 0);
        if (!(em->velY <= -50.0f)) {
            em->velY = -50.0f;
        }
        em->accelX = 0.0f;
        em->accelY = -10.0f;
        em->accelZ = 0.0f;
        Em_ClearTilt(em, 1);
        break;
    case 1:
        Em_MoveRotatedAccel(em, &em->rotX);
        if (em->posY <= em->floorY) {
            em->step++;
            em->posY = em->floorY;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, KHEZU_MOT_FALL_LAND, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KHEZU_MOT_GET_UP_A, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKhezu_EndState(em);
            Player_ClearVelocity(em);
        }
        break;
    }
}

/*
 * KHEZU_REACT_TRAPPED_STRUGGLE and the downed ends of status 4 and the
 * stun: struggles in the hold until Em.trapTimer runs out, then clears
 * Em.trapHit and escapes (KHEZU_FLY_ESCAPE_TRAP).
 */
void EmKhezu_Struggle(Em *em)
{
    em->holdTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, KHEZU_MOT_STRUGGLE, 0, 0);
        em->noReact = 1;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            Em_SetMotion(em, KHEZU_MOT_STRUGGLE, 0, 0);
        }
        if (--em->trapTimer <= 0) {
            em->step++;
            em->trapHit = 0;
            Em_SetMotion(em, KHEZU_MOT_STRUGGLE, 0, 0);
            EmKhezu_ChangeState(em, EM_GROUP_FLY, KHEZU_FLY_ESCAPE_TRAP, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if ((em->motion[0].playing == 0) && (em->remote == 0)) {
            Em_SetMotion(em, KHEZU_MOT_STRUGGLE, 0, 0);
            EmKhezu_ChangeState(em, EM_GROUP_FLY, KHEZU_FLY_ESCAPE_TRAP, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * KHEZU_REACT_STATUS4: under status 4 for Em.timer frames with its effect,
 * then KHEZU_IDLE_STATUS4_SHAKE_OFF.
 */
void EmKhezu_Status4(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_STATUS);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, KHEZU_MOT_STATUS4, 0, 0);
        em->noReact = 1;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmKhezu_ChangeState(em, EM_GROUP_IDLE, KHEZU_IDLE_STATUS4_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmKhezu_ChangeState(em, EM_GROUP_IDLE, KHEZU_IDLE_STATUS4_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
    Em_SpawnStatus4Effect(em, 2);
}

/*
 * KHEZU_REACT_TRAPPED: caught in a trap (hold 50 frames, blind; hits don't
 * make it flinch); when the motion ends, KHEZU_REACT_TRAPPED_STRUGGLE.
 */
void EmKhezu_Trapped(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_NO_FLINCH);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, KHEZU_MOT_TRAPPED, 0, 0);
        em->noReact = 1;
        em->holdTimer = 0x32;
        em->blind = 2;
        break;
    case 1:
        if ((s8) em->holdTimer < 5) {
            em->holdTimer = 5;
        }
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            EmKhezu_ChangeState(em, EM_GROUP_REACT, KHEZU_REACT_TRAPPED_STRUGGLE, EM_SYNC_OWNER);
        }
        break;
    case 2:
        if ((s8) em->holdTimer < 5) {
            em->holdTimer = 5;
        }
        if (em->remote == 0) {
            EmKhezu_ChangeState(em, EM_GROUP_REACT, KHEZU_REACT_TRAPPED_STRUGGLE, EM_SYNC_OWNER);
        }
        break;
    }
}

/*
 * KHEZU_REACT_STATUS4_DOWNED (variant 0) and KHEZU_REACT_STUN_DOWNED (1):
 * lies in a hold for Em.timer frames with status 4's effect or the stun's
 * stage object, then the matching downed end.
 */
void EmKhezu_HeldDowned(Em *em, EmWork *work, s32 variant)
{
    s32 k;

    k = variant & 0xFF;
    if (k == 0) {
        Em_SetHitReactMode(em, EM_HIT_REACT_STATUS);
    }
    em->holdTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, KHEZU_MOT_DOWNED, 0, 0);
        em->noReact = 1;
        Em_RestartScript(em);
        if (k == 1) {
            StunStars_CreateEm(em, 1, 0x22, (s32) ((f32) em->timer + (em->motion[0].frameCount / 2.0f)), 150.0f, 3.0f);
        }
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetTrapTimeGoal(em);
            if (k == 1) {
                EmKhezu_ChangeState(em, EM_GROUP_REACT, KHEZU_REACT_STUN_DOWNED_END, EM_SYNC_OWNER);
            } else {
                EmKhezu_ChangeState(em, EM_GROUP_REACT, KHEZU_REACT_STATUS4_DOWNED_END, EM_SYNC_OWNER);
            }
        }
        break;
    case 2:
        if (em->remote == 0) {
            if (k == 1) {
                EmKhezu_ChangeState(em, EM_GROUP_REACT, KHEZU_REACT_STUN_DOWNED_END, EM_SYNC_OWNER);
            } else {
                EmKhezu_ChangeState(em, EM_GROUP_REACT, KHEZU_REACT_STATUS4_DOWNED_END, EM_SYNC_OWNER);
            }
        }
        break;
    }
    switch (k) {
    default:
        Em_SpawnStatus4Effect(em, 2);
        break;
    case 1:
        break;
    }
}

/*
 * KHEZU_REACT_STATUS2_DOWNED_END and _STATUS1_DOWNED_END: gets up in the
 * hold, then KHEZU_REACT_TRAPPED_STRUGGLE.
 */
void EmKhezu_DownedGetUp(Em *em)
{
    em->holdTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KHEZU_MOT_DOWNED_GET_UP, 0, 0);
        em->blind = 2;
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            Em_SetTrapTimeGoal(em);
            EmKhezu_ChangeState(em, EM_GROUP_REACT, KHEZU_REACT_TRAPPED_STRUGGLE, EM_SYNC_OWNER);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmKhezu_ChangeState(em, EM_GROUP_REACT, KHEZU_REACT_TRAPPED_STRUGGLE, EM_SYNC_OWNER);
        }
        break;
    }
}

/*
 * KHEZU_REACT_HOLD2: held by the second hold: flinches for 52 frames (root
 * motion off), then lies for Em.timer frames and
 * KHEZU_IDLE_HOLD2_SHAKE_OFF.
 */
void EmKhezu_Hold2(Em *em)
{
    Em_SetHold2(em);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, KHEZU_MOT_FLINCH, 0, 0);
        Player_SetFlag((Player *) em, EM_FLAG_NO_ROOT_MOTION);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 52.0f) != 0) {
            em->step++;
            Em_SetMotion(em, KHEZU_MOT_STATUS4, 0xC, 0);
            Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            EmKhezu_ChangeState(em, EM_GROUP_IDLE, KHEZU_IDLE_HOLD2_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmKhezu_ChangeState(em, EM_GROUP_IDLE, KHEZU_IDLE_HOLD2_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * KHEZU_EVENT_STORY: the story scene of quest stage QUEST_ID_KHEZU_SCENE: waits, walks to
 * its goal, plays KHEZU_MOT_IDLE_ACTION_C, leaps across with the leap's
 * curves to a fixed point, plays KHEZU_MOT_ATK_ACTION_D and waits until
 * EVENT_FLAG_KHEZU_SCENE is set. The monster can't be hit or sense anything
 * meanwhile.
 */
void EmKhezu_StoryScene(Em *em, EmWork *work)
{
    f32 dist;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        GmEm_ResetSize(em);
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->posX = 12220.0f;
        em->posY = 0.0f;
        em->posZ = 10119.0f;
        em->goal[0] = 9290.0f;
        em->goal[1] = 0.0f;
        em->goal[2] = 10238.0f;
        em->rotX = 0;
        em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        em->rotZ = 0;
        Em_SetMotion(em, KHEZU_MOT_IDLE, 0, 0);
        em->timer = 180;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            em->timer = 300;
            Em_SetMotion(em, KHEZU_MOT_WALK, 0, 0);
        }
        break;
    case 2:
        dist = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer <= 0 || dist <= 200.0f) {
            em->step++;
            em->rotY = 0x1800;
            Em_SetMotion(em, KHEZU_MOT_IDLE_ACTION_C, 0, 0);
        }
        break;
    case 3:
        if (EmMotion_PassedFrameScaled(em, 246.0f, 0U) != 0) {
            em->step++;
            Em_SetMotion(em, KHEZU_MOT_LEAP, 0, 0);
            EmKhezu_StartCurves(em, 0xE);
            em->stance = EM_STANCE_GROUND;
        }
        break;
    case 4:
        if (EmMotion_PassedFrameScaled(em, 78.0f, 0U) != 0) {
            em->step++;
            em->stance = EM_STANCE_AIR;
            EmKhezu_UpdateCurves(em);
        }
        break;
    case 5:
        if (EmKhezu_UpdateCurves(em) != 0 && em->posY <= em->floorY) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posX = 9700.0f;
            em->posY = 0.0f;
            em->posZ = 10855.0f;
            Em_SetMotion(em, KHEZU_MOT_LEAP_LAND, 0, 0);
        }
        break;
    case 6:
        if (EmMotion_PassedFrameScaled(em, 150.0f, 0U) != 0) {
            em->step++;
            Em_SetMotion(em, KHEZU_MOT_ATK_ACTION_D, 0, 0);
        }
        break;
    case 7:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KHEZU_MOT_ATK_RECOVER, 0, 0);
        }
        break;
    case 8:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KHEZU_MOT_IDLE, 0, 0);
        }
        break;
    case 9:
        if (Save_TestEventFlag(EVENT_FLAG_KHEZU_SCENE) == 1) {
            em->step++;
            EmKhezu_EndState(em);
        }
        break;
    }
}

/*
 * KHEZU_EVENT_CAPTURED (and the unused event states 1-3): captured in
 * holdTimer's hold: lies (QuestEm_Captured), then, if the quest lets it
 * appear again, fades out over 150 frames and KHEZU_DEFEAT_REAPPEAR.
 */
void EmKhezu_Captured(Em *em, EmWork *work)
{
    f32 pos[3];

    Em_SetCaptured(em, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KHEZU_MOT_DOWNED, 0, 0);
        Em_LoadReactionSet(em, 1);
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
                EmKhezu_ChangeState(em, EM_GROUP_DEFEAT, KHEZU_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
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
        if (++em->timer % 135 == 0) {
            Em_PlaySeAtBone(em, 0x57, 0x23);
        }
    }
}

/*
 * KHEZU_EVENT_CAPTURED_HOLD2: the same in holdTimer2's hold, starting
 * status 1 itself (collapse, then lying).
 */
void EmKhezu_CapturedHold2(Em *em, EmWork *work)
{
    s32 timeNow;
    s32 timeLeft;

    Em_SetCaptured(em, 1);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KHEZU_MOT_COLLAPSE, 0, 0);
        Em_LoadReactionSet(em, 1);
        Em_BeginStatus1(em);
        Em_ClearPoison(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KHEZU_MOT_LIE, 0, 0);
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
        timeLeft = em->timer;
        if (timeLeft > 0) {
            em->timer = timeLeft - 1;
        }
        timeNow = em->timer;
        if (timeNow <= 0) {
            if (em->remote == 0) {
                EmKhezu_ChangeState(em, EM_GROUP_DEFEAT, KHEZU_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
                break;
            }
        } else {
            em->alpha = (f32) timeNow / 150.0f;
        }
        break;
    }
}

/*
 * KHEZU_VS_EAT_BAIT: at a versus match's end (Versus_DefeatCamera) the bait
 * is thrown to it (VersusBait_Create at EmKhezu_BaitThrowPt); it eats,
 * stands 30 frames, then Versus_RouteEmByTeam picks a pose. It can't be hit
 * meanwhile.
 */
void EmKhezu_VersusEatBait(Em *em, EmWork *work)
{
    Vec3f throwPt;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        if (Versus_DefeatCamera(em) == 1) {
            throwPt = EmKhezu_BaitThrowPt;
            VersusBait_Create(&gPlayers[(u8) em->side - 1], em, 0, 0, &throwPt.x);
            em->step++;
            Em_SetMotion(em, KHEZU_MOT_EAT_START, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KHEZU_MOT_EAT, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 30;
            Em_SetMotion(em, KHEZU_MOT_ALERT_IDLE, 0, 0);
        }
        break;
    case 3:
        if (--em->timer <= 0) {
            Versus_RouteEmByTeam(em);
        }
        break;
    }
}

/* KHEZU_VS_POSE_A: the flinch motion, then the idle and KHEZU_VS_WAIT_BOTH. */
void EmKhezu_VersusPoseA(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, KHEZU_MOT_FLINCH, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KHEZU_MOT_IDLE, 0, 0);
            EmKhezu_ChangeState(em, EM_GROUP_VERSUS, KHEZU_VS_WAIT_BOTH, EM_SYNC_SEND);
        }
        break;
    }
}

/*
 * KHEZU_VS_POSE_B: KHEZU_MOT_IDLE_ACTION_C, then the idle and
 * KHEZU_VS_WAIT_BOTH.
 */
void EmKhezu_VersusPoseB(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, KHEZU_MOT_IDLE_ACTION_C, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KHEZU_MOT_IDLE, 0, 0);
            EmKhezu_ChangeState(em, EM_GROUP_VERSUS, KHEZU_VS_WAIT_BOTH, EM_SYNC_SEND);
        }
        break;
    }
}

/*
 * KHEZU_VS_POSE_C: KHEZU_MOT_ATK_ACTION_D and its recovery, then the idle
 * and KHEZU_VS_WAIT_BOTH.
 */
void EmKhezu_VersusPoseC(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, KHEZU_MOT_ATK_ACTION_D, 0, 0);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            Em_SetMotion(em, KHEZU_MOT_ATK_RECOVER, 0, 0);
        }
        break;
    case 2:
        if (em->motion[1].playing == 0) {
            em->step++;
            Em_SetMotion(em, KHEZU_MOT_IDLE, 0, 0);
            EmKhezu_ChangeState(em, EM_GROUP_VERSUS, KHEZU_VS_WAIT_BOTH, EM_SYNC_SEND);
        }
        break;
    }
}

/*
 * KHEZU_VS_WAIT_BOTH: idles until both sides are defeated
 * (Versus_BothDefeated), then KHEZU_VS_END.
 */
void EmKhezu_VersusWaitBoth(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        if (em->motionId[0] != EM_MOTION_ID(0, KHEZU_MOT_IDLE)) {
            Em_SetMotion(em, KHEZU_MOT_IDLE, 0, 0);
        }
        
    case 1:
        if (Versus_BothDefeated() == 1) {
            em->step++;
            EmKhezu_ChangeState(em, EM_GROUP_VERSUS, KHEZU_VS_END, EM_SYNC_SEND);
        }
        break;
    }
}

/*
 * KHEZU_VS_END: idles 90 frames, makes the monster hittable again, then
 * EmKhezu_EndState.
 */
void EmKhezu_VersusEnd(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 0x5A;
        if (em->motionId[0] != EM_MOTION_ID(0, KHEZU_MOT_IDLE)) {
            Em_SetMotion(em, KHEZU_MOT_IDLE, 0, 0);
        }
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            em->senseDelay = 0;
            em->noHitTimer = 0;
            EmKhezu_EndState(em);
        }
        break;
    }
}

/*
 * KHEZU_DEFEAT_DIE: dies (QuestEm_Hunted, a camera shake at frame 212),
 * leaves the carcass's mine point and lies up to 1800 frames; if the quest
 * lets it appear again, fades out over 150 frames and
 * KHEZU_DEFEAT_REAPPEAR.
 */
void EmKhezu_Die(Em *em, EmWork *work)
{
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, KHEZU_MOT_DIE, 0, 0);
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
                EmKhezu_ChangeState(em, EM_GROUP_DEFEAT, KHEZU_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    case 99:
        if (em->motion[0].playing == 0) {
            em->stance = EM_STANCE_GROUND;
            EmKhezu_EndState(em);
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
        }
        break;
    }
}

/*
 * KHEZU_DEFEAT_DIE_HELD: as EmKhezu_Die in a hold, lying in the downed
 * motion.
 */
void EmKhezu_DieHeld(Em *em, EmWork *work)
{
    em->holdTimer = 5;
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, KHEZU_MOT_DOWNED, 0, 0);
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_DEAD;
        QuestEm_Hunted(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->timer = 1800;
            em->step++;
            EmMine_Create(em, 0);
            em->fieldCollide = 0;
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
                EmKhezu_ChangeState(em, EM_GROUP_DEFEAT, KHEZU_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    case 99:
        if (em->motion[0].playing == 0) {
            em->stance = EM_STANCE_GROUND;
            EmKhezu_EndState(em);
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
        }
        break;
    }
}

/*
 * KHEZU_DEFEAT_DIE_FALLING: dies in the air: falls, lands, and at frame 60
 * of the landing dies as EmKhezu_Die does.
 */
void EmKhezu_DieFalling(Em *em, EmWork *work)
{
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KHEZU_MOT_FALL, 0, 0);
        if (em->velY > -50.0f) {
            em->velY = -50.0f;
        }
        em->accelY = -10.0f;
        Em_ClearTilt(em, 1);
        break;
    case 1:
        Em_MoveRotatedAccel(em, &em->rotX);
        if (em->posY <= em->floorY) {
            em->step++;
            em->posY = em->floorY;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, KHEZU_MOT_FALL_LAND, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    case 2:
        if (EmMotion_PassedFrameScaled(em, 60.0f, 0) != 0) {
            em->step++;
            em->timer = 0;
            Em_SetMotion(em, KHEZU_MOT_DIE_FALLING, 0, 0);
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
                EmKhezu_ChangeState(em, EM_GROUP_DEFEAT, KHEZU_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    case 99:
        if (em->motion[0].playing == 0) {
            em->stance = EM_STANCE_GROUND;
            EmKhezu_EndState(em);
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
        }
        break;
    }
}

/*
 * KHEZU_DEFEAT_REAPPEAR: when the quest lets the monster appear again, it
 * comes in from the air at the area's flight height (KHEZU_FLY_LAND);
 * otherwise its task is finished (Em.taskPhase).
 */
void EmKhezu_Reappear(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        if (QuestEm_CanAppear(em) == 1) {
            em->posY = gAreaGroundY[em->area];
            em->stance = EM_STANCE_AIR;
            EmKhezu_ChangeState(em, EM_GROUP_FLY, KHEZU_FLY_LAND, EM_SYNC_LOCAL);
            break;
        }
        em->taskPhase++;
        break;
    }
}

/*
 * Runs the current group-0 state (KHEZU_IDLE_*); several states share a
 * function.
 */
void EmKhezu_RunIdleState(Em *em, EmWork *work)
{
    switch (em->state) {
    case KHEZU_IDLE_STAND:
        EmKhezu_Stand(em, work);
        break;
    case KHEZU_IDLE:
        EmKhezu_Idle(em, work);
        break;
    case KHEZU_IDLE_LOOP_START:
        EmKhezu_IdleLoopStart(em, work);
        break;
    case KHEZU_IDLE_LOOP:
        EmKhezu_IdleLoop(em, work);
        break;
    case KHEZU_IDLE_ACTION_A:
        EmKhezu_IdleActionA(em, work);
        break;
    case KHEZU_IDLE_ACTION_B:
        EmKhezu_IdleActionB(em, work);
        break;
    case KHEZU_IDLE_ACTION_C:
        EmKhezu_IdleActionC(em, work);
        break;
    case KHEZU_IDLE_WAIT:
        EmKhezu_Wait(em, work);
        break;
    case KHEZU_IDLE_WAIT_ALERT:
        EmKhezu_WaitAlert(em, work);
        break;
    case KHEZU_IDLE_TURN_TO_GOAL:
        EmKhezu_IdleTurnToGoal(em, work);
        break;
    case KHEZU_IDLE_ACTION_D:
        EmKhezu_IdleActionD(em, work);
        break;
    case KHEZU_IDLE_EAT_NEED2:
        EmKhezu_EatNeed2(em, work);
        break;
    case KHEZU_IDLE_ALERT:
        EmKhezu_AlertIdle(em, work);
        break;
    case KHEZU_IDLE_REST:
        EmKhezu_Rest(em, work);
        break;
    case KHEZU_IDLE_EAT_START:
        EmKhezu_EatStart(em, work);
        break;
    case KHEZU_IDLE_STATUS2_COLLAPSE:
        EmKhezu_Status2Collapse(em, work);
        break;
    case KHEZU_IDLE_STATUS2_LIE_DOWN:
        EmKhezu_Status2LieDown(em, work);
        break;
    case KHEZU_IDLE_STATUS4_SHAKE_OFF:
        EmKhezu_ShakeOff(em, work);
        break;
    case KHEZU_IDLE_REST_END:
        EmKhezu_RestEnd(em, work);
        break;
    case KHEZU_IDLE_STATUS2_END:
        EmKhezu_GetUpAfterStatus(em, work);
        break;
    case KHEZU_IDLE_SHAKE_OFF:
        EmKhezu_ShakeOff(em, work);
        break;
    case KHEZU_IDLE_EAT_NEED1:
        EmKhezu_EatNeed1(em, work);
        break;
    case KHEZU_IDLE_STATUS1_COLLAPSE:
        EmKhezu_Status1Collapse(em, work);
        break;
    case KHEZU_IDLE_STATUS1_END:
        EmKhezu_GetUpAfterStatus(em, work);
        break;
    case KHEZU_IDLE_STATUS2_DOWNED:
        EmKhezu_Status2Downed(em, work);
        break;
    case KHEZU_IDLE_STATUS1_DOWNED:
        EmKhezu_Status1Downed(em, work);
        break;
    case KHEZU_IDLE_HOLD2_SHAKE_OFF:
        EmKhezu_ShakeOff(em, work);
        break;
    case KHEZU_IDLE_STUN_SHAKE_OFF:
        EmKhezu_ShakeOff(em, work);
        break;
    case KHEZU_IDLE_EAT:
        EmKhezu_Eat(em, work);
        break;
    case KHEZU_IDLE_ROAR_RAGE:
        EmKhezu_AlertIdle(em, work);
        break;
    case KHEZU_IDLE_ROAR_PENDING_RAGE:
        EmKhezu_AlertIdle(em, work);
        
    default:
        break;
    }
}

/* Runs the current group-1 state (KHEZU_MOVE_*). */
void EmKhezu_RunMoveState(Em *em, EmWork *work)
{
    switch (em->state) {
    case KHEZU_MOVE_WALK:
        EmKhezu_Walk(em, work);
        break;
    case KHEZU_MOVE_TURN:
        EmKhezu_Turn(em, work);
        break;
    case KHEZU_MOVE_TURN_ALERT:
        EmKhezu_TurnAlert(em, work);
        break;
    case KHEZU_MOVE_LIMP:
        EmKhezu_Limp(em, work);
        break;
    case KHEZU_MOVE_TURN_TO_GOAL:
        EmKhezu_TurnToGoal(em, work);
        break;
    }
}

/* Runs the current group-2 state (KHEZU_FLY_*): flight and the ceiling. */
void EmKhezu_RunFlyState(Em *em, EmWork *work)
{
    switch (em->state) {
    case KHEZU_FLY_TAKE_OFF:
        EmKhezu_TakeOff(em, work);
        break;
    case KHEZU_FLY_LAND:
        EmKhezu_Land(em, work);
        break;
    case KHEZU_FLY_HOVER:
        EmKhezu_Hover(em, work);
        break;
    case KHEZU_FLY_HOP_TURN:
        EmKhezu_HopTurn(em, work);
        break;
    case KHEZU_FLY_HOP_FORWARD:
        EmKhezu_HopForward(em, work);
        break;
    case KHEZU_FLY_HOP_BACK:
        EmKhezu_HopBack(em, work);
        break;
    case KHEZU_FLY_TO_GOAL:
        EmKhezu_FlyToGoal(em, work);
        break;
    case KHEZU_FLY_TO_GOAL_PITCHED:
        EmKhezu_FlyToGoalPitched(em, work);
        break;
    case KHEZU_FLY_OUT_OF_AREA:
        EmKhezu_FlyOutOfArea(em, work);
        break;
    case KHEZU_FLY_TO_GOAL_AND_LAND:
        EmKhezu_FlyToGoalAndLand(em, work);
        break;
    case KHEZU_FLY_TO_GOAL_AND_LAND_B:
        EmKhezu_FlyToGoalAndLandB(em, work);
        break;
    case KHEZU_FLY_RISE_TO_AREA_HEIGHT:
        EmKhezu_RiseToAreaHeight(em, work);
        break;
    case KHEZU_FLY_INTO_AREA:
        EmKhezu_FlyIntoArea(em, work);
        break;
    case KHEZU_FLY_HOVER_ONCE:
        EmKhezu_HoverOnce(em, work);
        break;
    case KHEZU_FLY_RISE_ABOVE_GOAL:
        EmKhezu_RiseAboveGoal(em, work);
        break;
    case KHEZU_FLY_HOVER_TURN:
        EmKhezu_HoverTurn(em, work);
        break;
    case KHEZU_FLY_EMPTY_A:
        EmKhezu_FlyEmptyA();
        break;
    case KHEZU_FLY_EMPTY_B:
        EmKhezu_FlyEmptyB();
        break;
    case KHEZU_FLY_EMPTY_C:
        EmKhezu_FlyEmptyC();
        break;
    case KHEZU_FLY_HOP_SLAM:
        EmKhezu_HopSlam(em, work);
        break;
    case KHEZU_FLY_TO_GOAL_DIRECT:
        EmKhezu_FlyToGoalDirect(em, work);
        break;
    case KHEZU_FLY_EMPTY_D:
        EmKhezu_FlyEmptyD();
        break;
    case KHEZU_FLY_ESCAPE_TRAP:
        EmKhezu_EscapeTrap(em, work);
        break;
    case KHEZU_FLY_CEIL_LEAP_UP:
        EmKhezu_LeapToCeiling(em, work);
        break;
    case KHEZU_FLY_CEIL_HANG:
        EmKhezu_HangFromCeiling(em, work);
        break;
    case KHEZU_FLY_CEIL_DROP:
        EmKhezu_DropFromCeiling(em, work);
        break;
    case KHEZU_FLY_CEIL_TURN:
        EmKhezu_TurnOnCeiling(em, work);
        break;
    case KHEZU_FLY_CEIL_CRAWL:
        EmKhezu_CrawlOnCeiling(em, work);
        break;
    case KHEZU_FLY_CEIL_CIRCUIT:
        EmKhezu_CrawlCircuit(em, work);
        break;
    case KHEZU_FLY_CEIL_CIRCUIT_REVERSE:
        EmKhezu_CrawlCircuitReverse(em, work);
        break;
    case KHEZU_FLY_CEIL_TURN_TO_CIRCUIT:
        EmKhezu_TurnToCircuit(em, work);
        break;
    case KHEZU_FLY_CEIL_CRAWL_TO_ZONE:
        EmKhezu_CrawlToTargetZone(em, work);
        break;
    case KHEZU_FLY_CEIL_TURN_TO_ZONE:
        EmKhezu_TurnToTargetZone(em, work);
        break;
    case KHEZU_FLY_CEIL_TURN_TO_CRAWL:
        EmKhezu_TurnToCrawl(em, work);
        
    default:
        break;
    }
}

/* Runs the current group-3 state (KHEZU_ATK_*). */
void EmKhezu_RunAttackState(Em *em, EmWork *work)
{
    switch (em->state) {
    case KHEZU_ATK_LEAP:
        EmKhezu_Leap(em, work);
        break;
    case KHEZU_ATK_CEIL_SHOTS:
        EmKhezu_CeilingShots(em, work);
        break;
    case KHEZU_ATK_ACTION_A:
        EmKhezu_AttackActionA(em, work);
        break;
    case KHEZU_ATK_ACTION_B:
        EmKhezu_AttackActionB(em, work);
        break;
    case KHEZU_ATK_ACTION_C:
        EmKhezu_AttackActionC(em, work);
        break;
    case KHEZU_ATK_SHOT_FAN:
        EmKhezu_ShotFan(em, work);
        break;
    case KHEZU_ATK_SPIN:
        EmKhezu_Spin(em, work);
        break;
    case KHEZU_ATK_ACTION_D:
        EmKhezu_AttackActionD(em, work);
        break;
    case KHEZU_ATK_LEAP_QUICK:
        EmKhezu_LeapQuick(em, work);
        
    default:
        break;
    }
}

/*
 * Runs the current group-4 state (KHEZU_REACT_*); EmKhezu_KnockDown and
 * EmKhezu_HeldDowned take the state's variant.
 */
void EmKhezu_RunReactState(Em *em, EmWork *work)
{
    switch (em->state) {
    case KHEZU_REACT_FLINCH:
        EmKhezu_Flinch(em);
        break;
    case KHEZU_REACT_CEIL_FALL:
        EmKhezu_CeilingFall(em);
        break;
    case KHEZU_REACT_FLINCH_B:
        EmKhezu_FlinchB(em);
        break;
    case KHEZU_REACT_FLINCH_C:
        EmKhezu_FlinchC(em);
        break;
    case KHEZU_REACT_KNOCK_DOWN_PART3:
        EmKhezu_KnockDown(em, work, 0);
        break;
    case KHEZU_REACT_KNOCK_DOWN:
        EmKhezu_KnockDown(em, work, 1);
        break;
    case KHEZU_REACT_FALL:
        EmKhezu_Fall(em);
        break;
    case KHEZU_REACT_STATUS4_DOWNED_END:
        EmKhezu_Struggle(em);
        break;
    case KHEZU_REACT_STATUS4:
        EmKhezu_Status4(em);
        break;
    case KHEZU_REACT_TRAPPED:
        EmKhezu_Trapped(em);
        break;
    case KHEZU_REACT_TRAPPED_STRUGGLE:
        EmKhezu_Struggle(em);
        break;
    case KHEZU_REACT_STATUS4_DOWNED:
        EmKhezu_HeldDowned(em, work, 0);
        break;
    case KHEZU_REACT_STATUS2_DOWNED_END:
        EmKhezu_DownedGetUp(em);
        break;
    case KHEZU_REACT_STATUS1_DOWNED_END:
        EmKhezu_DownedGetUp(em);
        break;
    case KHEZU_REACT_HOLD2:
        EmKhezu_Hold2(em);
        break;
    case KHEZU_REACT_STUN:
        EmKhezu_KnockDown(em, work, 2);
        break;
    case KHEZU_REACT_STUN_B:
        EmKhezu_KnockDown(em, work, 3);
        break;
    case KHEZU_REACT_STUN_DOWNED:
        EmKhezu_HeldDowned(em, work, 1);
        break;
    case KHEZU_REACT_STUN_DOWNED_END:
        EmKhezu_Struggle(em);
        break;
    }
}

/* Runs the current group-5 state (KHEZU_DEFEAT_*). */
void EmKhezu_RunDefeatState(Em *em, EmWork *work)
{
    switch (em->state) {
    case KHEZU_DEFEAT_DIE:
        EmKhezu_Die(em, work);
        break;
    case KHEZU_DEFEAT_DIE_HELD:
        EmKhezu_DieHeld(em, work);
        break;
    case KHEZU_DEFEAT_DIE_FALLING:
        EmKhezu_DieFalling(em, work);
        break;
    case KHEZU_DEFEAT_REAPPEAR:
        EmKhezu_Reappear(em, work);
        break;
    }
}

/* Runs the current group-6 (or 7) state (KHEZU_EVENT_*). */
void EmKhezu_RunEventState(Em *em, EmWork *work)
{
    switch (em->state) {
    case KHEZU_EVENT_STORY:
        EmKhezu_StoryScene(em, work);
        break;
    case 1:
        EmKhezu_Captured(em, work);
        break;
    case 2:
        EmKhezu_Captured(em, work);
        break;
    case 3:
        EmKhezu_Captured(em, work);
        break;
    case KHEZU_EVENT_CAPTURED:
        EmKhezu_Captured(em, work);
        break;
    case KHEZU_EVENT_CAPTURED_HOLD2:
        EmKhezu_CapturedHold2(em, work);
        
    default:
        break;
    }
}

/* Runs the current group-8 state (KHEZU_VS_*). */
void EmKhezu_RunVersusState(Em *em, EmWork *work)
{
    switch (em->state) {
    case KHEZU_VS_EAT_BAIT:
        EmKhezu_VersusEatBait(em, work);
        break;
    case KHEZU_VS_POSE_A:
        EmKhezu_VersusPoseA(em, work);
        break;
    case KHEZU_VS_POSE_B:
        EmKhezu_VersusPoseB(em, work);
        break;
    case KHEZU_VS_POSE_C:
        EmKhezu_VersusPoseC(em, work);
        break;
    case KHEZU_VS_WAIT_BOTH:
        EmKhezu_VersusWaitBoth(em, work);
        break;
    case KHEZU_VS_END:
        EmKhezu_VersusEnd(em, work);
        break;
    }
}

/*
 * The family's per-frame update. Finds the ceiling over the monster in the
 * player's area (EmKhezuWork.ceilKind and zone; a fixed ceiling in the
 * areas EmKhezu_InSpecialArea names), decays the needs on two maps, then
 * turns Em_UpdateDamage's result into a state: death (KHEZU_DEFEAT_*),
 * traps, statuses, falls from the air or the ceiling, flinches by part, the
 * stun and the holds; a hit while lying ends the lying state. When parts 0
 * and 6 are broken, part 1 breaks too. Then the story scene of quest stage
 * 0x422 or the pending script, and the state (twice when a state was
 * requested).
 */
void EmKhezu_Update(Em *em)
{
    u8 hitInfo[4];
    EmKhezuWork *w = (EmKhezuWork *)&em->work;
    s32 result;

    em->moveFrozen = 0;
    if (gScene.area == em->area) {
        em->hasCeiling = AreaGrid2_GetCeiling(&em->posX, &em->surfaceY, (struct AreaTri *)&w->ceilKind);
    } else if (EmKhezu_InSpecialArea(em->area) == 1) {
        em->hasCeiling = 1;
        em->surfaceY = 1500.0f;
        if (em->stateGroup == EM_GROUP_IDLE || em->stateGroup == EM_GROUP_ATTACK) {
            Em_HoldPosNoPlayers(em, 0, 1);
        }
    } else {
        em->hasCeiling = 0;
    }
    Em_RaisePlayerAreaTriggerLax(em);
    Em_UpdateCombatTimer(em);
    if (em->noAlert != 1) {
        switch (Map_DayId(gScene.map)) {
        case MAP_SNOWY_MOUNTAINS:
            Em_DecayNeed1Only(em);
            Em_CheckLowHp(em, 0.2f);
            break;
        case MAP_SWAMP:
            Em_DecayNeed1(em);
            Em_CheckLowHp(em, 0.2f);
            break;
        }
    }
    result = Em_UpdateDamage(em, hitInfo) & 0xFF;
    if (result != 0 && result != 0xE) {
        w->unk044 = 0;
        w->unk045 = 0;
    }
    switch (result) {
    case EM_DMG_NONE:
    case EM_DMG_FLASH:
    case EM_DMG_PART8_DOWN:
    case EM_DMG_HIT:
    case EM_DMG_ATTACK5:
    case EM_DMG_ATTACK5_FLINCH:
        break;
    case EM_DMG_KILLED:
    case EM_DMG_DEAD:
        if (em->stance == EM_STANCE_AIR) {
            EmKhezu_ChangeState(em, EM_GROUP_DEFEAT, KHEZU_DEFEAT_DIE_FALLING, EM_SYNC_OWNER);
        } else if ((s8) em->holdTimer != 0) {
            EmKhezu_ChangeState(em, EM_GROUP_DEFEAT, KHEZU_DEFEAT_DIE_HELD, EM_SYNC_OWNER);
        } else {
            EmKhezu_ChangeState(em, EM_GROUP_DEFEAT, KHEZU_DEFEAT_DIE, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_ATTACK9:
    case EM_DMG_ATTACK6:
        if ((s8) em->holdTimer == 0) {
            Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
            Em_SetTrapTime(em, hitInfo);
            Em_SetTrapTimeGoal(em);
            EmKhezu_ChangeState(em, EM_GROUP_REACT, KHEZU_REACT_TRAPPED, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_STATUS_IN_AIR:
        if ((em->stateGroup != EM_GROUP_REACT || em->state != KHEZU_REACT_FALL) && (em->stateGroup != EM_GROUP_REACT || em->state != KHEZU_REACT_CEIL_FALL)) {
            if ((s8) em->ceilingTimer != 0) {
                EmKhezu_ChangeState(em, EM_GROUP_REACT, KHEZU_REACT_CEIL_FALL, EM_SYNC_OWNER);
            } else {
                EmKhezu_ChangeState(em, EM_GROUP_REACT, KHEZU_REACT_FALL, EM_SYNC_OWNER);
            }
        }
        break;
    case EM_DMG_STATUS4:
        if ((s8) em->holdTimer != 0) {
            Em_SetStatus4Timer(em);
            EmKhezu_ChangeState(em, EM_GROUP_REACT, KHEZU_REACT_STATUS4_DOWNED, EM_SYNC_OWNER);
        } else if ((em->stateGroup != EM_GROUP_REACT || em->state != KHEZU_REACT_CEIL_FALL) && (em->stateGroup != EM_GROUP_REACT || em->state != KHEZU_REACT_STATUS4) &&
                   (em->stateGroup != EM_GROUP_REACT || em->state != KHEZU_REACT_FALL)) {
            Em_SetStatus4Timer(em);
            EmKhezu_ChangeState(em, EM_GROUP_REACT, KHEZU_REACT_STATUS4, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_CAPTURED:
        if (Em_IsHeld(em) == 1) {
            Em_SetStatus1Timer(em);
            if ((s8) em->holdTimer != 0) {
                Em_SetStatus1Timer(em);
                EmKhezu_ChangeState(em, EM_GROUP_EVENT, KHEZU_EVENT_CAPTURED, EM_SYNC_OWNER);
                break;
            }
            if ((s8) em->holdTimer2 != 0) {
                EmKhezu_ChangeState(em, EM_GROUP_EVENT, KHEZU_EVENT_CAPTURED_HOLD2, EM_SYNC_OWNER);
                break;
            }
        }
        if ((s8) em->holdTimer != 0) {
            Em_SetStatus1Timer(em);
            EmKhezu_ChangeState(em, EM_GROUP_EVENT, KHEZU_EVENT_CAPTURED, EM_SYNC_OWNER);
        } else if ((em->stateGroup != EM_GROUP_IDLE || em->state != KHEZU_IDLE_STATUS1_COLLAPSE) && (em->stateGroup != EM_GROUP_REACT || em->state != KHEZU_REACT_CEIL_FALL) &&
                   (em->stateGroup != EM_GROUP_REACT || em->state != KHEZU_REACT_FALL)) {
            Em_SetStatus1Timer(em);
            EmKhezu_ChangeState(em, EM_GROUP_IDLE, KHEZU_IDLE_STATUS1_COLLAPSE, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_STATUS2:
        if ((s8) em->holdTimer != 0) {
            Em_SetStatus2Timer(em);
            EmKhezu_ChangeState(em, EM_GROUP_IDLE, KHEZU_IDLE_STATUS2_DOWNED, EM_SYNC_OWNER);
        } else if ((s8) em->holdTimer2 != 0) {
            Em_SetStatus2Timer(em);
            EmKhezu_ChangeState(em, EM_GROUP_IDLE, KHEZU_IDLE_STATUS2_COLLAPSE, EM_SYNC_OWNER);
        } else if ((em->stateGroup != EM_GROUP_IDLE || em->state != KHEZU_IDLE_STATUS2_COLLAPSE) && (em->stateGroup != EM_GROUP_REACT || em->state != KHEZU_REACT_CEIL_FALL) &&
                   (em->stateGroup != EM_GROUP_REACT || em->state != KHEZU_REACT_FALL)) {
            Em_SetStatus2Timer(em);
            EmKhezu_ChangeState(em, EM_GROUP_IDLE, KHEZU_IDLE_STATUS2_COLLAPSE, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_HIT_LYING:
        switch (em->state) {
        case 0x12:
            EmKhezu_ChangeState(em, EM_GROUP_IDLE, KHEZU_IDLE_REST_END, EM_SYNC_OWNER);
            em->scriptPending = 0;
            break;
        case 0x14:
            EmKhezu_ChangeState(em, EM_GROUP_IDLE, KHEZU_IDLE_STATUS2_END, EM_SYNC_OWNER);
            em->scriptPending = 0;
            break;
        case 0x15:
            EmKhezu_ChangeState(em, EM_GROUP_IDLE, KHEZU_IDLE_STATUS2_END, EM_SYNC_OWNER);
            em->scriptPending = 0;
            break;
        case 0x1B:
            EmKhezu_ChangeState(em, EM_GROUP_IDLE, KHEZU_IDLE_STATUS1_END, EM_SYNC_OWNER);
            em->scriptPending = 0;
            break;
        case 0x1D:
            EmKhezu_ChangeState(em, EM_GROUP_REACT, KHEZU_REACT_STATUS2_DOWNED_END, EM_SYNC_OWNER);
            em->scriptPending = 0;
            break;
        case 0x1F:
            EmKhezu_ChangeState(em, EM_GROUP_REACT, KHEZU_REACT_STATUS1_DOWNED_END, EM_SYNC_OWNER);
            em->scriptPending = 0;
            break;
        }
        break;
    case EM_DMG_PART_FLINCH:
        Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
        if (em->stance == EM_STANCE_AIR) {
            if ((s8) em->ceilingTimer != 0) {
                EmKhezu_ChangeState(em, EM_GROUP_REACT, KHEZU_REACT_CEIL_FALL, EM_SYNC_OWNER);
            } else {
                EmKhezu_ChangeState(em, EM_GROUP_REACT, KHEZU_REACT_FALL, EM_SYNC_OWNER);
            }
        } else {
            switch (em->flinchPart) {
            case 0:
            case 7:
                EmKhezu_ChangeState(em, EM_GROUP_REACT, KHEZU_REACT_FLINCH, EM_SYNC_OWNER);
                break;
            case 5:
            case 6:
                EmKhezu_ChangeState(em, EM_GROUP_REACT, KHEZU_REACT_FLINCH_B, EM_SYNC_OWNER);
                break;
            case 1:
            case 2:
                EmKhezu_ChangeState(em, EM_GROUP_REACT, KHEZU_REACT_FLINCH_C, EM_SYNC_OWNER);
                break;
            case 3:
            case 4:
            default:
                if (em->flinchPart != 3) {
                    EmKhezu_ChangeState(em, EM_GROUP_REACT, KHEZU_REACT_KNOCK_DOWN, EM_SYNC_OWNER);
                } else {
                    EmKhezu_ChangeState(em, EM_GROUP_REACT, KHEZU_REACT_KNOCK_DOWN_PART3, EM_SYNC_OWNER);
                }
                break;
            }
        }
        break;
    case EM_DMG_HIT_MODE3:
        if (em->stance != EM_STANCE_AIR) {
            EmKhezu_ChangeState(em, EM_GROUP_REACT, KHEZU_REACT_FLINCH, EM_SYNC_OWNER);
            em->scriptPending = 0;
        }
        break;
    case EM_DMG_STATUS5:
        if ((s8) em->holdTimer != 0) {
            Em_SetStatus5Timer(em);
            EmKhezu_ChangeState(em, EM_GROUP_REACT, KHEZU_REACT_STUN_DOWNED, EM_SYNC_OWNER);
        } else if ((em->stateGroup != EM_GROUP_REACT || em->state != KHEZU_REACT_STUN) && (em->stateGroup != EM_GROUP_REACT || em->state != KHEZU_REACT_STUN_B) &&
                   (em->stateGroup != EM_GROUP_REACT || em->state != KHEZU_REACT_FALL) && (em->stateGroup != EM_GROUP_REACT || em->state != KHEZU_REACT_CEIL_FALL)) {
            Em_SetStatus5Timer(em);
            if (((em->hitAngle - em->rotY) & 0xFFFF) > 0x8000) {
                EmKhezu_ChangeState(em, EM_GROUP_REACT, KHEZU_REACT_STUN_B, EM_SYNC_OWNER);
            } else {
                EmKhezu_ChangeState(em, EM_GROUP_REACT, KHEZU_REACT_STUN, EM_SYNC_OWNER);
            }
        }
        break;
    case EM_DMG_ATTACK13:
        Em_ResetKindTimer(em);
        EmKhezu_ChangeState(em, EM_GROUP_REACT, KHEZU_REACT_HOLD2, EM_SYNC_OWNER);
        break;
    }
    if (em->parts[0].state >= 2 && em->parts[6].state >= 2) {
        Em_BreakPart(em, 1);
    }
    switch (gQuest.stage) {
    case QUEST_ID_KHEZU_SCENE:
        if ((em->type == EM_KIND_KHEZU || em->type == EM_KIND_RED_KHEZU) && Area_DayId(em->area) == AREA_SNOWY_MOUNTAINS_3 && Save_TestEventFlag(EVENT_FLAG_KHEZU_SCENE) == 0) {
            if (Area_DayId(gScene.area) == AREA_SNOWY_MOUNTAINS_3 && gScene.eventRunning == 1 && em->stateGroup != EM_GROUP_EVENT) {
                EmKhezu_ChangeState(em, EM_GROUP_EVENT, KHEZU_EVENT_STORY, EM_SYNC_SEND);
            }
            break;
        }
    default:
        Em_UpdatePendingScript(em);
        break;
    }
    Em_ClearHitReactMode(em);
    EmKhezu_RunState(em, (EmWork *) w);
    if (em->stateRequest != 0) {
        EmKhezu_RunState(em, (EmWork *) w);
        em->stateRequest = 0;
    }
}

/*
 * Keeps the previous group and state, then runs the current group's
 * dispatcher (group 7 runs group 6's).
 */
void EmKhezu_RunState(Em *em, EmWork *work)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->stateGroup) {
    case EM_GROUP_IDLE:
        EmKhezu_RunIdleState(em, work);
        break;
    case EM_GROUP_MOVE:
        EmKhezu_RunMoveState(em, work);
        break;
    case EM_GROUP_FLY:
        EmKhezu_RunFlyState(em, work);
        break;
    case EM_GROUP_ATTACK:
        EmKhezu_RunAttackState(em, work);
        break;
    case EM_GROUP_REACT:
        EmKhezu_RunReactState(em, work);
        break;
    case EM_GROUP_DEFEAT:
        EmKhezu_RunDefeatState(em, work);
        break;
    case EM_GROUP_EVENT:
        EmKhezu_RunEventState(em, work);
        break;
    case EM_GROUP_EVENT2:
        EmKhezu_RunEventState(em, work);
        break;
    case EM_GROUP_VERSUS:
        EmKhezu_RunVersusState(em, work);
        
    default:
        break;
    }
}

/*
 * Requests the camera shake `level` when layer 0's motion passes frame
 * `frame`. The monster comes as an s32.
 */
void EmKhezu_CamShakeOnFrame(s32 em, s32 frame, s32 level)
{
    if (EmMotion_PassedFrameScaled((Em *) em, (f32) frame, 0U) != 0) {
        CamShake_ReqAIf((void *) em, level);
    }
}

/* The effects of a motion that has none: nothing. */
static void EmKhezu_MotionEffectsDefault(Em *em)
{
}

/*
 * The effects of the motion playing on layer 0, by motion and frame: the
 * contact hits (KhezuAttack), the impacts and dust (StageObj_K14, Effect0D),
 * the shots, sparks and camera shakes; with Em.rage set, the rage effect
 * every 4th frame. Keeps the motion id in EmKhezuWork.fxMotion and plays
 * its sounds (EmKhezu_MotionSounds) first.
 */
void EmKhezu_MotionEffects(Em *em, EmKhezuWork *work)
{
    f32 fxOfs[3];
    f32 fxOfsB[3];

    if (em->motionId[0] != work->fxMotion) {
        work->fxMotion = em->motionId[0];
    }
    EmKhezu_MotionSounds(em, (struct EmWork *)work);
    switch (work->fxMotion) {
    case EM_MOTION_ID(0, KHEZU_MOT_IDLE):
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_WALK):
        EmKhezu_CamShakeOnFrame((s32) em, 0x34, 1);
        EmKhezu_CamShakeOnFrame((s32) em, 0x74, 1);
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            KhezuAttack_Create(em, 0x14);
        }
        if (EmMotion_PassedFrameScaled(em, 50.0f, 0) != 0) {
            KhezuAttack_Create(em, 0x15);
        }
        if (EmMotion_PassedFrameScaled(em, 104.0f, 0) != 0) {
            KhezuAttack_Create(em, 0x16);
        }
        if (EmMotion_PassedFrameScaled(em, 162.0f, 0) != 0) {
            KhezuAttack_Create(em, 0x15);
        }
        if (EmMotion_PassedFrameScaled(em, 218.0f, 0) != 0) {
            KhezuAttack_Create(em, 0x17);
            break;
        }
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_TURN_RIGHT):
        EmKhezu_CamShakeOnFrame((s32) em, 0x3C, 1);
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            KhezuAttack_Create(em, 5);
        }
        if (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) {
            KhezuAttack_Create(em, 6);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 2.0f, 78.0f) != 0) && !(gScene.frame & 3)) {
            DustCloud_Create(em, 0x1A, 1, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_TURN_LEFT):
        EmKhezu_CamShakeOnFrame((s32) em, 0x3C, 1);
        if (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) {
            KhezuAttack_Create(em, 7);
        }
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            KhezuAttack_Create(em, 8);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 2.0f, 78.0f) != 0) && !(gScene.frame & 3)) {
            DustCloud_Create(em, 0x1A, 0, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_TURN_LEFT_ALERT):
        if (EmMotion_PassedFrameScaled(em, 18.0f, 0) != 0) {
            KhezuAttack_Create(em, 9);
        }
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            KhezuAttack_Create(em, 0xA);
        }
        if (em->rage != 0) {
            if ((EmMotion_InFrameRange(em, 0, 2.0f, 18.0f) != 0) && !(gScene.frame & 3)) {
                DustCloud_Create(em, 0x1A, 1, 1.0f);
            }
            if ((EmMotion_InFrameRange(em, 0, 20.0f, 70.0f) != 0) && !(gScene.frame & 3)) {
                DustCloud_Create(em, 0x1A, 0, 1.0f);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_TURN_RIGHT_ALERT):
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            KhezuAttack_Create(em, 0xB);
        }
        if (EmMotion_PassedFrameScaled(em, 18.0f, 0) != 0) {
            KhezuAttack_Create(em, 0xC);
        }
        if (em->rage != 0) {
            if ((EmMotion_InFrameRange(em, 0, 2.0f, 18.0f) != 0) && !(gScene.frame & 3)) {
                DustCloud_Create(em, 0x1A, 0, 1.0f);
            }
            if ((EmMotion_InFrameRange(em, 0, 20.0f, 70.0f) != 0) && !(gScene.frame & 3)) {
                DustCloud_Create(em, 0x1A, 1, 1.0f);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_LIMP):
        if (EmMotion_PassedFrameScaled(em, 42.0f, 0) != 0) {
            KhezuAttack_Create(em, 0x19);
        }
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            KhezuAttack_Create(em, 0x1A);
        }
        if (EmMotion_PassedFrameScaled(em, 74.0f, 0) != 0) {
            KhezuAttack_Create(em, 0x1A);
            break;
        }
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_DESCEND):
        if (EmMotion_PassedFrameScaled(em, 52.0f, 0) || EmMotion_PassedFrameScaled(em, 104.0f, 0) || EmMotion_PassedFrameScaled(em, 162.0f, 0)) {
            if (em->posY <= (1000.0f + em->floorY)) {
                PartFx_SpawnForEmScaled(em, 2, 7, 5.0f);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_HOVER):
        if (EmMotion_PassedFrameScaled(em, 8.0f, 0) || EmMotion_PassedFrameScaled(em, 86.0f, 0) || EmMotion_PassedFrameScaled(em, 160.0f, 0)) {
            if (em->posY <= (1000.0f + em->floorY)) {
                DustCloud_Create(em, 1, 0, 1.0f);
            }
        }
        if (em->rage != 0) {
            if (EmMotion_InFrameRange(em, 0, 70.0f, 84.0f) || EmMotion_InFrameRange(em, 0, 146.0f, 156.0f) || EmMotion_InFrameRange(em, 0, 220.0f, 234.0f)) {
                if (!(gScene.frame & 3)) {
                    DustCloud_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
                    break;
                }
            }
        }
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_TAKE_OFF):
        if (EmMotion_PassedFrameScaled(em, 42.0f, 0) || EmMotion_PassedFrameScaled(em, 102.0f, 0) || EmMotion_PassedFrameScaled(em, 156.0f, 0)) {
            if (em->posY <= (1000.0f + em->floorY)) {
                DustCloud_Create(em, 1, 0, 1.0f);
            }
        }
        if ((em->rage != 0) && (EmMotion_PassedFrameScaled(em, 38.0f, 0) != 0)) {
            DustCloud_Create(em, 0x19, 5, 1.0f);
            DustCloud_Create(em, 0x19, 4, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_TOUCH_DOWN):
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            KhezuAttack_Create(em, 0xD);
        }
        if (EmMotion_PassedFrameScaled(em, 16.0f, 0) != 0) {
            EmKhezu_SpawnLandingImpact(em);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 18.0f, 44.0f) != 0) && !(gScene.frame & 3)) {
            DustCloud_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_REST):
        fxOfs[1] = 10.0f;
        fxOfs[2] = 140.0f;
        fxOfs[0] = 0.0f;
        Em_SpawnLyingPuffs(em, 0x22, fxOfs, 1.6f);
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_REST_START):
    case EM_MOTION_ID(0, KHEZU_MOT_REST_END):
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_LIE):
        fxOfsB[1] = 10.0f;
        fxOfsB[2] = 140.0f;
        fxOfsB[0] = 0.0f;
        Em_SpawnLyingPuffs(em, 0x22, fxOfsB, 1.6f);
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_ATK_ACTION_B):
        if (EmMotion_PassedFrameScaled(em, 40.0f, 0) != 0) {
            KhezuAttack_Create(em, 0xE);
        }
        if (em->rage != 0) {
            if ((EmMotion_InFrameRange(em, 0, 28.0f, 64.0f) != 0) && !(gScene.frame & 3)) {
                DustCloud_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
            }
            if (EmMotion_InFrameRange(em, 0, 98.0f, 102.0f) != 0) {
                DustCloud_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_GET_UP):
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_COLLAPSE):
        if (EmMotion_PassedFrameScaled(em, 148.0f, 0) != 0) {
            KhezuAttack_Create(em, 0xF);
            break;
        }
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_LIE_DOWN):
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_SPIN):
        EmKhezu_CamShakeOnFrame((s32) em, 0x2C, 1);
        EmKhezu_CamShakeOnFrame((s32) em, 0x49, 1);
        if (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) {
            KhezuAttack_Create(em, 0x11);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 6.0f, 38.0f) != 0) && !(gScene.frame & 3)) {
            DustCloud_Create(em, 0x1A, 1, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_SHOT_FAN):
        if (EmMotion_PassedFrameScaled(em, 160.0f, 0) != 0) {
            KhezuAttack_Create(em, 0x25);
        }
        if (EmMotion_PassedFrameScaled(em, 58.0f, 0) != 0) {
            ThunderSpark_CreateTimed(em, 3, 0x72, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 172.0f, 0) != 0) {
            ThunderSpark_CreateOnEm(em, 4);
            break;
        }
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_ALERT_IDLE):
        if ((em->rage != 0) && ((EmMotion_InFrameRange(em, 0, 10.0f, 90.0f) != 0) || (EmMotion_InFrameRange(em, 0, 122.0f, 260.0f) != 0) || (EmMotion_ReachedFrame(em, 0, 296.0f) != 0)) && !(gScene.frame & 3)) {
            DustCloud_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_TURN_IN_PLACE):
        if ((em->stateGroup != EM_GROUP_EVENT) && (EmMotion_PassedFrameScaled(em, 78.0f, 0) != 0)) {
            KhezuAttack_Create(em, 0x13);
            ElementBurst_CreateOnEm(em, 5, 4, 1.0f);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 86.0f, 298.0f) != 0) && !(gScene.frame & 3)) {
            DustCloud_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_EAT):
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_FLINCH):
        if (EmMotion_PassedFrameScaled(em, 32.0f, 0) != 0) {
            PartFx_SpawnForEm(em, 0x1B, 7);
        }
        if (EmMotion_PassedFrameScaled(em, 54.0f, 0) != 0) {
            PartFx_SpawnForEm(em, 0x16, 7);
            break;
        }
        break;
    case EM_MOTION_ID(0, 0x3D):
        if (EmMotion_PassedFrameScaled(em, 24.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0x1A, 7, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_SHAKE_OFF):
    case EM_MOTION_ID(0, KHEZU_MOT_FLINCH_B):
    case EM_MOTION_ID(0, KHEZU_MOT_FLINCH_C):
    case EM_MOTION_ID(0, 0x43):
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_DIE):
        if (EmMotion_PassedFrameScaled(em, 268.0f, 0) != 0) {
            DustCloud_Create(em, 0, 0, 0.9f);
            break;
        }
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_KNOCK_DOWN_A):
        EmKhezu_CamShakeOnFrame((s32) em, 0x1A, 2);
        if (EmMotion_PassedFrameScaled(em, 26.0f, 0) != 0) {
            DustCloud_Create(em, 0x12, 0x80, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_DOWN_A):
    case EM_MOTION_ID(0, KHEZU_MOT_DOWN_B):
    case EM_MOTION_ID(0, KHEZU_MOT_GET_UP_A):
    case EM_MOTION_ID(0, KHEZU_MOT_GET_UP_B):
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_KNOCK_DOWN_B):
        EmKhezu_CamShakeOnFrame((s32) em, 0x1A, 2);
        if (EmMotion_PassedFrameScaled(em, 26.0f, 0) != 0) {
            DustCloud_Create(em, 0x12, 0x81, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_STATUS4):
    case EM_MOTION_ID(0, KHEZU_MOT_FALL):
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_FALL_LAND):
        if (EmMotion_PassedFrameScaled(em, 8.0f, 0) != 0) {
            DustCloud_Create(em, 0xB, 0, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_EAT_START):
        if (EmMotion_PassedFrameScaled(em, 48.0f, 0) || EmMotion_PassedFrameScaled(em, 106.0f, 0) || EmMotion_PassedFrameScaled(em, 174.0f, 0)) {
            KhezuAttack_Create(em, 0x10);
            break;
        }
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_DIE_FALLING):
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_HOP_SLAM):
        if (EmMotion_PassedFrameScaled(em, 42.0f, 0) || EmMotion_PassedFrameScaled(em, 60.0f, 0) || EmMotion_PassedFrameScaled(em, 80.0f, 0)) {
            DustCloud_Create(em, 1, 0, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, 0x5A):
        if ((EmMotion_PassedFrameScaled(em, 50.0f, 0) != 0) && (em->posY <= (1000.0f + em->floorY))) {
            DustCloud_Create(em, 9, 0, 1.0f);
            DustCloud_Create(em, 0xE, 0, 1.0f);
        }
        if ((EmMotion_PassedFrameScaled(em, 102.0f, 0) != 0) && (em->posY <= (1000.0f + em->floorY))) {
            DustCloud_Create(em, 9, 0, 1.0f);
            DustCloud_Create(em, 0xE, 0, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_TRAPPED):
        if (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0) {
            DustCloud_Create(em, 0xB, 0x80, 1.0f);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 4.0f, 60.0f) != 0) && !(gScene.frame & 3)) {
            DustCloud_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_STRUGGLE):
        if (em->rage != 0) {
            if (EmMotion_InFrameRange(em, 0, 12.0f, 24.0f) || EmMotion_InFrameRange(em, 0, 46.0f, 90.0f)) {
                if (!(gScene.frame & 3)) {
                    DustCloud_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
                    break;
                }
            }
        }
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_DOWNED):
    case EM_MOTION_ID(0, KHEZU_MOT_DOWNED_GET_UP):
    case EM_MOTION_ID(0, KHEZU_MOT_IDLE_LOOP):
    case EM_MOTION_ID(0, KHEZU_MOT_IDLE_LOOP_START):
    case EM_MOTION_ID(0, KHEZU_MOT_IDLE_ACTION_A):
    case EM_MOTION_ID(0, KHEZU_MOT_CEIL_HANG):
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_CEIL_LEAP):
        if (EmMotion_PassedFrameScaled(em, 110.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 2, 7, 3.5f);
            break;
        }
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_IDLE_ACTION_C):
        if (EmMotion_PassedFrameScaled(em, 66.0f, 0) != 0) {
            KhezuAttack_Create(em, 2);
            ElementBurst_CreateOnEm(em, 5, 5, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_LEAP):
        if (EmMotion_PassedFrameScaled(em, 80.0f, 0) != 0) {
            KhezuAttack_Create(em, 0x1D);
        }
        if (EmMotion_PassedFrameScaled(em, 70.0f, 0) != 0) {
            DustCloud_Create(em, 2, 6, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_LEAP_LAND):
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            KhezuAttack_Create(em, 0x1E);
        }
        if (EmMotion_PassedFrameScaled(em, 12.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 2, 7, 3.5f);
            break;
        }
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_LEAP_QUICK):
        if (EmMotion_PassedFrameScaled(em, 32.0f, 0) != 0) {
            KhezuAttack_Create(em, 0x1F);
        }
        if (EmMotion_PassedFrameScaled(em, 20.0f, 0) != 0) {
            DustCloud_Create(em, 2, 6, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_CEIL_DROP):
        if (EmMotion_PassedFrameScaled(em, 24.0f, 0) != 0) {
            KhezuAttack_Create(em, 0x22);
            break;
        }
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_CEIL_SHOTS):
        if ((EmMotion_ReachedFrame(em, 0, 200.0f) != 0) && !(gScene.frame & 0xF)) {
            EmRangedAttack_CreateAtBone(em, 0x22, 5, 0, 0, 0);
            break;
        }
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_CEIL_SHOTS_END):
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_ATK_ACTION_A):
        if (EmMotion_PassedFrameScaled(em, 180.0f, 0) != 0) {
            KhezuAttack_Create(em, 0x18);
        }
        if (EmMotion_PassedFrameScaled(em, 172.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0x1A, 3, 3.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 178.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0x14, 3, 3.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_ATK_ACTION_C):
        if (em->type == EM_KIND_KHEZU) {
            if (EmMotion_PassedFrameScaled(em, 66.0f, 0) != 0) {
                KhezuAttack_Create(em, 1);
            }
        } else {
            if (EmMotion_PassedFrameScaled(em, 16.0f, 0) != 0) {
                KhezuAttack_Create(em, 0x27);
            }
            if (EmMotion_PassedFrameScaled(em, 130.0f, 0) != 0) {
                KhezuAttack_Create(em, 0x28);
            }
            if ((EmMotion_InFrameRange(em, 0, 120.0f, 180.0f) != 0) && (((s32) gScene.frame % 5) == 0)) {
                ThunderSpark_CreateOnEm(em, 9);
            }
        }
        if (em->rage != 0) {
            if ((EmMotion_InFrameRange(em, 0, 40.0f, 72.0f) != 0) && !(gScene.frame & 3)) {
                DustCloud_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
            }
            if (EmMotion_InFrameRange(em, 0, 102.0f, 106.0f) != 0) {
                DustCloud_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_ATK_ACTION_D):
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            ThunderSpark_CreateOnEm(em, 0);
        }
        if (EmMotion_PassedFrameScaled(em, 114.0f, 0) != 0) {
            KhezuAttack_Create(em, 0x1B);
            break;
        }
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_ATK_RECOVER):
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_CEIL_DROP_LAND):
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            KhezuAttack_Create(em, 0x1C);
        }
        if (EmMotion_PassedFrameScaled(em, 12.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 2, 7, 3.5f);
            break;
        }
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_CEIL_LATCH):
    case EM_MOTION_ID(0, KHEZU_MOT_CEIL_CRAWL):
    case EM_MOTION_ID(0, KHEZU_MOT_CEIL_TURN_RIGHT):
    case EM_MOTION_ID(0, KHEZU_MOT_CEIL_TURN_LEFT):
        break;
    default:
        EmKhezu_MotionEffectsDefault(em);
        break;
    }
}

/*
 * Callback slot 3: from the second frame on, runs EmKhezu_MotionEffects
 * every frame.
 */
void EmKhezu_UpdateMotionEffects(Em *em)
{
    EmKhezuWork *w = (EmKhezuWork *)&em->work;

    switch (w->fxStep) {
    case 0:
        w->fxStep++;
        break;
    case 1:
        EmKhezu_MotionEffects(em, (EmKhezuWork *) w);
        break;
    }
}

/*
 * The landing impact (stage object kind 0x14, type 0xB) at the monster:
 * KHEZU_MOT_TOUCH_DOWN's frame 16 and EmKhezu_HopSlam's landing.
 */
void EmKhezu_SpawnLandingImpact(Em *em)
{
    DustCloud_Create((Em *) em, 0xB, 0, 1.0f);
}

/* Callback slot 2: nothing. */
void EmKhezu_TableNop(void)
{
}

/*
 * Callback slot 4: the script's questions about the monster, by the low
 * byte of `query`: 0, is it in one of EmKhezu_InSpecialArea's areas; 1, is
 * there a ceiling over it (Em.hasCeiling). Returns 1 or 0.
 */
s32 EmKhezu_ScriptQuery(Em *em, s32 query)
{
    s32 queryId;

    queryId = query & 0xFF;
    switch (queryId) {
    case 0:
        return EmKhezu_InSpecialArea(em->area) == 1;
    case 1:
        return em->hasCeiling != 0;
    default:
        return 0;
    }
}

/*
 * Enters group-0 state `state` (EM_SYNC_* `mode`): the alert idle and wait
 * instead of the plain ones in combat, and each state's entry calls (the
 * statuses begun or ended, the needs, the rage).
 */
void EmKhezu_SetIdleState(Em *em, u16 state, u8 mode)
{
    em->motionSpeed = 1.0f;
    switch (state) {
    case KHEZU_IDLE:
        if ((em->hp >= (s16) (0.2f * (f32) em->hpMax)) && (em->combat == 1)) {
            state = KHEZU_IDLE_ALERT;
        }
        break;
    case KHEZU_IDLE_WAIT:
        if (em->combat == 1) {
            state = KHEZU_IDLE_WAIT_ALERT;
        }
        break;
    case KHEZU_IDLE_REST:
        Em_ResetNotice(em);
        break;
    case KHEZU_IDLE_STATUS2_COLLAPSE:
        Em_BeginStatus2(em);
        break;
    case KHEZU_IDLE_STATUS2_LIE_DOWN:
        Em_BeginStatus2(em);
        break;
    case KHEZU_IDLE_STATUS4_SHAKE_OFF:
        Em_EndStatus4(em);
        break;
    case KHEZU_IDLE_REST_END:
        Em_RefillNeed3(em);
        break;
    case KHEZU_IDLE_STATUS2_END:
        Em_EndStatus2(em);
        break;
    case KHEZU_IDLE_STATUS1_COLLAPSE:
        Em_BeginStatus1(em);
        break;
    case KHEZU_IDLE_STATUS1_END:
        Em_EndStatus1(em);
        break;
    case KHEZU_IDLE_STATUS2_DOWNED:
        Em_BeginStatus2(em);
        break;
    case KHEZU_IDLE_STATUS1_DOWNED:
        Em_BeginStatus1(em);
        break;
    case KHEZU_IDLE_HOLD2_SHAKE_OFF:
        em->holdTimer2 = 0;
        break;
    case KHEZU_IDLE_STUN_SHAKE_OFF:
        Em_EndStatus5(em);
        break;
    case KHEZU_IDLE_EAT:
        Em_StartGoalAction(em, 0x2710);
        break;
    case KHEZU_IDLE_ROAR_RAGE:
        Em_StartRage(em);
        break;
    case KHEZU_IDLE_ROAR_PENDING_RAGE:
        Em_StartPendingRage(em);
        break;
    }
    em->stance = EM_STANCE_GROUND;
    Em_SetStateSync(em, EM_GROUP_IDLE, state, mode);
}

/*
 * Enters group-1 state `state`: the alert turn in combat, the limp below
 * 20% of its hp, and the walk's goal and frames (to the goal at speed 7, or
 * a random wander of 90 to 149 frames, the limp at most 300).
 */
void EmKhezu_SetMoveState(Em *em, u16 state, u8 mode)
{
    f32 dist;

    em->stance = EM_STANCE_GROUND;
    em->motionSpeed = 1.0f;
    switch (state) {
    case KHEZU_MOVE_WALK:
        Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
        if (em->hp < (s16) (0.2f * em->hpMax)) {
            state = KHEZU_MOVE_LIMP;
            goto limp; /* the limp's set-up: a jump into the next case, as the original has it */
        }
        Em_SetSpeedScaled(em, 0, 1000.0f, 500.0f);
        if (em->curGoalKind == 0) {
            em->hasGoal = 0;
            em->timer = (em->stateRand % 120 + 180) / 2;
        } else {
            em->hasGoal = 1;
            em->timer = (s32) em->goalDist / 7;
        }
        break;
    case KHEZU_MOVE_TURN:
        if (em->combat == 1) {
            state = KHEZU_MOVE_TURN_ALERT;
        }
        break;
    case KHEZU_MOVE_TURN_ALERT:
    case KHEZU_MOVE_TURN_TO_GOAL:
        break;
    case KHEZU_MOVE_LIMP:
    limp:
        Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
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
    }
    Em_SetStateSync(em, EM_GROUP_MOVE, state, mode);
}

/*
 * Enters group-2 state `state`: the flights' speeds, frames and goals (the
 * area's points, the exit and the next area; KHEZU_FLY_LAND or
 * KHEZU_FLY_TO_GOAL_AND_LAND where there is nowhere to go), and the
 * ceiling's: KHEZU_IDLE_WAIT where there is no ceiling, the circuit's legs
 * and direction, and the ceiling point of the target's zone
 * (KHEZU_FLY_CEIL_TURN_TO_CRAWL when there is none).
 */
void EmKhezu_SetFlyState(Em *em, u16 state, u8 mode)
{
    EmKhezuWork *w = (EmKhezuWork *)&em->work;
    struct {
        u8 kind;
        u8 extra;
        u16 zone;
    } targetCeil;
    f32 ceilY;
    f32 nextPt[3];
    f32 targetPos[3];
    Player *targetPl;
    s32 angle;
    f32 dist;

    em->motionSpeed = 1.0f;
    switch (state) {
    case KHEZU_FLY_TAKE_OFF:
    case KHEZU_FLY_LAND:
    case KHEZU_FLY_HOVER:
    case KHEZU_FLY_HOP_TURN:
    case KHEZU_FLY_HOP_FORWARD:
    case KHEZU_FLY_HOP_BACK:
    case 10:
    case KHEZU_FLY_RISE_TO_AREA_HEIGHT:
    case KHEZU_FLY_HOVER_ONCE:
    case KHEZU_FLY_HOVER_TURN:
    case KHEZU_FLY_HOP_SLAM:
    case KHEZU_FLY_EMPTY_D:
    case KHEZU_FLY_ESCAPE_TRAP:
    case KHEZU_FLY_CEIL_HANG:
    case KHEZU_FLY_CEIL_DROP:
    case KHEZU_FLY_CEIL_CRAWL_TO_ZONE:
        Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
        break;
    case KHEZU_FLY_TO_GOAL:
    case KHEZU_FLY_TO_GOAL_PITCHED:
    case KHEZU_FLY_TO_GOAL_DIRECT:
        Player_ClearVelocity(em);
        em->velZ = 20.0f;
        em->timer = (s32) (Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) / em->velZ) * 3;
        break;
    case KHEZU_FLY_INTO_AREA:
        Em_TypeSetup(em);
    case KHEZU_FLY_OUT_OF_AREA:
        if (em->remote == 0 && em->curGoalKind != 3) {
            EmKhezu_ChangeState(em, EM_GROUP_FLY, KHEZU_FLY_LAND, EM_SYNC_SEND);
            return;
        }
        break;
    case KHEZU_FLY_TO_GOAL_AND_LAND:
        if (em->remote == 0) {
            em->goalKind = 2;
            em->goalArg = 1;
            em->goalNo = 0;
            Em_ComputeGoal(em, em->goal);
        }
        em->timer = 500;
        Em_TypeSetup(em);
        break;
    case KHEZU_FLY_TO_GOAL_AND_LAND_B:
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
    case KHEZU_FLY_RISE_ABOVE_GOAL:
        em->timer = 500;
        break;
    case KHEZU_FLY_CEIL_LEAP_UP:
        if (em->hasCeiling == 0 && em->remote == 0) {
            EmKhezu_ChangeState(em, EM_GROUP_IDLE, KHEZU_IDLE_WAIT, EM_SYNC_SEND);
            return;
        }
        break;
    case KHEZU_FLY_CEIL_CRAWL:
        Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
        Em_SetSpeedScaled(em, 2, 1000.0f, 500.0f);
        break;
    case KHEZU_FLY_CEIL_CIRCUIT:
    case KHEZU_FLY_CEIL_CIRCUIT_REVERSE:
        w->legsLeft = 4;
        if (em->remote == 0) {
            em->goalNo = EmKhezu_NearestCeilingPoint(em, nextPt);
            em->goalKind = 2;
            em->goalArg = 2;
            Em_ComputeGoal(em, em->goal);
            angle = (u16) (Math_AngleXZ(&em->posX, em->goal) - em->rotY);
            if ((0 <= angle && angle <= 0x2000) || (angle >= 0xE000 && angle <= 0xFFFF)) {
                angle = (u16) (Math_AngleXZ(&em->posX, nextPt) - em->rotY);
                if (angle >= 0x4000 && angle <= 0xC000 && state == KHEZU_FLY_CEIL_CIRCUIT) {
                    state = KHEZU_FLY_CEIL_CIRCUIT_REVERSE;
                }
            } else {
                state = KHEZU_FLY_CEIL_TURN_TO_CIRCUIT;
            }
        } else {
            Em_ComputeGoal2(em, em->goal);
        }
        break;
    case KHEZU_FLY_CEIL_TURN_TO_ZONE:
        if (em->remote == 0) {
            targetPl = &gPlayers[em->targetNo];
            w->zonePicked = 0;
            if (em->targetNo == -1) {
                state = KHEZU_FLY_CEIL_TURN_TO_CRAWL;
                break;
            }
            em->target = targetPl;
            Em_GetTargetPos(em, em->targetNo, targetPos);
            if (AreaGrid2_GetCeiling(targetPos, &ceilY, (struct AreaTri *)&targetCeil) == 0) {
                state = KHEZU_FLY_CEIL_TURN_TO_CRAWL;
                break;
            }
            dist = Vec_DistanceXZ(&em->posX, targetPos);
            if (w->zone == targetCeil.zone || w->zone == 0 || w->zone >= 5 ||
                dist <= 500.0f) {
                state = KHEZU_FLY_CEIL_TURN_TO_CRAWL;
                break;
            }
            em->goalNo = (targetCeil.zone - 1) & 3;
            switch (w->zone) {
            case KHEZU_FLY_LAND:
                if (targetCeil.zone == 3) {
                    em->goalNo = (targetCeil.zone - 2) & 3;
                }
                break;
            case KHEZU_FLY_HOVER:
                if (targetCeil.zone == 4) {
                    em->goalNo = (targetCeil.zone - 2) & 3;
                }
                break;
            case KHEZU_FLY_HOP_TURN:
                if (targetCeil.zone == 1) {
                    em->goalNo = (targetCeil.zone - 2) & 3;
                }
                break;
            case KHEZU_FLY_HOP_FORWARD:
                if (targetCeil.zone == 2) {
                    em->goalNo = (targetCeil.zone - 2) & 3;
                }
                break;
            }
            em->goalKind = 2;
            em->goalArg = 2;
            Em_ComputeGoal(em, em->goal);
            w->zonePicked = 1;
            dist = Vec_DistanceXZ(&em->posX, em->goal);
            w->pickDist = dist;
            if (dist <= 500.0f) {
                state = KHEZU_FLY_CEIL_TURN_TO_CRAWL;
            }
        }
        break;
    case KHEZU_FLY_EMPTY_A:
    case KHEZU_FLY_EMPTY_B:
    case KHEZU_FLY_EMPTY_C:
    case KHEZU_FLY_CEIL_TURN:
    case KHEZU_FLY_CEIL_TURN_TO_CIRCUIT:
    case KHEZU_FLY_CEIL_TURN_TO_CRAWL:
        break;
    }
    Em_SetStateSync(em, EM_GROUP_FLY, state, mode);
}

/*
 * Enters group-3 state `state`; KHEZU_ATK_ACTION_A aims at the target and
 * keeps its distance (EmKhezuWork.pickDist).
 */
void EmKhezu_SetAttackState(Em *em, u16 state, u8 mode)
{
    EmKhezuWork *w = (EmKhezuWork *)&em->work;
    f32 targetPos[3];
    f32 dist;

    em->motionSpeed = 1.0f;
    switch (state) {
    case KHEZU_ATK_ACTION_A:
        if (em->targetNo == -1) {
            w->pickDist = 2500.0f;
            break;
        }
        em->target = &gPlayers[em->targetNo];
        Em_GetTargetPos(em, em->targetNo, targetPos);
        Vec_Set(em->goal, targetPos[0], targetPos[1], targetPos[2]);
        dist = Vec_DistanceXZ(&em->posX, em->goal);
        w->pickDist = dist;
        if (dist > 5000.0f) {
            w->pickDist = 5000.0f;
        } else if (dist < 2500.0f) {
            w->pickDist = 2500.0f;
        }
        break;
    case KHEZU_ATK_LEAP:
    case KHEZU_ATK_CEIL_SHOTS:
    case KHEZU_ATK_ACTION_B:
    case KHEZU_ATK_ACTION_C:
    case KHEZU_ATK_SHOT_FAN:
    case KHEZU_ATK_SPIN:
    case KHEZU_ATK_ACTION_D:
        break;
    }
    Em_SetStateSync(em, EM_GROUP_ATTACK, state, mode);
}

/*
 * Changes the state to (`group`, `state`) with sync mode `mode`: takes a
 * player as target, ignores modes EM_SYNC_OWNER and EM_SYNC_OWNER_FLAGGED
 * on a remote copy, and runs the group's Set*State or the entry calls of
 * groups 4, 5, 6 and 8 (statuses, QuestEm_Appear, Versus_MarkDefeat).
 */
void EmKhezu_ChangeState(Em *em, u16 group, u16 state, u8 mode)
{
    if (em->remote == 0) {
        Em_TargetPlayerInRange(em);
    } else if (mode == EM_SYNC_OWNER || mode == EM_SYNC_OWNER_FLAGGED) {
        return;
    }
    Em_LoadReactionSet(em, 0);
    switch (group) {
    case EM_GROUP_IDLE:
        EmKhezu_SetIdleState(em, state, mode);
        break;
    case EM_GROUP_MOVE:
        EmKhezu_SetMoveState(em, state, mode);
        break;
    case EM_GROUP_FLY:
        EmKhezu_SetFlyState(em, state, mode);
        break;
    case EM_GROUP_ATTACK:
        EmKhezu_SetAttackState(em, state, mode);
        break;
    case EM_GROUP_VERSUS:
        if (state == 0x98) {
            Versus_MarkDefeat(em);
        }
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        break;
    case EM_GROUP_DEFEAT:
        if (state == 3 && QuestEm_CanAppear(em) == 1) {
            QuestEm_Appear(em);
        }
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        break;
    case EM_GROUP_REACT:
        switch (state) {
        case KHEZU_REACT_STATUS4_DOWNED_END:
            Em_EndStatus4(em);
            break;
        case KHEZU_REACT_STATUS4:
        case KHEZU_REACT_STATUS4_DOWNED:
            Em_BeginStatus4(em);
            break;
        case KHEZU_REACT_TRAPPED:
        case KHEZU_REACT_HOLD2:
            Em_EndAllStatus(em);
            break;
        case KHEZU_REACT_STATUS2_DOWNED_END:
            Em_EndStatus2(em);
            break;
        case KHEZU_REACT_STATUS1_DOWNED_END:
            Em_EndStatus1(em);
            break;
        case KHEZU_REACT_STUN:
        case KHEZU_REACT_STUN_B:
        case KHEZU_REACT_STUN_DOWNED:
            Em_BeginStatus5(em);
            break;
        case KHEZU_REACT_STUN_DOWNED_END:
            Em_EndStatus5(em);
            break;
        }
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        break;
    case EM_GROUP_EVENT:
        switch (state) {
        case KHEZU_EVENT_CAPTURED:
            Em_BeginStatus1(em);
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
 * While the motion isn't blending, sets velY from the curve table
 * EmKhezu_MoveVelYCurves[`kind`] at the motion's frame, then moves along
 * rotY (Em_MoveRotated).
 */
void EmKhezu_MoveByVelYCurve(Em *em, s32 kind)
{
    EmWork *work = (EmWork *) &em->work;
    EmCurveKey *keys;
    EmCurveKey *key;
    f32 frame;
    f32 keyTime;
    s32 keyIndex;

    keys = EmKhezu_MoveVelYCurves[kind];
    if (keys != NULL && em->motion[0].blend == 0) {
        frame = em->motion[0].frame;
        if (frame == 0.0f || frame == 1.0f) {
            em->velY = keys->v;
        } else {
            keyIndex = 1;
            while (keyIndex != 0) {
                key = &keys[keyIndex];
                keyTime = key->t;
                if (frame > keyTime && keyTime != 0.0f) {
                    keyIndex++;
                } else if (keyTime == 0.0f) {
                    break;
                } else {
                    keyIndex = 0;
                    em->velY = (key->v - key[-1].v) / ((keyTime - key[-1].t) / em->motion[0].step);
                }
            }
        }
        work->moveAngBY = 0;
        work->moveAngBZ = em->rotY;
        work->moveAngCX = 0;
        Em_MoveRotated(em, &work->moveAngBY);
    }
}

/*
 * Starts the curves `kind` (EmKhezuWork.curveKind): which of the height and
 * speed curves run, and the motion frame they start from; the velocity is
 * cleared.
 */
void EmKhezu_StartCurves(Em *em, u8 kind)
{
    EmKhezuWork *w = (EmKhezuWork *)&em->work;

    u8 k;

    w->curveNone = 0;
    w->curveVelY = 0;
    w->curveSpeed = 0;
    w->curveKind = kind;
    k = w->curveKind;
    switch (k) {
    case 2:
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
    case 14:
        w->curveVelY = 1;
        w->curveSpeed = 1;
        w->curveFrame = 0x4E;
        break;
    case 15:
        w->curveVelY = 1;
        w->curveSpeed = 1;
        w->curveFrame = 0x1E;
        break;
    case 0x0:
        break;
    }
    Player_ClearVelocity(em);
}

/* The first curve: nothing; returns 0. */
static s32 EmKhezu_CurveNone(Em *em, EmWork *work)
{
    return 0;
}

extern EmCurveKey *EmKhezu_VelYCurves[];

/*
 * The height curve: sets velY from the key pair around
 * EmKhezuWork.curveFrame. Returns 2 when the table ends (velY then falls 1
 * a frame, to -50 at most) or a negative key stops it (velY 0); 0
 * otherwise.
 */
static u8 EmKhezu_CurveVelY(Em *em, EmWork *work)
{
    EmKhezuWork *w = (EmKhezuWork *) work;
    EmCurveKey *key;
    f32 frame;
    f32 keyTime;
    s32 keyIndex;
    EmCurveKey *keys;
    u8 ended;

    keys = EmKhezu_VelYCurves[((EmKhezuWork *)work)->curveKind];
    ended = 0;
    if (keys == NULL) {
        return ended;
    }
    frame = w->curveFrame;
    if (frame != 0.0f && frame != 1.0f) {
        keyIndex = 1;
        while (keyIndex != 0) {
            key = &keys[keyIndex];
            keyTime = key->t;
            if (frame > keyTime && keyTime > 0.0f) {
                keyIndex++;
            } else if (keyTime == 0.0f) {
                ended = 2;
                if (em->velY > -50.0f) {
                    em->velY -= 1.0f;
                }
                break;
            } else if (keyTime < 0.0f) {
                w->curveFrame = 0;
                ended = 2;
                em->velY = 0.0f;
                break;
            } else {
                keyIndex = 0;
                em->velY = (key->v - key[-1].v) / ((keyTime - key[-1].t) / em->motion[0].step);
            }
        }
    }
    return ended;
}

extern EmCurveKey *EmKhezu_SpeedCurves[];

/*
 * The speed curve: sets velZ from the key pair around the curve frame.
 * Returns 4 when the table ends; 0 otherwise.
 */
static u8 EmKhezu_CurveSpeed(Em *em, EmWork *work)
{
    EmKhezuWork *w = (EmKhezuWork *) work;
    EmCurveKey *key;
    f32 frame;
    f32 keyTime;
    s32 keyIndex;
    EmCurveKey *keys;
    u8 ended;

    keys = EmKhezu_SpeedCurves[((EmKhezuWork *)work)->curveKind];
    ended = 0;
    if (keys == NULL) {
        return ended;
    }
    frame = w->curveFrame;
    if (frame != 0.0f && frame != 1.0f) {
        keyIndex = 1;
        while (keyIndex != 0) {
            key = &keys[keyIndex];
            keyTime = key->t;
            if (frame > keyTime && keyTime != 0.0f) {
                keyIndex++;
            } else if (keyTime == 0.0f) {
                ended = 4;
                break;
            } else {
                keyIndex = 0;
                em->velZ = (key->v - key[-1].v) / ((keyTime - key[-1].t) / em->motion[0].step);
            }
        }
    }
    return ended;
}

/*
 * Runs the started curves, advances the curve frame by the motion's step,
 * and in the air moves along rotY (Em_MoveRotatedAccel). Returns the ended
 * curves' bits (2 the height curve, 4 the speed curve).
 */
u8 EmKhezu_UpdateCurves(Em *em)
{
    EmKhezuWork *w = (EmKhezuWork *)&em->work;
    u8 ret = 0;

    if (w->curveNone != 0) {
        ret = EmKhezu_CurveNone(em, (EmWork *) w);
    }
    if (w->curveVelY != 0) {
        ret |= EmKhezu_CurveVelY(em, (EmWork *) w);
    }
    if ((u8)w->curveSpeed != 0) {
        ret |= EmKhezu_CurveSpeed(em, (EmWork *) w);
    }
    w->curveFrame += (s16) em->motion[0].step;
    if (em->stance == EM_STANCE_AIR) {
        w->accelAngX = 0;
        w->accelAngY = em->rotY;
        w->accelAngZ = 0;
        Em_MoveRotatedAccel(em, &w->accelAngX);
    }
    return ret;
}

/*
 * Finds the nearest (XZ) of the area's four ceiling points (the area's
 * point table 2) and writes the point after it, round the circuit, to
 * `out`. Returns the nearest one's index, or 0 with (1000, 0, 1000) when
 * the area has no table.
 */
u8 EmKhezu_NearestCeilingPoint(Em *em, f32 *out)
{
    f32 dist[4];
    f32 (*pts)[3];
    u32 i;
    f32 (*p)[3];
    f32 best;
    f32 t;
    u8 k;
    f32 *sel;

    pts = (f32 (*)[3]) Em_AreaEntryValue(em, em->tables[2]);
    if (pts == NULL) {
        out[0] = 1000.0f;
        out[1] = 0.0f;
        out[2] = 1000.0f;
        return 0;
    }
    p = pts;
    for (i = 0; i < 4; i++) {
        dist[i] = Vec_DistanceXZ(&em->posX, *p);
        p++;
    }
    best = dist[0];
    k = 0;
    t = dist[1];
    if (best > t) {
        best = t;
        k = 1;
    }
    t = dist[2];
    if (best > t) {
        best = t;
        k = 2;
    }
    t = dist[3];
    if (best > t) {
        k = 3;
    }
    sel = pts[(k + 1) & 3];
    out[0] = sel[0];
    out[1] = sel[1];
    out[2] = sel[2];
    return k;
}

/*
 * 1 when the area's day id is AREA_SWAMP_3, AREA_SWAMP_4 or AREA_SWAMP_8
 * (where the monster, out of the player's area, is given a ceiling:
 * EmKhezu_Update), 0 otherwise.
 */
s32 EmKhezu_InSpecialArea(u16 area)
{
    switch (Area_DayId(area)) {
    case AREA_SWAMP_3:
    case AREA_SWAMP_4:
    case AREA_SWAMP_8:
        return 1;
    }
    return 0;
}

/* The file's initialised data, in the game's order */
extern u32 EmCephadrome_VelYCurves_3[];
EmTurnParams EmKhezu_TurnParams[6] = {
#include "game/008C7B80.inc"
};
Vec3f EmKhezu_BaitThrowPt = {
#include "game/008C7C28.inc"
};
void *EmKhezu_Callbacks[8] = {
    EmKhezu_SpawnEffects, EmKhezu_SpawnEffects, EmKhezu_TableNop, EmKhezu_UpdateMotionEffects,
    EmKhezu_ScriptQuery, EmKhezu_Reinit, NULL, NULL,
};
EmCurveKey EmKhezu_MoveVelYCurves_0[8] = {
#include "game/008C7C60.inc"
};
EmCurveKey EmKhezu_MoveVelYCurves_1[20] = {
#include "game/008C7CA0.inc"
};
EmCurveKey EmKhezu_MoveVelYCurves_3[10] = {
#include "game/008C7D40.inc"
};
EmCurveKey EmKhezu_MoveVelYCurves_5[6] = {
#include "game/008C7D90.inc"
};
EmCurveKey EmKhezu_MoveVelYCurves_6[10] = {
#include "game/008C7DC0.inc"
};
EmCurveKey *EmKhezu_MoveVelYCurves[16] = {
    EmKhezu_MoveVelYCurves_0, EmKhezu_MoveVelYCurves_1, NULL, EmKhezu_MoveVelYCurves_3,
    NULL, EmKhezu_MoveVelYCurves_5, EmKhezu_MoveVelYCurves_6, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
};
EmCurveKey EmKhezu_VelYCurves_3[10] = {
#include "game/008C7E50.inc"
};
EmCurveKey EmKhezu_VelYCurves_5[14] = {
#include "game/008C7EA0.inc"
};
EmCurveKey EmKhezu_VelYCurves_8[8] = {
#include "game/008C7F10.inc"
};
EmCurveKey EmKhezu_VelYCurves_9[18] = {
#include "game/008C7F50.inc"
};
EmCurveKey EmKhezu_VelYCurves_10[14] = {
#include "game/008C7FE0.inc"
};
EmCurveKey EmKhezu_VelYCurves_11[4] = {
#include "game/008C8050.inc"
};
EmCurveKey EmKhezu_VelYCurves_12[8] = {
#include "game/008C8070.inc"
};
EmCurveKey EmKhezu_VelYCurves_13[24] = {
#include "game/008C80B0.inc"
};
EmCurveKey EmKhezu_VelYCurves_14[8] = {
#include "game/008C8170.inc"
};
EmCurveKey EmKhezu_VelYCurves_15[8] = {
#include "game/008C81B0.inc"
};
EmCurveKey *EmKhezu_VelYCurves[16] = {
    NULL, NULL, (EmCurveKey *)EmCephadrome_VelYCurves_3, EmKhezu_VelYCurves_3,
    NULL, EmKhezu_VelYCurves_5, NULL, NULL,
    EmKhezu_VelYCurves_8, EmKhezu_VelYCurves_9, EmKhezu_VelYCurves_10, EmKhezu_VelYCurves_11,
    EmKhezu_VelYCurves_12, EmKhezu_VelYCurves_13, EmKhezu_VelYCurves_14, EmKhezu_VelYCurves_15,
};
EmCurveKey EmKhezu_SpeedCurves_5[14] = {
#include "game/008C8230.inc"
};
EmCurveKey EmKhezu_SpeedCurves_8[8] = {
#include "game/008C82A0.inc"
};
EmCurveKey EmKhezu_SpeedCurves_9[14] = {
#include "game/008C82E0.inc"
};
EmCurveKey EmKhezu_SpeedCurves_10[12] = {
#include "game/008C8350.inc"
};
EmCurveKey EmKhezu_SpeedCurves_14[8] = {
#include "game/008C83B0.inc"
};
EmCurveKey EmKhezu_SpeedCurves_15[8] = {
#include "game/008C83F0.inc"
};
EmCurveKey *EmKhezu_SpeedCurves[16] = {
    NULL, NULL, NULL, NULL,
    NULL, EmKhezu_SpeedCurves_5, NULL, NULL,
    EmKhezu_SpeedCurves_8, EmKhezu_SpeedCurves_9, EmKhezu_SpeedCurves_10, NULL,
    NULL, NULL, EmKhezu_SpeedCurves_14, EmKhezu_SpeedCurves_15,
};
