/*
 * Monster family: Yian Kut-Ku, Gypceros and their subspecies
 * (EM_KIND_YIAN_KUT_KU, EM_KIND_BLUE_YIAN_KUT_KU, EM_KIND_GYPCEROS and
 * EM_KIND_PURPLE_GYPCEROS), from the family's first function,
 * EmKutku_MotionSounds, to the next family's. EmKutku_Init sets a monster up
 * and EmKutku_Start starts it.
 *
 * EmKutku_Update runs every frame: the rage and the Gypceros's crest glow,
 * the needs and low hp, the reaction to Em_UpdateDamage's result, the
 * jungle story scene or the script, then the current state through
 * EmKutku_RunState and one dispatcher per state group. The state functions
 * take the monster and its work area (EmFlyerWork, the flying monsters'
 * view of EmWork); Em.step is the step within the current state.
 * EmKutku_ChangeState does each state's entry work through one
 * EmKutku_Set...State function per group. The states and motions are named
 * in em_kutku.h. The two kinds share the code and differ where it tests the
 * kind: the Gypceros feigns death and flashes, the Kut-Ku flinches
 * from attack kind 5.
 *
 * After the states: the callbacks (gGmEmCallbackTbl's row,
 * EmKutku_Callbacks), the motions' effects and attack hit zones
 * (EmKutku_MotionEffects), the draw helper for the mesh sets, the flight
 * helpers (the escape point, the pitch) and the velocity curves the take-offs
 * and landings follow (EmKutku_StartCurves, EmKutku_UpdateCurves).
 */
#include "common.h"
#include "scene.h"
#include "quest.h"
#include "mine_point.h"
#include "em.h"
#include "em_kutku.h"
#include "objattach.h"
#include "player.h"
#include "item.h"
#include "area.h"
/* prototypes of callees */
void GmEm_ResetSize(Em *em);
void KutkuAttack_CreateFlash(Em *em, s8 kind);
void Em_EndStatus4(Em *em);
void Em_EndStatus5(Em *em);
void Em_StartGoalAction(Em *em, s32 arg1);
void Em_ResetNotice(Em *em);
void Em_BeginStatus2(Em *em);
void Em_RefillNeed3(Em *em);
void Em_EndStatus2(Em *em);
void Em_EndStatus1(Em *em);
void Em_BeginStatus4(Em *em);
void Em_EndAllStatus(Em *em);
void Em_BeginStatus5(Em *em);

/* A mining point (as in sub_main's mine_point.c). */
/* prototypes */
void EmKutku_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void PartFx_SpawnForEmScaled(Em *em, s32 a, s32 b, f32 scale);
s32 EmMotion_InFrameRange(void *em, s32 layer, f32 from, f32 to);
u16 Rand_Next(s32 stream);
void EmRangedAttack_CreateAtBone(Em *, s32, s32, s32, s32, s32);
s8 EmMine_CreateTimed0(Em *em, f32 *pos);
/* unit's own declaration: declared here as the callers use it; with the definition's void (Em *em, s16 n), the calls in EmKutku_EatNeed2 and EmKutku_EatNeed1 compile differently */
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
void KutkuAttack_Create(Em *, s8);
void Em_SelectSightRow(Em *em, u8 idx);
void Em_MoveRotated(Em *em, void *src);
void Em_MoveRotatedAccel(Em *em, void *ang);
void Em_SetStateRand(Em *em, s32 arg1, s32 arg2);
void Em_LoadReactionSet(Em *em, u8 k);
void Em_ComputeGoal2(Em *em, f32 *target);
void Em_GetPitchToGoal(Em *em, s32 *out, f32 a, f32 b);
s32 Em_GetTargetPos(Em *em, u8 idx, f32 *out);
void Em_ClearLowHpNeed(Em *em);
void Em_SpawnStatus4Effect(Em *em, s32 a);
void Em_SetHitReactMode(Em *em, u8 a);
void Em_SetTrapTimeGoal(Em *em);
/* unit's own declaration: declared here as the callers use it; with the definition's s32 (Em *em, s16 n, s16 max, s16 interval), the calls in EmKutku_Rest and EmKutku_Status2LieDown compile differently */
s32 Em_RegenHp(Em *em, s32 n, s32 max, s32 interval);
void Em_ClearPoison(Em *em);
void Em_SetHold2(Em *em);
void Em_SetCaptured(Em *em, s32 a);
void Em_PlaySeAtBoneOnLayerFrame(Em *em, s32 frame, s32 se, s32 bone, s32 flag);
void Em_UpdateFlight(Em *em, s32 flags, EmFlyParam *p);
s32 Em_TurnToTargetMotion(Em *em, u16 range, u16 motFront, u16 motLeft, u16 motRight);
s32 Em_StepToGoal(Em *em, u16 step, u8 flags);
u8 Em_TurnToGoal(Em *em, u16 step, u8 flags);
void Em_StartTurnToTarget(Em *em, EmTurnParams *t);
s32 Em_TurnStep(Em *em, EmTurnParams *t);
void Em_RestartScript(Em *em);
void EmKutku_Init(Em *);
void EmKutku_Land(Em *em, EmWork *work);
void EmKutku_FlyOutOfArea(Em *, EmWork *);
void EmKutku_FlyToGoalAndLand(Em *, EmWork *);
void EmKutku_FlyThroughArea(Em *em, EmWork *work);
void EmKutku_AirFlinch(Em *em, EmWork *work);
void EmKutku_FallFromAir(Em *em, EmWork *work);
void EmKutku_Part6Break(Em *, EmWork *);
void EmKutku_FeignDeathCarvable(Em *, EmWork *);
void EmKutku_JungleIntro(Em *em, EmWork *work);
void EmKutku_Captured(Em *em);
void EmKutku_VsEatBait(Em *, EmWork *);
void EmKutku_VsPoseA(Em *, EmWork *);
void EmKutku_VsPoseB(Em *, EmWork *);
void EmKutku_VsPoseC(Em *em, EmWork *work);
void EmKutku_Die(Em *em, EmWork *work);
void EmKutku_DieHeld(Em *em, EmWork *work);
void EmKutku_DieFalling(Em *em, EmWork *work);
void EmKutku_Reappear(Em *, EmWork *);
void EmKutku_RunIdleState(Em *em, EmWork *work);
void EmKutku_RunMoveState(Em *em, EmWork *work);
void EmKutku_RunAttackState(Em *em, EmWork *work);
/* The view of the work area the motion-effects function takes: the current
   motion at +2 (a real field, so MWCC does not keep its address). */
typedef struct EmKutkuFxWork {
    s16 unk000;         /* EmFlyerWork.fxStep and the byte after it */
    s16 motion;         /* 0x02 */
    u8 unk004[0x8 - 0x4];
    s8 crestGlow;       /* 0x08: EmFlyerWork.crestGlow, set while the Gypceros's crest glows */
    u8 unk009[0x1B - 0x9];
    u8 partWhole;       /* 0x1B: EmFlyerWork.partWhole */
} EmKutkuFxWork;
void EmKutku_MotionSounds(Em *em, struct EmWork *);
void EmKutku_MotionEffects(Em *, EmKutkuFxWork *);
void EmKutku_SpawnLandingDust(Em *em);
void EmKutku_SpawnDustCloudAEvery5(Em *em);
void EmKutku_EndAttack(Em *em, EmWork *work);
void EmKutku_MoveByVelYCurve(Em *em, s32 kind);
void EmKutku_StartCurves(Em *em, u8 kind);
u8 EmKutku_UpdateCurves(Em *em);
void AlertMark_Create(void *owner, s16 v, u8 k);
void DustCloud_Create(Em *em, s16 k, s16 c, f32 scale);
void Mtx_GetTrans(f32 *out, f32 *m);
void FireBlast_CreateOwned(f32 *pos, s16 k, f32 scale, void *owner);
/* unit's own declaration: declared here as the callers use it; with the definition's void (Em *em, u8 k, s16 v, s16 t, f32 y, f32 scale), the calls in EmKutku_Stun and EmKutku_StunTrapped compile differently */
void StunStars_CreateEm(Em *, s32, s32, s32, f32, f32);
s32 Sound_RandomSe(s32 id, s16 lo, s32 alt, s16 n);
s32 EmMotion_PassedFrameScaled(void *p, f32 frame, s32 layer);
s32 EmMotion_ReachedFrame(void *obj, s32 a, f32 frame);
f32 Vec_Distance(f32 *a, f32 *b);
void CamShake_ReqAIf(Em *obj, s32 i);
f32 Vec_DistanceXZ(f32 *a, f32 *b);
s32 Versus_BothDefeated(void);
s32 QuestEm_CanAppear(Em *em);
void Player_ClearVelocity(void *);
void Player_ClearAccel2(void *);
void Motion_InitTime(void *p, s16 step);
void Shadow_Spawn(void *obj, s16 type);
extern EmFlyParam EmKutku_FlyParam;
extern EmFlyParam EmKutku_FlyParam2;
extern EmTurnParams EmKutku_TurnParams[2];
s32 Save_TestEventFlag(s32);
/* unit's own declaration: declared here as the callers use it; with the definition's void (Em *em, s32 group, s32 state, u8 mode), the calls in EmKutku_ChangeState, EmKutku_SetIdleState and others compile differently */
void Em_SetStateSync(Em *em, u16 group, u16 state, u8 mode);
void Em_SetSpeedScaled(Em *em, u16 flags, f32 x, f32 y);
void Em_ComputeGoal(Em *em, f32 *target);
void Em_TypeSetup(Em *em);
void Em_SetTargetAngle(Em *em);
void Em_StartRage(Em *em);
void Em_StartPendingRage(Em *em);
s32 Game_IsOnline(void);
void Vec_Set(f32 *out, f32 x, f32 y, f32 z);
void EmKutku_SetIdleState(Em *em, u16 state, u8 mode);
void EmKutku_SetMoveState(Em *em, u16 state, u8 mode);
void EmKutku_SetFlyState(Em *em, u16 state, u8 mode);
void EmKutku_SetAttackState(Em *em, u16 state, u8 mode);
void EmKutku_SetReactState(Em *em, u16 state, u8 mode);
u16 Map_DayId(u16);
void EmKutku_SetFarthestEscapePoint(Em *em);
void Em_TargetPlayerInRange(Em *);
s16 Player_IsState(void *obj, u8 state, u8 sub);
void Versus_MarkDefeat(Em *);
s8 QuestEm_Appear(Em *em);
void VersusBait_Create(Player *p, Em *em, u8 k, s32 unused, f32 *pos);
void Versus_RouteEmByTeam(Em *em);
s32 Versus_DefeatCamera(Em *pl);
extern Vec3f EmKutku_CarveThrowPt __attribute__((aligned(8)));
extern EmCurveKey *EmKutku_MoveVelYCurves[];
void Em_SpawnLyingPuffs(Em *em, s32 bone, f32 *pos, f32 scale);
void QuestEm_Hunted(Em *em);
s8 EmMine_Create(Em *em, s32 idx);
s32 EmMine_Update(Em *em);
void EmMine_Remove(Em *em);
AreaRecord *Stage_GetAreaRecord(u16 x);
struct EmIdEntry;
f32 *Em_AreaEntryValue(Em *em, struct EmIdEntry *entry);
s32 Area_ContainsPos(s32 id, f32 *pos);
void Em_EnterNextArea(Em *em);
void Em_AreaExitPoint(Em *em, f32 *out);
void Hud_TriggerGlow(Em *em);
u16 Math_Heading(f32 x1, f32 z1, f32 x2, f32 z2);
void Em_BonePos(void *obj, s16 bone, f32 *out);
void EmKutku_AddMinePoint(Em *em);
EmBone *Em_Bone(void *obj, s16 bone);
void Mtx_Copy(f32 *dst, f32 *src);
void Vec_RotateSelf(f32 *v, f32 *m);
void HitSpark_SpawnForOwner(void *owner, s32 ang, s32 type, s32 sub, f32 *pos, f32 scale);
u16 Area_IdForCurrentTime(u16);
extern f32 gAreaGroundY[];
extern u16 EmKutku_FxFrameBone[4][2];
s32 Quest_GetOutcome(s32 withLoss);
s32 Player_HasSkill(Player *p, u8 skill);
u8 Pouch_Add(Player *p, u16 item, s16 n);
u8 Notice_Push(u8 kind, s16 a, s16 b);
extern u16 EmKutku_StealItems[];
/* A mesh of the model (0x4C bytes; the list at model +0x10). */
/* A set of meshes (0x8C bytes). */
typedef struct EmKutkuMeshSet {
    s32 unk0;
    s32 count;          /* 0x04 */
    s32 mesh[33];       /* 0x08 */
} EmKutkuMeshSet;
s32 Gs_SetState(u8 reg, u32 v);
u16 Area_DayId(u16 id);
void EmTail_Create(Em *);
void Em_SetPartScaled(Em *em, s32 bit);
void Em_SetMotionRow(Em *em, s32 row, s32 arg2, s32 arg3);
s32 Em_ComputeMaxHp(Em *em, s16 idx);
void Em_CheckCarryRequest(Em *em);
void EmKutku_RunState(Em *em, s32 arg1);
s32 EmKutku_IsFeigningDeath(Em *em);
s32 Em_UpdateCombatTimer(Em *em);
void Em_RaisePlayerAreaTriggerLax(Em *em);
void Em_CheckLowHp(Em *em, f32 ratio);
void Em_DecayNeed2(Em *em);
void Em_DecayNeed1Only(Em *em);
void Em_DecayNeed1(Em *em);
u8 Em_UpdateDamage(Em *em, u8 *out);
void Em_SetTrapTime(Em *em, u8 *p);
void Em_SetStatus1Timer(Em *em);
void Em_SetStatus2Timer(Em *em);
void Em_SetStatus4Timer(Em *em);
void Em_SetStatus5Timer(Em *em);
void Em_ResetKindTimer(Em *em);
s32 Em_IsHeld(Em *em);
void Em_CheckRageByHp(Em *em, s16 n);
void Em_BreakPart(Em *em, s32 part);
void Em_UpdatePendingScript(Em *em);
void Em_ClearHitReactMode(Em *em);
u8 Em_AllPlayersHere(Em *em);
void Area_ToWorld(s32 area, f32 *pos, f32 *out);
void Em_GetPitchToTarget(Em *em, s32 *out, f32 a, f32 b);
void EmKutku_EndSwoop(Em *em, s32 arg1);
void EmKutku_StepPitch(Em *em, s32 mode, f32 height);
s32 EmKutku_SwoopClawIfNear(Em *em);
/* end of prototypes */

/*
 * Plays the sounds on the frames of the current layer-0 motion
 * (work->sndMotion) through Em_PlaySeAtBoneOnFrame; motion 0x07 and
 * KUTKU_MOT_ALERT_IDLE have Gypceros-only sound sets, the second with random
 * cries (Sound_RandomSe).
 */
void EmKutku_MotionSounds(Em *em, EmWork *work)
{
    s16 motion;
    u8 kind;
    u8 emKind;

    motion = work->sndMotion;
    switch (motion) {
    case EM_MOTION_ID(0, KUTKU_MOT_WALK):
        Em_PlaySeAtBoneOnFrame(em, 0x34, 1, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x74, 1, 0x1A);
        break;
    case EM_MOTION_ID(0, 0x4):
        Em_PlaySeAtBoneOnFrame(em, 0x28, 6, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 6, 0x1A);
        break;
    case EM_MOTION_ID(0, 0x5):
        Em_PlaySeAtBoneOnFrame(em, 0x19, 1, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 1, 0x1A);
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_TURN_LEFT):
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
    case EM_MOTION_ID(0, KUTKU_MOT_LIMP):
        Em_PlaySeAtBoneOnFrame(em, 6, 0x59, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 6, 4, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 0, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 0x12, 0x14);
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_DESCEND):
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0xB, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x42, 0xB, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x7A, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x7E, 0xB, 0xC);
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_GLIDE):
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
    case EM_MOTION_ID(0, KUTKU_MOT_FLY):
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
    case EM_MOTION_ID(0, KUTKU_MOT_HOVER):
        Em_PlaySeAtBoneOnFrame(em, 4, 0xC, 6);
        Em_PlaySeAtBoneOnFrame(em, 8, 0xC, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 0xB, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x8E, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x8A, 0xB, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0xD4, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0xD8, 0xB, 0xC);
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_RUN):
        Em_PlaySeAtBoneOnFrame(em, 8, 0x4F, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 6, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 6, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 0x51, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x52, 0x51, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0x18, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x54, 0x18, 0x23);
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_TAKE_OFF):
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0xD, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x52, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x56, 0xB, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x88, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x8C, 0xB, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0xB6, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0xBA, 0xB, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 7, 0x1A);
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_TOUCH_DOWN):
        Em_PlaySeAtBoneOnFrame(em, 2, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 6, 0xB, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 4, 1, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 9, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x62, 3, 0x14);
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_IDLE_ACTION_C):
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 2, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0xE, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 8, 0xE, 6);
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_REST):
        kind = em->type;
        if ((kind == EM_KIND_GYPCEROS) || (kind == EM_KIND_PURPLE_GYPCEROS)) {
            Em_PlaySeAtBoneOnFrame(em, 2, 0x55, 0x23);
            Em_PlaySeAtBoneOnFrame(em, 0xB6, 0x55, 0x23);
            Em_PlaySeAtBoneOnFrame(em, 0x16C, 0x55, 0x23);
            break;
        }
        Em_PlaySeAtBoneOnFrame(em, 2, 0x55, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x7E, 0x38, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xB6, 0x55, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x16C, 0x55, 0x23);
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_REST_LIE_DOWN):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x57, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x8E, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x17, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x13, 0);
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_REST_GET_UP):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x57, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0x2D, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x7E, 3, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0xB6, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0x16, 0);
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_LIE):
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0x57, 0x23);
        break;
    case EM_MOTION_ID(0, 0x23):
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
        Em_PlaySeAtBoneOnFrame(em, 2, 0x4B, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0x21, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x80, 0x22, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xB8, 0x24, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x120, 0x22, 0x23);
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_STARE):
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0x4C, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0x22, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 0, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0x15, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0xC, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0xC, 6);
        Em_PlaySeAtBoneOnFrame(em, 0xA6, 0, 0x1A);
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_GET_UP):
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x48, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0xC, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 2, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 1, 0x1A);
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_COLLAPSE):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x57, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 0xA4, 9, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x90, 3, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x88, 0xE, 6);
        Em_PlaySeAtBoneOnFrame(em, 0xB2, 4, 0x22);
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_LIE_DOWN):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x57, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 2, 0x17, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 3, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x66, 0x10, 0x1A);
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_LEAP):
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 0xD, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x66, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 0xD, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x88, 0xA, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x8C, 0xA, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x8C, 0x27, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 2, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 4, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 5, 0x1A);
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
    case EM_MOTION_ID(0, KUTKU_MOT_LEAP_LAND):
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x27, 0x23);
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
    case EM_MOTION_ID(0, KUTKU_MOT_TAIL_SPIN):
        Em_PlaySeAtBoneOnFrame(em, 8, 0x4B, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0xF, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x14, 0x2A);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 0, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 1, 0x1A);
        break;
    case EM_MOTION_ID(0, 0x2B):
        Em_PlaySeAtBoneOnFrame(em, 8, 0x4B, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0xF, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x14, 0x2A);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 1, 0x14);
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_SWOOP_DIVE):
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 0xB, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 6, 0x1A, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x1B, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0x1B, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 0x1A, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 0x1B, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x1B, 6);
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_SWOOP_CLAW):
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0x4F, 0x23);
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
    case EM_MOTION_ID(0, KUTKU_MOT_SPIT):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 0x12, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0x4F, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 0x12, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0x18, 0x23);
        break;
    case EM_MOTION_ID(0, 0x30):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x22, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 0x22, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 0x22, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x72, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 1, 0x14);
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_ALERT_IDLE):
        emKind = em->type;
        if ((emKind == EM_KIND_GYPCEROS) || (emKind == EM_KIND_PURPLE_GYPCEROS)) {
            Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0);
            Em_PlaySeAtBoneOnFrame(em, 0x20, 0x20, 0x23);
            Em_PlaySeAtBoneOnFrame(em, 0x58, 0x38, 0x22);
            Em_PlaySeAtBoneOnFrame(em, 0x92, 0x38, 0x22);
            Em_PlaySeAtBoneOnFrame(em, 0xC6, 0x38, 0x22);
            Em_PlaySeAtBoneOnFrame(em, 0x44, Sound_RandomSe(0x32, 4, 0x33, 4), 0x23);
            Em_PlaySeAtBoneOnFrame(em, 0xB6, Sound_RandomSe(0x34, 4, 0x33, 4), 0x23);
            Em_PlaySeAtBoneOnFrame(em, 0x88, Sound_RandomSe(0x34, 4, 0x32, 4), 0x23);
            break;
        }
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0x12, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 6, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x96, 0x28, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xFC, 0x4F, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xEE, 0x16, 0);
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_BACK_HOP):
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0x28, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 0x4F, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 6, 0x14, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 0xF, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 0xF, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x6E, 0xB, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0xAC, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0xB0, 0xB, 0xC);
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_TURN_IN_PLACE):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x12, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 4, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 4, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 5, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x78, 4, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x108, 4, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x14, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 0x28, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x84, 0x4F, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x84, 0x29, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0xC, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 0xC, 6);
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_BACK_HOP_LAND):
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x12, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 8, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 2, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 4, 0xD, 0xC);
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_FEED):
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 6, 0x22, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x68, 3, 0x1A);
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_FLINCH):
        Em_PlaySeAtBoneOnFrame(em, 6, 0x53, 0x23);
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
    case EM_MOTION_ID(0, KUTKU_MOT_FLINCH_E):
        Em_PlaySeAtBoneOnFrame(em, 6, 0x53, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x12, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 6, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 4, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0xAC, 3, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x6E, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x5A, 0x15, 0);
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_SHAKE_OFF):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x4F, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x56, 0x1F, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 4, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 8, 0xD, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x62, 0x13, 0x2A);
        Em_PlaySeAtBoneOnFrame(em, 0x7E, 3, 0x1A);
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_FLINCH_C):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x52, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 0x1F, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0, 0x1A);
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_FLINCH_D):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x52, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x76, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x7A, 0, 0x1A);
        break;
    case EM_MOTION_ID(0, 0x41):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x54, 0x23);
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
    case EM_MOTION_ID(0, KUTKU_MOT_FLINCH_B):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x52, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x13, 0);
        break;
    case EM_MOTION_ID(0, 0x43):
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0x55, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0x17, 0);
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_DIE):
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 2, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 4, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0xDC, 0x10, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x108, 9, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x108, 7, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x114, 0x17, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x180, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0x52, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x8C, 0x28, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xB4, 0x2C, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x13A, 0x2F, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x192, 0x2B, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x174, 0xF, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x176, 0xE, 0xC);
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_KNOCK_DOWN):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x52, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x13, 0x2A);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 9, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0x11, 0);
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_DOWN):
    case EM_MOTION_ID(0, KUTKU_MOT_DOWN_B):
        Em_PlaySeAtBoneOnFrame(em, 8, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 0x2A, 0x23);
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_DOWN_GET_UP):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 1, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 1, 0x1A);
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_DOWN_GET_UP_B):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 1, 0x14);
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_KNOCK_DOWN_B):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x52, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x13, 0x2A);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 9, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0x11, 0);
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_STATUS4):
        Em_PlaySeAtBoneOnFrame(em, 0x10, 0x5F, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 1, 0x1A);
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_FALL):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x53, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0xB, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 0x14, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 0x1C, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 0x1B, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x5C, 0x1B, 0xC);
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_FALL_LAND):
        Em_PlaySeAtBoneOnFrame(em, 0xA, 9, 0);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 7, 0x2A);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x11, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x2C, 0x23);
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_FORAGE):
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x24, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0x22, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x66, 0x22, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xA4, 0x22, 0x23);
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_IDLE_PAIR_A):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x16, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 0x17, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 0x55, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xC8, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 3, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x140, 3, 0x1A);
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_IDLE_PAIR_B):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x16, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 0x17, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 0x55, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xC8, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x140, 3, 0x14);
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_DIE_FALLING):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0);
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_HOP_IN_PLACE):
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
    case EM_MOTION_ID(0, KUTKU_MOT_SWOOP):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x1A, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 8, 0x1B, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x1B, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 0x1C, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 0x1C, 0x22);
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_LEVEL_OUT):
        Em_PlaySeAtBoneOnFrame(em, 0x20, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0xD, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 0xD, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x7C, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x80, 0xB, 0xC);
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_CHARGE_TRIP):
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0x35, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 6, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 6, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0xB0, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0xD6, 3, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x108, 1, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x130, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x11, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 9, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x5C, 0x10, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x8C, 0x11, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 0xF, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0xF, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x114, 0xC, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x118, 0xC, 0xC);
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_TRAPPED):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x10, 0);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x12, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 9, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x4F, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x2A, 0x23);
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_STRUGGLE):
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0x12, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 0x12, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0xC, 6);
        Em_PlaySeAtBoneOnFrame(em, 8, 0xC, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x50, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0);
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_DOWNED):
        Em_PlaySeAtBoneOnFrame(em, 0x22, 0x16, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 0x16, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x62, 4, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 3, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x52, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x2B, 0x23);
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_DOWNED_GET_UP):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 6);
        Em_PlaySeAtBoneOnFrame(em, 8, 0x16, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 3, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x50, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0x20, 0x23);
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_THREAT):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 0x4F, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 0xD, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x92, 0x34, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xB2, 0x33, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xD2, 0x34, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 0x12, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 0, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x84, 0, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x8E, 6, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0xBA, 6, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0xDC, 6, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0xE0, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0xFC, 0, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x78, 0x38, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x98, 0x38, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0xBA, 0x38, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0xDE, 0x38, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x120, 0x38, 0x22);
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_TURN_TO_GOAL):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x4F, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 8, 0xD, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x144, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0xD, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x36, 6, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x36, 7, 0x14);
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_PECK_COMBO):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x5C, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 0x32, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x33, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0x32, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x5A, 0x33, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 5, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 0x12, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 1, 0x1A);
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_WALK_B):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x5E, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 0x32, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x68, 0x33, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 2, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 6, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 6, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 6, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 0xD, 0xC);
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_SPIT_VOLLEY):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 6, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 6, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 0x51, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 0x18, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x8C, 0x51, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x94, 0x18, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x102, 4, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x11A, 6, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0xE8, 0x51, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xEE, 0x18, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x120, 0x51, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x126, 0x18, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x186, 1, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x178, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x190, 0, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x1A4, 0, 0x1A);
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_PECK_COMBO_B):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x12, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0x4C, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x32, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 0x33, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x60, 0x32, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x74, 0x33, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 8, 6, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 6, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x5A, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0xC, 0xC);
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_FLASH):
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0x5E, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x8C, 0x24, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 0x39, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 0x3A, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0xD, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x8C, 0xC, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x90, 0xC, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 5, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 5, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x86, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0xA6, 1, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0xC2, 0, 0x1A);
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_ATTACK5_FLINCH):
        Em_PlaySeAtBoneOnFrame(em, 0, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0x12, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0x54, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x19C, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xC8, 0x37, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x130, 0x16, 0x22);
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_FEIGN_DEATH_RISE):
        Em_PlaySeAtBoneOnFrame(em, 0x10, 0x5E, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x36, 0x27, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xB6, 0x23, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x17, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x88, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 0x14, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xA2, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0xCC, 6, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0xDA, 6, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 0xC, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x54, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 0xAC, 0xD, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0xB0, 0xD, 6);
        
    case EM_MOTION_ID(0, KUTKU_MOT_IDLE):
    default:
        break;
    }
}

/*
 * Empty per-kind setup hook, called by the per-kind dispatch Em_TypeSetup.
 */
void EmKutku_Setup(Em *em)
{
}

/*
 * Kind callback slots 0 and 1 (EmKutku_Callbacks): creates the alert mark
 * (AlertMark_Create 0x22) and effect 01 type 0 on the monster; same as
 * EmRathalos/EmKhezu_SpawnEffects.
 */
void EmKutku_SpawnEffects(void *em)
{
    AlertMark_Create(em, 0x22, 0);
    Shadow_Spawn((Player *) em, 0);
}

/*
 * Scales parts 3, 4 and 8; on quest stage 0 places the monster by the area
 * (Area_DayId) with 1500 units of x per slotNo; idle motion,
 * EM_STANCE_GROUND, hp from Em_ComputeMaxHp, flight turn/pitch/roll speeds,
 * no mine point; Kut-Ku kinds (6, 0x26) hide mesh 1 and create stage object
 * K09, unless fxFlags bit 0 was set.
 */
