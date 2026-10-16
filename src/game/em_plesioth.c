/*
 * Monster family: Plesioth and Green Plesioth (kinds 0x15, 0x2E).
 * The code from the family's setup function (0x789FE0, called by the per-kind
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
void Em_PlaySe1AtBoneOnFrame(Em *em, s32 frame, s32 se, s32 bone);
s32 Sound_RandomSe(s32 id, s16 lo, s32 alt, s16 n);

/* prototypes */
void EmApceros_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void EmPlesioth_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void Em_BeginStatus1(Em *em);
void Em_BeginStatus2(Em *em);
void Em_EndStatus1(Em *em);
void Em_EndStatus2(Em *em);
void Em_EndStatus4(Em *em);
void Em_EndStatus5(Em *em);
void Em_PlaySeAtBone(Em *em, s32 se, s32 bone);
void Em_PlaySeAtBoneOnFrame(Em *em, s32 frame, s32 se, s32 bone);
void Em_SetLayerMotion(Em *em, s32 motion, s32 blend, s32 frame, s32 layer);
void Em_SetMotion(Em *em, s32 motion, s32 a, s32 b);
/* unit's own declaration: with the definition's void (Em *em, s32 group, s32 state, u8 mode) (game/unk_701810.c), EmApceros_ChangeState: 3 of 94 words differ, EmPlesioth_ChangeState: 7 of 171 words differ ... */
void Em_SetStateSync(Em *em, u16 group, u16 state, u8 mode);
u16 Math_AngleXZ(f32 *from, f32 *to);
void Player_ClearFlag(void *, u32 flag);
void Player_SetFlag(void *obj, u32 flag);
void QuestEm_Captured(Em *em);
void QuestEm_Hunted(Em *em);
void QuestEm_Leave(Em *em);
u16 Rand_Next(s32 stream);
s32 Save_TestEventFlag(s32);
void EffEm08_CreateAtBone2(Em *, s32, s32, s32, s32, s32, s32);
s32 MsgEvent_Check(u16 kind);
void Em_MoveRotated(Em *em, void *src);
void Em_MoveRotatedAccel(Em *em, void *ang);
void Em_SetStateRand(Em *em, s32 arg1, s32 arg2);
void Em_ComputeGoal2(Em *em, f32 *target);
/* unit's own declaration: with the definition's void (Em *em, u8 a, u8 prio) (game/unk_701810.c), EmApceros_SetRequest: 8 of 19 words differ */
void Em_SetRequest(Em *em, s32 a, s32 prio);
void Em_GetPitchToGoal(Em *em, s32 *out, f32 a, f32 b);
void Em_ClearLowHpNeed(Em *em);
void Em_SpawnEffect06Bone(Em *em, s32 a);
void Em_SetHitReactMode(Em *em, u8 a);
void Em_SpawnEffect06Periodic(Em *em, s32 bone, f32 *pos, f32 scale);
void Em_SetGoal12(Em *em);
void Em_ApplyAreaValues(Em *em);
void Em_ClearPoison(Em *em);
void Em_SetHold2(Em *em);
void Em_SetCaptured(Em *em, s32 a);
void Em_PlaySe6AtBoneInFrames(Em *em, s32 frame, s32 frame2, s32 se, s32 bone, s32 vol, s32 a);
void Em_PlaySe6AtBoneOnFrame(Em *em, s32 frame, s32 se, s32 bone);
void Em_PlaySeAtBoneVolOnFrame(Em *em, s32 frame, s32 se, s32 bone, s32 vol);
s32 Em_StepToGoal(Em *em, u16 step, u8 flags);
u8 Em_TurnToGoal(Em *em, u16 step, u8 flags);
void Em_StartTurnToTarget(Em *em, EmTurnParams *t);
s32 Em_TurnStep(Em *em, EmTurnParams *t);
void Em_RestartScript(Em *em);
void EmPlesioth_Reset(Em *);
void EmPlesioth_SetMode(Em *, u8, u8);
void EmPlesioth_State3_06(Em *, EmWork *);
void EmPlesioth_State6_00_Captured(Em *, EmWork *);
void EmPlesioth_State6_01_Entrance(Em *, EmWork *);
void EmPlesioth_State5_00_Hunted(Em *, EmWork *);
void EmPlesioth_State5_01_Hunted(Em *, EmWork *);
void EmPlesioth_State5_02_HuntedFalling(Em *, EmWork *);
void EmPlesioth_State5_03_Hunted(Em *, EmWork *);
void EmPlesioth_RunGroup0(Em *em, EmWork *work);
void EmPlesioth_RunGroup2(Em *, EmWork *);
void EmPlesioth_RunGroup4(Em *em, EmWork *work);
void EmPlesioth_MotionEffects(Em *, EmWork *);
void EmPlesioth_UpdateFins(Em *em, EmWork *work);
void EmPlesioth_SpawnSurfaceEffect2(Em *em, f32 scale);
void EmPlesioth_SpawnSurfaceEffect5(Em *em, f32 scale);
void EmPlesioth_SpawnSurfaceEffect4(Em *em, f32 scale);
void EmPlesioth_StartCurves(Em *em, u8 kind);
u8 EmPlesioth_UpdateCurves(Em *);
s32 EmPlesioth_OverDeepWater(Em *em, u8 arg1);
void EmKind0A_MotionSounds(void *arg0, EmWork *work);
s32 EmKind0A_IsPlayerNear(Em *);
void EmKind0A_ToIdle(Em *em, u8 flag, u16 a, u16 b);
void EmKind0A_TurnToYaw(Em *);
void EmKind0A_State0_00(Em *);
void EmKind0A_State0_02(Em *);
void EmKind0A_State0_03(Em *);
void EmKind0A_State0_04(Em *em);
void EmKind0A_State0_05(Em *, s32);
void EmKind0A_State0_09(Em *);
void EmKind0A_State0_0A(Em *);
void EmKind0A_RunState(Em *em);
void EmApceros_Init(Em *em);
void EmApceros_EndState(Em *);
void EmApceros_State0_02(Em *);
void EmApceros_State0_08(Em *);
void EmApceros_State1_00(Em *);
void EmApceros_State1_01(Em *);
void EmApceros_State1_02(Em *);
void EmApceros_State1_03(Em *);
void EmApceros_State1_04(Em *);
void EmApceros_State1_05(Em *);
void EmApceros_State1_08(Em *);
void EmApceros_State4_01(Em *);
void EmApceros_State4_02(Em *);
void EmApceros_State4_03(Em *, s32);
void EmApceros_State4_05(Em *);
void EmApceros_State4_06(Em *);
void EmApceros_State5_00_Hunted(Em *, s32);
void EmApceros_SpawnEffect02AtBone2(Em *em, f32 scale, s32 arg2);
void EmApceros_State6_01(Em *);
void EmApceros_State6_05(Em *);
void EmApceros_State6_06(Em *em);
void EmApceros_MotionEffects(Em *, EmWork *);
void StageObj_K13_Create(void *owner, s16 v, u8 k);
void StageObj_K14_Create(Em *em, s16 k, s16 c, f32 scale);
/* unit's own declaration: with the definition's void (Em *em, u8 k, s16 v, s16 t, f32 y, f32 scale) (game/unk_874B90.c), EmPlesioth_State4_0E: 72 of 117 words differ */
void StageObj_Stun_CreateEm(Em *, s32, s32, s32, f32, f32);
s32 Em_PlaySeChecked(Em *em, s32 id, s32 a, void *pos, s32 b, s32 c);
s32 Player_PassedFrameScaled(void *p, f32 frame, s32 layer);
s32 EmMotion_ReachedFrame(void *obj, s32 a, f32 frame);
s32 EmMotion_InFrameRange(void *em, s32 layer, f32 from, f32 to);
void Mtx_GetTrans(f32 *out, f32 *m);
void CamShake_ReqAIf(Em *obj, s32 i);
void Vec_Set(f32 *out, f32 x, f32 y, f32 z);
f32 Vec_DistanceXZ(f32 *a, f32 *b);
void ObjListA70_Remove(EmObj *obj);
s32 QuestEm_CanAppear(Em *em);
void Player_ClearVelocity(void *);
void Player_ClearAccel2(void *obj);
void Motion_InitTime(void *p, s16 step);
u8 Player_InCurrentArea(void *obj);
void Effect01_Spawn(void *obj, s16 type);
void Effect08_Spawn(f32 *pos, s32 k, s32 b, f32 scale);
extern EmTurnParams EmPlesioth_TurnParams[3];
extern EmTurnParams EmApceros_TurnParams[1];
s32 EmKind0A_Trade(Em *em);
void GmEm_ResetSize(Em *em);
void Em_TargetPlayerInRange(Em *);
s32 Game_IsOnline(void);
void Versus_MarkDefeat(Em *);
s8 QuestEm_Appear(Em *em);
void Em_ResetToBasePose(Em *em);
void Em_BreakPart(Em *em, s32 part);
void Em_EndAllStatus(Em *em);
void Em_BeginStatus4(Em *em);
void Em_BeginStatus5(Em *em);
void Em_LoadReactionSet(Em *em, u8 k);
void Em_SetSpeedScaled(Em *em, u16 flags, f32 x, f32 y);
void Em_CheckRageByHp(Em *em, s16 n);
f32 Vec_Distance(f32 *a, f32 *b);
void Em_ComputeGoal(Em *em, f32 *target);
void Em_TypeSetup(Em *em);
void Em_SetTargetAngle(Em *em);
s32 Em_GetTargetPos(Em *em, u8 idx, f32 *out);
void Em_StartRage(Em *em);
void Em_StartPendingRage(Em *em);
void Em_ResetNotice(Em *em);
void Em_RefillNeed3(Em *em);
void Em_StartGoalAction(Em *em, s32 arg1);
void EmPlesioth_SetGroup1State(Em *em, u16 state, u8 mode);
void EmPlesioth_SetGroup2State(Em *em, u16 state, u8 mode);
void EmPlesioth_SetGroup3State(Em *em, u16 state, u8 mode);
u8 EmPlesioth_NextAreaPoint(Em *em, f32 *out);
void EmPlesioth_SetGroup0State(Em *em, u16 state, u8 mode);
void Em_ClearPart(Em *em, u16 part);
void Em_AddHp(Em *em, s16 n);
void Em_AddNeed1(Em *em, s32 n);
void Em_AddNeed2(Em *em, s32 n);
void Em_ClearNeed1State(Em *em);
void Em_ClearNeed2State(Em *em);
/* unit's own declaration: with the definition's s16 (void *obj, u8 state, u8 sub) (sub_main/unk_3A1070.c), EmKind0A_TurnToYaw: 33 of 42 words differ, EmPlesioth_State2_11: 59 of 123 words differ */
s32 Player_IsState(void *obj, s32 group, s32 state);
void MinePoint_Remove(s32);
void Vec_Copy(f32 *out, f32 *in);
f32 *Mtx_FromAnglesXYZ(s32 *angles, f32 *m);
s32 Angle_TurnToward(s32 *cur, s32 *target, s32 step);
u16 Math_Heading(f32 x1, f32 z1, f32 x2, f32 z2);
void Em_SetMotionRow(Em *em, s32 row, s32 arg2, s32 arg3);
void Em_SetPartScaled(Em *em, s32 bit);
s32 Em_ComputeSize(Em *em, s16 idx);
void StageObj_K09_Create(Em *);
void EmPlesioth_State2_10(Em *em, EmWork *work, u8 variant);
void EmPlesioth_State4_05(Em *em, EmWork *work, s16 variant);
void EmPlesioth_State4_06(Em *em, EmWork *work);
void EmPlesioth_State4_09(Em *em, EmWork *work);
void EmPlesioth_State4_0F(Em *em, EmWork *work);
void EmPlesioth_State4_16(Em *em, EmWork *work);
void EmPlesioth_State0_04(Em *em, EmWork *work, u8 variant);
s8 EmMine_Create(Em *em, s32 idx);
s32 EmMine_Update(Em *em);
void EmMine_Remove(Em *em);
void Vec_RotateY(f32 *v, f32 angle);
s32 Quest_IsTraining(void);
extern EmCurveKey *EmPlesioth_VelYCurves[];
extern EmCurveKey *EmPlesioth_SpeedCurves[];
extern u16 EmPlesioth_FinFoldTarget[];
void Em_MotionDelta(Em *em, f32 *out);
s8 Em_FindTargetPlayer(Em *em, u16 range, f32 maxDist, f32 maxHeight);
s32 Obj_SameArea(void *obj, Player *b);
s32 sprintf(char *, const char *, ...);
extern char *gItemNames[];
extern char gEmKind0AMsgBufs[];
s32 Obj_InCurrentArea(void *obj);
void Em_InitParts(Em *em);
void Player_MoveAccel(Player *p);
void Player_ClearAccel(Player *p);
s32 Player_StartJump(void *obj, s16 frames);
void Vec_Rotate(f32 *out, f32 *in, f32 *mtx);
u16 Vec_Heading(f32 *a, f32 *b);
void Sound_PlaySeFlat(u32 slot, u32 id, s32 prog);
s32 Quest_IsDrill(void);
u16 Pouch_FindCarryItem(Player *p);
s32 EmKind0A_AdjustItem(Em *em, s32 item);
void EmKind0A_ShowMessage(Em *em, u16 id);
struct EmIdEntry;
f32 *Em_AreaEntryValue(Em *em, struct EmIdEntry *entry);
s32 Em_UpdateCombatTimer(Em *em);
u8 Em_UpdateDamage(Em *em, u8 *out);
void Em_SetStatus4Timer(Em *em);
void Em_SetStatus2Timer(Em *em);
void Em_UpdatePendingScript(Em *em);
void Em_ClearHitReactMode(Em *em);
u8 Em_SenseObjects(Em *em, EmObj *self);
s32 ObjListA70_Add(EmObj *obj);
u16 Area_DayId(u16 id);
void EmApceros_RunState(Em *em);
extern u8 gEmIsLarge2[];
s32 Em_InBlockArea1(Em *em);
void Em_HoldPosNoPlayers(Em *em, u16 a, u16 b);
void Em_SetGoal12Dist(Em *em, u8 *p);
void Em_SetStatus1Timer(Em *em);
void Em_SetStatus5Timer(Em *em);
void Em_ResetKindTimer(Em *em);
void EmCephadrome_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void EmPlesioth_RunState(Em *em, EmWork *work);
s32 Player_IsLocal(void *obj);
s32 Lure_PlayerHasMonsterBait(void *pl);
void Vib_StartLocal(void *port, s32 i);
f32 *Mtx_FromAnglesYXZ(s32 *angles, f32 *m);
void CamReq_Mode05(void *arg);
extern s16 gEmCombatInit[];
s8 EmMine_CreateTimed0(Em *em, f32 *pos);
void Mtx_Copy(f32 *dst, f32 *src);
EmBone *Em_Bone(void *obj, s16 bone);
void Vec_RotateSelf(f32 *v, f32 *m);
f32 Effect_Scale(void);
f32 Math_Atan2(f32 y, f32 x);
f32 Math_Sqrtf(f32 x);
void Effect02_SpawnAtScaled(u16 ax, u16 ay, s32 kind, f32 *pos, f32 scale);
extern f32 EmApceros_ScalesType0C[];
extern f32 EmApceros_ScalesType46[];
void GmEm_ApplySize(Em *em);
s16 Pouch_Count(Player *p, u16 item);
s16 Pouch_HasFreeSlot(Player *p);
extern u16 EmKind0A_GiftItemsV0[];
extern u16 EmKind0A_GiftItemsV1[];
void EffEm11_Create(Em *, s8);
void Effect0D_SpawnOnGround(f32 *pos, s32 a, f32 scale);
void Em_BonePos(void *obj, s16 bone, f32 *out);
void Effect0D_SpawnForEm(Em *em, s32 a, s32 b);
s32 Notice_OpenTalk(char *text, u32 frame, s32 a, s32 b);
s32 Quest_PhaseIsOver(s32);
u16 Area_BaseId(u16);
s16 Pouch_Space(Player *p, u16 item);
/* unit's own declaration: with the definition's u8 (Player *p, u16 item, s16 n) (sub_main/pouch.c), EmKind0A_Trade: 1 of 1018 words differ */
u16 Pouch_Add(Player *pl, u16 item, s16 n);
s16 Pouch_ClearSlot(Player *p, s16 slot);
u8 Notice_Push(u8 kind, s16 a, s16 b);
s32 Quest_DrillTakeCharm(void);
extern u16 EmKind0A_TradeArea02V0[];
extern u16 EmKind0A_TradeArea02V1[];
extern u16 EmKind0A_TradeArea04V0[];
extern u16 EmKind0A_TradeArea04V1[];
extern u16 EmKind0A_TradeArea06V0[];
extern u16 EmKind0A_TradeArea06V1[];
extern u16 EmKind0A_TradeArea03V0[];
extern u16 EmKind0A_TradeArea03V1[];
extern u16 EmKind0A_TradeArea11V0[];
extern u16 EmKind0A_TradeArea11V1[];
typedef struct EmTradeRate {
    u16 weight;
    u16 item;
} EmTradeRate;
extern EmTradeRate EmKind0A_TradeRates[];
void Effect0D_SpawnForEmScaled(Em *em, s32 a, s32 b, f32 scale);
void EffPlesioth20_Create(Em *em, s8 kind);
void EmPlesioth_MotionSounds(Em *em, EmWork *work);
void EmPlesioth_SetFinMode(Em *em, EmWork *work, s32 mode);
void EmPlesioth_SpawnK14Type0B(Em *em);
/* end of prototypes */

void EmPlesioth_Setup(Em *em)
{
}

void EmPlesioth_SpawnEffects(void *em)
{
    StageObj_K13_Create(em, 0x22, 0);
    Effect01_Spawn((Player *) em, 0);
}

void EmPlesioth_Reset(Em *em)
{
    EmPlesiothWork *w = (EmPlesiothWork *)&em->work;

    s16 size;
    u8 fxFlags;

    Em_SetMotionRow(em, 1, 0, 0);
    Em_SetPartScaled(em, 3);
    Em_SetPartScaled(em, 4);
    w->unk010 = 0;
    size = Em_ComputeSize(em, 0);
    em->hp = size;
    em->hpMax = size;
    em->scriptPending = 1;
    em->blind = 0;
    em->unk_InitFlag = 1;
    em->noAlert = 0;
    em->painted = 0;
    *(f32 *) &em->surfaceDepth = 670.0f;
    w->unk00A = 0x4000;
    w->unk028 = 0x100;
    w->unk02C = 0x200;
    w->unk030 = 0x100;
    w->unk018 = 0x2000;
    w->unk014 = 0;
    w->unk034 = 0;
    w->unk035 = 0;
    w->unk036 = 0;
    w->finMode = 0;
    w->finWait[0] = 0;
    w->finState[0][0] = 0;
    w->finState[0][1] = 0;
    w->finRot[0].fold = 0;
    w->finRot[0].wave = 0;
    w->finWait[1] = 0;
    w->finState[1][0] = 0;
    w->finState[1][1] = 0;
    w->finRot[1].fold = 0;
    w->finRot[1].wave = 0;
    w->finWait[2] = 0;
    w->finState[2][0] = 0;
    w->finState[2][1] = 0;
    w->finRot[2].fold = 0;
    w->finRot[2].wave = 0;
    w->finWait[3] = 0;
    w->finState[3][0] = 0;
    w->finState[3][1] = 0;
    w->finRot[3].fold = 0;
    w->finRot[3].wave = 0;
    fxFlags = em->fxFlags & 1;
    em->fxFlags = fxFlags;
    if (fxFlags == 0) {
        switch (em->type) {
        case 0x15:
        case 0x2E:
            em->meshShown[1] = 0;
            StageObj_K09_Create(em);
            break;
        }
    }
}

void EmPlesioth_Start(Em *em)
{
    f32 fslot;
    u8 slotNo;

    EmPlesioth_Reset(em);
    if (gQuest.stage == 0) {
        em->rotY = 0x8000;
        slotNo = em->slotNo;
        fslot = (f32) (u32) slotNo;
        em->posX = 5000.0f + (1500.0f * fslot);
        em->posY = 0.0f;
        em->posZ = 9000.0f;
        em->stance = 4;
        EmPlesioth_ChangeState(em, 2, 0, 0);
        return;
    }
    em->stance = 4;
    EmPlesioth_ChangeState(em, 2, 0, 0);
}

void EmPlesioth_Reinit(Em *em)
{
    EmPlesioth_Reset(em);
}

void EmPlesioth_EndState(Em *em)
{
    em->motionSpeed = 1.0f;
    em->stance = 0;
    em->lookAtTarget = 0;
    em->scriptPending = 1;
    EmPlesioth_ChangeState(em, 0, 1, 0);
}

void EmPlesioth_EndState2(Em *em)
{
    em->motionSpeed = 1.0f;
    em->stance = 4;
    em->lookAtTarget = 0;
    em->scriptPending = 1;
    EmPlesioth_ChangeState(em, 2, 0, 0);
}

void EmPlesioth_SetMode(Em *em, u8 mode, u8 flag)
{
    em->stance = mode;
    if (flag == 0) {
        switch (mode) {
        case 0: case 1: default:
            Em_LoadReactionSet(em, 0);
            break;
        case 4:
            Em_LoadReactionSet(em, 2);
            break;
        }
    } else {
        Em_LoadReactionSet(em, 1);
    }
}

s32 EmPlesioth_NoneHookedNear(Em *em, Player *p)
{
    Em *m;
    s16 i;

    for (m = gEms, i = 0; i < 20; i++, m++) {
        if (m != em && m->active && m->visible && (Obj_SameArea(m, p) & 0xFF)) {
            if (m->hooked != 0 || (m->stateGroup == 4 && m->state == 0xF)) {
                return 0;
            }
        }
    }
    return 1;
}

void EmPlesioth_State0_00(Em *em, EmWork *work)
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

void EmPlesioth_State0_01(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmPlesioth_SetMode(em, 0, 0);
        em->lookAtTarget = 0;
        if (em->motionId[0] != 0x3E9) {
            Em_SetLayerMotion(em, 0x3E9, 0, 0, 0);
        }
        if (em->motionId[1] != 0x4B1) {
            Em_SetLayerMotion(em, 0x4B1, 0, 0, 1);
        }
        if (em->motionId[2] != 0x579) {
            Em_SetLayerMotion(em, 0x579, 0, 0, 2);
        }
        break;
    case 1:
        if ((em->remote == 0) && (em->motion[0].playing == 0)) {
            em->step++;
            EmPlesioth_EndState(em);
        }
        break;
    }
}

void EmPlesioth_State0_02(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmPlesioth_SetMode(em, 0, 0);
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x19, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 1, 0, 0x5C);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmPlesioth_EndState(em);
        }
        break;
    }
}

void EmPlesioth_State0_04(Em *em, EmWork *work, u8 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmPlesioth_SetMode(em, 0, 0);
        em->lookAtTarget = 0;
        switch (variant) {
        default:
            Em_SetMotion(em, 0x79, 0, 0);
            break;
        case 1:
            Em_SetMotion(em, 0x7B, 0, 0);
            break;
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stance = 1;
            Em_SetMotion(em, 0x7A, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stance = 0;
            Em_SetMotion(em, 0x7C, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmPlesioth_EndState(em);
        }
        break;
    }
}

void EmPlesioth_State0_05(Em *em, s32 unused, s32 variant)
{
    Em_SetHitReactMode(em, 2U);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        EmPlesioth_SetMode(em, 0, 1);
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x26, 0, 0);
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
            if (variant == 0) {
                EmPlesioth_ChangeState(em, 0, 6, 4);
                break;
            }
            EmPlesioth_ChangeState(em, 0, 0x2A, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            if (variant == 0) {
                EmPlesioth_ChangeState(em, 0, 6, 4);
                break;
            }
            EmPlesioth_ChangeState(em, 0, 0x2A, 4);
        }
        break;
    }
}

void EmPlesioth_State0_06(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmPlesioth_SetMode(em, 0, 0);
        Em_SetMotion(em, 0x25, 0, 0);
        em->blind = 2;
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            EmPlesioth_ChangeState(em, 0, 7, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmPlesioth_ChangeState(em, 0, 7, 4);
        }
        break;
    }
}

void EmPlesioth_State0_07(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmPlesioth_SetMode(em, 0, 0);
        Em_SetMotion(em, 0x3E, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmPlesioth_EndState(em);
        }
        break;
    }
}

void EmPlesioth_State0_08(Em *em, EmWork *work)
{
    f32 pos[3];

    Em_SetHitReactMode(em, 2);
    em->holdTimer = 5;
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        EmPlesioth_SetMode(em, 0, 1);
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x61, 0, 0);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmPlesioth_ChangeState(em, 4, 0x13, 3);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmPlesioth_ChangeState(em, 4, 0x13, 3);
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

void EmPlesioth_State0_0A(Em *em, EmWork *work)
{
    f32 pos[3];

    Em_SetHitReactMode(em, 2);
    em->holdTimer = 5;
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        EmPlesioth_SetMode(em, 0, 1);
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x61, 0, 0);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmPlesioth_ChangeState(em, 4, 0x14, 3);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmPlesioth_ChangeState(em, 4, 0x14, 3);
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

void EmPlesioth_State0_0C(Em *em)
{
    Em_SetHitReactMode(em, 2U);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        EmPlesioth_SetMode(em, 0, 1);
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x26, 0, 0);
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
            EmPlesioth_ChangeState(em, 0, 0xD, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmPlesioth_ChangeState(em, 0, 0xD, 4);
        }
        break;
    }
}

void EmPlesioth_State0_0E(Em *em)
{
    Em_SetHitReactMode(em, 2U);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        EmPlesioth_SetMode(em, 1, 1);
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x4B, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x80, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x89, 0, 0);
        }
        break;
    case 3:
        if (--em->timer <= 0) {
            em->step++;
            EmPlesioth_ChangeState(em, 0, 0xF, 4);
        }
        break;
    case 4:
        if (em->remote == 0) {
            EmPlesioth_ChangeState(em, 0, 0xF, 4);
        }
        break;
    }
}

void EmPlesioth_State0_0F(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmPlesioth_SetMode(em, 0, 0);
        Em_SetMotion(em, 0x7C, 0, 0);
        em->blind = 2;
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            EmPlesioth_ChangeState(em, 0, 7, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmPlesioth_ChangeState(em, 0, 7, 4);
        }
        break;
    }
}

void EmPlesioth_State0_10(Em *em)
{
    Em_SetHitReactMode(em, 2U);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        EmPlesioth_SetMode(em, 0, 1);
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x4B, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x80, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x89, 0, 0);
        }
        break;
    case 3:
        if (--em->timer <= 0) {
            em->step++;
            EmPlesioth_ChangeState(em, 0, 0x11, 4);
        }
        break;
    case 4:
        if (em->remote == 0) {
            EmPlesioth_ChangeState(em, 0, 0x11, 4);
        }
        break;
    }
}

void EmPlesioth_State0_12(Em *em)
{
    s16 hpMax;
    s16 hp;

    Em_SetHitReactMode(em, 2U);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        EmPlesioth_SetMode(em, 0, 1);
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x20, 0, 0);
        em->timer = 0x708;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x1F, 0, 0);
        }
        break;
    case 2:
        hpMax = em->hpMax;
        hp = em->hp;
        if (hp < hpMax) {
            em->hp = hp + 1;
        } else {
            em->hp = hpMax;
        }
        if (--em->timer <= 0) {
            em->step++;
            em->hp = em->hpMax;
            Em_ClearLowHpNeed(em);
            EmPlesioth_ChangeState(em, 0, 0x13, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmPlesioth_ChangeState(em, 0, 0x13, 4);
        }
        break;
    }
}

void EmPlesioth_State0_13(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmPlesioth_SetMode(em, 0, 0);
        Em_SetMotion(em, 0x21, 0, 0);
        em->blind = 2;
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            EmPlesioth_ChangeState(em, 0, 7, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmPlesioth_ChangeState(em, 0, 7, 4);
        }
        break;
    }
}

void EmPlesioth_State1_00(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        EmPlesioth_SetMode(em, 0, 0);
        Em_SetMotion(em, 3, 0, 0);
        break;
    case 1:
        if (em->hasGoal != 0) {
            if (Em_StepToGoal(em, 0x40, 1) == 1) {
                em->step++;
                EmPlesioth_EndState(em);
                break;
            }
        } else {
            if (--em->timer <= 0) {
                em->step++;
                EmPlesioth_EndState(em);
            }
        }
        break;
    }
}

void EmPlesioth_State1_01(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        EmPlesioth_SetMode(em, 0, 0);
        Em_SetMotion(em, 4, 0, 0);
        break;
    case 1:
        if (em->hasGoal != 0) {
            if (Em_StepToGoal(em, 0x40, 1) == 1) {
                em->step++;
                Em_SetMotion(em, 0x85, 0, 0);
                break;
            }
        } else {
            if (--em->timer <= 0) {
                em->step++;
                Em_SetMotion(em, 0x85, 0, 0);
            }
            break;
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmPlesioth_EndState(em);
        }
        break;
    }
}

void EmPlesioth_State1_02(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        EmPlesioth_SetMode(em, 0, 0);
        Em_StartTurnToTarget(em, &EmPlesioth_TurnParams[0]);
        break;
    case 1:
        if (Em_TurnStep(em, &EmPlesioth_TurnParams[0]) == 1) {
            em->step++;
            Player_ClearFlag(em, 0x20000);
            EmPlesioth_EndState(em);
        }
        break;
    }
}

void EmPlesioth_State1_03(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        EmPlesioth_SetMode(em, 0, 0);
        Em_SetMotion(em, 0xA, 0, 0);
        break;
    case 1:
        if (em->hasGoal != 0) {
            if (Em_StepToGoal(em, 0x40, 1) == 1) {
                em->step++;
                EmPlesioth_EndState(em);
                break;
            }
        } else {
            if (--em->timer <= 0) {
                em->step++;
                EmPlesioth_EndState(em);
            }
        }
        break;
    }
}

void EmPlesioth_State2_00(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x64, 0, 0);
        EmPlesioth_SetMode(em, 4, 0);
        break;
    case 1:
        if ((em->remote == 0) && (em->motion[0].playing == 0)) {
            em->step++;
            EmPlesioth_EndState2(em);
        }
        break;
    }
}

void EmPlesioth_State2_01(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x64, 0, 0);
        EmPlesioth_SetMode(em, 4, 0);
        Em_ApplyAreaValues(em);
        break;
    case 1:
        if ((em->remote == 0) && (em->motion[0].playing == 0)) {
            em->step++;
            EmPlesioth_EndState2(em);
        }
        break;
    }
}

void EmPlesioth_State2_02(void)
{
}

void EmPlesioth_State2_03(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        EmPlesioth_SetMode(em, 4, 0);
        Em_StartTurnToTarget(em, &EmPlesioth_TurnParams[1]);
        break;
    case 1:
        if (Em_TurnStep(em, &EmPlesioth_TurnParams[1]) == 1) {
            em->step++;
            EmPlesioth_EndState2(em);
        }
        break;
    }
}

void EmPlesioth_State2_04(Em *em, EmWork *work)
{

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        EmPlesioth_SetMode(em, 4, 0);
        Em_SetMotion(em, 0x65, 0, 0);
        Player_ClearVelocity(em);
        em->velZ = 5.0f;
        em->accelZ = 1.0f;
        break;
    case 1:
        Em_GetPitchToGoal(em, (s32 *) &work->moveAngBX, 0.0f, 0.0f);
        work->moveAngBY = em->rotY;
        work->moveAngBZ = 0;
        Em_MoveRotated(em, &work->moveAngBX);
        if (!(em->velZ < 50.0f)) {
            em->velZ = 50.0f;
        }
        if (Em_StepToGoal(em, 0x40, 2) == 1) {
            EmPlesioth_EndState2(em);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x66, 0, 0);
        }
        break;
    case 2:
        Em_GetPitchToGoal(em, (s32 *) &work->moveAngBX, 0.0f, 0.0f);
        work->moveAngBY = em->rotY;
        work->moveAngBZ = 0;
        Em_MoveRotated(em, &work->moveAngBX);
        if (!(em->velZ < 50.0f)) {
            em->velZ = 50.0f;
        }
        em->goalDist -= em->velZ;
        if (em->goalDist <= 0.0f) {
            EmPlesioth_EndState2(em);
        }
        break;
    }
}

void EmPlesioth_State2_05(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        EmPlesioth_SetMode(em, 4, 0);
        Em_SetMotion(em, 0x67, 0, 0);
        Player_ClearVelocity(em);
        em->velZ = 30.0f;
        em->accelZ = 10.0f;
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 30.0f) != 0) {
            Em_GetPitchToGoal(em, (s32 *) &work->moveAngBX, 0.0f, 0.0f);
            work->moveAngBY = em->rotY;
            work->moveAngBZ = 0;
            Em_MoveRotated(em, &work->moveAngBX);
            if (!(em->velZ < 100.0f)) {
                em->velZ = 100.0f;
            }
            if (Em_StepToGoal(em, 0x40, 2) == 1) {
                em->step++;
                EmPlesioth_EndState2(em);
            }
            EmPlesioth_SpawnSurfaceEffect2(em, 8.0f);
            EmPlesioth_SpawnSurfaceEffect4(em, 8.0f);
        }
        break;
    }
}

