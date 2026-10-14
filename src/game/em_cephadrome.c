/*
 * Monster family: Cephadrome and Cephalos (kinds 8, 0x22).
 * The code from the family's setup function (0x7D43D0, called by the per-kind
 * dispatch Em_TypeSetup) to the next family's. The state functions take
 * the monster and the work area its state dispatcher passes on (EmWork);
 * Em.step is the step within the current state. See include/em.h.
 */
#include "common.h"
#include "quest.h"
#include "em.h"
#include "player.h"
#include "scene.h"
#include "objattach.h"
/* prototypes of callees (round 13) */
void Em_SetMotionRow(Em *em, s32 row, s32 arg2, s32 arg3);
void Em_SetPartScaled(Em *em, s32 bit);
s32 Em_ComputeSize(Em *em, s16 idx);
void StageObj_K09_Create(Em *);
u16 Rand_Next(s32 stream);

/* prototypes */
void EmCephadrome_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void EmCephadrome_RunGroup0(Em *em, EmWork *work);
void EmCephadrome_RunGroup2(Em *em, EmWork *work);
void EmCephadrome_RunGroup4(Em *em, EmWork *work);
void EmCephadrome_StartCurves(Em *em, u8 kind);
void EmMelynx_ChangeState(Em *em, u16 group, u16 state, u8 mode);
/* unit's own declaration: with the definition's void (Em *em, s16 n) (game/unk_708850.c), EmCephadrome_State0_03: 38 of 225 words differ */
void Em_AddHp(Em *em, s32 n);
void Em_BeginStatus1(Em *em);
void Em_BeginStatus2(Em *em);
void Em_BeginStatus4(Em *em);
void Em_BeginStatus5(Em *em);
void Em_EndAllStatus(Em *em);
void Em_EndStatus1(Em *em);
void Em_EndStatus2(Em *em);
void Em_EndStatus4(Em *em);
void Em_EndStatus5(Em *em);
void Em_PlaySeAtBone(Em *em, s32 se, s32 bone);
void Em_PlaySeAtBoneOnFrame(Em *em, s32 frame, s32 se, s32 bone);
void Em_SetLayerMotion(Em *em, s32 motion, s32 blend, s32 frame, s32 layer);
void Em_SetMotion(Em *em, s32 motion, s32 a, s32 b);
/* unit's own declaration: with the definition's void (Em *em, s32 group, s32 state, u8 mode) (game/unk_701810.c), EmCephadrome_ChangeState: 30 of 139 words differ, EmCephadrome_SetGroup0State: 1 of 106 words differ ... */
void Em_SetStateSync(Em *em, u16 group, u16 state, u8 mode);
u16 Math_AngleXZ(f32 *from, f32 *to);
void Player_ClearFlag(void *, u32 flag);
void Player_SetFlag(void *obj, u32 flag);
s8 QuestEm_Appear(Em *em);
void QuestEm_Captured(Em *em);
ObjAttach *Eff02_Create(Em *em, s8 kind);
void EffEm08_CreateAtBone(Em *, s32, s32, s32, s32, s32);
void EffEm14_Create(Em *, s8);
void EffCephadrome21_Create(Em *, s32);
void EffCephadrome23_Create(Em *, s32);
void Em_SetRow638(Em *em, u8 idx);
void Em_StartGoalAction(Em *em, s32 arg1);
void Em_MoveRotated(Em *em, void *src);
void Em_MoveRotatedAccel(Em *em, void *ang);
void Em_LoadReactionSet(Em *em, u8 k);
void Em_ComputeGoal(Em *em, f32 *target);
void Em_ComputeGoal2(Em *em, f32 *target);
void Em_SetRequest(Em *em, u8 a, u8 prio);
void Em_GetPitchToGoal(Em *em, s32 *out, f32 a, f32 b);
void Em_ClearLowHpNeed(Em *em);
void Em_Clear7ECState(Em *em);
void Em_Refill7F4(Em *em);
void Em_ResetNotice(Em *em);
void Em_SpawnEffect06Bone(Em *em, s32 a);
void Em_Add7EC(Em *em, s32 n);
void Em_SetFlag6AE(Em *em, u8 a);
/* unit's own declaration: with the definition's s32 (Em *em, s16 n, s16 max, s16 interval) (game/unk_708850.c), EmCephadrome_State0_12: 45 of 94 words differ */
s32 Em_RegenHp(Em *em, s32 n, s32 max, s32 interval);
void Em_ClearPoison(Em *em);
void Em_SetHold2(Em *em);
void Em_SetCaptured(Em *em, s32 a);
void Em_PlaySe6AtBoneInFrames(Em *em, s32 frame, s32 frame2, s32 se, s32 bone, s32 vol, s32 a);
void Em_PlaySe6AtBoneOnFrame(Em *em, s32 frame, s32 se, s32 bone);
void Em_PlaySeOnFrame(Em *em, s32 frame, s32 se);
void Em_SetSpeedScaled(Em *em, u16 flags, f32 x, f32 y);
s32 Em_StepToGoal(Em *em, u16 step, u8 flags);
void Em_StartTurnToTarget(Em *em, EmTurnParams *t);
s32 Em_TurnStep(Em *em, EmTurnParams *t);
void Em_RestartScript(Em *em);
void Em_TargetPlayerInRange(Em *);
void EmCephadrome_Reset(Em *);
void EmCephadrome_State3_06(Em *, EmWork *);
void EmCephadrome_State6_00_Entrance(Em *em, EmWork *work);
void EmCephadrome_State5_00_Hunted(Em *, EmWork *);
void EmCephadrome_State5_01_Hunted(Em *, EmWork *);
void EmCephadrome_State5_02_HuntedFalling(Em *, EmWork *);
void EmCephadrome_State5_05_HuntedFalling(Em *, EmWork *);
void EmCephadrome_MotionEffects(Em *, EmWork *);
void EmCephadrome_UpdateFins(Em *em, EmWork *work);
void EmCephadrome_SpawnSurfaceEffect2(Em *em, f32 scale);
void EmCephadrome_SpawnSurfaceEffect5(Em *em, f32 scale);
void EmCephadrome_VibrateNearby(Em *em);
u8 EmCephadrome_UpdateCurves(Em *);
u8 EmCephadrome_NextAreaPoint(Em *em, f32 *out);
void EmMelynx_Reset(Em *);
void EmMelynx_CapTimerOffTarget(Em *em);
void EmMelynx_State0_0C(Em *);
void EmMelynx_State0_0E(Em *);
void EmMelynx_State1_00(Em *);
void EmMelynx_State1_02(Em *);
void EmMelynx_State1_06(Em *);
void EmMelynx_State1_07(Em *);
void EmMelynx_State1_0B(Em *);
void EmMelynx_State3_01(Em *em);
void EmMelynx_State4_01(Em *);
void EmMelynx_State4_03(Em *);
void EmMelynx_State5_00_Hunted(Em *);
void EmMelynx_State5_01_Hunted(Em *);
void StageObj_K11_CreateAtBoneB(Em *em, s16 bone, s16 kind, u16 arg3);
void StageObj_K13_Create(void *owner, s16 v, u8 k);
void StageObj_K14_Create(Em *em, s16 k, s16 c, f32 scale);
void StageObj_Stun_CreateEm(Em *em, u8 k, s16 v, s16 t, f32 y, f32 scale);
s32 Sound_RandomSe(s32 id, s16 lo, s32 alt, s16 n);
s32 Player_PassedFrameScaled(void *p, f32 frame, s32 layer);
s32 EmMotion_ReachedFrame(void *obj, s32 a, f32 frame);
void Vec_RotateY(f32 *v, f32 angle);
f32 Vec_Distance(f32 *a, f32 *b);
void CamShake_ReqAIf(Em *obj, s32 i);
f32 *Mtx_FromAnglesXYZ(s32 *angles, f32 *m);
void Vec_Set(f32 *out, f32 x, f32 y, f32 z);
f32 Vec_DistanceXZ(f32 *a, f32 *b);
s32 Angle_TurnToward(s32 *cur, s32 *target, s32 step);
u16 Area_DayId(u16 id);
s32 QuestEm_CanAppear(Em *em);
void Quest_DeductReward(Player *p, u16 item, s16 n);
void Player_Integrate(void *);
void Player_ClearVelocity(void *);
void Motion_InitTime(void *p, s16 step);
u8 Player_InCurrentArea(void *obj);
void Effect01_Spawn(void *obj, s16 type);
void Effect08_Spawn(f32 *pos, s32 k, s32 b, f32 scale);
extern EmTurnParams EmCephadrome_TurnParams[3];
void EmCephadrome_State2_0A(Em *em, EmWork *work);
void EmCephadrome_State2_0B(Em *em, EmWork *work);
void EmCephadrome_State2_10(Em *em, EmWork *work);
void EmCephadrome_State2_11(Em *em, EmWork *work);
void EmCephadrome_State4_05(Em *em, EmWork *work, s16 variant);
void EmCephadrome_State4_06(Em *em, EmWork *work);
void EmCephadrome_State4_09(Em *em, EmWork *work);
void EmCephadrome_State4_11(Em *em, EmWork *work, s32 variant);
void EmCephadrome_State0_04(Em *em, EmWork *work, u8 variant);
void EmCephadrome_State0_0E(Em *em, EmWork *work);
void EmCephadrome_State0_10(Em *em, EmWork *work);
void QuestEm_Hunted(Em *em);
s8 EmMine_Create(Em *em, s32 idx);
s32 EmMine_Update(Em *em);
void EmMine_Remove(Em *em);
/* Melynx's own meanings of the work area's bytes (EmWork, include/em.h, has the shared ones). */
typedef struct EmMelynxWork {
    u8 faceStep;                /* 0x000: UpdateFace: 0, then 1 (seeds blinkTimer) */
    u8 unk001[0x004 - 0x001];
    s16 stateTimer;             /* 0x004: State1_00: 138; State4_03: 58, counted down */
    u8 unk006[0x00C - 0x006];
    u16 unk00C;                 /* 0x00C: Reset: 0; Update counts it down */
    s16 unk00E;                 /* 0x00E: ChangeState and some states: 1; State3_04, and Update when hit: 0; EndState keeps it across its ChangeState */
    u8 unk010[0x014 - 0x010];
    f32 home[3];                /* 0x014: Reset: the spawn position (posX, 0, posZ); State1_0C turns to it */
    ObjAttach *eff;             /* 0x020: State3_01: the Eff02_Create effect (kind 7 or 2) */
    Player *stolenFrom;         /* 0x024: StealItem: the player robbed; ReturnItem gives the item back to it; DeductReward passes it to Quest_DeductReward */
    EmObj sense;                /* 0x028: Em_SenseObjects' object; Update places it at the position when hit (radius 3000, kind 1) and adds it to ObjListA70 (alertState 1) */
    u8 unk044[0x048 - 0x044];
    u16 blinkTimer;             /* 0x048: UpdateFace: frames, the eyes' frame by (blinkTimer / 2) % 20 */
    s16 stolenItem;             /* 0x04A: the item StealItem took (Reset: -1, none); ReturnItem gives it back, DeductReward takes it from the quest's reward */
    s16 stolenCount;            /* 0x04C: StealItem: -1 (Pouch_Add takes one); ReturnItem adds -stolenCount */
    u8 eyeFrame;                /* 0x04E: UpdateFace: 0 open, 1 and 2 the blink, 3 in groups 4 and 5 */
    u8 alertState;              /* 0x04F: 0 calm; 1 when hit (sense listed); 2 alerted (Update: something sensed while calm, or sense removed) */
    u8 unk050[0x100 - 0x050];
} EmMelynxWork;
typedef char EmMelynxWork_size_check[(sizeof(EmMelynxWork) == sizeof(EmWork)) ? 1 : -1];

void EmMelynx_ReturnItem(Em *em, EmMelynxWork *w);
void ObjListA70_Remove(EmObj *obj);
void EmMine_Drop(Em *em, f32 *pos);
s32 Player_MoveFall(Player *p);
void Vec_Rotate(f32 *out, f32 *in, f32 *mtx);
extern EmCurveKey *EmCephadrome_VelYCurves[];
extern EmCurveKey EmCephadrome_Curves_2[];
extern EmCurveKey *EmCephadrome_Curves[];
void Em_SpawnEffect06Periodic(Em *em, s32 bone, f32 *pos, f32 scale);
u8 Em_TurnToGoal(Em *em, u16 step, u8 flags);
void EmCephadrome_StepRouteIndex(Em *em, u8 dir);
void Player_ClearAccel2(void *);
s8 EmMine_CreateTimed0(Em *em, f32 *pos);
/* unit's own declaration: with the definition's u8 (Player *p, u16 item, s16 n) (sub_main/pouch.c), EmMelynx_ReturnItem: 1 of 62 words differ */
s32 Pouch_Add(Player *p, u16 item, s16 n);
s32 Player_IsLocal(void *obj);
u8 Notice_Push(u8 kind, s16 a, s16 b);
void Vib_StartLocal(void *port, s32 i);
s32 Obj_SameArea(void *obj, Player *b);
void EmMelynx_MotionEffects(Em *em, EmWork *work);
s32 Quest_IsRunning(s32 ending);
s32 Player_HasSkill(Player *p, u8 skill);
s16 Pouch_Count(Player *p, u16 item);
void GmEm_ApplySize(Em *em);
extern s16 gEmCombatInit[];
void Em_InitParts(Em *em);
s32 EmMelynx_StealItem(Em *em, Player *p, EmMelynxWork *w);
s16 Player_IsState(void *obj, u8 state, u8 sub);
void Em_CheckRageByHp(Em *em, s16 n);
u8 Em_SenseObjects(Em *em, EmObj *self);
s32 Em_ScriptFlagsFrom(Em *em, u8 mask);
s32 ObjListA70_Add(EmObj *obj);
void Em_ResetPartHp(Em *em, u32 i);
void Em_SetStatus5Timer(Em *em);
void Em_SetStatus4Timer(Em *em);
void Em_SetStatus2Timer(Em *em);
s32 Em_UpdateCombatTimer(Em *em);
u8 Em_UpdateDamage(Em *em, u8 *out);
void Em_UpdatePendingScript(Em *em);
void Em_ClearFlag6AE(Em *em);
void EmMelynx_RunState(Em *em);
void Em_RaiseTrigger80Lax(Em *em);
void Em_CheckLowHp(Em *em, f32 ratio);
void Em_ResetKindTimer(Em *em);
s32 EmCephadrome_ShouldEndState(Em *em);
void EmCephadrome_RunState(Em *em, s32 arg1);
typedef struct EmBEntry {
    s16 id;
    s16 n;
    f32 *pts;
} EmBEntry;
struct EmIdEntry;
struct EmIdEntry *Em_FindAreaEntry(Em *em, struct EmIdEntry *entry, s16 id);
void Effect0D_SpawnForEm(Em *em, s32 a, s32 b);
/* A set of meshes (0x8C bytes). */
typedef struct EmMelynxMeshSet {
    s32 unk0;
    s32 count;          /* 0x04 */
    s32 mesh[33];       /* 0x08 */
} EmMelynxMeshSet;
s32 Gs_SetState(u8 reg, u32 v);
s32 EmMotion_InFrameRange(void *em, s32 layer, f32 from, f32 to);
void Effect0D_SpawnForEmScaled(Em *em, s32 a, s32 b, f32 scale);
void EmCephadrome_SpawnK14Type0B(Em *em);
void EmCephadrome_MotionSounds(Em *em, EmWork *work);
void EmCephadrome_SetFinMode(Em *em, EmWork *work, s32 mode);
void EmCephadrome_SpawnEffect21(Em *em, s32 arg1);
/* end of prototypes */

/* Cephadrome's own meanings of the work area's bytes (EmWork, include/em.h, has the shared ones). */
typedef struct EmCephadromeWork {
    u8 fxStep;                  /* 0x000: UpdateMotionEffects: 0, then 1 */
    u8 unk001[0x007 - 0x001];
    u8 unk007;                  /* 0x007: State2_0A counts it down */
    u8 unk008[0x00A - 0x008];
    u16 unk00A;                 /* 0x00A: an angle: Reset 0x4000; State6_00_Entrance: Math_AngleXZ to the goal */
    u8 unk00C[0x00F - 0x00C];
    u8 finMode;                 /* 0x00F: SetFinMode */
    s16 unk010;                 /* 0x010 */
    u8 unk012[0x013 - 0x012];
    u8 unk013;                  /* 0x013 */
    s32 unk014;                 /* 0x014 */
    s32 unk018;                 /* 0x018 */
    s32 moveAngBX;              /* 0x01C: angles X, Y, Z for Em_MoveRotatedAccel (UpdateCurves: 0, rotY, 0) */
    s32 moveAngBY;              /* 0x020 */
    s32 moveAngBZ;              /* 0x024 */
    s32 unk028;                 /* 0x028: Reset: 0x100 */
    s32 unk02C;                 /* 0x02C: Reset: 0x200 */
    s32 unk030;                 /* 0x030: Reset: 0x100 */
    f32 distLeft;               /* 0x034: the distance left to the goal (Vec_DistanceXZ, or 1000), less velZ each frame */
    u8 unk038;                  /* 0x038 */
    u8 unk039;                  /* 0x039 */
    u8 unk03A;                  /* 0x03A */
    u8 unk03B[0x03C - 0x03B];
    u8 curveNone;               /* 0x03C: the three curves' run flags (StartCurves, UpdateCurves) */
    u8 curveVelY;               /* 0x03D */
    u8 curveSpeed;              /* 0x03E */
    u8 curveKind;               /* 0x03F: the curve tables' index (StartCurves) */
    s16 curveFrame;             /* 0x040: the curve frame (UpdateCurves adds the motion's step) */
    s16 unk042;                 /* 0x042 */
    s16 unk044;                 /* 0x044 */
    s16 unk046;                 /* 0x046 */
    u16 unk048;                 /* 0x048 */
    u8 unk04A;                  /* 0x04A */
    u8 unk04B;                  /* 0x04B */
    s8 unk04C;                  /* 0x04C */
    s8 unk04D;                  /* 0x04D */
    u8 unk04E;                  /* 0x04E */
    u8 unk04F;                  /* 0x04F */
    s8 unk050;                  /* 0x050 */
    s8 unk051;                  /* 0x051 */
    s16 unk052;                 /* 0x052 */
    s16 unk054;                 /* 0x054 */
    s16 unk056;                 /* 0x056 */
    s16 unk058;                 /* 0x058 */
    s16 unk05A;                 /* 0x05A */
    s16 unk05C;                 /* 0x05C */
    s16 unk05E;                 /* 0x05E */
    s16 unk060;                 /* 0x060 */
    u8 unk062[0x100 - 0x062];
} EmCephadromeWork;
typedef char EmCephadromeWork_size_check[(sizeof(EmCephadromeWork) == sizeof(EmWork)) ? 1 : -1];

void EmCephadrome_Setup(Em *em)
{
    if (em->type == 8) {
        switch (Area_DayId(em->area)) {
        case 0x92:
        case 0x95:
            Em_SetCombat(em, 0, 0);
            em->noReact = 1;
            em->senseDelay = 5;
            em->noHitTimer = 5;
            em->noPushTimer = 5;
            break;
        }
    }
}

void EmCephadrome_SpawnEffects(void *em)
{
    StageObj_K13_Create(em, 0x22, 0);
    Effect01_Spawn((Player *) em, 0);
}

void EmCephadrome_Reset(Em *em)
{
    EmCephadromeWork *w = (EmCephadromeWork *)&em->work;

    s16 size;
    u8 fxFlags;

    Em_SetMotionRow(em, 1, 0, 0);
    Em_SetPartScaled(em, 3);
    Em_SetPartScaled(em, 4);
    w->unk010 = 0;
    em->scriptPending = 1;
    em->blind = 0;
    size = Em_ComputeSize(em, 0);
    em->hp = size;
    em->hpMax = size;
    if (em->type == 8) {
        em->unk_InitFlag = 1;
    } else {
        em->remote = 0;
        em->unk_InitFlag = 0;
    }
    em->noAlert = 0;
    em->painted = 0;
    w->unk00A = 0x4000;
    w->unk028 = 0x100;
    w->unk02C = 0x200;
    w->unk030 = 0x100;
    w->unk018 = 0x2000;
    w->unk014 = 0;
    w->unk038 = 0;
    w->unk039 = 0;
    w->unk03A = 0;
    w->finMode = 0;
    w->unk042 = 0;
    w->unk04A = 0;
    w->unk04B = 0;
    w->unk052 = 0;
    w->unk054 = 0;
    w->unk044 = 0;
    w->unk04C = 0;
    w->unk04D = 0;
    w->unk056 = 0;
    w->unk058 = 0;
    w->unk046 = 0;
    w->unk04E = 0;
    w->unk04F = 0;
    w->unk05A = 0;
    w->unk05C = 0;
    w->unk048 = 0;
    w->unk050 = 0;
    w->unk051 = 0;
    w->unk05E = 0;
    w->unk060 = 0;
    if (em->type == 8) {
        *(f32 *) &em->surfaceDepth = 450.0f;
    } else {
        *(f32 *) &em->surfaceDepth = 250.0f;
    }
    fxFlags = em->fxFlags & 1;
    em->fxFlags = fxFlags;
    if (fxFlags == 0) {
        switch (em->type) {
        case 8:
        case 0x22:
            em->meshShown[1] = 0;
            StageObj_K09_Create(em);
            break;
        }
    }
}

void EmCephadrome_Start(Em *em)
{
    f32 fslot;
    u8 slotNo;

    EmCephadrome_Reset(em);
    if (gQuest.stage == 0) {
        slotNo = em->slotNo;
        fslot = (f32) (u32) slotNo;
        em->posX = 5000.0f + (1500.0f * fslot);
        em->posY = 0.0f;
        em->posZ = 9000.0f;
        em->stance = 0;
        EmCephadrome_ChangeState(em, 2, 0, 0);
        return;
    }
    em->stance = 4;
    EmCephadrome_ChangeState(em, 2, 0, 0);
}

void EmCephadrome_Reinit(Em *em)
{
    EmCephadrome_Reset(em);
}

void EmCephadrome_EndState(Em *em)
{
    em->motionSpeed = 1.0f;
    em->stance = 0;
    em->lookAtTarget = 0;
    em->scriptPending = 1;
    EmCephadrome_ChangeState(em, 0, 1, 0);
}

void EmCephadrome_EndState2(Em *em)
{
    em->motionSpeed = 1.0f;
    em->stance = 4;
    em->lookAtTarget = 0;
    em->scriptPending = 1;
    EmCephadrome_ChangeState(em, 2, 0, 0);
}

void EmCephadrome_State0_00(Em *em, EmWork *work)
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

void EmCephadrome_State0_01(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
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
            EmCephadrome_EndState(em);
        }
        break;
    }
}

void EmCephadrome_State0_02(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
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
            EmCephadrome_EndState(em);
        }
        break;
    }
}

void EmCephadrome_State0_03(Em *em, EmWork *work)
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
            Em_SetRow638(em, 0);
            Em_LoadReactionSet(em, 0);
            Em_Clear7ECState(em);
            EmCephadrome_EndState(em);
        }
        break;
    }
    Em_Add7EC(em, 0xDE);
}

void EmCephadrome_State0_04(Em *em, EmWork *work, u8 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
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
            EmCephadrome_EndState(em);
        }
        break;
    }
}

void EmCephadrome_State0_05(Em *em, s32 unused, s32 variant)
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
            if (variant == 0) {
                EmCephadrome_ChangeState(em, 0, 6, 4);
                break;
            }
            EmCephadrome_ChangeState(em, 0, 0x2A, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            if (variant == 0) {
                EmCephadrome_ChangeState(em, 0, 6, 4);
                break;
            }
            EmCephadrome_ChangeState(em, 0, 0x2A, 4);
        }
        break;
    }
}

void EmCephadrome_State0_06(Em *em)
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
            EmCephadrome_ChangeState(em, 0, 7, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmCephadrome_ChangeState(em, 0, 7, 4);
        }
        break;
    }
}

