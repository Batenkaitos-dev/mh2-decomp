/*
 * Monster family: Gravios, Black Gravios and Basarios (kinds 0x11, 0x2F, 0x16).
 * The code from the family's setup function (0x764DE0, called by the per-kind
 * dispatch Em_TypeSetup) to the next family's. The state functions take
 * the monster and the work area its state dispatcher passes on (EmWork);
 * Em.step is the step within the current state. See include/em.h.
 */
#include "common.h"
#include "quest.h"
#include "em.h"
#include "player.h"
#include "scene.h"
/* prototypes of callees (round 13) */
s32 Versus_DefeatCamera(Em *pl);
void Versus_RouteEmByTeam(Em *em);
void Em_PlaySeAtBone(Em *em, s32 se, s32 bone);
void Em_SetStateRand(Em *em, s32 arg1, s32 arg2);

/* prototypes */
void EmGravios_ChangeState(Em *em, u16 group, u16 state, u8 mode);
s8 EmMine_CreateTimed0(Em *em, f32 *pos);
void EmVespoid_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void Em_AddHp(Em *em, s16 n);
void Em_BeginStatus1(Em *em);
void Em_BeginStatus4(Em *em);
void Em_EndStatus2(Em *em);
void Em_EndStatus4(Em *em);
void Em_PlaySeAtBoneOnFrame(Em *em, s32 frame, s32 se, s32 bone);
void Em_SetLayerMotion(Em *em, s32 motion, s32 blend, s32 frame, s32 layer);
void Em_SetMotion(Em *em, s32 motion, s32 a, s32 b);
u16 Math_AngleXZ(f32 *from, f32 *to);
s32 Motion_PassedFrame(void *, s32 layer, f32 frame);
void Player_ClearFlag(void *, u32 flag);
void Player_SetFlag(void *obj, u32 flag);
void QuestEm_Captured(Em *em);
u16 Rand_Next(s32 stream);
void EffEm08_CreateAtBone2(Em *, s32, s32, s32, s32, s32, s32);
void EffEm11_Create(Em *, s8);
void Em_SetRow638(Em *em, u8 idx);
void Em_MoveRotated(Em *em, void *src);
void Em_MoveRotatedAccel(Em *, void *);
void Em_LoadReactionSet(Em *em, u8 k);
void Em_GetPitchToGoal(Em *em, s32 *out, f32 a, f32 b);
void Em_ClearLowHpNeed(Em *em);
void Em_SpawnEffect06Bone(Em *em, s32 a);
void Em_SetFlag6AE(Em *em, u8 a);
void Em_SetGoal12(Em *em);
void Em_UpdatePendingScript(Em *em);
void Em_ClearFlag6AE(Em *em);
void Em_ClearPoison(Em *em);
void Em_SetHold2(Em *em);
void Em_SetCaptured(Em *em, s32 a);
void Em_PlaySeAtBoneOnFrame2(Em *em, s32 frame, s32 se, s32 bone, s32 flag);
s32 Em_TurnToTargetMotion(Em *em, u16 range, u16 motFront, u16 motLeft, u16 motRight);
s32 Em_StepToGoal(Em *em, u16 step, u8 flags);
u8 Em_TurnToGoal(Em *em, u16 step, u8 flags);
void Em_RestartScript(Em *em);
void EmGravios_Reset(Em *);
void EmGravios_State0_24(Em *, EmWork *);
void EmGravios_State1_03(Em *, s32, s32);
void EmGravios_State1_07(Em *, EmWork *);
void EmGravios_State2_03(Em *em, EmWork *work);
void EmGravios_State6_00_Entrance(Em *, EmWork *);
void EmGravios_State8_00(Em *, EmWork *);
void EmGravios_State5_00_Hunted(Em *, EmWork *);
void EmGravios_State5_01_Hunted(Em *, EmWork *);
void EmGravios_State5_02_HuntedFalling(Em *, EmWork *);
void EmGravios_State5_03_Hunted(Em *, EmWork *);
void EmGravios_State5_04(Em *, EmWork *);
void EmGravios_RunGroup3(Em *em, EmWork *work);
void EmGravios_RunGroup4(Em *em, EmWork *work);
struct EmGraviosFxWork;
void EmGravios_MotionEffects(Em *em, struct EmGraviosFxWork *);
void EmGravios_MoveByVelYCurve(Em *em, s32 kind);
void EmGravios_StartCurves(Em *em, u8 kind);
u8 EmGravios_UpdateCurves(Em *em);
void EmGravios_SpawnPeriodicEffect06(Em *em);
void EmKind12_State1_01(Em *em);
void EmKind12_State1_02(Em *);
void EmKind12_RunState(Em *em);
void EmKind12_MotionEffects(Em *, EmWork *);
void EmVespoid_Init(Em *em);
void EmVespoid_State0_03(Em *);
void EmVespoid_RunGroup1(Em *em);
void EmVespoid_State2_00(Em *em, s32 arg1);
void EmVespoid_State2_01(Em *, s16);
void EmVespoid_State2_03(Em *em);
void EmVespoid_State2_04(Em *em);
void EmVespoid_State2_06(Em *em);
void EmVespoid_State2_08(Em *);
void EmVespoid_State3_00(Em *em);
void EmVespoid_State3_01(Em *);
void EmVespoid_State3_04(Em *);
void EmVespoid_State4_01(Em *em);
void EmVespoid_State4_04(Em *em);
void EmVespoid_State5_00_Hunted(Em *);
void EmVespoid_State5_01_HuntedFalling(Em *);
void EmVespoid_State5_02(Em *);
void EmVespoid_State7_00(Em *);
void EmVespoid_StartCurves(Em *em, u8 kind);
u8 EmVespoid_UpdateCurves(Em *);
void EmVespoid_MoveByVelocity(Em *em);
void StageObj_K13_Create(void *owner, s16 v, u8 k);
/* unit's own declaration: with the definition's void (Em *em, u8 k, s16 v, s16 t, f32 y, f32 scale) (game/unk_874B90.c), EmGravios_State4_0E: 69 of 109 words differ, EmGravios_State4_10: 55 of 84 words differ */
void StageObj_Stun_CreateEm(Em *, s32, s32, s32, f32, f32);
s32 Sound_RandomSe(s32 id, s16 lo, s32 alt, s16 n);
s32 Player_PassedFrameScaled(void *p, f32 frame, s32 layer);
s32 EmMotion_ReachedFrame(void *obj, s32 a, f32 frame);
f32 Vec_Distance(f32 *a, f32 *b);
void CamShake_ReqAIf(Em *obj, s32 i);
f32 *Mtx_FromAnglesXYZ(s32 *angles, f32 *m);
f32 Vec_DistanceXZ(f32 *a, f32 *b);
s32 Versus_BothDefeated(void);
s32 QuestEm_CanAppear(Em *em);
void Player_Integrate(void *);
void Player_ClearVelocity(void *);
void Motion_InitTime(void *p, s16 step);
void Effect01_Spawn(void *obj, s16 type);
void Em_PlaySeOnFrame(Em *em, s32 frame, s32 se);
/* unit's own declaration: with the definition's s16 (void *obj, u8 state, u8 sub) (sub_main/unk_3A1070.c), func_00770310: 18 of 26 words differ */
s32 Player_IsState(void *obj, s32 group, s32 state);
void Em_SpawnEffect06Periodic(Em *em, s32 bone, f32 *ofs, f32 scale);
void Em_SetTargetAngle(Em *em);
/* unit's own declaration: with the definition's void (Em *em, s32 group, s32 state, u8 mode) (game/unk_701810.c), EmGravios_ChangeState: 8 of 237 words differ, EmGravios_SetGroup0State: 1 of 148 words differ ... */
void Em_SetStateSync(Em *em, u16 group, u16 state, u8 mode);
void Em_TargetPlayerInRange(Em *);
s32 Game_IsOnline(void);
s32 func_00770310(Em *em);
void Versus_MarkDefeat(Em *);
s8 QuestEm_Appear(Em *em);
void Em_ResetToBasePose(Em *em);
void Em_BreakPart(Em *em, s32 part);
void Em_EndAllStatus(Em *em);
void Em_BeginStatus5(Em *em);
void Em_EndStatus1(Em *em);
void Em_EndStatus5(Em *em);
void EmGravios_SetGroup0State(Em *em, u16 state, u8 mode);
void EmGravios_SetGroup1State(Em *em, u16 state, u8 mode);
void EmGravios_SetGroup2State(Em *em, u16 state, u8 mode);
void Em_SetSpeedScaled(Em *em, u16 flags, f32 x, f32 y);
void Em_CheckRageByHp(Em *em, s16 n);
void Em_ComputeGoal(Em *em, f32 *target);
void Em_TypeSetup(Em *em);
void Em_BeginStatus2(Em *em);
s32 Em_GetTargetPos(Em *em, u8 idx, f32 *out);
void Em_StartRage(Em *em);
void Em_StartPendingRage(Em *em);
void Vec_Set(f32 *out, f32 x, f32 y, f32 z);
void Em_ResetNotice(Em *em);
void Em_Refill7F4(Em *em);
void Em_StartGoalAction(Em *em, s32 arg1);
void Em_ComputeGoal2(Em *em, f32 *target);
void Em_ClearPart(Em *em, u16 part);
void Em_Add7F0(Em *em, s32 n);
void Em_Add7EC(Em *em, s32 n);
void Em_Clear7F0State(Em *em);
void Em_Clear7ECState(Em *em);
void MinePoint_Remove(s32);
void Vec_Copy(f32 *out, f32 *in);
void Vec_Sub(f32 *out, f32 *a, f32 *b);
void Vec_Normalize(f32 *v);
void Em_StartTurnToTarget(Em *em, EmTurnParams *t);
s32 Em_TurnStep(Em *em, EmTurnParams *t);
extern EmTurnParams EmGravios_TurnParams[1];
void Em_SetMotionRow(Em *em, s32 row, s32 arg2, s32 arg3);
void Em_SetPartScaled(Em *em, s32 bit);
s32 Em_ComputeSize(Em *em, s16 idx);
void StageObj_K09_Create(Em *);
void EffEm08_CreateAtBone(Em *, s32, s32, s32, s32, s32);
extern u8 D_008C4B30;
extern u8 D_008C4B32;
typedef struct EmGraviosVec {
    f32 x, y, z;
} EmGraviosVec;
extern EmGraviosVec EmGravios_CarveThrowPt __attribute__((aligned(8)));
void StageObj_K25_Create(Player *p, Em *em, u8 k, s32 unused, f32 *pos);
extern EmCurveKey *EmGravios_MoveVelYCurves[];
void EmGravios_State3_04(Em *em, EmWork *work, s32 variant);
void EmGravios_State3_05(Em *em, EmWork *work, s16 variant);
void QuestEm_Hunted(Em *em);
s8 EmMine_Create(Em *em, s32 idx);
s32 EmMine_Update(Em *em);
void EmMine_Remove(Em *em);
u16 Area_BaseId(u16);
u16 Area_DayId(u16 id);
extern f32 gAreaGroundY[];
extern EmCurveKey *EmVespoid_VelYCurves[];
extern EmCurveKey *EmVespoid_SpeedCurves[];
void Vec_RotateY(f32 *v, f32 angle);
s32 Em_UpdateCombatTimer(Em *em);
u8 Em_UpdateDamage(Em *em, u8 *out);
void Em_SetStatus4Timer(Em *em);
void Em_SetStatus2Timer(Em *em);
void EmVespoid_RunState(Em *em);
void Em_RaiseTrigger80Lax(Em *em);
void Em_HoldPosNoPlayers(Em *em, u16 a, u16 b);
void Em_SetGoal12Dist(Em *em, u8 *p);
void Em_SetStatus1Timer(Em *em);
void Em_SetStatus5Timer(Em *em);
void Em_ResetKindTimer(Em *em);
s32 Save_TestEventFlag(s32 flag);
void EmGravios_RunState(Em *em, s32 arg1);
void EmGravios_StartDown(Em *em);
typedef struct EmBEntry {
    s16 id;
    s16 pad;
    f32 *pts;
} EmBEntry;
struct EmIdEntry;
struct EmIdEntry *Em_FindAreaEntry(Em *em, struct EmIdEntry *entry, s16 id);
extern EmBEntry **gEmKindTables[];
extern f32 gStartPos[];
void EffGravios19_Create(Em *em, s8 kind);
void StageObj_K14_Create(Em *em, s16 k, s16 c, f32 scale);
void GmEm_ResetSize(Em *em);
/* the calls that mask the value again (u8 or & 0xFF) cast it to s32 first: MWCC keeps the game's andi that way */
u8 Player_InCurrentArea(void *obj);
void StageObj_K15_Create(Em *em, u8 k);
void Em_BonePos(void *obj, s16 bone, f32 *out);
void Effect02_SpawnForOwner(void *owner, s32 ang, s32 type, s32 sub, f32 *pos, f32 scale);
void Effect0D_SpawnAt(f32 *pos, s32 a, f32 scale);
void Player_Move(void *obj);
void Effect0D_SpawnForEm(Em *em, s32 a, s32 b);
extern f32 EmVespoid_Scales[];
void GmEm_ApplySize(Em *em);
void Em_InitParts(Em *em);
typedef struct EmGraviosFxWork {
    u8 pad[2];
    s16 motion;         /* 0x02: the last motion seen by the effects */
} EmGraviosFxWork;
void EmGravios_MotionSounds(Em *em, EmWork *work);
void StageObj_K0F_CreateOnEm(Em *em, u8 k, u8 c, f32 scale);
void StageObj_K11_CreateAtScaled(f32 *pos, s32 v, s16 k, f32 scale);
u8 AreaGrid2_OnFlaggedFloor(f32 *pos);
s32 EmMotion_InFrameRange(void *em, s32 layer, f32 from, f32 to);
void Effect0D_SpawnForEmScaled(Em *em, s32 a, s32 b, f32 scale);
/* end of prototypes */

void EmGravios_Setup(Em *em)
{
    if (em->type == 0x16) {
        Em_SetCombat(em, 0, 0);
        return;
    }
    if (Area_DayId(em->area) == 0xA8) {
        Em_SetCombat(em, 0, 0);
        em->noReact = 1;
        em->senseDelay = 5;
        em->noHitTimer = 5;
        em->noPushTimer = 5;
    }
}

void EmGravios_SpawnEffects(void *em)
{
    StageObj_K13_Create(em, 0x22, 0);
    Effect01_Spawn((Player *) em, 0);
}

void EmGravios_Reset(Em *em)
{
    EmFlyerWork *w = (EmFlyerWork *)&em->work;

    f32 fslot;
    s16 size;
    u8 slotNo;
    u8 fxFlags;

    Em_SetPartScaled(em, 3);
    Em_SetPartScaled(em, 4);
    Em_SetPartScaled(em, 8);
    if (gQuest.stage == 0) {
        em->rotY = 0x8000;
        slotNo = em->slotNo;
        fslot = (f32) (u32) slotNo;
        em->posX = 5000.0f + (1500.0f * fslot);
        em->posY = 0.0f;
        em->posZ = 9000.0f;
    }
    em->stateGroup = 0;
    em->state = 0;
    size = Em_ComputeSize(em, 0);
    em->hp = size;
    em->hpMax = size;
    em->stance = 0;
    w->fxMotion = 0;
    em->blind = 0;
    em->unk_InitFlag = 1;
    em->noAlert = 0;
    em->painted = 0;
    ((EmWork *)w)->goalYaw = 0x4000;
    w->pitchStep = 0x100;
    ((EmWork *)w)->turnSpeed = 0x200;
    w->rollSpeed = 0x100;
    w->moveAngBY = 0x2000;
    w->moveAngBX = 0;
    w->unk048 = 0;
    w->unk049 = 0;
    w->unk04A = 0;
    fxFlags = em->fxFlags & 1;
    em->fxFlags = fxFlags;
    if (fxFlags == 0) {
        switch (em->type) {
        case 0x11:
        case 0x2F:
        case 0x16:
            em->meshShown[1] = 0;
            StageObj_K09_Create(em);
            break;
        }
    }
}

void EmGravios_Start(Em *em)
{
    EmGravios_Reset(em);
    if (em->type == 0x11 || em->type == 0x2F) {
        Em_SetMotionRow(em, 1, 0, 0);
        EmGravios_ChangeState(em, 0, 1, 0);
        em->scriptPending = 1;
    } else {
        Em_SetMotionRow(em, 0x6B, 0, 0);
        EmGravios_ChangeState(em, 0, 0x16, 0);
        em->scriptPending = 0;
    }
}

void EmGravios_Reinit(Em *em)
{
    EmGravios_Reset(em);
}

void EmGravios_EndState(Em *em, s32 unused1, s32 unused2)
{
    em->scriptPending = 1;
    em->motionSpeed = 1.0f;
    em->stance = 0;
    em->lookAtTarget = 0;
    EmGravios_ChangeState(em, 0, 1, 0);
}

void EmGravios_EndState2(Em *em, s32 unused)
{
    em->scriptPending = 1;
    em->motionSpeed = 1.0f;
    em->stance = 2;
    em->lookAtTarget = 0;
    EmGravios_ChangeState(em, 2, 0xE, 0);
}

void EmGravios_RestartLayerMotion(Em *em, s32 layer)
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

void EmGravios_State0_00(Em *em, EmWork *work)
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

void EmGravios_State0_01(Em *em, EmWork *work)
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
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGravios_State0_02(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x65, 0, 0);
        break;
    case 1:
        if ((em->remote == 0) && (em->motion[0].playing == 0)) {
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGravios_State0_03(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x66, 0, 0);
        em->timer = (em->stateRand & 3) * 0x12C;
        break;
    case 1:
        if (em->remote == 0) {
            if (--em->timer <= 0) {
                em->step++;
                EmGravios_EndState(em, 0, 0);
            }
        }
        break;
    }
}

void EmGravios_State0_04(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x67, 0, 0);
        break;
    case 1:
        if ((em->remote == 0) && (em->motion[0].playing == 0)) {
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGravios_State0_05(Em *em, EmWork *work)
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
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
    EmGravios_RestartLayerMotion(em, 0);
    EmGravios_RestartLayerMotion(em, 2);
    Em_PlaySeAtBoneOnFrame2(em, 0x20, 0x57, 0x23, 1);
}

void EmGravios_State0_06(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x19, 0, 0);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
    EmGravios_RestartLayerMotion(em, 0);
    EmGravios_RestartLayerMotion(em, 2);
    Em_PlaySeAtBoneOnFrame2(em, 0x20, 0x20, 0x23, 1);
    Em_PlaySeAtBoneOnFrame2(em, 0xA0, 0x17, 0x23, 1);
}

void EmGravios_State0_07(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x1A, 0, 0);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
    EmGravios_RestartLayerMotion(em, 0);
    EmGravios_RestartLayerMotion(em, 2);
    Em_PlaySeAtBoneOnFrame2(em, 4, 0x55, 0x23, 1);
}

void EmGravios_State0_08(Em *em, EmWork *work)
{
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x6B, 0, 0);
        Em_LoadReactionSet(em, 1);
        em->timer = (em->stateRand & 3) * 0x12C;
        break;
    case 1:
        if (em->remote == 0) {
            if (--em->timer <= 0) {
                em->step++;
                EmGravios_EndState(em, 0, 0);
            }
        }
        break;
    }
}

void EmGravios_State0_09(Em *em, EmWork *work)
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
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGravios_State0_0A(Em *em, EmWork *work)
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
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGravios_State0_0B(Em *em, EmWork *work)
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
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
    EmGravios_RestartLayerMotion(em, 0);
    EmGravios_RestartLayerMotion(em, 2);
    Em_PlaySeAtBoneOnFrame2(em, 0x32, 0x23, 0x23, 1);
    Em_PlaySeAtBoneOnFrame2(em, 0x68, 0x1F, 0x23, 1);
}

void EmGravios_State0_0C(Em *em, EmWork *work)
{
    em->guardTimer = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x6C, 0, 0);
        em->blind = 0;
        EmMine_CreateTimed0(em, &em->posX);
        break;
    case 1:
        if ((em->remote == 0) && (em->motion[0].playing == 0)) {
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGravios_State0_0D(Em *em, EmWork *work)
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
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
    EmGravios_RestartLayerMotion(em, 0);
    EmGravios_RestartLayerMotion(em, 2);
    Em_PlaySeAtBoneOnFrame2(em, 2, 0x20, 0x23, 1);
    Em_PlaySeAtBoneOnFrame2(em, 0x4A, 0x1F, 0x23, 1);
}

