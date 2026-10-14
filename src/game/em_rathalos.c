/*
 * Monster family: Rathian, Rathalos and their subspecies (kinds 1, 0xB, 0x25, 0x29, 0x2A, 0x31).
 * The code from the family's setup function (0x7E7240, called by the per-kind
 * dispatch Em_TypeSetup) to the next family's. The state functions take
 * the monster and the work area its state dispatcher passes on (EmWork);
 * Em.step is the step within the current state. See include/em.h.
 */
#include "common.h"
#include "scene.h"
#include "quest.h"
#include "mine_point.h"
#include "em.h"
#include "player.h"
#include "stageobj.h"
/* prototypes of callees (round 13) */
void Em_SetPartScaled(Em *em, s32 bit);
void StageObj_K09_Create(Em *);
void Em_AreaExitPoint(Em *em, f32 *out);
s32 Area_ContainsPos(s32, f32 *);
void Em_EnterNextArea(Em *);
struct EmIdEntry;
f32 *Em_AreaEntryValue(Em *em, struct EmIdEntry *entry);
u16 Rand_Next(s32 stream);

/* A mining point (as in sub_main's mine_point.c). */
/* prototypes */
void EmFatalis_ChangeState(Em *em, u16 group, u16 state, u8 mode);
s8 EmMine_CreateTimed0(Em *em, f32 *pos);
void EmRathalos_ChangeState(Em *em, u16 group, u16 state, u8 mode);
/* unit's own declaration: with the definition's void (Em *em, s16 n) (game/unk_708850.c), EmRathalos_State0_0E: 50 of 241 words differ, EmRathalos_State0_1A: 42 of 86 words differ */
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
void Eff01_Create(Em *, s8);
void Eff04_Create(Em *, s8);
void EffEm08_CreateAtBone(Em *em, s32 a, s32 b, s32 c, s32 d, s32 e);
void *EffEm33_Create(Em *em, s32 kind, s32 c, s32 a, s32 b);
void Em_SetRow638(Em *em, u8 idx);
void Em_MoveRotated(Em *em, void *src);
void Em_MoveRotatedAccel(Em *em, void *ang);
void Em_SetStateRand(Em *em, s32 arg1, s32 arg2);
void Em_LoadReactionSet(Em *em, u8 k);
void Em_ComputeGoal2(Em *em, f32 *target);
void Em_GetPitchToGoal(Em *em, s32 *out, f32 a, f32 b);
s32 Em_GetTargetPos(Em *em, u8 idx, f32 *out);
void Em_Clear7F0State(Em *em);
void Em_Clear7ECState(Em *em);
void Em_SpawnEffect06Bone(Em *em, s32 a);
void Em_Add7F0(Em *em, s32 n);
void Em_Add7EC(Em *em, s32 n);
void Em_SetFlag6AE(Em *em, u8 a);
void Em_SetGoal12(Em *em);
void Em_ClearPoison(Em *em);
void Em_SetHold2(Em *em);
void Em_SetCaptured(Em *em, s32 a);
void Em_PlaySe1AtBoneOnFrame(Em *em, s32 frame, s32 se, s32 bone);
void Em_PlaySeAtBoneVolOnFrame(Em *em, s32 frame, s32 se, s32 bone, s32 vol);
void Em_PlaySeAtBoneVolOnFrame2(Em *em, s32 frame, s32 se, s32 bone, s32 flag, s32 vol);
void Em_UpdateFlight(Em *em, s32 flags, EmFlyParam *p);
s32 Em_TurnToTargetMotion(Em *em, u16 range, u16 motFront, u16 motLeft, u16 motRight);
s32 Em_StepToGoal(Em *em, u16 step, u8 flags);
u8 Em_TurnToGoal(Em *em, u16 step, u8 flags);
void Em_StartTurnToTarget(Em *em, EmTurnParams *t);
s32 Em_TurnStep(Em *em, EmTurnParams *t);
void Em_RestartScript(Em *em);
void EmRathalos_Init(Em *);
void EmRathalos_State1_07(Em *, EmWork *);
void EmRathalos_State2_00(Em *, EmWork *);
void EmRathalos_State2_01_FlyInLand(Em *, EmWork *);
void EmRathalos_State2_03(Em *, EmWork *);
void EmRathalos_State2_08(Em *em, EmWork *work);
void EmRathalos_State2_09(Em *, EmWork *);
void EmRathalos_State2_0A(Em *em, EmWork *work);
void EmRathalos_State2_0D(Em *, EmWork *);
void EmRathalos_StepForward(Em *, s32);
void EmRathalos_State6_02_QuestIntro(Em *, EmWork *);
void EmRathalos_State6_04_Captured(Em *em);
void EmRathalos_State6_06(Em *);
void EmRathalos_State8_00_CarveStart(Em *em, EmWork *work);
void EmRathalos_State8_03(Em *, EmWork *);
void EmRathalos_State5_00_Hunted(Em *em, EmWork *work);
void EmRathalos_State5_01_Hunted(Em *em, EmWork *work);
void EmRathalos_State5_02_HuntedFalling(Em *em, EmWork *work);
void EmRathalos_RunGroup0(Em *em, EmWork *work);
void EmRathalos_RunGroup3(Em *em, EmWork *work);
typedef struct EmRathalosFxWork {
    u8 pad[2];
    s16 motion;         /* 0x02: the last motion seen by the effects */
} EmRathalosFxWork;
void Effect0D_SpawnForEmScaled(Em *em, s32 a, s32 b, f32 scale);
void EmRathalos_MotionSounds(Em *em, EmWork *work);
void StageObj_K0F_CreateOnEm(Em *em, u8 k, u8 c, f32 scale);
void EmRathalos_MotionEffects(Em *em, EmRathalosFxWork *work);
void EmRathalos_SpawnK14Type0B(Em *em);
void EmRathalos_SpawnK14Type0AEvery5(Em *em);
void EmRathalos_MoveByVelYCurve(Em *em, s32 kind);
void EmRathalos_StartCurves(Em *em, u8 kind);
u8 EmRathalos_UpdateCurves(Em *em);
void EmFatalis_EndState2(Em *em, u8 a);
void EmFatalis_State0_04(Em *, EmWork *);
void EmFatalis_State1_02(Em *, EmWork *);
void EmFatalis_State2_02(Em *, EmWork *);
void EmFatalis_State3_04(Em *, s32, s32);
void EmFatalis_State3_0D(Em *, EmWork *);
void EmFatalis_State3_12(Em *, s32, s32);
void EmFatalis_State3_14(Em *, EmWork *);
void EmFatalis_State3_15(Em *);
void EmFatalis_State4_03(Em *, EmWork *);
void EmFatalis_State4_04(Em *, EmWork *);
void EmFatalis_State4_08(Em *, EmWork *);
void EmFatalis_State5_00_Hunted(Em *em, EmWork *work);
void EmFatalis_State5_01_Hunted(Em *, EmWork *);
void EmFatalis_State5_02_HuntedFalling(Em *, EmWork *);
void EmFatalis_State6_00(Em *em, EmWork *work);
void EmFatalis_State6_01(Em *, EmWork *);
void EmFatalis_State6_03(Em *, EmWork *);
void EmFatalis_State6_04(Em *, EmWork *);
typedef struct EmFatalisFxWork {
    u8 pad[2];
    s16 motion;         /* 0x02: the last motion seen by the effects */
} EmFatalisFxWork;
void EmFatalis_RequestOnFrame(void *arg0, s32 arg1, s32 arg2);
void StageObj_K1C_CreateOnEmC(Em *em, u8 k, u8 c, f32 scale);
void EmFatalis_MotionEffects(Em *em, EmFatalisFxWork *work);
void EmFatalis_MoveByVelYCurve(Em *, s32);
void EmFatalis_StartCurves(Em *em, u8 arg1);
/* unit's own declaration: with the definition's u8 (Em *em) (game/em_bulldrome.c), EmFatalis_State2_00: 19 of 71 words differ, EmFatalis_State2_0B: 46 of 91 words differ ... */
s32 EmFatalis_UpdateCurves(Em *);
void StageObj_K11_CreateAtBone(Em *em, s16 bone, s16 k, u16 c);
void StageObj_K13_Create(void *owner, s16 v, u8 k);
void StageObj_K14_Create(Em *em, s16 k, s16 c, f32 scale);
/* unit's own declaration: with the definition's void (Em *em, u8 k, s16 v, s16 t, f32 y, f32 scale) (game/unk_874B90.c), EmRathalos_State4_14: 66 of 98 words differ, EmRathalos_State4_15: 43 of 74 words differ */
void StageObj_Stun_CreateEm(Em *, s32, s32, s32, f32, f32);
/* unit's own declaration: with the definition's void (Em *em, u8 k, u16 w, s16 v, f32 scale) (game/unk_880D50.c), EmFatalis_SpawnK1COrK11: 29 of 32 words differ */
void StageObj_K1C_CreateOnEmB(Em *em, s32, s32, s32, f32);
s32 Sound_RandomSe(s32 id, s16 lo, s32 alt, s16 n);
s32 Player_PassedFrameScaled(void *p, f32 frame, s32 layer);
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
void Effect01_Spawn(void *obj, s16 type);
extern f32 gAreaGroundY[];
extern EmFlyParam EmRathalos_FlyParam;
extern EmFlyParam EmRathalos_FlyParam2;
extern EmTurnParams EmRathalos_TurnParams[2];
extern EmTurnParams EmFatalis_TurnParams[1];
void QuestEm_Hunted(Em *em);
void EmFatalis_AddMinePoints(Em *);
void EmFatalis_UpdateMinePoints(Em *);
void EmFatalis_RemoveMinePoints(Em *em);
void CamReq_BossHunted(void);
s32 MinePoint_Count(s32);
void Em_BonePos(void *obj, s16 bone, f32 *out);
extern s8 EmFatalis_MinePtBones[][2];
extern f32 EmFatalis_MinePtRadius[];
void func_3A6120(MinePoint *pt);
void EmRathalos_SetGroup0State(Em *em, u16 state, u8 mode);
void EmRathalos_SetGroup1State(Em *em, u16 state, u8 mode);
void EmRathalos_SetGroup2State(Em *em, u16 state, u8 mode);
void EmRathalos_SetGroup3State(Em *em, u16 state, u8 mode);
/* unit's own declaration: with the definition's void (Em *em, s32 group, s32 state, u8 mode) (game/unk_701810.c), EmRathalos_ChangeState: 6 of 227 words differ, EmRathalos_SetGroup0State: 1 of 114 words differ ... */
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
void Em_Refill7F4(Em *em);
void Em_StartGoalAction(Em *em, s32 arg1);
void Em_ClearPart(Em *em, u16 part);
s16 Player_IsState(void *obj, u8 state, u8 sub);
void StageObj_K25_Create(Player *p, Em *em, u8 k, s32 unused, f32 *pos);
void Versus_RouteEmByTeam(Em *em);
s32 Versus_DefeatCamera(Em *pl);
void Em_SpawnEffect06Periodic(Em *em, s32 bone, f32 *pos, f32 scale);
extern EmCurveKey *EmRathalos_MoveVelYCurves[];
s8 EmMine_Create(Em *em, s32 idx);
s32 EmMine_Update(Em *em);
void EmMine_Remove(Em *em);
typedef struct EmRathalosVec {
    f32 x, y, z;
} EmRathalosVec;
extern EmRathalosVec EmRathalos_CarveThrowPt __attribute__((aligned(8)));
u16 Math_Heading(f32 x1, f32 z1, f32 x2, f32 z2);
void Em_SetMotionRow(Em *em, s32 row, s32 arg2, s32 arg3);
s32 Em_ComputeSize(Em *em, s16 idx);
StageObj *StageObj_K1C_CreateOnEm(Em *em, u8 k, f32 scale);
/* unit's own declaration: with the definition's s32 (Em *em, s16 n, s16 max, s16 interval) (game/unk_708850.c), EmRathalos_State0_12: 64 of 138 words differ, EmRathalos_State0_15: 64 of 138 words differ */
s32 Em_RegenHp(Em *em, s32 n, s32 max, s32 interval);
void Em_ClearLowHpNeed(Em *em);
s32 Save_TestEventFlag(s32);
u16 Area_IdForCurrentTime(u16);
extern s8 *D_008C9BCC[];
s8 Em_NearestActionToBone(Em *em, s32 bone, f32 *pos);
void GmEm_ResetSize(Em *em);
void CamShake_ReqC(Em *em, s32 a);
void *EffEm33_CreatePlus18(Em *em, s16 a, s16 b, s32 c, s32 d);
void Em_AimAtTarget(Em *em);
void Em_GetPitchToTarget(Em *em, s32 *out, f32 a, f32 b);
f32 Math_Sqrtf(f32 x);
void Em_RaiseTrigger80Lax(Em *em);
void Em_HoldPosNoPlayers(Em *em, u16 a, u16 b);
u16 Area_DayId(u16 id);
s32 Quest_IsRunning(s32 ending);
void CamReq_ForMonster(Em *em);
u8 Em_UpdateDamage(Em *em, u8 *out);
void Em_UpdatePendingScript(Em *em);
void Em_ClearFlag6AE(Em *em);
void Em_SetStatus4Timer(Em *em);
void Em_SetStatus2Timer(Em *em);
void EmFatalis_RunState(Em *em, s32 arg1);
void EmFatalis_UpdateRageTimer(Em *em);
void Em_Decay7F4(Em *em);
void Em_CheckCarryRequest(Em *em);
void EmRathalos_RunState(Em *em, s32 arg1);
s32 Em_UpdateCombatTimer(Em *em);
void Em_SetGoal12Dist(Em *em, u8 *p);
void Em_SetStatus1Timer(Em *em);
void Em_SetStatus5Timer(Em *em);
void Em_ResetKindTimer(Em *em);
s32 Em_IsHeld(Em *em);
u16 Area_BaseId(u16);
void Em_CheckLowHp(Em *em, f32 ratio);
void Em_Decay7EC(Em *em);
void Em_Decay7F0Only(Em *em);
void Em_Decay7F0(Em *em);
void EmHermitaur_ChangeState(Em *em, u16 group, u16 state, u8 mode);
s32 EmRathalos_ReachedTarget(Em *em);
void EmRathalos_EndSwoop(Em *em, s32 arg1);
void EmRathalos_StepPitch(Em *em, s32 mode, f32 height);
AreaRecord *Stage_GetAreaRecord(u16 x);
void Area_ToWorld(s32 area, f32 *pos, f32 *out);
u8 Em_AllPlayersHere(Em *em);
s32 Obj_SameArea(void *obj, Player *b);
s32 EmRathalos_ReachedPoint(Em *em);
void Effect0D_SpawnForEm(Em *em, s32 a, s32 b);
f32 *Mtx_FromAnglesYXZ(s32 *angles, f32 *m);
void Vec_Rotate(f32 *out, f32 *in, f32 *mtx);
/* end of prototypes */

void EmRathalos_Setup(Em *em)
{
}

void EmRathalos_SpawnEffects(void *em)
{
    StageObj_K13_Create(em, 0x22, 0);
    Effect01_Spawn((Player *) em, 0);
}

void EmRathalos_Init(Em *em)
{
    EmFlyerWork *w = (EmFlyerWork *)&em->work;
    s16 hp;

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
        case 0xF:
            em->posX = 7900.0f + 1500.0f * (f32) em->slotNo;
            em->posY = 1050.0f;
            em->posZ = 13900.0f;
            break;
        case 0x12:
            em->posX = 10000.0f + 1500.0f * (f32) em->slotNo;
            em->posY = 200.0f;
            em->posZ = 8000.0f;
            break;
        case 0x16:
            em->posX = 9900.0f + 1500.0f * (f32) em->slotNo;
            em->posY = 300.0f;
            em->posZ = 10350.0f;
            break;
        case 0x18:
            em->posX = 10000.0f + 1500.0f * (f32) em->slotNo;
            em->posY = 0.0f;
            em->posZ = 5700.0f;
            em->rotY = 0x4000;
            break;
        case 0x1B:
            em->posX = 14300.0f + 1500.0f * (f32) em->slotNo;
            em->posY = 0.0f;
            em->posZ = 7100.0f;
            break;
        case 0x25:
            em->posX = 9750.0f + 1500.0f * (f32) em->slotNo;
            em->posY = 300.0f;
            em->posZ = 7750.0f;
            break;
        case 0x28:
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
    em->stance = 0;
    w->fxMotion = 0;
    hp = Em_ComputeSize(em, 0);
    em->hp = hp;
    em->hpMax = hp;
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
        case 1:
        case 0x2A:
        case 0x25:
        case 0xB:
        case 0x29:
        case 0x31:
            em->meshShown[1] = 0;
            StageObj_K09_Create(em);
            break;
        }
    }
}

void EmRathalos_Start(Em *em)
{
    EmRathalos_Init(em);
    switch (gQuest.stage) {
    case 0x434:
    case 0x436:
        if (em->type == 0xB || em->type == 0x29 || em->type == 0x31) {
            if (Save_TestEventFlag(0xAB) == 0) {
                em->area = Area_IdForCurrentTime(0x21);
                em->posX = 20000.0f;
                em->posY = 2000.0f;
                em->posZ = 12000.0f;
            }
        }
        break;
    case 0x3F6:
    case 0x3F8:
        if (em->type == 1 || em->type == 0x2A || em->type == 0x25) {
            if (Save_TestEventFlag(0x9C) == 0) {
                em->area = Area_IdForCurrentTime(0x71);
                em->posX = 14350.0f;
                em->posY = 0.0f;
                em->posZ = 9830.0f;
            }
        }
        break;
    }
    EmRathalos_ChangeState(em, 0, 1, 0);
}

void EmRathalos_Reinit(Em *em)
{
    EmRathalos_Init(em);
}

void EmRathalos_EndState(Em *em, s16 arg1, s32 unused)
{
    em->scriptPending = 1;
    if (em->stance == 0) {
        if (arg1 == 1) {
            EmRathalos_ChangeState(em, 0, 0x11, 0);
            return;
        }
        EmRathalos_ChangeState(em, 0, 1, 0);
        return;
    }
    EmRathalos_ChangeState(em, 2, 2, 0);
}

void EmRathalos_EndState2(Em *em, u8 next)
{
    s32 k;

    k = next & 0xFF;
    em->scriptPending = 1;
    em->motionSpeed = 1.0f;
    switch (k) {
    case 0:
        EmRathalos_ChangeState(em, 2, 0xE, 0);
        break;
    case 1:
        EmRathalos_ChangeState(em, 2, 0xF, 0);
        break;
    }
}

void EmRathalos_RestartLayerMotion(Em *em, s32 layer)
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

void EmRathalos_LayerMotionToIdle(Em *em, s32 layer)
{
    switch (layer) {
    case 0:
        if (em->motionId[0] == 0x405) {
            Em_SetLayerMotion(em, 0x3E9, 0xA, 0, 0);
        }
        if (em->motionId[0] == 0x41B) {
            Em_SetLayerMotion(em, 0x3E9, 0xA, 0, 0);
        }
        break;
    case 1:
        if (em->motionId[1] == 0x4CD) {
            Em_SetLayerMotion(em, 0x4B1, 0xA, 0, 1);
        }
        if (em->motionId[1] == 0x4E3) {
            Em_SetLayerMotion(em, 0x4B1, 0xA, 0, 1);
        }
        break;
    case 2:
        if (em->motionId[2] == 0x595) {
            Em_SetLayerMotion(em, 0x579, 0xA, 0, 2);
        }
        if (em->motionId[2] == 0x5AB) {
            Em_SetLayerMotion(em, 0x579, 0xA, 0, 2);
        }
        break;
    }
}

void EmRathalos_State0_00(Em *em)
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

void EmRathalos_State0_01(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x1D, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 1, 0, 0);
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

void EmRathalos_State0_02(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
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

void EmRathalos_State0_03(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
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
    Em_PlaySeAtBoneVolOnFrame2(em, 0x4E, 0x17, 0x2A, 2, 0xD);
}

void EmRathalos_State0_04(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 1, 0, 0);
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

void EmRathalos_State0_05(Em *em)
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
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
    EmRathalos_RestartLayerMotion(em, 0);
    EmRathalos_RestartLayerMotion(em, 2);
    Em_PlaySeAtBoneVolOnFrame2(em, 0x20, 0x2F, 0x23, 1, 0xD);
}

void EmRathalos_State0_06(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
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
    Em_PlaySeAtBoneVolOnFrame2(em, 0x20, 0x20, 0x23, 1, 0xD);
    Em_PlaySeAtBoneVolOnFrame2(em, 0xA0, 0x17, 0x23, 1, 0xD);
}

void EmRathalos_State0_07(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
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
    Em_PlaySeAtBoneVolOnFrame2(em, 4, 0x2D, 0x23, 1, 0xD);
}

void EmRathalos_State0_08(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x1B, 0, 0);
        EmRathalos_LayerMotionToIdle(em, 0);
        EmRathalos_LayerMotionToIdle(em, 1);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 26.0f, 2U) != 0) {
            Eff01_Create(em, 0x29);
        }
        if (Player_PassedFrameScaled(em, 90.0f, 2U) != 0) {
            Eff01_Create(em, 0x29);
        }
        if (em->motion[2].playing == 0) {
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
    EmRathalos_RestartLayerMotion(em, 0);
    EmRathalos_RestartLayerMotion(em, 1);
    Em_PlaySeAtBoneVolOnFrame2(em, 0x10, 0x16, 0x2A, 2, 0xD);
}

void EmRathalos_State0_09(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
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

void EmRathalos_State0_0A(Em *em)
{
    u16 aimAngle;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x36, 0, 0);
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

void EmRathalos_State0_0B(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
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
    Em_PlaySeAtBoneVolOnFrame2(em, 0x10, 0x21, 0x23, 1, 0xD);
    Em_PlaySeAtBoneVolOnFrame2(em, 0x72, 0x1F, 0x23, 1, 0xD);
}

void EmRathalos_State0_0C(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
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

void EmRathalos_State0_0D(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
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
    Em_PlaySeAtBoneVolOnFrame2(em, 2, 0x20, 0x23, 1, 0xD);
    Em_PlaySeAtBoneVolOnFrame2(em, 0x32, 0x1F, 0x23, 1, 0xD);
}

void EmRathalos_State0_0E(Em *em)
{
    u8 temp_v1_2;

    Em_SetFlag6AE(em, 3U);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x4F, 0, 0);
        Em_SetRow638(em, 1);
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
        if ((em->motion[0].playing == 0) && ((em->step += 1, Em_SetMotion(em, 0x3B, 0, 0), Em_AddHp(em, (s32) (0.05f * (f32) em->hpMax)), temp_v1_2 = em->type, (temp_v1_2 == 1)) || (temp_v1_2 == 0x2A) || (temp_v1_2 == 0x25))) {
            EmMine_CreateTimed0(em, &em->posX);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetRow638(em, 0);
            Em_LoadReactionSet(em, 0);
            Em_Clear7ECState(em);
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
    Em_Add7EC(em, 0xDE);
}

void EmRathalos_State0_0F(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x50, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 1, 0, 0);
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
            Em_SetMotion(em, 1, 0, 0);
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

void EmRathalos_State0_10(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x51, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 1, 0, 0);
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
            Em_SetMotion(em, 1, 0, 0);
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

void EmRathalos_State0_11(Em *em)
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
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmRathalos_State0_12(Em *em, EmWork *work)
{
    Em_SetFlag6AE(em, 2U);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x27, 0, 0);
        Em_LoadReactionSet(em, 1);
        if (em->type == 1 || em->type == 0x2A || em->type == 0x25) {
            em->timer = 9000;
        } else {
            em->timer = 7200;
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x22, 0, 0);
        }
        break;
    case 2:
        if (em->type == 1 || em->type == 0x2A || em->type == 0x25) {
            Em_RegenHp(em, 1, (s32) (0.6f * (f32) em->hpMax), 3);
        } else {
            Em_RegenHp(em, 1, (s32) (0.7f * (f32) em->hpMax), 3);
        }
        if (--em->timer <= 0) {
            em->step++;
            Em_ClearLowHpNeed(em);
            EmRathalos_ChangeState(em, 0, 0x17, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, 0, 0x17, 4);
        }
        break;
    }
}

void EmRathalos_State0_13(Em *em)
{
    Em_SetFlag6AE(em, 3U);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x4F, 0, 0);
        Em_SetRow638(em, 1);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 200.0f, 0U) != 0) {
            em->step++;
            EmRathalos_ChangeState(em, 0, 0x28, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, 0, 0x28, 4);
        }
        break;
    }
}

void EmRathalos_State0_28(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetRow638(em, 0);
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
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmRathalos_State0_14(Em *em)
{
    Em_SetFlag6AE(em, 2U);
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
            EmRathalos_ChangeState(em, 0, 0x18, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, 0, 0x18, 4);
        }
        break;
    }
}

void EmRathalos_State0_15(Em *em, EmWork *work)
{
    Em_SetFlag6AE(em, 2U);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x27, 0, 0);
        Em_LoadReactionSet(em, 1);
        if (em->type == 1 || em->type == 0x2A || em->type == 0x25) {
            em->timer = 12600;
        } else {
            em->timer = 9000;
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x22, 0, 0);
        }
        break;
    case 2:
        if (em->type == 1 || em->type == 0x2A || em->type == 0x25) {
            Em_RegenHp(em, 1, (s32) (0.6f * (f32) em->hpMax), 3);
        } else {
            Em_RegenHp(em, 1, (s32) (0.7f * (f32) em->hpMax), 3);
        }
        if (--em->timer <= 0) {
            em->step++;
            Em_ClearLowHpNeed(em);
            EmRathalos_ChangeState(em, 0, 0x18, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, 0, 0x18, 4);
        }
        break;
    }
}

void EmRathalos_State0_16(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
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

void EmRathalos_State0_17(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x25, 0, 0);
        Em_LoadReactionSet(em, 0);
        em->blind = 2;
        if (em->type == 0xB || em->type == 0x29 || em->type == 0x31) {
            EmMine_CreateTimed0(em, &em->posX);
        }
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            EmRathalos_ChangeState(em, 0, 0x19, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, 0, 0x19, 4);
        }
        break;
    }
}

void EmRathalos_State0_18(Em *em)
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
            EmRathalos_ChangeState(em, 0, 0x19, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, 0, 0x19, 4);
        }
        break;
    }
}

void EmRathalos_State0_19(Em *em)
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
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmRathalos_State0_1A(Em *em)
{
    Em_SetFlag6AE(em, 3U);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x4F, 0, 0);
        Em_SetRow638(em, 1);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x3B, 0, 0);
            Em_AddHp(em, (s32) (0.05f * (f32) em->hpMax));
            Em_Add7F0(em, em->need1Max);
            Em_Clear7F0State(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetRow638(em, 0);
            Em_LoadReactionSet(em, 0);
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmRathalos_State0_1B(Em *em)
{
    Em_SetFlag6AE(em, 2U);
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
            EmRathalos_ChangeState(em, 0, 0x1C, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, 0, 0x1C, 4);
        }
        break;
    }
}

void EmRathalos_State0_1C(Em *em)
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
            EmRathalos_ChangeState(em, 0, 0x19, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, 0, 0x19, 4);
        }
        break;
    }
}

void EmRathalos_State0_1D(Em *em, EmWork *work)
{
    f32 pos[3];

    em->holdTimer = 5;
    Em_SetFlag6AE(em, 2);
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
            EmRathalos_ChangeState(em, 4, 0x12, 3);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, 4, 0x12, 3);
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

void EmRathalos_State0_1F(Em *em, EmWork *work)
{
    f32 pos[3];

    em->holdTimer = 5;
    Em_SetFlag6AE(em, 2);
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
            EmRathalos_ChangeState(em, 4, 0x13, 3);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, 4, 0x13, 3);
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