void EmCephadrome_State0_07(Em *em)
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
            EmCephadrome_EndState(em);
        }
        break;
    }
}

void EmCephadrome_State0_0E(Em *em, EmWork *work)
{
    f32 pos[3];
    f32 pos2[3];

    Em_SetFlag6AE(em, 2);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 1;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x4B, 0, 0);
        Em_LoadReactionSet(em, 1);
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
            EmCephadrome_ChangeState(em, 0, 0xF, 4);
        }
        pos[0] = 0.0f;
        pos[1] = 10.0f;
        pos[2] = 140.0f;
        Em_SpawnEffect06Periodic(em, 0x22, pos, 1.6f);
        if (em->timer % 135 == 0) {
            Em_PlaySeAtBone(em, 0x57, 0x23);
        }
        break;
    case 4:
        if (em->remote == 0) {
            EmCephadrome_ChangeState(em, 0, 0xF, 4);
        }
        pos2[0] = 0.0f;
        pos2[1] = 10.0f;
        pos2[2] = 140.0f;
        Em_SpawnEffect06Periodic(em, 0x22, pos2, 1.6f);
        if (em->timer % 135 == 0) {
            Em_PlaySeAtBone(em, 0x57, 0x23);
        }
        break;
    }
}

void EmCephadrome_State0_0F(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x7C, 0, 0);
        Em_LoadReactionSet(em, 0);
        em->blind = 2;
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            EmCephadrome_ChangeState(em, 0, 7, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmCephadrome_ChangeState(em, 0, 7, 4);
        }
        break;
    }
}

void EmCephadrome_State0_10(Em *em, EmWork *work)
{
    f32 pos[3];
    f32 pos2[3];

    Em_SetFlag6AE(em, 2);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x4B, 0, 0);
        Em_LoadReactionSet(em, 1);
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
            EmCephadrome_ChangeState(em, 0, 0x11, 4);
        }
        pos[0] = 0.0f;
        pos[1] = 10.0f;
        pos[2] = 140.0f;
        Em_SpawnEffect06Periodic(em, 0x22, pos, 1.6f);
        if (em->timer % 135 == 0) {
            Em_PlaySeAtBone(em, 0x57, 0x23);
        }
        break;
    case 4:
        if (em->remote == 0) {
            EmCephadrome_ChangeState(em, 0, 0x11, 4);
        }
        pos2[0] = 0.0f;
        pos2[1] = 10.0f;
        pos2[2] = 140.0f;
        Em_SpawnEffect06Periodic(em, 0x22, pos2, 1.6f);
        if (em->timer % 135 == 0) {
            Em_PlaySeAtBone(em, 0x57, 0x23);
        }
        break;
    }
}

void EmCephadrome_State0_12(Em *em)
{
    Em_SetFlag6AE(em, 2U);
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
        Em_RegenHp(em, 1, (s32) (0.3f * (f32) em->hpMax), 6);
        if (--em->timer <= 0) {
            em->step++;
            Em_ClearLowHpNeed(em);
            EmCephadrome_ChangeState(em, 0, 0x13, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmCephadrome_ChangeState(em, 0, 0x13, 4);
        }
        break;
    }
}

void EmCephadrome_State0_13(Em *em)
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
            EmCephadrome_ChangeState(em, 0, 7, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmCephadrome_ChangeState(em, 0, 7, 4);
        }
        break;
    }
}

void EmCephadrome_State0_14(Em *em)
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
            EmCephadrome_ChangeState(em, 0, 0x28, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmCephadrome_ChangeState(em, 0, 0x28, 4);
        }
        break;
    }
}

void EmCephadrome_State0_28(Em *em)
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
            EmCephadrome_EndState(em);
        }
        break;
    }
}

void EmCephadrome_State1_00(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 0;
        Em_SetMotion(em, 3, 0, 0);
        break;
    case 1:
        if (Em_StepToGoal(em, 0x40, 0) == 1) {
            em->step++;
            EmCephadrome_EndState(em);
        }
        break;
    }
}

void EmCephadrome_State1_01(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 0;
        Em_SetMotion(em, 4, 0, 0);
        break;
    case 1:
        if (Em_StepToGoal(em, 0x40, 0) == 1) {
            em->step++;
            Em_SetMotion(em, 0x85, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmCephadrome_EndState(em);
        }
        break;
    }
}

void EmCephadrome_State1_02(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 0;
        Em_StartTurnToTarget(em, &EmCephadrome_TurnParams[0]);
        break;
    case 1:
        if (Em_TurnStep(em, &EmCephadrome_TurnParams[0]) == 1) {
            em->step++;
            Player_ClearFlag(em, 0x20000);
            EmCephadrome_EndState(em);
        }
        break;
    }
}

void EmCephadrome_State1_03(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 0;
        Em_SetMotion(em, 0xA, 0, 0);
        break;
    case 1:
        if (Em_StepToGoal(em, 0x40, 0) == 1) {
            em->step++;
            EmCephadrome_EndState(em);
        }
        break;
    }
}

void EmCephadrome_State2_00(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x64, 0, 0);
        em->stance = 4;
        break;
    case 1:
        if ((em->remote == 0) && (em->motion[0].playing == 0)) {
            em->step++;
            EmCephadrome_EndState2(em);
        }
        break;
    }
}

void EmCephadrome_State2_01(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 4;
        Em_SetMotion(em, 0x6D, 0, 0);
        break;
    case 1:
        if ((em->remote == 0) && (em->motion[0].playing == 0)) {
            em->step++;
            EmCephadrome_EndState2(em);
        }
        break;
    }
}

void EmCephadrome_State2_02(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 4;
        Em_SetMotion(em, 0x72, 0, 0);
        em->timer = 0x84;
        Player_ClearVelocity(em);
        em->velZ = 40.0f;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmCephadrome_EndState2(em);
        }
        em->rotY += 0x1F0;
        Em_GetPitchToGoal(em, (s32 *) &work->moveAngBX, 0.0f, 0.0f);
        work->moveAngBY = em->rotY;
        work->moveAngBZ = 0;
        Em_MoveRotated(em, &work->moveAngBX);
        EmCephadrome_SpawnSurfaceEffect2(em, 8.0f);
        break;
    }
}

void EmCephadrome_State2_03(Em *em, s32 unused, s32 variant)
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
        em->lookAtTarget = 0;
        em->stance = 4;
        Em_StartTurnToTarget(em, &EmCephadrome_TurnParams[1]);
        EmCephadrome_SpawnSurfaceEffect5(em, 8.0f);
        break;
    case 1:
        if (Em_TurnStep(em, &EmCephadrome_TurnParams[1]) == 1) {
            em->step++;
            EmCephadrome_EndState2(em);
        }
        break;
    }
}

void EmCephadrome_State2_04(Em *em, EmWork *work)
{

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 4;
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
            em->step = 0xA;
            EmCephadrome_EndState2(em);
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
            em->step++;
            EmCephadrome_EndState2(em);
        }
        break;
    }
}

void EmCephadrome_State2_05(Em *em, EmWork *work, s32 variant)
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
        em->lookAtTarget = 0;
        em->stance = 4;
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
                EmCephadrome_EndState2(em);
            }
            EmCephadrome_SpawnSurfaceEffect2(em, 8.0f);
        }
        break;
    }
}

void EmCephadrome_State2_06(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 4;
        Em_SetMotion(em, 0x6E, 0, 0);
        EmCephadrome_StartCurves(em, 0U);
        break;
    case 1:
        if (EmCephadrome_UpdateCurves(em)) {
            em->step++;
            EmCephadrome_EndState2(em);
        }
        break;
    }
}

void EmCephadrome_State2_07(Em *em, EmWork *work)
{
    f32 posY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 4;
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
        Em_MoveRotatedAccel(em, (s32 *) &work->moveAngBX);
        if (!(em->velY <= 50.0f)) {
            em->velY = 50.0f;
            em->accelY = 0.0f;
        }
        posY = em->surfaceY - *(f32 *)&em->surfaceDepth;
        if (!(em->posY <= posY)) {
            em->posY = posY;
            em->step++;
            Em_SetMotion(em, 0x86, 0, 0);
            EmCephadrome_SpawnSurfaceEffect2(em, 8.0f);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmCephadrome_EndState2(em);
        }
        break;
    }
}

void EmCephadrome_State2_08(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 4;
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
            EmCephadrome_EndState2(em);
        }
        break;
    }
}

void EmCephadrome_State2_09(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 4;
        Em_StartTurnToTarget(em, &EmCephadrome_TurnParams[2]);
        EmCephadrome_SpawnSurfaceEffect5(em, 8.0f);
        break;
    case 1:
        if (Em_TurnStep(em, &EmCephadrome_TurnParams[2]) == 1) {
            em->step++;
            em->timer = 0x12C;
            EmCephadrome_ChangeState(em, 2, 0xA, 1);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->timer = 0x12C;
            if (em->remote == 0) {
                EmCephadrome_ChangeState(em, 2, 0xA, 1);
            }
        }
        break;
    }
}

void EmCephadrome_State2_0A(Em *em, EmWork *work)
{
    f32 v;
    u8 temp;

    switch (em->step) {
    case 0:
        em->stance = 4;
        Em_SetMotion(em, 0x67, 0, 0);
        em->step++;
        em->stateVar1 = 0;
        em->lookAtTarget = 0;
        em->velX = 0.0f;
        em->velY = 0.0f;
        Player_ClearAccel2(em);
        v = 50.0f;
        if (em->type == 0x22 && (em->stateRand & 0x1F) < 0x10) {
            v *= 0.8f;
        }
        if (em->velZ <= v) {
            em->velZ = v;
        }
        break;
    case 1:
        if (!(Vec_DistanceXZ(&em->posX, em->goal) <= 300.0f)) {
            if (--em->timer <= 0) {
                goto block_9;
            }
        } else {
block_9:
            temp = ((EmCephadromeWork *)work)->unk007 - 1;
            ((EmCephadromeWork *)work)->unk007 = temp;
            if ((temp & 0xFF) <= 0) {
                em->step++;
                EmCephadrome_EndState2(em);
            } else {
                EmCephadrome_StepRouteIndex(em, 0);
                Em_ComputeGoal2(em, em->goal);
                em->timer = (s32) ((1.5f * Vec_DistanceXZ(&em->posX, em->goal)) / 50.0f);
                EmCephadrome_ChangeState(em, 2, 0x13, 0);
            }
        }
        Em_TurnToGoal(em, 0x200, 0);
        Em_GetPitchToGoal(em, (s32 *) &work->moveAngBX, 0.0f, 0.0f);
        work->moveAngBY = em->rotY;
        work->moveAngBZ = 0;
        Em_MoveRotated(em, &work->moveAngBX);
        EmCephadrome_SpawnSurfaceEffect2(em, 8.0f);
        break;
    }
}

void EmCephadrome_State2_0B(Em *em, EmWork *work)
{
    f32 v;
    u8 temp;

    switch (em->step) {
    case 0:
        em->stance = 4;
        Em_SetMotion(em, 0x67, 0, 0);
        em->step++;
        em->stateVar1 = 0;
        em->lookAtTarget = 0;
        em->velX = 0.0f;
        em->velY = 0.0f;
        Player_ClearAccel2(em);
        v = 50.0f;
        if (em->type == 0x22 && (em->stateRand & 0x1F) < 0x10) {
            v *= 0.8f;
        }
        if (em->velZ <= v) {
            em->velZ = v;
        }
        break;
    case 1:
        if (!(Vec_DistanceXZ(&em->posX, em->goal) <= 300.0f)) {
            if (--em->timer <= 0) {
                goto block_9;
            }
        } else {
block_9:
            temp = ((EmCephadromeWork *)work)->unk007 - 1;
            ((EmCephadromeWork *)work)->unk007 = temp;
            if ((temp & 0xFF) <= 0) {
                em->step++;
                EmCephadrome_EndState2(em);
            } else {
                EmCephadrome_StepRouteIndex(em, 1);
                Em_ComputeGoal2(em, em->goal);
                em->timer = (s32) ((1.5f * Vec_DistanceXZ(&em->posX, em->goal)) / 50.0f);
                EmCephadrome_ChangeState(em, 2, 0x14, 0);
            }
        }
        Em_TurnToGoal(em, 0x200, 0);
        Em_GetPitchToGoal(em, (s32 *) &work->moveAngBX, 0.0f, 0.0f);
        work->moveAngBY = em->rotY;
        work->moveAngBZ = 0;
        Em_MoveRotated(em, &work->moveAngBX);
        EmCephadrome_SpawnSurfaceEffect2(em, 8.0f);
        break;
    }
}

void EmCephadrome_State2_0C(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 4;
        Em_SetMotion(em, 0x72, 0, 0);
        em->timer = 0x84;
        Player_ClearVelocity(em);
        em->velZ = 40.0f;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmCephadrome_EndState2(em);
        }
        em->rotY += 0x1F0;
        em->rotY = (s32) (u16) em->rotY;
        Em_GetPitchToGoal(em, (s32 *) &work->moveAngBX, 0.0f, 0.0f);
        work->moveAngBY = em->rotY;
        work->moveAngBZ = 0;
        Em_MoveRotated(em, &work->moveAngBX);
        EmCephadrome_SpawnSurfaceEffect2(em, 8.0f);
        break;
    }
}

void EmCephadrome_State2_0D(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 4;
        Em_SetMotion(em, 0x73, 0, 0);
        em->timer = 0x84;
        Player_ClearVelocity(em);
        em->velZ = 40.0f;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmCephadrome_EndState2(em);
        }
        em->rotY -= 0x1F0;
        em->rotY = (s32) (u16) em->rotY;
        Em_GetPitchToGoal(em, (s32 *) &work->moveAngBX, 0.0f, 0.0f);
        work->moveAngBY = em->rotY;
        work->moveAngBZ = 0;
        Em_MoveRotated(em, &work->moveAngBX);
        EmCephadrome_SpawnSurfaceEffect2(em, 8.0f);
        break;
    }
}

void EmCephadrome_State2_0E(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 4;
        Player_ClearVelocity(em);
        Em_SetMotion(em, 0x77, 0, 0);
        em->velY = 132.0f;
        em->velZ = 60.0f;
        work->moveAngBX = 0;
        work->moveAngBZ = 0;
        break;
    case 1:
        work->moveAngBY = em->rotY;
        Em_MoveRotated(em, (s32 *) &work->moveAngBX);
        if (!(em->posY < (em->surfaceY - *(f32 *)&em->surfaceDepth))) {
            em->step++;
            em->stance = 2;
            em->velY = 50.0f;
            em->accelY = -5.0f;
            EmCephadrome_SpawnSurfaceEffect5(em, 8.0f);
            EmCephadrome_VibrateNearby(em);
            CamShake_ReqAIf(em, 3);
        }
        break;
    case 2:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, (s32 *) &work->moveAngBX);
        if ((em->velY < 0.0f) && (em->posY < em->floorY)) {
            em->step++;
            em->posY = em->floorY;
            EmCephadrome_ChangeState(em, 0, 4, 4);
            EmCephadrome_SpawnSurfaceEffect5(em, 8.0f);
        }
        break;
    case 3:
        if (em->remote == 0) {
            em->posY = em->floorY;
            EmCephadrome_ChangeState(em, 0, 4, 4);
        }
        break;
    }
}

void EmCephadrome_State2_0F(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 4;
        Em_SetMotion(em, 0x79, 0, 0);
        Player_ClearVelocity(em);
        em->posY = em->surfaceY - *(f32 *)&em->surfaceDepth;
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 10.0f, 0U) != 0) {
            em->step++;
            EmCephadrome_EndState2(em);
        }
        break;
    }
}

void EmCephadrome_State2_10(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0x0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 0;
        Em_SetMotion(em, 0x78, 0, 0);
        Player_ClearVelocity(em);
        break;
    case 0x1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stance = 2;
            Em_SetMotion(em, 0x77, 0, 0);
            em->velY = 50.0f;
            em->velZ = 80.0f;
            em->accelY = -4.0f;
            EmCephadrome_SpawnSurfaceEffect5(em, 8.0f);
        }
        break;
    case 0x2:
        work->moveAngBX = 0;
        work->moveAngBY = em->rotY;
        work->moveAngBZ = 0;
        Em_MoveRotatedAccel(em, (s32 *) &work->moveAngBX);
        if (em->posY <= (em->surfaceY - *(f32 *)&em->surfaceDepth)) {
            em->stance = 4;
        }
        if (em->velY < 0.0f) {
            if (gScene.area == em->area) {
                if (em->surfaceKind != 0) {
                    if (em->posY < (em->surfaceY - *(f32 *)&em->surfaceDepth)) {
                        em->step++;
                        em->stance = 4;
                        em->posY = em->surfaceY - *(f32 *)&em->surfaceDepth;
                        em->velY = 0.0f;
                        em->accelY = 0.0f;
                        Em_SetMotion(em, 0x67, 0xA, 0);
                        ((EmCephadromeWork *)work)->distLeft = 1000.0f;
                        em->timer = 0x3C;
                        em->accelZ = (((EmCephadromeWork *)work)->distLeft - em->timer * em->velZ) / ((em->timer * em->timer) / 2);
                        EmCephadrome_SpawnSurfaceEffect5(em, 8.0f);
                    }
                } else if (em->posY < em->floorY) {
                    em->step = 0x63;
                    em->posY = em->floorY;
                    EmCephadrome_ChangeState(em, 0, 4, 4);
                    EmCephadrome_SpawnSurfaceEffect5(em, 8.0f);
                }
            } else {
                if (em->posY < (em->surfaceY - *(f32 *)&em->surfaceDepth)) {
                    em->step++;
                    em->stance = 4;
                    em->posY = em->surfaceY - *(f32 *)&em->surfaceDepth;
                    em->velY = 0.0f;
                    em->accelY = 0.0f;
                    Em_SetMotion(em, 0x67, 0xA, 0);
                    ((EmCephadromeWork *)work)->distLeft = 1000.0f;
                    em->timer = 0x3C;
                    em->accelZ = (((EmCephadromeWork *)work)->distLeft - em->timer * em->velZ) / ((em->timer * em->timer) / 2);
                    EmCephadrome_SpawnSurfaceEffect5(em, 8.0f);
                }
            }
        }
        break;
    case 0x3:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, (s32 *) &work->moveAngBX);
        ((EmCephadromeWork *)work)->distLeft -= em->velZ;
        if (((EmCephadromeWork *)work)->distLeft <= 0.0f || em->velZ <= 0.0f) {
            em->step++;
            EmCephadrome_EndState2(em);
        }
        break;
    case 0x63:
        if (em->remote == 0) {
            em->posY = em->floorY;
            EmCephadrome_ChangeState(em, 0, 4, 4);
            EmCephadrome_SpawnSurfaceEffect5(em, 8.0f);
        }
        break;
    }
}

void EmCephadrome_State2_11(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0x0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 4;
        Player_ClearVelocity(em);
        Em_SetMotion(em, 0x77, 0, 0);
        em->velY = 100.0f;
        em->velZ = 60.0f;
        em->accelY = -8.0f;
        work->moveAngBX = 0;
        work->moveAngBZ = 0;
        break;
    case 0x1:
        work->moveAngBY = em->rotY;
        Em_MoveRotated(em, &work->moveAngBX);
        if (!(em->posY <= (em->surfaceY - *(f32 *)&em->surfaceDepth))) {
            em->stance = 2;
            EmCephadrome_SpawnSurfaceEffect5(em, 8.0f);
            em->step++;
            EmCephadrome_VibrateNearby(em);
            CamShake_ReqAIf(em, 3);
        }
        break;
    case 0x2:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, (s32 *) &work->moveAngBX);
        if (em->posY < (em->surfaceY - *(f32 *)&em->surfaceDepth)) {
            em->stance = 4;
        }
        if (em->velY < 0.0f) {
            if (gScene.area == em->area) {
                if (em->surfaceKind != 0) {
                    if (em->posY < (em->surfaceY - *(f32 *)&em->surfaceDepth)) {
                        em->step++;
                        em->stance = 4;
                        em->posY = em->surfaceY - *(f32 *)&em->surfaceDepth;
                        em->velY = 0.0f;
                        em->accelY = 0.0f;
                        Em_SetMotion(em, 0x67, 0xA, 0);
                        ((EmCephadromeWork *)work)->distLeft = 1000.0f;
                        em->timer = 0x3C;
                        em->accelZ = (((EmCephadromeWork *)work)->distLeft - em->timer * em->velZ) / ((em->timer * em->timer) / 2);
                        EmCephadrome_SpawnSurfaceEffect5(em, 8.0f);
                    }
                } else if (em->posY < em->floorY) {
                    em->step = 0x63;
                    em->posY = em->floorY;
                    EmCephadrome_ChangeState(em, 0, 4, 4);
                    EmCephadrome_SpawnSurfaceEffect5(em, 8.0f);
                }
            } else {
                if (em->posY < (em->surfaceY - *(f32 *)&em->surfaceDepth)) {
                    em->step++;
                    em->posY = em->surfaceY - *(f32 *)&em->surfaceDepth;
                    em->stance = 4;
                    em->velY = 0.0f;
                    em->accelY = 0.0f;
                    Em_SetMotion(em, 0x67, 0xA, 0);
                    ((EmCephadromeWork *)work)->distLeft = 1000.0f;
                    em->timer = 0x3C;
                    em->accelZ = (((EmCephadromeWork *)work)->distLeft - em->timer * em->velZ) / ((em->timer * em->timer) / 2);
                    EmCephadrome_SpawnSurfaceEffect5(em, 8.0f);
                }
            }
        }
        break;
    case 0x3:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, (s32 *) &work->moveAngBX);
        ((EmCephadromeWork *)work)->distLeft -= em->velZ;
        if (((EmCephadromeWork *)work)->distLeft <= 0.0f || em->velZ <= 0.0f) {
            em->step++;
            EmCephadrome_EndState2(em);
        }
        break;
    case 0x63:
        if (em->remote == 0) {
            em->posY = em->floorY;
            EmCephadrome_ChangeState(em, 0, 4, 4);
            EmCephadrome_SpawnSurfaceEffect5(em, 8.0f);
        }
        break;
    }
}

void EmCephadrome_State2_12(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 0;
        Em_SetMotion(em, 0x78, 0, 0);
        Player_ClearVelocity(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stance = 2;
            Em_SetMotion(em, 0x77, 0, 0);
            em->velY = 50.0f;
            em->velZ = 100.0f;
            em->accelY = -4.0f;
            EmCephadrome_SpawnSurfaceEffect5(em, 8.0f);
        }
        break;
    case 2:
        work->moveAngBX = 0;
        work->moveAngBY = em->rotY;
        work->moveAngBZ = 0;
        Em_MoveRotatedAccel(em, &work->moveAngBX);
        if ((em->velY < 0.0f) && (em->posY < em->floorY)) {
            em->step++;
            em->posY = em->floorY;
            EmCephadrome_ChangeState(em, 0, 4, 4);
            EmCephadrome_SpawnSurfaceEffect5(em, 8.0f);
        }
        break;
    case 3:
        if (em->remote == 0) {
            em->posY = em->floorY;
            EmCephadrome_ChangeState(em, 0, 4, 4);
            EmCephadrome_SpawnSurfaceEffect5(em, 8.0f);
        }
        break;
    }
}

void EmCephadrome_State2_15(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 4;
        Em_SetMotion(em, 0x83, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmCephadrome_EndState2(em);
        }
        break;
    }
}

void EmCephadrome_State2_16(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        Player_ClearVelocity(em);
        Em_SetMotion(em, 0x77, 0, 0);
        em->timer = 0x96;
        em->goal[0] = 6000.0f;
        em->goal[1] = -1000.0f;
        em->goal[2] = 10000.0f;
        em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        em->accelY = -2.0f;
        em->velY = ((em->goal[1] - em->posY) / (f32) em->timer) - ((em->accelY * (f32) em->timer) / 2.0f);
        em->velZ = Vec_DistanceXZ(&em->posX, em->goal) / (f32) em->timer;
        work->moveAngBX = 0;
        work->moveAngBZ = 0;
        break;
    case 1:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, (s32 *) &work->moveAngBX);
        if (--em->timer <= 0) {
            em->step++;
            em->posY = -1000.0f;
            EmCephadrome_ChangeState(em, 2, 0, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            em->posY = -1000.0f;
            EmCephadrome_ChangeState(em, 2, 0, 4);
        }
        break;
    }
}

