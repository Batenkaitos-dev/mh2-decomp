/*
 * Monster family: Yian Kut-Ku, Gypceros and their subspecies (kinds 6, 0x26, 0x14, 0x27).
 * The code from the family's setup function (0x777C30, called by the per-kind
 * dispatch Em_TypeSetup) to the next family's. The state functions take
 * the monster and the work area its state dispatcher passes on (EmWork);
 * Em.step is the step within the current state. See include/em.h.
 */
#include "common.h"
#include "scene.h"
#include "quest.h"
#include "mine_point.h"
#include "em.h"
#include "objattach.h"
#include "player.h"
#include "item.h"
/* prototypes of callees (round 13) */
void GmEm_ResetSize(Em *em);
void EffKutku17_Create2(Em *em, s8 kind);
void Em_EndStatus4(Em *em);
void Em_EndStatus5(Em *em);
void Em_StartGoalAction(Em *em, s32 arg1);
void Em_ResetNotice(Em *em);
void Em_BeginStatus2(Em *em);
void Em_RefillNeed3(Em *em);
void Em_EndStatus2(Em *em);
void Em_EndStatus1(Em *em);
void Em_BeginStatus4(Em *em);
void Em_EndAllStatus(Em *em);
void Em_BeginStatus5(Em *em);

/* A mining point (as in sub_main's mine_point.c). */
/* prototypes */
void EmKutku_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void Effect0D_SpawnForEmScaled(Em *em, s32 a, s32 b, f32 scale);
s32 EmMotion_InFrameRange(void *em, s32 layer, f32 from, f32 to);
u16 Rand_Next(s32 stream);
void EffEm08_CreateAtBone(Em *, s32, s32, s32, s32, s32);
s8 EmMine_CreateTimed0(Em *em, f32 *pos);
/* unit's own declaration: with the definition's void (Em *em, s16 n) (game/unk_708850.c), EmKutku_State0_0E: 46 of 234 words differ, EmKutku_State0_1A: 40 of 84 words differ */
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
void EffKutku17_Create(Em *, s8);
void Em_SelectSightRow(Em *em, u8 idx);
void Em_MoveRotated(Em *em, void *src);
void Em_MoveRotatedAccel(Em *em, void *ang);
void Em_SetStateRand(Em *em, s32 arg1, s32 arg2);
void Em_LoadReactionSet(Em *em, u8 k);
void Em_ComputeGoal2(Em *em, f32 *target);
void Em_GetPitchToGoal(Em *em, s32 *out, f32 a, f32 b);
s32 Em_GetTargetPos(Em *em, u8 idx, f32 *out);
void Em_ClearLowHpNeed(Em *em);
void Em_SpawnEffect06Bone(Em *em, s32 a);
void Em_SetHitReactMode(Em *em, u8 a);
void Em_SetGoal12(Em *em);
/* unit's own declaration: with the definition's s32 (Em *em, s16 n, s16 max, s16 interval) (game/unk_708850.c), EmKutku_State0_12: 66 of 130 words differ, EmKutku_State0_15: 64 of 128 words differ */
s32 Em_RegenHp(Em *em, s32 n, s32 max, s32 interval);
void Em_ClearPoison(Em *em);
void Em_SetHold2(Em *em);
void Em_SetCaptured(Em *em, s32 a);
void Em_PlaySeAtBoneOnFrame2(Em *em, s32 frame, s32 se, s32 bone, s32 flag);
void Em_UpdateFlight(Em *em, s32 flags, EmFlyParam *p);
s32 Em_TurnToTargetMotion(Em *em, u16 range, u16 motFront, u16 motLeft, u16 motRight);
s32 Em_StepToGoal(Em *em, u16 step, u8 flags);
u8 Em_TurnToGoal(Em *em, u16 step, u8 flags);
void Em_StartTurnToTarget(Em *em, EmTurnParams *t);
s32 Em_TurnStep(Em *em, EmTurnParams *t);
void Em_RestartScript(Em *em);
void EmKutku_Init(Em *);
void EmKutku_State2_01_FlyInLand(Em *em, EmWork *work);
void EmKutku_State2_08(Em *, EmWork *);
void EmKutku_State2_09(Em *, EmWork *);
void EmKutku_State2_0D(Em *em, EmWork *work);
void EmKutku_State4_07(Em *em, EmWork *work);
void EmKutku_State4_08(Em *em, EmWork *work);
void EmKutku_State4_14(Em *, EmWork *);
void EmKutku_State4_19_FeignDeath(Em *, EmWork *);
void EmKutku_State6_00(Em *em, EmWork *work);
void EmKutku_State6_04_Captured(Em *em);
void EmKutku_State8_00_CarveStart(Em *, EmWork *);
void EmKutku_State8_01(Em *, EmWork *);
void EmKutku_State8_02(Em *, EmWork *);
void EmKutku_State8_03(Em *em, EmWork *work);
void EmKutku_State5_00_Hunted(Em *em, EmWork *work);
void EmKutku_State5_01_Hunted(Em *em, EmWork *work);
void EmKutku_State5_02_HuntedFalling(Em *em, EmWork *work);
void EmKutku_State5_03_Appear(Em *, EmWork *);
void EmKutku_RunGroup0(Em *em, EmWork *work);
void EmKutku_RunGroup1(Em *em, EmWork *work);
void EmKutku_RunGroup3(Em *em, EmWork *work);
/* The view of the work area the motion-effects function takes: the current
   motion at +2 (a real field, so MWCC does not keep its address). */
typedef struct EmKutkuFxWork {
    s16 unk000;
    s16 motion;
    u8 unk004[0x1B - 0x4];
    u8 unk01B;
} EmKutkuFxWork;
void EmKutku_MotionSounds(Em *em, struct EmWork *);
void EmKutku_MotionEffects(Em *, EmKutkuFxWork *);
void EmKutku_SpawnK14Type0B(Em *em);
void EmKutku_SpawnK14Type0AEvery5(Em *em);
void EmKutku_State3_End(Em *em, EmWork *work);
void EmKutku_MoveByVelYCurve(Em *em, s32 kind);
void EmKutku_StartCurves(Em *em, u8 kind);
u8 EmKutku_UpdateCurves(Em *em);
void StageObj_K13_Create(void *owner, s16 v, u8 k);
void StageObj_K14_Create(Em *em, s16 k, s16 c, f32 scale);
void Mtx_GetTrans(f32 *out, f32 *m);
void StageObj_K0E_CreateOwned(f32 *pos, s16 k, f32 scale, void *owner);
/* unit's own declaration: with the definition's void (Em *em, u8 k, s16 v, s16 t, f32 y, f32 scale) (game/unk_874B90.c), EmKutku_State4_15: 66 of 98 words differ, EmKutku_State4_16: 43 of 74 words differ */
void StageObj_Stun_CreateEm(Em *, s32, s32, s32, f32, f32);
s32 Sound_RandomSe(s32 id, s16 lo, s32 alt, s16 n);
s32 Player_PassedFrameScaled(void *p, f32 frame, s32 layer);
s32 EmMotion_ReachedFrame(void *obj, s32 a, f32 frame);
f32 Vec_Distance(f32 *a, f32 *b);
void CamShake_ReqAIf(Em *obj, s32 i);
f32 Vec_DistanceXZ(f32 *a, f32 *b);
s32 Versus_BothDefeated(void);
s32 QuestEm_CanAppear(Em *em);
void Player_ClearVelocity(void *);
void Player_ClearAccel2(void *);
void Motion_InitTime(void *p, s16 step);
void Effect01_Spawn(void *obj, s16 type);
extern EmFlyParam EmKutku_FlyParam;
extern EmFlyParam EmKutku_FlyParam2;
extern EmTurnParams EmKutku_TurnParams[2];
s32 Save_TestEventFlag(s32);
/* unit's own declaration: with the definition's void (Em *em, s32 group, s32 state, u8 mode) (game/unk_701810.c), EmKutku_ChangeState: 4 of 148 words differ, EmKutku_SetGroup0State: 1 of 126 words differ ... */
void Em_SetStateSync(Em *em, u16 group, u16 state, u8 mode);
void Em_SetSpeedScaled(Em *em, u16 flags, f32 x, f32 y);
void Em_ComputeGoal(Em *em, f32 *target);
void Em_TypeSetup(Em *em);
void Em_SetTargetAngle(Em *em);
void Em_StartRage(Em *em);
void Em_StartPendingRage(Em *em);
s32 Game_IsOnline(void);
void Vec_Set(f32 *out, f32 x, f32 y, f32 z);
void EmKutku_SetGroup0State(Em *em, u16 state, u8 mode);
void EmKutku_SetGroup1State(Em *em, u16 state, u8 mode);
void EmKutku_SetGroup2State(Em *em, u16 state, u8 mode);
void EmKutku_SetGroup3State(Em *em, u16 state, u8 mode);
void EmKutku_SetGroup4State(Em *em, u16 state, u8 mode);
u16 Area_BaseId(u16);
void EmKutku_SetEscapePoint(Em *em);
void Em_TargetPlayerInRange(Em *);
s16 Player_IsState(void *obj, u8 state, u8 sub);
void Versus_MarkDefeat(Em *);
s8 QuestEm_Appear(Em *em);
void StageObj_K25_Create(Player *p, Em *em, u8 k, s32 unused, f32 *pos);
void Versus_RouteEmByTeam(Em *em);
s32 Versus_DefeatCamera(Em *pl);
typedef struct EmKutkuVec {
    f32 x, y, z;
} EmKutkuVec;
extern EmKutkuVec EmKutku_CarveThrowPt __attribute__((aligned(8)));
extern EmCurveKey *EmKutku_MoveVelYCurves[];
void Em_SpawnEffect06Periodic(Em *em, s32 bone, f32 *pos, f32 scale);
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
u16 Math_Heading(f32 x1, f32 z1, f32 x2, f32 z2);
void Em_BonePos(void *obj, s16 bone, f32 *out);
void EmKutku_AddMinePoint(Em *em);
EmBone *Em_Bone(void *obj, s16 bone);
void Mtx_Copy(f32 *dst, f32 *src);
void Vec_RotateSelf(f32 *v, f32 *m);
void Effect02_SpawnForOwner(void *owner, s32 ang, s32 type, s32 sub, f32 *pos, f32 scale);
u16 Area_IdForCurrentTime(u16);
extern f32 gAreaGroundY[];
extern u16 EmKutku_FxFrameBone[4][2];
s32 Quest_IsRunning(s32 ending);
s32 Player_HasSkill(Player *p, u8 skill);
u8 Pouch_Add(Player *p, u16 item, s16 n);
u8 Notice_Push(u8 kind, s16 a, s16 b);
extern u16 EmKutku_StealItems[];
/* A mesh of the model (0x4C bytes; the list at model +0x10). */
/* A set of meshes (0x8C bytes). */
typedef struct EmKutkuMeshSet {
    s32 unk0;
    s32 count;          /* 0x04 */
    s32 mesh[33];       /* 0x08 */
} EmKutkuMeshSet;
s32 Gs_SetState(u8 reg, u32 v);
u16 Area_DayId(u16 id);
void StageObj_K09_Create(Em *);
void Em_SetPartScaled(Em *em, s32 bit);
void Em_SetMotionRow(Em *em, s32 row, s32 arg2, s32 arg3);
s32 Em_ComputeSize(Em *em, s16 idx);
void Em_CheckCarryRequest(Em *em);
void EmKutku_RunState(Em *em, s32 arg1);
s32 func_00786E50(Em *em);
s32 Em_UpdateCombatTimer(Em *em);
void Em_RaiseTrigger80Lax(Em *em);
void Em_CheckLowHp(Em *em, f32 ratio);
void Em_DecayNeed2(Em *em);
void Em_DecayNeed1Only(Em *em);
void Em_DecayNeed1(Em *em);
u8 Em_UpdateDamage(Em *em, u8 *out);
void Em_SetGoal12Dist(Em *em, u8 *p);
void Em_SetStatus1Timer(Em *em);
void Em_SetStatus2Timer(Em *em);
void Em_SetStatus4Timer(Em *em);
void Em_SetStatus5Timer(Em *em);
void Em_ResetKindTimer(Em *em);
s32 Em_IsHeld(Em *em);
void Em_CheckRageByHp(Em *em, s16 n);
void Em_BreakPart(Em *em, s32 part);
void Em_UpdatePendingScript(Em *em);
void Em_ClearHitReactMode(Em *em);
u8 Em_AllPlayersHere(Em *em);
void Area_ToWorld(s32 area, f32 *pos, f32 *out);
void Em_GetPitchToTarget(Em *em, s32 *out, f32 a, f32 b);
void EmKutku_EndSwoop(Em *em, s32 arg1);
void EmKutku_StepPitch(Em *em, s32 mode, f32 height);
s32 EmKutku_ReachedTarget(Em *em);
/* end of prototypes */

void EmKutku_Setup(Em *em)
{
}

void EmKutku_SpawnEffects(void *em)
{
    StageObj_K13_Create(em, 0x22, 0);
    Effect01_Spawn((Player *) em, 0);
}

void EmKutku_Init(Em *em)
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
        case 0x21:
            em->posX = 17300.0f;
            em->posY = 0.0f;
            em->posZ = 18000.0f;
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
    w->partWhole = 1;
    w->unk008 = 0;
    w->minePoint = -1;
    hp = Em_ComputeSize(em, 0);
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
    w->moveAngBX = 0;
    if ((em->fxFlags &= 1) == 0) {
        switch (em->type) {
        case 6:
        case 0x26:
            em->meshShown[1] = 0;
            StageObj_K09_Create(em);
            break;
        case 0x14:
        case 0x27:
            break;
        }
    }
}

void EmKutku_Start(Em *em)
{
    EmKutku_Init(em);
    EmKutku_ChangeState(em, 0, 1, 0);
}

void EmKutku_Reinit(Em *em)
{
    EmKutku_Init(em);
}

void EmKutku_ToIdle(Em *em)
{
    em->scriptPending = 1;
    if (em->stance == 0) {
        EmKutku_ChangeState(em, 0, 1, 0);
        return;
    }
    EmKutku_ChangeState(em, 2, 2, 0);
}

void EmKutku_EndState(Em *em, s32 unused1, s32 unused2)
{
    EmKutku_ToIdle(em);
}

void EmKutku_EndStateOrState3_1E(Em *em, s32 unused1, s32 unused2)
{
    u8 type;

    if (((s32) em->stateRand % 100) < 0xA) {
        EmKutku_EndState(em, 0, 0);
        em->scriptPending = 0;
        type = em->type;
        if ((type == 0x14) || (type == 0x27)) {
            EmKutku_ChangeState(em, 3, 0x1E, 1);
            return;
        }
        if (em->rage != 0) {
            EmKutku_ChangeState(em, 3, 0x1F, 1);
            return;
        }
        EmKutku_ChangeState(em, 3, 0x1E, 1);
        return;
    }
    EmKutku_EndState(em, unused1, unused2);
}

