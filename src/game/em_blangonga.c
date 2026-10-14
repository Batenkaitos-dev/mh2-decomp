/*
 * Monster family: Blangonga, Congalala and Rajang (kinds 0x33, 0x34, 0x35).
 * The code from the family's setup function (0x80CDF0, called by the per-kind
 * dispatch Em_TypeSetup) to the next family's. The state functions take
 * the monster and the work area its state dispatcher passes on (EmWork);
 * Em.step is the step within the current state. See include/em.h.
 */
#include "common.h"
#include "scene.h"
#include "quest.h"
#include "em.h"
#include "player.h"
/* prototypes of callees (round 13) */
s32 EmBlangonga_RageTimerFull(Em *em);

/* prototypes */
void EmBlangonga_ChangeState(Em *em, u16 group, u16 state, u8 mode);
s8 EmMine_Create(Em *em, s32 idx);
s8 EmMine_CreateTimed(Em *em, f32 *pos, s32 idx);
s8 EmMine_CreateTimed0(Em *em, f32 *pos);
void EmMine_Remove(Em *em);
s32 EmMine_Update(Em *em);
/* unit's own declaration: with the definition's void (Em *em, s16 n) (game/unk_708850.c), EmBlangonga_State0_0B: 35 of 75 words differ */
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
/* unit's own declaration: with the definition's void (Em *em, s32 bone, s32 a, s32 b, s32 c, s32 d) (game/unk_6CADF0.c), EmBlangonga_State3_07: 30 of 89 words differ */
void EffEm08_CreateAtBone(Em *, s32, s32, s32, u16, s32);
void EffBlangonga27_Create(Em *em, s32 kind);
/* unit's own declaration: with the definition's void *(Em *em, s32 kind, s32 c, s32 a, s32 b) (game/unk_6E3B00.c), EmBlangonga_State3_08: 24 of 73 words differ */
void * EffEm33_Create(Em *, s16, s16, s32, s32);
void Em_SetRow638(Em *em, u8 idx);
void Em_MoveRotatedAccel(Em *em, void *ang);
/* unit's own declaration: with the definition's void (Em *em, s32 group, s32 state, u8 mode) (game/unk_701810.c), EmBlangonga_ChangeState: 3 of 671 words differ, EmKushala_ChangeState: 27 of 217 words differ ... */
void Em_SetStateSync(Em *em, u16 group, u16 state, u8 mode);
void Em_CheckRageByHp(Em *em, s16 n);
void Em_LoadReactionSet(Em *em, u8 k);
void Em_ClearLowHpNeed(Em *em);
void Em_Clear7ECState(Em *em);
void Em_SpawnEffect06Bone(Em *em, s32 a);
void Em_Add7EC(Em *em, s32 n);
void Em_SetFlag6AE(Em *em, u8 a);
void Em_SetGoal12(Em *em);
/* unit's own declaration: with the definition's s32 (Em *em, s16 n, s16 max, s16 interval) (game/unk_708850.c), EmBlangonga_State0_03: 82 of 132 words differ */
s32 Em_RegenHp(Em *em, s32 n, s32 max, s32 interval);
void Em_EndRage(Em *em);
void Em_ClearPoison(Em *em);
void Em_SetHold2(Em *em);
void Em_SetCaptured(Em *em, s32 a);
void Em_PlaySe6AtBoneInFrames(Em *em, s32 frame, s32 frame2, s32 se, s32 bone, s32 vol, s32 a);
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
s32 Em_ComputeSize(Em *em, s16 idx);
void Em_InitParts(Em *em);
s32 Quest_IsVersus(void);
s32 Versus_TeamHasFlag(Em *em, s32 kind);
void EmBlangonga_ClearPartFlag(Em *em, u16 part);
f32 Math_Absf(f32 x);
f32 Math_Sin(f32 x);
void EmBlangonga_SpawnPeriodicEffect06(Em *em, s32 kind);
/* unit's own declaration: with the definition's void (Em *em, s32 bone, f32 *pos, f32 scale) (game/unk_7077A0.c), EmBlangonga_SpawnPeriodicEffect06: 7 of 32 words differ */
void Em_SpawnEffect06Periodic(Em *em, u8 bone, f32 *pos, f32 scale);
void Em_ApplyPartBreaks(Em *em);
void GmEm_ResetSize(Em *em);
EmBone *Em_Bone(void *obj, s16 bone);
s32 Em_PlaySeChecked(Em *em, s32 id, s32 a, void *pos, s32 b, s32 c);
typedef struct { f32 x, y, z; } EmBVec;
extern EmBVec EmShenGaoren_SeOffsets[];
typedef struct { u16 weight; u16 value; } EmBPick;
u16 EmBlangonga_PickWeightedPart(EmBPick *p, u16 r);
extern EmBPick D_008C9E08[];
extern EmBPick D_008C9E18[];
/* the turn parameters (D_008C9DD0, D_008C9DEC) and the two part-pick tables after them: one object */
typedef struct { EmTurnParams turn[2]; EmBPick pick0[4]; EmBPick pick1[4]; } EmBlangongaTurnData;
extern EmBVec EmBlangonga_CarveThrowPt __attribute__((aligned(8)));
void StageObj_K25_Create(Player *p, Em *em, u8 k, s32 unused, f32 *pos);
void Versus_RouteEmByTeam(Em *em);
s32 Versus_DefeatCamera(Em *pl);
void Em_FlagPartBreak(Em *em, s32 arg1);
void EmBlangonga_StartCurves(Em *em, u8 kind);
void EmBlangonga_RunGroup0(Em *em, EmWork *work);
void EmBlangonga_State1_00(Em *, s32, s32);
void EmBlangonga_State1_0B(Em *, s32, s32);
void EmBlangonga_State2_00(Em *, EmWork *, u8);
void EmBlangonga_State2_01(Em *, EmWork *, s32);
void EmBlangonga_State2_04(Em *, EmWork *, s32);
void EmBlangonga_State2_05(Em *);
void EmBlangonga_State2_08(Em *, EmWork *);
void EmBlangonga_State2_09(Em *, EmWork *);
void EmBlangonga_State2_0B(Em *, EmWork *);
void EmBlangonga_RunGroup3(Em *em, EmWork *work);
void EmBlangonga_RunGroup4(Em *em, EmWork *work);
void EmBlangonga_State5_00_Hunted(Em *, s32);
void EmBlangonga_State5_01_Hunted(Em *, s32);
void EmBlangonga_State5_03_HuntedFalling(Em *, s32);
void EmBlangonga_State5_04(Em *em, s32 arg1);
void EmBlangonga_RunGroup6(Em *em, EmWork *work);
void EmBlangonga_State8_00_CarveStart(Em *, EmWork *);
void EmBlangonga_State0_08(Em *em, EmWork *work, s32 variant);
void EmBlangonga_State0_0C(Em *em, EmWork *work, s32 variant);
void EmBlangonga_State3_01(Em *em, EmWork *work, s32 variant);
void EmBlangonga_State3_02(Em *em, EmWork *work, s32 variant);
void EmBlangonga_State3_07(Em *em, EmWork *work, s32 variant);
void EmBlangonga_State3_11(Em *em, EmWork *work, s32 variant);
void EmBlangonga_State3_12(Em *em, EmWork *work, s32 variant);
void EmBlangonga_State3_13(Em *em, EmWork *work, s32 variant);
void EmBlangonga_State4_02(Em *em, EmWork *work, u8 variant);
/* unit's own declaration: with the definition's void (Em *em, u8 k, s16 v, s16 t, f32 y, f32 scale) (game/unk_874B90.c), EmBlangonga_State4_02: 116 of 165 words differ, EmBlangonga_State4_09: 109 of 175 words differ */
void StageObj_Stun_CreateEm(Em *, s32, s32, s32, f32, f32);
void EmBlangonga_State4_09(Em *em, EmWork *work, u8 variant);
void EmBlangonga_State6_05(Em *em, EmWork *work, u8 variant);
void EmBlangonga_State3_08(Em *em, EmWork *work);
void EmBlangonga_State3_09(Em *em, EmWork *work);
void EmBlangonga_State3_0E(Em *em, EmWork *work);
void EmBlangonga_State6_02(Em *em, EmWork *work);
void EmBlangonga_State6_03(Em *em, EmWork *work);
void EmBlangonga_State6_04(Em *em, EmWork *work);
void EmBlangonga_MotionEffects(Em *, EmWork *);
u8 EmBlangonga_ScriptQuery(Em *, u8);
void EmShenGaoren_SetGroup1State(Em *em, u16 state, u8 mode);
void EmShenGaoren_SetGroup4State(Em *em, u16 state, u8 mode);
void EmShenGaoren_PlaySeAtOffset(Em *em, s32 se, s32 idx, s32 vol);
void StageObj_K0F_CreateOnEm(Em *em, u8 k, u8 c, f32 scale);
void StageObj_K1C_CreateOnEmB(Em *em, u8 k, u16 w, s16 v, f32 scale);
s32 Sound_RandomSe(s32 id, s16 lo, s32 alt, s16 n);
s32 Player_PassedFrameScaled(void *p, f32 frame, s32 layer);
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
void Effect01_Spawn(void *obj, s16 type);
extern EmTurnParams D_008C9DEC;
void Em_BeginStatus4(Em *em);
void Em_BeginStatus5(Em *em);
void Em_BreakPart(Em *em, s32 part);
/* unit's own declaration: Em_EndAllStatus reads a0 = em at EmKushala_SetGroup4State state 6 (left by EmKushala_SetWind(em, ...), which keeps a0) */
void Em_EndAllStatus();
void Em_SetMotionRow(Em *em, s32 row, s32 arg2, s32 arg3);
extern f32 gAreaGroundY[];
/* unit's own declaration: with the definition's void (void *obj, s16 bone, f32 *out) (sub_main/unk_3DDBA0.c), EmBlangonga_SpawnGroundEffect: 11 of 31 words differ */
void Em_BonePos(void *obj, s32 a, f32 *out);
void Effect0D_SpawnOnGround(f32 *pos, s32 a, f32 scale);
void Em_SetSpeedScaled(Em *em, u16 flags, f32 x, f32 y);
s32 EmMotion_InFrameRange(void *em, s32 layer, f32 from, f32 to);
void EmKushala_SetGroup0State(Em *em, u16 state, u8 mode);
void EmKushala_SetGroup1State(Em *em, u16 state, u8 mode);
void EmKushala_SetGroup2State(Em *em, u16 state, u8 mode);
u16 Math_AngleXZ(f32 *from, f32 *to);
f32 Vec_DistanceXZ(f32 *a, f32 *b);
void Em_ClearPart(Em *em, u16 part);
void Em_ResetNotice(Em *em);
void Em_Refill7F4(Em *em);
void Em_BeginStatus2(Em *em);
void Em_EndStatus1(Em *em);
void Em_EndStatus2(Em *em);
void Em_EndStatus4(Em *em);
void Em_EndStatus5(Em *em);
void Em_StartGoalAction(Em *em, s32 arg1);
void Em_Add7F0(Em *em, s32 n);
void Em_Clear7F0State(Em *em);
void Vec_Copy(f32 *out, f32 *in);
typedef struct EmBEntry {
    s16 id;
    s16 pad;
    f32 *pts;
} EmBEntry;
struct EmIdEntry;
struct EmIdEntry *Em_FindAreaEntry(Em *em, struct EmIdEntry *entry, s16 id);
extern f32 gStartPos[];
f32 *Mtx_FromAnglesYXZ(s32 *angles, f32 *m);
void Vec_Rotate(f32 *out, f32 *in, f32 *mtx);
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
/* unit's own declaration: with the definition's s16 (void *obj, u8 state, u8 sub) (sub_main/unk_3A1070.c), EmBlangonga_Update: 871 of 983 words differ */
s32 Player_IsState(void *obj, s32 group, s32 state);
void MinePoint_Remove(s32);
void EmKushala_SetGroup3State(Em *em, u16 state, u8 mode);
void EmKushala_SetGroup4State(Em *em, u16 state, u8 mode);
void EmKushala_ChangeState(Em *em, u16 group, u16 state, u8 mode);
extern EmTurnParams D_008C9DD0;
static s32 EmBlangonga_IsBlangonga(Em *em);
static s32 EmBlangonga_IsCongalala(Em *em);
s32 Em_IsSyncFree(Em *em, u8 mode);
u16 Area_BaseId(u16);
u16 Area_DayId(u16 id);
void ObjListA70_Remove(EmObj *obj);
void EmBlangonga_SetPartFlag(Em *em, u16 part);
u16 Quest_Random(void);
u16 Area_IdForCurrentTime(u16);
void StageObj_K1C_CreateOnEmC(Em *em, u8 k, u8 c, f32 scale);
struct StageObj *StageObj_K1C_CreateOnEm(struct Em *em, u8 k, f32 scale);
void EffEm07_CreateAt(Em *em, f32 *pos, s8 kind);
void EffEm08_CreateAtBone2(Em *em, s32 bone, s32 a, s32 b, s32 c, s32 d, s32 e);
void Vec_RotateSelf(f32 *v, f32 *m);
void Mtx_GetTrans(f32 *out, f32 *m);
void Vec_Add(f32 *, f32 *, f32 *);
s32 Area_ContainsPos(s32, f32 *);
void Em_EnterNextArea(Em *);
void Em_MoveRotated(Em *em, void *src);
void EmBlangonga_EnterNewArea(Em *);
void EmBlangonga_State3Pick(Em *em);
void StageObj_K11_CreateAtScaled(f32 *pos, s32 v, s16 k, f32 scale);
s32 Em_UpdateCombatTimer(Em *em);
void Em_RaiseTrigger80(Em *em);
void Em_HoldPosNoPlayers(Em *em, u16 a, u16 b);
u8 Em_SenseObjects(Em *em, EmObj *self);
s32 Em_ScriptFlagsFrom(Em *em, u8 mask);
void Em_CheckLowHp(Em *em, f32 ratio);
void Em_Decay7F0(Em *em);
void Em_Decay7F0Only(Em *em);
void Em_Decay7EC(Em *em);
u8 Em_UpdateDamage(Em *em, u8 *out);
void Em_SetGoal12Dist(Em *em, u8 *p);
void Em_SetStatus1Timer(Em *em);
void Em_SetStatus2Timer(Em *em);
void Em_SetStatus4Timer(Em *em);
void Em_SetStatus5Timer(Em *em);
s32 Em_IsHeld(Em *em);
void Em_ResetKindTimer(Em *em);
s32 func_00725820(Em *em, s32 arg1);
s32 Save_TestEventFlag(s32 flag);
void Em_UpdatePendingScript(Em *em);
void Em_ClearFlag6AE(Em *em);
void EmBlangonga_RunState(Em *em, s32 arg1);
void EmBlangonga_BreakParts(Em *em, s32 apply);
/* end of prototypes */

void EmBlangonga_Setup(Em *em)
{
}

void EmBlangonga_SpawnEffect01(s32 em)
{
    if (EmBlangonga_IsSmallKind((Em *) em) == 0) {
        Effect01_Spawn((Player *) em, 3);
        return;
    }
    Effect01_Spawn((Player *) em, 0);
}

void EmBlangonga_Init(Em *em)
{
    EmBlangongaWork *w = (EmBlangongaWork *)&em->work;
    u16 part;

    if (em->type == 0x33) {
        Em_SetPartScaled(em, 0);
        Em_SetPartScaled(em, 1);
        Em_SetPartScaled(em, 2);
    } else if (em->type == 0x34) {
        Em_SetPartScaled(em, 2);
        Em_SetPartScaled(em, 3);
    }
    if (gQuest.stage == 0) {
        em->posX = 2400.0f + 150.0f * (u32) em->slotNo;
        em->posY = 0.0f;
        em->posZ = 3600.0f;
    }
    Em_SetMotionRow(em, 1, 0, 0);
    em->stance = 0;
    em->hpMax = em->hp = Em_ComputeSize(em, 0);
    em->scriptPending = 1;
    em->blind = 0;
    em->unk_InitFlag = 1;
    if (EmBlangonga_IsSmallKind(em) == 1) {
        em->remote = 0;
    }
    w->rageTimer = 0;
    w->unk03A = 0;
    w->unk019 = 0;
    Em_BonePos(em, 0xF, &w->bonePos[0]);
    w->unk018 = 0;
    w->partSet[0] = 1;
    w->partSet[1] = 1;
    w->partSet[2] = 1;
    if (em->type == 0x34) {
        part = EmBlangonga_PickWeightedPart((EmBPick *) em, w->pickSeed);
        if (part != 0xFFFF) {
            EmBlangonga_ClearPartFlag(em, part);
        }
    }
    if (em->type == 0x34) {
        if (Quest_IsVersus() == 1) {
            if (Versus_TeamHasFlag(em, 3)) {
                w->unk044 = 2;
            } else if (Versus_TeamHasFlag(em, 4)) {
                w->unk044 = 1;
            } else if (Versus_TeamHasFlag(em, 5)) {
                w->unk044 = 3;
            } else {
                w->unk044 = 0;
            }
        } else {
            w->unk044 = 0;
        }
    } else {
        w->unk044 = 0;
    }
    switch (em->type) {
    case 0x33:
        w->tint[0] = 0x73;
        w->tint[1] = 0xEB;
        w->tint[2] = 0xFF;
        break;
    case 0x3D:
        w->tint[0] = 0xCD;
        w->tint[1] = 0x91;
        w->tint[2] = 0xC3;
        break;
    case 0x34:
    case 0x3E:
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
    w->unk045 = 0;
    Em_InitParts(em);
}

void EmBlangonga_Start(Em *em)
{
    EmBlangongaWork *w = (EmBlangongaWork *)&em->work;

    if (em->type == 0x34) {
        w->pickSeed = Quest_Random();
    }
    EmBlangonga_Init(em);
    if (em->type == 0x34) {
        switch (gQuest.stage) {
        case 0x3F3:
        case 0x3F4:
            if (Save_TestEventFlag(0x9B) == 0) {
                em->area = Area_IdForCurrentTime(0x71);
                em->posX = 11870.0f;
                em->posY = 0.0f;
                em->posZ = 10100.0f;
                em->rotY = 0x9000;
            }
            break;
        }
    }
    if (em->type == 0x3D && em->variant == 1) {
        EmBlangonga_ChangeState(em, 0, 0x18, 0);
        return;
    }
    EmBlangonga_ChangeState(em, 0, 1, 0);
}

void EmBlangonga_Reinit(Em *em)
{
    EmBlangonga_Init(em);
}

void EmBlangonga_ChangeState(Em *em, u16 group, u16 state, u8 mode)
{
    EmBlangongaWork *w = (EmBlangongaWork *)&em->work;
    u8 count = 0;
    f32 f;
    u16 v;

    if (w->unk019 != 0) {
        ObjListA70_Remove((EmObj *)((&w->sense)));
        w->unk019 = 0;
    }
    if (em->remote == 0) {
        Em_TargetPlayerInRange(em);
    } else if (mode == 3 || mode == 4) {
        return;
    }
    if (EmBlangonga_IsBlangonga(em) == 1) {
        f = 0.3f;
    } else if (EmBlangonga_IsCongalala(em) == 1) {
        f = 0.2f;
    } else {
        f = 0.15f;
    }
    em->hitzoneSet = 0;
    switch (group) {
    case 0:
        switch (state) {
        case 1:
            if (mode == 0) {
                count = w->unk018;
            }
            if (em->combat != 0) {
                state = 6;
            }
            break;
        case 3:
            Em_ResetNotice(em);
            break;
        case 4:
            Em_EndStatus2(em);
            break;
        case 5:
        case 19:
            Em_BeginStatus2(em);
            break;
        case 10:
            Em_EndStatus4(em);
            break;
        case 24:
            em->senseDelay = 5;
            em->stateArmor = 1;
            em->scriptPending = 0;
            break;
        case 25:
            Em_EndStatus5(em);
            break;
        case 26:
            Em_Refill7F4(em);
            break;
        case 27:
            em->holdTimer2 = 0;
            break;
        case 28:
        case 29:
        case 30:
        case 31:
        case 33:
            switch (state) {
            case 28:
                w->unk044 = 1;
                break;
            case 29:
                w->unk044 = 2;
                break;
            case 30:
                w->unk044 = 3;
                break;
            case 31:
                w->unk044 = 4;
                break;
            }
            if (em->type == 0x34) {
                EmBlangonga_SetPartFlag(em, 0);
                EmBlangonga_SetPartFlag(em, 1);
                EmBlangonga_SetPartFlag(em, 2);
            }
            break;
        case 34:
            Em_StartGoalAction(em, 10000);
            break;
        case 36:
        case 37:
            Em_BeginStatus1(em);
            break;
        case 38:
            Em_EndStatus1(em);
            break;
        case 40:
            Em_StartRage(em);
            break;
        case 41:
            Em_StartPendingRage(em);
            break;
        }
        break;
    case 1:
        switch (state) {
        case 1:
        case 14:
            if (em->hp < (s16) (em->hpMax * f)) {
                state = 2;
            }
        case 2:
        walk:
            if (em->curGoalKind == 7) {
                Em_SetSpeedScaled(em, 0, 1000.0f, 350.0f);
            } else {
                Em_SetSpeedScaled(em, 0, 1000.0f, 100.0f);
            }
            break;
        case 4:
        case 5:
        case 6:
        case 7:
            if (Em_IsSyncFree(em, mode) != 0) {
                w->unk018++;
            }
            count = w->unk018;
            break;
        case 8:
            if (em->remote == 0 && em->curGoalKind != 3) {
                EmBlangonga_ChangeState(em, 0, 1, 1);
                return;
            }
            break;
        case 11:
        case 13:
            if (em->hp < (s16) (em->hpMax * f)) {
                state = 12;
            }
        case 12:
            if (em->curGoalKind == 0) {
                em->hasGoal = 0;
            } else {
                em->hasGoal = 1;
            }
            break;
        case 15:
            if (em->hp < (s16) (em->hpMax * f)) {
                state = 2;
                goto walk;
            }
            break;
        case 0:
        case 3:
        case 9:
        case 10:
            break;
        }
        break;
    case 2:
        switch (state) {
        case 2:
            if ((EmBlangonga_ScriptQuery(em, 1) & 0xFF) == 0) {
                EmBlangonga_ChangeState(em, 0, 0x12, 1);
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
                switch (Area_BaseId(gScene.map)) {
                case 11:
                default:
                    v = Area_DayId(0x64);
                    em->destArea = v;
                    em->nextArea = v;
                    em->goalNo = v;
                    break;
                case 6:
                    v = Area_DayId(0x71);
                    em->destArea = v;
                    em->nextArea = v;
                    em->goalNo = v;
                    break;
                case 4:
                    v = Area_DayId(0xA0);
                    em->destArea = v;
                    em->nextArea = v;
                    em->goalNo = v;
                    break;
                case 5:
                    v = Area_DayId(0xA6);
                    em->destArea = v;
                    em->nextArea = v;
                    em->goalNo = v;
                    break;
                }
                em->goalKind = 3;
                Em_ComputeGoal(em, em->goal);
                EmBlangonga_ChangeState(em, 1, 8, 1);
                return;
            }
            break;
        }
        break;
    case 3:
        switch (state) {
        case 6:
            Em_SetSpeedScaled(em, 0, 1000.0f, 1100.0f);
            break;
        case 22:
            Em_SetSpeedScaled(em, 0, 1000.0f, 500.0f);
            break;
        case 23:
            if (em->curGoalKind != 1) {
                Em_SetSpeedScaled(em, 0, 1000.0f, 350.0f);
            } else {
                Em_SetSpeedScaled(em, 0, 1000.0f, -200.0f);
            }
            break;
        }
        break;
    case 4:
        switch (state) {
        case 4:
        case 9:
            Em_BeginStatus4(em);
            break;
        case 5:
            Em_EndAllStatus(em);
            break;
        case 10:
            Em_EndStatus4(em);
            break;
        case 11:
            Em_EndStatus2(em);
            break;
        case 19:
            Em_EndStatus1(em);
            break;
        case 12:
        case 13:
        case 14:
            Em_BeginStatus5(em);
            break;
        case 15:
            Em_EndStatus5(em);
            break;
        case 16:
            Em_EndAllStatus(em);
            break;
        case 17:
        case 21:
        case 22:
        case 23:
            if (em->type == 0x34) {
                EmBlangonga_SetPartFlag(em, 0);
                EmBlangonga_SetPartFlag(em, 1);
                EmBlangonga_SetPartFlag(em, 2);
            }
            break;
        case 20:
            em->hit8Count++;
            if (em->type == 0x33) {
                Em_BreakPart(em, 1);
            } else if (em->type == 0x35) {
                Em_EndRage(em);
                Em_BreakPart(em, 2);
            }
            break;
        case 24:
            Em_EndAllStatus(em);
            em->guardTimer = 2;
            break;
        case 6:
        case 7:
        case 8:
        case 18:
            break;
        }
        break;
    case 5:
        switch (state) {
        case 0:
            if (EmBlangonga_IsSmallKind(em) == 1 && em->stance == 2) {
                EmBlangonga_ChangeState(em, 5, 3, mode);
                return;
            }
            break;
        case 4:
            if (QuestEm_CanAppear(em) == 1) {
                QuestEm_Appear(em);
            }
            break;
        }
        break;
    case 8:
        if (state == 0x98) {
            Versus_MarkDefeat(em);
        }
        em->motionSpeed = 1.0f;
        break;
    case 6:
    case 7:
        break;
    }
    w->unk018 = count;
    Em_SetStateSync(em, group, state, mode);
}

static void EmBlangonga_EndState(Em *em, s32 unused1, s32 unused2)
{
    em->motionSpeed = 1.0f;
    em->stance = 0;
    em->lookAtTarget = 0;
    em->scriptPending = 1;
    EmBlangonga_ChangeState(em, 0, 1, 0);
}

static void EmBlangonga_EndState2(Em *em)
{
    em->motionSpeed = 1.0f;
    em->stance = 0;
    em->stateArmor = 1;
    em->noPushTimer = 2;
    em->lookAtTarget = 0;
    em->scriptPending = 1;
    EmBlangonga_ChangeState(em, 2, 3, 0);
}

/* p: the first argument (the caller passes em, unused here) reused as the
   loop pointer; a local gives other registers. */
u16 EmBlangonga_PickWeightedPart(EmBPick *p, u16 r)
{
    u16 sum;
    u16 acc;
    EmBPick *t;
    u16 v;

    switch (Area_BaseId(gScene.map)) {
    case 6:
        t = D_008C9E08;
        break;
    case 4:
        t = D_008C9E18;
        break;
    default:
        return 0xFFFF;
    }
    sum = 0;
    p = t;
    if (t->weight != 0xFFFF) {
        do {
            acc = p->weight;
            sum += acc;
            p++;
        } while (p->weight != 0xFFFF);
    }
    acc = 0;
    v = r % sum;
    if (t->weight != 0xFFFF) {
        do {
            acc += *(u16 *) t;
            if (acc > v) {
                return t->value;
            }
            t++;
        } while (t->weight != 0xFFFF);
    }
    return 0xFFFF;
}

void EmBlangonga_SetPartFlag(Em *em, u16 part)
{
    EmBlangongaWork *w = (EmBlangongaWork *)&em->work;

    switch (part) {
    case 0:
        w->partSet[0] = 1;
        break;
    case 1:
        w->partSet[1] = 1;
        break;
    case 2:
        w->partSet[2] = 1;
        break;
    }
}

void EmBlangonga_ClearPartFlag(Em *em, u16 part)
{
    EmBlangongaWork *w = (EmBlangongaWork *)&em->work;

    switch (part) {
    case 0:
        w->partSet[0] = 0;
        break;
    case 1:
        w->partSet[1] = 0;
        break;
    case 2:
        w->partSet[2] = 0;
        break;
    }
}

s32 EmBlangonga_PartFlagUnset(Em *em, u16 part)
{
    EmBlangongaWork *w = (EmBlangongaWork *)&em->work;

    switch (part) {
    case 0:
        if (w->partSet[0]) {
            return 0;
        }
        break;
    case 1:
        if (w->partSet[1]) {
            return 0;
        }
        break;
    case 2:
        if (w->partSet[2]) {
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

s32 EmBlangonga_IsSmallKind(Em *em)
{
    u8 type;

    type = em->type;
    if ((type == 0x3D) || (type == 0x3E)) {
        return 1;
    }
    return 0;
}

static s32 EmBlangonga_IsBlangonga(Em *em)
{
    u8 type;

    type = em->type;
    if ((type == 0x33) || (type == 0x3D)) {
        return 1;
    }
    return 0;
}

static s32 EmBlangonga_IsCongalala(Em *em)
{
    u8 type;

    type = em->type;
    if ((type == 0x34) || (type == 0x3E)) {
        return 1;
    }
    return 0;
}

static u64 EmBlangonga_IsRajang(Em *em)
{
    return em->type == 0x35;
}

s32 EmBlangonga_IgnoresFlash(Em *em)
{
    if (em->stateArmor == 1
        || (em->stateGroup == 0 && em->state == 0x13)
        || (em->stateGroup == 0 && em->state == 0x24)
        || (em->stateGroup == 4 && em->state == 5)
        || (em->stateGroup == 4 && em->state == 6)
        || (em->stateGroup == 4 && em->state == 9)
        || (em->stateGroup == 4 && em->state == 0xA)
        || (em->stateGroup == 4 && em->state == 0xB)
        || (em->stateGroup == 4 && em->state == 0xE)
        || (em->stateGroup == 4 && em->state == 0xF)) {
        return 1;
    }
    return 0;
}

s32 EmBlangonga_InStateArmor(Em *em)
{
    return em->stateArmor != 0;
}

f32 EmBlangonga_ShadowFade(Em *em)
{
    f32 d;
    f32 r;

    if (em->stateGroup == 2) {
        switch (em->state) {
        case 8:
        case 9:
            d = em->posY - em->floorY;
            if (d < 2000.0f) {
                return 1.0f;
            }
            if (d < 3500.0f) {
                r = 1.0f - ((d - 2000.0f) / 1500.0f);
            } else {
                r = 0.0f;
            }
            return r;
        case 11:
            return 0.0f;
        }
    }
    return 1.0f;
}

void EmBlangonga_UpdateRageTimer(Em *em)
{
    EmBlangongaWork *w = (EmBlangongaWork *)&em->work;
    s16 lim;

    switch (em->type) {
    case 0x35:
    default:
        lim = 0xF;
        break;
    case 0x34:
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
                StageObj_K1C_CreateOnEm(em, 0x18, 1.0f);
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

s32 func_0080E400(Em *em)
{
    u8 *w = &em->work;

    if (em->stateGroup == 6 && em->state == 2) {
        return w[0x48] ? 1 : 2;
    }
    switch (em->motionId[0]) {
    case 0x421:
        if (EmMotion_InFrameRange(em, 0, 36.0f, 382.0f)) {
            return 1;
        }
        break;
    }
    return 0;
}

void EmBlangonga_UpdateTint(Em *em)
{
    u8 *w = &em->work;
    u8 col[3];
    s16 i;
    s16 d;
    u8 *c;
    u8 *p;
    u8 step;
    f32 t;
    u8 *c2;
    u8 *p2;
    s16 i2;
    s16 d2;

    switch (em->type) {
    case 0x33:
    case 0x3D:
        if (em->rage != 0) {
            col[0] = 0xFF;
            col[1] = 0x64;
            col[2] = 0x5A;
        } else if (em->type == 0x33) {
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
        for (i = 0, c = col; i < 3; i++, c++) {
            p = w + i + 0x41;
            d = *c - *p;
            if (Math_Absf(d) <= 12.0f) {
                *p = *c;
            } else if (d < 0) {
                *p -= 12;
            } else {
                *p += 12;
            }
        }
        break;
    case 0x34:
        if (em->rage != 0) {
            t = 0.5f * (1.0f + Math_Sin((6.2831855f * (f32) (gScene.frame << 10)) / 65536.0f));
            col[0] = 255.0f + -100.0f * t;
            col[1] = 40.0f + 10.0f * t;
            col[2] = 10.0f + 5.0f * t;
        } else {
            col[0] = 0x99;
            col[1] = 0x80;
            col[2] = 0x73;
        }
        for (i2 = 0, c2 = col; i2 < 3; i2++, c2++) {
            if (EmBlangonga_RageTimerFull(em) == 1) {
                w[i2 + 0x41] = *c2;
            } else {
                p2 = w + i2 + 0x41;
                d2 = *c2 - *p2;
                if (em->rage != 0) {
                    step = 12;
                } else {
                    step = 6;
                }
                if (Math_Absf(d2) <= step) {
                    *p2 = *c2;
                } else if (d2 < 0) {
                    *p2 -= step;
                } else {
                    *p2 += step;
                }
            }
        }
        break;
    default:
        return;
    }
}

s32 EmBlangonga_RageTimerFull(Em *em)
{
    EmBlangongaWork *w = (EmBlangongaWork *)&em->work;
    s16 k;

    switch (em->type) {
    case 0x35:
    default:
        k = 0xF;
        break;
    case 0x34:
        if (em->rage != 0) {
            k = 0x3C;
        } else {
            k = 0x96;
        }
        break;
    }
    return w->rageTimer == k;
}

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
    Em_SpawnEffect06Periodic(em, bone, pos, 1.6f);
}

s32 EmBlangonga_ShouldEndState(Em *em)
{
    if (!((s32)Player_InCurrentArea(em) & 0xFF)) {
        return 0;
    }
    if ((em->stateArmor == 1) && !(EmBlangonga_ScriptQuery(em, 1) & 0xFF)) {
        return 1;
    }
    return 0;
}

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

/* The value of curve work[0x17] at the frame in the work area (a table of keys): sets
 * em->velY from the key pair around the frame, 2 at the table's end. */
static u8 EmBlangonga_CurveVelY(Em *em, EmWork *work)
{
    EmBlangongaWork *w = (EmBlangongaWork *) work;
    EmCurveKey *p;
    f32 frame;
    f32 t;
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

extern EmCurveKey *EmBlangonga_SpeedCurves[];

/* The value of curve work[0x17] at the frame in the work area: sets em->velZ from the key
 * pair around the frame, 4 at the table's end. */
static u8 EmBlangonga_CurveSpeed(Em *em, EmWork *work)
{
    EmBlangongaWork *w = (EmBlangongaWork *) work;
    EmCurveKey *p;
    f32 frame;
    f32 t;
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

/* Runs the family's two curves when their flags in the work area (+0x438) are set, advances
 * the curve frame by the motion's step, and turns toward the target while unk390 is 2. */
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
    if (em->stance == 2) {
        w->accelAngX = 0;
        w->accelAngY = em->rotY;
        w->accelAngZ = 0;
        Em_MoveRotatedAccel(em, &w->accelAngX);
    }
    return ret;
}

extern EmBEntry *D_00921AA4;
void EmBlangonga_EnterNewArea(Em *em)
{
    f32 *pts;
    s16 from;
    s16 to;

    em->prevArea = em->area;
    em->area = em->nextArea;
    pts = ((EmBEntry *)Em_FindAreaEntry(em, (struct EmIdEntry *)D_00921AA4, em->area))->pts;
    if (pts == NULL) {
        Vec_Copy(&em->posX, &gStartPos[em->area * 3]);
        em->rotY = 0;
    } else {
        switch (Area_DayId(em->area)) {
        case 0x5E:
        default:
        case 0x5F:
        case 0x60:
        case 0x61:
        case 0x63:
        case 0x64:
        case 0x77:
        case 0xA2:
        case 0xA5:
        case 0xA6:
        case 0xA8:
            from = 0;
            to = 1;
            break;
        case 0x6F:
            switch (Area_DayId(em->prevArea)) {
            case 0x70:
                from = 5;
                to = 4;
                break;
            default:
                from = 0;
                to = 1;
                break;
            }
            break;
        case 0x70:
            switch (Area_DayId(em->prevArea)) {
            case 0x71:
                from = 4;
                to = 3;
                break;
            default:
                from = 0;
                to = 1;
                break;
            }
            break;
        case 0x71:
            switch (Area_DayId(em->prevArea)) {
            case 0x70:
            case 0x74:
                from = 4;
                to = 3;
                break;
            default:
                from = 0;
                to = 1;
                break;
            }
            break;
        case 0x74:
            switch (Area_DayId(em->prevArea)) {
            case 0x6F:
            case 0x77:
                from = 8;
                to = 7;
                break;
            default:
                from = 0;
                to = 1;
                break;
            }
            break;
        case 0x78:
            from = 4;
            to = 3;
            break;
        case 0x99:
            from = 0;
            to = 1;
            break;
        case 0x9C:
            switch (Area_DayId(em->prevArea)) {
            case 0x9D:
                from = 5;
                to = 4;
                break;
            default:
                from = 0;
                to = 1;
                break;
            }
            break;
        case 0x9D:
            switch (Area_DayId(em->prevArea)) {
            case 0x9E:
                from = 6;
                to = 5;
                break;
            default:
                from = 0;
                to = 1;
                break;
            }
            break;
        case 0x9E:
            switch (Area_DayId(em->prevArea)) {
            case 0xA0:
                from = 2;
                to = 1;
                break;
            default:
                from = 0;
                to = 1;
                break;
            }
            break;
        case 0x9F:
            from = 0;
            to = 1;
            break;
        case 0xA0:
            switch (Area_DayId(em->prevArea)) {
            case 0x99:
                from = 4;
                to = 3;
                break;
            case 0x9F:
                from = 6;
                to = 5;
                break;
            default:
                from = 0;
                to = 1;
                break;
            }
            break;
        case 0xA7:
            switch (Area_DayId(em->prevArea)) {
            case 0xA6:
                from = 4;
                to = 3;
                break;
            case 0xA8:
            default:
                from = 0;
                to = 1;
                break;
            }
            break;
        }
        Vec_Copy(&em->posX, &pts[from * 3]);
        em->rotY = Math_AngleXZ(&em->posX, &pts[to * 3]);
    }
    em->posY += 5000.0f;
    Vec_Copy(&em->prevPosX, &em->posX);
}

void EmBlangonga_State0_00(void)
{
}

void EmBlangonga_State0_01(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 1, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->scriptPending = 1;
        }
        break;
    }
}

void EmBlangonga_State0_03(Em *em)
{
    Em_SetFlag6AE(em, 2U);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x22, 0, 0);
        em->timer = 0x1518;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x1F, 0, 0);
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
            EmBlangonga_ChangeState(em, 0, 0x1A, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmBlangonga_ChangeState(em, 0, 0x1A, 4);
        }
        break;
    }
}

void EmBlangonga_State0_04(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x20, 0, 0);
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

void EmBlangonga_State0_05(Em *em, EmWork *work, s32 variant)
{
    Em_SetFlag6AE(em, 2U);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x21, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x1F, 0, 0);
        }
        break;
    case 2:
        EmBlangonga_SpawnPeriodicEffect06(em, 0);
        if (--em->timer <= 0) {
            em->step++;
            if (!(variant & 0xFF)) {
                EmBlangonga_ChangeState(em, 0, 4, 4);
                break;
            }
            EmBlangonga_ChangeState(em, 0, 0x26, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            if (!(variant & 0xFF)) {
                EmBlangonga_ChangeState(em, 0, 4, 4);
                break;
            }
            EmBlangonga_ChangeState(em, 0, 0x26, 4);
        }
        break;
    }
}

