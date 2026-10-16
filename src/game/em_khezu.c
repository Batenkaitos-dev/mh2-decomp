/*
 * Monster family: Khezu and Red Khezu (kinds 0xF, 0x2D).
 * The code from the family's setup function (0x7AE400, called by the per-kind
 * dispatch Em_TypeSetup) to the next family's. The state functions take
 * the monster and the work area its state dispatcher passes on (EmWork);
 * Em.step is the step within the current state. See include/em.h.
 */
#include "common.h"
#include "scene.h"
#include "quest.h"
#include "em.h"
#include "player.h"
#include "objattach.h"
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
void EffKhezu16_Create(Em *em, s8 kind);
void EmFxSpark_SpawnOnEm(Em *em, u8 k);
void EmFxSpark_SpawnTimed(Em *em, u8 k, s32 n, f32 f);
void StageObj_K0F_CreateOnEm(Em *em, u8 k, u8 c, f32 scale);
void EmKhezu_ChangeState(Em *em, u16 group, u16 state, u8 mode);
u8 EmKhezu_UpdateCurves(Em *em);
void EmMosswine_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void EmVelociprey_ChangeState(Em *em, u16 group, u16 state, u8 mode);
/* unit's own declaration: with the definition's void (Em *em, s16 n) (game/unk_708850.c), EmKhezu_State0_0E: 38 of 225 words differ, EmKhezu_State0_1A: 36 of 84 words differ */
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
ObjAttach *Eff02_Create(Em *em, s8 kind);
void EffEm08_CreateAtBone(Em *, s32, s32, s32, s32, s32);
void EffVelociprey13_Create(Em *, s8);
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
void Em_SpawnEffect06Bone(Em *em, s32 a);
void Em_AddNeed1(Em *em, s32 n);
void Em_AddNeed2(Em *em, s32 n);
void Em_SetHitReactMode(Em *em, u8 a);
void Em_SetGoal12(Em *em);
/* unit's own declaration: with the definition's s32 (Em *em, s16 n, s16 max, s16 interval) (game/unk_708850.c), EmKhezu_State0_12: 44 of 92 words differ, EmKhezu_State0_15: 42 of 90 words differ */
s32 Em_RegenHp(Em *em, s32 n, s32 max, s32 interval);
void Em_ClearPoison(Em *em);
void Em_SetHold2(Em *em);
void Em_SetCaptured(Em *em, s32 a);
void Em_PlaySe6AtBoneInFrames(Em *em, s32 frame, s32 frame2, s32 se, s32 bone, s32 vol, s32 a);
void Em_PlaySeAtBoneOnFrame2(Em *em, s32 frame, s32 se, s32 bone, s32 flag);
void Em_PlaySeAtBoneVolOnFrame(Em *em, s32 frame, s32 se, s32 bone, s32 vol);
s32 Em_StepToGoal(Em *em, u16 step, u8 flags);
u8 Em_TurnToGoal(Em *em, u16 step, u8 flags);
void Em_StartTurnToTarget(Em *em, EmTurnParams *t);
s32 Em_TurnStep(Em *em, EmTurnParams *t);
void Em_ClearTilt(Em *em, s32 arg1);
void Em_RestartScript(Em *em);
void EmKhezu_Init(Em *);
void EmKhezu_State0_1D(Em *, EmWork *);
void EmKhezu_State0_1F(Em *, EmWork *);
void EmKhezu_State1_07(Em *, EmWork *);
void EmKhezu_State2_01_FlyInLand(Em *, EmWork *);
void EmKhezu_State2_08(Em *, EmWork *);
void EmKhezu_State2_09(Em *, EmWork *);
void EmKhezu_State2_0D(Em *, EmWork *);
void EmKhezu_State3_01(Em *, EmWork *);
void EmKhezu_State6_00(Em *, EmWork *);
void EmKhezu_State6_01_Captured(Em *, EmWork *);
void EmKhezu_State8_00_CarveStart(Em *, EmWork *);
void EmKhezu_State5_00_Hunted(Em *em, EmWork *work);
void EmKhezu_State5_01_Hunted(Em *, EmWork *);
void EmKhezu_State5_02_HuntedFalling(Em *, EmWork *);
void EmKhezu_RunGroup4(Em *, EmWork *);
void EmKhezu_MotionSounds(Em *em, struct EmWork *);
void EmKhezu_MotionEffects(Em *, EmKhezuWork *);
void EmKhezu_SpawnK14Type0B(Em *em);
void EmKhezu_MoveByVelYCurve(Em *em, s32 kind);
void EmKhezu_StartCurves(Em *em, u8 kind);
s32 EmKhezu_InSpecialArea(u16 area);
void EmVelociprey_Init(Em *);
void EmVelociprey_Respawn(Em *);
void EmVelociprey_State1_01(Em *, EmWork *);
void EmVelociprey_RunGroup3(Em *em, EmWork *work);
void EmVelociprey_State5_00_Hunted(Em *em, EmWork *work);
void EmVelociprey_State5_01_Hunted(Em *em, EmWork *work);
void EmVelociprey_State5_02_Hunted(Em *, EmWork *);
void EmVelociprey_State5_03_Hunted(Em *em, EmWork *work);
void EmVelociprey_State5_04_Appear(Em *);
void EmVelociprey_State5_06_Hunted(Em *em, EmWork *work);
void EmVelociprey_State5_07_Hunted(Em *em, EmWork *work);
void EmVelociprey_State5_08_Hunted(Em *, EmWork *);
void EmVelociprey_State5_09_Hunted(Em *em, EmWork *work);
void EmVelociprey_State6_00(Em *, EmWork *);
void EmVelociprey_MotionEffects(Em *em, EmWork *work);
void EmVelociprey_StartCurves(Em *em, u8 kind);
u8 EmVelociprey_UpdateCurves(Em *em);
void EmAnteka_Init(Em *);
void EmAnteka_State4_02(Em *em, EmWork *work, s16 variant);
void EmAnteka_State5_00_Hunted(Em *, EmWork *);
void EmAnteka_EndState(Em *em);
void EmAnteka_SetMotion(Em *em, s32 arg1, s32 arg2, s32 arg3);
void EmAnteka_MotionEffects(Em *, EmWork *);
void EmMosswine_Init(Em *);
void EmMosswine_State0_0A(Em *);
void EmMosswine_State1_00(Em *, s32);
void EmMosswine_State1_01(Em *);
void EmMosswine_State1_02(Em *em);
void EmMosswine_State1_03(Em *em);
void EmMosswine_State1_04(Em *);
void EmMosswine_State1_05(Em *em);
void EmMosswine_State3_00_Charge(Em *, u32);
void EmMosswine_State4_01(Em *);
void EmMosswine_State4_02(Em *);
void EmMosswine_State5_00_Hunted(Em *);
void EmMosswine_State5_01_Hunted(Em *);
void StageObj_K13_Create(void *owner, s16 v, u8 k);
void StageObj_K14_Create(Em *em, s16 k, s16 c, f32 scale);
/* unit's own declaration: with the definition's void (Em *em, u8 k, s16 v, s16 t, f32 y, f32 scale) (game/unk_874B90.c), EmKhezu_State4_0E: 78 of 121 words differ */
void StageObj_Stun_CreateEm(Em *, s32, s32, s32, f32, f32);
s32 Sound_RandomSe(s32 id, s16 lo, s32 alt, s16 n);
s32 Player_PassedFrameScaled(void *p, f32 frame, s32 layer);
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
void EmKhezu_SetGroup0State(Em *em, u16 state, u8 mode);
void EmKhezu_SetGroup1State(Em *em, u16 state, u8 mode);
void EmKhezu_SetGroup2State(Em *em, u16 state, u8 mode);
void EmKhezu_SetGroup3State(Em *em, u16 state, u8 mode);
void EmVelociprey_SetGroup0State(Em *em, u16 state, u8 mode);
void EmVelociprey_SetGroup1State(Em *em, u16 state, u8 mode);
u8 EmKhezu_NextAreaPoint(Em *em, f32 *out);
struct AreaTri;
u8 AreaGrid2_GetCeiling(f32 *pos, f32 *y, struct AreaTri *info);
void EmVelociprey_SetGroup2State(Em *em, u16 state, u8 mode);
void EmVelociprey_SetGroup3State(Em *em, u16 state, u8 mode);
extern f32 EmMosswine_VariantParams[][6];
void Em_SpawnEffect06Periodic(Em *em, s32 bone, f32 *pos, f32 scale);
extern f32 EmMosswine_Effect0BOfs[3];
void Em_SetMotionRow(Em *em, s32 row, s32 arg2, s32 arg3);
void Em_SetPartScaled(Em *em, s32 bit);
s32 Em_ComputeSize(Em *em, s16 idx);
void StageObj_K09_Create(Em *);
s8 EmMine_CreateTimed0(Em *em, f32 *pos);
void Vec_Copy(f32 *out, f32 *in);
void StageObj_K25_Create(Player *p, Em *em, u8 k, s32 unused, f32 *pos);
void Versus_RouteEmByTeam(Em *em);
s32 Versus_DefeatCamera(Em *pl);
typedef struct EmKhezuVec {
    f32 x, y, z;
} EmKhezuVec;
extern EmKhezuVec EmKhezu_CarveThrowPt __attribute__((aligned(8)));
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
extern u8 gEmIsLarge2[];
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
void Em_RaiseTrigger80Lax(Em *em);
void Em_HoldPosNoPlayers(Em *em, u16 a, u16 b);
void Em_SetGoal12Dist(Em *em, u8 *p);
void Em_SetStatus1Timer(Em *em);
void Em_ResetKindTimer(Em *em);
s32 Em_IsHeld(Em *em);
u16 Area_BaseId(u16);
void Em_CheckLowHp(Em *em, f32 ratio);
void Em_DecayNeed1Only(Em *em);
void Em_DecayNeed1(Em *em);
/* Velociprey's quest appearance points: a position and the point it faces
   (splat split these tables at each field), or a position and a heading. */
typedef struct EmVelocipreyAppearPoint {
    f32 pos[3];
    f32 target[3];
} EmVelocipreyAppearPoint;
typedef struct EmVelocipreyAppearAt {
    f32 pos[3];
    u16 angle;
} EmVelocipreyAppearAt;
extern EmVelocipreyAppearPoint EmVelociprey_AppearPts1F[];
extern EmVelocipreyAppearPoint D_008C8474[];
extern EmVelocipreyAppearPoint D_008C8478[];
extern EmVelocipreyAppearPoint D_008C847C[];
extern EmVelocipreyAppearPoint D_008C8480[];
extern EmVelocipreyAppearPoint D_008C8484[];
extern EmVelocipreyAppearPoint EmVelociprey_AppearPts26[];
extern EmVelocipreyAppearPoint D_008C8534[];
extern EmVelocipreyAppearPoint D_008C8538[];
extern EmVelocipreyAppearPoint D_008C853C[];
extern EmVelocipreyAppearPoint D_008C8540[];
extern EmVelocipreyAppearPoint D_008C8544[];
extern EmVelocipreyAppearPoint EmVelociprey_AppearPts27[];
extern EmVelocipreyAppearPoint D_008C85B4[];
extern EmVelocipreyAppearPoint D_008C85B8[];
extern EmVelocipreyAppearPoint D_008C85BC[];
extern EmVelocipreyAppearPoint D_008C85C0[];
extern EmVelocipreyAppearPoint D_008C85C4[];
extern f32 D_008C8610;
extern f32 D_008C8614;
extern f32 D_008C8618;
extern f32 D_008C861C;
extern f32 D_008C8620;
extern f32 D_008C8624;
extern EmVelocipreyAppearAt EmVelociprey_AppearPts22[];
extern EmVelocipreyAppearAt EmVelociprey_AppearPts24[];
extern EmVelocipreyAppearAt EmVelociprey_AppearPts70[];
extern EmVelocipreyAppearAt EmVelociprey_AppearPts71[];
extern EmVelocipreyAppearAt EmVelociprey_AppearPts72[];
extern EmVelocipreyAppearAt EmVelociprey_AppearPts74[];
extern EmVelocipreyAppearAt EmVelociprey_AppearPts77[];
extern EmVelocipreyAppearAt EmVelociprey_AppearPts80[];
extern EmVelocipreyAppearAt EmVelociprey_AppearPts81[];
extern EmVelocipreyAppearAt EmVelociprey_AppearPts8E[];
extern EmVelocipreyAppearAt EmVelociprey_AppearPts90[];
extern EmVelocipreyAppearAt EmVelociprey_AppearPts93[];
extern EmVelocipreyAppearAt EmVelociprey_AppearPts9C[];
extern EmVelocipreyAppearAt EmVelociprey_AppearPts9F[];
extern EmVelocipreyAppearAt EmVelociprey_AppearPtsA0[];
extern EmVelocipreyAppearAt EmVelociprey_AppearPtsA4[];
extern EmVelocipreyAppearAt EmVelociprey_AppearPtsA7[];
extern EmVelocipreyAppearAt EmVelociprey_AppearPtsB0[];
extern EmVelocipreyAppearAt EmVelociprey_AppearPtsB1[];
extern EmVelocipreyAppearAt EmVelociprey_AppearPtsB2[];
/* end of prototypes */

void EmKhezu_Setup(Em *em)
{
}

void EmKhezu_SpawnEffects(void *em)
{
    StageObj_K13_Create(em, 0x22, 0);
    Effect01_Spawn((Player *) em, 0);
}

void EmKhezu_Init(Em *em)
{
    EmKhezuWork *w = (EmKhezuWork *)&em->work;

    f32 fslot;
    s16 size;
    u8 slotNo;
    u8 fxFlags;

    Em_SetMotionRow(em, 1, 0, 0);
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
    size = Em_ComputeSize(em, 0);
    em->hp = size;
    em->hpMax = size;
    em->scriptPending = 1;
    em->blind = 0;
    em->unk_InitFlag = 1;
    em->noAlert = 0;
    em->painted = 0;
    w->unk014 = 0x4000;
    w->unk02C = 0x100;
    *(s32 *)((u8 *)w + 0x30) = 0x200;
    w->unk034 = 0x100;
    w->unk01C = 0;
    w->unk044 = 0;
    fxFlags = em->fxFlags & 1;
    em->fxFlags = fxFlags;
    if (fxFlags == 0) {
        switch (em->type) {
        case 0xF:
        case 0x2D:
            em->meshShown[1] = 0;
            StageObj_K09_Create(em);
            break;
        }
    }
}

void EmKhezu_Start(Em *em)
{
    EmKhezu_Init(em);
    if (em->type == 0xF || em->type == 0x2D) {
        switch (gQuest.stage) {
        case 0x422:
            if (Save_TestEventFlag(0xA7) == 0) {
                em->area = Area_IdForCurrentTime(0x5E);
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
        em->stance = 0;
        EmKhezu_ChangeState(em, 0, 1, 0);
    }
}

void EmKhezu_Reinit(Em *em)
{
    EmKhezu_Init(em);
}

void EmKhezu_EndState(Em *em)
{
    em->motionSpeed = 1.0f;
    em->stance = 0;
    em->lookAtTarget = 0;
    em->scriptPending = 1;
    EmKhezu_ChangeState(em, 0, 1, 0);
}

void EmKhezu_EndState2To2_19(Em *em)
{
    em->motionSpeed = 1.0f;
    em->stance = 2;
    em->lookAtTarget = 0;
    em->scriptPending = 1;
    em->ceilingTimer = 5;
    EmKhezu_ChangeState(em, 2, 0x19, 0);
}

void EmKhezu_EndState2(Em *em)
{
    em->scriptPending = 1;
    em->motionSpeed = 1.0f;
    em->stance = 2;
    em->lookAtTarget = 0;
    EmKhezu_ChangeState(em, 2, 0xE, 0);
}

void EmKhezu_RestartLayerMotion(Em *em, s32 layer)
{
    if (em->motion[layer].playing == 0) {
        switch (layer) {
        case 0:
            Em_SetLayerMotion(em, 0x3E9, 10, 0, 0);
            break;
        case 1:
            Em_SetLayerMotion(em, 0x4B1, 10, 0, 1);
            break;
        case 2:
            Em_SetLayerMotion(em, 0x579, 10, 0, 2);
            break;
        }
    }
}

void EmKhezu_State0_00(Em *em, EmWork *work)
{
    s32 t;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 1, 0, 0);
        em->timer = 0x1E;
        break;
    case 1:
        if ((em->remote == 0) && ((em->motion[0].playing == 0) || (t = em->timer - 1, em->timer = t, (t <= 0)))) {
            em->step++;
            EmKhezu_EndState(em);
        }
        break;
    }
}

void EmKhezu_State0_01(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 1, 0, 0);
        break;
    case 1:
        if ((em->remote == 0) && (em->motion[0].playing == 0)) {
            em->step++;
            EmKhezu_EndState(em);
        }
        break;
    }
}

void EmKhezu_State0_02(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x65, 0, 0);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmKhezu_ChangeState(em, 0, 3, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmKhezu_ChangeState(em, 0, 3, 4);
        }
        break;
    }
}

void EmKhezu_State0_03(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x64, 0, 0);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmKhezu_ChangeState(em, 0, 3, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmKhezu_ChangeState(em, 0, 3, 4);
        }
        break;
    }
}

void EmKhezu_State0_04(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x66, 0, 0);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmKhezu_EndState(em);
        }
        break;
    }
}

void EmKhezu_State0_05(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x18, 0, 0);
        Em_SetLayerMotion(em, 0x3E9, 0xA, 0, 0);
        Em_SetLayerMotion(em, 0x579, 0xA, 0, 2);
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

void EmKhezu_State0_06(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x69, 0, 0);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmKhezu_EndState(em);
        }
        break;
    }
}

void EmKhezu_State0_07(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 1, 0, 0);
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

void EmKhezu_State0_08(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x33, 0, 0);
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

void EmKhezu_State0_0A(Em *em, EmWork *work)
{
    u16 goalYaw;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x36, 0, 0);
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

void EmKhezu_State0_0D(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x14, 0, 0);
        if (em->motionId[0] != 0x3E9) {
            Em_SetLayerMotion(em, 0x3E9, 0xA, 0, 0);
        }
        if (em->motionId[2] != 0x4B1) {
            Em_SetLayerMotion(em, 0x579, 0xA, 0, 2);
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
    Em_PlaySeAtBoneOnFrame2(em, 2, 0x20, 0x23, 1);
    Em_PlaySeAtBoneOnFrame2(em, 0x4A, 0x1F, 0x23, 1);
}

void EmKhezu_State0_0E(Em *em, EmWork *work)
{
    Em_SetHitReactMode(em, 3U);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x4F, 0, 0);
        Em_SelectSightRow(em, 1);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if ((Player_PassedFrameScaled(em, 56.0f, 0U) == 0) && (Player_PassedFrameScaled(em, 108.0f, 0U) == 0) && (Player_PassedFrameScaled(em, 126.0f, 0U) == 0) && (Player_PassedFrameScaled(em, 138.0f, 0U) == 0)) {
            if (Player_PassedFrameScaled(em, 170.0f, 0U) != 0) {
                goto block_12;
            }
        } else {
block_12:
            Em_PlaySeAtBone(em, 0x26, 0x23);
            StageObj_K14_Create(em, 7, 0, 5.0f);
        }
        if ((Player_PassedFrameScaled(em, 210.0f, 0U) != 0) || (Player_PassedFrameScaled(em, 218.0f, 0U) != 0)) {
            StageObj_K14_Create(em, 7, 0, 2.5f);
        }
        if (Player_PassedFrameScaled(em, 52.0f, 0U) != 0) {
            StageObj_K14_Create(em, 8, 0, 1.0f);
        }
        if ((Player_PassedFrameScaled(em, 106.0f, 0U) != 0) || (Player_PassedFrameScaled(em, 130.0f, 0U) != 0)) {
            StageObj_K14_Create(em, 6, 0, 1.0f);
        }
        if (Player_PassedFrameScaled(em, 124.0f, 0U) != 0) {
            StageObj_K14_Create(em, 6, 1, 1.0f);
        }
        if (Player_PassedFrameScaled(em, 170.0f, 0U) != 0) {
            StageObj_K14_Create(em, 0xF, 0, 1.0f);
        }
        if (Player_PassedFrameScaled(em, 192.0f, 0U) != 0) {
            StageObj_K14_Create(em, 0x10, 0, 1.0f);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x3B, 0, 0);
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

void EmKhezu_State0_11(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x33, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKhezu_EndState(em);
        }
        break;
    }
}

void EmKhezu_State0_13(Em *em, EmWork *work)
{
    Em_SetHitReactMode(em, 3U);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x4F, 0, 0);
        Em_SelectSightRow(em, 1);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 200.0f, 0U) != 0) {
            em->step++;
            EmKhezu_ChangeState(em, 0, 0x28, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmKhezu_ChangeState(em, 0, 0x28, 4);
        }
        break;
    }
}

void EmKhezu_State0_28(Em *em, EmWork *work)
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
            Em_SetMotion(em, 0x3B, 0, 0);
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

void EmKhezu_State0_12(Em *em, EmWork *work)
{
    Em_SetHitReactMode(em, 2U);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x20, 0, 0);
        Em_LoadReactionSet(em, 1);
        em->timer = 0x2328;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x1F, 0, 0);
        }
        break;
    case 2:
        Em_RegenHp(em, 1, (s32) (0.5f * (f32) em->hpMax), 0xC);
        if (--em->timer <= 0) {
            em->step++;
            Em_ClearLowHpNeed(em);
            EmKhezu_ChangeState(em, 0, 0x17, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmKhezu_ChangeState(em, 0, 0x17, 4);
        }
        break;
    }
}

void EmKhezu_State0_14(Em *em, EmWork *work)
{
    Em_SetHitReactMode(em, 2U);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x26, 0, 0);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x22, 0, 0);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            EmKhezu_ChangeState(em, 0, 0x18, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmKhezu_ChangeState(em, 0, 0x18, 4);
        }
        break;
    }
}

void EmKhezu_State0_15(Em *em, EmWork *work)
{
    Em_SetHitReactMode(em, 2U);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x27, 0, 0);
        Em_LoadReactionSet(em, 1);
        em->timer = 0x2328;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x22, 0, 0);
        }
        break;
    case 2:
        Em_RegenHp(em, 1, (s32) (0.5f * (f32) em->hpMax), 5);
        if (--em->timer <= 0) {
            em->step++;
            EmKhezu_ChangeState(em, 0, 0x18, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmKhezu_ChangeState(em, 0, 0x18, 4);
        }
        break;
    }
}

void EmKhezu_State0_16(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x3E, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKhezu_EndState(em);
        }
        break;
    }
}

void EmKhezu_State0_17(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x21, 0, 0);
        Em_LoadReactionSet(em, 0);
        em->blind = 2;
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            EmKhezu_ChangeState(em, 0, 0x19, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmKhezu_ChangeState(em, 0, 0x19, 4);
        }
        break;
    }
}

void EmKhezu_State0_18(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x25, 0, 0);
        Em_LoadReactionSet(em, 0);
        em->blind = 2;
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            EmKhezu_ChangeState(em, 0, 0x19, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmKhezu_ChangeState(em, 0, 0x19, 4);
        }
        break;
    }
}

void EmKhezu_State0_1A(Em *em, EmWork *work)
{
    Em_SetHitReactMode(em, 3U);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x4F, 0, 0);
        Em_SelectSightRow(em, 1);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x3B, 0, 0);
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

void EmKhezu_State0_1B(Em *em, EmWork *work)
{
    Em_SetHitReactMode(em, 2U);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x26, 0, 0);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x22, 0, 0);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            EmKhezu_ChangeState(em, 0, 0x1C, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmKhezu_ChangeState(em, 0, 0x1C, 4);
        }
        break;
    }
}

void EmKhezu_State0_1D(Em *em, EmWork *work)
{
    f32 pos[3];

    Em_SetHitReactMode(em, 2);
    em->holdTimer = 5;
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x61, 0, 0);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmKhezu_ChangeState(em, 4, 0xF, 3);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmKhezu_ChangeState(em, 4, 0xF, 3);
        }
        break;
    }
    pos[0] = 0.0f;
    pos[1] = 10.0f;
    pos[2] = 140.0f;
    Em_SpawnEffect06Periodic(em, 0x22, pos, 1.6f);
    if (em->timer % 135 == 0) {
        Em_PlaySeAtBone(em, 0x57, 0x23);
    }
}

void EmKhezu_State0_1F(Em *em, EmWork *work)
{
    f32 pos[3];

    Em_SetHitReactMode(em, 2);
    em->holdTimer = 5;
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x61, 0, 0);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmKhezu_ChangeState(em, 4, 0x10, 3);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmKhezu_ChangeState(em, 4, 0x10, 3);
        }
        break;
    }
    pos[0] = 0.0f;
    pos[1] = 10.0f;
    pos[2] = 140.0f;
    Em_SpawnEffect06Periodic(em, 0x22, pos, 1.6f);
    if (em->timer % 135 == 0) {
        Em_PlaySeAtBone(em, 0x57, 0x23);
    }
}

void EmKhezu_State1_00(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 3, 0, 0);
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

void EmKhezu_State1_03(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_StartTurnToTarget(em, &EmKhezu_TurnParams[0]);
        break;
    case 1:
        if (Em_TurnStep(em, &EmKhezu_TurnParams[0]) == 1) {
            em->step++;
            Player_ClearFlag(em, 0x20000);
            EmKhezu_EndState(em);
        }
        break;
    }
}

void EmKhezu_State1_05(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_StartTurnToTarget(em, &EmKhezu_TurnParams[1]);
        break;
    case 1:
        if (Em_TurnStep(em, &EmKhezu_TurnParams[1]) == 1) {
            em->step++;
            Player_ClearFlag(em, 0x20000);
            EmKhezu_EndState(em);
        }
        break;
    }
}

void EmKhezu_State1_06(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0xA, 0, 0);
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

void EmKhezu_State1_07(Em *em, EmWork *work)
{
    u16 goalYaw;
    u32 diff;
    s32 rotY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x36, 0, 0);
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

void EmKhezu_State2_00(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x12, 0, 0);
        EmKhezu_StartCurves(em, 2);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 38.0f, 0U) != 0) {
            em->step++;
            em->stance = 2;
            EmKhezu_UpdateCurves(em);
        }
        break;
    case 2:
        if (EmKhezu_UpdateCurves(em)) {
            em->step++;
            EmKhezu_EndState2(em);
        }
        break;
    }
}