void EmKutku_Init(Em *em)
{
    EmFlyerWork *flyWork = (EmFlyerWork *)&em->work;
    s16 maxHp;

    Em_SetPartScaled(em, 3);
    Em_SetPartScaled(em, 4);
    Em_SetPartScaled(em, 8);
    if (gQuest.stage == 0) {
        em->rotY = 0;
        switch (Area_DayId(gScene.area)) {
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
        case AREA_FOREST_HILLS_3:
            em->posX = 17300.0f;
            em->posY = 0.0f;
            em->posZ = 18000.0f;
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
    Em_SetMotionRow(em, KUTKU_MOT_IDLE, 0, 0);
    em->stance = EM_STANCE_GROUND;
    flyWork->fxMotion = 0;
    flyWork->partWhole = 1;
    flyWork->crestGlow = 0;
    flyWork->minePoint = -1;
    maxHp = Em_ComputeMaxHp(em, 0);
    em->hp = maxHp;
    em->hpMax = maxHp;
    em->scriptPending = 1;
    em->blind = 0;
    em->unk_InitFlag = 1;
    em->noAlert = 0;
    em->painted = 0;
    em->aimAngle = 0x4000;
    flyWork->pitchStep = 0x100;
    ((EmWork *)flyWork)->turnSpeed = 0x200;
    flyWork->rollSpeed = 0x100;
    flyWork->moveAngBY = 0x2000;
    flyWork->moveAngBX = 0;
    if ((em->fxFlags &= 1) == 0) {
        switch (em->type) {
        case EM_KIND_YIAN_KUT_KU:
        case EM_KIND_BLUE_YIAN_KUT_KU:
            em->meshShown[1] = 0;
            EmTail_Create(em);
            break;
        case EM_KIND_GYPCEROS:
        case EM_KIND_PURPLE_GYPCEROS:
            break;
        }
    }
}

/*
 * Spawn entry (the monster task): Init, then KUTKU_IDLE.
 */
void EmKutku_Start(Em *em)
{
    EmKutku_Init(em);
    EmKutku_ChangeState(em, EM_GROUP_IDLE, KUTKU_IDLE, EM_SYNC_LOCAL);
}

/*
 * Kind callback slot 5: Init again without changing state.
 */
void EmKutku_Reinit(Em *em)
{
    EmKutku_Init(em);
}

/*
 * Asks the script for the next state (scriptPending) and rests in KUTKU_IDLE
 * on the ground or KUTKU_FLY_HOVER otherwise.
 */
void EmKutku_ToIdle(Em *em)
{
    em->scriptPending = 1;
    if (em->stance == EM_STANCE_GROUND) {
        EmKutku_ChangeState(em, EM_GROUP_IDLE, KUTKU_IDLE, EM_SYNC_LOCAL);
        return;
    }
    EmKutku_ChangeState(em, EM_GROUP_FLY, KUTKU_FLY_HOVER, EM_SYNC_LOCAL);
}

/*
 * The ordinary end of a state: EmKutku_ToIdle; both extra arguments are
 * unused.
 */
void EmKutku_EndState(Em *em, s32 unused1, s32 unused2)
{
    EmKutku_ToIdle(em);
}

/*
 * Ends the state; with a 10% chance (stateRand % 100 < 10) it then cancels
 * the script request and chains KUTKU_ATK_SPIT_AFTER_FLINCH (Gypceros, or a
 * calm Kut-Ku) or 0x1F (an enraged Kut-Ku). Used at the end of several
 * group-3/4 states.
 */
void EmKutku_EndStateOrSpit(Em *em, s32 unused1, s32 unused2)
{
    u8 kind;

    if (((s32) em->stateRand % 100) < 0xA) {
        EmKutku_EndState(em, 0, 0);
        em->scriptPending = 0;
        kind = em->type;
        if ((kind == EM_KIND_GYPCEROS) || (kind == EM_KIND_PURPLE_GYPCEROS)) {
            EmKutku_ChangeState(em, EM_GROUP_ATTACK, KUTKU_ATK_SPIT_AFTER_FLINCH, EM_SYNC_SEND);
            return;
        }
        if (em->rage != 0) {
            EmKutku_ChangeState(em, EM_GROUP_ATTACK, KUTKU_ATK_SPIT_VOLLEY_AFTER_FLINCH, EM_SYNC_SEND);
            return;
        }
        EmKutku_ChangeState(em, EM_GROUP_ATTACK, KUTKU_ATK_SPIT_AFTER_FLINCH, EM_SYNC_SEND);
        return;
    }
    EmKutku_EndState(em, unused1, unused2);
}

/*
 * Ends a state in the air: EM_STANCE_AIR, scriptPending, motionSpeed 1, then
 * KUTKU_FLY_HOVER_ONCE (glide 0: starts in the hover motion) or 0x0F (glide
 * 1: starts in the glide motion KUTKU_MOT_GLIDE).
 */
void EmKutku_EndStateInAir(Em *em, u8 glide)
{
    s32 glide2;

    glide2 = glide & 0xFF;
    em->stance = EM_STANCE_AIR;
    em->scriptPending = 1;
    em->motionSpeed = 1.0f;
    switch (glide2) {
    case 0:
        EmKutku_ChangeState(em, EM_GROUP_FLY, KUTKU_FLY_HOVER_ONCE, EM_SYNC_LOCAL);
        break;
    case 1:
        EmKutku_ChangeState(em, EM_GROUP_FLY, KUTKU_FLY_GLIDE_ONCE, EM_SYNC_LOCAL);
        break;
    }
}

/*
 * (carcass gathering point, table 0x175, 3 gathers, at bone 2 on the floor;
 * stores its index in minePoint).
 */
void EmKutku_AddMinePoint(Em *em)
{
    f32 pos[3];
    MinePoint point;

    point.radius = 333.0f;
    point.table = 0x175;
    point.count = 3;
    point.action = MINE_ACTION_CARVE;
    point.area = (u8) em->area;
    point.flags = 1;
    point.timer = 0;
    Em_BonePos(em, 2, pos);
    pos[1] = em->floorY;
    em->minePoint = MinePoint_AddUntimed(&point, pos, 0);
}

/*
 * only for the local player, nothing once the quest has an outcome
 * (Quest_GetOutcome(1) != 0) or with SKILL_THEFT_GUARD; picks (stateRand) a pouch
 * slot holding one of the first 9 EmKutku_StealItems entries, else any item
 * whose ItemData type is 0 and rarity below 4; removes one and pushes notice
 * (1, 6, item). Called by EmKutku_PeckSteal on contact.
 */
void EmKutku_StealItem(Em *em, Player *player)
{
    s32 slots[20];
    s32 count;
    s32 i;
    s32 j;
    ItemData *itemData;
    s16 item;

    if (player->index != gScene.localPlayer) {
        return;
    }
    if (Quest_GetOutcome(1) != 0) {
        return;
    }
    if (Player_HasSkill(player, SKILL_THEFT_GUARD) == 1) {
        return;
    }
    count = 0;
    for (i = 0; i < 20; i++) {
        if (player->pouch[i].item != 0) {
            for (j = 0; j < 9; j++) {
                if (player->pouch[i].item == EmKutku_StealItems[j]) {
                    slots[count] = i;
                    count++;
                }
            }
        }
    }
    if (count == 0) {
        for (i = 0; i < 20; i++) {
            if (player->pouch[i].item != 0) {
                itemData = &gItemData[player->pouch[i].item];
                if (itemData->type == 0 && itemData->rarity < 4) {
                    slots[count] = i;
                    count++;
                }
            }
        }
        if (count == 0) {
            return;
        }
    }
    item = player->pouch[slots[em->stateRand % count]].item;
    Pouch_Add(player, item, -1);
    if (player->index == gScene.localPlayer) {
        Notice_Push(1, 6, item);
    }
}

/*
 * When the given layer (0-2) has stopped playing, restarts the idle motion
 * of that layer (EM_MOTION_ID(layer, 1)); lets a gesture play on one layer
 * while the others idle.
 */
void EmKutku_KeepLayerIdle(Em *em, s32 layer)
{
    if (em->motion[layer].playing == 0) {
        switch (layer) {
        case 0:
            Em_SetLayerMotion(em, EM_MOTION_ID(0, KUTKU_MOT_IDLE), 10, 0, 0);
            break;
        case 1:
            Em_SetLayerMotion(em, EM_MOTION_ID(1, KUTKU_MOT_IDLE), 10, 0, 1);
            break;
        case 2:
            Em_SetLayerMotion(em, EM_MOTION_ID(2, KUTKU_MOT_IDLE), 10, 0, 2);
            break;
        }
    }
}

/*
 * Puts the idle motion on all three layers (lookAtTarget off) and asks the
 * script for the next state when em->timer runs out.
 */
void EmKutku_IdleStand(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        if (em->motionId[0] != EM_MOTION_ID(0, KUTKU_MOT_IDLE)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(0, KUTKU_MOT_IDLE), 0xA, 0, 0);
        }
        if (em->motionId[1] != EM_MOTION_ID(0, KUTKU_MOT_TURN_TO_GOAL)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(1, KUTKU_MOT_IDLE), 0xA, 0, 1);
        }
        if (em->motionId[2] != EM_MOTION_ID(1, KUTKU_MOT_IDLE)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(2, KUTKU_MOT_IDLE), 0xA, 0, 2);
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
 * One loop of the idle motion (EM_STANCE_GROUND), then EndState once the
 * local console runs the monster (remote == 0).
 */
void EmKutku_Idle(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUTKU_MOT_IDLE, 0, 0);
        break;
    case 1:
        if ((em->remote == 0) && (em->motion[0].playing == 0)) {
            em->step++;
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * Plays motion KUTKU_MOT_PECK_COMBO_B once (the row KUTKU_ATK_PECK_COMBO_B
 * also plays), then EndState.
 */
void EmKutku_IdleActionA(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUTKU_MOT_PECK_COMBO_B, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->lookAtTarget = 0;
            em->step++;
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * Plays motion KUTKU_MOT_PECK_COMBO once (the row of attacks 0x0A and
 * 0x0B_Steal), then EndState.
 */
void EmKutku_IdleActionB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUTKU_MOT_PECK_COMBO, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * One idle loop on layer 0, keeping layers 1 and 2 idling; like KUTKU_IDLE
 * but without the remote check.
 */
void EmKutku_IdleOneLoop(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUTKU_MOT_IDLE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
    EmKutku_KeepLayerIdle(em, 1);
    EmKutku_KeepLayerIdle(em, 2);
}

/*
 * Gesture KUTKU_MOT_UPPER_A on layer 1 (layers 0 and 2 kept idle) with sound 0x2F at
 * layer-1 frame 0x20; ends when layer 1 stops.
 */
void EmKutku_IdleUpperA(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUTKU_MOT_UPPER_A, 0, 0);
        Em_SetLayerMotion(em, EM_MOTION_ID(0, KUTKU_MOT_IDLE), 0xA, 0, 0);
        Em_SetLayerMotion(em, EM_MOTION_ID(2, KUTKU_MOT_IDLE), 0xA, 0, 2);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
    EmKutku_KeepLayerIdle(em, 0);
    EmKutku_KeepLayerIdle(em, 2);
    Em_PlaySeAtBoneOnLayerFrame(em, 0x20, 0x2F, 0x23, 1);
}

/*
 * Gesture KUTKU_MOT_UPPER_B on layer 1 with sounds 0x20 and 0x17 at frames 0x20 and 0xA0;
 * ends when layer 1 stops.
 */
void EmKutku_IdleUpperB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUTKU_MOT_UPPER_B, 0, 0);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
    EmKutku_KeepLayerIdle(em, 0);
    EmKutku_KeepLayerIdle(em, 2);
    Em_PlaySeAtBoneOnLayerFrame(em, 0x20, 0x20, 0x23, 1);
    Em_PlaySeAtBoneOnLayerFrame(em, 0xA0, 0x17, 0x23, 1);
}

/*
 * Gesture KUTKU_MOT_UPPER_B on layer 1 like IdleUpperB, but leaves lookAtTarget as it is
 * and plays no sounds.
 */
void EmKutku_IdleUpperBQuiet(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, KUTKU_MOT_UPPER_B, 0, 0);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
    EmKutku_KeepLayerIdle(em, 0);
    EmKutku_KeepLayerIdle(em, 2);
}

/*
 * Plays motion KUTKU_MOT_IDLE_PAIR_A or 0x51 (stateRand bit 0) once, then
 * EndState.
 */
void EmKutku_IdlePairA(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        if (em->stateRand & 1) {
            Em_SetMotion(em, KUTKU_MOT_IDLE_PAIR_A, 0, 0);
            break;
        }
        Em_SetMotion(em, KUTKU_MOT_IDLE_PAIR_B, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * Motion 0x65 with aimAngle set toward the goal; snaps rotY onto it once
 * within 0x200; past frame 60 waits for layer 1 to stop, then EndState.
 */
void EmKutku_IdleTurnToGoal(Em *em)
{
    u16 aimAngle;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUTKU_MOT_TURN_TO_GOAL, 0, 0);
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
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * Gesture KUTKU_MOT_UPPER_C on layer 1 with sounds 0x21 and 0x1F at frames 0x38 and 0x68;
 * ends when layer 1 stops.
 */
void EmKutku_IdleUpperC(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUTKU_MOT_UPPER_C, 0, 0);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
    EmKutku_KeepLayerIdle(em, 0);
    EmKutku_KeepLayerIdle(em, 2);
    Em_PlaySeAtBoneOnLayerFrame(em, 0x38, 0x21, 0x23, 1);
    Em_PlaySeAtBoneOnLayerFrame(em, 0x68, 0x1F, 0x23, 1);
}

/*
 * Plays motion KUTKU_MOT_IDLE_ACTION_C once on layer 0, then EndState.
 */
void EmKutku_IdleActionC(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUTKU_MOT_IDLE_ACTION_C, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * Gesture KUTKU_MOT_UPPER_D on layer 1 with sounds 0x20 and 0x1F at frames 2 and 0x4A;
 * ends when layer 1 stops.
 */
void EmKutku_IdleUpperD(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUTKU_MOT_UPPER_D, 0, 0);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
    EmKutku_KeepLayerIdle(em, 0);
    EmKutku_KeepLayerIdle(em, 2);
    Em_PlaySeAtBoneOnLayerFrame(em, 2, 0x20, 0x23, 1);
    Em_PlaySeAtBoneOnLayerFrame(em, 0x4A, 0x1F, 0x23, 1);
}

/*
 * No flinch, sight row KUTKU_MOT_IDLE and reaction set 1; forage motion
 * KUTKU_MOT_FORAGE with dust clouds and sounds at its frames, then the feed
 * motion KUTKU_MOT_FEED and +5% hpMax; restores row/set 0 and ends. Adds
 * 0xDE to need2Gauge every frame (capped at need2Max).
 */
void EmKutku_EatNeed2(Em *em)
{
    s32 need2Max;

    Em_SetHitReactMode(em, EM_HIT_REACT_NO_FLINCH);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUTKU_MOT_FORAGE, 0, 0);
        Em_SelectSightRow(em, 1);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if ((EmMotion_PassedFrameScaled(em, 56.0f, 0U) == 0) && (EmMotion_PassedFrameScaled(em, 108.0f, 0U) == 0) && (EmMotion_PassedFrameScaled(em, 126.0f, 0U) == 0) && (EmMotion_PassedFrameScaled(em, 138.0f, 0U) == 0)) {
            if (EmMotion_PassedFrameScaled(em, 170.0f, 0U) != 0) {
                goto block_12;
            }
        } else {
block_12:
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
            Em_SetMotion(em, KUTKU_MOT_FEED, 0, 0);
            Em_AddHp(em, (s32) (0.05f * (f32) em->hpMax));
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->hitReactMode = 0;
            Em_SelectSightRow(em, 0);
            Em_LoadReactionSet(em, 0);
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
    em->need2Gauge += 0xDE;
    need2Max = em->need2Max;
    if (need2Max < em->need2Gauge) {
        em->need2Gauge = need2Max;
    }
}

/*
 * Motion 0x50, idle, motion KUTKU_MOT_IDLE_PAIR_B, idle (each waiting for
 * the blend to settle), then EndState.
 */
void EmKutku_IdlePairB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUTKU_MOT_IDLE_PAIR_A, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUTKU_MOT_IDLE, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].blend == 0) {
            em->step++;
            Em_SetMotion(em, KUTKU_MOT_IDLE_PAIR_B, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUTKU_MOT_IDLE, 0, 0);
        }
        break;
    case 4:
        if (em->motion[0].blend == 0) {
            em->step++;
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * The same as IdlePairB with KUTKU_MOT_IDLE_PAIR_B first and 0x50 second.
 */
void EmKutku_IdlePairC(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUTKU_MOT_IDLE_PAIR_B, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUTKU_MOT_IDLE, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].blend == 0) {
            em->step++;
            Em_SetMotion(em, KUTKU_MOT_IDLE_PAIR_A, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUTKU_MOT_IDLE, 0, 0);
        }
        break;
    case 4:
        if (em->motion[0].blend == 0) {
            em->step++;
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * The combat idle (EmKutku_SetIdleState turns KUTKU_IDLE into it while
 * combat == 1): motion KUTKU_MOT_ALERT_IDLE once, then EndState.
 */
void EmKutku_IdleAlert(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUTKU_MOT_ALERT_IDLE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * Wakes on a hit (EM_HIT_REACT_WAKE), blind; lies down
 * (KUTKU_MOT_REST_LIE_DOWN) and rests (KUTKU_MOT_REST) for 5400 frames
 * (Kut-Ku) or 9000 (Gypceros), regenerating hp up to 35% of hpMax every 9
 * frames (Kut-Ku) or 40% every 5 (Gypceros); then clears the low-hp need and
 * goes to KUTKU_IDLE_REST_END.
 */
void EmKutku_Rest(Em *em)
{
    u8 kind;
    u8 emKind;

    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUTKU_MOT_REST_LIE_DOWN, 0, 0);
        Em_LoadReactionSet(em, 1);
        kind = em->type;
        if ((kind == EM_KIND_YIAN_KUT_KU) || (kind == EM_KIND_BLUE_YIAN_KUT_KU)) {
            em->timer = 0x1518;
            break;
        }
        em->timer = 0x2328;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUTKU_MOT_REST, 0, 0);
        }
        break;
    case 2:
        emKind = em->type;
        if ((emKind == EM_KIND_YIAN_KUT_KU) || (emKind == EM_KIND_BLUE_YIAN_KUT_KU)) {
            Em_RegenHp(em, 1, (s32) (0.35f * (f32) em->hpMax), 9);
        } else {
            Em_RegenHp(em, 1, (s32) (0.4f * (f32) em->hpMax), 5);
        }
        if (--em->timer <= 0) {
            em->step++;
            Em_ClearLowHpNeed(em);
            EmKutku_ChangeState(em, EM_GROUP_IDLE, KUTKU_IDLE_REST_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmKutku_ChangeState(em, EM_GROUP_IDLE, KUTKU_IDLE_REST_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * No flinch, sight row KUTKU_MOT_IDLE, reaction set 1; forage motion
 * KUTKU_MOT_FORAGE, and at its frame 200 hands over to KUTKU_IDLE_EAT.
 */
void EmKutku_EatStart(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_NO_FLINCH);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUTKU_MOT_FORAGE, 0, 0);
        Em_SelectSightRow(em, 1);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 200.0f, 0U) != 0) {
            em->step++;
            EmKutku_ChangeState(em, EM_GROUP_IDLE, KUTKU_IDLE_EAT, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmKutku_ChangeState(em, EM_GROUP_IDLE, KUTKU_IDLE_EAT, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * Restores hitReactMode, sight row and reaction set 0; lets the forage
 * motion finish, plays the feed motion KUTKU_MOT_FEED, then EndState (no hp
 * or gauge refill; EmKutku_SetIdleState starts a goal action on entry).
 */
void EmKutku_Eat(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->hitReactMode = 0;
        Em_SelectSightRow(em, 0);
        Em_LoadReactionSet(em, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUTKU_MOT_FEED, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * Wakes on a hit, blind; collapses (KUTKU_MOT_COLLAPSE) and lies
 * (KUTKU_MOT_LIE) until em->timer (set by Em_SetStatus2Timer) runs out, then
 * KUTKU_IDLE_STATUS2_END.
 */
void EmKutku_Status2Collapse(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUTKU_MOT_COLLAPSE, 0, 0);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUTKU_MOT_LIE, 0, 0);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            EmKutku_ChangeState(em, EM_GROUP_IDLE, KUTKU_IDLE_STATUS2_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmKutku_ChangeState(em, EM_GROUP_IDLE, KUTKU_IDLE_STATUS2_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * Wakes on a hit, blind; lies down (KUTKU_MOT_LIE_DOWN) and lies
 * (KUTKU_MOT_LIE) for the rest time (5400/9000 frames) regenerating hp as in
 * EmKutku_Rest, then KUTKU_IDLE_STATUS2_END.
 */
void EmKutku_Status2LieDown(Em *em)
{
    u8 kind;
    u8 emKind;

    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUTKU_MOT_LIE_DOWN, 0, 0);
        Em_LoadReactionSet(em, 1);
        kind = em->type;
        if ((kind == EM_KIND_YIAN_KUT_KU) || (kind == EM_KIND_BLUE_YIAN_KUT_KU)) {
            em->timer = 0x1518;
            break;
        }
        em->timer = 0x2328;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUTKU_MOT_LIE, 0, 0);
        }
        break;
    case 2:
        emKind = em->type;
        if ((emKind == EM_KIND_YIAN_KUT_KU) || (emKind == EM_KIND_BLUE_YIAN_KUT_KU)) {
            Em_RegenHp(em, 1, (s32) (0.35f * (f32) em->hpMax), 9);
        } else {
            Em_RegenHp(em, 1, (s32) (0.4f * (f32) em->hpMax), 5);
        }
        if (--em->timer <= 0) {
            em->step++;
            EmKutku_ChangeState(em, EM_GROUP_IDLE, KUTKU_IDLE_STATUS2_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmKutku_ChangeState(em, EM_GROUP_IDLE, KUTKU_IDLE_STATUS2_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * Plays motion KUTKU_MOT_IDLE_PAIR_A once, then EndState.
 */
void EmKutku_IdleActionD(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUTKU_MOT_IDLE_PAIR_A, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * Blind while it gets up from the rest (KUTKU_MOT_REST_GET_UP), reaction set
 * 0, then KUTKU_IDLE_SHAKE_OFF.
 */
void EmKutku_RestEnd(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, KUTKU_MOT_REST_GET_UP, 0, 0);
        Em_LoadReactionSet(em, 0);
        em->blind = 2;
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            EmKutku_ChangeState(em, EM_GROUP_IDLE, KUTKU_IDLE_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmKutku_ChangeState(em, EM_GROUP_IDLE, KUTKU_IDLE_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * Blind while it gets up (KUTKU_MOT_GET_UP), reaction set 0, then
 * KUTKU_IDLE_SHAKE_OFF.
 */
void EmKutku_Status2End(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, KUTKU_MOT_GET_UP, 0, 0);
        Em_LoadReactionSet(em, 0);
        em->blind = 2;
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            EmKutku_ChangeState(em, EM_GROUP_IDLE, KUTKU_IDLE_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmKutku_ChangeState(em, EM_GROUP_IDLE, KUTKU_IDLE_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * Plays the shake-off motion KUTKU_MOT_SHAKE_OFF once, then EndState; the
 * end of every lying state.
 */
void EmKutku_ShakeOff(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, KUTKU_MOT_SHAKE_OFF, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * Like EmKutku_EatNeed2 without the dust clouds: forage KUTKU_MOT_FORAGE,
 * feed KUTKU_MOT_FEED, need1Gauge refilled to need1Max and +5% hpMax.
 */
void EmKutku_EatNeed1(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_NO_FLINCH);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUTKU_MOT_FORAGE, 0, 0);
        Em_SelectSightRow(em, 1);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUTKU_MOT_FEED, 0, 0);
            em->need1Gauge = em->need1Max;
            Em_AddHp(em, (s32) (0.05f * (f32) em->hpMax));
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->hitReactMode = 0;
            Em_SelectSightRow(em, 0);
            Em_LoadReactionSet(em, 0);
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * Wakes on a hit, blind; collapses (KUTKU_MOT_COLLAPSE) and lies
 * (KUTKU_MOT_LIE) until em->timer (Em_SetStatus1Timer) runs out, then
 * KUTKU_IDLE_STATUS1_END.
 */
void EmKutku_Status1Collapse(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUTKU_MOT_COLLAPSE, 0, 0);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUTKU_MOT_LIE, 0, 0);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            EmKutku_ChangeState(em, EM_GROUP_IDLE, KUTKU_IDLE_STATUS1_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmKutku_ChangeState(em, EM_GROUP_IDLE, KUTKU_IDLE_STATUS1_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * Blind while it gets up (KUTKU_MOT_GET_UP), reaction set 0, then
 * KUTKU_IDLE_SHAKE_OFF.
 */
void EmKutku_Status1End(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, KUTKU_MOT_GET_UP, 0, 0);
        em->blind = 2;
        Em_LoadReactionSet(em, 0);
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            EmKutku_ChangeState(em, EM_GROUP_IDLE, KUTKU_IDLE_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmKutku_ChangeState(em, EM_GROUP_IDLE, KUTKU_IDLE_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * Status 2 begun in a hold (holdTimer kept at 5): wakes on a hit, blind,
 * downed motion KUTKU_MOT_DOWNED with lying puffs at bone 0x22 and sound
 * 0x57 every 135 frames; when em->timer runs out goes to
 * KUTKU_REACT_STATUS2_DOWNED_END.
 */
void EmKutku_Status2Downed(Em *em, EmWork *work)
{
    f32 puffOfs[3];

    em->holdTimer = 5;
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUTKU_MOT_DOWNED, 0, 0);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmKutku_ChangeState(em, EM_GROUP_REACT, KUTKU_REACT_STATUS2_DOWNED_END, EM_SYNC_OWNER);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmKutku_ChangeState(em, EM_GROUP_REACT, KUTKU_REACT_STATUS2_DOWNED_END, EM_SYNC_OWNER);
        }
        break;
    }
    puffOfs[0] = 0.0f;
    puffOfs[1] = 10.0f;
    puffOfs[2] = 140.0f;
    Em_SpawnLyingPuffs(em, 0x22, puffOfs, 1.6f);
    if (em->timer % 135 == 0) {
        Em_PlaySeAtBone(em, 0x57, 0x23);
    }
}

/*
 * The same as Status2Downed for status 1; ends in
 * KUTKU_REACT_STATUS1_DOWNED_END.
 */
void EmKutku_Status1Downed(Em *em, EmWork *work)
{
    f32 puffOfs[3];

    em->holdTimer = 5;
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUTKU_MOT_DOWNED, 0, 0);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmKutku_ChangeState(em, EM_GROUP_REACT, KUTKU_REACT_STATUS1_DOWNED_END, EM_SYNC_OWNER);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmKutku_ChangeState(em, EM_GROUP_REACT, KUTKU_REACT_STATUS1_DOWNED_END, EM_SYNC_OWNER);
        }
        break;
    }
    puffOfs[0] = 0.0f;
    puffOfs[1] = 10.0f;
    puffOfs[2] = 140.0f;
    Em_SpawnLyingPuffs(em, 0x22, puffOfs, 1.6f);
    if (em->timer % 135 == 0) {
        Em_PlaySeAtBone(em, 0x57, 0x23);
    }
}

/*
 * Serves KUTKU_IDLE_DISPLAY, KUTKU_IDLE_DISPLAY_RAGE and
 * KUTKU_IDLE_DISPLAY_PENDING_RAGE: motion KUTKU_MOT_THREAT (a layer-1
 * motion) once, then EndState; EmKutku_SetIdleState starts the rage or
 * pending rage on entry.
 */
void EmKutku_IdleDisplay(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUTKU_MOT_THREAT, 0, 0);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * Serves KUTKU_IDLE_HOLD2_SHAKE_OFF (variant 2),
 * KUTKU_IDLE_STATUS4_SHAKE_OFF (0) and KUTKU_IDLE_STUN_SHAKE_OFF (1); all
 * play the shake-off motion KUTKU_MOT_SHAKE_OFF once and EndState (the
 * variant is unused; EmKutku_SetIdleState does the per-state work: clear
 * holdTimer2, Em_EndStatus4, Em_EndStatus5).
 */
void EmKutku_StatusShakeOff(Em *em, EmWork *work, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, KUTKU_MOT_SHAKE_OFF, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * Walk motion KUTKU_MOT_WALK to the goal (Em_StepToGoal 0x40) or, with no
 * goal, for em->timer frames; then EndState.
 */
void EmKutku_Walk(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUTKU_MOT_WALK, 0, 0);
        break;
    case 1:
        if (em->hasGoal != 0) {
            if (Em_StepToGoal(em, 0x40, 1) == 1) {
                em->step++;
                EmKutku_EndState(em, 0, 0);
                break;
            }
        } else {
            if (--em->timer <= 0) {
                em->step++;
                EmKutku_EndState(em, 0, 0);
            }
        }
        break;
    }
}

/*
 * Ends at once (EndState); never runs, see NOTE.
 */
void EmKutku_MoveEnd(Em *em)
{
    em->step++;
    EmKutku_EndState(em, 0, 0);
}

/*
 * Like EmKutku_Walk with motion KUTKU_MOT_WALK_B.
 */
void EmKutku_WalkB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUTKU_MOT_WALK_B, 0, 0);
        break;
    case 1:
        if (em->hasGoal != 0) {
            if (Em_StepToGoal(em, 0x40, 1) == 1) {
                em->step++;
                EmKutku_EndState(em, 0, 0);
                break;
            }
        } else {
            if (--em->timer <= 0) {
                em->step++;
                EmKutku_EndState(em, 0, 0);
            }
        }
        break;
    }
}

/*
 * Turns toward the target with EmKutku_TurnParams[0]
 * (Em_StartTurnToTarget/Em_TurnStep), clears EM_FLAG_NO_ROOT_MOTION,
 * EndState.
 */
void EmKutku_Turn(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_StartTurnToTarget(em, &EmKutku_TurnParams[0]);
        break;
    case 1:
        if (Em_TurnStep(em, &EmKutku_TurnParams[0]) == 1) {
            em->step++;
            Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * Serves KUTKU_MOVE_RUN and KUTKU_MOVE_RUN_SLOW (motionSpeed 0.8 set by
 * EmKutku_SetMoveState): run motion KUTKU_MOT_RUN to the goal or for
 * em->timer frames, then EndState.
 */
void EmKutku_Run(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUTKU_MOT_RUN, 0, 0);
        break;
    case 1:
        if (em->hasGoal != 0) {
            if (Em_StepToGoal(em, 0x40, 1) == 1) {
                em->step++;
                EmKutku_EndState(em, 0, 0);
                break;
            }
        } else {
            if (--em->timer <= 0) {
                em->step++;
                EmKutku_EndState(em, 0, 0);
            }
        }
        break;
    }
}

/*
 * Like EmKutku_Turn with EmKutku_TurnParams[1] (EmKutku_SetMoveState picks
 * it for KUTKU_MOVE_TURN in combat).
 */
void EmKutku_TurnInCombat(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_StartTurnToTarget(em, &EmKutku_TurnParams[1]);
        break;
    case 1:
        if (Em_TurnStep(em, &EmKutku_TurnParams[1]) == 1) {
            em->step++;
            Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * Limp motion KUTKU_MOT_LIMP toward the goal until em->timer (at most 300)
 * runs out or the goal is reached; EmKutku_SetMoveState picks it for walks
 * below 20% hp.
 */
void EmKutku_Limp(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUTKU_MOT_LIMP, 0, 0);
        break;
    case 1:
        if (Em_StepToGoal(em, 0x40, 1) == 1) {
            em->timer = 1;
        }
        if (--em->timer <= 0) {
            em->step++;
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * Motion 0x36 turning rotY toward the goal 0x200 a frame (snapping when
 * within 0x200) until frame 60, then waits for layer 1 to stop and ends.
 */
void EmKutku_TurnToGoal(Em *em, EmWork *work)
{
    u16 goalAngle;
    u32 diff;
    s32 yaw;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUTKU_MOT_TURN_IN_PLACE, 0, 0);
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        break;
    case 1:
        yaw = em->rotY;
        goalAngle = em->aimAngle;
        diff = (goalAngle - (yaw & 0xFFFF)) & 0xFFFF;
        if (((diff + 0x200) & 0xFFFF) < 0x400U) {
            em->rotY = goalAngle;
        } else if (diff < 0x8000U) {
            em->rotY = (yaw + 0x200) & 0xFFFF;
        } else {
            em->rotY = (yaw - 0x200) & 0xFFFF;
        }
        if (EmMotion_ReachedFrame(em, 0, 60.0f) != 0) {
            em->step++;
        }
        break;
    case 2:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * Empty; KUTKU_MOVE_NOP does nothing until something else changes the state.
 */
void EmKutku_MoveNop(void)
{
}

/*
 * Walk motion KUTKU_MOT_WALK_B to the goal or for em->timer frames, then
 * motion KUTKU_MOT_TURN_TO_GOAL (the idle turn to the goal) once, then
 * EndState.
 */
void EmKutku_WalkBAndTurn(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUTKU_MOT_WALK_B, 0, 0);
        break;
    case 1:
        if (Em_StepToGoal(em, 0x40, 1) == 1) {
            em->timer = 1;
        }
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, KUTKU_MOT_TURN_TO_GOAL, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * Take-off motion KUTKU_MOT_TAKE_OFF with curve set 2 (EmKutku_StartCurves);
 * airborne (EM_STANCE_AIR) from frame 38; when the curves finish ends in the
 * air (EmKutku_EndStateInAir 0: KUTKU_FLY_HOVER_ONCE).
 */
void EmKutku_TakeOff(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUTKU_MOT_TAKE_OFF, 0, 0);
        EmKutku_StartCurves(em, 2);
        work->flyGuard = 0;
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 38.0f, 0U) != 0) {
            em->step++;
            em->stance = EM_STANCE_AIR;
            EmKutku_UpdateCurves(em);
        }
        break;
    case 2:
        if (EmKutku_UpdateCurves(em)) {
            em->step++;
            EmKutku_EndStateInAir(em, 0U);
        }
        break;
    }
}

/*
 * Hover motion KUTKU_MOT_HOVER; picks a starting downward speed that,
 * slowing 0.75 a frame, brings it near 1000 above the floor; below
 * 630*scaleY over the floor at a wing beat (frames 10, 86, 160) plays the
 * descent 0xB for 30 frames, then the touch-down 0x13 on the floor
 * (EM_STANCE_GROUND, camera shake 1) and EndState. Never below floorY.
 */
void EmKutku_Land(Em *em, EmWork *work)
{
    f32 height;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        em->rotX = 0;
        em->rotZ = 0;
        Em_SetMotion(em, KUTKU_MOT_HOVER, 0, 0);
        Player_ClearVelocity(em);
        em->velY = -10.0f;
        height = em->floorY;
        height = em->posY - (1000.0f + height);
        if (height > 0.0f) {
            do {
                em->velY -= 0.75f;
                height += em->velY;
            } while (height > 0.0f && !(em->velY < -200.0f));
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
        if (!((em->floorY + 630.0f * em->scaleY) <= em->posY) && ((EmMotion_PassedFrameScaled(em, 10.0f, 0U) != 0) || (EmMotion_PassedFrameScaled(em, 86.0f, 0U) != 0) || (EmMotion_PassedFrameScaled(em, 160.0f, 0U) != 0))) {
            em->step++;
            Em_SetMotion(em, KUTKU_MOT_DESCEND, 0, 0);
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
            Em_SetMotion(em, KUTKU_MOT_TOUCH_DOWN, 0, 0);
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
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
    height = em->floorY;
    if (em->posY < height) {
        em->posY = height;
    }
}

/*
 * Stance 2, hover motion KUTKU_MOT_HOVER, height from velY curve 1
 * (EmKutku_MoveByVelYCurve), turning to the goal at 0x100; when em->timer
 * runs out lands (KUTKU_FLY_LAND); with 300 frames left ends in the air
 * instead (KUTKU_FLY_HOVER_ONCE).
 */
void EmKutku_Hover(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        work->turnSpeed = 0x100;
        Em_SetMotion(em, KUTKU_MOT_HOVER, 0, 0);
        work->flyGuard = 0;
        break;
    case 1:
        EmKutku_MoveByVelYCurve(em, 1U);
        if (--em->timer <= 0) {
            em->step++;
            EmKutku_ChangeState(em, EM_GROUP_FLY, KUTKU_FLY_LAND, EM_SYNC_SEND);
        }
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 1);
        if (em->timer == 0x12C) {
            EmKutku_EndStateInAir(em, 0U);
        }
        break;
    case 2:
        EmKutku_MoveByVelYCurve(em, 1U);
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 1);
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * Take-off motion KUTKU_MOT_TAKE_OFF with curve set 2 turning to the goal
 * between frames 50 and 114, airborne from frame 30; after the curves,
 * within 800*scaleY of the floor descends (KUTKU_MOT_DESCEND), touches down (KUTKU_MOT_TOUCH_DOWN, camera
 * shake 2), sets fxMotion 150 (frames since landing) and ends.
 */
void EmKutku_HopToGoal(Em *em, EmWork *work)
{
    f32 velY;
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUTKU_MOT_TAKE_OFF, 0, 0);
        Player_ClearVelocity(em);
        EmKutku_StartCurves(em, 2);
        work->flyGuard = 0;
        work->turnSpeed = 0x100;
        break;
    case 1:
        if ((EmMotion_ReachedFrame(em, 0, 50.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 114.0f) == 0)) {
            Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 1);
        }
        if (EmMotion_ReachedFrame(em, 0, 30.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            EmKutku_UpdateCurves(em);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
        }
        break;
    case 2:
        if ((EmKutku_UpdateCurves(em) & 0xFF) && (em->posY <= (em->floorY + (800.0f * em->scaleY)))) {
            em->step++;
            Em_SetMotion(em, KUTKU_MOT_DESCEND, 0, 0);
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
            Em_SetMotion(em, KUTKU_MOT_TOUCH_DOWN, 0, 0);
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            CamShake_ReqAIf(em, 2);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            work->fxMotion = 0x96;
            em->step++;
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * Takes off (motion KUTKU_MOT_TAKE_OFF) with the velY curve 3 and velZ 20,
 * airborne from frame 30; when the curve ends at the floor it plays the
 * touch-down (KUTKU_MOT_TOUCH_DOWN) with a camera shake. Sets fxMotion
 * (frames since landing) to 150 and ends (EmKutku_EndState).
 */
void EmKutku_HopForward(Em *em, EmWork *work)
{
    f32 floorY;
    s32 ended;

    ended = 0;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUTKU_MOT_TAKE_OFF, 0, 0);
        EmKutku_StartCurves(em, 3);
        em->velZ = 20.0f;
        work->flyGuard = 0;
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 30.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            ended = EmKutku_UpdateCurves(em) & 0xFF;
        }
        if ((ended != 0) && (em->posY <= em->floorY)) {
            Em_SetMotion(em, KUTKU_MOT_TOUCH_DOWN, 0, 0);
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
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * As EmKutku_HopForward with velZ -20: a hop backward that lands again
 * (KUTKU_MOT_TAKE_OFF, 0x13, camera shake, fxMotion 150).
 */
void EmKutku_HopBack(Em *em, EmWork *work)
{
    f32 floorY;
    s32 ended;

    ended = 0;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUTKU_MOT_TAKE_OFF, 0, 0);
        EmKutku_StartCurves(em, 3);
        em->velZ = -20.0f;
        work->flyGuard = 0;
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 30.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            ended = EmKutku_UpdateCurves(em) & 0xFF;
        }
        if ((ended != 0) && (em->posY <= em->floorY)) {
            Em_SetMotion(em, KUTKU_MOT_TOUCH_DOWN, 0, 0);
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
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * Flies a route of area points: glides (KUTKU_MOT_GLIDE), then flies (KUTKU_MOT_FLY) toward the
 * goal with EmKutku_FlyParam; at each goal (within 3000 or timer out) takes
 * one of legsLeft (EmKutku_SetFlyState: 4), steps curGoalNo and restarts
 * itself (Em_SetStateRand); with none left it ends into the glide
 * (EmKutku_EndStateInAir(1)).
 */
void EmKutku_FlyRoute(Em *em, EmWork *work)
{
    f32 floorY;
    s32 timerLeft;
    u8 legs;
    u8 stateVar1;

    switch (em->step) {
    case 0:
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, KUTKU_MOT_GLIDE, 0, 0);
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
            Em_SetMotion(em, KUTKU_MOT_FLY, 0, 0);
        }
        if ((Vec_DistanceXZ(&em->posX, em->goal) <= 3000.0f) || (timerLeft = em->timer - 1, em->timer = timerLeft, (timerLeft <= 0))) {
            legs = ((EmFlyerWork *)work)->legsLeft - 1;
            ((EmFlyerWork *)work)->legsLeft = legs;
            if ((legs & 0xFF) <= 0) {
                EmKutku_EndStateInAir(em, 1U);
            } else {
                em->curGoalNo++;
                em->curGoalNo &= 3;
                Em_ComputeGoal2(em, em->goal);
                em->timer = (s32) ((1.5f * Vec_DistanceXZ(&em->posX, em->goal)) / 50.0f);
                Em_SetStateRand(em, EM_GROUP_FLY, KUTKU_FLY_ROUTE);
            }
        }
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 0xE1, &EmKutku_FlyParam);
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
 * Hovers (KUTKU_MOT_HOVER) toward the goal, turning to it (turnSpeed 0x100), pitching to
 * it and riding the hover velY curve, until within 10*velZ or the timer
 * (EmKutku_SetFlyState: 3x the travel time at velZ 20) runs out; then stops
 * and ends into the hover (EmKutku_EndStateInAir(0)).
 */
void EmKutku_HoverToGoal(Em *em, EmWork *work)
{
    f32 dist;
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        work->turnSpeed = 0x100;
        Em_SetMotion(em, KUTKU_MOT_HOVER, 0, 0);
        work->flyGuard = 0;
        break;
    case 1:
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 1);
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_GetPitchToGoal(em, &work->moveAngX, 0.0f, 0.0f);
        Em_MoveRotated(em, &work->moveAngX);
        EmKutku_MoveByVelYCurve(em, 1U);
        dist = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer > 0) {
            if (dist <= (10.0f * em->velZ)) {
                goto block_9;
            }
        } else {
block_9:
            em->step++;
            Player_ClearVelocity(em);
            EmKutku_EndStateInAir(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * Rises in the hover (velY 20, growing above floor+500) to the area exit
 * point height, then glides (KUTKU_MOT_GLIDE) to the exit until it is outside the area;
 * there it enters the next area: lands (KUTKU_FLY_TO_GOAL_AND_LAND) when
 * that is its destination or there is none, else flies on
 * (KUTKU_FLY_THROUGH_AREA). Step 5 retries the area move every 600 frames on
 * a host whose ChangeState did not take.
 */
void EmKutku_FlyOutOfArea(Em *em, EmWork *work)
{
    u16 next;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, KUTKU_MOT_HOVER, 0, 0);
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
            Em_SetMotion(em, KUTKU_MOT_GLIDE, 0, 0);
            work->flyGuard = 1;
        }
        break;
    case 2:
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 0x4E1, &EmKutku_FlyParam);
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
            EmKutku_ChangeState(em, EM_GROUP_FLY, KUTKU_FLY_TO_GOAL_AND_LAND, EM_SYNC_SEND);
        } else {
            if (em->remote == 0) {
                em->nextArea = next;
                em->goalNo = next;
                em->goalKind = 3;
                Em_ComputeGoal(em, em->goal);
            }
            EmKutku_ChangeState(em, EM_GROUP_FLY, KUTKU_FLY_THROUGH_AREA, EM_SYNC_SEND);
            Hud_TriggerGlow(em);
        }
        break;
    case 5:
        if (em->remote == 0) {
            if (--em->timer <= 0) {
                em->step = 4;
            }
        }
        break;
    }
    if (em->posY < em->floorY) {
        em->posY = em->floorY;
    }
}

