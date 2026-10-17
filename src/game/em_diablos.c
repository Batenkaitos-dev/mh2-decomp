/*
 * Monster family: Diablos, Monoblos and their subspecies (EM_KIND_DIABLOS,
 * EM_KIND_BLACK_DIABLOS, EM_KIND_MONOBLOS and EM_KIND_WHITE_MONOBLOS). The
 * code from the family's setup function (EmDiablos_Setup, called by the
 * per-kind dispatch Em_TypeSetup) to the next family's. The state functions
 * take the monster and the work area its state dispatcher passes on (EmWork);
 * Em.step is the step within the current state. See include/em.h.
 */
#include "common.h"
#include "scene.h"
#include "quest.h"
#include "em.h"
#include "player.h"
#include "area.h"
/* prototypes of callees (round 13) */
s32 EmDiablos_InMotion448To44A(Em *em);
void Em_PlaySeAtBone(Em *em, s32 se, s32 bone);
void Em_MoveRotatedAccel(Em *em, void *ang);

/* prototypes */
void EmDiablos_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void Effect02_SpawnForOwner(void *owner, s32 ang, s32 type, s32 sub, f32 *pos, f32 scale);
void Effect0D_SpawnAt(f32 *pos, s32 kind, f32 scale);
void Effect0D_SpawnIfHere(void *o, f32 *pos, s32 type, f32 scale);
void Vec_Add(f32 *out, f32 *a, f32 *b);
void DiablosAttack_Create(Em *em, s8 kind);
void StageObj_K11_CreateAtScaled(f32 *pos, s32 v, s16 k, f32 scale);
void StageObj_K15_Create(Em *em, u8 k);
s32 Em_PlaySeChecked(Em *em, s32 id, s32 a, void *pos, s32 b, s32 c);
void Em_BonePos(void *obj, s16 bone, f32 *out);
void Effect0D_SpawnForEmScaled(Em *em, s32 a, s32 b, f32 scale);
s32 EmMotion_InFrameRange(void *em, s32 layer, f32 from, f32 to);
u16 Rand_Next(s32 stream);
void StageObj_K0F_CreateOnEm(Em *em, u8 k, u8 c, f32 scale);
void StageObj_K14_Create(Em *em, s16 k, s16 c, f32 scale);
void Em_AddHp(Em *em, s16 n);
void Em_BeginStatus1(Em *em);
void Em_BeginStatus2(Em *em);
void Em_EndStatus1(Em *em);
void Em_EndStatus2(Em *em);
void Em_EndStatus4(Em *em);
void Em_EndStatus5(Em *em);
void Em_PlaySeAtBoneOnFrame(Em *em, s32 frame, s32 se, s32 bone);
void Em_SetLayerMotion(Em *em, s32 motion, s32 blend, s32 frame, s32 layer);
void Em_SetMotion(Em *em, s32 motion, s32 a, s32 b);
u16 Math_AngleXZ(f32 *from, f32 *to);
void Player_ClearFlag(void *, u32 flag);
void Player_SetFlag(void *obj, u32 flag);
void QuestEm_Captured(Em *em);
void Em_MoveRotated(Em *, void *);
/* unit's own declaration: with the definition's void (Em *em, s32 group, s32 state, u8 mode) (game/unk_701810.c), EmDiablos_ChangeState: 6 of 193 words differ, EmDiablos_SetGroup0State: 1 of 98 words differ ... */
void Em_SetStateSync(Em *em, u16 group, u16 state, u8 mode);
void Em_StartRage(Em *);
void Em_StartPendingRage(Em *);
void Em_LoadReactionSet(Em *em, u8 k);
void Em_GetPitchToGoal(Em *em, s32 *out, f32 a, f32 b);
void Em_ClearLowHpNeed(Em *em);
void Em_RefillNeed3(Em *em);
void Em_ResetNotice(Em *em);
void Em_SpawnStatus4Effect(Em *em, s32 a);
void Em_SetHitReactMode(Em *em, u8 a);
void Em_SpawnLyingPuffs(Em *em, s32 bone, f32 *pos, f32 scale);
void Em_SetTrapTimeGoal(Em *em);
void Em_ClearPoison(Em *em);
void Em_SetHold2(Em *em);
void Em_SetCaptured(Em *em, s32 a);
void Em_PlaySeAtBoneOnLayerFrame(Em *em, s32 frame, s32 se, s32 bone, s32 flag);
s32 Em_TurnToTargetMotion(Em *em, u16 range, u16 motFront, u16 motLeft, u16 motRight);
s32 Em_StepToGoal(Em *em, u16 step, u8 flags);
u8 Em_TurnToGoal(Em *em, u16 step, u8 flags);
void Em_StartTurnToTarget(Em *em, EmTurnParams *t);
s32 Em_TurnStep(Em *em, EmTurnParams *t);
void Em_RestartScript(Em *em);
void EmDiablos_Init(Em *);
void EmDiablos_SetUnderground(Em *em, u8 next);
void EmDiablos_State0_02(Em *, EmWork *);
void EmDiablos_State0_1D(Em *, EmWork *);
void EmDiablos_State1_07(Em *, EmWork *);
void EmDiablos_State2_04(Em *, EmWork *);
void EmDiablos_State2_08(Em *, EmWork *);
void EmDiablos_State6_00(Em *, EmWork *);
void EmDiablos_State6_01(Em *, EmWork *);
void EmDiablos_State8_00_CarveStart(Em *, EmWork *);
/* The view of the work area the motion-effects function takes: the current
   motion at +2 (a real field, so MWCC does not keep its address). */
typedef struct EmDiablosFxWork {
    s16 unk000;
    s16 motion;
} EmDiablosFxWork;
void EmDiablos_MotionSounds(Em *em, struct EmWork *);
void EmDiablos_MotionEffects(Em *, EmDiablosFxWork *);
void EmDiablos_State5_00_Hunted(Em *em, EmWork *work);
void EmDiablos_State5_01_Hunted(Em *, EmWork *);
void EmDiablos_State5_02_HuntedFalling(Em *, EmWork *);
void EmDiablos_RunGroup4(Em *, EmWork *);
void EmDiablos_State3_End(Em *em, EmWork *work);
void EmDiablos_MoveByVelYCurve(Em *em, s32 kind);
void EmDiablos_StartCurves(Em *em, u8 kind);
u8 EmDiablos_UpdateCurves(Em *em);
s32 EmDiablos_ChargeHitWall(Em *em);
s32 EmDiablos_InSandState(Em *em);
void StageObj_K13_Create(void *owner, s16 v, u8 k);
s32 EmMotion_PassedFrameScaled(void *p, f32 frame, s32 layer);
s32 EmMotion_ReachedFrame(void *obj, s32 a, f32 frame);
void CamShake_ReqAIf(Em *obj, s32 i);
f32 Vec_DistanceXZ(f32 *a, f32 *b);
s32 Versus_BothDefeated(void);
s32 QuestEm_CanAppear(Em *em);
void Player_ClearVelocity(void *);
void Motion_InitTime(void *p, s16 step);
/* the calls that mask the value again (u8 or & 0xFF) cast it to s32 first: MWCC keeps the game's andi that way */
u8 Player_InCurrentArea(void *obj);
void Effect01_Spawn(void *obj, s16 type);
extern f32 gAreaGroundY[];
extern EmTurnParams EmDiablos_TurnParams[3];
void Em_SetStateRand(Em *em, s32 arg1, s32 arg2);
void Em_TargetPlayerInRange(Em *);
s32 Game_IsOnline(void);
void Versus_MarkDefeat(Em *);
s8 QuestEm_Appear(Em *em);
void Em_ResetToBasePose(Em *em);
void Em_BreakPart(Em *em, s32 part);
void Em_EndAllStatus(Em *em);
void Em_BeginStatus4(Em *em);
void Em_BeginStatus5(Em *em);
void Em_SetSpeedScaled(Em *em, u16 flags, f32 x, f32 y);
void Em_CheckRageByHp(Em *em, s16 n);
f32 Vec_Distance(f32 *a, f32 *b);
void Em_ComputeGoal(Em *em, f32 *target);
void Em_TypeSetup(Em *em);
void Em_SetTargetAngle(Em *em);
s32 Em_GetTargetPos(Em *em, u8 idx, f32 *out);
void Vec_Set(f32 *out, f32 x, f32 y, f32 z);
void Em_StartGoalAction(Em *em, s32 arg1);
void Em_ComputeGoal2(Em *em, f32 *target);
void Em_ClearPart(Em *em, u16 part);
void Em_AddNeed1(Em *em, s32 n);
void Em_AddNeed2(Em *em, s32 n);
void Em_ClearNeed1State(Em *em);
void Em_ClearNeed2State(Em *em);
s16 Player_IsState(void *obj, u8 state, u8 sub);
void MinePoint_Remove(s32);
void EmDiablos_SetGroup1State(Em *em, u16 state, u8 mode);
void EmDiablos_SetGroup2State(Em *em, u16 state, u8 mode);
void EmDiablos_SetGroup3State(Em *em, u16 state, u8 mode);
void EmDiablos_SetGroup0State(Em *em, u16 state, u8 mode);
void Em_SetMotionRow(Em *em, s32 row, s32 arg2, s32 arg3);
void Em_SetPartScaled(Em *em, s32 bit);
s32 Em_ComputeMaxHp(Em *em, s16 idx);
void StageObj_K09_Create(Em *);
void StageObj_K25_Create(Player *p, Em *em, u8 k, s32 unused, f32 *pos);
void Versus_RouteEmByTeam(Em *em);
s32 Versus_DefeatCamera(Em *pl);
typedef struct EmDiablosVec {
    f32 x, y, z;
} EmDiablosVec;
extern EmDiablosVec EmDiablos_CarveThrowPt __attribute__((aligned(8)));
extern EmCurveKey *EmDiablos_MoveVelYCurves[];
void QuestEm_Hunted(Em *em);
s8 EmMine_Create(Em *em, s32 idx);
s32 EmMine_Update(Em *em);
void EmMine_Remove(Em *em);
typedef struct EmWork_4E {
    u8 pad[0x4E];
    s16 cnt;            /* 0x4E */
} EmWork_4E;
s32 Obj_InCurrentArea(void *obj);
s32 Obj_SameArea(void *obj, Player *b);
void Vib_Start(s32 port, s32 i);
void Em_MotionDelta(Em *em, f32 *out);
EmBone *Em_Bone(void *obj, s16 bone);
void Vec_RotateSelf(f32 *v, f32 *m);
void Mtx_GetTrans(f32 *out, f32 *m);
void EmImpactAttack_CreateAt(Em *em, f32 *pos, s8 kind);
s8 EmMine_CreateTimed0(Em *em, f32 *pos);
s32 Save_TestEventFlag(s32 flag);
void GmEm_ResetSize(Em *em);
void StageObj_Stun_CreateEm(Em *em, u8 k, s16 v, s16 t, f32 y, f32 scale);
s32 Em_UpdateCombatTimer(Em *em);
void Em_RaisePlayerAreaTriggerLax(Em *em);
void Em_HoldPosNoPlayers(Em *em, u16 a, u16 b);
u8 Em_UpdateDamage(Em *em, u8 *out);
void Em_SetStatus1Timer(Em *em);
void Em_SetStatus2Timer(Em *em);
void Em_SetStatus4Timer(Em *em);
void Em_SetStatus5Timer(Em *em);
void Em_ResetKindTimer(Em *em);
void Em_UpdatePendingScript(Em *em);
void Em_ClearHitReactMode(Em *em);
u16 Area_DayId(u16 id);
void EmDiablos_RunState(Em *em, EmWork *work);
s32 EmDiablos_ShouldEndState(Em *em);
/* end of prototypes */