void EmCephadrome_State2_18(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 4;
        em->blind = 2;
        Em_SetMotion(em, 0x83, 0, 0);
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            EmCephadrome_EndState2(em);
        }
        break;
    }
}

void EmCephadrome_State3_00(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 0;
        Em_SetMotion(em, 0x24, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmCephadrome_EndState(em);
        }
        break;
    }
}

void EmCephadrome_State3_01(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 0;
        Em_SetMotion(em, 0x2F, 0, 0);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 78.0f, 0U) != 0) {
            StageObj_K11_CreateAtBoneB((Em *) em, 0x22, 0xC, 0x71C);
            if (em->type == 8) {
                EffEm08_CreateAtBone(em, 0x22, 7, 0, 0x71C, 0);
            } else {
                EffEm08_CreateAtBone(em, 0x22, 7, 1, 0x71C, 0);
            }
        }
        if (Player_PassedFrameScaled(em, 80.0f, 0U) != 0) {
            StageObj_K11_CreateAtBoneB((Em *) em, 0x22, 0xC, 0x71C);
        }
        if (Player_PassedFrameScaled(em, 82.0f, 0U) != 0) {
            StageObj_K11_CreateAtBoneB((Em *) em, 0x22, 0xC, 0x71C);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmCephadrome_EndState(em);
        }
        break;
    }
}

void EmCephadrome_State3_02(Em *em, EmWork *work)
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
            EmCephadrome_EndState(em);
        }
        break;
    }
}

void EmCephadrome_State3_03(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x74, 0, 0);
        em->stance = 4;
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 122.0f, 0U) != 0) {
            StageObj_K11_CreateAtBoneB((Em *) em, 0x22, 0xC, 0x1C72);
            EffEm08_CreateAtBone(em, 0x22, 7, 0, 0x11C7, 0x1C72);
        }
        if (Player_PassedFrameScaled(em, 162.0f, 0U) != 0) {
            StageObj_K11_CreateAtBoneB((Em *) em, 0x22, 0xC, 0);
            EffEm08_CreateAtBone(em, 0x22, 7, 0, 0x11C7, 0);
        }
        if (Player_PassedFrameScaled(em, 192.0f, 0U) != 0) {
            StageObj_K11_CreateAtBoneB((Em *) em, 0x22, 0xC, 0xE38F);
            EffEm08_CreateAtBone(em, 0x22, 7, 0, 0x11C7, 0xE38F);
        }
        if (Player_PassedFrameScaled(em, 38.0f, 0U) != 0) {
            EmCephadrome_SpawnSurfaceEffect5(em, 8.0f);
        }
        if (Player_PassedFrameScaled(em, 230.0f, 0U) != 0) {
            EmCephadrome_SpawnSurfaceEffect5(em, 8.0f);
        }
        if (Player_PassedFrameScaled(em, 126.0f, 0U) != 0) {
            StageObj_K11_CreateAtBoneB((Em *) em, 0x22, 0xC, 0x1C72);
        }
        if (Player_PassedFrameScaled(em, 166.0f, 0U) != 0) {
            StageObj_K11_CreateAtBoneB((Em *) em, 0x22, 0xC, 0);
        }
        if (Player_PassedFrameScaled(em, 196.0f, 0U) != 0) {
            StageObj_K11_CreateAtBoneB((Em *) em, 0x22, 0xC, 0xE38F);
        }
        if (Player_PassedFrameScaled(em, 130.0f, 0U) != 0) {
            StageObj_K11_CreateAtBoneB((Em *) em, 0x22, 0xC, 0x1C72);
        }
        if (Player_PassedFrameScaled(em, 170.0f, 0U) != 0) {
            StageObj_K11_CreateAtBoneB((Em *) em, 0x22, 0xC, 0);
        }
        if (Player_PassedFrameScaled(em, 200.0f, 0U) != 0) {
            StageObj_K11_CreateAtBoneB((Em *) em, 0x22, 0xC, 0xE38F);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmCephadrome_EndState2(em);
        }
        break;
    }
}

void EmCephadrome_State3_04(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x75, 0, 0);
        em->stance = 4;
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 122.0f, 0U) != 0) {
            StageObj_K11_CreateAtBoneB((Em *) em, 0x22, 0xC, 0x1C72);
            EffEm08_CreateAtBone(em, 0x22, 7, 0, 0x11C7, 0xE38F);
        }
        if (Player_PassedFrameScaled(em, 162.0f, 0U) != 0) {
            StageObj_K11_CreateAtBoneB((Em *) em, 0x22, 0xC, 0);
            EffEm08_CreateAtBone(em, 0x22, 7, 0, 0x11C7, 0);
        }
        if (Player_PassedFrameScaled(em, 192.0f, 0U) != 0) {
            StageObj_K11_CreateAtBoneB((Em *) em, 0x22, 0xC, 0xE38F);
            EffEm08_CreateAtBone(em, 0x22, 7, 0, 0x11C7, 0x1C72);
        }
        if (Player_PassedFrameScaled(em, 38.0f, 0U) != 0) {
            EmCephadrome_SpawnSurfaceEffect5(em, 8.0f);
        }
        if (Player_PassedFrameScaled(em, 230.0f, 0U) != 0) {
            EmCephadrome_SpawnSurfaceEffect5(em, 8.0f);
        }
        if (Player_PassedFrameScaled(em, 126.0f, 0U) != 0) {
            StageObj_K11_CreateAtBoneB((Em *) em, 0x22, 0xC, 0xE38F);
        }
        if (Player_PassedFrameScaled(em, 166.0f, 0U) != 0) {
            StageObj_K11_CreateAtBoneB((Em *) em, 0x22, 0xC, 0);
        }
        if (Player_PassedFrameScaled(em, 196.0f, 0U) != 0) {
            StageObj_K11_CreateAtBoneB((Em *) em, 0x22, 0xC, 0x1C72);
        }
        if (Player_PassedFrameScaled(em, 130.0f, 0U) != 0) {
            StageObj_K11_CreateAtBoneB((Em *) em, 0x22, 0xC, 0xE38F);
        }
        if (Player_PassedFrameScaled(em, 170.0f, 0U) != 0) {
            StageObj_K11_CreateAtBoneB((Em *) em, 0x22, 0xC, 0);
        }
        if (Player_PassedFrameScaled(em, 200.0f, 0U) != 0) {
            StageObj_K11_CreateAtBoneB((Em *) em, 0x22, 0xC, 0x1C72);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmCephadrome_EndState2(em);
        }
        break;
    }
}

void EmCephadrome_State3_05(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x70, 0, 0);
        em->stance = 4;
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 58.0f, 0U) != 0) {
            StageObj_K11_CreateAtBoneB((Em *) em, 0x22, 0xC, 0);
            if (em->type == 8) {
                EffEm08_CreateAtBone(em, 0x22, 7, 0, 0xE39, 0);
            } else {
                EffEm08_CreateAtBone(em, 0x22, 7, 1, 0xE39, 0);
            }
        }
        if (Player_PassedFrameScaled(em, 26.0f, 0U) != 0) {
            EmCephadrome_SpawnSurfaceEffect5(em, 8.0f);
        }
        if (Player_PassedFrameScaled(em, 98.0f, 0U) != 0) {
            EmCephadrome_SpawnSurfaceEffect5(em, 8.0f);
        }
        if (Player_PassedFrameScaled(em, 60.0f, 0U) != 0) {
            StageObj_K11_CreateAtBoneB((Em *) em, 0x22, 0xC, 0);
        }
        if (Player_PassedFrameScaled(em, 62.0f, 0U) != 0) {
            StageObj_K11_CreateAtBoneB((Em *) em, 0x22, 0xC, 0);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmCephadrome_EndState2(em);
        }
        break;
    }
}

void EmCephadrome_State3_06(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0x0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x6C, 0, 0);
        em->stance = 4;
        Player_ClearVelocity(em);
        em->velZ = 100.0f;
        em->velY = 80.0f;
        em->accelY = -4.5f;
        work->moveAngBX = 0;
        work->moveAngBZ = 0;
        break;
    case 0x1:
        work->moveAngBY = em->rotY;
        Em_MoveRotated(em, &work->moveAngBX);
        if (!(em->posY <= (em->surfaceY - *(f32 *)&em->surfaceDepth))) {
            em->step++;
            em->stance = 2;
            EmCephadrome_SpawnSurfaceEffect5(em, 8.0f);
            EmCephadrome_VibrateNearby(em);
            CamShake_ReqAIf(em, 3);
        }
        break;
    case 0x2:
        work->moveAngBY = em->rotY;
        Em_MoveRotated(em, &work->moveAngBX);
        if (!(em->posY <= em->surfaceY)) {
            em->step++;
            em->velY = 0.0f;
            em->timer = 0x1E;
        }
        break;
    case 0x3:
        work->moveAngBY = em->rotY;
        Em_MoveRotated(em, &work->moveAngBX);
        if (--em->timer <= 0) {
            em->step++;
        }
        break;
    case 0x4:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, (s32 *) &work->moveAngBX);
        if ((gScene.area != em->area) || (em->surfaceKind != 0)) {
            if ((em->posY <= (em->surfaceY - *(f32 *)&em->surfaceDepth)) && (em->velY < 0.0f)) {
                em->step++;
                em->stance = 4;
                em->velY = 0.0f;
                em->accelY = 0.0f;
                Em_SetMotion(em, 0x67, 0xA, 0);
                ((EmCephadromeWork *)work)->distLeft = 1000.0f;
                em->timer = 0x3C;
                em->accelZ = (((EmCephadromeWork *)work)->distLeft - em->timer * em->velZ) / ((em->timer * em->timer) / 2);
                EmCephadrome_SpawnSurfaceEffect5(em, 8.0f);
                break;
            }
        } else if (em->posY < em->floorY) {
            em->step = 0x63;
            em->posY = em->floorY;
            EmCephadrome_ChangeState(em, 0, 4, 4);
            EmCephadrome_SpawnSurfaceEffect5(em, 8.0f);
        }
        break;
    case 0x5:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, (s32 *) &work->moveAngBX);
        ((EmCephadromeWork *)work)->distLeft -= em->velZ;
        if (((EmCephadromeWork *)work)->distLeft <= 0.0f || em->velZ <= 0.0f) {
            em->step++;
            EmCephadrome_EndState2(em);
        }
        break;
    case 0x63:
        if (em->remote == 0) {
            em->posY = em->floorY;
            EmCephadrome_ChangeState(em, 0, 4, 4);
            EmCephadrome_SpawnSurfaceEffect5(em, 8.0f);
        }
        break;
    }
}

void EmCephadrome_State3_07(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 0;
        Em_SetMotion(em, 0x88, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmCephadrome_EndState(em);
        }
        break;
    }
}

void EmCephadrome_State3_08(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        Em_SetMotion(em, 0x71, 0, 0);
        Player_ClearVelocity(em);
        em->velY = 16.0f;
        em->velZ = 100.0f;
        em->accelY = -0.8f;
        work->moveAngBX = 0;
        work->moveAngBZ = 0;
        EmCephadrome_VibrateNearby(em);
        CamShake_ReqAIf(em, 3);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 54.0f, 0U) != 0) {
            em->timer = 0x12;
            em->accelZ = (-em->velZ / (f32) em->timer);
        }
        if ((EmMotion_ReachedFrame(em, 0, 84.0f) != 0) && (em->posY <= (em->surfaceY - *(f32 *)&em->surfaceDepth))) {
            em->velY = 0.0f;
            em->accelY = 0.0f;
        }
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, (s32 *) &work->moveAngBX);
        if (em->velZ <= 0.0f) {
            em->velZ = 0.0f;
            em->accelZ = 0.0f;
        }
        if (em->motion[0].playing == 0) {
            em->stance = 4;
            em->step++;
            EmCephadrome_EndState2(em);
        }
        break;
    }
}

void EmCephadrome_State4_00(Em *em)
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
            EmCephadrome_EndState(em);
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

void EmCephadrome_State4_01(Em *em)
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
            EmCephadrome_EndState(em);
        }
        break;
    }
}

void EmCephadrome_State4_02(Em *em)
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
            EmCephadrome_EndState(em);
        }
        break;
    }
}

void EmCephadrome_State4_03(Em *em)
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
            EmCephadrome_EndState(em);
        }
        break;
    }
}

void EmCephadrome_State4_04(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 0;
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
            EmCephadrome_EndState(em);
        }
        break;
    }
}

void EmCephadrome_State4_05(Em *em, EmWork *work, s16 variant)
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
                EmCephadrome_ChangeState(em, 0, 0x17, 4);
                break;
            default:
                EmCephadrome_EndState(em);
                break;
            }
        }
        break;
    case 5:
        if (em->remote == 0) {
            EmCephadrome_ChangeState(em, 0, 0x17, 4);
        }
        break;
    }
}

void EmCephadrome_State4_06(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x7E, 0, 0);
        em->stance = 2;
        em->accelY = -3.0f;
        em->accelX = 0.0f;
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
        if (em->posY < em->floorY) {
            em->step++;
            em->stance = 0;
            em->posY = em->floorY;
            EmCephadrome_ChangeState(em, 4, 0x10, 3);
        }
        break;
    }
}

void EmCephadrome_State4_07(Em *em)
{
    Em_SetFlag6AE(em, 4U);
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x4B, 0, 0);
        em->stance = 0;
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
            EmCephadrome_ChangeState(em, 4, 0x13, 3);
        }
        Em_SpawnEffect06Bone(em, 2);
        break;
    case 4:
        if (em->remote == 0) {
            EmCephadrome_ChangeState(em, 4, 0x13, 3);
        }
        Em_SpawnEffect06Bone(em, 2);
        break;
    }
}

void EmCephadrome_State4_09(Em *em, EmWork *work)
{
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x7E, 0, 0);
        em->stance = 2;
        em->accelY = -3.0f;
        em->accelX = 0.0f;
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
        if (em->posY < em->floorY) {
            em->step++;
            em->stance = 0;
            em->posY = em->floorY;
            EmCephadrome_ChangeState(em, 4, 0x10, 3);
        }
        break;
    }
}

void EmCephadrome_State4_0A(Em *em)
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
            EmCephadrome_EndState(em);
        }
        break;
    }
}

void EmCephadrome_State4_0B(Em *em)
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
            EmCephadrome_ChangeState(em, 0, 0x15, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmCephadrome_ChangeState(em, 0, 0x15, 4);
        }
        break;
    }
    Em_SpawnEffect06Bone(em, 2);
}

void EmCephadrome_State4_10(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 0;
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
            EmCephadrome_EndState(em);
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
            Player_ClearVelocity(em);
        }
        break;
    }
}

void EmCephadrome_State4_11(Em *em, EmWork *work, s32 variant)
{
    f32 pos[3];

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x7E, 0, 0);
        Player_ClearVelocity(em);
        em->velY = 100.0f;
        em->accelY = -2.75f;
        work->moveAngBX = 0;
        work->moveAngBZ = 0;
        if ((em->type == 8) && ((variant & 0xFF) == 1)) {
            pos[0] = em->posX;
            pos[1] = em->surfaceY;
            pos[2] = em->posZ;
            EmMine_CreateTimed0(em, pos);
        }
        break;
    case 1:
        work->moveAngBY = em->rotY;
        if (!(em->posY < (em->surfaceY - *(f32 *)&em->surfaceDepth))) {
            em->stance = 2;
            em->velY = 80.0f;
            Em_MoveRotatedAccel(em, (s32 *) &work->moveAngBX);
            em->step++;
            EmCephadrome_SpawnSurfaceEffect5(em, 8.0f);
            break;
        }
        em->stance = 4;
        Em_MoveRotated(em, &work->moveAngBX);
        break;
    case 2:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, (s32 *) &work->moveAngBX);
        if ((em->posY < em->surfaceY) && (em->velY < 0.0f)) {
            em->step++;
            em->stance = 0;
            em->posY = em->floorY;
            Em_SetMotion(em, 0x79, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x4B, 0, 0);
        }
        break;
    case 4:
    case 5:
        if (em->motion[0].playing == 0) {
            em->step++;
        }
        break;
    case 6:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x47, 0, 0);
        }
        break;
    case 7:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmCephadrome_EndState(em);
        }
        break;
    }
}

void EmCephadrome_State4_13(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x7C, 0, 0);
        em->stance = 1;
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmCephadrome_EndState(em);
        }
        break;
    }
}

void EmCephadrome_State4_15(Em *em)
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
            EmCephadrome_ChangeState(em, 0, 0x15, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmCephadrome_ChangeState(em, 0, 0x15, 4);
        }
        break;
    }
}

void EmCephadrome_State6_00_Entrance(Em *em, EmWork *work)
{
    EmCephadromeWork *cw = (EmCephadromeWork *)work;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step = 3;
        em->stance = 4;
        em->posX = 11900.0f;
        em->posY = -*(f32 *)&em->surfaceDepth;
        em->posZ = 5220.0f;
        em->goal[0] = 16700.0f;
        em->goal[1] = -*(f32 *)&em->surfaceDepth;
        em->goal[2] = 8450.0f;
        em->rotX = 0;
        em->rotY = Math_AngleXZ(&em->posX, em->goal);
        em->rotZ = 0;
        cw->distLeft = Vec_DistanceXZ(&em->posX, em->goal);
        Player_ClearVelocity(em);
        em->velZ = 50.0f;
        Em_SetMotion(em, 0x66, 0, 0);
        work->moveAngBX = 0;
        work->moveAngBZ = 0;
        break;
    case 1:
        Em_TurnToGoal(em, 0x40, 0);
        work->moveAngBY = em->rotY;
        Em_MoveRotated(em, &work->moveAngBX);
        cw->distLeft -= em->velZ;
        if (cw->distLeft <= 0.0f) {
            em->step++;
            em->goal[0] = 16700.0f;
            em->goal[1] = -*(f32 *)&em->surfaceDepth;
            em->goal[2] = 8450.0f;
            cw->unk00A = Math_AngleXZ(&em->posX, em->goal);
            cw->distLeft = Vec_DistanceXZ(&em->posX, em->goal);
            Em_SetMotion(em, 0x6B, 0, 0);
        }
        break;
    case 2:
        {
        u32 spd;
        u16 t;
        u32 d;
        s32 a;
        spd = (32768.0f / (em->motion[0].frameCount / 2.0f)) * em->motionSpeed;
        a = em->rotY;
        t = cw->unk00A;
        d = (t - (a & 0xFFFF)) & 0xFFFF;
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x66, 0, 0);
        } else if (((d + spd) & 0xFFFF) < spd * 2) {
            em->rotY = t;
        } else if (d < 0x8000U) {
            em->rotY = (a + spd) & 0xFFFF;
        } else {
            em->rotY = (a - spd) & 0xFFFF;
        }
        }
        break;
    case 3:
        Em_TurnToGoal(em, 0x40, 0);
        work->moveAngBY = em->rotY;
        Em_MoveRotated(em, &work->moveAngBX);
        cw->distLeft -= em->velZ;
        if (cw->distLeft <= 0.0f) {
            em->step++;
            em->goal[0] = 12728.0f;
            em->goal[1] = -*(f32 *)&em->surfaceDepth;
            em->goal[2] = 13160.0f;
            cw->unk00A = Math_AngleXZ(&em->posX, em->goal);
            cw->distLeft = Vec_DistanceXZ(&em->posX, em->goal);
            Em_SetMotion(em, 0x6A, 0, 0);
        }
        EmCephadrome_SpawnSurfaceEffect2(em, 8.0f);
        break;
    case 4:
        {
        s32 a;
        u32 spd;
        u16 t;
        u32 d;
        spd = (32768.0f / (em->motion[0].frameCount / 2.0f)) * em->motionSpeed;
        a = em->rotY;
        t = cw->unk00A;
        d = (t - (a & 0xFFFF)) & 0xFFFF;
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x66, 0, 0);
        } else if (((d + spd) & 0xFFFF) < spd * 2) {
            em->rotY = t;
        } else if (d < 0x8000U) {
            em->rotY = (a + spd) & 0xFFFF;
        } else {
            em->rotY = (a - spd) & 0xFFFF;
        }
        }
        break;
    case 5:
        Em_TurnToGoal(em, 0x40, 0);
        work->moveAngBY = em->rotY;
        Em_MoveRotated(em, &work->moveAngBX);
        cw->distLeft -= em->velZ;
        if (cw->distLeft <= 0.0f) {
            em->step++;
            em->goal[0] = 14380.0f;
            em->goal[1] = 0.0f;
            em->goal[2] = 14930.0f;
            em->rotY = Math_AngleXZ(&em->posX, em->goal);
            Em_SetMotion(em, 0x77, 0, 0);
            em->velY = 132.0f;
            em->velZ = 60.0f;
        }
        EmCephadrome_SpawnSurfaceEffect2(em, 8.0f);
        break;
    case 6:
        work->moveAngBY = em->rotY;
        Em_MoveRotated(em, &work->moveAngBX);
        if (!(em->posY < (em->surfaceY - *(f32 *)&em->surfaceDepth))) {
            em->step++;
            em->stance = 2;
            em->velY = 50.0f;
            em->accelY = -5.0f;
            EmCephadrome_SpawnSurfaceEffect5(em, 8.0f);
        }
        break;
    case 7:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, (s32 *) &work->moveAngBX);
        if (em->velY < 0.0f && em->posY < em->floorY) {
            em->step++;
            em->posY = em->floorY;
            EmCephadrome_SpawnSurfaceEffect5(em, 8.0f);
            em->stance = 0;
            Em_SetMotion(em, 0x79, 0, 0);
        }
        break;
    case 8:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stance = 1;
            Em_SetMotion(em, 0x7A, 0, 0);
        }
        break;
    case 9:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stance = 0;
            Em_SetMotion(em, 0x7C, 0, 0);
        }
        break;
    case 10:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x19, 0, 0);
        }
        break;
    case 11:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 1, 0, 0);
        }
        break;
    case 12:
        em->step++;
        EmCephadrome_EndState(em);
        break;
    }
}

void EmCephadrome_State6_01(Em *em)
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
        EmCephadrome_EndState(em);
        break;
    }
}

void EmCephadrome_State6_02_Captured(Em *em)
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
                EmCephadrome_ChangeState(em, 5, 4, 4);
                break;
            }
        } else {
            em->alpha = (f32) timer / 150.0f;
        }
        break;
    }
}

void EmCephadrome_State5_00_Hunted(Em *em, EmWork *work)
{
    if (em->type == 0x22) {
        em->noHitTimer = 10;
    }
    em->noPushTimer = 10;
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
            em->step++;
            em->timer = 900;
            EmMine_Create(em, 0);
            em->fieldCollide = 0;
        }
        break;
    case 2:
        EmMine_Update(em);
        if (em->type != 8 || QuestEm_CanAppear(em) != 0) {
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
                EmCephadrome_ChangeState(em, 5, 4, 4);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    }
}

void EmCephadrome_State5_01_Hunted(Em *em, EmWork *work)
{
    if (em->type == 0x22) {
        em->noHitTimer = 10;
    }
    em->noPushTimer = 10;
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
            em->step++;
            em->timer = 900;
            EmMine_Create(em, 0);
            em->fieldCollide = 0;
        }
        break;
    case 2:
        EmMine_Update(em);
        if (em->type != 8 || QuestEm_CanAppear(em) != 0) {
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
                EmCephadrome_ChangeState(em, 5, 4, 4);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    }
}