/*
 * Faces the goal (EmKutku_SetFlyState: a point of the area, on the jungle
 * map point 1 for Kut-Ku and 0 for Gypceros) and flies (KUTKU_MOT_HOVER, velZ 80);
 * within 1000 or after 500 frames it lands (KUTKU_FLY_LAND). Clears
 * destArea.
 */
void EmKutku_FlyToGoalAndLand(Em *em, EmWork *work)
{
    f32 goalDist;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        work->turnSpeed = 0x100;
        Em_SetMotion(em, KUTKU_MOT_HOVER, 0, 0);
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
        Em_UpdateFlight(em, 0xE5, &EmKutku_FlyParam);
        work->moveAngX = em->rotX;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
        goalDist = Vec_DistanceXZ(&em->posX, em->goal);
        em->timer--;
        if (goalDist <= 1000.0f || em->timer < 0) {
            em->step++;
            em->timer = 600;
            EmKutku_ChangeState(em, EM_GROUP_FLY, KUTKU_FLY_LAND, EM_SYNC_SEND);
        }
        break;
    case 2:
        if (em->remote == 0) {
            if (--em->timer <= 0) {
                em->timer = 600;
                EmKutku_ChangeState(em, EM_GROUP_FLY, KUTKU_FLY_LAND, EM_SYNC_SEND);
            }
        }
        break;
    }
}

/*
 * Only ends the flight state into the glide (EmKutku_EndStateInAir(1),
 * KUTKU_FLY_GLIDE_ONCE).
 */
void EmKutku_FlyEndToGlide(Em *em)
{
    EmKutku_EndStateInAir(em, 0x1);
}

/*
 * Glides (KUTKU_MOT_GLIDE, velZ at most 100) to the goal; within 1000 it slows to 50 and
 * lands (KUTKU_FLY_LAND), as it does when the timer (500) runs out.
 */
void EmKutku_GlideToGoalAndLand(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUTKU_MOT_GLIDE, 0, 0);
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
            EmKutku_ChangeState(em, EM_GROUP_FLY, KUTKU_FLY_LAND, EM_SYNC_SEND);
        } else {
            em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
            em->aimAngle = em->aimAngle - em->rotY;
            Em_UpdateFlight(em, 0xE1, &EmKutku_FlyParam);
            work->moveAngX = em->rotX;
            work->moveAngY = em->rotY;
            work->moveAngZ = 0;
            Em_MoveRotated(em, &work->moveAngX);
            if (--em->timer <= 0) {
                em->step++;
                EmKutku_ChangeState(em, EM_GROUP_FLY, KUTKU_FLY_LAND, EM_SYNC_SEND);
            }
        }
        break;
    case 2:
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 0xE1, &EmKutku_FlyParam);
        work->moveAngX = em->rotX;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
        if (--em->timer <= 0) {
            em->timer = 0x12C;
            if (em->remote == 0) {
                EmKutku_ChangeState(em, EM_GROUP_FLY, KUTKU_FLY_LAND, EM_SYNC_SEND);
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
 * A hop backward from the ground (KUTKU_MOT_BACK_HOP, curve 0xE: velY and speed), airborne
 * from frame 76; lands with KUTKU_MOT_BACK_HOP_LAND, a dust cloud (EmKutku_SpawnLandingDust) and
 * a camera shake, then ends. Unlike the other landings it leaves fxMotion
 * alone.
 */
void EmKutku_BackHop(Em *em)
{
    f32 floorY;
    s32 ended;

    ended = 0;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUTKU_MOT_BACK_HOP, 0, 0);
        EmKutku_StartCurves(em, 0xE);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 76.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            ended = EmKutku_UpdateCurves(em) & 0xFF;
        }
        if ((ended != 0) && (em->posY <= em->floorY)) {
            Em_SetMotion(em, KUTKU_MOT_BACK_HOP_LAND, 0, 0);
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            EmKutku_SpawnLandingDust(em);
            CamShake_ReqAIf(em, 2);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * After an area change: flies (KUTKU_MOT_HOVER, velZ 80) toward the area entry point
 * (Em_AreaEntryValue of tables[0], else the area centre) at its bottom
 * height, then to the exit point and out of the area, and as
 * EmKutku_FlyOutOfArea lands (0x09) at the destination or flies on through
 * the next area (0x0D).
 */
void EmKutku_FlyThroughArea(Em *em, EmWork *work)
{
    AreaRecord *area;
    f32 *entryPoint;
    f32 goalDist;
    u16 next;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        work->turnSpeed = 0x100;
        Em_SetMotion(em, KUTKU_MOT_HOVER, 0, 0);
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
        break;
    case 1:
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 0xE5, &EmKutku_FlyParam);
        work->moveAngX = em->rotX;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
        goalDist = Vec_DistanceXZ(&em->posX, em->goal);
        em->timer--;
        if (goalDist <= 1000.0f || em->timer < 0) {
            em->step++;
            Em_AreaExitPoint(em, (f32 *) em->goal);
        }
        break;
    case 2:
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 0xE1, &EmKutku_FlyParam);
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
        em->timer = 600;
        Em_EnterNextArea(em);
        next = *(u16 *)&em->destArea;
        if (em->nextArea == next || next == AREA_NONE) {
            Hud_TriggerGlow(em);
            EmKutku_ChangeState(em, EM_GROUP_FLY, KUTKU_FLY_TO_GOAL_AND_LAND, EM_SYNC_SEND);
        } else {
            if (em->remote == 0) {
                em->nextArea = next;
                em->goalNo = next;
                em->goalKind = 3;
                Em_ComputeGoal(em, em->goal);
            }
            EmKutku_ChangeState(em, EM_GROUP_FLY, KUTKU_FLY_THROUGH_AREA, EM_SYNC_SEND);
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
 * Plays the hover (KUTKU_MOT_HOVER) once on the velY curve, then ends into itself again
 * (EmKutku_EndStateInAir(0) sets scriptPending so the script picks the next
 * state).
 */
void EmKutku_HoverOnce(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, KUTKU_MOT_HOVER, 0, 0);
        work->flyGuard = 0;
        break;
    case 1:
        EmKutku_MoveByVelYCurve(em, 1U);
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKutku_EndStateInAir(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * Plays the glide (KUTKU_MOT_GLIDE) once with velX/velY cleared, then ends into itself
 * (EmKutku_EndStateInAir(1)).
 */
void EmKutku_GlideOnce(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, KUTKU_MOT_GLIDE, 0, 0);
        Player_ClearAccel2(em);
        em->velY = 0.0f;
        em->velX = 0.0f;
        work->flyGuard = 1;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKutku_EndStateInAir(em, 1U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * Hovers (KUTKU_MOT_HOVER) on the velY curve turning to the goal (turnSpeed 0x100) until
 * it faces it or 150 frames pass, then ends into the hover.
 */
void EmKutku_HoverTurn(Em *em, EmWork *work)
{
    f32 floorY;
    s32 facingGoal;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, KUTKU_MOT_HOVER, 0, 0);
        work->turnSpeed = 0x100;
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        work->flyGuard = 0;
        em->timer = 0x96;
        break;
    case 1:
        EmKutku_MoveByVelYCurve(em, 1U);
        facingGoal = Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 1) & 0xFF;
        if (--em->timer > 0) {
            if (facingGoal != 0) {
                goto block_9;
            }
        } else {
block_9:
            em->step++;
            EmKutku_EndStateInAir(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * As EmKutku_FlyRoute (glide 0xC unless already gliding or flying, then
 * KUTKU_FLY_HOVER_ONCE) with EmKutku_FlyParam2 and flight flags 0x321, but
 * with no leg count: at each goal (within 1000 or timer out) it steps to the
 * next of 4 points and restarts itself, so it only leaves by another state
 * change.
 */
void EmKutku_FlyRouteLoop(Em *em, EmWork *work)
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
            Em_SetMotion(em, KUTKU_MOT_GLIDE, 0, 0);
        } else {
            em->stateVar1 = 1;
            motionId = em->motionId[0];
            if ((motionId != EM_MOTION_ID(0, KUTKU_MOT_GLIDE)) && (motionId != EM_MOTION_ID(0, KUTKU_MOT_FLY))) {
                Em_SetMotion(em, KUTKU_MOT_GLIDE, 0, 0);
            }
        }
        work->flyGuard = 1;
        break;
    case 1:
        stateVar1 = em->stateVar1;
        if ((stateVar1 == 0) && (em->motion[0].playing == 0)) {
            em->stateVar1 = stateVar1 + 1;
            Em_SetMotion(em, KUTKU_MOT_FLY, 0, 0);
        }
        if (!(Vec_DistanceXZ(&em->posX, em->goal) <= 1000.0f)) {
            if (--em->timer <= 0) {
                goto block_17;
            }
        } else {
block_17:
            em->curGoalNo++;
            em->curGoalNo &= 3;
            Em_ComputeGoal2(em, em->goal);
            em->timer = (s32) ((1.5f * Vec_DistanceXZ(&em->posX, em->goal)) / 50.0f) + 0x96;
            em->step++;
            Em_SetStateRand(em, EM_GROUP_FLY, KUTKU_FLY_ROUTE_LOOP);
        }
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 0x321, &EmKutku_FlyParam2);
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
 * Levels out (KUTKU_MOT_LEVEL_OUT, curve 9) stepping rotX by pitchStep and rotZ by
 * rollSpeed back to 0; when the curve ends it stops, plays the hover (KUTKU_MOT_HOVER)
 * and lands (KUTKU_FLY_LAND). EmKutku_SetFlyState turns a landing
 * (KUTKU_MOT_IDLE) requested at velZ 50 or more into this state.
 */
void EmKutku_LevelOutAndLand(Em *em, EmWork *work)
{
    f32 floorY;
    s32 rotX;
    s32 rotZ;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUTKU_MOT_LEVEL_OUT, 0, 0);
        EmKutku_StartCurves(em, 9);
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
        if (EmKutku_UpdateCurves(em) & 0xFF) {
            em->step++;
            Player_ClearVelocity(em);
            em->timer = 0x12C;
            Em_SetMotion(em, KUTKU_MOT_HOVER, 0, 0);
            EmKutku_ChangeState(em, EM_GROUP_FLY, KUTKU_FLY_LAND, EM_SYNC_SEND);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->timer = 0x12C;
            Em_SetMotion(em, KUTKU_MOT_HOVER, 0, 0);
            if (em->remote == 0) {
                EmKutku_ChangeState(em, EM_GROUP_FLY, KUTKU_FLY_LAND, EM_SYNC_SEND);
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
 * One leg of EmKutku_FlyRouteLoop (glide then fly with EmKutku_FlyParam2,
 * flags 0x121) to the current goal; at the goal (1000) or timer out it
 * changes to the glide (KUTKU_FLY_GLIDE_ONCE) instead of a next goal.
 * EmKutku_SetFlyState sets legsLeft 1 and the timer.
 */
void EmKutku_GlideToGoal(Em *em, EmWork *work)
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
            Em_SetMotion(em, KUTKU_MOT_GLIDE, 0, 0);
        } else {
            em->stateVar1 = 1;
            motionId = em->motionId[0];
            if ((motionId != EM_MOTION_ID(0, KUTKU_MOT_GLIDE)) && (motionId != EM_MOTION_ID(0, KUTKU_MOT_FLY))) {
                Em_SetMotion(em, KUTKU_MOT_GLIDE, 0, 0);
            }
        }
        work->flyGuard = 1;
        break;
    case 1:
        stateVar1 = em->stateVar1;
        if ((stateVar1 == 0) && (em->motion[0].playing == 0)) {
            em->stateVar1 = stateVar1 + 1;
            Em_SetMotion(em, KUTKU_MOT_FLY, 0, 0);
        }
        if (!(Vec_DistanceXZ(&em->posX, em->goal) <= 1000.0f)) {
            if (--em->timer <= 0) {
                goto block_17;
            }
        } else {
block_17:
            em->step++;
            Em_SetStateRand(em, EM_GROUP_FLY, KUTKU_FLY_GLIDE_ONCE);
        }
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 0x121, &EmKutku_FlyParam2);
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
 * A hop straight up on the ground (KUTKU_MOT_HOP_IN_PLACE, curve 0xC: velY only, velocity
 * cleared), airborne from frame 36; landing at the floor makes a dust cloud
 * and a camera shake (no landing motion), then it ends.
 */
void EmKutku_HopInPlace(Em *em, EmWork *work)
{
    f32 floorY;
    s32 ended;

    ended = 0;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUTKU_MOT_HOP_IN_PLACE, 0, 0);
        EmKutku_StartCurves(em, 0xC);
        work->flyGuard = 0;
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 36.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            ended = EmKutku_UpdateCurves(em) & 0xFF;
        }
        if ((ended != 0) && (em->posY <= em->floorY)) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            EmKutku_SpawnLandingDust(em);
            CamShake_ReqAIf(em, 2);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * Hovers (KUTKU_MOT_HOVER) heading straight at the goal (yaw from Math_AngleXZ, no turn
 * step, pitch to it) until within 10*velZ in 3D or the timer runs out; then
 * stops and ends into the hover.
 */
void EmKutku_HoverStraightToGoal(Em *em, EmWork *work)
{
    f32 dist;
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        work->turnSpeed = 0x100;
        Em_SetMotion(em, KUTKU_MOT_HOVER, 0, 0);
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
            EmKutku_EndStateInAir(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * As EmKutku_LevelOutAndLand (KUTKU_MOT_LEVEL_OUT, curve 9, rotX and rotZ stepped to 0) but
 * ends into the hover (EmKutku_EndStateInAir(0)) instead of landing.
 */
void EmKutku_LevelOut(Em *em, EmWork *work)
{
    s32 rotX;
    s32 rotZ;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUTKU_MOT_LEVEL_OUT, 0, 0);
        EmKutku_StartCurves(em, 9);
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
        if (EmKutku_UpdateCurves(em) & 0xFF) {
            em->step++;
            Player_ClearVelocity(em);
            EmKutku_EndStateInAir(em, 0U);
        }
        break;
    case 2:
        EmKutku_UpdateCurves(em);
        break;
    }
}

/*
 * Leaves a trap: clears holdTimer and trapHit, sets noReact and takes off
 * (0x12 blended, curve 2); when the curve ends within 800*scaleY of the
 * floor it descends (KUTKU_MOT_DESCEND) at the speed that reaches the floor in 30 frames
 * and touches down (KUTKU_MOT_TOUCH_DOWN, camera shake, fxMotion 150), then ends. Entered
 * from the trap struggles (KUTKU_REACT_TRAPPED_STRUGGLE, 0x11, 0x17).
 */
void EmKutku_TrapEscape(Em *em, EmWork *work)
{
    f32 floorY;
    f32 velY;
    f32 floorY2;

    switch (em->step) {
    case 0:
        em->step++;
        em->hitReactMode = 0;
        Em_SetMotion(em, KUTKU_MOT_TAKE_OFF, 0xA, 0x1E);
        EmKutku_StartCurves(em, 2);
        em->stance = EM_STANCE_AIR;
        em->holdTimer = 0;
        em->trapHit = 0;
        em->noReact = 1;
        break;
    case 1:
        EmKutku_UpdateCurves(em);
        if (em->motion[0].playing == 0) {
            em->step++;
        }
        break;
    case 2:
        if ((EmKutku_UpdateCurves(em) & 0xFF) && (em->posY <= (em->floorY + (800.0f * em->scaleY)))) {
            em->step++;
            Em_SetMotion(em, KUTKU_MOT_DESCEND, 0, 0);
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
            Em_SetMotion(em, KUTKU_MOT_TOUCH_DOWN, 0, 0);
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            CamShake_ReqAIf(em, 2);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            work->fxMotion = 0x96;
            em->step++;
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
    floorY2 = em->floorY;
    if (em->posY < floorY2) {
        em->posY = floorY2;
    }
}

/*
 * Rises in the hover (KUTKU_MOT_HOVER, velY 20) until 1000 above the goal height or the
 * timer (EmKutku_SetFlyState: distance / velZ) runs out, then stops and ends
 * into the hover. Reads an uninitialized pos (see NOTE).
 */
void EmKutku_HoverRise(Em *em, EmWork *work)
{
    f32 pos[3];
    f32 floorY;
    s32 timer;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, KUTKU_MOT_HOVER, 0, 0);
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
            EmKutku_EndStateInAir(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * Turns to the target (Em_TurnToTargetMotion: walk KUTKU_MOT_WALK, turn
 * motions KUTKU_MOT_TURN_LEFT and 5), then plays KUTKU_MOT_STARE and ends.
 */
void EmKutku_TurnAndStare(Em *em)
{
    switch (em->step) {
    case 0:
        if (Em_TurnToTargetMotion(em, 0xE38, 3, 6, 5) != 0) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->lookAtTarget = 0;
            Em_SetMotion(em, KUTKU_MOT_STARE, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * Turns to the target, then leaps (KUTKU_MOT_LEAP, curve 5), airborne from frame 60,
 * covering runDist (EmKutku_SetAttackState: the target distance clamped to
 * 2500..5000); then falls (accelY -10) to the floor and lands
 * (KUTKU_MOT_LEAP_LAND), with dust every 5 frames in the last 500. Ends with
 * EmKutku_EndAttack.
 */
void EmKutku_Leap(Em *em, EmWork *work)
{
    EmFlyerWork *flyer = (EmFlyerWork *)work;
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
                Em_SetMotion(em, KUTKU_MOT_LEAP, 0, 0);
                EmKutku_StartCurves(em, 5U);
            }
        }
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 60.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            EmKutku_UpdateCurves(em);
            em->step++;
        }
        break;
    case 2:
        flyer->runDist = (flyer->runDist - em->velZ);
        EmKutku_UpdateCurves(em);
        if (flyer->runDist <= 0.0f) {
            em->step++;
            em->accelY = -10.0f;
        }
        if (flyer->runDist <= 500.0f) {
            EmKutku_SpawnDustCloudAEvery5(em);
        }
        break;
    case 3:
        flyer->moveAngX = 0;
        flyer->moveAngY = em->rotY;
        flyer->moveAngZ = 0;
        Em_MoveRotatedAccel(em, &flyer->moveAngX);
        if (flyer->runDist <= 500.0f) {
            EmKutku_SpawnDustCloudAEvery5(em);
        }
        if (em->posY <= em->floorY) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            Em_SetMotion(em, KUTKU_MOT_LEAP_LAND, 0, 0);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKutku_EndAttack(em, work);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * Plays 0x24 in place with lookAtTarget set, then ends.
 */
void EmKutku_Stare(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 1;
        Em_SetMotion(em, KUTKU_MOT_STARE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->lookAtTarget = 0;
            em->step++;
            EmKutku_EndAttack(em, work);
        }
        break;
    }
}

/*
 * Collapses (KUTKU_MOT_DIE, the death motion), unhittable (noHitTimer 5) to frame
 * 350, lies for the timer (EmKutku_SetAttackState: 1800), then lies still
 * (KUTKU_ATK_FEIGN_DEATH_LIE). While in this state or 0x09
 * EmKutku_ChangeState turns any non-react, non-defeat change into
 * KUTKU_ATK_FEIGN_DEATH_RISE (getting up). No kind test here.
 */
void EmKutku_FeignDeath(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUTKU_MOT_DIE, 0, 0);
        em->noHitTimer = 5;
        break;
    case 1:
        em->noHitTimer = 5;
        if (EmMotion_PassedFrameScaled(em, 350.0f, 0U) != 0) {
            em->step++;
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            em->timer = 0x12C;
            EmKutku_ChangeState(em, EM_GROUP_ATTACK, KUTKU_ATK_FEIGN_DEATH_LIE, EM_SYNC_SEND);
        }
        break;
    case 3:
        if (--em->timer <= 0) {
            em->timer = 0x12C;
            if (em->remote == 0) {
                EmKutku_ChangeState(em, EM_GROUP_ATTACK, KUTKU_ATK_FEIGN_DEATH_LIE, EM_SYNC_SEND);
            }
        }
        break;
    }
}

/*
 * Spins in place twice (0x2A twice, rotY + 0x200 a frame), then ends. Waits
 * on layer 1 motion (see NOTE).
 */
void EmKutku_TailSpin(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUTKU_MOT_TAIL_SPIN, 0, 0);
        Motion_InitTime(em, 0);
        break;
    case 1:
        em->rotY += 0x200;
        if (em->motion[1].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUTKU_MOT_TAIL_SPIN, 0, 0);
        }
        break;
    case 2:
        em->rotY += 0x200;
        if (em->motion[1].playing == 0) {
            em->step++;
            EmKutku_EndAttack(em, work);
        }
        break;
    }
}

