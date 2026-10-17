/*
 * Monster family: Yian Garuga (EM_KIND_YIAN_GARUGA). The code from the
 * family's setup function (EmGaruga_Setup, called by the per-kind dispatch
 * Em_TypeSetup) to the next family's. The state functions take the monster
 * and the work area its state dispatcher passes on (EmWork); Em.step is the
 * step within the current state. See include/em.h.
 */
#include "common.h"
#include "scene.h"
#include "quest.h"
#include "em.h"
#include "player.h"
#include "area.h"
/* prototypes of callees (round 13) */
u16 Rand_Next(s32 stream);
void GmEm_ResetSize(Em *em);
void Em_CheckRageByHp(Em *em, s16 n);
void EmGaruga_MotionSounds(Em *em, EmWork *work);
void Em_StartPendingRage(Em *);
void Em_StartGoalAction(Em *em, s32 arg1);
void Em_ResetNotice(Em *em);
void Em_BeginStatus2(Em *em);
void Em_RefillNeed3(Em *em);
void Em_PlaySeAtBoneVolOnFrame(Em *em, s32 frame, s32 se, s32 bone, s32 vol);
s32 Sound_RandomSe(s32 id, s16 lo, s32 alt, s16 n);
void Em_PlaySeAtBoneOnFrame(Em *em, s32 frame, s32 se, s32 bone);

/* prototypes */
u16 Area_IdForCurrentTime(u16);
void EmGaruga_ChangeState(Em *em, u16 group, u16 state, u8 mode);
s8 EmMine_CreateTimed0(Em *em, f32 *pos);
void Em_BeginStatus1(Em *em);
void Em_BeginStatus4(Em *em);
void Em_BeginStatus5(Em *em);
void Em_BreakPart(Em *em, s32 part);
void Em_EndAllStatus(Em *em);
void Em_EndStatus1(Em *em);
void Em_EndStatus2(Em *em);
void Em_EndStatus4(Em *em);
void Em_EndStatus5(Em *em);
void Em_SetLayerMotion(Em *em, s32 motion, s32 blend, s32 frame, s32 layer);
void Em_SetMotion(Em *em, s32 motion, s32 a, s32 b);
u16 Math_AngleXZ(f32 *from, f32 *to);
void Player_ClearFlag(void *, u32 flag);
void Player_SetFlag(void *obj, u32 flag);
s8 QuestEm_Appear(Em *em);
void QuestEm_Captured(Em *em);
s32 Save_TestEventFlag(s32);
void EmRangedAttack_CreateAtBone(Em *, s32, s32, s32, s32, s32);
void GarugaAttack_Create(Em *, s8);
void Em_SelectSightRow(Em *em, u8 idx);
void Em_MoveRotated(Em *em, void *src);
void Em_MoveRotatedAccel(Em *em, void *ang);
void Em_SetStateRand(Em *em, s32 arg1, s32 arg2);
/* unit's own declaration: with the definition's void (Em *em, s32 group, s32 state, u8 mode) (game/unk_701810.c), EmGaruga_ChangeState: 30 of 121 words differ, EmGaruga_SetGroup0State: 1 of 126 words differ ... */
void Em_SetStateSync(Em *em, u16 group, u16 state, u8 mode);
void Em_LoadReactionSet(Em *em, u8 k);
void Em_ComputeGoal2(Em *em, f32 *target);
void Em_GetPitchToGoal(Em *em, s32 *out, f32 a, f32 b);
void Em_ClearLowHpNeed(Em *em);
void Em_SpawnStatus4Effect(Em *em, s32 a);
void Em_SetHitReactMode(Em *em, u8 a);
void Em_SetTrapTimeGoal(Em *em);
/* unit's own declaration: with the definition's s32 (Em *em, s16 n, s16 max, s16 interval) (game/unk_708850.c), EmGaruga_State0_12: 45 of 94 words differ, EmGaruga_State0_15: 43 of 92 words differ */
s32 Em_RegenHp(Em *em, s32 n, s32 max, s32 interval);
void Em_ResetToBasePose(Em *em);
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
void Em_TargetPlayerInRange(Em *);
void EmGaruga_Init(Em *);
void EmGaruga_State2_01_FlyInLand(Em *em, EmWork *work);
void EmGaruga_State2_08(Em *, EmWork *);
void EmGaruga_State2_09(Em *, EmWork *);
void EmGaruga_State2_0D(Em *, EmWork *);
void EmGaruga_State3_07(Em *, EmWork *);
void EmGaruga_State3_08(Em *, EmWork *);
void EmGaruga_State3_09(Em *em, EmWork *work);
void EmGaruga_State4_07(Em *em, EmWork *work);
void EmGaruga_State4_08(Em *em, EmWork *work);
void EmGaruga_State6_00(Em *em, EmWork *work);
void EmGaruga_State6_04_Captured(Em *);
void EmGaruga_State8_00_CarveStart(Em *, EmWork *);
void EmGaruga_State5_00_Hunted(Em *, EmWork *);
void EmGaruga_State5_01_Hunted(Em *, EmWork *);
void EmGaruga_State5_02_HuntedFalling(Em *, EmWork *);
void EmGaruga_RunGroup0(Em *em, EmWork *work);
void EmGaruga_RunGroup1(Em *em, EmWork *work);
s32 EmMotion_InFrameRange(void *em, s32 layer, f32 from, f32 to);
void GarugaAttack_CreateForEffects(Em *em, s8 kind);
void StageObj_K0F_CreateAtMarked(f32 *pos, s32 k, s32 c, f32 scale);
void StageObj_K0F_CreateOnEm(Em *em, u8 k, u8 c, f32 scale);
void Mtx_GetTrans(f32 *out, f32 *m);
void Effect0D_SpawnForEmScaled(Em *em, s32 a, s32 b, f32 scale);
u8 Player_InCurrentArea(void *obj);
typedef struct EmGarugaFxWork {
    u8 pad[2];
    s16 motion;         /* 0x02: the last motion seen by the effects */
} EmGarugaFxWork;
void EmGaruga_MotionEffects(Em *em, EmGarugaFxWork *work);
void EmGaruga_SpawnK14Type0B(Em *em);
void EmGaruga_State3_End(Em *em, EmWork *work);
void EmGaruga_SetGroup0State(Em *em, u16 state, u8 mode);
void EmGaruga_SetGroup1State(Em *em, u16 state, u8 mode);
void EmGaruga_SetGroup2State(Em *em, u16 state, u8 mode);
void EmGaruga_SetGroup3State(Em *em, u16 state, u8 mode);
void EmGaruga_MoveByVelYCurve(Em *em, s32 kind);
void EmGaruga_StartCurves(Em *em, u8 kind);
u8 EmGaruga_UpdateCurves(Em *em);
void StageObj_K11_CreateAtBone(Em *em, s16 bone, s16 k, u16 c);
void StageObj_K13_Create(void *owner, s16 v, u8 k);
void StageObj_K14_Create(Em *em, s16 k, s16 c, f32 scale);
/* unit's own declaration: with the definition's void (Em *em, u8 k, s16 v, s16 t, f32 y, f32 scale) (game/unk_874B90.c), EmGaruga_State4_15: 66 of 98 words differ, EmGaruga_State4_16: 43 of 74 words differ */
void StageObj_Stun_CreateEm(Em *, s32, s32, s32, f32, f32);
s32 EmMotion_PassedFrameScaled(void *p, f32 frame, s32 layer);
s32 EmMotion_ReachedFrame(void *obj, s32 a, f32 frame);
f32 Vec_Distance(f32 *a, f32 *b);
void CamShake_ReqAIf(Em *obj, s32 i);
f32 Vec_DistanceXZ(f32 *a, f32 *b);
u16 Map_DayId(u16);
void Versus_MarkDefeat(Em *em);
s32 Versus_BothDefeated(void);
s32 QuestEm_CanAppear(Em *em);
void Player_ClearVelocity(void *);
void Player_ClearAccel2(void *);
void Motion_InitTime(void *p, s16 step);
void Effect01_Spawn(void *obj, s16 type);
extern f32 gAreaGroundY[];
extern EmFlyParam EmGaruga_FlyParam;
extern EmFlyParam EmGaruga_FlyParam2;
extern EmTurnParams EmGaruga_TurnParams[2];
void Em_ComputeGoal(Em *em, f32 *target);
void Em_TypeSetup(Em *em);
void Em_SetSpeedScaled(Em *em, u16 flags, f32 x, f32 y);
void Em_SetTargetAngle(Em *em);
s32 Em_GetTargetPos(Em *em, u8 idx, f32 *out);
void StageObj_K25_Create(Player *p, Em *em, u8 k, s32 unused, f32 *pos);
void Versus_RouteEmByTeam(Em *em);
s32 Versus_DefeatCamera(Em *pl);
typedef struct EmGarugaVec {
    f32 x, y, z;
} EmGarugaVec;
extern EmGarugaVec EmGaruga_CarveThrowPt __attribute__((aligned(8)));
extern EmCurveKey *EmGaruga_MoveVelYCurves[];
void Em_SpawnLyingPuffs(Em *em, s32 bone, f32 *pos, f32 scale);
void Em_PlaySeAtBone(Em *em, s32 se, s32 bone);
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
u16 Area_DayId(u16 id);
void StageObj_K09_Create(Em *);
void Em_SetPartScaled(Em *em, s32 bit);
void Em_SetMotionRow(Em *em, s32 row, s32 arg2, s32 arg3);
s32 Em_ComputeMaxHp(Em *em, s16 idx);
s32 Em_UpdateCombatTimer(Em *em);
void Em_RaisePlayerAreaTriggerLax(Em *em);
u8 Em_UpdateDamage(Em *em, u8 *out);
void Em_SetTrapTime(Em *em, u8 *p);
void Em_SetStatus4Timer(Em *em);
void Em_SetStatus1Timer(Em *em);
void Em_SetStatus2Timer(Em *em);
void Em_SetStatus5Timer(Em *em);
void Em_ResetKindTimer(Em *em);
void Em_UpdatePendingScript(Em *em);
void Em_ClearHitReactMode(Em *em);
s32 Em_IsHeld(Em *em);
void EmGaruga_RunState(Em *em, s32 arg1);
/* end of prototypes */

void EmGaruga_Setup(Em *em)
{
}

void EmGaruga_SpawnEffects(void *em)
{
    StageObj_K13_Create(em, 0x22, 0);
    Effect01_Spawn((Player *) em, 0);
}

void EmGaruga_Init(Em *em)
{
    EmFlyerWork *w = (EmFlyerWork *)&em->work;
    s16 hp;

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
    Em_SetMotionRow(em, 1, 0, 0);
    em->stance = 0;
    w->fxMotion = 0;
    hp = Em_ComputeMaxHp(em, 0);
    em->hp = hp;
    em->hpMax = hp;
    em->scriptPending = 1;
    em->blind = 0;
    em->unk_InitFlag = 1;
    em->noAlert = 0;
    em->painted = 0;
    em->aimAngle = 0x4000;
    w->pitchStep = 0x100;
    ((EmWork *)w)->turnSpeed = 0x200;
    w->rollSpeed = 0x100;
    w->moveAngBY = 0x2000;
    *(s32 *) &em->flightTurn = 0;
    if ((em->fxFlags &= 1) == 0) {
        switch (em->type) {
        case 0x28:
            em->meshShown[1] = 0;
            StageObj_K09_Create(em);
            break;
        }
    }
}

void EmGaruga_Start(Em *em)
{
    EmGaruga_Init(em);
    EmGaruga_ChangeState(em, 0, 1, 0);
    switch (gQuest.stage) {
    case 0x1771:
        if (Save_TestEventFlag(0xA1) == 0) {
            em->area = Area_IdForCurrentTime(AREA_JUNGLE_9);
            em->posX = 10051.0f;
            em->posY = 1040.0f;
            em->posZ = 8395.0f;
            em->rotX = 0;
            em->rotY = 0;
            em->rotZ = 0;
        }
        break;
    }
}

void EmGaruga_Reinit(Em *em)
{
    EmGaruga_Init(em);
    if ((Map_DayId((u16) gScene.map) & 0xFFFF) == MAP_GREAT_ARENA) {
        em->area = Area_IdForCurrentTime(AREA_GREAT_ARENA_1);
        if (em->variant == 0) {
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
}

void EmGaruga_ToIdle(Em *em)
{
    em->scriptPending = 1;
    if (em->stance == 0) {
        EmGaruga_ChangeState(em, 0, 1, 0);
        return;
    }
    EmGaruga_ChangeState(em, 2, 2, 0);
}

void EmGaruga_EndState(Em *em, s32 unused1, s32 unused2)
{
    EmGaruga_ToIdle(em);
}

void EmGaruga_State4_End(Em *em, s32 unused1, s32 unused2)
{
    EmGaruga_EndState(em, unused1, unused2);
}

void EmGaruga_EndState2(Em *em, u8 next)
{
    s32 k;

    k = next & 0xFF;
    em->stance = 2;
    em->scriptPending = 1;
    em->motionSpeed = 1.0f;
    switch (k) {
    case 0:
        Em_SetStateRand(em, 2, 0xE);
        break;
    case 1:
        Em_SetStateRand(em, 2, 0xF);
        break;
    }
}

void EmGaruga_RestartLayerMotion(Em *em, s32 layer)
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

void EmGaruga_State0_00(Em *em)
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

void EmGaruga_State0_01(Em *em)
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
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGaruga_State0_02(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x6A, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->lookAtTarget = 0;
            em->step++;
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGaruga_State0_03(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x66, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGaruga_State0_04(Em *em)
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
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
    EmGaruga_RestartLayerMotion(em, 1);
    EmGaruga_RestartLayerMotion(em, 2);
}

void EmGaruga_State0_05(Em *em)
{
    EmGaruga_EndState(em, 0, 0);
}

void EmGaruga_State0_06(Em *em)
{
    EmGaruga_EndState(em, 0, 0);
}

void EmGaruga_State0_07(Em *em)
{
    EmGaruga_EndState(em, 0, 0);
}

void EmGaruga_State0_09(Em *em)
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
            em->step++;
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGaruga_State0_0A(Em *em)
{
    u16 aimAngle;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x65, 0, 0);
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
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGaruga_State0_0B(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x1C, 0, 0);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
    EmGaruga_RestartLayerMotion(em, 0);
    EmGaruga_RestartLayerMotion(em, 2);
    Em_PlaySeAtBoneOnLayerFrame(em, 0x38, 0x21, 0x23, 1);
    Em_PlaySeAtBoneOnLayerFrame(em, 0x68, 0x1F, 0x23, 1);
}

void EmGaruga_State0_0C(Em *em)
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
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGaruga_State0_0D(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x14, 0, 0);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
    EmGaruga_RestartLayerMotion(em, 0);
    EmGaruga_RestartLayerMotion(em, 2);
    Em_PlaySeAtBoneOnLayerFrame(em, 2, 0x20, 0x23, 1);
    Em_PlaySeAtBoneOnLayerFrame(em, 0x4A, 0x1F, 0x23, 1);
}

void EmGaruga_State0_0E(Em *em)
{
    EmGaruga_EndState(em, 0, 0);
}

void EmGaruga_State0_0F(Em *em)
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
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGaruga_State0_10(Em *em)
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
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGaruga_State0_11(Em *em)
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
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGaruga_State0_12(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x27, 0, 0);
        Em_LoadReactionSet(em, 1);
        em->timer = 0x1518;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x22, 0, 0);
        }
        break;
    case 2:
        Em_RegenHp(em, 1, (s32) (0.35f * (f32) em->hpMax), 9);
        if (--em->timer <= 0) {
            em->step++;
            Em_ClearLowHpNeed(em);
            EmGaruga_ChangeState(em, 0, 0x17, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmGaruga_ChangeState(em, 0, 0x17, 4);
        }
        break;
    }
}

void EmGaruga_State0_13(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_NO_FLINCH);
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
        if (EmMotion_PassedFrameScaled(em, 200.0f, 0U) != 0) {
            em->step++;
            EmGaruga_ChangeState(em, 0, 0x28, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmGaruga_ChangeState(em, 0, 0x28, 4);
        }
        break;
    }
}

void EmGaruga_State0_28(Em *em)
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
            Em_SetMotion(em, 0x3B, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGaruga_State0_14(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
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
            EmGaruga_ChangeState(em, 0, 0x18, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmGaruga_ChangeState(em, 0, 0x18, 4);
        }
        break;
    }
}

void EmGaruga_State0_15(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x26, 0, 0);
        Em_LoadReactionSet(em, 1);
        em->timer = 0x1518;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x22, 0, 0);
        }
        break;
    case 2:
        Em_RegenHp(em, 1, (s32) (0.35f * (f32) em->hpMax), 9);
        if (--em->timer <= 0) {
            em->step++;
            EmGaruga_ChangeState(em, 0, 0x18, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmGaruga_ChangeState(em, 0, 0x18, 4);
        }
        break;
    }
}

void EmGaruga_State0_16(Em *em)
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
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGaruga_State0_17(Em *em)
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
            EmGaruga_ChangeState(em, 0, 0x19, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmGaruga_ChangeState(em, 0, 0x19, 4);
        }
        break;
    }
}

void EmGaruga_State0_18(Em *em)
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
            EmGaruga_ChangeState(em, 0, 0x19, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmGaruga_ChangeState(em, 0, 0x19, 4);
        }
        break;
    }
}

void EmGaruga_State0_19(Em *em)
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
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGaruga_State0_1A(Em *em)
{
    EmGaruga_EndState(em, 0, 0);
}

void EmGaruga_State0_1B(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
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
            EmGaruga_ChangeState(em, 0, 0x1C, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmGaruga_ChangeState(em, 0, 0x1C, 4);
        }
        break;
    }
}

void EmGaruga_State0_1C(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x25, 0, 0);
        em->blind = 2;
        Em_LoadReactionSet(em, 0);
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            EmGaruga_ChangeState(em, 0, 0x19, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmGaruga_ChangeState(em, 0, 0x19, 4);
        }
        break;
    }
}

void EmGaruga_State0_1D(Em *em, EmWork *work)
{
    f32 pos[3];

    em->holdTimer = 5;
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
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
            EmGaruga_ChangeState(em, 4, 0x12, 3);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmGaruga_ChangeState(em, 4, 0x12, 3);
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

void EmGaruga_State0_1F(Em *em, EmWork *work)
{
    f32 pos[3];

    em->holdTimer = 5;
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
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
            EmGaruga_ChangeState(em, 4, 0x13, 3);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmGaruga_ChangeState(em, 4, 0x13, 3);
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

void EmGaruga_State0_21(Em *em)
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
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGaruga_State0_08(Em *em, EmWork *work, s32 variant)
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
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGaruga_State1_00(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 3, 0, 0);
        break;
    case 1:
        if (em->hasGoal != 0) {
            if (Em_StepToGoal(em, 0x40, 1) == 1) {
                em->step++;
                EmGaruga_EndState(em, 0, 0);
                break;
            }
        } else {
            if (--em->timer <= 0) {
                em->step++;
                EmGaruga_EndState(em, 0, 0);
            }
        }
        break;
    }
}

void EmGaruga_State1_01(Em *em)
{
    em->step++;
    EmGaruga_EndState(em, 0, 0);
}

void EmGaruga_State1_02(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x68, 0, 0);
        break;
    case 1:
        if (em->hasGoal != 0) {
            if (Em_StepToGoal(em, 0x40, 1) == 1) {
                em->step++;
                EmGaruga_EndState(em, 0, 0);
                break;
            }
        } else {
            if (--em->timer <= 0) {
                em->step++;
                EmGaruga_EndState(em, 0, 0);
            }
        }
        break;
    }
}

void EmGaruga_State1_03(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_StartTurnToTarget(em, &EmGaruga_TurnParams[0]);
        break;
    case 1:
        if (Em_TurnStep(em, &EmGaruga_TurnParams[0]) == 1) {
            em->step++;
            Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGaruga_State1_04(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x11, 0, 0);
        break;
    case 1:
        if (em->hasGoal != 0) {
            if (Em_StepToGoal(em, 0x40, 1) == 1) {
                em->step++;
                EmGaruga_EndState(em, 0, 0);
                break;
            }
        } else {
            if (--em->timer <= 0) {
                em->step++;
                EmGaruga_EndState(em, 0, 0);
            }
        }
        break;
    }
}

void EmGaruga_State1_05(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_StartTurnToTarget(em, &EmGaruga_TurnParams[1]);
        break;
    case 1:
        if (Em_TurnStep(em, &EmGaruga_TurnParams[1]) == 1) {
            em->step++;
            Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGaruga_State1_06(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0xA, 0, 0);
        break;
    case 1:
        if (em->hasGoal != 0) {
            if (Em_StepToGoal(em, 0x40, 1) == 1) {
                em->step++;
                EmGaruga_EndState(em, 0, 0);
                break;
            }
        } else {
            if (--em->timer <= 0) {
                em->step++;
                EmGaruga_EndState(em, 0, 0);
            }
        }
        break;
    }
}

void EmGaruga_State1_07(Em *em, EmWork *work)
{
    u16 t;
    u32 d;
    s32 a;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x36, 0, 0);
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
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
        if (em->motion[1].playing == 0) {
            em->step++;
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGaruga_State1_09(void)
{
}

void EmGaruga_State1_0A(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x68, 0, 0);
        break;
    case 1:
        if (em->hasGoal != 0) {
            if (Em_StepToGoal(em, 0x40, 1) == 1) {
                em->step++;
                Em_SetMotion(em, 0x65, 0, 0);
                break;
            }
        } else {
            if (--em->timer <= 0) {
                em->step++;
                Em_SetMotion(em, 0x65, 0, 0);
            }
            break;
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGaruga_State2_00(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x12, 0, 0);
        EmGaruga_StartCurves(em, 2);
        work->flyGuard = 0;
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 38.0f, 0U) != 0) {
            em->step++;
            em->stance = 2;
            EmGaruga_UpdateCurves(em);
        }
        break;
    case 2:
        if (EmGaruga_UpdateCurves(em)) {
            em->step++;
            EmGaruga_EndState2(em, 0U);
        }
        break;
    }
}

void EmGaruga_State2_01_FlyInLand(Em *em, EmWork *work)
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
        if (!((630.0f + em->floorY) <= em->posY) && ((EmMotion_PassedFrameScaled(em, 10.0f, 0U) != 0) || (EmMotion_PassedFrameScaled(em, 86.0f, 0U) != 0) || (EmMotion_PassedFrameScaled(em, 160.0f, 0U) != 0))) {
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
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
    h = em->floorY;
    if (em->posY < h) {
        em->posY = h;
    }
}