void EmGravios_State0_0E(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        break;
    case 1:
        if (Em_TurnToTargetMotion(em, 0xE38, 2, 6, 5) != 0) {
            if (em->type == 0x16) {
                em->step = 3;
            } else {
                em->step++;
            }
            em->lookAtTarget = 0;
            Em_SetMotion(em, 0x6E, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->lookAtTarget = 0;
            em->step++;
            Em_SetMotion(em, 0x72, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->lookAtTarget = 0;
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGravios_State0_0F(Em *em, EmWork *work)
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
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
    Em_PlaySeAtBoneOnFrame2(em, 0x20, 0x57, 0x23, 1);
    Em_PlaySeAtBoneOnFrame2(em, 0x11A, 0x20, 0x23, 1);
}

void EmGravios_State0_10(Em *em, EmWork *work)
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
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGravios_State0_11(Em *em, EmWork *work)
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
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGravios_State0_13(Em *em, EmWork *work)
{
    Em_SetFlag6AE(em, 3U);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x68, 0, 0);
        Em_SetRow638(em, 1);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetRow638(em, 0);
            Em_LoadReactionSet(em, 0);
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGravios_State0_12(Em *em, EmWork *work)
{
    s16 n;

    Em_SetFlag6AE(em, 2U);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x27, 0, 0);
        Em_LoadReactionSet(em, 1);
        em->timer = 0x708;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x22, 0, 0);
        }
        break;
    case 2:
        n = em->hpMax;
        if (em->hp < n) {
            Em_AddHp(em, 1);
        } else {
            Em_AddHp(em, n);
        }
        if (--em->timer <= 0) {
            em->step++;
            Em_AddHp(em, em->hpMax);
            Em_ClearLowHpNeed(em);
            EmGravios_ChangeState(em, 0, 0x17, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmGravios_ChangeState(em, 0, 0x17, 4);
        }
        break;
    }
}

void EmGravios_State0_14(Em *em, EmWork *work)
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
            EmGravios_ChangeState(em, 0, 0x18, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmGravios_ChangeState(em, 0, 0x18, 4);
        }
        break;
    }
}

void EmGravios_State0_16(Em *em, EmWork *work)
{
    em->hideShadowHold = 2;
    em->hideShadow = 2;
    em->blind = 2;
    Em_SetFlag6AE(em, 3U);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x6B, 0, 0);
        Em_LoadReactionSet(em, 1);
        em->timer = 0x1518;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmGravios_ChangeState(em, 0, 0xC, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmGravios_ChangeState(em, 0, 0xC, 4);
        }
        break;
    }
}

void EmGravios_State0_17(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x25, 0, 0);
        Em_LoadReactionSet(em, 0);
        em->blind = 2;
        EmMine_CreateTimed0(em, &em->posX);
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            EmGravios_ChangeState(em, 0, 0x19, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmGravios_ChangeState(em, 0, 0x19, 4);
        }
        break;
    }
}

void EmGravios_State0_19(Em *em, EmWork *work)
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
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGravios_State0_1A(Em *em, EmWork *work)
{
    em->hideShadowHold = 2;
    em->hideShadow = 2;
    em->blind = 2;
    Em_SetFlag6AE(em, 3U);
    switch (em->step) {
    case 0:
        em->noReact = 0;
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x6B, 0, 0);
        Em_LoadReactionSet(em, 1);
        em->timer = 0x258;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            em->timer = 0x1518;
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            EmGravios_ChangeState(em, 0, 0xC, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmGravios_ChangeState(em, 0, 0xC, 4);
        }
        break;
    }
}

void EmGravios_State0_1B(Em *em, EmWork *work)
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
            EmGravios_ChangeState(em, 0, 0x1C, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmGravios_ChangeState(em, 0, 0x1C, 4);
        }
        break;
    }
}

void EmGravios_State0_1D(Em *em, EmWork *work)
{
    Em_SetFlag6AE(em, 2U);
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
            EmGravios_ChangeState(em, 4, 0x13, 3);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmGravios_ChangeState(em, 4, 0x13, 3);
        }
        break;
    }
    EmGravios_SpawnPeriodicEffect06(em);
}

void EmGravios_State0_1F(Em *em, EmWork *work)
{
    Em_SetFlag6AE(em, 2U);
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
            EmGravios_ChangeState(em, 4, 0x14, 3);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmGravios_ChangeState(em, 4, 0x14, 3);
        }
        break;
    }
    EmGravios_SpawnPeriodicEffect06(em);
}

void EmGravios_State0_24(Em *em, EmWork *work)
{
    f32 *pts;
    u16 ofs;
    u16 base;

    em->hideShadowHold = 2;
    em->hideShadow = 2;
    em->blind = 2;
    em->noReact = 1;
    em->senseDelay = 5;
    em->guardTimer = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x78, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 60;
            Player_SetFlag((Player *) em, 0x20000);
        }
        break;
    case 2:
        em->noHitTimer = 2;
        if (--em->timer <= 0) {
            em->prevArea = em->area;
            em->area = em->nextArea;
            pts = ((EmBEntry *)Em_FindAreaEntry(em, (struct EmIdEntry *)gEmKindTables[em->type][1], em->area))->pts;
            if (pts == NULL) {
                Vec_Copy(&em->posX, &gStartPos[em->area * 3]);
                em->rotY = 0;
            } else {
                switch (em->variant) {
                default:
                    ofs = em->stateRand & 1;
                    break;
                case 1:
                    ofs = 0;
                    break;
                case 2:
                    ofs = 1;
                    break;
                }
                switch (Area_DayId(em->area)) {
                case 0xA2:
                    base = 3;
                    break;
                case 0xA5:
                    base = 0;
                    break;
                case 0xA7:
                    base = 5;
                    break;
                case 0xA8:
                    base = 1;
                    break;
                default:
                    base = 0;
                    break;
                }
                base += ofs;
                Vec_Copy(&em->posX, &pts[base * 3]);
            }
            Vec_Copy(&em->prevPosX, &em->posX);
            Vec_Copy(em->goal, &em->posX);
            EmGravios_ChangeState(em, 0, 0x26, 1);
        }
        break;
    }
}

void EmGravios_State0_25(Em *em, EmWork *work)
{
    em->hideShadowHold = 2;
    em->hideShadow = 2;
    em->blind = 2;
    em->noReact = 1;
    em->senseDelay = 5;
    em->guardTimer = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x79, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGravios_ChangeState(em, 0, 0x1A, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmGravios_ChangeState(em, 0, 0x1A, 4);
        }
        break;
    }
}

void EmGravios_State0_27(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x36, 0, 0);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGravios_State1_00(Em *em, s32 unused, s32 variant)
{
    s32 k;

    k = variant & 0xFF;
    if (k == 1) {
        em->noReact = 1;
        em->senseDelay = 5;
        em->noHitTimer = 5;
        em->noPushTimer = 5;
    }
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        if (k == 2) {
            em->speedBonus = 0x96;
        }
        Em_SetMotion(em, 2, 0, 0);
        break;
    case 1:
        if (em->hasGoal != 0) {
            if (Em_StepToGoal(em, 0x80, 1) == 1) {
                em->step++;
                EmGravios_EndState(em, 0, 0);
                break;
            }
        } else {
            if (--em->timer <= 0) {
                em->step++;
                EmGravios_EndState(em, 0, 0);
            }
        }
        break;
    }
}

void EmGravios_State1_01(Em *em)
{
    EmGravios_EndState(em, 0, 0);
}

void EmGravios_State1_02(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x64, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGravios_State1_03(Em *em, s32 unused, s32 variant)
{
    if ((variant & 0xFF) == 1) {
        em->noReact = 1;
        em->senseDelay = 5;
        em->noHitTimer = 5;
        em->noPushTimer = 5;
    }
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_StartTurnToTarget(em, &EmGravios_TurnParams[0]);
        break;
    case 1:
        if (Em_TurnStep(em, &EmGravios_TurnParams[0]) == 1) {
            em->step++;
            Player_ClearFlag(em, 0x20000);
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGravios_State1_04(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x11, 0, 0);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 150.0f, 0U) != 0) {
            em->step++;
        }
        break;
    case 2:
        if (em->hasGoal != 0) {
            if (Em_StepToGoal(em, 0x40, 1) == 1) {
                em->step++;
                Em_SetMotion(em, 0x64, 0, 0);
                break;
            }
        } else {
            if (--em->timer <= 0) {
                em->step++;
                Em_SetMotion(em, 0x64, 0, 0);
                break;
            }
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGravios_State1_05(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 2, 0, 0);
        break;
    case 1:
        if (em->hasGoal != 0) {
            if (Em_StepToGoal(em, 0x40, 1) == 1) {
                em->step++;
                EmGravios_EndState(em, 0, 0);
                break;
            }
        } else {
            if (--em->timer <= 0) {
                em->step++;
                EmGravios_EndState(em, 0, 0);
            }
        }
        break;
    }
}

void EmGravios_State1_06(Em *em)
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
                EmGravios_EndState(em, 0, 0);
                break;
            }
        } else {
            if (--em->timer <= 0) {
                em->step++;
                EmGravios_EndState(em, 0, 0);
            }
        }
        break;
    }
}

void EmGravios_State1_07(Em *em, EmWork *work)
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
        work->goalYaw = Math_AngleXZ(&em->posX, em->goal);
        break;
    case 1:
        a = em->rotY;
        t = work->goalYaw;
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
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGravios_State1_0A(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x78, 0, 0);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGravios_State1_0B(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x79, 0, 0);
        
    case 1:
        break;
    }
}

void EmGravios_State2_00(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x12, 0, 0);
        EmGravios_StartCurves(em, 2U);
        work->flyGuard = 0;
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 96.0f, 0U) != 0) {
            em->step++;
            em->stance = 2;
            EmGravios_UpdateCurves(em);
        }
        break;
    case 2:
        if (EmGravios_UpdateCurves(em)) {
            em->step++;
            EmGravios_EndState2(em, 0);
        }
        break;
    }
}

void EmGravios_State2_01(Em *em, EmWork *work)
{
    f32 velY;
    f32 floorY;

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
        work->flyGuard = 0;
        break;
    case 1:
        work->moveAngX = 0;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
        if (!((630.0f + em->floorY) <= em->posY) && ((Player_PassedFrameScaled(em, 10.0f, 0U) != 0) || (Player_PassedFrameScaled(em, 86.0f, 0U) != 0) || (Player_PassedFrameScaled(em, 160.0f, 0U) != 0))) {
            em->step++;
            Em_SetMotion(em, 0xB, 0, 0);
            velY = (em->floorY - em->posY) / 30.0f;
            em->velY = velY;
            if (!(velY < 0.0f)) {
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
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmGravios_State2_02(Em *em, EmWork *work)
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
        EmGravios_MoveByVelYCurve(em, 1U);
        if (--em->timer <= 0) {
            em->step++;
            EmGravios_ChangeState(em, 2, 1, 1);
        }
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 1);
        if (em->timer == 0x12C) {
            EmGravios_EndState2(em, 0);
        }
        break;
    case 2:
        EmGravios_MoveByVelYCurve(em, 1U);
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 1);
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmGravios_State2_03(Em *em, EmWork *work)
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
        
    case 1:
        if (Player_PassedFrameScaled(em, 96.0f, 0U) != 0) {
            em->step++;
            Player_ClearVelocity(em);
            EmGravios_StartCurves(em, 2U);
            work->flyGuard = 0;
        }
        break;
    case 2:
        if ((EmMotion_ReachedFrame(em, 0, 50.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 114.0f) == 0)) {
            Em_TurnToGoal(em, (s32) ((EmFlyerWork *)work)->turnSpeed, 1);
        }
        if (EmMotion_ReachedFrame(em, 0, 30.0f) != 0) {
            em->stance = 2;
            EmGravios_UpdateCurves(em);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
        }
        break;
    case 3:
        if ((EmGravios_UpdateCurves(em) != 0) && (em->posY <= (800.0f + em->floorY))) {
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
    case 4:
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
    case 5:
        if (em->motion[0].playing == 0) {
            work->fxMotion = 0x96;
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmGravios_State2_04(Em *em, EmWork *work)
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
        EmGravios_StartCurves(em, 3U);
        em->velZ = 20.0f;
        work->flyGuard = 0;
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 30.0f) != 0) {
            em->stance = 2;
            ended = EmGravios_UpdateCurves(em) & 0xFF;
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
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmGravios_State2_05(Em *em, EmWork *work)
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
        EmGravios_StartCurves(em, 3U);
        em->velZ = -20.0f;
        work->flyGuard = 0;
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 30.0f) != 0) {
            em->stance = 2;
            ended = EmGravios_UpdateCurves(em) & 0xFF;
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
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmGravios_State2_07(Em *em, EmWork *work)
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
        EmGravios_MoveByVelYCurve(em, 1U);
        dist = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer > 0) {
            if (dist <= (10.0f * em->velZ)) {
                goto block_9;
            }
        } else {
block_9:
            em->step++;
            Player_ClearVelocity(em);
            EmGravios_EndState2(em, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmGravios_State2_0E(Em *em, EmWork *work)
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
            EmGravios_EndState2(em, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmGravios_State2_10(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        Em_SetMotion(em, 0xF, 0, 0);
        work->turnSpeed = 0x100;
        work->goalYaw = Math_AngleXZ(&em->posX, em->goal);
        work->goalYaw = work->goalYaw - em->rotY;
        work->flyGuard = 0;
        em->timer = 0x96;
        break;
    case 1:
        EmGravios_MoveByVelYCurve(em, 1U);
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 1);
        if (--em->timer <= 0) {
            em->step++;
            EmGravios_EndState2(em, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmGravios_State2_14(Em *em, EmWork *work)
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
            EmGravios_EndState2(em, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmGravios_State2_15(Em *em, EmWork *work)
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
            EmGravios_EndState2(em, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmGravios_State2_17(Em *em, EmWork *work)
{
    f32 floorY;
    f32 velY;
    f32 y;

    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x12, 0xA, 0x60);
        EmGravios_StartCurves(em, 2U);
        em->stance = 2;
        em->holdTimer = 0;
        em->unk8AD = 0;
        em->noReact = 1;
        break;
    case 1:
        EmGravios_UpdateCurves(em);
        if (em->motion[0].playing == 0) {
            em->step++;
        }
        break;
    case 2:
        if ((EmGravios_UpdateCurves(em) & 0xFF) && (em->posY <= (800.0f + em->floorY))) {
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
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
    y = em->floorY;
    if (em->posY < y) {
        em->posY = y;
    }
}

void EmGravios_State3_00(Em *em)
{
    switch (em->step) {
    case 0:
        if (Em_TurnToTargetMotion(em, 0xE38, 2, 6, 5) != 0) {
            em->step++;
            em->stance = 0;
            em->lookAtTarget = 0;
            Em_SetMotion(em, 0x24, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGravios_State3_01(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        break;
    case 1:
        if (Em_TurnToTargetMotion(em, 0xE38, 2, 6, 5) != 0) {
            em->step++;
            em->lookAtTarget = 0;
            Em_SetMotion(em, 0x69, 0, 0);
        }
        break;
    case 2:
        if (Player_PassedFrameScaled(em, 122.0f, 0U) != 0) {
            EffEm08_CreateAtBone2(em, 0x22, 4, 0, 0, 0, 0x14);
        }
        if (em->motion[0].playing == 0) {
            em->lookAtTarget = 0;
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGravios_State3_02(Em *em, EmWork *work)
{
    f32 velY;
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x77, 0, 0);
        Player_ClearVelocity(em);
        
    case 1:
        if (Player_PassedFrameScaled(em, 70.0f, 0U) != 0) {
            em->step++;
            em->velY = 73.0f;
            em->accelY = -3.38f;
            work->flyGuard = 0;
            em->stance = 2;
            work->moveAngX = 0;
            work->moveAngY = em->rotY;
            work->moveAngZ = 0;
        }
        break;
    case 2:
        Em_MoveRotatedAccel(em, &work->moveAngX);
        velY = em->velY;
        if (velY <= 0.0f) {
            if (velY < -100.0f) {
                em->velY = -100.0f;
            }
            if (em->posY <= (em->floorY + (100.0f * em->scaleY))) {
                Em_SetMotion(em, 0x7A, 0, 0);
                em->stance = 0;
                em->posY = em->floorY;
                em->step++;
                Player_ClearVelocity(em);
            }
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmGravios_State3_03(Em *em)
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
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGravios_State3_04(Em *em, EmWork *work, s32 variant)
{
    f32 frame;
    s32 off;
    u16 temp_v0;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        break;
    case 1:
        if (Em_TurnToTargetMotion(em, 0xE38, 2, 6, 5) != 0) {
            if (em->type == 0x16) {
                em->step = 3;
            } else {
                em->step++;
            }
            em->lookAtTarget = 0;
            Em_SetMotion(em, 0x6E, 0, 0);
        }
        break;
    case 2:
        if (Player_PassedFrameScaled(em, 136.0f, 0U) != 0) {
            EffEm08_CreateAtBone2(em, 0x22, 4, 1, 0, 0, 0x23);
        }
        if (em->motion[0].playing == 0) {
            em->lookAtTarget = 0;
            em->step++;
            Em_SetMotion(em, 0x72, 0, 0);
        }
        break;
    case 3:
        if (em->type == 0x16) {
            off = variant * 4;
            temp_v0 = *(u16 *) (&D_008C4B30 + off);
            frame = (f32) (u32) temp_v0;
            if (Player_PassedFrameScaled(em, frame, 0U) != 0) {
                EffEm08_CreateAtBone(em, 0x22, 0xD, 0, (s32) *(u16 *) (&D_008C4B32 + off), 0);
            }
        }
        if (em->motion[0].playing == 0) {
            em->lookAtTarget = 0;
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGravios_State3_05(Em *em, EmWork *work, s16 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        em->timer = 120;
        Em_SetMotion(em, 0x6F, 0, 0);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 196.0f, 0)) {
            switch (variant) {
            default:
                EffGravios19_Create(em, 0xE);
                break;
            case 1:
                EffGravios19_Create(em, 0xC);
                break;
            case 2:
                EffGravios19_Create(em, 0x1F);
                break;
            }
        }
        if (EmMotion_ReachedFrame(em, 0, 124.0f) && gScene.frame % 24 == 0) {
            switch (variant) {
            default:
                StageObj_K14_Create(em, 0x1D, 0, em->scaleX);
                break;
            case 1:
                StageObj_K14_Create(em, 0x1D, 1, em->scaleX);
                break;
            case 2:
                StageObj_K14_Create(em, 0x1D, 2, 1.1f * em->scaleX);
                break;
            }
        }
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x71, 0, 0);
        }
        break;
    case 2:
        if (!EmMotion_ReachedFrame(em, 0, 50.0f) && gScene.frame % 24 == 0) {
            switch (variant) {
            case 0:
                StageObj_K14_Create(em, 0x1D, 0, em->scaleX);
                break;
            case 1:
                StageObj_K14_Create(em, 0x1D, 1, em->scaleX);
                break;
            case 2:
                StageObj_K14_Create(em, 0x1D, 2, 1.1f * em->scaleX);
                break;
            }
        }
        if (em->motion[0].playing == 0) {
            em->lookAtTarget = 0;
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGravios_State3_06(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        if (em->type == 0x16) {
            Em_SetMotion(em, 0x2A, 0, 0);
        } else {
            Em_SetMotion(em, 0x2B, 0, 0);
        }
        Motion_InitTime(em, 0);
        break;
    case 1:
        if (em->type == 0x16) {
            em->rotY += 0x200;
        } else {
            em->rotY -= 0x200;
        }
        if (em->motion[1].playing == 0) {
            em->step++;
            if (em->type == 0x16) {
                Em_SetMotion(em, 0x2A, 0, 0);
                break;
            }
            Em_SetMotion(em, 0x2B, 0, 0);
        }
        break;
    case 2:
        if (em->type == 0x16) {
            em->rotY += 0x200;
        } else {
            em->rotY -= 0x200;
        }
        if (em->motion[1].playing == 0) {
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGravios_State3_08(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        
    case 1:
        if (Em_TurnToTargetMotion(em, 0xE38, 2, 6, 5) != 0) {
            em->step++;
            em->lookAtTarget = 0;
            Em_SetMotion(em, 0x6D, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->lookAtTarget = 0;
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGravios_State3_09(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x70, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->lookAtTarget = 0;
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGravios_State3_0C(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x75, 0, 0);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 118.0f, 0U) != 0) {
            EffEm08_CreateAtBone2(em, 0x22, 4, 0, 0, 0, 0x28);
        }
        if (em->motion[0].playing == 0) {
            em->lookAtTarget = 0;
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGravios_State4_00(Em *em)
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
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGravios_State4_01(Em *em)
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
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGravios_State4_02(Em *em)
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
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGravios_State4_03(Em *em)
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
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGravios_State4_04(Em *em)
{
    f32 turnF;
    u32 turn;
    u8 type;

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
            type = em->type;
            if ((type != 0x14) && (type != 0x27)) {
                em->meshShown[1] = 0;
            }
        }
        if ((EmMotion_ReachedFrame(em, 0, 52.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 122.0f) == 0)) {
            turnF = 936.0f * em->motionSpeed;
            turn = (u32) turnF;
            em->rotY -= (s32) turn & 0xFFFF;
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            em->rotY -= 8;
            EmGravios_ChangeState(em, 4, 0x11, 3);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmGravios_ChangeState(em, 4, 0x11, 3);
        }
        break;
    }
}

void EmGravios_State4_05(Em *em)
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
            EmGravios_EndState(em, 0, 0);
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
        }
        break;
    }
}

void EmGravios_State4_06(Em *em)
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
            EmGravios_EndState(em, 0, 0);
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
        }
        break;
    }
}

void EmGravios_State4_07(Em *em, EmWork *work)
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
        if (Player_PassedFrameScaled(em, 14.0f, 0) != 0) {
            em->step++;
            EmGravios_EndState2(em, 1);
            em->velY = 0.0f;
        }
        Em_MoveRotated(em, &work->moveAngX);
        break;
    }
}

void EmGravios_State4_08(Em *em, EmWork *work)
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
            Em_SetMotion(em, 0x76, 6, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGravios_EndState(em, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    }
}

void EmGravios_State4_09(Em *em, EmWork *work)
{
    em->blind = 2;
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
            Em_SetMotion(em, 0x76, 6, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGravios_EndState(em, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    }
}

void EmGravios_State4_0A(Em *em)
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
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGravios_State4_0B(Em *em)
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
            EmGravios_ChangeState(em, 0, 0x21, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmGravios_ChangeState(em, 0, 0x21, 4);
        }
        break;
    }
    Em_SpawnEffect06Bone(em, 2);
}

void EmGravios_State4_0C(Em *em)
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
            EmGravios_ChangeState(em, 4, 0xD, 3);
        }
        break;
    case 2:
        if ((s8) em->holdTimer < 5) {
            em->holdTimer = 5;
        }
        if (em->remote == 0) {
            EmGravios_ChangeState(em, 4, 0xD, 3);
        }
        break;
    }
}

void EmGravios_State4_0D(Em *em)
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
            EmGravios_ChangeState(em, 2, 0x17, 4);
        }
        break;
    case 2:
        if ((em->motion[0].playing == 0) && (em->remote == 0)) {
            Em_SetMotion(em, 0x60, 0, 0);
            EmGravios_ChangeState(em, 2, 0x17, 4);
        }
        break;
    }
}

void EmGravios_State4_0E(Em *em, s32 unused, s32 variant)
{
    s32 k;

    k = variant & 0xFF;
    if (k == 0) {
        Em_SetFlag6AE(em, 4U);
        Em_SpawnEffect06Bone(em, 2);
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
            if (k == 0) {
                EmGravios_ChangeState(em, 4, 0x12, 3);
                break;
            }
            EmGravios_ChangeState(em, 4, 0x17, 3);
        }
        break;
    case 2:
        if (em->remote == 0) {
            if (k == 0) {
                EmGravios_ChangeState(em, 4, 0x12, 3);
                break;
            }
            EmGravios_ChangeState(em, 4, 0x17, 3);
        }
        break;
    }
}

void EmGravios_State4_0F(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x73, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGravios_State4_10(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x74, 0, 0);
        StageObj_Stun_CreateEm(em, 1, 0x22, (s32) ((f32) em->timer + (em->motion[0].frameCount / 2.0f)), 150.0f, 3.0f);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x76, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGravios_ChangeState(em, 0, 0x23, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmGravios_ChangeState(em, 0, 0x23, 4);
        }
        break;
    }
}