void EmRathalos_State0_21(Em *em, EmWork *work, s32 variant)
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
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmRathalos_State1_00(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 3, 0, 0);
        break;
    case 1:
        if (Em_StepToGoal(em, 0x40, 0) == 1) {
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmRathalos_State1_01(Em *em, EmWork *work)
{
    EmRathalos_EndState(em, 0, 0);
}

void EmRathalos_State1_02(Em *em, EmWork *work)
{
    EmRathalos_EndState(em, 0, 0);
}

void EmRathalos_State1_03(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_StartTurnToTarget(em, &EmRathalos_TurnParams[0]);
        break;
    case 1:
        if (Em_TurnStep(em, &EmRathalos_TurnParams[0]) == 1) {
            em->step++;
            Player_ClearFlag(em, 0x20000);
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmRathalos_State1_04(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x11, 0, 0);
        break;
    case 1:
        if (Em_StepToGoal(em, 0x40, 0) == 1) {
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmRathalos_State1_05(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_StartTurnToTarget(em, &EmRathalos_TurnParams[1]);
        break;
    case 1:
        if (Em_TurnStep(em, &EmRathalos_TurnParams[1]) == 1) {
            em->step++;
            Player_ClearFlag(em, 0x20000);
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmRathalos_State1_06(Em *em, EmWork *work)
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
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmRathalos_State1_07(Em *em, EmWork *work)
{
    u16 aimAngle;
    u32 diff;
    s32 rotY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x36, 0, 0);
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

void EmRathalos_State2_00(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x12, 0, 0);
        if (em->type == 0xB || em->type == 0x29 || em->type == 0x31) {
            EmRathalos_StartCurves(em, 0xF);
        } else {
            EmRathalos_StartCurves(em, 2);
        }
        work->flyGuard = 0;
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 38.0f, 0U) != 0) {
            em->step++;
            em->stance = 2;
            EmRathalos_UpdateCurves(em);
        }
        break;
    case 2:
        if (EmRathalos_UpdateCurves(em)) {
            em->step++;
            EmRathalos_EndState2(em, 0U);
        }
        break;
    }
}

void EmRathalos_State2_01_FlyInLand(Em *em, EmWork *work)
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
        work->moveAngY = em->rotY;
        Em_MoveRotated(em, &work->moveAngX);
        if (em->velY < -10.0f) {
            em->velY += 0.75f;
        }
        if (!((630.0f + em->floorY) <= em->posY) && ((Player_PassedFrameScaled(em, 10.0f, 0U) != 0) || (Player_PassedFrameScaled(em, 86.0f, 0U) != 0) || (Player_PassedFrameScaled(em, 160.0f, 0U) != 0))) {
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
        work->moveAngX = 0;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
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
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
    h = em->floorY;
    if (em->posY < h) {
        em->posY = h;
    }
}

void EmRathalos_State2_02(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        work->turnSpeed = 0x100;
        Em_SetMotion(em, 0xF, 0, 0);
        work->flyGuard = 0;
        break;
    case 1:
        EmRathalos_MoveByVelYCurve(em, 1U);
        if (--em->timer <= 0) {
            em->step++;
            EmRathalos_ChangeState(em, 2, 1, 1);
        }
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 1);
        if (em->timer == 0x12C) {
            EmRathalos_EndState2(em, 0U);
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

void EmRathalos_State2_03(Em *em, EmWork *work)
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
        if (em->type == 0xB || em->type == 0x29 || em->type == 0x31) {
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
            em->stance = 2;
            EmRathalos_UpdateCurves(em);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
        }
        break;
    case 2:
        if ((EmRathalos_UpdateCurves(em) & 0xFF) && (em->posY <= (800.0f + em->floorY))) {
            em->step++;
            Em_SetMotion(em, 0xB, 0, 0);
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
            Em_SetMotion(em, 0x13, 0, 0);
            em->stance = 0;
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

void EmRathalos_State2_04(Em *em, EmWork *work)
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
        EmRathalos_StartCurves(em, 3);
        em->velZ = 20.0f;
        work->flyGuard = 0;
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 30.0f) != 0) {
            em->stance = 2;
            ended = EmRathalos_UpdateCurves(em) & 0xFF;
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

void EmRathalos_State2_05(Em *em, EmWork *work)
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
        EmRathalos_StartCurves(em, 3);
        em->velZ = -20.0f;
        work->flyGuard = 0;
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 30.0f) != 0) {
            em->stance = 2;
            ended = EmRathalos_UpdateCurves(em) & 0xFF;
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

void EmRathalos_State2_06(Em *em, EmWork *work)
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
        if (em->velZ <= 50.0f) {
            em->velZ = 50.0f;
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
            n = ((EmFlyerWork *)work)->legsLeft - 1;
            ((EmFlyerWork *)work)->legsLeft = n;
            if ((n & 0xFF) <= 0) {
                EmRathalos_EndState2(em, 1U);
            } else {
                em->curGoalNo++;
                em->curGoalNo &= 3;
                Em_ComputeGoal2(em, em->goal);
                em->timer = (s32) ((1.5f * Vec_DistanceXZ(&em->posX, em->goal)) / 50.0f);
                Em_SetStateRand(em, 2, 6);
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

void EmRathalos_State2_07(Em *em, EmWork *work)
{
    f32 dist;
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        work->turnSpeed = 0x100;
        Em_SetMotion(em, 0xF, 0, 0);
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
        if (--em->timer > 0) {
            if (dist <= (10.0f * em->velZ)) {
                goto block_9;
            }
        } else {
block_9:
            em->step++;
            Player_ClearVelocity(em);
            EmRathalos_EndState2(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void Hud_TriggerGlow(Em *em);
void EmRathalos_State2_08(Em *em, EmWork *work)
{
    u16 next;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        Em_SetMotion(em, 0xF, 0, 0);
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
            em->stance = 2;
            em->lookAtTarget = 0;
            Player_ClearVelocity(em);
            em->velZ = 50.0f;
            Em_SetMotion(em, 0xC, 0, 0);
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
        if (em->nextArea == next || next == 0xFFFF) {
            Hud_TriggerGlow(em);
            EmRathalos_ChangeState(em, 2, 9, 1);
        } else {
            if (em->remote == 0) {
                em->nextArea = next;
                em->goalNo = next;
                em->goalKind = 3;
                Em_ComputeGoal(em, em->goal);
            }
            EmRathalos_ChangeState(em, 2, 0xD, 1);
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

void EmRathalos_State2_09(Em *em, EmWork *work)
{
    f32 d;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        work->turnSpeed = 0x100;
        Em_SetMotion(em, 0xF, 0, 0);
        Player_ClearVelocity(em);
        em->velZ = 80.0f;
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->rotY = (s32) em->aimAngle;
        *(u16 *)&em->destArea = 0xFFFF;
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
        d = Vec_DistanceXZ(&em->posX, em->goal);
        em->timer--;
        if (d <= 1000.0f || em->timer < 0) {
            em->step++;
            em->timer = 600;
            EmRathalos_ChangeState(em, 2, 1, 1);
        }
        break;
    case 2:
        if (em->remote == 0) {
            em->timer = 0x258;
            EmRathalos_ChangeState(em, 2, 1, 1);
        }
        break;
    }
    if (em->posY < em->floorY) {
        em->posY = em->floorY;
    }
}

void EmRathalos_State2_0A(Em *em, EmWork *work)
{
    f32 temp_f1;
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        work->turnSpeed = 0x100;
        Em_SetMotion(em, 0xF, 0, 0);
        em->stance = 2;
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
        Em_SetMotion(em, 0xC, 0, 0);
        
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
        if (em->area == 0xF) {
            em->area = 0x13;
        } else {
            em->area = 0xF;
        }
        EmRathalos_EndState2(em, 1U);
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmRathalos_State2_0B(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0xC, 0, 0);
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
            EmRathalos_ChangeState(em, 2, 1, 1);
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
                EmRathalos_ChangeState(em, 2, 1, 1);
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
                EmRathalos_ChangeState(em, 2, 1, 1);
            }
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmRathalos_State2_0C(Em *em)
{
    f32 floorY;
    s32 ended;

    ended = 0;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x34, 0, 0);
        EmRathalos_StartCurves(em, 0xE);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 76.0f) != 0) {
            em->stance = 2;
            ended = EmRathalos_UpdateCurves(em) & 0xFF;
        }
        if ((ended != 0) && (em->posY <= em->floorY)) {
            Em_SetMotion(em, 0x39, 0, 0);
            em->step++;
            em->stance = 0;
            em->posY = em->floorY;
            EmRathalos_SpawnK14Type0B(em);
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

void EmRathalos_State2_0D(Em *em, EmWork *work)
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
        Em_SetMotion(em, 0xF, 0, 0);
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
        if (em->nextArea == next || next == 0xFFFF) {
            Hud_TriggerGlow(em);
            EmRathalos_ChangeState(em, 2, 9, 1);
        } else {
            if (em->remote == 0) {
                em->nextArea = next;
                em->goalNo = next;
                em->goalKind = 3;
                Em_ComputeGoal(em, em->goal);
            }
            EmRathalos_ChangeState(em, 2, 0xD, 1);
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

void EmRathalos_State2_0E(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        Em_SetMotion(em, 0xF, 0, 0);
        work->flyGuard = 0;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmRathalos_EndState2(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmRathalos_State2_0F(Em *em, EmWork *work)
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
            EmRathalos_EndState2(em, 1U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmRathalos_State2_10(Em *em, EmWork *work)
{
    f32 floorY;
    s32 done;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        Em_SetMotion(em, 0xF, 0, 0);
        work->turnSpeed = 0x180;
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        work->flyGuard = 0;
        em->timer = 0x96;
        break;
    case 1:
        EmRathalos_MoveByVelYCurve(em, 1U);
        done = Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 1) & 0xFF;
        if (--em->timer > 0) {
            if (done != 0) {
                goto block_9;
            }
        } else {
block_9:
            em->step++;
            EmRathalos_EndState2(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmRathalos_State2_11(Em *em, EmWork *work)
{
    f32 floorY;
    u16 motionId;
    u8 stateVar1;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        em->velX = 0.0f;
        em->velY = 0.0f;
        Player_ClearAccel2(em);
        if (em->velZ < 50.0f) {
            em->stateVar1 = 0;
            em->velZ = 50.0f;
            Em_SetMotion(em, 0xC, 0, 0);
        } else {
            em->stateVar1 = 1;
            motionId = em->motionId[0];
            if ((motionId != 0x3F4) && (motionId != 0x3F6)) {
                Em_SetMotion(em, 0xC, 0, 0);
            }
        }
        work->flyGuard = 1;
        break;
    case 1:
        stateVar1 = em->stateVar1;
        if ((stateVar1 == 0) && (em->motion[0].playing == 0)) {
            em->stateVar1 = stateVar1 + 1;
            Em_SetMotion(em, 0xE, 0, 0);
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
            Em_SetStateRand(em, 2, 0x11);
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

void EmRathalos_State2_12(Em *em, EmWork *work)
{
    f32 floorY;
    s32 rotX;
    s32 rotZ;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x5A, 0, 0);
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
            Em_SetMotion(em, 0xF, 0, 0);
            em->timer = 0x12C;
            EmRathalos_ChangeState(em, 2, 1, 1);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->timer = 0x12C;
            Em_SetMotion(em, 0xF, 0, 0);
            if (em->remote == 0) {
                EmRathalos_ChangeState(em, 2, 1, 1);
            }
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmRathalos_State2_13(Em *em, EmWork *work)
{
    f32 floorY;
    u16 motionId;
    u8 stateVar1;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        em->velX = 0.0f;
        em->velY = 0.0f;
        Player_ClearAccel2(em);
        if (em->velZ < 50.0f) {
            em->stateVar1 = 0;
            em->velZ = 50.0f;
            Em_SetMotion(em, 0xC, 0, 0);
        } else {
            em->stateVar1 = 1;
            motionId = em->motionId[0];
            if ((motionId != 0x3F4) && (motionId != 0x3F6)) {
                Em_SetMotion(em, 0xC, 0, 0);
            }
        }
        work->flyGuard = 1;
        break;
    case 1:
        stateVar1 = em->stateVar1;
        if ((stateVar1 == 0) && (em->motion[0].playing == 0)) {
            em->stateVar1 = stateVar1 + 1;
            Em_SetMotion(em, 0xE, 0, 0);
        }
        if (!(Vec_DistanceXZ(&em->posX, em->goal) <= 1000.0f)) {
            if (--em->timer <= 0) {
                goto block_17;
            }
        } else {
block_17:
            em->step++;
            EmRathalos_EndState2(em, 1U);
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

void EmRathalos_State2_14(Em *em)
{
    EmRathalos_EndState(em, 0, 0);
}

void EmRathalos_State2_15(Em *em, EmWork *work)
{
    f32 dist;
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        work->turnSpeed = 0x100;
        Em_SetMotion(em, 0xF, 0, 0);
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
            EmRathalos_EndState2(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmRathalos_State2_16(Em *em, EmWork *work)
{
    f32 floorY;
    s32 rotX;
    s32 rotZ;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x5A, 0, 0);
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
            EmRathalos_EndState2(em, 0U);
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

void EmRathalos_State2_17(Em *em, EmWork *work)
{
    f32 floorY;
    f32 velY;
    f32 y;

    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x12, 0xA, 0x1E);
        EmRathalos_StartCurves(em, 2);
        em->stance = 2;
        em->holdTimer = 0;
        em->unk8AD = 0;
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
        }
        break;
    case 3:
        work->moveAngX = 0;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
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

void EmRathalos_State2_18(Em *em, EmWork *work)
{
    f32 pos[3];
    f32 floorY;
    s32 timer;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        Em_SetMotion(em, 0xF, 0, 0);
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
            EmRathalos_EndState2(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmRathalos_State3_00(Em *em)
{
    switch (em->step) {
    case 0:
        if (Em_TurnToTargetMotion(em, 0xE38, 3, 6, 5) != 0) {
            em->step++;
            em->stance = 0;
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

void EmRathalos_State3_02(Em *em, EmWork *work)
{
    f32 floorY;
    u8 stateVar0;

    switch (em->step) {
    case 0:
        stateVar0 = em->stateVar0;
        switch (stateVar0) {
        case 0:
            em->stateVar0 = stateVar0 + 1;
            em->stance = 0;
            
        case 1:
            if (Em_TurnToTargetMotion(em, 0xE38, 3, 6, 5) != 0) {
                em->lookAtTarget = 0;
                em->step++;
                Em_SetMotion(em, 0x28, 0, 0);
                EmRathalos_StartCurves(em, 5);
            }
        }
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 60.0f) != 0) {
            em->stance = 2;
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
            em->stance = 0;
            em->posY = em->floorY;
            Em_SetMotion(em, 0x29, 0, 0);
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

void EmRathalos_State3_03(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
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

void EmRathalos_State3_04(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        break;
    case 1:
        if (Em_TurnToTargetMotion(em, 0xE38, 3, 6, 5) != 0) {
            em->step++;
            em->lookAtTarget = 0;
            Em_SetMotion(em, 0x2F, 0, 0);
        }
        break;
    case 2:
        if (EmMotion_ReachedFrame(em, 0, 154.0f) != 0 && EmMotion_ReachedFrame(em, 0, 276.0f) == 0) {
            if (!(gScene.frame & 3)) {
                StageObj_K14_Create((Em *) em, 0x1A, (u16) Rand_Next(1) & 1, 1.0f);
            }
        } else if (Player_PassedFrameScaled(em, 78.0f, 0U) != 0) {
            StageObj_K11_CreateAtBone(em, 0x24, 1, 0);
            StageObj_K11_CreateAtBone(em, 0x24, 2, 0);
            EffEm08_CreateAtBone(em, 0x22, 0, 0, 0, 0);
        }
        if (em->motion[0].playing == 0) {
            em->lookAtTarget = 0;
            em->step++;
            if (--((EmFlyerWork *)work)->countdown <= 0) {
                EmRathalos_EndState(em, 0, 0);
            } else {
                Em_SetTargetAngle(em);
                Em_SetStateRand(em, 3, 4);
            }
        }
        break;
    }
}

void EmRathalos_State3_05(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x23, 0, 0);
        Motion_InitTime(em, 0);
        em->lookAtTarget = 1;
        Eff01_Create(em, 1);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmRathalos_State3_06(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x2B, 0, 0);
        Motion_InitTime(em, 0);
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
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmRathalos_State3_07(Em *em)
{
    u8 stateVar0;

    switch (em->step) {
    case 0:
        stateVar0 = em->stateVar0;
        switch (stateVar0) {
        case 0:
            em->stateVar0 = stateVar0 + 1;
            em->stance = 0;
            em->lookAtTarget = 0;
            
        case 1:
            if (Em_TurnToTargetMotion(em, 0xE38, 3, 6, 5) != 0) {
                em->step++;
                em->lookAtTarget = 0;
                Em_SetMotion(em, 0x22, 0, 0);
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

void EmRathalos_State3_08_SwoopThenLeave(Em *em, EmWork *work)
{
    f32 to[3];
    f32 pos[3];
    f32 p2[3];
    AreaRecord *area;
    Player *p;

    area = Stage_GetAreaRecord(em->area);
    if (work->wallTimer != 0) {
        if (--work->wallTimer <= 0) {
            work->wallTimer = 0;
        }
    }
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
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
        p = &gPlayers[em->targetNo];
        if ((Obj_SameArea(em, p) & 0xFF) == 0 && p->active != 0) {
            EmRathalos_EndSwoop(em, (s32) work);
            break;
        }
        Em_GetTargetPos(em, em->targetNo, pos);
        Area_ToWorld(p->area, pos, to);
        em->aimAngle = Math_AngleXZ(em->mapPos, to);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 0xD1, &EmRathalos_FlyParam);
        if (em->aimAngle <= 0x800 || em->aimAngle >= 0xF800) {
            if (!(4000.0f < Vec_DistanceXZ(&em->posX, pos))) {
                EmRathalos_EndSwoop(em, (s32) work);
            } else {
                em->step++;
                work->turnSpeed = 0x100;
                Em_SetMotion(em, 0x2C, 0, 0);
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
            Em_SetMotion(em, 0x57, 0, 0);
            em->velZ = 100.0f;
            em->timer = 600;
        }
        p = &gPlayers[em->targetNo];
        if ((Obj_SameArea(em, p) & 0xFF) == 0 && p->active != 0) {
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
        if (em->motionId[0] == 0x415) {
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
        if (em->motionId[0] == 0x415 && em->motion[0].playing == 0) {
            em->step = 6;
            Em_SetMotion(em, 0xE, 0, 0);
            EmRathalos_SetEscapePoint(em);
        }
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 2);
        if (work->wallTimer == 0) {
        EmRathalos_StepPitch(em, 2, 100.0f);
        if (em->motionId[0] == 0x415 && EmMotion_ReachedFrame(em, 0, 76.0f)) {
            Em_GetPitchToTarget(em, &work->moveAngX, 300.0f, 0.0f);
        } else if (em->targetNo != -1) {
            Em_GetTargetPos(em, em->targetNo, p2);
            if (!(200.0f + p2[1] < em->posY)) {
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

void EmRathalos_State3_09(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x31, 0, 0);
        EmRathalos_StartCurves(em, 0xD);
        break;
    case 1:
        Em_PlaySeAtBoneOnFrame(em, 0x78, 0x20, 0x23);
        Em_TurnToGoal(em, (s32) ((EmFlyerWork *)work)->turnSpeed, 2);
        EmRathalos_UpdateCurves(em);
        if (Player_PassedFrameScaled(em, 60.0f, 0U) != 0) {
            em->step++;
            Em_SetMotion(em, 0x2E, 0, 0);
            EmRathalos_StartCurves(em, 0x10);
        }
        break;
    case 2:
        Em_PlaySeAtBoneOnFrame(em, 0x78, 0x20, 0x23);
        Em_TurnToGoal(em, (s32) ((EmFlyerWork *)work)->turnSpeed, 2);
        EmRathalos_UpdateCurves(em);
        if (Player_PassedFrameScaled(em, 6.0f, 0U) != 0) {
            StageObj_K11_CreateAtBone(em, 0x24, 1, 0);
            StageObj_K11_CreateAtBone(em, 0x24, 2, 0);
            EffEm08_CreateAtBone(em, 0x22, 0, 1, 0x1E94, 0);
        }
        if (Player_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            em->step++;
            Em_SetMotion(em, 0xF, 2, 0x22);
        }
        break;
    case 3:
        EmRathalos_MoveByVelYCurve(em, 1U);
        if (em->motion[0].playing == 0) {
            em->step++;
            EmRathalos_EndState2(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmRathalos_State3_0A(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x31, 0, 0);
        EmRathalos_StartCurves(em, 0xD);
        break;
    case 1:
        Em_PlaySeAtBoneOnFrame(em, 0x78, 0x20, 0x23);
        Em_TurnToGoal(em, (s32) ((EmFlyerWork *)work)->turnSpeed, 2);
        EmRathalos_UpdateCurves(em);
        if (Player_PassedFrameScaled(em, 60.0f, 0U) != 0) {
            em->step++;
            em->step++;
            Em_SetMotion(em, 0x2E, 0, 0);
            EmRathalos_StartCurves(em, 0x10);
        }
        break;
    case 2:
    case 3:
        EmRathalos_UpdateCurves(em);
        Em_TurnToGoal(em, (s32) ((EmFlyerWork *)work)->turnSpeed, 2);
        if (Player_PassedFrameScaled(em, 6.0f, 0U) != 0) {
            StageObj_K11_CreateAtBone(em, 0x24, 1, 0);
            StageObj_K11_CreateAtBone(em, 0x24, 2, 0);
            EffEm08_CreateAtBone(em, 0x22, 0, 2, 0x1E94, 0);
        }
        if (Player_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            em->step++;
            Em_SetMotion(em, 0xF, 2, 0x22);
        }
        break;
    case 4:
        EmRathalos_MoveByVelYCurve(em, 1U);
        if (em->motion[0].playing == 0) {
            em->step++;
            EmRathalos_EndState2(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmRathalos_State3_0B(Em *em, EmWork *work)
{
    f32 pos[4];
    f32 spd;
    f32 d;
    s32 n;
    EmFlyerWork *fw = (EmFlyerWork *)work;

    switch (em->step) {
    case 0:
        em->stance = 2;
        Vec_Set(fw->startPos, em->posX, em->posY, em->posZ);
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 2);
        Em_AimAtTarget(em);
        em->lookAtTarget = 0;
        if (em->aimAngle <= 0x100 || em->aimAngle >= 0xFF00) {
            em->step = 2;
            Em_SetMotion(em, 0x35, 0, 0);
            Player_ClearVelocity(em);
            em->timer = 45;
        } else {
            em->step++;
            em->timer = 120;
            if (em->motionId[0] != 0x3F7) {
                Em_SetMotion(em, 0xF, 0, 0);
            }
        }
        break;
    case 1:
        EmRathalos_MoveByVelYCurve(em, 1);
        if (--em->timer <= 0) {
            em->step = 0x62;
            em->timer = 300;
            EmRathalos_ChangeState(em, 2, 1, 1);
            break;
        }
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 2);
        Em_AimAtTarget(em);
        if (em->aimAngle <= 0x100 || em->aimAngle >= 0xFF00) {
            em->step = 2;
            Em_SetMotion(em, 0x35, 0, 0);
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
                spd = em->targetDist / (f32) em->timer--;
                if (1.2f * em->velZ >= spd && spd <= 0.8f * em->velZ) {
                    em->velZ = spd;
                } else if (em->velZ < spd) {
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
            spd = pos[1];
            if (em->posY < spd) {
                em->posY = spd;
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
        d = Vec_Distance((f32 *) &em->posX, fw->startPos);
        work->moveAngX = 0;
        work->moveAngY = Math_AngleXZ(&em->posX, fw->startPos) & 0xFFFF;
        work->moveAngZ = 0;
        if (em->timer != 0) {
            em->velZ = d / (f32) em->timer;
            em->velY = (fw->startPos[1] - em->posY) / (f32) em->timer--;
            Em_MoveRotated(em, &work->moveAngX);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            Player_ClearVelocity(em);
            EmRathalos_EndState2(em, 0);
        }
        break;
    case 0x62:
    case 0x63:
        if (--em->timer <= 0) {
            em->timer = 300;
            if (em->remote == 0) {
                EmRathalos_ChangeState(em, 2, 1, 1);
            }
        }
        break;
    }
}

void EmRathalos_State3_0C(Em *em, EmWork *work)
{
    f32 pos[3];
    s32 r;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 1;
        em->lookAtPoint = 1;
        break;
    case 1:
        r = Em_NearestActionToBone(em, 0x23, em->lookPoint);
        Em_BonePos(em, 0x23, pos);
        if (!(150.0f < Vec_Distance((f32 *) pos, (f32 *) em->lookPoint))) {
            em->step++;
            Em_SetMotion(em, 0x23, 0, 0);
            break;
        }
        if (r == 0) {
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

void EmRathalos_State3_0D(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
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

void EmRathalos_State3_0E(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
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

void EmRathalos_State3_0F(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x11, 0, 0);
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

void EmRathalos_State3_10(Em *em, EmWork *work, s32 variant)
{
    f32 floorY;
    s32 ended;
    u8 step;

    step = em->step;
    ended = 0;
    switch (step) {
    case 0:
        em->step = step + 1;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x34, 0, 0);
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
        if (Player_PassedFrameScaled(em, 90.0f, 0U) != 0) {
            StageObj_K11_CreateAtBone(em, 0x24, 1, 0);
            StageObj_K11_CreateAtBone(em, 0x24, 2, 0);
            EffEm08_CreateAtBone(em, 0x22, 0, 2, 0x2222, 0);
        }
        if (EmMotion_ReachedFrame(em, 0, 76.0f) != 0) {
            em->stance = 2;
            ended = EmRathalos_UpdateCurves(em) & 0xFF;
        }
        if ((ended != 0) && (em->posY <= em->floorY)) {
            Em_SetMotion(em, 0x39, 0, 0);
            em->step++;
            em->stance = 0;
            em->posY = em->floorY;
            EmRathalos_SpawnK14Type0B(em);
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

void EmRathalos_State3_11(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x31, 0, 0);
        EmRathalos_StartCurves(em, 0xDU);
        break;
    case 1:
        Em_PlaySeAtBoneOnFrame(em, 0x78, 0x20, 0x23);
        EmRathalos_UpdateCurves(em);
        Em_TurnToGoal(em, (s32) ((EmFlyerWork *)work)->turnSpeed, 2);
        if (Player_PassedFrameScaled(em, 60.0f, 0U) != 0) {
            em->step++;
            em->step++;
            Em_SetMotion(em, 0x2E, 0, 0);
            EmRathalos_StartCurves(em, 0x10U);
        }
        break;
    case 3:
    case 2:
        EmRathalos_UpdateCurves(em);
        Em_TurnToGoal(em, (s32) ((EmFlyerWork *)work)->turnSpeed, 2);
        if (Player_PassedFrameScaled(em, 6.0f, 0U) != 0) {
            StageObj_K11_CreateAtBone(em, 0x24, 1, 0);
            StageObj_K11_CreateAtBone(em, 0x24, 2, 0);
            EffEm08_CreateAtBone(em, 0x22, 0, 2, 0x1E94, 0);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x2E, 4, 0);
        }
        break;
    case 4:
        EmRathalos_UpdateCurves(em);
        Em_TurnToGoal(em, (s32) ((EmFlyerWork *)work)->turnSpeed, 2);
        if (Player_PassedFrameScaled(em, 6.0f, 1U) != 0) {
            StageObj_K11_CreateAtBone(em, 0x24, 1, 0);
            StageObj_K11_CreateAtBone(em, 0x24, 2, 0);
            EffEm08_CreateAtBone(em, 0x22, 0, 2, 0x1E94, 0);
        }
        if (Player_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            em->step++;
            Em_SetMotion(em, 0xF, 2, 0x22);
        }
        break;
    case 5:
        EmRathalos_MoveByVelYCurve(em, 1U);
        if (em->motion[1].playing == 0) {
            em->step++;
            EmRathalos_EndState2(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmRathalos_State3_12(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x11, 0, 0);
        break;
    case 1:
        Em_StepToGoal(em, 0x40, 1);
        if (em->goalDist <= 500.0f) {
            em->timer = 1;
        }
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x5B, 0, 0);
            Eff01_Create(em, 0x24);
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

void EmRathalos_State3_13(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x23, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmRathalos_State3_14(Em *em)
{
    f32 floorY;
    s32 ended;

    ended = 0;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x5C, 0, 0);
        EmRathalos_StartCurves(em, 0xA);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 94.0f) != 0) {
            em->stance = 2;
            ended = EmRathalos_UpdateCurves(em) & 0xFF;
        }
        if ((ended != 0) && (em->posY <= (372.0f + em->floorY))) {
            em->step++;
            Em_SetMotion(em, 0x5D, 0, 0);
            EmRathalos_StartCurves(em, 0xB);
        }
        break;
    case 2:
        EmRathalos_UpdateCurves(em);
        if (em->posY <= em->floorY) {
            em->step++;
            Em_SetMotion(em, 0x5E, 0, 0);
            em->stance = 0;
            em->posY = em->floorY;
            EmRathalos_SpawnK14Type0B(em);
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

void EmRathalos_State3_15_SwoopThenLand(Em *em, EmWork *work)
{
    f32 to[3];
    f32 pos[3];
    f32 p2[3];
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
        em->stance = 2;
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
        Area_ToWorld(((Player *) &gPlayers[idx])->area, pos, to);
        em->aimAngle = Math_AngleXZ(em->mapPos, to);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 0xD1, &EmRathalos_FlyParam);
        if (em->aimAngle <= 0x800 || em->aimAngle >= 0xF800) {
            if (!(4000.0f < Vec_DistanceXZ(&em->posX, pos))) {
                EmRathalos_EndSwoop(em, (s32) work);
            } else {
                em->step++;
                work->turnSpeed = 0x100;
                Em_SetMotion(em, 0x2C, 0, 0);
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
            Em_SetMotion(em, 0x57, 0, 0);
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
        if (em->motionId[0] == 0x415) {
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
        if (em->motionId[0] == 0x415 && em->motion[0].playing == 0) {
            em->step = 6;
            em->accelX = 0.0f;
            em->accelZ = -10.0f;
            em->accelY = -10.0f;
        }
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 2);
        if (work->wallTimer == 0) {
        EmRathalos_StepPitch(em, 2, 100.0f);
        if (em->motionId[0] == 0x415 && EmMotion_ReachedFrame(em, 0, 76.0f)) {
            Em_GetPitchToTarget(em, &work->moveAngX, 50.0f, 0.0f);
        } else if (em->targetNo != -1) {
            Em_GetTargetPos(em, em->targetNo, p2);
            if (!(200.0f + p2[1] < em->posY)) {
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
            em->stance = 0;
            Em_SetMotion(em, 0x29, 0, 0);
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

void EmRathalos_State3_16(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x31, 0, 0);
        EmRathalos_StartCurves(em, 0xDU);
        break;
    case 1:
        Em_PlaySeAtBoneOnFrame(em, 0x78, 0x20, 0x23);
        EmRathalos_UpdateCurves(em);
        Em_TurnToGoal(em, (s32) ((EmFlyerWork *)work)->turnSpeed, 2);
        if (Player_PassedFrameScaled(em, 60.0f, 0U) != 0) {
            em->step++;
            em->step++;
            Em_SetMotion(em, 0x2E, 0, 0);
            EmRathalos_StartCurves(em, 0x10U);
        }
        break;
    case 3:
    case 4:
    case 2:
        EmRathalos_UpdateCurves(em);
        Em_TurnToGoal(em, (s32) ((EmFlyerWork *)work)->turnSpeed, 2);
        if (Player_PassedFrameScaled(em, 6.0f, 1U) != 0) {
            StageObj_K11_CreateAtBone(em, 0x24, 1, 0);
            StageObj_K11_CreateAtBone(em, 0x24, 2, 0);
            EffEm08_CreateAtBone(em, 0x22, 0, 2, 0x1E94, 0);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x2E, 4, 0);
        }
        break;
    case 5:
        EmRathalos_UpdateCurves(em);
        Em_TurnToGoal(em, (s32) ((EmFlyerWork *)work)->turnSpeed, 2);
        if (Player_PassedFrameScaled(em, 6.0f, 1U) != 0) {
            StageObj_K11_CreateAtBone(em, 0x24, 1, 0);
            StageObj_K11_CreateAtBone(em, 0x24, 2, 0);
            EffEm08_CreateAtBone(em, 0x22, 0, 2, 0x1E94, 0);
        }
        if (Player_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            em->step++;
            Em_SetMotion(em, 0xF, 2, 0x22);
        }
        break;
    case 6:
        EmRathalos_MoveByVelYCurve(em, 1U);
        if (em->motion[1].playing == 0) {
            em->step++;
            EmRathalos_EndState2(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmRathalos_State3_17(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        break;
    case 1:
        if (Em_TurnToTargetMotion(em, 0xE38, 3, 6, 5) != 0) {
            em->step++;
            em->lookAtTarget = 0;
            Em_SetMotion(em, 0x32, 0, 0);
        }
        break;
    case 2:
        if ((EmMotion_ReachedFrame(em, 0, 256.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 316.0f) == 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create((Em *) em, 0x1A, 1, 1.0f);
        }
        if (Player_PassedFrameScaled(em, 78.0f, 0U) != 0) {
            StageObj_K11_CreateAtBone(em, 0x24, 1, 0);
            StageObj_K11_CreateAtBone(em, 0x24, 2, 0);
            EffEm08_CreateAtBone(em, 0x22, 0, 0, 0x5B0, 0);
        }
        if (Player_PassedFrameScaled(em, 120.0f, 0U) != 0) {
            StageObj_K11_CreateAtBone(em, 0x24, 1, 0);
            StageObj_K11_CreateAtBone(em, 0x24, 2, 0);
            EffEm08_CreateAtBone(em, 0x22, 0, 0, 0x4FA, 0xF1C8);
        }
        if (Player_PassedFrameScaled(em, 164.0f, 0U) != 0) {
            StageObj_K11_CreateAtBone(em, 0x24, 1, 0);
            StageObj_K11_CreateAtBone(em, 0x24, 2, 0);
            EffEm08_CreateAtBone(em, 0x22, 0, 0, 0x444, 0x9F5);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            em->lookAtTarget = 0;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmRathalos_State3_18(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        break;
    case 1:
        if (Em_TurnToTargetMotion(em, 0xE38, 3, 6, 5) != 0) {
            em->step++;
            em->lookAtTarget = 0;
            Em_SetMotion(em, 0x2F, 0, 0);
        }
        break;
    case 2:
        if (EmMotion_ReachedFrame(em, 0, 154.0f) != 0 && EmMotion_ReachedFrame(em, 0, 276.0f) == 0) {
            if (!(gScene.frame & 3)) {
                StageObj_K14_Create((Em *) em, 0x1A, (u16) Rand_Next(1) & 1, 1.0f);
            }
        } else if (Player_PassedFrameScaled(em, 78.0f, 0U) != 0) {
            StageObj_K11_CreateAtBone(em, 0x24, 1, 0);
            StageObj_K11_CreateAtBone(em, 0x24, 2, 0);
            EffEm08_CreateAtBone(em, 0x22, 0, 0, 0xE39, 0);
        }
        if (em->motion[0].playing == 0) {
            em->lookAtTarget = 0;
            em->step++;
            if (--((EmFlyerWork *)work)->countdown <= 0) {
                EmRathalos_EndState(em, 0, 0);
            } else {
                Em_SetTargetAngle(em);
                Em_SetStateRand(em, 3, 0x18);
            }
        }
        break;
    }
}

void EmRathalos_State3_19(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        break;
    case 1:
        if (Em_TurnToTargetMotion(em, 0xE38, 3, 6, 5) != 0) {
            em->step++;
            em->lookAtTarget = 0;
            Em_SetMotion(em, 0x2F, 0, 0);
        }
        break;
    case 2:
        if (EmMotion_ReachedFrame(em, 0, 154.0f) != 0 && EmMotion_ReachedFrame(em, 0, 276.0f) == 0) {
            if (!(gScene.frame & 3)) {
                StageObj_K14_Create((Em *) em, 0x1A, (u16) Rand_Next(1) & 1, 1.0f);
            }
        } else if (Player_PassedFrameScaled(em, 78.0f, 0U) != 0) {
            StageObj_K11_CreateAtBone(em, 0x24, 1, 0);
            StageObj_K11_CreateAtBone(em, 0x24, 2, 0);
            EffEm08_CreateAtBone(em, 0x22, 0, 0, 0xF1C8, 0);
        }
        if (em->motion[0].playing == 0) {
            em->lookAtTarget = 0;
            em->step++;
            if (--((EmFlyerWork *)work)->countdown <= 0) {
                EmRathalos_EndState(em, 0, 0);
            } else {
                Em_SetTargetAngle(em);
                Em_SetStateRand(em, 3, 0x19);
            }
        }
        break;
    }
}

void EmRathalos_State3_1A(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x2A, 0, 0);
        Motion_InitTime(em, 0);
        break;
    case 1:
        em->rotY += 0x200;
        if (em->motion[1].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x2A, 0, 0);
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

void EmRathalos_State3_1B(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x11, 0, 0);
        break;
    case 1:
        Em_StepToGoal(em, 0x40, 1);
        if (em->goalDist <= 500.0f) {
            em->timer = 1;
        }
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 9, 0, 0);
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

void EmRathalos_StepForward(Em *em, s32 unused)
{
    f32 v[4];
    f32 out[4];
    s32 ang[4];
    f32 mtx[16];

    ang[0] = 0;
    ang[1] = em->rotY;
    ang[2] = 0;
    Vec_Set(v, 0.0f, 0.0f, 10.0f);
    Mtx_FromAnglesYXZ(ang, mtx);
    Vec_Rotate(out, v, mtx);
    em->posX += out[0];
    em->posZ += out[2];
}

void EmRathalos_State3_1C(Em *em, s32 work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x31, 0, 0);
        EmRathalos_StartCurves(em, 0xD);
        break;
    case 1:
        Em_PlaySeAtBoneOnFrame(em, 0x78, 0x20, 0x23);
        EmRathalos_UpdateCurves(em);
        EmRathalos_StepForward(em, work);
        if (Player_PassedFrameScaled(em, 60.0f, 0U) != 0) {
            em->step++;
            Em_SetMotion(em, 0x2E, 0, 0);
            EmRathalos_StartCurves(em, 0x10);
        }
        break;
    case 2:
        Em_PlaySeAtBoneOnFrame(em, 0x78, 0x20, 0x23);
        EmRathalos_UpdateCurves(em);
        EmRathalos_StepForward(em, work);
        if (Player_PassedFrameScaled(em, 6.0f, 0U) != 0) {
            StageObj_K11_CreateAtBone(em, 0x24, 1, 0);
            StageObj_K11_CreateAtBone(em, 0x24, 2, 0);
            EffEm08_CreateAtBone(em, 0x22, 0, 1, 0x2D83, 0);
        }
        if (Player_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            em->step++;
            Em_SetMotion(em, 0xF, 2, 0x22);
        }
        break;
    case 3:
        EmRathalos_MoveByVelYCurve(em, 1U);
        if (em->motion[0].playing == 0) {
            em->step++;
            EmRathalos_EndState2(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmRathalos_State3_1D(Em *em, s32 work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x31, 0, 0);
        EmRathalos_StartCurves(em, 0xDU);
        break;
    case 1:
        Em_PlaySeAtBoneOnFrame(em, 0x78, 0x20, 0x23);
        EmRathalos_UpdateCurves(em);
        EmRathalos_StepForward(em, work);
        if (Player_PassedFrameScaled(em, 60.0f, 0U) != 0) {
            em->step++;
            em->step++;
            Em_SetMotion(em, 0x2E, 0, 0);
            EmRathalos_StartCurves(em, 0x10U);
        }
        break;
    case 3:
    case 4:
    case 2:
        EmRathalos_UpdateCurves(em);
        EmRathalos_StepForward(em, work);
        if (Player_PassedFrameScaled(em, 6.0f, 1U) != 0) {
            StageObj_K11_CreateAtBone(em, 0x24, 1, 0);
            StageObj_K11_CreateAtBone(em, 0x24, 2, 0);
            EffEm08_CreateAtBone(em, 0x22, 0, 2, 0x2D83, 0);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x2E, 4, 0);
        }
        break;
    case 5:
        EmRathalos_UpdateCurves(em);
        EmRathalos_StepForward(em, work);
        if (Player_PassedFrameScaled(em, 6.0f, 1U) != 0) {
            StageObj_K11_CreateAtBone(em, 0x24, 1, 0);
            StageObj_K11_CreateAtBone(em, 0x24, 2, 0);
            EffEm08_CreateAtBone(em, 0x22, 0, 2, 0x2D83, 0);
        }
        if (Player_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            em->step++;
            Em_SetMotion(em, 0xF, 2, 0x22);
        }
        break;
    case 6:
        EmRathalos_MoveByVelYCurve(em, 1U);
        if (em->motion[1].playing == 0) {
            em->step++;
            EmRathalos_EndState2(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmRathalos_State3_1E(Em *em)
{
    f32 floorY;
    s32 ended;
    u8 step;

    step = em->step;
    ended = 0;
    switch (step) {
    case 0:
        em->step = step + 1;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x5C, 0, 0);
        EmRathalos_StartCurves(em, 0xAU);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 94.0f) != 0) {
            em->stance = 2;
            ended = EmRathalos_UpdateCurves(em) & 0xFF;
        }
        if ((ended != 0) && (em->posY <= (372.0f + em->floorY))) {
            em->step++;
            Em_SetMotion(em, 0x5D, 0, 0);
            EmRathalos_StartCurves(em, 0xBU);
        }
        break;
    case 2:
        EmRathalos_UpdateCurves(em);
        if (em->posY <= em->floorY) {
            em->step++;
            Em_SetMotion(em, 0x37, 0, 0);
            em->stance = 0;
            em->posY = em->floorY;
            EmRathalos_SpawnK14Type0B(em);
            CamShake_ReqAIf(em, 2);
        }
        break;
    case 3:
        if (EmMotion_InFrameRange(em, 0, 50.0f, 70.0f) != 0) {
            Em_TurnToGoal(em, 0x100, 2);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x5C, 2, 0x46);
            EmRathalos_StartCurves(em, 0xAU);
        }
        break;
    case 4:
        if (EmMotion_ReachedFrame(em, 0, 94.0f) != 0) {
            em->stance = 2;
            ended = EmRathalos_UpdateCurves(em) & 0xFF;
        }
        if ((ended != 0) && (em->posY <= (372.0f + em->floorY))) {
            em->step++;
            Em_SetMotion(em, 0x5D, 0, 0);
            EmRathalos_StartCurves(em, 0xBU);
        }
        break;
    case 5:
        EmRathalos_UpdateCurves(em);
        if (em->posY <= em->floorY) {
            em->step++;
            Em_SetMotion(em, 0x5E, 0, 0);
            em->stance = 0;
            em->posY = em->floorY;
            EmRathalos_SpawnK14Type0B(em);
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

void EmRathalos_State4_00(Em *em, EmWork *work)
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
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmRathalos_State4_01(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
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

void EmRathalos_State4_02(Em *em, EmWork *work)
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
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmRathalos_State4_03(Em *em, EmWork *work)
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
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmRathalos_State4_04(Em *em, EmWork *work)
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
        Em_SetMotion(em, 0x41, 0, 0);
        break;
    case 1:
        if ((Player_PassedFrameScaled(em, 6.0f, 0U) != 0) && ((s8) em->meshShown[1] != 0)) {
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
            EmRathalos_ChangeState(em, 4, 0xF, 3);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, 4, 0xF, 3);
        }
        break;
    }
}

void EmRathalos_State4_05(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x45, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 0;
            Em_SetMotion(em, 0x46, 0, 0);
        }
        break;
    case 2:
        em->timer++;
        if (em->timer >= 0x78) {
            em->step++;
            Em_SetMotion(em, 0x47, 0, 0);
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

void EmRathalos_State4_06(Em *em, EmWork *work)
{
    EmRathalos_EndState2(em, 0x1);
}

void EmRathalos_State4_07(Em *em, EmWork *work)
{
    EmRathalos_EndState2(em, 0x1);
}

void EmRathalos_State4_08(Em *em, EmWork *work)
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
            EmRathalos_EndState(em, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    }
}

void EmRathalos_State4_09(Em *em, EmWork *work)
{
    em->blind = 2;
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
            EmRathalos_EndState(em, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    }
}

void EmRathalos_State4_0A(Em *em, EmWork *work)
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
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmRathalos_State4_0B(Em *em, EmWork *work)
{
    Em_SetFlag6AE(em, 4U);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x4C, 0, 0);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmRathalos_ChangeState(em, 0, 0x21, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, 0, 0x21, 4);
        }
        break;
    }
    Em_SpawnEffect06Bone(em, 2);
}

void EmRathalos_State4_0C(Em *em, EmWork *work)
{
    Em_SetFlag6AE(em, 3U);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x5F, 0, 0);
        em->noReact = 1;
        em->blind = 0;
        break;
    case 1:
        if ((s8) em->holdTimer < 5) {
            em->holdTimer = 5;
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmRathalos_ChangeState(em, 4, 0xD, 3);
        }
        break;
    case 2:
        if ((s8) em->holdTimer < 5) {
            em->holdTimer = 5;
        }
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, 4, 0xD, 3);
        }
        break;
    }
}

void EmRathalos_State4_0D(Em *em, EmWork *work)
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
            EmRathalos_ChangeState(em, 2, 0x17, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, 2, 0x17, 4);
        }
        if (em->motion[0].playing == 0) {
            Em_SetMotion(em, 0x60, 0, 0);
        }
        break;
    }
}

void EmRathalos_State4_0E(Em *em, EmWork *work)
{
    em->holdTimer = 5;
    Em_SetFlag6AE(em, 4U);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x61, 0, 0);
        Em_RestartScript(em);
        em->noReact = 1;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetGoal12(em);
            EmRathalos_ChangeState(em, 4, 0x11, 3);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, 4, 0x11, 3);
        }
        break;
    }
    Em_SpawnEffect06Bone(em, 2);
}

void EmRathalos_State4_0F(Em *em, EmWork *work)
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

void EmRathalos_State4_10(Em *em, EmWork *work)
{
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

void EmRathalos_State4_11(Em *em, EmWork *work)
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
            EmRathalos_ChangeState(em, 2, 0x17, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, 2, 0x17, 4);
        }
        break;
    }
}

void EmRathalos_State4_12(Em *em, EmWork *work)
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
            Em_SetGoal12(em);
            em->blind = 0;
            EmRathalos_ChangeState(em, 4, 0xD, 3);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, 4, 0xD, 3);
        }
        break;
    }
}

void EmRathalos_State4_13(Em *em, EmWork *work)
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
            Em_SetGoal12(em);
            em->blind = 0;
            EmRathalos_ChangeState(em, 4, 0xD, 3);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, 4, 0xD, 3);
        }
        break;
    }
}

void EmRathalos_State4_14(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x45, 0, 0);
        StageObj_Stun_CreateEm(em, 1, 0x22, (s32) ((f32) em->timer + (em->motion[0].frameCount / 2.0f)), 150.0f, 3.0f);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x46, 0, 0);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x47, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmRathalos_ChangeState(em, 0, 0x22, 4);
        }
        break;
    case 4:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, 0, 0x22, 4);
        }
        break;
    }
}

void EmRathalos_State4_15(Em *em, EmWork *work)
{
    em->holdTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x61, 0, 0);
        Em_RestartScript(em);
        em->noReact = 1;
        StageObj_Stun_CreateEm(em, 1, 0x22, (s32) ((f32) em->timer + (em->motion[0].frameCount / 2.0f)), 150.0f, 3.0f);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetGoal12(em);
            EmRathalos_ChangeState(em, 4, 0x16, 3);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, 4, 0x16, 3);
        }
        break;
    }
}

void EmRathalos_State4_16(Em *em, EmWork *work)
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
            EmRathalos_ChangeState(em, 2, 0x17, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, 2, 0x17, 4);
        }
        break;
    }
}

void EmRathalos_State4_17(Em *em, EmWork *work)
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
            EmRathalos_ChangeState(em, 0, 0x23, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, 0, 0x23, 4);
        }
        break;
    }
}

void EmRathalos_State6_02_QuestIntro(Em *em, EmWork *work)
{
    f32 pos[3];
    f32 off[3];
    f32 out[3];
    s32 ang[3];
    f32 off2[3];
    f32 out2[3];
    s32 ang2[3];
    f32 mtx[16];
    f32 mtx2[16];
    Em *m;
    s8 i;

    Stage_GetAreaRecord(em->area);
    for (i = 0, m = gEms; i < 20; i++, m++) {
        if ((m->type == 0xC || m->type == 0x46) && m->active != 0 && m->variant == 0) {
            goto found;
        }
    }
    m = NULL;
found:
    if (m == NULL) {
        Vec_Set(pos, 10000.0f, 0.0f, 10000.0f);
    } else {
        *(Em **) &m->unk898[0] = em;
        Vec_Set(pos, m->posX, m->posY, m->posZ);
    }
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        em->velZ = 100.0f;
        em->posX = 20000.0f;
        em->posY = 2000.0f;
        em->posZ = 12000.0f;
        em->goal[0] = 26500.0f;
        em->goal[1] = 2000.0f;
        em->goal[2] = 19000.0f;
        em->rotY = Math_AngleXZ(&em->posX, em->goal);
        GmEm_ResetSize(em);
        Em_SetMotion(em, 0xE, 0, 0);
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
            Em_SetMotion(em, 0x2C, 0, 0);
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
            Em_SetMotion(em, 0x57, 0, 0);
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
            (em->motionId[0] == 0x415 && (EmMotion_ReachedFrame(em, 0, 52.0f) || em->motion[0].playing == 0))) {
            em->step++;
            em->posY = em->floorY;
            em->rotX = 0;
            em->rotZ = 0;
            em->posY = em->floorY;
            em->stance = 0;
            Em_SetMotion(em, 0x29, 4, 0);
            if (m != NULL) {
                ang[0] = 0;
                ang[1] = em->rotY;
                ang[2] = 0;
                Vec_Set(off, 0.0f, 0.0f, 600.0f);
                Mtx_FromAnglesYXZ(ang, mtx);
                Vec_Rotate(out, off, mtx);
                m->posX = em->posX + out[0];
                m->posZ = em->posZ + out[2];
                if (m->stateGroup != 6 || m->state != 1) {
                    Em_SetStateRand(m, 6, 1);
                    m->rotY = (em->rotY - 0x4000) & 0xFFFF;
                }
            }
        }
        break;
    case 6:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x23, 0, 0);
            if (m != NULL) {
                Em_SetStateRand(m, 6, 2);
            }
        }
        if (m != NULL) {
            ang2[0] = 0;
            ang2[1] = em->rotY;
            ang2[2] = 0;
            Vec_Set(off2, 0.0f, 0.0f, 600.0f);
            Mtx_FromAnglesYXZ(ang2, mtx2);
            Vec_Rotate(out2, off2, mtx2);
            m->posX = em->posX + out2[0];
            m->posZ = em->posZ + out2[2];
            m->noPushTimer = 5;
        }
        break;
    case 7:
        if (Player_PassedFrameScaled(em, 60.0f, 0) || Player_PassedFrameScaled(em, 202.0f, 0)) {
            Effect0D_SpawnForEm(em, 0x1C, 6);
        }
        if (Player_PassedFrameScaled(em, 52.0f, 0) || Player_PassedFrameScaled(em, 208.0f, 0)) {
            Effect0D_SpawnForEm(em, 0x16, 6);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x4F, 0xA, 0);
            if (m != NULL) {
                Em_SetStateRand(m, 6, 3);
            }
        }
        break;
    case 8:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x3B, 0, 0);
        }
        break;
    case 9:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x36, 0, 0);
            if (m != NULL) {
                m->motionSpeed = 0.0f;
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

void EmRathalos_State6_04_Captured(Em *em)
{
    f32 pos[3];

    Em_SetCaptured(em, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->stateVar0 = 0;
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
                EmRathalos_ChangeState(em, 5, 3, 4);
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
        if (++em->stateVar0 % 135 == 0) {
            Em_PlaySeAtBone(em, 0x57, 0x23);
        }
    }
}

void EmRathalos_State6_05_Captured(Em *em)
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
                EmRathalos_ChangeState(em, 5, 3, 4);
                break;
            }
        } else {
            em->alpha = (f32) timer / 150.0f;
        }
        break;
    }
}

void EmRathalos_State6_06(Em *em)
{
    f32 d;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 0;
        em->fieldCollide = 0;
        em->posX = 14350.0f;
        em->posY = 0.0f;
        em->posZ = 9830.0f;
        em->goal[0] = 11000.0f;
        em->goal[1] = 0.0f;
        em->goal[2] = 10870.0f;
        em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        GmEm_ResetSize(em);
        Em_SetMotion(em, 1, 0, 0);
        em->timer = 120;
        em->stateVar0 = 0;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 3, 0, 0);
            em->timer = 900;
        }
        break;
    case 2:
        d = Vec_DistanceXZ(&em->posX, em->goal);
        em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        if (--em->timer <= 0 || d <= 150.0f) {
            em->step++;
            em->timer = 100;
            Em_SetMotion(em, 0x51, 0, 0);
            em->timer = 90;
            em->posX = 8551.0f;
            em->posY = 0.0f;
            em->posZ = 9386.0f;
            em->rotY = 0xB000;
        } else if (Player_PassedFrameScaled(em, 32.0f, 0U) != 0) {
            CamShake_ReqC(em, 2);
        } else if (Player_PassedFrameScaled(em, 88.0f, 0U) != 0) {
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
            Em_SetMotion(em, 0x11, 0, 0);
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
        d = Vec_DistanceXZ(&em->posX, em->goal);
        em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        if (--em->timer <= 0 || d <= 300.0f) {
            em->step++;
            Em_SetMotion(em, 0x5B, 0, 0);
        }
        break;
    case 7:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x33, 0, 0);
        }
        break;
    case 8:
        if (Save_TestEventFlag(0x9C) == 1) {
            em->step++;
            em->senseDelay = 0;
            em->noHitTimer = 0;
            em->fieldCollide = 1;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmRathalos_State8_00_CarveStart(Em *em, EmWork *work)
{
    EmRathalosVec v;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        if (Versus_DefeatCamera(em) == 1) {
            em->step++;
            Em_SetMotion(em, 0x1A, 0, 0);
            Em_SetLayerMotion(em, 0x3E9, 0, 0, 0);
            Em_SetLayerMotion(em, 0x3E9, 0, 0, 2);
            em->timer = 30;
        }
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 10.0f, 0) != 0) {
            v = EmRathalos_CarveThrowPt;
            StageObj_K25_Create(&gPlayers[(u8) em->side - 1], em, 0, 0, &v.x);
        }
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x4F, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x3B, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 30;
            Em_SetMotion(em, 0x33, 0, 0);
        }
        break;
    case 4:
        if (--em->timer <= 0) {
            Versus_RouteEmByTeam(em);
        }
        break;
    }
}