void EmBlangonga_State0_06(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x33, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

void EmBlangonga_State0_07(Em *em, EmWork *work, s32 variant)
{
    s32 k;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x34, 0, 0);
        break;
    case 1:
        k = variant & 0xFF;
        switch (k) {
        case 0:
            break;
        case 1:
            if (((s8) em->hit8Count <= 0) && (Player_PassedFrameScaled(em, 120.0f, 0U) != 0)) {
                Vec_Set((f32 *) &work->moveAngBX, em->posX, em->posY, em->posZ);
                work->moveAngCX = 0x453B8000;
                ((EmBlangongaWork *)work)->sense.area = (s16) *(u8 *)&em->area;
                ((EmBlangongaWork *)work)->sense.kind = 3;
                ObjListA70_Add((EmObj *)&work->moveAngBX);
                work->unk019 = 1;
            }
            break;
        case 2:
            if (Player_PassedFrameScaled(em, 50.0f, 0U) != 0) {
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

void EmBlangonga_State0_08(Em *em, EmWork *work, s32 variant)
{
    u16 t = 0;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        switch (variant & 0xFF) {
        case 0:
        default:
            Em_SetMotion(em, 0x35, 0, 0);
            break;
        case 1:
            Em_SetMotion(em, 0x36, 0, 0);
            break;
        }
        break;
    case 1:
        if (EmMotion_InFrameRange(em, 0, 102.0f, 202.0f)) {
            t = -327.68f * em->motionSpeed;
        } else if (EmMotion_InFrameRange(em, 0, 402.0f, 452.0f)) {
            t = 655.36f * em->motionSpeed;
        }
        switch (variant & 0xFF) {
        case 1:
            t = -t;
            break;
        }
        em->rotY += t;
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

void EmBlangonga_State0_0A(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x3E, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

void EmBlangonga_State0_0B(Em *em)
{
    Em_SetFlag6AE(em, 3U);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x37, 0, 0);
        em->timer = 0x12C;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x38, 0, 0);
            Em_AddHp(em, (s32) (0.05f * (f32) em->hpMax));
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_Clear7ECState(em);
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
    Em_Add7EC(em, 0xDE);
}

void EmBlangonga_State0_0C(Em *em, EmWork *work, s32 variant)
{
    u16 t = 0;

    Em_SetFlag6AE(em, 3);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x39, 0, 0);
        em->timer = 0xF0;
        break;
    case 1:
        if (EmMotion_InFrameRange(em, 0, 4.0f, 26.0f)) {
            t = 1638.4f * em->motionSpeed;
        } else if (EmMotion_InFrameRange(em, 0, 370.0f, 418.0f)) {
            t = -712.34784f * em->motionSpeed;
        }
        em->rotY += t;
        if (em->motion[0].playing == 0) {
            em->step++;
            if ((variant & 0xFF) == 0) {
                Em_AddHp(em, 0.05f * em->hpMax);
            } else {
                Em_AddHp(em, 0.01f * em->hpMax);
            }
            Em_Add7F0(em, em->need1Max);
            Em_Clear7F0State(em);
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

void EmBlangonga_State0_0D(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x57, 0, 0);
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

void EmBlangonga_State0_0E(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        switch (em->type) {
        case 0x33:
        default:
        case 0x3D:
            Em_SetMotion(em, 0x16, 6, 0xE);
            break;
        case 0x34:
        case 0x3E:
            Em_SetMotion(em, 0x53, 0, 0);
            break;
        }
        em->stance = 0;
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

void EmBlangonga_State0_0F(Em *em, EmWork *work, s32 variant)
{
    f32 frame;
    s32 k;

    k = variant & 0xFF;
    if (k == 0) {
        Em_SetFlag6AE(em, 3U);
    }
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        if (k == 0) {
            Em_SetMotion(em, 0x39, 0, 0);
            break;
        }
        Em_SetMotion(em, 0x54, 0, 0);
        break;
    case 1:
        if (k == 0) {
            frame = 300.0f;
        } else {
            frame = 200.0f;
        }
        if (Player_PassedFrameScaled(em, frame, 0U) != 0) {
            em->step++;
            EmBlangonga_ChangeState(em, 0, 0x22, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmBlangonga_ChangeState(em, 0, 0x22, 4);
        }
        break;
    }
}

void EmBlangonga_State0_10(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x55, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

void EmBlangonga_State0_11(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x56, 0, 0);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 128.0f, 0U) != 0) {
            StageObj_K0F_CreateOnEm(em, 5, 1, 0.4f * em->scaleX);
            EffBlangonga27_Create(em, 0x22);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x57, 0, 0);
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

void EmBlangonga_State0_12(Em *em)
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
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

void EmBlangonga_State0_13(Em *em, EmWork *work, s32 variant)
{
    u16 motion;

    Em_SetFlag6AE(em, 2U);
    em->holdTimer = 5;
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        if ((em->type == 0x33) && (motion = em->motionId[0], (motion != 0x3FE)) && (motion != 0x432) && (motion != 0x433)) {
            Em_SetMotion(em, 0x16, 0, 0);
            em->stateArmor = 1;
            em->noPushTimer = 5;
        } else {
            em->step++;
            Em_SetMotion(em, 0x4B, 0, 0);
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
            Em_SetMotion(em, 0x4B, 0, 0);
        }
        break;
    case 2:
        if (((s32) em->timer % 135) == 0) {
            Em_PlaySeAtBone(em, 0x52, 0x13);
        }
        if (em->type == 0x34) {
            EmBlangonga_SpawnPeriodicEffect06(em, 1);
        } else {
            EmBlangonga_SpawnPeriodicEffect06(em, 0);
        }
        if (--em->timer <= 0) {
            em->step++;
            if (!(variant & 0xFF)) {
                EmBlangonga_ChangeState(em, 4, 0xB, 3);
                break;
            }
            EmBlangonga_ChangeState(em, 4, 0x13, 3);
        }
        break;
    case 3:
        if (em->type == 0x34) {
            EmBlangonga_SpawnPeriodicEffect06(em, 1);
        } else {
            EmBlangonga_SpawnPeriodicEffect06(em, 0);
        }
        if (em->remote == 0) {
            if (!(variant & 0xFF)) {
                EmBlangonga_ChangeState(em, 4, 0xB, 3);
                break;
            }
            EmBlangonga_ChangeState(em, 4, 0x13, 3);
        }
        break;
    }
}

void EmBlangonga_State0_16(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        if (em->type == 0x33) {
            Em_SetMotion(em, 0x3B, 0, 0);
            break;
        }
        Em_SetMotion(em, 0x34, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

void EmBlangonga_State0_17(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x59, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

void EmBlangonga_State0_18(Em *em)
{
    em->senseDelay = 5;
    em->noPushTimer = 5;
    em->noHitTimer = 5;
    em->stateArmor = 1;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 0;
        Em_SetMotionRow(em, 4, 0, 0);
        break;
    case 1:
        break;
    }
}

void EmBlangonga_State0_20(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x56, 0, 0);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 184.0f, 0U) != 0) {
            EmMine_CreateTimed0(em, &em->posX);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

void EmBlangonga_State0_22(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetRow638(em, 0);
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

void EmBlangonga_RunGroup0(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmBlangonga_State0_00();
        break;
    case 1:
        EmBlangonga_State0_01(em);
        break;
    case 3:
        EmBlangonga_State0_03(em);
        break;
    case 4:
        EmBlangonga_State0_04(em);
        break;
    case 5:
        EmBlangonga_State0_05(em, work, 0);
        break;
    case 6:
        EmBlangonga_State0_06(em);
        break;
    case 7:
        EmBlangonga_State0_07(em, work, 0);
        break;
    case 8:
        EmBlangonga_State0_08(em, work, 0);
        break;
    case 9:
        EmBlangonga_State0_08(em, work, 1);
        break;
    case 0xA:
        EmBlangonga_State0_0A(em);
        break;
    case 0xB:
        EmBlangonga_State0_0B(em);
        break;
    case 0xC:
        EmBlangonga_State0_0C(em, work, 0);
        break;
    case 0xD:
        EmBlangonga_State0_0D(em);
        break;
    case 0xE:
        EmBlangonga_State0_0E(em);
        break;
    case 0xF:
        EmBlangonga_State0_0F(em, work, 1);
        break;
    case 0x10:
        EmBlangonga_State0_10(em);
        break;
    case 0x11:
        EmBlangonga_State0_11(em);
        break;
    case 0x12:
        EmBlangonga_State0_12(em);
        break;
    case 0x13:
        EmBlangonga_State0_13(em, work, 0);
        break;
    case 0x14:
        EmBlangonga_State0_07(em, work, 1);
        break;
    case 0x15:
        EmBlangonga_State0_07(em, work, 2);
        break;
    case 0x16:
        EmBlangonga_State0_16(em);
        break;
    case 0x17:
        EmBlangonga_State0_17(em);
        break;
    case 0x18:
        EmBlangonga_State0_18(em);
        break;
    case 0x19:
        EmBlangonga_State0_0A(em);
        break;
    case 0x1A:
        EmBlangonga_State0_04(em);
        break;
    case 0x1B:
        EmBlangonga_State0_0A(em);
        break;
    case 0x1C:
        EmBlangonga_State0_0C(em, work, 1);
        break;
    case 0x1D:
        EmBlangonga_State0_0C(em, work, 1);
        break;
    case 0x1E:
        EmBlangonga_State0_0C(em, work, 1);
        break;
    case 0x1F:
        EmBlangonga_State0_0C(em, work, 1);
        break;
    case 0x20:
        EmBlangonga_State0_20(em);
        break;
    case 0x21:
        EmBlangonga_State0_0C(em, work, 0);
        break;
    case 0x22:
        EmBlangonga_State0_22(em);
        break;
    case 0x23:
        EmBlangonga_State0_07(em, work, 0);
        break;
    case 0x24:
        EmBlangonga_State0_13(em, work, 1);
        break;
    case 0x25:
        EmBlangonga_State0_05(em, work, 1);
        break;
    case 0x26:
        EmBlangonga_State0_04(em);
        break;
    case 0x27:
        EmBlangonga_State0_0F(em, work, 0);
        break;
    case 0x28:
        EmBlangonga_State0_07(em, work, 0);
        break;
    case 0x29:
        EmBlangonga_State0_07(em, work, 0);
        break;
    }
}

void EmBlangonga_State1_00(Em *em, s32 unused, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        switch (variant & 0xFF) {
        case 0:
        default:
            Em_StartTurnToTarget(em, &D_008C9DD0);
            break;
        case 1:
            Em_StartTurnAway(em, &D_008C9DD0);
            break;
        }
        break;
    case 1:
        if (Em_TurnStep(em, &D_008C9DD0) == 1) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

void EmBlangonga_State1_01(Em *em, s32 unused, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        switch (variant & 0xFF) {
        case 0:
        default:
            if (em->type == 0x3D) {
                em->speedBonus = 0x19;
            }
            Em_SetMotion(em, 3, 0, 0);
            break;
        case 1:
            Em_SetMotion(em, 9, 0, 0);
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

void EmBlangonga_State1_03(Em *em, s32 unused, s32 variant)
{
    s32 k;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        k = variant & 0xFF;
        em->lookAtTarget = 0;
        switch (k) {
        case 1:
            Em_AreaExitPoint(em, (f32 *) em->goal);
            Em_StartTurnToTarget(em, &D_008C9DEC);
            break;
        case 2:
            Em_StartTurnAway(em, &D_008C9DEC);
            break;
        default:
            Em_StartTurnToTarget(em, &D_008C9DEC);
        }
        break;
    case 1:
        if (Em_TurnStep(em, &D_008C9DEC) == 1) {
            if ((variant & 0xFF) == 1) {
                em->step++;
                EmBlangonga_ChangeState(em, 2, 8, 4);
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

void EmBlangonga_State1_04(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0xC, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

void EmBlangonga_State1_05(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0xE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

void EmBlangonga_State1_06(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x10, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

void EmBlangonga_State1_07(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0xA, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

void EmBlangonga_State1_0B(Em *em, s32 unused, s32 variant)
{
    switch (em->step) {
    case 0:
        em->stance = 0;
        em->lookAtTarget = 0;
        em->timer = 0x78;
        switch (variant & 0xFF) {
        case 0:
        case 3:
        default:
            em->step++;
            Em_SetMotion(em, 0x17, 0, 0);
            break;
        case 1:
            em->step = 2;
            Em_SetMotion(em, 9, 0, 0);
            break;
        case 2:
            em->step = 2;
            Em_SetMotion(em, 3, 0, 0);
            break;
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 3, 0, 0x44);
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

void EmBlangonga_RunGroup1(Em *em, s32 work)
{
    switch (em->state) {
    case 0:
        EmBlangonga_State1_00(em, work, 0);
        break;
    case 1:
        EmBlangonga_State1_01(em, work, 0);
        break;
    case 2:
        EmBlangonga_State1_01(em, work, 1);
        break;
    case 3:
        EmBlangonga_State1_03(em, work, 0);
        break;
    case 4:
        EmBlangonga_State1_04(em);
        break;
    case 5:
        EmBlangonga_State1_05(em);
        break;
    case 6:
        EmBlangonga_State1_06(em);
        break;
    case 7:
        EmBlangonga_State1_07(em);
        break;
    case 8:
        EmBlangonga_State1_03(em, work, 1);
        break;
    case 9:
        EmBlangonga_State1_00(em, work, 1);
        break;
    case 10:
        EmBlangonga_State1_03(em, work, 2);
        break;
    case 11:
        EmBlangonga_State1_0B(em, work, 0);
        break;
    case 12:
        EmBlangonga_State1_0B(em, work, 1);
        break;
    case 13:
        EmBlangonga_State1_0B(em, work, 2);
        break;
    case 14:
        EmBlangonga_State1_0B(em, work, 3);
        break;
    case 15:
        EmBlangonga_State1_07(em);
        
    default:
        break;
    }
}

void EmBlangonga_State2_00(Em *em, EmWork *work, u8 variant)
{
    f32 frame;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x11, 0, 0);
        Player_ClearVelocity(em);
        break;
    case 1:
        switch (em->type) {
        case 0x33:
        default:
        case 0x3D:
            frame = 52.0f;
            break;
        case 0x34:
        case 0x3E:
            frame = 56.0f;
            break;
        case 0x35:
            frame = 48.0f;
            break;
        }
        if (EmMotion_ReachedFrame(em, 0, frame)) {
            em->noReact = 1;
            em->stance = 2;
            em->step++;
            if (variant == 2) {
                em->accelY = -6.4f;
                em->velY = (em->goal[1] - em->posY) / 60.0f - 60.0f * (0.5f * em->accelY);
                em->velZ = Vec_DistanceXZ(em->goal, &em->posX) / 60.0f;
            } else {
                switch (em->type) {
                case 0x33:
                default:
                case 0x3D:
                case 0x35:
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
            Em_SetMotion(em, 0x12, 0, 0);
            em->step++;
        }
        break;
    case 3:
        Em_MoveRotatedAccel(em, &work->accelAngX);
        if (em->posY <= em->floorY) {
            em->posY = em->floorY;
            em->step++;
            em->stance = 0;
            CamShake_ReqAIf(em, 5);
            switch (em->type) {
            case 0x33:
            default:
            case 0x3D:
            case 0x35:
                Em_SetMotion(em, 0x14, 0, 0);
                break;
            case 0x34:
            case 0x3E:
                Em_SetMotion(em, 0x14, 8, 0);
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

void EmBlangonga_State2_01(Em *em, EmWork *work, s32 variant)
{
    if (EmBlangonga_IsCongalala(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
        return;
    }
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x30, 0, 0);
        Player_ClearVelocity(em);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 26.0f)) {
            em->step++;
            em->stance = 2;
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
        if (Player_PassedFrameScaled(em, 50.0f, 0)) {
            EffBlangonga27_Create(em, 0x21);
        }
        Em_MoveRotatedAccel(em, &work->accelAngX);
        if (em->posY <= em->floorY) {
            em->posY = em->floorY;
            em->step++;
            em->stance = 0;
            if (EmBlangonga_IsSmallKind(em) == 1) {
                CamShake_ReqAIf(em, 2);
            } else {
                CamShake_ReqAIf(em, 5);
            }
            Em_SetMotion(em, 0x31, 0, 0);
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

void EmBlangonga_State2_02(Em *em)
{
    if (EmBlangonga_IsBlangonga(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
        return;
    }
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        if (em->motionId[0] != 0x3FD) {
            Em_SetMotion(em, 0x15, 0, 0);
        }
        em->noReact = 1;
        em->stance = 0;
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
            EmBlangonga_EndState2(em);
        }
        break;
    }
    em->guardTimer = 2;
}

void EmBlangonga_State2_03(Em *em)
{
    if (EmBlangonga_IsBlangonga(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
    } else {
        em->stateArmor = 1;
        em->noPushTimer = 2;
        switch (em->step) {
        case 0:
            em->step++;
            em->stance = 0;
            em->lookAtTarget = 0;
            Em_SetMotion(em, 4, 0, 0);
            break;
        case 1:
            if ((em->remote == 0) && (em->motion[0].playing == 0)) {
                em->step++;
                EmBlangonga_EndState2(em);
            }
            break;
        }
    }
}

void EmBlangonga_State2_04(Em *em, EmWork *work, s32 variant)
{
    if (EmBlangonga_IsBlangonga(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
        return;
    }
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 0;
        if (em->motionId[0] != 0x3FE) {
            Em_SetMotion(em, 0x16, 0, 0);
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
            if (Player_PassedFrameScaled(em, 2.0f, 0)) {
                EffBlangonga27_Create(em, 6);
            }
            if (Player_PassedFrameScaled(em, 52.0f, 0)) {
                EffBlangonga27_Create(em, 7);
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

void EmBlangonga_State2_05(Em *em)
{
    u32 spd;
    u32 d;

    if (EmBlangonga_IsBlangonga(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
        return;
    }
    em->stateArmor = 1;
    em->noPushTimer = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        Em_SetMotion(em, 4, 0, 0);
        em->timer = 0x28;
        break;
    case 1:
        spd = 409.6f * em->motionSpeed;
        d = (em->aimAngle - *(u16 *) &em->rotY) & 0xFFFF;
        if (--em->timer <= 0) {
            if (((d + spd) & 0xFFFF) < spd * 2) {
                em->step++;
                EmBlangonga_EndState2(em);
            } else {
                em->timer = 0x28;
            }
        } else {
            if (((d + spd) & 0xFFFF) < spd * 2) {
                em->rotY = em->aimAngle;
            } else if (d < 0x8000) {
                em->rotY = (em->rotY + spd) & 0xFFFF;
            } else {
                em->rotY = (em->rotY - spd) & 0xFFFF;
            }
        }
        break;
    }
}

void EmBlangonga_State2_06(Em *em, EmWork *work)
{
    Player *p;

    p = &gPlayers[gScene.localPlayer];
    if (EmBlangonga_IsBlangonga(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
    } else {
        em->stateArmor = 1;
        em->noPushTimer = 2;
        switch (em->step) {
        case 0:
            em->step++;
            em->stance = 0;
            em->lookAtTarget = 0;
            Em_SetMotion(em, 4, 0, 0x64);
            Player_ClearVelocity(em);
            em->velZ = 70.0f;
            work->accelAngX = em->rotX;
            work->accelAngY = em->rotY;
            work->accelAngZ = 0;
            break;
        case 1:
            Em_MoveRotatedAccel(em, &work->accelAngX);
            Em_StepToGoal(em, 0x10, 2);
            if (!(gScene.frame & 7) && (Obj_SameArea(em, p) != 0) && (Obj_InCurrentArea(p) != 0) && (Vec_Distance((f32 *) &em->posX, (f32 *) p->pos) < 1000.0f)) {
                Vib_Start(0, 2);
            }
            if (em->goalDist <= 0.0f) {
                em->step++;
                EmBlangonga_EndState2(em);
            }
            break;
        }
    }
}

void EmBlangonga_State2_07(Em *em)
{
    em->stateArmor = 1;
    em->noPushTimer = 2;
    if (EmBlangonga_IsBlangonga(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            em->stance = 0;
            em->lookAtTarget = 0;
            Em_SetMotion(em, 4, 0, 0);
            em->timer = 0x5A;
            break;
        case 1:
            if (--em->timer <= 0) {
                em->step++;
                EmBlangonga_EndState2(em);
            }
            break;
        }
    }
}

void EmBlangonga_State2_08(Em *em, EmWork *work)
{
    f32 f;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x11, 0, 0);
        Player_ClearVelocity(em);
        break;
    case 1:
        switch (em->type) {
        case 0x33:
        default:
        case 0x3D:
            f = 52.0f;
            break;
        case 0x34:
        case 0x3E:
            f = 56.0f;
            break;
        case 0x35:
            f = 48.0f;
            break;
        }
        if (EmMotion_ReachedFrame(em, 0, f)) {
            em->step++;
            em->noReact = 1;
            em->stance = 2;
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
            if (em->nextArea == *(u16 *) &em->destArea || *(u16 *) &em->destArea == 0xFFFF) {
                EmBlangonga_EnterNewArea(em);
                EmBlangonga_ChangeState(em, 2, 9, 1);
                break;
            }
            Em_EnterNextArea(em);
            if (em->remote == 0) {
                em->goalNo = em->nextArea = *(u16 *) &em->destArea;
                em->goalKind = 3;
                Em_ComputeGoal(em, em->goal);
            }
            EmBlangonga_ChangeState(em, 2, 0xB, 1);
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

void EmBlangonga_State2_09(Em *em, EmWork *work)
{
    u16 t;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        em->noReact = 1;
        Em_SetMotionRow(em, 0x12, 0, 0);
        Player_ClearVelocity(em);
        em->velY = -30.0f;
        em->velZ = 0.0f;
        em->accelY = -4.7f;
        work->accelAngX = em->rotX;
        work->accelAngY = em->rotY;
        work->accelAngZ = 0;
        if (em->type == 0x33 || em->type == 0x35) {
            t = em->prevArea;
            Em_RestartScript(em);
            em->prevArea = t;
        }
        break;
    case 1:
        Em_MoveRotatedAccel(em, &work->accelAngX);
        if (em->posY <= em->floorY) {
            em->posY = em->floorY;
            em->step++;
            em->stance = 0;
            CamShake_ReqAIf(em, 5);
            switch (em->type) {
            case 0x33:
            default:
            case 0x3D:
            case 0x35:
                Em_SetMotion(em, 0x14, 0, 0);
                break;
            case 0x34:
            case 0x3E:
                Em_SetMotion(em, 0x14, 8, 0);
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

void EmBlangonga_State2_0B(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noPushTimer = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        em->noReact = 1;
        Em_SetMotion(em, 0x11, 0, 0x38);
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
        if (em->nextArea == (u16) em->destArea || (u16) em->destArea == 0xFFFF) {
            EmBlangonga_EnterNewArea(em);
            EmBlangonga_ChangeState(em, 2, 9, 1);
            break;
        }
        Em_EnterNextArea(em);
        if (em->remote == 0) {
            em->goalNo = em->nextArea = *(u16 *) &em->destArea;
            em->goalKind = 3;
            Em_ComputeGoal(em, em->goal);
        }
        EmBlangonga_ChangeState(em, 2, 0xB, 1);
        break;
    case 3:
        if (em->remote == 0) {
            em->step = 2;
        }
        break;
    }
}

void EmBlangonga_State2_0D(Em *em)
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
            em->stance = 0;
            em->lookAtTarget = 0;
            Em_SetMotion(em, 4, 0, 0);
            em->timer = (Rand_Next(1) & 0xFFFF & 0xF) + 1;
            break;
        case 1:
            if (--em->timer <= 0) {
                em->step++;
                EmBlangonga_EndState2(em);
            }
            break;
        }
    }
}

void EmBlangonga_RunGroup2(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmBlangonga_State2_00(em, work, 0);
        break;
    case 1:
        EmBlangonga_State2_01(em, work, 0);
        break;
    case 2:
        EmBlangonga_State2_02(em);
        break;
    case 3:
        EmBlangonga_State2_03(em);
        break;
    case 4:
        EmBlangonga_State2_04(em, work, 0);
        break;
    case 5:
        EmBlangonga_State2_05(em);
        break;
    case 6:
        EmBlangonga_State2_06(em, work);
        break;
    case 7:
        EmBlangonga_State2_07(em);
        break;
    case 8:
        EmBlangonga_State2_08(em, work);
        break;
    case 9:
        EmBlangonga_State2_09(em, work);
        break;
    case 10:
        EmBlangonga_State2_04(em, work, 1);
        break;
    case 11:
        EmBlangonga_State2_0B(em, work);
        break;
    case 12:
        EmBlangonga_State2_00(em, work, 1);
        break;
    case 13:
        EmBlangonga_State2_0D(em);
        break;
    case 14:
        EmBlangonga_State2_01(em, work, 1);
        break;
    case 15:
        EmBlangonga_State2_01(em, work, 2);
        break;
    case 16:
        EmBlangonga_State2_00(em, work, 2);
        
    default:
        break;
    }
}

void EmBlangonga_State3_00(Em *em)
{
    if (EmBlangonga_IsBlangonga(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            em->stance = 0;
            Em_SetMotion(em, 0x28, 0, 0);
            break;
        case 1:
            if (EmMotion_ReachedFrame(em, 0, 40.0f) == 0) {
                Em_TurnToGoal(em, 0x60, 0);
            }
            if (Player_PassedFrameScaled(em, 36.0f, 0U) != 0) {
                EffBlangonga27_Create(em, 0x12);
            }
            if (Player_PassedFrameScaled(em, 64.0f, 0U) != 0) {
                EffBlangonga27_Create(em, 0x18);
            }
            if (Player_PassedFrameScaled(em, 86.0f, 0U) != 0) {
                EffBlangonga27_Create(em, 1);
            }
            if (em->motion[0].playing == 0) {
                em->step++;
                Em_SetMotion(em, 0x29, 0, 0);
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

void EmBlangonga_State3_01(Em *em, EmWork *work, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x2A, 0, 0);
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
            Em_SetMotion(em, 0x58, 0, 0);
        }
        break;
    case 2:
        if (Player_PassedFrameScaled(em, 20.0f, 0)) {
            EffBlangonga27_Create(em, 2);
        }
        if (EmBlangonga_IsSmallKind(em) == 0 && Player_PassedFrameScaled(em, 30.0f, 0)) {
            switch (variant & 0xFF) {
            case 1:
                EffBlangonga27_Create(em, 8);
                CamShake_ReqAIf(em, 5);
                if (EmBlangonga_IsBlangonga(em) == 1 || EmBlangonga_IsRajang(em) == 1) {
                    EmBlangonga_State3Pick(em);
                }
                break;
            default:
                CamShake_ReqAIf(em, 3);
                EffBlangonga27_Create(em, 0x19);
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

void EmBlangonga_State3_02(Em *em, EmWork *work, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        switch (variant & 0xFF) {
        case 0:
        default:
            Em_SetMotion(em, 0x2B, 0, 0);
            break;
        case 1:
            Em_SetMotion(em, 0x2C, 0, 0);
            break;
        }
        break;
    case 1:
        switch (variant & 0xFF) {
        case 0:
        default:
            if (Player_PassedFrameScaled(em, 14.0f, 0)) {
                if (EmBlangonga_IsSmallKind(em) == 0) {
                    EffBlangonga27_Create(em, 3);
                } else {
                    EffBlangonga27_Create(em, 0x15);
                }
            }
            break;
        case 1:
            if (Player_PassedFrameScaled(em, 14.0f, 0)) {
                if (EmBlangonga_IsSmallKind(em) == 0) {
                    EffBlangonga27_Create(em, 4);
                } else {
                    EffBlangonga27_Create(em, 0x16);
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

void EmBlangonga_State3_04(Em *em)
{
    if (EmBlangonga_IsCongalala(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            em->stance = 0;
            Em_SetMotion(em, 0x2D, 0, 0);
            break;
        case 1:
            if (Player_PassedFrameScaled(em, 110.0f, 0U) != 0) {
                EffEm33_Create(em, 6, 0, 0, 0);
            }
            if (em->motion[0].playing == 0) {
                em->step++;
                EmBlangonga_EndState(em, 0, 0);
            }
            break;
        }
    }
}

void EmBlangonga_State3_05(Em *em)
{
    if (em->type != 0x3D) {
        EmBlangonga_EndState(em, 0, 0);
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            em->stance = 0;
            Em_SetMotion(em, 0x2E, 0, 0);
            break;
        case 1:
            if (Player_PassedFrameScaled(em, 74.0f, 0U) != 0) {
                EffEm08_CreateAtBone(em, 0xE, 0x11, 0, 0xFC00, 0);
            }
            if (em->motion[0].playing == 0) {
                em->step++;
                EmBlangonga_EndState(em, 0, 0);
            }
            break;
        }
    }
}

void EmBlangonga_State3_06(Em *em)
{
    if (EmBlangonga_IsCongalala(em) == 1) {
        EmBlangonga_EndState(em, 0, 0);
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            em->stateVar0 = 0;
            em->stance = 0;
            Em_SetMotion(em, 0x13, 0, 0);
            break;
        case 1:
            if (Player_PassedFrameScaled(em, 6.0f, 0U) != 0) {
                EffBlangonga27_Create(em, 0xF);
            }
            if ((em->stateVar0 == 0) && (Em_StepToGoal(em, 0x80, 0) == 1)) {
                em->step++;
                em->stateVar0++;
                Em_SetMotion(em, 0x30, 0, 0);
            }
            break;
        case 2:
            if (EmBlangonga_IsBlangonga(em) == 1) {
                if (Player_PassedFrameScaled(em, 10.0f, 0U) != 0) {
                    EffBlangonga27_Create(em, 0x11);
                }
                if (Player_PassedFrameScaled(em, 20.0f, 0U) != 0) {
                    if (EmBlangonga_IsSmallKind(em) == 1) {
                        EffBlangonga27_Create(em, 0x1A);
                    } else {
                        EffBlangonga27_Create(em, 5);
                    }
                }
            } else if (Player_PassedFrameScaled(em, 22.0f, 0U) != 0) {
                EffBlangonga27_Create(em, 0x25);
            }
            if (em->motion[0].playing == 0) {
                em->step++;
                EmBlangonga_EndState(em, 0, 0);
            }
            break;
        }
    }
}

void EmBlangonga_State3_07(Em *em, EmWork *work, s32 variant)
{
    u16 ang;

    if (em->type != 0x33) {
        EmBlangonga_EndState(em, 0, 0);
        return;
    }
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x2F, 0, 0);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 98.0f, 0)) {
            EffBlangonga27_Create(em, 0x13);
        }
        if (Player_PassedFrameScaled(em, 102.0f, 0)) {
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
            EffEm08_CreateAtBone(em, 0xD, 0xF, 0, ang, 0);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

void EmBlangonga_State3_08(Em *em, EmWork *work)
{
    EmBlangongaWork *w = (EmBlangongaWork *)work;

    if (em->type != 0x34) {
        EmBlangonga_EndState(em, 0, 0);
        return;
    }
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x28, 0, 0);
        break;
    case 1:
        if (w->unk044 != 0 && Player_PassedFrameScaled(em, 100.0f, 0)) {
            StageObj_K1C_CreateOnEmC(em, 0x17, w->unk044 - 1, 1.0f);
            EffEm33_Create(em, 1, w->unk044 - 1, 0, 0);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

void EmBlangonga_State3_09(Em *em, EmWork *work)
{
    if (EmBlangonga_IsCongalala(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
        return;
    }
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x29, 0, 0);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 26.0f, 0)) {
            EffBlangonga27_Create(em, 0x1F);
        }
        if (EmMotion_InFrameRange(em, 0, 170.0f, 180.0f)) {
            em->rotY += 0xCCC;
        }
        if (EmBlangonga_IsSmallKind(em) == 0 && Player_PassedFrameScaled(em, 200.0f, 0)) {
            EffEm07_CreateAt(em, &em->posX, 0x10);
            CamShake_ReqAIf(em, 5);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x50, 0, 0);
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

void EmBlangonga_State3_0A(Em *em)
{
    if (em->type != 0x34) {
        EmBlangonga_EndState(em, 0, 0);
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            em->stance = 0;
            Em_SetMotion(em, 0x51, 0, 0);
            break;
        case 1:
            if (Player_PassedFrameScaled(em, 40.0f, 0U) != 0) {
                EffBlangonga27_Create(em, 0x20);
                em->hitzoneSet = 1;
            }
            if (em->motion[0].playing == 0) {
                em->step++;
                Em_SetMotion(em, 0x52, 0, 0);
                em->hitzoneSet = 0;
            }
            break;
        case 2:
            if (Player_PassedFrameScaled(em, 88.0f, 0U) != 0) {
                EffBlangonga27_Create(em, 0x29);
            }
            if (em->motion[0].playing == 0) {
                em->step++;
                EmBlangonga_EndState(em, 0, 0);
            }
            break;
        }
    }
}

void EmBlangonga_State3_0B(Em *em, EmWork *work, s32 variant)
{
    if (EmBlangonga_IsCongalala(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            em->stance = 0;
            Em_SetMotion(em, 0x2E, 0, 0);
            break;
        case 1:
            if (!(variant & 0xFF)) {
                if (Player_PassedFrameScaled(em, 66.0f, 0U) != 0) {
                    EffEm08_CreateAtBone(em, 0x21, 0x13, 0, 0x93F, 0);
                }
            } else if (Player_PassedFrameScaled(em, 66.0f, 0U) != 0) {
                EffEm08_CreateAtBone(em, 0x21, 0x13, 0, 0x2000, 0);
            }
            if (em->motion[0].playing == 0) {
                em->step++;
                EmBlangonga_EndState(em, 0, 0);
            }
            break;
        }
    }
}

void EmBlangonga_State3_0C(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x55, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

void EmBlangonga_State3_0E(Em *em, EmWork *work)
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
        em->stance = 0;
        Em_SetMotion(em, 0x50, 0, 0);
        break;
    case 1:
        if (em->stateVar1 == 0 && Player_PassedFrameScaled(em, 98.0f, 0)) {
            StageObj_K1C_CreateOnEm(em, 8, 0.75f * em->scaleX);
            EffBlangonga27_Create(em, 0xA);
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
            Em_SetMotion(em, 0x51, 0, 0);
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

void EmBlangonga_State3_11(Em *em, EmWork *work, s32 variant)
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
        em->stance = 0;
        Em_SetMotion(em, 0x2B, 0, 0);
        if ((variant & 0xFF) == 0) {
            em->timer = 3;
        } else {
            em->timer = 1;
        }
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 16.0f, 0) && em->stateVar0 == 0) {
            em->stateVar0++;
            EffBlangonga27_Create(em, 0x24);
        }
        if (em->motion[0].playing == 0 && --em->timer <= 0) {
            em->step++;
            em->stateVar0 = 0;
            Em_SetMotion(em, 0x2C, 0, 0);
        }
        break;
    case 2:
        if (Player_PassedFrameScaled(em, 26.0f, 0) && em->stateVar0 == 0) {
            em->stateVar0++;
            Em_BonePos(em, 0xD, pos);
            pos[1] = em->floorY;
            EffEm07_CreateAt(em, pos, 0xF);
            CamShake_ReqAIf(em, 5);
            EmBlangonga_State3Pick(em);
            if ((s32)Player_InCurrentArea(em) & 0xFF) {
                StageObj_K11_CreateAtScaled(pos, *(u16 *) &em->rotY, 0xA, 0.4f * em->scaleX);
            }
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

void EmBlangonga_State3_12(Em *em, EmWork *work, s32 variant)
{
    if (EmBlangonga_IsRajang(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
        return;
    }
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x50, 2, 0);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 84.0f, 0)) {
            em->step++;
            switch (variant & 0xFF) {
            case 0:
            default:
                EffEm33_Create(em, 2, 0, 0x71C, 0);
                StageObj_K1C_CreateOnEmB(em, 0x25, 0x71C, 0, 1.0f);
                break;
            case 1:
                EffEm33_Create(em, 2, 0, 0xF8E5, 0);
                StageObj_K1C_CreateOnEmB(em, 0x25, 0xF8E5, 0, 1.0f);
                break;
            }
            Em_SetMotion(em, 0x51, 2, 0);
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

void EmBlangonga_State3_13(Em *em, EmWork *work, s32 variant)
{
    if (EmBlangonga_IsRajang(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
        return;
    }
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x50, 0, 0);
        em->timer = 2;
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 80.0f, 0)) {
            switch (variant & 0xFF) {
            case 0:
            default:
                EffEm08_CreateAtBone2(em, 1, 0x15, 0, 0, 0, 0x28);
                break;
            case 1:
                EffEm08_CreateAtBone2(em, 1, 0x15, 1, 0, 0, 0x28);
                break;
            }
        }
        if (em->motion[0].playing == 0) {
            em->timer--;
        }
        if (em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x51, 0, 0);
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

void EmBlangonga_State3_14(Em *em, EmWork *work, s32 variant)
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
            em->stance = 0;
            Em_SetMotion(em, 0x52, 0, 0);
            if (k == 1) {
                em->holdTimer = 0x32;
            }
            break;
        case 1:
            if (Player_PassedFrameScaled(em, 24.0f, 0U) != 0) {
                em->step++;
                em->stance = 2;
                em->posY += 170.0f;
                EmBlangonga_StartCurves(em, 1U);
            }
            break;
        case 2:
            EmBlangonga_UpdateCurves(em);
            if (Player_PassedFrameScaled(em, 30.0f, 0U) != 0) {
                em->step++;
                Em_SetMotion(em, 0x53, 6, 0);
                EmBlangonga_StartCurves(em, 2U);
            }
            break;
        case 3:
            if (Player_PassedFrameScaled(em, 22.0f, 0U) != 0) {
                EffEm33_Create(em, 2, 1, 0x31C7, 0);
                StageObj_K1C_CreateOnEmB(em, 0x25, 0x31C7, 0, 1.0f);
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
                em->stance = 0;
                Em_SetMotion(em, 0x54, 0, 0);
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

void EmBlangonga_State3_15(Em *em)
{
    if (EmBlangonga_IsRajang(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            em->stance = 0;
            Em_SetMotion(em, 0x56, 2, 0);
            break;
        case 1:
            if (Player_PassedFrameScaled(em, 56.0f, 0U) != 0) {
                EffBlangonga27_Create(em, 0x26);
            }
            if (em->motion[0].playing == 0) {
                em->step++;
                EmBlangonga_EndState(em, 0, 0);
            }
            break;
        }
    }
}

void EmBlangonga_State3_16(Em *em)
{
    u8 stateVar0;

    if (EmBlangonga_IsRajang(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            em->stateVar0 = 0;
            em->stance = 0;
            Em_SetMotion(em, 0x57, 2, 0);
            break;
        case 1:
            if (Player_PassedFrameScaled(em, 6.0f, 0U) != 0) {
                stateVar0 = em->stateVar0;
                if (stateVar0 == 0) {
                    em->stateVar0 = stateVar0 + 1;
                    EffBlangonga27_Create(em, 0x27);
                }
            }
            if (Em_StepToGoal(em, 0x80, 0) == 1) {
                em->step++;
                Em_SetMotion(em, 0x5A, 0, 0);
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

void EmBlangonga_State3_17(Em *em)
{
    if (EmBlangonga_IsCongalala(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            em->stance = 0;
            Em_SetMotion(em, 0x13, 0, 0);
            if (EmBlangonga_IsSmallKind(em) == 1) {
                EffBlangonga27_Create(em, 0x23);
                break;
            }
            EffBlangonga27_Create(em, 0x1D);
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

void EmBlangonga_RunGroup3(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmBlangonga_State3_00(em);
        break;
    case 1:
        EmBlangonga_State3_01(em, work, 0);
        break;
    case 2:
        EmBlangonga_State3_02(em, work, 0);
        break;
    case 3:
        EmBlangonga_State3_02(em, work, 1);
        break;
    case 4:
        EmBlangonga_State3_04(em);
        break;
    case 5:
        EmBlangonga_State3_05(em);
        break;
    case 6:
        EmBlangonga_State3_06(em);
        break;
    case 7:
        EmBlangonga_State3_07(em, work, 0);
        break;
    case 8:
        EmBlangonga_State3_08(em, work);
        break;
    case 9:
        EmBlangonga_State3_09(em, work);
        break;
    case 0xA:
        EmBlangonga_State3_0A(em);
        break;
    case 0xB:
        EmBlangonga_State3_0B(em, work, 0);
        break;
    case 0xC:
        EmBlangonga_State3_0C(em);
        break;
    case 0xD:
        EmBlangonga_State3_01(em, work, 1);
        break;
    case 0xE:
        EmBlangonga_State3_0E(em, work);
        break;
    case 0xF:
        EmBlangonga_State3_07(em, work, 1);
        break;
    case 0x10:
        EmBlangonga_State3_07(em, work, 2);
        break;
    case 0x11:
        EmBlangonga_State3_11(em, work, 0);
        break;
    case 0x12:
        EmBlangonga_State3_12(em, work, 0);
        break;
    case 0x13:
        EmBlangonga_State3_13(em, work, 0);
        break;
    case 0x14:
        EmBlangonga_State3_14(em, work, 0);
        break;
    case 0x15:
        EmBlangonga_State3_15(em);
        break;
    case 0x16:
        EmBlangonga_State3_16(em);
        break;
    case 0x17:
        EmBlangonga_State3_17(em);
        break;
    case 0x18:
        EmBlangonga_State3_13(em, work, 1);
        break;
    case 0x19:
        EmBlangonga_State3_0B(em, work, 1);
        break;
    case 0x1A:
        EmBlangonga_State3_11(em, work, 1);
        break;
    case 0x1B:
        EmBlangonga_State3_12(em, work, 1);
        break;
    }
}

void EmBlangonga_State4_00(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x3D, 0, 0);
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

void EmBlangonga_State4_01(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x3C, 0, 0);
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

void EmBlangonga_State4_02(Em *em, EmWork *work, u8 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        if (!(variant & 1)) {
            Em_SetMotion(em, 0x45, 0, 0);
        } else {
            Em_SetMotion(em, 0x46, 0, 0);
        }
        switch (variant) {
        case 2:
        case 3:
            StageObj_Stun_CreateEm(em, 1, 0x11, (s32) ((f32) em->timer + (em->motion[0].frameCount / 2.0f)), 100.0f, 2.0f);
            break;
        }
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            if (!(variant & 1)) {
                Em_SetMotion(em, 0x3F, 0, 0);
            } else {
                Em_SetMotion(em, 0x4D, 0, 0);
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
                Em_SetMotion(em, 0x40, 0, 0);
            } else {
                Em_SetMotion(em, 0x4E, 0, 0);
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
                EmBlangonga_ChangeState(em, 0, 0x19, 4);
                break;
            }
        }
        break;
    case 4:
        if (em->remote == 0) {
            EmBlangonga_ChangeState(em, 0, 0x19, 4);
        }
        break;
    }
}

void EmBlangonga_State4_04(Em *em)
{
    Em_SetFlag6AE(em, 4U);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x41, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmBlangonga_ChangeState(em, 0, 0xA, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmBlangonga_ChangeState(em, 0, 0xA, 4);
        }
        break;
    }
    Em_SpawnEffect06Bone(em, 2);
}

void EmBlangonga_State4_05(Em *em)
{
    Em_SetFlag6AE(em, 3U);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x49, 0, 0);
        em->noReact = 1;
        em->holdTimer = 0x32;
        break;
    case 1:
        if ((s8) em->holdTimer < 5) {
            em->holdTimer = 5;
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_ChangeState(em, 4, 6, 3);
        }
        break;
    case 2:
        if ((s8) em->holdTimer < 5) {
            em->holdTimer = 5;
        }
        if (em->remote == 0) {
            EmBlangonga_ChangeState(em, 4, 6, 3);
        }
        break;
    }
}

void EmBlangonga_State4_06(Em *em)
{
    u8 stateVar1;

    em->holdTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x4A, 0, 0);
        em->noReact = 1;
        em->stateVar1 = 0;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            Em_SetMotion(em, 0x4A, 0, 0);
            em->stateVar1++;
        }
        if ((em->type == 0x34) && (em->rage != 0) && (Motion_PassedFrame(em, 0, 104.0f) != 0)) {
            stateVar1 = em->stateVar1;
            if ((stateVar1 & 7) == (((s32) em->stateRand >> (stateVar1 & 3)) & 7)) {
                EffEm33_Create(em, 7, 0, 0, 0);
            }
        }
        if (--em->trapTimer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x4A, 0, 0);
            EmBlangonga_ChangeState(em, 0, 0xE, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmBlangonga_ChangeState(em, 0, 0xE, 4);
        }
        if (em->motion[0].playing == 0) {
            Em_SetMotion(em, 0x4A, 0, 0);
        }
        break;
    }
}

void EmBlangonga_State4_07(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x42, 0, 0);
        EmBlangonga_StartCurves(em, 0U);
        break;
    case 1:
        EmBlangonga_UpdateCurves(em);
        if (em->posY <= (em->floorY + (180.0f * em->scaleX))) {
            em->step++;
            em->posY = em->floorY;
            em->stance = 0;
            Em_SetMotion(em, 0x43, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 0x3C;
            Em_SetMotion(em, 0x3F, 0, 0);
        }
        break;
    case 3:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x40, 0, 0);
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

void EmBlangonga_State4_08(void)
{
}

void EmBlangonga_State4_09(Em *em, EmWork *work, u8 variant)
{
    u16 m;

    em->holdTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        if (em->type == 0x33 && (m = em->motionId[0], m != 0x3FE) && m != 0x432 && m != 0x433) {
            Em_SetMotion(em, 0x16, 0, 0);
            em->stateArmor = 1;
            em->noPushTimer = 5;
        } else {
            em->step++;
            Em_SetMotion(em, 0x4B, 0, 0);
        }
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        switch (variant) {
        case 0:
            break;
        case 1:
            StageObj_Stun_CreateEm(em, 1, 0x11, (s32) ((f32) em->timer + (em->motion[0].frameCount / 2.0f)), 100.0f, 2.0f);
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
            Em_SetMotion(em, 0x4B, 0, 0);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetGoal12(em);
            switch (variant) {
            case 0:
            default:
                EmBlangonga_ChangeState(em, 4, 0xA, 3);
                break;
            case 1:
                EmBlangonga_ChangeState(em, 4, 0xF, 3);
                break;
            }
        }
        break;
    case 3:
        if (em->remote == 0) {
            switch (variant) {
            case 0:
            default:
                EmBlangonga_ChangeState(em, 4, 0xA, 3);
                break;
            case 1:
                EmBlangonga_ChangeState(em, 4, 0xF, 3);
                break;
            }
        }
        break;
    }
    if (variant != 1) {
        Em_SpawnEffect06Bone(em, 2);
    }
}

void EmBlangonga_State4_0B(Em *em)
{
    em->holdTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x4A, 0, 0);
        em->noReact = 1;
        em->blind = 2;
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetGoal12(em);
            em->blind = 0;
            EmBlangonga_ChangeState(em, 4, 6, 3);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmBlangonga_ChangeState(em, 4, 6, 3);
        }
        break;
    }
}

void EmBlangonga_State4_10(Em *em)
{
    Em_SetHold2(em);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x41, 0, 0);
        Player_SetFlag((Player *) em, 0x20000U);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmBlangonga_ChangeState(em, 0, 0x1B, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmBlangonga_ChangeState(em, 0, 0x1B, 4);
        }
        break;
    }
}

void EmBlangonga_State4_11(Em *em, EmWork *work, s32 variant)
{
    s32 k;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x4F, 0, 0);
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

void EmBlangonga_State4_12(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x3C, 0, 0x32);
        Em_RestartScript(em);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 94.0f, 0U) != 0) {
            em->step++;
            Em_SetMotion(em, 0x51, 0xC, 0x14);
        }
        break;
    case 2:
        if (Player_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            EffBlangonga27_Create(em, 0x20);
            em->hitzoneSet = 1;
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x52, 0, 0);
            em->hitzoneSet = 0;
        }
        break;
    case 3:
        if (Player_PassedFrameScaled(em, 88.0f, 0U) != 0) {
            EffBlangonga27_Create(em, 0x29);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

void EmBlangonga_State4_18(Em *em, EmWork *work)
{
    EmBlangonga_State3_14(em, work, 0x1);
}

void EmBlangonga_RunGroup4(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmBlangonga_State4_00(em);
        break;
    case 1:
        EmBlangonga_State4_01(em);
        break;
    case 2:
        EmBlangonga_State4_02(em, work, 0);
        break;
    case 3:
        EmBlangonga_State4_02(em, work, 1);
        break;
    case 4:
        EmBlangonga_State4_04(em);
        break;
    case 5:
        EmBlangonga_State4_05(em);
        break;
    case 6:
        EmBlangonga_State4_06(em);
        break;
    case 7:
        EmBlangonga_State4_07(em);
        break;
    case 8:
        EmBlangonga_State4_08();
        break;
    case 9:
        EmBlangonga_State4_09(em, work, 0);
        break;
    case 0xA:
        EmBlangonga_State4_06(em);
        break;
    case 0xB:
        EmBlangonga_State4_0B(em);
        break;
    case 0xC:
        EmBlangonga_State4_02(em, work, 2);
        break;
    case 0xD:
        EmBlangonga_State4_02(em, work, 3);
        break;
    case 0xE:
        EmBlangonga_State4_09(em, work, 1);
        break;
    case 0xF:
        EmBlangonga_State4_06(em);
        break;
    case 0x10:
        EmBlangonga_State4_10(em);
        break;
    case 0x11:
        EmBlangonga_State4_11(em, work, 0);
        break;
    case 0x12:
        EmBlangonga_State4_12(em);
        break;
    case 0x13:
        EmBlangonga_State4_0B(em);
        break;
    case 0x14:
        EmBlangonga_State4_01(em);
        break;
    case 0x15:
        EmBlangonga_State4_11(em, work, 1);
        break;
    case 0x16:
        EmBlangonga_State4_11(em, work, 2);
        break;
    case 0x17:
        EmBlangonga_State4_11(em, work, 3);
        break;
    case 0x18:
        EmBlangonga_State4_18(em, work);
        break;
    }
}

void EmBlangonga_State5_00_Hunted(Em *em, s32 unused)
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
        Em_SetMotion(em, 0x44, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x47, 0, 0);
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
            EmBlangonga_ChangeState(em, 5, 4, 4);
        }
        break;
    }
}

void EmBlangonga_State5_01_Hunted(Em *em, s32 unused)
{
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x4B, 0, 0);
        em->lookAtTarget = 0;
        em->stance = 3;
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
            EmBlangonga_ChangeState(em, 5, 4, 4);
        }
        break;
    }
}

void EmBlangonga_State5_02_Hunted(Em *em, s32 unused)
{
    f32 alpha;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->stateArmor = 1;
        em->noPushTimer = 5;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x16, 0, 0);
        QuestEm_Hunted(em);
        Em_EndRage(em);
        em->noReact = 1;
        break;
    case 1:
        if (em->motion[0].blend != 0) {
            em->stateArmor = 1;
            em->noPushTimer = 5;
        }
        if (Player_PassedFrameScaled(em, 14.0f, 0U) != 0) {
            em->stance = 0;
            Em_SetMotion(em, 0x4B, 0, 0);
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
            EmBlangonga_ChangeState(em, 5, 4, 4);
        }
        break;
    }
}

void EmBlangonga_State5_03_HuntedFalling(Em *em, s32 unused)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x42, 0, 0);
        QuestEm_Hunted(em);
        Em_EndRage(em);
        EmBlangonga_StartCurves(em, 0);
        break;
    case 1:
        EmBlangonga_UpdateCurves(em);
        if (em->posY <= 180.0f + em->floorY) {
            em->step++;
            em->posY = em->floorY;
            em->stance = 3;
            Em_SetMotion(em, 0x43, 0, 0);
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
            EmBlangonga_ChangeState(em, 5, 4, 4);
        }
        break;
    }
}

void EmBlangonga_State5_04(Em *em, s32 unused)
{
    switch (em->step) {
    case 0:
        if (QuestEm_CanAppear(em) == 1) {
            if (EmBlangonga_IsSmallKind(em) == 1) {
                if ((em->type == 0x3D) && (em->spawnParam == 1)) {
                    em->stance = 0;
                    Em_SetMotionRow(em, 4, 0, 0);
                    EmBlangonga_ChangeState(em, 0, 0x18, 0);
                    break;
                }
                EmBlangonga_ChangeState(em, 0, 1, 0);
                break;
            }
            em->posY = gAreaGroundY[em->area];
            em->stance = 2;
            EmBlangonga_ChangeState(em, 2, 9, 0);
            break;
        }
        em->taskPhase++;
        break;
    }
}

void EmBlangonga_RunGroup5(Em *em, s32 work)
{
    if (EmBlangonga_IsSmallKind(em) == 1) {
        em->noHitTimer = 0xA;
    }
    em->noPushTimer = 0xA;
    switch (em->state) {
    case 0:
        EmBlangonga_State5_00_Hunted(em, work);
        break;
    case 1:
        EmBlangonga_State5_01_Hunted(em, work);
        break;
    case 2:
        EmBlangonga_State5_02_Hunted(em, work);
        break;
    case 3:
        EmBlangonga_State5_03_HuntedFalling(em, work);
        break;
    case 4:
        EmBlangonga_State5_04(em, work);
        break;
    }
}

void EmBlangonga_State6_00_Captured(Em *em)
{
    s32 timer;
    s32 t;
    u16 motion;

    Em_SetCaptured(em, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        if ((em->type == 0x33) && (motion = em->motionId[0], (motion != 0x3FE)) && (motion != 0x432) && (motion != 0x433)) {
            Em_SetMotion(em, 0x16, 0, 0);
            em->stateArmor = 1;
            em->noPushTimer = 5;
        } else {
            em->step++;
            Em_SetMotion(em, 0x4B, 0, 0);
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
            Em_SetMotion(em, 0x4B, 0, 0);
            QuestEm_Captured(em);
            em->timer = 0x708;
        }
        break;
    case 2:
        if (((s32) em->timer % 135) == 0) {
            Em_PlaySeAtBone(em, 0x52, 0x13);
        }
        if (em->type == 0x34) {
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
        t = em->timer;
        if (t > 0) {
            em->timer = t - 1;
        }
        timer = em->timer;
        if (timer <= 0) {
            if (em->remote == 0) {
                EmBlangonga_ChangeState(em, 5, 4, 4);
                break;
            }
        } else {
            em->alpha = (f32) timer / 150.0f;
        }
        break;
    }
}

void EmBlangonga_State6_01_Captured(Em *em)
{
    s32 timer;
    s32 t;

    Em_SetCaptured(em, 1);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x21, 0, 0);
        Em_BeginStatus1(em);
        Em_ClearPoison(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x1F, 0, 0);
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
        t = em->timer;
        if (t > 0) {
            em->timer = t - 1;
        }
        timer = em->timer;
        if (timer <= 0) {
            if (em->remote == 0) {
                EmBlangonga_ChangeState(em, 5, 4, 4);
                break;
            }
        } else {
            em->alpha = (f32) timer / 150.0f;
        }
        break;
    }
}

void EmBlangonga_State6_02(Em *em, EmWork *work)
{
    EmBlangongaWork *cw = (EmBlangongaWork *)work;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        GmEm_ResetSize(em);
        em->step++;
        em->stance = 0;
        em->posX = 12212.0f;
        em->posY = 0.0f;
        em->posZ = 9484.0f;
        em->rotX = 0;
        em->rotY = 0x4000;
        em->rotZ = 0;
        Em_SetMotion(em, 1, 0, 0);
        em->timer = 180;
        cw->unk048 = 0;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x39, 0, 0);
        }
        break;
    case 2:
        if (EmMotion_InFrameRange(em, 0, 4.0f, 26.0f)) {
            em->rotY += (u32) (1638.4f * em->motionSpeed);
        }
        if (cw->unk048 == 0 && EmMotion_ReachedFrame(em, 0, 34.0f)) {
            cw->unk048 = 1;
        }
        if (Player_PassedFrameScaled(em, 300.0f, 0)) {
            em->step++;
            Em_SetMotion(em, 0x39, 0, 0x96);
        }
        break;
    case 3:
        if (Player_PassedFrameScaled(em, 250.0f, 0)) {
            em->step++;
            Em_SetMotion(em, 0x55, 0, 0);
            cw->unk048 = 0;
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x34, 0, 0);
        }
        break;
    case 5:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 1, 0, 0);
        }
        break;
    case 6:
        if (Save_TestEventFlag(0x9B) == 1) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

void EmBlangonga_State6_03(Em *em, EmWork *work)
{
    s16 i;
    u8 n = 0;
    Em *m;

    if (em->type == 0x33) {
        EmBlangonga_ChangeState(em, 6, 4, 0);
        return;
    }
    for (i = 0, m = gEms; i < 20; i++, m++) {
        if (m->type == 0x3D) {
            if (em->slotNo == m->slotNo) {
                switch (n) {
                case 0:
                    EmBlangonga_ChangeState(em, 6, 5, 0);
                    return;
                case 1:
                    EmBlangonga_ChangeState(em, 6, 6, 0);
                    return;
                case 2:
                    EmBlangonga_ChangeState(em, 6, 7, 0);
                    return;
                default:
                    EmBlangonga_ChangeState(em, 6, 8, 0);
                    return;
                }
            }
            n++;
        }
    }
}

void EmBlangonga_State6_04(Em *em, EmWork *work)
{
    f32 d;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        GmEm_ResetSize(em);
        em->step++;
        em->stance = 0;
        em->posX = 10747.0f;
        em->posY = 2468.0f;
        em->posZ = 8181.0f;
        Vec_Copy(&em->prevPosX, &em->posX);
        em->rotX = 0;
        em->rotY = 0xFC00;
        em->rotZ = 0;
        Em_SetMotion(em, 1, 0, 0);
        em->timer = 300;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x3B, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 190;
            Em_SetMotion(em, 1, 0, 0);
        }
        break;
    case 3:
        if (--em->timer <= 0) {
            em->step++;
            em->stance = 2;
            Em_SetMotion(em, 0xA, 0, 0);
        }
        break;
    case 4:
        if (Player_PassedFrameScaled(em, 26.0f, 0)) {
            em->step++;
            Player_ClearVelocity(em);
            Em_SetMotion(em, 0x12, 0, 0x32);
        }
        break;
    case 5:
        if (Player_PassedFrameScaled(em, 70.0f, 0)) {
            em->step++;
            em->stance = 0;
            em->posX = 10682.0f;
            em->posY = 288.0f;
            em->posZ = 9620.0f;
            em->goal[0] = 10682.0f;
            em->goal[1] = 288.0f;
            em->goal[2] = 12400.0f;
            em->rotX = 0;
            em->rotY = Math_AngleXZ(&em->posX, em->goal);
            em->rotZ = 0;
            Em_SetMotion(em, 0x14, 0, 0);
        }
        break;
    case 6:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 1200;
            Em_SetMotion(em, 3, 0, 0);
        }
        break;
    case 7:
        d = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer <= 0 || d <= 100.0f) {
            em->step++;
            Em_SetMotion(em, 1, 0, 0);
        }
        break;
    case 8:
        if (Save_TestEventFlag(0xA4) == 1) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

void EmBlangonga_State6_05(Em *em, EmWork *work, u8 variant)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        GmEm_ResetSize(em);
        em->step++;
        em->stance = 0;
        em->stateArmor = 1;
        Em_SetMotion(em, 4, 0, 0);
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
            Em_SetMotion(em, 0x16, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x33, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stateVar1 = 0;
            Em_SetMotion(em, 0x34, 0, 0);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            if (em->stateVar1 == 0) {
                em->stateVar1 = 1;
                Em_SetMotion(em, 0x33, 0, 0);
            } else {
                em->stateVar1 = 0;
                Em_SetMotion(em, 0x34, 0, 0);
            }
        }
        if (Save_TestEventFlag(0xA4) == 1) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

void EmBlangonga_State6_08(Em *em)
{
    switch (em->step) {
    case 0:
        GmEm_ResetSize(em);
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 0;
        Em_SetMotion(em, 1, 0, 0);
        em->visible = 0;
        em->scriptPending = 0;
        break;
    case 1:
        if (Save_TestEventFlag(0xA4) == 1) {
            em->step++;
            em->visible = 1;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

void EmBlangonga_RunGroup6(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmBlangonga_State6_00_Captured(em);
        break;
    case 1:
        EmBlangonga_State6_01_Captured(em);
        break;
    case 2:
        EmBlangonga_State6_02(em, work);
        break;
    case 3:
        EmBlangonga_State6_03(em, work);
        break;
    case 4:
        EmBlangonga_State6_04(em, work);
        break;
    case 5:
        EmBlangonga_State6_05(em, work, 0);
        break;
    case 6:
        EmBlangonga_State6_05(em, work, 1);
        break;
    case 7:
        EmBlangonga_State6_05(em, work, 2);
        break;
    case 8:
        EmBlangonga_State6_08(em);
        break;
    }
}

void EmBlangonga_State8_00_CarveStart(Em *em, EmWork *work)
{
    EmBVec v;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        if (Versus_DefeatCamera(em) == 1) {
            v = EmBlangonga_CarveThrowPt;
            StageObj_K25_Create(&gPlayers[(u8) em->side - 1], em, 0, 0, &v.x);
            em->step++;
            if (em->type == 0x34) {
                em->step++;
                Em_SetMotion(em, 0x54, 0, 0);
            } else {
                Em_SetMotion(em, 0x39, 0, 0);
            }
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x3A, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            Versus_RouteEmByTeam(em);
        }
        break;
    }
}

void EmBlangonga_State8_01(Em *em, EmWork *work)
{
    u8 step;
    u8 step2;
    u8 step3;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->type) {
    case 0x33:
        step = em->step;
        switch (step) {
        case 0:
            em->step = step + 1;
            Em_SetMotion(em, 0x59, 0, 0);
            break;
        case 1:
            if (em->motion[0].playing == 0) {
                em->step = step + 1;
                Em_SetMotion(em, 1, 0, 0);
                EmBlangonga_ChangeState(em, 8, 0x98, 1);
            }
            break;
        }
        break;
    case 0x34:
        step2 = em->step;
        switch (step2) {
        case 0:
            em->step = step2 + 1;
            Em_SetMotion(em, 0x3C, 0, 0);
            break;
        case 1:
            if (em->motion[0].playing == 0) {
                em->step = step2 + 1;
                Em_SetMotion(em, 1, 0, 0);
                EmBlangonga_ChangeState(em, 8, 0x98, 1);
            }
            break;
        }
        break;
    default:
        step3 = em->step;
        switch (step3) {
        case 0:
            em->step = step3 + 1;
            Em_SetMotion(em, 0x3D, 0, 0);
            break;
        case 1:
            if (em->motion[0].playing == 0) {
                em->step = step3 + 1;
                Em_SetMotion(em, 1, 0, 0);
                EmBlangonga_ChangeState(em, 8, 0x98, 1);
            }
        }
        break;
    }
}

void EmBlangonga_State8_02(Em *em, EmWork *work)
{
    u8 step;
    u8 step3;
    u8 step2;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->type) {
    case 0x33:
        step = em->step;
        switch (step) {
        case 0:
            em->step = step + 1;
            Em_SetMotion(em, 0x3B, 0, 0);
            break;
        case 1:
            if (em->motion[0].playing == 0) {
                em->step = step + 1;
                Em_SetMotion(em, 1, 0, 0);
                EmBlangonga_ChangeState(em, 8, 0x98, 1);
            }
            break;
        }
        break;
    case 0x34:
        step2 = em->step;
        switch (step2) {
        case 0:
            em->step = step2 + 1;
            Em_SetMotion(em, 0x34, 0, 0);
            break;
        case 1:
            if (em->motion[0].playing == 0) {
                em->step = step2 + 1;
                Em_SetMotion(em, 1, 0, 0);
                EmBlangonga_ChangeState(em, 8, 0x98, 1);
            }
            break;
        }
        break;
    default:
        step3 = em->step;
        switch (step3) {
        case 0:
            em->step = step3 + 1;
            Em_SetMotion(em, 0x3B, 0, 0);
            break;
        case 1:
            if (em->motion[0].playing == 0) {
                em->step = step3 + 1;
                Em_SetMotion(em, 1, 0, 0);
                EmBlangonga_ChangeState(em, 8, 0x98, 1);
            }
        }
        break;
    }
}

void EmBlangonga_State8_03(Em *em, EmWork *work)
{
    u8 step;
    u8 step2;
    u8 step3;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->type) {
    case 0x33:
        step2 = em->step;
        switch (step2) {
        case 0:
            em->step = step2 + 1;
            Em_SetMotion(em, 0x34, 0, 0);
            break;
        case 1:
            if (em->motion[0].playing == 0) {
                em->step = step2 + 1;
                Em_SetMotion(em, 1, 0, 0);
                EmBlangonga_ChangeState(em, 8, 0x98, 1);
            }
            break;
        }
        break;
    case 0x34:
        step = em->step;
        switch (step) {
        case 0:
            em->step = step + 1;
            Em_SetMotion(em, 0x51, 0, 0);
            break;
        case 1:
            if (em->motion[0].playing == 0) {
                em->step = step + 1;
                Em_SetMotion(em, 0x52, 0, 0);
            }
            break;
        case 2:
            if (em->motion[0].playing == 0) {
                em->step = step + 1;
                Em_SetMotion(em, 1, 0, 0);
                EmBlangonga_ChangeState(em, 8, 0x98, 1);
            }
            break;
        }
        break;
    default:
        step3 = em->step;
        switch (step3) {
        case 0:
            em->step = step3 + 1;
            Em_SetMotion(em, 0x34, 0, 0);
            break;
        case 1:
            if (em->motion[0].playing == 0) {
                em->step = step3 + 1;
                Em_SetMotion(em, 1, 0, 0);
                EmBlangonga_ChangeState(em, 8, 0x98, 1);
            }
        }
        break;
    }
}