void EmGravios_State4_11(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGravios_State4_13(Em *em)
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
            EmGravios_ChangeState(em, 4, 0xD, 3);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmGravios_ChangeState(em, 4, 0xD, 3);
        }
        break;
    }
}

void EmGravios_State4_15(Em *em)
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
            EmGravios_ChangeState(em, 0, 0x22, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmGravios_ChangeState(em, 0, 0x22, 4);
        }
        break;
    }
}

void EmGravios_State6_00_Entrance(Em *em, EmWork *work)
{
    f32 pos[3];
    f32 d;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        GmEm_ResetSize(em);
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 0;
        em->posX = 9840.0f;
        em->posY = 2.0f;
        em->posZ = 13000.0f;
        em->goal[0] = 10740.0f;
        em->goal[1] = 2.0f;
        em->goal[2] = 14275.0f;
        em->rotX = 0;
        em->rotY = Math_AngleXZ(&em->posX, em->goal);
        em->rotZ = 0;
        Em_SetMotion(em, 1, 0, 0);
        em->timer = 300;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            em->timer = 300;
            Em_SetLayerMotion(em, 0x3EA, 0, 0, 0);
            Em_SetLayerMotion(em, 0x4C9, 0, 0, 1);
            Em_SetLayerMotion(em, 0x57A, 0, 0, 2);
        }
        break;
    case 2:
        if (em->motion[1].playing == 0) {
            Em_SetLayerMotion(em, 0x4C9, 0, 0, 1);
        }
        d = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer <= 0 || d <= 200.0f) {
            em->step++;
            Em_SetMotion(em, 0x68, 0, 0);
        }
        break;
    case 3:
        if ((Player_PassedFrameScaled(em, 68.0f, 0) || Player_PassedFrameScaled(em, 94.0f, 0) || Player_PassedFrameScaled(em, 204.0f, 0)) && ((s32)Player_InCurrentArea(em) & 0xFF)) {
            StageObj_K15_Create(em, 1);
            Em_BonePos(em, 0x24, pos);
            Effect02_SpawnForOwner(em, 0, 9, 0, pos, 1.0f);
            Effect0D_SpawnAt(pos, 0x14, 4.0f);
        }
        if (Player_PassedFrameScaled(em, 250.0f, 0)) {
            em->step++;
            Em_SetMotion(em, 0x68, 0, 0x50);
        }
        break;
    case 4:
        if ((Player_PassedFrameScaled(em, 68.0f, 0) || Player_PassedFrameScaled(em, 94.0f, 0) || Player_PassedFrameScaled(em, 204.0f, 0)) && ((s32)Player_InCurrentArea(em) & 0xFF)) {
            StageObj_K15_Create(em, 1);
            Em_BonePos(em, 0x24, pos);
            Effect02_SpawnForOwner(em, 0, 9, 0, pos, 1.0f);
            Effect0D_SpawnAt(pos, 0x14, 4.0f);
        }
        if (Player_PassedFrameScaled(em, 400.0f, 0)) {
            em->step++;
            Em_SetLayerMotion(em, 0x3E9, 0, 0, 0);
            Em_SetLayerMotion(em, 0x4C9, 0, 0, 1);
            Em_SetLayerMotion(em, 0x579, 0, 0, 2);
        }
        break;
    case 5:
        if (em->motion[1].playing == 0) {
            em->step++;
            em->rotY -= 0x4000;
            Em_SetMotion(em, 5, 0, 0);
        }
        break;
    case 6:
        em->rotY -= (u32) ((16384.0f / (em->motion[0].frameCount / 2.0f)) * em->motionSpeed);
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x6E, 0, 0);
            em->timer = 4;
        }
        break;
    case 7:
        if (Player_PassedFrameScaled(em, 136.0f, 0)) {
            em->step++;
            EffEm08_CreateAtBone2(em, 0x22, 4, 1, 0, 0, 0x37);
        }
    case 8:
        if (em->motion[0].playing == 0) {
            if (--em->timer <= 0) {
                em->lookAtTarget = 0;
                em->step++;
                Em_SetMotion(em, 0x72, 0, 0);
            }
        }
        break;
    case 9:
        if (em->motion[1].playing == 0) {
            em->step++;
            Em_SetMotion(em, 1, 0, 0);
        }
        break;
    case 10:
        if (Save_TestEventFlag(0xA9) == 1) {
            em->senseDelay = 0;
            em->noHitTimer = 0;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGravios_State6_04_Captured(Em *em, EmWork *work)
{
    s32 timer;
    s32 t;

    Em_SetCaptured(em, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x61, 0, 0);
        Em_LoadReactionSet(em, 1);
        Em_ClearPoison(em);
        em->timer = 0x708;
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
                em->timer = 0x96;
            }
        }
        break;
    case 3:
        timer = em->timer;
        if (timer > 0) {
            em->timer = timer - 1;
        }
        t = em->timer;
        if (t <= 0) {
            if (em->remote == 0) {
                EmGravios_ChangeState(em, 5, 4, 4);
            }
        } else {
            em->alpha = (f32) t / 150.0f;
        }
        break;
    }
    EmGravios_SpawnPeriodicEffect06(em);
}

void EmGravios_State6_05_Captured(Em *em, EmWork *work)
{
    s32 timer;
    s32 t;

    Em_SetCaptured(em, 1);
    switch (em->step) {
    case 0:
        em->step++;
        Em_BeginStatus1(em);
        Em_ClearPoison(em);
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x26, 0, 0);
        Em_LoadReactionSet(em, 1);
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
                EmGravios_ChangeState(em, 5, 4, 4);
                break;
            }
        } else {
            em->alpha = (f32) timer / 150.0f;
        }
        break;
    }
}

void EmGravios_State8_00(Em *em, EmWork *work)
{
    EmGraviosVec v = EmGravios_CarveThrowPt;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        if (Versus_DefeatCamera(em) == 1) {
            em->step++;
            em->stateVar0 = 5;
            em->stateVar1 = 0;
            Em_SetMotion(em, 1, 0, 0);
            em->timer = 30;
        }
        break;
    case 1:
        if (em->stateVar0 == 0 && em->stateVar1 == 0) {
            em->stateVar1 = 1;
            StageObj_K25_Create(&gPlayers[(u8) em->side - 1], em, 0, 0, &v.x);
        } else {
            em->stateVar0--;
        }
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x68, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 30;
            Em_SetMotion(em, 0x6A, 0, 0);
        }
        break;
    case 3:
        if (--em->timer <= 0) {
            Versus_RouteEmByTeam(em);
        }
        break;
    }
}

void EmGravios_State8_01(Em *em, EmWork *work)
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
            EmGravios_ChangeState(em, 8, 0x98, 1);
        }
        break;
    }
}

void EmGravios_State8_02(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x6A, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 1, 0, 0);
            EmGravios_ChangeState(em, 8, 0x98, 1);
        }
        break;
    }
}

void EmGravios_State8_03(Em *em, EmWork *work)
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
            Em_SetMotion(em, 0x72, 0, 0);
            EmGravios_ChangeState(em, 8, 0x98, 1);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 1, 0, 0);
            EmGravios_ChangeState(em, 8, 0x98, 1);
        }
        break;
    }
}

void EmGravios_State8_98(Em *em, EmWork *work)
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
            EmGravios_ChangeState(em, 8, 0x99, 1);
        }
        break;
    }
}

void EmGravios_State8_99(Em *em, EmWork *work)
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
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGravios_State5_00_Hunted(Em *em, EmWork *work)
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
                EmGravios_ChangeState(em, 5, 4, 4);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    }
}

void EmGravios_State5_01_Hunted(Em *em, EmWork *work)
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
        em->timer = 3600;
        em->holdTimer = 5;
        break;
    case 1:
        em->holdTimer = 5;
        em->timer--;
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
                EmGravios_ChangeState(em, 5, 4, 4);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    }
}

void EmGravios_State5_02_HuntedFalling(Em *em, EmWork *work)
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
        if (Player_PassedFrameScaled(em, 60.0f, 0) != 0) {
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
                EmGravios_ChangeState(em, 5, 4, 4);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    }
}

void EmGravios_State5_03_Hunted(Em *em, EmWork *work)
{
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 3;
        em->lookAtTarget = 0;
        if (em->motionId[0] != 0x6C) {
            Em_SetMotion(em, 0x6C, 0, 0);
        }
        QuestEm_Hunted(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x44, 0, 0);
            em->lookAtTarget = 0;
        }
        break;
    case 2:
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
    case 3:
        EmMine_Update(em);
        if (QuestEm_CanAppear(em) != 0) {
            if (--em->timer <= 0) {
                em->step++;
                EmMine_Remove(em);
                em->timer = 150;
            }
        }
        break;
    case 4:
        if (em->timer > 0) {
            em->timer--;
        }
        if (em->timer <= 0) {
            if (em->remote == 0) {
                EmGravios_ChangeState(em, 5, 4, 4);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    }
}

void EmGravios_State5_04(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        if (QuestEm_CanAppear(em) == 1) {
            switch (Area_BaseId(gScene.map)) {
            case 9:
                em->area = Area_DayId(0x5A);
                switch (em->variant) {
                default:
                    em->posX = 15949.7f;
                    em->posY = 1010.0f;
                    em->posZ = 7225.8f;
                    em->rotY = 0xDA3D;
                    break;
                case 1:
                    em->posX = 11948.9f;
                    em->posY = 1010.0f;
                    em->posZ = 15923.8f;
                    em->rotY = 0x8A76;
                    break;
                }
                EmGravios_ChangeState(em, 0, 1, 0);
                break;
            default:
                em->posY = gAreaGroundY[em->area];
                em->stance = 2;
                EmGravios_ChangeState(em, 2, 1, 0);
                break;
            }
        } else {
            em->taskPhase++;
        }
        break;
    }
}

void EmGravios_StartDown(Em *em)
{
    if ((s32) em->parts[6].state > 1) {
        EmGravios_ChangeState(em, 4, 0, 3);
        return;
    }
    EmGravios_ChangeState(em, 4, 0xF, 3);
}

void EmGravios_RunGroup0(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmGravios_State0_00(em, work);
        break;
    case 1:
        EmGravios_State0_01(em, work);
        break;
    case 2:
        EmGravios_State0_02(em, work);
        break;
    case 3:
        EmGravios_State0_03(em, work);
        break;
    case 4:
        EmGravios_State0_04(em, work);
        break;
    case 5:
        EmGravios_State0_05(em, work);
        break;
    case 6:
        EmGravios_State0_06(em, work);
        break;
    case 7:
        EmGravios_State0_07(em, work);
        break;
    case 8:
        EmGravios_State0_08(em, work);
        break;
    case 9:
        EmGravios_State0_09(em, work);
        break;
    case 10:
        EmGravios_State0_0A(em, work);
        break;
    case 11:
        EmGravios_State0_0B(em, work);
        break;
    case 12:
        EmGravios_State0_0C(em, work);
        break;
    case 13:
        EmGravios_State0_0D(em, work);
        break;
    case 14:
        EmGravios_State0_0E(em, work);
        break;
    case 15:
        EmGravios_State0_0F(em, work);
        break;
    case 16:
        EmGravios_State0_10(em, work);
        break;
    case 17:
        EmGravios_State0_11(em, work);
        break;
    case 18:
        EmGravios_State0_12(em, work);
        break;
    case 19:
        EmGravios_State0_13(em, work);
        break;
    case 20:
        EmGravios_State0_14(em, work);
        break;
    case 22:
        EmGravios_State0_16(em, work);
        break;
    case 23:
        EmGravios_State0_17(em, work);
        break;
    case 24:
        EmGravios_State0_17(em, work);
        break;
    case 25:
        EmGravios_State0_19(em, work);
        break;
    case 26:
        EmGravios_State0_1A(em, work);
        break;
    case 27:
        EmGravios_State0_1B(em, work);
        break;
    case 28:
        EmGravios_State0_17(em, work);
        break;
    case 29:
        EmGravios_State0_1D(em, work);
        break;
    case 31:
        EmGravios_State0_1F(em, work);
        break;
    case 33:
        EmGravios_State0_19(em, work);
        break;
    case 34:
        EmGravios_State0_19(em, work);
        break;
    case 35:
        EmGravios_State0_19(em, work);
        break;
    case 36:
        EmGravios_State0_24(em, work);
        break;
    case 37:
        EmGravios_State0_25(em, work);
        break;
    case 38:
        EmGravios_State0_25(em, work);
        break;
    case 39:
        EmGravios_State0_27(em, work);
        break;
    case 40:
        EmGravios_State0_27(em, work);
        
    default:
        break;
    }
}

void EmGravios_RunGroup1(Em *em, s32 work)
{
    switch (em->state) {
    case 0:
        EmGravios_State1_00(em, work, 0);
        break;
    case 1:
        EmGravios_State1_01(em);
        break;
    case 2:
        EmGravios_State1_02(em);
        break;
    case 3:
        EmGravios_State1_03(em, work, 0);
        break;
    case 4:
        EmGravios_State1_04(em);
        break;
    case 5:
        EmGravios_State1_05(em);
        break;
    case 6:
        EmGravios_State1_06(em);
        break;
    case 7:
        EmGravios_State1_07(em, (EmWork *) work);
        break;
    case 8:
        EmGravios_State1_04(em);
        break;
    case 9:
        EmGravios_State1_07(em, (EmWork *) work);
        break;
    case 10:
        EmGravios_State1_0A(em);
        break;
    case 11:
        EmGravios_State1_0B(em);
        break;
    case 12:
        EmGravios_State1_03(em, work, 1);
        break;
    case 13:
        EmGravios_State1_00(em, work, 1);
        break;
    case 14:
        EmGravios_State1_00(em, work, 2);
        
    default:
        break;
    }
}

void EmGravios_RunGroup2(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmGravios_State2_00(em, work);
        break;
    case 1:
        EmGravios_State2_01(em, work);
        break;
    case 2:
        EmGravios_State2_02(em, work);
        break;
    case 3:
        EmGravios_State2_03(em, work);
        break;
    case 4:
        EmGravios_State2_04(em, work);
        break;
    case 5:
        EmGravios_State2_05(em, work);
        break;
    case 7:
        EmGravios_State2_07(em, work);
        break;
    case 14:
        EmGravios_State2_0E(em, work);
        break;
    case 16:
        EmGravios_State2_10(em, work);
        break;
    case 20:
        EmGravios_State2_14(em, work);
        break;
    case 21:
        EmGravios_State2_15(em, work);
        break;
    case 23:
        EmGravios_State2_17(em, work);
        break;
    }
}

void EmGravios_RunGroup3(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmGravios_State3_00(em);
        break;
    case 1:
        EmGravios_State3_01(em);
        break;
    case 2:
        EmGravios_State3_02(em, work);
        break;
    case 3:
        EmGravios_State3_03(em);
        break;
    case 4:
        EmGravios_State3_04(em, work, 0);
        break;
    case 5:
        EmGravios_State3_05(em, work, 0);
        break;
    case 6:
        EmGravios_State3_06(em);
        break;
    case 7:
        EmGravios_State3_05(em, work, 1);
        break;
    case 8:
        EmGravios_State3_08(em);
        break;
    case 9:
        EmGravios_State3_09(em);
        break;
    case 10:
        EmGravios_State3_05(em, work, 2);
        break;
    case 11:
        EmGravios_State3_04(em, work, 1);
        break;
    case 12:
        EmGravios_State3_0C(em);
        break;
    }
}

