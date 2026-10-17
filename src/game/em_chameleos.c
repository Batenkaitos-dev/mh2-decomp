/*
 * Monster family: Chameleos (EM_KIND_CHAMELEOS). The code from the family's
 * setup function (EmChameleos_Setup, called by the per-kind dispatch
 * Em_TypeSetup) to the next family's. The state functions take the monster
 * and the work area its state dispatcher passes on (EmWork); Em.step is the
 * step within the current state. See include/em.h.
 */
#include "common.h"
#include "scene.h"
#include "quest.h"
#include "em.h"
#include "player.h"
#include "objattach.h"
#include "area.h"
/* prototypes of callees (round 13) */
void CamShake_ReqAIf(Em *obj, s32 i);
void Em_AreaExitPoint(Em *em, f32 *out);
s32 Area_ContainsPos(s32, f32 *);
void Em_EnterNextArea(Em *);
AreaRecord *Stage_GetAreaRecord(u16 x);
struct EmIdEntry;
f32 *Em_AreaEntryValue(Em *em, struct EmIdEntry *entry);
void Em_PlaySeAtBoneOnFrame(Em *em, s32 frame, s32 se, s32 bone);
s32 Sound_RandomSe(s32 id, s16 lo, s32 alt, s16 n);


/* prototypes */
void EmBulldrome_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void EmChameleos_ChangeState(Em *em, u16 group, u16 state, u8 mode);
u8 EmChameleos_UpdateCurves(Em *em);
s8 EmMine_CreateTimed0(Em *em, f32 *pos);
void EmRemobra_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void Em_BeginStatus4(Em *em);
void Em_BeginStatus5(Em *em);
void Em_BreakPart(Em *em, s32 part);
void Em_EndAllStatus(Em *em);
void Em_SetLayerMotion(Em *em, s32 motion, s32 blend, s32 frame, s32 layer);
void Em_SetMotion(Em *em, s32 motion, s32 a, s32 b);
/* unit's own declaration: with the definition's void (Em *em, s32 group, s32 state, u8 mode) (game/unk_701810.c), EmChameleos_ChangeState: 4 of 139 words differ, EmChameleos_SetGroup0State: 1 of 96 words differ ... */
void Em_SetStateSync(Em *em, u16 group, u16 state, u8 mode);
u16 Math_AngleXZ(f32 *from, f32 *to);
void Player_ClearFlag(void *, u32 flag);
s32 Player_MoveFall(Player *p);
void Player_SetFlag(void *obj, u32 flag);
s8 QuestEm_Appear(Em *em);
void EmRangedAttack_CreateAtBone(Em *, s32, s32, s32, s32, s32);
void * EmEffectAttack_Create(Em *, s32, s32, s32, s32);
ObjAttach *BulldromeAttack_Create(Em *em, s8 kind);
void ChameleosAttack_Create(Em *em, s32 k);
void RemobraAttack_Create(Em *em, u8 k);
void Em_MoveRotated(Em *em, void *src);
void Em_MoveRotatedAccel(Em *, void *);
void Em_LoadReactionSet(Em *em, u8 k);
void Em_ComputeGoal2(Em *em, f32 *target);
void Em_GetPitchToGoal(Em *em, s32 *out, f32 a, f32 b);
void Em_ClearLowHpNeed(Em *em);
void Em_SpawnLyingPuffs(Em *em, s32 bone, f32 *pos, f32 scale);
void Em_SpawnStatus4Effect(Em *em, s32 a);
void Em_SetHitReactMode(Em *em, u8 a);
/* unit's own declaration: with the definition's s32 (Em *em, s16 n, s16 max, s16 interval) (game/unk_708850.c), EmBulldrome_State0_0B: 97 of 147 words differ, EmChameleos_State0_12: 42 of 80 words differ */
s32 Em_RegenHp(Em *em, s32 n, s32 max, s32 interval);
void Em_SetHold2(Em *em);
void Em_UpdateFlight(Em *em, s32 flags, EmFlyParam *p);
s32 Em_StepToGoal(Em *em, u16 step, u8 flags);
u8 Em_TurnToGoal(Em *em, u16 step, u8 flags);
void Em_RestartScript(Em *em);
void Em_TargetPlayerInRange(Em *);
void EmChameleos_Init(Em *em);
void EmChameleos_StealItem(Em *em, Player *p);
void EmChameleos_State2_01(Em *, EmWork *);
void EmChameleos_State2_08(Em *, EmWork *);
void EmChameleos_State2_09(Em *, EmWork *);
void EmChameleos_State2_0D(Em *, EmWork *);
void EmChameleos_State2_11(Em *em, EmWork *work);
void EmChameleos_State3_06(Em *em, s32 arg1, u8 variant);
void EmChameleos_State3_07(Em *em, s32 arg1, u8 variant);
void EmChameleos_State6_00(Em *, EmWork *);
void EmChameleos_State6_01(Em *, EmWork *);
void EmChameleos_State6_02(Em *, EmWork *);
void EmChameleos_State5_00_Hunted(Em *, EmWork *);
void EmChameleos_State5_02_HuntedFalling(Em *, EmWork *);
void EmChameleos_RunGroup0(Em *em, EmWork *work);
void EmChameleos_RunGroup1(Em *em, EmWork *work);
void EmChameleos_RunGroup4(Em *em, EmWork *work);
void EmChameleos_SpawnGroundEffect(Em *em, s32 a1, s32 a2, f32 scale);
/* Chameleos' own meanings of the work area's bytes (EmWork, include/em.h, has the shared ones). */
typedef struct EmChameleosWork {
    u8 fxStep;                  /* 0x000: UpdateMotionEffects: 0, then 1 (runs MotionEffects) */
    u8 unk001[0x002 - 0x001];
    s16 fxMotion;               /* 0x002: the motion id MotionSounds compares */
    u8 unk004[0x005 - 0x004];
    u8 unk005;                  /* 0x005: SetGroup2State: 4; State2_06 counts it down */
    u8 unk006[0x010 - 0x006];
    s8 unk010;                  /* 0x010: group 2's states set it (0 or 1); Update reads it in group 2 */
    u8 unk011[0x013 - 0x011];
    u8 wantInvisible;           /* 0x013: Init: 2; UpdateInvisibility sets it from EmChameleos_WantsInvisible when 0 */
    s32 unk014;                 /* 0x014: Init: 0 */
    s32 unk018;                 /* 0x018: Init: 0x2000 */
    u8 unk01C[0x028 - 0x01C];
    s32 moveAngCX;              /* 0x028: angles X, Y, Z for Em_MoveRotatedAccel (UpdateCurves; Z at moveAngX) */
    s32 moveAngCY;              /* 0x02C */
    s32 moveAngX;               /* 0x030 */
    s32 moveAngY;               /* 0x034 */
    s32 moveAngZ;               /* 0x038 */
    s32 pitchStep;              /* 0x03C */
    u8 unk040[0x044 - 0x040];
    s8 curveNone;               /* 0x044: the three curves' run flags (StartCurves, UpdateCurves) */
    s8 curveVelY;               /* 0x045 */
    s8 curveSpeed;              /* 0x046 */
    u8 curveKind;               /* 0x047: the curve tables' index (StartCurves) */
    s16 curveFrame;             /* 0x048: the curve frame (StartCurves, UpdateCurves) */
    u8 unk04A[0x04C - 0x04A];
    u16 unk04C;                 /* 0x04C: Init: 0xFFFF; UpdateInvisibility */
    s16 unk04E;                 /* 0x04E: Init: 900; UpdateInvisibility counts it down */
    u8 unk050[0x100 - 0x050];
} EmChameleosWork;
typedef char EmChameleosWork_size_check[(sizeof(EmChameleosWork) == sizeof(EmWork)) ? 1 : -1];
void EmChameleos_MotionEffects(Em *em, EmChameleosWork *work);
void EmRemobra_MotionSounds(s32 arg0, EmWork *work);
void EmRemobra_Init(Em *em);
void EmRemobra_UpdateHop(Em *);
void EmRemobra_State2_00(Em *em);
void EmRemobra_State2_02(Em *, s32);
void EmRemobra_State2_03(Em *, s32, s32);
void EmRemobra_State2_09(Em *em, s32 variant);
void EmRemobra_State2_0A(Em *);
void EmRemobra_State2_0C(Em *em, s32 variant);
void EmRemobra_State2_0F(Em *, s32);
void EmRemobra_State2_15(Em *, s32);
void EmRemobra_State3_01(Em *, s32);
void EmRemobra_State5_00_Hunted(Em *);
void EmRemobra_State5_01(Em *em);
void EmRemobra_SpawnGroundEffect(Em *em, s32 a1, s32 a2, f32 scale);
void EmRemobra_MoveByVelocity(Em *em);
void EmRemobra_StartCurves(Em *em, u8 a);
u8 EmRemobra_UpdateCurves(Em *);
void EmChameleos_SetGroup0State(Em *em, u16 state, u8 mode);
void EmChameleos_SetGroup1State(Em *em, u16 state, u8 mode);
void EmChameleos_SetGroup2State(Em *em, u16 state, u8 mode);
void EmChameleos_SetGroup3State(Em *em, u16 state, u8 mode);
void EmChameleos_StartCurves(Em *em, u8 kind);
void EmChameleos_SetFlag43C(Em *em);
void EmChameleos_ClearFlag43C(Em *em);
s32 EmChameleos_IsInvisible(Em *em);
s32 EmChameleos_WantsInvisible(Em *);
s32 EmChameleos_InvisibilityBlocked(Em *em);
void EmChameleos_SpawnPeriodicEffect06(Em *em);
s32 func_00858980(Em *em);
void EmBulldrome_Init(Em *em);
void EmBulldrome_State0_0B(Em *, s32);
void EmBulldrome_State1_00(Em *, s32);
void EmBulldrome_State1_05(Em *em);
void EmBulldrome_State1_07(Em *em, u8 variant);
void EmBulldrome_State1_08(Em *em, u8 variant);
void EmBulldrome_State2_01(Em *);
void EmBulldrome_State2_02(Em *);
void EmBulldrome_State3_06(Em *);
void EmBulldrome_State5_00_Hunted(Em *);
void EmBulldrome_State5_01_Hunted(Em *);
void EmBulldrome_State6_00_Captured(Em *);
void EmBulldrome_State6_01(Em *);
void EmBulldrome_RunGroup8(Em *em);
void EmBulldrome_LaunchFromHit(Em *);
void EmBulldrome_SetMotionForSide(Em *em, s16 arg1);
void StageObj_K13_Create(void *owner, s16 v, u8 k);
void StageObj_Stun_CreateEm(Em *em, u8 k, s16 v, s16 t, f32 y, f32 scale);
s32 EmMotion_PassedFrameScaled(void *p, f32 frame, s32 layer);
s32 EmMotion_ReachedFrame(void *obj, s32 a, f32 frame);
s32 EmMotion_InFrameRange(void *em, s32 layer, f32 from, f32 to);
f32 Vec_Distance(f32 *a, f32 *b);
f32 Vec_DistanceXZ(f32 *a, f32 *b);
s32 Versus_BothDefeated(void);
s32 QuestEm_CanAppear(Em *em);
void Player_ClearVelocity(void *);
void Player_ClearAccel2(void *);
void Motion_InitTime(void *p, s16 step);
void Effect01_Spawn(void *obj, s16 type);
extern f32 gAreaGroundY[];
extern EmFlyParam EmChameleos_FlyParam;
extern EmFlyParam EmChameleos_FlyParam2;
f32 Math_Absf(f32 x);
void Em_InitParts(Em *em);
void Em_SetMotionRow(Em *em, s32 row, s32 arg2, s32 arg3);
s32 Em_ComputeMaxHp(Em *em, s16 idx);
void EmBulldrome_State8_00(Em *, EmWork *);
void Em_SetPartScaled(Em *em, s32 bit);
void QuestEm_LoadPersistentHp(Em *em);
u8 Quest_FindSpecialMonster(u8 id);
s32 Em_PartBroken(Em *em, s32 arg1);
void StageObj_K09_Create(Em *);
f32 *Mtx_FromAnglesXYZ(s32 *angles, f32 *m);
s32 Angle_TurnToward(s32 *cur, s32 *target, s32 step);
/* unit's own declaration: with the definition's void (void *obj, s16 bone, f32 *out) (sub_main/unk_3DDBA0.c), EmChameleos_SpawnGroundEffect: 11 of 31 words differ, EmChameleos_SpawnGroundEffectOffset: 53 of 68 words differ ... */
void Em_BonePos(void *obj, s32 a, f32 *out);
void Effect0D_SpawnOnGround(f32 *pos, s32 a, f32 scale);
s32 EmChameleos_HasFlag43C(Em *em);
u16 Map_DayId(u16 area);
void Em_BeginStatus1(Em *em);
void Em_BeginStatus2(Em *em);
void Em_EndStatus1(Em *em);
void Em_EndStatus2(Em *em);
void Em_EndStatus4(Em *em);
void Em_EndStatus5(Em *em);
void Em_RefillNeed3(Em *em);
void Em_ResetNotice(Em *em);
s32 Game_IsOnline(void);
void Versus_MarkDefeat(Em *);
void Em_ResetToBasePose(Em *em);
void Em_SetSpeedScaled(Em *em, u16 flags, f32 x, f32 y);
void Em_CheckRageByHp(Em *em, s16 n);
void Em_ComputeGoal(Em *em, f32 *target);
void Em_TypeSetup(Em *em);
void Em_SetTargetAngle(Em *em);
s32 Em_GetTargetPos(Em *em, u8 idx, f32 *out);
void Em_StartRage(Em *em);
void Em_StartPendingRage(Em *em);
void Vec_Set(f32 *out, f32 x, f32 y, f32 z);
void Em_StartGoalAction(Em *em, s32 arg1);
void Em_ClearPart(Em *em, u16 part);
void Em_AddHp(Em *em, s16 n);
void Em_AddNeed1(Em *em, s32 n);
void Em_AddNeed2(Em *em, s32 n);
void Em_ClearNeed1State(Em *em);
void Em_ClearNeed2State(Em *em);
s16 Player_IsState(void *obj, u8 state, u8 sub);
void MinePoint_Remove(s32);
void Vec_Copy(f32 *out, f32 *in);
u16 Rand_Next(s32 stream);
extern f32 EmBulldrome_Effect0BOfs[3];
void Em_ApplyPartBreaks(Em *em);
void Em_FlagPartBreak(Em *em, s32 arg1);
void EmImpactAttack_CreateAt(Em *em, f32 *pos, s8 kind);
/* unit's own declaration: with the definition's void (Em *em, s16 k, s16 c, f32 scale) (game/unk_874B90.c), EmChameleos_SpawnK14NearGround: 26 of 39 words differ */
void StageObj_K14_Create(Em *em, s32, s32, f32);
extern f32 D_008CAE40;
void StageObj_K25_Create(Player *p, Em *em, u8 k, s32 unused, f32 *pos);
void Versus_RouteEmByTeam(Em *em);
s32 Versus_DefeatCamera(Em *pl);
typedef struct EmBulldromeVec {
    f32 x, y, z;
} EmBulldromeVec;
extern EmBulldromeVec EmBulldrome_CarveThrowPt __attribute__((aligned(8)));
void EmChameleos_State1_01(Em *em, EmWork *work);
void EmChameleos_State1_02(Em *em, EmWork *work, u8 variant);
void EmChameleos_State1_06(Em *em, EmWork *work);
void EmChameleos_State4_03(Em *em, EmWork *work, u8 variant);
void EmChameleos_State0_0A(Em *em, EmWork *work, u8 variant);
void QuestEm_Hunted(Em *em);
s8 EmMine_Create(Em *em, s32 idx);
s32 EmMine_Update(Em *em);
void EmMine_Remove(Em *em);
void GmEm_ResetSize(Em *em);
s32 Save_TestEventFlag(s32);
void QuestEm_Captured(Em *em);
void Em_SetCaptured(Em *em, s32 a);
void Em_ClearPoison(Em *em);
void EmBulldrome_EndState(Em *em);
extern EmCurveKey *EmRemobra_VelYCurves[];
extern EmCurveKey *EmRemobra_SpeedCurves[];
void Vec_RotateY(f32 *v, f32 angle);
void EmEffectAttack_CreateTimed(Em *, s32, s32, s32, s32, s16);
void Em_MotionDelta(Em *em, f32 *out);
f32 *Mtx_FromAnglesYXZ(s32 *angles, f32 *m);
void Vec_Rotate(f32 *out, f32 *in, f32 *mtx);
extern EmTurnParams EmChameleos_TurnParams[1];
extern EmTurnParams EmBulldrome_TurnParams[1];
void Em_StartTurnToTarget(Em *em, EmTurnParams *t);
s32 Em_TurnStep(Em *em, EmTurnParams *t);
void Em_SelectSightRow(Em *em, u8 idx);
/* the calls that mask the value again (u8 or & 0xFF) cast it to s32 first: MWCC keeps the game's andi that way */
u8 Player_InCurrentArea(void *obj);
void Effect08_Spawn(f32 *pos, s32 k, s32 b, f32 scale);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);
struct StageObj *StageObj_K1C_CreateOnEm(struct Em *em, u8 k, f32 scale);
void EmChameleos_MotionSounds(Em *em, EmWork *work);
void StageObj_K11_CreateAtScaled(f32 *pos, s32 v, s16 k, f32 scale);
s32 Quest_IsRunning(s32 ending);
s32 Player_HasSkill(Player *p, u8 skill);
s32 Item_IsStealable(s32 arg0);
u8 Pouch_Add(Player *p, u16 item, s16 n);
u8 Notice_Push(u8 kind, s16 a, s16 b);
s32 Em_UpdateCombatTimer(Em *em);
void Em_UpdatePendingScript(Em *em);
void Em_ClearHitReactMode(Em *em);
u8 Em_UpdateDamage(Em *em, u8 *out);
void Em_SetStatus4Timer(Em *em);
void EmRemobra_RunState(Em *em);
void Em_RaisePlayerAreaTrigger(Em *em);
void Em_HoldPosNoPlayers(Em *em, u16 a, u16 b);
void Em_CheckLowHp(Em *em, f32 ratio);
void Em_SetStatus2Timer(Em *em);
void Em_SetStatus5Timer(Em *em);
void Em_ResetKindTimer(Em *em);
s32 ScEvent_CurId(void);
void EmBulldrome_RunState(Em *em);
u16 Area_DayId(u16 id);
void Em_RaisePlayerAreaTriggerLax(Em *em);
void Em_DecayNeed2(Em *em);
s32 Em_DamageResultIsReaction(Em *arg0, s32 arg1);
void CamReq_ForMonster(Em *em);
void EmChameleos_RunState(Em *em, s32 arg1);
s32 EmChameleos_CanTurnInvisible(Em *em);
void EmChameleos_BreakParts(Em *em, s32 apply);
void EmChameleos_UpdateInvisibility(Em *em);
/* end of prototypes */

void EmChameleos_Setup(Em *em)
{
    EmChameleos_SetFlag43C(em);
}

void EmChameleos_SpawnEffects(void *em)
{
    StageObj_K13_Create(em, 0x22, 0);
    Effect01_Spawn((Player *) em, 0);
}

void EmChameleos_Init(Em *em)
{
    EmChameleosWork *w = (EmChameleosWork *)&em->work;

    f32 fslot;
    s16 size;
    u8 slotNo;

    Em_SetPartScaled(em, 3);
    Em_SetPartScaled(em, 4);
    Em_SetPartScaled(em, 8);
    if (gQuest.stage == 0) {
        em->rotY = 0;
        slotNo = em->slotNo;
        fslot = (f32) (u32) slotNo;
        em->posX = 5000.0f + (1500.0f * fslot);
        em->posY = 0.0f;
        em->posZ = 9000.0f;
    }
    Em_SetMotionRow(em, 1, 0, 0);
    em->stance = 0;
    if ((Quest_FindSpecialMonster(em->type) & 0xFF) == 0xFF) {
        size = Em_ComputeMaxHp(em, 0);
        em->hp = size;
        em->hpMax = size;
    } else {
        QuestEm_LoadPersistentHp(em);
    }
    em->scriptPending = 1;
    em->blind = 0;
    em->unk_InitFlag = 1;
    em->aimAngle = 0x4000;
    w->moveAngY = 0x100;
    w->moveAngZ = 0x200;
    w->pitchStep = 0x100;
    w->unk018 = 0x2000;
    EmChameleos_SetFlag43C(em);
    w->unk014 = 0;
    w->unk04C = 0xFFFF;
    w->unk04E = 0x384;
    w->wantInvisible = 2;
    em->meshShown[1] = 0;
    if (Em_PartBroken(em, 1) == 0) {
        StageObj_K09_Create(em);
        return;
    }
    em->fxFlags |= 1;
    em->partBreakCount = 1;
}

void EmChameleos_Start(Em *em)
{
    EmChameleos_Init(em);
    EmChameleos_ChangeState(em, 0, 1, 0);
}

void EmChameleos_Reinit(Em *em)
{
    EmChameleos_Init(em);
}

void EmChameleos_EndState(Em *em, s32 unused1, s32 unused2)
{
    em->lookAtTarget = 0;
    em->scriptPending = 1;
    em->motionSpeed = 1.0f;
    em->stance = 0;
    EmChameleos_ChangeState(em, 0, 1, 0);
}

void EmChameleos_EndState2(Em *em, u8 next)
{
    s32 k;

    k = next & 0xFF;
    em->lookAtTarget = 0;
    em->scriptPending = 1;
    em->motionSpeed = 1.0f;
    em->stance = 2;
    switch (k) {
    case 0:
        EmChameleos_ChangeState(em, 2, 2, 0);
        break;
    case 1:
        EmChameleos_ChangeState(em, 2, 0xF, 0);
        break;
    case 2:
        EmChameleos_ChangeState(em, 2, 3, 0);
        break;
    }
}