void EmKutku_EndState2(Em *em, u8 next)
{
    s32 k;

    k = next & 0xFF;
    em->stance = 2;
    em->scriptPending = 1;
    em->motionSpeed = 1.0f;
    switch (k) {
    case 0:
        EmKutku_ChangeState(em, 2, 0xE, 0);
        break;
    case 1:
        EmKutku_ChangeState(em, 2, 0xF, 0);
        break;
    }
}

/* The carcass' mine point: a gathering point (table 0x175, 3 gathers) at bone 2 on the ground. */
void EmKutku_AddMinePoint(Em *em)
{
    f32 pos[3];
    MinePoint pt;

    pt.radius = 333.0f;
    pt.table = 0x175;
    pt.count = 3;
    pt.unk14 = 2;
    pt.area = (u8) em->area;
    pt.flags = 1;
    pt.timer = 0;
    Em_BonePos(em, 2, pos);
    pos[1] = em->floorY;
    em->minePoint = MinePoint_AddUntimed(&pt, pos, 0);
}

/* Gypceros steals: one item of the local player's pouch, from the list EmKutku_StealItems if the
 * pouch holds any of them, otherwise any item of types 0-3 whose type byte is 0 (skill 0x5C
 * protects; not while quest phase 1 runs). */
void EmKutku_StealItem(Em *em, Player *p)
{
    s32 idx[20];
    s32 n;
    s32 i;
    s32 j;
    ItemData *d;
    s16 item;

    if (p->index != gScene.localPlayer) {
        return;
    }
    if (Quest_IsRunning(1) != 0) {
        return;
    }
    if (Player_HasSkill(p, 0x5C) == 1) {
        return;
    }
    n = 0;
    for (i = 0; i < 20; i++) {
        if (p->pouch[i].item != 0) {
            for (j = 0; j < 9; j++) {
                if (p->pouch[i].item == EmKutku_StealItems[j]) {
                    idx[n] = i;
                    n++;
                }
            }
        }
    }
    if (n == 0) {
        for (i = 0; i < 20; i++) {
            if (p->pouch[i].item != 0) {
                d = &gItemData[p->pouch[i].item];
                if (d->type == 0 && d->rarity < 4) {
                    idx[n] = i;
                    n++;
                }
            }
        }
        if (n == 0) {
            return;
        }
    }
    item = p->pouch[idx[em->stateRand % n]].item;
    Pouch_Add(p, item, -1);
    if (p->index == gScene.localPlayer) {
        Notice_Push(1, 6, item);
    }
}

void EmKutku_RestartLayerMotion(Em *em, s32 layer)
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

void EmKutku_State0_00(Em *em)
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

void EmKutku_State0_01(Em *em)
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
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKutku_State0_02(Em *em)
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
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKutku_State0_03(Em *em)
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
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKutku_State0_04(Em *em)
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
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
    EmKutku_RestartLayerMotion(em, 1);
    EmKutku_RestartLayerMotion(em, 2);
}

void EmKutku_State0_05(Em *em)
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
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
    EmKutku_RestartLayerMotion(em, 0);
    EmKutku_RestartLayerMotion(em, 2);
    Em_PlaySeAtBoneOnFrame2(em, 0x20, 0x2F, 0x23, 1);
}

void EmKutku_State0_06(Em *em)
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
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
    EmKutku_RestartLayerMotion(em, 0);
    EmKutku_RestartLayerMotion(em, 2);
    Em_PlaySeAtBoneOnFrame2(em, 0x20, 0x20, 0x23, 1);
    Em_PlaySeAtBoneOnFrame2(em, 0xA0, 0x17, 0x23, 1);
}

void EmKutku_State0_07(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x19, 0, 0);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
    EmKutku_RestartLayerMotion(em, 0);
    EmKutku_RestartLayerMotion(em, 2);
}

void EmKutku_State0_09(Em *em)
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
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKutku_State0_0A(Em *em)
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
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKutku_State0_0B(Em *em)
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
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
    EmKutku_RestartLayerMotion(em, 0);
    EmKutku_RestartLayerMotion(em, 2);
    Em_PlaySeAtBoneOnFrame2(em, 0x38, 0x21, 0x23, 1);
    Em_PlaySeAtBoneOnFrame2(em, 0x68, 0x1F, 0x23, 1);
}

void EmKutku_State0_0C(Em *em)
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
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKutku_State0_0D(Em *em)
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
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
    EmKutku_RestartLayerMotion(em, 0);
    EmKutku_RestartLayerMotion(em, 2);
    Em_PlaySeAtBoneOnFrame2(em, 2, 0x20, 0x23, 1);
    Em_PlaySeAtBoneOnFrame2(em, 0x4A, 0x1F, 0x23, 1);
}

void EmKutku_State0_0E(Em *em)
{
    s32 need2Max;

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
            em->hitReactMode = 0;
            Em_SelectSightRow(em, 0);
            Em_LoadReactionSet(em, 0);
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
    em->need2Gauge += 0xDE;
    need2Max = em->need2Max;
    if (need2Max < em->need2Gauge) {
        em->need2Gauge = need2Max;
    }
}

void EmKutku_State0_0F(Em *em)
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
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKutku_State0_10(Em *em)
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
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKutku_State0_11(Em *em)
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
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKutku_State0_12(Em *em)
{
    u8 type;
    u8 emType;

    Em_SetHitReactMode(em, 2U);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x20, 0, 0);
        Em_LoadReactionSet(em, 1);
        type = em->type;
        if ((type == 6) || (type == 0x26)) {
            em->timer = 0x1518;
            break;
        }
        em->timer = 0x2328;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x1F, 0, 0);
        }
        break;
    case 2:
        emType = em->type;
        if ((emType == 6) || (emType == 0x26)) {
            Em_RegenHp(em, 1, (s32) (0.35f * (f32) em->hpMax), 9);
        } else {
            Em_RegenHp(em, 1, (s32) (0.4f * (f32) em->hpMax), 5);
        }
        if (--em->timer <= 0) {
            em->step++;
            Em_ClearLowHpNeed(em);
            EmKutku_ChangeState(em, 0, 0x17, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmKutku_ChangeState(em, 0, 0x17, 4);
        }
        break;
    }
}

void EmKutku_State0_13(Em *em)
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
            EmKutku_ChangeState(em, 0, 0x28, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmKutku_ChangeState(em, 0, 0x28, 4);
        }
        break;
    }
}

void EmKutku_State0_28(Em *em)
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
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKutku_State0_14(Em *em)
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
            EmKutku_ChangeState(em, 0, 0x18, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmKutku_ChangeState(em, 0, 0x18, 4);
        }
        break;
    }
}

void EmKutku_State0_15(Em *em)
{
    u8 type;
    u8 emType;

    Em_SetHitReactMode(em, 2U);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x27, 0, 0);
        Em_LoadReactionSet(em, 1);
        type = em->type;
        if ((type == 6) || (type == 0x26)) {
            em->timer = 0x1518;
            break;
        }
        em->timer = 0x2328;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x22, 0, 0);
        }
        break;
    case 2:
        emType = em->type;
        if ((emType == 6) || (emType == 0x26)) {
            Em_RegenHp(em, 1, (s32) (0.35f * (f32) em->hpMax), 9);
        } else {
            Em_RegenHp(em, 1, (s32) (0.4f * (f32) em->hpMax), 5);
        }
        if (--em->timer <= 0) {
            em->step++;
            EmKutku_ChangeState(em, 0, 0x18, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmKutku_ChangeState(em, 0, 0x18, 4);
        }
        break;
    }
}

void EmKutku_State0_16(Em *em)
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
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKutku_State0_17(Em *em)
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
            EmKutku_ChangeState(em, 0, 0x19, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmKutku_ChangeState(em, 0, 0x19, 4);
        }
        break;
    }
}

void EmKutku_State0_18(Em *em)
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
            EmKutku_ChangeState(em, 0, 0x19, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmKutku_ChangeState(em, 0, 0x19, 4);
        }
        break;
    }
}

void EmKutku_State0_19(Em *em)
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
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKutku_State0_1A(Em *em)
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
            em->need1Gauge = em->need1Max;
            Em_AddHp(em, (s32) (0.05f * (f32) em->hpMax));
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->hitReactMode = 0;
            Em_SelectSightRow(em, 0);
            Em_LoadReactionSet(em, 0);
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKutku_State0_1B(Em *em)
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
            EmKutku_ChangeState(em, 0, 0x1C, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmKutku_ChangeState(em, 0, 0x1C, 4);
        }
        break;
    }
}

void EmKutku_State0_1C(Em *em)
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
            EmKutku_ChangeState(em, 0, 0x19, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmKutku_ChangeState(em, 0, 0x19, 4);
        }
        break;
    }
}

void EmKutku_State0_1D(Em *em, EmWork *work)
{
    f32 pos[3];

    em->holdTimer = 5;
    Em_SetHitReactMode(em, 2);
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
            EmKutku_ChangeState(em, 4, 0x12, 3);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmKutku_ChangeState(em, 4, 0x12, 3);
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

void EmKutku_State0_1F(Em *em, EmWork *work)
{
    f32 pos[3];

    em->holdTimer = 5;
    Em_SetHitReactMode(em, 2);
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
            EmKutku_ChangeState(em, 4, 0x13, 3);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmKutku_ChangeState(em, 4, 0x13, 3);
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

void EmKutku_State0_21(Em *em)
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
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKutku_State0_08(Em *em, EmWork *work, s32 variant)
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
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKutku_State1_00(Em *em)
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
                EmKutku_EndState(em, 0, 0);
                break;
            }
        } else {
            if (--em->timer <= 0) {
                em->step++;
                EmKutku_EndState(em, 0, 0);
            }
        }
        break;
    }
}

void EmKutku_State1_01(Em *em)
{
    em->step++;
    EmKutku_EndState(em, 0, 0);
}

void EmKutku_State1_02(Em *em)
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
                EmKutku_EndState(em, 0, 0);
                break;
            }
        } else {
            if (--em->timer <= 0) {
                em->step++;
                EmKutku_EndState(em, 0, 0);
            }
        }
        break;
    }
}

void EmKutku_State1_03(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_StartTurnToTarget(em, &EmKutku_TurnParams[0]);
        break;
    case 1:
        if (Em_TurnStep(em, &EmKutku_TurnParams[0]) == 1) {
            em->step++;
            Player_ClearFlag(em, 0x20000);
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKutku_State1_04(Em *em)
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
                EmKutku_EndState(em, 0, 0);
                break;
            }
        } else {
            if (--em->timer <= 0) {
                em->step++;
                EmKutku_EndState(em, 0, 0);
            }
        }
        break;
    }
}

void EmKutku_State1_05(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_StartTurnToTarget(em, &EmKutku_TurnParams[1]);
        break;
    case 1:
        if (Em_TurnStep(em, &EmKutku_TurnParams[1]) == 1) {
            em->step++;
            Player_ClearFlag(em, 0x20000);
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKutku_State1_06(Em *em)
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
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKutku_State1_07(Em *em, EmWork *work)
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
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKutku_State1_09(void)
{
}

void EmKutku_State1_0A(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x68, 0, 0);
        break;
    case 1:
        if (Em_StepToGoal(em, 0x40, 1) == 1) {
            em->timer = 1;
        }
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x65, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKutku_State2_00(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x12, 0, 0);
        EmKutku_StartCurves(em, 2);
        work->flyGuard = 0;
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 38.0f, 0U) != 0) {
            em->step++;
            em->stance = 2;
            EmKutku_UpdateCurves(em);
        }
        break;
    case 2:
        if (EmKutku_UpdateCurves(em)) {
            em->step++;
            EmKutku_EndState2(em, 0U);
        }
        break;
    }
}

void EmKutku_State2_01_FlyInLand(Em *em, EmWork *work)
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
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
    h = em->floorY;
    if (em->posY < h) {
        em->posY = h;
    }
}

void EmKutku_State2_02(Em *em, EmWork *work)
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
        EmKutku_MoveByVelYCurve(em, 1U);
        if (--em->timer <= 0) {
            em->step++;
            EmKutku_ChangeState(em, 2, 1, 1);
        }
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 1);
        if (em->timer == 0x12C) {
            EmKutku_EndState2(em, 0U);
        }
        break;
    case 2:
        EmKutku_MoveByVelYCurve(em, 1U);
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 1);
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmKutku_State2_03(Em *em, EmWork *work)
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
        EmKutku_StartCurves(em, 2);
        work->flyGuard = 0;
        work->turnSpeed = 0x100;
        break;
    case 1:
        if ((EmMotion_ReachedFrame(em, 0, 50.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 114.0f) == 0)) {
            Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 1);
        }
        if (EmMotion_ReachedFrame(em, 0, 30.0f) != 0) {
            em->stance = 2;
            EmKutku_UpdateCurves(em);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
        }
        break;
    case 2:
        if ((EmKutku_UpdateCurves(em) & 0xFF) && (em->posY <= (em->floorY + (800.0f * em->scaleY)))) {
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
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmKutku_State2_04(Em *em, EmWork *work)
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
        EmKutku_StartCurves(em, 3);
        em->velZ = 20.0f;
        work->flyGuard = 0;
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 30.0f) != 0) {
            em->stance = 2;
            ended = EmKutku_UpdateCurves(em) & 0xFF;
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
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmKutku_State2_05(Em *em, EmWork *work)
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
        EmKutku_StartCurves(em, 3);
        em->velZ = -20.0f;
        work->flyGuard = 0;
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 30.0f) != 0) {
            em->stance = 2;
            ended = EmKutku_UpdateCurves(em) & 0xFF;
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
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmKutku_State2_06(Em *em, EmWork *work)
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
                EmKutku_EndState2(em, 1U);
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
        Em_UpdateFlight(em, 0xE1, &EmKutku_FlyParam);
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

void EmKutku_State2_07(Em *em, EmWork *work)
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
        EmKutku_MoveByVelYCurve(em, 1U);
        dist = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer > 0) {
            if (dist <= (10.0f * em->velZ)) {
                goto block_9;
            }
        } else {
block_9:
            em->step++;
            Player_ClearVelocity(em);
            EmKutku_EndState2(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmKutku_State2_08(Em *em, EmWork *work)
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
        Em_UpdateFlight(em, 0x4E1, &EmKutku_FlyParam);
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
            EmKutku_ChangeState(em, 2, 9, 1);
        } else {
            if (em->remote == 0) {
                em->nextArea = next;
                em->goalNo = next;
                em->goalKind = 3;
                Em_ComputeGoal(em, em->goal);
            }
            EmKutku_ChangeState(em, 2, 0xD, 1);
            Hud_TriggerGlow(em);
        }
        break;
    case 5:
        if (em->remote == 0) {
            if (--em->timer <= 0) {
                em->step = 4;
            }
        }
        break;
    }
    if (em->posY < em->floorY) {
        em->posY = em->floorY;
    }
}

