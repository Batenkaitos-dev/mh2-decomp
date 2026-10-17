/*
 * Monster families: Khezu and Red Khezu (EmKhezu_*), then the small
 * monsters that follow them in the game's code: the Velociprey family
 * (EmVelociprey_*: Velociprey, Genprey, Ioprey, Giaprey), Anteka
 * (EmAnteka_*: the Anteka and the Kelbi) and Mosswine (EmMosswine_*:
 * Mosswine and Bullfango), and the first of Lao-Shan Lung's functions
 * (EmLaoshan_MotionSounds; the rest are in em_laoshan.c).
 *
 * Each family has the same shape (include/em.h): Init and Start, one
 * function per state, a dispatcher per state group (Run*State) and a
 * function that enters a state (Set*State, ChangeState), the per-frame
 * Update that turns Em_UpdateDamage's result into reactions, and the
 * motion effects and curves. The state functions take the monster and
 * its work area (EmWork, or the family's view of it: EmKhezuWork,
 * EmVelocipreyWork, EmMosswineWork); Em.step is the step within the
 * current state. Each family's motions and states are named in its
 * header: include/em_khezu.h, em_velociprey.h, em_anteka.h and
 * em_mosswine.h.
 */
#include "common.h"
#include "scene.h"
#include "quest.h"
#include "em.h"
#include "em_khezu.h"
#include "em_anteka.h"
#include "em_velociprey.h"
#include "em_mosswine.h"
#include "player.h"
#include "objattach.h"
#include "area.h"
/* prototypes of callees (round 13) */
u16 Area_IdForCurrentTime(u16);
void QuestEm_Hunted(Em *em);
void Vec_Rotate(f32 *out, f32 *in, f32 *mtx);
void Player_ClearAccel2(void *);

/* prototypes */
void EmAnteka_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void Effect0D_SpawnForEm(Em *em, s32 a, s32 b);
s32 EmMotion_InFrameRange(void *em, s32 layer, f32 from, f32 to);
u16 Rand_Next(s32 stream);
void KhezuAttack_Create(Em *em, s8 kind);
void EmFxSpark_SpawnOnEm(Em *em, u8 k);
void EmFxSpark_SpawnTimed(Em *em, u8 k, s32 n, f32 f);
void StageObj_K0F_CreateOnEm(Em *em, u8 k, u8 c, f32 scale);
void EmKhezu_ChangeState(Em *em, u16 group, u16 state, u8 mode);
u8 EmKhezu_UpdateCurves(Em *em);
void EmMosswine_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void EmVelociprey_ChangeState(Em *em, u16 group, u16 state, u8 mode);
/* unit's own declaration: with the definition's void (Em *em, s16 n) (game/unk_708850.c), EmKhezu_EatNeed2: 38 of 225 words differ, EmKhezu_EatNeed1: 36 of 84 words differ */
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
/* unit's own declaration: with the definition's void (Em *em, s32 group, s32 state, u8 mode) (game/unk_701810.c), EmAnteka_ChangeState: 2 of 92 words differ, EmKhezu_ChangeState: 8 of 171 words differ ... */
void Em_SetStateSync(Em *em, u16 group, u16 state, u8 mode);
u16 Math_AngleXZ(f32 *from, f32 *to);
void Player_ClearFlag(void *, u32 flag);
void Player_SetFlag(void *obj, u32 flag);
void QuestEm_Captured(Em *em);
void QuestEm_Leave(Em *em);
ObjAttach *EmContactAttack_Create(Em *em, s8 kind);
void EmRangedAttack_CreateAtBone(Em *, s32, s32, s32, s32, s32);
void VelocipreyAttack_Create(Em *, s8);
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
/* unit's own declaration: with the definition's s32 (Em *em, s16 n, s16 max, s16 interval) (game/unk_708850.c), EmKhezu_Rest: 44 of 92 words differ, EmKhezu_Status2LieDown: 42 of 90 words differ */
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
void EmVelociprey_Init(Em *);
void EmVelociprey_Respawn(Em *);
void EmVelociprey_Walk(Em *, EmWork *);
void EmVelociprey_RunAttackState(Em *em, EmWork *work);
void EmVelociprey_DieBack(Em *em, EmWork *work);
void EmVelociprey_DieForward(Em *em, EmWork *work);
void EmVelociprey_DieSideA(Em *, EmWork *);
void EmVelociprey_DieSideB(Em *em, EmWork *work);
void EmVelociprey_Reappear(Em *);
void EmVelociprey_DieInAirBack(Em *em, EmWork *work);
void EmVelociprey_DieInAirForward(Em *em, EmWork *work);
void EmVelociprey_DieInAirSideA(Em *, EmWork *);
void EmVelociprey_DieInAirSideB(Em *em, EmWork *work);
void EmVelociprey_EventLookAround(Em *, EmWork *);
void EmVelociprey_MotionEffects(Em *em, EmWork *work);
void EmVelociprey_StartCurves(Em *em, u8 kind);
u8 EmVelociprey_UpdateCurves(Em *em);
void EmAnteka_Init(Em *);
void EmAnteka_BlownBack(Em *em, EmWork *work, s16 variant);
void EmAnteka_Die(Em *, EmWork *);
void EmAnteka_EndState(Em *em);
void EmAnteka_SetMotion(Em *em, s32 arg1, s32 arg2, s32 arg3);
void EmAnteka_MotionEffects(Em *, EmWork *);
void EmMosswine_Init(Em *);
void EmMosswine_Status2Lie(Em *);
void EmMosswine_Turn(Em *, s32);
void EmMosswine_Walk(Em *);
void EmMosswine_Run(Em *em);
void EmMosswine_WalkB(Em *em);
void EmMosswine_WalkScripted(Em *);
void EmMosswine_WanderHome(Em *em);
void EmMosswine_Charge(Em *, u32);
void EmMosswine_BlownAway(Em *);
void EmMosswine_BlownAwayQuick(Em *);
void EmMosswine_Die(Em *);
void EmMosswine_DieQuick(Em *);
void StageObj_K13_Create(void *owner, s16 v, u8 k);
void StageObj_K14_Create(Em *em, s16 k, s16 c, f32 scale);
/* unit's own declaration: with the definition's void (Em *em, u8 k, s16 v, s16 t, f32 y, f32 scale) (game/unk_874B90.c), EmKhezu_HeldDowned: 78 of 121 words differ */
void StageObj_Stun_CreateEm(Em *, s32, s32, s32, f32, f32);
s32 Sound_RandomSe(s32 id, s16 lo, s32 alt, s16 n);
s32 EmMotion_PassedFrameScaled(void *p, f32 frame, s32 layer);
s32 EmMotion_ReachedFrame(void *obj, s32 a, f32 frame);
f32 Vec_Distance(f32 *a, f32 *b);
void CamShake_ReqAIf(Em *obj, s32 i);
s32 Em_GetFloorY(Em *em, f32 *pos, f32 *out);
f32 *Mtx_FromAnglesXYZ(s32 *angles, f32 *m);
void Vec_Set(f32 *out, f32 x, f32 y, f32 z);
f32 Vec_DistanceXZ(f32 *a, f32 *b);
s32 Angle_TurnToward(s32 *cur, s32 *target, s32 step);
s32 ObjListA70_Add(EmObj *obj);
void ObjListA70_Remove(EmObj *obj);
s32 Versus_BothDefeated(void);
s32 QuestEm_CanAppear(Em *em);
void Player_ClearVelocity(void *);
void Motion_InitTime(void *p, s16 step);
void Effect01_Spawn(void *obj, s16 type);
extern f32 gAreaGroundY[];
extern EmTurnParams EmKhezu_TurnParams[6];
extern EmTurnParams EmVelociprey_TurnParams[2];
u16 Area_DayId(u16 area);
s32 EmList_FindKindNear(Em *em, u16 kind, s32 mode, f32 dist);
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
/* unit's own declaration: with the definition's s16 (void *obj, u8 state, u8 sub) (sub_main/unk_3A1070.c), EmMosswine_ScriptQuery: 17 of 31 words differ, EmMosswine_Update: 109 of 256 words differ */
s32 Player_IsState(void *obj, s32 group, s32 state);
void MinePoint_Remove(s32);
void EmKhezu_SetIdleState(Em *em, u16 state, u8 mode);
void EmKhezu_SetMoveState(Em *em, u16 state, u8 mode);
void EmKhezu_SetFlyState(Em *em, u16 state, u8 mode);
void EmKhezu_SetAttackState(Em *em, u16 state, u8 mode);
void EmVelociprey_SetIdleState(Em *em, u16 state, u8 mode);
void EmVelociprey_SetMoveState(Em *em, u16 state, u8 mode);
u8 EmKhezu_NearestCeilingPoint(Em *em, f32 *out);
struct AreaTri;
u8 AreaGrid2_GetCeiling(f32 *pos, f32 *y, struct AreaTri *info);
void EmVelociprey_SetHopState(Em *em, u16 state, u8 mode);
void EmVelociprey_SetAttackState(Em *em, u16 state, u8 mode);
extern f32 EmMosswine_VariantParams[][6];
void Em_SpawnLyingPuffs(Em *em, s32 bone, f32 *pos, f32 scale);
extern f32 EmMosswine_Effect0BOfs[3];
void Em_SetMotionRow(Em *em, s32 row, s32 arg2, s32 arg3);
void Em_SetPartScaled(Em *em, s32 bit);
s32 Em_ComputeMaxHp(Em *em, s16 idx);
void StageObj_K09_Create(Em *);
s8 EmMine_CreateTimed0(Em *em, f32 *pos);
void Vec_Copy(f32 *out, f32 *in);
void StageObj_K25_Create(Player *p, Em *em, u8 k, s32 unused, f32 *pos);
void Versus_RouteEmByTeam(Em *em);
s32 Versus_DefeatCamera(Em *pl);
extern EmKhezuVec EmKhezu_BaitThrowPt __attribute__((aligned(8)));
extern EmCurveKey *EmKhezu_MoveVelYCurves[];
s8 EmMine_Create(Em *em, s32 idx);
s32 EmMine_Update(Em *em);
void EmMine_Remove(Em *em);
void Em_FindLink(Em *em);
void Em_InitParts(Em *em);
void GmEm_ApplySize(Em *em);
void GmEm_ResetState(Em *em);
struct EmIdEntry;
f32 *Em_AreaEntryValue(Em *em, struct EmIdEntry *entry);
extern EmTurnParams EmMosswine_TurnParams[1];
void Em_PickRoutePoint(Em *em);
void Em_AreaExitPoint(Em *em, f32 *out);
void Em_EnterNextArea(Em *em);
s32 Area_ContainsPos(s32 id, f32 *pos);
void Hud_TriggerGlow(Em *em);
AreaRecord *Stage_GetAreaRecord(u16 x);
s32 Player_MoveFall(Player *p);
s32 Save_TestEventFlag(s32 flag);
void GmEm_ResetSize(Em *em);
extern s16 gEmCombatInit[];
s32 Em_SetCombat(Em *em, u8 a, s16 b);
s32 Obj_SameArea(void *obj, Player *b);
s32 Em_InBlockArea1(Em *em);
void Em_SetRequest(Em *em, u8 a, u8 prio);
extern u8 gEmShowsOnMap[];
s32 Em_UpdateCombatTimer(Em *em);
void Em_SetStatus4Timer(Em *em);
void Em_SetStatus2Timer(Em *em);
void EmAnteka_RunState(Em *em);
u8 Em_UpdateDamage(Em *em, u8 *out);
void Em_UpdatePendingScript(Em *em);
void Em_ClearHitReactMode(Em *em);
void Em_SetStatus5Timer(Em *em);
void Em_ResetPartHp(Em *em, u32 i);
void EmMosswine_RunState(Em *em);
void Effect0D_SpawnForEmScaled(Em *em, s32 a, s32 b, f32 scale);
u8 Em_SenseObjects(Em *em, EmObj *obj);
s32 Em_ScriptFlagsFrom(Em *em, u8 a);
void EmVelociprey_RunState(Em *em, s32 arg1);
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
extern EmVelocipreyAppearPoint EmVelociprey_AppearPtsFortress6[];
extern EmVelocipreyAppearPoint EmVelociprey_AppearPtsFortress6_PosY[];
extern EmVelocipreyAppearPoint EmVelociprey_AppearPtsFortress6_PosZ[];
extern EmVelocipreyAppearPoint EmVelociprey_AppearPtsFortress6_TargetX[];
extern EmVelocipreyAppearPoint EmVelociprey_AppearPtsFortress6_TargetY[];
extern EmVelocipreyAppearPoint EmVelociprey_AppearPtsFortress6_TargetZ[];
extern EmVelocipreyAppearPoint EmVelociprey_AppearPtsForestHills2[];
extern EmVelocipreyAppearPoint EmVelociprey_AppearPtsForestHills2_PosY[];
extern EmVelocipreyAppearPoint EmVelociprey_AppearPtsForestHills2_PosZ[];
extern EmVelocipreyAppearPoint EmVelociprey_AppearPtsForestHills2_TargetX[];
extern EmVelocipreyAppearPoint EmVelociprey_AppearPtsForestHills2_TargetY[];
extern EmVelocipreyAppearPoint EmVelociprey_AppearPtsForestHills2_TargetZ[];
extern EmVelocipreyAppearPoint EmVelociprey_AppearPtsForestHills1[];
extern EmVelocipreyAppearPoint EmVelociprey_AppearPtsForestHills1_PosY[];
extern EmVelocipreyAppearPoint EmVelociprey_AppearPtsForestHills1_PosZ[];
extern EmVelocipreyAppearPoint EmVelociprey_AppearPtsForestHills1_TargetX[];
extern EmVelocipreyAppearPoint EmVelociprey_AppearPtsForestHills1_TargetY[];
extern EmVelocipreyAppearPoint EmVelociprey_AppearPtsForestHills1_TargetZ[];
extern f32 EmVelociprey_AppearPtSnowyMountains3_PosX;
extern f32 EmVelociprey_AppearPtSnowyMountains3_PosY;
extern f32 EmVelociprey_AppearPtSnowyMountains3_PosZ;
extern f32 EmVelociprey_AppearPtSnowyMountains3_TargetX;
extern f32 EmVelociprey_AppearPtSnowyMountains3_TargetY;
extern f32 EmVelociprey_AppearPtSnowyMountains3_TargetZ;
extern EmVelocipreyAppearAt EmVelociprey_AppearPtsForestHills10[];
extern EmVelocipreyAppearAt EmVelociprey_AppearPtsForestHills9[];
extern EmVelocipreyAppearAt EmVelociprey_AppearPtsJungle2[];
extern EmVelocipreyAppearAt EmVelociprey_AppearPtsJungle3[];
extern EmVelocipreyAppearAt EmVelociprey_AppearPtsJungle4[];
extern EmVelocipreyAppearAt EmVelociprey_AppearPtsJungle6[];
extern EmVelocipreyAppearAt EmVelociprey_AppearPtsJungle9[];
extern EmVelocipreyAppearAt EmVelociprey_AppearPtsTower6[];
extern EmVelocipreyAppearAt EmVelociprey_AppearPtsTower7[];
extern EmVelocipreyAppearAt EmVelociprey_AppearPtsDesert2[];
extern EmVelocipreyAppearAt EmVelociprey_AppearPtsDesert4[];
extern EmVelocipreyAppearAt EmVelociprey_AppearPtsDesert7[];
extern EmVelocipreyAppearAt EmVelociprey_AppearPtsSwamp5[];
extern EmVelocipreyAppearAt EmVelociprey_AppearPtsSwamp8[];
extern EmVelocipreyAppearAt EmVelociprey_AppearPtsSwamp9[];
extern EmVelocipreyAppearAt EmVelociprey_AppearPtsVolcano3[];
extern EmVelocipreyAppearAt EmVelociprey_AppearPtsVolcano6[];
extern EmVelocipreyAppearAt EmVelociprey_AppearPtsBattleTown1[];
extern EmVelocipreyAppearAt EmVelociprey_AppearPtsBattleTown2[];
extern EmVelocipreyAppearAt EmVelociprey_AppearPtsBattleTown3[];
/* end of prototypes */

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
    StageObj_K13_Create(em, 0x22, 0);
    Effect01_Spawn((Player *) em, 0);
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
            StageObj_K09_Create(em);
            break;
        }
    }
}

/*
 * Starts a Khezu: Init, then in the story stage 0x422, until event flag
 * 0xA7 is set, places it at the scene's spot facing its goal; then
 * KHEZU_IDLE on the ground.
 */
void EmKhezu_Start(Em *em)
{
    EmKhezu_Init(em);
    if (em->type == EM_KIND_KHEZU || em->type == EM_KIND_RED_KHEZU) {
        switch (gQuest.stage) {
        case 0x422:
            if (Save_TestEventFlag(0xA7) == 0) {
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
            StageObj_K14_Create(em, 7, 0, 5.0f);
        }
        if ((EmMotion_PassedFrameScaled(em, 210.0f, 0U) != 0) || (EmMotion_PassedFrameScaled(em, 218.0f, 0U) != 0)) {
            StageObj_K14_Create(em, 7, 0, 2.5f);
        }
        if (EmMotion_PassedFrameScaled(em, 52.0f, 0U) != 0) {
            StageObj_K14_Create(em, 8, 0, 1.0f);
        }
        if ((EmMotion_PassedFrameScaled(em, 106.0f, 0U) != 0) || (EmMotion_PassedFrameScaled(em, 130.0f, 0U) != 0)) {
            StageObj_K14_Create(em, 6, 0, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 124.0f, 0U) != 0) {
            StageObj_K14_Create(em, 6, 1, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 170.0f, 0U) != 0) {
            StageObj_K14_Create(em, 0xF, 0, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 192.0f, 0U) != 0) {
            StageObj_K14_Create(em, 0x10, 0, 1.0f);
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
            StageObj_Stun_CreateEm(em, 1, 0x22, (f32) em->timer + em->motion[0].frameCount / 2.0f, 150.0f, 3.0f);
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
 * Em.unk8AD and escapes (KHEZU_FLY_ESCAPE_TRAP).
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
            em->unk8AD = 0;
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
            StageObj_Stun_CreateEm(em, 1, 0x22, (s32) ((f32) em->timer + (em->motion[0].frameCount / 2.0f)), 150.0f, 3.0f);
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
 * KHEZU_EVENT_STORY: the story scene of quest stage 0x422: waits, walks to
 * its goal, plays KHEZU_MOT_IDLE_ACTION_C, leaps across with the leap's
 * curves to a fixed point, plays KHEZU_MOT_ATK_ACTION_D and waits until
 * event flag 0xA7 is set. The monster can't be hit or sense anything
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
        if (Save_TestEventFlag(0xA7) == 1) {
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
 * is thrown to it (StageObj_K25_Create at EmKhezu_BaitThrowPt); it eats,
 * stands 30 frames, then Versus_RouteEmByTeam picks a pose. It can't be hit
 * meanwhile.
 */
void EmKhezu_VersusEatBait(Em *em, EmWork *work)
{
    EmKhezuVec throwPt;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        if (Versus_DefeatCamera(em) == 1) {
            throwPt = EmKhezu_BaitThrowPt;
            StageObj_K25_Create(&gPlayers[(u8) em->side - 1], em, 0, 0, &throwPt.x);
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
    case 0x422:
        if ((em->type == EM_KIND_KHEZU || em->type == EM_KIND_RED_KHEZU) && Area_DayId(em->area) == AREA_SNOWY_MOUNTAINS_3 && Save_TestEventFlag(0xA7) == 0) {
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
            StageObj_K14_Create(em, 0x1A, 1, 1.0f);
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
            StageObj_K14_Create(em, 0x1A, 0, 1.0f);
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
                StageObj_K14_Create(em, 0x1A, 1, 1.0f);
            }
            if ((EmMotion_InFrameRange(em, 0, 20.0f, 70.0f) != 0) && !(gScene.frame & 3)) {
                StageObj_K14_Create(em, 0x1A, 0, 1.0f);
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
                StageObj_K14_Create(em, 0x1A, 0, 1.0f);
            }
            if ((EmMotion_InFrameRange(em, 0, 20.0f, 70.0f) != 0) && !(gScene.frame & 3)) {
                StageObj_K14_Create(em, 0x1A, 1, 1.0f);
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
                Effect0D_SpawnForEmScaled(em, 2, 7, 5.0f);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_HOVER):
        if (EmMotion_PassedFrameScaled(em, 8.0f, 0) || EmMotion_PassedFrameScaled(em, 86.0f, 0) || EmMotion_PassedFrameScaled(em, 160.0f, 0)) {
            if (em->posY <= (1000.0f + em->floorY)) {
                StageObj_K14_Create(em, 1, 0, 1.0f);
            }
        }
        if (em->rage != 0) {
            if (EmMotion_InFrameRange(em, 0, 70.0f, 84.0f) || EmMotion_InFrameRange(em, 0, 146.0f, 156.0f) || EmMotion_InFrameRange(em, 0, 220.0f, 234.0f)) {
                if (!(gScene.frame & 3)) {
                    StageObj_K14_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
                    break;
                }
            }
        }
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_TAKE_OFF):
        if (EmMotion_PassedFrameScaled(em, 42.0f, 0) || EmMotion_PassedFrameScaled(em, 102.0f, 0) || EmMotion_PassedFrameScaled(em, 156.0f, 0)) {
            if (em->posY <= (1000.0f + em->floorY)) {
                StageObj_K14_Create(em, 1, 0, 1.0f);
            }
        }
        if ((em->rage != 0) && (EmMotion_PassedFrameScaled(em, 38.0f, 0) != 0)) {
            StageObj_K14_Create(em, 0x19, 5, 1.0f);
            StageObj_K14_Create(em, 0x19, 4, 1.0f);
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
            StageObj_K14_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
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
                StageObj_K14_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
            }
            if (EmMotion_InFrameRange(em, 0, 98.0f, 102.0f) != 0) {
                StageObj_K14_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
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
            StageObj_K14_Create(em, 0x1A, 1, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_SHOT_FAN):
        if (EmMotion_PassedFrameScaled(em, 160.0f, 0) != 0) {
            KhezuAttack_Create(em, 0x25);
        }
        if (EmMotion_PassedFrameScaled(em, 58.0f, 0) != 0) {
            EmFxSpark_SpawnTimed(em, 3, 0x72, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 172.0f, 0) != 0) {
            EmFxSpark_SpawnOnEm(em, 4);
            break;
        }
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_ALERT_IDLE):
        if ((em->rage != 0) && ((EmMotion_InFrameRange(em, 0, 10.0f, 90.0f) != 0) || (EmMotion_InFrameRange(em, 0, 122.0f, 260.0f) != 0) || (EmMotion_ReachedFrame(em, 0, 296.0f) != 0)) && !(gScene.frame & 3)) {
            StageObj_K14_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_TURN_IN_PLACE):
        if ((em->stateGroup != EM_GROUP_EVENT) && (EmMotion_PassedFrameScaled(em, 78.0f, 0) != 0)) {
            KhezuAttack_Create(em, 0x13);
            StageObj_K0F_CreateOnEm(em, 5, 4, 1.0f);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 86.0f, 298.0f) != 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_EAT):
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_FLINCH):
        if (EmMotion_PassedFrameScaled(em, 32.0f, 0) != 0) {
            Effect0D_SpawnForEm(em, 0x1B, 7);
        }
        if (EmMotion_PassedFrameScaled(em, 54.0f, 0) != 0) {
            Effect0D_SpawnForEm(em, 0x16, 7);
            break;
        }
        break;
    case EM_MOTION_ID(0, 0x3D):
        if (EmMotion_PassedFrameScaled(em, 24.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x1A, 7, 1.0f);
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
            StageObj_K14_Create(em, 0, 0, 0.9f);
            break;
        }
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_KNOCK_DOWN_A):
        EmKhezu_CamShakeOnFrame((s32) em, 0x1A, 2);
        if (EmMotion_PassedFrameScaled(em, 26.0f, 0) != 0) {
            StageObj_K14_Create(em, 0x12, 0x80, 1.0f);
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
            StageObj_K14_Create(em, 0x12, 0x81, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_STATUS4):
    case EM_MOTION_ID(0, KHEZU_MOT_FALL):
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_FALL_LAND):
        if (EmMotion_PassedFrameScaled(em, 8.0f, 0) != 0) {
            StageObj_K14_Create(em, 0xB, 0, 1.0f);
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
            StageObj_K14_Create(em, 1, 0, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, 0x5A):
        if ((EmMotion_PassedFrameScaled(em, 50.0f, 0) != 0) && (em->posY <= (1000.0f + em->floorY))) {
            StageObj_K14_Create(em, 9, 0, 1.0f);
            StageObj_K14_Create(em, 0xE, 0, 1.0f);
        }
        if ((EmMotion_PassedFrameScaled(em, 102.0f, 0) != 0) && (em->posY <= (1000.0f + em->floorY))) {
            StageObj_K14_Create(em, 9, 0, 1.0f);
            StageObj_K14_Create(em, 0xE, 0, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_TRAPPED):
        if (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0) {
            StageObj_K14_Create(em, 0xB, 0x80, 1.0f);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 4.0f, 60.0f) != 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_STRUGGLE):
        if (em->rage != 0) {
            if (EmMotion_InFrameRange(em, 0, 12.0f, 24.0f) || EmMotion_InFrameRange(em, 0, 46.0f, 90.0f)) {
                if (!(gScene.frame & 3)) {
                    StageObj_K14_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
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
            Effect0D_SpawnForEmScaled(em, 2, 7, 3.5f);
            break;
        }
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_IDLE_ACTION_C):
        if (EmMotion_PassedFrameScaled(em, 66.0f, 0) != 0) {
            KhezuAttack_Create(em, 2);
            StageObj_K0F_CreateOnEm(em, 5, 5, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_LEAP):
        if (EmMotion_PassedFrameScaled(em, 80.0f, 0) != 0) {
            KhezuAttack_Create(em, 0x1D);
        }
        if (EmMotion_PassedFrameScaled(em, 70.0f, 0) != 0) {
            StageObj_K14_Create(em, 2, 6, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_LEAP_LAND):
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            KhezuAttack_Create(em, 0x1E);
        }
        if (EmMotion_PassedFrameScaled(em, 12.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 2, 7, 3.5f);
            break;
        }
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_LEAP_QUICK):
        if (EmMotion_PassedFrameScaled(em, 32.0f, 0) != 0) {
            KhezuAttack_Create(em, 0x1F);
        }
        if (EmMotion_PassedFrameScaled(em, 20.0f, 0) != 0) {
            StageObj_K14_Create(em, 2, 6, 1.0f);
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
            Effect0D_SpawnForEmScaled(em, 0x1A, 3, 3.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 178.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x14, 3, 3.0f);
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
                EmFxSpark_SpawnOnEm(em, 9);
            }
        }
        if (em->rage != 0) {
            if ((EmMotion_InFrameRange(em, 0, 40.0f, 72.0f) != 0) && !(gScene.frame & 3)) {
                StageObj_K14_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
            }
            if (EmMotion_InFrameRange(em, 0, 102.0f, 106.0f) != 0) {
                StageObj_K14_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, KHEZU_MOT_ATK_ACTION_D):
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            EmFxSpark_SpawnOnEm(em, 0);
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
            Effect0D_SpawnForEmScaled(em, 2, 7, 3.5f);
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
    StageObj_K14_Create((Em *) em, 0xB, 0, 1.0f);
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

/*
 * The Velociprey family's motion sounds: footsteps, cries and thuds at
 * fixed frames of the layer-0 motion kept in work->prevMotion (one picked
 * at random by Sound_RandomSe). Called every frame by
 * EmVelociprey_MotionEffects. The monster comes as an s32 and is cast at
 * each call, which the match needs. EmDrome_MotionSounds is the dromes'
 * copy, with some other sounds.
 */
void EmVelociprey_MotionSounds(s32 em, EmWork *work)
{
    s16 prevMotion;

    prevMotion = work->prevMotion;
    switch (prevMotion) {
    case EM_MOTION_ID(0, VPREY_MOT_IDLE):
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xC8, Sound_RandomSe(0xE, 2, 0xE, 2), 0);
        break;
    case EM_MOTION_ID(0, VPREY_MOT_WALK):
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 2, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x20, 2, 0);
        break;
    case EM_MOTION_ID(0, VPREY_MOT_HOP):
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x14, 8, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x10, 2, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x14, 3, 0);
        break;
    case EM_MOTION_ID(0, VPREY_MOT_HOP_UP):
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 8, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 2, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 3, 0);
        break;
    case EM_MOTION_ID(0, VPREY_MOT_HOP_LAND):
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 2, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA, 1, 0);
        break;
    case EM_MOTION_ID(0, VPREY_MOT_TURN_LEFT):
    case EM_MOTION_ID(0, VPREY_MOT_TURN_RIGHT):
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, Sound_RandomSe(5, 2, 5, 2), 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1A, 2, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1A, 4, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2A, 2, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x30, 1, 0);
        break;
    case EM_MOTION_ID(0, VPREY_MOT_MOVE_ACTION_A):
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 2, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 4, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1A, 2, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x20, 1, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x40, 7, 0);
        break;
    case EM_MOTION_ID(0, VPREY_MOT_MOVE_ACTION_B):
    case EM_MOTION_ID(0, VPREY_MOT_MOVE_ACTION_C):
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xC, 2, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xC, 4, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1E, 2, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x24, 1, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x40, 7, 0);
        break;
    case EM_MOTION_ID(0, VPREY_MOT_ALERT):
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x44, 0x11, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA4, 0x12, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x22, 0x17, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x104, 0x17, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x120, 0x17, 0);
        break;
    case EM_MOTION_ID(0, VPREY_MOT_IDLE_B):
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x76, 0xE, 0);
        break;
    case EM_MOTION_ID(0, VPREY_MOT_LEAP):
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 2, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 3, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 8, 0);
        break;
    case EM_MOTION_ID(0, VPREY_MOT_LEAP_LAND):
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 2, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 1, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x24, 0, 0);
        break;
    case EM_MOTION_ID(0, VPREY_MOT_STRIKE):
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x28, 2, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3C, 2, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x76, 2, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1A, 0x10, 0);
        break;
    case EM_MOTION_ID(0, VPREY_MOT_POUNCE):
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 2, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 3, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 8, 0);
        break;
    case EM_MOTION_ID(0, VPREY_MOT_POUNCE_LAND):
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 2, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 1, 0);
        break;
    case EM_MOTION_ID(0, VPREY_MOT_QUICK_STRIKE):
        Em_PlaySeAtBoneOnFrame((Em *) em, 2, 0x10, 0);
        break;
    case EM_MOTION_ID(0, VPREY_MOT_SHOOT):
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3C, 0, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x80, 2, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x48, 0x1C, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3C, 8, 0);
        break;
    case EM_MOTION_ID(0, VPREY_MOT_FLINCH):
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 9, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1E, 2, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1C, 1, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x36, 8, 0);
        break;
    case EM_MOTION_ID(0, VPREY_MOT_KNOCKED_BACK):
    case EM_MOTION_ID(0, VPREY_MOT_KNOCKED_FORWARD):
    case EM_MOTION_ID(0, VPREY_MOT_KNOCKED_SIDE_B):
    case EM_MOTION_ID(0, VPREY_MOT_KNOCKED_SIDE_A):
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0xB, 0);
        break;
    case EM_MOTION_ID(0, VPREY_MOT_KNOCKED_BACK_LAND):
    case EM_MOTION_ID(0, VPREY_MOT_KNOCKED_FORWARD_LAND):
    case EM_MOTION_ID(0, VPREY_MOT_KNOCKED_SIDE_B_LAND):
    case EM_MOTION_ID(0, VPREY_MOT_KNOCKED_SIDE_A_LAND):
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0xF, 0);
        break;
    case EM_MOTION_ID(0, VPREY_MOT_GET_UP):
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x18, 3, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2C, 1, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x36, 2, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x58, 0, 0);
        break;
    case EM_MOTION_ID(0, VPREY_MOT_DAZED):
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xB4, 9, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x84, 0xB, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x34, 0xA, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x44, 1, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x56, 0, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x84, 1, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x8C, 0, 0);
        break;
    case EM_MOTION_ID(0, VPREY_MOT_COLLAPSE):
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x11, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x42, 0x11, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x82, 0x12, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x30, 0x17, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x7A, 0, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA2, 0xF, 0);
        break;
    case EM_MOTION_ID(0, VPREY_MOT_LIE):
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0xE, 0);
        break;
    case EM_MOTION_ID(0, VPREY_MOT_DIE):
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA, 0xD, 0);
        break;
    case EM_MOTION_ID(0, VPREY_MOT_STATUS4):
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xE, 0x19, 0);
        break;
    case EM_MOTION_ID(0, VPREY_MOT_IDLE_ACTION_A):
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x20, 0xC, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x44, 0xC, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x62, 6, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x14, 0, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA0, 0x17, 0);
        break;
    case EM_MOTION_ID(0, VPREY_MOT_CALL):
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA, 7, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x44, 7, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 1, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x7A, 1, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x86, 0, 0);
        break;
    case EM_MOTION_ID(0, VPREY_MOT_SHAKE_OFF):
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 5, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x62, 5, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 1, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3A, 0, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x6A, 1, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x9E, 0, 0);
        /* no break: the cases below are empty */
    case EM_MOTION_ID(0, VPREY_MOT_EVENT_IDLE):
    default:
        break;
    }
}

/*
 * Callback slots 0 and 1 (EmVt.init, update): makes the monster's effect
 * 01, as the other families do.
 */