void EmGravios_RunGroup4(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmGravios_State4_00(em);
        break;
    case 1:
        EmGravios_State4_01(em);
        break;
    case 2:
        EmGravios_State4_02(em);
        break;
    case 3:
        EmGravios_State4_03(em);
        break;
    case 4:
        EmGravios_State4_04(em);
        break;
    case 5:
        EmGravios_State4_05(em);
        break;
    case 6:
        EmGravios_State4_06(em);
        break;
    case 7:
        EmGravios_State4_07(em, work);
        break;
    case 8:
        EmGravios_State4_08(em, work);
        break;
    case 9:
        EmGravios_State4_09(em, work);
        break;
    case 10:
        EmGravios_State4_0A(em);
        break;
    case 11:
        EmGravios_State4_0B(em);
        break;
    case 12:
        EmGravios_State4_0C(em);
        break;
    case 13:
        EmGravios_State4_0D(em);
        break;
    case 14:
        EmGravios_State4_0E(em, (s32) work, 0);
        break;
    case 15:
        EmGravios_State4_0F(em);
        break;
    case 16:
        EmGravios_State4_10(em);
        break;
    case 17:
        EmGravios_State4_11(em);
        break;
    case 18:
        EmGravios_State4_0D(em);
        break;
    case 19:
        EmGravios_State4_13(em);
        break;
    case 20:
        EmGravios_State4_13(em);
        break;
    case 21:
        EmGravios_State4_15(em);
        break;
    case 22:
        EmGravios_State4_0E(em, (s32) work, 1);
        break;
    case 23:
        EmGravios_State4_0D(em);
        break;
    }
}

void EmGravios_RunGroup5(Em *em, EmWork *work)
{
    em->noPushTimer = 5;
    switch (em->state) {
    case 0:
        EmGravios_State5_00_Hunted(em, work);
        break;
    case 1:
        EmGravios_State5_01_Hunted(em, work);
        break;
    case 2:
        EmGravios_State5_02_HuntedFalling(em, work);
        break;
    case 3:
        EmGravios_State5_03_Hunted(em, work);
        break;
    case 4:
        EmGravios_State5_04(em, work);
        break;
    }
}

void EmGravios_RunGroup6(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmGravios_State6_00_Entrance(em, work);
        break;
    case 1:
        EmGravios_State6_00_Entrance(em, work);
        break;
    case 2:
        EmGravios_State6_00_Entrance(em, work);
        break;
    case 3:
        EmGravios_State6_00_Entrance(em, work);
        break;
    case 4:
        EmGravios_State6_04_Captured(em, work);
        break;
    case 5:
        EmGravios_State6_05_Captured(em, work);
        
    default:
        break;
    }
}

void EmGravios_RunGroup8(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0x0:
        EmGravios_State8_00(em, work);
        break;
    case 0x1:
        EmGravios_State8_01(em, work);
        break;
    case 0x2:
        EmGravios_State8_02(em, work);
        break;
    case 0x3:
        EmGravios_State8_03(em, work);
        break;
    case 0x98:
        EmGravios_State8_98(em, work);
        break;
    case 0x99:
        EmGravios_State8_99(em, work);
        break;
    }
}

void EmGravios_Update(Em *em)
{
    u8 buf[4];
    EmFlyerWork *w = (EmFlyerWork *)&em->work;

    Em_UpdateCombatTimer(em);
    Em_RaiseTrigger80Lax(em);
    Em_HoldPosNoPlayers(em, 0, 1);
    if (w->fxMotion != 0) {
        w->fxMotion -= 1;
    }
    switch (Em_UpdateDamage(em, buf) & 0xFF) {
    case 0:
    case 9:
    case 13:
    case 14:
    case 15:
    case 16:
        break;
    case 1:
    case 2:
        if (em->stance == 2) {
            EmGravios_ChangeState(em, 5, 2, 3);
        } else if ((s8) em->holdTimer != 0) {
            EmGravios_ChangeState(em, 5, 1, 3);
        } else if (func_00770310(em) == 1 || (em->stateGroup == 0 && em->state == 8) || (em->stateGroup == 0 && em->state == 0xC)) {
            EmGravios_ChangeState(em, 5, 3, 3);
        } else {
            EmGravios_ChangeState(em, 5, 0, 3);
        }
        break;
    case 3:
    case 4:
        if ((s8) em->holdTimer == 0) {
            Em_SetGoal12Dist(em, buf);
            Em_SetGoal12(em);
            EmGravios_ChangeState(em, 4, 0xC, 3);
        }
        break;
    case 5:
        if (em->stateGroup != 4 || em->state != 8) {
            EmGravios_ChangeState(em, 4, 8, 3);
        }
        break;
    case 6:
        if ((s8) em->holdTimer != 0) {
            Em_SetStatus4Timer(em);
            EmGravios_ChangeState(em, 4, 0xE, 3);
        } else if ((em->stateGroup != 4 || em->state != 0xB) && (em->stateGroup != 4 || em->state != 8)) {
            Em_SetStatus4Timer(em);
            EmGravios_ChangeState(em, 4, 0xB, 3);
        }
        break;
    case 7:
        if ((s8) em->holdTimer != 0) {
            EmGravios_ChangeState(em, 6, 4, 3);
        } else if ((s8) em->holdTimer2 != 0) {
            EmGravios_ChangeState(em, 6, 5, 3);
        } else if ((em->stateGroup != 0 || em->state != 0x1B) && (em->stateGroup != 4 || em->state != 8)) {
            Em_SetStatus1Timer(em);
            EmGravios_ChangeState(em, 0, 0x1B, 3);
        }
        break;
    case 8:
        if ((s8) em->holdTimer != 0) {
            Em_SetStatus2Timer(em);
            EmGravios_ChangeState(em, 0, 0x1D, 3);
        } else if ((em->stateGroup != 0 || em->state != 0x14) && (em->stateGroup != 4 || em->state != 8)) {
            Em_SetStatus2Timer(em);
            EmGravios_ChangeState(em, 0, 0x14, 3);
        }
        break;
    case 10:
        switch (em->state) {
        case 0x12:
            EmGravios_ChangeState(em, 0, 0x17, 3);
            em->scriptPending = 0;
            Em_CheckRageByHp(em, em->rageThreshold);
            break;
        case 0x14:
            EmGravios_ChangeState(em, 0, 0x18, 3);
            em->scriptPending = 0;
            break;
        case 0x1B:
            EmGravios_ChangeState(em, 0, 0x1C, 3);
            em->scriptPending = 0;
            break;
        case 0x1D:
            EmGravios_ChangeState(em, 4, 0x13, 3);
            em->scriptPending = 0;
            break;
        case 0x1F:
            EmGravios_ChangeState(em, 4, 0x14, 3);
            em->scriptPending = 0;
            break;
        }
        break;
    case 11:
        EmGravios_ChangeState(em, 4, 4, 3);
        break;
    case 12:
        Player_ClearFlag(em, 0x20000);
        if (em->stance == 2) {
            EmGravios_ChangeState(em, 4, 8, 3);
        } else {
            switch (em->flinchPart) {
            case 6:
                EmGravios_StartDown(em);
                break;
            case 0:
            case 7:
                EmGravios_ChangeState(em, 4, 0xF, 3);
                break;
            case 5:
                EmGravios_ChangeState(em, 4, 2, 3);
                break;
            case 1:
            case 2:
                EmGravios_ChangeState(em, 4, 3, 3);
                break;
            case 3:
            case 4:
            default:
                if (em->parts[em->flinchPart].state >= 2) {
                    if (em->flinchPart != 3) {
                        EmGravios_ChangeState(em, 4, 5, 3);
                    } else {
                        EmGravios_ChangeState(em, 4, 6, 3);
                    }
                } else {
                    EmGravios_ChangeState(em, 4, 1, 3);
                }
                break;
            }
        }
        break;
    case 17:
        if ((s8) em->holdTimer != 0) {
            Em_SetStatus5Timer(em);
            EmGravios_ChangeState(em, 4, 0x16, 3);
        } else if ((em->stateGroup != 4 || em->state != 0x10) && (em->stateGroup != 4 || em->state != 8)) {
            Em_SetStatus5Timer(em);
            EmGravios_ChangeState(em, 4, 0x10, 3);
        }
        break;
    case 18:
        Em_ResetKindTimer(em);
        EmGravios_ChangeState(em, 4, 0x15, 3);
        break;
    }
    switch (gQuest.stage) {
    case 0x42C:
        if ((em->type == 0x11 || em->type == 0x2F) && Area_DayId(em->area) == 0xA7 && Save_TestEventFlag(0xA9) == 0) {
            if (Area_DayId(gScene.area) == 0xA7 && gScene.eventRunning == 1 && em->stateGroup != 6) {
                EmGravios_ChangeState(em, 6, 0, 1);
            }
            break;
        }
    default:
        Em_UpdatePendingScript(em);
        break;
    }
    Em_ClearFlag6AE(em);
    EmGravios_RunState(em, (s32) w);
    if (em->stateRequest != 0) {
        EmGravios_RunState(em, (s32) w);
        em->stateRequest = 0;
    }
    if (em->stateGroup == 2 && (s8) w->flyGuard == 1) {
        em->guardTimer = 2;
    }
}

void EmGravios_RunState(Em *em, s32 work)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->stateGroup) {
    case 0:
        EmGravios_RunGroup0(em, (EmWork *) work);
        break;
    case 1:
        EmGravios_RunGroup1(em, work);
        break;
    case 2:
        EmGravios_RunGroup2(em, (EmWork *) work);
        break;
    case 3:
        EmGravios_RunGroup3(em, (EmWork *) work);
        break;
    case 4:
        EmGravios_RunGroup4(em, (EmWork *) work);
        break;
    case 5:
        EmGravios_RunGroup5(em, (EmWork *) work);
        break;
    case 6:
        EmGravios_RunGroup6(em, (EmWork *) work);
        break;
    case 7:
        EmGravios_RunGroup6(em, (EmWork *) work);
        break;
    case 8:
        EmGravios_RunGroup8(em, (EmWork *) work);
        
    default:
        break;
    }
}

void EmGravios_RequestOnFrame(void *em, s32 frame, s32 i)
{
    if (Player_PassedFrameScaled((Em *) em, (f32) frame, 0U) != 0) {
        CamShake_ReqAIf(em, i);
    }
}

static void EmGravios_MotionEffectsDefault(Em *em)
{
}

void EmGravios_MotionEffects(Em *em, EmGraviosFxWork *work)
{
    f32 pos[3];
    f32 v1[3];

    if (em->motionId[0] != work->motion) {
        work->motion = em->motionId[0];
    }
    ((u8 *) work)[0x1B] = AreaGrid2_OnFlaggedFloor(&em->posX);
    EmGravios_MotionSounds(em, (EmWork *) work);
    switch (work->motion) {
    case 0x3E9:
        break;
    case 0x3EA:
        EmGravios_RequestOnFrame(em, 0x2C, 1);
        EmGravios_RequestOnFrame(em, 0x74, 1);
        if (Player_PassedFrameScaled(em, 2.0f, 0) != 0) {
            EffGravios19_Create(em, 9);
        }
        if (Player_PassedFrameScaled(em, 70.0f, 0) != 0) {
            EffGravios19_Create(em, 0xA);
        }
        if (Player_PassedFrameScaled(em, 148.0f, 0) != 0) {
            EffGravios19_Create(em, 9);
            break;
        }
        break;
    case 0x3ED:
        EmGravios_RequestOnFrame(em, 0x3C, 1);
        if (Player_PassedFrameScaled(em, 2.0f, 0) != 0) {
            EffGravios19_Create(em, 5);
        }
        if (Player_PassedFrameScaled(em, 30.0f, 0) != 0) {
            EffGravios19_Create(em, 6);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 2.0f, 78.0f) != 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create(em, 0x1A, 1, 1.0f);
            break;
        }
        break;
    case 0x3EE:
        EmGravios_RequestOnFrame(em, 0x3C, 1);
        if (Player_PassedFrameScaled(em, 30.0f, 0) != 0) {
            EffGravios19_Create(em, 7);
        }
        if (Player_PassedFrameScaled(em, 2.0f, 0) != 0) {
            EffGravios19_Create(em, 8);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 2.0f, 78.0f) != 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create(em, 0x1A, 0, 1.0f);
            break;
        }
        break;
    case 0x3F2:
        if (Player_PassedFrameScaled(em, 42.0f, 0) != 0) {
            EffGravios19_Create(em, 0x13);
        }
        if (Player_PassedFrameScaled(em, 2.0f, 0) != 0) {
            EffGravios19_Create(em, 0x14);
        }
        if (Player_PassedFrameScaled(em, 74.0f, 0) != 0) {
            EffGravios19_Create(em, 0x14);
            break;
        }
        break;
    case 0x3F3:
        if (Player_PassedFrameScaled(em, 52.0f, 0) || Player_PassedFrameScaled(em, 104.0f, 0) || Player_PassedFrameScaled(em, 162.0f, 0)) {
            Effect0D_SpawnForEmScaled(em, 2, 7, 5.0f);
            break;
        }
        break;
    case 0x3F5:
        if ((Player_PassedFrameScaled(em, 26.0f, 0) != 0) || (Player_PassedFrameScaled(em, 52.0f, 0) != 0)) {
            Effect0D_SpawnForEmScaled(em, 1, 7, 5.0f);
            break;
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
    case 0x3F8:
        break;
    case 0x3F9:
        if (Player_PassedFrameScaled(em, 150.0f, 0) != 0) {
            if (em->type == 0x11 || em->type == 0x2F) {
                EffGravios19_Create(em, 0xB);
            } else {
                EffGravios19_Create(em, 0x1B);
            }
        }
        if ((Player_PassedFrameScaled(em, 140.0f, 0) != 0) || (Player_PassedFrameScaled(em, 214.0f, 0) != 0)) {
            StageObj_K14_Create(em, 2, 0, 1.0f);
        }
        if ((Player_PassedFrameScaled(em, 128.0f, 0) != 0) || (Player_PassedFrameScaled(em, 182.0f, 0) != 0)) {
            StageObj_K14_Create(em, 3, 0, 1.0f);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 8.0f, 116.0f) != 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
            break;
        }
        break;
    case 0x3FA:
        if ((Player_PassedFrameScaled(em, 98.0f, 0) != 0) && (em->posY <= (1000.0f + em->floorY))) {
            StageObj_K14_Create(em, 1, 0, 1.0f);
        }
        if ((Player_PassedFrameScaled(em, 100.0f, 0) != 0) || (Player_PassedFrameScaled(em, 176.0f, 0) != 0) || (Player_PassedFrameScaled(em, 234.0f, 0) != 0) || (Player_PassedFrameScaled(em, 288.0f, 0) != 0)) {
            StageObj_K14_Create(em, 1, 0, 1.0f);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 14.0f, 86.0f) != 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
            break;
        }
        break;
    case 0x3FB:
        if (Player_PassedFrameScaled(em, 2.0f, 0) != 0) {
            EffGravios19_Create(em, 0xD);
        }
        if (Player_PassedFrameScaled(em, 16.0f, 0) != 0) {
            StageObj_K14_Create(em, 0xB, 0, 1.0f);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 18.0f, 44.0f) != 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
            break;
        }
        break;
    case 0x405:
    case 0x406:
        break;
    case 0x40A:
        v1[0] = 0.0f;
        v1[1] = 10.0f;
        v1[2] = 140.0f;
        Em_SpawnEffect06Periodic(em, 0x22, v1, 1.6f);
        break;
    case 0x40B:
        break;
    case 0x40C:
        if (Player_PassedFrameScaled(em, 50.0f, 0) != 0) {
            if (em->type == 0x11 || em->type == 0x2F) {
                EffGravios19_Create(em, 1);
            } else {
                EffGravios19_Create(em, 0x19);
            }
        }
        if (Player_PassedFrameScaled(em, 46.0f, 0) != 0) {
            EffGravios19_Create(em, 0x15);
            break;
        }
        break;
    case 0x40D:
        break;
    case 0x40E:
        if (Player_PassedFrameScaled(em, 148.0f, 0) != 0) {
            EffGravios19_Create(em, 0xF);
        }
        if (Player_PassedFrameScaled(em, 156.0f, 0) != 0) {
            StageObj_K14_Create(em, 0, 0, 1.0f);
            break;
        }
        break;
    case 0x40F:
        break;
    case 0x412:
        if (Player_PassedFrameScaled(em, 30.0f, 0) != 0) {
            EffGravios19_Create(em, 0x1C);
            break;
        }
        break;
    case 0x413:
        EmGravios_RequestOnFrame(em, 0x2C, 1);
        EmGravios_RequestOnFrame(em, 0x49, 1);
        if (Player_PassedFrameScaled(em, 30.0f, 0) != 0) {
            EffGravios19_Create(em, 0x10);
            break;
        }
        break;
    case 0x414:
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 6.0f, 38.0f) != 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create(em, 0x1A, 1, 1.0f);
            break;
        }
        break;
    case 0x417:
    case 0x418:
    case 0x419:
    case 0x41A:
    case 0x41B:
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
            break;
        }
        break;
    case 0x41E:
        if (Player_PassedFrameScaled(em, 102.0f, 0) != 0) {
            EffGravios19_Create(em, 0x11);
        }
        if (Player_PassedFrameScaled(em, 110.0f, 0) != 0) {
            EffGravios19_Create(em, 0x12);
            StageObj_K0F_CreateOnEm(em, 5, 4, 1.0f);
            break;
        }
        break;
    case 0x424:
        if (Player_PassedFrameScaled(em, 48.0f, 0) != 0) {
            Effect0D_SpawnForEm(em, 0x1B, 6);
            break;
        }
        break;
    case 0x425:
        if (Player_PassedFrameScaled(em, 30.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x1A, 7, 1.0f);
            break;
        }
        break;
    case 0x426:
    case 0x427:
    case 0x428:
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
            break;
        }
        break;
    case 0x42A:
    case 0x42B:
        break;
    case 0x42C:
        if (Player_PassedFrameScaled(em, 268.0f, 0) != 0) {
            StageObj_K14_Create(em, 0, 0, 0.9f);
            break;
        }
        break;
    case 0x42D:
    case 0x432:
        if (Player_PassedFrameScaled(em, 162.0f, 0) != 0) {
            if (em->type == 0x11 || em->type == 0x2F) {
                EffGravios19_Create(em, 0x17);
            } else {
                EffGravios19_Create(em, 0x1D);
            }
        }
        if (work->motion == 0x432) {
            if (Player_PassedFrameScaled(em, 142.0f, 0) != 0) {
                StageObj_K14_Create(em, 0x12, 0x81, 1.0f);
            }
        } else if (Player_PassedFrameScaled(em, 142.0f, 0) != 0) {
            StageObj_K14_Create(em, 0x12, 0x80, 1.0f);
        }
        if ((Player_PassedFrameScaled(em, 148.0f, 0) != 0) || (Player_PassedFrameScaled(em, 188.0f, 0) != 0) || (Player_PassedFrameScaled(em, 238.0f, 0) != 0) || (Player_PassedFrameScaled(em, 282.0f, 0) != 0)) {
            StageObj_K14_Create(em, 1, 0x80, 1.0f);
            break;
        }
        break;
    case 0x42E:
    case 0x433:
    case 0x42F:
    case 0x430:
        break;
    case 0x431:
        EmGravios_RequestOnFrame(em, 0x1A, 2);
        if ((EmMotion_ReachedFrame(em, 0, 28.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 52.0f) == 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create(em, 0xA, 0x80, 1.0f);
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
    case 0x438:
    case 0x439:
    case 0x43A:
    case 0x43B:
        break;
    case 0x43C:
        if (Player_PassedFrameScaled(em, 42.0f, 0) || Player_PassedFrameScaled(em, 60.0f, 0) || Player_PassedFrameScaled(em, 80.0f, 0)) {
            StageObj_K14_Create(em, 1, 0, 1.0f);
            break;
        }
        break;
    case 0x43F:
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
        break;
    case 0x44C:
        if (Player_PassedFrameScaled(em, 28.0f, 0) != 0) {
            StageObj_K14_Create(em, 2, 6, 1.8f);
        }
        if ((EmMotion_InFrameRange(em, 0, 54.0f, 120.0f) != 0) && !(gScene.frame & 3)) {
            Effect0D_SpawnForEmScaled(em, 2, 3, 7.0f);
            break;
        }
        break;
    case 0x44D:
        if (em->rage != 0) {
            if (EmMotion_InFrameRange(em, 0, 58.0f, 270.0f) || EmMotion_InFrameRange(em, 0, 410.0f, 440.0f)) {
                if (!(gScene.frame & 3)) {
                    StageObj_K14_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
                    break;
                }
            }
        }
        break;
    case 0x44F:
    case 0x450:
        break;
    case 0x451:
        if (em->stateGroup != 8) {
            if (Player_PassedFrameScaled(em, 106.0f, 0) != 0) {
                EffGravios19_Create(em, 0x18);
            }
            if ((EmMotion_InFrameRange(em, 0, 64.0f, 88.0f) != 0) && !(gScene.frame & 3)) {
                StageObj_K14_Create(em, 0x1A, 0, 1.0f);
            }
            if ((EmMotion_InFrameRange(em, 0, 208.0f, 268.0f) != 0) && !(gScene.frame & 3)) {
                StageObj_K14_Create(em, 0x1A, 1, 1.0f);
                break;
            }
        }
        break;
    case 0x452:
        if (Player_PassedFrameScaled(em, 110.0f, 0) != 0) {
            EffGravios19_Create(em, 0x12);
            StageObj_K0F_CreateOnEm(em, 5, 0xE, 1.0f);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 64.0f, 468.0f) != 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
            break;
        }
        break;
    case 0x454:
        if (Player_PassedFrameScaled(em, 70.0f, 0) != 0) {
            EffGravios19_Create(em, 3);
        }
        if (Player_PassedFrameScaled(em, 74.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 2, 0x13, 1.4f);
            if (Player_InCurrentArea(em)) {
                Em_BonePos(em, 2, pos);
                pos[1] = em->floorY;
                StageObj_K11_CreateAtScaled(pos, *(u16 *) &em->rotY, 8, 1.0f);
            }
        }
        if (Player_PassedFrameScaled(em, 14.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0xC, 0x14, 8.0f);
        }
        if (Player_PassedFrameScaled(em, 32.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 6, 0x14, 8.0f);
            break;
        }
        break;
    case 0x455:
        if (Player_PassedFrameScaled(em, 68.0f, 0) != 0) {
            if (em->type == 0x11 || em->type == 0x2F) {
                EffGravios19_Create(em, 2);
            } else {
                EffGravios19_Create(em, 0x1A);
            }
        }
        if (Player_PassedFrameScaled(em, 76.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x14, 3, 2.5f);
        }
        if ((Player_PassedFrameScaled(em, 72.0f, 0) != 0) || (Player_PassedFrameScaled(em, 74.0f, 0) != 0)) {
            Effect0D_SpawnForEmScaled(em, 0x14, 3, 2.5f);
            break;
        }
        break;
    case 0x456:
        if (em->type == 0x11 || em->type == 0x2F) {
            if (Player_PassedFrameScaled(em, 122.0f, 0) != 0) {
                EffGravios19_Create(em, 0x18);
            }
            if ((EmMotion_InFrameRange(em, 0, 64.0f, 88.0f) != 0) && !(gScene.frame & 3)) {
                StageObj_K14_Create(em, 0x1A, 0, 1.0f);
            }
            if ((EmMotion_InFrameRange(em, 0, 208.0f, 268.0f) != 0) && !(gScene.frame & 3)) {
                StageObj_K14_Create(em, 0x1A, 1, 1.0f);
                break;
            }
        }
        break;
    case 0x457:
        if ((em->type == 0x11 || em->type == 0x2F) && !(em->fxFlags & 1) && (Player_PassedFrameScaled(em, 56.0f, 0) != 0)) {
            Effect0D_SpawnForEmScaled(em, 0x2C, 7, 2.0f);
            break;
        }
        break;
    case 0x458:
        if (Player_PassedFrameScaled(em, 46.0f, 0) != 0) {
            EffGravios19_Create(em, 4);
            break;
        }
        break;
    case 0x459:
        break;
    case 0x45A:
        if ((gScene.eventRunning == 0) && (em->stateGroup != 8)) {
            if (Player_PassedFrameScaled(em, 94.0f, 0) != 0) {
                EffGravios19_Create(em, 0x20);
            }
            if (Player_PassedFrameScaled(em, 64.0f, 0) != 0) {
                StageObj_K14_Create(em, 0xD, 0, em->scaleX);
            }
            if (Player_PassedFrameScaled(em, 66.0f, 0) != 0) {
                StageObj_K14_Create(em, 0x15, 0, em->scaleX);
                break;
            }
        }
        break;
    case 0x45B:
        break;
    case 0x45C:
        if ((Player_PassedFrameScaled(em, 198.0f, 0) != 0) || (Player_PassedFrameScaled(em, 284.0f, 0) != 0) || (Player_PassedFrameScaled(em, 328.0f, 0) != 0) || (Player_PassedFrameScaled(em, 368.0f, 0) != 0) || (Player_PassedFrameScaled(em, 408.0f, 0) != 0) || (Player_PassedFrameScaled(em, 468.0f, 0) != 0)) {
            StageObj_K14_Create(em, 1, 0, 1.0f);
            break;
        }
        break;
    case 0x45E:
    case 0x45F:
        break;
    case 0x460:
        if (em->type == 0x16) {
            if (Player_PassedFrameScaled(em, 22.0f, 0) != 0) {
                StageObj_K14_Create(em, 2, 0, 1.5f);
            }
            if (Player_PassedFrameScaled(em, 82.0f, 0) != 0) {
                StageObj_K14_Create(em, 3, 0, 1.5f);
            }
            if (Player_PassedFrameScaled(em, 102.0f, 0) != 0) {
                Effect0D_SpawnForEmScaled(em, 2, 0x13, 1.0f);
            }
            if (Player_PassedFrameScaled(em, 170.0f, 0) != 0) {
                StageObj_K14_Create(em, 0xB, 0x80, 0.8f);
                break;
            }
        }
        break;
    case 0x461:
        if (em->type == 0x16) {
            if (Player_PassedFrameScaled(em, 8.0f, 0) != 0) {
                Effect0D_SpawnForEmScaled(em, 2, 0x13, 1.0f);
                if (Player_InCurrentArea(em)) {
                    Em_BonePos(em, 2, pos);
                    pos[1] = em->floorY;
                    StageObj_K11_CreateAtScaled(pos, *(u16 *) &em->rotY, 8, 1.0f);
                }
            }
            if (Player_PassedFrameScaled(em, 206.0f, 0) != 0) {
                StageObj_K14_Create(em, 0xB, 0x80, 0.5f);
                break;
            }
        }
        break;
    case 0x462:
        EmGravios_RequestOnFrame(em, 2, 5);
        if (Player_PassedFrameScaled(em, 2.0f, 0) != 0) {
            EffGravios19_Create(em, 0x1E);
        }
        if (Player_PassedFrameScaled(em, 2.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 2, 0x13, 1.4f);
            break;
        }
        break;
    default:
        EmGravios_MotionEffectsDefault(em);
        break;
    }
}

