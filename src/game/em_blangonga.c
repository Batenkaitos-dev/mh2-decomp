/*
 * The Blangonga family: Blangonga, Congalala and Rajang, and the small
 * Blango and Conga (EM_KIND_BLANGONGA, _CONGALALA, _RAJANG, _BLANGO,
 * _CONGA): the code from the family's first function,
 * EmBlangonga_MotionSounds, to the next family's (EmBlangonga_Setup, called
 * by the per-kind dispatch Em_TypeSetup, sets a monster up), to Shen
 * Gaoren's state changes.
 *
 * A monster of the family runs a state machine (include/em_blangonga.h
 * names the states and motions; include/em.h the groups): EmBlangonga_Update
 * reads the hits (Em_UpdateDamage) and picks the reactions, then
 * EmBlangonga_RunState calls the group's dispatcher (EmBlangonga_RunIdleState
 * ... RunVersusState), which calls one function per state with the monster,
 * its work area (EmWork; the family's view is EmBlangongaWork) and, for the
 * functions several states share, a variant. A state function switches on
 * Em.step; EmBlangonga_ChangeState starts the next state and
 * EmBlangonga_EndState hands the choice back to the monster script, which
 * asks its questions through EmBlangonga_ScriptQuery. The family doesn't
 * fly: its group 2 holds the jumps, the leaps between areas and the
 * Blangonga kinds' burrowing under the snow. Congalala carries one of three
 * items it can drop as a mine point, and eats to change its breath
 * (EmBlangongaWork.carryHidden, eatenKind); Blangonga's and Congalala's
 * story quests play scenes (group 6). EmBlangonga_MotionEffects adds each
 * motion's hits and effects, and EmBlangonga_MotionSounds,
 * MotionSoundsCongalala and MotionSoundsRajang its sounds.
 */
#include "common.h"
#include "scene.h"
#include "quest.h"
#include "em.h"
#include "em_blangonga.h"
#include "player.h"
#include "objattach.h"
#include "area.h"
/* prototypes of callees */
s32 EmBlangonga_RageTimerFull(Em *em);

/* prototypes */
void EmBlangonga_ChangeState(Em *em, u16 group, u16 state, u8 mode);
s8 EmMine_Create(Em *em, s32 idx);
s8 EmMine_CreateTimed(Em *em, f32 *pos, s32 idx);
s8 EmMine_CreateTimed0(Em *em, f32 *pos);
void EmMine_Remove(Em *em);
s32 EmMine_Update(Em *em);
/* unit's own declaration: declared here as the callers use it; with the definition's void (Em *em, s16 n), the calls in EmBlangonga_EatNeed2 compile differently */
void Em_AddHp(Em *em, s32 n);
void Em_BeginStatus1(Em *em);
void Em_PlaySeAtBone(Em *em, s32 se, s32 bone);
void Em_PlaySeAtBoneOnFrame(Em *em, s32 frame, s32 se, s32 bone);
void Em_SetMotion(Em *em, s32 motion, s32 a, s32 b);
s32 Motion_PassedFrame(void *, s32 layer, f32 frame);
s32 Obj_InCurrentArea(void *obj);
s32 Obj_SameArea(void *obj, Player *b);
void Player_SetFlag(void *obj, u32 flag);
void QuestEm_Captured(Em *em);
void QuestEm_Hunted(Em *em);
u16 Rand_Next(s32 stream);
void Vib_Start(s32 port, s32 i);
/* unit's own declaration: declared here as the callers use it; with the definition's void (Em *em, s32 bone, s32 a, s32 b, s32 c, s32 d), the calls in EmBlangonga_DigThrow compile differently */
void EmRangedAttack_CreateAtBone(Em *, s32, s32, s32, u16, s32);
void BlangongaAttack_Create(Em *em, s32 kind);
/* unit's own declaration: declared here as the callers use it; with the definition's void *(Em *em, s32 kind, s32 c, s32 a, s32 b), the calls in EmBlangonga_CongalalaEatenEmission compile differently */
void * EmEffectAttack_Create(Em *, s16, s16, s32, s32);
void Em_SelectSightRow(Em *em, u8 idx);
void Em_MoveRotatedAccel(Em *em, void *ang);
/* unit's own declaration: declared here as the callers use it; with the definition's void (Em *em, s32 group, s32 state, u8 mode), the calls in EmBlangonga_ChangeState, EmKushala_ChangeState and others compile differently */
void Em_SetStateSync(Em *em, u16 group, u16 state, u8 mode);
void Em_CheckRageByHp(Em *em, s16 n);
void Em_LoadReactionSet(Em *em, u8 k);
void Em_ClearLowHpNeed(Em *em);
void Em_ClearNeed2State(Em *em);
void Em_SpawnStatus4Effect(Em *em, s32 a);
void Em_AddNeed2(Em *em, s32 n);
void Em_SetHitReactMode(Em *em, u8 a);
void Em_SetTrapTimeGoal(Em *em);
/* unit's own declaration: declared here as the callers use it; with the definition's s32 (Em *em, s16 n, s16 max, s16 interval), the calls in EmBlangonga_Rest compile differently */
s32 Em_RegenHp(Em *em, s32 n, s32 max, s32 interval);
void Em_EndRage(Em *em);
void Em_ClearPoison(Em *em);
void Em_SetHold2(Em *em);
void Em_SetCaptured(Em *em, s32 a);
void Em_PlaySeTypeAtBoneInFrames(Em *em, s32 frame, s32 frame2, s32 se, s32 bone, s32 vol, s32 a);
void Em_PlaySeAtBoneVolOnFrame(Em *em, s32 frame, s32 se, s32 bone, s32 vol);
s32 Em_StepToGoal(Em *em, u16 step, u8 flags);
u8 Em_TurnToGoal(Em *em, u16 step, u8 flags);
void Em_StartTurnToTarget(Em *em, EmTurnParams *t);
void Em_StartTurnAway(Em *em, EmTurnParams *t);
s32 Em_TurnStep(Em *em, EmTurnParams *t);
s32 Em_PartBroken(Em *em, s32 arg1);
void Em_RestartScript(Em *em);
void Em_AreaExitPoint(Em *em, f32 *out);
void Em_TargetPlayerInRange(Em *);
void EmBlangonga_Init(Em *);
s32 EmBlangonga_IsSmallKind(Em *em);
void Em_SetPartScaled(Em *em, s32 bit);
s32 Em_ComputeMaxHp(Em *em, s16 idx);
void Em_InitParts(Em *em);
s32 Quest_IsVersus(void);
s32 Versus_TeamHasFlag(Em *em, s32 kind);
void EmBlangonga_ShowCarried(Em *em, u16 part);
f32 Math_Absf(f32 x);
f32 Math_Sin(f32 x);
void EmBlangonga_SpawnPeriodicEffect06(Em *em, s32 kind);
/* unit's own declaration: declared here as the callers use it; with the definition's void (Em *em, s32 bone, f32 *pos, f32 scale), the calls in EmBlangonga_SpawnPeriodicEffect06 compile differently */
void Em_SpawnLyingPuffs(Em *em, u8 bone, f32 *pos, f32 scale);
void Em_ApplyPartBreaks(Em *em);
void GmEm_ResetSize(Em *em);
EmBone *Em_Bone(void *obj, s16 bone);
s32 Em_PlaySeChecked(Em *em, s32 id, s32 a, void *pos, s32 b, s32 c);
u16 EmBlangonga_PickCarried(EmBlangongaPickRow *p, u16 r);
extern EmBlangongaTurnData EmBlangonga_TurnData;
extern Vec3f EmBlangonga_CarveThrowPt __attribute__((aligned(8)));
void VersusBait_Create(Player *p, Em *em, u8 k, s32 unused, f32 *pos);
void Versus_RouteEmByTeam(Em *em);
s32 Versus_DefeatCamera(Em *pl);
void Em_FlagPartBreak(Em *em, s32 arg1);
void EmBlangonga_StartCurves(Em *em, u8 kind);
void EmBlangonga_RunIdleState(Em *em, EmWork *work);
void EmBlangonga_Turn(Em *, s32, s32);
void EmBlangonga_Approach(Em *, s32, s32);
void EmBlangonga_Jump(Em *, EmWork *, u8);
void EmBlangonga_Hop(Em *, EmWork *, s32);
void EmBlangonga_DigOut(Em *, EmWork *, s32);
void EmBlangonga_BuriedTurn(Em *);
void EmBlangonga_LeapOutOfArea(Em *, EmWork *);
void EmBlangonga_DropIn(Em *, EmWork *);
void EmBlangonga_CrossArea(Em *, EmWork *);
void EmBlangonga_RunAttackState(Em *em, EmWork *work);
void EmBlangonga_RunReactState(Em *em, EmWork *work);
void EmBlangonga_Die(Em *, s32);
void EmBlangonga_DieHeld(Em *, s32);
void EmBlangonga_DieFalling(Em *, s32);
void EmBlangonga_Reappear(Em *em, s32 arg1);
void EmBlangonga_RunEventState(Em *em, EmWork *work);
void EmBlangonga_VersusEatBait(Em *, EmWork *);
void EmBlangonga_TurnAbout(Em *em, EmWork *work, s32 variant);
void EmBlangonga_EatNeed1(Em *em, EmWork *work, s32 variant);
void EmBlangonga_Slam(Em *em, EmWork *work, s32 variant);
void EmBlangonga_Strike(Em *em, EmWork *work, s32 variant);
void EmBlangonga_DigThrow(Em *em, EmWork *work, s32 variant);
void EmBlangonga_RajangComboSlam(Em *em, EmWork *work, s32 variant);
void EmBlangonga_RajangShot(Em *em, EmWork *work, s32 variant);
void EmBlangonga_RajangBeam(Em *em, EmWork *work, s32 variant);
void EmBlangonga_KnockDown(Em *em, EmWork *work, u8 variant);
/* unit's own declaration: declared here as the callers use it; with the definition's void (Em *em, u8 k, s16 v, s16 t, f32 y, f32 scale), the calls in EmBlangonga_KnockDown and EmBlangonga_HeldDowned compile differently */
void StunStars_CreateEm(Em *, s32, s32, s32, f32, f32);
void EmBlangonga_HeldDowned(Em *em, EmWork *work, u8 variant);
void EmBlangonga_BlangoEmerge(Em *em, EmWork *work, u8 variant);
void EmBlangonga_CongalalaEatenEmission(Em *em, EmWork *work);
void EmBlangonga_CongalalaCombo(Em *em, EmWork *work);
void EmBlangonga_EffectStrike(Em *em, EmWork *work);
void EmBlangonga_CongalalaScene(Em *em, EmWork *work);
void EmBlangonga_StartBlangongaScene(Em *em, EmWork *work);
void EmBlangonga_BlangongaLeapDown(Em *em, EmWork *work);
void EmBlangonga_MotionEffects(Em *, EmWork *);
u8 EmBlangonga_ScriptQuery(Em *, u8);
void ElementBurst_CreateOnEm(Em *em, u8 k, u8 c, f32 scale);
void MonsterFx_CreateOnEmAngled(Em *em, u8 k, u16 w, s16 v, f32 scale);
s32 Sound_RandomSe(s32 id, s16 lo, s32 alt, s16 n);
s32 EmMotion_PassedFrameScaled(void *p, f32 frame, s32 layer);
s32 EmMotion_ReachedFrame(void *obj, s32 a, f32 frame);
f32 Vec_Distance(f32 *a, f32 *b);
void CamShake_ReqAIf(Em *obj, s32 i);
void Vec_Set(f32 *out, f32 x, f32 y, f32 z);
s32 ObjListA70_Add(EmObj *obj);
s32 Versus_BothDefeated(void);
s32 QuestEm_CanAppear(Em *em);
void Player_ClearVelocity(void *);
/* the calls that mask the value again (u8 or & 0xFF) cast it to s32 first: MWCC keeps the game's andi that way */
u8 Player_InCurrentArea(void *obj);
void Shadow_Spawn(void *obj, s16 type);
void Em_BeginStatus4(Em *em);
void Em_BeginStatus5(Em *em);
void Em_BreakPart(Em *em, s32 part);
/* unit's own declaration: Em_EndAllStatus reads a0 = em at EmKushala_SetReactState state 6 (left by EmKushala_SetWind(em, ...), which keeps a0) */
void Em_EndAllStatus();
void Em_SetMotionRow(Em *em, s32 row, s32 arg2, s32 arg3);
extern f32 gAreaGroundY[];
/* unit's own declaration: declared here as the callers use it; with the definition's void (void *obj, s16 bone, f32 *out), the calls in EmBlangonga_SpawnGroundEffect compile differently */
void Em_BonePos(void *obj, s32 a, f32 *out);
void PartFx_SpawnOnGround(f32 *pos, s32 a, f32 scale);
void Em_SetSpeedScaled(Em *em, u16 flags, f32 x, f32 y);
s32 EmMotion_InFrameRange(void *em, s32 layer, f32 from, f32 to);
u16 Math_AngleXZ(f32 *from, f32 *to);
f32 Vec_DistanceXZ(f32 *a, f32 *b);
void Em_ResetNotice(Em *em);
void Em_RefillNeed3(Em *em);
void Em_BeginStatus2(Em *em);
void Em_EndStatus1(Em *em);
void Em_EndStatus2(Em *em);
void Em_EndStatus4(Em *em);
void Em_EndStatus5(Em *em);
void Em_StartGoalAction(Em *em, s32 arg1);
void Em_AddNeed1(Em *em, s32 n);
void Em_ClearNeed1State(Em *em);
void Vec_Copy(f32 *out, f32 *in);
struct EmIdEntry;
struct EmIdEntry *Em_FindAreaEntry(Em *em, struct EmIdEntry *entry, s16 id);
extern f32 gStartPos[];
void Em_ComputeGoal(Em *em, f32 *target);
void Em_TypeSetup(Em *em);
s32 Game_IsOnline(void);
void Versus_MarkDefeat(Em *);
s8 QuestEm_Appear(Em *em);
void Em_ResetToBasePose(Em *em);
void Em_SetTargetAngle(Em *em);
s32 Em_GetTargetPos(Em *em, u8 idx, f32 *out);
void Em_StartRage(Em *em);
void Em_StartPendingRage(Em *em);
void Em_ComputeGoal2(Em *em, f32 *target);
/* unit's own declaration: declared here as the callers use it; with the definition's s16 (void *obj, u8 state, u8 sub), the calls in EmBlangonga_Update compile differently */
s32 Player_IsState(void *obj, s32 group, s32 state);
void MinePoint_Remove(s32);
static s32 EmBlangonga_IsBlangonga(Em *em);
static s32 EmBlangonga_IsCongalala(Em *em);
s32 Em_IsSyncFree(Em *em, u8 mode);
u16 Map_DayId(u16);
u16 Area_DayId(u16 id);
void ObjListA70_Remove(EmObj *obj);
void EmBlangonga_HideCarried(Em *em, u16 part);
u16 Quest_Random(void);
u16 Area_IdForCurrentTime(u16);
void MonsterFx_CreateOnEmVariant(Em *em, u8 k, u8 c, f32 scale);
struct StageObj *MonsterFx_CreateOnEm(struct Em *em, u8 k, f32 scale);
void EmImpactAttack_CreateAt(Em *em, f32 *pos, s8 kind);
void EmRangedAttack_CreateAtBoneTimed(Em *em, s32 bone, s32 a, s32 b, s32 c, s32 d, s32 e);
void Vec_RotateSelf(f32 *v, f32 *m);
void Mtx_GetTrans(f32 *out, f32 *m);
void Vec_Add(f32 *, f32 *, f32 *);
s32 Area_ContainsPos(s32, f32 *);
void Em_EnterNextArea(Em *);
void Em_MoveRotated(Em *em, void *src);
void EmBlangonga_EnterNewArea(Em *);
void EmBlangonga_DropFallingObjects(Em *em);
void AttackBurst_CreateAtScaled(f32 *pos, s32 v, s16 k, f32 scale);
s32 Em_UpdateCombatTimer(Em *em);
void Em_RaisePlayerAreaTrigger(Em *em);
void Em_HoldPosNoPlayers(Em *em, u16 a, u16 b);
u8 Em_SenseObjects(Em *em, EmObj *self);
s32 Em_ScriptFlagsFrom(Em *em, u8 mask);
void Em_CheckLowHp(Em *em, f32 ratio);
void Em_DecayNeed1(Em *em);
void Em_DecayNeed1Only(Em *em);
void Em_DecayNeed2(Em *em);
u8 Em_UpdateDamage(Em *em, u8 *out);
void Em_SetTrapTime(Em *em, u8 *p);
void Em_SetStatus1Timer(Em *em);
void Em_SetStatus2Timer(Em *em);
void Em_SetStatus4Timer(Em *em);
void Em_SetStatus5Timer(Em *em);
s32 Em_IsHeld(Em *em);
void Em_ResetKindTimer(Em *em);
s32 Em_DamageResultIsReaction(Em *em, s32 arg1);
s32 Save_TestEventFlag(s32 flag);
void Em_UpdatePendingScript(Em *em);
void Em_ClearHitReactMode(Em *em);
void EmBlangonga_RunState(Em *em, s32 arg1);
void EmBlangonga_BreakParts(Em *em, s32 apply);
void Em_PlayCommonSeAtBoneOnFrame(Em *em, s32 frame, s32 se, s32 bone);
/* end of prototypes */

/*
 * Plays the sounds of the motion on layer 0 at its frames
 * (Em_PlaySeAtBoneOnFrame: frame, sound id, bone) for Blangonga and Blango
 * (EmBlangonga_MotionEffects calls it each frame), switching on the motion
 * id in EmWork.prevMotion.
 */
void EmBlangonga_MotionSounds(Em *em, EmWork *work)
{
    s16 prevMotion;

    prevMotion = work->prevMotion;
    switch (prevMotion) {
    case EM_MOTION_ID(0, BLANG_MOT_WALK):
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x9E, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x5E, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x90, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TURN_LEFT):
        Em_PlaySeAtBoneOnFrame(em, 0x16, Sound_RandomSe(0x19, 2, 0x19, 2), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x52, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TURN_RIGHT):
        Em_PlaySeAtBoneOnFrame(em, 0x16, Sound_RandomSe(0x19, 2, 0x19, 2), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x52, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 3, 0x17);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TURN_LEFT_QUICK):
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x2C, 1, 0x2C, 2), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 3, 0x17);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TURN_RIGHT_QUICK):
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x2C, 1, 0x2C, 2), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_LIMP):
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x2D, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0x2D, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x68, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0x12, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 0x12, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 3, 0x17);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_MOVE_ACTION_D):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x1D, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x14, 3);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 5, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x42, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x54, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x60, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 5, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 4, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 4, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x52, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x60, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_MOVE_ACTION_A):
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x1D, 1, 0x1E, 1), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x14, 3);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 5, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 5, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_MOVE_ACTION_B):
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x1D, 1, 0x1E, 1), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x14, 3);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 5, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 5, 0x17);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_MOVE_ACTION_C):
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x1D, 1, 0x1E, 1), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x14, 3);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 5, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 3, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 5, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 4, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 4, 0x17);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_JUMP):
        if (em->type == 0x3D) {
            Em_PlaySeAtBoneVolOnFrame(em, 0x16, 0x36, 0x13, 7);
            Em_PlaySeAtBoneOnFrame(em, 4, 6, 9);
            Em_PlaySeAtBoneOnFrame(em, 8, 0x12, 9);
            Em_PlaySeAtBoneOnFrame(em, 8, 3, 0x1B);
            Em_PlaySeAtBoneOnFrame(em, 0x32, 5, 0x1B);
            Em_PlaySeAtBoneOnFrame(em, 0x34, 4, 0x17);
            Em_PlaySeAtBoneOnFrame(em, 0x34, 0x14, 3);
            break;
        }
        Em_PlaySeAtBoneVolOnFrame(em, 0x16, 0x36, 0x13, 7);
        Em_PlaySeAtBoneVolOnFrame(em, 0x36, 0x24, 0x13, 7);
        Em_PlaySeAtBoneVolOnFrame(em, 0x36, 0x3F, 0x13, 7);
        Em_PlaySeAtBoneVolOnFrame(em, 0x46, 0x40, 0x13, 7);
        Em_PlaySeAtBoneOnFrame(em, 4, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 8, 0x12, 9);
        Em_PlaySeAtBoneOnFrame(em, 8, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 5, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 4, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 0x14, 3);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_CHARGE):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x1E, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x28, Sound_RandomSe(0x1D, 3, 0x1E, 1), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 5, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 4, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 8, 5, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 5, 0x17);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_JUMP_LAND):
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x37, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 8, 8, 3);
        Em_PlaySeAtBoneOnFrame(em, 6, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x5E, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 3, 0x17);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DIG_IN):
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0x10, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 0x10, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x5A, 0x11, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x68, 0x11, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x76, 0x11, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x84, 0x11, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x92, 0x12, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x9E, 0x12, 9);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_EMERGE):
        if (em->type == 0x3D) {
            Em_PlaySeAtBoneVolOnFrame(em, 4, 0xD, 0x12, 6);
            Em_PlaySeAtBoneVolOnFrame(em, 4, 0x1A, 0x13, 6);
            Em_PlaySeAtBoneOnFrame(em, 0x58, 0x2C, 0x13);
            Em_PlaySeAtBoneOnFrame(em, 4, 0x14, 0x12);
            Em_PlaySeAtBoneOnFrame(em, 0x34, 2, 9);
            Em_PlaySeAtBoneOnFrame(em, 0x3C, 1, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x82, 6, 9);
            Em_PlaySeAtBoneOnFrame(em, 0x92, 0, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x38, 5, 0x17);
            Em_PlaySeAtBoneOnFrame(em, 0x3C, 4, 0x1B);
            Em_PlaySeAtBoneOnFrame(em, 0x7A, 3, 0x1B);
            Em_PlaySeAtBoneOnFrame(em, 0x38, 8, 0x1B);
            break;
        }
        if ((em->stateGroup == 0) && (em->state == 0xE)) {
            Em_PlaySeAtBoneOnFrame(em, 0x14, 0x14, 0x12);
            Em_PlaySeAtBoneOnFrame(em, 0x12, 7, 0x12);
            Em_PlaySeAtBoneOnFrame(em, 0x12, 0x1A, 0x13);
            Em_PlaySeAtBoneOnFrame(em, 0x58, 0x2C, 0x13);
            Em_PlaySeAtBoneOnFrame(em, 0x12, 0x14, 0x12);
            Em_PlaySeAtBoneOnFrame(em, 0x34, 2, 9);
            Em_PlaySeAtBoneOnFrame(em, 0x3C, 1, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x82, 6, 9);
            Em_PlaySeAtBoneOnFrame(em, 0x92, 0, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x38, 5, 0x17);
            Em_PlaySeAtBoneOnFrame(em, 0x3C, 4, 0x1B);
            Em_PlaySeAtBoneOnFrame(em, 0x7A, 3, 0x1B);
            Em_PlaySeAtBoneOnFrame(em, 0x38, 8, 0x1B);
            break;
        }
        Em_PlaySeAtBoneOnFrame(em, 6, 0x17, 0x12);
        Em_PlaySeAtBoneOnFrame(em, 4, 7, 0x12);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x1A, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 0x2C, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x14, 0x12);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x82, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x92, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 5, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 4, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x7A, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 8, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_WALK_START):
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_LIE):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x2A, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 0x29, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xA2, 0x2A, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x104, 0x29, 0x13);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_GET_UP):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x10, 3);
        Em_PlaySeAtBoneOnFrame(em, 2, 0x2B, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x82, 0x35, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 0xF, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x8C, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xA0, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x6E, 3, 0x17);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_COLLAPSE):
        Em_PlaySeAtBoneOnFrame(em, 0x28, 0x33, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xA0, 0x29, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xDC, 0x2A, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 2, 0xF, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x42, 0x12, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x54, 8, 0xB);
        Em_PlaySeAtBoneOnFrame(em, 0x76, 0x10, 0xB);
        Em_PlaySeAtBoneOnFrame(em, 0x94, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xA4, 4, 0x17);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_LIE_DOWN):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x32, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0x33, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 4, 6, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xA2, 0x10, 0xB);
        Em_PlaySeAtBoneOnFrame(em, 0xB4, 7, 3);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TRIPLE_HIT):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x1C, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0x61, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x42, 5, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x42, 0x14, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 0xC, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 0x3D, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x5E, 8, 0xB);
        Em_PlaySeAtBoneOnFrame(em, 0x72, 0x11, 0xB);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TRIPLE_HIT_END):
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0x38, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 4, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 4, 5, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x36, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 3, 0x17);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_SLAM_WINDUP):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x5C, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x13, 0xE);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_STRIKE_A):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x36, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 0x35, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 8, 0x15, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0xA, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x60, 1, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_STRIKE_B):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x36, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 0x35, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 8, 0x15, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0xA, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x60, 1, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 3, 0x17);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_THROW):
        Em_PlaySeAtBoneOnFrame(em, 0x37, 0x39, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x13, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 0x15, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x9C, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0xAE, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x60, 6, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DIG_THROW):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x2F, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 0x2E, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xBA, 0x2C, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x62, 8, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x62, 9, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0x12, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x62, 0x13, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 0x15, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x8C, 5, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xC2, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 4, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x8C, 5, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x8E, 4, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0xBA, 3, 0x17);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_CHARGE_END):
        Em_PlaySeAtBoneOnFrame(em, 8, 0x1B, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 6, 0x14, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x54, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 4, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 3, 0x17);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_ALERT_IDLE):
        if (em->type == 0x3D) {
            Em_PlaySeAtBoneOnFrame(em, 0x18, 0x37, 0x13);
            Em_PlaySeAtBoneOnFrame(em, 0x48, 0x38, 0x13);
            Em_PlaySeAtBoneOnFrame(em, 0x68, 0x36, 0x13);
            Em_PlaySeAtBoneOnFrame(em, 0xA8, 0x3C, 0x13);
            Em_PlaySeAtBoneOnFrame(em, 0x52, 0, 9);
            Em_PlaySeAtBoneOnFrame(em, 0x56, 0, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x88, 2, 9);
            Em_PlaySeAtBoneOnFrame(em, 0x8E, 2, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0xC2, 1, 9);
            Em_PlaySeAtBoneOnFrame(em, 0x1C, 3, 0x1B);
            Em_PlaySeAtBoneOnFrame(em, 0xC0, 4, 0x1B);
            break;
        }
        Em_PlaySeAtBoneOnFrame(em, 8, 0x37, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 0x38, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x68, 0x36, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xA8, 0x1B, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x52, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x56, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x88, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x8E, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0xC2, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0xC0, 4, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_ROAR):
        if (em->type == 0x3D) {
            Em_PlaySeAtBoneOnFrame(em, 4, 0x36, 0x13);
            Em_PlaySeAtBoneOnFrame(em, 0x32, 0x3C, 0x13);
            Em_PlaySeAtBoneOnFrame(em, 0x38, 3, 0x1B);
            Em_PlaySeAtBoneOnFrame(em, 0x3A, 2, 9);
            Em_PlaySeAtBoneOnFrame(em, 0x40, 1, 0xE);
            break;
        }
        Em_PlaySeAtBoneVolOnFrame(em, 0x1E, 0x63, 0x13, 7);
        Em_PlaySeAtBoneVolOnFrame(em, 0x3C, 0x3F, 0x13, 7);
        Em_PlaySeAtBoneOnFrame(em, 0x78, 0x2C, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 8, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x72, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x76, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x9A, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x9C, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x8E, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0xA4, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TURN_ABOUT):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x2B, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 0x35, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x100, 0x19, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x168, 0x2B, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x8A, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0xB0, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xDA, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0xF0, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x1B0, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x1C8, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x66, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x8C, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x1CA, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TURN_ABOUT_B):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x2B, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 0x35, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x100, 0x19, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x168, 0x2B, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x8A, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xB0, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0xDA, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xF0, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x1B0, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x1C8, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x66, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x8C, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x1CA, 3, 0x17);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_EAT_NEED2):
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0x23, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x78, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 3, 0x17);
        Em_PlayCommonSeAtBoneOnFrame(em, 0x36, 0x2E, 0x13);
        Em_PlayCommonSeAtBoneOnFrame(em, 0x7C, 0x2E, 0x13);
        Em_PlayCommonSeAtBoneOnFrame(em, 0xA2, 0x2E, 0x13);
        Em_PlayCommonSeAtBoneOnFrame(em, 0xC6, 0x2E, 0x13);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_EAT_NEED2_END):
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x2C, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x7A, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_EAT):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x2B, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0x3E, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x36, 0x3D, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x5E, 0x3D, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x7E, 0x3D, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xB8, 0x3E, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x122, 0x3D, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x140, 0x3D, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x186, 0x3E, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x16C, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 3, 0x17);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_EAT_END):
        Em_PlaySeAtBoneOnFrame(em, 0x36, 0x2C, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x7C, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_ROAR_B):
        Em_PlaySeAtBoneVolOnFrame(em, 0x3A, 0x4C, 0x13, 7);
        Em_PlaySeAtBoneVolOnFrame(em, 0xAA, 0x3F, 0x13, 7);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x94, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xA2, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x94, 1, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0xA2, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_FLINCH_B):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x50, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x8E, 0x2C, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x56, 9, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x5E, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0xA0, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xA2, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 4, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x42, 4, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x8A, 4, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0xA6, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_FLINCH):
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x48, 4, 0x47, 4), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 0x23, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 4, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_SHAKE_OFF):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x48, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 0x26, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 0x23, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xA4, 0x19, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 6, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0xB2, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0xC6, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0xA4, 3, 0x17);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DOWN):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x51, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x20, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 0x11, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 7, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x74, 4, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0x12, 0x17);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DOWN_GET_UP):
        Em_PlaySeAtBoneOnFrame(em, 6, 0x54, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x12, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_STATUS4):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x48, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x4E, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x7C, 0x25, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x8A, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0xB4, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_FALL):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x4F, 0x13);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_FALL_LAND):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x47, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 8, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 7, 3);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DIE):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x4F, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 0x21, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xAC, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xB0, 0x11, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xC2, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 4, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x7E, 4, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0xB0, 8, 3);
        Em_PlaySeAtBoneOnFrame(em, 0xBC, 7, 3);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_KNOCK_DOWN):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x47, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 7, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x10, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0x12, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 7, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 2, 0xE);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_KNOCK_DOWN_B):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x47, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 7, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x10, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0x12, 0xB);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 7, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 2, 9);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DIE_END):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x49, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x11, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x88, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x12, 0x17);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TRAPPED):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x48, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 8, 0x10, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x12, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 9, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 7, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_STRUGGLE):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x58, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x31, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 6, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 2, 9);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DOWNED):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x4F, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x60, 0x22, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x56, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x66, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x68, 7, 0x13);
        break;
    case EM_MOTION_ID(0, 0x4C):
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x4B, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x68, 1, 9);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DOWN_B):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x51, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x20, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 0x11, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 7, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x74, 4, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0x12, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DOWN_GET_UP_B):
        Em_PlaySeAtBoneOnFrame(em, 6, 0x54, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x12, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 3, 0x17);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_EFFECT_STRIKE):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x5C, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x5E, 0x61, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 0x18, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 0x18, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 2, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x5A, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x5E, 2, 0xE);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_EFFECT_STRIKE_END):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x2C, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x5C, 3, 0x17);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_BACK_STRIKE):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x36, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x37, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 0x14, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 4, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 5, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 4, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x70, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x70, 6, 0xE);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_IDLE_ACTION_B):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x19, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x86, 0x35, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0xC6, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0xDC, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 0x11, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 0x11, 9);
        Em_PlaySeAtBoneOnFrame(em, 6, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 4, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0xCC, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0xE2, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_IDLE_ACTION_B_END):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x22, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 0x23, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x54, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x96, 3, 0x17);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_SLAM):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x61, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x88, 0x2C, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 9, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 8, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 0x17, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x7C, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 4, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 4, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x76, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0xA4, 3, 0x17);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_IDLE_ACTION_C):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x36, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x42, 0x35, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x8C, 0x39, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x92, 2, 0xE);
        
    case EM_MOTION_ID(0, BLANG_MOT_IDLE):
    default:
        break;
    }
}