void EmKhezu_State2_01_FlyInLand(Em *em, EmWork *work)
{
    f32 h;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        em->rotX = 0;
        em->rotZ = 0;
        Em_SetMotion(em, 0xF, 0, 0);
        Player_ClearVelocity(em);
        em->velY = -10.0f;
        Em_ClearTilt(em, 1);
        h = em->floorY;
        h = em->posY - (1000.0f + h);
        if (h > 0.0f) {
            do {
                em->velY -= 0.75f;
                h += em->velY;
            } while (h > 0.0f && !(em->velY < -200.0f));
        }
        break;
    case 1:
        Em_MoveRotated(em, &em->rotX);
        if (em->velY < -10.0f) {
            em->velY += 0.75f;
        }
        if (!((em->floorY + 630.0f * em->scaleY) <= em->posY) && ((Player_PassedFrameScaled(em, 10.0f, 0U) != 0) || (Player_PassedFrameScaled(em, 86.0f, 0U) != 0) || (Player_PassedFrameScaled(em, 160.0f, 0U) != 0))) {
            em->step++;
            Em_SetMotion(em, 0xB, 0, 0);
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
            Em_SetMotion(em, 0x13, 0, 0);
            em->posY = em->floorY;
            em->rotX = 0;
            em->rotZ = 0;
            em->stance = 0;
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
    h = em->floorY;
    if (em->posY < h) {
        em->posY = h;
    }
}

void EmKhezu_State2_02(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        work->moveAngX = 0x100;
        Em_SetMotion(em, 0xF, 0, 0);
        break;
    case 1:
        EmKhezu_MoveByVelYCurve(em, 1U);
        if (--em->timer <= 0) {
            em->step++;
            EmKhezu_ChangeState(em, 2, 1, 1);
        }
        Em_TurnToGoal(em, (s32) (u16) work->moveAngX, 1);
        if (em->timer == 0x12C) {
            EmKhezu_EndState2(em);
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

void EmKhezu_State2_03(Em *em, EmWork *work)
{
    f32 velY;
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x12, 0, 0);
        Player_ClearVelocity(em);
        EmKhezu_StartCurves(em, 2);
        break;
    case 1:
        if ((EmMotion_ReachedFrame(em, 0, 50.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 114.0f) == 0)) {
            Em_TurnToGoal(em, (s32) ((EmKhezuWork *)work)->turnSpeed, 1);
        }
        if (EmMotion_ReachedFrame(em, 0, 30.0f) != 0) {
            em->stance = 2;
            EmKhezu_UpdateCurves(em);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
        }
        break;
    case 2:
        if ((EmKhezu_UpdateCurves(em) & 0xFF) && (em->posY <= (em->floorY + (800.0f * em->scaleY)))) {
            em->step++;
            Em_SetMotion(em, 0xB, 0, 0);
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
            Em_SetMotion(em, 0x13, 0, 0);
            em->stance = 0;
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

void EmKhezu_State2_04(Em *em, EmWork *work)
{
    f32 floorY;
    s32 ended;

    ended = 0;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x12, 0, 0);
        EmKhezu_StartCurves(em, 3);
        em->velZ = 20.0f;
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 30.0f) != 0) {
            em->stance = 2;
            ended = EmKhezu_UpdateCurves(em) & 0xFF;
        }
        if ((ended != 0) && (em->posY <= em->floorY)) {
            Em_SetMotion(em, 0x13, 0, 0);
            em->step++;
            em->stance = 0;
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

void EmKhezu_State2_05(Em *em, EmWork *work)
{
    f32 floorY;
    s32 ended;

    ended = 0;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x12, 0, 0);
        EmKhezu_StartCurves(em, 3);
        em->velZ = -20.0f;
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 30.0f) != 0) {
            em->stance = 2;
            ended = EmKhezu_UpdateCurves(em) & 0xFF;
        }
        if ((ended != 0) && (em->posY <= em->floorY)) {
            Em_SetMotion(em, 0x13, 0, 0);
            em->step++;
            em->stance = 0;
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

void EmKhezu_State2_06(Em *em, EmWork *work)
{
    f32 dist;
    f32 floorY;
    s32 timer;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        work->moveAngX = 0x100;
        Em_SetMotion(em, 0xF, 0, 0);
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
            EmKhezu_EndState2(em);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmKhezu_State2_07(Em *em, EmWork *work)
{
    f32 dist;
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        work->moveAngX = 0x100;
        Em_SetMotion(em, 0xF, 0, 0);
        break;
    case 1:
        Em_TurnToGoal(em, (s32) (u16) work->moveAngX, 1);
        work->moveAngBZ = em->rotY;
        work->moveAngCX = 0;
        Em_GetPitchToGoal(em, &work->moveAngBY, 0.0f, 0.0f);
        Em_MoveRotated(em, &work->moveAngBY);
        EmKhezu_MoveByVelYCurve(em, 1U);
        dist = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer > 0) {
            if (dist <= (10.0f * em->velZ)) {
                goto block_9;
            }
        } else {
block_9:
            em->step++;
            Player_ClearVelocity(em);
            EmKhezu_EndState2(em);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmKhezu_State2_08(Em *em, EmWork *work)
{
    u16 next;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        Player_ClearVelocity(em);
        em->velZ = 40.0f;
        Em_SetMotion(em, 0xF, 0, 0);
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
        if (em->nextArea == next || next == 0xFFFF) {
            Hud_TriggerGlow(em);
            EmKhezu_ChangeState(em, 2, 9, 1);
        } else {
            if (em->remote == 0) {
                em->nextArea = next;
                em->goalNo = next;
                em->goalKind = 3;
                Em_ComputeGoal(em, em->goal);
            }
            EmKhezu_ChangeState(em, 2, 0xD, 1);
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

void EmKhezu_State2_09(Em *em, EmWork *work)
{
    f32 d;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        work->moveAngX = 0x100;
        Em_SetMotion(em, 0xF, 0, 0);
        Player_ClearVelocity(em);
        em->velZ = 40.0f;
        work->goalYaw = Math_AngleXZ(&em->posX, em->goal);
        em->rotY = work->goalYaw;
        *(u16 *) &em->destArea = 0xFFFF;
        break;
    case 1:
        Em_TurnToGoal(em, (s32) (u16) work->moveAngX, 0);
        work->moveAngBY = em->rotX;
        work->moveAngBZ = em->rotY;
        work->moveAngCX = 0;
        Em_MoveRotated(em, &work->moveAngBY);
        d = Vec_DistanceXZ(&em->posX, em->goal);
        em->timer--;
        if (d <= 500.0f || em->timer < 0) {
            em->step++;
            em->timer = 600;
            EmKhezu_ChangeState(em, 2, 1, 1);
        }
        break;
    case 2:
        if (em->remote == 0) {
            if (--em->timer <= 0) {
                em->timer = 600;
                EmKhezu_ChangeState(em, 2, 9, 1);
            }
        }
        break;
    }
}

void EmKhezu_State2_0B(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0xF, 0, 0);
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
            EmKhezu_ChangeState(em, 2, 1, 1);
        } else {
            Em_TurnToGoal(em, (s32) ((EmKhezuWork *)work)->turnSpeed, 0);
            work->moveAngBY = em->rotX;
            work->moveAngBZ = em->rotY;
            work->moveAngCX = 0;
            Em_MoveRotated(em, &work->moveAngBY);
            if (--em->timer <= 0) {
                em->step++;
                EmKhezu_ChangeState(em, 2, 1, 1);
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
                EmKhezu_ChangeState(em, 2, 1, 1);
            }
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmKhezu_State2_0C(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        Em_SetMotion(em, 0xB, 0, 0);
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
            EmKhezu_EndState2(em);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmKhezu_State2_0D(Em *em, EmWork *work)
{
    AreaRecord *area;
    f32 d;
    u16 next;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        work->moveAngX = 0x100;
        Em_SetMotion(em, 0xF, 0, 0);
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
        d = Vec_DistanceXZ(&em->posX, em->goal);
        em->timer--;
        if (d <= 500.0f || em->timer < 0) {
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
        if (em->nextArea == next || next == 0xFFFF) {
            Hud_TriggerGlow(em);
            EmKhezu_ChangeState(em, 2, 9, 1);
        } else {
            if (em->remote == 0) {
                em->nextArea = next;
                em->goalNo = next;
                em->goalKind = 3;
                Em_ComputeGoal(em, em->goal);
            }
            EmKhezu_ChangeState(em, 2, 0xD, 1);
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

void EmKhezu_State2_0E(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        Em_SetMotion(em, 0xF, 0, 0);
        break;
    case 1:
        EmKhezu_MoveByVelYCurve(em, 1U);
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKhezu_EndState2(em);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmKhezu_State2_0F(Em *em, EmWork *work)
{
    f32 pos[3];
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        Em_SetMotion(em, 0xF, 0, 0);
        work->moveAngBY = 0;
        work->moveAngCX = 0;
        Player_ClearVelocity(em);
        em->velY = 20.0f;
        break;
    case 1:
        work->moveAngBZ = Math_AngleXZ(&em->posX, pos) & 0xFFFF;
        Em_MoveRotated(em, &work->moveAngBY);
        if (--em->timer > 0) {
            if (!(em->posY <= (3000.0f + em->goal[1]))) {
                goto block_8;
            }
        } else {
block_8:
            em->step++;
            Player_ClearVelocity(em);
            EmKhezu_EndState2(em);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmKhezu_State2_10(Em *em, EmWork *work)
{
    f32 floorY;
    s32 done;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        Em_SetMotion(em, 0xF, 0, 0);
        work->moveAngX = 0x100;
        work->goalYaw = Math_AngleXZ(&em->posX, em->goal);
        work->goalYaw = work->goalYaw - em->rotY;
        em->timer = 0x12C;
        break;
    case 1:
        EmKhezu_MoveByVelYCurve(em, 1U);
        done = Em_TurnToGoal(em, (s32) (u16) work->moveAngX, 1) & 0xFF;
        if (--em->timer > 0) {
            if (done != 0) {
                goto block_8;
            }
        } else {
block_8:
            em->step++;
            EmKhezu_EndState2(em);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmKhezu_State2_11(void)
{
}

void EmKhezu_State2_12(void)
{
}

void EmKhezu_State2_13(void)
{
}

void EmKhezu_State2_14(Em *em, EmWork *work)
{
    f32 floorY;
    s32 ended;

    ended = 0;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x54, 0, 0);
        EmKhezu_StartCurves(em, 0xC);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 44.0f) != 0) {
            em->stance = 2;
            ended = EmKhezu_UpdateCurves(em) & 0xFF;
        }
        if ((ended != 0) && (em->posY <= em->floorY)) {
            em->step++;
            em->stance = 0;
            em->posY = em->floorY;
            EmKhezu_SpawnK14Type0B(em);
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

void EmKhezu_State2_15(Em *em, EmWork *work)
{
    f32 dist;
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        work->moveAngX = 0x100;
        Em_SetMotion(em, 0xF, 0, 0);
        break;
    case 1:
        work->moveAngBZ = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        work->moveAngCX = 0;
        Em_GetPitchToGoal(em, &work->moveAngBY, 0.0f, 0.0f);
        Em_MoveRotated(em, &work->moveAngBY);
        dist = Vec_Distance((f32 *) &em->posX, (f32 *) em->goal);
        if (--em->timer > 0) {
            if (dist <= (10.0f * em->velZ)) {
                goto block_9;
            }
        } else {
block_9:
            em->step++;
            Player_ClearVelocity(em);
            EmKhezu_EndState2(em);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmKhezu_State2_16(void)
{
}

void EmKhezu_State2_17(Em *em, EmWork *work)
{
    f32 floorY;
    f32 velY;
    f32 y;

    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x12, 0xA, 0x1E);
        EmKhezu_StartCurves(em, 2);
        em->stance = 2;
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
            Em_SetMotion(em, 0xB, 0, 0);
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
            Em_SetMotion(em, 0x13, 0, 0);
            em->stance = 0;
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

void EmKhezu_State2_18(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x68, 0, 0);
        em->stance = 0;
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 112.0f, 0U) != 0) {
            em->step++;
            em->stance = 2;
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
            Em_SetMotion(em, 0x76, 0, 0);
            em->ceilingTimer = 5;
        }
        break;
    case 3:
        em->ceilingTimer = 5;
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKhezu_EndState2To2_19(em);
        }
        break;
    }
}

void EmKhezu_State2_19(Em *em, EmWork *work)
{
    em->ceilingTimer = 5;
    em->posY = em->surfaceY;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        if (em->motionId[0] != 0x44F) {
            Em_SetMotion(em, 0x67, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKhezu_EndState2To2_19(em);
        }
        break;
    }
}

void EmKhezu_State2_1A(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        Em_SetMotion(em, 0x6D, 0, 0);
        Player_ClearVelocity(em);
        em->velY = -50.0f;
        em->accelY = -5.0f;
        Em_ClearTilt(em, 1);
        em->ceilingTimer = 5;
        break;
    case 1:
        em->ceilingTimer = 5;
        if (Player_PassedFrameScaled(em, 12.0f, 0U) != 0) {
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
            em->stance = 0;
            Em_SetMotion(em, 0x75, 0, 0);
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

void EmKhezu_State2_1B(Em *em, EmWork *work)
{
    em->ceilingTimer = 5;
    em->posY = em->surfaceY;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        Em_StartTurnToTarget(em, &EmKhezu_TurnParams[2]);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 8.0f) != 0) {
            em->step++;
        case 2:
            if (Em_TurnStep(em, &EmKhezu_TurnParams[2]) == 1) {
                em->step++;
                Player_ClearFlag(em, 0x20000);
                EmKhezu_EndState2To2_19(em);
            }
        }
        break;
    }
}

void EmKhezu_State2_1C(Em *em, EmWork *work)
{
    em->ceilingTimer = 5;
    em->posY = em->surfaceY;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x77, 0, 0);
        break;
    case 1:
        if (Em_StepToGoal(em, 0x40, 0) == 1) {
            em->step++;
            EmKhezu_EndState2To2_19(em);
        }
        break;
    }
}

void EmKhezu_State2_1D(Em *em, EmWork *work)
{
    s8 n;

    em->ceilingTimer = 5;
    em->posY = em->surfaceY;
    switch (em->step) {
    case 0:
        em->stance = 2;
        Em_SetMotion(em, 0x77, 0, 0);
        em->step++;
        em->stateVar1 = 0;
        em->lookAtTarget = 0;
        em->timer = 0x12C;
        break;
    case 1:
        if (!(Vec_DistanceXZ(&em->posX, em->goal) <= 300.0f)) {
            if (--em->timer <= 0) {
                goto block_8;
            }
        } else {
block_8:
            n = ((EmKhezuWork *)work)->unk00A - 1;
            ((EmKhezuWork *)work)->unk00A = n;
            if (n <= 0) {
                em->step++;
                EmKhezu_EndState2To2_19(em);
            } else {
                em->curGoalNo++;
                em->curGoalNo &= 3;
                Em_ComputeGoal2(em, em->goal);
                EmKhezu_ChangeState(em, 2, 0x1D, 0);
            }
        }
        Em_TurnToGoal(em, 0x200, 0);
        break;
    }
}

void EmKhezu_State2_1E(Em *em, EmWork *work)
{
    s8 n;

    em->ceilingTimer = 5;
    em->posY = em->surfaceY;
    switch (em->step) {
    case 0:
        em->stance = 2;
        Em_SetMotion(em, 0x77, 0, 0);
        em->step++;
        em->stateVar1 = 0;
        em->lookAtTarget = 0;
        em->timer = 0x12C;
        break;
    case 1:
        if (!(Vec_DistanceXZ(&em->posX, em->goal) <= 300.0f)) {
            if (--em->timer <= 0) {
                goto block_8;
            }
        } else {
block_8:
            n = ((EmKhezuWork *)work)->unk00A - 1;
            ((EmKhezuWork *)work)->unk00A = n;
            if (n <= 0) {
                em->step++;
                EmKhezu_EndState2To2_19(em);
            } else {
                em->curGoalNo -= 1;
                em->curGoalNo &= 3;
                Em_ComputeGoal2(em, em->goal);
                EmKhezu_ChangeState(em, 2, 0x1E, 0);
            }
        }
        Em_TurnToGoal(em, 0x200, 0);
        break;
    }
}

void EmKhezu_State2_1F(Em *em, EmWork *work)
{
    em->ceilingTimer = 5;
    em->posY = em->surfaceY;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        Player_SetFlag((Player *) em, 0x20000U);
        Em_StartTurnToTarget(em, &EmKhezu_TurnParams[3]);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 8.0f) != 0) {
            em->step++;
        case 2:
            if (Em_TurnStep(em, &EmKhezu_TurnParams[3]) == 1) {
                em->step++;
                em->timer = 0x12C;
                Player_ClearFlag(em, 0x20000);
                EmKhezu_ChangeState(em, 2, 0x1D, 1);
                break;
            }
        } else {
        }
        break;
    case 3:
        if (--em->timer <= 0) {
            em->timer = 0x12C;
            if (em->remote == 0) {
                EmKhezu_ChangeState(em, 2, 0x1D, 1);
            }
        }
        break;
    }
}

void EmKhezu_State2_20(Em *em, EmWork *work)
{
    em->ceilingTimer = 5;
    em->posY = em->surfaceY;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x77, 0, 0);
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
            EmKhezu_ChangeState(em, 2, 0x21, 1);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->timer = 0x12C;
            if (em->remote == 0) {
                EmKhezu_ChangeState(em, 2, 0x21, 1);
            }
        }
        break;
    }
}

void EmKhezu_State2_21(Em *em, EmWork *work)
{
    em->ceilingTimer = 5;
    em->posY = em->surfaceY;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        Em_StartTurnToTarget(em, &EmKhezu_TurnParams[4]);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 8.0f) != 0) {
            em->step++;
        case 2:
            if (Em_TurnStep(em, &EmKhezu_TurnParams[4]) == 1) {
                em->step++;
                Player_ClearFlag(em, 0x20000);
                em->timer = 0x12C;
                EmKhezu_ChangeState(em, 2, 0x20, 1);
                break;
            }
        } else {
        }
        break;
    case 3:
        if (--em->timer <= 0) {
            em->timer = 0x12C;
            if (em->remote == 0) {
                EmKhezu_ChangeState(em, 2, 0x20, 1);
            }
        }
        break;
    }
}

void EmKhezu_State2_22(Em *em, EmWork *work)
{
    em->ceilingTimer = 5;
    em->posY = em->surfaceY;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        Em_StartTurnToTarget(em, &EmKhezu_TurnParams[5]);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 8.0f) != 0) {
            em->step++;
        case 2:
            if (Em_TurnStep(em, &EmKhezu_TurnParams[5]) == 1) {
                em->step++;
                Player_ClearFlag(em, 0x20000);
                em->timer = 0x12C;
                EmKhezu_ChangeState(em, 2, 0x1C, 1);
                break;
            }
        } else {
        }
        break;
    case 3:
        if (--em->timer <= 0) {
            em->timer = 0x12C;
            if (em->remote == 0) {
                EmKhezu_ChangeState(em, 2, 0x1C, 1);
            }
        }
        break;
    }
}

void EmKhezu_State3_00(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x6A, 0, 0);
        EmKhezu_StartCurves(em, 0xE);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 78.0f, 0U) != 0) {
            em->step++;
            em->stance = 2;
            EmKhezu_UpdateCurves(em);
        }
        break;
    case 2:
        if ((EmKhezu_UpdateCurves(em) & 0xFF) && (em->posY <= em->floorY)) {
            em->step++;
            em->stance = 0;
            Em_SetMotion(em, 0x6B, 0, 0);
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

void EmKhezu_State3_01(Em *em, EmWork *work)
{
    f32 pos[3];

    em->ceilingTimer = 5;
    em->posY = em->surfaceY;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x6E, 0, 0);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            Vec_Copy(pos, &em->posX);
            pos[1] = em->floorY;
            EmMine_CreateTimed0(em, pos);
            em->step++;
            Em_SetMotion(em, 0x6F, 0, 0);
        }
        break;
    case 2:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmKhezu_EndState2To2_19(em);
        }
        break;
    }
}

void EmKhezu_State3_02(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x70, 0, 0);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmKhezu_EndState(em);
        }
        break;
    }
}

void EmKhezu_State3_03(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x23, 0, 0);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmKhezu_EndState(em);
        }
        break;
    }
}

void EmKhezu_State3_04(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x71, 0, 0);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmKhezu_EndState(em);
        }
        break;
    }
}

void EmKhezu_State3_05(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x2F, 0, 0);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 176.0f, 0U) != 0) {
            EffEm08_CreateAtBone(em, 0x22, 6, 0, 0, 0);
            if (em->stateRand & 0x30) {
                EffEm08_CreateAtBone(em, 0x22, 6, 0, 0, 0xE39);
                EffEm08_CreateAtBone(em, 0x22, 6, 0, 0, 0xF1C8);
            } else {
                EffEm08_CreateAtBone(em, 0x22, 6, 0, 0, 0x71C);
                EffEm08_CreateAtBone(em, 0x22, 6, 0, 0, 0xF8E5);
            }
        }
        if (Player_PassedFrameScaled(em, 254.0f, 0U) != 0) {
            em->step++;
            Em_SetMotion(em, 0x73, 0, 0);
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

void EmKhezu_State3_06(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x2B, 0, 0);
        break;
    case 1:
        em->rotY -= 0x200;
        if (em->motion[1].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x2B, 0, 0);
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

void EmKhezu_State3_07(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x72, 0, 0);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x73, 0, 0);
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

void EmKhezu_State3_08(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x6C, 0, 0);
        EmKhezu_StartCurves(em, 0xF);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 30.0f, 0U) != 0) {
            em->step++;
            em->stance = 2;
            EmKhezu_UpdateCurves(em);
        }
        break;
    case 2:
        if ((EmKhezu_UpdateCurves(em) & 0xFF) && (em->posY <= em->floorY)) {
            em->step++;
            em->stance = 0;
            Em_SetMotion(em, 0x6B, 0, 0);
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

void EmKhezu_State4_00(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x3C, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKhezu_EndState(em);
        }
        break;
    }
}

void EmKhezu_State4_01(Em *em)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        Em_SetMotion(em, 0x6D, 0, 0);
        Player_ClearVelocity(em);
        em->velY = -50.0f;
        em->accelY = -5.0f;
        Em_ClearTilt(em, 1);
        em->ceilingTimer = 5;
        Em_RestartScript(em);
        break;
    case 1:
        em->ceilingTimer = 5;
        if (Player_PassedFrameScaled(em, 12.0f, 0U) != 0) {
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
            em->stance = 0;
            Em_SetMotion(em, 0x75, 0, 0);
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

void EmKhezu_State4_02(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x3F, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKhezu_EndState(em);
        }
        break;
    }
}

void EmKhezu_State4_03(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x40, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKhezu_EndState(em);
        }
        break;
    }
}

void EmKhezu_State4_04(Em *em, EmWork *work, u8 variant)
{
    s32 k;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        k = variant;
        if ((k & 1) == 0) {
            Em_SetMotion(em, 0x4A, 0, 0);
        } else {
            Em_SetMotion(em, 0x45, 0, 0);
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
                Em_SetMotion(em, 0x4B, 0, 0);
            } else {
                Em_SetMotion(em, 0x46, 0, 0);
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
                Em_SetMotion(em, 0x48, 0, 0);
            } else {
                Em_SetMotion(em, 0x47, 0, 0);
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
                EmKhezu_ChangeState(em, 0, 0x22, 4);
                break;
            default:
                EmKhezu_EndState(em);
                break;
            }
        }
        break;
    case 4:
        if (em->remote == 0) {
            EmKhezu_ChangeState(em, 0, 0x22, 4);
        }
        break;
    }
}

void EmKhezu_State4_08(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x4D, 0, 0);
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
            em->stance = 0;
            Em_SetMotion(em, 0x4E, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x47, 0, 0);
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

void EmKhezu_State4_0A(Em *em)
{
    em->holdTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x60, 0, 0);
        em->noReact = 1;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            Em_SetMotion(em, 0x60, 0, 0);
        }
        if (--em->trapTimer <= 0) {
            em->step++;
            em->unk8AD = 0;
            Em_SetMotion(em, 0x60, 0, 0);
            EmKhezu_ChangeState(em, 2, 0x17, 4);
        }
        break;
    case 2:
        if ((em->motion[0].playing == 0) && (em->remote == 0)) {
            Em_SetMotion(em, 0x60, 0, 0);
            EmKhezu_ChangeState(em, 2, 0x17, 4);
        }
        break;
    }
}

void EmKhezu_State4_0B(Em *em)
{
    Em_SetHitReactMode(em, 4U);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x4C, 0, 0);
        em->noReact = 1;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmKhezu_ChangeState(em, 0, 0x16, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmKhezu_ChangeState(em, 0, 0x16, 4);
        }
        break;
    }
    Em_SpawnEffect06Bone(em, 2);
}

void EmKhezu_State4_0C(Em *em)
{
    Em_SetHitReactMode(em, 3U);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x5F, 0, 0);
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
            EmKhezu_ChangeState(em, 4, 0xD, 3);
        }
        break;
    case 2:
        if ((s8) em->holdTimer < 5) {
            em->holdTimer = 5;
        }
        if (em->remote == 0) {
            EmKhezu_ChangeState(em, 4, 0xD, 3);
        }
        break;
    }
}

void EmKhezu_State4_0E(Em *em, EmWork *work, s32 variant)
{
    s32 k;

    k = variant & 0xFF;
    if (k == 0) {
        Em_SetHitReactMode(em, 4U);
    }
    em->holdTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x61, 0, 0);
        em->noReact = 1;
        Em_RestartScript(em);
        if (k == 1) {
            StageObj_Stun_CreateEm(em, 1, 0x22, (s32) ((f32) em->timer + (em->motion[0].frameCount / 2.0f)), 150.0f, 3.0f);
        }
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetGoal12(em);
            if (k == 1) {
                EmKhezu_ChangeState(em, 4, 0x15, 3);
            } else {
                EmKhezu_ChangeState(em, 4, 0xA, 3);
            }
        }
        break;
    case 2:
        if (em->remote == 0) {
            if (k == 1) {
                EmKhezu_ChangeState(em, 4, 0x15, 3);
            } else {
                EmKhezu_ChangeState(em, 4, 0xA, 3);
            }
        }
        break;
    }
    switch (k) {
    default:
        Em_SpawnEffect06Bone(em, 2);
        break;
    case 1:
        break;
    }
}

void EmKhezu_State4_0F(Em *em)
{
    em->holdTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x62, 0, 0);
        em->blind = 2;
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            Em_SetGoal12(em);
            EmKhezu_ChangeState(em, 4, 0xD, 3);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmKhezu_ChangeState(em, 4, 0xD, 3);
        }
        break;
    }
}

void EmKhezu_State4_11(Em *em)
{
    Em_SetHold2(em);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x3C, 0, 0);
        Player_SetFlag((Player *) em, 0x20000U);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 52.0f) != 0) {
            em->step++;
            Em_SetMotion(em, 0x4C, 0xC, 0);
            Player_ClearFlag(em, 0x20000);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            EmKhezu_ChangeState(em, 0, 0x21, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmKhezu_ChangeState(em, 0, 0x21, 4);
        }
        break;
    }
}

void EmKhezu_State6_00(Em *em, EmWork *work)
{
    f32 d;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        GmEm_ResetSize(em);
        em->step++;
        em->stance = 0;
        em->posX = 12220.0f;
        em->posY = 0.0f;
        em->posZ = 10119.0f;
        em->goal[0] = 9290.0f;
        em->goal[1] = 0.0f;
        em->goal[2] = 10238.0f;
        em->rotX = 0;
        em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        em->rotZ = 0;
        Em_SetMotion(em, 1, 0, 0);
        em->timer = 180;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            em->timer = 300;
            Em_SetMotion(em, 3, 0, 0);
        }
        break;
    case 2:
        d = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer <= 0 || d <= 200.0f) {
            em->step++;
            em->rotY = 0x1800;
            Em_SetMotion(em, 0x69, 0, 0);
        }
        break;
    case 3:
        if (Player_PassedFrameScaled(em, 246.0f, 0U) != 0) {
            em->step++;
            Em_SetMotion(em, 0x6A, 0, 0);
            EmKhezu_StartCurves(em, 0xE);
            em->stance = 0;
        }
        break;
    case 4:
        if (Player_PassedFrameScaled(em, 78.0f, 0U) != 0) {
            em->step++;
            em->stance = 2;
            EmKhezu_UpdateCurves(em);
        }
        break;
    case 5:
        if (EmKhezu_UpdateCurves(em) != 0 && em->posY <= em->floorY) {
            em->step++;
            em->stance = 0;
            em->posX = 9700.0f;
            em->posY = 0.0f;
            em->posZ = 10855.0f;
            Em_SetMotion(em, 0x6B, 0, 0);
        }
        break;
    case 6:
        if (Player_PassedFrameScaled(em, 150.0f, 0U) != 0) {
            em->step++;
            Em_SetMotion(em, 0x72, 0, 0);
        }
        break;
    case 7:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x73, 0, 0);
        }
        break;
    case 8:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 1, 0, 0);
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

void EmKhezu_State6_01_Captured(Em *em, EmWork *work)
{
    f32 pos[3];

    Em_SetCaptured(em, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x61, 0, 0);
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
                EmKhezu_ChangeState(em, 5, 3, 4);
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
        Em_SpawnEffect06Periodic(em, 0x22, pos, 1.6f);
        if (++em->timer % 135 == 0) {
            Em_PlaySeAtBone(em, 0x57, 0x23);
        }
    }
}

void EmKhezu_State6_05_Captured(Em *em, EmWork *work)
{
    s32 timer;
    s32 t;

    Em_SetCaptured(em, 1);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x26, 0, 0);
        Em_LoadReactionSet(em, 1);
        Em_BeginStatus1(em);
        Em_ClearPoison(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x22, 0, 0);
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
        t = em->timer;
        if (t > 0) {
            em->timer = t - 1;
        }
        timer = em->timer;
        if (timer <= 0) {
            if (em->remote == 0) {
                EmKhezu_ChangeState(em, 5, 3, 4);
                break;
            }
        } else {
            em->alpha = (f32) timer / 150.0f;
        }
        break;
    }
}