void EmGaruga_State2_02(Em *em, EmWork *work)
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
        EmGaruga_MoveByVelYCurve(em, 1U);
        if (--em->timer <= 0) {
            em->step++;
            EmGaruga_ChangeState(em, 2, 1, 1);
        }
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 1);
        if (em->timer == 0x12C) {
            EmGaruga_EndState2(em, 0U);
        }
        break;
    case 2:
        EmGaruga_MoveByVelYCurve(em, 1U);
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 1);
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmGaruga_State2_03(Em *em, EmWork *work)
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
        EmGaruga_StartCurves(em, 2);
        work->flyGuard = 0;
        work->turnSpeed = 0x100;
        break;
    case 1:
        if ((EmMotion_ReachedFrame(em, 0, 50.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 114.0f) == 0)) {
            Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 1);
        }
        if (EmMotion_ReachedFrame(em, 0, 30.0f) != 0) {
            em->stance = 2;
            EmGaruga_UpdateCurves(em);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
        }
        break;
    case 2:
        if ((EmGaruga_UpdateCurves(em) & 0xFF) && (em->posY <= (800.0f + em->floorY))) {
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
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmGaruga_State2_04(Em *em, EmWork *work)
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
        EmGaruga_StartCurves(em, 3);
        em->velZ = 20.0f;
        work->flyGuard = 0;
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 30.0f) != 0) {
            em->stance = 2;
            ended = EmGaruga_UpdateCurves(em) & 0xFF;
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
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmGaruga_State2_05(Em *em, EmWork *work)
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
        EmGaruga_StartCurves(em, 3);
        em->velZ = -20.0f;
        work->flyGuard = 0;
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 30.0f) != 0) {
            em->stance = 2;
            ended = EmGaruga_UpdateCurves(em) & 0xFF;
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
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmGaruga_State2_06(Em *em, EmWork *work)
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
                EmGaruga_EndState2(em, 1U);
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
        Em_UpdateFlight(em, 0xE1, &EmGaruga_FlyParam);
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

void EmGaruga_State2_07(Em *em, EmWork *work)
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
        EmGaruga_MoveByVelYCurve(em, 1U);
        dist = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer > 0) {
            if (dist <= (10.0f * em->velZ)) {
                goto block_9;
            }
        } else {
block_9:
            em->step++;
            Player_ClearVelocity(em);
            EmGaruga_EndState2(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmGaruga_State2_08(Em *em, EmWork *work)
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
        Em_UpdateFlight(em, 0x4E1, &EmGaruga_FlyParam);
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
            EmGaruga_ChangeState(em, 2, 9, 1);
        } else {
            if (em->remote == 0) {
                em->nextArea = next;
                em->goalNo = next;
                em->goalKind = 3;
                Em_ComputeGoal(em, em->goal);
            }
            EmGaruga_ChangeState(em, 2, 0xD, 1);
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

void EmGaruga_State2_09(Em *em, EmWork *work)
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
        em->rotY = em->aimAngle;
        *(u16 *) &em->destArea = AREA_NONE;
        work->flyGuard = 1;
        break;
    case 1:
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 0xE5, &EmGaruga_FlyParam);
        work->moveAngX = em->rotX;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
        d = Vec_DistanceXZ(&em->posX, em->goal);
        em->timer--;
        if (d <= 1000.0f || em->timer < 0) {
            em->step++;
            em->timer = 600;
            EmGaruga_ChangeState(em, 2, 1, 1);
        }
        break;
    case 2:
        if (em->remote == 0) {
            if (--em->timer <= 0) {
                em->timer = 600;
                EmGaruga_ChangeState(em, 2, 1, 1);
            }
        }
        break;
    }
}

void EmGaruga_State2_0A(Em *em)
{
    EmGaruga_EndState2(em, 0x1);
}

void EmGaruga_State2_0B(Em *em, EmWork *work)
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
            EmGaruga_ChangeState(em, 2, 1, 1);
        } else {
            em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
            em->aimAngle = em->aimAngle - em->rotY;
            Em_UpdateFlight(em, 0xE1, &EmGaruga_FlyParam);
            work->moveAngX = em->rotX;
            work->moveAngY = em->rotY;
            work->moveAngZ = 0;
            Em_MoveRotated(em, &work->moveAngX);
            if (--em->timer <= 0) {
                em->step++;
                EmGaruga_ChangeState(em, 2, 1, 1);
            }
        }
        break;
    case 2:
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 0xE1, &EmGaruga_FlyParam);
        work->moveAngX = em->rotX;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
        if (--em->timer <= 0) {
            em->timer = 0x12C;
            if (em->remote == 0) {
                EmGaruga_ChangeState(em, 2, 1, 1);
            }
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmGaruga_State2_0C(Em *em)
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
        EmGaruga_StartCurves(em, 0xE);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 76.0f) != 0) {
            em->stance = 2;
            ended = EmGaruga_UpdateCurves(em) & 0xFF;
        }
        if ((ended != 0) && (em->posY <= em->floorY)) {
            Em_SetMotion(em, 0x39, 0, 0);
            em->step++;
            em->stance = 0;
            em->posY = em->floorY;
            EmGaruga_SpawnK14Type0B(em);
            CamShake_ReqAIf(em, 2);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmGaruga_State2_0D(Em *em, EmWork *work)
{
    AreaRecord *area;
    f32 *pt;
    f32 d;
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
        break;
    case 1:
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 0xE5, &EmGaruga_FlyParam);
        work->moveAngX = em->rotX;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
        d = Vec_DistanceXZ(&em->posX, em->goal);
        em->timer--;
        if (d <= 1000.0f || em->timer < 0) {
            em->step++;
            Em_AreaExitPoint(em, (f32 *) em->goal);
        }
        break;
    case 2:
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 0xE1, &EmGaruga_FlyParam);
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
            EmGaruga_ChangeState(em, 2, 9, 1);
        } else {
            if (em->remote == 0) {
                em->nextArea = next;
                em->goalNo = next;
                em->goalKind = 3;
                Em_ComputeGoal(em, em->goal);
            }
            EmGaruga_ChangeState(em, 2, 0xD, 1);
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

void EmGaruga_State2_0E(Em *em, EmWork *work)
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
        EmGaruga_MoveByVelYCurve(em, 1U);
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGaruga_EndState2(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmGaruga_State2_0F(Em *em, EmWork *work)
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
            EmGaruga_EndState2(em, 1U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmGaruga_State2_10(Em *em, EmWork *work)
{
    f32 floorY;
    s32 done;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        Em_SetMotion(em, 0xF, 0, 0);
        work->turnSpeed = 0x100;
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        work->flyGuard = 0;
        em->timer = 0x96;
        break;
    case 1:
        EmGaruga_MoveByVelYCurve(em, 1U);
        done = Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 1) & 0xFF;
        if (--em->timer > 0) {
            if (done != 0) {
                goto block_9;
            }
        } else {
block_9:
            em->step++;
            EmGaruga_EndState2(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmGaruga_State2_11(Em *em, EmWork *work)
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
        Em_UpdateFlight(em, 0x321, &EmGaruga_FlyParam2);
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

void EmGaruga_State2_12(Em *em, EmWork *work)
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
        EmGaruga_StartCurves(em, 9);
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
        if (EmGaruga_UpdateCurves(em) & 0xFF) {
            em->step++;
            Player_ClearVelocity(em);
            em->timer = 0x12C;
            Em_SetMotion(em, 0xF, 0, 0);
            EmGaruga_ChangeState(em, 2, 1, 1);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->timer = 0x12C;
            Em_SetMotion(em, 0xF, 0, 0);
            if (em->remote == 0) {
                EmGaruga_ChangeState(em, 2, 1, 1);
            }
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmGaruga_State2_13(Em *em, EmWork *work)
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
            Em_SetStateRand(em, 2, 0xF);
        }
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 0x121, &EmGaruga_FlyParam2);
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

void EmGaruga_State2_14(Em *em, EmWork *work)
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
        EmGaruga_StartCurves(em, 0xC);
        work->flyGuard = 0;
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 36.0f) != 0) {
            em->stance = 2;
            ended = EmGaruga_UpdateCurves(em) & 0xFF;
        }
        if ((ended != 0) && (em->posY <= em->floorY)) {
            em->step++;
            em->stance = 0;
            em->posY = em->floorY;
            EmGaruga_SpawnK14Type0B(em);
            CamShake_ReqAIf(em, 2);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmGaruga_State2_15(Em *em, EmWork *work)
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
            EmGaruga_EndState2(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmGaruga_State2_16(Em *em, EmWork *work)
{
    s32 rotX;
    s32 rotZ;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x5A, 0, 0);
        EmGaruga_StartCurves(em, 9);
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
        if (EmGaruga_UpdateCurves(em) & 0xFF) {
            em->step++;
            Player_ClearVelocity(em);
            EmGaruga_EndState2(em, 0U);
        }
        break;
    case 2:
        EmGaruga_UpdateCurves(em);
        break;
    }
}

void EmGaruga_State2_17(Em *em, EmWork *work)
{
    f32 floorY;
    f32 velY;
    f32 y;

    switch (em->step) {
    case 0:
        em->step++;
        em->hitReactMode = 0;
        Em_SetMotion(em, 0x12, 0xA, 0x1E);
        EmGaruga_StartCurves(em, 2);
        em->stance = 2;
        em->holdTimer = 0;
        em->unk8AD = 0;
        em->noReact = 1;
        break;
    case 1:
        EmGaruga_UpdateCurves(em);
        if (em->motion[0].playing == 0) {
            em->step++;
        }
        break;
    case 2:
        if ((EmGaruga_UpdateCurves(em) & 0xFF) && (em->posY <= (800.0f + em->floorY))) {
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
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
    y = em->floorY;
    if (em->posY < y) {
        em->posY = y;
    }
}

void EmGaruga_State2_18(Em *em, EmWork *work)
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
            EmGaruga_EndState2(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmGaruga_State3_00(Em *em, EmWork *work)
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
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGaruga_State3_01(Em *em, EmWork *work)
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
            EmGaruga_State3_End(em, work);
        }
        break;
    }
}

void EmGaruga_State3_02(Em *em, EmWork *work)
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
            EmGaruga_State3_End(em, work);
        }
        break;
    }
}

void EmGaruga_State3_03(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        if (Em_TurnToTargetMotion(em, 0xE38, 3, 6, 5) != 0) {
            em->step++;
            em->stance = 0;
            em->lookAtTarget = 0;
            Em_SetMotion(em, 0x66, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGaruga_State3_04(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x11, 0, 0);
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
            Em_SetMotion(em, 0x5B, 0, 0);
            GarugaAttack_Create(em, 0x24);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGaruga_State3_05(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        
    case 1:
        if (Em_TurnToTargetMotion(em, 0xE38, 3, 6, 5) != 0) {
            em->step++;
            em->lookAtTarget = 0;
            Em_SetMotion(em, 0x6A, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->lookAtTarget = 0;
            em->step++;
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGaruga_State3_06(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x28, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->lookAtTarget = 0;
            em->step++;
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGaruga_State3_07(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
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
        } else if (EmMotion_PassedFrameScaled(em, 78.0f, 0U) != 0) {
            StageObj_K11_CreateAtBone(em, 0x24, 1, 0);
            StageObj_K11_CreateAtBone(em, 0x24, 2, 0);
            EmRangedAttack_CreateAtBone(em, 0x22, 0, 0, 0, 0);
        }
        if (em->motion[0].playing == 0) {
            em->lookAtTarget = 0;
            em->step++;
            if (--((EmFlyerWork *)work)->countdown <= 0) {
                EmGaruga_EndState(em, 0, 0);
            } else {
                Em_SetTargetAngle(em);
                Em_SetStateRand(em, 3, 7);
            }
        }
        break;
    }
}

void EmGaruga_State3_08(Em *em, EmWork *work)
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
        } else if (EmMotion_PassedFrameScaled(em, 78.0f, 0U) != 0) {
            StageObj_K11_CreateAtBone(em, 0x24, 1, 0);
            StageObj_K11_CreateAtBone(em, 0x24, 2, 0);
            EmRangedAttack_CreateAtBone(em, 0x22, 0, 0, 0xE39, 0);
        }
        if (em->motion[0].playing == 0) {
            em->lookAtTarget = 0;
            em->step++;
            if (--((EmFlyerWork *)work)->countdown <= 0) {
                EmGaruga_EndState(em, 0, 0);
            } else {
                Em_SetTargetAngle(em);
                Em_SetStateRand(em, 3, 8);
            }
        }
        break;
    }
}

void EmGaruga_State3_09(Em *em, EmWork *work)
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
        } else if (EmMotion_PassedFrameScaled(em, 78.0f, 0U) != 0) {
            StageObj_K11_CreateAtBone(em, 0x24, 1, 0);
            StageObj_K11_CreateAtBone(em, 0x24, 2, 0);
            EmRangedAttack_CreateAtBone(em, 0x22, 0, 0, 0xF1C8, 0);
        }
        if (em->motion[0].playing == 0) {
            em->lookAtTarget = 0;
            em->step++;
            if (--((EmFlyerWork *)work)->countdown <= 0) {
                EmGaruga_EndState(em, 0, 0);
            } else {
                Em_SetTargetAngle(em);
                Em_SetStateRand(em, 3, 9);
            }
        }
        break;
    }
}

void EmGaruga_State3_0A(Em *em, EmWork *work)
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
            Em_SetMotion(em, 0x69, 0, 0);
        }
        break;
    case 2:
        if ((EmMotion_ReachedFrame(em, 0, 256.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 316.0f) == 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create((Em *) em, 0x1A, 1, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 78.0f, 0U) != 0) {
            StageObj_K11_CreateAtBone(em, 0x24, 1, 0);
            StageObj_K11_CreateAtBone(em, 0x24, 2, 0);
            EmRangedAttack_CreateAtBone(em, 0x22, 0, 0, 0x5B0, 0);
        }
        if (EmMotion_PassedFrameScaled(em, 120.0f, 0U) != 0) {
            StageObj_K11_CreateAtBone(em, 0x24, 1, 0);
            StageObj_K11_CreateAtBone(em, 0x24, 2, 0);
            EmRangedAttack_CreateAtBone(em, 0x22, 0, 0, 0x4FA, 0xF1C8);
        }
        if (EmMotion_PassedFrameScaled(em, 164.0f, 0U) != 0) {
            StageObj_K11_CreateAtBone(em, 0x24, 1, 0);
            StageObj_K11_CreateAtBone(em, 0x24, 2, 0);
            EmRangedAttack_CreateAtBone(em, 0x22, 0, 0, 0x444, 0x9F5);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            em->lookAtTarget = 0;
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGaruga_State3_0B(Em *em, EmWork *work)
{
    f32 floorY;
    s32 ended;

    ended = 0;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x29, 0, 0);
        EmGaruga_StartCurves(em, 0xA);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 94.0f) != 0) {
            em->stance = 2;
            ended = EmGaruga_UpdateCurves(em) & 0xFF;
        }
        if ((ended != 0) && (em->posY <= (372.0f + em->floorY))) {
            em->step++;
            Em_SetMotion(em, 0x2D, 0, 0);
            EmGaruga_StartCurves(em, 0xB);
        }
        break;
    case 2:
        EmGaruga_UpdateCurves(em);
        if (em->posY <= em->floorY) {
            em->step++;
            Em_SetMotion(em, 0x57, 0, 0);
            em->stance = 0;
            em->posY = em->floorY;
            EmGaruga_SpawnK14Type0B(em);
            CamShake_ReqAIf(em, 2);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmGaruga_State3_0D(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x66, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGaruga_State4_00(Em *em, EmWork *work)
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
            EmGaruga_State4_End(em, 0, 0);
        }
        break;
    }
}

void EmGaruga_State4_01(Em *em, EmWork *work)
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
            EmGaruga_State4_End(em, 0, 0);
        }
        break;
    }
}

void EmGaruga_State4_02(Em *em, EmWork *work)
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
            EmGaruga_State4_End(em, 0, 0);
        }
        break;
    }
}

void EmGaruga_State4_03(Em *em, EmWork *work)
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
            EmGaruga_State4_End(em, 0, 0);
        }
        break;
    }
}

void EmGaruga_State4_04(Em *em, EmWork *work)
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
            EmGaruga_ChangeState(em, 4, 0x14, 3);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmGaruga_ChangeState(em, 4, 0x14, 3);
        }
        break;
    }
}

void EmGaruga_State4_05(Em *em, EmWork *work)
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
            EmGaruga_EndState(em, 0, 0);
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
        }
        break;
    }
}

void EmGaruga_State4_06(Em *em, EmWork *work)
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
            EmGaruga_EndState(em, 0, 0);
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
        }
        break;
    }
}

void EmGaruga_State4_07(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x4D, 0, 0);
        if (em->velY > -50.0f) {
            em->velY = -50.0f;
        }
        Em_RestartScript(em);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 14.0f, 0) != 0) {
            em->step++;
            EmGaruga_EndState2(em, 1);
            em->velY = 0.0f;
        }
        Em_MoveRotated(em, &work->moveAngX);
        break;
    }
}

void EmGaruga_State4_08(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x4D, 0, 0);
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
            EmGaruga_State4_End(em, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    }
}

void EmGaruga_State4_09(Em *em, EmWork *work)
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
            EmGaruga_State4_End(em, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    }
}

void EmGaruga_State4_0A(Em *em, EmWork *work)
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
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGaruga_State4_0B(Em *em, EmWork *work)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_STATUS);
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
            em->noReact = 0;
            EmGaruga_ChangeState(em, 0, 0x22, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmGaruga_ChangeState(em, 0, 0x22, 4);
        }
        break;
    }
    Em_SpawnStatus4Effect(em, 2);
}

void EmGaruga_State4_0C(Em *em, EmWork *work)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_NO_FLINCH);
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
            EmGaruga_ChangeState(em, 4, 0xD, 3);
        }
        break;
    case 2:
        if ((s8) em->holdTimer < 5) {
            em->holdTimer = 5;
        }
        if (em->remote == 0) {
            EmGaruga_ChangeState(em, 4, 0xD, 3);
        }
        break;
    }
}

void EmGaruga_State4_0D(Em *em, EmWork *work)
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
            Em_SetMotion(em, 0x60, 0, 0);
            EmGaruga_ChangeState(em, 2, 0x17, 4);
        }
        break;
    case 2:
        if ((em->motion[0].playing == 0) && (em->remote == 0)) {
            Em_SetMotion(em, 0x60, 0, 0);
            EmGaruga_ChangeState(em, 2, 0x17, 4);
        }
        break;
    }
}

void EmGaruga_State4_0E(Em *em, EmWork *work)
{
    em->holdTimer = 5;
    Em_SetHitReactMode(em, EM_HIT_REACT_STATUS);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x61, 0, 0);
        em->noReact = 1;
        Em_RestartScript(em);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetTrapTimeGoal(em);
            EmGaruga_ChangeState(em, 4, 0x11, 3);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmGaruga_ChangeState(em, 4, 0x11, 3);
        }
        break;
    }
    Em_SpawnStatus4Effect(em, 2);
}

void EmGaruga_State4_10(Em *em, EmWork *work)
{
    f32 floorY;
    s32 ended;

    ended = 0;
    em->guardTimer = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x34, 0, 0x46);
        em->holdTimer = 0x32;
        EmGaruga_StartCurves(em, 0xE);
        EmMine_CreateTimed0(em, &em->posX);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 76.0f) != 0) {
            em->stance = 2;
            ended = EmGaruga_UpdateCurves(em) & 0xFF;
        }
        if ((ended != 0) && (em->posY <= em->floorY)) {
            Em_SetMotion(em, 0x39, 0, 0);
            em->step++;
            em->stance = 0;
            em->posY = em->floorY;
            EmGaruga_SpawnK14Type0B(em);
            CamShake_ReqAIf(em, 2);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmGaruga_State4_11(Em *em, EmWork *work)
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
            Em_SetMotion(em, 0x60, 0, 0);
            EmGaruga_ChangeState(em, 2, 0x17, 4);
        }
        break;
    case 2:
        if ((em->motion[0].playing == 0) && (em->remote == 0)) {
            Em_SetMotion(em, 0x60, 0, 0);
            EmGaruga_ChangeState(em, 2, 0x17, 4);
        }
        break;
    }
}

void EmGaruga_State4_12(Em *em, EmWork *work)
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
            Em_SetTrapTimeGoal(em);
            em->blind = 0;
            EmGaruga_ChangeState(em, 4, 0xD, 3);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmGaruga_ChangeState(em, 4, 0xD, 3);
        }
        break;
    }
}

void EmGaruga_State4_13(Em *em, EmWork *work)
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
            Em_SetTrapTimeGoal(em);
            em->blind = 0;
            EmGaruga_ChangeState(em, 4, 0xD, 3);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmGaruga_ChangeState(em, 4, 0xD, 3);
        }
        break;
    }
}

void EmGaruga_State4_14(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGaruga_State4_15(Em *em, EmWork *work)
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
            EmGaruga_ChangeState(em, 0, 0x24, 4);
        }
        break;
    case 4:
        if (em->remote == 0) {
            EmGaruga_ChangeState(em, 0, 0x24, 4);
        }
        break;
    }
}

void EmGaruga_State4_16(Em *em, EmWork *work)
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
            Em_SetTrapTimeGoal(em);
            EmGaruga_ChangeState(em, 4, 0x17, 3);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmGaruga_ChangeState(em, 4, 0x17, 3);
        }
        break;
    }
}

void EmGaruga_State4_17(Em *em, EmWork *work)
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
            EmGaruga_ChangeState(em, 2, 0x17, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmGaruga_ChangeState(em, 2, 0x17, 4);
        }
        break;
    }
}

void EmGaruga_State4_18(Em *em, EmWork *work)
{
    Em_SetHold2(em);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
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
            EmGaruga_ChangeState(em, 0, 8, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmGaruga_ChangeState(em, 0, 8, 4);
        }
        break;
    }
}