/* Diablos's own meanings of the work area's bytes (EmWork, include/em.h, has the shared ones). */
typedef struct EmDiablosWork {
    u8 fxStep;                  /* 0x000: UpdateMotionEffects: 0, then 1 */
    u8 unk001[0x006 - 0x001];
    s16 fxMotion;               /* 0x006: as EmWork's: frames since landing (Init: 0; Update counts it down) */
    u8 unk008[0x014 - 0x008];
    u16 goalYaw;                /* 0x014: as EmWork's (Init: 0x4000) */
    u8 unk016[0x01B - 0x016];
    s8 rageFade;                /* 0x01B: Update: counts up to 60 while enraged (then rageStep 1), down to 0 otherwise */
    s32 moveAngBX;              /* 0x01C: as EmWork's */
    s32 moveAngBY;              /* 0x020 */
    u8 unk024[0x030 - 0x024];
    s32 moveAngX;               /* 0x030 */
    s32 moveAngY;               /* 0x034 */
    s32 moveAngZ;               /* 0x038 */
    s32 pitchStep;              /* 0x03C */
    s32 turnSpeed;              /* 0x040 */
    s32 rollSpeed;              /* 0x044 */
    u8 curveNone;               /* 0x048: the three curves' run flags (StartCurves, UpdateCurves) */
    u8 curveVelY;               /* 0x049 */
    u8 curveSpeed;              /* 0x04A */
    u8 curveKind;               /* 0x04B: the curve tables' index (StartCurves) */
    s16 curveFrame;             /* 0x04C: the curve frame (StartCurves, UpdateCurves, CurveVelY, CurveSpeed) */
    u8 unk04E[0x100 - 0x04E];
} EmDiablosWork;
typedef char EmDiablosWork_size_check[(sizeof(EmDiablosWork) == sizeof(EmWork)) ? 1 : -1];

void EmDiablos_Setup(Em *em)
{
}

void EmDiablos_SpawnEffects(void *em)
{
    StageObj_K13_Create(em, 0x22, 0);
    Effect01_Spawn((Player *) em, 0);
}

void EmDiablos_Init(Em *em)
{
    EmDiablosWork *w = (EmDiablosWork *)&em->work;

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
    Em_SetMotionRow(em, 1, 0, 0);
    em->stance = 4;
    w->fxMotion = 0;
    size = Em_ComputeMaxHp(em, 0);
    em->hp = size;
    em->hpMax = size;
    em->scriptPending = 1;
    em->blind = 0;
    em->unk_InitFlag = 1;
    em->noAlert = 0;
    em->painted = 0;
    w->goalYaw = 0x4000;
    w->pitchStep = 0x100;
    w->turnSpeed = 0x200;
    w->rollSpeed = 0x100;
    w->moveAngBY = 0x2000;
    w->moveAngBX = 0;
    w->rageFade = 0;
    em->surfaceDepth = 0x447A0000;
    fxFlags = em->fxFlags & 1;
    em->fxFlags = fxFlags;
    if (fxFlags == 0) {
        switch (em->type) {
        case 0xE:
        case 0x2B:
        case 0x1A:
        case 0x2C:
            em->meshShown[1] = 0;
            StageObj_K09_Create(em);
            break;
        }
    }
}

void EmDiablos_Start(Em *em)
{
    EmDiablos_Init(em);
    em->stance = 4;
    EmDiablos_ChangeState(em, 2, 1, 0);
}

void EmDiablos_Reinit(Em *em)
{
    EmDiablos_Init(em);
}

void EmDiablos_EndState(Em *em, s32 unused1, s32 unused2)
{
    em->motionSpeed = 1.0f;
    em->stance = 0;
    em->lookAtTarget = 0;
    em->scriptPending = 1;
    EmDiablos_ChangeState(em, 0, 1, 0);
}

void EmDiablos_EndState2(Em *em)
{
    em->motionSpeed = 1.0f;
    em->stance = 4;
    em->lookAtTarget = 0;
    em->scriptPending = 1;
    EmDiablos_ChangeState(em, 2, 1, 0);
}

void EmDiablos_SetUnderground(Em *em, u8 next)
{
    em->scriptPending = 1;
    em->motionSpeed = 1.0f;
    em->stance = 2;
    em->lookAtTarget = 0;
    switch (next) {
    case 0:
        Em_SetStateRand(em, 2, 0xE);
        break;
    }
}

void EmDiablos_RestartLayerMotion(Em *em, s32 layer)
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

void EmDiablos_State0_00(Em *em, EmWork *work)
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

void EmDiablos_State0_01(Em *em, EmWork *work)
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
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDiablos_State0_02(Em *em, EmWork *work)
{
    f32 v[4];
    f32 pos[4];
    void *m;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x72, 0, 0);
        em->timer = 240;
        v[0] = 0.0f;
        v[1] = 270.0f;
        v[2] = 270.0f;
        m = Em_Bone(em, 0x22);
        Vec_RotateSelf(v, m);
        Mtx_GetTrans(pos, m);
        pos[0] += v[0];
        pos[1] += v[1];
        pos[2] += v[2];
        EmImpactAttack_CreateAt(em, pos, 8);
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = 320.0f;
        m = Em_Bone(em, 1);
        Vec_RotateSelf(v, m);
        Mtx_GetTrans(pos, m);
        pos[0] += v[0];
        pos[1] = em->floorY;
        pos[2] += v[2];
        EmMine_CreateTimed0(em, pos);
        break;
    case 1:
        if (--em->timer == 0) {
            em->step++;
            Em_SetMotion(em, 0x73, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDiablos_State0_03(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x33, 0, 0);
        em->timer = 0xA;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDiablos_State0_04(Em *em, EmWork *work)
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
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
    EmDiablos_RestartLayerMotion(em, 1);
    EmDiablos_RestartLayerMotion(em, 2);
}

void EmDiablos_State0_05(Em *em, EmWork *work)
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
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
    EmDiablos_RestartLayerMotion(em, 0);
    EmDiablos_RestartLayerMotion(em, 2);
    Em_PlaySeAtBoneOnLayerFrame(em, 0x24, 0x2F, 0x23, 1);
}

void EmDiablos_State0_06(Em *em, EmWork *work)
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
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
    EmDiablos_RestartLayerMotion(em, 0);
    EmDiablos_RestartLayerMotion(em, 2);
    Em_PlaySeAtBoneOnLayerFrame(em, 0x3E, 0x1F, 0x23, 1);
    Em_PlaySeAtBoneOnLayerFrame(em, 4, 0x17, 0, 1);
}

void EmDiablos_State0_07(Em *em, EmWork *work)
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
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
    EmDiablos_RestartLayerMotion(em, 0);
    EmDiablos_RestartLayerMotion(em, 2);
}

void EmDiablos_State0_08(void)
{
}

void EmDiablos_State0_09(Em *em, EmWork *work)
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
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDiablos_State0_0A(Em *em, EmWork *work)
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
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDiablos_State0_0B(Em *em, EmWork *work)
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
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
    EmDiablos_RestartLayerMotion(em, 0);
    EmDiablos_RestartLayerMotion(em, 2);
    Em_PlaySeAtBoneOnLayerFrame(em, 2, 0x21, 0x23, 1);
    Em_PlaySeAtBoneOnLayerFrame(em, 0x6E, 0x1F, 0x23, 1);
}

void EmDiablos_State0_0C(void)
{
}

void EmDiablos_State0_0D(Em *em, EmWork *work)
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
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
    EmDiablos_RestartLayerMotion(em, 0);
    EmDiablos_RestartLayerMotion(em, 2);
    Em_PlaySeAtBoneOnLayerFrame(em, 2, 0x20, 0x23, 1);
    Em_PlaySeAtBoneOnLayerFrame(em, 0x4A, 0x1F, 0x23, 1);
}

void EmDiablos_State0_0E(void)
{
}

void EmDiablos_State0_0F(Em *em, EmWork *work)
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
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDiablos_State0_10(Em *em, EmWork *work)
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
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDiablos_State0_11(Em *em, EmWork *work)
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
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDiablos_State0_12(Em *em, EmWork *work)
{
    s16 n;

    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x20, 0, 0);
        Em_LoadReactionSet(em, 1);
        em->timer = 0x708;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x1F, 0, 0);
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
            EmDiablos_ChangeState(em, 0, 0x17, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmDiablos_ChangeState(em, 0, 0x17, 4);
        }
        break;
    }
}

void EmDiablos_State0_13(void)
{
}

void EmDiablos_State0_14(Em *em, EmWork *work)
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
            EmDiablos_ChangeState(em, 0, 0x18, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmDiablos_ChangeState(em, 0, 0x18, 4);
        }
        break;
    }
}

void EmDiablos_State0_15(Em *em, EmWork *work)
{
    s16 n;

    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x27, 0, 0);
        em->timer = 0x258;
        Em_LoadReactionSet(em, 1);
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
            EmDiablos_ChangeState(em, 0, 0x18, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmDiablos_ChangeState(em, 0, 0x18, 4);
        }
        break;
    }
}

void EmDiablos_State0_16(Em *em, EmWork *work)
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
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDiablos_State0_17(Em *em, EmWork *work)
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
            EmDiablos_ChangeState(em, 0, 0x19, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmDiablos_ChangeState(em, 0, 0x19, 4);
        }
        break;
    }
}

void EmDiablos_State0_18(Em *em, EmWork *work)
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
            EmDiablos_ChangeState(em, 0, 0x19, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmDiablos_ChangeState(em, 0, 0x19, 4);
        }
        break;
    }
}

void EmDiablos_State0_19(Em *em, EmWork *work)
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
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDiablos_State0_1A(void)
{
}

void EmDiablos_State0_1B(Em *em, EmWork *work)
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
            EmDiablos_ChangeState(em, 0, 0x1C, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmDiablos_ChangeState(em, 0, 0x1C, 4);
        }
        break;
    }
}

void EmDiablos_State0_1D(Em *em, EmWork *work)
{
    f32 pos[3];

    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        if (EmDiablos_InMotion448To44A(em) == 1) {
            em->stance = 0;
            em->step++;
            Em_SetMotion(em, 0x61, 0, 0);
        } else {
            em->stance = 4;
            if (em->motionId[0] != 0x451) {
                Em_SetMotion(em, 0x69, 0, 0);
            }
        }
        em->lookAtTarget = 0;
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (em->motion[0].blend != 0) {
            em->stance = 4;
        } else {
            em->stance = 0;
        }
        if (em->motion[0].playing == 0) {
            em->stance = 0;
            em->step++;
            Em_SetMotion(em, 0x61, 0, 0);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            EmDiablos_ChangeState(em, 4, 0x12, 3);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmDiablos_ChangeState(em, 4, 0x12, 3);
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

void EmDiablos_State1_00(Em *em, EmWork *work)
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
                EmDiablos_EndState(em, 0, 0);
                break;
            }
        } else {
            if (--em->timer <= 0) {
                em->step++;
                EmDiablos_EndState(em, 0, 0);
            }
        }
        break;
    }
}

void EmDiablos_State1_01(Em *em, EmWork *work)
{
    EmDiablos_EndState(em, 0, 0);
}

void EmDiablos_State1_02(Em *em, EmWork *work)
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
                EmDiablos_EndState(em, 0, 0);
                break;
            }
            if (EmDiablos_ChargeHitWall(em) != 0) {
                EmDiablos_ChangeState(em, 0, 2, 1);
                break;
            }
        } else {
            if (--em->timer <= 0) {
                em->step++;
                EmDiablos_EndState(em, 0, 0);
            }
        }
        break;
    }
}

void EmDiablos_State1_03(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_StartTurnToTarget(em, &EmDiablos_TurnParams[0]);
        break;
    case 1:
        if (Em_TurnStep(em, &EmDiablos_TurnParams[0]) == 1) {
            em->step++;
            Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDiablos_State1_04(Em *em, EmWork *work)
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
                Em_SetMotion(em, 0x64, 0, 0);
                break;
            }
            if (EmDiablos_ChargeHitWall(em) != 0) {
                EmDiablos_ChangeState(em, 0, 2, 1);
                break;
            }
        } else {
            if (--em->timer <= 0) {
                em->step++;
                Em_SetMotion(em, 0x64, 0, 0);
            }
            break;
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDiablos_State1_05(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_StartTurnToTarget(em, &EmDiablos_TurnParams[1]);
        break;
    case 1:
        if (Em_TurnStep(em, &EmDiablos_TurnParams[1]) == 1) {
            em->step++;
            Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDiablos_State1_06(Em *em, EmWork *work)
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
                EmDiablos_EndState(em, 0, 0);
                break;
            }
        } else {
            if (--em->timer <= 0) {
                em->step++;
                EmDiablos_EndState(em, 0, 0);
            }
        }
        break;
    }
}