void EmBlangonga_State8_98(Em *em, EmWork *work)
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
            EmBlangonga_ChangeState(em, 8, 0x99, 1);
        }
        break;
    }
}

void EmBlangonga_State8_99(Em *em, EmWork *work)
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
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

void EmBlangonga_RunGroup8(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0x0:
        EmBlangonga_State8_00_CarveStart(em, work);
        break;
    case 0x1:
        EmBlangonga_State8_01(em, work);
        break;
    case 0x2:
        EmBlangonga_State8_02(em, work);
        break;
    case 0x3:
        EmBlangonga_State8_03(em, work);
        break;
    case 0x98:
        EmBlangonga_State8_98(em, work);
        break;
    case 0x99:
        EmBlangonga_State8_99(em, work);
        break;
    }
}

void EmBlangonga_Update(Em *em)
{
    EmBlangongaWork *w = (EmBlangongaWork *)&em->work;
    u8 saved;
    s16 sum;
    s32 ev;
    u8 k;
    s32 hit;
    s32 i;
    u8 buf[4];

    EmBlangonga_UpdateRageTimer(em);
    if (em->stateGroup <= 4) {
        em->alpha = 1.0f;
        em->visible = 1;
    }
    if (*(s8 *) &em->senseDelay != 0) {
        em->noHitTimer = 5;
    }
    if (w->unk045 > 0) {
        w->unk045--;
    }
    Em_UpdateCombatTimer(em);
    if (EmBlangonga_IsSmallKind(em) == 0) {
        Em_RaiseTrigger80(em);
    }
    if ((em->stateGroup != 2 || em->state != 8) && (em->stateGroup != 2 || em->state != 9) &&
        (em->stateGroup != 2 || em->state != 0xB)) {
        Em_HoldPosNoPlayers(em, 0, 1);
    }
    if (EmBlangonga_ShouldEndState(em) == 1) {
        EmBlangonga_EndState(em, 0, 0);
    }
    if ((s16) Player_IsState(em, 0, 0x18)) {
        saved = em->senseDelay;
        em->senseDelay = 0;
    }
    if (em->type == 0x3D) {
        hit = Em_SenseObjects(em, &w->sense) & 0xFF;
        if (*(s8 *) &em->senseDelay == 0 && em->noReact == 0 && hit != 0 && Em_ScriptFlagsFrom(em, 4) == 0) {
            em->triggers |= 4;
            em->scriptPending = 1;
        } else if ((s16) Player_IsState(em, 0, 0x18)) {
            em->senseDelay = saved;
        }
    }
    if (em->noAlert != 1 && em->rage == 0) {
        k = Area_BaseId(gScene.map);
        switch (em->type) {
        case 0x33:
            switch (k) {
            case 0xB:
                Em_Decay7F0Only(em);
                Em_CheckLowHp(em, 0.3f);
                break;
            }
            break;
        case 0x34:
            switch (k) {
            case 6:
                Em_Decay7F0(em);
                Em_Decay7EC(em);
                Em_CheckLowHp(em, 0.2f);
                break;
            case 4:
                Em_Decay7F0(em);
                Em_CheckLowHp(em, 0.2f);
                break;
            case 9:
            case 0xA:
                Em_Decay7F0Only(em);
                break;
            }
            break;
        }
    }
    ev = Em_UpdateDamage(em, buf) & 0xFF;
    switch (ev) {
    case 0:
    case 9:
    case 13:
    case 15:
    case 16:
        break;
    case 1:
    case 2:
        if (*(s8 *) &em->holdTimer != 0) {
            EmBlangonga_ChangeState(em, 5, 1, 3);
        } else if (EmBlangonga_InStateArmor(em) == 1) {
            EmBlangonga_ChangeState(em, 5, 2, 3);
        } else {
            switch (em->stance) {
            case 2:
                EmBlangonga_ChangeState(em, 5, 3, 3);
                break;
            default:
                EmBlangonga_ChangeState(em, 5, 0, 3);
                break;
            }
        }
        break;
    case 3:
    case 4:
        if (*(s8 *) &em->holdTimer == 0) {
            if (EmBlangonga_IsRajang(em) == 1 && em->rage == 0 && em->status == 0) {
                EmBlangonga_ChangeState(em, 4, 0x18, 3);
            } else {
                Em_SetGoal12Dist(em, buf);
                Em_SetGoal12(em);
                EmBlangonga_ChangeState(em, 4, 5, 3);
            }
        }
        break;
    case 5:
        if (em->stateGroup != 4 || em->state != 7) {
            EmBlangonga_ChangeState(em, 4, 7, 3);
        }
        break;
    case 6:
        if (em->stateGroup != 4 || em->state != 9) {
            if (*(s8 *) &em->holdTimer != 0) {
                Em_SetStatus4Timer(em);
                EmBlangonga_ChangeState(em, 4, 9, 3);
            } else if ((em->stateGroup != 4 || em->state != 4) && (em->stateGroup != 4 || em->state != 7)) {
                Em_SetStatus4Timer(em);
                EmBlangonga_ChangeState(em, 4, 4, 3);
            }
        }
        break;
    case 7:
        if (Em_IsHeld(em) == 1) {
            Em_SetStatus1Timer(em);
            if (EmBlangonga_IsBlangonga(em) != 1) {
                EmBlangonga_IsCongalala(em);
            }
            if (*(s8 *) &em->holdTimer != 0) {
                EmBlangonga_ChangeState(em, 6, 0, 3);
                break;
            }
            if (*(s8 *) &em->holdTimer2 != 0) {
                EmBlangonga_ChangeState(em, 6, 1, 3);
                break;
            }
        }
        if ((em->stateGroup != 0 || em->state != 0x25) && (em->stateGroup != 4 || em->state != 7)) {
            Em_SetStatus1Timer(em);
            EmBlangonga_ChangeState(em, 0, 0x25, 3);
        }
        break;
    case 8:
        if (*(s8 *) &em->holdTimer != 0) {
            Em_SetStatus2Timer(em);
            EmBlangonga_ChangeState(em, 0, 0x13, 3);
        } else if ((em->stateGroup != 0 || em->state != 5) && (em->stateGroup != 4 || em->state != 7)) {
            Em_SetStatus2Timer(em);
            EmBlangonga_ChangeState(em, 0, 5, 3);
        }
        break;
    case 10:
        Em_RestartScript(em);
        switch (em->state) {
        case 3:
            EmBlangonga_ChangeState(em, 0, 0x1A, 3);
            break;
        case 5:
            EmBlangonga_ChangeState(em, 0, 4, 3);
            break;
        case 0x25:
            EmBlangonga_ChangeState(em, 0, 0x26, 3);
            break;
        case 0x13:
            EmBlangonga_ChangeState(em, 4, 0xB, 3);
            break;
        case 0x24:
            EmBlangonga_ChangeState(em, 4, 0x13, 3);
            break;
        }
        break;
    case 11:
        EmBlangonga_ChangeState(em, 4, 0x14, 3);
        break;
    case 12:
        EmBlangonga_BreakParts(em, 1);
        switch (em->stance) {
        case 2:
            EmBlangonga_ChangeState(em, 4, 7, 3);
            break;
        default:
            sum = 0;
            if (EmBlangonga_IsCongalala(em) == 1) {
                if (em->type == 0x34) {
                    switch (em->flinchPart) {
                    case 4:
                        if (em->parts[em->flinchPart].state >= 2) {
                            if (EmBlangonga_PartFlagUnset(em, 1) == 1) {
                                EmBlangonga_ChangeState(em, 4, 0x16, 3);
                                goto done;
                            }
                            if (EmBlangonga_PartFlagUnset(em, 2) == 1) {
                                EmBlangonga_ChangeState(em, 4, 0x17, 3);
                                goto done;
                            }
                            if (EmBlangonga_PartFlagUnset(em, 0) == 1) {
                                EmBlangonga_ChangeState(em, 4, 0x15, 3);
                                goto done;
                            }
                        }
                    }
                }
                for (i = 0; i < 8; i++) {
                    sum += em->parts[i].state;
                }
                if (sum <= 3) {
                    EmBlangonga_ChangeState(em, 4, 0, 3);
                    break;
                }
                switch (em->flinchPart) {
                case 0:
                    if (EmBlangonga_IsSmallKind(em) == 0) {
                        EmBlangonga_ChangeState(em, 4, 0x12, 3);
                    } else {
                        EmBlangonga_ChangeState(em, 4, 1, 3);
                    }
                    break;
                case 2:
                    EmBlangonga_ChangeState(em, 4, 2, 3);
                    break;
                case 3:
                    EmBlangonga_ChangeState(em, 4, 3, 3);
                    break;
                case 4:
                default:
                    EmBlangonga_ChangeState(em, 4, 1, 3);
                    break;
                }
            } else {
                if (EmBlangonga_IsSmallKind(em) == 0) {
                    for (i = 0; i < 8; i++) {
                        sum += em->parts[i].state;
                    }
                    if (sum <= 3) {
                        EmBlangonga_ChangeState(em, 4, 0, 3);
                        break;
                    }
                }
                switch (em->flinchPart) {
                case 1:
                    EmBlangonga_ChangeState(em, 4, 3, 3);
                    break;
                case 2:
                    EmBlangonga_ChangeState(em, 4, 2, 3);
                    break;
                default:
                    EmBlangonga_ChangeState(em, 4, 1, 3);
                    break;
                }
            }
            break;
        }
        break;
    case 14:
        if (em->type == 0x3D) {
            if (em->stance == 0) {
                em->scriptPending = 0;
                EmBlangonga_ChangeState(em, 4, 0, 3);
            }
        } else if (em->type == 0x34 && em->stateGroup == 3 && em->state == 0xA && *(s8 *) &em->hitzoneSet == 1 &&
                   w->unk045 == 0) {
            Em_PlaySeChecked(em, 0x1B, 0, &em->posX, 6, 0);
            w->unk045 = 30;
        }
        break;
    case 18:
        Em_ResetKindTimer(em);
        EmBlangonga_ChangeState(em, 4, 0x10, 3);
        break;
    case 17:
        if ((em->stateGroup != 4 || em->state != 7) && (em->stateGroup != 4 || em->state != 0xE)) {
            if (*(s8 *) &em->holdTimer != 0) {
                Em_SetStatus5Timer(em);
                EmBlangonga_ChangeState(em, 4, 0xE, 3);
            } else if ((em->stateGroup != 4 || em->state != 0xC) && (em->stateGroup != 4 || em->state != 0xD)) {
                Em_SetStatus5Timer(em);
                if (((em->hitAngle - em->rotY) & 0xFFFF) > 0x8000) {
                    EmBlangonga_ChangeState(em, 4, 0xC, 3);
                } else {
                    EmBlangonga_ChangeState(em, 4, 0xD, 3);
                }
            }
        }
        break;
    }
done:
    if (func_00725820(em, ev) == 1) {
        EmBlangonga_BreakParts(em, 0);
    }
    switch (gQuest.stage) {
    case 0x3F3:
    case 0x3F4:
        if (em->type == 0x34 && Area_DayId(em->area) == 0x71 && Save_TestEventFlag(0x9B) == 0) {
            if (Area_DayId(gScene.area) == 0x71 && gScene.eventRunning == 1 && em->stateGroup != 6) {
                EmBlangonga_ChangeState(em, 6, 2, 1);
            }
            goto skip;
        }
        break;
    case 0x423:
    case 0x424:
        if ((em->type == 0x33 || em->type == 0x3D) && Area_DayId(em->area) == 0x61 && Save_TestEventFlag(0xA4) == 0) {
            if (Area_DayId(gScene.area) == 0x61 && gScene.eventRunning == 1 && em->stateGroup != 6) {
                EmBlangonga_ChangeState(em, 6, 3, 1);
            }
            goto skip;
        }
        break;
    }
    Em_UpdatePendingScript(em);
skip:
    Em_ClearFlag6AE(em);
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

void EmBlangonga_RunState(Em *em, s32 work)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->stateGroup) {
    case 0:
        EmBlangonga_RunGroup0(em, (EmWork *) work);
        break;
    case 1:
        EmBlangonga_RunGroup1(em, work);
        break;
    case 2:
        EmBlangonga_RunGroup2(em, (EmWork *) work);
        break;
    case 3:
        EmBlangonga_RunGroup3(em, (EmWork *) work);
        break;
    case 4:
        EmBlangonga_RunGroup4(em, (EmWork *) work);
        break;
    case 5:
        EmBlangonga_RunGroup5(em, work);
        break;
    case 6:
        EmBlangonga_RunGroup6(em, (EmWork *) work);
        break;
    case 7:
        EmBlangonga_RunGroup6(em, (EmWork *) work);
        break;
    case 8:
        EmBlangonga_RunGroup8(em, (EmWork *) work);
        
    default:
        break;
    }
}