/* EmBlangonga_MotionSounds for Congalala and Conga. */
void EmBlangonga_MotionSoundsCongalala(Em *em, EmWork *work)
{
    s16 prevMotion;

    prevMotion = work->prevMotion;
    switch (prevMotion) {
    case EM_MOTION_ID(0, BLANG_MOT_IDLE):
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0x53, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xA0, 0x3F, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x11E, 0x40, 0x13);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_WALK):
        Em_PlaySeAtBoneOnFrame(em, 0x34, Sound_RandomSe(0x3F, 2, 0x3F, 2), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x76, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TURN_LEFT):
        Em_PlaySeAtBoneOnFrame(em, 0x38, Sound_RandomSe(0x40, 2, 0x41, 2), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x52, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TURN_RIGHT):
        Em_PlaySeAtBoneOnFrame(em, 0x38, Sound_RandomSe(0x40, 2, 0x41, 2), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x52, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 3, 0x17);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TURN_LEFT_QUICK):
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x3C, 1, 0x3D, 2), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x36, Sound_RandomSe(0x41, 1, 0x41, 2), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 3, 0x17);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TURN_RIGHT_QUICK):
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x3C, 1, 0x3D, 2), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x36, Sound_RandomSe(0x41, 1, 0x41, 2), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_LIMP):
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x4F, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x68, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0x12, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 0x12, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 3, 0x17);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_MOVE_ACTION_D):
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x2D, 6, 0x2E, 2), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x14, 3);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 5, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 9, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x68, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x74, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 5, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 4, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x52, 4, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x66, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x72, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_MOVE_ACTION_C):
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x2D, 6, 0x2E, 2), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x5C, Sound_RandomSe(0x3C, 4, 0x3D, 4), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x14, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x36, 9, 3);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 5, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 5, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 4, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x74, 4, 0x17);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_JUMP):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x23, 0x13);
        Em_PlaySeAtBoneVolOnFrame(em, 0x36, 0x58, 0x13, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x4E, 0xF, 0x13, 8);
        Em_PlaySeAtBoneOnFrame(em, 4, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 8, 0x12, 9);
        Em_PlaySeAtBoneOnFrame(em, 8, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 5, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 4, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 0x14, 3);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_JUMP_FALL):
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x60, 0x13);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_CHARGE):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x3D, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x3C, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x3D, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 0x3C, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 0x3D, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x36, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 5, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 5, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 5, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 5, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_JUMP_LAND):
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x42, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 0x2E, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 8, 8, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 9, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x74, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x80, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 3, 0x17);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_LIE):
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x51, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 0x52, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xAA, 0x51, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xFA, 0x52, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x13C, 0x35, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x1E6, 0x3F, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xFA, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x110, 6, 0xE);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_GET_UP):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x10, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 0x3D, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 0x12, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x8C, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xA0, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x6E, 3, 0x17);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_COLLAPSE):
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0x53, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xBE, 0x29, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 2, 0x12, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x42, 0x12, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x54, 8, 0xB);
        Em_PlaySeAtBoneOnFrame(em, 0x76, 0x10, 0xB);
        Em_PlaySeAtBoneOnFrame(em, 0x94, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xA4, 4, 0x17);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_LIE_DOWN):
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0x53, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xC8, 0x35, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 0x10, 9);
        Em_PlaySeAtBoneOnFrame(em, 4, 6, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0xA2, 0x11, 0xB);
        Em_PlaySeAtBoneOnFrame(em, 0xB4, 7, 3);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_CONGALALA_EATEN_EMISSION):
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0x22, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0x5C, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 0x58, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x112, 0x3D, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x162, 0x40, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x12A, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 0x19, 0x13);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_CONGALALA_COMBO):
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0x2D, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x42, 0x2E, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x70, 0x2D, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x9C, 0x57, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xEE, 0x3C, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xFE, 0x3D, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x114, 0x29, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xAA, 0x12, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0xB6, 0x13, 3);
        Em_PlaySeAtBoneOnFrame(em, 0xC8, 8, 3);
        Em_PlaySeAtBoneOnFrame(em, 0xDC, 7, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 4, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 4, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x82, 4, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 0xB, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 0xB, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x76, 0xB, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0x12, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 0x12, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x7A, 0x12, 9);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_SLAM_WINDUP):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x23, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x13, 0xE);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_STRIKE_A):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x21, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0xB, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x5A, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x5C, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 4, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x5C, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_STRIKE_B):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x21, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0xB, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x5A, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x5C, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 4, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x5C, 3, 0x17);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_CONGALALA_EMISSION):
        if (em->type == 0x3E) {
            Em_PlaySeAtBoneOnFrame(em, 2, 0x22, 0x13);
            Em_PlaySeAtBoneOnFrame(em, 0x20, 0x23, 0x13);
            Em_PlaySeAtBoneOnFrame(em, 0x5A, 0x24, 0x13);
            Em_PlaySeAtBoneOnFrame(em, 0x72, 0x25, 0x13);
            Em_PlaySeAtBoneOnFrame(em, 0xA8, 0x4E, 0x13);
            Em_PlaySeAtBoneOnFrame(em, 0x10A, 0x3F, 0x13);
            Em_PlaySeAtBoneOnFrame(em, 0x16, 1, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x1C, 0, 9);
            Em_PlaySeAtBoneOnFrame(em, 0x136, 1, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x140, 0, 9);
            Em_PlaySeAtBoneOnFrame(em, 0x16, 3, 0x1B);
            Em_PlaySeAtBoneOnFrame(em, 0x140, 3, 0x1B);
            break;
        }
        Em_PlaySeAtBoneOnFrame(em, 2, 0x22, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 0x23, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x5A, 0x24, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x72, 0x16, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x7A, 0x18, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0xA8, 0x4E, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x10A, 0x3F, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x136, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x140, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x140, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_THROW):
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x39, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 0x2D, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 0x41, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 0x12, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x8A, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x7A, 4, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x8A, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x36, 0xC, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0x15, 0x1F);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_CONGALALA_HOP):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x39, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0x42, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x60, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 0x13, 3);
        Em_PlaySeAtBoneOnFrame(em, 2, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 4, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_HOP_LAND):
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0x24, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 0x35, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xC0, 0x40, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 8, 9, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 8, 3);
        Em_PlaySeAtBoneOnFrame(em, 6, 0x14, 3);
        Em_PlaySeAtBoneOnFrame(em, 0xA8, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0xBE, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x52, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x60, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x90, 4, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x76, 4, 0x17);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_ALERT_IDLE):
        Em_PlaySeAtBoneOnFrame(em, 0x20, 0x41, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 0x2D, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 0x2E, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xAA, 0x5F, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x52, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x56, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x88, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x8E, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0xAA, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x10A, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_ROAR):
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0x31, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 0x5A, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xA0, 0x33, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xDE, 0x40, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xBE, 7, 3);
        Em_PlaySeAtBoneOnFrame(em, 2, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 8, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xAE, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0xAE, 5, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 4, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0xE0, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TURN_ABOUT):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x22, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 0x40, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x13E, 0x41, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x16E, 0x43, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x8A, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0xB0, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xDA, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0xF0, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x1B0, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x1C8, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x66, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x8C, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x1CA, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TURN_ABOUT_B):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x22, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 0x40, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x13E, 0x41, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x16E, 0x43, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x8A, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xB0, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0xDA, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xF0, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x1B0, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x1C8, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x66, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x8C, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x1CA, 3, 0x17);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_EAT_NEED2):
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0x23, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x78, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0x3F, 0x13);
        Em_PlayCommonSeAtBoneOnFrame(em, 0x78, 0x2E, 0x13);
        Em_PlayCommonSeAtBoneOnFrame(em, 0x7C, 0x2E, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xA0, 0x3F, 0x13);
        Em_PlayCommonSeAtBoneOnFrame(em, 0xA2, 0x2E, 0x13);
        Em_PlayCommonSeAtBoneOnFrame(em, 0xC6, 0x2E, 0x13);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_EAT_NEED2_END):
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x43, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x7A, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_EAT):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x43, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 0x2D, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x80, 0x3E, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xA2, 0x3E, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xC2, 0x3E, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xDA, 0x3E, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xF8, 0x3E, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x118, 0x3E, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x138, 0x3E, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x154, 0x17, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x17A, 0x44, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x1C2, 0x41, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 0x12, 0x1C);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 8, 0x1C);
        Em_PlaySeAtBoneOnFrame(em, 0x182, 0x12, 0x1C);
        Em_PlaySeAtBoneOnFrame(em, 4, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x62, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x66, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x1AE, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x1D0, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x1E8, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 4, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x56, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x5A, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x17A, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x18A, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x1D0, 3, 0x17);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_FLINCH_B):
        Em_PlaySeAtBoneOnFrame(em, 6, 0x47, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 0x3C, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x7C, 0x3D, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 4, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x56, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x60, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0xA0, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xA2, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 4, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x42, 5, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 4, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x88, 4, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0xA6, 4, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_FLINCH):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x46, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 4, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 4, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_SHAKE_OFF):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x55, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x43, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0x3D, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 0x3C, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 0x2C, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x90, 0x40, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xA6, 0x3F, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 6, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0xB2, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0xC6, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0xA4, 3, 0x17);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DOWN):
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x4F, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 0x11, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 7, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x74, 4, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0x12, 0x17);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DOWN_B):
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x4F, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 0x11, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 7, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x74, 4, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0x12, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DOWN_GET_UP):
        Em_PlaySeAtBoneOnFrame(em, 6, 0x4E, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x12, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DOWN_GET_UP_B):
        Em_PlaySeAtBoneOnFrame(em, 6, 0x4E, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x12, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 3, 0x17);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_STATUS4):
        Em_PlaySeAtBoneOnFrame(em, 6, 0x46, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 0x50, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x7A, 0x12, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0xB8, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_FALL):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x60, 0x13);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_FALL_LAND):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x47, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 8, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 7, 3);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DIE):
        if (em->type == 0x3E) {
            Em_PlaySeAtBoneOnFrame(em, 4, 0x47, 0x13);
            Em_PlaySeAtBoneOnFrame(em, 0x60, 0x33, 0x13);
            Em_PlaySeAtBoneOnFrame(em, 0x78, 0x57, 0x13);
            Em_PlaySeAtBoneOnFrame(em, 0xA, 6, 9);
            Em_PlaySeAtBoneOnFrame(em, 0xAC, 2, 9);
            Em_PlaySeAtBoneOnFrame(em, 0xB0, 0x11, 9);
            Em_PlaySeAtBoneOnFrame(em, 0xC2, 2, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x16, 3, 0x17);
            Em_PlaySeAtBoneOnFrame(em, 0x3A, 4, 0x1B);
            Em_PlaySeAtBoneOnFrame(em, 0x7E, 4, 0x1B);
            Em_PlaySeAtBoneOnFrame(em, 0xB0, 8, 3);
            Em_PlaySeAtBoneOnFrame(em, 0xBC, 7, 3);
            break;
        }
        Em_PlaySeAtBoneOnFrame(em, 4, 0x60, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x60, 0x1F, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x78, 0x2F, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xE6, 0x3D, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xAC, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xB0, 0x11, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xC2, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 4, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x7E, 4, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0xB0, 8, 3);
        Em_PlaySeAtBoneOnFrame(em, 0xBC, 7, 3);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_KNOCK_DOWN):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x48, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 7, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x10, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0x12, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 7, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 2, 0xE);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_KNOCK_DOWN_B):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x48, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 7, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x10, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0x12, 0xB);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 7, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 2, 9);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DIE_END):
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x47, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 0x41, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x11, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x88, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x12, 0x17);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TRAPPED):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x48, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x10, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 6, 0x12, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 9, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 7, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 5, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 4, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_STRUGGLE):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x5E, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 4, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 5, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x36, 4, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 5, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 4, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x70, 5, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DOWNED):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x48, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x68, 7, 0x13);
        break;
    case EM_MOTION_ID(0, 0x4C):
        Em_PlaySeAtBoneOnFrame(em, 0x22, 0x12, 3);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 5, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 4, 0x17);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DROP_CARRIED):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x48, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0x31, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 8, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 0x12, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x96, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0xAC, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 4, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 4, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x74, 4, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x88, 4, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0xA0, 4, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_CONGALALA_COMBO_END):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x10, 3);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x35, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 0x43, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 4, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 4, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x5C, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_CONGALALA_HITZONE_COMBO):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x2D, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x5C, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x35, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x5C, 0x35, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 4, 6, 9);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_CONGALALA_HITZONE_COMBO_END):
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0x57, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x5E, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x62, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x84, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x5E, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x8E, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_CONGALALA_CLIMB_OUT):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x23, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 0x24, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x5A, 0x2D, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x86, 0x57, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xF4, 0x3D, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0x12, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0x14, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x5E, 8, 0x1C);
        Em_PlaySeAtBoneOnFrame(em, 0xD4, 8, 0x1C);
        Em_PlaySeAtBoneOnFrame(em, 0xC2, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0xC4, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xE0, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xF4, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x10A, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xE4, 4, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0xFC, 4, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x10C, 3, 0x17);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_CONGALALA_EAT):
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x2C, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x54, 0x3E, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 0x3E, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x82, 0x3E, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xA2, 0x3E, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xC6, 0x44, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 2, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x10C, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x112, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 4, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 4, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0xE4, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0xFA, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_BACK_STRIKE):
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0x2C, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 0x2C, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x9C, 0x40, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xC0, 0x41, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 4, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 4, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x98, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0xD2, 4, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0xB4, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xBA, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0xE4, 0, 9);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_IDLE_ACTION_B):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x43, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 0x31, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x8A, 0x5A, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x8C, 0xF, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xD2, 0xF, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 2, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 3, 0x17);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_IDLE_ACTION_B_END):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x22, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0x43, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x54, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x96, 3, 0x17);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_SLAM):
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x21, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x8E, 0x43, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 9, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 8, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 0x18, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x8A, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x42, 4, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 4, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x80, 4, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0xB0, 3, 0x17);
        
    default:
        break;
    }
}

/* EmBlangonga_MotionSounds for Rajang. */
void EmBlangonga_MotionSoundsRajang(Em *em, EmWork *work)
{
    s16 prevMotion;
    u8 stateGroup;

    prevMotion = work->prevMotion;
    switch (prevMotion) {
    case EM_MOTION_ID(0, BLANG_MOT_WALK):
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x9E, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x5E, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x90, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TURN_LEFT):
        Em_PlaySeAtBoneOnFrame(em, 0x16, Sound_RandomSe(0x2C, 2, 0x2C, 2), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x52, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TURN_RIGHT):
        Em_PlaySeAtBoneOnFrame(em, 0x16, Sound_RandomSe(0x2C, 2, 0x2C, 2), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x52, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 3, 0x17);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TURN_LEFT_QUICK):
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x39, 1, 0x39, 2), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 3, 0x17);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TURN_RIGHT_QUICK):
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x39, 1, 0x39, 2), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_LIMP):
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x55, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0x55, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x68, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0x12, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 0x12, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 3, 0x17);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_MOVE_ACTION_D):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x1D, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x14, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 0xB, 0x12);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 5, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x42, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x54, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x60, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 5, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 4, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 4, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x52, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x60, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_MOVE_ACTION_A):
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x1D, 1, 0x1E, 1), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x14, 3);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 5, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 5, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_MOVE_ACTION_B):
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x1D, 1, 0x1E, 1), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x14, 3);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 5, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 5, 0x17);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_MOVE_ACTION_C):
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x1D, 1, 0x1E, 1), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x14, 3);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 5, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 3, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 5, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 4, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 4, 0x17);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_JUMP):
        Em_PlaySeAtBoneVolOnFrame(em, 0x16, 0x36, 0x13, 7);
        Em_PlaySeAtBoneVolOnFrame(em, 0x36, 0x24, 0x13, 7);
        Em_PlaySeAtBoneOnFrame(em, 4, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 8, 0x12, 9);
        Em_PlaySeAtBoneOnFrame(em, 8, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 5, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 4, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 0x14, 3);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_JUMP_FALL):
        Em_PlaySeAtBoneOnFrame(em, 0x22, 0x14, 3);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_CHARGE):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x1E, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x28, Sound_RandomSe(0x1D, 3, 0x1E, 1), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 5, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 4, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 8, 5, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 5, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x15, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 0x15, 3);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_JUMP_LAND):
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x37, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 0x39, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 8, 9, 3);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 7, 3);
        Em_PlaySeAtBoneOnFrame(em, 6, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x6E, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x84, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 3, 0x17);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_EMERGE):
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0x14, 0x12);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 7, 0x12);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0x1A, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 0x2C, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0x14, 0x12);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x82, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x92, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 5, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 4, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x7A, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 8, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_WALK_START):
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_LIE):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x2A, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 0x29, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xA2, 0x2A, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x104, 0x29, 0x13);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_GET_UP):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x10, 3);
        Em_PlaySeAtBoneOnFrame(em, 2, 0x2B, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x82, 0x35, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x8C, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xA0, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x6E, 3, 0x17);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_COLLAPSE):
        Em_PlaySeAtBoneOnFrame(em, 0x28, 0x33, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xA0, 0x29, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xDC, 0x2A, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 2, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x42, 0x12, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x54, 8, 0xB);
        Em_PlaySeAtBoneOnFrame(em, 0x76, 0x10, 0xB);
        Em_PlaySeAtBoneOnFrame(em, 0x94, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xA4, 4, 0x17);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_LIE_DOWN):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x2A, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0x33, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 4, 6, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xA2, 0x10, 0xB);
        Em_PlaySeAtBoneOnFrame(em, 0xB4, 7, 3);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TRIPLE_HIT):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x1C, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0x19, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x42, 5, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x42, 0x14, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 0xC, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 0x3D, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x5E, 9, 0xB);
        Em_PlaySeAtBoneOnFrame(em, 0x60, 7, 0xB);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TRIPLE_HIT_END):
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x60, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 4, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 4, 5, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x36, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 3, 0x17);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_SLAM_WINDUP):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x34, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x13, 0xE);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_STRIKE_A):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x1C, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 0x1E, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x1D, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0xF, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0xF, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 0xF, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0xB, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 0xB, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 5, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x52, 5, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_STRIKE_B):
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x1B, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 5, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 0xD, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 9, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 7, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 5, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x42, 4, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 6, 0xE);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_CHARGE_END):
        Em_PlaySeAtBoneOnFrame(em, 4, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x1D, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0x37, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0x1B, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 0x14, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 0x12, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 8, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 0x11, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x82, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x96, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x42, 0xB, 0x12);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_ALERT_IDLE):
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0x5F, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 0x38, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x68, 0x36, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xA8, 0x1B, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x52, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x56, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x88, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x8E, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0xC2, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0xC0, 4, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_ROAR):
        Em_PlaySeAtBoneVolOnFrame(em, 0x1E, 0x63, 0x13, 7);
        Em_PlaySeAtBoneOnFrame(em, 0x78, 0x2C, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 8, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x72, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x76, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x9A, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x9C, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x8E, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0xA4, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TURN_ABOUT):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x53, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 0x35, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x100, 0x2C, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x168, 0x2B, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x8A, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0xB0, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xDA, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0xF0, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x1B0, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x1C8, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x66, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x8C, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x1CA, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TURN_ABOUT_B):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x53, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 0x35, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x100, 0x2C, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x168, 0x2B, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x8A, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xB0, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0xDA, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xF0, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x1B0, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x1C8, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x66, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x8C, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x1CA, 3, 0x17);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_EAT_NEED2):
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0x4B, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x78, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 3, 0x17);
        Em_PlayCommonSeAtBoneOnFrame(em, 0x36, 0x2E, 0x13);
        Em_PlayCommonSeAtBoneOnFrame(em, 0x7C, 0x2E, 0x13);
        Em_PlayCommonSeAtBoneOnFrame(em, 0xA2, 0x2E, 0x13);
        Em_PlayCommonSeAtBoneOnFrame(em, 0xC6, 0x2E, 0x13);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_EAT_NEED2_END):
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x2C, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x7A, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_EAT):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x53, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0x3E, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x36, 0x3D, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x5E, 0x3D, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x7E, 0x3D, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xB8, 0x3E, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x122, 0x3D, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x140, 0x3D, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x186, 0x3E, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x16C, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 3, 0x17);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_EAT_END):
        Em_PlaySeAtBoneOnFrame(em, 0x36, 0x54, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x7C, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_ROAR_B):
        Em_PlaySeAtBoneVolOnFrame(em, 0x3A, 0x4C, 0x13, 7);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x94, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xA2, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x94, 1, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0xA2, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_FLINCH_B):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x50, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x8E, 0x2C, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x56, 9, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x5E, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0xA0, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xA2, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 4, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x42, 4, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x8A, 4, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0xA6, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_FLINCH):
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x48, 4, 0x47, 4), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 0x23, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 4, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_SHAKE_OFF):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x48, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 0x26, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 0x23, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xA4, 0x2C, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 6, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0xB2, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0xC6, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0xA4, 3, 0x17);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DOWN):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x51, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x20, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 0x11, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 7, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x74, 4, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0x12, 0x17);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DOWN_GET_UP):
        Em_PlaySeAtBoneOnFrame(em, 6, 0x61, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x12, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_STATUS4):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x48, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x26, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x7C, 0x25, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x8A, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0xB4, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_FALL):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x4F, 0x13);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_FALL_LAND):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x47, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 8, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 7, 3);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DIE):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x4F, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 0x21, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xAC, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xB0, 0x11, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xC2, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 4, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x7E, 4, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0xB0, 8, 3);
        Em_PlaySeAtBoneOnFrame(em, 0xBC, 7, 3);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_KNOCK_DOWN):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x47, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 7, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x10, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0x12, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 7, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 2, 0xE);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_KNOCK_DOWN_B):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x47, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 7, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x10, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0x12, 0xB);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 7, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 2, 9);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DIE_END):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x49, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x11, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x88, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x12, 0x17);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TRAPPED):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x48, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 8, 0x10, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x12, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 9, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 7, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_STRUGGLE):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x58, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x59, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 6, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 2, 9);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DOWNED):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x4F, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x60, 0x22, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x56, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x66, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x68, 7, 0x13);
        break;
    case EM_MOTION_ID(0, 0x4C):
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x4B, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x68, 1, 9);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DOWN_B):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x51, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x20, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 0x11, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 7, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x74, 4, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0x12, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DOWN_GET_UP_B):
        Em_PlaySeAtBoneOnFrame(em, 6, 0x61, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x12, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 3, 0x17);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_RAJANG_AIM):
        stateGroup = em->stateGroup;
        if ((stateGroup == 3) && (em->state == 0x13)) {
            Em_PlaySeAtBoneOnFrame(em, 0x66, 0x16, 0x13);
            Em_PlaySeAtBoneOnFrame(em, 0x5A, 0x17, 0x13);
            Em_PlaySeAtBoneOnFrame(em, 0x66, 0x18, 0x13);
            Em_PlaySeAtBoneOnFrame(em, 0x1C, 0x56, 0x13);
            Em_PlaySeAtBoneOnFrame(em, 0x48, 0x4C, 0x13);
            Em_PlaySeAtBoneOnFrame(em, 4, 6, 9);
            Em_PlaySeAtBoneOnFrame(em, 0x4E, 2, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x52, 1, 9);
            break;
        }
        if ((stateGroup == 3) && (em->state == 0x18)) {
            Em_PlaySeAtBoneOnFrame(em, 0x66, 0x16, 0x13);
            Em_PlaySeAtBoneOnFrame(em, 0x5A, 0x18, 0x13);
            Em_PlaySeAtBoneOnFrame(em, 0x1C, 0x56, 0x13);
            Em_PlaySeAtBoneOnFrame(em, 0x48, 0x19, 0x13);
            Em_PlaySeAtBoneOnFrame(em, 4, 6, 9);
            Em_PlaySeAtBoneOnFrame(em, 0x4E, 2, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x52, 1, 9);
            break;
        }
        Em_PlaySeAtBoneOnFrame(em, 0x50, 0x16, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 0x5F, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 4, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x52, 1, 9);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_RAJANG_AIM_END):
        Em_PlaySeAtBoneOnFrame(em, 0x34, 0x22, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x66, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x84, 2, 0xE);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_RAJANG_JUMP):
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0x1A, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 4, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 8, 0x12, 9);
        Em_PlaySeAtBoneOnFrame(em, 8, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 5, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 4, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x14, 3);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_RAJANG_JUMP_SHOT):
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0x16, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x13, 3);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_RAJANG_JUMP_SHOT_LAND):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x37, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x39, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 8, 8, 3);
        Em_PlaySeAtBoneOnFrame(em, 6, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 5, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 4, 0x1B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_BACK_STRIKE):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x36, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x37, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 0x14, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 4, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 5, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 4, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x70, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x70, 6, 0xE);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_RAJANG_STRIKE):
        Em_PlaySeAtBoneOnFrame(em, 4, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 8, 0x37, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x74, 0x2C, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xA0, 0x38, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 0x13, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x12, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 0x13, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x60, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x60, 0x12, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x9A, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xA2, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0xAC, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 0xC, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0xB, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 0x14, 0xE);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_RAJANG_CHARGE):
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x1D, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 5, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 5, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0x14, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 4, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x1E, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 5, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 5, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 0x14, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x42, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 4, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x56, 0x1D, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x54, 5, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x56, 5, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x54, 0x14, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x66, 1, 9);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_SLAM):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x19, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x88, 0x2C, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 9, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 7, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 9, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x7C, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 4, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 4, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x76, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0xA4, 3, 0x17);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_RAJANG_CHARGE_END):
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x1D, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 5, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 5, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 0x14, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0x37, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 8, 3);
        
    case EM_MOTION_ID(0, BLANG_MOT_IDLE):
    default:
        break;
    }
}

/*
 * The family's setup (Em_TypeSetup's entry for the Blangonga kinds):
 * nothing to do.
 */
void EmBlangonga_Setup(Em *em)
{
}

/*
 * Callback slots 0 and 1: creates the family's effect 01 (type 3 for the
 * large kinds, 0 for the Blango and the Conga). The monster comes as an
 * s32.
 */
void EmBlangonga_SpawnEffect01(s32 em)
{
    if (EmBlangonga_IsSmallKind((Em *) em) == 0) {
        Shadow_Spawn((Player *) em, 3);
        return;
    }
    Shadow_Spawn((Player *) em, 0);
}

/*
 * Sets a monster of the family up: the scaled parts (0-2 for the Blangonga
 * kinds, 2-3 for the Congalala kinds); in the quest-less stage
 * (gQuest.stage 0) a fixed start position per slot; the idle motion and
 * full hp from the size (the Blango and the Conga are always run locally);
 * the rage timer and work cleared; every carried item hidden, then the
 * Congalala kinds show the one EmBlangonga_PickCarried picks; eatenKind
 * from the versus team flags; the kind's body colour; the parts.
 */
void EmBlangonga_Init(Em *em)
{
    EmBlangongaWork *w = (EmBlangongaWork *)&em->work;
    u16 part;

    if (em->type == EM_KIND_BLANGONGA) {
        Em_SetPartScaled(em, 0);
        Em_SetPartScaled(em, 1);
        Em_SetPartScaled(em, 2);
    } else if (em->type == EM_KIND_CONGALALA) {
        Em_SetPartScaled(em, 2);
        Em_SetPartScaled(em, 3);
    }
    if (gQuest.stage == 0) {
        em->posX = 2400.0f + 150.0f * (u32) em->slotNo;
        em->posY = 0.0f;
        em->posZ = 3600.0f;
    }
    Em_SetMotionRow(em, BLANG_MOT_IDLE, 0, 0);
    em->stance = EM_STANCE_GROUND;
    em->hpMax = em->hp = Em_ComputeMaxHp(em, 0);
    em->scriptPending = 1;
    em->blind = 0;
    em->unk_InitFlag = 1;
    if (EmBlangonga_IsSmallKind(em) == 1) {
        em->remote = 0;
    }
    w->rageTimer = 0;
    w->unk03A = 0;
    w->senseListed = 0;
    Em_BonePos(em, 0xF, &w->bonePos[0]);
    w->moveCount = 0;
    w->carryHidden[0] = 1;
    w->carryHidden[1] = 1;
    w->carryHidden[2] = 1;
    if (em->type == EM_KIND_CONGALALA) {
        part = EmBlangonga_PickCarried((EmBlangongaPickRow *) em, w->pickSeed);
        if (part != 0xFFFF) {
            EmBlangonga_ShowCarried(em, part);
        }
    }
    if (em->type == EM_KIND_CONGALALA) {
        if (Quest_IsVersus() == 1) {
            if (Versus_TeamHasFlag(em, 3)) {
                w->eatenKind = 2;
            } else if (Versus_TeamHasFlag(em, 4)) {
                w->eatenKind = 1;
            } else if (Versus_TeamHasFlag(em, 5)) {
                w->eatenKind = 3;
            } else {
                w->eatenKind = 0;
            }
        } else {
            w->eatenKind = 0;
        }
    } else {
        w->eatenKind = 0;
    }
    switch (em->type) {
    case EM_KIND_BLANGONGA:
        w->tint[0] = 0x73;
        w->tint[1] = 0xEB;
        w->tint[2] = 0xFF;
        break;
    case EM_KIND_BLANGO:
        w->tint[0] = 0xCD;
        w->tint[1] = 0x91;
        w->tint[2] = 0xC3;
        break;
    case EM_KIND_CONGALALA:
    case EM_KIND_CONGA:
        w->tint[0] = 0x99;
        w->tint[1] = 0x80;
        w->tint[2] = 0x73;
        break;
    default:
        w->tint[0] = 0xFF;
        w->tint[1] = 0xFF;
        w->tint[2] = 0xFF;
        break;
    }
    w->hitSeCooldown = 0;
    Em_InitParts(em);
}

/*
 * Starts a monster: the Congalala kinds first draw pickSeed so that Init
 * and Reinit pick the same carried item; Init; in Congalala's two story
 * quests, before EVENT_FLAG_CONGALALA_SCENE, places it at a fixed point of AREA_JUNGLE_3;
 * then BLANG_IDLE_BURIED for a Blango of variant 1, BLANG_IDLE for the
 * others.
 */
void EmBlangonga_Start(Em *em)
{
    EmBlangongaWork *w = (EmBlangongaWork *)&em->work;

    if (em->type == EM_KIND_CONGALALA) {
        w->pickSeed = Quest_Random();
    }
    EmBlangonga_Init(em);
    if (em->type == EM_KIND_CONGALALA) {
        switch (gQuest.stage) {
        case QUEST_ID_CONGALALA_SCENE_1:
        case QUEST_ID_CONGALALA_SCENE_2:
            if (Save_TestEventFlag(EVENT_FLAG_CONGALALA_SCENE) == 0) {
                em->area = Area_IdForCurrentTime(AREA_JUNGLE_3);
                em->posX = 11870.0f;
                em->posY = 0.0f;
                em->posZ = 10100.0f;
                em->rotY = 0x9000;
            }
            break;
        }
    }
    if (em->type == EM_KIND_BLANGO && em->variant == 1) {
        EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_BURIED, EM_SYNC_LOCAL);
        return;
    }
    EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE, EM_SYNC_LOCAL);
}

/*
 * Callback slot 5: sets the monster up again (Init) without changing its
 * state.
 */
void EmBlangonga_Reinit(Em *em)
{
    EmBlangonga_Init(em);
}

/*
 * Starts state `state` of group `group` (EM_SYNC_* `mode`, passed on to
 * Em_SetStateSync): takes the roar's sensed object off the list, does what
 * the state needs on entry (statuses begun or ended, speeds and stop
 * distances, the move count, the low-hp limp variants below 30% (Blangonga
 * kinds), 20% (Congalala kinds) or 15% (Rajang) of hpMax, the carried items
 * shown again, the part break of BLANG_REACT_PART_BREAK), then sets it. A
 * remote copy ignores the owner's modes.
 */