void EmDiablos_State1_07(Em *em, EmWork *work)
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
            break;
        }
        break;
    case 2:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDiablos_State2_00(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        if (em->motionId[0] != 0x44F) {
            Em_SetMotion(em, 0x67, 0, 0);
        }
        em->noReact = 1;
        em->stance = 0;
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 100.0f) != 0) {
            em->hideShadowHold = 2;
            em->hideShadow = 2;
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            em->noReact = 0;
            EmDiablos_EndState2(em);
        }
        break;
    }
    em->guardTimer = 2;
}

void EmDiablos_State2_01(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 4;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 1, 0, 0);
        break;
    case 1:
        if ((em->remote == 0) && (em->motion[0].playing == 0)) {
            em->step++;
            EmDiablos_EndState2(em);
        }
        break;
    }
}

void EmDiablos_State2_02(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        if (em->motionId[0] != 0x450) {
            Em_SetMotion(em, 0x68, 0, 0);
            em->stance = 4;
        } else if (em->motion[0].blend != 0) {
            em->stance = 4;
        } else {
            em->stance = 0;
        }
        em->noReact = 1;
        break;
    case 1:
        if (em->motion[0].blend != 0) {
            em->stance = 4;
        } else {
            em->stance = 0;
            if (EmMotion_ReachedFrame(em, 0, 100.0f) == 0) {
                em->hideShadowHold = 2;
                em->hideShadow = 2;
            }
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            em->noReact = 0;
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
    em->guardTimer = 2;
}

void EmDiablos_State2_03(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 4;
        em->lookAtTarget = 0;
        Em_StartTurnToTarget(em, &EmDiablos_TurnParams[2]);
        break;
    case 1:
        if (Em_TurnStep(em, &EmDiablos_TurnParams[2]) == 1) {
            em->step++;
            EmDiablos_EndState2(em);
        }
        break;
    }
}

void EmDiablos_State2_04(Em *em, EmWork *work)
{
    EmWork_4E *w = (EmWork_4E *) work;
    Player *p = &gPlayers[gScene.localPlayer];
    f32 v[3];

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 4;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x11, 0, 0x64);
        w->cnt = 2;
        break;
    case 1:
        if (--w->cnt == 0) {
            CamShake_ReqAIf(em, 5);
            w->cnt = 8;
        }
        if (em->hasGoal != 0) {
            if (EmMotion_ReachedFrame(em, 0, 150.0f) == 0) {
                Em_StepToGoal(em, 0x40, 1);
            } else {
                Em_MotionDelta(em, v);
                em->goalDist -= v[2];
            }
            if (em->goalDist <= 0.0f) {
                em->step++;
                EmDiablos_EndState2(em);
            }
        }
        if (!(gScene.frame & 7) && Obj_SameArea(em, p) && Obj_InCurrentArea(p) &&
            Vec_Distance((f32 *) &em->posX, (f32 *) p->pos) < 2000.0f) {
            Vib_Start(0, 2);
        }
        if (em->hasGoal == 0) {
            if (--em->timer <= 0) {
                em->step++;
                EmDiablos_EndState2(em);
            }
        }
        break;
    }
}

void EmDiablos_State2_05(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 4;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 3, 0, 0);
        break;
    case 1:
        if (Em_StepToGoal(em, 0x40, 1) == 1) {
            em->timer = 1;
        }
        if (--em->timer <= 0) {
            em->step++;
            EmDiablos_EndState2(em);
        }
        break;
    }
}

void EmDiablos_State2_06(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x12, 0, 0);
        EmDiablos_StartCurves(em, 2U);
        work->flyGuard = 0;
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 70.0f, 0U) != 0) {
            em->step++;
            em->stance = 2;
            EmDiablos_UpdateCurves(em);
        }
        break;
    case 2:
        if (EmDiablos_UpdateCurves(em)) {
            em->step++;
            EmDiablos_SetUnderground(em, 0);
        }
        break;
    }
}

void EmDiablos_State2_07(Em *em, EmWork *work)
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
        work->goalYaw = Math_AngleXZ(&em->posX, em->goal);
        work->goalYaw = work->goalYaw - em->rotY;
        work->flyGuard = 0;
        em->timer = 0x96;
        break;
    case 1:
        EmDiablos_MoveByVelYCurve(em, 1);
        done = Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 1) & 0xFF;
        if (--em->timer > 0) {
            if (done != 0) {
                goto block_9;
            }
        } else {
block_9:
            em->step++;
            EmDiablos_SetUnderground(em, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmDiablos_State2_08(Em *em, EmWork *work)
{
    EmWork_4E *w = (EmWork_4E *) work;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 4;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x11, 0, 0x64);
        w->cnt = 2;
        break;
    case 1:
        if (--w->cnt == 0) {
            CamShake_ReqAIf(em, 5);
            w->cnt = 8;
        }
        if (em->hasGoal != 0) {
            if (Em_StepToGoal(em, 0x40, 1) == 1) {
                em->step++;
                EmDiablos_EndState2(em);
                break;
            }
        } else {
            if (--em->timer <= 0) {
                em->step++;
                EmDiablos_EndState2(em);
            }
        }
        break;
    }
}

void EmDiablos_State2_09(Em *em, EmWork *work)
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
            EmDiablos_SetUnderground(em, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmDiablos_State2_0A(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 4;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 1, 0, 0);
        em->timer = 0x5A;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmDiablos_EndState2(em);
        }
        break;
    }
}

void EmDiablos_State2_0B(Em *em, EmWork *work)
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
        EmDiablos_MoveByVelYCurve(em, 1);
        dist = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer > 0) {
            if (dist <= (10.0f * em->velZ)) {
                goto block_9;
            }
        } else {
block_9:
            em->step++;
            Player_ClearVelocity(em);
            EmDiablos_SetUnderground(em, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmDiablos_State2_0C(Em *em, EmWork *work)
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
        if (!((630.0f + em->floorY) <= em->posY) && ((EmMotion_PassedFrameScaled(em, 10.0f, 0U) != 0) || (EmMotion_PassedFrameScaled(em, 86.0f, 0U) != 0) || (EmMotion_PassedFrameScaled(em, 160.0f, 0U) != 0))) {
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
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmDiablos_State2_0E(Em *em, EmWork *work)
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
            EmDiablos_SetUnderground(em, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmDiablos_State2_17(Em *em, EmWork *work)
{
    f32 floorY;
    f32 velY;
    f32 y;

    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x12, 0xA, 0x60);
        EmDiablos_StartCurves(em, 2U);
        em->stance = 2;
        em->noReact = 1;
        break;
    case 1:
        EmDiablos_UpdateCurves(em);
        if (em->motion[0].playing == 0) {
            em->step++;
        }
        break;
    case 2:
        if ((EmDiablos_UpdateCurves(em) & 0xFF) && (em->posY <= (800.0f + em->floorY))) {
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
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
    y = em->floorY;
    if (em->posY < y) {
        em->posY = y;
    }
}

void EmDiablos_State3_00(Em *em, EmWork *work)
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
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDiablos_State3_03(Em *em, EmWork *work)
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
            EmDiablos_State3_End(em, work);
        }
        break;
    }
}

void EmDiablos_State3_06(Em *em, EmWork *work)
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
            EmDiablos_State3_End(em, work);
        }
        break;
    }
}

void EmDiablos_State3_0F(Em *em, EmWork *work)
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
                Em_SetMotion(em, 0x64, 0, 0);
                em->step++;
                break;
            }
            if (EmDiablos_ChargeHitWall(em) != 0) {
                EmDiablos_ChangeState(em, 0, 2, 1);
                break;
            }
        } else {
            if (--em->timer <= 0) {
                Em_SetMotion(em, 0x64, 0, 0);
                em->step++;
            }
            break;
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDiablos_State3_10(Em *em, EmWork *work)
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
                Em_SetMotion(em, 0x6A, 0, 0);
                em->step++;
                break;
            }
            if (EmDiablos_ChargeHitWall(em) != 0) {
                EmDiablos_ChangeState(em, 0, 2, 1);
                break;
            }
        } else {
            if (--em->timer <= 0) {
                Em_SetMotion(em, 0x6A, 0, 0);
                em->step++;
            }
            break;
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDiablos_State3_11(Em *em, EmWork *work)
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
                Em_SetMotion(em, 0x6B, 0, 0);
                em->step++;
                break;
            }
            if (EmDiablos_ChargeHitWall(em) != 0) {
                EmDiablos_ChangeState(em, 0, 2, 1);
                break;
            }
        } else {
            if (--em->timer <= 0) {
                Em_SetMotion(em, 0x6B, 0, 0);
                em->step++;
            }
            break;
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDiablos_State3_1A(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        if (Em_TurnToTargetMotion(em, 0xE38, 3, 6, 5) != 0) {
            em->step++;
            em->stance = 0;
            em->lookAtTarget = 0;
            Em_SetMotion(em, 0x6A, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDiablos_State3_1B(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        if (Em_TurnToTargetMotion(em, 0xE38, 3, 6, 5) != 0) {
            em->step++;
            em->stance = 0;
            em->lookAtTarget = 0;
            Em_SetMotion(em, 0x6B, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDiablos_State3_1C(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x6C, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDiablos_State3_1D(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        if (Em_TurnToTargetMotion(em, 0xE38, 3, 6, 5) != 0) {
            em->step++;
            em->stance = 0;
            em->lookAtTarget = 0;
            Em_SetMotion(em, 0x6D, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDiablos_State3_1E(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x6F, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDiablos_State4_00(Em *em)
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
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDiablos_State4_01(Em *em)
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
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDiablos_State4_02(Em *em)
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
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDiablos_State4_03(Em *em)
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
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDiablos_State4_04(Em *em)
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
        if ((EmMotion_PassedFrameScaled(em, 6.0f, 0U) != 0) && ((s8) em->meshShown[1] != 0)) {
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
            EmDiablos_ChangeState(em, 4, 0xF, 3);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmDiablos_ChangeState(em, 4, 0xF, 3);
        }
        break;
    }
}

void EmDiablos_State4_05(Em *em, EmWork *work, u8 variant)
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
            Em_SetMotion(em, 0x45, 0, 0);
        } else {
            Em_SetMotion(em, 0x4A, 0, 0);
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
                Em_SetMotion(em, 0x46, 0, 0);
            } else {
                Em_SetMotion(em, 0x4B, 0, 0);
            }
            if (k != 3 && k != 2) {
                em->timer = 120;
            }
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            if ((variant & 1) == 0) {
                Em_SetMotion(em, 0x47, 0, 0);
            } else {
                Em_SetMotion(em, 0x48, 0, 0);
            }
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
            switch (variant) {
            case 0:
            default:
            case 1:
                EmDiablos_EndState(em, 0, 0);
                break;
            case 2:
            case 3:
                em->step++;
                EmDiablos_ChangeState(em, 0, 0x23, 4);
                break;
            }
        }
        break;
    case 4:
        if (em->remote == 0) {
            EmDiablos_ChangeState(em, 0, 0x23, 4);
        }
        break;
    }
}

void EmDiablos_State4_06(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 4;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x69, 0, 0);
        em->noReact = 1;
        break;
    case 1:
        if (em->motion[0].blend != 0) {
            em->stance = 4;
        } else {
            em->stance = 0;
        }
        if (em->motion[0].playing == 0) {
            em->stance = 0;
            Em_SetMotion(em, 0x60, 0, 0);
            em->trapTimer = 0xD0;
            Em_SetTrapTimeGoal(em);
            em->step++;
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            Em_SetMotion(em, 0x60, 0, 0);
        }
        if (--em->trapTimer <= 0) {
            em->step++;
            em->noReact = 0;
            EmDiablos_ChangeState(em, 2, 0x17, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmDiablos_ChangeState(em, 2, 0x17, 4);
        }
        if (em->motion[0].playing == 0) {
            Em_SetMotion(em, 0x60, 0, 0);
        }
        break;
    }
}

void EmDiablos_State4_07(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x71, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDiablos_State4_08(Em *em, EmWork *work)
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
            EmDiablos_EndState(em, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    }
}

void EmDiablos_State4_09(Em *em, EmWork *work)
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
            Em_SetMotion(em, 0x47, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmDiablos_EndState(em, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    }
}

void EmDiablos_State4_0A(Em *em)
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
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDiablos_State4_0B(Em *em)
{
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
            EmDiablos_ChangeState(em, 0, 0x21, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmDiablos_ChangeState(em, 0, 0x21, 4);
        }
        break;
    }
    Em_SpawnStatus4Effect(em, 2);
}

void EmDiablos_State4_0D(Em *em)
{
    em->holdTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x60, 0, 0);
        Em_RestartScript(em);
        em->noReact = 1;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            Em_SetMotion(em, 0x60, 0, 0);
        }
        if (--em->trapTimer <= 0) {
            em->step++;
            em->unk8AD = 0;
            em->noReact = 0;
            EmDiablos_ChangeState(em, 2, 0x17, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmDiablos_ChangeState(em, 2, 0x17, 4);
        }
        if (em->motion[0].playing == 0) {
            Em_SetMotion(em, 0x60, 0, 0);
        }
        break;
    }
}

void EmDiablos_State4_0E(Em *em, EmWork *work, u8 variant)
{
    s32 k = variant;

    if (k == 0) {
        Em_SetHitReactMode(em, EM_HIT_REACT_STATUS);
    }
    switch (em->step) {
    case 0:
        em->step++;
        if (EmDiablos_InMotion448To44A(em) == 1) {
            em->stance = 0;
            em->step++;
            Em_SetMotion(em, 0x61, 0, 0);
            if (k == 1) {
                StageObj_Stun_CreateEm(em, 1, 0x22, (f32) em->timer + em->motion[0].frameCount / 2.0f, 150.0f, 3.0f);
            }
        } else {
            em->stance = 4;
            if (em->motionId[0] != 0x451) {
                Em_SetMotion(em, 0x69, 0, 0);
            }
        }
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        em->noReact = 1;
        break;
    case 1:
        if (em->motion[0].blend != 0) {
            em->stance = 4;
        } else {
            em->stance = 0;
        }
        if (em->motion[0].playing == 0) {
            em->stance = 0;
            em->step++;
            Em_SetMotion(em, 0x61, 0, 0);
            if (k == 1) {
                StageObj_Stun_CreateEm(em, 1, 0x22, (f32) em->timer + em->motion[0].frameCount / 2.0f, 150.0f, 3.0f);
            }
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetTrapTimeGoal(em);
            switch (k) {
            default:
                EmDiablos_ChangeState(em, 4, 0x11, 3);
                break;
            case 1:
                EmDiablos_ChangeState(em, 4, 0x17, 3);
                break;
            }
        }
        break;
    case 3:
        if (em->remote == 0) {
            switch (k) {
            case 0:
            default:
                EmDiablos_ChangeState(em, 4, 0x11, 3);
                break;
            case 1:
                EmDiablos_ChangeState(em, 4, 0x17, 3);
                break;
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

void EmDiablos_State4_0F(Em *em)
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
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDiablos_State4_11(Em *em)
{
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
            EmDiablos_ChangeState(em, 2, 0x17, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmDiablos_ChangeState(em, 2, 0x17, 4);
        }
        if (em->motion[0].playing == 0) {
            Em_SetMotion(em, 0x60, 0, 0);
        }
        break;
    }
}

void EmDiablos_State4_12(Em *em)
{
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
            Em_SetTrapTimeGoal(em);
            EmDiablos_ChangeState(em, 4, 0xD, 3);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmDiablos_ChangeState(em, 4, 0xD, 3);
        }
        break;
    }
}

void EmDiablos_State4_13(Em *em)
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
            EmDiablos_ChangeState(em, 0, 0x22, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmDiablos_ChangeState(em, 0, 0x22, 4);
        }
        break;
    }
}

void EmDiablos_State6_00(Em *em, EmWork *work)
{
    f32 d;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        GmEm_ResetSize(em);
        em->step++;
        em->stance = 0;
        em->posX = 6660.0f;
        em->posY = 50.0f;
        em->posZ = 5737.0f;
        em->rotX = 0;
        em->rotY = 0x2400;
        em->rotZ = 0;
        Em_SetMotion(em, 1, 0, 0);
        em->timer = 300;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x33, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x6F, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x67, 0, 0);
        }
        break;
    case 4:
        if (EmMotion_ReachedFrame(em, 0, 100.0f) != 0) {
            em->hideShadowHold = 2;
            em->hideShadow = 2;
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stance = 4;
            em->goal[0] = 11000.0f;
            em->goal[1] = 50.0f;
            em->goal[2] = 8813.0f;
            em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
            em->timer = 300;
            Em_SetMotion(em, 0x11, 0, 100);
        }
        break;
    case 5:
        d = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer <= 0 || d <= 200.0f) {
            em->step++;
            Em_SetMotion(em, 1, 0, 0);
        }
        break;
    case 6:
        if (Save_TestEventFlag(0xA8) == 1) {
            em->step++;
            EmDiablos_EndState2(em);
        }
        break;
    }
}

