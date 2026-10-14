/*
 * Monster family: the dromes: Velocidrome, Gendrome and Iodrome (kinds 0x1B, 0x1C, 0x1F).
 * The code from the family's setup function (0x7366B0, called by the per-kind
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
void QuestEm_Hunted(Em *em);
s8 EmMine_Create(Em *em, s32 idx);
s32 EmMine_Update(Em *em);
void EmMine_Remove(Em *em);
s32 Versus_DefeatCamera(Em *pl);
void Versus_RouteEmByTeam(Em *em);
void Effect0D_SpawnForEm(Em *em, s32 a, s32 b);

/* prototypes */
void EmDrome_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void EmKirin_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void Em_BeginStatus1(Em *em);
void Em_EndStatus5(Em *em);
void Em_PlaySeAtBoneOnFrame(Em *em, s32 frame, s32 se, s32 bone);
void Em_ResetPartHp(Em *em, u32 i);
void Em_SetLayerMotion(Em *em, s32 motion, s32 blend, s32 frame, s32 layer);
void Em_SetMotion(Em *em, s32 motion, s32 a, s32 b);
u16 Math_AngleXZ(f32 *from, f32 *to);
void Player_ClearFlag(void *, u32 flag);
void Player_SetFlag(void *obj, u32 flag);
void QuestEm_Captured(Em *em);
ObjAttach *Eff02_Create(Em *em, s8 kind);
void EffEm08_CreateAtBone(Em *, s32, s32, s32, s32, s32);
void EffDrome15_Create(Em *, s8);
void Em_MoveRotatedAccel(Em *, void *);
void Em_SetStateRand(Em *em, s32 arg1, s32 arg2);
/* unit's own declaration: with the definition's void (Em *em, s32 group, s32 state, u8 mode) (game/unk_701810.c), EmDrome_ChangeState: 47 of 148 words differ, EmDrome_SetGroup0State: 1 of 146 words differ ... */
void Em_SetStateSync(Em *em, u16 group, u16 state, u8 mode);
void Em_SpawnEffect06Bone(Em *em, s32 a);
void Em_SetFlag6AE(Em *em, u8 a);
void Em_UpdatePendingScript(Em *em);
void Em_ClearFlag6AE(Em *em);
void Em_ClearPoison(Em *em);
void Em_SetHold2(Em *em);
void Em_SetCaptured(Em *em, s32 a);
void Em_PlaySe6AtBoneInFrames(Em *em, s32 frame, s32 frame2, s32 se, s32 bone, s32 vol, s32 a);
void Em_PlaySe6AtBoneOnFrame(Em *em, s32 frame, s32 se, s32 bone);
s32 Em_StepToGoal(Em *em, u16 step, u8 flags);
u8 Em_TurnToGoal(Em *em, u16 step, u8 flags);
void Em_StartTurnToTarget(Em *em, EmTurnParams *t);
s32 Em_TurnStep(Em *em, EmTurnParams *t);
u8 Em_UpdateDamage(Em *em, u8 *out);
void Em_RestartScript(Em *em);
void EmDrome_Reset(Em *em);
void EmDrome_RunGroup0(Em *em, EmWork *work);
void EmDrome_State1_01(Em *, EmWork *);
void EmDrome_RunGroup3(Em *em, EmWork *work);
void EmDrome_State5_00_Hunted(Em *, EmWork *);
void EmDrome_State5_01_Hunted(Em *em, EmWork *work);
void EmDrome_State5_02_Hunted(Em *em, EmWork *work);
void EmDrome_State5_03_Hunted(Em *em, EmWork *work);
void EmDrome_State5_04_Appear(Em *, EmWork *);
void EmDrome_State6_02(Em *, EmWork *);
void EmDrome_State8_00_CarveStart(Em *, EmWork *);
void EmDrome_MotionEffects(Em *, EmWork *);
void EmDrome_StartCurves(Em *em, u8 kind);
u8 EmDrome_UpdateCurves(Em *em);
void EmKind1D_RunGroup5(Em *);
void EmKind1D_RunState(Em *em);
void EmKirin_State5_00_Hunted(Em *, EmWork *);
void EmKirin_State6_00(Em *);
void EmKirin_EndState(Em *em);
void EmKirin_MotionEffects(Em *, EmWork *);
void EmFxSpark_SpawnTimed(Em *em, u8 k, s32 n, f32 f);
void EmFxTri_Spawn(Em *em, s16 kind);
void StageObj_Stun_CreateEm(Em *em, u8 k, s16 v, s16 t, f32 y, f32 scale);
s32 Sound_RandomSe(s32 id, s16 lo, s32 alt, s16 n);
s32 Player_PassedFrameScaled(void *p, f32 frame, s32 layer);
s32 EmMotion_ReachedFrame(void *obj, s32 a, f32 frame);
f32 *Mtx_FromAnglesXYZ(s32 *angles, f32 *m);
void Vec_Set(f32 *out, f32 x, f32 y, f32 z);
s32 Angle_TurnToward(s32 *cur, s32 *target, s32 step);
s32 ObjListA70_Add(EmObj *obj);
void ObjListA70_Remove(EmObj *obj);
s32 Versus_BothDefeated(void);
s32 QuestEm_CanAppear(Em *em);
void Player_ClearVelocity(void *);
void Effect01_Spawn(void *obj, s16 type);
extern EmTurnParams EmDrome_TurnParams[2];
void Em_InitParts(Em *em);
void Em_SetMotionRow(Em *em, s32 row, s32 arg2, s32 arg3);
s32 Em_ComputeSize(Em *em, s16 idx);
u16 Area_DayId(u16 id);
void EmKirin_State4_02(Em *em, EmWork *work, s16 variant);
void EmDrome_SetGroup0State(Em *em, u16 state, u8 mode);
void EmDrome_SetGroup1State(Em *em, u16 state, u8 mode);
void EmDrome_SetGroup2State(Em *em, u16 state, u8 mode);
void EmDrome_SetGroup3State(Em *em, u16 state, u8 mode);
s32 Game_IsOnline(void);
void Em_SetSpeedScaled(Em *em, u16 flags, f32 x, f32 y);
void Em_StartRage(Em *em);
void Em_StartPendingRage(Em *em);
void Em_BeginStatus2(Em *em);
void Em_EndStatus1(Em *em);
void Em_EndStatus2(Em *em);
void Em_EndStatus4(Em *em);
void Em_TargetPlayerInRange(Em *);
void Versus_MarkDefeat(Em *);
s8 QuestEm_Appear(Em *em);
void Em_ResetToBasePose(Em *em);
void Em_BreakPart(Em *em, s32 part);
void Em_EndAllStatus(Em *em);
void Em_BeginStatus4(Em *em);
void Em_BeginStatus5(Em *em);
void Em_LoadReactionSet(Em *em, u8 k);
void Em_CheckRageByHp(Em *em, s16 n);
f32 Vec_Distance(f32 *a, f32 *b);
void Em_ComputeGoal(Em *em, f32 *target);
void Em_TypeSetup(Em *em);
f32 Vec_DistanceXZ(f32 *a, f32 *b);
void Em_SetTargetAngle(Em *em);
s32 Em_GetTargetPos(Em *em, u8 idx, f32 *out);
void Em_ResetNotice(Em *em);
void Em_Refill7F4(Em *em);
void Em_StartGoalAction(Em *em, s32 arg1);
void Em_ComputeGoal2(Em *em, f32 *target);
void Em_ClearPart(Em *em, u16 part);
void Em_AddHp(Em *em, s16 n);
void Em_Add7F0(Em *em, s32 n);
void Em_Add7EC(Em *em, s32 n);
void Em_Clear7F0State(Em *em);
void Em_Clear7ECState(Em *em);
s16 Player_IsState(void *obj, u8 state, u8 sub);
void MinePoint_Remove(s32);
f32 GmEm_ScaleBySize(Em *em, f32 v);
s32 Em_GetFloorY(Em *em, f32 *pos, f32 *out);
typedef struct EmDromeVec {
    f32 x, y, z;
} EmDromeVec;
extern EmDromeVec EmDrome_CarveThrowPt __attribute__((aligned(8)));
void StageObj_K25_Create(Player *p, Em *em, u8 k, s32 unused, f32 *pos);
void Em_SpawnEffect06Periodic(Em *em, s32 bone, f32 *pos, f32 scale);
void Em_PlaySe(Em *em, s32 se);
void Effect02_SpawnForOwner(void *owner, s32 ang, s32 type, s32 sub, f32 *pos, f32 scale);
void Effect0D_SpawnIfHere(void *o, f32 *pos, s32 type, f32 scale);
void StageObj_K0E_CreateScaled(f32 *pos, s16 k, f32 scale);
void Eff09_CreateAt(f32 *pos, s32 kind, u16 area, u8 arg3);
/* Per sub-kind (Em +0x1B) data of the Drome family. */
typedef struct EmDromeKindEntry {
    s16 part;           /* 0x00: the part flag set at the start (Em +0x4C8 + part) */
    s16 hp;             /* 0x02: 0 or less: 4000 */
    f32 scale;          /* 0x04 */
    s32 se;             /* 0x08: the sound at death */
} EmDromeKindEntry;
extern EmDromeKindEntry EmKind1D_Variants[];
void GmEm_ApplySize(Em *em);
s32 Save_TestEventFlag(s32 flag);
void GmEm_ResetSize(Em *em);
s32 Em_UpdateCombatTimer(Em *em);
void Em_RaiseTrigger80(Em *em);
void Em_HoldPosNoPlayers(Em *em, u16 a, u16 b);
void Em_SetStatus4Timer(Em *em);
void Em_SetStatus2Timer(Em *em);
void EmKirin_RunState(Em *em);
/* A quest appearance point: the position and the point the monster faces. */
typedef struct EmDromeAppearPoint {
    f32 pos[3];
    f32 target[3];
} EmDromeAppearPoint;
extern EmDromeAppearPoint EmDrome_AppearPt22;
extern EmDromeAppearPoint EmDrome_AppearPts26[2];
extern EmDromeAppearPoint EmDrome_AppearPts27[4];
void Effect0D_SpawnForEmScaled(Em *em, s32 a, s32 b, f32 scale);
u8 Em_SenseObjects(Em *em, EmObj *self);
s32 Em_ScriptFlagsFrom(Em *em, u8 mask);
void Em_CheckLowHp(Em *em, f32 ratio);
void Em_SetStatus5Timer(Em *em);
void Em_ResetKindTimer(Em *em);
void EmHermitaur_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void EmDrome_RunState(Em *em, s32 arg1);
/* end of prototypes */

/* Velocidrome's own meanings of the work area's bytes (EmWork, include/em.h, has the shared ones). */
typedef struct EmDromeWork {
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
    u8 unk054[0x05E - 0x054];
    u8 hunted;                  /* 0x05E: as EmWork's hunted */
    u8 unk05F[0x100 - 0x05F];
} EmDromeWork;
typedef char EmDromeWork_size_check[(sizeof(EmDromeWork) == sizeof(EmWork)) ? 1 : -1];

void EmDrome_Setup(Em *em)
{
}

void EmDrome_SpawnEffect01(u8 *em)
{
    Effect01_Spawn((Player *) em, 0);
}