void EmPlesioth_State2_06(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        EmPlesioth_SetMode(em, 4, 0);
        Em_SetMotion(em, 0x6E, 0, 0);
        EmPlesioth_StartCurves(em, 0U);
        break;
    case 1:
        if (EmPlesioth_UpdateCurves(em)) {
            em->step++;
            EmPlesioth_EndState2(em);
        }
        break;
    }
}

void EmPlesioth_State2_07(Em *em, EmWork *work)
{
    f32 surfaceY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        EmPlesioth_SetMode(em, 4, 0);
        Em_SetMotion(em, 0x68, 0, 0);
        Player_ClearVelocity(em);
        em->velZ = 20.0f;
        em->velY = 2.0f;
        em->accelY = 4.0f;
        work->moveAngBX = 0;
        work->moveAngBZ = 0;
        break;
    case 1:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, &work->moveAngBX);
        if (!(em->velY <= 50.0f)) {
            em->velY = 50.0f;
            em->accelY = 0.0f;
        }
        surfaceY = em->surfaceY;
        if (!(em->posY <= surfaceY)) {
            em->posY = surfaceY;
            em->step++;
            Em_SetMotion(em, 0x86, 0, 0);
            EmPlesioth_SpawnSurfaceEffect5(em, 8.0f);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmPlesioth_EndState2(em);
        }
        break;
    }
}

void EmPlesioth_State2_08(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        EmPlesioth_SetMode(em, 4, 0);
        Em_SetMotion(em, 0x69, 0, 0);
        Player_ClearVelocity(em);
        em->velZ = 20.0f;
        em->velY = -2.0f;
        em->accelY = -4.0f;
        work->moveAngBX = 0;
        work->moveAngBZ = 0;
        break;
    case 1:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, &work->moveAngBX);
        if (em->velY < -50.0f) {
            em->velY = -50.0f;
            em->accelY = 0.0f;
        }
        floorY = em->floorY;
        if (em->posY < floorY) {
            em->posY = floorY;
            em->step++;
            Em_SetMotion(em, 0x87, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmPlesioth_EndState2(em);
        }
        break;
    }
}

void EmPlesioth_State2_09(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        EmPlesioth_SetMode(em, 4, 0);
        Em_StartTurnToTarget(em, &EmPlesioth_TurnParams[2]);
        break;
    case 1:
        if (Em_TurnStep(em, &EmPlesioth_TurnParams[2]) == 1) {
            em->step++;
            EmPlesioth_ChangeState(em, 2, 0xA, 3);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmPlesioth_ChangeState(em, 2, 0xA, 3);
        }
        break;
    }
}

void EmPlesioth_State2_0A(Em *em, EmWork *work)
{
    u8 n;

    switch (em->step) {
    case 0:
        EmPlesioth_SetMode(em, 4, 0);
        Em_SetMotion(em, 0x67, 0, 0);
        em->step++;
        em->stateVar1 = 0;
        em->lookAtTarget = 0;
        em->velX = 0.0f;
        em->velY = 0.0f;
        Player_ClearAccel2(em);
        if (em->velZ <= 50.0f) {
            em->velZ = 50.0f;
        }
        break;
    case 1:
        if (!(Vec_DistanceXZ(&em->posX, em->goal) <= 300.0f)) {
            if (--em->timer <= 0) {
                goto block_9;
            }
        } else {
block_9:
            n = ((EmPlesiothWork *)work)->unk007 - 1;
            ((EmPlesiothWork *)work)->unk007 = n;
            if ((n & 0xFF) <= 0) {
                em->step++;
                EmPlesioth_EndState2(em);
            } else {
                em->curGoalNo++;
                em->curGoalNo &= 3;
                Em_ComputeGoal2(em, em->goal);
                em->timer = (s32) ((1.5f * Vec_DistanceXZ(&em->posX, em->goal)) / 50.0f);
                Em_SetStateRand(em, 2, 0xA);
            }
        }
        Em_TurnToGoal(em, 0x200, 0);
        Em_GetPitchToGoal(em, (s32 *) &work->moveAngBX, 0.0f, 0.0f);
        work->moveAngBY = em->rotY;
        work->moveAngBZ = 0;
        Em_MoveRotated(em, (s32 *) &work->moveAngBX);
        EmPlesioth_SpawnSurfaceEffect2(em, 8.0f);
        EmPlesioth_SpawnSurfaceEffect4(em, 8.0f);
        break;
    }
}

void EmPlesioth_State2_0B(Em *em, EmWork *work)
{
    u8 n;

    switch (em->step) {
    case 0:
        EmPlesioth_SetMode(em, 4, 0);
        Em_SetMotion(em, 0x67, 0, 0);
        em->step++;
        em->stateVar1 = 0;
        em->lookAtTarget = 0;
        em->velX = 0.0f;
        em->velY = 0.0f;
        Player_ClearAccel2(em);
        if (em->velZ <= 50.0f) {
            em->velZ = 50.0f;
        }
        break;
    case 1:
        if (!(Vec_DistanceXZ(&em->posX, em->goal) <= 300.0f)) {
            if (--em->timer <= 0) {
                goto block_9;
            }
        } else {
block_9:
            n = ((EmPlesiothWork *)work)->unk007 - 1;
            ((EmPlesiothWork *)work)->unk007 = n;
            if ((n & 0xFF) <= 0) {
                em->step++;
                EmPlesioth_EndState2(em);
            } else {
                em->curGoalNo -= 1;
                em->curGoalNo &= 3;
                Em_ComputeGoal2(em, em->goal);
                em->timer = (s32) ((1.5f * Vec_DistanceXZ(&em->posX, em->goal)) / 50.0f);
                Em_SetStateRand(em, 2, 0xB);
            }
        }
        Em_TurnToGoal(em, 0x200, 0);
        Em_GetPitchToGoal(em, (s32 *) &work->moveAngBX, 0.0f, 0.0f);
        work->moveAngBY = em->rotY;
        work->moveAngBZ = 0;
        Em_MoveRotated(em, (s32 *) &work->moveAngBX);
        EmPlesioth_SpawnSurfaceEffect2(em, 8.0f);
        EmPlesioth_SpawnSurfaceEffect4(em, 8.0f);
        break;
    }
}

void EmPlesioth_State2_0C(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        EmPlesioth_SetMode(em, 4, 0);
        Em_SetMotion(em, 0x72, 0, 0);
        em->timer = 0x84;
        Player_ClearVelocity(em);
        em->velZ = 40.0f;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmPlesioth_EndState2(em);
        }
        em->rotY += 0x1F0;
        em->rotY = (s32) (u16) em->rotY;
        Em_GetPitchToGoal(em, (s32 *) &work->moveAngBX, 0.0f, 0.0f);
        work->moveAngBY = em->rotY;
        work->moveAngBZ = 0;
        Em_MoveRotated(em, &work->moveAngBX);
        break;
    }
}

void EmPlesioth_State2_0D(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        EmPlesioth_SetMode(em, 4, 0);
        Em_SetMotion(em, 0x73, 0, 0);
        em->timer = 0x84;
        Player_ClearVelocity(em);
        em->velZ = 40.0f;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmPlesioth_EndState2(em);
        }
        em->rotY -= 0x1F0;
        em->rotY = (s32) (u16) em->rotY;
        Em_GetPitchToGoal(em, (s32 *) &work->moveAngBX, 0.0f, 0.0f);
        work->moveAngBY = em->rotY;
        work->moveAngBZ = 0;
        Em_MoveRotated(em, &work->moveAngBX);
        break;
    }
}

void EmPlesioth_State2_0E(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0x0:
        em->step++;
        em->lookAtTarget = 0;
        EmPlesioth_SetMode(em, 4, 0);
        Player_ClearVelocity(em);
        Em_SetMotion(em, 0x77, 0, 0);
        em->velY = 130.0f;
        if ((Area_DayId(em->area) & 0xFFFF) == 0x72) {
            em->velY *= 0.8f;
        }
        em->velZ = 70.0f;
        em->accelY = -4.0f;
        work->moveAngBX = 0;
        work->moveAngBZ = 0;
        break;
    case 0x1:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, (s32 *) &work->moveAngBX);
        if (!(em->posY < em->surfaceY)) {
            if (em->stance == 4) {
                EmPlesioth_SpawnSurfaceEffect5(em, 8.0f);
            }
            em->stance = 2;
        }
        if (em->velY < 0.0f) {
            if (EmPlesioth_OverDeepWater(em, 1U) == 1) {
                if (em->posY < em->surfaceY) {
                    em->step++;
                    em->posY = em->surfaceY;
                    em->stance = 4;
                    em->velY = 0.0f;
                    em->accelY = 0.0f;
                    Em_SetMotion(em, 0x67, 0xA, 0);
                    em->goalDist = 1000.0f;
                    em->timer = 0x3C;
                    em->accelZ = (em->goalDist - em->timer * em->velZ) / ((em->timer * em->timer) / 2);
                    EmPlesioth_SpawnSurfaceEffect5(em, 8.0f);
                    break;
                }
            } else if (em->posY < em->floorY) {
                em->step = 0x63;
                em->posY = em->floorY;
                EmPlesioth_ChangeState(em, 0, 4, 4);
                break;
            }
        }
        break;
    case 0x2:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, (s32 *) &work->moveAngBX);
        em->goalDist -= em->velZ;
        if ((em->goalDist <= 0.0f) || (em->velZ <= 0.0f)) {
            EmPlesioth_EndState2(em);
        }
        break;
    case 0x63:
        if (em->remote == 0) {
            em->posY = em->floorY;
            EmPlesioth_ChangeState(em, 0, 4, 4);
        }
        break;
    }
}

void EmPlesioth_State2_0F(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        EmPlesioth_SetMode(em, 4, 0);
        Em_SetMotion(em, 0x79, 0, 0);
        Player_ClearVelocity(em);
        em->posY = em->surfaceY;
        EmPlesioth_SpawnSurfaceEffect5(em, 8.0f);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 10.0f, 0U) != 0) {
            em->step++;
            EmPlesioth_EndState2(em);
        }
        break;
    }
}

void EmPlesioth_State2_10(Em *em, EmWork *work, u8 variant)
{
    switch (em->step) {
    case 0x0:
        em->step++;
        em->lookAtTarget = 0;
        EmPlesioth_SetMode(em, 0, 0);
        Em_SetMotion(em, 0x78, 0, 0);
        Player_ClearVelocity(em);
        switch (variant) {
        case 1:
            em->rotY = Math_AngleXZ(&em->posX, em->goal);
            break;
        }
        break;
    case 0x1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stance = 2;
            Em_SetMotion(em, 0x77, 0, 0);
            em->accelY = -4.0f;
            switch (variant) {
            default:
                em->velY = 50.0f;
                em->velZ = 100.0f;
                break;
            case 1:
                em->velY = 60.0f * (-0.5f * em->accelY);
                em->velZ = Vec_DistanceXZ(em->goal, &em->posX) / 60.0f;
                break;
            }
        }
        break;
    case 0x2:
        work->moveAngBX = 0;
        work->moveAngBY = em->rotY;
        work->moveAngBZ = 0;
        Em_MoveRotatedAccel(em, (s32 *) &work->moveAngBX);
        if (em->posY <= em->surfaceY) {
            em->stance = 4;
        }
        if (em->velY < 0.0f) {
            if ((variant != 0 && EmPlesioth_OverDeepWater(em, 0) == 1) || (variant == 0 && EmPlesioth_OverDeepWater(em, 1) == 1)) {
                if (em->posY < em->surfaceY) {
                    em->step++;
                    em->posY = em->surfaceY;
                    em->stance = 4;
                    em->velY = 0.0f;
                    em->accelY = 0.0f;
                    Em_SetMotion(em, 0x67, 0xA, 0);
                    if (variant != 1) {
                        em->goalDist = 1000.0f;
                    } else {
                        em->goalDist = 0.0f;
                    }
                    em->timer = 0x3C;
                    em->accelZ = (em->goalDist - em->timer * em->velZ) / ((em->timer * em->timer) / 2);
                    EmPlesioth_SpawnSurfaceEffect5(em, 8.0f);
                }
            } else if (em->posY < em->floorY) {
                em->step = 0x63;
                em->posY = em->floorY;
                EmPlesioth_ChangeState(em, 0, 4, 4);
            }
        }
        break;
    case 0x3:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, (s32 *) &work->moveAngBX);
        em->goalDist -= em->velZ;
        if ((em->goalDist <= 0.0f) || (em->velZ <= 0.0f)) {
            EmPlesioth_EndState2(em);
        }
        break;
    case 0x63:
        if (em->remote == 0) {
            em->posY = em->floorY;
            EmPlesioth_ChangeState(em, 0, 4, 4);
        }
        break;
    }
}

void EmPlesioth_State2_11(Em *em, EmWork *work)
{
    Player *p = &gPlayers[em->targetNo];

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        EmPlesioth_SetMode(em, 4, 0);
        Em_SetMotion(em, 0x64, 0, 0);
        em->hooked = 0;
        em->timer = 120;
        em->noReact = 1;
        if (Player_IsLocal(p) == 1 && Lure_PlayerHasMonsterBait(p) == 1 && EmPlesioth_NoneHookedNear(em, p) == 1) {
            p->fishTimer = em->timer;
            Vib_StartLocal(p, 2);
        }
        break;
    case 1:
        if (em->remote == 0 && em->combat == 0 && p->active && Player_IsLocal(p) == 1 && (s16) Player_IsState(p, 0, 0x53) && Obj_SameArea(em, p) && Lure_PlayerHasMonsterBait(p) == 1) {
            if (EmPlesioth_NoneHookedNear(em, p)) {
                em->step++;
                em->hooked = 1;
                em->hookedBy = em->targetNo;
                EmPlesioth_ChangeState(em, 2, 0x12, 1);
                break;
            }
        } else if (--em->timer > 0 && em->combat != 1) {
            break;
        }
        em->step++;
        EmPlesioth_EndState2(em);
        break;
    }
}

void EmPlesioth_State2_12(Em *em, EmWork *work)
{
    Player *p = &gPlayers[em->targetNo];
    f32 v[3];
    f32 out[3];
    s32 ang[3];
    f32 mtx[16];

    em->guardTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        EmPlesioth_SetMode(em, 4, 0);
        Em_SetMotion(em, 0x84, 0, 0);
        em->hooked = 1;
        em->hookedBy = em->targetNo;
        em->noReact = 1;
        em->timer = 195;
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = 700.0f;
        ang[0] = 0;
        ang[1] = p->rot[1];
        ang[2] = 0;
        Mtx_FromAnglesYXZ(ang, mtx);
        Vec_Rotate(out, v, mtx);
        em->goal[0] = p->pos[0] + out[0];
        em->goal[1] = p->pos[1] + out[1];
        em->goal[2] = p->pos[2] + out[2];
        Player_ClearVelocity(em);
        em->rotY = Math_AngleXZ(&em->posX, em->goal);
        em->velY = (em->goal[1] - em->posY) / (f32) em->timer;
        em->velZ = Vec_DistanceXZ(&em->posX, em->goal) / (f32) em->timer;
        work->moveAngBX = 0;
        work->moveAngBZ = 0;
        EmPlesioth_SpawnSurfaceEffect5(em, 8.0f);
        Vib_StartLocal(p, 2);
        if (Player_IsLocal(p) == 1) {
            CamReq_Mode05(em);
        }
        break;
    case 1:
        work->moveAngBY = em->rotY;
        Em_MoveRotated(em, (s32 *) &work->moveAngBX);
        if (em->timer == 60 || em->timer == 120 || em->timer == 180 || em->timer == 240 || em->timer == 300) {
            EmPlesioth_SpawnSurfaceEffect5(em, 8.0f);
        }
        if (em->timer == 30 || em->timer == 60 || em->timer == 90 || em->timer == 120 || em->timer == 150 ||
            em->timer == 180 || em->timer == 210 || em->timer == 240 || em->timer == 270 || em->timer == 300) {
            Vib_StartLocal(p, 2);
        }
        if (--em->timer <= 0) {
            em->step++;
            EmPlesioth_ChangeState(em, 4, 0xF, 3);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmPlesioth_ChangeState(em, 4, 0xF, 3);
        }
        break;
    }
}

void EmPlesioth_State2_13(Em *em, EmWork *work)
{
    f32 pos[3];
    f32 pos2[3];

    Em_SetHitReactMode(em, 2);
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x7E, 0, 0);
        Player_ClearVelocity(em);
        em->velY = 60.0f;
        em->accelY = -2.75f;
        work->moveAngBX = 0;
        work->moveAngBZ = 0;
        em->blind = 2;
        break;
    case 1:
        work->moveAngBY = em->rotY;
        Em_MoveRotated(em, (s32 *) &work->moveAngBX);
        Em_SetHitReactMode(em, 2);
        em->blind = 2;
        if (!(em->posY <= em->surfaceY)) {
            em->stance = 2;
            em->step++;
            EmPlesioth_SpawnSurfaceEffect5(em, 8.0f);
        }
        break;
    case 2:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, (s32 *) &work->moveAngBX);
        Em_SetHitReactMode(em, 2);
        em->blind = 2;
        if (em->posY <= em->surfaceY) {
            em->step++;
            em->stance = 4;
            EmPlesioth_SpawnSurfaceEffect5(em, 8.0f);
        }
        break;
    case 3:
        Em_SetHitReactMode(em, 2);
        em->blind = 2;
        if (em->posY < em->surfaceY) {
            em->step++;
            em->lookAtTarget = 0;
            em->stance = 4;
            Em_SetMotion(em, 0x82, 0, 0);
        }
        break;
    case 4:
        Em_SetHitReactMode(em, 2);
        em->blind = 2;
        if (--em->timer <= 0) {
            em->step++;
            em->blind = 0;
            EmPlesioth_ChangeState(em, 2, 0x18, 4);
        }
        pos[0] = -150.0f;
        pos[1] = 50.0f;
        pos[2] = 140.0f;
        Em_SpawnEffect06Periodic(em, 0x22, pos, 1.6f);
        if (em->timer % 135 == 0) {
            Em_PlaySeAtBone(em, 0x57, 0x23);
        }
        break;
    case 5:
        if (em->remote == 0) {
            EmPlesioth_ChangeState(em, 2, 0x18, 4);
        }
        pos2[0] = -150.0f;
        pos2[1] = 50.0f;
        pos2[2] = 140.0f;
        Em_SpawnEffect06Periodic(em, 0x22, pos2, 1.6f);
        if (em->timer % 135 == 0) {
            Em_PlaySeAtBone(em, 0x57, 0x23);
        }
        break;
    }
}

void EmPlesioth_State2_14(Em *em, EmWork *work)
{
    f32 pos[3];
    f32 pos2[3];

    Em_SetHitReactMode(em, 2);
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x7E, 0, 0);
        Player_ClearVelocity(em);
        em->velY = 60.0f;
        em->accelY = -2.75f;
        work->moveAngBX = 0;
        work->moveAngBZ = 0;
        EmPlesioth_SpawnSurfaceEffect5(em, 8.0f);
        break;
    case 1:
        work->moveAngBY = em->rotY;
        Em_MoveRotated(em, (s32 *) &work->moveAngBX);
        if (!(em->posY <= em->surfaceY)) {
            em->stance = 2;
            em->step++;
            EmPlesioth_SpawnSurfaceEffect5(em, 8.0f);
        }
        break;
    case 2:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, (s32 *) &work->moveAngBX);
        if (em->posY <= em->surfaceY) {
            em->step++;
            em->stance = 4;
        }
        break;
    case 3:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, (s32 *) &work->moveAngBX);
        if (em->posY < em->surfaceY) {
            em->step++;
            em->lookAtTarget = 0;
            em->stance = 4;
            Em_SetMotion(em, 0x82, 0, 0);
            Em_SetHitReactMode(em, 2);
            em->blind = 2;
        }
        break;
    case 4:
        Em_SetHitReactMode(em, 2);
        em->blind = 2;
        if (--em->timer <= 0) {
            em->step++;
            em->blind = 0;
            EmPlesioth_ChangeState(em, 2, 0x19, 4);
        }
        pos[0] = -150.0f;
        pos[1] = 50.0f;
        pos[2] = 140.0f;
        Em_SpawnEffect06Periodic(em, 0x22, pos, 1.6f);
        if (em->timer % 135 == 0) {
            Em_PlaySeAtBone(em, 0x57, 0x23);
        }
        break;
    case 5:
        if (em->remote == 0) {
            EmPlesioth_ChangeState(em, 2, 0x19, 4);
        }
        pos2[0] = -150.0f;
        pos2[1] = 50.0f;
        pos2[2] = 140.0f;
        Em_SpawnEffect06Periodic(em, 0x22, pos2, 1.6f);
        if (em->timer % 135 == 0) {
            Em_PlaySeAtBone(em, 0x57, 0x23);
        }
        break;
    }
}

void EmPlesioth_State2_15(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        EmPlesioth_SetMode(em, 4, 0);
        Em_SetMotion(em, 0x83, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmPlesioth_EndState2(em);
        }
        break;
    }
}

void EmPlesioth_State2_16(Em *em)
{
    EmPlesioth_ChangeState(em, 0x2, 0, 0x4);
}

void EmPlesioth_State2_17(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x4B, 0, 0);
        EmPlesioth_SetMode(em, 0, 0);
        em->noReact = 1;
        em->holdTimer = 0;
        em->unk8AD = 0;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x80, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x7C, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmPlesioth_EndState(em);
        }
        break;
    }
}

void EmPlesioth_State2_18(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        EmPlesioth_SetMode(em, 4, 0);
        em->blind = 2;
        Em_SetMotion(em, 0x83, 0, 0);
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            EmPlesioth_EndState2(em);
        }
        break;
    }
}

void EmPlesioth_State3_00(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        EmPlesioth_SetMode(em, 0, 0);
        Em_SetMotion(em, 0x24, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmPlesioth_EndState(em);
        }
        break;
    }
}

void EmPlesioth_State3_01(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        EmPlesioth_SetMode(em, 0, 0);
        Em_SetMotion(em, 0x2F, 0, 0);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 78.0f, 0U) != 0) {
            EffEm08_CreateAtBone2(em, 0x22, 1, 0, 0x444, 0xF1C8, (s32) (11.0f * em->motionSpeed));
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmPlesioth_EndState(em);
        }
        break;
    }
}

void EmPlesioth_State3_02(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmPlesioth_SetMode(em, 0, 0);
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
            EmPlesioth_EndState(em);
        }
        break;
    }
}

void EmPlesioth_State3_03(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x74, 0, 0);
        EmPlesioth_SetMode(em, 4, 0);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 114.0f, 0U) != 0) {
            EffEm08_CreateAtBone2(em, 0x22, 1, 0, 0x4FA, 0, (s32) (46.0f * em->motionSpeed));
        }
        if (Player_PassedFrameScaled(em, 30.0f, 0U) != 0) {
            EmPlesioth_SpawnSurfaceEffect5(em, 8.0f);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmPlesioth_EndState2(em);
        }
        break;
    }
}

void EmPlesioth_State3_04(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x75, 0, 0);
        EmPlesioth_SetMode(em, 4, 0);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 114.0f, 0U) != 0) {
            EffEm08_CreateAtBone2(em, 0x22, 1, 0, 0x4FA, 0, (s32) (46.0f * em->motionSpeed));
        }
        if (Player_PassedFrameScaled(em, 30.0f, 0U) != 0) {
            EmPlesioth_SpawnSurfaceEffect5(em, 8.0f);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmPlesioth_EndState2(em);
        }
        break;
    }
}

void EmPlesioth_State3_05(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x70, 0, 0);
        EmPlesioth_SetMode(em, 4, 0);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 118.0f, 0U) != 0) {
            EffEm08_CreateAtBone2(em, 0x22, 1, 1, 0x71C, 0, (s32) (30.0f * em->motionSpeed));
        }
        if (Player_PassedFrameScaled(em, 30.0f, 0U) != 0) {
            EmPlesioth_SpawnSurfaceEffect5(em, 8.0f);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmPlesioth_EndState2(em);
        }
        break;
    }
}

void EmPlesioth_State3_06(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0x0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x6C, 0, 0);
        EmPlesioth_SetMode(em, 4, 0);
        Player_ClearVelocity(em);
        em->velZ = 100.0f;
        em->velY = 100.0f;
        em->accelY = -5.0f;
        work->moveAngBX = 0;
        work->moveAngBZ = 0;
        em->timer = 0x12C;
        break;
    case 0x1:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, (s32 *) &work->moveAngBX);
        if (!(em->posY < em->surfaceY)) {
            em->step++;
            em->stance = 2;
        }
        if (em->motion[0].playing == 0) {
            em->step = 0x63;
            EmPlesioth_EndState2(em);
        }
        break;
    case 0x2:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, (s32 *) &work->moveAngBX);
        floorY = em->floorY;
        if (em->posY < floorY) {
            em->posY = floorY;
            em->accelY = 0.0f;
        }
        if (em->posY <= em->surfaceY) {
            em->step++;
            em->stance = 4;
            em->posY = em->surfaceY;
            em->velY = 0.0f;
            em->accelY = 0.0f;
            Em_SetMotion(em, 0x67, 0xA, 0);
            em->goalDist = 1000.0f;
            em->timer = 0x3C;
            em->accelZ = (em->goalDist - em->timer * em->velZ) / ((em->timer * em->timer) / 2);
            EmPlesioth_SpawnSurfaceEffect5(em, 8.0f);
            break;
        }
        if (--em->timer <= 0) {
            em->step = 0x63;
            em->posY = em->floorY;
            EmPlesioth_ChangeState(em, 0, 4, 4);
        }
        break;
    case 0x3:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, (s32 *) &work->moveAngBX);
        em->goalDist -= em->velZ;
        if ((em->goalDist <= 0.0f) || (em->velZ <= 0.0f)) {
            em->step++;
            EmPlesioth_EndState2(em);
        }
        break;
    case 0x63:
        if (em->remote == 0) {
            em->posY = em->floorY;
            EmPlesioth_ChangeState(em, 0, 4, 4);
        }
        break;
    }
}

void EmPlesioth_State3_07(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        EmPlesioth_SetMode(em, 0, 0);
        Em_SetMotion(em, 0x88, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmPlesioth_EndState(em);
        }
        break;
    }
}

void EmPlesioth_State4_00(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x3C, 0, 0);
        break;
    case 1:
        if (em->motion[em->stateVar1].playing == 0) {
            em->step++;
            EmPlesioth_EndState(em);
            break;
        }
        if (em->motion[1].playing == 0) {
            Em_SetMotion(em, 1, 0, 0);
        }
        if (em->motion[2].playing == 0) {
            Em_SetMotion(em, 1, 0, 0);
        }
        break;
    }
}

void EmPlesioth_State4_01(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x42, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmPlesioth_EndState(em);
        }
        break;
    }
}

void EmPlesioth_State4_02(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x3F, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmPlesioth_EndState(em);
        }
        break;
    }
}

void EmPlesioth_State4_03(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x40, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmPlesioth_EndState(em);
        }
        break;
    }
}

void EmPlesioth_State4_04(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        EmPlesioth_SetMode(em, 0, 0);
        Em_RestartScript(em);
        Em_SetMotion(em, 0x4B, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x80, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x7C, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmPlesioth_EndState(em);
        }
        break;
    }
}

void EmPlesioth_State4_05(Em *em, EmWork *work, s16 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, (s16) ((variant & 1) == 0 ? 0x4A : 0x45), 0, 0);
        switch (variant) {
        case 2:
        case 3:
            StageObj_Stun_CreateEm(em, 1, 0x22, em->timer + em->motion[0].frameCount / 2.0f, 150.0f, 3.0f);
            break;
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, (s16) ((variant & 1) == 0 ? 0x4B : 0x46), 0, 0);
            em->stateVar1 = 0;
        }
        break;
    case 2:
        switch (variant) {
        case 2:
        case 3:
            if (--em->timer <= 0) {
                em->stateVar1 = 1;
            }
            break;
        default:
            if (em->motion[0].playing == 0) {
                em->stateVar1 = 1;
            }
            break;
        }
        if (em->stateVar1) {
            em->step++;
            Em_SetMotion(em, (s16) ((variant & 1) == 0 ? 0x81 : 0x80), 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 0;
            Em_SetMotion(em, (s16) ((variant & 1) == 0 ? 0x7C : 0x47), 0, 0);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
            switch (variant) {
            case 2:
            case 3:
                em->step++;
                EmPlesioth_ChangeState(em, 0, 0x16, 4);
                break;
            default:
                EmPlesioth_EndState(em);
                break;
            }
        }
        break;
    case 5:
        if (em->remote == 0) {
            EmPlesioth_ChangeState(em, 0, 0x16, 4);
        }
        break;
    }
}

void EmPlesioth_State4_06(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x7E, 0, 0);
        EmPlesioth_SetMode(em, 2, 0);
        em->accelX = 0.0f;
        em->accelY = -10.0f;
        em->accelZ = 0.0f;
        work->moveAngBX = 0;
        work->moveAngBZ = 0;
        break;
    case 1:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, &work->moveAngBX);
        if (!(em->posY < em->surfaceY)) {
            em->stance = 2;
        } else {
            em->stance = 4;
        }
        if (EmPlesioth_OverDeepWater(em, 1) == 1) {
            if (em->posY < em->surfaceY) {
                em->stance = 4;
                em->posY = em->surfaceY;
                EmPlesioth_EndState2(em);
            }
        } else if (em->posY < em->floorY) {
            em->step++;
            em->stance = 0;
            em->posY = em->floorY;
            EmPlesioth_ChangeState(em, 4, 0x10, 3);
        }
        break;
    case 2:
        if (em->remote == 0) {
            em->stance = 0;
            em->posY = em->floorY;
            EmPlesioth_ChangeState(em, 4, 0x10, 3);
        }
        break;
    }
}

void EmPlesioth_State4_07(Em *em)
{
    Em_SetHitReactMode(em, 4U);
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x4B, 0, 0);
        EmPlesioth_SetMode(em, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x80, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x8A, 0, 0);
        }
        break;
    case 3:
        if (--em->timer <= 0) {
            em->step++;
            EmPlesioth_ChangeState(em, 4, 0x15, 3);
        }
        Em_SpawnEffect06Bone(em, 2);
        break;
    case 4:
        Em_SpawnEffect06Bone(em, 2);
        if (em->remote == 0) {
            EmPlesioth_ChangeState(em, 4, 0x15, 3);
        }
        break;
    }
}

void EmPlesioth_State4_08(Em *em)
{
    em->holdTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
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
            EmPlesioth_ChangeState(em, 2, 0x17, 4);
        }
        break;
    case 2:
        if ((em->motion[0].playing == 0) && (em->remote == 0)) {
            Em_SetMotion(em, 0x60, 0, 0);
            EmPlesioth_ChangeState(em, 2, 0x17, 4);
        }
        break;
    }
}

void EmPlesioth_State4_09(Em *em, EmWork *work)
{
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x7E, 0, 0);
        EmPlesioth_SetMode(em, 2, 0);
        em->accelX = 0.0f;
        em->accelY = -10.0f;
        em->accelZ = 0.0f;
        work->moveAngBX = 0;
        work->moveAngBZ = 0;
        break;
    case 1:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, &work->moveAngBX);
        if (!(em->posY < em->surfaceY)) {
            em->stance = 2;
        } else {
            em->stance = 4;
        }
        if (EmPlesioth_OverDeepWater(em, 1) == 1) {
            if (em->posY < em->surfaceY) {
                em->stance = 4;
                em->posY = em->surfaceY;
                EmPlesioth_EndState2(em);
            }
        } else if (em->posY < em->floorY) {
            em->step++;
            em->stance = 0;
            em->posY = em->floorY;
            EmPlesioth_ChangeState(em, 4, 0x10, 3);
        }
        break;
    case 2:
        if (em->remote == 0) {
            em->stance = 0;
            em->posY = em->floorY;
            EmPlesioth_ChangeState(em, 4, 0x10, 3);
        }
        break;
    }
}

void EmPlesioth_State4_0A(Em *em)
{
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x3D, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmPlesioth_EndState(em);
        }
        break;
    }
}

void EmPlesioth_State4_0B(Em *em)
{
    Em_SetHitReactMode(em, 4U);
    switch (em->step) {
    case 0:
        em->step++;
        EmPlesioth_SetMode(em, 0, 0);
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x4C, 0, 0);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmPlesioth_ChangeState(em, 0, 0x14, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmPlesioth_ChangeState(em, 0, 0x14, 4);
        }
        break;
    }
    Em_SpawnEffect06Bone(em, 2);
}

void EmPlesioth_State4_0C(Em *em)
{
    Em_SetHitReactMode(em, 3U);
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x5F, 0, 0);
        em->noReact = 1;
        em->holdTimer = 0x32;
        em->blind = 0;
        break;
    case 1:
        if ((s8) em->holdTimer < 5) {
            em->holdTimer = 5;
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmPlesioth_ChangeState(em, 4, 0xD, 3);
        }
        break;
    case 2:
        if ((s8) em->holdTimer < 5) {
            em->holdTimer = 5;
        }
        if (em->remote == 0) {
            EmPlesioth_ChangeState(em, 4, 0xD, 3);
        }
        break;
    }
}