void EmDiablos_State6_01(Em *em, EmWork *work)
{
    f32 d;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        GmEm_ResetSize(em);
        em->step++;
        em->stance = 4;
        em->posX = 8492.0f;
        em->posY = 50.0f;
        em->posZ = 5247.0f;
        em->goal[0] = 6040.0f;
        em->goal[1] = 50.0f;
        em->goal[2] = 8236.0f;
        em->rotX = 0;
        em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        em->rotZ = 0;
        Em_SetMotion(em, 1, 0, 0);
        em->timer = 500;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stance = 4;
            em->goal[0] = 5237.0f;
            em->goal[1] = 50.0f;
            em->goal[2] = 10800.0f;
            em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
            em->timer = 300;
            Em_SetMotion(em, 0x11, 0, 100);
        }
        break;
    case 2:
        d = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer <= 0 || d <= 200.0f) {
            em->step++;
            Em_SetMotion(em, 0x68, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].blend != 0) {
            em->stance = 4;
        } else {
            em->stance = 0;
            if (EmMotion_ReachedFrame(em, 0, 100.0f) == 0) {
                em->hideShadowHold = 2;
                em->hideShadow = 2;
            }
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x6F, 0, 0);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->posX = 7100.0f;
            em->posY = 50.0f;
            em->posZ = 13545.0f;
            em->goal[0] = 10000.0f;
            em->goal[1] = 50.0f;
            em->goal[2] = 9100.0f;
            em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
            Em_SetMotion(em, 0x11, 0, 0);
        }
        break;
    case 5:
        d = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer <= 0 || d <= 200.0f) {
            em->step++;
            Em_SetMotion(em, 1, 0, 0);
        }
        break;
    case 6:
        if (Save_TestEventFlag(0xAA) == 1) {
            em->step++;
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDiablos_State6_02_Captured(Em *em, EmWork *work)
{
    s32 timer;
    s32 t;

    Em_SetCaptured(em, 1);
    switch (em->step) {
    case 0:
        em->step++;
        QuestEm_Captured(em);
        em->timer = 0x960;
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
                EmDiablos_ChangeState(em, 5, 3, 4);
                break;
            }
        } else {
            em->alpha = (f32) timer / 150.0f;
        }
        break;
    }
}

void EmDiablos_State8_00_CarveStart(Em *em, EmWork *work)
{
    EmDiablosVec v;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->stance = 0;
        if (Versus_DefeatCamera(em) == 1) {
            v = EmDiablos_CarveThrowPt;
            StageObj_K25_Create(&gPlayers[(u8) em->side - 1], em, 0, 0, &v.x);
            em->step++;
            Em_SetMotion(em, 0x24, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 30;
            Em_SetMotion(em, 0x33, 0, 0);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            Versus_RouteEmByTeam(em);
        }
        break;
    }
}

void EmDiablos_State8_01(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x3D, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 1, 0, 0);
            EmDiablos_ChangeState(em, 8, 0x98, 1);
        }
        break;
    }
}

void EmDiablos_State8_02(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x36, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 1, 0, 0);
            EmDiablos_ChangeState(em, 8, 0x98, 1);
        }
        break;
    }
}

void EmDiablos_State8_03(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x6F, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 1, 0, 0);
            EmDiablos_ChangeState(em, 8, 0x98, 1);
        }
        break;
    }
}

void EmDiablos_State8_98(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        if (em->motionId[0] != 0x3E9) {
            Em_SetMotion(em, 1, 0, 0);
        }
        em->stance = 0;
        
    case 1:
        if (Versus_BothDefeated() == 1) {
            em->step++;
            EmDiablos_ChangeState(em, 8, 0x99, 1);
        }
        break;
    }
}

void EmDiablos_State8_99(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
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
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDiablos_State5_00_Hunted(Em *em, EmWork *work)
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
                EmDiablos_ChangeState(em, 5, 3, 4);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    case 99:
        if (em->motion[0].playing == 0) {
            em->stance = 0;
            EmDiablos_EndState(em, 0, 0);
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
        }
        break;
    }
}

void EmDiablos_State5_01_Hunted(Em *em, EmWork *work)
{
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        if (EmDiablos_InMotion448To44A(em) == 1) {
            QuestEm_Hunted(em);
            em->stance = 3;
            em->step++;
            Em_SetMotion(em, 0x61, 0, 0);
        } else {
            em->stance = 4;
            if (em->motionId[0] != 0x451) {
                Em_SetMotion(em, 0x69, 0, 0);
            }
        }
        em->lookAtTarget = 0;
        break;
    case 1:
        if (em->motion[0].blend != 0) {
            em->stance = 4;
        } else {
            em->stance = 3;
        }
        if (em->motion[0].playing == 0) {
            em->stance = 3;
            QuestEm_Hunted(em);
            em->step++;
            Em_SetMotion(em, 0x61, 0, 0);
        }
        break;
    case 2:
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
                em->timer = 150;
                EmMine_Remove(em);
            }
        }
        break;
    case 4:
        if (em->timer > 0) {
            em->timer--;
        }
        if (em->timer <= 0) {
            if (em->remote == 0) {
                EmDiablos_ChangeState(em, 5, 3, 4);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    }
}

void EmDiablos_State5_02_HuntedFalling(Em *em, EmWork *work)
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
                em->timer = 150;
                EmMine_Remove(em);
            }
        }
        break;
    case 5:
        if (em->timer > 0) {
            em->timer--;
        }
        if (em->timer <= 0) {
            if (em->remote == 0) {
                EmDiablos_ChangeState(em, 5, 3, 4);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    }
}

void EmDiablos_State5_03_Appear(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        if (QuestEm_CanAppear(em) == 1) {
            em->posY = gAreaGroundY[em->area];
            em->stance = 2;
            EmDiablos_ChangeState(em, 2, 0xC, 0);
            break;
        }
        em->taskPhase++;
    case 1:
        break;
    }
}

void EmDiablos_RunGroup0(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmDiablos_State0_00(em, work);
        break;
    case 1:
        EmDiablos_State0_01(em, work);
        break;
    case 2:
        EmDiablos_State0_02(em, work);
        break;
    case 3:
        EmDiablos_State0_03(em, work);
        break;
    case 4:
        EmDiablos_State0_04(em, work);
        break;
    case 5:
        EmDiablos_State0_05(em, work);
        break;
    case 6:
        EmDiablos_State0_06(em, work);
        break;
    case 7:
        EmDiablos_State0_07(em, work);
        break;
    case 8:
        EmDiablos_State0_08();
        break;
    case 9:
        EmDiablos_State0_09(em, work);
        break;
    case 10:
        EmDiablos_State0_0A(em, work);
        break;
    case 11:
        EmDiablos_State0_0B(em, work);
        break;
    case 12:
        EmDiablos_State0_0C();
        break;
    case 13:
        EmDiablos_State0_0D(em, work);
        break;
    case 14:
        EmDiablos_State0_0E();
        break;
    case 15:
        EmDiablos_State0_0F(em, work);
        break;
    case 16:
        EmDiablos_State0_10(em, work);
        break;
    case 17:
        EmDiablos_State0_11(em, work);
        break;
    case 18:
        EmDiablos_State0_12(em, work);
        break;
    case 19:
        EmDiablos_State0_13();
        break;
    case 20:
        EmDiablos_State0_14(em, work);
        break;
    case 21:
        EmDiablos_State0_15(em, work);
        break;
    case 22:
        EmDiablos_State0_16(em, work);
        break;
    case 23:
        EmDiablos_State0_17(em, work);
        break;
    case 24:
        EmDiablos_State0_18(em, work);
        break;
    case 25:
        EmDiablos_State0_19(em, work);
        break;
    case 26:
        EmDiablos_State0_1A();
        break;
    case 27:
        EmDiablos_State0_1B(em, work);
        break;
    case 28:
        EmDiablos_State0_18(em, work);
        break;
    case 29:
        EmDiablos_State0_1D(em, work);
        break;
    case 33:
        EmDiablos_State0_19(em, work);
        break;
    case 34:
        EmDiablos_State0_19(em, work);
        break;
    case 35:
        EmDiablos_State0_19(em, work);
        break;
    case 36:
        EmDiablos_State0_11(em, work);
        break;
    case 37:
        EmDiablos_State0_11(em, work);
        
    default:
        break;
    }
}