void EmBlangonga_BreakParts(Em *em, s32 apply)
{
    u8 k;

    if (em->type == 0x33) {
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

void EmBlangonga_SpawnGroundEffect(Em *em, s32 a1, s32 a2, f32 scale)
{
    f32 pos[3];

    if (Player_InCurrentArea(em)) {
        Em_BonePos(em, a1, pos);
        pos[1] = em->floorY;
        Effect0D_SpawnOnGround(pos, a2, scale * em->scaleX);
    }
}

void EmBlangonga_SpawnK1CInMotions(Em *em, s16 motion)
{
    switch (motion) {
    case 0x3E9:
    case 0x3EB:
    case 0x3ED:
    case 0x3EE:
    case 0x3EF:
    case 0x3F0:
    case 0x3F1:
    case 0x412:
    case 0x413:
    case 0x414:
    case 0x41B:
    case 0x41C:
    case 0x41D:
    case 0x41E:
    case 0x426:
    case 0x43F:
        if (em->rage != 0) {
            if (EmBlangonga_IsRajang(em) == 1) {
                if ((gScene.frame & 0xF) == em->index * 2) {
                    StageObj_K1C_CreateOnEm(em, 0x27, 1.0f);
                }
            } else if (gScene.frame % 45 == em->index * 2) {
                StageObj_K1C_CreateOnEm(em, 7, em->scaleX);
            }
        }
        break;
    }
}

void EmBlangonga_RequestOnFrame(s32 em, s32 frame, s32 i)
{
    if (Player_PassedFrameScaled((Em *) em, (f32) frame, 0U) != 0) {
        CamShake_ReqAIf((void *) em, i);
    }
}

void EmBlangonga_MotionEffectsDefault(Em *em)
{
}

void StageObj_K14_Create(Em *em, s16 k, s16 c, f32 scale);
void StageObj_K1C_CreateAt(f32 *src, u8 k, f32 f);
void Vec_RotateY(f32 *v, f32 angle);
void EmBlangonga_MotionSounds(Em *em, EmWork *work);
void EmBlangonga_MotionSoundsCongalala(Em *em, EmWork *work);
void EmBlangonga_MotionSoundsRajang(Em *em, EmWork *work);

void EmBlangonga_MotionEffects(Em *em, EmWork *work)
{
    f32 ofs[3];
    f32 pos[3];

    if (em->motionId[0] != work->prevMotion) {
        work->prevMotion = em->motionId[0];
    }
    switch (em->type) {
    case 0x33:
    default:
    case 0x3D:
        EmBlangonga_MotionSounds(em, work);
        break;
    case 0x34:
    case 0x3E:
        EmBlangonga_MotionSoundsCongalala(em, work);
        break;
    case 0x35:
        EmBlangonga_MotionSoundsRajang(em, work);
        break;
    }
    EmBlangonga_SpawnK1CInMotions(em, work->prevMotion);
    switch (work->prevMotion) {
    case 0x3EF:
        if ((EmBlangonga_IsSmallKind(em) == 0) && (Player_PassedFrameScaled(em, 10.0f, 0) != 0)) {
            EffBlangonga27_Create(em, 0xE);
        }
        break;
    case 0x3F0:
        if ((EmBlangonga_IsSmallKind(em) == 0) && (Player_PassedFrameScaled(em, 10.0f, 0) != 0)) {
            EffBlangonga27_Create(em, 0x10);
        }
        break;
    case 0x3F2:
        if ((EmBlangonga_IsSmallKind(em) == 0) && (Player_PassedFrameScaled(em, 10.0f, 0) != 0)) {
            EffBlangonga27_Create(em, 0xB);
        }
        if (Player_PassedFrameScaled(em, 14.0f, 0) != 0) {
            StageObj_K14_Create((Em *) em, 2, 0xF, em->scaleX);
        }
        break;
    case 0x3F3:
        if (Player_PassedFrameScaled(em, 10.0f, 0) != 0) {
            EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
        }
        break;
    case 0x3F4:
        if ((EmBlangonga_IsSmallKind(em) == 0) && (Player_PassedFrameScaled(em, 18.0f, 0) != 0)) {
            EffBlangonga27_Create(em, 0xC);
        }
        if (Player_PassedFrameScaled(em, 14.0f, 0) != 0) {
            StageObj_K14_Create((Em *) em, 2, 0xF, em->scaleX);
        }
        break;
    case 0x3F5:
        if (Player_PassedFrameScaled(em, 10.0f, 0) != 0) {
            EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
        }
        break;
    case 0x3F6:
        if ((EmBlangonga_IsSmallKind(em) == 0) && (Player_PassedFrameScaled(em, 18.0f, 0) != 0)) {
            EffBlangonga27_Create(em, 0xD);
        }
        if (Player_PassedFrameScaled(em, 14.0f, 0) != 0) {
            StageObj_K14_Create((Em *) em, 3, 0x10, em->scaleX);
        }
        break;
    case 0x3F7:
        if (Player_PassedFrameScaled(em, 10.0f, 0) != 0) {
            EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
        }
        break;
    case 0x3F8:
        if (Player_PassedFrameScaled(em, 18.0f, 0) != 0) {
            StageObj_K14_Create((Em *) em, 2, 0x11, em->scaleX);
        }
        if (Player_PassedFrameScaled(em, 22.0f, 0) != 0) {
            StageObj_K14_Create((Em *) em, 3, 0x12, em->scaleX);
        }
        break;
    case 0x3F9:
        if (EmBlangonga_IsRajang(em) == 1) {
            if (Player_PassedFrameScaled(em, 48.0f, 0) != 0) {
                StageObj_K14_Create((Em *) em, 0x13, 0, em->scaleX);
                break;
            }
        } else if (EmBlangonga_IsCongalala(em) == 1) {
            if (Player_PassedFrameScaled(em, 54.0f, 0) != 0) {
                StageObj_K14_Create((Em *) em, 0x13, 0, em->scaleX);
                break;
            }
        } else if (Player_PassedFrameScaled(em, 50.0f, 0) != 0) {
            StageObj_K14_Create((Em *) em, 0x13, 0, em->scaleX);
        }
        break;
    case 0x3FB:
        if (EmBlangonga_IsCongalala(em) == 1) {
            if (Player_PassedFrameScaled(em, 14.0f, 0) != 0) {
                StageObj_K14_Create((Em *) em, 2, 0xF, em->scaleX);
            }
            if (Player_PassedFrameScaled(em, 42.0f, 0) != 0) {
                StageObj_K14_Create((Em *) em, 3, 0x10, em->scaleX);
                break;
            }
        } else {
            if (Player_PassedFrameScaled(em, 10.0f, 0) != 0) {
                StageObj_K14_Create((Em *) em, 3, 0x10, em->scaleX);
            }
            if (Player_PassedFrameScaled(em, 38.0f, 0) != 0) {
                StageObj_K14_Create((Em *) em, 2, 0x11, em->scaleX);
            }
            if (Player_PassedFrameScaled(em, 42.0f, 0) != 0) {
                StageObj_K14_Create((Em *) em, 3, 0x10, em->scaleX);
                break;
            }
        }
        break;
    case 0x3FC:
        if (EmBlangonga_IsRajang(em) == 1) {
            if (Player_PassedFrameScaled(em, 12.0f, 0) != 0) {
                StageObj_K14_Create((Em *) em, 0xB, 0, 0.6f * em->scaleX);
                break;
            }
        } else if (Player_PassedFrameScaled(em, 8.0f, 0) != 0) {
            EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
        }
        break;
    case 0x3FD:
        if (Player_PassedFrameScaled(em, 22.0f, 0) || Player_PassedFrameScaled(em, 92.0f, 0) || Player_PassedFrameScaled(em, 160.0f, 0) || Player_PassedFrameScaled(em, 210.0f, 0) || Player_PassedFrameScaled(em, 246.0f, 0)) {
            StageObj_K14_Create((Em *) em, 2, 0x11, em->scaleX);
        }
        if ((Player_PassedFrameScaled(em, 64.0f, 0) != 0) || (Player_PassedFrameScaled(em, 134.0f, 0) != 0) || (Player_PassedFrameScaled(em, 170.0f, 0) != 0) || (Player_PassedFrameScaled(em, 200.0f, 0) != 0)) {
            StageObj_K14_Create((Em *) em, 3, 0x12, em->scaleX);
        }
        if (Player_PassedFrameScaled(em, 254.0f, 0) != 0) {
            StageObj_K14_Create((Em *) em, 2, 0, em->scaleX);
        }
        if (Player_PassedFrameScaled(em, 42.0f, 0) != 0) {
            EffBlangonga27_Create(em, 0x1B);
        }
        break;
    case 0x3FE:
        if (EmBlangonga_IsSmallKind(em) == 0) {
            EmBlangonga_RequestOnFrame((s32) em, 4, 5);
        }
        if ((Player_PassedFrameScaled(em, 8.0f, 0) != 0) && Player_InCurrentArea(em)) {
            Em_BonePos(em, 1, pos);
            pos[1] = em->floorY;
            StageObj_K1C_CreateAt(pos, 6, 1.0f);
        }
        if (Player_PassedFrameScaled(em, 52.0f, 0) != 0) {
            EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
        }
        break;
    case 0x409:
        if (Player_PassedFrameScaled(em, 84.0f, 0) != 0) {
            EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
        }
        break;
    case 0x40A:
        if (Player_PassedFrameScaled(em, 170.0f, 0) != 0) {
            EmBlangonga_SpawnGroundEffect(em, 0x10, 7, 3.5f);
        }
        break;
    case 0x410:
        if (EmBlangonga_IsBlangonga(em) == 1) {
            if (Player_PassedFrameScaled(em, 70.0f, 0) != 0) {
                StageObj_K14_Create((Em *) em, 2, 0xF, em->scaleX);
            }
            if (Player_PassedFrameScaled(em, 92.0f, 0) != 0) {
                StageObj_K14_Create((Em *) em, 2, 0x11, em->scaleX);
            }
            if (Player_PassedFrameScaled(em, 104.0f, 0) != 0) {
                EmBlangonga_SpawnGroundEffect(em, 0x10, 7, 3.5f);
                break;
            }
        }
        break;
    case 0x411:
        if (EmBlangonga_IsCongalala(em) == 1) {
            if ((Player_PassedFrameScaled(em, 38.0f, 0) != 0) || (Player_PassedFrameScaled(em, 124.0f, 0) != 0)) {
                StageObj_K14_Create((Em *) em, 3, 0x22, em->scaleX);
            }
            if ((Player_PassedFrameScaled(em, 76.0f, 0) != 0) || (Player_PassedFrameScaled(em, 172.0f, 0) != 0)) {
                StageObj_K14_Create((Em *) em, 2, 0x21, em->scaleX);
            }
            if (Player_PassedFrameScaled(em, 200.0f, 0) != 0) {
                EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
                break;
            }
        }
        break;
    case 0x413:
        if (EmBlangonga_IsRajang(em) == 1) {
            if (Player_PassedFrameScaled(em, 48.0f, 0) != 0) {
                StageObj_K14_Create((Em *) em, 3, 0x22, em->scaleX);
            }
            if ((Player_PassedFrameScaled(em, 22.0f, 0) != 0) || (Player_PassedFrameScaled(em, 74.0f, 0) != 0)) {
                StageObj_K14_Create((Em *) em, 2, 0x21, em->scaleX);
                break;
            }
        }
        break;
    case 0x414:
        if ((EmBlangonga_IsRajang(em) == 1) && (Player_PassedFrameScaled(em, 30.0f, 0) != 0)) {
            EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
        }
        break;
    case 0x417:
        if ((Player_PassedFrameScaled(em, 102.0f, 0) != 0) && Player_InCurrentArea(em)) {
            Em_BonePos(em, 1, pos);
            pos[1] = em->floorY;
            ofs[0] = 0.0f;
            ofs[1] = 0.0f;
            ofs[2] = 330.0f * em->scaleX;
            Vec_RotateY(ofs, (6.2831855f * (f32) em->rotY) / 65536.0f);
            pos[0] += ofs[0];
            pos[1] += ofs[1];
            pos[2] += ofs[2];
            StageObj_K1C_CreateAt(pos, 6, 0.8f);
        }
        break;
    case 0x418:
        if (EmBlangonga_IsCongalala(em) == 1) {
            if (Player_PassedFrameScaled(em, 34.0f, 0) != 0) {
                StageObj_K14_Create((Em *) em, 3, 0x10, em->scaleX);
                break;
            }
        } else if (EmBlangonga_IsRajang(em) != 0) {
            if (Player_PassedFrameScaled(em, 30.0f, 0) != 0) {
                StageObj_K14_Create((Em *) em, 2, 0xF, em->scaleX);
            }
            if (Player_PassedFrameScaled(em, 56.0f, 0) != 0) {
                EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
                break;
            }
        } else {
            if (Player_PassedFrameScaled(em, 12.0f, 0) != 0) {
                StageObj_K14_Create((Em *) em, 2, 0xF, em->scaleX);
            }
            if (Player_PassedFrameScaled(em, 38.0f, 0) != 0) {
                EmBlangonga_SpawnGroundEffect(em, 0x10, 7, 3.5f);
                break;
            }
        }
        break;
    case 0x419:
        if ((EmBlangonga_IsCongalala(em) == 1) && (Player_PassedFrameScaled(em, 10.0f, 0) != 0)) {
            EmBlangonga_SpawnGroundEffect(em, 1, 0x28, 1.0f);
            if ((s32)Player_InCurrentArea(em) & 0xFF) {
                StageObj_K11_CreateAtScaled(&em->posX, (u16) em->rotY, 0xAU, 0.4f * em->scaleX);
                break;
            }
        }
        break;
    case 0x41C:
        if (EmBlangonga_IsBlangonga(em) == 1) {
            if (em->type == 0x33) {
                if ((em->hit8Count <= 0) && (Player_PassedFrameScaled(em, 38.0f, 0) != 0)) {
                    EffBlangonga27_Create(em, 0x17);
                    StageObj_K0F_CreateOnEm(em, 5, 2, 0.4f * em->scaleX);
                    break;
                }
            } else if (Player_PassedFrameScaled(em, 60.0f, 0) != 0) {
                EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
                break;
            }
        } else if (em->type == 0x34) {
            if ((gScene.eventRunning == 0) && (Player_PassedFrameScaled(em, 70.0f, 0) != 0)) {
                EffEm33_Create(em, 7, 0, 0, 0);
                break;
            }
        } else if ((EmBlangonga_IsRajang(em) == 1) && (Player_PassedFrameScaled(em, 38.0f, 0) != 0)) {
            EffBlangonga27_Create(em, 0x17);
            StageObj_K0F_CreateOnEm(em, 5, 2, 0.4f * em->scaleX);
        }
        break;
    case 0x421:
        if ((EmBlangonga_IsCongalala(em) == 1) && (Player_PassedFrameScaled(em, 66.0f, 0) != 0)) {
            EmBlangonga_SpawnGroundEffect(em, 2, 7, 3.5f);
        }
        break;
    case 0x423:
        if ((EmBlangonga_IsSmallKind(em) == 0) && (Player_PassedFrameScaled(em, 64.0f, 0) != 0)) {
            EffBlangonga27_Create(em, 0x17);
            StageObj_K0F_CreateOnEm(em, 5, 2, 0.4f * em->scaleX);
        }
        break;
    case 0x427:
        if (Player_PassedFrameScaled(em, 108.0f, 0) != 0) {
            EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
        }
        break;
    case 0x42B:
        if (Player_PassedFrameScaled(em, 10.0f, 0) != 0) {
            EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
        }
        break;
    case 0x42C:
        if (Player_PassedFrameScaled(em, 180.0f, 0) != 0) {
            EmBlangonga_SpawnGroundEffect(em, 0x10, 7, 3.5f);
        }
        break;
    case 0x42D:
        if ((Player_PassedFrameScaled(em, 28.0f, 0) != 0) || (Player_PassedFrameScaled(em, 78.0f, 0) != 0)) {
            EmBlangonga_SpawnGroundEffect(em, 4, 7, 3.5f);
        }
        break;
    case 0x42E:
        if ((Player_PassedFrameScaled(em, 28.0f, 0) != 0) || (Player_PassedFrameScaled(em, 78.0f, 0) != 0)) {
            EmBlangonga_SpawnGroundEffect(em, 4, 7, 3.5f);
        }
        break;
    case 0x42F:
        if (Player_PassedFrameScaled(em, 18.0f, 0) != 0) {
            EffBlangonga27_Create(em, 0x1C);
        }
        break;
    case 0x431:
        if (EmBlangonga_IsCongalala(em) == 1) {
            if (Player_PassedFrameScaled(em, 16.0f, 0) != 0) {
                EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
                break;
            }
        } else if (Player_PassedFrameScaled(em, 30.0f, 0) != 0) {
            EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
        }
        break;
    case 0x432:
        if ((EmBlangonga_IsCongalala(em) == 1) && (em->rage != 0) && ((Player_PassedFrameScaled(em, 2.0f, 0) != 0) || (Player_PassedFrameScaled(em, 40.0f, 0) != 0) || (Player_PassedFrameScaled(em, 80.0f, 0) != 0))) {
            StageObj_K1C_CreateOnEm(em, 0x14, 1.0f);
        }
        break;
    case 0x433:
        if (Player_PassedFrameScaled(em, 92.0f, 0) != 0) {
            EmBlangonga_SpawnGroundEffect(em, 4, 7, 3.5f);
        }
        break;
    case 0x435:
        if (Player_PassedFrameScaled(em, 108.0f, 0) != 0) {
            EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
        }
        break;
    case 0x437:
        if ((EmBlangonga_IsCongalala(em) == 1) && (Player_PassedFrameScaled(em, 30.0f, 0) != 0)) {
            EmBlangonga_SpawnGroundEffect(em, 0x10, 7, 3.5f);
        }
        break;
    case 0x438:
        if ((EmBlangonga_IsRajang(em) == 1) && (Player_PassedFrameScaled(em, 74.0f, 0) != 0)) {
            EffBlangonga27_Create(em, 0x28);
        }
        break;
    case 0x439:
        if ((EmBlangonga_IsCongalala(em) == 1) && (Player_PassedFrameScaled(em, 44.0f, 0) != 0)) {
            EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
        }
        break;
    case 0x43A:
        if (EmBlangonga_IsCongalala(em) == 1) {
            if (Player_PassedFrameScaled(em, 96.0f, 0) != 0) {
                EmBlangonga_SpawnGroundEffect(em, 0x10, 7, 3.5f);
                break;
            }
        } else {
            if ((EmBlangonga_IsRajang(em) == 1) && (Player_PassedFrameScaled(em, 26.0f, 0) != 0)) {
                StageObj_K14_Create((Em *) em, 0x13, 0, em->scaleX);
                break;
            }
        }
        break;
    case 0x43B:
        if (EmBlangonga_IsCongalala(em) == 1) {
            if (Player_PassedFrameScaled(em, 56.0f, 0) != 0) {
                EmBlangonga_SpawnGroundEffect(em, 0x10, 7, 3.5f);
            }
            if (Player_PassedFrameScaled(em, 90.0f, 0) != 0) {
                EmBlangonga_SpawnGroundEffect(em, 2, 7, 3.5f);
                break;
            }
        }
        break;
    case 0x43C:
        if ((EmBlangonga_IsRajang(em) == 1) && (Player_PassedFrameScaled(em, 2.0f, 0) != 0)) {
            StageObj_K14_Create((Em *) em, 0xB, 0, 0.6f * em->scaleX);
        }
        break;
    case 0x43D:
        if (EmBlangonga_IsBlangonga(em) == 1 || EmBlangonga_IsRajang(em) == 1) {
            if (Player_PassedFrameScaled(em, 44.0f, 0) != 0) {
                EffBlangonga27_Create(em, 9);
            }
            if (Player_PassedFrameScaled(em, 50.0f, 0) != 0) {
                EffBlangonga27_Create(em, 0x14);
            }
            if ((Player_PassedFrameScaled(em, 48.0f, 0) != 0) || (Player_PassedFrameScaled(em, 78.0f, 0) != 0)) {
                EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
                break;
            }
        }
        break;
    case 0x43E:
        if (EmBlangonga_IsBlangonga(em) == 1) {
            if ((Player_PassedFrameScaled(em, 60.0f, 0) != 0) || (Player_PassedFrameScaled(em, 94.0f, 0) != 0)) {
                StageObj_K14_Create((Em *) em, 3, 0x12, em->scaleX);
                break;
            }
        } else {
            if (EmBlangonga_IsRajang(em) == 1) {
                if (Player_PassedFrameScaled(em, 52.0f, 0) != 0) {
                    StageObj_K14_Create((Em *) em, 2, 0xF, em->scaleX);
                }
                if (Player_PassedFrameScaled(em, 62.0f, 0) != 0) {
                    StageObj_K14_Create((Em *) em, 3, 0x10, em->scaleX);
                }
                if (Player_PassedFrameScaled(em, 96.0f, 0) != 0) {
                    EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
                    break;
                }
            }
        }
        break;
    case 0x43F:
        if (EmBlangonga_IsRajang(em) == 1) {
            if (Player_PassedFrameScaled(em, 16.0f, 0) || Player_PassedFrameScaled(em, 52.0f, 0) || Player_PassedFrameScaled(em, 84.0f, 0)) {
                StageObj_K14_Create((Em *) em, 2, 0xF, em->scaleX);
            }
            if ((Player_PassedFrameScaled(em, 46.0f, 0) != 0) || (Player_PassedFrameScaled(em, 90.0f, 0) != 0)) {
                StageObj_K14_Create((Em *) em, 3, 0x10, em->scaleX);
                break;
            }
        }
        break;
    case 0x440:
        if (EmBlangonga_IsCongalala(em) == 1) {
            if (Player_PassedFrameScaled(em, 36.0f, 0) != 0) {
                EmBlangonga_SpawnGroundEffect(em, 0x10, 0x13, 1.0f);
            }
        } else {
            if (Player_PassedFrameScaled(em, 22.0f, 0) != 0) {
                EmBlangonga_SpawnGroundEffect(em, 0x10, 0x13, 1.0f);
            }
            if ((Player_PassedFrameScaled(em, 24.0f, 0) != 0) && Player_InCurrentArea(em)) {
                Em_BonePos(em, 0x10, pos);
                pos[1] = em->floorY += 20.0f;
                StageObj_K11_CreateAtScaled(pos, (u16) em->rotY, 0xAU, 0.6f * em->scaleX);
            }
        }
    case 0x442:
        if ((EmBlangonga_IsRajang(em) == 1) && (Player_PassedFrameScaled(em, 36.0f, 0) != 0)) {
            EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
        }
        break;
    default:
        EmBlangonga_MotionEffectsDefault(em);
        break;
    case 0x3E9:
    case 0x3EA:
    case 0x3EB:
    case 0x3ED:
    case 0x3EE:
    case 0x3F1:
    case 0x3FA:
    case 0x407:
    case 0x408:
    case 0x412:
    case 0x416:
    case 0x41B:
    case 0x41D:
    case 0x41E:
    case 0x41F:
    case 0x420:
    case 0x422:
    case 0x424:
    case 0x425:
    case 0x426:
    case 0x428:
    case 0x429:
    case 0x42A:
    case 0x434:
    case 0x436:
        break;
    }
}

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

void EmBlangonga_TableNop(void)
{
}

u8 EmBlangonga_ScriptQuery(Em *em, u8 kind)
{
    EmBlangongaWork *w = (EmBlangongaWork *)&em->work;
    Em *m;
    s16 i;

    switch (kind) {
    case 0:
        return w->unk018 >= 4;
    case 1:
        switch (Area_DayId(em->area)) {
        case 0x60:
        case 0x61:
        case 0x64:
            if (Player_InCurrentArea(em)) {
                if (*(u8 *) &em->floorAttr == 1) {
                    return 1;
                }
                return 0;
            }
            return 0;
        }
        return 0;
    case 2:
        return EmBlangonga_InStateArmor(em) == 1;
    case 3:
        return w->unk044;
    case 4:
        for (m = gEms, i = 0; i < 20; i++, m++) {
            if (m->active != 0 && m->visible != 0 && (Obj_SameArea(em, (Player *) m) & 0xFF) && m->type == 0x33 && Player_IsState(m, 0, 0x23)) {
                return 1;
            }
        }
        return 0;
    case 5:
        switch (Area_DayId(*(u16 *) &em->destArea)) {
        case 0x6F:
            return 0;
        case 0x70:
            return 1;
        case 0x71:
            return 2;
        case 0x74:
            return 3;
        case 0x77:
            return 4;
        case 0x78:
            return 5;
        case 0x99:
            return 6;
        case 0x9C:
            return 7;
        case 0x9D:
            return 8;
        case 0x9E:
            return 9;
        case 0x9F:
            return 10;
        case 0xA0:
            return 11;
        default:
            return 0xFF;
        }
    case 6:
        if (EmBlangonga_PartFlagUnset(em, 0) == 1) {
            return 1;
        }
        if (EmBlangonga_PartFlagUnset(em, 2) == 1) {
            return 2;
        }
        if (EmBlangonga_PartFlagUnset(em, 1) == 1) {
            return 3;
        }
        return 0;
    default:
        return 0;
    }
}

s32 EmBlangonga_AllowHit8(Em *em)
{
    if ((s8) em->hit8Count != 0) {
        return 0;
    }
    if (em->type == 0x33) {
        if (em->stance == 0) {
            return 1;
        }
    } else if (em->type == 0x35 && em->rage != 0 && em->stance == 0) {
        return 1;
    }
    return 0;
}

void EmShenGaoren_SetGroup0State(Em *em, u16 state, u8 mode)
{
    em->motionSpeed = 1.0f;
    switch (state) {
    case 3:
        em->stance = 0;
        break;
    case 4:
        em->stance = 1;
        break;
    }
    Em_SetStateSync(em, 0, state, mode);
}

void EmShenGaoren_SetGroup1State(Em *em, u16 state, u8 mode)
{
    s32 a;

    em->motionSpeed = 1.0f;
    switch (state) {
    case 0:
    case 2:
        if (em->curGoalKind == 0) {
            em->hasGoal = 0;
            em->goalDist = 500.0f;
        } else {
            em->hasGoal = 1;
            em->goalDist = Vec_DistanceXZ(&em->posX, em->goal);
        }
        break;
    case 1:
        if (em->curGoalKind == 0) {
            em->hasGoal = 0;
            em->goalDist = 500.0f;
        } else {
            em->hasGoal = 1;
            em->goalDist = Vec_DistanceXZ(&em->posX, em->goal);
        }
        a = (u16) (Math_AngleXZ(&em->posX, em->goal) - em->rotY);
        if ((0 <= a && a <= 0x4000) || (a >= 0xC000 && a <= 0xFFFF)) {
            state = 0;
        }
        break;
    case 3:
        if (em->curGoalKind == 0) {
            em->hasGoal = 0;
            em->goalDist = 500.0f;
        } else {
            em->hasGoal = 1;
            em->goalDist = Vec_DistanceXZ(&em->posX, em->goal);
        }
        a = (u16) (Math_AngleXZ(&em->posX, em->goal) - em->rotY);
        if ((0 <= a && a <= 0x4000) || (a >= 0xC000 && a <= 0xFFFF)) {
            state = 2;
        }
        break;
    case 4:
    case 5:
    case 6:
    case 7:
        Math_AngleXZ(&em->posX, em->goal);
        break;
    }
    Em_SetStateSync(em, 1, state, mode);
}

void EmShenGaoren_SetGroup2State(Em *em, u16 state, u8 mode)
{
    em->motionSpeed = 1.0f;
    Em_SetStateSync(em, 0x2, state, mode);
}

void EmShenGaoren_SetGroup3State(Em *em, u16 state, u8 mode)
{
    em->motionSpeed = 1.0f;
    Em_SetStateSync(em, 0x3, state, mode);
}

void EmShenGaoren_SetGroup4State(Em *em, u16 state, u8 mode)
{
    EmShenGaorenWork *w = (EmShenGaorenWork *)&em->work;
    em->motionSpeed = 1.0f;
    switch (state) {
    case 3:
    case 5:
    case 4:
    case 2:
        w->partFlinch[state + 1] = 1;
        break;
    case 6:
    case 8:
        w->partFlinch[3] = 0;
        w->partFlinch[4] = 0;
        w->partFlinch[5] = 0;
        w->partFlinch[6] = 0;
        Em_BreakPart(em, 1U);
        break;
    case 9:
    case 10:
    case 11:
    case 12:
        w->partFlinch[state - 6] = 1;
        break;
    case 17:
    case 18:
    case 19:
    case 20:
        w->partFlinch[state - 14] = 2;
        break;
    case 0x0:
        break;
    }
    Em_SetStateSync(em, 4, state, mode);
}

void EmShenGaoren_ChangeState(Em *em, u16 group, u16 state, u8 mode)
{
    if (em->remote == 0) {
        Em_TargetPlayerInRange(em);
        goto block_5;
    }
    if ((mode == 3) || (mode == 4)) {
        return;
    }
block_5:
    switch (group) {
    case 0:
        EmShenGaoren_SetGroup0State(em, state, mode);
        return;
    case 1:
        EmShenGaoren_SetGroup1State(em, state, mode);
        return;
    case 2:
        EmShenGaoren_SetGroup2State(em, state, mode);
        return;
    case 3:
        EmShenGaoren_SetGroup3State(em, state, mode);
        return;
    case 4:
        EmShenGaoren_SetGroup4State(em, state, mode);
        return;
    case 5:
    case 6:
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        
    default:
        return;
    }
}

void EmKushala_SetWind(void *unused, s32 w, s32 on)
{
    if (on == 1) {
        *(s8 *)((u8 *)w + 8) = 1;
        *(s16 *)((u8 *)w + 0xA) = 0x5A;
        return;
    }
    *(s8 *)((u8 *)w + 8) = 0;
    *(s16 *)((u8 *)w + 0xA) = 0;
}

s32 EmKushala_WindActive(Em *em)
{
    EmKushalaWork *w = (EmKushalaWork *)&em->work;

    if ((em->blindTimer > 0) || (em->poisonDamage > 0) || (em->hitReactMode == 4) || ((em->stateGroup == 4) && (em->state == 0xC))) {
        return 0;
    }
    if (Em_PartBroken(em, 2) == 1) {
        return 0;
    }
    if (Em_PartBroken(em, 0) == 1) {
        return 0;
    }
    return w->windOn != 0;
}

s32 EmKushala_WindActive2(Em *em)
{
    EmKushalaWork *w = (EmKushalaWork *)&em->work;

    if ((em->blindTimer > 0) || (em->poisonDamage > 0) || (em->hitReactMode == 4) || ((em->stateGroup == 4) && (em->state == 0xC))) {
        return 0;
    }
    if (Em_PartBroken(em, 2) == 1) {
        return 0;
    }
    if (Em_PartBroken(em, 0) == 1) {
        return 0;
    }
    return w->windOn != 0;
}

void EmKushala_SetGroup0State(Em *em, u16 state, u8 mode)
{
    EmKushalaWork *w = (EmKushalaWork *)&em->work;

    em->motionSpeed = 1.0f;
    switch (state) {
    case 1:
        if (em->combat == 1) {
            state = 3;
        }
        break;
    case 9:
        Em_ClearPart(em, 0);
        em->parts[0].state = 0;
        Em_ResetNotice(em);
        EmKushala_SetWind(em, (s32) w, 0);
        break;
    case 11:
        Em_BeginStatus2(em);
        EmKushala_SetWind(em, (s32) w, 0);
        break;
    case 21:
        Em_BeginStatus1(em);
        EmKushala_SetWind(em, (s32) w, 0);
        break;
    case 5:
        Em_ClearPart(em, 0);
        em->parts[0].state = 0;
        Em_AddHp(em, 0.05f * em->hpMax);
        Em_Add7F0(em, em->need1Max);
        Em_Clear7F0State(em);
        break;
    case 7:
        Em_EndStatus4(em);
        break;
    case 10:
        Em_Refill7F4(em);
        break;
    case 12:
        Em_EndStatus2(em);
        break;
    case 14:
        Em_StartGoalAction(em, 10000);
        break;
    case 22:
        Em_EndStatus1(em);
        break;
    case 24:
        Em_EndStatus5(em);
        break;
    case 26:
        Em_AddHp(em, 0.05f * em->hpMax);
        Em_Add7EC(em, em->need2Max);
        Em_Clear7ECState(em);
        break;
    case 20:
        if (em->remote == 0 && w->windOn != 0) {
            if (em->type == 0x36 || em->type == 0x3C) {
                EmKushala_ChangeState(em, 3, 1, 1);
                return;
            }
            state = 3;
            break;
        }
        EmKushala_SetWind(em, (s32) w, 1);
        break;
    case 15:
        EmKushala_SetWind(em, (s32) w, 1);
        break;
    case 16:
        em->spawnParam = 1;
        EmKushala_SetWind(em, (s32) w, 1);
        break;
    case 17:
        em->spawnParam = 2;
        EmKushala_SetWind(em, (s32) w, 1);
        break;
    case 0:
    case 2:
    case 3:
    case 4:
    case 6:
    case 8:
    case 13:
    case 18:
    case 19:
    case 23:
    case 25:
    case 27:
        break;
    }
    Em_SetStateSync(em, 0, state, mode);
}

void EmKushala_SetGroup1State(Em *em, u16 state, u8 mode)
{
    EmKushalaWork *w = (EmKushalaWork *)&em->work;
    f32 v[3];
    s32 rot[3];
    f32 m[16];
    f32 f;

    em->motionSpeed = 1.0f;
    switch (state) {
    case 0:
    case 1:
        if (em->type == 0x36 || em->type == 0x3C) {
            f = 0.12f;
        } else {
            f = 0.15f;
        }
        if (em->hp < (s16) (em->hpMax * f)) {
            state = 1;
        }
        Em_SetSpeedScaled(em, 0, 1000.0f, 500.0f);
        break;
    case 23:
    case 29:
        Em_SetSpeedScaled(em, 0, 1000.0f, 500.0f);
        break;
    case 24:
        Em_SetSpeedScaled(em, 0, 1000.0f, 800.0f);
        break;
    case 28:
        Em_SetSpeedScaled(em, 0, 1000.0f, -200.0f);
        break;
    case 2:
        if (em->combat == 1) {
            state = 3;
        }
        break;
    case 26:
        if (em->combat == 1) {
            state = 30;
        }
        break;
    case 21:
    case 22:
        if (em->remote == 0) {
            em->timer = Vec_DistanceXZ(&em->posX, em->goal);
            Vec_Copy(w->dashGoal, em->goal);
        } else {
            rot[0] = 0;
            rot[1] = em->rotY;
            rot[2] = 0;
            Vec_Set(v, 0.0f, 0.0f, em->timer);
            Mtx_FromAnglesYXZ(rot, m);
            Vec_Rotate(w->dashGoal, v, m);
            w->dashGoal[0] += em->posX;
            w->dashGoal[1] += em->posY;
            w->dashGoal[2] += em->posZ;
        }
        break;
    case 4:
    case 5:
        Em_SetSpeedScaled(em, 0, 1000.0f, 500.0f);
        break;
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
        Em_SetSpeedScaled(em, 0, 1000.0f, -700.0f);
        break;
    case 3:
    case 6:
    case 7:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
    case 25:
    case 27:
    case 30:
        break;
    }
    Em_SetStateSync(em, 1, state, mode);
}

void EmKushala_SetGroup2State(Em *em, u16 state, u8 mode)
{
    EmKushalaWork *w = (EmKushalaWork *)&em->work;

    em->motionSpeed = 1.0f;
    switch (state) {
    case 17:
        if (em->remote == 0 && em->curGoalKind == 0) {
            EmKushala_ChangeState(em, 2, 1, 1);
            return;
        }
        break;
    case 1:
        if (em->combat == 1) {
            state = 14;
        }
        break;
    case 4:
    case 5:
        Player_ClearVelocity(em);
        em->velZ = 20.0f;
        em->timer = (s32) (Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) / em->velZ) * 3;
        break;
    case 11:
        w->legsLeft = 1;
        if (em->remote == 0) {
            em->timer = 1.5f * Vec_DistanceXZ(&em->posX, em->goal) / 50.0f;
        }
        break;
    case 13:
        w->legsLeft = 4;
        if (em->remote == 0) {
            em->goalNo = em->stateRand & 3;
            em->goalKind = 2;
            em->goalArg = 2;
            Em_ComputeGoal(em, em->goal);
            em->timer = 1.5f * Vec_DistanceXZ(&em->posX, em->goal) / 50.0f;
        }
        break;
    case 10:
        Em_TypeSetup(em);
    case 8:
        if (em->remote == 0 && em->curGoalKind != 3) {
            EmKushala_ChangeState(em, 2, 1, 1);
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
    case 0:
    case 2:
    case 3:
    case 6:
    case 7:
    case 12:
    case 14:
    case 15:
    case 16:
        break;
    }
    Em_SetStateSync(em, 2, state, mode);
}

void EmKushala_SetGroup3State(Em *em, u16 state, u8 mode)
{
    em->motionSpeed = 1.0f;
    switch ((u16) state) {
    case 0xB:
        Em_SetSpeedScaled(em, 0, 1000.0f, 520.0f);
        break;
    }
    Em_SetStateSync(em, 3, state, mode);
}

void EmKushala_SetGroup4State(Em *em, u16 state, u8 mode)
{
    u8 *w;

    em->motionSpeed = 1.0f;
    w = &em->work;
    switch (state) {
    case 5:
        Em_BeginStatus4(em);
        break;
    case 6:
        EmKushala_SetWind(em, (s32) w, 0);
        em->fxFlags |= 1;
        em->hit8Count = (s8) em->hit8Count + 1;
        em->meshShown[1] = 0;
        Em_EndAllStatus();
        Em_BreakPart(em, 2U);
        break;
    case 12:
        Em_BeginStatus5(em);
        break;
    case 3:
    case 8:
    case 11:
        EmKushala_SetWind(em, (s32) w, 0);
        break;
    case 13:
    case 14:
        Em_EndAllStatus(em);
        break;
    case 15:
        em->hit8Count = (s8) em->hit8Count + 1;
        break;
    }
    Em_SetStateSync(em, 4, state, mode);
}

void EmKushala_ChangeState(Em *em, u16 group, u16 state, u8 mode)
{
    if (em->remote == 0) {
        Em_TargetPlayerInRange(em);
        switch (em->stance) {
        case 1:
            switch (group) {
            case 0:
                switch (state) {
                case 1:
                case 3:
                case 19:
                    state = 2;
                    break;
                }
                break;
            case 1:
                if (state < 20) {
                    state = 2;
                    group = 0;
                }
                break;
            case 2:
                if (state != 15 && state != 16 && state != 17) {
                    state = 1;
                }
                break;
            }
            break;
        case 0:
            switch (group) {
            case 0:
                switch (state) {
                case 2:
                    state = 1;
                    break;
                }
                break;
            case 1:
                if (state >= 21) {
                    state = 1;
                    group = 0;
                }
                break;
            case 2:
                if (state >= 2) {
                    state = 1;
                }
                break;
            }
            break;
        case 2:
            switch (group) {
            case 0:
                switch (state) {
                case 1:
                case 2:
                case 3:
                case 19:
                    group = 2;
                    state = 1;
                    break;
                }
                break;
            case 1:
                group = 2;
                state = 1;
                break;
            case 2:
                break;
            }
            break;
        }
    } else if (mode == 3 || mode == 4) {
        return;
    }
    Em_LoadReactionSet(em, 0);
    switch (group) {
    case 0:
        EmKushala_SetGroup0State(em, state, mode);
        break;
    case 1:
        EmKushala_SetGroup1State(em, state, mode);
        break;
    case 2:
        EmKushala_SetGroup2State(em, state, mode);
        break;
    case 3:
        EmKushala_SetGroup3State(em, state, mode);
        break;
    case 4:
        EmKushala_SetGroup4State(em, state, mode);
        break;
    case 5:
        if (state == 3 && QuestEm_CanAppear(em) == 1) {
            QuestEm_Appear(em);
        }
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        break;
    case 6:
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        break;
    }
}

void EmKushala_StartCurves(Em *em, u8 kind)
{
    EmKushalaWork *w = (EmKushalaWork *)&em->work;

    w->curveNone = 0;
    w->curveVelY = 0;
    w->curveSpeed = 0;
    w->curveKind = kind;
    switch (w->curveKind) {
    case 0:
        w->curveVelY = 1;
        w->curveFrame = 0x30;
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
    }
    Player_ClearVelocity(em);
}

static s32 EmKushala_CurveNone(Em *em, EmWork *work)
{
    return 0;
}


extern EmCurveKey *EmKushala_VelYCurves[];

/* The value of curve work[0x4F] at the frame in the work area (a table of keys): sets
 * em->velY from the key pair around the frame, 2 at the table's end. */
static u8 EmKushala_CurveVelY(Em *em, EmWork *work)
{
    EmKushalaWork *w = (EmKushalaWork *) work;
    EmCurveKey *p;
    f32 frame;
    f32 t;
    s32 i;
    EmCurveKey *tbl;
    u8 ret;

    tbl = EmKushala_VelYCurves[((EmKushalaWork *)work)->curveKind];
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

extern EmCurveKey *EmKushala_SpeedCurves[];

/* The value of curve work[0x4F] at the frame in the work area: sets em->velZ from the key
 * pair around the frame, 4 at the table's end. */
static u8 EmKushala_CurveSpeed(Em *em, EmWork *work)
{
    EmKushalaWork *w = (EmKushalaWork *) work;
    EmCurveKey *p;
    f32 frame;
    f32 t;
    s32 i;
    EmCurveKey *tbl;
    u8 ret;

    tbl = EmKushala_SpeedCurves[((EmKushalaWork *)work)->curveKind];
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
 * the curve frame by the motion's step, and turns toward the target while unk390 is 1 or 2. */
u8 EmKushala_UpdateCurves(Em *em)
{
    EmKushalaWork *w = (EmKushalaWork *)&em->work;
    u8 ret = 0;

    if (w->curveNone != 0) {
        ret = EmKushala_CurveNone(em, (EmWork *) w);
    }
    if (w->curveVelY != 0) {
        ret |= EmKushala_CurveVelY(em, (EmWork *) w);
    }
    if (w->curveSpeed != 0) {
        ret |= EmKushala_CurveSpeed(em, (EmWork *) w);
    }
    w->curveFrame += (s16) em->motion[0].step;
    if (em->stance == 2 || em->stance == 1) {
        w->moveAngX = 0;
        w->moveAngY = em->rotY;
        w->moveAngZ = 0;
        Em_MoveRotatedAccel(em, &w->moveAngX);
    }
    return ret;
}

void EmShenGaoren_PlaySeAtOffset(Em *em, s32 se, s32 idx, s32 vol)
{
    f32 ofs[3];
    f32 *p;
    f32 rot[3];
    f32 pos[3];

    p = &EmShenGaoren_SeOffsets[idx].x;
    ofs[0] = *p++;
    ofs[1] = *p++;
    ofs[2] = *p;
    Em_BonePos(em, 0xE, pos);
    Vec_Rotate(rot, ofs, (f32 *)Em_Bone(em, 0xE));
    pos[0] = pos[0] + rot[0];
    pos[1] = pos[1] + rot[1];
    pos[2] = pos[2] + rot[2];
    Em_PlaySeChecked(em, se, 0, pos, vol, 0);
}

void EmShenGaoren_PlaySeOnFrame(s32 em, s32 frame, s32 se, s32 idx, s32 vol)
{
    if (Player_PassedFrameScaled((Em *) em, (f32) frame, 0U) != 0) {
        EmShenGaoren_PlaySeAtOffset((Em *) em, se, idx, vol);
    }
}

void EmShenGaoren_MotionSounds(Em *em, EmWork *work)
{
    s16 motion;

    motion = ((EmShenGaorenWork *)work)->fxMotion;
    switch (motion) {
    case 0x3E9:
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x3A, 4, 0x3B, 4), 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0xB8, Sound_RandomSe(0x3C, 4, 0x3A, 4), 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x162, Sound_RandomSe(0x3B, 4, 0x3C, 4), 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x1A4, Sound_RandomSe(0x3A, 4, 0x3B, 4), 0x1F);
        break;
    case 0x3EA:
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x3A, 4, 0x3B, 4), 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0xB8, Sound_RandomSe(0x3C, 4, 0x3A, 4), 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x162, Sound_RandomSe(0x3B, 4, 0x3C, 4), 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, Sound_RandomSe(4, 4, 5, 4), 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x96, Sound_RandomSe(6, 4, 7, 4), 0x16);
        Em_PlaySeAtBoneOnFrame(em, 0xFC, Sound_RandomSe(4, 4, 5, 4), 0x12);
        Em_PlaySeAtBoneOnFrame(em, 0x164, Sound_RandomSe(6, 4, 7, 4), 0x1E);
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(8, 4, 9, 4), 0x19);
        Em_PlaySeAtBoneOnFrame(em, 0x6E, Sound_RandomSe(0xA, 4, 0xB, 4), 0x15);
        Em_PlaySeAtBoneOnFrame(em, 0xD4, Sound_RandomSe(8, 4, 9, 4), 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x13C, Sound_RandomSe(0xA, 4, 0xB, 4), 0x1D);
        Em_PlaySeAtBoneVolOnFrame(em, 0x2C, 0x64, 0x1A, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x96, 0x64, 0x16, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xFC, 0x64, 0x12, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x164, 0x64, 0x1E, 0x16);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, Sound_RandomSe(0xC, 4, 0xD, 4), 2);
        Em_PlaySeAtBoneOnFrame(em, 0x96, Sound_RandomSe(0xE, 4, 0xF, 4), 2);
        Em_PlaySeAtBoneOnFrame(em, 0xFC, Sound_RandomSe(0xC, 4, 0xD, 4), 2);
        Em_PlaySeAtBoneOnFrame(em, 0x164, Sound_RandomSe(0xE, 4, 0xF, 4), 2);
        break;
    case 0x3EB:
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x3A, 4, 0x3B, 4), 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0xB8, Sound_RandomSe(0x3C, 4, 0x3A, 4), 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x162, Sound_RandomSe(0x3B, 4, 0x3C, 4), 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x5E, Sound_RandomSe(6, 4, 7, 4), 0x1E);
        Em_PlaySeAtBoneOnFrame(em, 0xC2, Sound_RandomSe(4, 4, 5, 4), 0x12);
        Em_PlaySeAtBoneOnFrame(em, 0x12E, Sound_RandomSe(6, 4, 7, 4), 0x16);
        Em_PlaySeAtBoneOnFrame(em, 0x196, Sound_RandomSe(4, 4, 5, 4), 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x36, Sound_RandomSe(0xA, 4, 0xB, 4), 0x1D);
        Em_PlaySeAtBoneOnFrame(em, 0x9A, Sound_RandomSe(8, 4, 9, 4), 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x106, Sound_RandomSe(0xA, 4, 0xB, 4), 0x15);
        Em_PlaySeAtBoneOnFrame(em, 0x16E, Sound_RandomSe(8, 4, 9, 4), 0x1D);
        Em_PlaySeAtBoneVolOnFrame(em, 0x5E, 0x64, 0x1A, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xC2, 0x64, 0x16, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x12E, 0x64, 0x12, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x196, 0x64, 0x1E, 0x16);
        Em_PlaySeAtBoneOnFrame(em, 6, Sound_RandomSe(0xE, 4, 0xF, 4), 2);
        Em_PlaySeAtBoneOnFrame(em, 0x72, Sound_RandomSe(0xC, 4, 0xD, 4), 2);
        Em_PlaySeAtBoneOnFrame(em, 0xD6, Sound_RandomSe(0xE, 4, 0xF, 4), 2);
        Em_PlaySeAtBoneOnFrame(em, 0x142, Sound_RandomSe(0xC, 4, 0xD, 4), 2);
        break;
    case 0x3EC:
        Em_PlaySeAtBoneOnFrame(em, 0x26, Sound_RandomSe(4, 4, 5, 4), 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x8E, Sound_RandomSe(6, 4, 7, 4), 0x16);
        Em_PlaySeAtBoneOnFrame(em, 0xF6, Sound_RandomSe(4, 4, 5, 4), 0x12);
        Em_PlaySeAtBoneOnFrame(em, 0x15E, Sound_RandomSe(6, 4, 7, 4), 0x1E);
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(8, 4, 9, 4), 0x19);
        Em_PlaySeAtBoneOnFrame(em, 0x7C, Sound_RandomSe(0xA, 4, 0xB, 4), 0x15);
        Em_PlaySeAtBoneOnFrame(em, 0xD4, Sound_RandomSe(8, 4, 9, 4), 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x13C, Sound_RandomSe(0xA, 4, 0xB, 4), 0x1D);
        Em_PlaySeAtBoneVolOnFrame(em, 0x26, 0x64, 0x1A, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x8E, 0x64, 0x16, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xF6, 0x64, 0x12, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x15E, 0x64, 0x1E, 0x16);
        Em_PlaySeAtBoneOnFrame(em, 0x32, Sound_RandomSe(0xE, 4, 0xF, 4), 2);
        Em_PlaySeAtBoneOnFrame(em, 0x96, Sound_RandomSe(0xC, 4, 0xD, 4), 2);
        Em_PlaySeAtBoneOnFrame(em, 0xF6, Sound_RandomSe(0xE, 4, 0xF, 4), 2);
        Em_PlaySeAtBoneOnFrame(em, 0x168, Sound_RandomSe(0xC, 4, 0xD, 4), 2);
        break;
    case 0x3ED:
        Em_PlaySeAtBoneOnFrame(em, 0x26, Sound_RandomSe(4, 4, 5, 4), 0x12);
        Em_PlaySeAtBoneOnFrame(em, 0x8E, Sound_RandomSe(6, 4, 7, 4), 0x1E);
        Em_PlaySeAtBoneOnFrame(em, 0xF6, Sound_RandomSe(4, 4, 5, 4), 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x15E, Sound_RandomSe(6, 4, 7, 4), 0x16);
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(8, 4, 9, 4), 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x7C, Sound_RandomSe(0xA, 4, 0xB, 4), 0x1D);
        Em_PlaySeAtBoneOnFrame(em, 0xD4, Sound_RandomSe(8, 4, 9, 4), 0x19);
        Em_PlaySeAtBoneOnFrame(em, 0x13C, Sound_RandomSe(0xA, 4, 0xB, 4), 0x1D);
        Em_PlaySeAtBoneVolOnFrame(em, 0x26, 0x64, 0x12, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x8E, 0x64, 0x1E, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xF6, 0x64, 0x1A, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x15E, 0x64, 0x16, 0x16);
        Em_PlaySeAtBoneOnFrame(em, 0x32, Sound_RandomSe(0xE, 4, 0xF, 4), 2);
        Em_PlaySeAtBoneOnFrame(em, 0x96, Sound_RandomSe(0xC, 4, 0xD, 4), 2);
        Em_PlaySeAtBoneOnFrame(em, 0xF6, Sound_RandomSe(0xE, 4, 0xF, 4), 2);
        Em_PlaySeAtBoneOnFrame(em, 0x168, Sound_RandomSe(0xC, 4, 0xD, 4), 2);
        break;
    case 0x3EE:
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0x51, 0x1F, 9);
        Em_PlaySeAtBoneVolOnFrame(em, 0xD0, 0x52, 0x1F, 9);
        Em_PlaySeAtBoneVolOnFrame(em, 0x17C, 0x53, 0x1F, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 0x11, 2);
        Em_PlaySeAtBoneOnFrame(em, 0xBE, 0x13, 2);
        Em_PlaySe6AtBoneInFrames(em, 0xA, 0x168, 0x14, 2, 8, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x152, 0xC, 0x18);
        Em_PlaySeAtBoneOnFrame(em, 0x152, 0xF, 0x10);
        Em_PlaySeAtBoneOnFrame(em, 0x168, 9, 0x1C);
        Em_PlaySeAtBoneOnFrame(em, 0x168, 0xA, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x46, Sound_RandomSe(4, 4, 5, 4), 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x8C, Sound_RandomSe(4, 4, 5, 4), 0x12);
        Em_PlaySeAtBoneOnFrame(em, 0xD2, Sound_RandomSe(6, 4, 7, 4), 0x1E);
        Em_PlaySeAtBoneOnFrame(em, 0x118, Sound_RandomSe(6, 4, 7, 4), 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x46, 0x64, 0x1A, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x8C, 0x64, 0x12, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xD2, 0x64, 0x1E, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x118, 0x64, 0x16, 0x16);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, Sound_RandomSe(0x17, 4, 0x18, 4), 0x19);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, Sound_RandomSe(0x17, 4, 0x18, 4), 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0xBA, Sound_RandomSe(0x17, 4, 0x18, 4), 0x1D);
        Em_PlaySeAtBoneOnFrame(em, 0x104, Sound_RandomSe(0x17, 4, 0x18, 4), 0x15);
        Em_PlaySeAtBoneOnFrame(em, 0x170, Sound_RandomSe(0x15, 4, 0x16, 4), 0x18);
        Em_PlaySeAtBoneOnFrame(em, 0x1B8, Sound_RandomSe(0x15, 4, 0x16, 4), 0x10);
        Em_PlaySeAtBoneOnFrame(em, 0xAE, Sound_RandomSe(0x15, 4, 0x16, 4), 2);
        Em_PlaySeAtBoneOnFrame(em, 0x48, Sound_RandomSe(8, 4, 9, 4), 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x8E, Sound_RandomSe(0xA, 4, 0xB, 4), 0x1D);
        Em_PlaySeAtBoneOnFrame(em, 0xD4, Sound_RandomSe(8, 4, 9, 4), 0x19);
        Em_PlaySeAtBoneOnFrame(em, 0x11A, Sound_RandomSe(0xA, 4, 0xB, 4), 0x11);
        break;
    case 0x3F2:
        Em_PlaySeAtBoneVolOnFrame(em, 0x28, 0x50, 0x1F, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x13, 0xE);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0x98, 0x11, 0, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x132, Sound_RandomSe(0xE, 4, 0xF, 4), 2);
        Em_PlaySeAtBoneOnFrame(em, 0x12, Sound_RandomSe(8, 4, 9, 4), 0x19);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, Sound_RandomSe(0, 4, 1, 4), 0x1A);
        Em_PlaySeAtBoneVolOnFrame(em, 0x3A, 0x64, 0x1A, 0x16);
        Em_PlaySeAtBoneOnFrame(em, 0x28, Sound_RandomSe(0xC, 4, 0xD, 4), 9);
        Em_PlaySeAtBoneOnFrame(em, 0x8C, 0x19, 0xA);
        Em_PlaySeAtBoneVolOnFrame(em, 0xA0, 0x27, 0xA, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x114, Sound_RandomSe(0xC, 4, 0xD, 4), 9);
        break;
    case 0x3F3:
        Em_PlaySeAtBoneVolOnFrame(em, 0x28, 0x50, 0x1F, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x13, 0xE);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0x98, 0x11, 0, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x132, Sound_RandomSe(0xE, 4, 0xF, 4), 2);
        Em_PlaySeAtBoneOnFrame(em, 0x12, Sound_RandomSe(8, 4, 9, 4), 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, Sound_RandomSe(0, 4, 1, 4), 0x12);
        Em_PlaySeAtBoneVolOnFrame(em, 0x3A, 0x64, 0x12, 0x16);
        Em_PlaySeAtBoneOnFrame(em, 0x28, Sound_RandomSe(0xC, 4, 0xD, 4), 5);
        Em_PlaySeAtBoneOnFrame(em, 0x8C, 0x19, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0xA0, 0x27, 6, 9);
        Em_PlaySeAtBoneVolOnFrame(em, 0xA0, 0x64, 0x12, 0x16);
        Em_PlaySeAtBoneOnFrame(em, 0x114, Sound_RandomSe(0xC, 4, 0xD, 4), 5);
        break;
    case 0x3F4:
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0x51, 0x1F, 9);
        Em_PlaySeAtBoneVolOnFrame(em, 0x28, 0x54, 0x1F, 9);
        Em_PlaySeAtBoneVolOnFrame(em, 0x154, 0x53, 0x1F, 9);
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0xC, 4, 0xD, 4), 2);
        Em_PlaySeAtBoneOnFrame(em, 0x8A, Sound_RandomSe(0xE, 4, 0xF, 4), 2);
        Em_PlaySeAtBoneOnFrame(em, 0xE0, Sound_RandomSe(0xC, 4, 0xD, 4), 2);
        Em_PlaySeAtBoneOnFrame(em, 0x12E, Sound_RandomSe(0xE, 4, 0xF, 4), 2);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0x82, 0x1F, 0, 8);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0xD2, 0x1F, 0, 8);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0x120, 0x1F, 0, 8);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0x180, 0x20, 0, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, Sound_RandomSe(0x1D, 4, 0x1E, 4), 6);
        Em_PlaySeAtBoneOnFrame(em, 0x7E, Sound_RandomSe(0x1D, 4, 0x1E, 4), 0xA);
        Em_PlaySeAtBoneOnFrame(em, 0xCE, Sound_RandomSe(0x1D, 4, 0x1E, 4), 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x3A, 0x64, 0x12, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x7E, 0x64, 0x12, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xCE, 0x64, 0x12, 0x16);
        Em_PlaySeAtBoneOnFrame(em, 0x62, Sound_RandomSe(0, 4, 1, 4), 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0xB0, Sound_RandomSe(2, 4, 3, 4), 0x16);
        Em_PlaySeAtBoneOnFrame(em, 0x100, Sound_RandomSe(0, 4, 1, 4), 0x12);
        Em_PlaySeAtBoneOnFrame(em, 0x15A, Sound_RandomSe(2, 4, 3, 4), 0x1E);
        Em_PlaySeAtBoneVolOnFrame(em, 0x62, 0x64, 0x1A, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xB0, 0x64, 0x12, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x100, 0x64, 0x1E, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x15A, 0x64, 0x16, 0x16);
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0xA, 4, 0xB, 4), 4);
        Em_PlaySeAtBoneOnFrame(em, 0xC, Sound_RandomSe(0xA, 4, 0xB, 4), 8);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, Sound_RandomSe(8, 4, 9, 4), 0x19);
        Em_PlaySeAtBoneOnFrame(em, 0x8A, Sound_RandomSe(0xA, 4, 0xB, 4), 0x15);
        Em_PlaySeAtBoneOnFrame(em, 0xDC, Sound_RandomSe(8, 4, 9, 4), 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x136, Sound_RandomSe(0xA, 4, 0xB, 4), 0x1D);
        break;
    case 0x3F5:
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0x51, 0x1F, 9);
        Em_PlaySeAtBoneVolOnFrame(em, 0x6E, 0x55, 0x1F, 9);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0x17C, 0x23, 0, 9);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0x154, 0x24, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x58, Sound_RandomSe(0x1D, 4, 0x1E, 4), 0xA);
        Em_PlaySeAtBoneOnFrame(em, 0x9E, Sound_RandomSe(0x1D, 4, 0x1E, 4), 0xA);
        Em_PlaySeAtBoneVolOnFrame(em, 0x58, 0x64, 0x1E, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x9E, 0x64, 0x16, 0x16);
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0xA, 4, 0xB, 4), 4);
        Em_PlaySeAtBoneOnFrame(em, 0x76, Sound_RandomSe(0xA, 4, 0xB, 4), 9);
        Em_PlaySeAtBoneOnFrame(em, 0x8C, Sound_RandomSe(2, 4, 3, 4), 0x1E);
        Em_PlaySeAtBoneOnFrame(em, 0xEE, Sound_RandomSe(2, 4, 3, 4), 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x8C, 0x64, 0x1E, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xEE, 0x64, 0x16, 0x16);
        Em_PlaySeAtBoneOnFrame(em, 0x7C, Sound_RandomSe(0xA, 4, 0xB, 4), 0x1D);
        Em_PlaySeAtBoneOnFrame(em, 0xDE, Sound_RandomSe(0xA, 4, 0xB, 4), 0x15);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0xB6, 0x20, 1, 8);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0xB6, 0x22, 2, 8);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0xF4, 0x11, 0, 8);
        break;
    case 0x3F6:
        Em_PlaySeAtBoneVolOnFrame(em, 0xD2, 0x57, 0x1F, 9);
        EmShenGaoren_PlaySeOnFrame((s32) em, 4, 0x25, 0, 8);
        EmShenGaoren_PlaySeOnFrame((s32) em, 4, 0x26, 0, 9);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0xE, 0x20, 0, 8);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0x96, 0x20, 0, 8);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0x4C, 9, 1, 8);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0x4E, 0xB, 2, 8);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0xA4, 8, 0, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 0x12, 2);
        Em_PlaySeAtBoneOnFrame(em, 0x24, Sound_RandomSe(0x15, 4, 0x16, 4), 0x18);
        Em_PlaySeAtBoneOnFrame(em, 0x28, Sound_RandomSe(0x15, 4, 0x16, 4), 0x10);
        Em_PlaySeAtBoneOnFrame(em, 0xD2, Sound_RandomSe(0x15, 4, 0x17, 4), 0x18);
        Em_PlaySeAtBoneOnFrame(em, 0xD8, Sound_RandomSe(0x16, 4, 0x18, 4), 0x10);
        Em_PlaySeAtBoneOnFrame(em, 0xD8, Sound_RandomSe(0xC, 4, 0xD, 4), 9);
        Em_PlaySeAtBoneOnFrame(em, 0xDC, Sound_RandomSe(0xC, 4, 0xE, 4), 5);
        break;
    case 0x3F7:
        Em_PlaySeAtBoneVolOnFrame(em, 0xA, 0x51, 0x1F, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xA8, Sound_RandomSe(2, 4, 3, 4), 0x1E);
        Em_PlaySeAtBoneOnFrame(em, 0x10C, Sound_RandomSe(2, 4, 3, 4), 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xA8, 0x64, 0x1E, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x10C, 0x64, 0x16, 0x16);
        Em_PlaySeAtBoneOnFrame(em, 0x8C, Sound_RandomSe(0xA, 4, 0xB, 4), 0x1D);
        Em_PlaySeAtBoneOnFrame(em, 0xE6, Sound_RandomSe(0xA, 4, 0xB, 4), 0x15);
        Em_PlaySeAtBoneOnFrame(em, 0xBE, 0x1C, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x122, 0x1C, 0xA);
        Em_PlaySeAtBoneOnFrame(em, 0xEA, Sound_RandomSe(0xA, 4, 0xB, 4), 5);
        Em_PlaySeAtBoneOnFrame(em, 0x144, Sound_RandomSe(0xA, 4, 0xB, 4), 9);
        Em_PlaySeAtBoneOnFrame(em, 0xCE, 0x22, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x144, 0x20, 0xE);
        break;
    case 0x3F8:
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0x50, 0x1F, 9);
        Em_PlaySeAtBoneVolOnFrame(em, 0xD2, 0x2F, 0x1F, 9);
        Em_PlaySeAtBoneVolOnFrame(em, 0x138, 0x51, 0x1F, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x13, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0xEC, 0x13, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0x34, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x86, 0x19, 0xA);
        Em_PlaySeAtBoneOnFrame(em, 0x8C, 0x19, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x10A, Sound_RandomSe(0xE, 4, 0xF, 4), 9);
        Em_PlaySeAtBoneOnFrame(em, 0x118, Sound_RandomSe(0xE, 4, 0xF, 4), 5);
        Em_PlaySeAtBoneOnFrame(em, 0x8E, 0x27, 0xA);
        Em_PlaySeAtBoneOnFrame(em, 0x98, 0x33, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x8E, 0x64, 0x16, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x98, 0x64, 0x16, 0x16);
        Em_PlaySeAtBoneOnFrame(em, 0x22, Sound_RandomSe(0xC, 4, 0xD, 4), 0x19);
        Em_PlaySeAtBoneOnFrame(em, 0x42, Sound_RandomSe(0xE, 4, 0xF, 4), 0x15);
        Em_PlaySeAtBoneOnFrame(em, 0x28, Sound_RandomSe(0xC, 4, 0xD, 4), 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, Sound_RandomSe(0xE, 4, 0xF, 4), 0x1D);
        Em_PlaySeAtBoneOnFrame(em, 0xAA, Sound_RandomSe(8, 4, 9, 4), 0x18);
        Em_PlaySeAtBoneOnFrame(em, 0xAE, Sound_RandomSe(0xA, 4, 0xB, 4), 0x10);
        Em_PlaySeAtBoneOnFrame(em, 0xB6, Sound_RandomSe(8, 4, 9, 4), 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0xBA, Sound_RandomSe(0xA, 4, 0xB, 4), 0x1C);
        Em_PlaySeAtBoneOnFrame(em, 0xDC, Sound_RandomSe(8, 4, 9, 4), 0x19);
        Em_PlaySeAtBoneOnFrame(em, 0xF6, Sound_RandomSe(0xA, 4, 0xB, 4), 0x15);
        Em_PlaySeAtBoneOnFrame(em, 0xE4, Sound_RandomSe(8, 4, 9, 4), 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0xFA, Sound_RandomSe(0xA, 4, 0xB, 4), 0x1D);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0x80, 0x12, 0, 8);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0xC2, 0x1F, 0, 8);
        break;
    case 0x3FC:
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0x59, 0x1F, 9);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x33, 2);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 0x35, 2);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 0x36, 2);
        Em_PlaySeAtBoneVolOnFrame(em, 0x7C, 0x2B, 0x1F, 9);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0x34, 0x1F, 0, 8);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0xCA, 0x20, 0, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x38, Sound_RandomSe(4, 4, 5, 4), 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, Sound_RandomSe(4, 4, 5, 4), 0x12);
        Em_PlaySeAtBoneOnFrame(em, 0x7A, Sound_RandomSe(2, 4, 3, 4), 0x16);
        Em_PlaySeAtBoneOnFrame(em, 0xB0, Sound_RandomSe(0, 4, 1, 4), 0x12);
        Em_PlaySeAtBoneOnFrame(em, 0xEC, Sound_RandomSe(0, 4, 1, 4), 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x11E, Sound_RandomSe(2, 4, 3, 4), 0x1E);
        Em_PlaySeAtBoneVolOnFrame(em, 0x7A, 0x64, 0x16, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xB0, 0x64, 0x12, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xEC, 0x64, 0x1A, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x11E, 0x64, 0x1E, 0x16);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, Sound_RandomSe(8, 4, 9, 4), 0x15);
        Em_PlaySeAtBoneOnFrame(em, 0xA0, Sound_RandomSe(0xC, 4, 0xD, 4), 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0xDC, Sound_RandomSe(0xC, 4, 0xD, 4), 0x19);
        Em_PlaySeAtBoneOnFrame(em, 0x10E, Sound_RandomSe(0xC, 4, 0xD, 4), 0x1D);
        break;
    case 0x3FD:
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0x58, 0x1F, 9);
        Em_PlaySeAtBoneVolOnFrame(em, 0x7C, 0x2B, 0x1F, 9);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x33, 2);
        Em_PlaySeAtBoneOnFrame(em, 0x30, Sound_RandomSe(0, 4, 1, 4), 0x1A);
        Em_PlaySeAtBoneVolOnFrame(em, 0x30, 0x64, 0x1A, 0x16);
        Em_PlaySeAtBoneOnFrame(em, 8, Sound_RandomSe(8, 4, 9, 4), 0x19);
        Em_PlaySeAtBoneOnFrame(em, 0x10, Sound_RandomSe(8, 4, 9, 4), 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, Sound_RandomSe(8, 4, 9, 4), 0x1D);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, Sound_RandomSe(0xC, 4, 0xD, 4), 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0xCA, Sound_RandomSe(0xC, 4, 0xD, 4), 0x15);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0x50, 0x22, 0, 8);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0xD8, 0x20, 0, 8);
        break;
    case 0x3FE:
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0x5A, 0x1F, 9);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x33, 2);
        Em_PlaySeAtBoneVolOnFrame(em, 0x60, 0x27, 0xE, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x7A, 0x36, 2);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, Sound_RandomSe(0, 4, 1, 4), 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x86, Sound_RandomSe(0, 4, 1, 4), 0x1A);
        Em_PlaySeAtBoneVolOnFrame(em, 0x3C, 0x64, 0x1A, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x86, 0x64, 0x1A, 0x16);
        Em_PlaySeAtBoneOnFrame(em, 0x74, Sound_RandomSe(0x1D, 4, 0x1E, 4), 0xA);
        Em_PlaySeAtBoneVolOnFrame(em, 0x74, 0x64, 0xA, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x66, 0x19, 6);
        Em_PlaySeAtBoneOnFrame(em, 0xC, Sound_RandomSe(8, 4, 9, 4), 9);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0x16, 0x21, 0, 8);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0x8A, 0x20, 0, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x52, Sound_RandomSe(8, 4, 9, 4), 0x19);
        Em_PlaySeAtBoneOnFrame(em, 0x12, Sound_RandomSe(0xE, 4, 0xF, 4), 0x15);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, Sound_RandomSe(0xA, 4, 0xB, 4), 0x15);
        Em_PlaySeAtBoneOnFrame(em, 0x5A, Sound_RandomSe(8, 4, 9, 4), 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0xC, Sound_RandomSe(0xE, 4, 0xF, 4), 0x1D);
        Em_PlaySeAtBoneOnFrame(em, 0x74, Sound_RandomSe(0xA, 4, 0xB, 4), 0x1D);
        break;
    case 0x3FF:
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0x5F, 0x1F, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x12, Sound_RandomSe(0xC, 2, 0x16, 2), 0x19);
        Em_PlaySeAtBoneOnFrame(em, 0x18, Sound_RandomSe(0xE, 2, 0x17, 2), 0x15);
        Em_PlaySeAtBoneOnFrame(em, 0x7A, Sound_RandomSe(0xD, 2, 0x17, 2), 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x82, Sound_RandomSe(0xF, 2, 0x16, 2), 0x1D);
        break;
    case 0x400:
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0x52, 0x1F, 9);
        Em_PlaySeAtBoneVolOnFrame(em, 0xF4, 0x57, 0x1F, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x98, 0x13, 2);
        Em_PlaySeAtBoneOnFrame(em, 0xEE, Sound_RandomSe(0, 4, 1, 4), 0x1A);
        Em_PlaySeAtBoneVolOnFrame(em, 0xEE, 0x64, 0x1A, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x100, Sound_RandomSe(8, 4, 9, 4), 0x19);
        Em_PlaySeAtBoneOnFrame(em, 0x152, Sound_RandomSe(0, 4, 1, 4), 0x12);
        Em_PlaySeAtBoneVolOnFrame(em, 0x152, 0x64, 0x12, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x154, Sound_RandomSe(8, 4, 9, 4), 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x18, Sound_RandomSe(0x17, 4, 0x18, 4), 0x19);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, Sound_RandomSe(0x17, 4, 0x18, 4), 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x68, Sound_RandomSe(0x17, 4, 0x18, 4), 0x1D);
        Em_PlaySeAtBoneOnFrame(em, 0x38, Sound_RandomSe(0x17, 4, 0x18, 4), 0x15);
        Em_PlaySeAtBoneOnFrame(em, 0x15E, Sound_RandomSe(0x17, 4, 0x18, 4), 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x30, Sound_RandomSe(0x15, 4, 0x16, 4), 0x19);
        Em_PlaySeAtBoneOnFrame(em, 0x64, Sound_RandomSe(0x15, 4, 0x16, 4), 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, Sound_RandomSe(0x15, 4, 0x16, 4), 0x1D);
        Em_PlaySeAtBoneOnFrame(em, 0x18, Sound_RandomSe(0x15, 4, 0x16, 4), 0x15);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0x140, 0x20, 0, 8);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0x190, 0x22, 0, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 0x1C, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x7C, 0x1E, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x7C, 0x64, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xC6, 0x1C, 0xA);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, Sound_RandomSe(0xC, 4, 0xD, 4), 0x1D);
        Em_PlaySeAtBoneOnFrame(em, 0xC6, Sound_RandomSe(8, 4, 9, 4), 0x1D);
        break;
    case 0x401:
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0x52, 0x1F, 9);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x13, 2);
        Em_PlaySeAtBoneOnFrame(em, 0x90, 0x12, 2);
        Em_PlaySeAtBoneOnFrame(em, 0x24, Sound_RandomSe(4, 4, 5, 4), 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x56, Sound_RandomSe(6, 4, 7, 4), 0x16);
        Em_PlaySeAtBoneOnFrame(em, 0x88, Sound_RandomSe(4, 4, 5, 4), 0x12);
        Em_PlaySeAtBoneOnFrame(em, 0xBA, Sound_RandomSe(6, 4, 7, 4), 0x1E);
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(8, 4, 9, 4), 0x19);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, Sound_RandomSe(0xA, 4, 0xB, 4), 0x15);
        Em_PlaySeAtBoneOnFrame(em, 0x66, Sound_RandomSe(8, 4, 9, 4), 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0xA8, Sound_RandomSe(0xA, 4, 0xB, 4), 0x1D);
        Em_PlaySeAtBoneOnFrame(em, 0xA0, Sound_RandomSe(0xC, 4, 0xD, 4), 0x19);
        Em_PlaySeAtBoneOnFrame(em, 0x5C, Sound_RandomSe(0xE, 4, 0xF, 4), 0x15);
        Em_PlaySeAtBoneOnFrame(em, 0x66, Sound_RandomSe(0xC, 4, 0xD, 4), 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0xA8, Sound_RandomSe(0xE, 4, 0xF, 4), 0x1D);
        Em_PlaySeAtBoneVolOnFrame(em, 0x24, 0x64, 0x1A, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x56, 0x64, 0x16, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x88, 0x64, 0x12, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xBA, 0x64, 0x1E, 0x16);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0xD2, 0x20, 0, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x20, Sound_RandomSe(0x15, 4, 0x16, 4), 0x19);
        Em_PlaySeAtBoneOnFrame(em, 0x24, Sound_RandomSe(0x15, 4, 0x16, 4), 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, Sound_RandomSe(0x15, 4, 0x16, 4), 0x1D);
        Em_PlaySeAtBoneOnFrame(em, 0x22, Sound_RandomSe(0x15, 4, 0x16, 4), 0x15);
        break;
    case 0x406:
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x3A, 4, 0x3B, 4), 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0xB8, Sound_RandomSe(0x3C, 4, 0x3A, 4), 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x162, Sound_RandomSe(0x3B, 4, 0x3C, 4), 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x1A4, Sound_RandomSe(0x3A, 4, 0x3B, 4), 0x1F);
        break;
    case 0x407:
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x3A, 4, 0x3B, 4), 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0xB8, Sound_RandomSe(0x3C, 4, 0x3A, 4), 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x162, Sound_RandomSe(0x3B, 4, 0x3C, 4), 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, Sound_RandomSe(4, 4, 5, 4), 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x96, Sound_RandomSe(6, 4, 7, 4), 0x16);
        Em_PlaySeAtBoneOnFrame(em, 0xFE, Sound_RandomSe(4, 4, 5, 4), 0x12);
        Em_PlaySeAtBoneOnFrame(em, 0x166, Sound_RandomSe(6, 4, 7, 4), 0x1E);
        Em_PlaySeAtBoneOnFrame(em, 0xC, Sound_RandomSe(8, 4, 9, 4), 0x18);
        Em_PlaySeAtBoneOnFrame(em, 0x74, Sound_RandomSe(0xA, 4, 0xB, 4), 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0xDC, Sound_RandomSe(8, 4, 9, 4), 0x10);
        Em_PlaySeAtBoneOnFrame(em, 0x144, Sound_RandomSe(0xA, 4, 0xB, 4), 0x1C);
        Em_PlaySeAtBoneVolOnFrame(em, 0x2E, 0x64, 0x1A, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x96, 0x64, 0x16, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xFE, 0x64, 0x12, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x166, 0x64, 0x1E, 0x16);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, Sound_RandomSe(0xE, 4, 0xF, 4), 2);
        Em_PlaySeAtBoneOnFrame(em, 0xA4, Sound_RandomSe(0xC, 4, 0xD, 4), 2);
        Em_PlaySeAtBoneOnFrame(em, 0xEA, Sound_RandomSe(0xE, 4, 0xF, 4), 2);
        Em_PlaySeAtBoneOnFrame(em, 0x152, Sound_RandomSe(0xC, 4, 0xD, 4), 2);
        break;
    case 0x408:
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x3A, 4, 0x3B, 4), 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0xB8, Sound_RandomSe(0x3C, 4, 0x3A, 4), 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x162, Sound_RandomSe(0x3B, 4, 0x3C, 4), 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x5C, Sound_RandomSe(6, 4, 7, 4), 0x1E);
        Em_PlaySeAtBoneOnFrame(em, 0xC4, Sound_RandomSe(4, 4, 5, 4), 0x12);
        Em_PlaySeAtBoneOnFrame(em, 0x12C, Sound_RandomSe(6, 4, 7, 4), 0x16);
        Em_PlaySeAtBoneOnFrame(em, 0x194, Sound_RandomSe(4, 4, 5, 4), 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, Sound_RandomSe(0xA, 4, 0xB, 4), 0x1D);
        Em_PlaySeAtBoneOnFrame(em, 0xA6, Sound_RandomSe(8, 4, 9, 4), 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x10E, Sound_RandomSe(0xA, 4, 0xB, 4), 0x15);
        Em_PlaySeAtBoneOnFrame(em, 0x176, Sound_RandomSe(8, 4, 9, 4), 0x1D);
        Em_PlaySeAtBoneVolOnFrame(em, 0x5C, 0x64, 0x1A, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xC4, 0x64, 0x16, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x12C, 0x64, 0x12, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x194, 0x64, 0x1E, 0x16);
        Em_PlaySeAtBoneOnFrame(em, 6, Sound_RandomSe(0xE, 4, 0xF, 4), 2);
        Em_PlaySeAtBoneOnFrame(em, 0x72, Sound_RandomSe(0xC, 4, 0xD, 4), 2);
        Em_PlaySeAtBoneOnFrame(em, 0xD6, Sound_RandomSe(0xE, 4, 0xF, 4), 2);
        Em_PlaySeAtBoneOnFrame(em, 0x142, Sound_RandomSe(0xC, 4, 0xD, 4), 2);
        break;
    case 0x409:
        Em_PlaySeAtBoneOnFrame(em, 0x26, Sound_RandomSe(4, 4, 5, 4), 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x8E, Sound_RandomSe(6, 4, 7, 4), 0x16);
        Em_PlaySeAtBoneOnFrame(em, 0xF6, Sound_RandomSe(4, 4, 5, 4), 0x12);
        Em_PlaySeAtBoneOnFrame(em, 0x15E, Sound_RandomSe(6, 4, 7, 4), 0x1E);
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(8, 4, 9, 4), 0x19);
        Em_PlaySeAtBoneOnFrame(em, 0x7C, Sound_RandomSe(0xA, 4, 0xB, 4), 0x15);
        Em_PlaySeAtBoneOnFrame(em, 0xD4, Sound_RandomSe(8, 4, 9, 4), 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x13C, Sound_RandomSe(0xA, 4, 0xB, 4), 0x1D);
        Em_PlaySeAtBoneVolOnFrame(em, 0x26, 0x64, 0x1A, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x8E, 0x64, 0x16, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xF6, 0x64, 0x12, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x15E, 0x64, 0x1E, 0x16);
        Em_PlaySeAtBoneOnFrame(em, 0x32, Sound_RandomSe(0xE, 4, 0xF, 4), 2);
        Em_PlaySeAtBoneOnFrame(em, 0x96, Sound_RandomSe(0xC, 4, 0xD, 4), 2);
        Em_PlaySeAtBoneOnFrame(em, 0xF6, Sound_RandomSe(0xE, 4, 0xF, 4), 2);
        Em_PlaySeAtBoneOnFrame(em, 0x168, Sound_RandomSe(0xC, 4, 0xD, 4), 2);
        break;
    case 0x40A:
        Em_PlaySeAtBoneOnFrame(em, 0x26, Sound_RandomSe(4, 4, 5, 4), 0x12);
        Em_PlaySeAtBoneOnFrame(em, 0x8E, Sound_RandomSe(6, 4, 7, 4), 0x1E);
        Em_PlaySeAtBoneOnFrame(em, 0xF6, Sound_RandomSe(4, 4, 5, 4), 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x15E, Sound_RandomSe(6, 4, 7, 4), 0x16);
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(8, 4, 9, 4), 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x7C, Sound_RandomSe(0xA, 4, 0xB, 4), 0x1D);
        Em_PlaySeAtBoneOnFrame(em, 0xD4, Sound_RandomSe(8, 4, 9, 4), 0x19);
        Em_PlaySeAtBoneOnFrame(em, 0x13C, Sound_RandomSe(0xA, 4, 0xB, 4), 0x15);
        Em_PlaySeAtBoneVolOnFrame(em, 0x26, 0x64, 0x12, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x8E, 0x64, 0x1E, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xF6, 0x64, 0x1A, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x15E, 0x64, 0x16, 0x16);
        Em_PlaySeAtBoneOnFrame(em, 0x32, Sound_RandomSe(0xE, 4, 0xF, 4), 2);
        Em_PlaySeAtBoneOnFrame(em, 0x96, Sound_RandomSe(0xC, 4, 0xD, 4), 2);
        Em_PlaySeAtBoneOnFrame(em, 0xF6, Sound_RandomSe(0xE, 4, 0xF, 4), 2);
        Em_PlaySeAtBoneOnFrame(em, 0x168, Sound_RandomSe(0xC, 4, 0xD, 4), 2);
        break;
    case 0x40B:
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0x52, 0x1F, 9);
        Em_PlaySeAtBoneVolOnFrame(em, 0x11C, 0x2B, 0x1F, 9);
        Em_PlaySe6AtBoneInFrames(em, 4, 0x164, 0x14, 2, 8, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x94, 0x11, 2);
        Em_PlaySeAtBoneOnFrame(em, 0x100, 0x12, 2);
        Em_PlaySeAtBoneOnFrame(em, 0x8A, Sound_RandomSe(0, 4, 1, 4), 0x12);
        Em_PlaySeAtBoneOnFrame(em, 0xF0, Sound_RandomSe(2, 4, 3, 4), 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x152, Sound_RandomSe(0, 4, 1, 4), 0x16);
        Em_PlaySeAtBoneOnFrame(em, 0x1B6, Sound_RandomSe(2, 4, 3, 4), 0x1E);
        Em_PlaySeAtBoneVolOnFrame(em, 0x8A, 0x64, 0x1A, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xF0, 0x64, 0x16, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x152, 0x64, 0x12, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1B6, 0x64, 0x1E, 0x16);
        Em_PlaySeAtBoneOnFrame(em, 0x72, Sound_RandomSe(8, 4, 9, 4), 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0xD8, Sound_RandomSe(0xA, 4, 0xB, 4), 0x19);
        Em_PlaySeAtBoneOnFrame(em, 0x13A, Sound_RandomSe(8, 4, 9, 4), 0x15);
        Em_PlaySeAtBoneOnFrame(em, 0x19E, Sound_RandomSe(0xA, 4, 0xB, 4), 0x1D);
        Em_PlaySeAtBoneOnFrame(em, 0x104, Sound_RandomSe(0xC, 4, 0xD, 4), 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x106, Sound_RandomSe(0xE, 4, 0xF, 4), 0x19);
        Em_PlaySeAtBoneOnFrame(em, 0x1C2, Sound_RandomSe(8, 4, 9, 4), 0x19);
        Em_PlaySeAtBoneOnFrame(em, 0xAA, Sound_RandomSe(0xC, 4, 0xD, 4), 0x15);
        Em_PlaySeAtBoneOnFrame(em, 0x16A, Sound_RandomSe(8, 4, 9, 4), 0x15);
        Em_PlaySeAtBoneOnFrame(em, 0xAA, Sound_RandomSe(0xE, 4, 0xF, 4), 0x1D);
        Em_PlaySeAtBoneOnFrame(em, 0xF2, Sound_RandomSe(0xA, 4, 0xB, 4), 0x1D);
        Em_PlaySeAtBoneOnFrame(em, 0x1CC, Sound_RandomSe(0xA, 4, 0xB, 4), 0x1D);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, Sound_RandomSe(0x17, 4, 0x18, 4), 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x5A, Sound_RandomSe(0x17, 4, 0x18, 4), 0x1D);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, Sound_RandomSe(0x17, 4, 0x18, 4), 0x15);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0x9C, 0x21, 0, 8);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0xEA, 0x1F, 0, 8);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0x100, 0x22, 0, 8);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0x150, 0x20, 0, 8);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0x1C8, 0x1F, 0, 8);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0x20E, 0x1F, 0, 8);
        Em_PlaySeAtBoneOnFrame(em, 0xA6, Sound_RandomSe(0xC, 4, 0xD, 4), 0xA);
        Em_PlaySeAtBoneOnFrame(em, 0xAA, Sound_RandomSe(0xE, 4, 0xF, 4), 6);
        Em_PlaySeAtBoneOnFrame(em, 0x188, Sound_RandomSe(8, 4, 9, 4), 0xA);
        Em_PlaySeAtBoneOnFrame(em, 0x1AA, Sound_RandomSe(8, 4, 9, 4), 6);
        break;
    case 0x411:
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0x54, 0x1F, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x13, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x82, 0x34, 6);
        Em_PlaySeAtBoneOnFrame(em, 0xC2, 0x19, 0xA);
        Em_PlaySeAtBoneOnFrame(em, 0xC8, 0x19, 6);
        Em_PlaySeAtBoneOnFrame(em, 8, Sound_RandomSe(0xE, 4, 0xF, 4), 9);
        Em_PlaySeAtBoneOnFrame(em, 0xE, Sound_RandomSe(0xE, 4, 0xF, 4), 5);
        Em_PlaySeAtBoneOnFrame(em, 0xF0, Sound_RandomSe(8, 4, 9, 4), 0xA);
        Em_PlaySeAtBoneOnFrame(em, 0xF6, Sound_RandomSe(0xA, 4, 0xB, 4), 6);
        Em_PlaySeAtBoneOnFrame(em, 0x134, Sound_RandomSe(0xC, 4, 0xD, 4), 9);
        Em_PlaySeAtBoneOnFrame(em, 0x13A, Sound_RandomSe(0xE, 4, 0xF, 4), 5);
        Em_PlaySeAtBoneOnFrame(em, 0x70, Sound_RandomSe(0xC, 4, 0xD, 4), 0x19);
        Em_PlaySeAtBoneOnFrame(em, 0x80, Sound_RandomSe(0xE, 4, 0xF, 4), 0x15);
        Em_PlaySeAtBoneOnFrame(em, 0x7C, Sound_RandomSe(0xC, 4, 0xD, 4), 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x74, Sound_RandomSe(0xE, 4, 0xF, 4), 0x1D);
        Em_PlaySeAtBoneOnFrame(em, 0xBE, Sound_RandomSe(8, 4, 9, 4), 0x18);
        Em_PlaySeAtBoneOnFrame(em, 0xC2, Sound_RandomSe(0xA, 4, 0xB, 4), 0x10);
        Em_PlaySeAtBoneOnFrame(em, 0xCA, Sound_RandomSe(8, 4, 9, 4), 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0xCE, Sound_RandomSe(0xA, 4, 0xB, 4), 0x1C);
        Em_PlaySeAtBoneOnFrame(em, 0x122, Sound_RandomSe(8, 4, 9, 4), 0x19);
        Em_PlaySeAtBoneOnFrame(em, 0x126, Sound_RandomSe(0xA, 4, 0xB, 4), 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x12E, Sound_RandomSe(8, 4, 9, 4), 0x15);
        Em_PlaySeAtBoneOnFrame(em, 0x132, Sound_RandomSe(0xA, 4, 0xB, 4), 0x1D);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0x80, 0x12, 0, 8);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0xC2, 0x21, 0, 8);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0x104, 0x1F, 0, 8);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0x14E, 0x1F, 0, 8);
        Em_PlaySeAtBoneOnFrame(em, 0xD0, 0x27, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0xD2, 0x64, 6, 0x16);
        break;
    case 0x412:
        Em_PlaySeAtBoneOnFrame(em, 0x96, 0x19, 0xA);
        Em_PlaySeAtBoneOnFrame(em, 0xA4, 0x19, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x62, Sound_RandomSe(0, 4, 1, 4), 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0xB0, Sound_RandomSe(2, 4, 3, 4), 0x16);
        Em_PlaySeAtBoneOnFrame(em, 0x100, Sound_RandomSe(0, 4, 1, 4), 0x12);
        Em_PlaySeAtBoneOnFrame(em, 0x15A, Sound_RandomSe(2, 4, 3, 4), 0x1E);
        Em_PlaySeAtBoneVolOnFrame(em, 0x62, 0x64, 0x1A, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xB0, 0x64, 0x12, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x100, 0x64, 0x1E, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x15A, 0x64, 0x16, 0x16);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0x82, 0x1F, 0, 8);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0xD2, 0x1F, 0, 8);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0x120, 0x1F, 0, 8);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0x180, 0x20, 0, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0x51, 0x1F, 9);
        Em_PlaySeAtBoneVolOnFrame(em, 0x28, 0x54, 0x1F, 9);
        Em_PlaySeAtBoneVolOnFrame(em, 0x154, 0x2B, 0x1F, 9);
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0xC, 4, 0xD, 4), 2);
        Em_PlaySeAtBoneOnFrame(em, 0x8A, Sound_RandomSe(0xE, 4, 0xF, 4), 2);
        Em_PlaySeAtBoneOnFrame(em, 0xE0, Sound_RandomSe(0xC, 4, 0xD, 4), 2);
        Em_PlaySeAtBoneOnFrame(em, 0x12E, Sound_RandomSe(0xE, 4, 0xF, 4), 2);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, Sound_RandomSe(0xA, 4, 0xB, 4), 4);
        Em_PlaySeAtBoneOnFrame(em, 0x76, Sound_RandomSe(0xA, 4, 0xB, 4), 8);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, Sound_RandomSe(8, 4, 9, 4), 0x19);
        Em_PlaySeAtBoneOnFrame(em, 0x8A, Sound_RandomSe(0xA, 4, 0xB, 4), 0x15);
        Em_PlaySeAtBoneOnFrame(em, 0xDC, Sound_RandomSe(8, 4, 9, 4), 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x136, Sound_RandomSe(0xA, 4, 0xB, 4), 0x1D);
        break;
    case 0x413:
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0x51, 0x1F, 9);
        Em_PlaySeAtBoneVolOnFrame(em, 0x6E, 0x55, 0x1F, 9);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0x17C, 0x23, 0, 9);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0x154, 0x24, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x28, Sound_RandomSe(0xC, 4, 0xD, 4), 0x19);
        Em_PlaySeAtBoneOnFrame(em, 0x24, Sound_RandomSe(0xE, 4, 0xF, 4), 0x15);
        Em_PlaySeAtBoneOnFrame(em, 0x28, Sound_RandomSe(0xC, 4, 0xD, 4), 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x24, Sound_RandomSe(0xE, 4, 0xF, 4), 0x1D);
        Em_PlaySeAtBoneOnFrame(em, 0x58, Sound_RandomSe(0x1D, 4, 0x1E, 4), 0xA);
        Em_PlaySeAtBoneOnFrame(em, 0x9E, Sound_RandomSe(0x1D, 4, 0x1E, 4), 0xA);
        Em_PlaySeAtBoneVolOnFrame(em, 0x58, 0x64, 0x1E, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x9E, 0x64, 0x16, 0x16);
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0xA, 4, 0xB, 4), 4);
        Em_PlaySeAtBoneOnFrame(em, 0x76, Sound_RandomSe(0xA, 4, 0xB, 4), 9);
        Em_PlaySeAtBoneOnFrame(em, 0x8C, Sound_RandomSe(2, 4, 3, 4), 0x1E);
        Em_PlaySeAtBoneOnFrame(em, 0xEE, Sound_RandomSe(2, 4, 3, 4), 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x8C, 0x64, 0x1E, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xEE, 0x64, 0x16, 0x16);
        Em_PlaySeAtBoneOnFrame(em, 0x7C, Sound_RandomSe(0xA, 4, 0xB, 4), 0x1D);
        Em_PlaySeAtBoneOnFrame(em, 0xDE, Sound_RandomSe(0xA, 4, 0xB, 4), 0x15);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0xB6, 0x20, 1, 8);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0xB6, 0x22, 2, 8);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0xF4, 0x11, 0, 8);
        break;
    case 0x414:
        Em_PlaySeAtBoneVolOnFrame(em, 0xD2, 0x57, 0x1F, 9);
        EmShenGaoren_PlaySeOnFrame((s32) em, 4, 0x25, 0, 8);
        EmShenGaoren_PlaySeOnFrame((s32) em, 4, 0x26, 0, 9);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0xE, 0x20, 0, 8);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0x96, 0x20, 0, 8);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0x4C, 9, 1, 8);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0x4E, 0xB, 2, 8);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0xA4, 8, 0, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 0x12, 2);
        Em_PlaySeAtBoneOnFrame(em, 0x24, Sound_RandomSe(0x15, 4, 0x16, 4), 0x18);
        Em_PlaySeAtBoneOnFrame(em, 0x28, Sound_RandomSe(0x15, 4, 0x16, 4), 0x10);
        Em_PlaySeAtBoneOnFrame(em, 0xD2, Sound_RandomSe(0x15, 4, 0x17, 4), 0x18);
        Em_PlaySeAtBoneOnFrame(em, 0xD8, Sound_RandomSe(0x16, 4, 0x18, 4), 0x10);
        Em_PlaySeAtBoneOnFrame(em, 0xD8, Sound_RandomSe(0xC, 4, 0xD, 4), 9);
        Em_PlaySeAtBoneOnFrame(em, 0xDC, Sound_RandomSe(0xC, 4, 0xE, 4), 5);
        break;
    case 0x415:
        Em_PlaySeAtBoneVolOnFrame(em, 0xA, 0x51, 0x1F, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x13, 2);
        Em_PlaySeAtBoneOnFrame(em, 0x17C, 0x15, 2);
        Em_PlaySeAtBoneOnFrame(em, 0xA8, Sound_RandomSe(2, 4, 3, 4), 0x1E);
        Em_PlaySeAtBoneOnFrame(em, 0x10C, Sound_RandomSe(2, 4, 3, 4), 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xA8, 0x64, 0x1E, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x10C, 0x64, 0x16, 0x16);
        Em_PlaySeAtBoneOnFrame(em, 0x8C, Sound_RandomSe(0xA, 4, 0xB, 4), 0x1D);
        Em_PlaySeAtBoneOnFrame(em, 0xE6, Sound_RandomSe(0xA, 4, 0xB, 4), 0x15);
        Em_PlaySeAtBoneOnFrame(em, 0xBE, 0x1C, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x122, 0x1C, 0xA);
        Em_PlaySeAtBoneOnFrame(em, 0xEA, Sound_RandomSe(0xA, 4, 0xB, 4), 5);
        Em_PlaySeAtBoneOnFrame(em, 0x144, Sound_RandomSe(0xA, 4, 0xB, 4), 9);
        Em_PlaySeAtBoneOnFrame(em, 0xCE, 0x22, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x144, 0x20, 0xE);
        break;
    case 0x419:
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0x58, 0x1F, 9);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x33, 2);
        Em_PlaySeAtBoneOnFrame(em, 0x22, Sound_RandomSe(2, 4, 3, 4), 0x1E);
        Em_PlaySeAtBoneOnFrame(em, 0x40, Sound_RandomSe(2, 4, 3, 4), 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x22, 0x64, 0x1E, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x40, 0x64, 0x16, 0x16);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, Sound_RandomSe(0xA, 4, 0xB, 4), 0x1D);
        Em_PlaySeAtBoneOnFrame(em, 0x38, Sound_RandomSe(0xA, 4, 0xB, 4), 0x15);
        Em_PlaySeAtBoneOnFrame(em, 0x12, Sound_RandomSe(0xC, 4, 0xD, 4), 0x19);
        Em_PlaySeAtBoneOnFrame(em, 0x18, Sound_RandomSe(0xC, 4, 0xD, 4), 0x11);
        EmShenGaoren_PlaySeOnFrame((s32) em, 6, 0x21, 0, 8);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0x32, 0x1F, 0, 8);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x1C, 0xA);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x1D, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 0x34, 6);
        break;
    case 0x41A:
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0x59, 0x1F, 9);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x33, 2);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 0x35, 0x1E);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 0x36, 2);
        Em_PlaySeAtBoneVolOnFrame(em, 0x7C, 0x2B, 0x1F, 9);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0x34, 0x1F, 0, 8);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0xCA, 0x20, 0, 8);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x34, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 0x34, 0x12);
        Em_PlaySeAtBoneOnFrame(em, 0x5A, 0x34, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x122, Sound_RandomSe(0x15, 4, 0x16, 4), 2);
        Em_PlaySeAtBoneOnFrame(em, 0x38, Sound_RandomSe(0, 4, 1, 4), 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, Sound_RandomSe(0, 4, 1, 4), 0x12);
        Em_PlaySeAtBoneOnFrame(em, 0x7A, Sound_RandomSe(2, 4, 3, 4), 0x16);
        Em_PlaySeAtBoneOnFrame(em, 0xB0, Sound_RandomSe(0, 4, 1, 4), 0x12);
        Em_PlaySeAtBoneOnFrame(em, 0xEC, Sound_RandomSe(0, 4, 1, 4), 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x11E, Sound_RandomSe(2, 4, 3, 4), 0x1E);
        Em_PlaySeAtBoneVolOnFrame(em, 0x38, 0x64, 0x1A, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x4A, 0x64, 0x12, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x7A, 0x64, 0x16, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xB0, 0x64, 0x12, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xEC, 0x64, 0x1A, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x11E, 0x64, 0x1E, 0x16);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, Sound_RandomSe(8, 4, 9, 4), 0x15);
        Em_PlaySeAtBoneOnFrame(em, 0xA0, Sound_RandomSe(0xC, 4, 0xD, 4), 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0xDC, Sound_RandomSe(0xC, 4, 0xD, 4), 0x19);
        Em_PlaySeAtBoneOnFrame(em, 0x10E, Sound_RandomSe(0xC, 4, 0xD, 4), 0x1D);
        break;
    case 0x41B:
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0x58, 0x1F, 9);
        Em_PlaySeAtBoneVolOnFrame(em, 0x7C, 0x2B, 0x1F, 9);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x33, 2);
        Em_PlaySeAtBoneOnFrame(em, 0xD8, Sound_RandomSe(0x15, 4, 0x16, 4), 2);
        Em_PlaySeAtBoneOnFrame(em, 0x30, Sound_RandomSe(0, 4, 1, 4), 0x1A);
        Em_PlaySeAtBoneVolOnFrame(em, 0x30, 0x64, 0x1A, 0x16);
        Em_PlaySeAtBoneOnFrame(em, 8, Sound_RandomSe(8, 4, 9, 4), 0x19);
        Em_PlaySeAtBoneOnFrame(em, 0x10, Sound_RandomSe(8, 4, 9, 4), 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, Sound_RandomSe(8, 4, 9, 4), 0x1D);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, Sound_RandomSe(0xC, 4, 0xD, 4), 0x12);
        Em_PlaySeAtBoneOnFrame(em, 0xCA, Sound_RandomSe(0xC, 4, 0xD, 4), 0x1E);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x34, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 0x34, 0x12);
        Em_PlaySeAtBoneOnFrame(em, 0x60, 0x34, 0x1A);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0x50, 0x22, 0, 8);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0xD8, 0x20, 0, 8);
        break;
    case 0x41C:
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0x5A, 0x1F, 9);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x33, 2);
        Em_PlaySeAtBoneOnFrame(em, 0x56, 0x11, 2);
        Em_PlaySeAtBoneVolOnFrame(em, 0x7A, 0x27, 0xE, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 0x33, 2);
        Em_PlaySeAtBoneOnFrame(em, 0x8A, 0x36, 2);
        Em_PlaySeAtBoneOnFrame(em, 0x18, Sound_RandomSe(0x17, 4, 0x18, 4), 0x19);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, Sound_RandomSe(0x17, 4, 0x18, 4), 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x68, Sound_RandomSe(0x17, 4, 0x18, 4), 0x1D);
        Em_PlaySeAtBoneOnFrame(em, 0x38, Sound_RandomSe(0x17, 4, 0x18, 4), 0x15);
        Em_PlaySeAtBoneOnFrame(em, 0x30, Sound_RandomSe(0x15, 4, 0x16, 4), 0x19);
        Em_PlaySeAtBoneOnFrame(em, 0x64, Sound_RandomSe(0x15, 4, 0x16, 4), 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, Sound_RandomSe(0x15, 4, 0x16, 4), 0x1D);
        Em_PlaySeAtBoneOnFrame(em, 0x18, Sound_RandomSe(0x15, 4, 0x16, 4), 0x15);
        Em_PlaySeAtBoneOnFrame(em, 0x12, Sound_RandomSe(8, 4, 9, 4), 9);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, Sound_RandomSe(8, 4, 9, 4), 5);
        Em_PlaySeAtBoneOnFrame(em, 0x70, 0x19, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x76, 0x19, 0xA);
        Em_PlaySeAtBoneOnFrame(em, 0x86, 0x1D, 0xA);
        Em_PlaySeAtBoneOnFrame(em, 0x8A, 0x1E, 6);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0x1F, 0x21, 0, 8);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0x80, 0x22, 0, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x8C, Sound_RandomSe(8, 4, 9, 4), 0x19);
        Em_PlaySeAtBoneOnFrame(em, 0x90, Sound_RandomSe(0xA, 4, 0xB, 4), 0x15);
        Em_PlaySeAtBoneOnFrame(em, 0x94, Sound_RandomSe(8, 4, 9, 4), 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x98, Sound_RandomSe(0xA, 4, 0xB, 4), 0x1D);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0x6C, 0x1D, 0, 8);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0x6C, 0x1E, 0, 8);
        break;
    case 0x41D:
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0x5F, 0x1F, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x12, Sound_RandomSe(0xC, 2, 0x16, 2), 0x19);
        Em_PlaySeAtBoneOnFrame(em, 0x18, Sound_RandomSe(0xE, 2, 0x17, 2), 0x15);
        Em_PlaySeAtBoneOnFrame(em, 0x7A, Sound_RandomSe(0xD, 2, 0x17, 2), 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x82, Sound_RandomSe(0xF, 2, 0x16, 2), 0x1D);
        Em_PlaySeAtBoneOnFrame(em, 0x44, Sound_RandomSe(0xD, 2, 0x17, 2), 5);
        Em_PlaySeAtBoneOnFrame(em, 0x60, Sound_RandomSe(0xF, 2, 0x16, 2), 9);
        break;
    case 0x41E:
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0x52, 0x1F, 9);
        Em_PlaySeAtBoneVolOnFrame(em, 0xF4, 0x2F, 0x1F, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x98, 0x13, 2);
        Em_PlaySeAtBoneOnFrame(em, 0x50, Sound_RandomSe(0x1D, 4, 0x1E, 4), 0xA);
        Em_PlaySeAtBoneOnFrame(em, 0x7C, Sound_RandomSe(0x1D, 4, 0x1E, 4), 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x50, 0x64, 0xA, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x7C, 0x64, 6, 0x16);
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(8, 4, 9, 4), 9);
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0xC, 4, 0xD, 4), 5);
        Em_PlaySeAtBoneOnFrame(em, 0x50, Sound_RandomSe(8, 4, 9, 4), 5);
        Em_PlaySeAtBoneOnFrame(em, 0x56, 0x1C, 6);
        Em_PlaySeAtBoneOnFrame(em, 0xC6, 0x1C, 0xA);
        Em_PlaySeAtBoneOnFrame(em, 0xF8, 0x1C, 6);
        Em_PlaySeAtBoneOnFrame(em, 0xCE, Sound_RandomSe(8, 4, 9, 4), 9);
        Em_PlaySeAtBoneOnFrame(em, 0xD8, Sound_RandomSe(0xC, 4, 0xD, 4), 5);
        Em_PlaySeAtBoneOnFrame(em, 0xEE, Sound_RandomSe(0, 4, 1, 4), 0x1E);
        Em_PlaySeAtBoneVolOnFrame(em, 0xEE, 0x64, 0x1E, 0x16);
        Em_PlaySeAtBoneOnFrame(em, 0x100, Sound_RandomSe(8, 4, 9, 4), 0x1D);
        Em_PlaySeAtBoneOnFrame(em, 0x152, Sound_RandomSe(0, 4, 1, 4), 0x12);
        Em_PlaySeAtBoneVolOnFrame(em, 0x152, 0x64, 0x12, 0x16);
        Em_PlaySeAtBoneOnFrame(em, 0x154, Sound_RandomSe(8, 4, 9, 4), 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x18, Sound_RandomSe(0x17, 4, 0x18, 4), 0x19);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, Sound_RandomSe(0x17, 4, 0x18, 4), 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x68, Sound_RandomSe(0x17, 4, 0x18, 4), 0x1D);
        Em_PlaySeAtBoneOnFrame(em, 0x38, Sound_RandomSe(0x17, 4, 0x18, 4), 0x15);
        Em_PlaySeAtBoneOnFrame(em, 0x15E, Sound_RandomSe(0x17, 4, 0x18, 4), 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x30, Sound_RandomSe(0x15, 4, 0x16, 4), 0x19);
        Em_PlaySeAtBoneOnFrame(em, 0x64, Sound_RandomSe(0x15, 4, 0x16, 4), 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, Sound_RandomSe(0x15, 4, 0x16, 4), 0x1D);
        Em_PlaySeAtBoneOnFrame(em, 0x18, Sound_RandomSe(0x15, 4, 0x16, 4), 0x15);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0x140, 0x20, 0, 8);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0x190, 0x22, 0, 8);
        break;
    case 0x41F:
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0x58, 0x1F, 9);
        Em_PlaySeAtBoneVolOnFrame(em, 0x64, 0x2B, 0x1F, 9);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x33, 2);
        Em_PlaySeAtBoneOnFrame(em, 0x40, Sound_RandomSe(0, 4, 1, 4), 0x1A);
        Em_PlaySeAtBoneVolOnFrame(em, 0x40, 0x64, 0x1A, 0x16);
        Em_PlaySeAtBoneOnFrame(em, 0xBA, Sound_RandomSe(0, 4, 1, 4), 0x1A);
        Em_PlaySeAtBoneVolOnFrame(em, 0xBA, 0x64, 0x1A, 0x16);
        Em_PlaySeAtBoneOnFrame(em, 0x14, Sound_RandomSe(8, 4, 9, 4), 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x98, Sound_RandomSe(0xC, 4, 0xD, 4), 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x28, Sound_RandomSe(8, 4, 9, 4), 0xA);
        Em_PlaySeAtBoneOnFrame(em, 0x10, Sound_RandomSe(0xC, 4, 0xD, 4), 5);
        Em_PlaySeAtBoneOnFrame(em, 0x28, Sound_RandomSe(0xC, 4, 0xD, 4), 0x1D);
        Em_PlaySeAtBoneOnFrame(em, 0x14, Sound_RandomSe(0x17, 4, 0x18, 4), 0x12);
        Em_PlaySeAtBoneOnFrame(em, 0x14, Sound_RandomSe(0x15, 4, 0x16, 4), 0x16);
        Em_PlaySeAtBoneOnFrame(em, 0x28, Sound_RandomSe(0x15, 4, 0x16, 4), 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, Sound_RandomSe(0x17, 4, 0x18, 4), 0x15);
        Em_PlaySeAtBoneOnFrame(em, 0xA0, Sound_RandomSe(0x15, 4, 0x16, 4), 0x1D);
        Em_PlaySeAtBoneOnFrame(em, 0x78, Sound_RandomSe(0x15, 4, 0x16, 4), 0x15);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0x3A, 0x21, 0, 8);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0x40, 0x20, 0, 8);
        break;
    case 0x420:
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0x58, 0x1F, 9);
        Em_PlaySeAtBoneVolOnFrame(em, 0x64, 0x2B, 0x1F, 9);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x33, 2);
        Em_PlaySeAtBoneOnFrame(em, 0x40, Sound_RandomSe(0, 4, 1, 4), 0x12);
        Em_PlaySeAtBoneVolOnFrame(em, 0x40, 0x64, 0x12, 0x16);
        Em_PlaySeAtBoneOnFrame(em, 0xBA, Sound_RandomSe(0, 4, 1, 4), 0x12);
        Em_PlaySeAtBoneVolOnFrame(em, 0xBA, 0x64, 0x12, 0x16);
        Em_PlaySeAtBoneOnFrame(em, 0x14, Sound_RandomSe(8, 4, 9, 4), 0x12);
        Em_PlaySeAtBoneOnFrame(em, 0x98, Sound_RandomSe(0xC, 4, 0xD, 4), 0x12);
        Em_PlaySeAtBoneOnFrame(em, 0x28, Sound_RandomSe(8, 4, 9, 4), 6);
        Em_PlaySeAtBoneOnFrame(em, 0x10, Sound_RandomSe(0xC, 4, 0xD, 4), 9);
        Em_PlaySeAtBoneOnFrame(em, 0x28, Sound_RandomSe(0xC, 4, 0xD, 4), 0x15);
        Em_PlaySeAtBoneOnFrame(em, 0x14, Sound_RandomSe(0x17, 4, 0x18, 4), 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x14, Sound_RandomSe(0x15, 4, 0x16, 4), 0x1E);
        Em_PlaySeAtBoneOnFrame(em, 0x28, Sound_RandomSe(0x15, 4, 0x16, 4), 0x19);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, Sound_RandomSe(0x17, 4, 0x18, 4), 0x1D);
        Em_PlaySeAtBoneOnFrame(em, 0xA0, Sound_RandomSe(0x15, 4, 0x16, 4), 0x15);
        Em_PlaySeAtBoneOnFrame(em, 0x78, Sound_RandomSe(0x15, 4, 0x16, 4), 0x1D);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0x3A, 0x21, 0, 8);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0x40, 0x20, 0, 8);
        break;
    case 0x421:
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0x58, 0x1F, 9);
        Em_PlaySeAtBoneVolOnFrame(em, 0x64, 0x2B, 0x1F, 9);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x33, 2);
        Em_PlaySeAtBoneOnFrame(em, 0x40, Sound_RandomSe(0, 4, 1, 4), 0x1E);
        Em_PlaySeAtBoneVolOnFrame(em, 0x40, 0x64, 0x1E, 0x16);
        Em_PlaySeAtBoneOnFrame(em, 0xBA, Sound_RandomSe(0, 4, 1, 4), 0x1E);
        Em_PlaySeAtBoneVolOnFrame(em, 0xBA, 0x64, 0x1E, 0x16);
        Em_PlaySeAtBoneOnFrame(em, 0x14, Sound_RandomSe(8, 4, 9, 4), 0x1E);
        Em_PlaySeAtBoneOnFrame(em, 0x98, Sound_RandomSe(0xC, 4, 0xD, 4), 0x1E);
        Em_PlaySeAtBoneOnFrame(em, 0x28, Sound_RandomSe(8, 4, 9, 4), 0xA);
        Em_PlaySeAtBoneOnFrame(em, 0x10, Sound_RandomSe(0xC, 4, 0xD, 4), 5);
        Em_PlaySeAtBoneOnFrame(em, 0x28, Sound_RandomSe(0xC, 4, 0xD, 4), 0x1D);
        Em_PlaySeAtBoneOnFrame(em, 0x14, Sound_RandomSe(0x17, 4, 0x18, 4), 0x12);
        Em_PlaySeAtBoneOnFrame(em, 0x14, Sound_RandomSe(0x15, 4, 0x16, 4), 0x16);
        Em_PlaySeAtBoneOnFrame(em, 0x28, Sound_RandomSe(0x15, 4, 0x16, 4), 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, Sound_RandomSe(0x17, 4, 0x18, 4), 0x15);
        Em_PlaySeAtBoneOnFrame(em, 0xA0, Sound_RandomSe(0x15, 4, 0x16, 4), 0x1D);
        Em_PlaySeAtBoneOnFrame(em, 0x78, Sound_RandomSe(0x15, 4, 0x16, 4), 0x15);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0x3A, 0x36, 0, 8);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0x40, 0x20, 0, 8);
        break;
    case 0x422:
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0x58, 0x1F, 9);
        Em_PlaySeAtBoneVolOnFrame(em, 0x64, 0x2B, 0x1F, 9);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x33, 2);
        Em_PlaySeAtBoneOnFrame(em, 0x40, Sound_RandomSe(0, 4, 1, 4), 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x40, 0x64, 0x16, 0x16);
        Em_PlaySeAtBoneOnFrame(em, 0xBA, Sound_RandomSe(0, 4, 1, 4), 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xBA, 0x64, 0x16, 0x16);
        Em_PlaySeAtBoneOnFrame(em, 0x14, Sound_RandomSe(8, 4, 9, 4), 0x16);
        Em_PlaySeAtBoneOnFrame(em, 0x98, Sound_RandomSe(0xC, 4, 0xD, 4), 0x16);
        Em_PlaySeAtBoneOnFrame(em, 0x28, Sound_RandomSe(8, 4, 9, 4), 6);
        Em_PlaySeAtBoneOnFrame(em, 0x10, Sound_RandomSe(0xC, 4, 0xD, 4), 9);
        Em_PlaySeAtBoneOnFrame(em, 0x28, Sound_RandomSe(0xC, 4, 0xD, 4), 0x15);
        Em_PlaySeAtBoneOnFrame(em, 0x14, Sound_RandomSe(0x17, 4, 0x18, 4), 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x14, Sound_RandomSe(0x15, 4, 0x16, 4), 0x1E);
        Em_PlaySeAtBoneOnFrame(em, 0x28, Sound_RandomSe(0x15, 4, 0x16, 4), 0x19);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, Sound_RandomSe(0x17, 4, 0x18, 4), 0x1D);
        Em_PlaySeAtBoneOnFrame(em, 0xA0, Sound_RandomSe(0x15, 4, 0x16, 4), 0x15);
        Em_PlaySeAtBoneOnFrame(em, 0x78, Sound_RandomSe(0x15, 4, 0x16, 4), 0x1D);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0x3A, 0x36, 0, 8);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0x40, 0x20, 0, 8);
        break;
    case 0x423:
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0x32, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x33, 2);
        Em_PlaySeAtBoneVolOnFrame(em, 0x28, 0x13, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xE, Sound_RandomSe(0xA, 4, 0xB, 4), 0x1D);
        Em_PlaySeAtBoneOnFrame(em, 0x18, Sound_RandomSe(0xA, 4, 0xB, 4), 0x15);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, Sound_RandomSe(8, 4, 9, 4), 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x44, Sound_RandomSe(8, 4, 9, 4), 0x19);
        Em_PlaySeAtBoneOnFrame(em, 0xA6, 0x35, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0xA6, 0x11, 0x12);
        Em_PlaySeAtBoneVolOnFrame(em, 0x140, 0x30, 2, 9);
        Em_PlaySeAtBoneVolOnFrame(em, 0xDC, 0x27, 2, 9);
        Em_PlaySeAtBoneVolOnFrame(em, 0xDC, 0x36, 0xE, 9);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1A2, 0x27, 2, 9);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1A8, 0x36, 0xE, 9);
        Em_PlaySeAtBoneVolOnFrame(em, 0xF4, 0x5F, 0x1F, 9);
        Em_PlaySeAtBoneVolOnFrame(em, 0x16A, 0x34, 0x1F, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xFC, Sound_RandomSe(0xA, 4, 0xB, 4), 0x1D);
        Em_PlaySeAtBoneOnFrame(em, 0x106, Sound_RandomSe(0xA, 4, 0xB, 4), 0x15);
        Em_PlaySeAtBoneOnFrame(em, 0xE2, Sound_RandomSe(8, 4, 9, 4), 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0xE8, Sound_RandomSe(8, 4, 9, 4), 0x19);
        Em_PlaySeAtBoneOnFrame(em, 0x100, Sound_RandomSe(0x17, 4, 0x18, 4), 0x19);
        Em_PlaySeAtBoneOnFrame(em, 0x10A, Sound_RandomSe(0x17, 4, 0x18, 4), 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0xF8, Sound_RandomSe(0x17, 4, 0x18, 4), 0x1D);
        Em_PlaySeAtBoneOnFrame(em, 0xFE, Sound_RandomSe(0x17, 4, 0x18, 4), 0x15);
        Em_PlaySeAtBoneOnFrame(em, 0x12C, Sound_RandomSe(0x15, 4, 0x16, 4), 0x19);
        Em_PlaySeAtBoneOnFrame(em, 0x130, Sound_RandomSe(0x15, 4, 0x16, 4), 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x138, Sound_RandomSe(0x15, 4, 0x16, 4), 0x1D);
        Em_PlaySeAtBoneOnFrame(em, 0x140, Sound_RandomSe(0x15, 4, 0x16, 4), 0x15);
        Em_PlaySeAtBoneOnFrame(em, 0x230, Sound_RandomSe(0x15, 4, 0x16, 4), 0x1D);
        Em_PlaySeAtBoneOnFrame(em, 0x238, Sound_RandomSe(0x15, 4, 0x16, 4), 0x15);
        Em_PlaySeAtBoneOnFrame(em, 0xE2, 4, 0x12);
        Em_PlaySeAtBoneOnFrame(em, 0xE2, 5, 0x1A);
        Em_PlaySeAtBoneVolOnFrame(em, 0xE2, 0x64, 0x1E, 0x16);
        Em_PlaySeAtBoneOnFrame(em, 0x190, 0x1D, 0xA);
        Em_PlaySeAtBoneOnFrame(em, 0x1A4, 0x1E, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x1E0, 4, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1E0, 0x64, 0x1E, 0x16);
        Em_PlaySeAtBoneOnFrame(em, 0x16E, 0x35, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x16E, 0x11, 0x12);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0x244, 0x1F, 0, 8);
        EmShenGaoren_PlaySeOnFrame((s32) em, 0x1F4, 0x21, 0, 8);
        
    default:
        break;
    }
}