void EmPlesioth_State4_0D(Em *em)
{
    em->holdTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
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
            EmPlesioth_ChangeState(em, 2, 0x17, 4);
        }
        break;
    case 2:
        if ((em->motion[0].playing == 0) && (em->remote == 0)) {
            Em_SetMotion(em, 0x60, 0, 0);
            EmPlesioth_ChangeState(em, 2, 0x17, 4);
        }
        break;
    }
}

void EmPlesioth_State4_0E(Em *em, s32 unused, s32 variant)
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
        EmPlesioth_SetMode(em, 0, 0);
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
                EmPlesioth_ChangeState(em, 4, 0x1B, 3);
            } else {
                EmPlesioth_ChangeState(em, 4, 8, 3);
            }
        }
        break;
    case 2:
        if (em->remote == 0) {
            if (k == 1) {
                EmPlesioth_ChangeState(em, 4, 0x1B, 3);
            } else {
                EmPlesioth_ChangeState(em, 4, 8, 3);
            }
        }
        break;
    }
    if (k != 1) {
        Em_SpawnEffect06Bone(em, 2);
    }
}

void EmPlesioth_State4_0F(Em *em, EmWork *work)
{
    Em_SetHitReactMode(em, 3);
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        EmPlesioth_SetMode(em, 4, 0);
        Player_ClearVelocity(em);
        Em_SetMotion(em, 0x7E, 0, 0);
        em->rotY = Math_AngleXZ(&em->posX, em->goal);
        em->hooked = 0;
        em->timer = 30;
        em->accelY = -10.0f;
        em->velY = (em->goal[1] - em->posY) / (f32) em->timer - (em->accelY * (f32) em->timer) / 2.0f;
        em->velZ = Vec_DistanceXZ(&em->posX, em->goal) / (f32) em->timer;
        Em_SetCombat(em, 1, gEmCombatInit[em->type]);
        work->moveAngBX = 0;
        work->moveAngBZ = 0;
        Em_RestartScript(em);
        em->blind = 0;
        break;
    case 1:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, (s32 *) &work->moveAngBX);
        if (!(em->posY < em->surfaceY)) {
            em->step++;
            em->stance = 2;
            EmPlesioth_SpawnSurfaceEffect5(em, 8.0f);
        }
        break;
    case 2:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, (s32 *) &work->moveAngBX);
        if (em->velY < 0.0f) {
            if (em->posY < em->floorY) {
                em->step++;
                em->stance = 0;
                em->posY = em->floorY;
                Em_SetMotion(em, 0x79, 0, 0);
            }
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            EmMine_CreateTimed0(em, &em->posX);
            em->hp -= 100;
            if (em->remote == 0 && em->hp <= 0) {
                em->hp = 0;
                em->step = 0x63;
                EmPlesioth_ChangeState(em, 5, 3, 3);
                break;
            }
            em->step++;
            Em_SetMotion(em, 0x4B, 0, 0);
            em->timer = 300;
        }
        break;
    case 4:
        if (--em->timer <= 0) {
            em->step++;
        }
        break;
    case 5:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x80, 0, 0);
        }
        break;
    case 6:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x7C, 0, 0);
        }
        break;
    case 7:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_CheckRageByHp(em, em->rageThreshold);
            EmPlesioth_EndState(em);
        }
        break;
    }
}

void EmPlesioth_State4_10(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        EmPlesioth_SetMode(em, 0, 0);
        Em_SetMotion(em, 0x79, 0xC, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x7C, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmPlesioth_EndState(em);
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
            Player_ClearVelocity(em);
        }
        break;
    }
}

void EmPlesioth_State4_11(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x7E, 0, 0);
        Player_ClearVelocity(em);
        em->velY = 60.0f;
        em->accelY = -2.75f;
        work->moveAngBX = 0;
        work->moveAngBZ = 0;
        break;
    case 1:
        work->moveAngBY = em->rotY;
        Em_MoveRotated(em, &work->moveAngBX);
        if (!(em->posY <= em->surfaceY)) {
            em->stance = 2;
            em->step++;
            EmPlesioth_SpawnSurfaceEffect5(em, 8.0f);
        }
        break;
    case 2:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, &work->moveAngBX);
        if (em->posY <= em->surfaceY) {
            em->step++;
            em->stance = 4;
            EmPlesioth_SpawnSurfaceEffect5(em, 8.0f);
        }
        break;
    case 3:
        if (EmPlesioth_OverDeepWater(em, 1U) == 1) {
            if (em->posY <= em->surfaceY) {
                em->stance = 4;
                em->posY = em->surfaceY;
                EmPlesioth_EndState2(em);
                break;
            }
            work->moveAngBY = em->rotY;
            Em_MoveRotatedAccel(em, &work->moveAngBX);
            if (em->stance == 2) {
                if (!(em->posY <= em->surfaceY)) {
                    em->stance = 2;
                    break;
                }
                em->stance = 4;
                break;
            }
        } else if (em->posY < em->floorY) {
            em->step++;
            em->stance = 0;
            em->posY = em->floorY;
            EmPlesioth_ChangeState(em, 4, 0x10, 3);
        }
        break;
    case 4:
        if (em->remote == 0) {
            em->stance = 0;
            em->posY = em->floorY;
            EmPlesioth_ChangeState(em, 4, 0x10, 3);
        }
        break;
    }
}

void EmPlesioth_State4_12(Em *em, EmWork *work)
{
    Em_SetHitReactMode(em, 4U);
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x7E, 0, 0);
        Player_ClearVelocity(em);
        em->velY = 60.0f;
        em->accelY = -2.75f;
        work->moveAngBX = 0;
        work->moveAngBZ = 0;
        em->noReact = 1;
        break;
    case 1:
        work->moveAngBY = em->rotY;
        Em_MoveRotated(em, (s32 *) &work->moveAngBX);
        if (!(em->posY <= em->surfaceY)) {
            em->stance = 2;
            em->step++;
            EmPlesioth_SpawnSurfaceEffect5(em, 8.0f);
            break;
        }
    default:
        break;
    case 2:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, (s32 *) &work->moveAngBX);
        if (em->posY <= em->surfaceY) {
            em->step++;
            em->stance = 4;
            EmPlesioth_SpawnSurfaceEffect5(em, 8.0f);
        }
        break;
    case 3:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, (s32 *) &work->moveAngBX);
        if (em->posY < em->surfaceY) {
            em->step++;
            em->posY = em->surfaceY;
            em->lookAtTarget = 0;
            em->stance = 4;
            Em_SetMotion(em, 0x82, 0, 0);
        }
        break;
    case 4:
        if (--em->timer <= 0) {
            em->step++;
            EmPlesioth_ChangeState(em, 2, 0x15, 4);
        }
        Em_SpawnEffect06Bone(em, 2);
        break;
    case 5:
        if (em->remote == 0) {
            EmPlesioth_ChangeState(em, 2, 0x15, 4);
        }
        Em_SpawnEffect06Bone(em, 2);
        break;
    }
}

void EmPlesioth_State4_13(Em *em)
{
    em->holdTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        EmPlesioth_SetMode(em, 0, 0);
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
            EmPlesioth_ChangeState(em, 4, 0xD, 3);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmPlesioth_ChangeState(em, 4, 0xD, 3);
        }
        break;
    }
}

void EmPlesioth_State4_15(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x7C, 0, 0);
        EmPlesioth_SetMode(em, 1, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmPlesioth_EndState(em);
        }
        break;
    }
}

void EmPlesioth_State4_16(Em *em, EmWork *work)
{
    f32 pos[3];
    f32 pos2[3];

    Em_SetHitReactMode(em, 2);
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x7E, 0, 0);
        Player_ClearVelocity(em);
        em->velY = 60.0f;
        em->accelY = -2.75f;
        work->moveAngBX = 0;
        work->moveAngBZ = 0;
        EmPlesioth_SpawnSurfaceEffect5(em, 8.0f);
        break;
    case 1:
        work->moveAngBY = em->rotY;
        Em_MoveRotated(em, (s32 *) &work->moveAngBX);
        if (!(em->posY <= em->surfaceY)) {
            em->stance = 2;
            em->step++;
            EmPlesioth_SpawnSurfaceEffect5(em, 8.0f);
        }
        break;
    case 2:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, (s32 *) &work->moveAngBX);
        if (em->posY <= em->surfaceY) {
            em->step++;
            em->stance = 4;
        }
        break;
    case 3:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, (s32 *) &work->moveAngBX);
        if (em->posY < em->surfaceY) {
            em->step++;
            em->lookAtTarget = 0;
            em->stance = 4;
            Em_SetMotion(em, 0x82, 0, 0);
            em->blind = 2;
            Em_SetHitReactMode(em, 2);
        }
        break;
    case 4:
        Em_SetHitReactMode(em, 2);
        em->blind = 2;
        if (--em->timer <= 0) {
            em->step++;
            em->blind = 0;
            EmPlesioth_ChangeState(em, 2, 0x19, 4);
        }
        pos[0] = -150.0f;
        pos[1] = 50.0f;
        pos[2] = 140.0f;
        Em_SpawnEffect06Periodic(em, 0x22, pos, 1.6f);
        if (em->timer % 135 == 0) {
            Em_PlaySeAtBone(em, 0x57, 0x23);
        }
        break;
    case 5:
        if (em->remote == 0) {
            EmPlesioth_ChangeState(em, 2, 0x19, 4);
        }
        pos2[0] = -150.0f;
        pos2[1] = 50.0f;
        pos2[2] = 140.0f;
        Em_SpawnEffect06Periodic(em, 0x22, pos2, 1.6f);
        if (em->timer % 135 == 0) {
            Em_PlaySeAtBone(em, 0x57, 0x23);
        }
        break;
    }
}

void EmPlesioth_State4_18(Em *em)
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
            EmPlesioth_ChangeState(em, 0, 0x15, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmPlesioth_ChangeState(em, 0, 0x15, 4);
        }
        break;
    }
}

void EmPlesioth_State6_00_Captured(Em *em, EmWork *work)
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
                EmPlesioth_ChangeState(em, 5, 4, 4);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    }
    pos[0] = 0.0f;
    pos[1] = 10.0f;
    pos[2] = 140.0f;
    Em_SpawnEffect06Periodic(em, 0x22, pos, 1.6f);
    if (++em->timer % 135 == 0) {
        Em_PlaySeAtBone(em, 0x57, 0x23);
    }
}

void EmPlesioth_State6_01_Entrance(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->posX = 6694.0f;
        em->posY = -550.0f;
        em->posZ = 17459.0f;
        em->goal[0] = 10412.0f;
        em->goal[1] = -550.0f;
        em->goal[2] = 12820.0f;
        em->rotX = 0;
        em->rotY = Math_AngleXZ(&em->posX, em->goal);
        em->rotZ = 0;
        em->goalDist = Vec_DistanceXZ(&em->posX, em->goal);
        Player_ClearVelocity(em);
        Em_SetMotion(em, 0x82, 0, 0);
        work->moveAngBX = 0;
        work->moveAngBZ = 0;
        em->timer = 180;
        em->fieldCollide = 0;
        break;
    case 1:
        em->fieldCollide = 1;
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x83, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->posX = 6649.0f;
            em->posY = -550.0f;
            em->posZ = 17459.0f;
            em->goal[0] = 10412.0f;
            em->goal[1] = -550.0f;
            em->goal[2] = 12820.0f;
            em->rotX = 0;
            em->rotY = Math_AngleXZ(&em->posX, em->goal);
            em->rotZ = 0;
            em->goalDist = Vec_DistanceXZ(&em->posX, em->goal);
            Em_SetMotion(em, 0x66, 0, 0);
            work->moveAngBX = 0;
            work->moveAngBZ = 0;
            em->timer = 30;
        }
        break;
    case 3:
        if (--em->timer <= 0) {
            em->step++;
            em->velZ = 30.0f;
        }
        break;
    case 4:
        work->moveAngBY = em->rotY;
        Em_MoveRotated(em, (s32 *) &work->moveAngBX);
        em->goalDist -= em->velZ;
        if (em->goalDist <= 0.0f) {
            em->step++;
            Player_ClearVelocity(em);
            em->posX = 15251.0f;
            em->posY = -230.0f;
            em->posZ = 13531.0f;
            em->goal[0] = 14094.0f;
            em->goal[1] = -230.0f;
            em->goal[2] = 13301.0f;
            em->rotX = 0;
            em->rotY = Math_AngleXZ(&em->posX, em->goal);
            em->rotZ = 0;
            em->goalDist = Vec_DistanceXZ(&em->posX, em->goal);
            work->moveAngBX = 0;
            work->moveAngBZ = 0;
            em->velZ = 20.0f;
        }
        EmPlesioth_SpawnSurfaceEffect2(em, 8.0f);
        break;
    case 5:
        work->moveAngBY = em->rotY;
        Em_MoveRotated(em, (s32 *) &work->moveAngBX);
        em->goalDist -= em->velZ;
        if (em->goalDist <= 0.0f) {
            em->step++;
            Player_ClearVelocity(em);
            em->goal[0] = 10191.0f;
            em->goal[1] = -200.0f;
            em->goal[2] = 12523.0f;
            em->rotX = 0;
            em->rotY = Math_AngleXZ(&em->posX, em->goal);
            em->rotZ = 0;
            em->goalDist = Vec_DistanceXZ(&em->posX, em->goal);
            em->velZ = 50.0f;
            Em_SetMotion(em, 0x67, 0, 0);
        }
        EmPlesioth_SpawnSurfaceEffect2(em, 8.0f);
        break;
    case 6:
        work->moveAngBY = em->rotY;
        Em_MoveRotated(em, (s32 *) &work->moveAngBX);
        em->goalDist -= em->velZ;
        if (em->goalDist <= 0.0f) {
            em->step++;
            Player_ClearVelocity(em);
            em->posX = 10679.0f;
            em->posY = -550.0f;
            em->posZ = 11603.0f;
            em->goal[0] = 10000.0f;
            em->goal[1] = -550.0f;
            em->goal[2] = 8560.0f;
            em->rotX = 0;
            em->rotY = Math_AngleXZ(&em->posX, em->goal) - 0x3000;
            em->rotZ = 0;
            Em_SetMotion(em, 0x72, 0, 0);
            work->moveAngBX = 0;
            work->moveAngBZ = 0;
            em->timer = 90;
            em->velZ = 60.0f;
        }
        EmPlesioth_SpawnSurfaceEffect2(em, 8.0f);
        break;
    case 7:
        em->rotY += 0x100;
        em->rotY = (u16) em->rotY;
        work->moveAngBX = 0;
        work->moveAngBY = em->rotY;
        work->moveAngBZ = 0;
        Em_MoveRotated(em, (s32 *) &work->moveAngBX);
        if (--em->timer <= 0) {
            em->step++;
            Player_ClearVelocity(em);
            em->posX = 5000.0f;
            em->posY = -550.0f;
            em->posZ = 7950.0f;
            em->goal[0] = 5570.0f;
            em->goal[1] = -550.0f;
            em->goal[2] = 8900.0f;
            em->rotX = 0;
            em->rotY = Math_AngleXZ(&em->posX, em->goal) - 0x3000;
            em->rotZ = 0;
            Em_SetMotion(em, 0x72, 0, 0);
            work->moveAngBX = 0;
            work->moveAngBZ = 0;
            em->timer = 45;
            em->velZ = 20.0f;
        }
        EmPlesioth_SpawnSurfaceEffect2(em, 8.0f);
        break;
    case 8:
        em->rotY += 0x180;
        em->rotY = (u16) em->rotY;
        work->moveAngBX = 0;
        work->moveAngBY = em->rotY;
        work->moveAngBZ = 0;
        Em_MoveRotated(em, (s32 *) &work->moveAngBX);
        if (--em->timer <= 0) {
            em->step++;
            em->goal[0] = 6600.0f;
            em->goal[1] = -550.0f;
            em->goal[2] = 9330.0f;
            em->rotX = 0;
            em->rotY = Math_AngleXZ(&em->posX, em->goal);
            em->rotZ = 0;
            em->goalDist = Vec_DistanceXZ(&em->posX, em->goal);
            Em_SetMotion(em, 0x6C, 0, 0);
            em->velZ = 50.0f;
        }
        EmPlesioth_SpawnSurfaceEffect2(em, 8.0f);
        break;
    case 9:
        work->moveAngBY = em->rotY;
        Em_MoveRotated(em, (s32 *) &work->moveAngBX);
        em->goalDist -= em->velZ;
        if (em->goalDist <= 0.0f) {
            em->step++;
            em->goal[0] = 9120.0f;
            em->goal[1] = -550.0f;
            em->goal[2] = 10400.0f;
            em->rotX = 0;
            em->rotY = Math_AngleXZ(&em->posX, em->goal);
            em->rotZ = 0;
            em->goalDist = Vec_DistanceXZ(&em->posX, em->goal);
            Em_SetMotion(em, 0x67, 0, 0);
            em->velZ = 30.0f;
            em->velY = -5.0f;
        }
        EmPlesioth_SpawnSurfaceEffect2(em, 8.0f);
        break;
    case 10:
        work->moveAngBY = em->rotY;
        Em_MoveRotated(em, (s32 *) &work->moveAngBX);
        em->goalDist -= em->velZ;
        if (em->goalDist <= 0.0f) {
            em->step++;
            Em_SetMotion(em, 1, 0, 0);
            Player_ClearVelocity(em);
            em->timer = 210;
        }
        EmPlesioth_SpawnSurfaceEffect2(em, 8.0f);
        break;
    case 11:
        if (--em->timer <= 0) {
            em->step++;
            em->posX = 9100.0f;
            em->posY = -600.0f;
            em->posZ = 10350.0f;
            em->goal[0] = 11131.0f;
            em->goal[1] = -600.0f;
            em->goal[2] = 11228.0f;
            em->rotX = 0;
            em->rotY = Math_AngleXZ(&em->posX, em->goal);
            em->rotZ = 0;
            Em_SetMotion(em, 0x75, 0, 0);
        }
        EmPlesioth_SpawnSurfaceEffect2(em, 8.0f);
        break;
    case 12:
        if (Player_PassedFrameScaled(em, 114.0f, 0)) {
            EffEm08_CreateAtBone2(em, 0x22, 1, 0, 0x71C, 0, (s32) (46.0f * em->motionSpeed));
        }
        if (Player_PassedFrameScaled(em, 30.0f, 0)) {
            EmPlesioth_SpawnSurfaceEffect5(em, 8.0f);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 1, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    case 13:
        em->step++;
        EmPlesioth_EndState2(em);
        break;
    }
}

void EmPlesioth_State6_02_Captured(Em *em, EmWork *work)
{
    s32 timer;
    s32 t;

    Em_SetCaptured(em, 1);
    switch (em->step) {
    case 0:
        em->step++;
        QuestEm_Captured(em);
        em->timer = 0x708;
        Em_BeginStatus1(em);
        Em_ClearPoison(em);
        em->stance = 0;
        EmPlesioth_SetMode(em, 0, 1);
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x26, 0, 0);
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
                EmPlesioth_ChangeState(em, 5, 4, 4);
                break;
            }
        } else {
            em->alpha = (f32) timer / 150.0f;
        }
        break;
    }
}

void EmPlesioth_State5_00_Hunted(Em *em, EmWork *work)
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
                EmPlesioth_ChangeState(em, 5, 4, 4);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    }
}

void EmPlesioth_State5_01_Hunted(Em *em, EmWork *work)
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
            em->timer = 450;
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
                EmPlesioth_ChangeState(em, 5, 4, 4);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    }
}

void EmPlesioth_State5_02_HuntedFalling(Em *em, EmWork *work)
{
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x7E, 0, 0);
        Player_ClearVelocity(em);
        em->velY = 60.0f;
        em->accelY = -2.75f;
        work->moveAngBX = 0;
        work->moveAngBZ = 0;
        break;
    case 1:
        work->moveAngBY = em->rotY;
        Em_MoveRotated(em, &work->moveAngBX);
        if (!(em->posY <= em->surfaceY)) {
            em->step++;
            em->stance = 2;
            EmPlesioth_SpawnSurfaceEffect5(em, 8.0f);
        }
        break;
    case 2:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, &work->moveAngBX);
        if (em->posY <= em->surfaceY) {
            em->step++;
            em->stance = 4;
            EmPlesioth_SpawnSurfaceEffect5(em, 8.0f);
        }
        break;
    case 3:
        if (em->posY < em->surfaceY) {
            em->step++;
            em->stance = 4;
            em->accelY = 1.0f;
            Em_SetMotion(em, 0x6F, 0, 0);
            QuestEm_Hunted(em);
            *(f32 *) &em->surfaceDepth = 444.3f;
        } else {
            work->moveAngBY = em->rotY;
            Em_MoveRotatedAccel(em, &work->moveAngBX);
            if (em->stance == 2) {
                if (!(em->posY < em->surfaceY)) {
                    em->stance = 2;
                } else {
                    em->stance = 4;
                }
            }
        }
        break;
    case 4:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, &work->moveAngBX);
        if (!(em->velY < 5.0f)) {
            em->velY = 5.0f;
            em->accelY = 0.0f;
        }
        if (!(em->posY < em->surfaceY)) {
            em->posY = em->surfaceY;
            *(s32 *) &em->motionSpeed = 0;
            em->step++;
            em->stateVar0 = 0;
            EmMine_Create(em, 0);
            em->fieldCollide = 0;
        }
        break;
    case 5:
        switch (em->stateVar0) {
        case 0:
            em->stateVar0++;
            *(f32 *) &em->velY = -0.5f;
            em->timer = (Rand_Next(1) & 3) * 10;
            break;
        case 1:
            work->moveAngBY = em->rotY;
            Em_MoveRotated(em, &work->moveAngBX);
            if (--em->timer <= 0) {
                em->stateVar0++;
                *(f32 *) &em->velY = 1.0f;
            }
            break;
        case 2:
            work->moveAngBY = em->rotY;
            Em_MoveRotated(em, &work->moveAngBX);
            if (!(em->posY < em->surfaceY)) {
                em->posY = em->surfaceY;
                em->stateVar0 = 0;
            }
            break;
        }
        EmMine_Update(em);
        if (QuestEm_CanAppear(em) != 0) {
            if (--em->timer <= 0) {
                em->step++;
                EmMine_Remove(em);
                em->timer = 150;
            }
        }
        break;
    case 6:
        if (em->timer > 0) {
            em->timer--;
        }
        if (em->timer <= 0) {
            if (em->remote == 0) {
                EmPlesioth_ChangeState(em, 5, 4, 4);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    }
}

void EmPlesioth_State5_03_Hunted(Em *em, EmWork *work)
{
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x4B, 0, 0);
        em->lookAtTarget = 0;
        em->stance = 3;
        QuestEm_Hunted(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x80, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x7F, 0, 0);
        }
    case 3:
        if (em->motion[0].playing == 0) {
            em->timer = 450;
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
                EmPlesioth_ChangeState(em, 5, 4, 4);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    }
}

void EmPlesioth_RunGroup0(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmPlesioth_State0_00(em, work);
        break;
    case 1:
        EmPlesioth_State0_01(em, work);
        break;
    case 2:
        EmPlesioth_State0_02(em, work);
        break;
    case 4:
        EmPlesioth_State0_04(em, work, 0);
        break;
    case 5:
        EmPlesioth_State0_05(em, (s32) work, 0);
        break;
    case 6:
        EmPlesioth_State0_06(em);
        break;
    case 7:
        EmPlesioth_State0_07(em);
        break;
    case 8:
        EmPlesioth_State0_08(em, work);
        break;
    case 9:
        EmPlesioth_State0_04(em, work, 1);
        break;
    case 10:
        EmPlesioth_State0_0A(em, work);
        break;
    case 12:
        EmPlesioth_State0_0C(em);
        break;
    case 13:
        EmPlesioth_State0_06(em);
        break;
    case 14:
        EmPlesioth_State0_0E(em);
        break;
    case 15:
        EmPlesioth_State0_0F(em);
        break;
    case 16:
        EmPlesioth_State0_10(em);
        break;
    case 17:
        EmPlesioth_State0_0F(em);
        break;
    case 18:
        EmPlesioth_State0_12(em);
        break;
    case 19:
        EmPlesioth_State0_13(em);
        break;
    case 20:
        EmPlesioth_State0_07(em);
        break;
    case 21:
        EmPlesioth_State0_07(em);
        break;
    case 22:
        EmPlesioth_State0_07(em);
        break;
    case 41:
        EmPlesioth_State0_05(em, (s32) work, 1);
        break;
    case 42:
        EmPlesioth_State0_06(em);
        break;
    }
}

void EmPlesioth_RunGroup1(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmPlesioth_State1_00(em, work);
        break;
    case 1:
        EmPlesioth_State1_01(em, work);
        break;
    case 2:
        EmPlesioth_State1_02(em, work);
        break;
    case 3:
        EmPlesioth_State1_03(em, work);
        break;
    }
}

void EmPlesioth_RunGroup2(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmPlesioth_State2_00(em);
        break;
    case 1:
        EmPlesioth_State2_01(em);
        break;
    case 2:
        EmPlesioth_State2_02();
        break;
    case 3:
        EmPlesioth_State2_03(em);
        break;
    case 4:
        EmPlesioth_State2_04(em, work);
        break;
    case 5:
        EmPlesioth_State2_05(em, work);
        break;
    case 6:
        EmPlesioth_State2_06(em);
        break;
    case 7:
        EmPlesioth_State2_07(em, work);
        break;
    case 8:
        EmPlesioth_State2_08(em, work);
        break;
    case 9:
        EmPlesioth_State2_09(em);
        break;
    case 10:
        EmPlesioth_State2_0A(em, work);
        break;
    case 11:
        EmPlesioth_State2_0B(em, work);
        break;
    case 12:
        EmPlesioth_State2_0C(em, work);
        break;
    case 13:
        EmPlesioth_State2_0D(em, work);
        break;
    case 14:
        EmPlesioth_State2_0E(em, work);
        break;
    case 15:
        EmPlesioth_State2_0F(em);
        break;
    case 16:
        EmPlesioth_State2_10(em, work, 0);
        break;
    case 17:
        EmPlesioth_State2_11(em, work);
        break;
    case 18:
        EmPlesioth_State2_12(em, work);
        break;
    case 19:
        EmPlesioth_State2_13(em, work);
        break;
    case 20:
        EmPlesioth_State2_14(em, work);
        break;
    case 21:
        EmPlesioth_State2_15(em);
        break;
    case 22:
        EmPlesioth_State2_16(em);
        break;
    case 23:
        EmPlesioth_State2_17(em);
        break;
    case 24:
        EmPlesioth_State2_18(em);
        break;
    case 25:
        EmPlesioth_State2_18(em);
        break;
    case 26:
        EmPlesioth_State2_10(em, work, 1);
        break;
    case 27:
        EmPlesioth_State2_00(em);
        
    default:
        break;
    }
}

void EmPlesioth_RunGroup3(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmPlesioth_State3_00(em, work);
        break;
    case 1:
        EmPlesioth_State3_01(em, work);
        break;
    case 2:
        EmPlesioth_State3_02(em, work);
        break;
    case 3:
        EmPlesioth_State3_03(em, work);
        break;
    case 4:
        EmPlesioth_State3_04(em, work);
        break;
    case 5:
        EmPlesioth_State3_05(em, work);
        break;
    case 6:
        EmPlesioth_State3_06(em, work);
        break;
    case 7:
        EmPlesioth_State3_07(em, work);
        
    default:
        break;
    }
}

void EmPlesioth_RunGroup4(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmPlesioth_State4_00(em);
        break;
    case 1:
        EmPlesioth_State4_01(em);
        break;
    case 2:
        EmPlesioth_State4_02(em);
        break;
    case 3:
        EmPlesioth_State4_03(em);
        break;
    case 4:
        EmPlesioth_State4_04(em);
        break;
    case 5:
        EmPlesioth_State4_05(em, work, 1);
        break;
    case 6:
        EmPlesioth_State4_06(em, work);
        break;
    case 7:
        EmPlesioth_State4_07(em);
        break;
    case 8:
        EmPlesioth_State4_08(em);
        break;
    case 9:
        EmPlesioth_State4_09(em, work);
        break;
    case 10:
        EmPlesioth_State4_0A(em);
        break;
    case 11:
        EmPlesioth_State4_0B(em);
        break;
    case 12:
        EmPlesioth_State4_0C(em);
        break;
    case 13:
        EmPlesioth_State4_0D(em);
        break;
    case 14:
        EmPlesioth_State4_0E(em, (s32) work, 0);
        break;
    case 15:
        EmPlesioth_State4_0F(em, work);
        break;
    case 16:
        EmPlesioth_State4_10(em);
        break;
    case 17:
        EmPlesioth_State4_11(em, work);
        break;
    case 18:
        EmPlesioth_State4_12(em, work);
        break;
    case 19:
        EmPlesioth_State4_13(em);
        break;
    case 20:
        EmPlesioth_State4_13(em);
        break;
    case 21:
        EmPlesioth_State4_15(em);
        break;
    case 22:
        EmPlesioth_State4_16(em, work);
        break;
    case 23:
        EmPlesioth_State4_05(em, work, 0);
        break;
    case 24:
        EmPlesioth_State4_18(em);
        break;
    case 25:
        EmPlesioth_State4_05(em, work, 3);
        break;
    case 26:
        EmPlesioth_State4_05(em, work, 2);
        break;
    case 27:
        EmPlesioth_State4_08(em);
        break;
    case 28:
        EmPlesioth_State4_0E(em, (s32) work, 1);
        break;
    }
}

void EmPlesioth_RunGroup5(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmPlesioth_State5_00_Hunted(em, work);
        break;
    case 1:
        EmPlesioth_State5_01_Hunted(em, work);
        break;
    case 2:
        EmPlesioth_State5_02_HuntedFalling(em, work);
        break;
    case 3:
        EmPlesioth_State5_03_Hunted(em, work);
        break;
    }
}

void EmPlesioth_RunGroup6(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmPlesioth_State6_00_Captured(em, work);
        break;
    case 1:
        EmPlesioth_State6_01_Entrance(em, work);
        break;
    case 2:
        EmPlesioth_State6_02_Captured(em, work);
        break;
    }
}