void EmBlangonga_ChangeState(Em *em, u16 group, u16 state, u8 mode)
{
    EmBlangongaWork *w = (EmBlangongaWork *)&em->work;
    u8 count = 0;
    f32 lowHpRatio;
    u16 area;

    if (w->senseListed != 0) {
        ObjListA70_Remove((EmObj *)((&w->sense)));
        w->senseListed = 0;
    }
    if (em->remote == 0) {
        Em_TargetPlayerInRange(em);
    } else if (mode == EM_SYNC_OWNER || mode == EM_SYNC_OWNER_FLAGGED) {
        return;
    }
    if (EmBlangonga_IsBlangonga(em) == 1) {
        lowHpRatio = 0.3f;
    } else if (EmBlangonga_IsCongalala(em) == 1) {
        lowHpRatio = 0.2f;
    } else {
        lowHpRatio = 0.15f;
    }
    em->hitzoneSet = 0;
    switch (group) {
    case EM_GROUP_IDLE:
        switch (state) {
        case BLANG_IDLE:
            if (mode == EM_SYNC_LOCAL) {
                count = w->moveCount;
            }
            if (em->combat != 0) {
                state = BLANG_IDLE_ALERT;
            }
            break;
        case BLANG_IDLE_REST:
            Em_ResetNotice(em);
            break;
        case BLANG_IDLE_STATUS2_END:
            Em_EndStatus2(em);
            break;
        case BLANG_IDLE_STATUS2_COLLAPSE:
        case BLANG_IDLE_STATUS2_DOWNED:
            Em_BeginStatus2(em);
            break;
        case BLANG_IDLE_STATUS4_SHAKE_OFF:
            Em_EndStatus4(em);
            break;
        case BLANG_IDLE_BURIED:
            em->senseDelay = 5;
            em->stateArmor = 1;
            em->scriptPending = 0;
            break;
        case BLANG_IDLE_STUN_SHAKE_OFF:
            Em_EndStatus5(em);
            break;
        case BLANG_IDLE_REST_END:
            Em_RefillNeed3(em);
            break;
        case BLANG_IDLE_HOLD2_SHAKE_OFF:
            em->holdTimer2 = 0;
            break;
        case BLANG_IDLE_EAT_KIND_A:
        case BLANG_IDLE_EAT_KIND_B:
        case BLANG_IDLE_EAT_KIND_C:
        case BLANG_IDLE_EAT_KIND_D:
        case BLANG_IDLE_EAT_NEED1_B:
            switch (state) {
            case BLANG_IDLE_EAT_KIND_A:
                w->eatenKind = 1;
                break;
            case BLANG_IDLE_EAT_KIND_B:
                w->eatenKind = 2;
                break;
            case BLANG_IDLE_EAT_KIND_C:
                w->eatenKind = 3;
                break;
            case BLANG_IDLE_EAT_KIND_D:
                w->eatenKind = 4;
                break;
            }
            if (em->type == EM_KIND_CONGALALA) {
                EmBlangonga_HideCarried(em, 0);
                EmBlangonga_HideCarried(em, 1);
                EmBlangonga_HideCarried(em, 2);
            }
            break;
        case BLANG_IDLE_EAT:
            Em_StartGoalAction(em, 10000);
            break;
        case BLANG_IDLE_STATUS1_DOWNED:
        case BLANG_IDLE_STATUS1_COLLAPSE:
            Em_BeginStatus1(em);
            break;
        case BLANG_IDLE_STATUS1_END:
            Em_EndStatus1(em);
            break;
        case BLANG_IDLE_ROAR_RAGE:
            Em_StartRage(em);
            break;
        case BLANG_IDLE_ROAR_PENDING_RAGE:
            Em_StartPendingRage(em);
            break;
        }
        break;
    case EM_GROUP_MOVE:
        switch (state) {
        case BLANG_MOVE_WALK:
        case BLANG_MOVE_START_AND_WALK:
            if (em->hp < (s16) (em->hpMax * lowHpRatio)) {
                state = BLANG_MOVE_LIMP;
            }
        case BLANG_MOVE_LIMP:
        walk: /* BLANG_MOVE_ACTION_E at low hp joins the limp here */
            if (em->curGoalKind == 7) {
                Em_SetSpeedScaled(em, 0, 1000.0f, 350.0f);
            } else {
                Em_SetSpeedScaled(em, 0, 1000.0f, 100.0f);
            }
            break;
        case BLANG_MOVE_ACTION_A:
        case BLANG_MOVE_ACTION_B:
        case BLANG_MOVE_ACTION_C:
        case BLANG_MOVE_ACTION_D:
            if (Em_IsSyncFree(em, mode) != 0) {
                w->moveCount++;
            }
            count = w->moveCount;
            break;
        case BLANG_MOVE_TURN_TO_EXIT:
            if (em->remote == 0 && em->curGoalKind != 3) {
                EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE, EM_SYNC_SEND);
                return;
            }
            break;
        case BLANG_MOVE_START_AND_APPROACH:
        case BLANG_MOVE_APPROACH:
            if (em->hp < (s16) (em->hpMax * lowHpRatio)) {
                state = BLANG_MOVE_LIMP_APPROACH;
            }
        case BLANG_MOVE_LIMP_APPROACH:
            if (em->curGoalKind == 0) {
                em->hasGoal = 0;
            } else {
                em->hasGoal = 1;
            }
            break;
        case BLANG_MOVE_ACTION_E:
            if (em->hp < (s16) (em->hpMax * lowHpRatio)) {
                state = BLANG_MOVE_LIMP;
                goto walk;
            }
            break;
        case BLANG_MOVE_TURN:
        case BLANG_MOVE_TURN_QUICK:
        case BLANG_MOVE_TURN_AWAY:
        case BLANG_MOVE_TURN_AWAY_QUICK:
            break;
        }
        break;
    case EM_GROUP_FLY:
        switch (state) {
        case 2:
            if ((EmBlangonga_ScriptQuery(em, BLANG_QUERY_ON_SNOW) & 0xFF) == 0) {
                EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_ALERT_BRIEF, EM_SYNC_SEND);
                return;
            }
            break;
        case 6:
            Em_SetSpeedScaled(em, 0, 1000.0f, 250.0f);
            break;
        case 9:
            Em_TypeSetup(em);
            break;
        case 11:
            Em_TypeSetup(em);
            if (em->remote == 0 && em->curGoalKind != 3) {
                switch (Map_DayId(gScene.map)) {
                case MAP_SNOWY_MOUNTAINS:
                default:
                    area = Area_DayId(AREA_SNOWY_MOUNTAINS_8);
                    em->destArea = area;
                    em->nextArea = area;
                    em->goalNo = area;
                    break;
                case MAP_JUNGLE:
                    area = Area_DayId(AREA_JUNGLE_3);
                    em->destArea = area;
                    em->nextArea = area;
                    em->goalNo = area;
                    break;
                case MAP_SWAMP:
                    area = Area_DayId(AREA_SWAMP_9);
                    em->destArea = area;
                    em->nextArea = area;
                    em->goalNo = area;
                    break;
                case MAP_VOLCANO:
                    area = Area_DayId(AREA_VOLCANO_5);
                    em->destArea = area;
                    em->nextArea = area;
                    em->goalNo = area;
                    break;
                }
                em->goalKind = 3;
                Em_ComputeGoal(em, em->goal);
                EmBlangonga_ChangeState(em, EM_GROUP_MOVE, BLANG_MOVE_TURN_TO_EXIT, EM_SYNC_SEND);
                return;
            }
            break;
        }
        break;
    case EM_GROUP_ATTACK:
        switch (state) {
        case BLANG_ATK_CHARGE:
            Em_SetSpeedScaled(em, 0, 1000.0f, 1100.0f);
            break;
        case BLANG_ATK_RAJANG_CHARGE:
            Em_SetSpeedScaled(em, 0, 1000.0f, 500.0f);
            break;
        case BLANG_ATK_CONGALALA_CHARGE:
            if (em->curGoalKind != 1) {
                Em_SetSpeedScaled(em, 0, 1000.0f, 350.0f);
            } else {
                Em_SetSpeedScaled(em, 0, 1000.0f, -200.0f);
            }
            break;
        }
        break;
    case EM_GROUP_REACT:
        switch (state) {
        case BLANG_REACT_STATUS4:
        case BLANG_REACT_STATUS4_DOWNED:
            Em_BeginStatus4(em);
            break;
        case BLANG_REACT_TRAPPED:
            Em_EndAllStatus(em);
            break;
        case BLANG_REACT_STATUS4_DOWNED_END:
            Em_EndStatus4(em);
            break;
        case BLANG_REACT_STATUS2_DOWNED_END:
            Em_EndStatus2(em);
            break;
        case BLANG_REACT_STATUS1_DOWNED_END:
            Em_EndStatus1(em);
            break;
        case BLANG_REACT_STUN:
        case BLANG_REACT_STUN_B:
        case BLANG_REACT_STUN_DOWNED:
            Em_BeginStatus5(em);
            break;
        case BLANG_REACT_STUN_DOWNED_END:
            Em_EndStatus5(em);
            break;
        case BLANG_REACT_HOLD2:
            Em_EndAllStatus(em);
            break;
        case BLANG_REACT_DROP_CARRIED:
        case BLANG_REACT_DROP_CARRIED_A:
        case BLANG_REACT_DROP_CARRIED_B:
        case BLANG_REACT_DROP_CARRIED_C:
            if (em->type == EM_KIND_CONGALALA) {
                EmBlangonga_HideCarried(em, 0);
                EmBlangonga_HideCarried(em, 1);
                EmBlangonga_HideCarried(em, 2);
            }
            break;
        case BLANG_REACT_PART_BREAK:
            em->partBreakCount++;
            if (em->type == EM_KIND_BLANGONGA) {
                Em_BreakPart(em, 1);
            } else if (em->type == EM_KIND_RAJANG) {
                Em_EndRage(em);
                Em_BreakPart(em, 2);
            }
            break;
        case BLANG_REACT_TRAP_ESCAPE:
            Em_EndAllStatus(em);
            em->guardTimer = 2;
            break;
        case BLANG_REACT_TRAPPED_STRUGGLE:
        case BLANG_REACT_FALL:
        case BLANG_REACT_NONE:
        case BLANG_REACT_FLINCH_ATTACK:
            break;
        }
        break;
    case EM_GROUP_DEFEAT:
        switch (state) {
        case BLANG_DEFEAT_DIE:
            if (EmBlangonga_IsSmallKind(em) == 1 && em->stance == EM_STANCE_AIR) {
                EmBlangonga_ChangeState(em, EM_GROUP_DEFEAT, BLANG_DEFEAT_DIE_FALLING, mode);
                return;
            }
            break;
        case BLANG_DEFEAT_REAPPEAR:
            if (QuestEm_CanAppear(em) == 1) {
                QuestEm_Appear(em);
            }
            break;
        }
        break;
    case EM_GROUP_VERSUS:
        if (state == 0x98) {
            Versus_MarkDefeat(em);
        }
        em->motionSpeed = 1.0f;
        break;
    case EM_GROUP_EVENT:
    case EM_GROUP_EVENT2:
        break;
    }
    w->moveCount = count;
    Em_SetStateSync(em, group, state, mode);
}

/*
 * Ends a state: back to BLANG_IDLE with the motion speed, stance and look
 * reset, the script asked for the next state (scriptPending). Both
 * arguments are unused.
 */
static void EmBlangonga_EndState(Em *em, s32 unused1, s32 unused2)
{
    em->motionSpeed = 1.0f;
    em->stance = EM_STANCE_GROUND;
    em->lookAtTarget = 0;
    em->scriptPending = 1;
    EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE, EM_SYNC_LOCAL);
}

/*
 * Ends a buried state: as EmBlangonga_EndState, but stays under the snow
 * (stateArmor) and goes back to BLANG_LEAP_BURIED.
 */
static void EmBlangonga_EndBuriedState(Em *em)
{
    em->motionSpeed = 1.0f;
    em->stance = EM_STANCE_GROUND;
    em->stateArmor = 1;
    em->noPushTimer = 2;
    em->lookAtTarget = 0;
    em->scriptPending = 1;
    EmBlangonga_ChangeState(em, EM_GROUP_FLY, BLANG_LEAP_BURIED, EM_SYNC_LOCAL);
}

/*
 * Picks the carried item for the current map (MAP_JUNGLE or MAP_SWAMP: the two
 * weight tables of EmBlangonga_TurnData) from `seed`: the value of the row
 * whose running weight passes seed modulo the total; 0xFFFF on other maps.
 * The first argument is unused (its register is the loop pointer: a local
 * of its own gives other registers).
 */
u16 EmBlangonga_PickCarried(EmBlangongaPickRow *row, u16 seed)
{
    u16 sum;
    u16 acc;
    EmBlangongaPickRow *table;
    u16 roll;

    switch (Map_DayId(gScene.map)) {
    case MAP_JUNGLE:
        table = EmBlangonga_TurnData.pick0;
        break;
    case MAP_SWAMP:
        table = EmBlangonga_TurnData.pick1;
        break;
    default:
        return 0xFFFF;
    }
    sum = 0;
    row = table;
    if (table->weight != 0xFFFF) {
        do {
            acc = row->weight;
            sum += acc;
            row++;
        } while (row->weight != 0xFFFF);
    }
    acc = 0;
    roll = seed % sum;
    if (table->weight != 0xFFFF) {
        do {
            acc += *(u16 *) table; /* the weight; read as a u16 the match needs (table->weight compiles differently) */
            if (acc > roll) {
                return table->value;
            }
            table++;
        } while (table->weight != 0xFFFF);
    }
    return 0xFFFF;
}

/* Hides carried item `part` (0-2): sets its carryHidden flag. */
void EmBlangonga_HideCarried(Em *em, u16 part)
{
    EmBlangongaWork *w = (EmBlangongaWork *)&em->work;

    switch (part) {
    case 0:
        w->carryHidden[0] = 1;
        break;
    case 1:
        w->carryHidden[1] = 1;
        break;
    case 2:
        w->carryHidden[2] = 1;
        break;
    }
}

/* Shows carried item `part` (0-2): clears its carryHidden flag. */
void EmBlangonga_ShowCarried(Em *em, u16 part)
{
    EmBlangongaWork *w = (EmBlangongaWork *)&em->work;

    switch (part) {
    case 0:
        w->carryHidden[0] = 0;
        break;
    case 1:
        w->carryHidden[1] = 0;
        break;
    case 2:
        w->carryHidden[2] = 0;
        break;
    }
}

/*
 * Returns 1 when carried item `part` (0-2) is shown; for `part` 6, 1 while
 * part 2 is unbroken (Em_CheckRageByHp and gm_em.c ask that).
 */
s32 EmBlangonga_CarriedShown(Em *em, u16 part)
{
    EmBlangongaWork *w = (EmBlangongaWork *)&em->work;

    switch (part) {
    case 0:
        if (w->carryHidden[0]) {
            return 0;
        }
        break;
    case 1:
        if (w->carryHidden[1]) {
            return 0;
        }
        break;
    case 2:
        if (w->carryHidden[2]) {
            return 0;
        }
        break;
    case 6:
        if (Em_PartBroken(em, 2) == 1) {
            return 0;
        }
        break;
    }
    return 1;
}

/* Returns 1 for the small kinds, the Blango and the Conga. */
s32 EmBlangonga_IsSmallKind(Em *em)
{
    u8 type;

    type = em->type;
    if ((type == 0x3D) || (type == 0x3E)) {
        return 1;
    }
    return 0;
}

/* Returns 1 for Blangonga and the Blango. */
static s32 EmBlangonga_IsBlangonga(Em *em)
{
    u8 type;

    type = em->type;
    if ((type == 0x33) || (type == 0x3D)) {
        return 1;
    }
    return 0;
}

/* Returns 1 for Congalala and the Conga. */
static s32 EmBlangonga_IsCongalala(Em *em)
{
    u8 type;

    type = em->type;
    if ((type == 0x34) || (type == 0x3E)) {
        return 1;
    }
    return 0;
}

/* Returns 1 for Rajang. */
static u64 EmBlangonga_IsRajang(Em *em)
{
    return em->type == EM_KIND_RAJANG;
}

/*
 * Returns 1 in the states a flash doesn't affect (Em_ApplyFlash): while
 * stateArmor, lying in a hold under status 1 or 2, and the trapped,
 * status-4 and stun states of EM_GROUP_REACT.
 */
s32 EmBlangonga_IgnoresFlash(Em *em)
{
    if (em->stateArmor == 1
        || (em->stateGroup == EM_GROUP_IDLE && em->state == BLANG_IDLE_STATUS2_DOWNED)
        || (em->stateGroup == EM_GROUP_IDLE && em->state == BLANG_IDLE_STATUS1_DOWNED)
        || (em->stateGroup == EM_GROUP_REACT && em->state == BLANG_REACT_TRAPPED)
        || (em->stateGroup == EM_GROUP_REACT && em->state == BLANG_REACT_TRAPPED_STRUGGLE)
        || (em->stateGroup == EM_GROUP_REACT && em->state == BLANG_REACT_STATUS4_DOWNED)
        || (em->stateGroup == EM_GROUP_REACT && em->state == BLANG_REACT_STATUS4_DOWNED_END)
        || (em->stateGroup == EM_GROUP_REACT && em->state == BLANG_REACT_STATUS2_DOWNED_END)
        || (em->stateGroup == EM_GROUP_REACT && em->state == BLANG_REACT_STUN_DOWNED)
        || (em->stateGroup == EM_GROUP_REACT && em->state == BLANG_REACT_STUN_DOWNED_END)) {
        return 1;
    }
    return 0;
}

/* Returns 1 while Em.stateArmor is set (buried, or out of reach in a scene). */
s32 EmBlangonga_InStateArmor(Em *em)
{
    return em->stateArmor != 0;
}

/*
 * The shadow's brightness (sub_main's Shadow_Draw): fades from 1 at 2000
 * above the floor to 0 at 3500 during BLANG_LEAP_OUT_OF_AREA and _DROP_IN,
 * 0 during BLANG_LEAP_CROSS_AREA, else 1.
 */
f32 EmBlangonga_ShadowFade(Em *em)
{
    f32 height;
    f32 fade;

    if (em->stateGroup == EM_GROUP_FLY) {
        switch (em->state) {
        case BLANG_LEAP_OUT_OF_AREA:
        case BLANG_LEAP_DROP_IN:
            height = em->posY - em->floorY;
            if (height < 2000.0f) {
                return 1.0f;
            }
            if (height < 3500.0f) {
                fade = 1.0f - ((height - 2000.0f) / 1500.0f);
            } else {
                fade = 0.0f;
            }
            return fade;
        case BLANG_LEAP_CROSS_AREA:
            return 0.0f;
        }
    }
    return 1.0f;
}

/*
 * Counts Em.rage's timer (EmBlangongaWork.rageTimer) up to its limit while
 * enraged and down to 0 otherwise (the limit: 15 frames, Congalala 60
 * enraged or 150); Rajang's rage begins with stage object 0x18.
 */
void EmBlangonga_UpdateRageTimer(Em *em)
{
    EmBlangongaWork *w = (EmBlangongaWork *)&em->work;
    s16 lim;

    switch (em->type) {
    case EM_KIND_RAJANG:
    default:
        lim = 0xF;
        break;
    case EM_KIND_CONGALALA:
        if (em->rage != 0) {
            lim = 0x3C;
        } else {
            lim = 0x96;
        }
        break;
    }
    if (em->rage != 0) {
        switch (em->rageStep) {
        case 0:
            if (EmBlangonga_IsRajang(em) == 1) {
                MonsterFx_CreateOnEm(em, 0x18, 1.0f);
            }
            em->rageStep++;
        case 1:
            if (++w->rageTimer > lim) {
                w->rageTimer = lim;
                em->rageStep++;
            }
            break;
        case 2:
            w->rageTimer = lim;
            break;
        }
    } else {
        em->rageStep = 0;
        if (--w->rageTimer < 0) {
            w->rageTimer = 0;
        }
    }
}

/*
 * For gm_em.c's Congalala meshes: 1 while Congalala holds its food (frames
 * 36-382 of BLANG_MOT_EAT, or in its story scene once it has picked it up),
 * 2 in the scene before that, else 0.
 */
s32 EmBlangonga_EatPropMode(Em *em)
{
    EmBlangongaWork *w = (EmBlangongaWork *)&em->work;

    if (em->stateGroup == EM_GROUP_EVENT && em->state == BLANG_EVENT_CONGALALA_SCENE) {
        return w->sceneEating ? 1 : 2;
    }
    switch (em->motionId[0]) {
    case EM_MOTION_ID(0, BLANG_MOT_EAT):
        if (EmMotion_InFrameRange(em, 0, 36.0f, 382.0f)) {
            return 1;
        }
        break;
    }
    return 0;
}

/*
 * Steps the body colour (EmBlangongaWork.tint) toward the kind's target:
 * the Blangonga kinds' rage colour, or Blangonga's own colour above 30% hp
 * and a second one below; Congalala's pulsing rage colour or its calm one
 * (at once when the rage timer is full). 12 per frame (Congalala calming:
 * 6). Rajang and the Conga keep theirs.
 */
void EmBlangonga_UpdateTint(Em *em)
{
    EmBlangongaWork *w = (EmBlangongaWork *)&em->work;
    u8 col[3];
    s16 i;
    s16 diff;
    u8 *target;
    u8 *channel;
    u8 step;
    f32 pulse;
    u8 *target2;
    u8 *channel2;
    s16 j;
    s16 diff2;

    switch (em->type) {
    case EM_KIND_BLANGONGA:
    case EM_KIND_BLANGO:
        if (em->rage != 0) {
            col[0] = 0xFF;
            col[1] = 0x64;
            col[2] = 0x5A;
        } else if (em->type == EM_KIND_BLANGONGA) {
            if (em->hp > (s16) (0.3f * em->hpMax)) {
                col[0] = 0x73;
                col[1] = 0xEB;
                col[2] = 0xFF;
            } else {
                col[0] = 0xCD;
                col[1] = 0x91;
                col[2] = 0xC3;
            }
        } else {
            col[0] = 0xCD;
            col[1] = 0x91;
            col[2] = 0xC3;
        }
        for (i = 0, target = col; i < 3; i++, target++) {
            channel = &w->tint[i];
            diff = *target - *channel;
            if (Math_Absf(diff) <= 12.0f) {
                *channel = *target;
            } else if (diff < 0) {
                *channel -= 12;
            } else {
                *channel += 12;
            }
        }
        break;
    case EM_KIND_CONGALALA:
        if (em->rage != 0) {
            pulse = 0.5f * (1.0f + Math_Sin((6.2831855f * (f32) (gScene.frame << 10)) / 65536.0f));
            col[0] = 255.0f + -100.0f * pulse;
            col[1] = 40.0f + 10.0f * pulse;
            col[2] = 10.0f + 5.0f * pulse;
        } else {
            col[0] = 0x99;
            col[1] = 0x80;
            col[2] = 0x73;
        }
        for (j = 0, target2 = col; j < 3; j++, target2++) {
            if (EmBlangonga_RageTimerFull(em) == 1) {
                w->tint[j] = *target2;
            } else {
                channel2 = &w->tint[j];
                diff2 = *target2 - *channel2;
                if (em->rage != 0) {
                    step = 12;
                } else {
                    step = 6;
                }
                if (Math_Absf(diff2) <= step) {
                    *channel2 = *target2;
                } else if (diff2 < 0) {
                    *channel2 -= step;
                } else {
                    *channel2 += step;
                }
            }
        }
        break;
    default:
        return;
    }
}

/*
 * Returns 1 when the rage timer has reached its limit
 * (EmBlangonga_UpdateRageTimer's); also read by sub_main for Rajang's mesh.
 */
s32 EmBlangonga_RageTimerFull(Em *em)
{
    EmBlangongaWork *w = (EmBlangongaWork *)&em->work;
    s16 k;

    switch (em->type) {
    case EM_KIND_RAJANG:
    default:
        k = 0xF;
        break;
    case EM_KIND_CONGALALA:
        if (em->rage != 0) {
            k = 0x3C;
        } else {
            k = 0x96;
        }
        break;
    }
    return w->rageTimer == k;
}

/*
 * Creates the periodic effect 06 of a lying monster at scale 1.6: at bone 1
 * for `kind` 1 (Congalala), at bone 0x12 for the others.
 */
void EmBlangonga_SpawnPeriodicEffect06(Em *em, s32 kind)
{
    f32 pos[3];
    s16 bone;

    switch ((u8) kind) {
    default:
        pos[1] = -55.0f;
        bone = 0x12;
        pos[0] = 0.0f;
        pos[2] = 50.0f;
        break;
    case 1:
        bone = 1;
        pos[1] = -30.0f;
        pos[0] = 0.0f;
        pos[2] = 100.0f;
        break;
    }
    Em_SpawnLyingPuffs(em, bone, pos, 1.6f);
}

/*
 * Returns 1 when a monster under stateArmor in the player's area is off the
 * snow (ScriptQuery 1 is 0): EmBlangonga_Update then ends its state.
 */
s32 EmBlangonga_ShouldEndState(Em *em)
{
    if (!((s32)Player_InCurrentArea(em) & 0xFF)) {
        return 0;
    }
    if ((em->stateArmor == 1) && !(EmBlangonga_ScriptQuery(em, BLANG_QUERY_ON_SNOW) & 0xFF)) {
        return 1;
    }
    return 0;
}

/*
 * Starts the family's movement curves (EmBlangonga_VelYCurves and
 * SpeedCurves) for `kind`: 0 the vertical speed from frame 0, 1 from frame
 * 24, 2 both speeds; the velocity is cleared.
 */
void EmBlangonga_StartCurves(Em *em, u8 kind)
{
    EmBlangongaWork *w = (EmBlangongaWork *)&em->work;

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
        w->curveFrame = 0x18;
        break;
    case 2:
        w->curveVelY = 1;
        w->curveSpeed = 1;
        w->curveFrame = 0;
        break;
    }
    Player_ClearVelocity(em);
}

extern EmCurveKey *EmBlangonga_VelYCurves[];

/*
 * Sets Em.velY from the curve keys around the curve frame (curve
 * curveKind); at the table's end applies gravity or stops, and returns 2.
 */
static u8 EmBlangonga_CurveVelY(Em *em, EmWork *work)
{
    EmBlangongaWork *w = (EmBlangongaWork *) work;
    EmCurveKey *key;
    f32 frame;
    f32 keyFrame;
    s32 i;
    EmCurveKey *tbl;
    u8 ret;

    tbl = EmBlangonga_VelYCurves[((EmBlangongaWork *)work)->curveKind];
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

extern EmCurveKey *EmBlangonga_SpeedCurves[];

/*
 * Sets Em.velZ from the speed curve keys around the curve frame; returns 4
 * at the table's end.
 */
static u8 EmBlangonga_CurveSpeed(Em *em, EmWork *work)
{
    EmBlangongaWork *w = (EmBlangongaWork *) work;
    EmCurveKey *key;
    f32 frame;
    f32 keyFrame;
    s32 i;
    EmCurveKey *tbl;
    u8 ret;

    tbl = EmBlangonga_SpeedCurves[((EmBlangongaWork *)work)->curveKind];
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
 * Runs the started curves, advances the curve frame by the motion's step
 * and, in the air, moves along rotY (Em_MoveRotatedAccel). Returns the
 * ended curves' bits (2, 4).
 */
u8 EmBlangonga_UpdateCurves(Em *em)
{
    EmBlangongaWork *w = (EmBlangongaWork *)&em->work;
    u8 ret = 0;

    if (w->curveVelY != 0) {
        ret |= EmBlangonga_CurveVelY(em, (EmWork *) w);
    }
    if (w->curveSpeed != 0) {
        ret |= EmBlangonga_CurveSpeed(em, (EmWork *) w);
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

/* The family's entry points per area: its entry of gm_sub's table of every family's area entry points. */
extern EmAreaPoints *EmBlangonga_AreaEntries;

/*
 * Moves the monster into Em.nextArea: from the area's entry list (gm_sub's
 * per-area table) it picks the entry point and the point to face by the
 * area it came from, or uses the start position; it starts 5000 above the
 * floor.
 */
void EmBlangonga_EnterNewArea(Em *em)
{
    f32 *pts;
    s16 entryNo;
    s16 faceNo;

    em->prevArea = em->area;
    em->area = em->nextArea;
    pts = ((EmAreaPoints *)Em_FindAreaEntry(em, (struct EmIdEntry *)EmBlangonga_AreaEntries, em->area))->pts;
    if (pts == NULL) {
        Vec_Copy(&em->posX, &gStartPos[em->area * 3]);
        em->rotY = 0;
    } else {
        switch (Area_DayId(em->area)) {
        case AREA_SNOWY_MOUNTAINS_3:
        default:
        case AREA_SNOWY_MOUNTAINS_4:
        case AREA_SNOWY_MOUNTAINS_5:
        case AREA_SNOWY_MOUNTAINS_6:
        case AREA_SNOWY_MOUNTAINS_7:
        case AREA_SNOWY_MOUNTAINS_8:
        case AREA_JUNGLE_9:
        case AREA_VOLCANO_1:
        case AREA_VOLCANO_4:
        case AREA_VOLCANO_5:
        case AREA_VOLCANO_7:
            entryNo = 0;
            faceNo = 1;
            break;
        case AREA_JUNGLE_1:
            switch (Area_DayId(em->prevArea)) {
            case AREA_JUNGLE_2:
                entryNo = 5;
                faceNo = 4;
                break;
            default:
                entryNo = 0;
                faceNo = 1;
                break;
            }
            break;
        case AREA_JUNGLE_2:
            switch (Area_DayId(em->prevArea)) {
            case AREA_JUNGLE_3:
                entryNo = 4;
                faceNo = 3;
                break;
            default:
                entryNo = 0;
                faceNo = 1;
                break;
            }
            break;
        case AREA_JUNGLE_3:
            switch (Area_DayId(em->prevArea)) {
            case AREA_JUNGLE_2:
            case AREA_JUNGLE_6:
                entryNo = 4;
                faceNo = 3;
                break;
            default:
                entryNo = 0;
                faceNo = 1;
                break;
            }
            break;
        case AREA_JUNGLE_6:
            switch (Area_DayId(em->prevArea)) {
            case AREA_JUNGLE_1:
            case AREA_JUNGLE_9:
                entryNo = 8;
                faceNo = 7;
                break;
            default:
                entryNo = 0;
                faceNo = 1;
                break;
            }
            break;
        case AREA_JUNGLE_10:
            entryNo = 4;
            faceNo = 3;
            break;
        case AREA_SWAMP_2:
            entryNo = 0;
            faceNo = 1;
            break;
        case AREA_SWAMP_5:
            switch (Area_DayId(em->prevArea)) {
            case AREA_SWAMP_6:
                entryNo = 5;
                faceNo = 4;
                break;
            default:
                entryNo = 0;
                faceNo = 1;
                break;
            }
            break;
        case AREA_SWAMP_6:
            switch (Area_DayId(em->prevArea)) {
            case AREA_SWAMP_7:
                entryNo = 6;
                faceNo = 5;
                break;
            default:
                entryNo = 0;
                faceNo = 1;
                break;
            }
            break;
        case AREA_SWAMP_7:
            switch (Area_DayId(em->prevArea)) {
            case AREA_SWAMP_9:
                entryNo = 2;
                faceNo = 1;
                break;
            default:
                entryNo = 0;
                faceNo = 1;
                break;
            }
            break;
        case AREA_SWAMP_8:
            entryNo = 0;
            faceNo = 1;
            break;
        case AREA_SWAMP_9:
            switch (Area_DayId(em->prevArea)) {
            case AREA_SWAMP_2:
                entryNo = 4;
                faceNo = 3;
                break;
            case AREA_SWAMP_8:
                entryNo = 6;
                faceNo = 5;
                break;
            default:
                entryNo = 0;
                faceNo = 1;
                break;
            }
            break;
        case AREA_VOLCANO_6:
            switch (Area_DayId(em->prevArea)) {
            case AREA_VOLCANO_5:
                entryNo = 4;
                faceNo = 3;
                break;
            case AREA_VOLCANO_7:
            default:
                entryNo = 0;
                faceNo = 1;
                break;
            }
            break;
        }
        Vec_Copy(&em->posX, &pts[entryNo * 3]);
        em->rotY = Math_AngleXZ(&em->posX, &pts[faceNo * 3]);
    }
    em->posY += 5000.0f;
    Vec_Copy(&em->prevPosX, &em->posX);
}

/* BLANG_IDLE_NONE: does nothing. */
void EmBlangonga_IdleNone(void)
{
}

/*
 * BLANG_IDLE: plays BLANG_MOT_IDLE once, then asks the script for the next
 * state.
 */
void EmBlangonga_Idle(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_IDLE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->scriptPending = 1;
        }
        break;
    }
}

/*
 * BLANG_IDLE_REST: lies down and lies for 5400 frames, blind and without
 * flinching, regaining hp (1 every 6 frames up to 40% of hpMax; the
 * Congalala kinds every 8 frames and Rajang every 6, up to 50%) with the
 * periodic effect; then BLANG_IDLE_REST_END.
 */
void EmBlangonga_Rest(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_LIE_DOWN, 0, 0);
        em->timer = 0x1518;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_LIE, 0, 0);
        }
        break;
    case 2:
        if (EmBlangonga_IsBlangonga(em) == 1) {
            Em_RegenHp(em, 1, (s32) (0.4f * (f32) em->hpMax), 6);
        } else if (EmBlangonga_IsCongalala(em) == 1) {
            Em_RegenHp(em, 1, (s32) (0.5f * (f32) em->hpMax), 8);
        } else {
            Em_RegenHp(em, 1, (s32) (0.5f * (f32) em->hpMax), 6);
        }
        EmBlangonga_SpawnPeriodicEffect06(em, 0);
        if (--em->timer <= 0) {
            em->step++;
            Em_ClearLowHpNeed(em);
            EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_REST_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_REST_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * BLANG_IDLE_STATUS2_END, _STATUS1_END and _REST_END: gets up (blind
 * meanwhile) and ends the state.
 */
void EmBlangonga_GetUp(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_GET_UP, 0, 0);
        em->blind = 2;
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_IDLE_STATUS2_COLLAPSE (variant 0) and _STATUS1_COLLAPSE (1): falls
 * over and lies, blind and without flinching, until the status timer runs
 * out, then the status's _END state.
 */
void EmBlangonga_StatusCollapse(Em *em, EmWork *work, s32 variant)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_COLLAPSE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_LIE, 0, 0);
        }
        break;
    case 2:
        EmBlangonga_SpawnPeriodicEffect06(em, 0);
        if (--em->timer <= 0) {
            em->step++;
            if (!(variant & 0xFF)) {
                EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_STATUS2_END, EM_SYNC_OWNER_FLAGGED);
                break;
            }
            EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_STATUS1_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 3:
        if (em->remote == 0) {
            if (!(variant & 0xFF)) {
                EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_STATUS2_END, EM_SYNC_OWNER_FLAGGED);
                break;
            }
            EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_STATUS1_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/* BLANG_IDLE_ALERT: plays BLANG_MOT_ALERT_IDLE once and ends the state. */