void EmGravios_UpdateMotionEffects(Em *em)
{
    EmFlyerWork *w = (EmFlyerWork *)&em->work;

    switch (w->fxStep) {
    case 0:
        w->fxStep++;
        break;
    case 1:
        EmGravios_MotionEffects(em, (EmGraviosFxWork *) w);
        break;
    }
}

void EmGravios_TableNop(void)
{
}

s32 EmGravios_AllowHit8(Em *em)
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
            if (em->state != 0x11) {
                goto block_8;
            }
            goto block_21;
        }
block_8:
        if (stateGroup == 4) {
            if (em->state != 0x12) {
                goto block_11;
            }
            goto block_21;
        }
block_11:
        if (stateGroup == 4) {
            if (em->state != 0x13) {
                goto block_14;
            }
            goto block_21;
        }
block_14:
        if (stateGroup == 4) {
            if (em->state != 0x14) {
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

void EmGravios_SetGroup0State(Em *em, u16 state, u8 mode)
{
    em->motionSpeed = 1.0f;
    switch (state) {
    case 1:
        if (em->rage != 0) {
            state = 2;
        }
        break;
    case 14:
        Em_SetTargetAngle(em);
        break;
    case 18:
        Em_ResetNotice(em);
        break;
    case 20:
        Em_BeginStatus2(em);
        break;
    case 22:
        em->motionSpeed = 0.0f;
        break;
    case 23:
        Em_Refill7F4(em);
        break;
    case 24:
        Em_EndStatus2(em);
        break;
    case 26:
        em->motionSpeed = 0.0f;
        Em_SetCombat(em, 0U, 0);
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
        Em_BeginStatus2(em);
        break;
    case 33:
        Em_EndStatus4(em);
        break;
    case 34:
        em->holdTimer2 = 0;
        break;
    case 35:
        Em_EndStatus5(em);
        break;
    case 37:
        Vec_Copy(&em->posX, em->goal);
        break;
    case 38:
        Em_TypeSetup(em);
        break;
    case 39:
        Em_StartRage(em);
        break;
    case 40:
        Em_StartPendingRage(em);
        break;
    }
    em->stance = 0;
    Em_SetStateSync(em, 0, state, mode);
}

void EmGravios_SetGroup1State(Em *em, u16 state, u8 mode)
{
    em->stance = 0;
    em->motionSpeed = 1.0f;
    switch (state) {
    case 0:
    case 5:
    case 13:
    case 14:
        switch (state) {
        case 0:
        default:
        case 5:
        case 14:
            if (em->hp < (s16) (0.3f * em->hpMax)) {
                state = 6;
                goto state6;
            }
            break;
        case 13:
            em->noReact = 1;
            em->senseDelay = 5;
            em->noHitTimer = 5;
            em->noPushTimer = 5;
            break;
        }
        Em_SetSpeedScaled(em, 0, 1000.0f, 500.0f);
        if (em->remote == 0 && em->goalKind == 0) {
            em->hasGoal = 0;
            em->timer = (em->stateRand % 120 + 180) / 2;
        }
        break;
    case 1:
        em->scriptPending = 1;
        return;
    case 8:
        em->motionSpeed = 0.7f;
    case 4:
        Em_SetSpeedScaled(em, 2, 300.0f, 0.0f);
        if (em->curGoalKind == 0) {
            em->timer = (em->stateRand % 120 + 180) / 2;
        }
        break;
    case 6:
    state6:
        Em_SetSpeedScaled(em, 6, 0.0f, 0.0f);
        if (em->curGoalKind == 0) {
            em->timer = (em->stateRand % 120 + 180) / 2;
        }
        break;
    case 9:
        Em_CheckRageByHp(em, em->rageThreshold);
        break;
    case 12:
        em->noReact = 1;
        em->senseDelay = 5;
        em->noHitTimer = 5;
        em->noPushTimer = 5;
        break;
    case 2:
    case 3:
    case 7:
    case 10:
    case 11:
        break;
    }
    Em_SetStateSync(em, 1, state, mode);
}

void EmGravios_SetGroup2State(Em *em, u16 state, u8 mode)
{
    em->motionSpeed = 1.0f;
    switch (state) {
    case 0:
    case 4:
    case 5:
    case 14:
    case 23:
    case 1:
    case 2:
    case 3:
        break;
    case 7:
    case 21:
        Player_ClearVelocity(em);
        em->velZ = 20.0f;
        em->timer = (s32) (Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) / em->velZ) * 3;
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
        break;
    case 16:
        break;
    case 20:
        if (em->velZ <= 0.0f) {
            em->velZ = 20.0f;
        }
        if (em->remote == 0) {
            em->timer = Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) / em->velZ;
        }
        break;
    }
    Em_SetStateSync(em, 2, state, mode);
}

void EmGravios_SetGroup3State(Em *em, u16 state, u8 mode)
{
    em->motionSpeed = 1.0f;
    switch ((u16) state) {
    case 0:
    case 8:
        Em_SetTargetAngle(em);
        break;
    case 1:
    case 4:
    case 0xB:
        Em_SetTargetAngle(em);
        break;
    case 2:
    case 3:
    case 5:
    case 6:
    case 7:
    case 9:
    case 0xA:
        break;
    }
    Em_SetStateSync(em, 3, state, mode);
}