void EmRathalos_State8_01(Em *em, EmWork *work)
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
            Em_SetMotion(em, 1, 0, 0);
            EmRathalos_ChangeState(em, 8, 0x98, 1);
        }
        break;
    }
}

void EmRathalos_State8_02(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x36, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 1, 0, 0);
            EmRathalos_ChangeState(em, 8, 0x98, 1);
        }
        break;
    }
}

void EmRathalos_State8_03(Em *em, EmWork *work)
{
    s32 done = 0;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    if (em->type == 1 || em->type == 0x2A || em->type == 0x25) {
        switch (em->step) {
        case 0:
            em->step++;
            em->stance = 0;
            em->lookAtTarget = 0;
            Em_SetMotion(em, 0x5C, 0, 0);
            EmRathalos_StartCurves(em, 0xA);
            break;
        case 1:
            if (EmMotion_ReachedFrame(em, 0, 94.0f) != 0) {
                em->stance = 2;
                done = EmRathalos_UpdateCurves(em);
            }
            if (done != 0 && em->posY <= 372.0f + em->floorY) {
                em->step++;
                Em_SetMotion(em, 0x5D, 0, 0);
                EmRathalos_StartCurves(em, 0xB);
            }
            break;
        case 2:
            EmRathalos_UpdateCurves(em);
            if (em->posY <= em->floorY) {
                em->step++;
                Em_SetMotion(em, 0x5E, 0, 0);
                em->stance = 0;
                em->posY = em->floorY;
                EmRathalos_SpawnK14Type0B(em);
                CamShake_ReqAIf(em, 2);
            }
            break;
        case 3:
            if (em->motion[0].playing == 0) {
                em->step++;
                Em_SetMotion(em, 1, 0, 0);
                EmRathalos_ChangeState(em, 8, 0x98, 1);
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
            em->stance = 0;
            em->lookAtTarget = 0;
            Em_SetMotion(em, 0x2F, 0, 0);
            break;
        case 1:
            if (em->motion[1].playing == 0) {
                em->step++;
                Em_SetMotion(em, 1, 0, 0);
                EmRathalos_ChangeState(em, 8, 0x98, 1);
            }
            break;
        }
    }
}

void EmRathalos_State8_98(Em *em, EmWork *work)
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
            EmRathalos_ChangeState(em, 8, 0x99, 1);
        }
        break;
    }
}

void EmRathalos_State8_99(Em *em, EmWork *work)
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
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmRathalos_State5_00_Hunted(Em *em, EmWork *work)
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
                EmRathalos_ChangeState(em, 5, 3, 4);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    case 99:
        if (em->motion[0].playing == 0) {
            em->stance = 0;
            EmRathalos_EndState(em, 0, 0);
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
        }
        break;
    }
}

void EmRathalos_State5_01_Hunted(Em *em, EmWork *work)
{
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x61, 0, 0);
        em->lookAtTarget = 0;
        em->stance = 3;
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
                EmRathalos_ChangeState(em, 5, 3, 4);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    case 99:
        if (em->motion[0].playing == 0) {
            em->stance = 0;
            EmRathalos_EndState(em, 0, 0);
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
        }
        break;
    }
}

void EmRathalos_State5_02_HuntedFalling(Em *em, EmWork *work)
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
                EmRathalos_ChangeState(em, 5, 3, 4);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    case 99:
        if (em->motion[0].playing == 0) {
            em->stance = 0;
            EmRathalos_EndState(em, 0, 0);
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
        }
        break;
    }
}

void EmRathalos_State5_03_Appear(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        if (QuestEm_CanAppear(em) == 1) {
            em->posY = gAreaGroundY[em->area];
            em->stance = 2;
            EmRathalos_ChangeState(em, 2, 1, 0);
            break;
        }
        em->taskPhase++;
    case 1:
        break;
    }
}

void EmRathalos_RunGroup0(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmRathalos_State0_00(em);
        break;
    case 1:
        EmRathalos_State0_01(em);
        break;
    case 2:
        EmRathalos_State0_02(em);
        break;
    case 3:
        EmRathalos_State0_03(em);
        break;
    case 4:
        EmRathalos_State0_04(em);
        break;
    case 5:
        EmRathalos_State0_05(em);
        break;
    case 6:
        EmRathalos_State0_06(em);
        break;
    case 7:
        EmRathalos_State0_07(em);
        break;
    case 8:
        EmRathalos_State0_08(em);
        break;
    case 9:
        EmRathalos_State0_09(em);
        break;
    case 0xA:
        EmRathalos_State0_0A(em);
        break;
    case 0xB:
        EmRathalos_State0_0B(em);
        break;
    case 0xC:
        EmRathalos_State0_0C(em);
        break;
    case 0xD:
        EmRathalos_State0_0D(em);
        break;
    case 0xE:
        EmRathalos_State0_0E(em);
        break;
    case 0xF:
        EmRathalos_State0_0F(em);
        break;
    case 0x10:
        EmRathalos_State0_10(em);
        break;
    case 0x11:
        EmRathalos_State0_11(em);
        break;
    case 0x12:
        EmRathalos_State0_12(em, work);
        break;
    case 0x13:
        EmRathalos_State0_13(em);
        break;
    case 0x14:
        EmRathalos_State0_14(em);
        break;
    case 0x15:
        EmRathalos_State0_15(em, work);
        break;
    case 0x16:
        EmRathalos_State0_16(em);
        break;
    case 0x17:
        EmRathalos_State0_17(em, work);
        break;
    case 0x18:
        EmRathalos_State0_18(em);
        break;
    case 0x19:
        EmRathalos_State0_19(em);
        break;
    case 0x1A:
        EmRathalos_State0_1A(em);
        break;
    case 0x1B:
        EmRathalos_State0_1B(em);
        break;
    case 0x1C:
        EmRathalos_State0_1C(em);
        break;
    case 0x1D:
        EmRathalos_State0_1D(em, work);
        break;
    case 0x1F:
        EmRathalos_State0_1F(em, work);
        break;
    case 0x21:
        EmRathalos_State0_21(em, work, 0);
        break;
    case 0x22:
        EmRathalos_State0_21(em, work, 1);
        break;
    case 0x23:
        EmRathalos_State0_21(em, work, 2);
        break;
    case 0x28:
        EmRathalos_State0_28(em);
        break;
    }
}

void EmRathalos_RunGroup1(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmRathalos_State1_00(em, work);
        break;
    case 1:
        EmRathalos_State1_01(em, work);
        break;
    case 2:
        EmRathalos_State1_02(em, work);
        break;
    case 3:
        EmRathalos_State1_03(em, work);
        break;
    case 4:
        EmRathalos_State1_04(em, work);
        break;
    case 5:
        EmRathalos_State1_05(em, work);
        break;
    case 6:
        EmRathalos_State1_06(em, work);
        break;
    case 7:
        EmRathalos_State1_07(em, work);
        break;
    case 8:
        EmRathalos_State1_04(em, work);
        break;
    case 9:
        EmRathalos_State1_07(em, work);
        break;
    case 10:
        EmRathalos_State1_07(em, work);
        
    default:
        break;
    }
}

void EmRathalos_RunGroup2(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmRathalos_State2_00(em, work);
        break;
    case 1:
        EmRathalos_State2_01_FlyInLand(em, work);
        break;
    case 2:
        EmRathalos_State2_02(em, work);
        break;
    case 3:
        EmRathalos_State2_03(em, work);
        break;
    case 4:
        EmRathalos_State2_04(em, work);
        break;
    case 5:
        EmRathalos_State2_05(em, work);
        break;
    case 6:
        EmRathalos_State2_06(em, work);
        break;
    case 7:
        EmRathalos_State2_07(em, work);
        break;
    case 8:
        EmRathalos_State2_08(em, work);
        break;
    case 9:
        EmRathalos_State2_09(em, work);
        break;
    case 10:
        EmRathalos_State2_0A(em, work);
        break;
    case 11:
        EmRathalos_State2_0B(em, work);
        break;
    case 12:
        EmRathalos_State2_0C(em);
        break;
    case 13:
        EmRathalos_State2_0D(em, work);
        break;
    case 14:
        EmRathalos_State2_0E(em, work);
        break;
    case 15:
        EmRathalos_State2_0F(em, work);
        break;
    case 16:
        EmRathalos_State2_10(em, work);
        break;
    case 17:
        EmRathalos_State2_11(em, work);
        break;
    case 18:
        EmRathalos_State2_12(em, work);
        break;
    case 19:
        EmRathalos_State2_13(em, work);
        break;
    case 20:
        EmRathalos_State2_14(em);
        break;
    case 21:
        EmRathalos_State2_15(em, work);
        break;
    case 22:
        EmRathalos_State2_16(em, work);
        break;
    case 23:
        EmRathalos_State2_17(em, work);
        break;
    case 24:
        EmRathalos_State2_18(em, work);
        
    default:
        break;
    }
}

void EmRathalos_RunGroup3(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmRathalos_State3_00(em);
        break;
    case 2:
        EmRathalos_State3_02(em, work);
        break;
    case 3:
        EmRathalos_State3_03(em);
        break;
    case 4:
        EmRathalos_State3_04(em, work);
        break;
    case 5:
        EmRathalos_State3_05(em);
        break;
    case 6:
        EmRathalos_State3_06(em);
        break;
    case 7:
        EmRathalos_State3_07(em);
        break;
    case 8:
        EmRathalos_State3_08_SwoopThenLeave(em, work);
        break;
    case 9:
        EmRathalos_State3_09(em, work);
        break;
    case 0xA:
        EmRathalos_State3_0A(em, work);
        break;
    case 0xB:
        EmRathalos_State3_0B(em, work);
        break;
    case 0xC:
        EmRathalos_State3_0C(em, work);
        break;
    case 0xD:
        EmRathalos_State3_0D(em);
        break;
    case 0xE:
        EmRathalos_State3_0E(em);
        break;
    case 0xF:
        EmRathalos_State3_0F(em);
        break;
    case 0x10:
        EmRathalos_State3_10(em, work, 0);
        break;
    case 0x11:
        EmRathalos_State3_11(em, work);
        break;
    case 0x12:
        EmRathalos_State3_12(em);
        break;
    case 0x13:
        EmRathalos_State3_13(em);
        break;
    case 0x14:
        EmRathalos_State3_14(em);
        break;
    case 0x15:
        EmRathalos_State3_15_SwoopThenLand(em, work);
        break;
    case 0x16:
        EmRathalos_State3_16(em, work);
        break;
    case 0x17:
        EmRathalos_State3_17(em);
        break;
    case 0x18:
        EmRathalos_State3_18(em, work);
        break;
    case 0x19:
        EmRathalos_State3_19(em, work);
        break;
    case 0x1A:
        EmRathalos_State3_1A(em);
        break;
    case 0x1B:
        EmRathalos_State3_1B(em);
        break;
    case 0x1C:
        EmRathalos_State3_1C(em, (s32) work);
        break;
    case 0x1D:
        EmRathalos_State3_1D(em, (s32) work);
        break;
    case 0x1E:
        EmRathalos_State3_1E(em);
        break;
    case 0x1F:
        EmRathalos_State3_10(em, work, 1);
        break;
    case 0x20:
        EmRathalos_State3_10(em, work, 2);
        break;
    }
}

void EmRathalos_RunGroup4(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmRathalos_State4_00(em, work);
        break;
    case 1:
        EmRathalos_State4_01(em, work);
        break;
    case 2:
        EmRathalos_State4_02(em, work);
        break;
    case 3:
        EmRathalos_State4_03(em, work);
        break;
    case 4:
        EmRathalos_State4_04(em, work);
        break;
    case 5:
        EmRathalos_State4_05(em, work);
        break;
    case 6:
        EmRathalos_State4_06(em, work);
        break;
    case 7:
        EmRathalos_State4_07(em, work);
        break;
    case 8:
        EmRathalos_State4_08(em, work);
        break;
    case 9:
        EmRathalos_State4_09(em, work);
        break;
    case 10:
        EmRathalos_State4_0A(em, work);
        break;
    case 11:
        EmRathalos_State4_0B(em, work);
        break;
    case 12:
        EmRathalos_State4_0C(em, work);
        break;
    case 13:
        EmRathalos_State4_0D(em, work);
        break;
    case 14:
        EmRathalos_State4_0E(em, work);
        break;
    case 15:
        EmRathalos_State4_0F(em, work);
        break;
    case 16:
        EmRathalos_State4_10(em, work);
        break;
    case 17:
        EmRathalos_State4_11(em, work);
        break;
    case 18:
        EmRathalos_State4_12(em, work);
        break;
    case 19:
        EmRathalos_State4_13(em, work);
        break;
    case 20:
        EmRathalos_State4_14(em, work);
        break;
    case 21:
        EmRathalos_State4_15(em, work);
        break;
    case 22:
        EmRathalos_State4_16(em, work);
        break;
    case 23:
        EmRathalos_State4_17(em, work);
        
    default:
        break;
    }
}

void EmRathalos_RunGroup5(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmRathalos_State5_00_Hunted(em, work);
        break;
    case 1:
        EmRathalos_State5_01_Hunted(em, work);
        break;
    case 2:
        EmRathalos_State5_02_HuntedFalling(em, work);
        break;
    case 3:
        EmRathalos_State5_03_Appear(em, work);
        break;
    }
}

void EmRathalos_RunGroup6(Em *em, EmWork *work)
{
    switch (em->state) {
    case 2:
        EmRathalos_State6_02_QuestIntro(em, work);
        break;
    case 4:
        EmRathalos_State6_04_Captured(em);
        break;
    case 5:
        EmRathalos_State6_05_Captured(em);
        break;
    case 6:
        EmRathalos_State6_06(em);
        break;
    }
}

void EmRathalos_RunGroup8(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0x0:
        EmRathalos_State8_00_CarveStart(em, work);
        break;
    case 0x1:
        EmRathalos_State8_01(em, work);
        break;
    case 0x2:
        EmRathalos_State8_02(em, work);
        break;
    case 0x3:
        EmRathalos_State8_03(em, work);
        break;
    case 0x98:
        EmRathalos_State8_98(em, work);
        break;
    case 0x99:
        EmRathalos_State8_99(em, work);
        break;
    }
}