void EmKutku_State2_09(Em *em, EmWork *work)
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
        *(u16 *) &em->destArea = 0xFFFF;
        work->flyGuard = 1;
        break;
    case 1:
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 0xE5, &EmKutku_FlyParam);
        work->moveAngX = em->rotX;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
        d = Vec_DistanceXZ(&em->posX, em->goal);
        em->timer--;
        if (d <= 1000.0f || em->timer < 0) {
            em->step++;
            em->timer = 600;
            EmKutku_ChangeState(em, 2, 1, 1);
        }
        break;
    case 2:
        if (em->remote == 0) {
            if (--em->timer <= 0) {
                em->timer = 600;
                EmKutku_ChangeState(em, 2, 1, 1);
            }
        }
        break;
    }
}

void EmKutku_State2_0A(Em *em)
{
    EmKutku_EndState2(em, 0x1);
}

void EmKutku_State2_0B(Em *em, EmWork *work)
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
            EmKutku_ChangeState(em, 2, 1, 1);
        } else {
            em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
            em->aimAngle = em->aimAngle - em->rotY;
            Em_UpdateFlight(em, 0xE1, &EmKutku_FlyParam);
            work->moveAngX = em->rotX;
            work->moveAngY = em->rotY;
            work->moveAngZ = 0;
            Em_MoveRotated(em, &work->moveAngX);
            if (--em->timer <= 0) {
                em->step++;
                EmKutku_ChangeState(em, 2, 1, 1);
            }
        }
        break;
    case 2:
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 0xE1, &EmKutku_FlyParam);
        work->moveAngX = em->rotX;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
        if (--em->timer <= 0) {
            em->timer = 0x12C;
            if (em->remote == 0) {
                EmKutku_ChangeState(em, 2, 1, 1);
            }
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmKutku_State2_0C(Em *em)
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
        EmKutku_StartCurves(em, 0xE);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 76.0f) != 0) {
            em->stance = 2;
            ended = EmKutku_UpdateCurves(em) & 0xFF;
        }
        if ((ended != 0) && (em->posY <= em->floorY)) {
            Em_SetMotion(em, 0x39, 0, 0);
            em->step++;
            em->stance = 0;
            em->posY = em->floorY;
            EmKutku_SpawnK14Type0B(em);
            CamShake_ReqAIf(em, 2);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmKutku_State2_0D(Em *em, EmWork *work)
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
        Em_UpdateFlight(em, 0xE5, &EmKutku_FlyParam);
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
        Em_UpdateFlight(em, 0xE1, &EmKutku_FlyParam);
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
        if (em->nextArea == next || next == 0xFFFF) {
            Hud_TriggerGlow(em);
            EmKutku_ChangeState(em, 2, 9, 1);
        } else {
            if (em->remote == 0) {
                em->nextArea = next;
                em->goalNo = next;
                em->goalKind = 3;
                Em_ComputeGoal(em, em->goal);
            }
            EmKutku_ChangeState(em, 2, 0xD, 1);
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

void EmKutku_State2_0E(Em *em, EmWork *work)
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
        EmKutku_MoveByVelYCurve(em, 1U);
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKutku_EndState2(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmKutku_State2_0F(Em *em, EmWork *work)
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
            EmKutku_EndState2(em, 1U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmKutku_State2_10(Em *em, EmWork *work)
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
        EmKutku_MoveByVelYCurve(em, 1U);
        done = Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 1) & 0xFF;
        if (--em->timer > 0) {
            if (done != 0) {
                goto block_9;
            }
        } else {
block_9:
            em->step++;
            EmKutku_EndState2(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmKutku_State2_11(Em *em, EmWork *work)
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
        Em_UpdateFlight(em, 0x321, &EmKutku_FlyParam2);
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

void EmKutku_State2_12(Em *em, EmWork *work)
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
        EmKutku_StartCurves(em, 9);
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
        if (EmKutku_UpdateCurves(em) & 0xFF) {
            em->step++;
            Player_ClearVelocity(em);
            em->timer = 0x12C;
            Em_SetMotion(em, 0xF, 0, 0);
            EmKutku_ChangeState(em, 2, 1, 1);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->timer = 0x12C;
            Em_SetMotion(em, 0xF, 0, 0);
            if (em->remote == 0) {
                EmKutku_ChangeState(em, 2, 1, 1);
            }
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmKutku_State2_13(Em *em, EmWork *work)
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
        Em_UpdateFlight(em, 0x121, &EmKutku_FlyParam2);
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

void EmKutku_State2_14(Em *em, EmWork *work)
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
        EmKutku_StartCurves(em, 0xC);
        work->flyGuard = 0;
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 36.0f) != 0) {
            em->stance = 2;
            ended = EmKutku_UpdateCurves(em) & 0xFF;
        }
        if ((ended != 0) && (em->posY <= em->floorY)) {
            em->step++;
            em->stance = 0;
            em->posY = em->floorY;
            EmKutku_SpawnK14Type0B(em);
            CamShake_ReqAIf(em, 2);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmKutku_State2_15(Em *em, EmWork *work)
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
            EmKutku_EndState2(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmKutku_State2_16(Em *em, EmWork *work)
{
    s32 rotX;
    s32 rotZ;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x5A, 0, 0);
        EmKutku_StartCurves(em, 9);
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
        if (EmKutku_UpdateCurves(em) & 0xFF) {
            em->step++;
            Player_ClearVelocity(em);
            EmKutku_EndState2(em, 0U);
        }
        break;
    case 2:
        EmKutku_UpdateCurves(em);
        break;
    }
}

void EmKutku_State2_17(Em *em, EmWork *work)
{
    f32 floorY;
    f32 velY;
    f32 y;

    switch (em->step) {
    case 0:
        em->step++;
        em->hitReactMode = 0;
        Em_SetMotion(em, 0x12, 0xA, 0x1E);
        EmKutku_StartCurves(em, 2);
        em->stance = 2;
        em->holdTimer = 0;
        em->unk8AD = 0;
        em->noReact = 1;
        break;
    case 1:
        EmKutku_UpdateCurves(em);
        if (em->motion[0].playing == 0) {
            em->step++;
        }
        break;
    case 2:
        if ((EmKutku_UpdateCurves(em) & 0xFF) && (em->posY <= (em->floorY + (800.0f * em->scaleY)))) {
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
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
    y = em->floorY;
    if (em->posY < y) {
        em->posY = y;
    }
}

void EmKutku_State2_18(Em *em, EmWork *work)
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
            EmKutku_EndState2(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmKutku_State3_00(Em *em)
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
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKutku_State3_02(Em *em, EmWork *work)
{
    EmFlyerWork *fw = (EmFlyerWork *)work;
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
                EmKutku_StartCurves(em, 5U);
            }
        }
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 60.0f) != 0) {
            em->stance = 2;
            EmKutku_UpdateCurves(em);
            em->step++;
        }
        break;
    case 2:
        fw->runDist = (fw->runDist - em->velZ);
        EmKutku_UpdateCurves(em);
        if (fw->runDist <= 0.0f) {
            em->step++;
            em->accelY = -10.0f;
        }
        if (fw->runDist <= 500.0f) {
            EmKutku_SpawnK14Type0AEvery5(em);
        }
        break;
    case 3:
        fw->moveAngX = 0;
        fw->moveAngY = em->rotY;
        fw->moveAngZ = 0;
        Em_MoveRotatedAccel(em, &fw->moveAngX);
        if (fw->runDist <= 500.0f) {
            EmKutku_SpawnK14Type0AEvery5(em);
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
            EmKutku_State3_End(em, work);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmKutku_State3_03(Em *em, EmWork *work)
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
            EmKutku_State3_End(em, work);
        }
        break;
    }
}

void EmKutku_State3_04(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x44, 0, 0);
        em->noHitTimer = 5;
        break;
    case 1:
        em->noHitTimer = 5;
        if (Player_PassedFrameScaled(em, 350.0f, 0U) != 0) {
            em->step++;
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            em->timer = 0x12C;
            EmKutku_ChangeState(em, 3, 9, 1);
        }
        break;
    case 3:
        if (--em->timer <= 0) {
            em->timer = 0x12C;
            if (em->remote == 0) {
                EmKutku_ChangeState(em, 3, 9, 1);
            }
        }
        break;
    }
}

void EmKutku_State3_06(Em *em, EmWork *work)
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
            EmKutku_State3_End(em, work);
        }
        break;
    }
}

void EmKutku_State3_07(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x6D, 0, 0);
        em->blind = 0;
        em->scriptPending = 0;
        Em_RestartScript(em);
        em->noReact = 1;
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKutku_State3_08_SwoopThenLeave(Em *em, EmWork *work)
{
    f32 to[3];
    f32 pos[3];
    f32 p2[3];
    AreaRecord *area;
    s8 idx;

    area = Stage_GetAreaRecord(em->area);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        break;
    case 1:
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 0xD1, &EmKutku_FlyParam);
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
            EmKutku_EndSwoop(em, (s32) work);
        }
        break;
    case 2:
        if (em->targetNo == -1) {
            EmKutku_EndSwoop(em, (s32) work);
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
        Em_UpdateFlight(em, 0xD1, &EmKutku_FlyParam);
        if (em->aimAngle <= 0x800 || em->aimAngle >= 0xF800) {
            if (!(4000.0f < Vec_DistanceXZ(&em->posX, pos))) {
                EmKutku_EndSwoop(em, (s32) work);
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
            EmKutku_EndSwoop(em, (s32) work);
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
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 2);
        work->moveAngX = em->rotX;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0xF;
        Em_MoveRotated(em, &work->moveAngX);
        em->posY -= 0.1f;
        if (--em->timer <= 0 || Em_AllPlayersHere(em) == 0) {
            EmKutku_EndSwoop(em, (s32) work);
        }
        break;
    case 4:
        if (em->posY <= 1000.0f + area->bottom) {
            em->step++;
            em->timer = 600;
        }
        if (EmKutku_ReachedTarget(em) && em->step == 4) {
            em->step++;
            em->timer = 600;
        }
        if (em->motionId[0] == 0x415) {
            EmKutku_StepPitch(em, 2, 0.0f);
        } else {
            EmKutku_StepPitch(em, 0, -200.0f);
        }
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 2);
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_GetPitchToTarget(em, &work->moveAngX, -3000.0f, 0.0f);
        if (em->wallHitDirs != 0) {
            work->moveAngX = 0;
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
            EmKutku_EndSwoop(em, (s32) work);
        }
        break;
    case 5:
        EmKutku_ReachedTarget(em);
        if (em->motionId[0] == 0x415 && em->motion[0].playing == 0) {
            em->step = 6;
            Em_SetMotion(em, 0xE, 0, 0);
            EmKutku_SetEscapePoint(em);
        }
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 2);
        EmKutku_StepPitch(em, 2, 100.0f);
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
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        if (em->wallHitDirs != 0) {
            work->moveAngX = 0;
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
            EmKutku_EndSwoop(em, (s32) work);
        }
        break;
    case 6:
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        EmKutku_StepPitch(em, 1, 0.0f);
        Em_UpdateFlight(em, 0xD1, &EmKutku_FlyParam);
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
        EmKutku_StepPitch(em, 2, 0.0f);
        Em_UpdateFlight(em, 0xD1, &EmKutku_FlyParam);
        work->moveAngX = em->rotX;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
        if (Vec_DistanceXZ(&em->posX, em->goal) <= 3000.0f) {
            EmKutku_SetEscapePoint(em);
        }
        if ((em->rotX == 0 && em->rotZ == 0) || --em->timer <= 0) {
            em->rotX = 0;
            em->rotZ = 0;
            EmKutku_EndSwoop(em, (s32) work);
        }
        break;
    }
    if (em->posY < em->floorY) {
        em->posY = em->floorY;
    }
}

void EmKutku_State3_09(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x6E, 0, 0);
        Em_SetCombat(em, 0, 0);
        em->noHitTimer = 5;
        EmMine_CreateTimed0(em, &em->posX);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            em->timer = 300;
            EmKutku_ChangeState(em, 3, 7, 1);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->timer = 300;
            if (em->remote == 0) {
                EmKutku_ChangeState(em, 3, 7, 1);
            }
        }
        break;
    }
}

void EmKutku_State3_0A(Em *em)
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
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKutku_State3_0B_Steal(Em *em, EmWork *work)
{
    Player *p;

    switch (em->step) {
    case 0:
        if (Em_TurnToTargetMotion(em, 0xE38, 3, 6, 5) != 0) {
            em->step++;
            em->stance = 0;
            em->lookAtTarget = 0;
            Em_SetMotion(em, 0x66, 0, 0);
            em->hitPending = 0;
        }
        break;
    case 1:
        if (em->hitPending != 0) {
            p = em->contact;
            if (p != NULL && p->index < 4 && p == &gPlayers[p->index] && em->contactAttach->kind == 0x11 && p->active != 0) {
                em->step++;
                EmKutku_StealItem(em, em->contact);
            }
        }
    case 2:
        if (em->motion[0].playing == 0) {
            em->step = 3;
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKutku_State3_12(Em *em)
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
            EffKutku17_Create(em, 0x24);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKutku_State3_15_SwoopThenLand(Em *em, EmWork *work)
{
    f32 to[3];
    f32 pos[3];
    f32 p2[3];
    AreaRecord *area;
    s8 idx;

    area = Stage_GetAreaRecord(em->area);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        break;
    case 1:
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 0xD1, &EmKutku_FlyParam);
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
            EmKutku_EndSwoop(em, (s32) work);
        }
        break;
    case 2:
        if (em->targetNo == -1) {
            EmKutku_EndSwoop(em, (s32) work);
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
        Em_UpdateFlight(em, 0xD1, &EmKutku_FlyParam);
        if (em->aimAngle <= 0x800 || em->aimAngle >= 0xF800) {
            if (!(4000.0f < Vec_DistanceXZ(&em->posX, pos))) {
                EmKutku_EndSwoop(em, (s32) work);
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
            EmKutku_EndSwoop(em, (s32) work);
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
            EmKutku_StepPitch(em, 0, -200.0f);
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
            EmKutku_EndSwoop(em, (s32) work);
        }
        break;
    case 4:
        if (em->posY <= 1000.0f + area->bottom) {
            em->step++;
            em->timer = 600;
        }
        if (EmKutku_ReachedTarget(em) && em->step == 4) {
            em->step++;
            em->timer = 600;
        }
        if (em->motionId[0] == 0x415) {
            EmKutku_StepPitch(em, 2, 0.0f);
        } else {
            EmKutku_StepPitch(em, 0, -200.0f);
        }
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 2);
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_GetPitchToTarget(em, &work->moveAngX, -3000.0f, 0.0f);
        if (em->wallHitDirs != 0) {
            work->moveAngX = 0;
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
            EmKutku_EndSwoop(em, (s32) work);
        }
        break;
    case 5:
        EmKutku_ReachedTarget(em);
        if (em->motionId[0] == 0x415 && em->motion[0].playing == 0) {
            em->step = 6;
            em->accelX = 0.0f;
            em->accelZ = -10.0f;
            em->accelY = -10.0f;
        }
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 2);
        EmKutku_StepPitch(em, 2, 100.0f);
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
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        if (em->wallHitDirs != 0) {
            work->moveAngX = 0;
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
            EmKutku_EndSwoop(em, (s32) work);
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
            EmKutku_State3_End(em, work);
        }
        break;
    }
    if (em->posY < em->floorY) {
        em->posY = em->floorY;
    }
}