void EmGravios_ChangeState(Em *em, u16 group, u16 state, u8 mode)
{
    if (em->remote == 0) {
        Em_TargetPlayerInRange(em);
    } else if (mode == 3 || mode == 4) {
        return;
    }
    Em_LoadReactionSet(em, 0);
    if (Game_IsOnline() == 0) {
        if (func_00770310(em) == 1) {
            group = 0;
            state = 12;
        }
    } else if (mode != 0) {
        if (func_00770310(em) == 1) {
            group = 0;
            state = 12;
        }
    }
    switch (group) {
    case 0:
        EmGravios_SetGroup0State(em, state, mode);
        break;
    case 1:
        EmGravios_SetGroup1State(em, state, mode);
        break;
    case 2:
        EmGravios_SetGroup2State(em, state, mode);
        break;
    case 3:
        EmGravios_SetGroup3State(em, state, mode);
        break;
    case 8:
        if (state == 0x98) {
            Versus_MarkDefeat(em);
        }
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        break;
    case 4:
        switch (state) {
        case 4:
            em->fxFlags |= 1;
            em->hit8Count++;
            Em_EndAllStatus(em);
            break;
        case 12:
        case 21:
            Em_EndAllStatus(em);
            break;
        case 11:
            Em_BeginStatus4(em);
            break;
        case 14:
            Em_BeginStatus4(em);
            break;
        case 17:
            Em_ResetToBasePose(em);
            Em_BreakPart(em, 2);
            break;
        case 18:
            Em_EndStatus4(em);
            break;
        case 19:
            Em_EndStatus2(em);
            break;
        case 20:
            Em_EndStatus1(em);
            break;
        case 16:
        case 22:
            Em_BeginStatus5(em);
            break;
        case 23:
            Em_EndStatus5(em);
            break;
        }
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        break;
    case 5:
        if (state == 4 && QuestEm_CanAppear(em) == 1) {
            QuestEm_Appear(em);
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

void EmGravios_MoveByVelYCurve(Em *em, s32 kind)
{
    EmWork *work = (EmWork *) &em->work;
    EmCurveKey *tbl;
    EmCurveKey *p;
    f32 frame;
    f32 t;
    s32 i;

    tbl = EmGravios_MoveVelYCurves[kind];
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

void EmGravios_StartCurves(Em *em, u8 kind)
{
    EmFlyerWork *w = (EmFlyerWork *)&em->work;

    w->curveNone = 0;
    w->curveVelY = 0;
    w->curveSpeed = 0;
    w->curveKind = kind;
    switch (w->curveKind) {
    case 2:
        w->curveVelY = 1;
        w->curveFrame = 0x60;
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
    }
    Player_ClearVelocity(em);
}

static s32 EmGravios_CurveNone(Em *em, EmWork *work)
{
    return 0;
}


extern EmCurveKey *EmGravios_VelYCurves[];

/* The value of curve work[0x4F] at the frame in the work area (a table of keys): sets
 * em->velY from the key pair around the frame, 2 at the table's end. */
static u8 EmGravios_CurveVelY(Em *em, EmWork *work)
{
    EmFlyerWork *w = (EmFlyerWork *) work;
    EmCurveKey *p;
    f32 frame;
    f32 t;
    s32 i;
    EmCurveKey *tbl;
    u8 ret;

    tbl = EmGravios_VelYCurves[((EmFlyerWork *)work)->curveKind];
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

extern EmCurveKey *EmGravios_SpeedCurves[];

/* The value of curve work[0x4F] at the frame in the work area: sets em->velZ from the key
 * pair around the frame, 4 at the table's end. */
static u8 EmGravios_CurveSpeed(Em *em, EmWork *work)
{
    EmFlyerWork *w = (EmFlyerWork *) work;
    EmCurveKey *p;
    f32 frame;
    f32 t;
    s32 i;
    EmCurveKey *tbl;
    u8 ret;

    tbl = EmGravios_SpeedCurves[((EmFlyerWork *)work)->curveKind];
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
u8 EmGravios_UpdateCurves(Em *em)
{
    EmFlyerWork *w = (EmFlyerWork *)&em->work;
    u8 ret = 0;

    if (w->curveNone != 0) {
        ret = EmGravios_CurveNone(em, (EmWork *) w);
    }
    if (w->curveVelY != 0) {
        ret |= EmGravios_CurveVelY(em, (EmWork *) w);
    }
    if (w->curveSpeed != 0) {
        ret |= EmGravios_CurveSpeed(em, (EmWork *) w);
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

s32 func_00770310(Em *em)
{
    if ((s16) Player_IsState(em, 0, 0x16) || (s16) Player_IsState(em, 0, 0x1A)) {
        return 1;
    }
    return 0;
}

void EmGravios_SpawnPeriodicEffect06(Em *em)
{
    f32 v[3];

    v[1] = 10.0f;
    v[2] = 140.0f;
    v[0] = 0.0f;
    Em_SpawnEffect06Periodic(em, 0x22, v, 1.6f);
    if (em->timer % 135 == 0) {
        Em_PlaySeAtBone(em, 0x57, 0x23);
    }
}

void EmKind12_MotionSounds(Em *em, EmWork *work)
{
    switch (work->fxMotion) {
    case 0x3E9:
        break;
    case 0x3EA:
        Em_PlaySeOnFrame(em, 2, 0);
        Em_PlaySeOnFrame(em, 0x26, 0);
        Em_PlaySeOnFrame(em, 0x14, Sound_RandomSe(2, 4, 4, 4));
        Em_PlaySeOnFrame(em, 0x3A, Sound_RandomSe(3, 4, 5, 4));
        break;
    case 0x3EB:
        Em_PlaySeOnFrame(em, 2, 1);
        break;
    }
}

void EmKind12_Init(Em *em)
{
    EmMosswineWork *w = (EmMosswineWork *)&em->work;

    f32 fslot;
    u8 slotNo;

    if (gQuest.stage == 0) {
        if (gScene.area == 0xF) {
            em->posX = 9500.0f + (150.0f * (f32) (em->slotNo / 8));
            em->posY = 0.0f;
            em->posZ = 9200.0f + (150.0f * (f32) (em->slotNo & 7));
        } else {
            slotNo = em->slotNo;
            fslot = (f32) (u32) slotNo;
            em->posX = 2400.0f + (150.0f * fslot);
            em->posY = 0.0f;
            em->posZ = 3600.0f;
        }
    }
    Em_SetMotion(em, 1, 0, 0);
    em->stance = 0;
    Em_SetStateRand(em, 0, 1);
    em->hp = 0x32;
    em->hpMax = 0x32;
    em->scriptPending = 1;
    w->unk00C = 0;
    w->unk00E = 0;
    w->home[0] = em->posX;
    w->home[1] = 0.0f;
    w->home[2] = em->posY;
    em->remote = 0;
    em->senseDelay = 5;
}

void EmKind12_State0_00(void)
{
}

void EmKind12_State0_01(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        if (em->motionId[0] != 0x3E9) {
            Em_SetMotion(em, 1, 0, 0);
        }
        em->timer = 0xC8;
        break;
    case 1:
        if (--em->timer < 0) {
            em->scriptPending = 1;
        }
        break;
    }
}

void EmKind12_State0_02(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        if (em->motionId[0] != 0x3EB) {
            Em_SetMotion(em, 3, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->scriptPending = 1;
        }
        break;
    }
}

void EmKind12_RunGroup0(Em *em)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->state) {
    case 0:
        EmKind12_State0_00();
        break;
    case 1:
        EmKind12_State0_01(em);
        break;
    case 2:
        EmKind12_State0_02(em);
        break;
    }
}

void EmKind12_State1_00(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        if (em->motionId[0] != 0x3EA) {
            Em_SetMotion(em, 2, 0, 0);
        }
        em->timer = 0x258;
        break;
    case 1:
        em->rotY += 0x70;
        em->rotY = (s32) (u16) em->rotY;
        Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
        if (--em->timer <= 0) {
            em->scriptPending = 1;
        }
        break;
    }
}

void EmKind12_State1_01(Em *em)
{
    if (((Em *) &gPlayers[em->cartPlayer])->stateGroup != 4) {
        em->taskPhase++;
        em->visible = 0;
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            if (em->motionId[0] != 0x3EA) {
                Em_SetMotion(em, 2, 0, 0);
            }
            em->timer = 0x5A;
            break;
        case 1:
            if (--em->timer <= 0) {
                em->scriptPending = 1;
            }
            break;
        }
    }
}

void EmKind12_State1_02(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 0x35;
        if (em->motionId[0] != 0x3EA) {
            Em_SetMotion(em, 2, 0, 0);
        }
        break;
    case 1:
        switch (gScene.area) {
        case 0xA:
        case 0x14:
            em->rotY += 0x2A0;
            break;
        default:
            em->rotY += 0x2B0;
            break;
        }
        em->rotY = (s32) (u16) em->rotY;
        Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
        if (--em->timer <= 0) {
            em->scriptPending = 1;
        }
        break;
    }
}

void EmKind12_State1_03(Em *em)
{
    s32 timer;

    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 0x64;
        if (em->motionId[0] != 0x3EA) {
            Em_SetMotion(em, 2, 0, 0);
        }
        break;
    case 1:
        if (--em->timer <= 0) {
            em->taskPhase++;
            em->visible = 0;
            em->timer = 0;
        }
        timer = em->timer;
        if (timer < 5) {
            em->alpha = (f32) timer / 5.0f;
        }
        break;
    }
}

void EmKind12_RunGroup1(Em *em)
{
    switch (em->state) {
    case 0:
        EmKind12_State1_00(em);
        break;
    case 1:
        EmKind12_State1_01(em);
        break;
    case 2:
        EmKind12_State1_02(em);
        break;
    case 3:
        EmKind12_State1_03(em);
        break;
    }
}

void EmKind12_RunGroup2(void)
{
}

void EmKind12_State3_00(void)
{
}

void EmKind12_RunGroup3(Em *em)
{
    switch (em->state) {
    case 0:
        EmKind12_State3_00();
        break;
    }
}

void EmKind12_State4_00(void)
{
}

void EmKind12_RunGroup4(Em *em)
{
    switch (em->state) {
    case 0:
        EmKind12_State4_00();
        break;
    }
}

void EmKind12_Update(Em *em)
{
    EmMosswineWork *w = (EmMosswineWork *)&em->work;

    u16 t;

    t = w->unk00C;
    if (t != 0) {
        w->unk00C = t - 1;
    }
    em->senseDelay = 5;
    Em_UpdatePendingScript(em);
    Em_ClearFlag6AE(em);
    EmKind12_RunState(em);
    if (em->stateRequest != 0) {
        EmKind12_RunState(em);
        em->stateRequest = 0;
    }
}

void EmKind12_RunState(Em *em)
{
    switch (em->stateGroup) {
    case 0:
        EmKind12_RunGroup0(em);
        break;
    case 1:
        EmKind12_RunGroup1(em);
        break;
    case 2:
        EmKind12_RunGroup2();
        break;
    case 3:
        EmKind12_RunGroup3(em);
        break;
    case 4:
        EmKind12_RunGroup4(em);
        break;
    case 5:
        EmKind12_RunGroup4(em);
        break;
    case 6:
        EmKind12_RunGroup4(em);
        break;
    case 7:
        EmKind12_RunGroup4(em);
        
    default:
        break;
    }
}

void EmKind12_MotionEffects(Em *em, EmWork *work)
{
    if (em->motionId[0] != work->fxMotion) {
        work->fxMotion = em->motionId[0];
    }
    EmKind12_MotionSounds(em, work);
    switch (work->fxMotion) {
    case 0x3E9:
        break;
    case 0x3EA:
        if (EmMotion_ReachedFrame(em, 0, 6.0f) && gScene.frame % 12 == 1) {
            StageObj_K14_Create(em, 2, 5, 0.2f);
        }
        if (EmMotion_ReachedFrame(em, 0, 4.0f) && gScene.frame % 12 == 0) {
            StageObj_K14_Create(em, 3, 5, 0.2f);
        }
        break;
    case 0x3EB:
        if (Player_PassedFrameScaled(em, 16.0f, 0)) {
            Effect0D_SpawnForEm(em, 0x10, 7);
        }
        if (Player_PassedFrameScaled(em, 46.0f, 0)) {
            StageObj_K14_Create(em, 0x1C, 0, 1.0f);
            StageObj_K14_Create(em, 0x1C, 1, 1.0f);
        }
        break;
    }
}

void EmKind12_UpdateMotionEffects(Em *em)
{
    EmMosswineWork *w = (EmMosswineWork *)&em->work;

    switch (w->fxStep) {
    case 0:
        w->fxStep++;
        break;
    case 1:
        EmKind12_MotionEffects(em, (EmWork *) w);
        break;
    }
}

void EmKind12_SpawnEffect01(u8 *em)
{
    Effect01_Spawn((Player *) em, 0);
}

void EmKind12_TableNop(void)
{
}

/* Vespoid's own meanings of the work area's bytes (EmWork, include/em.h, has the shared ones). */
typedef struct EmVespoidWork {
    u8 fxStep;                  /* 0x000: UpdateMotionEffects: 0, then 1 */
    u8 unk001[0x00C - 0x001];
    s16 unk00C;                 /* 0x00C: Init: 0 */
    s16 unk00E;                 /* 0x00E: Init: 0 */
    u8 unk010[0x014 - 0x010];
    f32 home[3];                /* 0x014: Init: the spawn position (posX, 0, posZ); ChangeState copies it to the goal */
    s32 accelAngX;              /* 0x020: angles X, Y, Z for Em_MoveRotatedAccel (UpdateCurves: 0, rotY, 0) */
    s32 accelAngY;              /* 0x024 */
    s32 accelAngZ;              /* 0x028 */
    u8 curveNone;               /* 0x02C: the three curves' run flags (StartCurves, UpdateCurves) */
    u8 curveVelY;               /* 0x02D */
    u8 curveSpeed;              /* 0x02E */
    u8 curveKind;               /* 0x02F: the curve tables' index (StartCurves) */
    s16 curveFrame;             /* 0x030: the curve frame (StartCurves, UpdateCurves) */
    u8 unk032[0x034 - 0x032];
    f32 homeDist;               /* 0x034: ChangeState (1, 9): the distance to the goal set 20 above home */
    u8 unk038[0x03C - 0x038];
    s16 unk03C;                 /* 0x03C: State2_08: the timer's first value */
    u8 unk03E[0x100 - 0x03E];
} EmVespoidWork;
typedef char EmVespoidWork_size_check[(sizeof(EmVespoidWork) == sizeof(EmWork)) ? 1 : -1];

void EmVespoid_MotionSounds(s32 em, EmWork *work)
{
    s16 motion;

    motion = work->fxMotion;
    switch (motion) {
    case 0x3E9:
    case 0x3EA:
    case 0x3EB:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2A, 0, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x50, 0, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x76, 0, 0);
        break;
    case 0x3ED:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, Sound_RandomSe(0xD, 1, 0xD, 1), 0);
        break;
    case 0x3EE:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 4, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 5, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x22, 0, 0);
        break;
    case 0x3EF:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 5, 0);
        break;
    case 0x3F0:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 1, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3C, 1, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x78, 1, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xB4, 1, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xF0, 1, 0);
        break;
    case 0x3F1:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 3, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x10, 3, 0);
        break;
    case 0x3F2:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x24, 0, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 5, 0);
        break;
    case 0x3F3:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 6, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1C, 7, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x26, 0, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x48, 0, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 5, 0);
        break;
    case 0x3F4:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 9, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x44, 0, 0);
        break;
    case 0x3F5:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0xA, 0);
        break;
    case 0x3F6:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0xB, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 5, 0);
        break;
    case 0x3F7:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 8, 0);
        break;
    case 0x3F8:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0xC, 0);
        break;
    case 0x3F9:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 5, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x20, 2, 0);
        break;
    case 0x3FA:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 9, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x14, 2, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x10, 5, 0);
        break;
    case 0x3FB:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, Sound_RandomSe(0xD, 1, 0xD, 1), 0);
        break;
    case 0x3FC:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 8, 0);
        break;
    case 0x3FD:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0xA, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xC, 5, 0);
        break;
    case 0x3FE:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 5, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x32, 2, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x4C, 3, 0);
        
    default:
        break;
    }
}

void EmVespoid_Init(Em *em)
{
    EmVespoidWork *w = (EmVespoidWork *)&em->work;
    s16 hp;

    if (gQuest.stage == 0) {
        em->posX = 150.0f * (f32) (u32) em->slotNo + gStartPos[gScene.area * 3];
        em->posY = gStartPos[gScene.area * 3 + 1];
        em->posZ = gStartPos[gScene.area * 3 + 2];
    }
    Em_SetMotionRow(em, 1, 0, 0);
    em->stance = 0;
    hp = Em_ComputeSize(em, em->variant);
    em->hp = hp;
    em->hpMax = hp;
    em->scaleX = EmVespoid_Scales[em->variant];
    em->scaleY = EmVespoid_Scales[em->variant];
    em->scaleZ = EmVespoid_Scales[em->variant];
    em->bodyScale = em->scaleX;
    GmEm_ApplySize(em);
    em->scriptPending = 1;
    em->blind = 0;
    w->unk00C = 0;
    w->unk00E = 0;
    w->home[0] = em->posX;
    w->home[1] = 0.0f;
    w->home[2] = em->posZ;
    Em_InitParts(em);
    em->remote = 0;
}

void EmVespoid_Start(Em *em)
{
    EmVespoid_Init(em);
    EmVespoid_ChangeState(em, 0, 1, 0);
}

void EmVespoid_Reinit(Em *em)
{
    EmVespoid_Init(em);
}

void EmVespoid_ChangeState(Em *em, u16 group, u16 state, u8 mode)
{
    EmVespoidWork *w = (EmVespoidWork *)&em->work;
    f32 v[3];
    f32 f;

    switch (group) {
    case 1:
        switch (state) {
        case 0:
        case 1:
            if (state == 0) {
                if (em->goalKind == 0) {
                    em->hasGoal = 1;
                    Vec_Copy(em->goal, w->home);
                    em->goalDist = 600.0f + Vec_DistanceXZ(&em->posX, em->goal);
                } else {
                    em->hasGoal = 1;
                    em->goalDist = Vec_DistanceXZ(&em->posX, em->goal);
                    if (em->curGoalKind == 1 && em->curGoalArg == 0) {
                        f = em->goalDist - 50.0f;
                        em->goalDist = f;
                        if (f <= 0.0f) {
                            em->scriptPending = 1;
                            return;
                        }
                    }
                }
            } else {
                Em_SetSpeedScaled(em, 2, em->goalDist, 50.0f);
                if (em->goalKind == 0) {
                    em->timer = (em->stateRand % 120 + 90) / 2;
                } else if (em->goalDist <= 0.0f) {
                    em->scriptPending = 1;
                    return;
                }
            }
            break;
        }
        break;
    case 2:
        switch (state) {
        case 5:
            Player_ClearVelocity(em);
            if (em->goalKind == 0) {
                em->hasGoal = 0;
                em->timer = (em->stateRand % 120 + 90) / 2;
            } else {
                em->goal[1] += 20.0f;
                Em_SetSpeedScaled(em, 10, em->goalDist, 5.0f);
                if (em->curGoalKind == 1 && em->curGoalArg == 0 && em->goalDist <= 0.0f) {
                    em->scriptPending = 1;
                    return;
                }
                Vec_Sub(v, em->goal, &em->posX);
                Vec_Normalize(v);
                em->velX = 10.0f * v[0];
                em->velY = 10.0f * v[1];
                em->velZ = 10.0f * v[2];
            }
            break;
        case 8:
            if (em->goalKind == 0) {
                em->hasGoal = 0;
            } else {
                em->hasGoal = 1;
            }
            break;
        case 9:
            Player_ClearVelocity(em);
            Vec_Copy(em->goal, w->home);
            em->goal[1] += 20.0f;
            w->homeDist = Vec_Distance((f32 *) &em->posX, (f32 *) em->goal);
            em->velX = 0.0f;
            em->velY = 0.0f;
            em->velZ = 10.0f;
            break;
        }
        break;
    case 3:
        switch (state) {
        case 3:
            Em_SetSpeedScaled(em, 2, em->goalDist, -100.0f);
            if (em->goalKind == 0) {
                em->timer = (em->stateRand % 120 + 90) / 2;
            } else if (em->goalDist <= 0.0f) {
                em->scriptPending = 1;
                return;
            }
            break;
        }
        break;
    case 5:
        if (state == 2 && QuestEm_CanAppear(em) == 1) {
            QuestEm_Appear(em);
        }
        break;
    case 0:
    case 4:
    case 6:
    case 7:
    case 8:
        break;
    }
    Em_SetStateSync(em, group, state, mode);
}

void EmVespoid_EndState(Em *em)
{
    em->scriptPending = 1;
    em->stance = 0;
    EmVespoid_ChangeState(em, 0, 1, 0);
}

void EmVespoid_EndState2(Em *em)
{
    em->scriptPending = 1;
    em->stance = 2;
    EmVespoid_ChangeState(em, 2, 3, 0);
}

void EmVespoid_State0_00(void)
{
}

void EmVespoid_State0_01(Em *em)
{
    switch (em->step) {
    case 0:
        em->stance = 0;
        em->step++;
        em->timer = ((em->stateRand & 3) * 0x1E) + 0x1E;
        if (em->motionId[0] != 0x3ED) {
            Em_SetMotion(em, 5, 0, 0);
        }
        break;
    case 1:
        if (--em->timer <= 0) {
            EmVespoid_EndState(em);
        }
        break;
    }
}

void EmVespoid_State0_02(Em *em)
{
    switch (em->step) {
    case 0:
        em->stance = 0;
        em->step++;
        if (em->motionId[0] != 0x3FB) {
            Em_SetMotion(em, 0x13, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmVespoid_EndState(em);
        }
        break;
    }
}

void EmVespoid_State0_03(Em *em)
{
    f32 pos[3];

    Em_SetFlag6AE(em, 2);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->stance = 0;
        if (em->motionId[0] == 0x3F6) {
            Em_SetMotion(em, 0x10, 0, 0);
            em->step = 2;
        } else {
            Em_SetMotion(em, 0x15, 0, 0);
            em->step++;
        }
        Em_BeginStatus2(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x10, 0, 0);
        }
        break;
    case 2:
        pos[0] = 0.0f;
        pos[1] = 0.0f;
        pos[2] = 20.0f;
        Em_SpawnEffect06Periodic(em, 8, pos, 0.6f);
        if (--em->timer <= 0) {
            em->step++;
            EmVespoid_ChangeState(em, 0, 4, 4);
        }
        break;
    }
}

void EmVespoid_State0_04(Em *em)
{
    switch (em->step) {
    case 0:
        em->stance = 0;
        em->blind = 2;
        Em_EndStatus2(em);
        Em_SetMotion(em, 0x16, 0, 0);
        em->step++;
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            EmVespoid_EndState(em);
        }
        break;
    }
}

void EmVespoid_RunGroup0(Em *em)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->state) {
    case 0:
        EmVespoid_State0_00();
        break;
    case 1:
        EmVespoid_State0_01(em);
        break;
    case 2:
        EmVespoid_State0_02(em);
        break;
    case 3:
        EmVespoid_State0_03(em);
        break;
    case 4:
        EmVespoid_State0_04(em);
        break;
    }
}

void EmVespoid_State1_00(Em *em)
{
    if (em->type == 0x18) {
        em->motionSpeed = 1.2f;
    }
    switch (em->step) {
    case 0:
        em->stance = 0;
        em->step++;
        Em_SetMotion(em, 8, 0, 0);
        break;
    case 1:
        if (em->hasGoal != 0) {
            if (Em_StepToGoal(em, 0x40, 1) == 1) {
                EmVespoid_EndState(em);
                break;
            }
        } else {
            if (--em->timer <= 0) {
                EmVespoid_EndState(em);
            }
        }
        break;
    }
}

void EmVespoid_State1_01(Em *em)
{
    switch (em->step) {
    case 0:
        em->stance = 0;
        em->step++;
        Em_SetMotion(em, 9, 0, 0);
        break;
    case 1:
        if (em->hasGoal != 0) {
            if (Em_StepToGoal(em, 0x40, 1) == 1) {
                EmVespoid_EndState(em);
                break;
            }
        } else {
            if (--em->timer <= 0) {
                EmVespoid_EndState(em);
            }
        }
        break;
    }
}

void EmVespoid_State1_02(Em *em)
{
    switch (em->step) {
    case 0:
        em->stance = 0;
        em->step++;
        Player_SetFlag(em, 0x20000);
        Em_SetMotion(em, 6, 0, 0);
        break;
    case 1:
        Em_TurnToGoal(em, 0x71C, 0);
        if (Player_PassedFrameScaled(em, 16.0f, 0U) != 0) {
            em->step++;
            Em_SetMotion(em, 7, 0xA, 0);
        }
        break;
    case 2:
        Em_TurnToGoal(em, 0x71C, 0);
        if (em->motion[0].playing == 0) {
            Player_ClearFlag(em, 0x20000);
            em->step++;
            EmVespoid_EndState(em);
        }
        break;
    }
}

void EmVespoid_RunGroup1(Em *em)
{
    switch (em->state) {
    case 0:
        EmVespoid_State1_00(em);
        break;
    case 1:
        EmVespoid_State1_01(em);
        break;
    case 2:
        EmVespoid_State1_02(em);
        break;
    }
}

void EmVespoid_State2_00(Em *em, s32 unused)
{
    switch (em->step) {
    case 0:
        em->stance = 2;
        em->step++;
        em->timer = 0x64;
        Em_SetMotion(em, 2, 0, 0);
        EmVespoid_StartCurves(em, 0U);
        break;
    case 1:
        EmVespoid_UpdateCurves(em);
        if (em->posY < em->floorY) {
            em->posY = em->floorY;
        } else if (!(em->posY <= (1000.0f + em->floorY))) {
            em->posY -= 20.0f;
        }
        if (--em->timer <= 0) {
            EmVespoid_EndState2(em);
        }
        break;
    }
}

void EmVespoid_State2_01(Em *em, s16 variant)
{
    switch (em->step) {
    case 0:
        em->stance = 2;
        em->step++;
        Em_SetMotion(em, 3, 0, 0);
        if (variant == 0) {
            em->timer = 100;
            EmVespoid_StartCurves(em, 1);
        } else {
            em->motionSpeed = 8.0f;
            em->timer = 10;
            EmVespoid_StartCurves(em, 6);
        }
        break;
    case 1:
        if (variant != 0) {
            em->motionSpeed = 8.0f;
        }
        EmVespoid_UpdateCurves(em);
        if (em->posY < em->floorY) {
            em->posY = em->floorY;
        } else if (!(em->posY <= 1000.0f + em->floorY)) {
            em->posY -= 20.0f;
        }
        if (--em->timer <= 0) {
            EmVespoid_EndState2(em);
        }
        break;
    }
}

void EmVespoid_State2_02(Em *em, s32 unused)
{
    switch (em->step) {
    case 0:
        em->stance = 0;
        em->step++;
        Em_SetMotion(em, 6, 0, 0);
        EmVespoid_StartCurves(em, 3);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 6.0f, 0U) != 0) {
            em->step++;
            em->stance = 2;
            EmVespoid_UpdateCurves(em);
            EffEm11_Create(em, 8);
        }
        break;
    case 2:
        EmVespoid_UpdateCurves(em);
        if (Player_PassedFrameScaled(em, 32.0f, 0U) != 0) {
            em->step++;
            Em_SetMotion(em, 7, 0, 0);
            Player_ClearVelocity(em);
            em->stance = 0;
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            EmVespoid_EndState(em);
        }
        break;
    }
}

void EmVespoid_State2_03(Em *em)
{
    switch (em->step) {
    case 0:
        em->stance = 2;
        em->step++;
        Player_ClearVelocity(em);
        if (em->motionId[0] != 0x3E9) {
            Em_SetMotion(em, 1, 0, 0);
        }
        em->timer = ((em->stateRand & 3) * 0x1E) + 0x1E;
        break;
    case 1:
        if (em->posY < em->floorY) {
            em->posY = em->floorY;
        } else if (!(em->posY <= (1000.0f + em->floorY))) {
            em->posY -= 20.0f;
        }
        if (--em->timer <= 0) {
            em->step++;
            EmVespoid_EndState2(em);
        }
        break;
    }
}