void EmChameleos_StealItem(Em *em, Player *p)
{
    s32 i;
    s32 nRare;
    s32 nNormal;
    s16 item;
    s16 n;
    s32 rare[20];
    s32 normal[20];

    if (p->index == gScene.localPlayer && Quest_IsRunning(1) == 0 && Player_HasSkill(p, 0x5C) != 1) {
        nRare = 0;
        nNormal = 0;
        for (i = 0; i < 20; i++) {
            if (p->pouch[i].item != 0 && gItemData[p->pouch[i].item].type == 0 && gItemData[p->pouch[i].item].rarity < 4) {
                if (Item_IsStealable(p->pouch[i].item) == 1) {
                    rare[nRare++] = i;
                } else {
                    normal[nNormal++] = i;
                }
            }
        }
        if (nRare > 0) {
            item = p->pouch[rare[em->stateRand % nRare]].item;
            n = -1;
        } else if (nNormal > 0) {
            item = p->pouch[normal[em->stateRand % nNormal]].item;
            n = -1;
        } else {
            return;
        }
        Pouch_Add(p, item, n);
        if (p->index == gScene.localPlayer) {
            Notice_Push(1, 6, item);
        }
    }
}

void EmChameleos_State0_00(Em *em)
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
        em->lookAtTarget = 1;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->scriptPending = 1;
        }
        break;
    }
}

void EmChameleos_State0_01(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 1;
        Em_SetMotion(em, 1, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
        case 2:
            if (em->remote == 0) {
                em->step++;
                EmChameleos_EndState(em, 0, 0);
            }
        }
        break;
    }
}

void EmChameleos_State0_02(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 1;
        Em_SetMotion(em, 0x1F, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
        case 2:
            if (em->remote == 0) {
                em->step++;
                EmChameleos_EndState(em, 0, 0);
            }
        }
        break;
    }
}

void EmChameleos_State0_03(Em *em)
{
    u16 aimAngle;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x20, 0, 0);
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
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmChameleos_State0_04(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x22, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmChameleos_State0_05(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x22, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmChameleos_State0_09(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x21, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmChameleos_State0_0A(Em *em, EmWork *work, u8 variant)
{
    f32 pos[3];
    f32 v[3];

    Em_SetHitReactMode(em, EM_HIT_REACT_NO_FLINCH);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x31, 0, 0);
        Em_SelectSightRow(em, 1);
        Em_LoadReactionSet(em, 1);
        switch (variant) {
        case 0:
        default:
            em->timer = 1;
            break;
        case 1:
            em->timer = 3;
            break;
        }
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 36.0f, 0) != 0) {
            switch (variant) {
            default:
                EmChameleos_SpawnGroundEffect(em, 0x29, 0x14, 4.0f);
                break;
            case 1:
                if (Player_InCurrentArea(em)) {
                    v[0] = 0.0f;
                    v[1] = 0.0f;
                    v[2] = 200.0f * em->scaleZ;
                    Em_BonePos(em, 0x29, pos);
                    Vec_RotateY(v, 6.2831855f * em->rotY / 65536.0f);
                    pos[0] += v[0];
                    pos[1] = 5.0f + em->floorY;
                    pos[2] += v[2];
                    Effect08_Spawn(pos, 0, 0, 2.0f);
                    Sound_PlaySeAtPosChecked(1, 0x2E, 0, pos, 1, 0);
                }
                break;
            }
        }
        if (em->motion[0].playing == 0) {
            if (--em->timer <= 0) {
                em->step++;
                Em_AddHp(em, 0.05f * em->hpMax);
                switch (variant) {
                default:
                    Em_AddNeed1(em, em->need1Max);
                    Em_ClearNeed1State(em);
                    break;
                case 1:
                    Em_AddNeed2(em, em->need2Max);
                    Em_ClearNeed2State(em);
                    break;
                }
                Em_SelectSightRow(em, 0);
                Em_LoadReactionSet(em, 0);
                EmChameleos_EndState(em, 0, 0);
            } else {
                Em_SetMotion(em, 0x31, 0, 0);
            }
        }
        break;
    }
}

void EmChameleos_State0_12(Em *em)
{
    em->blind = 2;
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x24, 0, 0);
        Em_LoadReactionSet(em, 1);
        em->timer = 0x2328;
        break;
    case 1:
        EmChameleos_SpawnPeriodicEffect06(em);
        Em_RegenHp(em, 1, (s32) (0.5f * (f32) em->hpMax), 6);
        if (--em->timer <= 0) {
            em->step++;
            Em_ClearLowHpNeed(em);
            EmChameleos_ChangeState(em, 0, 0x17, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmChameleos_ChangeState(em, 0, 0x17, 4);
        }
        break;
    }
}

void EmChameleos_State0_14(Em *em)
{
    em->blind = 2;
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x47, 0, 0);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        EmChameleos_SpawnPeriodicEffect06(em);
        if (--em->timer <= 0) {
            em->step++;
            EmChameleos_ChangeState(em, 0, 0x18, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmChameleos_ChangeState(em, 0, 0x18, 4);
        }
        break;
    }
}

void EmChameleos_State0_17(Em *em)
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
            em->blind = 0;
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmChameleos_State0_18(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x43, 0, 0);
        Em_LoadReactionSet(em, 0);
        em->blind = 2;
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->blind = 0;
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmChameleos_State0_19(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x46, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmChameleos_State0_1B(Em *em)
{
    em->blind = 2;
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x47, 0, 0);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        EmChameleos_SpawnPeriodicEffect06(em);
        if (--em->timer <= 0) {
            em->step++;
            EmChameleos_ChangeState(em, 0, 0x1C, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmChameleos_ChangeState(em, 0, 0x1C, 4);
        }
        break;
    }
}