void EmKutku_State3_17(Em *em, EmWork *work, s32 variant)
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
        if (Player_PassedFrameScaled(em, EmKutku_FxFrameBone[variant][0], 0U) != 0) {
            if (em->type == 6 || em->type == 0x26) {
                EffEm08_CreateAtBone(em, 0x22, 3, 0, EmKutku_FxFrameBone[variant][1], 0);
            } else if (em->type == 0x27) {
                EffEm08_CreateAtBone(em, 0x22, 2, 1, EmKutku_FxFrameBone[variant][1], 0);
            } else {
                EffEm08_CreateAtBone(em, 0x22, 2, 0, EmKutku_FxFrameBone[variant][1], 0);
            }
        }
        if (em->motion[0].playing == 0) {
            em->lookAtTarget = 0;
            em->step++;
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKutku_State3_1B(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        
    case 1:
        if (Em_TurnToTargetMotion(em, 0xE38, 3, 6, 5) != 0) {
            em->step++;
            em->lookAtTarget = 0;
            Em_SetMotion(em, 0x69, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->lookAtTarget = 0;
            em->step++;
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKutku_State3_1C(Em *em)
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
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKutku_State3_1D(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x6B, 0, 0);
        ((EmFlyerWork *)work)->unk008 = 1;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->lookAtTarget = 0;
            em->step++;
            ((EmFlyerWork *)work)->unk008 = 0;
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKutku_State3_1E(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x2F, 0, 0);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, EmKutku_FxFrameBone[0][0], 0U) != 0) {
            if (em->type == 6 || em->type == 0x26) {
                EffEm08_CreateAtBone(em, 0x22, 3, 0, EmKutku_FxFrameBone[0][1], 0);
            } else if (em->type == 0x27) {
                EffEm08_CreateAtBone(em, 0x22, 2, 1, EmKutku_FxFrameBone[0][1], 0);
            } else {
                EffEm08_CreateAtBone(em, 0x22, 2, 0, EmKutku_FxFrameBone[0][1], 0);
            }
        }
        if (em->motion[0].playing == 0) {
            em->lookAtTarget = 0;
            em->step++;
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKutku_State3_1F(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x69, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->lookAtTarget = 0;
            em->step++;
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKutku_State4_00(Em *em, EmWork *work)
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
            EmKutku_EndStateOrState3_1E(em, 0, 0);
        }
        break;
    }
}

void EmKutku_State4_01(Em *em, EmWork *work)
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
            EmKutku_EndStateOrState3_1E(em, 0, 0);
        }
        break;
    }
}

void EmKutku_State4_02(Em *em, EmWork *work)
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
            EmKutku_EndStateOrState3_1E(em, 0, 0);
        }
        break;
    }
}

void EmKutku_State4_03(Em *em, EmWork *work)
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
            EmKutku_EndStateOrState3_1E(em, 0, 0);
        }
        break;
    }
}

void EmKutku_State4_04(void)
{
}

void EmKutku_State4_05(Em *em, EmWork *work)
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
            EmKutku_EndState(em, 0, 0);
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
        }
        break;
    }
}

void EmKutku_State4_06(Em *em, EmWork *work)
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
            EmKutku_EndState(em, 0, 0);
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
        }
        break;
    }
}

void EmKutku_State4_07(Em *em, EmWork *work)
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
            EmKutku_EndState2(em, 1);
            em->velY = 0.0f;
        }
        Em_MoveRotated(em, &work->moveAngX);
        break;
    }
}

void EmKutku_State4_08(Em *em, EmWork *work)
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
            EmKutku_EndStateOrState3_1E(em, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    }
}

void EmKutku_State4_09(Em *em, EmWork *work)
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
            EmKutku_EndStateOrState3_1E(em, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    }
}

void EmKutku_State4_0A(Em *em, EmWork *work)
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
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKutku_State4_0B(Em *em, EmWork *work)
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
            em->noReact = 0;
            EmKutku_ChangeState(em, 0, 0x22, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmKutku_ChangeState(em, 0, 0x22, 4);
        }
        break;
    }
    Em_SpawnEffect06Bone(em, 2);
}

void EmKutku_State4_0C(Em *em, EmWork *work)
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
        em->blind = 0;
        break;
    case 1:
        if ((s8) em->holdTimer < 5) {
            em->holdTimer = 5;
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKutku_ChangeState(em, 4, 0xD, 3);
        }
        break;
    case 2:
        if ((s8) em->holdTimer < 5) {
            em->holdTimer = 5;
        }
        if (em->remote == 0) {
            EmKutku_ChangeState(em, 4, 0xD, 3);
        }
        break;
    }
}

void EmKutku_State4_0D(Em *em, EmWork *work)
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
            EmKutku_ChangeState(em, 2, 0x17, 4);
        }
        break;
    case 2:
        if ((em->motion[0].playing == 0) && (em->remote == 0)) {
            Em_SetMotion(em, 0x60, 0, 0);
            EmKutku_ChangeState(em, 2, 0x17, 4);
        }
        break;
    }
}

void EmKutku_State4_0E(Em *em, EmWork *work)
{
    em->holdTimer = 5;
    Em_SetHitReactMode(em, 4U);
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
            Em_SetGoal12(em);
            EmKutku_ChangeState(em, 4, 0x11, 3);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmKutku_ChangeState(em, 4, 0x11, 3);
        }
        break;
    }
    Em_SpawnEffect06Bone(em, 2);
}

void EmKutku_State4_0F(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x6C, 0, 0);
        EmMine_CreateTimed0(em, &em->posX);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x64, 0xA, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKutku_State4_10(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x44, 0, 0);
        em->noHitTimer = 5;
        break;
    case 1:
        em->noHitTimer = 5;
        if (Player_PassedFrameScaled(em, 350.0f, 0U) != 0) {
            em->step++;
        }
        break;
    case 2:
        em->noHitTimer = 5;
        if (em->motion[0].playing == 0) {
            EmKutku_ChangeState(em, 3, 9, 4);
        }
        break;
    }
}

void EmKutku_State4_11(Em *em, EmWork *work)
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
            EmKutku_ChangeState(em, 2, 0x17, 4);
        }
        break;
    case 2:
        if ((em->motion[0].playing == 0) && (em->remote == 0)) {
            Em_SetMotion(em, 0x60, 0, 0);
            EmKutku_ChangeState(em, 2, 0x17, 4);
        }
        break;
    }
}

void EmKutku_State4_12(Em *em, EmWork *work)
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
            EmKutku_ChangeState(em, 4, 0xD, 3);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmKutku_ChangeState(em, 4, 0xD, 3);
        }
        break;
    }
}

void EmKutku_State4_13(Em *em, EmWork *work)
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
            EmKutku_ChangeState(em, 4, 0xD, 3);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmKutku_ChangeState(em, 4, 0xD, 3);
        }
        break;
    }
}

void EmKutku_State4_14(Em *em, EmWork *work)
{
    f32 v[3];
    f32 pos[3];
    f32 m[16];

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x3F, 0, 0);
        v[0] = 0.0f;
        v[1] = 90.0f;
        v[2] = 0.0f;
        Mtx_Copy(m, (f32 *)Em_Bone(em, 0x27));
        Vec_RotateSelf(v, m);
        pos[0] = m[12] + v[0];
        pos[1] = m[13] + v[1];
        pos[2] = m[14] + v[2];
        Effect02_SpawnForOwner(em, 0, 0xA, 0, pos, 1.0f);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKutku_EndStateOrState3_1E(em, 0, 0);
        }
        break;
    }
}

void EmKutku_State4_15(Em *em, EmWork *work)
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
            EmKutku_ChangeState(em, 0, 0x24, 4);
        }
        break;
    case 4:
        if (em->remote == 0) {
            EmKutku_ChangeState(em, 0, 0x24, 4);
        }
        break;
    }
}

void EmKutku_State4_16(Em *em, EmWork *work)
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
            EmKutku_ChangeState(em, 4, 0x17, 3);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmKutku_ChangeState(em, 4, 0x17, 3);
        }
        break;
    }
}

void EmKutku_State4_17(Em *em, EmWork *work)
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
            EmKutku_ChangeState(em, 2, 0x17, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmKutku_ChangeState(em, 2, 0x17, 4);
        }
        break;
    }
}

void EmKutku_State4_18(Em *em, EmWork *work)
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
            EmKutku_ChangeState(em, 0, 8, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmKutku_ChangeState(em, 0, 8, 4);
        }
        break;
    }
}

void EmKutku_State4_19_FeignDeath(Em *em, EmWork *work)
{
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x44, 0, 0);
        em->noHitTimer = 5;
        em->noReact = 1;
        em->timer = 300;
        break;
    case 1:
        em->noHitTimer = 5;
        if (Player_PassedFrameScaled(em, 212.0f, 0) != 0) {
            CamShake_ReqAIf(em, 2);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKutku_AddMinePoint(em);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            EmKutku_ChangeState(em, 3, 0x20, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmKutku_ChangeState(em, 3, 0x20, 4);
        }
        break;
    }
}

void EmKutku_State6_00(Em *em, EmWork *work)
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
        if (Save_TestEventFlag(0xA6) == 1) {
            em->step++;
            em->senseDelay = 0;
            em->noHitTimer = 0;
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmKutku_State6_04_Captured(Em *em)
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
                EmKutku_ChangeState(em, 5, 3, 4);
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

void EmKutku_State6_05_Captured(Em *em)
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
                EmKutku_ChangeState(em, 5, 3, 4);
                break;
            }
        } else {
            em->alpha = (f32) timer / 150.0f;
        }
        break;
    }
}

void EmKutku_State8_00_CarveStart(Em *em, EmWork *work)
{
    EmKutkuVec v;

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
            v = EmKutku_CarveThrowPt;
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

void EmKutku_State8_01(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    if (em->type == 6 || em->type == 0x26) {
        switch (em->step) {
        case 0:
            em->step++;
            Em_SetMotion(em, 0x6C, 0, 0);
            break;
        case 1:
            if (em->motion[0].playing == 0) {
                em->step++;
                Em_SetMotion(em, 1, 0, 0);
                EmKutku_ChangeState(em, 8, 0x98, 1);
            }
            break;
        }
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            Em_SetMotion(em, 0x44, 0, 0);
            break;
        case 1:
            if (em->motion[0].playing == 0) {
                em->step++;
                Em_SetMotion(em, 0x6D, 0, 0);
            }
            break;
        case 2:
            if (em->motion[0].playing == 0) {
                em->step++;
                Em_SetMotion(em, 1, 0, 0);
                EmKutku_ChangeState(em, 8, 0x98, 1);
            }
            break;
        }
    }
}

void EmKutku_State8_02(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    if (em->type == 6 || em->type == 0x26) {
        switch (em->step) {
        case 0:
            em->step++;
            Em_SetMotion(em, 0x65, 0, 0);
            break;
        case 1:
            if (em->motion[0].playing == 0) {
                em->step++;
                Em_SetMotion(em, 1, 0, 0);
                EmKutku_ChangeState(em, 8, 0x98, 1);
            }
            break;
        }
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            Em_SetMotion(em, 0x64, 0, 0);
            break;
        case 1:
            if (em->motion[0].playing == 0) {
                em->step++;
                Em_SetMotion(em, 1, 0, 0);
                EmKutku_ChangeState(em, 8, 0x98, 1);
            }
            break;
        }
    }
}

void EmKutku_State8_03(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    if (em->type == 6 || em->type == 0x26) {
        switch (em->step) {
        case 0:
            em->step++;
            Em_SetMotion(em, 0x36, 0, 0);
            break;
        case 1:
            if (em->motion[0].playing == 0) {
                em->step++;
                Em_SetMotion(em, 1, 0, 0);
                EmKutku_ChangeState(em, 8, 0x98, 1);
            }
            break;
        }
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            Em_SetMotion(em, 0x6B, 0, 0);
            break;
        case 1:
            if (em->motion[0].playing == 0) {
                em->step++;
                Em_SetMotion(em, 1, 0, 0);
                EmKutku_ChangeState(em, 8, 0x98, 1);
            }
            break;
        }
    }
}

void EmKutku_State8_98(Em *em, EmWork *work)
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
            EmKutku_ChangeState(em, 8, 0x99, 1);
        }
        break;
    }
}

void EmKutku_State8_99(Em *em, EmWork *work)
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
            EmKutku_EndState(em, 0, 0);
        }
        break;
    }
}

void EmKutku_State5_00_Hunted(Em *em, EmWork *work)
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
                EmKutku_ChangeState(em, 5, 3, 4);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    case 99:
        if (em->motion[0].playing == 0) {
            em->stance = 0;
            EmKutku_EndState(em, 0, 0);
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
        }
        break;
    }
}

void EmKutku_State5_01_Hunted(Em *em, EmWork *work)
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
                EmKutku_ChangeState(em, 5, 3, 4);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    case 99:
        if (em->motion[0].playing == 0) {
            em->stance = 0;
            EmKutku_EndState(em, 0, 0);
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
        }
        break;
    }
}

void EmKutku_State5_02_HuntedFalling(Em *em, EmWork *work)
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
                EmKutku_ChangeState(em, 5, 3, 4);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    case 99:
        if (em->motion[0].playing == 0) {
            em->stance = 0;
            EmKutku_EndState(em, 0, 0);
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
        }
        break;
    }
}

void EmKutku_State5_03_Appear(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        if (QuestEm_CanAppear(em) == 1) {
            switch (Area_BaseId(gScene.map)) {
            case 9:
                em->area = Area_IdForCurrentTime(0x5A);
                if (em->variant == 0) {
                    em->rotY = 0x8A76;
                    em->posX = 11948.9f;
                    em->posY = 1010.0f;
                    em->posZ = 15923.8f;
                } else {
                    em->rotY = 0x4861;
                    em->posX = 3743.2f;
                    em->posY = 1010.0f;
                    em->posZ = 11415.0f;
                }
                EmKutku_ChangeState(em, 0, 1, 0);
                break;
            default:
                em->posY = gAreaGroundY[em->area];
                em->stance = 2;
                EmKutku_ChangeState(em, 2, 1, 0);
                break;
            }
            break;
        }
        em->taskPhase++;
        break;
    }
}