void EmPlesioth_Update(Em *em)
{
    u8 buf[4];
    u8 *w = &em->work;

    em->moveFrozen = 0;
    if (em->remote == 0 && em->taskPhase == 1) {
        if (em->stance == 0) {
            Em_HoldPosNoPlayers(em, 0, 1);
        } else if (em->stance == 4) {
            Em_HoldPosNoPlayers(em, 2, 0);
        }
    } else if (em->area == gScene.area) {
        switch (em->stance) {
        case 0:
            if (EmPlesioth_OverDeepWater(em, 1) == 1) {
                EmPlesioth_ChangeState(em, 2, 0, 0);
            }
            break;
        case 4:
            if (EmPlesioth_OverDeepWater(em, 1) == 0) {
                EmPlesioth_ChangeState(em, 0, 1, 0);
            }
            break;
        }
    }
    Em_UpdateCombatTimer(em);
    switch (Em_UpdateDamage(em, buf) & 0xFF) {
    case 0:
    case 9:
    case 11:
        break;
    case 1:
    case 2:
        if (em->stance != 2) {
            if (em->stance == 4) {
                EmPlesioth_ChangeState(em, 5, 2, 3);
            } else if ((s8) em->holdTimer != 0) {
                EmPlesioth_ChangeState(em, 5, 1, 3);
            } else {
                EmPlesioth_ChangeState(em, 5, 0, 3);
            }
        }
        break;
    case 3:
    case 4:
        if ((s8) em->holdTimer == 0) {
            Em_SetGoal12Dist(em, buf);
            Em_SetGoal12(em);
            EmPlesioth_ChangeState(em, 4, 0xC, 3);
        }
        break;
    case 17:
        if (em->stance == 4) {
            Em_EndStatus5(em);
        } else if ((s8) em->holdTimer != 0) {
            Em_SetStatus5Timer(em);
            EmPlesioth_ChangeState(em, 4, 0x1C, 3);
        } else if ((em->stateGroup != 4 || em->state != 0x19) && (em->stateGroup != 4 || em->state != 0x1A) && (em->stateGroup != 4 || em->state != 6)) {
            Em_SetStatus5Timer(em);
            if ((u16) (em->hitAngle - em->rotY) > 0x8000) {
                EmPlesioth_ChangeState(em, 4, 0x19, 3);
            } else {
                EmPlesioth_ChangeState(em, 4, 0x1A, 3);
            }
        }
        break;
    case 18:
        Em_ResetKindTimer(em);
        EmPlesioth_ChangeState(em, 4, 0x18, 3);
        break;
    case 15:
        if (em->stance == 4) {
            EmPlesioth_ChangeState(em, 4, 0x11, 3);
            Em_CheckRageByHp(em, em->rageThreshold);
        }
        break;
    case 5:
        if (em->stateGroup != 4 || em->state != 6) {
            EmPlesioth_ChangeState(em, 4, 6, 3);
        }
        break;
    case 6:
        if ((s8) em->holdTimer != 0) {
            Em_SetStatus4Timer(em);
            EmPlesioth_ChangeState(em, 4, 0xE, 3);
        } else if ((em->stateGroup != 4 || em->state != 0xB) && (em->stateGroup != 4 || em->state != 6) && (em->stateGroup != 4 || em->state != 7) && em->stance == 1) {
            Em_SetStatus4Timer(em);
            EmPlesioth_ChangeState(em, 4, 7, 3);
        } else if ((em->stateGroup != 4 || em->state != 0xB) && (em->stateGroup != 4 || em->state != 6) && (em->stateGroup != 4 || em->state != 7) && em->stance == 0) {
            Em_SetStatus4Timer(em);
            EmPlesioth_ChangeState(em, 4, 0xB, 3);
        } else if ((em->stateGroup != 4 || em->state != 0xF) && (em->stateGroup != 4 || em->state != 6) && (em->stateGroup != 4 || em->state != 7) && em->stance == 4) {
            Em_SetStatus4Timer(em);
            EmPlesioth_ChangeState(em, 4, 0x12, 3);
        }
        break;
    case 7:
        if ((s8) em->holdTimer != 0) {
            Em_SetStatus1Timer(em);
            EmPlesioth_ChangeState(em, 6, 0, 3);
        } else if ((s8) em->holdTimer2 != 0) {
            EmPlesioth_ChangeState(em, 6, 2, 3);
        } else if ((em->stateGroup != 4 || em->state != 0x16) && (em->stateGroup != 4 || em->state != 6) && em->stance == 4) {
            Em_SetStatus1Timer(em);
            EmPlesioth_ChangeState(em, 4, 0x16, 3);
        } else if ((em->stateGroup != 0 || em->state != 0x10) && (em->stateGroup != 4 || em->state != 6) && em->stance == 1) {
            Em_SetStatus1Timer(em);
            EmPlesioth_ChangeState(em, 0, 0x10, 3);
        } else if ((em->stateGroup != 0 || em->state != 0xC) && (em->stateGroup != 4 || em->state != 6) && em->stance == 0) {
            Em_SetStatus1Timer(em);
            EmPlesioth_ChangeState(em, 0, 0xC, 3);
        }
        break;
    case 8:
        if (em->unk8AD == 6) {
            Em_SetStatus2Timer(em);
            EmPlesioth_ChangeState(em, 0, 8, 3);
        } else if ((em->stateGroup != 2 || em->state != 0x13) && (em->stateGroup != 4 || em->state != 6) && em->stance == 4) {
            Em_SetStatus2Timer(em);
            EmPlesioth_ChangeState(em, 2, 0x13, 3);
        } else if ((em->stateGroup != 0 || em->state != 0xE) && (em->stateGroup != 4 || em->state != 6) && em->stance == 1) {
            Em_SetStatus2Timer(em);
            EmPlesioth_ChangeState(em, 0, 0xE, 3);
        } else if ((em->stateGroup != 0 || em->state != 5) && (em->stateGroup != 4 || em->state != 6) && em->stance == 0) {
            Em_SetStatus2Timer(em);
            EmPlesioth_ChangeState(em, 0, 5, 3);
        }
        break;
    case 10:
        if (em->stateGroup == 0) {
            switch (em->state) {
            case 5:
                EmPlesioth_ChangeState(em, 0, 6, 3);
                em->scriptPending = 0;
                break;
            case 8:
                EmPlesioth_ChangeState(em, 4, 0x13, 3);
                em->scriptPending = 0;
                break;
            case 0xA:
                EmPlesioth_ChangeState(em, 4, 0x14, 3);
                em->scriptPending = 0;
                break;
            case 0xC:
                EmPlesioth_ChangeState(em, 0, 0xD, 3);
                em->scriptPending = 0;
                break;
            case 0xE:
                EmPlesioth_ChangeState(em, 0, 0xF, 3);
                em->scriptPending = 0;
                break;
            case 0x12:
                EmPlesioth_ChangeState(em, 0, 0x13, 3);
                em->scriptPending = 0;
                break;
            case 0x29:
                EmCephadrome_ChangeState(em, 0, 0x2A, 3);
                em->scriptPending = 0;
                break;
            }
        } else if (em->stateGroup == 2) {
            switch (em->state) {
            case 0x13:
                EmPlesioth_ChangeState(em, 2, 0x18, 3);
                em->scriptPending = 0;
                break;
            case 0x14:
                EmPlesioth_ChangeState(em, 2, 0x19, 3);
                em->scriptPending = 0;
                break;
            }
        } else if (em->stateGroup == 4) {
            switch (em->state) {
            case 0x16:
                EmPlesioth_ChangeState(em, 2, 0x19, 3);
                em->scriptPending = 0;
                break;
            }
        }
        break;
    case 16:
        if (em->stance == 4) {
            EmPlesioth_ChangeState(em, 4, 0x11, 3);
            Em_CheckRageByHp(em, em->rageThreshold);
            break;
        }
    case 12:
        switch (em->stance) {
        case 0:
        case 3:
            switch (em->flinchPart) {
            case 0:
            case 7:
                EmPlesioth_ChangeState(em, 4, 0, 3);
                break;
            case 5:
            case 6:
                EmPlesioth_ChangeState(em, 4, 2, 3);
                break;
            case 1:
            case 2:
                EmPlesioth_ChangeState(em, 4, 3, 3);
                break;
            case 3:
            case 4:
            default:
                if (em->parts[em->flinchPart].state >= 2) {
                    if (em->flinchPart != 3) {
                        EmPlesioth_ChangeState(em, 4, 5, 3);
                    } else {
                        EmPlesioth_ChangeState(em, 4, 0x17, 3);
                    }
                } else {
                    EmPlesioth_ChangeState(em, 4, 1, 3);
                }
                break;
            }
            break;
        case 1:
            EmPlesioth_ChangeState(em, 4, 4, 3);
            break;
        case 2:
            EmPlesioth_ChangeState(em, 4, 6, 3);
            break;
        case 4:
            EmPlesioth_ChangeState(em, 4, 0x11, 3);
            break;
        }
        break;
    case 13:
        if (em->stance != 2) {
            EmPlesioth_ChangeState(em, 4, 0, 3);
            em->scriptPending = 0;
        }
        break;
    case 14:
        if (em->stateGroup == 2 && em->state == 0x12) {
            em->scriptPending = 0;
        }
        break;
    }
    Em_UpdatePendingScript(em);
    Em_ClearHitReactMode(em);
    EmPlesioth_RunState(em, (EmWork *) w);
    if (em->stateRequest != 0) {
        EmPlesioth_RunState(em, (EmWork *) w);
        em->stateRequest = 0;
    }
    if (em->stance == 4) {
        if (em->posY > em->surfaceY) {
            em->posY = em->surfaceY;
        }
        if (em->posY < em->floorY) {
            em->posY = em->floorY;
        }
    }
}

void EmPlesioth_RunState(Em *em, EmWork *work)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->stateGroup) {
    case 0:
        EmPlesioth_RunGroup0(em, work);
        break;
    case 1:
        EmPlesioth_RunGroup1(em, work);
        break;
    case 2:
        EmPlesioth_RunGroup2(em, work);
        break;
    case 3:
        EmPlesioth_RunGroup3(em, work);
        break;
    case 4:
        EmPlesioth_RunGroup4(em, work);
        break;
    case 5:
        EmPlesioth_RunGroup5(em, work);
        break;
    case 6:
        EmPlesioth_RunGroup6(em, work);
        break;
    case 7:
        EmPlesioth_RunGroup6(em, work);
        
    default:
        break;
    }
}

void EmPlesioth_RequestOnFrame(s32 em, s32 frame, s32 i)
{
    if (Player_PassedFrameScaled((Em *) em, (f32) frame, 0U) != 0) {
        CamShake_ReqAIf((void *) em, i);
    }
}

static void EmPlesioth_MotionEffectsDefault(Em *em)
{
}

void EmPlesioth_MotionEffects(Em *em, EmWork *work)
{
    f32 v4[3];
    f32 v3[3];
    f32 v2[3];
    f32 v1[3];

    if (em->motionId[0] != work->prevMotion) {
        work->prevMotion = em->motionId[0];
    }
    EmPlesioth_MotionSounds(em, work);
    switch (work->prevMotion) {
    case 0x3E9:
        EmPlesioth_SetFinMode(em, work, 3);
        break;
    case 0x3EB:
        EmPlesioth_RequestOnFrame((s32) em, 0x34, 1);
        EmPlesioth_RequestOnFrame((s32) em, 0x74, 1);
        if (Player_PassedFrameScaled(em, 2.0f, 0) != 0) {
            EffPlesioth20_Create(em, 2);
        }
        if (Player_PassedFrameScaled(em, 50.0f, 0) != 0) {
            EffPlesioth20_Create(em, 3);
        }
        if (Player_PassedFrameScaled(em, 120.0f, 0) != 0) {
            EffPlesioth20_Create(em, 4);
        }
        EmPlesioth_SetFinMode(em, work, 1);
        break;
    case 0x3EC:
        EmPlesioth_SetFinMode(em, work, 0);
        if (Player_PassedFrameScaled(em, 22.0f, 0) != 0) {
            EffPlesioth20_Create(em, 0x13);
        }
        if (Player_PassedFrameScaled(em, 16.0f, 0) || Player_PassedFrameScaled(em, 56.0f, 0) || Player_PassedFrameScaled(em, 102.0f, 0)) {
            EffPlesioth20_Create(em, 0x14);
        }
        if ((Player_PassedFrameScaled(em, 30.0f, 0) != 0) || (Player_PassedFrameScaled(em, 78.0f, 0) != 0)) {
            EffPlesioth20_Create(em, 0x15);
        }
        if ((Player_PassedFrameScaled(em, 18.0f, 0) != 0) || (Player_PassedFrameScaled(em, 66.0f, 0) != 0)) {
            Effect0D_SpawnForEmScaled(em, 0x1A, 3, 4.0f);
        }
        if ((Player_PassedFrameScaled(em, 42.0f, 0) != 0) || (Player_PassedFrameScaled(em, 90.0f, 0) != 0)) {
            Effect0D_SpawnForEmScaled(em, 0x14, 3, 4.0f);
            break;
        }
        break;
    case 0x3ED:
        EmPlesioth_SetFinMode(em, work, 1);
        if (Player_PassedFrameScaled(em, 2.0f, 0) != 0) {
            EffPlesioth20_Create(em, 5);
        }
        if (Player_PassedFrameScaled(em, 30.0f, 0) != 0) {
            EffPlesioth20_Create(em, 6);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 2.0f, 78.0f) != 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create(em, 0x1A, 1, 1.0f);
            break;
        }
        break;
    case 0x3EE:
        EmPlesioth_SetFinMode(em, work, 1);
        if (Player_PassedFrameScaled(em, 30.0f, 0) != 0) {
            EffPlesioth20_Create(em, 7);
        }
        if (Player_PassedFrameScaled(em, 2.0f, 0) != 0) {
            EffPlesioth20_Create(em, 8);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 2.0f, 78.0f) != 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create(em, 0x1A, 0, 1.0f);
            break;
        }
        break;
    case 0x3F2:
        EmPlesioth_SetFinMode(em, work, 1);
        if (Player_PassedFrameScaled(em, 42.0f, 0) != 0) {
            EffPlesioth20_Create(em, 0x11);
        }
        if (Player_PassedFrameScaled(em, 2.0f, 0) != 0) {
            EffPlesioth20_Create(em, 0x12);
        }
        if (Player_PassedFrameScaled(em, 74.0f, 0) != 0) {
            EffPlesioth20_Create(em, 0x12);
            break;
        }
        break;
    case 0x3F3:
        EmPlesioth_SetFinMode(em, work, 1);
        break;
    case 0x3FA:
        EmPlesioth_SetFinMode(em, work, 1);
        if (Player_PassedFrameScaled(em, 42.0f, 0) || Player_PassedFrameScaled(em, 102.0f, 0) || Player_PassedFrameScaled(em, 156.0f, 0)) {
            if (em->posY <= (1000.0f + em->floorY)) {
                StageObj_K14_Create(em, 1, 0, 1.0f);
            }
        }
        break;
    case 0x3FB:
        EmPlesioth_SetFinMode(em, work, 1);
        if (Player_PassedFrameScaled(em, 2.0f, 0) != 0) {
            EffPlesioth20_Create(em, 0xD);
        }
        if (Player_PassedFrameScaled(em, 16.0f, 0) != 0) {
            EmPlesioth_SpawnK14Type0B(em);
            break;
        }
        break;
    case 0x401:
        EmPlesioth_SetFinMode(em, work, 3);
        break;
    case 0x407:
        v4[0] = 0.0f;
        v4[1] = 10.0f;
        v4[2] = 140.0f;
        Em_SpawnEffect06Periodic(em, 0x22, v4, 1.6f);
        break;
    case 0x408:
    case 0x409:
        break;
    case 0x40A:
        EmPlesioth_SetFinMode(em, work, 3);
        v3[0] = 0.0f;
        v3[1] = 10.0f;
        v3[2] = 140.0f;
        Em_SpawnEffect06Periodic(em, 0x22, v3, 1.6f);
        break;
    case 0x40C:
        EmPlesioth_SetFinMode(em, work, 3);
        if (Player_PassedFrameScaled(em, 50.0f, 0) != 0) {
            EffPlesioth20_Create(em, 1);
            break;
        }
        break;
    case 0x40D:
        EmPlesioth_SetFinMode(em, work, 3);
        break;
    case 0x40E:
        EmPlesioth_SetFinMode(em, work, 3);
        if (Player_PassedFrameScaled(em, 148.0f, 0) != 0) {
            EffPlesioth20_Create(em, 0xF);
        }
        if (Player_PassedFrameScaled(em, 170.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 2, 6, 4.0f);
        }
        v2[0] = 0.0f;
        v2[1] = 10.0f;
        v2[2] = 140.0f;
        Em_SpawnEffect06Periodic(em, 0x22, v2, 1.6f);
        if (Player_PassedFrameScaled(em, 170.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 2, 6, 4.0f);
            break;
        }
        break;
    case 0x413:
        EmPlesioth_SetFinMode(em, work, 1);
        if (Player_PassedFrameScaled(em, 30.0f, 0) != 0) {
            EffPlesioth20_Create(em, 0x10);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 6.0f, 38.0f) != 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create(em, 0x1A, 1, 1.0f);
            break;
        }
        break;
    case 0x417:
        EmPlesioth_SetFinMode(em, work, 1);
        if (Player_PassedFrameScaled(em, 66.0f, 0) != 0) {
            EffPlesioth20_Create(em, 0x19);
            break;
        }
        break;
    case 0x418:
        break;
    case 0x424:
        EmPlesioth_SetFinMode(em, work, 1);
        if (Player_PassedFrameScaled(em, 44.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x1B, 7, 1.0f);
        }
        if (Player_PassedFrameScaled(em, 62.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x16, 7, 1.0f);
        }
        if (Player_PassedFrameScaled(em, 80.0f, 0) != 0) {
            StageObj_K14_Create(em, 1, 0x80, 0.5f);
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
        EmPlesioth_SetFinMode(em, work, 3);
        break;
    case 0x427:
        EmPlesioth_SetFinMode(em, work, 1);
        break;
    case 0x428:
        EmPlesioth_SetFinMode(em, work, 1);
        break;
    case 0x42A:
        EmPlesioth_SetFinMode(em, work, 1);
        break;
    case 0x42B:
        EmPlesioth_SetFinMode(em, work, 1);
        break;
    case 0x42C:
        EmPlesioth_SetFinMode(em, work, 0);
        if (Player_PassedFrameScaled(em, 50.0f, 0) != 0) {
            StageObj_K14_Create(em, 0, 0, 0.8f);
            break;
        }
        break;
    case 0x42D:
        EmPlesioth_SetFinMode(em, work, 1);
        if (Player_PassedFrameScaled(em, 26.0f, 0) != 0) {
            StageObj_K14_Create(em, 0x12, 0x80, 1.0f);
            break;
        }
        break;
    case 0x42E:
        EmPlesioth_SetFinMode(em, work, 3);
        if ((Player_PassedFrameScaled(em, 16.0f, 0) != 0) || (Player_PassedFrameScaled(em, 66.0f, 0) != 0)) {
            Effect0D_SpawnForEmScaled(em, 2, 6, 4.0f);
            break;
        }
        break;
    case 0x42F:
        EmPlesioth_SetFinMode(em, work, 3);
        break;
    case 0x432:
        EmPlesioth_SetFinMode(em, work, 1);
        if (Player_PassedFrameScaled(em, 26.0f, 0) != 0) {
            StageObj_K14_Create(em, 0x12, 0x81, 1.0f);
            break;
        }
        break;
    case 0x433:
        EmPlesioth_SetFinMode(em, work, 3);
        if ((Player_PassedFrameScaled(em, 16.0f, 0) != 0) || (Player_PassedFrameScaled(em, 66.0f, 0) != 0)) {
            Effect0D_SpawnForEmScaled(em, 2, 6, 4.0f);
            break;
        }
        break;
    case 0x434:
        EmPlesioth_SetFinMode(em, work, 3);
        break;
    case 0x447:
        EmPlesioth_SetFinMode(em, work, 1);
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 4.0f, 60.0f) != 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
            break;
        }
        break;
    case 0x448:
        EmPlesioth_SetFinMode(em, work, 1);
        if (em->rage != 0 && (EmMotion_InFrameRange(em, 0, 12.0f, 24.0f) || EmMotion_InFrameRange(em, 0, 46.0f, 90.0f)) && !(gScene.frame & 3)) {
            StageObj_K14_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
        }
        break;
    case 0x449:
    case 0x44A:
        break;
    case 0x44D:
        EmPlesioth_SetFinMode(em, work, 0);
        break;
    case 0x44E:
        EmPlesioth_SetFinMode(em, work, 0);
        break;
    case 0x44F:
        EmPlesioth_SetFinMode(em, work, 0);
        break;
    case 0x450:
        EmPlesioth_SetFinMode(em, work, 0);
        break;
    case 0x451:
        EmPlesioth_SetFinMode(em, work, 0);
        break;
    case 0x452:
        EmPlesioth_SetFinMode(em, work, 0);
        break;
    case 0x453:
        EmPlesioth_SetFinMode(em, work, 0);
        break;
    case 0x454:
        EmPlesioth_SetFinMode(em, work, 0);
        if (Player_PassedFrameScaled(em, 10.0f, 0) != 0) {
            EffPlesioth20_Create(em, 0xE);
            break;
        }
        break;
    case 0x457:
        EmPlesioth_SetFinMode(em, work, 0);
        break;
    case 0x458:
        EmPlesioth_SetFinMode(em, work, 0);
        break;
    case 0x459:
        EmPlesioth_SetFinMode(em, work, 0);
        break;
    case 0x45A:
        EmPlesioth_SetFinMode(em, work, 0);
        break;
    case 0x45B:
        EmPlesioth_SetFinMode(em, work, 0);
        break;
    case 0x45C:
        EmPlesioth_SetFinMode(em, work, 0);
        break;
    case 0x45D:
        EmPlesioth_SetFinMode(em, work, 0);
        break;
    case 0x45E:
        EmPlesioth_SetFinMode(em, work, 0);
        break;
    case 0x45F:
        EmPlesioth_SetFinMode(em, work, 0);
        if (Player_PassedFrameScaled(em, 50.0f, 0) != 0) {
            EffPlesioth20_Create(em, 0x18);
            break;
        }
        break;
    case 0x460:
        EmPlesioth_SetFinMode(em, work, 0);
        break;
    case 0x461:
        EmPlesioth_SetFinMode(em, work, 0);
        if (Player_PassedFrameScaled(em, 10.0f, 0) != 0) {
            StageObj_K14_Create(em, 0, 0x89, 0.7f);
            break;
        }
        break;
    case 0x462:
        EmPlesioth_SetFinMode(em, work, 0);
        if (Player_PassedFrameScaled(em, 2.0f, 0) != 0) {
            EffPlesioth20_Create(em, 0x17);
        }
        if ((Player_PassedFrameScaled(em, 24.0f, 0) != 0) || (Player_PassedFrameScaled(em, 42.0f, 0) != 0) || (Player_PassedFrameScaled(em, 62.0f, 0) != 0) || (Player_PassedFrameScaled(em, 82.0f, 0) != 0)) {
            Effect0D_SpawnForEmScaled(em, 6, 3, 8.0f + (0.0005f * (f32) (Rand_Next(1) & 0x3FF)));
        }
        if ((Player_PassedFrameScaled(em, 10.0f, 0) != 0) || (Player_PassedFrameScaled(em, 32.0f, 0) != 0) || (Player_PassedFrameScaled(em, 52.0f, 0) != 0) || (Player_PassedFrameScaled(em, 74.0f, 0) != 0)) {
            Effect0D_SpawnForEmScaled(em, 0xC, 3, 8.0f + (0.0005f * (f32) (Rand_Next(1) & 0x3FF)));
            break;
        }
        break;
    case 0x464:
        EmPlesioth_SetFinMode(em, work, 0);
        break;
    case 0x466:
        EmPlesioth_SetFinMode(em, work, 3);
        break;
    case 0x467:
        EmPlesioth_SetFinMode(em, work, 0);
        if (Player_PassedFrameScaled(em, 50.0f, 0) != 0) {
            StageObj_K14_Create(em, 0, 0, 0.8f);
            break;
        }
        break;
    case 0x468:
        EmPlesioth_SetFinMode(em, work, 3);
        if (Player_PassedFrameScaled(em, 42.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 2, 6, 4.0f);
            break;
        }
        break;
    case 0x469:
        EmPlesioth_SetFinMode(em, work, 3);
        if (Player_PassedFrameScaled(em, 42.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 2, 6, 4.0f);
            break;
        }
        break;
    case 0x46A:
        EmPlesioth_SetFinMode(em, work, 0);
        break;
    case 0x46B:
        EmPlesioth_SetFinMode(em, work, 0);
        break;
    case 0x46C:
        EmPlesioth_SetFinMode(em, work, 0);
        break;
    case 0x46D:
        EmPlesioth_SetFinMode(em, work, 0);
        if ((Player_PassedFrameScaled(em, 6.0f, 0) != 0) || (Player_PassedFrameScaled(em, 10.0f, 0) != 0)) {
            Effect0D_SpawnForEmScaled(em, 2, 3, 7.0f);
            break;
        }
        break;
    case 0x46E:
        EmPlesioth_SetFinMode(em, work, 0);
        break;
    case 0x46F:
        EmPlesioth_SetFinMode(em, work, 0);
        break;
    case 0x470:
        EmPlesioth_SetFinMode(em, work, 0);
        if (Player_PassedFrameScaled(em, 54.0f, 0) != 0) {
            EffPlesioth20_Create(em, 0x16);
            break;
        }
        break;
    case 0x471:
        v1[0] = 0.0f;
        v1[1] = 10.0f;
        v1[2] = 140.0f;
        Em_SpawnEffect06Periodic(em, 0x22, v1, 1.6f);
        break;
    case 0x472:
        break;
    default:
        EmPlesioth_MotionEffectsDefault(em);
        break;
    }
}

static void EmPlesioth_UpdateFinFold(Em *em, EmWork *work, s32 i)
{
    EmPlesiothWork *p = (EmPlesiothWork *) work;
    u8 *st;

    /* the cast (a no-op) gives the game's operand order (&p->finState[i][1] swaps an addu) */
    switch (*(st = &((u8 (*)[2]) p->finState)[i][1])) {
    case 0:
        if (p->finMode == 2 || p->finMode == 3) {
            if (p->finRot[i].fold != EmPlesioth_FinFoldTarget[i]) {
                *st = 2;
                p->finCount[i] = 20;
            }
        } else if (p->finMode == 1) {
            if (p->finRot[i].fold != 0) {
                *st = 1;
                p->finCount[i] = 20;
            }
        }
        break;
    case 1:
        if (--p->finCount[i] <= 0) {
            p->finRot[i].fold = 0;
            *st = 0;
        } else {
            p->finRot[i].fold += (u16) ((0x10000 - p->finRot[i].fold) / p->finCount[i]);
        }
        break;
    case 2:
        if (--p->finCount[i] <= 0) {
            p->finRot[i].fold = EmPlesioth_FinFoldTarget[i];
            *st = 0;
        } else {
            p->finRot[i].fold -= (u16) ((u16) (0x10000 - (EmPlesioth_FinFoldTarget[i] - p->finRot[i].fold)) / p->finCount[i]);
        }
        break;
    }
}

typedef struct EmFinKey {
    u16 f;
    u16 v;
} EmFinKey;

extern EmFinKey *EmPlesioth_FinKeys2[];
extern EmFinKey *EmPlesioth_FinKeys1[];
extern s16 EmPlesioth_FinWait1[];
extern s16 EmPlesioth_FinWait2[];
extern s16 EmPlesioth_FinRewait1[];
extern s16 EmPlesioth_FinRewait2[];

static inline u8 FinFindKey(EmFinKey *key, u16 f)
{
    s32 n = 0;

    for (;;) {
        if (!(n != 0) == 0 && key->f == 0) {
            n = (u8) (n | 0x80);
            break;
        }
        if (key->f < f) {
            n = (u8) (n + 1);
            key++;
        } else {
            break;
        }
    }
    return n;
}

#define FIN_S16(base, j) (*(s16 *) ((u8 *) (base) + (j)))

/* j: the first argument (the caller passes em) reused as fin i's byte
   offset into the s16 arrays; only this gives the game's registers. */
static void EmPlesioth_UpdateFinWave(s32 j, EmWork *work, s32 i)
{
    EmPlesiothWork *p = (EmPlesiothWork *) work;
    s16 f;
    EmFinKey *tbl;
    u8 m;
    u8 *st;

    j = i * 2;
    switch (*(st = (u8 *) p->finState + j)) {
    case 0:
        if (p->finMode == 2) {
            (*st)++;
            FIN_S16(p->finWait, j) = FIN_S16(EmPlesioth_FinWait2, j);
        } else if (p->finMode == 1) {
            (*st)++;
            FIN_S16(p->finWait, j) = FIN_S16(EmPlesioth_FinWait1, j);
        }
        break;
    case 1:
        if (--FIN_S16(p->finWait, j) <= 0) {
            if (p->finMode == 2) {
                *st = 3;
            } else {
                *st = 2;
            }
            FIN_S16(p->finWait, j) = 0;
            FIN_S16(p->finFrame, j) = 0;
        }
        break;
    case 2:
        f = FIN_S16(p->finFrame, j)++;
        tbl = EmPlesioth_FinKeys2[i];
        m = FinFindKey(tbl, f);
        if (m & 0x80) {
            if (p->finMode == 2) {
                *st = 1;
                FIN_S16(p->finWait, j) = FIN_S16(EmPlesioth_FinRewait2, j);
            } else if (p->finMode == 1) {
                *st = 1;
                FIN_S16(p->finWait, j) = FIN_S16(EmPlesioth_FinRewait1, j);
            } else {
                *st = 0;
            }
            p->finRot[i].wave = tbl[m & 0x7F].v;
        } else {
            p->finRot[i].wave += tbl[m].v;
        }
        break;
    case 3:
        f = FIN_S16(p->finFrame, j)++;
        tbl = EmPlesioth_FinKeys1[i];
        m = FinFindKey(tbl, f);
        if (m & 0x80) {
            if (p->finMode == 2) {
                *st = 1;
                FIN_S16(p->finWait, j) = FIN_S16(EmPlesioth_FinRewait2, j);
            } else if (p->finMode == 1) {
                *st = 1;
                FIN_S16(p->finWait, j) = FIN_S16(EmPlesioth_FinRewait1, j);
            } else {
                *st = 0;
            }
            p->finRot[i].wave = tbl[m & 0x7F].v;
        } else {
            p->finRot[i].wave += tbl[m].v;
        }
        break;
    }
}

void EmPlesioth_UpdateFins(Em *em, EmWork *work)
{
    s32 i;

    for (i = 0; i < 4; i++) {
        EmPlesioth_UpdateFinWave((s32) em, work, i);
        EmPlesioth_UpdateFinFold(em, work, i);
    }
}

void EmPlesioth_UpdateMotionEffects(Em *em)
{
    EmPlesiothWork *work = (EmPlesiothWork *)&em->work;

    switch (work->fxStep) {
    case 0:
        work->fxStep++;
        break;
    case 1:
        EmPlesioth_MotionEffects(em, (EmWork *) work);
        EmPlesioth_UpdateFins(em, (EmWork *) work);
        break;
    }
}

void EmPlesioth_SetFinMode(Em *em, EmWork *work, s32 mode)
{
    EmPlesiothWork *w = (EmPlesiothWork *)work;

    w->finMode = mode;
    if (w->finMode == 1) {
        if (em->hp <= (s16) (0.2f * em->hpMax)) {
            w->finMode = 2;
        }
    }
    if (w->finMode == 3) {
        if (em->hp > (s16) (0.2f * em->hpMax)) {
            w->finMode = 0;
        }
    }
}

void EmPlesioth_SpawnK14Type0B(Em *em)
{
    StageObj_K14_Create((Em *) em, 0xB, 0, 1.0f);
}

void EmPlesioth_SpawnSurfaceEffect2(Em *em, f32 scale)
{
    f32 pos[3];
    f32 surfaceY;

    if ((gScene.area == em->area) && !(gScene.frame & 3)) {
        surfaceY = em->surfaceY;
        if (!(em->posY < (surfaceY - 100.0f))) {
            Vec_Set(pos, em->posX, 10.0f + (surfaceY + *(f32 *)&em->surfaceDepth), em->posZ);
            Effect08_Spawn(pos, 2, 0, scale);
            Em_PlaySeAtBone(em, 0x1B, 6);
        }
    }
}

void EmPlesioth_SpawnSurfaceEffect5(Em *em, f32 scale)
{
    f32 pos[3];

    if (gScene.area == em->area) {
        Vec_Set(pos, em->posX, 10.0f + (em->surfaceY + *(f32 *)&em->surfaceDepth), em->posZ);
        Effect08_Spawn(pos, 5, 0, scale);
        Em_PlaySeAtBone(em, 0x1E, 6);
    }
}

void EmPlesioth_SpawnSurfaceEffect4(Em *em, f32 scale)
{
    f32 pos[3];
    f32 surfaceY;

    if ((gScene.area == em->area) && !(gScene.frame & 7)) {
        surfaceY = em->surfaceY;
        if (!(em->posY < (surfaceY - 100.0f))) {
            Vec_Set(pos, em->posX, 10.0f + (surfaceY + *(f32 *)&em->surfaceDepth), em->posZ);
            Effect08_Spawn(pos, 4, 0, scale);
        }
    }
}

void EmPlesioth_TableNop(void)
{
}

void EmPlesioth_SetGroup0State(Em *em, u16 state, u8 mode)
{
    em->motionSpeed = 1.0f;
    switch (state) {
    case 5:
    case 8:
    case 14:
        Em_BeginStatus2(em);
        break;
    case 10:
    case 12:
    case 16:
        Em_BeginStatus1(em);
        break;
    case 6:
    case 15:
        Em_EndStatus2(em);
        break;
    case 13:
    case 17:
        Em_EndStatus1(em);
        break;
    case 19:
        Em_EndStatus1(em);
        break;
    case 20:
        Em_EndStatus4(em);
        break;
    case 21:
        em->holdTimer2 = 0;
        break;
    case 22:
        Em_EndStatus5(em);
        break;
    case 41:
        Em_BeginStatus1(em);
        break;
    case 42:
        Em_EndStatus1(em);
        break;
    }
    em->stance = 0;
    Em_SetStateSync(em, 0, state, mode);
}

void EmPlesioth_SetGroup1State(Em *em, u16 state, u8 mode)
{
    f32 f;

    em->stance = 0;
    em->motionSpeed = 1.0f;
    switch (state) {
    case 0:
        if (em->hp < (s16) (0.2f * em->hpMax)) {
            state = 3;
            goto state3;
        }
        Em_SetSpeedScaled(em, 0, 1000.0f, 500.0f);
        if (em->hasGoal == 0) {
            em->timer = (em->stateRand % 120 + 180) / 2;
        }
        break;
    case 1:
        if (em->curGoalKind == 0) {
            em->hasGoal = 0;
            em->goalDist = 1000.0f;
        } else {
            em->hasGoal = 1;
            f = Vec_DistanceXZ(&em->posX, em->goal) - 300.0f;
            em->goalDist = f;
            if (f < 0.0f) {
                em->goalDist = 0.0f;
            }
        }
        break;
    case 2:
        break;
    case 3:
    state3:
        Em_SetSpeedScaled(em, 6, em->goalDist, 0.0f);
        if (em->hasGoal == 0) {
            em->timer = (em->stateRand % 120 + 180) / 2;
            if (em->timer > 300) {
                em->timer = 300;
            }
        }
        break;
    }
    Em_SetStateSync(em, 1, state, mode);
}