void EmChameleos_State0_1C(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x43, 0, 0);
        Em_LoadReactionSet(em, 0);
        em->blind = 2;
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->blind = 0;
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmChameleos_State0_20(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x46, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmChameleos_State1_00(Em *em)
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
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmChameleos_State1_01(Em *em, EmWork *work)
{
    f32 pos[3];
    s32 d;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x23, 0, 0);
        break;
    case 1:
        if (em->hasGoal != 0) {
            d = (Math_AngleXZ(&em->posX, em->goal) - em->rotY) & 0xFFFF;
            if (d <= 0x8000) {
                if (d <= 0x3F) {
                    em->rotY += d;
                } else {
                    em->rotY += 0x40;
                }
            } else if (d > 0xFFC0) {
                em->rotY += d;
            } else {
                em->rotY -= 0x40;
            }
            Em_MotionDelta(em, pos);
            em->goalDist -= pos[2];
            if (em->goalDist <= 0.0f) {
                em->timer = 1;
            }
        }
        if (--em->timer <= 0) {
            em->step++;
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmChameleos_State1_02(Em *em, EmWork *work, u8 variant)
{
    f32 v[3];

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        em->speedBonus = 30;
        switch (variant) {
        case 0:
            break;
        case 1:
            v[0] = 0.0f;
            v[1] = 0.0f;
            v[2] = 500.0f;
            Vec_RotateY(v, 6.2831855f * em->stateRand / 65536.0f);
            em->goal[0] = em->posX + v[0];
            em->goal[1] = em->posY;
            em->goal[2] = em->posZ + v[2];
            break;
        }
        Em_StartTurnToTarget(em, &EmChameleos_TurnParams[0]);
        break;
    case 1:
        if (Em_TurnStep(em, &EmChameleos_TurnParams[0]) == 1) {
            em->step++;
            Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmChameleos_State1_04(Em *em, s32 unused, s32 variant)
{
    s32 k;

    switch (em->step) {
    case 0:
        em->step++;
        em->stateVar0 = 0;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 3, 0, 0);
        break;
    case 1:
        k = variant & 0xFF;
        switch (k) {
        case 0:
            if ((em->stateVar0 == 0) && (EmMotion_PassedFrameScaled(em, 34.0f, 0U) != 0)) {
                ChameleosAttack_Create(em, 6);
                em->stateVar0++;
            }
            break;
        case 1:
            break;
        }
        if (Em_StepToGoal(em, 0x40, 0) == 1) {
            em->step++;
            Em_SetMotion(em, 4, 0, 0);
        }
        break;
    case 2:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmChameleos_State1_05(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 7, 0, 0);
        break;
    case 1:
        if (EmMotion_InFrameRange(em, 0, 44.0f, 90.0f) != 0) {
            em->rotY -= 0x590;
        }
        if (em->motion[1].playing == 0) {
            em->step++;
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmChameleos_State1_06(Em *em, EmWork *work)
{
    f32 v[3];
    f32 out[3];
    s32 ang[3];
    f32 mtx[16];

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 9, 0, 0);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 44.0f) != 0 && EmMotion_ReachedFrame(em, 0, 82.0f) == 0) {
            em->stance = 2;
            ang[0] = 0;
            ang[1] = em->rotY;
            ang[2] = 0;
            Vec_Set(v, 0.0f, 0.0f, -139.46706f);
            Mtx_FromAnglesYXZ(ang, mtx);
            Vec_Rotate(out, v, mtx);
            em->posX += out[0];
            em->posZ += out[2];
        } else {
            em->stance = 0;
        }
        if (em->motion[1].playing == 0) {
            em->stance = 0;
            em->step++;
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmChameleos_State2_00(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0xA, 0, 0);
        EmChameleos_StartCurves(em, 0U);
        ((EmChameleosWork *)work)->unk010 = 0;
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 58.0f, 0U) != 0) {
            em->step++;
            em->stance = 2;
        }
        break;
    case 2:
        EmChameleos_UpdateCurves(em);
        if (EmMotion_PassedFrameScaled(em, 72.0f, 0U) != 0) {
            em->step++;
        }
        break;
    case 3:
        em->posY += 30.0f;
        if (!(em->posY < (em->floorY + (600.0f * em->scaleY)))) {
            em->step++;
            Em_SetMotion(em, 0x13, 0, 0);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmChameleos_EndState2(em, 0U);
        }
        break;
    }
}

void EmChameleos_State2_01(Em *em, EmWork *work)
{
    f32 floorY;
    f32 dy;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        em->rotX = 0;
        em->rotZ = 0;
        Em_SetMotion(em, 0x12, 0, 0);
        Player_ClearVelocity(em);
        em->velY = -10.0f;
        ((EmChameleosWork *)work)->unk010 = 0;
        work->moveAngCX = 0;
        work->moveAngX = 0;
        floorY = em->floorY;
        dy = em->posY - (1000.0f + floorY);
        if (dy > 0.0f) {
            do {
                em->velY -= 0.75f;
                dy += em->velY;
            } while (dy > 0.0f && !(em->velY < -200.0f));
        }
        break;
    case 1:
        work->moveAngCY = em->rotY;
        Em_MoveRotated(em, &work->moveAngCX);
        if (em->velY < -10.0f) {
            em->velY += 0.75f;
        }
        if (!((em->floorY + (500.0f * em->scaleY)) <= em->posY)) {
            em->step++;
            Em_SetMotion(em, 0x11, 0, 0);
            em->posY = em->floorY;
            em->rotX = 0;
            em->rotZ = 0;
            em->stance = 0;
            CamShake_ReqAIf(em, 1);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmChameleos_State2_02(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        Em_SetMotion(em, 0x12, 0, 0);
        ((EmChameleosWork *)work)->unk010 = 0;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmChameleos_EndState2(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmChameleos_State2_03(void)
{
}

void EmChameleos_State2_04(void)
{
}

void EmChameleos_State2_05(void)
{
}

void EmChameleos_State2_06(Em *em, EmWork *work)
{
    f32 floorY;
    s32 t;
    u8 n;
    u8 stateVar1;

    switch (em->step) {
    case 0:
        em->stance = 2;
        Em_SetMotion(em, 0xD, 0, 0);
        em->step++;
        em->stateVar1 = 0;
        em->lookAtTarget = 0;
        em->velX = 0.0f;
        em->velY = 0.0f;
        Player_ClearAccel2(em);
        if (em->velZ <= 50.0f) {
            em->velZ = 50.0f;
        }
        ((EmChameleosWork *)work)->unk010 = 1;
        break;
    case 1:
        stateVar1 = em->stateVar1;
        if ((stateVar1 == 0) && (em->motion[0].playing == 0)) {
            em->stateVar1 = stateVar1 + 1;
            Em_SetMotion(em, 0xC, 0, 0);
        }
        if ((Vec_DistanceXZ(&em->posX, em->goal) <= 3000.0f) || (t = em->timer - 1, em->timer = t, (t <= 0))) {
            n = ((EmChameleosWork *)work)->unk005 - 1;
            ((EmChameleosWork *)work)->unk005 = n;
            if ((n & 0xFF) <= 0) {
                EmChameleos_EndState2(em, 1U);
            } else {
                em->curGoalNo++;
                em->curGoalNo &= 3;
                Em_ComputeGoal2(em, em->goal);
                em->timer = (s32) ((1.5f * Vec_DistanceXZ(&em->posX, em->goal)) / 50.0f);
                EmChameleos_ChangeState(em, 2, 6, 0);
            }
        }
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 0xE1, &EmChameleos_FlyParam);
        work->moveAngCX = em->rotX;
        work->moveAngCY = em->rotY;
        work->moveAngX = 0;
        Em_MoveRotated(em, &work->moveAngCX);
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmChameleos_State2_07(Em *em, EmWork *work)
{
    f32 dist;
    f32 floorY;
    s32 timer;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        work->moveAngZ = 0x100;
        Em_SetMotion(em, 0x12, 0, 0);
        ((EmChameleosWork *)work)->unk010 = 0;
        break;
    case 1:
        Em_TurnToGoal(em, (s32) (u16) work->moveAngZ, 1);
        work->moveAngCY = em->rotY;
        work->moveAngX = 0;
        Em_GetPitchToGoal(em, &work->moveAngCX, 0.0f, 0.0f);
        Em_MoveRotated(em, &work->moveAngCX);
        dist = Vec_DistanceXZ(&em->posX, em->goal);
        timer = em->timer - 1;
        em->timer = timer;
        if ((timer <= 0) || (dist <= (10.0f * em->velZ))) {
            em->step++;
            Player_ClearVelocity(em);
            EmChameleos_EndState2(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void Hud_TriggerGlow(Em *em);
void EmChameleos_State2_08(Em *em, EmWork *work)
{
    f32 floorY;
    u16 next;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        Player_ClearVelocity(em);
        em->velZ = 50.0f;
        Em_SetMotion(em, 0xC, 0, 0);
        Em_AreaExitPoint(em, (f32 *) em->goal);
        ((EmChameleosWork *)work)->unk010 = 1;
        break;
    case 1:
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 0xE1, &EmChameleos_FlyParam);
        work->moveAngCX = em->rotX;
        work->moveAngCY = em->rotY;
        work->moveAngX = 0;
        Em_MoveRotated(em, &work->moveAngCX);
        if (Area_ContainsPos((s32) em->area, &em->posX) == 0) {
            em->step++;
        }
        break;
    case 2:
        em->step++;
        break;
    case 3:
        em->step++;
        em->timer = 0x258;
        Em_EnterNextArea(em);
        next = *(u16 *) &em->destArea;
        if (em->nextArea == next || next == AREA_NONE) {
            Hud_TriggerGlow(em);
            EmChameleos_ChangeState(em, 2, 9, 1);
        } else {
            if (em->remote == 0) {
                em->nextArea = next;
                em->goalNo = next;
                em->goalKind = 3;
                Em_ComputeGoal(em, em->goal);
            }
            EmChameleos_ChangeState(em, 2, 0xD, 1);
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
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmChameleos_State2_09(Em *em, EmWork *work)
{
    f32 d;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        work->moveAngZ = 0x100;
        Em_SetMotion(em, 0x12, 0, 0);
        Player_ClearVelocity(em);
        em->velZ = 80.0f;
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->rotY = (s32) em->aimAngle;
        *(u16 *) &em->destArea = AREA_NONE;
        ((EmChameleosWork *)work)->unk010 = 1;
        break;
    case 1:
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 0xE5, &EmChameleos_FlyParam);
        work->moveAngCX = em->rotX;
        work->moveAngCY = em->rotY;
        work->moveAngX = 0;
        Em_MoveRotated(em, &work->moveAngCX);
        d = Vec_DistanceXZ(&em->posX, em->goal);
        em->timer--;
        if (d <= 1000.0f || em->timer < 0) {
            em->step++;
            em->timer = 600;
            EmChameleos_ChangeState(em, 2, 1, 1);
        }
        break;
    case 2:
        if (em->remote == 0) {
            if (--em->timer <= 0) {
                em->timer = 0x258;
                EmChameleos_ChangeState(em, 2, 1, 1);
            }
        }
        break;
    }
    if (em->posY < em->floorY) {
        em->posY = em->floorY;
    }
}

void EmChameleos_State2_0A(void)
{
}

void EmChameleos_State2_0B(Em *em, EmWork *work)
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
        ((EmChameleosWork *)work)->unk010 = 1;
        break;
    case 1:
        if (Vec_DistanceXZ(&em->posX, em->goal) <= 1000.0f) {
            if (!(em->velZ <= 50.0f)) {
                em->velZ = 50.0f;
            }
            em->step++;
            em->timer = 0x12C;
            EmChameleos_ChangeState(em, 2, 1, 1);
        } else {
            em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
            em->aimAngle = em->aimAngle - em->rotY;
            Em_UpdateFlight(em, 0xE1, &EmChameleos_FlyParam);
            work->moveAngCX = em->rotX;
            work->moveAngCY = em->rotY;
            work->moveAngX = 0;
            Em_MoveRotated(em, &work->moveAngCX);
            if (--em->timer <= 0) {
                em->step++;
                em->timer = 0x12C;
                EmChameleos_ChangeState(em, 2, 1, 1);
            }
        }
        break;
    case 2:
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 0xE1, &EmChameleos_FlyParam);
        work->moveAngCX = em->rotX;
        work->moveAngCY = em->rotY;
        work->moveAngX = 0;
        Em_MoveRotated(em, &work->moveAngCX);
        if (--em->timer <= 0) {
            em->timer = 0x12C;
            if (em->remote == 0) {
                EmChameleos_ChangeState(em, 2, 1, 1);
            }
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmChameleos_State2_0C(void)
{
}

void EmChameleos_State2_0D(Em *em, EmWork *work)
{
    AreaRecord *area;
    f32 *pt;
    u16 next;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        work->moveAngZ = 0x100;
        Em_SetMotion(em, 0x12, 0, 0);
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
        ((EmChameleosWork *)work)->unk010 = 1;
        em->timer = 900;
        break;
    case 1:
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 0xE5, &EmChameleos_FlyParam);
        work->moveAngCX = em->rotX;
        work->moveAngCY = em->rotY;
        work->moveAngX = 0;
        Em_MoveRotated(em, &work->moveAngCX);
        if (Vec_DistanceXZ(&em->posX, em->goal) <= 1000.0f || --em->timer <= 0) {
            em->step++;
            em->timer = 900;
            Em_AreaExitPoint(em, (f32 *) em->goal);
        }
        break;
    case 2:
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 0xE1, &EmChameleos_FlyParam);
        work->moveAngCX = em->rotX;
        work->moveAngCY = em->rotY;
        work->moveAngX = 0;
        Em_MoveRotated(em, &work->moveAngCX);
        if (Area_ContainsPos(em->area, &em->posX) == 0 || --em->timer <= 0) {
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
            EmChameleos_ChangeState(em, 2, 9, 1);
        } else {
            if (em->remote == 0) {
                em->nextArea = next;
                em->goalNo = next;
                em->goalKind = 3;
                Em_ComputeGoal(em, em->goal);
            }
            EmChameleos_ChangeState(em, 2, 0xD, 1);
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

void EmChameleos_State2_0E(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        Em_SetMotion(em, 0x12, 0, 0);
        ((EmChameleosWork *)work)->unk010 = 0;
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
            EmChameleos_EndState2(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmChameleos_State2_0F(Em *em, EmWork *work)
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
        ((EmChameleosWork *)work)->unk010 = 1;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmChameleos_EndState2(em, 1U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmChameleos_State2_10(Em *em, EmWork *work)
{
    f32 floorY;
    s32 done;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        Em_SetMotion(em, 0x12, 0, 0);
        work->moveAngZ = 0x180;
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        ((EmChameleosWork *)work)->unk010 = 0;
        em->timer = 0x96;
        break;
    case 1:
        done = Em_TurnToGoal(em, (s32) (u16) work->moveAngZ, 1) & 0xFF;
        if (--em->timer > 0) {
            if (done != 0) {
                goto block_9;
            }
        } else {
block_9:
            em->step++;
            EmChameleos_EndState2(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmChameleos_State2_11(Em *em, EmWork *work)
{
    f32 floorY;
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
            Em_SetMotion(em, 0xD, 0, 0);
        } else {
            em->stateVar1 = 1;
            if ((u32) (em->motionId[0] - 0x3F4) > 1) {
                Em_SetMotion(em, 0xD, 0, 0);
            }
        }
        ((EmChameleosWork *)work)->unk010 = 1;
        break;
    case 1:
        stateVar1 = em->stateVar1;
        if ((stateVar1 == 0) && (em->motion[0].playing == 0)) {
            em->stateVar1 = stateVar1 + 1;
            Em_SetMotion(em, 0xC, 0, 0);
        }
        if (!(Vec_DistanceXZ(&em->posX, em->goal) <= 1000.0f)) {
            if (--em->timer <= 0) {
                goto block_16;
            }
        } else {
block_16:
            em->curGoalNo++;
            em->curGoalNo &= 3;
            Em_ComputeGoal2(em, em->goal);
            em->timer = (s32) ((1.5f * Vec_DistanceXZ(&em->posX, em->goal)) / 50.0f) + 0x96;
            em->step++;
            EmChameleos_ChangeState(em, 2, 0x11, 0);
        }
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 0xE1, &EmChameleos_FlyParam2);
        work->moveAngCX = em->rotX;
        work->moveAngCY = em->rotY;
        work->moveAngX = 0;
        Em_MoveRotated(em, &work->moveAngCX);
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmChameleos_State2_12(Em *em, EmWork *work)
{
    f32 floorY;
    s32 rotX;
    s32 rotZ;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x10, 0, 0);
        EmChameleos_StartCurves(em, 2U);
        ((EmChameleosWork *)work)->unk010 = 0;
        break;
    case 1:
        rotX = em->rotX;
        if (rotX != 0) {
            if (rotX < 0x8000) {
                em->rotX = rotX - work->moveAngY;
            } else {
                em->rotX = rotX + work->moveAngY;
            }
            em->rotX = (s32) (u16) em->rotX;
        }
        rotZ = em->rotZ;
        if (rotZ != 0) {
            if (rotZ < 0x8000) {
                em->rotZ = rotZ - work->pitchStep;
            } else {
                em->rotZ = rotZ + work->pitchStep;
            }
            em->rotZ = (s32) (u16) em->rotZ;
        }
        if (EmChameleos_UpdateCurves(em) & 0xFF) {
            em->step++;
            Player_ClearVelocity(em);
            Em_SetMotion(em, 0xF, 0, 0);
            em->timer = 0x12C;
            EmChameleos_ChangeState(em, 2, 1, 1);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->timer = 0x12C;
            Em_SetMotion(em, 0x12, 0, 0);
            if (em->remote == 0) {
                EmChameleos_ChangeState(em, 2, 1, 1);
            }
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmChameleos_State2_13(Em *em, EmWork *work)
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
            Em_SetMotion(em, 0xD, 0, 0);
        } else {
            em->stateVar1 = 1;
            motionId = em->motionId[0];
            if ((motionId != 0x3F4) && (motionId != 0x3F6)) {
                Em_SetMotion(em, 0xD, 0, 0);
            }
        }
        ((EmChameleosWork *)work)->unk010 = 1;
        break;
    case 1:
        stateVar1 = em->stateVar1;
        if ((stateVar1 == 0) && (em->motion[0].playing == 0)) {
            em->stateVar1 = stateVar1 + 1;
            Em_SetMotion(em, 0xC, 0, 0);
        }
        if (!(Vec_DistanceXZ(&em->posX, em->goal) <= 1000.0f)) {
            if (--em->timer <= 0) {
                goto block_17;
            }
        } else {
block_17:
            em->step++;
            EmChameleos_EndState2(em, 1U);
        }
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 0xE1, &EmChameleos_FlyParam2);
        work->moveAngCX = em->rotX;
        work->moveAngCY = em->rotY;
        work->moveAngX = 0;
        Em_MoveRotated(em, &work->moveAngCX);
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmChameleos_State2_14(void)
{
}

void EmChameleos_State2_15(Em *em, EmWork *work)
{
    f32 dist;
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        work->moveAngZ = 0x100;
        Em_SetMotion(em, 0x12, 0, 0);
        ((EmChameleosWork *)work)->unk010 = 0;
        break;
    case 1:
        work->moveAngCY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        work->moveAngX = 0;
        Em_GetPitchToGoal(em, &work->moveAngCX, 0.0f, 0.0f);
        Em_MoveRotated(em, &work->moveAngCX);
        dist = Vec_Distance((f32 *) &em->posX, (f32 *) em->goal);
        if (--em->timer > 0) {
            if (dist <= (10.0f * em->velZ)) {
                goto block_9;
            }
        } else {
block_9:
            em->step++;
            Player_ClearVelocity(em);
            EmChameleos_EndState2(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmChameleos_State2_16(Em *em, EmWork *work)
{
    f32 floorY;
    s32 rotX;
    s32 rotZ;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x10, 0, 0);
        EmChameleos_StartCurves(em, 2U);
        ((EmChameleosWork *)work)->unk010 = 0;
        break;
    case 1:
        rotX = em->rotX;
        if (rotX != 0) {
            if (rotX < 0x8000) {
                em->rotX = rotX - work->moveAngY;
            } else {
                em->rotX = rotX + work->moveAngY;
            }
            em->rotX = (s32) (u16) em->rotX;
        }
        rotZ = em->rotZ;
        if (rotZ != 0) {
            if (rotZ < 0x8000) {
                em->rotZ = rotZ - work->pitchStep;
            } else {
                em->rotZ = rotZ + work->pitchStep;
            }
            em->rotZ = (s32) (u16) em->rotZ;
        }
        if (EmChameleos_UpdateCurves(em) & 0xFF) {
            em->step++;
            Player_ClearVelocity(em);
            EmChameleos_EndState2(em, 0U);
        }
        break;
    case 2:
        EmChameleos_UpdateCurves(em);
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmChameleos_State2_17(void)
{
}

void EmChameleos_State2_18(Em *em, EmWork *work)
{
    f32 pos[3];
    f32 floorY;
    s32 timer;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        Em_SetMotion(em, 0x12, 0, 0);
        ((EmChameleosWork *)work)->unk010 = 0;
        work->moveAngCX = 0;
        work->moveAngX = 0;
        Player_ClearVelocity(em);
        em->velY = 20.0f;
        break;
    case 1:
        work->moveAngCY = Math_AngleXZ(&em->posX, pos) & 0xFFFF;
        Em_MoveRotated(em, &work->moveAngCX);
        timer = em->timer - 1;
        em->timer = timer;
        if ((timer <= 0) || !(em->posY <= (1000.0f + em->goal[1]))) {
            em->step++;
            Player_ClearVelocity(em);
            EmChameleos_EndState2(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmChameleos_State3_00(Em *em, s32 unused, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 1;
        if (!(variant & 0xFF)) {
            Em_SetMotion(em, 0x30, 0, 0);
            break;
        }
        Em_SetMotion(em, 0x31, 0, 0);
        em->step++;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x31, 0, 0);
        }
        break;
    case 2:
        if (EmMotion_PassedFrameScaled(em, 36.0f, 0U) != 0) {
            EmChameleos_SpawnGroundEffect(em, 0x29, 0x14, 4.0f);
        }
        if (func_00858980(em) == 1) {
            em->step++;
            EmChameleos_StealItem(em, em->contact);
        }
        
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmChameleos_State3_01(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x29, 0, 0);
        break;
    case 1:
        if (func_00858980(em) == 1) {
            em->step++;
            EmChameleos_StealItem(em, em->contact);
        }
        
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmChameleos_State3_02(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 0x64;
        em->stance = 0;
        Em_SetMotion(em, 0x2D, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmChameleos_State3_03(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 0x64;
        em->stance = 0;
        Em_SetMotion(em, 0x2E, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmChameleos_State3_04(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 1;
        Em_SetMotion(em, 0x30, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x32, 0, 0);
        }
        break;
    case 2:
        if (func_00858980(em) == 1) {
            em->step++;
            EmChameleos_StealItem(em, em->contact);
        }
        
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmChameleos_State3_05(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 1;
        Em_SetMotion(em, 0x33, 0, 0);
        break;
    case 1:
        if (func_00858980(em) == 1) {
            em->step++;
            EmChameleos_StealItem(em, em->contact);
        }
        
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmChameleos_State3_06(Em *em, s32 unused, u8 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x2A, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            if (variant == 0) {
                em->timer = 10;
            } else {
                em->timer = 60;
            }
            Em_SetMotion(em, 0x2B, 0, 0);
        }
        break;
    case 2:
        if (variant == 0) {
            if (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0) {
                EmEffectAttack_Create(em, 9, 0, 0x71C, 0);
            }
        } else {
            if (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0) {
                EmEffectAttack_CreateTimed(em, 0xB, 0, 0xAAB, 0, em->timer - 20);
            }
        }
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x2C, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmChameleos_State3_07(Em *em, s32 unused, u8 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x2A, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            if (variant == 0) {
                em->timer = 10;
            } else {
                em->timer = 60;
            }
            Em_SetMotion(em, 0x34, 0, 0);
        }
        break;
    case 2:
        if (variant == 0) {
            if (EmMotion_PassedFrameScaled(em, 6.0f, 0) != 0) {
                EmEffectAttack_Create(em, 9, 0, 0, 0);
            }
        } else {
            if (EmMotion_PassedFrameScaled(em, 6.0f, 0) != 0) {
                EmEffectAttack_CreateTimed(em, 0xB, 0, 0, 0, em->timer - 20);
            }
        }
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x35, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmChameleos_State3_08(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x2F, 0, 0);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 88.0f) != 0) {
            em->step++;
            em->stance = 2;
            ((EmChameleosWork *)work)->unk010 = 0;
            work->moveAngCX = 0;
            work->moveAngCY = em->rotY;
            work->moveAngX = 0;
            Player_ClearVelocity(em);
            em->velY = 12.0f;
            em->velZ = -7.0f;
            EmEffectAttack_Create(em, 9, 1, 0x4000, 0);
        }
        break;
    case 2:
        Em_MoveRotated(em, &work->moveAngCX);
        if (em->motion[1].playing == 0) {
            em->step++;
            EmChameleos_EndState2(em, 0U);
        }
        break;
    }
}

void EmChameleos_State3_0C(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 1;
        Em_SetMotion(em, 0x30, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmChameleos_State4_00(Em *em)
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
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmChameleos_State4_01(Em *em)
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
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmChameleos_State4_02(Em *em, s32 unused, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x49, 0, 0);
        if ((variant & 0xFF) == 1) {
            EmMine_CreateTimed0(em, &em->posX);
        }
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 96.0f, 0U) != 0) {
            em->step++;
            Em_SetMotion(em, 0x3E, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmChameleos_State4_03(Em *em, EmWork *work, u8 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x49, 0, 0);
        switch (variant) {
        case 1:
            StageObj_Stun_CreateEm(em, 1, 0x25, em->timer + em->motion[0].frameCount / 2.0f, 200.0f, 3.5f);
            break;
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            switch (variant) {
            default:
                em->timer = 120;
            case 1:
                break;
            }
            Em_SetMotion(em, 0x42, 0, 0x18);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x43, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            switch (variant) {
            case 0:
            default:
                EmChameleos_EndState(em, 0, 0);
                break;
            case 1:
                em->step++;
                EmChameleos_ChangeState(em, 0, 0x20, 4);
                break;
            }
        }
        break;
    case 4:
        if (em->remote == 0) {
            EmChameleos_ChangeState(em, 0, 0x20, 4);
        }
        break;
    }
}

void EmChameleos_State4_04(Em *em)
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
        Em_SetMotion(em, 0x4A, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x4B, 0, 0);
        }
        break;
    case 2:
        if (EmMotion_InFrameRange(em, 0, 0.0f, 104.0f) != 0) {
            turnF = 630.0f * em->motionSpeed;
            turn = (u32) turnF;
            em->rotY -= (s32) turn & 0xFFFF;
        }
        if (em->motion[0].playing == 0) {
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmChameleos_State4_06(void)
{
}

void EmChameleos_State4_07(void)
{
}

void EmChameleos_State4_08(Em *em, EmWork *work, s32 variant)
{
    s32 k;

    k = variant & 0xFF;
    if (k == 1) {
        em->blind = 2;
    }
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x40, 0, 0);
        if (!(em->velY <= -50.0f)) {
            em->velY = -50.0f;
        }
        em->accelX = 0.0f;
        em->accelY = -10.0f;
        em->accelZ = 0.0f;
        work->moveAngCX = 0;
        work->moveAngX = 0;
        break;
    case 1:
        work->moveAngCY = em->rotY;
        Em_MoveRotatedAccel(em, &work->moveAngCX);
        if (em->posY <= em->floorY) {
            em->step++;
            em->posY = em->floorY;
            em->stance = 0;
            Em_SetMotion(em, 0x41, 0, 0);
            Player_ClearVelocity(em);
            if (k == 2) {
                EmMine_CreateTimed0(em, &em->posX);
                break;
            }
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 0x3C;
            Em_SetMotion(em, 0x42, 0, 0);
        }
        break;
    case 3:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x43, 0, 0);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmChameleos_EndState(em, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    }
}

void EmChameleos_State4_0A(Em *em)
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
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmChameleos_State4_0B(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_STATUS);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x45, 0, 0);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmChameleos_ChangeState(em, 0, 0x21, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmChameleos_ChangeState(em, 0, 0x21, 4);
        }
        break;
    }
    Em_SpawnStatus4Effect(em, 2);
}

void EmChameleos_State6_00(Em *em, EmWork *work)
{
    f32 d;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        GmEm_ResetSize(em);
        em->posX = 9712.0f;
        em->posY = 0.0f;
        em->posZ = 13303.0f;
        em->goal[0] = 10091.0f;
        em->goal[1] = 0.0f;
        em->goal[2] = 7312.0f;
        em->rotX = 0;
        em->rotY = Math_AngleXZ(&em->posX, em->goal);
        em->rotZ = 0;
        EmChameleos_SetFlag43C(em);
        Em_SetMotion(em, 2, 0, 0);
        em->timer = 600;
        break;
    case 1:
        d = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer <= 0 || d <= 200.0f) {
            em->step++;
            em->goal[0] = 10384.0f;
            em->goal[1] = 0.0f;
            em->goal[2] = 5541.0f;
            em->rotX = 0;
            em->rotY = Math_AngleXZ(&em->posX, em->goal);
            em->rotZ = 0;
            Em_SetMotion(em, 1, 0, 0);
        }
        break;
    case 2:
        if (EmMotion_PassedFrameScaled(em, 50.0f, 0) != 0) {
            EmChameleos_ClearFlag43C(em);
            em->step++;
        }
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 600;
            Em_SetMotion(em, 2, 0, 0);
        }
        break;
    case 4:
        d = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer <= 0 || d <= 1300.0f) {
            em->step++;
            em->timer = 600;
            EmChameleos_SetFlag43C(em);
        }
        break;
    case 5:
        d = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer <= 0 || d <= 2000.0f) {
            em->step++;
            Em_SetMotion(em, 1, 0, 0);
        }
        break;
    case 6:
        if (Save_TestEventFlag(0xA2) == 1) {
            em->step++;
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
}

void EmChameleos_State6_01(Em *em, EmWork *work)
{
    f32 temp;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x49, 0, 0);
        ((EmChameleosWork *)work)->unk010 = 0;
        EmChameleos_ClearFlag43C(em);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 96.0f, 0) != 0) {
            em->step++;
            Em_SetMotion(em, 0x3E, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stance = 0;
            em->lookAtTarget = 0;
            Em_SetMotion(em, 0xA, 0, 0);
            EmChameleos_StartCurves(em, 0);
        }
        break;
    case 3:
        if (EmMotion_PassedFrameScaled(em, 58.0f, 0) != 0) {
            em->step++;
            em->stance = 2;
        }
        break;
    case 4:
        EmChameleos_UpdateCurves(em);
        if (EmMotion_PassedFrameScaled(em, 72.0f, 0) != 0) {
            em->step++;
        }
        break;
    case 5:
        em->posY += 30.0f;
        if (!(em->posY < em->floorY + 600.0f * em->scaleY)) {
            em->step++;
            Em_SetMotion(em, 0x13, 0, 0);
        }
    case 6:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x12, 0, 0);
            ((EmChameleosWork *)work)->unk010 = 0;
            Player_ClearVelocity(em);
            em->velY = 10.0f;
            Em_SetCombat(em, 0, 0);
        }
        break;
    case 7:
        Em_MoveRotated(em, &em->rotX);
        if (!(em->posY <= 500.0f + em->floorY)) {
            if (em->velY < 100.0f) {
                em->velY += 0.75f;
            }
        }
        if (!(em->posY < 50000.0f)) {
            em->step++;
            Player_ClearVelocity(em);
            em->velZ = 10.0f;
        }
        break;
    case 8:
        Em_MoveRotated(em, &em->rotX);
        break;
    }
    temp = em->floorY;
    if (em->posY < temp) {
        em->posY = temp;
    }
}

void EmChameleos_State6_02(Em *em, EmWork *work)
{
    f32 temp;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        Em_SetMotion(em, 0x12, 0, 0);
        ((EmChameleosWork *)work)->unk010 = 0;
        Player_ClearVelocity(em);
        em->velY = 10.0f;
        Em_SetCombat(em, 0, 0);
        EmChameleos_ClearFlag43C(em);
        break;
    case 1:
        Em_MoveRotated(em, &em->rotX);
        if (!(em->posY <= 500.0f + em->floorY)) {
            if (em->velY < 100.0f) {
                em->velY += 0.75f;
            }
        }
        if (!(em->posY < 50000.0f)) {
            em->step++;
            Player_ClearVelocity(em);
            em->velZ = 10.0f;
        }
        break;
    case 2:
        Em_MoveRotated(em, &em->rotX);
        break;
    }
    temp = em->floorY;
    if (em->posY < temp) {
        em->posY = temp;
    }
}

void EmChameleos_State5_00_Hunted(Em *em, EmWork *work)
{
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x44, 0, 0);
        EmChameleos_ClearFlag43C(em);
        em->lookAtTarget = 0;
        em->stance = 3;
        QuestEm_Hunted(em);
        break;
    case 1:
        EmMotion_PassedFrameScaled(em, 212.0f, 0);
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
                EmChameleos_ChangeState(em, 5, 3, 4);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    }
}

void EmChameleos_State5_01(void)
{
}

void EmChameleos_State5_02_HuntedFalling(Em *em, EmWork *work)
{
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x40, 0, 0);
        EmChameleos_ClearFlag43C(em);
        if (em->velY > -50.0f) {
            em->velY = -50.0f;
        }
        em->accelY = -10.0f;
        work->moveAngCX = 0;
        work->moveAngX = 0;
        break;
    case 1:
        work->moveAngCY = em->rotY;
        Em_MoveRotatedAccel(em, &work->moveAngCX);
        if (em->posY <= em->floorY) {
            em->step++;
            em->posY = em->floorY;
            Em_SetMotion(em, 0x41, 0, 0);
            Player_ClearVelocity(em);
            em->stance = 3;
            QuestEm_Hunted(em);
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
                EmChameleos_ChangeState(em, 5, 3, 4);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    case 0x63:
        if (em->motion[0].playing == 0) {
            em->stance = 0;
            EmChameleos_EndState(em, 0, 0);
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
        }
        break;
    }
}

void EmChameleos_State5_03(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        if (QuestEm_CanAppear(em) == 1) {
            em->rotY = 0x4861;
            em->posX = 3743.2f;
            em->posY = 1010.0f;
            em->posZ = 11415.0f;
            EmChameleos_ChangeState(em, 0, 1, 0);
            break;
        }
        em->taskPhase++;
    case 1:
        break;
    }
}

void EmChameleos_RunGroup0(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmChameleos_State0_00(em);
        break;
    case 1:
        EmChameleos_State0_01(em);
        break;
    case 2:
        EmChameleos_State0_02(em);
        break;
    case 3:
        EmChameleos_State0_03(em);
        break;
    case 4:
        EmChameleos_State0_04(em);
        break;
    case 5:
        EmChameleos_State0_05(em);
        break;
    case 9:
        EmChameleos_State0_09(em);
        break;
    case 10:
        EmChameleos_State0_0A(em, work, 0);
        break;
    case 11:
        EmChameleos_State0_0A(em, work, 1);
        break;
    case 18:
        EmChameleos_State0_12(em);
        break;
    case 20:
        EmChameleos_State0_14(em);
        break;
    case 23:
        EmChameleos_State0_17(em);
        break;
    case 24:
        EmChameleos_State0_18(em);
        break;
    case 25:
        EmChameleos_State0_19(em);
        break;
    case 27:
        EmChameleos_State0_1B(em);
        break;
    case 28:
        EmChameleos_State0_1C(em);
        break;
    case 32:
        EmChameleos_State0_20(em);
        break;
    case 33:
        EmChameleos_State0_20(em);
        break;
    }
}

void EmChameleos_RunGroup1(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmChameleos_State1_00(em);
        break;
    case 1:
        EmChameleos_State1_01(em, work);
        break;
    case 2:
        EmChameleos_State1_02(em, work, 0);
        break;
    case 3:
        break;
    case 4:
        EmChameleos_State1_04(em, (s32) work, 0);
        break;
    case 5:
        EmChameleos_State1_05(em);
        break;
    case 6:
        EmChameleos_State1_06(em, work);
        break;
    case 7:
        EmChameleos_State1_04(em, (s32) work, 1);
        break;
    case 8:
        EmChameleos_State1_02(em, work, 1);
        break;
    }
}

void EmChameleos_RunGroup2(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmChameleos_State2_00(em, work);
        break;
    case 1:
        EmChameleos_State2_01(em, work);
        break;
    case 2:
        EmChameleos_State2_02(em, work);
        break;
    case 3:
        EmChameleos_State2_03();
        break;
    case 4:
        EmChameleos_State2_04();
        break;
    case 5:
        EmChameleos_State2_05();
        break;
    case 6:
        EmChameleos_State2_06(em, work);
        break;
    case 7:
        EmChameleos_State2_07(em, work);
        break;
    case 8:
        EmChameleos_State2_08(em, work);
        break;
    case 9:
        EmChameleos_State2_09(em, work);
        break;
    case 10:
        EmChameleos_State2_0A();
        break;
    case 11:
        EmChameleos_State2_0B(em, work);
        break;
    case 12:
        EmChameleos_State2_0C();
        break;
    case 13:
        EmChameleos_State2_0D(em, work);
        break;
    case 14:
        EmChameleos_State2_0E(em, work);
        break;
    case 15:
        EmChameleos_State2_0F(em, work);
        break;
    case 16:
        EmChameleos_State2_10(em, work);
        break;
    case 17:
        EmChameleos_State2_11(em, work);
        break;
    case 18:
        EmChameleos_State2_12(em, work);
        break;
    case 19:
        EmChameleos_State2_13(em, work);
        break;
    case 20:
        EmChameleos_State2_14();
        break;
    case 21:
        EmChameleos_State2_15(em, work);
        break;
    case 22:
        EmChameleos_State2_16(em, work);
        break;
    case 23:
        EmChameleos_State2_17();
        break;
    case 24:
        EmChameleos_State2_18(em, work);
        
    default:
        break;
    }
}

void EmChameleos_RunGroup3(Em *em, s32 work)
{
    switch (em->state) {
    case 0:
        EmChameleos_State3_00(em, work, 0);
        break;
    case 1:
        EmChameleos_State3_01(em);
        break;
    case 2:
        EmChameleos_State3_02(em);
        break;
    case 3:
        EmChameleos_State3_03(em);
        break;
    case 4:
        EmChameleos_State3_04(em);
        break;
    case 5:
        EmChameleos_State3_05(em);
        break;
    case 6:
        EmChameleos_State3_06(em, work, 0);
        break;
    case 7:
        EmChameleos_State3_07(em, work, 0);
        break;
    case 8:
        EmChameleos_State3_08(em, (EmWork *) work);
        break;
    case 9:
        EmChameleos_State3_06(em, work, 1);
        break;
    case 10:
        EmChameleos_State3_07(em, work, 1);
        break;
    case 11:
        EmChameleos_State3_00(em, work, 1);
        break;
    case 12:
        EmChameleos_State3_0C(em);
        
    default:
        break;
    }
}