void EmKutku_RunGroup0(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmKutku_State0_00(em);
        break;
    case 1:
        EmKutku_State0_01(em);
        break;
    case 2:
        EmKutku_State0_02(em);
        break;
    case 3:
        EmKutku_State0_03(em);
        break;
    case 4:
        EmKutku_State0_04(em);
        break;
    case 5:
        EmKutku_State0_05(em);
        break;
    case 6:
        EmKutku_State0_06(em);
        break;
    case 7:
        EmKutku_State0_07(em);
        break;
    case 8:
        EmKutku_State0_08(em, work, 2);
        break;
    case 9:
        EmKutku_State0_09(em);
        break;
    case 0xA:
        EmKutku_State0_0A(em);
        break;
    case 0xB:
        EmKutku_State0_0B(em);
        break;
    case 0xC:
        EmKutku_State0_0C(em);
        break;
    case 0xD:
        EmKutku_State0_0D(em);
        break;
    case 0xE:
        EmKutku_State0_0E(em);
        break;
    case 0xF:
        EmKutku_State0_0F(em);
        break;
    case 0x10:
        EmKutku_State0_10(em);
        break;
    case 0x11:
        EmKutku_State0_11(em);
        break;
    case 0x12:
        EmKutku_State0_12(em);
        break;
    case 0x13:
        EmKutku_State0_13(em);
        break;
    case 0x14:
        EmKutku_State0_14(em);
        break;
    case 0x15:
        EmKutku_State0_15(em);
        break;
    case 0x16:
        EmKutku_State0_16(em);
        break;
    case 0x17:
        EmKutku_State0_17(em);
        break;
    case 0x18:
        EmKutku_State0_18(em);
        break;
    case 0x19:
        EmKutku_State0_19(em);
        break;
    case 0x1A:
        EmKutku_State0_1A(em);
        break;
    case 0x1B:
        EmKutku_State0_1B(em);
        break;
    case 0x1C:
        EmKutku_State0_1C(em);
        break;
    case 0x1D:
        EmKutku_State0_1D(em, work);
        break;
    case 0x1F:
        EmKutku_State0_1F(em, work);
        break;
    case 0x21:
        EmKutku_State0_21(em);
        break;
    case 0x22:
        EmKutku_State0_08(em, work, 0);
        break;
    case 0x23:
        EmKutku_State0_21(em);
        break;
    case 0x24:
        EmKutku_State0_08(em, work, 1);
        break;
    case 0x25:
        EmKutku_State0_21(em);
        break;
    case 0x28:
        EmKutku_State0_28(em);
        break;
    }
}

void EmKutku_RunGroup1(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmKutku_State1_00(em);
        break;
    case 1:
        EmKutku_State1_01(em);
        break;
    case 2:
        EmKutku_State1_02(em);
        break;
    case 3:
        EmKutku_State1_03(em);
        break;
    case 4:
        EmKutku_State1_04(em);
        break;
    case 5:
        EmKutku_State1_05(em);
        break;
    case 6:
        EmKutku_State1_06(em);
        break;
    case 7:
        EmKutku_State1_07(em, work);
        break;
    case 8:
        EmKutku_State1_04(em);
        break;
    case 9:
        EmKutku_State1_09();
        break;
    case 10:
        EmKutku_State1_0A(em);
        
    default:
        break;
    }
}

void EmKutku_RunGroup2(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmKutku_State2_00(em, work);
        break;
    case 1:
        EmKutku_State2_01_FlyInLand(em, work);
        break;
    case 2:
        EmKutku_State2_02(em, work);
        break;
    case 3:
        EmKutku_State2_03(em, work);
        break;
    case 4:
        EmKutku_State2_04(em, work);
        break;
    case 5:
        EmKutku_State2_05(em, work);
        break;
    case 6:
        EmKutku_State2_06(em, work);
        break;
    case 7:
        EmKutku_State2_07(em, work);
        break;
    case 8:
        EmKutku_State2_08(em, work);
        break;
    case 9:
        EmKutku_State2_09(em, work);
        break;
    case 10:
        EmKutku_State2_0A(em);
        break;
    case 11:
        EmKutku_State2_0B(em, work);
        break;
    case 12:
        EmKutku_State2_0C(em);
        break;
    case 13:
        EmKutku_State2_0D(em, work);
        break;
    case 14:
        EmKutku_State2_0E(em, work);
        break;
    case 15:
        EmKutku_State2_0F(em, work);
        break;
    case 16:
        EmKutku_State2_10(em, work);
        break;
    case 17:
        EmKutku_State2_11(em, work);
        break;
    case 18:
        EmKutku_State2_12(em, work);
        break;
    case 19:
        EmKutku_State2_13(em, work);
        break;
    case 20:
        EmKutku_State2_14(em, work);
        break;
    case 21:
        EmKutku_State2_15(em, work);
        break;
    case 22:
        EmKutku_State2_16(em, work);
        break;
    case 23:
        EmKutku_State2_17(em, work);
        break;
    case 24:
        EmKutku_State2_18(em, work);
        
    default:
        break;
    }
}

void EmKutku_RunGroup3(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmKutku_State3_00(em);
        break;
    case 2:
        EmKutku_State3_02(em, work);
        break;
    case 3:
        EmKutku_State3_03(em, work);
        break;
    case 4:
        EmKutku_State3_04(em);
        break;
    case 6:
        EmKutku_State3_06(em, work);
        break;
    case 7:
        EmKutku_State3_07(em);
        break;
    case 8:
        EmKutku_State3_08_SwoopThenLeave(em, work);
        break;
    case 9:
        EmKutku_State3_09(em, work);
        break;
    case 0xA:
        EmKutku_State3_0A(em);
        break;
    case 0xB:
        EmKutku_State3_0B_Steal(em, work);
        break;
    case 0x12:
        EmKutku_State3_12(em);
        break;
    case 0x15:
        EmKutku_State3_15_SwoopThenLand(em, work);
        break;
    case 0x17:
        EmKutku_State3_17(em, work, 1);
        break;
    case 0x18:
        EmKutku_State3_17(em, work, 2);
        break;
    case 0x19:
        EmKutku_State3_17(em, work, 3);
        break;
    case 0x1A:
        EmKutku_State3_17(em, work, 0);
        break;
    case 0x1B:
        EmKutku_State3_1B(em);
        break;
    case 0x1C:
        EmKutku_State3_1C(em);
        break;
    case 0x1D:
        EmKutku_State3_1D(em, work);
        break;
    case 0x1E:
        EmKutku_State3_1E(em, work);
        break;
    case 0x1F:
        EmKutku_State3_1F(em);
        break;
    case 0x20:
        EmKutku_State3_07(em);
        break;
    }
}

void EmKutku_RunGroup4(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmKutku_State4_00(em, work);
        break;
    case 1:
        EmKutku_State4_01(em, work);
        break;
    case 2:
        EmKutku_State4_02(em, work);
        break;
    case 3:
        EmKutku_State4_03(em, work);
        break;
    case 4:
        EmKutku_State4_04();
        break;
    case 5:
        EmKutku_State4_05(em, work);
        break;
    case 6:
        EmKutku_State4_06(em, work);
        break;
    case 7:
        EmKutku_State4_07(em, work);
        break;
    case 8:
        EmKutku_State4_08(em, work);
        break;
    case 9:
        EmKutku_State4_09(em, work);
        break;
    case 10:
        EmKutku_State4_0A(em, work);
        break;
    case 11:
        EmKutku_State4_0B(em, work);
        break;
    case 12:
        EmKutku_State4_0C(em, work);
        break;
    case 13:
        EmKutku_State4_0D(em, work);
        break;
    case 14:
        EmKutku_State4_0E(em, work);
        break;
    case 15:
        EmKutku_State4_0F(em, work);
        break;
    case 16:
        EmKutku_State4_10(em, work);
        break;
    case 17:
        EmKutku_State4_11(em, work);
        break;
    case 18:
        EmKutku_State4_12(em, work);
        break;
    case 19:
        EmKutku_State4_13(em, work);
        break;
    case 20:
        EmKutku_State4_14(em, work);
        break;
    case 21:
        EmKutku_State4_15(em, work);
        break;
    case 22:
        EmKutku_State4_16(em, work);
        break;
    case 23:
        EmKutku_State4_17(em, work);
        break;
    case 24:
        EmKutku_State4_18(em, work);
        break;
    case 25:
        EmKutku_State4_19_FeignDeath(em, work);
        
    default:
        break;
    }
}

void EmKutku_RunGroup5(Em *em, EmWork *work)
{
    em->noPushTimer = 5;
    switch (em->state) {
    case 0:
        EmKutku_State5_00_Hunted(em, work);
        break;
    case 1:
        EmKutku_State5_01_Hunted(em, work);
        break;
    case 2:
        EmKutku_State5_02_HuntedFalling(em, work);
        break;
    case 3:
        EmKutku_State5_03_Appear(em, work);
        break;
    }
}

void EmKutku_RunGroup6(Em *em, s32 work)
{
    switch (em->state) {
    case 0:
        EmKutku_State6_00(em, (EmWork *) work);
        break;
    case 4:
        EmKutku_State6_04_Captured(em);
        break;
    case 5:
        EmKutku_State6_05_Captured(em);
        break;
    }
}

void EmKutku_RunGroup8(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0x0:
        EmKutku_State8_00_CarveStart(em, work);
        break;
    case 0x1:
        EmKutku_State8_01(em, work);
        break;
    case 0x2:
        EmKutku_State8_02(em, work);
        break;
    case 0x3:
        EmKutku_State8_03(em, work);
        break;
    case 0x98:
        EmKutku_State8_98(em, work);
        break;
    case 0x99:
        EmKutku_State8_99(em, work);
        break;
    }
}

void EmKutku_Update(Em *em)
{
    u8 buf[4];
    EmFlyerWork *w = (EmFlyerWork *)&em->work;

    Em_UpdateCombatTimer(em);
    Em_RaiseTrigger80Lax(em);
    if (em->stateGroup != 5) {
        if (func_00786E50(em) == 1) {
            w->unk008 = 0;
        } else if (em->rage != 0) {
            w->unk008 ^= 1;
        } else {
            w->unk008 = 0;
        }
    } else {
        w->unk008 = 0;
    }
    if (em->noAlert != 1 && em->rage == 0) {
        switch (Area_BaseId(gScene.map)) {
        case 9:
        case 0xA:
            break;
        case 4:
            Em_CheckLowHp(em, 0.2f);
            Em_DecayNeed2(em);
            Em_CheckCarryRequest(em);
            Em_DecayNeed1Only(em);
            break;
        case 2:
            Em_CheckLowHp(em, 0.2f);
            Em_DecayNeed2(em);
            Em_DecayNeed1(em);
            break;
        case 6:
            Em_CheckLowHp(em, 0.2f);
            Em_DecayNeed2(em);
            Em_DecayNeed1Only(em);
            break;
        default:
            Em_DecayNeed1Only(em);
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
        break;
    case 1:
    case 2:
        if (em->stance == 2) {
            EmKutku_ChangeState(em, 5, 2, 3);
        } else if ((s8) em->holdTimer != 0) {
            EmKutku_ChangeState(em, 5, 1, 3);
        } else {
            EmKutku_ChangeState(em, 5, 0, 3);
        }
        break;
    case 3:
    case 4:
        if ((s8) em->holdTimer == 0) {
            Em_SetGoal12Dist(em, buf);
            Em_SetGoal12(em);
            EmKutku_ChangeState(em, 4, 0xC, 3);
        }
        break;
    case 17:
        if ((s8) em->holdTimer != 0) {
            Em_SetStatus5Timer(em);
            EmKutku_ChangeState(em, 4, 0x16, 3);
        } else if ((em->stateGroup != 4 || em->state != 0x15) && (em->stateGroup != 4 || em->state != 8)) {
            Em_SetStatus5Timer(em);
            EmKutku_ChangeState(em, 4, 0x15, 3);
        }
        break;
    case 18:
        Em_ResetKindTimer(em);
        EmKutku_ChangeState(em, 4, 0x18, 3);
        break;
    case 15:
        if (em->stateGroup == 4 && em->state == 0xF) {
            break;
        }
        if ((s8) em->holdTimer != 0) {
            break;
        }
        if (em->stance == 2) {
            if (em->rage == 0) {
                Em_CheckRageByHp(em, em->rageThreshold);
            }
            EmKutku_ChangeState(em, 4, 8, 3);
            break;
        }
        if (em->rage == 0 && em->type != 0x14 && em->type != 0x27) {
            Em_CheckRageByHp(em, em->rageThreshold);
            EmKutku_ChangeState(em, 4, 0xF, 3);
        }
        break;
    case 5:
        if (em->stateGroup != 4 || em->state != 8) {
            EmKutku_ChangeState(em, 4, 8, 3);
        }
        break;
    case 6:
        if ((s8) em->holdTimer != 0) {
            Em_SetStatus4Timer(em);
            EmKutku_ChangeState(em, 4, 0xE, 3);
        } else if ((em->stateGroup != 4 || em->state != 0xB) && (em->stateGroup != 4 || em->state != 8)) {
            Em_SetStatus4Timer(em);
            EmKutku_ChangeState(em, 4, 0xB, 3);
        }
        break;
    case 7:
        if (Em_IsHeld(em) == 1) {
            Em_SetStatus1Timer(em);
            if ((s8) em->holdTimer != 0) {
                EmKutku_ChangeState(em, 6, 4, 3);
                break;
            }
            if ((s8) em->holdTimer2 != 0) {
                EmKutku_ChangeState(em, 6, 5, 3);
                break;
            }
        }
        if ((em->stateGroup != 0 || em->state != 0x1B) && (em->stateGroup != 4 || em->state != 8)) {
            Em_SetStatus1Timer(em);
            EmKutku_ChangeState(em, 0, 0x1B, 3);
        }
        break;
    case 8:
        if ((s8) em->holdTimer != 0) {
            Em_SetStatus2Timer(em);
            EmKutku_ChangeState(em, 0, 0x1D, 3);
        } else if ((s8) em->holdTimer2 != 0) {
            Em_SetStatus2Timer(em);
            EmKutku_ChangeState(em, 0, 0x14, 3);
        } else if ((em->stateGroup != 0 || em->state != 0x14) && (em->stateGroup != 4 || em->state != 8)) {
            Em_SetStatus2Timer(em);
            EmKutku_ChangeState(em, 0, 0x14, 3);
        }
        break;
    case 10:
        switch (em->state) {
        case 0x12:
            EmKutku_ChangeState(em, 0, 0x17, 3);
            em->scriptPending = 0;
            Em_CheckRageByHp(em, em->rageThreshold);
            break;
        case 0x14:
        case 0x15:
            EmKutku_ChangeState(em, 0, 0x18, 3);
            em->scriptPending = 0;
            break;
        case 0x1B:
            EmKutku_ChangeState(em, 0, 0x1C, 3);
            em->scriptPending = 0;
            break;
        case 0x1D:
            EmKutku_ChangeState(em, 4, 0x12, 3);
            em->scriptPending = 0;
            break;
        case 0x1F:
            EmKutku_ChangeState(em, 4, 0x13, 3);
            em->scriptPending = 0;
            break;
        }
        break;
    case 11:
        EmKutku_ChangeState(em, 4, 4, 3);
        break;
    case 16:
        if (em->stateGroup == 4 && em->state == 0xF) {
            break;
        }
        if ((s8) em->holdTimer != 0) {
            break;
        }
        if (em->stance == 2) {
            if (em->rage == 0) {
                Em_CheckRageByHp(em, em->rageThreshold);
            }
            EmKutku_ChangeState(em, 4, 8, 3);
            break;
        }
        if (em->rage == 0 && em->type != 0x14 && em->type != 0x27) {
            Em_CheckRageByHp(em, em->rageThreshold);
            EmKutku_ChangeState(em, 4, 0xF, 3);
            break;
        }
    case 12:
        Player_ClearFlag(em, 0x20000);
        if (em->stance == 2) {
            EmKutku_ChangeState(em, 4, 8, 3);
            break;
        }
        if (em->type == 0x14 || em->type == 0x27) {
            if (em->hp <= em->hpMax * 30 / 100) {
                if (em->stateRand % 100 < 30) {
                    if (em->level >= 11 && em->level < 18) {
                        EmKutku_ChangeState(em, 4, 0x19, 3);
                    } else {
                        EmKutku_ChangeState(em, 4, 0x10, 3);
                    }
                    break;
                }
            } else if (em->hp <= em->hpMax * 50 / 100) {
                if (em->stateRand % 100 < 20) {
                    if (em->level >= 11 && em->level < 18) {
                        EmKutku_ChangeState(em, 4, 0x19, 3);
                    } else {
                        EmKutku_ChangeState(em, 4, 0x10, 3);
                    }
                    break;
                }
            }
        }
        switch (em->flinchPart) {
        case 0:
        case 7:
            EmKutku_ChangeState(em, 4, 0, 3);
            break;
        case 6:
            if ((em->type == 0x14 || em->type == 0x27) && w->partWhole != 0 && em->parts[6].state >= 2) {
                EmKutku_ChangeState(em, 4, 0x14, 3);
                break;
            }
        case 5:
            EmKutku_ChangeState(em, 4, 2, 3);
            break;
        case 1:
        case 2:
            EmKutku_ChangeState(em, 4, 3, 3);
            break;
        case 3:
        case 4:
        default:
            if (em->parts[em->flinchPart].state >= 2) {
                if (em->flinchPart != 3) {
                    EmKutku_ChangeState(em, 4, 5, 3);
                } else {
                    EmKutku_ChangeState(em, 4, 6, 3);
                }
            } else {
                EmKutku_ChangeState(em, 4, 1, 3);
            }
            break;
        }
        break;
    case 13:
        if (em->stance != 2) {
            EmKutku_ChangeState(em, 4, 0, 3);
            em->scriptPending = 0;
        }
        break;
    }
    if ((em->type == 6 || em->type == 0x26) && em->parts[6].state >= 2) {
        Em_BreakPart(em, 1);
    }
    switch (gQuest.stage) {
    case 0x3EE:
    case 0x3F0:
        if (Area_DayId(em->area) == 0x77 && Save_TestEventFlag(0xA6) == 0) {
            if (Area_DayId(gScene.area) == 0x77 && gScene.eventRunning == 1 && em->stateGroup != 6) {
                EmKutku_ChangeState(em, 6, 0, 1);
            }
            break;
        }
    default:
        Em_UpdatePendingScript(em);
        break;
    }
    Em_ClearHitReactMode(em);
    EmKutku_RunState(em, (s32) w);
    if (em->stateRequest != 0) {
        EmKutku_RunState(em, (s32) w);
        em->stateRequest = 0;
    }
    if (em->stateGroup == 2 && (s8) w->flyGuard == 1) {
        em->guardTimer = 2;
    }
}

void EmKutku_RunState(Em *em, s32 work)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->stateGroup) {
    case 0:
        EmKutku_RunGroup0(em, (EmWork *) work);
        break;
    case 1:
        EmKutku_RunGroup1(em, (EmWork *) work);
        break;
    case 2:
        EmKutku_RunGroup2(em, (EmWork *) work);
        break;
    case 3:
        EmKutku_RunGroup3(em, (EmWork *) work);
        break;
    case 4:
        EmKutku_RunGroup4(em, (EmWork *) work);
        break;
    case 5:
        EmKutku_RunGroup5(em, (EmWork *) work);
        break;
    case 6:
        EmKutku_RunGroup6(em, work);
        break;
    case 7:
        EmKutku_RunGroup6(em, work);
        break;
    case 8:
        EmKutku_RunGroup8(em, (EmWork *) work);
        
    default:
        break;
    }
}