void EmCephadrome_State5_02_HuntedFalling(Em *em, EmWork *work)
{
    if (em->type == 0x22) {
        em->noHitTimer = 10;
    }
    em->noPushTimer = 10;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x7E, 0, 0);
        Player_ClearVelocity(em);
        em->velY = 100.0f;
        em->accelY = -2.75f;
        work->moveAngBX = 0;
        work->moveAngBZ = 0;
        break;
    case 1:
        work->moveAngBY = em->rotY;
        if (!(em->posY < em->surfaceY - *(f32 *) &em->surfaceDepth)) {
            em->stance = 2;
            em->velY = 80.0f;
            Em_MoveRotatedAccel(em, &work->moveAngBX);
            em->step++;
        } else {
            em->stance = 4;
            Em_MoveRotated(em, &work->moveAngBX);
        }
        break;
    case 2:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, &work->moveAngBX);
        if (em->posY < em->surfaceY) {
            if (em->velY < 0.0f) {
                em->step++;
                em->stance = 0;
                em->posY = em->floorY;
                Em_SetMotion(em, 0x79, 0, 0);
            }
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x44, 0, 0);
            em->lookAtTarget = 0;
            em->stance = 3;
            QuestEm_Hunted(em);
        }
        break;
    case 4:
        if (Player_PassedFrameScaled(em, 212.0f, 0) != 0) {
            CamShake_ReqAIf(em, 2);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 900;
            EmMine_Create(em, 0);
            em->fieldCollide = 0;
        }
        break;
    case 5:
        EmMine_Update(em);
        if (em->type != 8 || QuestEm_CanAppear(em) != 0) {
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
                EmCephadrome_ChangeState(em, 5, 4, 4);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    }
}

void EmCephadrome_State5_04(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        if (QuestEm_CanAppear(em) == 1) {
            em->stance = 4;
            EmCephadrome_ChangeState(em, 2, 0, 0);
            break;
        }
        em->taskPhase++;
        break;
    }
}

void EmCephadrome_State5_05_HuntedFalling(Em *em, EmWork *work)
{
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x7E, 0, 0);
        em->stance = 2;
        em->accelY = -3.0f;
        em->accelX = 0.0f;
        em->accelZ = 0;
        work->moveAngBX = 0;
        work->moveAngBZ = 0;
        break;
    case 1:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, &work->moveAngBX);
        if (em->posY < em->floorY) {
            em->step++;
            em->stance = 0;
            em->posY = em->floorY;
            Em_SetMotion(em, 0x79, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x44, 0, 0);
            em->lookAtTarget = 0;
            em->stance = 3;
            QuestEm_Hunted(em);
        }
        break;
    case 3:
        if (Player_PassedFrameScaled(em, 212.0f, 0) != 0) {
            CamShake_ReqAIf(em, 2);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 900;
            EmMine_Create(em, 0);
            em->fieldCollide = 0;
        }
        break;
    case 4:
        EmMine_Update(em);
        if (em->type != 8 || QuestEm_CanAppear(em) != 0) {
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
                EmCephadrome_ChangeState(em, 5, 4, 4);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    }
}

void EmCephadrome_RunGroup0(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmCephadrome_State0_00(em, work);
        break;
    case 1:
        EmCephadrome_State0_01(em, work);
        break;
    case 2:
        EmCephadrome_State0_02(em, work);
        break;
    case 3:
        EmCephadrome_State0_03(em, work);
        break;
    case 4:
        EmCephadrome_State0_04(em, work, 0);
        break;
    case 5:
        EmCephadrome_State0_05(em, (s32) work, 0);
        break;
    case 6:
        EmCephadrome_State0_06(em);
        break;
    case 7:
        EmCephadrome_State0_07(em);
        break;
    case 9:
        EmCephadrome_State0_04(em, work, 1);
        break;
    case 14:
        EmCephadrome_State0_0E(em, work);
        break;
    case 15:
        EmCephadrome_State0_0F(em);
        break;
    case 16:
        EmCephadrome_State0_10(em, work);
        break;
    case 17:
        EmCephadrome_State0_0F(em);
        break;
    case 18:
        EmCephadrome_State0_12(em);
        break;
    case 19:
        EmCephadrome_State0_13(em);
        break;
    case 20:
        EmCephadrome_State0_14(em);
        break;
    case 21:
        EmCephadrome_State0_07(em);
        break;
    case 22:
        EmCephadrome_State0_07(em);
        break;
    case 23:
        EmCephadrome_State0_07(em);
        break;
    case 40:
        EmCephadrome_State0_28(em);
        break;
    case 41:
        EmCephadrome_State0_05(em, (s32) work, 1);
        break;
    case 42:
        EmCephadrome_State0_06(em);
        break;
    }
}

void EmCephadrome_RunGroup1(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmCephadrome_State1_00(em, work);
        break;
    case 1:
        EmCephadrome_State1_01(em, work);
        break;
    case 2:
        EmCephadrome_State1_02(em, work);
        break;
    case 3:
        EmCephadrome_State1_03(em, work);
        break;
    }
}

void EmCephadrome_RunGroup2(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmCephadrome_State2_00(em);
        break;
    case 1:
        EmCephadrome_State2_01(em);
        break;
    case 2:
        EmCephadrome_State2_02(em, work);
        break;
    case 3:
        EmCephadrome_State2_03(em, (s32) work, 0);
        break;
    case 4:
        EmCephadrome_State2_04(em, work);
        break;
    case 5:
        EmCephadrome_State2_05(em, work, 0);
        break;
    case 6:
        EmCephadrome_State2_06(em);
        break;
    case 7:
        EmCephadrome_State2_07(em, work);
        break;
    case 8:
        EmCephadrome_State2_08(em, work);
        break;
    case 9:
        EmCephadrome_State2_09(em);
        break;
    case 10:
        EmCephadrome_State2_0A(em, work);
        break;
    case 11:
        EmCephadrome_State2_0B(em, work);
        break;
    case 12:
        EmCephadrome_State2_0C(em, work);
        break;
    case 13:
        EmCephadrome_State2_0D(em, work);
        break;
    case 14:
        EmCephadrome_State2_0E(em, work);
        break;
    case 15:
        EmCephadrome_State2_0F(em);
        break;
    case 16:
        EmCephadrome_State2_10(em, work);
        break;
    case 17:
        EmCephadrome_State2_11(em, work);
        break;
    case 18:
        EmCephadrome_State2_12(em, work);
        break;
    case 19:
        EmCephadrome_State2_0A(em, work);
        break;
    case 20:
        EmCephadrome_State2_0B(em, work);
        break;
    case 21:
        EmCephadrome_State2_15(em);
        break;
    case 22:
        EmCephadrome_State2_16(em, work);
        break;
    case 23:
        break;
    case 24:
        EmCephadrome_State2_18(em);
        break;
    case 25:
        EmCephadrome_State2_18(em);
        break;
    case 26:
        EmCephadrome_State2_03(em, (s32) work, 1);
        break;
    case 27:
        EmCephadrome_State2_05(em, work, 1);
        break;
    }
}

void EmCephadrome_RunGroup3(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmCephadrome_State3_00(em, work);
        break;
    case 1:
        EmCephadrome_State3_01(em, work);
        break;
    case 2:
        EmCephadrome_State3_02(em, work);
        break;
    case 3:
        EmCephadrome_State3_03(em, work);
        break;
    case 4:
        EmCephadrome_State3_04(em, work);
        break;
    case 5:
        EmCephadrome_State3_05(em, work);
        break;
    case 6:
        EmCephadrome_State3_06(em, work);
        break;
    case 7:
        EmCephadrome_State3_07(em, work);
        break;
    case 8:
        EmCephadrome_State3_08(em, work);
        
    default:
        break;
    }
}

void EmCephadrome_RunGroup4(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmCephadrome_State4_00(em);
        break;
    case 1:
        EmCephadrome_State4_01(em);
        break;
    case 2:
        EmCephadrome_State4_02(em);
        break;
    case 3:
        EmCephadrome_State4_03(em);
        break;
    case 4:
        EmCephadrome_State4_04(em);
        break;
    case 5:
        EmCephadrome_State4_05(em, work, 1);
        break;
    case 6:
        EmCephadrome_State4_06(em, work);
        break;
    case 7:
        EmCephadrome_State4_07(em);
        break;
    case 8:
        break;
    case 9:
        EmCephadrome_State4_09(em, work);
        break;
    case 10:
        EmCephadrome_State4_0A(em);
        break;
    case 11:
        EmCephadrome_State4_0B(em);
        break;
    case 12:
        break;
    case 13:
        break;
    case 14:
        break;
    case 15:
        break;
    case 16:
        EmCephadrome_State4_10(em);
        break;
    case 17:
        EmCephadrome_State4_11(em, work, 0);
        break;
    case 18:
        EmCephadrome_State4_11(em, work, 1);
        break;
    case 19:
        EmCephadrome_State4_13(em);
        break;
    case 20:
        EmCephadrome_State4_05(em, work, 0);
        break;
    case 21:
        EmCephadrome_State4_15(em);
        break;
    case 22:
        EmCephadrome_State4_05(em, work, 3);
        break;
    case 23:
        EmCephadrome_State4_05(em, work, 2);
        break;
    }
}

void EmCephadrome_RunGroup5(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmCephadrome_State5_00_Hunted(em, work);
        break;
    case 1:
        EmCephadrome_State5_01_Hunted(em, work);
        break;
    case 2:
        EmCephadrome_State5_02_HuntedFalling(em, work);
        break;
    case 4:
        EmCephadrome_State5_04(em, work);
        break;
    case 5:
        EmCephadrome_State5_05_HuntedFalling(em, work);
        break;
    }
}

void EmCephadrome_RunGroup6(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmCephadrome_State6_00_Entrance(em, work);
        break;
    case 1:
        EmCephadrome_State6_01(em);
        break;
    case 2:
        EmCephadrome_State6_02_Captured(em);
        break;
    }
}

void EmCephadrome_Update(Em *em)
{
    u8 buf[4];
    u8 *w = &em->work;
    f32 f;

    Em_UpdateCombatTimer(em);
    Em_RaiseTrigger80Lax(em);
    if (EmCephadrome_ShouldEndState(em) == 1) {
        EmCephadrome_EndState(em);
    }
    if (em->noAlert != 1 && em->rage == 0 && em->type == 8 && em->variant != 1) {
        Em_CheckLowHp(em, 0.1f);
    }
    switch (Em_UpdateDamage(em, buf) & 0xFF) {
    case 0:
    case 3:
    case 4:
    case 9:
    case 11:
    case 14:
        break;
    case 1:
    case 2:
        if ((s8) em->holdTimer != 0) {
            EmCephadrome_ChangeState(em, 5, 1, 3);
        } else {
            switch (em->stance) {
            case 4:
            case 1:
                EmCephadrome_ChangeState(em, 5, 2, 3);
                break;
            case 2:
                if (em->posY >= em->surfaceY) {
                    EmCephadrome_ChangeState(em, 5, 5, 3);
                } else {
                    EmCephadrome_ChangeState(em, 5, 2, 3);
                }
                break;
            default:
            case 0:
                EmCephadrome_ChangeState(em, 5, 0, 3);
                break;
            }
        }
        break;
    case 17:
        if (em->stance == 4) {
            Em_EndStatus5(em);
        } else if ((em->stateGroup != 4 || em->state != 0x16) && (em->stateGroup != 4 || em->state != 0x17) && (em->stateGroup != 4 || em->state != 6)) {
            Em_SetStatus5Timer(em);
            if ((u16) (em->hitAngle - em->rotY) > 0x8000) {
                EmCephadrome_ChangeState(em, 4, 0x16, 3);
            } else {
                EmCephadrome_ChangeState(em, 4, 0x17, 3);
            }
        }
        break;
    case 18:
        Em_ResetKindTimer(em);
        EmCephadrome_ChangeState(em, 4, 0x15, 3);
        break;
    case 15:
        if (em->stance == 4) {
            EmCephadrome_ChangeState(em, 4, 0x12, 3);
        }
        break;
    case 5:
        if ((em->stateGroup != 4 || em->state != 6) && (em->stateGroup != 4 || em->state != 0x11) && (em->stateGroup != 4 || em->state != 0x12)) {
            EmCephadrome_ChangeState(em, 4, 6, 3);
        }
        break;
    case 6:
        if ((em->stateGroup != 4 || em->state != 0xB) && (em->stateGroup != 4 || em->state != 6) && (em->stateGroup != 4 || em->state != 7) && em->stance == 1) {
            Em_SetStatus4Timer(em);
            EmCephadrome_ChangeState(em, 4, 7, 3);
        } else if ((em->stateGroup != 4 || em->state != 0xB) && (em->stateGroup != 4 || em->state != 6) && (em->stateGroup != 4 || em->state != 7) && em->stance == 0) {
            Em_SetStatus4Timer(em);
            EmCephadrome_ChangeState(em, 4, 0xB, 3);
        } else if ((em->stateGroup != 4 || em->state != 6) && (em->stateGroup != 4 || em->state != 0x11) && (em->stateGroup != 4 || em->state != 0x12) && em->stance == 4) {
            Em_SetStatus4Timer(em);
            EmCephadrome_ChangeState(em, 4, 0x11, 3);
        }
        break;
    case 7:
        EmCephadrome_ChangeState(em, 6, 2, 3);
        break;
    case 8:
        if ((em->stateGroup != 4 || em->state != 6) && (em->stateGroup != 4 || em->state != 0x11) && (em->stateGroup != 4 || em->state != 0x12) && em->stance == 4) {
            EmCephadrome_ChangeState(em, 4, 0x11, 3);
        } else if ((em->stateGroup != 0 || em->state != 0xE) && (em->stateGroup != 4 || em->state != 6) && em->stance == 1) {
            Em_SetStatus2Timer(em);
            EmCephadrome_ChangeState(em, 0, 0xE, 3);
        } else if ((em->stateGroup != 0 || em->state != 5) && (em->stateGroup != 4 || em->state != 6) && em->stance == 0) {
            Em_SetStatus2Timer(em);
            EmCephadrome_ChangeState(em, 0, 5, 3);
        }
        break;
    case 10:
        switch (em->state) {
        case 5:
            EmCephadrome_ChangeState(em, 0, 6, 3);
            em->scriptPending = 0;
            break;
        case 0xE:
            EmCephadrome_ChangeState(em, 0, 0xF, 3);
            em->scriptPending = 0;
            break;
        case 0x12:
            EmCephadrome_ChangeState(em, 0, 0x13, 3);
            em->scriptPending = 0;
            break;
        case 0x29:
            EmCephadrome_ChangeState(em, 0, 0x2A, 3);
            em->scriptPending = 0;
            break;
        }
        break;
    case 12:
    case 16:
        switch (em->stance) {
        case 0:
        case 3:
            switch (em->flinchPart) {
            case 0:
            case 7:
                EmCephadrome_ChangeState(em, 4, 0, 3);
                break;
            case 5:
            case 6:
                EmCephadrome_ChangeState(em, 4, 2, 3);
                break;
            case 1:
            case 2:
                EmCephadrome_ChangeState(em, 4, 3, 3);
                break;
            case 3:
            case 4:
            default:
                if (em->parts[em->flinchPart].state >= 2) {
                    if (em->flinchPart != 3) {
                        EmCephadrome_ChangeState(em, 4, 5, 3);
                    } else {
                        EmCephadrome_ChangeState(em, 4, 0x14, 3);
                    }
                } else {
                    EmCephadrome_ChangeState(em, 4, 1, 3);
                }
                break;
            }
            break;
        case 1:
            EmCephadrome_ChangeState(em, 4, 4, 3);
            break;
        case 2:
            EmCephadrome_ChangeState(em, 4, 6, 3);
            break;
        case 4:
            EmCephadrome_ChangeState(em, 4, 0x11, 3);
            break;
        }
        break;
    case 13:
        if (em->stance != 2) {
            EmCephadrome_ChangeState(em, 4, 0, 3);
            em->scriptPending = 0;
        }
        break;
    }
    Em_UpdatePendingScript(em);
    Em_ClearFlag6AE(em);
    EmCephadrome_RunState(em, (s32) w);
    if (em->stateRequest != 0) {
        EmCephadrome_RunState(em, (s32) w);
        em->stateRequest = 0;
    }
    if (em->surfaceKind != 0 && gScene.area == em->area) {
        if (em->stance == 4) {
            f = em->surfaceY - *(f32 *) &em->surfaceDepth;
            if (em->posY > f) {
                em->posY = f;
            }
            f = em->surfaceY - 1000.0f;
            if (em->floorY >= f) {
                em->floorY = f;
            }
            if (em->posY < em->floorY) {
                em->posY = em->floorY;
            }
        }
        if (em->stance == 0) {
            em->floorY = em->surfaceY;
            if (em->posY < em->floorY) {
                em->posY = em->floorY;
            }
        }
    }
}

void EmCephadrome_RunState(Em *em, s32 work)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->stateGroup) {
    case 0:
        EmCephadrome_RunGroup0(em, (EmWork *) work);
        break;
    case 1:
        EmCephadrome_RunGroup1(em, (EmWork *) work);
        break;
    case 2:
        EmCephadrome_RunGroup2(em, (EmWork *) work);
        break;
    case 3:
        EmCephadrome_RunGroup3(em, (EmWork *) work);
        break;
    case 4:
        EmCephadrome_RunGroup4(em, (EmWork *) work);
        break;
    case 5:
        EmCephadrome_RunGroup5(em, (EmWork *) work);
        break;
    case 6:
        EmCephadrome_RunGroup6(em, (EmWork *) work);
        break;
    case 7:
        EmCephadrome_RunGroup6(em, (EmWork *) work);
        
    default:
        break;
    }
}

void EmCephadrome_RequestOnFrame(s32 em, s32 frame, s32 i)
{
    if (Player_PassedFrameScaled((Em *) em, (f32) frame, 0U) != 0) {
        CamShake_ReqAIf((void *) em, i);
    }
}

static void EmCephadrome_MotionEffectsDefault(Em *em)
{
}