void EmDiablos_RunGroup1(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmDiablos_State1_00(em, work);
        break;
    case 1:
        EmDiablos_State1_01(em, work);
        break;
    case 2:
        EmDiablos_State1_02(em, work);
        break;
    case 3:
        EmDiablos_State1_03(em, work);
        break;
    case 4:
        EmDiablos_State1_04(em, work);
        break;
    case 5:
        EmDiablos_State1_05(em, work);
        break;
    case 6:
        EmDiablos_State1_06(em, work);
        break;
    case 7:
        EmDiablos_State1_07(em, work);
        break;
    case 8:
        EmDiablos_State1_04(em, work);
        
    default:
        break;
    }
}

void EmDiablos_RunGroup2(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmDiablos_State2_00(em, work);
        break;
    case 1:
        EmDiablos_State2_01(em, work);
        break;
    case 2:
        EmDiablos_State2_02(em, work);
        break;
    case 3:
        EmDiablos_State2_03(em, work);
        break;
    case 4:
        EmDiablos_State2_04(em, work);
        break;
    case 5:
        EmDiablos_State2_05(em, work);
        break;
    case 6:
        EmDiablos_State2_06(em, work);
        break;
    case 7:
        EmDiablos_State2_07(em, work);
        break;
    case 8:
        EmDiablos_State2_08(em, work);
        break;
    case 9:
        EmDiablos_State2_09(em, work);
        break;
    case 10:
        EmDiablos_State2_0A(em, work);
        break;
    case 11:
        EmDiablos_State2_0B(em, work);
        break;
    case 12:
        EmDiablos_State2_0C(em, work);
        break;
    case 14:
        EmDiablos_State2_0E(em, work);
        break;
    case 23:
        EmDiablos_State2_17(em, work);
        break;
    }
}

void EmDiablos_RunGroup3(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmDiablos_State3_00(em, work);
        break;
    case 3:
        EmDiablos_State3_03(em, work);
        break;
    case 6:
        EmDiablos_State3_06(em, work);
        break;
    case 15:
        EmDiablos_State3_0F(em, work);
        break;
    case 16:
        EmDiablos_State3_10(em, work);
        break;
    case 17:
        EmDiablos_State3_11(em, work);
        break;
    case 26:
        EmDiablos_State3_1A(em, work);
        break;
    case 27:
        EmDiablos_State3_1B(em, work);
        break;
    case 28:
        EmDiablos_State3_1C(em, work);
        break;
    case 29:
        EmDiablos_State3_1D(em, work);
        break;
    case 30:
        EmDiablos_State3_1E(em, work);
        break;
    case 31:
        EmDiablos_State3_1C(em, work);
        break;
    }
}

void EmDiablos_RunGroup4(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmDiablos_State4_00(em);
        break;
    case 1:
        EmDiablos_State4_01(em);
        break;
    case 2:
        EmDiablos_State4_02(em);
        break;
    case 3:
        EmDiablos_State4_03(em);
        break;
    case 4:
        EmDiablos_State4_04(em);
        break;
    case 5:
        EmDiablos_State4_05(em, work, 0);
        break;
    case 6:
        EmDiablos_State4_06(em);
        break;
    case 7:
        EmDiablos_State4_07(em);
        break;
    case 8:
        EmDiablos_State4_08(em, work);
        break;
    case 9:
        EmDiablos_State4_09(em, work);
        break;
    case 0xA:
        EmDiablos_State4_0A(em);
        break;
    case 0xB:
        EmDiablos_State4_0B(em);
        break;
    case 0xD:
        EmDiablos_State4_0D(em);
        break;
    case 0xE:
        EmDiablos_State4_0E(em, work, 0);
        break;
    case 0xF:
        EmDiablos_State4_0F(em);
        break;
    case 0x10:
        EmDiablos_State4_05(em, work, 1);
        break;
    case 0x11:
        EmDiablos_State4_11(em);
        break;
    case 0x12:
        EmDiablos_State4_12(em);
        break;
    case 0x13:
        EmDiablos_State4_13(em);
        break;
    case 0x14:
        EmDiablos_State4_05(em, work, 2);
        break;
    case 0x15:
        EmDiablos_State4_05(em, work, 3);
        break;
    case 0x16:
        EmDiablos_State4_0E(em, work, 1);
        break;
    case 0x17:
        EmDiablos_State4_11(em);
        break;
    }
}

void EmDiablos_RunGroup5(Em *em, EmWork *work)
{
    em->noPushTimer = 5;
    switch (em->state) {
    case 0:
        EmDiablos_State5_00_Hunted(em, work);
        break;
    case 1:
        EmDiablos_State5_01_Hunted(em, work);
        break;
    case 2:
        EmDiablos_State5_02_HuntedFalling(em, work);
        break;
    case 3:
        EmDiablos_State5_03_Appear(em, work);
        break;
    }
}

void EmDiablos_RunGroup6(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmDiablos_State6_00(em, work);
        break;
    case 1:
        EmDiablos_State6_01(em, work);
        break;
    case 2:
        EmDiablos_State6_02_Captured(em, work);
        break;
    }
}

void EmDiablos_RunGroup8(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0x0:
        EmDiablos_State8_00_CarveStart(em, work);
        break;
    case 0x1:
        EmDiablos_State8_01(em, work);
        break;
    case 0x2:
        EmDiablos_State8_02(em, work);
        break;
    case 0x3:
        EmDiablos_State8_03(em, work);
        break;
    case 0x98:
        EmDiablos_State8_98(em, work);
        break;
    case 0x99:
        EmDiablos_State8_99(em, work);
        break;
    }
}

void EmDiablos_Update(Em *em)
{
    u8 buf[4];
    EmDiablosWork *w = (EmDiablosWork *)&em->work;

    if (em->rage != 0) {
        switch (em->rageStep) {
        case 0:
            if (++w->rageFade > 60) {
                w->rageFade = 60;
                em->rageStep++;
            }
            break;
        case 1:
            w->rageFade = 60;
            break;
        }
    } else {
        em->rageStep = 0;
        if (--w->rageFade < 0) {
            w->rageFade = 0;
        }
    }
    Em_UpdateCombatTimer(em);
    Em_RaisePlayerAreaTriggerLax(em);
    if (em->stance == 4) {
        Em_HoldPosNoPlayers(em, 0, 3);
    } else {
        Em_HoldPosNoPlayers(em, 0, 1);
    }
    if (EmDiablos_ShouldEndState(em) == 1) {
        EmDiablos_EndState(em, 0, 0);
    }
    if (w->fxMotion != 0) {
        w->fxMotion -= 1;
    }
    switch (Em_UpdateDamage(em, buf) & 0xFF) {
    case 0:
    case 3:
    case 4:
    case 9:
    case 14:
        break;
    case 1:
    case 2:
        if (em->stance == 2) {
            EmDiablos_ChangeState(em, 5, 2, 3);
        } else if (em->stance == 4 || EmDiablos_InSandState(em) == 1) {
            EmDiablos_ChangeState(em, 5, 1, 3);
        } else {
            EmDiablos_ChangeState(em, 5, 0, 3);
        }
        break;
    case 17:
        if (em->stateGroup != 4 || em->state != 0x16) {
            if (em->stance == 4 || EmDiablos_InSandState(em) == 1) {
                Em_SetStatus5Timer(em);
                EmDiablos_ChangeState(em, 4, 0x16, 3);
            } else if ((em->stateGroup != 4 || em->state != 0x14) && (em->stateGroup != 4 || em->state != 0x15) &&
                       (em->stateGroup != 4 || em->state != 8)) {
                Em_SetStatus5Timer(em);
                if (((em->hitAngle - em->rotY) & 0xFFFF) > 0x8000) {
                    EmDiablos_ChangeState(em, 4, 0x14, 3);
                } else {
                    EmDiablos_ChangeState(em, 4, 0x15, 3);
                }
            }
        }
        break;
    case 18:
        if (EmDiablos_InSandState(em) == 0) {
            Em_ResetKindTimer(em);
            EmDiablos_ChangeState(em, 4, 0x13, 3);
        }
        break;
    case 15:
        if (em->rage == 0 && em->stance == 4 && (em->stateGroup != 4 || em->state != 6)) {
            EmDiablos_ChangeState(em, 4, 6, 3);
        }
        break;
    case 5:
        if (em->stateGroup != 4 || em->state != 8) {
            EmDiablos_ChangeState(em, 4, 8, 3);
        }
        break;
    case 6:
        if (em->stateGroup != 4 || em->state != 0xE) {
            if (em->stance == 4 || EmDiablos_InSandState(em) == 1) {
                Em_SetStatus4Timer(em);
                EmDiablos_ChangeState(em, 4, 0xE, 3);
            } else if ((em->stateGroup != 4 || em->state != 0xB) && (em->stateGroup != 4 || em->state != 8)) {
                Em_SetStatus4Timer(em);
                EmDiablos_ChangeState(em, 4, 0xB, 3);
            }
        }
        break;
    case 7:
        if ((s8) em->holdTimer2 != 0) {
            EmDiablos_ChangeState(em, 6, 2, 3);
        } else if (em->stateGroup != 0 || em->state != 0x1D) {
            if (em->stance == 4 || EmDiablos_InSandState(em) == 1) {
                Em_SetStatus1Timer(em);
                EmDiablos_ChangeState(em, 0, 0x1D, 3);
            } else if ((em->stateGroup != 0 || em->state != 0x1B) && (em->stateGroup != 4 || em->state != 8)) {
                Em_SetStatus1Timer(em);
                EmDiablos_ChangeState(em, 0, 0x1B, 3);
            }
        }
        break;
    case 8:
        if (em->stateGroup != 0 || em->state != 0x1D) {
            if (em->stance == 4 || EmDiablos_InSandState(em) == 1) {
                Em_SetStatus2Timer(em);
                EmDiablos_ChangeState(em, 0, 0x1D, 3);
            } else if ((em->stateGroup != 0 || em->state != 0x14) && (em->stateGroup != 4 || em->state != 8)) {
                Em_SetStatus2Timer(em);
                EmDiablos_ChangeState(em, 0, 0x14, 3);
            }
        }
        break;
    case 10:
        switch (em->state) {
        case 0x14:
            EmDiablos_ChangeState(em, 0, 0x18, 3);
            em->scriptPending = 0;
            break;
        case 0x1B:
            EmDiablos_ChangeState(em, 0, 0x1C, 3);
            em->scriptPending = 0;
            break;
        case 0x1D:
            EmDiablos_ChangeState(em, 4, 0x12, 3);
            em->scriptPending = 0;
            break;
        }
        break;
    case 11:
        EmDiablos_ChangeState(em, 4, 4, 3);
        break;
    case 16:
        if (em->rage == 0 && em->stance == 4 && (em->stateGroup != 4 || em->state != 6)) {
            EmDiablos_ChangeState(em, 4, 6, 3);
            break;
        }
    case 12:
        Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
        if (em->stance == 2) {
            EmDiablos_ChangeState(em, 4, 8, 3);
        } else if (em->stance == 4) {
            EmDiablos_ChangeState(em, 4, 6, 3);
        } else {
            switch (em->flinchPart) {
            case 0:
            case 7:
                EmDiablos_ChangeState(em, 4, 0, 3);
                break;
            case 6:
                if (em->type == 0xE || em->type == 0x2B) {
                    if (em->parts[em->flinchPart].state == 1 || em->parts[em->flinchPart].state == 2) {
                        EmDiablos_ChangeState(em, 4, 7, 3);
                    } else {
                        EmDiablos_ChangeState(em, 4, 2, 3);
                    }
                } else if (em->parts[em->flinchPart].state == 2) {
                    EmDiablos_ChangeState(em, 4, 7, 3);
                } else {
                    EmDiablos_ChangeState(em, 4, 2, 3);
                }
                break;
            case 5:
                EmDiablos_ChangeState(em, 4, 2, 3);
                break;
            case 1:
            case 2:
                EmDiablos_ChangeState(em, 4, 3, 3);
                break;
            case 3:
            case 4:
            default:
                if (em->parts[em->flinchPart].state >= 2) {
                    if (em->flinchPart != 3) {
                        EmDiablos_ChangeState(em, 4, 5, 3);
                    } else {
                        EmDiablos_ChangeState(em, 4, 0x10, 3);
                    }
                } else {
                    EmDiablos_ChangeState(em, 4, 1, 3);
                }
                break;
            }
        }
        break;
    case 13:
        if (em->stance != 2 && em->stance != 4) {
            EmDiablos_ChangeState(em, 4, 0, 3);
            em->scriptPending = 0;
        }
        break;
    }
    switch (gQuest.stage) {
    case 0x406:
        if ((em->type == 0xE || em->type == 0x2B) && Area_DayId(em->area) == AREA_DESERT_2 && Save_TestEventFlag(0xA8) == 0) {
            if (Area_DayId(gScene.area) == AREA_DESERT_2 && gScene.eventRunning == 1 && em->stateGroup != 6) {
                EmDiablos_ChangeState(em, 6, 0, 1);
            }
            break;
        }
        goto normal;
    case 0x1390:
        if ((em->type == 0x1A || em->type == 0x2C) && Area_DayId(em->area) == AREA_DESERT_2 && Save_TestEventFlag(0xAA) == 0) {
            if (Area_DayId(gScene.area) == AREA_DESERT_2 && gScene.eventRunning == 1 && em->stateGroup != 6) {
                EmDiablos_ChangeState(em, 6, 1, 1);
            }
            break;
        }
    default:
    normal:
        Em_UpdatePendingScript(em);
        break;
    }
    Em_ClearHitReactMode(em);
    EmDiablos_RunState(em, (EmWork *) w);
    if (em->stateRequest != 0) {
        EmDiablos_RunState(em, (EmWork *) w);
        em->stateRequest = 0;
    }
    if (em->surfaceKind != 0 && gScene.area == em->area) {
        if (em->stance == 4) {
            f32 t = em->surfaceY - *(f32 *) &em->surfaceDepth;
            if (!(em->posY <= t)) {
                em->posY = t;
            }
            t = em->surfaceY - 1000.0f;
            if (!(em->floorY < t)) {
                em->floorY = t;
            }
            if (em->posY < em->floorY) {
                em->posY = em->floorY;
            }
        }
        if (em->stance == 0 || em->stance == 3) {
            em->floorY = em->surfaceY;
            if (em->posY < em->floorY) {
                em->posY = em->floorY;
            }
        }
    }
}