void EmRathalos_Update(Em *em)
{
    u8 buf[4];
    EmFlyerWork *w = (EmFlyerWork *)&em->work;
    f32 ratio;

    Em_UpdateCombatTimer(em);
    Em_RaiseTrigger80Lax(em);
    if (em->type == 1 || em->type == 0x2A || em->type == 0x25) {
        ratio = 0.1f;
    } else {
        ratio = 0.3f;
    }
    if (em->noAlert != 1 && em->rage == 0) {
        switch (Area_BaseId(gScene.map)) {
        case 2:
            if (em->questKey == 1) {
                if (em->type == 1 || em->type == 0x2A || em->type == 0x25) {
                    Em_CheckLowHp(em, ratio);
                    Em_Decay7F4(em);
                } else {
                    Em_Decay7EC(em);
                }
                Em_Decay7F0Only(em);
            } else {
                Em_CheckLowHp(em, ratio);
                Em_Decay7EC(em);
                Em_Decay7F0(em);
                Em_Decay7F4(em);
            }
            break;
        case 6:
            Em_CheckLowHp(em, ratio);
            Em_Decay7EC(em);
            Em_Decay7F0(em);
            Em_Decay7F4(em);
            Em_CheckCarryRequest(em);
            break;
        case 3:
            Em_CheckCarryRequest(em);
        case 4:
            Em_CheckLowHp(em, ratio);
            Em_Decay7EC(em);
            Em_Decay7F0Only(em);
            break;
        }
    }
    if (w->fxMotion != 0) {
        w->fxMotion -= 1;
    }
    switch (Em_UpdateDamage(em, buf) & 0xFF) {
    case 0:
    case 9:
    case 14:
    case 15:
    case 16:
        break;
    case 1:
    case 2:
        if (em->stance == 2) {
            EmRathalos_ChangeState(em, 5, 2, 3);
        } else if ((s8) em->holdTimer != 0) {
            EmRathalos_ChangeState(em, 5, 1, 3);
        } else {
            EmRathalos_ChangeState(em, 5, 0, 3);
        }
        break;
    case 3:
    case 4:
        if ((s8) em->holdTimer == 0) {
            Em_SetGoal12Dist(em, buf);
            Em_SetGoal12(em);
            EmRathalos_ChangeState(em, 4, 0xC, 3);
        }
        break;
    case 5:
        if (em->stateGroup != 4 || em->state != 8) {
            EmRathalos_ChangeState(em, 4, 8, 3);
        }
        break;
    case 6:
        if ((s8) em->holdTimer != 0) {
            Em_SetStatus4Timer(em);
            EmRathalos_ChangeState(em, 4, 0xE, 3);
        } else if ((em->stateGroup != 4 || em->state != 0xB) && (em->stateGroup != 4 || em->state != 8)) {
            Em_SetStatus4Timer(em);
            EmRathalos_ChangeState(em, 4, 0xB, 3);
        }
        break;
    case 7:
        if (Em_IsHeld(em) == 1) {
            Em_SetStatus1Timer(em);
            if ((s8) em->holdTimer != 0) {
                EmRathalos_ChangeState(em, 6, 4, 3);
                break;
            }
            if ((s8) em->holdTimer2 != 0) {
                EmRathalos_ChangeState(em, 6, 5, 3);
                break;
            }
        }
        if ((em->stateGroup != 0 || em->state != 0x1B) && (em->stateGroup != 4 || em->state != 8)) {
            Em_SetStatus1Timer(em);
            EmRathalos_ChangeState(em, 0, 0x1B, 3);
        }
        break;
    case 8:
        if ((s8) em->holdTimer != 0) {
            Em_SetStatus2Timer(em);
            EmRathalos_ChangeState(em, 0, 0x1D, 3);
        } else if ((s8) em->holdTimer2 != 0) {
            Em_SetStatus2Timer(em);
            EmRathalos_ChangeState(em, 0, 0x14, 3);
        } else if ((em->stateGroup != 0 || em->state != 0x14) && (em->stateGroup != 4 || em->state != 8)) {
            Em_SetStatus2Timer(em);
            EmRathalos_ChangeState(em, 0, 0x14, 3);
        }
        break;
    case 10:
        switch (em->state) {
        case 0x12:
            EmRathalos_ChangeState(em, 0, 0x17, 3);
            em->scriptPending = 0;
            Em_CheckRageByHp(em, em->rageThreshold);
            break;
        case 0x14:
            EmRathalos_ChangeState(em, 0, 0x18, 3);
            em->scriptPending = 0;
            break;
        case 0x1B:
            EmRathalos_ChangeState(em, 0, 0x1C, 3);
            em->scriptPending = 0;
            break;
        case 0x1D:
            EmRathalos_ChangeState(em, 4, 0x12, 3);
            em->scriptPending = 0;
            break;
        case 0x1F:
            EmRathalos_ChangeState(em, 4, 0x13, 3);
            em->scriptPending = 0;
            break;
        }
        break;
    case 11:
        EmRathalos_ChangeState(em, 4, 4, 3);
        break;
    case 12:
        if (em->stance == 2) {
            EmRathalos_ChangeState(em, 4, 8, 3);
        } else {
            switch (em->flinchPart) {
            case 0:
            case 7:
                EmRathalos_ChangeState(em, 4, 0, 3);
                break;
            case 5:
            case 6:
                EmRathalos_ChangeState(em, 4, 2, 3);
                break;
            case 1:
            case 2:
                EmRathalos_ChangeState(em, 4, 3, 3);
                break;
            case 3:
            case 4:
            default:
                if (em->parts[em->flinchPart].state >= 2) {
                    if (em->flinchPart != 3) {
                        EmRathalos_ChangeState(em, 4, 5, 3);
                    } else {
                        EmRathalos_ChangeState(em, 4, 0x10, 3);
                    }
                } else {
                    EmRathalos_ChangeState(em, 4, 1, 3);
                }
                break;
            }
        }
        break;
    case 13:
        if (em->stance != 2) {
            EmRathalos_ChangeState(em, 4, 0, 3);
            em->scriptPending = 0;
        }
        break;
    case 17:
        if ((s8) em->holdTimer != 0) {
            Em_SetStatus5Timer(em);
            EmRathalos_ChangeState(em, 4, 0x15, 3);
        } else if ((em->stateGroup != 4 || em->state != 0x14) && (em->stateGroup != 4 || em->state != 8)) {
            Em_SetStatus5Timer(em);
            EmRathalos_ChangeState(em, 4, 0x14, 3);
        }
        break;
    case 18:
        Em_ResetKindTimer(em);
        EmRathalos_ChangeState(em, 4, 0x17, 3);
        break;
    }
    switch (gQuest.stage) {
    case 0x434:
    case 0x436:
        if ((em->type == 0xB || em->type == 0x29 || em->type == 0x31) && Area_DayId(em->area) == 0x21 && Save_TestEventFlag(0xAB) == 0) {
            if (Area_DayId(gScene.area) == 0x21 && gScene.eventRunning == 1 && em->stateGroup != 6) {
                EmRathalos_ChangeState(em, 6, 2, 1);
            }
            break;
        }
        goto normal;
    case 0x3F6:
    case 0x3F8:
        if ((em->type == 1 || em->type == 0x2A || em->type == 0x25) && Area_DayId(em->area) == 0x71 && Save_TestEventFlag(0x9C) == 0) {
            if (Area_DayId(gScene.area) == 0x71 && gScene.eventRunning == 1 && em->stateGroup != 6) {
                EmHermitaur_ChangeState(em, 6, 6, 1);
            }
            break;
        }
    default:
    normal:
        Em_UpdatePendingScript(em);
        break;
    }
    Em_ClearFlag6AE(em);
    EmRathalos_RunState(em, (s32) w);
    if (em->stateRequest != 0) {
        EmRathalos_RunState(em, (s32) w);
        em->stateRequest = 0;
    }
    if (em->stateGroup == 2 && (s8) w->flyGuard == 1) {
        em->guardTimer = 2;
    }
}

void EmRathalos_RunState(Em *em, s32 work)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->stateGroup) {
    case 0:
        EmRathalos_RunGroup0(em, (EmWork *) work);
        break;
    case 1:
        EmRathalos_RunGroup1(em, (EmWork *) work);
        break;
    case 2:
        EmRathalos_RunGroup2(em, (EmWork *) work);
        break;
    case 3:
        EmRathalos_RunGroup3(em, (EmWork *) work);
        break;
    case 4:
        EmRathalos_RunGroup4(em, (EmWork *) work);
        break;
    case 5:
        EmRathalos_RunGroup5(em, (EmWork *) work);
        break;
    case 6:
        EmRathalos_RunGroup6(em, (EmWork *) work);
        break;
    case 7:
        EmRathalos_RunGroup6(em, (EmWork *) work);
        break;
    case 8:
        EmRathalos_RunGroup8(em, (EmWork *) work);
        
    default:
        break;
    }
}

void EmRathalos_RequestOnFrame(void *em, s32 frame, s32 i)
{
    if (Player_PassedFrameScaled((Em *) em, (f32) frame, 0U) != 0) {
        CamShake_ReqAIf(em, i);
    }
}

static void EmRathalos_MotionEffectsDefault(Em *em)
{
}

void EmRathalos_MotionEffects(Em *em, EmRathalosFxWork *work)
{
    f32 v1[3];

    if (em->motionId[0] != work->motion) {
        work->motion = em->motionId[0];
    }
    EmRathalos_MotionSounds(em, (EmWork *) work);
    switch (work->motion) {
    case 0x3EB:
        EmRathalos_RequestOnFrame(em, 0x34, 1);
        EmRathalos_RequestOnFrame(em, 0x74, 1);
        if (Player_PassedFrameScaled(em, 2.0f, 0) != 0) {
            Eff01_Create(em, 2);
        }
        if (Player_PassedFrameScaled(em, 50.0f, 0) != 0) {
            Eff01_Create(em, 3);
        }
        if (Player_PassedFrameScaled(em, 120.0f, 0) != 0) {
            Eff01_Create(em, 4);
        }
        break;
    case 0x3EC:
        if (Player_PassedFrameScaled(em, 12.0f, 0) != 0) {
            StageObj_K14_Create(em, 2, 0, 1.0f);
        }
        if (Player_PassedFrameScaled(em, 52.0f, 0) != 0) {
            StageObj_K14_Create(em, 3, 0, 1.0f);
        }
        if (Player_PassedFrameScaled(em, 92.0f, 0) != 0) {
            StageObj_K14_Create(em, 2, 0, 1.0f);
        }
        break;
    case 0x3ED:
        EmRathalos_RequestOnFrame(em, 0x3C, 1);
        if (Player_PassedFrameScaled(em, 2.0f, 0) != 0) {
            Eff01_Create(em, 5);
        }
        if (Player_PassedFrameScaled(em, 30.0f, 0) != 0) {
            Eff01_Create(em, 6);
        }
        if ((em->rage != 0) && (EmMotion_ReachedFrame(em, 0, 2.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 78.0f) == 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create(em, 0x1A, 1, 1.0f);
        }
        break;
    case 0x3EE:
        EmRathalos_RequestOnFrame(em, 0x3C, 1);
        if (Player_PassedFrameScaled(em, 30.0f, 0) != 0) {
            Eff01_Create(em, 7);
        }
        if (Player_PassedFrameScaled(em, 2.0f, 0) != 0) {
            Eff01_Create(em, 8);
        }
        if ((em->rage != 0) && (EmMotion_ReachedFrame(em, 0, 2.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 78.0f) == 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create(em, 0x1A, 0, 1.0f);
        }
        break;
    case 0x3EF:
        if (Player_PassedFrameScaled(em, 18.0f, 0) != 0) {
            Eff01_Create(em, 9);
        }
        if (Player_PassedFrameScaled(em, 2.0f, 0) != 0) {
            Eff01_Create(em, 0xA);
        }
        if (em->rage != 0) {
            if ((EmMotion_ReachedFrame(em, 0, 2.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 18.0f) == 0) && !(gScene.frame & 3)) {
                StageObj_K14_Create(em, 0x1A, 1, 1.0f);
            }
            if ((EmMotion_ReachedFrame(em, 0, 20.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 70.0f) == 0) && !(gScene.frame & 3)) {
                StageObj_K14_Create(em, 0x1A, 0, 1.0f);
                break;
            }
        }
        break;
    case 0x3F0:
        if (Player_PassedFrameScaled(em, 2.0f, 0) != 0) {
            Eff01_Create(em, 0xB);
        }
        if (Player_PassedFrameScaled(em, 18.0f, 0) != 0) {
            Eff01_Create(em, 0xC);
        }
        if (em->rage != 0) {
            if ((EmMotion_ReachedFrame(em, 0, 2.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 18.0f) == 0) && !(gScene.frame & 3)) {
                StageObj_K14_Create(em, 0x1A, 0, 1.0f);
            }
            if ((EmMotion_ReachedFrame(em, 0, 20.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 70.0f) == 0) && !(gScene.frame & 3)) {
                StageObj_K14_Create(em, 0x1A, 1, 1.0f);
                break;
            }
        }
        break;
    case 0x3F2:
        if (Player_PassedFrameScaled(em, 42.0f, 0) != 0) {
            Eff01_Create(em, 0x19);
        }
        if (Player_PassedFrameScaled(em, 2.0f, 0) != 0) {
            Eff01_Create(em, 0x1A);
        }
        if (Player_PassedFrameScaled(em, 74.0f, 0) != 0) {
            Eff01_Create(em, 0x1A);
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
    case 0x3F5:
        if (((Player_PassedFrameScaled(em, 26.0f, 0) != 0) || (Player_PassedFrameScaled(em, 52.0f, 0) != 0)) && (em->posY <= (1000.0f + em->floorY))) {
            Effect0D_SpawnForEmScaled(em, 1, 7, 5.0f);
        }
        break;
    case 0x3F6:
    case 0x43D:
    case 0x43E:
    case 0x440:
    case 0x441:
        if ((em->rage != 0) && (((s32) gScene.frame % 19) == 0)) {
            StageObj_K14_Create(em, 0x19, ((u16) Rand_Next(1) & 1) + 2, 1.0f);
        }
        break;
    case 0x3F7:
        if (Player_PassedFrameScaled(em, 8.0f, 0) || Player_PassedFrameScaled(em, 86.0f, 0) || Player_PassedFrameScaled(em, 160.0f, 0)) {
            if (em->posY <= (1000.0f + em->floorY)) {
                StageObj_K14_Create(em, 1, 0, 1.0f);
            }
        }
        if (em->rage != 0) {
            if (EmMotion_ReachedFrame(em, 0, 70.0f) != 0) {
                if (Player_PassedFrameScaled(em, 84.0f, 0) != 0) {
                    goto block_95;
                }
                goto block_99;
            }
block_95:
            if (((EmMotion_ReachedFrame(em, 0, 146.0f) != 0) && (Player_PassedFrameScaled(em, 156.0f, 0) == 0)) || ((EmMotion_ReachedFrame(em, 0, 220.0f) != 0) && (Player_PassedFrameScaled(em, 234.0f, 0) == 0))) {
block_99:
                if (!(gScene.frame & 3)) {
                    StageObj_K14_Create(em, 0x1A, (u16) Rand_Next(1) & 1, 1.0f);
                    break;
                }
            }
        }
        break;
    case 0x3F9:
        if (Player_PassedFrameScaled(em, 2.0f, 0) != 0) {
            Eff01_Create(em, 0x18);
        }
        if ((em->rage != 0) && (((s32) gScene.frame % 10) == 0)) {
            StageObj_K14_Create(em, 0x19, ((u16) Rand_Next(1) & 1) + 2, 1.0f);
        }
        if ((Player_PassedFrameScaled(em, 12.0f, 0) != 0) || (Player_PassedFrameScaled(em, 78.0f, 0) != 0)) {
            StageObj_K14_Create(em, 2, 0, 1.0f);
        }
        if (Player_PassedFrameScaled(em, 42.0f, 0) != 0) {
            StageObj_K14_Create(em, 3, 0, 1.0f);
        }
        break;
    case 0x3FA:
        if (Player_PassedFrameScaled(em, 42.0f, 0) || Player_PassedFrameScaled(em, 102.0f, 0) || Player_PassedFrameScaled(em, 156.0f, 0)) {
            if (em->posY <= (1000.0f + em->floorY)) {
                StageObj_K14_Create(em, 1, 0, 1.0f);
            }
        }
        if ((em->rage != 0) && (Player_PassedFrameScaled(em, 62.0f, 0) != 0)) {
            StageObj_K14_Create(em, 0x19, 4, 1.0f);
            StageObj_K14_Create(em, 0x19, 5, 1.0f);
        }
        break;
    case 0x3FB:
        if (Player_PassedFrameScaled(em, 2.0f, 0) != 0) {
            Eff01_Create(em, 0xD);
        }
        if (Player_PassedFrameScaled(em, 16.0f, 0) != 0) {
            EmRathalos_SpawnK14Type0B(em);
        }
        if ((em->rage != 0) && (EmMotion_ReachedFrame(em, 0, 18.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 44.0f) == 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create(em, 0x19, ((u16) Rand_Next(1) & 1) + 2, 1.0f);
        }
        break;
    case 0x407:
    case 0x40A:
        v1[1] = 10.0f;
        v1[2] = 140.0f;
        v1[0] = 0.0f;
        Em_SpawnEffect06Periodic(em, 0x22, v1, 1.6f);
        break;
    case 0x40C:
        if (Player_PassedFrameScaled(em, 50.0f, 0) != 0) {
            Eff01_Create(em, 1);
        }
        if (Player_PassedFrameScaled(em, 46.0f, 0) != 0) {
            Eff01_Create(em, 0x29);
        }
        break;
    case 0x40E:
        if (Player_PassedFrameScaled(em, 148.0f, 0) != 0) {
            Eff01_Create(em, 0xF);
        }
        if (Player_PassedFrameScaled(em, 156.0f, 0) != 0) {
            StageObj_K14_Create(em, 0, 0, 1.0f);
        }
        break;
    case 0x410:
        if (Player_PassedFrameScaled(em, 60.0f, 0) != 0) {
            Eff01_Create(em, 0x22);
        }
        if ((em->rage != 0) && (EmMotion_ReachedFrame(em, 0, 142.0f) != 0) && (Player_PassedFrameScaled(em, 236.0f, 0) == 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create(em, 0x1A, (u16) Rand_Next(1) & 1, 1.0f);
        }
        break;
    case 0x411:
        if (Player_PassedFrameScaled(em, 2.0f, 0) != 0) {
            Eff01_Create(em, 0x23);
        }
        if ((EmMotion_ReachedFrame(em, 0, 20.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 50.0f) == 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create(em, 2, 0, 1.0f);
        }
        if (Player_PassedFrameScaled(em, 102.0f, 0) != 0) {
            StageObj_K14_Create(em, 2, 0, 1.0f);
        }
        if ((EmMotion_ReachedFrame(em, 0, 22.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 38.0f) == 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create(em, 3, 0, 1.0f);
        }
        if (Player_PassedFrameScaled(em, 62.0f, 0) != 0) {
            StageObj_K14_Create(em, 3, 0, 1.0f);
        }
        if ((em->rage != 0) && (EmMotion_ReachedFrame(em, 0, 46.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 134.0f) == 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create(em, 0x1A, (u16) Rand_Next(1) & 1, 1.0f);
        }
        break;
    case 0x412:
        EmRathalos_RequestOnFrame(em, 0x2C, 1);
        EmRathalos_RequestOnFrame(em, 0x49, 1);
        if (Player_PassedFrameScaled(em, 30.0f, 0) != 0) {
            Eff01_Create(em, 0x11);
        }
        if ((em->rage != 0) && (EmMotion_ReachedFrame(em, 0, 6.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 38.0f) == 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create(em, 0x1A, 0, 1.0f);
        }
        break;
    case 0x413:
        EmRathalos_RequestOnFrame(em, 0x2C, 1);
        EmRathalos_RequestOnFrame(em, 0x49, 1);
        if (Player_PassedFrameScaled(em, 30.0f, 0) != 0) {
            Eff01_Create(em, 0x11);
        }
        if ((em->rage != 0) && (EmMotion_ReachedFrame(em, 0, 6.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 38.0f) == 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create(em, 0x1A, 1, 1.0f);
        }
        break;
    case 0x415:
        if (Player_PassedFrameScaled(em, 22.0f, 0) != 0) {
            Eff01_Create(em, 0x14);
            Eff01_Create(em, 0x15);
        }
        if (Player_PassedFrameScaled(em, 20.0f, 0) || Player_PassedFrameScaled(em, 68.0f, 0) || Player_PassedFrameScaled(em, 114.0f, 0)) {
            if (em->posY <= (1000.0f + em->floorY)) {
                StageObj_K14_Create(em, 9, 0, 1.0f);
                StageObj_K14_Create(em, 0xE, 0, 1.0f);
            }
        }
        if ((em->rage != 0) && (EmMotion_ReachedFrame(em, 0, 18.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 110.0f) == 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create(em, 0x1A, (u16) Rand_Next(1) & 1, 1.0f);
        }
        break;
    case 0x419:
        if ((Player_PassedFrameScaled(em, 92.0f, 0) != 0) && (em->posY <= (1000.0f + em->floorY))) {
            StageObj_K14_Create(em, 1, 0, 1.0f);
        }
        if ((em->rage != 0) && (EmMotion_ReachedFrame(em, 0, 60.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 132.0f) == 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create(em, 0x1A, (u16) Rand_Next(1) & 1, 1.0f);
        }
        break;
    case 0x41B:
        if (Player_PassedFrameScaled(em, 2.0f, 0) != 0) {
            Eff01_Create(em, 0x1B);
        }
        if (Player_PassedFrameScaled(em, 168.0f, 0) != 0) {
            Eff01_Create(em, 0x1B);
        }
        if (Player_PassedFrameScaled(em, 56.0f, 0) != 0) {
            Eff01_Create(em, 0x1C);
        }
        if (Player_PassedFrameScaled(em, 54.0f, 0) != 0) {
            StageObj_K14_Create(em, 2, 2, 0.8f);
        }
        if (em->rage != 0) {
            if (EmMotion_InFrameRange(em, 0, 154.0f, 244.0f) || EmMotion_InFrameRange(em, 0, 254.0f, 330.0f)) {
                if (!(gScene.frame & 3)) {
                    StageObj_K14_Create(em, 0x1A, (u16) Rand_Next(1) & 1, 1.0f);
                    break;
                }
            }
        }
        break;
    case 0x41C:
        if (Player_PassedFrameScaled(em, 72.0f, 0) != 0) {
            StageObj_K14_Create(em, 4, 0, 1.0f);
        }
        if (Player_PassedFrameScaled(em, 80.0f, 0) != 0) {
            StageObj_K14_Create(em, 1, 0, 1.0f);
        }
        if (Player_PassedFrameScaled(em, 84.0f, 0) != 0) {
            StageObj_K14_Create(em, 4, 1, 1.0f);
        }
        break;
    case 0x41D:
        if (Player_PassedFrameScaled(em, 90.0f, 0) != 0) {
            Eff01_Create(em, 0x16);
        }
        if (Player_PassedFrameScaled(em, 108.0f, 0) != 0) {
            Eff01_Create(em, 0x17);
        }
        break;
    case 0x41E:
        if (Player_PassedFrameScaled(em, 102.0f, 0) != 0) {
            Eff01_Create(em, 0x12);
        }
        if (Player_PassedFrameScaled(em, 110.0f, 0) != 0) {
            Eff01_Create(em, 0x13);
            StageObj_K0F_CreateOnEm(em, 5, 4, 1.0f);
        }
        if (em->rage != 0) {
            if (EmMotion_ReachedFrame(em, 0, 20.0f) != 0) {
                if (EmMotion_ReachedFrame(em, 0, 36.0f) != 0) {
                    goto block_239;
                }
                goto block_247;
            }
block_239:
            if (((EmMotion_ReachedFrame(em, 0, 46.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 168.0f) == 0)) || ((EmMotion_ReachedFrame(em, 0, 172.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 192.0f) == 0)) || ((EmMotion_ReachedFrame(em, 0, 196.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 216.0f) == 0)) || ((EmMotion_ReachedFrame(em, 0, 230.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 238.0f) == 0))) {
block_247:
                if (!(gScene.frame & 3)) {
                    StageObj_K14_Create(em, 0x1A, (u16) Rand_Next(1) & 1, 1.0f);
                    break;
                }
            }
        }
        break;
    case 0x421:
        if (Player_PassedFrameScaled(em, 12.0f, 0) != 0) {
            StageObj_K14_Create(em, 4, 0, 1.0f);
        }
        if (Player_PassedFrameScaled(em, 30.0f, 0) != 0) {
            StageObj_K14_Create(em, 5, 0, 1.0f);
        }
        if (Player_PassedFrameScaled(em, 28.0f, 0) != 0) {
            Eff01_Create(em, 0x1D);
        }
        if (Player_PassedFrameScaled(em, 2.0f, 0) != 0) {
            Eff01_Create(em, 0x25);
        }
        if (Player_PassedFrameScaled(em, 36.0f, 0) != 0) {
            Eff01_Create(em, 0x28);
        }
        break;
    case 0x424:
        if (Player_PassedFrameScaled(em, 80.0f, 0) != 0) {
            StageObj_K14_Create(em, 1, 0x80, 0.5f);
        }
        if (Player_PassedFrameScaled(em, 44.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x1B, 7, 1.0f);
        }
        if (Player_PassedFrameScaled(em, 62.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x16, 7, 1.0f);
        }
        break;
    case 0x425:
        if (Player_PassedFrameScaled(em, 24.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x1A, 7, 1.0f);
        }
        break;
    case 0x429:
        if (Player_PassedFrameScaled(em, 20.0f, 0) != 0) {
            StageObj_K14_Create(em, 3, 0x80, 1.0f);
        }
        if ((Player_PassedFrameScaled(em, 128.0f, 0) != 0) || (Player_PassedFrameScaled(em, 164.0f, 0) != 0)) {
            StageObj_K14_Create(em, 3, 0x80, 0.6f);
        }
        if (Player_PassedFrameScaled(em, 60.0f, 0) != 0) {
            StageObj_K14_Create(em, 1, 0x80, 1.0f);
        }
        break;
    case 0x42C:
        if (Player_PassedFrameScaled(em, 268.0f, 0) != 0) {
            StageObj_K14_Create(em, 0, 0, 0.9f);
        }
        break;
    case 0x42D:
        EmRathalos_RequestOnFrame(em, 0x1A, 2);
        if (Player_PassedFrameScaled(em, 26.0f, 0) != 0) {
            StageObj_K14_Create(em, 0x12, 0x80, 1.0f);
        }
        break;
    case 0x431:
        EmRathalos_RequestOnFrame(em, 0x1A, 2);
        if ((EmMotion_ReachedFrame(em, 0, 28.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 52.0f) == 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create(em, 0xA, 0x80, 1.0f);
        }
        break;
    case 0x432:
        EmRathalos_RequestOnFrame(em, 0x1A, 2);
        if (Player_PassedFrameScaled(em, 26.0f, 0) != 0) {
            StageObj_K14_Create(em, 0x12, 0x81, 1.0f);
        }
        break;
    case 0x436:
        if (Player_PassedFrameScaled(em, 8.0f, 0) != 0) {
            StageObj_K14_Create(em, 0xB, 0, 1.0f);
        }
        break;
    case 0x437:
        if (Player_PassedFrameScaled(em, 48.0f, 0) || Player_PassedFrameScaled(em, 106.0f, 0) || Player_PassedFrameScaled(em, 174.0f, 0)) {
            Eff01_Create(em, 0x10);
        }
        break;
    case 0x43C:
        if (Player_PassedFrameScaled(em, 38.0f, 0) || Player_PassedFrameScaled(em, 60.0f, 0) || Player_PassedFrameScaled(em, 78.0f, 0)) {
            Effect0D_SpawnForEmScaled(em, 2, 7, 5.0f);
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
        }
        break;
    case 0x443:
        if (Player_PassedFrameScaled(em, 34.0f, 0) != 0) {
            StageObj_K14_Create(em, 3, 0, 1.0f);
        }
        if (Player_PassedFrameScaled(em, 70.0f, 0) != 0) {
            StageObj_K14_Create(em, 2, 0, 1.0f);
        }
        if ((EmMotion_ReachedFrame(em, 0, 96.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 120.0f) == 0) && (((s32) gScene.frame % 6) == 0)) {
            StageObj_K14_Create(em, 0x12, 0, 1.0f);
        }
        if ((em->rage != 0) && (((s32) gScene.frame % 19) == 0)) {
            StageObj_K14_Create(em, 0x19, ((u16) Rand_Next(1) & 1) + 2, 1.0f);
        }
        break;
    case 0x444:
        if (Player_PassedFrameScaled(em, 108.0f, 0) != 0) {
            Eff01_Create(em, 0x1E);
        }
        if (Player_PassedFrameScaled(em, 104.0f, 0) != 0) {
            StageObj_K14_Create(em, 1, 0, 1.0f);
        }
        if (Player_PassedFrameScaled(em, 200.0f, 0) != 0) {
            StageObj_K14_Create(em, 1, 0, 1.0f);
        }
        break;
    case 0x446:
        if (Player_PassedFrameScaled(em, 2.0f, 0) != 0) {
            Eff01_Create(em, 0x1F);
        }
        break;
    case 0x447:
        if (Player_PassedFrameScaled(em, 10.0f, 0) != 0) {
            StageObj_K14_Create(em, 0xB, 0x80, 1.0f);
        }
        if ((em->rage != 0) && (EmMotion_ReachedFrame(em, 0, 4.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 60.0f) == 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create(em, 0x1A, (u16) Rand_Next(1) & 1, 1.0f);
        }
        break;
    case 0x448:
        if (em->rage != 0) {
            if (EmMotion_ReachedFrame(em, 0, 12.0f) != 0) {
                if (EmMotion_ReachedFrame(em, 0, 24.0f) != 0) {
                    goto block_344;
                }
                goto block_346;
            }
block_344:
            if ((EmMotion_ReachedFrame(em, 0, 46.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 90.0f) == 0)) {
block_346:
                if (!(gScene.frame & 3)) {
                    StageObj_K14_Create(em, 0x1A, (u16) Rand_Next(1) & 1, 1.0f);
                    break;
                }
            }
        }
        break;
    default:
        EmRathalos_MotionEffectsDefault(em);
        break;
    case 0x3E9:
    case 0x3F1:
    case 0x3F4:
    case 0x3F8:
    case 0x405:
    case 0x406:
    case 0x40B:
    case 0x40D:
    case 0x40F:
    case 0x414:
    case 0x416:
    case 0x417:
    case 0x418:
    case 0x41A:
    case 0x423:
    case 0x426:
    case 0x427:
    case 0x428:
    case 0x42A:
    case 0x42B:
    case 0x42E:
    case 0x42F:
    case 0x430:
    case 0x433:
    case 0x434:
    case 0x435:
    case 0x438:
    case 0x439:
    case 0x43A:
    case 0x43B:
    case 0x43F:
    case 0x445:
    case 0x449:
    case 0x44A:
        break;
    }
}

void EmRathalos_UpdateMotionEffects(Em *em)
{
    EmFlyerWork *w = (EmFlyerWork *)&em->work;

    switch (w->fxStep) {
    case 0:
        w->fxStep++;
        break;
    case 1:
        EmRathalos_MotionEffects(em, (EmRathalosFxWork *) w);
        break;
    }
}

void EmRathalos_SpawnK14Type0B(Em *em)
{
    StageObj_K14_Create((Em *) em, 0xB, 0, 1.0f);
}

void EmRathalos_SpawnK14Type0AEvery5(Em *em)
{
    if (((s32) gScene.frame % 5) == 0) {
        StageObj_K14_Create((Em *) em, 0xA, 0, 1.0f);
    }
}

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
    if (em->motionId[0] == 0x415) {
        return 1;
    }
    if (dist <= (4.5f + (30.0f * em->velZ))) {
        Em_SetMotion(em, 0x2D, 0xA, 0);
        return 1;
    }
    return 0;
}

s32 EmRathalos_ReachedPoint(Em *em)
{
    f32 dist;

    dist = Vec_Distance((f32 *) &em->posX, (f32 *) em->goal);
    if (em->motionId[0] == 0x415) {
        return 1;
    }
    if (dist <= (2.4199998f + (22.0f * em->velZ))) {
        Em_SetMotion(em, 0x2D, 0xA, 0);
        return 1;
    }
    return 0;
}

void EmRathalos_EndSwoop(Em *em, s32 unused)
{
    em->step = 99;
    EmRathalos_EndState2(em, 1);
}

void EmRathalos_TableNop(void)
{
}

s32 EmRathalos_AllowHit8(Em *em)
{
    u8 stateGroup;

    stateGroup = em->stateGroup;
    if ((stateGroup != 4) || (em->state != 0xC)) {
        if (stateGroup == 4) {
            if (em->state != 0xD) {
                goto block_5;
            }
            goto block_21;
        }
block_5:
        if (stateGroup == 4) {
            if (em->state != 0xE) {
                goto block_8;
            }
            goto block_21;
        }
block_8:
        if (stateGroup == 4) {
            if (em->state != 0x11) {
                goto block_11;
            }
            goto block_21;
        }
block_11:
        if (stateGroup == 4) {
            if (em->state != 0x12) {
                goto block_14;
            }
            goto block_21;
        }
block_14:
        if (stateGroup == 4) {
            if (em->state != 0x13) {
                goto block_17;
            }
            goto block_21;
        }
block_17:
        if ((em->stance != 2) && ((s8) em->hit8Count == 0)) {
            return 1;
        }
        goto block_21;
    }
block_21:
    return 0;
}

void EmRathalos_SetGroup0State(Em *em, u16 state, u8 mode)
{
    em->motionSpeed = 1.0f;
    switch (state) {
    case 1:
        if (em->combat == 1) {
            state = 0x11;
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
    case 23:
        Em_Refill7F4(em);
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
        Em_EndStatus4(em);
        break;
    case 34:
        Em_EndStatus5(em);
        break;
    case 35:
        em->holdTimer2 = 0;
        break;
    case 40:
        Em_StartGoalAction(em, 0x2710);
        break;
    }
    em->stance = 0;
    Em_SetStateSync(em, 0, state, mode);
}

void EmRathalos_SetGroup1State(Em *em, u16 state, u8 mode)
{
    f32 f;
    f32 dist;

    em->stance = 0;
    em->motionSpeed = 1.0f;
    switch (state) {
    case 0:
    case 2:
        if (em->type == 1 || em->type == 0x2A || em->type == 0x25) {
            f = 0.1f;
        } else {
            f = 0.3f;
        }
        if (em->hp < (s16) (em->hpMax * f)) {
            state = 6;
            goto state6;
        }
        if (state == 2 && em->combat == 1) {
            state = 4;
            goto state4;
        }
        if (state == 2) {
            state = 0;
        }
        Em_SetSpeedScaled(em, 0, 1000.0f, 500.0f);
        break;
    case 1:
        em->scriptPending = 1;
        return;
    case 3:
        if (em->combat == 1) {
            state = 5;
        }
        break;
    case 8:
        em->motionSpeed = 0.7f;
    case 9:
        Em_StartRage(em);
        break;
    case 10:
        Em_StartPendingRage(em);
        break;
    case 4:
    state4:
        Em_SetSpeedScaled(em, 2, 1000.0f, 500.0f);
        break;
    case 6:
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
    case 5:
    case 7:
        break;
    }
    Em_SetStateSync(em, 1, state, mode);
}

void EmRathalos_SetGroup2State(Em *em, u16 state, u8 mode)
{
    EmFlyerWork *w = (EmFlyerWork *)&em->work;

    em->motionSpeed = 1.0f;
    switch (state) {
    case 1:
        if (em->remote == 0 && !(em->velZ < 50.0f)) {
            state = 18;
        }
        break;
    case 6:
    case 17:
        w->legsLeft = 4;
        if (em->remote == 0) {
            em->goalNo = em->stateRand & 3;
            em->goalKind = 2;
            em->goalArg = 2;
            Em_ComputeGoal(em, em->goal);
            em->timer = 1.5f * Vec_DistanceXZ(&em->posX, em->goal) / 50.0f;
        }
        break;
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
            EmRathalos_ChangeState(em, 2, 1, 1);
            return;
        }
        break;
    case 9:
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
    case 19:
        w->legsLeft = 1;
        if (em->remote == 0) {
            em->timer = 1.5f * Vec_DistanceXZ(&em->posX, em->goal) / 50.0f;
        }
        break;
    case 24:
        if (em->velZ <= 0.0f) {
            em->velZ = 20.0f;
        }
        if (em->remote == 0) {
            em->timer = Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) / em->velZ;
        }
        break;
    case 0:
    case 2:
    case 3:
    case 4:
    case 5:
    case 10:
    case 12:
    case 14:
    case 15:
    case 16:
    case 18:
    case 20:
    case 22:
    case 23:
        break;
    }
    Em_SetStateSync(em, 2, state, mode);
}

void EmRathalos_SetGroup3State(Em *em, u16 state, u8 mode)
{
    EmFlyerWork *w = (EmFlyerWork *)&em->work;
    f32 pos2[3];
    f32 pos[3];
    f32 f;

    em->motionSpeed = 1.0f;
    switch (state) {
    case 0:
    case 1:
    case 7:
        Em_SetTargetAngle(em);
        break;
    case 6:
        if (em->remote == 0 && (Game_IsOnline() == 0 || mode != 0) && (em->stateRand & 1)) {
            state = 26;
        }
        break;
    case 2:
        Em_SetTargetAngle(em);
        if (em->targetNo == -1) {
            em->goalDist = 2500.0f;
            break;
        }
        em->target = &gPlayers[em->targetNo];
        Em_GetTargetPos(em, em->targetNo, pos2);
        Vec_Set(em->goal, pos2[0], pos2[1], pos2[2]);
        f = Vec_DistanceXZ(&em->posX, em->goal);
        em->goalDist = f;
        if (f > 5000.0f) {
            em->goalDist = 5000.0f;
        } else if (f < 2500.0f) {
            em->goalDist = 2500.0f;
        }
        break;
    case 4:
        if (em->remote == 0 && (Game_IsOnline() == 0 || mode != 0) && em->curGoalKind == 1 && em->curGoalArg == 0) {
            if (em->targetNo == -1) {
                w->countdown = 1;
                Em_SetTargetAngle(em);
                break;
            }
            Em_GetTargetPos(em, em->targetNo, pos);
            f = em->playerDistXZ[em->targetNo];
            if (f <= 1000.0f) {
                f = pos[1];
                if (f <= em->posY + 50.0f) {
                    state = 24;
                } else if (f <= em->posY + 150.0f) {
                    state = 4;
                } else {
                    state = 25;
                }
            } else if (f <= 2000.0f) {
                f = pos[1];
                if (f <= em->posY - 150.0f) {
                    state = 24;
                } else if (f <= em->posY + 300.0f) {
                    state = 4;
                } else {
                    state = 25;
                }
            } else {
                f = pos[1];
                if (f <= em->posY - 200.0f) {
                    state = 24;
                } else if (f <= em->posY + 400.0f) {
                    state = 4;
                } else {
                    state = 25;
                }
            }
        }
    case 23:
    case 24:
    case 25:
        w->countdown = 1;
        Em_SetTargetAngle(em);
        break;
    case 8:
    case 21:
        if (em->remote == 0) {
            EmRathalos_SetEscapePoint(em);
            em->timer = 1.5f * Vec_DistanceXZ(&em->posX, em->goal) / 50.0f;
        }
        break;
    case 10:
    case 17:
    case 22:
    case 29:
        w->countdown = em->stateRand % 2 + 1;
    case 9:
    case 28:
        if (em->targetNo == -1) {
            em->timer = 200;
            EmRathalos_ChangeState(em, 2, 2, 1);
            return;
        }
        break;
    case 11:
        if (em->remote == 0) {
            if (em->targetNo == -1) {
                em->timer = 200;
                EmRathalos_ChangeState(em, 2, 2, 1);
                return;
            }
            em->target = &gPlayers[em->targetNo];
        } else {
            em->target = &gPlayers[em->targetNo];
        }
        break;
    case 18:
    case 27:
        if (em->targetNo == -1) {
            em->timer = 150;
            em->hasGoal = 0;
        } else {
            em->hasGoal = 1;
            f = Vec_DistanceXZ(&em->posX, em->goal);
            em->goalDist = f - 500.0f;
            em->timer = (s32) (f / 30.0f) + 30;
        }
        if (em->timer < 30) {
            em->timer = 30;
        }
        break;
    case 15:
        if (em->targetNo == -1) {
            em->timer = 150;
            em->hasGoal = 0;
        } else {
            em->hasGoal = 1;
            f = Vec_DistanceXZ(&em->posX, em->goal) + 700.0f;
            em->goalDist = f;
            em->timer = (s32) (f / 30.0f) + 30;
        }
        break;
    case 16:
    case 31:
    case 32:
        if (em->targetNo == -1) {
            EmRathalos_ChangeState(em, 0, 1, 1);
            return;
        }
        em->target = &gPlayers[em->targetNo];
        break;
    case 3:
    case 5:
    case 12:
    case 13:
    case 14:
    case 19:
    case 20:
    case 26:
    case 30:
        break;
    }
    Em_SetStateSync(em, 3, state, mode);
}

void EmRathalos_ChangeState(Em *em, u16 group, u16 state, u8 mode)
{
    if (em->remote == 0) {
        Em_TargetPlayerInRange(em);
        if (em->stance == 0) {
            switch (group) {
            case 2:
                switch (state) {
                case 1:
                case 2:
                case 6:
                case 7:
                case 8:
                case 9:
                case 10:
                case 11:
                case 13:
                case 14:
                case 15:
                case 16:
                case 17:
                case 18:
                case 19:
                case 21:
                case 22:
                case 24:
                    state = 0;
                    group = 1;
                    break;
                case 0:
                case 3:
                case 4:
                case 5:
                case 12:
                case 20:
                case 23:
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
    case 0:
        EmRathalos_SetGroup0State(em, state, mode);
        break;
    case 1:
        EmRathalos_SetGroup1State(em, state, mode);
        break;
    case 2:
        EmRathalos_SetGroup2State(em, state, mode);
        break;
    case 3:
        EmRathalos_SetGroup3State(em, state, mode);
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
    case 6:
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        break;
    case 4:
        switch (state) {
        case 4:
            Em_EndAllStatus(em);
            em->fxFlags |= 1;
            em->hit8Count++;
            break;
        case 11:
            Em_BeginStatus4(em);
            break;
        case 12:
            em->holdTimer = 50;
            Em_EndAllStatus(em);
            break;
        case 14:
            Em_BeginStatus4(em);
            break;
        case 15:
            Em_ResetToBasePose(em);
            Em_BreakPart(em, 3);
            break;
        case 17:
            Em_EndStatus4(em);
            break;
        case 18:
            Em_EndStatus2(em);
            break;
        case 19:
            Em_EndStatus1(em);
            break;
        case 20:
            Em_BeginStatus5(em);
            break;
        case 21:
            Em_BeginStatus5(em);
            break;
        case 22:
            Em_EndStatus5(em);
            break;
        case 23:
            Em_EndAllStatus(em);
            break;
        }
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        break;
    case 7:
        break;
    }
}

void EmRathalos_SetEscapePoint(Em *em)
{
    f32 pos[3];
    f32 dist[4];
    f32 (*pts)[3];
    u32 i;
    f32 (*p)[3];
    f32 best;
    f32 t;
    u8 k;
    f32 *sel;

    if (em->targetNo == -1) {
        EmRathalos_ChangeState(em, 2, 6, 1);
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
    t = dist[1];
    if (best < t) {
        best = t;
        k = 1;
    }
    t = dist[2];
    if (best < t) {
        best = t;
        k = 2;
    }
    t = dist[3];
    if (best < t) {
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

void EmRathalos_StepPitch(Em *em, s32 mode, f32 height)
{
    EmWork *work = (EmWork *) &em->work;
    f32 pos[3];
    s32 d;

    if (em->targetNo == -1) {
        return;
    }
    switch (mode) {
    case 0:
        Em_GetTargetPos(em, em->targetNo, pos);
        d = 0x10000 - Math_Heading(Vec_Distance((f32 *) &em->posX, (f32 *) pos), em->posY, 0.0f, pos[1] + height);
        d = (d - em->rotX) & 0xFFFF;
        break;
    case 1:
        d = (0xD000 - em->rotX) & 0xFFFF;
        break;
    case 2:
        d = -em->rotX & 0xFFFF;
        break;
    }
    if (d == 0) {
        return;
    }
    if (d < 0x8000) {
        if (em->rotX < 0x3000 || em->rotX >= 0xD000) {
            if (d <= work->pitchStep) {
                em->rotX += d;
            } else {
                em->rotX += work->pitchStep;
            }
        }
    } else {
        if (em->rotX > 0xD000 || em->rotX <= 0x3000) {
            if (d >= 0x10000 - work->pitchStep) {
                em->rotX -= 0x10000 - d;
            } else {
                em->rotX -= work->pitchStep;
            }
        }
    }
    em->rotX = (u16) em->rotX;
}

void EmRathalos_MoveByVelYCurve(Em *em, s32 kind)
{
    EmWork *work = (EmWork *) &em->work;
    EmCurveKey *tbl;
    EmCurveKey *p;
    f32 frame;
    f32 t;
    s32 i;

    tbl = EmRathalos_MoveVelYCurves[kind];
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
        work->moveAngX = 0;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
    }
}

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

static s32 EmRathalos_CurveNone(Em *em, EmWork *work)
{
    return 0;
}


extern EmCurveKey *EmRathalos_VelYCurves[];

/* The value of curve work[0x4F] at the frame in the work area (a table of keys): sets
 * em->velY from the key pair around the frame, 2 at the table's end. */
static u8 EmRathalos_CurveVelY(Em *em, EmWork *work)
{
    EmFlyerWork *w = (EmFlyerWork *) work;
    EmCurveKey *p;
    f32 frame;
    f32 t;
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

extern EmCurveKey *EmRathalos_SpeedCurves[];

/* The value of curve work[0x4F] at the frame in the work area: sets em->velZ from the key
 * pair around the frame, 4 at the table's end. */
static u8 EmRathalos_CurveSpeed(Em *em, EmWork *work)
{
    EmFlyerWork *w = (EmFlyerWork *) work;
    EmCurveKey *p;
    f32 frame;
    f32 t;
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
    if (em->stance == 2) {
        w->moveAngX = 0;
        w->moveAngY = em->rotY;
        w->moveAngZ = 0;
        Em_MoveRotatedAccel(em, &w->moveAngX);
    }
    return ret;
}

void EmFatalis_MotionSounds(s32 em, EmWork *work)
{
    s16 motion;

    motion = work->sndMotion;
    switch (motion) {
    case 0x3E9:
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 0x5B, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x118, 0x54, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x30, 9, 0x22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x34, 9, 0x28);
        break;
    case 0x3EA:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x14, 0x52, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x10, 1, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x56, 1, 5);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xC, 0xE, 0xB);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x52, 0xE, 0xB);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x12, 0x65, 9, 8);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x58, 0x65, 5, 8);
        break;
    case 0x3EB:
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 0x46, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 0x16, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x60, 0x2A, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1C, 1, 5);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x5E, 1, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3C, 0x1A, 0x11);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x6C, 0x19, 0x11);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x60, 0x65, 9, 8);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x1E, 0x65, 5, 8);
        break;
    case 0x3EC:
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 0x46, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 0x16, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x60, 0x2A, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1C, 1, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x5E, 1, 5);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3C, 0x1A, 0x11);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x6C, 0x19, 0x11);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x60, 0x65, 5, 8);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x1E, 0x65, 9, 8);
        break;
    case 0x3ED:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x12, 0x46, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x44, 0x26, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x22, 3, 0x1D);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x26, 6, 0x18);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x70, 2, 0x1D);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1C, 9, 0x22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x20, 9, 0x28);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2E, 0x10, 0xB);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1C, 0x1A, 0xB);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x28, 0x65, 0, 8);
        break;
    case 0x3EE:
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 0x16, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3A, 0x1A, 0xB);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2E, 0x46, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x80, 1, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2E, 1, 5);
        break;
    case 0x3EF:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA, 0x4B, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x11, 0x11);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x24, 4, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x44, 4, 5);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x62, 4, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x86, 4, 5);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1E, 2, 0x18);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3E, 2, 0x1D);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x5C, 2, 0x18);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x7E, 2, 0x1D);
        break;
    case 0x3F0:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x32, 0x46, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0xF, 0x11);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x16, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x66, 4, 5);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x86, 4, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x50, 0xD, 0x22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x54, 0xD, 0x28);
        break;
    case 0x3F1:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x4A, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x34, 0x2A, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 0x11, 0x11);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x8C, 7, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xC, 0xA, 0x22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x12, 0xA, 0x28);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x4C, 0xA, 0x22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x50, 0xA, 0x28);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x7C, 0xA, 0x22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x82, 0xA, 0x28);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xBA, 0xA, 0x22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xBE, 0xA, 0x28);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xF6, 0xA, 0x22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xFA, 0xA, 0x28);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x138, 0xA, 0x22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x13C, 0xA, 0x28);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x180, 0xA, 0x22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x184, 0xA, 0x28);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x8C, 0x65, 0, 8);
        break;
    case 0x3F2:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1E, 0xA, 0x22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x22, 0xA, 0x28);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x64, 0xA, 0x22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x68, 0xA, 0x28);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2E, 0x18, 0x11);
        break;
    case 0x3F3:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x59, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1C, 0xA, 0x22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x20, 0xA, 0x28);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x5A, 0xC, 0x22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x5E, 0xC, 0x28);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x8C, 0xC, 0x22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x90, 0xC, 0x28);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2E, 0x1A, 0x11);
        break;
    case 0x3F4:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x14, 0x46, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x7A, 0x4B, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x10, 3, 5);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x14, 5, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1A, 8, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2C, 0xA, 0x18);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2C, 6, 0x1D);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x8E, 2, 0x1D);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x9A, 4, 5);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xB6, 3, 0x18);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x10, 0xD, 0x22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x14, 0xD, 0x28);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2E, 0xA, 0x22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x32, 0xA, 0x28);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x6C, 0xD, 0x22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x70, 0xD, 0x28);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x14, 0x65, 9, 8);
        break;
    case 0x3F5:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x14, 0x47, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x56, 0x48, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x5E, 0x1B, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xB0, 0x21, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x17, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x60, 0x1A, 0x11);
        break;
    case 0x3F6:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x47, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x58, 0x48, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x5E, 0x1B, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xB0, 0x21, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x17, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x60, 0x1A, 0x11);
        break;
    case 0x3F7:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0xF, 0xB);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xB4, 0xF, 0xB);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1E, 0x1C, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x52, 0x4B, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xEE, 0x2A, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x56, 0x19, 0x33);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2C, 1, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x5E, 1, 5);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xFA, 1, 5);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x12C, 1, 5);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x60, 0x65, 5, 8);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x2E, 0x65, 9, 8);
        break;
    case 0x3F8:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x14, 0x48, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x14, 0x1B, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xC, 0xC, 0x22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x10, 0xC, 0x28);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x5C, 0xC, 0x22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3C6, 0xC, 0x28);
        break;
    case 0x3FA:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x20, 0x4A, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x94, 0x21, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x12, 4, 5);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x90, 4, 5);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2C, 0x1A, 0x1D);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 0xF, 0xB);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x50, 0xF, 0xB);
        break;
    case 0x3FB:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x1C, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x32, 0x1D, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x6C, 0x46, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x38, 0x1A, 0x33);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xE, 1, 5);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xAC, 4, 5);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x11, 0x11);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3E, 0x11, 0x11);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x8E, 0x11, 0x11);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x10, 0x65, 5, 8);
        break;
    case 0x3FC:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x4C, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x58, 0x29, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xAA, 0x4E, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA, 0x17, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x10, 1, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xB8, 4, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xCE, 4, 5);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x14, 0x11, 0xB);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x86, 0xF, 0xB);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x12, 9, 0x22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 9, 0x28);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x7A, 9, 0x22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x7E, 9, 0x28);
        break;
    case 0x3FD:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x4F, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x64, 0x50, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1A, 1, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x36, 1, 5);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xC4, 1, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xE2, 1, 5);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x48, 6, 0x18);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA, 0x17, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x20, 9, 0x22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x24, 9, 0x28);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA0, 0xF, 0xB);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x4A, 0x65, 0x1D, 8);
        break;
    case 0x3FE:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x4F, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x78, 0x29, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xC2, 0x4E, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 0x14, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1E, 3, 5);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x38, 3, 0x18);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3C, 6, 0x1D);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x20, 9, 0x22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x24, 9, 0x28);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x4A, 0x10, 0xB);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x14, 0xF, 0xB);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x46, 0x18, 0x11);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA8, 1, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x9C, 0x18, 0x11);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x106, 0, 5);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x3E, 0x65, 0x1D, 8);
        break;
    case 0x3FF:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x4F, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x78, 0x29, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xC2, 0x4E, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1E, 6, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x22, 0x14, 5);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA2, 1, 5);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x40, 3, 0x18);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x42, 6, 0x1D);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1A, 9, 0x22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1E, 9, 0x28);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x10, 0xB);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x46, 0xF, 0xB);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1A, 0x1A, 0x11);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x40, 0x18, 0xB);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xF8, 0, 5);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x3E, 0x65, 0x1D, 8);
        break;
    case 0x400:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x49, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x70, 0x35, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x196, 0x35, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x280, 0x29, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x12, 3, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x82, 1, 5);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x112, 0x12, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x136, 6, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x5C, 0xA, 0x22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x58, 0xA, 0x28);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xEA, 0xA, 0x22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xEE, 0xA, 0x28);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1A2, 0x19, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x198, 0x16, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x58, 0x1A, 0x11);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xD8, 0x11, 0xB);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xE6, 0x1A, 0x11);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x254, 0x11, 0xB);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x136, 0x65, 0, 9);
        break;
    case 0x401:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x4C, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0xE, 0x22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 0xE, 0x28);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x18, 0x11);
        break;
    case 0x402:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xE, 8, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x34, 6, 0x33);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xE, 0x53, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x78, 2, 0x18);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x86, 2, 0x1D);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x118, 4, 5);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xDC, 4, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA6, 0x15, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x8A, 0x21, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xCC, 2, 0x18);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x10, 0x65, 0, 9);
        break;
    case 0x403:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x49, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x52, 0x2F, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xAC, 0x30, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x128, 0x56, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x24, 3, 0x1D);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x24, 7, 0x18);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x38, 6, 0x33);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x86, 0x16, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x11, 0xB);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x18, 0x1A, 0x11);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x60, 0x18, 0x11);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x8E, 0x1A, 0x11);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x138, 0x18, 0x11);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x26, 0x65, 0x1D, 8);
        break;
    case 0x404:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x50, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x8C, 0x5A, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x12E, 0x59, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x15, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x8C, 0x17, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xDC, 3, 0x1D);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xDC, 6, 0x18);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0xDE, 0x65, 0x1D, 8);
        break;
    case 0x405:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x50, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x78, 0x35, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x162, 0x32, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x162, 0x2B, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1AE, 0x35, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x15, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x50, 0x18, 0x11);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x13E, 0xF, 0xB);
        break;
    case 0x406:
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 0x5B, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xBA, 0x21, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 9, 0x22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1A, 9, 0x28);
        break;
    case 0x407:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x1F, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1C, 0xA, 0x22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x20, 0xA, 0x28);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x6A, 0xA, 0x22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x6E, 0xA, 0x28);
        break;
    case 0x408:
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 4, 0x55, 0x34, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3E, 0xC, 0x22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x42, 0xC, 0x28);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0xF, 0x11);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x78, 0x11, 0x11);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x18, 0xB);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x34, 0x18, 0xB);
        break;
    case 0x409:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x47, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x50, 0x48, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x5E, 0x1B, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x82, 0x21, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x9C, 0x1E, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x38, 0xA, 0x22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3C, 0xA, 0x28);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x7A, 0xC, 0x22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x7E, 0xC, 0x28);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xB4, 0xC, 0x22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xBA, 0xC, 0x28);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xF4, 0xC, 0x22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x94, 0xC, 0x28);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3E, 7, 5);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x17, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0xF, 0x11);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x4C, 0x1A, 0x11);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x40, 0x65, 0, 8);
        break;
    case 0x40A:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x5A, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x96, 0x50, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x140, 0x33, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1A4, 0x5C, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x17, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xF8, 6, 0x1D);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x10A, 8, 0x33);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xEA, 0x19, 0x33);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x10E, 0x65, 0x33, 8);
        break;
    case 0x40B:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x12, 0xE, 0x22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 0xE, 0x28);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x46, 0xC, 0x22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x4A, 0xC, 0x28);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 5, 9);
        break;
    case 0x40C:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x3F, 0x33);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x40, 0x28);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 0x40, 0x22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x22, 0x33);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x26, 0xC, 0x22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x28, 0xC, 0x28);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x4E, 0xA, 0x22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x52, 0xA, 0x28);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x9E, 9, 0x22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA2, 9, 0x28);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2C, 0x20, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x24, 0x12, 5);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3E, 5, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x9E, 1, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x54, 6, 0);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 0x54, 0x65, 0, 8);
        break;
    case 0x40D:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x10, 0x49, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x18, 0xE, 0x22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1C, 0xE, 0x28);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x3F, 0x33);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x40, 0x28);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 0x40, 0x22);
        break;
    case 0x40E:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x3F, 0x33);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x40, 0x28);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 0x40, 0x22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x22, 0x41, 0x33);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x22, 0x40, 0x28);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x26, 0x40, 0x22);
        break;
    case 0x40F:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x3F, 0x33);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x40, 0x28);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 0x40, 0x22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x22, 0x33);
        break;
    case 0x410:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x3F, 0x33);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x40, 0x28);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 0x40, 0x22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x22, 0x41, 0x33);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x22, 0x40, 0x28);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x26, 0x40, 0x22);
        break;
    case 0x411:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x20, 0x4A, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x94, 0x21, 0x34);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x12, 4, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x90, 4, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2C, 0x1A, 0x18);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 0xF, 0xB);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x50, 0xF, 0xB);
        
    default:
        break;
    }
}