void EmBlangonga_AlertIdle(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_ALERT_IDLE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * The roars: BLANG_IDLE_ROAR, _ROAR_SIGNAL, _ROAR_RAGE and
 * _ROAR_PENDING_RAGE (variant 0), _ROAR_CALL (1: at frame 120 adds the
 * sensed object of kind 3 other monsters react to, until the first part
 * break), _ROAR_RAISE_RAGE (2: at frame 50 raises the rage by hp and cuts a
 * flash's blindness short).
 */
void EmBlangonga_Roar(Em *em, EmWork *work, s32 variant)
{
    s32 k;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, BLANG_MOT_ROAR, 0, 0);
        break;
    case 1:
        k = variant & 0xFF;
        switch (k) {
        case 0:
            break;
        case 1:
            if (((s8) em->partBreakCount <= 0) && (EmMotion_PassedFrameScaled(em, 120.0f, 0U) != 0)) {
                Vec_Set((f32 *) &work->moveAngBX, em->posX, em->posY, em->posZ);
                work->moveAngCX = 0x453B8000;
                ((EmBlangongaWork *)work)->sense.area = (s16) *(u8 *)&em->area;
                ((EmBlangongaWork *)work)->sense.kind = 3;
                ObjListA70_Add((EmObj *)&work->moveAngBX);
                ((EmBlangongaWork *)work)->senseListed = 1;
            }
            break;
        case 2:
            if (EmMotion_PassedFrameScaled(em, 50.0f, 0U) != 0) {
                em->noReact = 1;
                Em_CheckRageByHp(em, (s32) em->rageThreshold);
                if (em->blindTimer > 0) {
                    em->blindTimer = 1;
                }
            }
            break;
        }
        if (em->motion[0].playing == 0) {
            em->noReact = 0;
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_IDLE_TURN_ABOUT (variant 0) and _B (1, the other way): turns half
 * round over frames 102-202 and back over 402-452, then ends the state.
 */
void EmBlangonga_TurnAbout(Em *em, EmWork *work, s32 variant)
{
    u16 turn = 0;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        switch (variant & 0xFF) {
        case 0:
        default:
            Em_SetMotion(em, BLANG_MOT_TURN_ABOUT, 0, 0);
            break;
        case 1:
            Em_SetMotion(em, BLANG_MOT_TURN_ABOUT_B, 0, 0);
            break;
        }
        break;
    case 1:
        if (EmMotion_InFrameRange(em, 0, 102.0f, 202.0f)) {
            turn = -327.68f * em->motionSpeed;
        } else if (EmMotion_InFrameRange(em, 0, 402.0f, 452.0f)) {
            turn = 655.36f * em->motionSpeed;
        }
        switch (variant & 0xFF) {
        case 1:
            turn = -turn;
            break;
        }
        em->rotY += turn;
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_IDLE_STATUS4_SHAKE_OFF, _STUN_SHAKE_OFF and _HOLD2_SHAKE_OFF: plays
 * BLANG_MOT_SHAKE_OFF once and ends the state.
 */
void EmBlangonga_ShakeOff(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, BLANG_MOT_SHAKE_OFF, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_IDLE_EAT_NEED2: eats without flinching for 300 frames, filling need
 * 2, then regains 5% of hpMax and clears need 2's state.
 */
void EmBlangonga_EatNeed2(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_NO_FLINCH);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, BLANG_MOT_EAT_NEED2, 0, 0);
        em->timer = 0x12C;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_EAT_NEED2_END, 0, 0);
            Em_AddHp(em, (s32) (0.05f * (f32) em->hpMax));
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_ClearNeed2State(em);
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
    Em_AddNeed2(em, 0xDE);
}

/*
 * BLANG_IDLE_EAT_NEED1, _EAT_NEED1_B (variant 0) and _EAT_KIND_A..D (1):
 * eats without flinching, turning in frames 4-26 and back in 370-418, then
 * regains 5% (variant 1: 1%) of hpMax, refills need 1 and clears its state.
 */
void EmBlangonga_EatNeed1(Em *em, EmWork *work, s32 variant)
{
    u16 turn = 0;

    Em_SetHitReactMode(em, EM_HIT_REACT_NO_FLINCH);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_EAT, 0, 0);
        em->timer = 0xF0;
        break;
    case 1:
        if (EmMotion_InFrameRange(em, 0, 4.0f, 26.0f)) {
            turn = 1638.4f * em->motionSpeed;
        } else if (EmMotion_InFrameRange(em, 0, 370.0f, 418.0f)) {
            turn = -712.34784f * em->motionSpeed;
        }
        em->rotY += turn;
        if (em->motion[0].playing == 0) {
            em->step++;
            if ((variant & 0xFF) == 0) {
                Em_AddHp(em, 0.05f * em->hpMax);
            } else {
                Em_AddHp(em, 0.01f * em->hpMax);
            }
            Em_AddNeed1(em, em->need1Max);
            Em_ClearNeed1State(em);
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_IDLE_CLEAR_PAINT: plays its motion and clears the paint mark
 * (Em.painted, paintTimer) when it ends.
 */
void EmBlangonga_ClearPaint(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_IDLE_ACTION_B_END, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->painted = 0;
            em->paintTimer = 0;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_IDLE_LEAVE_TRAP: climbs out of the trap (the Blangonga kinds and
 * Rajang with BLANG_MOT_EMERGE from frame 14, the Congalala kinds with
 * their own motion), then ends the state.
 */
void EmBlangonga_LeaveTrap(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        switch (em->type) {
        case EM_KIND_BLANGONGA:
        default:
        case EM_KIND_BLANGO:
            Em_SetMotion(em, BLANG_MOT_EMERGE, 6, 0xE);
            break;
        case EM_KIND_CONGALALA:
        case EM_KIND_CONGA:
            Em_SetMotion(em, BLANG_MOT_CONGALALA_CLIMB_OUT, 0, 0);
            break;
        }
        em->stance = EM_STANCE_GROUND;
        em->noReact = 1;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_IDLE_EAT_START (variant 0, BLANG_MOT_EAT to frame 300) and
 * _EAT_START_B (1, Congalala's motion to frame 200): starts eating, then
 * BLANG_IDLE_EAT while the motion plays on.
 */
void EmBlangonga_EatStart(Em *em, EmWork *work, s32 variant)
{
    f32 frame;
    s32 k;

    k = variant & 0xFF;
    if (k == 0) {
        Em_SetHitReactMode(em, EM_HIT_REACT_NO_FLINCH);
    }
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        if (k == 0) {
            Em_SetMotion(em, BLANG_MOT_EAT, 0, 0);
            break;
        }
        Em_SetMotion(em, BLANG_MOT_CONGALALA_EAT, 0, 0);
        break;
    case 1:
        if (k == 0) {
            frame = 300.0f;
        } else {
            frame = 200.0f;
        }
        if (EmMotion_PassedFrameScaled(em, frame, 0U) != 0) {
            em->step++;
            EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_EAT, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_EAT, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/* BLANG_IDLE_ACTION_A: plays BLANG_MOT_BACK_STRIKE once and ends the state. */
void EmBlangonga_IdleActionA(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_BACK_STRIKE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_IDLE_ACTION_B: plays BLANG_MOT_IDLE_ACTION_B (a hit and stage
 * object 0x0F at frame 128), then its end, and ends the state.
 */
void EmBlangonga_IdleActionB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_IDLE_ACTION_B, 0, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 128.0f, 0U) != 0) {
            ElementBurst_CreateOnEm(em, 5, 1, 0.4f * em->scaleX);
            BlangongaAttack_Create(em, 0x22);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_IDLE_ACTION_B_END, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_IDLE_ALERT_BRIEF: BLANG_MOT_ALERT_IDLE for 10 frames, then ends the
 * state.
 */
void EmBlangonga_AlertBrief(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, BLANG_MOT_ALERT_IDLE, 0, 0);
        em->timer = 0xA;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_IDLE_STATUS2_DOWNED (variant 0) and _STATUS1_DOWNED (1): lies in
 * the hold (holdTimer kept at 5), blind and without flinching, with the
 * periodic effect and a sound every 135 frames, until the status timer runs
 * out; then BLANG_REACT_STATUS2_DOWNED_END or _STATUS1_DOWNED_END.
 * Blangonga first comes up out of the pit (BLANG_MOT_EMERGE to frame 14).
 */
void EmBlangonga_StatusDowned(Em *em, EmWork *work, s32 variant)
{
    u16 motion;

    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->holdTimer = 5;
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        if ((em->type == EM_KIND_BLANGONGA) && (motion = em->motionId[0], (motion != EM_MOTION_ID(0, BLANG_MOT_EMERGE))) && (motion != EM_MOTION_ID(0, BLANG_MOT_STRUGGLE)) && (motion != EM_MOTION_ID(0, BLANG_MOT_DOWNED))) {
            Em_SetMotion(em, BLANG_MOT_EMERGE, 0, 0);
            em->stateArmor = 1;
            em->noPushTimer = 5;
        } else {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_DOWNED, 0, 0);
        }
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].blend != 0) {
            em->stateArmor = 1;
            em->noPushTimer = 5;
        }
        if (EmMotion_ReachedFrame(em, 0, 14.0f) != 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_DOWNED, 0, 0);
        }
        break;
    case 2:
        if (((s32) em->timer % 135) == 0) {
            Em_PlaySeAtBone(em, 0x52, 0x13);
        }
        if (em->type == EM_KIND_CONGALALA) {
            EmBlangonga_SpawnPeriodicEffect06(em, 1);
        } else {
            EmBlangonga_SpawnPeriodicEffect06(em, 0);
        }
        if (--em->timer <= 0) {
            em->step++;
            if (!(variant & 0xFF)) {
                EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_STATUS2_DOWNED_END, EM_SYNC_OWNER);
                break;
            }
            EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_STATUS1_DOWNED_END, EM_SYNC_OWNER);
        }
        break;
    case 3:
        if (em->type == EM_KIND_CONGALALA) {
            EmBlangonga_SpawnPeriodicEffect06(em, 1);
        } else {
            EmBlangonga_SpawnPeriodicEffect06(em, 0);
        }
        if (em->remote == 0) {
            if (!(variant & 0xFF)) {
                EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_STATUS2_DOWNED_END, EM_SYNC_OWNER);
                break;
            }
            EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_STATUS1_DOWNED_END, EM_SYNC_OWNER);
        }
        break;
    }
}

/*
 * BLANG_IDLE_ROAR_B: roars (Blangonga with BLANG_MOT_ROAR_B, the others
 * BLANG_MOT_ROAR) and ends the state.
 */