void EmKutku_RequestOnFrame(void *em, s32 frame, s32 i)
{
    if (Player_PassedFrameScaled((Em *) em, (f32) frame, 0U) != 0) {
        CamShake_ReqAIf(em, i);
    }
}

static void EmKutku_MotionEffectsDefault(Em *em)
{
}

void EmKutku_MotionEffects(Em *em, EmKutkuFxWork *work)
{
    f32 v2[3];
    f32 v1[3];
    f32 v0[3];

    if (em->motionId[0] != work->motion) {
        work->motion = em->motionId[0];
    }
    EmKutku_MotionSounds(em, (struct EmWork *)work);
    switch (work->motion) {
    case 0x3EB:
        EmKutku_RequestOnFrame(em, 0x34, 1);
        EmKutku_RequestOnFrame(em, 0x74, 1);
        if (Player_PassedFrameScaled(em, 2.0f, 0) != 0) {
            EffKutku17_Create(em, 2);
        }
        if (Player_PassedFrameScaled(em, 50.0f, 0) != 0) {
            EffKutku17_Create(em, 3);
        }
        if (Player_PassedFrameScaled(em, 120.0f, 0) != 0) {
            EffKutku17_Create(em, 4);
            break;
        }
        break;
    case 0x3EC:
        if (Player_PassedFrameScaled(em, 38.0f, 0) != 0) {
            StageObj_K14_Create(em, 3, 0, 1.0f);
        }
        if (Player_PassedFrameScaled(em, 74.0f, 0) != 0) {
            StageObj_K14_Create(em, 2, 0, 1.0f);
            break;
        }
        break;
    case 0x3ED:
        EmKutku_RequestOnFrame(em, 0x3C, 1);
        if (Player_PassedFrameScaled(em, 2.0f, 0) != 0) {
            EffKutku17_Create(em, 5);
        }
        if (Player_PassedFrameScaled(em, 30.0f, 0) != 0) {
            EffKutku17_Create(em, 6);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 2.0f, 78.0f) != 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create(em, 0x1A, 1, 1.0f);
            break;
        }
        break;
    case 0x3EE:
        EmKutku_RequestOnFrame(em, 0x3C, 1);
        if (Player_PassedFrameScaled(em, 30.0f, 0) != 0) {
            EffKutku17_Create(em, 7);
        }
        if (Player_PassedFrameScaled(em, 2.0f, 0) != 0) {
            EffKutku17_Create(em, 8);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 2.0f, 78.0f) != 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create(em, 0x1A, 0, 1.0f);
            break;
        }
        break;
    case 0x3EF:
        if (Player_PassedFrameScaled(em, 18.0f, 0) != 0) {
            EffKutku17_Create(em, 9);
        }
        if (Player_PassedFrameScaled(em, 2.0f, 0) != 0) {
            EffKutku17_Create(em, 0xA);
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
            EffKutku17_Create(em, 0xB);
        }
        if (Player_PassedFrameScaled(em, 18.0f, 0) != 0) {
            EffKutku17_Create(em, 0xC);
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
            EffKutku17_Create(em, 0x19);
        }
        if (Player_PassedFrameScaled(em, 2.0f, 0) != 0) {
            EffKutku17_Create(em, 0x1A);
        }
        if (Player_PassedFrameScaled(em, 74.0f, 0) != 0) {
            EffKutku17_Create(em, 0x1A);
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
    case 0x3F6:
    case 0x43D:
    case 0x43E:
    case 0x440:
    case 0x441:
        if ((em->rage != 0) && ((gScene.frame % 10) == 0)) {
            StageObj_K14_Create(em, 0x19, (Rand_Next(1) & 1) + 2, 1.0f);
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
    case 0x3F9:
        if (Player_PassedFrameScaled(em, 18.0f, 0) != 0) {
            EffKutku17_Create(em, 0x13);
        }
        if (Player_PassedFrameScaled(em, 48.0f, 0) != 0) {
            if (em->type == 6 || em->type == 38) {
            EffEm08_CreateAtBone(em, 0x22, 3, 0, 0x222, 0x2CCD);
            } else if (em->type == 39) {
            EffEm08_CreateAtBone(em, 0x22, 2, 1, 0x222, 0x2CCD);
            } else {
            EffEm08_CreateAtBone(em, 0x22, 2, 0, 0x222, 0x2CCD);
            }
        }
        if (Player_PassedFrameScaled(em, 86.0f, 0) != 0) {
            if (em->type == 6 || em->type == 38) {
            EffEm08_CreateAtBone(em, 0x22, 3, 0, 0x444, 0xD112);
            } else if (em->type == 39) {
            EffEm08_CreateAtBone(em, 0x22, 2, 1, 0x444, 0xD112);
            } else {
            EffEm08_CreateAtBone(em, 0x22, 2, 0, 0x444, 0xD112);
            }
        }
        if ((Player_PassedFrameScaled(em, 12.0f, 0) != 0) || (Player_PassedFrameScaled(em, 72.0f, 0) != 0)) {
            StageObj_K14_Create(em, 2, 0, 1.0f);
        }
        if (Player_PassedFrameScaled(em, 40.0f, 0) != 0) {
            StageObj_K14_Create(em, 3, 0, 1.0f);
            break;
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
            EffKutku17_Create(em, 0xD);
        }
        if (Player_PassedFrameScaled(em, 16.0f, 0) != 0) {
            EmKutku_SpawnK14Type0B(em);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 18.0f, 44.0f) != 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
            break;
        }
        break;
    case 0x407:
        v2[1] = 10.0f;
        v2[2] = 140.0f;
        v2[0] = 0.0f;
        Em_SpawnEffect06Periodic(em, 0x22, v2, 1.6f);
        break;
    case 0x40A:
        v1[1] = 10.0f;
        v1[2] = 140.0f;
        v1[0] = 0.0f;
        Em_SpawnEffect06Periodic(em, 0x22, v1, 1.6f);
        break;
    case 0x40C:
        if (Player_PassedFrameScaled(em, 50.0f, 0) != 0) {
            EffKutku17_Create(em, 1);
        }
        if (Player_PassedFrameScaled(em, 46.0f, 0) != 0) {
            EffKutku17_Create(em, 0x21);
            break;
        }
        break;
    case 0x40E:
        if (Player_PassedFrameScaled(em, 148.0f, 0) != 0) {
            EffKutku17_Create(em, 0xF);
        }
        if (Player_PassedFrameScaled(em, 156.0f, 0) != 0) {
            StageObj_K14_Create(em, 0, 0, 1.0f);
            break;
        }
        break;
    case 0x410:
        if (Player_PassedFrameScaled(em, 60.0f, 0) != 0) {
            EffKutku17_Create(em, 0x1C);
            break;
        }
        break;
    case 0x411:
        if (Player_PassedFrameScaled(em, 2.0f, 0) != 0) {
            EffKutku17_Create(em, 0x1D);
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
            break;
        }
        break;
    case 0x412:
        EmKutku_RequestOnFrame(em, 0x2C, 1);
        EmKutku_RequestOnFrame(em, 0x49, 1);
        if (Player_PassedFrameScaled(em, 30.0f, 0) != 0) {
            EffKutku17_Create(em, 0x11);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 6.0f, 38.0f) != 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create(em, 0x1A, 0, 1.0f);
            break;
        }
        break;
    case 0x413:
        EmKutku_RequestOnFrame(em, 0x2C, 1);
        EmKutku_RequestOnFrame(em, 0x49, 1);
        if (Player_PassedFrameScaled(em, 30.0f, 0) != 0) {
            EffKutku17_Create(em, 0x11);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 6.0f, 38.0f) != 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create(em, 0x1A, 1, 1.0f);
            break;
        }
        break;
    case 0x415:
        if (Player_PassedFrameScaled(em, 22.0f, 0) != 0) {
            EffKutku17_Create(em, 0x14);
            EffKutku17_Create(em, 0x15);
        }
        if (Player_PassedFrameScaled(em, 20.0f, 0) || Player_PassedFrameScaled(em, 68.0f, 0) || Player_PassedFrameScaled(em, 114.0f, 0)) {
            if (em->posY <= (1000.0f + em->floorY)) {
                StageObj_K14_Create(em, 9, 0, 1.0f);
                StageObj_K14_Create(em, 0xE, 0, 1.0f);
                break;
            }
        }
        break;
    case 0x417:
        if (Player_PassedFrameScaled(em, 30.0f, 0) != 0) {
            EffKutku17_Create(em, 0x22);
            break;
        }
        break;
    case 0x41B:
        if ((em->type == 0x14) || (em->type == 0x27)) {
            if (em->rage != 0) {
                if (EmMotion_InFrameRange(em, 0, 86.0f, 92.0f) || EmMotion_InFrameRange(em, 0, 146.0f, 152.0f) || EmMotion_InFrameRange(em, 0, 196.0f, 202.0f)) {
                    StageObj_K14_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
                }
            }
            if ((work->unk01B != 0) && ((Player_PassedFrameScaled(em, 88.0f, 0) != 0) || (Player_PassedFrameScaled(em, 146.0f, 0) != 0) || (Player_PassedFrameScaled(em, 198.0f, 0) != 0))) {
                *(s8 *) ((u8 *) work + 8) = 1;
                break;
            }
        } else if (em->rage != 0) {
            if (EmMotion_InFrameRange(em, 0, 72.0f, 80.0f) || EmMotion_InFrameRange(em, 0, 132.0f, 140.0f) || EmMotion_InFrameRange(em, 0, 184.0f, 192.0f)) {
                StageObj_K14_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
                break;
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
            break;
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
            EffKutku17_Create(em, 0x1B);
        }
        if (Player_PassedFrameScaled(em, 2.0f, 0) != 0) {
            EffKutku17_Create(em, 0x1F);
        }
        if (Player_PassedFrameScaled(em, 36.0f, 0) != 0) {
            EffKutku17_Create(em, 0x20);
            break;
        }
        break;
    case 0x424:
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
    case 0x42C:
        if (Player_PassedFrameScaled(em, 270.0f, 0) != 0) {
            StageObj_K14_Create(em, 0x12, 0x80, 0.9f);
            break;
        }
        break;
    case 0x42D:
        EmKutku_RequestOnFrame(em, 0x1A, 2);
        if (Player_PassedFrameScaled(em, 26.0f, 0) != 0) {
            StageObj_K14_Create(em, 0x12, 0x80, 1.0f);
            break;
        }
        break;
    case 0x432:
        EmKutku_RequestOnFrame(em, 0x1A, 2);
        if (Player_PassedFrameScaled(em, 26.0f, 0) != 0) {
            StageObj_K14_Create(em, 0x12, 0x81, 1.0f);
            break;
        }
        break;
    case 0x436:
        if (Player_PassedFrameScaled(em, 8.0f, 0) != 0) {
            StageObj_K14_Create(em, 0xB, 0, 1.0f);
            break;
        }
        break;
    case 0x437:
        if (Player_PassedFrameScaled(em, 48.0f, 0) || Player_PassedFrameScaled(em, 106.0f, 0) || Player_PassedFrameScaled(em, 174.0f, 0)) {
            EffKutku17_Create(em, 0x10);
            break;
        }
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
    case 0x443:
        if (Player_PassedFrameScaled(em, 34.0f, 0) != 0) {
            StageObj_K14_Create(em, 3, 0, 1.0f);
        }
        if (Player_PassedFrameScaled(em, 70.0f, 0) != 0) {
            StageObj_K14_Create(em, 2, 0, 1.0f);
        }
        if ((EmMotion_ReachedFrame(em, 0, 96.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 120.0f) == 0) && ((gScene.frame % 6) == 0)) {
            StageObj_K14_Create(em, 0x12, 0, 1.0f);
        }
        if ((em->rage != 0) && ((gScene.frame % 19) == 0)) {
            StageObj_K14_Create(em, 0x19, (Rand_Next(1) & 1) + 2, 1.0f);
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
    case 0x44C:
        if (Player_PassedFrameScaled(em, 118.0f, 0) || Player_PassedFrameScaled(em, 164.0f, 0) || Player_PassedFrameScaled(em, 204.0f, 0)) {
            EffKutku17_Create(em, 0x23);
        }
        if (em->rage != 0) {
            if (EmMotion_InFrameRange(em, 0, 54.0f, 250.0f) || EmMotion_InFrameRange(em, 0, 282.0f, 296.0f)) {
                if (!(gScene.frame & 3)) {
                    StageObj_K14_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
                    break;
                }
            }
        }
        break;
    case 0x44D:
        if (Player_PassedFrameScaled(em, 52.0f, 0) != 0) {
            StageObj_K14_Create(em, 1, 0x80, 1.0f);
        }
        if (em->rage != 0) {
            if (EmMotion_InFrameRange(em, 0, 10.0f, 68.0f) || EmMotion_InFrameRange(em, 0, 84.0f, 96.0f)) {
                if (!(gScene.frame & 3)) {
                    StageObj_K14_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
                    break;
                }
            }
        }
        break;
    case 0x44E:
        if ((Player_PassedFrameScaled(em, 32.0f, 0) != 0) || (Player_PassedFrameScaled(em, 54.0f, 0) != 0) || (Player_PassedFrameScaled(em, 74.0f, 0) != 0) || (Player_PassedFrameScaled(em, 92.0f, 0) != 0)) {
            EffKutku17_Create(em, 0x18);
        }
        if (em->rage != 0) {
            if ((EmMotion_InFrameRange(em, 0, 10.0f, 20.0f) != 0) && !(gScene.frame & 3)) {
                StageObj_K14_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
            }
            if ((Player_PassedFrameScaled(em, 30.0f, 0) != 0) || (Player_PassedFrameScaled(em, 54.0f, 0) != 0) || (Player_PassedFrameScaled(em, 72.0f, 0) != 0) || (Player_PassedFrameScaled(em, 92.0f, 0) != 0)) {
                StageObj_K14_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
                break;
            }
        }
        break;
    case 0x450:
        if (Player_PassedFrameScaled(em, 30.0f, 0) != 0) {
            EffKutku17_Create(em, 0x17);
        }
        if ((Player_PassedFrameScaled(em, 18.0f, 0) != 0) || (Player_PassedFrameScaled(em, 60.0f, 0) != 0)) {
            Effect0D_SpawnForEmScaled(em, 0x1A, 3, 4.0f);
        }
        if ((Player_PassedFrameScaled(em, 38.0f, 0) != 0) || (Player_PassedFrameScaled(em, 80.0f, 0) != 0)) {
            Effect0D_SpawnForEmScaled(em, 0x14, 3, 4.0f);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 54.0f, 112.0f) != 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
            break;
        }
        break;
    case 0x451:
        if (Player_PassedFrameScaled(em, 78.0f, 0) != 0) {
            if (em->type == 6 || em->type == 38) {
            EffEm08_CreateAtBone(em, 0x22, 3, 0, 0xF556, 0xEF4B);
            } else if (em->type == 39) {
            EffEm08_CreateAtBone(em, 0x22, 2, 1, 0xF556, 0xEF4B);
            } else {
            EffEm08_CreateAtBone(em, 0x22, 2, 0, 0xF556, 0xEF4B);
            }
        }
        if (Player_PassedFrameScaled(em, 150.0f, 0) != 0) {
            if (em->type == 6 || em->type == 38) {
            EffEm08_CreateAtBone(em, 0x22, 3, 0, 0xF556, 0x199A);
            } else if (em->type == 39) {
            EffEm08_CreateAtBone(em, 0x22, 2, 1, 0xF556, 0x199A);
            } else {
            EffEm08_CreateAtBone(em, 0x22, 2, 0, 0xF556, 0x199A);
            }
        }
        if (Player_PassedFrameScaled(em, 240.0f, 0) != 0) {
            if (em->type == 6 || em->type == 38) {
            EffEm08_CreateAtBone(em, 0x22, 3, 0, 0xF1C8, 0xDC73);
            } else if (em->type == 39) {
            EffEm08_CreateAtBone(em, 0x22, 2, 1, 0xF1C8, 0xDC73);
            } else {
            EffEm08_CreateAtBone(em, 0x22, 2, 0, 0xF1C8, 0xDC73);
            }
        }
        if (Player_PassedFrameScaled(em, 296.0f, 0) != 0) {
            if (em->type == 6 || em->type == 38) {
            EffEm08_CreateAtBone(em, 0x22, 3, 0, 0xF940, 0x1C72);
            } else if (em->type == 39) {
            EffEm08_CreateAtBone(em, 0x22, 2, 1, 0xF940, 0x1C72);
            } else {
            EffEm08_CreateAtBone(em, 0x22, 2, 0, 0xF940, 0x1C72);
            }
        }
        break;
    case 0x452:
        if (Player_PassedFrameScaled(em, 20.0f, 0) != 0) {
            EffKutku17_Create(em, 0x12);
        }
        if ((Player_PassedFrameScaled(em, 50.0f, 0) != 0) || (Player_PassedFrameScaled(em, 76.0f, 0) != 0) || (Player_PassedFrameScaled(em, 96.0f, 0) != 0) || (Player_PassedFrameScaled(em, 116.0f, 0) != 0)) {
            EffKutku17_Create(em, 0x18);
        }
        if (Player_PassedFrameScaled(em, 20.0f, 0) != 0) {
            StageObj_K14_Create(em, 3, 0, 1.0f);
        }
        if (em->rage != 0) {
            if ((EmMotion_InFrameRange(em, 0, 24.0f, 42.0f) != 0) && !(gScene.frame & 3)) {
                StageObj_K14_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
            }
            if ((Player_PassedFrameScaled(em, 54.0f, 0) != 0) || (Player_PassedFrameScaled(em, 80.0f, 0) != 0) || (Player_PassedFrameScaled(em, 100.0f, 0) != 0) || (Player_PassedFrameScaled(em, 122.0f, 0) != 0)) {
                StageObj_K14_Create(em, 0x1A, (s16) (Rand_Next(1) & 1), 1.0f);
                break;
            }
        }
        break;
    case 0x453:
        if (work->unk01B != 0) {
            *(s8 *) ((u8 *) work + 8) = 1;
            if (Player_PassedFrameScaled(em, 40.0f, 0) != 0) {
                Mtx_GetTrans(v0, em->model->bones[35].world);
                StageObj_K0E_CreateOwned(v0, 5, 1.0f, em);
                EffKutku17_Create2(em, 0x16);
                break;
            }
        }
        break;
    case 0x455:
        if (Player_PassedFrameScaled(em, 20.0f, 0) != 0) {
            if (em->state == 0x20) {
                EffKutku17_Create(em, 0x25);
            } else {
                EffKutku17_Create(em, 0xE);
            }
        }
        if (Player_PassedFrameScaled(em, 20.0f, 0) || Player_PassedFrameScaled(em, 92.0f, 0) || Player_PassedFrameScaled(em, 152.0f, 0)) {
            Effect0D_SpawnForEmScaled(em, 2, 7, 3.0f);
            break;
        }
        break;
    default:
        EmKutku_MotionEffectsDefault(em);
        break;
    case 0x3E9:
    case 0x3F4:
    case 0x405:
    case 0x408:
    case 0x409:
    case 0x40B:
    case 0x40D:
    case 0x40F:
    case 0x414:
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
    case 0x43F:
    case 0x449:
    case 0x44A:
    case 0x454:
        break;
    }
}