void EmChameleos_RunGroup4(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmChameleos_State4_00(em);
        break;
    case 1:
        EmChameleos_State4_01(em);
        break;
    case 2:
        EmChameleos_State4_02(em, (s32) work, 0);
        break;
    case 3:
        EmChameleos_State4_03(em, work, 0);
        break;
    case 4:
        EmChameleos_State4_04(em);
        break;
    case 5:
        EmChameleos_State4_03(em, work, 1);
        break;
    case 6:
        EmChameleos_State4_06();
        break;
    case 7:
        EmChameleos_State4_07();
        break;
    case 8:
        EmChameleos_State4_08(em, work, 0);
        break;
    case 9:
        EmChameleos_State4_08(em, work, 1);
        break;
    case 10:
        EmChameleos_State4_0A(em);
        break;
    case 11:
        EmChameleos_State4_0B(em);
        break;
    case 12:
        EmChameleos_State4_02(em, (s32) work, 1);
        break;
    case 13:
        EmChameleos_State4_08(em, work, 2);
        break;
    }
}

void EmChameleos_RunGroup5(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmChameleos_State5_00_Hunted(em, work);
        break;
    case 1:
        EmChameleos_State5_01();
        break;
    case 2:
        EmChameleos_State5_02_HuntedFalling(em, work);
        break;
    case 3:
        EmChameleos_State5_03(em, work);
        break;
    }
}

void EmChameleos_RunGroup6(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmChameleos_State6_00(em, work);
        break;
    case 1:
        EmChameleos_State6_01(em, work);
        break;
    case 2:
        EmChameleos_State6_02(em, work);
        break;
    }
}

void EmChameleos_Update(Em *em)
{
    u8 buf[4];
    EmChameleosWork *w = (EmChameleosWork *)&em->work;
    u32 r;

    Em_UpdateCombatTimer(em);
    Em_RaisePlayerAreaTriggerLax(em);
    if (em->noAlert != 1 && em->rage == 0) {
        switch ((u16) Map_DayId(gScene.map)) {
        case MAP_FOREST_HILLS:
            Em_CheckLowHp(em, 0.15f);
            Em_DecayNeed2(em);
            break;
        case MAP_SWAMP:
            Em_CheckLowHp(em, 0.15f);
            break;
        case MAP_JUNGLE:
            Em_CheckLowHp(em, 0.15f);
            Em_DecayNeed2(em);
            break;
        }
    }
    r = Em_UpdateDamage(em, buf) & 0xFF;
    switch (r) {
    case 0:
    case 3:
    case 4:
    case 7:
    case 9:
    case 13:
    case 14:
    case 18:
        break;
    case 1:
    case 2:
        if (em->stance == 2) {
            EmChameleos_ChangeState(em, 5, 2, 3);
        } else {
            EmChameleos_ChangeState(em, 5, 0, 3);
        }
        break;
    case 5:
        if (em->stateGroup != 4 || em->state != 8) {
            EmChameleos_ChangeState(em, 4, 8, 3);
        }
        break;
    case 6:
        if ((em->stateGroup != 4 || em->state != 0xB) && (em->stateGroup != 4 || em->state != 8)) {
            Em_SetStatus4Timer(em);
            EmChameleos_ChangeState(em, 4, 0xB, 3);
        }
        break;
    case 8:
        if ((em->stateGroup != 0 || em->state != 0x14) && (em->stateGroup != 4 || em->state != 8)) {
            Em_SetStatus2Timer(em);
            EmChameleos_ChangeState(em, 0, 0x14, 3);
        }
        break;
    case 10:
        switch (em->state) {
        case 0x12:
            EmChameleos_ChangeState(em, 0, 0x17, 3);
            em->scriptPending = 0;
            break;
        case 0x14:
            EmChameleos_ChangeState(em, 0, 0x18, 3);
            em->scriptPending = 0;
            break;
        }
        break;
    case 11:
        EmChameleos_ChangeState(em, 4, 4, 3);
        break;
    case 15:
        if (EmChameleos_HasFlag43C(em) == 1 && (EmChameleos_WantsInvisible(em) & 0xFF) == 0 && (EmChameleos_CanTurnInvisible(em) & 0xFF) == 2) {
            if (em->stance == 2) {
                EmChameleos_ChangeState(em, 4, 0xD, 3);
            } else {
                EmChameleos_ChangeState(em, 4, 0xC, 3);
            }
        }
        break;
    case 16:
        if (EmChameleos_HasFlag43C(em) == 1 && EmChameleos_IsInvisible(em) == 0) {
            if (em->stance == 2) {
                EmChameleos_ChangeState(em, 4, 0xD, 3);
            } else {
                EmChameleos_ChangeState(em, 4, 0xC, 3);
            }
            break;
        }
    case 12:
        if (em->stance == 2) {
            EmChameleos_ChangeState(em, 4, 8, 3);
        } else {
            EmChameleos_BreakParts(em, 1);
            switch (em->flinchPart) {
            case 1:
            case 2:
            case 5:
                if (em->parts[em->flinchPart].state >= 2) {
                    EmChameleos_ChangeState(em, 4, 2, 3);
                } else {
                    EmChameleos_ChangeState(em, 4, 1, 3);
                }
                break;
            case 0:
            case 6:
                if (em->parts[em->flinchPart].state >= 2) {
                    EmChameleos_ChangeState(em, 4, 2, 3);
                } else {
                    EmChameleos_ChangeState(em, 4, 0, 3);
                }
                break;
            case 3:
            case 4:
            default:
                if (em->parts[em->flinchPart].state >= 2) {
                    EmChameleos_ChangeState(em, 4, 3, 3);
                } else {
                    EmChameleos_ChangeState(em, 4, 1, 3);
                }
                break;
            }
        }
        break;
    case 17:
        if ((em->stateGroup != 4 || em->state != 8) && (em->stateGroup != 4 || em->state != 5) && (em->stateGroup != 4 || em->state != 5)) {
            Em_SetStatus5Timer(em);
            EmChameleos_ChangeState(em, 4, 5, 3);
        }
        break;
    }
    switch (gQuest.stage) {
    case 0x437:
    case 0x438:
        if (Area_DayId(em->area) == AREA_FOREST_HILLS_9 && Save_TestEventFlag(0xA2) == 0) {
            if (Area_DayId(gScene.area) == AREA_FOREST_HILLS_9 && gScene.eventRunning == 1 && em->stateGroup != 6) {
                EmChameleos_ChangeState(em, 6, 0, 1);
            }
            break;
        }
    default:
        if (Em_DamageResultIsReaction(em, r) == 1) {
            EmChameleos_BreakParts(em, 0);
        }
        if (Quest_IsRunning(0) == 1 && em->stateGroup != 6 && em->stateGroup != 5) {
            em->scriptPending = 0;
            if (em->stance == 2) {
                EmChameleos_ChangeState(em, 6, 2, 0);
            } else {
                if (gScene.area == em->area) {
                    CamReq_ForMonster(em);
                }
                EmChameleos_ChangeState(em, 6, 1, 0);
            }
        }
        Em_UpdatePendingScript(em);
        break;
    }
    Em_ClearHitReactMode(em);
    EmChameleos_RunState(em, (s32) w);
    if (em->stateRequest != 0) {
        EmChameleos_RunState(em, (s32) w);
        em->stateRequest = 0;
    }
    if (em->stateGroup == 2 && (s8) w->unk010 == 1) {
        em->guardTimer = 2;
    }
    EmChameleos_UpdateInvisibility(em);
    if (EmChameleos_IsInvisible(em) == 1) {
        em->hitzoneSet = 1;
    } else {
        em->hitzoneSet = 0;
    }
}

void EmChameleos_RunState(Em *em, s32 work)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->stateGroup) {
    case 0:
        EmChameleos_RunGroup0(em, (EmWork *) work);
        break;
    case 1:
        EmChameleos_RunGroup1(em, (EmWork *) work);
        break;
    case 2:
        EmChameleos_RunGroup2(em, (EmWork *) work);
        break;
    case 3:
        EmChameleos_RunGroup3(em, work);
        break;
    case 4:
        EmChameleos_RunGroup4(em, (EmWork *) work);
        break;
    case 5:
        EmChameleos_RunGroup5(em, (EmWork *) work);
        break;
    case 6:
        EmChameleos_RunGroup6(em, (EmWork *) work);
        break;
    case 7:
        EmChameleos_RunGroup6(em, (EmWork *) work);
        
    default:
        break;
    }
}

void EmChameleos_BreakParts(Em *em, s32 apply)
{
    u8 i = em->flinchPart;

    switch (i) {
    case 0:
        if ((s32) em->parts[i].state > 0 && (*(u8 *) (i + (u32) em + 0x6D4) & 0x20)) {
            Em_FlagPartBreak(em, 0);
        }
        break;
    }
    if (apply != 0) {
        Em_ApplyPartBreaks(em);
    }
}

void EmChameleos_SpawnGroundEffect(Em *em, s32 a1, s32 a2, f32 scale)
{
    f32 pos[3];

    if (Player_InCurrentArea(em)) {
        Em_BonePos(em, a1, pos);
        pos[1] = em->floorY;
        Effect0D_SpawnOnGround(pos, a2, scale * em->scaleX);
    }
}

void EmChameleos_SpawnGroundEffectOffset(Em *em, s32 bone, s32 kind, f32 *ofs, f32 scale)
{
    f32 pos[3];
    f32 v[3];

    if (Player_InCurrentArea(em)) {
        Em_BonePos(em, bone, pos);
        Vec_Copy(v, ofs);
        Vec_RotateY(v, 6.2831855f * em->rotY / 65536.0f);
        pos[0] += v[0] * em->scaleX;
        pos[1] = em->floorY + v[1] * em->scaleY;
        pos[2] += v[2] * em->scaleZ;
        Effect0D_SpawnOnGround(pos, kind, scale * em->scaleX);
    }
}

void EmChameleos_SpawnGroundEffectNearGround(Em *em, s32 arg1, s32 arg2, s32 flag, f32 scale)
{
    f32 pos[3];

    if (em->posY <= 1000.0f + em->floorY) {
        EmChameleos_SpawnGroundEffect(em, arg1, arg2, scale);
        if (flag & 0xFF) {
            pos[0] = em->posX;
            pos[1] = em->floorY;
            pos[2] = em->posZ;
            EmImpactAttack_CreateAt(em, (f32 *) pos, 3);
        }
    }
}

void EmChameleos_SpawnK14NearGround(Em *em, s32 k, f32 scale, s32 c, s32 flag)
{
    f32 pos[3];

    if (em->posY <= 1000.0f + em->floorY) {
        StageObj_K14_Create(em, k, c, scale * em->scaleX);
        if (flag & 0xFF) {
            pos[0] = em->posX;
            pos[1] = em->floorY;
            pos[2] = em->posZ;
            EmImpactAttack_CreateAt(em, (f32 *) pos, 3);
        }
    }
}

void EmChameleos_SpawnK1CInMotions(Em *em, s16 motion)
{
    switch (motion) {
    case 0x3E9:
    case 0x3EA:
    case 0x3ED:
    case 0x3EE:
    case 0x3EF:
    case 0x3FC:
    case 0x407:
    case 0x408:
    case 0x409:
    case 0x40A:
    case 0x40B:
    case 0x414:
    case 0x418:
    case 0x41D:
    case 0x42E:
    case 0x433:
        if (em->rage != 0 && gScene.frame % 45 == 0) {
            StageObj_K1C_CreateOnEm(em, 0xD, 2.0f * em->scaleX);
        }
        break;
    }
}

static void EmChameleos_MotionEffectsDefault(Em *em)
{
}

void EmChameleos_MotionEffects(Em *em, EmChameleosWork *work)
{
    f32 pos[3];
    f32 ofs[3];

    if (em->motionId[0] != work->fxMotion) {
        work->fxMotion = em->motionId[0];
    }
    EmChameleos_MotionSounds(em, (EmWork *) work);
    EmChameleos_SpawnK1CInMotions(em, work->fxMotion);
    switch (work->fxMotion) {
    case 0x3EB:
        if (EmMotion_PassedFrameScaled(em, 52.0f, 0)) {
            EmChameleos_SpawnGroundEffect(em, 8, 0x14, 4.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 70.0f, 0)) {
            EmChameleos_SpawnGroundEffect(em, 0xD, 0x14, 4.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 66.0f, 0)) {
            EmChameleos_SpawnGroundEffect(em, 0x1A, 0x14, 4.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 48.0f, 0)) {
            EmChameleos_SpawnGroundEffect(em, 0x1F, 0x14, 4.0f);
        }
        break;
    case 0x3EC:
        if (EmMotion_PassedFrameScaled(em, 24.0f, 0)) {
            EmChameleos_SpawnGroundEffect(em, 1, 7, 8.0f);
        }
        break;
    case 0x3EF:
        if (EmMotion_PassedFrameScaled(em, 110.0f, 0)) {
            EmChameleos_SpawnGroundEffect(em, 1, 7, 8.0f);
        }
        break;
    case 0x3F1:
        if (EmMotion_PassedFrameScaled(em, 50.0f, 0)) {
            StageObj_K14_Create(em, 2, 0x29, 1.5f * em->scaleX);
        }
        if (EmMotion_PassedFrameScaled(em, 86.0f, 0)) {
            EmChameleos_SpawnGroundEffect(em, 1, 7, 8.0f);
        }
        break;
    case 0x3F2:
        if (EmMotion_PassedFrameScaled(em, 60.0f, 0)) {
            StageObj_K14_Create(em, 0x13, 0, em->scaleX);
        }
        break;
    case 0x3F3:
        if (EmMotion_PassedFrameScaled(em, 64.0f, 0)) {
            EmChameleos_SpawnGroundEffectNearGround(em, 1, 7, 0, 8.0f);
        }
        break;
    case 0x3F8:
        if (EmMotion_PassedFrameScaled(em, 22.0f, 0)) {
            EmChameleos_SpawnK14NearGround(em, 1, 1.0f, 0, 0);
        }
        if (EmMotion_PassedFrameScaled(em, 92.0f, 0)) {
            EmChameleos_SpawnGroundEffectNearGround(em, 1, 7, 0, 8.0f);
        }
        break;
    case 0x3F9:
        if (EmMotion_PassedFrameScaled(em, 32.0f, 0)) {
            EmChameleos_SpawnK14NearGround(em, 0xB, 1.0f, 0, 1);
        }
        break;
    case 0x3FA:
        if (EmMotion_PassedFrameScaled(em, 36.0f, 0)) {
            EmChameleos_SpawnK14NearGround(em, 1, 1.0f, 0, 1);
        }
        break;
    case 0x3FB:
        if (EmMotion_PassedFrameScaled(em, 40.0f, 0)) {
            EmChameleos_SpawnGroundEffectNearGround(em, 1, 7, 0, 8.0f);
        }
        break;
    case 0x411:
        if (EmMotion_PassedFrameScaled(em, 66.0f, 0)) {
            ChameleosAttack_Create(em, 2);
        }
        if (EmMotion_PassedFrameScaled(em, 68.0f, 0)) {
            ChameleosAttack_Create(em, 8);
        }
        if (EmMotion_PassedFrameScaled(em, 74.0f, 0) || EmMotion_PassedFrameScaled(em, 102.0f, 0)) {
            StageObj_K14_Create(em, 2, 0x2B, 1.5f * em->scaleX);
        }
        break;
    case 0x412:
        if (EmMotion_PassedFrameScaled(em, 86.0f, 0)) {
            ChameleosAttack_Create(em, 9);
        }
        break;
    case 0x415:
        if (EmMotion_PassedFrameScaled(em, 42.0f, 0)) {
            ChameleosAttack_Create(em, 3);
        }
        if (EmMotion_PassedFrameScaled(em, 44.0f, 0) || EmMotion_PassedFrameScaled(em, 48.0f, 0) || EmMotion_PassedFrameScaled(em, 52.0f, 0)) {
            EmChameleos_SpawnGroundEffect(em, 0x1F, 0x14, 8.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 50.0f, 0)) {
            StageObj_K14_Create(em, 2, 0x2F, 1.5f * em->scaleX);
        }
        break;
    case 0x416:
        if (EmMotion_PassedFrameScaled(em, 72.0f, 0)) {
            ChameleosAttack_Create(em, 4);
        }
        if (EmMotion_PassedFrameScaled(em, 84.0f, 0)) {
            EmChameleos_SpawnGroundEffect(em, 0x10, 0xC, 8.0f);
            Em_BonePos(em, 0x10, pos);
            pos[1] = em->floorY;
            if ((u8)(s32)Player_InCurrentArea(em)) {
                StageObj_K11_CreateAtScaled(pos, *(u16 *) &em->rotY, 0xA, 1.0f);
            }
            EmImpactAttack_CreateAt(em, (f32 *) pos, 3);
        }
        break;
    case 0x418:
        if (EmMotion_PassedFrameScaled(em, 66.0f, 0) || EmMotion_PassedFrameScaled(em, 162.0f, 0) || EmMotion_PassedFrameScaled(em, 258.0f, 0) || EmMotion_PassedFrameScaled(em, 356.0f, 0)) {
            EmChameleos_SpawnGroundEffect(em, 0x10, 7, 8.5f);
            Em_BonePos(em, 0x10, pos);
            pos[1] = em->floorY;
            if ((u8)(s32)Player_InCurrentArea(em)) {
                StageObj_K11_CreateAtScaled(pos, *(u16 *) &em->rotY, 0xA, 1.0f);
            }
            EmImpactAttack_CreateAt(em, (f32 *) pos, 3);
        }
        if (EmMotion_PassedFrameScaled(em, 64.0f, 0) || EmMotion_PassedFrameScaled(em, 160.0f, 0) || EmMotion_PassedFrameScaled(em, 256.0f, 0) || EmMotion_PassedFrameScaled(em, 354.0f, 0)) {
            ChameleosAttack_Create(em, 7);
        }
        break;
    case 0x419:
        if (EmMotion_PassedFrameScaled(em, 28.0f, 0)) {
            ChameleosAttack_Create(em, 1);
        }
        if (EmMotion_PassedFrameScaled(em, 38.0f, 0)) {
            ofs[0] = 0.0f;
            ofs[1] = 0.0f;
            ofs[2] = 200.0f;
            EmChameleos_SpawnGroundEffectOffset(em, 0x29, 0x14, ofs, 8.0f);
        }
        break;
    case 0x41A:
        if (EmMotion_PassedFrameScaled(em, 22.0f, 0)) {
            ChameleosAttack_Create(em, 5);
        }
        if (EmMotion_PassedFrameScaled(em, 28.0f, 0)) {
            ChameleosAttack_Create(em, 1);
        }
        if (EmMotion_PassedFrameScaled(em, 28.0f, 0)) {
            EmChameleos_SpawnGroundEffect(em, 0x1F, 0x14, 4.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 38.0f, 0)) {
            ofs[0] = 0.0f;
            ofs[1] = 0.0f;
            ofs[2] = 200.0f;
            EmChameleos_SpawnGroundEffectOffset(em, 0x29, 0x14, ofs, 8.0f);
        }
        break;
    case 0x41B:
        if (EmMotion_PassedFrameScaled(em, 26.0f, 0)) {
            ChameleosAttack_Create(em, 1);
        }
        if (EmMotion_PassedFrameScaled(em, 30.0f, 0)) {
            ofs[0] = 0.0f;
            ofs[1] = 0.0f;
            ofs[2] = 350.0f;
            EmChameleos_SpawnGroundEffectOffset(em, 0x29, 0x14, ofs, 4.0f);
        }
        break;
    case 0x426:
        if (EmMotion_PassedFrameScaled(em, 64.0f, 0)) {
            EmChameleos_SpawnGroundEffect(em, 1, 7, 4.0f);
        }
        break;
    case 0x429:
        if (EmMotion_PassedFrameScaled(em, 12.0f, 0)) {
            StageObj_K14_Create(em, 0xB, 0, em->scaleX);
        }
        break;
    case 0x42A:
        if (EmMotion_PassedFrameScaled(em, 62.0f, 0)) {
            EmChameleos_SpawnGroundEffect(em, 1, 7, 8.0f);
        }
        break;
    case 0x42C:
        if (EmMotion_PassedFrameScaled(em, 468.0f, 0)) {
            StageObj_K14_Create(em, 0, 0, em->scaleX);
        }
        break;
    case 0x42F:
        if (EmMotion_PassedFrameScaled(em, 182.0f, 0)) {
            StageObj_K14_Create(em, 0, 0, em->scaleX);
        }
        break;
    case 0x431:
        if (EmMotion_PassedFrameScaled(em, 166.0f, 0)) {
            StageObj_K14_Create(em, 0, 0, em->scaleX);
        }
        break;
    case 0x432:
        if (EmMotion_PassedFrameScaled(em, 54.0f, 0)) {
            StageObj_K14_Create(em, 0, 0, em->scaleX);
        }
        break;
    default:
        EmChameleos_MotionEffectsDefault(em);
        break;
    }
}

void EmChameleos_UpdateMotionEffects(Em *em)
{
    EmChameleosWork *w = (EmChameleosWork *)&em->work;

    switch (w->fxStep) {
    case 0:
        w->fxStep++;
        break;
    case 1:
        EmChameleos_MotionEffects(em, (EmChameleosWork *) w);
        break;
    }
}

void EmChameleos_TableNop(void)
{
}

s32 EmChameleos_ScriptQuery(Em *em, u8 k)
{
    switch (k) {
    case 0:
        return EmChameleos_HasFlag43C(em) == 1;
    case 1:
        return (u8) Map_DayId(gScene.map);
    case 2:
        return EmChameleos_WantsInvisible(em);
    }
    return 0;
}

s32 EmChameleos_AllowHit8(Em *em)
{
    if (em->stance != 2 && em->hp <= (s16) (0.3f * em->hpMax) && em->partBreakCount == 0) {
        return 1;
    }
    return 0;
}

/* Remobra's own meanings of the work area's bytes (EmWork, include/em.h, has the shared ones). */
typedef struct EmRemobraWork {
    u8 fxStep;                  /* 0x000: UpdateMotionEffects: 0, then 1 */
    u8 unk001[0x014 - 0x001];
    f32 home[3];                /* 0x014: Init: the position; ChangeState copies it to the goal */
    u8 curveNone;               /* 0x020: the three curves' run flags (StartCurves, UpdateCurves) */
    u8 curveVelY;               /* 0x021 */
    u8 curveSpeed;              /* 0x022 */
    u8 curveKind;               /* 0x023: the curve tables' index (StartCurves) */
    s16 curveFrame;             /* 0x024: the curve frame (StartCurves, UpdateCurves) */
    u8 unk026[0x028 - 0x026];
    f32 distLeft;               /* 0x028: ChangeState: the distance to the goal (Vec_DistanceXZ, or 1000) */
    u16 unk02C;                 /* 0x02C: ChangeState: an angle (Math_AngleXZ to the goal, or rotY plus a random part) */
    u8 unk02E[0x02F - 0x02E];
    u8 unk02F;                  /* 0x02F: ChangeState: 0 or 1 */
    f32 hopVel;                 /* 0x030: UpdateHop: 14, then less hopAccel each frame */
    f32 hopAccel;               /* 0x034: UpdateHop: -hopVel / (hopFrames / 2) */
    u8 unk038[0x03A - 0x038];
    s16 hopFrames;              /* 0x03A: UpdateHop: 0x22 */
    u8 unk03C[0x100 - 0x03C];
} EmRemobraWork;
typedef char EmRemobraWork_size_check[(sizeof(EmRemobraWork) == sizeof(EmWork)) ? 1 : -1];