void EmGaruga_State6_00(Em *em, EmWork *work)
{
    f32 dist;
    f32 d;
    f32 turnF;
    f32 floorY;
    u32 turn;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
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
        Em_SetMotion(em, 0xE, 0, 0);
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
        dist = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer > 0) {
            if (dist <= 500.0f) {
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
            Em_SetMotion(em, 0xC, 0, 0);
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
        d = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer > 0) {
            if (d <= 500.0f) {
                goto block_11;
            }
        } else {
block_11:
            em->step++;
            em->fieldCollide = 1;
            Em_SetMotion(em, 0xF, 0, 0);
            Player_ClearVelocity(em);
            em->velY = -8.0f;
        }
        break;
    case 3:
        work->moveAngY = em->rotY;
        Em_MoveRotated(em, &work->moveAngX);
        if (!(em->floorY <= em->posY)) {
            em->step++;
            em->stance = 0;
            Em_SetMotion(em, 0x13, 0, 0);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 6, 0, 0);
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
        turnF = (16384.0f / (*(f32 *)((u8 *)em + 0x168) / 2.0f)) * em->motionSpeed;
        turn = (u32) turnF;
        em->rotY += (s32) turn;
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 1, 0, 0);
            em->timer = 0;
        }
        break;
    case 6:
        em->timer++;
        if (em->timer == 0x28) {
            Em_SetLayerMotion(em, 0x4C4, 0xA, 0, 1);
        }
        if (em->timer == 0xA) {
            Em_SetLayerMotion(em, 0x593, 0xA, 0, 2);
        }
        if (em->motion[1].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x33, 0, 0);
            em->timer = 1;
        }
        break;
    case 7:
        if (em->motion[0].playing == 0) {
            if (--em->timer <= 0) {
                em->step++;
                Em_SetMotion(em, 0x69, 0, 0);
            }
        }
        break;
    case 8:
        if ((EmMotion_ReachedFrame(em, 0, 256.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 316.0f) == 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create((Em *) em, 0x1A, 1, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 78.0f, 0U) != 0) {
            StageObj_K11_CreateAtBone(em, 0x24, 1, 0);
            StageObj_K11_CreateAtBone(em, 0x24, 2, 0);
            EmRangedAttack_CreateAtBone(em, 0x22, 0, 0, 0x5B0, 0);
        }
        if (EmMotion_PassedFrameScaled(em, 120.0f, 0U) != 0) {
            StageObj_K11_CreateAtBone(em, 0x24, 1, 0);
            StageObj_K11_CreateAtBone(em, 0x24, 2, 0);
            EmRangedAttack_CreateAtBone(em, 0x22, 0, 0, 0x4FA, 0xF1C8);
        }
        if (EmMotion_PassedFrameScaled(em, 164.0f, 0U) != 0) {
            StageObj_K11_CreateAtBone(em, 0x24, 1, 0);
            StageObj_K11_CreateAtBone(em, 0x24, 2, 0);
            EmRangedAttack_CreateAtBone(em, 0x22, 0, 0, 0x444, 0x9F5);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x33, 0, 0);
        }
        break;
    case 9:
        if (Save_TestEventFlag(0xA1) == 1) {
            em->step++;
            em->senseDelay = 0;
            em->noHitTimer = 0;
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmGaruga_State6_04_Captured(Em *em)
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
                EmGaruga_ChangeState(em, 5, 3, 4);
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

void EmGaruga_State6_05_Captured(Em *em)
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
                EmGaruga_ChangeState(em, 5, 3, 4);
                break;
            }
        } else {
            em->alpha = (f32) timer / 150.0f;
        }
        break;
    }
}

void EmGaruga_State8_00_CarveStart(Em *em, EmWork *work)
{
    EmGarugaVec v;

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
        if (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0) {
            v = EmGaruga_CarveThrowPt;
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

void EmGaruga_State8_01(Em *em, EmWork *work)
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
            EmGaruga_ChangeState(em, 8, 0x98, 1);
        }
        break;
    }
}

void EmGaruga_State8_02(Em *em, EmWork *work)
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
            EmGaruga_ChangeState(em, 8, 0x98, 1);
        }
        break;
    }
}

void EmGaruga_State8_03(Em *em, EmWork *work)
{
    f32 floorY;
    s32 ended;
    u8 step;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    step = em->step;
    ended = 0;
    switch (step) {
    case 0:
        em->step = step + 1;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x29, 0, 0);
        EmGaruga_StartCurves(em, 0xA);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 94.0f) != 0) {
            em->stance = 2;
            ended = EmGaruga_UpdateCurves(em) & 0xFF;
        }
        if ((ended != 0) && (em->posY <= (372.0f + em->floorY))) {
            em->step++;
            Em_SetMotion(em, 0x2D, 0, 0);
            EmGaruga_StartCurves(em, 0xB);
        }
        break;
    case 2:
        EmGaruga_UpdateCurves(em);
        if (em->posY <= em->floorY) {
            em->step++;
            Em_SetMotion(em, 0x57, 0, 0);
            em->stance = 0;
            em->posY = em->floorY;
            EmGaruga_SpawnK14Type0B(em);
            CamShake_ReqAIf(em, 2);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step = step + 1;
            Em_SetMotion(em, 1, 0, 0);
            EmGaruga_ChangeState(em, 8, 0x98, 1);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmGaruga_State8_98(Em *em, EmWork *work)
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
            EmGaruga_ChangeState(em, 8, 0x99, 1);
        }
        break;
    }
}

void EmGaruga_State8_99(Em *em, EmWork *work)
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
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGaruga_State5_00_Hunted(Em *em, EmWork *work)
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
                EmGaruga_ChangeState(em, 5, 3, 4);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    case 99:
        if (em->motion[0].playing == 0) {
            em->stance = 0;
            EmGaruga_EndState(em, 0, 0);
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
        }
        break;
    }
}

void EmGaruga_State5_01_Hunted(Em *em, EmWork *work)
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
                EmGaruga_ChangeState(em, 5, 3, 4);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    case 99:
        if (em->motion[0].playing == 0) {
            em->stance = 0;
            EmGaruga_EndState(em, 0, 0);
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
        }
        break;
    }
}

void EmGaruga_State5_02_HuntedFalling(Em *em, EmWork *work)
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
        if (EmMotion_PassedFrameScaled(em, 60.0f, 0) != 0) {
            em->step++;
            em->timer = 0;
            Em_SetMotion(em, 0x52, 6, 0);
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
                EmGaruga_ChangeState(em, 5, 3, 4);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    case 99:
        if (em->motion[0].playing == 0) {
            em->stance = 0;
            EmGaruga_EndState(em, 0, 0);
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
        }
        break;
    }
}

void EmGaruga_State5_03_Appear(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        if (QuestEm_CanAppear(em) == 1) {
            em->posY = gAreaGroundY[em->area];
            em->stance = 2;
            EmGaruga_ChangeState(em, 2, 1, 0);
            break;
        }
        em->taskPhase++;
        break;
    }
}

void EmGaruga_RunGroup0(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmGaruga_State0_00(em);
        break;
    case 1:
        EmGaruga_State0_01(em);
        break;
    case 2:
        EmGaruga_State0_02(em);
        break;
    case 3:
        EmGaruga_State0_03(em);
        break;
    case 4:
        EmGaruga_State0_04(em);
        break;
    case 5:
        EmGaruga_State0_05(em);
        break;
    case 6:
        EmGaruga_State0_06(em);
        break;
    case 7:
        EmGaruga_State0_07(em);
        break;
    case 8:
        EmGaruga_State0_08(em, work, 2);
        break;
    case 9:
        EmGaruga_State0_09(em);
        break;
    case 0xA:
        EmGaruga_State0_0A(em);
        break;
    case 0xB:
        EmGaruga_State0_0B(em);
        break;
    case 0xC:
        EmGaruga_State0_0C(em);
        break;
    case 0xD:
        EmGaruga_State0_0D(em);
        break;
    case 0xE:
        EmGaruga_State0_0E(em);
        break;
    case 0xF:
        EmGaruga_State0_0F(em);
        break;
    case 0x10:
        EmGaruga_State0_10(em);
        break;
    case 0x11:
        EmGaruga_State0_11(em);
        break;
    case 0x12:
        EmGaruga_State0_12(em);
        break;
    case 0x13:
        EmGaruga_State0_13(em);
        break;
    case 0x14:
        EmGaruga_State0_14(em);
        break;
    case 0x15:
        EmGaruga_State0_15(em);
        break;
    case 0x16:
        EmGaruga_State0_16(em);
        break;
    case 0x17:
        EmGaruga_State0_17(em);
        break;
    case 0x18:
        EmGaruga_State0_18(em);
        break;
    case 0x19:
        EmGaruga_State0_19(em);
        break;
    case 0x1A:
        EmGaruga_State0_1A(em);
        break;
    case 0x1B:
        EmGaruga_State0_1B(em);
        break;
    case 0x1C:
        EmGaruga_State0_1C(em);
        break;
    case 0x1D:
        EmGaruga_State0_1D(em, work);
        break;
    case 0x1F:
        EmGaruga_State0_1F(em, work);
        break;
    case 0x21:
        EmGaruga_State0_21(em);
        break;
    case 0x22:
        EmGaruga_State0_08(em, work, 0);
        break;
    case 0x23:
        EmGaruga_State0_21(em);
        break;
    case 0x24:
        EmGaruga_State0_08(em, work, 1);
        break;
    case 0x25:
        EmGaruga_State0_21(em);
        break;
    case 0x28:
        EmGaruga_State0_28(em);
        break;
    }
}

void EmGaruga_RunGroup1(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmGaruga_State1_00(em);
        break;
    case 1:
        EmGaruga_State1_01(em);
        break;
    case 2:
        EmGaruga_State1_02(em);
        break;
    case 3:
        EmGaruga_State1_03(em);
        break;
    case 4:
        EmGaruga_State1_04(em);
        break;
    case 5:
        EmGaruga_State1_05(em);
        break;
    case 6:
        EmGaruga_State1_06(em);
        break;
    case 7:
        EmGaruga_State1_07(em, work);
        break;
    case 8:
        EmGaruga_State1_04(em);
        break;
    case 9:
        EmGaruga_State1_09();
        break;
    case 10:
        EmGaruga_State1_0A(em);
        
    default:
        break;
    }
}

void EmGaruga_RunGroup2(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmGaruga_State2_00(em, work);
        break;
    case 1:
        EmGaruga_State2_01_FlyInLand(em, work);
        break;
    case 2:
        EmGaruga_State2_02(em, work);
        break;
    case 3:
        EmGaruga_State2_03(em, work);
        break;
    case 4:
        EmGaruga_State2_04(em, work);
        break;
    case 5:
        EmGaruga_State2_05(em, work);
        break;
    case 6:
        EmGaruga_State2_06(em, work);
        break;
    case 7:
        EmGaruga_State2_07(em, work);
        break;
    case 8:
        EmGaruga_State2_08(em, work);
        break;
    case 9:
        EmGaruga_State2_09(em, work);
        break;
    case 10:
        EmGaruga_State2_0A(em);
        break;
    case 11:
        EmGaruga_State2_0B(em, work);
        break;
    case 12:
        EmGaruga_State2_0C(em);
        break;
    case 13:
        EmGaruga_State2_0D(em, work);
        break;
    case 14:
        EmGaruga_State2_0E(em, work);
        break;
    case 15:
        EmGaruga_State2_0F(em, work);
        break;
    case 16:
        EmGaruga_State2_10(em, work);
        break;
    case 17:
        EmGaruga_State2_11(em, work);
        break;
    case 18:
        EmGaruga_State2_12(em, work);
        break;
    case 19:
        EmGaruga_State2_13(em, work);
        break;
    case 20:
        EmGaruga_State2_14(em, work);
        break;
    case 21:
        EmGaruga_State2_15(em, work);
        break;
    case 22:
        EmGaruga_State2_16(em, work);
        break;
    case 23:
        EmGaruga_State2_17(em, work);
        break;
    case 24:
        EmGaruga_State2_18(em, work);
        
    default:
        break;
    }
}

void EmGaruga_RunGroup3(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmGaruga_State3_00(em, work);
        break;
    case 1:
        EmGaruga_State3_01(em, work);
        break;
    case 2:
        EmGaruga_State3_02(em, work);
        break;
    case 3:
        EmGaruga_State3_03(em, work);
        break;
    case 4:
        EmGaruga_State3_04(em, work);
        break;
    case 5:
        EmGaruga_State3_05(em, work);
        break;
    case 6:
        EmGaruga_State3_06(em, work);
        break;
    case 7:
        EmGaruga_State3_07(em, work);
        break;
    case 8:
        EmGaruga_State3_08(em, work);
        break;
    case 9:
        EmGaruga_State3_09(em, work);
        break;
    case 10:
        EmGaruga_State3_0A(em, work);
        break;
    case 11:
        EmGaruga_State3_0B(em, work);
        break;
    case 12:
        EmGaruga_State3_02(em, work);
        break;
    case 13:
        EmGaruga_State3_0D(em, work);
        
    default:
        break;
    }
}

void EmGaruga_RunGroup4(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmGaruga_State4_00(em, work);
        break;
    case 1:
        EmGaruga_State4_01(em, work);
        break;
    case 2:
        EmGaruga_State4_02(em, work);
        break;
    case 3:
        EmGaruga_State4_03(em, work);
        break;
    case 4:
        EmGaruga_State4_04(em, work);
        break;
    case 5:
        EmGaruga_State4_05(em, work);
        break;
    case 6:
        EmGaruga_State4_06(em, work);
        break;
    case 7:
        EmGaruga_State4_07(em, work);
        break;
    case 8:
        EmGaruga_State4_08(em, work);
        break;
    case 9:
        EmGaruga_State4_09(em, work);
        break;
    case 10:
        EmGaruga_State4_0A(em, work);
        break;
    case 11:
        EmGaruga_State4_0B(em, work);
        break;
    case 12:
        EmGaruga_State4_0C(em, work);
        break;
    case 13:
        EmGaruga_State4_0D(em, work);
        break;
    case 14:
        EmGaruga_State4_0E(em, work);
        break;
    case 16:
        EmGaruga_State4_10(em, work);
        break;
    case 17:
        EmGaruga_State4_11(em, work);
        break;
    case 18:
        EmGaruga_State4_12(em, work);
        break;
    case 19:
        EmGaruga_State4_13(em, work);
        break;
    case 20:
        EmGaruga_State4_14(em, work);
        break;
    case 21:
        EmGaruga_State4_15(em, work);
        break;
    case 22:
        EmGaruga_State4_16(em, work);
        break;
    case 23:
        EmGaruga_State4_17(em, work);
        break;
    case 24:
        EmGaruga_State4_18(em, work);
        
    default:
        break;
    }
}

void EmGaruga_RunGroup5(Em *em, EmWork *work)
{
    em->noPushTimer = 5;
    switch (em->state) {
    case 0:
        EmGaruga_State5_00_Hunted(em, work);
        break;
    case 1:
        EmGaruga_State5_01_Hunted(em, work);
        break;
    case 2:
        EmGaruga_State5_02_HuntedFalling(em, work);
        break;
    case 3:
        EmGaruga_State5_03_Appear(em, work);
        break;
    }
}

void EmGaruga_RunGroup6(Em *em, s32 work)
{
    switch (em->state) {
    case 0:
        EmGaruga_State6_00(em, (EmWork *) work);
        break;
    case 4:
        EmGaruga_State6_04_Captured(em);
        break;
    case 5:
        EmGaruga_State6_05_Captured(em);
        break;
    }
}

void EmGaruga_RunGroup8(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0x0:
        EmGaruga_State8_00_CarveStart(em, work);
        break;
    case 0x1:
        EmGaruga_State8_01(em, work);
        break;
    case 0x2:
        EmGaruga_State8_02(em, work);
        break;
    case 0x3:
        EmGaruga_State8_03(em, work);
        break;
    case 0x98:
        EmGaruga_State8_98(em, work);
        break;
    case 0x99:
        EmGaruga_State8_99(em, work);
        break;
    }
}

void EmGaruga_Update(Em *em)
{
    u8 buf[4];
    EmFlyerWork *w = (EmFlyerWork *)&em->work;

    Em_UpdateCombatTimer(em);
    Em_RaisePlayerAreaTriggerLax(em);
    if (em->noAlert != 1 && em->rage == 0) {
        Map_DayId(gScene.map);
    }
    if (w->fxMotion != 0) {
        w->fxMotion -= 1;
    }
    switch (Em_UpdateDamage(em, buf) & 0xFF) {
    case 0:
    case 9:
    case 14:
    case 15:
        break;
    case 1:
    case 2:
        if (em->stance == 2) {
            EmGaruga_ChangeState(em, 5, 2, 3);
        } else if ((s8) em->holdTimer != 0) {
            EmGaruga_ChangeState(em, 5, 1, 3);
        } else {
            EmGaruga_ChangeState(em, 5, 0, 3);
        }
        break;
    case 3:
    case 4:
        if ((s8) em->holdTimer == 0) {
            if (em->rage != 0 && em->status == EM_STATUS_NONE) {
                EmGaruga_ChangeState(em, 4, 0x10, 3);
            } else {
                Em_SetTrapTime(em, buf);
                Em_SetTrapTimeGoal(em);
                EmGaruga_ChangeState(em, 4, 0xC, 3);
            }
        }
        break;
    case 17:
        if ((s8) em->holdTimer != 0) {
            Em_SetStatus5Timer(em);
            EmGaruga_ChangeState(em, 4, 0x16, 3);
        } else if ((em->stateGroup != 4 || em->state != 0x15) && (em->stateGroup != 4 || em->state != 8)) {
            Em_SetStatus5Timer(em);
            EmGaruga_ChangeState(em, 4, 0x15, 3);
        }
        break;
    case 18:
        Em_ResetKindTimer(em);
        EmGaruga_ChangeState(em, 4, 0x18, 3);
        break;
    case 5:
        if (em->stateGroup != 4 || em->state != 8) {
            EmGaruga_ChangeState(em, 4, 8, 3);
        }
        break;
    case 6:
        if ((s8) em->holdTimer != 0) {
            Em_SetStatus4Timer(em);
            EmGaruga_ChangeState(em, 4, 0xE, 3);
        } else if ((em->stateGroup != 4 || em->state != 0xB) && (em->stateGroup != 4 || em->state != 8)) {
            Em_SetStatus4Timer(em);
            EmGaruga_ChangeState(em, 4, 0xB, 3);
        }
        break;
    case 7:
        if (Em_IsHeld(em) == 1) {
            if ((s8) em->holdTimer != 0) {
                Em_SetStatus1Timer(em);
                EmGaruga_ChangeState(em, 6, 4, 3);
                break;
            }
            if ((s8) em->holdTimer2 != 0) {
                EmGaruga_ChangeState(em, 6, 5, 3);
                break;
            }
        }
        if ((em->stateGroup != 0 || em->state != 0x1B) && (em->stateGroup != 4 || em->state != 8)) {
            Em_SetStatus1Timer(em);
            EmGaruga_ChangeState(em, 0, 0x1B, 3);
        }
        break;
    case 8:
        if ((s8) em->holdTimer != 0) {
            Em_SetStatus2Timer(em);
            EmGaruga_ChangeState(em, 0, 0x1D, 3);
        } else if ((s8) em->holdTimer2 != 0) {
            Em_SetStatus2Timer(em);
            EmGaruga_ChangeState(em, 0, 0x14, 3);
        } else if ((em->stateGroup != 0 || em->state != 0x14) && (em->stateGroup != 4 || em->state != 8)) {
            Em_SetStatus2Timer(em);
            EmGaruga_ChangeState(em, 0, 0x14, 3);
        }
        break;
    case 10:
        switch (em->state) {
        case 0x12:
            EmGaruga_ChangeState(em, 0, 0x17, 3);
            em->scriptPending = 0;
            Em_CheckRageByHp(em, em->rageThreshold);
            break;
        case 0x14:
        case 0x15:
            EmGaruga_ChangeState(em, 0, 0x18, 3);
            em->scriptPending = 0;
            break;
        case 0x1B:
            EmGaruga_ChangeState(em, 0, 0x1C, 3);
            em->scriptPending = 0;
            break;
        case 0x1D:
            EmGaruga_ChangeState(em, 4, 0x12, 3);
            em->scriptPending = 0;
            break;
        case 0x1F:
            EmGaruga_ChangeState(em, 4, 0x13, 3);
            em->scriptPending = 0;
            break;
        }
        break;
    case 11:
        EmGaruga_ChangeState(em, 4, 4, 3);
        break;
    case 12:
    case 16:
        Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
        if (em->stance == 2) {
            EmGaruga_ChangeState(em, 4, 8, 3);
        } else {
            switch (em->flinchPart) {
            case 0:
            case 7:
                EmGaruga_ChangeState(em, 4, 0, 3);
                break;
            case 5:
            case 6:
                EmGaruga_ChangeState(em, 4, 2, 3);
                break;
            case 1:
            case 2:
                EmGaruga_ChangeState(em, 4, 3, 3);
                break;
            case 3:
            case 4:
            default:
                if (em->parts[em->flinchPart].state >= 2) {
                    if (em->flinchPart != 3) {
                        EmGaruga_ChangeState(em, 4, 5, 3);
                    } else {
                        EmGaruga_ChangeState(em, 4, 6, 3);
                    }
                } else {
                    EmGaruga_ChangeState(em, 4, 1, 3);
                }
                break;
            }
        }
        break;
    case 13:
        if (em->stance != 2) {
            EmGaruga_ChangeState(em, 4, 0, 3);
            em->scriptPending = 0;
        }
        break;
    }
    if (em->parts[6].state >= 2) {
        Em_BreakPart(em, 1);
    }
    if (em->parts[6].state >= 3) {
        Em_BreakPart(em, 2);
    }
    if (em->parts[0].state >= 2) {
        Em_BreakPart(em, 3);
    }
    if (em->parts[1].state >= 2) {
        Em_BreakPart(em, 4);
    }
    if (em->parts[2].state >= 2) {
        Em_BreakPart(em, 4);
    }
    switch (gQuest.stage) {
    case 0x1771:
        if (Area_DayId(em->area) == AREA_JUNGLE_9 && Save_TestEventFlag(0xA1) == 0) {
            if (Area_DayId(gScene.area) == AREA_JUNGLE_9 && gScene.eventRunning == 1 && em->stateGroup != 6) {
                EmGaruga_ChangeState(em, 6, 0, 1);
            }
            break;
        }
    default:
        Em_UpdatePendingScript(em);
        break;
    }
    Em_ClearHitReactMode(em);
    EmGaruga_RunState(em, (s32) w);
    if (em->stateRequest != 0) {
        EmGaruga_RunState(em, (s32) w);
        em->stateRequest = 0;
    }
    if (em->stateGroup == 2 && (s8) w->flyGuard == 1) {
        em->guardTimer = 2;
    }
}

void EmGaruga_RunState(Em *em, s32 work)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->stateGroup) {
    case 0:
        EmGaruga_RunGroup0(em, (EmWork *) work);
        break;
    case 1:
        EmGaruga_RunGroup1(em, (EmWork *) work);
        break;
    case 2:
        EmGaruga_RunGroup2(em, (EmWork *) work);
        break;
    case 3:
        EmGaruga_RunGroup3(em, (EmWork *) work);
        break;
    case 4:
        EmGaruga_RunGroup4(em, (EmWork *) work);
        break;
    case 5:
        EmGaruga_RunGroup5(em, (EmWork *) work);
        break;
    case 6:
        EmGaruga_RunGroup6(em, work);
        break;
    case 7:
        EmGaruga_RunGroup6(em, work);
        break;
    case 8:
        EmGaruga_RunGroup8(em, (EmWork *) work);
        
    default:
        break;
    }
}

void EmGaruga_RequestOnFrame(void *em, s32 frame, s32 i)
{
    if (EmMotion_PassedFrameScaled((Em *) em, (f32) frame, 0U) != 0) {
        CamShake_ReqAIf(em, i);
    }
}

static void EmGaruga_MotionEffectsDefault(Em *em)
{
}