void EmKutku_UpdateMotionEffects(Em *em)
{
    EmFlyerWork *w = (EmFlyerWork *)&em->work;

    switch (w->fxStep) {
    case 0:
        w->fxStep++;
        break;
    case 1:
        EmKutku_MotionEffects(em, (EmKutkuFxWork *) w);
        break;
    }
}

void EmKutku_SpawnK14Type0B(Em *em)
{
    StageObj_K14_Create((Em *) em, 0xB, 0, 1.0f);
}

void EmKutku_SpawnK14Type0AEvery5(Em *em)
{
    if (((s32) gScene.frame % 5) == 0) {
        StageObj_K14_Create((Em *) em, 0xA, 0, 1.0f);
    }
}

s32 EmKutku_ReachedTarget(Em *em)
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

void EmKutku_State3_End(Em *em, EmWork *work)
{
    EmKutku_EndState(em, 0, 0);
}

void EmKutku_EndSwoop(Em *em, s32 unused)
{
    EmKutku_EndState2(em, 0x1);
}

void EmKutku_SetMeshSetAlpha(Em *em, s32 kind, EmKutkuMeshSet *sets)
{
    ModelMat *meshes;
    s32 i;
    u8 *w;
    ModelMat *m;

    meshes = em->model->mats;
    w = &em->work;
    for (i = 0; i < sets[kind].count; i++) {
        m = &meshes[sets[kind].mesh[i]];
        m->alpha = em->alpha;
        switch (kind) {
        case 0:
            switch (i) {
            case 1:
                m->alpha = 0.0f;
                break;
            case 3:
                if (w[0x1B] == 0) {
                    m->alpha = 0.0f;
                } else {
                    m->alpha = em->alpha;
                }
                break;
            }
            break;
        case 2:
            switch (i) {
            case 5:
                if (w[0x1B] != 0) {
                    m->alpha = 0.0f;
                } else {
                    m->alpha = em->alpha;
                }
                break;
            case 6:
                m->alpha = 0.0f;
                break;
            }
            break;
        case 3:
            switch (i) {
            case 0:
                if (w[0x1B] == 0 || w[8] == 0) {
                    m->alpha = 0.0f;
                } else {
                    m->alpha = em->alpha;
                }
                break;
            case 5:
                if (em->rage != 0 && em->stateGroup != 5) {
                    m->alpha = 1.0f;
                } else {
                    m->alpha = 0.0f;
                }
                break;
            }
            break;
        }
        Gs_SetState((i + 0x3A) & 0xFF, (s32) m);
    }
}

s32 func_00786E50(Em *em)
{
    u8 stateGroup;
    u8 type;

    type = em->type;
    if ((type == 0x14) || (type == 0x27)) {
        stateGroup = em->stateGroup;
        if ((stateGroup != 3) || (em->state != 4)) {
            if (stateGroup == 3) {
                if (em->state != 9) {
                    goto block_7;
                }
                goto block_10;
            }
block_7:
            if ((stateGroup == 4) && (em->state == 0x19)) {
                goto block_10;
            }
            goto block_11;
        }
block_10:
        return 1;
    }
block_11:
    return 0;
}

void EmKutku_TableNop(void)
{
}