void EmKhezu_State8_00_CarveStart(Em *em, EmWork *work)
{
    EmKhezuVec v;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        if (Versus_DefeatCamera(em) == 1) {
            v = EmKhezu_CarveThrowPt;
            StageObj_K25_Create(&gPlayers[(u8) em->side - 1], em, 0, 0, &v.x);
            em->step++;
            Em_SetMotion(em, 0x4F, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x3B, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 30;
            Em_SetMotion(em, 0x33, 0, 0);
        }
        break;
    case 3:
        if (--em->timer <= 0) {
            Versus_RouteEmByTeam(em);
        }
        break;
    }
}

void EmKhezu_State8_01(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x3C, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 1, 0, 0);
            EmKhezu_ChangeState(em, 8, 0x98, 1);
        }
        break;
    }
}

void EmKhezu_State8_02(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x69, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 1, 0, 0);
            EmKhezu_ChangeState(em, 8, 0x98, 1);
        }
        break;
    }
}

void EmKhezu_State8_03(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x72, 0, 0);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x73, 0, 0);
        }
        break;
    case 2:
        if (em->motion[1].playing == 0) {
            em->step++;
            Em_SetMotion(em, 1, 0, 0);
            EmKhezu_ChangeState(em, 8, 0x98, 1);
        }
        break;
    }
}

void EmKhezu_State8_98(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        if (em->motionId[0] != 0x3E9) {
            Em_SetMotion(em, 1, 0, 0);
        }
        
    case 1:
        if (Versus_BothDefeated() == 1) {
            em->step++;
            EmKhezu_ChangeState(em, 8, 0x99, 1);
        }
        break;
    }
}

void EmKhezu_State8_99(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 0x5A;
        if (em->motionId[0] != 0x3E9) {
            Em_SetMotion(em, 1, 0, 0);
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

void EmKhezu_State5_00_Hunted(Em *em, EmWork *work)
{
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x44, 0, 0);
        em->lookAtTarget = 0;
        em->stance = 3;
        QuestEm_Hunted(em);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 212.0f, 0) != 0) {
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
                EmKhezu_ChangeState(em, 5, 3, 4);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    case 99:
        if (em->motion[0].playing == 0) {
            em->stance = 0;
            EmKhezu_EndState(em);
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
        }
        break;
    }
}

void EmKhezu_State5_01_Hunted(Em *em, EmWork *work)
{
    em->holdTimer = 5;
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x61, 0, 0);
        em->lookAtTarget = 0;
        em->stance = 3;
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
                EmKhezu_ChangeState(em, 5, 3, 4);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    case 99:
        if (em->motion[0].playing == 0) {
            em->stance = 0;
            EmKhezu_EndState(em);
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
        }
        break;
    }
}

void EmKhezu_State5_02_HuntedFalling(Em *em, EmWork *work)
{
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x4D, 0, 0);
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
            em->stance = 0;
            Em_SetMotion(em, 0x4E, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    case 2:
        if (Player_PassedFrameScaled(em, 60.0f, 0) != 0) {
            em->step++;
            em->timer = 0;
            Em_SetMotion(em, 0x52, 0, 0);
            em->stance = 3;
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
                EmKhezu_ChangeState(em, 5, 3, 4);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    case 99:
        if (em->motion[0].playing == 0) {
            em->stance = 0;
            EmKhezu_EndState(em);
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
        }
        break;
    }
}

void EmKhezu_State5_03_Appear(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        if (QuestEm_CanAppear(em) == 1) {
            em->posY = gAreaGroundY[em->area];
            em->stance = 2;
            EmKhezu_ChangeState(em, 2, 1, 0);
            break;
        }
        em->taskPhase++;
        break;
    }
}

void EmKhezu_RunGroup0(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmKhezu_State0_00(em, work);
        break;
    case 1:
        EmKhezu_State0_01(em, work);
        break;
    case 2:
        EmKhezu_State0_02(em, work);
        break;
    case 3:
        EmKhezu_State0_03(em, work);
        break;
    case 4:
        EmKhezu_State0_04(em, work);
        break;
    case 5:
        EmKhezu_State0_05(em, work);
        break;
    case 6:
        EmKhezu_State0_06(em, work);
        break;
    case 7:
        EmKhezu_State0_07(em, work);
        break;
    case 8:
        EmKhezu_State0_08(em, work);
        break;
    case 10:
        EmKhezu_State0_0A(em, work);
        break;
    case 13:
        EmKhezu_State0_0D(em, work);
        break;
    case 14:
        EmKhezu_State0_0E(em, work);
        break;
    case 17:
        EmKhezu_State0_11(em, work);
        break;
    case 18:
        EmKhezu_State0_12(em, work);
        break;
    case 19:
        EmKhezu_State0_13(em, work);
        break;
    case 20:
        EmKhezu_State0_14(em, work);
        break;
    case 21:
        EmKhezu_State0_15(em, work);
        break;
    case 22:
        EmKhezu_State0_16(em, work);
        break;
    case 23:
        EmKhezu_State0_17(em, work);
        break;
    case 24:
        EmKhezu_State0_18(em, work);
        break;
    case 25:
        EmKhezu_State0_16(em, work);
        break;
    case 26:
        EmKhezu_State0_1A(em, work);
        break;
    case 27:
        EmKhezu_State0_1B(em, work);
        break;
    case 28:
        EmKhezu_State0_18(em, work);
        break;
    case 29:
        EmKhezu_State0_1D(em, work);
        break;
    case 31:
        EmKhezu_State0_1F(em, work);
        break;
    case 33:
        EmKhezu_State0_16(em, work);
        break;
    case 34:
        EmKhezu_State0_16(em, work);
        break;
    case 40:
        EmKhezu_State0_28(em, work);
        break;
    case 41:
        EmKhezu_State0_11(em, work);
        break;
    case 42:
        EmKhezu_State0_11(em, work);
        
    default:
        break;
    }
}

void EmKhezu_RunGroup1(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmKhezu_State1_00(em, work);
        break;
    case 3:
        EmKhezu_State1_03(em, work);
        break;
    case 5:
        EmKhezu_State1_05(em, work);
        break;
    case 6:
        EmKhezu_State1_06(em, work);
        break;
    case 7:
        EmKhezu_State1_07(em, work);
        break;
    }
}

void EmKhezu_RunGroup2(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmKhezu_State2_00(em, work);
        break;
    case 1:
        EmKhezu_State2_01_FlyInLand(em, work);
        break;
    case 2:
        EmKhezu_State2_02(em, work);
        break;
    case 3:
        EmKhezu_State2_03(em, work);
        break;
    case 4:
        EmKhezu_State2_04(em, work);
        break;
    case 5:
        EmKhezu_State2_05(em, work);
        break;
    case 6:
        EmKhezu_State2_06(em, work);
        break;
    case 7:
        EmKhezu_State2_07(em, work);
        break;
    case 8:
        EmKhezu_State2_08(em, work);
        break;
    case 9:
        EmKhezu_State2_09(em, work);
        break;
    case 11:
        EmKhezu_State2_0B(em, work);
        break;
    case 12:
        EmKhezu_State2_0C(em, work);
        break;
    case 13:
        EmKhezu_State2_0D(em, work);
        break;
    case 14:
        EmKhezu_State2_0E(em, work);
        break;
    case 15:
        EmKhezu_State2_0F(em, work);
        break;
    case 16:
        EmKhezu_State2_10(em, work);
        break;
    case 17:
        EmKhezu_State2_11();
        break;
    case 18:
        EmKhezu_State2_12();
        break;
    case 19:
        EmKhezu_State2_13();
        break;
    case 20:
        EmKhezu_State2_14(em, work);
        break;
    case 21:
        EmKhezu_State2_15(em, work);
        break;
    case 22:
        EmKhezu_State2_16();
        break;
    case 23:
        EmKhezu_State2_17(em, work);
        break;
    case 24:
        EmKhezu_State2_18(em, work);
        break;
    case 25:
        EmKhezu_State2_19(em, work);
        break;
    case 26:
        EmKhezu_State2_1A(em, work);
        break;
    case 27:
        EmKhezu_State2_1B(em, work);
        break;
    case 28:
        EmKhezu_State2_1C(em, work);
        break;
    case 29:
        EmKhezu_State2_1D(em, work);
        break;
    case 30:
        EmKhezu_State2_1E(em, work);
        break;
    case 31:
        EmKhezu_State2_1F(em, work);
        break;
    case 32:
        EmKhezu_State2_20(em, work);
        break;
    case 33:
        EmKhezu_State2_21(em, work);
        break;
    case 34:
        EmKhezu_State2_22(em, work);
        
    default:
        break;
    }
}

void EmKhezu_RunGroup3(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmKhezu_State3_00(em, work);
        break;
    case 1:
        EmKhezu_State3_01(em, work);
        break;
    case 2:
        EmKhezu_State3_02(em, work);
        break;
    case 3:
        EmKhezu_State3_03(em, work);
        break;
    case 4:
        EmKhezu_State3_04(em, work);
        break;
    case 5:
        EmKhezu_State3_05(em, work);
        break;
    case 6:
        EmKhezu_State3_06(em, work);
        break;
    case 7:
        EmKhezu_State3_07(em, work);
        break;
    case 8:
        EmKhezu_State3_08(em, work);
        
    default:
        break;
    }
}

void EmKhezu_RunGroup4(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmKhezu_State4_00(em);
        break;
    case 1:
        EmKhezu_State4_01(em);
        break;
    case 2:
        EmKhezu_State4_02(em);
        break;
    case 3:
        EmKhezu_State4_03(em);
        break;
    case 4:
        EmKhezu_State4_04(em, work, 0);
        break;
    case 5:
        EmKhezu_State4_04(em, work, 1);
        break;
    case 8:
        EmKhezu_State4_08(em);
        break;
    case 0xA:
        EmKhezu_State4_0A(em);
        break;
    case 0xB:
        EmKhezu_State4_0B(em);
        break;
    case 0xC:
        EmKhezu_State4_0C(em);
        break;
    case 0xD:
        EmKhezu_State4_0A(em);
        break;
    case 0xE:
        EmKhezu_State4_0E(em, work, 0);
        break;
    case 0xF:
        EmKhezu_State4_0F(em);
        break;
    case 0x10:
        EmKhezu_State4_0F(em);
        break;
    case 0x11:
        EmKhezu_State4_11(em);
        break;
    case 0x12:
        EmKhezu_State4_04(em, work, 2);
        break;
    case 0x13:
        EmKhezu_State4_04(em, work, 3);
        break;
    case 0x14:
        EmKhezu_State4_0E(em, work, 1);
        break;
    case 0x15:
        EmKhezu_State4_0A(em);
        break;
    }
}

void EmKhezu_RunGroup5(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmKhezu_State5_00_Hunted(em, work);
        break;
    case 1:
        EmKhezu_State5_01_Hunted(em, work);
        break;
    case 2:
        EmKhezu_State5_02_HuntedFalling(em, work);
        break;
    case 3:
        EmKhezu_State5_03_Appear(em, work);
        break;
    }
}

void EmKhezu_RunGroup6(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmKhezu_State6_00(em, work);
        break;
    case 1:
        EmKhezu_State6_01_Captured(em, work);
        break;
    case 2:
        EmKhezu_State6_01_Captured(em, work);
        break;
    case 3:
        EmKhezu_State6_01_Captured(em, work);
        break;
    case 4:
        EmKhezu_State6_01_Captured(em, work);
        break;
    case 5:
        EmKhezu_State6_05_Captured(em, work);
        
    default:
        break;
    }
}

void EmKhezu_RunGroup8(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0x0:
        EmKhezu_State8_00_CarveStart(em, work);
        break;
    case 0x1:
        EmKhezu_State8_01(em, work);
        break;
    case 0x2:
        EmKhezu_State8_02(em, work);
        break;
    case 0x3:
        EmKhezu_State8_03(em, work);
        break;
    case 0x98:
        EmKhezu_State8_98(em, work);
        break;
    case 0x99:
        EmKhezu_State8_99(em, work);
        break;
    }
}