void EmGaruga_MotionEffects(Em *em, EmGarugaFxWork *work)
{
    f32 v4[3];
    f32 v3[3];
    f32 v2[3];
    f32 v1[3];

    if (em->motionId[0] != work->motion) {
        work->motion = em->motionId[0];
    }
    EmGaruga_MotionSounds(em, (EmWork *) work);
    switch (work->motion) {
    case 0x3EB:
        EmGaruga_RequestOnFrame(em, 0x34, 1);
        EmGaruga_RequestOnFrame(em, 0x74, 1);
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            GarugaAttack_Create(em, 2);
        }
        if (EmMotion_PassedFrameScaled(em, 50.0f, 0) != 0) {
            GarugaAttack_Create(em, 3);
        }
        if (EmMotion_PassedFrameScaled(em, 120.0f, 0) != 0) {
            GarugaAttack_Create(em, 4);
        }
        break;
    case 0x3EC:
        if (EmMotion_PassedFrameScaled(em, 38.0f, 0) != 0) {
            StageObj_K14_Create((Em *) em, 3, 0, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 74.0f, 0) != 0) {
            StageObj_K14_Create((Em *) em, 2, 0, 1.0f);
        }
        break;
    case 0x3ED:
        EmGaruga_RequestOnFrame(em, 0x3C, 1);
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            GarugaAttack_Create(em, 5);
        }
        if (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) {
            GarugaAttack_Create(em, 6);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 2.0f, 78.0f) != 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create((Em *) em, 0x1A, 1, 1.0f);
        }
        break;
    case 0x3EE:
        EmGaruga_RequestOnFrame(em, 0x3C, 1);
        if (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) {
            GarugaAttack_Create(em, 7);
        }
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            GarugaAttack_Create(em, 8);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 2.0f, 78.0f) != 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create((Em *) em, 0x1A, 0, 1.0f);
        }
        break;
    case 0x3EF:
        if (EmMotion_PassedFrameScaled(em, 18.0f, 0) != 0) {
            GarugaAttack_Create(em, 9);
        }
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            GarugaAttack_Create(em, 0xA);
        }
        if (em->rage != 0) {
            if ((EmMotion_InFrameRange(em, 0, 2.0f, 18.0f) != 0) && !(gScene.frame & 3)) {
                StageObj_K14_Create((Em *) em, 0x1A, 1, 1.0f);
            }
            if ((EmMotion_InFrameRange(em, 0, 20.0f, 70.0f) != 0) && !(gScene.frame & 3)) {
                StageObj_K14_Create((Em *) em, 0x1A, 0, 1.0f);
                break;
            }
        }
        break;
    case 0x3F0:
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            GarugaAttack_Create(em, 0xB);
        }
        if (EmMotion_PassedFrameScaled(em, 18.0f, 0) != 0) {
            GarugaAttack_Create(em, 0xC);
        }
        if (em->rage != 0) {
            if ((EmMotion_InFrameRange(em, 0, 2.0f, 18.0f) != 0) && !(gScene.frame & 3)) {
                StageObj_K14_Create((Em *) em, 0x1A, 0, 1.0f);
            }
            if ((EmMotion_InFrameRange(em, 0, 20.0f, 70.0f) != 0) && !(gScene.frame & 3)) {
                StageObj_K14_Create((Em *) em, 0x1A, 1, 1.0f);
                break;
            }
        }
        break;
    case 0x3F2:
        if (EmMotion_PassedFrameScaled(em, 42.0f, 0) != 0) {
            GarugaAttack_Create(em, 0x19);
        }
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            GarugaAttack_Create(em, 0x1A);
        }
        if (EmMotion_PassedFrameScaled(em, 74.0f, 0) != 0) {
            GarugaAttack_Create(em, 0x1A);
        }
        break;
    case 0x3F3:
        if (EmMotion_PassedFrameScaled(em, 52.0f, 0) || EmMotion_PassedFrameScaled(em, 104.0f, 0) || EmMotion_PassedFrameScaled(em, 162.0f, 0)) {
            Effect0D_SpawnForEmScaled(em, 2, 7, 5.0f);
        }
        break;
    case 0x3F5:
        if ((EmMotion_PassedFrameScaled(em, 26.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 52.0f, 0) != 0)) {
            Effect0D_SpawnForEmScaled(em, 1, 7, 5.0f);
        }
        break;
    case 0x3F6:
    case 0x43D:
    case 0x43E:
    case 0x440:
    case 0x441:
        if ((em->rage != 0) && (((s32) gScene.frame % 10) == 0)) {
            StageObj_K14_Create((Em *) em, 0x19, ((u16) Rand_Next(1) & 1) + 2, 1.0f);
        }
        break;
    case 0x3F7:
        if (EmMotion_PassedFrameScaled(em, 8.0f, 0) || EmMotion_PassedFrameScaled(em, 86.0f, 0) || EmMotion_PassedFrameScaled(em, 160.0f, 0)) {
            if (em->posY <= (1000.0f + em->floorY)) {
                StageObj_K14_Create((Em *) em, 1, 0, 1.0f);
            }
        }
        if (em->rage != 0) {
            if (EmMotion_InFrameRange(em, 0, 70.0f, 84.0f) || EmMotion_InFrameRange(em, 0, 146.0f, 156.0f) || EmMotion_InFrameRange(em, 0, 220.0f, 234.0f)) {
                if (!(gScene.frame & 3)) {
                    StageObj_K14_Create((Em *) em, 0x1A, (u16) Rand_Next(1) & 1, 1.0f);
                    break;
                }
            }
        }
        break;
    case 0x3F9:
        if (EmMotion_PassedFrameScaled(em, 18.0f, 0) != 0) {
            GarugaAttack_Create(em, 0x13);
        }
        if ((EmMotion_PassedFrameScaled(em, 12.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 72.0f, 0) != 0)) {
            StageObj_K14_Create((Em *) em, 2, 0, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 40.0f, 0) != 0) {
            StageObj_K14_Create((Em *) em, 3, 0, 1.0f);
        }
        break;
    case 0x3FA:
        if (EmMotion_PassedFrameScaled(em, 42.0f, 0) || EmMotion_PassedFrameScaled(em, 102.0f, 0) || EmMotion_PassedFrameScaled(em, 156.0f, 0)) {
            if (em->posY <= (1000.0f + em->floorY)) {
                StageObj_K14_Create((Em *) em, 1, 0, 1.0f);
            }
        }
        if ((em->rage != 0) && (EmMotion_PassedFrameScaled(em, 38.0f, 0) != 0)) {
            StageObj_K14_Create((Em *) em, 0x19, 5, 1.0f);
            StageObj_K14_Create((Em *) em, 0x19, 4, 1.0f);
        }
        break;
    case 0x3FB:
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            GarugaAttack_Create(em, 0xD);
        }
        if (EmMotion_PassedFrameScaled(em, 16.0f, 0) != 0) {
            EmGaruga_SpawnK14Type0B(em);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 18.0f, 44.0f) != 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create((Em *) em, 0x1A, (u16) Rand_Next(1) & 1, 1.0f);
        }
        break;
    case 0x407:
        v4[1] = 10.0f;
        v4[2] = 140.0f;
        v4[0] = 0.0f;
        Em_SpawnLyingPuffs(em, 0x22, v4, 1.6f);
        break;
    case 0x40A:
        v3[1] = 10.0f;
        v3[2] = 140.0f;
        v3[0] = 0.0f;
        Em_SpawnLyingPuffs(em, 0x22, v3, 1.6f);
        break;
    case 0x40C:
        if (EmMotion_PassedFrameScaled(em, 50.0f, 0) != 0) {
            GarugaAttack_Create(em, 1);
        }
        if (EmMotion_PassedFrameScaled(em, 46.0f, 0) != 0) {
            GarugaAttack_Create(em, 0x21);
        }
        break;
    case 0x40E:
        if (EmMotion_PassedFrameScaled(em, 148.0f, 0) != 0) {
            GarugaAttack_Create(em, 0xF);
        }
        if (EmMotion_PassedFrameScaled(em, 156.0f, 0) != 0) {
            StageObj_K14_Create((Em *) em, 0, 0, 1.0f);
        }
        break;
    case 0x410:
        if (EmMotion_PassedFrameScaled(em, 60.0f, 0) != 0) {
            Mtx_GetTrans(v2, em->model->bones[35].world);
            StageObj_K0F_CreateOnEm(em, 5, 1, 1.0f);
            GarugaAttack_CreateForEffects(em, 0x16);
        }
        break;
    case 0x411:
        if (EmMotion_PassedFrameScaled(em, 108.0f, 0) != 0) {
            GarugaAttack_Create(em, 0x25);
        }
        if (EmMotion_PassedFrameScaled(em, 104.0f, 0) != 0) {
            StageObj_K14_Create((Em *) em, 1, 0, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 200.0f, 0) != 0) {
            StageObj_K14_Create((Em *) em, 1, 0, 1.0f);
        }
        break;
    case 0x412:
        EmGaruga_RequestOnFrame(em, 0x2C, 1);
        EmGaruga_RequestOnFrame(em, 0x49, 1);
        if (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) {
            if (em->state == 0xC) {
                GarugaAttack_Create(em, 0x27);
            } else {
                GarugaAttack_Create(em, 0x11);
            }
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 6.0f, 38.0f) != 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create((Em *) em, 0x1A, 0, 1.0f);
        }
        break;
    case 0x413:
        EmGaruga_RequestOnFrame(em, 0x2C, 1);
        EmGaruga_RequestOnFrame(em, 0x49, 1);
        if (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) {
            if (em->state == 0xC) {
                GarugaAttack_Create(em, 0x27);
            } else {
                GarugaAttack_Create(em, 0x11);
            }
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 6.0f, 38.0f) != 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create((Em *) em, 0x1A, 1, 1.0f);
        }
        break;
    case 0x417:
        if (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) {
            GarugaAttack_Create(em, 0x22);
        }
        break;
    case 0x41B:
        if (em->rage != 0) {
            if (EmMotion_InFrameRange(em, 0, 72.0f, 80.0f) || EmMotion_InFrameRange(em, 0, 132.0f, 140.0f) || EmMotion_InFrameRange(em, 0, 184.0f, 192.0f)) {
                StageObj_K14_Create((Em *) em, 0x1A, (u16) Rand_Next(1) & 1, 1.0f);
                break;
            }
        }
        break;
    case 0x41C:
        if ((em->stateGroup == 2) && (em->state == 0xC) && (EmMotion_PassedFrameScaled(em, 70.0f, 0) != 0)) {
            Mtx_GetTrans(v1, em->model->bones[35].world);
            if (Player_InCurrentArea(em) & 0xFF) {
                StageObj_K0F_CreateAtMarked(v1, 4, 0, 1.0f);
            }
            GarugaAttack_CreateForEffects(em, 0x16);
        }
        if (EmMotion_PassedFrameScaled(em, 72.0f, 0) != 0) {
            StageObj_K14_Create((Em *) em, 4, 0, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 80.0f, 0) != 0) {
            StageObj_K14_Create((Em *) em, 1, 0, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 84.0f, 0) != 0) {
            StageObj_K14_Create((Em *) em, 4, 1, 1.0f);
        }
        break;
    case 0x421:
        if (EmMotion_PassedFrameScaled(em, 12.0f, 0) != 0) {
            StageObj_K14_Create((Em *) em, 4, 0, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) {
            StageObj_K14_Create((Em *) em, 5, 0, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 28.0f, 0) != 0) {
            GarugaAttack_Create(em, 0x1B);
        }
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            GarugaAttack_Create(em, 0x1F);
        }
        if (EmMotion_PassedFrameScaled(em, 36.0f, 0) != 0) {
            GarugaAttack_Create(em, 0x20);
        }
        break;
    case 0x424:
        if (EmMotion_PassedFrameScaled(em, 44.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x1B, 7, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 62.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x16, 7, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 80.0f, 0) != 0) {
            StageObj_K14_Create((Em *) em, 1, 0x80, 0.5f);
        }
        break;
    case 0x425:
        if (EmMotion_PassedFrameScaled(em, 24.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x1A, 7, 1.0f);
        }
        break;
    case 0x429:
        if (EmMotion_PassedFrameScaled(em, 20.0f, 0) != 0) {
            StageObj_K14_Create((Em *) em, 3, 0x80, 1.0f);
        }
        if ((EmMotion_PassedFrameScaled(em, 128.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 164.0f, 0) != 0)) {
            StageObj_K14_Create((Em *) em, 3, 0x80, 0.6f);
        }
        if (EmMotion_PassedFrameScaled(em, 60.0f, 0) != 0) {
            StageObj_K14_Create((Em *) em, 1, 0x80, 1.0f);
        }
        break;
    case 0x42C:
        if (EmMotion_PassedFrameScaled(em, 270.0f, 0) != 0) {
            StageObj_K14_Create((Em *) em, 0x12, 0x80, 0.9f);
        }
        break;
    case 0x42D:
        EmGaruga_RequestOnFrame(em, 0x1A, 2);
        if (EmMotion_PassedFrameScaled(em, 26.0f, 0) != 0) {
            StageObj_K14_Create((Em *) em, 0x12, 0x80, 1.0f);
        }
        break;
    case 0x432:
        EmGaruga_RequestOnFrame(em, 0x1A, 2);
        if (EmMotion_PassedFrameScaled(em, 26.0f, 0) != 0) {
            StageObj_K14_Create((Em *) em, 0x12, 0x81, 1.0f);
        }
        break;
    case 0x436:
        if (EmMotion_PassedFrameScaled(em, 8.0f, 0) != 0) {
            StageObj_K14_Create((Em *) em, 0xB, 0, 1.0f);
        }
        break;
    case 0x437:
        if (EmMotion_PassedFrameScaled(em, 48.0f, 0) || EmMotion_PassedFrameScaled(em, 106.0f, 0) || EmMotion_PassedFrameScaled(em, 174.0f, 0)) {
            GarugaAttack_Create(em, 0x10);
        }
        break;
    case 0x43C:
        if (EmMotion_PassedFrameScaled(em, 42.0f, 0) || EmMotion_PassedFrameScaled(em, 60.0f, 0) || EmMotion_PassedFrameScaled(em, 80.0f, 0)) {
            StageObj_K14_Create((Em *) em, 1, 0, 1.0f);
        }
        break;
    case 0x43F:
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            GarugaAttack_Create(em, 0x26);
        }
        break;
    case 0x442:
        if ((EmMotion_PassedFrameScaled(em, 50.0f, 0) != 0) && (em->posY <= (1000.0f + em->floorY))) {
            StageObj_K14_Create((Em *) em, 9, 0, 1.0f);
            StageObj_K14_Create((Em *) em, 0xE, 0, 1.0f);
        }
        if ((EmMotion_PassedFrameScaled(em, 102.0f, 0) != 0) && (em->posY <= (1000.0f + em->floorY))) {
            StageObj_K14_Create((Em *) em, 9, 0, 1.0f);
            StageObj_K14_Create((Em *) em, 0xE, 0, 1.0f);
        }
        break;
    case 0x443:
        if (EmMotion_PassedFrameScaled(em, 34.0f, 0) != 0) {
            StageObj_K14_Create((Em *) em, 3, 0, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 70.0f, 0) != 0) {
            StageObj_K14_Create((Em *) em, 2, 0, 1.0f);
        }
        if ((EmMotion_ReachedFrame(em, 0, 96.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 120.0f) == 0) && (((s32) gScene.frame % 6) == 0)) {
            StageObj_K14_Create((Em *) em, 0x12, 0, 1.0f);
        }
        if ((em->rage != 0) && (((s32) gScene.frame % 19) == 0)) {
            StageObj_K14_Create((Em *) em, 0x19, ((u16) Rand_Next(1) & 1) + 2, 1.0f);
        }
        break;
    case 0x447:
        if (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0) {
            StageObj_K14_Create((Em *) em, 0xB, 0x80, 1.0f);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 4.0f, 60.0f) != 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create((Em *) em, 0x1A, (u16) Rand_Next(1) & 1, 1.0f);
        }
        break;
    case 0x448:
        if (em->rage != 0) {
            if (EmMotion_InFrameRange(em, 0, 12.0f, 24.0f) || EmMotion_InFrameRange(em, 0, 46.0f, 90.0f)) {
                if (!(gScene.frame & 3)) {
                    StageObj_K14_Create((Em *) em, 0x1A, (u16) Rand_Next(1) & 1, 1.0f);
                    break;
                }
            }
        }
        break;
    case 0x44C:
        if (EmMotion_PassedFrameScaled(em, 118.0f, 0) || EmMotion_PassedFrameScaled(em, 164.0f, 0) || EmMotion_PassedFrameScaled(em, 204.0f, 0)) {
            GarugaAttack_Create(em, 0x23);
        }
        if (em->rage != 0) {
            if (EmMotion_InFrameRange(em, 0, 54.0f, 250.0f) || EmMotion_InFrameRange(em, 0, 282.0f, 296.0f)) {
                if (!(gScene.frame & 3)) {
                    StageObj_K14_Create((Em *) em, 0x1A, (u16) Rand_Next(1) & 1, 1.0f);
                    break;
                }
            }
        }
        break;
    case 0x44D:
        if (EmMotion_PassedFrameScaled(em, 52.0f, 0) != 0) {
            StageObj_K14_Create((Em *) em, 1, 0x80, 1.0f);
        }
        if (em->rage != 0) {
            if (EmMotion_InFrameRange(em, 0, 10.0f, 68.0f) || EmMotion_InFrameRange(em, 0, 84.0f, 96.0f)) {
                if (!(gScene.frame & 3)) {
                    StageObj_K14_Create((Em *) em, 0x1A, (u16) Rand_Next(1) & 1, 1.0f);
                    break;
                }
            }
        }
        break;
    case 0x44E:
        if ((EmMotion_PassedFrameScaled(em, 32.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 54.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 74.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 92.0f, 0) != 0)) {
            GarugaAttack_Create(em, 0x18);
        }
        if (em->rage != 0) {
            if ((EmMotion_InFrameRange(em, 0, 10.0f, 20.0f) != 0) && !(gScene.frame & 3)) {
                StageObj_K14_Create((Em *) em, 0x1A, (u16) Rand_Next(1) & 1, 1.0f);
            }
            if ((EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 54.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 72.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 92.0f, 0) != 0)) {
                StageObj_K14_Create((Em *) em, 0x1A, (u16) Rand_Next(1) & 1, 1.0f);
                break;
            }
        }
        break;
    case 0x450:
        if (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) {
            GarugaAttack_Create(em, 0x17);
        }
        if ((EmMotion_PassedFrameScaled(em, 18.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 60.0f, 0) != 0)) {
            Effect0D_SpawnForEmScaled(em, 0x1A, 3, 4.0f);
        }
        if ((EmMotion_PassedFrameScaled(em, 38.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 80.0f, 0) != 0)) {
            Effect0D_SpawnForEmScaled(em, 0x14, 3, 4.0f);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 54.0f, 112.0f) != 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create((Em *) em, 0x1A, (u16) Rand_Next(1) & 1, 1.0f);
        }
        break;
    case 0x452:
        if (EmMotion_PassedFrameScaled(em, 20.0f, 0) != 0) {
            GarugaAttack_Create(em, 0x12);
        }
        if ((EmMotion_PassedFrameScaled(em, 50.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 76.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 96.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 116.0f, 0) != 0)) {
            GarugaAttack_Create(em, 0x18);
        }
        if (EmMotion_PassedFrameScaled(em, 20.0f, 0) != 0) {
            StageObj_K14_Create((Em *) em, 3, 0, 1.0f);
        }
        if (em->rage != 0) {
            if ((EmMotion_InFrameRange(em, 0, 24.0f, 42.0f) != 0) && !(gScene.frame & 3)) {
                StageObj_K14_Create((Em *) em, 0x1A, (u16) Rand_Next(1) & 1, 1.0f);
            }
            if ((EmMotion_PassedFrameScaled(em, 54.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 80.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 100.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 122.0f, 0) != 0)) {
                StageObj_K14_Create((Em *) em, 0x1A, (u16) Rand_Next(1) & 1, 1.0f);
                break;
            }
        }
        break;
    case 0x455:
        if (EmMotion_PassedFrameScaled(em, 20.0f, 0) != 0) {
            GarugaAttack_Create(em, 0xE);
        }
        if (EmMotion_PassedFrameScaled(em, 20.0f, 0) || EmMotion_PassedFrameScaled(em, 92.0f, 0) || EmMotion_PassedFrameScaled(em, 152.0f, 0)) {
            Effect0D_SpawnForEmScaled(em, 2, 7, 3.0f);
        }
        break;
    default:
        EmGaruga_MotionEffectsDefault(em);
        break;
    case 0x3E9:
    case 0x3F4:
    case 0x405:
    case 0x408:
    case 0x409:
    case 0x40D:
    case 0x40F:
    case 0x415:
    case 0x418:
    case 0x41E:
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
    case 0x449:
    case 0x44A:
    case 0x451:
    case 0x454:
        break;
    }
}

void EmGaruga_UpdateMotionEffects(Em *em)
{
    EmFlyerWork *w = (EmFlyerWork *)&em->work;

    switch (w->fxStep) {
    case 0:
        w->fxStep++;
        break;
    case 1:
        EmGaruga_MotionEffects(em, (EmGarugaFxWork *) w);
        break;
    }
}

void EmGaruga_SpawnK14Type0B(Em *em)
{
    StageObj_K14_Create((Em *) em, 0xB, 0, 1.0f);
}

void EmGaruga_State3_End(Em *em, EmWork *work)
{
    EmGaruga_EndState(em, 0, 0);
}

void EmGaruga_TableNop(void)
{
}

s32 EmGaruga_AllowHit8(Em *em)
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
        if ((em->stance != 2) && ((s8) em->partBreakCount == 0)) {
            return 1;
        }
        goto block_21;
    }
block_21:
    return 0;
}

void EmGaruga_SetGroup0State(Em *em, u16 state, u8 mode)
{
    em->motionSpeed = 1.0f;
    switch (state) {
    case 1:
        if (em->combat == 1) {
            state = 0x11;
        }
        break;
    case 35:
        Em_CheckRageByHp(em, (s32) em->rageThreshold);
        break;
    case 37:
        Em_StartPendingRage(em);
        break;
    case 8:
        em->holdTimer2 = 0;
        break;
    case 34:
        Em_EndStatus4(em);
        break;
    case 36:
        Em_EndStatus5(em);
        break;
    case 40:
        Em_StartGoalAction(em, 0x2710);
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
    }
    em->stance = 0;
    Em_SetStateSync(em, 0, state, mode);
}

void EmGaruga_SetGroup1State(Em *em, u16 state, u8 mode)
{
    em->stance = 0;
    em->motionSpeed = 1.0f;
    switch (state) {
    case 0:
        if (em->hp < (s16) (0.3f * em->hpMax)) {
            state = 6;
            goto state6;
        }
        Em_SetSpeedScaled(em, 0, 1000.0f, 500.0f);
        if (em->remote == 0 && em->goalKind == 0) {
            em->timer = (em->stateRand % 120 + 180) / 2;
        }
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
        em->motionSpeed = 0.8f;
    case 2:
    case 4:
    case 10:
        Em_SetSpeedScaled(em, 2, 1000.0f, 500.0f);
        if (em->curGoalKind == 0) {
            em->timer = (em->stateRand % 120 + 180) / 2;
        }
        break;
    case 6:
    state6:
        Em_SetSpeedScaled(em, 6, em->goalDist, 0.0f);
        if (em->curGoalKind == 0) {
            em->timer = (em->stateRand % 120 + 180) / 2;
        }
        break;
    case 5:
    case 7:
    case 9:
        break;
    }
    Em_SetStateSync(em, 1, state, mode);
}