void EmDrome_Reset(Em *em)
{
    EmDromeWork *w = (EmDromeWork *)&em->work;

    f32 fslot;
    s16 size;
    s32 dayId;
    u8 slotNo;

    if (gQuest.stage == 0) {
        dayId = Area_DayId(gScene.area) & 0xFFFF;
        switch (dayId) {
        case 15:
            em->posX = 9500.0f + (150.0f * (f32) (em->slotNo / 8));
            em->posY = 0.0f;
            em->posZ = 9200.0f + (150.0f * (f32) (em->slotNo & 7));
            break;
        case 13:
            em->posX = 5000.0f + (150.0f * (f32) (em->slotNo / 8));
            em->posY = 0.0f;
            em->posZ = 9000.0f + (150.0f * (f32) (em->slotNo & 7));
            break;
        case 34:
            em->posX = 9522.0f + (150.0f * (f32) (em->slotNo / 8));
            em->posY = 0.0f;
            em->posZ = 9559.0f + (150.0f * (f32) (em->slotNo & 7));
            break;
        default:
            slotNo = em->slotNo;
            fslot = (f32) (u32) slotNo;
            em->posX = 2400.0f + (150.0f * fslot);
            em->posY = 0.0f;
            em->posZ = 3600.0f;
            break;
        }
    }
    Em_SetMotionRow(em, 1, 0, 0);
    em->stance = 0;
    size = Em_ComputeSize(em, 0);
    em->hp = size;
    em->hpMax = size;
    em->scriptPending = 1;
    em->blind = 0;
    em->unk_InitFlag = 1;
    w->countdown = 0;
    w->hunted = 0;
    Em_InitParts(em);
}

void EmDrome_Start(Em *em)
{
    EmDrome_Reset(em);
    EmDrome_ChangeState(em, 0, 1, 0);
}

void EmDrome_Reinit(Em *em)
{
    EmDrome_Reset(em);
}

void EmDrome_EndState(Em *em, s32 unused1, s32 unused2)
{
    em->motionSpeed = 1.2f;
    em->stance = 0;
    em->lookAtTarget = 0;
    em->scriptPending = 1;
    EmDrome_ChangeState(em, 0, 1, 0);
}

void EmDrome_State0_00(void)
{
}

void EmDrome_State0_01(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
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

void EmDrome_State0_02(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x32, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDrome_State0_03(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x33, 0, 0);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            Vec_Set((f32 *) &work->moveAngBY, em->posX, em->posY, em->posZ);
            work->moveAngCY = 0x447A0000;
            ((EmDromeWork *)work)->sense.area = (s16) *(u8 *)&em->area;
            ((EmDromeWork *)work)->sense.kind = 0;
            ObjListA70_Add((EmObj *)&work->moveAngBY);
            work->countdown = 1;
        }
        if (em->motion[0].playing == 0) {
            if (work->countdown != 0) {
                ObjListA70_Remove((EmObj *)&work->moveAngBY);
                work->countdown = 0;
            }
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDrome_State0_04(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x34, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDrome_State0_05(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0xB, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDrome_State0_06(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x27, 0, 0);
        em->noReact = 1;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDrome_State0_07(Em *em, EmWork *work, s32 variant)
{
    Em_SetFlag6AE(em, 2U);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x2B, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x2C, 0, 0);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            if (variant == 0) {
                EmDrome_ChangeState(em, 0, 8, 4);
                break;
            }
            EmDrome_ChangeState(em, 0, 0xE, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            if (variant == 0) {
                EmDrome_ChangeState(em, 0, 8, 4);
                break;
            }
            EmDrome_ChangeState(em, 0, 0xE, 4);
        }
        break;
    }
}

void EmDrome_State0_08(Em *em, EmWork *work, s32 variant)
{
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x27, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->blind = 0;
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDrome_State0_09(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        if (em->motionId[0] != 0x412) {
            Em_SetMotion(em, 0x2A, 0, 0);
        }
        break;
    case 1:
        if (em->blindTimer == 0) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDrome_State0_0A(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0xB, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDrome_State0_0B(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0xC, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDrome_State0_0F(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x34, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDrome_State0_10(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x34, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDrome_State0_11(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x32, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDrome_State0_12(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x33, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDrome_RunGroup0(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmDrome_State0_00();
        break;
    case 1:
        EmDrome_State0_01(em);
        break;
    case 2:
        EmDrome_State0_02(em);
        break;
    case 3:
        EmDrome_State0_03(em, work);
        break;
    case 4:
        EmDrome_State0_04(em);
        break;
    case 5:
        EmDrome_State0_05(em);
        break;
    case 6:
        EmDrome_State0_06(em);
        break;
    case 7:
        EmDrome_State0_07(em, work, 0);
        break;
    case 8:
        EmDrome_State0_08(em, work, 0);
        break;
    case 9:
        EmDrome_State0_09(em);
        break;
    case 0xA:
        EmDrome_State0_0A(em);
        break;
    case 0xB:
        EmDrome_State0_0B(em);
        break;
    case 0xC:
        EmDrome_State0_01(em);
        break;
    case 0xD:
        EmDrome_State0_07(em, work, 1);
        break;
    case 0xE:
        EmDrome_State0_08(em, work, 1);
        break;
    case 0xF:
        EmDrome_State0_0F(em);
        break;
    case 0x10:
        EmDrome_State0_10(em);
        break;
    case 0x11:
        EmDrome_State0_11(em);
        break;
    case 0x12:
        EmDrome_State0_12(em);
        break;
    }
}

void EmDrome_State1_00(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_StartTurnToTarget(em, &EmDrome_TurnParams[0]);
        break;
    case 1:
        if (Em_TurnStep(em, &EmDrome_TurnParams[0]) == 1) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDrome_State1_01(Em *em, EmWork *work)
{
    f32 pos[3];

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 2, 0, 0);
        work->startX = em->posX;
        work->startZ = em->posZ;
        work->walkFrames = 0;
        break;
    case 1:
        work->walkFrames++;
        if (Em_StepToGoal(em, 0x80, 0) == 1) {
            if (em->remote == 0) {
                pos[0] = work->startX;
                pos[1] = em->posY;
                pos[2] = work->startZ;
                if (Vec_DistanceXZ(&em->posX, pos) < 100.0f && work->walkFrames > 0x3C) {
                    EmDrome_ChangeState(em, 2, 0, 4);
                    break;
                }
                EmDrome_EndState(em, 0, 0);
            } else {
                EmDrome_EndState(em, 0, 0);
            }
        }
        break;
    }
}

void EmDrome_State1_02(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 8, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDrome_State1_03(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 9, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDrome_State1_04(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0xA, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDrome_State1_05(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_StartTurnToTarget(em, &EmDrome_TurnParams[1]);
        break;
    case 1:
        if (Em_TurnStep(em, &EmDrome_TurnParams[1]) == 1) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDrome_RunGroup1(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmDrome_State1_00(em, work);
        break;
    case 1:
        EmDrome_State1_01(em, work);
        break;
    case 2:
        EmDrome_State1_02(em, work);
        break;
    case 3:
        EmDrome_State1_03(em, work);
        break;
    case 4:
        EmDrome_State1_04(em, work);
        break;
    case 5:
        EmDrome_State1_05(em, work);
        
    default:
        break;
    }
}

void EmDrome_State2_00(Em *em, EmWork *work)
{
    s32 ended;

    ended = 0;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 3, 0, 0);
        EmDrome_StartCurves(em, 2U);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 10.0f) != 0) {
            em->stance = 2;
            ended = EmDrome_UpdateCurves(em) & 0xFF;
        }
        if ((ended != 0) && (em->posY <= em->floorY)) {
            em->step++;
            em->stance = 0;
            Em_SetMotion(em, 5, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDrome_State2_01(Em *em, EmWork *work)
{
    s32 ended;

    ended = 0;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 4, 0, 0);
        EmDrome_StartCurves(em, 3U);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 22.0f) != 0) {
            em->stance = 2;
            ended = EmDrome_UpdateCurves(em) & 0xFF;
        }
        if ((ended != 0) && (em->posY <= em->floorY)) {
            em->step++;
            em->stance = 0;
            Em_SetMotion(em, 5, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDrome_RunGroup2(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmDrome_State2_00(em, work);
        break;
    case 1:
        EmDrome_State2_01(em, work);
        break;
    }
}

void EmDrome_State3_00(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x16, 0, 0);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            switch (em->type) {
            case 0x1B:
                EffDrome15_Create(em, 1);
                break;
            case 0x1C:
                EffDrome15_Create(em, 0xD);
                break;
            case 0x1F:
                EffDrome15_Create(em, 7);
                break;
            }
        }
        Em_TurnToGoal(em, 0x80, 0);
        if (em->motion[0].playing == 0) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDrome_State3_01(Em *em)
{
    s32 ended;
    u8 step;

    step = em->step;
    ended = 0;
    switch (step) {
    case 0:
        em->step = step + 1;
        em->stance = 0;
        Em_SetMotion(em, 0x17, 0, 0);
        EmDrome_StartCurves(em, 1U);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 24.0f) != 0) {
            em->stance = 2;
            ended = EmDrome_UpdateCurves(em) & 0xFF;
        }
        if ((em->hasGoal != 0) && (em->stance == 2)) {
            Em_TurnToGoal(em, 0x40, 0);
        }
        if ((ended != 0) && (em->posY <= em->floorY)) {
            em->step++;
            em->stance = 0;
            Em_SetMotion(em, 0x18, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDrome_State3_02(Em *em, EmWork *work)
{
    EmDromeWork *cw = (EmDromeWork *)work;
    u8 landed;

    landed = 0;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x14, 0, 0);
        Player_ClearVelocity(em);
        em->velY = 75.0f;
        em->accelY = -10.0f;
        em->velZ = 50.0f;
        em->accelZ = -1.5f;
        work->accelAngX = 0;
        work->accelAngZ = 0;
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
            case 0x1B:
                EffDrome15_Create(em, 3);
                break;
            case 0x1C:
                EffDrome15_Create(em, 8);
                break;
            case 0x1F:
                EffDrome15_Create(em, 9);
                break;
            }
        }
        if (EmMotion_ReachedFrame(em, 0, 22.0f) != 0) {
            em->stance = 2;
            work->accelAngY = em->rotY;
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
        if (landed && em->posY <= em->floorY) {
            em->step++;
            em->stance = 0;
            em->posY = em->floorY;
            Em_SetMotion(em, 0x15, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDrome_State3_03(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x19, 0, 0);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 12.0f, 0U) != 0) {
            switch (em->type) {
            case 0x1B:
                EffDrome15_Create(em, 5);
                break;
            case 0x1C:
                EffDrome15_Create(em, 0xA);
                break;
            case 0x1F:
                EffDrome15_Create(em, 0xB);
                break;
            }
        }
        if (em->motion[0].playing == 0) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDrome_State3_04(Em *em, EmWork *work)
{
    EmDromeWork *cw = (EmDromeWork *)work;
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
            case 0x1B:
                EffDrome15_Create(em, 3);
                break;
            case 0x1C:
                EffDrome15_Create(em, 8);
                break;
            case 0x1F:
                EffDrome15_Create(em, 9);
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
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDrome_State3_05(Em *em, EmWork *work)
{
    EmDromeWork *cw = (EmDromeWork *)work;
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
            case 0x1B:
                EffDrome15_Create(em, 3);
                break;
            case 0x1C:
                EffDrome15_Create(em, 8);
                break;
            case 0x1F:
                EffDrome15_Create(em, 9);
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
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDrome_State3_06(Em *em, EmWork *work, s32 variant)
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
                EffEm08_CreateAtBone(em, 0xF, 9, 1, 0xF8E5, 0);
            }
            break;
        case 1:
            if (Player_PassedFrameScaled(em, 80.0f, 0U) != 0) {
                EffEm08_CreateAtBone(em, 0xF, 9, 3, 0xF8E5, 0);
            }
            break;
        }
        if (em->motion[0].playing == 0) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDrome_State3_07(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x16, 0, 0);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            EffDrome15_Create(em, 6);
        }
        if (em->hasGoal != 0) {
            Em_TurnToGoal(em, 0x80, 0);
        }
        if (em->motion[0].playing == 0) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDrome_State3_08(Em *em, EmWork *work)
{
    EmDromeWork *cw = (EmDromeWork *)work;
    u8 landed;

    landed = 0;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x14, 0, 0);
        Player_ClearVelocity(em);
        em->velY = 75.0f;
        em->accelY = -10.0f;
        em->velZ = 50.0f;
        em->accelZ = -1.5f;
        work->accelAngX = 0;
        work->accelAngZ = 0;
        work->pitchStep = 0;
        work->turnSpeed = 0;
        work->rollSpeed = 0;
        work->unk048 = 0;
        work->unk04C = 0;
        cw->unk050 = 0;
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            EffDrome15_Create(em, 3);
        }
        if (EmMotion_ReachedFrame(em, 0, 22.0f) != 0) {
            em->stance = 2;
            work->accelAngY = em->rotY;
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
        if (landed && em->posY <= em->floorY) {
            em->step++;
            em->stance = 0;
            em->posY = em->floorY;
            Em_SetMotion(em, 0x15, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDrome_State3_09(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x19, 0, 0);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 12.0f, 0U) != 0) {
            EffDrome15_Create(em, 5);
        }
        if (em->motion[0].playing == 0) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDrome_State3_0A(Em *em, EmWork *work)
{
    EmDromeWork *cw = (EmDromeWork *)work;
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
            EffDrome15_Create(em, 3);
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
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDrome_State3_0B(Em *em, EmWork *work)
{
    EmDromeWork *cw = (EmDromeWork *)work;
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
            EffDrome15_Create(em, 3);
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
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDrome_State3_0C(Em *em, EmWork *work)
{
    EmDromeWork *cw = (EmDromeWork *)work;
    u8 landed;
    f32 d;

    landed = 0;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x14, 0, 0);
        Player_ClearVelocity(em);
        if (!(em->posY < em->goal[1])) {
            em->velY = 75.0f;
            em->accelY = -10.0f;
            em->velZ = 50.0f;
            em->accelZ = -1.5f;
        } else {
            d = Vec_DistanceXZ(&em->posX, em->goal);
            em->velZ = 100.0f;
            em->accelY = -10.0f;
            em->timer = d / em->velZ;
            em->velY = (1000.0f + em->goal[1] - em->posY) / (f32) em->timer - (em->accelY * (f32) em->timer) / 2.0f;
        }
        work->accelAngX = 0;
        work->accelAngZ = 0;
        work->pitchStep = 0;
        work->turnSpeed = 0;
        work->rollSpeed = 0;
        work->unk048 = 0;
        work->unk04C = 0;
        cw->unk050 = 0;
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            EffDrome15_Create(em, 3);
        }
        if (EmMotion_ReachedFrame(em, 0, 22.0f) != 0) {
            em->stance = 2;
            work->accelAngY = em->rotY;
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
        if (landed && em->posY <= em->floorY) {
            em->step++;
            em->stance = 0;
            em->posY = em->floorY;
            Em_SetMotion(em, 0x15, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDrome_State3_0D(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x16, 0, 0);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            EffDrome15_Create(em, 0xC);
        }
        if (em->hasGoal != 0) {
            Em_TurnToGoal(em, 0x80, 0);
        }
        if (em->motion[0].playing == 0) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDrome_RunGroup3(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmDrome_State3_00(em, work);
        break;
    case 1:
        EmDrome_State3_01(em);
        break;
    case 2:
        EmDrome_State3_02(em, work);
        break;
    case 3:
        EmDrome_State3_03(em, work);
        break;
    case 4:
        EmDrome_State3_04(em, work);
        break;
    case 5:
        EmDrome_State3_05(em, work);
        break;
    case 6:
        EmDrome_State3_06(em, work, 0);
        break;
    case 7:
        EmDrome_State3_07(em);
        break;
    case 8:
        EmDrome_State3_08(em, work);
        break;
    case 9:
        EmDrome_State3_09(em);
        break;
    case 0xA:
        EmDrome_State3_0A(em, work);
        break;
    case 0xB:
        EmDrome_State3_0B(em, work);
        break;
    case 0xC:
        EmDrome_State3_0C(em, work);
        break;
    case 0xD:
        EmDrome_State3_0D(em);
        break;
    case 0xE:
        EmDrome_State3_06(em, work, 1);
        break;
    }
}

void EmDrome_State4_00(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x1E, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDrome_State4_01(Em *em, EmWork *work)
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
            em->step++;
            EmDrome_ChangeState(em, 0, 6, 0);
        }
        break;
    }
}

void EmDrome_State4_02(Em *em, EmWork *work)
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
            em->step++;
            EmDrome_ChangeState(em, 0, 6, 0);
        }
        break;
    }
}

void EmDrome_State4_03(Em *em, EmWork *work)
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
            em->step++;
            EmDrome_ChangeState(em, 0, 6, 0);
        }
        break;
    }
}

void EmDrome_State4_04(Em *em, EmWork *work)
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
            em->step++;
            EmDrome_ChangeState(em, 0, 6, 0);
        }
        break;
    }
}