void EmBlangonga_RoarB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        if (em->type == EM_KIND_BLANGONGA) {
            Em_SetMotion(em, BLANG_MOT_ROAR_B, 0, 0);
            break;
        }
        Em_SetMotion(em, BLANG_MOT_ROAR, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_IDLE_ACTION_C: plays BLANG_MOT_IDLE_ACTION_C once and ends the
 * state.
 */
void EmBlangonga_IdleActionC(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, BLANG_MOT_IDLE_ACTION_C, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_IDLE_BURIED: waits under the snow, unhittable, unpushable, sensing
 * nothing and under stateArmor, until the script moves it on.
 */
void EmBlangonga_WaitBuried(Em *em)
{
    em->senseDelay = 5;
    em->noPushTimer = 5;
    em->noHitTimer = 5;
    em->stateArmor = 1;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotionRow(em, BLANG_MOT_BURIED, 0, 0);
        break;
    case 1:
        break;
    }
}

/*
 * BLANG_IDLE_LEAVE_MINE_POINT: plays BLANG_MOT_IDLE_ACTION_B and at frame
 * 184 leaves a timed mine point where it stands, then ends the state.
 */
void EmBlangonga_LeaveMinePoint(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_IDLE_ACTION_B, 0, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 184.0f, 0U) != 0) {
            EmMine_CreateTimed0(em, &em->posX);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_IDLE_EAT: selects sight row 0 and reaction set 0, waits for the
 * eating motion to end, and ends the state.
 */
void EmBlangonga_Eat(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SelectSightRow(em, 0);
        Em_LoadReactionSet(em, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * Runs the current EM_GROUP_IDLE state, passing the shared functions their
 * variant.
 */
void EmBlangonga_RunIdleState(Em *em, EmWork *work)
{
    switch (em->state) {
    case BLANG_IDLE_NONE:
        EmBlangonga_IdleNone();
        break;
    case BLANG_IDLE:
        EmBlangonga_Idle(em);
        break;
    case BLANG_IDLE_REST:
        EmBlangonga_Rest(em);
        break;
    case BLANG_IDLE_STATUS2_END:
        EmBlangonga_GetUp(em);
        break;
    case BLANG_IDLE_STATUS2_COLLAPSE:
        EmBlangonga_StatusCollapse(em, work, 0);
        break;
    case BLANG_IDLE_ALERT:
        EmBlangonga_AlertIdle(em);
        break;
    case BLANG_IDLE_ROAR:
        EmBlangonga_Roar(em, work, 0);
        break;
    case BLANG_IDLE_TURN_ABOUT:
        EmBlangonga_TurnAbout(em, work, 0);
        break;
    case BLANG_IDLE_TURN_ABOUT_B:
        EmBlangonga_TurnAbout(em, work, 1);
        break;
    case BLANG_IDLE_STATUS4_SHAKE_OFF:
        EmBlangonga_ShakeOff(em);
        break;
    case BLANG_IDLE_EAT_NEED2:
        EmBlangonga_EatNeed2(em);
        break;
    case BLANG_IDLE_EAT_NEED1:
        EmBlangonga_EatNeed1(em, work, 0);
        break;
    case BLANG_IDLE_CLEAR_PAINT:
        EmBlangonga_ClearPaint(em);
        break;
    case BLANG_IDLE_LEAVE_TRAP:
        EmBlangonga_LeaveTrap(em);
        break;
    case BLANG_IDLE_EAT_START_B:
        EmBlangonga_EatStart(em, work, 1);
        break;
    case BLANG_IDLE_ACTION_A:
        EmBlangonga_IdleActionA(em);
        break;
    case BLANG_IDLE_ACTION_B:
        EmBlangonga_IdleActionB(em);
        break;
    case BLANG_IDLE_ALERT_BRIEF:
        EmBlangonga_AlertBrief(em);
        break;
    case BLANG_IDLE_STATUS2_DOWNED:
        EmBlangonga_StatusDowned(em, work, 0);
        break;
    case BLANG_IDLE_ROAR_CALL:
        EmBlangonga_Roar(em, work, 1);
        break;
    case BLANG_IDLE_ROAR_RAISE_RAGE:
        EmBlangonga_Roar(em, work, 2);
        break;
    case BLANG_IDLE_ROAR_B:
        EmBlangonga_RoarB(em);
        break;
    case BLANG_IDLE_ACTION_C:
        EmBlangonga_IdleActionC(em);
        break;
    case BLANG_IDLE_BURIED:
        EmBlangonga_WaitBuried(em);
        break;
    case BLANG_IDLE_STUN_SHAKE_OFF:
        EmBlangonga_ShakeOff(em);
        break;
    case BLANG_IDLE_REST_END:
        EmBlangonga_GetUp(em);
        break;
    case BLANG_IDLE_HOLD2_SHAKE_OFF:
        EmBlangonga_ShakeOff(em);
        break;
    case BLANG_IDLE_EAT_KIND_A:
        EmBlangonga_EatNeed1(em, work, 1);
        break;
    case BLANG_IDLE_EAT_KIND_B:
        EmBlangonga_EatNeed1(em, work, 1);
        break;
    case BLANG_IDLE_EAT_KIND_C:
        EmBlangonga_EatNeed1(em, work, 1);
        break;
    case BLANG_IDLE_EAT_KIND_D:
        EmBlangonga_EatNeed1(em, work, 1);
        break;
    case BLANG_IDLE_LEAVE_MINE_POINT:
        EmBlangonga_LeaveMinePoint(em);
        break;
    case BLANG_IDLE_EAT_NEED1_B:
        EmBlangonga_EatNeed1(em, work, 0);
        break;
    case BLANG_IDLE_EAT:
        EmBlangonga_Eat(em);
        break;
    case BLANG_IDLE_ROAR_SIGNAL:
        EmBlangonga_Roar(em, work, 0);
        break;
    case BLANG_IDLE_STATUS1_DOWNED:
        EmBlangonga_StatusDowned(em, work, 1);
        break;
    case BLANG_IDLE_STATUS1_COLLAPSE:
        EmBlangonga_StatusCollapse(em, work, 1);
        break;
    case BLANG_IDLE_STATUS1_END:
        EmBlangonga_GetUp(em);
        break;
    case BLANG_IDLE_EAT_START:
        EmBlangonga_EatStart(em, work, 0);
        break;
    case BLANG_IDLE_ROAR_RAGE:
        EmBlangonga_Roar(em, work, 0);
        break;
    case BLANG_IDLE_ROAR_PENDING_RAGE:
        EmBlangonga_Roar(em, work, 0);
        break;
    }
}

/*
 * BLANG_MOVE_TURN (variant 0: to the target) and _TURN_AWAY (1): turns with
 * the slow turn parameters until the turn is done, then ends the state.
 */
void EmBlangonga_Turn(Em *em, s32 unused, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        switch (variant & 0xFF) {
        case 0:
        default:
            Em_StartTurnToTarget(em, &EmBlangonga_TurnData.turn[0]);
            break;
        case 1:
            Em_StartTurnAway(em, &EmBlangonga_TurnData.turn[0]);
            break;
        }
        break;
    case 1:
        if (Em_TurnStep(em, &EmBlangonga_TurnData.turn[0]) == 1) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_MOVE_WALK (variant 0; the Blango walks faster) and _LIMP (1): steps
 * to the goal (Em_StepToGoal), then ends the state.
 */
void EmBlangonga_WalkToGoal(Em *em, s32 unused, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        switch (variant & 0xFF) {
        case 0:
        default:
            if (em->type == EM_KIND_BLANGO) {
                em->speedBonus = 0x19;
            }
            Em_SetMotion(em, BLANG_MOT_WALK, 0, 0);
            break;
        case 1:
            Em_SetMotion(em, BLANG_MOT_LIMP, 0, 0);
            break;
        }
        break;
    case 1:
        if (Em_StepToGoal(em, 0x80, 0) == 1) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_MOVE_TURN_QUICK (variant 0), _TURN_TO_EXIT (1) and _TURN_AWAY_QUICK
 * (2): turns with the quick turn parameters and ends the state;
 * _TURN_TO_EXIT first takes the area's exit point as its goal and then
 * leaps out (BLANG_LEAP_OUT_OF_AREA).
 */
void EmBlangonga_TurnQuick(Em *em, s32 unused, s32 variant)
{
    s32 k;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        k = variant & 0xFF;
        em->lookAtTarget = 0;
        switch (k) {
        case 1:
            Em_AreaExitPoint(em, (f32 *) em->goal);
            Em_StartTurnToTarget(em, &EmBlangonga_TurnData.turn[1]);
            break;
        case 2:
            Em_StartTurnAway(em, &EmBlangonga_TurnData.turn[1]);
            break;
        default:
            Em_StartTurnToTarget(em, &EmBlangonga_TurnData.turn[1]);
        }
        break;
    case 1:
        if (Em_TurnStep(em, &EmBlangonga_TurnData.turn[1]) == 1) {
            if ((variant & 0xFF) == 1) {
                em->step++;
                EmBlangonga_ChangeState(em, EM_GROUP_FLY, BLANG_LEAP_OUT_OF_AREA, EM_SYNC_OWNER_FLAGGED);
                break;
            }
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    case 2:
        if (em->remote == 0) {
            em->step = 1;
        }
        break;
    }
}

/*
 * BLANG_MOVE_ACTION_A: plays BLANG_MOT_MOVE_ACTION_A once and ends the
 * state.
 */
void EmBlangonga_MoveActionA(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_MOVE_ACTION_A, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_MOVE_ACTION_B: plays BLANG_MOT_MOVE_ACTION_B once and ends the
 * state.
 */
void EmBlangonga_MoveActionB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_MOVE_ACTION_B, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_MOVE_ACTION_C: plays BLANG_MOT_MOVE_ACTION_C once and ends the
 * state.
 */
void EmBlangonga_MoveActionC(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_MOVE_ACTION_C, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_MOVE_ACTION_D and _ACTION_E: plays BLANG_MOT_MOVE_ACTION_D once and
 * ends the state.
 */
void EmBlangonga_MoveActionD(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_MOVE_ACTION_D, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_MOVE_START_AND_APPROACH (variant 0), _LIMP_APPROACH (1), _APPROACH
 * (2) and _START_AND_WALK (3): walks (0 and 3 start with
 * BLANG_MOT_WALK_START); 0-2 turn to the goal and stop after 120 frames or
 * within 700 (scaled) of it, 3 steps all the way to the goal.
 */
void EmBlangonga_Approach(Em *em, s32 unused, s32 variant)
{
    switch (em->step) {
    case 0:
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        em->timer = 0x78;
        switch (variant & 0xFF) {
        case 0:
        case 3:
        default:
            em->step++;
            Em_SetMotion(em, BLANG_MOT_WALK_START, 0, 0);
            break;
        case 1:
            em->step = 2;
            Em_SetMotion(em, BLANG_MOT_LIMP, 0, 0);
            break;
        case 2:
            em->step = 2;
            Em_SetMotion(em, BLANG_MOT_WALK, 0, 0);
            break;
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_WALK, 0, 0x44);
        }
    case 2:
        switch (variant & 0xFF) {
        case 3:
            if (Em_StepToGoal(em, 0x80, 0) == 1) {
                em->step++;
                EmBlangonga_EndState(em, 0, 0);
            }
            break;
        default:
            if (em->hasGoal != 0) {
                Em_TurnToGoal(em, 0x80, 0);
            }
            if (--em->timer <= 0 || (em->hasGoal != 0 && Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) < 700.0f * em->scaleX)) {
                em->step++;
                EmBlangonga_EndState(em, 0, 0);
            }
            break;
        }
        break;
    }
}

/* Runs the current EM_GROUP_MOVE state. The work area comes as an s32. */
void EmBlangonga_RunMoveState(Em *em, s32 work)
{
    switch (em->state) {
    case BLANG_MOVE_TURN:
        EmBlangonga_Turn(em, work, 0);
        break;
    case BLANG_MOVE_WALK:
        EmBlangonga_WalkToGoal(em, work, 0);
        break;
    case BLANG_MOVE_LIMP:
        EmBlangonga_WalkToGoal(em, work, 1);
        break;
    case BLANG_MOVE_TURN_QUICK:
        EmBlangonga_TurnQuick(em, work, 0);
        break;
    case BLANG_MOVE_ACTION_A:
        EmBlangonga_MoveActionA(em);
        break;
    case BLANG_MOVE_ACTION_B:
        EmBlangonga_MoveActionB(em);
        break;
    case BLANG_MOVE_ACTION_C:
        EmBlangonga_MoveActionC(em);
        break;
    case BLANG_MOVE_ACTION_D:
        EmBlangonga_MoveActionD(em);
        break;
    case BLANG_MOVE_TURN_TO_EXIT:
        EmBlangonga_TurnQuick(em, work, 1);
        break;
    case BLANG_MOVE_TURN_AWAY:
        EmBlangonga_Turn(em, work, 1);
        break;
    case BLANG_MOVE_TURN_AWAY_QUICK:
        EmBlangonga_TurnQuick(em, work, 2);
        break;
    case BLANG_MOVE_START_AND_APPROACH:
        EmBlangonga_Approach(em, work, 0);
        break;
    case BLANG_MOVE_LIMP_APPROACH:
        EmBlangonga_Approach(em, work, 1);
        break;
    case BLANG_MOVE_APPROACH:
        EmBlangonga_Approach(em, work, 2);
        break;
    case BLANG_MOVE_START_AND_WALK:
        EmBlangonga_Approach(em, work, 3);
        break;
    case BLANG_MOVE_ACTION_E:
        EmBlangonga_MoveActionD(em);
        
    default:
        break;
    }
}

/*
 * BLANG_LEAP_JUMP (variant 0), _JUMP_HIGH (1) and _JUMP_TO_GOAL (2):
 * crouches, takes off (frame 52, 56 or 48 by kind) on a curve, falls after
 * the apex and lands with a camera shake. The variant gives the speeds (2:
 * those that land on the goal in 60 frames); the Congalala kinds jump
 * lower.
 */
void EmBlangonga_Jump(Em *em, EmWork *work, u8 variant)
{
    f32 frame;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, BLANG_MOT_JUMP, 0, 0);
        Player_ClearVelocity(em);
        break;
    case 1:
        switch (em->type) {
        case EM_KIND_BLANGONGA:
        default:
        case EM_KIND_BLANGO:
            frame = 52.0f;
            break;
        case EM_KIND_CONGALALA:
        case EM_KIND_CONGA:
            frame = 56.0f;
            break;
        case EM_KIND_RAJANG:
            frame = 48.0f;
            break;
        }
        if (EmMotion_ReachedFrame(em, 0, frame)) {
            em->noReact = 1;
            em->stance = EM_STANCE_AIR;
            em->step++;
            if (variant == 2) {
                em->accelY = -6.4f;
                em->velY = (em->goal[1] - em->posY) / 60.0f - 60.0f * (0.5f * em->accelY);
                em->velZ = Vec_DistanceXZ(em->goal, &em->posX) / 60.0f;
            } else {
                switch (em->type) {
                case EM_KIND_BLANGONGA:
                default:
                case EM_KIND_BLANGO:
                case EM_KIND_RAJANG:
                    switch (variant) {
                    case 0:
                    default:
                        em->velY = 105.0f;
                        em->velZ = 30.0f;
                        em->accelY = -6.4f;
                        break;
                    case 1:
                        em->velY = 150.0f;
                        em->velZ = 45.0f;
                        em->accelY = -5.0f;
                        break;
                    }
                    break;
                case 0x34:
                case 0x3E:
                    em->velY = 80.0f;
                    em->velZ = 45.0f;
                    em->accelY = -4.0f;
                    break;
                }
            }
            work->accelAngX = em->rotX;
            work->accelAngY = em->rotY;
            work->accelAngZ = 0;
        }
        break;
    case 2:
        Em_MoveRotatedAccel(em, &work->accelAngX);
        if (em->velY <= 0.0f) {
            Em_SetMotion(em, BLANG_MOT_JUMP_FALL, 0, 0);
            em->step++;
        }
        break;
    case 3:
        Em_MoveRotatedAccel(em, &work->accelAngX);
        if (em->posY <= em->floorY) {
            em->posY = em->floorY;
            em->step++;
            em->stance = EM_STANCE_GROUND;
            CamShake_ReqAIf(em, 5);
            switch (em->type) {
            case EM_KIND_BLANGONGA:
            default:
            case EM_KIND_BLANGO:
            case EM_KIND_RAJANG:
                Em_SetMotion(em, BLANG_MOT_JUMP_LAND, 0, 0);
                break;
            case EM_KIND_CONGALALA:
            case EM_KIND_CONGA:
                Em_SetMotion(em, BLANG_MOT_JUMP_LAND, 8, 0);
                break;
            }
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->noReact = 0;
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_LEAP_HOP_SHORT (variant 0), _HOP (1) and _HOP_LONG (2), the
 * Congalala kinds: takes off at frame 26 with a hit at frame 50, lands with
 * a camera shake. The variant gives the forward speed.
 */
void EmBlangonga_Hop(Em *em, EmWork *work, s32 variant)
{
    if (EmBlangonga_IsCongalala(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
        return;
    }
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, BLANG_MOT_CONGALALA_HOP, 0, 0);
        Player_ClearVelocity(em);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 26.0f)) {
            em->step++;
            em->stance = EM_STANCE_AIR;
            switch (variant & 0xFF) {
            case 0:
                em->velZ = 4.0f;
                break;
            case 1:
            default:
                em->velZ = 44.0f;
                break;
            case 2:
                em->velZ = 76.0f;
                break;
            }
            if (em->scaleY < 1.0f) {
                em->velY = 70.0f * em->scaleY;
                em->accelY = -5.5f * em->scaleY;
            } else {
                em->velY = 70.0f;
                em->accelY = -5.5f;
            }
            work->accelAngX = em->rotX;
            work->accelAngY = em->rotY;
            work->accelAngZ = 0;
            em->noReact = 1;
        }
        break;
    case 2:
        if (EmMotion_PassedFrameScaled(em, 50.0f, 0)) {
            BlangongaAttack_Create(em, 0x21);
        }
        Em_MoveRotatedAccel(em, &work->accelAngX);
        if (em->posY <= em->floorY) {
            em->posY = em->floorY;
            em->step++;
            em->stance = EM_STANCE_GROUND;
            if (EmBlangonga_IsSmallKind(em) == 1) {
                CamShake_ReqAIf(em, 2);
            } else {
                CamShake_ReqAIf(em, 5);
            }
            Em_SetMotion(em, BLANG_MOT_HOP_LAND, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_LEAP_DIG_IN (the Blangonga kinds): digs into the snow (the shadow
 * goes from frame 80), then BLANG_LEAP_BURIED.
 */
void EmBlangonga_DigIn(Em *em)
{
    if (EmBlangonga_IsBlangonga(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
        return;
    }
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        if (em->motionId[0] != EM_MOTION_ID(0, BLANG_MOT_DIG_IN)) {
            Em_SetMotion(em, BLANG_MOT_DIG_IN, 0, 0);
        }
        em->noReact = 1;
        em->stance = EM_STANCE_GROUND;
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 80.0f) != 0) {
            em->hideShadowHold = 2;
            em->hideShadow = 2;
            em->noPushTimer = 2;
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            em->noReact = 0;
            EmBlangonga_EndBuriedState(em);
        }
        break;
    }
    em->guardTimer = 2;
}

/*
 * BLANG_LEAP_BURIED: stays under the snow (stateArmor); when its motion
 * ends the owner hands the choice back to the script
 * (EmBlangonga_EndBuriedState).
 */
void EmBlangonga_Buried(Em *em)
{
    if (EmBlangonga_IsBlangonga(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
    } else {
        em->stateArmor = 1;
        em->noPushTimer = 2;
        switch (em->step) {
        case 0:
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->lookAtTarget = 0;
            Em_SetMotion(em, BLANG_MOT_BURIED, 0, 0);
            break;
        case 1:
            if ((em->remote == 0) && (em->motion[0].playing == 0)) {
                em->step++;
                EmBlangonga_EndBuriedState(em);
            }
            break;
        }
    }
}

/*
 * BLANG_LEAP_DIG_OUT (variant 0) and _DIG_OUT_RAGE (1: the rage raised by
 * hp on entry): comes up out of the snow with two hits (frames 2 and 52),
 * the shadow back from frame 40, then ends the state.
 */
void EmBlangonga_DigOut(Em *em, EmWork *work, s32 variant)
{
    if (EmBlangonga_IsBlangonga(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
        return;
    }
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        if (em->motionId[0] != EM_MOTION_ID(0, BLANG_MOT_EMERGE)) {
            Em_SetMotion(em, BLANG_MOT_EMERGE, 0, 0);
            em->stateArmor = 1;
            em->noPushTimer = 2;
        } else if (em->motion[0].blend != 0) {
            em->stateArmor = 1;
            em->noPushTimer = 2;
        }
        em->noReact = 1;
        switch (variant & 0xFF) {
        case 1:
            Em_CheckRageByHp(em, em->rageThreshold);
            break;
        }
        break;
    case 1:
        if (em->motion[0].blend != 0) {
            em->stateArmor = 1;
            em->noPushTimer = 2;
        } else {
            if (EmMotion_PassedFrameScaled(em, 2.0f, 0)) {
                BlangongaAttack_Create(em, 6);
            }
            if (EmMotion_PassedFrameScaled(em, 52.0f, 0)) {
                BlangongaAttack_Create(em, 7);
            }
            if (!EmMotion_ReachedFrame(em, 0, 40.0f)) {
                em->hideShadowHold = 2;
                em->hideShadow = 2;
            }
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            em->noReact = 0;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
    em->guardTimer = 2;
}

/*
 * BLANG_LEAP_BURIED_TURN: turns toward the goal under the snow, checking
 * every 40 frames whether it faces it.
 */
void EmBlangonga_BuriedTurn(Em *em)
{
    u32 spd;
    u32 angleDiff;

    if (EmBlangonga_IsBlangonga(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
        return;
    }
    em->stateArmor = 1;
    em->noPushTimer = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        Em_SetMotion(em, BLANG_MOT_BURIED, 0, 0);
        em->timer = 0x28;
        break;
    case 1:
        spd = 409.6f * em->motionSpeed;
        angleDiff = (em->aimAngle - *(u16 *) &em->rotY) & 0xFFFF;
        if (--em->timer <= 0) {
            if (((angleDiff + spd) & 0xFFFF) < spd * 2) {
                em->step++;
                EmBlangonga_EndBuriedState(em);
            } else {
                em->timer = 0x28;
            }
        } else {
            if (((angleDiff + spd) & 0xFFFF) < spd * 2) {
                em->rotY = em->aimAngle;
            } else if (angleDiff < 0x8000) {
                em->rotY = (em->rotY + spd) & 0xFFFF;
            } else {
                em->rotY = (em->rotY - spd) & 0xFFFF;
            }
        }
        break;
    }
}

/*
 * BLANG_LEAP_BURIED_MOVE: moves under the snow to the goal, rumbling the
 * local player's pad when within 1000 of it.
 */
void EmBlangonga_BuriedMove(Em *em, EmWork *work)
{
    Player *player;

    player = &gPlayers[gScene.localPlayer];
    if (EmBlangonga_IsBlangonga(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
    } else {
        em->stateArmor = 1;
        em->noPushTimer = 2;
        switch (em->step) {
        case 0:
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->lookAtTarget = 0;
            Em_SetMotion(em, BLANG_MOT_BURIED, 0, 0x64);
            Player_ClearVelocity(em);
            em->velZ = 70.0f;
            work->accelAngX = em->rotX;
            work->accelAngY = em->rotY;
            work->accelAngZ = 0;
            break;
        case 1:
            Em_MoveRotatedAccel(em, &work->accelAngX);
            Em_StepToGoal(em, 0x10, 2);
            if (!(gScene.frame & 7) && (Obj_SameArea(em, player) != 0) && (Obj_InCurrentArea(player) != 0) && (Vec_Distance((f32 *) &em->posX, (f32 *) player->pos) < 1000.0f)) {
                Vib_Start(0, 2);
            }
            if (em->goalDist <= 0.0f) {
                em->step++;
                EmBlangonga_EndBuriedState(em);
            }
            break;
        }
    }
}

/*
 * BLANG_LEAP_BURIED_WAIT: waits 90 frames under the snow, then
 * BLANG_LEAP_BURIED.
 */
void EmBlangonga_BuriedWait(Em *em)
{
    em->stateArmor = 1;
    em->noPushTimer = 2;
    if (EmBlangonga_IsBlangonga(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->lookAtTarget = 0;
            Em_SetMotion(em, BLANG_MOT_BURIED, 0, 0);
            em->timer = 0x5A;
            break;
        case 1:
            if (--em->timer <= 0) {
                em->step++;
                EmBlangonga_EndBuriedState(em);
            }
            break;
        }
    }
}

/*
 * BLANG_LEAP_OUT_OF_AREA: takes off and rises for 150 frames, then drops
 * into the destination area (BLANG_LEAP_DROP_IN) or crosses to the next one
 * (BLANG_LEAP_CROSS_AREA). A remote copy waits for the owner's change.
 */
void EmBlangonga_LeapOutOfArea(Em *em, EmWork *work)
{
    f32 takeOffFrame;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, BLANG_MOT_JUMP, 0, 0);
        Player_ClearVelocity(em);
        break;
    case 1:
        switch (em->type) {
        case EM_KIND_BLANGONGA:
        default:
        case EM_KIND_BLANGO:
            takeOffFrame = 52.0f;
            break;
        case EM_KIND_CONGALALA:
        case EM_KIND_CONGA:
            takeOffFrame = 56.0f;
            break;
        case EM_KIND_RAJANG:
            takeOffFrame = 48.0f;
            break;
        }
        if (EmMotion_ReachedFrame(em, 0, takeOffFrame)) {
            em->step++;
            em->noReact = 1;
            em->stance = EM_STANCE_AIR;
            em->velY = 230.0f;
            em->velZ = 70.0f;
            em->accelY = -1.55f;
            work->accelAngX = em->rotX;
            work->accelAngY = em->rotY;
            work->accelAngZ = 0;
            em->timer = 150;
        }
        break;
    case 2:
        Em_MoveRotatedAccel(em, &work->accelAngX);
        if (--em->timer <= 0) {
            em->step++;
            if (em->nextArea == *(u16 *) &em->destArea || *(u16 *) &em->destArea == AREA_NONE) {
                EmBlangonga_EnterNewArea(em);
                EmBlangonga_ChangeState(em, EM_GROUP_FLY, BLANG_LEAP_DROP_IN, EM_SYNC_SEND);
                break;
            }
            Em_EnterNextArea(em);
            if (em->remote == 0) {
                em->goalNo = em->nextArea = *(u16 *) &em->destArea;
                em->goalKind = 3;
                Em_ComputeGoal(em, em->goal);
            }
            EmBlangonga_ChangeState(em, EM_GROUP_FLY, BLANG_LEAP_CROSS_AREA, EM_SYNC_SEND);
        }
        break;
    case 3:
        if (em->remote == 0) {
            em->step = 2;
        }
        break;
    }
    em->guardTimer = 2;
}

/*
 * BLANG_LEAP_DROP_IN: falls into the area from above and lands with a
 * camera shake; Blangonga and Rajang then restart their script.
 */
void EmBlangonga_DropIn(Em *em, EmWork *work)
{
    u16 leftArea;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        em->noReact = 1;
        Em_SetMotionRow(em, BLANG_MOT_JUMP_FALL, 0, 0);
        Player_ClearVelocity(em);
        em->velY = -30.0f;
        em->velZ = 0.0f;
        em->accelY = -4.7f;
        work->accelAngX = em->rotX;
        work->accelAngY = em->rotY;
        work->accelAngZ = 0;
        if (em->type == EM_KIND_BLANGONGA || em->type == EM_KIND_RAJANG) {
            leftArea = em->prevArea;
            Em_RestartScript(em);
            em->prevArea = leftArea;
        }
        break;
    case 1:
        Em_MoveRotatedAccel(em, &work->accelAngX);
        if (em->posY <= em->floorY) {
            em->posY = em->floorY;
            em->step++;
            em->stance = EM_STANCE_GROUND;
            CamShake_ReqAIf(em, 5);
            switch (em->type) {
            case EM_KIND_BLANGONGA:
            default:
            case EM_KIND_BLANGO:
            case EM_KIND_RAJANG:
                Em_SetMotion(em, BLANG_MOT_JUMP_LAND, 0, 0);
                break;
            case EM_KIND_CONGALALA:
            case EM_KIND_CONGA:
                Em_SetMotion(em, BLANG_MOT_JUMP_LAND, 8, 0);
                break;
            }
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->noReact = 0;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_LEAP_CROSS_AREA: unseen and unhittable, runs to the area's exit
 * until it leaves (or 300 frames), then changes area as
 * BLANG_LEAP_OUT_OF_AREA does.
 */
void EmBlangonga_CrossArea(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noPushTimer = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        em->noReact = 1;
        Em_SetMotion(em, BLANG_MOT_JUMP, 0, 0x38);
        Player_ClearVelocity(em);
        em->velZ = 70.0f;
        Em_AreaExitPoint(em, (f32 *) em->goal);
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->rotY = em->aimAngle;
        em->timer = 300;
        work->accelAngX = em->rotX;
        work->accelAngY = em->rotY;
        work->accelAngZ = 0;
        break;
    case 1:
        Em_MoveRotated(em, &work->accelAngX);
        if (!Area_ContainsPos(em->area, &em->posX) || --em->timer <= 0) {
            em->step++;
        }
        break;
    case 2:
        em->step++;
        em->timer = 600;
        if (em->nextArea == (u16) em->destArea || (u16) em->destArea == AREA_NONE) {
            EmBlangonga_EnterNewArea(em);
            EmBlangonga_ChangeState(em, EM_GROUP_FLY, BLANG_LEAP_DROP_IN, EM_SYNC_SEND);
            break;
        }
        Em_EnterNextArea(em);
        if (em->remote == 0) {
            em->goalNo = em->nextArea = *(u16 *) &em->destArea;
            em->goalKind = 3;
            Em_ComputeGoal(em, em->goal);
        }
        EmBlangonga_ChangeState(em, EM_GROUP_FLY, BLANG_LEAP_CROSS_AREA, EM_SYNC_SEND);
        break;
    case 3:
        if (em->remote == 0) {
            em->step = 2;
        }
        break;
    }
}

/* BLANG_LEAP_BURIED_WAIT_RANDOM: waits 1-16 random frames under the snow. */
void EmBlangonga_BuriedWaitRandom(Em *em)
{
    em->stateArmor = 1;
    em->noPushTimer = 2;
    em->noReact = 1;
    if (EmBlangonga_IsBlangonga(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->lookAtTarget = 0;
            Em_SetMotion(em, BLANG_MOT_BURIED, 0, 0);
            em->timer = (Rand_Next(1) & 0xFFFF & 0xF) + 1;
            break;
        case 1:
            if (--em->timer <= 0) {
                em->step++;
                EmBlangonga_EndBuriedState(em);
            }
            break;
        }
    }
}

/*
 * Runs the current group-2 state (the family's EM_GROUP_FLY: jumps, leaps
 * and burrowing).
 */
void EmBlangonga_RunLeapState(Em *em, EmWork *work)
{
    switch (em->state) {
    case BLANG_LEAP_JUMP:
        EmBlangonga_Jump(em, work, 0);
        break;
    case BLANG_LEAP_HOP_SHORT:
        EmBlangonga_Hop(em, work, 0);
        break;
    case BLANG_LEAP_DIG_IN:
        EmBlangonga_DigIn(em);
        break;
    case BLANG_LEAP_BURIED:
        EmBlangonga_Buried(em);
        break;
    case BLANG_LEAP_DIG_OUT:
        EmBlangonga_DigOut(em, work, 0);
        break;
    case BLANG_LEAP_BURIED_TURN:
        EmBlangonga_BuriedTurn(em);
        break;
    case BLANG_LEAP_BURIED_MOVE:
        EmBlangonga_BuriedMove(em, work);
        break;
    case BLANG_LEAP_BURIED_WAIT:
        EmBlangonga_BuriedWait(em);
        break;
    case BLANG_LEAP_OUT_OF_AREA:
        EmBlangonga_LeapOutOfArea(em, work);
        break;
    case BLANG_LEAP_DROP_IN:
        EmBlangonga_DropIn(em, work);
        break;
    case BLANG_LEAP_DIG_OUT_RAGE:
        EmBlangonga_DigOut(em, work, 1);
        break;
    case BLANG_LEAP_CROSS_AREA:
        EmBlangonga_CrossArea(em, work);
        break;
    case BLANG_LEAP_JUMP_HIGH:
        EmBlangonga_Jump(em, work, 1);
        break;
    case BLANG_LEAP_BURIED_WAIT_RANDOM:
        EmBlangonga_BuriedWaitRandom(em);
        break;
    case BLANG_LEAP_HOP:
        EmBlangonga_Hop(em, work, 1);
        break;
    case BLANG_LEAP_HOP_LONG:
        EmBlangonga_Hop(em, work, 2);
        break;
    case BLANG_LEAP_JUMP_TO_GOAL:
        EmBlangonga_Jump(em, work, 2);
        
    default:
        break;
    }
}

/*
 * BLANG_ATK_TRIPLE_HIT (the Blangonga kinds): turns to the goal for 40
 * frames and hits three times (frames 36, 64 and 86; the first behind it).
 */
void EmBlangonga_TripleHit(Em *em)
{
    if (EmBlangonga_IsBlangonga(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, BLANG_MOT_TRIPLE_HIT, 0, 0);
            break;
        case 1:
            if (EmMotion_ReachedFrame(em, 0, 40.0f) == 0) {
                Em_TurnToGoal(em, 0x60, 0);
            }
            if (EmMotion_PassedFrameScaled(em, 36.0f, 0U) != 0) {
                BlangongaAttack_Create(em, 0x12);
            }
            if (EmMotion_PassedFrameScaled(em, 64.0f, 0U) != 0) {
                BlangongaAttack_Create(em, 0x18);
            }
            if (EmMotion_PassedFrameScaled(em, 86.0f, 0U) != 0) {
                BlangongaAttack_Create(em, 1);
            }
            if (em->motion[0].playing == 0) {
                em->step++;
                Em_SetMotion(em, BLANG_MOT_TRIPLE_HIT_END, 0, 0);
            }
            break;
        case 2:
            if (em->motion[0].playing == 0) {
                em->step++;
                EmBlangonga_EndState(em, 0, 0);
            }
            break;
        }
    }
}

/*
 * BLANG_ATK_SLAM (variant 0) and _SLAM_HEAVY (1): winds up for 20 or 40
 * frames and slams (a hit at frame 20); at frame 30 the large kinds add a
 * heavier hit and a camera shake, and the heavy slam also brings down the
 * area's falling objects (Blangonga and Rajang).
 */
void EmBlangonga_Slam(Em *em, EmWork *work, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_SLAM_WINDUP, 0, 0);
        switch (variant & 0xFF) {
        case 0:
        default:
            em->timer = 0x14;
            break;
        case 1:
            em->timer = 0x28;
            break;
        }
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_SLAM, 0, 0);
        }
        break;
    case 2:
        if (EmMotion_PassedFrameScaled(em, 20.0f, 0)) {
            BlangongaAttack_Create(em, 2);
        }
        if (EmBlangonga_IsSmallKind(em) == 0 && EmMotion_PassedFrameScaled(em, 30.0f, 0)) {
            switch (variant & 0xFF) {
            case 1:
                BlangongaAttack_Create(em, 8);
                CamShake_ReqAIf(em, 5);
                if (EmBlangonga_IsBlangonga(em) == 1 || EmBlangonga_IsRajang(em) == 1) {
                    EmBlangonga_DropFallingObjects(em);
                }
                break;
            default:
                CamShake_ReqAIf(em, 3);
                BlangongaAttack_Create(em, 0x19);
                break;
            }
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_ATK_STRIKE_A (variant 0) and _B (1), any kind: one blow with a hit
 * at frame 14 (the small kinds' weaker hit).
 */
void EmBlangonga_Strike(Em *em, EmWork *work, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        switch (variant & 0xFF) {
        case 0:
        default:
            Em_SetMotion(em, BLANG_MOT_STRIKE_A, 0, 0);
            break;
        case 1:
            Em_SetMotion(em, BLANG_MOT_STRIKE_B, 0, 0);
            break;
        }
        break;
    case 1:
        switch (variant & 0xFF) {
        case 0:
        default:
            if (EmMotion_PassedFrameScaled(em, 14.0f, 0)) {
                if (EmBlangonga_IsSmallKind(em) == 0) {
                    BlangongaAttack_Create(em, 3);
                } else {
                    BlangongaAttack_Create(em, 0x15);
                }
            }
            break;
        case 1:
            if (EmMotion_PassedFrameScaled(em, 14.0f, 0)) {
                if (EmBlangonga_IsSmallKind(em) == 0) {
                    BlangongaAttack_Create(em, 4);
                } else {
                    BlangongaAttack_Create(em, 0x16);
                }
            }
            break;
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_ATK_CONGALALA_EMISSION (the Congalala kinds): plays its motion and
 * at frame 110 creates effect 33 of kind 6 at bone 0x1C.
 */
void EmBlangonga_CongalalaEmission(Em *em)
{
    if (EmBlangonga_IsCongalala(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, BLANG_MOT_CONGALALA_EMISSION, 0, 0);
            break;
        case 1:
            if (EmMotion_PassedFrameScaled(em, 110.0f, 0U) != 0) {
                EmEffectAttack_Create(em, EMFX_CONGALALA_GAS, 0, 0, 0);
            }
            if (em->motion[0].playing == 0) {
                em->step++;
                EmBlangonga_EndState(em, 0, 0);
            }
            break;
        }
    }
}

/*
 * BLANG_ATK_BLANGO_THROW (the Blango): throws a projectile (EmRangedAttack type
 * 0x11) from bone 0xE at frame 74.
 */
void EmBlangonga_BlangoThrow(Em *em)
{
    if (em->type != EM_KIND_BLANGO) {
        EmBlangonga_EndState(em, 0, 0);
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, BLANG_MOT_THROW, 0, 0);
            break;
        case 1:
            if (EmMotion_PassedFrameScaled(em, 74.0f, 0U) != 0) {
                EmRangedAttack_CreateAtBone(em, 0xE, 0x11, 0, 0xFC00, 0);
            }
            if (em->motion[0].playing == 0) {
                em->step++;
                EmBlangonga_EndState(em, 0, 0);
            }
            break;
        }
    }
}

/*
 * BLANG_ATK_CHARGE (not the Congalala kinds): runs at the goal with a hit
 * along, then finishes with BLANG_MOT_CHARGE_END and its hits.
 */
void EmBlangonga_Charge(Em *em)
{
    if (EmBlangonga_IsCongalala(em) == 1) {
        EmBlangonga_EndState(em, 0, 0);
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            em->stateVar0 = 0;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, BLANG_MOT_CHARGE, 0, 0);
            break;
        case 1:
            if (EmMotion_PassedFrameScaled(em, 6.0f, 0U) != 0) {
                BlangongaAttack_Create(em, 0xF);
            }
            if ((em->stateVar0 == 0) && (Em_StepToGoal(em, 0x80, 0) == 1)) {
                em->step++;
                em->stateVar0++;
                Em_SetMotion(em, BLANG_MOT_CHARGE_END, 0, 0);
            }
            break;
        case 2:
            if (EmBlangonga_IsBlangonga(em) == 1) {
                if (EmMotion_PassedFrameScaled(em, 10.0f, 0U) != 0) {
                    BlangongaAttack_Create(em, 0x11);
                }
                if (EmMotion_PassedFrameScaled(em, 20.0f, 0U) != 0) {
                    if (EmBlangonga_IsSmallKind(em) == 1) {
                        BlangongaAttack_Create(em, 0x1A);
                    } else {
                        BlangongaAttack_Create(em, 5);
                    }
                }
            } else if (EmMotion_PassedFrameScaled(em, 22.0f, 0U) != 0) {
                BlangongaAttack_Create(em, 0x25);
            }
            if (em->motion[0].playing == 0) {
                em->step++;
                EmBlangonga_EndState(em, 0, 0);
            }
            break;
        }
    }
}

/*
 * BLANG_ATK_DIG_THROW_A, _B and _C (variants 0-2; Blangonga): digs up a
 * lump with a hit at frame 98 and throws it (EmRangedAttack type 0xF from bone
 * 0xD) at frame 102; the variant sets the throw's pitch.
 */
void EmBlangonga_DigThrow(Em *em, EmWork *work, s32 variant)
{
    u16 ang;

    if (em->type != EM_KIND_BLANGONGA) {
        EmBlangonga_EndState(em, 0, 0);
        return;
    }
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_DIG_THROW, 0, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 98.0f, 0)) {
            BlangongaAttack_Create(em, 0x13);
        }
        if (EmMotion_PassedFrameScaled(em, 102.0f, 0)) {
            switch (variant & 0xFF) {
            default:
                ang = 0xC300;
                break;
            case 1:
                ang = 0xC500;
                break;
            case 2:
                ang = 0xC900;
                break;
            }
            EmRangedAttack_CreateAtBone(em, 0xD, 0xF, 0, ang, 0);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_ATK_CONGALALA_EATEN_EMISSION (Congalala): at frame 100, when it has
 * eaten (eatenKind), creates stage object 0x17 and effect 33 of kind 1 by
 * what it ate.
 */
void EmBlangonga_CongalalaEatenEmission(Em *em, EmWork *work)
{
    EmBlangongaWork *w = (EmBlangongaWork *)work;

    if (em->type != EM_KIND_CONGALALA) {
        EmBlangonga_EndState(em, 0, 0);
        return;
    }
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_CONGALALA_EATEN_EMISSION, 0, 0);
        break;
    case 1:
        if (w->eatenKind != 0 && EmMotion_PassedFrameScaled(em, 100.0f, 0)) {
            MonsterFx_CreateOnEmVariant(em, 0x17, w->eatenKind - 1, 1.0f);
            EmEffectAttack_Create(em, EMFX_CONGALALA_BREATH, w->eatenKind - 1, 0, 0);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_ATK_CONGALALA_COMBO (the Congalala kinds): a long combo with a hit
 * from frame 26, half a turn over frames 170-180 and an impact at frame 200
 * (the large kind), then BLANG_MOT_CONGALALA_COMBO_END.
 */
void EmBlangonga_CongalalaCombo(Em *em, EmWork *work)
{
    if (EmBlangonga_IsCongalala(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
        return;
    }
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_CONGALALA_COMBO, 0, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 26.0f, 0)) {
            BlangongaAttack_Create(em, 0x1F);
        }
        if (EmMotion_InFrameRange(em, 0, 170.0f, 180.0f)) {
            em->rotY += 0xCCC;
        }
        if (EmBlangonga_IsSmallKind(em) == 0 && EmMotion_PassedFrameScaled(em, 200.0f, 0)) {
            EmImpactAttack_CreateAt(em, &em->posX, 0x10);
            CamShake_ReqAIf(em, 5);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_CONGALALA_COMBO_END, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_ATK_CONGALALA_HITZONE_COMBO (Congalala): a hit from frame 40 with
 * the second damage table (hitzoneSet 1) until the first motion ends, then
 * a second blow with a hit at frame 88.
 */
void EmBlangonga_CongalalaHitzoneCombo(Em *em)
{
    if (em->type != EM_KIND_CONGALALA) {
        EmBlangonga_EndState(em, 0, 0);
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, BLANG_MOT_CONGALALA_HITZONE_COMBO, 0, 0);
            break;
        case 1:
            if (EmMotion_PassedFrameScaled(em, 40.0f, 0U) != 0) {
                BlangongaAttack_Create(em, 0x20);
                em->hitzoneSet = 1;
            }
            if (em->motion[0].playing == 0) {
                em->step++;
                Em_SetMotion(em, BLANG_MOT_CONGALALA_HITZONE_COMBO_END, 0, 0);
                em->hitzoneSet = 0;
            }
            break;
        case 2:
            if (EmMotion_PassedFrameScaled(em, 88.0f, 0U) != 0) {
                BlangongaAttack_Create(em, 0x29);
            }
            if (em->motion[0].playing == 0) {
                em->step++;
                EmBlangonga_EndState(em, 0, 0);
            }
            break;
        }
    }
}

/*
 * BLANG_ATK_CONGALALA_THROW_A (variant 0) and _B (1), the Congalala kinds:
 * throws a projectile (EmRangedAttack type 0x13) from bone 0x21 at frame 66; the
 * variant sets its pitch.
 */
void EmBlangonga_CongalalaThrow(Em *em, EmWork *work, s32 variant)
{
    if (EmBlangonga_IsCongalala(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, BLANG_MOT_THROW, 0, 0);
            break;
        case 1:
            if (!(variant & 0xFF)) {
                if (EmMotion_PassedFrameScaled(em, 66.0f, 0U) != 0) {
                    EmRangedAttack_CreateAtBone(em, 0x21, 0x13, 0, 0x93F, 0);
                }
            } else if (EmMotion_PassedFrameScaled(em, 66.0f, 0U) != 0) {
                EmRangedAttack_CreateAtBone(em, 0x21, 0x13, 0, 0x2000, 0);
            }
            if (em->motion[0].playing == 0) {
                em->step++;
                EmBlangonga_EndState(em, 0, 0);
            }
            break;
        }
    }
}

/*
 * BLANG_ATK_BACK_STRIKE: plays BLANG_MOT_BACK_STRIKE to its end; its hits
 * come from EmBlangonga_MotionEffects.
 */
void EmBlangonga_BackStrike(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_BACK_STRIKE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_ATK_EFFECT_STRIKE (the Blangonga kinds): at frame 98 creates stage
 * object 8 on itself and a hit, then plays the end motion. It also works
 * out a point in front of bone 1 that nothing reads.
 */
void EmBlangonga_EffectStrike(Em *em, EmWork *work)
{
    f32 pos[3];
    f32 ofs[3];
    void *bone;

    if (EmBlangonga_IsBlangonga(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
        return;
    }
    switch (em->step) {
    case 0:
        em->step++;
        em->stateVar1 = 0;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_EFFECT_STRIKE, 0, 0);
        break;
    case 1:
        if (em->stateVar1 == 0 && EmMotion_PassedFrameScaled(em, 98.0f, 0)) {
            MonsterFx_CreateOnEm(em, 8, 0.75f * em->scaleX);
            BlangongaAttack_Create(em, 0xA);
            if (Player_InCurrentArea(em)) {
                ofs[1] = 145.0f;
                ofs[0] = 0.0f;
                ofs[2] = 485.0f;
                bone = Em_Bone(em, 1);
                Vec_RotateSelf(ofs, bone);
                Mtx_GetTrans(pos, bone);
                Vec_Add(pos, pos, ofs);
            }
            em->stateVar1++;
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_EFFECT_STRIKE_END, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_ATK_RAJANG_COMBO_SLAM_A (variant 0) and _B (1): a blow with a hit,
 * then 3 (or 1) frames after its end a slam whose impact at frame 26 shakes
 * the camera and brings down the area's falling objects.
 */
void EmBlangonga_RajangComboSlam(Em *em, EmWork *work, s32 variant)
{
    f32 pos[3];

    if (EmBlangonga_IsRajang(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
        return;
    }
    switch (em->step) {
    case 0:
        em->step++;
        em->stateVar0 = 0;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_STRIKE_A, 0, 0);
        if ((variant & 0xFF) == 0) {
            em->timer = 3;
        } else {
            em->timer = 1;
        }
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 16.0f, 0) && em->stateVar0 == 0) {
            em->stateVar0++;
            BlangongaAttack_Create(em, 0x24);
        }
        if (em->motion[0].playing == 0 && --em->timer <= 0) {
            em->step++;
            em->stateVar0 = 0;
            Em_SetMotion(em, BLANG_MOT_STRIKE_B, 0, 0);
        }
        break;
    case 2:
        if (EmMotion_PassedFrameScaled(em, 26.0f, 0) && em->stateVar0 == 0) {
            em->stateVar0++;
            Em_BonePos(em, 0xD, pos);
            pos[1] = em->floorY;
            EmImpactAttack_CreateAt(em, pos, 0xF);
            CamShake_ReqAIf(em, 5);
            EmBlangonga_DropFallingObjects(em);
            if ((s32)Player_InCurrentArea(em) & 0xFF) {
                AttackBurst_CreateAtScaled(pos, *(u16 *) &em->rotY, 0xA, 0.4f * em->scaleX);
            }
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_ATK_RAJANG_SHOT_A (variant 0) and _B (1): aims and at frame 84
 * fires effect 33 of kind 2 with stage object 0x25; the variant sets the
 * pitch.
 */
void EmBlangonga_RajangShot(Em *em, EmWork *work, s32 variant)
{
    if (EmBlangonga_IsRajang(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
        return;
    }
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_RAJANG_AIM, 2, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 84.0f, 0)) {
            em->step++;
            switch (variant & 0xFF) {
            case 0:
            default:
                EmEffectAttack_Create(em, EMFX_RAJANG_SHOT, 0, 0x71C, 0);
                MonsterFx_CreateOnEmAngled(em, 0x25, 0x71C, 0, 1.0f);
                break;
            case 1:
                EmEffectAttack_Create(em, EMFX_RAJANG_SHOT, 0, 0xF8E5, 0);
                MonsterFx_CreateOnEmAngled(em, 0x25, 0xF8E5, 0, 1.0f);
                break;
            }
            Em_SetMotion(em, BLANG_MOT_RAJANG_AIM_END, 2, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_ATK_RAJANG_BEAM_A (variant 0) and _B (1): aims and at frame 80
 * fires the beam (EmRangedAttack type 0x15, the variant passed on) from bone 1 for
 * 0x28 frames.
 */
void EmBlangonga_RajangBeam(Em *em, EmWork *work, s32 variant)
{
    if (EmBlangonga_IsRajang(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
        return;
    }
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_RAJANG_AIM, 0, 0);
        em->timer = 2;
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 80.0f, 0)) {
            switch (variant & 0xFF) {
            case 0:
            default:
                EmRangedAttack_CreateAtBoneTimed(em, 1, 0x15, 0, 0, 0, 0x28);
                break;
            case 1:
                EmRangedAttack_CreateAtBoneTimed(em, 1, 0x15, 1, 0, 0, 0x28);
                break;
            }
        }
        if (em->motion[0].playing == 0) {
            em->timer--;
        }
        if (em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_RAJANG_AIM_END, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_ATK_RAJANG_JUMP_SHOT (variant 0) and BLANG_REACT_TRAP_ESCAPE (1:
 * unguarded hits and the trap's hold set): jumps, fires a shot downward
 * from the air (frame 22) while held above the floor, and lands when the
 * curves end.
 */
void EmBlangonga_RajangJumpShot(Em *em, EmWork *work, s32 variant)
{
    f32 posY;
    s32 ended;
    s32 k;

    if (EmBlangonga_IsRajang(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
    } else {
        k = variant & 0xFF;
        if (k == 1) {
            em->guardTimer = 2;
        }
        switch (em->step) {
        case 0:
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, BLANG_MOT_RAJANG_JUMP, 0, 0);
            if (k == 1) {
                em->holdTimer = 0x32;
            }
            break;
        case 1:
            if (EmMotion_PassedFrameScaled(em, 24.0f, 0U) != 0) {
                em->step++;
                em->stance = EM_STANCE_AIR;
                em->posY += 170.0f;
                EmBlangonga_StartCurves(em, 1U);
            }
            break;
        case 2:
            EmBlangonga_UpdateCurves(em);
            if (EmMotion_PassedFrameScaled(em, 30.0f, 0U) != 0) {
                em->step++;
                Em_SetMotion(em, BLANG_MOT_RAJANG_JUMP_SHOT, 6, 0);
                EmBlangonga_StartCurves(em, 2U);
            }
            break;
        case 3:
            if (EmMotion_PassedFrameScaled(em, 22.0f, 0U) != 0) {
                EmEffectAttack_Create(em, EMFX_RAJANG_SHOT, 1, 0x31C7, 0);
                MonsterFx_CreateOnEmAngled(em, 0x25, 0x31C7, 0, 1.0f);
            }
            ended = EmBlangonga_UpdateCurves(em) & 0xFF;
            posY = 345.0f + em->floorY;
            if (ended == 0) {
                if (em->posY <= posY) {
                    em->posY = posY;
                    break;
                }
            } else if ((ended != 0) && (em->posY <= posY)) {
                em->step++;
                em->posY = em->floorY;
                em->stance = EM_STANCE_GROUND;
                Em_SetMotion(em, BLANG_MOT_RAJANG_JUMP_SHOT_LAND, 0, 0);
                Player_ClearVelocity(em);
            }
            break;
        case 4:
            if (em->motion[0].playing == 0) {
                em->step++;
                EmBlangonga_EndState(em, 0, 0);
            }
            break;
        }
    }
}

/* BLANG_ATK_RAJANG_STRIKE: one blow with a hit at frame 56. */
void EmBlangonga_RajangStrike(Em *em)
{
    if (EmBlangonga_IsRajang(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, BLANG_MOT_RAJANG_STRIKE, 2, 0);
            break;
        case 1:
            if (EmMotion_PassedFrameScaled(em, 56.0f, 0U) != 0) {
                BlangongaAttack_Create(em, 0x26);
            }
            if (em->motion[0].playing == 0) {
                em->step++;
                EmBlangonga_EndState(em, 0, 0);
            }
            break;
        }
    }
}

/*
 * BLANG_ATK_RAJANG_CHARGE: runs at the goal with a hit that lasts into the
 * end motion.
 */
void EmBlangonga_RajangCharge(Em *em)
{
    u8 stateVar0;

    if (EmBlangonga_IsRajang(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            em->stateVar0 = 0;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, BLANG_MOT_RAJANG_CHARGE, 2, 0);
            break;
        case 1:
            if (EmMotion_PassedFrameScaled(em, 6.0f, 0U) != 0) {
                stateVar0 = em->stateVar0;
                if (stateVar0 == 0) {
                    em->stateVar0 = stateVar0 + 1;
                    BlangongaAttack_Create(em, 0x27);
                }
            }
            if (Em_StepToGoal(em, 0x80, 0) == 1) {
                em->step++;
                Em_SetMotion(em, BLANG_MOT_RAJANG_CHARGE_END, 0, 0);
            }
            break;
        case 2:
            if (em->motion[0].playing == 0) {
                em->step++;
                EmBlangonga_EndState(em, 0, 0);
            }
            break;
        }
    }
}

/*
 * BLANG_ATK_CONGALALA_CHARGE (the Congalala kinds): runs at the goal with a
 * hit along (the small kind's weaker), then ends the state.
 */
void EmBlangonga_CongalalaCharge(Em *em)
{
    if (EmBlangonga_IsCongalala(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, BLANG_MOT_CHARGE, 0, 0);
            if (EmBlangonga_IsSmallKind(em) == 1) {
                BlangongaAttack_Create(em, 0x23);
                break;
            }
            BlangongaAttack_Create(em, 0x1D);
            break;
        case 1:
            if (Em_StepToGoal(em, 0x40, 0) == 1) {
                em->step++;
                EmBlangonga_EndState(em, 0, 0);
            }
            break;
        }
    }
}

/*
 * Runs the current EM_GROUP_ATTACK state, passing the shared functions
 * their variant.
 */
void EmBlangonga_RunAttackState(Em *em, EmWork *work)
{
    switch (em->state) {
    case BLANG_ATK_TRIPLE_HIT:
        EmBlangonga_TripleHit(em);
        break;
    case BLANG_ATK_SLAM:
        EmBlangonga_Slam(em, work, 0);
        break;
    case BLANG_ATK_STRIKE_A:
        EmBlangonga_Strike(em, work, 0);
        break;
    case BLANG_ATK_STRIKE_B:
        EmBlangonga_Strike(em, work, 1);
        break;
    case BLANG_ATK_CONGALALA_EMISSION:
        EmBlangonga_CongalalaEmission(em);
        break;
    case BLANG_ATK_BLANGO_THROW:
        EmBlangonga_BlangoThrow(em);
        break;
    case BLANG_ATK_CHARGE:
        EmBlangonga_Charge(em);
        break;
    case BLANG_ATK_DIG_THROW_A:
        EmBlangonga_DigThrow(em, work, 0);
        break;
    case BLANG_ATK_CONGALALA_EATEN_EMISSION:
        EmBlangonga_CongalalaEatenEmission(em, work);
        break;
    case BLANG_ATK_CONGALALA_COMBO:
        EmBlangonga_CongalalaCombo(em, work);
        break;
    case BLANG_ATK_CONGALALA_HITZONE_COMBO:
        EmBlangonga_CongalalaHitzoneCombo(em);
        break;
    case BLANG_ATK_CONGALALA_THROW_A:
        EmBlangonga_CongalalaThrow(em, work, 0);
        break;
    case BLANG_ATK_BACK_STRIKE:
        EmBlangonga_BackStrike(em);
        break;
    case BLANG_ATK_SLAM_HEAVY:
        EmBlangonga_Slam(em, work, 1);
        break;
    case BLANG_ATK_EFFECT_STRIKE:
        EmBlangonga_EffectStrike(em, work);
        break;
    case BLANG_ATK_DIG_THROW_B:
        EmBlangonga_DigThrow(em, work, 1);
        break;
    case BLANG_ATK_DIG_THROW_C:
        EmBlangonga_DigThrow(em, work, 2);
        break;
    case BLANG_ATK_RAJANG_COMBO_SLAM_A:
        EmBlangonga_RajangComboSlam(em, work, 0);
        break;
    case BLANG_ATK_RAJANG_SHOT_A:
        EmBlangonga_RajangShot(em, work, 0);
        break;
    case BLANG_ATK_RAJANG_BEAM_A:
        EmBlangonga_RajangBeam(em, work, 0);
        break;
    case BLANG_ATK_RAJANG_JUMP_SHOT:
        EmBlangonga_RajangJumpShot(em, work, 0);
        break;
    case BLANG_ATK_RAJANG_STRIKE:
        EmBlangonga_RajangStrike(em);
        break;
    case BLANG_ATK_RAJANG_CHARGE:
        EmBlangonga_RajangCharge(em);
        break;
    case BLANG_ATK_CONGALALA_CHARGE:
        EmBlangonga_CongalalaCharge(em);
        break;
    case BLANG_ATK_RAJANG_BEAM_B:
        EmBlangonga_RajangBeam(em, work, 1);
        break;
    case BLANG_ATK_CONGALALA_THROW_B:
        EmBlangonga_CongalalaThrow(em, work, 1);
        break;
    case BLANG_ATK_RAJANG_COMBO_SLAM_B:
        EmBlangonga_RajangComboSlam(em, work, 1);
        break;
    case BLANG_ATK_RAJANG_SHOT_B:
        EmBlangonga_RajangShot(em, work, 1);
        break;
    }
}

/* BLANG_REACT_FLINCH: flinches (BLANG_MOT_FLINCH) and ends the state. */
void EmBlangonga_Flinch(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_FLINCH, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_REACT_FLINCH_B and _PART_BREAK: flinches (BLANG_MOT_FLINCH_B) and
 * ends the state.
 */
void EmBlangonga_FlinchB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_FLINCH_B, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_REACT_KNOCK_DOWN (variant 0), _KNOCK_DOWN_B (1), _STUN (2) and
 * _STUN_B (3): is knocked down (the odd variants from the other side), lies
 * and gets up. The knock-downs lie 90 frames; the stuns add the stun's
 * stage object, lie until the status-5 timer runs out and then shake it off
 * (BLANG_IDLE_STUN_SHAKE_OFF).
 */
void EmBlangonga_KnockDown(Em *em, EmWork *work, u8 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        if (!(variant & 1)) {
            Em_SetMotion(em, BLANG_MOT_KNOCK_DOWN, 0, 0);
        } else {
            Em_SetMotion(em, BLANG_MOT_KNOCK_DOWN_B, 0, 0);
        }
        switch (variant) {
        case 2:
        case 3:
            StunStars_CreateEm(em, 1, 0x11, (s32) ((f32) em->timer + (em->motion[0].frameCount / 2.0f)), 100.0f, 2.0f);
            break;
        }
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            if (!(variant & 1)) {
                Em_SetMotion(em, BLANG_MOT_DOWN, 0, 0);
            } else {
                Em_SetMotion(em, BLANG_MOT_DOWN_B, 0, 0);
            }
            switch (variant) {
            case 0:
            default:
            case 1:
                em->timer = 90;
                break;
            case 2:
            case 3:
                break;
            }
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            if (!(variant & 1)) {
                Em_SetMotion(em, BLANG_MOT_DOWN_GET_UP, 0, 0);
            } else {
                Em_SetMotion(em, BLANG_MOT_DOWN_GET_UP_B, 0, 0);
            }
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            switch (variant) {
            case 0:
            default:
            case 1:
                EmBlangonga_EndState(em, 0, 0);
                break;
            case 2:
            case 3:
                em->step++;
                EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_STUN_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
                break;
            }
        }
        break;
    case 4:
        if (em->remote == 0) {
            EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_STUN_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * BLANG_REACT_STATUS4: hit-react mode 4 with the periodic effect at bone 2
 * until the status-4 timer runs out, then BLANG_IDLE_STATUS4_SHAKE_OFF.
 */
void EmBlangonga_Status4(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_STATUS);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_STATUS4, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_STATUS4_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_STATUS4_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
    Em_SpawnStatus4Effect(em, 2);
}

/*
 * BLANG_REACT_TRAPPED: caught in a trap (hit-react mode 3, holdTimer 50)
 * while BLANG_MOT_TRAPPED plays, then BLANG_REACT_TRAPPED_STRUGGLE.
 */
void EmBlangonga_Trapped(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_NO_FLINCH);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, BLANG_MOT_TRAPPED, 0, 0);
        em->noReact = 1;
        em->holdTimer = 0x32;
        break;
    case 1:
        if ((s8) em->holdTimer < 5) {
            em->holdTimer = 5;
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_TRAPPED_STRUGGLE, EM_SYNC_OWNER);
        }
        break;
    case 2:
        if ((s8) em->holdTimer < 5) {
            em->holdTimer = 5;
        }
        if (em->remote == 0) {
            EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_TRAPPED_STRUGGLE, EM_SYNC_OWNER);
        }
        break;
    }
}

/*
 * BLANG_REACT_TRAPPED_STRUGGLE, _STATUS4_DOWNED_END and _STUN_DOWNED_END:
 * struggles in the hold (holdTimer kept at 5) until the trap's timer runs
 * out, then BLANG_IDLE_LEAVE_TRAP. An enraged Congalala makes effect 33 of
 * kind 7 in some loops.
 */
void EmBlangonga_TrappedStruggle(Em *em)
{
    u8 stateVar1;

    em->holdTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_STRUGGLE, 0, 0);
        em->noReact = 1;
        em->stateVar1 = 0;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            Em_SetMotion(em, BLANG_MOT_STRUGGLE, 0, 0);
            em->stateVar1++;
        }
        if ((em->type == EM_KIND_CONGALALA) && (em->rage != 0) && (Motion_PassedFrame(em, 0, 104.0f) != 0)) {
            stateVar1 = em->stateVar1;
            if ((stateVar1 & 7) == (((s32) em->stateRand >> (stateVar1 & 3)) & 7)) {
                EmEffectAttack_Create(em, EMFX_CONGALALA_RAGE_GAS, 0, 0, 0);
            }
        }
        if (--em->trapTimer <= 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_STRUGGLE, 0, 0);
            EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_LEAVE_TRAP, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_LEAVE_TRAP, EM_SYNC_OWNER_FLAGGED);
        }
        if (em->motion[0].playing == 0) {
            Em_SetMotion(em, BLANG_MOT_STRUGGLE, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_REACT_FALL: falls out of the air on curve 0 and lands within 180
 * (scaled) of the floor, lies 60 frames, gets up and ends the state.
 */
void EmBlangonga_Fall(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, BLANG_MOT_FALL, 0, 0);
        EmBlangonga_StartCurves(em, 0U);
        break;
    case 1:
        EmBlangonga_UpdateCurves(em);
        if (em->posY <= (em->floorY + (180.0f * em->scaleX))) {
            em->step++;
            em->posY = em->floorY;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, BLANG_MOT_FALL_LAND, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 0x3C;
            Em_SetMotion(em, BLANG_MOT_DOWN, 0, 0);
        }
        break;
    case 3:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_DOWN_GET_UP, 0, 0);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    }
}

/* BLANG_REACT_NONE: does nothing. */
void EmBlangonga_ReactNop(void)
{
}

/*
 * BLANG_REACT_STATUS4_DOWNED (variant 0) and _STUN_DOWNED (1): lies in the
 * hold until the status timer runs out, then the matching _DOWNED_END
 * state; variant 0 adds the periodic effect, variant 1 the stun's stage
 * object. Blangonga first comes up out of the pit (BLANG_MOT_EMERGE to
 * frame 14).
 */
void EmBlangonga_HeldDowned(Em *em, EmWork *work, u8 variant)
{
    u16 curMotion;

    em->holdTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        if (em->type == EM_KIND_BLANGONGA && (curMotion = em->motionId[0], curMotion != EM_MOTION_ID(0, BLANG_MOT_EMERGE)) && curMotion != EM_MOTION_ID(0, BLANG_MOT_STRUGGLE) && curMotion != EM_MOTION_ID(0, BLANG_MOT_DOWNED)) {
            Em_SetMotion(em, BLANG_MOT_EMERGE, 0, 0);
            em->stateArmor = 1;
            em->noPushTimer = 5;
        } else {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_DOWNED, 0, 0);
        }
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        switch (variant) {
        case 0:
            break;
        case 1:
            StunStars_CreateEm(em, 1, 0x11, (s32) ((f32) em->timer + (em->motion[0].frameCount / 2.0f)), 100.0f, 2.0f);
            break;
        }
        em->noReact = 1;
        break;
    case 1:
        if (em->motion[0].blend != 0) {
            em->stateArmor = 1;
            em->noPushTimer = 5;
        }
        if (EmMotion_ReachedFrame(em, 0, 14.0f)) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_DOWNED, 0, 0);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetTrapTimeGoal(em);
            switch (variant) {
            case 0:
            default:
                EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_STATUS4_DOWNED_END, EM_SYNC_OWNER);
                break;
            case 1:
                EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_STUN_DOWNED_END, EM_SYNC_OWNER);
                break;
            }
        }
        break;
    case 3:
        if (em->remote == 0) {
            switch (variant) {
            case 0:
            default:
                EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_STATUS4_DOWNED_END, EM_SYNC_OWNER);
                break;
            case 1:
                EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_STUN_DOWNED_END, EM_SYNC_OWNER);
                break;
            }
        }
        break;
    }
    if (variant != 1) {
        Em_SpawnStatus4Effect(em, 2);
    }
}