void EmKhezu_Update(Em *em)
{
    u8 buf[4];
    EmKhezuWork *w = (EmKhezuWork *)&em->work;
    s32 ev;

    em->moveFrozen = 0;
    if (gScene.area == em->area) {
        em->hasCeiling = AreaGrid2_GetCeiling(&em->posX, &em->surfaceY, (struct AreaTri *)((u8 *)w + 0x40));
    } else if (EmKhezu_InSpecialArea(em->area) == 1) {
        em->hasCeiling = 1;
        em->surfaceY = 1500.0f;
        if (em->stateGroup == 0 || em->stateGroup == 3) {
            Em_HoldPosNoPlayers(em, 0, 1);
        }
    } else {
        em->hasCeiling = 0;
    }
    Em_RaiseTrigger80Lax(em);
    Em_UpdateCombatTimer(em);
    if (em->noAlert != 1) {
        switch (Area_BaseId(gScene.map)) {
        case 0xB:
            Em_DecayNeed1Only(em);
            Em_CheckLowHp(em, 0.2f);
            break;
        case 4:
            Em_DecayNeed1(em);
            Em_CheckLowHp(em, 0.2f);
            break;
        }
    }
    ev = Em_UpdateDamage(em, buf) & 0xFF;
    if (ev != 0 && ev != 0xE) {
        w->unk044 = 0;
        w->unk045 = 0;
    }
    switch (ev) {
    case 0:
    case 9:
    case 11:
    case 14:
    case 15:
    case 16:
        break;
    case 1:
    case 2:
        if (em->stance == 2) {
            EmKhezu_ChangeState(em, 5, 2, 3);
        } else if ((s8) em->holdTimer != 0) {
            EmKhezu_ChangeState(em, 5, 1, 3);
        } else {
            EmKhezu_ChangeState(em, 5, 0, 3);
        }
        break;
    case 3:
    case 4:
        if ((s8) em->holdTimer == 0) {
            Player_ClearFlag(em, 0x20000);
            Em_SetGoal12Dist(em, buf);
            Em_SetGoal12(em);
            EmKhezu_ChangeState(em, 4, 0xC, 3);
        }
        break;
    case 5:
        if ((em->stateGroup != 4 || em->state != 8) && (em->stateGroup != 4 || em->state != 1)) {
            if ((s8) em->ceilingTimer != 0) {
                EmKhezu_ChangeState(em, 4, 1, 3);
            } else {
                EmKhezu_ChangeState(em, 4, 8, 3);
            }
        }
        break;
    case 6:
        if ((s8) em->holdTimer != 0) {
            Em_SetStatus4Timer(em);
            EmKhezu_ChangeState(em, 4, 0xE, 3);
        } else if ((em->stateGroup != 4 || em->state != 1) && (em->stateGroup != 4 || em->state != 0xB) &&
                   (em->stateGroup != 4 || em->state != 8)) {
            Em_SetStatus4Timer(em);
            EmKhezu_ChangeState(em, 4, 0xB, 3);
        }
        break;
    case 7:
        if (Em_IsHeld(em) == 1) {
            Em_SetStatus1Timer(em);
            if ((s8) em->holdTimer != 0) {
                Em_SetStatus1Timer(em);
                EmKhezu_ChangeState(em, 6, 4, 3);
                break;
            }
            if ((s8) em->holdTimer2 != 0) {
                EmKhezu_ChangeState(em, 6, 5, 3);
                break;
            }
        }
        if ((s8) em->holdTimer != 0) {
            Em_SetStatus1Timer(em);
            EmKhezu_ChangeState(em, 6, 4, 3);
        } else if ((em->stateGroup != 0 || em->state != 0x1B) && (em->stateGroup != 4 || em->state != 1) &&
                   (em->stateGroup != 4 || em->state != 8)) {
            Em_SetStatus1Timer(em);
            EmKhezu_ChangeState(em, 0, 0x1B, 3);
        }
        break;
    case 8:
        if ((s8) em->holdTimer != 0) {
            Em_SetStatus2Timer(em);
            EmKhezu_ChangeState(em, 0, 0x1D, 3);
        } else if ((s8) em->holdTimer2 != 0) {
            Em_SetStatus2Timer(em);
            EmKhezu_ChangeState(em, 0, 0x14, 3);
        } else if ((em->stateGroup != 0 || em->state != 0x14) && (em->stateGroup != 4 || em->state != 1) &&
                   (em->stateGroup != 4 || em->state != 8)) {
            Em_SetStatus2Timer(em);
            EmKhezu_ChangeState(em, 0, 0x14, 3);
        }
        break;
    case 10:
        switch (em->state) {
        case 0x12:
            EmKhezu_ChangeState(em, 0, 0x17, 3);
            em->scriptPending = 0;
            break;
        case 0x14:
            EmKhezu_ChangeState(em, 0, 0x18, 3);
            em->scriptPending = 0;
            break;
        case 0x15:
            EmKhezu_ChangeState(em, 0, 0x18, 3);
            em->scriptPending = 0;
            break;
        case 0x1B:
            EmKhezu_ChangeState(em, 0, 0x1C, 3);
            em->scriptPending = 0;
            break;
        case 0x1D:
            EmKhezu_ChangeState(em, 4, 0xF, 3);
            em->scriptPending = 0;
            break;
        case 0x1F:
            EmKhezu_ChangeState(em, 4, 0x10, 3);
            em->scriptPending = 0;
            break;
        }
        break;
    case 12:
        Player_ClearFlag(em, 0x20000);
        if (em->stance == 2) {
            if ((s8) em->ceilingTimer != 0) {
                EmKhezu_ChangeState(em, 4, 1, 3);
            } else {
                EmKhezu_ChangeState(em, 4, 8, 3);
            }
        } else {
            switch (em->flinchPart) {
            case 0:
            case 7:
                EmKhezu_ChangeState(em, 4, 0, 3);
                break;
            case 5:
            case 6:
                EmKhezu_ChangeState(em, 4, 2, 3);
                break;
            case 1:
            case 2:
                EmKhezu_ChangeState(em, 4, 3, 3);
                break;
            case 3:
            case 4:
            default:
                if (em->flinchPart != 3) {
                    EmKhezu_ChangeState(em, 4, 5, 3);
                } else {
                    EmKhezu_ChangeState(em, 4, 4, 3);
                }
                break;
            }
        }
        break;
    case 13:
        if (em->stance != 2) {
            EmKhezu_ChangeState(em, 4, 0, 3);
            em->scriptPending = 0;
        }
        break;
    case 17:
        if ((s8) em->holdTimer != 0) {
            Em_SetStatus5Timer(em);
            EmKhezu_ChangeState(em, 4, 0x14, 3);
        } else if ((em->stateGroup != 4 || em->state != 0x12) && (em->stateGroup != 4 || em->state != 0x13) &&
                   (em->stateGroup != 4 || em->state != 8) && (em->stateGroup != 4 || em->state != 1)) {
            Em_SetStatus5Timer(em);
            if (((em->hitAngle - em->rotY) & 0xFFFF) > 0x8000) {
                EmKhezu_ChangeState(em, 4, 0x13, 3);
            } else {
                EmKhezu_ChangeState(em, 4, 0x12, 3);
            }
        }
        break;
    case 18:
        Em_ResetKindTimer(em);
        EmKhezu_ChangeState(em, 4, 0x11, 3);
        break;
    }
    if (em->parts[0].state >= 2 && em->parts[6].state >= 2) {
        Em_BreakPart(em, 1);
    }
    switch (gQuest.stage) {
    case 0x422:
        if ((em->type == 0xF || em->type == 0x2D) && Area_DayId(em->area) == 0x5E && Save_TestEventFlag(0xA7) == 0) {
            if (Area_DayId(gScene.area) == 0x5E && gScene.eventRunning == 1 && em->stateGroup != 6) {
                EmKhezu_ChangeState(em, 6, 0, 1);
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

void EmKhezu_RunState(Em *em, EmWork *work)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->stateGroup) {
    case 0:
        EmKhezu_RunGroup0(em, work);
        break;
    case 1:
        EmKhezu_RunGroup1(em, work);
        break;
    case 2:
        EmKhezu_RunGroup2(em, work);
        break;
    case 3:
        EmKhezu_RunGroup3(em, work);
        break;
    case 4:
        EmKhezu_RunGroup4(em, work);
        break;
    case 5:
        EmKhezu_RunGroup5(em, work);
        break;
    case 6:
        EmKhezu_RunGroup6(em, work);
        break;
    case 7:
        EmKhezu_RunGroup6(em, work);
        break;
    case 8:
        EmKhezu_RunGroup8(em, work);
        
    default:
        break;
    }
}

void EmKhezu_RequestOnFrame(s32 em, s32 frame, s32 i)
{
    if (Player_PassedFrameScaled((Em *) em, (f32) frame, 0U) != 0) {
        CamShake_ReqAIf((void *) em, i);
    }
}

static void EmKhezu_MotionEffectsDefault(Em *em)
{
}


void EmKhezu_MotionEffects(Em *em, EmKhezuWork *work)
{
    f32 v1[3];
    f32 v0[3];

    if (em->motionId[0] != work->fxMotion) {
        work->fxMotion = em->motionId[0];
    }
    EmKhezu_MotionSounds(em, (struct EmWork *)work);
    switch (work->fxMotion) {
    case 0x3E9:
        break;
    case 0x3EB:
        EmKhezu_RequestOnFrame((s32) em, 0x34, 1);
        EmKhezu_RequestOnFrame((s32) em, 0x74, 1);
        if (Player_PassedFrameScaled(em, 2.0f, 0) != 0) {
            EffKhezu16_Create(em, 0x14);
        }
        if (Player_PassedFrameScaled(em, 50.0f, 0) != 0) {
            EffKhezu16_Create(em, 0x15);
        }
        if (Player_PassedFrameScaled(em, 104.0f, 0) != 0) {
            EffKhezu16_Create(em, 0x16);
        }
        if (Player_PassedFrameScaled(em, 162.0f, 0) != 0) {
            EffKhezu16_Create(em, 0x15);
        }
        if (Player_PassedFrameScaled(em, 218.0f, 0) != 0) {
            EffKhezu16_Create(em, 0x17);
            break;
        }
        break;
    case 0x3ED:
        EmKhezu_RequestOnFrame((s32) em, 0x3C, 1);
        if (Player_PassedFrameScaled(em, 2.0f, 0) != 0) {
            EffKhezu16_Create(em, 5);
        }
        if (Player_PassedFrameScaled(em, 30.0f, 0) != 0) {
            EffKhezu16_Create(em, 6);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 2.0f, 78.0f) != 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create(em, 0x1A, 1, 1.0f);
            break;
        }
        break;
    case 0x3EE:
        EmKhezu_RequestOnFrame((s32) em, 0x3C, 1);
        if (Player_PassedFrameScaled(em, 30.0f, 0) != 0) {
            EffKhezu16_Create(em, 7);
        }
        if (Player_PassedFrameScaled(em, 2.0f, 0) != 0) {
            EffKhezu16_Create(em, 8);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 2.0f, 78.0f) != 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create(em, 0x1A, 0, 1.0f);
            break;
        }
        break;
    case 0x3EF:
        if (Player_PassedFrameScaled(em, 18.0f, 0) != 0) {
            EffKhezu16_Create(em, 9);
        }
        if (Player_PassedFrameScaled(em, 2.0f, 0) != 0) {
            EffKhezu16_Create(em, 0xA);
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
    case 0x3F0:
        if (Player_PassedFrameScaled(em, 2.0f, 0) != 0) {
            EffKhezu16_Create(em, 0xB);
        }
        if (Player_PassedFrameScaled(em, 18.0f, 0) != 0) {
            EffKhezu16_Create(em, 0xC);
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
    case 0x3F2:
        if (Player_PassedFrameScaled(em, 42.0f, 0) != 0) {
            EffKhezu16_Create(em, 0x19);
        }
        if (Player_PassedFrameScaled(em, 2.0f, 0) != 0) {
            EffKhezu16_Create(em, 0x1A);
        }
        if (Player_PassedFrameScaled(em, 74.0f, 0) != 0) {
            EffKhezu16_Create(em, 0x1A);
            break;
        }
        break;
    case 0x3F3:
        if (Player_PassedFrameScaled(em, 52.0f, 0) || Player_PassedFrameScaled(em, 104.0f, 0) || Player_PassedFrameScaled(em, 162.0f, 0)) {
            if (em->posY <= (1000.0f + em->floorY)) {
                Effect0D_SpawnForEmScaled(em, 2, 7, 5.0f);
                break;
            }
        }
        break;
    case 0x3F7:
        if (Player_PassedFrameScaled(em, 8.0f, 0) || Player_PassedFrameScaled(em, 86.0f, 0) || Player_PassedFrameScaled(em, 160.0f, 0)) {
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
    case 0x3FA:
        if (Player_PassedFrameScaled(em, 42.0f, 0) || Player_PassedFrameScaled(em, 102.0f, 0) || Player_PassedFrameScaled(em, 156.0f, 0)) {
            if (em->posY <= (1000.0f + em->floorY)) {
                StageObj_K14_Create(em, 1, 0, 1.0f);
            }
        }
        if ((em->rage != 0) && (Player_PassedFrameScaled(em, 38.0f, 0) != 0)) {
            StageObj_K14_Create(em, 0x19, 5, 1.0f);
            StageObj_K14_Create(em, 0x19, 4, 1.0f);
            break;
        }
        break;
    case 0x3FB:
        if (Player_PassedFrameScaled(em, 2.0f, 0) != 0) {
            EffKhezu16_Create(em, 0xD);
        }
        if (Player_PassedFrameScaled(em, 16.0f, 0) != 0) {
            EmKhezu_SpawnK14Type0B(em);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 18.0f, 44.0f) != 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
            break;
        }
        break;
    case 0x407:
        v1[1] = 10.0f;
        v1[2] = 140.0f;
        v1[0] = 0.0f;
        Em_SpawnEffect06Periodic(em, 0x22, v1, 1.6f);
        break;
    case 0x408:
    case 0x409:
        break;
    case 0x40A:
        v0[1] = 10.0f;
        v0[2] = 140.0f;
        v0[0] = 0.0f;
        Em_SpawnEffect06Periodic(em, 0x22, v0, 1.6f);
        break;
    case 0x40B:
        if (Player_PassedFrameScaled(em, 40.0f, 0) != 0) {
            EffKhezu16_Create(em, 0xE);
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
    case 0x40D:
        break;
    case 0x40E:
        if (Player_PassedFrameScaled(em, 148.0f, 0) != 0) {
            EffKhezu16_Create(em, 0xF);
            break;
        }
        break;
    case 0x40F:
        break;
    case 0x413:
        EmKhezu_RequestOnFrame((s32) em, 0x2C, 1);
        EmKhezu_RequestOnFrame((s32) em, 0x49, 1);
        if (Player_PassedFrameScaled(em, 30.0f, 0) != 0) {
            EffKhezu16_Create(em, 0x11);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 6.0f, 38.0f) != 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create(em, 0x1A, 1, 1.0f);
            break;
        }
        break;
    case 0x417:
        if (Player_PassedFrameScaled(em, 160.0f, 0) != 0) {
            EffKhezu16_Create(em, 0x25);
        }
        if (Player_PassedFrameScaled(em, 58.0f, 0) != 0) {
            EmFxSpark_SpawnTimed(em, 3, 0x72, 1.0f);
        }
        if (Player_PassedFrameScaled(em, 172.0f, 0) != 0) {
            EmFxSpark_SpawnOnEm(em, 4);
            break;
        }
        break;
    case 0x41B:
        if ((em->rage != 0) && ((EmMotion_InFrameRange(em, 0, 10.0f, 90.0f) != 0) || (EmMotion_InFrameRange(em, 0, 122.0f, 260.0f) != 0) || (EmMotion_ReachedFrame(em, 0, 296.0f) != 0)) && !(gScene.frame & 3)) {
            StageObj_K14_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
            break;
        }
        break;
    case 0x41E:
        if ((em->stateGroup != 6) && (Player_PassedFrameScaled(em, 78.0f, 0) != 0)) {
            EffKhezu16_Create(em, 0x13);
            StageObj_K0F_CreateOnEm(em, 5, 4, 1.0f);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 86.0f, 298.0f) != 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
            break;
        }
        break;
    case 0x423:
        break;
    case 0x424:
        if (Player_PassedFrameScaled(em, 32.0f, 0) != 0) {
            Effect0D_SpawnForEm(em, 0x1B, 7);
        }
        if (Player_PassedFrameScaled(em, 54.0f, 0) != 0) {
            Effect0D_SpawnForEm(em, 0x16, 7);
            break;
        }
        break;
    case 0x425:
        if (Player_PassedFrameScaled(em, 24.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x1A, 7, 1.0f);
            break;
        }
        break;
    case 0x426:
    case 0x427:
    case 0x428:
    case 0x42B:
        break;
    case 0x42C:
        if (Player_PassedFrameScaled(em, 268.0f, 0) != 0) {
            StageObj_K14_Create(em, 0, 0, 0.9f);
            break;
        }
        break;
    case 0x42D:
        EmKhezu_RequestOnFrame((s32) em, 0x1A, 2);
        if (Player_PassedFrameScaled(em, 26.0f, 0) != 0) {
            StageObj_K14_Create(em, 0x12, 0x80, 1.0f);
            break;
        }
        break;
    case 0x42E:
    case 0x433:
    case 0x42F:
    case 0x430:
        break;
    case 0x432:
        EmKhezu_RequestOnFrame((s32) em, 0x1A, 2);
        if (Player_PassedFrameScaled(em, 26.0f, 0) != 0) {
            StageObj_K14_Create(em, 0x12, 0x81, 1.0f);
            break;
        }
        break;
    case 0x434:
    case 0x435:
        break;
    case 0x436:
        if (Player_PassedFrameScaled(em, 8.0f, 0) != 0) {
            StageObj_K14_Create(em, 0xB, 0, 1.0f);
            break;
        }
        break;
    case 0x437:
        if (Player_PassedFrameScaled(em, 48.0f, 0) || Player_PassedFrameScaled(em, 106.0f, 0) || Player_PassedFrameScaled(em, 174.0f, 0)) {
            EffKhezu16_Create(em, 0x10);
            break;
        }
        break;
    case 0x43A:
        break;
    case 0x43C:
        if (Player_PassedFrameScaled(em, 42.0f, 0) || Player_PassedFrameScaled(em, 60.0f, 0) || Player_PassedFrameScaled(em, 80.0f, 0)) {
            StageObj_K14_Create(em, 1, 0, 1.0f);
            break;
        }
        break;
    case 0x442:
        if ((Player_PassedFrameScaled(em, 50.0f, 0) != 0) && (em->posY <= (1000.0f + em->floorY))) {
            StageObj_K14_Create(em, 9, 0, 1.0f);
            StageObj_K14_Create(em, 0xE, 0, 1.0f);
        }
        if ((Player_PassedFrameScaled(em, 102.0f, 0) != 0) && (em->posY <= (1000.0f + em->floorY))) {
            StageObj_K14_Create(em, 9, 0, 1.0f);
            StageObj_K14_Create(em, 0xE, 0, 1.0f);
            break;
        }
        break;
    case 0x447:
        if (Player_PassedFrameScaled(em, 10.0f, 0) != 0) {
            StageObj_K14_Create(em, 0xB, 0x80, 1.0f);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 4.0f, 60.0f) != 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
            break;
        }
        break;
    case 0x448:
        if (em->rage != 0) {
            if (EmMotion_InFrameRange(em, 0, 12.0f, 24.0f) || EmMotion_InFrameRange(em, 0, 46.0f, 90.0f)) {
                if (!(gScene.frame & 3)) {
                    StageObj_K14_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
                    break;
                }
            }
        }
        break;
    case 0x449:
    case 0x44A:
    case 0x44C:
    case 0x44D:
    case 0x44E:
    case 0x44F:
        break;
    case 0x450:
        if (Player_PassedFrameScaled(em, 110.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 2, 7, 3.5f);
            break;
        }
        break;
    case 0x451:
        if (Player_PassedFrameScaled(em, 66.0f, 0) != 0) {
            EffKhezu16_Create(em, 2);
            StageObj_K0F_CreateOnEm(em, 5, 5, 1.0f);
            break;
        }
        break;
    case 0x452:
        if (Player_PassedFrameScaled(em, 80.0f, 0) != 0) {
            EffKhezu16_Create(em, 0x1D);
        }
        if (Player_PassedFrameScaled(em, 70.0f, 0) != 0) {
            StageObj_K14_Create(em, 2, 6, 1.0f);
            break;
        }
        break;
    case 0x453:
        if (Player_PassedFrameScaled(em, 2.0f, 0) != 0) {
            EffKhezu16_Create(em, 0x1E);
        }
        if (Player_PassedFrameScaled(em, 12.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 2, 7, 3.5f);
            break;
        }
        break;
    case 0x454:
        if (Player_PassedFrameScaled(em, 32.0f, 0) != 0) {
            EffKhezu16_Create(em, 0x1F);
        }
        if (Player_PassedFrameScaled(em, 20.0f, 0) != 0) {
            StageObj_K14_Create(em, 2, 6, 1.0f);
            break;
        }
        break;
    case 0x455:
        if (Player_PassedFrameScaled(em, 24.0f, 0) != 0) {
            EffKhezu16_Create(em, 0x22);
            break;
        }
        break;
    case 0x456:
        if ((EmMotion_ReachedFrame(em, 0, 200.0f) != 0) && !(gScene.frame & 0xF)) {
            EffEm08_CreateAtBone(em, 0x22, 5, 0, 0, 0);
            break;
        }
        break;
    case 0x457:
        break;
    case 0x458:
        if (Player_PassedFrameScaled(em, 180.0f, 0) != 0) {
            EffKhezu16_Create(em, 0x18);
        }
        if (Player_PassedFrameScaled(em, 172.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x1A, 3, 3.0f);
        }
        if (Player_PassedFrameScaled(em, 178.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x14, 3, 3.0f);
            break;
        }
        break;
    case 0x459:
        if (em->type == 0xF) {
            if (Player_PassedFrameScaled(em, 66.0f, 0) != 0) {
                EffKhezu16_Create(em, 1);
            }
        } else {
            if (Player_PassedFrameScaled(em, 16.0f, 0) != 0) {
                EffKhezu16_Create(em, 0x27);
            }
            if (Player_PassedFrameScaled(em, 130.0f, 0) != 0) {
                EffKhezu16_Create(em, 0x28);
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
    case 0x45A:
        if (Player_PassedFrameScaled(em, 2.0f, 0) != 0) {
            EmFxSpark_SpawnOnEm(em, 0);
        }
        if (Player_PassedFrameScaled(em, 114.0f, 0) != 0) {
            EffKhezu16_Create(em, 0x1B);
            break;
        }
        break;
    case 0x45B:
        break;
    case 0x45D:
        if (Player_PassedFrameScaled(em, 2.0f, 0) != 0) {
            EffKhezu16_Create(em, 0x1C);
        }
        if (Player_PassedFrameScaled(em, 12.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 2, 7, 3.5f);
            break;
        }
        break;
    case 0x45E:
    case 0x45F:
    case 0x460:
    case 0x461:
        break;
    default:
        EmKhezu_MotionEffectsDefault(em);
        break;
    }
}

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

void EmKhezu_SpawnK14Type0B(Em *em)
{
    StageObj_K14_Create((Em *) em, 0xB, 0, 1.0f);
}

void EmKhezu_TableNop(void)
{
}

s32 EmKhezu_ScriptQuery(Em *em, s32 kind)
{
    s32 k;

    k = kind & 0xFF;
    switch (k) {
    case 0:
        return EmKhezu_InSpecialArea(em->area) == 1;
    case 1:
        return em->hasCeiling != 0;
    default:
        return 0;
    }
}

void EmKhezu_SetGroup0State(Em *em, u16 state, u8 mode)
{
    em->motionSpeed = 1.0f;
    switch (state) {
    case 1:
        if ((em->hp >= (s16) (0.2f * (f32) em->hpMax)) && (em->combat == 1)) {
            state = 0x11;
        }
        break;
    case 7:
        if (em->combat == 1) {
            state = 8;
        }
        break;
    case 18:
        Em_ResetNotice(em);
        break;
    case 20:
        Em_BeginStatus2(em);
        break;
    case 21:
        Em_BeginStatus2(em);
        break;
    case 22:
        Em_EndStatus4(em);
        break;
    case 23:
        Em_RefillNeed3(em);
        break;
    case 24:
        Em_EndStatus2(em);
        break;
    case 27:
        Em_BeginStatus1(em);
        break;
    case 28:
        Em_EndStatus1(em);
        break;
    case 29:
        Em_BeginStatus2(em);
        break;
    case 31:
        Em_BeginStatus1(em);
        break;
    case 33:
        em->holdTimer2 = 0;
        break;
    case 34:
        Em_EndStatus5(em);
        break;
    case 40:
        Em_StartGoalAction(em, 0x2710);
        break;
    case 41:
        Em_StartRage(em);
        break;
    case 42:
        Em_StartPendingRage(em);
        break;
    }
    em->stance = 0;
    Em_SetStateSync(em, 0, state, mode);
}

void EmKhezu_SetGroup1State(Em *em, u16 state, u8 mode)
{
    f32 f;

    em->stance = 0;
    em->motionSpeed = 1.0f;
    switch (state) {
    case 0:
        Player_ClearFlag(em, 0x20000);
        if (em->hp < (s16) (0.2f * em->hpMax)) {
            state = 6;
            goto state6;
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
    case 3:
        if (em->combat == 1) {
            state = 5;
        }
        break;
    case 5:
    case 7:
        break;
    case 6:
    state6:
        Player_ClearFlag(em, 0x20000);
        if (em->curGoalKind == 0) {
            em->hasGoal = 0;
            em->timer = (em->stateRand % 120 + 180) / 2;
        } else {
            em->hasGoal = 1;
            f = Vec_DistanceXZ(&em->posX, em->goal);
            em->goalDist = f;
            em->timer = (s32) f / 7;
        }
        if (em->timer > 300) {
            em->timer = 300;
        }
        break;
    }
    Em_SetStateSync(em, 1, state, mode);
}

void EmKhezu_SetGroup2State(Em *em, u16 state, u8 mode)
{
    EmKhezuWork *w = (EmKhezuWork *)&em->work;
    struct {
        u8 unk0;
        u8 unk1;
        u16 zone;
    } info;
    f32 h;
    f32 pos2[3];
    f32 pos[3];
    Player *p;
    s32 a;
    f32 f;

    em->motionSpeed = 1.0f;
    switch (state) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 10:
    case 12:
    case 14:
    case 16:
    case 20:
    case 22:
    case 23:
    case 25:
    case 26:
    case 32:
        Player_ClearFlag(em, 0x20000);
        break;
    case 6:
    case 7:
    case 21:
        Player_ClearVelocity(em);
        em->velZ = 20.0f;
        em->timer = (s32) (Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) / em->velZ) * 3;
        break;
    case 13:
        Em_TypeSetup(em);
    case 8:
        if (em->remote == 0 && em->curGoalKind != 3) {
            EmKhezu_ChangeState(em, 2, 1, 1);
            return;
        }
        break;
    case 9:
        if (em->remote == 0) {
            em->goalKind = 2;
            em->goalArg = 1;
            em->goalNo = 0;
            Em_ComputeGoal(em, em->goal);
        }
        em->timer = 500;
        Em_TypeSetup(em);
        break;
    case 11:
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
    case 15:
        em->timer = 500;
        break;
    case 24:
        if (em->hasCeiling == 0 && em->remote == 0) {
            EmKhezu_ChangeState(em, 0, 7, 1);
            return;
        }
        break;
    case 28:
        Player_ClearFlag(em, 0x20000);
        Em_SetSpeedScaled(em, 2, 1000.0f, 500.0f);
        break;
    case 29:
    case 30:
        w->unk00A = 4;
        if (em->remote == 0) {
            em->goalNo = EmKhezu_NextAreaPoint(em, pos2);
            em->goalKind = 2;
            em->goalArg = 2;
            Em_ComputeGoal(em, em->goal);
            a = (u16) (Math_AngleXZ(&em->posX, em->goal) - em->rotY);
            if ((0 <= a && a <= 0x2000) || (a >= 0xE000 && a <= 0xFFFF)) {
                a = (u16) (Math_AngleXZ(&em->posX, pos2) - em->rotY);
                if (a >= 0x4000 && a <= 0xC000 && state == 29) {
                    state = 30;
                }
            } else {
                state = 31;
            }
        } else {
            Em_ComputeGoal2(em, em->goal);
        }
        break;
    case 33:
        if (em->remote == 0) {
            p = &gPlayers[em->targetNo];
            w->unk017 = 0;
            if (em->targetNo == -1) {
                state = 34;
                break;
            }
            em->target = p;
            Em_GetTargetPos(em, em->targetNo, pos);
            if (AreaGrid2_GetCeiling(pos, &h, (struct AreaTri *)&info) == 0) {
                state = 34;
                break;
            }
            f = Vec_DistanceXZ(&em->posX, pos);
            if (w->zone == info.zone || w->zone == 0 || w->zone >= 5 ||
                f <= 500.0f) {
                state = 34;
                break;
            }
            em->goalNo = (info.zone - 1) & 3;
            switch (w->zone) {
            case 1:
                if (info.zone == 3) {
                    em->goalNo = (info.zone - 2) & 3;
                }
                break;
            case 2:
                if (info.zone == 4) {
                    em->goalNo = (info.zone - 2) & 3;
                }
                break;
            case 3:
                if (info.zone == 1) {
                    em->goalNo = (info.zone - 2) & 3;
                }
                break;
            case 4:
                if (info.zone == 2) {
                    em->goalNo = (info.zone - 2) & 3;
                }
                break;
            }
            em->goalKind = 2;
            em->goalArg = 2;
            Em_ComputeGoal(em, em->goal);
            w->unk017 = 1;
            f = Vec_DistanceXZ(&em->posX, em->goal);
            w->unk010 = f;
            if (f <= 500.0f) {
                state = 34;
            }
        }
        break;
    case 17:
    case 18:
    case 19:
    case 27:
    case 31:
    case 34:
        break;
    }
    Em_SetStateSync(em, 2, state, mode);
}

void EmKhezu_SetGroup3State(Em *em, u16 state, u8 mode)
{
    EmKhezuWork *w = (EmKhezuWork *)&em->work;
    f32 pos[3];
    f32 f;

    em->motionSpeed = 1.0f;
    switch (state) {
    case 2:
        if (em->targetNo == -1) {
            w->unk010 = 2500.0f;
            break;
        }
        em->target = &gPlayers[em->targetNo];
        Em_GetTargetPos(em, em->targetNo, pos);
        Vec_Set(em->goal, pos[0], pos[1], pos[2]);
        f = Vec_DistanceXZ(&em->posX, em->goal);
        w->unk010 = f;
        if (f > 5000.0f) {
            w->unk010 = 5000.0f;
        } else if (f < 2500.0f) {
            w->unk010 = 2500.0f;
        }
        break;
    case 0:
    case 1:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
        break;
    }
    Em_SetStateSync(em, 3, state, mode);
}

void EmKhezu_ChangeState(Em *em, u16 group, u16 state, u8 mode)
{
    if (em->remote == 0) {
        Em_TargetPlayerInRange(em);
    } else if (mode == 3 || mode == 4) {
        return;
    }
    Em_LoadReactionSet(em, 0);
    switch (group) {
    case 0:
        EmKhezu_SetGroup0State(em, state, mode);
        break;
    case 1:
        EmKhezu_SetGroup1State(em, state, mode);
        break;
    case 2:
        EmKhezu_SetGroup2State(em, state, mode);
        break;
    case 3:
        EmKhezu_SetGroup3State(em, state, mode);
        break;
    case 8:
        if (state == 0x98) {
            Versus_MarkDefeat(em);
        }
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        break;
    case 5:
        if (state == 3 && QuestEm_CanAppear(em) == 1) {
            QuestEm_Appear(em);
        }
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        break;
    case 4:
        switch (state) {
        case 10:
            Em_EndStatus4(em);
            break;
        case 11:
        case 14:
            Em_BeginStatus4(em);
            break;
        case 12:
        case 17:
            Em_EndAllStatus(em);
            break;
        case 15:
            Em_EndStatus2(em);
            break;
        case 16:
            Em_EndStatus1(em);
            break;
        case 18:
        case 19:
        case 20:
            Em_BeginStatus5(em);
            break;
        case 21:
            Em_EndStatus5(em);
            break;
        }
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        break;
    case 6:
        switch (state) {
        case 4:
            Em_BeginStatus1(em);
            break;
        }
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        break;
    case 7:
        break;
    }
}

void EmKhezu_MoveByVelYCurve(Em *em, s32 kind)
{
    EmWork *work = (EmWork *) &em->work;
    EmCurveKey *tbl;
    EmCurveKey *p;
    f32 frame;
    f32 t;
    s32 i;

    tbl = EmKhezu_MoveVelYCurves[kind];
    if (tbl != NULL && em->motion[0].blend == 0) {
        frame = em->motion[0].frame;
        if (frame == 0.0f || frame == 1.0f) {
            em->velY = tbl->v;
        } else {
            i = 1;
            while (i != 0) {
                p = &tbl[i];
                t = p->t;
                if (frame > t && t != 0.0f) {
                    i++;
                } else if (t == 0.0f) {
                    break;
                } else {
                    i = 0;
                    em->velY = (p->v - p[-1].v) / ((t - p[-1].t) / em->motion[0].step);
                }
            }
        }
        work->moveAngBY = 0;
        work->moveAngBZ = em->rotY;
        work->moveAngCX = 0;
        Em_MoveRotated(em, &work->moveAngBY);
    }
}

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

static s32 EmKhezu_CurveNone(Em *em, EmWork *work)
{
    return 0;
}


extern EmCurveKey *EmKhezu_VelYCurves[];

/* The value of curve work[0x7] at the frame in the work area (a table of keys): sets
 * em->velY from the key pair around the frame, 2 at the table's end. */
static u8 EmKhezu_CurveVelY(Em *em, EmWork *work)
{
    EmKhezuWork *w = (EmKhezuWork *) work;
    EmCurveKey *p;
    f32 frame;
    f32 t;
    s32 i;
    EmCurveKey *tbl;
    u8 ret;

    tbl = EmKhezu_VelYCurves[((EmKhezuWork *)work)->curveKind];
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

extern EmCurveKey *EmKhezu_SpeedCurves[];

/* The value of curve work[0x7] at the frame in the work area: sets em->velZ from the key
 * pair around the frame, 4 at the table's end. */
static u8 EmKhezu_CurveSpeed(Em *em, EmWork *work)
{
    EmKhezuWork *w = (EmKhezuWork *) work;
    EmCurveKey *p;
    f32 frame;
    f32 t;
    s32 i;
    EmCurveKey *tbl;
    u8 ret;

    tbl = EmKhezu_SpeedCurves[((EmKhezuWork *)work)->curveKind];
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
    if (em->stance == 2) {
        w->accelAngX = 0;
        w->accelAngY = em->rotY;
        w->accelAngZ = 0;
        Em_MoveRotatedAccel(em, &w->accelAngX);
    }
    return ret;
}

u8 EmKhezu_NextAreaPoint(Em *em, f32 *out)
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

s32 EmKhezu_InSpecialArea(u16 area)
{
    switch (Area_DayId(area)) {
    case 0x9A:
    case 0x9B:
    case 0x9F:
        return 1;
    }
    return 0;
}

/* Velociprey's own meanings of the work area's bytes (EmWork, include/em.h, has the shared ones). */
typedef struct EmVelocipreyWork {
    u8 fxStep;                  /* 0x000: UpdateMotionEffects: 0, then 1 */
    u8 unk001[0x008 - 0x001];
    s32 accelAngX;              /* 0x008: angles X, Y, Z for Em_MoveRotatedAccel (UpdateCurves: 0, rotY, 0) */
    s32 accelAngY;              /* 0x00C */
    s32 accelAngZ;              /* 0x010 */
    u8 curveNone;               /* 0x014: the three curves' run flags (StartCurves, UpdateCurves) */
    u8 curveVelY;               /* 0x015 */
    u8 curveSpeed;              /* 0x016 */
    u8 curveKind;               /* 0x017: the curve tables' index (StartCurves) */
    s16 curveFrame;             /* 0x018: the curve frame (StartCurves, UpdateCurves) */
    s8 countdown;               /* 0x01A: Init: 0 */
    u8 unk01B[0x020 - 0x01B];
    EmObj sense;                /* 0x020: the object Em_SenseObjects fills (ChangeState removes it from the list) */
    u8 unk03C[0x044 - 0x03C];
    f32 wallVelZ;               /* 0x044: State3: the velZ kept while a wall stops the jump */
    u8 unk048[0x050 - 0x048];
    s32 unk050;                 /* 0x050: State3 */
    f32 spawnPos[3];            /* 0x054: Init: the position; Respawn puts it back */
    s32 spawnRotY;              /* 0x060: Init: rotY; Respawn puts it back */
    u8 unk064[0x06E - 0x064];
    u8 huntedB;                 /* 0x06E: as EmWork's huntedB (Init, Respawn: 0) */
    u8 unk06F[0x100 - 0x06F];
} EmVelocipreyWork;
typedef char EmVelocipreyWork_size_check[(sizeof(EmVelocipreyWork) == sizeof(EmWork)) ? 1 : -1];

void EmVelociprey_MotionSounds(s32 em, EmWork *work)
{
    s16 prevMotion;

    prevMotion = work->prevMotion;
    switch (prevMotion) {
    case 0x3E9:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xC8, Sound_RandomSe(0xE, 2, 0xE, 2), 0);
        break;
    case 0x3EA:
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 2, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x20, 2, 0);
        break;
    case 0x3EB:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x14, 8, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x10, 2, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x14, 3, 0);
        break;
    case 0x3EC:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 8, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 2, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 3, 0);
        break;
    case 0x3ED:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 2, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA, 1, 0);
        break;
    case 0x3EE:
    case 0x3EF:
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, Sound_RandomSe(5, 2, 5, 2), 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1A, 2, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1A, 4, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2A, 2, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x30, 1, 0);
        break;
    case 0x3F0:
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 2, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 4, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1A, 2, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x20, 1, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x40, 7, 0);
        break;
    case 0x3F1:
    case 0x3F2:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xC, 2, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xC, 4, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1E, 2, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x24, 1, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x40, 7, 0);
        break;
    case 0x3F3:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x44, 0x11, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA4, 0x12, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x22, 0x17, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x104, 0x17, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x120, 0x17, 0);
        break;
    case 0x3F4:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x76, 0xE, 0);
        break;
    case 0x3FC:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 2, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 3, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 8, 0);
        break;
    case 0x3FD:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 2, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 1, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x24, 0, 0);
        break;
    case 0x3FE:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x28, 2, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3C, 2, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x76, 2, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1A, 0x10, 0);
        break;
    case 0x3FF:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 2, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 3, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 8, 0);
        break;
    case 0x400:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 2, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 1, 0);
        break;
    case 0x401:
        Em_PlaySeAtBoneOnFrame((Em *) em, 2, 0x10, 0);
        break;
    case 0x402:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3C, 0, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x80, 2, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x48, 0x1C, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3C, 8, 0);
        break;
    case 0x406:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 9, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1E, 2, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1C, 1, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x36, 8, 0);
        break;
    case 0x407:
    case 0x409:
    case 0x40B:
    case 0x40D:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0xB, 0);
        break;
    case 0x408:
    case 0x40A:
    case 0x40C:
    case 0x40E:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0xF, 0);
        break;
    case 0x40F:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x18, 3, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2C, 1, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x36, 2, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x58, 0, 0);
        break;
    case 0x412:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xB4, 9, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x84, 0xB, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x34, 0xA, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x44, 1, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x56, 0, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x84, 1, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x8C, 0, 0);
        break;
    case 0x413:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x11, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x42, 0x11, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x82, 0x12, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x30, 0x17, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x7A, 0, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA2, 0xF, 0);
        break;
    case 0x414:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0xE, 0);
        break;
    case 0x416:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA, 0xD, 0);
        break;
    case 0x417:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xE, 0x19, 0);
        break;
    case 0x41A:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x20, 0xC, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x44, 0xC, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x62, 6, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x14, 0, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA0, 0x17, 0);
        break;
    case 0x41B:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA, 7, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x44, 7, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 1, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x7A, 1, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x86, 0, 0);
        break;
    case 0x41C:
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 5, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x62, 5, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 1, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3A, 0, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x6A, 1, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x9E, 0, 0);
        
    case 0x41D:
    default:
        break;
    }
}

void EmVelociprey_SpawnEffect01(u8 *em)
{
    Effect01_Spawn((Player *) em, 0);
}