void EmDrome_State4_05(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x2A, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDrome_State4_06(Em *em, EmWork *work)
{
    Em_SetFlag6AE(em, 4U);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x2F, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmDrome_ChangeState(em, 0, 0xA, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmDrome_ChangeState(em, 0, 0xA, 4);
        }
        break;
    }
    Em_SpawnEffect06Bone(em, 2);
}

void EmDrome_State4_07(Em *em, EmWork *work)
{
    Em_SetHold2(em);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x1E, 0, 0);
        Player_SetFlag((Player *) em, 0x20000U);
        Em_RestartScript(em);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 40.0f) != 0) {
            em->step++;
            Em_SetMotion(em, 0x2F, 4, 0);
            Player_ClearFlag(em, 0x20000);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            EmDrome_ChangeState(em, 0, 0xF, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmDrome_ChangeState(em, 0, 0xF, 4);
        }
        break;
    }
}

void EmDrome_State4_08(Em *em, EmWork *work)
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
            EmDrome_ChangeState(em, 0, 0x10, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmDrome_ChangeState(em, 0, 0x10, 4);
        }
        break;
    }
}

void EmDrome_RunGroup4(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmDrome_State4_00(em, work);
        break;
    case 1:
        EmDrome_State4_01(em, work);
        break;
    case 2:
        EmDrome_State4_02(em, work);
        break;
    case 3:
        EmDrome_State4_03(em, work);
        break;
    case 4:
        EmDrome_State4_04(em, work);
        break;
    case 5:
        EmDrome_State4_05(em, work);
        break;
    case 6:
        EmDrome_State4_06(em, work);
        break;
    case 7:
        EmDrome_State4_07(em, work);
        break;
    case 8:
        EmDrome_State4_08(em, work);
        
    default:
        break;
    }
}

void EmDrome_State5_00_Hunted(Em *em, EmWork *work)
{
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        work->hunted = 1;
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
            em->timer = 2700;
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
            em->alpha = 0.0f;
            em->step++;
        }
        break;
    case 6:
        if (em->remote == 0) {
            em->step++;
            EmDrome_ChangeState(em, 5, 4, 4);
        }
        break;
    }
}

void EmDrome_State5_01_Hunted(Em *em, EmWork *work)
{
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        work->hunted = 1;
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
            em->timer = 2700;
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
            em->alpha = 0.0f;
            em->step++;
        }
        break;
    case 6:
        if (em->remote == 0) {
            em->step++;
            EmDrome_ChangeState(em, 5, 4, 4);
        }
        break;
    }
}

void EmDrome_State5_02_Hunted(Em *em, EmWork *work)
{
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        work->hunted = 1;
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
            em->timer = 2700;
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
            em->alpha = 0.0f;
            em->step++;
        }
        break;
    case 6:
        if (em->remote == 0) {
            em->step++;
            EmDrome_ChangeState(em, 5, 4, 4);
        }
        break;
    }
}

void EmDrome_State5_03_Hunted(Em *em, EmWork *work)
{
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        work->hunted = 1;
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
            em->timer = 2700;
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
            em->alpha = 0.0f;
            em->step++;
        }
        break;
    case 6:
        if (em->remote == 0) {
            em->step++;
            EmDrome_ChangeState(em, 5, 4, 4);
        }
        break;
    }
}

void EmDrome_State5_04_Appear(Em *em, EmWork *work)
{
    f32 target[4];
    u16 k;

    switch (em->step) {
    case 0:
        if (QuestEm_CanAppear(em) == 1) {
            switch (Area_DayId(em->area)) {
            case 0x22:
                Vec_Set(&em->posX, EmDrome_AppearPt22.pos[0], EmDrome_AppearPt22.pos[1], EmDrome_AppearPt22.pos[2]);
                Vec_Set(&em->prevPosX, EmDrome_AppearPt22.pos[0], EmDrome_AppearPt22.pos[1], EmDrome_AppearPt22.pos[2]);
                Vec_Set(target, EmDrome_AppearPt22.target[0], EmDrome_AppearPt22.target[1], EmDrome_AppearPt22.target[2]);
                em->rotY = Math_AngleXZ(&em->posX, target) & 0xFFFF;
                break;
            case 0x26:
                k = em->stateRand % 2;
                Vec_Set(&em->posX, EmDrome_AppearPts26[k].pos[0], EmDrome_AppearPts26[k].pos[1], EmDrome_AppearPts26[k].pos[2]);
                Vec_Set(&em->prevPosX, EmDrome_AppearPts26[k].pos[0], EmDrome_AppearPts26[k].pos[1], EmDrome_AppearPts26[k].pos[2]);
                Vec_Set(target, EmDrome_AppearPts26[k].target[0], EmDrome_AppearPts26[k].target[1], EmDrome_AppearPts26[k].target[2]);
                em->rotY = Math_AngleXZ(&em->posX, target) & 0xFFFF;
                break;
            case 0x27:
                k = em->stateRand % 4;
                Vec_Set(&em->posX, EmDrome_AppearPts27[k].pos[0], EmDrome_AppearPts27[k].pos[1], EmDrome_AppearPts27[k].pos[2]);
                Vec_Set(&em->prevPosX, EmDrome_AppearPts27[k].pos[0], EmDrome_AppearPts27[k].pos[1], EmDrome_AppearPts27[k].pos[2]);
                Vec_Set(target, EmDrome_AppearPts27[k].target[0], EmDrome_AppearPts27[k].target[1], EmDrome_AppearPts27[k].target[2]);
                em->rotY = Math_AngleXZ(&em->posX, target) & 0xFFFF;
                break;
            case 0x75:
                em->posY += 1300.0f;
                em->stance = 2;
                em->rotY = 0;
                break;
            }
            switch (Area_DayId(em->area)) {
            case 0x22:
                EmDrome_ChangeState(em, 7, 0, 0);
                break;
            case 0x26:
                EmDrome_ChangeState(em, 7, 0, 0);
                break;
            case 0x27:
                EmDrome_ChangeState(em, 7, 0, 0);
                break;
            case 0x75:
                EmDrome_ChangeState(em, 7, 1, 0);
                break;
            default:
                EmDrome_ChangeState(em, 0, 0xB, 0);
                break;
            }
            break;
        }
        em->taskPhase++;
        break;
    }
}

void EmDrome_RunGroup5(Em *em, s32 work)
{
    em->noHitTimer = 0xA;
    em->noPushTimer = 0xA;
    switch (em->state) {
    case 0:
        EmDrome_State5_00_Hunted(em, (EmWork *) work);
        break;
    case 1:
        EmDrome_State5_01_Hunted(em, (EmWork *) work);
        break;
    case 2:
        EmDrome_State5_02_Hunted(em, (EmWork *) work);
        break;
    case 3:
        EmDrome_State5_03_Hunted(em, (EmWork *) work);
        break;
    case 4:
        EmDrome_State5_04_Appear(em, (EmWork *) work);
        break;
    }
}

void EmDrome_State6_01_Captured(Em *em, EmWork *work)
{
    s32 timer;
    s32 t;

    Em_SetCaptured(em, 1);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x2B, 0, 0);
        em->timer = 0x96;
        Em_BeginStatus1(em);
        Em_ClearPoison(em);
        em->stance = 0;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x2C, 0, 0);
            QuestEm_Captured(em);
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
                EmDrome_ChangeState(em, 5, 4, 4);
                break;
            }
        } else {
            em->alpha = (f32) timer / 150.0f;
        }
        break;
    }
}