void EmFatalis_Init(Em *em)
{
    EmFatalisWork *w = (EmFatalisWork *)&em->work;

    f32 fslot;
    f32 fslot2;
    s16 size;
    u8 slotNo;
    u8 slot;

    if (gQuest.stage == 0) {
        switch (gScene.area) {
        case 0x19:
            slotNo = em->slotNo;
            fslot = (f32) (u32) slotNo;
            em->posX = 12000.0f - (300.0f * fslot);
            em->posY = 0.0f;
            slot = em->slotNo;
            fslot2 = (f32) (u32) slot;
            em->posZ = 16000.0f - (300.0f * fslot2);
            break;
        }
    }
    size = Em_ComputeSize(em, 0);
    em->hp = size;
    em->hpMax = size;
    w->initHp = em->hp;
    em->blind = 0;
    em->unk_InitFlag = 1;
    em->combatPlayers = 0xF;
    em->combat = 1;
    Em_SetMotionRow(em, 1, 0, 0);
    em->stance = 0;
    EmFatalis_ChangeState(em, 0, 1, 0);
    w->rageTimer = -1;
}

void EmFatalis_UpdateRageTimer(Em *em)
{
    EmFatalisWork *w = (EmFatalisWork *)&em->work;
    s16 a;
    s16 b;

    if (em->type == 0x24 || em->type == 0x47) {
        a = 0x28;
        if (em->type == 0x24) {
            b = 0x32;
        } else {
            a = 0x3C;
            b = a;
        }
        if (em->rage != 0) {
            switch (em->rageStep) {
            case 0:
                if (++w->rageTimer > a) {
                    em->rageStep++;
                    w->rageTimer = 0;
                }
                break;
            case 1:
                w->rageTimer = 0;
                break;
            }
        } else {
            em->rageStep = 0;
            if (w->rageTimer != -1) {
                if (++w->rageTimer > b) {
                    w->rageTimer = -1;
                }
            }
        }
    }
}

void EmFatalis_AddMinePoints(Em *em)
{
    u8 *w = &em->work;
    s32 i;
    MinePoint pt;
    f32 pos[3];

    if (em->type == 2) {
        switch (gQuest.header->tier) {
        case 0:
        default:
            pt.table = 3;
            break;
        case 1:
            pt.table = 0x7F;
            break;
        case 2:
            pt.table = 0xFC;
            break;
        case 3:
            pt.table = 0xFC;
            break;
        }
    } else if (em->type == 0x47) {
        switch (gQuest.header->tier) {
        default:
            pt.table = 0x7A;
            break;
        case 1:
            pt.table = 0xF7;
            break;
        case 2:
            pt.table = 0x173;
            break;
        case 3:
            pt.table = 0x173;
            break;
        }
    } else {
        switch (gQuest.header->tier) {
        default:
            pt.table = 0x30;
            break;
        case 1:
            pt.table = 0xAC;
            break;
        case 2:
            pt.table = 0x129;
            break;
        case 3:
            pt.table = 0x129;
            break;
        }
    }
    for (i = 0; i < 3; i++) {
        pt.radius = EmFatalis_MinePtRadius[i];
        pt.area = (u8) em->area;
        pt.timer = 0;
        pt.flags = 1;
        Em_BonePos(em, EmFatalis_MinePtBones[i][0], pos);
        pos[1] = em->floorY;
        pt.unk14 = 2;
        pt.count = EmFatalis_MinePtBones[i][1];
        func_3A6120(&pt);
        w[i + 0x54] = MinePoint_AddUntimed(&pt, pos, 0);
    }
}

void EmFatalis_UpdateMinePoints(Em *em)
{
    u8 *w = &em->work;
    s32 i;
    s8 *t;
    f32 *y;
    s8 *p;
    f32 pos[3];

    t = EmFatalis_MinePtBones[0];
    y = &pos[1];
    for (i = 0; i < 3; i++) {
        p = (s8 *) (w + i) + 0x54;
        if (*p != -1) {
            Em_BonePos(em, *t, pos);
            *y = em->floorY;
            MinePoint_SetPos(*p, pos);
            if (MinePoint_Count(*p) <= 0) {
                MinePoint_Remove(*p);
                *p = -1;
            }
        }
        t += 2;
    }
}

void EmFatalis_RemoveMinePoints(Em *em)
{
    u8 *w = &em->work;
    s32 i;
    s8 *p;

    for (i = 0; i < 3; i++) {
        p = (s8 *) (w + i) + 0x54;
        if (*p != -1) {
            MinePoint_Remove(*p);
            *p = -1;
        }
    }
}

void EmFatalis_SpawnK1COrK11(Em *em, s32 w, s32 v)
{
    if (em->type == 0x47) {
        StageObj_K1C_CreateOnEmB(em, 0x26, w, v, 1.0f);
        return;
    }
    StageObj_K11_CreateAtBone(em, 0x33, 1, 1);
    StageObj_K11_CreateAtBone(em, 0x33, 2, 1);
}

void EmFatalis_SpawnEffect33Or08(Em *em, s32 arg1, s32 d)
{
    if (em->type == 0x47) {
        EffEm33_Create(em, 0xD, 0, arg1, d);
        return;
    }
    EffEm08_CreateAtBone(em, 0x33, 0xB, 0, arg1, d);
}

void EmFatalis_SpawnEffect33Or08b(Em *em, s32 arg1)
{
    if (em->type == 0x47) {
        EffEm33_Create(em, 0xC, arg1, 0, 0);
    } else {
        EffEm08_CreateAtBone(em, 0, 0xC, arg1, 0x4000, 0);
    }
}

void EmFatalis_EndState(Em *em)
{
    em->scriptPending = 1;
    em->motionSpeed = 1.0f;
    EmFatalis_ChangeState(em, 0, 1, 0);
}

void EmFatalis_EndState2(Em *em, u8 a)
{
    em->scriptPending = 1;
    em->motionSpeed = 1.0f;
    switch (a) {
    case 0:
        EmFatalis_ChangeState(em, 2, 2, 0);
        break;
    }
}

void EmFatalis_Update(Em *em)
{
    u8 *w = &em->work;
    u8 buf[4];

    EmFatalis_UpdateRageTimer(em);
    em->painted = 0;
    em->paintTimer = 0;
    Em_RaiseTrigger80Lax(em);
    Em_HoldPosNoPlayers(em, 0, 1);
    if (em->stateGroup != 5) {
        if (em->hp <= (s16) (0.65f * em->hpMax)) {
            if (em->rage == 0) {
                Em_CheckRageByHp(em, em->rageThreshold);
            } else {
                em->rage = 1;
                em->rageTimer = 1800;
            }
        }
    } else {
        em->rage = 0;
    }
    if (em->type == 0x24 || em->type == 0x47) {
        if (em->hp <= (s16) (0.2f * em->hpMax)) {
            em->hitzoneSet = 0;
        } else if (em->hp <= (s16) (0.5f * em->hpMax)) {
            em->hitzoneSet = 1;
        } else {
            em->hitzoneSet = 0;
        }
    }
    switch (Em_UpdateDamage(em, buf) & 0xFF) {
    case 0:
    case 2:
    case 4:
    case 7:
    case 9:
    case 14:
    case 15:
    case 16:
    case 18:
        break;
    case 1:
        if (em->stance == 2) {
            EmFatalis_ChangeState(em, 5, 2, 3);
        } else if (em->lastAttackKind == 9) {
            EmFatalis_ChangeState(em, 5, 1, 3);
        } else {
            EmFatalis_ChangeState(em, 5, 0, 3);
        }
        break;
    case 3:
        EmFatalis_ChangeState(em, 4, 4, 3);
        break;
    case 5:
        if (em->stateGroup != 4 || em->state != 3) {
            EmFatalis_ChangeState(em, 4, 3, 3);
        }
        break;
    case 6:
        if ((em->stateGroup != 4 || em->state != 5) && (em->stateGroup != 4 || em->state != 3)) {
            Em_SetStatus4Timer(em);
            EmFatalis_ChangeState(em, 4, 5, 3);
        }
        break;
    case 8:
        if ((em->stateGroup != 0 || em->state != 4) && (em->stateGroup != 4 || em->state != 3)) {
            Em_SetStatus2Timer(em);
            EmFatalis_ChangeState(em, 0, 4, 3);
        }
        break;
    case 10:
        if (em->stateGroup == 0 && em->state == 4) {
            EmFatalis_ChangeState(em, 0, 5, 3);
            em->scriptPending = 0;
        }
        break;
    case 11:
        EmFatalis_ChangeState(em, 4, 4, 3);
        break;
    case 12:
        Player_ClearFlag((Player *) em, 0x20000);
        if (em->stance == 2) {
            EmFatalis_ChangeState(em, 4, 3, 3);
        } else {
            switch (em->flinchPart) {
            case 0:
            case 1:
            case 4:
            case 5:
                EmFatalis_ChangeState(em, 4, 0, 3);
                break;
            case 6:
                EmFatalis_ChangeState(em, 4, 1, 3);
                break;
            case 7:
            default:
                if (em->flinchPart != 3) {
                    EmFatalis_ChangeState(em, 4, 2, 3);
                } else {
                    EmFatalis_ChangeState(em, 4, 6, 3);
                }
                break;
            }
        }
        break;
    case 13:
        if (em->stance != 2) {
            EmFatalis_ChangeState(em, 4, 0, 3);
            em->scriptPending = 0;
        }
        break;
    case 17:
        Em_EndStatus5(em);
        break;
    }
    if (em->type == 2) {
        if (Area_DayId(em->area) == 0x19) {
            if (Save_TestEventFlag(0xB0) == 0) {
                if (gScene.eventRunning == 1 && em->stateGroup != 6) {
                    EmFatalis_ChangeState(em, 6, 1, 0);
                }
            } else if (Quest_IsRunning(0) == 1) {
                if (em->stateGroup != 6 && em->stateGroup != 5) {
                    em->scriptPending = 0;
                    if (gScene.area == em->area) {
                        CamReq_ForMonster(em);
                    }
                    if (em->stance == 2) {
                        EmFatalis_ChangeState(em, 6, 2, 0);
                    } else {
                        EmFatalis_ChangeState(em, 6, 0, 0);
                    }
                }
            } else {
                goto normal;
            }
        } else {
            goto normal;
        }
    } else if (em->type == 0x24) {
        if (Area_DayId(em->area) == 0x58 && Save_TestEventFlag(0xB1) == 0) {
            if (gScene.eventRunning == 1 && em->stateGroup != 6) {
                EmFatalis_ChangeState(em, 6, 3, 0);
            }
        } else {
            goto normal;
        }
    } else if (Area_DayId(em->area) == 0x7C && em->type == 0x47 && Save_TestEventFlag(0xB2) == 0) {
        if (gScene.eventRunning == 1 && em->stateGroup != 6) {
            EmFatalis_ChangeState(em, 6, 4, 0);
        }
    } else {
    normal:
        Em_UpdatePendingScript(em);
    }
    Em_ClearFlag6AE(em);
    EmFatalis_RunState(em, (s32) w);
    if (em->stateRequest != 0) {
        EmFatalis_RunState(em, (s32) w);
        em->stateRequest = 0;
    }
}