void EmVelociprey_Init(Em *em)
{
    EmVelocipreyWork *w = (EmVelocipreyWork *)&em->work;
    s16 hp;

    if (gQuest.stage == 0) {
        switch (gScene.area) {
        case 0xF:
            em->posX = 9500.0f + 150.0f * (f32) (em->slotNo / 8);
            em->posY = 0.0f;
            em->posZ = 9200.0f + 150.0f * (f32) (em->slotNo & 7);
            break;
        case 0xD:
            em->posX = 5000.0f + 150.0f * (f32) (em->slotNo / 8);
            em->posY = 0.0f;
            em->posZ = 9000.0f + 150.0f * (f32) (em->slotNo & 7);
            break;
        case 0x25:
            em->posX = 10000.0f + 150.0f * (f32) (em->slotNo / 8);
            em->posY = 0.0f;
            em->posZ = 10000.0f + 150.0f * (f32) (em->slotNo & 7);
            em->rotY = 0;
            break;
        case 0x26:
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
    case 0xD:
        em->motionSpeed = 1.2f;
        break;
    case 0x10:
    case 0x23:
        em->motionSpeed = 1.0f;
        break;
    case 0x1E:
        em->motionSpeed = 1.1f;
        break;
    }
    Em_SetMotionRow(em, 1, 0, 0);
    em->stance = 0;
    hp = Em_ComputeSize(em, 0);
    em->hp = hp;
    em->hpMax = hp;
    em->scriptPending = 0;
    em->blind = 0;
    w->countdown = 0;
    w->spawnPos[0] = em->posX;
    w->spawnPos[1] = em->posY;
    w->spawnPos[2] = em->posZ;
    w->spawnRotY = em->rotY;
    Em_InitParts(em);
    em->remote = 0;
    w->huntedB = 0;
    switch (em->spawnParam) {
    case 1:
        em->senseDelay = 5;
        Em_FindLink(em);
        break;
    }
    if (em->type == 0x1E && em->variant == 1) {
        Em_SetCombat(em, 1, gEmCombatInit[em->type]);
    }
}

void EmVelociprey_Start(Em *em)
{
    EmVelociprey_Init(em);
    EmVelociprey_ChangeState(em, 0, 0xC, 0);
    switch (em->spawnParam) {
    case 1:
        EmVelociprey_ChangeState(em, 0, 0xA, 0);
        break;
    }
}

void EmVelociprey_Reinit(Em *em)
{
    EmVelociprey_Init(em);
}

void EmVelociprey_Respawn(Em *em)
{
    EmVelocipreyWork *w = (EmVelocipreyWork *)&em->work;

    s16 size;

    GmEm_ResetState(em);
    Em_SetMotionRow(em, 1, 0, 0);
    em->stance = 0;
    EmVelociprey_ChangeState(em, 0, 0xC, 0);
    size = Em_ComputeSize(em, 0);
    em->hp = size;
    em->hpMax = size;
    em->blind = 0;
    w->countdown = 0;
    Em_InitParts(em);
    em->remote = 0;
    w->huntedB = 0;
    switch (em->spawnParam) {
    case 1:
        em->senseDelay = 5;
        Em_FindLink(em);
        EmVelociprey_ChangeState(em, 0, 0xA, 0);
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

void EmVelociprey_EndState(Em *em, s32 unused1, s32 unused2)
{
    switch (em->type) {
    case 13:
        em->motionSpeed = 1.2f;
        break;
    case 16:
    case 35:
        em->motionSpeed = 1.0f;
        break;
    case 30:
        em->motionSpeed = 1.1f;
        break;
    }
    em->stance = 0;
    em->lookAtTarget = 0;
    em->scriptPending = 1;
    EmVelociprey_ChangeState(em, 0, 1, 0);
}

void EmVelociprey_State0_00(Em *em, EmWork *work)
{
    em->motionSpeed = 1.2f;
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = ((em->stateRand & 3) * 0x1E) + 0x1E;
        if (em->motionId[0] != 0x3E9) {
            Em_SetMotion(em, 1, 0, 0);
        }
        break;
    case 1:
        if (--em->timer <= 0) {
            em->scriptPending = 1;
        }
        break;
    }
}

void EmVelociprey_State0_01(Em *em, EmWork *work)
{
    em->motionSpeed = 1.2f;
    switch (em->step) {
    case 0:
        em->step++;
        if (em->motionId[0] != 0x3E9) {
            Em_SetMotion(em, 1, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->scriptPending = 1;
        }
        break;
    }
}

void EmVelociprey_State0_02(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x32, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

void EmVelociprey_State0_03(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x33, 0, 0);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            Vec_Set((f32 *) &work->moveAngBY, em->posX, em->posY, em->posZ);
            work->moveAngCY = 0x447A0000;
            ((EmVelocipreyWork *)work)->sense.area = (s16) *(u8 *)&em->area;
            ((EmVelocipreyWork *)work)->sense.kind = 0;
            ObjListA70_Add((EmObj *)&work->moveAngBY);
            work->countdown = 1;
        }
        if (em->motion[0].playing == 0) {
            if (work->countdown != 0) {
                ObjListA70_Remove((EmObj *)&work->moveAngBY);
                work->countdown = 0;
            }
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

void EmVelociprey_State0_04(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x34, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

void EmVelociprey_State0_05(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0xB, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

void EmVelociprey_State0_06(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x27, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

void EmVelociprey_State0_07(Em *em, EmWork *work)
{
    Em_SetHitReactMode(em, 2U);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x2B, 0, 0);
        Em_BeginStatus2(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x2C, 0, 0);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            EmVelociprey_ChangeState(em, 0, 8, 4);
        }
        break;
    }
}

void EmVelociprey_State0_08(Em *em, EmWork *work)
{
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x27, 0, 0);
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

void EmVelociprey_State0_09(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        if (em->motionId[0] != 0x412) {
            Em_SetMotion(em, 0x2A, 0, 0);
        }
        break;
    case 1:
        if (em->blindTimer == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

void EmVelociprey_State0_0A(Em *em, EmWork *work)
{
    em->motionSpeed = 1.2f;
    em->senseDelay = 5;
    em->noPushTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        if (em->motionId[0] != 0x3F4) {
            Em_SetMotion(em, 0xC, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->scriptPending = 1;
        }
        break;
    }
}

void EmVelociprey_State0_0B(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0xB, 0, 0);
        Em_EndStatus4(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

void EmVelociprey_State0_0C(Em *em, EmWork *work)
{
    em->motionSpeed = 1.2f;
    switch (em->step) {
    case 0:
        em->step++;
        if (em->motionId[0] != 0x3E9) {
            Em_SetMotion(em, 0xC, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->scriptPending = 1;
        }
        break;
    }
}

void EmVelociprey_State0_10(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x34, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

void EmVelociprey_RunGroup0(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmVelociprey_State0_00(em, work);
        break;
    case 1:
        EmVelociprey_State0_01(em, work);
        break;
    case 2:
        EmVelociprey_State0_02(em, work);
        break;
    case 3:
        EmVelociprey_State0_03(em, work);
        break;
    case 4:
        EmVelociprey_State0_04(em, work);
        break;
    case 5:
        EmVelociprey_State0_05(em, work);
        break;
    case 6:
        EmVelociprey_State0_06(em, work);
        break;
    case 7:
        EmVelociprey_State0_07(em, work);
        break;
    case 8:
        EmVelociprey_State0_08(em, work);
        break;
    case 9:
        EmVelociprey_State0_09(em, work);
        break;
    case 10:
        EmVelociprey_State0_0A(em, work);
        break;
    case 11:
        EmVelociprey_State0_0B(em, work);
        break;
    case 12:
        EmVelociprey_State0_0C(em, work);
        break;
    case 13:
        EmVelociprey_State0_01(em, work);
        break;
    case 16:
        EmVelociprey_State0_10(em, work);
        
    default:
        break;
    }
}

void EmVelociprey_State1_00(Em *em, EmWork *work)
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

void EmVelociprey_State1_01(Em *em, EmWork *work)
{
    f32 pos[3];

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 2, 0, 0);
        work->startXB = em->posX;
        work->startZB = em->posZ;
        work->walkFramesB = 0;
        break;
    case 1:
        work->walkFramesB++;
        if (Em_StepToGoal(em, 0x40, 0) == 1) {
            if (em->remote == 0) {
                pos[0] = work->startXB;
                pos[1] = em->posY;
                pos[2] = work->startZB;
                if (Vec_DistanceXZ(&em->posX, pos) < 100.0f && work->walkFramesB > 0x3C) {
                    EmVelociprey_ChangeState(em, 2, 0, 4);
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

void EmVelociprey_State1_02(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 8, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

void EmVelociprey_State1_03(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 9, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

void EmVelociprey_State1_04(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0xA, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

void EmVelociprey_State1_05(Em *em, EmWork *work)
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

void EmVelociprey_RunGroup1(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmVelociprey_State1_00(em, work);
        break;
    case 1:
        EmVelociprey_State1_01(em, work);
        break;
    case 2:
        EmVelociprey_State1_02(em, work);
        break;
    case 3:
        EmVelociprey_State1_03(em, work);
        break;
    case 4:
        EmVelociprey_State1_04(em, work);
        break;
    case 5:
        EmVelociprey_State1_05(em, work);
        
    default:
        break;
    }
}

void EmVelociprey_State2_00(Em *em, EmWork *work)
{
    s32 ended;

    ended = 0;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 3, 0, 0);
        EmVelociprey_StartCurves(em, 2U);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 10.0f) != 0) {
            em->stance = 2;
            ended = EmVelociprey_UpdateCurves(em) & 0xFF;
        }
        if ((ended != 0) && (em->posY <= em->floorY)) {
            em->step++;
            em->stance = 0;
            em->posY = em->floorY;
            Em_SetMotion(em, 5, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

void EmVelociprey_State2_01(Em *em, EmWork *work)
{
    s32 ended;

    ended = 0;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 4, 0, 0);
        EmVelociprey_StartCurves(em, 3U);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 22.0f) != 0) {
            em->stance = 2;
            ended = EmVelociprey_UpdateCurves(em) & 0xFF;
        }
        if ((ended != 0) && (em->posY <= em->floorY)) {
            em->step++;
            em->stance = 0;
            Em_SetMotion(em, 5, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

void EmVelociprey_State2_02(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 3, 0, 0);
        em->stance = 0;
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
            em->stance = 2;
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
            em->stance = 0;
            em->posY = em->floorY;
            Em_SetMotion(em, 5, 0, 0);
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

void EmVelociprey_RunGroup2(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmVelociprey_State2_00(em, work);
        break;
    case 1:
        EmVelociprey_State2_01(em, work);
        break;
    case 2:
        EmVelociprey_State2_02(em, work);
        break;
    }
}

void EmVelociprey_State3_00(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x16, 0, 0);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            if (em->type == 0x1E) {
                EffVelociprey13_Create(em, 0xB);
            } else {
                EffVelociprey13_Create(em, 1);
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

void EmVelociprey_State3_01(Em *em)
{
    EmVelociprey_EndState(em, 0, 0);
}

void EmVelociprey_State3_02(Em *em, EmWork *work)
{
    EmVelocipreyWork *cw = (EmVelocipreyWork *)work;
    u8 landed;

    landed = 0;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x14, 0, 0);
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
        cw->unk050 = 0;
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            switch (em->type) {
            case 0x10:
            case 0x23:
                EffVelociprey13_Create(em, 3);
                break;
            case 0xD:
                EffVelociprey13_Create(em, 0xC);
                break;
            case 0x1E:
                EffVelociprey13_Create(em, 0xD);
                break;
            }
        }
        if (EmMotion_ReachedFrame(em, 0, 22.0f) != 0) {
            em->stance = 2;
            work->accelAngX = 0;
            work->accelAngY = em->rotY;
            work->accelAngZ = 0;
            landed = 1;
            if (em->wallHitDirs & 0xF000000F) {
                cw->wallVelZ = em->velZ;
                em->velZ = 0.0f;
            }
            Em_MoveRotatedAccel(em, &work->accelAngX);
            if (cw->wallVelZ != 0.0f) {
                em->velZ = cw->wallVelZ;
                cw->wallVelZ = 0.0f;
            }
            if (em->velZ < 10.0f) {
                em->velZ = 10.0f;
                em->accelZ = 0.0f;
            }
        }
        if (em->hasGoal != 0 && em->stance == 2) {
            Em_TurnToGoal(em, 0x100, 0);
        }
        if (landed && em->posY <= em->floorY) {
            em->step++;
            em->stance = 0;
            em->posY = em->floorY;
            Em_SetMotion(em, 0x15, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

void EmVelociprey_State3_03(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x19, 0, 0);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 16.0f, 0U) != 0) {
            switch (em->type) {
            case 0x10:
            case 0x23:
                EffVelociprey13_Create(em, 5);
                break;
            case 0xD:
                EffVelociprey13_Create(em, 5);
                break;
            case 0x1E:
                EffVelociprey13_Create(em, 0xE);
                break;
            }
        }
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

void EmVelociprey_State3_04(Em *em, EmWork *work, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x1A, 0, 0);
        break;
    case 1:
        switch (variant) {
        case 0:
        default:
            if (Player_PassedFrameScaled(em, 80.0f, 0U) != 0) {
                EffEm08_CreateAtBone(em, 0xF, 9, 0, 0xF8E5, 0);
            }
            break;
        case 1:
            if (Player_PassedFrameScaled(em, 80.0f, 0U) != 0) {
                EffEm08_CreateAtBone(em, 0xF, 9, 2, 0xF8E5, 0);
            }
            break;
        }
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

void EmVelociprey_State3_05(Em *em, EmWork *work)
{
    EmVelocipreyWork *cw = (EmVelocipreyWork *)work;
    u8 landed;

    landed = 0;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x14, 0, 0);
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
        cw->unk050 = 0;
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            switch (em->type) {
            case 0x10:
            case 0x23:
                EffVelociprey13_Create(em, 5);
                break;
            case 0xD:
                EffVelociprey13_Create(em, 0xC);
                break;
            case 0x1E:
                EffVelociprey13_Create(em, 0xF);
                break;
            }
        }
        if (EmMotion_ReachedFrame(em, 0, 22.0f) != 0) {
            em->stance = 2;
            work->accelAngX = 0;
            work->accelAngY = em->rotY;
            work->accelAngZ = 0;
            landed = 1;
            if (em->wallHitDirs & 0xF000000F) {
                cw->wallVelZ = em->velZ;
                em->velZ = 0.0f;
            }
            Em_MoveRotatedAccel(em, &work->accelAngX);
            if (cw->wallVelZ != 0.0f) {
                em->velZ = cw->wallVelZ;
                cw->wallVelZ = 0.0f;
            }
            if (em->velZ < 10.0f) {
                em->velZ = 10.0f;
                em->accelZ = 0.0f;
            }
        }
        if (em->hasGoal != 0 && em->stance == 2) {
            Em_TurnToGoal(em, 0x100, 0);
        }
        if (landed && em->posY <= em->floorY) {
            em->step++;
            em->stance = 0;
            em->posY = em->floorY;
            Em_SetMotion(em, 0x15, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

void EmVelociprey_State3_06(Em *em, EmWork *work)
{
    EmVelocipreyWork *cw = (EmVelocipreyWork *)work;
    u8 landed;

    landed = 0;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x14, 0, 0);
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
        cw->unk050 = 0;
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            switch (em->type) {
            case 0x10:
            case 0x23:
                EffVelociprey13_Create(em, 5);
                break;
            case 0xD:
                EffVelociprey13_Create(em, 0xC);
                break;
            case 0x1E:
                EffVelociprey13_Create(em, 0x10);
                break;
            }
        }
        if (EmMotion_ReachedFrame(em, 0, 22.0f) != 0) {
            em->stance = 2;
            work->accelAngX = 0;
            work->accelAngY = em->rotY;
            work->accelAngZ = 0;
            landed = 1;
            if (em->wallHitDirs & 0xF000000F) {
                cw->wallVelZ = em->velZ;
                em->velZ = 0.0f;
            }
            Em_MoveRotatedAccel(em, &work->accelAngX);
            if (cw->wallVelZ != 0.0f) {
                em->velZ = cw->wallVelZ;
                cw->wallVelZ = 0.0f;
            }
            if (em->velZ < 10.0f) {
                em->velZ = 10.0f;
                em->accelZ = 0.0f;
            }
        }
        if (em->hasGoal != 0 && em->stance == 2) {
            Em_TurnToGoal(em, 0x100, 0);
        }
        if (landed && em->posY <= em->floorY) {
            em->step++;
            em->stance = 0;
            em->posY = em->floorY;
            Em_SetMotion(em, 0x15, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

void EmVelociprey_State3_07(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x16, 0, 0);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            EffVelociprey13_Create(em, 7);
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

void EmVelociprey_State3_08(Em *em, EmWork *work)
{
    EmVelocipreyWork *cw = (EmVelocipreyWork *)work;
    u8 landed;

    landed = 0;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x14, 0, 0);
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
        cw->unk050 = 0;
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            EffVelociprey13_Create(em, 9);
        }
        if (EmMotion_ReachedFrame(em, 0, 22.0f) != 0) {
            em->stance = 2;
            work->accelAngX = 0;
            work->accelAngY = em->rotY;
            work->accelAngZ = 0;
            landed = 1;
            if (em->wallHitDirs & 0xF000000F) {
                cw->wallVelZ = em->velZ;
                em->velZ = 0.0f;
            }
            Em_MoveRotatedAccel(em, &work->accelAngX);
            if (cw->wallVelZ != 0.0f) {
                em->velZ = cw->wallVelZ;
                cw->wallVelZ = 0.0f;
            }
            if (em->velZ < 10.0f) {
                em->velZ = 10.0f;
                em->accelZ = 0.0f;
            }
        }
        if (em->hasGoal != 0 && em->stance == 2) {
            Em_TurnToGoal(em, 0x100, 0);
        }
        if (landed && em->posY <= em->floorY) {
            em->step++;
            em->stance = 0;
            em->posY = em->floorY;
            Em_SetMotion(em, 0x15, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

void EmVelociprey_State3_09(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x19, 0, 0);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 16.0f, 0U) != 0) {
            EffVelociprey13_Create(em, 0xA);
        }
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

void EmVelociprey_State3_0A(Em *em, EmWork *work)
{
    EmVelocipreyWork *cw = (EmVelocipreyWork *)work;
    u8 landed;

    landed = 0;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x14, 0, 0);
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
        cw->unk050 = 0;
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            EffVelociprey13_Create(em, 9);
        }
        if (EmMotion_ReachedFrame(em, 0, 22.0f) != 0) {
            em->stance = 2;
            work->accelAngX = 0;
            work->accelAngY = em->rotY;
            work->accelAngZ = 0;
            landed = 1;
            if (em->wallHitDirs & 0xF000000F) {
                cw->wallVelZ = em->velZ;
                em->velZ = 0.0f;
            }
            Em_MoveRotatedAccel(em, &work->accelAngX);
            if (cw->wallVelZ != 0.0f) {
                em->velZ = cw->wallVelZ;
                cw->wallVelZ = 0.0f;
            }
            if (em->velZ < 10.0f) {
                em->velZ = 10.0f;
                em->accelZ = 0.0f;
            }
        }
        if (em->hasGoal != 0 && em->stance == 2) {
            Em_TurnToGoal(em, 0x100, 0);
        }
        if (landed && em->posY <= em->floorY) {
            em->step++;
            em->stance = 0;
            em->posY = em->floorY;
            Em_SetMotion(em, 0x15, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

void EmVelociprey_State3_0B(Em *em, EmWork *work)
{
    EmVelocipreyWork *cw = (EmVelocipreyWork *)work;
    u8 landed;

    landed = 0;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x14, 0, 0);
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
        cw->unk050 = 0;
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            EffVelociprey13_Create(em, 9);
        }
        if (EmMotion_ReachedFrame(em, 0, 22.0f) != 0) {
            em->stance = 2;
            work->accelAngX = 0;
            work->accelAngY = em->rotY;
            work->accelAngZ = 0;
            landed = 1;
            if (em->wallHitDirs & 0xF000000F) {
                cw->wallVelZ = em->velZ;
                em->velZ = 0.0f;
            }
            Em_MoveRotatedAccel(em, &work->accelAngX);
            if (cw->wallVelZ != 0.0f) {
                em->velZ = cw->wallVelZ;
                cw->wallVelZ = 0.0f;
            }
            if (em->velZ < 10.0f) {
                em->velZ = 10.0f;
                em->accelZ = 0.0f;
            }
        }
        if (em->hasGoal != 0 && em->stance == 2) {
            Em_TurnToGoal(em, 0x100, 0);
        }
        if (landed && em->posY <= em->floorY) {
            em->step++;
            em->stance = 0;
            em->posY = em->floorY;
            Em_SetMotion(em, 0x15, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

void EmVelociprey_State3_0C(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x16, 0, 0);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            EffVelociprey13_Create(em, 0x11);
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

void EmVelociprey_State3_0E(Em *em, EmWork *work, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x1A, 0, 0);
        break;
    case 1:
        switch (variant) {
        case 0:
        default:
            if (Player_PassedFrameScaled(em, 80.0f, 0U) != 0) {
                EffEm08_CreateAtBone(em, 0xF, 0x17, 0, 0xF8E5, 0);
            }
            break;
        case 1:
            if (Player_PassedFrameScaled(em, 80.0f, 0U) != 0) {
                EffEm08_CreateAtBone(em, 0xF, 0x17, 1, 0xF8E5, 0);
            }
            break;
        }
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

void EmVelociprey_RunGroup3(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmVelociprey_State3_00(em);
        break;
    case 1:
        EmVelociprey_State3_01(em);
        break;
    case 2:
        EmVelociprey_State3_02(em, work);
        break;
    case 3:
        EmVelociprey_State3_03(em, work);
        break;
    case 4:
        EmVelociprey_State3_04(em, work, 0);
        break;
    case 5:
        EmVelociprey_State3_05(em, work);
        break;
    case 6:
        EmVelociprey_State3_06(em, work);
        break;
    case 7:
        EmVelociprey_State3_07(em);
        break;
    case 8:
        EmVelociprey_State3_08(em, work);
        break;
    case 9:
        EmVelociprey_State3_09(em);
        break;
    case 0xA:
        EmVelociprey_State3_0A(em, work);
        break;
    case 0xB:
        EmVelociprey_State3_0B(em, work);
        break;
    case 0xC:
        EmVelociprey_State3_0C(em);
        break;
    case 0xD:
        EmVelociprey_State3_04(em, work, 1);
        break;
    case 0xE:
        EmVelociprey_State3_0E(em, work, 0);
        break;
    case 0xF:
        EmVelociprey_State3_0E(em, work, 1);
        break;
    }
}

void EmVelociprey_State4_00(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x1E, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

void EmVelociprey_State4_01(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x1F, 0, 0);
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
            em->stance = 2;
            Em_MoveRotatedAccel(em, &work->accelAngX);
            if (em->posY <= em->floorY) {
                em->step++;
                em->stance = 0;
                Em_SetMotion(em, 0x20, 0, 0);
                break;
            }
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmVelociprey_ChangeState(em, 0, 6, 0);
        }
        break;
    }
}

void EmVelociprey_State4_02(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x21, 0, 0);
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
            em->stance = 2;
            Em_MoveRotatedAccel(em, &work->accelAngX);
            if (em->posY <= em->floorY) {
                em->step++;
                em->stance = 0;
                Em_SetMotion(em, 0x22, 0, 0);
                break;
            }
        }
        break;
    case 2:
        if (EmMotion_ReachedFrame(em, 0, 26.0f) == 0) {
            em->rotY -= 0x555;
        }
        if (em->motion[0].playing == 0) {
            EmVelociprey_ChangeState(em, 0, 6, 0);
        }
        break;
    }
}

void EmVelociprey_State4_03(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x25, 0, 0);
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
            em->stance = 2;
            Em_MoveRotatedAccel(em, &work->accelAngX);
            if (em->posY <= em->floorY) {
                em->step++;
                em->stance = 0;
                Em_SetMotion(em, 0x26, 0, 0);
                break;
            }
        }
        break;
    case 2:
        if (EmMotion_ReachedFrame(em, 0, 26.0f) == 0) {
            em->rotY -= 0x2AA;
        }
        if (em->motion[0].playing == 0) {
            EmVelociprey_ChangeState(em, 0, 6, 0);
        }
        break;
    }
}

void EmVelociprey_State4_04(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x23, 0, 0);
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
            em->stance = 2;
            Em_MoveRotatedAccel(em, &work->accelAngX);
            if (em->posY <= em->floorY) {
                em->step++;
                em->stance = 0;
                Em_SetMotion(em, 0x24, 0, 0);
                break;
            }
        }
        break;
    case 2:
        if (EmMotion_ReachedFrame(em, 0, 26.0f) == 0) {
            em->rotY += 0x2AA;
        }
        if (em->motion[0].playing == 0) {
            EmVelociprey_ChangeState(em, 0, 6, 0);
        }
        break;
    }
}

void EmVelociprey_State4_05(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x2A, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

void EmVelociprey_State4_06(Em *em, EmWork *work)
{
    Em_SetHitReactMode(em, 4U);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x2F, 0, 0);
        Em_BeginStatus4(em);
        Em_RestartScript(em);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmVelociprey_ChangeState(em, 0, 0xB, 4);
        }
        break;
    }
    Em_SpawnEffect06Bone(em, 2);
}

void EmVelociprey_State4_08(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x2A, 0, 0);
        StageObj_Stun_CreateEm(em, 1, 0xE, *(s16 *)((u8 *)em + 8), 50.0f, 1.0f);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmVelociprey_ChangeState(em, 0, 0x10, 0);
        }
        break;
    }
}

void EmVelociprey_RunGroup4(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmVelociprey_State4_00(em, work);
        break;
    case 1:
        EmVelociprey_State4_01(em, work);
        break;
    case 2:
        EmVelociprey_State4_02(em, work);
        break;
    case 3:
        EmVelociprey_State4_03(em, work);
        break;
    case 4:
        EmVelociprey_State4_04(em, work);
        break;
    case 5:
        EmVelociprey_State4_05(em, work);
        break;
    case 6:
        EmVelociprey_State4_06(em, work);
        break;
    case 8:
        EmVelociprey_State4_08(em, work);
        
    default:
        break;
    }
}

void EmVelociprey_State5_00_Hunted(Em *em, EmWork *work)
{
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        work->huntedB = 1;
        QuestEm_Hunted(em);
        Em_SetMotion(em, 0x1F, 0, 0);
        work->accelAngX = 0;
        work->accelAngY = em->rotY;
        work->accelAngZ = 0;
        Player_ClearVelocity(em);
        em->velY = 23.35f;
        em->velZ = -27.0f;
        em->accelY = -2.0f;
        em->stance = 2;
        break;
    case 1:
        Em_MoveRotatedAccel(em, &work->accelAngX);
        if (em->posY <= em->floorY) {
            em->step++;
            em->stance = 0;
            Em_SetMotion(em, 0x20, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x2E, 0, 0);
            em->fieldCollide = 0;
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x29, 0, 0);
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
            EmVelociprey_ChangeState(em, 5, 4, 4);
        }
        break;
    }
}

void EmVelociprey_State5_01_Hunted(Em *em, EmWork *work)
{
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        work->huntedB = 1;
        QuestEm_Hunted(em);
        Em_SetMotion(em, 0x21, 0, 0);
        work->accelAngX = 0;
        work->accelAngY = (em->rotY + 0x8000) & 0xFFFF;
        work->accelAngZ = 0;
        Player_ClearVelocity(em);
        em->velY = 23.35f;
        em->velZ = -27.0f;
        em->accelY = -2.0f;
        em->stance = 2;
        break;
    case 1:
        Em_MoveRotatedAccel(em, &work->accelAngX);
        if (em->posY <= em->floorY) {
            em->step++;
            em->stance = 0;
            Em_SetMotion(em, 0x22, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x2E, 0, 0);
            em->fieldCollide = 0;
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x29, 0, 0);
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
            EmVelociprey_ChangeState(em, 5, 4, 4);
        }
        break;
    }
}

void EmVelociprey_State5_02_Hunted(Em *em, EmWork *work)
{
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        work->huntedB = 1;
        QuestEm_Hunted(em);
        Em_SetMotion(em, 0x25, 0, 0);
        work->accelAngX = 0;
        work->accelAngY = (em->rotY - 0x4000) & 0xFFFF;
        work->accelAngZ = 0;
        Player_ClearVelocity(em);
        em->velY = 23.35f;
        em->velZ = -27.0f;
        em->accelY = -2.0f;
        em->stance = 2;
        break;
    case 1:
        Em_MoveRotatedAccel(em, &work->accelAngX);
        if (em->posY <= em->floorY) {
            em->step++;
            em->stance = 0;
            Em_SetMotion(em, 0x26, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x2E, 0, 0);
            em->fieldCollide = 0;
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x29, 0, 0);
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
            EmVelociprey_ChangeState(em, 5, 4, 4);
        }
        break;
    }
}

void EmVelociprey_State5_03_Hunted(Em *em, EmWork *work)
{
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        work->huntedB = 1;
        QuestEm_Hunted(em);
        Em_SetMotion(em, 0x23, 0, 0);
        work->accelAngX = 0;
        work->accelAngY = (em->rotY + 0x4000) & 0xFFFF;
        work->accelAngZ = 0;
        Player_ClearVelocity(em);
        em->velY = 23.35f;
        em->velZ = -27.0f;
        em->accelY = -2.0f;
        em->stance = 2;
        break;
    case 1:
        Em_MoveRotatedAccel(em, &work->accelAngX);
        if (em->posY <= em->floorY) {
            em->step++;
            em->stance = 0;
            Em_SetMotion(em, 0x24, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x2E, 0, 0);
            em->fieldCollide = 0;
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x29, 0, 0);
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
            EmVelociprey_ChangeState(em, 5, 4, 4);
        }
        break;
    }
}