void EmDrome_State6_02(Em *em, EmWork *work)
{
    f32 d;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        GmEm_ResetSize(em);
        em->lookAtTarget = 0;
        em->stance = 0;
        em->posX = 13007.0f;
        em->posY = 0.0f;
        em->posZ = 3167.0f;
        em->goal[0] = 13360.0f;
        em->goal[1] = 0.0f;
        em->goal[2] = 5724.0f;
        em->rotY = 0xC00;
        Em_SetMotion(em, 1, 0, 0);
        em->timer = 90;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 2, 0, 0);
            em->timer = 300;
        }
        break;
    case 2:
        d = Vec_DistanceXZ(&em->posX, em->goal);
        em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        if (--em->timer <= 0 || d <= 100.0f) {
            em->step++;
            Em_SetMotion(em, 1, 0, 0);
            em->timer = 90;
        }
        break;
    case 3:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0xB, 0, 0);
            em->timer = 60;
        }
        break;
    case 4:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x32, 0, 0);
        }
        break;
    case 5:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0xB, 0, 0);
        }
        break;
    case 6:
        if (Save_TestEventFlag(0x98) == 1) {
            em->senseDelay = 0;
            em->noHitTimer = 0;
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDrome_RunGroup6(Em *em, EmWork *work)
{
    switch (em->state) {
    case 1:
        EmDrome_State6_01_Captured(em, work);
        break;
    case 2:
        EmDrome_State6_02(em, work);
        break;
    }
}

void EmDrome_State7_00(Em *em, EmWork *work)
{
    s32 ended;

    ended = 0;
    em->senseDelay = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->scriptPending = 0;
        Em_SetMotion(em, 3, 0, 0);
        EmDrome_StartCurves(em, 2U);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 10.0f) != 0) {
            em->stance = 2;
            ended = EmDrome_UpdateCurves(em) & 0xFF;
        }
        if ((ended != 0) && (em->posY <= em->floorY)) {
            em->step++;
            em->stance = 0;
            Em_SetMotion(em, 5, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->senseDelay = 0;
            em->noHitTimer = 5;
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDrome_State7_01(Em *em, EmWork *work)
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
            em->senseDelay = 0;
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDrome_State7_02(Em *em, EmWork *work)
{
    u8 step;

    step = em->step;
    switch (step) {
    case 0:
        em->step = step + 1;
        EmDrome_ChangeState(em, 0, 0, 0);
        break;
    }
}

void EmDrome_State7_03(Em *em, EmWork *work)
{
    u8 step;

    step = em->step;
    switch (step) {
    case 0:
        em->step = step + 1;
        EmDrome_ChangeState(em, 0, 0, 0);
        break;
    }
}

void EmDrome_RunGroup7(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmDrome_State7_00(em, work);
        break;
    case 1:
        EmDrome_State7_01(em, work);
        break;
    case 2:
        EmDrome_State7_02(em, work);
        break;
    case 3:
        EmDrome_State7_03(em, work);
        break;
    }
}

void EmDrome_State8_00_CarveStart(Em *em, EmWork *work)
{
    EmDromeVec v;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        if (Versus_DefeatCamera(em) == 1) {
            em->step++;
            Em_SetMotion(em, 0x35, 0, 0);
            em->timer = 1;
        }
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 10.0f, 0) != 0) {
            v = EmDrome_CarveThrowPt;
            StageObj_K25_Create(&gPlayers[(u8) em->side - 1], em, 0, 0, &v.x);
        }
        if (em->motion[0].playing == 0) {
            if (--em->timer <= 0) {
                Versus_RouteEmByTeam(em);
            }
        }
        break;
    case 2:
        break;
    }
}

void EmDrome_State8_01(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x2A, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 1, 0, 0);
            EmDrome_ChangeState(em, 8, 0x98, 1);
        }
        break;
    }
}

void EmDrome_State8_02(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x33, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 1, 0, 0);
            EmDrome_ChangeState(em, 8, 0x98, 1);
        }
        break;
    }
}

void EmDrome_State8_03(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x32, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 1, 0, 0);
            EmDrome_ChangeState(em, 8, 0x98, 1);
        }
        break;
    }
}

void EmDrome_State8_98(Em *em, EmWork *work)
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
            EmDrome_ChangeState(em, 8, 0x99, 1);
        }
        break;
    }
}

void EmDrome_State8_99(Em *em, EmWork *work)
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
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

void EmDrome_RunGroup8(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0x0:
        EmDrome_State8_00_CarveStart(em, work);
        break;
    case 0x1:
        EmDrome_State8_01(em, work);
        break;
    case 0x2:
        EmDrome_State8_02(em, work);
        break;
    case 0x3:
        EmDrome_State8_03(em, work);
        break;
    case 0x98:
        EmDrome_State8_98(em, work);
        break;
    case 0x99:
        EmDrome_State8_99(em, work);
        break;
    }
}

void EmDrome_Update(Em *em)
{
    u8 buf[4];
    EmDromeWork *w = (EmDromeWork *)&em->work;
    s32 seen;

    if (em->stateGroup <= 4) {
        em->alpha = 1.0f;
        em->visible = 1;
    }
    em->motionSpeed = 1.2f;
    if (em->senseDelay != 0) {
        em->noHitTimer = 5;
    }
    Em_UpdateCombatTimer(em);
    Em_RaiseTrigger80(em);
    Em_HoldPosNoPlayers(em, 0, 1);
    seen = Em_SenseObjects(em, &w->sense) & 0xFF;
    if (em->senseDelay == 0 && em->combat == 0 && seen != 0 && Em_ScriptFlagsFrom(em, 4) == 0) {
        em->triggers |= 4;
    }
    if (em->noAlert != 1 && em->combat == 1) {
        Em_CheckLowHp(em, 0.2f);
    }
    switch (Em_UpdateDamage(em, buf) & 0xFF) {
    case 0:
    case 3:
    case 4:
    case 9:
    case 11:
    case 13:
    case 15:
    case 16:
        break;
    case 1:
    case 2: {
        u16 a = em->hitAngle;
        s32 d = (a - em->rotY) & 0xFFFF;
        if (d >= 0x6000 && d < 0xA000) {
            em->rotY = a + 0x8000;
            EmDrome_ChangeState(em, 5, 0, 3);
        } else if (d >= 0x2000 && d < 0x6000) {
            em->rotY = a - 0x4000;
            EmDrome_ChangeState(em, 5, 2, 3);
        } else if (d >= 0xA000 && d < 0xE000) {
            em->rotY = a + 0x4000;
            EmDrome_ChangeState(em, 5, 3, 3);
        } else {
            em->rotY = a & 0xFFFF;
            EmDrome_ChangeState(em, 5, 1, 3);
        }
        break;
    }
    case 5:
        if ((em->stateGroup != 4 || em->state != 1) && (em->stateGroup != 4 || em->state != 2) &&
            (em->stateGroup != 4 || em->state != 3) && (em->stateGroup != 4 || em->state != 4)) {
            u16 a = em->hitAngle;
            s32 d = (a - em->rotY) & 0xFFFF;
            if (d >= 0x6000 && d < 0xA000) {
                em->rotY = a + 0x8000;
                EmDrome_ChangeState(em, 4, 1, 3);
            } else if (d >= 0x2000 && d < 0x6000) {
                em->rotY = a - 0x4000;
                EmDrome_ChangeState(em, 4, 3, 3);
            } else if (d >= 0xA000 && d < 0xE000) {
                em->rotY = a + 0x4000;
                EmDrome_ChangeState(em, 4, 4, 3);
            } else {
                em->rotY = a & 0xFFFF;
                EmDrome_ChangeState(em, 4, 2, 3);
            }
        }
        break;
    case 6:
        if ((em->stateGroup != 4 || em->state != 1) && (em->stateGroup != 4 || em->state != 2) &&
            (em->stateGroup != 4 || em->state != 3) && (em->stateGroup != 4 || em->state != 4)) {
            Em_RestartScript(em);
            Em_SetStatus4Timer(em);
            EmDrome_ChangeState(em, 4, 6, 3);
        }
        break;
    case 7:
        EmDrome_ChangeState(em, 6, 1, 3);
        break;
    case 8:
        if ((em->stateGroup != 4 || em->state != 1) && (em->stateGroup != 4 || em->state != 2) &&
            (em->stateGroup != 4 || em->state != 3) && (em->stateGroup != 4 || em->state != 4)) {
            Em_SetStatus2Timer(em);
            EmDrome_ChangeState(em, 0, 7, 3);
        }
        break;
    case 10:
        switch (em->state) {
        case 0xD:
            EmDrome_ChangeState(em, 0, 0xE, 3);
            break;
        default:
            Em_RestartScript(em);
            EmDrome_ChangeState(em, 0, 8, 3);
            break;
        }
        break;
    case 12:
        if (em->type == 0x1B) {
            u16 a = em->hitAngle;
            s32 d = (a - em->rotY) & 0xFFFF;
            if (d >= 0x6000 && d < 0xA000) {
                em->rotY = a + 0x8000;
                EmDrome_ChangeState(em, 4, 1, 3);
            } else if (d >= 0x2000 && d < 0x6000) {
                em->rotY = a - 0x4000;
                EmDrome_ChangeState(em, 4, 3, 3);
            } else if (d >= 0xA000 && d < 0xE000) {
                em->rotY = a + 0x4000;
                EmDrome_ChangeState(em, 4, 4, 3);
            } else {
                em->rotY = a & 0xFFFF;
                EmDrome_ChangeState(em, 4, 2, 3);
            }
        } else if (em->stance == 2) {
            u16 a = em->hitAngle;
            s32 d = (a - em->rotY) & 0xFFFF;
            if (d >= 0x6000 && d < 0xA000) {
                em->rotY = a + 0x8000;
                EmDrome_ChangeState(em, 4, 1, 3);
            } else if (d >= 0x2000 && d < 0x6000) {
                em->rotY = a - 0x4000;
                EmDrome_ChangeState(em, 4, 3, 3);
            } else if (d >= 0xA000 && d < 0xE000) {
                em->rotY = a + 0x4000;
                EmDrome_ChangeState(em, 4, 4, 3);
            } else {
                em->rotY = a & 0xFFFF;
                EmDrome_ChangeState(em, 4, 2, 3);
            }
        } else {
            EmDrome_ChangeState(em, 4, 0, 3);
        }
        break;
    case 14:
        if (em->type == 0x1B) {
            em->scriptPending = 0;
            if (em->stance == 2) {
                if (em->hitAngle >= 0x6000 && em->hitAngle < 0xA000) {
                    em->rotY = em->hitAngle + 0x8000;
                    EmDrome_ChangeState(em, 4, 1, 3);
                } else if (em->hitAngle >= 0x2000 && em->hitAngle < 0x6000) {
                    em->rotY = em->hitAngle - 0x4000;
                    EmDrome_ChangeState(em, 4, 3, 3);
                } else if (em->hitAngle >= 0xA000 && em->hitAngle < 0xE000) {
                    em->rotY = em->hitAngle + 0x4000;
                    EmDrome_ChangeState(em, 4, 4, 3);
                } else {
                    em->rotY = em->hitAngle & 0xFFFF;
                    EmDrome_ChangeState(em, 4, 2, 3);
                }
            } else {
                EmDrome_ChangeState(em, 4, 0, 3);
            }
        }
        break;
    case 17:
        if ((em->stateGroup != 4 || em->state != 1) && (em->stateGroup != 4 || em->state != 2) &&
            (em->stateGroup != 4 || em->state != 3) && (em->stateGroup != 4 || em->state != 4) &&
            (em->stateGroup != 4 || em->state != 8)) {
            Em_SetStatus5Timer(em);
            EmDrome_ChangeState(em, 4, 8, 3);
        }
        break;
    case 18:
        Em_ResetKindTimer(em);
        EmDrome_ChangeState(em, 4, 7, 3);
        break;
    }
    switch (gQuest.stage) {
    case 0x12:
    case 0x3EA:
    case 0x3EB:
        if (em->type == 0x1B && Area_DayId(em->area) == 0x75 && Save_TestEventFlag(0x98) == 0) {
            if (Area_DayId(gScene.area) == 0x75 && gScene.eventRunning == 1 && em->stateGroup != 6) {
                EmHermitaur_ChangeState(em, 6, 2, 1);
            }
            break;
        }
    default:
        Em_UpdatePendingScript(em);
        break;
    }
    Em_ClearFlag6AE(em);
    EmDrome_RunState(em, (s32) w);
    if (em->stateRequest != 0) {
        EmDrome_RunState(em, (s32) w);
        em->stateRequest = 0;
    }
}