void EmCephadrome_MotionEffects(Em *em, EmWork *work)
{
    f32 v1[3];
    f32 v2[3];
    f32 v3[3];

    if (em->motionId[0] != work->prevMotion) {
        work->prevMotion = em->motionId[0];
    }
    EmCephadrome_MotionSounds(em, work);
    switch (work->prevMotion) {
    case 0x3E9:
        EmCephadrome_SetFinMode(em, work, 3);
        break;
    case 0x3EB:
        EmCephadrome_SetFinMode(em, work, 1);
        if (Player_PassedFrameScaled(em, 2.0f, 0) != 0) {
            EmCephadrome_SpawnEffect21(em, 2);
        }
        if (Player_PassedFrameScaled(em, 50.0f, 0) != 0) {
            EmCephadrome_SpawnEffect21(em, 3);
        }
        if (Player_PassedFrameScaled(em, 120.0f, 0) != 0) {
            EmCephadrome_SpawnEffect21(em, 4);
            break;
        }
        break;
    case 0x3EC:
        EmCephadrome_SetFinMode(em, work, 0);
        if (Player_PassedFrameScaled(em, 22.0f, 0) != 0) {
            EmCephadrome_SpawnEffect21(em, 0x13);
        }
        if (Player_PassedFrameScaled(em, 16.0f, 0) || Player_PassedFrameScaled(em, 56.0f, 0) || Player_PassedFrameScaled(em, 102.0f, 0)) {
            EmCephadrome_SpawnEffect21(em, 0x14);
        }
        if ((Player_PassedFrameScaled(em, 30.0f, 0) != 0) || (Player_PassedFrameScaled(em, 78.0f, 0) != 0)) {
            EmCephadrome_SpawnEffect21(em, 0x15);
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
        EmCephadrome_SetFinMode(em, work, 1);
        if (Player_PassedFrameScaled(em, 2.0f, 0) != 0) {
            EmCephadrome_SpawnEffect21(em, 5);
        }
        if (Player_PassedFrameScaled(em, 30.0f, 0) != 0) {
            EmCephadrome_SpawnEffect21(em, 6);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 2.0f, 78.0f) != 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create(em, 0x1A, 1, 1.0f);
            break;
        }
        break;
    case 0x3EE:
        EmCephadrome_SetFinMode(em, work, 1);
        if (Player_PassedFrameScaled(em, 30.0f, 0) != 0) {
            EmCephadrome_SpawnEffect21(em, 7);
        }
        if (Player_PassedFrameScaled(em, 2.0f, 0) != 0) {
            EmCephadrome_SpawnEffect21(em, 8);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 2.0f, 78.0f) != 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create(em, 0x1A, 0, 1.0f);
            break;
        }
        break;
    case 0x3F2:
        EmCephadrome_SetFinMode(em, work, 1);
        if (Player_PassedFrameScaled(em, 42.0f, 0) != 0) {
            EmCephadrome_SpawnEffect21(em, 0x11);
        }
        if (Player_PassedFrameScaled(em, 2.0f, 0) != 0) {
            EmCephadrome_SpawnEffect21(em, 0x12);
        }
        if (Player_PassedFrameScaled(em, 74.0f, 0) != 0) {
            EmCephadrome_SpawnEffect21(em, 0x12);
            break;
        }
        break;
    case 0x3F3:
        EmCephadrome_SetFinMode(em, work, 1);
        break;
    case 0x3F9:
        break;
    case 0x3FA:
        EmCephadrome_SetFinMode(em, work, 1);
        if (Player_PassedFrameScaled(em, 42.0f, 0) || Player_PassedFrameScaled(em, 102.0f, 0) || Player_PassedFrameScaled(em, 156.0f, 0)) {
            if (em->posY <= (1000.0f + em->floorY)) {
                StageObj_K14_Create(em, 1, 0, 1.0f);
                break;
            }
        }
        break;
    case 0x3FB:
        EmCephadrome_SetFinMode(em, work, 1);
        if (Player_PassedFrameScaled(em, 2.0f, 0) != 0) {
            EmCephadrome_SpawnEffect21(em, 0xD);
        }
        if (Player_PassedFrameScaled(em, 16.0f, 0) != 0) {
            EmCephadrome_SpawnK14Type0B(em);
            break;
        }
        break;
    case 0x401:
        EmCephadrome_SetFinMode(em, work, 3);
        break;
    case 0x407:
        v1[0] = 0.0f;
        v1[1] = 10.0f;
        v1[2] = 140.0f;
        Em_SpawnEffect06Periodic(em, 0x22, v1, 1.6f);
        break;
    case 0x408:
    case 0x409:
        break;
    case 0x40A:
        EmCephadrome_SetFinMode(em, work, 3);
        v2[0] = 0.0f;
        v2[1] = 10.0f;
        v2[2] = 140.0f;
        Em_SpawnEffect06Periodic(em, 0x22, v2, 1.6f);
        break;
    case 0x40C:
        EmCephadrome_SetFinMode(em, work, 3);
        if (Player_PassedFrameScaled(em, 50.0f, 0) != 0) {
            EmCephadrome_SpawnEffect21(em, 1);
            break;
        }
        break;
    case 0x40D:
        EmCephadrome_SetFinMode(em, work, 3);
        break;
    case 0x40E:
        EmCephadrome_SetFinMode(em, work, 3);
        if (Player_PassedFrameScaled(em, 148.0f, 0) != 0) {
            EmCephadrome_SpawnEffect21(em, 0xF);
        }
        v3[0] = 0.0f;
        v3[1] = 10.0f;
        v3[2] = 140.0f;
        Em_SpawnEffect06Periodic(em, 0x22, v3, 1.6f);
        if (Player_PassedFrameScaled(em, 156.0f, 0) != 0) {
            StageObj_K14_Create(em, 0, 0, 1.0f);
            break;
        }
        break;
    case 0x413:
        EmCephadrome_SetFinMode(em, work, 1);
        if (Player_PassedFrameScaled(em, 30.0f, 0) != 0) {
            EmCephadrome_SpawnEffect21(em, 0x10);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 6.0f, 38.0f) != 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create(em, 0x1A, 1, 1.0f);
            break;
        }
        break;
    case 0x417:
        EmCephadrome_SetFinMode(em, work, 1);
        if (Player_PassedFrameScaled(em, 66.0f, 0) != 0) {
            EmCephadrome_SpawnEffect21(em, 0x19);
            break;
        }
        break;
    case 0x418:
    case 0x423:
        break;
    case 0x424:
        EmCephadrome_SetFinMode(em, work, 1);
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
        EmCephadrome_SetFinMode(em, work, 3);
        break;
    case 0x427:
        EmCephadrome_SetFinMode(em, work, 1);
        break;
    case 0x428:
        EmCephadrome_SetFinMode(em, work, 1);
        break;
    case 0x42A:
        EmCephadrome_SetFinMode(em, work, 1);
        if (Player_PassedFrameScaled(em, 268.0f, 0) != 0) {
            StageObj_K14_Create(em, 0, 0, 0.9f);
            break;
        }
        break;
    case 0x42B:
        EmCephadrome_SetFinMode(em, work, 1);
        break;
    case 0x42C:
        EmCephadrome_SetFinMode(em, work, 0);
        if (Player_PassedFrameScaled(em, 50.0f, 0) != 0) {
            StageObj_K14_Create(em, 0, 0, 0.8f);
            break;
        }
        break;
    case 0x42D:
        EmCephadrome_SetFinMode(em, work, 1);
        if (Player_PassedFrameScaled(em, 26.0f, 0) != 0) {
            StageObj_K14_Create(em, 0x12, 0x80, 1.0f);
            break;
        }
        break;
    case 0x42E:
        EmCephadrome_SetFinMode(em, work, 3);
        if ((Player_PassedFrameScaled(em, 16.0f, 0) != 0) || (Player_PassedFrameScaled(em, 66.0f, 0) != 0)) {
            Effect0D_SpawnForEmScaled(em, 2, 6, 4.0f);
            break;
        }
        break;
    case 0x42F:
        EmCephadrome_SetFinMode(em, work, 3);
        break;
    case 0x432:
        EmCephadrome_SetFinMode(em, work, 1);
        if (Player_PassedFrameScaled(em, 26.0f, 0) != 0) {
            StageObj_K14_Create(em, 0x12, 0x81, 1.0f);
            break;
        }
        break;
    case 0x433:
        EmCephadrome_SetFinMode(em, work, 3);
        if ((Player_PassedFrameScaled(em, 16.0f, 0) != 0) || (Player_PassedFrameScaled(em, 66.0f, 0) != 0)) {
            Effect0D_SpawnForEmScaled(em, 2, 6, 4.0f);
            break;
        }
        break;
    case 0x434:
        EmCephadrome_SetFinMode(em, work, 3);
        break;
    case 0x437:
        break;
    case 0x447:
        EmCephadrome_SetFinMode(em, work, 1);
        break;
    case 0x448:
        EmCephadrome_SetFinMode(em, work, 1);
        break;
    case 0x44C:
        EmCephadrome_SetFinMode(em, work, 0);
        break;
    case 0x44D:
        EmCephadrome_SetFinMode(em, work, 0);
        break;
    case 0x44E:
        EmCephadrome_SetFinMode(em, work, 0);
        break;
    case 0x44F:
        EmCephadrome_SetFinMode(em, work, 0);
        if (Player_PassedFrameScaled(em, 10.0f, 0) != 0) {
            EmCephadrome_SpawnEffect21(em, 0x1A);
            break;
        }
        break;
    case 0x450:
        EmCephadrome_SetFinMode(em, work, 0);
        break;
    case 0x451:
        EmCephadrome_SetFinMode(em, work, 0);
        break;
    case 0x452:
        EmCephadrome_SetFinMode(em, work, 0);
        break;
    case 0x453:
        EmCephadrome_SetFinMode(em, work, 0);
        break;
    case 0x454:
        EmCephadrome_SetFinMode(em, work, 0);
        if (Player_PassedFrameScaled(em, 10.0f, 0) != 0) {
            EmCephadrome_SpawnEffect21(em, 0xE);
            break;
        }
        break;
    case 0x457:
        EmCephadrome_SetFinMode(em, work, 0);
        break;
    case 0x458:
        EmCephadrome_SetFinMode(em, work, 0);
        EmCephadrome_RequestOnFrame((s32) em, 0x1A, 3);
        if (Player_PassedFrameScaled(em, 26.0f, 0) != 0) {
            EmCephadrome_VibrateNearby(em);
            EmCephadrome_SpawnEffect21(em, 0x1D);
            break;
        }
        break;
    case 0x459:
        if (Player_PassedFrameScaled(em, 28.0f, 0) != 0) {
            EmCephadrome_SpawnEffect21(em, 0x1B);
        }
        EmCephadrome_SetFinMode(em, work, 0);
        break;
    case 0x45A:
        EmCephadrome_SetFinMode(em, work, 0);
        break;
    case 0x45B:
        EmCephadrome_SetFinMode(em, work, 0);
        break;
    case 0x45C:
        EmCephadrome_SetFinMode(em, work, 0);
        break;
    case 0x45D:
        EmCephadrome_SetFinMode(em, work, 0);
        break;
    case 0x45E:
        break;
    case 0x45F:
        EmCephadrome_SetFinMode(em, work, 0);
        if (Player_PassedFrameScaled(em, 2.0f, 0) != 0) {
            EmCephadrome_SpawnEffect21(em, 0x18);
            break;
        }
        break;
    case 0x460:
        EmCephadrome_SetFinMode(em, work, 0);
        break;
    case 0x461:
        EmCephadrome_SetFinMode(em, work, 0);
        if (Player_PassedFrameScaled(em, 10.0f, 0) != 0) {
            StageObj_K14_Create(em, 0, 0x89, 0.7f);
            break;
        }
        break;
    case 0x462:
        EmCephadrome_SetFinMode(em, work, 0);
        if (Player_PassedFrameScaled(em, 2.0f, 0) != 0) {
            EmCephadrome_SpawnEffect21(em, 0x17);
        }
        if ((Player_PassedFrameScaled(em, 24.0f, 0) != 0) || (Player_PassedFrameScaled(em, 42.0f, 0) != 0) || (Player_PassedFrameScaled(em, 62.0f, 0) != 0) || (Player_PassedFrameScaled(em, 82.0f, 0) != 0)) {
            Effect0D_SpawnForEmScaled(em, 6, 3, 8.0f + (0.0005f * (f32) ((u16) Rand_Next(1) & 0x3FF)));
        }
        if ((Player_PassedFrameScaled(em, 10.0f, 0) != 0) || (Player_PassedFrameScaled(em, 32.0f, 0) != 0) || (Player_PassedFrameScaled(em, 52.0f, 0) != 0) || (Player_PassedFrameScaled(em, 74.0f, 0) != 0)) {
            Effect0D_SpawnForEmScaled(em, 0xC, 3, 8.0f + (0.0005f * (f32) ((u16) Rand_Next(1) & 0x3FF)));
            break;
        }
        break;
    case 0x464:
        EmCephadrome_SetFinMode(em, work, 0);
        break;
    case 0x466:
        EmCephadrome_SetFinMode(em, work, 3);
        break;
    case 0x467:
        EmCephadrome_SetFinMode(em, work, 0);
        if (Player_PassedFrameScaled(em, 42.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 2, 6, 4.0f);
            break;
        }
        break;
    case 0x468:
        EmCephadrome_SetFinMode(em, work, 3);
        if (Player_PassedFrameScaled(em, 42.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 2, 6, 4.0f);
            break;
        }
        break;
    case 0x469:
        EmCephadrome_SetFinMode(em, work, 3);
        if (Player_PassedFrameScaled(em, 46.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 2, 6, 4.0f);
            break;
        }
        break;
    case 0x46A:
        EmCephadrome_SetFinMode(em, work, 0);
        break;
    case 0x46B:
        EmCephadrome_SetFinMode(em, work, 0);
        break;
    case 0x46C:
        EmCephadrome_SetFinMode(em, work, 0);
        break;
    case 0x46D:
        EmCephadrome_SetFinMode(em, work, 0);
        if ((Player_PassedFrameScaled(em, 6.0f, 0) != 0) || (Player_PassedFrameScaled(em, 10.0f, 0) != 0)) {
            Effect0D_SpawnForEmScaled(em, 2, 3, 7.0f);
            break;
        }
        break;
    case 0x46E:
        EmCephadrome_SetFinMode(em, work, 0);
        if (Player_PassedFrameScaled(em, 2.0f, 0) != 0) {
            EmCephadrome_SpawnEffect21(em, 0x1C);
            break;
        }
        break;
    case 0x46F:
        EmCephadrome_SetFinMode(em, work, 0);
        break;
    case 0x470:
        EmCephadrome_SetFinMode(em, work, 0);
        if (Player_PassedFrameScaled(em, 60.0f, 0) != 0) {
            EmCephadrome_SpawnEffect21(em, 0x16);
            break;
        }
        break;
    case 0x471:
    case 0x472:
        break;
    default:
        EmCephadrome_MotionEffectsDefault(em);
        break;
    }
}

extern u16 EmCephadrome_FinFoldTarget[];

typedef struct EmCephadromeFins {
    u8 pad00[0xF];
    u8 mode;            /* 0x0F */
    u8 pad10[0x42 - 0x10];
    s16 wait[4];        /* 0x42 */
    u8 st[4][2];        /* 0x4A */
    struct {
        u16 v;
        u16 w;
    } val[4];           /* 0x52 */
    s16 frame[4];       /* 0x62 */
    s8 cnt[4];          /* 0x6A */
} EmCephadromeFins;

static void EmCephadrome_UpdateFinFold(Em *em, EmWork *work, s32 i)
{
    EmCephadromeFins *p = (EmCephadromeFins *) work;
    u8 *st;

    switch (*(st = &((u8 (*)[2]) p->st)[i][1])) {
    case 0:
        if (p->mode == 2 || p->mode == 3) {
            if (p->val[i].v != EmCephadrome_FinFoldTarget[i]) {
                *st = 2;
                p->cnt[i] = 20;
            }
        } else if (p->mode == 1) {
            if (p->val[i].v != 0) {
                *st = 1;
                p->cnt[i] = 20;
            }
        }
        break;
    case 1:
        if (--p->cnt[i] <= 0) {
            p->val[i].v = 0;
            *st = 0;
        } else {
            p->val[i].v += (u16) ((0x10000 - p->val[i].v) / p->cnt[i]);
        }
        break;
    case 2:
        if (--p->cnt[i] <= 0) {
            p->val[i].v = EmCephadrome_FinFoldTarget[i];
            *st = 0;
        } else {
            p->val[i].v -= (u16) ((u16) (0x10000 - (EmCephadrome_FinFoldTarget[i] - p->val[i].v)) / p->cnt[i]);
        }
        break;
    }
}


typedef struct EmCephFinKey {
    u16 f;
    u16 v;
} EmCephFinKey;

extern EmCephFinKey *EmCephadrome_FinKeys2[];
extern EmCephFinKey *EmCephadrome_FinKeys1[];
extern s16 EmCephadrome_FinWait1[];
extern s16 EmCephadrome_FinWait2[];
extern s16 EmCephadrome_FinRewait1[];
extern s16 EmCephadrome_FinRewait2[];

static inline u8 CephFinFindKey(EmCephFinKey *key, u16 f)
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
static void EmCephadrome_UpdateFinWave(s32 j, EmWork *work, s32 i)
{
    EmCephadromeFins *p = (EmCephadromeFins *) work;
    s16 f;
    EmCephFinKey *tbl;
    u8 m;
    u8 *st;

    j = i * 2;
    switch (*(st = (u8 *) p->st + j)) {
    case 0:
        if (p->mode == 2) {
            (*st)++;
            FIN_S16(p->wait, j) = FIN_S16(EmCephadrome_FinWait2, j);
        } else if (p->mode == 1) {
            (*st)++;
            FIN_S16(p->wait, j) = FIN_S16(EmCephadrome_FinWait1, j);
        }
        break;
    case 1:
        if (--FIN_S16(p->wait, j) <= 0) {
            if (p->mode == 2) {
                *st = 3;
            } else {
                *st = 2;
            }
            FIN_S16(p->wait, j) = 0;
            FIN_S16(p->frame, j) = 0;
        }
        break;
    case 2:
        f = FIN_S16(p->frame, j)++;
        tbl = EmCephadrome_FinKeys2[i];
        m = CephFinFindKey(tbl, f);
        if (m & 0x80) {
            if (p->mode == 2) {
                *st = 1;
                FIN_S16(p->wait, j) = FIN_S16(EmCephadrome_FinRewait2, j);
            } else if (p->mode == 1) {
                *st = 1;
                FIN_S16(p->wait, j) = FIN_S16(EmCephadrome_FinRewait1, j);
            } else {
                *st = 0;
            }
            p->val[i].w = tbl[m & 0x7F].v;
        } else {
            p->val[i].w += tbl[m].v;
        }
        break;
    case 3:
        f = FIN_S16(p->frame, j)++;
        tbl = EmCephadrome_FinKeys1[i];
        m = CephFinFindKey(tbl, f);
        if (m & 0x80) {
            if (p->mode == 2) {
                *st = 1;
                FIN_S16(p->wait, j) = FIN_S16(EmCephadrome_FinRewait2, j);
            } else if (p->mode == 1) {
                *st = 1;
                FIN_S16(p->wait, j) = FIN_S16(EmCephadrome_FinRewait1, j);
            } else {
                *st = 0;
            }
            p->val[i].w = tbl[m & 0x7F].v;
        } else {
            p->val[i].w += tbl[m].v;
        }
        break;
    }
}

void EmCephadrome_UpdateFins(Em *em, EmWork *work)
{
    s32 i;

    for (i = 0; i < 4; i++) {
        EmCephadrome_UpdateFinWave((s32) em, work, i);
        EmCephadrome_UpdateFinFold(em, work, i);
    }
}


void EmCephadrome_UpdateMotionEffects(Em *em)
{
    EmCephadromeWork *work = (EmCephadromeWork *)&em->work;

    switch (work->fxStep) {
    case 0:
        work->fxStep++;
        break;
    case 1:
        EmCephadrome_MotionEffects(em, (EmWork *) work);
        EmCephadrome_UpdateFins(em, (EmWork *) work);
        break;
    }
}

void EmCephadrome_SetFinMode(Em *em, EmWork *work, s32 mode)
{
    EmCephadromeWork *w = (EmCephadromeWork *)work;

    w->finMode = mode;
    if (w->finMode == 1) {
        if (em->hp <= (s16) (0.1f * em->hpMax)) {
            w->finMode = 2;
        }
    }
    if (w->finMode == 3) {
        if (em->hp > (s16) (0.1f * em->hpMax)) {
            w->finMode = 0;
        }
    }
}

void EmCephadrome_SpawnK14Type0B(Em *em)
{
    StageObj_K14_Create((Em *) em, 0xB, 0, 1.0f);
}

void EmCephadrome_SpawnSurfaceEffect2(Em *em, f32 scale)
{
    f32 pos[3];
    f32 y;

    if ((gScene.area == em->area) && !(gScene.frame & 3)) {
        y = em->surfaceY;
        if (!(em->posY < ((y - *(f32 *)&em->surfaceDepth) - 100.0f))) {
            Vec_Set(pos, em->posX, y, em->posZ);
            Effect08_Spawn(pos, 2, 2, scale * em->scaleX);
        }
    }
}

void EmCephadrome_SpawnSurfaceEffect5(Em *em, f32 scale)
{
    f32 pos[3];

    if (gScene.area == em->area) {
        Vec_Set(pos, em->posX, em->surfaceY, em->posZ);
        Effect08_Spawn(pos, 5, 2, scale * em->scaleX);
        Em_PlaySeAtBone(em, 0x1A, 6);
    }
}

void EmCephadrome_SpawnEffect21(Em *em, s32 kind)
{
    if (em->type == 8) {
        EffCephadrome21_Create(em, kind);
        return;
    }
    EffCephadrome23_Create(em, kind);
}

void EmCephadrome_VibrateNearby(Em *em)
{
    Player *p;
    s8 i;

    for (p = gPlayers, i = 0; i < gScene.playerCount; i++, p++) {
        if (i == gScene.localPlayer && p->active && Obj_SameArea(em, p) && p->active && Vec_Distance((f32 *) p->pos, (f32 *) &em->posX) <= 1000.0f) {
            Vib_StartLocal(p, 3);
        }
    }
}

void func_007DE230(void)
{
}