void EmVelociprey_SpawnEffect01(u8 *em)
{
    Effect01_Spawn((Player *) em, 0);
}

/*
 * Sets up a Velociprey, Genprey, Ioprey or Giaprey. Outside a quest
 * (gQuest.stage 0) it is placed on a test grid by its slot number (rows of
 * 8, 150 apart, from a corner that depends on the area). Then the kind's
 * motion speed, the idle motion, full hp from Em_ComputeMaxHp, the spawn
 * position and heading kept for EmVelociprey_Respawn, and the parts. A pack
 * member (Em.spawnParam 1) waits for senses and links to its leader
 * (Em_FindLink); an Ioprey of variant 1 starts in combat.
 */
void EmVelociprey_Init(Em *em)
{
    EmVelocipreyWork *w = (EmVelocipreyWork *)&em->work;
    s16 hp;

    if (gQuest.stage == 0) {
        switch (gScene.area) {
        case AREA_SNOWY_MOUNTAINS_NIGHT_9:
            em->posX = 9500.0f + 150.0f * (f32) (em->slotNo / 8);
            em->posY = 0.0f;
            em->posZ = 9200.0f + 150.0f * (f32) (em->slotNo & 7);
            break;
        case 0xD: /* an area AreaId doesn't name */
            em->posX = 5000.0f + 150.0f * (f32) (em->slotNo / 8);
            em->posY = 0.0f;
            em->posZ = 9000.0f + 150.0f * (f32) (em->slotNo & 7);
            break;
        case AREA_FOREST_HILLS_4:
            em->posX = 10000.0f + 150.0f * (f32) (em->slotNo / 8);
            em->posY = 0.0f;
            em->posZ = 10000.0f + 150.0f * (f32) (em->slotNo & 7);
            em->rotY = 0;
            break;
        case AREA_FOREST_HILLS_2:
            em->posX = 8400.0f + 150.0f * (f32) (em->slotNo / 8);
            em->posY = 0.0f;
            em->posZ = 9000.0f + 150.0f * (f32) (em->slotNo & 7);
            em->rotY = 0x3000;
            break;
        default:
            em->posX = 2400.0f + 150.0f * (f32) em->slotNo;
            em->posY = 0.0f;
            em->posZ = 3600.0f;
            break;
        }
    }
    switch (em->type) {
    case EM_KIND_GENPREY:
        em->motionSpeed = 1.2f;
        break;
    case EM_KIND_VELOCIPREY:
    case EM_KIND_GIAPREY:
        em->motionSpeed = 1.0f;
        break;
    case EM_KIND_IOPREY:
        em->motionSpeed = 1.1f;
        break;
    }
    Em_SetMotionRow(em, VPREY_MOT_IDLE, 0, 0);
    em->stance = EM_STANCE_GROUND;
    hp = Em_ComputeMaxHp(em, 0);
    em->hp = hp;
    em->hpMax = hp;
    em->scriptPending = 0;
    em->blind = 0;
    w->callActive = 0;
    w->spawnPos[0] = em->posX;
    w->spawnPos[1] = em->posY;
    w->spawnPos[2] = em->posZ;
    w->spawnRotY = em->rotY;
    Em_InitParts(em);
    em->remote = 0;
    w->hunted = 0;
    switch (em->spawnParam) {
    case 1:
        em->senseDelay = 5;
        Em_FindLink(em);
        break;
    }
    if (em->type == EM_KIND_IOPREY && em->variant == 1) {
        Em_SetCombat(em, 1, gEmCombatInit[em->type]);
    }
}

/*
 * Init, then VPREY_IDLE_SPAWN, or VPREY_IDLE_PACK_WAIT for a pack member
 * (Em.spawnParam 1). GmEm's setup calls it for the four kinds.
 */
void EmVelociprey_Start(Em *em)
{
    EmVelociprey_Init(em);
    EmVelociprey_ChangeState(em, EM_GROUP_IDLE, VPREY_IDLE_SPAWN, EM_SYNC_LOCAL);
    switch (em->spawnParam) {
    case 1:
        EmVelociprey_ChangeState(em, EM_GROUP_IDLE, VPREY_IDLE_PACK_WAIT, EM_SYNC_LOCAL);
        break;
    }
}

/* Callback slot 5 (EmVt.appear): Init again, the state kept. */
void EmVelociprey_Reinit(Em *em)
{
    EmVelociprey_Init(em);
}

/*
 * Brings a hunted raptor back where it first stood
 * (VPREY_DEFEAT_FADE_RESPAWN's end): resets the monster (GmEm_ResetState),
 * full hp, the idle motion, VPREY_IDLE_SPAWN or, for a pack member,
 * VPREY_IDLE_PACK_WAIT, and the spawn position and heading Init kept.
 */
void EmVelociprey_Respawn(Em *em)
{
    EmVelocipreyWork *w = (EmVelocipreyWork *)&em->work;

    s16 fullHp;

    GmEm_ResetState(em);
    Em_SetMotionRow(em, VPREY_MOT_IDLE, 0, 0);
    em->stance = EM_STANCE_GROUND;
    EmVelociprey_ChangeState(em, EM_GROUP_IDLE, VPREY_IDLE_SPAWN, EM_SYNC_LOCAL);
    fullHp = Em_ComputeMaxHp(em, 0);
    em->hp = fullHp;
    em->hpMax = fullHp;
    em->blind = 0;
    w->callActive = 0;
    Em_InitParts(em);
    em->remote = 0;
    w->hunted = 0;
    switch (em->spawnParam) {
    case 1:
        em->senseDelay = 5;
        Em_FindLink(em);
        EmVelociprey_ChangeState(em, EM_GROUP_IDLE, VPREY_IDLE_PACK_WAIT, EM_SYNC_LOCAL);
        break;
    }
    em->posX = w->spawnPos[0];
    em->posY = w->spawnPos[1];
    em->posZ = w->spawnPos[2];
    em->prevPosX = em->posX;
    em->prevPosY = em->posY;
    em->prevPosZ = em->posZ;
    em->rotY = w->spawnRotY;
    em->scriptPending = 0;
}

/*
 * Ends a state: the kind's motion speed, on the ground, no look-at, and
 * VPREY_IDLE with the script asked for the next state. The two last
 * arguments are unused.
 */
void EmVelociprey_EndState(Em *em, s32 unused1, s32 unused2)
{
    switch (em->type) {
    case EM_KIND_GENPREY:
        em->motionSpeed = 1.2f;
        break;
    case EM_KIND_VELOCIPREY:
    case EM_KIND_GIAPREY:
        em->motionSpeed = 1.0f;
        break;
    case EM_KIND_IOPREY:
        em->motionSpeed = 1.1f;
        break;
    }
    em->stance = EM_STANCE_GROUND;
    em->lookAtTarget = 0;
    em->scriptPending = 1;
    EmVelociprey_ChangeState(em, EM_GROUP_IDLE, VPREY_IDLE, EM_SYNC_LOCAL);
}

/*
 * VPREY_IDLE_STAND: the idle motion for 30 to 120 frames (by Em.stateRand),
 * then asks the script.
 */
void EmVelociprey_IdleStand(Em *em, EmWork *work)
{
    em->motionSpeed = 1.2f;
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = ((em->stateRand & 3) * 30) + 30;
        if (em->motionId[0] != EM_MOTION_ID(0, VPREY_MOT_IDLE)) {
            Em_SetMotion(em, VPREY_MOT_IDLE, 0, 0);
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
 * VPREY_IDLE and VPREY_IDLE_WAIT: the idle motion (unless it is already
 * playing); once it ends, asks the script every frame until the script
 * changes the state.
 */
void EmVelociprey_Idle(Em *em, EmWork *work)
{
    em->motionSpeed = 1.2f;
    switch (em->step) {
    case 0:
        em->step++;
        if (em->motionId[0] != EM_MOTION_ID(0, VPREY_MOT_IDLE)) {
            Em_SetMotion(em, VPREY_MOT_IDLE, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->scriptPending = 1;
        }
        break;
    }
}

/* VPREY_IDLE_ACTION_A: one motion, then EmVelociprey_EndState. */
void EmVelociprey_IdleActionA(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_IDLE_ACTION_A, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * VPREY_IDLE_PACK_CALL: the call motion; at its frame 40 the monster puts
 * its call object (EmVelocipreyWork.callObj, radius 1000, kind 0) at its
 * position in the sense list, where the other raptors' Em_SenseObjects
 * notices it. The object is removed when the motion ends or the state
 * changes.
 */
void EmVelociprey_PackCall(Em *em, EmWork *work)
{
    EmVelocipreyWork *w = (EmVelocipreyWork *)work;

    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_CALL, 0, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            Vec_Set(w->callObj.pos, em->posX, em->posY, em->posZ);
            *(s32 *)&w->callObj.radius = 0x447A0000; /* 1000.0f, stored as its bits */
            w->callObj.area = (s16) *(u8 *)&em->area;
            w->callObj.kind = 0;
            ObjListA70_Add(&w->callObj);
            w->callActive = 1;
        }
        if (em->motion[0].playing == 0) {
            if (w->callActive != 0) {
                ObjListA70_Remove(&w->callObj);
                w->callActive = 0;
            }
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/* VPREY_IDLE_SHAKE_OFF: the shake-off motion, then EmVelociprey_EndState. */
void EmVelociprey_ShakeOff(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_SHAKE_OFF, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * VPREY_IDLE_ALERT, the combat idle: the alert motion once, then
 * EmVelociprey_EndState.
 */
void EmVelociprey_IdleAlert(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_ALERT, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * VPREY_IDLE_GET_UP, after the four knock-backs: the get-up motion, then
 * EmVelociprey_EndState.
 */
void EmVelociprey_GetUp(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_GET_UP, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * VPREY_IDLE_STATUS2: blinded and waking on a hit (EM_HIT_REACT_WAKE)
 * throughout; collapses (Em_BeginStatus2), then lies until the status timer
 * runs out and goes to VPREY_IDLE_STATUS2_END.
 */
void EmVelociprey_Status2Collapse(Em *em, EmWork *work)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_COLLAPSE, 0, 0);
        Em_BeginStatus2(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, VPREY_MOT_LIE, 0, 0);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            EmVelociprey_ChangeState(em, EM_GROUP_IDLE, VPREY_IDLE_STATUS2_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * VPREY_IDLE_STATUS2_END: ends status 2 and gets up, blinded until the
 * motion ends, then EmVelociprey_EndState.
 */
void EmVelociprey_Status2End(Em *em, EmWork *work)
{
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_GET_UP, 0, 0);
        Em_EndStatus2(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->blind = 0;
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * VPREY_IDLE_BLINDED: the dazed motion until Em.blindTimer runs out, then
 * EmVelociprey_EndState.
 */
void EmVelociprey_Blinded(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        if (em->motionId[0] != EM_MOTION_ID(0, VPREY_MOT_DAZED)) {
            Em_SetMotion(em, VPREY_MOT_DAZED, 0, 0);
        }
        break;
    case 1:
        if (em->blindTimer == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * VPREY_IDLE_PACK_WAIT: a pack member waiting for its leader, unaware and
 * unpushable (Em.senseDelay and Em.noPushTimer held; EmVelociprey_Update
 * then keeps it unhittable). Plays VPREY_MOT_IDLE_B and asks the script
 * when it ends.
 */
void EmVelociprey_PackWait(Em *em, EmWork *work)
{
    em->motionSpeed = 1.2f;
    em->senseDelay = 5;
    em->noPushTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        if (em->motionId[0] != EM_MOTION_ID(0, VPREY_MOT_IDLE_B)) {
            Em_SetMotion(em, VPREY_MOT_IDLE_B, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->scriptPending = 1;
        }
        break;
    }
}

/*
 * VPREY_IDLE_STATUS4_END: ends status 4 with the alert motion, then
 * EmVelociprey_EndState.
 */
void EmVelociprey_Status4End(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_ALERT, 0, 0);
        Em_EndStatus4(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * VPREY_IDLE_SPAWN, after Start, Respawn and a reappearance: plays
 * VPREY_MOT_IDLE_B unless the idle motion is playing, then asks the script.
 */
void EmVelociprey_IdleSpawn(Em *em, EmWork *work)
{
    em->motionSpeed = 1.2f;
    switch (em->step) {
    case 0:
        em->step++;
        if (em->motionId[0] != EM_MOTION_ID(0, VPREY_MOT_IDLE)) {
            Em_SetMotion(em, VPREY_MOT_IDLE_B, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->scriptPending = 1;
        }
        break;
    }
}

/*
 * VPREY_IDLE_STUN_END, when the stun ends (EmVelociprey_SetIdleState ends
 * status 5): the shake-off motion, then EmVelociprey_EndState.
 */
void EmVelociprey_StunEnd(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, VPREY_MOT_SHAKE_OFF, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/* Runs the state of group 0, EM_GROUP_IDLE. */
void EmVelociprey_RunIdleState(Em *em, EmWork *work)
{
    switch (em->state) {
    case VPREY_IDLE_STAND:
        EmVelociprey_IdleStand(em, work);
        break;
    case VPREY_IDLE:
        EmVelociprey_Idle(em, work);
        break;
    case VPREY_IDLE_ACTION_A:
        EmVelociprey_IdleActionA(em, work);
        break;
    case VPREY_IDLE_PACK_CALL:
        EmVelociprey_PackCall(em, work);
        break;
    case VPREY_IDLE_SHAKE_OFF:
        EmVelociprey_ShakeOff(em, work);
        break;
    case VPREY_IDLE_ALERT:
        EmVelociprey_IdleAlert(em, work);
        break;
    case VPREY_IDLE_GET_UP:
        EmVelociprey_GetUp(em, work);
        break;
    case VPREY_IDLE_STATUS2:
        EmVelociprey_Status2Collapse(em, work);
        break;
    case VPREY_IDLE_STATUS2_END:
        EmVelociprey_Status2End(em, work);
        break;
    case VPREY_IDLE_BLINDED:
        EmVelociprey_Blinded(em, work);
        break;
    case VPREY_IDLE_PACK_WAIT:
        EmVelociprey_PackWait(em, work);
        break;
    case VPREY_IDLE_STATUS4_END:
        EmVelociprey_Status4End(em, work);
        break;
    case VPREY_IDLE_SPAWN:
        EmVelociprey_IdleSpawn(em, work);
        break;
    case VPREY_IDLE_WAIT:
        EmVelociprey_Idle(em, work);
        break;
    case VPREY_IDLE_STUN_END:
        EmVelociprey_StunEnd(em, work);
        
    default:
        break;
    }
}

/*
 * VPREY_MOVE_TURN: turns to the target with the turn row
 * EmVelociprey_TurnParams[0], then EmVelociprey_EndState.
 */
void EmVelociprey_TurnToTarget(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_StartTurnToTarget(em, &EmVelociprey_TurnParams[0]);
        break;
    case 1:
        if (Em_TurnStep(em, &EmVelociprey_TurnParams[0]) == 1) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * VPREY_MOVE_WALK: walks to the goal (Em_StepToGoal), counting frames. At
 * the goal, a local monster that walked over 60 frames and ended within 100
 * of where it started (it made no headway) hops forward
 * (VPREY_HOP_FORWARD); otherwise EmVelociprey_EndState.
 */
void EmVelociprey_Walk(Em *em, EmWork *work)
{
    f32 start[3];

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, VPREY_MOT_WALK, 0, 0);
        work->startXB = em->posX;
        work->startZB = em->posZ;
        work->walkFramesB = 0;
        break;
    case 1:
        work->walkFramesB++;
        if (Em_StepToGoal(em, 0x40, 0) == 1) {
            if (em->remote == 0) {
                start[0] = work->startXB;
                start[1] = em->posY;
                start[2] = work->startZB;
                if (Vec_DistanceXZ(&em->posX, start) < 100.0f && work->walkFramesB > 60) {
                    EmVelociprey_ChangeState(em, EM_GROUP_FLY, VPREY_HOP_FORWARD, EM_SYNC_OWNER_FLAGGED);
                    break;
                }
                EmVelociprey_EndState(em, 0, 0);
            } else {
                EmVelociprey_EndState(em, 0, 0);
            }
        }
        break;
    }
}

/*
 * VPREY_MOVE_ACTION_A: one motion (dust at frame 12), then
 * EmVelociprey_EndState.
 */
void EmVelociprey_MoveActionA(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_MOVE_ACTION_A, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/* VPREY_MOVE_ACTION_B: one motion, then EmVelociprey_EndState. */
void EmVelociprey_MoveActionB(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_MOVE_ACTION_B, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/* VPREY_MOVE_ACTION_C: one motion, then EmVelociprey_EndState. */
void EmVelociprey_MoveActionC(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_MOVE_ACTION_C, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * VPREY_MOVE_TURN_ALERT, the turn in combat: as EmVelociprey_TurnToTarget
 * with the turn row EmVelociprey_TurnParams[1].
 */
void EmVelociprey_TurnToTargetAlert(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_StartTurnToTarget(em, &EmVelociprey_TurnParams[1]);
        break;
    case 1:
        if (Em_TurnStep(em, &EmVelociprey_TurnParams[1]) == 1) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/* Runs the state of group 1, EM_GROUP_MOVE. */
void EmVelociprey_RunMoveState(Em *em, EmWork *work)
{
    switch (em->state) {
    case VPREY_MOVE_TURN:
        EmVelociprey_TurnToTarget(em, work);
        break;
    case VPREY_MOVE_WALK:
        EmVelociprey_Walk(em, work);
        break;
    case VPREY_MOVE_ACTION_A:
        EmVelociprey_MoveActionA(em, work);
        break;
    case VPREY_MOVE_ACTION_B:
        EmVelociprey_MoveActionB(em, work);
        break;
    case VPREY_MOVE_ACTION_C:
        EmVelociprey_MoveActionC(em, work);
        break;
    case VPREY_MOVE_TURN_ALERT:
        EmVelociprey_TurnToTargetAlert(em, work);
        
    default:
        break;
    }
}

/*
 * VPREY_HOP_FORWARD: the hop motion; from its frame 10 the monster is
 * airborne on the curves VPREY_CURVE_HOP_FORWARD (up and forward) until
 * they end at floor height; it lands there, then EmVelociprey_EndState.
 */
void EmVelociprey_HopForward(Em *em, EmWork *work)
{
    s32 curveEnded;

    curveEnded = 0;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, VPREY_MOT_HOP, 0, 0);
        EmVelociprey_StartCurves(em, VPREY_CURVE_HOP_FORWARD);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 10.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            curveEnded = EmVelociprey_UpdateCurves(em) & 0xFF;
        }
        if ((curveEnded != 0) && (em->posY <= em->floorY)) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            Em_SetMotion(em, VPREY_MOT_HOP_LAND, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * VPREY_HOP_UP: as EmVelociprey_HopForward on the spot (VPREY_CURVE_HOP_UP,
 * airborne from frame 22), without setting the height on landing.
 */
void EmVelociprey_HopUp(Em *em, EmWork *work)
{
    s32 curveEnded;

    curveEnded = 0;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, VPREY_MOT_HOP_UP, 0, 0);
        EmVelociprey_StartCurves(em, VPREY_CURVE_HOP_UP);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 22.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            curveEnded = EmVelociprey_UpdateCurves(em) & 0xFF;
        }
        if ((curveEnded != 0) && (em->posY <= em->floorY)) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, VPREY_MOT_HOP_LAND, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * VPREY_HOP_ARC: a hop on a set arc (up 60, forward 50, gravity 4, drag 1;
 * falling at most 50, forward at least 20), unaware throughout; lands, then
 * EmVelociprey_EndState. The VPREY_EVENT2 hops are the same with other
 * speeds.
 */
void EmVelociprey_HopArc(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, VPREY_MOT_HOP, 0, 0);
        em->stance = EM_STANCE_GROUND;
        Player_ClearVelocity(em);
        em->velY = 60.0f;
        em->velZ = 50.0f;
        em->accelY = -4.0f;
        em->accelZ = -1.0f;
        work->accelAngX = 0;
        work->accelAngZ = 0;
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 10.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            work->accelAngY = em->rotY;
            Em_MoveRotatedAccel(em, &work->accelAngX);
        }
        if (em->velY < -50.0f) {
            em->velY = -50.0f;
            em->accelY = 0.0f;
        }
        if (em->velZ < 20.0f) {
            em->velZ = 20.0f;
            em->accelZ = 0.0f;
        }
        if ((em->velY < 0.0f) && (em->posY <= em->floorY)) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            Em_SetMotion(em, VPREY_MOT_HOP_LAND, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/* Runs the state of group 2 (EM_GROUP_FLY): the hops. */
void EmVelociprey_RunHopState(Em *em, EmWork *work)
{
    switch (em->state) {
    case VPREY_HOP_FORWARD:
        EmVelociprey_HopForward(em, work);
        break;
    case VPREY_HOP_UP:
        EmVelociprey_HopUp(em, work);
        break;
    case VPREY_HOP_ARC:
        EmVelociprey_HopArc(em, work);
        break;
    }
}

/*
 * VPREY_ATK_STRIKE_A: the strike motion with its hit at frame 40
 * (VelocipreyAttack row 1, the Ioprey's 0xB), turning to the goal until
 * frame 68.
 */
void EmVelociprey_AttackStrikeA(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_STRIKE, 0, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            if (em->type == EM_KIND_IOPREY) {
                VelocipreyAttack_Create(em, 0xB);
            } else {
                VelocipreyAttack_Create(em, 1);
            }
        }
        if ((em->hasGoal != 0) && (EmMotion_ReachedFrame(em, 0, 68.0f) == 0)) {
            Em_TurnToGoal(em, 0x80, 0);
        }
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/* VPREY_ATK_NONE: ends at once. */
void EmVelociprey_AttackNone(Em *em)
{
    EmVelociprey_EndState(em, 0, 0);
}

/*
 * VPREY_ATK_LEAP_MEDIUM_A: a leap (up 75, forward 50) airborne from frame
 * 22, turning to the goal in the air; the hit at frame 40 by kind
 * (VelocipreyAttack row 3, the Genprey's 0xC, the Ioprey's 0xD). A wall in
 * front holds the forward speed (EmVelocipreyWork.wallVelZ) for the move;
 * lands, then EmVelociprey_EndState.
 */
void EmVelociprey_AttackLeapMediumA(Em *em, EmWork *work)
{
    EmVelocipreyWork *w = (EmVelocipreyWork *)work;
    u8 inFlight;

    inFlight = 0;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_LEAP, 0, 0);
        Player_ClearVelocity(em);
        em->velY = 75.0f;
        em->accelY = -10.0f;
        em->velZ = 50.0f;
        em->accelZ = -1.5f;
        work->pitchStep = 0;
        work->turnSpeed = 0;
        work->rollSpeed = 0;
        work->unk048 = 0;
        work->unk04C = 0;
        w->unk050 = 0;
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            switch (em->type) {
            case EM_KIND_VELOCIPREY:
            case EM_KIND_GIAPREY:
                VelocipreyAttack_Create(em, 3);
                break;
            case EM_KIND_GENPREY:
                VelocipreyAttack_Create(em, 0xC);
                break;
            case EM_KIND_IOPREY:
                VelocipreyAttack_Create(em, 0xD);
                break;
            }
        }
        if (EmMotion_ReachedFrame(em, 0, 22.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            work->accelAngX = 0;
            work->accelAngY = em->rotY;
            work->accelAngZ = 0;
            inFlight = 1;
            if (em->wallHitDirs & 0xF000000F) {
                w->wallVelZ = em->velZ;
                em->velZ = 0.0f;
            }
            Em_MoveRotatedAccel(em, &work->accelAngX);
            if (w->wallVelZ != 0.0f) {
                em->velZ = w->wallVelZ;
                w->wallVelZ = 0.0f;
            }
            if (em->velZ < 10.0f) {
                em->velZ = 10.0f;
                em->accelZ = 0.0f;
            }
        }
        if (em->hasGoal != 0 && em->stance == EM_STANCE_AIR) {
            Em_TurnToGoal(em, 0x100, 0);
        }
        if (inFlight && em->posY <= em->floorY) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            Em_SetMotion(em, VPREY_MOT_LEAP_LAND, 0, 0);
        }
        break;
    case EM_KIND_FATALIS:
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * VPREY_ATK_QUICK_STRIKE_A: the quick strike with its hit at frame 16
 * (VelocipreyAttack row 5, the Ioprey's 0xE).
 */
void EmVelociprey_AttackQuickStrikeA(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_QUICK_STRIKE, 0, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 16.0f, 0U) != 0) {
            switch (em->type) {
            case EM_KIND_VELOCIPREY:
            case EM_KIND_GIAPREY:
                VelocipreyAttack_Create(em, 5);
                break;
            case EM_KIND_GENPREY:
                VelocipreyAttack_Create(em, 5);
                break;
            case EM_KIND_IOPREY:
                VelocipreyAttack_Create(em, 0xE);
                break;
            }
        }
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * VPREY_ATK_SHOT_A and VPREY_ATK_SHOT_A_FAST: at frame 80 of the shoot
 * motion, an EmRangedAttack of type 9 from bone 0xF, aimed slightly down;
 * fastShot picks its faster variant.
 */
void EmVelociprey_AttackShotA(Em *em, EmWork *work, s32 fastShot)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_SHOOT, 0, 0);
        break;
    case 1:
        switch (fastShot) {
        case 0:
        default:
            if (EmMotion_PassedFrameScaled(em, 80.0f, 0U) != 0) {
                EmRangedAttack_CreateAtBone(em, 0xF, 9, 0, 0xF8E5, 0);
            }
            break;
        case 1:
            if (EmMotion_PassedFrameScaled(em, 80.0f, 0U) != 0) {
                EmRangedAttack_CreateAtBone(em, 0xF, 9, 2, 0xF8E5, 0);
            }
            break;
        }
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * VPREY_ATK_LEAP_LONG_A: as EmVelociprey_AttackLeapMediumA, longer (forward
 * 100, slowing by 0.75), with VelocipreyAttack row 5 (the Genprey's 0xC,
 * the Ioprey's 0xF).
 */
void EmVelociprey_AttackLeapLongA(Em *em, EmWork *work)
{
    EmVelocipreyWork *w = (EmVelocipreyWork *)work;
    u8 inFlight;

    inFlight = 0;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_LEAP, 0, 0);
        Player_ClearVelocity(em);
        em->velY = 75.0f;
        em->accelY = -10.0f;
        em->velZ = 100.0f;
        em->accelZ = -0.75f;
        work->pitchStep = 0;
        work->turnSpeed = 0;
        work->rollSpeed = 0;
        work->unk048 = 0;
        work->unk04C = 0;
        w->unk050 = 0;
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            switch (em->type) {
            case EM_KIND_VELOCIPREY:
            case EM_KIND_GIAPREY:
                VelocipreyAttack_Create(em, 5);
                break;
            case EM_KIND_GENPREY:
                VelocipreyAttack_Create(em, 0xC);
                break;
            case EM_KIND_IOPREY:
                VelocipreyAttack_Create(em, 0xF);
                break;
            }
        }
        if (EmMotion_ReachedFrame(em, 0, 22.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            work->accelAngX = 0;
            work->accelAngY = em->rotY;
            work->accelAngZ = 0;
            inFlight = 1;
            if (em->wallHitDirs & 0xF000000F) {
                w->wallVelZ = em->velZ;
                em->velZ = 0.0f;
            }
            Em_MoveRotatedAccel(em, &work->accelAngX);
            if (w->wallVelZ != 0.0f) {
                em->velZ = w->wallVelZ;
                w->wallVelZ = 0.0f;
            }
            if (em->velZ < 10.0f) {
                em->velZ = 10.0f;
                em->accelZ = 0.0f;
            }
        }
        if (em->hasGoal != 0 && em->stance == EM_STANCE_AIR) {
            Em_TurnToGoal(em, 0x100, 0);
        }
        if (inFlight && em->posY <= em->floorY) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            Em_SetMotion(em, VPREY_MOT_LEAP_LAND, 0, 0);
        }
        break;
    case EM_KIND_FATALIS:
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * VPREY_ATK_LEAP_HIGH_A: as EmVelociprey_AttackLeapMediumA, higher (up 150,
 * gravity 20, forward 25), with VelocipreyAttack row 5 (the Genprey's 0xC,
 * the Ioprey's 0x10).
 */
void EmVelociprey_AttackLeapHighA(Em *em, EmWork *work)
{
    EmVelocipreyWork *w = (EmVelocipreyWork *)work;
    u8 inFlight;

    inFlight = 0;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_LEAP, 0, 0);
        Player_ClearVelocity(em);
        em->velY = 150.0f;
        em->accelY = -20.0f;
        em->velZ = 25.0f;
        em->accelZ = -3.0f;
        work->pitchStep = 0;
        work->turnSpeed = 0;
        work->rollSpeed = 0;
        work->unk048 = 0;
        work->unk04C = 0;
        w->unk050 = 0;
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            switch (em->type) {
            case EM_KIND_VELOCIPREY:
            case EM_KIND_GIAPREY:
                VelocipreyAttack_Create(em, 5);
                break;
            case EM_KIND_GENPREY:
                VelocipreyAttack_Create(em, 0xC);
                break;
            case EM_KIND_IOPREY:
                VelocipreyAttack_Create(em, 0x10);
                break;
            }
        }
        if (EmMotion_ReachedFrame(em, 0, 22.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            work->accelAngX = 0;
            work->accelAngY = em->rotY;
            work->accelAngZ = 0;
            inFlight = 1;
            if (em->wallHitDirs & 0xF000000F) {
                w->wallVelZ = em->velZ;
                em->velZ = 0.0f;
            }
            Em_MoveRotatedAccel(em, &work->accelAngX);
            if (w->wallVelZ != 0.0f) {
                em->velZ = w->wallVelZ;
                w->wallVelZ = 0.0f;
            }
            if (em->velZ < 10.0f) {
                em->velZ = 10.0f;
                em->accelZ = 0.0f;
            }
        }
        if (em->hasGoal != 0 && em->stance == EM_STANCE_AIR) {
            Em_TurnToGoal(em, 0x100, 0);
        }
        if (inFlight && em->posY <= em->floorY) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            Em_SetMotion(em, VPREY_MOT_LEAP_LAND, 0, 0);
        }
        break;
    case EM_KIND_FATALIS:
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * VPREY_ATK_STRIKE_B: as EmVelociprey_AttackStrikeA with VelocipreyAttack
 * row 7, turning to the goal throughout.
 */
void EmVelociprey_AttackStrikeB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_STRIKE, 0, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            VelocipreyAttack_Create(em, 7);
        }
        if (em->hasGoal != 0) {
            Em_TurnToGoal(em, 0x80, 0);
        }
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * VPREY_ATK_LEAP_MEDIUM_B: as EmVelociprey_AttackLeapMediumA with
 * VelocipreyAttack row 9 for every kind.
 */
void EmVelociprey_AttackLeapMediumB(Em *em, EmWork *work)
{
    EmVelocipreyWork *w = (EmVelocipreyWork *)work;
    u8 inFlight;

    inFlight = 0;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_LEAP, 0, 0);
        Player_ClearVelocity(em);
        em->velY = 75.0f;
        em->accelY = -10.0f;
        em->velZ = 50.0f;
        em->accelZ = -1.5f;
        work->pitchStep = 0;
        work->turnSpeed = 0;
        work->rollSpeed = 0;
        work->unk048 = 0;
        work->unk04C = 0;
        w->unk050 = 0;
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            VelocipreyAttack_Create(em, 9);
        }
        if (EmMotion_ReachedFrame(em, 0, 22.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            work->accelAngX = 0;
            work->accelAngY = em->rotY;
            work->accelAngZ = 0;
            inFlight = 1;
            if (em->wallHitDirs & 0xF000000F) {
                w->wallVelZ = em->velZ;
                em->velZ = 0.0f;
            }
            Em_MoveRotatedAccel(em, &work->accelAngX);
            if (w->wallVelZ != 0.0f) {
                em->velZ = w->wallVelZ;
                w->wallVelZ = 0.0f;
            }
            if (em->velZ < 10.0f) {
                em->velZ = 10.0f;
                em->accelZ = 0.0f;
            }
        }
        if (em->hasGoal != 0 && em->stance == EM_STANCE_AIR) {
            Em_TurnToGoal(em, 0x100, 0);
        }
        if (inFlight && em->posY <= em->floorY) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            Em_SetMotion(em, VPREY_MOT_LEAP_LAND, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * VPREY_ATK_QUICK_STRIKE_B: as EmVelociprey_AttackQuickStrikeA with
 * VelocipreyAttack row 0xA for every kind.
 */
void EmVelociprey_AttackQuickStrikeB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_QUICK_STRIKE, 0, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 16.0f, 0U) != 0) {
            VelocipreyAttack_Create(em, 0xA);
        }
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * VPREY_ATK_LEAP_LONG_B: as EmVelociprey_AttackLeapLongA with
 * VelocipreyAttack row 9 for every kind.
 */
void EmVelociprey_AttackLeapLongB(Em *em, EmWork *work)
{
    EmVelocipreyWork *w = (EmVelocipreyWork *)work;
    u8 inFlight;

    inFlight = 0;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_LEAP, 0, 0);
        Player_ClearVelocity(em);
        em->velY = 75.0f;
        em->accelY = -10.0f;
        em->velZ = 100.0f;
        em->accelZ = -0.75f;
        work->pitchStep = 0;
        work->turnSpeed = 0;
        work->rollSpeed = 0;
        work->unk048 = 0;
        work->unk04C = 0;
        w->unk050 = 0;
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            VelocipreyAttack_Create(em, 9);
        }
        if (EmMotion_ReachedFrame(em, 0, 22.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            work->accelAngX = 0;
            work->accelAngY = em->rotY;
            work->accelAngZ = 0;
            inFlight = 1;
            if (em->wallHitDirs & 0xF000000F) {
                w->wallVelZ = em->velZ;
                em->velZ = 0.0f;
            }
            Em_MoveRotatedAccel(em, &work->accelAngX);
            if (w->wallVelZ != 0.0f) {
                em->velZ = w->wallVelZ;
                w->wallVelZ = 0.0f;
            }
            if (em->velZ < 10.0f) {
                em->velZ = 10.0f;
                em->accelZ = 0.0f;
            }
        }
        if (em->hasGoal != 0 && em->stance == EM_STANCE_AIR) {
            Em_TurnToGoal(em, 0x100, 0);
        }
        if (inFlight && em->posY <= em->floorY) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            Em_SetMotion(em, VPREY_MOT_LEAP_LAND, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * VPREY_ATK_LEAP_HIGH_B: as EmVelociprey_AttackLeapHighA with
 * VelocipreyAttack row 9 for every kind.
 */
void EmVelociprey_AttackLeapHighB(Em *em, EmWork *work)
{
    EmVelocipreyWork *w = (EmVelocipreyWork *)work;
    u8 inFlight;

    inFlight = 0;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_LEAP, 0, 0);
        Player_ClearVelocity(em);
        em->velY = 150.0f;
        em->accelY = -20.0f;
        em->velZ = 25.0f;
        em->accelZ = -3.0f;
        work->pitchStep = 0;
        work->turnSpeed = 0;
        work->rollSpeed = 0;
        work->unk048 = 0;
        work->unk04C = 0;
        w->unk050 = 0;
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            VelocipreyAttack_Create(em, 9);
        }
        if (EmMotion_ReachedFrame(em, 0, 22.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            work->accelAngX = 0;
            work->accelAngY = em->rotY;
            work->accelAngZ = 0;
            inFlight = 1;
            if (em->wallHitDirs & 0xF000000F) {
                w->wallVelZ = em->velZ;
                em->velZ = 0.0f;
            }
            Em_MoveRotatedAccel(em, &work->accelAngX);
            if (w->wallVelZ != 0.0f) {
                em->velZ = w->wallVelZ;
                w->wallVelZ = 0.0f;
            }
            if (em->velZ < 10.0f) {
                em->velZ = 10.0f;
                em->accelZ = 0.0f;
            }
        }
        if (em->hasGoal != 0 && em->stance == EM_STANCE_AIR) {
            Em_TurnToGoal(em, 0x100, 0);
        }
        if (inFlight && em->posY <= em->floorY) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            Em_SetMotion(em, VPREY_MOT_LEAP_LAND, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * VPREY_ATK_STRIKE_C: as EmVelociprey_AttackStrikeA with VelocipreyAttack
 * row 0x11.
 */
void EmVelociprey_AttackStrikeC(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_STRIKE, 0, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            VelocipreyAttack_Create(em, 0x11);
        }
        if ((em->hasGoal != 0) && (EmMotion_ReachedFrame(em, 0, 68.0f) == 0)) {
            Em_TurnToGoal(em, 0x80, 0);
        }
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * VPREY_ATK_SHOT_B and VPREY_ATK_SHOT_B_FAST: as EmVelociprey_AttackShotA
 * with the EmRangedAttack type 0x17.
 */
void EmVelociprey_AttackShotB(Em *em, EmWork *work, s32 fastShot)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_SHOOT, 0, 0);
        break;
    case 1:
        switch (fastShot) {
        case 0:
        default:
            if (EmMotion_PassedFrameScaled(em, 80.0f, 0U) != 0) {
                EmRangedAttack_CreateAtBone(em, 0xF, 0x17, 0, 0xF8E5, 0);
            }
            break;
        case 1:
            if (EmMotion_PassedFrameScaled(em, 80.0f, 0U) != 0) {
                EmRangedAttack_CreateAtBone(em, 0xF, 0x17, 1, 0xF8E5, 0);
            }
            break;
        }
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/* Runs the state of group 3, EM_GROUP_ATTACK. */
void EmVelociprey_RunAttackState(Em *em, EmWork *work)
{
    switch (em->state) {
    case VPREY_ATK_STRIKE_A:
        EmVelociprey_AttackStrikeA(em);
        break;
    case VPREY_ATK_NONE:
        EmVelociprey_AttackNone(em);
        break;
    case VPREY_ATK_LEAP_MEDIUM_A:
        EmVelociprey_AttackLeapMediumA(em, work);
        break;
    case VPREY_ATK_QUICK_STRIKE_A:
        EmVelociprey_AttackQuickStrikeA(em, work);
        break;
    case VPREY_ATK_SHOT_A:
        EmVelociprey_AttackShotA(em, work, 0);
        break;
    case VPREY_ATK_LEAP_LONG_A:
        EmVelociprey_AttackLeapLongA(em, work);
        break;
    case VPREY_ATK_LEAP_HIGH_A:
        EmVelociprey_AttackLeapHighA(em, work);
        break;
    case VPREY_ATK_STRIKE_B:
        EmVelociprey_AttackStrikeB(em);
        break;
    case VPREY_ATK_LEAP_MEDIUM_B:
        EmVelociprey_AttackLeapMediumB(em, work);
        break;
    case VPREY_ATK_QUICK_STRIKE_B:
        EmVelociprey_AttackQuickStrikeB(em);
        break;
    case VPREY_ATK_LEAP_LONG_B:
        EmVelociprey_AttackLeapLongB(em, work);
        break;
    case VPREY_ATK_LEAP_HIGH_B:
        EmVelociprey_AttackLeapHighB(em, work);
        break;
    case VPREY_ATK_STRIKE_C:
        EmVelociprey_AttackStrikeC(em);
        break;
    case VPREY_ATK_SHOT_A_FAST:
        EmVelociprey_AttackShotA(em, work, 1);
        break;
    case VPREY_ATK_SHOT_B:
        EmVelociprey_AttackShotB(em, work, 0);
        break;
    case VPREY_ATK_SHOT_B_FAST:
        EmVelociprey_AttackShotB(em, work, 1);
        break;
    }
}

/*
 * VPREY_REACT_FLINCH, a hit on the ground: the flinch motion, the script
 * restarted, then EmVelociprey_EndState.
 */
void EmVelociprey_Flinch(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_FLINCH, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * VPREY_REACT_KNOCKED_BACK, a hit from the front (or a status in the air):
 * thrown backward from frame 10, lands, then VPREY_IDLE_GET_UP.
 */
void EmVelociprey_KnockedBack(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_KNOCKED_BACK, 0, 0);
        work->accelAngX = 0;
        work->accelAngY = em->rotY;
        work->accelAngZ = 0;
        Player_ClearVelocity(em);
        em->velY = 23.35f;
        em->velZ = -27.0f;
        em->accelY = -2.0f;
        Em_RestartScript(em);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 10.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            Em_MoveRotatedAccel(em, &work->accelAngX);
            if (em->posY <= em->floorY) {
                em->step++;
                em->stance = EM_STANCE_GROUND;
                Em_SetMotion(em, VPREY_MOT_KNOCKED_BACK_LAND, 0, 0);
                break;
            }
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmVelociprey_ChangeState(em, EM_GROUP_IDLE, VPREY_IDLE_GET_UP, EM_SYNC_LOCAL);
        }
        break;
    }
}

/*
 * VPREY_REACT_KNOCKED_FORWARD, a hit from behind: thrown the other way (the
 * move's heading is rotY plus a half turn, written as 0x7FFF + 1 for the
 * match), turning while it lands, then VPREY_IDLE_GET_UP.
 */
void EmVelociprey_KnockedForward(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_KNOCKED_FORWARD, 0, 0);
        work->accelAngX = 0;
        work->accelAngY = (em->rotY + 0x7FFF + 1) & 0xFFFF;
        work->accelAngZ = 0;
        Player_ClearVelocity(em);
        em->velY = 23.35f;
        em->velZ = -27.0f;
        em->accelY = -2.0f;
        Em_RestartScript(em);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 10.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            Em_MoveRotatedAccel(em, &work->accelAngX);
            if (em->posY <= em->floorY) {
                em->step++;
                em->stance = EM_STANCE_GROUND;
                Em_SetMotion(em, VPREY_MOT_KNOCKED_FORWARD_LAND, 0, 0);
                break;
            }
        }
        break;
    case 2:
        if (EmMotion_ReachedFrame(em, 0, 26.0f) == 0) {
            em->rotY -= 0x555;
        }
        if (em->motion[0].playing == 0) {
            EmVelociprey_ChangeState(em, EM_GROUP_IDLE, VPREY_IDLE_GET_UP, EM_SYNC_LOCAL);
        }
        break;
    }
}

/*
 * VPREY_REACT_KNOCKED_SIDE_A: thrown sideways, turning a quarter while it
 * lands, then VPREY_IDLE_GET_UP.
 */
void EmVelociprey_KnockedSideA(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_KNOCKED_SIDE_A, 0, 0);
        work->accelAngX = 0;
        work->accelAngY = (em->rotY - 0x4000) & 0xFFFF;
        work->accelAngZ = 0;
        Player_ClearVelocity(em);
        em->velY = 23.35f;
        em->velZ = -27.0f;
        em->accelY = -2.0f;
        Em_RestartScript(em);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 10.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            Em_MoveRotatedAccel(em, &work->accelAngX);
            if (em->posY <= em->floorY) {
                em->step++;
                em->stance = EM_STANCE_GROUND;
                Em_SetMotion(em, VPREY_MOT_KNOCKED_SIDE_A_LAND, 0, 0);
                break;
            }
        }
        break;
    case 2:
        if (EmMotion_ReachedFrame(em, 0, 26.0f) == 0) {
            em->rotY -= 0x2AA;
        }
        if (em->motion[0].playing == 0) {
            EmVelociprey_ChangeState(em, EM_GROUP_IDLE, VPREY_IDLE_GET_UP, EM_SYNC_LOCAL);
        }
        break;
    }
}

/* VPREY_REACT_KNOCKED_SIDE_B: the mirror of EmVelociprey_KnockedSideA. */
void EmVelociprey_KnockedSideB(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_KNOCKED_SIDE_B, 0, 0);
        work->accelAngX = 0;
        work->accelAngY = (em->rotY + 0x4000) & 0xFFFF;
        work->accelAngZ = 0;
        Player_ClearVelocity(em);
        em->velY = 23.35f;
        em->velZ = -27.0f;
        em->accelY = -2.0f;
        Em_RestartScript(em);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 10.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            Em_MoveRotatedAccel(em, &work->accelAngX);
            if (em->posY <= em->floorY) {
                em->step++;
                em->stance = EM_STANCE_GROUND;
                Em_SetMotion(em, VPREY_MOT_KNOCKED_SIDE_B_LAND, 0, 0);
                break;
            }
        }
        break;
    case 2:
        if (EmMotion_ReachedFrame(em, 0, 26.0f) == 0) {
            em->rotY += 0x2AA;
        }
        if (em->motion[0].playing == 0) {
            EmVelociprey_ChangeState(em, EM_GROUP_IDLE, VPREY_IDLE_GET_UP, EM_SYNC_LOCAL);
        }
        break;
    }
}

/*
 * VPREY_REACT_DAZE: the dazed motion once, the script restarted, then
 * EmVelociprey_EndState. EmVelociprey_Update never picks it.
 */
void EmVelociprey_Daze(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_DAZED, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * VPREY_REACT_STATUS4: Em_BeginStatus4 and the status motion while Em.timer
 * counts down, with an effect on bone 2; then VPREY_IDLE_STATUS4_END.
 */
void EmVelociprey_Status4(Em *em, EmWork *work)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_STATUS);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_STATUS4, 0, 0);
        Em_BeginStatus4(em);
        Em_RestartScript(em);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmVelociprey_ChangeState(em, EM_GROUP_IDLE, VPREY_IDLE_STATUS4_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
    Em_SpawnStatus4Effect(em, 2);
}

/*
 * VPREY_REACT_STUN (status 5): the dazed motion and the stun's stars
 * (StageObj_Stun, for the low half of Em.timer frames) until Em.timer runs
 * out, then VPREY_IDLE_STUN_END.
 */
void EmVelociprey_Stun(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, VPREY_MOT_DAZED, 0, 0);
        StageObj_Stun_CreateEm(em, 1, 0xE, *(s16 *)&em->timer, 50.0f, 1.0f);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmVelociprey_ChangeState(em, EM_GROUP_IDLE, VPREY_IDLE_STUN_END, EM_SYNC_LOCAL);
        }
        break;
    }
}