/* em_blangonga's .data (0x8C9C60-0x8CA080) */
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
EmBVec EmBlangonga_CarveThrowPt = {
#include "game/008C9E28.inc"
};
void *EmBlangonga_Callbacks[8] = {
    EmBlangonga_SpawnEffect01, EmBlangonga_SpawnEffect01, EmBlangonga_TableNop, EmBlangonga_UpdateMotionEffects,
    EmBlangonga_ScriptQuery, EmBlangonga_Reinit, EmBlangonga_AllowHit8, NULL,
};
EmCurveKey EmKushala_VelYCurves_0[6] = {
#include "game/008C9E60.inc"
};
EmCurveKey EmKushala_VelYCurves_1[12] = {
#include "game/008C9E90.inc"
};
EmCurveKey EmKushala_SpeedCurves_1[12] = {
#include "game/008C9EF0.inc"
};
EmCurveKey EmKushala_VelYCurves_2[14] = {
#include "game/008C9F50.inc"
};
EmCurveKey EmKushala_SpeedCurves_2[14] = {
#include "game/008C9FC0.inc"
};
EmCurveKey *EmKushala_VelYCurves[4] = {
    EmKushala_VelYCurves_0, EmKushala_VelYCurves_1, EmKushala_VelYCurves_2, NULL,
};
EmCurveKey *EmKushala_SpeedCurves[4] = {
    NULL, EmKushala_SpeedCurves_1, EmKushala_SpeedCurves_2, NULL,
};
EmBVec EmShenGaoren_SeOffsets[4] = {
#include "game/008CA050.inc"
};