void EmGaruga_SetGroup2State(Em *em, u16 state, u8 mode)
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
        if (em->remote == 0 && em->goalKind != 3) {
            EmGaruga_ChangeState(em, 2, 0, 1);
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

void EmGaruga_SetGroup3State(Em *em, u16 state, u8 mode)
{
    EmFlyerWork *w = (EmFlyerWork *)&em->work;
    f32 pos[3];
    f32 f;

    em->motionSpeed = 1.0f;
    switch (state) {
    case 0:
    case 3:
    case 5:
        Em_SetTargetAngle(em);
        break;
    case 8:
    case 9:
    case 10:
        w->countdown = 1;
        Em_SetTargetAngle(em);
        break;
    case 4:
        if (em->targetNo == -1) {
            em->timer = 150;
            em->hasGoal = 0;
        } else {
            em->hasGoal = 1;
            f = Vec_DistanceXZ(&em->posX, em->goal);
            em->goalDist = f;
            em->timer = (s32) (f / 30.0f) + 30;
        }
        break;
    case 7:
        w->countdown = 1;
        if (em->remote == 0 && em->curGoalKind == 1 && em->curGoalArg == 0) {
            if (em->targetNo == -1) {
                Em_SetTargetAngle(em);
                break;
            }
            Em_GetTargetPos(em, em->targetNo, pos);
            f = em->playerDistXZ[em->targetNo];
            if (f <= 1000.0f) {
                f = pos[1];
                if (f <= em->posY + 50.0f) {
                    state = 8;
                } else if (f <= em->posY + 150.0f) {
                    state = 7;
                } else {
                    state = 9;
                }
            } else if (f <= 2000.0f) {
                f = pos[1];
                if (f <= em->posY - 150.0f) {
                    state = 8;
                } else if (f <= em->posY + 300.0f) {
                    state = 7;
                } else {
                    state = 9;
                }
            } else {
                f = pos[1];
                if (f <= em->posY - 200.0f) {
                    state = 8;
                } else if (f <= em->posY + 400.0f) {
                    state = 7;
                } else {
                    state = 9;
                }
            }
        }
        Em_SetTargetAngle(em);
        break;
    case 1:
    case 2:
    case 6:
    case 11:
    case 12:
    case 13:
        break;
    }
    Em_SetStateSync(em, 3, state, mode);
}

void EmGaruga_SetGroup4State(Em *em, u16 state, u8 mode)
{
    em->motionSpeed = 1.0f;
    switch (state) {
    case 4:
        Em_EndAllStatus(em);
        em->fxFlags |= 1;
        em->partBreakCount = (s8) em->partBreakCount + 1;
        break;
    case 11:
        Em_BeginStatus4(em);
        break;
    case 12:
        em->holdTimer = 0x32;
        Em_EndAllStatus(em);
        break;
    case 14:
        Em_BeginStatus4(em);
        break;
    case 16:
        em->guardTimer = 2;
        Em_EndAllStatus(em);
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
        Em_ResetToBasePose(em);
        Em_BreakPart(em, 5U);
        break;
    case 21:
        Em_BeginStatus5(em);
        break;
    case 22:
        Em_BeginStatus5(em);
        break;
    case 23:
        Em_EndStatus5(em);
        break;
    case 24:
        Em_EndAllStatus(em);
        break;
    }
    Em_SetStateSync(em, 4, state, mode);
}

void EmGaruga_ChangeState(Em *em, u16 group, u16 state, u8 mode)
{
    if (em->remote == 0) {
        Em_TargetPlayerInRange(em);
        goto block_5;
    }
    if ((mode == 3) || (mode == 4)) {
        return;
    }
block_5:
    Em_LoadReactionSet(em, 0);
    switch (group) {
    case 0:
        EmGaruga_SetGroup0State(em, state, mode);
        return;
    case 1:
        EmGaruga_SetGroup1State(em, state, mode);
        return;
    case 2:
        EmGaruga_SetGroup2State(em, state, mode);
        return;
    case 3:
        EmGaruga_SetGroup3State(em, state, mode);
        return;
    case 4:
        EmGaruga_SetGroup4State(em, state, mode);
        return;
    case 5:
        if ((state == 3) && (QuestEm_CanAppear(em) == 1)) {
            QuestEm_Appear(em);
        }
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        return;
    case 8:
        if (state == 0x98) {
            Versus_MarkDefeat(em);
        }
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        return;
    case 6:
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        
    default:
        return;
    }
}

void EmGaruga_MoveByVelYCurve(Em *em, s32 kind)
{
    EmWork *work = (EmWork *) &em->work;
    EmCurveKey *tbl;
    EmCurveKey *p;
    f32 frame;
    f32 t;
    s32 i;

    tbl = EmGaruga_MoveVelYCurves[kind];
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

void EmGaruga_StartCurves(Em *em, u8 kind)
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

static s32 EmGaruga_CurveNone(Em *em, EmWork *work)
{
    return 0;
}


extern EmCurveKey *EmGaruga_VelYCurves[];

/* The value of curve work[0x4F] at the frame in the work area (a table of keys): sets
 * em->velY from the key pair around the frame, 2 at the table's end. */
static u8 EmGaruga_CurveVelY(Em *em, EmWork *work)
{
    EmFlyerWork *w = (EmFlyerWork *) work;
    EmCurveKey *p;
    f32 frame;
    f32 t;
    s32 i;
    EmCurveKey *tbl;
    u8 ret;

    tbl = EmGaruga_VelYCurves[((EmFlyerWork *)work)->curveKind];
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

extern EmCurveKey *EmGaruga_SpeedCurves[];

/* The value of curve work[0x4F] at the frame in the work area: sets em->velZ from the key
 * pair around the frame, 4 at the table's end. */
static u8 EmGaruga_CurveSpeed(Em *em, EmWork *work)
{
    EmFlyerWork *w = (EmFlyerWork *) work;
    EmCurveKey *p;
    f32 frame;
    f32 t;
    s32 i;
    EmCurveKey *tbl;
    u8 ret;

    tbl = EmGaruga_SpeedCurves[((EmFlyerWork *)work)->curveKind];
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
u8 EmGaruga_UpdateCurves(Em *em)
{
    EmFlyerWork *w = (EmFlyerWork *)&em->work;
    u8 ret = 0;

    if (w->curveNone != 0) {
        ret = EmGaruga_CurveNone(em, (EmWork *) w);
    }
    if (w->curveVelY != 0) {
        ret |= EmGaruga_CurveVelY(em, (EmWork *) w);
    }
    if (w->curveSpeed != 0) {
        ret |= EmGaruga_CurveSpeed(em, (EmWork *) w);
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

void EmHermitaur_MotionSounds(Em *em, EmWork *work)
{
    u8 type;

    type = em->type;
    if ((type == 0x30) || (type == 0x43)) {
                switch (work->fxMotion) {
        case 0x3E9:
            Em_PlaySeAtBoneVolOnFrame(em, 4, Sound_RandomSe(0x3C, 6, 0x3D, 2), 0xE, 0xA);
            Em_PlaySeAtBoneVolOnFrame(em, 0x22, Sound_RandomSe(0x3E, 6, 0x3C, 2), 0xE, 0xA);
            Em_PlaySeAtBoneVolOnFrame(em, 0x5A, Sound_RandomSe(0x3D, 6, 0x3E, 2), 0xE, 0xA);
            Em_PlaySeAtBoneVolOnFrame(em, 0x96, Sound_RandomSe(0x3C, 6, 0x3D, 2), 0xE, 0xA);
            Em_PlaySeAtBoneVolOnFrame(em, 0xDE, Sound_RandomSe(0x3E, 6, 0x3C, 2), 0xE, 0xA);
            Em_PlaySeAtBoneVolOnFrame(em, 0x13A, Sound_RandomSe(0x3D, 6, 0x3E, 2), 0xE, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0xA8, 4, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x122, 5, 0x15);
            break;
        case 0x3EA:
            Em_PlaySeAtBoneVolOnFrame(em, 0x16, Sound_RandomSe(0x3C, 4, 0x3D, 4), 0xE, 0xA);
            Em_PlaySeAtBoneVolOnFrame(em, 0x40, Sound_RandomSe(0x3E, 4, 0x3C, 4), 0xE, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0x32, Sound_RandomSe(0xF, 4, 0x11, 4), 2);
            Em_PlaySeAtBoneOnFrame(em, 0x2A, 0, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0x54, 1, 0x15);
            Em_PlaySeAtBoneOnFrame(em, 0x5C, 0, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x20, 1, 0x1E);
            break;
        case 0x3EB:
            Em_PlaySeAtBoneVolOnFrame(em, 0x14, Sound_RandomSe(0x3C, 4, 0x3D, 4), 0xE, 0xA);
            Em_PlaySeAtBoneVolOnFrame(em, 0x48, Sound_RandomSe(0x3E, 4, 0x3C, 4), 0xE, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0x32, Sound_RandomSe(0xF, 4, 0x11, 4), 2);
            Em_PlaySeAtBoneOnFrame(em, 2, 0, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0x3C, 1, 0x15);
            Em_PlaySeAtBoneOnFrame(em, 0x34, 0, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 8, 1, 0x1E);
            break;
        case 0x3ED:
            Em_PlaySeAtBoneVolOnFrame(em, 0x12, Sound_RandomSe(0x3C, 4, 0x3D, 4), 0xE, 0xA);
            Em_PlaySeAtBoneVolOnFrame(em, 0x40, Sound_RandomSe(0x3E, 6, 0x3C, 2), 0xE, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0x24, Sound_RandomSe(0xF, 4, 0x11, 4), 2);
            Em_PlaySeAtBoneOnFrame(em, 0x1C, 0, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0x28, 1, 0x15);
            Em_PlaySeAtBoneOnFrame(em, 0x48, 0, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0x54, 1, 0x15);
            Em_PlaySeAtBoneOnFrame(em, 0x12, 0, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x20, 1, 0x1E);
            Em_PlaySeAtBoneOnFrame(em, 0x3E, 0, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x4E, 1, 0x1E);
            break;
        case 0x3EE:
            Em_PlaySeAtBoneVolOnFrame(em, 0x12, Sound_RandomSe(0x3C, 4, 0x3D, 4), 0xE, 0xA);
            Em_PlaySeAtBoneVolOnFrame(em, 0x40, Sound_RandomSe(0x3E, 6, 0x3C, 2), 0xE, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0x24, Sound_RandomSe(0xF, 4, 0x11, 4), 2);
            Em_PlaySeAtBoneOnFrame(em, 0x1C, 0, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x28, 1, 0x1E);
            Em_PlaySeAtBoneOnFrame(em, 0x48, 0, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x54, 1, 0x1E);
            Em_PlaySeAtBoneOnFrame(em, 0x12, 0, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0x20, 1, 0x15);
            Em_PlaySeAtBoneOnFrame(em, 0x3E, 0, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0x4E, 1, 0x15);
            break;
        case 0x3EF:
            Em_PlaySeAtBoneVolOnFrame(em, 4, Sound_RandomSe(0x3F, 4, 0x40, 4), 0xE, 0xA);
            Em_PlaySeAtBoneVolOnFrame(em, 0x2E, Sound_RandomSe(0x3D, 6, 0x3F, 2), 0xE, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 2, 8, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 8, 9, 6);
            Em_PlaySeAtBoneOnFrame(em, 0xC, 0, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x1A, 1, 0x1E);
            Em_PlaySeAtBoneOnFrame(em, 0x2C, 0, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x38, 1, 0x1E);
            Em_PlaySeAtBoneOnFrame(em, 0x12, 0, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0x1E, 1, 0x15);
            Em_PlaySeAtBoneOnFrame(em, 0x34, 0, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0x3E, 1, 0x15);
            break;
        case 0x3F0:
            Em_PlaySeAtBoneVolOnFrame(em, 4, Sound_RandomSe(0x3F, 4, 0x40, 4), 0xE, 0xA);
            Em_PlaySeAtBoneVolOnFrame(em, 0x2E, Sound_RandomSe(0x3D, 6, 0x3F, 2), 0xE, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 2, 8, 6);
            Em_PlaySeAtBoneOnFrame(em, 8, 9, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0xC, 0, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0x1A, 1, 0x15);
            Em_PlaySeAtBoneOnFrame(em, 0x2C, 0, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0x38, 1, 0x15);
            Em_PlaySeAtBoneOnFrame(em, 0x12, 0, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x1E, 1, 0x1E);
            Em_PlaySeAtBoneOnFrame(em, 0x36, 0, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x3E, 1, 0x1E);
            break;
        case 0x3F1:
            Em_PlaySeAtBoneVolOnFrame(em, 4, Sound_RandomSe(0x3C, 6, 0x3D, 2), 0xE, 0xA);
            Em_PlaySeAtBoneVolOnFrame(em, 0x22, Sound_RandomSe(0x3E, 6, 0x3C, 2), 0xE, 0xA);
            Em_PlaySeAtBoneVolOnFrame(em, 0x5A, Sound_RandomSe(0x3D, 6, 0x3E, 2), 0xE, 0xA);
            Em_PlaySeAtBoneVolOnFrame(em, 0x96, Sound_RandomSe(0x3C, 6, 0x3D, 2), 0xE, 0xA);
            Em_PlaySeAtBoneVolOnFrame(em, 0xDE, Sound_RandomSe(0x3E, 6, 0x3C, 2), 0xE, 0xA);
            Em_PlaySeAtBoneVolOnFrame(em, 0x13A, Sound_RandomSe(0x3D, 6, 0x3E, 2), 0xE, 0xA);
            break;
        case 0x3F2:
            if (type == 0x43) {
                Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x2C, 0xE);
                Em_PlaySeAtBoneOnFrame(em, 0x6E, 0x24, 0xE);
                Em_PlaySeAtBoneOnFrame(em, 0x6E, 0xB, 2);
                Em_PlaySeAtBoneOnFrame(em, 0x70, 0x12, 2);
                Em_PlaySeAtBoneOnFrame(em, 0x20, 0xE, 2);
                Em_PlaySeAtBoneOnFrame(em, 0x4A, 0x10, 2);
                Em_PlaySeAtBoneOnFrame(em, 0x24, 0, 0x1A);
                Em_PlaySeAtBoneOnFrame(em, 0x52, 1, 0x1E);
                Em_PlaySeAtBoneOnFrame(em, 0x62, 0, 0x11);
                Em_PlaySeAtBoneOnFrame(em, 0x32, 1, 0x15);
            } else {
                Em_PlaySeAtBoneOnFrame(em, 0x30, 0x2C, 0xE);
                Em_PlaySeAtBoneOnFrame(em, 0x5E, 0x24, 0xE);
                Em_PlaySeAtBoneOnFrame(em, 0x5C, 0xB, 2);
                Em_PlaySeAtBoneOnFrame(em, 0x5A, 0x12, 2);
                Em_PlaySeAtBoneOnFrame(em, 0x20, 0xE, 2);
                Em_PlaySeAtBoneOnFrame(em, 0x3A, 0x10, 2);
                Em_PlaySeAtBoneOnFrame(em, 0xC, 0, 0x1A);
                Em_PlaySeAtBoneOnFrame(em, 0x30, 1, 0x1E);
                Em_PlaySeAtBoneOnFrame(em, 0x24, 0, 0x11);
                Em_PlaySeAtBoneOnFrame(em, 0x18, 1, 0x15);
            }
            break;
        case 0x3F3:
            if (type == 0x43) {
                Em_PlaySeAtBoneOnFrame(em, 0x28, Sound_RandomSe(0x3C, 4, 0x3D, 4), 0xE);
                Em_PlaySeAtBoneOnFrame(em, 0x6E, Sound_RandomSe(0x3C, 4, 0x3D, 4), 0xE);
                Em_PlaySeAtBoneOnFrame(em, 0xBA, Sound_RandomSe(0x3C, 4, 0x3D, 4), 0xE);
                Em_PlaySeAtBoneVolOnFrame(em, 8, 0x65, 2, 0x16);
                Em_PlaySeAtBoneOnFrame(em, 6, 0x13, 2);
                Em_PlaySeAtBoneOnFrame(em, 0x74, 0, 0x1A);
                Em_PlaySeAtBoneOnFrame(em, 0x9E, 0, 0x11);
                Em_PlaySeAtBoneOnFrame(em, 0x84, 1, 0x15);
                Em_PlaySeAtBoneOnFrame(em, 0xAE, 1, 0x1E);
                Em_PlaySeAtBoneOnFrame(em, 0xC2, 0x14, 0xA);
                Em_PlaySeAtBoneOnFrame(em, 0xC4, 0x1D, 6);
                Em_PlaySeAtBoneOnFrame(em, 0x3E, 0x10, 2);
                Em_PlaySeAtBoneOnFrame(em, 0x58, 0x11, 2);
                Em_PlaySeAtBoneOnFrame(em, 0x9E, 0xE, 2);
            } else {
                Em_PlaySeAtBoneOnFrame(em, 0x36, Sound_RandomSe(0x3C, 4, 0x3D, 4), 0xE);
                Em_PlaySeAtBoneOnFrame(em, 6, 0x13, 2);
                Em_PlaySeAtBoneOnFrame(em, 4, 0x12, 2);
                Em_PlaySeAtBoneOnFrame(em, 8, 0x1B, 2);
                Em_PlaySeAtBoneVolOnFrame(em, 0xA, 0x65, 2, 0x16);
                Em_PlaySeAtBoneOnFrame(em, 0x88, 0xE, 2);
                Em_PlaySeAtBoneOnFrame(em, 0xB6, 4, 0x11);
                Em_PlaySeAtBoneOnFrame(em, 0xA6, 5, 0x15);
                Em_PlaySeAtBoneOnFrame(em, 0x94, 2, 0x1A);
                Em_PlaySeAtBoneOnFrame(em, 0xC4, 5, 0x1E);
                Em_PlaySeAtBoneOnFrame(em, 0x62, 0x15, 0xA);
                Em_PlaySeAtBoneOnFrame(em, 0x82, 4, 6);
            }
            break;
        case 0x3F4:
            Em_PlaySeAtBoneVolOnFrame(em, 0x16, Sound_RandomSe(0x3C, 4, 0x3D, 4), 0xE, 0xA);
            Em_PlaySeAtBoneVolOnFrame(em, 0x40, Sound_RandomSe(0x3E, 4, 0x3C, 4), 0xE, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0xA, Sound_RandomSe(0x10, 4, 0x11, 4), 2);
            Em_PlaySeAtBoneOnFrame(em, 0x28, Sound_RandomSe(0x10, 4, 0x11, 4), 2);
            Em_PlaySeAtBoneOnFrame(em, 0x22, 1, 0x1E);
            Em_PlaySeAtBoneOnFrame(em, 0x42, 1, 0x15);
            Em_PlaySeAtBoneOnFrame(em, 0x3E, 0, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x20, 0, 0x11);
            break;
        case 0x3F5:
            Em_PlaySeAtBoneVolOnFrame(em, 8, Sound_RandomSe(0x3C, 4, 0x3D, 4), 0xE, 0xA);
            Em_PlaySeAtBoneVolOnFrame(em, 0x48, Sound_RandomSe(0x3E, 6, 0x3C, 2), 0xE, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0x1E, Sound_RandomSe(0x10, 4, 0x11, 4), 2);
            Em_PlaySeAtBoneOnFrame(em, 0x32, Sound_RandomSe(0x10, 4, 0x11, 4), 2);
            Em_PlaySeAtBoneOnFrame(em, 0x56, Sound_RandomSe(0x10, 4, 0x11, 4), 2);
            Em_PlaySeAtBoneOnFrame(em, 0x12, 0, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x2E, 0, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0x48, 0, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x60, 0, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0x1E, 1, 0x1E);
            Em_PlaySeAtBoneOnFrame(em, 0x34, 1, 0x15);
            Em_PlaySeAtBoneOnFrame(em, 0x54, 1, 0x1E);
            Em_PlaySeAtBoneOnFrame(em, 0x6A, 1, 0x15);
            break;
        case 0x3F6:
            Em_PlaySeAtBoneVolOnFrame(em, 8, Sound_RandomSe(0x3C, 4, 0x3D, 4), 0xE, 0xA);
            Em_PlaySeAtBoneVolOnFrame(em, 0x48, Sound_RandomSe(0x3E, 6, 0x3C, 2), 0xE, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0x1E, Sound_RandomSe(0x10, 4, 0x11, 4), 2);
            Em_PlaySeAtBoneOnFrame(em, 0x32, Sound_RandomSe(0x10, 4, 0x11, 4), 2);
            Em_PlaySeAtBoneOnFrame(em, 0x56, Sound_RandomSe(0x10, 4, 0x11, 4), 2);
            Em_PlaySeAtBoneOnFrame(em, 0x12, 0, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0x2E, 0, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x48, 0, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0x60, 0, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x1E, 1, 0x15);
            Em_PlaySeAtBoneOnFrame(em, 0x34, 1, 0x1E);
            Em_PlaySeAtBoneOnFrame(em, 0x54, 1, 0x15);
            Em_PlaySeAtBoneOnFrame(em, 0x6A, 1, 0x1E);
            break;
        case 0x3F7:
            Em_PlaySeAtBoneVolOnFrame(em, 4, Sound_RandomSe(0x3C, 3, 0x3D, 3), 0xE, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0x34, 0, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0xA, 1, 0x15);
            Em_PlaySeAtBoneOnFrame(em, 2, 0, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x34, 1, 0x1E);
            break;
        case 0x3F8:
            Em_PlaySeAtBoneVolOnFrame(em, 4, Sound_RandomSe(0x3C, 3, 0x3D, 3), 0xE, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0x34, 0, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0xA, 1, 0x1E);
            Em_PlaySeAtBoneOnFrame(em, 2, 0, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0x34, 1, 0x15);
            break;
        case 0x3FA:
            Em_PlaySeAtBoneVolOnFrame(em, 4, 0x1E, 0xE, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 4, 0x11, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x16, 9, 0xA);
            break;
        case 0x3FB:
            if (type == 0x43) {
                Em_PlaySeAtBoneVolOnFrame(em, 4, 0x1F, 0xE, 0xA);
                Em_PlaySeAtBoneOnFrame(em, 0xC, 0xC, 0xA);
                Em_PlaySeAtBoneOnFrame(em, 0x14, 0x1D, 0xA);
                Em_PlaySeAtBoneOnFrame(em, 0x18, 0x16, 0xA);
                Em_PlaySeAtBoneOnFrame(em, 0x3A, 0x10, 2);
                Em_PlaySeAtBoneOnFrame(em, 0x6E, 0x11, 0xA);
            } else {
                Em_PlaySeAtBoneVolOnFrame(em, 4, 0x1F, 0xE, 0xA);
                Em_PlaySeAtBoneOnFrame(em, 0xC, 0xC, 0xA);
                Em_PlaySeAtBoneOnFrame(em, 0x14, 0x1D, 0xA);
                Em_PlaySeAtBoneOnFrame(em, 0x3A, 0x10, 2);
                Em_PlaySeAtBoneOnFrame(em, 0x58, 9, 6);
                Em_PlaySeAtBoneOnFrame(em, 0x6E, 0x11, 0xA);
            }
            break;
        case 0x3FC:
            Em_PlaySeAtBoneOnFrame(em, 0x16, 2, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 6, 3, 0x15);
            Em_PlaySeAtBoneOnFrame(em, 0x18, 3, 0x1E);
            Em_PlaySeAtBoneOnFrame(em, 2, 2, 0x1A);
            break;
        case 0x3FD:
            if (type == 0x43) {
                Em_PlaySeAtBoneOnFrame(em, 4, 0x20, 0xE);
                Em_PlaySeAtBoneOnFrame(em, 0x58, 0x10, 2);
                Em_PlaySeAtBoneOnFrame(em, 0xC, 0xC, 0xA);
                Em_PlaySeAtBoneOnFrame(em, 0x16, 0x1D, 0xA);
                Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x16, 0xA);
                Em_PlaySeAtBoneOnFrame(em, 0x28, 2, 0x1A);
                Em_PlaySeAtBoneOnFrame(em, 0x54, 0, 0x1A);
                Em_PlaySeAtBoneOnFrame(em, 0x16, 2, 0x11);
                Em_PlaySeAtBoneOnFrame(em, 0x3C, 0, 0x11);
                Em_PlaySeAtBoneOnFrame(em, 0x2E, 1, 0x15);
                Em_PlaySeAtBoneOnFrame(em, 0x66, 5, 0x15);
                Em_PlaySeAtBoneOnFrame(em, 0x16, 3, 0x1E);
                Em_PlaySeAtBoneOnFrame(em, 0x44, 1, 0x1E);
            } else {
                Em_PlaySeAtBoneOnFrame(em, 4, 0x20, 0xE);
                Em_PlaySeAtBoneOnFrame(em, 0x58, 0x10, 2);
                Em_PlaySeAtBoneOnFrame(em, 0xC, 0xC, 0xA);
                Em_PlaySeAtBoneOnFrame(em, 0x16, 0x1D, 0xA);
                Em_PlaySeAtBoneOnFrame(em, 0x1A, 9, 6);
                Em_PlaySeAtBoneOnFrame(em, 0x58, 8, 6);
                Em_PlaySeAtBoneOnFrame(em, 0x74, 9, 0xA);
                Em_PlaySeAtBoneOnFrame(em, 0x28, 2, 0x1A);
                Em_PlaySeAtBoneOnFrame(em, 0x54, 0, 0x1A);
                Em_PlaySeAtBoneOnFrame(em, 0x16, 2, 0x11);
                Em_PlaySeAtBoneOnFrame(em, 0x3C, 0, 0x11);
                Em_PlaySeAtBoneOnFrame(em, 0x2E, 1, 0x15);
                Em_PlaySeAtBoneOnFrame(em, 0x66, 5, 0x15);
                Em_PlaySeAtBoneOnFrame(em, 0x16, 3, 0x1E);
                Em_PlaySeAtBoneOnFrame(em, 0x44, 1, 0x1E);
            }
            break;
        case 0x3FE:
            Em_PlaySeAtBoneOnFrame(em, 0x16, 2, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 6, 3, 0x1E);
            Em_PlaySeAtBoneOnFrame(em, 0x18, 3, 0x15);
            Em_PlaySeAtBoneOnFrame(em, 2, 2, 0x11);
            break;
        case 0x3FF:
            if (type == 0x43) {
                Em_PlaySeAtBoneOnFrame(em, 4, 0x20, 0xE);
                Em_PlaySeAtBoneOnFrame(em, 0x58, 0x10, 2);
                Em_PlaySeAtBoneOnFrame(em, 0xC, 0xC, 6);
                Em_PlaySeAtBoneOnFrame(em, 0x16, 0x1D, 6);
                Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x16, 6);
                Em_PlaySeAtBoneOnFrame(em, 0x28, 2, 0x11);
                Em_PlaySeAtBoneOnFrame(em, 0x54, 0, 0x11);
                Em_PlaySeAtBoneOnFrame(em, 0x16, 2, 0x1A);
                Em_PlaySeAtBoneOnFrame(em, 0x3C, 0, 0x1A);
                Em_PlaySeAtBoneOnFrame(em, 0x2E, 1, 0x1E);
                Em_PlaySeAtBoneOnFrame(em, 0x66, 5, 0x1E);
                Em_PlaySeAtBoneOnFrame(em, 0x16, 3, 0x15);
                Em_PlaySeAtBoneOnFrame(em, 0x44, 1, 0x15);
            } else {
                Em_PlaySeAtBoneOnFrame(em, 4, 0x20, 0xE);
                Em_PlaySeAtBoneOnFrame(em, 0x58, 0x10, 2);
                Em_PlaySeAtBoneOnFrame(em, 0xC, 0xC, 6);
                Em_PlaySeAtBoneOnFrame(em, 0x16, 0x1D, 6);
                Em_PlaySeAtBoneOnFrame(em, 0x1A, 9, 0xA);
                Em_PlaySeAtBoneOnFrame(em, 0x58, 8, 0xA);
                Em_PlaySeAtBoneOnFrame(em, 0x74, 9, 6);
                Em_PlaySeAtBoneOnFrame(em, 0x28, 2, 0x11);
                Em_PlaySeAtBoneOnFrame(em, 0x54, 0, 0x11);
                Em_PlaySeAtBoneOnFrame(em, 0x16, 2, 0x1A);
                Em_PlaySeAtBoneOnFrame(em, 0x3C, 0, 0x1A);
                Em_PlaySeAtBoneOnFrame(em, 0x2E, 1, 0x1E);
                Em_PlaySeAtBoneOnFrame(em, 0x66, 5, 0x1E);
                Em_PlaySeAtBoneOnFrame(em, 0x16, 3, 0x15);
                Em_PlaySeAtBoneOnFrame(em, 0x44, 1, 0x15);
            }
            break;
        case 0x400:
            Em_PlaySeAtBoneOnFrame(em, 4, 0x1E, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 4, 0x11, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0x16, 9, 6);
            break;
        case 0x401:
            if (type == 0x43) {
                Em_PlaySeAtBoneVolOnFrame(em, 4, 0x1F, 0xE, 0xA);
                Em_PlaySeAtBoneOnFrame(em, 0xC, 0xC, 6);
                Em_PlaySeAtBoneOnFrame(em, 0x14, 0x1D, 6);
                Em_PlaySeAtBoneOnFrame(em, 0x18, 0x16, 6);
                Em_PlaySeAtBoneOnFrame(em, 0x3A, 0x10, 2);
                Em_PlaySeAtBoneOnFrame(em, 0x6E, 0x11, 6);
            } else {
                Em_PlaySeAtBoneVolOnFrame(em, 4, 0x1F, 0xE, 0xA);
                Em_PlaySeAtBoneOnFrame(em, 0xC, 0xC, 6);
                Em_PlaySeAtBoneOnFrame(em, 0x14, 0x1D, 6);
                Em_PlaySeAtBoneOnFrame(em, 0x3A, 0x10, 2);
                Em_PlaySeAtBoneOnFrame(em, 0x58, 9, 0xA);
                Em_PlaySeAtBoneOnFrame(em, 0x6E, 0x11, 6);
            }
            break;
        case 0x407:
            Em_PlaySeAtBoneOnFrame(em, 4, 0x21, 0xE);
            Em_PlaySeAtBoneVolOnFrame(em, 0x8C, Sound_RandomSe(0x43, 6, 0x42, 2), 0xE, 0xA);
            Em_PlaySeAtBoneVolOnFrame(em, 0xBC, Sound_RandomSe(0x42, 6, 0x43, 2), 0xE, 0xA);
            break;
        case 0x408:
            Em_PlaySeAtBoneOnFrame(em, 0xC, 0x22, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x3A, 0xE, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x66, 0xD, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x7E, 8, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0x80, 9, 6);
            break;
        case 0x409:
            Em_PlaySeAtBoneVolOnFrame(em, 0xA, Sound_RandomSe(0x42, 6, 0x43, 2), 0xE, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0x4E, 0x21, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x10, 0xE, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x62, 0xE, 2);
            break;
        case 0x40A:
            if (type == 0x43) {
                Em_PlaySeAtBoneOnFrame(em, 0x5E, 0x2C, 0xE);
                Em_PlaySeAtBoneOnFrame(em, 4, 0x1B, 2);
                Em_PlaySeAtBoneOnFrame(em, 2, 0x1A, 2);
                Em_PlaySeAtBoneVolOnFrame(em, 4, 0x65, 2, 0x16);
                Em_PlaySeAtBoneOnFrame(em, 0x10, 0x16, 0xA);
                Em_PlaySeAtBoneOnFrame(em, 0x1C, 0x16, 6);
                Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x1D, 0xA);
                Em_PlaySeAtBoneOnFrame(em, 0x22, 0x1D, 6);
                Em_PlaySeAtBoneOnFrame(em, 0x5C, 6, 0x1A);
                Em_PlaySeAtBoneOnFrame(em, 0x64, 4, 0x11);
                Em_PlaySeAtBoneOnFrame(em, 0x88, 3, 0x15);
                Em_PlaySeAtBoneOnFrame(em, 0x90, 1, 0x1E);
                Em_PlaySeAtBoneOnFrame(em, 0x86, 0, 0x1A);
                Em_PlaySeAtBoneOnFrame(em, 0x9C, 0, 0x11);
                Em_PlaySeAtBoneOnFrame(em, 0xB4, 1, 0x15);
                Em_PlaySeAtBoneOnFrame(em, 0x2A, 0xF, 2);
                Em_PlaySeAtBoneOnFrame(em, 0x42, 0xF, 2);
                Em_PlaySeAtBoneOnFrame(em, 0x58, 0xF, 2);
                Em_PlaySeAtBoneOnFrame(em, 0x9E, 0xE, 2);
                Em_PlaySeAtBoneOnFrame(em, 0x52, 0x19, 2);
                Em_PlaySeAtBoneOnFrame(em, 0x7A, 0x1A, 2);
                Em_PlaySeAtBoneOnFrame(em, 0x76, 0x16, 0xA);
                Em_PlaySeAtBoneOnFrame(em, 0x82, 0x16, 6);
                Em_PlaySeAtBoneOnFrame(em, 0x7A, 0x1D, 0xA);
                Em_PlaySeAtBoneOnFrame(em, 0x8A, 0x1D, 6);
            } else {
                Em_PlaySeAtBoneOnFrame(em, 0x7E, 0x2C, 0xE);
                Em_PlaySeAtBoneOnFrame(em, 0xA, 0x1A, 2);
                Em_PlaySeAtBoneVolOnFrame(em, 2, 0x65, 2, 0x16);
                Em_PlaySeAtBoneOnFrame(em, 8, 0xF, 2);
                Em_PlaySeAtBoneOnFrame(em, 0x2A, 0xF, 2);
                Em_PlaySeAtBoneOnFrame(em, 0x42, 0xF, 2);
                Em_PlaySeAtBoneOnFrame(em, 0x58, 0xF, 2);
                Em_PlaySeAtBoneOnFrame(em, 0x9E, 0xE, 2);
                Em_PlaySeAtBoneOnFrame(em, 0x52, 0x19, 0xA);
                Em_PlaySeAtBoneOnFrame(em, 0x7A, 0x19, 0xA);
                Em_PlaySeAtBoneOnFrame(em, 0x62, 7, 0x1E);
                Em_PlaySeAtBoneOnFrame(em, 0x76, 6, 0x1A);
                Em_PlaySeAtBoneOnFrame(em, 0xA8, 1, 0x1E);
                Em_PlaySeAtBoneOnFrame(em, 0xC6, 0, 0x1A);
            }
            break;
        case 0x40C:
            Em_PlaySeAtBoneOnFrame(em, 6, 0x23, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 6, 0xE, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x2E, 0x10, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x48, 0x10, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x5A, 0x10, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x68, 0x11, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x78, 0x10, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x88, 0x10, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x9A, 0x11, 2);
            Em_PlaySeAtBoneOnFrame(em, 0xA8, 0xF, 2);
            break;
        case 0x40D:
            Em_PlaySeAtBoneVolOnFrame(em, 4, Sound_RandomSe(0x3F, 4, 0x40, 4), 0xE, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 2, 2, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x12, 2, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0x24, 3, 0x15);
            Em_PlaySeAtBoneOnFrame(em, 0x10, 3, 0x1E);
            break;
        case 0x40E:
            Em_PlaySeAtBoneVolOnFrame(em, 4, Sound_RandomSe(0x3C, 6, 0x3D, 2), 0xE, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0x24, 0, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0x2A, 0x11, 2);
            break;
        case 0x40F:
            Em_PlaySeAtBoneOnFrame(em, 4, 0x23, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x4E, 0x24, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x50, 0x18, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 4, 0xF, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x2A, 0x10, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x12, 9, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0x18, 0xD, 6);
            Em_PlaySeAtBoneOnFrame(em, 0x12, 1, 0x1E);
            Em_PlaySeAtBoneOnFrame(em, 0x24, 0, 0x11);
            break;
        case 0x410:
            if (type == 0x43) {
                Em_PlaySeAtBoneOnFrame(em, 8, 0x35, 0xE);
                Em_PlaySeAtBoneOnFrame(em, 0x3C, 0x2B, 0xE);
                Em_PlaySeAtBoneVolOnFrame(em, 0x96, Sound_RandomSe(0x3C, 4, 0x3D, 4), 0xE, 0xA);
                Em_PlaySeAtBoneOnFrame(em, 4, 0x10, 2);
                Em_PlaySeAtBoneOnFrame(em, 0x7E, 0xE, 2);
                Em_PlaySeAtBoneOnFrame(em, 0x3E, 0xC, 0xA);
                Em_PlaySeAtBoneOnFrame(em, 0x42, 0x16, 0xA);
                Em_PlaySeAtBoneOnFrame(em, 0x46, 0x12, 0xA);
                Em_PlaySeAtBoneOnFrame(em, 0xC, 2, 0x1A);
                Em_PlaySeAtBoneOnFrame(em, 0x2E, 2, 0x11);
                Em_PlaySeAtBoneOnFrame(em, 0x20, 3, 0x1E);
                Em_PlaySeAtBoneOnFrame(em, 0x18, 3, 0x15);
            } else {
                Em_PlaySeAtBoneOnFrame(em, 4, 0x36, 0xE);
                Em_PlaySeAtBoneOnFrame(em, 0x38, 0xE, 2);
                Em_PlaySeAtBoneOnFrame(em, 0x32, 0x1D, 0xA);
                Em_PlaySeAtBoneOnFrame(em, 0x30, 9, 0xA);
                Em_PlaySeAtBoneOnFrame(em, 0x30, 8, 6);
                Em_PlaySeAtBoneOnFrame(em, 0xC, 2, 0x1A);
                Em_PlaySeAtBoneOnFrame(em, 0x32, 6, 0x1A);
                Em_PlaySeAtBoneOnFrame(em, 0x42, 3, 0x1E);
                Em_PlaySeAtBoneOnFrame(em, 0x24, 3, 0x15);
                Em_PlaySeAtBoneOnFrame(em, 0x38, 6, 0x11);
            }
            break;
        case 0x411:
            if (type == 0x43) {
                Em_PlaySeAtBoneOnFrame(em, 8, 0x35, 0xE);
                Em_PlaySeAtBoneOnFrame(em, 0x3C, 0x2B, 0xE);
                Em_PlaySeAtBoneVolOnFrame(em, 0x96, Sound_RandomSe(0x3C, 4, 0x3D, 4), 0xE, 0xA);
                Em_PlaySeAtBoneOnFrame(em, 4, 0x10, 2);
                Em_PlaySeAtBoneOnFrame(em, 0x7E, 0xE, 2);
                Em_PlaySeAtBoneOnFrame(em, 0x3E, 0xC, 6);
                Em_PlaySeAtBoneOnFrame(em, 0x42, 0x16, 6);
                Em_PlaySeAtBoneOnFrame(em, 0x46, 0x12, 6);
                Em_PlaySeAtBoneOnFrame(em, 0xC, 2, 0x11);
                Em_PlaySeAtBoneOnFrame(em, 0x2E, 2, 0x1A);
                Em_PlaySeAtBoneOnFrame(em, 0x20, 3, 0x15);
                Em_PlaySeAtBoneOnFrame(em, 0x18, 3, 0x1E);
            } else {
                Em_PlaySeAtBoneOnFrame(em, 8, 0x35, 0xE);
                Em_PlaySeAtBoneOnFrame(em, 0x8A, 0x2A, 0xE);
                Em_PlaySeAtBoneOnFrame(em, 4, 0x15, 0xA);
                Em_PlaySeAtBoneOnFrame(em, 0x50, 0xE, 2);
                Em_PlaySeAtBoneOnFrame(em, 0x76, 2, 0x1A);
                Em_PlaySeAtBoneOnFrame(em, 0x8C, 3, 0x1E);
                Em_PlaySeAtBoneOnFrame(em, 0x82, 2, 0x15);
                Em_PlaySeAtBoneOnFrame(em, 0x96, 0, 0x11);
            }
            break;
        case 0x412:
            if (type == 0x43) {
                Em_PlaySeAtBoneOnFrame(em, 2, 0x2B, 0xE);
                Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x1F, 0xE);
                Em_PlaySeAtBoneOnFrame(em, 4, 0xE, 2);
                Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x16, 0xA);
                Em_PlaySeAtBoneOnFrame(em, 0x34, 0x16, 6);
                Em_PlaySeAtBoneOnFrame(em, 0x32, 0x1D, 0xA);
                Em_PlaySeAtBoneOnFrame(em, 0x38, 0x1D, 6);
            } else {
                Em_PlaySeAtBoneOnFrame(em, 2, 0x2B, 0xE);
                Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x1F, 0xE);
                Em_PlaySeAtBoneOnFrame(em, 4, 0xE, 2);
                Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x16, 0xA);
            }
            break;
        case 0x413:
            if (type == 0x43) {
                Em_PlaySeAtBoneOnFrame(em, 0x14, 0x20, 0xE);
                Em_PlaySeAtBoneOnFrame(em, 0xC, 0x16, 0xA);
                Em_PlaySeAtBoneOnFrame(em, 0x16, 0x16, 6);
                Em_PlaySeAtBoneOnFrame(em, 0x14, 0xD, 0xA);
                Em_PlaySeAtBoneOnFrame(em, 0x1E, 0xD, 6);
                Em_PlaySeAtBoneOnFrame(em, 0x1C, 0x14, 2);
                Em_PlaySeAtBoneOnFrame(em, 0x3C, 0xE, 2);
                Em_PlaySeAtBoneOnFrame(em, 0x3C, 0x10, 2);
                Em_PlaySeAtBoneOnFrame(em, 0x2B, 0, 0x11);
                Em_PlaySeAtBoneOnFrame(em, 0x64, 1, 0x1E);
                Em_PlaySeAtBoneOnFrame(em, 0x82, 0, 0x1A);
            } else {
                Em_PlaySeAtBoneOnFrame(em, 0x14, 0x20, 0xE);
                Em_PlaySeAtBoneOnFrame(em, 0xC, 0x16, 0xA);
                Em_PlaySeAtBoneOnFrame(em, 0x16, 0x16, 6);
                Em_PlaySeAtBoneOnFrame(em, 0x14, 0xD, 0xA);
                Em_PlaySeAtBoneOnFrame(em, 0x1E, 0xD, 6);
                Em_PlaySeAtBoneOnFrame(em, 0x1C, 0x14, 2);
                Em_PlaySeAtBoneOnFrame(em, 0x3C, 0xE, 2);
                Em_PlaySeAtBoneOnFrame(em, 0x3C, 0x10, 2);
                Em_PlaySeAtBoneOnFrame(em, 0x5E, 0, 0x11);
                Em_PlaySeAtBoneOnFrame(em, 0x6C, 1, 0x1E);
                Em_PlaySeAtBoneOnFrame(em, 0x7E, 0, 0x1A);
                Em_PlaySeAtBoneOnFrame(em, 0x86, 1, 0x15);
            }
            break;
        case 0x414:
            if (type == 0x43) {
                Em_PlaySeAtBoneOnFrame(em, 0x29, 0x1F, 0xE);
                Em_PlaySeAtBoneOnFrame(em, 0x2C, 0xE, 2);
                Em_PlaySeAtBoneOnFrame(em, 0x26, 5, 0x15);
            } else {
                Em_PlaySeAtBoneOnFrame(em, 0x29, 0x1F, 0xE);
                Em_PlaySeAtBoneOnFrame(em, 0x2C, 0xE, 2);
                Em_PlaySeAtBoneOnFrame(em, 0xC, 2, 0x11);
                Em_PlaySeAtBoneOnFrame(em, 0xC, 1, 0x1E);
                Em_PlaySeAtBoneOnFrame(em, 0x2E, 0, 0x1A);
                Em_PlaySeAtBoneOnFrame(em, 0x20, 1, 0x15);
            }
            break;
        case 0x415:
            if (type == 0x43) {
                Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x3C, 4, 0x3D, 4), 0xE);
                Em_PlaySeAtBoneOnFrame(em, 0x12, 2, 0x11);
                Em_PlaySeAtBoneOnFrame(em, 0x24, 3, 0x15);
                Em_PlaySeAtBoneOnFrame(em, 0xE, 3, 0x1E);
                Em_PlaySeAtBoneOnFrame(em, 2, 2, 0x1A);
            } else {
                Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x3C, 4, 0x3D, 4), 0xE);
                Em_PlaySeAtBoneOnFrame(em, 0x28, Sound_RandomSe(0x3D, 4, 0x3E, 4), 0xE);
                Em_PlaySeAtBoneOnFrame(em, 0x1C, 2, 0x11);
                Em_PlaySeAtBoneOnFrame(em, 0x3A, 3, 0x15);
                Em_PlaySeAtBoneOnFrame(em, 0x18, 3, 0x1E);
                Em_PlaySeAtBoneOnFrame(em, 0x40, 2, 0x1A);
            }
            break;
        case 0x416:
            if (type == 0x43) {
                Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x3F, 4, 0x40, 4), 0xE);
                Em_PlaySeAtBoneOnFrame(em, 0x1E, Sound_RandomSe(0x40, 4, 0x3D, 4), 0xE);
                Em_PlaySeAtBoneOnFrame(em, 2, 2, 0x11);
                Em_PlaySeAtBoneOnFrame(em, 0x2A, 3, 0x15);
                Em_PlaySeAtBoneOnFrame(em, 4, 3, 0x1E);
                Em_PlaySeAtBoneOnFrame(em, 0x24, 2, 0x1A);
            } else {
                Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x3F, 4, 0x40, 4), 0xE);
                Em_PlaySeAtBoneOnFrame(em, 0x28, Sound_RandomSe(0x40, 4, 0x3D, 4), 0xE);
                Em_PlaySeAtBoneOnFrame(em, 2, 2, 0x11);
                Em_PlaySeAtBoneOnFrame(em, 0x18, 3, 0x15);
                Em_PlaySeAtBoneOnFrame(em, 4, 3, 0x1E);
                Em_PlaySeAtBoneOnFrame(em, 0x16, 2, 0x1A);
            }
            break;
        case 0x417:
            if (type == 0x43) {
                Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x2C, 0xE);
                Em_PlaySeAtBoneOnFrame(em, 0x4A, 0x1F, 0xE);
                Em_PlaySeAtBoneOnFrame(em, 0x4E, 0xC, 6);
                Em_PlaySeAtBoneOnFrame(em, 0x54, 0x16, 6);
                Em_PlaySeAtBoneOnFrame(em, 0x52, 0x1D, 6);
                Em_PlaySeAtBoneOnFrame(em, 0x30, 2, 0x11);
                Em_PlaySeAtBoneOnFrame(em, 0x3E, 3, 0x15);
                Em_PlaySeAtBoneOnFrame(em, 0x4E, 2, 0x11);
                Em_PlaySeAtBoneOnFrame(em, 0x3E, 3, 0x15);
                Em_PlaySeAtBoneOnFrame(em, 0x34, 2, 0x1E);
                Em_PlaySeAtBoneOnFrame(em, 0x3C, 3, 0x1A);
                Em_PlaySeAtBoneOnFrame(em, 0x4C, 2, 0x1E);
                Em_PlaySeAtBoneOnFrame(em, 0x54, 3, 0x1A);
            } else {
                Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x2C, 0xE);
                Em_PlaySeAtBoneOnFrame(em, 0x4A, 0x1F, 0xE);
                Em_PlaySeAtBoneOnFrame(em, 0x50, 0x16, 6);
                Em_PlaySeAtBoneOnFrame(em, 0x6A, 0xD, 6);
                Em_PlaySeAtBoneOnFrame(em, 0x30, 2, 0x11);
                Em_PlaySeAtBoneOnFrame(em, 0x3E, 3, 0x15);
                Em_PlaySeAtBoneOnFrame(em, 0x4E, 2, 0x11);
                Em_PlaySeAtBoneOnFrame(em, 0x3E, 3, 0x15);
                Em_PlaySeAtBoneOnFrame(em, 0x34, 2, 0x1E);
                Em_PlaySeAtBoneOnFrame(em, 0x3C, 3, 0x1A);
                Em_PlaySeAtBoneOnFrame(em, 0x4C, 2, 0x1E);
                Em_PlaySeAtBoneOnFrame(em, 0x54, 3, 0x1A);
            }
            break;
        case 0x418:
            if (type == 0x43) {
                Em_PlaySeAtBoneOnFrame(em, 6, 0x1A, 2);
                Em_PlaySeAtBoneOnFrame(em, 6, 0x1B, 2);
                Em_PlaySeAtBoneVolOnFrame(em, 8, 0x65, 2, 0x16);
                Em_PlaySeAtBoneOnFrame(em, 0xE, 0x12, 0xE);
                Em_PlaySeAtBoneOnFrame(em, 0x30, 0x19, 2);
                Em_PlaySeAtBoneOnFrame(em, 0x3C, 0x1A, 2);
                Em_PlaySeAtBoneOnFrame(em, 0x12, 0x16, 6);
                Em_PlaySeAtBoneOnFrame(em, 0x20, 0x16, 0xA);
            } else {
                Em_PlaySeAtBoneOnFrame(em, 6, 0x1A, 2);
                Em_PlaySeAtBoneOnFrame(em, 6, 0x1B, 2);
                Em_PlaySeAtBoneVolOnFrame(em, 8, 0x65, 2, 0x16);
                Em_PlaySeAtBoneOnFrame(em, 0xE, 0x12, 0xE);
                Em_PlaySeAtBoneOnFrame(em, 0x30, 0x19, 2);
                Em_PlaySeAtBoneOnFrame(em, 0x3C, 0x1A, 2);
            }
            break;
        case 0x419:
            if (type == 0x43) {
                Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x2C, 0xE);
                Em_PlaySeAtBoneOnFrame(em, 0x4A, 0x1F, 0xE);
                Em_PlaySeAtBoneOnFrame(em, 0x4E, 0xC, 0xA);
                Em_PlaySeAtBoneOnFrame(em, 0x54, 0x16, 0xA);
                Em_PlaySeAtBoneOnFrame(em, 0x52, 0x1D, 0xA);
                Em_PlaySeAtBoneOnFrame(em, 0x30, 2, 0x1A);
                Em_PlaySeAtBoneOnFrame(em, 0x3E, 3, 0x1E);
                Em_PlaySeAtBoneOnFrame(em, 0x4E, 2, 0x1A);
                Em_PlaySeAtBoneOnFrame(em, 0x3E, 3, 0x1E);
                Em_PlaySeAtBoneOnFrame(em, 0x34, 2, 0x15);
                Em_PlaySeAtBoneOnFrame(em, 0x3C, 3, 0x11);
                Em_PlaySeAtBoneOnFrame(em, 0x4C, 2, 0x15);
                Em_PlaySeAtBoneOnFrame(em, 0x54, 3, 0x11);
            } else {
                Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x2C, 0xE);
                Em_PlaySeAtBoneOnFrame(em, 0x4A, 0x1F, 0xE);
                Em_PlaySeAtBoneOnFrame(em, 0x50, 0x16, 0xA);
                Em_PlaySeAtBoneOnFrame(em, 0x6A, 0xD, 0xA);
                Em_PlaySeAtBoneOnFrame(em, 0x30, 2, 0x1A);
                Em_PlaySeAtBoneOnFrame(em, 0x3E, 3, 0x1E);
                Em_PlaySeAtBoneOnFrame(em, 0x4E, 2, 0x1A);
                Em_PlaySeAtBoneOnFrame(em, 0x3E, 3, 0x1E);
                Em_PlaySeAtBoneOnFrame(em, 0x34, 2, 0x15);
                Em_PlaySeAtBoneOnFrame(em, 0x3C, 3, 0x11);
                Em_PlaySeAtBoneOnFrame(em, 0x4C, 2, 0x15);
                Em_PlaySeAtBoneOnFrame(em, 0x54, 3, 0x11);
            }
            break;
        case 0x41A:
            Em_PlaySeAtBoneVolOnFrame(em, 0x46, 0x1E, 0xE, 0xA);
            Em_PlaySeAtBoneVolOnFrame(em, 0xAC, 0x22, 0xE, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x19, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x22, 0x19, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x38, 0x1A, 2);
            Em_PlaySeAtBoneOnFrame(em, 0xA8, 2, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0xAC, 2, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x4C, 0xE, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x90, 0x10, 2);
            Em_PlaySeAtBoneOnFrame(em, 0xAE, 0x14, 2);
            Em_PlaySeAtBoneOnFrame(em, 0xB0, 0xE, 2);
            Em_PlaySeAtBoneOnFrame(em, 0xB2, 0xF, 2);
            Em_PlaySeAtBoneOnFrame(em, 0xB4, 0x10, 2);
            Em_PlaySeAtBoneOnFrame(em, 0xB6, 0x11, 2);
            break;
        case 0x41B:
            Em_PlaySeAtBoneOnFrame(em, 0x4A, 0x37, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x72, 0x38, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0xA6, 0x38, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x4C, 0x10, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x70, 0x10, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x1E, 0xD, 6);
            Em_PlaySeAtBoneOnFrame(em, 0x26, 0xD, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0x56, 0xD, 6);
            Em_PlaySeAtBoneOnFrame(em, 0x5E, 0xD, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0x80, 0xD, 6);
            Em_PlaySeAtBoneOnFrame(em, 0x86, 0xD, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0xBE, 0xD, 6);
            Em_PlaySeAtBoneOnFrame(em, 0xB8, 0xD, 0xA);
            break;
        case 0x41C:
            Em_PlaySeAtBoneOnFrame(em, 4, 0x2A, 0xE);
            Em_PlaySeAtBoneVolOnFrame(em, 0x98, Sound_RandomSe(0x3C, 6, 0x3D, 2), 0xE, 0xA);
            Em_PlaySeAtBoneVolOnFrame(em, 0xDE, Sound_RandomSe(0x3D, 2, 0x3E, 6), 0xE, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0x1A, 0xF, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x30, 0xE, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x1A, 9, 6);
            Em_PlaySeAtBoneOnFrame(em, 0x1C, 0xD, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0x6A, 2, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0x6C, 8, 6);
            Em_PlaySeAtBoneOnFrame(em, 0x6E, 9, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0xA6, 0, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0xD4, 0, 0x1A);
            break;
        case 0x41D:
        case 0x41E:
            Em_PlaySeAtBoneOnFrame(em, 0x28, 0xF, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x58, 0x36, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x10C, 0x37, 0xE);
            Em_PlaySeAtBoneVolOnFrame(em, 0x98, Sound_RandomSe(0x3C, 6, 0x3D, 2), 0xE, 0xA);
            Em_PlaySeAtBoneVolOnFrame(em, 0xDE, Sound_RandomSe(0x3D, 2, 0x3E, 6), 0xE, 0xA);
            Em_PlaySeAtBoneVolOnFrame(em, 0x11A, Sound_RandomSe(0x3C, 2, 0x3D, 6), 0xE, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0x5C, 0x11, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x10E, 0xE, 2);
            break;
        case 0x41F:
            Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x38, 0xE);
            Em_PlaySeAtBoneVolOnFrame(em, 0x40, Sound_RandomSe(0x40, 4, 0x3D, 4), 0xE, 0xA);
            Em_PlaySeAtBoneVolOnFrame(em, 0x72, Sound_RandomSe(0x3C, 4, 0x3D, 4), 0xE, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0x194, 0x17, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x1D6, 0x17, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x206, 0x17, 0xE);
            Em_PlaySeAtBoneVolOnFrame(em, 0x1F4, Sound_RandomSe(0x3F, 4, 0x40, 4), 0xE, 0xA);
            Em_PlaySeAtBoneVolOnFrame(em, 0x21C, Sound_RandomSe(0x40, 4, 0x3D, 4), 0xE, 0xA);
            Em_PlaySeAtBoneVolOnFrame(em, 0x244, Sound_RandomSe(0x3F, 4, 0x40, 4), 0xE, 0xA);
            Em_PlaySeAtBoneVolOnFrame(em, 0x26C, Sound_RandomSe(0x40, 4, 0x3D, 4), 0xE, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0x11E, 0x1D, 6);
            Em_PlaySeAtBoneOnFrame(em, 0xE4, 0xD, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0xFC, 0x1D, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0x174, 0x15, 6);
            Em_PlaySeAtBoneOnFrame(em, 0x176, 0x11, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0x1C0, 0x11, 6);
            Em_PlaySeAtBoneOnFrame(em, 0x1E0, 0x11, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0xA8, 2, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0x10A, 1, 0x15);
            Em_PlaySeAtBoneOnFrame(em, 0x296, 1, 0x15);
            Em_PlaySeAtBoneOnFrame(em, 0x2C0, 2, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0xC0, 1, 0x1E);
            Em_PlaySeAtBoneOnFrame(em, 0xDC, 0, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x2A8, 0, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x2CE, 3, 0x1E);
            Em_PlaySeAtBoneOnFrame(em, 0x2B2, 0x10, 2);
            break;
        case 0x421:
            Em_PlaySeAtBoneOnFrame(em, 4, 0x1E, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x20, 0x19, 6);
            Em_PlaySeAtBoneOnFrame(em, 0x24, 0x19, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0x36, 0x19, 6);
            Em_PlaySeAtBoneOnFrame(em, 0x42, 0x19, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0x50, 0x19, 6);
            Em_PlaySeAtBoneOnFrame(em, 0x5A, 0x19, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0x68, 0x19, 6);
            Em_PlaySeAtBoneOnFrame(em, 0x82, 0x1A, 2);
            Em_PlaySeAtBoneOnFrame(em, 0xA0, 0x1A, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x18, 6, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x30, 7, 0x1E);
            Em_PlaySeAtBoneOnFrame(em, 0x26, 1, 0x15);
            Em_PlaySeAtBoneOnFrame(em, 0x38, 2, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0x1A, 0xE, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x30, 0x10, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x46, 0xE, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x5A, 0xE, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x6E, 0xE, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x82, 0xE, 2);
            break;
        case 0x422:
            Em_PlaySeAtBoneVolOnFrame(em, 0xE, 0x35, 0xE, 0xA);
            Em_PlaySeAtBoneVolOnFrame(em, 0x40, Sound_RandomSe(0x3C, 4, 0x3D, 4), 0xE, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 4, 5, 0x15);
            Em_PlaySeAtBoneOnFrame(em, 6, 7, 0x1E);
            Em_PlaySeAtBoneOnFrame(em, 4, 4, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 6, 6, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0x12, 0xE, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x14, 0x11, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x28, 0x10, 2);
            break;
        case 0x423:
            Em_PlaySeAtBoneOnFrame(em, 0x12, 0x2C, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x6C, 0x2C, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 4, 0xE, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x38, 0x11, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x52, 0x11, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x6C, 0xA, 2);
            break;
        case 0x424:
            Em_PlaySeAtBoneOnFrame(em, 2, 0x26, 0xE);
            Em_PlaySeAtBoneVolOnFrame(em, 0x6E, Sound_RandomSe(0x42, 4, 0x43, 4), 0xE, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0xE, 7, 0x15);
            Em_PlaySeAtBoneOnFrame(em, 0x3A, 3, 0x15);
            Em_PlaySeAtBoneOnFrame(em, 0x4A, 3, 0x1E);
            Em_PlaySeAtBoneOnFrame(em, 0x2A, 0, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x60, 0, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0x2E, 0x15, 6);
            Em_PlaySeAtBoneOnFrame(em, 0x38, 0x1D, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x10, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x11, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x36, 0xE, 2);
            break;
        case 0x425:
            Em_PlaySeAtBoneOnFrame(em, 2, 0x25, 0xE);
            Em_PlaySeAtBoneVolOnFrame(em, 0x76, Sound_RandomSe(0x3C, 4, 0x3D, 4), 0xE, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0x38, 0x1D, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0x2E, 0, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0x54, 3, 0x1E);
            Em_PlaySeAtBoneOnFrame(em, 0x1C, 0xE, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x54, 0x10, 2);
            break;
        case 0x427:
            Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x26, 4, 0x27, 4), 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x1C, 0x13, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x24, 0x1D, 6);
            Em_PlaySeAtBoneOnFrame(em, 0x32, 0xE, 2);
            break;
        case 0x428:
            Em_PlaySeAtBoneOnFrame(em, 4, 0x31, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0xC, 0xE, 2);
            Em_PlaySeAtBoneOnFrame(em, 4, 0, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0x34, 0, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 2, 1, 0x15);
            Em_PlaySeAtBoneOnFrame(em, 0x26, 1, 0x15);
            Em_PlaySeAtBoneOnFrame(em, 4, 0xF, 6);
            Em_PlaySeAtBoneOnFrame(em, 0x36, 0xF, 6);
            break;
        case 0x429:
            Em_PlaySeAtBoneOnFrame(em, 4, 0xE, 2);
            Em_PlaySeAtBoneOnFrame(em, 0xE, 0, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0x10, 0x33, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x82, 0x34, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x28, 0x11, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x3C, 0x11, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x56, 0x11, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x6E, 0x11, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x82, 0x11, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x94, 0x11, 2);
            Em_PlaySeAtBoneOnFrame(em, 0xB6, 0x11, 2);
            Em_PlaySeAtBoneOnFrame(em, 0xCA, 0x11, 2);
            break;
        case 0x42A:
            Em_PlaySeAtBoneOnFrame(em, 2, 0x26, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x24, 0xE, 2);
            break;
        case 0x42B:
            Em_PlaySeAtBoneOnFrame(em, 0x2E, 0x26, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 4, 0x13, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x18, 0x1D, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0x2E, 0xE, 2);
            break;
        case 0x42C:
            Em_PlaySeAtBoneOnFrame(em, 2, 0x28, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x88, 0x21, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x44, 0xE, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x9E, 0x13, 2);
            Em_PlaySeAtBoneOnFrame(em, 0xA8, 0x12, 6);
            Em_PlaySeAtBoneOnFrame(em, 0xAE, 0x1D, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0x3A, 1, 0x1E);
            Em_PlaySeAtBoneOnFrame(em, 0x45, 1, 0x15);
            Em_PlaySeAtBoneOnFrame(em, 0xEE, 0xF, 0x15);
            Em_PlaySeAtBoneOnFrame(em, 0x12C, 0xF, 0x15);
            Em_PlaySeAtBoneOnFrame(em, 0x16C, 0x11, 0x15);
            break;
        case 0x42D:
            Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x25, 4, 0x27, 4), 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x40, 0x13, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x5C, 0x10, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x72, 0x10, 2);
            Em_PlaySeAtBoneOnFrame(em, 0xA6, 0xE, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x46, 0x1D, 6);
            Em_PlaySeAtBoneOnFrame(em, 0x4A, 0x1D, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0x62, 0x15, 6);
            Em_PlaySeAtBoneOnFrame(em, 0x6C, 0x15, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0x36, 0, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x4C, 1, 0x1E);
            Em_PlaySeAtBoneOnFrame(em, 0x5A, 2, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x64, 3, 0x1E);
            Em_PlaySeAtBoneOnFrame(em, 0x76, 2, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x7E, 3, 0x1E);
            Em_PlaySeAtBoneOnFrame(em, 0x8E, 2, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x96, 3, 0x1E);
            Em_PlaySeAtBoneOnFrame(em, 0xB6, 3, 0x1E);
            Em_PlaySeAtBoneOnFrame(em, 0xCC, 0, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x52, 1, 0x1E);
            Em_PlaySeAtBoneOnFrame(em, 0x34, 0, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0x30, 1, 0x15);
            Em_PlaySeAtBoneOnFrame(em, 0x54, 3, 0x15);
            Em_PlaySeAtBoneOnFrame(em, 0x62, 2, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0x68, 3, 0x15);
            Em_PlaySeAtBoneOnFrame(em, 0x7C, 2, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0x86, 3, 0x15);
            Em_PlaySeAtBoneOnFrame(em, 0x94, 2, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0x9C, 3, 0x15);
            Em_PlaySeAtBoneOnFrame(em, 0xC0, 0, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0xDA, 1, 0x15);
            Em_PlaySeAtBoneOnFrame(em, 0xC0, 9, 6);
            Em_PlaySeAtBoneOnFrame(em, 0xC2, 0xD, 0xA);
            break;
        case 0x42E:
            Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x42, 6, 0x43, 2), 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x42, Sound_RandomSe(0x43, 2, 0x43, 6), 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0xC, 0, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0x20, 1, 0x15);
            Em_PlaySeAtBoneOnFrame(em, 0x2A, 6, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x16, 7, 0x1E);
            Em_PlaySeAtBoneOnFrame(em, 0x1A, 0xD, 6);
            Em_PlaySeAtBoneOnFrame(em, 0x38, 0x10, 6);
            Em_PlaySeAtBoneOnFrame(em, 0x42, 0x14, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0x3C, 0xE, 2);
            break;
        case 0x42F:
            Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x26, 4, 0x27, 4), 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x1C, 0x13, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x24, 0x1D, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0x32, 0xE, 2);
            break;
        case 0x430:
            Em_PlaySeAtBoneOnFrame(em, 4, 0x31, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0xC, 0xE, 2);
            Em_PlaySeAtBoneOnFrame(em, 4, 0, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x34, 0, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 2, 1, 0x1E);
            Em_PlaySeAtBoneOnFrame(em, 0x26, 1, 0x1E);
            Em_PlaySeAtBoneOnFrame(em, 4, 0xF, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0x36, 0xF, 0xA);
            break;
        case 0x431:
            Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x42, 6, 0x43, 2), 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x42, Sound_RandomSe(0x42, 2, 0x43, 6), 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0xC, 0, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x20, 1, 0x1E);
            Em_PlaySeAtBoneOnFrame(em, 0x2A, 6, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0x16, 7, 0x15);
            Em_PlaySeAtBoneOnFrame(em, 0x1A, 0xD, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0x38, 0x10, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0x42, 0x14, 6);
            Em_PlaySeAtBoneOnFrame(em, 0x3C, 0xE, 2);
            break;
        case 0x432:
            if (type == 0x43) {
                Em_PlaySeAtBoneOnFrame(em, 4, 0x26, 0xE);
                Em_PlaySeAtBoneOnFrame(em, 0x50, 0x31, 0xE);
                Em_PlaySeAtBoneOnFrame(em, 0xC6, 0x37, 0xE);
                Em_PlaySeAtBoneOnFrame(em, 0x30, 0, 0x11);
                Em_PlaySeAtBoneOnFrame(em, 0x58, 0, 0x1A);
                Em_PlaySeAtBoneOnFrame(em, 0x7E, 0, 0x11);
                Em_PlaySeAtBoneOnFrame(em, 0x9C, 0, 0x1A);
                Em_PlaySeAtBoneOnFrame(em, 0x3C, 1, 0x1E);
                Em_PlaySeAtBoneOnFrame(em, 0x5E, 1, 0x15);
                Em_PlaySeAtBoneOnFrame(em, 0x86, 1, 0x1E);
                Em_PlaySeAtBoneOnFrame(em, 0xAE, 1, 0x15);
                Em_PlaySeAtBoneOnFrame(em, 0x12, 0xF, 2);
                Em_PlaySeAtBoneOnFrame(em, 0x3A, 0xE, 2);
                Em_PlaySeAtBoneOnFrame(em, 0x58, 0x11, 2);
                Em_PlaySeAtBoneOnFrame(em, 0xAE, 0xE, 2);
            } else {
                Em_PlaySeAtBoneOnFrame(em, 4, 0x26, 0xE);
                Em_PlaySeAtBoneOnFrame(em, 0x50, 0x31, 0xE);
                Em_PlaySeAtBoneOnFrame(em, 0xC6, Sound_RandomSe(0x3C, 4, 0x3E, 4), 0xE);
                Em_PlaySeAtBoneOnFrame(em, 0x7A, 0x12, 2);
                Em_PlaySeAtBoneOnFrame(em, 0x3A, 0xE, 2);
                Em_PlaySeAtBoneOnFrame(em, 0x12, 1, 0x1E);
                Em_PlaySeAtBoneOnFrame(em, 0x44, 5, 0x1E);
                Em_PlaySeAtBoneOnFrame(em, 0x60, 0, 0x1A);
                Em_PlaySeAtBoneOnFrame(em, 0x30, 6, 0x11);
                Em_PlaySeAtBoneOnFrame(em, 0x74, 1, 0x15);
                Em_PlaySeAtBoneOnFrame(em, 0x76, 0x1D, 6);
                Em_PlaySeAtBoneOnFrame(em, 0x80, 0x1D, 0xA);
            }
            break;
        case 0x433:
            Em_PlaySeAtBoneOnFrame(em, 4, 0x22, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x42, Sound_RandomSe(0x3C, 4, 0x3E, 4), 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x1C, 0x10, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x5C, 0xE, 2);
            Em_PlaySeAtBoneOnFrame(em, 4, 0xD, 6);
            Em_PlaySeAtBoneOnFrame(em, 0x10, 0x15, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0x5A, 0xF, 6);
            Em_PlaySeAtBoneOnFrame(em, 0x5E, 0x10, 0xA);
            break;
        case 0x434:
            Em_PlaySeAtBoneOnFrame(em, 0xE, 0x37, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 4, 0x10, 2);
            Em_PlaySeAtBoneOnFrame(em, 4, 0xE, 2);
            break;
        }
    } else {
                switch (work->fxMotion) {
        case 0x3E9:
            Em_PlaySeAtBoneOnFrame(em, 0xE, Sound_RandomSe(0xA, 2, 0xB, 3), 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x3C, Sound_RandomSe(0xB, 2, 0xC, 2), 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x78, Sound_RandomSe(0xC, 3, 0xA, 2), 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0xB4, Sound_RandomSe(0xA, 3, 0xB, 2), 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0xF0, Sound_RandomSe(0xB, 2, 0xC, 2), 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0xA8, 4, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x122, 5, 0x15);
            break;
        case 0x3EA:
            Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0xA, 2, 0xB, 2), 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x26, Sound_RandomSe(0xB, 2, 0xC, 2), 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x46, Sound_RandomSe(0xC, 2, 0xA, 2), 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x28, 0, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0x5C, 1, 0x15);
            break;
        case 0x3EB:
            Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0xA, 2, 0xB, 2), 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x26, Sound_RandomSe(0xB, 2, 0xC, 2), 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x46, Sound_RandomSe(0xC, 2, 0xA, 2), 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x62, 0, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0x30, 1, 0x15);
            Em_PlaySeAtBoneOnFrame(em, 0xC, 1, 0x1E);
            break;
        case 0x3ED:
            Em_PlaySeAtBoneOnFrame(em, 0x18, Sound_RandomSe(0xA, 4, 0xB, 4), 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x50, Sound_RandomSe(0xB, 4, 0xC, 4), 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x1C, 0, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0x28, 1, 0x15);
            Em_PlaySeAtBoneOnFrame(em, 0x48, 0, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0x54, 1, 0x15);
            Em_PlaySeAtBoneOnFrame(em, 0x12, 0, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x3E, 0, 0x1A);
            break;
        case 0x3EE:
            Em_PlaySeAtBoneOnFrame(em, 0x18, Sound_RandomSe(0xA, 4, 0xB, 4), 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x50, Sound_RandomSe(0xB, 4, 0xC, 4), 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x1C, 0, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x28, 1, 0x1E);
            Em_PlaySeAtBoneOnFrame(em, 0x48, 0, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x54, 1, 0x1E);
            Em_PlaySeAtBoneOnFrame(em, 0x12, 0, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0x3E, 0, 0x11);
            break;
        case 0x3EF:
            Em_PlaySeAtBoneOnFrame(em, 0x10, Sound_RandomSe(0xA, 4, 0xB, 4), 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x28, Sound_RandomSe(0xB, 4, 0xC, 4), 0xE);
            Em_PlaySeAtBoneOnFrame(em, 2, 0xD, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0xC, 0, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x2C, 0, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x38, 1, 0x1E);
            Em_PlaySeAtBoneOnFrame(em, 0x12, 0, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0x1E, 1, 0x15);
            Em_PlaySeAtBoneOnFrame(em, 0x3E, 1, 0x15);
            break;
        case 0x3F0:
            Em_PlaySeAtBoneOnFrame(em, 0x10, Sound_RandomSe(0xA, 4, 0xB, 4), 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x28, Sound_RandomSe(0xB, 4, 0xC, 4), 0xE);
            Em_PlaySeAtBoneOnFrame(em, 2, 0xD, 6);
            Em_PlaySeAtBoneOnFrame(em, 0xC, 0, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0x2C, 0, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0x38, 1, 0x15);
            Em_PlaySeAtBoneOnFrame(em, 0x12, 0, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x1E, 1, 0x1E);
            Em_PlaySeAtBoneOnFrame(em, 0x3E, 1, 0x1E);
            break;
        case 0x3F2:
            Em_PlaySeAtBoneOnFrame(em, 0x5C, 0x10, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x32, 0x11, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x50, 0xE, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x14, 0, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x2C, 1, 0x1E);
            Em_PlaySeAtBoneOnFrame(em, 0x2C, 0, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0x14, 1, 0x15);
            break;
        case 0x3F3:
            Em_PlaySeAtBoneOnFrame(em, 0x36, Sound_RandomSe(0xA, 4, 0xB, 4), 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x28, Sound_RandomSe(0xB, 4, 0xC, 4), 0xE);
            Em_PlaySeAtBoneOnFrame(em, 6, 0x12, 2);
            Em_PlaySeAtBoneOnFrame(em, 0xA, 0x13, 2);
            Em_PlaySeAtBoneOnFrame(em, 0xA, 0x10, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x88, 0x10, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x4A, 0x14, 6);
            Em_PlaySeAtBoneOnFrame(em, 0x4E, 0x14, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0x52, 4, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0x48, 5, 0x15);
            Em_PlaySeAtBoneOnFrame(em, 0x7C, 0, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0x8A, 1, 0x15);
            Em_PlaySeAtBoneOnFrame(em, 0x50, 4, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x54, 5, 0x1E);
            Em_PlaySeAtBoneOnFrame(em, 0x8A, 0, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x7A, 2, 0x1E);
            break;
        case 0x3F7:
            Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0xA, 2, 0xB, 2), 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x25, Sound_RandomSe(0xB, 2, 0xC, 2), 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x46, Sound_RandomSe(0xC, 2, 0xA, 2), 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x34, 0, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 2, 0, 0x1A);
            break;
        case 0x3F8:
            Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0xA, 2, 0xB, 2), 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x25, Sound_RandomSe(0xB, 2, 0xC, 2), 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x46, Sound_RandomSe(0xC, 2, 0xA, 2), 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x34, 0, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 2, 0, 0x11);
            break;
        case 0x3FA:
            Em_PlaySeAtBoneOnFrame(em, 4, 0x23, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 4, 0x11, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x16, 0xD, 0x1A);
            break;
        case 0x3FB:
            Em_PlaySeAtBoneOnFrame(em, 4, 0x24, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x10, 0x16, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x44, 0x10, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x14, 0x17, 0x1A);
            break;
        case 0x3FC:
            Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0xA, 3, 0xB, 2), 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x14, 0, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 6, 1, 0x15);
            Em_PlaySeAtBoneOnFrame(em, 0x28, 0, 0xA);
            break;
        case 0x3FD:
            Em_PlaySeAtBoneOnFrame(em, 4, 0x1E, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x18, 0, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 4, 0, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x28, 0, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x14, 0x17, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0x10, 0x16, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x3A, 0, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0x42, 1, 0x1E);
            Em_PlaySeAtBoneOnFrame(em, 0x56, 0, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x64, 1, 0x15);
            break;
        case 0x3FE:
            Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0xA, 2, 0xB, 2), 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x14, 0, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 6, 1, 0x1E);
            Em_PlaySeAtBoneOnFrame(em, 0x28, 0, 0x11);
            break;
        case 0x3FF:
            Em_PlaySeAtBoneOnFrame(em, 4, 0x1E, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x18, 0, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 4, 0, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0x28, 0, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0x14, 0x17, 6);
            Em_PlaySeAtBoneOnFrame(em, 0x10, 0x16, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0x3A, 0, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x42, 1, 0x15);
            Em_PlaySeAtBoneOnFrame(em, 0x56, 0, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0x64, 1, 0x1E);
            break;
        case 0x400:
            Em_PlaySeAtBoneOnFrame(em, 4, 0x23, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 4, 0x11, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x16, 0xD, 0x1A);
            break;
        case 0x401:
            Em_PlaySeAtBoneOnFrame(em, 4, 0x24, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x10, 0x16, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x44, 0x10, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x14, 0x17, 6);
            break;
        case 0x407:
            Em_PlaySeAtBoneOnFrame(em, 4, 0x25, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x4A, Sound_RandomSe(0xA, 4, 0xB, 4), 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x9A, 0x25, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0xE0, Sound_RandomSe(0xA, 4, 0xB, 4), 0xE);
            break;
        case 0x408:
            Em_PlaySeAtBoneOnFrame(em, 0x4A, Sound_RandomSe(0xA, 4, 0xB, 4), 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x36, 0x24, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x3C, 0x10, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x66, 0x11, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x7A, 0xD, 0xA);
            break;
        case 0x409:
            Em_PlaySeAtBoneOnFrame(em, 4, 0x21, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x3C, 0x10, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x66, 0x11, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x8E, 6, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x8E, 0xC, 0xE);
            break;
        case 0x40A:
            Em_PlaySeAtBoneOnFrame(em, 6, 0x16, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0xA, 0x16, 6);
            Em_PlaySeAtBoneOnFrame(em, 0x1A, 0xD, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0x3A, 0xD, 6);
            Em_PlaySeAtBoneOnFrame(em, 0x52, 0, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x52, 0, 0x1E);
            Em_PlaySeAtBoneOnFrame(em, 0x5C, 0, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0x86, 0, 0x1E);
            Em_PlaySeAtBoneOnFrame(em, 2, 0x19, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x40, 0x1A, 2);
            break;
        case 0x40E:
            Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0xA, 6, 0xB, 2), 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x24, 0, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0x2A, 0x11, 2);
            break;
        case 0x40F:
            Em_PlaySeAtBoneOnFrame(em, 4, 0x23, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 4, 0xF, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x12, 1, 0x1E);
            Em_PlaySeAtBoneOnFrame(em, 0x24, 0, 0x11);
            break;
        case 0x412:
            Em_PlaySeAtBoneOnFrame(em, 2, 0x23, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x24, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 4, 0x10, 2);
            Em_PlaySeAtBoneOnFrame(em, 6, 0x11, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x16, 0xA);
            break;
        case 0x413:
            Em_PlaySeAtBoneOnFrame(em, 6, 0x1E, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0xC, 0x16, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0x16, 0x16, 6);
            Em_PlaySeAtBoneOnFrame(em, 0x14, 0xD, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0x1E, 0xD, 6);
            Em_PlaySeAtBoneOnFrame(em, 0x12, 0x14, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x3C, 0x10, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x3C, 0x11, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x1C, 2, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0x64, 1, 0x1E);
            Em_PlaySeAtBoneOnFrame(em, 0x7A, 0, 0x1A);
            break;
        case 0x414:
            Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x1F, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x11, 2);
            Em_PlaySeAtBoneOnFrame(em, 0xC, 2, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0x2E, 0, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x20, 1, 0x15);
            break;
        case 0x415:
            Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0xA, 4, 0xB, 4), 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x28, Sound_RandomSe(0xB, 4, 0xC, 4), 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x1E, 0, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0x38, 1, 0x15);
            Em_PlaySeAtBoneOnFrame(em, 2, 0, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x1A, 1, 0x1E);
            break;
        case 0x416:
            Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0xA, 4, 0xB, 4), 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x28, Sound_RandomSe(0xB, 4, 0xC, 4), 0xE);
            Em_PlaySeAtBoneOnFrame(em, 2, 0, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0x16, 1, 0x15);
            Em_PlaySeAtBoneOnFrame(em, 0x28, 0, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0x3E, 1, 0x15);
            Em_PlaySeAtBoneOnFrame(em, 2, 1, 0x1E);
            Em_PlaySeAtBoneOnFrame(em, 0x16, 0, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x2A, 1, 0x1E);
            Em_PlaySeAtBoneOnFrame(em, 0x3E, 0, 0x1A);
            break;
        case 0x417:
            Em_PlaySeAtBoneOnFrame(em, 4, 0x1F, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x46, 0x1E, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x50, 0x16, 6);
            Em_PlaySeAtBoneOnFrame(em, 0x52, 0x17, 6);
            Em_PlaySeAtBoneOnFrame(em, 0x30, 2, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0x3E, 1, 0x15);
            Em_PlaySeAtBoneOnFrame(em, 0x4C, 1, 0x1E);
            Em_PlaySeAtBoneOnFrame(em, 0x54, 2, 0x1A);
            break;
        case 0x419:
            Em_PlaySeAtBoneOnFrame(em, 4, 0x1F, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x46, 0x1E, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x50, 0x16, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0x52, 0x17, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0x30, 2, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x3E, 3, 0x1E);
            Em_PlaySeAtBoneOnFrame(em, 0x4C, 3, 0x15);
            Em_PlaySeAtBoneOnFrame(em, 0x54, 2, 0x11);
            break;
        case 0x41B:
            Em_PlaySeAtBoneOnFrame(em, 0x4A, 0x23, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x72, 0x1E, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x1E, 0xD, 6);
            Em_PlaySeAtBoneOnFrame(em, 0x26, 0xD, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0x56, 0xD, 6);
            Em_PlaySeAtBoneOnFrame(em, 0x5E, 0xD, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0x80, 0xD, 6);
            Em_PlaySeAtBoneOnFrame(em, 0x86, 0xD, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0xBE, 0xD, 6);
            Em_PlaySeAtBoneOnFrame(em, 0xB8, 0xD, 0xA);
            break;
        case 0x41C:
            Em_PlaySeAtBoneOnFrame(em, 0xC, 0x1F, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x10, 0xD, 6);
            Em_PlaySeAtBoneOnFrame(em, 0x14, 0xD, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0x7A, 0xD, 6);
            Em_PlaySeAtBoneOnFrame(em, 0x7E, 0xD, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0x6A, 2, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0xA6, 0, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0xD4, 0, 0x1A);
            break;
        case 0x41D:
            break;
        case 0x41F:
            Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x24, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x40, Sound_RandomSe(0xA, 4, 0xB, 4), 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x72, Sound_RandomSe(0xB, 4, 0xA, 4), 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x194, 0x17, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x1D6, 0x17, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x206, 0x17, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x1F4, Sound_RandomSe(0xB, 4, 0xC, 4), 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x21C, Sound_RandomSe(0xB, 4, 0xA, 4), 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x244, Sound_RandomSe(0xA, 4, 0xC, 4), 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x26C, Sound_RandomSe(0xC, 4, 0xA, 4), 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x11E, 0x17, 6);
            Em_PlaySeAtBoneOnFrame(em, 0xE4, 0xD, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0xFC, 0x17, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0x174, 0x14, 6);
            Em_PlaySeAtBoneOnFrame(em, 0x176, 5, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0x1C0, 0x11, 6);
            Em_PlaySeAtBoneOnFrame(em, 0x1E0, 0x11, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0xA8, 2, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0x10A, 1, 0x15);
            Em_PlaySeAtBoneOnFrame(em, 0x296, 1, 0x15);
            Em_PlaySeAtBoneOnFrame(em, 0x2C0, 2, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0xC0, 1, 0x1E);
            Em_PlaySeAtBoneOnFrame(em, 0xDC, 0, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x2A8, 0, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x2CE, 3, 0x1E);
            Em_PlaySeAtBoneOnFrame(em, 0x2B2, 0x10, 2);
            break;
        case 0x421:
            Em_PlaySeAtBoneOnFrame(em, 0xC, 0x23, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x17, 6);
            Em_PlaySeAtBoneOnFrame(em, 0x24, 0x17, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0x36, 0x17, 6);
            Em_PlaySeAtBoneOnFrame(em, 0x42, 0x17, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0x50, 0x17, 6);
            Em_PlaySeAtBoneOnFrame(em, 0x5A, 0x17, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0x68, 0x17, 6);
            Em_PlaySeAtBoneOnFrame(em, 0x26, 1, 0x15);
            Em_PlaySeAtBoneOnFrame(em, 0x38, 2, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0x60, 0x10, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x68, 0x15, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x78, 0x1A, 2);
            break;
        case 0x424:
            Em_PlaySeAtBoneOnFrame(em, 2, 0x20, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x78, Sound_RandomSe(0xB, 4, 0xC, 4), 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x5E, 2, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0xE, 1, 0x15);
            Em_PlaySeAtBoneOnFrame(em, 0x4C, 3, 0x1E);
            Em_PlaySeAtBoneOnFrame(em, 0x28, 0, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x3A, 0x13, 0xA);
            Em_PlaySeAtBoneOnFrame(em, 0x20, 0xD, 6);
            break;
        case 0x425:
            Em_PlaySeAtBoneOnFrame(em, 2, 0x21, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x2E, 2, 0x11);
            Em_PlaySeAtBoneOnFrame(em, 0x58, 3, 0x1E);
            Em_PlaySeAtBoneOnFrame(em, 0x34, 0x11, 2);
            break;
        case 0x429:
            Em_PlaySeAtBoneOnFrame(em, 0x10, 0x26, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x78, 0x26, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x28, 0x11, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x3E, 0xD, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x6E, 0x11, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x84, 0xD, 2);
            Em_PlaySeAtBoneOnFrame(em, 0xB4, 0x11, 2);
            break;
        case 0x42C:
            Em_PlaySeAtBoneOnFrame(em, 0xDC, Sound_RandomSe(0xA, 4, 0xB, 4), 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x10E, Sound_RandomSe(0xB, 4, 0xC, 4), 0xE);
            Em_PlaySeAtBoneOnFrame(em, 2, 0x22, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0xA8, 0x12, 2);
            Em_PlaySeAtBoneOnFrame(em, 0xB0, 0x13, 2);
            Em_PlaySeAtBoneOnFrame(em, 0xBE, 0x10, 2);
            Em_PlaySeAtBoneOnFrame(em, 0xC0, 0x11, 2);
            Em_PlaySeAtBoneOnFrame(em, 0x3A, 1, 0x1E);
            Em_PlaySeAtBoneOnFrame(em, 0x45, 1, 0x15);
            break;
        case 0x42E:
            break;
        case 0x434:
            Em_PlaySeAtBoneOnFrame(em, 2, 0x1F, 0xE);
            Em_PlaySeAtBoneOnFrame(em, 0x20, 0xD, 6);
            Em_PlaySeAtBoneOnFrame(em, 0xC0, 0x10, 2);
        default:
            break;
        }
    }
}