/* Runs the state of group 4, EM_GROUP_REACT (no state 7). */
void EmVelociprey_RunReactState(Em *em, EmWork *work)
{
    switch (em->state) {
    case VPREY_REACT_FLINCH:
        EmVelociprey_Flinch(em, work);
        break;
    case VPREY_REACT_KNOCKED_BACK:
        EmVelociprey_KnockedBack(em, work);
        break;
    case VPREY_REACT_KNOCKED_FORWARD:
        EmVelociprey_KnockedForward(em, work);
        break;
    case VPREY_REACT_KNOCKED_SIDE_A:
        EmVelociprey_KnockedSideA(em, work);
        break;
    case VPREY_REACT_KNOCKED_SIDE_B:
        EmVelociprey_KnockedSideB(em, work);
        break;
    case VPREY_REACT_DAZE:
        EmVelociprey_Daze(em, work);
        break;
    case VPREY_REACT_STATUS4:
        EmVelociprey_Status4(em, work);
        break;
    case VPREY_REACT_STUN:
        EmVelociprey_Stun(em, work);
        
    default:
        break;
    }
}

/*
 * VPREY_DEFEAT_DIE_BACK, killed by a hit from the front: hunted
 * (QuestEm_Hunted), thrown backward, lands, dies and lies as a carcass with
 * a carve point (EmMine) for 300 frames, fades out over 60 frames, then
 * VPREY_DEFEAT_REAPPEAR.
 */
void EmVelociprey_DieBack(Em *em, EmWork *work)
{
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        work->huntedB = 1;
        QuestEm_Hunted(em);
        Em_SetMotion(em, VPREY_MOT_KNOCKED_BACK, 0, 0);
        work->accelAngX = 0;
        work->accelAngY = em->rotY;
        work->accelAngZ = 0;
        Player_ClearVelocity(em);
        em->velY = 23.35f;
        em->velZ = -27.0f;
        em->accelY = -2.0f;
        em->stance = EM_STANCE_AIR;
        break;
    case 1:
        Em_MoveRotatedAccel(em, &work->accelAngX);
        if (em->posY <= em->floorY) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, VPREY_MOT_KNOCKED_BACK_LAND, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, VPREY_MOT_DIE, 0, 0);
            em->fieldCollide = 0;
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, VPREY_MOT_DEAD, 0, 0);
            em->timer = 300;
            EmMine_Create(em, 0);
        }
        break;
    case 4:
        EmMine_Update(em);
        if (--em->timer <= 0) {
            em->step++;
            EmMine_Remove(em);
        }
        break;
    case 5:
        em->alpha -= 1.0f / 60.0f;
        if (em->alpha <= 0.0f) {
            em->visible = 0;
            EmVelociprey_ChangeState(em, EM_GROUP_DEFEAT, VPREY_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * VPREY_DEFEAT_DIE_FORWARD, killed by a hit from behind: as
 * EmVelociprey_DieBack, thrown forward.
 */
void EmVelociprey_DieForward(Em *em, EmWork *work)
{
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        work->huntedB = 1;
        QuestEm_Hunted(em);
        Em_SetMotion(em, VPREY_MOT_KNOCKED_FORWARD, 0, 0);
        work->accelAngX = 0;
        work->accelAngY = (em->rotY + 0x8000) & 0xFFFF;
        work->accelAngZ = 0;
        Player_ClearVelocity(em);
        em->velY = 23.35f;
        em->velZ = -27.0f;
        em->accelY = -2.0f;
        em->stance = EM_STANCE_AIR;
        break;
    case 1:
        Em_MoveRotatedAccel(em, &work->accelAngX);
        if (em->posY <= em->floorY) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, VPREY_MOT_KNOCKED_FORWARD_LAND, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, VPREY_MOT_DIE, 0, 0);
            em->fieldCollide = 0;
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, VPREY_MOT_DEAD, 0, 0);
            em->timer = 300;
            EmMine_Create(em, 0);
        }
        break;
    case 4:
        EmMine_Update(em);
        if (--em->timer <= 0) {
            em->step++;
            EmMine_Remove(em);
        }
        break;
    case 5:
        em->alpha -= 1.0f / 60.0f;
        if (em->alpha <= 0.0f) {
            em->visible = 0;
            EmVelociprey_ChangeState(em, EM_GROUP_DEFEAT, VPREY_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * VPREY_DEFEAT_DIE_SIDE_A: as EmVelociprey_DieBack, thrown sideways (a
 * quarter turn from rotY).
 */
void EmVelociprey_DieSideA(Em *em, EmWork *work)
{
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        work->huntedB = 1;
        QuestEm_Hunted(em);
        Em_SetMotion(em, VPREY_MOT_KNOCKED_SIDE_A, 0, 0);
        work->accelAngX = 0;
        work->accelAngY = (em->rotY - 0x4000) & 0xFFFF;
        work->accelAngZ = 0;
        Player_ClearVelocity(em);
        em->velY = 23.35f;
        em->velZ = -27.0f;
        em->accelY = -2.0f;
        em->stance = EM_STANCE_AIR;
        break;
    case 1:
        Em_MoveRotatedAccel(em, &work->accelAngX);
        if (em->posY <= em->floorY) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, VPREY_MOT_KNOCKED_SIDE_A_LAND, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, VPREY_MOT_DIE, 0, 0);
            em->fieldCollide = 0;
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, VPREY_MOT_DEAD, 0, 0);
            em->timer = 300;
            EmMine_Create(em, 0);
        }
        break;
    case 4:
        EmMine_Update(em);
        if (--em->timer <= 0) {
            em->step++;
            EmMine_Remove(em);
        }
        break;
    case 5:
        em->alpha -= 1.0f / 60.0f;
        if (em->alpha <= 0.0f) {
            em->visible = 0;
            EmVelociprey_ChangeState(em, EM_GROUP_DEFEAT, VPREY_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/* VPREY_DEFEAT_DIE_SIDE_B: the mirror of EmVelociprey_DieSideA. */
void EmVelociprey_DieSideB(Em *em, EmWork *work)
{
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        work->huntedB = 1;
        QuestEm_Hunted(em);
        Em_SetMotion(em, VPREY_MOT_KNOCKED_SIDE_B, 0, 0);
        work->accelAngX = 0;
        work->accelAngY = (em->rotY + 0x4000) & 0xFFFF;
        work->accelAngZ = 0;
        Player_ClearVelocity(em);
        em->velY = 23.35f;
        em->velZ = -27.0f;
        em->accelY = -2.0f;
        em->stance = EM_STANCE_AIR;
        break;
    case 1:
        Em_MoveRotatedAccel(em, &work->accelAngX);
        if (em->posY <= em->floorY) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, VPREY_MOT_KNOCKED_SIDE_B_LAND, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, VPREY_MOT_DIE, 0, 0);
            em->fieldCollide = 0;
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, VPREY_MOT_DEAD, 0, 0);
            em->timer = 300;
            EmMine_Create(em, 0);
        }
        break;
    case 4:
        EmMine_Update(em);
        if (--em->timer <= 0) {
            em->step++;
            EmMine_Remove(em);
        }
        break;
    case 5:
        em->alpha -= 1.0f / 60.0f;
        if (em->alpha <= 0.0f) {
            em->visible = 0;
            EmVelociprey_ChangeState(em, EM_GROUP_DEFEAT, VPREY_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * VPREY_DEFEAT_REAPPEAR: when the quest lets the monster appear again
 * (QuestEm_CanAppear), puts it at one of its area's appear points (by
 * Em.stateRand), facing the point's target or its heading, and hops it in
 * (group 7: VPREY_EVENT2_HOP_IN, _LOW or _HIGH by area). In an area without
 * appear points it waits in VPREY_IDLE_SPAWN, or VPREY_IDLE_PACK_WAIT when
 * linked. Refused, it advances its task phase.
 */
void EmVelociprey_Reappear(Em *em)
{
    f32 facePoint[4];
    s16 pointCount;
    u8 hopState;
    EmVelocipreyAppearAt *points;
    u16 pointIdx;

    switch (em->step) {
    case 0:
        if (QuestEm_CanAppear(em) == 1) {
            switch (Area_DayId(em->area)) {
            case AREA_FORTRESS_6:
                pointIdx = em->stateRand % 4;
                Vec_Set(&em->posX, EmVelociprey_AppearPtsFortress6[pointIdx].pos[0], EmVelociprey_AppearPtsFortress6_PosY[pointIdx].pos[0], EmVelociprey_AppearPtsFortress6_PosZ[pointIdx].pos[0]);
                Vec_Set(&em->prevPosX, EmVelociprey_AppearPtsFortress6[pointIdx].pos[0], EmVelociprey_AppearPtsFortress6_PosY[pointIdx].pos[0], EmVelociprey_AppearPtsFortress6_PosZ[pointIdx].pos[0]);
                Vec_Set(facePoint, EmVelociprey_AppearPtsFortress6_TargetX[pointIdx].pos[0], EmVelociprey_AppearPtsFortress6_TargetY[pointIdx].pos[0], EmVelociprey_AppearPtsFortress6_TargetZ[pointIdx].pos[0]);
                em->rotY = Math_AngleXZ(&em->posX, facePoint) & 0xFFFF;
                EmVelociprey_ChangeState(em, EM_GROUP_EVENT2, VPREY_EVENT2_HOP_IN_LOW, EM_SYNC_LOCAL);
                break;
            case AREA_FOREST_HILLS_2:
                pointIdx = em->stateRand % 5;
                Vec_Set(&em->posX, EmVelociprey_AppearPtsForestHills2[pointIdx].pos[0], EmVelociprey_AppearPtsForestHills2_PosY[pointIdx].pos[0], EmVelociprey_AppearPtsForestHills2_PosZ[pointIdx].pos[0]);
                Vec_Set(&em->prevPosX, EmVelociprey_AppearPtsForestHills2[pointIdx].pos[0], EmVelociprey_AppearPtsForestHills2_PosY[pointIdx].pos[0], EmVelociprey_AppearPtsForestHills2_PosZ[pointIdx].pos[0]);
                Vec_Set(facePoint, EmVelociprey_AppearPtsForestHills2_TargetX[pointIdx].pos[0], EmVelociprey_AppearPtsForestHills2_TargetY[pointIdx].pos[0], EmVelociprey_AppearPtsForestHills2_TargetZ[pointIdx].pos[0]);
                em->rotY = Math_AngleXZ(&em->posX, facePoint) & 0xFFFF;
                EmVelociprey_ChangeState(em, EM_GROUP_EVENT2, VPREY_EVENT2_HOP_IN_HIGH, EM_SYNC_LOCAL);
                break;
            case AREA_FOREST_HILLS_1:
                pointIdx = em->stateRand % 4;
                Vec_Set(&em->posX, EmVelociprey_AppearPtsForestHills1[pointIdx].pos[0], EmVelociprey_AppearPtsForestHills1_PosY[pointIdx].pos[0], EmVelociprey_AppearPtsForestHills1_PosZ[pointIdx].pos[0]);
                Vec_Set(&em->prevPosX, EmVelociprey_AppearPtsForestHills1[pointIdx].pos[0], EmVelociprey_AppearPtsForestHills1_PosY[pointIdx].pos[0], EmVelociprey_AppearPtsForestHills1_PosZ[pointIdx].pos[0]);
                Vec_Set(facePoint, EmVelociprey_AppearPtsForestHills1_TargetX[pointIdx].pos[0], EmVelociprey_AppearPtsForestHills1_TargetY[pointIdx].pos[0], EmVelociprey_AppearPtsForestHills1_TargetZ[pointIdx].pos[0]);
                em->rotY = Math_AngleXZ(&em->posX, facePoint) & 0xFFFF;
                EmVelociprey_ChangeState(em, EM_GROUP_EVENT2, VPREY_EVENT2_HOP_IN, EM_SYNC_LOCAL);
                break;
            case AREA_SNOWY_MOUNTAINS_3:
                Vec_Set(&em->posX, EmVelociprey_AppearPtSnowyMountains3_PosX, EmVelociprey_AppearPtSnowyMountains3_PosY, EmVelociprey_AppearPtSnowyMountains3_PosZ);
                Vec_Set(&em->prevPosX, EmVelociprey_AppearPtSnowyMountains3_PosX, EmVelociprey_AppearPtSnowyMountains3_PosY, EmVelociprey_AppearPtSnowyMountains3_PosZ);
                Vec_Set(facePoint, EmVelociprey_AppearPtSnowyMountains3_TargetX, EmVelociprey_AppearPtSnowyMountains3_TargetY, EmVelociprey_AppearPtSnowyMountains3_TargetZ);
                em->rotY = Math_AngleXZ(&em->posX, facePoint) & 0xFFFF;
                EmVelociprey_ChangeState(em, EM_GROUP_EVENT2, VPREY_EVENT2_HOP_IN_LOW, EM_SYNC_LOCAL);
                break;
            case AREA_FOREST_HILLS_10:
            case AREA_FOREST_HILLS_9:
            case AREA_JUNGLE_2:
            case AREA_JUNGLE_3:
            case AREA_JUNGLE_4:
            case AREA_JUNGLE_6:
            case AREA_JUNGLE_9:
            case AREA_TOWER_6:
            case AREA_TOWER_7:
            case AREA_DESERT_2:
            case AREA_DESERT_4:
            case AREA_DESERT_7:
            case AREA_SWAMP_4:
            case AREA_SWAMP_5:
            case AREA_SWAMP_8:
            case AREA_SWAMP_9:
            case AREA_VOLCANO_3:
            case AREA_VOLCANO_6:
            case AREA_BATTLE_TOWN_1:
            case AREA_BATTLE_TOWN_2:
            case AREA_BATTLE_TOWN_3:
                hopState = 0;
                switch (Area_DayId(em->area)) {
                case AREA_FOREST_HILLS_10:
                    pointCount = 3;
                    points = EmVelociprey_AppearPtsForestHills10;
                    break;
                case AREA_FOREST_HILLS_9:
                    pointCount = 3;
                    points = EmVelociprey_AppearPtsForestHills9;
                    break;
                case AREA_JUNGLE_2:
                    pointCount = 4;
                    points = EmVelociprey_AppearPtsJungle2;
                    break;
                case AREA_JUNGLE_3:
                    pointCount = 3;
                    points = EmVelociprey_AppearPtsJungle3;
                    break;
                case AREA_JUNGLE_4:
                    pointCount = 4;
                    points = EmVelociprey_AppearPtsJungle4;
                    break;
                case AREA_JUNGLE_6:
                    pointCount = 3;
                    points = EmVelociprey_AppearPtsJungle6;
                    break;
                case AREA_JUNGLE_9:
                    pointCount = 2;
                    points = EmVelociprey_AppearPtsJungle9;
                    break;
                case AREA_TOWER_6:
                    pointCount = 2;
                    points = EmVelociprey_AppearPtsTower6;
                    break;
                case AREA_TOWER_7:
                    pointCount = 2;
                    points = EmVelociprey_AppearPtsTower7;
                    break;
                case AREA_DESERT_2:
                    hopState = 2;
                    pointCount = 3;
                    points = EmVelociprey_AppearPtsDesert2;
                    break;
                case AREA_DESERT_4:
                    if (em->area == AREA_DESERT_NIGHT_4) {
                        pointCount = 1;
                    } else {
                        pointCount = 2;
                    }
                    points = EmVelociprey_AppearPtsDesert4;
                    break;
                case AREA_DESERT_7:
                    pointCount = 1;
                    points = EmVelociprey_AppearPtsDesert7;
                    break;
                case AREA_SWAMP_4:
                case AREA_SWAMP_5:
                    pointCount = 2;
                    points = EmVelociprey_AppearPtsSwamp5;
                    break;
                case AREA_SWAMP_8:
                    pointCount = 1;
                    points = EmVelociprey_AppearPtsSwamp8;
                    break;
                case AREA_SWAMP_9:
                    pointCount = 3;
                    points = EmVelociprey_AppearPtsSwamp9;
                    break;
                case AREA_VOLCANO_3:
                    pointCount = 2;
                    points = EmVelociprey_AppearPtsVolcano3;
                    break;
                case AREA_VOLCANO_6:
                    pointCount = 1;
                    points = EmVelociprey_AppearPtsVolcano6;
                    break;
                case AREA_BATTLE_TOWN_1:
                    hopState = 2;
                    pointCount = 4;
                    points = EmVelociprey_AppearPtsBattleTown1;
                    break;
                case AREA_BATTLE_TOWN_2:
                    pointCount = 2;
                    points = EmVelociprey_AppearPtsBattleTown2;
                    break;
                case AREA_BATTLE_TOWN_3:
                    pointCount = 2;
                    points = EmVelociprey_AppearPtsBattleTown3;
                    break;
                }
                pointIdx = em->stateRand % pointCount;
                Vec_Copy(&em->posX, points[pointIdx].pos);
                Vec_Copy(&em->prevPosX, &em->posX);
                em->rotY = points[pointIdx].angle;
                EmVelociprey_ChangeState(em, EM_GROUP_EVENT2, hopState, EM_SYNC_LOCAL);
                break;
            default:
                if (em->link == NULL) {
                    EmVelociprey_ChangeState(em, EM_GROUP_IDLE, VPREY_IDLE_SPAWN, EM_SYNC_LOCAL);
                } else {
                    em->senseDelay = 5;
                    EmVelociprey_ChangeState(em, EM_GROUP_IDLE, VPREY_IDLE_PACK_WAIT, EM_SYNC_LOCAL);
                }
                break;
            }
            break;
        }
        em->taskPhase++;
        break;
    }
}

/*
 * VPREY_DEFEAT_FADE_RESPAWN: unhittable, fades out over 100 frames, then
 * EmVelociprey_Respawn. Nothing in the file enters it.
 */
void EmVelociprey_FadeAndRespawn(Em *em)
{
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 0x64;
        break;
    case 1:
        em->timer -= 1;
        em->alpha = (f32) em->timer / 100.0f;
        if (em->timer <= 0) {
            EmVelociprey_Respawn(em);
        }
        break;
    }
}

/*
 * VPREY_DEFEAT_DIE_IN_AIR_BACK, killed in the air: hunted, flung backward
 * with its shadow hidden, fading out over 40 frames (no carcass), then
 * VPREY_DEFEAT_REAPPEAR.
 */