void EmFatalis_State0_00(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        if (em->motionId[0] != 0x3E9) {
            Em_SetLayerMotion(em, 0x3E9, 0xA, 0, 0);
        }
        if (em->motionId[1] != 0x44D) {
            Em_SetLayerMotion(em, 0x4B1, 0xA, 0, 1);
        }
        if (em->motionId[2] != 0x4B1) {
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

void EmFatalis_State0_01(Em *em, EmWork *work)
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
            EmFatalis_EndState(em);
        }
        break;
    }
}

void EmFatalis_State0_02(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x1E, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmFatalis_EndState(em);
        }
        break;
    }
}

void EmFatalis_State0_03(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x20, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmFatalis_EndState(em);
        }
        break;
    }
}

void EmFatalis_State0_04(Em *em, EmWork *work)
{
    f32 pos[3];

    Em_SetFlag6AE(em, 2U);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x22, 0, 0);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmFatalis_ChangeState(em, 0, 5, 3);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmFatalis_ChangeState(em, 0, 5, 3);
        }
        break;
    }
    if (EmMotion_ReachedFrame(em, 0, 296.0f) != 0) {
        pos[0] = 0.0f;
        pos[1] = 100.0f;
        pos[2] = 200.0f;
        Em_SpawnEffect06Periodic(em, 0x33, pos, 1.6f);
    }
}

void EmFatalis_State0_05(Em *em, EmWork *work)
{
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 8, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            EmFatalis_EndState(em);
        }
        break;
    }
}

void EmFatalis_State0_06(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 8, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmFatalis_EndState(em);
        }
        break;
    }
}

void EmFatalis_State1_00(Em *em, EmWork *work)
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
            EmFatalis_EndState(em);
        }
        break;
    }
}

void EmFatalis_State1_01(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_StartTurnToTarget(em, &EmFatalis_TurnParams[0]);
        break;
    case 1:
        if (Em_TurnStep(em, &EmFatalis_TurnParams[0]) == 1) {
            em->step++;
            Player_ClearFlag(em, 0x20000);
            EmFatalis_EndState(em);
        }
        break;
    }
}

void EmFatalis_State1_02(Em *em, EmWork *work)
{
    u32 diff;
    u32 step;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        work->goalYaw = Math_AngleXZ(&em->posX, em->goal);
        diff = (work->goalYaw - em->rotY) & 0xFFFF;
        if (diff <= 0x2000U || diff >= 0xE000U) {
            Em_SetMotion(em, 6, 0, 0);
        } else if (diff >= 0x8000U) {
            Em_SetMotion(em, 3, 0, 0);
        } else {
            Em_SetMotion(em, 4, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].blend != 0) {
            break;
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            Player_ClearFlag((Player *) em, 0x20000);
            EmFatalis_EndState(em);
            break;
        }
        if (em->motionId[0] == 0x3EE) {
            step = 91.0f * em->motionSpeed;
            diff = (work->goalYaw - (em->rotY & 0xFFFF)) & 0xFFFF;
            if ((u32) ((diff + step) & 0xFFFF) < step * 2) {
                em->rotY = work->goalYaw;
            } else if (diff < 0x8000U) {
                em->rotY = (em->rotY + step) & 0xFFFF;
            } else {
                em->rotY = (em->rotY - step) & 0xFFFF;
            }
        } else if (EmMotion_ReachedFrame(em, 0, 89.0f) == 0) {
            step = 372.0f * em->motionSpeed;
            diff = (work->goalYaw - (em->rotY & 0xFFFF)) & 0xFFFF;
            if ((u32) ((diff + step) & 0xFFFF) < step * 2) {
                em->rotY = work->goalYaw;
            } else if (diff < 0x8000U) {
                em->rotY = (em->rotY + step) & 0xFFFF;
            } else {
                em->rotY = (em->rotY - step) & 0xFFFF;
            }
        }
        break;
    }
}

void EmFatalis_State2_00(Em *em, EmWork *work, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        if (variant == 0) {
            em->stance = 0;
        } else {
            em->stance = 2;
            Player_ClearFlag(em, 0x20000);
        }
        em->lookAtTarget = 0;
        Em_SetMotion(em, 9, 0, 0);
        EmFatalis_StartCurves(em, 0);
        work->flyGuard = 0;
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 134.0f, 0U) != 0) {
            em->step++;
            em->stance = 2;
            EmFatalis_UpdateCurves(em);
        }
        break;
    case 2:
        if (EmFatalis_UpdateCurves(em) != 0) {
            em->step++;
            EmFatalis_EndState2(em, 0);
        }
        break;
    }
}

void EmFatalis_State2_01(Em *em, EmWork *work)
{
    f32 floorY;

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
        work->flyGuard = 0;
        work->moveAngX = 0;
        work->moveAngZ = 0;
        break;
    case 1:
        work->moveAngY = em->rotY;
        Em_MoveRotated(em, &work->moveAngX);
        if (!((700.0f + em->floorY) <= em->posY)) {
            em->step++;
            em->velY = -20.0f;
        }
        break;
    case 2:
        work->moveAngY = em->rotY;
        Em_MoveRotated(em, &work->moveAngX);
        if (em->posY <= em->floorY) {
            em->step++;
            Em_SetMotion(em, 0xC, 0, 0);
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
            Em_SetMotion(em, 8, 0, 0);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmFatalis_EndState(em);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmFatalis_State2_02(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        work->turnSpeed = 0x100;
        if (em->motionId[0] != 0x3F2) {
            Em_SetLayerMotion(em, 0x3F2, 0, 0, 0);
        }
        if (em->motionId[1] != 0x4BA) {
            Em_SetLayerMotion(em, 0x4BA, 0, 0, 1);
        }
        if (em->motionId[2] != 0x582) {
            Em_SetLayerMotion(em, 0x582, 0, 0, 2);
        }
        work->flyGuard = 0;
        Vec_Set(((EmFatalisWork *)work)->startPos, em->posX, em->posY, em->posZ);
        break;
    case 1:
        EmFatalis_MoveByVelYCurve(em, 1);
        if (!(em->posY < ((EmFatalisWork *)work)->startPos[1])) {
            em->posY = ((EmFatalisWork *)work)->startPos[1];
        }
        if (--em->timer <= 0) {
            em->step++;
            EmFatalis_EndState2(em, 0);
        }
        break;
    case 2:
        EmFatalis_MoveByVelYCurve(em, 1);
        if (!(em->posY < ((EmFatalisWork *)work)->startPos[1])) {
            em->posY = ((EmFatalisWork *)work)->startPos[1];
        }
        break;
    }
    if (em->posY < em->floorY) {
        em->posY = em->floorY;
    }
}

void EmFatalis_State2_03(Em *em, EmWork *work, s32 variant)
{
    f32 floorY;
    s32 done;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        work->turnSpeed = 0x100;
        Em_SetMotion(em, 0xA, 0, 0);
        work->flyGuard = 0;
        em->timer = 0x12C;
        if (variant != 0) {
            Player_ClearVelocity(em);
        }
        break;
    case 1:
        EmFatalis_MoveByVelYCurve(em, 1);
        done = Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 1) & 0xFF;
        if (--em->timer > 0) {
            if (done != 0) {
                goto block_9;
            }
        } else {
block_9:
            em->step++;
            EmFatalis_EndState2(em, 0);
        }
        break;
    case 2:
        EmFatalis_MoveByVelYCurve(em, 1);
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmFatalis_State2_04(Em *em, EmWork *work)
{
    f32 dist;
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        work->turnSpeed = 0x100;
        Em_SetMotion(em, 0xA, 0, 0);
        work->flyGuard = 0;
        work->moveAngZ = 0;
        break;
    case 1:
        EmFatalis_MoveByVelYCurve(em, 1);
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 1);
        work->moveAngY = em->rotY;
        Em_GetPitchToGoal(em, &work->moveAngX, 0.0f, 0.0f);
        Em_MoveRotated(em, &work->moveAngX);
        dist = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer > 0) {
            if (dist <= (100.0f * em->velZ)) {
                goto block_9;
            }
        } else {
block_9:
            em->step++;
            EmFatalis_EndState2(em, 0);
        }
        break;
    case 2:
        EmFatalis_MoveByVelYCurve(em, 1);
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmFatalis_State2_05(Em *em, EmWork *work)
{
    f32 pos[3];
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        work->turnSpeed = 0x100;
        Em_SetMotion(em, 0xA, 0, 0);
        work->flyGuard = 0;
        work->moveAngX = 0;
        work->moveAngZ = 0;
        Player_ClearVelocity(em);
        em->velY = 10.0f;
        break;
    case 1:
        work->moveAngY = Math_AngleXZ(&em->posX, pos) & 0xFFFF;
        Em_MoveRotated(em, &work->moveAngX);
        if (--em->timer > 0) {
            if (!(em->posY <= (1500.0f + em->goal[1]))) {
                goto block_9;
            }
        } else {
block_9:
            em->step++;
            Player_ClearVelocity(em);
            EmFatalis_EndState2(em, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmFatalis_State2_06(Em *em, EmWork *work)
{
    f32 dist;
    f32 floorY;
    s32 timer;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        work->turnSpeed = 0x100;
        Em_SetMotion(em, 0xA, 0, 0);
        work->flyGuard = 0;
        work->moveAngX = 0;
        work->moveAngZ = 0;
        break;
    case 1:
        EmFatalis_MoveByVelYCurve(em, 1);
        work->moveAngY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        Em_GetPitchToGoal(em, &work->moveAngX, 0.0f, 0.0f);
        Em_MoveRotated(em, &work->moveAngX);
        dist = Vec_DistanceXZ(&em->posX, em->goal);
        timer = em->timer - 1;
        em->timer = timer;
        if ((timer <= 0) || (dist <= (100.0f * em->velZ))) {
            em->step++;
            EmFatalis_EndState2(em, 0);
        }
        break;
    case 2:
        EmFatalis_MoveByVelYCurve(em, 1);
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmFatalis_State2_07(Em *em, EmWork *work)
{
    f32 pos[3];
    f32 floorY;
    s32 timer;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        work->turnSpeed = 0x100;
        Em_SetMotion(em, 0xA, 0, 0);
        work->flyGuard = 0;
        work->moveAngX = 0;
        work->moveAngZ = 0;
        Player_ClearVelocity(em);
        em->velY = -10.0f;
        break;
    case 1:
        work->moveAngY = Math_AngleXZ(&em->posX, pos) & 0xFFFF;
        Em_MoveRotated(em, &work->moveAngX);
        timer = em->timer - 1;
        em->timer = timer;
        if ((timer <= 0) || (em->posY <= (500.0f + em->floorY))) {
            em->step++;
            Player_ClearVelocity(em);
            EmFatalis_EndState2(em, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmFatalis_State2_08(Em *em, EmWork *work)
{
    f32 pos[3];
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        work->turnSpeed = 0x100;
        Em_SetMotion(em, 0xA, 0, 0);
        work->flyGuard = 0;
        Player_ClearVelocity(em);
        em->velY = 10.0f;
        break;
    case 1:
        work->moveAngY = Math_AngleXZ(&em->posX, pos) & 0xFFFF;
        Em_MoveRotated(em, &work->moveAngX);
        if (!(em->posY < 6000.0f)) {
            em->step++;
            Player_ClearVelocity(em);
            EmFatalis_EndState2(em, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmFatalis_State2_09(Em *em, EmWork *work)
{
    f32 dist;
    f32 floorY;
    s32 timer;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        work->turnSpeed = 0x100;
        Em_SetMotion(em, 0xA, 0, 0);
        work->flyGuard = 0;
        work->moveAngX = 0;
        work->moveAngZ = 0;
        break;
    case 1:
        EmFatalis_MoveByVelYCurve(em, 1);
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 1);
        work->moveAngY = em->rotY;
        Em_MoveRotated(em, &work->moveAngX);
        dist = Vec_DistanceXZ(&em->posX, em->goal);
        timer = em->timer - 1;
        em->timer = timer;
        if ((timer <= 0) || (dist <= (100.0f * em->velZ))) {
            em->step++;
            EmFatalis_EndState2(em, 0);
        }
        break;
    case 2:
        EmFatalis_MoveByVelYCurve(em, 1);
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmFatalis_State2_0A(Em *em, EmWork *work, s32 variant)
{
    f32 dist;
    f32 floorY;
    f32 frames;
    s32 timer;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        work->turnSpeed = 0x100;
        Em_SetMotion(em, 0xA, 0, 0);
        work->flyGuard = 0;
        work->moveAngZ = 0;
        break;
    case 1:
        EmFatalis_MoveByVelYCurve(em, 1);
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 1);
        work->moveAngY = em->rotY;
        Em_GetPitchToGoal(em, &work->moveAngX, 0.0f, 0.0f);
        Em_MoveRotated(em, &work->moveAngX);
        dist = Vec_DistanceXZ(&em->posX, em->goal);
        if (em->wallHitDirs & 0xF000000F) {
            em->posY += 20.0f;
        }
        if (variant == 0) {
            frames = 100.0f;
        } else {
            frames = 10.0f;
        }
        timer = em->timer - 1;
        em->timer = timer;
        if ((timer <= 0) || (dist <= (em->velZ * frames))) {
            em->step++;
            EmFatalis_EndState2(em, 0);
        }
        break;
    case 2:
        EmFatalis_MoveByVelYCurve(em, 1);
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmFatalis_State2_0B(Em *em, EmWork *work)
{
    f32 pos[3];

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 9, 0, 0);
        EmFatalis_StartCurves(em, 6);
        work->flyGuard = 0;
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 134.0f, 0U) != 0) {
            em->step++;
            em->stance = 2;
            EmFatalis_UpdateCurves(em);
        }
        break;
    case 2:
        if (EmFatalis_UpdateCurves(em) != 0) {
            em->step++;
            Player_ClearVelocity(em);
            em->velY = 10.0f;
        }
        break;
    case 3:
        work->moveAngY = Math_AngleXZ(&em->posX, pos) & 0xFFFF;
        Em_MoveRotated(em, &work->moveAngX);
        if (!(em->posY < 6000.0f)) {
            em->step++;
            Player_ClearVelocity(em);
            EmFatalis_EndState2(em, 0);
        }
        break;
    }
}

void EmFatalis_State2_0E(Em *em, EmWork *work)
{
    f32 floorY;

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
        work->flyGuard = 0;
        Player_SetFlag((Player *) em, 0x20000U);
        work->moveAngX = 0;
        work->moveAngZ = 0;
        break;
    case 1:
        work->moveAngY = em->rotY;
        Em_MoveRotated(em, &work->moveAngX);
        if (!(4284.0f <= em->posY)) {
            em->step++;
            em->velY = -20.0f;
        }
        break;
    case 2:
        work->moveAngY = em->rotY;
        Em_MoveRotated(em, &work->moveAngX);
        if (em->posY <= 3584.0f) {
            em->step++;
            Em_SetMotion(em, 0xC, 0, 0);
            em->posY = 3584.0f;
            em->rotX = 0;
            em->rotZ = 0;
            CamShake_ReqAIf(em, 1);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 8, 0, 0);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmFatalis_ChangeState(em, 3, 0x15, 4);
        }
        break;
    case 5:
        if (em->remote == 0) {
            EmFatalis_ChangeState(em, 3, 0x15, 4);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmFatalis_State3_00(Em *em, s32 unused, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        if (variant == 0) {
            Em_SetMotion(em, 0x12, 0, 0);
            break;
        }
        Em_SetMotion(em, 0x29, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmFatalis_EndState(em);
        }
        break;
    }
}

void EmFatalis_State3_01(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x13, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmFatalis_EndState(em);
        }
        break;
    }
}

void EmFatalis_State3_02(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0xD, 0, 0);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 102.0f, 0U) != 0) {
            EmFatalis_SpawnK1COrK11(em, 0, 0);
            EmFatalis_SpawnEffect33Or08(em, 0, 0);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmFatalis_EndState(em);
        }
        break;
    }
}

void EmFatalis_State3_03(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0xE, 0, 0);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 98.0f, 0U) != 0) {
            EmFatalis_SpawnK1COrK11(em, 0, 0);
            EmFatalis_SpawnEffect33Or08(em, 0, 0);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmFatalis_EndState(em);
        }
        break;
    }
}

void EmFatalis_State3_04(Em *em, s32 unused, s32 variant)
{
    s32 k;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0xF, 0, 0);
        break;
    case 1:
        k = variant & 0xFF;
        if (k != 0) {
            if (Player_PassedFrameScaled(em, 80.0f, 0U) != 0 || Player_PassedFrameScaled(em, 140.0f, 0U) != 0 || Player_PassedFrameScaled(em, 200.0f, 0U) != 0 ||
                Player_PassedFrameScaled(em, 260.0f, 0U) != 0 || Player_PassedFrameScaled(em, 320.0f, 0U) != 0) {
                EmFatalis_SpawnEffect33Or08b(em, D_008C9BCC[k][em->stateVar0]);
                em->stateVar0++;
            }
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmFatalis_EndState(em);
        }
        break;
    }
}

void EmFatalis_State3_05(Em *em, EmWork *work, s32 variant)
{
    f32 floorY;
    s32 k;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        work->turnSpeed = 0x100;
        Em_SetMotion(em, 0x1F, 0, 0);
        em->timer = 0x5A;
        break;
    case 1:
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 2);
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x10, 0, 0);
        }
        break;
    case 2:
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 2);
        if (Player_PassedFrameScaled(em, 30.0f, 0U) != 0) {
            EmFatalis_SpawnK1COrK11(em, 0, 0);
            k = variant & 0xFF;
            switch (k) {
            case 0:
                EmFatalis_SpawnEffect33Or08(em, 0x38E4, 0);
                break;
            case 1:
                EmFatalis_SpawnEffect33Or08(em, 0x2AAB, 0);
                break;
            case 2:
                EmFatalis_SpawnEffect33Or08(em, 0x1E94, 0);
                break;
            }
            work->countdown = (s8) work->countdown - 1;
        }
        if (((s8) work->countdown <= 0) && (Player_PassedFrameScaled(em, 76.0f, 0U) != 0)) {
            em->step++;
            Em_SetMotion(em, 0xA, 0xA, 0x1E);
        }
        if (em->motion[0].playing == 0) {
            Em_SetMotion(em, 0x10, 4, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmFatalis_EndState2(em, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmFatalis_State3_06(Em *em)
{
    em->guardTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 5, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 7, 0, 0);
            Eff04_Create(em, 2);
        }
        break;
    case 2:
        if (Em_StepToGoal(em, 0x40, 0) == 1) {
            em->step++;
            Em_SetMotion(em, 8, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmFatalis_EndState(em);
        }
        break;
    }
}

void EmFatalis_State3_07(Em *em)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x21, 0, 0);
        EmFatalis_StartCurves(em, 3);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 70.0f, 0U) != 0) {
            em->step++;
            em->stance = 2;
            EmFatalis_UpdateCurves(em);
        }
        break;
    case 2:
        if (Player_PassedFrameScaled(em, 94.0f, 0U) != 0) {
            EmFatalis_SpawnK1COrK11(em, 0, 0);
            EmFatalis_SpawnEffect33Or08(em, 0x38E4, 0);
        }
        EmFatalis_UpdateCurves(em);
        if (em->motion[0].playing == 0) {
            em->step++;
            EmFatalis_EndState2(em, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmFatalis_State3_08(Em *em)
{
    em->guardTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 5, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 8, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmFatalis_EndState(em);
        }
        break;
    }
}

void EmFatalis_State3_09(Em *em, EmWork *work)
{
    f32 floorY;
    s8 countdown;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x1F, 0, 0);
        em->timer = 0x5A;
        work->turnSpeed = 0x100;
        break;
    case 1:
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 2);
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x10, 0, 0);
        }
        break;
    case 2:
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 2);
        if (Player_PassedFrameScaled(em, 30.0f, 0U) != 0) {
            EmFatalis_SpawnK1COrK11(em, 0, 0);
            work->countdown = (s8) work->countdown - 1;
            countdown = (s8) work->countdown;
            switch (countdown) {
            case 2:
                EmFatalis_SpawnEffect33Or08(em, 0x38E4, 0);
                break;
            case 1:
                EmFatalis_SpawnEffect33Or08(em, 0x2AAB, 0);
                break;
            case 0:
                EmFatalis_SpawnEffect33Or08(em, 0x1E94, 0);
                break;
            }
        }
        if (((s8) work->countdown <= 0) && (Player_PassedFrameScaled(em, 76.0f, 0U) != 0)) {
            em->step++;
            Em_SetMotion(em, 0xA, 0xA, 0x1E);
        }
        if (em->motion[0].playing == 0) {
            Em_SetMotion(em, 0x10, 4, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmFatalis_EndState2(em, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmFatalis_State3_0C(Em *em)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x23, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stance = 2;
            Em_SetMotion(em, 0x28, 0, 0);
            EmFatalis_StartCurves(em, 4);
        }
        break;
    case 2:
        EmFatalis_UpdateCurves(em);
        em->goalDist -= em->velZ;
        if (em->goalDist <= 0.0f) {
            em->step++;
            em->stance = 0;
            em->posY = em->floorY;
            Em_SetMotion(em, 0x24, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 8, 0, 0);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmFatalis_EndState(em);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmFatalis_State3_0D(Em *em, EmWork *work)
{
    f32 v[4];
    f32 out[4];
    f32 mtx[16];

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        Em_SetMotion(em, 0x25, 0, 0);
        EmFatalis_StartCurves(em, 5);
        em->rotX = 0;
        break;
    case 1:
        em->rotX += 0x1A8;
        EmFatalis_UpdateCurves(em);
        ((EmFatalisWork *)work)->runDist -= em->velZ;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stance = 2;
            Em_SetMotion(em, 0x26, 0, 0);
            em->velX = 0.0f;
            em->velY = 0.0f;
            em->velZ = 80.0f;
            em->accelX = 0.0f;
            em->accelY = 0.0f;
            em->accelZ = 9.0f;
            Eff04_Create(em, 0x1E);
        }
        break;
    case 2:
        work->moveAngX = em->rotX;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotatedAccel(em, &work->moveAngX);
        ((EmFatalisWork *)work)->runDist -= em->velZ;
        if (em->posY <= 850.0f + em->floorY) {
            em->step++;
            Em_SetMotion(em, 0x27, 0, 0);
            em->accelZ = -4.0f;
        }
        break;
    case 3:
        if (EmMotion_ReachedFrame(em, 0, 15.0f) == 0) {
            em->rotX -= 0x6A2;
        }
        work->moveAngX = em->rotX;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotatedAccel(em, &work->moveAngX);
        ((EmFatalisWork *)work)->runDist -= em->velZ;
        if (em->posY < em->floorY) {
            work->moveAngX = 0;
            v[0] = (em->floorY - em->posY) / 3.0f;
            v[1] = 0.0f;
            v[2] = 0.0f;
            Mtx_FromAnglesYXZ(&work->moveAngX, mtx);
            Vec_Rotate(out, v, mtx);
            em->posX += out[0];
            em->posY += out[1];
            em->posZ += out[2];
        }
        em->posY -= 50.0f;
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x24, 0, 0);
            em->rotX = 0;
            em->stance = 0;
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 8, 0, 0);
        }
        break;
    case 5:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmFatalis_EndState(em);
        }
        break;
    }
    if (em->posY < em->floorY) {
        em->posY = em->floorY;
    }
}

void EmFatalis_State3_12(Em *em, s32 unused, s32 variant)
{
    s32 k;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0xF, 0, 0);
        break;
    case 1:
        k = variant & 0xFF;
        if (k != 0) {
            if (Player_PassedFrameScaled(em, 80.0f, 0U) != 0 || Player_PassedFrameScaled(em, 140.0f, 0U) != 0 || Player_PassedFrameScaled(em, 200.0f, 0U) != 0 ||
                Player_PassedFrameScaled(em, 260.0f, 0U) != 0 || Player_PassedFrameScaled(em, 320.0f, 0U) != 0) {
                EmFatalis_SpawnEffect33Or08b(em, D_008C9BCC[5][em->stateVar0]);
                em->stateVar0++;
            }
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0xF, 0, 0);
        }
        break;
    case 2:
        k = variant & 0xFF;
        if (k != 0) {
            if (Player_PassedFrameScaled(em, 80.0f, 0U) != 0 || Player_PassedFrameScaled(em, 140.0f, 0U) != 0 || Player_PassedFrameScaled(em, 200.0f, 0U) != 0 ||
                Player_PassedFrameScaled(em, 260.0f, 0U) != 0 || Player_PassedFrameScaled(em, 320.0f, 0U) != 0) {
                EmFatalis_SpawnEffect33Or08b(em, D_008C9BCC[5][em->stateVar0]);
                em->stateVar0++;
            }
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmFatalis_EndState(em);
        }
        break;
    }
}

void EmFatalis_State3_14(Em *em, EmWork *work)
{
    f32 v[4];
    f32 out[4];
    f32 mtx[16];

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        Em_SetMotion(em, 0x25, 0, 0);
        EmFatalis_StartCurves(em, 5);
        em->rotX = 0;
        break;
    case 1:
        em->rotX += 0x1A8;
        EmFatalis_UpdateCurves(em);
        ((EmFatalisWork *)work)->runDist -= em->velZ;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stance = 2;
            Em_SetMotion(em, 0x26, 0, 0);
            em->velX = 0.0f;
            em->velY = 0.0f;
            em->velZ = 80.0f;
            em->accelX = 0.0f;
            em->accelY = 0.0f;
            em->accelZ = 9.0f;
            Eff04_Create(em, 0x1E);
        }
        break;
    case 2:
        work->moveAngX = em->rotX;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotatedAccel(em, &work->moveAngX);
        ((EmFatalisWork *)work)->runDist -= em->velZ;
        if (em->posY <= 1050.0f) {
            em->step++;
            Em_SetMotion(em, 0x27, 0, 0);
            em->accelZ = -4.0f;
        }
        break;
    case 3:
        if (EmMotion_ReachedFrame(em, 0, 15.0f) == 0) {
            em->rotX -= 0x6A2;
        }
        work->moveAngX = em->rotX;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotatedAccel(em, &work->moveAngX);
        ((EmFatalisWork *)work)->runDist -= em->velZ;
        if (em->posY < em->floorY) {
            work->moveAngX = 0;
            v[0] = (em->floorY - em->posY) / 3.0f;
            v[1] = 0.0f;
            v[2] = 0.0f;
            Mtx_FromAnglesYXZ(&work->moveAngX, mtx);
            Vec_Rotate(out, v, mtx);
            em->posX += out[0];
            em->posY += out[1];
            em->posZ += out[2];
        }
        em->posY -= 50.0f;
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x24, 0, 0);
            em->rotX = 0;
            em->stance = 0;
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 8, 0, 0);
        }
        break;
    case 5:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmFatalis_EndState(em);
        }
        break;
    }
    if (em->posY < em->floorY) {
        em->posY = em->floorY;
    }
}

void EmFatalis_State3_15(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x20, 0, 0);
        Player_SetFlag((Player *) em, 0x20000U);
        em->timer = 10;
        break;
    case 1:
        if (em->stateVar0 < 8) {
            if (--em->timer <= 0) {
                EffEm33_Create(em, 0xE, D_008C9BCC[6 + (em->stateRand & 1)][em->stateVar0], 0, 0);
                em->stateVar0++;
                em->timer = 10;
            }
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x20, 0, 0);
            em->timer = 10;
        }
        break;
    case 2:
        if (em->stateVar0 < 16) {
            if (--em->timer <= 0) {
                EffEm33_Create(em, 0xE, D_008C9BCC[6 + (em->stateRand & 1)][em->stateVar0], 0, 0);
                em->stateVar0++;
                em->timer = 10;
            }
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x20, 0, 0);
            em->timer = 10;
        }
        break;
    case 3:
        if (em->stateVar0 < 24) {
            if (--em->timer <= 0) {
                EffEm33_Create(em, 0xE, D_008C9BCC[6 + (em->stateRand & 1)][em->stateVar0], 0, 0);
                em->stateVar0++;
                em->timer = 10;
            }
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmFatalis_ChangeState(em, 2, 0xF, 4);
        }
        break;
    case 4:
        if (em->remote == 0) {
            EmFatalis_ChangeState(em, 2, 0xF, 4);
        }
        break;
    }
}

void EmFatalis_State4_00(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x14, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmFatalis_EndState(em);
        }
        break;
    }
}

void EmFatalis_State4_01(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x15, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmFatalis_EndState(em);
        }
        break;
    }
}

void EmFatalis_State4_02(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x16, 0, 0);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 50.0f) == 0) {
            em->rotY += 0x28E;
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmFatalis_EndState(em);
        }
        break;
    }
}

void EmFatalis_State4_03(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x19, 0, 0);
        Em_RestartScript(em);
        if (em->velY > -50.0f) {
            em->velY = -50.0f;
        }
        em->accelY = -10.0f;
        break;
    case 1:
        Em_MoveRotatedAccel(em, &work->moveAngX);
        if (em->posY + 180.0f <= em->floorY) {
            em->step++;
            em->posY = em->floorY;
            em->stance = 0;
            Em_SetMotion(em, 0x1A, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 8, 4, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmFatalis_EndState(em);
        }
        break;
    }
}

void EmFatalis_State4_04(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x1B, 0, 0);
        em->timer = 120;
        Em_RestartScript(em);
        gScene.emEventFlags |= 2;
        em->hitReactMode = 3;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x1C, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->hitReactMode = 0;
            Em_SetMotion(em, 8, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmFatalis_EndState(em);
        }
        break;
    }
}

void EmFatalis_State4_05(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x22, 0, 0);
        em->noReact = 1;
        Em_RestartScript(em);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 296.0f, 0U) != 0) {
            em->step++;
            Em_SetMotion(em, 0x1B, 0xC, 0x124);
        }
        break;
    case 2:
        Em_SpawnEffect06Bone(em, 2);
        if (--em->timer <= 0) {
            em->step++;
            EmFatalis_ChangeState(em, 0, 6, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmFatalis_ChangeState(em, 0, 6, 4);
        }
        break;
    }
}

void EmFatalis_State4_06(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x17, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 50.0f) == 0) {
            em->rotY -= 0x28E;
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmFatalis_EndState(em);
        }
        break;
    }
}

void EmFatalis_State4_07(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x14, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmFatalis_EndState(em);
        }
        break;
    }
}

void EmFatalis_State4_08(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x19, 0, 0);
        if (em->velY > -50.0f) {
            em->velY = -50.0f;
        }
        em->accelY = -10.0f;
        Em_RestartScript(em);
        break;
    case 1:
        Em_MoveRotatedAccel(em, &work->moveAngX);
        if (em->posY + 180.0f <= em->floorY) {
            em->step++;
            em->posY = em->floorY;
            em->stance = 0;
            Em_SetMotion(em, 0x1A, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 8, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Player_ClearVelocity(em);
            EmFatalis_EndState(em);
        }
        break;
    }
}

void EmFatalis_State5_00_Hunted(Em *em, EmWork *work)
{
    em->noPushTimer = 5;
    em->combat = 0;
    switch (em->step) {
    case 0x0:
        em->step++;
        Em_SetMotion(em, 0x18, 0, 0);
        em->lookAtTarget = 0;
        em->stance = 3;
        QuestEm_Hunted(em);
        CamReq_BossHunted();
        break;
    case 0x1:
        if (em->motion[0].playing == 0) {
            em->timer = 0x1C2;
            em->step++;
            EmFatalis_AddMinePoints(em);
            em->fieldCollide = 0;
        }
        break;
    case 0x2:
        EmFatalis_UpdateMinePoints(em);
        break;
    case 0x3:
        if (--em->timer <= 0) {
            em->taskPhase++;
            em->visible = 0;
            EmFatalis_RemoveMinePoints(em);
            break;
        }
        em->alpha = (f32) em->timer / 150.0f;
        break;
    case 0x63:
        if (em->motion[0].playing == 0) {
            em->stance = 0;
            EmFatalis_EndState(em);
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
        }
        break;
    }
}