void EmRemobra_MotionSounds(s32 em, EmWork *work)
{
    s16 motion;

    motion = work->fxMotion;
    switch (motion) {
    case 0x3E9:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x40, Sound_RandomSe(0xB, 2, 0xB, 2), 0x15);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x48, 0x1C, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x4C, 0x1D, 0xE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x94, 0x1C, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x98, 0x1D, 0xE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xDE, 0x1C, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xE0, 0x1D, 0xE);
        break;
    case 0x3EA:
        Em_PlaySeAtBoneOnFrame((Em *) em, 2, 0, 0x15);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x76, 1, 0x15);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x50, 0x1A, 3);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x68, 0x20, 0xE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x98, 0x20, 0xE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x74, 2, 0x15);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xF4, 0x1B, 0x15);
        break;
    case 0x3EB:
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 0x1C, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA, 0x1D, 0xE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 0xB, 0x15);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x26, 0x1C, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2A, 0x1D, 0xE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x58, 0x1C, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x5C, 0x1D, 0xE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x80, 0x1C, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x84, 0x1D, 0xE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xDE, 0x1C, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xE0, 0x1D, 0xE);
        break;
    case 0x3EC:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0, 0x14, 3);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x15, 0x14, 3);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2B, 0x14, 3);
        break;
    case 0x3ED:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0, 0x14, 3);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x10, 0x1C, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x14, 0x1D, 0xE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x15, 0x14, 3);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2B, 0x14, 3);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x40, 0x14, 3);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x4E, 0x1C, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x52, 0x1D, 0xE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x55, 0x14, 3);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x6A, 0x14, 3);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x7F, 0x14, 3);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x8A, 0x14, 3);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x9F, 0x14, 3);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xB4, 0x14, 3);
        break;
    case 0x3EE:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0, 0x14, 3);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x15, 0x14, 3);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2B, 0x14, 3);
        break;
    case 0x3EF:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0, 0x14, 3);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x15, 0x14, 3);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2B, 0x14, 3);
        break;
    case 0x3F0:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0, 0x14, 3);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x15, 0x14, 3);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2B, 0x14, 3);
        break;
    case 0x3F1:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0, 0x14, 3);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x15, 0x14, 3);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2B, 0x14, 3);
        break;
    case 0x3F2:
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 3, 0x15);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2C, 0x1C, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x30, 0x1D, 0xE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x6C, 0x1E, 8);
        break;
    case 0x3F3:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x12, 4, 0x15);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1C, 0x22, 0x1D);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x30, 0x19, 0x19);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x44, 0x22, 0x19);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x80, 0x1C, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x84, 0x1D, 0xE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xBC, 0x1C, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xC0, 0x1D, 0xE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xF2, 0x1C, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xF6, 0x1D, 0xE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x122, 0x1C, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x126, 0x1D, 0xE);
        break;
    case 0x3F4:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x14, 5, 0x15);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0, 0x14, 3);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x15, 0x14, 3);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2B, 0x14, 3);
        break;
    case 0x3F5:
        Em_PlaySeAtBoneOnFrame((Em *) em, 2, 5, 0x15);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x1C, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 0x1D, 0xE);
        break;
    case 0x3F6:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x14, 6, 0x15);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x48, 0x1C, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x4C, 0x1D, 0xE);
        break;
    case 0x3F7:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 5, 0x15);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 0x1C, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xC, 0x1D, 0xE);
        break;
    case 0x3F8:
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 7, 0x15);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xE, 0x1E, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x12, 0x1E, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x22, 8, 0x15);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x26, 0x1C, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x30, 0x15, 0x20);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2A, 0x1D, 0xE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x48, 0x1C, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x4C, 0x1D, 0xE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x68, 0x1C, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x6C, 0x1D, 0xE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x90, 0x1C, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x94, 0x1D, 0xE);
        break;
    case 0x3F9:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x18, 9, 0x15);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3A, 0x1C, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x42, 0x1D, 0xE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x6E, 0x26, 3);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x78, 0x1C, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x7C, 0x1D, 0xE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x8C, 4, 0x15);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x9A, 0x29, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x9E, 0x2A, 0xE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x9C, 0x2B, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA0, 0x2C, 0xE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xCC, 0x1C, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xD2, 0x1D, 0xE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xFA, 0x1C, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xFE, 0x1D, 0xE);
        break;
    case 0x3FA:
        Em_PlaySeAtBoneOnFrame((Em *) em, 2, 0x16, 3);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xC, 2, 0x15);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x10, 0x1C, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x14, 0x1D, 0xE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2C, 0x1C, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x30, 0x1D, 0xE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x44, 0x1C, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x48, 0x1D, 0xE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x64, 0x1C, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x68, 0x1D, 0xE);
        break;
    case 0x3FB:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0, 0x16, 3);
        Em_PlaySeAtBoneOnFrame((Em *) em, 2, 3, 0x15);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x12, 0x20, 3);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x32, 0x1C, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x36, 0x1D, 0xE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x40, 0x14, 3);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x54, 0x14, 3);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x62, 0x14, 3);
        break;
    case 0x3FC:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0, 0x15);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xE, 0x1A, 3);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x22, 0x21, 0xE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x42, 0x20, 0xE);
        break;
    case 0x3FD:
        Em_PlaySeAtBoneOnFrame((Em *) em, 2, 0x17, 3);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x30, Sound_RandomSe(0xA, 5, 2, 3), 0x15);
        break;
    case 0x3FE:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0xA, 0x15);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x6C, Sound_RandomSe(0xB, 2, 0xB, 2), 0x15);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xDE, 0xA, 0x15);
        break;
    case 0x3FF:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0, 0x15);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x74, 0xE, 0x15);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x74, 0xF, 0x15);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x9E, 0x1A, 3);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xB2, 0x1B, 0x15);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xF4, 0xD, 0x15);
        break;
    case 0x400:
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 3, 0x15);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1E, 4, 0x15);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 0x22, 0x1D);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2C, 0x19, 0x19);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x40, 0x22, 0x1D);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x4C, 4, 0x15);
        break;
    case 0x401:
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 0x20, 0xE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA, 1, 0x15);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x30, 0x1F, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x5A, 2, 0x15);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x90, 0x1B, 0x15);
        break;
    case 0x402:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xE, 0xC, 0x15);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2C, 0x1C, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x34, 0x1D, 0xE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x62, 0x1C, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x6A, 0x1D, 0xE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x9E, 0x1C, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA6, 0x1D, 0xE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xE0, 0x1C, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xE6, 0x1D, 0xE);
        
    default:
        break;
    }
}

void EmRemobra_Init(Em *em)
{
    EmRemobraWork *w = (EmRemobraWork *)&em->work;

    s16 size;

    Em_SetMotionRow(em, 1, 0, 0);
    em->stance = 2;
    size = Em_ComputeMaxHp(em, 0);
    em->hp = size;
    em->hpMax = size;
    em->scriptPending = 1;
    em->blind = 0;
    Em_InitParts(em);
    w->home[0] = em->posX;
    w->home[1] = 0.0f;
    w->home[2] = (f32) em->posZ;
}

void EmRemobra_Start(Em *em)
{
    EmRemobra_Init(em);
    EmRemobra_ChangeState(em, 2, 9, 0);
}

void EmRemobra_Reinit(Em *em)
{
    EmRemobra_Init(em);
}

void EmRemobra_ChangeState(Em *em, u16 group, u16 state, u8 mode)
{
    EmRemobraWork *w = (EmRemobraWork *)&em->work;
    f32 d;
    f32 f;

    switch (group) {
    case 2:
        switch (state) {
        case 10:
            w->unk02C = Math_AngleXZ(&em->posX, em->goal);
            break;
        case 18:
            w->unk02C = em->rotY + (em->stateRand & 0x3FFF);
            break;
        case 2:
        case 20:
            d = 1300.0f;
            goto dist;
        case 15:
            d = 300.0f;
        dist:
            if (em->curGoalKind == 0) {
                w->unk02F = 0;
                em->timer = (em->stateRand % 120 + 180) / 2;
                w->distLeft = 1000.0f;
            } else {
                w->unk02F = 1;
                w->distLeft = Vec_DistanceXZ(&em->posX, em->goal);
                if (em->curGoalKind == 1 && em->curGoalArg == 0) {
                    if (em->targetNo == -1) {
                        w->unk02F = 0;
                        w->distLeft = 1000.0f;
                        break;
                    }
                    f = w->distLeft - d;
                    w->distLeft = f;
                    if (f <= 0.0f) {
                        w->distLeft = 0.0f;
                    }
                }
                if (em->curGoalKind == 7) {
                    f = w->distLeft - d;
                    w->distLeft = f;
                    if (f <= 0.0f) {
                        w->distLeft = 0.0f;
                    }
                }
                em->timer = (s32) w->distLeft / 7;
            }
            break;
        case 19:
            w->unk02F = 0;
            f = 600.0f;
            w->distLeft = f;
            em->timer = (s32) f / 7;
            break;
        case 12:
            em->timer = (em->stateRand & 3) * 30 + 30;
            break;
        case 13:
            em->timer = 120;
            if (em->curGoalKind == 0) {
                em->goal[1] = em->posY;
                em->goal[1] += (f32) ((u16) Rand_Next(1) % 200) - 50.0f;
            }
            break;
        case 14:
            Em_EndStatus4(em);
            break;
        case 17:
            Vec_Copy(em->goal, &w->home[0]);
            em->goal[1] += 20.0f;
            w->distLeft = Vec_Distance((f32 *) &em->posX, (f32 *) em->goal);
            break;
        case 26:
            em->goal[1] = 500.0f + em->floorY;
            break;
        }
        break;
    case 5:
        if (state == 1 && QuestEm_CanAppear(em) == 1) {
            QuestEm_Appear(em);
        }
        break;
    case 4:
        switch (state) {
        case 3:
            Em_BeginStatus4(em);
            break;
        }
        break;
    }
    Em_SetStateSync(em, group, state, mode);
}

void EmRemobra_EndState(Em *em)
{
    em->motionSpeed = 1.0f;
    em->stance = 0;
    em->lookAtTarget = 0;
    em->scriptPending = 1;
    em->blind = 0;
    EmRemobra_ChangeState(em, 0, 1, 0);
}

void EmRemobra_EndState2(Em *em)
{
    em->motionSpeed = 1.0f;
    em->stance = 2;
    em->lookAtTarget = 0;
    em->scriptPending = 1;
    em->blind = 0;
    EmRemobra_ChangeState(em, 2, 0, 0);
}

void EmRemobra_State0_00(void)
{
}

void EmRemobra_State0_01(Em *em)
{
    switch (em->step) {
    case 0:
        em->stance = 0;
        em->step++;
        em->timer = ((em->stateRand & 3) * 0x1E) + 0x1E;
        if (em->motionId[0] != 0x403) {
            Em_SetMotion(em, 0x1B, 0, 0);
        }
        break;
    case 1:
        if (--em->timer <= 0) {
            EmRemobra_EndState(em);
        }
        break;
    }
}

void EmRemobra_RunGroup0(Em *em)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->state) {
    case 0:
        EmRemobra_State0_00();
        break;
    case 1:
        EmRemobra_State0_01(em);
        break;
    }
}

void EmRemobra_State1_00(Em *em)
{
    EmRemobra_EndState(em);
}

void EmRemobra_RunGroup1(Em *em)
{
    switch (em->state) {
    case 0:
        EmRemobra_State1_00(em);
        break;
    }
}

void func_00854BC0(Em *em)
{
    *(s16 *)((u8 *)em + 0x472) = 0;
    em->unk468 = 0;
    em->unk46C = 0;
}

void EmRemobra_UpdateHop(Em *em)
{
    EmRemobraWork *w = (EmRemobraWork *)&em->work;
    s16 i;
    f32 *p;

    if (em->motionId[0] == 0x3E9) {
        for (i = 0, p = &D_008CAE40; i < 3; i++, p++) {
            if (EmMotion_PassedFrameScaled(em, *p, 0) != 0) {
                w->hopFrames = 0x22;
                w->hopVel = 14.0f;
                w->hopAccel = -(w->hopVel / (w->hopFrames / 2));
                w->hopFrames += 1;
            }
        }
        if (w->hopFrames > 0) {
            w->hopFrames -= 1;
            em->posY += w->hopVel;
            w->hopVel += w->hopAccel;
        }
    }
}

void EmRemobra_State2_00(Em *em)
{
    f32 posY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stateVar0 = 0;
        em->stateVar1 = 0;
        em->stance = 2;
        em->timer = 0x1E;
        if (em->motionId[0] != 0x3E9) {
            Em_SetMotion(em, 1, 0, 0);
            func_00854BC0(em);
        }
        
    case 1:
        if (--em->timer <= 0) {
            EmRemobra_EndState2(em);
            break;
        }
        posY = em->posY;
        if (((posY - em->floorY) <= 400.0f) && (em->stateVar0 == 0)) {
            em->posY += 18.0f;
            break;
        }
        em->stateVar0 = 1;
        break;
    }
}

void EmRemobra_State2_01(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        Em_SetMotion(em, 3, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmRemobra_EndState2(em);
        }
        break;
    }
}

void EmRemobra_State2_02(Em *em, s32 variant)
{
    EmRemobraWork *w = (EmRemobraWork *)&em->work;
    s32 d;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        if (variant == 0) {
            Em_SetMotion(em, 4, 0, 0);
        } else {
            Em_SetMotion(em, 0xC, 0, 0);
        }
        Player_ClearVelocity(em);
        em->velZ = 45.0f;
    case 1:
        if (w->unk02F != 0) {
            d = (u16) (Math_AngleXZ(&em->posX, em->goal) - em->rotY);
            if (d <= 0x8000) {
                if (d < 0x100) {
                    em->rotY += d;
                } else {
                    em->rotY += 0x100;
                }
            } else if (d > 0xFF00) {
                em->rotY += d;
            } else {
                em->rotY -= 0x100;
            }
        }
        Em_MoveRotatedAccel(em, &em->rotX);
        if (em->curGoalKind == 1 && em->curGoalArg == 0) {
            if (Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) < 300.0f || --em->timer <= 0) {
                em->step++;
                Em_SetMotion(em, 4, 0, 0);
                em->velZ *= 0.8f;
                break;
            }
        } else {
            w->distLeft -= em->velZ;
            if (w->distLeft <= 0.0f) {
                em->step++;
                Em_SetMotion(em, 4, 0, 0);
                em->velZ *= 0.8f;
                break;
            }
        }
        if (em->motionId[0] == 0x3F4) {
            if (em->motion[0].playing == 0) {
                Em_SetMotion(em, 0xC, 8, 0);
            } else if (EmMotion_PassedFrameScaled(em, 24.0f, 0)) {
                EmRangedAttack_CreateAtBone(em, 0x13, 0x18, 0, 0x3C72, 0);
            }
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmRemobra_ChangeState(em, 2, 1, 0);
        } else {
            Em_MoveRotatedAccel(em, &em->rotX);
        }
        break;
    }
}

void EmRemobra_State2_03(Em *em, s32 kind, s32 dir)
{
    u16 d;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        Player_ClearVelocity(em);
        if (kind == 0) {
            if (dir == 0) {
                Em_SetMotion(em, 6, 0, 0);
            } else {
                Em_SetMotion(em, 7, 0, 0);
            }
            em->velZ = 24.0f;
        } else {
            if (dir == 0) {
                Em_SetMotion(em, 8, 0, 0);
            } else {
                Em_SetMotion(em, 9, 0, 0);
            }
            em->velZ = 33.75f;
        }
        em->timer = 30;
        Em_MoveRotatedAccel(em, &em->rotX);
        break;
    case 1:
        if (kind == 0) {
            if (dir == 0) {
                d = 0x200;
            } else {
                d = -0x200;
            }
        } else {
            if (dir == 0) {
                d = 0x400;
            } else {
                d = -0x400;
            }
        }
        em->rotY += d;
        Em_MoveRotatedAccel(em, &em->rotX);
        if (--em->timer <= 0) {
            EmRemobra_ChangeState(em, 2, 2, 0);
        }
        break;
    }
}

void EmRemobra_State2_07(Em *em, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        Player_ClearVelocity(em);
        Em_SetMotion(em, 0xA, 0, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            em->step++;
            if (variant == 0) {
                em->velY = 30.0f;
                em->accelY = -0.8f;
                em->velZ = -30.0f;
                break;
            }
            em->velY = 32.0f;
            em->accelY = -0.8f;
            em->velZ = -40.0f;
        }
        break;
    case 2:
        Em_MoveRotatedAccel(em, &em->rotX);
        if (em->motion[0].playing == 0) {
            EmRemobra_EndState2(em);
        }
        break;
    }
}

void EmRemobra_State2_09(Em *em, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        if (variant == 0) {
            Em_SetMotion(em, 0xB, 0, 0x5A);
            break;
        }
        Em_SetMotion(em, 0xB, 0, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 196.0f, 0U) != 0) {
            em->step++;
            em->stance = 2;
            em->posY += 400.0f;
            Em_SetMotionRow(em, 1, 0, 0);
            if (variant == 2) {
                em->noReact = 0;
            }
            EmRemobra_EndState2(em);
        }
        break;
    }
}

void EmRemobra_State2_0A(Em *em)
{
    EmRemobraWork *w = (EmRemobraWork *)&em->work;
    u16 target;
    u32 diff;
    s32 angle;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        if (em->motionId[0] != 0x3E9) {
            Em_SetMotion(em, 1, 0, 0);
            func_00854BC0(em);
        }
        break;
    case 1:
        angle = em->rotY;
        target = w->unk02C;
        diff = (target - (angle & 0xFFFF)) & 0xFFFF;
        if ((u32) ((diff + 0x440) & 0xFFFF) < 0x880U) {
            em->rotY = (s32) target;
            em->step++;
            Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
            EmRemobra_EndState2(em);
        } else if (diff < 0x8000U) {
            em->rotY = (angle + 0x440) & 0xFFFF;
        } else {
            em->rotY = (angle - 0x440) & 0xFFFF;
        }
        break;
    }
    Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
}

void EmRemobra_State2_0C(Em *em, s32 variant)
{
    f32 posY;
    f32 y;
    f32 goalY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        if (em->motionId[0] != 0x3E9) {
            Em_SetMotion(em, 1, 0, 0);
            func_00854BC0(em);
        }
        break;
    case 1:
        if (--em->timer <= 0) {
            EmRemobra_EndState2(em);
            break;
        }
        posY = 180.0f + em->floorY;
        if (em->posY < posY) {
            em->posY = posY;
            EmRemobra_EndState2(em);
        }
        if (variant == 0) {
            goalY = 600.0f - (f32) ((s32) em->stateRand % 400);
        } else {
            goalY = em->goal[1];
        }
        if (!(Math_Absf(em->posY - goalY) < 50.0f)) {
            y = em->posY;
            if (y <= goalY) {
                em->posY += 18.0f;
                break;
            }
            em->posY -= 18.0f;
            break;
        }
        EmRemobra_EndState2(em);
        break;
    }
}

void EmRemobra_State2_0F(Em *em, s32 variant)
{
    f32 v[3];
    f32 out[3];
    s32 ang[3];
    f32 mtx[16];
    EmRemobraWork *w = (EmRemobraWork *)&em->work;
    s32 d;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        if (em->motionId[0] != 0x3E9) {
            Em_SetMotion(em, 1, 0, 0);
            func_00854BC0(em);
        }
        break;
    case 1:
        if (w->unk02F != 0) {
            d = (u16) (Math_AngleXZ(&em->posX, em->goal) - em->rotY);
            if (d <= 0x8000) {
                if (d < 0x280) {
                    em->rotY += d;
                } else {
                    em->rotY += 0x280;
                }
            } else if (d > 0xFD80) {
                em->rotY += d;
            } else {
                em->rotY -= 0x280;
            }
        }
        Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = 20.0f;
        ang[0] = 0;
        ang[1] = em->rotY;
        ang[2] = 0;
        Mtx_FromAnglesYXZ(ang, mtx);
        Vec_Rotate(out, v, mtx);
        em->posX += out[0];
        em->posZ += out[2];
        if (em->curGoalKind == 1 && em->curGoalArg == 0) {
            if (Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) < 300.0f || --em->timer <= 0) {
                em->step++;
                EmRemobra_EndState2(em);
            }
        } else {
            w->distLeft -= v[2];
            if (w->distLeft <= 0.0f) {
                em->step++;
                EmRemobra_EndState2(em);
            }
        }
        break;
    case 2:
        EmRemobra_EndState2(em);
        break;
    }
}

void EmRemobra_State2_10(Em *em, s32 unused)
{
    f32 posY;

    switch (em->step) {
    case 0:
        em->stance = 2;
        if (em->posY <= (400.0f + em->floorY)) {
            em->step++;
            if (em->motionId[0] != 0x3E9) {
                Em_SetMotion(em, 1, 0, 0);
            }
            break;
        }
        em->step = 2;
        Em_SetMotion(em, 0x1A, 0, 0);
        break;
    case 1:
        posY = em->posY;
        if ((posY - em->floorY) <= 400.0f) {
            em->posY += 24.0f;
            break;
        }
        em->step++;
        Em_SetMotion(em, 0x1A, 0, 0);
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmRemobra_EndState2(em);
        }
        break;
    }
}

void EmRemobra_State2_11(Em *em)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->stance = 2;
        em->step++;
        em->timer = ((em->stateRand & 3) * 0xF) + 0x1E;
        Em_SetMotion(em, 1, 0, 0);
        Player_ClearVelocity(em);
        em->velX = 0.0f;
        em->velY = 0.0f;
        em->velZ = 20.0f;
        break;
    case 1:
        Em_TurnToGoal(em, 0x5B0, 0);
        EmRemobra_MoveByVelocity(em);
        floorY = em->floorY;
        if (em->posY < floorY) {
            em->posY = floorY;
        }
        if (--em->timer <= 0) {
            em->step++;
            EmRemobra_EndState2(em);
        }
        break;
    }
}