void EmVelociprey_DieInAirBack(Em *em, EmWork *work)
{
    em->hideShadowHold = 2;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        work->huntedB = 1;
        QuestEm_Hunted(em);
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, VPREY_MOT_DIE_IN_AIR, 0, 0);
        em->hideShadow = 1;
        work->accelAngX = 0;
        work->accelAngY = em->rotY;
        work->accelAngZ = 0;
        Player_ClearVelocity(em);
        em->velY = 23.35f;
        em->velZ = -27.0f;
        em->accelY = -2.0f;
        break;
    case 1:
        Em_MoveRotatedAccel(em, &work->accelAngX);
        em->alpha -= 0.025f;
        if (em->alpha <= 0.0f) {
            em->visible = 0;
            em->step++;
            EmVelociprey_ChangeState(em, EM_GROUP_DEFEAT, VPREY_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * VPREY_DEFEAT_DIE_IN_AIR_FORWARD: as EmVelociprey_DieInAirBack, flung
 * forward.
 */
void EmVelociprey_DieInAirForward(Em *em, EmWork *work)
{
    em->hideShadowHold = 2;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        work->huntedB = 1;
        QuestEm_Hunted(em);
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, VPREY_MOT_DIE_IN_AIR, 0, 0);
        em->hideShadow = 1;
        work->accelAngX = 0;
        work->accelAngY = (em->rotY + 0x8000) & 0xFFFF;
        work->accelAngZ = 0;
        Player_ClearVelocity(em);
        em->velY = 23.35f;
        em->velZ = -27.0f;
        em->accelY = -2.0f;
        break;
    case 1:
        Em_MoveRotatedAccel(em, &work->accelAngX);
        em->alpha -= 0.025f;
        if (em->alpha <= 0.0f) {
            em->visible = 0;
            em->step++;
            EmVelociprey_ChangeState(em, EM_GROUP_DEFEAT, VPREY_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * VPREY_DEFEAT_DIE_IN_AIR_SIDE_A: as EmVelociprey_DieInAirBack, flung
 * sideways.
 */
void EmVelociprey_DieInAirSideA(Em *em, EmWork *work)
{
    em->hideShadowHold = 2;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        work->huntedB = 1;
        QuestEm_Hunted(em);
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, VPREY_MOT_DIE_IN_AIR, 0, 0);
        em->hideShadow = 1;
        work->accelAngX = 0;
        work->accelAngY = (em->rotY - 0x4000) & 0xFFFF;
        work->accelAngZ = 0;
        Player_ClearVelocity(em);
        em->velY = 23.35f;
        em->velZ = -27.0f;
        em->accelY = -2.0f;
        break;
    case 1:
        Em_MoveRotatedAccel(em, &work->accelAngX);
        em->alpha -= 0.025f;
        if (em->alpha <= 0.0f) {
            em->visible = 0;
            em->step++;
            EmVelociprey_ChangeState(em, EM_GROUP_DEFEAT, VPREY_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/* VPREY_DEFEAT_DIE_IN_AIR_SIDE_B: the mirror of EmVelociprey_DieInAirSideA. */
void EmVelociprey_DieInAirSideB(Em *em, EmWork *work)
{
    em->hideShadowHold = 2;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        work->huntedB = 1;
        QuestEm_Hunted(em);
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, VPREY_MOT_DIE_IN_AIR, 0, 0);
        em->hideShadow = 1;
        work->accelAngX = 0;
        work->accelAngY = (em->rotY + 0x4000) & 0xFFFF;
        work->accelAngZ = 0;
        Player_ClearVelocity(em);
        em->velY = 23.35f;
        em->velZ = -27.0f;
        em->accelY = -2.0f;
        break;
    case 1:
        Em_MoveRotatedAccel(em, &work->accelAngX);
        em->alpha -= 0.025f;
        if (em->alpha <= 0.0f) {
            em->visible = 0;
            em->step++;
            EmVelociprey_ChangeState(em, EM_GROUP_DEFEAT, VPREY_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * Runs the state of group 5, EM_GROUP_DEFEAT, with the monster unhittable
 * and unpushable throughout.
 */
void EmVelociprey_RunDefeatState(Em *em, EmWork *work)
{
    em->noHitTimer = 0xA;
    em->noPushTimer = 0xA;
    switch (em->state) {
    case VPREY_DEFEAT_DIE_BACK:
        EmVelociprey_DieBack(em, work);
        break;
    case VPREY_DEFEAT_DIE_FORWARD:
        EmVelociprey_DieForward(em, work);
        break;
    case VPREY_DEFEAT_DIE_SIDE_A:
        EmVelociprey_DieSideA(em, work);
        break;
    case VPREY_DEFEAT_DIE_SIDE_B:
        EmVelociprey_DieSideB(em, work);
        break;
    case VPREY_DEFEAT_REAPPEAR:
        EmVelociprey_Reappear(em);
        break;
    case VPREY_DEFEAT_FADE_RESPAWN:
        EmVelociprey_FadeAndRespawn(em);
        break;
    case VPREY_DEFEAT_DIE_IN_AIR_BACK:
        EmVelociprey_DieInAirBack(em, work);
        break;
    case VPREY_DEFEAT_DIE_IN_AIR_FORWARD:
        EmVelociprey_DieInAirForward(em, work);
        break;
    case VPREY_DEFEAT_DIE_IN_AIR_SIDE_A:
        EmVelociprey_DieInAirSideA(em, work);
        break;
    case VPREY_DEFEAT_DIE_IN_AIR_SIDE_B:
        EmVelociprey_DieInAirSideB(em, work);
        
    default:
        break;
    }
}

/*
 * VPREY_EVENT_LOOK_AROUND, a scripted scene at a fixed spot: holds its
 * event motion for 450 frames, an idle action, two left turns toward the
 * heading in work->fxMotion (0x666 a frame between frames 24 and 44), the
 * call and the alert motions, then EmVelociprey_EndState. Nothing in the
 * file writes that heading.
 */
void EmVelociprey_EventLookAround(Em *em, EmWork *work)
{
    s32 headingDiff;
    s32 headingDiff2;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        em->posX = 8460.0f;
        em->posY = 0.0f;
        em->posZ = 9367.0f;
        em->goal[0] = 8212.0f;
        em->goal[1] = 0.0f;
        em->goal[2] = 9356.0f;
        em->rotX = 0;
        em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        em->rotZ = 0;
        Em_SetMotion(em, VPREY_MOT_EVENT_IDLE, 0, 0);
        em->timer = 450;
        break;
    case 1:
        if (--em->timer <= 0 && (em->motion[0].playing == 0)) {
            em->step++;
            Em_SetMotion(em, VPREY_MOT_IDLE, 0, 0);
            break;
        }
    default:
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, VPREY_MOT_IDLE_ACTION_A, 0, 0);
        }
        break;
    case 4:
        if (em->motion[0].blend == 0) {
            headingDiff = ((u16) work->fxMotion - (u16) em->rotY) & 0xFFFF;
            if ((EmMotion_ReachedFrame(em, 0, 24.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 44.0f) == 0)) {
                if ((((headingDiff & 0xFFFF) + 0x666) & 0xFFFF) < 0xCCC) {
                    em->rotY = (u16) work->fxMotion;
                } else {
                    em->rotY = (em->rotY - 0x666) & 0xFFFF;
                }
            }
        case 3:
            if (em->motion[0].playing == 0) {
                em->step++;
                Em_SetMotion(em, VPREY_MOT_TURN_LEFT, 0, 0);
                em->goal[0] = 8836.0f;
                em->goal[1] = 0.0f;
                em->goal[2] = 9382.0f;
                break;
            }
        }
        break;
    case 5:
        if (em->motion[0].blend == 0) {
            headingDiff2 = ((u16) work->fxMotion - (u16) em->rotY) & 0xFFFF;
            if ((EmMotion_ReachedFrame(em, 0, 24.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 44.0f) == 0)) {
                if ((((headingDiff2 & 0xFFFF) + 0x666) & 0xFFFF) < 0xCCC) {
                    em->rotY = (u16) work->fxMotion;
                } else {
                    em->rotY = (em->rotY - 0x666) & 0xFFFF;
                }
            }
            if (em->motion[0].playing == 0) {
                em->step++;
                Em_SetMotion(em, VPREY_MOT_CALL, 0, 0);
                break;
            }
        }
        break;
    case 6:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, VPREY_MOT_ALERT, 0, 0);
        }
        break;
    case 7:
        em->step++;
        EmVelociprey_EndState(em, 0, 0);
        break;
    }
}

/*
 * VPREY_EVENT_WALK_TO_GOAL_A, a scripted scene: from a fixed start, without
 * field collision, idles 750 frames, then walks to a fixed goal until
 * within 100 of it or for 300 frames, idles, then EmVelociprey_EndState.
 */
void EmVelociprey_EventWalkToGoalA(Em *em)
{
    f32 goalDist;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        em->posX = 8200.0f;
        em->posY = 0.0f;
        em->posZ = 7300.0f;
        em->goal[0] = 7730.0f;
        em->goal[1] = 0.0f;
        em->goal[2] = 9300.0f;
        em->rotX = 0;
        em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        em->rotZ = 0;
        Em_SetMotion(em, VPREY_MOT_IDLE, 0, 0);
        em->timer = 750;
        em->fieldCollide = 0;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, VPREY_MOT_WALK, 0, 0);
            em->timer = 300;
        }
        break;
    case 2:
        goalDist = Vec_DistanceXZ(&em->posX, em->goal);
        em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        if (--em->timer <= 0 || goalDist <= 100.0f) {
            em->step++;
            Em_SetMotion(em, VPREY_MOT_IDLE, 0, 0);
        }
        break;
    case 3:
        em->step++;
        EmVelociprey_EndState(em, 0, 0);
        em->fieldCollide = 1;
        break;
    }
}

/*
 * VPREY_EVENT_WALK_TO_GOAL_B: as EmVelociprey_EventWalkToGoalA from another
 * start, with an idle action on arrival.
 */
void EmVelociprey_EventWalkToGoalB(Em *em)
{
    f32 goalDist;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        em->posX = 8340.0f;
        em->posY = 300.0f;
        em->posZ = 12470.0f;
        em->goal[0] = 8000.0f;
        em->goal[1] = 300.0f;
        em->goal[2] = 10850.0f;
        em->rotX = 0;
        em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        em->rotZ = 0;
        Em_SetMotion(em, VPREY_MOT_IDLE, 0, 0);
        em->timer = 750;
        em->fieldCollide = 0;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, VPREY_MOT_WALK, 0, 0);
            em->timer = 300;
        }
        break;
    case 2:
        goalDist = Vec_DistanceXZ(&em->posX, em->goal);
        em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        if (--em->timer <= 0 || goalDist <= 100.0f) {
            em->step++;
            Em_SetMotion(em, VPREY_MOT_IDLE_ACTION_A, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, VPREY_MOT_IDLE, 0, 0);
            em->timer = 300;
        }
        break;
    case 4:
        em->step++;
        em->fieldCollide = 1;
        EmVelociprey_EndState(em, 0, 0);
        break;
    }
}

/*
 * VPREY_EVENT_BY_VARIANT: one of the three scenes above, by Em.variant (0,
 * 1, 2).
 */
void EmVelociprey_EventByVariant(Em *em)
{
    switch (em->variant) {
    case 0:
        EmVelociprey_ChangeState(em, EM_GROUP_EVENT, VPREY_EVENT_LOOK_AROUND, EM_SYNC_LOCAL);
        break;
    case 1:
        EmVelociprey_ChangeState(em, EM_GROUP_EVENT, VPREY_EVENT_WALK_TO_GOAL_A, EM_SYNC_LOCAL);
        break;
    case 2:
        EmVelociprey_ChangeState(em, EM_GROUP_EVENT, VPREY_EVENT_WALK_TO_GOAL_B, EM_SYNC_LOCAL);
        break;
    }
}

/*
 * VPREY_EVENT_HIDE_A: hidden for one step (the script not asked), then
 * shown and EmVelociprey_EndState.
 */
void EmVelociprey_EventHideA(Em *em)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, VPREY_MOT_IDLE, 0, 0);
        em->visible = 0;
        em->scriptPending = 0;
        break;
    case 1:
        em->step++;
        em->visible = 1;
        EmVelociprey_EndState(em, 0, 0);
        break;
    }
}

/* VPREY_EVENT_HIDE_B: the same as EmVelociprey_EventHideA. */
void EmVelociprey_EventHideB(Em *em)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, VPREY_MOT_IDLE, 0, 0);
        em->visible = 0;
        em->scriptPending = 0;
        break;
    case 1:
        em->step++;
        em->visible = 1;
        EmVelociprey_EndState(em, 0, 0);
        break;
    }
}

/* VPREY_EVENT_HIDE_C: the same as EmVelociprey_EventHideA. */
void EmVelociprey_EventHideC(Em *em)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, VPREY_MOT_IDLE, 0, 0);
        em->visible = 0;
        em->scriptPending = 0;
        break;
    case 1:
        em->step++;
        em->visible = 1;
        EmVelociprey_EndState(em, 0, 0);
        break;
    }
}

/* Runs the state of group 6, EM_GROUP_EVENT. */
void EmVelociprey_RunEventState(Em *em, EmWork *work)
{
    switch (em->state) {
    case VPREY_EVENT_LOOK_AROUND:
        EmVelociprey_EventLookAround(em, work);
        break;
    case VPREY_EVENT_WALK_TO_GOAL_A:
        EmVelociprey_EventWalkToGoalA(em);
        break;
    case VPREY_EVENT_WALK_TO_GOAL_B:
        EmVelociprey_EventWalkToGoalB(em);
        break;
    case VPREY_EVENT_BY_VARIANT:
        EmVelociprey_EventByVariant(em);
        break;
    case VPREY_EVENT_HIDE_A:
        EmVelociprey_EventHideA(em);
        break;
    case VPREY_EVENT_HIDE_B:
        EmVelociprey_EventHideB(em);
        break;
    case VPREY_EVENT_HIDE_C:
        EmVelociprey_EventHideC(em);
        
    default:
        break;
    }
}

/*
 * VPREY_EVENT2_HOP_IN: hops into the area after a reappearance (up 70,
 * forward 50, airborne from frame 10), unaware until it lands, then
 * EmVelociprey_EndState.
 */
void EmVelociprey_HopIn(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, VPREY_MOT_HOP, 0, 0);
        Player_ClearVelocity(em);
        em->velY = 70.0f;
        em->velZ = 50.0f;
        em->accelY = -4.0f;
        em->accelZ = -1.0f;
        work->accelAngX = 0;
        work->accelAngZ = 0;
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 10.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            work->accelAngY = em->rotY;
            Em_MoveRotatedAccel(em, &work->accelAngX);
        }
        if (em->velY < -50.0f) {
            em->velY = -50.0f;
            em->accelY = 0.0f;
        }
        if (em->velZ < 20.0f) {
            em->velZ = 20.0f;
            em->accelZ = 0.0f;
        }
        if ((em->velY < 0.0f) && (em->posY <= em->floorY)) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            Em_SetMotion(em, VPREY_MOT_HOP_LAND, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->senseDelay = 0;
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/* VPREY_EVENT2_HOP_IN_LOW: as EmVelociprey_HopIn, lower (up 50, forward 30). */
void EmVelociprey_HopInLow(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, VPREY_MOT_HOP, 0, 0);
        Player_ClearVelocity(em);
        em->velY = 50.0f;
        em->velZ = 30.0f;
        em->accelY = -4.0f;
        em->accelZ = -1.0f;
        work->accelAngX = 0;
        work->accelAngZ = 0;
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 10.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            work->accelAngY = em->rotY;
            Em_MoveRotatedAccel(em, &work->accelAngX);
        }
        if (em->velY < -50.0f) {
            em->velY = -50.0f;
            em->accelY = 0.0f;
        }
        if (em->velZ < 10.0f) {
            em->velZ = 10.0f;
            em->accelZ = 0.0f;
        }
        if ((em->velY < 0.0f) && (em->posY <= em->floorY)) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            Em_SetMotion(em, VPREY_MOT_HOP_LAND, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->senseDelay = 0;
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * VPREY_EVENT2_HOP_IN_HIGH: as EmVelociprey_HopIn, higher (up 80, forward
 * 50).
 */
void EmVelociprey_HopInHigh(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, VPREY_MOT_HOP, 0, 0);
        Player_ClearVelocity(em);
        em->velY = 80.0f;
        em->velZ = 50.0f;
        em->accelY = -4.0f;
        em->accelZ = -0.5f;
        work->accelAngX = 0;
        work->accelAngZ = 0;
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 10.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            work->accelAngY = em->rotY;
            Em_MoveRotatedAccel(em, &work->accelAngX);
        }
        if (em->velY < -50.0f) {
            em->velY = -50.0f;
            em->accelY = 0.0f;
        }
        if (em->velZ < 30.0f) {
            em->velZ = 30.0f;
            em->accelZ = 0.0f;
        }
        if ((em->velY < 0.0f) && (em->posY <= em->floorY)) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            Em_SetMotion(em, VPREY_MOT_HOP_LAND, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->senseDelay = 0;
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/* Runs the state of group 7, EM_GROUP_EVENT2: the hops in. */
void EmVelociprey_RunEvent2State(Em *em, EmWork *work)
{
    switch (em->state) {
    case VPREY_EVENT2_HOP_IN:
        EmVelociprey_HopIn(em, work);
        break;
    case VPREY_EVENT2_HOP_IN_LOW:
        EmVelociprey_HopInLow(em, work);
        break;
    case VPREY_EVENT2_HOP_IN_HIGH:
        EmVelociprey_HopInHigh(em, work);
        break;
    }
}

/*
 * The family's update, run every frame by GmEm_Update: the motion speed;
 * notices a pack call (EM_TRIGGER_SENSED, unless it is still waiting to
 * sense or in combat); then Em_UpdateDamage's result picks the reaction: by
 * the hit's side a knock-back (or, killed, a death: in the air with the
 * part flag, the airborne one), status 2 and 4 unless knocked back or
 * getting up, a flinch on the ground. Then runs the state (twice when one
 * asked for it).
 */
void EmVelociprey_Update(Em *em)
{
    EmVelocipreyWork *w = (EmVelocipreyWork *)&em->work;
    u8 hitInfo[4];
    s32 sensed;
    u32 result;

    em->motionSpeed = 1.2f;
    if (em->senseDelay != 0) {
        em->noHitTimer = 5;
    }
    sensed = Em_SenseObjects(em, &w->callObj);
    if (em->senseDelay == 0 && em->combat == 0 && sensed != 0 && Em_ScriptFlagsFrom(em, 4) == 0) {
        em->triggers |= EM_TRIGGER_SENSED;
    }
    Em_UpdateCombatTimer(em);
    result = Em_UpdateDamage(em, hitInfo) & 0xFF;
    switch (result) {
    case EM_DMG_NONE:
    case EM_DMG_ATTACK9:
    case EM_DMG_ATTACK6:
    case EM_DMG_CAPTURED:
    case EM_DMG_FLASH:
    case EM_DMG_PART8_DOWN:
    case EM_DMG_HIT_MODE3:
    case EM_DMG_ATTACK5:
    case EM_DMG_ATTACK5_FLINCH:
    case EM_DMG_ATTACK13:
        break;
    case EM_DMG_KILLED:
    case EM_DMG_DEAD:
        if (em->stance == EM_STANCE_AIR && result == EM_DMG_KILLED && (em->partFlags[0] & 2)) {
            u16 angle = em->hitAngle;
            s32 relAngle = (angle - em->rotY) & 0xFFFF;
            if (relAngle >= 0x6000 && relAngle < 0xA000) {
                em->rotY = angle + 0x8000;
                EmVelociprey_ChangeState(em, EM_GROUP_DEFEAT, VPREY_DEFEAT_DIE_IN_AIR_BACK, EM_SYNC_OWNER);
            } else if (relAngle >= 0x2000 && relAngle < 0x6000) {
                em->rotY = angle - 0x4000;
                EmVelociprey_ChangeState(em, EM_GROUP_DEFEAT, VPREY_DEFEAT_DIE_IN_AIR_SIDE_A, EM_SYNC_OWNER);
            } else if (relAngle >= 0xA000 && relAngle < 0xE000) {
                em->rotY = angle + 0x4000;
                EmVelociprey_ChangeState(em, EM_GROUP_DEFEAT, VPREY_DEFEAT_DIE_IN_AIR_SIDE_B, EM_SYNC_OWNER);
            } else {
                em->rotY = angle & 0xFFFF;
                EmVelociprey_ChangeState(em, EM_GROUP_DEFEAT, VPREY_DEFEAT_DIE_IN_AIR_FORWARD, EM_SYNC_OWNER);
            }
        } else {
            u16 angle = em->hitAngle;
            s32 relAngle = (angle - em->rotY) & 0xFFFF;
            if (relAngle >= 0x6000 && relAngle < 0xA000) {
                em->rotY = angle + 0x8000;
                EmVelociprey_ChangeState(em, EM_GROUP_DEFEAT, VPREY_DEFEAT_DIE_BACK, EM_SYNC_OWNER);
            } else if (relAngle >= 0x2000 && relAngle < 0x6000) {
                em->rotY = angle - 0x4000;
                EmVelociprey_ChangeState(em, EM_GROUP_DEFEAT, VPREY_DEFEAT_DIE_SIDE_A, EM_SYNC_OWNER);
            } else if (relAngle >= 0xA000 && relAngle < 0xE000) {
                em->rotY = angle + 0x4000;
                EmVelociprey_ChangeState(em, EM_GROUP_DEFEAT, VPREY_DEFEAT_DIE_SIDE_B, EM_SYNC_OWNER);
            } else {
                em->rotY = angle & 0xFFFF;
                EmVelociprey_ChangeState(em, EM_GROUP_DEFEAT, VPREY_DEFEAT_DIE_FORWARD, EM_SYNC_OWNER);
            }
        }
        break;
    case EM_DMG_STATUS_IN_AIR:
        if (em->stateGroup != EM_GROUP_REACT || em->state != VPREY_REACT_KNOCKED_BACK) {
            EmVelociprey_ChangeState(em, EM_GROUP_REACT, VPREY_REACT_KNOCKED_BACK, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_STATUS4:
        if ((em->stateGroup != EM_GROUP_REACT || em->state != VPREY_REACT_KNOCKED_BACK) && (em->stateGroup != EM_GROUP_IDLE || em->state != VPREY_IDLE_GET_UP)) {
            Em_SetStatus4Timer(em);
            EmVelociprey_ChangeState(em, EM_GROUP_REACT, VPREY_REACT_STATUS4, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_STATUS2:
        if ((em->stateGroup != EM_GROUP_REACT || em->state != VPREY_REACT_KNOCKED_BACK) && (em->stateGroup != EM_GROUP_IDLE || em->state != VPREY_IDLE_GET_UP)) {
            Em_SetStatus2Timer(em);
            EmVelociprey_ChangeState(em, EM_GROUP_IDLE, VPREY_IDLE_STATUS2, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_HIT_LYING:
        EmVelociprey_ChangeState(em, EM_GROUP_IDLE, VPREY_IDLE_STATUS2_END, EM_SYNC_OWNER);
        break;
    case EM_DMG_PART_FLINCH: {
        u16 angle = em->hitAngle;
        s32 relAngle = (angle - em->rotY) & 0xFFFF;
        if (relAngle >= 0x6000 && relAngle < 0xA000) {
            em->rotY = angle + 0x8000;
            EmVelociprey_ChangeState(em, EM_GROUP_REACT, VPREY_REACT_KNOCKED_BACK, EM_SYNC_OWNER);
        } else if (relAngle >= 0x2000 && relAngle < 0x6000) {
            em->rotY = angle - 0x4000;
            EmVelociprey_ChangeState(em, EM_GROUP_REACT, VPREY_REACT_KNOCKED_SIDE_A, EM_SYNC_OWNER);
        } else if (relAngle >= 0xA000 && relAngle < 0xE000) {
            em->rotY = angle + 0x4000;
            EmVelociprey_ChangeState(em, EM_GROUP_REACT, VPREY_REACT_KNOCKED_SIDE_B, EM_SYNC_OWNER);
        } else {
            em->rotY = angle & 0xFFFF;
            EmVelociprey_ChangeState(em, EM_GROUP_REACT, VPREY_REACT_KNOCKED_FORWARD, EM_SYNC_OWNER);
        }
        break;
    }
    case EM_DMG_HIT:
        em->scriptPending = 0;
        if (em->stance == EM_STANCE_AIR) {
            if (em->hitAngle >= 0x6000 && em->hitAngle < 0xA000) {
                em->rotY = em->hitAngle + 0x8000;
                EmVelociprey_ChangeState(em, EM_GROUP_REACT, VPREY_REACT_KNOCKED_BACK, EM_SYNC_OWNER);
            } else if (em->hitAngle >= 0x2000 && em->hitAngle < 0x6000) {
                em->rotY = em->hitAngle - 0x4000;
                EmVelociprey_ChangeState(em, EM_GROUP_REACT, VPREY_REACT_KNOCKED_SIDE_A, EM_SYNC_OWNER);
            } else if (em->hitAngle >= 0xA000 && em->hitAngle < 0xE000) {
                em->rotY = em->hitAngle + 0x4000;
                EmVelociprey_ChangeState(em, EM_GROUP_REACT, VPREY_REACT_KNOCKED_SIDE_B, EM_SYNC_OWNER);
            } else {
                em->rotY = em->hitAngle & 0xFFFF;
                EmVelociprey_ChangeState(em, EM_GROUP_REACT, VPREY_REACT_KNOCKED_FORWARD, EM_SYNC_OWNER);
            }
        } else {
            EmVelociprey_ChangeState(em, EM_GROUP_REACT, VPREY_REACT_FLINCH, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_STATUS5:
        if ((em->stateGroup != EM_GROUP_REACT || em->state != VPREY_REACT_STUN) && (em->stateGroup != EM_GROUP_REACT || em->state != VPREY_REACT_KNOCKED_BACK)) {
            Em_SetStatus5Timer(em);
            EmVelociprey_ChangeState(em, EM_GROUP_REACT, VPREY_REACT_STUN, EM_SYNC_OWNER);
        }
        break;
    }
    Em_UpdatePendingScript(em);
    Em_ClearHitReactMode(em);
    EmVelociprey_RunState(em, (s32) w);
    if (em->stateRequest != 0) {
        EmVelociprey_RunState(em, (s32) w);
        em->stateRequest = 0;
    }
}

/* Runs the monster's state through its group's dispatcher. */
void EmVelociprey_RunState(Em *em, s32 work)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->stateGroup) {
    case EM_GROUP_IDLE:
        EmVelociprey_RunIdleState(em, (EmWork *) work);
        break;
    case EM_GROUP_MOVE:
        EmVelociprey_RunMoveState(em, (EmWork *) work);
        break;
    case EM_GROUP_FLY:
        EmVelociprey_RunHopState(em, (EmWork *) work);
        break;
    case EM_GROUP_ATTACK:
        EmVelociprey_RunAttackState(em, (EmWork *) work);
        break;
    case EM_GROUP_REACT:
        EmVelociprey_RunReactState(em, (EmWork *) work);
        break;
    case EM_GROUP_DEFEAT:
        EmVelociprey_RunDefeatState(em, (EmWork *) work);
        break;
    case EM_GROUP_EVENT:
        EmVelociprey_RunEventState(em, (EmWork *) work);
        break;
    case EM_GROUP_EVENT2:
        EmVelociprey_RunEvent2State(em, (EmWork *) work);
        
    default:
        break;
    }
}

/* The motion effects of the motions without any: nothing. */
void EmVelociprey_MotionEffectsDefault(Em *em)
{
}

/*
 * Every frame from the second on: keeps the layer-0 motion in
 * work->prevMotion, plays its sounds (EmVelociprey_MotionSounds), and its
 * effects at their frames: dust for the hops and moves, the dromes' pounce
 * hit (VelocipreyAttack row 2) and its dust, and a periodic effect at the
 * head while lying.
 */
void EmVelociprey_MotionEffects(Em *em, EmWork *work)
{
    f32 pos[3];

    if (em->motionId[0] != work->prevMotion) {
        work->prevMotion = em->motionId[0];
    }
    EmVelociprey_MotionSounds((s32) em, work);
    switch (work->prevMotion) {
    case EM_MOTION_ID(0, VPREY_MOT_HOP):
        if (EmMotion_PassedFrameScaled(em, 18.0f, 0U) != 0) {
            Effect0D_SpawnForEm(em, 1, 0);
        }
        break;
    case EM_MOTION_ID(0, VPREY_MOT_HOP_UP):
        if (EmMotion_PassedFrameScaled(em, 20.0f, 0U) != 0) {
            Effect0D_SpawnForEm(em, 1, 0);
        }
        break;
    case EM_MOTION_ID(0, VPREY_MOT_MOVE_ACTION_A):
        if (EmMotion_PassedFrameScaled(em, 12.0f, 0U) != 0) {
            Effect0D_SpawnForEm(em, 1, 0);
        }
        break;
    case EM_MOTION_ID(0, VPREY_MOT_MOVE_ACTION_B):
    case EM_MOTION_ID(0, VPREY_MOT_MOVE_ACTION_C):
        if (EmMotion_PassedFrameScaled(em, 14.0f, 0U) != 0) {
            Effect0D_SpawnForEm(em, 1, 0);
        }
        break;
    case EM_MOTION_ID(0, VPREY_MOT_POUNCE):
        if (EmMotion_PassedFrameScaled(em, 32.0f, 0U) != 0) {
            VelocipreyAttack_Create(em, 2);
        }
        if (EmMotion_PassedFrameScaled(em, 22.0f, 0U) != 0) {
            Effect0D_SpawnForEm(em, 0x1B, 0);
        }
        break;
    case EM_MOTION_ID(0, VPREY_MOT_LIE):
        pos[0] = 0.0f;
        pos[1] = 10.0f;
        pos[2] = 40.0f;
        Em_SpawnLyingPuffs(em, 0xF, pos, 1.0f);
        break;
    default:
        EmVelociprey_MotionEffectsDefault(em);
        break;
    case EM_MOTION_ID(0, VPREY_MOT_IDLE):
    case EM_MOTION_ID(0, VPREY_MOT_WALK):
    case EM_MOTION_ID(0, VPREY_MOT_HOP_LAND):
    case EM_MOTION_ID(0, VPREY_MOT_TURN_LEFT):
    case EM_MOTION_ID(0, VPREY_MOT_TURN_RIGHT):
    case EM_MOTION_ID(0, VPREY_MOT_ALERT):
    case EM_MOTION_ID(0, VPREY_MOT_IDLE_B):
    case EM_MOTION_ID(0, VPREY_MOT_LEAP):
    case EM_MOTION_ID(0, VPREY_MOT_LEAP_LAND):
    case EM_MOTION_ID(0, VPREY_MOT_STRIKE):
    case EM_MOTION_ID(0, VPREY_MOT_POUNCE_LAND):
    case EM_MOTION_ID(0, VPREY_MOT_QUICK_STRIKE):
    case EM_MOTION_ID(0, VPREY_MOT_SHOOT):
    case EM_MOTION_ID(0, VPREY_MOT_FLINCH):
    case EM_MOTION_ID(0, VPREY_MOT_KNOCKED_BACK):
    case EM_MOTION_ID(0, VPREY_MOT_KNOCKED_BACK_LAND):
    case EM_MOTION_ID(0, VPREY_MOT_KNOCKED_FORWARD):
    case EM_MOTION_ID(0, VPREY_MOT_KNOCKED_FORWARD_LAND):
    case EM_MOTION_ID(0, VPREY_MOT_KNOCKED_SIDE_B):
    case EM_MOTION_ID(0, VPREY_MOT_KNOCKED_SIDE_B_LAND):
    case EM_MOTION_ID(0, VPREY_MOT_KNOCKED_SIDE_A):
    case EM_MOTION_ID(0, VPREY_MOT_KNOCKED_SIDE_A_LAND):
    case EM_MOTION_ID(0, VPREY_MOT_GET_UP):
    case EM_MOTION_ID(0, VPREY_MOT_DAZED):
    case EM_MOTION_ID(0, VPREY_MOT_COLLAPSE):
    case EM_MOTION_ID(0, VPREY_MOT_DIE):
    case EM_MOTION_ID(0, VPREY_MOT_STATUS4):
    case EM_MOTION_ID(0, VPREY_MOT_IDLE_ACTION_A):
    case EM_MOTION_ID(0, VPREY_MOT_CALL):
    case EM_MOTION_ID(0, VPREY_MOT_SHAKE_OFF):
    case EM_MOTION_ID(0, VPREY_MOT_EVENT_IDLE):
        break;
    }
}

/*
 * Callback slot 3 (EmVt hook 3, every frame): the first frame only advances
 * fxStep, later frames run EmVelociprey_MotionEffects.
 */
void EmVelociprey_UpdateMotionEffects(Em *em)
{
    EmVelocipreyWork *w = (EmVelocipreyWork *)&em->work;

    switch (w->fxStep) {
    case 0:
        w->fxStep++;
        break;
    case 1:
        EmVelociprey_MotionEffects(em, (EmWork *) w);
        break;
    }
}

/*
 * Callback slot 4: the script's question about the monster. Query 1: is a
 * Rathian (EM_KIND_RATHIAN) on the ground within Em.nearRadius. Returns 1
 * or 0.
 */
s32 EmVelociprey_ScriptQuery(Em *em, u8 query)
{
    switch (query) {
    case 1:
        if (EmList_FindKindNear(em, EM_KIND_RATHIAN, 1, em->nearRadius) == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

/* Callback slot 2: nothing. */
void EmVelociprey_TableNop(void)
{
}

/*
 * Enters a state of group 0, EM_GROUP_IDLE, on the ground. In combat,
 * VPREY_IDLE becomes the alert idle while hp is at least 30 percent, and so
 * does VPREY_IDLE_WAIT on half the frames (Em.stateRand even) when the goal
 * is a player beyond Em.nearRadius. VPREY_IDLE_STUN_END ends status 5.
 */
void EmVelociprey_SetIdleState(Em *em, u16 state, u8 mode)
{
    switch (state) {
    case VPREY_IDLE:
        if (em->combat == 1 && em->hp >= (s16) (0.3f * em->hpMax)) {
            state = VPREY_IDLE_ALERT;
        }
        break;
    case VPREY_IDLE_WAIT:
        if (em->combat == 1 && em->curGoalKind == 1 && em->curGoalArg == 0 && em->curGoalNo != -1 &&
            em->stateRand % 2 == 0 && !(((f32 *) em->playerDist)[em->curGoalNo] <= em->nearRadius)) {
            state = VPREY_IDLE_ALERT;
        }
        break;
    case VPREY_IDLE_STUN_END:
        Em_EndStatus5(em);
        break;
    }
    em->stance = EM_STANCE_GROUND;
    Em_SetStateSync(em, EM_GROUP_IDLE, state, mode);
}

/*
 * Enters a state of group 1, EM_GROUP_MOVE, on the ground: VPREY_MOVE_TURN
 * becomes VPREY_MOVE_TURN_ALERT in combat; VPREY_MOVE_WALK sets the walking
 * speed (Em_SetSpeedScaled 1000, 300 short of a player).
 */
void EmVelociprey_SetMoveState(Em *em, u16 state, u8 mode)
{
    em->stance = EM_STANCE_GROUND;
    switch (state) {
    case VPREY_MOVE_TURN:
        if (em->combat == 1) {
            state = VPREY_MOVE_TURN_ALERT;
        }
        break;
    case VPREY_MOVE_WALK:
        Em_SetSpeedScaled(em, 2, 1000.0f, 300.0f);
        break;
    }
    Em_SetStateSync(em, EM_GROUP_MOVE, state, mode);
}

/* Enters a state of group 2, the hops. */
void EmVelociprey_SetHopState(Em *em, u16 state, u8 mode)
{
    Em_SetStateSync(em, EM_GROUP_FLY, state, mode);
}

/*
 * Enters a state of group 3, EM_GROUP_ATTACK: the melee attacks and leaps
 * aim at the goal (Em.hasGoal) when it is a player and there is a target;
 * the shots don't.
 */
void EmVelociprey_SetAttackState(Em *em, u16 state, u8 mode)
{
    switch (state) {
    case VPREY_ATK_STRIKE_A:
    case VPREY_ATK_LEAP_MEDIUM_A:
    case VPREY_ATK_LEAP_LONG_A:
    case VPREY_ATK_LEAP_HIGH_A:
    case VPREY_ATK_STRIKE_B:
    case VPREY_ATK_LEAP_MEDIUM_B:
    case VPREY_ATK_QUICK_STRIKE_B:
    case VPREY_ATK_LEAP_LONG_B:
    case VPREY_ATK_LEAP_HIGH_B:
    case VPREY_ATK_STRIKE_C:
    case VPREY_ATK_QUICK_STRIKE_A:
    case VPREY_ATK_NONE:
        em->hasGoal = 0;
        if ((em->curGoalKind == 1) && (em->targetNo != -1)) {
            em->hasGoal = 1;
        }
        break;
    }
    Em_SetStateSync(em, EM_GROUP_ATTACK, state, mode);
}

/*
 * The family's state change (Em_ChangeState dispatches here by kind):
 * removes the pack call's object if it is out, then enters the state
 * through the group's Set*State, or directly: VPREY_DEFEAT_REAPPEAR first
 * lets the quest place it (QuestEm_Appear), VPREY_REACT_STUN begins status
 * 5.
 */
void EmVelociprey_ChangeState(Em *em, u16 group, u16 state, u8 mode)
{
    EmVelocipreyWork *w = (EmVelocipreyWork *)&em->work;

    if (w->callActive != 0) {
        ObjListA70_Remove(&w->callObj);
        w->callActive = 0;
    }
    switch (group) {
    case EM_GROUP_IDLE:
        EmVelociprey_SetIdleState(em, state, mode);
        break;
    case EM_GROUP_MOVE:
        EmVelociprey_SetMoveState(em, state, mode);
        break;
    case EM_GROUP_FLY:
        EmVelociprey_SetHopState(em, state, mode);
        break;
    case EM_GROUP_ATTACK:
        EmVelociprey_SetAttackState(em, state, mode);
        break;
    case EM_GROUP_EVENT:
        Em_SetStateSync(em, group, state, mode);
        break;
    case EM_GROUP_DEFEAT:
        if (state == 4 && QuestEm_CanAppear(em) == 1) {
            QuestEm_Appear(em);
        }
        Em_SetStateSync(em, group, state, mode);
        break;
    case EM_GROUP_REACT:
        switch (state) {
        case VPREY_REACT_STUN:
            Em_BeginStatus5(em);
            break;
        }
        Em_SetStateSync(em, group, state, mode);
        break;
    case EM_GROUP_EVENT2:
        Em_SetStateSync(em, group, state, mode);
        break;
    }
}

/*
 * Starts the hop curves of curveKind (EmVelocipreyCurveKind) from frame 22,
 * with the velocity cleared.
 */
void EmVelociprey_StartCurves(Em *em, u8 kind)
{
    EmVelocipreyWork *w = (EmVelocipreyWork *)&em->work;

    w->curveNone = 0;
    w->curveVelY = 0;
    w->curveSpeed = 0;
    w->curveKind = kind;
    switch (w->curveKind) {
    case 0:
        w->curveVelY = 1;
        w->curveSpeed = 1;
        w->curveFrame = 22;
        break;
    case 1:
        w->curveVelY = 1;
        w->curveFrame = 22;
        break;
    case VPREY_CURVE_HOP_FORWARD:
        w->curveVelY = 1;
        w->curveSpeed = 1;
        w->curveFrame = 22;
        break;
    case VPREY_CURVE_HOP_UP:
        w->curveVelY = 1;
        w->curveFrame = 22;
        break;
    }
    Player_ClearVelocity(em);
}


/* A curve that does nothing; no kind runs it. */
static s32 EmVelociprey_CurveNone(Em *em, EmWork *work)
{
    return 0;
}

extern EmCurveKey *EmVelociprey_VelYCurves[];

/*
 * The vertical curve: sets Em.velY from the keys of the curve kind's table
 * at the curve frame (the first key's value at frames 0 and 1). Past the
 * table's end it returns 2 and falls 1 faster a frame, to -50.
 */
static u8 EmVelociprey_CurveVelY(Em *em, EmWork *work)
{
    EmVelocipreyWork *w = (EmVelocipreyWork *) work;
    EmCurveKey *key;
    f32 frame;
    f32 keyTime;
    s32 keyIdx;
    EmCurveKey *keys;
    u8 endBits;

    keys = EmVelociprey_VelYCurves[((EmVelocipreyWork *)work)->curveKind];
    endBits = 0;
    if (keys == NULL) {
        return endBits;
    }
    frame = w->curveFrame;
    if (frame == 0.0f || frame == 1.0f) {
        em->velY = keys->v;
    } else {
        keyIdx = 1;
        while (keyIdx != 0) {
            key = &keys[keyIdx];
            keyTime = key->t;
            if (frame > keyTime && keyTime != 0.0f) {
                keyIdx++;
            } else if (keyTime == 0.0f) {
                endBits = 2;
                if (em->velY > -50.0f) {
                    em->velY -= 1.0f;
                }
                break;
            } else {
                keyIdx = 0;
                em->velY = (key->v - key[-1].v) / ((keyTime - key[-1].t) / em->motion[0].step);
            }
        }
    }
    return endBits;
}

extern EmCurveKey *EmVelociprey_SpeedCurves[];

/*
 * The forward curve: sets Em.velZ from the curve kind's table as
 * EmVelociprey_CurveVelY does; returns 4 past the table's end. A wall in
 * front (Em.wallHitDirs 0xF000000F) stops it.
 */
static u8 EmVelociprey_CurveSpeed(Em *em, EmWork *work)
{
    EmVelocipreyWork *w = (EmVelocipreyWork *) work;
    EmCurveKey *key;
    f32 frame;
    f32 keyTime;
    s32 keyIdx;
    EmCurveKey *keys;
    u8 endBits;

    keys = EmVelociprey_SpeedCurves[((EmVelocipreyWork *)work)->curveKind];
    endBits = 0;
    if (keys == NULL) {
        return endBits;
    }
    frame = w->curveFrame;
    if (frame == 0.0f || frame == 1.0f) {
        em->velZ = keys->v;
    } else {
        keyIdx = 1;
        while (keyIdx != 0) {
            key = &keys[keyIdx];
            keyTime = key->t;
            if (frame > keyTime && keyTime != 0.0f) {
                keyIdx++;
            } else if (keyTime == 0.0f) {
                endBits = 4;
                break;
            } else {
                keyIdx = 0;
                em->velZ = (key->v - key[-1].v) / ((keyTime - key[-1].t) / em->motion[0].step);
            }
        }
    }
    if (em->wallHitDirs & 0xF000000F) {
        *(s32 *) &em->velZ = 0;
    }
    return endBits;
}

/*
 * Every frame of a curve hop: runs the curves of EmVelociprey_StartCurves'
 * kind, advances the curve frame, and moves the monster while it is in the
 * air. Returns their end bits (2 the vertical, 4 the forward curve),
 * nonzero once one ran past its table.
 */
u8 EmVelociprey_UpdateCurves(Em *em)
{
    EmVelocipreyWork *w = (EmVelocipreyWork *)&em->work;
    u8 endBits = 0;

    if (w->curveNone != 0) {
        endBits = EmVelociprey_CurveNone(em, (EmWork *) w);
    }
    if (w->curveVelY != 0) {
        endBits |= EmVelociprey_CurveVelY(em, (EmWork *) w);
    }
    if (w->curveSpeed != 0) {
        endBits |= EmVelociprey_CurveSpeed(em, (EmWork *) w);
    }
    w->curveFrame += (s16) em->motion[0].step;
    if (em->stance == EM_STANCE_AIR) {
        w->accelAngX = 0;
        w->accelAngY = em->rotY;
        w->accelAngZ = 0;
        Em_MoveRotatedAccel(em, &w->accelAngX);
    }
    return endBits;
}

/*
 * The Anteka family's motion sounds: footsteps, thuds and calls at fixed
 * frames of the layer-0 motion in work->fxMotion (some picked at random by
 * Sound_RandomSe; one only while hunted). Called by EmAnteka_MotionEffects.
 */
void EmAnteka_MotionSounds(Em *em, EmWork *work)
{
    s16 motion;

    motion = work->fxMotion;
    switch (motion) {
    case EM_MOTION_ID(0, ANTEKA_MOT_IDLE):
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_IDLE_ACTION_A):
        Em_PlaySeAtBoneOnFrame(em, 0x5C, 0xD, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x7A, 0xD, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x92, 0xD, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0xB6, 0xD, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0xD2, 0xD, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0xF0, 0xD, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x10E, 0xD, 0x17);
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_IDLE_ACTION_B):
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0xB, 0x17);
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_IDLE_ACTION_C):
        Em_PlaySeAtBoneOnFrame(em, 6, 4, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0, 0);
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_IDLE_ACTION_D):
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x10, 4, 0x11, 4), 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x54, Sound_RandomSe(0x11, 4, 0x12, 4), 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0xC8, Sound_RandomSe(0x10, 4, 0x12, 4), 0x17);
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_IDLE_ACTION_E):
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0xE, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 0xE, 0x17);
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_WALK):
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 0, 0);
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_RUN):
        Em_PlaySeAtBoneOnFrame(em, 6, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x13, 0);
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_CHARGE):
        Em_PlaySeAtBoneOnFrame(em, 0x10, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0x13, 0);
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_MOVE_ACTION_A):
    case EM_MOTION_ID(0, ANTEKA_MOT_MOVE_ACTION_B):
        Em_PlaySeAtBoneOnFrame(em, 0xC, 1, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0x13, 0);
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_RECOVER):
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 0, 0);
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_TURN_AROUND_RIGHT):
    case EM_MOTION_ID(0, ANTEKA_MOT_TURN_AROUND_LEFT):
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 1, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 0x13, 0);
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_ATK_COMBO):
        Em_PlaySeAtBoneOnFrame(em, 4, 7, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 8, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 9, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x94, 0xA, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0, 0);
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_ATK_SLAM):
        Em_PlaySeAtBoneOnFrame(em, 0xE, 2, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 1, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0, 0);
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_TURN_QUICK):
        Em_PlaySeAtBoneOnFrame(em, 0x18, 1, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 0x13, 0);
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_LEAP):
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0x13, 0);
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_ATK_STRIKE):
        Em_PlaySeAtBoneOnFrame(em, 0xC, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x36, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 0, 0);
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_IDLE_ACTION_F):
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 1, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x11, 0);
        break;
    case EM_MOTION_ID(0, 0x27):
        Em_PlaySeAtBoneOnFrame(em, 2, 0xB, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x17, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x5A, 2, 0);
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_FLINCH_A):
    case EM_MOTION_ID(0, ANTEKA_MOT_FLINCH_B):
        Em_PlaySeAtBoneOnFrame(em, 4, 5, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 0, 0);
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_BLOWN_BACK):
        Em_PlaySeAtBoneOnFrame(em, 4, 1, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 4, 5, 0);
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_BLOWN_BACK_LAND):
        if ((em->stateGroup == EM_GROUP_DEFEAT) && (em->state == 0)) {
            Em_PlaySeAtBoneOnFrame(em, 0x3E, 0xC, 0);
            Em_PlaySeAtBoneOnFrame(em, 4, 6, 0);
            Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x14, 0);
            break;
        }
        Em_PlaySeAtBoneOnFrame(em, 4, 6, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x14, 0);
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_GET_UP):
        Em_PlaySeAtBoneOnFrame(em, 0xE, 1, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 1, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 0, 0);
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_COLLAPSE):
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x42, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x7C, 0x14, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x62, 1, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 4, 0);
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_LIE):
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_STATUS4):
        Em_PlaySeAtBoneOnFrame(em, 4, 0xF, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x8A, 0xF, 0x17);
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_STATUS4_END):
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x10, 4, 0x11, 4), 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0, 0);
        
        break;
    }
}