void EmCephadrome_SetGroup0State(Em *em, u16 state, u8 mode)
{
    em->motionSpeed = 1.0f;
    switch (state) {
    case 0x1:
        break;
    case 5:
    case 14:
        Em_BeginStatus2(em);
        break;
    case 16:
        Em_BeginStatus1(em);
        break;
    case 18:
        Em_ResetNotice(em);
        break;
    case 6:
    case 15:
        Em_EndStatus2(em);
        break;
    case 17:
        Em_EndStatus1(em);
        break;
    case 19:
        Em_Refill7F4(em);
        break;
    case 21:
        Em_EndStatus4(em);
        break;
    case 22:
        em->holdTimer2 = 0;
        break;
    case 23:
        Em_EndStatus5(em);
        break;
    case 40:
        Em_StartGoalAction(em, 0x2710);
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

void EmCephadrome_SetGroup1State(Em *em, u16 state, u8 mode)
{
    em->stance = 0;
    em->motionSpeed = 1.0f;
    switch (state) {
    case 2:
        break;
    case 0:
        if (em->hp < (s16) (em->hpMax * 0.1f)) {
            state = 3;
        }
    case 1:
    case 3:
        Em_SetSpeedScaled(em, 6, 1000.0f, 550.0f);
        break;
    }
    Em_SetStateSync(em, 1, state, mode);
}

void EmCephadrome_SetGroup2State(Em *em, u16 state, u8 mode)
{
    EmCephadromeWork *w = (EmCephadromeWork *)&em->work;
    f32 pos[3];
    s32 a;

    em->motionSpeed = 1.0f;
    switch (state) {
    case 26:
        if (state == 27) {
            em->noReact = 1;
            em->senseDelay = 5;
            em->noHitTimer = 5;
            em->noPushTimer = 5;
        }
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
    case 4:
    case 5:
    case 27:
        Em_SetSpeedScaled(em, 0, 1000.0f, 500.0f);
        if (state == 27) {
            em->noReact = 1;
            em->senseDelay = 5;
            em->noHitTimer = 5;
            em->noPushTimer = 5;
        }
        break;
    case 6:
        state = 8;
        break;
    case 10:
    case 11:
        w->unk007 = 4;
        if (em->remote == 0) {
            em->goalNo = EmCephadrome_NextAreaPoint(em, pos);
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
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
    case 22:
    case 23:
        break;
    }
    Em_SetStateSync(em, 2, state, mode);
}

void EmCephadrome_SetGroup3State(Em *em, u16 state, u8 mode)
{
    EmCephadromeWork *w = (EmCephadromeWork *)&em->work;

    em->motionSpeed = 1.0f;
    switch (state) {
    case 0:
    case 2:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 3:
        break;
    case 1:
        w->unk013 = 1;
        break;
    }
    Em_SetStateSync(em, 0x3, state, mode);
}

void EmCephadrome_ChangeState(Em *em, u16 group, u16 state, u8 mode)
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
        EmCephadrome_SetGroup0State(em, state, mode);
        return;
    case 1:
        EmCephadrome_SetGroup1State(em, state, mode);
        return;
    case 2:
        EmCephadrome_SetGroup2State(em, state, mode);
        return;
    case 3:
        EmCephadrome_SetGroup3State(em, state, mode);
        return;
    case 4:
        switch (state) {
        case 7:
        case 11:
            Em_BeginStatus4(em);
            break;
        case 19:
            Em_EndStatus4(em);
            break;
        case 21:
            Em_EndAllStatus(em);
            break;
        case 22:
        case 23:
            Em_BeginStatus5(em);
            break;
        }
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        return;
    case 5:
        if ((state == 4) && (QuestEm_CanAppear(em) == 1)) {
            QuestEm_Appear(em);
        }
        Em_SetStateSync(em, group, state, mode);
        return;
    case 6:
    case 8:
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        
    default:
        return;
    }
}

void EmCephadrome_StartCurves(Em *em, u8 kind)
{
    EmCephadromeWork *w = (EmCephadromeWork *)&em->work;

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

static s32 EmCephadrome_CurveNone(Em *em, EmWork *work)
{
    return 0;
}

static u8 EmCephadrome_CurveVelY(Em *em, EmWork *work)
{
    EmCurveKey *p;
    f32 frame;
    f32 t;
    s32 i;
    EmCurveKey *tbl;
    u8 ret;

    tbl = EmCephadrome_VelYCurves[((EmCephadromeWork *)work)->curveKind];
    ret = 0;
    if (tbl == NULL) {
        return ret;
    }
    if (em->motion[0].blend != 0) {
        frame = 0.0f;
    } else {
        frame = ((EmCephadromeWork *)work)->curveFrame;
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

static u8 EmCephadrome_CurveSpeed(Em *em, EmWork *work)
{
    EmCurveKey *p;
    f32 frame;
    f32 t;
    s32 i;
    EmCurveKey *tbl;
    u8 ret;

    tbl = EmCephadrome_Curves[((EmCephadromeWork *)work)->curveKind];
    ret = 0;
    if (tbl == NULL) {
        return ret;
    }
    if (em->motion[0].blend != 0) {
        frame = 0.0f;
    } else {
        frame = ((EmCephadromeWork *)work)->curveFrame;
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

u8 EmCephadrome_UpdateCurves(Em *em)
{
    EmCephadromeWork *w = (EmCephadromeWork *)&em->work;
    u8 ret = 0;

    if (w->curveNone != 0) {
        ret = EmCephadrome_CurveNone(em, (EmWork *) w);
    }
    if (w->curveVelY != 0) {
        ret |= EmCephadrome_CurveVelY(em, (EmWork *) w);
    }
    if (w->curveSpeed != 0) {
        ret |= EmCephadrome_CurveSpeed(em, (EmWork *) w);
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

u8 EmCephadrome_NextAreaPoint(Em *em, f32 *out)
{
    EmBEntry *e;
    f32 *p;
    u32 i;
    u8 k;
    f32 best;
    f32 d;
    u8 n;
    f32 (*sel)[3];

    k = 0;
    e = (EmBEntry *)Em_FindAreaEntry(em, (struct EmIdEntry *)(((EmBEntry **) em->tables)[2]), em->area);
    if (e == NULL || (p = e->pts) == NULL) {
        out[0] = 1000.0f;
        out[1] = 0.0f;
        out[2] = 1000.0f;
        k = 0;
    } else {
        for (i = 0; i < e->n; i++, p += 3) {
            d = Vec_DistanceXZ(&em->posX, p);
            if (k == i) {
                best = d;
            } else if (best > d) {
                best = d;
                k = i;
            }
        }
        n = k + 1;
        if (n >= e->n) {
            n -= e->n;
        }
        sel = (f32 (*)[3]) e->pts;
        out[0] = sel[n][0];
        out[1] = sel[n][1];
        out[2] = sel[n][2];
    }
    return k;
}

s32 EmCephadrome_ShouldEndState(Em *em)
{
    if (!(Player_InCurrentArea(em) & 0xFF)) {
        return 0;
    }
    if ((em->surfaceKind == 0) && (em->stance == 4)) {
        return 1;
    }
    return 0;
}

void EmCephadrome_StepRouteIndex(Em *em, u8 dir)
{
    EmBEntry *e;

    if (em->type == 0x22) {
        switch (Area_DayId(em->area)) {
        case 0x8D:
        case 0x91:
        case 0x92:
            if ((em->stateRand & 7) == 0) {
                em->curGoalNo += 2;
                em->curGoalNo &= 3;
            }
            break;
        case 0x8E:
            break;
        }
    } else {
        switch (Area_DayId(em->area)) {
        case 0x8D:
            if ((em->stateRand & 7) == 0) {
                switch (em->curGoalNo) {
                case 0:
                    em->curGoalNo = 3;
                    return;
                case 1:
                    em->curGoalNo = 4;
                    return;
                case 3:
                    em->curGoalNo = 0;
                    return;
                case 4:
                    em->curGoalNo = 1;
                    return;
                }
            }
            break;
        case 0x91:
            if ((em->stateRand & 7) == 0) {
                switch (em->curGoalNo) {
                case 0:
                    em->curGoalNo = 2;
                    return;
                case 1:
                    em->curGoalNo = 4;
                    return;
                case 2:
                    em->curGoalNo = 0;
                    return;
                case 4:
                    em->curGoalNo = 1;
                    return;
                }
            }
            break;
        case 0x8E:
            break;
        }
    }
    if (dir == 0) {
        em->curGoalNo++;
    } else {
        em->curGoalNo--;
    }
    e = (EmBEntry *)Em_FindAreaEntry(em, (struct EmIdEntry *)(((EmBEntry **) em->tables)[2]), em->area);
    if (e != NULL) {
        while (em->curGoalNo >= e->n) {
            em->curGoalNo -= e->n;
        }
    } else {
        em->curGoalNo = 0;
    }
}

void EmMelynx_MotionSounds(Em *em, EmWork *work)
{
    s16 motion;

    motion = work->fxMotion;
    switch (motion) {
    case 0x3E9:
        Em_PlaySeOnFrame(em, 4, Sound_RandomSe(0x12, 1, 0x12, 1));
        break;
    case 0x3EA:
        Em_PlaySeOnFrame(em, 0x12, 1);
        Em_PlaySeOnFrame(em, 0x26, 1);
        Em_PlaySeOnFrame(em, 0x72, 1);
        Em_PlaySeOnFrame(em, 0x7E, 1);
        Em_PlaySeOnFrame(em, 0x98, 1);
        Em_PlaySeOnFrame(em, 0xCC, 0xC);
        Em_PlaySeOnFrame(em, 8, 0x1E);
        Em_PlaySeOnFrame(em, 0x58, 0x1E);
        Em_PlaySeOnFrame(em, 0xD0, 0x1E);
        break;
    case 0x3EB:
        Em_PlaySeOnFrame(em, 0x26, 0x12);
        Em_PlaySeOnFrame(em, 6, 0x1E);
        Em_PlaySeOnFrame(em, 0x16, 0x1E);
        Em_PlaySeOnFrame(em, 0x9A, 0x1E);
        Em_PlaySeOnFrame(em, 0xA4, 0x1E);
        break;
    case 0x3EC:
        Em_PlaySeOnFrame(em, 0xE, Sound_RandomSe(9, 2, 9, 2));
        Em_PlaySeOnFrame(em, 0x4C, Sound_RandomSe(0xC, 2, 0xC, 2));
        Em_PlaySeOnFrame(em, 0xA0, Sound_RandomSe(9, 2, 9, 2));
        Em_PlaySeOnFrame(em, 0xC8, Sound_RandomSe(0xA, 2, 0xA, 2));
        Em_PlaySeOnFrame(em, 8, 0x1E);
        Em_PlaySeOnFrame(em, 0x50, 0x1E);
        Em_PlaySeOnFrame(em, 0x8C, 0x1E);
        Em_PlaySeOnFrame(em, 0xC8, 0x1E);
        break;
    case 0x3ED:
        Em_PlaySeOnFrame(em, 4, 0x17);
        Em_PlaySeOnFrame(em, 0xE, 3);
        break;
    case 0x3F3:
        Em_PlaySeOnFrame(em, 0x50, Sound_RandomSe(0x12, 2, 0x12, 2));
        Em_PlaySeOnFrame(em, 0xC, 0x1E);
        Em_PlaySeOnFrame(em, 0x1E, 0x1E);
        Em_PlaySeOnFrame(em, 0x2E, 0x1E);
        Em_PlaySeOnFrame(em, 0x42, 0x1E);
        Em_PlaySeOnFrame(em, 0x58, 0x1E);
        Em_PlaySeOnFrame(em, 0x76, 0x1E);
        Em_PlaySeOnFrame(em, 0x8C, 0x1E);
        Em_PlaySeOnFrame(em, 0x98, 0x1E);
        Em_PlaySeOnFrame(em, 0xA2, 0x1E);
        Em_PlaySeOnFrame(em, 0xAE, 0x1E);
        Em_PlaySeOnFrame(em, 0xB8, 0x1E);
        Em_PlaySeOnFrame(em, 0xC2, 0x1E);
        Em_PlaySeOnFrame(em, 0xAE, 0x1E);
        break;
    case 0x3F4:
        Em_PlaySeOnFrame(em, 8, Sound_RandomSe(0xE, 2, 0xE, 2));
        Em_PlaySeOnFrame(em, 0xA, 0x1E);
        Em_PlaySeOnFrame(em, 0x16, 0x1E);
        Em_PlaySeOnFrame(em, 0x20, 0x1E);
        Em_PlaySeOnFrame(em, 0x2E, 0x1E);
        Em_PlaySeOnFrame(em, 0x38, 0x1E);
        Em_PlaySeOnFrame(em, 0x44, 0x1E);
        Em_PlaySeOnFrame(em, 0x4E, 0x1E);
        Em_PlaySeOnFrame(em, 0x58, 0x1E);
        Em_PlaySeOnFrame(em, 0x64, 0x1E);
        Em_PlaySeOnFrame(em, 0x70, 0x1E);
        break;
    case 0x3F5:
        Em_PlaySeOnFrame(em, 8, Sound_RandomSe(0xD, 2, 0xD, 2));
        Em_PlaySeOnFrame(em, 0xC, 0x21);
        Em_PlaySeOnFrame(em, 0xC, 0x19);
        Em_PlaySeOnFrame(em, 0x2A, 0x17);
        Em_PlaySeOnFrame(em, 0x2E, 0x18);
        break;
    case 0x3F6:
        Em_PlaySeOnFrame(em, 8, 0x1E);
        Em_PlaySeOnFrame(em, 0x18, 0x1E);
        Em_PlaySeOnFrame(em, 0x28, 0x1E);
        break;
    case 0x3F7:
        Em_PlaySeOnFrame(em, 8, Sound_RandomSe(0, 2, 0, 2));
        Em_PlaySeOnFrame(em, 0xC, 0x18);
        Em_PlaySeOnFrame(em, 0x12, 0x22);
        Em_PlaySeOnFrame(em, 0x22, 0x18);
        Em_PlaySeOnFrame(em, 0x28, 0x22);
        break;
    case 0x3F8:
        Em_PlaySeOnFrame(em, 0x2E, Sound_RandomSe(0x11, 1, 0x11, 1));
        Em_PlaySeOnFrame(em, 0xA, 0x17);
        Em_PlaySeOnFrame(em, 0x18, 0x17);
        Em_PlaySeOnFrame(em, 0x26, 0x17);
        Em_PlaySeOnFrame(em, 0x32, 0x17);
        break;
    case 0x3F9:
        Em_PlaySeOnFrame(em, 0x10, 0x1A);
        Em_PlaySeOnFrame(em, 0x24, 0x1A);
        Em_PlaySeOnFrame(em, 0x3A, 0x1A);
        Em_PlaySeOnFrame(em, 0x4E, 0x1A);
        Em_PlaySeOnFrame(em, 0x7E, 0x14);
        Em_PlaySeOnFrame(em, 0x80, 0x19);
        Em_PlaySeOnFrame(em, 0x78, 6);
        Em_PlaySeOnFrame(em, 0x9C, 0x1B);
        break;
    case 0x3FD:
        Em_PlaySeOnFrame(em, 2, 0x16);
        Em_PlaySeOnFrame(em, 0xE, 0x17);
        Em_PlaySeOnFrame(em, 0x18, 3);
        Em_PlaySeOnFrame(em, 0x48, 0x17);
        break;
    case 0x3FE:
        Em_PlaySeOnFrame(em, 2, 0x18);
        Em_PlaySeOnFrame(em, 6, 0x21);
        Em_PlaySeOnFrame(em, 0x10, 4);
        break;
    case 0x3FF:
        Em_PlaySeOnFrame(em, 0x12, 0x18);
        Em_PlaySeOnFrame(em, 0x22, 0x17);
        Em_PlaySeOnFrame(em, 0x2A, 0xB);
        Em_PlaySeOnFrame(em, 0x70, 0xF);
        Em_PlaySeOnFrame(em, 0xAA, 0xF);
        Em_PlaySeOnFrame(em, 0xF0, 0xD);
        Em_PlaySeOnFrame(em, 0x68, 0x16);
        Em_PlaySeOnFrame(em, 0x82, 0x17);
        Em_PlaySeOnFrame(em, 0x8A, 0x16);
        Em_PlaySeOnFrame(em, 0xA2, 0x17);
        Em_PlaySeOnFrame(em, 0xA8, 0x16);
        Em_PlaySeOnFrame(em, 0xBE, 0x17);
        Em_PlaySeOnFrame(em, 0xC8, 0x16);
        Em_PlaySeOnFrame(em, 0xE0, 0x17);
        Em_PlaySeOnFrame(em, 0xE6, 0x20);
        Em_PlaySeOnFrame(em, 0x124, 0x17);
        break;
    case 0x400:
        Em_PlaySeOnFrame(em, 0x10, 0x1E);
        Em_PlaySeOnFrame(em, 0x1A, 0x1E);
        Em_PlaySeOnFrame(em, 0x24, 0x1E);
        Em_PlaySeOnFrame(em, 0x30, 0x1E);
        Em_PlaySeOnFrame(em, 0x3A, 0x1E);
        Em_PlaySeOnFrame(em, 0x46, 0x1E);
        Em_PlaySeOnFrame(em, 0x52, 0x1E);
        Em_PlaySeOnFrame(em, 0x92, 0x1E);
        break;
    case 0x407:
        Em_PlaySeOnFrame(em, 0xE, 0xB);
        Em_PlaySeOnFrame(em, 0x44, 0x13);
        Em_PlaySeOnFrame(em, 0x52, 0x13);
        Em_PlaySeOnFrame(em, 0x5E, 0x13);
        Em_PlaySeOnFrame(em, 0x44, 0x16);
        Em_PlaySeOnFrame(em, 0x52, 0x16);
        Em_PlaySeOnFrame(em, 0x5E, 0x16);
        Em_PlaySeOnFrame(em, 0x70, 0x1C);
        Em_PlaySeOnFrame(em, 0x72, 6);
        break;
    case 0x408:
        Em_PlaySeOnFrame(em, 4, 0x15);
        Em_PlaySeOnFrame(em, 0xC, 0x18);
        Em_PlaySeOnFrame(em, 8, Sound_RandomSe(5, 4, 7, 4));
        Em_PlaySeOnFrame(em, 0x1E, 0x15);
        Em_PlaySeOnFrame(em, 0x36, 0x1A);
        break;
    case 0x409:
        Em_PlaySeOnFrame(em, 8, 0x15);
        Em_PlaySeOnFrame(em, 6, 0x19);
        Em_PlaySeOnFrame(em, 0x26, 0x21);
        Em_PlaySeOnFrame(em, 0x26, 0x1A);
        break;
    case 0x40A:
        Em_PlaySeOnFrame(em, 8, 0x17);
        Em_PlaySeOnFrame(em, 0xC, 0x21);
        Em_PlaySeOnFrame(em, 0x26, 0x21);
        break;
    case 0x40B:
        Em_PlaySeOnFrame(em, 0xA, 0x17);
        Em_PlaySeOnFrame(em, 0xE, 0x21);
        Em_PlaySeOnFrame(em, 0xE, Sound_RandomSe(5, 4, 7, 4));
        Em_PlaySeOnFrame(em, 0x16, 0x15);
        Em_PlaySeOnFrame(em, 0x1E, 0x15);
        Em_PlaySeOnFrame(em, 0x26, 0x15);
        Em_PlaySeOnFrame(em, 0x2E, 0x15);
        Em_PlaySeOnFrame(em, 0x34, 0x15);
        Em_PlaySeOnFrame(em, 0x3C, 0x15);
        Em_PlaySeOnFrame(em, 0x44, 0x15);
        Em_PlaySeOnFrame(em, 0x52, 0x17);
        Em_PlaySeOnFrame(em, 0x6C, 0x1E);
        break;
    case 0x40C:
        if ((em->stateGroup == 0) && (em->state == 0xF)) {
            Em_PlaySeOnFrame(em, 4, 0x11);
            Em_PlaySeOnFrame(em, 0x54, 0x19);
            Em_PlaySeOnFrame(em, 0x5E, 0x19);
            Em_PlaySeOnFrame(em, 0x78, 0x18);
            Em_PlaySeOnFrame(em, 0x3E, 0x24);
            break;
        }
        Em_PlaySeOnFrame(em, 4, 0x11);
        Em_PlaySeOnFrame(em, 0x54, 0x19);
        Em_PlaySeOnFrame(em, 0x5E, 0x19);
        Em_PlaySeOnFrame(em, 0x78, 0x18);
        Em_PlaySeOnFrame(em, 0x46, 0x23);
        break;
    case 0x40D:
        Em_PlaySeOnFrame(em, 8, 0x18);
        Em_PlaySeOnFrame(em, 0xE, 0x15);
        Em_PlaySeOnFrame(em, 0x16, 0x15);
        Em_PlaySeOnFrame(em, 0x1E, 0x15);
        Em_PlaySeOnFrame(em, 0x26, 0x15);
        Em_PlaySeOnFrame(em, 0x2E, 0x15);
        Em_PlaySeOnFrame(em, 0x34, 0x15);
        Em_PlaySeOnFrame(em, 0x44, 0x1F);
        Em_PlaySeOnFrame(em, 0x54, 0x1F);
        Em_PlaySeOnFrame(em, 0x72, 0x1E);
        break;
    case 0x40E:
        Em_PlaySeOnFrame(em, 0x12, Sound_RandomSe(8, 4, 0xA, 4));
        Em_PlaySeOnFrame(em, 0x3C, Sound_RandomSe(9, 4, 0xC, 4));
        Em_PlaySeOnFrame(em, 0x10, 0x15);
        Em_PlaySeOnFrame(em, 0x40, 0x15);
        Em_PlaySeOnFrame(em, 0x10, 0x18);
        Em_PlaySeOnFrame(em, 0x40, 0x18);
        break;
    case 0x424:
        Em_PlaySeOnFrame(em, 4, Sound_RandomSe(5, 4, 7, 4));
        Em_PlaySeOnFrame(em, 0x32, 0x10);
        Em_PlaySeOnFrame(em, 0xC, 0x1E);
        Em_PlaySeOnFrame(em, 0x16, 0x1E);
        Em_PlaySeOnFrame(em, 0x16, 0x1E);
        Em_PlaySeOnFrame(em, 0x20, 0x1E);
        Em_PlaySeOnFrame(em, 0x2E, 0x1E);
        Em_PlaySeOnFrame(em, 0x46, 0x1E);
        Em_PlaySeOnFrame(em, 0xD2, 0x1C);
        break;
    case 0x425:
        Em_PlaySeOnFrame(em, 4, Sound_RandomSe(5, 4, 7, 4));
        break;
    case 0x426:
        Em_PlaySeOnFrame(em, 4, Sound_RandomSe(4, 2, 4, 2));
        Em_PlaySeOnFrame(em, 4, 0x1C);
        Em_PlaySeOnFrame(em, 0x2E, 0x1D);
        Em_PlaySeOnFrame(em, 4, Sound_RandomSe(6, 3, 6, 3));
        Em_PlaySeOnFrame(em, 0x64, 0x1C);
        Em_PlaySeOnFrame(em, 0xA, 0x1A);
        Em_PlaySeOnFrame(em, 0x34, 0x1A);
        break;
    case 0x427:
        Em_PlaySeOnFrame(em, 0x1C, 0x15);
        Em_PlaySeOnFrame(em, 0x28, 0x15);
        Em_PlaySeOnFrame(em, 0x34, 0x15);
        Em_PlaySeOnFrame(em, 0x40, 0x15);
        Em_PlaySeOnFrame(em, 0x1C, 0x11);
        Em_PlaySeOnFrame(em, 0x66, 4);
        Em_PlaySeOnFrame(em, 0x68, 0x16);
        Em_PlaySeOnFrame(em, 0x80, 0x17);
        Em_PlaySeOnFrame(em, 0x84, 0x18);
        break;
    case 0x428:
        Em_PlaySeOnFrame(em, 0x14, 0x1E);
        Em_PlaySeOnFrame(em, 0x22, 0x1E);
        Em_PlaySeOnFrame(em, 0x2A, 0x1E);
        Em_PlaySeOnFrame(em, 0x46, 0x1E);
        Em_PlaySeOnFrame(em, 0x54, 0x1E);
        Em_PlaySeOnFrame(em, 0x6C, 0x1E);
        Em_PlaySeOnFrame(em, 0x8C, 0x13);
        break;
    case 0x429:
        Em_PlaySeOnFrame(em, 0x30, 0x13);
        Em_PlaySeOnFrame(em, 4, 6);
        break;
    case 0x42A:
        Em_PlaySeOnFrame(em, 4, 0x13);
        Em_PlaySeOnFrame(em, 0x40, 0x1A);
        Em_PlaySeOnFrame(em, 0x5A, 0x1D);
        break;
    case 0x42B:
        Em_PlaySeOnFrame(em, 4, 0x12);
        Em_PlaySeOnFrame(em, 0x9C, 0x1D);
        break;
    case 0x42E:
        Em_PlaySeOnFrame(em, 4, 0xC);
        Em_PlaySeOnFrame(em, 0x100, 1);
        break;
    case 0x42F:
        Em_PlaySeOnFrame(em, 0x10, 0x18);
        Em_PlaySeOnFrame(em, 0x12, 0x18);
        Em_PlaySeOnFrame(em, 0x24, 0xC);
        break;
    case 0x430:
        Em_PlaySeOnFrame(em, 0x30, 0xD);
        Em_PlaySeOnFrame(em, 8, 0x1E);
        break;
    case 0x431:
        Em_PlaySeOnFrame(em, 0xA, 0x12);
        break;
    case 0x432:
        Em_PlaySeOnFrame(em, 8, 0xA);
        Em_PlaySeOnFrame(em, 0x34, 4);
        Em_PlaySeOnFrame(em, 0x36, 0x21);
        Em_PlaySeOnFrame(em, 0x36, 0x20);
        Em_PlaySeOnFrame(em, 0x16, 0x18);
        Em_PlaySeOnFrame(em, 0x1E, 0x18);
        Em_PlaySeOnFrame(em, 0x80, 0x18);
        Em_PlaySeOnFrame(em, 0x94, 0x18);
        break;
    case 0x433:
        Em_PlaySeOnFrame(em, 0xE, 0xB);
        break;
    }
}

void EmMelynx_Reset(Em *em)
{
    EmMelynxWork *w = (EmMelynxWork *)&em->work;
    f32 fslot;
    u8 slotNo;
    s16 hp;

    if (gQuest.stage == 0) {
        if (Area_DayId(gScene.area) == 0xF) {
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
    em->blind = 0;
    em->remote = 0;
    Em_SetMotionRow(em, 1, 0, 0);
    em->stance = 0;
    hp = Em_ComputeSize(em, 0);
    em->hp = hp;
    em->hpMax = hp;
    w->unk00E = 0;
    w->unk00C = 0;
    w->home[0] = em->posX;
    w->home[1] = 0.0f;
    w->home[2] = em->posZ;
    w->alertState = 0;
    w->stolenItem = -1;
    em->painted = 0;
    if (em->variant & 0x80) {
        em->scaleZ = 10.0f;
        em->scaleY = 10.0f;
        em->scaleX = 10.0f;
        em->bodyScale = 10.0f;
        GmEm_ApplySize(em);
    }
    switch (em->spawnParam) {
    case 1:
        em->spawnParam = 0;
        Em_SetCombat(em, 1, gEmCombatInit[em->type]);
        break;
    }
    Em_InitParts(em);
}

void EmMelynx_Start(Em *em)
{
    EmMelynx_Reset(em);
    if (em->variant == 0xC) {
        EmMelynx_ChangeState(em, 0, 1, 0);
        return;
    }
    if ((Area_DayId(em->area) & 0xFFFF) == 0x94) {
        if (em->variant != 0) {
            em->noHitTimer = 0xA;
            em->noPushTimer = 0xA;
        }
        EmMelynx_ChangeState(em, 0, 1, 0);
        return;
    }
    EmMelynx_ChangeState(em, 1, 8, 0);
}

void EmMelynx_Reinit(Em *em)
{
    EmMelynx_Reset(em);
}

void EmMelynx_DeductReward(Em *em)
{
    EmMelynxWork *w = (EmMelynxWork *)&em->work;
    s16 v = w->stolenItem;

    if (v != -1) {
        Quest_DeductReward(w->stolenFrom, v, w->stolenCount);
    }
}

void EmMelynx_CapTimerOffTarget(Em *em)
{
    s8 t;

    if (em->timer > 0xF) {
        t = em->targetNo;
        if (t == -1) {
            em->timer = 0xF;
        } else if (em->area != ((Player *) &gPlayers[t])->area) {
            em->timer = 0xF;
        }
    }
}

void EmMelynx_ChangeState(Em *em, u16 group, u16 state, u8 mode)
{
    EmMelynxWork *w = (EmMelynxWork *)&em->work;

    em->unk446 = 0;
    switch (group) {
    case 0:
        em->stance = 0;
        switch (state) {
        case 11:
            break;
        case 15:
            break;
        case 16:
            Em_EndStatus5(em);
            break;
        }
        break;
    case 1:
        em->stance = 0;
        switch (state) {
        case 4:
        default:
            w->unk00E = 1;
        case 5:
        case 6:
            Em_ComputeGoal2(em, em->goal);
            break;
        case 2:
            em->timer = 600;
            break;
        }
        break;
    case 3:
        em->stance = 0;
        break;
    case 4:
        switch (state) {
        case 1:
            break;
        case 4:
            em->stance = 0;
            Em_BeginStatus5(em);
            break;
        default:
            em->stance = 0;
            break;
        }
        break;
    case 5:
        if (state == 2 && QuestEm_CanAppear(em) == 1) {
            QuestEm_Appear(em);
        }
        break;
    case 2:
    case 6:
        break;
    }
    Em_SetStateSync(em, group, state, mode);
}

void EmMelynx_EndState(Em *em)
{
    EmMelynxWork *w = (EmMelynxWork *)&em->work;

    u16 temp_s0;

    em->motionSpeed = 1.0f;
    em->scriptPending = 1;
    temp_s0 = (u16) w->unk00E;
    EmMelynx_ChangeState(em, 0, 1, 0);
    w->unk00E = (s16) temp_s0;
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
}

s32 EmCephadrome_EffEm14State(Em *em)
{
    u8 *w = &em->work;

    if (em->stance != 0 || (u32) (em->stateGroup - 4) <= 2 || em->active == 0) {
        return -1;
    }
    if (em->attackHit800 != 0) {
        return -2;
    }
    if (*(u16 *) (w + 0xE) != 0) {
        return 1;
    }
    return 0;
}

s32 EmMelynx_StealItem(Em *em, Player *p, EmMelynxWork *w)
{
    s32 list[20];
    s32 i;
    s32 n;

    if (p->index != gScene.localPlayer) {
        return 0;
    }
    if (Quest_IsRunning(1)) {
        return 0;
    }
    if (Player_HasSkill(p, 0x5C) == 1) {
        return 0;
    }
    if (Pouch_Count(p, 0xBD) > 0) {
        w->stolenItem = 0xBD;
        w->stolenCount = -1;
    } else if (Pouch_Count(p, 0xF9) > 0) {
        w->stolenItem = 0xF9;
        w->stolenCount = -1;
    } else {
        for (i = 0, n = 0; i < 20; i++) {
            if (p->pouch[i].item != 0 && gItemData[p->pouch[i].item].type == 0 && gItemData[p->pouch[i].item].rarity < 4) {
                list[n] = i;
                n++;
            }
        }
        if (n == 0) {
            return 0;
        }
        w->stolenItem = p->pouch[list[em->stateRand % n]].item;
        w->stolenCount = -1;
    }
    w->stolenFrom = p;
    Pouch_Add(p, (u16)w->stolenItem, w->stolenCount);
    Notice_Push(1, 6, w->stolenItem);
    return 1;
}

void EmMelynx_ReturnItem(Em *em, EmMelynxWork *w)
{
    if (w->stolenItem != -1) {
        switch ((u16) Pouch_Add(w->stolenFrom, w->stolenItem, -w->stolenCount)) {
        case 0:
        case 1:
        case 2:
            if (Player_IsLocal(w->stolenFrom) == 1) {
                Notice_Push(1, 7, w->stolenItem);
            }
            break;
        case 3:
            if (Player_IsLocal(w->stolenFrom) == 1) {
                Notice_Push(1, 3, w->stolenItem);
            }
            break;
        default:
            if (Player_IsLocal(w->stolenFrom) == 1) {
                Notice_Push(0, 0xC, 0);
            }
            break;
        }
        w->stolenItem = -1;
    }
}

void EmMelynx_State0_00(Em *em, s32 unused)
{
    switch (em->step) {
    case 0:
        em->step++;
        if (em->motionId[0] != 0x3E9) {
            Em_SetMotion(em, 1, 0, 0);
        }
        em->timer = (em->stateRand & 0xF) + 5;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->scriptPending = 1;
        }
        break;
    }
}

void EmMelynx_State0_01(Em *em, s32 unused)
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
            EmMelynx_EndState(em);
        }
        break;
    }
}

void EmMelynx_State0_02(Em *em, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        switch (variant) {
        case 0:
            Em_SetMotion(em, 2, 0, 0);
            break;
        case 1:
            Em_SetMotion(em, 3, 0, 0);
            break;
        case 2:
            Em_SetMotion(em, 4, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmMelynx_EndState(em);
        }
        break;
    }
}

void EmMelynx_State0_05(Em *em, s32 unused)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 5, 0, 0);
        em->timer = 0x78;
        break;
    case 1:
        if (--em->timer <= 0) {
            EmMelynx_EndState(em);
        }
        break;
    }
}

void EmMelynx_State0_06(Em *em, s32 unused)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x1F, 0, 0);
        StageObj_Stun_CreateEm(em, 1, 2, 0x3C, 30.0f, 1.0f);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x3F, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmMelynx_EndState(em);
        }
        break;
    }
}