/* em_garuga's .data (0x8C41E0-0x8C4A90) */
EmFlyParam EmGaruga_FlyParam = {
#include "game/008C41E0.inc"
};
EmFlyParam EmGaruga_FlyParam2 = {
#include "game/008C4240.inc"
};
EmTurnParams EmGaruga_TurnParams[2] = {
#include "game/008C42A0.inc"
};
EmGarugaVec EmGaruga_CarveThrowPt = {
#include "game/008C42D8.inc"
};
extern EmCurveKey EmCephadrome_VelYCurves_3[];
s32 GmEm_ReturnZero(void); /* gm_sub */
void *EmGaruga_Callbacks[8] = {
    EmGaruga_SpawnEffects, EmGaruga_SpawnEffects, EmGaruga_TableNop, EmGaruga_UpdateMotionEffects,
    GmEm_ReturnZero, EmGaruga_Reinit, EmGaruga_AllowHit8, NULL,
};
EmCurveKey EmGaruga_MoveVelYCurves_0[8] = {
#include "game/008C4310.inc"
};
EmCurveKey EmGaruga_MoveVelYCurves_1[20] = {
#include "game/008C4350.inc"
};
EmCurveKey EmGaruga_MoveVelYCurves_3[10] = {
#include "game/008C43F0.inc"
};
EmCurveKey EmGaruga_MoveVelYCurves_5[6] = {
#include "game/008C4440.inc"
};
EmCurveKey EmGaruga_MoveVelYCurves_6[10] = {
#include "game/008C4470.inc"
};
EmCurveKey *EmGaruga_MoveVelYCurves[16] = {
    EmGaruga_MoveVelYCurves_0, EmGaruga_MoveVelYCurves_1, NULL, EmGaruga_MoveVelYCurves_3,
    NULL, EmGaruga_MoveVelYCurves_5, EmGaruga_MoveVelYCurves_6, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
};
EmCurveKey EmGaruga_VelYCurves_3[10] = {
#include "game/008C4500.inc"
};
EmCurveKey EmGaruga_VelYCurves_5[14] = {
#include "game/008C4550.inc"
};
EmCurveKey EmGaruga_VelYCurves_8[8] = {
#include "game/008C45C0.inc"
};
EmCurveKey EmGaruga_VelYCurves_14[8] = {
#include "game/008C4600.inc"
};
EmCurveKey EmGaruga_VelYCurves_9[18] = {
#include "game/008C4640.inc"
};
EmCurveKey EmGaruga_VelYCurves_10[14] = {
#include "game/008C46D0.inc"
};
EmCurveKey EmGaruga_VelYCurves_11[4] = {
#include "game/008C4740.inc"
};
EmCurveKey EmGaruga_VelYCurves_12[6] = {
#include "game/008C4760.inc"
};
EmCurveKey EmGaruga_VelYCurves_13[24] = {
#include "game/008C4790.inc"
};
EmCurveKey *EmGaruga_VelYCurves[16] = {
    NULL, NULL, EmCephadrome_VelYCurves_3, EmGaruga_VelYCurves_3,
    NULL, EmGaruga_VelYCurves_5, NULL, NULL,
    EmGaruga_VelYCurves_8, EmGaruga_VelYCurves_9, EmGaruga_VelYCurves_10, EmGaruga_VelYCurves_11,
    EmGaruga_VelYCurves_12, EmGaruga_VelYCurves_13, EmGaruga_VelYCurves_14, NULL,
};
EmCurveKey EmGaruga_SpeedCurves_5[14] = {
#include "game/008C4890.inc"
};
EmCurveKey EmGaruga_SpeedCurves_8[8] = {
#include "game/008C4900.inc"
};
EmCurveKey EmGaruga_SpeedCurves_14[8] = {
#include "game/008C4940.inc"
};
EmCurveKey EmGaruga_SpeedCurves_9[14] = {
#include "game/008C4980.inc"
};
EmCurveKey EmGaruga_SpeedCurves_10[12] = {
#include "game/008C49F0.inc"
};
EmCurveKey *EmGaruga_SpeedCurves[16] = {
    NULL, NULL, NULL, NULL,
    NULL, EmGaruga_SpeedCurves_5, NULL, NULL,
    EmGaruga_SpeedCurves_8, EmGaruga_SpeedCurves_9, EmGaruga_SpeedCurves_10, NULL,
    NULL, NULL, EmGaruga_SpeedCurves_14, NULL,
};