void EmPlesioth_SetGroup2State(Em *em, u16 state, u8 mode)
{
    EmPlesiothWork *w = (EmPlesiothWork *)&em->work;
    f32 pos[3];
    s32 a;

    em->motionSpeed = 1.0f;
    switch (state) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 7:
    case 8:
    case 9:
    case 12:
    case 13:
    case 14:
    case 15:
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
    case 22:
    case 24:
    case 25:
        switch (state) {
        case 19:
            Em_BeginStatus2(em);
            break;
        case 20:
            Em_BeginStatus1(em);
            break;
        case 21:
            Em_EndStatus4(em);
            break;
        case 24:
            Em_EndStatus2(em);
            break;
        case 25:
            Em_EndStatus1(em);
            break;
        }
        if (em->remote == 0 && EmPlesioth_OverDeepWater(em, 0) == 0) {
            EmPlesioth_ChangeState(em, 0, 1, 1);
            return;
        }
        break;
    case 4:
    case 5:
        if (em->remote == 0 && EmPlesioth_OverDeepWater(em, 0) == 0) {
            EmPlesioth_ChangeState(em, 0, 1, 1);
            return;
        }
        Em_SetSpeedScaled(em, 0, 1000.0f, 500.0f);
        break;
    case 6:
        if (em->remote == 0 && EmPlesioth_OverDeepWater(em, 0) == 0) {
            EmPlesioth_ChangeState(em, 0, 1, 1);
            return;
        }
        state = 8;
        break;
    case 10:
    case 11:
        if (em->remote == 0 && EmPlesioth_OverDeepWater(em, 0) == 0) {
            EmPlesioth_ChangeState(em, 0, 1, 1);
            return;
        }
        w->unk007 = 4;
        if (em->remote == 0) {
            em->goalNo = EmPlesioth_NextAreaPoint(em, pos);
            em->goalKind = 2;
            em->goalArg = 2;
            Em_ComputeGoal(em, em->goal);
            a = (u16) (Math_AngleXZ(&em->posX, em->goal) - em->rotY);
            if ((0 <= a && a <= 0x2000) || (a >= 0xE000 && a <= 0xFFFF)) {
                a = (u16) (Math_AngleXZ(&em->posX, pos) - em->rotY);
                if (a >= 0x4000 && a <= 0xC000 && state == 10) {
                    state = 11;
                }
                em->timer = 1.5f * (Vec_DistanceXZ(&em->posX, em->goal) / 0.5f) / 50.0f;
            } else {
                state = 9;
            }
        } else {
            Em_ComputeGoal2(em, em->goal);
        }
        break;
    case 16:
    case 23:
    case 26:
    case 27:
        break;
    }
    Em_SetStateSync(em, 2, state, mode);
}

void EmPlesioth_SetGroup3State(Em *em, u16 state, u8 mode)
{
    EmPlesiothWork *w = (EmPlesiothWork *)&em->work;

    em->motionSpeed = 1.0f;
    switch (state) {
    case 3:
    case 4:
    case 5:
    case 6:
        if (em->remote == 0 && EmPlesioth_OverDeepWater(em, 0) == 0) {
            EmPlesioth_ChangeState(em, 0, 1, 1);
            return;
        }
        break;
    case 1:
        w->unk013 = 1;
        break;
    case 0:
    case 2:
    case 7:
        break;
    }
    Em_SetStateSync(em, 3, state, mode);
}

void EmPlesioth_ChangeState(Em *em, u16 group, u16 state, u8 mode)
{
    if (em->remote == 0) {
        Em_TargetPlayerInRange(em);
    } else if (mode == 3 || mode == 4) {
        return;
    }
    Em_LoadReactionSet(em, 0);
    switch (group) {
    case 0:
        EmPlesioth_SetGroup0State(em, state, mode);
        break;
    case 1:
        EmPlesioth_SetGroup1State(em, state, mode);
        break;
    case 2:
        EmPlesioth_SetGroup2State(em, state, mode);
        break;
    case 3:
        EmPlesioth_SetGroup3State(em, state, mode);
        break;
    case 4:
        switch (state) {
        case 7:
        case 11:
        case 14:
        case 18:
            Em_BeginStatus4(em);
            break;
        case 8:
        case 21:
            Em_EndStatus4(em);
            break;
        case 12:
        case 24:
            Em_EndAllStatus(em);
            break;
        case 19:
            Em_EndStatus2(em);
            break;
        case 20:
            Em_EndStatus1(em);
            break;
        case 22:
            Em_BeginStatus1(em);
            break;
        case 25:
        case 26:
        case 28:
            Em_BeginStatus5(em);
            break;
        case 27:
            Em_EndStatus5(em);
            break;
        case 15:
            em->goalKind = 2;
            em->goalArg = 1;
            em->goalNo = 1;
            Em_ComputeGoal(em, em->goal);
            Em_BreakPart(em, 2);
            break;
        }
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        break;
    case 5:
        if (state == 4 && QuestEm_CanAppear(em) == 1) {
            QuestEm_Appear(em);
        }
        Em_SetStateSync(em, group, state, mode);
        break;
    case 6:
        switch (state) {
        case 0:
            Em_BeginStatus1(em);
            break;
        }
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        break;
    }
}

void EmPlesioth_StartCurves(Em *em, u8 kind)
{
    EmPlesiothWork *w = (EmPlesiothWork *)&em->work;

    w->curveNone = 0;
    w->curveVelY = 0;
    w->curveSpeed = 0;
    w->curveKind = kind;
    switch (w->curveKind) {
    case 0:
        w->curveVelY = 1;
        w->curveSpeed = 1;
        w->curveFrame = 0;
        break;
    case 1:
        w->curveVelY = 1;
        w->curveSpeed = 1;
        w->curveFrame = 0;
        break;
    case 2:
        w->curveVelY = 1;
        w->curveSpeed = 1;
        w->curveFrame = 0;
        break;
    case 3:
        w->curveVelY = 1;
        w->curveFrame = 0x26;
        break;
    }
    Player_ClearVelocity(em);
}

static s32 EmPlesioth_CurveNone(Em *em, EmWork *work)
{
    return 0;
}

static u8 EmPlesioth_CurveVelY(Em *em, EmWork *work)
{
    EmCurveKey *p;
    f32 frame;
    f32 t;
    s32 i;
    EmCurveKey *tbl;
    u8 ret;

    tbl = EmPlesioth_VelYCurves[((EmPlesiothWork *)work)->curveKind];
    ret = 0;
    if (tbl == NULL) {
        return ret;
    }
    if (em->motion[0].blend != 0) {
        frame = 0.0f;
    } else {
        frame = ((EmPlesiothWork *)work)->curveFrame;
    }
    if (frame != 0.0f && frame != 1.0f) {
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

static u8 EmPlesioth_CurveSpeed(Em *em, EmWork *work)
{
    EmCurveKey *p;
    f32 frame;
    f32 t;
    s32 i;
    EmCurveKey *tbl;
    u8 ret;

    tbl = EmPlesioth_SpeedCurves[((EmPlesiothWork *)work)->curveKind];
    ret = 0;
    if (tbl == NULL) {
        return ret;
    }
    if (em->motion[0].blend != 0) {
        frame = 0.0f;
    } else {
        frame = ((EmPlesiothWork *)work)->curveFrame;
    }
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

u8 EmPlesioth_UpdateCurves(Em *em)
{
    EmPlesiothWork *w = (EmPlesiothWork *)&em->work;
    u8 ret = 0;

    if (w->curveNone != 0) {
        ret = EmPlesioth_CurveNone(em, (EmWork *) w);
    }
    if (w->curveVelY != 0) {
        ret |= EmPlesioth_CurveVelY(em, (EmWork *) w);
    }
    if (w->curveSpeed != 0) {
        ret |= EmPlesioth_CurveSpeed(em, (EmWork *) w);
    }
    w->curveFrame += (s16) em->motion[0].step;
    if (em->stance == 2 || em->stance == 4) {
        w->moveAngBX = 0;
        w->moveAngBY = em->rotY;
        w->moveAngBZ = 0;
        Em_MoveRotatedAccel(em, &w->moveAngBX);
    }
    return ret;
}

u8 EmPlesioth_NextAreaPoint(Em *em, f32 *out)
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

s32 EmPlesioth_OverDeepWater(Em *em, u8 arg1)
{
    if (em->floorUnderSurface != 0) {
        return 1;
    }
    if (!(arg1 & 0xFF) && !(Player_InCurrentArea(em) & 0xFF)) {
        return 1;
    }
    return 0;
}

/* The trader's (kind 0x0A) own meanings of the work area's bytes (EmWork has the shared ones). */
typedef struct EmKind0AWork {
    u8 fxStep;                  /* 0x000: UpdateMotionEffects: 0, then 1 (runs MotionEffects) */
    u8 unk001[0x008 - 0x001];
    f32 home[3];                /* 0x008: Init: the position; State0_01 puts the trader back there */
    u16 homeYaw;                /* 0x014: Init: rotY; State0_01 restores rotY and targetYaw from it */
    u8 unk016[0x018 - 0x016];
    u8 unk018;                  /* 0x018: Init: 0; State0_03 (turning to the player): 1 */
    u8 unk019[0x01A - 0x019];
    u8 noAnswer;                /* 0x01A: 1: Trade takes no answer (State0_05 and State0_09 set it before calling Trade while their motion plays); Trade and the states clear it */
    u8 choice;                  /* 0x01B: the talk's cursor (Notice_OpenTalk's last argument): 0 on the first answer, 1 on the second (ShowMessage's start); Trade moves it */
    s16 msgWait;                /* 0x01C: ShowMessage: 30 frames for the kinds that wait (1, 4, 5, 0xA), else 0; Trade counts it down and takes no answer before 0 */
    u8 msgBuf;                  /* 0x01E: which of the two 256-byte text buffers of gEmKind0AMsgBufs holds the message (SetMessage flips it) */
    u8 unk01F[0x020 - 0x01F];
    EmTraderMsg msg;            /* 0x020: the message shown: ShowMessage copies its gEmTraderMsgs row here, then points text at the buffer SetMessage wrote */
    u8 unk028[0x100 - 0x028];
} EmKind0AWork;
typedef char EmKind0AWork_size_check[(sizeof(EmKind0AWork) == sizeof(EmWork)) ? 1 : -1];

void EmKind0A_MotionSounds(void *em, EmWork *work)
{
    s16 motion;

    motion = work->fxMotion;
    switch (motion) {
    case 0x3EA:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2A, 0x1A, 0);
        break;
    case 0x3EB:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 0xA, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1A, 0x1B, 0);
        Em_PlaySe1AtBoneOnFrame((Em *) em, 6, 0x43, 0);
        break;
    case 0x3EC:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 4, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x16, 0);
        Em_PlaySe1AtBoneOnFrame((Em *) em, 6, 0x3A, 0);
        break;
    case 0x3ED:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xB0, 7, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1C, 0x1A, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xD4, 0x1A, 0);
        break;
    case 0x3EE:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 8, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xB2, 9, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x5C, 0x1C, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xCE, 0x1C, 0);
        break;
    case 0x3EF:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA, 6, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1A, 0x1A, 0);
        break;
    case 0x3F0:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2A, 0xB, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x26, Sound_RandomSe(0x22, 6, 0x23, 0), 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x80, Sound_RandomSe(0x23, 6, 0x24, 0), 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xB6, Sound_RandomSe(0x24, 6, 9, 2), 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xEE, 0xD, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x15E, 7, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x44, 0x1A, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xB2, 0x1A, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x11E, 0x1C, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x4E, 0x19, 0);
        break;
    case 0x3F1:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1E, 5, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 0x16, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA4, 0x17, 0);
        Em_PlaySe1AtBoneOnFrame((Em *) em, 0x18, 0x3D, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1E, 0x1B, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x7A, 0x1C, 0);
        break;
    case 0x3F2:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x12, 3, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x32, 0xA, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x62, 0xD, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x18, 0x17, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x74, 0x17, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA4, 0x16, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x24, 0x1B, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x7E, 0x1C, 0);
        break;
    case 0x3F3:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 0, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3C, 1, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x72, 2, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xB0, 3, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x14, 0x17, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1A, 0x1B, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x4C, 0x1B, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA0, 0x1B, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x20, 0x16, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xEA, 0x16, 0);
        Em_PlaySe1AtBoneOnFrame((Em *) em, 0x3C, 0x3D, 0);
        Em_PlaySe1AtBoneOnFrame((Em *) em, 0x72, 0x4A, 0);
        Em_PlaySe1AtBoneOnFrame((Em *) em, 0x88, 0x4A, 0);
        Em_PlaySe1AtBoneOnFrame((Em *) em, 0xB0, 0x4A, 0);
        break;
    case 0x3F4:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0xC, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x7E, 4, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1E, 0x1C, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA0, 0x1B, 0);
        Em_PlaySe1AtBoneOnFrame((Em *) em, 0x50, 0x4A, 0);
        break;
    case 0x3F5:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x26, Sound_RandomSe(0xE, 4, 0xF, 2), 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x6C, Sound_RandomSe(0x11, 2, 0xE, 4), 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xC, 0x14, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2C, 0x15, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x4C, 0x14, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x6A, 0x15, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x14, 0x1A, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x50, 0x1A, 0);
        break;
    case 0x3F6:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x18, Sound_RandomSe(0xE, 4, 0x11, 4), 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xB4, Sound_RandomSe(0x10, 3, 0x11, 3), 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x12C, Sound_RandomSe(0xE, 3, 0xF, 3), 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x174, Sound_RandomSe(0x11, 3, 0x10, 3), 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1DC, Sound_RandomSe(0xE, 3, 0xF, 3), 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x258, Sound_RandomSe(0xE, 3, 0x10, 3), 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x22, 0x1C, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xFE, 0x1A, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1BE, 0x1A, 0);
        Em_PlaySe1AtBoneOnFrame((Em *) em, 0x7E, 0x4C, 0);
        Em_PlaySe1AtBoneOnFrame((Em *) em, 0xB0, 0x4C, 0);
        Em_PlaySe1AtBoneOnFrame((Em *) em, 0xDC, 0x4C, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1A4, 0x19, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1B8, 0x19, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x80, 0x14, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x122, 0x15, 0);
        
    default:
        break;
    }
}

s32 EmKind0A_AdjustItem(Em *em, s32 item)
{
    s32 id;
    u8 variant;

    variant = em->variant;
    if (variant == 0) {
        return item;
    }
    id = item & 0xFF;
    if ((id >= 0x22) && (id < 0x59)) {
        return item;
    }
    if ((id >= 0x59) && (id < 0x64)) {
        return item;
    }
    return (id + variant) & 0xFF;
}

void EmKind0A_SetMessage(Em *em, EmKind0AWork *w)
{
    char *src;
    char *dst;

    w->msgBuf ^= 1;
    src = w->msg.text;
    dst = gEmKind0AMsgBufs + (w->msgBuf << 8);
    while (*src != 0) {
        if (src[0] == 0x7C && src[1] == 0x7C) {
            src += 2;
            dst += (s8) sprintf(dst, "~C05%s~C00", gItemNames[em->msgItem]);
        } else {
            *dst++ = *src++;
        }
    }
    *dst = 0;
}

s32 EmKind0A_IsPlayerNear(Em *em)
{
    Player *p = &gPlayers[gScene.localPlayer];

    if (!p->active || !p->visible) {
        return 0;
    }
    if (em->area != p->area) {
        return 0;
    }
    if (!(u8) Obj_InCurrentArea(p)) {
        return 0;
    }
    if (Vec_Distance((f32 *) p->pos, (f32 *) &em->posX) <= 500.0f) {
        return 1;
    }
    return 0;
}

void EmKind0A_ShowMessage(Em *em, u16 id)
{
    EmKind0AWork *w = (EmKind0AWork *)&em->work;
    EmTraderMsg *m = &w->msg;
    Player *p = &gPlayers[gScene.localPlayer];
    EmTraderMsg *r;

    p->tradeMsgTimer = 10;
    em->tradeStep = 0;
    em->msgId = id;
    em->msgTimer = 0;
    w->choice = 1;
    r = &gEmTraderMsgs[em->msgId];
    m->kind = r->kind;
    m->next = r->next;
    m->nextNo = r->nextNo;
    m->mode = r->mode;
    m->text = r->text;
    EmKind0A_SetMessage((Em *) p, w);
    m->text = gEmKind0AMsgBufs + (w->msgBuf << 8);
    switch (m->kind) {
    case 1:
    case 4:
    case 5:
    case 0xA:
        w->msgWait = 30;
        break;
    default:
        w->msgWait = 0;
        break;
    }
}

void EmKind0A_GiveRandomItem(Em *em)
{
    s16 r;
    u16 *it;
    EmKind0AWork *w;
    Player *p;
    EmTraderMsg *e;
    EmTraderMsg *m;
    s16 i;
    s16 n;

    w = (EmKind0AWork *)&em->work;
    p = &gPlayers[gScene.localPlayer];
    r = Rand_Next(1) % 100;
    if (r <= 39) {
        n = Rand_Next(1) % 101;
        i = 0;
        if (em->variant == 0) {
            it = EmKind0A_GiftItemsV0;
        } else {
            it = EmKind0A_GiftItemsV1;
        }
        for (; *it != 0xFFFF; it += 2) {
            i += it[0];
            if (i >= n) {
                if (Pouch_Count(p, it[1]) > 0 || Pouch_HasFreeSlot(p) != 1) {
                    goto other;
                }
                p->tradeGetItem = it[1];
                break;
            }
        }
        em->msgId = (u8) EmKind0A_AdjustItem(em, 0x14);
        Em_SetStateRand(em, 0, 9);
    } else {
    other:
        if (r <= 69) {
            em->msgId = 0x24;
        } else {
            em->msgId = 0x22;
        }
    }
    m = &w->msg;
    p->tradeMsgTimer = 10;
    em->tradeStep = 0;
    em->msgTimer = 0;
    w->choice = 1;
    e = &gEmTraderMsgs[em->msgId];
    m->kind = e->kind;
    m->next = e->next;
    m->nextNo = e->nextNo;
    m->mode = e->mode;
    m->text = e->text;
    EmKind0A_SetMessage((Em *) p, w);
    m->text = gEmKind0AMsgBufs + (w->msgBuf << 8);
    switch (e->kind) {
    case 1:
    case 4:
    case 5:
    case 0xA:
        w->msgWait = 30;
        break;
    default:
        w->msgWait = 0;
        break;
    }
}

extern s8 D_0043C73A;
s32 EmKind0A_Trade(Em *em)
{
    u16 *it;
    Player *p;
    EmKind0AWork *w;
    EmTraderMsg *m;
    u8 sel;
    u8 k;
    u8 ret;
    u16 item;
    s16 i;
    ItemStack *ps;
    EmTradeRate *tbl;
    s16 r;
    s16 sum;
    u16 v;

    w = (EmKind0AWork *)&em->work;
    p = &gPlayers[gScene.localPlayer];
    p->tradeMsgTimer = 5;
    m = &w->msg;
    if (*(u16 *) &em->msgTimer <= 999) {
        (*(u16 *) &em->msgTimer)++;
    } else {
        *(u16 *) &em->msgTimer = 1000;
    }
    switch ((u16) em->tradeStep) {
    case 0:
    if (Notice_OpenTalk(m->text, *(u16 *) &em->msgTimer, m->mode, w->choice) == 0) {
        if (Quest_PhaseIsOver(1) == 1) {
            goto end;
        }
        if (w->msgWait != 0) {
            w->msgWait--;
        }
        sel = w->choice;
        k = 0;
        if (sel == 0 && (p->input.pressed & 0x20)) {
            k = 1;
        } else if (sel == 1 && (p->input.pressed & 0x60)) {
            k = 2;
        }
        if (k != 0 && w->noAnswer == 0 && w->msgWait == 0) {
            switch ((u16) Area_BaseId(gScene.map)) {
            case 2:
            default:
                if (em->variant == 0) {
                    it = EmKind0A_TradeArea02V0;
                } else {
                    it = EmKind0A_TradeArea02V1;
                }
                break;
            case 4:
                if (em->variant == 0) {
                    it = EmKind0A_TradeArea04V0;
                } else {
                    it = EmKind0A_TradeArea04V1;
                }
                break;
            case 6:
                if (em->variant == 0) {
                    it = EmKind0A_TradeArea06V0;
                } else {
                    it = EmKind0A_TradeArea06V1;
                }
                break;
            case 3:
                if (em->variant == 0) {
                    it = EmKind0A_TradeArea03V0;
                } else {
                    it = EmKind0A_TradeArea03V1;
                }
                break;
            case 11:
                if (em->variant == 0) {
                    it = EmKind0A_TradeArea11V0;
                } else {
                    it = EmKind0A_TradeArea11V1;
                }
                break;
            }
            switch (m->kind) {
            case 0:
                p->tradeMsgTimer = 0;
                break;
            case 3:
                ret = 0;
                if ((u16) Pouch_FindCarryItem(p) != 0xFFFF) {
                    ret = 4;
                }
                if (ret == 0 && Pouch_Count(p, 0x2E0) >= 99 && Pouch_Space(p, 0x9D) > 0) {
                    p->tradeGiveItem = 0x2E0;
                    p->tradeGiveCount = -99;
                    p->tradeGetItem = 0x9D;
                    ret = 2;
                }
                if (ret == 0 && Pouch_Count(p, 0x226) > 0 && Pouch_Space(p, 0x2D8) > 0) {
                    p->tradeGiveItem = 0x226;
                    p->tradeGiveCount = -1;
                    p->tradeGetItem = 0x2D8;
                    ret = 2;
                }
                if (ret == 0 && Pouch_Count(p, 0x2A4) > 0 && Pouch_Space(p, 0x2A5) > 0) {
                    p->tradeGiveItem = 0x2A4;
                    p->tradeGiveCount = -1;
                    p->tradeGetItem = 0x2A5;
                    ret = 2;
                }
                if (ret == 0 && Pouch_Count(p, 0x2F5) > 0 && Pouch_Space(p, 0x62) > 0) {
                    p->tradeGiveItem = 0x2F5;
                    p->tradeGiveCount = -1;
                    p->tradeGetItem = 0x62;
                    ret = 2;
                }
                if (ret == 0 && Pouch_Count(p, 0x2F2) >= 99 && Pouch_Space(p, 0xA0) > 0) {
                    p->tradeGiveItem = 0x2F2;
                    p->tradeGiveCount = -99;
                    p->tradeGetItem = 0xA0;
                    ret = 2;
                }
                if (ret == 0 && Pouch_Count(p, 0x2B8) >= 5 && Game_IsOnline() == 1) {
                    p->tradeGiveItem = 0x2B8;
                    p->tradeGiveCount = -1;
                    p->tradeGetItem = 0x5C;
                    ret = 8;
                }
                if (ret == 0 && gQuest.traderWantItem != 0) {
                    if (Pouch_Count(p, gQuest.traderWantItem) > 0) {
                        p->tradeGiveItem = gQuest.traderWantItem;
                        for (; *it != 0xFFFF; it += 3) {
                            if (*it == gQuest.traderWantItem) {
                                p->tradeGiveItem = *it;
                                p->tradeGiveCount = -1;
                                p->tradeGetItem = it[1];
                                if (Pouch_Space(p, p->tradeGetItem) > 0) {
                                    ret = 7;
                                } else {
                                    ret = 9;
                                    if (p->tradeCount > 0) {
                                        p->tradeCount--;
                                        p->tradeCooldown = 0;
                                    }
                                }
                                goto chosen;
                            }
                        }
                        p->tradeGiveItem = gQuest.traderWantItem;
                        ret = 6;
                        if (p->tradeCount > 0) {
                            p->tradeCount--;
                            p->tradeCooldown = 0;
                        }
                    } else {
                        p->tradeGiveItem = gQuest.traderWantItem;
                        ret = 6;
                        if (p->tradeCount > 0) {
                            p->tradeCount--;
                            p->tradeCooldown = 0;
                        }
                    }
                }
            chosen:
                if (ret == 0) {
                    if (Rand_Next(1) % 100 >= 10 && gQuest.traderWantItem == 0) {
                        for (; *it != 0xFFFF; it += 3) {
                            if (Pouch_Count(p, *it) > 0) {
                                p->tradeGiveItem = *it;
                                p->tradeGiveCount = -1;
                                if (Rand_Next(1) % 100 >= 20) {
                                    p->tradeGetItem = it[1];
                                } else {
                                    p->tradeGetItem = it[2];
                                }
                                if (Pouch_Space(p, p->tradeGetItem) > 0) {
                                    ret = 2;
                                    break;
                                }
                            }
                        }
                    } else {
                        it += (Rand_Next(1) % 8 + 7) * 3;
                        gQuest.traderWantItem = *it;
                        p->tradeGiveItem = *it;
                        p->tradeGiveCount = -1;
                        if (Rand_Next(1) % 100 >= 20) {
                            p->tradeGetItem = it[1];
                        } else {
                            p->tradeGetItem = it[2];
                        }
                        ret = 5;
                        if (p->tradeCount > 2) {
                            p->tradeCount = 2;
                            p->tradeCooldown = 0;
                        }
                    }
                }
                switch (ret) {
                case 0:
                    EmKind0A_GiveRandomItem(em);
                    break;
                case 1:
                case 2:
                case 3:
                    EmKind0A_ShowMessage(em, (u8) EmKind0A_AdjustItem(em, 0xE));
                    break;
                case 4:
                    EmKind0A_ShowMessage(em, (u8) EmKind0A_AdjustItem(em, 0x18));
                    break;
                case 5:
                    EmKind0A_ShowMessage(em, (u8) EmKind0A_AdjustItem(em, 2));
                    break;
                case 6:
                    EmKind0A_ShowMessage(em, (u8) EmKind0A_AdjustItem(em, 6));
                    break;
                case 7:
                    EmKind0A_ShowMessage(em, (u8) EmKind0A_AdjustItem(em, 8));
                    break;
                case 8:
                    EmKind0A_ShowMessage(em, (u8) EmKind0A_AdjustItem(em, 0x64));
                    break;
                case 9:
                    EmKind0A_ShowMessage(em, (u8) EmKind0A_AdjustItem(em, 0x68));
                    break;
                }
                break;
            case 6:
                item = Pouch_FindCarryItem(p);
                if (item == 0xFFFF) { /* compiled-out check */
                } else {
                    while (*it != 0xFFFF) {
                        if (*it == item) {
                            if (Rand_Next(1) % 100 >= 20) {
                                p->tradeGetItem = it[1];
                            } else {
                                p->tradeGetItem = it[2];
                            }
                            break;
                        }
                        it += 3;
                    }
                    for (i = 0; i < 20; i++) {
                        if (p->pouch[i].item == item) {
                            Pouch_ClearSlot(p, i);
                        }
                    }
                    Pouch_Add(p, p->tradeGetItem, 1);
                    Notice_Push(1, 0xD, (s16) p->tradeGetItem);
                }
                p->tradeMsgTimer = 0;
                break;
            case 7:
                p->tradeMsgTimer = 0;
                Pouch_Add(p, p->tradeGetItem, 1);
                Notice_Push(1, 0xD, (s16) p->tradeGetItem);
                break;
            case 11:
                p->tradeMsgTimer = 0;
                Pouch_Add(p, 0x2B8, -5);
                tbl = EmKind0A_TradeRates;
                r = Rand_Next(1) % 32;
                p->tradeGetItem = 0x5E;
                sum = 0;
                if ((v = EmKind0A_TradeRates[0].weight) != 0xFFFF) {
                    do {
                        sum += v;
                        if (r < sum) {
                            p->tradeGetItem = tbl->item;
                            break;
                        }
                        tbl++;
                    } while ((v = tbl->weight) != 0xFFFF);
                }
                if (Pouch_Add(p, p->tradeGetItem, 1) == 3) {
                    Notice_Push(1, 3, (s16) p->tradeGetItem);
                } else {
                    Notice_Push(1, 0xD, (s16) p->tradeGetItem);
                }
                break;
            case 4:
                if (w->choice == 0) {
                    Em_SetStateRand(em, 0, 9);
                    Sound_PlaySeFlat(6, 0x21, D_0043C73A);
                    switch (em->msgId) {
                    case 0xF:
                    case 0xE:
                        EmKind0A_ShowMessage(em, (u8) EmKind0A_AdjustItem(em, 0x10));
                        break;
                    case 8:
                    case 9:
                        EmKind0A_ShowMessage(em, (u8) EmKind0A_AdjustItem(em, 0xC));
                        break;
                    }
                    Pouch_Add(p, p->tradeGiveItem, p->tradeGiveCount);
                    Pouch_Add(p, p->tradeGetItem, 1);
                    if (gQuest.traderWantItem == p->tradeGiveItem) {
                        gQuest.traderWantItem = 0;
                    }
                    if (p->tradeGiveCount == -1) {
                        Notice_Push(1, 0x10, (s16) p->tradeGiveItem);
                    } else {
                        Notice_Push(1, 0x11, (s16) p->tradeGiveItem);
                    }
                    Notice_Push(1, 0xD, (s16) p->tradeGetItem);
                } else {
                    Sound_PlaySeFlat(6, 0x20, D_0043C73A);
                    switch (em->msgId) {
                    case 0xF:
                    case 0xE:
                        EmKind0A_ShowMessage(em, (u8) EmKind0A_AdjustItem(em, 0x12));
                        break;
                    case 8:
                    case 9:
                        EmKind0A_ShowMessage(em, (u8) EmKind0A_AdjustItem(em, 0xA));
                        break;
                    }
                }
                break;
            case 5:
                if (w->choice == 0) {
                    Sound_PlaySeFlat(6, 0x21, D_0043C73A);
                    EmKind0A_GiveRandomItem(em);
                } else {
                    Sound_PlaySeFlat(6, 0x20, D_0043C73A);
                    p->tradeMsgTimer = 0;
                }
                break;
            case 10:
                if (w->choice == 0) {
                    Sound_PlaySeFlat(6, 0x21, D_0043C73A);
                    EmKind0A_ShowMessage(em, 0x60);
                    Quest_DrillTakeCharm();
                } else {
                    Sound_PlaySeFlat(6, 0x20, D_0043C73A);
                    EmKind0A_ShowMessage(em, 0x5F);
                }
                break;
            case 1:
                if (w->choice == 0) {
                    Sound_PlaySeFlat(6, 0x21, D_0043C73A);
                    if (m->next != 0xFF) {
                        Em_SetStateRand(em, 0, 9);
                        EmKind0A_ShowMessage(em, m->next);
                    } else {
                        p->tradeMsgTimer = 0;
                    }
                } else {
                    Sound_PlaySeFlat(6, 0x20, D_0043C73A);
                    if (m->nextNo != 0xFF) {
                        EmKind0A_ShowMessage(em, m->nextNo);
                    } else {
                        p->tradeMsgTimer = 0;
                    }
                }
                break;
            case 2:
                if (m->next != 0xFF) {
                    EmKind0A_ShowMessage(em, m->next);
                } else {
                    p->tradeMsgTimer = 0;
                }
                break;
            case 8:
                EmKind0A_ShowMessage(em, Rand_Next(1) % 29 + 0x26);
                break;
            case 9:
                EmKind0A_ShowMessage(em, Rand_Next(1) % 22 + 0x43);
                break;
            }
        } else {
            switch (m->kind) {
            case 1:
            case 4:
            case 5:
            case 0xA:
                if ((p->input.pressed & 0x800) && sel != 0) {
                    w->choice = 0;
                    Sound_PlaySeFlat(0, 0x16, 0);
                } else if ((p->input.pressed & 0x400) && sel == 0) {
                    w->choice = 1;
                    Sound_PlaySeFlat(0, 0x16, 0);
                } else if ((p->input.pressed & 0x40) && sel == 0) {
                    w->choice = 1;
                    Sound_PlaySeFlat(6, 0x20, D_0043C73A);
                }
                break;
            }
        }
    } else if ((p->input.pressed & 0x60) && *(u16 *) &em->msgTimer > 10) {
        *(u16 *) &em->msgTimer = 1000;
    }
        break;
    }
end:
    w->noAnswer = 0;
    return 0;
}

void EmKind0A_Init(Em *em)
{
    EmKind0AWork *w = (EmKind0AWork *)&em->work;
    u16 r;

    em->noHitTimer = 0x63;
    em->noPushTimer = 0x63;
    em->yawCenter = 0;
    em->unk634 = 0;
    em->msgItem = 0;
    r = Rand_Next(1) & 3;
    Em_SetStateRand(em, 0, 0);
    em->yawCenter = 1;
    switch (em->yawCenter) {
    case 0:
        Em_SetMotion(em, 2, 0, 0);
        break;
    case 1:
    case 2:
        Em_SetMotion(em, 1, 0, 0);
        break;
    }
    em->startPos[0] = em->posX;
    em->startPos[1] = em->posY;
    em->startPos[2] = em->posZ;
    em->targetYaw = em->rotY;
    em->stance = 0;
    em->timer = 0;
    if (r == 0xFF) {
        Em_SetStateRand(em, 0, 1);
        Em_SetMotion(em, 2, 0, 0);
    }
    em->hp = 200;
    em->hpMax = 200;
    em->scaleX = 1.0f;
    em->scaleY = 1.0f;
    em->scaleZ = 1.0f;
    em->scriptPending = 0;
    em->blind = 1;
    w->home[0] = em->posX;
    w->home[1] = em->posY;
    w->home[2] = em->posZ;
    w->homeYaw = em->rotY;
    w->unk018 = 0;
    w->noAnswer = 0;
    Em_InitParts(em);
}