void EmDrome_RunState(Em *em, s32 work)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->stateGroup) {
    case 0:
        EmDrome_RunGroup0(em, (EmWork *) work);
        break;
    case 1:
        EmDrome_RunGroup1(em, (EmWork *) work);
        break;
    case 2:
        EmDrome_RunGroup2(em, (EmWork *) work);
        break;
    case 3:
        EmDrome_RunGroup3(em, (EmWork *) work);
        break;
    case 4:
        EmDrome_RunGroup4(em, (EmWork *) work);
        break;
    case 5:
        EmDrome_RunGroup5(em, work);
        break;
    case 6:
        EmDrome_RunGroup6(em, (EmWork *) work);
        break;
    case 7:
        EmDrome_RunGroup7(em, (EmWork *) work);
        break;
    case 8:
        EmDrome_RunGroup8(em, (EmWork *) work);
        
    default:
        break;
    }
}

void EmDrome_MotionEffectsDefault(Em *em)
{
}

void EmDrome_MotionSounds(Em *em, EmWork *work);
void EmDrome_MotionEffects(Em *em, EmWork *work)
{
    f32 pos[3];

    if (em->motionId[0] != work->prevMotion) {
        work->prevMotion = em->motionId[0];
    }
    EmDrome_MotionSounds(em, work);
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
            EffDrome15_Create(em, 2);
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
        EmDrome_MotionEffectsDefault(em);
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

void EmDrome_UpdateMotionEffects(Em *em)
{
    EmDromeWork *w = (EmDromeWork *)&em->work;

    switch (w->fxStep) {
    case 0:
        w->fxStep++;
        break;
    case 1:
        EmDrome_MotionEffects(em, (EmWork *) w);
        break;
    }
}

void EmDrome_TableNop(void)
{
}

void EmDrome_SetGroup0State(Em *em, u16 state, u8 mode)
{
    switch (state) {
    case 1:
        if (em->combat != 0 && em->hp >= (s16) (0.3f * em->hpMax)) {
            state = 5;
        }
        break;
    case 12:
        if (em->remote == 0 && (Game_IsOnline() == 0 || (mode & 0xFF) != 0) && em->combat == 1 &&
            em->curGoalKind == 1 && em->curGoalArg == 0 && em->curGoalNo != -1 && em->stateRand % 2 == 0 &&
            !(((f32 *) em->playerDist)[em->curGoalNo] <= em->nearRadius)) {
            state = 5;
        }
        break;
    case 7:
        Em_BeginStatus2(em);
        break;
    case 8:
        Em_EndStatus2(em);
        break;
    case 10:
        Em_EndStatus4(em);
        break;
    case 13:
        Em_BeginStatus1(em);
        break;
    case 14:
        Em_EndStatus1(em);
        break;
    case 15:
        em->holdTimer2 = 0;
        break;
    case 16:
        Em_EndStatus5(em);
        break;
    case 17:
        Em_StartRage(em);
        break;
    case 18:
        Em_StartPendingRage(em);
        break;
    }
    em->stance = 0;
    Em_SetStateSync(em, 0, state, mode);
}

void EmDrome_SetGroup1State(Em *em, u16 state, u8 mode)
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

void EmDrome_SetGroup2State(Em *em, u16 state, u8 mode)
{
    Em_SetStateSync(em, 2, state, mode);
}

void EmDrome_SetGroup3State(Em *em, u16 state, u8 mode)
{
    switch (state) {
    case 0:
    case 2:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
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

void EmDrome_ChangeState(Em *em, u16 group, u16 state, u8 mode)
{
    EmDromeWork *w = (EmDromeWork *)&em->work;

    if ((u8)w->countdown != 0) {
        ObjListA70_Remove((EmObj *)((&w->sense)));
        w->countdown = 0U;
    }
    if (em->remote == 0) {
        Em_TargetPlayerInRange(em);
        goto block_8;
    }
    if ((mode == 3) || (mode == 4)) {
        return;
    }
block_8:
    switch (group) {
    case 0:
        EmDrome_SetGroup0State(em, state, mode);
        return;
    case 1:
        EmDrome_SetGroup1State(em, state, mode);
        return;
    case 2:
        EmDrome_SetGroup2State(em, state, mode);
        return;
    case 3:
        EmDrome_SetGroup3State(em, state, mode);
        return;
    case 5:
        if ((state == 4) && (QuestEm_CanAppear(em) == 1)) {
            QuestEm_Appear(em);
        }
        Em_SetStateSync(em, group, state, mode);
        return;
    case 4:
        switch (state) {
        case 6:
            Em_BeginStatus4(em);
            break;
        case 7:
            Em_EndAllStatus(em);
            break;
        case 8:
            Em_BeginStatus5(em);
            break;
        }
        
    case 6:
        Em_SetStateSync(em, group, state, mode);
        return;
    case 7:
        Em_SetStateSync(em, group, state, mode);
        return;
    case 8:
        if (state == 0x98) {
            Versus_MarkDefeat(em);
        }
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        
    default:
        return;
    }
}

void EmDrome_StartCurves(Em *em, u8 kind)
{
    EmDromeWork *w = (EmDromeWork *)&em->work;

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

static s32 EmDrome_CurveNone(Em *em, EmWork *work)
{
    return 0;
}


extern EmCurveKey *EmDrome_VelYCurves[];

/* em->velY (vertical speed) from curve work[0x17] at the frame in the work area: the first
 * key's value at frames 0 and 1; 2 at the end of the table (falling faster then, to -50). */
static u8 EmDrome_CurveVelY(Em *em, EmWork *work)
{
    EmDromeWork *w = (EmDromeWork *) work;
    EmCurveKey *p;
    f32 frame;
    f32 t;
    s32 i;
    EmCurveKey *tbl;
    u8 ret;

    tbl = EmDrome_VelYCurves[((EmDromeWork *)work)->curveKind];
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

extern EmCurveKey *EmDrome_SpeedCurves[];

/* em->velZ (horizontal speed) from curve work[0x17]; 4 at the end of the table; stopped
 * while em->unk698 has 0xF000000F set. */
static u8 EmDrome_CurveSpeed(Em *em, EmWork *work)
{
    EmDromeWork *w = (EmDromeWork *) work;
    EmCurveKey *p;
    f32 frame;
    f32 t;
    s32 i;
    EmCurveKey *tbl;
    u8 ret;

    tbl = EmDrome_SpeedCurves[((EmDromeWork *)work)->curveKind];
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

u8 EmDrome_UpdateCurves(Em *em)
{
    EmDromeWork *w = (EmDromeWork *)&em->work;
    u8 ret = 0;

    if (w->curveNone != 0) {
        ret = EmDrome_CurveNone(em, (EmWork *) w);
    }
    if (w->curveVelY != 0) {
        ret |= EmDrome_CurveVelY(em, (EmWork *) w);
    }
    if (w->curveSpeed != 0) {
        ret |= EmDrome_CurveSpeed(em, (EmWork *) w);
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

/* Kind 0x1D's own meaning of a work-area byte (EmWork has the shared ones). */
typedef struct EmKind1DWork {
    u8 fxStep;                  /* 0x000: as EmWork's */
    u8 unk001[0x004 - 0x001];
    s16 noHp;                   /* 0x004: Init: 1 when the variant's hp is 0 (the monster then gets 4000), else 0 */
    u8 unk006[0x100 - 0x006];
} EmKind1DWork;
typedef char EmKind1DWork_size_check[(sizeof(EmKind1DWork) == sizeof(EmWork)) ? 1 : -1];

void EmKind1D_Init(Em *em)
{
    EmKind1DWork *w = (EmKind1DWork *)&em->work;
    EmDromeKindEntry *e = &EmKind1D_Variants[em->variant];
    f32 f;

    if (gQuest.stage == 0) {
        if (Area_DayId(gScene.area) == 0xF) {
            em->posX = 9500.0f + (150.0f * (f32) (em->slotNo / 8));
            em->posY = 0.0f;
            em->posZ = 9200.0f + (150.0f * (f32) (em->slotNo & 7));
        } else {
            f = (f32) (u32) em->slotNo;
            em->posX = 5000.0f + (200.0f * f);
            em->posY = 0.0f;
            em->posZ = 5000.0f;
        }
    }
    em->blind = 2;
    em->senseDelay = 5;
    em->fieldCollide = 0;
    em->remote = 0;
    Em_SetMotionRow(em, 1, 0, 0);
    em->stance = 2;
    Em_SetStateRand(em, 0, 1);
    if (e->hp <= 0) {
        w->noHp = 1;
        em->hpMax = em->hp = 4000;
    } else {
        w->noHp = 0;
        em->hpMax = em->hp = e->hp;
    }
    em->bodyScale = em->scaleX = em->scaleY = em->scaleZ = e->scale;
    GmEm_ApplySize(em);
    em->meshShown[0] = 0;
    em->meshShown[1] = 0;
    em->meshShown[2] = 0;
    em->meshShown[3] = 0;
    em->meshShown[4] = 0;
    em->meshShown[e->part] = 1;
    Em_InitParts(em);
}

void EmKind1D_EndState(Em *em)
{
    em->scriptPending = 1;
}

void EmKind1D_State0_00(void)
{
}

void EmKind1D_RunGroup0(Em *em)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->state) {
    case 0:
        EmKind1D_State0_00();
        break;
    }
}

void EmKind1D_State1_00(Em *em)
{
    EmKind1D_EndState(em);
}

void EmKind1D_RunGroup1(Em *em)
{
    switch (em->state) {
    case 0:
        EmKind1D_State1_00(em);
        break;
    }
}

void EmKind1D_State3_00(Em *em)
{
    EmKind1D_EndState(em);
}

void EmKind1D_RunGroup3(Em *em)
{
    switch (em->state) {
    case 0:
        EmKind1D_State3_00(em);
        break;
    }
}

void EmKind1D_State4_00(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 0xA;
        break;
    case 1:
        if (--em->timer <= 0) {
            EmKind1D_EndState(em);
        }
        break;
    }
}

void EmKind1D_State4_01(Em *em)
{
    EmKind1D_EndState(em);
}

void EmKind1D_RunGroup4(Em *em)
{
    switch (em->state) {
    case 0:
        EmKind1D_State4_00(em);
        break;
    case 1:
        EmKind1D_State4_01(em);
        break;
    }
}

void EmKind1D_RunGroup5(Em *em)
{
    EmDromeKindEntry *e = &EmKind1D_Variants[em->variant];

    em->noHitTimer = 0xA;
    em->noPushTimer = 0xA;
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 0x3C;
        Em_PlaySe(em, e->se);
        Effect02_SpawnForOwner(em, 0, 9, 0, &em->posX, em->scaleX);
        Effect02_SpawnForOwner(em, 0, 9, 1, &em->posX, em->scaleX);
        Effect0D_SpawnIfHere(em, &em->posX, 0x22, 5.0f * em->scaleX);
        if (em->variant == 4) {
            if (em->area == gScene.area) {
                StageObj_K0E_CreateScaled(&em->posX, 0, 3.0f);
            }
            Eff09_CreateAt(&em->posX, 0x10, em->area, 0);
        }
        break;
    case 1:
        em->taskPhase++;
        em->visible = 0;
        break;
    }
}

void EmKind1D_Update(Em *em)
{
    u8 buf[3];
    u32 ev;

    em->blind = 2;
    if (*(s16 *)((u8 *)em + 0x43C) != 0) {
        em->guardTimer = 0xA;
    }
    ev = Em_UpdateDamage(em, buf) & 0xFF;
    switch (ev) {
    case 0:
    case 3:
    case 6:
    case 7:
    case 8:
    case 9:
    case 11:
    case 14:
    case 15:
    case 16:
    case 18:
    case 4:
    case 5:
    case 10:
    case 2:
        break;
    case 1:
        Em_SetStateRand(em, 5, 0);
        break;
    case 12:
    case 13:
        Em_SetStateRand(em, 4, 0);
        Em_ResetPartHp(em, 0U);
        break;
    case 17:
        Em_EndStatus5(em);
        break;
    }
    Em_UpdatePendingScript(em);
    Em_ClearFlag6AE(em);
    EmKind1D_RunState(em);
    if (em->stateRequest != 0) {
        EmKind1D_RunState(em);
        em->stateRequest = 0;
    }
}

void EmKind1D_RunState(Em *em)
{
    switch (em->stateGroup) {
    case 0:
        EmKind1D_RunGroup0(em);
        break;
    case 1:
        EmKind1D_RunGroup1(em);
        break;
    case 2:
        EmKind1D_RunGroup0(em);
        break;
    case 3:
        EmKind1D_RunGroup3(em);
        break;
    case 4:
        EmKind1D_RunGroup4(em);
        break;
    case 5:
        EmKind1D_RunGroup5(em);
        break;
    case 6:
        EmKind1D_RunGroup4(em);
        break;
    case 7:
        EmKind1D_RunGroup4(em);
        
    default:
        break;
    }
}

void EmKind1D_UpdateMotionEffects(Em *em)
{
    EmKind1DWork *w = (EmKind1DWork *)&em->work;

    switch (w->fxStep) {
    case 0:
        w->fxStep++;
        
    case 1:
        break;
    }
}

void EmKind1D_TableNop(void)
{
}

void EmKirin_MotionSounds(Em *em, EmWork *work)
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
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0x51, 0x17);
        break;
    case 0x3EC:
        Em_PlaySeAtBoneOnFrame(em, 6, 0x4A, 0x17);
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
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x49, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x36, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 0, 0);
        break;
    case 0x40E:
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 1, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x11, 0x17);
        break;
    case 0x40F:
        Em_PlaySeAtBoneOnFrame(em, 2, 0x51, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x17, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x5A, 2, 0);
        break;
    case 0x424:
    case 0x425:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x4B, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 0, 0);
        break;
    case 0x426:
        Em_PlaySeAtBoneOnFrame(em, 4, 1, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x4B, 0x17);
        break;
    case 0x427:
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
        Em_PlaySeAtBoneOnFrame(em, 4, 0x4A, 0x17);
        break;
    case 0x42B:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x59, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0xB8, 0x59, 0x17);
        break;
    case 0x42C:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x55, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x8A, 0x55, 0x17);
        break;
    case 0x42D:
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x56, 4, 0x57, 4), 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0, 0);
        
        break;
    }
}