void EmMelynx_State0_07(Em *em, s32 unused)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x25, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmMelynx_EndState(em);
        }
        break;
    }
}

void EmMelynx_State0_08(Em *em, s32 unused)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = (em->stateRand & 0x3F) + 0x3C;
        Em_SetMotion(em, 0x26, 0, 0);
        break;
    case 1:
        if (--em->timer <= 0) {
            EmMelynx_EndState(em);
        }
        break;
    }
}

void EmMelynx_State0_09(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 0x96;
        Em_SetMotion(em, 0x20, 0, 0);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x22, 4, 4);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmMelynx_EndState(em);
        }
        break;
    }
}

void EmMelynx_State0_0A(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 0x78;
        Em_SetMotion(em, 0x23, 0, 0);
        em->blind = 0;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmMelynx_EndState(em);
        }
        break;
    }
}

void EmMelynx_State0_0B(Em *em)
{
    EmMelynxWork *w = (EmMelynxWork *)&em->work;

    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 0x14;
        Em_SetMotion(em, 0x24, 0, 0);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            em->timer = 0x6E;
            w->unk00E = 1;
            EffEm14_Create(em, 0);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            EmMelynx_EndState(em);
        }
        break;
    }
}

void EmMelynx_State0_0C(Em *em)
{
    f32 pos[3];

    Em_SetFlag6AE(em, 2);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x43, 0, 0);
        Em_BeginStatus2(em);
        break;
    case 1:
        pos[0] = 0.0f;
        pos[1] = 0.0f;
        pos[2] = 10.0f;
        Em_SpawnEffect06Periodic(em, 0xA, pos, 0.6f);
        if (--em->timer <= 0) {
            em->step++;
            EmMelynx_ChangeState(em, 0, 0xD, 4);
        }
        break;
    }
}

void EmMelynx_State0_0D(Em *em)
{
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        Em_EndStatus2(em);
        Em_SetMotion(em, 0x3F, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            EmMelynx_EndState(em);
        }
        break;
    }
}

void EmMelynx_State0_0E(Em *em)
{
    EmMelynxWork *w;
    s16 v;

    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x11, 0, 0);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 144.0f, 0) != 0) {
            em->step++;
        }
        break;
    case 2:
        em->noHitTimer = 10;
        em->noPushTimer = 10;
        if (em->motion[0].playing == 0) {
            w = (EmMelynxWork *)&em->work;
            if (w->alertState == 1) {
                ObjListA70_Remove(&w->sense);
                w->alertState = 2;
            }
            v = w->stolenItem;
            if (v != -1) {
                Quest_DeductReward(w->stolenFrom, v, w->stolenCount);
            }
            QuestEm_Appear(em);
        }
        break;
    }
}

void EmMelynx_State0_0F(Em *em)
{
    EmMelynxWork *w = (EmMelynxWork *)&em->work;

    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 0x14;
        Em_SetMotion(em, 0x24, 0, 0);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            em->timer = 0x6E;
            w->unk00E = 1;
            EffEm14_Create(em, 1);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            EmMelynx_EndState(em);
        }
        break;
    }
}

void EmMelynx_State0_10(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x3F, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmMelynx_EndState(em);
        }
        break;
    }
}

void EmMelynx_RunGroup0(Em *em)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->state) {
    case 0:
        EmMelynx_State0_00(em, 0);
        break;
    case 1:
        EmMelynx_State0_01(em, 0);
        break;
    case 2:
        EmMelynx_State0_02(em, 0);
        break;
    case 3:
        EmMelynx_State0_02(em, 1);
        break;
    case 4:
        EmMelynx_State0_02(em, 2);
        break;
    case 5:
        EmMelynx_State0_05(em, 2);
        break;
    case 6:
        EmMelynx_State0_06(em, 0);
        break;
    case 7:
        EmMelynx_State0_07(em, 0);
        break;
    case 8:
        EmMelynx_State0_08(em, 0);
        break;
    case 9:
        EmMelynx_State0_09(em);
        break;
    case 10:
        EmMelynx_State0_0A(em);
        break;
    case 11:
        EmMelynx_State0_0B(em);
        break;
    case 12:
        EmMelynx_State0_0C(em);
        break;
    case 13:
        EmMelynx_State0_0D(em);
        break;
    case 14:
        EmMelynx_State0_0E(em);
        break;
    case 15:
        EmMelynx_State0_0F(em);
        break;
    case 16:
        EmMelynx_State0_10(em);
        
    default:
        break;
    }
}

void EmMelynx_State1_00(Em *em)
{
    EmMelynxWork *w = (EmMelynxWork *)&em->work;

    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 300;
        w->stateTimer = 0x8A;
        Em_SetMotion(em, 0xC, 0, 0);
        break;
    case 1:
        if (Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) < 100.0f || --em->timer <= 0) {
            EmMelynx_EndState(em);
            break;
        }
        em->targetAngle = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        Angle_TurnToward((s32 *) &em->rotY, (s32 *) &em->targetAngle, 0x800);
        Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
        EmMelynx_CapTimerOffTarget(em);
        break;
    }
}

void EmMelynx_State1_01(Em *em)
{
    u8 step;

    step = em->step;
    switch (step) {
    case 0:
        em->step = step + 1;
        Em_SetMotion(em, 0x21, 0, 0);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 46.0f, 0U) != 0) {
            em->step++;
            Em_SetMotion(em, 0xE, 0, 0);
            em->timer = 0x12C;
        }
        break;
    case 2:
        EmMelynx_CapTimerOffTarget(em);
        if (!(Vec_DistanceXZ(&em->posX, em->goal) <= 200.0f)) {
            if (--em->timer <= 0) {
                goto block_12;
            }
        } else {
block_12:
            em->step++;
            Em_SetMotion(em, 0x22, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            EmMelynx_EndState(em);
        }
        break;
    }
}

void EmMelynx_State1_02(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0xB, 0, 0);
        em->targetAngle = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        break;
    case 1:
        if (Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) < 100.0f || --em->timer <= 0) {
            EmMelynx_EndState(em);
            break;
        }
        Angle_TurnToward((s32 *) &em->rotY, (s32 *) &em->targetAngle, 0x800);
        Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
        break;
    }
}

void EmMelynx_State1_04(Em *em)
{
    s32 t;

    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 0x8C;
        Em_SetMotion(em, 0x10, 0, 0);
        em->targetAngle = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        Eff02_Create(em, 4);
        break;
    case 1:
        if ((Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) < 100.0f) || ((em->attackHit800 != 0) && (em->contact->isEm == 0)) || (t = em->timer - 1, em->timer = t, (t <= 0))) {
            EmMelynx_EndState(em);
            break;
        }
        em->targetAngle = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        Angle_TurnToward((s32 *) &em->rotY, (s32 *) &em->targetAngle, 0x800);
        Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
        EmMelynx_CapTimerOffTarget(em);
        break;
    }
}

void EmMelynx_State1_05(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->targetAngle = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        Em_SetMotion(em, 0xB, 0, 0);
        Player_SetFlag((Player *) em, 0x20000U);
        break;
    case 1:
        if (Angle_TurnToward((s32 *) &em->rotY, (s32 *) &em->targetAngle, 0x800) < 0x800) {
            Player_ClearFlag(em, 0x20000);
            EmMelynx_EndState(em);
        }
        break;
    }
    Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
}

void EmMelynx_State1_06(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 300;
        Em_SetMotion(em, 0xC, 0, 0);
        em->targetAngle = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        break;
    case 1:
        if (Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) < 100.0f || --em->timer <= 0) {
            EmMelynx_EndState(em);
            break;
        }
        Angle_TurnToward((s32 *) &em->rotY, (s32 *) &em->targetAngle, 0x800);
        Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
        break;
    }
}

void EmMelynx_State1_07(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 300;
        Em_SetMotion(em, 0xF, 0, 0);
        em->targetAngle = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        break;
    case 1:
        if (Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) < 100.0f || --em->timer <= 0) {
            EmMelynx_EndState(em);
            break;
        }
        Angle_TurnToward((s32 *) &em->rotY, (s32 *) &em->targetAngle, 0x800);
        Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
        break;
    }
}

void EmMelynx_State1_08(Em *em)
{
    s32 timer;

    switch (em->step) {
    case 0:
        em->step++;
        em->timer = (em->stateRand & 0x7F) + 0x78;
        Em_SetMotion(em, 0xB, 0, 0);
        em->targetAngle = (em->stateRand & 0xF) << 0xC;
        break;
    case 1:
        timer = em->timer;
        if (timer > 0) {
            em->timer = timer - 1;
        }
        if ((em->remote == 0) && (em->timer <= 0)) {
            EmMelynx_EndState(em);
            break;
        }
        Angle_TurnToward((s32 *) &em->rotY, (s32 *) &em->targetAngle, 0x100);
        Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
        break;
    }
}

void EmMelynx_State1_09(Em *em)
{
    s32 t;

    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 0x64;
        Em_SetMotion(em, 0x10, 0, 0);
        em->targetAngle = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        break;
    case 1:
        if ((Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) < 100.0f) || ((em->attackHit800 != 0) && (em->contact->isEm == 0)) || (t = em->timer - 1, em->timer = t, (t <= 0))) {
            EmMelynx_EndState(em);
            break;
        }
        em->targetAngle = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        Angle_TurnToward((s32 *) &em->rotY, (s32 *) &em->targetAngle, 0x800);
        Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
        EmMelynx_CapTimerOffTarget(em);
        break;
    }
}

void EmMelynx_State1_0A(Em *em)
{
    s32 timer;

    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 0x78;
        Em_SetMotion(em, 0xF, 0, 0);
        em->targetAngle = (em->stateRand & 0xF) << 0xC;
        break;
    case 1:
        timer = em->timer;
        if (timer > 0) {
            em->timer = timer - 1;
        }
        if ((em->remote == 0) && (em->timer <= 0)) {
            EmMelynx_EndState(em);
            break;
        }
        Angle_TurnToward((s32 *) &em->rotY, (s32 *) &em->targetAngle, 0x100);
        Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
        break;
    }
}

void EmMelynx_State1_0B(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 300;
        Em_SetMotion(em, 0xB, 0, 0);
        em->targetAngle = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        break;
    case 1:
        if (Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) < 100.0f || --em->timer <= 0) {
            EmMelynx_EndState(em);
            break;
        }
        Angle_TurnToward((s32 *) &em->rotY, (s32 *) &em->targetAngle, 0x800);
        Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
        break;
    }
}

void EmMelynx_State1_0C(Em *em)
{
    EmMelynxWork *w = (EmMelynxWork *)&em->work;
    s32 timer;

    switch (em->step) {
    case 0:
        em->step++;
        em->timer = (em->stateRand & 0x7F) + 0x78;
        Em_SetMotion(em, 0xB, 0, 0);
        break;
    case 1:
        timer = em->timer;
        if (timer > 0) {
            em->timer = timer - 1;
        }
        if ((em->remote == 0) && (em->timer <= 0)) {
            EmMelynx_EndState(em);
            break;
        }
        em->targetAngle = Math_AngleXZ(&em->posX, w->home) & 0xFFFF;
        Angle_TurnToward((s32 *) &em->rotY, (s32 *) &em->targetAngle, 0x100);
        Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
        break;
    }
}

void EmMelynx_RunGroup1(Em *em)
{
    switch (em->state) {
    case 0:
        EmMelynx_State1_00(em);
        break;
    case 1:
        EmMelynx_State1_01(em);
        break;
    case 2:
        EmMelynx_State1_02(em);
        break;
    case 4:
        EmMelynx_State1_04(em);
        break;
    case 5:
        EmMelynx_State1_05(em);
        break;
    case 6:
        EmMelynx_State1_06(em);
        break;
    case 7:
        EmMelynx_State1_07(em);
        break;
    case 8:
        EmMelynx_State1_08(em);
        break;
    case 9:
        EmMelynx_State1_09(em);
        break;
    case 10:
        EmMelynx_State1_0A(em);
        break;
    case 11:
        EmMelynx_State1_0B(em);
        break;
    case 12:
        EmMelynx_State1_0C(em);
        
    default:
        break;
    }
}

void EmMelynx_State2_00(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0xD, 0, 0);
        em->velY = 0.0f;
        em->accelY = -0.5f;
        break;
    case 1:
        Player_Integrate(em);
        if (em->posY <= em->floorY) {
            em->stance = 0;
            em->posY = em->floorY;
            EmMelynx_EndState(em);
        }
        break;
    }
}

void EmMelynx_RunGroup2(Em *em)
{
    switch (em->state) {
    case 0:
        EmMelynx_State2_00(em);
        break;
    }
}

void EmMelynx_State3_00(Em *em)
{
    f32 floorY;
    u8 stance;

    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x15, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stance = 2;
            em->velX = 0.0f;
            em->velY = 16.0f;
            em->velY = 30.0f;
            em->velZ = 5.0f;
            Vec_RotateY(&em->velX, (6.2831855f * (f32) em->rotY) / 65536.0f);
            em->accelX = 0.0f;
            em->accelY = -1.0f;
            em->accelY = -3.0f;
            em->accelZ = 0.0f;
            Em_SetMotion(em, 0x16, 0, 0);
            Eff02_Create(em, 1);
            em->stateVar0 = 0;
        }
        break;
    case 2:
        Player_Integrate(em);
        if ((em->motion[0].playing == 0) && ((em->posY - 50.0f) <= em->floorY)) {
            if (em->hitPending == 0) {
                em->step++;
                Em_SetMotion(em, 0x18, 0, 0);
                em->velY *= 0.5f;
                em->accelY = -0.5f;
                em->stateVar1 = 0;
                break;
            }
            em->step++;
            Em_SetMotion(em, 0x17, 0, 0);
            em->stateVar1 = 1;
        }
        break;
    case 3:
        stance = em->stance;
        if (stance != 0) {
            floorY = em->floorY;
            if ((em->posY <= floorY) && (stance != 0)) {
                em->posY = floorY;
                em->stance = 0;
            } else {
                Player_Integrate(em);
            }
        }
        if (em->motion[0].playing == 0) {
            em->posY = em->floorY;
            em->stance = 0;
            if (em->stateVar1 != 0) {
                em->stateVar1 = 0;
                EmMelynx_EndState(em);
                break;
            }
            EmMelynx_ChangeState(em, 0, 3, 4);
        }
        break;
    }
}

void EmMelynx_State3_01(Em *em)
{
    EmMelynxWork *w = (EmMelynxWork *)&em->work;
    Player *p;

    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0xF, 0, 0);
        em->targetAngle = Math_AngleXZ(&em->posX, em->goal);
        if (em->state == 3) {
            em->timer = 120;
            w->eff = Eff02_Create(em, 7);
            em->motionSpeed = 1.2f;
        } else {
            em->timer = 60;
            w->eff = Eff02_Create(em, 2);
            em->motionSpeed = 1.0f;
        }
        break;
    case 1:
        if (em->attackHit800 != 0 && (p = em->contact) != NULL && p->isEm == 0 && p == &gPlayers[p->index] && p->active) {
            EmMelynx_StealItem(em, p, w);
            Em_SetRequest(em, 1, 0x90);
            em->step++;
            Em_SetMotion(em, 0x17, 0, 0);
        } else if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 3, 0, 0);
        } else {
            if (em->state == 3) {
                em->targetAngle = Math_AngleXZ(&em->posX, em->goal);
            }
            Angle_TurnToward((s32 *) &em->rotY, (s32 *) &em->targetAngle, 0x800);
            Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
            EmMelynx_CapTimerOffTarget(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmMelynx_EndState(em);
        }
        break;
    }
}

void EmMelynx_State3_02(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->velX = 0.0f;
        em->velY = 16.0f;
        em->velY = 30.0f;
        em->velZ = 5.0f;
        Vec_RotateY(&em->velX, 6.2831855f * em->rotY / 65536.0f);
        em->accelX = 0.0f;
        em->accelY = -1.0f;
        em->accelY = -3.0f;
        em->accelZ = 0.0f;
        Em_SetMotion(em, 0x16, 0, 0);
        Eff02_Create(em, 1);
        em->stateVar0 = 0;
        break;
    case 1:
        Player_Integrate(em);
        if (em->motion[0].playing == 0 && em->posY - 50.0f <= em->floorY) {
            if (em->hitPending == 0) {
                em->step++;
                Em_SetMotion(em, 0x18, 0, 0);
                em->velY *= 0.5f;
                em->accelY = -0.5f;
                em->stateVar1 = 0;
            } else {
                em->step++;
                Em_SetMotion(em, 0x18, 0, 0);
                em->stateVar1 = 1;
            }
        }
        break;
    case 2:
        if (em->stance != 0) {
            if (em->posY <= em->floorY && em->stance != 0) {
                em->posY = em->floorY;
                em->stance = 0;
            } else {
                Player_Integrate(em);
            }
        }
        if (em->motion[0].playing == 0) {
            em->posY = em->floorY;
            em->stance = 0;
            if (em->stateVar1 != 0) {
                em->stateVar1 = 0;
                EmMelynx_EndState(em);
                break;
            }
            EmMelynx_ChangeState(em, 0, 3, 4);
        }
        break;
    }
}

void EmMelynx_State3_04(Em *em)
{
    EmMelynxWork *w = (EmMelynxWork *)&em->work;

    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 0x14;
        Em_SetMotion(em, 0x24, 0, 0);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            em->timer = 0x3C;
            w->unk00E = 1;
            if (em->state == 4) {
                EffEm14_Create(em, 2);
                break;
            }
            EffEm14_Create(em, 3);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x4A, 0, 0);
        }
        break;
    case 3:
        if (Player_PassedFrameScaled(em, 48.0f, 0U) != 0) {
            em->step++;
            w->unk00E = 0;
        }
        
    case 4:
        if (em->motion[0].playing == 0) {
            EmMelynx_EndState(em);
        }
        break;
    }
}

void EmMelynx_RunGroup3(Em *em)
{
    switch (em->state) {
    case 0:
        EmMelynx_State3_00(em);
        break;
    case 1:
        EmMelynx_State3_01(em);
        break;
    case 2:
        EmMelynx_State3_02(em);
        break;
    case 3:
        EmMelynx_State3_01(em);
        break;
    case 4:
        EmMelynx_State3_04(em);
        break;
    case 5:
        EmMelynx_State3_04(em);
        
    default:
        break;
    }
}

void EmMelynx_State4_00(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x3C, 0, 0);
        em->rotY = em->hitAngle + 0x7FFF + 1;
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
            EmMelynx_EndState(em);
        }
        break;
    }
}

void EmMelynx_State4_01(Em *em)
{
    f32 in[3];
    f32 out[3];
    s32 ang[3];
    f32 mtx[16];

    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x3D, 0, 0);
        ang[0] = 0;
        em->rotY = ang[1] = em->hitAngle + 0x7FFF + 1;
        ang[2] = 0;
        Mtx_FromAnglesXYZ((s32 *) ang, mtx);
        in[0] = 0.0f;
        in[1] = 10.0f;
        in[2] = -12.0f;
        Vec_Rotate(out, in, mtx);
        Player_ClearAccel2(em);
        em->velX = out[0];
        em->velY = out[1];
        em->velZ = out[2];
        em->accelY = -0.4f;
        em->accelZ = 0.15f;
        em->stance = 2;
        Em_RestartScript(em);
        break;
    case 1:
        if (!(em->velZ * em->accelZ < 0.0f)) {
            em->accelZ = 0.0f;
        }
        if (Player_MoveFall((Player *) em) != 0) {
            em->step++;
            em->stance = 0;
            Em_SetMotion(em, 0x3E, 0, 0);
            em->timer = 90;
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x3F, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            EmMelynx_EndState(em);
        }
        break;
    }
}

void EmMelynx_State4_02(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x1F, 0, 0);
        Em_SetRequest(em, 0U, 0x90U);
        Em_RestartScript(em);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x3F, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmMelynx_EndState(em);
        }
        break;
    }
}

void EmMelynx_State4_03(Em *em)
{
    EmMelynxWork *w = (EmMelynxWork *)&em->work;

    if (em->timer > 0) {
        em->timer--;
    }
    if (em->step <= 3) {
        Em_SpawnEffect06Bone(em, 2);
        Em_SetFlag6AE(em, 4);
    }
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x3C, 0, 0);
        em->rotY = em->hitAngle + 0x7FFF + 1;
        Em_BeginStatus4(em);
        em->noReact = 1;
        Em_RestartScript(em);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 120.0f, 0) != 0) {
            em->step++;
            Em_SetMotion(em, 0x41, 0, 0);
            w->stateTimer = 0x3A;
        }
        break;
    case 2:
        if (--w->stateTimer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x42, 0, 0);
        }
        break;
    case 3:
        if (em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x40, 0, 0);
            Em_EndStatus4(em);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->noReact = 0;
            EmMelynx_EndState(em);
        }
        break;
    }
}

void EmMelynx_State4_04(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x1F, 0, 0);
        StageObj_Stun_CreateEm(em, 1, 0, *(s16 *)((u8 *)em + 8), 90.0f, 1.0f);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmMelynx_ChangeState(em, 0, 0x10, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmMelynx_ChangeState(em, 0, 0x10, 4);
        }
        break;
    }
}

void EmMelynx_RunGroup4(Em *em)
{
    switch (em->state) {
    case 0:
        EmMelynx_State4_00(em);
        break;
    case 1:
        EmMelynx_State4_01(em);
        break;
    case 2:
        EmMelynx_State4_02(em);
        break;
    case 3:
        EmMelynx_State4_03(em);
        break;
    case 4:
        EmMelynx_State4_04(em);
        break;
    }
}