void EmVelociprey_State5_04_Appear(Em *em)
{
    f32 target[4];
    s16 n;
    u8 state;
    EmVelocipreyAppearAt *tbl;
    u16 k;

    switch (em->step) {
    case 0:
        if (QuestEm_CanAppear(em) == 1) {
            switch (Area_DayId(em->area)) {
            case 0x1F:
                k = em->stateRand % 4;
                Vec_Set(&em->posX, EmVelociprey_AppearPts1F[k].pos[0], D_008C8474[k].pos[0], D_008C8478[k].pos[0]);
                Vec_Set(&em->prevPosX, EmVelociprey_AppearPts1F[k].pos[0], D_008C8474[k].pos[0], D_008C8478[k].pos[0]);
                Vec_Set(target, D_008C847C[k].pos[0], D_008C8480[k].pos[0], D_008C8484[k].pos[0]);
                em->rotY = Math_AngleXZ(&em->posX, target) & 0xFFFF;
                EmVelociprey_ChangeState(em, 7, 1, 0);
                break;
            case 0x26:
                k = em->stateRand % 5;
                Vec_Set(&em->posX, EmVelociprey_AppearPts26[k].pos[0], D_008C8534[k].pos[0], D_008C8538[k].pos[0]);
                Vec_Set(&em->prevPosX, EmVelociprey_AppearPts26[k].pos[0], D_008C8534[k].pos[0], D_008C8538[k].pos[0]);
                Vec_Set(target, D_008C853C[k].pos[0], D_008C8540[k].pos[0], D_008C8544[k].pos[0]);
                em->rotY = Math_AngleXZ(&em->posX, target) & 0xFFFF;
                EmVelociprey_ChangeState(em, 7, 2, 0);
                break;
            case 0x27:
                k = em->stateRand % 4;
                Vec_Set(&em->posX, EmVelociprey_AppearPts27[k].pos[0], D_008C85B4[k].pos[0], D_008C85B8[k].pos[0]);
                Vec_Set(&em->prevPosX, EmVelociprey_AppearPts27[k].pos[0], D_008C85B4[k].pos[0], D_008C85B8[k].pos[0]);
                Vec_Set(target, D_008C85BC[k].pos[0], D_008C85C0[k].pos[0], D_008C85C4[k].pos[0]);
                em->rotY = Math_AngleXZ(&em->posX, target) & 0xFFFF;
                EmVelociprey_ChangeState(em, 7, 0, 0);
                break;
            case 0x5E:
                Vec_Set(&em->posX, D_008C8610, D_008C8614, D_008C8618);
                Vec_Set(&em->prevPosX, D_008C8610, D_008C8614, D_008C8618);
                Vec_Set(target, D_008C861C, D_008C8620, D_008C8624);
                em->rotY = Math_AngleXZ(&em->posX, target) & 0xFFFF;
                EmVelociprey_ChangeState(em, 7, 1, 0);
                break;
            case 0x22:
            case 0x24:
            case 0x70:
            case 0x71:
            case 0x72:
            case 0x74:
            case 0x77:
            case 0x80:
            case 0x81:
            case 0x8E:
            case 0x90:
            case 0x93:
            case 0x9B:
            case 0x9C:
            case 0x9F:
            case 0xA0:
            case 0xA4:
            case 0xA7:
            case 0xB0:
            case 0xB1:
            case 0xB2:
                state = 0;
                switch (Area_DayId(em->area)) {
                case 0x22:
                    n = 3;
                    tbl = EmVelociprey_AppearPts22;
                    break;
                case 0x24:
                    n = 3;
                    tbl = EmVelociprey_AppearPts24;
                    break;
                case 0x70:
                    n = 4;
                    tbl = EmVelociprey_AppearPts70;
                    break;
                case 0x71:
                    n = 3;
                    tbl = EmVelociprey_AppearPts71;
                    break;
                case 0x72:
                    n = 4;
                    tbl = EmVelociprey_AppearPts72;
                    break;
                case 0x74:
                    n = 3;
                    tbl = EmVelociprey_AppearPts74;
                    break;
                case 0x77:
                    n = 2;
                    tbl = EmVelociprey_AppearPts77;
                    break;
                case 0x80:
                    n = 2;
                    tbl = EmVelociprey_AppearPts80;
                    break;
                case 0x81:
                    n = 2;
                    tbl = EmVelociprey_AppearPts81;
                    break;
                case 0x8E:
                    state = 2;
                    n = 3;
                    tbl = EmVelociprey_AppearPts8E;
                    break;
                case 0x90:
                    if (em->area == 0x32) {
                        n = 1;
                    } else {
                        n = 2;
                    }
                    tbl = EmVelociprey_AppearPts90;
                    break;
                case 0x93:
                    n = 1;
                    tbl = EmVelociprey_AppearPts93;
                    break;
                case 0x9B:
                case 0x9C:
                    n = 2;
                    tbl = EmVelociprey_AppearPts9C;
                    break;
                case 0x9F:
                    n = 1;
                    tbl = EmVelociprey_AppearPts9F;
                    break;
                case 0xA0:
                    n = 3;
                    tbl = EmVelociprey_AppearPtsA0;
                    break;
                case 0xA4:
                    n = 2;
                    tbl = EmVelociprey_AppearPtsA4;
                    break;
                case 0xA7:
                    n = 1;
                    tbl = EmVelociprey_AppearPtsA7;
                    break;
                case 0xB0:
                    state = 2;
                    n = 4;
                    tbl = EmVelociprey_AppearPtsB0;
                    break;
                case 0xB1:
                    n = 2;
                    tbl = EmVelociprey_AppearPtsB1;
                    break;
                case 0xB2:
                    n = 2;
                    tbl = EmVelociprey_AppearPtsB2;
                    break;
                }
                k = em->stateRand % n;
                Vec_Copy(&em->posX, tbl[k].pos);
                Vec_Copy(&em->prevPosX, &em->posX);
                em->rotY = tbl[k].angle;
                EmVelociprey_ChangeState(em, 7, state, 0);
                break;
            default:
                if (em->link == NULL) {
                    EmVelociprey_ChangeState(em, 0, 0xC, 0);
                } else {
                    em->senseDelay = 5;
                    EmVelociprey_ChangeState(em, 0, 0xA, 0);
                }
                break;
            }
            break;
        }
        em->taskPhase++;
        break;
    }
}

void EmVelociprey_State5_05(Em *em)
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

void EmVelociprey_State5_06_Hunted(Em *em, EmWork *work)
{
    em->hideShadowHold = 2;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        work->huntedB = 1;
        QuestEm_Hunted(em);
        em->stance = 2;
        Em_SetMotion(em, 0x2D, 0, 0);
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
            EmVelociprey_ChangeState(em, 5, 4, 4);
        }
        break;
    }
}

void EmVelociprey_State5_07_Hunted(Em *em, EmWork *work)
{
    em->hideShadowHold = 2;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        work->huntedB = 1;
        QuestEm_Hunted(em);
        em->stance = 2;
        Em_SetMotion(em, 0x2D, 0, 0);
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
            EmVelociprey_ChangeState(em, 5, 4, 4);
        }
        break;
    }
}

void EmVelociprey_State5_08_Hunted(Em *em, EmWork *work)
{
    em->hideShadowHold = 2;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        work->huntedB = 1;
        QuestEm_Hunted(em);
        em->stance = 2;
        Em_SetMotion(em, 0x2D, 0, 0);
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
            EmVelociprey_ChangeState(em, 5, 4, 4);
        }
        break;
    }
}

void EmVelociprey_State5_09_Hunted(Em *em, EmWork *work)
{
    em->hideShadowHold = 2;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        work->huntedB = 1;
        QuestEm_Hunted(em);
        em->stance = 2;
        Em_SetMotion(em, 0x2D, 0, 0);
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
            EmVelociprey_ChangeState(em, 5, 4, 4);
        }
        break;
    }
}

void EmVelociprey_RunGroup5(Em *em, EmWork *work)
{
    em->noHitTimer = 0xA;
    em->noPushTimer = 0xA;
    switch (em->state) {
    case 0:
        EmVelociprey_State5_00_Hunted(em, work);
        break;
    case 1:
        EmVelociprey_State5_01_Hunted(em, work);
        break;
    case 2:
        EmVelociprey_State5_02_Hunted(em, work);
        break;
    case 3:
        EmVelociprey_State5_03_Hunted(em, work);
        break;
    case 4:
        EmVelociprey_State5_04_Appear(em);
        break;
    case 5:
        EmVelociprey_State5_05(em);
        break;
    case 6:
        EmVelociprey_State5_06_Hunted(em, work);
        break;
    case 7:
        EmVelociprey_State5_07_Hunted(em, work);
        break;
    case 8:
        EmVelociprey_State5_08_Hunted(em, work);
        break;
    case 9:
        EmVelociprey_State5_09_Hunted(em, work);
        
    default:
        break;
    }
}

void EmVelociprey_State6_00(Em *em, EmWork *work)
{
    s32 diff;
    s32 diff2;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 0;
        em->posX = 8460.0f;
        em->posY = 0.0f;
        em->posZ = 9367.0f;
        em->goal[0] = 8212.0f;
        em->goal[1] = 0.0f;
        em->goal[2] = 9356.0f;
        em->rotX = 0;
        em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        em->rotZ = 0;
        Em_SetMotion(em, 0x35, 0, 0);
        em->timer = 0x1C2;
        break;
    case 1:
        if (--em->timer <= 0 && (em->motion[0].playing == 0)) {
            em->step++;
            Em_SetMotion(em, 1, 0, 0);
            break;
        }
    default:
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x32, 0, 0);
        }
        break;
    case 4:
        if (em->motion[0].blend == 0) {
            diff = ((u16) work->fxMotion - (u16) em->rotY) & 0xFFFF;
            if ((EmMotion_ReachedFrame(em, 0, 24.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 44.0f) == 0)) {
                if ((((diff & 0xFFFF) + 0x666) & 0xFFFF) < 0xCCC) {
                    em->rotY = (u16) work->fxMotion;
                } else {
                    em->rotY = (em->rotY - 0x666) & 0xFFFF;
                }
            }
        case 3:
            if (em->motion[0].playing == 0) {
                em->step++;
                Em_SetMotion(em, 6, 0, 0);
                em->goal[0] = 8836.0f;
                em->goal[1] = 0.0f;
                em->goal[2] = 9382.0f;
                break;
            }
        }
        break;
    case 5:
        if (em->motion[0].blend == 0) {
            diff2 = ((u16) work->fxMotion - (u16) em->rotY) & 0xFFFF;
            if ((EmMotion_ReachedFrame(em, 0, 24.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 44.0f) == 0)) {
                if ((((diff2 & 0xFFFF) + 0x666) & 0xFFFF) < 0xCCC) {
                    em->rotY = (u16) work->fxMotion;
                } else {
                    em->rotY = (em->rotY - 0x666) & 0xFFFF;
                }
            }
            if (em->motion[0].playing == 0) {
                em->step++;
                Em_SetMotion(em, 0x33, 0, 0);
                break;
            }
        }
        break;
    case 6:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0xB, 0, 0);
        }
        break;
    case 7:
        em->step++;
        EmVelociprey_EndState(em, 0, 0);
        break;
    }
}

void EmVelociprey_State6_01(Em *em)
{
    f32 dist;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 0;
        em->posX = 8200.0f;
        em->posY = 0.0f;
        em->posZ = 7300.0f;
        em->goal[0] = 7730.0f;
        em->goal[1] = 0.0f;
        em->goal[2] = 9300.0f;
        em->rotX = 0;
        em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        em->rotZ = 0;
        Em_SetMotion(em, 1, 0, 0);
        em->timer = 0x2EE;
        em->fieldCollide = 0;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 2, 0, 0);
            em->timer = 0x12C;
        }
        break;
    case 2:
        dist = Vec_DistanceXZ(&em->posX, em->goal);
        em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        if (--em->timer > 0) {
            if (dist <= 100.0f) {
                goto block_12;
            }
        } else {
block_12:
            em->step++;
            Em_SetMotion(em, 1, 0, 0);
        }
        break;
    case 3:
        em->step++;
        EmVelociprey_EndState(em, 0, 0);
        em->fieldCollide = 1;
        break;
    }
}

void EmVelociprey_State6_02(Em *em)
{
    f32 dist;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 0;
        em->posX = 8340.0f;
        em->posY = 300.0f;
        em->posZ = 12470.0f;
        em->goal[0] = 8000.0f;
        em->goal[1] = 300.0f;
        em->goal[2] = 10850.0f;
        em->rotX = 0;
        em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        em->rotZ = 0;
        Em_SetMotion(em, 1, 0, 0);
        em->timer = 0x2EE;
        em->fieldCollide = 0;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 2, 0, 0);
            em->timer = 0x12C;
        }
        break;
    case 2:
        dist = Vec_DistanceXZ(&em->posX, em->goal);
        em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        if (--em->timer > 0) {
            if (dist <= 100.0f) {
                goto block_13;
            }
        } else {
block_13:
            em->step++;
            Em_SetMotion(em, 0x32, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 1, 0, 0);
            em->timer = 0x12C;
        }
        break;
    case 4:
        em->step++;
        em->fieldCollide = 1;
        EmVelociprey_EndState(em, 0, 0);
        break;
    }
}

void EmVelociprey_State6_03(Em *em)
{
    switch (em->variant) {
    case 0:
        EmVelociprey_ChangeState(em, 6, 0, 0);
        break;
    case 1:
        EmVelociprey_ChangeState(em, 6, 1, 0);
        break;
    case 2:
        EmVelociprey_ChangeState(em, 6, 2, 0);
        break;
    }
}

void EmVelociprey_State6_04(Em *em)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 0;
        Em_SetMotion(em, 1, 0, 0);
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

void EmVelociprey_State6_05(Em *em)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 0;
        Em_SetMotion(em, 1, 0, 0);
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

void EmVelociprey_State6_06(Em *em)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 0;
        Em_SetMotion(em, 1, 0, 0);
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

void EmVelociprey_RunGroup6(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmVelociprey_State6_00(em, work);
        break;
    case 1:
        EmVelociprey_State6_01(em);
        break;
    case 2:
        EmVelociprey_State6_02(em);
        break;
    case 3:
        EmVelociprey_State6_03(em);
        break;
    case 4:
        EmVelociprey_State6_04(em);
        break;
    case 5:
        EmVelociprey_State6_05(em);
        break;
    case 6:
        EmVelociprey_State6_06(em);
        
    default:
        break;
    }
}

void EmVelociprey_State7_00(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 3, 0, 0);
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
            em->stance = 2;
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
            em->stance = 0;
            em->posY = em->floorY;
            Em_SetMotion(em, 5, 0, 0);
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

void EmVelociprey_State7_01(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 3, 0, 0);
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
            em->stance = 2;
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
            em->stance = 0;
            em->posY = em->floorY;
            Em_SetMotion(em, 5, 0, 0);
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

void EmVelociprey_State7_02(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 3, 0, 0);
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
            em->stance = 2;
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
            em->stance = 0;
            em->posY = em->floorY;
            Em_SetMotion(em, 5, 0, 0);
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

void EmVelociprey_RunGroup7(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmVelociprey_State7_00(em, work);
        break;
    case 1:
        EmVelociprey_State7_01(em, work);
        break;
    case 2:
        EmVelociprey_State7_02(em, work);
        break;
    }
}

void EmVelociprey_Update(Em *em)
{
    EmVelocipreyWork *w = (EmVelocipreyWork *)&em->work;
    u8 buf[4];
    s32 seen;
    u32 r;

    em->motionSpeed = 1.2f;
    if (em->senseDelay != 0) {
        em->noHitTimer = 5;
    }
    seen = Em_SenseObjects(em, &w->sense);
    if (em->senseDelay == 0 && em->combat == 0 && seen != 0 && Em_ScriptFlagsFrom(em, 4) == 0) {
        em->triggers |= 4;
    }
    Em_UpdateCombatTimer(em);
    r = Em_UpdateDamage(em, buf) & 0xFF;
    switch (r) {
    case 0:
    case 3:
    case 4:
    case 7:
    case 9:
    case 11:
    case 13:
    case 15:
    case 16:
    case 18:
        break;
    case 1:
    case 2:
        if (em->stance == 2 && r == 1 && (em->partFlags[0] & 2)) {
            u16 a = em->hitAngle;
            s32 d = (a - em->rotY) & 0xFFFF;
            if (d >= 0x6000 && d < 0xA000) {
                em->rotY = a + 0x8000;
                EmVelociprey_ChangeState(em, 5, 6, 3);
            } else if (d >= 0x2000 && d < 0x6000) {
                em->rotY = a - 0x4000;
                EmVelociprey_ChangeState(em, 5, 8, 3);
            } else if (d >= 0xA000 && d < 0xE000) {
                em->rotY = a + 0x4000;
                EmVelociprey_ChangeState(em, 5, 9, 3);
            } else {
                em->rotY = a & 0xFFFF;
                EmVelociprey_ChangeState(em, 5, 7, 3);
            }
        } else {
            u16 a = em->hitAngle;
            s32 d = (a - em->rotY) & 0xFFFF;
            if (d >= 0x6000 && d < 0xA000) {
                em->rotY = a + 0x8000;
                EmVelociprey_ChangeState(em, 5, 0, 3);
            } else if (d >= 0x2000 && d < 0x6000) {
                em->rotY = a - 0x4000;
                EmVelociprey_ChangeState(em, 5, 2, 3);
            } else if (d >= 0xA000 && d < 0xE000) {
                em->rotY = a + 0x4000;
                EmVelociprey_ChangeState(em, 5, 3, 3);
            } else {
                em->rotY = a & 0xFFFF;
                EmVelociprey_ChangeState(em, 5, 1, 3);
            }
        }
        break;
    case 5:
        if (em->stateGroup != 4 || em->state != 1) {
            EmVelociprey_ChangeState(em, 4, 1, 3);
        }
        break;
    case 6:
        if ((em->stateGroup != 4 || em->state != 1) && (em->stateGroup != 0 || em->state != 6)) {
            Em_SetStatus4Timer(em);
            EmVelociprey_ChangeState(em, 4, 6, 3);
        }
        break;
    case 8:
        if ((em->stateGroup != 4 || em->state != 1) && (em->stateGroup != 0 || em->state != 6)) {
            Em_SetStatus2Timer(em);
            EmVelociprey_ChangeState(em, 0, 7, 3);
        }
        break;
    case 10:
        EmVelociprey_ChangeState(em, 0, 8, 3);
        break;
    case 12: {
        u16 a = em->hitAngle;
        s32 d = (a - em->rotY) & 0xFFFF;
        if (d >= 0x6000 && d < 0xA000) {
            em->rotY = a + 0x8000;
            EmVelociprey_ChangeState(em, 4, 1, 3);
        } else if (d >= 0x2000 && d < 0x6000) {
            em->rotY = a - 0x4000;
            EmVelociprey_ChangeState(em, 4, 3, 3);
        } else if (d >= 0xA000 && d < 0xE000) {
            em->rotY = a + 0x4000;
            EmVelociprey_ChangeState(em, 4, 4, 3);
        } else {
            em->rotY = a & 0xFFFF;
            EmVelociprey_ChangeState(em, 4, 2, 3);
        }
        break;
    }
    case 14:
        em->scriptPending = 0;
        if (em->stance == 2) {
            if (em->hitAngle >= 0x6000 && em->hitAngle < 0xA000) {
                em->rotY = em->hitAngle + 0x8000;
                EmVelociprey_ChangeState(em, 4, 1, 3);
            } else if (em->hitAngle >= 0x2000 && em->hitAngle < 0x6000) {
                em->rotY = em->hitAngle - 0x4000;
                EmVelociprey_ChangeState(em, 4, 3, 3);
            } else if (em->hitAngle >= 0xA000 && em->hitAngle < 0xE000) {
                em->rotY = em->hitAngle + 0x4000;
                EmVelociprey_ChangeState(em, 4, 4, 3);
            } else {
                em->rotY = em->hitAngle & 0xFFFF;
                EmVelociprey_ChangeState(em, 4, 2, 3);
            }
        } else {
            EmVelociprey_ChangeState(em, 4, 0, 3);
        }
        break;
    case 17:
        if ((em->stateGroup != 4 || em->state != 8) && (em->stateGroup != 4 || em->state != 1)) {
            Em_SetStatus5Timer(em);
            EmVelociprey_ChangeState(em, 4, 8, 3);
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

void EmVelociprey_RunState(Em *em, s32 work)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->stateGroup) {
    case 0:
        EmVelociprey_RunGroup0(em, (EmWork *) work);
        break;
    case 1:
        EmVelociprey_RunGroup1(em, (EmWork *) work);
        break;
    case 2:
        EmVelociprey_RunGroup2(em, (EmWork *) work);
        break;
    case 3:
        EmVelociprey_RunGroup3(em, (EmWork *) work);
        break;
    case 4:
        EmVelociprey_RunGroup4(em, (EmWork *) work);
        break;
    case 5:
        EmVelociprey_RunGroup5(em, (EmWork *) work);
        break;
    case 6:
        EmVelociprey_RunGroup6(em, (EmWork *) work);
        break;
    case 7:
        EmVelociprey_RunGroup7(em, (EmWork *) work);
        
    default:
        break;
    }
}

void EmVelociprey_MotionEffectsDefault(Em *em)
{
}

void EmVelociprey_MotionEffects(Em *em, EmWork *work)
{
    f32 pos[3];

    if (em->motionId[0] != work->prevMotion) {
        work->prevMotion = em->motionId[0];
    }
    EmVelociprey_MotionSounds((s32) em, work);
    switch (work->prevMotion) {
    case 0x3EB:
        if (Player_PassedFrameScaled(em, 18.0f, 0U) != 0) {
            Effect0D_SpawnForEm(em, 1, 0);
        }
        break;
    case 0x3EC:
        if (Player_PassedFrameScaled(em, 20.0f, 0U) != 0) {
            Effect0D_SpawnForEm(em, 1, 0);
        }
        break;
    case 0x3F0:
        if (Player_PassedFrameScaled(em, 12.0f, 0U) != 0) {
            Effect0D_SpawnForEm(em, 1, 0);
        }
        break;
    case 0x3F1:
    case 0x3F2:
        if (Player_PassedFrameScaled(em, 14.0f, 0U) != 0) {
            Effect0D_SpawnForEm(em, 1, 0);
        }
        break;
    case 0x3FF:
        if (Player_PassedFrameScaled(em, 32.0f, 0U) != 0) {
            EffVelociprey13_Create(em, 2);
        }
        if (Player_PassedFrameScaled(em, 22.0f, 0U) != 0) {
            Effect0D_SpawnForEm(em, 0x1B, 0);
        }
        break;
    case 0x414:
        pos[0] = 0.0f;
        pos[1] = 10.0f;
        pos[2] = 40.0f;
        Em_SpawnEffect06Periodic(em, 0xF, pos, 1.0f);
        break;
    default:
        EmVelociprey_MotionEffectsDefault(em);
        break;
    case 0x3E9:
    case 0x3EA:
    case 0x3ED:
    case 0x3EE:
    case 0x3EF:
    case 0x3F3:
    case 0x3F4:
    case 0x3FC:
    case 0x3FD:
    case 0x3FE:
    case 0x400:
    case 0x401:
    case 0x402:
    case 0x406:
    case 0x407:
    case 0x408:
    case 0x409:
    case 0x40A:
    case 0x40B:
    case 0x40C:
    case 0x40D:
    case 0x40E:
    case 0x40F:
    case 0x412:
    case 0x413:
    case 0x416:
    case 0x417:
    case 0x41A:
    case 0x41B:
    case 0x41C:
    case 0x41D:
        break;
    }
}

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

s32 EmVelociprey_ScriptQuery(Em *em, u8 flag)
{
    switch (flag) {
    case 1:
        if (EmList_FindKindNear(em, 1, 1, em->nearRadius) == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

void EmVelociprey_TableNop(void)
{
}

void EmVelociprey_SetGroup0State(Em *em, u16 state, u8 mode)
{
    switch (state) {
    case 1:
        if (em->combat == 1 && em->hp >= (s16) (0.3f * em->hpMax)) {
            state = 5;
        }
        break;
    case 13:
        if (em->combat == 1 && em->curGoalKind == 1 && em->curGoalArg == 0 && em->curGoalNo != -1 &&
            em->stateRand % 2 == 0 && !(((f32 *) em->playerDist)[em->curGoalNo] <= em->nearRadius)) {
            state = 5;
        }
        break;
    case 16:
        Em_EndStatus5(em);
        break;
    }
    em->stance = 0;
    Em_SetStateSync(em, 0, state, mode);
}

void EmVelociprey_SetGroup1State(Em *em, u16 state, u8 mode)
{
    em->stance = 0;
    switch (state) {
    case 0:
        if (em->combat == 1) {
            state = 5;
        }
        break;
    case 1:
        Em_SetSpeedScaled(em, 2, 1000.0f, 300.0f);
        break;
    }
    Em_SetStateSync(em, 1, state, mode);
}

void EmVelociprey_SetGroup2State(Em *em, u16 state, u8 mode)
{
    Em_SetStateSync(em, 2, state, mode);
}

void EmVelociprey_SetGroup3State(Em *em, u16 state, u8 mode)
{
    switch (state) {
    case 0:
    case 2:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 3:
    case 1:
        em->hasGoal = 0;
        if ((em->curGoalKind == 1) && (em->targetNo != -1)) {
            em->hasGoal = 1;
        }
        break;
    }
    Em_SetStateSync(em, 0x3, state, mode);
}

void EmVelociprey_ChangeState(Em *em, u16 group, u16 state, u8 mode)
{
    EmVelocipreyWork *w = (EmVelocipreyWork *)&em->work;

    if ((u8)w->countdown != 0) {
        ObjListA70_Remove((EmObj *)((&w->sense)));
        w->countdown = 0;
    }
    switch (group) {
    case 0:
        EmVelociprey_SetGroup0State(em, state, mode);
        break;
    case 1:
        EmVelociprey_SetGroup1State(em, state, mode);
        break;
    case 2:
        EmVelociprey_SetGroup2State(em, state, mode);
        break;
    case 3:
        EmVelociprey_SetGroup3State(em, state, mode);
        break;
    case 6:
        Em_SetStateSync(em, group, state, mode);
        break;
    case 5:
        if (state == 4 && QuestEm_CanAppear(em) == 1) {
            QuestEm_Appear(em);
        }
        Em_SetStateSync(em, group, state, mode);
        break;
    case 4:
        switch (state) {
        case 8:
            Em_BeginStatus5(em);
            break;
        }
        Em_SetStateSync(em, group, state, mode);
        break;
    case 7:
        Em_SetStateSync(em, group, state, mode);
        break;
    }
}

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
        w->curveFrame = 0x16;
        break;
    case 1:
        w->curveVelY = 1;
        w->curveFrame = 0x16;
        break;
    case 2:
        w->curveVelY = 1;
        w->curveSpeed = 1;
        w->curveFrame = 0x16;
        break;
    case 3:
        w->curveVelY = 1;
        w->curveFrame = 0x16;
        break;
    }
    Player_ClearVelocity(em);
}


static s32 EmVelociprey_CurveNone(Em *em, EmWork *work)
{
    return 0;
}

extern EmCurveKey *EmVelociprey_VelYCurves[];

/* em->velY (vertical speed) from curve work[0x17] at the frame in the work area: the first
 * key's value at frames 0 and 1; 2 at the end of the table (falling faster then, to -50). */
static u8 EmVelociprey_CurveVelY(Em *em, EmWork *work)
{
    EmVelocipreyWork *w = (EmVelocipreyWork *) work;
    EmCurveKey *p;
    f32 frame;
    f32 t;
    s32 i;
    EmCurveKey *tbl;
    u8 ret;

    tbl = EmVelociprey_VelYCurves[((EmVelocipreyWork *)work)->curveKind];
    ret = 0;
    if (tbl == NULL) {
        return ret;
    }
    frame = w->curveFrame;
    if (frame == 0.0f || frame == 1.0f) {
        em->velY = tbl->v;
    } else {
        i = 1;
        while (i != 0) {
            p = &tbl[i];
            t = p->t;
            if (frame > t && t != 0.0f) {
                i++;
            } else if (t == 0.0f) {
                ret = 2;
                if (em->velY > -50.0f) {
                    em->velY -= 1.0f;
                }
                break;
            } else {
                i = 0;
                em->velY = (p->v - p[-1].v) / ((t - p[-1].t) / em->motion[0].step);
            }
        }
    }
    return ret;
}

extern EmCurveKey *EmVelociprey_SpeedCurves[];

/* em->velZ (horizontal speed) from curve work[0x17]; 4 at the end of the table; stopped
 * while em->unk698 has 0xF000000F set. */
static u8 EmVelociprey_CurveSpeed(Em *em, EmWork *work)
{
    EmVelocipreyWork *w = (EmVelocipreyWork *) work;
    EmCurveKey *p;
    f32 frame;
    f32 t;
    s32 i;
    EmCurveKey *tbl;
    u8 ret;

    tbl = EmVelociprey_SpeedCurves[((EmVelocipreyWork *)work)->curveKind];
    ret = 0;
    if (tbl == NULL) {
        return ret;
    }
    frame = w->curveFrame;
    if (frame == 0.0f || frame == 1.0f) {
        em->velZ = tbl->v;
    } else {
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
    if (em->wallHitDirs & 0xF000000F) {
        *(s32 *) &em->velZ = 0;
    }
    return ret;
}

u8 EmVelociprey_UpdateCurves(Em *em)
{
    EmVelocipreyWork *w = (EmVelocipreyWork *)&em->work;
    u8 ret = 0;

    if (w->curveNone != 0) {
        ret = EmVelociprey_CurveNone(em, (EmWork *) w);
    }
    if (w->curveVelY != 0) {
        ret |= EmVelociprey_CurveVelY(em, (EmWork *) w);
    }
    if (w->curveSpeed != 0) {
        ret |= EmVelociprey_CurveSpeed(em, (EmWork *) w);
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

void EmAnteka_MotionSounds(Em *em, EmWork *work)
{
    s16 motion;

    motion = work->fxMotion;
    switch (motion) {
    case 0x3E9:
        break;
    case 0x3EA:
        Em_PlaySeAtBoneOnFrame(em, 0x5C, 0xD, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x7A, 0xD, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x92, 0xD, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0xB6, 0xD, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0xD2, 0xD, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0xF0, 0xD, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x10E, 0xD, 0x17);
        break;
    case 0x3EB:
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0xB, 0x17);
        break;
    case 0x3EC:
        Em_PlaySeAtBoneOnFrame(em, 6, 4, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0, 0);
        break;
    case 0x3ED:
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x10, 4, 0x11, 4), 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x54, Sound_RandomSe(0x11, 4, 0x12, 4), 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0xC8, Sound_RandomSe(0x10, 4, 0x12, 4), 0x17);
        break;
    case 0x3EE:
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0xE, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 0xE, 0x17);
        break;
    case 0x3F1:
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 0, 0);
        break;
    case 0x3F2:
        Em_PlaySeAtBoneOnFrame(em, 6, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x13, 0);
        break;
    case 0x3F3:
        Em_PlaySeAtBoneOnFrame(em, 0x10, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0x13, 0);
        break;
    case 0x3F4:
    case 0x3F5:
        Em_PlaySeAtBoneOnFrame(em, 0xC, 1, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0x13, 0);
        break;
    case 0x3F6:
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 0, 0);
        break;
    case 0x3F7:
    case 0x3FE:
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 1, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 0x13, 0);
        break;
    case 0x3F8:
        Em_PlaySeAtBoneOnFrame(em, 4, 7, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 8, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 9, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x94, 0xA, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0, 0);
        break;
    case 0x3F9:
        Em_PlaySeAtBoneOnFrame(em, 0xE, 2, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 1, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0, 0);
        break;
    case 0x3FA:
        Em_PlaySeAtBoneOnFrame(em, 0x18, 1, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 0x13, 0);
        break;
    case 0x3FC:
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0x13, 0);
        break;
    case 0x400:
        Em_PlaySeAtBoneOnFrame(em, 0xC, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x36, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 0, 0);
        break;
    case 0x40E:
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 1, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x11, 0);
        break;
    case 0x40F:
        Em_PlaySeAtBoneOnFrame(em, 2, 0xB, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x17, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x5A, 2, 0);
        break;
    case 0x424:
    case 0x425:
        Em_PlaySeAtBoneOnFrame(em, 4, 5, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 0, 0);
        break;
    case 0x426:
        Em_PlaySeAtBoneOnFrame(em, 4, 1, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 4, 5, 0);
        break;
    case 0x427:
        if ((em->stateGroup == 5) && (em->state == 0)) {
            Em_PlaySeAtBoneOnFrame(em, 0x3E, 0xC, 0);
            Em_PlaySeAtBoneOnFrame(em, 4, 6, 0);
            Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x14, 0);
            break;
        }
        Em_PlaySeAtBoneOnFrame(em, 4, 6, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x14, 0);
        break;
    case 0x428:
        Em_PlaySeAtBoneOnFrame(em, 0xE, 1, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 1, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 0, 0);
        break;
    case 0x429:
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x42, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x7C, 0x14, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x62, 1, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 4, 0);
        break;
    case 0x42B:
        break;
    case 0x42C:
        Em_PlaySeAtBoneOnFrame(em, 4, 0xF, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x8A, 0xF, 0x17);
        break;
    case 0x42D:
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x10, 4, 0x11, 4), 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0, 0);
        
        break;
    }
}