/*
 * BLANG_REACT_STATUS2_DOWNED_END and _STATUS1_DOWNED_END: blinded briefly,
 * struggles once, then BLANG_REACT_TRAPPED_STRUGGLE.
 */
void EmBlangonga_HeldStatusEnd(Em *em)
{
    em->holdTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, BLANG_MOT_STRUGGLE, 0, 0);
        em->noReact = 1;
        em->blind = 2;
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetTrapTimeGoal(em);
            em->blind = 0;
            EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_TRAPPED_STRUGGLE, EM_SYNC_OWNER);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_TRAPPED_STRUGGLE, EM_SYNC_OWNER);
        }
        break;
    }
}

/*
 * BLANG_REACT_HOLD2: held by the second hold (Em_SetHold2 every frame, root
 * motion off) until the kind timer runs out, then
 * BLANG_IDLE_HOLD2_SHAKE_OFF.
 */
void EmBlangonga_Hold2(Em *em)
{
    Em_SetHold2(em);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, BLANG_MOT_STATUS4, 0, 0);
        Player_SetFlag((Player *) em, EM_FLAG_NO_ROOT_MOTION);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_HOLD2_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_HOLD2_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * BLANG_REACT_DROP_CARRIED (variant 0) and _DROP_CARRIED_A..C (1-3):
 * Congalala drops its carried item; variants 1-3 leave it as timed mine
 * point variant - 1.
 */
void EmBlangonga_DropCarried(Em *em, EmWork *work, s32 variant)
{
    s32 k;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_DROP_CARRIED, 0, 0);
        Em_RestartScript(em);
        k = variant & 0xFF;
        if (k > 0) {
            EmMine_CreateTimed(em, &em->posX, k - 1);
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_REACT_FLINCH_ATTACK (Congalala): flinches and at frame 94 turns it
 * into the hitzone combo's two blows.
 */
void EmBlangonga_FlinchAttack(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_FLINCH_B, 0, 0x32);
        Em_RestartScript(em);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 94.0f, 0U) != 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_CONGALALA_HITZONE_COMBO, 0xC, 0x14);
        }
        break;
    case 2:
        if (EmMotion_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            BlangongaAttack_Create(em, 0x20);
            em->hitzoneSet = 1;
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_CONGALALA_HITZONE_COMBO_END, 0, 0);
            em->hitzoneSet = 0;
        }
        break;
    case 3:
        if (EmMotion_PassedFrameScaled(em, 88.0f, 0U) != 0) {
            BlangongaAttack_Create(em, 0x29);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_REACT_TRAP_ESCAPE: Rajang leaps out of the trap
 * (EmBlangonga_RajangJumpShot, variant 1); the other kinds just end the
 * state.
 */
void EmBlangonga_TrapEscape(Em *em, EmWork *work)
{
    EmBlangonga_RajangJumpShot(em, work, 0x1);
}

/*
 * Runs the current EM_GROUP_REACT state, passing the shared functions their
 * variant.
 */
void EmBlangonga_RunReactState(Em *em, EmWork *work)
{
    switch (em->state) {
    case BLANG_REACT_FLINCH:
        EmBlangonga_Flinch(em);
        break;
    case BLANG_REACT_FLINCH_B:
        EmBlangonga_FlinchB(em);
        break;
    case BLANG_REACT_KNOCK_DOWN:
        EmBlangonga_KnockDown(em, work, 0);
        break;
    case BLANG_REACT_KNOCK_DOWN_B:
        EmBlangonga_KnockDown(em, work, 1);
        break;
    case BLANG_REACT_STATUS4:
        EmBlangonga_Status4(em);
        break;
    case BLANG_REACT_TRAPPED:
        EmBlangonga_Trapped(em);
        break;
    case BLANG_REACT_TRAPPED_STRUGGLE:
        EmBlangonga_TrappedStruggle(em);
        break;
    case BLANG_REACT_FALL:
        EmBlangonga_Fall(em);
        break;
    case BLANG_REACT_NONE:
        EmBlangonga_ReactNop();
        break;
    case BLANG_REACT_STATUS4_DOWNED:
        EmBlangonga_HeldDowned(em, work, 0);
        break;
    case BLANG_REACT_STATUS4_DOWNED_END:
        EmBlangonga_TrappedStruggle(em);
        break;
    case BLANG_REACT_STATUS2_DOWNED_END:
        EmBlangonga_HeldStatusEnd(em);
        break;
    case BLANG_REACT_STUN:
        EmBlangonga_KnockDown(em, work, 2);
        break;
    case BLANG_REACT_STUN_B:
        EmBlangonga_KnockDown(em, work, 3);
        break;
    case BLANG_REACT_STUN_DOWNED:
        EmBlangonga_HeldDowned(em, work, 1);
        break;
    case BLANG_REACT_STUN_DOWNED_END:
        EmBlangonga_TrappedStruggle(em);
        break;
    case BLANG_REACT_HOLD2:
        EmBlangonga_Hold2(em);
        break;
    case BLANG_REACT_DROP_CARRIED:
        EmBlangonga_DropCarried(em, work, 0);
        break;
    case BLANG_REACT_FLINCH_ATTACK:
        EmBlangonga_FlinchAttack(em);
        break;
    case BLANG_REACT_STATUS1_DOWNED_END:
        EmBlangonga_HeldStatusEnd(em);
        break;
    case BLANG_REACT_PART_BREAK:
        EmBlangonga_FlinchB(em);
        break;
    case BLANG_REACT_DROP_CARRIED_A:
        EmBlangonga_DropCarried(em, work, 1);
        break;
    case BLANG_REACT_DROP_CARRIED_B:
        EmBlangonga_DropCarried(em, work, 2);
        break;
    case BLANG_REACT_DROP_CARRIED_C:
        EmBlangonga_DropCarried(em, work, 3);
        break;
    case BLANG_REACT_TRAP_ESCAPE:
        EmBlangonga_TrapEscape(em, work);
        break;
    }
}

/*
 * BLANG_DEFEAT_DIE: hunted (QuestEm_Hunted, the rage ended): falls dead and
 * leaves a carcass mine point (the large kinds for 1800 frames once the
 * quest lets them appear again, the small ones for 1350 frames or until
 * carved), fades out over 60 frames, then BLANG_DEFEAT_REAPPEAR.
 */
void EmBlangonga_Die(Em *em, s32 unused)
{
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        if (EmBlangonga_IsSmallKind(em) == 1) {
            em->step = 2;
        } else {
            em->step++;
        }
        QuestEm_Hunted(em);
        Em_EndRage(em);
        Em_SetMotion(em, BLANG_MOT_DIE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_DIE_END, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->fieldCollide = 0;
            if (EmBlangonga_IsSmallKind(em) == 1) {
                em->timer = 0x546;
            } else {
                em->timer = 0x708;
            }
            EmMine_Create(em, 0);
        }
        break;
    case 3:
        EmMine_Update(em);
        if (EmBlangonga_IsSmallKind(em) == 1) {
            if (EmMine_Update(em) <= 0 || --em->timer <= 0) {
                em->step++;
                EmMine_Remove(em);
            }
        } else if (QuestEm_CanAppear(em) && --em->timer <= 0) {
            em->step++;
            em->timer = 150;
            EmMine_Remove(em);
        }
        break;
    case 4:
        if ((em->alpha -= 0.016666668f) <= 0.0f) {
            em->visible = 0;
            em->alpha = 0.0f;
            em->step++;
        }
        break;
    case 5:
        if (em->remote == 0) {
            em->visible = 0;
            EmBlangonga_ChangeState(em, EM_GROUP_DEFEAT, BLANG_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * BLANG_DEFEAT_DIE_HELD: as EmBlangonga_Die, lying in the hold (holdTimer
 * kept at 5).
 */
void EmBlangonga_DieHeld(Em *em, s32 unused)
{
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, BLANG_MOT_DOWNED, 0, 0);
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_DEAD;
        QuestEm_Hunted(em);
        Em_EndRage(em);
        em->holdTimer = 5;
        break;
    case 1:
        em->holdTimer = 5;
        if (em->motion[0].playing == 0) {
            em->step++;
            EmMine_Create(em, 0);
            em->fieldCollide = 0;
            if (EmBlangonga_IsSmallKind(em) == 1) {
                em->timer = 0x546;
            } else {
                em->timer = 0x708;
            }
        }
        break;
    case 2:
        EmMine_Update(em);
        em->holdTimer = 5;
        if ((EmBlangonga_IsSmallKind(em) || QuestEm_CanAppear(em)) && --em->timer <= 0) {
            em->step++;
            EmMine_Remove(em);
        }
        break;
    case 3:
        if ((em->alpha -= 0.016666668f) <= 0.0f) {
            em->visible = 0;
            em->alpha = 0.0f;
            em->step++;
        }
        break;
    case 4:
        if (em->remote == 0) {
            em->visible = 0;
            EmBlangonga_ChangeState(em, EM_GROUP_DEFEAT, BLANG_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * BLANG_DEFEAT_DIE_ARMORED: as EmBlangonga_Die for a monster under
 * stateArmor: comes up (BLANG_MOT_EMERGE to frame 14) and lies down.
 */
void EmBlangonga_DieArmored(Em *em, s32 unused)
{
    f32 alpha;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->stateArmor = 1;
        em->noPushTimer = 5;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, BLANG_MOT_EMERGE, 0, 0);
        QuestEm_Hunted(em);
        Em_EndRage(em);
        em->noReact = 1;
        break;
    case 1:
        if (em->motion[0].blend != 0) {
            em->stateArmor = 1;
            em->noPushTimer = 5;
        }
        if (EmMotion_PassedFrameScaled(em, 14.0f, 0U) != 0) {
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, BLANG_MOT_DOWNED, 0, 0);
            em->step++;
            break;
        }
    default:
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmMine_Create(em, 0);
            em->fieldCollide = 0;
            if (EmBlangonga_IsSmallKind(em) == 1) {
                em->timer = 0x546;
                break;
            }
            em->timer = 0x708;
        }
        break;
    case 3:
        EmMine_Update(em);
        if ((EmBlangonga_IsSmallKind(em) != 0) || (QuestEm_CanAppear(em) != 0)) {
            if (--em->timer <= 0) {
                em->step++;
                EmMine_Remove(em);
                break;
            }
        }
        break;
    case 4:
        alpha = em->alpha - 0.016666668f;
        em->alpha = alpha;
        if (alpha <= 0.0f) {
            em->visible = 0;
            em->alpha = 0.0f;
            em->step++;
        }
        break;
    case 5:
        if (em->remote == 0) {
            em->visible = 0;
            EmBlangonga_ChangeState(em, EM_GROUP_DEFEAT, BLANG_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * BLANG_DEFEAT_DIE_FALLING: as EmBlangonga_Die for a monster in the air:
 * falls on curve 0 and lands dead within 180 of the floor.
 */
void EmBlangonga_DieFalling(Em *em, s32 unused)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, BLANG_MOT_FALL, 0, 0);
        QuestEm_Hunted(em);
        Em_EndRage(em);
        EmBlangonga_StartCurves(em, 0);
        break;
    case 1:
        EmBlangonga_UpdateCurves(em);
        if (em->posY <= 180.0f + em->floorY) {
            em->step++;
            em->posY = em->floorY;
            em->stance = EM_STANCE_DEAD;
            Em_SetMotion(em, BLANG_MOT_FALL_LAND, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->fieldCollide = 0;
            if (EmBlangonga_IsSmallKind(em) == 1) {
                em->timer = 0x546;
            } else {
                em->timer = 0x708;
            }
            EmMine_Create(em, 0);
        }
        break;
    case 3:
        EmMine_Update(em);
        if ((EmBlangonga_IsSmallKind(em) || QuestEm_CanAppear(em)) && --em->timer <= 0) {
            em->step++;
            EmMine_Remove(em);
        }
        break;
    case 4:
        if ((em->alpha -= 0.016666668f) <= 0.0f) {
            em->visible = 0;
            em->alpha = 0.0f;
            em->step++;
        }
        break;
    case 5:
        if (em->remote == 0) {
            em->visible = 0;
            EmBlangonga_ChangeState(em, EM_GROUP_DEFEAT, BLANG_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * BLANG_DEFEAT_REAPPEAR: when the quest lets the monster appear again, a
 * Blango of spawnParam 1 hides under the snow again, the other small kinds
 * go to BLANG_IDLE and the large ones drop in from above
 * (BLANG_LEAP_DROP_IN); otherwise the monster's task is finished
 * (taskPhase). EmBlangonga_Start tests Em.variant for the same hidden
 * start: QuestEm_Spawn copies the two from different fields of the quest's
 * monster entry, so the two tests are the game's own.
 */
void EmBlangonga_Reappear(Em *em, s32 unused)
{
    switch (em->step) {
    case 0:
        if (QuestEm_CanAppear(em) == 1) {
            if (EmBlangonga_IsSmallKind(em) == 1) {
                if ((em->type == EM_KIND_BLANGO) && (em->spawnParam == 1)) {
                    em->stance = EM_STANCE_GROUND;
                    Em_SetMotionRow(em, BLANG_MOT_BURIED, 0, 0);
                    EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_BURIED, EM_SYNC_LOCAL);
                    break;
                }
                EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE, EM_SYNC_LOCAL);
                break;
            }
            em->posY = gAreaGroundY[em->area];
            em->stance = EM_STANCE_AIR;
            EmBlangonga_ChangeState(em, EM_GROUP_FLY, BLANG_LEAP_DROP_IN, EM_SYNC_LOCAL);
            break;
        }
        em->taskPhase++;
        break;
    }
}

/*
 * Runs the current EM_GROUP_DEFEAT state, unpushable (and the small kinds
 * unhittable) meanwhile.
 */
void EmBlangonga_RunDefeatState(Em *em, s32 work)
{
    if (EmBlangonga_IsSmallKind(em) == 1) {
        em->noHitTimer = 0xA;
    }
    em->noPushTimer = 0xA;
    switch (em->state) {
    case BLANG_DEFEAT_DIE:
        EmBlangonga_Die(em, work);
        break;
    case BLANG_DEFEAT_DIE_HELD:
        EmBlangonga_DieHeld(em, work);
        break;
    case BLANG_DEFEAT_DIE_ARMORED:
        EmBlangonga_DieArmored(em, work);
        break;
    case BLANG_DEFEAT_DIE_FALLING:
        EmBlangonga_DieFalling(em, work);
        break;
    case BLANG_DEFEAT_REAPPEAR:
        EmBlangonga_Reappear(em, work);
        break;
    }
}

/*
 * BLANG_EVENT_CAPTURED: captured in holdTimer's hold (Em_SetCaptured 0,
 * status 1, poison ended): lies downed (Blangonga first comes up out of the
 * pit), QuestEm_Captured, a sound every 135 frames and the periodic effect;
 * 1800 frames once the quest lets it appear again, then a 150-frame fade
 * and BLANG_DEFEAT_REAPPEAR.
 */
void EmBlangonga_Captured(Em *em)
{
    s32 timer;
    s32 prevTimer;
    u16 motion;

    Em_SetCaptured(em, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        if ((em->type == EM_KIND_BLANGONGA) && (motion = em->motionId[0], (motion != EM_MOTION_ID(0, BLANG_MOT_EMERGE))) && (motion != EM_MOTION_ID(0, BLANG_MOT_STRUGGLE)) && (motion != EM_MOTION_ID(0, BLANG_MOT_DOWNED))) {
            Em_SetMotion(em, BLANG_MOT_EMERGE, 0, 0);
            em->stateArmor = 1;
            em->noPushTimer = 5;
        } else {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_DOWNED, 0, 0);
            QuestEm_Captured(em);
            em->timer = 0x708;
        }
        em->lookAtTarget = 0;
        Em_BeginStatus1(em);
        Em_ClearPoison(em);
        break;
    case 1:
        if (em->motion[0].blend != 0) {
            em->stateArmor = 1;
            em->noPushTimer = 5;
        }
        if (EmMotion_ReachedFrame(em, 0, 14.0f) != 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_DOWNED, 0, 0);
            QuestEm_Captured(em);
            em->timer = 0x708;
        }
        break;
    case 2:
        if (((s32) em->timer % 135) == 0) {
            Em_PlaySeAtBone(em, 0x52, 0x13);
        }
        if (em->type == EM_KIND_CONGALALA) {
            EmBlangonga_SpawnPeriodicEffect06(em, 1);
        } else {
            EmBlangonga_SpawnPeriodicEffect06(em, 0);
        }
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
        timer = em->timer;
        if (timer <= 0) {
            if (em->remote == 0) {
                EmBlangonga_ChangeState(em, EM_GROUP_DEFEAT, BLANG_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
                break;
            }
        } else {
            em->alpha = (f32) timer / 150.0f;
        }
        break;
    }
}

/*
 * BLANG_EVENT_CAPTURED_HOLD2: as EmBlangonga_Captured in the second hold
 * (Em_SetCaptured 1), collapsing and lying instead, without the sound.
 */
void EmBlangonga_CapturedHold2(Em *em)
{
    s32 timer;
    s32 prevTimer;

    Em_SetCaptured(em, 1);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_COLLAPSE, 0, 0);
        Em_BeginStatus1(em);
        Em_ClearPoison(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_LIE, 0, 0);
            QuestEm_Captured(em);
            em->timer = 0x708;
        }
        break;
    case 2:
        EmBlangonga_SpawnPeriodicEffect06(em, 0);
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
        timer = em->timer;
        if (timer <= 0) {
            if (em->remote == 0) {
                EmBlangonga_ChangeState(em, EM_GROUP_DEFEAT, BLANG_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
                break;
            }
        } else {
            em->alpha = (f32) timer / 150.0f;
        }
        break;
    }
}

/*
 * BLANG_EVENT_CONGALALA_SCENE: Congalala's story scene at a fixed point:
 * idles 180 frames, eats twice and roars, then idles until EVENT_FLAG_CONGALALA_SCENE
 * is set; senseDelay and noHitTimer are kept up meanwhile.
 */
void EmBlangonga_CongalalaScene(Em *em, EmWork *work)
{
    EmBlangongaWork *w = (EmBlangongaWork *)work;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        GmEm_ResetSize(em);
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->posX = 12212.0f;
        em->posY = 0.0f;
        em->posZ = 9484.0f;
        em->rotX = 0;
        em->rotY = 0x4000;
        em->rotZ = 0;
        Em_SetMotion(em, BLANG_MOT_IDLE, 0, 0);
        em->timer = 180;
        w->sceneEating = 0;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_EAT, 0, 0);
        }
        break;
    case 2:
        if (EmMotion_InFrameRange(em, 0, 4.0f, 26.0f)) {
            em->rotY += (u32) (1638.4f * em->motionSpeed);
        }
        if (w->sceneEating == 0 && EmMotion_ReachedFrame(em, 0, 34.0f)) {
            w->sceneEating = 1;
        }
        if (EmMotion_PassedFrameScaled(em, 300.0f, 0)) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_EAT, 0, 0x96);
        }
        break;
    case 3:
        if (EmMotion_PassedFrameScaled(em, 250.0f, 0)) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_BACK_STRIKE, 0, 0);
            w->sceneEating = 0;
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_ROAR, 0, 0);
        }
        break;
    case 5:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_IDLE, 0, 0);
        }
        break;
    case 6:
        if (Save_TestEventFlag(EVENT_FLAG_CONGALALA_SCENE) == 1) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_EVENT_BLANGONGA_SCENE: hands each monster its part of Blangonga's
 * story scene: Blangonga leaps down (BLANG_EVENT_BLANGONGA_LEAP_DOWN), the
 * first three Blangos in gEms emerge (_EMERGE_A..C) and the rest stay
 * hidden.
 */