/*
 * Gets up from the feigned death (KUTKU_MOT_FEIGN_DEATH_RISE, whose effects make an attack at
 * frame 20: KutkuAttack 0x25 in KUTKU_ATK_FEIGN_DEATH_RISE_B, 0xE in 0x07),
 * clearing blind and scriptPending and restarting the script with noReact
 * set; ends when the layer 1 motion ends. Serves states 0x07 and 0x20.
 */
void EmKutku_FeignDeathRise(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUTKU_MOT_FEIGN_DEATH_RISE, 0, 0);
        em->blind = 0;
        em->scriptPending = 0;
        Em_RestartScript(em);
        em->noReact = 1;
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * A swoop at the target from the air: glides to the escape point
 * (EmKutku_SetAttackState: the one of 4 area points farthest from the
 * target) slowing to 100, then turns at the target; facing it from over 4000
 * it dives (KUTKU_MOT_SWOOP_DIVE) and swoops down (KUTKU_MOT_SWOOP, velZ 100) aiming
 * 3000 below it, pushed up off walls; near the floor or the target
 * (EmKutku_SwoopClawIfNear plays the claw 0x2D) it climbs, and when the claw
 * ends flies (KUTKU_MOT_FLY) up to 1300 over the area bottom and away to a new escape
 * point. Ends (EmKutku_EndSwoop: the glide) when it levels out, on timeouts
 * (600, 300), when the target is lost or not every player is here.
 */
void EmKutku_SwoopThenLeave(Em *em, EmWork *work)
{
    f32 targetWorldPos[3];
    f32 targetPos[3];
    f32 targetPosB[3];
    AreaRecord *area;
    s8 idx;

    area = Stage_GetAreaRecord(em->area);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        break;
    case 1:
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 0xD1, &EmKutku_FlyParam);
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
            EmKutku_EndSwoop(em, (s32) work);
        }
        break;
    case 2:
        if (em->targetNo == -1) {
            EmKutku_EndSwoop(em, (s32) work);
            break;
        }
        if (80.0f < em->velZ) {
            em->velZ -= 1.0f;
        }
        idx = em->targetNo;
        Em_GetTargetPos(em, idx, targetPos);
        Area_ToWorld(((Player *) &gPlayers[idx])->area, targetPos, targetWorldPos);
        em->aimAngle = Math_AngleXZ(em->mapPos, targetWorldPos);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 0xD1, &EmKutku_FlyParam);
        if (em->aimAngle <= 0x800 || em->aimAngle >= 0xF800) {
            if (!(4000.0f < Vec_DistanceXZ(&em->posX, targetPos))) {
                EmKutku_EndSwoop(em, (s32) work);
            } else {
                em->step++;
                work->turnSpeed = 0x100;
                Em_SetMotion(em, KUTKU_MOT_SWOOP_DIVE, 0, 0);
                em->timer = 600;
            }
        }
        work->moveAngX = em->rotX;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
        if (--em->timer <= 0 || Em_AllPlayersHere(em) == 0) {
            EmKutku_EndSwoop(em, (s32) work);
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
            Em_SetMotion(em, KUTKU_MOT_SWOOP, 0, 0);
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
            EmKutku_EndSwoop(em, (s32) work);
        }
        break;
    case 4:
        if (em->posY <= 1000.0f + area->bottom) {
            em->step++;
            em->timer = 600;
        }
        if (EmKutku_SwoopClawIfNear(em) && em->step == 4) {
            em->step++;
            em->timer = 600;
        }
        if (em->motionId[0] == EM_MOTION_ID(0, KUTKU_MOT_SWOOP_CLAW)) {
            EmKutku_StepPitch(em, 2, 0.0f);
        } else {
            EmKutku_StepPitch(em, 0, -200.0f);
        }
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 2);
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_GetPitchToTarget(em, &work->moveAngX, -3000.0f, 0.0f);
        if (em->wallHitDirs != 0) {
            work->moveAngX = 0;
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
            EmKutku_EndSwoop(em, (s32) work);
        }
        break;
    case 5:
        EmKutku_SwoopClawIfNear(em);
        if (em->motionId[0] == EM_MOTION_ID(0, KUTKU_MOT_SWOOP_CLAW) && em->motion[0].playing == 0) {
            em->step = 6;
            Em_SetMotion(em, KUTKU_MOT_FLY, 0, 0);
            EmKutku_SetFarthestEscapePoint(em);
        }
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 2);
        EmKutku_StepPitch(em, 2, 100.0f);
        if (em->motionId[0] == EM_MOTION_ID(0, KUTKU_MOT_SWOOP_CLAW) && EmMotion_ReachedFrame(em, 0, 76.0f)) {
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
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        if (em->wallHitDirs != 0) {
            work->moveAngX = 0;
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
            EmKutku_EndSwoop(em, (s32) work);
        }
        break;
    case 6:
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        EmKutku_StepPitch(em, 1, 0.0f);
        Em_UpdateFlight(em, 0xD1, &EmKutku_FlyParam);
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
        EmKutku_StepPitch(em, 2, 0.0f);
        Em_UpdateFlight(em, 0xD1, &EmKutku_FlyParam);
        work->moveAngX = em->rotX;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
        if (Vec_DistanceXZ(&em->posX, em->goal) <= 3000.0f) {
            EmKutku_SetFarthestEscapePoint(em);
        }
        if ((em->rotX == 0 && em->rotZ == 0) || --em->timer <= 0) {
            em->rotX = 0;
            em->rotZ = 0;
            EmKutku_EndSwoop(em, (s32) work);
        }
        break;
    }
    if (em->posY < em->floorY) {
        em->posY = em->floorY;
    }
}

/*
 * Lies still (KUTKU_MOT_FEIGN_DEATH_LIE) out of combat (Em_SetCombat(em, 0,
 * 0)), briefly unhittable, leaving a timed gathering point at its position
 * (EmMine_CreateTimed0); after the timer (EmKutku_SetAttackState: 1800) it
 * gets up (KUTKU_ATK_FEIGN_DEATH_RISE).
 */
void EmKutku_FeignDeathLie(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUTKU_MOT_FEIGN_DEATH_LIE, 0, 0);
        Em_SetCombat(em, 0, 0);
        em->noHitTimer = 5;
        EmMine_CreateTimed0(em, &em->posX);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            em->timer = 300;
            EmKutku_ChangeState(em, EM_GROUP_ATTACK, KUTKU_ATK_FEIGN_DEATH_RISE, EM_SYNC_SEND);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->timer = 300;
            if (em->remote == 0) {
                EmKutku_ChangeState(em, EM_GROUP_ATTACK, KUTKU_ATK_FEIGN_DEATH_RISE, EM_SYNC_SEND);
            }
        }
        break;
    }
}

/*
 * Turns to the target, then plays KUTKU_MOT_PECK_COMBO (four pecks: KutkuAttack 0x18 at
 * frames 32, 54, 74, 92) and ends.
 */
void EmKutku_PeckCombo(Em *em)
{
    switch (em->step) {
    case 0:
        if (Em_TurnToTargetMotion(em, 0xE38, 3, 6, 5) != 0) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->lookAtTarget = 0;
            Em_SetMotion(em, KUTKU_MOT_PECK_COMBO, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * As EmKutku_PeckCombo, but when its own KutkuAttack (attach kind 0x11) hits
 * a live player it steals an item from that player (EmKutku_StealItem),
 * once. No kind test here.
 */
void EmKutku_PeckSteal(Em *em, EmWork *work)
{
    Player *player;

    switch (em->step) {
    case 0:
        if (Em_TurnToTargetMotion(em, 0xE38, 3, 6, 5) != 0) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->lookAtTarget = 0;
            Em_SetMotion(em, KUTKU_MOT_PECK_COMBO, 0, 0);
            em->hitPending = 0;
        }
        break;
    case 1:
        if (em->hitPending != 0) {
            player = em->contact;
            if (player != NULL && player->index < 4 && player == &gPlayers[player->index] && em->contactAttach->kind == 0x11 && player->active != 0) {
                em->step++;
                EmKutku_StealItem(em, em->contact);
            }
        }
    case 2:
        if (em->motion[0].playing == 0) {
            em->step = 3;
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * Runs (KUTKU_MOT_RUN) toward the goal (Em_StepToGoal 0x40) until within 500
 * or the timer (EmKutku_SetAttackState: distance / 30 + 30, or 150 without a
 * target) runs out, then trips (KUTKU_MOT_CHARGE_TRIP) making KutkuAttack
 * 0x24, and ends.
 */
void EmKutku_ChargeTrip(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUTKU_MOT_RUN, 0, 0);
        break;
    case 1:
        if (em->hasGoal != 0) {
            Em_StepToGoal(em, 0x40, 1);
            if (em->goalDist <= 500.0f) {
                em->timer = 1;
            }
        }
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, KUTKU_MOT_CHARGE_TRIP, 0, 0);
            KutkuAttack_Create(em, 0x24);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * As EmKutku_SwoopThenLeave up to the claw (also pitching down from frame 46
 * of the dive); when the claw ends it brakes (accelZ, accelY -10) and drops
 * to the floor, levels and lands (KUTKU_MOT_LEAP_LAND), then ends with
 * EmKutku_EndAttack.
 */
void EmKutku_SwoopThenLand(Em *em, EmWork *work)
{
    f32 targetWorldPos[3];
    f32 targetPos[3];
    f32 targetPosB[3];
    AreaRecord *area;
    s8 idx;

    area = Stage_GetAreaRecord(em->area);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        break;
    case 1:
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 0xD1, &EmKutku_FlyParam);
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
            EmKutku_EndSwoop(em, (s32) work);
        }
        break;
    case 2:
        if (em->targetNo == -1) {
            EmKutku_EndSwoop(em, (s32) work);
            break;
        }
        if (80.0f < em->velZ) {
            em->velZ -= 1.0f;
        }
        idx = em->targetNo;
        Em_GetTargetPos(em, idx, targetPos);
        Area_ToWorld(((Player *) &gPlayers[idx])->area, targetPos, targetWorldPos);
        em->aimAngle = Math_AngleXZ(em->mapPos, targetWorldPos);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 0xD1, &EmKutku_FlyParam);
        if (em->aimAngle <= 0x800 || em->aimAngle >= 0xF800) {
            if (!(4000.0f < Vec_DistanceXZ(&em->posX, targetPos))) {
                EmKutku_EndSwoop(em, (s32) work);
            } else {
                em->step++;
                work->turnSpeed = 0x100;
                Em_SetMotion(em, KUTKU_MOT_SWOOP_DIVE, 0, 0);
                em->timer = 600;
            }
        }
        work->moveAngX = em->rotX;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
        if (--em->timer <= 0 || Em_AllPlayersHere(em) == 0) {
            EmKutku_EndSwoop(em, (s32) work);
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
            EmKutku_StepPitch(em, 0, -200.0f);
            if (50.0f < em->velZ) {
                em->velZ -= 0.1f;
            }
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUTKU_MOT_SWOOP, 0, 0);
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
            EmKutku_EndSwoop(em, (s32) work);
        }
        break;
    case 4:
        if (em->posY <= 1000.0f + area->bottom) {
            em->step++;
            em->timer = 600;
        }
        if (EmKutku_SwoopClawIfNear(em) && em->step == 4) {
            em->step++;
            em->timer = 600;
        }
        if (em->motionId[0] == EM_MOTION_ID(0, KUTKU_MOT_SWOOP_CLAW)) {
            EmKutku_StepPitch(em, 2, 0.0f);
        } else {
            EmKutku_StepPitch(em, 0, -200.0f);
        }
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 2);
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_GetPitchToTarget(em, &work->moveAngX, -3000.0f, 0.0f);
        if (em->wallHitDirs != 0) {
            work->moveAngX = 0;
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
            EmKutku_EndSwoop(em, (s32) work);
        }
        break;
    case 5:
        EmKutku_SwoopClawIfNear(em);
        if (em->motionId[0] == EM_MOTION_ID(0, KUTKU_MOT_SWOOP_CLAW) && em->motion[0].playing == 0) {
            em->step = 6;
            em->accelX = 0.0f;
            em->accelZ = -10.0f;
            em->accelY = -10.0f;
        }
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 2);
        EmKutku_StepPitch(em, 2, 100.0f);
        if (em->motionId[0] == EM_MOTION_ID(0, KUTKU_MOT_SWOOP_CLAW) && EmMotion_ReachedFrame(em, 0, 76.0f)) {
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
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        if (em->wallHitDirs != 0) {
            work->moveAngX = 0;
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
            EmKutku_EndSwoop(em, (s32) work);
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
            Em_SetMotion(em, KUTKU_MOT_LEAP_LAND, 0, 0);
        }
        break;
    case 7:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKutku_EndAttack(em, work);
        }
        break;
    }
    if (em->posY < em->floorY) {
        em->posY = em->floorY;
    }
}

/*
 * Turns to the target and plays KUTKU_MOT_SPIT; at the frame in
 * EmKutku_FxFrameBone[variant][0] it spits one projectile from bone 0x22 at
 * the angle in [variant][1]: Kut-Ku its fireball (EmRangedAttack type 3),
 * Purple Gypceros type 2 sub 1, Gypceros type 2 sub 0. Serves states 0x1A,
 * 0x17, 0x18, 0x19 (variants 0-3).
 */
void EmKutku_Spit(Em *em, EmWork *work, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
    case 1:
        if (Em_TurnToTargetMotion(em, 0xE38, 3, 6, 5) != 0) {
            em->step++;
            em->lookAtTarget = 0;
            Em_SetMotion(em, KUTKU_MOT_SPIT, 0, 0);
        }
        break;
    case 2:
        if (EmMotion_PassedFrameScaled(em, EmKutku_FxFrameBone[variant][0], 0U) != 0) {
            if (em->type == EM_KIND_YIAN_KUT_KU || em->type == EM_KIND_BLUE_YIAN_KUT_KU) {
                EmRangedAttack_CreateAtBone(em, 0x22, 3, 0, EmKutku_FxFrameBone[variant][1], 0);
            } else if (em->type == EM_KIND_PURPLE_GYPCEROS) {
                EmRangedAttack_CreateAtBone(em, 0x22, 2, 1, EmKutku_FxFrameBone[variant][1], 0);
            } else {
                EmRangedAttack_CreateAtBone(em, 0x22, 2, 0, EmKutku_FxFrameBone[variant][1], 0);
            }
        }
        if (em->motion[0].playing == 0) {
            em->lookAtTarget = 0;
            em->step++;
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * Turns to the target, then plays KUTKU_MOT_SPIT_VOLLEY (four projectiles at frames 78, 150,
 * 240, 296 at four angles, by kind as EmKutku_Fireball) and ends.
 */
void EmKutku_SpitVolley(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        
    case 1:
        if (Em_TurnToTargetMotion(em, 0xE38, 3, 6, 5) != 0) {
            em->step++;
            em->lookAtTarget = 0;
            Em_SetMotion(em, KUTKU_MOT_SPIT_VOLLEY, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->lookAtTarget = 0;
            em->step++;
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * Turns to the target, then plays KUTKU_MOT_PECK_COMBO_B (KutkuAttack 0x12 at frame 20, then
 * four 0x18 pecks) and ends.
 */
void EmKutku_PeckComboB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        
    case 1:
        if (Em_TurnToTargetMotion(em, 0xE38, 3, 6, 5) != 0) {
            em->step++;
            em->lookAtTarget = 0;
            Em_SetMotion(em, KUTKU_MOT_PECK_COMBO_B, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->lookAtTarget = 0;
            em->step++;
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * Plays 0x6B with EmFlyerWork.crestGlow set for its length; while partWhole is
 * set the motion effects flash at frame 40 (FireBlast at bone 35,
 * KutkuAttack_CreateFlash 0x16). Then ends.
 */
void EmKutku_Flash(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUTKU_MOT_FLASH, 0, 0);
        ((EmFlyerWork *)work)->crestGlow = 1;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->lookAtTarget = 0;
            em->step++;
            ((EmFlyerWork *)work)->crestGlow = 0;
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * The fireball of EmKutku_Fireball variant 0 without turning first.
 * EmKutku_EndStateOrSpit picks it (10%) after a flinch for Gypceros and a
 * calm Kut-Ku.
 */
void EmKutku_SpitAfterFlinch(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUTKU_MOT_SPIT, 0, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, EmKutku_FxFrameBone[0][0], 0U) != 0) {
            if (em->type == EM_KIND_YIAN_KUT_KU || em->type == EM_KIND_BLUE_YIAN_KUT_KU) {
                EmRangedAttack_CreateAtBone(em, 0x22, 3, 0, EmKutku_FxFrameBone[0][1], 0);
            } else if (em->type == EM_KIND_PURPLE_GYPCEROS) {
                EmRangedAttack_CreateAtBone(em, 0x22, 2, 1, EmKutku_FxFrameBone[0][1], 0);
            } else {
                EmRangedAttack_CreateAtBone(em, 0x22, 2, 0, EmKutku_FxFrameBone[0][1], 0);
            }
        }
        if (em->motion[0].playing == 0) {
            em->lookAtTarget = 0;
            em->step++;
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * The barrage 0x69 without turning first; EmKutku_EndStateOrSpit picks it
 * for an enraged Kut-Ku.
 */
void EmKutku_SpitVolleyAfterFlinch(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUTKU_MOT_SPIT_VOLLEY, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->lookAtTarget = 0;
            em->step++;
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * Restarts the script and plays KUTKU_MOT_FLINCH, then ends (EmKutku_EndStateOrSpit: 10%
 * a fireball). Update: a flinch of parts 0 and 7, or EM_DMG_HIT_MODE3 on the
 * ground.
 */
void EmKutku_Flinch(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, KUTKU_MOT_FLINCH, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKutku_EndStateOrSpit(em, 0, 0);
        }
        break;
    }
}

/*
 * As EmKutku_Flinch with KUTKU_MOT_FLINCH_B: a flinch of an unbroken part 3 or 4 (or
 * another part).
 */
void EmKutku_FlinchB(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, KUTKU_MOT_FLINCH_B, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKutku_EndStateOrSpit(em, 0, 0);
        }
        break;
    }
}

/*
 * As EmKutku_Flinch with KUTKU_MOT_FLINCH_C: a flinch of parts 5 and 6.
 */
void EmKutku_FlinchC(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, KUTKU_MOT_FLINCH_C, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKutku_EndStateOrSpit(em, 0, 0);
        }
        break;
    }
}

/*
 * As EmKutku_Flinch with KUTKU_MOT_FLINCH_D: a flinch of parts 1 and 2.
 */
void EmKutku_FlinchD(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, KUTKU_MOT_FLINCH_D, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKutku_EndStateOrSpit(em, 0, 0);
        }
        break;
    }
}

/*
 * Empty. Update enters it on EM_DMG_PART8_DOWN; nothing here leaves it (see
 * NOTE).
 */
void EmKutku_PartBreak(void)
{
}

/*
 * Knocked down (KUTKU_MOT_KNOCK_DOWN_B), lies (KUTKU_MOT_DOWN_B) 120 frames,
 * gets up (KUTKU_MOT_DOWN_GET_UP_B) and ends, then overwrites
 * prevStateGroup/prevState with the state just set. Update: a flinch of a
 * broken part other than 3.
 */
void EmKutku_KnockDown(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, KUTKU_MOT_KNOCK_DOWN_B, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 0;
            Em_SetMotion(em, KUTKU_MOT_DOWN_B, 0, 0);
        }
        break;
    case 2:
        em->timer++;
        if (em->timer >= 0x78) {
            em->step++;
            Em_SetMotion(em, KUTKU_MOT_DOWN_GET_UP_B, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKutku_EndState(em, 0, 0);
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
        }
        break;
    }
}

/*
 * As EmKutku_KnockDown with KUTKU_MOT_KNOCK_DOWN, 0x46, 0x47: a flinch of the broken part 3.
 */
void EmKutku_KnockDownPart3(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, KUTKU_MOT_KNOCK_DOWN, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 0;
            Em_SetMotion(em, KUTKU_MOT_DOWN, 0, 0);
        }
        break;
    case 2:
        em->timer++;
        if (em->timer >= 0x78) {
            em->step++;
            Em_SetMotion(em, KUTKU_MOT_DOWN_GET_UP, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKutku_EndState(em, 0, 0);
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
        }
        break;
    }
}

/*
 * In the air: starts the fall (KUTKU_MOT_FALL, velY at most -50) and from frame 14
 * recovers into the glide (EmKutku_EndStateInAir(1)) with velY 0. No caller
 * in this file.
 */
void EmKutku_AirFlinch(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUTKU_MOT_FALL, 0, 0);
        if (em->velY > -50.0f) {
            em->velY = -50.0f;
        }
        Em_RestartScript(em);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 14.0f, 0) != 0) {
            em->step++;
            EmKutku_EndStateInAir(em, 1);
            em->velY = 0.0f;
        }
        Em_MoveRotated(em, &work->moveAngX);
        break;
    }
}

/*
 * Falls (KUTKU_MOT_FALL, accelY -10) to the floor, lands (KUTKU_MOT_FALL_LAND), gets
 * up (KUTKU_MOT_DOWN_GET_UP) and ends (EmKutku_EndStateOrSpit). Update:
 * EM_DMG_STATUS_IN_AIR, or a flinch or an attack-5 hit while airborne.
 */
void EmKutku_FallFromAir(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, KUTKU_MOT_FALL, 0, 0);
        if (em->velY > -50.0f) {
            em->velY = -50.0f;
        }
        em->accelY = -10.0f;
        break;
    case 1:
        Em_MoveRotatedAccel(em, &work->moveAngX);
        if (em->posY <= em->floorY) {
            em->step++;
            em->posY = em->floorY;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, KUTKU_MOT_FALL_LAND, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUTKU_MOT_DOWN_GET_UP, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKutku_EndStateOrSpit(em, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    }
}

/*
 * As EmKutku_FallFromAir with blind 2 every frame and the fall straight
 * along rotY. No caller in this file.
 */
void EmKutku_FallFromAirBlind(Em *em, EmWork *work)
{
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, KUTKU_MOT_FALL, 0, 0);
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
            Em_SetMotion(em, KUTKU_MOT_FALL_LAND, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUTKU_MOT_DOWN_GET_UP, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKutku_EndStateOrSpit(em, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    }
}

/*
 * Plays 0x3D and ends (plain EmKutku_EndState). No caller in this file.
 */
void EmKutku_FlinchE(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, KUTKU_MOT_FLINCH_E, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * Under status 4 standing (EmKutku_SetReactState: Em_BeginStatus4): plays
 * 0x4C with noReact and the status hit mode, the status-4 effect each frame;
 * when the timer (Em_SetStatus4Timer) runs out, KUTKU_IDLE_STATUS4_SHAKE_OFF
 * (EmKutku_StatusShakeOff variant 0).
 */
void EmKutku_Status4(Em *em, EmWork *work)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_STATUS);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, KUTKU_MOT_STATUS4, 0, 0);
        em->noReact = 1;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            em->noReact = 0;
            EmKutku_ChangeState(em, EM_GROUP_IDLE, KUTKU_IDLE_STATUS4_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmKutku_ChangeState(em, EM_GROUP_IDLE, KUTKU_IDLE_STATUS4_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
    Em_SpawnStatus4Effect(em, 2);
}

/*
 * Caught (EM_DMG_ATTACK9/ATTACK6; EmKutku_SetReactState: holdTimer 50, all
 * statuses ended): plays KUTKU_MOT_TRAPPED unflinching with holdTimer kept at 5 or more
 * and blind cleared, then struggles (KUTKU_REACT_TRAPPED_STRUGGLE).
 */
void EmKutku_Trapped(Em *em, EmWork *work)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_NO_FLINCH);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, KUTKU_MOT_TRAPPED, 0, 0);
        em->noReact = 1;
        em->blind = 0;
        break;
    case 1:
        if ((s8) em->holdTimer < 5) {
            em->holdTimer = 5;
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKutku_ChangeState(em, EM_GROUP_REACT, KUTKU_REACT_TRAPPED_STRUGGLE, EM_SYNC_OWNER);
        }
        break;
    case 2:
        if ((s8) em->holdTimer < 5) {
            em->holdTimer = 5;
        }
        if (em->remote == 0) {
            EmKutku_ChangeState(em, EM_GROUP_REACT, KUTKU_REACT_TRAPPED_STRUGGLE, EM_SYNC_OWNER);
        }
        break;
    }
}

/*
 * Held (holdTimer 5): loops 0x60 until trapTimer runs out, then escapes
 * (KUTKU_FLY_TRAP_ESCAPE).
 */