void EmRemobra_State2_15(Em *em, s32 variant)
{
    f32 out[3];
    s32 ang[3];
    f32 mtx[16];

    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 40;
        em->stance = 2;
        if (em->motionId[0] != 0x3E9) {
            Em_SetMotion(em, 1, 0, 0);
            func_00854BC0(em);
        }
        Player_ClearVelocity(em);
        switch (variant) {
        case 0:
            em->velZ = 20.0f;
            break;
        case 1:
            em->velZ = -23.0f;
            break;
        case 2:
            em->velX = 20.0f;
            break;
        case 3:
            em->velX = -20.0f;
            break;
        case 4:
            em->velY = 20.0f;
            em->velZ = -23.0f;
            break;
        }
        break;
    case 1:
        Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
        ang[0] = 0;
        ang[1] = em->rotY;
        ang[2] = 0;
        Mtx_FromAnglesYXZ(ang, mtx);
        Vec_Rotate(out, &em->velX, mtx);
        em->posX += out[0];
        em->posY += out[1];
        em->posZ += out[2];
        if (--em->timer <= 0) {
            em->step++;
            EmRemobra_EndState2(em);
        }
        break;
    case 2:
        EmRemobra_EndState2(em);
        break;
    }
}

void EmRemobra_RunGroup2(Em *em)
{
    f32 floorY;

    switch (em->state) {
    case 0:
        EmRemobra_State2_00(em);
        break;
    case 1:
        EmRemobra_State2_01(em);
        break;
    case 2:
        EmRemobra_State2_02(em, 0);
        break;
    case 3:
        EmRemobra_State2_03(em, 0, 0);
        break;
    case 4:
        EmRemobra_State2_03(em, 0, 1);
        break;
    case 5:
        EmRemobra_State2_03(em, 1, 0);
        break;
    case 6:
        EmRemobra_State2_03(em, 1, 1);
        break;
    case 7:
        EmRemobra_State2_07(em, 0);
        break;
    case 8:
        EmRemobra_State2_07(em, 1);
        break;
    case 9:
        EmRemobra_State2_09(em, 0);
        break;
    case 10:
        EmRemobra_State2_0A(em);
        break;
    case 11:
        EmRemobra_State2_09(em, 1);
        break;
    case 12:
        EmRemobra_State2_0C(em, 0);
        break;
    case 13:
        EmRemobra_State2_0C(em, 1);
        break;
    case 14:
        EmRemobra_State2_09(em, 2);
        break;
    case 15:
        EmRemobra_State2_0F(em, 0);
        break;
    case 16:
        EmRemobra_State2_10(em, 0);
        break;
    case 17:
        EmRemobra_State2_11(em);
        break;
    case 18:
        EmRemobra_State2_0A(em);
        break;
    case 19:
        EmRemobra_State2_0F(em, 0);
        break;
    case 20:
        EmRemobra_State2_02(em, 1);
        break;
    case 21:
        EmRemobra_State2_15(em, 0);
        break;
    case 22:
        EmRemobra_State2_15(em, 1);
        break;
    case 23:
        EmRemobra_State2_15(em, 2);
        break;
    case 24:
        EmRemobra_State2_15(em, 3);
        break;
    case 25:
        EmRemobra_State2_15(em, 4);
        break;
    case 26:
        EmRemobra_State2_0C(em, 1);
        break;
    }
    EmRemobra_UpdateHop(em);
    floorY = em->floorY;
    if (em->posY <= floorY) {
        em->posY = floorY;
    }
}

void EmRemobra_State3_00(Em *em, s32 unused)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        Em_SetMotion(em, 0x10, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmRemobra_EndState2(em);
            break;
        }
        if (EmMotion_PassedFrameScaled(em, 46.0f, 0U) != 0) {
            RemobraAttack_Create(em, 1);
        }
        break;
    }
}

void EmRemobra_State3_01(Em *em, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stateVar0 = 0;
        em->stance = 2;
        switch (variant) {
        case 0:
            em->timer = 40;
            break;
        case 1:
            em->timer = 50;
            break;
        case 2:
            em->timer = 60;
            break;
        }
        Em_SetMotion(em, 0xE, 0, 0);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0xF, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmRemobra_EndState2(em);
            break;
        }
        if (EmMotion_PassedFrameScaled(em, 8.0f, 0U) != 0) {
            switch (variant) {
            case 0:
                EmRangedAttack_CreateAtBone(em, 0x13, 0x18, 0, 0x2000, 0);
                break;
            case 1:
                EmRangedAttack_CreateAtBone(em, 0x13, 0x18, 0, 0x2000, 0);
                break;
            case 2:
                EmRangedAttack_CreateAtBone(em, 0x13, 0x18, 0, 0x2000, 0);
                break;
            }
        }
        break;
    }
}

void EmRemobra_State3_04(Em *em, s32 unused)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        Em_SetMotion(em, 0xC, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmRemobra_EndState2(em);
            break;
        }
        if (EmMotion_PassedFrameScaled(em, 24.0f, 0U) != 0) {
            EmRangedAttack_CreateAtBone(em, 0x13, 0x18, 0, 0x3C72, 0);
        }
        break;
    }
}

void EmRemobra_State3_05(Em *em, s32 unused)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        Em_SetMotion(em, 0x11, 0, 0);
        EmRemobra_StartCurves(em, 0U);
        break;
    case 1:
        EmRemobra_UpdateCurves(em);
        floorY = em->floorY;
        if (em->posY <= floorY) {
            em->posY = floorY;
        }
        if (em->motion[0].playing == 0) {
            EmRemobra_EndState2(em);
            break;
        }
        if (EmMotion_PassedFrameScaled(em, 104.0f, 0U) != 0) {
            RemobraAttack_Create(em, 2);
        }
        if (EmMotion_PassedFrameScaled(em, 168.0f, 0U) != 0) {
            RemobraAttack_Create(em, 3);
        }
        break;
    }
}

void EmRemobra_RunGroup3(Em *em)
{
    switch (em->state) {
    case 0:
        EmRemobra_State3_00(em, 0);
        break;
    case 1:
        EmRemobra_State3_01(em, 0);
        break;
    case 2:
        EmRemobra_State3_01(em, 1);
        break;
    case 3:
        EmRemobra_State3_01(em, 2);
        break;
    case 4:
        EmRemobra_State3_04(em, 0);
        break;
    case 5:
        EmRemobra_State3_05(em, 0);
        
    default:
        break;
    }
}

void EmRemobra_State4_00(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x12, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmRemobra_EndState2(em);
        }
        break;
    }
}

void EmRemobra_State4_01(Em *em, s32 unused)
{

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        Em_SetMotion(em, 0x13, 0, 0);
        Em_RestartScript(em);
        em->velY = 10.0f;
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 66.0f, 0U) != 0) {
            em->velY = 40.0f;
        }
        em->posY -= em->velY;
        if (em->posY <= em->floorY) {
            em->step++;
            em->stance = 0;
            em->posY = em->floorY;
            Em_SetMotion(em, 0x14, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmRemobra_ChangeState(em, 4, 2, 3);
        }
        break;
    case 3:
        EmRemobra_ChangeState(em, 4, 2, 3);
        break;
    }
}

void EmRemobra_State4_02(Em *em, s32 unused)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_STATUS);
    switch (em->step) {
    case 0:
        em->step++;
        Em_RestartScript(em);
        em->timer = 0x96;
        Em_SetMotion(em, 0x15, 0, 0);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0xB, 0, 0);
        }
        break;
    case 2:
        if (EmMotion_ReachedFrame(em, 0, 90.0f) != 0) {
            em->step++;
            EmRemobra_EndState(em);
        }
        break;
    }
}

void EmRemobra_State4_03(Em *em, s32 unused)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_STATUS);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0xB, 0, 0);
        em->noReact = 1;
        Em_RestartScript(em);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 90.0f) != 0) {
            em->step++;
            Em_SetMotion(em, 0x16, 0, 0);
        }
        break;
    case 2:
        Em_SpawnStatus4Effect(em, 2);
        if (--em->timer <= 0) {
            em->step++;
            EmRemobra_ChangeState(em, 2, 0xE, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmRemobra_ChangeState(em, 2, 0xE, 4);
        }
        break;
    }
}

void EmRemobra_RunGroup4(Em *em)
{
    switch (em->state) {
    case 0:
        EmRemobra_State4_00(em);
        break;
    case 1:
        EmRemobra_State4_01(em, 0);
        break;
    case 2:
        EmRemobra_State4_02(em, 0);
        break;
    case 3:
        EmRemobra_State4_03(em, 0);
        break;
    }
}

void EmRemobra_State5_00_Hunted(Em *em)
{
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->stateVar0 = 0;
        Em_SetMotion(em, 0x19, 0, 0);
        em->lookAtTarget = 0;
        em->stance = 3;
        QuestEm_Hunted(em);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 158.0f, 0) != 0 && em->stateVar0 == 0) {
            em->stateVar0 = 1;
            EmMine_Create(em, 0);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 300;
            em->fieldCollide = 0;
        }
        break;
    case 2:
        EmMine_Update(em);
        if (--em->timer <= 0) {
            em->step++;
            em->timer = 150;
            EmMine_Remove(em);
        }
        break;
    case 3:
        if (em->timer > 0) {
            em->timer--;
        }
        if (em->timer <= 0) {
            if (em->remote == 0) {
                EmRemobra_ChangeState(em, 5, 1, 4);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    }
}

void EmRemobra_State5_01(Em *em)
{
    switch (em->step) {
    case 0:
        if (QuestEm_CanAppear(em) == 1) {
            Em_SetMotionRow(em, 1, 0, 0);
            EmRemobra_ChangeState(em, 7, 0, 0);
            break;
        }
        em->taskPhase++;
        break;
    }
}

void EmRemobra_State5_02(Em *em)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        Em_SetMotion(em, 0x13, 0, 0);
        em->velY = 10.0f;
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 66.0f, 0U) != 0) {
            em->velY = 40.0f;
        }
        em->posY -= em->velY;
        floorY = em->floorY;
        if (em->posY <= floorY) {
            em->posY = floorY;
            em->step++;
            Em_SetMotion(em, 0x14, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmRemobra_ChangeState(em, 5, 0, 3);
        }
        break;
    }
}

void EmRemobra_RunGroup5(Em *em)
{
    em->noHitTimer = 0xA;
    em->noPushTimer = 0xA;
    switch (em->state) {
    case 0:
        EmRemobra_State5_00_Hunted(em);
        break;
    case 1:
        EmRemobra_State5_01(em);
        break;
    case 2:
        EmRemobra_State5_02(em);
        break;
    }
}

void EmRemobra_State6_00(Em *em)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
}

void EmRemobra_RunGroup6(Em *em)
{
    switch (em->state) {
    case 0:
        EmRemobra_State6_00(em);
        break;
    }
}

void EmRemobra_State7_00(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 1, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmRemobra_EndState2(em);
        }
        break;
    }
}

void EmRemobra_RunGroup7(Em *em)
{
    em->noHitTimer = 0xA;
    em->noPushTimer = 0xA;
    em->senseDelay = 5;
    switch (em->state) {
    case 0:
        EmRemobra_State7_00(em);
        break;
    }
}

void EmRemobra_Update(Em *em)
{
    u8 buf[3];

    em->motionSpeed = 1.0f;
    Em_UpdateCombatTimer(em);
    switch (Em_UpdateDamage(em, buf) & 0xFF) {
    case 0:
    case 3:
    case 4:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 18:
        break;
    case 1:
    case 2:
        if (em->stance == 2) {
            EmRemobra_ChangeState(em, 5, 2, 3);
        } else {
            EmRemobra_ChangeState(em, 5, 0, 3);
        }
        em->scriptPending = 0;
        break;
    case 5:
        if (em->stateGroup != 4 || em->state != 1) {
            EmRemobra_ChangeState(em, 4, 1, 3);
        }
        break;
    case 6:
        if (em->stateGroup != 4 || em->state != 1) {
            Em_SetStatus4Timer(em);
            EmRemobra_ChangeState(em, 4, 3, 3);
        }
        break;
    case 12:
    case 13:
        if (em->stance == 2) {
            EmRemobra_ChangeState(em, 4, 1, 3);
        }
        break;
    case 14:
        if (em->stance == 2) {
            EmRemobra_ChangeState(em, 4, 0, 3);
        }
        break;
    case 17:
        Em_EndStatus5(em);
        break;
    case 15:
    case 16:
        if (em->stance == 2) {
            EmRemobra_ChangeState(em, 4, 1, 3);
        }
        break;
    }
    Em_UpdatePendingScript(em);
    Em_ClearHitReactMode(em);
    EmRemobra_RunState(em);
    if (em->stateRequest != 0) {
        EmRemobra_RunState(em);
        em->stateRequest = 0;
    }
}

void EmRemobra_RunState(Em *em)
{
    switch (em->stateGroup) {
    case 0:
        EmRemobra_RunGroup0(em);
        break;
    case 1:
        EmRemobra_RunGroup1(em);
        break;
    case 2:
        EmRemobra_RunGroup2(em);
        break;
    case 3:
        EmRemobra_RunGroup3(em);
        break;
    case 4:
        EmRemobra_RunGroup4(em);
        break;
    case 5:
        EmRemobra_RunGroup5(em);
        break;
    case 6:
        EmRemobra_RunGroup6(em);
        break;
    case 7:
        EmRemobra_RunGroup7(em);
        
    default:
        break;
    }
}

void EmRemobra_SpawnGroundEffect(Em *em, s32 a1, s32 a2, f32 scale)
{
    f32 pos[3];

    if (Player_InCurrentArea(em)) {
        Em_BonePos(em, a1, pos);
        pos[1] = em->floorY;
        Effect0D_SpawnOnGround(pos, a2, scale * em->scaleX);
    }
}

void EmRemobra_SpawnGroundEffectNearGround(Em *em, s32 arg1, s32 arg2, f32 scale)
{
    if (em->posY <= (300.0f + em->floorY)) {
        EmRemobra_SpawnGroundEffect(em, arg1, arg2, scale);
    }
}

void EmRemobra_MotionEffects(Em *em, EmWork *work)
{
    s16 motion;
    u16 motionId;

    motionId = em->motionId[0];
    if (motionId != work->fxMotion) {
        work->fxMotion = (s16) motionId;
    }
    EmRemobra_MotionSounds((s32) em, work);
    motion = work->fxMotion;
    switch (motion) {
    case 0x3E9:
        if ((EmMotion_PassedFrameScaled(em, 4.0f, 0U) == 0) && (EmMotion_PassedFrameScaled(em, 84.0f, 0U) == 0)) {
            if (EmMotion_PassedFrameScaled(em, 156.0f, 0U) != 0) {
                goto block_15;
            }
            break;
        }
block_15:
        EmRemobra_SpawnGroundEffectNearGround(em, 1, 7, 2.0f);
        break;
    case 0x3EA:
        if (EmMotion_PassedFrameScaled(em, 84.0f, 0U) != 0) {
            EmRemobra_SpawnGroundEffect(em, 3, 7, 2.0f);
        }
        break;
    case 0x3EB:
        if ((EmMotion_PassedFrameScaled(em, 50.0f, 0U) != 0) || (EmMotion_PassedFrameScaled(em, 102.0f, 0U) != 0)) {
            EmRemobra_SpawnGroundEffectNearGround(em, 1, 7, 2.0f);
        }
        break;
    case 0x3F2:
        if (EmMotion_PassedFrameScaled(em, 52.0f, 0U) != 0) {
            EmRemobra_SpawnGroundEffectNearGround(em, 1, 7, 2.0f);
        }
        break;
    case 0x3F3:
        if (EmMotion_PassedFrameScaled(em, 138.0f, 0U) != 0) {
            EmRemobra_SpawnGroundEffect(em, 1, 4, 2.0f);
        }
        break;
    case 0x3F6:
        if (EmMotion_PassedFrameScaled(em, 78.0f, 0U) != 0) {
            EmRemobra_SpawnGroundEffectNearGround(em, 1, 7, 2.0f);
        }
        break;
    case 0x3FC:
        if (EmMotion_PassedFrameScaled(em, 14.0f, 0U) != 0) {
            EmRemobra_SpawnGroundEffect(em, 3, 7, 2.0f);
        }
        break;
    case 0x3FF:
        if (EmMotion_PassedFrameScaled(em, 164.0f, 0U) != 0) {
            EmRemobra_SpawnGroundEffect(em, 3, 7, 2.0f);
        }
        break;
    }
}

void EmRemobra_UpdateMotionEffects(Em *em)
{
    EmRemobraWork *w = (EmRemobraWork *)&em->work;

    switch (w->fxStep) {
    case 0:
        w->fxStep++;
        break;
    case 1:
        EmRemobra_MotionEffects(em, (EmWork *) w);
        break;
    }
}

void EmRemobra_MoveByVelocity(Em *em)
{
    f32 v[3];

    Vec_Copy(v, &em->velX);
    Vec_RotateY(v, 6.2831855f * em->rotY / 65536.0f);
    em->posX += v[0];
    em->posY += v[1];
    em->posZ += v[2];
}

void EmRemobra_StartCurves(Em *em, u8 a)
{
    EmRemobraWork *w = (EmRemobraWork *)&em->work;

    w->curveNone = 0;
    w->curveVelY = 0;
    w->curveSpeed = 0;
    w->curveKind = a;
    switch (w->curveKind) {
    case 0:
        w->curveVelY = 1;
        w->curveSpeed = 1;
        w->curveFrame = 0;
        break;
    }
    Player_ClearVelocity(em);
}