void EmAnteka_Init(Em *em)
{
    EmWork *w = (EmWork *)&em->work;
    s16 hp;

    if (gQuest.stage == 0) {
        switch (gScene.area) {
        case 0x25:
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
    em->stance = 0;
    em->motionSpeed = 1.0f;
    hp = Em_ComputeSize(em, 0);
    em->hp = hp;
    em->hpMax = hp;
    if (em->type == 3) {
        if (gQuest.stage == 0) {
            em->gender = (u16) Rand_Next(1) & 1;
            em->gender = 0;
        } else {
            em->gender = em->variant;
        }
        if (em->gender != 0) {
            hp = Em_ComputeSize(em, 1);
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
    Em_SetMotionRow(em, 1, 0, 0);
    if (em->gender != 0) {
        Em_SetLayerMotion(em, 0x4B1, 0, 0, 1);
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

void EmAnteka_Start(Em *em)
{
    EmAnteka_Init(em);
    EmAnteka_ChangeState(em, 0, 1, 0);
}

void EmAnteka_Reinit(Em *em)
{
    EmAnteka_Init(em);
}

void EmAnteka_Update(Em *em)
{
    u8 found1 = 0;
    u8 inGroup6 = 0;
    u8 found0 = 0;
    s16 i;
    Em *m = gEms;
    u8 buf[3];

    Em_UpdateCombatTimer(em);
    if (em->requested != 1 && em->stateGroup != 6) {
        for (i = 0; i < 20; i++, m++) {
            if (m->active && m->visible && (Obj_SameArea(em, (Player *) m) & 0xFF)) {
                if (gEmIsLarge2[m->type] != 0) {
                    if (m->combat == 1) {
                        found1 = 1;
                        break;
                    }
                    if (m->combat == 0) {
                        found0 = 1;
                    }
                }
                if (m->variant == 0 && m->stateGroup == 6) {
                    inGroup6 = 1;
                }
            }
        }
        if (found1 != 0 || (found0 != 0 && inGroup6 != 0)) {
            Em_SetRequest(em, 1, 0x90);
            Em_RestartScript(em);
            em->scriptPending = 1;
        }
    }
    switch (Em_UpdateDamage(em, buf) & 0xFF) {
    case 0:
    case 3:
    case 4:
    case 5:
    case 7:
    case 9:
    case 11:
    case 15:
    case 16:
    case 18:
        break;
    case 1:
    case 2:
        EmAnteka_ChangeState(em, 5, 0, 3);
        break;
    case 6:
        Em_SetStatus4Timer(em);
        EmAnteka_ChangeState(em, 4, 3, 3);
        break;
    case 8:
        Em_SetStatus2Timer(em);
        EmAnteka_ChangeState(em, 0, 8, 3);
        break;
    case 10:
        EmAnteka_ChangeState(em, 4, 5, 3);
        break;
    case 12:
    case 13:
        {
            s32 d = em->hitAngle - em->rotY;

            if (((d - 0x4000) & 0xFFFF) < 0x8000 && em->parts[0].hp > 0 && em->stance == 0) {
                if ((d & 0xFFFF) < 0x8000) {
                    EmAnteka_ChangeState(em, 4, 0, 3);
                } else {
                    EmAnteka_ChangeState(em, 4, 1, 3);
                }
            } else {
                EmAnteka_ChangeState(em, 4, 2, 3);
            }
        }
        break;
    case 14:
        if (em->stance == 0) {
            s32 d2 = (em->hitAngle - em->rotY) & 0xFFFF;

            em->scriptPending = 0;
            if (d2 < 0x8000) {
                EmAnteka_ChangeState(em, 4, 0, 3);
            } else {
                EmAnteka_ChangeState(em, 4, 1, 3);
            }
        }
        break;
    case 17:
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

void EmAnteka_State0_00(void)
{
}

void EmAnteka_State0_01(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, 1, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
}

void EmAnteka_State0_02(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, 2, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
}

void EmAnteka_State0_03(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, 3, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
}

void EmAnteka_State0_04(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, 4, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
}

void EmAnteka_State0_05(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, 5, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
}

void EmAnteka_State0_06(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, 6, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
}

void EmAnteka_State0_07(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->targetAngle = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        if (((em->targetAngle - em->rotY) & 0xFFFF) < 0x8001) {
            EmAnteka_SetMotion(em, 7, 0, 0);
            break;
        }
        EmAnteka_SetMotion(em, 8, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
}

void EmAnteka_State0_08(Em *em, EmWork *work)
{
    Em_SetHitReactMode(em, 2U);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        Em_BeginStatus2(em);
        Em_SetMotion(em, 0x41, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x43, 0, 0);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            EmAnteka_ChangeState(em, 0, 9, 4);
        }
        break;
    }
}

void EmAnteka_State0_09(Em *em, EmWork *work)
{
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        Em_EndStatus2(em);
        Em_SetMotion(em, 0x40, 0, 0);
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

void EmAnteka_State0_0A(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x45, 0, 0);
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

void EmAnteka_State0_0B(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, 0x26, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
}

void EmAnteka_RunGroup0(Em *em, EmWork *work)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->state) {
    case 0:
        EmAnteka_State0_00();
        break;
    case 1:
        EmAnteka_State0_01(em, work);
        break;
    case 2:
        EmAnteka_State0_02(em, work);
        break;
    case 3:
        EmAnteka_State0_03(em, work);
        break;
    case 4:
        EmAnteka_State0_04(em, work);
        break;
    case 5:
        EmAnteka_State0_05(em, work);
        break;
    case 6:
        EmAnteka_State0_06(em, work);
        break;
    case 7:
        EmAnteka_State0_07(em, work);
        break;
    case 8:
        EmAnteka_State0_08(em, work);
        break;
    case 9:
        EmAnteka_State0_09(em, work);
        break;
    case 10:
        EmAnteka_State0_0A(em, work);
        break;
    case 11:
        EmAnteka_State0_0B(em, work);
        
    default:
        break;
    }
}

void EmAnteka_State1_00(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->targetAngle = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        EmAnteka_SetMotion(em, 9, 0, 0);
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

void EmAnteka_State1_01(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, 9, 0, 0);
        break;
    case 1:
        if (Em_StepToGoal(em, 0x40, 0) == 1) {
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
}

void EmAnteka_State1_02(Em *em, EmWork *work, s32 unused)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, 0xA, 0, 0);
        break;
    case 1:
        if ((Em_StepToGoal(em, 0x40, 0) == 1) && (em->motion[0].playing == 0)) {
            em->step++;
            EmAnteka_SetMotion(em, 0xE, 4, 0);
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

void EmAnteka_State1_03(Em *em, EmWork *work, s32 unused)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stateVar0 = 0;
        EmAnteka_SetMotion(em, 0xB, 0, 0);
        break;
    case 1:
        if ((em->type == 0x45) && (em->stateVar0 == 0) && (Player_PassedFrameScaled(em, 20.0f, 0U) != 0)) {
            Eff02_Create(em, 0x16);
            em->stateVar0++;
        }
        if ((Em_StepToGoal(em, 0x40, 0) == 1) && (em->motion[0].playing == 0)) {
            em->step++;
            EmAnteka_SetMotion(em, 0xE, 4, 0);
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

void EmAnteka_State1_04(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, 0xC, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_SetMotion(em, 0xE, 0, 0x1A);
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

void EmAnteka_State1_05(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, 0xD, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_SetMotion(em, 0xE, 0, 0x1A);
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

void EmAnteka_State1_06(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, 0xF, 0, 0);
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

void EmAnteka_State1_07(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, 0x16, 0, 0);
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

void EmAnteka_State1_08(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, 0x12, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
}

void EmAnteka_State1_0B(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, 0xC, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_SetMotion(em, 0xD, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_SetMotion(em, 0xC, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_SetMotion(em, 0xD, 0, 0);
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

void EmAnteka_State1_0C(Em *em, EmWork *work)
{
    f32 frame;
    u32 step;
    u32 diff;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        em->targetAngle = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        diff = (em->targetAngle - em->rotY) & 0xFFFF;
        if ((em->type == 0x45 && diff <= 0x4000U) || diff >= 0xC000U || (em->type != 0x45 && diff <= 0x3000U) || diff >= 0xD000U) {
            if (em->type == 0x45) {
                if (diff >= 0x8000U) {
                    EmAnteka_SetMotion(em, 8, 0, 0);
                } else {
                    EmAnteka_SetMotion(em, 7, 0, 0);
                }
            } else {
                EmAnteka_SetMotion(em, 0x12, 0, 0);
            }
        } else if (diff >= 0x8000U) {
            EmAnteka_SetMotion(em, 0x16, 0, 0);
        } else {
            EmAnteka_SetMotion(em, 0xF, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].blend != 0) {
            break;
        }
        switch (em->motionId[0]) {
        case 0x3FA:
            frame = 26.0f;
            step = 0x4000U / (u32) (frame / 2.0f);
            break;
        case 0x3EF:
        case 0x3F0:
            frame = 48.0f;
            step = 0x4000U / (u32) (frame / 2.0f);
            break;
        case 0x3F7:
        case 0x3FE:
        default:
            frame = 30.0f;
            step = 0x8000U / (u32) (frame / 2.0f);
            break;
        }
        diff = (em->targetAngle - (em->rotY & 0xFFFF)) & 0xFFFF;
        if ((u32) ((diff + step) & 0xFFFF) < step * 2) {
            em->rotY = em->targetAngle;
        } else if (diff < 0x8000U) {
            em->rotY = (em->rotY + step) & 0xFFFF;
        } else {
            em->rotY = (em->rotY - step) & 0xFFFF;
        }
        if (EmMotion_ReachedFrame(em, 0, frame) != 0) {
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

void EmAnteka_State1_0D(Em *em, EmWork *work)
{
    u8 ended;
    u8 step;

    step = em->step;
    ended = 0;
    switch (step) {
    case 0:
        em->step = step + 1;
        em->stance = 0;
        Em_SetMotion(em, 0x14, 0, 0);
        Player_ClearVelocity(em);
        em->velY = 75.0f;
        em->accelY = -10.0f;
        em->velZ = 50.0f;
        work->accelAngX = 0;
        work->accelAngZ = 0;
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 26.0f) != 0) {
            em->stance = 2;
            ended = 1;
            work->accelAngY = em->rotY;
            Em_MoveRotatedAccel(em, &work->accelAngX);
        }
        if (EmMotion_ReachedFrame(em, 0, 36.0f) != 0) {
            em->motionSpeed = 0.0f;
        }
        Em_GetFloorY(em, &em->posX, &em->floorY);
        if (ended && (em->posY <= em->floorY)) {
            em->step++;
            em->stance = 0;
            em->posY = em->floorY;
            em->motionSpeed = 1.0f;
        }
        break;
    case 2:
        if (EmMotion_ReachedFrame(em, 0, 44.0f) != 0) {
            em->step++;
            Em_SetMotion(em, 0xE, 4, 0xA);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step = step + 1;
            EmAnteka_EndState(em);
        }
        break;
    }
}

void EmAnteka_RunGroup1(Em *em, EmWork *work)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->state) {
    case 0:
        EmAnteka_State1_00(em);
        break;
    case 1:
        EmAnteka_State1_01(em);
        break;
    case 2:
        EmAnteka_State1_02(em, work, 0);
        break;
    case 3:
        EmAnteka_State1_03(em, work, 0);
        break;
    case 4:
        EmAnteka_State1_04(em);
        break;
    case 5:
        EmAnteka_State1_05(em);
        break;
    case 6:
        EmAnteka_State1_06(em);
        break;
    case 7:
        EmAnteka_State1_07(em);
        break;
    case 8:
        EmAnteka_State1_08(em);
        break;
    case 9:
        EmAnteka_State1_02(em, work, 1);
        break;
    case 0xA:
        EmAnteka_State1_03(em, work, 1);
        break;
    case 0xB:
        EmAnteka_State1_0B(em);
        break;
    case 0xC:
        EmAnteka_State1_0C(em, work);
        break;
    case 0xD:
        EmAnteka_State1_0D(em, work);
        break;
    }
}

void EmAnteka_State3_00(Em *em, EmWork *work)
{
    u8 type;

    switch (em->step) {
    case 0:
        em->step++;
        type = em->type;
        if ((type == 3) || (type == 0x45)) {
            EmAnteka_SetMotion(em, 0x10, 0, 0);
            break;
        }
        EmAnteka_SetMotion(em, 0x26, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
}

void EmAnteka_State3_01(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, 0x11, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_SetMotion(em, 0xE, 0, 0x1A);
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

void EmAnteka_State3_02(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, 0x14, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_SetMotion(em, 0x18, 0, 8);
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

void EmAnteka_State3_03(Em *em, EmWork *work)
{
    EmAnteka_EndState(em);
}

void EmAnteka_State3_04(Em *em, EmWork *work)
{
    EmAnteka_EndState(em);
}

void EmAnteka_State3_05(Em *em, EmWork *work)
{
    EmAnteka_EndState(em);
}

void EmAnteka_State3_06(Em *em, EmWork *work)
{
    EmAnteka_EndState(em);
}

void EmAnteka_State3_07(Em *em, EmWork *work)
{
    EmAnteka_EndState(em);
}

void EmAnteka_State3_08(Em *em, EmWork *work)
{
    EmAnteka_EndState(em);
}

void EmAnteka_RunGroup3(Em *em, EmWork *work)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->state) {
    case 0:
        EmAnteka_State3_00(em, work);
        break;
    case 1:
        EmAnteka_State3_01(em, work);
        break;
    case 2:
        EmAnteka_State3_02(em, work);
        break;
    case 3:
        EmAnteka_State3_03(em, work);
        break;
    case 4:
        EmAnteka_State3_04(em, work);
        break;
    case 5:
        EmAnteka_State3_05(em, work);
        break;
    case 6:
        EmAnteka_State3_06(em, work);
        break;
    case 7:
        EmAnteka_State3_07(em, work);
        break;
    case 8:
        EmAnteka_State3_08(em, work);
        
    default:
        break;
    }
}

void EmAnteka_State4_00(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, 0x3C, 0, 0);
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

void EmAnteka_State4_01(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, 0x3D, 0, 0);
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

void EmAnteka_State4_02(Em *em, EmWork *work, s16 variant)
{
    switch (variant) {
    case 1:
        em->blind = 2;
        break;
    }
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, 0x3E, 0, 0);
        switch (variant) {
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
        em->stance = 2;
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
            em->stance = 0;
            EmAnteka_SetMotion(em, 0x3F, 0, 0);
            em->timer = 0x5A;
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            EmAnteka_SetMotion(em, 0x40, 0, 0);
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

void EmAnteka_State4_03(Em *em)
{
    Em_SetHitReactMode(em, 4U);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x44, 0, 0);
        Em_BeginStatus4(em);
        em->noReact = 1;
        Em_RestartScript(em);
        break;
    case 1:
        Em_SpawnEffect06Bone(em, 2);
        if (--em->timer <= 0) {
            em->step++;
            EmAnteka_ChangeState(em, 0, 0xA, 4);
        }
        break;
    }
}

void EmAnteka_State4_04(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, 0x41, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_SetMotion(em, 0x40, 0, 0);
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

void EmAnteka_RunGroup4(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmAnteka_State4_00(em);
        break;
    case 1:
        EmAnteka_State4_01(em);
        break;
    case 2:
        EmAnteka_State4_02(em, work, 0);
        break;
    case 3:
        EmAnteka_State4_03(em);
        break;
    case 4:
        EmAnteka_State4_04(em);
        break;
    case 5:
        EmAnteka_State4_02(em, work, 1);
        break;
    }
}

void EmAnteka_State5_00_Hunted(Em *em, EmWork *work)
{
    em->noHitTimer = 0xA;
    em->noPushTimer = 0xA;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, 0x3E, 0, 0);
        em->rotY = em->hitAngle + 0x8000;
        em->motionSpeed = 1.0f;
        Player_ClearVelocity(em);
        em->velX = 0.0f;
        em->velY = 17.0f;
        em->velZ = -23.0f;
        em->accelY = -3.15f;
        em->accelZ = 0.08f;
        em->stance = 2;
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
            em->stance = 0;
            EmAnteka_SetMotion(em, 0x3F, 0, 0);
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
            EmAnteka_ChangeState(em, 5, 4, 4);
        }
        break;
    }
    if (em->posY < em->floorY) {
        em->posY = em->floorY;
    }
}

void EmAnteka_State5_04(Em *em)
{
    switch (em->step) {
    case 0:
        if (QuestEm_CanAppear(em) == 1) {
            EmAnteka_ChangeState(em, 0, 1, 0);
            break;
        }
        em->taskPhase++;
        em->visible = 0;
        break;
    }
}

void EmAnteka_State5_05(Em *em)
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

void EmAnteka_RunGroup5(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmAnteka_State5_00_Hunted(em, work);
        break;
    case 4:
        EmAnteka_State5_04(em);
        break;
    case 5:
        EmAnteka_State5_05(em);
        break;
    }
}

void EmAnteka_State6_00(Em *em)
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

void EmAnteka_State6_01(Em *em)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 0;
        em->posX = 14419.0f;
        em->posY = 0.0f;
        em->posZ = 15007.0f;
        em->rotX = 0;
        em->rotY = 0x8000;
        em->rotZ = 0;
        Em_SetMotion(em, 2, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 3, 6, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 0x96;
            Em_SetMotion(em, 0xB, 6, 0);
        }
        break;
    case 3:
        em->step++;
        EmAnteka_EndState(em);
        break;
    }
}

void EmAnteka_State6_02(Em *em)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->motionSpeed = 1.0f;
        Em_SetMotion(em, 1, 0, 0);
        break;
    case 1:
        EmAnteka_EndState(em);
        break;
    }
}

void EmAnteka_RunGroup6(Em *em)
{
    switch (em->state) {
    case 0:
        EmAnteka_State6_00(em);
        break;
    case 1:
        EmAnteka_State6_01(em);
        break;
    case 2:
        EmAnteka_State6_02(em);
        break;
    }
}

void EmAnteka_RunState(Em *em)
{
    u8 *w = &em->work;

    switch (em->stateGroup) {
    case 0:
        EmAnteka_RunGroup0(em, (EmWork *) w);
        break;
    case 1:
        EmAnteka_RunGroup1(em, (EmWork *) w);
        break;
    case 2:
        EmAnteka_RunGroup0(em, (EmWork *) w);
        break;
    case 3:
        EmAnteka_RunGroup3(em, (EmWork *) w);
        break;
    case 4:
        EmAnteka_RunGroup4(em, (EmWork *) w);
        break;
    case 5:
        EmAnteka_RunGroup5(em, (EmWork *) w);
        break;
    case 6:
        EmAnteka_RunGroup6(em);
        break;
    case 7:
        EmAnteka_RunGroup6(em);
        
    default:
        break;
    }
}

void EmAnteka_ChangeState(Em *em, u16 group, u16 state, u8 mode)
{
    f32 f;

    if (em->remote == 0) {
        Em_TargetPlayerInRange(em);
    }
    switch (group) {
    case 1:
        switch (state) {
        case 0:
            Em_SetSpeedScaled(em, 0, 1000.0f, 100.0f);
            break;
        case 1:
        case 2:
        case 3:
            f = 100.0f;
            goto move;
        case 9:
        case 10:
            f = 300.0f;
        move:
            Em_SetSpeedScaled(em, 0, 1000.0f, f);
            break;
        }
        break;
    case 5:
        if (state == 4 && QuestEm_CanAppear(em) == 1) {
            QuestEm_Appear(em);
        }
        break;
    case 0:
    case 2:
    case 3:
    case 4:
    case 6:
    case 7:
    case 8:
        break;
    }
    em->motionSpeed = 1.0f;
    Em_SetStateSync(em, group, state, mode);
}

void EmAnteka_EndState(Em *em)
{
    em->motionSpeed = 1.0f;
    em->stance = 0;
    em->lookAtTarget = 0;
    em->scriptPending = 1;
    EmAnteka_ChangeState(em, 0, 1, 0);
}

void EmAnteka_SetMotion(Em *em, s32 motion, s32 blend, s32 frame)
{
    s32 layerMotion;

    Em_SetMotion(em, motion, blend, frame);
    if (em->gender != 0) {
        switch (em->motionId[0]) {
        case 0x3E9:
            layerMotion = 0x4B1;
            break;
        case 0x3EA:
            layerMotion = 0x4B2;
            break;
        case 0x3EB:
            layerMotion = 0x4B3;
            break;
        case 0x3ED:
            layerMotion = 0x4B5;
            break;
        case 0x3F1:
            layerMotion = 0x4B9;
            break;
        case 0x3F2:
            layerMotion = 0x4BA;
            break;
        case 0x3F3:
            layerMotion = 0x4BB;
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

void EmAnteka_MotionEffects(Em *em, EmWork *work)
{
    f32 pos[3];

    if (em->motionId[0] != work->fxMotion) {
        work->fxMotion = em->motionId[0];
    }
    EmAnteka_MotionSounds(em, work);
    switch (work->fxMotion) {
    case 0x3E9:
    case 0x3EA:
    case 0x3EB:
    case 0x3EC:
    case 0x3ED:
    case 0x3EE:
    case 0x3F1:
        break;
    case 0x3F2:
        if (Player_PassedFrameScaled(em, 4.0f, 0U) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x15, 3, 0.6f);
        }
        if (Player_PassedFrameScaled(em, 28.0f, 0U) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x11, 3, 0.6f);
        }
        break;
    case 0x3F3:
        if (Player_PassedFrameScaled(em, 12.0f, 0U) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x15, 3, 0.7f);
        }
        if (Player_PassedFrameScaled(em, 18.0f, 0U) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x11, 3, 0.7f);
        }
        break;
    case 0x3F4:
    case 0x3F5:
        if (work->fxMotion == 0x3F7) {
            if (Player_PassedFrameScaled(em, 12.0f, 0U) != 0) {
                Effect0D_SpawnForEmScaled(em, 0x15, 3, 0.9f);
            }
        } else if (Player_PassedFrameScaled(em, 12.0f, 0U) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x11, 3, 0.9f);
        }
        break;
    case 0x3F6:
        if (Player_PassedFrameScaled(em, 12.0f, 0U) != 0) {
            Effect0D_SpawnForEmScaled(em, 0xB, 3, 0.6f);
        }
        if (Player_PassedFrameScaled(em, 16.0f, 0U) != 0) {
            Effect0D_SpawnForEmScaled(em, 0xB, 3, 0.6f);
        }
        if (Player_PassedFrameScaled(em, 12.0f, 0U) != 0) {
            Effect0D_SpawnForEmScaled(em, 7, 3, 0.6f);
        }
        if (Player_PassedFrameScaled(em, 16.0f, 0U) != 0) {
            Effect0D_SpawnForEmScaled(em, 7, 3, 0.6f);
        }
        break;
    case 0x3F7:
    case 0x3FE:
        if (work->fxMotion == 0x3F7) {
            if (Player_PassedFrameScaled(em, 34.0f, 0U) != 0) {
                Effect0D_SpawnForEmScaled(em, 0x15, 3, 0.6f);
            }
        } else if (Player_PassedFrameScaled(em, 34.0f, 0U) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x11, 3, 0.6f);
        }
        break;
    case 0x3F8:
        if (em->type == 3 || em->type == 0x45) {
            if (Player_PassedFrameScaled(em, 64.0f, 0U) != 0) {
                Eff02_Create(em, 0x11);
            }
            if (Player_PassedFrameScaled(em, 80.0f, 0U) != 0) {
                Eff02_Create(em, 0x12);
            }
            if (Player_PassedFrameScaled(em, 146.0f, 0U) != 0) {
                Eff02_Create(em, 0x13);
            }
        }
        break;
    case 0x3F9:
        if (Player_PassedFrameScaled(em, 20.0f, 0U) != 0) {
            if (em->type == 3 || em->type == 0x45) {
                Eff02_Create(em, 0x15);
            } else {
                Eff02_Create(em, 0xB);
            }
        }
        if (Player_PassedFrameScaled(em, 24.0f, 0U) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x16, 3, 0.9f);
        }
        break;
    case 0x3FA:
        if (Player_PassedFrameScaled(em, 26.0f, 0U) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x16, 3, 0.9f);
        }
        break;
    case 0x3FC:
        break;
    case 0x400:
        if (Player_PassedFrameScaled(em, 14.0f, 0U) != 0) {
            if (em->type == 3 || em->type == 0x45) {
                Eff02_Create(em, 0x14);
            } else {
                Eff02_Create(em, 0xC);
            }
        }
        break;
    case 0x40E:
    case 0x40F:
    case 0x424:
    case 0x425:
    case 0x426:
        break;
    case 0x427:
        if (Player_PassedFrameScaled(em, 44.0f, 0U) != 0) {
            Effect0D_SpawnForEmScaled(em, 2, 6, 0.5f);
        }
        break;
    case 0x428:
    case 0x429:
        break;
    case 0x42B:
        pos[0] = 0.0f;
        pos[1] = 0.0f;
        pos[2] = 10.0f;
        Em_SpawnEffect06Periodic(em, 0x17, pos, 1.0f);
        break;
    case 0x42C:
    case 0x42D:
        break;
    default:
        EmAnteka_MotionEffectsDefault(em);
        break;
    }
}

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