void EmKutku_TrapStruggle(Em *em, EmWork *work)
{
    em->holdTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, KUTKU_MOT_STRUGGLE, 0, 0);
        em->noReact = 1;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            Em_SetMotion(em, KUTKU_MOT_STRUGGLE, 0, 0);
        }
        if (--em->trapTimer <= 0) {
            em->step++;
            Em_SetMotion(em, KUTKU_MOT_STRUGGLE, 0, 0);
            EmKutku_ChangeState(em, EM_GROUP_FLY, KUTKU_FLY_TRAP_ESCAPE, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if ((em->motion[0].playing == 0) && (em->remote == 0)) {
            Em_SetMotion(em, KUTKU_MOT_STRUGGLE, 0, 0);
            EmKutku_ChangeState(em, EM_GROUP_FLY, KUTKU_FLY_TRAP_ESCAPE, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * Under status 4 while held (EmKutku_SetReactState: Em_BeginStatus4): lies
 * downed (KUTKU_MOT_DOWNED) with the status hit mode and effect; when the
 * timer runs out it sets the trap time goal and goes to
 * KUTKU_REACT_STATUS4_DOWNED_END.
 */
void EmKutku_Status4Downed(Em *em, EmWork *work)
{
    em->holdTimer = 5;
    Em_SetHitReactMode(em, EM_HIT_REACT_STATUS);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, KUTKU_MOT_DOWNED, 0, 0);
        em->noReact = 1;
        Em_RestartScript(em);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetTrapTimeGoal(em);
            EmKutku_ChangeState(em, EM_GROUP_REACT, KUTKU_REACT_STATUS4_DOWNED_END, EM_SYNC_OWNER);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmKutku_ChangeState(em, EM_GROUP_REACT, KUTKU_REACT_STATUS4_DOWNED_END, EM_SYNC_OWNER);
        }
        break;
    }
    Em_SpawnStatus4Effect(em, 2);
}

/*
 * Kut-Ku only (Update: attack kind 5 on the ground, calm, not Gypceros):
 * plays KUTKU_MOT_ATTACK5_FLINCH, leaving a timed gathering point at its position, then KUTKU_MOT_THREAT
 * (blend 10) and ends.
 */
void EmKutku_Attack5Flinch(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, KUTKU_MOT_ATTACK5_FLINCH, 0, 0);
        EmMine_CreateTimed0(em, &em->posX);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUTKU_MOT_THREAT, 0xA, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * Gypceros below half hp (Update, outside levels 11-17;
 * EmKutku_FeignDeathCarvable inside them): collapses (KUTKU_MOT_DIE)
 * unhittable, then lies still (KUTKU_ATK_FEIGN_DEATH_LIE).
 */
void EmKutku_FeignDeathOnHit(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, KUTKU_MOT_DIE, 0, 0);
        em->noHitTimer = 5;
        break;
    case 1:
        em->noHitTimer = 5;
        if (EmMotion_PassedFrameScaled(em, 350.0f, 0U) != 0) {
            em->step++;
        }
        break;
    case 2:
        em->noHitTimer = 5;
        if (em->motion[0].playing == 0) {
            EmKutku_ChangeState(em, EM_GROUP_ATTACK, KUTKU_ATK_FEIGN_DEATH_LIE, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * Same code as EmKutku_TrapStruggle (0x60 until trapTimer ends, then
 * KUTKU_FLY_TRAP_ESCAPE); EmKutku_SetReactState ends status 4 on entry.
 */
void EmKutku_Status4DownedEnd(Em *em, EmWork *work)
{
    em->holdTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, KUTKU_MOT_STRUGGLE, 0, 0);
        em->noReact = 1;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            Em_SetMotion(em, KUTKU_MOT_STRUGGLE, 0, 0);
        }
        if (--em->trapTimer <= 0) {
            em->step++;
            Em_SetMotion(em, KUTKU_MOT_STRUGGLE, 0, 0);
            EmKutku_ChangeState(em, EM_GROUP_FLY, KUTKU_FLY_TRAP_ESCAPE, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if ((em->motion[0].playing == 0) && (em->remote == 0)) {
            Em_SetMotion(em, KUTKU_MOT_STRUGGLE, 0, 0);
            EmKutku_ChangeState(em, EM_GROUP_FLY, KUTKU_FLY_TRAP_ESCAPE, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * Held, blind: gets up (KUTKU_MOT_DOWNED_GET_UP) from lying downed, sets the
 * trap time goal and struggles (KUTKU_REACT_TRAPPED_STRUGGLE).
 * EmKutku_SetReactState ends status 2; entered from
 * KUTKU_IDLE_STATUS2_DOWNED.
 */
void EmKutku_Status2DownedEnd(Em *em, EmWork *work)
{
    em->holdTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUTKU_MOT_DOWNED_GET_UP, 0, 0);
        em->blind = 2;
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetTrapTimeGoal(em);
            em->blind = 0;
            EmKutku_ChangeState(em, EM_GROUP_REACT, KUTKU_REACT_TRAPPED_STRUGGLE, EM_SYNC_OWNER);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmKutku_ChangeState(em, EM_GROUP_REACT, KUTKU_REACT_TRAPPED_STRUGGLE, EM_SYNC_OWNER);
        }
        break;
    }
}

/*
 * Same code as EmKutku_Status2DownedEnd; EmKutku_SetReactState ends status
 * 1; entered from KUTKU_IDLE_STATUS1_DOWNED.
 */
void EmKutku_Status1DownedEnd(Em *em, EmWork *work)
{
    em->holdTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUTKU_MOT_DOWNED_GET_UP, 0, 0);
        em->blind = 2;
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetTrapTimeGoal(em);
            em->blind = 0;
            EmKutku_ChangeState(em, EM_GROUP_REACT, KUTKU_REACT_TRAPPED_STRUGGLE, EM_SYNC_OWNER);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmKutku_ChangeState(em, EM_GROUP_REACT, KUTKU_REACT_TRAPPED_STRUGGLE, EM_SYNC_OWNER);
        }
        break;
    }
}

/*
 * Gypceros only (Update: a flinch of part 6 once it is broken, while
 * partWhole is set; EmKutku_SetReactState clears partWhole and calls
 * Em_BreakPart(em, 1)): plays KUTKU_MOT_FLINCH_C with effect 0xA 90 above bone 0x27, then
 * ends (EmKutku_EndStateOrSpit).
 */
void EmKutku_Part6Break(Em *em, EmWork *work)
{
    f32 offset[3];
    f32 fxPos[3];
    f32 boneMtx[16];

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, KUTKU_MOT_FLINCH_C, 0, 0);
        offset[0] = 0.0f;
        offset[1] = 90.0f;
        offset[2] = 0.0f;
        Mtx_Copy(boneMtx, (f32 *)Em_Bone(em, 0x27));
        Vec_RotateSelf(offset, boneMtx);
        fxPos[0] = boneMtx[12] + offset[0];
        fxPos[1] = boneMtx[13] + offset[1];
        fxPos[2] = boneMtx[14] + offset[2];
        HitSpark_SpawnForOwner(em, 0, 0xA, 0, fxPos, 1.0f);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKutku_EndStateOrSpit(em, 0, 0);
        }
        break;
    }
}

/*
 * The stun when not held (Update: EM_DMG_STATUS5; EmKutku_SetReactState 21
 * calls Em_BeginStatus5): knock-down motion with stun stars over half its
 * length, lies down until the timer ends, gets up, then goes to
 * KUTKU_IDLE_STUN_SHAKE_OFF (EmKutku_StatusShakeOff shake-off;
 * EmKutku_SetIdleState 36 calls Em_EndStatus5).
 */
void EmKutku_Stun(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, KUTKU_MOT_KNOCK_DOWN, 0, 0);
        StunStars_CreateEm(em, 1, 0x22, (s32) ((f32) em->timer + (em->motion[0].frameCount / 2.0f)), 150.0f, 3.0f);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUTKU_MOT_DOWN, 0, 0);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, KUTKU_MOT_DOWN_GET_UP, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKutku_ChangeState(em, EM_GROUP_IDLE, KUTKU_IDLE_STUN_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 4:
        if (em->remote == 0) {
            EmKutku_ChangeState(em, EM_GROUP_IDLE, KUTKU_IDLE_STUN_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * The stun while held (holdTimer set; EmKutku_SetReactState 22 begins status
 * 5): keeps holdTimer at 5, lies downed (KUTKU_MOT_DOWNED) with stun stars
 * and noReact until the timer ends, then Em_SetTrapTimeGoal and on to
 * STUN_TRAPPED_STRUGGLE.
 */
void EmKutku_StunTrapped(Em *em, EmWork *work)
{
    em->holdTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, KUTKU_MOT_DOWNED, 0, 0);
        Em_RestartScript(em);
        em->noReact = 1;
        StunStars_CreateEm(em, 1, 0x22, (s32) ((f32) em->timer + (em->motion[0].frameCount / 2.0f)), 150.0f, 3.0f);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetTrapTimeGoal(em);
            EmKutku_ChangeState(em, EM_GROUP_REACT, KUTKU_REACT_STUN_TRAPPED_STRUGGLE, EM_SYNC_OWNER);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmKutku_ChangeState(em, EM_GROUP_REACT, KUTKU_REACT_STUN_TRAPPED_STRUGGLE, EM_SYNC_OWNER);
        }
        break;
    }
}

/*
 * Struggles in the trap (0x60 looped, holdTimer 5, noReact) until trapTimer
 * runs out, then KUTKU_FLY_TRAP_ESCAPE (the escape upward, as
 * EmKutku_TrapStruggle). EmKutku_SetReactState 23 ends status 5 on entry.
 */
void EmKutku_StunTrappedStruggle(Em *em, EmWork *work)
{
    em->holdTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, KUTKU_MOT_STRUGGLE, 0, 0);
        em->noReact = 1;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            Em_SetMotion(em, KUTKU_MOT_STRUGGLE, 0, 0);
        }
        if (--em->trapTimer <= 0) {
            em->step++;
            EmKutku_ChangeState(em, EM_GROUP_FLY, KUTKU_FLY_TRAP_ESCAPE, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmKutku_ChangeState(em, EM_GROUP_FLY, KUTKU_FLY_TRAP_ESCAPE, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * The EM_DMG_ATTACK13 reaction (Rathalos model: REACT_HOLD2): Em_SetHold2
 * every frame, flinch motion KUTKU_MOT_FLINCH without root motion to frame
 * 52, then motion KUTKU_MOT_STATUS4 until the timer ends, then
 * KUTKU_IDLE_HOLD2_SHAKE_OFF (EmKutku_StatusShakeOff shake-off;
 * EmKutku_SetIdleState 8 clears holdTimer2). EmKutku_SetReactState 24 ends
 * all statuses.
 */
void EmKutku_Hold2Flinch(Em *em, EmWork *work)
{
    Em_SetHold2(em);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, KUTKU_MOT_FLINCH, 0, 0);
        Player_SetFlag((Player *) em, EM_FLAG_NO_ROOT_MOTION);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 52.0f) != 0) {
            em->step++;
            Em_SetMotion(em, KUTKU_MOT_STATUS4, 0xC, 0);
            Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            EmKutku_ChangeState(em, EM_GROUP_IDLE, KUTKU_IDLE_HOLD2_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmKutku_ChangeState(em, EM_GROUP_IDLE, KUTKU_IDLE_HOLD2_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * Gypceros only (Update, part flinch at 30 or 50 percent hp, random, level
 * 11 to 17; other levels get EmKutku_FeignDeathOnHit): plays the death
 * motion KUTKU_MOT_DIE with noHitTimer and a camera shake at frame 212, adds
 * a mine point (carvable while playing dead), lies 300 frames, then
 * KUTKU_ATK_FEIGN_DEATH_RISE_B (EmKutku_FeignDeathRise: the feign get-up).
 */
void EmKutku_FeignDeathCarvable(Em *em, EmWork *work)
{
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, KUTKU_MOT_DIE, 0, 0);
        em->noHitTimer = 5;
        em->noReact = 1;
        em->timer = 300;
        break;
    case 1:
        em->noHitTimer = 5;
        if (EmMotion_PassedFrameScaled(em, 212.0f, 0) != 0) {
            CamShake_ReqAIf(em, 2);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKutku_AddMinePoint(em);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            EmKutku_ChangeState(em, EM_GROUP_ATTACK, KUTKU_ATK_FEIGN_DEATH_RISE_B, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmKutku_ChangeState(em, EM_GROUP_ATTACK, KUTKU_ATK_FEIGN_DEATH_RISE_B, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * The story scene in AREA_JUNGLE_9 (EmKutku_Update starts it in two quests
 * while EVENT_FLAG_KUTKU_SCENE is clear): placed at fixed points, flies
 * (KUTKU_MOT_FLY) then glides (KUTKU_MOT_GLIDE) toward a goal, hovers down
 * (KUTKU_MOT_HOVER, velY -8) and touches down
 * (KUTKU_MOT_TOUCH_DOWN), turns 90 degrees over half of motion
 * KUTKU_MOT_TURN_LEFT, idles with two layer motions, alert idle
 * (KUTKU_MOT_ALERT_IDLE), then motion KUTKU_MOT_SPIT_VOLLEY, and ends
 * (EndState) when EVENT_FLAG_KUTKU_SCENE is set. Keeps senseDelay/noHitTimer at 5
 * and the body above the floor.
 */
void EmKutku_JungleIntro(Em *em, EmWork *work)
{
    f32 goalDistA;
    f32 goalDistB;
    f32 turnStepF;
    f32 floorY;
    u32 turnStep;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        em->posX = 10051.0f;
        em->posY = 1040.0f;
        em->posZ = 8395.0f;
        em->goal[0] = 12865.0f;
        em->goal[1] = 1040.0f;
        em->goal[2] = 9646.0f;
        em->rotX = 0;
        em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        em->rotZ = 0;
        GmEm_ResetSize(em);
        Em_SetMotion(em, KUTKU_MOT_FLY, 0, 0);
        Player_ClearVelocity(em);
        em->velZ = 50.0f;
        work->moveAngX = 0;
        work->moveAngZ = 0;
        em->timer = 0x12C;
        em->fieldCollide = 0;
        break;
    case 1:
        work->moveAngY = em->rotY;
        Em_MoveRotated(em, &work->moveAngX);
        goalDistA = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer > 0) {
            if (goalDistA <= 500.0f) {
                goto block_7;
            }
        } else {
block_7:
            em->step++;
            em->posX = 11375.0f;
            em->posY = 1840.0f;
            em->posZ = 8983.0f;
            em->goal[0] = 12865.0f;
            em->goal[1] = 1840.0f;
            em->goal[2] = 9646.0f;
            em->rotX = 0;
            em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
            em->rotZ = 0;
            Em_SetMotion(em, KUTKU_MOT_GLIDE, 0, 0);
            Player_ClearVelocity(em);
            em->velZ = 50.0f;
            work->moveAngX = 0;
            work->moveAngZ = 0;
            em->timer = 0x12C;
            em->fieldCollide = 0;
        }
        break;
    case 2:
        work->moveAngY = em->rotY;
        Em_MoveRotated(em, &work->moveAngX);
        goalDistB = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer > 0) {
            if (goalDistB <= 500.0f) {
                goto block_11;
            }
        } else {
block_11:
            em->step++;
            em->fieldCollide = 1;
            Em_SetMotion(em, KUTKU_MOT_HOVER, 0, 0);
            Player_ClearVelocity(em);
            em->velY = -8.0f;
        }
        break;
    case 3:
        work->moveAngY = em->rotY;
        Em_MoveRotated(em, &work->moveAngX);
        if (!(em->floorY <= em->posY)) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, KUTKU_MOT_TOUCH_DOWN, 0, 0);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUTKU_MOT_TURN_LEFT, 0, 0);
            em->timer = 0;
            em->posX = 12426.0f;
            em->posY = 1040.0f;
            em->posZ = 9451.0f;
            em->goal[0] = 10051.0f;
            em->goal[1] = 1040.0f;
            em->goal[2] = 8395.0f;
            em->rotX = 0;
            em->rotY = (Math_AngleXZ(&em->posX, em->goal) & 0xFFFF) - 0x4000;
            em->rotZ = 0;
        }
        break;
    case 5:
        turnStepF = (16384.0f / (em->motion[0].frameCount / 2.0f)) * em->motionSpeed;
        turnStep = (u32) turnStepF;
        em->rotY += (s32) turnStep;
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUTKU_MOT_IDLE, 0, 0);
            em->timer = 0;
        }
        break;
    case 6:
        em->timer++;
        if (em->timer == 0x28) {
            Em_SetLayerMotion(em, EM_MOTION_ID(1, KUTKU_MOT_UPPER_D), 0xA, 0, 1);
        }
        if (em->timer == 0xA) {
            Em_SetLayerMotion(em, EM_MOTION_ID(2, 0x1B), 0xA, 0, 2);
        }
        if (em->motion[1].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUTKU_MOT_ALERT_IDLE, 0, 0);
            em->timer = 1;
        }
        break;
    case 7:
        if (em->motion[0].playing == 0) {
            if (--em->timer <= 0) {
                em->step++;
                Em_SetMotion(em, KUTKU_MOT_SPIT_VOLLEY, 0, 0);
            }
        }
        break;
    case 8:
        if (Save_TestEventFlag(EVENT_FLAG_KUTKU_SCENE) == 1) {
            em->step++;
            em->senseDelay = 0;
            em->noHitTimer = 0;
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * Captured in the holdTimer hold (Update: EM_DMG_CAPTURED while held):
 * downed motion KUTKU_MOT_DOWNED, reaction set 1, begins status 1, clears
 * poison, QuestEm_Captured; after 1800 frames (once QuestEm_CanAppear) fades
 * out over 150 and goes to DEFEAT_REAPPEAR. Lying puffs and a periodic sound
 * at bone 0x23 until the fade.
 */
void EmKutku_Captured(Em *em)
{
    f32 puffOfs[3];

    Em_SetCaptured(em, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->stateVar0 = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUTKU_MOT_DOWNED, 0, 0);
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
                EmKutku_ChangeState(em, EM_GROUP_DEFEAT, KUTKU_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    }
    if (em->step < 3) {
        puffOfs[0] = 0.0f;
        puffOfs[1] = 10.0f;
        puffOfs[2] = 140.0f;
        Em_SpawnLyingPuffs(em, 0x22, puffOfs, 1.6f);
        if (++em->stateVar0 % 135 == 0) {
            Em_PlaySeAtBone(em, 0x57, 0x23);
        }
    }
}

/*
 * Captured in the holdTimer2 hold: as EmKutku_Captured but collapses
 * (KUTKU_MOT_COLLAPSE) and lies (KUTKU_MOT_LIE), EM_STANCE_GROUND, and makes
 * no puffs or sound.
 */
void EmKutku_CapturedHold2(Em *em)
{
    s32 timeLeft;
    s32 prevTimer;

    Em_SetCaptured(em, 1);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUTKU_MOT_COLLAPSE, 0, 0);
        Em_LoadReactionSet(em, 1);
        Em_BeginStatus1(em);
        Em_ClearPoison(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUTKU_MOT_LIE, 0, 0);
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
        prevTimer = em->timer;
        if (prevTimer > 0) {
            em->timer = prevTimer - 1;
        }
        timeLeft = em->timer;
        if (timeLeft <= 0) {
            if (em->remote == 0) {
                EmKutku_ChangeState(em, EM_GROUP_DEFEAT, KUTKU_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
                break;
            }
        } else {
            em->alpha = (f32) timeLeft / 150.0f;
        }
        break;
    }
}

/*
 * Versus: when Versus_DefeatCamera is ready plays KUTKU_MOT_VS_BAIT (idle on layers 0 and
 * 2), creates the versus bait at frame 10 at EmKutku_CarveThrowPt, then
 * head-down (KUTKU_MOT_FORAGE), feed (KUTKU_MOT_FEED), alert idle
 * (KUTKU_MOT_ALERT_IDLE) for 30 frames, then Versus_RouteEmByTeam.
 */
void EmKutku_VsEatBait(Em *em, EmWork *work)
{
    Vec3f throwPt;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        if (Versus_DefeatCamera(em) == 1) {
            em->step++;
            Em_SetMotion(em, KUTKU_MOT_VS_BAIT, 0, 0);
            Em_SetLayerMotion(em, EM_MOTION_ID(0, KUTKU_MOT_IDLE), 0, 0, 0);
            Em_SetLayerMotion(em, EM_MOTION_ID(0, KUTKU_MOT_IDLE), 0, 0, 2);
            em->timer = 30;
        }
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0) {
            throwPt = EmKutku_CarveThrowPt;
            VersusBait_Create(&gPlayers[(u8) em->side - 1], em, 0, 0, &throwPt.x);
        }
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, KUTKU_MOT_FORAGE, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUTKU_MOT_FEED, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 30;
            Em_SetMotion(em, KUTKU_MOT_ALERT_IDLE, 0, 0);
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
 * Versus pose A, then VS_WAIT_BOTH: Kut-Ku plays KUTKU_MOT_ATTACK5_FLINCH; Gypceros plays the
 * death motion KUTKU_MOT_DIE then the feign get-up KUTKU_MOT_FEIGN_DEATH_RISE.
 */
void EmKutku_VsPoseA(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    if (em->type == EM_KIND_YIAN_KUT_KU || em->type == EM_KIND_BLUE_YIAN_KUT_KU) {
        switch (em->step) {
        case 0:
            em->step++;
            Em_SetMotion(em, KUTKU_MOT_ATTACK5_FLINCH, 0, 0);
            break;
        case 1:
            if (em->motion[0].playing == 0) {
                em->step++;
                Em_SetMotion(em, KUTKU_MOT_IDLE, 0, 0);
                EmKutku_ChangeState(em, EM_GROUP_VERSUS, KUTKU_VS_WAIT_BOTH, EM_SYNC_SEND);
            }
            break;
        }
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            Em_SetMotion(em, KUTKU_MOT_DIE, 0, 0);
            break;
        case 1:
            if (em->motion[0].playing == 0) {
                em->step++;
                Em_SetMotion(em, KUTKU_MOT_FEIGN_DEATH_RISE, 0, 0);
            }
            break;
        case 2:
            if (em->motion[0].playing == 0) {
                em->step++;
                Em_SetMotion(em, KUTKU_MOT_IDLE, 0, 0);
                EmKutku_ChangeState(em, EM_GROUP_VERSUS, KUTKU_VS_WAIT_BOTH, EM_SYNC_SEND);
            }
            break;
        }
    }
}

/*
 * Versus pose B, then VS_WAIT_BOTH: Kut-Ku plays KUTKU_MOT_TURN_TO_GOAL; Gypceros plays KUTKU_MOT_THREAT.
 */
void EmKutku_VsPoseB(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    if (em->type == EM_KIND_YIAN_KUT_KU || em->type == EM_KIND_BLUE_YIAN_KUT_KU) {
        switch (em->step) {
        case 0:
            em->step++;
            Em_SetMotion(em, KUTKU_MOT_TURN_TO_GOAL, 0, 0);
            break;
        case 1:
            if (em->motion[0].playing == 0) {
                em->step++;
                Em_SetMotion(em, KUTKU_MOT_IDLE, 0, 0);
                EmKutku_ChangeState(em, EM_GROUP_VERSUS, KUTKU_VS_WAIT_BOTH, EM_SYNC_SEND);
            }
            break;
        }
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            Em_SetMotion(em, KUTKU_MOT_THREAT, 0, 0);
            break;
        case 1:
            if (em->motion[0].playing == 0) {
                em->step++;
                Em_SetMotion(em, KUTKU_MOT_IDLE, 0, 0);
                EmKutku_ChangeState(em, EM_GROUP_VERSUS, KUTKU_VS_WAIT_BOTH, EM_SYNC_SEND);
            }
            break;
        }
    }
}

/*
 * Versus pose C, then VS_WAIT_BOTH: Kut-Ku plays KUTKU_MOT_TURN_IN_PLACE; Gypceros plays the
 * flash 0x6B.
 */
void EmKutku_VsPoseC(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    if (em->type == EM_KIND_YIAN_KUT_KU || em->type == EM_KIND_BLUE_YIAN_KUT_KU) {
        switch (em->step) {
        case 0:
            em->step++;
            Em_SetMotion(em, KUTKU_MOT_TURN_IN_PLACE, 0, 0);
            break;
        case 1:
            if (em->motion[0].playing == 0) {
                em->step++;
                Em_SetMotion(em, KUTKU_MOT_IDLE, 0, 0);
                EmKutku_ChangeState(em, EM_GROUP_VERSUS, KUTKU_VS_WAIT_BOTH, EM_SYNC_SEND);
            }
            break;
        }
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            Em_SetMotion(em, KUTKU_MOT_FLASH, 0, 0);
            break;
        case 1:
            if (em->motion[0].playing == 0) {
                em->step++;
                Em_SetMotion(em, KUTKU_MOT_IDLE, 0, 0);
                EmKutku_ChangeState(em, EM_GROUP_VERSUS, KUTKU_VS_WAIT_BOTH, EM_SYNC_SEND);
            }
            break;
        }
    }
}

/*
 * Idles (unless already on the idle motion) until Versus_BothDefeated, then
 * VS_END. ChangeState calls Versus_MarkDefeat on entry.
 */
void EmKutku_VsWaitBoth(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        if (em->motionId[0] != EM_MOTION_ID(0, KUTKU_MOT_IDLE)) {
            Em_SetMotion(em, KUTKU_MOT_IDLE, 0, 0);
        }
        
    case 1:
        if (Versus_BothDefeated() == 1) {
            em->step++;
            EmKutku_ChangeState(em, EM_GROUP_VERSUS, KUTKU_VS_END, EM_SYNC_SEND);
        }
        break;
    }
}

/*
 * Idles 90 frames, clears senseDelay/noHitTimer and ends the state
 * (EndState).
 */
void EmKutku_VsEnd(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 0x5A;
        if (em->motionId[0] != EM_MOTION_ID(0, KUTKU_MOT_IDLE)) {
            Em_SetMotion(em, KUTKU_MOT_IDLE, 0, 0);
        }
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            em->senseDelay = 0;
            em->noHitTimer = 0;
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * Killed on the ground (Update: EM_DMG_KILLED/DEAD, not held, not in the
 * air): death motion KUTKU_MOT_DIE, EM_STANCE_DEAD, QuestEm_Hunted, camera
 * shake at frame 212; then a carve mine (EmMine_Create) for 1800 frames once
 * QuestEm_CanAppear, removes it, fades over 150 frames and goes to
 * DEFEAT_REAPPEAR.
 */
void EmKutku_Die(Em *em, EmWork *work)
{
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, KUTKU_MOT_DIE, 0, 0);
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
                EmKutku_ChangeState(em, EM_GROUP_DEFEAT, KUTKU_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    case 99:
        if (em->motion[0].playing == 0) {
            em->stance = EM_STANCE_GROUND;
            EmKutku_EndState(em, 0, 0);
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
        }
        break;
    }
}

/*
 * Killed while held (holdTimer): as EmKutku_Die with the downed motion
 * KUTKU_MOT_DOWNED and holdTimer kept at 5 until the fade.
 */
void EmKutku_DieHeld(Em *em, EmWork *work)
{
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, KUTKU_MOT_DOWNED, 0, 0);
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
                EmKutku_ChangeState(em, EM_GROUP_DEFEAT, KUTKU_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    case 99:
        if (em->motion[0].playing == 0) {
            em->stance = EM_STANCE_GROUND;
            EmKutku_EndState(em, 0, 0);
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
        }
        break;
    }
}

/*
 * Killed in the air: falls (KUTKU_MOT_FALL, velY at most -50, accel -10) to the floor,
 * lands (KUTKU_MOT_FALL_LAND), at frame 60 plays KUTKU_MOT_DIE_FALLING with EM_STANCE_DEAD
 * and QuestEm_Hunted, then mine, fade and reappear as EmKutku_Die.
 */
void EmKutku_DieFalling(Em *em, EmWork *work)
{
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUTKU_MOT_FALL, 0, 0);
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
            Em_SetMotion(em, KUTKU_MOT_FALL_LAND, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    case 2:
        if (EmMotion_PassedFrameScaled(em, 60.0f, 0) != 0) {
            em->step++;
            em->timer = 0;
            Em_SetMotion(em, KUTKU_MOT_DIE_FALLING, 6, 0);
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
                EmKutku_ChangeState(em, EM_GROUP_DEFEAT, KUTKU_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    case 99:
        if (em->motion[0].playing == 0) {
            em->stance = EM_STANCE_GROUND;
            EmKutku_EndState(em, 0, 0);
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
        }
        break;
    }
}

/*
 * Waits for QuestEm_CanAppear (counts taskPhase up while it cannot): in the
 * Great Arena puts the monster at one of two fixed spots by variant and goes
 * to KUTKU_IDLE; elsewhere sets posY to the area ground (gAreaGroundY),
 * EM_STANCE_AIR, and goes to KUTKU_FLY_LAND (FlyInLand). ChangeState calls
 * QuestEm_Appear on entry.
 */
void EmKutku_Reappear(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        if (QuestEm_CanAppear(em) == 1) {
            switch (Map_DayId(gScene.map)) {
            case MAP_GREAT_ARENA:
                em->area = Area_IdForCurrentTime(AREA_GREAT_ARENA_1);
                if (em->variant == 0) {
                    em->rotY = 0x8A76;
                    em->posX = 11948.9f;
                    em->posY = 1010.0f;
                    em->posZ = 15923.8f;
                } else {
                    em->rotY = 0x4861;
                    em->posX = 3743.2f;
                    em->posY = 1010.0f;
                    em->posZ = 11415.0f;
                }
                EmKutku_ChangeState(em, EM_GROUP_IDLE, KUTKU_IDLE, EM_SYNC_LOCAL);
                break;
            default:
                em->posY = gAreaGroundY[em->area];
                em->stance = EM_STANCE_AIR;
                EmKutku_ChangeState(em, EM_GROUP_FLY, KUTKU_FLY_LAND, EM_SYNC_LOCAL);
                break;
            }
            break;
        }
        em->taskPhase++;
        break;
    }
}

/*
 * Group 0 (idle) dispatcher on em->state; EmKutku_StatusShakeOff serves 8,
 * 0x22 and 0x24 (variants 2, 0, 1), EmKutku_IdleDisplay serves 0x21, 0x23
 * and 0x25.
 */