void EmMelynx_State5_00_Hunted(Em *em)
{
    EmMelynxWork *w = (EmMelynxWork *)&em->work;
    f32 in[3];
    f32 out[3];
    s32 ang[3];
    f32 mtx[16];

    em->noHitTimer = 10;
    em->noPushTimer = 10;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x3D, 0, 0);
        in[0] = 0.0f;
        in[1] = 10.0f;
        ang[0] = 0;
        in[2] = -12.0f;
        em->rotY = ang[1] = em->hitAngle + 0x7FFF + 1;
        ang[2] = 0;
        Mtx_FromAnglesXYZ((s32 *) ang, mtx);
        Vec_Rotate(out, in, mtx);
        Player_ClearAccel2(em);
        em->velX = out[0];
        em->velY = out[1];
        em->velZ = out[2];
        em->accelY = -0.4f;
        em->accelZ = 0.15f;
        em->stance = 2;
        EmMelynx_ReturnItem(em, w);
        QuestEm_Hunted(em);
        break;
    case 1:
        if (!(em->velZ * em->accelZ < 0.0f)) {
            em->accelZ = 0.0f;
        }
        if (Player_MoveFall((Player *) em) != 0) {
            em->step++;
            em->stance = 0;
            Em_SetMotion(em, 0x3E, 0, 0);
            em->timer = 300;
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x3F, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x11, 0, 0);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            if (w->alertState == 1) {
                ObjListA70_Remove(&w->sense);
                w->alertState = 2;
            }
            em->visible = 0;
            if (em->type == 0x17) {
                EmMine_Drop(em, &em->posX);
                EmMelynx_ChangeState(em, 5, 2, 4);
            } else {
                EmMelynx_ChangeState(em, 5, 2, 4);
            }
        }
        break;
    case 5:
        if (--em->timer <= 0) {
            EmMine_Remove(em);
            EmMelynx_ChangeState(em, 5, 2, 4);
        }
        break;
    }
}

void EmMelynx_State5_01_Hunted(Em *em)
{
    EmMelynxWork *w = (EmMelynxWork *)&em->work;

    em->noHitTimer = 10;
    em->noPushTimer = 10;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x3C, 0, 0);
        em->rotY = em->hitAngle + 0x7FFF + 1;
        em->stance = 0;
        EmMelynx_ReturnItem(em, w);
        QuestEm_Hunted(em);
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
            Em_SetMotion(em, 0x11, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            if (w->alertState == 1) {
                ObjListA70_Remove(&w->sense);
                w->alertState = 2;
            }
            em->visible = 0;
            if (em->type == 0x17) {
                EmMine_Drop(em, &em->posX);
                EmMelynx_ChangeState(em, 5, 2, 4);
            } else {
                EmMelynx_ChangeState(em, 5, 2, 4);
            }
        }
        break;
    case 4:
        if (--em->timer <= 0) {
            EmMine_Remove(em);
            EmMelynx_ChangeState(em, 5, 2, 4);
        }
        break;
    }
}

void EmMelynx_State5_02(Em *em)
{
    switch (em->step) {
    case 0:
        if (QuestEm_CanAppear(em) == 1) {
            if (em->variant == 0xC) {
                EmMelynx_ChangeState(em, 0, 1, 0);
                break;
            }
            EmMelynx_ChangeState(em, 1, 8, 0);
            break;
        }
        em->taskPhase++;
    case 1:
        break;
    }
}

void EmMelynx_RunGroup5(Em *em)
{
    switch (em->state) {
    case 0:
        EmMelynx_State5_00_Hunted(em);
        break;
    case 1:
        EmMelynx_State5_01_Hunted(em);
        break;
    case 2:
        EmMelynx_State5_02(em);
        break;
    }
}

void EmMelynx_State6_00(Em *em)
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
        EmMelynx_EndState(em);
        break;
    }
}

void EmMelynx_RunGroup6(Em *em)
{
    switch (em->state) {
    case 0:
        EmMelynx_State6_00(em);
        break;
    }
}

void EmMelynx_Update(Em *em)
{
    u8 buf[4];
    EmMelynxWork *w = (EmMelynxWork *)&em->work;
    u32 r;

    if (Area_DayId(em->area) == 0x94 && em->variant != 0) {
        em->noHitTimer = 10;
        em->noPushTimer = 10;
    }
    if (w->unk00C != 0) {
        w->unk00C -= 1;
    }
    if (em->combat == 1 && em->rage == 0) {
        Em_CheckRageByHp(em, (s16) (em->rageThreshold / 1800));
    }
    r = (u8) Em_SenseObjects(em, &w->sense);
    if (w->alertState == 0 && em->stance == 0 && em->combat == 0 && r != 0 && Em_ScriptFlagsFrom(em, 4) == 0) {
        em->triggers |= 4;
        w->alertState = 2;
        em->blind = 0;
    }
    if (w->alertState == 1) {
        ObjListA70_Remove(&w->sense);
        w->alertState = 2;
    }
    r = Em_UpdateDamage(em, buf) & 0xFF;
    switch (r) {
    case 0:
    case 3:
    case 4:
    case 9:
    case 11:
    case 15:
    case 16:
    case 18:
        break;
    case 1:
    case 2:
        if (em->stance == 2) {
            EmMelynx_ChangeState(em, 5, 0, 3);
        }
        EmMelynx_ChangeState(em, 5, 1, 3);
        break;
    case 5:
        if (Player_IsState((Player *) em, 4, 1) == 0) {
            EmMelynx_ChangeState(em, 4, 1, 3);
            Em_ResetPartHp(em, 0);
        }
        break;
    case 6:
        if (Player_IsState((Player *) em, 4, 1) == 0) {
            Em_SetStatus4Timer(em);
            EmMelynx_ChangeState(em, 4, 3, 3);
        }
        break;
    case 7:
        EmMelynx_ChangeState(em, 4, 1, 3);
        Em_ResetPartHp(em, 0);
        break;
    case 8:
        if (Player_IsState((Player *) em, 4, 1) == 0) {
            Em_SetStatus2Timer(em);
            EmMelynx_ChangeState(em, 0, 0xC, 3);
        }
        break;
    case 10:
        em->blind = 0;
        Em_EndStatus2(em);
        EmMelynx_ChangeState(em, 4, 1, 3);
        break;
    case 12:
    case 13:
        EmMelynx_ChangeState(em, 4, 1, 3);
        Em_ResetPartHp(em, 0);
        break;
    case 14:
        if (em->stance == 2) {
            EmMelynx_ChangeState(em, 4, 1, 3);
            Em_ResetPartHp(em, 0);
        } else {
            EmMelynx_ChangeState(em, 4, 0, 3);
        }
        em->scriptPending = 0;
        break;
    case 17:
        if ((em->stateGroup != 4 || em->state != 4) && (em->stateGroup != 4 || em->state != 1)) {
            Em_SetStatus5Timer(em);
            EmMelynx_ChangeState(em, 4, 4, 3);
        }
        break;
    }
    if ((u8) r != 0) {
        w->unk00E = 0;
        em->blind = 0;
        if (w->alertState == 0) {
            Vec_Set(w->sense.pos, em->posX, em->posY, em->posZ);
            w->sense.radius = 3000.0f;
            w->sense.area = (u8) em->area;
            w->sense.kind = 1;
            ObjListA70_Add(&w->sense);
            w->alertState = 1;
        }
    }
    Em_UpdatePendingScript(em);
    Em_ClearFlag6AE(em);
    EmMelynx_RunState(em);
    if (em->stateRequest != 0) {
        EmMelynx_RunState(em);
        em->stateRequest = 0;
    }
}

void EmMelynx_RunState(Em *em)
{
    switch (em->stateGroup) {
    case 0:
        EmMelynx_RunGroup0(em);
        break;
    case 1:
        EmMelynx_RunGroup1(em);
        break;
    case 2:
        EmMelynx_RunGroup2(em);
        break;
    case 3:
        EmMelynx_RunGroup3(em);
        break;
    case 4:
        EmMelynx_RunGroup4(em);
        break;
    case 5:
        EmMelynx_RunGroup5(em);
        break;
    case 6:
        EmMelynx_RunGroup6(em);
        break;
    case 7:
        EmMelynx_RunGroup0(em);
        
    default:
        break;
    }
}

void EmMelynx_MotionEffectsDefault(Em *em)
{
}

void EmMelynx_MotionEffects(Em *em, EmWork *work)
{
    if (em->motionId[0] != work->fxMotion) {
        work->fxMotion = em->motionId[0];
    }
    EmMelynx_MotionSounds(em, work);
    switch (work->fxMotion) {
    case 0x3E9:
    case 0x3EA:
    case 0x3EB:
    case 0x3EC:
    case 0x3ED:
    case 0x3F3:
    case 0x3F4:
    case 0x3F5:
    case 0x3F6:
    case 0x3F7:
    case 0x3F8:
        break;
    case 0x3F9:
        if (Player_PassedFrameScaled(em, 18.0f, 0)) {
            StageObj_K14_Create(em, 3, 4, 0.5f);
        }
        if (Player_PassedFrameScaled(em, 58.0f, 0)) {
            StageObj_K14_Create(em, 3, 4, 0.5f);
        }
        if (Player_PassedFrameScaled(em, 38.0f, 0)) {
            StageObj_K14_Create(em, 2, 4, 0.5f);
        }
        if (Player_PassedFrameScaled(em, 126.0f, 0)) {
            Effect0D_SpawnForEm(em, 2, 7);
        }
        if (Player_PassedFrameScaled(em, 158.0f, 0)) {
            Effect0D_SpawnForEm(em, 2, 3);
        }
        if (Player_PassedFrameScaled(em, 166.0f, 0)) {
            Effect0D_SpawnForEm(em, 2, 3);
        }
        if (Player_PassedFrameScaled(em, 176.0f, 0)) {
            Effect0D_SpawnForEm(em, 2, 3);
        }
        break;
    case 0x3FD:
    case 0x3FE:
    case 0x3FF:
    case 0x400:
    case 0x407:
    case 0x408:
    case 0x409:
    case 0x40A:
    case 0x40B:
    case 0x40C:
    case 0x40D:
    case 0x40E:
    case 0x424:
    case 0x425:
    case 0x426:
    case 0x427:
    case 0x428:
    case 0x429:
    case 0x42A:
    case 0x42B:
    case 0x42E:
    case 0x42F:
    case 0x430:
    case 0x431:
    case 0x432:
    case 0x433:
        break;
    default:
        EmMelynx_MotionEffectsDefault(em);
        break;
    }
}

void EmMelynx_UpdateFace(Em *em)
{
    EmMelynxWork *w = (EmMelynxWork *)&em->work;

    switch (w->faceStep) {
    default:
        return;
    case 0:
        w->faceStep++;
        w->blinkTimer = em->stateRand & 0xF;
        break;
    case 1:
        if (em->stateGroup == 4 || em->stateGroup == 5) {
            w->eyeFrame = 3;
            w->blinkTimer = 2;
        } else if (em->motionId[0] != 0x42B) {
            (w->blinkTimer)++;
            switch ((w->blinkTimer >> 1) % 20) {
            case 1:
                w->eyeFrame = 1;
                break;
            case 2:
                w->eyeFrame = 2;
                break;
            default:
                w->eyeFrame = 0;
                break;
            }
        } else {
            w->blinkTimer = 0;
            w->eyeFrame = 2;
        }
        EmMelynx_MotionEffects(em, (EmWork *) w);
        break;
    }
}

void EmMelynx_SetMeshSetAlpha(Em *em, s32 kind, EmMelynxMeshSet *sets)
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
        case 1:
            switch (w[0x4E]) {
            case 0:
            default:
                switch (i) {
                case 2:
                case 3:
                case 4:
                    m->alpha = 0.0f;
                    break;
                }
                break;
            case 1:
                switch (i) {
                case 1:
                case 3:
                case 4:
                    m->alpha = 0.0f;
                    break;
                }
                break;
            case 2:
                switch (i) {
                case 1:
                case 2:
                case 4:
                    m->alpha = 0.0f;
                    break;
                }
                break;
            case 3:
                switch (i) {
                case 1:
                case 2:
                case 3:
                    m->alpha = 0.0f;
                    break;
                }
                break;
            }
            break;
        }
        Gs_SetState((i + 0x3A) & 0xFF, (s32) m);
    }
}

void EmMelynx_SpawnEffect01(u8 *em)
{
    Effect01_Spawn((Player *) em, 0);
}

void EmMelynx_TableNop(void)
{
}

void EmRathalos_MotionSounds(Em *em, EmWork *work)
{
    s16 motion;

    motion = work->sndMotion;
    switch (motion) {
    case 0x3EA:
        Em_PlaySeAtBoneOnFrame(em, 6, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 3, 0x1A);
        break;
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
    case 0x3F1:
        Em_PlaySeAtBoneOnFrame(em, 4, 6, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 6, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 4, 0x1A);
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
        Em_PlaySe6AtBoneOnFrame(em, 0x12, 0x4F, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 6, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 6, 0x1A);
        break;
    case 0x3FA:
        Em_PlaySeAtBoneOnFrame(em, 0x28, 7, 0x14);
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
        Em_PlaySeAtBoneOnFrame(em, 0x60, 0, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 7, 9);
        break;
    case 0x405:
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 2, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0xE, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 8, 0xE, 6);
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
        Em_PlaySe6AtBoneOnFrame(em, 0x30, 0x49, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x80, 0x22, 0x23);
        Em_PlaySe6AtBoneOnFrame(em, 0xB8, 0x4C, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x120, 0x22, 0x23);
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
    case 0x410:
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 0xD, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x66, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 0xD, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x88, 0xA, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x8C, 0xA, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 2, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 4, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 5, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 7, 0x1A);
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
        Em_PlaySe6AtBoneOnFrame(em, 0xC, 0x4F, 0x23);
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
        Em_PlaySe6AtBoneOnFrame(em, 8, 0x4B, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0xF, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x14, 0x2A);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 0, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 1, 0x1A);
        break;
    case 0x413:
        Em_PlaySe6AtBoneOnFrame(em, 8, 0x4B, 0x23);
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
        Em_PlaySeAtBoneOnFrame(em, 0x22, 0x5B, 0x23);
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
    case 0x416:
        Em_PlaySeAtBoneOnFrame(em, 6, 0x18, 0x23);
        Em_PlaySe6AtBoneOnFrame(em, 6, 0x51, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0xF, 6);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0xF, 0xC);
        break;
    case 0x417:
        Em_PlaySeAtBoneOnFrame(em, 2, 0x12, 0);
        if ((em->stateGroup != 8) || (em->state != 3)) {
            Em_PlaySeAtBoneOnFrame(em, 0x50, 0x18, 0x23);
        }
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0x28, 0x23);
        Em_PlaySe6AtBoneOnFrame(em, 0x4A, 0x51, 0x23);
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
    case 0x419:
        Em_PlaySeAtBoneOnFrame(em, 2, 0x48, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 0xF, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0xF, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x74, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x78, 0xB, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0xBE, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0xC2, 0xB, 0xC);
        break;
    case 0x41A:
        Em_PlaySe6AtBoneOnFrame(em, 0x14, 0x56, 0x23);
        Em_PlaySe6AtBoneOnFrame(em, 0x4E, 0x51, 0x23);
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
    case 0x41B:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x13, 0x2A);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 0x10, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0x12, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 6, 0x1A);
        Em_PlaySe6AtBoneOnFrame(em, 0xA0, 0x56, 0x23);
        Em_PlaySe6AtBoneOnFrame(em, 0xE6, 0x5D, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xEE, 0x16, 0);
        break;
    case 0x41C:
        Em_PlaySe6AtBoneOnFrame(em, 0x16, 0x56, 0x23);
        Em_PlaySe6AtBoneOnFrame(em, 0x48, 0x51, 0x23);
        if ((em->stateGroup == 3) && (em->state == 0x10)) {
            Em_PlaySeAtBoneOnFrame(em, 0x52, 0x18, 0x23);
        }
        Em_PlaySeAtBoneOnFrame(em, 6, 0x14, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 0xF, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 0xF, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x6E, 0xB, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0xAC, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0xB0, 0xB, 0xC);
        break;
    case 0x41D:
        Em_PlaySe6AtBoneOnFrame(em, 0x50, 0x49, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 0x30, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0xF, 6);
        Em_PlaySeAtBoneOnFrame(em, 8, 0xF, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 0xF, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0xF, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x42, 0xF, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 0xF, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x5C, 0xF, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 0xF, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x70, 0xF, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x74, 0xF, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x92, 0xF, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x96, 0xF, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0xBC, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0xB8, 0xB, 0xC);
        break;
    case 0x41E:
        Em_PlaySe6AtBoneOnFrame(em, 0x10, 0x56, 0x23);
        Em_PlaySe6AtBoneInFrames(em, 0x6E, 0x118, 0x5C, 0x23, 7, 0);
        Em_PlaySeAtBoneOnFrame(em, 2, 0x12, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 4, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 4, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 5, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x78, 4, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x108, 4, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x14, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0xC, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 0xC, 6);
        break;
    case 0x41F:
        Em_PlaySeAtBoneOnFrame(em, 8, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 8, 8, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 5, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 1, 0x14);
        break;
    case 0x421:
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x12, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x28, 0x1A);
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
        Em_PlaySeAtBoneOnFrame(em, 0x6E, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0xAC, 3, 0x1A);
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
        Em_PlaySeAtBoneOnFrame(em, 0x44, 0x20, 0x23);
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
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0x31, 0x23);
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
        Em_PlaySeAtBoneOnFrame(em, 0x18, 9, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0x11, 0);
        break;
    case 0x434:
        Em_PlaySe6AtBoneOnFrame(em, 0x10, 0x54, 0x23);
        Em_PlaySe6AtBoneOnFrame(em, 0x74, 0x5A, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 1, 0x1A);
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
        Em_PlaySeAtBoneOnFrame(em, 0x10, 0x2C, 0x23);
        break;
    case 0x437:
        Em_PlaySeAtBoneOnFrame(em, 0x26, 1, 0x1A);
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
        Em_PlaySe6AtBoneOnFrame(em, 0x32, 0x54, 0x23);
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
        Em_PlaySe6AtBoneOnFrame(em, 4, 0x53, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xA8, 0x27, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xA2, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 0xA6, 0xD, 0xC);
        break;
    case 0x43C:
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0x2A, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0xD, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 0xA, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0xE, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 0xE, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 0xA, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 0xC, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x5C, 0xC, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 5, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x56, 3, 0x14);
        break;
    case 0x43F:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x1A, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 8, 0x1B, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x1B, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 0x1C, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 0x1C, 0x22);
        break;
    case 0x442:
        Em_PlaySe6AtBoneOnFrame(em, 0x30, 0x51, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0xD, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 0xD, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x7C, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x80, 0xB, 0xC);
        break;
    case 0x443:
        Em_PlaySeAtBoneOnFrame(em, 0x10, 6, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 6, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0xB0, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0xD6, 3, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x108, 1, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x130, 1, 0x1A);
        Em_PlaySe6AtBoneOnFrame(em, 4, 0x5B, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 9, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x5C, 0x10, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x8C, 0x11, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 0xF, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0xF, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x10A, 0xC, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x10E, 0xC, 0xC);
        break;
    case 0x444:
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
    case 0x445:
        Em_PlaySeAtBoneOnFrame(em, 8, 0xA, 6);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0xA, 0xC);
        break;
    case 0x446:
        Em_PlaySeAtBoneOnFrame(em, 8, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 8, 8, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x68, 5, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 1, 0x14);
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
        Em_PlaySe6AtBoneOnFrame(em, 0x28, 0x53, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0);
        break;
    case 0x449:
        Em_PlaySeAtBoneOnFrame(em, 0x22, 0x16, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 0x16, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x62, 4, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 3, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x2A, 0x23);
        Em_PlaySe6AtBoneOnFrame(em, 4, 0x53, 0x23);
        break;
    case 0x44A:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 6);
        Em_PlaySeAtBoneOnFrame(em, 8, 0x16, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 3, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x50, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0x20, 0x23);
        
    case 0x3E9:
    default:
        break;
    }
}

/* em_cephadrome's .data (0x8C8C10-0x8C9100) */
extern u32 EmCephadrome_VelYCurves_3[];
EmTurnParams EmCephadrome_TurnParams[3] = {
#include "game/008C8C10.inc"
};
EmCephFinKey EmCephadrome_FinKeys2_0[8] = {
#include "game/008C8C70.inc"
};
EmCephFinKey EmCephadrome_FinKeys2_1[8] = {
#include "game/008C8C90.inc"
};
EmCephFinKey EmCephadrome_FinKeys2_2[8] = {
#include "game/008C8CB0.inc"
};
EmCephFinKey EmCephadrome_FinKeys2_3[8] = {
#include "game/008C8CD0.inc"
};
EmCephFinKey *EmCephadrome_FinKeys2[4] = {
    EmCephadrome_FinKeys2_0, EmCephadrome_FinKeys2_1, EmCephadrome_FinKeys2_2, EmCephadrome_FinKeys2_3,
};
EmCephFinKey EmCephadrome_FinKeys1_0[12] = {
#include "game/008C8D00.inc"
};
EmCephFinKey EmCephadrome_FinKeys1_1[12] = {
#include "game/008C8D30.inc"
};
EmCephFinKey EmCephadrome_FinKeys1_2[8] = {
#include "game/008C8D60.inc"
};
EmCephFinKey EmCephadrome_FinKeys1_3[8] = {
#include "game/008C8D80.inc"
};
EmCephFinKey *EmCephadrome_FinKeys1[4] = {
    EmCephadrome_FinKeys1_0, EmCephadrome_FinKeys1_1, EmCephadrome_FinKeys1_2, EmCephadrome_FinKeys1_3,
};
s16 EmCephadrome_FinWait1[4] = {
#include "game/008C8DB0.inc"
};
s16 EmCephadrome_FinWait2[4] = {
#include "game/008C8DB8.inc"
};
s16 EmCephadrome_FinRewait1[4] = {
#include "game/008C8DC0.inc"
};
s16 EmCephadrome_FinRewait2[4] = {
#include "game/008C8DC8.inc"
};
u16 EmCephadrome_FinFoldTarget[4] = {
#include "game/008C8DD0.inc"
};
s32 func_0090A9C0(void); /* gm_sub */
void *EmCephadrome_Callbacks[8] = {
    EmCephadrome_SpawnEffects, EmCephadrome_SpawnEffects, func_007DE230, EmCephadrome_UpdateMotionEffects,
    func_0090A9C0, EmCephadrome_Reinit, NULL, NULL,
};
EmCurveKey EmCephadrome_VelYCurves_0[16] = {
#include "game/008C8E00.inc"
};
EmCurveKey EmCephadrome_VelYCurves_1[16] = {
#include "game/008C8E80.inc"
};
EmCurveKey EmCephadrome_VelYCurves_2[12] = {
#include "game/008C8F00.inc"
};
EmCurveKey *EmCephadrome_VelYCurves[4] = {
    EmCephadrome_VelYCurves_0, EmCephadrome_VelYCurves_1, EmCephadrome_VelYCurves_2, (EmCurveKey *)EmCephadrome_VelYCurves_3,
};
EmCurveKey EmCephadrome_Curves_0[16] = {
#include "game/008C8F70.inc"
};
EmCurveKey EmCephadrome_Curves_1[16] = {
#include "game/008C8FF0.inc"
};
EmCurveKey EmCephadrome_Curves_2[11] = {
#include "game/008C9070.inc"
};
/* The curves EmCephadrome_CurveSpeed picks from. */
EmCurveKey *EmCephadrome_Curves[3] = {EmCephadrome_Curves_0, EmCephadrome_Curves_1, EmCephadrome_Curves_2};
void *EmMelynx_Callbacks[8] = {
    EmMelynx_SpawnEffect01, EmMelynx_SpawnEffect01, EmMelynx_TableNop, EmMelynx_UpdateFace,
    func_0090A9C0, EmMelynx_Reinit, NULL, NULL,
};