void EmDiablos_RunState(Em *em, EmWork *work)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->stateGroup) {
    case 0:
        EmDiablos_RunGroup0(em, work);
        break;
    case 1:
        EmDiablos_RunGroup1(em, work);
        break;
    case 2:
        EmDiablos_RunGroup2(em, work);
        break;
    case 3:
        EmDiablos_RunGroup3(em, work);
        break;
    case 4:
        EmDiablos_RunGroup4(em, work);
        break;
    case 5:
        EmDiablos_RunGroup5(em, work);
        break;
    case 6:
        EmDiablos_RunGroup6(em, work);
        break;
    case 7:
        EmDiablos_RunGroup6(em, work);
        break;
    case 8:
        EmDiablos_RunGroup8(em, work);
        
    default:
        break;
    }
}

void EmDiablos_RequestOnFrame(void *em, s32 frame, s32 i)
{
    if (EmMotion_PassedFrameScaled((Em *) em, (f32) frame, 0U) != 0) {
        CamShake_ReqAIf(em, i);
    }
}

static void EmDiablos_MotionEffectsDefault(Em *em)
{
}

void EmDiablos_MotionEffects(Em *em, EmDiablosFxWork *work)
{
    f32 v3[3];
    f32 v2[3];
    f32 v1[3];
    f32 v0[3];

    if (em->motionId[0] != work->motion) {
        work->motion = em->motionId[0];
    }
    EmDiablos_MotionSounds(em, (struct EmWork *)work);
    switch (work->motion) {
    case 0x3E9:
        break;
    case 0x3EB:
        if (em->stance != 4) {
            EmDiablos_RequestOnFrame(em, 0x34, 1);
            EmDiablos_RequestOnFrame(em, 0x74, 1);
            if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
                DiablosAttack_Create(em, 2);
            }
            if (EmMotion_PassedFrameScaled(em, 50.0f, 0) != 0) {
                DiablosAttack_Create(em, 3);
            }
            if (EmMotion_PassedFrameScaled(em, 120.0f, 0) != 0) {
                DiablosAttack_Create(em, 4);
                break;
            }
        } else {
            break;
        }
        break;
    case 0x3ED:
        if (em->stance != 4) {
            EmDiablos_RequestOnFrame(em, 0x3C, 1);
            if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
                DiablosAttack_Create(em, 5);
            }
            if (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) {
                DiablosAttack_Create(em, 6);
            }
            if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 2.0f, 78.0f) != 0) && !(gScene.frame & 3)) {
                StageObj_K14_Create(em, 0x1A, 1, 1.0f);
                break;
            }
        }
        break;
    case 0x3EE:
        if (em->stance != 4) {
            EmDiablos_RequestOnFrame(em, 0x3C, 1);
            if (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) {
                DiablosAttack_Create(em, 7);
            }
            if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
                DiablosAttack_Create(em, 8);
            }
            if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 2.0f, 78.0f) != 0) && !(gScene.frame & 3)) {
                StageObj_K14_Create(em, 0x1A, 0, 1.0f);
                break;
            }
        }
        break;
    case 0x3EF:
        if (EmMotion_PassedFrameScaled(em, 18.0f, 0) != 0) {
            DiablosAttack_Create(em, 9);
        }
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            DiablosAttack_Create(em, 0xA);
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
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            DiablosAttack_Create(em, 0xB);
        }
        if (EmMotion_PassedFrameScaled(em, 18.0f, 0) != 0) {
            DiablosAttack_Create(em, 0xC);
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
        if (EmMotion_PassedFrameScaled(em, 42.0f, 0) != 0) {
            DiablosAttack_Create(em, 0x19);
        }
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            DiablosAttack_Create(em, 0x1A);
        }
        if (EmMotion_PassedFrameScaled(em, 74.0f, 0) != 0) {
            DiablosAttack_Create(em, 0x1A);
            break;
        }
        break;
    case 0x3F3:
        if (EmMotion_PassedFrameScaled(em, 52.0f, 0) || EmMotion_PassedFrameScaled(em, 104.0f, 0) || EmMotion_PassedFrameScaled(em, 162.0f, 0)) {
            Effect0D_SpawnForEmScaled(em, 2, 7, 5.0f);
            break;
        }
        break;
    case 0x3F4:
        break;
    case 0x3F5:
        if ((EmMotion_PassedFrameScaled(em, 26.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 52.0f, 0) != 0)) {
            Effect0D_SpawnForEmScaled(em, 1, 7, 5.0f);
            break;
        }
        break;
    case 0x3F7:
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
    case 0x3F9:
        if (em->stance == 4) {
            if (gScene.area == em->area) {
                Vec_Set(v3, em->posX, em->surfaceY, em->posZ);
                if (!(gScene.frame & 3)) {
                    Effect0D_SpawnAt(v3, 0x14, 20.0f);
                    Em_PlaySeChecked(em, 0x22, 0, v3, 0x18, 0);
                    break;
                }
            }
        } else {
            if (EmMotion_PassedFrameScaled(em, 106.0f, 0) != 0) {
                DiablosAttack_Create(em, 0xE);
            }
            if ((EmMotion_PassedFrameScaled(em, 100.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 154.0f, 0) != 0)) {
                StageObj_K14_Create(em, 2, 0, 1.0f);
            }
            if ((EmMotion_PassedFrameScaled(em, 88.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 132.0f, 0) != 0)) {
                StageObj_K14_Create(em, 3, 0, 1.0f);
            }
            if (em->rage != 0) {
                if ((EmMotion_PassedFrameScaled(em, 106.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 132.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 158.0f, 0) != 0)) {
                    StageObj_K14_Create(em, 0x19, (Rand_Next(1) & 1) + 2, 1.0f);
                }
                if ((EmMotion_InFrameRange(em, 0, 4.0f, 50.0f) != 0) && !(gScene.frame & 3)) {
                    StageObj_K14_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
                    break;
                }
            }
        }
        break;
    case 0x3FA:
        if (((EmMotion_PassedFrameScaled(em, 74.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 140.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 194.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 240.0f, 0) != 0)) && (em->posY <= (1000.0f + em->floorY))) {
            StageObj_K14_Create(em, 1, 0, 1.0f);
        }
        if (em->rage != 0) {
            if ((EmMotion_InFrameRange(em, 0, 10.0f, 64.0f) != 0) && !(gScene.frame & 3)) {
                StageObj_K14_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
            }
            if (EmMotion_InFrameRange(em, 0, 136.0f, 140.0f) || EmMotion_InFrameRange(em, 0, 188.0f, 192.0f) || EmMotion_InFrameRange(em, 0, 136.0f, 234.0f)) {
                StageObj_K14_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
                break;
            }
        }
        break;
    case 0x3FB:
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            DiablosAttack_Create(em, 0xD);
        }
        if (EmMotion_PassedFrameScaled(em, 16.0f, 0) != 0) {
            StageObj_K14_Create(em, 0xB, 0, 1.0f);
        }
        if (em->rage != 0) {
            if (EmMotion_PassedFrameScaled(em, 14.0f, 0) != 0) {
                StageObj_K14_Create(em, 0x19, 2, 1.0f);
                StageObj_K14_Create(em, 0x19, 3, 1.0f);
            }
            if ((EmMotion_InFrameRange(em, 0, 18.0f, 44.0f) != 0) && !(gScene.frame & 3)) {
                StageObj_K14_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
                break;
            }
        }
        break;
    case 0x405:
        break;
    case 0x407:
        v1[1] = 10.0f;
        v1[2] = 140.0f;
        v1[0] = 0.0f;
        Em_SpawnLyingPuffs(em, 0x22, v1, 1.6f);
        break;
    case 0x408:
    case 0x409:
        break;
    case 0x40A:
        v0[1] = 10.0f;
        v0[2] = 140.0f;
        v0[0] = 0.0f;
        Em_SpawnLyingPuffs(em, 0x22, v0, 1.6f);
        break;
    case 0x40C:
        if (EmMotion_PassedFrameScaled(em, 50.0f, 0) != 0) {
            DiablosAttack_Create(em, 1);
        }
        if (EmMotion_PassedFrameScaled(em, 46.0f, 0) != 0) {
            DiablosAttack_Create(em, 0x1F);
            break;
        }
        break;
    case 0x40D:
        break;
    case 0x40E:
        if (EmMotion_PassedFrameScaled(em, 148.0f, 0) != 0) {
            DiablosAttack_Create(em, 0xF);
        }
        if (EmMotion_PassedFrameScaled(em, 156.0f, 0) != 0) {
            StageObj_K14_Create(em, 0, 0, 1.0f);
            break;
        }
        break;
    case 0x40F:
        break;
    case 0x412:
        EmDiablos_RequestOnFrame(em, 0x2C, 1);
        EmDiablos_RequestOnFrame(em, 0x49, 1);
        if (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) {
            DiablosAttack_Create(em, 0x11);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 6.0f, 38.0f) != 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create(em, 0x1A, 0, 1.0f);
            break;
        }
        break;
    case 0x41B:
        if (EmMotion_PassedFrameScaled(em, 54.0f, 0) != 0) {
            StageObj_K14_Create(em, 2, 2, 0.8f);
        }
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            DiablosAttack_Create(em, 0x1B);
        }
        if (EmMotion_PassedFrameScaled(em, 168.0f, 0) != 0) {
            DiablosAttack_Create(em, 0x1B);
        }
        if (EmMotion_PassedFrameScaled(em, 56.0f, 0) != 0) {
            DiablosAttack_Create(em, 0x1C);
        }
        if (em->rage != 0) {
            if (EmMotion_InFrameRange(em, 0, 156.0f, 248.0f) || EmMotion_InFrameRange(em, 0, 276.0f, 346.0f)) {
                StageObj_K14_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
                break;
            }
        }
        break;
    case 0x41E:
        if (EmMotion_PassedFrameScaled(em, 102.0f, 0) != 0) {
            DiablosAttack_Create(em, 0x12);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 16.0f, 26.0f) != 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
            break;
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
            StageObj_K14_Create(em, 1, 0x80, 0.6f);
            break;
        }
        break;
    case 0x425:
        if (EmMotion_PassedFrameScaled(em, 24.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x1A, 7, 1.0f);
            break;
        }
        break;
    case 0x426:
    case 0x427:
    case 0x428:
        break;
    case 0x429:
        if (EmMotion_PassedFrameScaled(em, 20.0f, 0) != 0) {
            StageObj_K14_Create(em, 3, 0x80, 1.0f);
        }
        if ((EmMotion_PassedFrameScaled(em, 128.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 164.0f, 0) != 0)) {
            StageObj_K14_Create(em, 3, 0x80, 0.6f);
        }
        if (EmMotion_PassedFrameScaled(em, 60.0f, 0) != 0) {
            StageObj_K14_Create(em, 1, 0x80, 1.0f);
            break;
        }
        break;
    case 0x42A:
    case 0x42B:
        break;
    case 0x42C:
        if (EmMotion_PassedFrameScaled(em, 268.0f, 0) != 0) {
            StageObj_K14_Create(em, 0, 0, 0.9f);
            break;
        }
        break;
    case 0x42D:
        EmDiablos_RequestOnFrame(em, 0x1A, 2);
        if (EmMotion_PassedFrameScaled(em, 26.0f, 0) != 0) {
            StageObj_K14_Create(em, 0x12, 0x80, 1.0f);
            break;
        }
        break;
    case 0x42E:
    case 0x433:
    case 0x42F:
    case 0x430:
        break;
    case 0x431:
        EmDiablos_RequestOnFrame(em, 0x1A, 2);
        if ((EmMotion_ReachedFrame(em, 0, 28.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 52.0f) == 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create(em, 0xA, 0x80, 1.0f);
            break;
        }
        break;
    case 0x432:
        EmDiablos_RequestOnFrame(em, 0x1A, 2);
        if (EmMotion_PassedFrameScaled(em, 26.0f, 0) != 0) {
            StageObj_K14_Create(em, 0x12, 0x81, 1.0f);
            break;
        }
        break;
    case 0x434:
    case 0x435:
        break;
    case 0x436:
        if (EmMotion_PassedFrameScaled(em, 8.0f, 0) != 0) {
            StageObj_K14_Create(em, 0xB, 0, 1.0f);
            break;
        }
        break;
    case 0x438:
    case 0x439:
    case 0x43A:
    case 0x43B:
    case 0x442:
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
        if (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0) {
            DiablosAttack_Create(em, 0x16);
        }
        if (EmMotion_PassedFrameScaled(em, 18.0f, 0) != 0) {
            StageObj_K14_Create(em, 2, 6, 1.8f);
        }
        if ((EmMotion_InFrameRange(em, 0, 40.0f, 80.0f) != 0) && !(gScene.frame & 3)) {
            Effect0D_SpawnForEmScaled(em, 2, 3, 7.0f);
        }
        if (em->rage != 0) {
            if (EmMotion_PassedFrameScaled(em, 22.0f, 0) != 0) {
                StageObj_K14_Create(em, 0x19, 2, 1.0f);
                StageObj_K14_Create(em, 0x19, 3, 1.0f);
            }
            if ((EmMotion_InFrameRange(em, 0, 72.0f, 116.0f) != 0) && !(gScene.frame & 3)) {
                StageObj_K14_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
                break;
            }
        }
        break;
    case 0x44F:
        EmDiablos_RequestOnFrame(em, 0x68, 4);
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            DiablosAttack_Create(em, 0x20);
        }
        if (EmMotion_PassedFrameScaled(em, 20.0f, 0) != 0) {
            StageObj_K14_Create(em, 2, 0, 1.2f);
        }
        if (EmMotion_PassedFrameScaled(em, 82.0f, 0) != 0) {
            StageObj_K14_Create(em, 3, 0, 1.2f);
        }
        if (EmMotion_PassedFrameScaled(em, 104.0f, 0) != 0) {
            StageObj_K14_Create(em, 1, 0, 1.0f);
        }
        if ((EmMotion_InFrameRange(em, 0, 40.0f, 62.0f) != 0) && !(gScene.frame & 3)) {
            Effect0D_SpawnForEmScaled(em, 0xF, 3, 5.0f);
        }
        if ((EmMotion_PassedFrameScaled(em, 18.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 22.0f, 0) != 0)) {
            Effect0D_SpawnForEmScaled(em, 9, 3, 5.0f);
        }
        if ((EmMotion_PassedFrameScaled(em, 148.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 160.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 172.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 184.0f, 0) != 0)) {
            Effect0D_SpawnForEmScaled(em, 0x29, 3, 6.0f);
        }
        if (!(em->fxFlags & 1) && (EmMotion_PassedFrameScaled(em, 180.0f, 0) != 0)) {
            Effect0D_SpawnForEmScaled(em, 0x2A, 0x13, 0.7f);
            break;
        }
        break;
    case 0x450:
        EmDiablos_RequestOnFrame(em, 2, 5);
        if (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0) {
            DiablosAttack_Create(em, 0x15);
        }
        if (EmMotion_PassedFrameScaled(em, 40.0f, 0) != 0) {
            DiablosAttack_Create(em, 0x21);
        }
        if (EmMotion_PassedFrameScaled(em, 12.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x22, 0x13, 1.4f);
            if (Player_InCurrentArea(em)) {
                Em_BonePos(em, 0x22, v3);
                v3[1] = em->floorY;
                StageObj_K11_CreateAtScaled(v3, (u16) em->rotY, 8, 1.0f);
            }
        }
        if (em->rage != 0) {
            if (EmMotion_InFrameRange(em, 0, 12.0f, 36.0f) || EmMotion_InFrameRange(em, 0, 66.0f, 130.0f) || EmMotion_InFrameRange(em, 0, 168.0f, 204.0f)) {
                if (!(gScene.frame & 3)) {
                    StageObj_K14_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
                    break;
                }
            }
        }
        break;
    case 0x451:
        EmDiablos_RequestOnFrame(em, 0xC, 3);
        if (EmMotion_PassedFrameScaled(em, 12.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 2, 0x13, 1.4f);
            if (Player_InCurrentArea(em)) {
                Em_BonePos(em, 2, v3);
                v3[1] = em->floorY;
                StageObj_K11_CreateAtScaled(v3, (u16) em->rotY, 8, 1.0f);
                break;
            }
        }
        break;
    case 0x452:
        if (EmMotion_PassedFrameScaled(em, 48.0f, 0) != 0) {
            DiablosAttack_Create(em, 0x18);
        }
        if (EmMotion_PassedFrameScaled(em, 50.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x14, 3, 5.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 54.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x14, 3, 5.0f);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 86.0f, 152.0f) != 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
            break;
        }
        break;
    case 0x453:
        if (EmMotion_PassedFrameScaled(em, 50.0f, 0) != 0) {
            DiablosAttack_Create(em, 0x10);
        }
        if (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x1A, 7, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 44.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x14, 3, 5.0f);
        }
        if (em->rage != 0) {
            if (EmMotion_InFrameRange(em, 0, 8.0f, 24.0f) || EmMotion_InFrameRange(em, 0, 136.0f, 172.0f)) {
                if (!(gScene.frame & 3)) {
                    StageObj_K14_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
                    break;
                }
            }
        }
        break;
    case 0x454:
        if (EmMotion_PassedFrameScaled(em, 46.0f, 0) != 0) {
            DiablosAttack_Create(em, 0x14);
        }
        if (EmMotion_PassedFrameScaled(em, 94.0f, 0) != 0) {
            DiablosAttack_Create(em, 0x1D);
        }
        if (!(em->fxFlags & 1)) {
            if (EmMotion_PassedFrameScaled(em, 54.0f, 0) != 0) {
                Effect0D_SpawnForEmScaled(em, 0x2B, 7, 2.0f);
            }
            if (EmMotion_PassedFrameScaled(em, 104.0f, 0) != 0) {
                Effect0D_SpawnForEmScaled(em, 0x2B, 7, 3.0f);
                break;
            }
        }
        break;
    case 0x455:
        if (EmMotion_PassedFrameScaled(em, 54.0f, 0) != 0) {
            DiablosAttack_Create(em, 0x1E);
        }
        if (EmMotion_PassedFrameScaled(em, 54.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x1A, 3, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 52.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x14, 3, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 56.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x14, 3, 2.0f);
            break;
        }
        break;
    case 0x457:
        if (EmMotion_PassedFrameScaled(em, 112.0f, 0) != 0) {
            DiablosAttack_Create(em, 0x13);
            StageObj_K0F_CreateOnEm(em, 5, 6, 1.0f);
            break;
        }
        break;
    case 0x459:
        break;
    case 0x45A:
        EmDiablos_RequestOnFrame(em, 4, 3);
        EmDiablos_RequestOnFrame(em, 0x68, 0);
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            v2[1] = 30.0f;
            v2[0] = 0.0f;
            v2[2] = 100.0f;
            Vec_RotateSelf((f32 *) v2, (f32 *) (Em_Bone(em, 0x27)));
            Em_BonePos(em, 0x27, v3);
            Vec_Add(v3, v3, v2);
            Effect02_SpawnForOwner(em, 0, 9, 0, v3, 1.0f);
            Effect02_SpawnForOwner(em, 0, 9, 1, v3, 1.0f);
            Effect0D_SpawnIfHere(em, v3, 0x22, 10.0f);
        }
        if (!(gScene.frame & 0x1F)) {
            StageObj_K15_Create(em, 2);
            break;
        }
        break;
    case 0x45B:
        if (EmMotion_PassedFrameScaled(em, 68.0f, 0) != 0) {
            StageObj_K15_Create(em, 2);
            break;
        }
        break;
    default:
        EmDiablos_MotionEffectsDefault(em);
        break;
    }
}