void EmKirin_Init(Em *em)
{
    f32 fslot;
    f32 fslot2;
    s16 size;
    u16 area;
    u8 slotNo;
    u8 slot;

    if (gQuest.stage == 0) {
        area = gScene.area;
        switch (area) {
        case 15:
            em->posX = 9500.0f + (150.0f * (f32) (em->slotNo / 8));
            em->posY = 0.0f;
            em->posZ = 9200.0f + (150.0f * (f32) (em->slotNo & 7));
            break;
        case 16:
            em->posX = 6000.0f + (150.0f * (f32) (em->slotNo / 8));
            em->posY = 0.0f;
            em->posZ = 7000.0f + (150.0f * (f32) (em->slotNo & 7));
            break;
        case 19:
            em->posX = 11000.0f + (150.0f * (f32) (em->slotNo / 8));
            em->posY = 0.0f;
            em->posZ = 10000.0f + (150.0f * (f32) (em->slotNo & 7));
            break;
        case 37:
            em->posX = 9500.0f + (150.0f * (f32) (em->slotNo / 8));
            em->posY = 0.0f;
            em->posZ = 10000.0f + (150.0f * (f32) (em->slotNo & 7));
            break;
        default:
            slotNo = em->slotNo;
            fslot = (f32) (u32) slotNo;
            em->posX = 5000.0f - (80.0f * fslot);
            em->posY = 0.0f;
            slot = em->slotNo;
            fslot2 = (f32) (u32) slot;
            em->posZ = 5000.0f - (80.0f * fslot2);
            break;
        }
    }
    em->blind = 0;
    em->stance = 0;
    em->unk_InitFlag = 1;
    em->gender = 0;
    size = Em_ComputeSize(em, 0);
    em->hp = size;
    em->hpMax = size;
    em->motionSpeed = 1.0f;
    Em_SetMotionRow(em, 1, 0, 0);
    Em_InitParts(em);
    EmKirin_ChangeState(em, 0, 1, 0);
}

void EmKirin_Update(Em *em)
{
    u8 buf[3];
    s32 a;
    s32 d;

    Em_UpdateCombatTimer(em);
    Em_RaiseTrigger80(em);
    Em_HoldPosNoPlayers(em, 0, 1);
    switch (Em_UpdateDamage(em, buf) & 0xFF) {
    case 0:
    case 3:
    case 4:
    case 5:
    case 7:
    case 9:
    case 11:
    case 14:
    case 15:
    case 16:
    case 18:
        break;
    case 1:
    case 2:
        em->rotY = em->hitAngle + 0x7FFF + 1;
        EmKirin_ChangeState(em, 5, 0, 3);
        break;
    case 6:
        Em_SetStatus4Timer(em);
        EmKirin_ChangeState(em, 4, 3, 3);
        break;
    case 8:
        Em_SetStatus2Timer(em);
        EmKirin_ChangeState(em, 0, 8, 3);
        break;
    case 10:
        em->rotY = em->hitAngle + 0x7FFF + 1;
        EmKirin_ChangeState(em, 4, 5, 3);
        break;
    case 12:
    case 13:
        a = em->hitAngle;
        d = a - em->rotY;
        if (((d - 0x4000) & 0xFFFF) < 0x8000 && em->parts[0].hp > 0 && em->stance == 0) {
            if ((d & 0xFFFF) < 0x8000) {
                EmKirin_ChangeState(em, 4, 0, 3);
            } else {
                EmKirin_ChangeState(em, 4, 1, 3);
            }
        } else {
            em->rotY = a + 0x7FFF + 1;
            EmKirin_ChangeState(em, 4, 2, 3);
        }
        break;
    case 17:
        Em_EndStatus5(em);
        break;
    }
    switch (gQuest.stage) {
    case 0x425:
    case 0x426:
        if (Area_DayId(em->area) == 0x64 && Save_TestEventFlag(0xAC) == 0) {
            if (Area_DayId(gScene.area) == 0x64 && gScene.eventRunning == 1 && em->stateGroup != 6) {
                EmKirin_ChangeState(em, 6, 0, 1);
            }
            break;
        }
    default:
        Em_UpdatePendingScript(em);
        break;
    }
    Em_ClearFlag6AE(em);
    EmKirin_RunState(em);
    if (em->stateRequest != 0) {
        EmKirin_RunState(em);
        em->stateRequest = 0;
    }
}

void EmKirin_State0_00(void)
{
}

void EmKirin_State0_01(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 1, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

void EmKirin_State0_02(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 2, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

void EmKirin_State0_03(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 3, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

void EmKirin_State0_04(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 4, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

void EmKirin_State0_05(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 5, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

void EmKirin_State0_06(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 6, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

void EmKirin_State0_07(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->targetAngle = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        if (((em->targetAngle - em->rotY) & 0xFFFF) < 0x8001) {
            Em_SetMotion(em, 7, 0, 0);
            break;
        }
        Em_SetMotion(em, 8, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

void EmKirin_State0_08(Em *em, EmWork *work)
{
    Em_SetFlag6AE(em, 2U);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
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
            EmKirin_ChangeState(em, 0, 9, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmKirin_ChangeState(em, 0, 9, 4);
        }
        break;
    }
}

void EmKirin_State0_09(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->blind = 2;
        Em_SetMotion(em, 0x40, 0, 0);
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            EmKirin_EndState(em);
        }
        break;
    }
}

void EmKirin_State0_0A(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x45, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->noReact = 0;
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

void EmKirin_State0_0B(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x26, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

void EmKirin_State0_0C(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x26, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 1, 8, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

void EmKirin_RunGroup0(Em *em, EmWork *work)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->state) {
    case 0:
        EmKirin_State0_00();
        break;
    case 1:
        EmKirin_State0_01(em, work);
        break;
    case 2:
        EmKirin_State0_02(em, work);
        break;
    case 3:
        EmKirin_State0_03(em, work);
        break;
    case 4:
        EmKirin_State0_04(em, work);
        break;
    case 5:
        EmKirin_State0_05(em, work);
        break;
    case 6:
        EmKirin_State0_06(em, work);
        break;
    case 7:
        EmKirin_State0_07(em, work);
        break;
    case 8:
        EmKirin_State0_08(em, work);
        break;
    case 9:
        EmKirin_State0_09(em, work);
        break;
    case 10:
        EmKirin_State0_0A(em, work);
        break;
    case 11:
        EmKirin_State0_0B(em, work);
        break;
    case 12:
        EmKirin_State0_0C(em, work);
        
    default:
        break;
    }
}

void EmKirin_State1_00(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->targetAngle = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        Em_SetMotion(em, 9, 0, 0);
        break;
    case 1:
        if (Angle_TurnToward((s32 *) &em->rotY, (s32 *) &em->targetAngle, 0x100) < 0x100) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
    Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
}

void EmKirin_State1_01(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 9, 0, 0);
        break;
    case 1:
        if (Em_StepToGoal(em, 0x40, 0) == 1) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

void EmKirin_State1_02(Em *em, EmWork *work, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0xA, 0, 0);
        Eff02_Create(em, 0x10);
        break;
    case 1:
        if ((Em_StepToGoal(em, 0x40, 0) == 1) && (em->motion[0].playing == 0)) {
            em->step++;
            Em_SetMotion(em, 0xE, 4, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

void EmKirin_State1_03(Em *em, EmWork *work, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0xB, 0, 0);
        Eff02_Create(em, 0x10);
        break;
    case 1:
        if (Em_StepToGoal(em, 0x40, 0) == 1) {
            if (em->motion[0].playing == 0) {
                em->step++;
                Em_SetLayerMotion(em, 0x3F6, 0, 2, 0);
                Em_SetLayerMotion(em, 0x4BE, 0, 2, 1);
                break;
            }
        } else {
            if (em->motion[0].playing == 0) {
                Em_SetLayerMotion(em, 0x3F3, 0, 0, 0);
                Em_SetLayerMotion(em, 0x4BB, 0, 0, 1);
            }
            break;
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmKirin_EndState(em);
        }
        break;
    }
}

void EmKirin_State1_04(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0xC, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0xE, 0, 0x1A);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

void EmKirin_State1_05(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0xD, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0xE, 0, 0x1A);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

void EmKirin_State1_06(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0xF, 0, 0);
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
            EmKirin_EndState(em);
        }
        break;
    }
}

void EmKirin_State1_07(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x16, 0, 0);
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
            EmKirin_EndState(em);
        }
        break;
    }
}

void EmKirin_State1_08(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x12, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

void EmKirin_State1_0B(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0xC, 0, 0);
        Eff02_Create(em, 0xD);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0xD, 0, 0);
            Eff02_Create(em, 0xD);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0xC, 0, 0);
            Eff02_Create(em, 0xD);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0xD, 0, 0);
            Eff02_Create(em, 0xD);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

void EmKirin_State1_0C(Em *em, EmWork *work)
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
        if (diff <= 0x3000U || diff >= 0xD000U) {
            Em_SetMotion(em, 0x12, 0, 0);
        } else if (diff >= 0x8000U) {
            Em_SetMotion(em, 0x16, 0, 0);
        } else {
            Em_SetMotion(em, 0xF, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].blend != 0) {
            break;
        }
        if (em->motionId[0] == 0x3FA) {
            frame = 26.0f;
            step = 0x4000U / (u32) (frame / 2.0f);
        } else {
            frame = 30.0f;
            step = 0x8000U / (u32) (frame / 2.0f);
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
            EmKirin_EndState(em);
        }
        break;
    }
}