/*
 * Sets up an Anteka (EM_KIND_ANTEKA) or a Kelbi (EM_KIND_KELBI): outside a quest a test
 * placement by slot number; the blindness, stance and motion speed; hp from
 * Em_ComputeMaxHp; Em.gender (the Kelbi only: from the quest entry's variant,
 * giving a body scaled by 0.85, a second hp row and the layer-1 idle); the
 * idle motion, the parts, and the work byte from Em.spawnParam.
 */
void EmAnteka_Init(Em *em)
{
    EmWork *w = (EmWork *)&em->work;
    s16 hp;

    if (gQuest.stage == 0) {
        switch (gScene.area) {
        case AREA_FOREST_HILLS_4:
            em->posX = 9500.0f + 150.0f * (f32) (em->slotNo / 8);
            em->posY = 0.0f;
            em->posZ = 10000.0f + 150.0f * (f32) (em->slotNo & 7);
            break;
        default:
            em->posX = 5000.0f - 80.0f * (f32) em->slotNo;
            em->posY = 0.0f;
            em->posZ = 5000.0f - 80.0f * (f32) em->slotNo;
            break;
        }
    }
    em->blind = 0;
    em->stance = EM_STANCE_GROUND;
    em->motionSpeed = 1.0f;
    hp = Em_ComputeMaxHp(em, 0);
    em->hp = hp;
    em->hpMax = hp;
    if (em->type == EM_KIND_KELBI) {
        if (gQuest.stage == 0) {
            em->gender = (u16) Rand_Next(1) & 1;
            em->gender = 0;
        } else {
            em->gender = em->variant;
        }
        if (em->gender != 0) {
            hp = Em_ComputeMaxHp(em, 1);
            em->hp = hp;
            em->hpMax = hp;
            em->scaleX *= 0.85f;
            em->scaleY *= 0.85f;
            em->scaleZ *= 0.85f;
        }
        em->bodyScale = em->scaleX;
        GmEm_ApplySize(em);
    } else {
        em->gender = 0;
    }
    Em_SetMotionRow(em, ANTEKA_MOT_IDLE, 0, 0);
    if (em->gender != 0) {
        Em_SetLayerMotion(em, EM_MOTION_ID(1, ANTEKA_MOT_IDLE), 0, 0, 1);
    }
    Em_InitParts(em);
    switch (em->spawnParam) {
    case 1:
        w->unk019 = 1;
        break;
    default:
        w->unk019 = 0;
        break;
    }
}

/* Init, then ANTEKA_IDLE. */
void EmAnteka_Start(Em *em)
{
    EmAnteka_Init(em);
    EmAnteka_ChangeState(em, EM_GROUP_IDLE, ANTEKA_IDLE, EM_SYNC_LOCAL);
}

/*
 * Runs Init again: the family's appear hook (EmVt.appear, slot 5 of
 * EmAnteka_Callbacks).
 */
void EmAnteka_Reinit(Em *em)
{
    EmAnteka_Init(em);
}

/*
 * The per-frame update. Out of an event it asks for script requests (a
 * large monster fighting in its area; Em_InBlockArea1), then turns
 * Em_UpdateDamage's result into a state: killed or dead ANTEKA_DEFEAT_DIE;
 * a part or mode-3 hit a flinch by side, or ANTEKA_REACT_BLOWN_BACK when
 * the hit comes from behind, part 0 is out of hp or it is off the ground; a
 * plain hit on the ground a flinch by side; statuses 2 and 4 their states;
 * a hit while lying ANTEKA_REACT_BLOWN_BACK_LYING; the stun only ends
 * status 5. The family is never trapped or captured. Then the pending
 * script, and the state (twice when a state was requested).
 */
void EmAnteka_Update(Em *em)
{
    u8 largeFighting = 0;
    u8 inGroup6 = 0;
    u8 largeCalm = 0;
    s16 i;
    Em *other = gEms;
    u8 hitInfo[3];

    Em_UpdateCombatTimer(em);
    if (em->requested != 1 && em->stateGroup != EM_GROUP_EVENT) {
        for (i = 0; i < 20; i++, other++) {
            if (other->active && other->visible && (Obj_SameArea(em, (Player *) other) & 0xFF)) {
                if (gEmShowsOnMap[other->type] != 0) {
                    if (other->combat == 1) {
                        largeFighting = 1;
                        break;
                    }
                    if (other->combat == 0) {
                        largeCalm = 1;
                    }
                }
                if (other->variant == 0 && other->stateGroup == EM_GROUP_EVENT) {
                    inGroup6 = 1;
                }
            }
        }
        if (largeFighting != 0 || (largeCalm != 0 && inGroup6 != 0)) {
            Em_SetRequest(em, 1, 0x90);
            Em_RestartScript(em);
            em->scriptPending = 1;
        }
    }
    switch (Em_UpdateDamage(em, hitInfo) & 0xFF) {
    case EM_DMG_NONE:
    case EM_DMG_ATTACK9:
    case EM_DMG_ATTACK6:
    case EM_DMG_STATUS_IN_AIR:
    case EM_DMG_CAPTURED:
    case EM_DMG_FLASH:
    case EM_DMG_PART8_DOWN:
    case EM_DMG_ATTACK5:
    case EM_DMG_ATTACK5_FLINCH:
    case EM_DMG_ATTACK13:
        break;
    case EM_DMG_KILLED:
    case EM_DMG_DEAD:
        EmAnteka_ChangeState(em, EM_GROUP_DEFEAT, ANTEKA_DEFEAT_DIE, EM_SYNC_OWNER);
        break;
    case EM_DMG_STATUS4:
        Em_SetStatus4Timer(em);
        EmAnteka_ChangeState(em, EM_GROUP_REACT, ANTEKA_REACT_STATUS4, EM_SYNC_OWNER);
        break;
    case EM_DMG_STATUS2:
        Em_SetStatus2Timer(em);
        EmAnteka_ChangeState(em, EM_GROUP_IDLE, ANTEKA_IDLE_STATUS2, EM_SYNC_OWNER);
        break;
    case EM_DMG_HIT_LYING:
        EmAnteka_ChangeState(em, EM_GROUP_REACT, ANTEKA_REACT_BLOWN_BACK_LYING, EM_SYNC_OWNER);
        break;
    case EM_DMG_PART_FLINCH:
    case EM_DMG_HIT_MODE3:
        {
            s32 hitDelta = em->hitAngle - em->rotY;

            if (((hitDelta - 0x4000) & 0xFFFF) < 0x8000 && em->parts[0].hp > 0 && em->stance == EM_STANCE_GROUND) {
                if ((hitDelta & 0xFFFF) < 0x8000) {
                    EmAnteka_ChangeState(em, EM_GROUP_REACT, ANTEKA_REACT_FLINCH_A, EM_SYNC_OWNER);
                } else {
                    EmAnteka_ChangeState(em, EM_GROUP_REACT, ANTEKA_REACT_FLINCH_B, EM_SYNC_OWNER);
                }
            } else {
                EmAnteka_ChangeState(em, EM_GROUP_REACT, ANTEKA_REACT_BLOWN_BACK, EM_SYNC_OWNER);
            }
        }
        break;
    case EM_DMG_HIT:
        if (em->stance == EM_STANCE_GROUND) {
            s32 plainHitDelta = (em->hitAngle - em->rotY) & 0xFFFF;

            em->scriptPending = 0;
            if (plainHitDelta < 0x8000) {
                EmAnteka_ChangeState(em, EM_GROUP_REACT, ANTEKA_REACT_FLINCH_A, EM_SYNC_OWNER);
            } else {
                EmAnteka_ChangeState(em, EM_GROUP_REACT, ANTEKA_REACT_FLINCH_B, EM_SYNC_OWNER);
            }
        }
        break;
    case EM_DMG_STATUS5:
        Em_EndStatus5(em);
        break;
    }
    if (em->requested != 1 && (u8) Em_InBlockArea1(em) == 1) {
        Em_SetRequest(em, 2, 0x90);
        Em_RestartScript(em);
        em->scriptPending = 1;
    }
    Em_UpdatePendingScript(em);
    Em_ClearHitReactMode(em);
    EmAnteka_RunState(em);
    if (em->stateRequest != 0) {
        EmAnteka_RunState(em);
        em->stateRequest = 0;
    }
}

/* ANTEKA_IDLE_NONE: does nothing. */
void EmAnteka_IdleNone(void)
{
}

/*
 * ANTEKA_IDLE: plays the idle motion once, then EmAnteka_EndState (the
 * script picks the next state).
 */
void EmAnteka_Idle(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, ANTEKA_MOT_IDLE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
}

/* ANTEKA_IDLE_ACTION_A: plays its motion once, then EmAnteka_EndState. */
void EmAnteka_IdleActionA(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, ANTEKA_MOT_IDLE_ACTION_A, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
}

/* ANTEKA_IDLE_ACTION_B: as ANTEKA_IDLE_ACTION_A with its own motion. */
void EmAnteka_IdleActionB(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, ANTEKA_MOT_IDLE_ACTION_B, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
}

/* ANTEKA_IDLE_ACTION_C: as ANTEKA_IDLE_ACTION_A with its own motion. */
void EmAnteka_IdleActionC(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, ANTEKA_MOT_IDLE_ACTION_C, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
}

/* ANTEKA_IDLE_ACTION_D: as ANTEKA_IDLE_ACTION_A with its own motion. */
void EmAnteka_IdleActionD(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, ANTEKA_MOT_IDLE_ACTION_D, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
}

/* ANTEKA_IDLE_ACTION_E: as ANTEKA_IDLE_ACTION_A with its own motion. */
void EmAnteka_IdleActionE(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, ANTEKA_MOT_IDLE_ACTION_E, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
}

/*
 * ANTEKA_IDLE_FACE_GOAL: sets targetAngle toward the goal and plays the
 * quarter-turn motion for the goal's side; rotY is not changed here.
 */
void EmAnteka_FaceGoal(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->targetAngle = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        if (((em->targetAngle - em->rotY) & 0xFFFF) < 0x8001) {
            EmAnteka_SetMotion(em, ANTEKA_MOT_TURN_RIGHT, 0, 0);
            break;
        }
        EmAnteka_SetMotion(em, ANTEKA_MOT_TURN_LEFT, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
}

/*
 * ANTEKA_IDLE_STATUS2: begins status 2, collapses and lies (hit-react mode
 * EM_HIT_REACT_WAKE, blind 2) until the status timer (Em_SetStatus2Timer)
 * runs out, then ANTEKA_IDLE_STATUS2_END.
 */
void EmAnteka_Status2Collapse(Em *em, EmWork *work)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        Em_BeginStatus2(em);
        Em_SetMotion(em, ANTEKA_MOT_COLLAPSE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, ANTEKA_MOT_LIE, 0, 0);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            EmAnteka_ChangeState(em, EM_GROUP_IDLE, ANTEKA_IDLE_STATUS2_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * ANTEKA_IDLE_STATUS2_END: ends status 2, gets up, sees again, then
 * EmAnteka_EndState.
 */
void EmAnteka_Status2End(Em *em, EmWork *work)
{
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        Em_EndStatus2(em);
        Em_SetMotion(em, ANTEKA_MOT_GET_UP, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            EmAnteka_EndState(em);
        }
        break;
    }
}

/*
 * ANTEKA_IDLE_STATUS4_END: ends status 4 and plays its end motion, then
 * EmAnteka_EndState.
 */
void EmAnteka_Status4End(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, ANTEKA_MOT_STATUS4_END, 0, 0);
        Em_EndStatus4(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->noReact = 0;
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
}

/* ANTEKA_IDLE_ACTION_F: as ANTEKA_IDLE_ACTION_A with its own motion. */
void EmAnteka_IdleActionF(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, ANTEKA_MOT_IDLE_ACTION_F, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
}

/*
 * Runs the group 0 state (EmAnteka_RunState also sends group 2 here),
 * keeping the group and state as the previous ones.
 */
void EmAnteka_RunIdleState(Em *em, EmWork *work)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->state) {
    case ANTEKA_IDLE_NONE:
        EmAnteka_IdleNone();
        break;
    case ANTEKA_IDLE:
        EmAnteka_Idle(em, work);
        break;
    case ANTEKA_IDLE_ACTION_A:
        EmAnteka_IdleActionA(em, work);
        break;
    case ANTEKA_IDLE_ACTION_B:
        EmAnteka_IdleActionB(em, work);
        break;
    case ANTEKA_IDLE_ACTION_C:
        EmAnteka_IdleActionC(em, work);
        break;
    case ANTEKA_IDLE_ACTION_D:
        EmAnteka_IdleActionD(em, work);
        break;
    case ANTEKA_IDLE_ACTION_E:
        EmAnteka_IdleActionE(em, work);
        break;
    case ANTEKA_IDLE_FACE_GOAL:
        EmAnteka_FaceGoal(em, work);
        break;
    case ANTEKA_IDLE_STATUS2:
        EmAnteka_Status2Collapse(em, work);
        break;
    case ANTEKA_IDLE_STATUS2_END:
        EmAnteka_Status2End(em, work);
        break;
    case ANTEKA_IDLE_STATUS4_END:
        EmAnteka_Status4End(em, work);
        break;
    case ANTEKA_IDLE_ACTION_F:
        EmAnteka_IdleActionF(em, work);
        
    default:
        break;
    }
}

/*
 * ANTEKA_MOVE_WALK_TURN: walks while rotY turns toward the goal's angle
 * (0x100 a frame); ends when within 0x100.
 */
void EmAnteka_WalkTurn(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->targetAngle = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        EmAnteka_SetMotion(em, ANTEKA_MOT_WALK, 0, 0);
        break;
    case 1:
        if (Angle_TurnToward((s32 *) &em->rotY, (s32 *) &em->targetAngle, 0x100) < 0x100) {
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
    Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
}

/*
 * ANTEKA_MOVE_WALK: walks to the goal (Em_StepToGoal), then
 * EmAnteka_EndState.
 */
void EmAnteka_Walk(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, ANTEKA_MOT_WALK, 0, 0);
        break;
    case 1:
        if (Em_StepToGoal(em, 0x40, 0) == 1) {
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
}

/*
 * ANTEKA_MOVE_RUN and ANTEKA_MOVE_RUN_FAR (they differ only in the stop
 * distance EmAnteka_ChangeState sets; `farStop` tells them apart and is not
 * read): runs to the goal, then the recover motion, then EmAnteka_EndState.
 */
void EmAnteka_Run(Em *em, EmWork *work, s32 farStop)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, ANTEKA_MOT_RUN, 0, 0);
        break;
    case 1:
        if ((Em_StepToGoal(em, 0x40, 0) == 1) && (em->motion[0].playing == 0)) {
            em->step++;
            EmAnteka_SetMotion(em, ANTEKA_MOT_RECOVER, 4, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
}

/*
 * ANTEKA_MOVE_CHARGE and ANTEKA_MOVE_CHARGE_FAR: as EmAnteka_Run with the
 * charge motion; an Anteka (not a Kelbi) also makes one contact attack at
 * frame 20 (stateVar0 keeps it to one).
 */
void EmAnteka_Charge(Em *em, EmWork *work, s32 farStop)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stateVar0 = 0;
        EmAnteka_SetMotion(em, ANTEKA_MOT_CHARGE, 0, 0);
        break;
    case 1:
        if ((em->type == EM_KIND_ANTEKA) && (em->stateVar0 == 0) && (EmMotion_PassedFrameScaled(em, 20.0f, 0U) != 0)) {
            EmContactAttack_Create(em, 0x16);
            em->stateVar0++;
        }
        if ((Em_StepToGoal(em, 0x40, 0) == 1) && (em->motion[0].playing == 0)) {
            em->step++;
            EmAnteka_SetMotion(em, ANTEKA_MOT_RECOVER, 4, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
}

/*
 * ANTEKA_MOVE_ACTION_A: plays its motion, then the recover motion from
 * frame 26, then EmAnteka_EndState.
 */
void EmAnteka_MoveActionA(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, ANTEKA_MOT_MOVE_ACTION_A, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_SetMotion(em, ANTEKA_MOT_RECOVER, 0, 0x1A);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
}

/* ANTEKA_MOVE_ACTION_B: as ANTEKA_MOVE_ACTION_A with its own motion. */
void EmAnteka_MoveActionB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, ANTEKA_MOT_MOVE_ACTION_B, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_SetMotion(em, ANTEKA_MOT_RECOVER, 0, 0x1A);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
}

/*
 * ANTEKA_MOVE_TURN_AROUND_RIGHT: a half turn: rotY grows by 0x888 a frame
 * once the blend is over, plus 8 at frame 30 (exactly 0x8000 in all), then
 * EmAnteka_EndState.
 */
void EmAnteka_TurnAroundRight(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, ANTEKA_MOT_TURN_AROUND_RIGHT, 0, 0);
        break;
    case 1:
        if (em->motion[0].blend == 0) {
            em->rotY += 0x888;
            if (EmMotion_ReachedFrame(em, 0, 30.0f) != 0) {
                em->step++;
                em->rotY += 8;
                break;
            }
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
}

/* ANTEKA_MOVE_TURN_AROUND_LEFT: the mirror of EmAnteka_TurnAroundRight. */
void EmAnteka_TurnAroundLeft(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, ANTEKA_MOT_TURN_AROUND_LEFT, 0, 0);
        break;
    case 1:
        if (em->motion[0].blend == 0) {
            em->rotY -= 0x888;
            if (EmMotion_ReachedFrame(em, 0, 30.0f) != 0) {
                em->step++;
                em->rotY -= 8;
                break;
            }
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
}

/*
 * ANTEKA_MOVE_TURN_QUICK: plays the quick-turn motion alone (rotY
 * unchanged), then EmAnteka_EndState.
 */
void EmAnteka_TurnQuick(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, ANTEKA_MOT_TURN_QUICK, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
}

/*
 * ANTEKA_MOVE_ACTION_SERIES: plays the two move actions twice in turn, then
 * EmAnteka_EndState.
 */
void EmAnteka_MoveActionSeries(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, ANTEKA_MOT_MOVE_ACTION_A, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_SetMotion(em, ANTEKA_MOT_MOVE_ACTION_B, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_SetMotion(em, ANTEKA_MOT_MOVE_ACTION_A, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_SetMotion(em, ANTEKA_MOT_MOVE_ACTION_B, 0, 0);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
}

/*
 * ANTEKA_MOVE_TURN_TO_GOAL: picks a turn motion from the goal's side and
 * angle (a quarter turn, the Kelbi's quick turn, or a half turn) and steps
 * rotY by hand each frame (the angle over half the motion's last frame),
 * snapping onto targetAngle when within a step; ends with the motion.
 */
void EmAnteka_TurnToGoal(Em *em, EmWork *work)
{
    f32 turnFrame;
    u32 turnStep;
    u32 angleDiff;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        em->targetAngle = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        angleDiff = (em->targetAngle - em->rotY) & 0xFFFF;
        if ((em->type == EM_KIND_ANTEKA && angleDiff <= 0x4000U) || angleDiff >= 0xC000U || (em->type != EM_KIND_ANTEKA && angleDiff <= 0x3000U) || angleDiff >= 0xD000U) {
            if (em->type == EM_KIND_ANTEKA) {
                if (angleDiff >= 0x8000U) {
                    EmAnteka_SetMotion(em, ANTEKA_MOT_TURN_LEFT, 0, 0);
                } else {
                    EmAnteka_SetMotion(em, ANTEKA_MOT_TURN_RIGHT, 0, 0);
                }
            } else {
                EmAnteka_SetMotion(em, ANTEKA_MOT_TURN_QUICK, 0, 0);
            }
        } else if (angleDiff >= 0x8000U) {
            EmAnteka_SetMotion(em, ANTEKA_MOT_TURN_AROUND_LEFT, 0, 0);
        } else {
            EmAnteka_SetMotion(em, ANTEKA_MOT_TURN_AROUND_RIGHT, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].blend != 0) {
            break;
        }
        switch (em->motionId[0]) {
        case 0x3FA:
            turnFrame = 26.0f;
            turnStep = 0x4000U / (u32) (turnFrame / 2.0f);
            break;
        case 0x3EF:
        case 0x3F0:
            turnFrame = 48.0f;
            turnStep = 0x4000U / (u32) (turnFrame / 2.0f);
            break;
        case 0x3F7:
        case 0x3FE:
        default:
            turnFrame = 30.0f;
            turnStep = 0x8000U / (u32) (turnFrame / 2.0f);
            break;
        }
        angleDiff = (em->targetAngle - (em->rotY & 0xFFFF)) & 0xFFFF;
        if ((u32) ((angleDiff + turnStep) & 0xFFFF) < turnStep * 2) {
            em->rotY = em->targetAngle;
        } else if (angleDiff < 0x8000U) {
            em->rotY = (em->rotY + turnStep) & 0xFFFF;
        } else {
            em->rotY = (em->rotY - turnStep) & 0xFFFF;
        }
        if (EmMotion_ReachedFrame(em, 0, turnFrame) != 0) {
            em->step++;
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
}

/*
 * ANTEKA_MOVE_LEAP: a forward leap (velY 75, accelY -10, velZ 50), airborne
 * from frame 26; the motion holds at frame 36 until it lands, then the
 * recover motion, then EmAnteka_EndState. `entryStep` is the step on entry,
 * written back + 1 (the original's form).
 */
void EmAnteka_Leap(Em *em, EmWork *work)
{
    u8 tookOff;
    u8 entryStep;

    entryStep = em->step;
    tookOff = 0;
    switch (entryStep) {
    case 0:
        em->step = entryStep + 1;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, ANTEKA_MOT_LEAP, 0, 0);
        Player_ClearVelocity(em);
        em->velY = 75.0f;
        em->accelY = -10.0f;
        em->velZ = 50.0f;
        work->accelAngX = 0;
        work->accelAngZ = 0;
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 26.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            tookOff = 1;
            work->accelAngY = em->rotY;
            Em_MoveRotatedAccel(em, &work->accelAngX);
        }
        if (EmMotion_ReachedFrame(em, 0, 36.0f) != 0) {
            em->motionSpeed = 0.0f;
        }
        Em_GetFloorY(em, &em->posX, &em->floorY);
        if (tookOff && (em->posY <= em->floorY)) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            em->motionSpeed = 1.0f;
        }
        break;
    case 2:
        if (EmMotion_ReachedFrame(em, 0, 44.0f) != 0) {
            em->step++;
            Em_SetMotion(em, ANTEKA_MOT_RECOVER, 4, 0xA);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step = entryStep + 1;
            EmAnteka_EndState(em);
        }
        break;
    }
}

/*
 * Runs the group 1 state, keeping the group and state as the previous ones;
 * ANTEKA_MOVE_RUN_FAR and ANTEKA_MOVE_CHARGE_FAR run the functions of
 * ANTEKA_MOVE_RUN and ANTEKA_MOVE_CHARGE.
 */
void EmAnteka_RunMoveState(Em *em, EmWork *work)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->state) {
    case ANTEKA_MOVE_WALK_TURN:
        EmAnteka_WalkTurn(em);
        break;
    case ANTEKA_MOVE_WALK:
        EmAnteka_Walk(em);
        break;
    case ANTEKA_MOVE_RUN:
        EmAnteka_Run(em, work, 0);
        break;
    case ANTEKA_MOVE_CHARGE:
        EmAnteka_Charge(em, work, 0);
        break;
    case ANTEKA_MOVE_ACTION_A:
        EmAnteka_MoveActionA(em);
        break;
    case ANTEKA_MOVE_ACTION_B:
        EmAnteka_MoveActionB(em);
        break;
    case ANTEKA_MOVE_TURN_AROUND_RIGHT:
        EmAnteka_TurnAroundRight(em);
        break;
    case ANTEKA_MOVE_TURN_AROUND_LEFT:
        EmAnteka_TurnAroundLeft(em);
        break;
    case ANTEKA_MOVE_TURN_QUICK:
        EmAnteka_TurnQuick(em);
        break;
    case ANTEKA_MOVE_RUN_FAR:
        EmAnteka_Run(em, work, 1);
        break;
    case ANTEKA_MOVE_CHARGE_FAR:
        EmAnteka_Charge(em, work, 1);
        break;
    case ANTEKA_MOVE_ACTION_SERIES:
        EmAnteka_MoveActionSeries(em);
        break;
    case ANTEKA_MOVE_TURN_TO_GOAL:
        EmAnteka_TurnToGoal(em, work);
        break;
    case ANTEKA_MOVE_LEAP:
        EmAnteka_Leap(em, work);
        break;
    }
}