void EmDiablos_UpdateMotionEffects(Em *em)
{
    EmDiablosWork *w = (EmDiablosWork *)&em->work;

    switch (w->fxStep) {
    case 0:
        w->fxStep++;
        break;
    case 1:
        EmDiablos_MotionEffects(em, (EmDiablosFxWork *) w);
        break;
    }
}

void EmDiablos_State3_End(Em *em, EmWork *work)
{
    EmDiablos_EndState(em, 0, 0);
}

void EmDiablos_TableNop(void)
{
}

s32 EmDiablos_AllowHit8(Em *em)
{
    if ((EmDiablos_InSandState(em) == 0) && (em->stance != 2) && ((s8) em->partBreakCount == 0)) {
        return 1;
    }
    return 0;
}

void EmDiablos_SetGroup0State(Em *em, u16 state, u8 mode)
{
    em->motionSpeed = 1.0f;
    switch (state) {
    case 18:
        Em_ResetNotice(em);
        break;
    case 20:
    case 21:
    case 29:
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
    case 33:
        Em_EndStatus4(em);
        break;
    case 34:
        em->holdTimer2 = 0;
        break;
    case 35:
        Em_EndStatus5(em);
        break;
    case 36:
        Em_StartRage(em);
        break;
    case 37:
        Em_StartPendingRage(em);
        break;
    }
    em->stance = 0;
    Em_SetStateSync(em, 0, state, mode);
}