void EmKirin_State1_0D(Em *em, EmWork *work)
{
    f32 dist;
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
        work->accelAngX = 0;
        work->accelAngZ = 0;
        if (!(em->posY < em->goal[1])) {
            em->velY = 75.0f;
            em->accelY = -10.0f;
            em->velZ = 50.0f;
            break;
        }
        dist = Vec_DistanceXZ(&em->posX, em->goal);
        em->velZ = 100.0f;
        em->accelY = -10.0f;
        em->timer = (s32) (dist / em->velZ);
        em->velY = (1000.0f + em->goal[1] - em->posY) / (f32) em->timer - (em->accelY * (f32) em->timer) / 2.0f;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
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
            Em_SetMotion(em, 0xE, 4, 0xA);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step = step + 1;
            EmKirin_EndState(em);
        }
        break;
    }
}

void EmKirin_RunGroup1(Em *em, EmWork *work)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->state) {
    case 0:
        EmKirin_State1_00(em);
        break;
    case 1:
        EmKirin_State1_01(em);
        break;
    case 2:
        EmKirin_State1_02(em, work, 0);
        break;
    case 3:
        EmKirin_State1_03(em, work, 0);
        break;
    case 4:
        EmKirin_State1_04(em);
        break;
    case 5:
        EmKirin_State1_05(em);
        break;
    case 6:
        EmKirin_State1_06(em);
        break;
    case 7:
        EmKirin_State1_07(em);
        break;
    case 8:
        EmKirin_State1_08(em);
        break;
    case 9:
        EmKirin_State1_02(em, work, 1);
        break;
    case 0xA:
        EmKirin_State1_03(em, work, 1);
        break;
    case 0xB:
        EmKirin_State1_0B(em);
        break;
    case 0xC:
        EmKirin_State1_0C(em, work);
        break;
    case 0xD:
        EmKirin_State1_0D(em, work);
        break;
    }
}

void EmKirin_State3_00(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x26, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

void EmKirin_State3_01(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x11, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0xE, 0, 0x1A);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

void EmKirin_State3_02(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x14, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x18, 0, 8);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

void EmKirin_State3_03(Em *em, EmWork *work, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x18, 0, 0);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 22.0f, 0U) != 0) {
            if (!(variant & 0xFF)) {
                EffEm08_CreateAtBone(em, 0x17, 8, 0, 0, 0);
            } else {
                EffEm08_CreateAtBone(em, 0x17, 8, 6, 0, 0);
            }
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

void EmKirin_State3_04(Em *em, EmWork *work, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x27, 0, 0);
        EmFxSpark_SpawnTimed(em, 8, 0x5A, 1.0f);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 90.0f, 0U) != 0) {
            if (!(variant & 0xFF)) {
                EffEm08_CreateAtBone(em, 0x17, 8, 1, 0, 0);
            } else {
                EffEm08_CreateAtBone(em, 0x17, 8, 7, 0, 0);
            }
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

void EmKirin_State3_05(Em *em, EmWork *work, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x27, 0, 0);
        EmFxSpark_SpawnTimed(em, 8, 0x5A, 1.0f);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 90.0f, 0U) != 0) {
            if (!(variant & 0xFF)) {
                EffEm08_CreateAtBone(em, 0x17, 8, 2, 0, 0);
            } else {
                EffEm08_CreateAtBone(em, 0x17, 8, 8, 0, 0);
            }
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

void EmKirin_State3_06(Em *em, EmWork *work, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x27, 0, 0);
        EmFxSpark_SpawnTimed(em, 8, 0x5A, 1.0f);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 90.0f, 0U) != 0) {
            if (!(variant & 0xFF)) {
                EffEm08_CreateAtBone(em, 0x17, 8, 3, 0, 0);
            } else {
                EffEm08_CreateAtBone(em, 0x17, 8, 9, 0, 0);
            }
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

void EmKirin_State3_07(Em *em, EmWork *work, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x27, 0, 0);
        EmFxSpark_SpawnTimed(em, 8, 0x5A, 1.0f);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 90.0f, 0U) != 0) {
            if (!(variant & 0xFF)) {
                EffEm08_CreateAtBone(em, 0x17, 8, 4, 0, 0);
            } else {
                EffEm08_CreateAtBone(em, 0x17, 8, 0xA, 0, 0);
            }
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

void EmKirin_State3_08(Em *em, EmWork *work, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x27, 0, 0);
        EmFxSpark_SpawnTimed(em, 8, 0x5A, 1.0f);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 90.0f, 0U) != 0) {
            if (!(variant & 0xFF)) {
                EffEm08_CreateAtBone(em, 0x17, 8, 5, 0, 0);
            } else {
                EffEm08_CreateAtBone(em, 0x17, 8, 0xB, 0, 0);
            }
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

void EmKirin_RunGroup3(Em *em, EmWork *work)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->state) {
    case 0:
        EmKirin_State3_00(em);
        break;
    case 1:
        EmKirin_State3_01(em);
        break;
    case 2:
        EmKirin_State3_02(em);
        break;
    case 3:
        EmKirin_State3_03(em, work, 0);
        break;
    case 4:
        EmKirin_State3_04(em, work, 0);
        break;
    case 5:
        EmKirin_State3_05(em, work, 0);
        break;
    case 6:
        EmKirin_State3_06(em, work, 0);
        break;
    case 7:
        EmKirin_State3_07(em, work, 0);
        break;
    case 8:
        EmKirin_State3_08(em, work, 0);
        break;
    case 9:
        EmKirin_State3_03(em, work, 1);
        break;
    case 0xA:
        EmKirin_State3_04(em, work, 1);
        break;
    case 0xB:
        EmKirin_State3_05(em, work, 1);
        break;
    case 0xC:
        EmKirin_State3_06(em, work, 1);
        break;
    case 0xD:
        EmKirin_State3_07(em, work, 1);
        break;
    case 0xE:
        EmKirin_State3_08(em, work, 1);
        break;
    }
}

void EmKirin_State4_00(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x3C, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

void EmKirin_State4_01(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x3D, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

void EmKirin_State4_02(Em *em, EmWork *work, s16 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x3E, 0, 0);
        switch (variant) {
        case 1:
            em->blind = 0;
            break;
        }
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
            Em_SetMotion(em, 0x3F, 0, 0);
            em->timer = 0x5A;
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x40, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

void EmKirin_State4_03(Em *em)
{
    Em_SetFlag6AE(em, 4U);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x44, 0, 0);
        em->noReact = 1;
        Em_RestartScript(em);
        break;
    case 1:
        Em_SpawnEffect06Bone(em, 2);
        if (--em->timer <= 0) {
            em->step++;
            EmKirin_ChangeState(em, 0, 0xA, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmKirin_ChangeState(em, 0, 0xA, 4);
        }
        break;
    }
}

void EmKirin_State4_04(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x41, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x40, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

void EmKirin_RunGroup4(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmKirin_State4_00(em);
        break;
    case 1:
        EmKirin_State4_01(em);
        break;
    case 2:
        EmKirin_State4_02(em, work, 0);
        break;
    case 3:
        EmKirin_State4_03(em);
        break;
    case 4:
        EmKirin_State4_04(em);
        break;
    case 5:
        EmKirin_State4_02(em, work, 1);
        break;
    }
}

void EmKirin_State5_00_Hunted(Em *em, EmWork *work)
{
    em->noHitTimer = 0xA;
    em->noPushTimer = 0xA;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x3E, 0, 0);
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
            Em_SetMotion(em, 0x3F, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 90;
            em->motionSpeed = 0.0f;
            EmMine_Create(em, 0);
            em->fieldCollide = 0;
        }
        break;
    case 3:
        EmMine_Update(em);
        break;
    }
    if (em->posY < em->floorY) {
        em->posY = em->floorY;
    }
}

void EmKirin_RunGroup5(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmKirin_State5_00_Hunted(em, work);
        break;
    }
}

void EmKirin_State6_00(Em *em)
{
    f32 d;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        GmEm_ResetSize(em);
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 0;
        em->posX = 12773.0f;
        em->posY = 1200.0f;
        em->posZ = 3971.0f;
        em->goal[0] = 12368.0f;
        em->goal[1] = 1240.0f;
        em->goal[2] = 4606.0f;
        em->rotX = 0;
        em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        em->rotZ = 0;
        Em_SetMotion(em, 1, 0, 0);
        em->timer = 140;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            em->posX = 12773.0f;
            em->posY = 1200.0f;
            em->posZ = 3971.0f;
            Em_SetMotion(em, 9, 0, 0);
            em->timer = 600;
        }
        break;
    case 2:
        d = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer <= 0 || d <= 100.0f) {
            em->step++;
            Em_SetMotion(em, 4, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 3, 0, 0);
            em->timer = 0;
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x12, 0, 0);
        }
        break;
    case 5:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->posX = 9964.0f;
            em->posY = 0.0f;
            em->posZ = 9481.0f;
            em->goal[0] = 9882.0f;
            em->goal[1] = 0.0f;
            em->goal[2] = 9808.0f;
            em->rotX = 0;
            em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
            em->rotZ = 0;
            Em_SetMotion(em, 0xE, 0, 0);
        }
        break;
    case 6:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 9, 0, 0);
            em->timer = 30;
        }
        break;
    case 7:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 5, 0, 0);
        }
        break;
    case 8:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x26, 0, 0);
            em->timer = 80;
        }
        break;
    case 9:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x27, 0, 0);
            EmFxSpark_SpawnTimed(em, 8, 0x5A, 1.0f);
        }
        break;
    case 10:
        if (Player_PassedFrameScaled(em, 90.0f, 0U) != 0) {
            EffEm08_CreateAtBone(em, 0x17, 8, 4, 0, 0);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 3, 0, 0);
        }
        break;
    case 11:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 1, 0, 0);
        }
        break;
    case 12:
        if (Save_TestEventFlag(0xAC) == 1) {
            EmKirin_EndState(em);
        }
        break;
    }
}

void EmKirin_RunGroup6(Em *em)
{
    switch (em->state) {
    case 0:
        EmKirin_State6_00(em);
        break;
    }
}

void EmKirin_RunState(Em *em)
{
    u8 *w = &em->work;

    switch (em->stateGroup) {
    case 0:
        EmKirin_RunGroup0(em, (EmWork *) w);
        break;
    case 1:
        EmKirin_RunGroup1(em, (EmWork *) w);
        break;
    case 2:
        EmKirin_RunGroup0(em, (EmWork *) w);
        break;
    case 3:
        EmKirin_RunGroup3(em, (EmWork *) w);
        break;
    case 4:
        EmKirin_RunGroup4(em, (EmWork *) w);
        break;
    case 5:
        EmKirin_RunGroup5(em, (EmWork *) w);
        break;
    case 6:
        EmKirin_RunGroup6(em);
        break;
    case 7:
        EmKirin_RunGroup6(em);
        
    default:
        break;
    }
}

void EmKirin_ChangeState(Em *em, u16 group, u16 state, u8 mode)
{
    f32 d;
    f32 f;

    if (em->remote == 0) {
        Em_TargetPlayerInRange(em);
    } else if (mode == 3 || mode == 4) {
        return;
    }
    switch (group) {
    case 0:
        switch (state) {
        case 7:
            break;
        case 8:
            Em_BeginStatus2(em);
            break;
        case 9:
            Em_EndStatus2(em);
            break;
        case 10:
            Em_EndStatus4(em);
            break;
        }
        break;
    case 1:
        switch (state) {
        case 0:
            break;
        case 1:
        case 2:
        case 3:
            d = 100.0f;
            goto move;
        case 9:
        case 10:
            d = 300.0f;
        move:
            Em_SetSpeedScaled(em, 0, 1000.0f, d);
            if (state == 10 && em->curGoalKind != 1) {
                f = em->goalDist - GmEm_ScaleBySize(em, 300.0f);
                em->goalDist = f;
                if (f <= 0.0f) {
                    em->goalDist = 0.0f;
                }
            }
            break;
        }
        break;
    case 3:
        break;
    case 4:
        switch (state) {
        case 3:
            Em_BeginStatus4(em);
            break;
        case 5:
            Em_EndStatus2(em);
            break;
        }
        break;
    }
    Em_SetStateSync(em, group, state, mode);
}