/*
 * ANTEKA_ATK_COMBO: plays the combo motion (its three contact attacks come
 * from EmAnteka_MotionEffects), then EmAnteka_EndState. The other branch of
 * the kind test cannot run: only EM_KIND_KELBI and EM_KIND_ANTEKA reach these
 * functions.
 */
void EmAnteka_AttackCombo(Em *em, EmWork *work)
{
    u8 kind;

    switch (em->step) {
    case 0:
        em->step++;
        kind = em->type;
        if ((kind == 3) || (kind == EM_KIND_ANTEKA)) {
            EmAnteka_SetMotion(em, ANTEKA_MOT_ATK_COMBO, 0, 0);
            break;
        }
        EmAnteka_SetMotion(em, ANTEKA_MOT_IDLE_ACTION_F, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
}

/*
 * ANTEKA_ATK_SLAM: one hit with a ground impact, then the recover motion
 * from frame 26, then EmAnteka_EndState.
 */
void EmAnteka_AttackSlam(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, ANTEKA_MOT_ATK_SLAM, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_SetMotion(em, ANTEKA_MOT_RECOVER, 0, 0x1A);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
}

/*
 * ANTEKA_ATK_STRIKE: the leap's motion in place, then the strike from frame
 * 8 (one contact attack), then EmAnteka_EndState.
 */
void EmAnteka_AttackStrike(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, ANTEKA_MOT_LEAP, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_SetMotion(em, ANTEKA_MOT_ATK_STRIKE, 0, 8);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
}

/* ANTEKA_ATK_NONE_A: EmAnteka_EndState at once. */
void EmAnteka_AttackNoneA(Em *em, EmWork *work)
{
    EmAnteka_EndState(em);
}

/* ANTEKA_ATK_NONE_B: EmAnteka_EndState at once. */
void EmAnteka_AttackNoneB(Em *em, EmWork *work)
{
    EmAnteka_EndState(em);
}

/* ANTEKA_ATK_NONE_C: EmAnteka_EndState at once. */
void EmAnteka_AttackNoneC(Em *em, EmWork *work)
{
    EmAnteka_EndState(em);
}

/* ANTEKA_ATK_NONE_D: EmAnteka_EndState at once. */
void EmAnteka_AttackNoneD(Em *em, EmWork *work)
{
    EmAnteka_EndState(em);
}

/* ANTEKA_ATK_NONE_E: EmAnteka_EndState at once. */
void EmAnteka_AttackNoneE(Em *em, EmWork *work)
{
    EmAnteka_EndState(em);
}

/* ANTEKA_ATK_NONE_F: EmAnteka_EndState at once. */
void EmAnteka_AttackNoneF(Em *em, EmWork *work)
{
    EmAnteka_EndState(em);
}

/*
 * Runs the group 3 state, keeping the group and state as the previous ones.
 * Only the script enters group 3.
 */
void EmAnteka_RunAttackState(Em *em, EmWork *work)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->state) {
    case ANTEKA_ATK_COMBO:
        EmAnteka_AttackCombo(em, work);
        break;
    case ANTEKA_ATK_SLAM:
        EmAnteka_AttackSlam(em, work);
        break;
    case ANTEKA_ATK_STRIKE:
        EmAnteka_AttackStrike(em, work);
        break;
    case ANTEKA_ATK_NONE_A:
        EmAnteka_AttackNoneA(em, work);
        break;
    case ANTEKA_ATK_NONE_B:
        EmAnteka_AttackNoneB(em, work);
        break;
    case ANTEKA_ATK_NONE_C:
        EmAnteka_AttackNoneC(em, work);
        break;
    case ANTEKA_ATK_NONE_D:
        EmAnteka_AttackNoneD(em, work);
        break;
    case ANTEKA_ATK_NONE_E:
        EmAnteka_AttackNoneE(em, work);
        break;
    case ANTEKA_ATK_NONE_F:
        EmAnteka_AttackNoneF(em, work);
        
    default:
        break;
    }
}

/*
 * ANTEKA_REACT_FLINCH_A: plays its flinch, restarts the script, then
 * EmAnteka_EndState.
 */
void EmAnteka_FlinchA(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, ANTEKA_MOT_FLINCH_A, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
}

/* ANTEKA_REACT_FLINCH_B: the mirror flinch. */
void EmAnteka_FlinchB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, ANTEKA_MOT_FLINCH_B, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
}

/*
 * ANTEKA_REACT_BLOWN_BACK and (`fromLying` 1)
 * ANTEKA_REACT_BLOWN_BACK_LYING, which also ends status 2 and stays blind:
 * turns to face the hit, is thrown up and back (stance EM_STANCE_AIR) until
 * it reaches the floor, lies for 90 frames, gets up, then
 * EmAnteka_EndState.
 */
void EmAnteka_BlownBack(Em *em, EmWork *work, s16 fromLying)
{
    switch (fromLying) {
    case 1:
        em->blind = 2;
        break;
    }
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, ANTEKA_MOT_BLOWN_BACK, 0, 0);
        switch (fromLying) {
        case 1:
            Em_EndStatus2(em);
            break;
        }
        em->rotY = em->hitAngle + 0x8000;
        Player_ClearVelocity(em);
        em->velX = 0.0f;
        em->velY = 17.0f;
        em->velZ = -20.0f;
        em->accelY = -2.38f;
        em->accelZ = 0.11f;
        em->stance = EM_STANCE_AIR;
        Em_RestartScript(em);
        work->accelAngX = 0;
        work->accelAngZ = 0;
        break;
    case 1:
        work->accelAngY = em->rotY;
        Em_MoveRotatedAccel(em, &work->accelAngX);
        if (!((em->velZ * em->accelZ) < 0.0f)) {
            em->accelZ = 0.0f;
        }
        if (em->posY <= em->floorY) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            EmAnteka_SetMotion(em, ANTEKA_MOT_BLOWN_BACK_LAND, 0, 0);
            em->timer = 0x5A;
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            EmAnteka_SetMotion(em, ANTEKA_MOT_GET_UP, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->blind = 0;
            EmAnteka_EndState(em);
        }
        break;
    }
}

/*
 * ANTEKA_REACT_STATUS4: begins status 4 (hit-react mode EM_HIT_REACT_STATUS
 * every frame) with an effect at bone 2 each frame, until the status timer
 * (Em_SetStatus4Timer) runs out; then ANTEKA_IDLE_STATUS4_END.
 */
void EmAnteka_Status4(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_STATUS);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, ANTEKA_MOT_STATUS4, 0, 0);
        Em_BeginStatus4(em);
        em->noReact = 1;
        Em_RestartScript(em);
        break;
    case 1:
        Em_SpawnStatus4Effect(em, 2);
        if (--em->timer <= 0) {
            em->step++;
            EmAnteka_ChangeState(em, EM_GROUP_IDLE, ANTEKA_IDLE_STATUS4_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/* ANTEKA_REACT_COLLAPSE: collapses, gets up, then EmAnteka_EndState. */
void EmAnteka_CollapseAndGetUp(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, ANTEKA_MOT_COLLAPSE, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_SetMotion(em, ANTEKA_MOT_GET_UP, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
}

/*
 * Runs the group 4 state; ANTEKA_REACT_BLOWN_BACK_LYING runs
 * EmAnteka_BlownBack with 1.
 */
void EmAnteka_RunReactState(Em *em, EmWork *work)
{
    switch (em->state) {
    case ANTEKA_REACT_FLINCH_A:
        EmAnteka_FlinchA(em);
        break;
    case ANTEKA_REACT_FLINCH_B:
        EmAnteka_FlinchB(em);
        break;
    case ANTEKA_REACT_BLOWN_BACK:
        EmAnteka_BlownBack(em, work, 0);
        break;
    case ANTEKA_REACT_STATUS4:
        EmAnteka_Status4(em);
        break;
    case ANTEKA_REACT_COLLAPSE:
        EmAnteka_CollapseAndGetUp(em);
        break;
    case ANTEKA_REACT_BLOWN_BACK_LYING:
        EmAnteka_BlownBack(em, work, 1);
        break;
    }
}

/*
 * ANTEKA_DEFEAT_DIE, the death: unhittable (10 frames, refreshed); thrown
 * back (QuestEm_Hunted), lands, leaves a carcass mine point (EmMine_Create)
 * for 2400 frames or until it is used up, fades over 60 frames and hides,
 * then ANTEKA_DEFEAT_REAPPEAR. The body is kept on the floor every frame.
 */
void EmAnteka_Die(Em *em, EmWork *work)
{
    em->noHitTimer = 0xA;
    em->noPushTimer = 0xA;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, ANTEKA_MOT_BLOWN_BACK, 0, 0);
        em->rotY = em->hitAngle + 0x8000;
        em->motionSpeed = 1.0f;
        Player_ClearVelocity(em);
        em->velX = 0.0f;
        em->velY = 17.0f;
        em->velZ = -23.0f;
        em->accelY = -3.15f;
        em->accelZ = 0.08f;
        em->stance = EM_STANCE_AIR;
        QuestEm_Hunted(em);
        work->accelAngX = 0;
        work->accelAngZ = 0;
        break;
    case 1:
        work->accelAngY = em->rotY;
        Em_MoveRotatedAccel(em, &work->accelAngX);
        if (!((em->velZ * em->accelZ) < 0.0f)) {
            em->accelZ = 0.0f;
        }
        if (em->posY <= em->floorY) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            EmAnteka_SetMotion(em, ANTEKA_MOT_BLOWN_BACK_LAND, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 2400;
            em->motionSpeed = 0.0f;
            EmMine_Create(em, 0);
            em->fieldCollide = 0;
        }
        break;
    case 3:
        if (--em->timer <= 0 || EmMine_Update(em) <= 0) {
            em->step++;
            EmMine_Remove(em);
        }
        break;
    case 4:
        em->alpha -= 1.0f / 60.0f;
        if (em->alpha <= 0.0f) {
            em->visible = 0;
            EmAnteka_ChangeState(em, EM_GROUP_DEFEAT, ANTEKA_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
    if (em->posY < em->floorY) {
        em->posY = em->floorY;
    }
}

/*
 * ANTEKA_DEFEAT_REAPPEAR: back to ANTEKA_IDLE when QuestEm_CanAppear allows
 * (EmAnteka_ChangeState called QuestEm_Appear), else ends the task and
 * hides the monster.
 */
void EmAnteka_Reappear(Em *em)
{
    switch (em->step) {
    case 0:
        if (QuestEm_CanAppear(em) == 1) {
            EmAnteka_ChangeState(em, EM_GROUP_IDLE, ANTEKA_IDLE, EM_SYNC_LOCAL);
            break;
        }
        em->taskPhase++;
        em->visible = 0;
        break;
    }
}

/*
 * ANTEKA_DEFEAT_LEAVE: unhittable, fades over 100 frames, then
 * QuestEm_Leave ends the task and hides the monster.
 */
void EmAnteka_FadeAndLeave(Em *em)
{
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 0x64;
        break;
    case 1:
        em->timer -= 1;
        em->alpha = (f32) em->timer / 100.0f;
        if (em->timer <= 0) {
            QuestEm_Leave(em);
            em->taskPhase++;
            em->visible = 0;
        }
        break;
    }
}

/* Runs the group 5 state. */
void EmAnteka_RunDefeatState(Em *em, EmWork *work)
{
    switch (em->state) {
    case ANTEKA_DEFEAT_DIE:
        EmAnteka_Die(em, work);
        break;
    case ANTEKA_DEFEAT_REAPPEAR:
        EmAnteka_Reappear(em);
        break;
    case ANTEKA_DEFEAT_LEAVE:
        EmAnteka_FadeAndLeave(em);
        break;
    }
}

/*
 * ANTEKA_EVENT_LEAVE: fades over 60 frames, then QuestEm_Leave ends the
 * task and hides the monster.
 */
void EmAnteka_EventFadeAndLeave(Em *em)
{
    f32 alpha;

    switch (em->step) {
    case 0:
        em->step++;
        break;
    case 1:
        alpha = em->alpha - 0.016666668f;
        em->alpha = alpha;
        if (alpha <= 0.0f) {
            QuestEm_Leave(em);
            em->taskPhase++;
            em->visible = 0;
        }
        break;
    }
}

/*
 * ANTEKA_EVENT_SCENE: unhittable and not sensing; placed at a fixed spot
 * facing rotY 0x8000, plays three motions in a row, then EmAnteka_EndState.
 */
void EmAnteka_EventScene(Em *em)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        em->posX = 14419.0f;
        em->posY = 0.0f;
        em->posZ = 15007.0f;
        em->rotX = 0;
        em->rotY = 0x8000;
        em->rotZ = 0;
        Em_SetMotion(em, ANTEKA_MOT_IDLE_ACTION_A, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, ANTEKA_MOT_IDLE_ACTION_B, 6, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 0x96;
            Em_SetMotion(em, ANTEKA_MOT_CHARGE, 6, 0);
        }
        break;
    case 3:
        em->step++;
        EmAnteka_EndState(em);
        break;
    }
}

/*
 * ANTEKA_EVENT_IDLE: unhittable and not sensing; plays the idle motion,
 * then EmAnteka_EndState.
 */
void EmAnteka_EventIdle(Em *em)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->motionSpeed = 1.0f;
        Em_SetMotion(em, ANTEKA_MOT_IDLE, 0, 0);
        break;
    case 1:
        EmAnteka_EndState(em);
        break;
    }
}

/* Runs the group 6 state (EmAnteka_RunState also sends group 7 here). */
void EmAnteka_RunEventState(Em *em)
{
    switch (em->state) {
    case ANTEKA_EVENT_LEAVE:
        EmAnteka_EventFadeAndLeave(em);
        break;
    case ANTEKA_EVENT_SCENE:
        EmAnteka_EventScene(em);
        break;
    case ANTEKA_EVENT_IDLE:
        EmAnteka_EventIdle(em);
        break;
    }
}

/*
 * Runs the state group's dispatcher: group 2 runs group 0's, group 7 group
 * 6's, group 8 nothing. The work area is passed on as an EmWork.
 */
void EmAnteka_RunState(Em *em)
{
    u8 *w = &em->work;

    switch (em->stateGroup) {
    case EM_GROUP_IDLE:
        EmAnteka_RunIdleState(em, (EmWork *) w);
        break;
    case EM_GROUP_MOVE:
        EmAnteka_RunMoveState(em, (EmWork *) w);
        break;
    case EM_GROUP_FLY:
        EmAnteka_RunIdleState(em, (EmWork *) w);
        break;
    case EM_GROUP_ATTACK:
        EmAnteka_RunAttackState(em, (EmWork *) w);
        break;
    case EM_GROUP_REACT:
        EmAnteka_RunReactState(em, (EmWork *) w);
        break;
    case EM_GROUP_DEFEAT:
        EmAnteka_RunDefeatState(em, (EmWork *) w);
        break;
    case EM_GROUP_EVENT:
        EmAnteka_RunEventState(em);
        break;
    case EM_GROUP_EVENT2:
        EmAnteka_RunEventState(em);
        
    default:
        break;
    }
}

/*
 * Changes the state (Em_SetStateSync). On the machine that runs the monster
 * it first targets a player in range (Em_TargetPlayerInRange); the move
 * states set the stop distance (100 for the run and charge, 300 for their
 * _FAR versions); ANTEKA_DEFEAT_REAPPEAR calls QuestEm_Appear when
 * QuestEm_CanAppear allows. The `goto move` shares the speed call between
 * the case groups (the match).
 */
void EmAnteka_ChangeState(Em *em, u16 group, u16 state, u8 mode)
{
    f32 stopDist;

    if (em->remote == 0) {
        Em_TargetPlayerInRange(em);
    }
    switch (group) {
    case EM_GROUP_MOVE:
        switch (state) {
        case ANTEKA_MOVE_WALK_TURN:
            Em_SetSpeedScaled(em, 0, 1000.0f, 100.0f);
            break;
        case ANTEKA_MOVE_WALK:
        case ANTEKA_MOVE_RUN:
        case ANTEKA_MOVE_CHARGE:
            stopDist = 100.0f;
            goto move;
        case ANTEKA_MOVE_RUN_FAR:
        case ANTEKA_MOVE_CHARGE_FAR:
            stopDist = 300.0f;
        move:
            Em_SetSpeedScaled(em, 0, 1000.0f, stopDist);
            break;
        }
        break;
    case EM_GROUP_DEFEAT:
        if (state == 4 && QuestEm_CanAppear(em) == 1) {
            QuestEm_Appear(em);
        }
        break;
    case EM_GROUP_IDLE:
    case EM_GROUP_FLY:
    case EM_GROUP_ATTACK:
    case EM_GROUP_REACT:
    case EM_GROUP_EVENT:
    case EM_GROUP_EVENT2:
    case EM_GROUP_VERSUS:
        break;
    }
    em->motionSpeed = 1.0f;
    Em_SetStateSync(em, group, state, mode);
}

/*
 * Ends the state: motion speed 1, on the ground, not looking at the target,
 * the script pending, then ANTEKA_IDLE.
 */
void EmAnteka_EndState(Em *em)
{
    em->motionSpeed = 1.0f;
    em->stance = EM_STANCE_GROUND;
    em->lookAtTarget = 0;
    em->scriptPending = 1;
    EmAnteka_ChangeState(em, EM_GROUP_IDLE, ANTEKA_IDLE, EM_SYNC_LOCAL);
}

/*
 * Plays a motion on layer 0 (Em_SetMotion); for a Kelbi with Em.gender
 * also starts the same motion on layer 1 (Em_SetLayerMotion) for the seven
 * motions listed.
 */
void EmAnteka_SetMotion(Em *em, s32 motion, s32 blend, s32 frame)
{
    s32 layerMotion;

    Em_SetMotion(em, motion, blend, frame);
    if (em->gender != 0) {
        switch (em->motionId[0]) {
        case EM_MOTION_ID(0, ANTEKA_MOT_IDLE):
            layerMotion = EM_MOTION_ID(1, ANTEKA_MOT_IDLE);
            break;
        case EM_MOTION_ID(0, ANTEKA_MOT_IDLE_ACTION_A):
            layerMotion = EM_MOTION_ID(1, ANTEKA_MOT_IDLE_ACTION_A);
            break;
        case EM_MOTION_ID(0, ANTEKA_MOT_IDLE_ACTION_B):
            layerMotion = EM_MOTION_ID(1, ANTEKA_MOT_IDLE_ACTION_B);
            break;
        case EM_MOTION_ID(0, ANTEKA_MOT_IDLE_ACTION_D):
            layerMotion = EM_MOTION_ID(1, ANTEKA_MOT_IDLE_ACTION_D);
            break;
        case EM_MOTION_ID(0, ANTEKA_MOT_WALK):
            layerMotion = EM_MOTION_ID(1, ANTEKA_MOT_WALK);
            break;
        case EM_MOTION_ID(0, ANTEKA_MOT_RUN):
            layerMotion = EM_MOTION_ID(1, ANTEKA_MOT_RUN);
            break;
        case EM_MOTION_ID(0, ANTEKA_MOT_CHARGE):
            layerMotion = EM_MOTION_ID(1, ANTEKA_MOT_CHARGE);
            break;
        default:
            layerMotion = 0;
            break;
        }
        if (layerMotion != 0) {
            Em_SetLayerMotion(em, layerMotion, blend, frame, 1);
        }
    }
}

void EmAnteka_SpawnEffect01(u8 *em)
{
    Effect01_Spawn((Player *) em, 0);
}

void EmAnteka_TableNop(void)
{
}

void EmAnteka_MotionEffectsDefault(Em *em)
{
}

/*
 * Runs the motion sounds (EmAnteka_MotionSounds) and, by the layer-0 motion
 * kept in work->fxMotion, the dust and impact effects at the footfall and
 * impact frames and the contact attacks at the strike frames. The kind
 * tests are always true (only EM_KIND_KELBI and EM_KIND_ANTEKA reach it), and the
 * first test is redundant: both kept for the match.
 */
void EmAnteka_MotionEffects(Em *em, EmWork *work)
{
    f32 boneOffset[3];

    if (em->motionId[0] != work->fxMotion) {
        work->fxMotion = em->motionId[0];
    }
    EmAnteka_MotionSounds(em, work);
    switch (work->fxMotion) {
    case EM_MOTION_ID(0, ANTEKA_MOT_IDLE):
    case EM_MOTION_ID(0, ANTEKA_MOT_IDLE_ACTION_A):
    case EM_MOTION_ID(0, ANTEKA_MOT_IDLE_ACTION_B):
    case EM_MOTION_ID(0, ANTEKA_MOT_IDLE_ACTION_C):
    case EM_MOTION_ID(0, ANTEKA_MOT_IDLE_ACTION_D):
    case EM_MOTION_ID(0, ANTEKA_MOT_IDLE_ACTION_E):
    case EM_MOTION_ID(0, ANTEKA_MOT_WALK):
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_RUN):
        if (EmMotion_PassedFrameScaled(em, 4.0f, 0U) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x15, 3, 0.6f);
        }
        if (EmMotion_PassedFrameScaled(em, 28.0f, 0U) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x11, 3, 0.6f);
        }
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_CHARGE):
        if (EmMotion_PassedFrameScaled(em, 12.0f, 0U) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x15, 3, 0.7f);
        }
        if (EmMotion_PassedFrameScaled(em, 18.0f, 0U) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x11, 3, 0.7f);
        }
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_MOVE_ACTION_A):
    case EM_MOTION_ID(0, ANTEKA_MOT_MOVE_ACTION_B):
        if (work->fxMotion == 0x3F7) {
            if (EmMotion_PassedFrameScaled(em, 12.0f, 0U) != 0) {
                Effect0D_SpawnForEmScaled(em, 0x15, 3, 0.9f);
            }
        } else if (EmMotion_PassedFrameScaled(em, 12.0f, 0U) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x11, 3, 0.9f);
        }
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_RECOVER):
        if (EmMotion_PassedFrameScaled(em, 12.0f, 0U) != 0) {
            Effect0D_SpawnForEmScaled(em, 0xB, 3, 0.6f);
        }
        if (EmMotion_PassedFrameScaled(em, 16.0f, 0U) != 0) {
            Effect0D_SpawnForEmScaled(em, 0xB, 3, 0.6f);
        }
        if (EmMotion_PassedFrameScaled(em, 12.0f, 0U) != 0) {
            Effect0D_SpawnForEmScaled(em, 7, 3, 0.6f);
        }
        if (EmMotion_PassedFrameScaled(em, 16.0f, 0U) != 0) {
            Effect0D_SpawnForEmScaled(em, 7, 3, 0.6f);
        }
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_TURN_AROUND_RIGHT):
    case EM_MOTION_ID(0, ANTEKA_MOT_TURN_AROUND_LEFT):
        if (work->fxMotion == 0x3F7) {
            if (EmMotion_PassedFrameScaled(em, 34.0f, 0U) != 0) {
                Effect0D_SpawnForEmScaled(em, 0x15, 3, 0.6f);
            }
        } else if (EmMotion_PassedFrameScaled(em, 34.0f, 0U) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x11, 3, 0.6f);
        }
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_ATK_COMBO):
        if (em->type == EM_KIND_KELBI || em->type == EM_KIND_ANTEKA) {
            if (EmMotion_PassedFrameScaled(em, 64.0f, 0U) != 0) {
                EmContactAttack_Create(em, 0x11);
            }
            if (EmMotion_PassedFrameScaled(em, 80.0f, 0U) != 0) {
                EmContactAttack_Create(em, 0x12);
            }
            if (EmMotion_PassedFrameScaled(em, 146.0f, 0U) != 0) {
                EmContactAttack_Create(em, 0x13);
            }
        }
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_ATK_SLAM):
        if (EmMotion_PassedFrameScaled(em, 20.0f, 0U) != 0) {
            if (em->type == EM_KIND_KELBI || em->type == EM_KIND_ANTEKA) {
                EmContactAttack_Create(em, 0x15);
            } else {
                EmContactAttack_Create(em, 0xB);
            }
        }
        if (EmMotion_PassedFrameScaled(em, 24.0f, 0U) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x16, 3, 0.9f);
        }
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_TURN_QUICK):
        if (EmMotion_PassedFrameScaled(em, 26.0f, 0U) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x16, 3, 0.9f);
        }
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_LEAP):
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_ATK_STRIKE):
        if (EmMotion_PassedFrameScaled(em, 14.0f, 0U) != 0) {
            if (em->type == EM_KIND_KELBI || em->type == EM_KIND_ANTEKA) {
                EmContactAttack_Create(em, 0x14);
            } else {
                EmContactAttack_Create(em, 0xC);
            }
        }
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_IDLE_ACTION_F):
    case EM_MOTION_ID(0, 0x27):
    case EM_MOTION_ID(0, ANTEKA_MOT_FLINCH_A):
    case EM_MOTION_ID(0, ANTEKA_MOT_FLINCH_B):
    case EM_MOTION_ID(0, ANTEKA_MOT_BLOWN_BACK):
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_BLOWN_BACK_LAND):
        if (EmMotion_PassedFrameScaled(em, 44.0f, 0U) != 0) {
            Effect0D_SpawnForEmScaled(em, 2, 6, 0.5f);
        }
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_GET_UP):
    case EM_MOTION_ID(0, ANTEKA_MOT_COLLAPSE):
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_LIE):
        boneOffset[0] = 0.0f;
        boneOffset[1] = 0.0f;
        boneOffset[2] = 10.0f;
        Em_SpawnLyingPuffs(em, 0x17, boneOffset, 1.0f);
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_STATUS4):
    case EM_MOTION_ID(0, ANTEKA_MOT_STATUS4_END):
        break;
    default:
        EmAnteka_MotionEffectsDefault(em);
        break;
    }
}

/*
 * The family's hook 3 (Em_RunHook3, every frame): the first frame only
 * advances fxStep, later frames run EmAnteka_MotionEffects.
 */
void EmAnteka_UpdateMotionEffects(Em *em)
{
    EmWork *w = (EmWork *)&em->work;

    switch (w->fxStep) {
    case 0:
        w->fxStep++;
        break;
    case 1:
        EmAnteka_MotionEffects(em, w);
        break;
    }
}

/*
 * Sets up a Mosswine or a Bullfango. Outside a quest (gQuest.stage 0) it is
 * placed on a test row by its slot number. A Mosswine gets its hp from
 * Em_ComputeMaxHp; a Bullfango links to its pack's leader (Em_FindLink),
 * takes its hp row from Em.variant (row 0 past 3), and variants 1 and 3 are
 * drawn 2.7 times larger. Then the size (GmEm_ApplySize), its home
 * (EmMosswineWork.home, the position on the ground) and the parts.
 */
void EmMosswine_Init(Em *em)
{
    EmMosswineWork *w = (EmMosswineWork *)&em->work;
    f32 fslot;
    s16 fullHp;
    s16 variantHp;
    s16 baseHp;
    u8 row;
    u8 slotNo;

    if (gQuest.stage == 0) {
        if (gScene.area == AREA_SNOWY_MOUNTAINS_NIGHT_9) {
            em->posX = 9500.0f + (150.0f * (f32) (em->slotNo / 8));
            em->posY = 0.0f;
            em->posZ = 9200.0f + (150.0f * (f32) (em->slotNo & 7));
        } else {
            slotNo = em->slotNo;
            fslot = (f32) (u32) slotNo;
            em->posX = 5000.0f + (200.0f * fslot);
            em->posY = 0.0f;
            em->posZ = 5000.0f;
        }
    }
    em->blind = 0;
    em->remote = 0;
    Em_SetMotionRow(em, MOSS_MOT_IDLE, 0, 0);
    em->stance = EM_STANCE_GROUND;
    if (em->type == EM_KIND_MOSSWINE) {
        fullHp = Em_ComputeMaxHp(em, 0);
        em->hp = fullHp;
        em->hpMax = fullHp;
    } else {
        switch (em->variant) {
        case 1:
        case 3:
            em->scaleX = 2.7f;
            em->scaleY = 2.7f;
            em->scaleZ = 2.7f;
            em->bodyScale = 2.7f;
            break;
        }
        Em_FindLink(em);
        row = em->variant;
        if ((s32) row < 4) {
            variantHp = Em_ComputeMaxHp(em, (s16) row);
            em->hp = variantHp;
            em->hpMax = variantHp;
        } else {
            baseHp = Em_ComputeMaxHp(em, 0);
            em->hp = baseHp;
            em->hpMax = baseHp;
        }
    }
    GmEm_ApplySize(em);
    w->unk00C = 0;
    w->unk00E = 0;
    w->home[0] = em->posX;
    w->home[1] = 0.0f;
    w->home[2] = em->posZ;
    Em_InitParts(em);
}

/* Init, then MOSS_IDLE. */
void EmMosswine_Start(Em *em)
{
    EmMosswine_Init(em);
    EmMosswine_ChangeState(em, EM_GROUP_IDLE, MOSS_IDLE, EM_SYNC_LOCAL);
}

/* Callback slot 5 (EmVt.appear): Init again, the state kept. */
void EmMosswine_Reinit(Em *em)
{
    EmMosswine_Init(em);
}

/*
 * The family's state change (Em_ChangeState dispatches here by kind): the
 * motion speed back to 1, then per state: MOSS_IDLE becomes the kind's
 * alert idle in combat; MOSS_IDLE_STUN_END ends status 5 and blocks
 * reactions; the walks and the run head for Em_ComputeGoal2's goal for at
 * most 1800 frames, MOSS_MOVE_WALK_SCRIPTED from its variant's fixed start
 * to its goal (EmMosswine_VariantParams) for 600; each charge gets its
 * goal, its length (30 to 120 frames) and its motion speed; a Bullfango's
 * MOSS_REACT_STUN begins status 5; MOSS_DEFEAT_REAPPEAR lets the quest
 * place it first (QuestEm_Appear).
 */