void EmKutku_RunIdleState(Em *em, EmWork *work)
{
    switch (em->state) {
    case KUTKU_IDLE_STAND:
        EmKutku_IdleStand(em);
        break;
    case KUTKU_IDLE:
        EmKutku_Idle(em);
        break;
    case KUTKU_IDLE_ACTION_A:
        EmKutku_IdleActionA(em);
        break;
    case KUTKU_IDLE_ACTION_B:
        EmKutku_IdleActionB(em);
        break;
    case KUTKU_IDLE_ONE_LOOP:
        EmKutku_IdleOneLoop(em);
        break;
    case KUTKU_IDLE_UPPER_A:
        EmKutku_IdleUpperA(em);
        break;
    case KUTKU_IDLE_UPPER_B:
        EmKutku_IdleUpperB(em);
        break;
    case KUTKU_IDLE_UPPER_B_QUIET:
        EmKutku_IdleUpperBQuiet(em);
        break;
    case KUTKU_IDLE_HOLD2_SHAKE_OFF:
        EmKutku_StatusShakeOff(em, work, 2);
        break;
    case KUTKU_IDLE_PAIR_A:
        EmKutku_IdlePairA(em);
        break;
    case KUTKU_IDLE_TURN_TO_GOAL:
        EmKutku_IdleTurnToGoal(em);
        break;
    case KUTKU_IDLE_UPPER_C:
        EmKutku_IdleUpperC(em);
        break;
    case KUTKU_IDLE_ACTION_C:
        EmKutku_IdleActionC(em);
        break;
    case KUTKU_IDLE_UPPER_D:
        EmKutku_IdleUpperD(em);
        break;
    case KUTKU_IDLE_EAT_NEED2:
        EmKutku_EatNeed2(em);
        break;
    case KUTKU_IDLE_PAIR_B:
        EmKutku_IdlePairB(em);
        break;
    case KUTKU_IDLE_PAIR_C:
        EmKutku_IdlePairC(em);
        break;
    case KUTKU_IDLE_ALERT:
        EmKutku_IdleAlert(em);
        break;
    case KUTKU_IDLE_REST:
        EmKutku_Rest(em);
        break;
    case KUTKU_IDLE_EAT_START:
        EmKutku_EatStart(em);
        break;
    case KUTKU_IDLE_STATUS2_COLLAPSE:
        EmKutku_Status2Collapse(em);
        break;
    case KUTKU_IDLE_STATUS2_LIE_DOWN:
        EmKutku_Status2LieDown(em);
        break;
    case KUTKU_IDLE_ACTION_D:
        EmKutku_IdleActionD(em);
        break;
    case KUTKU_IDLE_REST_END:
        EmKutku_RestEnd(em);
        break;
    case KUTKU_IDLE_STATUS2_END:
        EmKutku_Status2End(em);
        break;
    case KUTKU_IDLE_SHAKE_OFF:
        EmKutku_ShakeOff(em);
        break;
    case KUTKU_IDLE_EAT_NEED1:
        EmKutku_EatNeed1(em);
        break;
    case KUTKU_IDLE_STATUS1_COLLAPSE:
        EmKutku_Status1Collapse(em);
        break;
    case KUTKU_IDLE_STATUS1_END:
        EmKutku_Status1End(em);
        break;
    case KUTKU_IDLE_STATUS2_DOWNED:
        EmKutku_Status2Downed(em, work);
        break;
    case KUTKU_IDLE_STATUS1_DOWNED:
        EmKutku_Status1Downed(em, work);
        break;
    case KUTKU_IDLE_DISPLAY:
        EmKutku_IdleDisplay(em);
        break;
    case KUTKU_IDLE_STATUS4_SHAKE_OFF:
        EmKutku_StatusShakeOff(em, work, 0);
        break;
    case KUTKU_IDLE_DISPLAY_RAGE:
        EmKutku_IdleDisplay(em);
        break;
    case KUTKU_IDLE_STUN_SHAKE_OFF:
        EmKutku_StatusShakeOff(em, work, 1);
        break;
    case KUTKU_IDLE_DISPLAY_PENDING_RAGE:
        EmKutku_IdleDisplay(em);
        break;
    case KUTKU_IDLE_EAT:
        EmKutku_Eat(em);
        break;
    }
}

/*
 * Group 1 (move) dispatcher; EmKutku_Run serves 4 and 8.
 */
void EmKutku_RunMoveState(Em *em, EmWork *work)
{
    switch (em->state) {
    case KUTKU_MOVE_WALK:
        EmKutku_Walk(em);
        break;
    case KUTKU_MOVE_END:
        EmKutku_MoveEnd(em);
        break;
    case KUTKU_MOVE_WALK_B:
        EmKutku_WalkB(em);
        break;
    case KUTKU_MOVE_TURN:
        EmKutku_Turn(em);
        break;
    case KUTKU_MOVE_RUN:
        EmKutku_Run(em);
        break;
    case KUTKU_MOVE_TURN_IN_COMBAT:
        EmKutku_TurnInCombat(em);
        break;
    case KUTKU_MOVE_LIMP:
        EmKutku_Limp(em);
        break;
    case KUTKU_MOVE_TURN_TO_GOAL:
        EmKutku_TurnToGoal(em, work);
        break;
    case KUTKU_MOVE_RUN_SLOW:
        EmKutku_Run(em);
        break;
    case KUTKU_MOVE_NOP:
        EmKutku_MoveNop();
        break;
    case KUTKU_MOVE_WALK_B_AND_TURN:
        EmKutku_WalkBAndTurn(em);
        
    default:
        break;
    }
}

/*
 * Group 2 (fly) dispatcher, states 0 to 24.
 */
void EmKutku_RunFlyState(Em *em, EmWork *work)
{
    switch (em->state) {
    case KUTKU_FLY_TAKE_OFF:
        EmKutku_TakeOff(em, work);
        break;
    case KUTKU_FLY_LAND:
        EmKutku_Land(em, work);
        break;
    case KUTKU_FLY_HOVER:
        EmKutku_Hover(em, work);
        break;
    case KUTKU_FLY_HOP_TO_GOAL:
        EmKutku_HopToGoal(em, work);
        break;
    case KUTKU_FLY_HOP_FORWARD:
        EmKutku_HopForward(em, work);
        break;
    case KUTKU_FLY_HOP_BACK:
        EmKutku_HopBack(em, work);
        break;
    case KUTKU_FLY_ROUTE:
        EmKutku_FlyRoute(em, work);
        break;
    case KUTKU_FLY_HOVER_TO_GOAL:
        EmKutku_HoverToGoal(em, work);
        break;
    case KUTKU_FLY_OUT_OF_AREA:
        EmKutku_FlyOutOfArea(em, work);
        break;
    case KUTKU_FLY_TO_GOAL_AND_LAND:
        EmKutku_FlyToGoalAndLand(em, work);
        break;
    case KUTKU_FLY_END_TO_GLIDE:
        EmKutku_FlyEndToGlide(em);
        break;
    case KUTKU_FLY_GLIDE_TO_GOAL_AND_LAND:
        EmKutku_GlideToGoalAndLand(em, work);
        break;
    case KUTKU_FLY_BACK_HOP:
        EmKutku_BackHop(em);
        break;
    case KUTKU_FLY_THROUGH_AREA:
        EmKutku_FlyThroughArea(em, work);
        break;
    case KUTKU_FLY_HOVER_ONCE:
        EmKutku_HoverOnce(em, work);
        break;
    case KUTKU_FLY_GLIDE_ONCE:
        EmKutku_GlideOnce(em, work);
        break;
    case KUTKU_FLY_HOVER_TURN:
        EmKutku_HoverTurn(em, work);
        break;
    case KUTKU_FLY_ROUTE_LOOP:
        EmKutku_FlyRouteLoop(em, work);
        break;
    case KUTKU_FLY_LEVEL_OUT_AND_LAND:
        EmKutku_LevelOutAndLand(em, work);
        break;
    case KUTKU_FLY_GLIDE_TO_GOAL:
        EmKutku_GlideToGoal(em, work);
        break;
    case KUTKU_FLY_HOP_IN_PLACE:
        EmKutku_HopInPlace(em, work);
        break;
    case KUTKU_FLY_HOVER_STRAIGHT_TO_GOAL:
        EmKutku_HoverStraightToGoal(em, work);
        break;
    case KUTKU_FLY_LEVEL_OUT:
        EmKutku_LevelOut(em, work);
        break;
    case KUTKU_FLY_TRAP_ESCAPE:
        EmKutku_TrapEscape(em, work);
        break;
    case KUTKU_FLY_HOVER_RISE:
        EmKutku_HoverRise(em, work);
        
    default:
        break;
    }
}

/*
 * Group 3 (attack) dispatcher; EmKutku_Spit serves 0x17-0x1A (variants 1, 2,
 * 3, 0), EmKutku_FeignDeathRise serves 7 and 0x20.
 */
void EmKutku_RunAttackState(Em *em, EmWork *work)
{
    switch (em->state) {
    case KUTKU_ATK_TURN_AND_STARE:
        EmKutku_TurnAndStare(em);
        break;
    case KUTKU_ATK_LEAP:
        EmKutku_Leap(em, work);
        break;
    case KUTKU_ATK_STARE:
        EmKutku_Stare(em, work);
        break;
    case KUTKU_ATK_FEIGN_DEATH:
        EmKutku_FeignDeath(em);
        break;
    case KUTKU_ATK_TAIL_SPIN:
        EmKutku_TailSpin(em, work);
        break;
    case KUTKU_ATK_FEIGN_DEATH_RISE:
        EmKutku_FeignDeathRise(em);
        break;
    case KUTKU_ATK_SWOOP_THEN_LEAVE:
        EmKutku_SwoopThenLeave(em, work);
        break;
    case KUTKU_ATK_FEIGN_DEATH_LIE:
        EmKutku_FeignDeathLie(em, work);
        break;
    case KUTKU_ATK_PECK_COMBO:
        EmKutku_PeckCombo(em);
        break;
    case KUTKU_ATK_PECK_STEAL:
        EmKutku_PeckSteal(em, work);
        break;
    case KUTKU_ATK_CHARGE_TRIP:
        EmKutku_ChargeTrip(em);
        break;
    case KUTKU_ATK_SWOOP_THEN_LAND:
        EmKutku_SwoopThenLand(em, work);
        break;
    case KUTKU_ATK_SPIT_B:
        EmKutku_Spit(em, work, 1);
        break;
    case KUTKU_ATK_SPIT_C:
        EmKutku_Spit(em, work, 2);
        break;
    case KUTKU_ATK_SPIT_D:
        EmKutku_Spit(em, work, 3);
        break;
    case KUTKU_ATK_SPIT:
        EmKutku_Spit(em, work, 0);
        break;
    case KUTKU_ATK_SPIT_VOLLEY:
        EmKutku_SpitVolley(em);
        break;
    case KUTKU_ATK_PECK_COMBO_B:
        EmKutku_PeckComboB(em);
        break;
    case KUTKU_ATK_FLASH:
        EmKutku_Flash(em, work);
        break;
    case KUTKU_ATK_SPIT_AFTER_FLINCH:
        EmKutku_SpitAfterFlinch(em, work);
        break;
    case KUTKU_ATK_SPIT_VOLLEY_AFTER_FLINCH:
        EmKutku_SpitVolleyAfterFlinch(em);
        break;
    case KUTKU_ATK_FEIGN_DEATH_RISE_B:
        EmKutku_FeignDeathRise(em);
        break;
    }
}

/*
 * Group 4 (react) dispatcher, states 0 to 25.
 */
void EmKutku_RunReactState(Em *em, EmWork *work)
{
    switch (em->state) {
    case KUTKU_REACT_FLINCH:
        EmKutku_Flinch(em, work);
        break;
    case KUTKU_REACT_FLINCH_B:
        EmKutku_FlinchB(em, work);
        break;
    case KUTKU_REACT_FLINCH_C:
        EmKutku_FlinchC(em, work);
        break;
    case KUTKU_REACT_FLINCH_D:
        EmKutku_FlinchD(em, work);
        break;
    case KUTKU_REACT_PART_BREAK:
        EmKutku_PartBreak();
        break;
    case KUTKU_REACT_KNOCK_DOWN:
        EmKutku_KnockDown(em, work);
        break;
    case KUTKU_REACT_KNOCK_DOWN_PART3:
        EmKutku_KnockDownPart3(em, work);
        break;
    case KUTKU_REACT_AIR_FLINCH:
        EmKutku_AirFlinch(em, work);
        break;
    case KUTKU_REACT_FALL:
        EmKutku_FallFromAir(em, work);
        break;
    case KUTKU_REACT_FALL_B:
        EmKutku_FallFromAirBlind(em, work);
        break;
    case KUTKU_REACT_FLINCH_E:
        EmKutku_FlinchE(em, work);
        break;
    case KUTKU_REACT_STATUS4:
        EmKutku_Status4(em, work);
        break;
    case KUTKU_REACT_TRAPPED:
        EmKutku_Trapped(em, work);
        break;
    case KUTKU_REACT_TRAPPED_STRUGGLE:
        EmKutku_TrapStruggle(em, work);
        break;
    case KUTKU_REACT_STATUS4_DOWNED:
        EmKutku_Status4Downed(em, work);
        break;
    case KUTKU_REACT_ATTACK5:
        EmKutku_Attack5Flinch(em, work);
        break;
    case KUTKU_REACT_FEIGN_DEATH:
        EmKutku_FeignDeathOnHit(em, work);
        break;
    case KUTKU_REACT_STATUS4_DOWNED_END:
        EmKutku_Status4DownedEnd(em, work);
        break;
    case KUTKU_REACT_STATUS2_DOWNED_END:
        EmKutku_Status2DownedEnd(em, work);
        break;
    case KUTKU_REACT_STATUS1_DOWNED_END:
        EmKutku_Status1DownedEnd(em, work);
        break;
    case KUTKU_REACT_PART6_BREAK:
        EmKutku_Part6Break(em, work);
        break;
    case KUTKU_REACT_STUN:
        EmKutku_Stun(em, work);
        break;
    case KUTKU_REACT_STUN_TRAPPED:
        EmKutku_StunTrapped(em, work);
        break;
    case KUTKU_REACT_STUN_TRAPPED_STRUGGLE:
        EmKutku_StunTrappedStruggle(em, work);
        break;
    case KUTKU_REACT_HOLD2:
        EmKutku_Hold2Flinch(em, work);
        break;
    case KUTKU_REACT_FEIGN_DEATH_CARVABLE:
        EmKutku_FeignDeathCarvable(em, work);
        
    default:
        break;
    }
}

/*
 * Group 5 (defeat) dispatcher; keeps noPushTimer at 5.
 */
void EmKutku_RunDefeatState(Em *em, EmWork *work)
{
    em->noPushTimer = 5;
    switch (em->state) {
    case KUTKU_DEFEAT_DIE:
        EmKutku_Die(em, work);
        break;
    case KUTKU_DEFEAT_DIE_HELD:
        EmKutku_DieHeld(em, work);
        break;
    case KUTKU_DEFEAT_DIE_FALLING:
        EmKutku_DieFalling(em, work);
        break;
    case KUTKU_DEFEAT_REAPPEAR:
        EmKutku_Reappear(em, work);
        break;
    }
}

/*
 * Event dispatcher for groups 6 and 7 (RunState sends both): 0 intro, 4 and
 * 5 captured.
 */
void EmKutku_RunEventState(Em *em, s32 work)
{
    switch (em->state) {
    case KUTKU_EVENT_INTRO:
        EmKutku_JungleIntro(em, (EmWork *) work);
        break;
    case KUTKU_EVENT_CAPTURED:
        EmKutku_Captured(em);
        break;
    case KUTKU_EVENT_CAPTURED_HOLD2:
        EmKutku_CapturedHold2(em);
        break;
    }
}

/*
 * Versus dispatcher: 0 to 3, 0x98, 0x99.
 */
void EmKutku_RunVersusState(Em *em, EmWork *work)
{
    switch (em->state) {
    case KUTKU_VS_EAT_BAIT:
        EmKutku_VsEatBait(em, work);
        break;
    case KUTKU_VS_POSE_A:
        EmKutku_VsPoseA(em, work);
        break;
    case KUTKU_VS_POSE_B:
        EmKutku_VsPoseB(em, work);
        break;
    case KUTKU_VS_POSE_C:
        EmKutku_VsPoseC(em, work);
        break;
    case KUTKU_VS_WAIT_BOTH:
        EmKutku_VsWaitBoth(em, work);
        break;
    case KUTKU_VS_END:
        EmKutku_VsEnd(em, work);
        break;
    }
}

/*
 * Per-frame update (the monster task kind table): combat timer and area
 * trigger; crestGlow flicker each frame while enraged (0 in group 5 or while
 * feigning death); per-map need decay and low-hp checks unless noAlert or
 * enraged; counts fxMotion down; maps the Em_UpdateDamage result to
 * defeat/react/idle/event states (see NUMBER lines); breaks part 1 for
 * Kut-Ku when part 6 is broken; starts the Jungle intro event; runs the
 * state (twice on stateRequest); in the fly group sets guardTimer while
 * flyGuard is 1.
 */