void EmMosswine_Init(Em *em)
{
    EmMosswineWork *w = (EmMosswineWork *)&em->work;
    f32 fslot;
    s16 size;
    s16 hp;
    s16 hp2;
    u8 idx;
    u8 slotNo;

    if (gQuest.stage == 0) {
        if (gScene.area == 0xF) {
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
    Em_SetMotionRow(em, 1, 0, 0);
    em->stance = 0;
    if (em->type == 4) {
        size = Em_ComputeSize(em, 0);
        em->hp = size;
        em->hpMax = size;
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
        idx = em->variant;
        if ((s32) idx < 4) {
            hp = Em_ComputeSize(em, (s16) idx);
            em->hp = hp;
            em->hpMax = hp;
        } else {
            hp2 = Em_ComputeSize(em, 0);
            em->hp = hp2;
            em->hpMax = hp2;
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

void EmMosswine_Start(Em *em)
{
    EmMosswine_Init(em);
    EmMosswine_ChangeState(em, 0, 1, 0);
}

void EmMosswine_Reinit(Em *em)
{
    EmMosswine_Init(em);
}

void EmMosswine_ChangeState(Em *em, u16 group, u16 state, u8 mode)
{
    f32 *p;

    em->motionSpeed = 1.0f;
    switch (group) {
    case 0:
        switch (state) {
        case 1:
            if (em->combat == 1) {
                if (em->type == 4) {
                    state = 12;
                } else {
                    state = 6;
                }
            }
            break;
        case 13:
            Em_EndStatus5(em);
            em->noReact = 1;
            break;
        }
        break;
    case 1:
        switch (state) {
        case 1:
        case 2:
        case 3:
            em->timer = 1800;
            Em_ComputeGoal2(em, em->goal);
            break;
        case 4:
            em->timer = 600;
            p = EmMosswine_VariantParams[em->variant];
            em->posX = *p++;
            em->posY = *p++;
            em->posZ = *p++;
            em->goal[0] = *p++;
            em->goal[1] = *p++;
            em->goal[2] = *p++;
            break;
        }
        break;
    case 3:
        Em_ComputeGoal2(em, em->goal);
        switch (state) {
        case 0:
            em->timer = 90;
            em->motionSpeed = 0.8f;
            break;
        case 1:
            em->timer = 90;
            em->motionSpeed = 0.8f;
            break;
        case 2:
            em->timer = 90;
            em->motionSpeed = 1.0f;
            break;
        case 3:
            em->timer = 120;
            em->motionSpeed = 1.0f;
            break;
        case 4:
            em->timer = 30;
            em->motionSpeed = 0.8f;
            break;
        case 5:
            em->timer = 45;
            em->motionSpeed = 0.8f;
            break;
        }
        break;
    case 4:
    case 6:
    case 8:
        switch (state) {
        case 5:
            if (em->type == 5) {
                Em_BeginStatus5(em);
            }
            break;
        }
        break;
    case 5:
        if (state == 2 && QuestEm_CanAppear(em) == 1) {
            QuestEm_Appear(em);
        }
        break;
    case 2:
    case 7:
        break;
    }
    Em_SetStateSync(em, group, state, mode);
}

void EmMosswine_EndState(Em *em)
{
    em->stance = 0;
    em->scriptPending = 1;
    EmMosswine_ChangeState(em, 0, 1, 0);
}

void EmMosswine_State0_00(void)
{
}

void EmMosswine_State0_01(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 1, 0xA, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmMosswine_EndState(em);
        }
        break;
    }
}

void EmMosswine_State0_02(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->timer = ((s32) em->stateRand % 60) + 0x1E;
        Em_SetMotion(em, 0xA, 6, 0);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0xD, 6, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmMosswine_EndState(em);
        }
        break;
    }
}

void EmMosswine_State0_03(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 7, 6, 0);
        Motion_InitTime(em, 0);
        break;
    case 1:
        if (em->motionTime >= 0xF0) {
            EmMosswine_EndState(em);
        }
        break;
    }
}

void EmMosswine_State0_04(Em *em, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->timer = variant;
        Em_SetMotion(em, 0xE, 6, 0);
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

void EmMosswine_State0_05(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0xC, 6, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmMosswine_EndState(em);
        }
        break;
    }
}

void EmMosswine_State0_06(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x11, 6, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmMosswine_EndState(em);
        }
        break;
    }
}

void EmMosswine_State0_0A(Em *em)
{
    Em_SetHitReactMode(em, 2U);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x46, 0, 0);
        Em_BeginStatus2(em);
        break;
    case 1:
        Em_SpawnEffect06Periodic(em, 0xB, (f32 *) &EmMosswine_Effect0BOfs, 1.0f);
        if (--em->timer <= 0) {
            EmMosswine_ChangeState(em, 0, 0xB, 4);
        }
        break;
    }
}

void EmMosswine_State0_0B(Em *em)
{
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_EndStatus2(em);
        Em_SetMotion(em, 0x49, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->blind = 0;
            EmMosswine_ChangeState(em, 0, 9, 4);
        }
        break;
    }
}

void EmMosswine_State0_09(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0xB, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmMosswine_EndState(em);
        }
        break;
    }
}

void EmMosswine_State0_0C(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x13, 6, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmMosswine_EndState(em);
        }
        break;
    }
}

void EmMosswine_State0_0D(Em *em)
{
    em->guardTimer = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x48, 0, 0);
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

void EmMosswine_RunGroup0(Em *em)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->state) {
    case 0:
        EmMosswine_State0_00();
        break;
    case 1:
        EmMosswine_State0_01(em);
        break;
    case 2:
        EmMosswine_State0_02(em);
        break;
    case 3:
        EmMosswine_State0_03(em);
        break;
    case 4:
        EmMosswine_State0_04(em, 1);
        break;
    case 5:
        EmMosswine_State0_05(em);
        break;
    case 6:
        EmMosswine_State0_06(em);
        break;
    case 7:
        EmMosswine_State0_04(em, 2);
        break;
    case 8:
        EmMosswine_State0_04(em, 4);
        break;
    case 9:
        EmMosswine_State0_09(em);
        break;
    case 12:
        EmMosswine_State0_0C(em);
        break;
    case 10:
        EmMosswine_State0_0A(em);
        break;
    case 11:
        EmMosswine_State0_0B(em);
        break;
    case 13:
        EmMosswine_State0_0D(em);
        
    default:
        break;
    }
}

void EmMosswine_State1_00(Em *em, s32 variant)
{
    u16 d;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        if (variant == 0) {
            Em_StartTurnToTarget(em, &EmMosswine_TurnParams[0]);
        } else {
            em->aimAngle = em->rotY + 0x4000;
            d = em->aimAngle - em->rotY;
            if (d <= 0x1000U || d >= 0xF000U) {
                Player_SetFlag((Player *) em, 0x20000);
                Em_SetMotion(em, 2, 0, 0);
            } else if (d >= 0x8000U) {
                Em_SetMotion(em, 5, 0, 0);
            } else {
                Em_SetMotion(em, 4, 0, 0);
            }
        }
        break;
    case 1:
        if (Em_TurnStep(em, &EmMosswine_TurnParams[0]) == 1) {
            Player_ClearFlag(em, 0x20000);
            EmMosswine_EndState(em);
        }
        break;
    }
}

void EmMosswine_State1_01(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 2, 6, 0);
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

void EmMosswine_State1_02(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 3, 6, 0);
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

void EmMosswine_State1_03(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 8, 6, 0);
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

void EmMosswine_State1_04(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 2, 0, 0);
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

void EmMosswine_State1_05(Em *em)
{
    EmMosswineWork *w = (EmMosswineWork *)&em->work;
    s32 timer;

    switch (em->step) {
    case 0:
        em->step++;
        em->timer = (em->stateRand & 0x7F) + 0x78;
        Em_SetMotion(em, 2, 6, 0);
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

void EmMosswine_RunGroup1(Em *em)
{
    switch (em->state) {
    case 0:
        EmMosswine_State1_00(em, 0);
        break;
    case 1:
        EmMosswine_State1_01(em);
        break;
    case 2:
        EmMosswine_State1_02(em);
        break;
    case 3:
        EmMosswine_State1_03(em);
        break;
    case 4:
        EmMosswine_State1_04(em);
        break;
    case 5:
        EmMosswine_State1_05(em);
        break;
    case 6:
        EmMosswine_State1_00(em, 1);
        
    default:
        break;
    }
}

void EmMosswine_State3_00_Charge(Em *em, u32 variant)
{
    f32 v[4];
    f32 out[4];
    s32 ang[4];
    f32 m[16];

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0xF, 0xA, 0);
        switch (variant) {
        case 0:
            Eff02_Create(em, 3);
            break;
        case 1:
            Eff02_Create(em, 8);
            break;
        case 2:
            Eff02_Create(em, 9);
            break;
        case 3:
            Eff02_Create(em, 0xA);
            break;
        case 4:
            Eff02_Create(em, 0xE);
            break;
        case 5:
            Eff02_Create(em, 0xF);
            break;
        }
        ang[0] = 0;
        ang[1] = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        ang[2] = 0;
        Mtx_FromAnglesXYZ((s32 *) ang, m);
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = 100.0f;
        Vec_Rotate(out, v, m);
        em->goal[0] += out[0];
        em->goal[2] += out[2];
        em->targetAngle = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        break;
    case 1:
        if (Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) < 100.0f || --em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x10, 0xA, 0);
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

void EmMosswine_RunGroup3(Em *em)
{
    switch (em->state) {
    case 0:
        EmMosswine_State3_00_Charge(em, 0U);
        break;
    case 1:
        EmMosswine_State3_00_Charge(em, 1U);
        break;
    case 2:
        EmMosswine_State3_00_Charge(em, 2U);
        break;
    case 3:
        EmMosswine_State3_00_Charge(em, 3U);
        break;
    case 4:
        EmMosswine_State3_00_Charge(em, 4U);
        break;
    case 5:
        EmMosswine_State3_00_Charge(em, 5U);
        
    default:
        break;
    }
}

void EmMosswine_State4_00(Em *em)
{
    s32 diff;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        diff = (em->hitAngle - em->rotY) & 0xFFFF;
        if ((diff > 24576) && (diff < 0xA000)) {
            Em_SetMotion(em, 0x14, 0, 0);
        } else if (diff < 0x8000) {
            Em_SetMotion(em, 0x3C, 0, 0);
        } else {
            Em_SetMotion(em, 0x3D, 0, 0);
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

void EmMosswine_State4_01(Em *em)
{
    f32 v[4];
    f32 out[4];
    s32 ang[4];
    f32 m[16];

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        ang[0] = (u16) (em->hitAngle - em->rotY);
        if (ang[0] < 0x8000) {
            Em_SetMotion(em, 0x3E, 0, 0);
        } else {
            Em_SetMotion(em, 0x43, 0, 0);
        }
        Em_RestartScript(em);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 10.0f, 0) != 0) {
            em->step++;
            ang[0] = 0;
            ang[1] = em->hitAngle + 0x8000;
            ang[2] = 0;
            Mtx_FromAnglesXYZ((s32 *) ang, m);
            v[0] = 0.0f;
            v[1] = 8.0f;
            v[2] = -21.0f;
            Vec_Rotate(out, v, m);
            Player_ClearAccel2(em);
            em->velX = out[0];
            em->velY = out[1];
            em->velZ = out[2];
            em->accelY = -1.09f;
            em->accelZ = 0.28f;
            em->stance = 2;
        }
        break;
    case 2:
        if (!((em->velZ * em->accelZ) < 0.0f)) {
            em->accelZ = 0.0f;
        }
        if (Player_MoveFall((Player *) em)) {
            em->step++;
            em->stance = 0;
            Em_SetMotion(em, 0x3F, 6, 0);
            em->timer = 150;
        }
        break;
    case 3:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x42, 6, 0);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            EmMosswine_EndState(em);
        }
        break;
    }
}

void EmMosswine_State4_02(Em *em)
{
    f32 v[4];
    f32 out[4];
    s32 ang[4];
    f32 m[16];

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        ang[0] = (u16) (em->hitAngle - em->rotY);
        if (ang[0] < 0x8000) {
            Em_SetMotion(em, 0x3E, 0, 0xA);
        } else {
            Em_SetMotion(em, 0x43, 0, 0xA);
        }
        ang[0] = 0;
        ang[1] = em->hitAngle + 0x8000;
        ang[2] = 0;
        Mtx_FromAnglesXYZ((s32 *) ang, m);
        v[0] = 0.0f;
        v[1] = 8.0f;
        v[2] = -21.0f;
        Vec_Rotate(out, v, m);
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
            em->stance = 0;
            Em_SetMotion(em, 0x3F, 6, 0);
            em->timer = 150;
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x42, 6, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            EmMosswine_EndState(em);
        }
        break;
    }
}

void EmMosswine_State4_03(Em *em)
{
    s32 timer;

    Em_SetHitReactMode(em, 4U);
    timer = em->timer;
    if (timer > 0) {
        em->timer = timer - 1;
    }
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x47, 0, 0);
        em->rotY = em->hitAngle + 0x7FFF + 1;
        Em_BeginStatus4(em);
        em->noReact = 1;
        Em_RestartScript(em);
        break;
    case 1:
        Em_SpawnEffect06Bone(em, 2);
        if (em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x48, 0, 0);
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

void EmMosswine_State4_04(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x47, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x48, 0, 0);
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

void EmMosswine_State4_05(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x47, 0, 0);
        Em_RestartScript(em);
        em->noReact = 1;
        StageObj_Stun_CreateEm(em, 1, 0, *(s16 *)((u8 *)em + 8), 100.0f, 1.2f);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmMosswine_ChangeState(em, 0, 0xD, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmMosswine_ChangeState(em, 0, 0xD, 4);
        }
        break;
    }
}

void EmMosswine_RunGroup4(Em *em)
{
    switch (em->state) {
    case 0:
        EmMosswine_State4_00(em);
        break;
    case 1:
        EmMosswine_State4_01(em);
        break;
    case 2:
        EmMosswine_State4_02(em);
        break;
    case 3:
        EmMosswine_State4_03(em);
        break;
    case 4:
        EmMosswine_State4_04(em);
        break;
    case 5:
        EmMosswine_State4_05(em);
        
    default:
        break;
    }
}

void EmMosswine_State5_00_Hunted(Em *em)
{
    f32 v[4];
    f32 out[4];
    s32 ang[4];
    f32 m[16];

    em->noHitTimer = 0xA;
    em->noPushTimer = 0xA;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        ang[0] = (u16) (em->hitAngle - em->rotY);
        if (ang[0] < 0x8000) {
            Em_SetMotion(em, 0x3E, 0, 0);
        } else {
            Em_SetMotion(em, 0x43, 0, 0);
        }
        QuestEm_Hunted(em);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 10.0f, 0) != 0) {
            em->step++;
            ang[0] = 0;
            ang[1] = em->hitAngle + 0x8000;
            ang[2] = 0;
            Mtx_FromAnglesXYZ((s32 *) ang, m);
            v[0] = 0.0f;
            v[1] = 0.0f;
            v[2] = -19.0f;
            Vec_Rotate(out, v, m);
            Player_ClearAccel2(em);
            em->velY = 20.0f;
            em->velX = out[0];
            em->velZ = out[2];
            v[2] = 0.4f;
            Vec_Rotate(out, v, m);
            em->accelY = -2.08f;
            em->accelX = out[0];
            em->accelZ = out[2];
            em->stance = 2;
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
            em->stance = 0;
            Em_SetMotion(em, 0x3F, 6, 0);
            em->timer = 150;
            Player_ClearVelocity(em);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x40, 0, 0);
            em->timer = 60;
            EmMine_Create(em, 0);
            em->fieldCollide = 0;
        }
        break;
    case 4:
        EmMine_Update(em);
        if (em->motion[0].playing == 0) {
            em->step++;
            if (em->type == 4) {
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
            EmMosswine_ChangeState(em, 5, 2, 4);
        }
        break;
    }
}

void EmMosswine_State5_01_Hunted(Em *em)
{
    f32 v[4];
    f32 out[4];
    s32 ang[4];
    f32 m[16];

    em->noHitTimer = 0xA;
    em->noPushTimer = 0xA;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        ang[0] = (u16) (em->hitAngle - em->rotY);
        if (ang[0] < 0x8000) {
            Em_SetMotion(em, 0x3E, 0, 0xA);
        } else {
            Em_SetMotion(em, 0x43, 0, 0xA);
        }
        QuestEm_Hunted(em);
        ang[0] = 0;
        ang[1] = em->hitAngle + 0x8000;
        ang[2] = 0;
        Mtx_FromAnglesXYZ((s32 *) ang, m);
        v[0] = 0.0f;
        v[1] = 8.0f;
        v[2] = -21.0f;
        Vec_Rotate(out, v, m);
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
            em->stance = 0;
            Em_SetMotion(em, 0x3F, 6, 0);
            em->timer = 150;
            Player_ClearVelocity(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x40, 0, 0);
            em->timer = 60;
            EmMine_Create(em, 0);
            em->fieldCollide = 0;
        }
        break;
    case 3:
        EmMine_Update(em);
        if (em->motion[0].playing == 0) {
            em->step++;
            if (em->type == 4) {
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
            EmMosswine_ChangeState(em, 5, 2, 4);
        }
        break;
    }
}

void EmMosswine_State5_02(Em *em)
{
    switch (em->step) {
    case 0:
        if (QuestEm_CanAppear(em) == 1) {
            EmMosswine_ChangeState(em, 0, 1, 0);
            break;
        }
        em->taskPhase++;
    case 1:
        break;
    }
}

void EmMosswine_RunGroup5(Em *em)
{
    switch (em->state) {
    case 0:
        EmMosswine_State5_00_Hunted(em);
        break;
    case 1:
        EmMosswine_State5_01_Hunted(em);
        break;
    case 2:
        EmMosswine_State5_02(em);
        break;
    }
}

void EmMosswine_State6_00(Em *em)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 0;
        Em_SetMotion(em, 1, 0, 0);
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

void EmMosswine_State6_01(Em *em)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 0;
        Em_SetMotion(em, 1, 0, 0);
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

void EmMosswine_RunGroup6(Em *em)
{
    switch (em->state) {
    case 0:
        EmMosswine_State6_00(em);
        break;
    case 1:
        EmMosswine_State6_01(em);
        break;
    }
}

void EmMosswine_Update(Em *em)
{
    EmMosswineWork *w = (EmMosswineWork *)&em->work;
    u8 buf[3];

    if (w->unk00C != 0) {
        (w->unk00C)--;
    }
    switch (Em_UpdateDamage(em, buf) & 0xFF) {
    case 0:
    case 3:
    case 4:
    case 9:
    case 11:
    case 14:
    case 15:
    case 16:
    case 18:
        break;
    case 1:
    case 2:
        if (em->stance == 2) {
            EmMosswine_ChangeState(em, 5, 0, 3);
        } else {
            EmMosswine_ChangeState(em, 5, 1, 3);
        }
        break;
    case 5:
        if ((em->stateGroup != 4 || em->state != 1) && (em->stateGroup != 4 || em->state != 2)) {
            EmMosswine_ChangeState(em, 4, 2, 3);
        }
        break;
    case 6:
        if ((em->stateGroup != 4 || em->state != 1) && (em->stateGroup != 4 || em->state != 2)) {
            Em_SetStatus4Timer(em);
            EmMosswine_ChangeState(em, 4, 3, 3);
        }
        break;
    case 7:
        EmMosswine_ChangeState(em, 4, 2, 3);
        break;
    case 8:
        if ((em->stateGroup != 4 || em->state != 1) && (em->stateGroup != 4 || em->state != 2)) {
            Em_SetStatus2Timer(em);
            EmMosswine_ChangeState(em, 0, 0xA, 3);
        }
        break;
    case 10:
        em->blind = 0;
        Em_EndStatus2(em);
        if (em->stance == 2) {
            EmMosswine_ChangeState(em, 4, 2, 3);
        } else {
            EmMosswine_ChangeState(em, 4, 1, 3);
        }
        break;
    case 12:
        if (em->stance == 2) {
            EmMosswine_ChangeState(em, 4, 2, 3);
        } else if ((s16) Player_IsState(em, 4, 0)) {
            EmMosswine_ChangeState(em, 4, 1, 3);
        } else {
            EmMosswine_ChangeState(em, 4, 0, 3);
        }
        break;
    case 13:
        if (em->stance == 2) {
            EmMosswine_ChangeState(em, 4, 2, 3);
        } else {
            EmMosswine_ChangeState(em, 4, 1, 3);
        }
        Em_ResetPartHp(em, 0);
        break;
    case 17:
        if (em->type == 5) {
            if ((em->stateGroup != 4 || em->state != 5) && (em->stateGroup != 4 || em->state != 1) &&
                (em->stateGroup != 4 || em->state != 2)) {
                Em_SetStatus5Timer(em);
                EmMosswine_ChangeState(em, 4, 5, 3);
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

void EmMosswine_RunState(Em *em)
{
    switch (em->stateGroup) {
    case 0:
        EmMosswine_RunGroup0(em);
        break;
    case 1:
        EmMosswine_RunGroup1(em);
        break;
    case 2:
        EmMosswine_RunGroup0(em);
        break;
    case 3:
        EmMosswine_RunGroup3(em);
        break;
    case 4:
        EmMosswine_RunGroup4(em);
        break;
    case 5:
        EmMosswine_RunGroup5(em);
        break;
    case 6:
        EmMosswine_RunGroup6(em);
        break;
    case 7:
        EmMosswine_RunGroup4(em);
        
    default:
        break;
    }
}

void EmMosswine_SpawnEffect01(u8 *em)
{
    Effect01_Spawn((Player *) em, 0);
}

void EmMosswine_TableNop(void)
{
}

s32 EmMosswine_ScriptQuery(Em *em, u8 flag)
{
    Em *t;

    switch (flag) {
    case 0:
        t = em->link;
        if (t != NULL && em->area == t->area && (s16) Player_IsState(t, 0, 6)) {
            return 1;
        }
        return 0;
    }
    return 0;
}

void EmLaoshan_MotionSounds(Em *em, EmWork *work)
{
    s16 motion;

    motion = ((EmLaoshanWork *)work)->fxMotion;
    switch (motion) {
    case 0x3E9:
        Em_PlaySe6AtBoneInFrames(em, 0x1CC, 0x24C, 0x46, 0x25, 6, 0);
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
        Em_PlaySe6AtBoneInFrames(em, 0x60, 0x1C2, 0x47, 0x25, 6, 0);
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
        Em_PlaySe6AtBoneInFrames(em, 0x9C, 0xC6, 0x22, 0x25, 8, 0);
        Em_PlaySeAtBoneVolOnFrame(em, 0x42, 0xB, 0x17, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x34, 0x6B, 0x17, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x34, 3, 0x17, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0xA, 0xC, 0xB, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x98, 0x6B, 0xE, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x98, 3, 0xE, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x8C, 0xC, 0x13, 8);
        break;
    case 0x3EF:
        Em_PlaySe6AtBoneInFrames(em, 2, 0x1E0, 0x48, 0x25, 9, 0);
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
        Em_PlaySe6AtBoneInFrames(em, 0x108, 0x294, 0x48, 0x25, 9, 0);
        Em_PlaySe6AtBoneInFrames(em, 0x108, 0x230, 0x1F, 3, 8, 0);
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
        Em_PlaySe6AtBoneInFrames(em, 0x140, 0x1E0, 0x26, 0x25, 8, 0);
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
        Em_PlaySe6AtBoneInFrames(em, 6, 0x168, 0x47, 0x25, 6, 0);
        Em_PlaySeAtBoneVolOnFrame(em, 0x18, 0x13, 3, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x88, 0x14, 0x32, 8);
        break;
    case 0x3F5:
        Em_PlaySeAtBoneVolOnFrame(em, 6, 0x22, 0x25, 8);
        Em_PlaySe6AtBoneInFrames(em, 0x8C, 0x12C, 0x23, 0x25, 9, 0);
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
        Em_PlaySe6AtBoneInFrames(em, 0xF6, 0x190, 0x4E, 0x25, 8, 0);
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
        Em_PlaySe6AtBoneInFrames(em, 0x120, 0x18E, 0x4E, 0x25, 6, 0);
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
        Em_PlaySe6AtBoneInFrames(em, 0xE4, 0x19A, 0x46, 0x25, 8, 0);
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
        Em_PlaySe6AtBoneInFrames(em, 4, 0xD2, 0x46, 0x25, 8, 0);
        Em_PlaySe6AtBoneInFrames(em, 0x104, 0x19A, 0x4B, 0x25, 9, 0);
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
        Em_PlaySe6AtBoneInFrames(em, 4, 0xD2, 0x4C, 0x25, 9, 0);
        Em_PlaySeAtBoneVolOnFrame(em, 0xAC, 0x17, 0x23, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x38, 0x17, 0x32, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x38, 0x16, 0x30, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0xE, 3, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0x13, 3, 8);
        break;
    case 0x427:
        Em_PlaySe6AtBoneInFrames(em, 4, 0x11C, 0x4C, 0x25, 9, 0);
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
        Em_PlaySe6AtBoneInFrames(em, 0x8A, 0xBC, 0x4C, 0x25, 9, 0);
        Em_PlaySeAtBoneVolOnFrame(em, 0x34, 8, 0xE, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x34, 0x69, 0xE, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xFC, 1, 0xE, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x38, 8, 8, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x38, 0x69, 8, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x38, 1, 8, 6);
        break;
    case 0x429:
        Em_PlaySe6AtBoneInFrames(em, 4, 0x18C, 0x49, 0x25, 9, 0);
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
        Em_PlaySe6AtBoneInFrames(em, 6, 0x274, 0x4D, 0x25, 9, 0);
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
        Em_PlaySe6AtBoneInFrames(em, 4, 0xD2, 0x4C, 0x25, 9, 0);
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
EmKhezuVec EmKhezu_CarveThrowPt = {
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
EmVelocipreyAppearPoint EmVelociprey_AppearPts1F[4] = {
#include "game/008C8470.inc"
};
EmVelocipreyAppearAt EmVelociprey_AppearPts22[3] = {
#include "game/008C84D0.inc"
};
EmVelocipreyAppearAt EmVelociprey_AppearPts24[3] = {
#include "game/008C8500.inc"
};
EmVelocipreyAppearPoint EmVelociprey_AppearPts26[5] = {
#include "game/008C8530.inc"
};
EmVelocipreyAppearPoint EmVelociprey_AppearPts27[4] = {
#include "game/008C85B0.inc"
};
EmVelocipreyAppearPoint EmVelociprey_AppearDay5E = {
#include "game/008C8610.inc"
};
EmVelocipreyAppearAt EmVelociprey_AppearPts70[4] = {
#include "game/008C8630.inc"
};
EmVelocipreyAppearAt EmVelociprey_AppearPts71[3] = {
#include "game/008C8670.inc"
};
EmVelocipreyAppearAt EmVelociprey_AppearPts72[4] = {
#include "game/008C86A0.inc"
};
EmVelocipreyAppearAt EmVelociprey_AppearPts74[3] = {
#include "game/008C86E0.inc"
};
EmVelocipreyAppearAt EmVelociprey_AppearPts77[2] = {
#include "game/008C8710.inc"
};
EmVelocipreyAppearAt EmVelociprey_AppearPts80[2] = {
#include "game/008C8730.inc"
};
EmVelocipreyAppearAt EmVelociprey_AppearPts81[2] = {
#include "game/008C8750.inc"
};
EmVelocipreyAppearAt EmVelociprey_AppearPts8E[3] = {
#include "game/008C8770.inc"
};
EmVelocipreyAppearAt EmVelociprey_AppearPts90[2] = {
#include "game/008C87A0.inc"
};
EmVelocipreyAppearAt EmVelociprey_AppearPts93[1] = {
#include "game/008C87C0.inc"
};
EmVelocipreyAppearAt EmVelociprey_AppearPts9C[2] = {
#include "game/008C87D0.inc"
};
EmVelocipreyAppearAt EmVelociprey_AppearPts9F[1] = {
#include "game/008C87F0.inc"
};
EmVelocipreyAppearAt EmVelociprey_AppearPtsA0[3] = {
#include "game/008C8800.inc"
};
EmVelocipreyAppearAt EmVelociprey_AppearPtsA4[2] = {
#include "game/008C8830.inc"
};
EmVelocipreyAppearAt EmVelociprey_AppearPtsA7[1] = {
#include "game/008C8850.inc"
};
EmVelocipreyAppearAt EmVelociprey_AppearPtsB0[4] = {
#include "game/008C8860.inc"
};
EmVelocipreyAppearAt EmVelociprey_AppearPtsB1[2] = {
#include "game/008C88A0.inc"
};
EmVelocipreyAppearAt EmVelociprey_AppearPtsB2[2] = {
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
s32 func_0090A9C0(void); /* gm_sub */
void GmEm_UpdateSfx(Em *em); /* gm_sub */
EmCurveKey *EmVelociprey_SpeedCurves[4] = {
    EmVelociprey_SpeedCurves_0, NULL, EmVelociprey_SpeedCurves_2, NULL,
};
void *EmAnteka_Callbacks[8] = {
    EmAnteka_SpawnEffect01, EmAnteka_SpawnEffect01, EmAnteka_TableNop, EmAnteka_UpdateMotionEffects,
    func_0090A9C0, EmAnteka_Reinit, NULL, NULL,
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