void EmDiablos_SetGroup1State(Em *em, u16 state, u8 mode)
{
    em->stance = 0;
    em->motionSpeed = 1.0f;
    switch (state) {
    case 0:
    case 2:
        if (em->hp < (s16) (0.3f * em->hpMax)) {
            state = 6;
            goto state6;
        }
        if (state == 2 && em->combat == 1) {
            state = 4;
            goto state4;
        }
        Em_SetSpeedScaled(em, 0, 1000.0f, 500.0f);
        if (em->remote == 0 && em->hasGoal == 0) {
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
        em->motionSpeed = 0.7f;
    case 4:
    state4:
        Em_SetSpeedScaled(em, 2, 1000.0f, 500.0f);
        if (em->curGoalKind == 0) {
            em->timer = (em->stateRand % 120 + 180) / 2;
        }
        break;
    case 6:
    state6:
        if (em->curGoalKind == 0) {
            em->hasGoal = 0;
            em->timer = (em->stateRand % 120 + 180) / 2;
        } else {
            em->hasGoal = 1;
            em->goalDist = Vec_DistanceXZ(&em->posX, em->goal);
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

void EmDiablos_SetGroup2State(Em *em, u16 state, u8 mode)
{
    em->motionSpeed = 1.0f;
    switch (state) {
    case 0:
        if (em->surfaceKind == 0 && gScene.area == em->area) {
            EmDiablos_ChangeState(em, 0, 3, 1);
            return;
        }
        break;
    case 4:
    case 8:
        Em_SetSpeedScaled(em, 2, 1000.0f, 500.0f);
        if (em->curGoalKind == 0) {
            em->timer = (em->stateRand % 120 + 180) / 2;
        }
        break;
    case 5:
        Em_SetSpeedScaled(em, 0, 1000.0f, 500.0f);
        if (em->goalKind == 0) {
            em->timer = (em->stateRand % 120 + 180) / 2;
        } else {
            em->timer = (s32) em->goalDist / 7;
        }
        break;
    case 7:
        Player_ClearVelocity(em);
        em->velZ = 20.0f;
        em->timer = (s32) (Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) / em->velZ) * 3;
        break;
    case 9:
        if (em->velZ <= 0.0f) {
            em->velZ = 20.0f;
        }
        if (em->remote == 0) {
            em->timer = Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) / em->velZ;
        }
        break;
    case 11:
        Player_ClearVelocity(em);
        em->velZ = 20.0f;
        em->timer = (s32) (Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) / em->velZ) * 3;
        break;
    case 1:
    case 2:
    case 3:
    case 6:
    case 10:
    case 12:
        break;
    }
    Em_SetStateSync(em, 2, state, mode);
}

void EmDiablos_SetGroup3State(Em *em, u16 state, u8 mode)
{
    f32 f;

    em->motionSpeed = 1.0f;
    switch (state) {
    case 0:
    case 26:
    case 27:
    case 29:
        Em_SetTargetAngle(em);
        break;
    case 3:
    case 6:
        break;
    case 15:
        if (em->targetNo == -1) {
            em->timer = 150;
            em->hasGoal = 0;
        } else {
            em->hasGoal = 1;
            em->goalDist = 500.0f + Vec_DistanceXZ(&em->posX, em->goal);
        }
        break;
    case 16:
    case 17:
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
    }
    Em_SetStateSync(em, 3, state, mode);
}

void EmDiablos_ChangeState(Em *em, u16 group, u16 state, u8 mode)
{
    if (em->remote == 0) {
        Em_TargetPlayerInRange(em);
    } else if (mode == 3 || mode == 4) {
        return;
    }
    Em_LoadReactionSet(em, 0);
    switch (group) {
    case 0:
        EmDiablos_SetGroup0State(em, state, mode);
        break;
    case 1:
        EmDiablos_SetGroup1State(em, state, mode);
        break;
    case 2:
        EmDiablos_SetGroup2State(em, state, mode);
        break;
    case 3:
        EmDiablos_SetGroup3State(em, state, mode);
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
        case 4:
            em->fxFlags |= 1;
            em->partBreakCount++;
            Em_EndAllStatus(em);
            break;
        case 19:
            Em_EndAllStatus(em);
            break;
        case 7:
            break;
        case 11:
            Em_BeginStatus4(em);
            break;
        case 14:
            Em_BeginStatus4(em);
            break;
        case 15:
            Em_ResetToBasePose(em);
            Em_BreakPart(em, 2);
            break;
        case 17:
            Em_EndStatus4(em);
            break;
        case 18:
            Em_EndStatus2(em);
            break;
        case 20:
        case 21:
        case 22:
            Em_BeginStatus5(em);
            break;
        case 23:
            Em_EndStatus5(em);
            break;
        }
    case 6:
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        break;
    case 7:
        break;
    }
}

void EmDiablos_MoveByVelYCurve(Em *em, s32 kind)
{
    EmWork *work = (EmWork *) &em->work;
    EmCurveKey *tbl;
    EmCurveKey *p;
    f32 frame;
    f32 t;
    s32 i;

    tbl = EmDiablos_MoveVelYCurves[kind];
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

void EmDiablos_StartCurves(Em *em, u8 kind)
{
    EmDiablosWork *w = (EmDiablosWork *)&em->work;

    w->curveNone = 0;
    w->curveVelY = 0;
    w->curveSpeed = 0;
    w->curveKind = kind;
    switch (w->curveKind) {
    case 2:
        w->curveVelY = 1;
        w->curveFrame = 0x46;
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

static s32 EmDiablos_CurveNone(Em *em, EmWork *work)
{
    return 0;
}


extern EmCurveKey *EmDiablos_VelYCurves[];

/* The value of curve work[0x4B] at the frame in the work area (a table of keys): sets
 * em->velY from the key pair around the frame, 2 at the table's end. */
static u8 EmDiablos_CurveVelY(Em *em, EmWork *work)
{
    EmDiablosWork *w = (EmDiablosWork *) work;
    EmCurveKey *p;
    f32 frame;
    f32 t;
    s32 i;
    EmCurveKey *tbl;
    u8 ret;

    tbl = EmDiablos_VelYCurves[((EmDiablosWork *)work)->curveKind];
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

extern EmCurveKey *EmDiablos_SpeedCurves[];

/* The value of curve work[0x4B] at the frame in the work area: sets em->velZ from the key
 * pair around the frame, 4 at the table's end. */
static u8 EmDiablos_CurveSpeed(Em *em, EmWork *work)
{
    EmDiablosWork *w = (EmDiablosWork *) work;
    EmCurveKey *p;
    f32 frame;
    f32 t;
    s32 i;
    EmCurveKey *tbl;
    u8 ret;

    tbl = EmDiablos_SpeedCurves[((EmDiablosWork *)work)->curveKind];
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
u8 EmDiablos_UpdateCurves(Em *em)
{
    EmDiablosWork *w = (EmDiablosWork *)&em->work;
    u8 ret = 0;

    if (w->curveNone != 0) {
        ret = EmDiablos_CurveNone(em, (EmWork *) w);
    }
    if (w->curveVelY != 0) {
        ret |= EmDiablos_CurveVelY(em, (EmWork *) w);
    }
    if (w->curveSpeed != 0) {
        ret |= EmDiablos_CurveSpeed(em, (EmWork *) w);
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

s32 EmDiablos_ShouldEndState(Em *em)
{
    if (!((s32)Player_InCurrentArea(em) & 0xFF)) {
        return 0;
    }
    if ((em->surfaceKind == 0) && (em->stance == 4)) {
        return 1;
    }
    return 0;
}

s32 EmDiablos_ChargeHitWall(Em *em)
{
    if ((s32) em->parts[6].state >= 2) {
        return 0;
    }
    if ((EmMotion_ReachedFrame(em, 0, 60.0f) != 0) && (em->wallHitDirs & 0xC0000003) && ((s8) em->wallHitSpecial != 0)) {
        return 1;
    }
    return 0;
}

s32 EmDiablos_InMotion448To44A(Em *em)
{
    u16 m = em->motionId[0];

    if ((u32) (m - 0x448) <= 1 || m == 0x44A) {
        return 1;
    }
    return 0;
}

s32 EmDiablos_InSandState(Em *em)
{
    u8 stateGroup;

    stateGroup = em->stateGroup;
    if (stateGroup == 0) {
        if (em->state != 0x1D) {
            goto block_3;
        }
        goto block_24;
    }
block_3:
    if (stateGroup == 4) {
        if (em->state != 6) {
            goto block_6;
        }
        goto block_24;
    }
block_6:
    if (stateGroup == 4) {
        if (em->state != 0xD) {
            goto block_9;
        }
        goto block_24;
    }
block_9:
    if (stateGroup == 4) {
        if (em->state != 0xE) {
            goto block_12;
        }
        goto block_24;
    }
block_12:
    if (stateGroup == 4) {
        if (em->state != 0x11) {
            goto block_15;
        }
        goto block_24;
    }
block_15:
    if (stateGroup == 4) {
        if (em->state != 0x12) {
            goto block_18;
        }
        goto block_24;
    }
block_18:
    if (stateGroup == 4) {
        if (em->state != 0x16) {
            goto block_21;
        }
        goto block_24;
    }
block_21:
    if ((stateGroup == 4) && (em->state == 0x17)) {
block_24:
        return 1;
    }
    return 0;
}

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

/* em_diablos's .data (0x8C7340-0x8C7B80) */
EmTurnParams EmDiablos_TurnParams[3] = {
#include "game/008C7340.inc"
};
EmDiablosVec EmDiablos_CarveThrowPt = {
#include "game/008C7398.inc"
};
s32 GmEm_ReturnZero(void); /* gm_sub */
void *EmDiablos_Callbacks[8] = {
    EmDiablos_SpawnEffects, EmDiablos_SpawnEffects, EmDiablos_TableNop, EmDiablos_UpdateMotionEffects,
    GmEm_ReturnZero, EmDiablos_Reinit, EmDiablos_AllowHit8, NULL,
};
EmCurveKey EmDiablos_MoveVelYCurves_0[8] = {
#include "game/008C73D0.inc"
};
EmCurveKey EmDiablos_MoveVelYCurves_1[20] = {
#include "game/008C7410.inc"
};
EmCurveKey EmDiablos_MoveVelYCurves_3[10] = {
#include "game/008C74B0.inc"
};
EmCurveKey EmDiablos_MoveVelYCurves_5[6] = {
#include "game/008C7500.inc"
};
EmCurveKey EmDiablos_MoveVelYCurves_6[10] = {
#include "game/008C7530.inc"
};
EmCurveKey *EmDiablos_MoveVelYCurves[16] = {
    EmDiablos_MoveVelYCurves_0, EmDiablos_MoveVelYCurves_1, NULL, EmDiablos_MoveVelYCurves_3,
    NULL, EmDiablos_MoveVelYCurves_5, EmDiablos_MoveVelYCurves_6, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
};
EmCurveKey EmDiablos_VelYCurves_2[20] = {
#include "game/008C75C0.inc"
};
EmCurveKey EmDiablos_VelYCurves_3[10] = {
#include "game/008C7660.inc"
};
EmCurveKey EmDiablos_VelYCurves_5[14] = {
#include "game/008C76B0.inc"
};
EmCurveKey EmDiablos_VelYCurves_8[8] = {
#include "game/008C7720.inc"
};
EmCurveKey EmDiablos_VelYCurves_9[18] = {
#include "game/008C7760.inc"
};
EmCurveKey EmDiablos_VelYCurves_10[14] = {
#include "game/008C77F0.inc"
};
EmCurveKey EmDiablos_VelYCurves_11[4] = {
#include "game/008C7860.inc"
};
EmCurveKey EmDiablos_VelYCurves_12[8] = {
#include "game/008C7880.inc"
};
EmCurveKey EmDiablos_VelYCurves_13[24] = {
#include "game/008C78C0.inc"
};
EmCurveKey *EmDiablos_VelYCurves[16] = {
    NULL, NULL, EmDiablos_VelYCurves_2, EmDiablos_VelYCurves_3,
    NULL, EmDiablos_VelYCurves_5, NULL, NULL,
    EmDiablos_VelYCurves_8, EmDiablos_VelYCurves_9, EmDiablos_VelYCurves_10, EmDiablos_VelYCurves_11,
    EmDiablos_VelYCurves_12, EmDiablos_VelYCurves_13, NULL, NULL,
};
EmCurveKey EmDiablos_SpeedCurves_5[14] = {
#include "game/008C79C0.inc"
};
EmCurveKey EmDiablos_SpeedCurves_8[8] = {
#include "game/008C7A30.inc"
};
EmCurveKey EmDiablos_SpeedCurves_9[14] = {
#include "game/008C7A70.inc"
};
EmCurveKey EmDiablos_SpeedCurves_10[12] = {
#include "game/008C7AE0.inc"
};
EmCurveKey *EmDiablos_SpeedCurves[16] = {
    NULL, NULL, NULL, NULL,
    NULL, EmDiablos_SpeedCurves_5, NULL, NULL,
    EmDiablos_SpeedCurves_8, EmDiablos_SpeedCurves_9, EmDiablos_SpeedCurves_10, NULL,
    NULL, NULL, NULL, NULL,
};