void EmKutku_Update(Em *em)
{
    u8 dmgInfo[4];
    EmFlyerWork *flyWork = (EmFlyerWork *)&em->work;

    Em_UpdateCombatTimer(em);
    Em_RaisePlayerAreaTriggerLax(em);
    if (em->stateGroup != EM_GROUP_DEFEAT) {
        if (EmKutku_IsFeigningDeath(em) == 1) {
            flyWork->crestGlow = 0;
        } else if (em->rage != 0) {
            flyWork->crestGlow ^= 1;
        } else {
            flyWork->crestGlow = 0;
        }
    } else {
        flyWork->crestGlow = 0;
    }
    if (em->noAlert != 1 && em->rage == 0) {
        switch (Map_DayId(gScene.map)) {
        case MAP_GREAT_ARENA:
        case MAP_ARENA:
            break;
        case MAP_SWAMP:
            Em_CheckLowHp(em, 0.2f);
            Em_DecayNeed2(em);
            Em_CheckCarryRequest(em);
            Em_DecayNeed1Only(em);
            break;
        case MAP_FOREST_HILLS:
            Em_CheckLowHp(em, 0.2f);
            Em_DecayNeed2(em);
            Em_DecayNeed1(em);
            break;
        case MAP_JUNGLE:
            Em_CheckLowHp(em, 0.2f);
            Em_DecayNeed2(em);
            Em_DecayNeed1Only(em);
            break;
        default:
            Em_DecayNeed1Only(em);
            break;
        }
    }
    if (flyWork->fxMotion != 0) {
        flyWork->fxMotion -= 1;
    }
    switch (Em_UpdateDamage(em, dmgInfo) & 0xFF) {
    case EM_DMG_NONE:
    case EM_DMG_FLASH:
    case EM_DMG_HIT:
        break;
    case EM_DMG_KILLED:
    case EM_DMG_DEAD:
        if (em->stance == EM_STANCE_AIR) {
            EmKutku_ChangeState(em, EM_GROUP_DEFEAT, KUTKU_DEFEAT_DIE_FALLING, EM_SYNC_OWNER);
        } else if ((s8) em->holdTimer != 0) {
            EmKutku_ChangeState(em, EM_GROUP_DEFEAT, KUTKU_DEFEAT_DIE_HELD, EM_SYNC_OWNER);
        } else {
            EmKutku_ChangeState(em, EM_GROUP_DEFEAT, KUTKU_DEFEAT_DIE, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_ATTACK9:
    case EM_DMG_ATTACK6:
        if ((s8) em->holdTimer == 0) {
            Em_SetTrapTime(em, dmgInfo);
            Em_SetTrapTimeGoal(em);
            EmKutku_ChangeState(em, EM_GROUP_REACT, KUTKU_REACT_TRAPPED, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_STATUS5:
        if ((s8) em->holdTimer != 0) {
            Em_SetStatus5Timer(em);
            EmKutku_ChangeState(em, EM_GROUP_REACT, KUTKU_REACT_STUN_TRAPPED, EM_SYNC_OWNER);
        } else if ((em->stateGroup != EM_GROUP_REACT || em->state != KUTKU_REACT_STUN) && (em->stateGroup != EM_GROUP_REACT || em->state != KUTKU_REACT_FALL)) {
            Em_SetStatus5Timer(em);
            EmKutku_ChangeState(em, EM_GROUP_REACT, KUTKU_REACT_STUN, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_ATTACK13:
        Em_ResetKindTimer(em);
        EmKutku_ChangeState(em, EM_GROUP_REACT, KUTKU_REACT_HOLD2, EM_SYNC_OWNER);
        break;
    case EM_DMG_ATTACK5:
        if (em->stateGroup == EM_GROUP_REACT && em->state == KUTKU_REACT_ATTACK5) {
            break;
        }
        if ((s8) em->holdTimer != 0) {
            break;
        }
        if (em->stance == EM_STANCE_AIR) {
            if (em->rage == 0) {
                Em_CheckRageByHp(em, em->rageThreshold);
            }
            EmKutku_ChangeState(em, EM_GROUP_REACT, KUTKU_REACT_FALL, EM_SYNC_OWNER);
            break;
        }
        if (em->rage == 0 && em->type != EM_KIND_GYPCEROS && em->type != EM_KIND_PURPLE_GYPCEROS) {
            Em_CheckRageByHp(em, em->rageThreshold);
            EmKutku_ChangeState(em, EM_GROUP_REACT, KUTKU_REACT_ATTACK5, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_STATUS_IN_AIR:
        if (em->stateGroup != EM_GROUP_REACT || em->state != KUTKU_REACT_FALL) {
            EmKutku_ChangeState(em, EM_GROUP_REACT, KUTKU_REACT_FALL, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_STATUS4:
        if ((s8) em->holdTimer != 0) {
            Em_SetStatus4Timer(em);
            EmKutku_ChangeState(em, EM_GROUP_REACT, KUTKU_REACT_STATUS4_DOWNED, EM_SYNC_OWNER);
        } else if ((em->stateGroup != EM_GROUP_REACT || em->state != KUTKU_REACT_STATUS4) && (em->stateGroup != EM_GROUP_REACT || em->state != KUTKU_REACT_FALL)) {
            Em_SetStatus4Timer(em);
            EmKutku_ChangeState(em, EM_GROUP_REACT, KUTKU_REACT_STATUS4, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_CAPTURED:
        if (Em_IsHeld(em) == 1) {
            Em_SetStatus1Timer(em);
            if ((s8) em->holdTimer != 0) {
                EmKutku_ChangeState(em, EM_GROUP_EVENT, KUTKU_EVENT_CAPTURED, EM_SYNC_OWNER);
                break;
            }
            if ((s8) em->holdTimer2 != 0) {
                EmKutku_ChangeState(em, EM_GROUP_EVENT, KUTKU_EVENT_CAPTURED_HOLD2, EM_SYNC_OWNER);
                break;
            }
        }
        if ((em->stateGroup != EM_GROUP_IDLE || em->state != KUTKU_IDLE_STATUS1_COLLAPSE) && (em->stateGroup != EM_GROUP_REACT || em->state != KUTKU_REACT_FALL)) {
            Em_SetStatus1Timer(em);
            EmKutku_ChangeState(em, EM_GROUP_IDLE, KUTKU_IDLE_STATUS1_COLLAPSE, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_STATUS2:
        if ((s8) em->holdTimer != 0) {
            Em_SetStatus2Timer(em);
            EmKutku_ChangeState(em, EM_GROUP_IDLE, KUTKU_IDLE_STATUS2_DOWNED, EM_SYNC_OWNER);
        } else if ((s8) em->holdTimer2 != 0) {
            Em_SetStatus2Timer(em);
            EmKutku_ChangeState(em, EM_GROUP_IDLE, KUTKU_IDLE_STATUS2_COLLAPSE, EM_SYNC_OWNER);
        } else if ((em->stateGroup != EM_GROUP_IDLE || em->state != KUTKU_IDLE_STATUS2_COLLAPSE) && (em->stateGroup != EM_GROUP_REACT || em->state != KUTKU_REACT_FALL)) {
            Em_SetStatus2Timer(em);
            EmKutku_ChangeState(em, EM_GROUP_IDLE, KUTKU_IDLE_STATUS2_COLLAPSE, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_HIT_LYING:
        switch (em->state) {
        case 0x12:
            EmKutku_ChangeState(em, EM_GROUP_IDLE, KUTKU_IDLE_REST_END, EM_SYNC_OWNER);
            em->scriptPending = 0;
            Em_CheckRageByHp(em, em->rageThreshold);
            break;
        case 0x14:
        case 0x15:
            EmKutku_ChangeState(em, EM_GROUP_IDLE, KUTKU_IDLE_STATUS2_END, EM_SYNC_OWNER);
            em->scriptPending = 0;
            break;
        case 0x1B:
            EmKutku_ChangeState(em, EM_GROUP_IDLE, KUTKU_IDLE_STATUS1_END, EM_SYNC_OWNER);
            em->scriptPending = 0;
            break;
        case 0x1D:
            EmKutku_ChangeState(em, EM_GROUP_REACT, KUTKU_REACT_STATUS2_DOWNED_END, EM_SYNC_OWNER);
            em->scriptPending = 0;
            break;
        case 0x1F:
            EmKutku_ChangeState(em, EM_GROUP_REACT, KUTKU_REACT_STATUS1_DOWNED_END, EM_SYNC_OWNER);
            em->scriptPending = 0;
            break;
        }
        break;
    case EM_DMG_PART8_DOWN:
        EmKutku_ChangeState(em, EM_GROUP_REACT, KUTKU_REACT_PART_BREAK, EM_SYNC_OWNER);
        break;
    case EM_DMG_ATTACK5_FLINCH:
        if (em->stateGroup == EM_GROUP_REACT && em->state == KUTKU_REACT_ATTACK5) {
            break;
        }
        if ((s8) em->holdTimer != 0) {
            break;
        }
        if (em->stance == EM_STANCE_AIR) {
            if (em->rage == 0) {
                Em_CheckRageByHp(em, em->rageThreshold);
            }
            EmKutku_ChangeState(em, EM_GROUP_REACT, KUTKU_REACT_FALL, EM_SYNC_OWNER);
            break;
        }
        if (em->rage == 0 && em->type != EM_KIND_GYPCEROS && em->type != EM_KIND_PURPLE_GYPCEROS) {
            Em_CheckRageByHp(em, em->rageThreshold);
            EmKutku_ChangeState(em, EM_GROUP_REACT, KUTKU_REACT_ATTACK5, EM_SYNC_OWNER);
            break;
        }
    case EM_DMG_PART_FLINCH:
        Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
        if (em->stance == EM_STANCE_AIR) {
            EmKutku_ChangeState(em, EM_GROUP_REACT, KUTKU_REACT_FALL, EM_SYNC_OWNER);
            break;
        }
        if (em->type == EM_KIND_GYPCEROS || em->type == EM_KIND_PURPLE_GYPCEROS) {
            if (em->hp <= em->hpMax * 30 / 100) {
                if (em->stateRand % 100 < 30) {
                    if (em->level >= 11 && em->level < 18) {
                        EmKutku_ChangeState(em, EM_GROUP_REACT, KUTKU_REACT_FEIGN_DEATH_CARVABLE, EM_SYNC_OWNER);
                    } else {
                        EmKutku_ChangeState(em, EM_GROUP_REACT, KUTKU_REACT_FEIGN_DEATH, EM_SYNC_OWNER);
                    }
                    break;
                }
            } else if (em->hp <= em->hpMax * 50 / 100) {
                if (em->stateRand % 100 < 20) {
                    if (em->level >= 11 && em->level < 18) {
                        EmKutku_ChangeState(em, EM_GROUP_REACT, KUTKU_REACT_FEIGN_DEATH_CARVABLE, EM_SYNC_OWNER);
                    } else {
                        EmKutku_ChangeState(em, EM_GROUP_REACT, KUTKU_REACT_FEIGN_DEATH, EM_SYNC_OWNER);
                    }
                    break;
                }
            }
        }
        switch (em->flinchPart) {
        case 0:
        case 7:
            EmKutku_ChangeState(em, EM_GROUP_REACT, KUTKU_REACT_FLINCH, EM_SYNC_OWNER);
            break;
        case 6:
            if ((em->type == EM_KIND_GYPCEROS || em->type == EM_KIND_PURPLE_GYPCEROS) && flyWork->partWhole != 0 && em->parts[6].state >= 2) {
                EmKutku_ChangeState(em, EM_GROUP_REACT, KUTKU_REACT_PART6_BREAK, EM_SYNC_OWNER);
                break;
            }
        case 5:
            EmKutku_ChangeState(em, EM_GROUP_REACT, KUTKU_REACT_FLINCH_C, EM_SYNC_OWNER);
            break;
        case 1:
        case 2:
            EmKutku_ChangeState(em, EM_GROUP_REACT, KUTKU_REACT_FLINCH_D, EM_SYNC_OWNER);
            break;
        case 3:
        case 4:
        default:
            if (em->parts[em->flinchPart].state >= 2) {
                if (em->flinchPart != 3) {
                    EmKutku_ChangeState(em, EM_GROUP_REACT, KUTKU_REACT_KNOCK_DOWN, EM_SYNC_OWNER);
                } else {
                    EmKutku_ChangeState(em, EM_GROUP_REACT, KUTKU_REACT_KNOCK_DOWN_PART3, EM_SYNC_OWNER);
                }
            } else {
                EmKutku_ChangeState(em, EM_GROUP_REACT, KUTKU_REACT_FLINCH_B, EM_SYNC_OWNER);
            }
            break;
        }
        break;
    case EM_DMG_HIT_MODE3:
        if (em->stance != EM_STANCE_AIR) {
            EmKutku_ChangeState(em, EM_GROUP_REACT, KUTKU_REACT_FLINCH, EM_SYNC_OWNER);
            em->scriptPending = 0;
        }
        break;
    }
    if ((em->type == EM_KIND_YIAN_KUT_KU || em->type == EM_KIND_BLUE_YIAN_KUT_KU) && em->parts[6].state >= 2) {
        Em_BreakPart(em, 1);
    }
    switch (gQuest.stage) {
    case QUEST_ID_KUTKU_SCENE_1:
    case QUEST_ID_KUTKU_SCENE_2:
        if (Area_DayId(em->area) == AREA_JUNGLE_9 && Save_TestEventFlag(EVENT_FLAG_KUTKU_SCENE) == 0) {
            if (Area_DayId(gScene.area) == AREA_JUNGLE_9 && gScene.eventRunning == 1 && em->stateGroup != EM_GROUP_EVENT) {
                EmKutku_ChangeState(em, EM_GROUP_EVENT, KUTKU_EVENT_INTRO, EM_SYNC_SEND);
            }
            break;
        }
    default:
        Em_UpdatePendingScript(em);
        break;
    }
    Em_ClearHitReactMode(em);
    EmKutku_RunState(em, (s32) flyWork);
    if (em->stateRequest != 0) {
        EmKutku_RunState(em, (s32) flyWork);
        em->stateRequest = 0;
    }
    if (em->stateGroup == EM_GROUP_FLY && (s8) flyWork->flyGuard == 1) {
        em->guardTimer = 2;
    }
}

/*
 * Saves prevStateGroup/prevState and calls the group dispatcher (group 7
 * goes to EmKutku_RunEventState).
 */
void EmKutku_RunState(Em *em, s32 work)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->stateGroup) {
    case EM_GROUP_IDLE:
        EmKutku_RunIdleState(em, (EmWork *) work);
        break;
    case EM_GROUP_MOVE:
        EmKutku_RunMoveState(em, (EmWork *) work);
        break;
    case EM_GROUP_FLY:
        EmKutku_RunFlyState(em, (EmWork *) work);
        break;
    case EM_GROUP_ATTACK:
        EmKutku_RunAttackState(em, (EmWork *) work);
        break;
    case EM_GROUP_REACT:
        EmKutku_RunReactState(em, (EmWork *) work);
        break;
    case EM_GROUP_DEFEAT:
        EmKutku_RunDefeatState(em, (EmWork *) work);
        break;
    case EM_GROUP_EVENT:
        EmKutku_RunEventState(em, work);
        break;
    case EM_GROUP_EVENT2:
        EmKutku_RunEventState(em, work);
        break;
    case EM_GROUP_VERSUS:
        EmKutku_RunVersusState(em, (EmWork *) work);
        
    default:
        break;
    }
}

/*
 * Requests camera shake kind i (CamShake_ReqAIf) when motion layer 0 passes
 * frame.
 */
void EmKutku_ShakeOnFrame(void *em, s32 frame, s32 shakeKind)
{
    if (EmMotion_PassedFrameScaled((Em *) em, (f32) frame, 0U) != 0) {
        CamShake_ReqAIf(em, shakeKind);
    }
}

/*
 * Empty default case of MotionEffects.
 */
static void EmKutku_MotionEffectsNone(Em *em)
{
}

/*
 * Per-frame effects keyed on the layer-0 motion id (1000+N): calls
 * MotionSounds, then creates attack hitboxes (KutkuAttack_Create), ranged
 * shots (EmRangedAttack_CreateAtBone, per kind), dust clouds, camera shakes
 * and enraged dust at set frames; Gypceros motions KUTKU_MOT_ALERT_IDLE and
 * 0x6B set crestGlow while partWhole; 0x6B makes the flash (FireBlast at
 * bone 35, KutkuAttack_CreateFlash).
 */
void EmKutku_MotionEffects(Em *em, EmKutkuFxWork *fxWork)
{
    f32 puffOfsA[3];
    f32 puffOfsB[3];
    f32 crestPos[3];

    if (em->motionId[0] != fxWork->motion) {
        fxWork->motion = em->motionId[0];
    }
    EmKutku_MotionSounds(em, (struct EmWork *)fxWork);
    switch (fxWork->motion) {
    case EM_MOTION_ID(0, KUTKU_MOT_WALK):
        EmKutku_ShakeOnFrame(em, 0x34, 1);
        EmKutku_ShakeOnFrame(em, 0x74, 1);
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            KutkuAttack_Create(em, 2);
        }
        if (EmMotion_PassedFrameScaled(em, 50.0f, 0) != 0) {
            KutkuAttack_Create(em, 3);
        }
        if (EmMotion_PassedFrameScaled(em, 120.0f, 0) != 0) {
            KutkuAttack_Create(em, 4);
            break;
        }
        break;
    case EM_MOTION_ID(0, 0x4):
        if (EmMotion_PassedFrameScaled(em, 38.0f, 0) != 0) {
            DustCloud_Create(em, 3, 0, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 74.0f, 0) != 0) {
            DustCloud_Create(em, 2, 0, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, 0x5):
        EmKutku_ShakeOnFrame(em, 0x3C, 1);
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            KutkuAttack_Create(em, 5);
        }
        if (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) {
            KutkuAttack_Create(em, 6);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 2.0f, 78.0f) != 0) && !(gScene.frame & 3)) {
            DustCloud_Create(em, 0x1A, 1, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_TURN_LEFT):
        EmKutku_ShakeOnFrame(em, 0x3C, 1);
        if (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) {
            KutkuAttack_Create(em, 7);
        }
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            KutkuAttack_Create(em, 8);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 2.0f, 78.0f) != 0) && !(gScene.frame & 3)) {
            DustCloud_Create(em, 0x1A, 0, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, 0x7):
        if (EmMotion_PassedFrameScaled(em, 18.0f, 0) != 0) {
            KutkuAttack_Create(em, 9);
        }
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            KutkuAttack_Create(em, 0xA);
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
    case EM_MOTION_ID(0, 0x8):
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            KutkuAttack_Create(em, 0xB);
        }
        if (EmMotion_PassedFrameScaled(em, 18.0f, 0) != 0) {
            KutkuAttack_Create(em, 0xC);
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
    case EM_MOTION_ID(0, KUTKU_MOT_LIMP):
        if (EmMotion_PassedFrameScaled(em, 42.0f, 0) != 0) {
            KutkuAttack_Create(em, 0x19);
        }
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            KutkuAttack_Create(em, 0x1A);
        }
        if (EmMotion_PassedFrameScaled(em, 74.0f, 0) != 0) {
            KutkuAttack_Create(em, 0x1A);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_DESCEND):
        if (EmMotion_PassedFrameScaled(em, 52.0f, 0) || EmMotion_PassedFrameScaled(em, 104.0f, 0) || EmMotion_PassedFrameScaled(em, 162.0f, 0)) {
            PartFx_SpawnForEmScaled(em, 2, 7, 5.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, 0xD):
        if ((EmMotion_PassedFrameScaled(em, 26.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 52.0f, 0) != 0)) {
            PartFx_SpawnForEmScaled(em, 1, 7, 5.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_FLY):
    case EM_MOTION_ID(0, 0x55):
    case EM_MOTION_ID(0, 0x56):
    case EM_MOTION_ID(0, 0x58):
    case EM_MOTION_ID(0, 0x59):
        if ((em->rage != 0) && ((gScene.frame % 10) == 0)) {
            DustCloud_Create(em, 0x19, (Rand_Next(1) & 1) + 2, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_HOVER):
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
    case EM_MOTION_ID(0, KUTKU_MOT_RUN):
        if (EmMotion_PassedFrameScaled(em, 18.0f, 0) != 0) {
            KutkuAttack_Create(em, 0x13);
        }
        if (EmMotion_PassedFrameScaled(em, 48.0f, 0) != 0) {
            if (em->type == EM_KIND_YIAN_KUT_KU || em->type == EM_KIND_BLUE_YIAN_KUT_KU) {
            EmRangedAttack_CreateAtBone(em, 0x22, 3, 0, 0x222, 0x2CCD);
            } else if (em->type == EM_KIND_PURPLE_GYPCEROS) {
            EmRangedAttack_CreateAtBone(em, 0x22, 2, 1, 0x222, 0x2CCD);
            } else {
            EmRangedAttack_CreateAtBone(em, 0x22, 2, 0, 0x222, 0x2CCD);
            }
        }
        if (EmMotion_PassedFrameScaled(em, 86.0f, 0) != 0) {
            if (em->type == EM_KIND_YIAN_KUT_KU || em->type == EM_KIND_BLUE_YIAN_KUT_KU) {
            EmRangedAttack_CreateAtBone(em, 0x22, 3, 0, 0x444, 0xD112);
            } else if (em->type == EM_KIND_PURPLE_GYPCEROS) {
            EmRangedAttack_CreateAtBone(em, 0x22, 2, 1, 0x444, 0xD112);
            } else {
            EmRangedAttack_CreateAtBone(em, 0x22, 2, 0, 0x444, 0xD112);
            }
        }
        if ((EmMotion_PassedFrameScaled(em, 12.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 72.0f, 0) != 0)) {
            DustCloud_Create(em, 2, 0, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 40.0f, 0) != 0) {
            DustCloud_Create(em, 3, 0, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_TAKE_OFF):
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
    case EM_MOTION_ID(0, KUTKU_MOT_TOUCH_DOWN):
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            KutkuAttack_Create(em, 0xD);
        }
        if (EmMotion_PassedFrameScaled(em, 16.0f, 0) != 0) {
            EmKutku_SpawnLandingDust(em);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 18.0f, 44.0f) != 0) && !(gScene.frame & 3)) {
            DustCloud_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_REST):
        puffOfsA[1] = 10.0f;
        puffOfsA[2] = 140.0f;
        puffOfsA[0] = 0.0f;
        Em_SpawnLyingPuffs(em, 0x22, puffOfsA, 1.6f);
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_LIE):
        puffOfsB[1] = 10.0f;
        puffOfsB[2] = 140.0f;
        puffOfsB[0] = 0.0f;
        Em_SpawnLyingPuffs(em, 0x22, puffOfsB, 1.6f);
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_STARE):
        if (EmMotion_PassedFrameScaled(em, 50.0f, 0) != 0) {
            KutkuAttack_Create(em, 1);
        }
        if (EmMotion_PassedFrameScaled(em, 46.0f, 0) != 0) {
            KutkuAttack_Create(em, 0x21);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_COLLAPSE):
        if (EmMotion_PassedFrameScaled(em, 148.0f, 0) != 0) {
            KutkuAttack_Create(em, 0xF);
        }
        if (EmMotion_PassedFrameScaled(em, 156.0f, 0) != 0) {
            DustCloud_Create(em, 0, 0, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_LEAP):
        if (EmMotion_PassedFrameScaled(em, 60.0f, 0) != 0) {
            KutkuAttack_Create(em, 0x1C);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_LEAP_LAND):
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            KutkuAttack_Create(em, 0x1D);
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
            break;
        }
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_TAIL_SPIN):
        EmKutku_ShakeOnFrame(em, 0x2C, 1);
        EmKutku_ShakeOnFrame(em, 0x49, 1);
        if (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) {
            KutkuAttack_Create(em, 0x11);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 6.0f, 38.0f) != 0) && !(gScene.frame & 3)) {
            DustCloud_Create(em, 0x1A, 0, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, 0x2B):
        EmKutku_ShakeOnFrame(em, 0x2C, 1);
        EmKutku_ShakeOnFrame(em, 0x49, 1);
        if (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) {
            KutkuAttack_Create(em, 0x11);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 6.0f, 38.0f) != 0) && !(gScene.frame & 3)) {
            DustCloud_Create(em, 0x1A, 1, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_SWOOP_CLAW):
        if (EmMotion_PassedFrameScaled(em, 22.0f, 0) != 0) {
            KutkuAttack_Create(em, 0x14);
            KutkuAttack_Create(em, 0x15);
        }
        if (EmMotion_PassedFrameScaled(em, 20.0f, 0) || EmMotion_PassedFrameScaled(em, 68.0f, 0) || EmMotion_PassedFrameScaled(em, 114.0f, 0)) {
            if (em->posY <= (1000.0f + em->floorY)) {
                DustCloud_Create(em, 9, 0, 1.0f);
                DustCloud_Create(em, 0xE, 0, 1.0f);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_SPIT):
        if (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) {
            KutkuAttack_Create(em, 0x22);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_ALERT_IDLE):
        if ((em->type == EM_KIND_GYPCEROS) || (em->type == EM_KIND_PURPLE_GYPCEROS)) {
            if (em->rage != 0) {
                if (EmMotion_InFrameRange(em, 0, 86.0f, 92.0f) || EmMotion_InFrameRange(em, 0, 146.0f, 152.0f) || EmMotion_InFrameRange(em, 0, 196.0f, 202.0f)) {
                    DustCloud_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
                }
            }
            if ((fxWork->partWhole != 0) && ((EmMotion_PassedFrameScaled(em, 88.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 146.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 198.0f, 0) != 0))) {
                fxWork->crestGlow = 1;
                break;
            }
        } else if (em->rage != 0) {
            if (EmMotion_InFrameRange(em, 0, 72.0f, 80.0f) || EmMotion_InFrameRange(em, 0, 132.0f, 140.0f) || EmMotion_InFrameRange(em, 0, 184.0f, 192.0f)) {
                DustCloud_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_BACK_HOP):
        if (EmMotion_PassedFrameScaled(em, 72.0f, 0) != 0) {
            DustCloud_Create(em, 4, 0, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 80.0f, 0) != 0) {
            DustCloud_Create(em, 1, 0, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 84.0f, 0) != 0) {
            DustCloud_Create(em, 4, 1, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_BACK_HOP_LAND):
        if (EmMotion_PassedFrameScaled(em, 12.0f, 0) != 0) {
            DustCloud_Create(em, 4, 0, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) {
            DustCloud_Create(em, 5, 0, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 28.0f, 0) != 0) {
            KutkuAttack_Create(em, 0x1B);
        }
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            KutkuAttack_Create(em, 0x1F);
        }
        if (EmMotion_PassedFrameScaled(em, 36.0f, 0) != 0) {
            KutkuAttack_Create(em, 0x20);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_FLINCH):
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
    case EM_MOTION_ID(0, KUTKU_MOT_FLINCH_E):
        if (EmMotion_PassedFrameScaled(em, 24.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0x1A, 7, 1.0f);
            break;
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
            break;
        }
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_DIE):
        if (EmMotion_PassedFrameScaled(em, 270.0f, 0) != 0) {
            DustCloud_Create(em, 0x12, 0x80, 0.9f);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_KNOCK_DOWN):
        EmKutku_ShakeOnFrame(em, 0x1A, 2);
        if (EmMotion_PassedFrameScaled(em, 26.0f, 0) != 0) {
            DustCloud_Create(em, 0x12, 0x80, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_KNOCK_DOWN_B):
        EmKutku_ShakeOnFrame(em, 0x1A, 2);
        if (EmMotion_PassedFrameScaled(em, 26.0f, 0) != 0) {
            DustCloud_Create(em, 0x12, 0x81, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_FALL_LAND):
        if (EmMotion_PassedFrameScaled(em, 8.0f, 0) != 0) {
            DustCloud_Create(em, 0xB, 0, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_FORAGE):
        if (EmMotion_PassedFrameScaled(em, 48.0f, 0) || EmMotion_PassedFrameScaled(em, 106.0f, 0) || EmMotion_PassedFrameScaled(em, 174.0f, 0)) {
            KutkuAttack_Create(em, 0x10);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_HOP_IN_PLACE):
        if (EmMotion_PassedFrameScaled(em, 42.0f, 0) || EmMotion_PassedFrameScaled(em, 60.0f, 0) || EmMotion_PassedFrameScaled(em, 80.0f, 0)) {
            DustCloud_Create(em, 1, 0, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_LEVEL_OUT):
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
    case EM_MOTION_ID(0, KUTKU_MOT_CHARGE_TRIP):
        if (EmMotion_PassedFrameScaled(em, 34.0f, 0) != 0) {
            DustCloud_Create(em, 3, 0, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 70.0f, 0) != 0) {
            DustCloud_Create(em, 2, 0, 1.0f);
        }
        if ((EmMotion_ReachedFrame(em, 0, 96.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 120.0f) == 0) && ((gScene.frame % 6) == 0)) {
            DustCloud_Create(em, 0x12, 0, 1.0f);
        }
        if ((em->rage != 0) && ((gScene.frame % 19) == 0)) {
            DustCloud_Create(em, 0x19, (Rand_Next(1) & 1) + 2, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_TRAPPED):
        if (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0) {
            DustCloud_Create(em, 0xB, 0x80, 1.0f);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 4.0f, 60.0f) != 0) && !(gScene.frame & 3)) {
            DustCloud_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_STRUGGLE):
        if (em->rage != 0) {
            if (EmMotion_InFrameRange(em, 0, 12.0f, 24.0f) || EmMotion_InFrameRange(em, 0, 46.0f, 90.0f)) {
                if (!(gScene.frame & 3)) {
                    DustCloud_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
                    break;
                }
            }
        }
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_THREAT):
        if (EmMotion_PassedFrameScaled(em, 118.0f, 0) || EmMotion_PassedFrameScaled(em, 164.0f, 0) || EmMotion_PassedFrameScaled(em, 204.0f, 0)) {
            KutkuAttack_Create(em, 0x23);
        }
        if (em->rage != 0) {
            if (EmMotion_InFrameRange(em, 0, 54.0f, 250.0f) || EmMotion_InFrameRange(em, 0, 282.0f, 296.0f)) {
                if (!(gScene.frame & 3)) {
                    DustCloud_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
                    break;
                }
            }
        }
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_TURN_TO_GOAL):
        if (EmMotion_PassedFrameScaled(em, 52.0f, 0) != 0) {
            DustCloud_Create(em, 1, 0x80, 1.0f);
        }
        if (em->rage != 0) {
            if (EmMotion_InFrameRange(em, 0, 10.0f, 68.0f) || EmMotion_InFrameRange(em, 0, 84.0f, 96.0f)) {
                if (!(gScene.frame & 3)) {
                    DustCloud_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
                    break;
                }
            }
        }
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_PECK_COMBO):
        if ((EmMotion_PassedFrameScaled(em, 32.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 54.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 74.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 92.0f, 0) != 0)) {
            KutkuAttack_Create(em, 0x18);
        }
        if (em->rage != 0) {
            if ((EmMotion_InFrameRange(em, 0, 10.0f, 20.0f) != 0) && !(gScene.frame & 3)) {
                DustCloud_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
            }
            if ((EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 54.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 72.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 92.0f, 0) != 0)) {
                DustCloud_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_WALK_B):
        if (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) {
            KutkuAttack_Create(em, 0x17);
        }
        if ((EmMotion_PassedFrameScaled(em, 18.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 60.0f, 0) != 0)) {
            PartFx_SpawnForEmScaled(em, 0x1A, 3, 4.0f);
        }
        if ((EmMotion_PassedFrameScaled(em, 38.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 80.0f, 0) != 0)) {
            PartFx_SpawnForEmScaled(em, 0x14, 3, 4.0f);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 54.0f, 112.0f) != 0) && !(gScene.frame & 3)) {
            DustCloud_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_SPIT_VOLLEY):
        if (EmMotion_PassedFrameScaled(em, 78.0f, 0) != 0) {
            if (em->type == EM_KIND_YIAN_KUT_KU || em->type == EM_KIND_BLUE_YIAN_KUT_KU) {
            EmRangedAttack_CreateAtBone(em, 0x22, 3, 0, 0xF556, 0xEF4B);
            } else if (em->type == EM_KIND_PURPLE_GYPCEROS) {
            EmRangedAttack_CreateAtBone(em, 0x22, 2, 1, 0xF556, 0xEF4B);
            } else {
            EmRangedAttack_CreateAtBone(em, 0x22, 2, 0, 0xF556, 0xEF4B);
            }
        }
        if (EmMotion_PassedFrameScaled(em, 150.0f, 0) != 0) {
            if (em->type == EM_KIND_YIAN_KUT_KU || em->type == EM_KIND_BLUE_YIAN_KUT_KU) {
            EmRangedAttack_CreateAtBone(em, 0x22, 3, 0, 0xF556, 0x199A);
            } else if (em->type == EM_KIND_PURPLE_GYPCEROS) {
            EmRangedAttack_CreateAtBone(em, 0x22, 2, 1, 0xF556, 0x199A);
            } else {
            EmRangedAttack_CreateAtBone(em, 0x22, 2, 0, 0xF556, 0x199A);
            }
        }
        if (EmMotion_PassedFrameScaled(em, 240.0f, 0) != 0) {
            if (em->type == EM_KIND_YIAN_KUT_KU || em->type == EM_KIND_BLUE_YIAN_KUT_KU) {
            EmRangedAttack_CreateAtBone(em, 0x22, 3, 0, 0xF1C8, 0xDC73);
            } else if (em->type == EM_KIND_PURPLE_GYPCEROS) {
            EmRangedAttack_CreateAtBone(em, 0x22, 2, 1, 0xF1C8, 0xDC73);
            } else {
            EmRangedAttack_CreateAtBone(em, 0x22, 2, 0, 0xF1C8, 0xDC73);
            }
        }
        if (EmMotion_PassedFrameScaled(em, 296.0f, 0) != 0) {
            if (em->type == EM_KIND_YIAN_KUT_KU || em->type == EM_KIND_BLUE_YIAN_KUT_KU) {
            EmRangedAttack_CreateAtBone(em, 0x22, 3, 0, 0xF940, 0x1C72);
            } else if (em->type == EM_KIND_PURPLE_GYPCEROS) {
            EmRangedAttack_CreateAtBone(em, 0x22, 2, 1, 0xF940, 0x1C72);
            } else {
            EmRangedAttack_CreateAtBone(em, 0x22, 2, 0, 0xF940, 0x1C72);
            }
        }
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_PECK_COMBO_B):
        if (EmMotion_PassedFrameScaled(em, 20.0f, 0) != 0) {
            KutkuAttack_Create(em, 0x12);
        }
        if ((EmMotion_PassedFrameScaled(em, 50.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 76.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 96.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 116.0f, 0) != 0)) {
            KutkuAttack_Create(em, 0x18);
        }
        if (EmMotion_PassedFrameScaled(em, 20.0f, 0) != 0) {
            DustCloud_Create(em, 3, 0, 1.0f);
        }
        if (em->rage != 0) {
            if ((EmMotion_InFrameRange(em, 0, 24.0f, 42.0f) != 0) && !(gScene.frame & 3)) {
                DustCloud_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
            }
            if ((EmMotion_PassedFrameScaled(em, 54.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 80.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 100.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 122.0f, 0) != 0)) {
                DustCloud_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_FLASH):
        if (fxWork->partWhole != 0) {
            fxWork->crestGlow = 1;
            if (EmMotion_PassedFrameScaled(em, 40.0f, 0) != 0) {
                Mtx_GetTrans(crestPos, em->model->bones[35].world);
                FireBlast_CreateOwned(crestPos, 5, 1.0f, em);
                KutkuAttack_CreateFlash(em, 0x16);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_FEIGN_DEATH_RISE):
        if (EmMotion_PassedFrameScaled(em, 20.0f, 0) != 0) {
            if (em->state == KUTKU_ATK_FEIGN_DEATH_RISE_B) {
                KutkuAttack_Create(em, 0x25);
            } else {
                KutkuAttack_Create(em, 0xE);
            }
        }
        if (EmMotion_PassedFrameScaled(em, 20.0f, 0) || EmMotion_PassedFrameScaled(em, 92.0f, 0) || EmMotion_PassedFrameScaled(em, 152.0f, 0)) {
            PartFx_SpawnForEmScaled(em, 2, 7, 3.0f);
            break;
        }
        break;
    default:
        EmKutku_MotionEffectsNone(em);
        break;
    case EM_MOTION_ID(0, KUTKU_MOT_IDLE):
    case EM_MOTION_ID(0, KUTKU_MOT_GLIDE):
    case EM_MOTION_ID(0, KUTKU_MOT_IDLE_ACTION_C):
    case EM_MOTION_ID(0, KUTKU_MOT_REST_LIE_DOWN):
    case EM_MOTION_ID(0, KUTKU_MOT_REST_GET_UP):
    case EM_MOTION_ID(0, 0x23):
    case EM_MOTION_ID(0, KUTKU_MOT_GET_UP):
    case EM_MOTION_ID(0, KUTKU_MOT_LIE_DOWN):
    case EM_MOTION_ID(0, KUTKU_MOT_SWOOP_DIVE):
    case EM_MOTION_ID(0, 0x30):
    case EM_MOTION_ID(0, KUTKU_MOT_TURN_IN_PLACE):
    case EM_MOTION_ID(0, KUTKU_MOT_FEED):
    case EM_MOTION_ID(0, KUTKU_MOT_SHAKE_OFF):
    case EM_MOTION_ID(0, KUTKU_MOT_FLINCH_C):
    case EM_MOTION_ID(0, KUTKU_MOT_FLINCH_D):
    case EM_MOTION_ID(0, KUTKU_MOT_FLINCH_B):
    case EM_MOTION_ID(0, 0x43):
    case EM_MOTION_ID(0, KUTKU_MOT_DOWN):
    case EM_MOTION_ID(0, KUTKU_MOT_DOWN_GET_UP):
    case EM_MOTION_ID(0, KUTKU_MOT_DOWN_GET_UP_B):
    case EM_MOTION_ID(0, KUTKU_MOT_DOWN_B):
    case EM_MOTION_ID(0, KUTKU_MOT_STATUS4):
    case EM_MOTION_ID(0, KUTKU_MOT_FALL):
    case EM_MOTION_ID(0, KUTKU_MOT_IDLE_PAIR_A):
    case EM_MOTION_ID(0, KUTKU_MOT_IDLE_PAIR_B):
    case EM_MOTION_ID(0, KUTKU_MOT_DIE_FALLING):
    case EM_MOTION_ID(0, KUTKU_MOT_SWOOP):
    case EM_MOTION_ID(0, KUTKU_MOT_DOWNED):
    case EM_MOTION_ID(0, KUTKU_MOT_DOWNED_GET_UP):
    case EM_MOTION_ID(0, KUTKU_MOT_ATTACK5_FLINCH):
        break;
    }
}

/*
 * Callback slot 3: skips the first frame (fxStep 0 to 1), then runs
 * MotionEffects each frame.
 */
void EmKutku_UpdateMotionEffects(Em *em)
{
    EmFlyerWork *w = (EmFlyerWork *)&em->work;

    switch (w->fxStep) {
    case 0:
        w->fxStep++;
        break;
    case 1:
        EmKutku_MotionEffects(em, (EmKutkuFxWork *) w);
        break;
    }
}

/*
 * DustCloud_Create kind 0xB at the monster (after a landing curve, lines
 * 2940 and 3337).
 */
void EmKutku_SpawnLandingDust(Em *em)
{
    DustCloud_Create((Em *) em, 0xB, 0, 1.0f);
}

/*
 * DustCloud_Create kind 0xA every fifth scene frame (lines 3597 and 3606).
 */
void EmKutku_SpawnDustCloudAEvery5(Em *em)
{
    if (((s32) gScene.frame % 5) == 0) {
        DustCloud_Create((Em *) em, 0xA, 0, 1.0f);
    }
}

/*
 * Returns 1 when the swoop-claw motion (KUTKU_MOT_SWOOP_CLAW) already plays,
 * or when the target is within 4.5 + 30*velZ (then starts KUTKU_MOT_SWOOP_CLAW with a
 * 10-frame blend); 0 without a target or when farther.
 */
s32 EmKutku_SwoopClawIfNear(Em *em)
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
    if (em->motionId[0] == EM_MOTION_ID(0, KUTKU_MOT_SWOOP_CLAW)) {
        return 1;
    }
    if (dist <= (4.5f + (30.0f * em->velZ))) {
        Em_SetMotion(em, KUTKU_MOT_SWOOP_CLAW, 0xA, 0);
        return 1;
    }
    return 0;
}

/*
 * Ends an attack state (EndState); used by group 3 states only.
 */
void EmKutku_EndAttack(Em *em, EmWork *work)
{
    EmKutku_EndState(em, 0, 0);
}

/*
 * Ends a swoop through EmKutku_EndStateInAir(em, 1).
 */
void EmKutku_EndSwoop(Em *em, s32 unused)
{
    EmKutku_EndStateInAir(em, 0x1);
}

/*
 * A draw helper for the monster draw code: sets each mesh of set kind to em->alpha and
 * hides or shows parts: set 0 mesh 3 and set 2 mesh 5 by partWhole (whole vs
 * broken), set 3 mesh 0 while partWhole and crestGlow, set 3 mesh 5 opaque
 * only while enraged outside group 5; writes each mesh to GS register
 * 0x3A+i.
 */
void EmKutku_SetMeshSetAlpha(Em *em, s32 setNo, EmKutkuMeshSet *sets)
{
    ModelMat *meshList;
    s32 i;
    EmFlyerWork *w;
    ModelMat *meshEntry;

    meshList = em->model->mats;
    w = (EmFlyerWork *) &em->work;
    for (i = 0; i < sets[setNo].count; i++) {
        meshEntry = &meshList[sets[setNo].mesh[i]];
        meshEntry->alpha = em->alpha;
        switch (setNo) {
        case 0:
            switch (i) {
            case 1:
                meshEntry->alpha = 0.0f;
                break;
            case 3:
                if (w->partWhole == 0) {
                    meshEntry->alpha = 0.0f;
                } else {
                    meshEntry->alpha = em->alpha;
                }
                break;
            }
            break;
        case 2:
            switch (i) {
            case 5:
                if (w->partWhole != 0) {
                    meshEntry->alpha = 0.0f;
                } else {
                    meshEntry->alpha = em->alpha;
                }
                break;
            case 6:
                meshEntry->alpha = 0.0f;
                break;
            }
            break;
        case 3:
            switch (i) {
            case 0:
                if (w->partWhole == 0 || w->crestGlow == 0) {
                    meshEntry->alpha = 0.0f;
                } else {
                    meshEntry->alpha = em->alpha;
                }
                break;
            case 5:
                if (em->rage != 0 && em->stateGroup != EM_GROUP_DEFEAT) {
                    meshEntry->alpha = 1.0f;
                } else {
                    meshEntry->alpha = 0.0f;
                }
                break;
            }
            break;
        }
        Gs_SetState((i + 0x3A) & 0xFF, (s32) meshEntry);
    }
}

/*
 * Returns 1 for a Gypceros in KUTKU_ATK_FEIGN_DEATH (EmKutku_FeignDeath) or
 * 9 (EmKutku_FeignDeathLie) or KUTKU_REACT_FEIGN_DEATH_CARVABLE
 * (FeignDeathCarvable), else 0. Update uses it to keep crestGlow off.
 */
s32 EmKutku_IsFeigningDeath(Em *em)
{
    u8 stateGroup;
    u8 type;

    type = em->type;
    if ((type == EM_KIND_GYPCEROS) || (type == EM_KIND_PURPLE_GYPCEROS)) {
        stateGroup = em->stateGroup;
        if ((stateGroup != EM_GROUP_ATTACK) || (em->state != KUTKU_ATK_FEIGN_DEATH)) {
            if (stateGroup == EM_GROUP_ATTACK) {
                if (em->state != KUTKU_ATK_FEIGN_DEATH_LIE) {
                    goto block_7;
                }
                goto block_10;
            }
block_7:
            if ((stateGroup == EM_GROUP_REACT) && (em->state == KUTKU_REACT_FEIGN_DEATH_CARVABLE)) {
                goto block_10;
            }
            goto block_11;
        }
block_10:
        return 1;
    }
block_11:
    return 0;
}

/*
 * Empty callback (slot 2 of EmKutku_Callbacks).
 */
void EmKutku_TableNop(void)
{
}

/*
 * Idle-group entry: state 1 becomes 0x11 in combat; per state starts rage
 * (0x23) or pending rage (KUTKU_MOT_GET_UP), ends status 4 (KUTKU_MOT_LIE)
 * or 5 (KUTKU_MOT_STARE), clears holdTimer2 (8), starts a goal action
 * (KUTKU_MOT_LEAP), resets notice (KUTKU_MOT_TAKE_OFF), begins/ends statuses
 * 1 and 2 (KUTKU_MOT_UPPER_D, 0x15, 0x18, 0x1B, 0x1C, 0x1D, 0x1F), refills need 3 (0x17);
 * EM_STANCE_GROUND.
 */
void EmKutku_SetIdleState(Em *em, u16 state, u8 mode)
{
    em->motionSpeed = 1.0f;
    switch (state) {
    case KUTKU_IDLE:
        if (em->combat == 1) {
            state = KUTKU_IDLE_ALERT;
        }
        break;
    case KUTKU_IDLE_DISPLAY_RAGE:
        Em_StartRage(em);
        break;
    case KUTKU_IDLE_DISPLAY_PENDING_RAGE:
        Em_StartPendingRage(em);
        break;
    case KUTKU_IDLE_HOLD2_SHAKE_OFF:
        em->holdTimer2 = 0;
        break;
    case KUTKU_IDLE_STATUS4_SHAKE_OFF:
        Em_EndStatus4(em);
        break;
    case KUTKU_IDLE_STUN_SHAKE_OFF:
        Em_EndStatus5(em);
        break;
    case KUTKU_IDLE_EAT:
        Em_StartGoalAction(em, 0x2710);
        break;
    case KUTKU_IDLE_REST:
        Em_ResetNotice(em);
        break;
    case KUTKU_IDLE_STATUS2_COLLAPSE:
        Em_BeginStatus2(em);
        break;
    case KUTKU_IDLE_STATUS2_LIE_DOWN:
        Em_BeginStatus2(em);
        break;
    case KUTKU_IDLE_REST_END:
        Em_RefillNeed3(em);
        break;
    case KUTKU_IDLE_STATUS2_END:
        Em_EndStatus2(em);
        break;
    case KUTKU_IDLE_STATUS1_COLLAPSE:
        Em_BeginStatus1(em);
        break;
    case KUTKU_IDLE_STATUS1_END:
        Em_EndStatus1(em);
        break;
    case KUTKU_IDLE_STATUS2_DOWNED:
        Em_BeginStatus2(em);
        break;
    case KUTKU_IDLE_STATUS1_DOWNED:
        Em_BeginStatus1(em);
        break;
    }
    em->stance = EM_STANCE_GROUND;
    Em_SetStateSync(em, EM_GROUP_IDLE, state, mode);
}

/*
 * Move-group entry: below 20 percent hp state 0 becomes 6 (limp); sets speed
 * and the wander or goal timer per state; state 1 only sets scriptPending
 * (no change); state 3 becomes 5 in combat; state 8 runs at 0.8 speed.
 */
void EmKutku_SetMoveState(Em *em, u16 state, u8 mode)
{
    em->stance = EM_STANCE_GROUND;
    em->motionSpeed = 1.0f;
    switch (state) {
    case KUTKU_MOVE_WALK:
        if (em->type == EM_KIND_GYPCEROS || em->type == EM_KIND_PURPLE_GYPCEROS) {
            if (em->hp < (s16) (0.2f * em->hpMax)) {
                state = KUTKU_MOVE_LIMP;
                goto state6;
            }
        } else {
            if (em->hp < (s16) (0.2f * em->hpMax)) {
                state = KUTKU_MOVE_LIMP;
                goto state6;
            }
        }
        Em_SetSpeedScaled(em, 0, 1000.0f, 500.0f);
        if (em->remote == 0 && em->goalKind == 0) {
            em->hasGoal = 0;
            em->timer = (em->stateRand % 120 + 180) / 2;
        }
        break;
    case KUTKU_MOVE_END:
        em->scriptPending = 1;
        return;
    case KUTKU_MOVE_TURN:
        if (em->combat == 1) {
            state = KUTKU_MOVE_TURN_IN_COMBAT;
        }
        break;
    case KUTKU_MOVE_RUN_SLOW:
        em->motionSpeed = 0.8f;
    case KUTKU_MOVE_WALK_B:
    case KUTKU_MOVE_RUN:
    case KUTKU_MOVE_WALK_B_AND_TURN:
        if (em->curGoalKind == 0) {
            em->hasGoal = 0;
            em->timer = (em->stateRand % 120 + 180) / 2;
        } else {
            Em_SetSpeedScaled(em, 2, 1000.0f, 500.0f);
            em->timer = (s32) em->goalDist / 7;
        }
        break;
    case KUTKU_MOVE_LIMP:
    state6:
        if (em->curGoalKind == 0) {
            em->hasGoal = 0;
            em->timer = (em->stateRand % 120 + 180) / 2;
        } else {
            Em_SetSpeedScaled(em, 6, em->goalDist, 0.0f);
            em->timer = (s32) em->goalDist / 7;
        }
        if (em->timer > 300) {
            em->timer = 300;
        }
        break;
    case KUTKU_MOVE_TURN_IN_COMBAT:
    case KUTKU_MOVE_TURN_TO_GOAL:
    case KUTKU_MOVE_NOP:
        break;
    }
    Em_SetStateSync(em, EM_GROUP_MOVE, state, mode);
}

/*
 * Fly-group entry: 1 becomes 0x12 when velZ >= 50; picks flight goals (6 and
 * 0x11: four legs, 0x13: one leg), escape/area-exit goals (9, 0xB) and
 * timers from the goal distance; 8 and 0xD fall back to KUTKU_FLY_TAKE_OFF
 * unless goalKind is 3.
 */
void EmKutku_SetFlyState(Em *em, u16 state, u8 mode)
{
    EmFlyerWork *flyWork = (EmFlyerWork *)&em->work;

    em->motionSpeed = 1.0f;
    switch (state) {
    case KUTKU_FLY_LAND:
        if (em->remote == 0 && !(em->velZ < 50.0f)) {
            state = KUTKU_FLY_LEVEL_OUT_AND_LAND;
        }
        break;
    case KUTKU_FLY_ROUTE:
    case KUTKU_FLY_ROUTE_LOOP:
        flyWork->legsLeft = 4;
        if (em->remote == 0) {
            em->goalNo = em->stateRand & 3;
            em->goalKind = 2;
            em->goalArg = 2;
            Em_ComputeGoal(em, em->goal);
            em->timer = 1.5f * Vec_DistanceXZ(&em->posX, em->goal) / 50.0f;
        }
        break;
    case KUTKU_FLY_HOVER_TO_GOAL:
    case KUTKU_FLY_HOVER_STRAIGHT_TO_GOAL:
        Player_ClearVelocity(em);
        em->velZ = 20.0f;
        em->timer = (s32) (Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) / em->velZ) * 3;
        break;
    case KUTKU_FLY_THROUGH_AREA:
        Em_TypeSetup(em);
    case KUTKU_FLY_OUT_OF_AREA:
        if (em->remote == 0 && em->goalKind != 3) {
            EmKutku_ChangeState(em, EM_GROUP_FLY, KUTKU_FLY_TAKE_OFF, EM_SYNC_SEND);
            return;
        }
        break;
    case KUTKU_FLY_TO_GOAL_AND_LAND:
        if (em->remote == 0) {
            em->goalKind = 2;
            if (em->areaSeqActive != 0) {
                em->goalArg = 3;
                if (Map_DayId(gScene.map) == MAP_JUNGLE) {
                    if (em->type == EM_KIND_YIAN_KUT_KU || em->type == EM_KIND_BLUE_YIAN_KUT_KU) {
                        em->goalNo = 1;
                    } else {
                        em->goalNo = 0;
                    }
                } else {
                    em->goalNo = 0;
                }
            } else {
                em->goalArg = 0;
            }
            Em_ComputeGoal(em, em->goal);
        }
        em->timer = 500;
        Em_TypeSetup(em);
        break;
    case KUTKU_FLY_GLIDE_TO_GOAL_AND_LAND:
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
    case KUTKU_FLY_GLIDE_TO_GOAL:
        flyWork->legsLeft = 1;
        if (em->remote == 0) {
            em->timer = 1.5f * Vec_DistanceXZ(&em->posX, em->goal) / 50.0f;
        }
        break;
    case KUTKU_FLY_HOVER_RISE:
        if (em->velZ <= 0.0f) {
            em->velZ = 20.0f;
        }
        if (em->remote == 0) {
            em->timer = Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) / em->velZ;
        }
        break;
    case KUTKU_FLY_TAKE_OFF:
    case KUTKU_FLY_HOVER:
    case KUTKU_FLY_HOP_TO_GOAL:
    case KUTKU_FLY_HOP_FORWARD:
    case KUTKU_FLY_HOP_BACK:
    case KUTKU_FLY_END_TO_GLIDE:
    case KUTKU_FLY_BACK_HOP:
    case KUTKU_FLY_HOVER_ONCE:
    case KUTKU_FLY_GLIDE_ONCE:
    case KUTKU_FLY_HOVER_TURN:
    case KUTKU_FLY_LEVEL_OUT_AND_LAND:
    case KUTKU_FLY_HOP_IN_PLACE:
    case KUTKU_FLY_LEVEL_OUT:
    case KUTKU_FLY_TRAP_ESCAPE:
        break;
    }
    Em_SetStateSync(em, EM_GROUP_FLY, state, mode);
}

/*
 * Attack-group entry: aims at the target for most attacks; 4 and 9 (Gypceros
 * feign death) get 1800 frames; 2 sets runDist (2500 to 5000); 8 and 0x15
 * take an escape point and a flight timer; 0x12 a goal timer.
 */
void EmKutku_SetAttackState(Em *em, u16 state, u8 mode)
{
    EmFlyerWork *flyWork = (EmFlyerWork *)&em->work;
    f32 targetPos[3];
    f32 dist;

    em->motionSpeed = 1.0f;
    switch (state) {
    case KUTKU_ATK_TURN_AND_STARE:
    case 1:
    case KUTKU_ATK_PECK_COMBO:
    case KUTKU_ATK_PECK_STEAL:
    case KUTKU_ATK_SPIT_B:
    case KUTKU_ATK_SPIT_C:
    case KUTKU_ATK_SPIT_D:
    case KUTKU_ATK_SPIT:
    case KUTKU_ATK_SPIT_VOLLEY:
    case KUTKU_ATK_PECK_COMBO_B:
        Em_SetTargetAngle(em);
        break;
    case KUTKU_ATK_FEIGN_DEATH:
    case KUTKU_ATK_FEIGN_DEATH_LIE:
        em->timer = 1800;
        break;
    case KUTKU_ATK_LEAP:
        Em_SetTargetAngle(em);
        if (em->targetNo == -1) {
            flyWork->runDist = 2500.0f;
            break;
        }
        em->target = &gPlayers[em->targetNo];
        Em_GetTargetPos(em, em->targetNo, targetPos);
        Vec_Set(em->goal, targetPos[0], targetPos[1], targetPos[2]);
        dist = Vec_DistanceXZ(&em->posX, em->goal);
        flyWork->runDist = dist;
        if (dist > 5000.0f) {
            flyWork->runDist = 5000.0f;
        } else if (dist < 2500.0f) {
            flyWork->runDist = 2500.0f;
        }
        break;
    case KUTKU_ATK_SWOOP_THEN_LEAVE:
    case KUTKU_ATK_SWOOP_THEN_LAND:
        if (em->remote == 0) {
            EmKutku_SetFarthestEscapePoint(em);
            em->timer = 1.5f * Vec_DistanceXZ(&em->posX, em->goal) / 50.0f;
        }
        break;
    case KUTKU_ATK_CHARGE_TRIP:
        if (em->targetNo == -1) {
            em->timer = 150;
            em->hasGoal = 0;
        } else {
            em->hasGoal = 1;
            dist = Vec_DistanceXZ(&em->posX, em->goal);
            em->goalDist = dist;
            em->timer = (s32) (dist / 30.0f) + 30;
        }
        break;
    case KUTKU_ATK_STARE:
    case 5:
    case KUTKU_ATK_TAIL_SPIN:
    case KUTKU_ATK_FEIGN_DEATH_RISE:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 19:
    case 20:
    case 22:
    case KUTKU_ATK_FLASH:
    case KUTKU_ATK_SPIT_AFTER_FLINCH:
    case KUTKU_ATK_SPIT_VOLLEY_AFTER_FLINCH:
    case KUTKU_ATK_FEIGN_DEATH_RISE_B:
        break;
    }
    Em_SetStateSync(em, EM_GROUP_ATTACK, state, mode);
}

/*
 * React-group entry: begins status 4 (KUTKU_MOT_DESCEND, 0xE) or 5 (0x15, 0x16), sets
 * holdTimer 50 and ends all statuses on the pitfall (KUTKU_MOT_GLIDE), ends statuses
 * 4/2/1 (0x11/0x12/0x13), 5 (0x17) or all (KUTKU_MOT_UPPER_A), and on 0x14
 * clears partWhole and breaks part 1.
 */
void EmKutku_SetReactState(Em *em, u16 state, u8 mode)
{
    EmFlyerWork *flyWork = (EmFlyerWork *)&em->work;

    em->motionSpeed = 1.0f;
    switch (state) {
    case KUTKU_REACT_STATUS4:
        Em_BeginStatus4(em);
        break;
    case KUTKU_REACT_TRAPPED:
        em->holdTimer = 0x32;
        Em_EndAllStatus(em);
        break;
    case KUTKU_REACT_STATUS4_DOWNED:
        Em_BeginStatus4(em);
        break;
    case KUTKU_REACT_STATUS4_DOWNED_END:
        Em_EndStatus4(em);
        break;
    case KUTKU_REACT_STATUS2_DOWNED_END:
        Em_EndStatus2(em);
        break;
    case KUTKU_REACT_STATUS1_DOWNED_END:
        Em_EndStatus1(em);
        break;
    case KUTKU_REACT_PART6_BREAK:
        flyWork->partWhole = 0;
        Em_BreakPart(em, 1);
        break;
    case KUTKU_REACT_STUN:
        Em_BeginStatus5(em);
        break;
    case KUTKU_REACT_STUN_TRAPPED:
        Em_BeginStatus5(em);
        break;
    case KUTKU_REACT_STUN_TRAPPED_STRUGGLE:
        Em_EndStatus5(em);
        break;
    case KUTKU_REACT_HOLD2:
        Em_EndAllStatus(em);
        break;
    }
    Em_SetStateSync(em, EM_GROUP_REACT, state, mode);
}

/*
 * The kind state change: remote copies ignore EM_SYNC_OWNER and EM_SYNC_OWNER_FLAGGED; targets a player in
 * range; clears crestGlow and the mine point; reaction set 0; while feigning
 * death (KUTKU_ATK_FEIGN_DEATH_LIE or KUTKU_ATK_FEIGN_DEATH) any change but react/defeat becomes
 * KUTKU_ATK_FEIGN_DEATH_RISE (the get-up); then the group's EmKutku_Set...State function,
 * QuestEm_Appear on DEFEAT_REAPPEAR, Versus_MarkDefeat on VS_WAIT_BOTH.
 * Group 7 is ignored.
 */
void EmKutku_ChangeState(Em *em, u16 group, u16 state, u8 mode)
{
    EmFlyerWork *flyWork = (EmFlyerWork *)&em->work;

    if (em->remote == 0) {
        Em_TargetPlayerInRange(em);
    } else if (mode == EM_SYNC_OWNER || mode == EM_SYNC_OWNER_FLAGGED) {
        return;
    }
    flyWork->crestGlow = 0;
    if (flyWork->minePoint != -1) {
        MinePoint_Remove(flyWork->minePoint);
        flyWork->minePoint = -1;
    }
    Em_LoadReactionSet(em, 0);
    if (em->remote == 0 && group != 5 && group != 4 &&
        ((s16) Player_IsState(em, EM_GROUP_ATTACK, KUTKU_ATK_FEIGN_DEATH_LIE) != 0 || (s16) Player_IsState(em, EM_GROUP_ATTACK, KUTKU_ATK_FEIGN_DEATH) != 0)) {
        group = 3;
        state = 7;
    }
    switch (group) {
    case EM_GROUP_IDLE:
        EmKutku_SetIdleState(em, state, mode);
        break;
    case EM_GROUP_MOVE:
        EmKutku_SetMoveState(em, state, mode);
        break;
    case EM_GROUP_FLY:
        EmKutku_SetFlyState(em, state, mode);
        break;
    case EM_GROUP_ATTACK:
        EmKutku_SetAttackState(em, state, mode);
        break;
    case EM_GROUP_REACT:
        EmKutku_SetReactState(em, state, mode);
        break;
    case EM_GROUP_DEFEAT:
        if (state == 3 && QuestEm_CanAppear(em) == 1) {
            QuestEm_Appear(em);
        }
    case EM_GROUP_EVENT:
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        break;
    case EM_GROUP_VERSUS:
        if (state == 0x98) {
            Versus_MarkDefeat(em);
        }
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        break;
    case EM_GROUP_EVENT2:
        break;
    }
}

/*
 * Picks the area escape point (four from em->tables[2]) farthest from the
 * target and makes it the goal (goalKind 2, arg 2, its index); no target:
 * KUTKU_FLY_ROUTE; no table: goal (1000, 0, 1000).
 */
void EmKutku_SetFarthestEscapePoint(Em *em)
{
    f32 targetPos[3];
    f32 ptDist[4];
    f32 (*escapePts)[3];
    u32 i;
    f32 (*pt)[3];
    f32 bestDist;
    f32 delta;
    u8 farIdx;
    f32 *farPt;

    if (em->targetNo == -1) {
        EmKutku_ChangeState(em, EM_GROUP_FLY, KUTKU_FLY_ROUTE, EM_SYNC_SEND);
        return;
    }
    escapePts = (f32 (*)[3]) Em_AreaEntryValue(em, em->tables[2]);
    if (escapePts == NULL) {
        em->goal[0] = 1000.0f;
        em->goal[1] = 0.0f;
        em->goal[2] = 1000.0f;
        return;
    }
    pt = escapePts;
    Em_GetTargetPos(em, em->targetNo, targetPos);
    for (i = 0; i < 4; i++) {
        ptDist[i] = Vec_DistanceXZ(targetPos, *pt);
        pt++;
    }
    bestDist = ptDist[0];
    farIdx = 0;
    delta = ptDist[1];
    if (bestDist < delta) {
        bestDist = delta;
        farIdx = 1;
    }
    delta = ptDist[2];
    if (bestDist < delta) {
        bestDist = delta;
        farIdx = 2;
    }
    delta = ptDist[3];
    if (bestDist < delta) {
        farIdx = 3;
    }
    farPt = escapePts[farIdx];
    em->goalKind = 2;
    em->goalArg = 2;
    em->goalNo = farIdx;
    em->curGoalKind = em->goalKind;
    em->curGoalArg = *(u16 *) &em->goalArg;
    em->curGoalNo = *(u16 *) &em->goalNo;
    em->goal[0] = farPt[0];
    em->goal[1] = farPt[1];
    em->goal[2] = farPt[2];
}

/*
 * Turns rotX by up to pitchStep toward: mode 0 the target position plus
 * height, mode 1 pitch 0xD000, mode 2 level; only within the +-0x3000 pitch
 * band.
 */
void EmKutku_StepPitch(Em *em, s32 mode, f32 height)
{
    EmWork *work = (EmWork *) &em->work;
    f32 targetPos[3];
    s32 pitchDelta;

    if (em->targetNo == -1) {
        return;
    }
    switch (mode) {
    case 0:
        Em_GetTargetPos(em, em->targetNo, targetPos);
        pitchDelta = 0x10000 - Math_Heading(Vec_Distance((f32 *) &em->posX, (f32 *) targetPos), em->posY, 0.0f, targetPos[1] + height);
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
 * Sets velY from curve table kind (EmKutku_MoveVelYCurves) at the layer-0
 * frame when not blending, and moves along rotY.
 */
void EmKutku_MoveByVelYCurve(Em *em, s32 kind)
{
    EmWork *work = (EmWork *) &em->work;
    EmCurveKey *keys;
    EmCurveKey *key;
    f32 frame;
    f32 keyT;
    s32 i;

    keys = EmKutku_MoveVelYCurves[kind];
    if (keys != NULL && em->motion[0].blend == 0) {
        frame = em->motion[0].frame;
        if (frame == 0.0f || frame == 1.0f) {
            em->velY = keys->v;
        } else {
            i = 1;
            while (i != 0) {
                key = &keys[i];
                keyT = key->t;
                if (frame > keyT && keyT != 0.0f) {
                    i++;
                } else if (keyT == 0.0f) {
                    break;
                } else {
                    i = 0;
                    em->velY = (key->v - key[-1].v) / ((keyT - key[-1].t) / em->motion[0].step);
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
 * Arms the velY and/or speed curves for curve kind kind with its start
 * frame, and clears velocity.
 */
void EmKutku_StartCurves(Em *em, u8 kind)
{
    EmFlyerWork *w = (EmFlyerWork *)&em->work;

    w->curveNone = 0;
    w->curveVelY = 0;
    w->curveSpeed = 0;
    w->curveKind = kind;
    switch (w->curveKind) {
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
        w->curveFrame = 0x24;
        break;
    case 13:
        w->curveVelY = 1;
        w->curveFrame = 0;
        break;
    case 0x0:
        break;
    }
    Player_ClearVelocity(em);
}

/*
 * Returns 0 (the unused first curve slot).
 */
static s32 EmKutku_CurveNone(Em *em, EmWork *work)
{
    return 0;
}


extern EmCurveKey *EmKutku_VelYCurves[];

/*
 * Sets velY from EmKutku_VelYCurves[curveKind] at curveFrame; returns 2 at
 * the table end (t==0: then lets velY fall by 1 to -50; t<0: zeroes velY and
 * the frame), else 0.
 */
static u8 EmKutku_CurveVelY(Em *em, EmWork *work)
{
    EmFlyerWork *flyWork = (EmFlyerWork *) work;
    EmCurveKey *key;
    f32 frame;
    f32 keyT;
    s32 i;
    EmCurveKey *keys;
    u8 ended;

    keys = EmKutku_VelYCurves[((EmFlyerWork *)work)->curveKind];
    ended = 0;
    if (keys == NULL) {
        return ended;
    }
    frame = flyWork->curveFrame;
    if (frame != 0.0f && frame != 1.0f) {
        i = 1;
        while (i != 0) {
            key = &keys[i];
            keyT = key->t;
            if (frame > keyT && keyT > 0.0f) {
                i++;
            } else if (keyT == 0.0f) {
                ended = 2;
                if (em->velY > -50.0f) {
                    em->velY -= 1.0f;
                }
                break;
            } else if (keyT < 0.0f) {
                flyWork->curveFrame = 0;
                ended = 2;
                em->velY = 0.0f;
                break;
            } else {
                i = 0;
                em->velY = (key->v - key[-1].v) / ((keyT - key[-1].t) / em->motion[0].step);
            }
        }
    }
    return ended;
}

extern EmCurveKey *EmKutku_SpeedCurves[];

/*
 * Sets velZ from EmKutku_SpeedCurves[curveKind] at curveFrame; returns 4 at
 * the table end, else 0.
 */
static u8 EmKutku_CurveSpeed(Em *em, EmWork *work)
{
    EmFlyerWork *flyWork = (EmFlyerWork *) work;
    EmCurveKey *key;
    f32 frame;
    f32 keyT;
    s32 i;
    EmCurveKey *keys;
    u8 ended;

    keys = EmKutku_SpeedCurves[((EmFlyerWork *)work)->curveKind];
    ended = 0;
    if (keys == NULL) {
        return ended;
    }
    frame = flyWork->curveFrame;
    if (frame != 0.0f && frame != 1.0f) {
        i = 1;
        while (i != 0) {
            key = &keys[i];
            keyT = key->t;
            if (frame > keyT && keyT != 0.0f) {
                i++;
            } else if (keyT == 0.0f) {
                ended = 4;
                break;
            } else {
                i = 0;
                em->velZ = (key->v - key[-1].v) / ((keyT - key[-1].t) / em->motion[0].step);
            }
        }
    }
    return ended;
}

/*
 * Runs the armed curves, ORs their end bits (2 velY, 4 speed), advances
 * curveFrame by the motion step and, in the air (EM_STANCE_AIR), moves with
 * Em_MoveRotatedAccel along rotY; returns the bits.
 */
u8 EmKutku_UpdateCurves(Em *em)
{
    EmFlyerWork *flyWork = (EmFlyerWork *)&em->work;
    u8 ended = 0;

    if (flyWork->curveNone != 0) {
        ended = EmKutku_CurveNone(em, (EmWork *) flyWork);
    }
    if (flyWork->curveVelY != 0) {
        ended |= EmKutku_CurveVelY(em, (EmWork *) flyWork);
    }
    if (flyWork->curveSpeed != 0) {
        ended |= EmKutku_CurveSpeed(em, (EmWork *) flyWork);
    }
    flyWork->curveFrame += (s16) em->motion[0].step;
    if (em->stance == EM_STANCE_AIR) {
        flyWork->moveAngX = 0;
        flyWork->moveAngY = em->rotY;
        flyWork->moveAngZ = 0;
        Em_MoveRotatedAccel(em, &flyWork->moveAngX);
    }
    return ended;
}


/* The file's initialised data, in the game's order */
extern EmCurveKey EmCephadrome_VelYCurves_3[];
u16 EmKutku_StealItems[16] = {
#include "game/008C5790.inc"
};
EmFlyParam EmKutku_FlyParam = {
#include "game/008C57B0.inc"
};
EmFlyParam EmKutku_FlyParam2 = {
#include "game/008C5810.inc"
};
EmTurnParams EmKutku_TurnParams[2] = {
#include "game/008C5870.inc"
};
u16 EmKutku_FxFrameBone[4][2] = {
#include "game/008C58B0.inc"
};
Vec3f EmKutku_CarveThrowPt = {
#include "game/008C58C0.inc"
};
s32 GmEm_ReturnZero(void); /* gm_sub */
void *EmKutku_Callbacks[8] = {
    EmKutku_SpawnEffects, EmKutku_SpawnEffects, EmKutku_TableNop, EmKutku_UpdateMotionEffects,
    GmEm_ReturnZero, EmKutku_Reinit, NULL, NULL,
};
EmCurveKey EmKutku_MoveVelYCurves_0[8] = {
#include "game/008C58F0.inc"
};
EmCurveKey EmKutku_MoveVelYCurves_1[20] = {
#include "game/008C5930.inc"
};
EmCurveKey EmKutku_MoveVelYCurves_3[10] = {
#include "game/008C59D0.inc"
};
EmCurveKey EmKutku_MoveVelYCurves_5[6] = {
#include "game/008C5A20.inc"
};
EmCurveKey EmKutku_MoveVelYCurves_6[10] = {
#include "game/008C5A50.inc"
};
EmCurveKey *EmKutku_MoveVelYCurves[16] = {
    EmKutku_MoveVelYCurves_0, EmKutku_MoveVelYCurves_1, NULL, EmKutku_MoveVelYCurves_3,
    NULL, EmKutku_MoveVelYCurves_5, EmKutku_MoveVelYCurves_6, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
};
EmCurveKey EmKutku_VelYCurves_3[10] = {
#include "game/008C5AE0.inc"
};
EmCurveKey EmKutku_VelYCurves_5[14] = {
#include "game/008C5B30.inc"
};
EmCurveKey EmKutku_VelYCurves_8[8] = {
#include "game/008C5BA0.inc"
};
EmCurveKey EmKutku_VelYCurves_14[8] = {
#include "game/008C5BE0.inc"
};
EmCurveKey EmKutku_VelYCurves_9[18] = {
#include "game/008C5C20.inc"
};
EmCurveKey EmKutku_VelYCurves_10[14] = {
#include "game/008C5CB0.inc"
};
EmCurveKey EmKutku_VelYCurves_11[4] = {
#include "game/008C5D20.inc"
};
EmCurveKey EmKutku_VelYCurves_12[6] = {
#include "game/008C5D40.inc"
};
EmCurveKey EmKutku_VelYCurves_13[24] = {
#include "game/008C5D70.inc"
};
EmCurveKey *EmKutku_VelYCurves[16] = {
    NULL, NULL, (EmCurveKey *)EmCephadrome_VelYCurves_3, EmKutku_VelYCurves_3,
    NULL, EmKutku_VelYCurves_5, NULL, NULL,
    EmKutku_VelYCurves_8, EmKutku_VelYCurves_9, EmKutku_VelYCurves_10, EmKutku_VelYCurves_11,
    EmKutku_VelYCurves_12, EmKutku_VelYCurves_13, EmKutku_VelYCurves_14, NULL,
};
EmCurveKey EmKutku_SpeedCurves_5[14] = {
#include "game/008C5E70.inc"
};
EmCurveKey EmKutku_SpeedCurves_8[8] = {
#include "game/008C5EE0.inc"
};
EmCurveKey EmKutku_SpeedCurves_14[8] = {
#include "game/008C5F20.inc"
};
EmCurveKey EmKutku_SpeedCurves_9[14] = {
#include "game/008C5F60.inc"
};
EmCurveKey EmKutku_SpeedCurves_10[12] = {
#include "game/008C5FD0.inc"
};
EmCurveKey *EmKutku_SpeedCurves[16] = {
    NULL, NULL, NULL, NULL,
    NULL, EmKutku_SpeedCurves_5, NULL, NULL,
    EmKutku_SpeedCurves_8, EmKutku_SpeedCurves_9, EmKutku_SpeedCurves_10, NULL,
    NULL, NULL, EmKutku_SpeedCurves_14, NULL,
};