void EmKirin_EndState(Em *em)
{
    em->motionSpeed = 1.0f;
    em->stance = 0;
    em->lookAtTarget = 0;
    em->scriptPending = 1;
    EmKirin_ChangeState(em, 0, 1, 0);
}

void EmKirin_SpawnEffects(Em *em)
{
    EmFxTri_Spawn(em, 0);
    EmFxTri_Spawn(em, 1);
    Effect01_Spawn((Player *) em, 4);
}

void EmKirin_TableNop(void)
{
}

void EmKirin_MotionEffectsDefault(Em *em)
{
}

void EmKirin_MotionEffects(Em *em, EmWork *work)
{
    f32 pos[3];

    if (em->motionId[0] != work->fxMotion) {
        work->fxMotion = em->motionId[0];
    }
    EmKirin_MotionSounds(em, work);
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
        break;
    case 0x3F9:
        if (Player_PassedFrameScaled(em, 20.0f, 0U) != 0) {
            Eff02_Create(em, 0xB);
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
            Eff02_Create(em, 0xC);
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
        EmKirin_MotionEffectsDefault(em);
        break;
    }
}

void EmKirin_UpdateMotionEffects(Em *em)
{
    EmWork *w = (EmWork *)&em->work;

    switch (w->fxStep) {
    case 0:
        w->fxStep++;
        break;
    case 1:
        EmKirin_MotionEffects(em, w);
        break;
    }
}

void EmGaruga_MotionSounds(Em *em, EmWork *work)
{
    s16 motion;

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
        Em_PlaySe6AtBoneOnFrame(em, 6, 0x59, 0x23);
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
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x12, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 6, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 6, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x29, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 0x29, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x52, 0x29, 0x23);
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
        Em_PlaySeAtBoneOnFrame(em, 2, 0x2D, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xB6, 0x2D, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x16C, 0x2D, 0x23);
        break;
    case 0x408:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x2F, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x8E, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x17, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x13, 0);
        break;
    case 0x409:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x2F, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0x2D, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x7E, 3, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0xB6, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0x16, 0);
        break;
    case 0x40A:
        Em_PlaySe6AtBoneOnFrame(em, 0x24, 0x57, 0x23);
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
        Em_PlaySeAtBoneOnFrame(em, 2, 0x23, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0x21, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x80, 0x22, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xB8, 0x24, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x120, 0x22, 0x23);
        break;
    case 0x40C:
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0x24, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0x22, 0x23);
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
    case 0x410:
        Em_PlaySe6AtBoneInFrames(em, 0x34, 0xC8, 0x60, 0x23, 6, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x96, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 2, 0x16, 0);
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
    case 0x411:
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 1, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 0xD, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 0x14, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 0x13, 0);
        Em_PlaySe6AtBoneOnFrame(em, 0x54, 0x4F, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xA0, 0xF, 6);
        Em_PlaySeAtBoneOnFrame(em, 0xA4, 0xF, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0xD2, 0xC, 6);
        Em_PlaySeAtBoneOnFrame(em, 0xD6, 0xC, 0xC);
        break;
    case 0x412:
        Em_PlaySe6AtBoneOnFrame(em, 8, 0x4B, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0xF, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x14, 0x2A);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 0, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 1, 0x1A);
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
    case 0x417:
        Em_PlaySeAtBoneOnFrame(em, 2, 0x12, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 0x18, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0x36, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 0x29, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xDC, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 0xD, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0xB4, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x104, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x120, 3, 0x1A);
        break;
    case 0x418:
        Em_PlaySeAtBoneOnFrame(em, 2, 0x22, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 0x22, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 0x22, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x72, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 1, 0x14);
        break;
    case 0x41B:
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0x12, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 6, 0x1A);
        Em_PlaySe6AtBoneOnFrame(em, 0x96, 0x5E, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xFC, 0x27, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xEE, 0x16, 0);
        break;
    case 0x41C:
        if ((em->stateGroup == 2) && (em->state == 0xC)) {
            Em_PlaySeAtBoneOnFrame(em, 0x16, 0x28, 0x23);
            Em_PlaySe6AtBoneInFrames(em, 0x48, 0xB2, 0x60, 0x23, 6, 0);
            Em_PlaySeAtBoneOnFrame(em, 6, 0x14, 0x22);
            Em_PlaySeAtBoneOnFrame(em, 0x16, 0, 0x1A);
            Em_PlaySeAtBoneOnFrame(em, 0x3A, 0xF, 6);
            Em_PlaySeAtBoneOnFrame(em, 0x3E, 0xF, 0xC);
            Em_PlaySeAtBoneOnFrame(em, 0x6A, 0xB, 6);
            Em_PlaySeAtBoneOnFrame(em, 0x6E, 0xB, 0xC);
            Em_PlaySeAtBoneOnFrame(em, 0xAC, 0xB, 6);
            Em_PlaySeAtBoneOnFrame(em, 0xB0, 0xB, 0xC);
        }
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0x28, 0x23);
        Em_PlaySe6AtBoneInFrames(em, 0x48, 0xB2, 0x4F, 0x23, 6, 0);
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
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0x36, 0x23);
        Em_PlaySe6AtBoneOnFrame(em, 0x7C, 0x4F, 0x23);
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
        Em_PlaySe6AtBoneOnFrame(em, 6, 0x53, 0x23);
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
        Em_PlaySe6AtBoneOnFrame(em, 6, 0x53, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x12, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 6, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 4, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0xAC, 3, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x6E, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x5A, 0x15, 0);
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
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0, 0x1A);
        break;
    case 0x428:
        Em_PlaySe6AtBoneOnFrame(em, 4, 0x52, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x76, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x7A, 0, 0x1A);
        break;
    case 0x429:
        Em_PlaySe6AtBoneOnFrame(em, 4, 0x54, 0x23);
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
        Em_PlaySeAtBoneOnFrame(em, 0x8C, 0x28, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xB4, 0x2C, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x13A, 0x2F, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x192, 0x2B, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x174, 0xF, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x176, 0xE, 0xC);
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
        Em_PlaySe6AtBoneOnFrame(em, 0x3A, 0x52, 0x23);
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
        Em_PlaySe6AtBoneOnFrame(em, 4, 0x52, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x13, 0x2A);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 9, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0x11, 0);
        break;
    case 0x434:
        Em_PlaySe6AtBoneOnFrame(em, 0x10, 0x5F, 0x23);
        break;
    case 0x435:
        Em_PlaySe6AtBoneOnFrame(em, 4, 0x53, 0x23);
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
        Em_PlaySe6AtBoneOnFrame(em, 4, 0x54, 0x23);
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
        Em_PlaySeAtBoneOnFrame(em, 0x26, 0x2D, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xC8, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 3, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x140, 3, 0x1A);
        break;
    case 0x439:
        Em_PlaySeAtBoneOnFrame(em, 2, 0x16, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 0x17, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 0x2D, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xC8, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x140, 3, 0x14);
        break;
    case 0x43A:
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
        Em_PlaySeAtBoneOnFrame(em, 4, 0x53, 0x23);
        Em_PlaySe6AtBoneOnFrame(em, 0xA8, 0x4F, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xA2, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 0xA6, 0xD, 0xC);
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
        Em_PlaySeAtBoneOnFrame(em, 8, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 8, 8, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x68, 5, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 1, 0x14);
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
        Em_PlaySeAtBoneOnFrame(em, 4, 0x27, 0x23);
        Em_PlaySe6AtBoneOnFrame(em, 4, 0x52, 0x23);
        break;
    case 0x448:
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0x12, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 0x12, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0xC, 6);
        Em_PlaySeAtBoneOnFrame(em, 8, 0xC, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x28, 0x23);
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
        Em_PlaySeAtBoneOnFrame(em, 4, 0x28, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0x20, 0x23);
        break;
    case 0x44C:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0x23);
        Em_PlaySe6AtBoneOnFrame(em, 0x2E, 0x4F, 0x23);
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
        break;
    case 0x44D:
        Em_PlaySe6AtBoneOnFrame(em, 4, 0x4F, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 8, 0xD, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x144, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0xD, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x36, 6, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x36, 7, 0x14);
        break;
    case 0x44E:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x34, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 0x32, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x33, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0x32, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x5A, 0x33, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 5, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 0x12, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 1, 0x1A);
        break;
    case 0x450:
        Em_PlaySe6AtBoneOnFrame(em, 4, 0x5E, 0x23);
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
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0x2E, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 0x29, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 0x18, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x78, 0x18, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xA4, 0x18, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x12, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x13, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 0xF, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 0xF, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0xFA, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x12A, 0x12, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x11E, 0x13, 0);
        break;
    case 0x452:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x12, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0x24, 0x23);
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
    case 0x454:
        Em_PlaySeAtBoneOnFrame(em, 0, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0x12, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0x2C, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x19C, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xC8, 0x37, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x130, 0x16, 0x22);
        
    case 0x3E9:
    default:
        break;
    }
}

/* em_drome's .data (0x8C3E50-0x8C41E0) */
EmDromeAppearPoint EmDrome_AppearPt22 = {
#include "game/008C3E50.inc"
};
EmDromeAppearPoint EmDrome_AppearPts26[2] = {
#include "game/008C3E70.inc"
};
EmDromeAppearPoint EmDrome_AppearPts27[4] = {
#include "game/008C3EA0.inc"
};
EmTurnParams EmDrome_TurnParams[2] = {
#include "game/008C3F00.inc"
};
EmDromeVec EmDrome_CarveThrowPt = {
#include "game/008C3F38.inc"
};
s32 func_0090A9C0(void); /* gm_sub */
void *EmDrome_Callbacks[8] = {
    EmDrome_SpawnEffect01, EmDrome_SpawnEffect01, EmDrome_TableNop, EmDrome_UpdateMotionEffects,
    func_0090A9C0, EmDrome_Reinit, NULL, NULL,
};
EmCurveKey EmDrome_VelYCurves_0[10] = {
#include "game/008C3F70.inc"
};
EmCurveKey EmDrome_VelYCurves_1[12] = {
#include "game/008C3FC0.inc"
};
EmCurveKey EmDrome_VelYCurves_2[8] = {
#include "game/008C4020.inc"
};
EmCurveKey EmDrome_VelYCurves_3[8] = {
#include "game/008C4060.inc"
};
EmCurveKey *EmDrome_VelYCurves[4] = {
    EmDrome_VelYCurves_0, EmDrome_VelYCurves_1, EmDrome_VelYCurves_2, EmDrome_VelYCurves_3,
};
EmCurveKey EmDrome_SpeedCurves_0[10] = {
#include "game/008C40B0.inc"
};
EmCurveKey EmDrome_SpeedCurves_2[6] = {
#include "game/008C4100.inc"
};
EmCurveKey *EmDrome_SpeedCurves[4] = {
    EmDrome_SpeedCurves_0, NULL, EmDrome_SpeedCurves_2, NULL,
};
EmDromeKindEntry EmKind1D_Variants[8] = {
#include "game/008C4140.inc"
};
void *EmKind1D_Callbacks[8] = {
    EmKind1D_TableNop, EmKind1D_TableNop, EmKind1D_TableNop, EmKind1D_UpdateMotionEffects,
    func_0090A9C0, EmKind1D_TableNop, NULL, NULL,
};
void *EmKirin_Callbacks[8] = {
    EmKirin_SpawnEffects, EmKirin_SpawnEffects, EmKirin_TableNop, EmKirin_UpdateMotionEffects,
    func_0090A9C0, EmKirin_TableNop, NULL, NULL,
};