void EmMosswine_ChangeState(Em *em, u16 group, u16 state, u8 mode)
{
    f32 *params;

    em->motionSpeed = 1.0f;
    switch (group) {
    case EM_GROUP_IDLE:
        switch (state) {
        case MOSS_IDLE:
            if (em->combat == 1) {
                if (em->type == EM_KIND_MOSSWINE) {
                    state = MOSS_IDLE_ALERT_MOSSWINE;
                } else {
                    state = MOSS_IDLE_ALERT_BULLFANGO;
                }
            }
            break;
        case MOSS_IDLE_STUN_END:
            Em_EndStatus5(em);
            em->noReact = 1;
            break;
        }
        break;
    case EM_GROUP_MOVE:
        switch (state) {
        case MOSS_MOVE_WALK:
        case MOSS_MOVE_RUN:
        case MOSS_MOVE_WALK_B:
            em->timer = 1800;
            Em_ComputeGoal2(em, em->goal);
            break;
        case MOSS_MOVE_WALK_SCRIPTED:
            em->timer = 600;
            params = EmMosswine_VariantParams[em->variant];
            em->posX = *params++;
            em->posY = *params++;
            em->posZ = *params++;
            em->goal[0] = *params++;
            em->goal[1] = *params++;
            em->goal[2] = *params++;
            break;
        }
        break;
    case EM_GROUP_ATTACK:
        Em_ComputeGoal2(em, em->goal);
        switch (state) {
        case MOSS_ATK_CHARGE_A:
            em->timer = 90;
            em->motionSpeed = 0.8f;
            break;
        case MOSS_ATK_CHARGE_B:
            em->timer = 90;
            em->motionSpeed = 0.8f;
            break;
        case MOSS_ATK_CHARGE_C:
            em->timer = 90;
            em->motionSpeed = 1.0f;
            break;
        case MOSS_ATK_CHARGE_D:
            em->timer = 120;
            em->motionSpeed = 1.0f;
            break;
        case MOSS_ATK_CHARGE_E:
            em->timer = 30;
            em->motionSpeed = 0.8f;
            break;
        case MOSS_ATK_CHARGE_F:
            em->timer = 45;
            em->motionSpeed = 0.8f;
            break;
        }
        break;
    case EM_GROUP_REACT:
    case EM_GROUP_EVENT:
    case EM_GROUP_VERSUS:
        switch (state) {
        case MOSS_REACT_STUN:
            if (em->type == EM_KIND_BULLFANGO) {
                Em_BeginStatus5(em);
            }
            break;
        }
        break;
    case EM_GROUP_DEFEAT:
        if (state == MOSS_DEFEAT_REAPPEAR && QuestEm_CanAppear(em) == 1) {
            QuestEm_Appear(em);
        }
        break;
    case EM_GROUP_FLY:
    case EM_GROUP_EVENT2:
        break;
    }
    Em_SetStateSync(em, group, state, mode);
}

/*
 * Ends a state: on the ground, and MOSS_IDLE with the script asked for the
 * next state.
 */
void EmMosswine_EndState(Em *em)
{
    em->stance = EM_STANCE_GROUND;
    em->scriptPending = 1;
    EmMosswine_ChangeState(em, EM_GROUP_IDLE, MOSS_IDLE, EM_SYNC_LOCAL);
}

/* MOSS_IDLE_NONE: nothing. */
void EmMosswine_IdleNone(void)
{
}

/* MOSS_IDLE: the idle motion once, then EmMosswine_EndState. */
void EmMosswine_Idle(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, MOSS_MOT_IDLE, 0xA, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmMosswine_EndState(em);
        }
        break;
    }
}

/*
 * MOSS_IDLE_ACTION_A: MOSS_MOT_IDLE_ACTION_A for 30 to 89 frames (by
 * Em.stateRand), its end motion, then EmMosswine_EndState.
 */
void EmMosswine_IdleActionA(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->timer = ((s32) em->stateRand % 60) + 0x1E;
        Em_SetMotion(em, MOSS_MOT_IDLE_ACTION_A, 6, 0);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, MOSS_MOT_IDLE_ACTION_A_END, 6, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmMosswine_EndState(em);
        }
        break;
    }
}

/*
 * MOSS_IDLE_ACTION_B: MOSS_MOT_IDLE_ACTION_B for 240 frames of motion time,
 * then EmMosswine_EndState.
 */
void EmMosswine_IdleActionB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, MOSS_MOT_IDLE_ACTION_B, 6, 0);
        Motion_InitTime(em, 0);
        break;
    case 1:
        if (em->motionTime >= 0xF0) {
            EmMosswine_EndState(em);
        }
        break;
    }
}

/*
 * MOSS_IDLE_ACTION_C, _TWICE and _4: MOSS_MOT_IDLE_ACTION_C `repeats`
 * times, turning toward the goal, then EmMosswine_EndState.
 */
void EmMosswine_IdleActionC(Em *em, s32 repeats)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->timer = repeats;
        Em_SetMotion(em, MOSS_MOT_IDLE_ACTION_C, 6, 0);
        em->targetAngle = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        break;
    case 1:
        Angle_TurnToward((s32 *) &em->rotY, (s32 *) &em->targetAngle, 0x200);
        Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
        if (em->motion[0].playing == 0) {
            if (--em->timer <= 0) {
                EmMosswine_EndState(em);
            }
        }
        break;
    }
}

/* MOSS_IDLE_ACTION_D: one motion, then EmMosswine_EndState. */
void EmMosswine_IdleActionD(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, MOSS_MOT_IDLE_ACTION_D, 6, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmMosswine_EndState(em);
        }
        break;
    }
}

/*
 * MOSS_IDLE_ALERT_BULLFANGO, the Bullfango's combat idle: one motion, then
 * EmMosswine_EndState.
 */
void EmMosswine_IdleAlertBullfango(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, MOSS_MOT_ALERT_BULLFANGO, 6, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmMosswine_EndState(em);
        }
        break;
    }
}

/*
 * MOSS_IDLE_STATUS2: blinded and waking on a hit (EM_HIT_REACT_WAKE)
 * throughout; lies down (Em_BeginStatus2) with a periodic effect over it
 * until the status timer runs out, then MOSS_IDLE_STATUS2_END.
 */
void EmMosswine_Status2Lie(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, MOSS_MOT_LIE, 0, 0);
        Em_BeginStatus2(em);
        break;
    case 1:
        Em_SpawnLyingPuffs(em, 0xB, (f32 *) &EmMosswine_Effect0BOfs, 1.0f);
        if (--em->timer <= 0) {
            EmMosswine_ChangeState(em, EM_GROUP_IDLE, MOSS_IDLE_STATUS2_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * MOSS_IDLE_STATUS2_END: ends status 2 and gets up, blinded until the
 * motion ends, then MOSS_IDLE_RECOVER.
 */
void EmMosswine_Status2End(Em *em)
{
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_EndStatus2(em);
        Em_SetMotion(em, MOSS_MOT_STATUS2_GET_UP, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->blind = 0;
            EmMosswine_ChangeState(em, EM_GROUP_IDLE, MOSS_IDLE_RECOVER, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/* MOSS_IDLE_RECOVER: one motion after status 2, then EmMosswine_EndState. */
void EmMosswine_Recover(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, MOSS_MOT_RECOVER, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmMosswine_EndState(em);
        }
        break;
    }
}

/*
 * MOSS_IDLE_ALERT_MOSSWINE, the Mosswine's combat idle: one motion, then
 * EmMosswine_EndState.
 */
void EmMosswine_IdleAlertMosswine(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, MOSS_MOT_ALERT_MOSSWINE, 6, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmMosswine_EndState(em);
        }
        break;
    }
}

/*
 * MOSS_IDLE_STUN_END: guarded, gets up from the stun turning back round,
 * then EmMosswine_EndState.
 */
void EmMosswine_StunEnd(Em *em)
{
    em->guardTimer = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, MOSS_MOT_DOWN_GET_UP, 0, 0);
        em->noReact = 1;
        em->rotY = em->rotY + 0x7FFF + 1;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmMosswine_EndState(em);
        }
        break;
    }
}

/*
 * Runs the state of group 0, EM_GROUP_IDLE (and of group 2, which the
 * family doesn't use).
 */
void EmMosswine_RunIdleState(Em *em)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->state) {
    case MOSS_IDLE_NONE:
        EmMosswine_IdleNone();
        break;
    case MOSS_IDLE:
        EmMosswine_Idle(em);
        break;
    case MOSS_IDLE_ACTION_A:
        EmMosswine_IdleActionA(em);
        break;
    case MOSS_IDLE_ACTION_B:
        EmMosswine_IdleActionB(em);
        break;
    case MOSS_IDLE_ACTION_C:
        EmMosswine_IdleActionC(em, 1);
        break;
    case MOSS_IDLE_ACTION_D:
        EmMosswine_IdleActionD(em);
        break;
    case MOSS_IDLE_ALERT_BULLFANGO:
        EmMosswine_IdleAlertBullfango(em);
        break;
    case MOSS_IDLE_ACTION_C_TWICE:
        EmMosswine_IdleActionC(em, 2);
        break;
    case MOSS_IDLE_ACTION_C_4:
        EmMosswine_IdleActionC(em, 4);
        break;
    case MOSS_IDLE_RECOVER:
        EmMosswine_Recover(em);
        break;
    case MOSS_IDLE_ALERT_MOSSWINE:
        EmMosswine_IdleAlertMosswine(em);
        break;
    case MOSS_IDLE_STATUS2:
        EmMosswine_Status2Lie(em);
        break;
    case MOSS_IDLE_STATUS2_END:
        EmMosswine_Status2End(em);
        break;
    case MOSS_IDLE_STUN_END:
        EmMosswine_StunEnd(em);
        
    default:
        break;
    }
}

/*
 * MOSS_MOVE_TURN (quarterTurn 0) aims at the target with
 * EmMosswine_TurnParams[0]; MOSS_MOVE_TURN_QUARTER (quarterTurn 1) aims a
 * quarter turn to the right (raising rotY) with that side's turn motion.
 * Both then turn by Em_TurnStep with EmMosswine_TurnParams[0], and end with
 * EmMosswine_EndState.
 */
void EmMosswine_Turn(Em *em, s32 quarterTurn)
{
    u16 turnDiff;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        if (quarterTurn == 0) {
            Em_StartTurnToTarget(em, &EmMosswine_TurnParams[0]);
        } else {
            em->aimAngle = em->rotY + 0x4000;
            turnDiff = em->aimAngle - em->rotY;
            if (turnDiff <= 0x1000U || turnDiff >= 0xF000U) {
                Player_SetFlag((Player *) em, EM_FLAG_NO_ROOT_MOTION);
                Em_SetMotion(em, MOSS_MOT_WALK, 0, 0);
            } else if (turnDiff >= 0x8000U) {
                Em_SetMotion(em, MOSS_MOT_TURN_LEFT, 0, 0);
            } else {
                Em_SetMotion(em, MOSS_MOT_TURN_RIGHT, 0, 0);
            }
        }
        break;
    case 1:
        if (Em_TurnStep(em, &EmMosswine_TurnParams[0]) == 1) {
            Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
            EmMosswine_EndState(em);
        }
        break;
    }
}

/*
 * MOSS_MOVE_WALK: walks toward the goal, turning to it, until within 200 or
 * the time is up, then EmMosswine_EndState.
 */
void EmMosswine_Walk(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, MOSS_MOT_WALK, 6, 0);
        break;
    case 1:
        if (Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) < 200.0f || --em->timer <= 0) {
            EmMosswine_EndState(em);
            break;
        }
        em->targetAngle = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        Angle_TurnToward((s32 *) &em->rotY, (s32 *) &em->targetAngle, 0x200);
        Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
        break;
    }
}

/* MOSS_MOVE_RUN: as EmMosswine_Walk, running, until within 100. */
void EmMosswine_Run(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, MOSS_MOT_RUN, 6, 0);
        break;
    case 1:
        if (Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) < 100.0f || --em->timer <= 0) {
            EmMosswine_EndState(em);
            break;
        }
        em->targetAngle = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        Angle_TurnToward((s32 *) &em->rotY, (s32 *) &em->targetAngle, 0x200);
        Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
        break;
    }
}

/* MOSS_MOVE_WALK_B: as EmMosswine_Walk with MOSS_MOT_WALK_B. */
void EmMosswine_WalkB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, MOSS_MOT_WALK_B, 6, 0);
        break;
    case 1:
        if (Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) < 200.0f || --em->timer <= 0) {
            EmMosswine_EndState(em);
            break;
        }
        em->targetAngle = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        Angle_TurnToward((s32 *) &em->rotY, (s32 *) &em->targetAngle, 0x200);
        Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
        break;
    }
}

/*
 * MOSS_MOVE_WALK_SCRIPTED: as EmMosswine_Walk, facing the goal at once;
 * restarts the script at the end.
 */
void EmMosswine_WalkScripted(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, MOSS_MOT_WALK, 0, 0);
        em->rotY = em->targetAngle = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        break;
    case 1:
        if (Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) < 200.0f || --em->timer <= 0) {
            Em_RestartScript(em);
            EmMosswine_EndState(em);
            break;
        }
        Angle_TurnToward((s32 *) &em->rotY, (s32 *) &em->targetAngle, 0x200);
        Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
        break;
    }
}

/*
 * MOSS_MOVE_WANDER_HOME: walks for 120 to 247 frames (by Em.stateRand),
 * turning slowly toward its home; a remote copy only follows. Then
 * EmMosswine_EndState.
 */
void EmMosswine_WanderHome(Em *em)
{
    EmMosswineWork *w = (EmMosswineWork *)&em->work;
    s32 timer;

    switch (em->step) {
    case 0:
        em->step++;
        em->timer = (em->stateRand & 0x7F) + 0x78;
        Em_SetMotion(em, MOSS_MOT_WALK, 6, 0);
        break;
    case 1:
        timer = em->timer;
        if (timer > 0) {
            em->timer = timer - 1;
        }
        if ((em->remote == 0) && (em->timer <= 0)) {
            EmMosswine_EndState(em);
            break;
        }
        em->targetAngle = Math_AngleXZ(&em->posX, w->home) & 0xFFFF;
        Angle_TurnToward((s32 *) &em->rotY, (s32 *) &em->targetAngle, 0x80);
        Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
        break;
    }
}

/* Runs the state of group 1, EM_GROUP_MOVE. */
void EmMosswine_RunMoveState(Em *em)
{
    switch (em->state) {
    case MOSS_MOVE_TURN:
        EmMosswine_Turn(em, 0);
        break;
    case MOSS_MOVE_WALK:
        EmMosswine_Walk(em);
        break;
    case MOSS_MOVE_RUN:
        EmMosswine_Run(em);
        break;
    case MOSS_MOVE_WALK_B:
        EmMosswine_WalkB(em);
        break;
    case MOSS_MOVE_WALK_SCRIPTED:
        EmMosswine_WalkScripted(em);
        break;
    case MOSS_MOVE_WANDER_HOME:
        EmMosswine_WanderHome(em);
        break;
    case MOSS_MOVE_TURN_QUARTER:
        EmMosswine_Turn(em, 1);
        
    default:
        break;
    }
}

/*
 * The charges (MOSS_ATK_CHARGE_A to _F, by `charge`): the contact attack of
 * the charge (EmContactAttack rows 3, 8, 9, 0xA, 0xE, 0xF), then a run at a
 * goal 100 past the target's position until within 100 of it or the
 * charge's time is up (cut to 15 frames when the target is gone or in
 * another area); the end motion, then EmMosswine_EndState.
 */
void EmMosswine_Charge(Em *em, u32 charge)
{
    f32 ofs[4];
    f32 out[4];
    s32 ang[4];
    f32 mtx[16];

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, MOSS_MOT_CHARGE, 0xA, 0);
        switch (charge) {
        case 0:
            EmContactAttack_Create(em, 3);
            break;
        case 1:
            EmContactAttack_Create(em, 8);
            break;
        case 2:
            EmContactAttack_Create(em, 9);
            break;
        case 3:
            EmContactAttack_Create(em, 0xA);
            break;
        case 4:
            EmContactAttack_Create(em, 0xE);
            break;
        case 5:
            EmContactAttack_Create(em, 0xF);
            break;
        }
        ang[0] = 0;
        ang[1] = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        ang[2] = 0;
        Mtx_FromAnglesXYZ((s32 *) ang, mtx);
        ofs[0] = 0.0f;
        ofs[1] = 0.0f;
        ofs[2] = 100.0f;
        Vec_Rotate(out, ofs, mtx);
        em->goal[0] += out[0];
        em->goal[2] += out[2];
        em->targetAngle = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        break;
    case 1:
        if (Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) < 100.0f || --em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, MOSS_MOT_CHARGE_END, 0xA, 0);
            break;
        }
        Angle_TurnToward((s32 *) &em->rotY, (s32 *) &em->targetAngle, 0x200);
        Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
        if (em->timer > 0xF) {
            if (em->targetNo == -1) {
                em->timer = 15;
            } else if (em->area != ((Player *) &gPlayers[em->targetNo])->area) {
                em->timer = 15;
            }
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmMosswine_EndState(em);
        }
        break;
    }
}

/* Runs the state of group 3, EM_GROUP_ATTACK: the six charges. */
void EmMosswine_RunAttackState(Em *em)
{
    switch (em->state) {
    case MOSS_ATK_CHARGE_A:
        EmMosswine_Charge(em, MOSS_ATK_CHARGE_A);
        break;
    case MOSS_ATK_CHARGE_B:
        EmMosswine_Charge(em, MOSS_ATK_CHARGE_B);
        break;
    case MOSS_ATK_CHARGE_C:
        EmMosswine_Charge(em, MOSS_ATK_CHARGE_C);
        break;
    case MOSS_ATK_CHARGE_D:
        EmMosswine_Charge(em, MOSS_ATK_CHARGE_D);
        break;
    case MOSS_ATK_CHARGE_E:
        EmMosswine_Charge(em, MOSS_ATK_CHARGE_E);
        break;
    case MOSS_ATK_CHARGE_F:
        EmMosswine_Charge(em, MOSS_ATK_CHARGE_F);
        
    default:
        break;
    }
}

/*
 * MOSS_REACT_FLINCH: a flinch motion by the hit's side (the front, side A
 * or side B), the script restarted, then EmMosswine_EndState.
 */
void EmMosswine_Flinch(Em *em)
{
    s32 diff;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        diff = (em->hitAngle - em->rotY) & 0xFFFF;
        if ((diff > 24576) && (diff < 0xA000)) {
            Em_SetMotion(em, MOSS_MOT_FLINCH_FRONT, 0, 0);
        } else if (diff < 0x8000) {
            Em_SetMotion(em, MOSS_MOT_FLINCH_A, 0, 0);
        } else {
            Em_SetMotion(em, MOSS_MOT_FLINCH_B, 0, 0);
        }
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmMosswine_EndState(em);
        }
        break;
    }
}

/*
 * MOSS_REACT_BLOWN_AWAY: the motion by the hit's side; from frame 10 thrown
 * away from the hit (up 8, back 21), lands and lies 150 frames, gets up,
 * then EmMosswine_EndState.
 */
void EmMosswine_BlownAway(Em *em)
{
    f32 ofs[4];
    f32 out[4];
    s32 ang[4];
    f32 mtx[16];

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        ang[0] = (u16) (em->hitAngle - em->rotY);
        if (ang[0] < 0x8000) {
            Em_SetMotion(em, MOSS_MOT_BLOWN_A, 0, 0);
        } else {
            Em_SetMotion(em, MOSS_MOT_BLOWN_B, 0, 0);
        }
        Em_RestartScript(em);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0) {
            em->step++;
            ang[0] = 0;
            ang[1] = em->hitAngle + 0x8000;
            ang[2] = 0;
            Mtx_FromAnglesXYZ((s32 *) ang, mtx);
            ofs[0] = 0.0f;
            ofs[1] = 8.0f;
            ofs[2] = -21.0f;
            Vec_Rotate(out, ofs, mtx);
            Player_ClearAccel2(em);
            em->velX = out[0];
            em->velY = out[1];
            em->velZ = out[2];
            em->accelY = -1.09f;
            em->accelZ = 0.28f;
            em->stance = EM_STANCE_AIR;
        }
        break;
    case 2:
        if (!((em->velZ * em->accelZ) < 0.0f)) {
            em->accelZ = 0.0f;
        }
        if (Player_MoveFall((Player *) em)) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, MOSS_MOT_BLOWN_LAND, 6, 0);
            em->timer = 150;
        }
        break;
    case 3:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, MOSS_MOT_BLOWN_GET_UP, 6, 0);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            EmMosswine_EndState(em);
        }
        break;
    }
}

/* MOSS_REACT_BLOWN_AWAY_QUICK: as EmMosswine_BlownAway, thrown at once. */
void EmMosswine_BlownAwayQuick(Em *em)
{
    f32 ofs[4];
    f32 out[4];
    s32 ang[4];
    f32 mtx[16];

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        ang[0] = (u16) (em->hitAngle - em->rotY);
        if (ang[0] < 0x8000) {
            Em_SetMotion(em, MOSS_MOT_BLOWN_A, 0, 0xA);
        } else {
            Em_SetMotion(em, MOSS_MOT_BLOWN_B, 0, 0xA);
        }
        ang[0] = 0;
        ang[1] = em->hitAngle + 0x8000;
        ang[2] = 0;
        Mtx_FromAnglesXYZ((s32 *) ang, mtx);
        ofs[0] = 0.0f;
        ofs[1] = 8.0f;
        ofs[2] = -21.0f;
        Vec_Rotate(out, ofs, mtx);
        Player_ClearAccel2(em);
        em->velX = out[0];
        em->velY = out[1];
        em->velZ = out[2];
        em->accelY = -1.09f;
        em->accelZ = 0.28f;
        Em_RestartScript(em);
        break;
    case 1:
        if (!((em->velZ * em->accelZ) < 0.0f)) {
            em->accelZ = 0.0f;
        }
        if (Player_MoveFall((Player *) em)) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, MOSS_MOT_BLOWN_LAND, 6, 0);
            em->timer = 150;
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, MOSS_MOT_BLOWN_GET_UP, 6, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            EmMosswine_EndState(em);
        }
        break;
    }
}

/*
 * MOSS_REACT_STATUS4: Em_BeginStatus4, down with the body turned away from
 * the hit and an effect on bone 2 while Em.timer counts down; then gets up
 * turning back (Em_EndStatus4), reactions blocked until then, and
 * EmMosswine_EndState.
 */
void EmMosswine_Status4(Em *em)
{
    s32 timer;

    Em_SetHitReactMode(em, EM_HIT_REACT_STATUS);
    timer = em->timer;
    if (timer > 0) {
        em->timer = timer - 1;
    }
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, MOSS_MOT_DOWN, 0, 0);
        em->rotY = em->hitAngle + 0x7FFF + 1;
        Em_BeginStatus4(em);
        em->noReact = 1;
        Em_RestartScript(em);
        break;
    case 1:
        Em_SpawnStatus4Effect(em, 2);
        if (em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, MOSS_MOT_DOWN_GET_UP, 0, 0);
            em->rotY = em->rotY + 0x7FFF + 1;
            Em_EndStatus4(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->noReact = 0;
            EmMosswine_EndState(em);
        }
        break;
    }
}

/*
 * MOSS_REACT_DOWN: as EmMosswine_Status4 without the status: down while
 * Em.timer counts, gets up, EmMosswine_EndState.
 */
void EmMosswine_Down(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, MOSS_MOT_DOWN, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, MOSS_MOT_DOWN_GET_UP, 0, 0);
            em->rotY = em->rotY + 0x7FFF + 1;
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmMosswine_EndState(em);
        }
        break;
    }
}

/*
 * MOSS_REACT_STUN (status 5, the Bullfango only): down with the stun's
 * stars (StageObj_Stun, for the low half of Em.timer frames) until Em.timer
 * runs out, then MOSS_IDLE_STUN_END.
 */
void EmMosswine_Stun(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, MOSS_MOT_DOWN, 0, 0);
        Em_RestartScript(em);
        em->noReact = 1;
        StageObj_Stun_CreateEm(em, 1, 0, *(s16 *)&em->timer, 100.0f, 1.2f);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmMosswine_ChangeState(em, EM_GROUP_IDLE, MOSS_IDLE_STUN_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmMosswine_ChangeState(em, EM_GROUP_IDLE, MOSS_IDLE_STUN_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/* Runs the state of group 4, EM_GROUP_REACT. */
void EmMosswine_RunReactState(Em *em)
{
    switch (em->state) {
    case MOSS_REACT_FLINCH:
        EmMosswine_Flinch(em);
        break;
    case MOSS_REACT_BLOWN_AWAY:
        EmMosswine_BlownAway(em);
        break;
    case MOSS_REACT_BLOWN_AWAY_QUICK:
        EmMosswine_BlownAwayQuick(em);
        break;
    case MOSS_REACT_STATUS4:
        EmMosswine_Status4(em);
        break;
    case MOSS_REACT_DOWN:
        EmMosswine_Down(em);
        break;
    case MOSS_REACT_STUN:
        EmMosswine_Stun(em);
        
    default:
        break;
    }
}

/*
 * MOSS_DEFEAT_DIE: unhittable; hunted (QuestEm_Hunted), the motion by the
 * hit's side, thrown away from frame 10, lands, dies and lies as a carcass
 * with a carve point (EmMine) for 2400 frames (a Mosswine) or 900 (a
 * Bullfango) or until it is carved out, fades out over 60 frames, then
 * MOSS_DEFEAT_REAPPEAR.
 */
void EmMosswine_Die(Em *em)
{
    f32 ofs[4];
    f32 out[4];
    s32 ang[4];
    f32 mtx[16];

    em->noHitTimer = 0xA;
    em->noPushTimer = 0xA;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        ang[0] = (u16) (em->hitAngle - em->rotY);
        if (ang[0] < 0x8000) {
            Em_SetMotion(em, MOSS_MOT_BLOWN_A, 0, 0);
        } else {
            Em_SetMotion(em, MOSS_MOT_BLOWN_B, 0, 0);
        }
        QuestEm_Hunted(em);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0) {
            em->step++;
            ang[0] = 0;
            ang[1] = em->hitAngle + 0x8000;
            ang[2] = 0;
            Mtx_FromAnglesXYZ((s32 *) ang, mtx);
            ofs[0] = 0.0f;
            ofs[1] = 0.0f;
            ofs[2] = -19.0f;
            Vec_Rotate(out, ofs, mtx);
            Player_ClearAccel2(em);
            em->velY = 20.0f;
            em->velX = out[0];
            em->velZ = out[2];
            ofs[2] = 0.4f;
            Vec_Rotate(out, ofs, mtx);
            em->accelY = -2.08f;
            em->accelX = out[0];
            em->accelZ = out[2];
            em->stance = EM_STANCE_AIR;
        }
        break;
    case 2:
        if (!((em->velZ * em->accelZ) < 0.0f)) {
            em->accelZ = 0.0f;
        }
        if (!((em->velX * em->accelX) < 0.0f)) {
            em->accelX = 0.0f;
        }
        if (Player_MoveFall((Player *) em)) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, MOSS_MOT_BLOWN_LAND, 6, 0);
            em->timer = 150;
            Player_ClearVelocity(em);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, MOSS_MOT_DIE, 0, 0);
            em->timer = 60;
            EmMine_Create(em, 0);
            em->fieldCollide = 0;
        }
        break;
    case 4:
        EmMine_Update(em);
        if (em->motion[0].playing == 0) {
            em->step++;
            if (em->type == EM_KIND_MOSSWINE) {
                em->timer = 2400;
            } else {
                em->timer = 900;
            }
        }
        break;
    case 5:
        if (--em->timer <= 0 || EmMine_Update(em) <= 0) {
            em->step++;
            EmMine_Remove(em);
        }
        break;
    case 6:
        em->alpha -= 1.0f / 60.0f;
        if (em->alpha <= 0.0f) {
            em->visible = 0;
            EmMosswine_ChangeState(em, EM_GROUP_DEFEAT, MOSS_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/* MOSS_DEFEAT_DIE_QUICK: as EmMosswine_Die, thrown at once. */
void EmMosswine_DieQuick(Em *em)
{
    f32 ofs[4];
    f32 out[4];
    s32 ang[4];
    f32 mtx[16];

    em->noHitTimer = 0xA;
    em->noPushTimer = 0xA;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        ang[0] = (u16) (em->hitAngle - em->rotY);
        if (ang[0] < 0x8000) {
            Em_SetMotion(em, MOSS_MOT_BLOWN_A, 0, 0xA);
        } else {
            Em_SetMotion(em, MOSS_MOT_BLOWN_B, 0, 0xA);
        }
        QuestEm_Hunted(em);
        ang[0] = 0;
        ang[1] = em->hitAngle + 0x8000;
        ang[2] = 0;
        Mtx_FromAnglesXYZ((s32 *) ang, mtx);
        ofs[0] = 0.0f;
        ofs[1] = 8.0f;
        ofs[2] = -21.0f;
        Vec_Rotate(out, ofs, mtx);
        Player_ClearAccel2(em);
        em->velX = out[0];
        em->velY = out[1];
        em->velZ = out[2];
        em->accelY = -1.09f;
        em->accelZ = 0.28f;
        break;
    case 1:
        if (!((em->velZ * em->accelZ) < 0.0f)) {
            em->accelZ = 0.0f;
        }
        if (Player_MoveFall((Player *) em)) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, MOSS_MOT_BLOWN_LAND, 6, 0);
            em->timer = 150;
            Player_ClearVelocity(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, MOSS_MOT_DIE, 0, 0);
            em->timer = 60;
            EmMine_Create(em, 0);
            em->fieldCollide = 0;
        }
        break;
    case 3:
        EmMine_Update(em);
        if (em->motion[0].playing == 0) {
            em->step++;
            if (em->type == EM_KIND_MOSSWINE) {
                em->timer = 2400;
            } else {
                em->timer = 900;
            }
        }
        break;
    case 4:
        if (--em->timer <= 0 || EmMine_Update(em) <= 0) {
            em->step++;
            EmMine_Remove(em);
        }
        break;
    case 5:
        em->alpha -= 1.0f / 60.0f;
        if (em->alpha <= 0.0f) {
            em->visible = 0;
            EmMosswine_ChangeState(em, EM_GROUP_DEFEAT, MOSS_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * MOSS_DEFEAT_REAPPEAR: when the quest lets the monster appear again
 * (QuestEm_CanAppear), MOSS_IDLE; refused, it advances its task phase.
 */
void EmMosswine_Reappear(Em *em)
{
    switch (em->step) {
    case 0:
        if (QuestEm_CanAppear(em) == 1) {
            EmMosswine_ChangeState(em, EM_GROUP_IDLE, MOSS_IDLE, EM_SYNC_LOCAL);
            break;
        }
        em->taskPhase++;
    case 1:
        break;
    }
}

/* Runs the state of group 5, EM_GROUP_DEFEAT. */
void EmMosswine_RunDefeatState(Em *em)
{
    switch (em->state) {
    case MOSS_DEFEAT_DIE:
        EmMosswine_Die(em);
        break;
    case MOSS_DEFEAT_DIE_QUICK:
        EmMosswine_DieQuick(em);
        break;
    case MOSS_DEFEAT_REAPPEAR:
        EmMosswine_Reappear(em);
        break;
    }
}

/*
 * MOSS_EVENT_HIDE_A: hidden for one step (the script not asked), then shown
 * and EmMosswine_EndState.
 */
void EmMosswine_EventHideA(Em *em)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, MOSS_MOT_IDLE, 0, 0);
        em->visible = 0;
        em->scriptPending = 0;
        break;
    case 1:
        em->step++;
        em->visible = 1;
        EmMosswine_EndState(em);
        break;
    }
}

/* MOSS_EVENT_HIDE_B: the same as EmMosswine_EventHideA. */
void EmMosswine_EventHideB(Em *em)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, MOSS_MOT_IDLE, 0, 0);
        em->visible = 0;
        em->scriptPending = 0;
        break;
    case 1:
        em->step++;
        em->visible = 1;
        EmMosswine_EndState(em);
        break;
    }
}

/* Runs the state of group 6, EM_GROUP_EVENT. */
void EmMosswine_RunEventState(Em *em)
{
    switch (em->state) {
    case MOSS_EVENT_HIDE_A:
        EmMosswine_EventHideA(em);
        break;
    case MOSS_EVENT_HIDE_B:
        EmMosswine_EventHideB(em);
        break;
    }
}

/*
 * The family's update, run every frame by GmEm_Update: counts
 * EmMosswineWork.unk00C down; then Em_UpdateDamage's result picks the
 * reaction: a death (MOSS_DEFEAT_DIE when in the air, else
 * MOSS_DEFEAT_DIE_QUICK), the blow-aways, status 4 and status 2 unless
 * being blown away, a flinch or blow-away for a part hit, a Bullfango's
 * stun (a Mosswine's ends at once). Then runs the state (twice when one
 * asked for it).
 */