void EmFatalis_State5_01_Hunted(Em *em, EmWork *work)
{
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x1D, 0, 0);
        em->lookAtTarget = 0;
        em->stance = 3;
        gScene.emEventFlags |= 2;
        QuestEm_Hunted(em);
        CamReq_BossHunted();
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->timer = 0x1C2;
            em->step++;
            EmFatalis_AddMinePoints(em);
            em->fieldCollide = 0;
        }
        break;
    case 2:
        EmFatalis_UpdateMinePoints(em);
        break;
    case 3:
        if (--em->timer <= 0) {
            em->taskPhase++;
            em->visible = 0;
            EmFatalis_RemoveMinePoints(em);
            break;
        }
        em->alpha = (f32) em->timer / 150.0f;
        break;
    case 99:
        if (em->motion[0].playing == 0) {
            em->stance = 0;
            EmFatalis_EndState(em);
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
        }
        break;
    }
}

void EmFatalis_State5_02_HuntedFalling(Em *em, EmWork *work)
{
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x19, 0, 0);
        if (em->velY > -50.0f) {
            em->velY = -50.0f;
        }
        em->accelY = -10.0f;
        CamReq_BossHunted();
        break;
    case 1:
        Em_MoveRotatedAccel(em, &work->moveAngX);
        if (em->posY <= em->floorY) {
            em->step++;
            em->posY = em->floorY;
            em->stance = 0;
            Em_SetMotion(em, 0x1A, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    case 2:
        if (Player_PassedFrameScaled(em, 100.0f, 0) != 0) {
            em->step++;
            em->motionSpeed = 0.0f;
            em->timer = 0;
            em->stance = 3;
            QuestEm_Hunted(em);
            EmFatalis_AddMinePoints(em);
            em->fieldCollide = 0;
        }
        break;
    case 3:
        em->motionSpeed = 0.0f;
        EmFatalis_UpdateMinePoints(em);
        if (em->motion[0].playing == 0) {
            em->timer = 0x1C2;
            em->step++;
        }
        break;
    case 4:
        EmFatalis_UpdateMinePoints(em);
        break;
    case 5:
        if (--em->timer <= 0) {
            em->taskPhase++;
            em->visible = 0;
            EmFatalis_RemoveMinePoints(em);
            break;
        }
        em->alpha = (f32) em->timer / 150.0f;
        break;
    case 99:
        if (em->motion[0].playing == 0) {
            em->stance = 0;
            EmFatalis_EndState(em);
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
        }
        break;
    }
}

void EmFatalis_RunGroup0(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmFatalis_State0_00(em, work);
        break;
    case 1:
        EmFatalis_State0_01(em, work);
        break;
    case 2:
        EmFatalis_State0_02(em, work);
        break;
    case 3:
        EmFatalis_State0_03(em, work);
        break;
    case 4:
        EmFatalis_State0_04(em, work);
        break;
    case 5:
        EmFatalis_State0_05(em, work);
        break;
    case 6:
        EmFatalis_State0_06(em, work);
        
    default:
        break;
    }
}

void EmFatalis_RunGroup1(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmFatalis_State1_00(em, work);
        break;
    case 1:
        EmFatalis_State1_01(em, work);
        break;
    case 2:
        EmFatalis_State1_02(em, work);
        break;
    }
}

void EmFatalis_RunGroup2(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmFatalis_State2_00(em, work, 0);
        break;
    case 1:
        EmFatalis_State2_01(em, work);
        break;
    case 2:
        EmFatalis_State2_02(em, work);
        break;
    case 3:
        EmFatalis_State2_03(em, work, 0);
        break;
    case 4:
        EmFatalis_State2_04(em, work);
        break;
    case 5:
        EmFatalis_State2_05(em, work);
        break;
    case 6:
        EmFatalis_State2_06(em, work);
        break;
    case 7:
        EmFatalis_State2_07(em, work);
        break;
    case 8:
        EmFatalis_State2_08(em, work);
        break;
    case 9:
        EmFatalis_State2_09(em, work);
        break;
    case 10:
        EmFatalis_State2_0A(em, work, 0);
        break;
    case 11:
        EmFatalis_State2_0B(em, work);
        break;
    case 12:
        EmFatalis_State2_0A(em, work, 1);
        break;
    case 13:
        EmFatalis_State2_03(em, work, 1);
        break;
    case 14:
        EmFatalis_State2_0E(em, work);
        break;
    case 15:
        EmFatalis_State2_00(em, work, 1);
        
    default:
        break;
    }
}

void EmFatalis_RunGroup3(Em *em, s32 work)
{
    switch (em->state) {
    case 0:
        EmFatalis_State3_00(em, work, 0);
        break;
    case 1:
        EmFatalis_State3_01(em);
        break;
    case 2:
        EmFatalis_State3_02(em);
        break;
    case 3:
        EmFatalis_State3_03(em);
        break;
    case 4:
        EmFatalis_State3_04(em, work, 0);
        break;
    case 5:
        EmFatalis_State3_05(em, (EmWork *) work, 0);
        break;
    case 6:
        EmFatalis_State3_06(em);
        break;
    case 7:
        EmFatalis_State3_07(em);
        break;
    case 8:
        EmFatalis_State3_08(em);
        break;
    case 9:
        EmFatalis_State3_09(em, (EmWork *) work);
        break;
    case 10:
        EmFatalis_State3_05(em, (EmWork *) work, 1);
        break;
    case 11:
        EmFatalis_State3_05(em, (EmWork *) work, 2);
        break;
    case 12:
        EmFatalis_State3_0C(em);
        break;
    case 13:
        EmFatalis_State3_0D(em, (EmWork *) work);
        break;
    case 14:
        EmFatalis_State3_04(em, work, 1);
        break;
    case 15:
        EmFatalis_State3_04(em, work, 2);
        break;
    case 16:
        EmFatalis_State3_04(em, work, 3);
        break;
    case 17:
        EmFatalis_State3_04(em, work, 4);
        break;
    case 18:
        EmFatalis_State3_12(em, work, 4);
        break;
    case 19:
        EmFatalis_State3_00(em, work, 1);
        break;
    case 20:
        EmFatalis_State3_14(em, (EmWork *) work);
        break;
    case 21:
        EmFatalis_State3_15(em);
        
    default:
        break;
    }
}

void EmFatalis_RunGroup4(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmFatalis_State4_00(em, work);
        break;
    case 1:
        EmFatalis_State4_01(em, work);
        break;
    case 2:
        EmFatalis_State4_02(em, work);
        break;
    case 3:
        EmFatalis_State4_03(em, work);
        break;
    case 4:
        EmFatalis_State4_04(em, work);
        break;
    case 5:
        EmFatalis_State4_05(em, work);
        break;
    case 6:
        EmFatalis_State4_06(em, work);
        break;
    case 7:
        EmFatalis_State4_07(em, work);
        break;
    case 8:
        EmFatalis_State4_08(em, work);
        
    default:
        break;
    }
}

void EmFatalis_RunGroup5(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmFatalis_State5_00_Hunted(em, work);
        break;
    case 1:
        EmFatalis_State5_01_Hunted(em, work);
        break;
    case 2:
        EmFatalis_State5_02_HuntedFalling(em, work);
        break;
    }
}

void EmFatalis_State6_00(Em *em, EmWork *work)
{

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x14, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x20, 0, 0);
            break;
        }
    default:
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 9, 0, 0);
            EmFatalis_StartCurves(em, 0U);
        }
        break;
    case 3:
        if (Player_PassedFrameScaled(em, 134.0f, 0U) != 0) {
            em->step++;
            em->stance = 2;
            EmFatalis_UpdateCurves(em);
        }
        break;
    case 4:
        if (EmFatalis_UpdateCurves(em) != 0) {
            em->step++;
            Em_SetMotion(em, 0xA, 0, 0);
            work->flyGuard = 0;
        }
        break;
    case 5:
        EmFatalis_MoveByVelYCurve(em, 1);
        em->posY += 10.0f;
        if (!(em->posY < 7500.0f)) {
            em->step++;
        }
        break;
    }
}

void EmFatalis_State6_01(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        GmEm_ResetSize(em);
        em->step++;
        em->stance = 0;
        em->posX = 17300.0f;
        em->posY = 0.0f;
        em->posZ = 31370.0f;
        em->goal[0] = 11980.0f;
        em->goal[1] = 5860.0f;
        em->goal[2] = 15200.0f;
        em->rotX = 0;
        em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        em->rotZ = 0;
        em->goal[0] = 17300.0f;
        em->goal[1] = 5860.0f;
        em->goal[2] = 31370.0f;
        Em_SetMotion(em, 1, 0, 0);
        em->timer = 150;
        Player_ClearVelocity(em);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 9, 0, 0);
            EmFatalis_StartCurves(em, 0);
            work->flyGuard = 0;
        }
        break;
    case 2:
        if (Player_PassedFrameScaled(em, 134.0f, 0U) != 0) {
            em->step++;
            em->stance = 2;
            EmFatalis_UpdateCurves(em);
        }
        break;
    case 3:
        if (EmFatalis_UpdateCurves(em) != 0) {
            em->step++;
            Em_SetMotion(em, 0xA, 0, 0);
            work->moveAngX = 0;
            work->moveAngY = em->rotY;
            work->moveAngZ = 0;
        }
        break;
    case 4:
        EmFatalis_MoveByVelYCurve(em, 1);
        em->posY += 20.0f;
        if (em->posY >= em->goal[1]) {
            em->step++;
            work->turnSpeed = 0x100;
            em->goal[0] = 11980.0f;
            em->goal[1] = 5860.0f;
            em->goal[2] = 15200.0f;
            Player_ClearVelocity(em);
            em->velZ = 50.0f;
        }
        break;
    case 5:
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 1);
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_GetPitchToGoal(em, &work->moveAngX, 0.0f, 0.0f);
        Em_MoveRotated(em, &work->moveAngX);
        if (Vec_DistanceXZ(&em->posX, em->goal) <= 10.0f * em->velZ) {
            em->step++;
            Player_ClearVelocity(em);
            em->velY = -10.0f;
        }
        break;
    case 6:
        work->moveAngY = em->rotY;
        Em_MoveRotated(em, &work->moveAngX);
        if (700.0f + em->floorY > em->posY) {
            em->step++;
            em->velY = -20.0f;
        }
        break;
    case 7:
        work->moveAngY = em->rotY;
        Em_MoveRotated(em, &work->moveAngX);
        if (em->posY <= em->floorY) {
            em->step++;
            Em_SetMotion(em, 0xC, 0, 0);
            em->posY = em->floorY;
            em->rotX = 0;
            em->rotZ = 0;
            em->stance = 0;
            CamShake_ReqAIf(em, 1);
        }
        break;
    case 8:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 8, 0, 0);
        }
        break;
    case 9:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x20, 0, 0);
        }
        break;
    case 10:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0xD, 0, 0);
        }
        break;
    case 11:
        if (Player_PassedFrameScaled(em, 102.0f, 0U) != 0) {
            EmFatalis_SpawnK1COrK11(em, 0, 0);
            EmFatalis_SpawnEffect33Or08(em, 0, 0);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 1, 0, 0);
        }
        break;
    case 12:
        if (Save_TestEventFlag(0xB0) == 1) {
            em->step++;
            EmFatalis_EndState(em);
        }
        break;
    }
}

void EmFatalis_State6_02(Em *em)
{

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x1F, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x10, 0, 0);
        }
        break;
    case 2:
        if (Player_PassedFrameScaled(em, 30.0f, 0U) != 0) {
            EmFatalis_SpawnK1COrK11(em, 0, 0);
            EmFatalis_SpawnEffect33Or08(em, 0x38E4, 0);
        }
        if (Player_PassedFrameScaled(em, 76.0f, 0U) != 0) {
            em->step++;
            Em_SetMotion(em, 0xA, 0xA, 0x1E);
        }
        break;
    case 3:
        EmFatalis_MoveByVelYCurve(em, 1);
        em->posY += 10.0f;
        if (!(em->posY < 7500.0f)) {
            em->step++;
        }
        break;
    }
}

void EmFatalis_State6_03(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        GmEm_ResetSize(em);
        em->step++;
        em->stance = 0;
        em->posX = 15779.0f;
        em->posY = 571.0f;
        em->posZ = 3495.0f;
        em->goal[0] = 9896.0f;
        em->goal[1] = 46.0f;
        em->goal[2] = 15107.0f;
        em->rotX = 0;
        em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        em->rotZ = 0;
        Em_SetMotion(em, 0x1D, 0, 0x27E);
        em->timer = 90;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x1C, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 8, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 1, 0x12, 0);
            em->timer = 30;
        }
        break;
    case 4:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x1E, 0, 0);
        }
        break;
    case 5:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 1, 0, 0);
            em->timer = 180;
        }
        break;
    case 6:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x20, 0, 0);
            em->rage = 1;
            em->rageTimer = 1800;
        }
        break;
    case 7:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x23, 0, 0);
        }
        break;
    case 8:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stance = 2;
            Em_SetMotion(em, 0x28, 0, 0);
            EmFatalis_StartCurves(em, 4);
            em->rage = 0;
            em->rageTimer = 0;
        }
        break;
    case 9:
        EmFatalis_UpdateCurves(em);
        if (Vec_DistanceXZ(&em->posX, em->goal) <= 1000.0f) {
            em->step++;
            em->posX = 12021.0f;
            em->posY = 100.0f;
            em->posZ = 7469.0f;
            em->rotX = 0;
            em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
            em->rotZ = 0;
        }
        break;
    case 10:
        EmFatalis_UpdateCurves(em);
        if (Vec_DistanceXZ(&em->posX, em->goal) <= 1000.0f) {
            em->step++;
            em->stance = 0;
            em->posY = em->floorY;
            Em_SetMotion(em, 0x24, 0, 0);
        }
        break;
    case 11:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 1, 0, 0);
        }
        break;
    case 12:
        if (Save_TestEventFlag(0xB1) == 1) {
            em->step++;
            EmFatalis_EndState(em);
        }
        break;
    }
}

void EmFatalis_State6_04(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        GmEm_ResetSize(em);
        em->step++;
        em->stance = 2;
        em->posX = 3038.0f;
        em->posY = 4680.0f;
        em->posZ = 5782.0f;
        em->goal[0] = 14022.0f;
        em->goal[1] = 4680.0f;
        em->goal[2] = 22144.0f;
        em->rotX = 0;
        em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        em->rotZ = 0;
        em->goalDist = Vec_DistanceXZ(&em->posX, em->goal);
        Em_SetMotion(em, 0x28, 0, 0);
        EmFatalis_StartCurves(em, 4);
        em->timer = 150;
        em->visible = 0;
        em->hideShadowHold = 2;
        em->hideShadow = 2;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            em->timer = 150;
            em->visible = 1;
            em->hideShadowHold = 0;
            em->hideShadow = 0;
        }
        break;
    case 2:
        EmFatalis_UpdateCurves(em);
        em->goalDist -= em->velZ;
        if (--em->timer <= 0 || em->goalDist <= 0.0f) {
            em->step++;
            em->posX = 10463.0f;
            em->posY = 2000.0f;
            em->posZ = 10184.0f;
            em->rotY = 0x4000;
            work->flyGuard = 0;
            work->moveAngX = 0;
            work->moveAngZ = 0;
            Player_ClearVelocity(em);
            em->velY = -10.0f;
            Em_SetMotionRow(em, 0xA, 0, 0);
            em->timer = 150;
        }
        break;
    case 3:
        Em_MoveRotated(em, &work->moveAngX);
        if (--em->timer <= 0 || em->posY < 1500.0f + em->floorY) {
            em->step++;
            Em_SetMotion(em, 0xB, 0, 0);
            em->timer = 150;
        }
        break;
    case 4:
        Em_MoveRotated(em, &work->moveAngX);
        if (--em->timer <= 0 || em->posY < em->floorY) {
            em->step++;
            em->posY = em->floorY;
            Em_SetMotion(em, 0xC, 0, 0);
            Player_ClearVelocity(em);
            em->timer = 90;
        }
        break;
    case 5:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotionRow(em, 1, 0, 0);
            em->timer = 150;
        }
        break;
    case 6:
        if (EmMotion_ReachedFrame(em, 0, 200.0f) != 0) {
            em->step++;
            em->rage = 1;
            em->rageTimer = 1800;
            Em_SetMotionRow(em, 0x1E, 0, 0);
            em->timer = 150;
        }
        break;
    case 7:
        if (--em->timer <= 0 || em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotionRow(em, 0x20, 0, 0);
        }
        break;
    case 8:
        if ((em->motionId[0] == 0x408 && EmMotion_ReachedFrame(em, 0, 60.0f) != 0) || em->motionId[0] != 0x408) {
            if (em->stateVar1++ % 10 == 0) {
                EffEm33_CreatePlus18(em, 0xC, em->stateVar0++, 0, 0);
            }
        }
        if (em->motion[0].playing == 0) {
            Em_SetMotionRow(em, 1, 0, 0);
        }
        if (em->stateVar0 >= 15) {
            em->step++;
            em->rage = 0;
            em->rageTimer = 0;
        }
        break;
    case 9:
        if (em->motion[0].playing == 0) {
            em->step++;
        }
        break;
    case 10:
        if (Save_TestEventFlag(0xB2) == 1) {
            em->step++;
            EmFatalis_EndState(em);
        }
        break;
    }
}

void EmFatalis_RunGroup6(Em *em, s32 work)
{
    em->motionSpeed = 1.0f;
    switch (em->state) {
    case 0:
        EmFatalis_State6_00(em, (EmWork *) work);
        break;
    case 1:
        EmFatalis_State6_01(em, (EmWork *) work);
        break;
    case 2:
        EmFatalis_State6_02(em);
        break;
    case 3:
        EmFatalis_State6_03(em, (EmWork *) work);
        break;
    case 4:
        EmFatalis_State6_04(em, (EmWork *) work);
        break;
    }
}

void EmFatalis_RunState(Em *em, s32 work)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->stateGroup) {
    case 0:
        EmFatalis_RunGroup0(em, (EmWork *) work);
        break;
    case 1:
        EmFatalis_RunGroup1(em, (EmWork *) work);
        break;
    case 2:
        EmFatalis_RunGroup2(em, (EmWork *) work);
        break;
    case 3:
        EmFatalis_RunGroup3(em, work);
        break;
    case 4:
        EmFatalis_RunGroup4(em, (EmWork *) work);
        break;
    case 5:
        EmFatalis_RunGroup5(em, (EmWork *) work);
        break;
    case 6:
        EmFatalis_RunGroup6(em, work);
        break;
    case 7:
        EmFatalis_RunGroup6(em, work);
        
    default:
        break;
    }
}

void EmFatalis_SpawnK14Type1A(Em *em)
{
    if (em->type != 0x47 && em->rage != 0 && !(gScene.frame & 3)) {
        StageObj_K14_Create(em, 0x1A, (s16) ((u16) Rand_Next(1) & 1), 1.0f);
    }
}

void EmFatalis_SpawnK1CInMotions(Em *em, EmWork *work)
{
    if (em->type == 0x47 && em->rage != 0) {
        switch (work->sndMotion) {
        case 0x3E9:
        case 0x3EA:
        case 0x3EB:
        case 0x3EC:
        case 0x3ED:
        case 0x3EE:
        case 0x3F0:
        case 0x3F1:
        case 0x3F2:
        case 0x3F3:
        case 0x3F4:
        case 0x3F7:
        case 0x3FB:
        case 0x406:
        case 0x407:
        case 0x40B:
        case 0x40C:
        case 0x40D:
        case 0x40E:
            if (gScene.frame % 30 == 0) {
                StageObj_K1C_CreateOnEm(em, 0x28, 1.0f);
            }
            break;
        }
    }
}

static void EmFatalis_MotionEffectsDefault(Em *em)
{
}