void EmKind0A_ToIdle(Em *em, u8 flag, u16 a, u16 b)
{
    EmKind0AWork *w = (EmKind0AWork *)&em->work;

    switch (flag) {
    case 0:
        w->noAnswer = 0;
        switch (em->yawCenter) {
        case 0:
            Em_SetStateRand(em, 0, 0);
            if (em->motionId[0] != 0x3E9) {
                Em_SetMotion(em, 1, a, b);
            }
            break;
        case 1:
            Em_SetStateRand(em, 0, 0xA);
            if (em->motionId[0] != 0x3F5) {
                Em_SetMotion(em, 0xD, a, b);
            }
            break;
        case 2:
            Em_SetStateRand(em, 0, 6);
            if (em->motionId[0] != 0x3ED) {
                Em_SetMotion(em, 5, a, b);
            }
            break;
        }
        break;
    }
    em->fieldCollide = 1;
}

void EmKind0A_TurnToYaw(Em *em)
{
    s32 step;
    u32 diff;

    if ((s16) Player_IsState(em, 0, 3)) {
        step = 0x71C;
    } else {
        step = 0xFA4;
    }
    diff = (em->targetYaw - (em->rotY & 0xFFFF)) & 0xFFFF;
    if ((u32) ((diff + step) & 0xFFFF) < step * 2) {
        em->rotY = em->targetYaw;
    } else if (diff < 0x8000U) {
        em->rotY = (em->rotY + step) & 0xFFFF;
    } else {
        em->rotY = (em->rotY - step) & 0xFFFF;
    }
}

void EmKind0A_State0_00(Em *em)
{
    EmKind0AWork *w = (EmKind0AWork *)&em->work;
    Player *p = &gPlayers[gScene.localPlayer];

    switch (em->step) {
    case 0:
        em->step++;
        em->timer = Rand_Next(1) & 0xBB;
        if (em->yawCenter == 2) {
            if (em->motionId[0] != 0x3ED) {
                Em_SetMotion(em, 5, 2, 0);
            }
        } else {
            if (em->motionId[0] != 0x3E9) {
                Em_SetMotion(em, 1, 2, 0);
            }
        }
        w->noAnswer = 0;
        break;
    case 1:
        if (EmKind0A_IsPlayerNear(em) == 1) {
            if (p->tradeMsgTimer != 0) {
                Em_SetStateRand(em, 0, 3);
            }
        } else {
            em->timer--;
            if (em->timer <= 0) {
                switch (em->yawCenter) {
                case 0:
                    break;
                case 1:
                    if (Rand_Next(1) & 3) {
                        Em_SetStateRand(em, 0, 0xA);
                    } else {
                        Em_SetStateRand(em, 0, 0xB);
                    }
                    break;
                }
            }
        }
        break;
    }
}

void EmKind0A_State0_01(Em *em)
{
    EmKind0AWork *w = (EmKind0AWork *)&em->work;

    switch (em->step) {
    case 0:
        em->step++;
        em->fieldCollide = 0;
        if (em->motionId[0] != 0x3EA) {
            em->posX = w->home[0];
            em->posY = w->home[1];
            em->posZ = w->home[2];
            em->rotY = (s32) w->homeYaw;
            em->targetYaw = w->homeYaw;
            Em_SetMotion(em, 2, 2, 0);
        }
        w->noAnswer = 0;
        break;
    case 1:
        if (EmKind0A_IsPlayerNear(em) == 1) {
            em->fieldCollide = 1;
            Em_SetStateRand(em, 0, 2);
        }
        break;
    }
}

void EmKind0A_State0_02(Em *em)
{
    EmKind0AWork *w = (EmKind0AWork *)&em->work;
    f32 in[3];
    f32 out[3];

    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 3, 0, 0);
        em->stance = 2;
        w->noAnswer = 0;
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 14.0f, 0) != 0) {
            em->step++;
            in[0] = 0.0f;
            in[1] = 0.0f;
            in[2] = 4.0f;
            Vec_Rotate(out, in, (f32 *) em->worldMtx);
            em->velX = out[0];
            em->velY = 6.0f;
            em->velZ = out[2];
            Player_ClearAccel((Player *) em);
            Player_StartJump(em, 0xC);
        }
        break;
    case 2:
        Player_MoveAccel((Player *) em);
        if (em->velY <= 0.0f && em->posY <= em->floorY) {
            em->step++;
            em->stance = 0;
            em->posY = em->floorY;
            Em_SetMotion(em, 4, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            Em_SetMotion(em, 1, 6, 0);
            Em_SetStateRand(em, 0, 0);
        }
        break;
    }
}

void EmKind0A_State0_03(Em *em)
{
    EmKind0AWork *w = (EmKind0AWork *)&em->work;
    Player *p = &gPlayers[gScene.localPlayer];

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 7, 8, 0);
        w->unk018 = 1;
        w->noAnswer = 0;
        em->targetYaw = Vec_Heading(&em->posX, p->pos) - 0x4000;
        Sound_PlaySeFlat(6, 0x1E, D_0043C73A);
        if (Quest_IsDrill() == 1) {
            if ((u16) Pouch_FindCarryItem(p) == 0xFFFF) {
                if (gQuest.drillCharm == 0) {
                    EmKind0A_ShowMessage(em, 0x59);
                } else {
                    EmKind0A_ShowMessage(em, Rand_Next(1) % 3 + 0x61);
                }
            } else {
                EmKind0A_ShowMessage(em, (u8) EmKind0A_AdjustItem(em, 0));
            }
        } else if (p->tradeCount >= 7 && (u16) Pouch_FindCarryItem(p) == 0xFFFF) {
            switch (em->stateRand % 3) {
            case 0:
                EmKind0A_ShowMessage(em, (u8) EmKind0A_AdjustItem(em, 0x1C));
                Em_SetStateRand(em, 0, 5);
                break;
            case 1:
                EmKind0A_ShowMessage(em, (u8) EmKind0A_AdjustItem(em, 0x1E));
                Em_SetStateRand(em, 0, 7);
                break;
            case 2:
                EmKind0A_ShowMessage(em, (u8) EmKind0A_AdjustItem(em, 0x20));
                Em_SetStateRand(em, 0, 8);
                break;
            }
        } else {
            EmKind0A_ShowMessage(em, (u8) EmKind0A_AdjustItem(em, 0));
        }
        break;
    case 1:
        w->noAnswer = 0;
        EmKind0A_Trade(em);
        if (p->tradeMsgTimer == 0) {
            EmKind0A_ToIdle(em, 0, 4, 0);
        } else if (em->motion[0].playing == 0) {
            Em_SetStateRand(em, 0, 4);
        }
        break;
    }
}

void EmKind0A_State0_04(Em *em)
{
    Player *p;
    u8 temp_a1;

    p = &gPlayers[gScene.localPlayer];
    *(s8 *)((u8 *)em + 0x452) = 0;
    switch (em->step) {
    case 0:
        em->step += 1;
        em->stateVar0 = 0;
        em->tradeStep = 0;
        Em_SetMotion(em, 8, 4, 0);
        EmKind0A_Trade(em);
        
    case 1:
        EmKind0A_Trade(em);
        if (p->tradeMsgTimer == 0) {
            EmKind0A_ToIdle(em, 0, 4, 0U);
        }
        break;
    }
}

void EmKind0A_State0_05(Em *em, s32 variant)
{
    EmKind0AWork *w = (EmKind0AWork *)&em->work;
    Player *p = &gPlayers[gScene.localPlayer];

    switch (em->step) {
    case 0:
        em->step++;
        switch (variant) {
        case 0:
            Em_SetMotion(em, 0xB, 6, 0);
            break;
        case 1:
            Em_SetMotion(em, 9, 4, 0);
            break;
        case 2:
            Em_SetMotion(em, 0xA, 4, 0);
            break;
        }
        w->noAnswer = 1;
        EmKind0A_Trade(em);
        break;
    case 1:
        w->noAnswer = 1;
        EmKind0A_Trade(em);
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 1, 2, 0);
        }
        break;
    case 2:
        w->noAnswer = 0;
        EmKind0A_Trade(em);
        if (p->tradeMsgTimer == 0) {
            EmKind0A_ToIdle(em, 0, 2, 0);
        }
        break;
    }
}

void EmKind0A_State0_06(Em *em)
{
    Player *p;
    u8 step;

    step = em->step;
    p = &gPlayers[gScene.localPlayer];
    switch (step) {
    case 0:
        em->step += 1;
        Em_SetMotion(em, 5, 4, 0);
        em->timer = Rand_Next(1) & 0xFFFF & 0xBF;
        break;
    case 1:
        if (--em->timer <= 0) {
            if (Rand_Next(1) & 0xFFFF & 1) {
                em->step = 2;
                em->timer = Rand_Next(1) & 0xFFFF & 0xBF;
                Em_SetMotion(em, 1, 4, 0);
            } else {
                em->step = 3;
                Em_SetMotion(em, 6, 4, 0);
            }
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step = 0;
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step = 0;
        }
        break;
    }
    if ((EmKind0A_IsPlayerNear(em) == 1) && (p->tradeMsgTimer != 0)) {
        Em_SetStateRand(em, 0, 3);
    }
}

void EmKind0A_State0_09(Em *em)
{
    EmKind0AWork *w = (EmKind0AWork *)&em->work;
    Player *p = &gPlayers[gScene.localPlayer];

    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0xC, 6, 0);
        w->noAnswer = 1;
        EmKind0A_Trade(em);
        break;
    case 1:
        w->noAnswer = 1;
        EmKind0A_Trade(em);
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 1, 2, 0);
        }
        break;
    case 2:
        w->noAnswer = 0;
        EmKind0A_Trade(em);
        if (p->tradeMsgTimer == 0) {
            EmKind0A_ToIdle(em, 0, 2, 0);
        }
        break;
    }
}

void EmKind0A_State0_0A(Em *em)
{
    Player *p;
    u8 step;
    u8 *w;
    step = em->step;
    p = &gPlayers[gScene.localPlayer];
    w = &em->work;
    switch (step) {
    case 0:
        em->step += 1;
        em->timer = (Rand_Next(1) & 0xFFFF & 0x3F) + 0x64;
        if (em->motionId[0] != 0x3F5) {
            Em_SetMotion(em, 0xD, 6, 0);
        }
        break;
    case 1:
        if (em->timer > 0) {
            em->timer--;
        }
        if (em->timer <= 0) {
            Em_SetStateRand(em, 0, 0xB);
        } else {
            em->targetAngle = Math_AngleXZ(&em->posX, (f32 *) (w + 8)) & 0xFFFF;
            Angle_TurnToward((s32 *) &em->rotY, (s32 *) &em->targetAngle, 0x100);
            em->targetYaw = (u16) em->rotY;
            Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
        }
        if ((EmKind0A_IsPlayerNear(em) == 1) && (p->tradeMsgTimer != 0)) {
            Em_SetStateRand(em, 0, 3);
        }
        break;
    }
}

void EmKind0A_State0_0B(Em *em)
{
    Player *p;
    u8 step;

    step = em->step;
    p = &gPlayers[gScene.localPlayer];
    switch (step) {
    case 0:
        em->step += 1;
        em->timer = 0;
        Em_SetMotion(em, 0xE, 4, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmKind0A_ToIdle(em, 0, 4, 0U);
        }
        if ((EmKind0A_IsPlayerNear(em) == 1) && (p->tradeMsgTimer != 0)) {
            Em_SetStateRand(em, 0, 3);
        }
        break;
    }
}

void EmKind0A_RunGroup0(Em *em)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->state) {
    case 0:
        EmKind0A_State0_00(em);
        break;
    case 1:
        EmKind0A_State0_01(em);
        break;
    case 2:
        EmKind0A_State0_02(em);
        break;
    case 3:
        EmKind0A_State0_03(em);
        break;
    case 4:
        EmKind0A_State0_04(em);
        break;
    case 5:
        EmKind0A_State0_05(em, 0);
        break;
    case 6:
        EmKind0A_State0_06(em);
        break;
    case 7:
        EmKind0A_State0_05(em, 1);
        break;
    case 8:
        EmKind0A_State0_05(em, 2);
        break;
    case 9:
        EmKind0A_State0_09(em);
        break;
    case 10:
        EmKind0A_State0_0A(em);
        break;
    case 11:
        EmKind0A_State0_0B(em);
        
    default:
        break;
    }
}

void EmKind0A_Update(Em *em)
{
    em->noHitTimer = 0x63;
    em->noPushTimer = 0x63;
    EmKind0A_RunState(em);
    if (em->stateRequest != 0) {
        EmKind0A_RunState(em);
        em->stateRequest = 0;
    }
    EmKind0A_TurnToYaw(em);
}

void EmKind0A_RunState(Em *em)
{
    switch (em->stateGroup) {
    case 0:
        EmKind0A_RunGroup0(em);
        break;
    case 1:
        EmKind0A_RunGroup0(em);
        break;
    case 2:
        EmKind0A_RunGroup0(em);
        break;
    case 3:
        EmKind0A_RunGroup0(em);
        break;
    case 4:
        EmKind0A_RunGroup0(em);
        break;
    case 5:
        EmKind0A_RunGroup0(em);
        break;
    case 6:
        EmKind0A_RunGroup0(em);
        break;
    case 7:
        EmKind0A_RunGroup0(em);
        
    default:
        break;
    }
}

void EmKind0A_MotionEffects(Em *em, EmWork *work)
{
    u16 motionId;

    motionId = em->motionId[0];
    if (motionId != work->fxMotion) {
        work->fxMotion = (s16) motionId;
    }
    EmKind0A_MotionSounds(em, work);
}

void EmKind0A_UpdateMotionEffects(Em *em)
{
    EmKind0AWork *w = (EmKind0AWork *)&em->work;

    switch (w->fxStep) {
    case 0:
        w->fxStep++;
        break;
    case 1:
        EmKind0A_MotionEffects(em, (EmWork *) w);
        break;
    }
}

void EmKind0A_SpawnEffect01(u8 *em)
{
    Effect01_Spawn((Player *) em, 0);
}

void EmKind0A_TableNop(void)
{
}

/* Apceros's own meanings of the work area's bytes (EmWork, include/em.h, has the shared ones). */
typedef struct EmApcerosWork {
    u8 fxStep;                  /* 0x000: UpdateMotionEffects: 0, then 1 */
    u8 unk001[0x00A - 0x001];
    u8 turnOn;                  /* 0x00A: the turn motions (State1_01 to 1_03, State4_05): set at frame 6, cleared at 122; rotY steps while set */
    u8 unk00B[0x00E - 0x00B];
    s16 turnDelta;              /* 0x00E: State1_04: targetAngle - rotY, turned by a 66th of it each frame */
    f32 home[3];                /* 0x010: Init: the spawn position; ChangeState copies it to the goal */
    u8 unk01C[0x020 - 0x01C];
    EmObj sense;                /* 0x020: Em_SenseObjects' object; Update places it at the position (radius 2000, kind 2) and adds it to ObjListA70 (objListed); the hunted states remove it */
    u8 objListed;               /* 0x03C: 1 while sense is in ObjListA70 (Update adds it, State0_0C and the hunted states remove it) */
    u8 alarmed;                 /* 0x03D: CheckHerdHurt: 1 when the herd's leader (variant 2) in the area is in group 4 or 5; Update sets it too; SetRequest drops request 2 while set; ScriptQuery returns it */
    u8 unk03E[0x100 - 0x03E];
} EmApcerosWork;
typedef char EmApcerosWork_size_check[(sizeof(EmApcerosWork) == sizeof(EmWork)) ? 1 : -1];

void EmApceros_PlaySeOnFrame(Em *em, s32 frame, s32 id, s32 bone)
{
    f32 pos[3];
    u8 type;

    if (Player_PassedFrameScaled(em, (f32) frame, 0U) != 0) {
        Mtx_GetTrans(pos, em->model->bones[bone].world);
        type = em->type;
        if (type == 0xC) {
            if (!(em->scaleX <= 0.5f)) {
                goto block_4;
            }
            goto block_7;
        }
block_4:
        if ((type == 0x46) && (em->scaleX <= 0.8f)) {
block_7:
            Em_PlaySeChecked(em, id + 0x20, 0, pos, 4, 0);
            return;
        }
        Em_PlaySeChecked(em, id, 0, pos, 4, 0);
    }
}

void EmApceros_MotionSounds(Em *em, EmWork *work)
{
    s16 motion;

    motion = work->fxMotion;
    switch (motion) {
    case 0x3E9:
        EmApceros_PlaySeOnFrame(em, 0x100, 9, 0x10);
        break;
    case 0x3EA:
        EmApceros_PlaySeOnFrame(em, 0x20, 0xA, 0x10);
        EmApceros_PlaySeOnFrame(em, 0x86, 0xB, 0x10);
        EmApceros_PlaySeOnFrame(em, 0xA0, 8, 0x10);
        EmApceros_PlaySeOnFrame(em, 0x26, 0x14, 0);
        EmApceros_PlaySeOnFrame(em, 0x52, 5, 0x14);
        EmApceros_PlaySeOnFrame(em, 0xCE, 4, 0x14);
        break;
    case 0x3EC:
        EmApceros_PlaySeOnFrame(em, 0x60, 0xB, 0x10);
        EmApceros_PlaySeOnFrame(em, 0xCC, 0xB, 0x10);
        break;
    case 0x3ED:
        EmApceros_PlaySeOnFrame(em, 0x46, 7, 0x10);
        EmApceros_PlaySeOnFrame(em, 0x6A, 8, 0x10);
        break;
    case 0x3EE:
        EmApceros_PlaySeOnFrame(em, 0x20, 8, 0x10);
        break;
    case 0x3EF:
        EmApceros_PlaySeOnFrame(em, 0x2C, 7, 0x10);
        EmApceros_PlaySeOnFrame(em, 0x28, 3, 6);
        EmApceros_PlaySeOnFrame(em, 0x26, 2, 0xA);
        break;
    case 0x3F3:
        EmApceros_PlaySeOnFrame(em, 0x20, 1, 6);
        EmApceros_PlaySeOnFrame(em, 0x6A, 1, 0xA);
        EmApceros_PlaySeOnFrame(em, 0xBA, 1, 6);
        EmApceros_PlaySeOnFrame(em, 0x2E, 4, 0x14);
        EmApceros_PlaySeOnFrame(em, 0x84, 4, 0x17);
        EmApceros_PlaySeOnFrame(em, 0xD0, 4, 0x14);
        break;
    case 0x3F4:
        EmApceros_PlaySeOnFrame(em, 0x26, 3, 6);
        EmApceros_PlaySeOnFrame(em, 0x30, 3, 0xA);
        EmApceros_PlaySeOnFrame(em, 0xC, 6, 0x17);
        EmApceros_PlaySeOnFrame(em, 0x12, 6, 0x14);
        break;
    case 0x3F5:
        EmApceros_PlaySeOnFrame(em, 0x14, 3, 0xA);
        EmApceros_PlaySeOnFrame(em, 0x4E, 3, 0xA);
        EmApceros_PlaySeOnFrame(em, 0x34, 3, 6);
        EmApceros_PlaySeOnFrame(em, 0x1A, 6, 0x14);
        EmApceros_PlaySeOnFrame(em, 0x3A, 6, 0x17);
        break;
    case 0x3FD:
        EmApceros_PlaySeOnFrame(em, 0x40, 1, 6);
        EmApceros_PlaySeOnFrame(em, 0x60, 1, 0xA);
        EmApceros_PlaySeOnFrame(em, 0x8A, 4, 0x17);
        EmApceros_PlaySeOnFrame(em, 0xAE, 4, 0x14);
        break;
    case 0x3FE:
        EmApceros_PlaySeOnFrame(em, 0x40, 1, 0xA);
        EmApceros_PlaySeOnFrame(em, 0x60, 1, 6);
        EmApceros_PlaySeOnFrame(em, 0x8A, 4, 0x14);
        EmApceros_PlaySeOnFrame(em, 0xAE, 4, 0x17);
        break;
    case 0x406:
        EmApceros_PlaySeOnFrame(em, 0x46, 7, 0x10);
        EmApceros_PlaySeOnFrame(em, 0x46, 4, 0x14);
        EmApceros_PlaySeOnFrame(em, 0x20, 4, 0x17);
        EmApceros_PlaySeOnFrame(em, 0x36, 2, 6);
        EmApceros_PlaySeOnFrame(em, 0x18, 2, 0xA);
        break;
    case 0x407:
        EmApceros_PlaySeOnFrame(em, 4, 0xE, 0);
        EmApceros_PlaySeOnFrame(em, 0x2E, 0x14, 0x1B);
        EmApceros_PlaySeOnFrame(em, 0x3C, 7, 0x10);
        EmApceros_PlaySeOnFrame(em, 0x34, 4, 0x14);
        EmApceros_PlaySeOnFrame(em, 0x7A, 1, 0x17);
        EmApceros_PlaySeOnFrame(em, 0xA6, 5, 0x14);
        EmApceros_PlaySeOnFrame(em, 0xD2, 2, 0xA);
        break;
    case 0x408:
        EmApceros_PlaySeOnFrame(em, 0x46, 0x13, 0x10);
        EmApceros_PlaySeOnFrame(em, 0xE6, 0x13, 0x10);
        EmApceros_PlaySeOnFrame(em, 0xCE, 4, 0x14);
        EmApceros_PlaySeOnFrame(em, 0x144, 4, 0x14);
        EmApceros_PlaySeOnFrame(em, 0x38, 4, 0x17);
        EmApceros_PlaySeOnFrame(em, 0xA2, 4, 0x17);
        EmApceros_PlaySeOnFrame(em, 0x4C, 2, 6);
        EmApceros_PlaySeOnFrame(em, 0x7A, 2, 6);
        EmApceros_PlaySeOnFrame(em, 0xFC, 2, 6);
        EmApceros_PlaySeOnFrame(em, 0x128, 2, 6);
        EmApceros_PlaySeOnFrame(em, 0x62, 2, 0xA);
        EmApceros_PlaySeOnFrame(em, 0x90, 2, 0xA);
        EmApceros_PlaySeOnFrame(em, 0xE6, 2, 0xA);
        EmApceros_PlaySeOnFrame(em, 0x118, 2, 0xA);
        break;
    case 0x409:
    case 0x40A:
        EmApceros_PlaySeOnFrame(em, 0x2C, 4, 0x14);
        EmApceros_PlaySeOnFrame(em, 0x70, 4, 0x14);
        EmApceros_PlaySeOnFrame(em, 0x1C, 4, 0x17);
        EmApceros_PlaySeOnFrame(em, 0x5C, 4, 0x17);
        EmApceros_PlaySeOnFrame(em, 0x3A, 2, 6);
        EmApceros_PlaySeOnFrame(em, 0x7C, 2, 6);
        EmApceros_PlaySeOnFrame(em, 0x26, 2, 0xA);
        EmApceros_PlaySeOnFrame(em, 0x4A, 2, 0xA);
        break;
    case 0x40B:
        EmApceros_PlaySeOnFrame(em, 0x2A, 4, 0x14);
        EmApceros_PlaySeOnFrame(em, 0x38, 1, 6);
        EmApceros_PlaySeOnFrame(em, 0x5A, 4, 0x17);
        EmApceros_PlaySeOnFrame(em, 0x6A, 1, 0xA);
        EmApceros_PlaySeOnFrame(em, 0x5A, 0xA, 0x10);
        break;
    case 0x40C:
        EmApceros_PlaySeOnFrame(em, 0x1A, 0xE, 0x14);
        EmApceros_PlaySeOnFrame(em, 0x1A, 0x14, 0x10);
        EmApceros_PlaySeOnFrame(em, 0x14, 0x13, 0x10);
        EmApceros_PlaySeOnFrame(em, 0x14, 3, 6);
        EmApceros_PlaySeOnFrame(em, 0x18, 6, 0x17);
        EmApceros_PlaySeOnFrame(em, 0x32, 3, 6);
        EmApceros_PlaySeOnFrame(em, 0x44, 4, 0x14);
        break;
    case 0x40D:
        EmApceros_PlaySeOnFrame(em, 0x14, 7, 0x10);
        EmApceros_PlaySeOnFrame(em, 0x5C, 7, 0x10);
        EmApceros_PlaySeOnFrame(em, 0x1E, 0x14, 0x1B);
        EmApceros_PlaySeOnFrame(em, 0x5C, 0x14, 0x1B);
        EmApceros_PlaySeOnFrame(em, 0xA, 4, 0x14);
        EmApceros_PlaySeOnFrame(em, 0x50, 4, 0x14);
        EmApceros_PlaySeOnFrame(em, 0x1E, 4, 0x17);
        EmApceros_PlaySeOnFrame(em, 0x5E, 4, 0x17);
        EmApceros_PlaySeOnFrame(em, 0x2C, 1, 0xA);
        EmApceros_PlaySeOnFrame(em, 0x74, 1, 0xA);
        EmApceros_PlaySeOnFrame(em, 0x18, 1, 6);
        EmApceros_PlaySeOnFrame(em, 0x42, 1, 6);
        break;
    case 0x425:
        EmApceros_PlaySeOnFrame(em, 4, 0x11, 0x10);
        EmApceros_PlaySeOnFrame(em, 0x10, 1, 6);
        EmApceros_PlaySeOnFrame(em, 0x1A, 1, 0xA);
        EmApceros_PlaySeOnFrame(em, 0x68, 0xE, 0x1B);
        EmApceros_PlaySeOnFrame(em, 0x6E, 1, 0xA);
        EmApceros_PlaySeOnFrame(em, 0x8C, 1, 0x17);
        break;
    case 0x426:
        EmApceros_PlaySeOnFrame(em, 4, 0x11, 0x10);
        EmApceros_PlaySeOnFrame(em, 0x10, 1, 0xA);
        EmApceros_PlaySeOnFrame(em, 0x1A, 1, 6);
        EmApceros_PlaySeOnFrame(em, 0x68, 0xE, 0x1B);
        EmApceros_PlaySeOnFrame(em, 0x6E, 1, 6);
        EmApceros_PlaySeOnFrame(em, 0x8C, 1, 0x14);
        break;
    case 0x427:
        EmApceros_PlaySeOnFrame(em, 4, 0x11, 0x10);
        EmApceros_PlaySeOnFrame(em, 0x1C, 0x12, 0);
        EmApceros_PlaySeOnFrame(em, 0x22, 0xF, 0);
        EmApceros_PlaySeOnFrame(em, 0x44, 3, 0xA);
        EmApceros_PlaySeOnFrame(em, 0x50, 4, 0x17);
        break;
    case 0x428:
        EmApceros_PlaySeOnFrame(em, 0x14, 0x10, 0x10);
        EmApceros_PlaySeOnFrame(em, 4, 0x16, 0x10);
        EmApceros_PlaySeOnFrame(em, 0x64, 0x15, 0);
        EmApceros_PlaySeOnFrame(em, 0xC2, 0x15, 0);
        EmApceros_PlaySeOnFrame(em, 0x32, 0xE, 0x14);
        EmApceros_PlaySeOnFrame(em, 0x96, 0xE, 0x14);
        break;
    case 0x429:
        EmApceros_PlaySeOnFrame(em, 6, 8, 0x10);
        EmApceros_PlaySeOnFrame(em, 0x42, 8, 0x10);
        break;
    case 0x42A:
        EmApceros_PlaySeOnFrame(em, 0xA, 0xE, 6);
        EmApceros_PlaySeOnFrame(em, 0x14, 0xA, 0x10);
        EmApceros_PlaySeOnFrame(em, 0x30, 3, 6);
        EmApceros_PlaySeOnFrame(em, 0x4E, 5, 0x14);
        EmApceros_PlaySeOnFrame(em, 0x64, 4, 0x17);
        EmApceros_PlaySeOnFrame(em, 0x82, 1, 6);
        break;
    case 0x42B:
        EmApceros_PlaySeOnFrame(em, 0x1A, 1, 6);
        EmApceros_PlaySeOnFrame(em, 0x1A, 0xE, 6);
        EmApceros_PlaySeOnFrame(em, 0x18, 0xE, 0x17);
        EmApceros_PlaySeOnFrame(em, 0x54, 0x12, 0);
        EmApceros_PlaySeOnFrame(em, 0x60, 0xF, 0);
        EmApceros_PlaySeOnFrame(em, 0x7E, 3, 6);
        EmApceros_PlaySeOnFrame(em, 0x8C, 4, 0x14);
        break;
    case 0x42C:
        EmApceros_PlaySeOnFrame(em, 2, 9, 0x10);
        EmApceros_PlaySeOnFrame(em, 0xB4, 9, 0x10);
        break;
    case 0x42D:
        EmApceros_PlaySeOnFrame(em, 6, 0x10, 0x10);
        break;
    }
}

void EmApceros_Init(Em *em)
{
    EmApcerosWork *w = (EmApcerosWork *)&em->work;
    s16 hp;

    if (gQuest.stage == 0) {
        em->posX = 2400.0f + 250.0f * (f32) (u32) em->slotNo;
        em->posY = 0.0f;
        em->posZ = 3600.0f;
    }
    Em_SetMotionRow(em, 1, 0, 0);
    em->stance = 0;
    if (em->type == 0xC) {
        if (em->variant == 0 || em->variant == 5) {
            hp = Em_ComputeSize(em, 1);
            em->hp = hp;
            em->hpMax = hp;
        } else {
            hp = Em_ComputeSize(em, 0);
            em->hp = hp;
            em->hpMax = hp;
        }
        em->scaleX = EmApceros_ScalesType0C[em->variant];
        em->scaleY = EmApceros_ScalesType0C[em->variant];
        em->scaleZ = EmApceros_ScalesType0C[em->variant];
    } else if (em->type == 0x46) {
        hp = Em_ComputeSize(em, 0);
        em->hp = hp;
        em->hpMax = hp;
        em->scaleX = EmApceros_ScalesType46[em->variant];
        em->scaleY = EmApceros_ScalesType46[em->variant];
        em->scaleZ = EmApceros_ScalesType46[em->variant];
    } else {
        hp = Em_ComputeSize(em, 0);
        em->hp = hp;
        em->hpMax = hp;
        em->scaleX = 1.0f;
        em->scaleY = 1.0f;
        em->scaleZ = 1.0f;
    }
    em->bodyScale = em->scaleX;
    GmEm_ApplySize(em);
    em->timer = 0;
    em->scriptPending = 1;
    em->blind = 0;
    w->home[0] = em->posX;
    w->home[1] = em->posY;
    w->home[2] = em->posZ;
    em->remote = 0;
    w->objListed = 0;
    w->alarmed = 0;
}

void EmApceros_Start(Em *em)
{
    EmApceros_Init(em);
    EmApceros_ChangeState(em, 0, 1, 0);
}

void EmApceros_Reinit(Em *em)
{
    EmApceros_Init(em);
}

void EmApceros_ChangeState(Em *em, u16 group, u16 state, u8 mode)
{
    EmApcerosWork *w = (EmApcerosWork *)&em->work;

    switch (group) {
    case 1:
        switch (state) {
        case 0:
            Vec_Copy(em->goal, w->home);
            break;
        case 4:
            break;
        case 5:
        case 8:
            Em_SetSpeedScaled(em, 2, 1000.0f, 200.0f);
            if (em->goalDist > 5000.0f) {
                em->goalDist = 5000.0f;
            }
            break;
        case 9:
            Vec_Copy(&em->posX, em->goal);
            Vec_Copy(&em->prevPosX, em->goal);
            break;
        }
        break;
    case 5:
        if (state == 3 && QuestEm_CanAppear(em) == 1) {
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
    Em_SetStateSync(em, group, state, mode);
}

void EmApceros_EndState(Em *em)
{
    em->scriptPending = 1;
    em->stance = 0;
    if ((em->stateGroup == 0 && em->state == 10) || (em->stateGroup == 0 && em->state == 13) ||
        (em->stateGroup == 1 && em->state == 6) || (em->stateGroup == 1 && em->state == 7) ||
        (em->stateGroup == 3 && em->state == 2)) {
        EmApceros_ChangeState(em, 0, 13, 0);
    } else {
        EmApceros_ChangeState(em, 0, 1, 0);
    }
}

void EmApceros_SetRequest(Em *em, s32 a)
{
    EmApcerosWork *w = (EmApcerosWork *)&em->work;

    switch ((u8) a) {
    case 2:
        if (w->alarmed != 0) {
            return;
        }
    }
    Em_SetRequest(em, a, 0x90);
}

void EmApceros_CheckHerdHurt(Em *em)
{
    EmApcerosWork *w = (EmApcerosWork *)&em->work;
    Em *m;
    s16 i;

    if (em->type == 0x46 && em->variant != 2) {
        for (i = 0, m = gEms; i < 20; i++, m++) {
            if (em != m && m->active && m->visible && (u8) Obj_SameArea(em, (Player *) m)) {
                if (m->type == 0x46 && m->variant == 2 && (m->stateGroup == 4 || m->stateGroup == 5)) {
                    w->alarmed = 1;
                    return;
                }
            }
        }
    }
}

void EmApceros_Group2None(void)
{
}

void EmApceros_State0_01(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        if (em->motionId[0] != 0x3E9) {
            Em_SetMotion(em, 1, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmApceros_EndState(em);
        }
        break;
    }
}

void EmApceros_State0_02(Em *em)
{
    s32 d;
    f32 v[3];

    Vec_Copy(v, (f32 *) ((u8 *) em + 0x448));
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = (em->stateRand & 3) * 30 + 120;
        if (em->motionId[0] != 0x3F3) {
            Em_SetMotion(em, 0xB, 0, 0);
        }
        if ((u16) ((u16) (Math_Heading(em->posX, em->posZ, v[0], v[2]) + 0x4000) - em->rotY) <= 0x8000U) {
            em->stateVar1 = 0;
        } else {
            em->stateVar1 = 1;
        }
        break;
    case 1:
        if (em->motion[0].blend == 0) {
            if (--em->timer <= 0) {
                EmApceros_EndState(em);
            } else {
                if (em->stateVar1) {
                    d = 0x40;
                } else {
                    d = -0x40;
                }
                em->rotY += d;
                em->rotY = (u16) em->rotY;
                Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
            }
        }
        break;
    }
}

void EmApceros_State0_03(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = ((em->stateRand & 7) * 0x1E) + 0x168;
        if (em->motionId[0] != 0x3EB) {
            Em_SetMotion(em, 3, 0, 0);
        }
        break;
    case 1:
        if (--em->timer <= 0) {
            EmApceros_EndState(em);
        }
        break;
    }
}