void EmBlangonga_StartBlangongaScene(Em *em, EmWork *work)
{
    s16 i;
    u8 n = 0;
    Em *other;

    if (em->type == EM_KIND_BLANGONGA) {
        EmBlangonga_ChangeState(em, EM_GROUP_EVENT, BLANG_EVENT_BLANGONGA_LEAP_DOWN, EM_SYNC_LOCAL);
        return;
    }
    for (i = 0, other = gEms; i < 20; i++, other++) {
        if (other->type == 0x3D) {
            if (em->slotNo == other->slotNo) {
                switch (n) {
                case 0:
                    EmBlangonga_ChangeState(em, EM_GROUP_EVENT, BLANG_EVENT_BLANGO_EMERGE_A, EM_SYNC_LOCAL);
                    return;
                case 1:
                    EmBlangonga_ChangeState(em, EM_GROUP_EVENT, BLANG_EVENT_BLANGO_EMERGE_B, EM_SYNC_LOCAL);
                    return;
                case 2:
                    EmBlangonga_ChangeState(em, EM_GROUP_EVENT, BLANG_EVENT_BLANGO_EMERGE_C, EM_SYNC_LOCAL);
                    return;
                default:
                    EmBlangonga_ChangeState(em, EM_GROUP_EVENT, BLANG_EVENT_BLANGO_HIDDEN, EM_SYNC_LOCAL);
                    return;
                }
            }
            n++;
        }
    }
}

/*
 * BLANG_EVENT_BLANGONGA_LEAP_DOWN: waits on a high point, roars, jumps down
 * and lands on the floor (frame 70 of the fall), walks to its goal (1200
 * frames or within 100), then idles until EVENT_FLAG_BLANGONGA_SCENE.
 */
void EmBlangonga_BlangongaLeapDown(Em *em, EmWork *work)
{
    f32 goalDist;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        GmEm_ResetSize(em);
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->posX = 10747.0f;
        em->posY = 2468.0f;
        em->posZ = 8181.0f;
        Vec_Copy(&em->prevPosX, &em->posX);
        em->rotX = 0;
        em->rotY = 0xFC00;
        em->rotZ = 0;
        Em_SetMotion(em, BLANG_MOT_IDLE, 0, 0);
        em->timer = 300;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_ROAR_B, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 190;
            Em_SetMotion(em, BLANG_MOT_IDLE, 0, 0);
        }
        break;
    case 3:
        if (--em->timer <= 0) {
            em->step++;
            em->stance = EM_STANCE_AIR;
            Em_SetMotion(em, BLANG_MOT_MOVE_ACTION_D, 0, 0);
        }
        break;
    case 4:
        if (EmMotion_PassedFrameScaled(em, 26.0f, 0)) {
            em->step++;
            Player_ClearVelocity(em);
            Em_SetMotion(em, BLANG_MOT_JUMP_FALL, 0, 0x32);
        }
        break;
    case 5:
        if (EmMotion_PassedFrameScaled(em, 70.0f, 0)) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posX = 10682.0f;
            em->posY = 288.0f;
            em->posZ = 9620.0f;
            em->goal[0] = 10682.0f;
            em->goal[1] = 288.0f;
            em->goal[2] = 12400.0f;
            em->rotX = 0;
            em->rotY = Math_AngleXZ(&em->posX, em->goal);
            em->rotZ = 0;
            Em_SetMotion(em, BLANG_MOT_JUMP_LAND, 0, 0);
        }
        break;
    case 6:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 1200;
            Em_SetMotion(em, BLANG_MOT_WALK, 0, 0);
        }
        break;
    case 7:
        goalDist = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer <= 0 || goalDist <= 100.0f) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_IDLE, 0, 0);
        }
        break;
    case 8:
        if (Save_TestEventFlag(EVENT_FLAG_BLANGONGA_SCENE) == 1) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_EVENT_BLANGO_EMERGE_A..C (variants 0-2): waits buried (stateArmor)
 * at the variant's place and facing for 440, 480 or 510 frames, comes out,
 * then idles and roars in turn until EVENT_FLAG_BLANGONGA_SCENE.
 */
void EmBlangonga_BlangoEmerge(Em *em, EmWork *work, u8 variant)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        GmEm_ResetSize(em);
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->stateArmor = 1;
        Em_SetMotion(em, BLANG_MOT_BURIED, 0, 0);
        switch (variant) {
        default:
            em->posX = 11000.0f;
            em->posY = 288.0f;
            em->posZ = 11500.0f;
            em->rotX = 0;
            em->rotY = 0x1000;
            em->rotZ = 0;
            em->timer = 440;
            break;
        case 1:
            em->posX = 10000.0f;
            em->posY = 288.0f;
            em->posZ = 10380.0f;
            em->rotX = 0;
            em->rotY = 0x1400;
            em->rotZ = 0;
            em->timer = 480;
            break;
        case 2:
            em->posX = 9900.0f;
            em->posY = 288.0f;
            em->posZ = 11819.0f;
            em->rotX = 0;
            em->rotY = 0x2000;
            em->rotZ = 0;
            em->timer = 510;
            break;
        }
        break;
    case 1:
        em->stateArmor = 1;
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_EMERGE, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_ALERT_IDLE, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stateVar1 = 0;
            Em_SetMotion(em, BLANG_MOT_ROAR, 0, 0);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            if (em->stateVar1 == 0) {
                em->stateVar1 = 1;
                Em_SetMotion(em, BLANG_MOT_ALERT_IDLE, 0, 0);
            } else {
                em->stateVar1 = 0;
                Em_SetMotion(em, BLANG_MOT_ROAR, 0, 0);
            }
        }
        if (Save_TestEventFlag(EVENT_FLAG_BLANGONGA_SCENE) == 1) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_EVENT_BLANGO_HIDDEN: stays invisible until EVENT_FLAG_BLANGONGA_SCENE, then
 * shows itself and ends the state.
 */
void EmBlangonga_BlangoHidden(Em *em)
{
    switch (em->step) {
    case 0:
        GmEm_ResetSize(em);
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_IDLE, 0, 0);
        em->visible = 0;
        em->scriptPending = 0;
        break;
    case 1:
        if (Save_TestEventFlag(EVENT_FLAG_BLANGONGA_SCENE) == 1) {
            em->step++;
            em->visible = 1;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/* Runs the current EM_GROUP_EVENT (or EVENT2) state. */
void EmBlangonga_RunEventState(Em *em, EmWork *work)
{
    switch (em->state) {
    case BLANG_EVENT_CAPTURED:
        EmBlangonga_Captured(em);
        break;
    case BLANG_EVENT_CAPTURED_HOLD2:
        EmBlangonga_CapturedHold2(em);
        break;
    case BLANG_EVENT_CONGALALA_SCENE:
        EmBlangonga_CongalalaScene(em, work);
        break;
    case BLANG_EVENT_BLANGONGA_SCENE:
        EmBlangonga_StartBlangongaScene(em, work);
        break;
    case BLANG_EVENT_BLANGONGA_LEAP_DOWN:
        EmBlangonga_BlangongaLeapDown(em, work);
        break;
    case BLANG_EVENT_BLANGO_EMERGE_A:
        EmBlangonga_BlangoEmerge(em, work, 0);
        break;
    case BLANG_EVENT_BLANGO_EMERGE_B:
        EmBlangonga_BlangoEmerge(em, work, 1);
        break;
    case BLANG_EVENT_BLANGO_EMERGE_C:
        EmBlangonga_BlangoEmerge(em, work, 2);
        break;
    case BLANG_EVENT_BLANGO_HIDDEN:
        EmBlangonga_BlangoHidden(em);
        break;
    }
}

/*
 * BLANG_VS_EAT_BAIT: after the defeat camera (Versus_DefeatCamera) the bait
 * comes from the winner's side (stage object 0x25) and the monster eats it
 * (Congalala with its own motion), then Versus_RouteEmByTeam.
 */
void EmBlangonga_VersusEatBait(Em *em, EmWork *work)
{
    Vec3f throwPoint;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        if (Versus_DefeatCamera(em) == 1) {
            throwPoint = EmBlangonga_CarveThrowPt;
            VersusBait_Create(&gPlayers[(u8) em->side - 1], em, 0, 0, &throwPoint.x);
            em->step++;
            if (em->type == EM_KIND_CONGALALA) {
                em->step++;
                Em_SetMotion(em, BLANG_MOT_CONGALALA_EAT, 0, 0);
            } else {
                Em_SetMotion(em, BLANG_MOT_EAT, 0, 0);
            }
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_EAT_END, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            Versus_RouteEmByTeam(em);
        }
        break;
    }
}

/*
 * BLANG_VS_POSE_A: one pose by kind (Blangonga, Congalala, the others),
 * then idles and BLANG_VS_WAIT_BOTH.
 */
void EmBlangonga_VersusPoseA(Em *em, EmWork *work)
{
    u8 blangongaStep;
    u8 congalalaStep;
    u8 otherStep;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->type) {
    case EM_KIND_BLANGONGA:
        blangongaStep = em->step;
        switch (blangongaStep) {
        case 0:
            em->step = blangongaStep + 1;
            Em_SetMotion(em, BLANG_MOT_IDLE_ACTION_C, 0, 0);
            break;
        case 1:
            if (em->motion[0].playing == 0) {
                em->step = blangongaStep + 1;
                Em_SetMotion(em, BLANG_MOT_IDLE, 0, 0);
                EmBlangonga_ChangeState(em, EM_GROUP_VERSUS, BLANG_VS_WAIT_BOTH, EM_SYNC_SEND);
            }
            break;
        }
        break;
    case 0x34:
        congalalaStep = em->step;
        switch (congalalaStep) {
        case 0:
            em->step = congalalaStep + 1;
            Em_SetMotion(em, BLANG_MOT_FLINCH_B, 0, 0);
            break;
        case 1:
            if (em->motion[0].playing == 0) {
                em->step = congalalaStep + 1;
                Em_SetMotion(em, BLANG_MOT_IDLE, 0, 0);
                EmBlangonga_ChangeState(em, EM_GROUP_VERSUS, BLANG_VS_WAIT_BOTH, EM_SYNC_SEND);
            }
            break;
        }
        break;
    default:
        otherStep = em->step;
        switch (otherStep) {
        case 0:
            em->step = otherStep + 1;
            Em_SetMotion(em, BLANG_MOT_FLINCH, 0, 0);
            break;
        case 1:
            if (em->motion[0].playing == 0) {
                em->step = otherStep + 1;
                Em_SetMotion(em, BLANG_MOT_IDLE, 0, 0);
                EmBlangonga_ChangeState(em, EM_GROUP_VERSUS, BLANG_VS_WAIT_BOTH, EM_SYNC_SEND);
            }
        }
        break;
    }
}

/* BLANG_VS_POSE_B: as BLANG_VS_POSE_A with the second poses. */
void EmBlangonga_VersusPoseB(Em *em, EmWork *work)
{
    u8 blangongaStep;
    u8 otherStep;
    u8 congalalaStep;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->type) {
    case EM_KIND_BLANGONGA:
        blangongaStep = em->step;
        switch (blangongaStep) {
        case 0:
            em->step = blangongaStep + 1;
            Em_SetMotion(em, BLANG_MOT_ROAR_B, 0, 0);
            break;
        case 1:
            if (em->motion[0].playing == 0) {
                em->step = blangongaStep + 1;
                Em_SetMotion(em, BLANG_MOT_IDLE, 0, 0);
                EmBlangonga_ChangeState(em, EM_GROUP_VERSUS, BLANG_VS_WAIT_BOTH, EM_SYNC_SEND);
            }
            break;
        }
        break;
    case 0x34:
        congalalaStep = em->step;
        switch (congalalaStep) {
        case 0:
            em->step = congalalaStep + 1;
            Em_SetMotion(em, BLANG_MOT_ROAR, 0, 0);
            break;
        case 1:
            if (em->motion[0].playing == 0) {
                em->step = congalalaStep + 1;
                Em_SetMotion(em, BLANG_MOT_IDLE, 0, 0);
                EmBlangonga_ChangeState(em, EM_GROUP_VERSUS, BLANG_VS_WAIT_BOTH, EM_SYNC_SEND);
            }
            break;
        }
        break;
    default:
        otherStep = em->step;
        switch (otherStep) {
        case 0:
            em->step = otherStep + 1;
            Em_SetMotion(em, BLANG_MOT_ROAR_B, 0, 0);
            break;
        case 1:
            if (em->motion[0].playing == 0) {
                em->step = otherStep + 1;
                Em_SetMotion(em, BLANG_MOT_IDLE, 0, 0);
                EmBlangonga_ChangeState(em, EM_GROUP_VERSUS, BLANG_VS_WAIT_BOTH, EM_SYNC_SEND);
            }
        }
        break;
    }
}

/*
 * BLANG_VS_POSE_C: as BLANG_VS_POSE_A with the third poses (Congalala's in
 * two motions).
 */
void EmBlangonga_VersusPoseC(Em *em, EmWork *work)
{
    u8 congalalaStep;
    u8 blangongaStep;
    u8 otherStep;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->type) {
    case EM_KIND_BLANGONGA:
        blangongaStep = em->step;
        switch (blangongaStep) {
        case 0:
            em->step = blangongaStep + 1;
            Em_SetMotion(em, BLANG_MOT_ROAR, 0, 0);
            break;
        case 1:
            if (em->motion[0].playing == 0) {
                em->step = blangongaStep + 1;
                Em_SetMotion(em, BLANG_MOT_IDLE, 0, 0);
                EmBlangonga_ChangeState(em, EM_GROUP_VERSUS, BLANG_VS_WAIT_BOTH, EM_SYNC_SEND);
            }
            break;
        }
        break;
    case 0x34:
        congalalaStep = em->step;
        switch (congalalaStep) {
        case 0:
            em->step = congalalaStep + 1;
            Em_SetMotion(em, BLANG_MOT_CONGALALA_HITZONE_COMBO, 0, 0);
            break;
        case 1:
            if (em->motion[0].playing == 0) {
                em->step = congalalaStep + 1;
                Em_SetMotion(em, BLANG_MOT_CONGALALA_HITZONE_COMBO_END, 0, 0);
            }
            break;
        case 2:
            if (em->motion[0].playing == 0) {
                em->step = congalalaStep + 1;
                Em_SetMotion(em, BLANG_MOT_IDLE, 0, 0);
                EmBlangonga_ChangeState(em, EM_GROUP_VERSUS, BLANG_VS_WAIT_BOTH, EM_SYNC_SEND);
            }
            break;
        }
        break;
    default:
        otherStep = em->step;
        switch (otherStep) {
        case 0:
            em->step = otherStep + 1;
            Em_SetMotion(em, BLANG_MOT_ROAR, 0, 0);
            break;
        case 1:
            if (em->motion[0].playing == 0) {
                em->step = otherStep + 1;
                Em_SetMotion(em, BLANG_MOT_IDLE, 0, 0);
                EmBlangonga_ChangeState(em, EM_GROUP_VERSUS, BLANG_VS_WAIT_BOTH, EM_SYNC_SEND);
            }
        }
        break;
    }
}

/*
 * BLANG_VS_WAIT_BOTH: idles until both sides are defeated
 * (Versus_BothDefeated), then BLANG_VS_END. Step 0 falls into step 1 to
 * test on the same frame.
 */
void EmBlangonga_VersusWaitBoth(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        if (em->motionId[0] != EM_MOTION_ID(0, BLANG_MOT_IDLE)) {
            Em_SetMotion(em, BLANG_MOT_IDLE, 0, 0);
        }
        
    case 1:
        if (Versus_BothDefeated() == 1) {
            em->step++;
            EmBlangonga_ChangeState(em, EM_GROUP_VERSUS, BLANG_VS_END, EM_SYNC_SEND);
        }
        break;
    }
}

/*
 * BLANG_VS_END: idles 90 frames, lets the monster be sensed and hit again,
 * and ends the state.
 */
void EmBlangonga_VersusEnd(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 0x5A;
        if (em->motionId[0] != EM_MOTION_ID(0, BLANG_MOT_IDLE)) {
            Em_SetMotion(em, BLANG_MOT_IDLE, 0, 0);
        }
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            em->senseDelay = 0;
            em->noHitTimer = 0;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/* Runs the current EM_GROUP_VERSUS state. */
void EmBlangonga_RunVersusState(Em *em, EmWork *work)
{
    switch (em->state) {
    case BLANG_VS_EAT_BAIT:
        EmBlangonga_VersusEatBait(em, work);
        break;
    case BLANG_VS_POSE_A:
        EmBlangonga_VersusPoseA(em, work);
        break;
    case BLANG_VS_POSE_B:
        EmBlangonga_VersusPoseB(em, work);
        break;
    case BLANG_VS_POSE_C:
        EmBlangonga_VersusPoseC(em, work);
        break;
    case BLANG_VS_WAIT_BOTH:
        EmBlangonga_VersusWaitBoth(em, work);
        break;
    case BLANG_VS_END:
        EmBlangonga_VersusEnd(em, work);
        break;
    }
}

/*
 * The family's frame: the rage timer, visibility and combat timer; the
 * Blango's sensing under the snow (EM_TRIGGER_SENSED); the needs' decay by
 * map and kind; then Em_UpdateDamage's result picks a reaction
 * (EM_GROUP_REACT), a death or a capture, or a status state; Congalala's
 * and Blangonga's story quests start their scenes; then the state runs
 * (twice when stateRequest is set), and stateArmor keeps the shadow hidden
 * and the guard up.
 */
void EmBlangonga_Update(Em *em)
{
    EmBlangongaWork *w = (EmBlangongaWork *)&em->work;
    u8 savedSenseDelay;
    s16 partStateSum;
    s32 result;
    u8 mapBase;
    s32 sensed;
    s32 i;
    u8 damageOut[4];

    EmBlangonga_UpdateRageTimer(em);
    if (em->stateGroup <= EM_GROUP_REACT) {
        em->alpha = 1.0f;
        em->visible = 1;
    }
    if (em->senseDelay != 0) {
        em->noHitTimer = 5;
    }
    if (w->hitSeCooldown > 0) {
        w->hitSeCooldown--;
    }
    Em_UpdateCombatTimer(em);
    if (EmBlangonga_IsSmallKind(em) == 0) {
        Em_RaisePlayerAreaTrigger(em);
    }
    if ((em->stateGroup != EM_GROUP_FLY || em->state != BLANG_LEAP_OUT_OF_AREA) && (em->stateGroup != EM_GROUP_FLY || em->state != BLANG_LEAP_DROP_IN) &&
        (em->stateGroup != EM_GROUP_FLY || em->state != BLANG_LEAP_CROSS_AREA)) {
        Em_HoldPosNoPlayers(em, 0, 1);
    }
    if (EmBlangonga_ShouldEndState(em) == 1) {
        EmBlangonga_EndState(em, 0, 0);
    }
    if ((s16) Player_IsState(em, EM_GROUP_IDLE, BLANG_IDLE_BURIED)) {
        savedSenseDelay = em->senseDelay;
        em->senseDelay = 0;
    }
    if (em->type == EM_KIND_BLANGO) {
        sensed = Em_SenseObjects(em, &w->sense) & 0xFF;
        if (em->senseDelay == 0 && em->noReact == 0 && sensed != 0 && Em_ScriptFlagsFrom(em, 4) == 0) {
            em->triggers |= EM_TRIGGER_SENSED;
            em->scriptPending = 1;
        } else if ((s16) Player_IsState(em, EM_GROUP_IDLE, BLANG_IDLE_BURIED)) {
            em->senseDelay = savedSenseDelay;
        }
    }
    if (em->noAlert != 1 && em->rage == 0) {
        mapBase = Map_DayId(gScene.map);
        switch (em->type) {
        case EM_KIND_BLANGONGA:
            switch (mapBase) {
            case MAP_SNOWY_MOUNTAINS:
                Em_DecayNeed1Only(em);
                Em_CheckLowHp(em, 0.3f);
                break;
            }
            break;
        case 0x34:
            switch (mapBase) {
            case MAP_JUNGLE:
                Em_DecayNeed1(em);
                Em_DecayNeed2(em);
                Em_CheckLowHp(em, 0.2f);
                break;
            case MAP_SWAMP:
                Em_DecayNeed1(em);
                Em_CheckLowHp(em, 0.2f);
                break;
            case MAP_GREAT_ARENA:
            case MAP_ARENA:
                Em_DecayNeed1Only(em);
                break;
            }
            break;
        }
    }
    result = Em_UpdateDamage(em, damageOut) & 0xFF;
    switch (result) {
    case EM_DMG_NONE:
    case EM_DMG_FLASH:
    case EM_DMG_HIT_MODE3:
    case EM_DMG_ATTACK5:
    case EM_DMG_ATTACK5_FLINCH:
        break;
    case EM_DMG_KILLED:
    case EM_DMG_DEAD:
        if ((s8) em->holdTimer != 0) {
            EmBlangonga_ChangeState(em, EM_GROUP_DEFEAT, BLANG_DEFEAT_DIE_HELD, EM_SYNC_OWNER);
        } else if (EmBlangonga_InStateArmor(em) == 1) {
            EmBlangonga_ChangeState(em, EM_GROUP_DEFEAT, BLANG_DEFEAT_DIE_ARMORED, EM_SYNC_OWNER);
        } else {
            switch (em->stance) {
            case EM_STANCE_AIR:
                EmBlangonga_ChangeState(em, EM_GROUP_DEFEAT, BLANG_DEFEAT_DIE_FALLING, EM_SYNC_OWNER);
                break;
            default:
                EmBlangonga_ChangeState(em, EM_GROUP_DEFEAT, BLANG_DEFEAT_DIE, EM_SYNC_OWNER);
                break;
            }
        }
        break;
    case EM_DMG_ATTACK9:
    case EM_DMG_ATTACK6:
        if ((s8) em->holdTimer == 0) {
            if (EmBlangonga_IsRajang(em) == 1 && em->rage == 0 && em->status == EM_STATUS_NONE) {
                EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_TRAP_ESCAPE, EM_SYNC_OWNER);
            } else {
                Em_SetTrapTime(em, damageOut);
                Em_SetTrapTimeGoal(em);
                EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_TRAPPED, EM_SYNC_OWNER);
            }
        }
        break;
    case EM_DMG_STATUS_IN_AIR:
        if (em->stateGroup != EM_GROUP_REACT || em->state != BLANG_REACT_FALL) {
            EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_FALL, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_STATUS4:
        if (em->stateGroup != EM_GROUP_REACT || em->state != BLANG_REACT_STATUS4_DOWNED) {
            if ((s8) em->holdTimer != 0) {
                Em_SetStatus4Timer(em);
                EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_STATUS4_DOWNED, EM_SYNC_OWNER);
            } else if ((em->stateGroup != EM_GROUP_REACT || em->state != BLANG_REACT_STATUS4) && (em->stateGroup != EM_GROUP_REACT || em->state != BLANG_REACT_FALL)) {
                Em_SetStatus4Timer(em);
                EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_STATUS4, EM_SYNC_OWNER);
            }
        }
        break;
    case EM_DMG_CAPTURED:
        if (Em_IsHeld(em) == 1) {
            Em_SetStatus1Timer(em);
            if (EmBlangonga_IsBlangonga(em) != 1) {
                EmBlangonga_IsCongalala(em);
            }
            if ((s8) em->holdTimer != 0) {
                EmBlangonga_ChangeState(em, EM_GROUP_EVENT, BLANG_EVENT_CAPTURED, EM_SYNC_OWNER);
                break;
            }
            if ((s8) em->holdTimer2 != 0) {
                EmBlangonga_ChangeState(em, EM_GROUP_EVENT, BLANG_EVENT_CAPTURED_HOLD2, EM_SYNC_OWNER);
                break;
            }
        }
        if ((em->stateGroup != EM_GROUP_IDLE || em->state != BLANG_IDLE_STATUS1_COLLAPSE) && (em->stateGroup != EM_GROUP_REACT || em->state != BLANG_REACT_FALL)) {
            Em_SetStatus1Timer(em);
            EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_STATUS1_COLLAPSE, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_STATUS2:
        if ((s8) em->holdTimer != 0) {
            Em_SetStatus2Timer(em);
            EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_STATUS2_DOWNED, EM_SYNC_OWNER);
        } else if ((em->stateGroup != EM_GROUP_IDLE || em->state != BLANG_IDLE_STATUS2_COLLAPSE) && (em->stateGroup != EM_GROUP_REACT || em->state != BLANG_REACT_FALL)) {
            Em_SetStatus2Timer(em);
            EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_STATUS2_COLLAPSE, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_HIT_LYING:
        Em_RestartScript(em);
        switch (em->state) { /* the lying states that set hitReactMode 2 are all in EM_GROUP_IDLE */
        case BLANG_IDLE_REST:
            EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_REST_END, EM_SYNC_OWNER);
            break;
        case BLANG_IDLE_STATUS2_COLLAPSE:
            EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_STATUS2_END, EM_SYNC_OWNER);
            break;
        case BLANG_IDLE_STATUS1_COLLAPSE:
            EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_STATUS1_END, EM_SYNC_OWNER);
            break;
        case BLANG_IDLE_STATUS2_DOWNED:
            EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_STATUS2_DOWNED_END, EM_SYNC_OWNER);
            break;
        case BLANG_IDLE_STATUS1_DOWNED:
            EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_STATUS1_DOWNED_END, EM_SYNC_OWNER);
            break;
        }
        break;
    case EM_DMG_PART8_DOWN:
        EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_PART_BREAK, EM_SYNC_OWNER);
        break;
    case EM_DMG_PART_FLINCH:
        EmBlangonga_BreakParts(em, 1);
        switch (em->stance) {
        case EM_STANCE_AIR:
            EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_FALL, EM_SYNC_OWNER);
            break;
        default:
            partStateSum = 0;
            if (EmBlangonga_IsCongalala(em) == 1) {
                if (em->type == EM_KIND_CONGALALA) {
                    switch (em->flinchPart) {
                    case 4:
                        if (em->parts[em->flinchPart].state >= 2) {
                            if (EmBlangonga_CarriedShown(em, 1) == 1) {
                                EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_DROP_CARRIED_B, EM_SYNC_OWNER);
                                goto done;
                            }
                            if (EmBlangonga_CarriedShown(em, 2) == 1) {
                                EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_DROP_CARRIED_C, EM_SYNC_OWNER);
                                goto done;
                            }
                            if (EmBlangonga_CarriedShown(em, 0) == 1) {
                                EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_DROP_CARRIED_A, EM_SYNC_OWNER);
                                goto done;
                            }
                        }
                    }
                }
                for (i = 0; i < 8; i++) {
                    partStateSum += em->parts[i].state;
                }
                if (partStateSum <= 3) {
                    EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_FLINCH, EM_SYNC_OWNER);
                    break;
                }
                switch (em->flinchPart) {
                case 0:
                    if (EmBlangonga_IsSmallKind(em) == 0) {
                        EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_FLINCH_ATTACK, EM_SYNC_OWNER);
                    } else {
                        EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_FLINCH_B, EM_SYNC_OWNER);
                    }
                    break;
                case 2:
                    EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_KNOCK_DOWN, EM_SYNC_OWNER);
                    break;
                case 3:
                    EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_KNOCK_DOWN_B, EM_SYNC_OWNER);
                    break;
                case 4:
                default:
                    EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_FLINCH_B, EM_SYNC_OWNER);
                    break;
                }
            } else {
                if (EmBlangonga_IsSmallKind(em) == 0) {
                    for (i = 0; i < 8; i++) {
                        partStateSum += em->parts[i].state;
                    }
                    if (partStateSum <= 3) {
                        EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_FLINCH, EM_SYNC_OWNER);
                        break;
                    }
                }
                switch (em->flinchPart) {
                case 1:
                    EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_KNOCK_DOWN_B, EM_SYNC_OWNER);
                    break;
                case 2:
                    EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_KNOCK_DOWN, EM_SYNC_OWNER);
                    break;
                default:
                    EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_FLINCH_B, EM_SYNC_OWNER);
                    break;
                }
            }
            break;
        }
        break;
    case EM_DMG_HIT:
        if (em->type == EM_KIND_BLANGO) {
            if (em->stance == EM_STANCE_GROUND) {
                em->scriptPending = 0;
                EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_FLINCH, EM_SYNC_OWNER);
            }
        } else if (em->type == EM_KIND_CONGALALA && em->stateGroup == EM_GROUP_ATTACK && em->state == BLANG_ATK_CONGALALA_HITZONE_COMBO && (s8) em->hitzoneSet == 1 &&
                   w->hitSeCooldown == 0) {
            Em_PlaySeChecked(em, 0x1B, 0, &em->posX, 6, 0);
            w->hitSeCooldown = 30;
        }
        break;
    case EM_DMG_ATTACK13:
        Em_ResetKindTimer(em);
        EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_HOLD2, EM_SYNC_OWNER);
        break;
    case EM_DMG_STATUS5:
        if ((em->stateGroup != EM_GROUP_REACT || em->state != BLANG_REACT_FALL) && (em->stateGroup != EM_GROUP_REACT || em->state != BLANG_REACT_STUN_DOWNED)) {
            if ((s8) em->holdTimer != 0) {
                Em_SetStatus5Timer(em);
                EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_STUN_DOWNED, EM_SYNC_OWNER);
            } else if ((em->stateGroup != EM_GROUP_REACT || em->state != BLANG_REACT_STUN) && (em->stateGroup != EM_GROUP_REACT || em->state != BLANG_REACT_STUN_B)) {
                Em_SetStatus5Timer(em);
                if (((em->hitAngle - em->rotY) & 0xFFFF) > 0x8000) {
                    EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_STUN, EM_SYNC_OWNER);
                } else {
                    EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_STUN_B, EM_SYNC_OWNER);
                }
            }
        }
        break;
    }
done: /* the Congalala drops leave the result switch from inside the part switch */
    if (Em_DamageResultIsReaction(em, result) == 1) {
        EmBlangonga_BreakParts(em, 0);
    }
    switch (gQuest.stage) {
    case QUEST_ID_CONGALALA_SCENE_1:
    case QUEST_ID_CONGALALA_SCENE_2:
        if (em->type == EM_KIND_CONGALALA && Area_DayId(em->area) == AREA_JUNGLE_3 && Save_TestEventFlag(EVENT_FLAG_CONGALALA_SCENE) == 0) {
            if (Area_DayId(gScene.area) == AREA_JUNGLE_3 && gScene.eventRunning == 1 && em->stateGroup != EM_GROUP_EVENT) {
                EmBlangonga_ChangeState(em, EM_GROUP_EVENT, BLANG_EVENT_CONGALALA_SCENE, EM_SYNC_SEND);
            }
            goto skip;
        }
        break;
    case QUEST_ID_BLANGONGA_SCENE_1:
    case QUEST_ID_BLANGONGA_SCENE_2:
        if ((em->type == EM_KIND_BLANGONGA || em->type == EM_KIND_BLANGO) && Area_DayId(em->area) == AREA_SNOWY_MOUNTAINS_6 && Save_TestEventFlag(EVENT_FLAG_BLANGONGA_SCENE) == 0) {
            if (Area_DayId(gScene.area) == AREA_SNOWY_MOUNTAINS_6 && gScene.eventRunning == 1 && em->stateGroup != EM_GROUP_EVENT) {
                EmBlangonga_ChangeState(em, EM_GROUP_EVENT, BLANG_EVENT_BLANGONGA_SCENE, EM_SYNC_SEND);
            }
            goto skip;
        }
        break;
    }
    Em_UpdatePendingScript(em);