static u8 EmRemobra_CurveVelY(Em *em, EmWork *work)
{
    EmCurveKey *p;
    f32 frame;
    f32 t;
    s32 i;
    EmCurveKey *tbl;
    u8 ret;

    tbl = EmRemobra_VelYCurves[((EmRemobraWork *)work)->curveKind];
    ret = 0;
    if (tbl == NULL) {
        return ret;
    }
    frame = ((EmRemobraWork *)work)->curveFrame;
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

static u8 EmRemobra_CurveSpeed(Em *em, EmWork *work)
{
    EmCurveKey *p;
    f32 frame;
    f32 t;
    s32 i;
    EmCurveKey *tbl;
    u8 ret;

    tbl = EmRemobra_SpeedCurves[((EmRemobraWork *)work)->curveKind];
    ret = 0;
    if (tbl == NULL) {
        return ret;
    }
    frame = ((EmRemobraWork *)work)->curveFrame;
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

u8 EmRemobra_UpdateCurves(Em *em)
{
    EmRemobraWork *w = (EmRemobraWork *)&em->work;
    u8 ret = 0;

    if (w->curveVelY != 0) {
        ret |= EmRemobra_CurveVelY(em, (EmWork *) w);
    }
    if (w->curveSpeed != 0) {
        ret |= EmRemobra_CurveSpeed(em, (EmWork *) w);
    }
    w->curveFrame += (s16) em->motion[0].step;
    if (em->stance == 2) {
        Em_MoveRotatedAccel(em, &em->rotX);
    }
    return ret;
}

void EmRemobra_SpawnEffect01(u8 *em)
{
    Effect01_Spawn((Player *) em, 0);
}

void EmRemobra_TableNop(void)
{
}

void EmChameleos_SetGroup0State(Em *em, u16 state, u8 mode)
{
    em->motionSpeed = 1.0f;
    switch (state) {
    case 1:
        if (em->combat == 1) {
            state = 2;
        }
        break;
    case 4:
        EmChameleos_SetFlag43C(em);
        break;
    case 5:
        EmChameleos_ClearFlag43C(em);
        break;
    case 18:
        Em_ResetNotice(em);
        break;
    case 20:
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
    case 32:
        Em_EndStatus5(em);
        break;
    case 33:
        Em_EndStatus4(em);
        break;
    }
    em->stance = 0;
    Em_SetStateSync(em, 0, state, mode);
}

void EmChameleos_SetGroup1State(Em *em, u16 state, u8 mode)
{
    f32 f;
    f32 rate;

    em->stance = 0;
    em->motionSpeed = 1.0f;
    switch (state) {
    case 0:
        rate = 0.15f;
        if (em->hp < (s16) (em->hpMax * rate)) {
            state = 1;
            goto state1;
        }
        Em_SetSpeedScaled(em, 0, 1000.0f, 500.0f);
        break;
    case 1:
    state1:
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
    case 4:
        Em_SetSpeedScaled(em, 2, 1000.0f, 500.0f);
        break;
    case 7:
        em->hasGoal = 0;
        em->goalDist = 1000.0f;
        break;
    case 2:
    case 3:
    case 5:
    case 6:
        break;
    }
    Em_SetStateSync(em, 1, state, mode);
}

void EmChameleos_SetGroup2State(Em *em, u16 state, u8 mode)
{
    EmChameleosWork *w = (EmChameleosWork *)&em->work;

    em->motionSpeed = 1.0f;
    switch (state) {
    case 1:
        if (em->remote == 0 && !(em->velZ < 50.0f)) {
            state = 18;
        }
        break;
    case 6:
    case 17:
        w->unk005 = 4;
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
            EmChameleos_ChangeState(em, 2, 1, 1);
            return;
        }
        break;
    case 9:
        if (em->remote == 0) {
            em->goalKind = 2;
            em->goalArg = 3;
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
    case 19:
        w->unk005 = 1;
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

void EmChameleos_SetGroup3State(Em *em, u16 state, u8 mode)
{
    f32 pos[3];

    em->motionSpeed = 1.0f;
    switch (state) {
    case 5:
        if (em->remote == 0 && (Game_IsOnline() == 0 || mode != 0) && em->curGoalKind == 1 && em->curGoalArg == 0 &&
            em->targetNo != -1) {
            Em_GetTargetPos(em, em->targetNo, pos);
            if (pos[1] <= em->posY + 50.0f) {
                state = 1;
            } else {
                state = 5;
            }
        }
        break;
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 6:
    case 7:
    case 8:
        break;
    }
    Em_SetStateSync(em, 3, state, mode);
}

void EmChameleos_ChangeState(Em *em, u16 group, u16 state, u8 mode)
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
        EmChameleos_SetGroup0State(em, state, mode);
        return;
    case 1:
        EmChameleos_SetGroup1State(em, state, mode);
        return;
    case 2:
        EmChameleos_SetGroup2State(em, state, mode);
        return;
    case 3:
        EmChameleos_SetGroup3State(em, state, mode);
        return;
    case 5:
        if ((state == 3) && (QuestEm_CanAppear(em) == 1)) {
            QuestEm_Appear(em);
        }
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        return;
    case 4:
        switch (state) {
        case 4:
            em->partBreakCount = (s8) em->partBreakCount + 1;
            em->fxFlags |= 1;
            Em_EndAllStatus(em);
            Em_BreakPart(em, 1U);
            break;
        case 5:
            Em_BeginStatus5(em);
            break;
        case 11:
            Em_BeginStatus4(em);
            break;
        case 12:
        case 13:
            EmChameleos_ClearFlag43C(em);
            break;
        }
        
    case 6:
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        
    default:
        return;
    }
}

void EmChameleos_StartCurves(Em *em, u8 kind)
{
    EmChameleosWork *w = (EmChameleosWork *)&em->work;

    u8 k;

    w->curveNone = 0;
    w->curveVelY = 0;
    w->curveSpeed = 0;
    w->curveKind = kind;
    k = w->curveKind;
    switch (k) {
    case 0:
        w->curveVelY = 1;
        w->curveSpeed = 0;
        w->curveFrame = 0x3A;
        break;
    case 1:
        w->curveVelY = 1;
        w->curveSpeed = 1;
        w->curveFrame = 0x66;
        break;
    case 2:
        w->curveVelY = 1;
        w->curveSpeed = 1;
        w->curveFrame = 0x4E;
        break;
    }
    Player_ClearVelocity(em);
}

static s32 EmChameleos_CurveNone(Em *em, EmWork *work)
{
    return 0;
}


extern EmCurveKey *EmChameleos_VelYCurves[];

/* The value of curve work[0x47] at the frame in the work area (a table of keys): sets
 * em->velY from the key pair around the frame, 2 at the table's end. */
static u8 EmChameleos_CurveVelY(Em *em, EmWork *work)
{
    EmChameleosWork *w = (EmChameleosWork *) work;
    EmCurveKey *p;
    f32 frame;
    f32 t;
    s32 i;
    EmCurveKey *tbl;
    u8 ret;

    tbl = EmChameleos_VelYCurves[((EmChameleosWork *)work)->curveKind];
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

extern EmCurveKey *EmChameleos_SpeedCurves[];

/* The value of curve work[0x47] at the frame in the work area: sets em->velZ from the key
 * pair around the frame, 4 at the table's end. */
static u8 EmChameleos_CurveSpeed(Em *em, EmWork *work)
{
    EmChameleosWork *w = (EmChameleosWork *) work;
    EmCurveKey *p;
    f32 frame;
    f32 t;
    s32 i;
    EmCurveKey *tbl;
    u8 ret;

    tbl = EmChameleos_SpeedCurves[((EmChameleosWork *)work)->curveKind];
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
u8 EmChameleos_UpdateCurves(Em *em)
{
    EmChameleosWork *w = (EmChameleosWork *)&em->work;
    u8 ret = 0;

    if ((u8)w->curveNone != 0) {
        ret = EmChameleos_CurveNone(em, (EmWork *) w);
    }
    if ((u8)w->curveVelY != 0) {
        ret |= EmChameleos_CurveVelY(em, (EmWork *) w);
    }
    if ((u8)w->curveSpeed != 0) {
        ret |= EmChameleos_CurveSpeed(em, (EmWork *) w);
    }
    w->curveFrame += (s16) em->motion[0].step;
    if (em->stance == 2) {
        w->moveAngCX = 0;
        w->moveAngCY = em->rotY;
        w->moveAngX = 0;
        Em_MoveRotatedAccel(em, &w->moveAngCX);
    }
    return ret;
}

void EmChameleos_SetFlag43C(Em *em)
{
    em->unk43C = 1;
}

void EmChameleos_ClearFlag43C(Em *em)
{
    em->unk43C = 0;
}

s32 EmChameleos_IsInvisible(Em *em)
{
    if (em->unk484 != 0xFFFF || (EmChameleos_WantsInvisible(em) & 0xFF) != 0) {
        return 1;
    }
    return 0;
}

s32 EmChameleos_HasFlag43C(Em *em)
{
    return em->unk43C != 0;
}

void EmChameleos_UpdateInvisibility(Em *em)
{
    EmChameleosWork *w = (EmChameleosWork *)&em->work;

    s16 t;
    s32 want;
    u16 temp_v0_2;
    u16 temp_v1;

    t = w->unk04E - 1;
    w->unk04E = t;
    if (t <= 0) {
        w->unk04E = 0x384;
    }
    if (EmChameleos_IsInvisible(em) == 1) {
        temp_v0_2 = w->unk04C;
        if (temp_v0_2 == 0xFFFF) {
            if ((em->stateGroup == 6) && (em->state == 0)) {
                w->unk04C = 1;
            } else {
                want = EmChameleos_WantsInvisible(em) & 0xFF;
                switch (want) {
                case 0:
                    w->unk04C = 0xFFFF;
                    break;
                case 2:
                    if (w->wantInvisible == 0) {
                        w->unk04C = 1;
                    }
                    if (w->unk04E == 0x384) {
                        w->unk04C = 1;
                    }
                    break;
                case 1:
                    w->unk04C = 1;
                    break;
                }
            }
        } else {
            w->unk04C = temp_v0_2 + 1;
            if ((s32) w->unk04C >= 0x20) {
                w->unk04C = 0xFFFF;
            }
        }
        w->wantInvisible = EmChameleos_WantsInvisible(em);
        return;
    }
    if (w->wantInvisible == 2) {
        if (EmChameleos_InvisibilityBlocked(em) == 1) {
            w->unk04C = 0xFFFF;
        } else if (w->unk04C == 0xFFFF) {
            w->unk04C = 0;
        }
    }
    temp_v1 = w->unk04C;
    if (temp_v1 != 0xFFFF) {
        w->unk04C = temp_v1 + 1;
        if ((s32) w->unk04C >= 0x20) {
            w->unk04C = 0xFFFF;
        }
    }
    w->wantInvisible = 0;
}

s32 EmChameleos_WantsInvisible(Em *em)
{
    s32 r;
    if (EmChameleos_InvisibilityBlocked(em) == 1 || Player_IsState(em, 1, 4) || em->hitReactMode == EM_HIT_REACT_WAKE || em->hitReactMode == EM_HIT_REACT_STATUS) {
        return 0;
    }
    r = (u8) EmChameleos_CanTurnInvisible(em);
    if (r == 0) {
        return r;
    }
    switch (em->motionId[0]) {
    case 0x411: case 0x412: case 0x413: case 0x415: case 0x416: case 0x417:
    case 0x419: case 0x41A: case 0x41B: case 0x41C:
        return 0;
    case 0x414: case 0x418: case 0x41D:
        return 1;
    }
    if (EmChameleos_HasFlag43C(em) == 0) {
        if (em->stateGroup == 6 && em->state == 0) {
            return 0;
        }
        return 1;
    }
    return r;
}

s32 EmChameleos_CanTurnInvisible(Em *em)
{
    if (em->fxFlags & 1) {
        if (Em_PartBroken(em, 0) != 1) {
            return 1;
        }
        return 0;
    }
    return 2;
}

s32 EmChameleos_InvisibilityBlocked(Em *em)
{
    if (em->stateGroup == 5 || Player_IsState(em, 6, 1) || Player_IsState(em, 6, 2)) {
        return 1;
    }
    return 0;
}

void EmChameleos_SpawnPeriodicEffect06(Em *em)
{
    f32 v[3];

    v[0] = 0.0f;
    v[1] = 80.0f;
    v[2] = 290.0f;
    Em_SpawnLyingPuffs(em, 0x25, v, 2.0f);
}

s32 func_00858980(Em *em)
{
    u16 idx;
    u8 temp_v1;
    void *temp_a0;
    void *contact;

    if (em->hitPending != 0) {
        contact = em->contact;
        if (contact != NULL) {
            idx = *(u16 *)((u8 *)contact + 0xC);
            if (((s32) idx < 4) && (contact == &gPlayers[idx]) && (*(u8 *)((u8 *)contact + 0) != 0)) {
                temp_a0 = *(void **)((u8 *)em + 0x6F0);
                if ((*(u8 *)((u8 *)temp_a0 + 2) == 0x28) && ((temp_v1 = *(u8 *)((u8 *)temp_a0 + 3), (temp_v1 == 1)) || (temp_v1 == 2))) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

/* Bulldrome's own meanings of the work area's bytes (EmWork, include/em.h, has the shared ones). */
typedef struct EmBulldromeWork {
    u8 unk000[0x00C - 0x000];
    u16 unk00C;                 /* 0x00C: Init: 0; Update tests it */
    s16 unk00E;                 /* 0x00E: Init: 0 */
    u8 unk010[0x014 - 0x010];
    f32 home[3];                /* 0x014: Init: the position; State1_05 turns toward it */
    u8 unk020[0x024 - 0x020];
    f32 distLeft;               /* 0x024: SetGroup2State: the distance to the goal; State2_02 takes the step from it */
    u16 unk028;                 /* 0x028: State2_01: Math_AngleXZ to the goal */
    u8 unk02A[0x02B - 0x02A];
    u8 unk02B;                  /* 0x02B: SetGroup2State: 1; State2_02 tests it */
    u8 unk02C;                  /* 0x02C: State1_07: 0 or 1 */
    u8 unk02D[0x02E - 0x02D];
    s16 unk02E;                 /* 0x02E: State1_07: 0x135, or a distance / 53 */
    u8 unk030[0x100 - 0x030];
} EmBulldromeWork;
typedef char EmBulldromeWork_size_check[(sizeof(EmBulldromeWork) == sizeof(EmWork)) ? 1 : -1];

void EmBulldrome_Init(Em *em)
{
    EmBulldromeWork *w = (EmBulldromeWork *)&em->work;

    f32 fslot;
    s16 size;
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
    Em_SetMotionRow(em, 1, 0, 0);
    em->stance = 0;
    size = Em_ComputeMaxHp(em, (s16) em->variant);
    em->hp = size;
    em->hpMax = size;
    em->scriptPending = 1;
    em->blind = 0;
    em->unk_InitFlag = 1;
    w->unk00C = 0;
    w->unk00E = 0;
    w->home[0] = em->posX;
    w->home[1] = 0.0f;
    w->home[2] = (f32) em->posZ;
    Em_InitParts(em);
}

void EmBulldrome_Start(Em *em)
{
    EmBulldrome_Init(em);
    EmBulldrome_ChangeState(em, 0, 1, 0);
}

void EmBulldrome_Reinit(Em *em)
{
    EmBulldrome_Init(em);
}

void EmBulldrome_EndState(Em *em)
{
    em->motionSpeed = 1.0f;
    em->stance = 0;
    em->lookAtTarget = 0;
    em->fieldCollide = 1;
    em->scriptPending = 1;
    EmBulldrome_ChangeState(em, 0, 1, 0);
}

void EmBulldrome_State0_00(void)
{
}

void EmBulldrome_State0_01(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 1, 0xA, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmBulldrome_EndState(em);
        }
        break;
    }
}

void EmBulldrome_State0_02(void)
{
}

void EmBulldrome_State0_03(Em *em)
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
            EmBulldrome_EndState(em);
        }
        break;
    }
}

void EmBulldrome_State0_04(Em *em, s32 timer)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->timer = timer;
        Em_SetMotion(em, 0xE, 6, 0);
        break;
    case 1:
        Em_TurnToGoal(em, 0x200, 0);
        if (em->motion[0].playing == 0) {
            if (--em->timer <= 0) {
                EmBulldrome_EndState(em);
            }
        }
        break;
    }
}

void EmBulldrome_State0_05(void)
{
}

void EmBulldrome_State0_06(Em *em, s32 unused)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x11, 6, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmBulldrome_EndState(em);
        }
        break;
    }
}

void EmBulldrome_State0_09(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0xB, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmBulldrome_EndState(em);
        }
        break;
    }
}

void EmBulldrome_State0_0B(Em *em, s32 variant)
{
    s32 k2;
    s32 k;

    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x46, 0, 0);
        if ((variant & 0xFFFF) == 2) {
            em->timer = 0x1518;
        }
        break;
    case 1:
        Em_SpawnLyingPuffs(em, 0xB, (f32 *) &EmBulldrome_Effect0BOfs, 1.0f);
        k = variant & 0xFFFF;
        if (k == 2) {
            Em_RegenHp(em, 1, (s32) (0.6f * (f32) em->hpMax), 0xA);
        }
        if (--em->timer <= 0) {
            em->step++;
            if (k == 2) {
                Em_ClearLowHpNeed(em);
            }
            switch (k) {
            case 0:
                EmBulldrome_ChangeState(em, 0, 0xC, 4);
                break;
            case 1:
                EmBulldrome_ChangeState(em, 0, 0xF, 4);
                break;
            case 2:
                EmBulldrome_ChangeState(em, 0, 0x13, 4);
                break;
            }
        }
        break;
    case 2:
        k2 = variant & 0xFFFF;
        if (em->remote == 0) {
            switch (k2) {
            case 0:
                EmBulldrome_ChangeState(em, 0, 0xC, 4);
                break;
            case 1:
                EmBulldrome_ChangeState(em, 0, 0xF, 4);
                break;
            case 2:
                EmBulldrome_ChangeState(em, 0, 0x13, 4);
                break;
            }
        }
        break;
    }
}

void EmBulldrome_State0_0C(Em *em, s32 unused)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->blind = 2;
        Em_SetMotion(em, 0x49, 0, 0);
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            EmBulldrome_ChangeState(em, 0, 9, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmBulldrome_ChangeState(em, 0, 9, 4);
        }
        break;
    }
}

void EmBulldrome_State0_0D(Em *em)
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
            EmBulldrome_EndState(em);
        }
        break;
    }
}

void EmBulldrome_RunGroup0(Em *em)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->state) {
    case 0:
        EmBulldrome_State0_00();
        break;
    case 1:
        EmBulldrome_State0_01(em);
        break;
    case 2:
        EmBulldrome_State0_02();
        break;
    case 3:
        EmBulldrome_State0_03(em);
        break;
    case 4:
        EmBulldrome_State0_04(em, 1);
        break;
    case 5:
        EmBulldrome_State0_05();
        break;
    case 6:
        EmBulldrome_State0_06(em, 1);
        break;
    case 7:
        EmBulldrome_State0_04(em, 2);
        break;
    case 8:
        EmBulldrome_State0_04(em, 4);
        break;
    case 9:
        EmBulldrome_State0_09(em);
        break;
    case 10:
        EmBulldrome_State0_06(em, 0);
        break;
    case 11:
        EmBulldrome_State0_0B(em, 0);
        break;
    case 12:
        EmBulldrome_State0_0C(em, 0);
        break;
    case 13:
        EmBulldrome_State0_0D(em);
        break;
    case 14:
        EmBulldrome_State0_0B(em, 1);
        break;
    case 15:
        EmBulldrome_State0_0C(em, 1);
        break;
    case 16:
        EmBulldrome_State0_0D(em);
        break;
    case 17:
        EmBulldrome_State0_0D(em);
        break;
    case 18:
        EmBulldrome_State0_0B(em, 2);
        break;
    case 19:
        EmBulldrome_State0_0C(em, 2);
        break;
    case 20:
        EmBulldrome_State0_06(em, 0);
        break;
    case 21:
        EmBulldrome_State0_06(em, 0);
        
    default:
        break;
    }
}

void EmBulldrome_State1_00(Em *em, s32 variant)
{
    u32 d;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        if (variant == 0) {
            Em_StartTurnToTarget(em, &EmBulldrome_TurnParams[0]);
            break;
        }
        em->aimAngle = em->rotY + 0x4000;
        d = (em->aimAngle - em->rotY) & 0xFFFF;
        if (d <= 0x1000 || d >= 0xF000) {
            Player_SetFlag((Player *) em, EM_FLAG_NO_ROOT_MOTION);
            Em_SetMotion(em, 2, 0, 0);
        } else if (d >= 0x8000) {
            Em_SetMotion(em, 5, 0, 0);
        } else {
            Em_SetMotion(em, 4, 0, 0);
        }
        break;
    case 1:
        if (Em_TurnStep(em, &EmBulldrome_TurnParams[0]) == 1) {
            em->step++;
            Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
            EmBulldrome_EndState(em);
        }
        break;
    }
}

void EmBulldrome_State1_01(Em *em, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, (u8) variant == 0 ? 2 : 3, 6, 0);
        break;
    case 1:
        if (Em_StepToGoal(em, 0x200, 0) == 1) {
            EmBulldrome_EndState(em);
        }
        break;
    }
}

void EmBulldrome_State1_03(void)
{
}

void EmBulldrome_State1_04(void)
{
}

void EmBulldrome_State1_05(Em *em)
{
    EmBulldromeWork *w = (EmBulldromeWork *)&em->work;
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
            EmBulldrome_EndState(em);
            break;
        }
        em->targetAngle = Math_AngleXZ(&em->posX, &w->home[0]) & 0xFFFF;
        Angle_TurnToward((s32 *) &em->rotY, (s32 *) &em->targetAngle, 0x80);
        Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
        break;
    }
}

void EmBulldrome_State1_07(Em *em, u8 variant)
{
    EmBulldromeWork *w = (EmBulldromeWork *)&em->work;
    u16 d;
    s16 v;

    switch (em->step) {
    case 0:
        em->step++;
        d = Math_AngleXZ(&em->posX, em->goal) - em->rotY;
        if (d < 0x8000) {
            w->unk02C = 0;
        } else {
            w->unk02C = 1;
            d = 0x10000 - d;
        }
        if (variant == 0) {
            w->unk02E = 0x135;
        } else {
            if (d < 0x2000) {
                d = 0x2000;
            } else if (d > 0x6000) {
                d = 0x6000;
            }
            w->unk02E = d / 53;
        }
        Em_SetMotion(em, 5 - w->unk02C, 0, 0);
        break;
    case 1:
        v = 0x135;
        if (w->unk02C != 0) {
            v = -0x135;
        }
        em->rotY += v;
        if (em->motion[0].playing == 0) {
            EmBulldrome_EndState(em);
        }
        break;
    }
}

void EmBulldrome_State1_08(Em *em, u8 variant)
{
    EmBulldromeWork *w = (EmBulldromeWork *)&em->work;
    u16 d;

    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x15, 0, 0);
        if (variant == 0) {
            w->unk02E = 0x26A;
            break;
        }
        d = Math_AngleXZ(&em->posX, em->goal) - em->rotY;
        if (d < 0x4000) {
            d = 0x4000;
        } else if (d > 0xA000) {
            d = 0xA000;
        }
        w->unk02E = d / 53;
        break;
    case 1:
        em->rotY += w->unk02E;
        if (em->motion[0].playing == 0) {
            EmBulldrome_EndState(em);
        }
        break;
    }
}

void EmBulldrome_RunGroup1(Em *em)
{
    switch (em->state) {
    case 0:
        EmBulldrome_State1_00(em, 0);
        break;
    case 1:
        EmBulldrome_State1_01(em, 0);
        break;
    case 2:
        EmBulldrome_State1_01(em, 1);
        break;
    case 3:
        EmBulldrome_State1_03();
        break;
    case 4:
        EmBulldrome_State1_04();
        break;
    case 5:
        EmBulldrome_State1_05(em);
        break;
    case 6:
        EmBulldrome_State1_00(em, 1);
        break;
    case 7:
        EmBulldrome_State1_07(em, 0);
        break;
    case 8:
        EmBulldrome_State1_08(em, 0);
        break;
    case 9:
        EmBulldrome_State1_07(em, 1);
        break;
    case 10:
        EmBulldrome_State1_08(em, 1);
        
    default:
        break;
    }
}

void EmBulldrome_State2_00(Em *em)
{
    em->stateArmor = 1;
    em->noPushTimer = 2;
    em->guardTimer = 2;
    em->noReact = 1;
    em->hideShadowHold = 2;
    em->hideShadow = 2;
    em->fieldCollide = 0;
    em->noHitTimer = 5;
    em->senseDelay = 5;
    switch (em->step) {
    case 0:
        em->stance = 0;
        em->step++;
        Em_SetMotion(em, 0x16, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBulldrome_ChangeState(em, 2, 1, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmBulldrome_ChangeState(em, 2, 1, 4);
        }
        break;
    }
}

void EmBulldrome_State2_01(Em *em)
{
    EmBulldromeWork *w = (EmBulldromeWork *)&em->work;

    u16 ang;
    u32 diff;
    s32 rotY;

    em->stateArmor = 1;
    em->noPushTimer = 2;
    em->guardTimer = 2;
    em->noReact = 1;
    em->hideShadowHold = 2;
    em->hideShadow = 2;
    em->fieldCollide = 0;
    em->noHitTimer = 5;
    em->senseDelay = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        w->unk028 = Math_AngleXZ(&em->posX, em->goal);
        Em_SetMotion(em, 0x18, 0, 0);
        break;
    case 1:
        rotY = em->rotY;
        ang = w->unk028;
        diff = (ang - (rotY & 0xFFFF)) & 0xFFFF;
        if ((u32) ((diff + 0x400) & 0xFFFF) < 0x800U) {
            em->rotY = (s32) ang;
            em->step++;
            Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
            EmBulldrome_ChangeState(em, 2, 2, 4);
        } else if (diff < 0x8000U) {
            em->rotY = (rotY + 0x400) & 0xFFFF;
        } else {
            em->rotY = (rotY - 0x400) & 0xFFFF;
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmBulldrome_ChangeState(em, 2, 2, 4);
        }
        break;
    }
    Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
}