void EmApceros_State0_04(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 0;
        if (em->motionId[0] != 0x3EA) {
            Em_SetMotion(em, 2, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            Em_SetMotion(em, 7, 0, 0);
            em->step++;
            em->timer = 0;
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmApceros_EndState(em);
        }
        break;
    }
}

void EmApceros_State0_05(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = ((em->stateRand & 3) * 0x1E) + 0xB4;
        Em_SetMotion(em, 4, 0, 0);
        break;
    case 1:
        if (--em->timer <= 0) {
            EmApceros_EndState(em);
        }
        break;
    }
}

void EmApceros_State0_06(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 5, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmApceros_EndState(em);
        }
        break;
    }
}

void EmApceros_State0_07(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 6, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmApceros_EndState(em);
        }
        break;
    }
}

void EmApceros_State0_08(Em *em)
{
    f32 v[3];

    Em_SetHitReactMode(em, 2);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x3F, 0, 0);
        Em_BeginStatus2(em);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 30.0f) == 0 && em->motion[0].blend == 0) {
            v[0] = 21.428572f * em->scaleX;
            v[1] = 0.0f;
            v[2] = 0.0f;
            Vec_RotateY(v, 6.2831855f * em->rotY / 65536.0f);
            em->posX += v[0];
            em->posZ += v[2];
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x44, 0, 0);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            EmApceros_ChangeState(em, 0, 9, 4);
        }
        break;
    }
}

void EmApceros_State0_09(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x42, 0, 0);
        em->blind = 2;
        Em_EndStatus2(em);
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            EmApceros_EndState(em);
        }
        break;
    }
}

void EmApceros_State0_0A(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x1E, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmApceros_EndState(em);
        }
        break;
    }
}

void EmApceros_State0_0B(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x23, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmApceros_EndState(em);
        }
        break;
    }
}

void EmApceros_State0_0C(Em *em)
{
    EmApcerosWork *w = (EmApcerosWork *)&em->work;

    em->noHitTimer = 0xA;
    em->noPushTimer = 0xA;
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 0x64;
        if (em->motionId[0] != 0x3F5) {
            Em_SetMotion(em, 0xD, 0, 0);
        }
        break;
    case 1:
        em->timer -= 1;
        em->alpha = (f32) em->timer / 100.0f;
        if (em->timer <= 0) {
            if (w->objListed == 1) {
                ObjListA70_Remove(&w->sense);
                w->objListed = 0U;
            }
            QuestEm_Leave(em);
            em->taskPhase++;
            em->visible = 0;
        }
        break;
    }
}

void EmApceros_State0_0D(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x20, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmApceros_EndState(em);
        }
        break;
    }
}

void EmApceros_RunGroup0(Em *em)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->state) {
    case 1:
        EmApceros_State0_01(em);
        break;
    case 2:
        EmApceros_State0_02(em);
        break;
    case 3:
        EmApceros_State0_03(em);
        break;
    case 4:
        EmApceros_State0_04(em);
        break;
    case 5:
        EmApceros_State0_05(em);
        break;
    case 6:
        EmApceros_State0_06(em);
        break;
    case 7:
        EmApceros_State0_07(em);
        break;
    case 8:
        EmApceros_State0_08(em);
        break;
    case 9:
        EmApceros_State0_09(em);
        break;
    case 10:
        EmApceros_State0_0A(em);
        break;
    case 11:
        EmApceros_State0_0B(em);
        break;
    case 12:
        EmApceros_State0_0C(em);
        break;
    case 13:
        EmApceros_State0_0D(em);
        
    case 0x0:
    default:
        break;
    }
}

void EmApceros_RunGroup2(void)
{
    EmApceros_Group2None();
}

void EmApceros_State1_00(Em *em)
{
    s32 d;

    switch (em->step) {
    case 0:
        em->step++;
        em->timer = (em->stateRand & 3) * 30 + 210;
        if (em->motionId[0] != 0x3F3) {
            Em_SetMotion(em, 0xB, 0, 0);
        }
        if ((u16) ((u16) (Math_Heading(em->posX, em->posZ, em->goal[0], em->goal[2]) + 0x4000) - em->rotY) <= 0x8000U) {
            em->stateVar1 = 0;
        } else {
            em->stateVar1 = 1;
        }
        break;
    case 1:
        if (em->motion[0].blend == 0) {
            if (--em->timer <= 0) {
                EmApceros_EndState(em);
            } else {
                if (em->stateVar1) {
                    d = 0x40;
                } else {
                    d = -0x40;
                }
                em->rotY += d;
                em->rotY = (u16) em->rotY;
                Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
            }
        }
        break;
    }
}

void EmApceros_State1_01(Em *em)
{
    EmApcerosWork *w = (EmApcerosWork *)&em->work;
    s32 d;
    s32 d2;

    switch (em->step) {
    case 0:
        em->step++;
        if (em->motionId[0] != 0x406) {
            em->targetDist = 400.0f;
        }
        Em_SetMotion(em, 0x1E, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            if (Em_FindTargetPlayer(em, 0x78E4, 700.0f, 300.0f) != -1) {
                d = (em->targetAngle - em->rotY) & 0xFFFF;
                if (d <= 0x2FFF) {
                    em->step = 4;
                    Em_SetMotion(em, 0x22, 0, 0);
                    em->stateVar1 = 0;
                    w->turnOn = 0;
                } else if (d > 0xD000) {
                    em->step = 4;
                    Em_SetMotion(em, 0x21, 0, 0);
                    em->stateVar1 = 1;
                    w->turnOn = 0;
                } else if (d > 0x5400 && d < 0xA600) {
                    EmApceros_ChangeState(em, 3, 0, 4);
                } else {
                    em->step++;
                    Em_SetMotion(em, 0x20, 0, 0);
                }
            } else {
                EmApceros_EndState(em);
            }
        }
        break;
    case 2:
        if (em->motion[0].blend == 0) {
            em->step++;
            em->timer = (em->stateRand & 3) * 30 + 210;
        }
        break;
    case 3:
        if (Em_FindTargetPlayer(em, 0x8000, 500.0f, 200.0f) != -1) {
            d2 = (em->targetAngle - em->rotY) & 0xFFFF;
            if (d2 > 0x5400 && d2 < 0xA600) {
                EmApceros_ChangeState(em, 3, 0, 4);
                break;
            }
            em->timer--;
            if (EmMotion_ReachedFrame(em, 0, 14.0f) != 0) {
                if (d2 < 0x8000) {
                    if (em->timer <= 0) {
                        em->step++;
                        Em_SetMotion(em, 0x22, 0, 0);
                        em->stateVar1 = 0;
                        w->turnOn = 0;
                    }
                    em->rotY -= 0x42;
                } else {
                    if (em->timer <= 0) {
                        em->step++;
                        Em_SetMotion(em, 0x21, 0, 0);
                        em->stateVar1 = 1;
                        w->turnOn = 0;
                    }
                    em->rotY += 0x42;
                }
            }
        } else {
            em->step = 5;
            Em_SetMotion(em, 0x23, 0, 0);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->step = 2;
            Em_SetMotion(em, 0x20, 0, 0);
            break;
        }
        if (Player_PassedFrameScaled(em, 6.0f, 0) != 0) {
            w->turnOn++;
        } else if (Player_PassedFrameScaled(em, 122.0f, 0) != 0) {
            w->turnOn = 0;
        }
        if (w->turnOn != 0) {
            if (em->stateVar1 == 0) {
                em->rotY -= 0x1D7;
            } else {
                em->rotY += 0x1D7;
            }
        }
        break;
    case 5:
        if (em->motion[0].playing == 0) {
            EmApceros_EndState(em);
        }
        break;
    }
}

void EmApceros_State1_02(Em *em)
{
    EmApcerosWork *w = (EmApcerosWork *)&em->work;

    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x15, 0, 0);
        w->turnOn = 0;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmApceros_EndState(em);
            w->turnOn = 0;
            break;
        }
        if (Player_PassedFrameScaled(em, 42.0f, 0) != 0) {
            w->turnOn++;
        } else if (Player_PassedFrameScaled(em, 170.0f, 0) != 0) {
            w->turnOn = 0;
        }
        if (w->turnOn != 0) {
            em->rotY += 0x100;
        }
        break;
    }
}

void EmApceros_State1_03(Em *em)
{
    EmApcerosWork *w = (EmApcerosWork *)&em->work;

    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x16, 0, 0);
        w->turnOn = 0;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmApceros_EndState(em);
            w->turnOn = 0;
            break;
        }
        if (Player_PassedFrameScaled(em, 42.0f, 0) != 0) {
            w->turnOn++;
        } else if (Player_PassedFrameScaled(em, 170.0f, 0) != 0) {
            w->turnOn = 0;
        }
        if (w->turnOn != 0) {
            em->rotY -= 0x100;
        }
        break;
    }
}

void EmApceros_State1_04(Em *em)
{
    EmApcerosWork *w = (EmApcerosWork *)&em->work;

    switch (em->step) {
    case 0:
        em->step++;
        em->targetAngle = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        w->turnDelta = em->targetAngle - em->rotY;
        if ((u16)w->turnDelta < 0x8000) {
            Em_SetMotion(em, 0x15, 0, 0);
        } else {
            Em_SetMotion(em, 0x16, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmApceros_EndState(em);
        } else if (EmMotion_InFrameRange(em, 0, 42.0f, 170.0f) != 0) {
            em->rotY += (u16) (w->turnDelta / 66.0f);
        }
        break;
    }
}

void EmApceros_State1_05(Em *em)
{
    f32 pos[3];

    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 300;
        if (em->motionId[0] != 0x3F5) {
            Em_SetMotion(em, 0xD, 0, 0);
        }
        if (em->requested == 1) {
            em->collMask |= 0x80;
        }
        break;
    case 1:
        if (em->hasGoal != 0) {
            em->rotY += (s16) (u16) ((s16) Math_AngleXZ(&em->posX, em->goal) - em->rotY) / 16;
        }
        Em_MotionDelta(em, pos);
        em->goalDist -= pos[2];
        if (em->goalDist <= 0.0f) {
            em->step++;
            EmApceros_EndState(em);
        }
        break;
    }
}

void EmApceros_State1_06(Em *em)
{
    f32 turnF;
    u32 turn;

    switch (em->step) {
    case 0:
        em->step++;
        em->aimAngle = ((Math_AngleXZ(&em->posX, em->goal) & 0xFFFF) - em->rotY) + 0x7FFF + 1;
        Em_SetMotion(em, 0x20, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmApceros_EndState(em);
            break;
        }
        if (EmMotion_InFrameRange(em, 0, 14.0f, 332.0f) != 0) {
            turnF = (f32) (s16) em->aimAngle / 159.0f;
            turn = (u32) turnF;
            em->rotY += (s32) turn & 0xFFFF;
        }
        break;
    }
}

void EmApceros_State1_07(Em *em)
{
    f32 turnF;
    u32 turn;

    switch (em->step) {
    case 0:
        em->step++;
        em->aimAngle = ((Math_AngleXZ(&em->posX, em->goal) & 0xFFFF) - em->rotY) + 0x7FFF + 1;
        if ((s32) em->aimAngle < 0x8000) {
            Em_SetMotion(em, 0x21, 0, 0);
            break;
        }
        Em_SetMotion(em, 0x22, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmApceros_EndState(em);
            break;
        }
        if (EmMotion_InFrameRange(em, 0, 6.0f, 122.0f) != 0) {
            turnF = (f32) (s16) em->aimAngle / 58.0f;
            turn = (u32) turnF;
            em->rotY += (s32) turn & 0xFFFF;
        }
        break;
    }
}

void EmApceros_State1_08(Em *em)
{
    f32 pos[3];

    switch (em->step) {
    case 0:
        em->step++;
        if (em->motionId[0] != 0x3F3) {
            Em_SetMotion(em, 0xB, 0, 0);
        }
        break;
    case 1:
        if (em->hasGoal != 0) {
            em->rotY += (s16) (u16) ((s16) Math_AngleXZ(&em->posX, em->goal) - em->rotY) / 16;
        }
        Em_MotionDelta(em, pos);
        em->goalDist -= pos[2];
        if (em->goalDist <= 0.0f) {
            em->step++;
            EmApceros_EndState(em);
        }
        break;
    }
}

void EmApceros_State1_09(Em *em)
{
    u8 step;

    step = em->step;
    switch (step) {
    case 0:
        em->step = step + 1;
        EmApceros_EndState(em);
        break;
    }
}

void EmApceros_State1_0A(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->speedBonus = 0x32;
        Em_StartTurnToTarget(em, &EmApceros_TurnParams[0]);
        break;
    case 1:
        if (Em_TurnStep(em, &EmApceros_TurnParams[0]) == 1) {
            EmApceros_EndState(em);
        }
        break;
    }
}

void EmApceros_RunGroup1(Em *em)
{
    switch (em->state) {
    case 0:
        EmApceros_State1_00(em);
        break;
    case 1:
        EmApceros_State1_01(em);
        break;
    case 2:
        EmApceros_State1_02(em);
        break;
    case 3:
        EmApceros_State1_03(em);
        break;
    case 4:
        EmApceros_State1_04(em);
        break;
    case 5:
        EmApceros_State1_05(em);
        break;
    case 6:
        EmApceros_State1_06(em);
        break;
    case 7:
        EmApceros_State1_07(em);
        break;
    case 8:
        EmApceros_State1_08(em);
        break;
    case 9:
        EmApceros_State1_09(em);
        break;
    case 10:
        EmApceros_State1_0A(em);
        
    default:
        break;
    }
}

void EmApceros_State3_00(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x1F, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmApceros_EndState(em);
        }
        break;
    }
}

void EmApceros_State3_01(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 6, 0, 0);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 102.0f, 0U) != 0) {
            em->step++;
            Em_SetMotion(em, 0x24, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmApceros_EndState(em);
        }
        break;
    }
}

void EmApceros_State3_02(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x25, 0, 0);
        break;
    case 1:
        if ((EmMotion_ReachedFrame(em, 0, 128.0f) == 0) && (em->motion[0].blend == 0)) {
            em->rotY += 0x1C7;
        }
        if (em->motion[0].playing == 0) {
            EmApceros_EndState(em);
        }
        break;
    }
}

void EmApceros_State3_03(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x24, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmApceros_EndState(em);
        }
        break;
    }
}

void EmApceros_State3_04(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 2, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 7, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmApceros_EndState(em);
        }
        break;
    }
}

void EmApceros_RunGroup3(Em *em)
{
    switch (em->state) {
    case 0:
        EmApceros_State3_00(em);
        break;
    case 1:
        EmApceros_State3_01(em);
        break;
    case 2:
        EmApceros_State3_02(em);
        break;
    case 3:
        EmApceros_State3_03(em);
        break;
    case 4:
        EmApceros_State3_04(em);
        break;
    }
}

void EmApceros_State4_00(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        if (em->motionId[0] != 0x424) {
            Em_SetMotion(em, 0x3C, 0, 0);
        }
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmApceros_ChangeState(em, 0, 1, 0);
            em->hp = 0x40;
            break;
        }
        Player_PassedFrameScaled(em, 2.0f, 0U);
        break;
    }
}

void EmApceros_State4_01(Em *em)
{
    f32 v[3];

    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x3E, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 22.0f) == 0 && em->motion[0].blend == 0) {
            v[0] = -10.909091f * em->scaleX;
            v[1] = 0.0f;
            v[2] = 0.0f;
            Vec_RotateY(v, 6.2831855f * em->rotY / 65536.0f);
            em->posX += v[0];
            em->posZ += v[2];
        }
        if (em->motion[0].playing == 0) {
            EmApceros_EndState(em);
        }
        break;
    }
}

void EmApceros_State4_02(Em *em)
{
    EmApcerosWork *w = (EmApcerosWork *)&em->work;
    f32 v[3];

    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x3D, 0, 0);
        w->turnOn = 0;
        Em_RestartScript(em);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 22.0f) == 0 && em->motion[0].blend == 0) {
            v[0] = 10.909091f * em->scaleX;
            v[1] = 0.0f;
            v[2] = 0.0f;
            Vec_RotateY(v, 6.2831855f * em->rotY / 65536.0f);
            em->posX += v[0];
            em->posZ += v[2];
        }
        if (em->motion[0].playing == 0) {
            EmApceros_EndState(em);
        }
        break;
    }
}

void EmApceros_State4_03(Em *em, s32 variant)
{
    f32 v[3];

    switch (em->step) {
    case 0:
        if (variant == 0) {
            em->step++;
            Em_SetMotion(em, 0x3E, 0, 0);
        } else {
            em->step = 2;
            if (em->motionId[0] != 0x427) {
                Em_SetMotion(em, 0x3F, 0, 0);
            }
        }
        Em_RestartScript(em);
        break;
    case 1:
        v[0] = -10.909091f * em->scaleX;
        v[1] = 0.0f;
        v[2] = 0.0f;
        Vec_RotateY(v, 6.2831855f * em->rotY / 65536.0f);
        em->posX += v[0];
        em->posZ += v[2];
        if (Player_PassedFrameScaled(em, 12.0f, 0) == 0) {
            em->step = 2;
            Em_SetMotion(em, 0x43, 0, 0);
        }
        break;
    case 2:
        if (variant == 0) {
            if (EmMotion_ReachedFrame(em, 0, 8.0f) == 0 && em->motion[0].blend == 0) {
                v[0] = -10.909091f * em->scaleX;
                v[1] = 0.0f;
                v[2] = 0.0f;
                Vec_RotateY(v, 6.2831855f * em->rotY / 65536.0f);
                em->posX += v[0];
                em->posZ += v[2];
            }
        } else {
            if (EmMotion_ReachedFrame(em, 0, 30.0f) == 0 && em->motion[0].blend == 0) {
                v[0] = 21.428572f * em->scaleX;
                v[1] = 0.0f;
                v[2] = 0.0f;
                Vec_RotateY(v, 6.2831855f * em->rotY / 65536.0f);
                em->posX += v[0] * em->scaleX;
                em->posZ += v[2];
            }
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 200;
            Em_SetMotion(em, 0x40, 0, 0);
        }
        break;
    case 3:
        if (--em->timer <= 0) {
            if (Player_PassedFrameScaled(em, 98.0f, 0) != 0) {
                em->step++;
                Em_SetMotion(em, 0x42, 0, 0);
            }
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            EmApceros_EndState(em);
        }
        break;
    }
}

void EmApceros_State4_05(Em *em)
{
    EmApcerosWork *w = (EmApcerosWork *)&em->work;
    f32 v[3];

    switch (em->step) {
    case 0:
        em->step++;
        if (em->stateRand & 1) {
            Em_SetMotion(em, 0x3D, 0, 0);
            w->turnOn = 0;
        } else {
            Em_SetMotion(em, 0x3E, 0, 0);
            w->turnOn = 1;
        }
        if (em->requested != 1) {
            EmApceros_SetRequest(em, 4);
        }
        Em_RestartScript(em);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 22.0f) == 0 && em->motion[0].blend == 0) {
            if (w->turnOn == 0) {
                v[0] = 10.909091f * em->scaleX;
            } else {
                v[0] = -10.909091f * em->scaleX;
            }
            v[1] = 0.0f;
            v[2] = 0.0f;
            Vec_RotateY(v, 6.2831855f * em->rotY / 65536.0f);
            em->posX += v[0];
            em->posZ += v[2];
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = em->blindTimer;
            EmApceros_EndState(em);
        }
        break;
    }
}

void EmApceros_State4_06(Em *em)
{
    f32 v[3];

    switch (em->step) {
    case 0:
        em->step++;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x3D, 0, 0);
        Em_BeginStatus4(em);
        em->noReact = 1;
        Em_SetHitReactMode(em, 4);
        break;
    case 1:
        Em_SetHitReactMode(em, 4);
        if (EmMotion_ReachedFrame(em, 0, 22.0f) == 0 && em->motion[0].blend == 0) {
            v[0] = 10.909091f * em->scaleX;
            v[1] = 0.0f;
            v[2] = 0.0f;
            Vec_RotateY(v, 6.2831855f * em->rotY / 65536.0f);
            em->posX += v[0];
            em->posZ += v[2];
        }
        if (Player_PassedFrameScaled(em, 30.0f, 0) != 0) {
            em->step++;
            Em_SetMotion(em, 0x45, 0, 0);
        }
        break;
    case 2:
        Em_SetHitReactMode(em, 4);
        Em_SpawnEffect06Bone(em, 2);
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x3D, 6, 0x1E);
            Em_EndStatus4(em);
        }
        break;
    case 3:
        if (EmMotion_ReachedFrame(em, 0, 22.0f) == 0 && em->motion[0].blend == 0) {
            v[0] = 10.909091f * em->scaleX;
            v[1] = 0.0f;
            v[2] = 0.0f;
            Vec_RotateY(v, 6.2831855f * em->rotY / 65536.0f);
            em->posX += v[0] * em->scaleX;
            em->posZ += v[2];
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmApceros_EndState(em);
        }
        break;
    }
}

void EmApceros_RunGroup4(Em *em)
{
    em->fieldCollide = 1;
    switch (em->state) {
    case 0:
        EmApceros_State4_00(em);
        break;
    case 1:
        EmApceros_State4_01(em);
        break;
    case 2:
        EmApceros_State4_02(em);
        break;
    case 3:
        EmApceros_State4_03(em, 0);
        break;
    case 4:
        EmApceros_State4_03(em, 1);
        break;
    case 5:
        EmApceros_State4_05(em);
        break;
    case 6:
        EmApceros_State4_06(em);
        
    default:
        break;
    }
}

void EmApceros_State5_00_Hunted(Em *em, s32 variant)
{
    EmApcerosWork *w = (EmApcerosWork *)&em->work;
    f32 v[3];

    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        QuestEm_Hunted(em);
        MsgEvent_Check(0xC);
        switch (em->motionId[0]) {
        case 0x427:
        case 0x42B:
            em->step = 2;
            break;
        case 0x428:
            em->timer = 40;
            em->step = 3;
            break;
        case 0x429:
            em->timer = 30;
            em->step = 4;
            break;
        default:
            if (variant == 0) {
                em->step++;
                Em_SetMotion(em, 0x3E, 0, 0);
            } else {
                em->step = 2;
                Em_SetMotion(em, 0x3F, 0, 0);
            }
            break;
        }
        break;
    case 1:
        v[0] = -10.909091f * em->scaleX;
        v[1] = 0.0f;
        v[2] = 0.0f;
        Vec_RotateY(v, 6.2831855f * em->rotY / 65536.0f);
        em->posX += v[0];
        em->posZ += v[2];
        if (Player_PassedFrameScaled(em, 12.0f, 0) == 0) {
            em->step = 2;
            Em_SetMotion(em, 0x43, 0, 0);
        }
        break;
    case 2:
        if (variant == 0) {
            if (EmMotion_ReachedFrame(em, 0, 8.0f) == 0 && em->motion[0].blend == 0) {
                v[0] = -10.909091f * em->scaleX;
                v[1] = 0.0f;
                v[2] = 0.0f;
                Vec_RotateY(v, 6.2831855f * em->rotY / 65536.0f);
                em->posX += v[0];
                em->posZ += v[2];
            }
        } else {
            if (EmMotion_ReachedFrame(em, 0, 30.0f) == 0 && em->motion[0].blend == 0) {
                v[0] = 21.428572f * em->scaleX;
                v[1] = 0.0f;
                v[2] = 0.0f;
                Vec_RotateY(v, 6.2831855f * em->rotY / 65536.0f);
                em->posX += v[0];
                em->posZ += v[2];
            }
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 40;
            Em_SetMotion(em, 0x40, 0, 0);
        }
        break;
    case 3:
        if (--em->timer <= 0) {
            if (Player_PassedFrameScaled(em, 98.0f, 0) != 0) {
                em->step++;
                Em_SetMotion(em, 0x41, 0, 0);
            }
        }
        break;
    case 4:
        if (EmMotion_ReachedFrame(em, 0, 50.0f) != 0) {
            em->step++;
            *(s32 *) &em->motionSpeed = 0;
            if (em->type == 0xC || em->type == 0x46) {
                em->timer = 2400;
            } else {
                em->timer = 1800;
            }
            if (Quest_IsTraining() == 1) {
                EmMine_Create(em, 1);
            } else {
                EmMine_Create(em, 0);
            }
            em->fieldCollide = 0;
        }
        break;
    case 5:
        *(s32 *) &em->motionSpeed = 0;
        if (EmMine_Update(em) <= 0 || --em->timer <= 0) {
            EmMine_Remove(em);
            em->step++;
            em->timer = 110;
        }
        break;
    case 6:
        *(s32 *) &em->motionSpeed = 0;
        em->timer--;
        if (em->timer <= 99) {
            em->alpha = em->timer / 100.0f;
        }
        if (em->timer <= 0) {
            if (w->objListed == 1) {
                ObjListA70_Remove(&w->sense);
                w->objListed = 0;
            }
            em->visible = 0;
            EmApceros_ChangeState(em, 5, 3, 4);
        }
        break;
    }
}

void EmApceros_State5_02_Hunted(Em *em)
{
    EmApcerosWork *w = (EmApcerosWork *)&em->work;

    em->noHitTimer = 0xA;
    em->noPushTimer = 0xA;
    switch (em->step) {
    case 0:
        QuestEm_Hunted(em);
        MsgEvent_Check(0xC);
        em->step++;
        em->timer = 0x64;
        if (em->motionId[0] != 0x3F3) {
            Em_SetMotion(em, 0xB, 0, 0);
        }
        break;
    case 1:
        em->timer -= 1;
        em->alpha = (f32) em->timer / 100.0f;
        if (em->timer <= 0) {
            if (w->objListed == 1) {
                ObjListA70_Remove(&w->sense);
                w->objListed = 0U;
            }
            em->visible = 0;
            EmApceros_ChangeState(em, 5, 3, 4);
        }
        break;
    }
}

void EmApceros_State5_03(Em *em)
{
    switch (em->step) {
    case 0:
        if (QuestEm_CanAppear(em) == 1) {
            EmApceros_ChangeState(em, 0, 1, 4);
            break;
        }
        em->taskPhase++;
    case 1:
        break;
    }
}

void EmApceros_RunGroup5(Em *em)
{
    em->noHitTimer = 0xA;
    em->noPushTimer = 0xA;
    switch (em->state) {
    case 0:
        EmApceros_State5_00_Hunted(em, 0);
        break;
    case 1:
        EmApceros_State5_00_Hunted(em, 1);
        break;
    case 2:
        EmApceros_State5_02_Hunted(em);
        break;
    case 3:
        EmApceros_State5_03(em);
        break;
    }
}

void EmApceros_SpawnEffect02AtBone2(Em *em, f32 scale, s32 unused)
{
    f32 mtx[16];
    f32 v[3];
    f32 pos[3];
    u16 ax;
    u16 ay;

    v[0] = -55.0f * em->scaleX;
    v[1] = em->scaleY * (scale * (f32) ((Rand_Next(1) & 0x3F) - 0x20));
    v[2] = em->scaleZ * (scale * (f32) ((Rand_Next(1) & 0x7F) - 0x20));
    Mtx_Copy(mtx, (f32 *)Em_Bone(em, 2));
    Vec_RotateSelf(v, mtx);
    pos[0] = mtx[12] + v[0];
    pos[1] = mtx[13] + v[1];
    pos[2] = mtx[14] + v[2];
    v[0] = -1.0f;
    v[1] = -0.2f;
    v[2] = 0.0f;
    Vec_RotateSelf(v, mtx);
    ax = -(u16) (s32) (0.5f + (65536.0f * Math_Atan2(v[1], Math_Sqrtf(v[0] * v[0] + v[2] * v[2]))) / 6.2831855f);
    ay = (u16) (s32) (0.5f + (65536.0f * Math_Atan2(v[0], v[2])) / 6.2831855f);
    scale *= Effect_Scale();
    Effect02_SpawnAtScaled(ax, ay, 3, pos, scale);
}

void EmApceros_State6_00(Em *em)
{
    em->noPushTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        if (em->motionId[0] != 0x425) {
            Em_SetMotion(em, 0x3D, 0, 0);
        }
        em->noReact = 1;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmApceros_ChangeState(em, 6, 1, 0);
        }
        break;
    }
}

void EmApceros_State6_01(Em *em)
{
    f32 v[3];

    if (!(gScene.frame & 3)) {
        EmApceros_SpawnEffect02AtBone2(em, 0.8f, 0);
    }
    em->noPushTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        if (em->motionId[0] != 0x427) {
            Em_SetMotion(em, 0x3F, 0, 0);
        }
        em->noReact = 1;
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 30.0f) == 0 && em->motion[0].blend == 0) {
            v[0] = 21.428572f * em->scaleX;
            v[1] = 0.0f;
            v[2] = 0.0f;
            Vec_RotateY(v, 6.2831855f * em->rotY / 65536.0f);
            em->posX += v[0];
            em->posZ += v[2];
        }
        if (em->motion[0].playing == 0) {
            em->step++;
        }
        break;
    case 2:
        break;
    }
}

void EmApceros_State6_02(Em *em)
{
    u8 temp_a0_2;
    void *temp_a1;

    em->noPushTimer = 5;
    if (!(gScene.frame & 3)) {
        EmApceros_SpawnEffect02AtBone2(em, 0.6f, 0);
    }
    switch (em->step) {
    case 0:
        em->step++;
        if (em->motionId[0] != 0x428) {
            Em_SetMotion(em, 0x40, 0, 0);
        }
        em->noReact = 1;
        break;
    case 1:
        temp_a1 = *(void **)((u8 *)em + 0x898);
        if (temp_a1 != NULL) {
            temp_a0_2 = *(u8 *)((u8 *)temp_a1 + 2);
            if ((temp_a0_2 != 1) && (temp_a0_2 != 0x25) && (temp_a0_2 != 0xB) && ((u32) (temp_a0_2 - 0x29) > 1U)) {
                if (temp_a0_2 == 0x31) {
                    goto block_16;
                }
            } else {
block_16:
                if (*(u8 *)((u8 *)temp_a1 + 0x14) != 6) {
                    EmApceros_ChangeState(em, 6, 3, 0);
                }
            }
        }
        break;
    }
}

void EmApceros_State6_03(Em *em)
{
    u8 temp_v1;
    void *temp_a0;

    em->noPushTimer = 5;
    em->scriptPending = 0;
    switch (em->step) {
    case 0:
        em->step++;
        if (em->motionId[0] != 0x429) {
            Em_SetMotion(em, 0x41, 0, 0);
        }
        em->noReact = 1;
        break;
    case 1:
        if (!(gScene.frame & 3)) {
            EmApceros_SpawnEffect02AtBone2(em, 0.3f, 0);
        }
        temp_a0 = *(void **)((u8 *)em + 0x898);
        if (temp_a0 != NULL) {
            temp_v1 = *(u8 *)((u8 *)temp_a0 + 2);
            if (((temp_v1 == 1) || (temp_v1 == 0x25) || (temp_v1 == 0xB) || ((u32) (temp_v1 - 0x29) <= 1U) || (temp_v1 == 0x31)) && (*(u8 *)((u8 *)temp_a0 + 0x14) != 6)) {
                em->motionSpeed = 0.0f;
            }
            if (Save_TestEventFlag(0xAB) == 1) {
                em->step++;
                em->hp = 0;
                em->motionSpeed = 1.0f;
                EmApceros_ChangeState(em, 5, 0, 3);
            }
        }
        break;
    }
}

void EmApceros_State6_04(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->motionSpeed = 1.0f;
        Em_SetMotion(em, 1, 0, 0);
        break;
    case 1:
        if (Save_TestEventFlag(0xAB) == 1) {
            EmApceros_EndState(em);
        }
        break;
    }
}

void EmApceros_State6_05(Em *em)
{
    em->noPushTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        GmEm_ResetSize(em);
        em->motionSpeed = 1.0f;
        Em_SetMotion(em, 1, 0, 0);
        em->posX = 17788.5f;
        em->posY = 0.0f;
        em->posZ = 17468.3f;
        em->rotY = 0x63FF;
        break;
    case 1:
        break;
    }
}