void EmMosswine_Update(Em *em)
{
    EmMosswineWork *w = (EmMosswineWork *)&em->work;
    u8 hitInfo[3];

    if (w->unk00C != 0) {
        (w->unk00C)--;
    }
    switch (Em_UpdateDamage(em, hitInfo) & 0xFF) {
    case EM_DMG_NONE:
    case EM_DMG_ATTACK9:
    case EM_DMG_ATTACK6:
    case EM_DMG_FLASH:
    case EM_DMG_PART8_DOWN:
    case EM_DMG_HIT:
    case EM_DMG_ATTACK5:
    case EM_DMG_ATTACK5_FLINCH:
    case EM_DMG_ATTACK13:
        break;
    case EM_DMG_KILLED:
    case EM_DMG_DEAD:
        if (em->stance == EM_STANCE_AIR) {
            EmMosswine_ChangeState(em, EM_GROUP_DEFEAT, MOSS_DEFEAT_DIE, EM_SYNC_OWNER);
        } else {
            EmMosswine_ChangeState(em, EM_GROUP_DEFEAT, MOSS_DEFEAT_DIE_QUICK, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_STATUS_IN_AIR:
        if ((em->stateGroup != EM_GROUP_REACT || em->state != MOSS_REACT_BLOWN_AWAY) && (em->stateGroup != EM_GROUP_REACT || em->state != MOSS_REACT_BLOWN_AWAY_QUICK)) {
            EmMosswine_ChangeState(em, EM_GROUP_REACT, MOSS_REACT_BLOWN_AWAY_QUICK, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_STATUS4:
        if ((em->stateGroup != EM_GROUP_REACT || em->state != MOSS_REACT_BLOWN_AWAY) && (em->stateGroup != EM_GROUP_REACT || em->state != MOSS_REACT_BLOWN_AWAY_QUICK)) {
            Em_SetStatus4Timer(em);
            EmMosswine_ChangeState(em, EM_GROUP_REACT, MOSS_REACT_STATUS4, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_CAPTURED:
        EmMosswine_ChangeState(em, EM_GROUP_REACT, MOSS_REACT_BLOWN_AWAY_QUICK, EM_SYNC_OWNER);
        break;
    case EM_DMG_STATUS2:
        if ((em->stateGroup != EM_GROUP_REACT || em->state != MOSS_REACT_BLOWN_AWAY) && (em->stateGroup != EM_GROUP_REACT || em->state != MOSS_REACT_BLOWN_AWAY_QUICK)) {
            Em_SetStatus2Timer(em);
            EmMosswine_ChangeState(em, EM_GROUP_IDLE, MOSS_IDLE_STATUS2, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_HIT_LYING:
        em->blind = 0;
        Em_EndStatus2(em);
        if (em->stance == EM_STANCE_AIR) {
            EmMosswine_ChangeState(em, EM_GROUP_REACT, MOSS_REACT_BLOWN_AWAY_QUICK, EM_SYNC_OWNER);
        } else {
            EmMosswine_ChangeState(em, EM_GROUP_REACT, MOSS_REACT_BLOWN_AWAY, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_PART_FLINCH:
        if (em->stance == EM_STANCE_AIR) {
            EmMosswine_ChangeState(em, EM_GROUP_REACT, MOSS_REACT_BLOWN_AWAY_QUICK, EM_SYNC_OWNER);
        } else if ((s16) Player_IsState(em, 4, 0)) {
            EmMosswine_ChangeState(em, EM_GROUP_REACT, MOSS_REACT_BLOWN_AWAY, EM_SYNC_OWNER);
        } else {
            EmMosswine_ChangeState(em, EM_GROUP_REACT, MOSS_REACT_FLINCH, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_HIT_MODE3:
        if (em->stance == EM_STANCE_AIR) {
            EmMosswine_ChangeState(em, EM_GROUP_REACT, MOSS_REACT_BLOWN_AWAY_QUICK, EM_SYNC_OWNER);
        } else {
            EmMosswine_ChangeState(em, EM_GROUP_REACT, MOSS_REACT_BLOWN_AWAY, EM_SYNC_OWNER);
        }
        Em_ResetPartHp(em, 0);
        break;
    case EM_DMG_STATUS5:
        if (em->type == EM_KIND_BULLFANGO) {
            if ((em->stateGroup != EM_GROUP_REACT || em->state != MOSS_REACT_STUN) && (em->stateGroup != EM_GROUP_REACT || em->state != MOSS_REACT_BLOWN_AWAY) &&
                (em->stateGroup != EM_GROUP_REACT || em->state != MOSS_REACT_BLOWN_AWAY_QUICK)) {
                Em_SetStatus5Timer(em);
                EmMosswine_ChangeState(em, EM_GROUP_REACT, MOSS_REACT_STUN, EM_SYNC_OWNER);
            }
        } else {
            Em_EndStatus5(em);
        }
        break;
    }
    Em_UpdatePendingScript(em);
    Em_ClearHitReactMode(em);
    EmMosswine_RunState(em);
    if (em->stateRequest != 0) {
        EmMosswine_RunState(em);
        em->stateRequest = 0;
    }
}

/* Runs the monster's state through its group's dispatcher. */
void EmMosswine_RunState(Em *em)
{
    switch (em->stateGroup) {
    case EM_GROUP_IDLE:
        EmMosswine_RunIdleState(em);
        break;
    case EM_GROUP_MOVE:
        EmMosswine_RunMoveState(em);
        break;
    case EM_GROUP_FLY:
        EmMosswine_RunIdleState(em);
        break;
    case EM_GROUP_ATTACK:
        EmMosswine_RunAttackState(em);
        break;
    case EM_GROUP_REACT:
        EmMosswine_RunReactState(em);
        break;
    case EM_GROUP_DEFEAT:
        EmMosswine_RunDefeatState(em);
        break;
    case EM_GROUP_EVENT:
        EmMosswine_RunEventState(em);
        break;
    case EM_GROUP_EVENT2:
        EmMosswine_RunReactState(em);
        
    default:
        break;
    }
}

/*
 * Callback slots 0 and 1 (EmVt.init, update): makes the monster's effect
 * 01, as the other families do.
 */
void EmMosswine_SpawnEffect01(u8 *em)
{
    Effect01_Spawn((Player *) em, 0);
}

/* Callback slot 2: nothing. */
void EmMosswine_TableNop(void)
{
}

/*
 * Callback slot 4: the script's question about the monster. Query 0: is its
 * leader (Em.link) in its area and in its group 0 state 6. Returns 1 or 0.
 */
s32 EmMosswine_ScriptQuery(Em *em, u8 query)
{
    Em *leader;

    switch (query) {
    case 0:
        leader = em->link;
        if (leader != NULL && em->area == leader->area && (s16) Player_IsState(leader, 0, 6)) {
            return 1;
        }
        return 0;
    }
    return 0;
}

/*
 * Lao-Shan Lung's motion sounds: footsteps, roars and thuds at fixed
 * frames of the layer-0 motion kept in EmLaoshanWork.fxMotion, some over a
 * span of frames. Called every frame by em_laoshan.c's motion effects; the
 * linker puts it here, after the Mosswine family.
 */
void EmLaoshan_MotionSounds(Em *em, EmWork *work)
{
    s16 motion;

    motion = ((EmLaoshanWork *)work)->fxMotion;
    switch (motion) {
    case 0x3E9:
        Em_PlaySeTypeAtBoneInFrames(em, 0x1CC, 0x24C, 0x46, 0x25, 6, 0);
        Em_PlaySeAtBoneVolOnFrame(em, 0xA0, 0x16, 0x30, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0x17, 0x32, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0x13, 0x21, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x122, 0x13, 0x21, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0xD, 3, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xBE, 0xF, 3, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1B8, 0x11, 3, 8);
        break;
    case 0x3EA:
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0x16, 0x30, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x5A, 0xB, 0x17, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x4C, 0x6B, 0x17, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x4C, 3, 0x17, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x32, 0xC, 0xB, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xB2, 8, 0xE, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xA4, 0x69, 0xE, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xA4, 1, 0xE, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x8A, 0xE, 0x13, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x116, 8, 0x1D, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x108, 0x68, 0x1D, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x108, 0, 0x1D, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0xEE, 0x10, 5, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x17A, 8, 8, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x16C, 0x69, 8, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x16C, 1, 8, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x152, 0x13, 0x19, 8);
        break;
    case 0x3EB:
        Em_PlaySeTypeAtBoneInFrames(em, 0x60, 0x1C2, 0x47, 0x25, 6, 0);
        Em_PlaySeAtBoneVolOnFrame(em, 0xDE, 0x16, 0x30, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x178, 0x17, 0x32, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x2A, 8, 0xE, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1C, 0x69, 0xE, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1C, 1, 0xE, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0xE, 0x13, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x9A, 8, 0x17, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x8C, 0x68, 0x17, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x8C, 0, 0x17, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x72, 0xC, 0xB, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xFA, 8, 8, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xEC, 0x69, 8, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xEC, 1, 8, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0xC8, 0x13, 0x19, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x16A, 8, 0x1D, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x15C, 0x68, 0x1D, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x15C, 0, 0x1D, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x136, 0x10, 5, 8);
        break;
    case 0x3EC:
        Em_PlaySeAtBoneVolOnFrame(em, 0xC2, 0x15, 0x30, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x18A, 0x14, 0x32, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x4E, 8, 8, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x40, 0x69, 8, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x40, 1, 8, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x26, 0x13, 0x19, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xC2, 8, 0x1D, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xB4, 0x68, 0x1D, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xB4, 0, 0x1D, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x9A, 0x10, 5, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x138, 8, 0xE, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x12A, 0x69, 0xE, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x12A, 1, 0xE, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x110, 0xE, 0x13, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1A2, 8, 0x17, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x194, 0x68, 0x17, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x194, 0, 0x17, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x17A, 0xC, 0xB, 8);
        break;
    case 0x3ED:
        Em_PlaySeAtBoneVolOnFrame(em, 0xC2, 0x15, 0x30, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x18A, 0x14, 0x32, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x4E, 8, 0xE, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x40, 0x69, 0xE, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x40, 1, 0xE, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x26, 0x13, 0x13, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xC2, 8, 0x17, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xB4, 0x68, 0x17, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xB4, 0, 0x17, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x9A, 0x10, 0xB, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x138, 8, 8, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x12A, 0x69, 8, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x12A, 1, 8, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x110, 0xE, 0x19, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1A2, 8, 0x1D, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x194, 0x68, 0x1D, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x194, 0, 0x1D, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x17A, 0xC, 5, 8);
        break;
    case 0x3EE:
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0x17, 0x23, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0x13, 0x21, 8);
        Em_PlaySeTypeAtBoneInFrames(em, 0x9C, 0xC6, 0x22, 0x25, 8, 0);
        Em_PlaySeAtBoneVolOnFrame(em, 0x42, 0xB, 0x17, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x34, 0x6B, 0x17, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x34, 3, 0x17, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0xA, 0xC, 0xB, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x98, 0x6B, 0xE, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x98, 3, 0xE, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x8C, 0xC, 0x13, 8);
        break;
    case 0x3EF:
        Em_PlaySeTypeAtBoneInFrames(em, 2, 0x1E0, 0x48, 0x25, 9, 0);
        Em_PlaySeAtBoneVolOnFrame(em, 0x3C, 0x1E, 3, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x16E, 0xB, 0x17, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x160, 0x6B, 0x17, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x160, 3, 0x17, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x138, 0xC, 0xB, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1B6, 0x6B, 0xE, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1B6, 3, 0xE, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x186, 0xC, 0x13, 8);
        break;
    case 0x3F0:
        Em_PlaySeTypeAtBoneInFrames(em, 0x108, 0x294, 0x48, 0x25, 9, 0);
        Em_PlaySeTypeAtBoneInFrames(em, 0x108, 0x230, 0x1F, 3, 8, 0);
        Em_PlaySeAtBoneVolOnFrame(em, 0x118, 0x17, 0x23, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0xE, 0x23, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xCC, 0xC, 0x23, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x186, 0x10, 0x13, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x186, 0x12, 0x19, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x5A, 0x17, 0x30, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x10E, 0x16, 0x32, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xFC, 8, 0xE, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xFC, 0x69, 0xE, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xFC, 1, 0xE, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x100, 8, 8, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x100, 0x69, 8, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x100, 1, 8, 6);
        break;
    case 0x3F2:
        Em_PlaySeAtBoneVolOnFrame(em, 0x13C, 0x17, 0x32, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xC8, 0x17, 0x32, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xC4, 0x17, 0x23, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xC4, 0, 0xB, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xD2, 0x18, 0xB, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0xD6, 0x19, 0xB, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xD6, 0x1B, 0x21, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xD6, 8, 0x21, 4);
        Em_PlaySeTypeAtBoneInFrames(em, 0x140, 0x1E0, 0x26, 0x25, 8, 0);
        Em_PlaySeAtBoneVolOnFrame(em, 0x38, 8, 8, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x2A, 0x69, 8, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x2A, 1, 8, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x10, 0xE, 0x19, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x64, 8, 0xE, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x56, 0x69, 0xE, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x56, 1, 0xE, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x3C, 0x13, 0x13, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x8C, 8, 0x1D, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x7E, 0x68, 0x1D, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x7E, 0, 0x1D, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x64, 0xC, 5, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xC2, 8, 0x17, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xB4, 0x68, 0x17, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xB4, 0, 0x17, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x9A, 0x10, 0xB, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x198, 9, 0x17, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x18A, 0x68, 0x17, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x18A, 0, 0x17, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x170, 0x10, 0xB, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1AA, 9, 0xE, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x19C, 0x69, 0xE, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x19C, 1, 0xE, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x182, 0x13, 0x13, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1D2, 9, 8, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1C4, 0x69, 8, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1C4, 1, 8, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1AA, 0xE, 0x19, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1FA, 9, 0x1D, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1EC, 0x68, 0x1D, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1EC, 0, 0x1D, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1D2, 0xC, 5, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x212, 0x69, 0xE, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x212, 1, 0xE, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1F8, 0x13, 0x13, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x228, 0x68, 0x17, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x228, 0, 0x17, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x20E, 0x10, 0xB, 8);
        break;
    case 0x3F4:
        Em_PlaySeTypeAtBoneInFrames(em, 6, 0x168, 0x47, 0x25, 6, 0);
        Em_PlaySeAtBoneVolOnFrame(em, 0x18, 0x13, 3, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x88, 0x14, 0x32, 8);
        break;
    case 0x3F5:
        Em_PlaySeAtBoneVolOnFrame(em, 6, 0x22, 0x25, 8);
        Em_PlaySeTypeAtBoneInFrames(em, 0x8C, 0x12C, 0x23, 0x25, 9, 0);
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0xD, 3, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x112, 0xD, 3, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0x15, 0x32, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x96, 0x17, 0x30, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xA0, 7, 0x30, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xA0, 2, 0x30, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x4A, 8, 0x17, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x3C, 0x68, 0x17, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x3C, 0, 0x17, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x12, 0x13, 0xB, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1FC, 8, 0x17, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1EE, 0x68, 0x17, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1EE, 0, 0x17, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1C6, 0x13, 0xB, 8);
        break;
    case 0x3F6:
        Em_PlaySeAtBoneVolOnFrame(em, 0x8E, 0x17, 0x23, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x88, 0, 0x23, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x8A, 0x68, 0x23, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x88, 0x18, 0x23, 9);
        Em_PlaySeAtBoneVolOnFrame(em, 0x96, 0x19, 0x23, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x96, 8, 0x23, 9);
        Em_PlaySeAtBoneVolOnFrame(em, 0x10, 0x12, 0x21, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xBA, 0xD, 3, 8);
        Em_PlaySeTypeAtBoneInFrames(em, 0xF6, 0x190, 0x4E, 0x25, 8, 0);
        Em_PlaySeAtBoneVolOnFrame(em, 0x54, 8, 0x1D, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x46, 0x68, 0x1D, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x46, 0, 0x1D, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1E, 0xE, 0x19, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xFA, 0xA, 0x1D, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xEC, 0x68, 0x1D, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xEC, 0, 0x1D, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0xC4, 0xE, 0x19, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x44, 0x16, 0x32, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x58, 0x17, 0x30, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xC4, 0x14, 0x30, 8);
        break;
    case 0x3F7:
        Em_PlaySeAtBoneVolOnFrame(em, 0x3A, 0x26, 0x25, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x140, 0x23, 0x25, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0xF, 3, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x122, 0x10, 3, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x78, 0xA, 0x17, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x6A, 0x6B, 0x17, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x6A, 3, 0x17, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x3C, 0xC, 0x13, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x118, 0xA, 0x1D, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x10A, 0x68, 0x1D, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x10A, 0, 0x1D, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0xF6, 0xE, 0x19, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x19A, 0xA, 0x17, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x18C, 0x68, 0x17, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x18C, 0, 0x17, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x172, 0x13, 0xB, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1DC, 8, 0xE, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1DC, 0x69, 0xE, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1DC, 1, 0xE, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1E0, 8, 8, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1E0, 0x69, 8, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1E0, 1, 8, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1FE, 0x11, 3, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x216, 0x18, 0x21, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1FA, 0x17, 0x32, 8);
        break;
    case 0x3F8:
        Em_PlaySeAtBoneVolOnFrame(em, 0xC8, 0x1E, 0x25, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x18, 0xA, 0x1D, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xA, 0x68, 0x1D, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xA, 0, 0x1D, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0xE, 0x13, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xA4, 0xA, 0x17, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x96, 0x68, 0x17, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x96, 0, 0x17, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x90, 0xE, 0x19, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1A, 0x14, 0x32, 8);
        break;
    case 0x3FC:
        Em_PlaySeAtBoneVolOnFrame(em, 0x4E, 9, 8, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x40, 0x69, 8, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x40, 1, 8, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x26, 0xE, 0x19, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xA4, 9, 0xE, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x96, 0x69, 0xE, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x96, 1, 0xE, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x7C, 0x13, 0x13, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x5E, 0x17, 0x32, 8);
        break;
    case 0x3FD:
    case 0x403:
        Em_PlaySeTypeAtBoneInFrames(em, 0x120, 0x18E, 0x4E, 0x25, 6, 0);
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0x16, 0x30, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x140, 0x17, 0x32, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0x13, 0x21, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x122, 0x13, 0x21, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0xD, 3, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xBE, 0xF, 3, 8);
        break;
    case 0x3FE:
        Em_PlaySeAtBoneVolOnFrame(em, 0x4E, 8, 0xE, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x40, 0x69, 0xE, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x40, 1, 0xE, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x26, 0x13, 0x13, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x8A, 8, 8, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x7C, 0x69, 8, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x7C, 1, 8, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x62, 0xE, 0x19, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x30, 0x16, 0x30, 8);
        break;
    case 0x3FF:
        Em_PlaySeAtBoneVolOnFrame(em, 0x48, 8, 0x1D, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x3A, 0x68, 0x1D, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x3A, 0, 0x1D, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x20, 0x10, 5, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x9A, 8, 0x17, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x8C, 0x68, 0x17, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x8C, 0, 0x17, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x72, 0xC, 0xB, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0x16, 0x30, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x2A, 0x17, 0x32, 8);
        break;
    case 0x400:
        Em_PlaySeAtBoneVolOnFrame(em, 0x52, 8, 0x1D, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x44, 0x68, 0x1D, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x44, 0, 0x1D, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x2A, 0x10, 5, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xA4, 8, 0x17, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x96, 0x68, 0x17, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x96, 0, 0x17, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x7C, 0xC, 0xB, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xF8, 8, 8, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xEA, 0x69, 8, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xEA, 1, 8, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0xD0, 0xE, 0x19, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x14E, 8, 0xE, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x140, 0x69, 0xE, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x140, 1, 0xE, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x12A, 0x13, 0x13, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x38, 0x17, 0x32, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x11E, 0x17, 0x32, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x90, 0x16, 0x30, 8);
        break;
    case 0x401:
        Em_PlaySeAtBoneVolOnFrame(em, 0x4A, 8, 0x17, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x3C, 0x68, 0x17, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x3C, 0, 0x17, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x22, 0xC, 0xB, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x9C, 8, 8, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x8E, 0x69, 8, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x8E, 1, 8, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x74, 0xE, 0x19, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xCC, 8, 0xE, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xBE, 0x69, 0xE, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xBE, 1, 0xE, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0xA4, 0x13, 0x13, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x116, 8, 0x1D, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x108, 0x68, 0x1D, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x108, 0, 0x1D, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0xEE, 0x10, 5, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x166, 8, 0x17, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x158, 0x68, 0x17, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x158, 0, 0x17, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x13E, 0xC, 0xB, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x66, 0x17, 0x32, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x154, 0x17, 0x32, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x2C, 0x16, 0x30, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xF4, 0x16, 0x30, 8);
        break;
    case 0x402:
        Em_PlaySeAtBoneVolOnFrame(em, 0x4E, 9, 0xE, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x40, 0x69, 0xE, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x40, 1, 0xE, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x26, 0x13, 0x13, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xA4, 9, 8, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x96, 0x69, 8, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x96, 1, 8, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x7C, 0xE, 0x19, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x5E, 0x17, 0x32, 8);
        break;
    case 0x404:
        Em_PlaySeAtBoneVolOnFrame(em, 0x4E, 8, 8, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x40, 0x69, 8, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x40, 1, 8, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x26, 0xE, 0x19, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x8A, 8, 0xE, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x7C, 0x69, 0xE, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x7C, 1, 0xE, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x62, 0x13, 0x13, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x30, 0x16, 0x30, 8);
        break;
    case 0x405:
        Em_PlaySeAtBoneVolOnFrame(em, 0x48, 8, 0x17, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x3A, 0x68, 0x17, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x3A, 0, 0x17, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x20, 0xC, 0xB, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x9A, 8, 0x1D, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x8C, 0x68, 0x1D, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x8C, 0, 0x1D, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x72, 0x10, 5, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0x16, 0x30, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x2A, 0x17, 0x32, 8);
        break;
    case 0x406:
        Em_PlaySeAtBoneVolOnFrame(em, 0x52, 8, 0x17, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x44, 0x68, 0x17, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x44, 0, 0x17, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x2A, 0xC, 0xB, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xA4, 8, 0x1D, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x96, 0x68, 0x1D, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x96, 0, 0x1D, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x7C, 0x10, 5, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xF8, 8, 0xE, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xEA, 0x69, 0xE, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xEA, 1, 0xE, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0xD0, 0x13, 0x13, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x14E, 8, 8, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x140, 0x69, 8, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x140, 1, 8, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x12A, 0xE, 0x19, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x38, 0x17, 0x32, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x11E, 0x17, 0x32, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x90, 0x16, 0x30, 8);
        break;
    case 0x407:
        Em_PlaySeAtBoneVolOnFrame(em, 0x4A, 8, 0x1D, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x3C, 0x68, 0x1D, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x3C, 0, 0x1D, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x22, 0x10, 5, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x9C, 8, 0xE, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x8E, 0x69, 0xE, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x8E, 1, 0xE, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x74, 0x13, 0x13, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xCC, 8, 8, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xBE, 0x69, 8, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xBE, 1, 8, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0xA4, 0xE, 0x19, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x116, 8, 0x17, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x108, 0x68, 0x17, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x108, 0, 0x17, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0xEE, 0xC, 0xB, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x166, 8, 0x1D, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x158, 0x68, 0x1D, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x158, 0, 0x1D, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x13E, 0x10, 5, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x66, 0x17, 0x32, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x154, 0x17, 0x32, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x2C, 0x16, 0x30, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xF4, 0x16, 0x30, 8);
        break;
    case 0x424:
        Em_PlaySeTypeAtBoneInFrames(em, 0xE4, 0x19A, 0x46, 0x25, 8, 0);
        Em_PlaySeAtBoneVolOnFrame(em, 0xB2, 0x18, 0x23, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xAC, 0x19, 0x23, 9);
        Em_PlaySeAtBoneVolOnFrame(em, 0xAE, 0x6B, 0x23, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xAC, 8, 0x23, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x56, 8, 0xE, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x48, 0x69, 0xE, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x48, 1, 0xE, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x2E, 0x13, 0x13, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xA4, 0xE, 0x13, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xA4, 0x10, 0x19, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x154, 0x17, 0x32, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x2C, 0x16, 0x30, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xE4, 0xD, 0x21, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x104, 0x13, 3, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x48, 0xA, 0x1D, 8);
        break;
    case 0x425:
        Em_PlaySeTypeAtBoneInFrames(em, 4, 0xD2, 0x46, 0x25, 8, 0);
        Em_PlaySeTypeAtBoneInFrames(em, 0x104, 0x19A, 0x4B, 0x25, 9, 0);
        Em_PlaySeAtBoneVolOnFrame(em, 0x60, 9, 0xE, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x52, 0x69, 0xE, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x52, 1, 0xE, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x38, 0x13, 0x13, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xA4, 9, 8, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x96, 0x69, 8, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x96, 1, 8, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x7C, 0xE, 0x19, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xEA, 9, 0x17, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xDC, 0x68, 0x17, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xDC, 0, 0x17, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0xC2, 0x10, 0xB, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x13A, 9, 0x1D, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x12C, 0x68, 0x1D, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x12C, 0, 0x1D, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x112, 0xC, 5, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x38, 0x17, 0x32, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x38, 0x16, 0x30, 8);
        break;
    case 0x426:
        Em_PlaySeTypeAtBoneInFrames(em, 4, 0xD2, 0x4C, 0x25, 9, 0);
        Em_PlaySeAtBoneVolOnFrame(em, 0xAC, 0x17, 0x23, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x38, 0x17, 0x32, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x38, 0x16, 0x30, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0xE, 3, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0x13, 3, 8);
        break;
    case 0x427:
        Em_PlaySeTypeAtBoneInFrames(em, 4, 0x11C, 0x4C, 0x25, 9, 0);
        Em_PlaySeAtBoneVolOnFrame(em, 0x2C, 0x17, 0x32, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x2C, 0x16, 0x30, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x16, 9, 0xE, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 8, 0x69, 0xE, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 8, 1, 0xE, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0xE, 0x13, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1C, 9, 0x1D, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xE, 0x68, 0x1D, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xE, 0, 0x1D, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0x10, 5, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x68, 9, 0x17, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x5A, 0x68, 0x17, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x5A, 0, 0x17, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x40, 0x10, 0xB, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xB4, 9, 0xE, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xA6, 0x69, 0xE, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xA6, 1, 0xE, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x8C, 0xE, 0x13, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xD6, 8, 0x1D, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xC8, 0x68, 0x1D, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xC8, 0, 0x1D, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0xAE, 0x10, 5, 8);
        break;
    case 0x428:
        Em_PlaySeTypeAtBoneInFrames(em, 0x8A, 0xBC, 0x4C, 0x25, 9, 0);
        Em_PlaySeAtBoneVolOnFrame(em, 0x34, 8, 0xE, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x34, 0x69, 0xE, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xFC, 1, 0xE, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x38, 8, 8, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x38, 0x69, 8, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x38, 1, 8, 6);
        break;
    case 0x429:
        Em_PlaySeTypeAtBoneInFrames(em, 4, 0x18C, 0x49, 0x25, 9, 0);
        Em_PlaySeAtBoneVolOnFrame(em, 0x26, 9, 0x17, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x18, 0x68, 0x17, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x18, 0, 0x17, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0x10, 0xB, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xA8, 9, 0xE, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x9A, 0x69, 0xE, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x9A, 1, 0xE, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x80, 0x13, 0x13, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xB4, 9, 8, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xA6, 0x69, 8, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xA6, 1, 8, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x8C, 0xE, 0x19, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x124, 9, 0x1D, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x116, 0x68, 0x1D, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x116, 0, 0x1D, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0xFC, 0xC, 5, 8);
        break;
    case 0x42A:
        break;
    case 0x42B:
        Em_PlaySeTypeAtBoneInFrames(em, 6, 0x274, 0x4D, 0x25, 9, 0);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1A8, 0x19, 0x23, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x154, 0x1A, 0x13, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x190, 0x19, 0x13, 9);
        Em_PlaySeAtBoneVolOnFrame(em, 0x190, 0x18, 0x13, 9);
        Em_PlaySeAtBoneVolOnFrame(em, 0x192, 0x6B, 0x13, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x19A, 0x18, 0x30, 9);
        Em_PlaySeAtBoneVolOnFrame(em, 0xD8, 9, 0x17, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xCA, 0x68, 0x17, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xCA, 0, 0x17, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0xAE, 0x10, 0xB, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x144, 0x17, 0x32, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x140, 0x16, 0x30, 8);
        break;
    case 0x42C:
        Em_PlaySeTypeAtBoneInFrames(em, 4, 0xD2, 0x4C, 0x25, 9, 0);
        Em_PlaySeAtBoneVolOnFrame(em, 0xAC, 0x17, 0x23, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x38, 0x17, 0x32, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x38, 0x16, 0x30, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x58, 8, 0xE, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x4A, 0x69, 0xE, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x4A, 1, 0xE, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x30, 0x13, 0x13, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xF6, 9, 0xE, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xE8, 0x69, 0xE, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xE8, 1, 0xE, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0xCE, 0x13, 0x13, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x3C, 0x17, 0x32, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x3C, 0x16, 0x30, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x30, 0x19, 0xE, 8);
        
        break;
    }
}

/* em_khezu's .data (0x8C7B80-0x8C8BD0) */
extern u32 EmCephadrome_VelYCurves_3[];
EmTurnParams EmKhezu_TurnParams[6] = {
#include "game/008C7B80.inc"
};
EmKhezuVec EmKhezu_BaitThrowPt = {
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
EmVelocipreyAppearPoint EmVelociprey_AppearPtsFortress6[4] = {
#include "game/008C8470.inc"
};
EmVelocipreyAppearAt EmVelociprey_AppearPtsForestHills10[3] = {
#include "game/008C84D0.inc"
};
EmVelocipreyAppearAt EmVelociprey_AppearPtsForestHills9[3] = {
#include "game/008C8500.inc"
};
EmVelocipreyAppearPoint EmVelociprey_AppearPtsForestHills2[5] = {
#include "game/008C8530.inc"
};
EmVelocipreyAppearPoint EmVelociprey_AppearPtsForestHills1[4] = {
#include "game/008C85B0.inc"
};
EmVelocipreyAppearPoint EmVelociprey_AppearPtSnowyMountains3 = {
#include "game/008C8610.inc"
};
EmVelocipreyAppearAt EmVelociprey_AppearPtsJungle2[4] = {
#include "game/008C8630.inc"
};
EmVelocipreyAppearAt EmVelociprey_AppearPtsJungle3[3] = {
#include "game/008C8670.inc"
};
EmVelocipreyAppearAt EmVelociprey_AppearPtsJungle4[4] = {
#include "game/008C86A0.inc"
};
EmVelocipreyAppearAt EmVelociprey_AppearPtsJungle6[3] = {
#include "game/008C86E0.inc"
};
EmVelocipreyAppearAt EmVelociprey_AppearPtsJungle9[2] = {
#include "game/008C8710.inc"
};
EmVelocipreyAppearAt EmVelociprey_AppearPtsTower6[2] = {
#include "game/008C8730.inc"
};
EmVelocipreyAppearAt EmVelociprey_AppearPtsTower7[2] = {
#include "game/008C8750.inc"
};
EmVelocipreyAppearAt EmVelociprey_AppearPtsDesert2[3] = {
#include "game/008C8770.inc"
};
EmVelocipreyAppearAt EmVelociprey_AppearPtsDesert4[2] = {
#include "game/008C87A0.inc"
};
EmVelocipreyAppearAt EmVelociprey_AppearPtsDesert7[1] = {
#include "game/008C87C0.inc"
};
EmVelocipreyAppearAt EmVelociprey_AppearPtsSwamp5[2] = {
#include "game/008C87D0.inc"
};
EmVelocipreyAppearAt EmVelociprey_AppearPtsSwamp8[1] = {
#include "game/008C87F0.inc"
};
EmVelocipreyAppearAt EmVelociprey_AppearPtsSwamp9[3] = {
#include "game/008C8800.inc"
};
EmVelocipreyAppearAt EmVelociprey_AppearPtsVolcano3[2] = {
#include "game/008C8830.inc"
};
EmVelocipreyAppearAt EmVelociprey_AppearPtsVolcano6[1] = {
#include "game/008C8850.inc"
};
EmVelocipreyAppearAt EmVelociprey_AppearPtsBattleTown1[4] = {
#include "game/008C8860.inc"
};
EmVelocipreyAppearAt EmVelociprey_AppearPtsBattleTown2[2] = {
#include "game/008C88A0.inc"
};
EmVelocipreyAppearAt EmVelociprey_AppearPtsBattleTown3[2] = {
#include "game/008C88C0.inc"
};
EmTurnParams EmVelociprey_TurnParams[2] = {
#include "game/008C88E0.inc"
};
void *EmVelociprey_Callbacks[8] = {
    EmVelociprey_SpawnEffect01, EmVelociprey_SpawnEffect01, EmVelociprey_TableNop, EmVelociprey_UpdateMotionEffects,
    EmVelociprey_ScriptQuery, EmVelociprey_Reinit, NULL, NULL,
};
EmCurveKey EmVelociprey_VelYCurves_0[10] = {
#include "game/008C8940.inc"
};
EmCurveKey EmVelociprey_VelYCurves_1[12] = {
#include "game/008C8990.inc"
};
EmCurveKey EmVelociprey_VelYCurves_2[8] = {
#include "game/008C89F0.inc"
};
EmCurveKey EmVelociprey_VelYCurves_3[8] = {
#include "game/008C8A30.inc"
};
EmCurveKey *EmVelociprey_VelYCurves[4] = {
    EmVelociprey_VelYCurves_0, EmVelociprey_VelYCurves_1, EmVelociprey_VelYCurves_2, EmVelociprey_VelYCurves_3,
};
EmCurveKey EmVelociprey_SpeedCurves_0[10] = {
#include "game/008C8A80.inc"
};
EmCurveKey EmVelociprey_SpeedCurves_2[6] = {
#include "game/008C8AD0.inc"
};
s32 GmEm_ReturnZero(void); /* gm_sub */
void GmEm_UpdateSfx(Em *em); /* gm_sub */
EmCurveKey *EmVelociprey_SpeedCurves[4] = {
    EmVelociprey_SpeedCurves_0, NULL, EmVelociprey_SpeedCurves_2, NULL,
};
void *EmAnteka_Callbacks[8] = {
    EmAnteka_SpawnEffect01, EmAnteka_SpawnEffect01, EmAnteka_TableNop, EmAnteka_UpdateMotionEffects,
    GmEm_ReturnZero, EmAnteka_Reinit, NULL, NULL,
};
f32 EmMosswine_VariantParams[3][6] = {
#include "game/008C8B30.inc"
};
EmTurnParams EmMosswine_TurnParams[1] = {
#include "game/008C8B80.inc"
};
f32 EmMosswine_Effect0BOfs[3] = {
#include "game/008C8BA0.inc"
};
void *EmMosswine_Callbacks[8] = {
    EmMosswine_SpawnEffect01, EmMosswine_SpawnEffect01, EmMosswine_TableNop, GmEm_UpdateSfx,
    EmMosswine_ScriptQuery, EmMosswine_Reinit, NULL, NULL,
};