void EmVespoid_State2_04(Em *em)
{
    switch (em->step) {
    case 0:
        em->stance = 0;
        em->step++;
        Em_SetMotion(em, 6, 0, 0);
        EmVespoid_StartCurves(em, 3U);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 6.0f, 0U) != 0) {
            em->step++;
            em->stance = 2;
            EmVespoid_UpdateCurves(em);
        }
        break;
    case 2:
        EmVespoid_UpdateCurves(em);
        if (Player_PassedFrameScaled(em, 16.0f, 0U) != 0) {
            em->step++;
            Em_SetMotion(em, 1, 0xA, 0);
            em->timer = 0xA;
        }
        break;
    case 3:
        EmVespoid_UpdateCurves(em);
        if (--em->timer <= 0) {
            EmVespoid_EndState2(em);
        }
        break;
    }
    if (em->posY < em->floorY) {
        em->posY = em->floorY;
        return;
    }
    if (!(em->posY <= (1000.0f + em->floorY))) {
        em->posY -= 20.0f;
    }
}

void EmVespoid_State2_05(Em *em)
{
    f32 goalDist;
    f32 floorY;

    switch (em->step) {
    case 0:
        em->stance = 2;
        em->step++;
        if (em->motionId[0] != 0x3E9) {
            em->timer = ((Rand_Next(1) & 0xFFFF & 3) * 0x1E) + 0x1E;
            Em_SetMotion(em, 1, 0, 0);
        }
        break;
    case 1:
        if (em->hasGoal != 0) {
            Em_TurnToGoal(em, 0x71C, 0);
            goalDist = em->goalDist - 10.0f;
            em->goalDist = goalDist;
            if (goalDist <= 0.0f) {
                em->step++;
                EmVespoid_EndState2(em);
            }
        } else {
            if (--em->timer <= 0) {
                em->step++;
                EmVespoid_EndState2(em);
            }
        }
        Player_Integrate(em);
        floorY = em->floorY;
        if (em->posY < floorY) {
            em->posY = floorY;
        }
        if (--em->timer <= 0) {
            em->step++;
            EmVespoid_EndState2(em);
        }
        break;
    }
}

void EmVespoid_State2_06(Em *em)
{
    switch (em->step) {
    case 0:
        em->stance = 2;
        em->step++;
        em->velX = 0.0f;
        em->velY = -5.0f;
        em->velZ = 0.0f;
        em->accelX = 0.0f;
        em->accelY = 0.0f;
        em->accelZ = 0.0f;
        Em_SetMotion(em, 1, 0, 0);
        break;
    case 1:
        Player_Integrate(em);
        if (em->posY < em->floorY) {
            em->step++;
            em->posY = em->floorY;
            Em_SetMotion(em, 0x11, 0, 0);
            em->stance = 0;
            break;
        }
        if (!(em->posY <= (1000.0f + em->floorY))) {
            em->posY -= 20.0f;
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmVespoid_EndState(em);
        }
        break;
    }
}

void EmVespoid_State2_08(Em *em)
{
    EmVespoidWork *w = (EmVespoidWork *)&em->work;

    switch (em->step) {
    case 0:
        em->stance = 2;
        em->step++;
        Player_ClearVelocity(em);
        if (em->stateRand & 1) {
            em->velX = 8.0f;
        } else {
            em->velX = -8.0f;
        }
        em->velY = (f32) ((em->stateRand & 7) - 4);
        em->velZ = (f32) (((em->stateRand >> 4) & 7) - 4);
        if (!(em->velZ < 0.0f)) {
            Em_SetMotion(em, 2, 0, 0);
        } else {
            Em_SetMotion(em, 3, 0, 0);
        }
        Vec_RotateY(&em->velX, 6.2831855f * em->rotY / 65536.0f);
        Vec_Copy(&em->accelX, &em->velX);
        em->timer = 3;
        w->unk03C = em->timer;
        break;
    case 1:
        Player_Integrate(em);
        if (em->hasGoal != 0) {
            Em_TurnToGoal(em, 0x71C, 0);
        }
        if (--em->timer <= 0) {
            em->timer = ((em->stateRand >> 1) & 3) + 4;
            em->accelX = -em->accelX;
            em->accelY = -em->accelY;
            em->accelZ = -em->accelZ;
            em->step++;
        }
        break;
    case 2:
        Player_Move(em);
        if (em->hasGoal != 0) {
            Em_TurnToGoal(em, 0x71C, 0);
        }
        if (--em->timer <= 0) {
            em->step++;
            em->timer = 3;
        }
        break;
    case 3:
        Player_Integrate(em);
        if (em->hasGoal != 0) {
            Em_TurnToGoal(em, 0x71C, 0);
        }
        if (--em->timer <= 0) {
            em->step++;
            EmVespoid_EndState2(em);
        }
        break;
    }
    if (em->posY < em->floorY) {
        em->posY = em->floorY;
        return;
    }
    if (!(em->posY <= (1000.0f + em->floorY))) {
        em->posY -= 20.0f;
    }
}

void EmVespoid_State2_09(Em *em)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->stance = 2;
        em->step++;
        em->timer = ((em->stateRand & 3) * 0xF) + 0x1E;
        Em_SetMotion(em, 2, 0, 0);
        break;
    case 1:
        Em_TurnToGoal(em, 0x71C, 0);
        EmVespoid_MoveByVelocity(em);
        floorY = em->floorY;
        if (em->posY < floorY) {
            em->posY = floorY;
        }
        if (--em->timer <= 0) {
            em->step++;
            EmVespoid_EndState2(em);
        }
        break;
    }
}

void EmVespoid_RunGroup2(Em *em)
{
    switch (em->state) {
    case 0:
        EmVespoid_State2_00(em, 0);
        break;
    case 1:
        EmVespoid_State2_01(em, 0);
        break;
    case 2:
        EmVespoid_State2_02(em, 0);
        break;
    case 3:
        EmVespoid_State2_03(em);
        break;
    case 4:
        EmVespoid_State2_04(em);
        break;
    case 5:
        EmVespoid_State2_05(em);
        break;
    case 6:
        EmVespoid_State2_06(em);
        break;
    case 7:
        EmVespoid_State2_01(em, 1);
        break;
    case 8:
        EmVespoid_State2_08(em);
        break;
    case 9:
        EmVespoid_State2_09(em);
        
    default:
        break;
    }
}

void EmVespoid_State3_00(Em *em)
{
    switch (em->step) {
    case 0:
        em->stance = 2;
        em->step++;
        Em_SetMotion(em, 0xA, 0, 0);
        EmVespoid_StartCurves(em, 2U);
        break;
    case 1:
        EmVespoid_UpdateCurves(em);
        if (em->posY <= em->floorY) {
            em->posY = em->floorY;
        } else if (!(em->posY <= (1000.0f + em->floorY))) {
            em->posY -= 20.0f;
        }
        if (em->motion[0].playing == 0) {
            EmVespoid_EndState2(em);
        }
        break;
    }
}

void EmVespoid_State3_01(Em *em)
{
    switch (em->step) {
    case 0:
        em->stance = 2;
        em->step++;
        Em_SetMotion(em, 0xB, 0, 0);
        em->velX = 0.0f;
        em->velY = 0.0f;
        em->velZ = 2.0f;
        em->accelX = 0.0f;
        em->accelY = 0.0f;
        em->accelZ = 0.0f;
        Vec_RotateY(&em->velX, 6.2831855f * em->rotY / 65536.0f);
        break;
    case 1:
        Player_Integrate(em);
        if (em->posY <= em->floorY) {
            em->posY = em->floorY;
        } else if (!(em->posY <= 1000.0f + em->floorY)) {
            em->posY -= 20.0f;
        }
        if (Motion_PassedFrame(em, 0, 34.0f) != 0) {
            switch (em->variant) {
            default:
                EffEm11_Create(em, 7);
                break;
            case 1:
                EffEm11_Create(em, 0xC);
                break;
            case 2:
                EffEm11_Create(em, 0x10);
                break;
            }
        }
        if (em->motion[0].playing == 0) {
            Player_ClearVelocity(em);
            EmVespoid_EndState2(em);
        }
        break;
    }
}

void EmVespoid_State3_02(Em *em)
{
    switch (em->step) {
    case 0:
        em->stance = 0;
        em->step++;
        Em_SetMotion(em, 6, 0, 0);
        EmVespoid_StartCurves(em, 3);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 6.0f, 0U) != 0) {
            em->step++;
            em->stance = 2;
            EmVespoid_UpdateCurves(em);
            EffEm11_Create(em, 8);
        }
        break;
    case 2:
        EmVespoid_UpdateCurves(em);
        if (Player_PassedFrameScaled(em, 32.0f, 0U) != 0) {
            em->step++;
            Em_SetMotion(em, 7, 0, 0);
            Player_ClearVelocity(em);
            em->stance = 0;
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            EmVespoid_EndState(em);
        }
        break;
    }
}

void EmVespoid_State3_03(Em *em)
{
    em->motionSpeed = 1.3f;
    switch (em->step) {
    case 0:
        em->stance = 0;
        em->step++;
        Em_SetMotion(em, 9, 0, 0);
        EffEm11_Create(em, 0xB);
        break;
    case 1:
        if (em->hasGoal != 0) {
            if (Em_StepToGoal(em, 0x40, 1) == 1) {
                EmVespoid_EndState(em);
                break;
            }
        } else {
            if (--em->timer <= 0) {
                EmVespoid_EndState(em);
            }
        }
        break;
    }
}

void EmVespoid_State3_04(Em *em)
{
    switch (em->step) {
    case 0:
        em->stance = 2;
        em->step++;
        Em_SetMotion(em, 0xB, 0, 0);
        em->velX = 0.0f;
        em->velY = 0.0f;
        em->velZ = 2.0f;
        em->accelX = 0.0f;
        em->accelY = 0.0f;
        em->accelZ = 0.0f;
        Vec_RotateY(&em->velX, 6.2831855f * em->rotY / 65536.0f);
        break;
    case 1:
        Player_Integrate(em);
        if (em->posY <= em->floorY) {
            em->posY = em->floorY;
        } else if (!(em->posY <= 1000.0f + em->floorY)) {
            em->posY -= 20.0f;
        }
        if (Motion_PassedFrame(em, 0, 34.0f) != 0) {
            switch (em->variant) {
            default:
                EffEm11_Create(em, 0xE);
                break;
            case 1:
                EffEm11_Create(em, 0xF);
                break;
            case 2:
                EffEm11_Create(em, 0x11);
                break;
            }
        }
        if (em->motion[0].playing == 0) {
            Player_ClearVelocity(em);
            EmVespoid_EndState2(em);
        }
        break;
    }
}

void EmVespoid_RunGroup3(Em *em)
{
    switch (em->state) {
    case 0:
        EmVespoid_State3_00(em);
        break;
    case 1:
        EmVespoid_State3_01(em);
        break;
    case 2:
        EmVespoid_State3_02(em);
        break;
    case 3:
        EmVespoid_State3_03(em);
        break;
    case 4:
        EmVespoid_State3_04(em);
        break;
    }
}

void EmVespoid_State4_00(Em *em)
{
    switch (em->step) {
    case 0:
        em->stance = 2;
        em->step++;
        Em_SetMotion(em, 0xC, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmVespoid_EndState2(em);
        }
        break;
    }
}

void EmVespoid_State4_01(Em *em)
{
    switch (em->step) {
    case 0:
        em->stance = 2;
        em->step++;
        Em_SetMotion(em, 0xD, 0, 0);
        Em_RestartScript(em);
        EmVespoid_StartCurves(em, 4U);
        break;
    case 1:
        EmVespoid_UpdateCurves(em);
        if (em->posY <= em->floorY) {
            em->step++;
            em->posY = em->floorY;
            em->stance = 0;
            Em_SetMotion(em, 0xE, 0, 0);
            Player_ClearVelocity(em);
            break;
        }
        if (!(em->posY <= (1000.0f + em->floorY))) {
            em->posY -= 20.0f;
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmVespoid_EndState(em);
        }
        break;
    }
}

void EmVespoid_State4_02(Em *em)
{
    switch (em->step) {
    case 0:
        em->stance = 0;
        if (em->motionId[0] == 0x3F6) {
            Em_SetMotion(em, 0x10, 0, 0);
            em->step = 2;
        } else {
            Em_SetMotion(em, 0x15, 0, 0);
            em->step++;
        }
        Em_RestartScript(em);
        Em_BeginStatus4(em);
        em->noReact = 1;
        Em_SetFlag6AE(em, 4U);
        break;
    case 1:
        Em_SetFlag6AE(em, 4U);
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x10, 0, 0);
        }
        break;
    case 2:
        Em_SetFlag6AE(em, 4U);
        Em_SpawnEffect06Bone(em, 2);
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x16, 0, 0);
            Em_EndStatus4(em);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmVespoid_EndState(em);
        }
        break;
    }
}

void EmVespoid_State4_03(Em *em)
{
    switch (em->step) {
    case 0:
        em->stance = 0;
        em->step++;
        Em_SetMotion(em, 0x12, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmVespoid_EndState(em);
        }
        break;
    }
}

void EmVespoid_State4_04(Em *em)
{
    switch (em->step) {
    case 0:
        em->stance = 2;
        em->step++;
        if (em->motionId[0] != 0x3F5) {
            Em_SetMotion(em, 0xD, 0, 0);
            EmVespoid_StartCurves(em, 4U);
        }
        Em_RestartScript(em);
        break;
    case 1:
        EmVespoid_UpdateCurves(em);
        if (em->posY <= em->floorY) {
            em->step++;
            em->posY = em->floorY;
            em->stance = 0;
            Em_SetMotion(em, 0xE, 0, 0);
            Player_ClearVelocity(em);
            break;
        }
        if (!(em->posY <= (1000.0f + em->floorY))) {
            em->posY -= 20.0f;
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x10, 0, 0);
            em->timer = 0xC8;
        }
        break;
    case 3:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x16, 0, 0);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmVespoid_EndState(em);
        }
        break;
    }
}

void EmVespoid_State4_05(Em *em)
{
    switch (em->step) {
    case 0:
        em->stance = 0;
        em->step++;
        if (em->motionId[0] != 0x3FD) {
            Em_SetMotion(em, 0x15, 0, 0);
        }
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x10, 0, 0);
            em->timer = 0xC8;
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x16, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmVespoid_EndState(em);
        }
        break;
    }
}

void EmVespoid_RunGroup4(Em *em)
{
    switch (em->state) {
    case 0:
        EmVespoid_State4_00(em);
        break;
    case 1:
        EmVespoid_State4_01(em);
        break;
    case 2:
        EmVespoid_State4_02(em);
        break;
    case 3:
        EmVespoid_State4_03(em);
        break;
    case 4:
        EmVespoid_State4_04(em);
        break;
    case 5:
        EmVespoid_State4_05(em);
        
    default:
        break;
    }
}

void EmVespoid_State5_00_Hunted(Em *em)
{
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        QuestEm_Hunted(em);
        em->step++;
        if (em->stance == 2) {
            if (em->motionId[0] != 0x3F7) {
                Em_SetMotion(em, 0xF, 0, 0);
            }
        } else {
            if (em->motionId[0] != 0x3FC) {
                Em_SetMotion(em, 0x14, 0, 0);
            }
        }
        break;
    case 1:
        em->hideShadowHold = 2;
        em->hideShadow = 2;
        if (EmMotion_ReachedFrame(em, 0, (em->motionId[0] == 0x3F7) ? 100 : 50) != 0) {
            em->alpha -= 1.0f / 60.0f;
            if (em->alpha <= 0.0f) {
                if (em->motion[0].playing == 0) {
                    em->visible = 0;
                    EmVespoid_ChangeState(em, 5, 2, 4);
                }
                em->alpha = 0.0f;
            }
        }
        break;
    }
}

void EmVespoid_State5_01_HuntedFalling(Em *em)
{
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        QuestEm_Hunted(em);
        if (em->stance == 2) {
            em->step++;
            if (em->motionId[0] != 0x3F5) {
                Em_SetMotion(em, 0xD, 0, 0);
                EmVespoid_StartCurves(em, 4);
            }
        } else {
            em->step = 2;
            if (em->motionId[0] != 0x3FD) {
                Em_SetMotion(em, 0x15, 0, 0);
            }
        }
        break;
    case 1:
        EmVespoid_UpdateCurves(em);
        if (em->posY <= em->floorY) {
            em->step++;
            em->posY = em->floorY;
            em->stance = 0;
            Em_SetMotion(em, 0xE, 0, 0);
            Player_ClearVelocity(em);
        } else if (!(em->posY <= em->floorY + 1000.0f)) {
            em->posY -= 20.0f;
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x10, 0, 0);
            EmMine_Create(em, 0);
            em->fieldCollide = 0;
            em->timer = 900;
        }
        break;
    case 3:
        if (EmMine_Update(em) <= 0 || --em->timer <= 0) {
            em->step++;
            EmMine_Remove(em);
        }
        break;
    case 4:
        em->alpha -= 1.0f / 60.0f;
        if (em->alpha <= 0.0f) {
            em->visible = 0;
            EmVespoid_ChangeState(em, 5, 2, 4);
            em->alpha = 0.0f;
        }
        break;
    }
}

void EmVespoid_State5_02(Em *em)
{
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        if (QuestEm_CanAppear(em) == 1) {
            em->posY += 300.0f;
            em->alpha = 0.0f;
            Em_SetMotionRow(em, 3, 0, 0);
            EmVespoid_ChangeState(em, 7, 0, 0);
        } else {
            em->taskPhase++;
        }
        break;
    case 1:
        break;
    }
}

void EmVespoid_RunGroup5(Em *em)
{
    em->noHitTimer = 0xA;
    em->noPushTimer = 0xA;
    switch (em->state) {
    case 0:
        EmVespoid_State5_00_Hunted(em);
        break;
    case 1:
        EmVespoid_State5_01_HuntedFalling(em);
        break;
    case 2:
        EmVespoid_State5_02(em);
        break;
    }
}

void EmVespoid_State6_00(Em *em)
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
        EmVespoid_EndState(em);
        break;
    }
}

void EmVespoid_State6_01(Em *em)
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
        EmVespoid_EndState(em);
        break;
    }
}

void EmVespoid_RunGroup6(Em *em)
{
    switch (em->state) {
    case 0:
        EmVespoid_State6_00(em);
        break;
    case 1:
        EmVespoid_State6_01(em);
        break;
    }
}

void EmVespoid_State7_00(Em *em)
{

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotionRow(em, 3, 0, 0);
        em->stance = 2;
        Player_ClearVelocity(em);
        em->timer = 0x96;
        
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            em->velY = -10.0f;
            em->timer = 0x1E;
        }
        break;
    case 2:
        em->alpha += 0.033333335f;
        if (!(em->alpha <= 1.0f)) {
            em->alpha = 1.0f;
        }
        Player_Integrate(em);
        if (em->posY < em->floorY) {
            em->posY = em->floorY;
        } else if (!(em->posY <= (1000.0f + em->floorY))) {
            em->posY = em->posY - 20.0f;
        }
        if (--em->timer <= 0) {
            em->timer = 0;
            em->step++;
            em->senseDelay = 0;
            EmVespoid_EndState2(em);
            em->alpha = 1.0f;
        }
        break;
    }
}

void EmVespoid_RunGroup7(Em *em)
{
    em->noHitTimer = 0xA;
    em->noPushTimer = 0xA;
    em->senseDelay = 5;
    switch (em->state) {
    case 0:
        EmVespoid_State7_00(em);
        break;
    }
}