void EmApceros_State6_06(Em *em)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        GmEm_ResetSize(em);
        em->lookAtTarget = 0;
        em->stance = 0;
        Em_SetMotion(em, 1, 0, 0);
        em->visible = 0;
        em->scriptPending = 0;
        break;
    case 1:
        em->step++;
        em->visible = 1;
        EmApceros_EndState(em);
        break;
    }
}

void EmApceros_RunGroup6(Em *em)
{
    switch (em->state) {
    case 0:
        EmApceros_State6_00(em);
        break;
    case 1:
        EmApceros_State6_01(em);
        break;
    case 2:
        EmApceros_State6_02(em);
        break;
    case 3:
        EmApceros_State6_03(em);
        break;
    case 4:
        EmApceros_State6_04(em);
        break;
    case 5:
        EmApceros_State6_05(em);
        break;
    case 6:
        EmApceros_State6_06(em);
        
    default:
        break;
    }
}

void EmApceros_State7_00(Em *em)
{

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0xD, 0, 0);
        em->timer = 0x64;
        break;
    case 1:
        em->alpha += 0.01f;
        if (!(em->alpha <= 1.0f)) {
            em->alpha = 1.0f;
        }
        if (--em->timer <= 0) {
            em->step++;
            EmApceros_EndState(em);
            em->senseDelay = 0;
            em->alpha = 1.0f;
        }
        break;
    }
}

void EmApceros_State7_01(Em *em)
{

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0xB, 0, 0);
        em->timer = 0x64;
        break;
    case 1:
        em->alpha += 0.01f;
        if (!(em->alpha <= 1.0f)) {
            em->alpha = 1.0f;
        }
        if (--em->timer <= 0) {
            em->step++;
            EmApceros_EndState(em);
            em->senseDelay = 0;
            em->alpha = 1.0f;
        }
        break;
    }
}

void EmApceros_RunGroup7(Em *em)
{
    em->noHitTimer = 0xA;
    em->noPushTimer = 0xA;
    em->senseDelay = 5;
    switch (em->state) {
    case 0:
        EmApceros_State7_00(em);
        break;
    case 1:
        EmApceros_State7_01(em);
        break;
    }
}

void EmApceros_Update(Em *em)
{
    u8 flag = 0;
    u8 c = 0;
    u8 b = 0;
    EmApcerosWork *w = (EmApcerosWork *)&em->work;
    Em *m = gEms;
    s16 i;
    u8 a = 0;
    u16 d;
    s32 r;
    u8 buf[4];

    EmApceros_CheckHerdHurt(em);
    r = Em_SenseObjects(em, &w->sense) & 0xFF;
    if (w->objListed == 0 && r != 0) {
        em->triggers |= 4;
        w->objListed = 2;
        if (em->requested != 1) {
            EmApceros_SetRequest(em, 2);
        }
    }
    if (w->objListed == 1) {
        ObjListA70_Remove(&w->sense);
        w->objListed = 2;
    }
    if (em->requested != 1 && em->stateGroup != 6 && em->stateGroup != 7) {
        for (i = 0; i < 20; i++, m++) {
            if (m->active && m->visible && (u8) Obj_SameArea(em, (Player *) m)) {
                if (gEmIsLarge2[m->type]) {
                    if (m->combat == 1) {
                        a = 1;
                        break;
                    }
                    if (m->combat == 0) {
                        b = 1;
                    }
                } else if ((m->type == 0xC || m->type == 0x46) && m->variant == 0 && m->stateGroup == 6) {
                    c = 1;
                }
            }
        }
        if (a != 0 || (b != 0 && c != 0)) {
            EmApceros_SetRequest(em, 1);
            em->scriptPending = 1;
        }
    }
    Em_UpdateCombatTimer(em);
    if (em->stateGroup != 6) {
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
            d = em->hitAngle - em->rotY;
            if (em->stateGroup != 5) {
                if (d > 0x8000) {
                    EmApceros_ChangeState(em, 5, 0, 3);
                } else {
                    EmApceros_ChangeState(em, 5, 1, 3);
                }
            }
            em->scriptPending = 0;
            break;
        case 6:
            if (em->stateGroup != 4 || em->state != 6) {
                Em_SetStatus4Timer(em);
                EmApceros_ChangeState(em, 4, 6, 3);
            }
            flag = 1;
            break;
        case 8:
            if (em->stateGroup != 0 || em->state != 8) {
                Em_SetStatus2Timer(em);
                EmApceros_ChangeState(em, 0, 8, 3);
                em->scriptPending = 0;
            }
            flag = 1;
            break;
        case 10:
            switch (em->state) {
            case 8:
                EmApceros_ChangeState(em, 0, 9, 3);
                em->scriptPending = 0;
                break;
            }
            flag = 1;
            break;
        case 13:
            flag = 1;
            break;
        case 14:
            if (em->type == 0x19) {
                break;
            }
        case 12:
            d = em->hitAngle - em->rotY;
            flag = 1;
            if ((em->type == 0xC || em->type == 0x46) && em->hp > 0.2f * em->hpMax) {
                if (d > 0x8000) {
                    EmApceros_ChangeState(em, 4, 1, 3);
                } else {
                    EmApceros_ChangeState(em, 4, 2, 3);
                }
            } else if (em->stateGroup != 4 || (em->state != 3 && em->state != 4)) {
                if (d > 0x8000) {
                    EmApceros_ChangeState(em, 4, 3, 3);
                } else {
                    EmApceros_ChangeState(em, 4, 4, 3);
                }
            }
            em->scriptPending = 0;
            break;
        case 17:
            Em_EndStatus5(em);
            break;
        }
        if (flag) {
            if (em->type == 0x46 && em->variant != 2) {
                w->alarmed = 1;
            } else if (em->requested != 1) {
                EmApceros_SetRequest(em, 2);
            }
            if (w->objListed == 0) {
                Vec_Set(w->sense.pos, em->posX, em->posY, em->posZ);
                w->sense.radius = 2000.0f;
                w->sense.area = (u8) em->area;
                w->sense.kind = 2;
                ObjListA70_Add(&w->sense);
                w->objListed = 1;
            }
        }
    }
    if (em->requested != 1 && em->stateGroup != 7 && em->stateGroup != 6 && (u8) Em_InBlockArea1(em) == 1) {
        EmApceros_SetRequest(em, 3);
        em->scriptPending = 1;
    }
    switch (gQuest.stage) {
    case 0x434:
    case 0x436:
        if (Area_DayId(em->area) == 0x21 && Save_TestEventFlag(0xAB) == 0) {
            if (Area_DayId(gScene.area) == 0x21 && gScene.eventRunning == 1 && em->stateGroup != 6) {
                if (em->variant == 0) {
                    EmApceros_ChangeState(em, 6, 5, 1);
                } else {
                    EmApceros_ChangeState(em, 6, 4, 1);
                }
            }
            break;
        }
    default:
        Em_UpdatePendingScript(em);
        break;
    }
    Em_ClearHitReactMode(em);
    EmApceros_RunState(em);
    if (em->stateRequest != 0) {
        EmApceros_RunState(em);
        em->stateRequest = 0;
    }
}

void EmApceros_RunState(Em *em)
{
    switch (em->stateGroup) {
    case 0:
        EmApceros_RunGroup0(em);
        break;
    case 1:
        EmApceros_RunGroup1(em);
        break;
    case 2:
        EmApceros_RunGroup2();
        break;
    case 3:
        EmApceros_RunGroup3(em);
        break;
    case 4:
        EmApceros_RunGroup4(em);
        break;
    case 5:
        EmApceros_RunGroup5(em);
        break;
    case 6:
        EmApceros_RunGroup6(em);
        break;
    case 7:
        EmApceros_RunGroup7(em);
        
    default:
        break;
    }
}

void EmApceros_MotionEffects(Em *em, EmWork *work)
{
    f32 pos[3];
    f32 v[3];

    if (em->motionId[0] != work->fxMotion) {
        work->fxMotion = em->motionId[0];
    }
    EmApceros_MotionSounds(em, work);
    switch (work->fxMotion) {
    case 0x3E9:
    case 0x3EA:
    case 0x3EC:
    case 0x3ED:
    case 0x3EE:
        break;
    case 0x3EF:
        if (Player_PassedFrameScaled(em, 40.0f, 0)) {
            Em_BonePos(em, 0xA, pos);
            pos[1] = em->floorY;
            Effect0D_SpawnOnGround(pos, 7, 2.0f);
            CamShake_ReqAIf(em, 1);
            EffEm11_Create(em, 0xD);
        }
        break;
    case 0x3F3:
    case 0x3F4:
        break;
    case 0x3F5:
        if (Player_PassedFrameScaled(em, 24.0f, 0)) {
            Effect0D_SpawnForEm(em, 0x17, 0x14);
        }
        if (Player_PassedFrameScaled(em, 52.0f, 0)) {
            Effect0D_SpawnForEm(em, 0x14, 0x14);
        }
        break;
    case 0x3FD:
    case 0x3FE:
    case 0x406:
        break;
    case 0x407:
        if (Player_PassedFrameScaled(em, 30.0f, 0)) {
            if (em->type == 0xC || em->type == 0x46) {
                EffEm11_Create(em, 1);
            } else {
                EffEm11_Create(em, 9);
            }
        }
        break;
    case 0x408:
    case 0x409:
    case 0x40A:
    case 0x40B:
        break;
    case 0x40C:
        if (em->type == 0x46) {
            if (Player_PassedFrameScaled(em, 32.0f, 0)) {
                EffEm11_Create(em, 0x12);
            }
            if (Player_PassedFrameScaled(em, 24.0f, 0)) {
                StageObj_K14_Create(em, 2, 0x2C, em->scaleX);
            }
            if (Player_PassedFrameScaled(em, 52.0f, 0) && Player_InCurrentArea(em)) {
                Em_BonePos(em, 7, pos);
                pos[1] = em->floorY;
                Effect0D_SpawnOnGround(pos, 4, 4.0f * em->scaleX);
            }
        } else if (Player_PassedFrameScaled(em, 20.0f, 0)) {
            EffEm11_Create(em, 2);
        }
        break;
    case 0x40D:
        if (Player_PassedFrameScaled(em, 24.0f, 0)) {
            if (em->type == 0xC || em->type == 0x46) {
                EffEm11_Create(em, 3);
            } else {
                EffEm11_Create(em, 0xA);
            }
        }
        break;
    case 0x425:
    case 0x426:
        break;
    case 0x427:
        if (Player_PassedFrameScaled(em, 28.0f, 0)) {
            StageObj_K14_Create(em, 0, 0, 0.4f * em->scaleX);
        }
        break;
    case 0x428:
        if (Player_PassedFrameScaled(em, 46.0f, 0)) {
            Effect0D_SpawnForEm(em, 0, 6);
        }
        if (Player_PassedFrameScaled(em, 88.0f, 0)) {
            Effect0D_SpawnForEm(em, 0, 6);
        }
        if (Player_PassedFrameScaled(em, 190.0f, 0)) {
            Effect0D_SpawnForEm(em, 0, 6);
        }
        break;
    case 0x429:
    case 0x42A:
    case 0x42B:
        break;
    case 0x42C:
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = 20.0f;
        Em_SpawnEffect06Periodic(em, 0xF, v, 1.2f);
        break;
    case 0x42D:
        break;
    }
}

void EmApceros_UpdateMotionEffects(Em *em)
{
    EmApcerosWork *w = (EmApcerosWork *)&em->work;

    switch (w->fxStep) {
    case 0:
        w->fxStep++;
        break;
    case 1:
        EmApceros_MotionEffects(em, (EmWork *) w);
        break;
    }
}

void EmApceros_SpawnEffect01(u8 *em)
{
    Effect01_Spawn((Player *) em, 0);
}

void EmApceros_TableNop(void)
{
}

s32 EmApceros_ScriptQuery(Em *em, u8 mode)
{
    EmApcerosWork *w = (EmApcerosWork *)&em->work;

    switch (mode) {
    case 0:
        switch (Area_DayId(em->area)) {
        default:
            return 0;
        case 0x22:
        case 0x24:
        case 0x25:
        case 0x27:
        case 0x5D:
        case 0x5E:
        case 0x5F:
        case 0x60:
        case 0x61:
        case 0x63:
        case 0x64:
        case 0x6F:
        case 0x71:
        case 0x75:
        case 0x76:
        case 0x77:
        case 0x78:
        case 0x7A:
        case 0x98:
        case 0x99:
        case 0x9A:
        case 0x9B:
        case 0x9E:
            return 1;
        case 0x21:
            return 2;
        case 0x70:
            return em->spawnParam == 0 ? 1 : 3;
        }
    case 1:
        return w->alarmed;
    }
    return 0;
}

void EmDiablos_MotionSounds(Em *em, EmWork *work)
{
    s16 motion;

    motion = work->sndMotion;
    switch (motion) {
    case 0x3E9:
        break;
    case 0x3EB:
        if (em->stance != 4) {
            Em_PlaySeAtBoneOnFrame(em, 0x34, 1, 0x14);
            Em_PlaySeAtBoneOnFrame(em, 0x74, 1, 0x1A);
            break;
        }
        break;
    case 0x3ED:
        if (em->stance != 4) {
            Em_PlaySeAtBoneOnFrame(em, 0x19, 1, 0x14);
            Em_PlaySeAtBoneOnFrame(em, 0x38, 1, 0x1A);
            break;
        }
        break;
    case 0x3EE:
        if (em->stance != 4) {
            Em_PlaySeAtBoneOnFrame(em, 0x19, 1, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x38, 1, 0x14);
            break;
        }
        break;
    case 0x3EF:
        Em_PlaySe6AtBoneOnFrame(em, 6, 0x48, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 1, 0x14);
        break;
    case 0x3F0:
        Em_PlaySe6AtBoneOnFrame(em, 6, 0x48, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 1, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 1, 0x1A);
        break;
    case 0x3F2:
        Em_PlaySe6AtBoneOnFrame(em, 6, 0x53, 0x23);
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
        break;
    case 0x3F5:
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0xF, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0xF, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0xF, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 0xF, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x42, 0xF, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0xF, 0xC);
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
        if (em->stance != 4) {
            Em_PlaySeAtBoneOnFrame(em, 4, 0x27, 0x23);
            Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0);
            Em_PlaySeAtBoneOnFrame(em, 4, 0xB, 6);
            Em_PlaySeAtBoneOnFrame(em, 8, 0xB, 0xC);
            Em_PlaySeAtBoneOnFrame(em, 0x1A, 1, 0x14);
            Em_PlaySeAtBoneOnFrame(em, 0x62, 0x1E, 0x14);
            Em_PlaySeAtBoneOnFrame(em, 0x78, 6, 0x14);
            Em_PlaySeAtBoneOnFrame(em, 0x80, 0x38, 0x14);
            Em_PlaySeAtBoneOnFrame(em, 0x90, 6, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x96, 0x38, 0x1A);
            Em_PlaySeAtBoneVolOnFrame(em, 0x78, 0x65, 0x14, 8);
            Em_PlaySeAtBoneVolOnFrame(em, 0x90, 0x65, 0x1A, 8);
            break;
        }
        break;
    case 0x3FA:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x23, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 4, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 8, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0xF, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0xF, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x8A, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x8E, 0xB, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0xBE, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0xC2, 0xB, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0xEE, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0xF2, 0xB, 0xC);
        Em_PlaySeAtBoneVolOnFrame(em, 0x48, 0x66, 0, 8);
        break;
    case 0x3FB:
        Em_PlaySeAtBoneOnFrame(em, 2, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 6, 0xB, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 4, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 4, 1, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 9, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x74, 0, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x96, 1, 0x14);
        Em_PlaySeAtBoneVolOnFrame(em, 0xE, 0x66, 0, 8);
        break;
    case 0x405:
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0x1F, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0xF, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 8, 0xF, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x78, 0xE, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x7C, 0xE, 6);
        break;
    case 0x407:
        Em_PlaySe6AtBoneOnFrame(em, 2, 0x55, 0x23);
        Em_PlaySe6AtBoneOnFrame(em, 0xB6, 0x55, 0x23);
        Em_PlaySe6AtBoneOnFrame(em, 0x16C, 0x55, 0x23);
        break;
    case 0x408:
        Em_PlaySe6AtBoneOnFrame(em, 4, 0x57, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 0x1F, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x8E, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x17, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x13, 0);
        break;
    case 0x409:
        Em_PlaySe6AtBoneOnFrame(em, 4, 0x57, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0x2D, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x7E, 3, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0xB6, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0x16, 0);
        break;
    case 0x40A:
        Em_PlaySe6AtBoneOnFrame(em, 0x24, 0x57, 0x23);
        break;
    case 0x40C:
        Em_PlaySe6AtBoneOnFrame(em, 0x24, 0x4C, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 0, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0x15, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0xC, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0xC, 6);
        Em_PlaySeAtBoneOnFrame(em, 0xA6, 0, 0x1A);
        break;
    case 0x40D:
        Em_PlaySe6AtBoneOnFrame(em, 0x1E, 0x48, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0xC, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 2, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 1, 0x1A);
        break;
    case 0x40E:
        Em_PlaySe6AtBoneOnFrame(em, 4, 0x57, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 0xA4, 9, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x90, 3, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x88, 0xE, 6);
        Em_PlaySeAtBoneOnFrame(em, 0xB2, 4, 0x22);
        break;
    case 0x40F:
        Em_PlaySe6AtBoneOnFrame(em, 2, 0x57, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 2, 0x17, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 3, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x66, 0x10, 0x1A);
        break;
    case 0x412:
        Em_PlaySe6AtBoneOnFrame(em, 8, 0x4B, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0xF, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x14, 0x2A);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 1, 0x14);
        break;
    case 0x41B:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x13, 0x2A);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 0x10, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0x12, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 6, 0x1A);
        Em_PlaySe6AtBoneOnFrame(em, 0x96, 0x56, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xEE, 0x16, 0);
        Em_PlaySeAtBoneVolOnFrame(em, 0x58, 0x65, 0x1A, 8);
        break;
    case 0x41E:
        Em_PlaySeAtBoneOnFrame(em, 2, 0x12, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 4, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 4, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 5, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x78, 4, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x108, 4, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x14, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 6, 0x23, 0x23);
        Em_PlaySe6AtBoneOnFrame(em, 0x4E, 0x56, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0xC, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 0xC, 6);
        break;
    case 0x424:
        Em_PlaySe6AtBoneOnFrame(em, 6, 0x53, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 0x13, 0x2A);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 6, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0x11, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x42, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x74, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x90, 3, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 0xC, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0xD, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0xC2, 3, 0x1A);
        Em_PlaySeAtBoneVolOnFrame(em, 0x30, 0x65, 0x1A, 8);
        break;
    case 0x425:
        Em_PlaySe6AtBoneOnFrame(em, 6, 0x53, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x12, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 6, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 4, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x6E, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x5A, 0x15, 0);
        Em_PlaySeAtBoneOnFrame(em, 0xAA, 3, 0x1A);
        Em_PlaySeAtBoneVolOnFrame(em, 0x24, 0x65, 0x1A, 8);
        break;
    case 0x426:
        Em_PlaySe6AtBoneOnFrame(em, 4, 0x4F, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x56, 0x1F, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 4, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 8, 0xD, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x62, 0x13, 0x2A);
        Em_PlaySeAtBoneOnFrame(em, 0x7E, 3, 0x1A);
        break;
    case 0x427:
        Em_PlaySe6AtBoneOnFrame(em, 4, 0x52, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 0x1F, 0x23);
        break;
    case 0x428:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x12, 0x1A);
        Em_PlaySe6AtBoneOnFrame(em, 4, 0x52, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x7A, 3, 0x1A);
        break;
    case 0x429:
        Em_PlaySe6AtBoneInFrames(em, 4, 0xF4, 0x54, 0x23, 6, 0);
        Em_PlaySeAtBoneOnFrame(em, 0xA8, 0x2A, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xFC, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x130, 0x27, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x42, 4, 0x2C);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0x12, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 6, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 0x10, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0xD2, 3, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x142, 3, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 9, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 7, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 0x12, 0);
        Em_PlaySeAtBoneOnFrame(em, 0xA2, 0x12, 0);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0xC, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0xC, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x11E, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x122, 0xD, 0xC);
        Em_PlaySeAtBoneVolOnFrame(em, 0x2C, 0x65, 0x1A, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x4A, 0x66, 0, 8);
        break;
    case 0x42A:
        Em_PlaySe6AtBoneOnFrame(em, 4, 0x52, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x13, 0);
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
        Em_PlaySeAtBoneOnFrame(em, 0x108, 9, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x108, 7, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x114, 0x17, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x180, 0x16, 0);
        Em_PlaySe6AtBoneOnFrame(em, 0x18, 0x52, 0x23);
        Em_PlaySe6AtBoneInFrames(em, 0x8C, 0x17C, 0x2C, 0x23, 6, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x192, 0x2B, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x174, 0xF, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x176, 0xE, 0xC);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1A, 0x66, 0, 8);
        break;
    case 0x42D:
        Em_PlaySe6AtBoneOnFrame(em, 4, 0x52, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x13, 0x2A);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 9, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0x11, 0);
        break;
    case 0x42E:
    case 0x433:
        Em_PlaySeAtBoneOnFrame(em, 8, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x1F, 0x23);
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
    case 0x431:
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
    case 0x432:
        Em_PlaySe6AtBoneOnFrame(em, 4, 0x52, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x13, 0x2A);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 9, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0x11, 0);
        Em_PlaySeAtBoneVolOnFrame(em, 0x18, 0x66, 0, 8);
        break;
    case 0x434:
        Em_PlaySe6AtBoneOnFrame(em, 0x10, 0x5A, 0x23);
        Em_PlaySe6AtBoneOnFrame(em, 0x6E, 0x5F, 0x23);
        break;
    case 0x435:
        Em_PlaySe6AtBoneOnFrame(em, 4, 0x53, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0xB, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 0x14, 0);
        break;
    case 0x436:
        Em_PlaySeAtBoneOnFrame(em, 0xA, 9, 0);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 7, 0x2A);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x11, 0);
        Em_PlaySe6AtBoneOnFrame(em, 0x10, 0x53, 0x23);
        Em_PlaySeAtBoneVolOnFrame(em, 0xA, 0x66, 0, 8);
        break;
    case 0x438:
        Em_PlaySeAtBoneOnFrame(em, 2, 0x16, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 0x17, 0x22);
        Em_PlaySe6AtBoneOnFrame(em, 0x26, 0x55, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xC8, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 3, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x140, 3, 0x1A);
        break;
    case 0x439:
        Em_PlaySeAtBoneOnFrame(em, 2, 0x16, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 0x17, 0x22);
        Em_PlaySe6AtBoneOnFrame(em, 0x26, 0x55, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xC8, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x140, 3, 0x14);
        break;
    case 0x43A:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x37, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0);
        break;
    case 0x43B:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x17, 0);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x10, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 0, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 0, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x80, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0xC6, 0, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x2B, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xA8, 0x27, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xA2, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 0xA6, 0xD, 0xC);
        break;
    case 0x442:
        Em_PlaySeAtBoneOnFrame(em, 0x20, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0xD, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 0xD, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x7C, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x80, 0xB, 0xC);
        break;
    case 0x448:
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0x12, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 0x12, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0xC, 6);
        Em_PlaySeAtBoneOnFrame(em, 8, 0xC, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0x13, 0);
        Em_PlaySe6AtBoneOnFrame(em, 4, 0x50, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0);
        break;
    case 0x449:
        Em_PlaySeAtBoneOnFrame(em, 0x22, 0x16, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 0x16, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x62, 4, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 3, 0x22);
        Em_PlaySe6AtBoneOnFrame(em, 4, 0x52, 0x23);
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
        Em_PlaySeAtBoneOnFrame(em, 0x14, 6, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0x10, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x11, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x8E, 0, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x9A, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x68, 0x16, 0);
        Em_PlaySeAtBoneVolOnFrame(em, 0x14, 0x65, 0x1A, 8);
        break;
    case 0x44F:
        Em_PlaySeAtBoneOnFrame(em, 0x10, 0x11, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 0x10, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0x1D, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x1D, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 0x1D, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x68, 0x1E, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x82, 0x16, 0);
        break;
    case 0x450:
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x1D, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0x1E, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0x2E, 0x23);
        Em_PlaySe6AtBoneOnFrame(em, 0xA8, 0x48, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 8, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 6, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 2, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0xB2, 1, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0xE8, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0xAE, 0xC, 6);
        Em_PlaySeAtBoneOnFrame(em, 0xB2, 0xC, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x15, 0x22);
        Em_PlaySeAtBoneVolOnFrame(em, 0xE, 0x66, 0, 8);
        break;
    case 0x451:
        Em_PlaySeAtBoneOnFrame(em, 6, 7, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 6, 0x1D, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 0xC, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 0xC, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x36, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x15, 0x22);
        Em_PlaySeAtBoneVolOnFrame(em, 8, 0x66, 0, 8);
        break;
    case 0x452:
        Em_PlaySeAtBoneOnFrame(em, 8, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x8A, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0x15, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x30, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x94, 0, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0xB4, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0x1F, 0x23);
        Em_PlaySeAtBoneVolOnFrame(em, 8, 0x65, 0x1A, 8);
        break;
    case 0x453:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x34, 0x23);
        Em_PlaySe6AtBoneOnFrame(em, 0x32, 0x5D, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x15, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 6, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0xA8, 0, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0xD8, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x74, 0xC, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x78, 0xC, 0xC);
        Em_PlaySeAtBoneVolOnFrame(em, 0xA, 0x65, 0x1A, 8);
        break;
    case 0x454:
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0x14, 0x2A);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x15, 0x2C);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x24, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0x14, 0x2A);
        Em_PlaySeAtBoneOnFrame(em, 0x5C, 0x15, 0x2C);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 0x1E, 0x2C);
        Em_PlaySeAtBoneOnFrame(em, 0x66, 0x1E, 0x2C);
        Em_PlaySeAtBoneOnFrame(em, 0xC6, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0xF2, 0, 0x14);
        Em_PlaySeAtBoneVolOnFrame(em, 0x34, 0x65, 0x2C, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x66, 0x65, 0x2C, 8);
        break;
    case 0x455:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x12, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x35, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 3, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x70, 4, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0xD8, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x108, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0x1D, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0x14, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x54, 0x16, 0);
        Em_PlaySeAtBoneVolOnFrame(em, 0x3A, 0x66, 0, 8);
        break;
    case 0x457:
        Em_PlaySeAtBoneOnFrame(em, 0xE, 2, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x13E, 4, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x170, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x1F, 0x23);
        Em_PlaySe6AtBoneInFrames(em, 0x64, 0x140, 0x5B, 0x23, 8, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x130, 0x16, 0);
        break;
    case 0x459:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0);
        Em_PlaySe6AtBoneOnFrame(em, 4, 0x5E, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x10A, 0x2B, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x11, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x36, 4, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x86, 0, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0xDC, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x128, 3, 0x1A);
        break;
    case 0x45A:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x37, 0x23);
        Em_PlaySe6AtBoneOnFrame(em, 0x66, 0x5C, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 6, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0xF, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 0xF, 0xC);
        Em_PlaySeAtBoneVolOnFrame(em, 0x22, 0x65, 0x2C, 8);
        break;
    case 0x45B:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x37, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x5C, 0x28, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x52, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x56, 0xD, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 0x1D, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 6, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x8E, 5, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0xA6, 0, 0x1A);
        Em_PlaySeAtBoneVolOnFrame(em, 0x56, 0x66, 0x22, 8);
        break;
    }
}

/* em_plesioth's .data (0x8C6070-0x8C7340) */
EmTurnParams EmPlesioth_TurnParams[3] = {
#include "game/008C6070.inc"
};
u32 EmPlesioth_FinKeys2_0[8] = {
#include "game/008C60D0.inc"
};
u32 EmPlesioth_FinKeys2_1[8] = {
#include "game/008C60F0.inc"
};
u32 EmPlesioth_FinKeys2_2[8] = {
#include "game/008C6110.inc"
};
u32 EmPlesioth_FinKeys2_3[8] = {
#include "game/008C6130.inc"
};
EmFinKey *EmPlesioth_FinKeys2[4] = {
    (EmFinKey *)EmPlesioth_FinKeys2_0, (EmFinKey *)EmPlesioth_FinKeys2_1, (EmFinKey *)EmPlesioth_FinKeys2_2, (EmFinKey *)EmPlesioth_FinKeys2_3,
};
u32 EmPlesioth_FinKeys1_0[12] = {
#include "game/008C6160.inc"
};
u32 EmPlesioth_FinKeys1_1[12] = {
#include "game/008C6190.inc"
};
u32 EmPlesioth_FinKeys1_2[8] = {
#include "game/008C61C0.inc"
};
u32 EmPlesioth_FinKeys1_3[8] = {
#include "game/008C61E0.inc"
};
EmFinKey *EmPlesioth_FinKeys1[4] = {
    (EmFinKey *)EmPlesioth_FinKeys1_0, (EmFinKey *)EmPlesioth_FinKeys1_1, (EmFinKey *)EmPlesioth_FinKeys1_2, (EmFinKey *)EmPlesioth_FinKeys1_3,
};
s16 EmPlesioth_FinWait1[4] = {
#include "game/008C6210.inc"
};
s16 EmPlesioth_FinWait2[4] = {
#include "game/008C6218.inc"
};
s16 EmPlesioth_FinRewait1[4] = {
#include "game/008C6220.inc"
};
s16 EmPlesioth_FinRewait2[4] = {
#include "game/008C6228.inc"
};
s32 func_0090A9C0(void); /* gm_sub */
u16 EmPlesioth_FinFoldTarget[8] = {
#include "game/008C6230.inc"
};
void *EmPlesioth_Callbacks[8] = {
    EmPlesioth_SpawnEffects, EmPlesioth_SpawnEffects, EmPlesioth_TableNop, EmPlesioth_UpdateMotionEffects,
    func_0090A9C0, EmPlesioth_Reinit, NULL, NULL,
};
EmCurveKey EmPlesioth_VelYCurves_0[16] = {
#include "game/008C6260.inc"
};
EmCurveKey EmPlesioth_VelYCurves_1[16] = {
#include "game/008C62E0.inc"
};
EmCurveKey EmPlesioth_VelYCurves_2[12] = {
#include "game/008C6360.inc"
};
extern EmCurveKey EmCephadrome_VelYCurves_3[];
EmCurveKey *EmPlesioth_VelYCurves[4] = {
    EmPlesioth_VelYCurves_0, EmPlesioth_VelYCurves_1, EmPlesioth_VelYCurves_2, EmCephadrome_VelYCurves_3,
};
EmCurveKey EmPlesioth_SpeedCurves_0[16] = {
#include "game/008C63D0.inc"
};
EmCurveKey EmPlesioth_SpeedCurves_1[16] = {
#include "game/008C6450.inc"
};
EmCurveKey EmPlesioth_SpeedCurves_2[11] = {
#include "game/008C64D0.inc"
};
EmCurveKey *EmPlesioth_SpeedCurves[3] = {
    EmPlesioth_SpeedCurves_0, EmPlesioth_SpeedCurves_1, EmPlesioth_SpeedCurves_2,
};
u16 EmKind0A_TradeArea06V0[168] = {
#include "game/008C6540.inc"
};
u16 EmKind0A_TradeArea06V1[168] = {
#include "game/008C6690.inc"
};
u16 EmKind0A_TradeArea02V0[160] = {
#include "game/008C67E0.inc"
};
u16 EmKind0A_TradeArea02V1[160] = {
#include "game/008C6920.inc"
};
u16 EmKind0A_TradeArea04V0[152] = {
#include "game/008C6A60.inc"
};
u16 EmKind0A_TradeArea04V1[152] = {
#include "game/008C6B90.inc"
};
u16 EmKind0A_TradeArea03V0[152] = {
#include "game/008C6CC0.inc"
};
u16 EmKind0A_TradeArea03V1[152] = {
#include "game/008C6DF0.inc"
};
u16 EmKind0A_TradeArea11V0[168] = {
#include "game/008C6F20.inc"
};
u16 EmKind0A_TradeArea11V1[168] = {
#include "game/008C7070.inc"
};
u16 EmKind0A_GiftItemsV0[40] = {
#include "game/008C71C0.inc"
};
u16 EmKind0A_GiftItemsV1[40] = {
#include "game/008C7210.inc"
};
EmTradeRate EmKind0A_TradeRates[12] = {
#include "game/008C7260.inc"
};
void *EmKind0A_Callbacks[8] = {
    EmKind0A_SpawnEffect01, EmKind0A_SpawnEffect01, EmKind0A_TableNop, EmKind0A_UpdateMotionEffects,
    func_0090A9C0, EmKind0A_TableNop, NULL, NULL,
};
f32 EmApceros_ScalesType0C[8] = {
#include "game/008C72B0.inc"
};
f32 EmApceros_ScalesType46[12] = {
#include "game/008C72D0.inc"
};
EmTurnParams EmApceros_TurnParams[1] = {
#include "game/008C7300.inc"
};
void *EmApceros_Callbacks[8] = {
    EmApceros_SpawnEffect01, EmApceros_SpawnEffect01, EmApceros_TableNop, EmApceros_UpdateMotionEffects,
    EmApceros_ScriptQuery, EmApceros_Reinit, NULL, NULL,
};