void EmKutku_SetGroup0State(Em *em, u16 state, u8 mode)
{
    em->motionSpeed = 1.0f;
    switch (state) {
    case 1:
        if (em->combat == 1) {
            state = 0x11;
        }
        break;
    case 35:
        Em_StartRage(em);
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

void EmKutku_SetGroup1State(Em *em, u16 state, u8 mode)
{
    em->stance = 0;
    em->motionSpeed = 1.0f;
    switch (state) {
    case 0:
        if (em->type == 0x14 || em->type == 0x27) {
            if (em->hp < (s16) (0.2f * em->hpMax)) {
                state = 6;
                goto state6;
            }
        } else {
            if (em->hp < (s16) (0.2f * em->hpMax)) {
                state = 6;
                goto state6;
            }
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
        if (em->curGoalKind == 0) {
            em->hasGoal = 0;
            em->timer = (em->stateRand % 120 + 180) / 2;
        } else {
            Em_SetSpeedScaled(em, 2, 1000.0f, 500.0f);
            em->timer = (s32) em->goalDist / 7;
        }
        break;
    case 6:
    state6:
        if (em->curGoalKind == 0) {
            em->hasGoal = 0;
            em->timer = (em->stateRand % 120 + 180) / 2;
        } else {
            Em_SetSpeedScaled(em, 6, em->goalDist, 0.0f);
            em->timer = (s32) em->goalDist / 7;
        }
        if (em->timer > 300) {
            em->timer = 300;
        }
        break;
    case 5:
    case 7:
    case 9:
        break;
    }
    Em_SetStateSync(em, 1, state, mode);
}

void EmKutku_SetGroup2State(Em *em, u16 state, u8 mode)
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
            EmKutku_ChangeState(em, 2, 0, 1);
            return;
        }
        break;
    case 9:
        if (em->remote == 0) {
            em->goalKind = 2;
            if (em->areaSeqActive != 0) {
                em->goalArg = 3;
                if (Area_BaseId(gScene.map) == 6) {
                    if (em->type == 6 || em->type == 0x26) {
                        em->goalNo = 1;
                    } else {
                        em->goalNo = 0;
                    }
                } else {
                    em->goalNo = 0;
                }
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

void EmKutku_SetGroup3State(Em *em, u16 state, u8 mode)
{
    EmFlyerWork *w = (EmFlyerWork *)&em->work;
    f32 pos[3];
    f32 f;

    em->motionSpeed = 1.0f;
    switch (state) {
    case 0:
    case 1:
    case 10:
    case 11:
    case 23:
    case 24:
    case 25:
    case 26:
    case 27:
    case 28:
        Em_SetTargetAngle(em);
        break;
    case 4:
    case 9:
        em->timer = 1800;
        break;
    case 2:
        Em_SetTargetAngle(em);
        if (em->targetNo == -1) {
            w->runDist = 2500.0f;
            break;
        }
        em->target = &gPlayers[em->targetNo];
        Em_GetTargetPos(em, em->targetNo, pos);
        Vec_Set(em->goal, pos[0], pos[1], pos[2]);
        f = Vec_DistanceXZ(&em->posX, em->goal);
        w->runDist = f;
        if (f > 5000.0f) {
            w->runDist = 5000.0f;
        } else if (f < 2500.0f) {
            w->runDist = 2500.0f;
        }
        break;
    case 8:
    case 21:
        if (em->remote == 0) {
            EmKutku_SetEscapePoint(em);
            em->timer = 1.5f * Vec_DistanceXZ(&em->posX, em->goal) / 50.0f;
        }
        break;
    case 18:
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
    case 3:
    case 5:
    case 6:
    case 7:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 19:
    case 20:
    case 22:
    case 29:
    case 30:
    case 31:
    case 32:
        break;
    }
    Em_SetStateSync(em, 3, state, mode);
}

void EmKutku_SetGroup4State(Em *em, u16 state, u8 mode)
{
    EmFlyerWork *w = (EmFlyerWork *)&em->work;

    em->motionSpeed = 1.0f;
    switch (state) {
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
        w->partWhole = 0;
        Em_BreakPart(em, 1);
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

void EmKutku_ChangeState(Em *em, u16 group, u16 state, u8 mode)
{
    EmFlyerWork *w = (EmFlyerWork *)&em->work;

    if (em->remote == 0) {
        Em_TargetPlayerInRange(em);
    } else if (mode == 3 || mode == 4) {
        return;
    }
    w->unk008 = 0;
    if (w->minePoint != -1) {
        MinePoint_Remove(w->minePoint);
        w->minePoint = -1;
    }
    Em_LoadReactionSet(em, 0);
    if (em->remote == 0 && group != 5 && group != 4 &&
        ((s16) Player_IsState(em, 3, 9) != 0 || (s16) Player_IsState(em, 3, 4) != 0)) {
        group = 3;
        state = 7;
    }
    switch (group) {
    case 0:
        EmKutku_SetGroup0State(em, state, mode);
        break;
    case 1:
        EmKutku_SetGroup1State(em, state, mode);
        break;
    case 2:
        EmKutku_SetGroup2State(em, state, mode);
        break;
    case 3:
        EmKutku_SetGroup3State(em, state, mode);
        break;
    case 4:
        EmKutku_SetGroup4State(em, state, mode);
        break;
    case 5:
        if (state == 3 && QuestEm_CanAppear(em) == 1) {
            QuestEm_Appear(em);
        }
    case 6:
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        break;
    case 8:
        if (state == 0x98) {
            Versus_MarkDefeat(em);
        }
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        break;
    case 7:
        break;
    }
}

void EmKutku_SetEscapePoint(Em *em)
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
        EmKutku_ChangeState(em, 2, 6, 1);
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

void EmKutku_StepPitch(Em *em, s32 mode, f32 height)
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

void EmKutku_MoveByVelYCurve(Em *em, s32 kind)
{
    EmWork *work = (EmWork *) &em->work;
    EmCurveKey *tbl;
    EmCurveKey *p;
    f32 frame;
    f32 t;
    s32 i;

    tbl = EmKutku_MoveVelYCurves[kind];
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

void EmKutku_StartCurves(Em *em, u8 kind)
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

static s32 EmKutku_CurveNone(Em *em, EmWork *work)
{
    return 0;
}


extern EmCurveKey *EmKutku_VelYCurves[];

/* The value of curve work[0x4F] at the frame in the work area (a table of keys): sets
 * em->velY from the key pair around the frame, 2 at the table's end. */
static u8 EmKutku_CurveVelY(Em *em, EmWork *work)
{
    EmFlyerWork *w = (EmFlyerWork *) work;
    EmCurveKey *p;
    f32 frame;
    f32 t;
    s32 i;
    EmCurveKey *tbl;
    u8 ret;

    tbl = EmKutku_VelYCurves[((EmFlyerWork *)work)->curveKind];
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

extern EmCurveKey *EmKutku_SpeedCurves[];

/* The value of curve work[0x4F] at the frame in the work area: sets em->velZ from the key
 * pair around the frame, 4 at the table's end. */
static u8 EmKutku_CurveSpeed(Em *em, EmWork *work)
{
    EmFlyerWork *w = (EmFlyerWork *) work;
    EmCurveKey *p;
    f32 frame;
    f32 t;
    s32 i;
    EmCurveKey *tbl;
    u8 ret;

    tbl = EmKutku_SpeedCurves[((EmFlyerWork *)work)->curveKind];
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
u8 EmKutku_UpdateCurves(Em *em)
{
    EmFlyerWork *w = (EmFlyerWork *)&em->work;
    u8 ret = 0;

    if (w->curveNone != 0) {
        ret = EmKutku_CurveNone(em, (EmWork *) w);
    }
    if (w->curveVelY != 0) {
        ret |= EmKutku_CurveVelY(em, (EmWork *) w);
    }
    if (w->curveSpeed != 0) {
        ret |= EmKutku_CurveSpeed(em, (EmWork *) w);
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

void EmPlesioth_MotionSounds(Em *em, EmWork *work)
{
    s16 prevMotion;

    prevMotion = work->prevMotion;
    switch (prevMotion) {
    case 0x3E9:
        Em_PlaySeAtBoneOnFrame(em, 0x40, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xC8, Sound_RandomSe(0x21, 2, 0x21, 2), 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x19A, Sound_RandomSe(0x21, 2, 0x21, 2), 0x23);
        break;
    case 0x3EB:
        Em_PlaySeAtBoneOnFrame(em, 0x34, 1, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x74, 1, 0x1A);
        break;
    case 0x3EC:
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x56, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 6, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 6, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 6, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 6, 0x1A);
        break;
    case 0x3ED:
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 1, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 1, 0x1A);
        break;
    case 0x3EE:
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 1, 0x14);
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
    case 0x3FA:
        Em_PlaySeAtBoneOnFrame(em, 0x28, 8, 0x1A);
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
        Em_PlaySeAtBoneOnFrame(em, 0xE, 9, 0);
        break;
    case 0x401:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x1F, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 0x22, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xAA, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xD6, 0x22, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x114, 0x1F, 0x23);
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
        Em_PlaySeAtBoneOnFrame(em, 4, 0x1F, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0x55, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x7E, 3, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0xB6, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0x16, 0);
        break;
    case 0x40A:
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0x57, 0x23);
        break;
    case 0x40C:
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0x24, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0x22, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 0, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0x15, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0xC, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0xC, 6);
        Em_PlaySeAtBoneOnFrame(em, 0xA6, 0, 0x1A);
        break;
    case 0x40D:
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x20, 0x23);
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
    case 0x413:
        Em_PlaySeAtBoneOnFrame(em, 8, 0x23, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0xF, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x14, 0x2A);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 1, 0x14);
        break;
    case 0x417:
        Em_PlaySeAtBoneOnFrame(em, 2, 0x12, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 0x18, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0x28, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 0x51, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 0xD, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0xB4, 0x13, 6);
        Em_PlaySeAtBoneOnFrame(em, 0xBB, 0x13, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0xF0, 0x12, 0);
        break;
    case 0x418:
        Em_PlaySeAtBoneOnFrame(em, 2, 0x22, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 0x22, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 0x22, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x72, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 1, 0x14);
        break;
    case 0x424:
        Em_PlaySeAtBoneOnFrame(em, 6, 0x53, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 0x13, 0x2A);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 6, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0x11, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x74, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x94, 3, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 0xC, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0xD, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0xC2, 3, 0x1A);
        break;
    case 0x425:
        Em_PlaySeAtBoneOnFrame(em, 6, 0x53, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x12, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 6, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 4, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 3, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0xAC, 3, 0x1A);
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
        break;
    case 0x428:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x52, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x78, 3, 0x1A);
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
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x54, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 8, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x14, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x30, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x2E, 0x23);
        break;
    case 0x42D:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x52, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x13, 0x2A);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 9, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0x11, 0);
        break;
    case 0x42E:
        Em_PlaySeAtBoneOnFrame(em, 0x20, 0x1D, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 0x1C, 0);
        break;
    case 0x42F:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0);
        break;
    case 0x432:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x52, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x13, 0x2A);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 9, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0x11, 0);
        break;
    case 0x433:
        Em_PlaySeAtBoneOnFrame(em, 0x20, 0x1D, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 0x1C, 0);
        break;
    case 0x434:
        Em_PlaySeAtBoneOnFrame(em, 0x10, 0x5A, 0x23);
        break;
    case 0x447:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x10, 0);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x12, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 9, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x27, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x52, 0x23);
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
    case 0x44D:
        Em_PlaySeAtBoneOnFrame(em, 4, 1, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0x2A);
        break;
    case 0x44E:
        break;
    case 0x44F:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x24, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 0x15, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0x15, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0x17, 0x2A);
        break;
    case 0x450:
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0x56, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x15, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 0x30, 0x2A);
        break;
    case 0x451:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x15, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x30, 0x2A);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0x30, 0x2A);
        break;
    case 0x452:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0);
        break;
    case 0x453:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0);
        break;
    case 0x454:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x56, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0x30, 0x2A);
        break;
    case 0x457:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x17, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x53, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x7A, 0x16, 0);
        break;
    case 0x458:
        Em_PlaySeAtBoneOnFrame(em, 0x74, 0x1A, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x28, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 0x51, 0x23);
        break;
    case 0x459:
        break;
    case 0x45A:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0x2A);
        break;
    case 0x45B:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0x2A);
        break;
    case 0x45C:
        Em_PlaySeAtBoneOnFrame(em, 0x74, 0x18, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x28, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 0x51, 0x23);
        break;
    case 0x45D:
        Em_PlaySeAtBoneOnFrame(em, 0x74, 0x18, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x28, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 0x51, 0x23);
        break;
    case 0x45E:
        break;
    case 0x45F:
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0x27, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 8, 0xD, 0xC);
        break;
    case 0x460:
        Em_PlaySeAtBoneOnFrame(em, 8, 0x33, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x12, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x15, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x14, 0);
        break;
    case 0x461:
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0x10, 0);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 9, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 8, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0x12, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x20, 0x23);
        break;
    case 0x462:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x35, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x35, 0);
        break;
    case 0x464:
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0x12, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0x15, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x36, 0, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 4, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 1);
        break;
    case 0x463:
        Em_PlaySeAtBoneOnFrame(em, 8, 0x33, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x12, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x15, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x14, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x36, 9, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0x1D, 0);
        break;
    case 0x466:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x53, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x5A, 0x30, 0x2A);
        break;
    case 0x467:
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x54, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 8, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x14, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x2E, 0x23);
        break;
    case 0x468:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x2C, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x10, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 9, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0x23);
        break;
    case 0x469:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x52, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x10, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 9, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0x23);
        break;
    case 0x46A:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x86, 0x17, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x86, 0x16, 0x23);
        break;
    case 0x46B:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x17, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x6E, 0x15, 0x23);
        break;
    case 0x46C:
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0x17, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0x16, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0x16, 0x22);
        break;
    case 0x46D:
        Em_PlaySeAtBoneOnFrame(em, 0xA, 6, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x1F, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x12, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x15, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x14, 0);
        break;
    case 0x46E:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x34, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x30, 0);
        break;
    case 0x46F:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x34, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x30, 0);
        break;
    case 0x470:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x12, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 3, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x6E, 4, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0xD8, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x108, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x15, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 9, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x1F, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x54, 0x16, 0);
        break;
    case 0x471:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x57, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x122, 0x55, 0x23);
        break;
    case 0x472:
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x53, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x78, 0x52, 0x23);
        
        break;
    }
}

/* em_kutku's .data (0x8C5790-0x8C6070) */
extern EmCurveKey EmCephadrome_VelYCurves_3[];
u16 EmKutku_StealItems[16] = {
#include "game/008C5790.inc"
};
EmFlyParam EmKutku_FlyParam = {
#include "game/008C57B0.inc"
};
EmFlyParam EmKutku_FlyParam2 = {
#include "game/008C5810.inc"
};
EmTurnParams EmKutku_TurnParams[2] = {
#include "game/008C5870.inc"
};
u16 EmKutku_FxFrameBone[4][2] = {
#include "game/008C58B0.inc"
};
EmKutkuVec EmKutku_CarveThrowPt = {
#include "game/008C58C0.inc"
};
s32 func_0090A9C0(void); /* gm_sub */
void *EmKutku_Callbacks[8] = {
    EmKutku_SpawnEffects, EmKutku_SpawnEffects, EmKutku_TableNop, EmKutku_UpdateMotionEffects,
    func_0090A9C0, EmKutku_Reinit, NULL, NULL,
};
EmCurveKey EmKutku_MoveVelYCurves_0[8] = {
#include "game/008C58F0.inc"
};
EmCurveKey EmKutku_MoveVelYCurves_1[20] = {
#include "game/008C5930.inc"
};
EmCurveKey EmKutku_MoveVelYCurves_3[10] = {
#include "game/008C59D0.inc"
};
EmCurveKey EmKutku_MoveVelYCurves_5[6] = {
#include "game/008C5A20.inc"
};
EmCurveKey EmKutku_MoveVelYCurves_6[10] = {
#include "game/008C5A50.inc"
};
EmCurveKey *EmKutku_MoveVelYCurves[16] = {
    EmKutku_MoveVelYCurves_0, EmKutku_MoveVelYCurves_1, NULL, EmKutku_MoveVelYCurves_3,
    NULL, EmKutku_MoveVelYCurves_5, EmKutku_MoveVelYCurves_6, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
};
EmCurveKey EmKutku_VelYCurves_3[10] = {
#include "game/008C5AE0.inc"
};
EmCurveKey EmKutku_VelYCurves_5[14] = {
#include "game/008C5B30.inc"
};
EmCurveKey EmKutku_VelYCurves_8[8] = {
#include "game/008C5BA0.inc"
};
EmCurveKey EmKutku_VelYCurves_14[8] = {
#include "game/008C5BE0.inc"
};
EmCurveKey EmKutku_VelYCurves_9[18] = {
#include "game/008C5C20.inc"
};
EmCurveKey EmKutku_VelYCurves_10[14] = {
#include "game/008C5CB0.inc"
};
EmCurveKey EmKutku_VelYCurves_11[4] = {
#include "game/008C5D20.inc"
};
EmCurveKey EmKutku_VelYCurves_12[6] = {
#include "game/008C5D40.inc"
};
EmCurveKey EmKutku_VelYCurves_13[24] = {
#include "game/008C5D70.inc"
};
EmCurveKey *EmKutku_VelYCurves[16] = {
    NULL, NULL, (EmCurveKey *)EmCephadrome_VelYCurves_3, EmKutku_VelYCurves_3,
    NULL, EmKutku_VelYCurves_5, NULL, NULL,
    EmKutku_VelYCurves_8, EmKutku_VelYCurves_9, EmKutku_VelYCurves_10, EmKutku_VelYCurves_11,
    EmKutku_VelYCurves_12, EmKutku_VelYCurves_13, EmKutku_VelYCurves_14, NULL,
};
EmCurveKey EmKutku_SpeedCurves_5[14] = {
#include "game/008C5E70.inc"
};
EmCurveKey EmKutku_SpeedCurves_8[8] = {
#include "game/008C5EE0.inc"
};
EmCurveKey EmKutku_SpeedCurves_14[8] = {
#include "game/008C5F20.inc"
};
EmCurveKey EmKutku_SpeedCurves_9[14] = {
#include "game/008C5F60.inc"
};
EmCurveKey EmKutku_SpeedCurves_10[12] = {
#include "game/008C5FD0.inc"
};
EmCurveKey *EmKutku_SpeedCurves[16] = {
    NULL, NULL, NULL, NULL,
    NULL, EmKutku_SpeedCurves_5, NULL, NULL,
    EmKutku_SpeedCurves_8, EmKutku_SpeedCurves_9, EmKutku_SpeedCurves_10, NULL,
    NULL, NULL, EmKutku_SpeedCurves_14, NULL,
};