void EmFatalis_MotionEffects(Em *em, EmFatalisFxWork *work)
{
    u8 stateGroup;
    u8 type;

    if (em->motionId[0] != work->motion) {
        work->motion = em->motionId[0];
    }
    EmFatalis_MotionSounds((s32) em, (EmWork *) work);
    EmFatalis_SpawnK1CInMotions(em, (EmWork *) work);
    switch (work->motion) {
    case 0x3E9:
        if (EmMotion_InFrameRange(em, 0, 28.0f, 80.0f) == 0) {
            if (EmMotion_InFrameRange(em, 0, 278.0f, 490.0f) != 0) {
                goto block_8;
            }
            break;
        }
block_8:
        EmFatalis_SpawnK14Type1A(em);
        break;
    case 0x3EA:
        EmFatalis_RequestOnFrame(em, 0xE, 2);
        EmFatalis_RequestOnFrame(em, 0x54, 2);
        if (Player_PassedFrameScaled(em, 2.0f, 0) != 0) {
            Eff04_Create(em, 5);
        }
        if (Player_PassedFrameScaled(em, 54.0f, 0) != 0) {
            Eff04_Create(em, 6);
        }
        if (Player_PassedFrameScaled(em, 130.0f, 0) != 0) {
            Eff04_Create(em, 7);
        }
        if (Player_PassedFrameScaled(em, 116.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0xA, 0x14, 5.0f);
        }
        if (Player_PassedFrameScaled(em, 46.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 6, 0x14, 5.0f);
        }
        if (EmMotion_InFrameRange(em, 0, 34.0f, 52.0f) || EmMotion_InFrameRange(em, 0, 108.0f, 130.0f)) {
            EmFatalis_SpawnK14Type1A(em);
        }
        break;
    case 0x3EB:
        EmFatalis_RequestOnFrame(em, 0x1A, 2);
        EmFatalis_RequestOnFrame(em, 0x60, 2);
        if (Player_PassedFrameScaled(em, 4.0f, 0) != 0) {
            Eff04_Create(em, 8);
        }
        if (Player_PassedFrameScaled(em, 56.0f, 0) != 0) {
            Eff04_Create(em, 9);
        }
        if (Player_PassedFrameScaled(em, 60.0f, 0) != 0) {
            Eff04_Create(em, 0x17);
        }
        if (Player_PassedFrameScaled(em, 20.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 6, 7, 1.2f);
        }
        if (Player_PassedFrameScaled(em, 94.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0xA, 7, 1.2f);
        }
        if (EmMotion_InFrameRange(em, 0, 6.0f, 24.0f) != 0) {
            EmFatalis_SpawnK14Type1A(em);
        }
        if (EmMotion_InFrameRange(em, 0, 46.0f, 70.0f) || EmMotion_InFrameRange(em, 0, 110.0f, 162.0f)) {
            EmFatalis_SpawnK14Type1A(em);
        }
        break;
    case 0x3EC:
        EmFatalis_RequestOnFrame(em, 0x20, 2);
        EmFatalis_RequestOnFrame(em, 0x5E, 2);
        if (Player_PassedFrameScaled(em, 4.0f, 0) != 0) {
            Eff04_Create(em, 0xA);
        }
        if (Player_PassedFrameScaled(em, 56.0f, 0) != 0) {
            Eff04_Create(em, 0xB);
        }
        if (Player_PassedFrameScaled(em, 60.0f, 0) != 0) {
            Eff04_Create(em, 0x17);
        }
        if (Player_PassedFrameScaled(em, 20.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0xA, 7, 1.2f);
        }
        if (Player_PassedFrameScaled(em, 94.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 6, 7, 1.2f);
        }
        if (EmMotion_InFrameRange(em, 0, 6.0f, 24.0f) != 0) {
            EmFatalis_SpawnK14Type1A(em);
        }
        if (EmMotion_InFrameRange(em, 0, 46.0f, 70.0f) || EmMotion_InFrameRange(em, 0, 110.0f, 162.0f)) {
            EmFatalis_SpawnK14Type1A(em);
        }
        break;
    case 0x3ED:
        EmFatalis_RequestOnFrame(em, 0x10, 2);
        EmFatalis_RequestOnFrame(em, 0x1C, 2);
        EmFatalis_RequestOnFrame(em, 0x2C, 2);
        EmFatalis_RequestOnFrame(em, 0x70, 5);
        if (Player_PassedFrameScaled(em, 38.0f, 0) != 0) {
            if (em->type == 0x47) {
                Eff04_Create(em, 0x23);
            } else {
                Eff04_Create(em, 0xC);
            }
        }
        if (Player_PassedFrameScaled(em, 36.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x1E, 7, 1.2f);
        }
        if (Player_PassedFrameScaled(em, 38.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x19, 7, 1.2f);
        }
        if (EmMotion_InFrameRange(em, 0, 66.0f, 112.0f) != 0) {
            EmFatalis_SpawnK14Type1A(em);
        }
        break;
    case 0x3EF:
        EmFatalis_RequestOnFrame(em, 0x1A, 2);
        EmFatalis_RequestOnFrame(em, 0x26, 2);
        EmFatalis_RequestOnFrame(em, 0x3C, 2);
        EmFatalis_RequestOnFrame(em, 0x58, 2);
        EmFatalis_RequestOnFrame(em, 0x7E, 2);
        EmFatalis_RequestOnFrame(em, 0x84, 2);
        if ((Player_PassedFrameScaled(em, 24.0f, 0) != 0) || (Player_PassedFrameScaled(em, 28.0f, 0) != 0) || (Player_PassedFrameScaled(em, 86.0f, 0) != 0) || (Player_PassedFrameScaled(em, 90.0f, 0) != 0)) {
            Effect0D_SpawnForEmScaled(em, 0x19, 3, 2.2f);
        }
        if ((Player_PassedFrameScaled(em, 36.0f, 0) != 0) || (Player_PassedFrameScaled(em, 40.0f, 0) != 0) || (Player_PassedFrameScaled(em, 96.0f, 0) != 0) || (Player_PassedFrameScaled(em, 100.0f, 0) != 0)) {
            Effect0D_SpawnForEmScaled(em, 0xA, 3, 2.2f);
        }
        if ((Player_PassedFrameScaled(em, 58.0f, 0) != 0) || (Player_PassedFrameScaled(em, 62.0f, 0) != 0) || (Player_PassedFrameScaled(em, 126.0f, 0) != 0) || (Player_PassedFrameScaled(em, 130.0f, 0) != 0)) {
            Effect0D_SpawnForEmScaled(em, 0x1E, 3, 2.2f);
        }
        if ((Player_PassedFrameScaled(em, 66.0f, 0) != 0) || (Player_PassedFrameScaled(em, 70.0f, 0) != 0) || (Player_PassedFrameScaled(em, 130.0f, 0) != 0) || (Player_PassedFrameScaled(em, 134.0f, 0) != 0)) {
            Effect0D_SpawnForEmScaled(em, 6, 3, 2.2f);
        }
        if (EmMotion_InFrameRange(em, 0, 12.0f, 40.0f) || EmMotion_InFrameRange(em, 0, 76.0f, 106.0f)) {
            EmFatalis_SpawnK14Type1A(em);
        }
        break;
    case 0x3EE:
        if (Player_PassedFrameScaled(em, 32.0f, 0) != 0) {
            Eff04_Create(em, 0x21);
        }
        if (Player_PassedFrameScaled(em, 114.0f, 0) != 0) {
            Eff04_Create(em, 0x22);
        }
        break;
    case 0x3F0:
        EmFatalis_RequestOnFrame(em, 0x5A, 2);
        if (EmMotion_InFrameRange(em, 0, 20.0f, 70.0f) || EmMotion_InFrameRange(em, 0, 84.0f, 140.0f)) {
            EmFatalis_SpawnK14Type1A(em);
        }
        break;
    case 0x3F1:
        if (Player_PassedFrameScaled(em, 196.0f, 0) || Player_PassedFrameScaled(em, 250.0f, 0) || Player_PassedFrameScaled(em, 316.0f, 0)) {
            Eff04_Create(em, 0x18);
        }
        if (((Player_PassedFrameScaled(em, 38.0f, 0) != 0) || (Player_PassedFrameScaled(em, 90.0f, 0) != 0)) && (em->posY <= (1000.0f + em->floorY))) {
            StageObj_K14_Create(em, 1, 0, 1.0f);
        }
        if (Player_PassedFrameScaled(em, 138.0f, 0) || Player_PassedFrameScaled(em, 196.0f, 0) || Player_PassedFrameScaled(em, 256.0f, 0) || Player_PassedFrameScaled(em, 322.0f, 0) || Player_PassedFrameScaled(em, 394.0f, 0)) {
            if (em->posY <= (1000.0f + em->floorY)) {
                StageObj_K14_Create(em, 1, 0, 1.2f);
            }
        }
        if (EmMotion_InFrameRange(em, 0, 6.0f, 34.0f) || EmMotion_InFrameRange(em, 0, 84.0f, 92.0f) || EmMotion_InFrameRange(em, 0, 124.0f, 130.0f) || EmMotion_InFrameRange(em, 0, 178.0f, 198.0f) || EmMotion_InFrameRange(em, 0, 252.0f, 284.0f) || EmMotion_InFrameRange(em, 0, 312.0f, 328.0f)) {
            EmFatalis_SpawnK14Type1A(em);
        }
        break;
    case 0x3F2:
        if (Player_PassedFrameScaled(em, 32.0f, 0) != 0) {
            Eff04_Create(em, 0x19);
        }
        if (((Player_PassedFrameScaled(em, 40.0f, 0) != 0) || (Player_PassedFrameScaled(em, 110.0f, 0) != 0)) && (em->posY <= (1000.0f + em->floorY))) {
            StageObj_K14_Create(em, 1, 0, 1.2f);
        }
        if (EmMotion_InFrameRange(em, 0, 44.0f, 68.0f) != 0) {
            EmFatalis_SpawnK14Type1A(em);
        }
        break;
    case 0x3F3:
        if ((Player_PassedFrameScaled(em, 122.0f, 0) != 0) || (Player_PassedFrameScaled(em, 164.0f, 0) != 0)) {
            Eff04_Create(em, 0x1A);
        }
        if (Player_PassedFrameScaled(em, 42.0f, 0) || Player_PassedFrameScaled(em, 102.0f, 0) || Player_PassedFrameScaled(em, 156.0f, 0)) {
            if (em->posY <= (1000.0f + em->floorY)) {
                StageObj_K14_Create(em, 1, 0, 1.2f);
                break;
            }
        }
        break;
    case 0x3F4:
        EmFatalis_RequestOnFrame(em, 0x12, 2);
        EmFatalis_RequestOnFrame(em, 0x28, 2);
        EmFatalis_RequestOnFrame(em, 0x86, 5);
        if (Player_PassedFrameScaled(em, 26.0f, 0) != 0) {
            Eff04_Create(em, 0xD);
        }
        if (Player_PassedFrameScaled(em, 40.0f, 0) != 0) {
            Eff04_Create(em, 0xE);
        }
        if (Player_PassedFrameScaled(em, 42.0f, 0) != 0) {
            StageObj_K14_Create(em, 1, 0, 1.0f);
        }
        if (Player_PassedFrameScaled(em, 190.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x19, 7, 1.2f);
        }
        if (EmMotion_InFrameRange(em, 0, 68.0f, 118.0f) || EmMotion_InFrameRange(em, 0, 160.0f, 252.0f) || EmMotion_InFrameRange(em, 0, 298.0f, 330.0f)) {
            EmFatalis_SpawnK14Type1A(em);
        }
        break;
    case 0x3F5:
        if (EmMotion_InFrameRange(em, 0, 10.0f, 32.0f) || EmMotion_InFrameRange(em, 0, 54.0f, 72.0f) || EmMotion_InFrameRange(em, 0, 158.0f, 212.0f)) {
            if (!(gScene.frame & 3)) {
                StageObj_K14_Create(em, 0x1A, (u16) Rand_Next(1) & 1, 1.0f);
            }
        }
        if (Player_PassedFrameScaled(em, 80.0f, 0) != 0) {
            Eff04_Create(em, 0x11);
        }
        break;
    case 0x3F6:
        if (EmMotion_InFrameRange(em, 0, 10.0f, 32.0f) || EmMotion_InFrameRange(em, 0, 54.0f, 72.0f) || EmMotion_InFrameRange(em, 0, 158.0f, 212.0f)) {
            if (!(gScene.frame & 3)) {
                StageObj_K14_Create(em, 0x1A, (u16) Rand_Next(1) & 1, 1.0f);
            }
        }
        if (Player_PassedFrameScaled(em, 80.0f, 0) != 0) {
            Eff04_Create(em, 0x11);
        }
        break;
    case 0x3F7:
        EmFatalis_RequestOnFrame(em, 0x26, 2);
        EmFatalis_RequestOnFrame(em, 0x56, 2);
        EmFatalis_RequestOnFrame(em, 0xF4, 2);
        EmFatalis_RequestOnFrame(em, 0x124, 2);
        if (Player_PassedFrameScaled(em, 80.0f, 0) != 0) {
            Eff04_Create(em, 3);
        }
        if (Player_PassedFrameScaled(em, 70.0f, 0) != 0) {
            Eff04_Create(em, 0x16);
        }
        break;
    case 0x3F8:
        if ((EmMotion_InFrameRange(em, 0, 98.0f, 130.0f) != 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create(em, 0x1A, (u16) Rand_Next(1) & 1, 1.0f);
        }
        if (Player_PassedFrameScaled(em, 2.0f, 0) != 0) {
            Eff04_Create(em, 0x12);
        }
        if ((Player_PassedFrameScaled(em, 34.0f, 0) != 0) || (Player_PassedFrameScaled(em, 112.0f, 0) != 0)) {
            Eff04_Create(em, 0x14);
        }
        break;
    case 0x3FA:
        EmFatalis_RequestOnFrame(em, 0x14, 2);
        EmFatalis_RequestOnFrame(em, 0x92, 2);
        if (em->type == 2) {
            if (Player_PassedFrameScaled(em, 48.0f, 0) != 0) {
                Eff04_Create(em, 4);
                break;
            }
        } else {
            if (Player_PassedFrameScaled(em, 38.0f, 0) != 0) {
                Eff04_Create(em, 0x1C);
            }
            if (Player_PassedFrameScaled(em, 34.0f, 0) != 0) {
                Eff04_Create(em, 0x1D);
            }
            if ((em->type == 0x47) && (Player_PassedFrameScaled(em, 34.0f, 0) != 0)) {
                StageObj_K1C_CreateOnEmC(em, 0x28, 1, 0.4f);
            }
            if (Player_PassedFrameScaled(em, 46.0f, 0) != 0) {
                StageObj_K14_Create(em, 0xA, 0x8B, 0.3f);
            }
            if (Player_PassedFrameScaled(em, 52.0f, 0) != 0) {
                StageObj_K14_Create(em, 0xA, 0x8B, 0.3f);
            }
            if (Player_PassedFrameScaled(em, 58.0f, 0) != 0) {
                StageObj_K14_Create(em, 0xA, 0x8B, 0.3f);
                break;
            }
        }
        break;
    case 0x411:
        EmFatalis_RequestOnFrame(em, 0x14, 2);
        EmFatalis_RequestOnFrame(em, 0x92, 2);
        type = em->type;
        if (type == 2) {
            if (Player_PassedFrameScaled(em, 48.0f, 0) != 0) {
                Eff04_Create(em, 4);
                break;
            }
        } else {
            if (type == 0x24) {
                if (Player_PassedFrameScaled(em, 38.0f, 0) != 0) {
                    Eff04_Create(em, 0x1C);
                }
                if (Player_PassedFrameScaled(em, 34.0f, 0) != 0) {
                    Eff04_Create(em, 0x1D);
                }
            } else {
                if (Player_PassedFrameScaled(em, 38.0f, 0) != 0) {
                    Eff04_Create(em, 0x1F);
                }
                if (Player_PassedFrameScaled(em, 34.0f, 0) != 0) {
                    Eff04_Create(em, 0x20);
                }
            }
            if ((em->type == 0x47) && (Player_PassedFrameScaled(em, 34.0f, 0) != 0)) {
                StageObj_K1C_CreateOnEmC(em, 0x28, 2, 0.4f);
            }
            if (Player_PassedFrameScaled(em, 46.0f, 0) != 0) {
                StageObj_K14_Create(em, 0xA, 0xB0, 0.3f);
            }
            if (Player_PassedFrameScaled(em, 52.0f, 0) != 0) {
                StageObj_K14_Create(em, 0xA, 0xB0, 0.3f);
            }
            if (Player_PassedFrameScaled(em, 58.0f, 0) != 0) {
                StageObj_K14_Create(em, 0xA, 0xB0, 0.3f);
                break;
            }
        }
        break;
    case 0x3FB:
        EmFatalis_RequestOnFrame(em, 0x44, 2);
        EmFatalis_RequestOnFrame(em, 0x118, 2);
        if (Player_PassedFrameScaled(em, 60.0f, 0) != 0) {
            Eff04_Create(em, 1);
        }
        break;
    case 0x3FC:
        EmFatalis_RequestOnFrame(em, 0x12, 2);
        EmFatalis_RequestOnFrame(em, 0xBE, 2);
        break;
    case 0x3FD:
        EmFatalis_RequestOnFrame(em, 0x1A, 2);
        EmFatalis_RequestOnFrame(em, 0x2E, 2);
        EmFatalis_RequestOnFrame(em, 0x42, 2);
        EmFatalis_RequestOnFrame(em, 0xC2, 2);
        EmFatalis_RequestOnFrame(em, 0x120, 2);
        if (Player_PassedFrameScaled(em, 64.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x1E, 7, 1.2f);
        }
        if (Player_PassedFrameScaled(em, 66.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x19, 7, 1.2f);
        }
        break;
    case 0x3FE:
        EmFatalis_RequestOnFrame(em, 0x20, 2);
        EmFatalis_RequestOnFrame(em, 0x3A, 2);
        EmFatalis_RequestOnFrame(em, 0xAA, 2);
        if (Player_PassedFrameScaled(em, 52.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x19, 7, 1.2f);
        }
        if (Player_PassedFrameScaled(em, 58.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x1E, 7, 1.2f);
        }
        if (Player_PassedFrameScaled(em, 162.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0xA, 7, 1.2f);
        }
        break;
    case 0x3FF:
        EmFatalis_RequestOnFrame(em, 0x20, 2);
        EmFatalis_RequestOnFrame(em, 0x44, 2);
        EmFatalis_RequestOnFrame(em, 0xA0, 2);
        if (Player_PassedFrameScaled(em, 52.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x1E, 7, 1.2f);
        }
        if (Player_PassedFrameScaled(em, 58.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x19, 7, 1.2f);
        }
        if (Player_PassedFrameScaled(em, 162.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 6, 7, 1.2f);
        }
        break;
    case 0x400:
        EmFatalis_RequestOnFrame(em, 0x14, 2);
        EmFatalis_RequestOnFrame(em, 0x82, 2);
        EmFatalis_RequestOnFrame(em, 0x11C, 2);
        EmFatalis_RequestOnFrame(em, 0x140, 2);
        EmFatalis_RequestOnFrame(em, 0x15E, 2);
        EmFatalis_RequestOnFrame(em, 0x238, 2);
        EmFatalis_RequestOnFrame(em, 0x2AC, 2);
        if (Player_PassedFrameScaled(em, 314.0f, 0) != 0) {
            StageObj_K14_Create(em, 0, 0, 1.2f);
        }
        if (Player_PassedFrameScaled(em, 418.0f, 0) != 0) {
            StageObj_K14_Create(em, 1, 0, 0.9f);
        }
        break;
    case 0x402:
        EmFatalis_RequestOnFrame(em, 0xC, 2);
        EmFatalis_RequestOnFrame(em, 0x20, 2);
        if (Player_PassedFrameScaled(em, 10.0f, 0) != 0) {
            StageObj_K14_Create(em, 0, 1, 1.2f);
        }
        break;
    case 0x403:
        EmFatalis_RequestOnFrame(em, 0x2E, 5);
        if (Player_PassedFrameScaled(em, 46.0f, 0) || Player_PassedFrameScaled(em, 112.0f, 0) || Player_PassedFrameScaled(em, 320.0f, 0)) {
            StageObj_K14_Create(em, 1, 0x80, 0.6f);
        }
        break;
    case 0x404:
        EmFatalis_RequestOnFrame(em, 0xDC, 2);
        if (Player_PassedFrameScaled(em, 216.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x1E, 7, 1.2f);
            Effect0D_SpawnForEmScaled(em, 0x19, 7, 1.2f);
        }
        break;
    case 0x406:
        if (EmMotion_InFrameRange(em, 0, 190.0f, 198.0f) != 0) {
            EmFatalis_SpawnK14Type1A(em);
        }
        break;
    case 0x407:
        if (EmMotion_InFrameRange(em, 0, 10.0f, 70.0f) || EmMotion_InFrameRange(em, 0, 114.0f, 122.0f)) {
            EmFatalis_SpawnK14Type1A(em);
        }
        if ((Player_PassedFrameScaled(em, 36.0f, 0) != 0) || (Player_PassedFrameScaled(em, 122.0f, 0) != 0)) {
            Eff04_Create(em, 0x13);
        }
        break;
    case 0x408:
        stateGroup = em->stateGroup;
        if (stateGroup != 6 || em->state) {
            if (stateGroup == 6) {
                if (em->state != 3) {
                    goto block_278;
                }
            } else {
block_278:
                if (Player_PassedFrameScaled(em, 66.0f, 0) != 0) {
                    Eff04_Create(em, 0x10);
                    StageObj_K0F_CreateOnEm(em, 5, 3, 1.0f);
                    break;
                }
            }
        }
        break;
    case 0x409:
        if (Player_PassedFrameScaled(em, 66.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x1F, 7, 8.0f);
        }
        if ((Player_PassedFrameScaled(em, 76.0f, 0) != 0) || (Player_PassedFrameScaled(em, 252.0f, 0) != 0)) {
            Eff04_Create(em, 0x1B);
        }
        if (EmMotion_InFrameRange(em, 0, 124.0f, 132.0f) || EmMotion_InFrameRange(em, 0, 154.0f, 170.0f)) {
            if (!(gScene.frame & 3)) {
                StageObj_K14_Create(em, 0x1A, (u16) Rand_Next(1) & 1, 1.0f);
                break;
            }
        }
        break;
    case 0x40A:
        EmFatalis_RequestOnFrame(em, 0xF2, 2);
        if (Player_PassedFrameScaled(em, 242.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x1E, 7, 1.2f);
        }
        if (Player_PassedFrameScaled(em, 244.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x19, 7, 1.2f);
        }
        break;
    case 0x40B:
        if (Player_PassedFrameScaled(em, 68.0f, 0) != 0) {
            Eff04_Create(em, 0x1E);
        }
        if ((Player_PassedFrameScaled(em, 34.0f, 0) != 0) && (em->posY <= (1000.0f + em->floorY))) {
            StageObj_K14_Create(em, 0x13, 0, 3.0f);
        }
        break;
    case 0x40C:
        if ((EmMotion_InFrameRange(em, 0, 30.0f, 70.0f) != 0) && (em->posY <= (1000.0f + em->floorY)) && !(gScene.frame & 3)) {
            StageObj_K14_Create(em, 0xA, 0x80, 1.0f);
        }
        if ((Player_PassedFrameScaled(em, 96.0f, 0) != 0) && (em->posY <= (1000.0f + em->floorY))) {
            StageObj_K14_Create(em, 0xB, 0, 2.0f);
        }
        break;
    case 0x40D:
        if ((Player_PassedFrameScaled(em, 52.0f, 0) != 0) && (em->posY <= (1000.0f + em->floorY))) {
            StageObj_K14_Create(em, 1, 0, 1.5f);
        }
        break;
    case 0x410:
        if ((em->posY <= (1000.0f + em->floorY)) && !(gScene.frame & 3)) {
            StageObj_K14_Create(em, 0xA, 0x80, 0.8f);
        }
        break;
    default:
        EmFatalis_MotionEffectsDefault(em);
        break;
    case 0x401:
    case 0x405:
    case 0x40E:
    case 0x40F:
        break;
    }
}

void EmFatalis_RequestOnFrame(void *em, s32 frame, s32 i)
{
    if (Player_PassedFrameScaled((Em *) em, (f32) frame, 0U) != 0) {
        CamShake_ReqAIf(em, i);
    }
}

void EmFatalis_UpdateMotionEffects(Em *em)
{
    EmFatalisWork *w = (EmFatalisWork *)&em->work;

    switch (w->fxStep) {
    case 0:
        w->fxStep++;
        break;
    case 1:
        EmFatalis_MotionEffects(em, (EmFatalisFxWork *) w);
        break;
    }
}

void EmFatalis_SpawnEffect01(u8 *em)
{
    Effect01_Spawn((Player *) em, 0);
}

void EmFatalis_TableNop(void)
{
}

void EmBlangonga_MotionSounds(Em *em, EmWork *work)
{
    s16 prevMotion;

    prevMotion = work->prevMotion;
    switch (prevMotion) {
    case 0x3EB:
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x9E, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x5E, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x90, 3, 0x1B);
        break;
    case 0x3ED:
        Em_PlaySeAtBoneOnFrame(em, 0x16, Sound_RandomSe(0x19, 2, 0x19, 2), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x52, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 3, 0x1B);
        break;
    case 0x3EE:
        Em_PlaySeAtBoneOnFrame(em, 0x16, Sound_RandomSe(0x19, 2, 0x19, 2), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x52, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 3, 0x17);
        break;
    case 0x3EF:
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x2C, 1, 0x2C, 2), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 3, 0x17);
        break;
    case 0x3F0:
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x2C, 1, 0x2C, 2), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 3, 0x1B);
        break;
    case 0x3F1:
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
    case 0x3F2:
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
    case 0x3F4:
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x1D, 1, 0x1E, 1), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x14, 3);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 5, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 5, 0x1B);
        break;
    case 0x3F6:
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x1D, 1, 0x1E, 1), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x14, 3);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 5, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 5, 0x17);
        break;
    case 0x3F8:
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
    case 0x3F9:
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
    case 0x3FB:
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
    case 0x3FC:
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x37, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 8, 8, 3);
        Em_PlaySeAtBoneOnFrame(em, 6, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x5E, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 3, 0x17);
        break;
    case 0x3FD:
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0x10, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 0x10, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x5A, 0x11, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x68, 0x11, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x76, 0x11, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x84, 0x11, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x92, 0x12, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x9E, 0x12, 9);
        break;
    case 0x3FE:
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
    case 0x3FF:
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 3, 0x1B);
        break;
    case 0x407:
        Em_PlaySeAtBoneOnFrame(em, 2, 0x2A, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 0x29, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xA2, 0x2A, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x104, 0x29, 0x13);
        break;
    case 0x408:
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
    case 0x409:
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
    case 0x40A:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x32, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0x33, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 4, 6, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xA2, 0x10, 0xB);
        Em_PlaySeAtBoneOnFrame(em, 0xB4, 7, 3);
        break;
    case 0x410:
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
    case 0x411:
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0x38, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 4, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 4, 5, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x36, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 3, 0x17);
        break;
    case 0x412:
        Em_PlaySeAtBoneOnFrame(em, 2, 0x5C, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x13, 0xE);
        break;
    case 0x413:
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
    case 0x414:
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
    case 0x416:
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
    case 0x417:
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
    case 0x418:
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
    case 0x41B:
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
    case 0x41C:
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
    case 0x41D:
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
    case 0x41E:
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
    case 0x41F:
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0x23, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x78, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 3, 0x17);
        Em_PlaySe1AtBoneOnFrame(em, 0x36, 0x2E, 0x13);
        Em_PlaySe1AtBoneOnFrame(em, 0x7C, 0x2E, 0x13);
        Em_PlaySe1AtBoneOnFrame(em, 0xA2, 0x2E, 0x13);
        Em_PlaySe1AtBoneOnFrame(em, 0xC6, 0x2E, 0x13);
        break;
    case 0x420:
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x2C, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x7A, 3, 0x1B);
        break;
    case 0x421:
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
    case 0x422:
        Em_PlaySeAtBoneOnFrame(em, 0x36, 0x2C, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x7C, 3, 0x1B);
        break;
    case 0x423:
        Em_PlaySeAtBoneVolOnFrame(em, 0x3A, 0x4C, 0x13, 7);
        Em_PlaySeAtBoneVolOnFrame(em, 0xAA, 0x3F, 0x13, 7);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x94, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xA2, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x94, 1, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0xA2, 3, 0x1B);
        break;
    case 0x424:
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
    case 0x425:
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x48, 4, 0x47, 4), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 0x23, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 4, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 3, 0x1B);
        break;
    case 0x426:
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
    case 0x427:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x51, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x20, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 0x11, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 7, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x74, 4, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0x12, 0x17);
        break;
    case 0x428:
        Em_PlaySeAtBoneOnFrame(em, 6, 0x54, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x12, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 3, 0x1B);
        break;
    case 0x429:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x48, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x4E, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x7C, 0x25, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x8A, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0xB4, 3, 0x1B);
        break;
    case 0x42A:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x4F, 0x13);
        break;
    case 0x42B:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x47, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 8, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 7, 3);
        break;
    case 0x42C:
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
    case 0x42D:
        Em_PlaySeAtBoneOnFrame(em, 2, 0x47, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 7, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x10, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0x12, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 7, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 2, 0xE);
        break;
    case 0x42E:
        Em_PlaySeAtBoneOnFrame(em, 2, 0x47, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 7, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x10, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0x12, 0xB);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 7, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 2, 9);
        break;
    case 0x42F:
        Em_PlaySeAtBoneOnFrame(em, 2, 0x49, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x11, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x88, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x12, 0x17);
        break;
    case 0x431:
        Em_PlaySeAtBoneOnFrame(em, 2, 0x48, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 8, 0x10, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x12, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 9, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 7, 0x1B);
        break;
    case 0x432:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x58, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x31, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 6, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 2, 9);
        break;
    case 0x433:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x4F, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x60, 0x22, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x56, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x66, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x68, 7, 0x13);
        break;
    case 0x434:
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x4B, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x68, 1, 9);
        break;
    case 0x435:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x51, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x20, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 0x11, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 7, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x74, 4, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0x12, 0x1B);
        break;
    case 0x436:
        Em_PlaySeAtBoneOnFrame(em, 6, 0x54, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x12, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 3, 0x17);
        break;
    case 0x438:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x5C, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x5E, 0x61, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 0x18, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 0x18, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 2, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x5A, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x5E, 2, 0xE);
        break;
    case 0x439:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x2C, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x5C, 3, 0x17);
        break;
    case 0x43D:
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
    case 0x43E:
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
    case 0x43F:
        Em_PlaySeAtBoneOnFrame(em, 2, 0x22, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 0x23, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x54, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x96, 3, 0x17);
        break;
    case 0x440:
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
    case 0x441:
        Em_PlaySeAtBoneOnFrame(em, 2, 0x36, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x42, 0x35, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x8C, 0x39, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x92, 2, 0xE);
        
    case 0x3E9:
    default:
        break;
    }
}

void EmBlangonga_MotionSoundsCongalala(Em *em, EmWork *work)
{
    s16 prevMotion;

    prevMotion = work->prevMotion;
    switch (prevMotion) {
    case 0x3E9:
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0x53, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xA0, 0x3F, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x11E, 0x40, 0x13);
        break;
    case 0x3EB:
        Em_PlaySeAtBoneOnFrame(em, 0x34, Sound_RandomSe(0x3F, 2, 0x3F, 2), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x76, 3, 0x1B);
        break;
    case 0x3ED:
        Em_PlaySeAtBoneOnFrame(em, 0x38, Sound_RandomSe(0x40, 2, 0x41, 2), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x52, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 3, 0x1B);
        break;
    case 0x3EE:
        Em_PlaySeAtBoneOnFrame(em, 0x38, Sound_RandomSe(0x40, 2, 0x41, 2), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x52, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 3, 0x17);
        break;
    case 0x3EF:
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x3C, 1, 0x3D, 2), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x36, Sound_RandomSe(0x41, 1, 0x41, 2), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 3, 0x17);
        break;
    case 0x3F0:
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x3C, 1, 0x3D, 2), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x36, Sound_RandomSe(0x41, 1, 0x41, 2), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 3, 0x1B);
        break;
    case 0x3F1:
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
    case 0x3F2:
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
    case 0x3F8:
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
    case 0x3F9:
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
    case 0x3FA:
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x60, 0x13);
        break;
    case 0x3FB:
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
    case 0x3FC:
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x42, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 0x2E, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 8, 8, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 9, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x74, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x80, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 3, 0x17);
        break;
    case 0x407:
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x51, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 0x52, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xAA, 0x51, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xFA, 0x52, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x13C, 0x35, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x1E6, 0x3F, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xFA, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x110, 6, 0xE);
        break;
    case 0x408:
        Em_PlaySeAtBoneOnFrame(em, 2, 0x10, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 0x3D, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 0x12, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x8C, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xA0, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x6E, 3, 0x17);
        break;
    case 0x409:
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
    case 0x40A:
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0x53, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xC8, 0x35, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 0x10, 9);
        Em_PlaySeAtBoneOnFrame(em, 4, 6, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0xA2, 0x11, 0xB);
        Em_PlaySeAtBoneOnFrame(em, 0xB4, 7, 3);
        break;
    case 0x410:
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0x22, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0x5C, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 0x58, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x112, 0x3D, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x162, 0x40, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x12A, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 0x19, 0x13);
        break;
    case 0x411:
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
    case 0x412:
        Em_PlaySeAtBoneOnFrame(em, 2, 0x23, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x13, 0xE);
        break;
    case 0x413:
        Em_PlaySeAtBoneOnFrame(em, 2, 0x21, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0xB, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x5A, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x5C, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 4, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x5C, 3, 0x1B);
        break;
    case 0x414:
        Em_PlaySeAtBoneOnFrame(em, 2, 0x21, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0xB, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x5A, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x5C, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 4, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x5C, 3, 0x17);
        break;
    case 0x415:
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
    case 0x416:
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
    case 0x418:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x39, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0x42, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x60, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 0x13, 3);
        Em_PlaySeAtBoneOnFrame(em, 2, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 4, 0x1B);
        break;
    case 0x419:
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
    case 0x41B:
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
    case 0x41C:
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
    case 0x41D:
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
    case 0x41E:
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
    case 0x41F:
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0x23, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x78, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0x3F, 0x13);
        Em_PlaySe1AtBoneOnFrame(em, 0x78, 0x2E, 0x13);
        Em_PlaySe1AtBoneOnFrame(em, 0x7C, 0x2E, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xA0, 0x3F, 0x13);
        Em_PlaySe1AtBoneOnFrame(em, 0xA2, 0x2E, 0x13);
        Em_PlaySe1AtBoneOnFrame(em, 0xC6, 0x2E, 0x13);
        break;
    case 0x420:
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x43, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x7A, 3, 0x1B);
        break;
    case 0x421:
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
    case 0x424:
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
    case 0x425:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x46, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 4, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 4, 0x1B);
        break;
    case 0x426:
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
    case 0x427:
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x4F, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 0x11, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 7, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x74, 4, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0x12, 0x17);
        break;
    case 0x435:
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x4F, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 0x11, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 7, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x74, 4, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0x12, 0x1B);
        break;
    case 0x428:
        Em_PlaySeAtBoneOnFrame(em, 6, 0x4E, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x12, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 3, 0x1B);
        break;
    case 0x436:
        Em_PlaySeAtBoneOnFrame(em, 6, 0x4E, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x12, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 3, 0x17);
        break;
    case 0x429:
        Em_PlaySeAtBoneOnFrame(em, 6, 0x46, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 0x50, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x7A, 0x12, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0xB8, 3, 0x1B);
        break;
    case 0x42A:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x60, 0x13);
        break;
    case 0x42B:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x47, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 8, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 7, 3);
        break;
    case 0x42C:
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
    case 0x42D:
        Em_PlaySeAtBoneOnFrame(em, 2, 0x48, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 7, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x10, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0x12, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 7, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 2, 0xE);
        break;
    case 0x42E:
        Em_PlaySeAtBoneOnFrame(em, 2, 0x48, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 7, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x10, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0x12, 0xB);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 7, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 2, 9);
        break;
    case 0x42F:
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x47, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 0x41, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x11, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x88, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x12, 0x17);
        break;
    case 0x431:
        Em_PlaySeAtBoneOnFrame(em, 2, 0x48, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x10, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 6, 0x12, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 9, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 7, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 5, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 4, 0x1B);
        break;
    case 0x432:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x5E, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 4, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 5, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x36, 4, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 5, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 4, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x70, 5, 0x1B);
        break;
    case 0x433:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x48, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x68, 7, 0x13);
        break;
    case 0x434:
        Em_PlaySeAtBoneOnFrame(em, 0x22, 0x12, 3);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 5, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 4, 0x17);
        break;
    case 0x437:
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
    case 0x438:
        Em_PlaySeAtBoneOnFrame(em, 2, 0x10, 3);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x35, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 0x43, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 4, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 4, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x5C, 3, 0x1B);
        break;
    case 0x439:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x2D, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x5C, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x35, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x5C, 0x35, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 4, 6, 9);
        break;
    case 0x43A:
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0x57, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x5E, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x62, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x84, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x5E, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x8E, 3, 0x1B);
        break;
    case 0x43B:
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
    case 0x43C:
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
    case 0x43D:
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
    case 0x43E:
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
    case 0x43F:
        Em_PlaySeAtBoneOnFrame(em, 2, 0x22, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0x43, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x54, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x96, 3, 0x17);
        break;
    case 0x440:
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

void EmBlangonga_MotionSoundsRajang(Em *em, EmWork *work)
{
    s16 prevMotion;
    u8 stateGroup;

    prevMotion = work->prevMotion;
    switch (prevMotion) {
    case 0x3EB:
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x9E, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x5E, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x90, 3, 0x1B);
        break;
    case 0x3ED:
        Em_PlaySeAtBoneOnFrame(em, 0x16, Sound_RandomSe(0x2C, 2, 0x2C, 2), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x52, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 3, 0x1B);
        break;
    case 0x3EE:
        Em_PlaySeAtBoneOnFrame(em, 0x16, Sound_RandomSe(0x2C, 2, 0x2C, 2), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x52, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 3, 0x17);
        break;
    case 0x3EF:
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x39, 1, 0x39, 2), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 3, 0x17);
        break;
    case 0x3F0:
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x39, 1, 0x39, 2), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 3, 0x1B);
        break;
    case 0x3F1:
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
    case 0x3F2:
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
    case 0x3F4:
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x1D, 1, 0x1E, 1), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x14, 3);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 5, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 5, 0x1B);
        break;
    case 0x3F6:
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x1D, 1, 0x1E, 1), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x14, 3);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 5, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 5, 0x17);
        break;
    case 0x3F8:
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
    case 0x3F9:
        Em_PlaySeAtBoneVolOnFrame(em, 0x16, 0x36, 0x13, 7);
        Em_PlaySeAtBoneVolOnFrame(em, 0x36, 0x24, 0x13, 7);
        Em_PlaySeAtBoneOnFrame(em, 4, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 8, 0x12, 9);
        Em_PlaySeAtBoneOnFrame(em, 8, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 5, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 4, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 0x14, 3);
        break;
    case 0x3FA:
        Em_PlaySeAtBoneOnFrame(em, 0x22, 0x14, 3);
        break;
    case 0x3FB:
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
    case 0x3FC:
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x37, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 0x39, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 8, 9, 3);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 7, 3);
        Em_PlaySeAtBoneOnFrame(em, 6, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x6E, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x84, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 3, 0x17);
        break;
    case 0x3FE:
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
    case 0x3FF:
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 3, 0x1B);
        break;
    case 0x407:
        Em_PlaySeAtBoneOnFrame(em, 2, 0x2A, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 0x29, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xA2, 0x2A, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x104, 0x29, 0x13);
        break;
    case 0x408:
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
    case 0x409:
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
    case 0x40A:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x2A, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0x33, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 4, 6, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xA2, 0x10, 0xB);
        Em_PlaySeAtBoneOnFrame(em, 0xB4, 7, 3);
        break;
    case 0x410:
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
    case 0x411:
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x60, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 4, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 4, 5, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x36, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 3, 0x17);
        break;
    case 0x412:
        Em_PlaySeAtBoneOnFrame(em, 2, 0x34, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x13, 0xE);
        break;
    case 0x413:
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
    case 0x414:
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
    case 0x418:
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
    case 0x41B:
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
    case 0x41C:
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
    case 0x41D:
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
    case 0x41E:
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
    case 0x41F:
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0x4B, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x78, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 3, 0x17);
        Em_PlaySe1AtBoneOnFrame(em, 0x36, 0x2E, 0x13);
        Em_PlaySe1AtBoneOnFrame(em, 0x7C, 0x2E, 0x13);
        Em_PlaySe1AtBoneOnFrame(em, 0xA2, 0x2E, 0x13);
        Em_PlaySe1AtBoneOnFrame(em, 0xC6, 0x2E, 0x13);
        break;
    case 0x420:
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x2C, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x7A, 3, 0x1B);
        break;
    case 0x421:
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
    case 0x422:
        Em_PlaySeAtBoneOnFrame(em, 0x36, 0x54, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x7C, 3, 0x1B);
        break;
    case 0x423:
        Em_PlaySeAtBoneVolOnFrame(em, 0x3A, 0x4C, 0x13, 7);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x94, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xA2, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x94, 1, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0xA2, 3, 0x1B);
        break;
    case 0x424:
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
    case 0x425:
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x48, 4, 0x47, 4), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 0x23, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 4, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 3, 0x1B);
        break;
    case 0x426:
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
    case 0x427:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x51, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x20, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 0x11, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 7, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x74, 4, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0x12, 0x17);
        break;
    case 0x428:
        Em_PlaySeAtBoneOnFrame(em, 6, 0x61, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x12, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 3, 0x1B);
        break;
    case 0x429:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x48, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x26, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x7C, 0x25, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x8A, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0xB4, 3, 0x1B);
        break;
    case 0x42A:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x4F, 0x13);
        break;
    case 0x42B:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x47, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 8, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 7, 3);
        break;
    case 0x42C:
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
    case 0x42D:
        Em_PlaySeAtBoneOnFrame(em, 2, 0x47, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 7, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x10, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0x12, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 7, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 2, 0xE);
        break;
    case 0x42E:
        Em_PlaySeAtBoneOnFrame(em, 2, 0x47, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 7, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x10, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0x12, 0xB);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 7, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 2, 9);
        break;
    case 0x42F:
        Em_PlaySeAtBoneOnFrame(em, 2, 0x49, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x11, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x88, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x12, 0x17);
        break;
    case 0x431:
        Em_PlaySeAtBoneOnFrame(em, 2, 0x48, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 8, 0x10, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x12, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 9, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 7, 0x1B);
        break;
    case 0x432:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x58, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x59, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 6, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 2, 9);
        break;
    case 0x433:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x4F, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x60, 0x22, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x56, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x66, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x68, 7, 0x13);
        break;
    case 0x434:
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x4B, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x68, 1, 9);
        break;
    case 0x435:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x51, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x20, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 0x11, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 7, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x74, 4, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0x12, 0x1B);
        break;
    case 0x436:
        Em_PlaySeAtBoneOnFrame(em, 6, 0x61, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x12, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 3, 0x17);
        break;
    case 0x438:
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
    case 0x439:
        Em_PlaySeAtBoneOnFrame(em, 0x34, 0x22, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x66, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x84, 2, 0xE);
        break;
    case 0x43A:
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0x1A, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 4, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 8, 0x12, 9);
        Em_PlaySeAtBoneOnFrame(em, 8, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 5, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 4, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x14, 3);
        break;
    case 0x43B:
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0x16, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x13, 3);
        break;
    case 0x43C:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x37, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x39, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 8, 8, 3);
        Em_PlaySeAtBoneOnFrame(em, 6, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 5, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 4, 0x1B);
        break;
    case 0x43D:
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
    case 0x43E:
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
    case 0x43F:
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
    case 0x440:
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
    case 0x442:
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x1D, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 5, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 5, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 0x14, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0x37, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 8, 3);
        
    case 0x3E9:
    default:
        break;
    }
}

/* em_rathalos's .data (0x8C9100-0x8C9C60) */
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
EmRathalosVec EmRathalos_CarveThrowPt = {
#include "game/008C91F8.inc"
};
s32 func_0090A9C0(void); /* gm_sub */
void *EmRathalos_Callbacks[8] = {
    EmRathalos_SpawnEffects, EmRathalos_SpawnEffects, EmRathalos_TableNop, EmRathalos_UpdateMotionEffects,
    func_0090A9C0, EmRathalos_Reinit, EmRathalos_AllowHit8, NULL,
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
u32 D_008C9B58[2] = {
#include "game/008C9B58.inc"
};
u32 D_008C9B60[2] = {
#include "game/008C9B60.inc"
};
u32 D_008C9B68[2] = {
#include "game/008C9B68.inc"
};
u32 D_008C9B70[2] = {
#include "game/008C9B70.inc"
};
u32 D_008C9B78[3] = {
#include "game/008C9B78.inc"
};
u32 D_008C9B90[8] = {
#include "game/008C9B90.inc"
};
u32 D_008C9BB0[12] = {
#include "game/008C9BB0.inc"
};
u32 D_008C9BE0[4] = {
    (u32)D_008C9B78, (u32)D_008C9B90, (u32)D_008C9BB0, 0,
};
EmTurnParams EmFatalis_TurnParams[1] = {
#include "game/008C9BF0.inc"
};
u32 D_008C9C10[6] = {
#include "game/008C9C10.inc"
};
s8 EmFatalis_MinePtBones[4][2] = {
#include "game/008C9C28.inc"
};
f32 EmFatalis_MinePtRadius[3] = {
#include "game/008C9C30.inc"
};
void *EmFatalis_Callbacks[8] = {
    EmFatalis_SpawnEffect01, EmFatalis_SpawnEffect01, EmFatalis_TableNop, EmFatalis_UpdateMotionEffects,
    func_0090A9C0, EmFatalis_TableNop, NULL, NULL,
};