void EmVespoid_Update(Em *em)
{
    u8 buf[4];

    em->motionSpeed = 1.0f;
    Em_UpdateCombatTimer(em);
    switch (Em_UpdateDamage(em, buf) & 0xFF) {
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
        if (em->stateGroup != 5) {
            if (em->stateRand % 20 == 0) {
                EmVespoid_ChangeState(em, 5, 1, 3);
            } else {
                EmVespoid_ChangeState(em, 5, 0, 3);
            }
        }
        em->scriptPending = 0;
        break;
    case 2:
        if (em->stateGroup != 5) {
            EmVespoid_ChangeState(em, 5, 1, 3);
        }
        em->scriptPending = 0;
        break;
    case 5:
        if (em->stateGroup != 4 || em->state != 1) {
            EmVespoid_ChangeState(em, 4, 1, 3);
        }
        break;
    case 6:
        if (em->stateGroup != 4 || em->state != 2) {
            Em_SetStatus4Timer(em);
            EmVespoid_ChangeState(em, 4, 2, 3);
        }
        break;
    case 8:
        if (em->stateGroup != 0 || em->state != 3) {
            Em_SetStatus2Timer(em);
            EmVespoid_ChangeState(em, 0, 3, 3);
            em->scriptPending = 0;
        }
        break;
    case 10:
        switch (em->state) {
        case 3:
            EmVespoid_ChangeState(em, 0, 4, 3);
            em->scriptPending = 0;
            break;
        }
        break;
    case 12:
    case 14:
        if (em->stance == 2) {
            EmVespoid_ChangeState(em, 4, 0, 3);
        } else {
            EmVespoid_ChangeState(em, 4, 3, 3);
        }
        em->scriptPending = 0;
        break;
    case 17:
        Em_EndStatus5(em);
        break;
    }
    Em_UpdatePendingScript(em);
    Em_ClearFlag6AE(em);
    EmVespoid_RunState(em);
    if (em->stateRequest != 0) {
        EmVespoid_RunState(em);
        em->stateRequest = 0;
    }
}

void EmVespoid_RunState(Em *em)
{
    switch (em->stateGroup) {
    case 0:
        EmVespoid_RunGroup0(em);
        break;
    case 1:
        EmVespoid_RunGroup1(em);
        break;
    case 2:
        EmVespoid_RunGroup2(em);
        break;
    case 3:
        EmVespoid_RunGroup3(em);
        break;
    case 4:
        EmVespoid_RunGroup4(em);
        break;
    case 5:
        EmVespoid_RunGroup5(em);
        break;
    case 6:
        EmVespoid_RunGroup6(em);
        break;
    case 7:
        EmVespoid_RunGroup7(em);
        
    default:
        break;
    }
}

void EmVespoid_MotionEffects(Em *em, EmWork *work)
{
    s16 motion;
    u16 motionId;

    motionId = em->motionId[0];
    if (motionId != work->fxMotion) {
        work->fxMotion = (s16) motionId;
    }
    EmVespoid_MotionSounds((s32) em, work);
    motion = work->fxMotion;
    switch (motion) {
    case 0x3E9:
    case 0x3EA:
    case 0x3EB:
    case 0x3EC:
    case 0x3ED:
    case 0x3EE:
    case 0x3EF:
    case 0x3F0:
    case 0x3F1:
    case 0x3F3:
    case 0x3F4:
    case 0x3F5:
    case 0x3F6:
    case 0x3F7:
    case 0x3F8:
    case 0x3F9:
    case 0x3FA:
    case 0x3FB:
    case 0x3FC:
    case 0x3FD:
    case 0x3FE:
        break;
    case 0x3F2:
        if (Motion_PassedFrame(em, 0, 12.0f) != 0) {
            EffEm11_Create(em, 6);
        }
        break;
    }
}

void EmVespoid_UpdateMotionEffects(Em *em)
{
    EmVespoidWork *w = (EmVespoidWork *)&em->work;

    switch (w->fxStep) {
    case 0:
        w->fxStep++;
        break;
    case 1:
        EmVespoid_MotionEffects(em, (EmWork *) w);
        break;
    }
}

void EmVespoid_SpawnEffect01(u8 *em)
{
    Effect01_Spawn((Player *) em, 0);
}

void EmVespoid_TableNop(void)
{
}

void EmVespoid_StartCurves(Em *em, u8 kind)
{
    EmVespoidWork *w = (EmVespoidWork *)&em->work;

    w->curveNone = 0;
    w->curveVelY = 0;
    w->curveSpeed = 0;
    w->curveKind = kind;
    switch (w->curveKind) {
    case 0:
        w->curveVelY = 1;
        w->curveSpeed = 1;
        w->curveFrame = 0xA;
        break;
    case 1:
        w->curveVelY = 1;
        w->curveSpeed = 1;
        w->curveFrame = 0xA;
        break;
    case 2:
        w->curveVelY = 1;
        w->curveSpeed = 1;
        w->curveFrame = 0x4;
        break;
    case 3:
        w->curveVelY = 1;
        w->curveSpeed = 1;
        w->curveFrame = 0x6;
        break;
    case 4:
        w->curveVelY = 1;
        w->curveSpeed = 1;
        w->curveFrame = 0x6;
        break;
    case 5:
        w->curveVelY = 1;
        w->curveSpeed = 1;
        w->curveFrame = 0;
        break;
    case 6:
        w->curveVelY = 1;
        w->curveSpeed = 1;
        w->curveFrame = 0x3;
        break;
    }
    Player_ClearVelocity(em);
}

static s32 EmVespoid_CurveNone(Em *em, EmWork *work)
{
    return 0;
}

static u8 EmVespoid_CurveVelY(Em *em, EmWork *work)
{
    EmCurveKey *p;
    f32 frame;
    f32 t;
    s32 i;
    EmCurveKey *tbl;
    u8 ret;

    tbl = EmVespoid_VelYCurves[((EmVespoidWork *)work)->curveKind];
    ret = 0;
    if (tbl == NULL) {
        return ret;
    }
    frame = ((EmVespoidWork *)work)->curveFrame;
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
                break;
            } else {
                i = 0;
                em->velY = (p->v - p[-1].v) / ((t - p[-1].t) / em->motion[0].step);
            }
        }
    }
    return ret;
}

static u8 EmVespoid_CurveSpeed(Em *em, EmWork *work)
{
    EmCurveKey *p;
    f32 frame;
    f32 t;
    s32 i;
    EmCurveKey *tbl;
    u8 ret;

    tbl = EmVespoid_SpeedCurves[((EmVespoidWork *)work)->curveKind];
    ret = 0;
    if (tbl == NULL) {
        return ret;
    }
    frame = ((EmVespoidWork *)work)->curveFrame;
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
    return ret;
}

u8 EmVespoid_UpdateCurves(Em *em)
{
    EmVespoidWork *w = (EmVespoidWork *)&em->work;
    u8 ret = 0;

    if (w->curveNone != 0) {
        ret = EmVespoid_CurveNone(em, (EmWork *) w);
    }
    if (w->curveVelY != 0) {
        ret |= EmVespoid_CurveVelY(em, (EmWork *) w);
    }
    if (w->curveSpeed != 0) {
        ret |= EmVespoid_CurveSpeed(em, (EmWork *) w);
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

void EmVespoid_MoveByVelocity(Em *em)
{
    f32 v[3];

    Vec_Copy(v, &em->velX);
    Vec_RotateY(v, 6.2831855f * em->rotY / 65536.0f);
    em->posX += v[0];
    em->posY += v[1];
    em->posZ += v[2];
}

void EmKutku_MotionSounds(Em *em, EmWork *work)
{
    s16 motion;
    u8 type;
    u8 emType;

    motion = work->sndMotion;
    switch (motion) {
    case 0x3EB:
        Em_PlaySeAtBoneOnFrame(em, 0x34, 1, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x74, 1, 0x1A);
        break;
    case 0x3EC:
        Em_PlaySeAtBoneOnFrame(em, 0x28, 6, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 6, 0x1A);
        break;
    case 0x3ED:
        Em_PlaySeAtBoneOnFrame(em, 0x19, 1, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 1, 0x1A);
        break;
    case 0x3EE:
        Em_PlaySeAtBoneOnFrame(em, 0x19, 1, 0x1A);
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
        Em_PlaySeAtBoneOnFrame(em, 6, 0x59, 0x23);
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
    case 0x3F4:
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
    case 0x3F5:
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0xF, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0xF, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0xF, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 0xF, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x42, 0xF, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0xF, 0xC);
        break;
    case 0x3F6:
    case 0x43D:
    case 0x43E:
    case 0x440:
    case 0x441:
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x1A, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0x19, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0x19, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 0x19, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 0x19, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x19, 6);
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
    case 0x3F9:
        Em_PlaySeAtBoneOnFrame(em, 8, 0x4F, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 6, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 6, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 0x51, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x52, 0x51, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0x18, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x54, 0x18, 0x23);
        break;
    case 0x3FA:
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
    case 0x3FB:
        Em_PlaySeAtBoneOnFrame(em, 2, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 6, 0xB, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 4, 1, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 9, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x62, 3, 0x14);
        break;
    case 0x405:
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 2, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0xE, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 8, 0xE, 6);
        break;
    case 0x407:
        type = em->type;
        if ((type == 0x14) || (type == 0x27)) {
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
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0x57, 0x23);
        break;
    case 0x40B:
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
    case 0x40C:
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0x4C, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0x22, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 0, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0x15, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0xC, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0xC, 6);
        Em_PlaySeAtBoneOnFrame(em, 0xA6, 0, 0x1A);
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
        Em_PlaySeAtBoneOnFrame(em, 0xA4, 9, 0);
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
    case 0x410:
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
    case 0x411:
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
    case 0x412:
        Em_PlaySeAtBoneOnFrame(em, 8, 0x4B, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0xF, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x14, 0x2A);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 0, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 1, 0x1A);
        break;
    case 0x413:
        Em_PlaySeAtBoneOnFrame(em, 8, 0x4B, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0xF, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x14, 0x2A);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 1, 0x14);
        break;
    case 0x414:
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 0xB, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 6, 0x1A, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x1B, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0x1B, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 0x1A, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 0x1B, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x1B, 6);
        break;
    case 0x415:
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
    case 0x417:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 0x12, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0x4F, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 0x12, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0x18, 0x23);
        break;
    case 0x418:
        Em_PlaySeAtBoneOnFrame(em, 2, 0x22, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 0x22, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 0x22, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x72, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 1, 0x14);
        break;
    case 0x41B:
        emType = em->type;
        if ((emType == 0x14) || (emType == 0x27)) {
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
    case 0x41C:
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
    case 0x41E:
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
    case 0x421:
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x12, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 8, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 2, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 4, 0xD, 0xC);
        break;
    case 0x423:
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 6, 0x22, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x68, 3, 0x1A);
        break;
    case 0x424:
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
    case 0x425:
        Em_PlaySeAtBoneOnFrame(em, 6, 0x53, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x12, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 6, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 4, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0xAC, 3, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x6E, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x5A, 0x15, 0);
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
        Em_PlaySeAtBoneOnFrame(em, 4, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x76, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x7A, 0, 0x1A);
        break;
    case 0x429:
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
    case 0x42A:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x52, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x13, 0);
        break;
    case 0x42B:
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0x55, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0x17, 0);
        break;
    case 0x42C:
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
    case 0x42D:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x52, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x13, 0x2A);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 9, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0x11, 0);
        break;
    case 0x42E:
    case 0x433:
        Em_PlaySeAtBoneOnFrame(em, 8, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 0x2A, 0x23);
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
        Em_PlaySeAtBoneOnFrame(em, 4, 0x52, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x13, 0x2A);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 9, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0x11, 0);
        break;
    case 0x434:
        Em_PlaySeAtBoneOnFrame(em, 0x10, 0x5F, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 1, 0x1A);
        break;
    case 0x435:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x53, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0xB, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 0x14, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 0x1C, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 0x1B, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x5C, 0x1B, 0xC);
        break;
    case 0x436:
        Em_PlaySeAtBoneOnFrame(em, 0xA, 9, 0);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 7, 0x2A);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x11, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x2C, 0x23);
        break;
    case 0x437:
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x24, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0x22, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x66, 0x22, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xA4, 0x22, 0x23);
        break;
    case 0x438:
        Em_PlaySeAtBoneOnFrame(em, 2, 0x16, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 0x17, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 0x55, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xC8, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 3, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x140, 3, 0x1A);
        break;
    case 0x439:
        Em_PlaySeAtBoneOnFrame(em, 2, 0x16, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 0x17, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 0x55, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xC8, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x140, 3, 0x14);
        break;
    case 0x43A:
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
    case 0x43F:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x1A, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 8, 0x1B, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x1B, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 0x1C, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 0x1C, 0x22);
        break;
    case 0x442:
        Em_PlaySeAtBoneOnFrame(em, 0x20, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0xD, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 0xD, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x7C, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x80, 0xB, 0xC);
        break;
    case 0x443:
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
    case 0x447:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x10, 0);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x12, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 9, 0);
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
        Em_PlaySeAtBoneOnFrame(em, 4, 0x52, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x2B, 0x23);
        break;
    case 0x44A:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 6);
        Em_PlaySeAtBoneOnFrame(em, 8, 0x16, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 3, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x50, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0x20, 0x23);
        break;
    case 0x44C:
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
    case 0x44D:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x4F, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 8, 0xD, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x144, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0xD, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x36, 6, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x36, 7, 0x14);
        break;
    case 0x44E:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x5C, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 0x32, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x33, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0x32, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x5A, 0x33, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 5, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 0x12, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 1, 0x1A);
        break;
    case 0x450:
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
    case 0x451:
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
    case 0x452:
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
    case 0x453:
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
    case 0x454:
        Em_PlaySeAtBoneOnFrame(em, 0, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0x12, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0x54, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x19C, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xC8, 0x37, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x130, 0x16, 0x22);
        break;
    case 0x455:
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
        
    case 0x3E9:
    default:
        break;
    }
}

/* em_gravios's .data (0x8C4B30-0x8C5790) */
u16 EmGravios_EffectBones[4][2] = {
#include "game/008C4B30.inc"
};
EmTurnParams EmGravios_TurnParams[1] = {
#include "game/008C4B40.inc"
};
EmGraviosVec EmGravios_CarveThrowPt = {
#include "game/008C4B60.inc"
};
s32 func_0090A9C0(void); /* gm_sub */
void *EmGravios_Callbacks[8] = {
    EmGravios_SpawnEffects, EmGravios_SpawnEffects, EmGravios_TableNop, EmGravios_UpdateMotionEffects,
    func_0090A9C0, EmGravios_Reinit, EmGravios_AllowHit8, NULL,
};
EmCurveKey EmGravios_MoveVelYCurves_0[8] = {
#include "game/008C4B90.inc"
};
EmCurveKey EmGravios_MoveVelYCurves_1[20] = {
#include "game/008C4BD0.inc"
};
EmCurveKey EmGravios_MoveVelYCurves_3[10] = {
#include "game/008C4C70.inc"
};
EmCurveKey EmGravios_MoveVelYCurves_5[6] = {
#include "game/008C4CC0.inc"
};
EmCurveKey EmGravios_MoveVelYCurves_6[10] = {
#include "game/008C4CF0.inc"
};
EmCurveKey *EmGravios_MoveVelYCurves[16] = {
    EmGravios_MoveVelYCurves_0, EmGravios_MoveVelYCurves_1, NULL, EmGravios_MoveVelYCurves_3,
    NULL, EmGravios_MoveVelYCurves_5, EmGravios_MoveVelYCurves_6, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
};
EmCurveKey EmGravios_VelYCurves_2[22] = {
#include "game/008C4D80.inc"
};
EmCurveKey EmGravios_VelYCurves_3[10] = {
#include "game/008C4E30.inc"
};
EmCurveKey EmGravios_VelYCurves_5[14] = {
#include "game/008C4E80.inc"
};
EmCurveKey EmGravios_VelYCurves_8[8] = {
#include "game/008C4EF0.inc"
};
EmCurveKey EmGravios_VelYCurves_9[18] = {
#include "game/008C4F30.inc"
};
EmCurveKey EmGravios_VelYCurves_10[14] = {
#include "game/008C4FC0.inc"
};
EmCurveKey EmGravios_VelYCurves_11[4] = {
#include "game/008C5030.inc"
};
EmCurveKey EmGravios_VelYCurves_12[8] = {
#include "game/008C5050.inc"
};
EmCurveKey EmGravios_VelYCurves_13[24] = {
#include "game/008C5090.inc"
};
EmCurveKey *EmGravios_VelYCurves[16] = {
    NULL, NULL, EmGravios_VelYCurves_2, EmGravios_VelYCurves_3,
    NULL, EmGravios_VelYCurves_5, NULL, NULL,
    EmGravios_VelYCurves_8, EmGravios_VelYCurves_9, EmGravios_VelYCurves_10, EmGravios_VelYCurves_11,
    EmGravios_VelYCurves_12, EmGravios_VelYCurves_13, NULL, NULL,
};
EmCurveKey EmGravios_SpeedCurves_5[14] = {
#include "game/008C5190.inc"
};
EmCurveKey EmGravios_SpeedCurves_8[8] = {
#include "game/008C5200.inc"
};
EmCurveKey EmGravios_SpeedCurves_9[14] = {
#include "game/008C5240.inc"
};
EmCurveKey EmGravios_SpeedCurves_10[12] = {
#include "game/008C52B0.inc"
};
EmCurveKey *EmGravios_SpeedCurves[16] = {
    NULL, NULL, NULL, NULL,
    NULL, EmGravios_SpeedCurves_5, NULL, NULL,
    EmGravios_SpeedCurves_8, EmGravios_SpeedCurves_9, EmGravios_SpeedCurves_10, NULL,
    NULL, NULL, NULL, NULL,
};
void *EmKind12_Callbacks[8] = {
    EmKind12_SpawnEffect01, EmKind12_SpawnEffect01, EmKind12_TableNop, EmKind12_UpdateMotionEffects,
    func_0090A9C0, EmKind12_TableNop, NULL, NULL,
};
f32 EmVespoid_Scales[3] = {
#include "game/008C5370.inc"
};
void *EmVespoid_Callbacks[8] = {
    EmVespoid_SpawnEffect01, EmVespoid_SpawnEffect01, EmVespoid_TableNop, EmVespoid_UpdateMotionEffects,
    func_0090A9C0, EmVespoid_Reinit, NULL, NULL,
};
EmCurveKey EmVespoid_VelYCurves_0[10] = {
#include "game/008C53A0.inc"
};
EmCurveKey EmVespoid_VelYCurves_2[14] = {
#include "game/008C53F0.inc"
};
EmCurveKey EmVespoid_VelYCurves_3[10] = {
#include "game/008C5460.inc"
};
EmCurveKey EmVespoid_VelYCurves_4[10] = {
#include "game/008C54B0.inc"
};
EmCurveKey EmVespoid_VelYCurves_5[8] = {
#include "game/008C5500.inc"
};
EmCurveKey *EmVespoid_VelYCurves[8] = {
    EmVespoid_VelYCurves_0, EmVespoid_VelYCurves_0, EmVespoid_VelYCurves_2, EmVespoid_VelYCurves_3,
    EmVespoid_VelYCurves_4, EmVespoid_VelYCurves_5, EmVespoid_VelYCurves_0, NULL,
};
EmCurveKey EmVespoid_SpeedCurves_0[8] = {
#include "game/008C5560.inc"
};
EmCurveKey EmVespoid_SpeedCurves_1[8] = {
#include "game/008C55A0.inc"
};
EmCurveKey EmVespoid_SpeedCurves_2[14] = {
#include "game/008C55E0.inc"
};
EmCurveKey EmVespoid_SpeedCurves_3[10] = {
#include "game/008C5650.inc"
};
EmCurveKey EmVespoid_SpeedCurves_4[10] = {
#include "game/008C56A0.inc"
};
EmCurveKey EmVespoid_SpeedCurves_5[8] = {
#include "game/008C56F0.inc"
};
EmCurveKey EmVespoid_SpeedCurves_6[8] = {
#include "game/008C5730.inc"
};
EmCurveKey *EmVespoid_SpeedCurves[8] = {
    EmVespoid_SpeedCurves_0, EmVespoid_SpeedCurves_1, EmVespoid_SpeedCurves_2, EmVespoid_SpeedCurves_3,
    EmVespoid_SpeedCurves_4, EmVespoid_SpeedCurves_5, EmVespoid_SpeedCurves_6, NULL,
};