void EmBulldrome_State2_02(Em *em)
{
    f32 v[3];
    f32 out[3];
    s32 ang[3];
    f32 mtx[16];
    EmBulldromeWork *w = (EmBulldromeWork *)&em->work;
    s32 d;

    em->stateArmor = 1;
    em->noPushTimer = 2;
    em->guardTimer = 2;
    em->noReact = 1;
    em->hideShadowHold = 2;
    em->hideShadow = 2;
    em->fieldCollide = 0;
    em->noHitTimer = 5;
    em->senseDelay = 5;
    switch (em->step) {
    case 0:
        em->stance = 0;
        em->step++;
        Em_SetMotion(em, 0x18, 0, 0);
        break;
    case 1:
        if (w->unk02B != 0) {
            d = (u16) (Math_AngleXZ(&em->posX, em->goal) - em->rotY);
            if (d <= 0x8000) {
                if (d < 0x40) {
                    em->rotY += d;
                } else {
                    em->rotY += 0x40;
                }
            } else if (d > 0xFFC0) {
                em->rotY += d;
            } else {
                em->rotY -= 0x40;
            }
        }
        Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = 80.0f;
        ang[0] = 0;
        ang[1] = em->rotY;
        ang[2] = 0;
        Mtx_FromAnglesYXZ(ang, mtx);
        Vec_Rotate(out, v, mtx);
        em->posX += out[0];
        em->posZ += out[2];
        w->distLeft -= v[2];
        if (w->distLeft <= 0.0f) {
            em->step++;
            EmBulldrome_ChangeState(em, 2, 3, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmBulldrome_ChangeState(em, 2, 3, 4);
        }
        break;
    }
}

void EmBulldrome_State2_03(Em *em)
{
    em->stateArmor = 1;
    em->noPushTimer = 2;
    em->guardTimer = 2;
    em->noReact = 1;
    em->hideShadowHold = 2;
    em->hideShadow = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x17, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmBulldrome_EndState(em);
        }
        break;
    }
}

void EmBulldrome_RunGroup2(Em *em)
{
    switch (em->state) {
    case 0:
        EmBulldrome_State2_00(em);
        break;
    case 1:
        EmBulldrome_State2_01(em);
        break;
    case 2:
        EmBulldrome_State2_02(em);
        break;
    case 3:
        EmBulldrome_State2_03(em);
        break;
    }
}

void EmBulldrome_State3_00(Em *em, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0xF, 0xA, 0);
        switch (variant) {
        case 0:
            BulldromeAttack_Create(em, 3);
            break;
        case 1:
            BulldromeAttack_Create(em, 4);
            break;
        case 2:
            BulldromeAttack_Create(em, 5);
        }
        break;
    case 1:
        if (Em_StepToGoal(em, 0x40, 0) == 1) {
            em->timer = 1;
        }
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x10, 0xA, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmBulldrome_EndState(em);
        }
        break;
    }
}

void EmBulldrome_State3_06(Em *em)
{
    EmBulldromeWork *w = (EmBulldromeWork *)&em->work;
    u16 d;
    s16 v;

    switch (em->step) {
    case 0:
        em->step++;
        d = Math_AngleXZ(&em->posX, em->goal) - em->rotY;
        if (d < 0x4000) {
            w->unk02C = 0;
        } else if (d < 0x8000) {
            w->unk02C = 2;
        } else if (d < 0xC000) {
            w->unk02C = 3;
        } else {
            w->unk02C = 1;
        }
        Em_SetMotion(em, (w->unk02C & 1) + 0x12, 0, 0);
        BulldromeAttack_Create(em, 1);
        break;
    case 1:
        v = 0xE6;
        if (w->unk02C & 1) {
            v = -0xE6;
        }
        em->rotY += v;
        if (EmMotion_ReachedFrame(em, 0, 60.0f)) {
            em->step++;
        }
        break;
    case 2:
        if (EmMotion_InFrameRange(em, 0, 142.0f, 164.0f)) {
            v = (w->unk02C & 2) == 0 ? 0x35C : 0x92E;
            if (w->unk02C & 1) {
                v = -v;
            }
            em->rotY += v;
        }
        if (em->motion[0].playing == 0) {
            EmBulldrome_EndState(em);
        }
        break;
    }
}

void EmBulldrome_RunGroup3(Em *em)
{
    switch (em->state) {
    case 0:
        EmBulldrome_State3_00(em, 0);
        break;
    case 1:
        EmBulldrome_State3_00(em, 1);
        break;
    case 2:
        EmBulldrome_State3_00(em, 2);
        break;
    case 3:
        EmBulldrome_State3_00(em, 2);
        break;
    case 4:
        EmBulldrome_State3_00(em, 2);
        break;
    case 5:
        EmBulldrome_State3_00(em, 2);
        break;
    case 6:
        EmBulldrome_State3_06(em);
        
    default:
        break;
    }
}

void EmBulldrome_State4_00(Em *em)
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
            EmBulldrome_EndState(em);
        }
        break;
    }
}

void EmBulldrome_State4_01(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        EmBulldrome_SetMotionForSide(em, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 10.0f, 0U) != 0) {
            em->step++;
            EmBulldrome_LaunchFromHit(em);
        }
        break;
    case 2:
        if (!((em->velZ * em->accelZ) < 0.0f)) {
            em->accelZ = 0.0f;
        }
        if (Player_MoveFall((Player *) em) != 0) {
            em->step++;
            em->stance = 0;
            Em_SetMotion(em, 0x3F, 6, 0);
            em->timer = 0x96;
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
            EmBulldrome_EndState(em);
        }
        break;
    }
}

void EmBulldrome_State4_02(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmBulldrome_SetMotionForSide(em, 0xA);
        EmBulldrome_LaunchFromHit(em);
        Em_RestartScript(em);
        break;
    case 1:
        if (!((em->velZ * em->accelZ) < 0.0f)) {
            em->accelZ = 0.0f;
        }
        if (Player_MoveFall((Player *) em) != 0) {
            em->step++;
            em->stance = 0;
            Em_SetMotion(em, 0x3F, 6, 0);
            em->timer = 0x96;
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
            EmBulldrome_EndState(em);
        }
        break;
    }
}

void EmBulldrome_State4_03(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_STATUS);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x47, 0, 0);
        em->rotY = em->hitAngle + 0x7FFF + 1;
        Em_RestartScript(em);
        break;
    case 1:
        Em_SpawnStatus4Effect(em, 2);
        if (--em->timer <= 0) {
            em->step++;
            EmBulldrome_ChangeState(em, 0, 0x10, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmBulldrome_ChangeState(em, 0, 0x10, 4);
        }
        break;
    }
}

void EmBulldrome_State4_04(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x47, 0, 0);
        Em_RestartScript(em);
        em->noReact = 1;
        StageObj_Stun_CreateEm(em, 1, 0, *(s16 *)((u8 *)em + 8), 100.0f, 2.2f);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmBulldrome_ChangeState(em, 0, 0x11, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmBulldrome_ChangeState(em, 0, 0x11, 4);
        }
        break;
    }
}

void EmBulldrome_State4_05(Em *em)
{
    Em_SetHold2(em);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x14, 0, 0);
        Player_SetFlag((Player *) em, EM_FLAG_NO_ROOT_MOTION);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x47, 0, 0);
            Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            EmBulldrome_ChangeState(em, 0, 0xD, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmBulldrome_ChangeState(em, 0, 0xD, 4);
        }
        break;
    }
}

void EmBulldrome_RunGroup4(Em *em)
{
    switch (em->state) {
    case 0:
        EmBulldrome_State4_00(em);
        break;
    case 1:
        EmBulldrome_State4_01(em);
        break;
    case 2:
        EmBulldrome_State4_02(em);
        break;
    case 3:
        EmBulldrome_State4_03(em);
        break;
    case 4:
        EmBulldrome_State4_04(em);
        break;
    case 5:
        EmBulldrome_State4_05(em);
        
    default:
        break;
    }
}

void EmBulldrome_State5_00_Hunted(Em *em)
{
    em->noHitTimer = 10;
    em->noPushTimer = 10;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        EmBulldrome_SetMotionForSide(em, 0);
        QuestEm_Hunted(em);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0) {
            em->step++;
            EmBulldrome_LaunchFromHit(em);
        }
        break;
    case 2:
        if (!(em->velZ * em->accelZ < 0.0f)) {
            em->accelZ = 0.0f;
        }
        if (!(em->velX * em->accelX < 0.0f)) {
            em->accelX = 0.0f;
        }
        if (Player_MoveFall((Player *) em) != 0) {
            em->step++;
            em->stance = 0;
            Em_SetMotion(em, 0x3F, 6, 0);
            Player_ClearVelocity(em);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x40, 0, 0);
            em->timer = 2700;
            EmMine_Create(em, 0);
            em->fieldCollide = 0;
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
            em->step++;
            em->visible = 0;
        }
        break;
    case 6:
        if (em->remote == 0) {
            em->step++;
            EmBulldrome_ChangeState(em, 5, 2, 4);
        }
        break;
    }
}

void EmBulldrome_State5_01_Hunted(Em *em)
{
    em->noHitTimer = 10;
    em->noPushTimer = 10;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        EmBulldrome_SetMotionForSide(em, 10);
        QuestEm_Hunted(em);
        EmBulldrome_LaunchFromHit(em);
        break;
    case 1:
        if (!(em->velZ * em->accelZ < 0.0f)) {
            em->accelZ = 0.0f;
        }
        if (Player_MoveFall((Player *) em) != 0) {
            em->step++;
            em->stance = 0;
            Em_SetMotion(em, 0x3F, 6, 0);
            Player_ClearVelocity(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x40, 0, 0);
            em->timer = 2700;
            EmMine_Create(em, 0);
            em->fieldCollide = 0;
        }
        break;
    case 3:
        EmMine_Update(em);
        if (--em->timer <= 0) {
            em->step++;
            EmMine_Remove(em);
        }
        break;
    case 4:
        em->alpha -= 1.0f / 60.0f;
        if (em->alpha <= 0.0f) {
            em->visible = 0;
            em->step++;
        }
        break;
    case 5:
        if (em->remote == 0) {
            em->step++;
            EmBulldrome_ChangeState(em, 5, 2, 4);
        }
        break;
    }
}

void EmBulldrome_State5_02(Em *em)
{
    switch (em->step) {
    case 0:
        if (QuestEm_CanAppear(em) == 1) {
            EmBulldrome_ChangeState(em, 0, 1, 0);
            break;
        }
        em->taskPhase++;
    case 1:
        break;
    }
}

void EmBulldrome_RunGroup5(Em *em)
{
    switch (em->state) {
    case 0:
        EmBulldrome_State5_00_Hunted(em);
        break;
    case 1:
        EmBulldrome_State5_01_Hunted(em);
        break;
    case 2:
        EmBulldrome_State5_02(em);
        break;
    }
}

void EmBulldrome_State6_00_Captured(Em *em)
{
    f32 pos[3];

    Em_SetCaptured(em, 1);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x47, 0x1E, 0x24);
        em->timer = 150;
        Em_BeginStatus1(em);
        Em_ClearPoison(em);
        em->stance = 0;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            QuestEm_Captured(em);
            *(s32 *) &em->motionSpeed = 0;
        }
        break;
    case 2:
        pos[0] = 0.0f;
        pos[1] = -20.0f;
        pos[2] = 0.0f;
        Em_SpawnLyingPuffs(em, 0xB, pos, 1.0f);
        if (QuestEm_CanAppear(em) != 0) {
            if (--em->timer <= 0) {
                em->step++;
            }
        }
        break;
    case 3:
        em->alpha -= 1.0f / 60.0f;
        if (em->alpha <= 0.0f) {
            em->visible = 0;
            em->step++;
        }
        break;
    case 4:
        if (em->remote == 0) {
            em->step++;
            EmBulldrome_ChangeState(em, 5, 2, 4);
        }
        break;
    }
}

void EmBulldrome_State6_01(Em *em)
{
    em->noHitTimer = 5;
    em->blind = 2;
    em->senseDelay = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 0;
        em->fieldCollide = 0;
        em->posX = 9800.0f;
        em->posY = -69.0f;
        em->posZ = 10336.0f;
        em->goal[0] = 7535.0f;
        em->goal[1] = -59.0f;
        em->goal[2] = 9654.0f;
        em->rotY = Math_AngleXZ(&em->posX, em->goal);
        GmEm_ResetSize(em);
        Em_SetMotion(em, 3, 0, 0);
        em->timer = 300;
        break;
    case 1:
        em->rotY = Math_AngleXZ(&em->posX, em->goal);
        if (--em->timer <= 0 || Vec_DistanceXZ(&em->posX, em->goal) <= 100.0f) {
            em->step++;
            Em_SetMotion(em, 1, 10, 0);
            em->timer = 240;
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x15, 0, 0);
        }
        break;
    case 3:
        em->rotY += 0x26A;
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0xE, 6, 0);
            em->goal[0] = 11900.0f;
            em->goal[1] = -59.0f;
            em->goal[2] = 10300.0f;
        }
        break;
    case 4:
        em->rotY += (Math_AngleXZ(&em->posX, em->goal) - em->rotY) >> 4;
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0xF, 10, 0);
            em->timer = 90;
            em->rotY = Math_AngleXZ(&em->posX, em->goal);
        }
        break;
    case 5:
        if (--em->timer <= 0) {
            em->timer = 0;
            if (Save_TestEventFlag(0x99) == 1) {
                em->senseDelay = 0;
                em->noHitTimer = 0;
                em->fieldCollide = 1;
                EmBulldrome_EndState(em);
            }
        }
        break;
    }
}

void EmBulldrome_RunGroup6(Em *em)
{
    switch (em->state) {
    case 0:
        EmBulldrome_State6_00_Captured(em);
        break;
    case 1:
        EmBulldrome_State6_01(em);
        break;
    }
}

void EmBulldrome_State8_00(Em *em, EmWork *work)
{
    EmBulldromeVec v;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        if (Versus_DefeatCamera(em) == 1) {
            em->step++;
            Em_SetMotion(em, 7, 0, 0);
        }
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0) {
            v = EmBulldrome_CarveThrowPt;
            StageObj_K25_Create(&gPlayers[(u8) em->side - 1], em, 0, 0, &v.x);
        }
        if (em->motion[0].playing == 0) {
            Versus_RouteEmByTeam(em);
        }
        break;
    }
}

void EmBulldrome_State8_01(Em *em)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x14, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 1, 0, 0);
            EmBulldrome_ChangeState(em, 8, 0x98, 1);
        }
        break;
    }
}

void EmBulldrome_State8_02(Em *em)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0xE, 0, 0);
        em->timer = 5;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            if (--em->timer <= 0) {
                em->step++;
                Em_SetMotion(em, 1, 0, 0);
                EmBulldrome_ChangeState(em, 8, 0x98, 1);
            }
        }
        break;
    }
}

void EmBulldrome_State8_03(Em *em)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x11, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 1, 0, 0);
            EmBulldrome_ChangeState(em, 8, 0x98, 1);
        }
        break;
    }
}

void EmBulldrome_State8_98(Em *em)
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
            EmBulldrome_ChangeState(em, 8, 0x99, 1);
        }
        break;
    }
}

void EmBulldrome_State8_99(Em *em)
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
            EmBulldrome_EndState(em);
        }
        break;
    }
}

void EmBulldrome_RunGroup8(Em *em)
{
    u8 *w = &em->work;

    switch (em->state) {
    case 0x0:
        EmBulldrome_State8_00(em, (EmWork *) w);
        break;
    case 0x1:
        EmBulldrome_State8_01(em);
        break;
    case 0x2:
        EmBulldrome_State8_02(em);
        break;
    case 0x3:
        EmBulldrome_State8_03(em);
        break;
    case 0x98:
        EmBulldrome_State8_98(em);
        break;
    case 0x99:
        EmBulldrome_State8_99(em);
        break;
    }
}

void EmBulldrome_Update(Em *em)
{
    u8 buf[3];
    EmBulldromeWork *w = (EmBulldromeWork *)&em->work;

    if (w->unk00C != 0) {
        w->unk00C -= 1;
    }
    Em_UpdateCombatTimer(em);
    Em_RaisePlayerAreaTrigger(em);
    Em_HoldPosNoPlayers(em, 0, 1);
    if (em->noAlert != 1 && em->combat == 1) {
        switch ((u16) Map_DayId(gScene.map)) {
        case MAP_JUNGLE:
        case MAP_SWAMP:
        case MAP_SNOWY_MOUNTAINS:
            Em_CheckLowHp(em, 0.3f);
            break;
        }
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
        break;
    case 1:
    case 2:
        EmBulldrome_ChangeState(em, 5, 0, 3);
        break;
    case 5:
        if (em->stateGroup != 4 || (u32) (em->state - 1) > 1) {
            EmBulldrome_ChangeState(em, 4, 2, 3);
        }
        break;
    case 6:
        if (em->stateGroup != 4 || (u32) (em->state - 1) > 1) {
            Em_RestartScript(em);
            Em_SetStatus4Timer(em);
            EmBulldrome_ChangeState(em, 4, 3, 3);
        }
        break;
    case 7:
        EmBulldrome_ChangeState(em, 6, 0, 3);
        break;
    case 8:
        if (em->stateGroup != 4 || (u32) (em->state - 1) > 1) {
            Em_SetStatus2Timer(em);
            EmBulldrome_ChangeState(em, 0, 0xB, 3);
        }
        break;
    case 10:
        switch (em->state) {
        case 0xE:
            EmBulldrome_ChangeState(em, 0, 0xF, 3);
            break;
        default:
            Em_RestartScript(em);
            EmBulldrome_ChangeState(em, 0, 0xC, 3);
            break;
        }
        break;
    case 12:
        if (em->parts[0].state & 3) {
            EmBulldrome_ChangeState(em, 4, 0, 3);
        } else {
            EmBulldrome_ChangeState(em, 4, 1, 3);
        }
        break;
    case 13:
        EmBulldrome_ChangeState(em, 4, 1, 3);
        break;
    case 17:
        if (em->stateGroup != 4 || (em->state != 4 && em->state != 2)) {
            Em_SetStatus5Timer(em);
            EmBulldrome_ChangeState(em, 4, 4, 3);
        }
        break;
    case 18:
        Em_ResetKindTimer(em);
        EmBulldrome_ChangeState(em, 4, 5, 3);
        break;
    }
    if ((gQuest.stage == 0x3EC || gQuest.stage == 0x3ED) && Area_DayId(em->area) == AREA_JUNGLE_5 && Save_TestEventFlag(0x99) == 0) {
        if (ScEvent_CurId() == 0x99 && em->stateGroup != 6) {
            EmBulldrome_ChangeState(em, 6, 1, 1);
        }
    } else {
        Em_UpdatePendingScript(em);
    }
    Em_ClearHitReactMode(em);
    EmBulldrome_RunState(em);
    if (em->stateRequest != 0) {
        EmBulldrome_RunState(em);
        em->stateRequest = 0;
    }
}

void EmBulldrome_RunState(Em *em)
{
    switch (em->stateGroup) {
    case 0:
        EmBulldrome_RunGroup0(em);
        break;
    case 1:
        EmBulldrome_RunGroup1(em);
        break;
    case 2:
        EmBulldrome_RunGroup2(em);
        break;
    case 3:
        EmBulldrome_RunGroup3(em);
        break;
    case 4:
        EmBulldrome_RunGroup4(em);
        break;
    case 5:
        EmBulldrome_RunGroup5(em);
        break;
    case 6:
        EmBulldrome_RunGroup6(em);
        break;
    case 7:
        EmBulldrome_RunGroup4(em);
        break;
    case 8:
        EmBulldrome_RunGroup8(em);
        
    default:
        break;
    }
}

/* em_chameleos's .data (0x8CAB00-0x8CB4E0) */
EmFlyParam EmChameleos_FlyParam = {
#include "game/008CAB00.inc"
};
EmFlyParam EmChameleos_FlyParam2 = {
#include "game/008CAB60.inc"
};
EmTurnParams EmChameleos_TurnParams[1] = {
#include "game/008CABC0.inc"
};
void *EmChameleos_Callbacks[8] = {
    EmChameleos_SpawnEffects, EmChameleos_SpawnEffects, EmChameleos_TableNop, EmChameleos_UpdateMotionEffects,
    EmChameleos_ScriptQuery, EmChameleos_Reinit, EmChameleos_AllowHit8, NULL,
};
EmCurveKey EmRemobra_VelYCurves_0[35] = {
#include "game/008CAC00.inc"
};
EmCurveKey *EmRemobra_VelYCurves[2] = {
    EmRemobra_VelYCurves_0, NULL,
};
EmCurveKey EmRemobra_SpeedCurves_0[35] = {
#include "game/008CAD20.inc"
};
EmCurveKey *EmRemobra_SpeedCurves[2] = {
    EmRemobra_SpeedCurves_0, NULL,
};
f32 EmRemobra_HopFrames[3] = {
#include "game/008CAE40.inc"
};
s32 GmEm_ReturnZero(void); /* gm_sub */
void GmEm_UpdateSfx(Em *em); /* gm_sub */
void *EmRemobra_Callbacks[8] = {
    EmRemobra_SpawnEffect01, EmRemobra_SpawnEffect01, EmRemobra_TableNop, EmRemobra_UpdateMotionEffects,
    GmEm_ReturnZero, EmRemobra_Reinit, NULL, NULL,
};
u32 gEmMotionRows_63[304] = {
#include "game/008CAE70.inc"
};
EmCurveKey EmChameleos_VelYCurves_0[6] = {
#include "game/008CB330.inc"
};
EmCurveKey EmChameleos_VelYCurves_1[10] = {
#include "game/008CB360.inc"
};
EmCurveKey EmChameleos_VelYCurves_2[5] = {
#include "game/008CB3B0.inc"
};
EmCurveKey *EmChameleos_VelYCurves[3] = {
    EmChameleos_VelYCurves_0, EmChameleos_VelYCurves_1, EmChameleos_VelYCurves_2,
};
EmCurveKey EmChameleos_SpeedCurves_1[10] = {
#include "game/008CB3F0.inc"
};
EmCurveKey EmChameleos_SpeedCurves_2[5] = {
#include "game/008CB440.inc"
};
EmCurveKey *EmChameleos_SpeedCurves[3] = {
    NULL, EmChameleos_SpeedCurves_1, EmChameleos_SpeedCurves_2,
};
EmTurnParams EmBulldrome_TurnParams[1] = {
#include "game/008CB480.inc"
};
f32 EmBulldrome_Effect0BOfs[3] = {
#include "game/008CB4A0.inc"
};
EmBulldromeVec EmBulldrome_CarveThrowPt = {
#include "game/008CB4B0.inc"
};
void EmBulldrome_SpawnEffect01(u8 *em);
void EmBulldrome_TableNop(void);
void *EmBulldrome_Callbacks[8] = {
    EmBulldrome_SpawnEffect01, EmBulldrome_SpawnEffect01, EmBulldrome_TableNop, GmEm_UpdateSfx,
    GmEm_ReturnZero, EmBulldrome_Reinit, NULL, NULL,
};