skip: /* a story scene keeps the pending script from starting */
    Em_ClearHitReactMode(em);
    EmBlangonga_RunState(em, (s32) w);
    if (em->stateRequest != 0) {
        EmBlangonga_RunState(em, (s32) w);
        em->stateRequest = 0;
    }
    if (EmBlangonga_InStateArmor(em) == 1) {
        em->hideShadowHold = 2;
        em->hideShadow = 2;
        em->guardTimer = 2;
    }
    EmBlangonga_UpdateTint(em);
}

/*
 * Runs the current state through its group's dispatcher (EM_GROUP_EVENT2
 * uses EM_GROUP_EVENT's) after keeping the group and state as the previous
 * ones.
 */
void EmBlangonga_RunState(Em *em, s32 work)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->stateGroup) {
    case EM_GROUP_IDLE:
        EmBlangonga_RunIdleState(em, (EmWork *) work);
        break;
    case EM_GROUP_MOVE:
        EmBlangonga_RunMoveState(em, work);
        break;
    case EM_GROUP_FLY:
        EmBlangonga_RunLeapState(em, (EmWork *) work);
        break;
    case EM_GROUP_ATTACK:
        EmBlangonga_RunAttackState(em, (EmWork *) work);
        break;
    case EM_GROUP_REACT:
        EmBlangonga_RunReactState(em, (EmWork *) work);
        break;
    case EM_GROUP_DEFEAT:
        EmBlangonga_RunDefeatState(em, work);
        break;
    case EM_GROUP_EVENT:
        EmBlangonga_RunEventState(em, (EmWork *) work);
        break;
    case EM_GROUP_EVENT2:
        EmBlangonga_RunEventState(em, (EmWork *) work);
        break;
    case EM_GROUP_VERSUS:
        EmBlangonga_RunVersusState(em, (EmWork *) work);
        
    default:
        break;
    }
}

/*
 * Blangonga: a flinch of the damaged part 4 whose flags have bit 2 flags
 * part 2's break (Em_FlagPartBreak); then, when `apply`, applies the part
 * breaks (Em_ApplyPartBreaks).
 */
void EmBlangonga_BreakParts(Em *em, s32 apply)
{
    u8 k;

    if (em->type == EM_KIND_BLANGONGA) {
        k = em->flinchPart;
        switch (k) {
        case 4:
            if (em->parts[k].state > 0 && (em->partFlags[k] & 2)) {
                Em_FlagPartBreak(em, 2);
            }
            break;
        }
    }
    if (apply != 0) {
        Em_ApplyPartBreaks(em);
    }
}

/*
 * In the player's area, creates the ground effect (Effect0D) of `kind` at
 * bone `bone` dropped to the floor, scaled by the monster's size.
 */
void EmBlangonga_SpawnGroundEffect(Em *em, s32 bone, s32 kind, f32 scale)
{
    f32 pos[3];

    if (Player_InCurrentArea(em)) {
        Em_BonePos(em, bone, pos);
        pos[1] = em->floorY;
        PartFx_SpawnOnGround(pos, kind, scale * em->scaleX);
    }
}

/*
 * While enraged, in the motions of its list: Rajang creates stage object
 * 0x27 every 16 frames, the others stage object 7 every 45 frames
 * (staggered by the monster's slot).
 */
void EmBlangonga_SpawnK1CInMotions(Em *em, s16 motion)
{
    switch (motion) {
    case EM_MOTION_ID(0, BLANG_MOT_IDLE):
    case EM_MOTION_ID(0, BLANG_MOT_WALK):
    case EM_MOTION_ID(0, BLANG_MOT_TURN_LEFT):
    case EM_MOTION_ID(0, BLANG_MOT_TURN_RIGHT):
    case EM_MOTION_ID(0, BLANG_MOT_TURN_LEFT_QUICK):
    case EM_MOTION_ID(0, BLANG_MOT_TURN_RIGHT_QUICK):
    case EM_MOTION_ID(0, BLANG_MOT_LIMP):
    case EM_MOTION_ID(0, BLANG_MOT_SLAM_WINDUP):
    case EM_MOTION_ID(0, BLANG_MOT_STRIKE_A):
    case EM_MOTION_ID(0, BLANG_MOT_STRIKE_B):
    case EM_MOTION_ID(0, BLANG_MOT_ALERT_IDLE):
    case EM_MOTION_ID(0, BLANG_MOT_ROAR):
    case EM_MOTION_ID(0, BLANG_MOT_TURN_ABOUT):
    case EM_MOTION_ID(0, BLANG_MOT_TURN_ABOUT_B):
    case EM_MOTION_ID(0, BLANG_MOT_SHAKE_OFF):
    case EM_MOTION_ID(0, BLANG_MOT_IDLE_ACTION_B_END):
        if (em->rage != 0) {
            if (EmBlangonga_IsRajang(em) == 1) {
                if ((gScene.frame & 0xF) == em->index * 2) {
                    MonsterFx_CreateOnEm(em, 0x27, 1.0f);
                }
            } else if (gScene.frame % 45 == em->index * 2) {
                MonsterFx_CreateOnEm(em, 7, em->scaleX);
            }
        }
        break;
    }
}

/*
 * Shakes the camera (CamShake_ReqAIf level `level`) when the layer-0 motion
 * passes `frame`.
 */
void EmBlangonga_CamShakeOnFrame(s32 em, s32 frame, s32 i)
{
    if (EmMotion_PassedFrameScaled((Em *) em, (f32) frame, 0U) != 0) {
        CamShake_ReqAIf((void *) em, i);
    }
}

/* Does nothing. */
void EmBlangonga_MotionEffectsDefault(Em *em)
{
}

void DustCloud_Create(Em *em, s16 k, s16 c, f32 scale);
void MonsterFx_CreateAt(f32 *src, u8 k, f32 f);
void Vec_RotateY(f32 *v, f32 angle);
void EmBlangonga_MotionSounds(Em *em, EmWork *work);
void EmBlangonga_MotionSoundsCongalala(Em *em, EmWork *work);
void EmBlangonga_MotionSoundsRajang(Em *em, EmWork *work);

/*
 * The motions' effects: keeps the layer-0 motion id (EmWork.prevMotion),
 * plays the kind's sounds (EmBlangonga_MotionSounds, ...Congalala,
 * ...Rajang) and the rage's stage objects, then per motion
 * and frame the hits (BlangongaAttack), ground effects, stage objects and
 * camera shakes.
 */
void EmBlangonga_MotionEffects(Em *em, EmWork *work)
{
    f32 ofs[3];
    f32 pos[3];

    if (em->motionId[0] != work->prevMotion) {
        work->prevMotion = em->motionId[0];
    }
    switch (em->type) {
    case EM_KIND_BLANGONGA:
    default:
    case EM_KIND_BLANGO:
        EmBlangonga_MotionSounds(em, work);
        break;
    case EM_KIND_CONGALALA:
    case EM_KIND_CONGA:
        EmBlangonga_MotionSoundsCongalala(em, work);
        break;
    case EM_KIND_RAJANG:
        EmBlangonga_MotionSoundsRajang(em, work);
        break;
    }
    EmBlangonga_SpawnK1CInMotions(em, work->prevMotion);
    switch (work->prevMotion) {
    case EM_MOTION_ID(0, BLANG_MOT_TURN_LEFT_QUICK):
        if ((EmBlangonga_IsSmallKind(em) == 0) && (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0)) {
            BlangongaAttack_Create(em, 0xE);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TURN_RIGHT_QUICK):
        if ((EmBlangonga_IsSmallKind(em) == 0) && (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0)) {
            BlangongaAttack_Create(em, 0x10);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_MOVE_ACTION_D):
        if ((EmBlangonga_IsSmallKind(em) == 0) && (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0)) {
            BlangongaAttack_Create(em, 0xB);
        }
        if (EmMotion_PassedFrameScaled(em, 14.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 2, 0xF, em->scaleX);
        }
        break;
    case EM_MOTION_ID(0, 0xB):
        if (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0) {
            EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_MOVE_ACTION_A):
        if ((EmBlangonga_IsSmallKind(em) == 0) && (EmMotion_PassedFrameScaled(em, 18.0f, 0) != 0)) {
            BlangongaAttack_Create(em, 0xC);
        }
        if (EmMotion_PassedFrameScaled(em, 14.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 2, 0xF, em->scaleX);
        }
        break;
    case EM_MOTION_ID(0, 0xD):
        if (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0) {
            EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_MOVE_ACTION_B):
        if ((EmBlangonga_IsSmallKind(em) == 0) && (EmMotion_PassedFrameScaled(em, 18.0f, 0) != 0)) {
            BlangongaAttack_Create(em, 0xD);
        }
        if (EmMotion_PassedFrameScaled(em, 14.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 3, 0x10, em->scaleX);
        }
        break;
    case EM_MOTION_ID(0, 0xF):
        if (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0) {
            EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_MOVE_ACTION_C):
        if (EmMotion_PassedFrameScaled(em, 18.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 2, 0x11, em->scaleX);
        }
        if (EmMotion_PassedFrameScaled(em, 22.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 3, 0x12, em->scaleX);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_JUMP):
        if (EmBlangonga_IsRajang(em) == 1) {
            if (EmMotion_PassedFrameScaled(em, 48.0f, 0) != 0) {
                DustCloud_Create((Em *) em, 0x13, 0, em->scaleX);
                break;
            }
        } else if (EmBlangonga_IsCongalala(em) == 1) {
            if (EmMotion_PassedFrameScaled(em, 54.0f, 0) != 0) {
                DustCloud_Create((Em *) em, 0x13, 0, em->scaleX);
                break;
            }
        } else if (EmMotion_PassedFrameScaled(em, 50.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 0x13, 0, em->scaleX);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_CHARGE):
        if (EmBlangonga_IsCongalala(em) == 1) {
            if (EmMotion_PassedFrameScaled(em, 14.0f, 0) != 0) {
                DustCloud_Create((Em *) em, 2, 0xF, em->scaleX);
            }
            if (EmMotion_PassedFrameScaled(em, 42.0f, 0) != 0) {
                DustCloud_Create((Em *) em, 3, 0x10, em->scaleX);
                break;
            }
        } else {
            if (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0) {
                DustCloud_Create((Em *) em, 3, 0x10, em->scaleX);
            }
            if (EmMotion_PassedFrameScaled(em, 38.0f, 0) != 0) {
                DustCloud_Create((Em *) em, 2, 0x11, em->scaleX);
            }
            if (EmMotion_PassedFrameScaled(em, 42.0f, 0) != 0) {
                DustCloud_Create((Em *) em, 3, 0x10, em->scaleX);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_JUMP_LAND):
        if (EmBlangonga_IsRajang(em) == 1) {
            if (EmMotion_PassedFrameScaled(em, 12.0f, 0) != 0) {
                DustCloud_Create((Em *) em, 0xB, 0, 0.6f * em->scaleX);
                break;
            }
        } else if (EmMotion_PassedFrameScaled(em, 8.0f, 0) != 0) {
            EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DIG_IN):
        if (EmMotion_PassedFrameScaled(em, 22.0f, 0) || EmMotion_PassedFrameScaled(em, 92.0f, 0) || EmMotion_PassedFrameScaled(em, 160.0f, 0) || EmMotion_PassedFrameScaled(em, 210.0f, 0) || EmMotion_PassedFrameScaled(em, 246.0f, 0)) {
            DustCloud_Create((Em *) em, 2, 0x11, em->scaleX);
        }
        if ((EmMotion_PassedFrameScaled(em, 64.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 134.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 170.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 200.0f, 0) != 0)) {
            DustCloud_Create((Em *) em, 3, 0x12, em->scaleX);
        }
        if (EmMotion_PassedFrameScaled(em, 254.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 2, 0, em->scaleX);
        }
        if (EmMotion_PassedFrameScaled(em, 42.0f, 0) != 0) {
            BlangongaAttack_Create(em, 0x1B);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_EMERGE):
        if (EmBlangonga_IsSmallKind(em) == 0) {
            EmBlangonga_CamShakeOnFrame((s32) em, 4, 5);
        }
        if ((EmMotion_PassedFrameScaled(em, 8.0f, 0) != 0) && Player_InCurrentArea(em)) {
            Em_BonePos(em, 1, pos);
            pos[1] = em->floorY;
            MonsterFx_CreateAt(pos, 6, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 52.0f, 0) != 0) {
            EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_COLLAPSE):
        if (EmMotion_PassedFrameScaled(em, 84.0f, 0) != 0) {
            EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_LIE_DOWN):
        if (EmMotion_PassedFrameScaled(em, 170.0f, 0) != 0) {
            EmBlangonga_SpawnGroundEffect(em, 0x10, 7, 3.5f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TRIPLE_HIT):
        if (EmBlangonga_IsBlangonga(em) == 1) {
            if (EmMotion_PassedFrameScaled(em, 70.0f, 0) != 0) {
                DustCloud_Create((Em *) em, 2, 0xF, em->scaleX);
            }
            if (EmMotion_PassedFrameScaled(em, 92.0f, 0) != 0) {
                DustCloud_Create((Em *) em, 2, 0x11, em->scaleX);
            }
            if (EmMotion_PassedFrameScaled(em, 104.0f, 0) != 0) {
                EmBlangonga_SpawnGroundEffect(em, 0x10, 7, 3.5f);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TRIPLE_HIT_END):
        if (EmBlangonga_IsCongalala(em) == 1) {
            if ((EmMotion_PassedFrameScaled(em, 38.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 124.0f, 0) != 0)) {
                DustCloud_Create((Em *) em, 3, 0x22, em->scaleX);
            }
            if ((EmMotion_PassedFrameScaled(em, 76.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 172.0f, 0) != 0)) {
                DustCloud_Create((Em *) em, 2, 0x21, em->scaleX);
            }
            if (EmMotion_PassedFrameScaled(em, 200.0f, 0) != 0) {
                EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_STRIKE_A):
        if (EmBlangonga_IsRajang(em) == 1) {
            if (EmMotion_PassedFrameScaled(em, 48.0f, 0) != 0) {
                DustCloud_Create((Em *) em, 3, 0x22, em->scaleX);
            }
            if ((EmMotion_PassedFrameScaled(em, 22.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 74.0f, 0) != 0)) {
                DustCloud_Create((Em *) em, 2, 0x21, em->scaleX);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_STRIKE_B):
        if ((EmBlangonga_IsRajang(em) == 1) && (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0)) {
            EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DIG_THROW):
        if ((EmMotion_PassedFrameScaled(em, 102.0f, 0) != 0) && Player_InCurrentArea(em)) {
            Em_BonePos(em, 1, pos);
            pos[1] = em->floorY;
            ofs[0] = 0.0f;
            ofs[1] = 0.0f;
            ofs[2] = 330.0f * em->scaleX;
            Vec_RotateY(ofs, (6.2831855f * (f32) em->rotY) / 65536.0f);
            pos[0] += ofs[0];
            pos[1] += ofs[1];
            pos[2] += ofs[2];
            MonsterFx_CreateAt(pos, 6, 0.8f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_CHARGE_END):
        if (EmBlangonga_IsCongalala(em) == 1) {
            if (EmMotion_PassedFrameScaled(em, 34.0f, 0) != 0) {
                DustCloud_Create((Em *) em, 3, 0x10, em->scaleX);
                break;
            }
        } else if (EmBlangonga_IsRajang(em) != 0) {
            if (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) {
                DustCloud_Create((Em *) em, 2, 0xF, em->scaleX);
            }
            if (EmMotion_PassedFrameScaled(em, 56.0f, 0) != 0) {
                EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
                break;
            }
        } else {
            if (EmMotion_PassedFrameScaled(em, 12.0f, 0) != 0) {
                DustCloud_Create((Em *) em, 2, 0xF, em->scaleX);
            }
            if (EmMotion_PassedFrameScaled(em, 38.0f, 0) != 0) {
                EmBlangonga_SpawnGroundEffect(em, 0x10, 7, 3.5f);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_HOP_LAND):
        if ((EmBlangonga_IsCongalala(em) == 1) && (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0)) {
            EmBlangonga_SpawnGroundEffect(em, 1, 0x28, 1.0f);
            if ((s32)Player_InCurrentArea(em) & 0xFF) {
                AttackBurst_CreateAtScaled(&em->posX, (u16) em->rotY, 0xAU, 0.4f * em->scaleX);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_ROAR):
        if (EmBlangonga_IsBlangonga(em) == 1) {
            if (em->type == EM_KIND_BLANGONGA) {
                if ((em->partBreakCount <= 0) && (EmMotion_PassedFrameScaled(em, 38.0f, 0) != 0)) {
                    BlangongaAttack_Create(em, 0x17);
                    ElementBurst_CreateOnEm(em, 5, 2, 0.4f * em->scaleX);
                    break;
                }
            } else if (EmMotion_PassedFrameScaled(em, 60.0f, 0) != 0) {
                EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
                break;
            }
        } else if (em->type == EM_KIND_CONGALALA) {
            if ((gScene.eventRunning == 0) && (EmMotion_PassedFrameScaled(em, 70.0f, 0) != 0)) {
                EmEffectAttack_Create(em, EMFX_CONGALALA_RAGE_GAS, 0, 0, 0);
                break;
            }
        } else if ((EmBlangonga_IsRajang(em) == 1) && (EmMotion_PassedFrameScaled(em, 38.0f, 0) != 0)) {
            BlangongaAttack_Create(em, 0x17);
            ElementBurst_CreateOnEm(em, 5, 2, 0.4f * em->scaleX);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_EAT):
        if ((EmBlangonga_IsCongalala(em) == 1) && (EmMotion_PassedFrameScaled(em, 66.0f, 0) != 0)) {
            EmBlangonga_SpawnGroundEffect(em, 2, 7, 3.5f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_ROAR_B):
        if ((EmBlangonga_IsSmallKind(em) == 0) && (EmMotion_PassedFrameScaled(em, 64.0f, 0) != 0)) {
            BlangongaAttack_Create(em, 0x17);
            ElementBurst_CreateOnEm(em, 5, 2, 0.4f * em->scaleX);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DOWN):
        if (EmMotion_PassedFrameScaled(em, 108.0f, 0) != 0) {
            EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_FALL_LAND):
        if (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0) {
            EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DIE):
        if (EmMotion_PassedFrameScaled(em, 180.0f, 0) != 0) {
            EmBlangonga_SpawnGroundEffect(em, 0x10, 7, 3.5f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_KNOCK_DOWN):
        if ((EmMotion_PassedFrameScaled(em, 28.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 78.0f, 0) != 0)) {
            EmBlangonga_SpawnGroundEffect(em, 4, 7, 3.5f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_KNOCK_DOWN_B):
        if ((EmMotion_PassedFrameScaled(em, 28.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 78.0f, 0) != 0)) {
            EmBlangonga_SpawnGroundEffect(em, 4, 7, 3.5f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DIE_END):
        if (EmMotion_PassedFrameScaled(em, 18.0f, 0) != 0) {
            BlangongaAttack_Create(em, 0x1C);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TRAPPED):
        if (EmBlangonga_IsCongalala(em) == 1) {
            if (EmMotion_PassedFrameScaled(em, 16.0f, 0) != 0) {
                EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
                break;
            }
        } else if (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) {
            EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_STRUGGLE):
        if ((EmBlangonga_IsCongalala(em) == 1) && (em->rage != 0) && ((EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 40.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 80.0f, 0) != 0))) {
            MonsterFx_CreateOnEm(em, 0x14, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DOWNED):
        if (EmMotion_PassedFrameScaled(em, 92.0f, 0) != 0) {
            EmBlangonga_SpawnGroundEffect(em, 4, 7, 3.5f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DOWN_B):
        if (EmMotion_PassedFrameScaled(em, 108.0f, 0) != 0) {
            EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DROP_CARRIED):
        if ((EmBlangonga_IsCongalala(em) == 1) && (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0)) {
            EmBlangonga_SpawnGroundEffect(em, 0x10, 7, 3.5f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_EFFECT_STRIKE):
        if ((EmBlangonga_IsRajang(em) == 1) && (EmMotion_PassedFrameScaled(em, 74.0f, 0) != 0)) {
            BlangongaAttack_Create(em, 0x28);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_EFFECT_STRIKE_END):
        if ((EmBlangonga_IsCongalala(em) == 1) && (EmMotion_PassedFrameScaled(em, 44.0f, 0) != 0)) {
            EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_CONGALALA_HITZONE_COMBO_END):
        if (EmBlangonga_IsCongalala(em) == 1) {
            if (EmMotion_PassedFrameScaled(em, 96.0f, 0) != 0) {
                EmBlangonga_SpawnGroundEffect(em, 0x10, 7, 3.5f);
                break;
            }
        } else {
            if ((EmBlangonga_IsRajang(em) == 1) && (EmMotion_PassedFrameScaled(em, 26.0f, 0) != 0)) {
                DustCloud_Create((Em *) em, 0x13, 0, em->scaleX);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_RAJANG_JUMP_SHOT):
        if (EmBlangonga_IsCongalala(em) == 1) {
            if (EmMotion_PassedFrameScaled(em, 56.0f, 0) != 0) {
                EmBlangonga_SpawnGroundEffect(em, 0x10, 7, 3.5f);
            }
            if (EmMotion_PassedFrameScaled(em, 90.0f, 0) != 0) {
                EmBlangonga_SpawnGroundEffect(em, 2, 7, 3.5f);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_RAJANG_JUMP_SHOT_LAND):
        if ((EmBlangonga_IsRajang(em) == 1) && (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0)) {
            DustCloud_Create((Em *) em, 0xB, 0, 0.6f * em->scaleX);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_BACK_STRIKE):
        if (EmBlangonga_IsBlangonga(em) == 1 || EmBlangonga_IsRajang(em) == 1) {
            if (EmMotion_PassedFrameScaled(em, 44.0f, 0) != 0) {
                BlangongaAttack_Create(em, 9);
            }
            if (EmMotion_PassedFrameScaled(em, 50.0f, 0) != 0) {
                BlangongaAttack_Create(em, 0x14);
            }
            if ((EmMotion_PassedFrameScaled(em, 48.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 78.0f, 0) != 0)) {
                EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_IDLE_ACTION_B):
        if (EmBlangonga_IsBlangonga(em) == 1) {
            if ((EmMotion_PassedFrameScaled(em, 60.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 94.0f, 0) != 0)) {
                DustCloud_Create((Em *) em, 3, 0x12, em->scaleX);
                break;
            }
        } else {
            if (EmBlangonga_IsRajang(em) == 1) {
                if (EmMotion_PassedFrameScaled(em, 52.0f, 0) != 0) {
                    DustCloud_Create((Em *) em, 2, 0xF, em->scaleX);
                }
                if (EmMotion_PassedFrameScaled(em, 62.0f, 0) != 0) {
                    DustCloud_Create((Em *) em, 3, 0x10, em->scaleX);
                }
                if (EmMotion_PassedFrameScaled(em, 96.0f, 0) != 0) {
                    EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
                    break;
                }
            }
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_IDLE_ACTION_B_END):
        if (EmBlangonga_IsRajang(em) == 1) {
            if (EmMotion_PassedFrameScaled(em, 16.0f, 0) || EmMotion_PassedFrameScaled(em, 52.0f, 0) || EmMotion_PassedFrameScaled(em, 84.0f, 0)) {
                DustCloud_Create((Em *) em, 2, 0xF, em->scaleX);
            }
            if ((EmMotion_PassedFrameScaled(em, 46.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 90.0f, 0) != 0)) {
                DustCloud_Create((Em *) em, 3, 0x10, em->scaleX);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_SLAM):
        if (EmBlangonga_IsCongalala(em) == 1) {
            if (EmMotion_PassedFrameScaled(em, 36.0f, 0) != 0) {
                EmBlangonga_SpawnGroundEffect(em, 0x10, 0x13, 1.0f);
            }
        } else {
            if (EmMotion_PassedFrameScaled(em, 22.0f, 0) != 0) {
                EmBlangonga_SpawnGroundEffect(em, 0x10, 0x13, 1.0f);
            }
            if ((EmMotion_PassedFrameScaled(em, 24.0f, 0) != 0) && Player_InCurrentArea(em)) {
                Em_BonePos(em, 0x10, pos);
                pos[1] = em->floorY += 20.0f;
                AttackBurst_CreateAtScaled(pos, (u16) em->rotY, 0xAU, 0.6f * em->scaleX);
            }
        }
    case EM_MOTION_ID(0, BLANG_MOT_RAJANG_CHARGE_END):
        if ((EmBlangonga_IsRajang(em) == 1) && (EmMotion_PassedFrameScaled(em, 36.0f, 0) != 0)) {
            EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
        }
        break;
    default:
        EmBlangonga_MotionEffectsDefault(em);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_IDLE):
    case EM_MOTION_ID(0, 0x2):
    case EM_MOTION_ID(0, BLANG_MOT_WALK):
    case EM_MOTION_ID(0, BLANG_MOT_TURN_LEFT):
    case EM_MOTION_ID(0, BLANG_MOT_TURN_RIGHT):
    case EM_MOTION_ID(0, BLANG_MOT_LIMP):
    case EM_MOTION_ID(0, BLANG_MOT_JUMP_FALL):
    case EM_MOTION_ID(0, BLANG_MOT_LIE):
    case EM_MOTION_ID(0, BLANG_MOT_GET_UP):
    case EM_MOTION_ID(0, BLANG_MOT_SLAM_WINDUP):
    case EM_MOTION_ID(0, BLANG_MOT_THROW):
    case EM_MOTION_ID(0, BLANG_MOT_ALERT_IDLE):
    case EM_MOTION_ID(0, BLANG_MOT_TURN_ABOUT):
    case EM_MOTION_ID(0, BLANG_MOT_TURN_ABOUT_B):
    case EM_MOTION_ID(0, BLANG_MOT_EAT_NEED2):
    case EM_MOTION_ID(0, BLANG_MOT_EAT_NEED2_END):
    case EM_MOTION_ID(0, BLANG_MOT_EAT_END):
    case EM_MOTION_ID(0, BLANG_MOT_FLINCH_B):
    case EM_MOTION_ID(0, BLANG_MOT_FLINCH):
    case EM_MOTION_ID(0, BLANG_MOT_SHAKE_OFF):
    case EM_MOTION_ID(0, BLANG_MOT_DOWN_GET_UP):
    case EM_MOTION_ID(0, BLANG_MOT_STATUS4):
    case EM_MOTION_ID(0, BLANG_MOT_FALL):
    case EM_MOTION_ID(0, 0x4C):
    case EM_MOTION_ID(0, BLANG_MOT_DOWN_GET_UP_B):
        break;
    }
}

/*
 * Callback slot 3: the first frame only marks the effects started (fxStep);
 * from then on runs EmBlangonga_MotionEffects every frame.
 */
void EmBlangonga_UpdateMotionEffects(Em *em)
{
    EmBlangongaWork *w = (EmBlangongaWork *)&em->work;

    switch (w->fxStep) {
    case 0:
        w->fxStep++;
        break;
    case 1:
        EmBlangonga_MotionEffects(em, (EmWork *) w);
        break;
    }
}

/* Callback slot 2: does nothing. */
void EmBlangonga_TableNop(void)
{
}

/*
 * Callback slot 4, the monster script's questions (`kind`): 0 whether
 * moveCount reached 4; 1 whether it stands on snow (floor attribute 1 in
 * AREA_SNOWY_MOUNTAINS_5, AREA_SNOWY_MOUNTAINS_6 and AREA_SNOWY_MOUNTAINS_8,
 * while in the player's area); 2 stateArmor; 3 eatenKind; 4 whether a visible Blangonga in its area is in
 * BLANG_IDLE_ROAR_SIGNAL; 5 the destination area's index among the family's
 * twelve areas (0xFF if none); 6 the first carried item shown (item 0 gives
 * 1, item 2 gives 2, item 1 gives 3; 0 if none).
 */
u8 EmBlangonga_ScriptQuery(Em *em, u8 query)
{
    EmBlangongaWork *w = (EmBlangongaWork *)&em->work;
    Em *other;
    s16 i;

    switch (query) {
    case BLANG_QUERY_MOVE_COUNT:
        return w->moveCount >= 4;
    case BLANG_QUERY_ON_SNOW:
        switch (Area_DayId(em->area)) {
        case AREA_SNOWY_MOUNTAINS_5:
        case AREA_SNOWY_MOUNTAINS_6:
        case AREA_SNOWY_MOUNTAINS_8:
            if (Player_InCurrentArea(em)) {
                if (*(u8 *) &em->floorAttr == 1) {
                    return 1;
                }
                return 0;
            }
            return 0;
        }
        return 0;
    case BLANG_QUERY_ARMOR:
        return EmBlangonga_InStateArmor(em) == 1;
    case BLANG_QUERY_EATEN_KIND:
        return w->eatenKind;
    case BLANG_QUERY_ROAR_SIGNAL:
        for (other = gEms, i = 0; i < EM_COUNT; i++, other++) {
            if (other->active != 0 && other->visible != 0 && (Obj_SameArea(em, (Player *) other) & 0xFF) && other->type == EM_KIND_BLANGONGA && Player_IsState(other, EM_GROUP_IDLE, BLANG_IDLE_ROAR_SIGNAL)) {
                return 1;
            }
        }
        return 0;
    case BLANG_QUERY_DEST_AREA:
        switch (Area_DayId(*(u16 *) &em->destArea)) {
        case AREA_JUNGLE_1:
            return 0;
        case AREA_JUNGLE_2:
            return 1;
        case AREA_JUNGLE_3:
            return 2;
        case AREA_JUNGLE_6:
            return 3;
        case AREA_JUNGLE_9:
            return 4;
        case AREA_JUNGLE_10:
            return 5;
        case AREA_SWAMP_2:
            return 6;
        case AREA_SWAMP_5:
            return 7;
        case AREA_SWAMP_6:
            return 8;
        case AREA_SWAMP_7:
            return 9;
        case AREA_SWAMP_8:
            return 10;
        case AREA_SWAMP_9:
            return 11;
        default:
            return 0xFF;
        }
    case BLANG_QUERY_CARRIED:
        if (EmBlangonga_CarriedShown(em, 0) == 1) {
            return 1;
        }
        if (EmBlangonga_CarriedShown(em, 2) == 1) {
            return 2;
        }
        if (EmBlangonga_CarriedShown(em, 1) == 1) {
            return 3;
        }
        return 0;
    default:
        return 0;
    }
}

/*
 * Callback slot 6: returns 1 while no part break has happened (Em.partBreakCount
 * 0) and Blangonga is on the ground, or Rajang enraged on the ground: the
 * condition for BLANG_REACT_PART_BREAK.
 */
s32 EmBlangonga_AllowPartBreak(Em *em)
{
    if ((s8) em->partBreakCount != 0) {
        return 0;
    }
    if (em->type == EM_KIND_BLANGONGA) {
        if (em->stance == EM_STANCE_GROUND) {
            return 1;
        }
    } else if (em->type == EM_KIND_RAJANG && em->rage != 0 && em->stance == EM_STANCE_GROUND) {
        return 1;
    }
    return 0;
}

/* The file's initialised data, in the game's order */
EmCurveKey EmBlangonga_VelYCurves_0[10] = {
#include "game/008C9C60.inc"
};
EmCurveKey EmBlangonga_VelYCurves_1[4] = {
#include "game/008C9CB0.inc"
};
EmCurveKey EmBlangonga_VelYCurves_2[13] = {
#include "game/008C9CD0.inc"
};
EmCurveKey *EmBlangonga_VelYCurves[3] = {
    EmBlangonga_VelYCurves_0, EmBlangonga_VelYCurves_1, EmBlangonga_VelYCurves_2,
};
EmCurveKey EmBlangonga_SpeedCurves_2[13] = {
#include "game/008C9D50.inc"
};
EmCurveKey *EmBlangonga_SpeedCurves[3] = {
    NULL, NULL, EmBlangonga_SpeedCurves_2,
};
EmBlangongaTurnData EmBlangonga_TurnData = {
#include "game/008C9DD0.inc"
};
Vec3f EmBlangonga_CarveThrowPt = {
#include "game/008C9E28.inc"
};
void *EmBlangonga_Callbacks[8] = {
    EmBlangonga_SpawnEffect01, EmBlangonga_SpawnEffect01, EmBlangonga_TableNop, EmBlangonga_UpdateMotionEffects,
    EmBlangonga_ScriptQuery, EmBlangonga_Reinit, EmBlangonga_AllowPartBreak, NULL,
};
