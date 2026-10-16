/*
 * The Rathalos family: Rathian, Rathalos and their subspecies (Pink and
 * Gold Rathian, Silver and Azure Rathalos), then Fatalis, then the sound
 * tables of the Blangonga family (the code from Rathalos's setup function,
 * 0x7E7240, to Blangonga's).
 *
 * A Rathalos runs a state machine (include/em_rathalos.h names the states
 * and motions; include/em.h the groups): EmRathalos_Update reads the hits
 * (Em_UpdateDamage) and picks reactions, then EmRathalos_RunState calls the
 * group's dispatcher (EmRathalos_RunIdleState ... RunVersusState), which
 * calls one function per state with the monster and its work area
 * (EmWork; the flying families' view is EmFlyerWork). A state function
 * switches on Em.step; EmRathalos_ChangeState starts the next state through
 * the group's Set function, and EmRathalos_EndState / EndAirState hand the
 * choice back to the monster script. Flights move along curves
 * (EmRathalos_StartCurves, UpdateCurves) and Em_UpdateFlight;
 * EmRathalos_MotionEffects adds each motion's effects and sounds.
 *
 * Fatalis (EmFatalis_*: EM_KIND_FATALIS, _B and _C) has the same shape with
 * its own states and motions (include/em_fatalis.h); its Set*State,
 * ChangeState and curve functions are in em_bulldrome.c. Each Fatalis kind
 * also plays a scene in its own area (EmFatalis_Update), and leaves a
 * carcass with three mine points.
 *
 * The three EmBlangonga_MotionSounds functions at the end play the sounds of
 * the Blangonga family's motions (em_blangonga.c holds the rest).
 */
#include "common.h"
#include "scene.h"
#include "quest.h"
#include "mine_point.h"
#include "em.h"
#include "em_rathalos.h"
#include "em_fatalis.h"
#include "player.h"
#include "stageobj.h"
/* prototypes of callees */
void Em_SetPartScaled(Em *em, s32 bit);
void StageObj_K09_Create(Em *);
void Em_AreaExitPoint(Em *em, f32 *out);
s32 Area_ContainsPos(s32, f32 *);
void Em_EnterNextArea(Em *);
struct EmIdEntry;
f32 *Em_AreaEntryValue(Em *em, struct EmIdEntry *entry);
u16 Rand_Next(s32 stream);

/* prototypes */
void EmFatalis_ChangeState(Em *em, u16 group, u16 state, u8 mode);
s8 EmMine_CreateTimed0(Em *em, f32 *pos);
void EmRathalos_ChangeState(Em *em, u16 group, u16 state, u8 mode);
/* unit's own declaration: with the definition's void (Em *em, s16 n) (game/unk_708850.c), EmRathalos_EatNeed2: 50 of 241 words differ, EmRathalos_EatNeed1: 42 of 86 words differ */
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
void Em_SelectSightRow(Em *em, u8 idx);
void Em_MoveRotated(Em *em, void *src);
void Em_MoveRotatedAccel(Em *em, void *ang);
void Em_SetStateRand(Em *em, s32 arg1, s32 arg2);
void Em_LoadReactionSet(Em *em, u8 k);
void Em_ComputeGoal2(Em *em, f32 *target);
void Em_GetPitchToGoal(Em *em, s32 *out, f32 a, f32 b);
s32 Em_GetTargetPos(Em *em, u8 idx, f32 *out);
void Em_ClearNeed1State(Em *em);
void Em_ClearNeed2State(Em *em);
void Em_SpawnEffect06Bone(Em *em, s32 a);
void Em_AddNeed1(Em *em, s32 n);
void Em_AddNeed2(Em *em, s32 n);
void Em_SetHitReactMode(Em *em, u8 a);
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
void EmRathalos_TurnToGoal(Em *, EmWork *);
void EmRathalos_TakeOff(Em *, EmWork *);
void EmRathalos_Land(Em *, EmWork *);
void EmRathalos_HopToGoal(Em *, EmWork *);
void EmRathalos_FlyOutOfArea(Em *em, EmWork *work);
void EmRathalos_FlyToGoalAndLand(Em *, EmWork *);
void EmRathalos_FlyOutOfAreaAlt(Em *em, EmWork *work);
void EmRathalos_FlyIntoArea(Em *, EmWork *);
void EmRathalos_StepForward(Em *, s32);
void EmRathalos_QuestIntro(Em *, EmWork *);
void EmRathalos_Captured(Em *em);
void EmRathalos_StoryEvent(Em *);
void EmRathalos_VersusEatBait(Em *em, EmWork *work);
void EmRathalos_VersusPoseC(Em *, EmWork *);
void EmRathalos_Die(Em *em, EmWork *work);
void EmRathalos_DieHeld(Em *em, EmWork *work);
void EmRathalos_DieFalling(Em *em, EmWork *work);
void EmRathalos_RunIdleState(Em *em, EmWork *work);
void EmRathalos_RunAttackState(Em *em, EmWork *work);
void Effect0D_SpawnForEmScaled(Em *em, s32 a, s32 b, f32 scale);
void EmRathalos_MotionSounds(Em *em, EmWork *work);
void StageObj_K0F_CreateOnEm(Em *em, u8 k, u8 c, f32 scale);
void EmRathalos_MotionEffects(Em *em, EmFlyerWork *work);
void EmRathalos_SpawnK14Type0B(Em *em);
void EmRathalos_SpawnK14Type0AEvery5(Em *em);
void EmRathalos_MoveByVelYCurve(Em *em, s32 kind);
void EmRathalos_StartCurves(Em *em, u8 kind);
u8 EmRathalos_UpdateCurves(Em *em);
void EmFatalis_EndAirState(Em *em, u8 a);
void EmFatalis_Status2Lie(Em *, EmWork *);
void EmFatalis_TurnToGoal(Em *, EmWork *);
void EmFatalis_Hover(Em *, EmWork *);
void EmFatalis_Sweep(Em *em, EmWork *work, s32 variant);
void EmFatalis_Dive(Em *, EmWork *);
void EmFatalis_SweepDouble(Em *em, EmWork *work, s32 variant);
void EmFatalis_DiveB(Em *, EmWork *);
void EmFatalis_Blasts(Em *);
void EmFatalis_Fall(Em *, EmWork *);
void EmFatalis_KnockDown(Em *, EmWork *);
void EmFatalis_FallB(Em *, EmWork *);
void EmFatalis_Die(Em *em, EmWork *work);
void EmFatalis_DieB(Em *, EmWork *);
void EmFatalis_DieFalling(Em *, EmWork *);
void EmFatalis_EventLeave(Em *em, EmWork *work);
void EmFatalis_EventFlyIn(Em *, EmWork *);
void EmFatalis_EventGetUp(Em *, EmWork *);
void EmFatalis_EventDescend(Em *, EmWork *);
void EmFatalis_ShakeOnFrame(void *em, s32 frame, s32 strength);
void StageObj_K1C_CreateOnEmC(Em *em, u8 k, u8 c, f32 scale);
void EmFatalis_MotionEffects(Em *em, EmFatalisWork *work);
void EmFatalis_MoveByVelYCurve(Em *, s32);
void EmFatalis_StartCurves(Em *em, u8 arg1);
/* unit's own declaration: with the definition's u8 (Em *em) (game/em_bulldrome.c), EmFatalis_TakeOff: 19 of 71 words differ, EmFatalis_TakeOffAndRise: 46 of 91 words differ ... */
s32 EmFatalis_UpdateCurves(Em *);
void StageObj_K11_CreateAtBone(Em *em, s16 bone, s16 k, u16 c);
void StageObj_K13_Create(void *owner, s16 v, u8 k);
void StageObj_K14_Create(Em *em, s16 k, s16 c, f32 scale);
/* unit's own declaration: with the definition's void (Em *em, u8 k, s16 v, s16 t, f32 y, f32 scale) (game/unk_874B90.c), EmRathalos_Stun: 66 of 98 words differ, EmRathalos_StunDowned: 43 of 74 words differ */
void StageObj_Stun_CreateEm(Em *, s32, s32, s32, f32, f32);
/* unit's own declaration: with the definition's void (Em *em, u8 k, u16 w, s16 v, f32 scale) (game/unk_880D50.c), EmFatalis_SpawnFireball: 29 of 32 words differ */
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
void EmRathalos_SetIdleState(Em *em, u16 state, u8 mode);
void EmRathalos_SetMoveState(Em *em, u16 state, u8 mode);
void EmRathalos_SetFlyState(Em *em, u16 state, u8 mode);
void EmRathalos_SetAttackState(Em *em, u16 state, u8 mode);
/* unit's own declaration: with the definition's void (Em *em, s32 group, s32 state, u8 mode) (game/unk_701810.c), EmRathalos_ChangeState: 6 of 227 words differ, EmRathalos_SetIdleState: 1 of 114 words differ ... */
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
void Em_RefillNeed3(Em *em);
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
/* A position copied whole (EmRathalos_CarveThrowPt): the original copies it as one struct. */
typedef struct EmRathalosVec {
    f32 x, y, z;
} EmRathalosVec;
extern EmRathalosVec EmRathalos_CarveThrowPt __attribute__((aligned(8)));
u16 Math_Heading(f32 x1, f32 z1, f32 x2, f32 z2);
void Em_SetMotionRow(Em *em, s32 row, s32 arg2, s32 arg3);
s32 Em_ComputeSize(Em *em, s16 idx);
StageObj *StageObj_K1C_CreateOnEm(Em *em, u8 k, f32 scale);
/* unit's own declaration: with the definition's s32 (Em *em, s16 n, s16 max, s16 interval) (game/unk_708850.c), EmRathalos_Rest: 64 of 138 words differ, EmRathalos_Status2LieDown: 64 of 138 words differ */
s32 Em_RegenHp(Em *em, s32 n, s32 max, s32 interval);
void Em_ClearLowHpNeed(Em *em);
s32 Save_TestEventFlag(s32);
u16 Area_IdForCurrentTime(u16);
/*
 * The spots of Fatalis's sweeps and blasts: eight rows (row 0 empty), each a
 * list of spot numbers the effects take in turn (EmFatalis_Sweep: rows 1-5;
 * EmFatalis_Blasts: rows 6 and 7). It lies 0x1C bytes into D_008C9BB0
 * (config/ld/game_aliases.ld).
 */
extern s8 *EmFatalis_SweepSpots[];
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
void Em_ClearHitReactMode(Em *em);
void Em_SetStatus4Timer(Em *em);
void Em_SetStatus2Timer(Em *em);
void EmFatalis_RunState(Em *em, EmWork *work);
void EmFatalis_UpdateRageTimer(Em *em);
void Em_DecayNeed3(Em *em);
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
void Em_DecayNeed2(Em *em);
void Em_DecayNeed1Only(Em *em);
void Em_DecayNeed1(Em *em);
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

/*
 * The family's setup (Em_TypeSetup's entry for the Rathalos kinds): nothing
 * to do.
 */
void EmRathalos_Setup(Em *em)
{
}

/*
 * Creates the family's attached objects: a stage object of kind 0x13 (type
 * 0x22) and attached effect 0.
 */
void EmRathalos_SpawnEffects(void *em)
{
    StageObj_K13_Create(em, 0x22, 0);
    Effect01_Spawn((Player *) em, 0);
}

/*
 * Sets a Rathalos up: the scaled parts 3, 4 and 8; in the quest-less stage
 * (gQuest.stage 0) a fixed start position per area, 1500 apart per slot;
 * the idle motion, full hp from the size, the flight work's turn and roll
 * speeds; and, the first time, hides mesh 1 and creates the stage object of
 * kind 9.
 */
void EmRathalos_Init(Em *em)
{
    EmFlyerWork *w = (EmFlyerWork *)&em->work;
    s16 maxHp;

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
    em->stance = EM_STANCE_GROUND;
    w->fxMotion = 0;
    maxHp = Em_ComputeSize(em, 0);
    em->hp = maxHp;
    em->hpMax = maxHp;
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
        case EM_KIND_RATHIAN:
        case EM_KIND_GOLD_RATHIAN:
        case EM_KIND_PINK_RATHIAN:
        case EM_KIND_RATHALOS:
        case EM_KIND_SILVER_RATHALOS:
        case EM_KIND_AZURE_RATHALOS:
            em->meshShown[1] = 0;
            StageObj_K09_Create(em);
            break;
        }
    }
}

/*
 * Starts a Rathalos: Init, then in two story quests per pair (Rathalos
 * kinds in stages 0x434/0x436, Rathian kinds in 0x3F6/0x3F8) places it in a
 * far area until the quest's event flag is set; then the idle state.
 */
void EmRathalos_Start(Em *em)
{
    EmRathalos_Init(em);
    switch (gQuest.stage) {
    case 0x434:
    case 0x436:
        if (em->type == EM_KIND_RATHALOS || em->type == EM_KIND_SILVER_RATHALOS || em->type == EM_KIND_AZURE_RATHALOS) {
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
        if (em->type == EM_KIND_RATHIAN || em->type == EM_KIND_GOLD_RATHIAN || em->type == EM_KIND_PINK_RATHIAN) {
            if (Save_TestEventFlag(0x9C) == 0) {
                em->area = Area_IdForCurrentTime(0x71);
                em->posX = 14350.0f;
                em->posY = 0.0f;
                em->posZ = 9830.0f;
            }
        }
        break;
    }
    EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE, 0);
}

/* Sets the monster up again (Init) without changing its state. */
void EmRathalos_Reinit(Em *em)
{
    EmRathalos_Init(em);
}

/*
 * Ends a ground or air state: asks the script for the next one
 * (scriptPending) and goes back to RATH_IDLE (RATH_IDLE_ALERT when toAlert
 * is 1), or to the hover when in the air.
 */
void EmRathalos_EndState(Em *em, s16 toAlert, s32 unused)
{
    em->scriptPending = 1;
    if (em->stance == EM_STANCE_GROUND) {
        if (toAlert == 1) {
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE_ALERT, 0);
            return;
        }
        EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE, 0);
        return;
    }
    EmRathalos_ChangeState(em, EM_GROUP_FLY, RATH_FLY_HOVER, 0);
}

/*
 * Ends an air state with a short flight: hover once (next 0) or glide once
 * (next 1), the script picking what follows.
 */
void EmRathalos_EndAirState(Em *em, u8 next)
{
    s32 k;

    k = next & 0xFF;
    em->scriptPending = 1;
    em->motionSpeed = 1.0f;
    switch (k) {
    case 0:
        EmRathalos_ChangeState(em, EM_GROUP_FLY, RATH_FLY_HOVER_ONCE, 0);
        break;
    case 1:
        EmRathalos_ChangeState(em, EM_GROUP_FLY, RATH_FLY_GLIDE_ONCE, 0);
        break;
    }
}

/*
 * Restarts a motion layer's idle loop (layer 0, 1 or 2) once its motion has
 * ended.
 */
void EmRathalos_RestartLayerMotion(Em *em, s32 layer)
{
    if (em->motion[layer].playing == 0) {
        switch (layer) {
        case 0:
            Em_SetLayerMotion(em, EM_MOTION_ID(0, RATH_MOT_IDLE), 10, 0, 0);
            break;
        case 1:
            Em_SetLayerMotion(em, EM_MOTION_ID(1, RATH_MOT_IDLE), 10, 0, 1);
            break;
        case 2:
            Em_SetLayerMotion(em, EM_MOTION_ID(2, RATH_MOT_IDLE), 10, 0, 2);
            break;
        }
    }
}

/*
 * Puts a layer back to its idle loop when it plays one of the two layer
 * motions that end in it.
 */
void EmRathalos_LayerMotionToIdle(Em *em, s32 layer)
{
    switch (layer) {
    case 0:
        if (em->motionId[0] == EM_MOTION_ID(0, 0x1D)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(0, RATH_MOT_IDLE), 0xA, 0, 0);
        }
        if (em->motionId[0] == EM_MOTION_ID(0, RATH_MOT_ALERT_IDLE)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(0, RATH_MOT_IDLE), 0xA, 0, 0);
        }
        break;
    case 1:
        if (em->motionId[1] == EM_MOTION_ID(1, 0x1D)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(1, RATH_MOT_IDLE), 0xA, 0, 1);
        }
        if (em->motionId[1] == EM_MOTION_ID(1, RATH_MOT_ALERT_IDLE)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(1, RATH_MOT_IDLE), 0xA, 0, 1);
        }
        break;
    case 2:
        if (em->motionId[2] == EM_MOTION_ID(2, 0x1D)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(2, RATH_MOT_IDLE), 0xA, 0, 2);
        }
        if (em->motionId[2] == EM_MOTION_ID(2, RATH_MOT_ALERT_IDLE)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(2, RATH_MOT_IDLE), 0xA, 0, 2);
        }
        break;
    }
}

/*
 * RATH_IDLE_STAND: keeps the three layers on their idle loops until the
 * timer runs out, then asks the script for the next state.
 */
void EmRathalos_Stand(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        if (em->motionId[0] != EM_MOTION_ID(0, RATH_MOT_IDLE)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(0, RATH_MOT_IDLE), 0xA, 0, 0);
        }
        if (em->motionId[1] != EM_MOTION_ID(1, RATH_MOT_IDLE)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(1, RATH_MOT_IDLE), 0xA, 0, 1);
        }
        if (em->motionId[2] != EM_MOTION_ID(2, RATH_MOT_IDLE)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(2, RATH_MOT_IDLE), 0xA, 0, 2);
        }
        break;
    case 1:
        if (--em->timer <= 0) {
            em->scriptPending = 1;
        }
        break;
    }
}

/*
 * RATH_IDLE: plays motion 0x1D, then the idle loop, then ends the state;
 * the state every action falls back to on the ground.
 */
void EmRathalos_Idle(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x1D, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_IDLE, 0, 0);
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

/*
 * RATH_IDLE_ACTION_A: plays motion 0x17 once, the layers kept on their idle
 * loops.
 */
void EmRathalos_IdleActionA(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
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

/* RATH_IDLE_ACTION_B: motion 0x17 with a sound at a bone. */
void EmRathalos_IdleActionB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
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

/*
 * RATH_IDLE_ACTION_C: the idle motion once, the layers kept on their idle
 * loops.
 */
void EmRathalos_IdleActionC(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_IDLE, 0, 0);
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

/*
 * RATH_IDLE_ACTION_D: motion 0x18 with a sound, layers 0 and 2 on their
 * idle loops.
 */
void EmRathalos_IdleActionD(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x18, 0, 0);
        Em_SetLayerMotion(em, EM_MOTION_ID(0, RATH_MOT_IDLE), 0xA, 0, 0);
        Em_SetLayerMotion(em, EM_MOTION_ID(2, RATH_MOT_IDLE), 0xA, 0, 2);
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

/* RATH_IDLE_ACTION_E: motion 0x19 with a sound. */
void EmRathalos_IdleActionE(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
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

/* RATH_IDLE_ACTION_F: motion 0x1A with a sound. */
void EmRathalos_IdleActionF(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
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

/*
 * RATH_IDLE_ACTION_G: motion 0x1B with a sound, and attached effect 1 at
 * frame 2.
 */
void EmRathalos_IdleActionG(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
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

/* RATH_IDLE_PAIR_A: motion 0x50 or 0x51 (stateRand's bit 0) once. */
void EmRathalos_IdlePairA(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
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

/*
 * RATH_IDLE_TURN_TO_GOAL: turns in place to the goal, 0x200 a frame, with
 * RATH_MOT_TURN_IN_PLACE.
 */
void EmRathalos_IdleTurnToGoal(Em *em)
{
    u16 aimAngle;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_TURN_IN_PLACE, 0, 0);
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

/* RATH_IDLE_ACTION_H: motion 0x1C with a sound. */
void EmRathalos_IdleActionH(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
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

/* RATH_IDLE_ACTION_I: motion 0x1D once. */
void EmRathalos_IdleActionI(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
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

/* RATH_IDLE_ACTION_J: motion 0x14 with a sound. */
void EmRathalos_IdleActionJ(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
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

/*
 * RATH_IDLE_EAT_NEED2: lowers its head and feeds, gaining hp and refilling
 * need 2's gauge (Em_AddNeed2) while it feeds; makes a timed mining point at
 * the spot.
 */
void EmRathalos_EatNeed2(Em *em)
{
    u8 kind;

    Em_SetHitReactMode(em, 3U);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_HEAD_DOWN, 0, 0);
        Em_SelectSightRow(em, 1);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 56.0f, 0U) || Player_PassedFrameScaled(em, 108.0f, 0U) || Player_PassedFrameScaled(em, 126.0f, 0U)
            || Player_PassedFrameScaled(em, 138.0f, 0U) || Player_PassedFrameScaled(em, 170.0f, 0U)) {
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
        if ((em->motion[0].playing == 0) && ((em->step += 1, Em_SetMotion(em, RATH_MOT_FEED, 0, 0), Em_AddHp(em, (s32) (0.05f * (f32) em->hpMax)), kind = em->type, (kind == EM_KIND_RATHIAN)) || (kind == EM_KIND_GOLD_RATHIAN) || (kind == EM_KIND_PINK_RATHIAN))) {
            EmMine_CreateTimed0(em, &em->posX);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SelectSightRow(em, 0);
            Em_LoadReactionSet(em, 0);
            Em_ClearNeed2State(em);
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
    Em_AddNeed2(em, 0xDE);
}

/* RATH_IDLE_PAIR_B: motion 0x50, the idle motion, 0x51, the idle motion. */
void EmRathalos_IdlePairB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x50, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_IDLE, 0, 0);
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
            Em_SetMotion(em, RATH_MOT_IDLE, 0, 0);
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

/* RATH_IDLE_PAIR_C: as RATH_IDLE_PAIR_B with 0x51 first. */
void EmRathalos_IdlePairC(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x51, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_IDLE, 0, 0);
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
            Em_SetMotion(em, RATH_MOT_IDLE, 0, 0);
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

/*
 * RATH_IDLE_ALERT: the alert idle (RATH_MOT_ALERT_IDLE) while it fights the
 * hunters.
 */
void EmRathalos_AlertIdle(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_ALERT_IDLE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * RATH_IDLE_REST: lies down and rests, regaining hp (Em_RegenHp) until
 * Em_UpdateDamage's result 10 ends it (RATH_IDLE_REST_END); a hit wakes it
 * too.
 */
void EmRathalos_Rest(Em *em, EmWork *work)
{
    Em_SetHitReactMode(em, 2U);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_LIE_DOWN, 0, 0);
        Em_LoadReactionSet(em, 1);
        if (em->type == EM_KIND_RATHIAN || em->type == EM_KIND_GOLD_RATHIAN || em->type == EM_KIND_PINK_RATHIAN) {
            em->timer = 9000;
        } else {
            em->timer = 7200;
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_LIE, 0, 0);
        }
        break;
    case 2:
        if (em->type == EM_KIND_RATHIAN || em->type == EM_KIND_GOLD_RATHIAN || em->type == EM_KIND_PINK_RATHIAN) {
            Em_RegenHp(em, 1, (s32) (0.6f * (f32) em->hpMax), 3);
        } else {
            Em_RegenHp(em, 1, (s32) (0.7f * (f32) em->hpMax), 3);
        }
        if (--em->timer <= 0) {
            em->step++;
            Em_ClearLowHpNeed(em);
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE_REST_END, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE_REST_END, 4);
        }
        break;
    }
}

/* RATH_IDLE_EAT_START: lowers its head, then goes on with RATH_IDLE_EAT. */
void EmRathalos_EatStart(Em *em)
{
    Em_SetHitReactMode(em, 3U);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_HEAD_DOWN, 0, 0);
        Em_SelectSightRow(em, 1);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 200.0f, 0U) != 0) {
            em->step++;
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE_EAT, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE_EAT, 4);
        }
        break;
    }
}

/* RATH_IDLE_EAT: feeds once (RATH_MOT_FEED) and ends the state. */
void EmRathalos_Eat(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SelectSightRow(em, 0);
        Em_LoadReactionSet(em, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_FEED, 0, 0);
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

/*
 * RATH_IDLE_STATUS2_COLLAPSE: status 2 began: falls over and lies until it
 * ends (RATH_IDLE_STATUS2_END).
 */
void EmRathalos_Status2Collapse(Em *em)
{
    Em_SetHitReactMode(em, 2U);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_COLLAPSE, 0, 0);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_LIE, 0, 0);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE_STATUS2_END, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE_STATUS2_END, 4);
        }
        break;
    }
}

/*
 * RATH_IDLE_STATUS2_LIE_DOWN: status 2 began: lies down and lies, regaining
 * hp, until it ends.
 */
void EmRathalos_Status2LieDown(Em *em, EmWork *work)
{
    Em_SetHitReactMode(em, 2U);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_LIE_DOWN, 0, 0);
        Em_LoadReactionSet(em, 1);
        if (em->type == EM_KIND_RATHIAN || em->type == EM_KIND_GOLD_RATHIAN || em->type == EM_KIND_PINK_RATHIAN) {
            em->timer = 12600;
        } else {
            em->timer = 9000;
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_LIE, 0, 0);
        }
        break;
    case 2:
        if (em->type == EM_KIND_RATHIAN || em->type == EM_KIND_GOLD_RATHIAN || em->type == EM_KIND_PINK_RATHIAN) {
            Em_RegenHp(em, 1, (s32) (0.6f * (f32) em->hpMax), 3);
        } else {
            Em_RegenHp(em, 1, (s32) (0.7f * (f32) em->hpMax), 3);
        }
        if (--em->timer <= 0) {
            em->step++;
            Em_ClearLowHpNeed(em);
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE_STATUS2_END, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE_STATUS2_END, 4);
        }
        break;
    }
}

/* RATH_IDLE_ACTION_K: motion 0x50 once. */
void EmRathalos_IdleActionK(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
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

/*
 * RATH_IDLE_REST_END: gets up after resting (a timed mining point appears
 * where it lay), then shakes off.
 */
void EmRathalos_RestEnd(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, RATH_MOT_GET_UP, 0, 0);
        Em_LoadReactionSet(em, 0);
        em->blind = 2;
        if (em->type == EM_KIND_RATHALOS || em->type == EM_KIND_SILVER_RATHALOS || em->type == EM_KIND_AZURE_RATHALOS) {
            EmMine_CreateTimed0(em, &em->posX);
        }
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE_SHAKE_OFF, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE_SHAKE_OFF, 4);
        }
        break;
    }
}

/* RATH_IDLE_STATUS2_END: gets up after status 2, then shakes off. */
void EmRathalos_Status2End(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, RATH_MOT_GET_UP, 0, 0);
        Em_LoadReactionSet(em, 0);
        em->blind = 2;
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE_SHAKE_OFF, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE_SHAKE_OFF, 4);
        }
        break;
    }
}

/* RATH_IDLE_SHAKE_OFF: shakes off (RATH_MOT_SHAKE_OFF) and ends the state. */
void EmRathalos_ShakeOff(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, RATH_MOT_SHAKE_OFF, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * RATH_IDLE_EAT_NEED1: lowers its head and feeds, gaining hp and refilling
 * need 1's gauge (Em_AddNeed1).
 */
void EmRathalos_EatNeed1(Em *em)
{
    Em_SetHitReactMode(em, 3U);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_HEAD_DOWN, 0, 0);
        Em_SelectSightRow(em, 1);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_FEED, 0, 0);
            Em_AddHp(em, (s32) (0.05f * (f32) em->hpMax));
            Em_AddNeed1(em, em->need1Max);
            Em_ClearNeed1State(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SelectSightRow(em, 0);
            Em_LoadReactionSet(em, 0);
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * RATH_IDLE_STATUS1_COLLAPSE: status 1 began: falls over and lies until it
 * ends (RATH_IDLE_STATUS1_END).
 */
void EmRathalos_Status1Collapse(Em *em)
{
    Em_SetHitReactMode(em, 2U);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_COLLAPSE, 0, 0);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_LIE, 0, 0);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE_STATUS1_END, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE_STATUS1_END, 4);
        }
        break;
    }
}

/* RATH_IDLE_STATUS1_END: gets up after status 1, then shakes off. */
void EmRathalos_Status1End(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, RATH_MOT_GET_UP, 0, 0);
        Em_LoadReactionSet(em, 0);
        em->blind = 2;
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE_SHAKE_OFF, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE_SHAKE_OFF, 4);
        }
        break;
    }
}

/*
 * RATH_IDLE_STATUS2_DOWNED: status 2 in a hold: lies downed, with the
 * periodic effect at bone 0x22, until it ends.
 */
void EmRathalos_Status2Downed(Em *em, EmWork *work)
{
    f32 pos[3];

    em->holdTimer = 5;
    Em_SetHitReactMode(em, 2);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_DOWNED, 0, 0);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_STATUS2_DOWNED_END, 3);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_STATUS2_DOWNED_END, 3);
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

/* RATH_IDLE_STATUS1_DOWNED: status 1 in a hold, as RATH_IDLE_STATUS2_DOWNED. */
void EmRathalos_Status1Downed(Em *em, EmWork *work)
{
    f32 pos[3];

    em->holdTimer = 5;
    Em_SetHitReactMode(em, 2);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_DOWNED, 0, 0);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_STATUS1_DOWNED_END, 3);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_STATUS1_DOWNED_END, 3);
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

/*
 * The shake-off after status 4, the stun or the second hold (variant 0, 1,
 * 2: RATH_IDLE_STATUS4_SHAKE_OFF to RATH_IDLE_HOLD2_SHAKE_OFF).
 */
void EmRathalos_ShakeOffAfterStatus(Em *em, EmWork *work, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, RATH_MOT_SHAKE_OFF, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

/* RATH_MOVE_WALK: walks to the goal (Em_StepToGoal) and ends there. */
void EmRathalos_Walk(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_WALK, 0, 0);
        break;
    case 1:
        if (Em_StepToGoal(em, 0x40, 0) == 1) {
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

/* RATH_MOVE_END: ends the state. */
void EmRathalos_MoveEnd(Em *em, EmWork *work)
{
    EmRathalos_EndState(em, 0, 0);
}

/* RATH_MOVE_END2: ends the state. */
void EmRathalos_MoveEnd2(Em *em, EmWork *work)
{
    EmRathalos_EndState(em, 0, 0);
}

/*
 * RATH_MOVE_TURN: turns to the target with the family's turn parameters
 * (Em_StartTurnToTarget, Em_TurnStep).
 */
void EmRathalos_Turn(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
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

/* RATH_MOVE_RUN (and RATH_MOVE_RUN_SLOW): runs to the goal. */
void EmRathalos_Run(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_RUN, 0, 0);
        break;
    case 1:
        if (Em_StepToGoal(em, 0x40, 0) == 1) {
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

/* RATH_MOVE_TURN_IN_COMBAT: the turn to the target while it fights. */
void EmRathalos_TurnInCombat(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
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

/* RATH_MOVE_LIMP: limps to the goal (low hp). */
void EmRathalos_Limp(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_LIMP, 0, 0);
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

/*
 * RATH_MOVE_TURN_TO_GOAL (and the two rage turns): turns in place to the
 * goal, 0x200 a frame, then ends the state toward the alert idle.
 */
void EmRathalos_TurnToGoal(Em *em, EmWork *work)
{
    u16 aimAngle;
    u32 diff;
    s32 rotY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_TURN_IN_PLACE, 0, 0);
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

/*
 * RATH_FLY_TAKE_OFF: takes off (airborne from frame 38) along the family's
 * take-off curve (0xF for the Rathalos kinds, 2 for the Rathian kinds),
 * then picks a short flight.
 */
void EmRathalos_TakeOff(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_TAKE_OFF, 0, 0);
        if (em->type == EM_KIND_RATHALOS || em->type == EM_KIND_SILVER_RATHALOS || em->type == EM_KIND_AZURE_RATHALOS) {
            EmRathalos_StartCurves(em, 0xF);
        } else {
            EmRathalos_StartCurves(em, 2);
        }
        work->flyGuard = 0;
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 38.0f, 0U) != 0) {
            em->step++;
            em->stance = EM_STANCE_AIR;
            EmRathalos_UpdateCurves(em);
        }
        break;
    case 2:
        if (EmRathalos_UpdateCurves(em)) {
            em->step++;
            EmRathalos_EndAirState(em, 0U);
        }
        break;
    }
}

/*
 * RATH_FLY_LAND: descends from the hover (velY eased toward the floor),
 * then lands with a camera shake.
 */
void EmRathalos_Land(Em *em, EmWork *work)
{
    f32 dropLeft;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        em->rotX = 0;
        em->rotZ = 0;
        Em_SetMotion(em, RATH_MOT_HOVER, 0, 0);
        Player_ClearVelocity(em);
        em->velY = -10.0f;
        dropLeft = em->floorY;
        dropLeft = em->posY - (1000.0f + dropLeft);
        if (dropLeft > 0.0f) {
            do {
                em->velY -= 0.75f;
                dropLeft += em->velY;
            } while (dropLeft > 0.0f && !(em->velY < -200.0f));
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
            Em_SetMotion(em, RATH_MOT_DESCEND, 0, 0);
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
            Em_SetMotion(em, RATH_MOT_TOUCH_DOWN, 0, 0);
            em->posY = em->floorY;
            em->rotX = 0;
            em->rotZ = 0;
            em->stance = EM_STANCE_GROUND;
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
    dropLeft = em->floorY;
    if (em->posY < dropLeft) {
        em->posY = dropLeft;
    }
}

/*
 * RATH_FLY_HOVER: hovers, turning to the goal, until the timer runs out,
 * then lands; at timer 300 it asks for a short flight instead.
 */
void EmRathalos_Hover(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        work->turnSpeed = 0x100;
        Em_SetMotion(em, RATH_MOT_HOVER, 0, 0);
        work->flyGuard = 0;
        break;
    case 1:
        EmRathalos_MoveByVelYCurve(em, 1U);
        if (--em->timer <= 0) {
            em->step++;
            EmRathalos_ChangeState(em, EM_GROUP_FLY, RATH_FLY_LAND, 1);
        }
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 1);
        if (em->timer == 0x12C) {
            EmRathalos_EndAirState(em, 0U);
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

/*
 * RATH_FLY_HOP_TO_GOAL: takes off turning to the goal, flies the curve, and
 * lands; the landing keeps fxMotion's after-landing count (150).
 */
void EmRathalos_HopToGoal(Em *em, EmWork *work)
{
    f32 velY;
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_TAKE_OFF, 0, 0);
        Player_ClearVelocity(em);
        if (em->type == EM_KIND_RATHALOS || em->type == EM_KIND_SILVER_RATHALOS || em->type == EM_KIND_AZURE_RATHALOS) {
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
            em->stance = EM_STANCE_AIR;
            EmRathalos_UpdateCurves(em);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
        }
        break;
    case 2:
        if ((EmRathalos_UpdateCurves(em) & 0xFF) && (em->posY <= (800.0f + em->floorY))) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_DESCEND, 0, 0);
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
            Em_SetMotion(em, RATH_MOT_TOUCH_DOWN, 0, 0);
            em->stance = EM_STANCE_GROUND;
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

/*
 * RATH_FLY_HOP_FORWARD: a short hop forward (velZ 20) along curve 3,
 * landing again.
 */
void EmRathalos_HopForward(Em *em, EmWork *work)
{
    f32 floorY;
    s32 ended;

    ended = 0;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_TAKE_OFF, 0, 0);
        EmRathalos_StartCurves(em, 3);
        em->velZ = 20.0f;
        work->flyGuard = 0;
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 30.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            ended = EmRathalos_UpdateCurves(em) & 0xFF;
        }
        if ((ended != 0) && (em->posY <= em->floorY)) {
            Em_SetMotion(em, RATH_MOT_TOUCH_DOWN, 0, 0);
            em->step++;
            em->stance = EM_STANCE_GROUND;
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

/* RATH_FLY_HOP_BACK: a short hop backward (velZ -20). */
void EmRathalos_HopBack(Em *em, EmWork *work)
{
    f32 floorY;
    s32 ended;

    ended = 0;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_TAKE_OFF, 0, 0);
        EmRathalos_StartCurves(em, 3);
        em->velZ = -20.0f;
        work->flyGuard = 0;
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 30.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            ended = EmRathalos_UpdateCurves(em) & 0xFF;
        }
        if ((ended != 0) && (em->posY <= em->floorY)) {
            Em_SetMotion(em, RATH_MOT_TOUCH_DOWN, 0, 0);
            em->step++;
            em->stance = EM_STANCE_GROUND;
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

/*
 * RATH_FLY_ROUTE: flies through the goal route (legsLeft legs, the goal
 * number stepping 0-3), then glides once.
 */
void EmRathalos_FlyRoute(Em *em, EmWork *work)
{
    f32 floorY;
    s32 timerLeft;
    u8 n;
    u8 stateVar1;

    switch (em->step) {
    case 0:
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, RATH_MOT_GLIDE, 0, 0);
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
            Em_SetMotion(em, RATH_MOT_FLY, 0, 0);
        }
        if ((Vec_DistanceXZ(&em->posX, em->goal) <= 3000.0f) || (timerLeft = em->timer - 1, em->timer = timerLeft, (timerLeft <= 0))) {
            n = ((EmFlyerWork *)work)->legsLeft - 1;
            ((EmFlyerWork *)work)->legsLeft = n;
            if ((n & 0xFF) <= 0) {
                EmRathalos_EndAirState(em, 1U);
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

/*
 * RATH_FLY_HOVER_TO_GOAL: hovers toward the goal, pitching at it, until it
 * is near or the timer runs out.
 */
void EmRathalos_HoverToGoal(Em *em, EmWork *work)
{
    f32 dist;
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        work->turnSpeed = 0x100;
        Em_SetMotion(em, RATH_MOT_HOVER, 0, 0);
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
        if (--em->timer <= 0 || dist <= 10.0f * em->velZ) {
            em->step++;
            Player_ClearVelocity(em);
            EmRathalos_EndAirState(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void Hud_TriggerGlow(Em *em);
/*
 * RATH_FLY_OUT_OF_AREA: rises, flies to the area's exit point and out; then
 * flies to the goal in the next area (RATH_FLY_INTO_AREA) or, when it
 * stays, to a landing point (RATH_FLY_TO_GOAL_AND_LAND).
 */
void EmRathalos_FlyOutOfArea(Em *em, EmWork *work)
{
    u16 next;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, RATH_MOT_HOVER, 0, 0);
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
            em->stance = EM_STANCE_AIR;
            em->lookAtTarget = 0;
            Player_ClearVelocity(em);
            em->velZ = 50.0f;
            Em_SetMotion(em, RATH_MOT_GLIDE, 0, 0);
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
            EmRathalos_ChangeState(em, EM_GROUP_FLY, RATH_FLY_TO_GOAL_AND_LAND, 1);
        } else {
            if (em->remote == 0) {
                em->nextArea = next;
                em->goalNo = next;
                em->goalKind = 3;
                Em_ComputeGoal(em, em->goal);
            }
            EmRathalos_ChangeState(em, EM_GROUP_FLY, RATH_FLY_INTO_AREA, 1);
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

/* RATH_FLY_TO_GOAL_AND_LAND: flies to the goal, then lands. */
void EmRathalos_FlyToGoalAndLand(Em *em, EmWork *work)
{
    f32 goalDist;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        work->turnSpeed = 0x100;
        Em_SetMotion(em, RATH_MOT_HOVER, 0, 0);
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
        goalDist = Vec_DistanceXZ(&em->posX, em->goal);
        em->timer--;
        if (goalDist <= 1000.0f || em->timer < 0) {
            em->step++;
            em->timer = 600;
            EmRathalos_ChangeState(em, EM_GROUP_FLY, RATH_FLY_LAND, 1);
        }
        break;
    case 2:
        if (em->remote == 0) {
            em->timer = 0x258;
            EmRathalos_ChangeState(em, EM_GROUP_FLY, RATH_FLY_LAND, 1);
        }
        break;
    }
    if (em->posY < em->floorY) {
        em->posY = em->floorY;
    }
}

/*
 * RATH_FLY_OUT_OF_AREA_ALT: as RATH_FLY_OUT_OF_AREA, but it moves between
 * areas 0xF and 0x13 directly.
 */
void EmRathalos_FlyOutOfAreaAlt(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        work->turnSpeed = 0x100;
        Em_SetMotion(em, RATH_MOT_HOVER, 0, 0);
        em->stance = EM_STANCE_AIR;
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
        Em_SetMotion(em, RATH_MOT_GLIDE, 0, 0);
        
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
        EmRathalos_EndAirState(em, 1U);
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/* RATH_FLY_GLIDE_TO_GOAL: glides to the goal (at most velZ 100), then lands. */
void EmRathalos_GlideToGoal(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_GLIDE, 0, 0);
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
            EmRathalos_ChangeState(em, EM_GROUP_FLY, RATH_FLY_LAND, 1);
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
                EmRathalos_ChangeState(em, EM_GROUP_FLY, RATH_FLY_LAND, 1);
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
                EmRathalos_ChangeState(em, EM_GROUP_FLY, RATH_FLY_LAND, 1);
            }
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * RATH_FLY_BACK_HOP: hops backward (airborne from frame 76) and lands with
 * ground effects and a camera shake.
 */
void EmRathalos_BackHop(Em *em)
{
    f32 floorY;
    s32 ended;

    ended = 0;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_BACK_HOP, 0, 0);
        EmRathalos_StartCurves(em, 0xE);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 76.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            ended = EmRathalos_UpdateCurves(em) & 0xFF;
        }
        if ((ended != 0) && (em->posY <= em->floorY)) {
            Em_SetMotion(em, RATH_MOT_BACK_HOP_LAND, 0, 0);
            em->step++;
            em->stance = EM_STANCE_GROUND;
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

/*
 * RATH_FLY_INTO_AREA: flies into the new area toward its entry point (or
 * its middle), then to the exit point and on, as RATH_FLY_OUT_OF_AREA.
 */
void EmRathalos_FlyIntoArea(Em *em, EmWork *work)
{
    AreaRecord *area;
    f32 *entryPoint;
    u16 next;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        work->turnSpeed = 0x100;
        Em_SetMotion(em, RATH_MOT_HOVER, 0, 0);
        Player_ClearVelocity(em);
        em->velZ = 80.0f;
        area = Stage_GetAreaRecord(em->area);
        em->goal[1] = area->bottom;
        entryPoint = Em_AreaEntryValue(em, *em->tables);
        if (entryPoint == NULL) {
            em->goal[0] = area->sizeX / 2.0f;
            em->goal[2] = area->sizeZ / 2.0f;
        } else {
            em->goal[0] = entryPoint[0];
            em->goal[2] = entryPoint[2];
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
            EmRathalos_ChangeState(em, EM_GROUP_FLY, RATH_FLY_TO_GOAL_AND_LAND, 1);
        } else {
            if (em->remote == 0) {
                em->nextArea = next;
                em->goalNo = next;
                em->goalKind = 3;
                Em_ComputeGoal(em, em->goal);
            }
            EmRathalos_ChangeState(em, EM_GROUP_FLY, RATH_FLY_INTO_AREA, 1);
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

/* RATH_FLY_HOVER_ONCE: hovers for one motion, then picks the next air state. */
void EmRathalos_HoverOnce(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, RATH_MOT_HOVER, 0, 0);
        work->flyGuard = 0;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmRathalos_EndAirState(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/* RATH_FLY_GLIDE_ONCE: glides for one motion, then picks the next air state. */
void EmRathalos_GlideOnce(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, RATH_MOT_GLIDE, 0, 0);
        Player_ClearAccel2(em);
        em->velY = 0.0f;
        em->velX = 0.0f;
        work->flyGuard = 1;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmRathalos_EndAirState(em, 1U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/* RATH_FLY_HOVER_TURN: hovers turning to the goal (150 frames at most). */
void EmRathalos_HoverTurn(Em *em, EmWork *work)
{
    f32 floorY;
    s32 done;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, RATH_MOT_HOVER, 0, 0);
        work->turnSpeed = 0x180;
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        work->flyGuard = 0;
        em->timer = 0x96;
        break;
    case 1:
        EmRathalos_MoveByVelYCurve(em, 1U);
        done = Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 1) & 0xFF;
        if (--em->timer <= 0 || done != 0) {
            em->step++;
            EmRathalos_EndAirState(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * RATH_FLY_ROUTE_B: flies the goal route as RATH_FLY_ROUTE, keeping a glide
 * already running.
 */
void EmRathalos_FlyRouteB(Em *em, EmWork *work)
{
    f32 floorY;
    u16 motionId;
    u8 stateVar1;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        em->velX = 0.0f;
        em->velY = 0.0f;
        Player_ClearAccel2(em);
        if (em->velZ < 50.0f) {
            em->stateVar1 = 0;
            em->velZ = 50.0f;
            Em_SetMotion(em, RATH_MOT_GLIDE, 0, 0);
        } else {
            em->stateVar1 = 1;
            motionId = em->motionId[0];
            if ((motionId != EM_MOTION_ID(0, RATH_MOT_GLIDE)) && (motionId != EM_MOTION_ID(0, RATH_MOT_FLY))) {
                Em_SetMotion(em, RATH_MOT_GLIDE, 0, 0);
            }
        }
        work->flyGuard = 1;
        break;
    case 1:
        stateVar1 = em->stateVar1;
        if ((stateVar1 == 0) && (em->motion[0].playing == 0)) {
            em->stateVar1 = stateVar1 + 1;
            Em_SetMotion(em, RATH_MOT_FLY, 0, 0);
        }
        if (Vec_DistanceXZ(&em->posX, em->goal) <= 1000.0f || --em->timer <= 0) {
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

/*
 * RATH_FLY_LEVEL_OUT_AND_LAND: levels pitch and roll, hovers for 300
 * frames, then lands.
 */
void EmRathalos_LevelOutAndLand(Em *em, EmWork *work)
{
    f32 floorY;
    s32 rotX;
    s32 rotZ;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_LEVEL_OUT, 0, 0);
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
            Em_SetMotion(em, RATH_MOT_HOVER, 0, 0);
            em->timer = 0x12C;
            EmRathalos_ChangeState(em, EM_GROUP_FLY, RATH_FLY_LAND, 1);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->timer = 0x12C;
            Em_SetMotion(em, RATH_MOT_HOVER, 0, 0);
            if (em->remote == 0) {
                EmRathalos_ChangeState(em, EM_GROUP_FLY, RATH_FLY_LAND, 1);
            }
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * RATH_FLY_GLIDE_TO_GOAL_B: glides to the goal with the second flight
 * parameters (EmRathalos_FlyParam2), then glides once.
 */
void EmRathalos_GlideToGoalB(Em *em, EmWork *work)
{
    f32 floorY;
    u16 motionId;
    u8 stateVar1;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        em->velX = 0.0f;
        em->velY = 0.0f;
        Player_ClearAccel2(em);
        if (em->velZ < 50.0f) {
            em->stateVar1 = 0;
            em->velZ = 50.0f;
            Em_SetMotion(em, RATH_MOT_GLIDE, 0, 0);
        } else {
            em->stateVar1 = 1;
            motionId = em->motionId[0];
            if ((motionId != EM_MOTION_ID(0, RATH_MOT_GLIDE)) && (motionId != EM_MOTION_ID(0, RATH_MOT_FLY))) {
                Em_SetMotion(em, RATH_MOT_GLIDE, 0, 0);
            }
        }
        work->flyGuard = 1;
        break;
    case 1:
        stateVar1 = em->stateVar1;
        if ((stateVar1 == 0) && (em->motion[0].playing == 0)) {
            em->stateVar1 = stateVar1 + 1;
            Em_SetMotion(em, RATH_MOT_FLY, 0, 0);
        }
        if (Vec_DistanceXZ(&em->posX, em->goal) <= 1000.0f || --em->timer <= 0) {
            em->step++;
            EmRathalos_EndAirState(em, 1U);
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

/* RATH_FLY_END: ends the state. */
void EmRathalos_EndFlight(Em *em)
{
    EmRathalos_EndState(em, 0, 0);
}

/*
 * RATH_FLY_HOVER_STRAIGHT_TO_GOAL: hovers straight at the goal in three
 * dimensions.
 */
void EmRathalos_HoverStraightToGoal(Em *em, EmWork *work)
{
    f32 dist;
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        work->turnSpeed = 0x100;
        Em_SetMotion(em, RATH_MOT_HOVER, 0, 0);
        work->flyGuard = 0;
        break;
    case 1:
        work->moveAngY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        work->moveAngZ = 0;
        Em_GetPitchToGoal(em, &work->moveAngX, 0.0f, 0.0f);
        Em_MoveRotated(em, &work->moveAngX);
        dist = Vec_Distance((f32 *) &em->posX, (f32 *) em->goal);
        if (--em->timer <= 0 || dist <= 10.0f * em->velZ) {
            em->step++;
            Player_ClearVelocity(em);
            EmRathalos_EndAirState(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * RATH_FLY_LEVEL_OUT: levels pitch and roll along curve 9, then picks the
 * next air state.
 */
void EmRathalos_LevelOut(Em *em, EmWork *work)
{
    f32 floorY;
    s32 rotX;
    s32 rotZ;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_LEVEL_OUT, 0, 0);
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
            EmRathalos_EndAirState(em, 0U);
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

/*
 * RATH_FLY_ESCAPE_UPWARD: breaks out of a trap: takes off (holdTimer
 * cleared, no reactions), then lands.
 */
void EmRathalos_EscapeUpward(Em *em, EmWork *work)
{
    f32 floorY;
    f32 velY;
    f32 y;

    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, RATH_MOT_TAKE_OFF, 0xA, 0x1E);
        EmRathalos_StartCurves(em, 2);
        em->stance = EM_STANCE_AIR;
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
            Em_SetMotion(em, RATH_MOT_DESCEND, 0, 0);
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
            Em_SetMotion(em, RATH_MOT_TOUCH_DOWN, 0, 0);
            em->stance = EM_STANCE_GROUND;
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

/* RATH_FLY_HOVER_RISE: hovers upward to 1000 above the goal. */
void EmRathalos_HoverRise(Em *em, EmWork *work)
{
    f32 pos[3];
    f32 floorY;
    s32 timer;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, RATH_MOT_HOVER, 0, 0);
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
            EmRathalos_EndAirState(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/* RATH_ATK_TURN_AND_STARE: turns to the target, then plays motion 0x24. */
void EmRathalos_TurnAndStare(Em *em)
{
    switch (em->step) {
    case 0:
        if (Em_TurnToTargetMotion(em, 0xE38, 3, 6, 5) != 0) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
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

/*
 * RATH_ATK_LEAP: turns to the target, leaps at it along curve 5 (goalDist),
 * and lands with ground effects near the end.
 */
void EmRathalos_Leap(Em *em, EmWork *work)
{
    f32 floorY;
    u8 stateVar0;

    switch (em->step) {
    case 0:
        stateVar0 = em->stateVar0;
        switch (stateVar0) {
        case 0:
            em->stateVar0 = stateVar0 + 1;
            em->stance = EM_STANCE_GROUND;
            
        case 1:
            if (Em_TurnToTargetMotion(em, 0xE38, 3, 6, 5) != 0) {
                em->lookAtTarget = 0;
                em->step++;
                Em_SetMotion(em, RATH_MOT_LEAP, 0, 0);
                EmRathalos_StartCurves(em, 5);
            }
        }
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 60.0f) != 0) {
            em->stance = EM_STANCE_AIR;
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
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            Em_SetMotion(em, RATH_MOT_LEAP_LAND, 0, 0);
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

/* RATH_ATK_STARE: motion 0x24 with the body bent toward the target. */
void EmRathalos_Stare(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
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

/*
 * RATH_ATK_FIREBALL (and RATH_ATK_FIREBALL_LOW and _HIGH): turns to the
 * target and breathes a fireball (two projectiles from bone 0x24 at frame
 * 78), countdown times.
 */
void EmRathalos_Fireball(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        break;
    case 1:
        if (Em_TurnToTargetMotion(em, 0xE38, 3, 6, 5) != 0) {
            em->step++;
            em->lookAtTarget = 0;
            Em_SetMotion(em, RATH_MOT_FIREBALL, 0, 0);
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

/* RATH_ATK_ROAR: roars at the target. */
void EmRathalos_Roar(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_ROAR, 0, 0);
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

/* RATH_ATK_TAIL_SPIN: spins in place twice (rotY - 0x200 a frame). */
void EmRathalos_TailSpin(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_TAIL_SPIN, 0, 0);
        Motion_InitTime(em, 0);
        break;
    case 1:
        em->rotY -= 0x200;
        if (em->motion[1].playing == 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_TAIL_SPIN, 0, 0);
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

/*
 * RATH_ATK_LIE_AND_RISE: turns to the target, plays RATH_MOT_LIE and at
 * frame 30 motion 0x30.
 */
void EmRathalos_LieAndRise(Em *em)
{
    u8 stateVar0;

    switch (em->step) {
    case 0:
        stateVar0 = em->stateVar0;
        switch (stateVar0) {
        case 0:
            em->stateVar0 = stateVar0 + 1;
            em->stance = EM_STANCE_GROUND;
            em->lookAtTarget = 0;
            
        case 1:
            if (Em_TurnToTargetMotion(em, 0xE38, 3, 6, 5) != 0) {
                em->step++;
                em->lookAtTarget = 0;
                Em_SetMotion(em, RATH_MOT_LIE, 0, 0);
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

/*
 * RATH_ATK_SWOOP_THEN_LEAVE: flies at the target and swoops on it
 * (RATH_MOT_SWOOP_DIVE, RATH_MOT_SWOOP, the claw), pulling up from walls,
 * then climbs away to the escape point and ends the swoop.
 */
void EmRathalos_SwoopThenLeave(Em *em, EmWork *work)
{
    f32 targetWorldPos[3];
    f32 pos[3];
    f32 targetPosB[3];
    AreaRecord *area;
    Player *player;

    area = Stage_GetAreaRecord(em->area);
    if (work->wallTimer != 0) {
        if (--work->wallTimer <= 0) {
            work->wallTimer = 0;
        }
    }
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
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
        player = &gPlayers[em->targetNo];
        if ((Obj_SameArea(em, player) & 0xFF) == 0 && player->active != 0) {
            EmRathalos_EndSwoop(em, (s32) work);
            break;
        }
        Em_GetTargetPos(em, em->targetNo, pos);
        Area_ToWorld(player->area, pos, targetWorldPos);
        em->aimAngle = Math_AngleXZ(em->mapPos, targetWorldPos);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 0xD1, &EmRathalos_FlyParam);
        if (em->aimAngle <= 0x800 || em->aimAngle >= 0xF800) {
            if (!(4000.0f < Vec_DistanceXZ(&em->posX, pos))) {
                EmRathalos_EndSwoop(em, (s32) work);
            } else {
                em->step++;
                work->turnSpeed = 0x100;
                Em_SetMotion(em, RATH_MOT_SWOOP_DIVE, 0, 0);
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
            Em_SetMotion(em, RATH_MOT_SWOOP, 0, 0);
            em->velZ = 100.0f;
            em->timer = 600;
        }
        player = &gPlayers[em->targetNo];
        if ((Obj_SameArea(em, player) & 0xFF) == 0 && player->active != 0) {
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
        if (em->motionId[0] == EM_MOTION_ID(0, RATH_MOT_SWOOP_CLAW)) {
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
        if (em->motionId[0] == EM_MOTION_ID(0, RATH_MOT_SWOOP_CLAW) && em->motion[0].playing == 0) {
            em->step = 6;
            Em_SetMotion(em, RATH_MOT_FLY, 0, 0);
            EmRathalos_SetEscapePoint(em);
        }
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 2);
        if (work->wallTimer == 0) {
        EmRathalos_StepPitch(em, 2, 100.0f);
        if (em->motionId[0] == EM_MOTION_ID(0, RATH_MOT_SWOOP_CLAW) && EmMotion_ReachedFrame(em, 0, 76.0f)) {
            Em_GetPitchToTarget(em, &work->moveAngX, 300.0f, 0.0f);
        } else if (em->targetNo != -1) {
            Em_GetTargetPos(em, em->targetNo, targetPosB);
            if (!(200.0f + targetPosB[1] < em->posY)) {
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

/*
 * RATH_ATK_AIR_FIREBALL: rises in the hover, breathes a fireball down
 * (frame 6 of RATH_MOT_AIR_FIREBALL), and hovers again.
 */
void EmRathalos_AirFireball(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_AIR_FIREBALL_RISE, 0, 0);
        EmRathalos_StartCurves(em, 0xD);
        break;
    case 1:
        Em_PlaySeAtBoneOnFrame(em, 0x78, 0x20, 0x23);
        Em_TurnToGoal(em, (s32) ((EmFlyerWork *)work)->turnSpeed, 2);
        EmRathalos_UpdateCurves(em);
        if (Player_PassedFrameScaled(em, 60.0f, 0U) != 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_AIR_FIREBALL, 0, 0);
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
            Em_SetMotion(em, RATH_MOT_HOVER, 2, 0x22);
        }
        break;
    case 3:
        EmRathalos_MoveByVelYCurve(em, 1U);
        if (em->motion[0].playing == 0) {
            em->step++;
            EmRathalos_EndAirState(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * RATH_ATK_AIR_FIREBALL_B: as RATH_ATK_AIR_FIREBALL with the other flash
 * effect.
 */
void EmRathalos_AirFireballB(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_AIR_FIREBALL_RISE, 0, 0);
        EmRathalos_StartCurves(em, 0xD);
        break;
    case 1:
        Em_PlaySeAtBoneOnFrame(em, 0x78, 0x20, 0x23);
        Em_TurnToGoal(em, (s32) ((EmFlyerWork *)work)->turnSpeed, 2);
        EmRathalos_UpdateCurves(em);
        if (Player_PassedFrameScaled(em, 60.0f, 0U) != 0) {
            em->step++;
            em->step++;
            Em_SetMotion(em, RATH_MOT_AIR_FIREBALL, 0, 0);
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
            Em_SetMotion(em, RATH_MOT_HOVER, 2, 0x22);
        }
        break;
    case 4:
        EmRathalos_MoveByVelYCurve(em, 1U);
        if (em->motion[0].playing == 0) {
            em->step++;
            EmRathalos_EndAirState(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * RATH_ATK_AIR_DIVE: from the hover, turns to the target and lunges at it
 * (speed from the distance left), then hovers again; lands if the target is
 * not in front in time.
 */
void EmRathalos_AirDive(Em *em, EmWork *work)
{
    f32 pos[4];
    f32 speed;
    f32 dist;
    s32 n;
    EmFlyerWork *flyWork = (EmFlyerWork *)work;

    switch (em->step) {
    case 0:
        em->stance = EM_STANCE_AIR;
        Vec_Set(flyWork->startPos, em->posX, em->posY, em->posZ);
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 2);
        Em_AimAtTarget(em);
        em->lookAtTarget = 0;
        if (em->aimAngle <= 0x100 || em->aimAngle >= 0xFF00) {
            em->step = 2;
            Em_SetMotion(em, RATH_MOT_AIR_DIVE, 0, 0);
            Player_ClearVelocity(em);
            em->timer = 45;
        } else {
            em->step++;
            em->timer = 120;
            if (em->motionId[0] != EM_MOTION_ID(0, RATH_MOT_HOVER)) {
                Em_SetMotion(em, RATH_MOT_HOVER, 0, 0);
            }
        }
        break;
    case 1:
        EmRathalos_MoveByVelYCurve(em, 1);
        if (--em->timer <= 0) {
            em->step = 0x62;
            em->timer = 300;
            EmRathalos_ChangeState(em, EM_GROUP_FLY, RATH_FLY_LAND, 1);
            break;
        }
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 2);
        Em_AimAtTarget(em);
        if (em->aimAngle <= 0x100 || em->aimAngle >= 0xFF00) {
            em->step = 2;
            Em_SetMotion(em, RATH_MOT_AIR_DIVE, 0, 0);
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
                speed = em->targetDist / (f32) em->timer--;
                if (1.2f * em->velZ >= speed && speed <= 0.8f * em->velZ) {
                    em->velZ = speed;
                } else if (em->velZ < speed) {
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
            speed = pos[1];
            if (em->posY < speed) {
                em->posY = speed;
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
        dist = Vec_Distance((f32 *) &em->posX, flyWork->startPos);
        work->moveAngX = 0;
        work->moveAngY = Math_AngleXZ(&em->posX, flyWork->startPos) & 0xFFFF;
        work->moveAngZ = 0;
        if (em->timer != 0) {
            em->velZ = dist / (f32) em->timer;
            em->velY = (flyWork->startPos[1] - em->posY) / (f32) em->timer--;
            Em_MoveRotated(em, &work->moveAngX);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            Player_ClearVelocity(em);
            EmRathalos_EndAirState(em, 0);
        }
        break;
    case 0x62:
    case 0x63:
        if (--em->timer <= 0) {
            em->timer = 300;
            if (em->remote == 0) {
                EmRathalos_ChangeState(em, EM_GROUP_FLY, RATH_FLY_LAND, 1);
            }
        }
        break;
    }
}

/*
 * RATH_ATK_ROAR_AT_POINT: looks at the action point nearest bone 0x23 and
 * roars at it, then motion 0x30.
 */
void EmRathalos_RoarAtPoint(Em *em, EmWork *work)
{
    f32 pos[3];
    s32 hasPoint;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 1;
        em->lookAtPoint = 1;
        break;
    case 1:
        hasPoint = Em_NearestActionToBone(em, 0x23, em->lookPoint);
        Em_BonePos(em, 0x23, pos);
        if (!(150.0f < Vec_Distance((f32 *) pos, (f32 *) em->lookPoint))) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_ROAR, 0, 0);
            break;
        }
        if (hasPoint == 0) {
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

/* RATH_ATK_SINGLE_A: motion 0x53 once. */
void EmRathalos_SingleAttackA(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
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

/* RATH_ATK_SINGLE_B: motion 0x38 once. */
void EmRathalos_SingleAttackB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
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

/* RATH_ATK_CHARGE: runs at the goal and ends there. */
void EmRathalos_Charge(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_RUN, 0, 0);
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

/*
 * RATH_ATK_BACK_HOP_FIREBALL (variant 0) and its two turning variants (1:
 * rotY + 0x666, 2: rotY - 0x666 a frame, for 20 frames): hops backward
 * breathing a fireball at frame 90, and lands.
 */
void EmRathalos_BackHopFireball(Em *em, EmWork *work, s32 variant)
{
    f32 floorY;
    s32 ended;
    u8 step;

    step = em->step;
    ended = 0;
    switch (step) {
    case 0:
        em->step = step + 1;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_BACK_HOP, 0, 0);
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
            em->stance = EM_STANCE_AIR;
            ended = EmRathalos_UpdateCurves(em) & 0xFF;
        }
        if ((ended != 0) && (em->posY <= em->floorY)) {
            Em_SetMotion(em, RATH_MOT_BACK_HOP_LAND, 0, 0);
            em->step++;
            em->stance = EM_STANCE_GROUND;
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

/* RATH_ATK_AIR_FIREBALL_DOUBLE: two fireballs from the hover. */
void EmRathalos_AirFireballDouble(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_AIR_FIREBALL_RISE, 0, 0);
        EmRathalos_StartCurves(em, 0xDU);
        break;
    case 1:
        Em_PlaySeAtBoneOnFrame(em, 0x78, 0x20, 0x23);
        EmRathalos_UpdateCurves(em);
        Em_TurnToGoal(em, (s32) ((EmFlyerWork *)work)->turnSpeed, 2);
        if (Player_PassedFrameScaled(em, 60.0f, 0U) != 0) {
            em->step++;
            em->step++;
            Em_SetMotion(em, RATH_MOT_AIR_FIREBALL, 0, 0);
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
            Em_SetMotion(em, RATH_MOT_AIR_FIREBALL, 4, 0);
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
            Em_SetMotion(em, RATH_MOT_HOVER, 2, 0x22);
        }
        break;
    case 5:
        EmRathalos_MoveByVelYCurve(em, 1U);
        if (em->motion[1].playing == 0) {
            em->step++;
            EmRathalos_EndAirState(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * RATH_ATK_CHARGE_TRIP: runs at the goal, and within 500 of it ends the
 * charge with RATH_MOT_CHARGE_TRIP.
 */
void EmRathalos_ChargeTrip(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_RUN, 0, 0);
        break;
    case 1:
        Em_StepToGoal(em, 0x40, 1);
        if (em->goalDist <= 500.0f) {
            em->timer = 1;
        }
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_CHARGE_TRIP, 0, 0);
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

/* RATH_ATK_ROAR_B: the roar without its attached effect. */
void EmRathalos_RoarB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_ROAR, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmRathalos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * RATH_ATK_SOMERSAULT: jumps up (airborne from frame 94), turns over and
 * lands with ground effects and a camera shake.
 */
void EmRathalos_Somersault(Em *em)
{
    f32 floorY;
    s32 ended;

    ended = 0;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_FLIP, 0, 0);
        EmRathalos_StartCurves(em, 0xA);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 94.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            ended = EmRathalos_UpdateCurves(em) & 0xFF;
        }
        if ((ended != 0) && (em->posY <= (372.0f + em->floorY))) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_FLIP_FALL, 0, 0);
            EmRathalos_StartCurves(em, 0xB);
        }
        break;
    case 2:
        EmRathalos_UpdateCurves(em);
        if (em->posY <= em->floorY) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_FLIP_LAND, 0, 0);
            em->stance = EM_STANCE_GROUND;
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

/*
 * RATH_ATK_SWOOP_THEN_LAND: the swoop of RATH_ATK_SWOOP_THEN_LEAVE, ending
 * on the ground (RATH_MOT_LEAP_LAND).
 */
void EmRathalos_SwoopThenLand(Em *em, EmWork *work)
{
    f32 targetWorldPos[3];
    f32 pos[3];
    f32 targetPosB[3];
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
        em->stance = EM_STANCE_AIR;
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
        Area_ToWorld(((Player *) &gPlayers[idx])->area, pos, targetWorldPos);
        em->aimAngle = Math_AngleXZ(em->mapPos, targetWorldPos);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 0xD1, &EmRathalos_FlyParam);
        if (em->aimAngle <= 0x800 || em->aimAngle >= 0xF800) {
            if (!(4000.0f < Vec_DistanceXZ(&em->posX, pos))) {
                EmRathalos_EndSwoop(em, (s32) work);
            } else {
                em->step++;
                work->turnSpeed = 0x100;
                Em_SetMotion(em, RATH_MOT_SWOOP_DIVE, 0, 0);
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
            Em_SetMotion(em, RATH_MOT_SWOOP, 0, 0);
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
        if (em->motionId[0] == EM_MOTION_ID(0, RATH_MOT_SWOOP_CLAW)) {
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
        if (em->motionId[0] == EM_MOTION_ID(0, RATH_MOT_SWOOP_CLAW) && em->motion[0].playing == 0) {
            em->step = 6;
            em->accelX = 0.0f;
            em->accelZ = -10.0f;
            em->accelY = -10.0f;
        }
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 2);
        if (work->wallTimer == 0) {
        EmRathalos_StepPitch(em, 2, 100.0f);
        if (em->motionId[0] == EM_MOTION_ID(0, RATH_MOT_SWOOP_CLAW) && EmMotion_ReachedFrame(em, 0, 76.0f)) {
            Em_GetPitchToTarget(em, &work->moveAngX, 50.0f, 0.0f);
        } else if (em->targetNo != -1) {
            Em_GetTargetPos(em, em->targetNo, targetPosB);
            if (!(200.0f + targetPosB[1] < em->posY)) {
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
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, RATH_MOT_LEAP_LAND, 0, 0);
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

/* RATH_ATK_AIR_FIREBALL_DOUBLE_B: as RATH_ATK_AIR_FIREBALL_DOUBLE. */
void EmRathalos_AirFireballDoubleB(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_AIR_FIREBALL_RISE, 0, 0);
        EmRathalos_StartCurves(em, 0xDU);
        break;
    case 1:
        Em_PlaySeAtBoneOnFrame(em, 0x78, 0x20, 0x23);
        EmRathalos_UpdateCurves(em);
        Em_TurnToGoal(em, (s32) ((EmFlyerWork *)work)->turnSpeed, 2);
        if (Player_PassedFrameScaled(em, 60.0f, 0U) != 0) {
            em->step++;
            em->step++;
            Em_SetMotion(em, RATH_MOT_AIR_FIREBALL, 0, 0);
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
            Em_SetMotion(em, RATH_MOT_AIR_FIREBALL, 4, 0);
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
            Em_SetMotion(em, RATH_MOT_HOVER, 2, 0x22);
        }
        break;
    case 6:
        EmRathalos_MoveByVelYCurve(em, 1U);
        if (em->motion[1].playing == 0) {
            em->step++;
            EmRathalos_EndAirState(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * RATH_ATK_FIREBALL_B: turns and breathes the second ground fireball
 * (RATH_MOT_FIREBALL_B).
 */
void EmRathalos_FireballB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        break;
    case 1:
        if (Em_TurnToTargetMotion(em, 0xE38, 3, 6, 5) != 0) {
            em->step++;
            em->lookAtTarget = 0;
            Em_SetMotion(em, RATH_MOT_FIREBALL_B, 0, 0);
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

/* RATH_ATK_FIREBALL_LOW: the fireball at a target below it. */
void EmRathalos_FireballLow(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        break;
    case 1:
        if (Em_TurnToTargetMotion(em, 0xE38, 3, 6, 5) != 0) {
            em->step++;
            em->lookAtTarget = 0;
            Em_SetMotion(em, RATH_MOT_FIREBALL, 0, 0);
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

/* RATH_ATK_FIREBALL_HIGH: the fireball at a target above it. */
void EmRathalos_FireballHigh(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        break;
    case 1:
        if (Em_TurnToTargetMotion(em, 0xE38, 3, 6, 5) != 0) {
            em->step++;
            em->lookAtTarget = 0;
            Em_SetMotion(em, RATH_MOT_FIREBALL, 0, 0);
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

/*
 * RATH_ATK_TAIL_SPIN_B: spins in place twice the other way (rotY + 0x200 a
 * frame).
 */
void EmRathalos_TailSpinB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_TAIL_SPIN_B, 0, 0);
        Motion_InitTime(em, 0);
        break;
    case 1:
        em->rotY += 0x200;
        if (em->motion[1].playing == 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_TAIL_SPIN_B, 0, 0);
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

/* RATH_ATK_RUN_AND_STOP: runs at the goal and stops (RATH_MOT_RUN_STOP). */
void EmRathalos_RunAndStop(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_RUN, 0, 0);
        break;
    case 1:
        Em_StepToGoal(em, 0x40, 1);
        if (em->goalDist <= 500.0f) {
            em->timer = 1;
        }
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_RUN_STOP, 0, 0);
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

/*
 * Moves the monster 10 forward along its yaw (the air fireballs that
 * advance).
 */
void EmRathalos_StepForward(Em *em, s32 unused)
{
    f32 forward[4];
    f32 out[4];
    s32 ang[4];
    f32 mtx[16];

    ang[0] = 0;
    ang[1] = em->rotY;
    ang[2] = 0;
    Vec_Set(forward, 0.0f, 0.0f, 10.0f);
    Mtx_FromAnglesYXZ(ang, mtx);
    Vec_Rotate(out, forward, mtx);
    em->posX += out[0];
    em->posZ += out[2];
}

/* RATH_ATK_AIR_FIREBALL_FORWARD: the air fireball while moving forward. */
void EmRathalos_AirFireballForward(Em *em, s32 work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_AIR_FIREBALL_RISE, 0, 0);
        EmRathalos_StartCurves(em, 0xD);
        break;
    case 1:
        Em_PlaySeAtBoneOnFrame(em, 0x78, 0x20, 0x23);
        EmRathalos_UpdateCurves(em);
        EmRathalos_StepForward(em, work);
        if (Player_PassedFrameScaled(em, 60.0f, 0U) != 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_AIR_FIREBALL, 0, 0);
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
            Em_SetMotion(em, RATH_MOT_HOVER, 2, 0x22);
        }
        break;
    case 3:
        EmRathalos_MoveByVelYCurve(em, 1U);
        if (em->motion[0].playing == 0) {
            em->step++;
            EmRathalos_EndAirState(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * RATH_ATK_AIR_FIREBALL_DOUBLE_FORWARD: two air fireballs while moving
 * forward.
 */
void EmRathalos_AirFireballDoubleForward(Em *em, s32 work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_AIR_FIREBALL_RISE, 0, 0);
        EmRathalos_StartCurves(em, 0xDU);
        break;
    case 1:
        Em_PlaySeAtBoneOnFrame(em, 0x78, 0x20, 0x23);
        EmRathalos_UpdateCurves(em);
        EmRathalos_StepForward(em, work);
        if (Player_PassedFrameScaled(em, 60.0f, 0U) != 0) {
            em->step++;
            em->step++;
            Em_SetMotion(em, RATH_MOT_AIR_FIREBALL, 0, 0);
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
            Em_SetMotion(em, RATH_MOT_AIR_FIREBALL, 4, 0);
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
            Em_SetMotion(em, RATH_MOT_HOVER, 2, 0x22);
        }
        break;
    case 6:
        EmRathalos_MoveByVelYCurve(em, 1U);
        if (em->motion[1].playing == 0) {
            em->step++;
            EmRathalos_EndAirState(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * RATH_ATK_SOMERSAULT_DOUBLE: a somersault, motion 0x37, and a second
 * somersault.
 */
void EmRathalos_SomersaultDouble(Em *em)
{
    f32 floorY;
    s32 ended;
    u8 step;

    step = em->step;
    ended = 0;
    switch (step) {
    case 0:
        em->step = step + 1;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_FLIP, 0, 0);
        EmRathalos_StartCurves(em, 0xAU);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 94.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            ended = EmRathalos_UpdateCurves(em) & 0xFF;
        }
        if ((ended != 0) && (em->posY <= (372.0f + em->floorY))) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_FLIP_FALL, 0, 0);
            EmRathalos_StartCurves(em, 0xBU);
        }
        break;
    case 2:
        EmRathalos_UpdateCurves(em);
        if (em->posY <= em->floorY) {
            em->step++;
            Em_SetMotion(em, 0x37, 0, 0);
            em->stance = EM_STANCE_GROUND;
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
            Em_SetMotion(em, RATH_MOT_FLIP, 2, 0x46);
            EmRathalos_StartCurves(em, 0xAU);
        }
        break;
    case 4:
        if (EmMotion_ReachedFrame(em, 0, 94.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            ended = EmRathalos_UpdateCurves(em) & 0xFF;
        }
        if ((ended != 0) && (em->posY <= (372.0f + em->floorY))) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_FLIP_FALL, 0, 0);
            EmRathalos_StartCurves(em, 0xBU);
        }
        break;
    case 5:
        EmRathalos_UpdateCurves(em);
        if (em->posY <= em->floorY) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_FLIP_LAND, 0, 0);
            em->stance = EM_STANCE_GROUND;
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

/* RATH_REACT_FLINCH: flinches (motion 0x3C); the script restarts. */
void EmRathalos_Flinch(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
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

/* RATH_REACT_FLINCH_B: flinches (motion 0x42). */
void EmRathalos_FlinchB(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
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

/* RATH_REACT_FLINCH_C: flinches (motion 0x3F). */
void EmRathalos_FlinchC(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
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

/* RATH_REACT_FLINCH_D: flinches (motion 0x40). */
void EmRathalos_FlinchD(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
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

/*
 * RATH_REACT_PART_BREAK: the once-only reaction (motion 0x41), then
 * RATH_REACT_PART_BREAK_END, which breaks part 3.
 */
void EmRathalos_PartBreak(Em *em, EmWork *work)
{
    f32 turnF;
    u32 turn;

    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
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
            EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_PART_BREAK_END, 3);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_PART_BREAK_END, 3);
        }
        break;
    }
}

/* RATH_REACT_KNOCK_DOWN: is knocked down, lies and gets up. */
void EmRathalos_KnockDown(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, RATH_MOT_KNOCK_DOWN, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 0;
            Em_SetMotion(em, RATH_MOT_DOWN, 0, 0);
        }
        break;
    case 2:
        em->timer++;
        if (em->timer >= 0x78) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_DOWN_GET_UP, 0, 0);
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

/* RATH_REACT_END_AIR: glides once. */
void EmRathalos_ReactEndAir(Em *em, EmWork *work)
{
    EmRathalos_EndAirState(em, 0x1);
}

/* RATH_REACT_END_AIR_B: glides once. */
void EmRathalos_ReactEndAirB(Em *em, EmWork *work)
{
    EmRathalos_EndAirState(em, 0x1);
}

/* RATH_REACT_FALL: falls out of the air, hits the floor and gets up. */
void EmRathalos_Fall(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, RATH_MOT_FALL, 0, 0);
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
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, RATH_MOT_FALL_LAND, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_DOWN_GET_UP, 0, 0);
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

/* RATH_REACT_FALL_B: as RATH_REACT_FALL. */
void EmRathalos_FallB(Em *em, EmWork *work)
{
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, RATH_MOT_FALL, 0, 0);
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
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, RATH_MOT_FALL_LAND, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_DOWN_GET_UP, 0, 0);
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

/* RATH_REACT_FLINCH_E: flinches (motion 0x3D). */
void EmRathalos_FlinchE(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
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

/* RATH_REACT_STATUS4: status 4 (motion 0x4C, no flinch), then shakes it off. */
void EmRathalos_Status4(Em *em, EmWork *work)
{
    Em_SetHitReactMode(em, 4U);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x4C, 0, 0);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE_STATUS4_SHAKE_OFF, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE_STATUS4_SHAKE_OFF, 4);
        }
        break;
    }
    Em_SpawnEffect06Bone(em, 2);
}

/* RATH_REACT_TRAPPED: caught (RATH_MOT_TRAPPED, no flinch), then struggles. */
void EmRathalos_Trapped(Em *em, EmWork *work)
{
    Em_SetHitReactMode(em, 3U);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, RATH_MOT_TRAPPED, 0, 0);
        em->noReact = 1;
        em->blind = 0;
        break;
    case 1:
        if ((s8) em->holdTimer < 5) {
            em->holdTimer = 5;
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_TRAPPED_STRUGGLE, 3);
        }
        break;
    case 2:
        if ((s8) em->holdTimer < 5) {
            em->holdTimer = 5;
        }
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_TRAPPED_STRUGGLE, 3);
        }
        break;
    }
}

/*
 * RATH_REACT_TRAPPED_STRUGGLE: struggles three times, then flies out
 * (RATH_FLY_ESCAPE_UPWARD).
 */
void EmRathalos_TrappedStruggle(Em *em, EmWork *work)
{
    em->holdTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, RATH_MOT_STRUGGLE, 0, 0);
        em->noReact = 1;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            Em_SetMotion(em, RATH_MOT_STRUGGLE, 0, 0);
        }
        if (--em->trapTimer <= 0) {
            em->step++;
            EmRathalos_ChangeState(em, EM_GROUP_FLY, RATH_FLY_ESCAPE_UPWARD, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, EM_GROUP_FLY, RATH_FLY_ESCAPE_UPWARD, 4);
        }
        if (em->motion[0].playing == 0) {
            Em_SetMotion(em, RATH_MOT_STRUGGLE, 0, 0);
        }
        break;
    }
}

/*
 * RATH_REACT_STATUS4_DOWNED: status 4 in a hold: lies downed with the
 * periodic effect until it ends.
 */
void EmRathalos_Status4Downed(Em *em, EmWork *work)
{
    em->holdTimer = 5;
    Em_SetHitReactMode(em, 4U);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, RATH_MOT_DOWNED, 0, 0);
        Em_RestartScript(em);
        em->noReact = 1;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetGoal12(em);
            EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_STATUS4_DOWNED_END, 3);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_STATUS4_DOWNED_END, 3);
        }
        break;
    }
    Em_SpawnEffect06Bone(em, 2);
}

/* RATH_REACT_PART_BREAK_END: ends the state. */
void EmRathalos_PartBreakEnd(Em *em, EmWork *work)
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

/*
 * RATH_REACT_KNOCK_DOWN_PART3: the reaction to a hit on the broken part 3
 * (motions 0x4A, 0x4B, 0x48).
 */
void EmRathalos_KnockDownPart3(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
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

/* RATH_REACT_STATUS4_DOWNED_END: struggles twice, then flies out. */
void EmRathalos_Status4DownedEnd(Em *em, EmWork *work)
{
    em->holdTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, RATH_MOT_STRUGGLE, 0, 0);
        em->noReact = 1;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            Em_SetMotion(em, RATH_MOT_STRUGGLE, 0, 0);
        }
        if (--em->trapTimer <= 0) {
            em->step++;
            EmRathalos_ChangeState(em, EM_GROUP_FLY, RATH_FLY_ESCAPE_UPWARD, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, EM_GROUP_FLY, RATH_FLY_ESCAPE_UPWARD, 4);
        }
        break;
    }
}

/*
 * RATH_REACT_STATUS2_DOWNED_END: gets up from the downed lying, then the
 * trap struggle.
 */
void EmRathalos_Status2DownedEnd(Em *em, EmWork *work)
{
    em->holdTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_DOWNED_GET_UP, 0, 0);
        em->blind = 2;
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetGoal12(em);
            em->blind = 0;
            EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_TRAPPED_STRUGGLE, 3);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_TRAPPED_STRUGGLE, 3);
        }
        break;
    }
}

/*
 * RATH_REACT_STATUS1_DOWNED_END: as RATH_REACT_STATUS2_DOWNED_END after
 * status 1.
 */
void EmRathalos_Status1DownedEnd(Em *em, EmWork *work)
{
    em->holdTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_DOWNED_GET_UP, 0, 0);
        em->blind = 2;
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetGoal12(em);
            em->blind = 0;
            EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_TRAPPED_STRUGGLE, 3);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_TRAPPED_STRUGGLE, 3);
        }
        break;
    }
}

/*
 * RATH_REACT_STUN: knocked down with the stun's stage object
 * (StageObj_Stun_CreateEm) until status 5 ends, then the shake-off.
 */
void EmRathalos_Stun(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, RATH_MOT_KNOCK_DOWN, 0, 0);
        StageObj_Stun_CreateEm(em, 1, 0x22, (s32) ((f32) em->timer + (em->motion[0].frameCount / 2.0f)), 150.0f, 3.0f);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_DOWN, 0, 0);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_DOWN_GET_UP, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE_STUN_SHAKE_OFF, 4);
        }
        break;
    case 4:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE_STUN_SHAKE_OFF, 4);
        }
        break;
    }
}

/*
 * RATH_REACT_STUN_DOWNED: the stun in a hold: lies downed with the stun's
 * object.
 */
void EmRathalos_StunDowned(Em *em, EmWork *work)
{
    em->holdTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, RATH_MOT_DOWNED, 0, 0);
        Em_RestartScript(em);
        em->noReact = 1;
        StageObj_Stun_CreateEm(em, 1, 0x22, (s32) ((f32) em->timer + (em->motion[0].frameCount / 2.0f)), 150.0f, 3.0f);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetGoal12(em);
            EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_STUN_DOWNED_END, 3);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_STUN_DOWNED_END, 3);
        }
        break;
    }
}

/* RATH_REACT_STUN_DOWNED_END: struggles twice, then flies out. */
void EmRathalos_StunDownedEnd(Em *em, EmWork *work)
{
    em->holdTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, RATH_MOT_STRUGGLE, 0, 0);
        em->noReact = 1;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            Em_SetMotion(em, RATH_MOT_STRUGGLE, 0, 0);
        }
        if (--em->trapTimer <= 0) {
            em->step++;
            EmRathalos_ChangeState(em, EM_GROUP_FLY, RATH_FLY_ESCAPE_UPWARD, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, EM_GROUP_FLY, RATH_FLY_ESCAPE_UPWARD, 4);
        }
        break;
    }
}

/*
 * RATH_REACT_HOLD2: held by the second hold (Em_SetHold2), the players
 * flagged while it lasts, then the shake-off.
 */
void EmRathalos_Hold2(Em *em, EmWork *work)
{
    Em_SetHold2(em);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
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
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE_HOLD2_SHAKE_OFF, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE_HOLD2_SHAKE_OFF, 4);
        }
        break;
    }
}

/*
 * RATH_EVENT_INTRO: the story quests' opening: flies in to the area's
 * point, swoops, lands, roars and feeds, the camera following.
 */
void EmRathalos_QuestIntro(Em *em, EmWork *work)
{
    f32 pos[3];
    f32 off[3];
    f32 out[3];
    s32 ang[3];
    f32 offsetB[3];
    f32 rotatedB[3];
    s32 anglesB[3];
    f32 mtx[16];
    f32 mtxB[16];
    Em *otherEm;
    s8 i;

    Stage_GetAreaRecord(em->area);
    for (i = 0, otherEm = gEms; i < 20; i++, otherEm++) {
        if ((otherEm->type == 0xC || otherEm->type == EM_KIND_POPO) && otherEm->active != 0 && otherEm->variant == 0) {
            goto found;
        }
    }
    otherEm = NULL;
found:
    if (otherEm == NULL) {
        Vec_Set(pos, 10000.0f, 0.0f, 10000.0f);
    } else {
        *(Em **) &otherEm->unk898[0] = em;
        Vec_Set(pos, otherEm->posX, otherEm->posY, otherEm->posZ);
    }
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        em->velZ = 100.0f;
        em->posX = 20000.0f;
        em->posY = 2000.0f;
        em->posZ = 12000.0f;
        em->goal[0] = 26500.0f;
        em->goal[1] = 2000.0f;
        em->goal[2] = 19000.0f;
        em->rotY = Math_AngleXZ(&em->posX, em->goal);
        GmEm_ResetSize(em);
        Em_SetMotion(em, RATH_MOT_FLY, 0, 0);
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
            Em_SetMotion(em, RATH_MOT_SWOOP_DIVE, 0, 0);
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
            Em_SetMotion(em, RATH_MOT_SWOOP, 0, 0);
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
            (em->motionId[0] == EM_MOTION_ID(0, RATH_MOT_SWOOP_CLAW) && (EmMotion_ReachedFrame(em, 0, 52.0f) || em->motion[0].playing == 0))) {
            em->step++;
            em->posY = em->floorY;
            em->rotX = 0;
            em->rotZ = 0;
            em->posY = em->floorY;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, RATH_MOT_LEAP_LAND, 4, 0);
            if (otherEm != NULL) {
                ang[0] = 0;
                ang[1] = em->rotY;
                ang[2] = 0;
                Vec_Set(off, 0.0f, 0.0f, 600.0f);
                Mtx_FromAnglesYXZ(ang, mtx);
                Vec_Rotate(out, off, mtx);
                otherEm->posX = em->posX + out[0];
                otherEm->posZ = em->posZ + out[2];
                if (otherEm->stateGroup != EM_GROUP_EVENT || otherEm->state != 1) {
                    Em_SetStateRand(otherEm, 6, 1);
                    otherEm->rotY = (em->rotY - 0x4000) & 0xFFFF;
                }
            }
        }
        break;
    case 6:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_ROAR, 0, 0);
            if (otherEm != NULL) {
                Em_SetStateRand(otherEm, 6, 2);
            }
        }
        if (otherEm != NULL) {
            anglesB[0] = 0;
            anglesB[1] = em->rotY;
            anglesB[2] = 0;
            Vec_Set(offsetB, 0.0f, 0.0f, 600.0f);
            Mtx_FromAnglesYXZ(anglesB, mtxB);
            Vec_Rotate(rotatedB, offsetB, mtxB);
            otherEm->posX = em->posX + rotatedB[0];
            otherEm->posZ = em->posZ + rotatedB[2];
            otherEm->noPushTimer = 5;
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
            Em_SetMotion(em, RATH_MOT_HEAD_DOWN, 0xA, 0);
            if (otherEm != NULL) {
                Em_SetStateRand(otherEm, 6, 3);
            }
        }
        break;
    case 8:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_FEED, 0, 0);
        }
        break;
    case 9:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_TURN_IN_PLACE, 0, 0);
            if (otherEm != NULL) {
                otherEm->motionSpeed = 0.0f;
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

/*
 * RATH_EVENT_CAPTURED: status 1 in holdTimer's hold: lies downed; once
 * QuestEm_CanAppear allows it, the capture (QuestEm_Captured) and
 * RATH_DEFEAT_REAPPEAR.
 */
void EmRathalos_Captured(Em *em)
{
    f32 pos[3];

    Em_SetCaptured(em, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->stateVar0 = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_DOWNED, 0, 0);
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
                EmRathalos_ChangeState(em, EM_GROUP_DEFEAT, RATH_DEFEAT_REAPPEAR, 4);
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

/*
 * RATH_EVENT_CAPTURED_HOLD2: the capture in the second hold, collapsing
 * first.
 */
void EmRathalos_CapturedHold2(Em *em)
{
    s32 timer;
    s32 timerLeft;

    Em_SetCaptured(em, 1);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_COLLAPSE, 0, 0);
        Em_LoadReactionSet(em, 1);
        Em_BeginStatus1(em);
        Em_ClearPoison(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_LIE, 0, 0);
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
        timerLeft = em->timer;
        if (timerLeft > 0) {
            em->timer = timerLeft - 1;
        }
        timer = em->timer;
        if (timer <= 0) {
            if (em->remote == 0) {
                EmRathalos_ChangeState(em, EM_GROUP_DEFEAT, RATH_DEFEAT_REAPPEAR, 4);
                break;
            }
        } else {
            em->alpha = (f32) timer / 150.0f;
        }
        break;
    }
}

/*
 * RATH_EVENT_STORY: the Rathian kinds' story scene (started through
 * EmHermitaur_ChangeState in stages 0x3F6/0x3F8): walks, runs at the point
 * and trips, the camera shaking.
 */
void EmRathalos_StoryEvent(Em *em)
{
    f32 goalDistXZ;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        em->fieldCollide = 0;
        em->posX = 14350.0f;
        em->posY = 0.0f;
        em->posZ = 9830.0f;
        em->goal[0] = 11000.0f;
        em->goal[1] = 0.0f;
        em->goal[2] = 10870.0f;
        em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        GmEm_ResetSize(em);
        Em_SetMotion(em, RATH_MOT_IDLE, 0, 0);
        em->timer = 120;
        em->stateVar0 = 0;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_WALK, 0, 0);
            em->timer = 900;
        }
        break;
    case 2:
        goalDistXZ = Vec_DistanceXZ(&em->posX, em->goal);
        em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        if (--em->timer <= 0 || goalDistXZ <= 150.0f) {
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
            Em_SetMotion(em, RATH_MOT_RUN, 0, 0);
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
        goalDistXZ = Vec_DistanceXZ(&em->posX, em->goal);
        em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        if (--em->timer <= 0 || goalDistXZ <= 300.0f) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_CHARGE_TRIP, 0, 0);
        }
        break;
    case 7:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_ALERT_IDLE, 0, 0);
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

/*
 * RATH_VS_EAT_BAIT: at the versus match's end, after the defeat camera,
 * eats the bait thrown from the winner's side (StageObj_K25_Create), then
 * routes the monster by team.
 */
void EmRathalos_VersusEatBait(Em *em, EmWork *work)
{
    EmRathalosVec throwPoint;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        if (Versus_DefeatCamera(em) == 1) {
            em->step++;
            Em_SetMotion(em, 0x1A, 0, 0);
            Em_SetLayerMotion(em, EM_MOTION_ID(0, RATH_MOT_IDLE), 0, 0, 0);
            Em_SetLayerMotion(em, EM_MOTION_ID(0, RATH_MOT_IDLE), 0, 0, 2);
            em->timer = 30;
        }
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 10.0f, 0) != 0) {
            throwPoint = EmRathalos_CarveThrowPt;
            StageObj_K25_Create(&gPlayers[(u8) em->side - 1], em, 0, 0, &throwPoint.x);
        }
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_HEAD_DOWN, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_FEED, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 30;
            Em_SetMotion(em, RATH_MOT_ALERT_IDLE, 0, 0);
        }
        break;
    case 4:
        if (--em->timer <= 0) {
            Versus_RouteEmByTeam(em);
        }
        break;
    }
}

/*
 * RATH_VS_POSE_A: motion 0x3D, then waits for both sides
 * (RATH_VS_WAIT_BOTH).
 */
void EmRathalos_VersusPoseA(Em *em, EmWork *work)
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
            Em_SetMotion(em, RATH_MOT_IDLE, 0, 0);
            EmRathalos_ChangeState(em, EM_GROUP_VERSUS, RATH_VS_WAIT_BOTH, 1);
        }
        break;
    }
}

/* RATH_VS_POSE_B: motion 0x36, then waits for both sides. */
void EmRathalos_VersusPoseB(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, RATH_MOT_TURN_IN_PLACE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, RATH_MOT_IDLE, 0, 0);
            EmRathalos_ChangeState(em, EM_GROUP_VERSUS, RATH_VS_WAIT_BOTH, 1);
        }
        break;
    }
}

/*
 * RATH_VS_POSE_C: a somersault for the Rathian kinds, a fireball for the
 * Rathalos kinds, then waits for both sides.
 */
void EmRathalos_VersusPoseC(Em *em, EmWork *work)
{
    s32 done = 0;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    if (em->type == EM_KIND_RATHIAN || em->type == EM_KIND_GOLD_RATHIAN || em->type == EM_KIND_PINK_RATHIAN) {
        switch (em->step) {
        case 0:
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->lookAtTarget = 0;
            Em_SetMotion(em, RATH_MOT_FLIP, 0, 0);
            EmRathalos_StartCurves(em, 0xA);
            break;
        case 1:
            if (EmMotion_ReachedFrame(em, 0, 94.0f) != 0) {
                em->stance = EM_STANCE_AIR;
                done = EmRathalos_UpdateCurves(em);
            }
            if (done != 0 && em->posY <= 372.0f + em->floorY) {
                em->step++;
                Em_SetMotion(em, RATH_MOT_FLIP_FALL, 0, 0);
                EmRathalos_StartCurves(em, 0xB);
            }
            break;
        case 2:
            EmRathalos_UpdateCurves(em);
            if (em->posY <= em->floorY) {
                em->step++;
                Em_SetMotion(em, RATH_MOT_FLIP_LAND, 0, 0);
                em->stance = EM_STANCE_GROUND;
                em->posY = em->floorY;
                EmRathalos_SpawnK14Type0B(em);
                CamShake_ReqAIf(em, 2);
            }
            break;
        case 3:
            if (em->motion[0].playing == 0) {
                em->step++;
                Em_SetMotion(em, RATH_MOT_IDLE, 0, 0);
                EmRathalos_ChangeState(em, EM_GROUP_VERSUS, RATH_VS_WAIT_BOTH, 1);
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
            em->stance = EM_STANCE_GROUND;
            em->lookAtTarget = 0;
            Em_SetMotion(em, RATH_MOT_FIREBALL, 0, 0);
            break;
        case 1:
            if (em->motion[1].playing == 0) {
                em->step++;
                Em_SetMotion(em, RATH_MOT_IDLE, 0, 0);
                EmRathalos_ChangeState(em, EM_GROUP_VERSUS, RATH_VS_WAIT_BOTH, 1);
            }
            break;
        }
    }
}

/*
 * RATH_VS_WAIT_BOTH: idles until both sides are defeated
 * (Versus_BothDefeated), then RATH_VS_END.
 */
void EmRathalos_VersusWaitBoth(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        if (em->motionId[0] != EM_MOTION_ID(0, RATH_MOT_IDLE)) {
            Em_SetMotion(em, RATH_MOT_IDLE, 0, 0);
        }
        
    case 1:
        if (Versus_BothDefeated() == 1) {
            em->step++;
            EmRathalos_ChangeState(em, EM_GROUP_VERSUS, RATH_VS_END, 1);
        }
        break;
    }
}

/* RATH_VS_END: idles 90 frames, then ends the state. */
void EmRathalos_VersusEnd(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 0x5A;
        if (em->motionId[0] != EM_MOTION_ID(0, RATH_MOT_IDLE)) {
            Em_SetMotion(em, RATH_MOT_IDLE, 0, 0);
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

/*
 * RATH_DEFEAT_DIE: falls dead (QuestEm_Hunted), leaves a carving mining
 * point while it lies, then disappears or reappears (RATH_DEFEAT_REAPPEAR).
 */
void EmRathalos_Die(Em *em, EmWork *work)
{
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, RATH_MOT_DIE, 0, 0);
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_DEAD;
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
                EmRathalos_ChangeState(em, EM_GROUP_DEFEAT, RATH_DEFEAT_REAPPEAR, 4);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    case 99:
        if (em->motion[0].playing == 0) {
            em->stance = EM_STANCE_GROUND;
            EmRathalos_EndState(em, 0, 0);
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
        }
        break;
    }
}

/* RATH_DEFEAT_DIE_HELD: dies lying downed in a hold, as RATH_DEFEAT_DIE. */
void EmRathalos_DieHeld(Em *em, EmWork *work)
{
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, RATH_MOT_DOWNED, 0, 0);
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_DEAD;
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
                EmRathalos_ChangeState(em, EM_GROUP_DEFEAT, RATH_DEFEAT_REAPPEAR, 4);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    case 99:
        if (em->motion[0].playing == 0) {
            em->stance = EM_STANCE_GROUND;
            EmRathalos_EndState(em, 0, 0);
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
        }
        break;
    }
}

/*
 * RATH_DEFEAT_DIE_FALLING: dies in the air: falls, hits the floor, and lies
 * as RATH_DEFEAT_DIE.
 */
void EmRathalos_DieFalling(Em *em, EmWork *work)
{
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_SetMotion(em, RATH_MOT_FALL, 0, 0);
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
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, RATH_MOT_FALL_LAND, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    case 2:
        if (Player_PassedFrameScaled(em, 60.0f, 0) != 0) {
            em->step++;
            em->timer = 0;
            Em_SetMotion(em, RATH_MOT_DIE_FALLING, 0, 0);
            em->stance = EM_STANCE_DEAD;
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
                EmRathalos_ChangeState(em, EM_GROUP_DEFEAT, RATH_DEFEAT_REAPPEAR, 4);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    case 99:
        if (em->motion[0].playing == 0) {
            em->stance = EM_STANCE_GROUND;
            EmRathalos_EndState(em, 0, 0);
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
        }
        break;
    }
}

/*
 * RATH_DEFEAT_REAPPEAR: when the quest lets it appear again, flies in and
 * lands.
 */
void EmRathalos_Reappear(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        if (QuestEm_CanAppear(em) == 1) {
            em->posY = gAreaGroundY[em->area];
            em->stance = EM_STANCE_AIR;
            EmRathalos_ChangeState(em, EM_GROUP_FLY, RATH_FLY_LAND, 0);
            break;
        }
        em->taskPhase++;
    case 1:
        break;
    }
}

/* Runs the current group-0 state (EM_GROUP_IDLE). */
void EmRathalos_RunIdleState(Em *em, EmWork *work)
{
    switch (em->state) {
    case RATH_IDLE_STAND:
        EmRathalos_Stand(em);
        break;
    case RATH_IDLE:
        EmRathalos_Idle(em);
        break;
    case RATH_IDLE_ACTION_A:
        EmRathalos_IdleActionA(em);
        break;
    case RATH_IDLE_ACTION_B:
        EmRathalos_IdleActionB(em);
        break;
    case RATH_IDLE_ACTION_C:
        EmRathalos_IdleActionC(em);
        break;
    case RATH_IDLE_ACTION_D:
        EmRathalos_IdleActionD(em);
        break;
    case RATH_IDLE_ACTION_E:
        EmRathalos_IdleActionE(em);
        break;
    case RATH_IDLE_ACTION_F:
        EmRathalos_IdleActionF(em);
        break;
    case RATH_IDLE_ACTION_G:
        EmRathalos_IdleActionG(em);
        break;
    case RATH_IDLE_PAIR_A:
        EmRathalos_IdlePairA(em);
        break;
    case RATH_IDLE_TURN_TO_GOAL:
        EmRathalos_IdleTurnToGoal(em);
        break;
    case RATH_IDLE_ACTION_H:
        EmRathalos_IdleActionH(em);
        break;
    case RATH_IDLE_ACTION_I:
        EmRathalos_IdleActionI(em);
        break;
    case RATH_IDLE_ACTION_J:
        EmRathalos_IdleActionJ(em);
        break;
    case RATH_IDLE_EAT_NEED2:
        EmRathalos_EatNeed2(em);
        break;
    case RATH_IDLE_PAIR_B:
        EmRathalos_IdlePairB(em);
        break;
    case RATH_IDLE_PAIR_C:
        EmRathalos_IdlePairC(em);
        break;
    case RATH_IDLE_ALERT:
        EmRathalos_AlertIdle(em);
        break;
    case RATH_IDLE_REST:
        EmRathalos_Rest(em, work);
        break;
    case RATH_IDLE_EAT_START:
        EmRathalos_EatStart(em);
        break;
    case RATH_IDLE_STATUS2_COLLAPSE:
        EmRathalos_Status2Collapse(em);
        break;
    case RATH_IDLE_STATUS2_LIE_DOWN:
        EmRathalos_Status2LieDown(em, work);
        break;
    case RATH_IDLE_ACTION_K:
        EmRathalos_IdleActionK(em);
        break;
    case RATH_IDLE_REST_END:
        EmRathalos_RestEnd(em, work);
        break;
    case RATH_IDLE_STATUS2_END:
        EmRathalos_Status2End(em);
        break;
    case RATH_IDLE_SHAKE_OFF:
        EmRathalos_ShakeOff(em);
        break;
    case RATH_IDLE_EAT_NEED1:
        EmRathalos_EatNeed1(em);
        break;
    case RATH_IDLE_STATUS1_COLLAPSE:
        EmRathalos_Status1Collapse(em);
        break;
    case RATH_IDLE_STATUS1_END:
        EmRathalos_Status1End(em);
        break;
    case RATH_IDLE_STATUS2_DOWNED:
        EmRathalos_Status2Downed(em, work);
        break;
    case RATH_IDLE_STATUS1_DOWNED:
        EmRathalos_Status1Downed(em, work);
        break;
    case RATH_IDLE_STATUS4_SHAKE_OFF:
        EmRathalos_ShakeOffAfterStatus(em, work, 0);
        break;
    case RATH_IDLE_STUN_SHAKE_OFF:
        EmRathalos_ShakeOffAfterStatus(em, work, 1);
        break;
    case RATH_IDLE_HOLD2_SHAKE_OFF:
        EmRathalos_ShakeOffAfterStatus(em, work, 2);
        break;
    case RATH_IDLE_EAT:
        EmRathalos_Eat(em);
        break;
    }
}

/* Runs the current group-1 state (EM_GROUP_MOVE). */
void EmRathalos_RunMoveState(Em *em, EmWork *work)
{
    switch (em->state) {
    case RATH_MOVE_WALK:
        EmRathalos_Walk(em, work);
        break;
    case RATH_MOVE_END:
        EmRathalos_MoveEnd(em, work);
        break;
    case RATH_MOVE_END2:
        EmRathalos_MoveEnd2(em, work);
        break;
    case RATH_MOVE_TURN:
        EmRathalos_Turn(em, work);
        break;
    case RATH_MOVE_RUN:
        EmRathalos_Run(em, work);
        break;
    case RATH_MOVE_TURN_IN_COMBAT:
        EmRathalos_TurnInCombat(em, work);
        break;
    case RATH_MOVE_LIMP:
        EmRathalos_Limp(em, work);
        break;
    case RATH_MOVE_TURN_TO_GOAL:
        EmRathalos_TurnToGoal(em, work);
        break;
    case RATH_MOVE_RUN_SLOW:
        EmRathalos_Run(em, work);
        break;
    case RATH_MOVE_TURN_RAGE:
        EmRathalos_TurnToGoal(em, work);
        break;
    case RATH_MOVE_TURN_PENDING_RAGE:
        EmRathalos_TurnToGoal(em, work);
        
    default:
        break;
    }
}

/* Runs the current group-2 state (EM_GROUP_FLY). */
void EmRathalos_RunFlyState(Em *em, EmWork *work)
{
    switch (em->state) {
    case RATH_FLY_TAKE_OFF:
        EmRathalos_TakeOff(em, work);
        break;
    case RATH_FLY_LAND:
        EmRathalos_Land(em, work);
        break;
    case RATH_FLY_HOVER:
        EmRathalos_Hover(em, work);
        break;
    case RATH_FLY_HOP_TO_GOAL:
        EmRathalos_HopToGoal(em, work);
        break;
    case RATH_FLY_HOP_FORWARD:
        EmRathalos_HopForward(em, work);
        break;
    case RATH_FLY_HOP_BACK:
        EmRathalos_HopBack(em, work);
        break;
    case RATH_FLY_ROUTE:
        EmRathalos_FlyRoute(em, work);
        break;
    case RATH_FLY_HOVER_TO_GOAL:
        EmRathalos_HoverToGoal(em, work);
        break;
    case RATH_FLY_OUT_OF_AREA:
        EmRathalos_FlyOutOfArea(em, work);
        break;
    case RATH_FLY_TO_GOAL_AND_LAND:
        EmRathalos_FlyToGoalAndLand(em, work);
        break;
    case RATH_FLY_OUT_OF_AREA_ALT:
        EmRathalos_FlyOutOfAreaAlt(em, work);
        break;
    case RATH_FLY_GLIDE_TO_GOAL:
        EmRathalos_GlideToGoal(em, work);
        break;
    case RATH_FLY_BACK_HOP:
        EmRathalos_BackHop(em);
        break;
    case RATH_FLY_INTO_AREA:
        EmRathalos_FlyIntoArea(em, work);
        break;
    case RATH_FLY_HOVER_ONCE:
        EmRathalos_HoverOnce(em, work);
        break;
    case RATH_FLY_GLIDE_ONCE:
        EmRathalos_GlideOnce(em, work);
        break;
    case RATH_FLY_HOVER_TURN:
        EmRathalos_HoverTurn(em, work);
        break;
    case RATH_FLY_ROUTE_B:
        EmRathalos_FlyRouteB(em, work);
        break;
    case RATH_FLY_LEVEL_OUT_AND_LAND:
        EmRathalos_LevelOutAndLand(em, work);
        break;
    case RATH_FLY_GLIDE_TO_GOAL_B:
        EmRathalos_GlideToGoalB(em, work);
        break;
    case RATH_FLY_END:
        EmRathalos_EndFlight(em);
        break;
    case RATH_FLY_HOVER_STRAIGHT_TO_GOAL:
        EmRathalos_HoverStraightToGoal(em, work);
        break;
    case RATH_FLY_LEVEL_OUT:
        EmRathalos_LevelOut(em, work);
        break;
    case RATH_FLY_ESCAPE_UPWARD:
        EmRathalos_EscapeUpward(em, work);
        break;
    case RATH_FLY_HOVER_RISE:
        EmRathalos_HoverRise(em, work);
        
    default:
        break;
    }
}

/* Runs the current group-3 state (EM_GROUP_ATTACK). */
void EmRathalos_RunAttackState(Em *em, EmWork *work)
{
    switch (em->state) {
    case RATH_ATK_TURN_AND_STARE:
        EmRathalos_TurnAndStare(em);
        break;
    case RATH_ATK_LEAP:
        EmRathalos_Leap(em, work);
        break;
    case RATH_ATK_STARE:
        EmRathalos_Stare(em);
        break;
    case RATH_ATK_FIREBALL:
        EmRathalos_Fireball(em, work);
        break;
    case RATH_ATK_ROAR:
        EmRathalos_Roar(em);
        break;
    case RATH_ATK_TAIL_SPIN:
        EmRathalos_TailSpin(em);
        break;
    case RATH_ATK_LIE_AND_RISE:
        EmRathalos_LieAndRise(em);
        break;
    case RATH_ATK_SWOOP_THEN_LEAVE:
        EmRathalos_SwoopThenLeave(em, work);
        break;
    case RATH_ATK_AIR_FIREBALL:
        EmRathalos_AirFireball(em, work);
        break;
    case RATH_ATK_AIR_FIREBALL_B:
        EmRathalos_AirFireballB(em, work);
        break;
    case RATH_ATK_AIR_DIVE:
        EmRathalos_AirDive(em, work);
        break;
    case RATH_ATK_ROAR_AT_POINT:
        EmRathalos_RoarAtPoint(em, work);
        break;
    case RATH_ATK_SINGLE_A:
        EmRathalos_SingleAttackA(em);
        break;
    case RATH_ATK_SINGLE_B:
        EmRathalos_SingleAttackB(em);
        break;
    case RATH_ATK_CHARGE:
        EmRathalos_Charge(em);
        break;
    case RATH_ATK_BACK_HOP_FIREBALL:
        EmRathalos_BackHopFireball(em, work, 0);
        break;
    case RATH_ATK_AIR_FIREBALL_DOUBLE:
        EmRathalos_AirFireballDouble(em, work);
        break;
    case RATH_ATK_CHARGE_TRIP:
        EmRathalos_ChargeTrip(em);
        break;
    case RATH_ATK_ROAR_B:
        EmRathalos_RoarB(em);
        break;
    case RATH_ATK_SOMERSAULT:
        EmRathalos_Somersault(em);
        break;
    case RATH_ATK_SWOOP_THEN_LAND:
        EmRathalos_SwoopThenLand(em, work);
        break;
    case RATH_ATK_AIR_FIREBALL_DOUBLE_B:
        EmRathalos_AirFireballDoubleB(em, work);
        break;
    case RATH_ATK_FIREBALL_B:
        EmRathalos_FireballB(em);
        break;
    case RATH_ATK_FIREBALL_LOW:
        EmRathalos_FireballLow(em, work);
        break;
    case RATH_ATK_FIREBALL_HIGH:
        EmRathalos_FireballHigh(em, work);
        break;
    case RATH_ATK_TAIL_SPIN_B:
        EmRathalos_TailSpinB(em);
        break;
    case RATH_ATK_RUN_AND_STOP:
        EmRathalos_RunAndStop(em);
        break;
    case RATH_ATK_AIR_FIREBALL_FORWARD:
        EmRathalos_AirFireballForward(em, (s32) work);
        break;
    case RATH_ATK_AIR_FIREBALL_DOUBLE_FORWARD:
        EmRathalos_AirFireballDoubleForward(em, (s32) work);
        break;
    case RATH_ATK_SOMERSAULT_DOUBLE:
        EmRathalos_SomersaultDouble(em);
        break;
    case RATH_ATK_BACK_HOP_FIREBALL_LEFT:
        EmRathalos_BackHopFireball(em, work, 1);
        break;
    case RATH_ATK_BACK_HOP_FIREBALL_RIGHT:
        EmRathalos_BackHopFireball(em, work, 2);
        break;
    }
}

/* Runs the current group-4 state (EM_GROUP_REACT). */
void EmRathalos_RunReactState(Em *em, EmWork *work)
{
    switch (em->state) {
    case RATH_REACT_FLINCH:
        EmRathalos_Flinch(em, work);
        break;
    case RATH_REACT_FLINCH_B:
        EmRathalos_FlinchB(em, work);
        break;
    case RATH_REACT_FLINCH_C:
        EmRathalos_FlinchC(em, work);
        break;
    case RATH_REACT_FLINCH_D:
        EmRathalos_FlinchD(em, work);
        break;
    case RATH_REACT_PART_BREAK:
        EmRathalos_PartBreak(em, work);
        break;
    case RATH_REACT_KNOCK_DOWN:
        EmRathalos_KnockDown(em, work);
        break;
    case RATH_REACT_END_AIR:
        EmRathalos_ReactEndAir(em, work);
        break;
    case RATH_REACT_END_AIR_B:
        EmRathalos_ReactEndAirB(em, work);
        break;
    case RATH_REACT_FALL:
        EmRathalos_Fall(em, work);
        break;
    case RATH_REACT_FALL_B:
        EmRathalos_FallB(em, work);
        break;
    case RATH_REACT_FLINCH_E:
        EmRathalos_FlinchE(em, work);
        break;
    case RATH_REACT_STATUS4:
        EmRathalos_Status4(em, work);
        break;
    case RATH_REACT_TRAPPED:
        EmRathalos_Trapped(em, work);
        break;
    case RATH_REACT_TRAPPED_STRUGGLE:
        EmRathalos_TrappedStruggle(em, work);
        break;
    case RATH_REACT_STATUS4_DOWNED:
        EmRathalos_Status4Downed(em, work);
        break;
    case RATH_REACT_PART_BREAK_END:
        EmRathalos_PartBreakEnd(em, work);
        break;
    case RATH_REACT_KNOCK_DOWN_PART3:
        EmRathalos_KnockDownPart3(em, work);
        break;
    case RATH_REACT_STATUS4_DOWNED_END:
        EmRathalos_Status4DownedEnd(em, work);
        break;
    case RATH_REACT_STATUS2_DOWNED_END:
        EmRathalos_Status2DownedEnd(em, work);
        break;
    case RATH_REACT_STATUS1_DOWNED_END:
        EmRathalos_Status1DownedEnd(em, work);
        break;
    case RATH_REACT_STUN:
        EmRathalos_Stun(em, work);
        break;
    case RATH_REACT_STUN_DOWNED:
        EmRathalos_StunDowned(em, work);
        break;
    case RATH_REACT_STUN_DOWNED_END:
        EmRathalos_StunDownedEnd(em, work);
        break;
    case RATH_REACT_HOLD2:
        EmRathalos_Hold2(em, work);
        
    default:
        break;
    }
}

/* Runs the current group-5 state (EM_GROUP_DEFEAT). */
void EmRathalos_RunDefeatState(Em *em, EmWork *work)
{
    switch (em->state) {
    case RATH_DEFEAT_DIE:
        EmRathalos_Die(em, work);
        break;
    case RATH_DEFEAT_DIE_HELD:
        EmRathalos_DieHeld(em, work);
        break;
    case RATH_DEFEAT_DIE_FALLING:
        EmRathalos_DieFalling(em, work);
        break;
    case RATH_DEFEAT_REAPPEAR:
        EmRathalos_Reappear(em, work);
        break;
    }
}

/* Runs the current group-6 or group-7 state (EM_GROUP_EVENT). */
void EmRathalos_RunEventState(Em *em, EmWork *work)
{
    switch (em->state) {
    case RATH_EVENT_INTRO:
        EmRathalos_QuestIntro(em, work);
        break;
    case RATH_EVENT_CAPTURED:
        EmRathalos_Captured(em);
        break;
    case RATH_EVENT_CAPTURED_HOLD2:
        EmRathalos_CapturedHold2(em);
        break;
    case RATH_EVENT_STORY:
        EmRathalos_StoryEvent(em);
        break;
    }
}

/* Runs the current group-8 state (EM_GROUP_VERSUS). */
void EmRathalos_RunVersusState(Em *em, EmWork *work)
{
    switch (em->state) {
    case RATH_VS_EAT_BAIT:
        EmRathalos_VersusEatBait(em, work);
        break;
    case RATH_VS_POSE_A:
        EmRathalos_VersusPoseA(em, work);
        break;
    case RATH_VS_POSE_B:
        EmRathalos_VersusPoseB(em, work);
        break;
    case RATH_VS_POSE_C:
        EmRathalos_VersusPoseC(em, work);
        break;
    case RATH_VS_WAIT_BOTH:
        EmRathalos_VersusWaitBoth(em, work);
        break;
    case RATH_VS_END:
        EmRathalos_VersusEnd(em, work);
        break;
    }
}

/*
 * The family's frame: the combat timer and the needs (by area kind), then
 * Em_UpdateDamage's result picks a reaction state (see EM_GROUP_REACT's
 * enum); the story quests' area events; otherwise the pending script, then
 * the current state (twice when a state was requested). In flight states it
 * keeps guardTimer at 2.
 */
void EmRathalos_Update(Em *em)
{
    u8 buf[4];
    EmFlyerWork *w = (EmFlyerWork *)&em->work;
    f32 ratio;

    Em_UpdateCombatTimer(em);
    Em_RaiseTrigger80Lax(em);
    if (em->type == EM_KIND_RATHIAN || em->type == EM_KIND_GOLD_RATHIAN || em->type == EM_KIND_PINK_RATHIAN) {
        ratio = 0.1f;
    } else {
        ratio = 0.3f;
    }
    if (em->noAlert != 1 && em->rage == 0) {
        switch (Area_BaseId(gScene.map)) {
        case 2:
            if (em->questKey == 1) {
                if (em->type == EM_KIND_RATHIAN || em->type == EM_KIND_GOLD_RATHIAN || em->type == EM_KIND_PINK_RATHIAN) {
                    Em_CheckLowHp(em, ratio);
                    Em_DecayNeed3(em);
                } else {
                    Em_DecayNeed2(em);
                }
                Em_DecayNeed1Only(em);
            } else {
                Em_CheckLowHp(em, ratio);
                Em_DecayNeed2(em);
                Em_DecayNeed1(em);
                Em_DecayNeed3(em);
            }
            break;
        case 6:
            Em_CheckLowHp(em, ratio);
            Em_DecayNeed2(em);
            Em_DecayNeed1(em);
            Em_DecayNeed3(em);
            Em_CheckCarryRequest(em);
            break;
        case 3:
            Em_CheckCarryRequest(em);
        case 4:
            Em_CheckLowHp(em, ratio);
            Em_DecayNeed2(em);
            Em_DecayNeed1Only(em);
            break;
        }
    }
    if (w->fxMotion != 0) {
        w->fxMotion -= 1;
    }
    switch (Em_UpdateDamage(em, buf) & 0xFF) {
    case EM_DMG_NONE:
    case EM_DMG_FLASH:
    case EM_DMG_HIT:
    case EM_DMG_ATTACK5:
    case EM_DMG_ATTACK5_FLINCH:
        break;
    case EM_DMG_KILLED:
    case EM_DMG_DEAD:
        if (em->stance == EM_STANCE_AIR) {
            EmRathalos_ChangeState(em, EM_GROUP_DEFEAT, RATH_DEFEAT_DIE_FALLING, 3);
        } else if ((s8) em->holdTimer != 0) {
            EmRathalos_ChangeState(em, EM_GROUP_DEFEAT, RATH_DEFEAT_DIE_HELD, 3);
        } else {
            EmRathalos_ChangeState(em, EM_GROUP_DEFEAT, RATH_DEFEAT_DIE, 3);
        }
        break;
    case EM_DMG_ATTACK9:
    case EM_DMG_ATTACK6:
        if ((s8) em->holdTimer == 0) {
            Em_SetGoal12Dist(em, buf);
            Em_SetGoal12(em);
            EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_TRAPPED, 3);
        }
        break;
    case EM_DMG_STATUS_IN_AIR:
        if (em->stateGroup != EM_GROUP_REACT || em->state != RATH_REACT_FALL) {
            EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_FALL, 3);
        }
        break;
    case EM_DMG_STATUS4:
        if ((s8) em->holdTimer != 0) {
            Em_SetStatus4Timer(em);
            EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_STATUS4_DOWNED, 3);
        } else if ((em->stateGroup != EM_GROUP_REACT || em->state != RATH_REACT_STATUS4) && (em->stateGroup != EM_GROUP_REACT || em->state != RATH_REACT_FALL)) {
            Em_SetStatus4Timer(em);
            EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_STATUS4, 3);
        }
        break;
    case EM_DMG_CAPTURED:
        if (Em_IsHeld(em) == 1) {
            Em_SetStatus1Timer(em);
            if ((s8) em->holdTimer != 0) {
                EmRathalos_ChangeState(em, EM_GROUP_EVENT, RATH_EVENT_CAPTURED, 3);
                break;
            }
            if ((s8) em->holdTimer2 != 0) {
                EmRathalos_ChangeState(em, EM_GROUP_EVENT, RATH_EVENT_CAPTURED_HOLD2, 3);
                break;
            }
        }
        if ((em->stateGroup != EM_GROUP_IDLE || em->state != RATH_IDLE_STATUS1_COLLAPSE) && (em->stateGroup != EM_GROUP_REACT || em->state != RATH_REACT_FALL)) {
            Em_SetStatus1Timer(em);
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE_STATUS1_COLLAPSE, 3);
        }
        break;
    case EM_DMG_STATUS2:
        if ((s8) em->holdTimer != 0) {
            Em_SetStatus2Timer(em);
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE_STATUS2_DOWNED, 3);
        } else if ((s8) em->holdTimer2 != 0) {
            Em_SetStatus2Timer(em);
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE_STATUS2_COLLAPSE, 3);
        } else if ((em->stateGroup != EM_GROUP_IDLE || em->state != RATH_IDLE_STATUS2_COLLAPSE) && (em->stateGroup != EM_GROUP_REACT || em->state != RATH_REACT_FALL)) {
            Em_SetStatus2Timer(em);
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE_STATUS2_COLLAPSE, 3);
        }
        break;
    case EM_DMG_HIT_LYING:
        switch (em->state) {
        case RATH_IDLE_REST:
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE_REST_END, 3);
            em->scriptPending = 0;
            Em_CheckRageByHp(em, em->rageThreshold);
            break;
        case RATH_IDLE_STATUS2_COLLAPSE:
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE_STATUS2_END, 3);
            em->scriptPending = 0;
            break;
        case RATH_IDLE_STATUS1_COLLAPSE:
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE_STATUS1_END, 3);
            em->scriptPending = 0;
            break;
        case RATH_IDLE_STATUS2_DOWNED:
            EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_STATUS2_DOWNED_END, 3);
            em->scriptPending = 0;
            break;
        case RATH_IDLE_STATUS1_DOWNED:
            EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_STATUS1_DOWNED_END, 3);
            em->scriptPending = 0;
            break;
        }
        break;
    case EM_DMG_PART8_DOWN:
        EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_PART_BREAK, 3);
        break;
    case EM_DMG_PART_FLINCH:
        if (em->stance == EM_STANCE_AIR) {
            EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_FALL, 3);
        } else {
            switch (em->flinchPart) {
            case 0:
            case 7:
                EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_FLINCH, 3);
                break;
            case 5:
            case 6:
                EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_FLINCH_C, 3);
                break;
            case 1:
            case 2:
                EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_FLINCH_D, 3);
                break;
            case 3:
            case 4:
            default:
                if (em->parts[em->flinchPart].state >= 2) {
                    if (em->flinchPart != 3) {
                        EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_KNOCK_DOWN, 3);
                    } else {
                        EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_KNOCK_DOWN_PART3, 3);
                    }
                } else {
                    EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_FLINCH_B, 3);
                }
                break;
            }
        }
        break;
    case EM_DMG_HIT_MODE3:
        if (em->stance != EM_STANCE_AIR) {
            EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_FLINCH, 3);
            em->scriptPending = 0;
        }
        break;
    case EM_DMG_STATUS5:
        if ((s8) em->holdTimer != 0) {
            Em_SetStatus5Timer(em);
            EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_STUN_DOWNED, 3);
        } else if ((em->stateGroup != EM_GROUP_REACT || em->state != RATH_REACT_STUN) && (em->stateGroup != EM_GROUP_REACT || em->state != RATH_REACT_FALL)) {
            Em_SetStatus5Timer(em);
            EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_STUN, 3);
        }
        break;
    case EM_DMG_ATTACK13:
        Em_ResetKindTimer(em);
        EmRathalos_ChangeState(em, EM_GROUP_REACT, RATH_REACT_HOLD2, 3);
        break;
    }
    switch (gQuest.stage) {
    case 0x434:
    case 0x436:
        if ((em->type == EM_KIND_RATHALOS || em->type == EM_KIND_SILVER_RATHALOS || em->type == EM_KIND_AZURE_RATHALOS) && Area_DayId(em->area) == 0x21 && Save_TestEventFlag(0xAB) == 0) {
            if (Area_DayId(gScene.area) == 0x21 && gScene.eventRunning == 1 && em->stateGroup != EM_GROUP_EVENT) {
                EmRathalos_ChangeState(em, EM_GROUP_EVENT, RATH_EVENT_INTRO, 1);
            }
            break;
        }
        goto normal;
    case 0x3F6:
    case 0x3F8:
        if ((em->type == EM_KIND_RATHIAN || em->type == EM_KIND_GOLD_RATHIAN || em->type == EM_KIND_PINK_RATHIAN) && Area_DayId(em->area) == 0x71 && Save_TestEventFlag(0x9C) == 0) {
            if (Area_DayId(gScene.area) == 0x71 && gScene.eventRunning == 1 && em->stateGroup != EM_GROUP_EVENT) {
                EmHermitaur_ChangeState(em, 6, 6, 1);
            }
            break;
        }
    default:
    normal:
        Em_UpdatePendingScript(em);
        break;
    }
    Em_ClearHitReactMode(em);
    EmRathalos_RunState(em, (s32) w);
    if (em->stateRequest != 0) {
        EmRathalos_RunState(em, (s32) w);
        em->stateRequest = 0;
    }
    if (em->stateGroup == EM_GROUP_FLY && (s8) w->flyGuard == 1) {
        em->guardTimer = 2;
    }
}

/*
 * Runs the current state through its group's dispatcher, keeping the group
 * and state as the previous ones.
 */
void EmRathalos_RunState(Em *em, s32 work)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->stateGroup) {
    case EM_GROUP_IDLE:
        EmRathalos_RunIdleState(em, (EmWork *) work);
        break;
    case EM_GROUP_MOVE:
        EmRathalos_RunMoveState(em, (EmWork *) work);
        break;
    case EM_GROUP_FLY:
        EmRathalos_RunFlyState(em, (EmWork *) work);
        break;
    case EM_GROUP_ATTACK:
        EmRathalos_RunAttackState(em, (EmWork *) work);
        break;
    case EM_GROUP_REACT:
        EmRathalos_RunReactState(em, (EmWork *) work);
        break;
    case EM_GROUP_DEFEAT:
        EmRathalos_RunDefeatState(em, (EmWork *) work);
        break;
    case EM_GROUP_EVENT:
        EmRathalos_RunEventState(em, (EmWork *) work);
        break;
    case EM_GROUP_EVENT2:
        EmRathalos_RunEventState(em, (EmWork *) work);
        break;
    case EM_GROUP_VERSUS:
        EmRathalos_RunVersusState(em, (EmWork *) work);
        
    default:
        break;
    }
}

/* Requests camera shake `level` when the motion passes frame `frame`. */
void EmRathalos_CamShakeOnFrame(void *em, s32 frame, s32 i)
{
    if (Player_PassedFrameScaled((Em *) em, (f32) frame, 0U) != 0) {
        CamShake_ReqAIf(em, i);
    }
}

static void EmRathalos_MotionEffectsDefault(Em *em)
{
}

/*
 * The effects of each motion: plays its sounds (EmRathalos_MotionSounds)
 * and on given frames creates attached effects, ground effects (stage
 * object kind 0x14; more of them while enraged) and camera shakes, at the
 * frames of the motion on layer 0 (work->sndMotion keeps its id).
 */
void EmRathalos_MotionEffects(Em *em, EmFlyerWork *work)
{
    f32 boneOffset[3];

    if (em->motionId[0] != work->sndMotion) {
        work->sndMotion = em->motionId[0];
    }
    EmRathalos_MotionSounds(em, (EmWork *) work);
    switch (work->sndMotion) {
    case EM_MOTION_ID(0, RATH_MOT_WALK):
        EmRathalos_CamShakeOnFrame(em, 0x34, 1);
        EmRathalos_CamShakeOnFrame(em, 0x74, 1);
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
    case EM_MOTION_ID(0, 0x4):
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
    case EM_MOTION_ID(0, RATH_MOT_TURN_RIGHT):
        EmRathalos_CamShakeOnFrame(em, 0x3C, 1);
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
    case EM_MOTION_ID(0, RATH_MOT_TURN_LEFT):
        EmRathalos_CamShakeOnFrame(em, 0x3C, 1);
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
    case EM_MOTION_ID(0, 0x7):
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
    case EM_MOTION_ID(0, 0x8):
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
    case EM_MOTION_ID(0, RATH_MOT_LIMP):
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
    case EM_MOTION_ID(0, RATH_MOT_DESCEND):
        if (Player_PassedFrameScaled(em, 52.0f, 0) || Player_PassedFrameScaled(em, 104.0f, 0) || Player_PassedFrameScaled(em, 162.0f, 0)) {
            if (em->posY <= (1000.0f + em->floorY)) {
                Effect0D_SpawnForEmScaled(em, 2, 7, 5.0f);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, 0xD):
        if (((Player_PassedFrameScaled(em, 26.0f, 0) != 0) || (Player_PassedFrameScaled(em, 52.0f, 0) != 0)) && (em->posY <= (1000.0f + em->floorY))) {
            Effect0D_SpawnForEmScaled(em, 1, 7, 5.0f);
        }
        break;
    case EM_MOTION_ID(0, RATH_MOT_FLY):
    case EM_MOTION_ID(0, 0x55):
    case EM_MOTION_ID(0, 0x56):
    case EM_MOTION_ID(0, 0x58):
    case EM_MOTION_ID(0, 0x59):
        if ((em->rage != 0) && (((s32) gScene.frame % 19) == 0)) {
            StageObj_K14_Create(em, 0x19, ((u16) Rand_Next(1) & 1) + 2, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, RATH_MOT_HOVER):
        if (Player_PassedFrameScaled(em, 8.0f, 0) || Player_PassedFrameScaled(em, 86.0f, 0) || Player_PassedFrameScaled(em, 160.0f, 0)) {
            if (em->posY <= (1000.0f + em->floorY)) {
                StageObj_K14_Create(em, 1, 0, 1.0f);
            }
        }
        if (em->rage != 0) {
            if (((EmMotion_ReachedFrame(em, 0, 70.0f) != 0) && (Player_PassedFrameScaled(em, 84.0f, 0) == 0)) || ((EmMotion_ReachedFrame(em, 0, 146.0f) != 0) && (Player_PassedFrameScaled(em, 156.0f, 0) == 0)) || ((EmMotion_ReachedFrame(em, 0, 220.0f) != 0) && (Player_PassedFrameScaled(em, 234.0f, 0) == 0))) {
                if (!(gScene.frame & 3)) {
                    StageObj_K14_Create(em, 0x1A, (u16) Rand_Next(1) & 1, 1.0f);
                    break;
                }
            }
        }
        break;
    case EM_MOTION_ID(0, RATH_MOT_RUN):
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
    case EM_MOTION_ID(0, RATH_MOT_TAKE_OFF):
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
    case EM_MOTION_ID(0, RATH_MOT_TOUCH_DOWN):
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
    case EM_MOTION_ID(0, 0x1F):
    case EM_MOTION_ID(0, RATH_MOT_LIE):
        boneOffset[1] = 10.0f;
        boneOffset[2] = 140.0f;
        boneOffset[0] = 0.0f;
        Em_SpawnEffect06Periodic(em, 0x22, boneOffset, 1.6f);
        break;
    case EM_MOTION_ID(0, 0x24):
        if (Player_PassedFrameScaled(em, 50.0f, 0) != 0) {
            Eff01_Create(em, 1);
        }
        if (Player_PassedFrameScaled(em, 46.0f, 0) != 0) {
            Eff01_Create(em, 0x29);
        }
        break;
    case EM_MOTION_ID(0, RATH_MOT_COLLAPSE):
        if (Player_PassedFrameScaled(em, 148.0f, 0) != 0) {
            Eff01_Create(em, 0xF);
        }
        if (Player_PassedFrameScaled(em, 156.0f, 0) != 0) {
            StageObj_K14_Create(em, 0, 0, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, RATH_MOT_LEAP):
        if (Player_PassedFrameScaled(em, 60.0f, 0) != 0) {
            Eff01_Create(em, 0x22);
        }
        if ((em->rage != 0) && (EmMotion_ReachedFrame(em, 0, 142.0f) != 0) && (Player_PassedFrameScaled(em, 236.0f, 0) == 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create(em, 0x1A, (u16) Rand_Next(1) & 1, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, RATH_MOT_LEAP_LAND):
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
    case EM_MOTION_ID(0, RATH_MOT_TAIL_SPIN_B):
        EmRathalos_CamShakeOnFrame(em, 0x2C, 1);
        EmRathalos_CamShakeOnFrame(em, 0x49, 1);
        if (Player_PassedFrameScaled(em, 30.0f, 0) != 0) {
            Eff01_Create(em, 0x11);
        }
        if ((em->rage != 0) && (EmMotion_ReachedFrame(em, 0, 6.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 38.0f) == 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create(em, 0x1A, 0, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, RATH_MOT_TAIL_SPIN):
        EmRathalos_CamShakeOnFrame(em, 0x2C, 1);
        EmRathalos_CamShakeOnFrame(em, 0x49, 1);
        if (Player_PassedFrameScaled(em, 30.0f, 0) != 0) {
            Eff01_Create(em, 0x11);
        }
        if ((em->rage != 0) && (EmMotion_ReachedFrame(em, 0, 6.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 38.0f) == 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create(em, 0x1A, 1, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, RATH_MOT_SWOOP_CLAW):
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
    case EM_MOTION_ID(0, RATH_MOT_AIR_FIREBALL_RISE):
        if ((Player_PassedFrameScaled(em, 92.0f, 0) != 0) && (em->posY <= (1000.0f + em->floorY))) {
            StageObj_K14_Create(em, 1, 0, 1.0f);
        }
        if ((em->rage != 0) && (EmMotion_ReachedFrame(em, 0, 60.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 132.0f) == 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create(em, 0x1A, (u16) Rand_Next(1) & 1, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, RATH_MOT_ALERT_IDLE):
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
    case EM_MOTION_ID(0, RATH_MOT_BACK_HOP):
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
    case EM_MOTION_ID(0, RATH_MOT_AIR_DIVE):
        if (Player_PassedFrameScaled(em, 90.0f, 0) != 0) {
            Eff01_Create(em, 0x16);
        }
        if (Player_PassedFrameScaled(em, 108.0f, 0) != 0) {
            Eff01_Create(em, 0x17);
        }
        break;
    case EM_MOTION_ID(0, RATH_MOT_TURN_IN_PLACE):
        if (Player_PassedFrameScaled(em, 102.0f, 0) != 0) {
            Eff01_Create(em, 0x12);
        }
        if (Player_PassedFrameScaled(em, 110.0f, 0) != 0) {
            Eff01_Create(em, 0x13);
            StageObj_K0F_CreateOnEm(em, 5, 4, 1.0f);
        }
        if (em->rage != 0) {
            if (((EmMotion_ReachedFrame(em, 0, 20.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 36.0f) == 0)) || ((EmMotion_ReachedFrame(em, 0, 46.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 168.0f) == 0)) || ((EmMotion_ReachedFrame(em, 0, 172.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 192.0f) == 0)) || ((EmMotion_ReachedFrame(em, 0, 196.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 216.0f) == 0)) || ((EmMotion_ReachedFrame(em, 0, 230.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 238.0f) == 0))) {
                if (!(gScene.frame & 3)) {
                    StageObj_K14_Create(em, 0x1A, (u16) Rand_Next(1) & 1, 1.0f);
                    break;
                }
            }
        }
        break;
    case EM_MOTION_ID(0, RATH_MOT_BACK_HOP_LAND):
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
    case EM_MOTION_ID(0, 0x3C):
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
    case EM_MOTION_ID(0, 0x3D):
        if (Player_PassedFrameScaled(em, 24.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x1A, 7, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, 0x41):
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
    case EM_MOTION_ID(0, RATH_MOT_DIE):
        if (Player_PassedFrameScaled(em, 268.0f, 0) != 0) {
            StageObj_K14_Create(em, 0, 0, 0.9f);
        }
        break;
    case EM_MOTION_ID(0, RATH_MOT_KNOCK_DOWN):
        EmRathalos_CamShakeOnFrame(em, 0x1A, 2);
        if (Player_PassedFrameScaled(em, 26.0f, 0) != 0) {
            StageObj_K14_Create(em, 0x12, 0x80, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, 0x49):
        EmRathalos_CamShakeOnFrame(em, 0x1A, 2);
        if ((EmMotion_ReachedFrame(em, 0, 28.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 52.0f) == 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create(em, 0xA, 0x80, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, 0x4A):
        EmRathalos_CamShakeOnFrame(em, 0x1A, 2);
        if (Player_PassedFrameScaled(em, 26.0f, 0) != 0) {
            StageObj_K14_Create(em, 0x12, 0x81, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, RATH_MOT_FALL_LAND):
        if (Player_PassedFrameScaled(em, 8.0f, 0) != 0) {
            StageObj_K14_Create(em, 0xB, 0, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, RATH_MOT_HEAD_DOWN):
        if (Player_PassedFrameScaled(em, 48.0f, 0) || Player_PassedFrameScaled(em, 106.0f, 0) || Player_PassedFrameScaled(em, 174.0f, 0)) {
            Eff01_Create(em, 0x10);
        }
        break;
    case EM_MOTION_ID(0, 0x54):
        if (Player_PassedFrameScaled(em, 38.0f, 0) || Player_PassedFrameScaled(em, 60.0f, 0) || Player_PassedFrameScaled(em, 78.0f, 0)) {
            Effect0D_SpawnForEmScaled(em, 2, 7, 5.0f);
        }
        break;
    case EM_MOTION_ID(0, RATH_MOT_LEVEL_OUT):
        if ((Player_PassedFrameScaled(em, 50.0f, 0) != 0) && (em->posY <= (1000.0f + em->floorY))) {
            StageObj_K14_Create(em, 9, 0, 1.0f);
            StageObj_K14_Create(em, 0xE, 0, 1.0f);
        }
        if ((Player_PassedFrameScaled(em, 102.0f, 0) != 0) && (em->posY <= (1000.0f + em->floorY))) {
            StageObj_K14_Create(em, 9, 0, 1.0f);
            StageObj_K14_Create(em, 0xE, 0, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, RATH_MOT_CHARGE_TRIP):
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
    case EM_MOTION_ID(0, RATH_MOT_FLIP):
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
    case EM_MOTION_ID(0, RATH_MOT_FLIP_LAND):
        if (Player_PassedFrameScaled(em, 2.0f, 0) != 0) {
            Eff01_Create(em, 0x1F);
        }
        break;
    case EM_MOTION_ID(0, RATH_MOT_TRAPPED):
        if (Player_PassedFrameScaled(em, 10.0f, 0) != 0) {
            StageObj_K14_Create(em, 0xB, 0x80, 1.0f);
        }
        if ((em->rage != 0) && (EmMotion_ReachedFrame(em, 0, 4.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 60.0f) == 0) && !(gScene.frame & 3)) {
            StageObj_K14_Create(em, 0x1A, (u16) Rand_Next(1) & 1, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, RATH_MOT_STRUGGLE):
        if (em->rage != 0) {
            if (((EmMotion_ReachedFrame(em, 0, 12.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 24.0f) == 0)) || (EmMotion_ReachedFrame(em, 0, 46.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 90.0f) == 0)) {
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
    case EM_MOTION_ID(0, RATH_MOT_IDLE):
    case EM_MOTION_ID(0, RATH_MOT_RUN_STOP):
    case EM_MOTION_ID(0, RATH_MOT_GLIDE):
    case EM_MOTION_ID(0, 0x10):
    case EM_MOTION_ID(0, 0x1D):
    case EM_MOTION_ID(0, 0x1E):
    case EM_MOTION_ID(0, RATH_MOT_ROAR):
    case EM_MOTION_ID(0, RATH_MOT_GET_UP):
    case EM_MOTION_ID(0, RATH_MOT_LIE_DOWN):
    case EM_MOTION_ID(0, RATH_MOT_SWOOP_DIVE):
    case EM_MOTION_ID(0, RATH_MOT_AIR_FIREBALL):
    case EM_MOTION_ID(0, RATH_MOT_FIREBALL):
    case EM_MOTION_ID(0, 0x30):
    case EM_MOTION_ID(0, RATH_MOT_FIREBALL_B):
    case EM_MOTION_ID(0, RATH_MOT_FEED):
    case EM_MOTION_ID(0, RATH_MOT_SHAKE_OFF):
    case EM_MOTION_ID(0, 0x3F):
    case EM_MOTION_ID(0, 0x40):
    case EM_MOTION_ID(0, 0x42):
    case EM_MOTION_ID(0, 0x43):
    case EM_MOTION_ID(0, RATH_MOT_DOWN):
    case EM_MOTION_ID(0, RATH_MOT_DOWN_GET_UP):
    case EM_MOTION_ID(0, 0x48):
    case EM_MOTION_ID(0, 0x4B):
    case EM_MOTION_ID(0, 0x4C):
    case EM_MOTION_ID(0, RATH_MOT_FALL):
    case EM_MOTION_ID(0, 0x50):
    case EM_MOTION_ID(0, 0x51):
    case EM_MOTION_ID(0, RATH_MOT_DIE_FALLING):
    case EM_MOTION_ID(0, 0x53):
    case EM_MOTION_ID(0, RATH_MOT_SWOOP):
    case EM_MOTION_ID(0, RATH_MOT_FLIP_FALL):
    case EM_MOTION_ID(0, RATH_MOT_DOWNED):
    case EM_MOTION_ID(0, RATH_MOT_DOWNED_GET_UP):
        break;
    }
}

/*
 * Runs EmRathalos_MotionEffects once the work area's step is set (the first
 * frame only sets it).
 */
void EmRathalos_UpdateMotionEffects(Em *em)
{
    EmFlyerWork *w = (EmFlyerWork *)&em->work;

    switch (w->fxStep) {
    case 0:
        w->fxStep++;
        break;
    case 1:
        EmRathalos_MotionEffects(em, w);
        break;
    }
}

/* Creates the ground effect of kind 0x14 type 0xB (made at the landings). */
void EmRathalos_SpawnK14Type0B(Em *em)
{
    StageObj_K14_Create((Em *) em, 0xB, 0, 1.0f);
}

/* Creates the ground effect of kind 0x14 type 0xA every fifth frame. */
void EmRathalos_SpawnK14Type0AEvery5(Em *em)
{
    if (((s32) gScene.frame % 5) == 0) {
        StageObj_K14_Create((Em *) em, 0xA, 0, 1.0f);
    }
}

/*
 * During a swoop: 1 once the claw pass runs; starts it
 * (RATH_MOT_SWOOP_CLAW) when the target is within 22 frames' flight. 0
 * without a target.
 */
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
    if (em->motionId[0] == EM_MOTION_ID(0, RATH_MOT_SWOOP_CLAW)) {
        return 1;
    }
    if (dist <= (4.5f + (30.0f * em->velZ))) {
        Em_SetMotion(em, RATH_MOT_SWOOP_CLAW, 0xA, 0);
        return 1;
    }
    return 0;
}

/* As EmRathalos_ReachedTarget, measured to the goal (the intro's swoop). */
s32 EmRathalos_ReachedPoint(Em *em)
{
    f32 dist;

    dist = Vec_Distance((f32 *) &em->posX, (f32 *) em->goal);
    if (em->motionId[0] == EM_MOTION_ID(0, RATH_MOT_SWOOP_CLAW)) {
        return 1;
    }
    if (dist <= (2.4199998f + (22.0f * em->velZ))) {
        Em_SetMotion(em, RATH_MOT_SWOOP_CLAW, 0xA, 0);
        return 1;
    }
    return 0;
}

/* Ends a swoop: step 99, then a glide once (EmRathalos_EndAirState). */
void EmRathalos_EndSwoop(Em *em, s32 unused)
{
    em->step = 99;
    EmRathalos_EndAirState(em, 1);
}

void EmRathalos_TableNop(void)
{
}

/*
 * Whether the once-only reaction (RATH_REACT_PART_BREAK) may run: not in
 * the trapped, downed or downed-end states, not in the air, and not after
 * it ran once.
 */
s32 EmRathalos_AllowPartBreak(Em *em)
{
    u8 stateGroup;

    stateGroup = em->stateGroup;
    if ((stateGroup != EM_GROUP_REACT || em->state != RATH_REACT_TRAPPED)
        && (stateGroup != EM_GROUP_REACT || em->state != RATH_REACT_TRAPPED_STRUGGLE)
        && (stateGroup != EM_GROUP_REACT || em->state != RATH_REACT_STATUS4_DOWNED)
        && (stateGroup != EM_GROUP_REACT || em->state != RATH_REACT_STATUS4_DOWNED_END)
        && (stateGroup != EM_GROUP_REACT || em->state != RATH_REACT_STATUS2_DOWNED_END)
        && (stateGroup != EM_GROUP_REACT || em->state != RATH_REACT_STATUS1_DOWNED_END)
        && em->stance != EM_STANCE_AIR && (s8) em->hit8Count == 0) {
        return 1;
    }
    return 0;
}

/*
 * Starts a group-0 state: the alert idle instead of the idle in combat, and
 * the statuses and needs each state begins or ends.
 */
void EmRathalos_SetIdleState(Em *em, u16 state, u8 mode)
{
    em->motionSpeed = 1.0f;
    switch (state) {
    case RATH_IDLE:
        if (em->combat == 1) {
            state = RATH_IDLE_ALERT;
        }
        break;
    case RATH_IDLE_REST:
        Em_ResetNotice(em);
        break;
    case RATH_IDLE_STATUS2_COLLAPSE:
        Em_BeginStatus2(em);
        break;
    case RATH_IDLE_STATUS2_LIE_DOWN:
        Em_BeginStatus2(em);
        break;
    case RATH_IDLE_REST_END:
        Em_RefillNeed3(em);
        break;
    case RATH_IDLE_STATUS2_END:
        Em_EndStatus2(em);
        break;
    case RATH_IDLE_STATUS1_COLLAPSE:
        Em_BeginStatus1(em);
        break;
    case RATH_IDLE_STATUS1_END:
        Em_EndStatus1(em);
        break;
    case RATH_IDLE_STATUS2_DOWNED:
        Em_BeginStatus2(em);
        break;
    case RATH_IDLE_STATUS1_DOWNED:
        Em_BeginStatus1(em);
        break;
    case RATH_IDLE_STATUS4_SHAKE_OFF:
        Em_EndStatus4(em);
        break;
    case RATH_IDLE_STUN_SHAKE_OFF:
        Em_EndStatus5(em);
        break;
    case RATH_IDLE_HOLD2_SHAKE_OFF:
        em->holdTimer2 = 0;
        break;
    case RATH_IDLE_EAT:
        Em_StartGoalAction(em, 0x2710);
        break;
    }
    em->stance = EM_STANCE_GROUND;
    Em_SetStateSync(em, EM_GROUP_IDLE, state, mode);
}

/*
 * Starts a group-1 state: the limp below the low-hp ratio, the run in
 * combat, the speed and the walk's goal and timer.
 */
void EmRathalos_SetMoveState(Em *em, u16 state, u8 mode)
{
    f32 lowHpRatio;
    f32 dist;

    em->stance = EM_STANCE_GROUND;
    em->motionSpeed = 1.0f;
    switch (state) {
    case RATH_MOVE_WALK:
    case RATH_MOVE_END2:
        if (em->type == EM_KIND_RATHIAN || em->type == EM_KIND_GOLD_RATHIAN || em->type == EM_KIND_PINK_RATHIAN) {
            lowHpRatio = 0.1f;
        } else {
            lowHpRatio = 0.3f;
        }
        /* low on hp it limps, in combat it runs: the gotos join those cases */
        if (em->hp < (s16) (em->hpMax * lowHpRatio)) {
            state = RATH_MOVE_LIMP;
            goto state6;
        }
        if (state == RATH_MOVE_END2 && em->combat == 1) {
            state = RATH_MOVE_RUN;
            goto state4;
        }
        if (state == RATH_MOVE_END2) {
            state = RATH_MOVE_WALK;
        }
        Em_SetSpeedScaled(em, 0, 1000.0f, 500.0f);
        break;
    case RATH_MOVE_END:
        em->scriptPending = 1;
        return;
    case RATH_MOVE_TURN:
        if (em->combat == 1) {
            state = RATH_MOVE_TURN_IN_COMBAT;
        }
        break;
    case RATH_MOVE_RUN_SLOW:
        em->motionSpeed = 0.7f;
    case RATH_MOVE_TURN_RAGE:
        Em_StartRage(em);
        break;
    case RATH_MOVE_TURN_PENDING_RAGE:
        Em_StartPendingRage(em);
        break;
    case RATH_MOVE_RUN:
    state4:
        Em_SetSpeedScaled(em, 2, 1000.0f, 500.0f);
        break;
    case RATH_MOVE_LIMP:
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
    case RATH_MOVE_TURN_IN_COMBAT:
    case RATH_MOVE_TURN_TO_GOAL:
        break;
    }
    Em_SetStateSync(em, EM_GROUP_MOVE, state, mode);
}

/*
 * Starts a group-2 state: the goals and timers of the flights (routes,
 * exits, landing points, the area sequence's point).
 */
void EmRathalos_SetFlyState(Em *em, u16 state, u8 mode)
{
    EmFlyerWork *w = (EmFlyerWork *)&em->work;

    em->motionSpeed = 1.0f;
    switch (state) {
    case RATH_FLY_LAND:
        if (em->remote == 0 && !(em->velZ < 50.0f)) {
            state = RATH_FLY_LEVEL_OUT_AND_LAND;
        }
        break;
    case RATH_FLY_ROUTE:
    case RATH_FLY_ROUTE_B:
        w->legsLeft = 4;
        if (em->remote == 0) {
            em->goalNo = em->stateRand & 3;
            em->goalKind = 2;
            em->goalArg = 2;
            Em_ComputeGoal(em, em->goal);
            em->timer = 1.5f * Vec_DistanceXZ(&em->posX, em->goal) / 50.0f;
        }
        break;
    case RATH_FLY_HOVER_TO_GOAL:
    case RATH_FLY_HOVER_STRAIGHT_TO_GOAL:
        Player_ClearVelocity(em);
        em->velZ = 20.0f;
        em->timer = (s32) (Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) / em->velZ) * 3;
        break;
    case RATH_FLY_INTO_AREA:
        Em_TypeSetup(em);
    case RATH_FLY_OUT_OF_AREA:
        if (em->remote == 0 && em->curGoalKind != 3) {
            EmRathalos_ChangeState(em, EM_GROUP_FLY, RATH_FLY_LAND, 1);
            return;
        }
        break;
    case RATH_FLY_TO_GOAL_AND_LAND:
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
    case RATH_FLY_GLIDE_TO_GOAL:
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
    case RATH_FLY_GLIDE_TO_GOAL_B:
        w->legsLeft = 1;
        if (em->remote == 0) {
            em->timer = 1.5f * Vec_DistanceXZ(&em->posX, em->goal) / 50.0f;
        }
        break;
    case RATH_FLY_HOVER_RISE:
        if (em->velZ <= 0.0f) {
            em->velZ = 20.0f;
        }
        if (em->remote == 0) {
            em->timer = Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) / em->velZ;
        }
        break;
    case RATH_FLY_TAKE_OFF:
    case RATH_FLY_HOVER:
    case RATH_FLY_HOP_TO_GOAL:
    case RATH_FLY_HOP_FORWARD:
    case RATH_FLY_HOP_BACK:
    case RATH_FLY_OUT_OF_AREA_ALT:
    case RATH_FLY_BACK_HOP:
    case RATH_FLY_HOVER_ONCE:
    case RATH_FLY_GLIDE_ONCE:
    case RATH_FLY_HOVER_TURN:
    case RATH_FLY_LEVEL_OUT_AND_LAND:
    case RATH_FLY_END:
    case RATH_FLY_LEVEL_OUT:
    case RATH_FLY_ESCAPE_UPWARD:
        break;
    }
    Em_SetStateSync(em, EM_GROUP_FLY, state, mode);
}

/*
 * Starts a group-3 state: the target and goal, the fireball's variant by
 * the target's height, the swoops' escape point, the repeat counts.
 */
void EmRathalos_SetAttackState(Em *em, u16 state, u8 mode)
{
    EmFlyerWork *w = (EmFlyerWork *)&em->work;
    f32 targetPos[3];
    f32 pos[3];
    f32 distOrHeight;

    em->motionSpeed = 1.0f;
    switch (state) {
    case RATH_ATK_TURN_AND_STARE:
    case RATH_ATK_TURN_AND_STARE_B:
    case RATH_ATK_LIE_AND_RISE:
        Em_SetTargetAngle(em);
        break;
    case RATH_ATK_TAIL_SPIN:
        if (em->remote == 0 && (Game_IsOnline() == 0 || mode != 0) && (em->stateRand & 1)) {
            state = RATH_ATK_TAIL_SPIN_B;
        }
        break;
    case RATH_ATK_LEAP:
        Em_SetTargetAngle(em);
        if (em->targetNo == -1) {
            em->goalDist = 2500.0f;
            break;
        }
        em->target = &gPlayers[em->targetNo];
        Em_GetTargetPos(em, em->targetNo, targetPos);
        Vec_Set(em->goal, targetPos[0], targetPos[1], targetPos[2]);
        distOrHeight = Vec_DistanceXZ(&em->posX, em->goal);
        em->goalDist = distOrHeight;
        if (distOrHeight > 5000.0f) {
            em->goalDist = 5000.0f;
        } else if (distOrHeight < 2500.0f) {
            em->goalDist = 2500.0f;
        }
        break;
    case RATH_ATK_FIREBALL:
        if (em->remote == 0 && (Game_IsOnline() == 0 || mode != 0) && em->curGoalKind == 1 && em->curGoalArg == 0) {
            if (em->targetNo == -1) {
                w->countdown = 1;
                Em_SetTargetAngle(em);
                break;
            }
            Em_GetTargetPos(em, em->targetNo, pos);
            distOrHeight = em->playerDistXZ[em->targetNo];
            if (distOrHeight <= 1000.0f) {
                distOrHeight = pos[1];
                if (distOrHeight <= em->posY + 50.0f) {
                    state = RATH_ATK_FIREBALL_LOW;
                } else if (distOrHeight <= em->posY + 150.0f) {
                    state = RATH_ATK_FIREBALL;
                } else {
                    state = RATH_ATK_FIREBALL_HIGH;
                }
            } else if (distOrHeight <= 2000.0f) {
                distOrHeight = pos[1];
                if (distOrHeight <= em->posY - 150.0f) {
                    state = RATH_ATK_FIREBALL_LOW;
                } else if (distOrHeight <= em->posY + 300.0f) {
                    state = RATH_ATK_FIREBALL;
                } else {
                    state = RATH_ATK_FIREBALL_HIGH;
                }
            } else {
                distOrHeight = pos[1];
                if (distOrHeight <= em->posY - 200.0f) {
                    state = RATH_ATK_FIREBALL_LOW;
                } else if (distOrHeight <= em->posY + 400.0f) {
                    state = RATH_ATK_FIREBALL;
                } else {
                    state = RATH_ATK_FIREBALL_HIGH;
                }
            }
        }
    case RATH_ATK_FIREBALL_B:
    case RATH_ATK_FIREBALL_LOW:
    case RATH_ATK_FIREBALL_HIGH:
        w->countdown = 1;
        Em_SetTargetAngle(em);
        break;
    case RATH_ATK_SWOOP_THEN_LEAVE:
    case RATH_ATK_SWOOP_THEN_LAND:
        if (em->remote == 0) {
            EmRathalos_SetEscapePoint(em);
            em->timer = 1.5f * Vec_DistanceXZ(&em->posX, em->goal) / 50.0f;
        }
        break;
    case RATH_ATK_AIR_FIREBALL_B:
    case RATH_ATK_AIR_FIREBALL_DOUBLE:
    case RATH_ATK_AIR_FIREBALL_DOUBLE_B:
    case RATH_ATK_AIR_FIREBALL_DOUBLE_FORWARD:
        w->countdown = em->stateRand % 2 + 1;
    case RATH_ATK_AIR_FIREBALL:
    case RATH_ATK_AIR_FIREBALL_FORWARD:
        if (em->targetNo == -1) {
            em->timer = 200;
            EmRathalos_ChangeState(em, EM_GROUP_FLY, RATH_FLY_HOVER, 1);
            return;
        }
        break;
    case RATH_ATK_AIR_DIVE:
        if (em->remote == 0) {
            if (em->targetNo == -1) {
                em->timer = 200;
                EmRathalos_ChangeState(em, EM_GROUP_FLY, RATH_FLY_HOVER, 1);
                return;
            }
            em->target = &gPlayers[em->targetNo];
        } else {
            em->target = &gPlayers[em->targetNo];
        }
        break;
    case RATH_ATK_CHARGE_TRIP:
    case RATH_ATK_RUN_AND_STOP:
        if (em->targetNo == -1) {
            em->timer = 150;
            em->hasGoal = 0;
        } else {
            em->hasGoal = 1;
            distOrHeight = Vec_DistanceXZ(&em->posX, em->goal);
            em->goalDist = distOrHeight - 500.0f;
            em->timer = (s32) (distOrHeight / 30.0f) + 30;
        }
        if (em->timer < 30) {
            em->timer = 30;
        }
        break;
    case RATH_ATK_CHARGE:
        if (em->targetNo == -1) {
            em->timer = 150;
            em->hasGoal = 0;
        } else {
            em->hasGoal = 1;
            distOrHeight = Vec_DistanceXZ(&em->posX, em->goal) + 700.0f;
            em->goalDist = distOrHeight;
            em->timer = (s32) (distOrHeight / 30.0f) + 30;
        }
        break;
    case RATH_ATK_BACK_HOP_FIREBALL:
    case RATH_ATK_BACK_HOP_FIREBALL_LEFT:
    case RATH_ATK_BACK_HOP_FIREBALL_RIGHT:
        if (em->targetNo == -1) {
            EmRathalos_ChangeState(em, EM_GROUP_IDLE, RATH_IDLE, 1);
            return;
        }
        em->target = &gPlayers[em->targetNo];
        break;
    case RATH_ATK_STARE:
    case RATH_ATK_ROAR:
    case RATH_ATK_ROAR_AT_POINT:
    case RATH_ATK_SINGLE_A:
    case RATH_ATK_SINGLE_B:
    case RATH_ATK_ROAR_B:
    case RATH_ATK_SOMERSAULT:
    case RATH_ATK_TAIL_SPIN_B:
    case RATH_ATK_SOMERSAULT_DOUBLE:
        break;
    }
    Em_SetStateSync(em, EM_GROUP_ATTACK, state, mode);
}

/*
 * Changes the state. On the ground, flight states that need the air become
 * RATH_MOVE_WALK; group 0 to 3 states go through their Set function, the
 * rest straight to Em_SetStateSync, with the statuses and part breaks group
 * 4's states begin or end. Remote copies skip modes 3 and 4.
 */
void EmRathalos_ChangeState(Em *em, u16 group, u16 state, u8 mode)
{
    if (em->remote == 0) {
        Em_TargetPlayerInRange(em);
        if (em->stance == EM_STANCE_GROUND) {
            switch (group) {
            case EM_GROUP_FLY:
                switch (state) {
                case RATH_FLY_LAND:
                case RATH_FLY_HOVER:
                case RATH_FLY_ROUTE:
                case RATH_FLY_HOVER_TO_GOAL:
                case RATH_FLY_OUT_OF_AREA:
                case RATH_FLY_TO_GOAL_AND_LAND:
                case RATH_FLY_OUT_OF_AREA_ALT:
                case RATH_FLY_GLIDE_TO_GOAL:
                case RATH_FLY_INTO_AREA:
                case RATH_FLY_HOVER_ONCE:
                case RATH_FLY_GLIDE_ONCE:
                case RATH_FLY_HOVER_TURN:
                case RATH_FLY_ROUTE_B:
                case RATH_FLY_LEVEL_OUT_AND_LAND:
                case RATH_FLY_GLIDE_TO_GOAL_B:
                case RATH_FLY_HOVER_STRAIGHT_TO_GOAL:
                case RATH_FLY_LEVEL_OUT:
                case RATH_FLY_HOVER_RISE:
                    state = RATH_MOVE_WALK;
                    group = EM_GROUP_MOVE;
                    break;
                case RATH_FLY_TAKE_OFF:
                case RATH_FLY_HOP_TO_GOAL:
                case RATH_FLY_HOP_FORWARD:
                case RATH_FLY_HOP_BACK:
                case RATH_FLY_BACK_HOP:
                case RATH_FLY_END:
                case RATH_FLY_ESCAPE_UPWARD:
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
    case EM_GROUP_IDLE:
        EmRathalos_SetIdleState(em, state, mode);
        break;
    case EM_GROUP_MOVE:
        EmRathalos_SetMoveState(em, state, mode);
        break;
    case EM_GROUP_FLY:
        EmRathalos_SetFlyState(em, state, mode);
        break;
    case EM_GROUP_ATTACK:
        EmRathalos_SetAttackState(em, state, mode);
        break;
    case EM_GROUP_VERSUS:
        if (state == RATH_VS_WAIT_BOTH) {
            Versus_MarkDefeat(em);
        }
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        break;
    case EM_GROUP_DEFEAT:
        if (state == RATH_DEFEAT_REAPPEAR && QuestEm_CanAppear(em) == 1) {
            QuestEm_Appear(em);
        }
    case EM_GROUP_EVENT:
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        break;
    case EM_GROUP_REACT:
        switch (state) {
        case RATH_REACT_PART_BREAK:
            Em_EndAllStatus(em);
            em->fxFlags |= 1;
            em->hit8Count++;
            break;
        case RATH_REACT_STATUS4:
            Em_BeginStatus4(em);
            break;
        case RATH_REACT_TRAPPED:
            em->holdTimer = 50;
            Em_EndAllStatus(em);
            break;
        case RATH_REACT_STATUS4_DOWNED:
            Em_BeginStatus4(em);
            break;
        case RATH_REACT_PART_BREAK_END:
            Em_ResetToBasePose(em);
            Em_BreakPart(em, 3);
            break;
        case RATH_REACT_STATUS4_DOWNED_END:
            Em_EndStatus4(em);
            break;
        case RATH_REACT_STATUS2_DOWNED_END:
            Em_EndStatus2(em);
            break;
        case RATH_REACT_STATUS1_DOWNED_END:
            Em_EndStatus1(em);
            break;
        case RATH_REACT_STUN:
            Em_BeginStatus5(em);
            break;
        case RATH_REACT_STUN_DOWNED:
            Em_BeginStatus5(em);
            break;
        case RATH_REACT_STUN_DOWNED_END:
            Em_EndStatus5(em);
            break;
        case RATH_REACT_HOLD2:
            Em_EndAllStatus(em);
            break;
        }
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        break;
    case EM_GROUP_EVENT2:
        break;
    }
}

/*
 * Sets the goal to the escape point of a swoop: of the area's four points
 * (Em_AreaEntryValue's table 2), the one farthest from the target; (1000,
 * 0, 1000) without points. Without a target it flies the route instead.
 */
void EmRathalos_SetEscapePoint(Em *em)
{
    f32 pos[3];
    f32 dist[4];
    f32 (*pts)[3];
    u32 i;
    f32 (*p)[3];
    f32 best;
    f32 candidate;
    u8 k;
    f32 *sel;

    if (em->targetNo == -1) {
        EmRathalos_ChangeState(em, EM_GROUP_FLY, RATH_FLY_ROUTE, 1);
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
    candidate = dist[1];
    if (best < candidate) {
        best = candidate;
        k = 1;
    }
    candidate = dist[2];
    if (best < candidate) {
        best = candidate;
        k = 2;
    }
    candidate = dist[3];
    if (best < candidate) {
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

/*
 * Steps rotX by the work area's pitchStep toward the pitch of `mode`: 0 at
 * the target's position raised by `height`, 1 nose up (0xD000), 2 level.
 */
void EmRathalos_StepPitch(Em *em, s32 mode, f32 height)
{
    EmWork *work = (EmWork *) &em->work;
    f32 pos[3];
    s32 pitchDelta;

    if (em->targetNo == -1) {
        return;
    }
    switch (mode) {
    case 0:
        Em_GetTargetPos(em, em->targetNo, pos);
        pitchDelta = 0x10000 - Math_Heading(Vec_Distance((f32 *) &em->posX, (f32 *) pos), em->posY, 0.0f, pos[1] + height);
        pitchDelta = (pitchDelta - em->rotX) & 0xFFFF;
        break;
    case 1:
        pitchDelta = (0xD000 - em->rotX) & 0xFFFF;
        break;
    case 2:
        pitchDelta = -em->rotX & 0xFFFF;
        break;
    }
    if (pitchDelta == 0) {
        return;
    }
    if (pitchDelta < 0x8000) {
        if (em->rotX < 0x3000 || em->rotX >= 0xD000) {
            if (pitchDelta <= work->pitchStep) {
                em->rotX += pitchDelta;
            } else {
                em->rotX += work->pitchStep;
            }
        }
    } else {
        if (em->rotX > 0xD000 || em->rotX <= 0x3000) {
            if (pitchDelta >= 0x10000 - work->pitchStep) {
                em->rotX -= 0x10000 - pitchDelta;
            } else {
                em->rotX -= work->pitchStep;
            }
        }
    }
    em->rotX = (u16) em->rotX;
}

/*
 * Sets velY from vertical-speed curve `kind` (EmRathalos_MoveVelYCurves) at
 * the motion's frame, while the motion is not blending.
 */
void EmRathalos_MoveByVelYCurve(Em *em, s32 kind)
{
    EmWork *work = (EmWork *) &em->work;
    EmCurveKey *tbl;
    EmCurveKey *key;
    f32 frame;
    f32 keyFrame;
    s32 i;

    tbl = EmRathalos_MoveVelYCurves[kind];
    if (tbl != NULL && em->motion[0].blend == 0) {
        frame = em->motion[0].frame;
        if (frame == 0.0f || frame == 1.0f) {
            em->velY = tbl->v;
        } else {
            i = 1;
            while (i != 0) {
                key = &tbl[i];
                keyFrame = key->t;
                if (frame > keyFrame && keyFrame != 0.0f) {
                    i++;
                } else if (keyFrame == 0.0f) {
                    break;
                } else {
                    i = 0;
                    em->velY = (key->v - key[-1].v) / ((keyFrame - key[-1].t) / em->motion[0].step);
                }
            }
        }
        work->moveAngX = 0;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
    }
}

/*
 * Starts curve set `index` of the family's movement curves (vertical speed
 * and forward speed).
 */
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

/* The curve step of a curve set without curves: does nothing and returns 0. */
static s32 EmRathalos_CurveNone(Em *em, EmWork *work)
{
    return 0;
}


extern EmCurveKey *EmRathalos_VelYCurves[];

/*
 * The vertical-speed curve (EmRathalos_VelYCurves[curveKind]): sets velY
 * from the key pair around the work area's curveFrame. Returns 2 at the
 * table's end (a key with t 0 eases velY down to -50; a negative t sets
 * velY 0 and restarts the frame), else 0.
 */
static u8 EmRathalos_CurveVelY(Em *em, EmWork *work)
{
    EmFlyerWork *w = (EmFlyerWork *) work;
    EmCurveKey *key;
    f32 frame;
    f32 keyFrame;
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
            key = &tbl[i];
            keyFrame = key->t;
            if (frame > keyFrame && keyFrame > 0.0f) {
                i++;
            } else if (keyFrame == 0.0f) {
                ret = 2;
                if (em->velY > -50.0f) {
                    em->velY -= 1.0f;
                }
                break;
            } else if (keyFrame < 0.0f) {
                w->curveFrame = 0;
                ret = 2;
                em->velY = 0.0f;
                break;
            } else {
                i = 0;
                em->velY = (key->v - key[-1].v) / ((keyFrame - key[-1].t) / em->motion[0].step);
            }
        }
    }
    return ret;
}

extern EmCurveKey *EmRathalos_SpeedCurves[];

/*
 * The forward-speed curve (EmRathalos_SpeedCurves[curveKind]): sets velZ
 * from the key pair around curveFrame; returns 4 at the table's end.
 */
static u8 EmRathalos_CurveSpeed(Em *em, EmWork *work)
{
    EmFlyerWork *w = (EmFlyerWork *) work;
    EmCurveKey *key;
    f32 frame;
    f32 keyFrame;
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
            key = &tbl[i];
            keyFrame = key->t;
            if (frame > keyFrame && keyFrame != 0.0f) {
                i++;
            } else if (keyFrame == 0.0f) {
                ret = 4;
                break;
            } else {
                i = 0;
                em->velZ = (key->v - key[-1].v) / ((keyFrame - key[-1].t) / em->motion[0].step);
            }
        }
    }
    return ret;
}

/*
 * Runs the curves whose flags (curveNone, curveVelY, curveSpeed) are set,
 * advances curveFrame by the motion's step, and in the air moves the
 * monster along its yaw. Returns the curves' end bits (2: vertical speed,
 * 4: forward speed).
 */
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
    if (em->stance == EM_STANCE_AIR) {
        w->moveAngX = 0;
        w->moveAngY = em->rotY;
        w->moveAngZ = 0;
        Em_MoveRotatedAccel(em, &w->moveAngX);
    }
    return ret;
}

/*
 * Plays the sounds of the motion on layer 0 at its frames
 * (Em_PlaySeAtBoneOnFrame: frame, sound id, bone), switching on the motion
 * id in EmWork.sndMotion (the motion EmFatalis_MotionEffects keeps there as
 * EmFatalisWork.fxMotion).
 */
void EmFatalis_MotionSounds(Em *em, EmWork *work)
{
    s16 motion;

    motion = work->sndMotion;
    switch (motion) {
    case EM_MOTION_ID(0, FAT_MOT_IDLE):
        Em_PlaySeAtBoneOnFrame(em, 6, 0x5B, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0x118, 0x54, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 9, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 9, 0x28);
        break;
    case EM_MOTION_ID(0, FAT_MOT_WALK):
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0x52, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x56, 1, 5);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0xE, 0xB);
        Em_PlaySeAtBoneOnFrame(em, 0x52, 0xE, 0xB);
        Em_PlaySeAtBoneVolOnFrame(em, 0x12, 0x65, 9, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x58, 0x65, 5, 8);
        break;
    case EM_MOTION_ID(0, FAT_MOT_TURN_LEFT):
        Em_PlaySeAtBoneOnFrame(em, 6, 0x46, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 6, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x60, 0x2A, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 1, 5);
        Em_PlaySeAtBoneOnFrame(em, 0x5E, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0x1A, 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 0x19, 0x11);
        Em_PlaySeAtBoneVolOnFrame(em, 0x60, 0x65, 9, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1E, 0x65, 5, 8);
        break;
    case EM_MOTION_ID(0, FAT_MOT_TURN_RIGHT):
        Em_PlaySeAtBoneOnFrame(em, 6, 0x46, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 6, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x60, 0x2A, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x5E, 1, 5);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0x1A, 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 0x19, 0x11);
        Em_PlaySeAtBoneVolOnFrame(em, 0x60, 0x65, 5, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1E, 0x65, 9, 8);
        break;
    case EM_MOTION_ID(0, FAT_MOT_CHARGE_START):
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0x46, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 0x26, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 3, 0x1D);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 6, 0x18);
        Em_PlaySeAtBoneOnFrame(em, 0x70, 2, 0x1D);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 9, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 9, 0x28);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 0x10, 0xB);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 0x1A, 0xB);
        Em_PlaySeAtBoneVolOnFrame(em, 0x28, 0x65, 0, 8);
        break;
    case EM_MOTION_ID(0, FAT_MOT_TURN_SMALL):
        Em_PlaySeAtBoneOnFrame(em, 6, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 0x1A, 0xB);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 0x46, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0x80, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 1, 5);
        break;
    case EM_MOTION_ID(0, FAT_MOT_CHARGE):
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x4B, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x11, 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 4, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 4, 5);
        Em_PlaySeAtBoneOnFrame(em, 0x62, 4, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x86, 4, 5);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 2, 0x18);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 2, 0x1D);
        Em_PlaySeAtBoneOnFrame(em, 0x5C, 2, 0x18);
        Em_PlaySeAtBoneOnFrame(em, 0x7E, 2, 0x1D);
        break;
    case EM_MOTION_ID(0, FAT_MOT_RECOVER):
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x46, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 4, 0xF, 0x11);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x66, 4, 5);
        Em_PlaySeAtBoneOnFrame(em, 0x86, 4, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 0xD, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x54, 0xD, 0x28);
        break;
    case EM_MOTION_ID(0, FAT_MOT_TAKE_OFF):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x4A, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 0x2A, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0x11, 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x8C, 7, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0xA, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0xA, 0x28);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 0xA, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 0xA, 0x28);
        Em_PlaySeAtBoneOnFrame(em, 0x7C, 0xA, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x82, 0xA, 0x28);
        Em_PlaySeAtBoneOnFrame(em, 0xBA, 0xA, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0xBE, 0xA, 0x28);
        Em_PlaySeAtBoneOnFrame(em, 0xF6, 0xA, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0xFA, 0xA, 0x28);
        Em_PlaySeAtBoneOnFrame(em, 0x138, 0xA, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x13C, 0xA, 0x28);
        Em_PlaySeAtBoneOnFrame(em, 0x180, 0xA, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x184, 0xA, 0x28);
        Em_PlaySeAtBoneVolOnFrame(em, 0x8C, 0x65, 0, 8);
        break;
    case EM_MOTION_ID(0, FAT_MOT_FLY):
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0xA, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 0xA, 0x28);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 0xA, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x68, 0xA, 0x28);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 0x18, 0x11);
        break;
    case EM_MOTION_ID(0, FAT_MOT_DESCEND):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x59, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 0xA, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 0xA, 0x28);
        Em_PlaySeAtBoneOnFrame(em, 0x5A, 0xC, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x5E, 0xC, 0x28);
        Em_PlaySeAtBoneOnFrame(em, 0x8C, 0xC, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x90, 0xC, 0x28);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 0x1A, 0x11);
        break;
    case EM_MOTION_ID(0, FAT_MOT_TOUCH_DOWN):
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0x46, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0x7A, 0x4B, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 3, 5);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 5, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 8, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0xA, 0x18);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 6, 0x1D);
        Em_PlaySeAtBoneOnFrame(em, 0x8E, 2, 0x1D);
        Em_PlaySeAtBoneOnFrame(em, 0x9A, 4, 5);
        Em_PlaySeAtBoneOnFrame(em, 0xB6, 3, 0x18);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 0xD, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0xD, 0x28);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 0xA, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0xA, 0x28);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 0xD, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x70, 0xD, 0x28);
        Em_PlaySeAtBoneVolOnFrame(em, 0x14, 0x65, 9, 8);
        break;
    case EM_MOTION_ID(0, FAT_MOT_FIREBALL):
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0x47, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0x56, 0x48, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0x5E, 0x1B, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0xB0, 0x21, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x17, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x60, 0x1A, 0x11);
        break;
    case EM_MOTION_ID(0, FAT_MOT_FIREBALL_B):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x47, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 0x48, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0x5E, 0x1B, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0xB0, 0x21, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x17, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x60, 0x1A, 0x11);
        break;
    case EM_MOTION_ID(0, FAT_MOT_SWEEP):
        Em_PlaySeAtBoneOnFrame(em, 4, 0xF, 0xB);
        Em_PlaySeAtBoneOnFrame(em, 0xB4, 0xF, 0xB);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x1C, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0x52, 0x4B, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0xEE, 0x2A, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0x56, 0x19, 0x33);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x5E, 1, 5);
        Em_PlaySeAtBoneOnFrame(em, 0xFA, 1, 5);
        Em_PlaySeAtBoneOnFrame(em, 0x12C, 1, 5);
        Em_PlaySeAtBoneVolOnFrame(em, 0x60, 0x65, 5, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x2E, 0x65, 9, 8);
        break;
    case EM_MOTION_ID(0, FAT_MOT_AIR_FIREBALL):
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0x48, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0x1B, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0xC, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 0xC, 0x28);
        Em_PlaySeAtBoneOnFrame(em, 0x5C, 0xC, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x3C6, 0xC, 0x28);
        break;
    case EM_MOTION_ID(0, 0x12):
        Em_PlaySeAtBoneOnFrame(em, 0x20, 0x4A, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0x94, 0x21, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 4, 5);
        Em_PlaySeAtBoneOnFrame(em, 0x90, 4, 5);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x1A, 0x1D);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0xF, 0xB);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 0xF, 0xB);
        break;
    case EM_MOTION_ID(0, 0x13):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x1C, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x1D, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 0x46, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0x1A, 0x33);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 1, 5);
        Em_PlaySeAtBoneOnFrame(em, 0xAC, 4, 5);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x11, 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 0x11, 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x8E, 0x11, 0x11);
        Em_PlaySeAtBoneVolOnFrame(em, 0x10, 0x65, 5, 8);
        break;
    case EM_MOTION_ID(0, FAT_MOT_FLINCH):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x4C, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 0x29, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0xAA, 0x4E, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x17, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xB8, 4, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xCE, 4, 5);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0x11, 0xB);
        Em_PlaySeAtBoneOnFrame(em, 0x86, 0xF, 0xB);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 9, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 9, 0x28);
        Em_PlaySeAtBoneOnFrame(em, 0x7A, 9, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x7E, 9, 0x28);
        break;
    case EM_MOTION_ID(0, FAT_MOT_FLINCH_B):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x4F, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 0x50, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x36, 1, 5);
        Em_PlaySeAtBoneOnFrame(em, 0xC4, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xE2, 1, 5);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 6, 0x18);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x17, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 9, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 9, 0x28);
        Em_PlaySeAtBoneOnFrame(em, 0xA0, 0xF, 0xB);
        Em_PlaySeAtBoneVolOnFrame(em, 0x4A, 0x65, 0x1D, 8);
        break;
    case EM_MOTION_ID(0, FAT_MOT_FLINCH_TURN_RIGHT):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x4F, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0x78, 0x29, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0xC2, 0x4E, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0x14, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 3, 5);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 3, 0x18);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 6, 0x1D);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 9, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 9, 0x28);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 0x10, 0xB);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0xF, 0xB);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0x18, 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0xA8, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x9C, 0x18, 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x106, 0, 5);
        Em_PlaySeAtBoneVolOnFrame(em, 0x3E, 0x65, 0x1D, 8);
        break;
    case EM_MOTION_ID(0, FAT_MOT_FLINCH_TURN_LEFT):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x4F, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0x78, 0x29, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0xC2, 0x4E, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 0x14, 5);
        Em_PlaySeAtBoneOnFrame(em, 0xA2, 1, 5);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 3, 0x18);
        Em_PlaySeAtBoneOnFrame(em, 0x42, 6, 0x1D);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 9, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 9, 0x28);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x10, 0xB);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0xF, 0xB);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x1A, 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 0x18, 0xB);
        Em_PlaySeAtBoneOnFrame(em, 0xF8, 0, 5);
        Em_PlaySeAtBoneVolOnFrame(em, 0x3E, 0x65, 0x1D, 8);
        break;
    case EM_MOTION_ID(0, FAT_MOT_DIE):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x49, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0x70, 0x35, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0x196, 0x35, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0x280, 0x29, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 3, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x82, 1, 5);
        Em_PlaySeAtBoneOnFrame(em, 0x112, 0x12, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x136, 6, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x5C, 0xA, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 0xA, 0x28);
        Em_PlaySeAtBoneOnFrame(em, 0xEA, 0xA, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0xEE, 0xA, 0x28);
        Em_PlaySeAtBoneOnFrame(em, 0x1A2, 0x19, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x198, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 0x1A, 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0xD8, 0x11, 0xB);
        Em_PlaySeAtBoneOnFrame(em, 0xE6, 0x1A, 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x254, 0x11, 0xB);
        Em_PlaySeAtBoneVolOnFrame(em, 0x136, 0x65, 0, 9);
        break;
    case EM_MOTION_ID(0, FAT_MOT_FALL):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x4C, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 4, 0xE, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 8, 0xE, 0x28);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x18, 0x11);
        break;
    case EM_MOTION_ID(0, FAT_MOT_FALL_LAND):
        Em_PlaySeAtBoneOnFrame(em, 0xE, 8, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 6, 0x33);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0x53, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0x78, 2, 0x18);
        Em_PlaySeAtBoneOnFrame(em, 0x86, 2, 0x1D);
        Em_PlaySeAtBoneOnFrame(em, 0x118, 4, 5);
        Em_PlaySeAtBoneOnFrame(em, 0xDC, 4, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xA6, 0x15, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x8A, 0x21, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0xCC, 2, 0x18);
        Em_PlaySeAtBoneVolOnFrame(em, 0x10, 0x65, 0, 9);
        break;
    case EM_MOTION_ID(0, FAT_MOT_DOWN):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x49, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0x52, 0x2F, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0xAC, 0x30, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0x128, 0x56, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 3, 0x1D);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 7, 0x18);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 6, 0x33);
        Em_PlaySeAtBoneOnFrame(em, 0x86, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x11, 0xB);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0x1A, 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x60, 0x18, 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x8E, 0x1A, 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x138, 0x18, 0x11);
        Em_PlaySeAtBoneVolOnFrame(em, 0x26, 0x65, 0x1D, 8);
        break;
    case EM_MOTION_ID(0, FAT_MOT_DOWN_GET_UP):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x50, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0x8C, 0x5A, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0x12E, 0x59, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x15, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x8C, 0x17, 0);
        Em_PlaySeAtBoneOnFrame(em, 0xDC, 3, 0x1D);
        Em_PlaySeAtBoneOnFrame(em, 0xDC, 6, 0x18);
        Em_PlaySeAtBoneVolOnFrame(em, 0xDE, 0x65, 0x1D, 8);
        break;
    case EM_MOTION_ID(0, FAT_MOT_DIE_B):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x50, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0x78, 0x35, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0x162, 0x32, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0x162, 0x2B, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0x1AE, 0x35, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x15, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 0x18, 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x13E, 0xF, 0xB);
        break;
    case EM_MOTION_ID(0, 0x1E):
        Em_PlaySeAtBoneOnFrame(em, 8, 0x5B, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0xBA, 0x21, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 9, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 9, 0x28);
        break;
    case EM_MOTION_ID(0, FAT_MOT_AIR_FIREBALL_AIM):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x1F, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 0xA, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 0xA, 0x28);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 0xA, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x6E, 0xA, 0x28);
        break;
    case EM_MOTION_ID(0, FAT_MOT_ROAR):
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0x55, 0x34, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 0xC, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x42, 0xC, 0x28);
        Em_PlaySeAtBoneOnFrame(em, 4, 0xF, 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x78, 0x11, 0x11);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x18, 0xB);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 0x18, 0xB);
        break;
    case EM_MOTION_ID(0, FAT_MOT_HOP_FIREBALL):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x47, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 0x48, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0x5E, 0x1B, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0x82, 0x21, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0x9C, 0x1E, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0xA, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0xA, 0x28);
        Em_PlaySeAtBoneOnFrame(em, 0x7A, 0xC, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x7E, 0xC, 0x28);
        Em_PlaySeAtBoneOnFrame(em, 0xB4, 0xC, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0xBA, 0xC, 0x28);
        Em_PlaySeAtBoneOnFrame(em, 0xF4, 0xC, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x94, 0xC, 0x28);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 7, 5);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x17, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0xF, 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 0x1A, 0x11);
        Em_PlaySeAtBoneVolOnFrame(em, 0x40, 0x65, 0, 8);
        break;
    case EM_MOTION_ID(0, FAT_MOT_LIE_DOWN):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x5A, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0x96, 0x50, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0x140, 0x33, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0x1A4, 0x5C, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x17, 0);
        Em_PlaySeAtBoneOnFrame(em, 0xF8, 6, 0x1D);
        Em_PlaySeAtBoneOnFrame(em, 0x10A, 8, 0x33);
        Em_PlaySeAtBoneOnFrame(em, 0xEA, 0x19, 0x33);
        Em_PlaySeAtBoneVolOnFrame(em, 0x10E, 0x65, 0x33, 8);
        break;
    case EM_MOTION_ID(0, FAT_MOT_GLIDE_START):
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0xE, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0xE, 0x28);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0xC, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 0xC, 0x28);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 5, 9);
        break;
    case EM_MOTION_ID(0, FAT_MOT_LAND_SLIDE):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x3F, 0x33);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x40, 0x28);
        Em_PlaySeAtBoneOnFrame(em, 8, 0x40, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x22, 0x33);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 0xC, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 0xC, 0x28);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 0xA, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x52, 0xA, 0x28);
        Em_PlaySeAtBoneOnFrame(em, 0x9E, 9, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0xA2, 9, 0x28);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x20, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0x12, 5);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 5, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x9E, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x54, 6, 0);
        Em_PlaySeAtBoneVolOnFrame(em, 0x54, 0x65, 0, 8);
        break;
    case EM_MOTION_ID(0, FAT_MOT_DIVE_RISE):
        Em_PlaySeAtBoneOnFrame(em, 0x10, 0x49, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0xE, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 0xE, 0x28);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x3F, 0x33);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x40, 0x28);
        Em_PlaySeAtBoneOnFrame(em, 8, 0x40, 0x22);
        break;
    case EM_MOTION_ID(0, FAT_MOT_DIVE):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x3F, 0x33);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x40, 0x28);
        Em_PlaySeAtBoneOnFrame(em, 8, 0x40, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 0x41, 0x33);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 0x40, 0x28);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 0x40, 0x22);
        break;
    case EM_MOTION_ID(0, FAT_MOT_DIVE_PULL_UP):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x3F, 0x33);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x40, 0x28);
        Em_PlaySeAtBoneOnFrame(em, 8, 0x40, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x22, 0x33);
        break;
    case EM_MOTION_ID(0, FAT_MOT_GLIDE):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x3F, 0x33);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x40, 0x28);
        Em_PlaySeAtBoneOnFrame(em, 8, 0x40, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 0x41, 0x33);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 0x40, 0x28);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 0x40, 0x22);
        break;
    case EM_MOTION_ID(0, 0x29):
        Em_PlaySeAtBoneOnFrame(em, 0x20, 0x4A, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0x94, 0x21, 0x34);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 4, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x90, 4, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x1A, 0x18);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0xF, 0xB);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 0xF, 0xB);
        
    default:
        break;
    }
}

/*
 * Sets a Fatalis up (gm_em.c calls it for the three kinds): on stage 0 of
 * area 0x19 it stands at a point set by its slot number; its hp comes from
 * Em_ComputeSize; it starts in FAT_IDLE, in combat with every player, with
 * the rage timer off.
 */
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
    Em_SetMotionRow(em, FAT_MOT_IDLE, 0, 0);
    em->stance = EM_STANCE_GROUND;
    EmFatalis_ChangeState(em, EM_GROUP_IDLE, FAT_IDLE, EM_SYNC_LOCAL);
    w->rageTimer = -1;
}

/*
 * EM_KIND_FATALIS_B and _C: while enraged, counts EmFatalisWork.rageTimer
 * up to riseFrames (40 or 60 by kind) and then sets Em.rageStep to 1; once
 * calm, counts it up to calmFrames (50 or 60) and turns it off (-1).
 */
void EmFatalis_UpdateRageTimer(Em *em)
{
    EmFatalisWork *w = (EmFatalisWork *)&em->work;
    s16 riseFrames;
    s16 calmFrames;

    if (em->type == EM_KIND_FATALIS_B || em->type == EM_KIND_FATALIS_C) {
        riseFrames = 40;
        if (em->type == EM_KIND_FATALIS_B) {
            calmFrames = 50;
        } else {
            riseFrames = 60;
            calmFrames = riseFrames;
        }
        if (em->rage != 0) {
            switch (em->rageStep) {
            case 0:
                if (++w->rageTimer > riseFrames) {
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
                if (++w->rageTimer > calmFrames) {
                    w->rageTimer = -1;
                }
            }
        }
    }
}

/*
 * Puts the carcass's three mine points at bones of EmFatalis_MinePtBones
 * (on the floor, radii from EmFatalis_MinePtRadius); each kind has its own
 * gathering tables by the quest's tier.
 */
void EmFatalis_AddMinePoints(Em *em)
{
    EmFatalisWork *w = (EmFatalisWork *)&em->work;
    s32 i;
    MinePoint point;
    f32 pos[3];

    if (em->type == EM_KIND_FATALIS) {
        switch (gQuest.header->tier) {
        case 0:
        default:
            point.table = 3;
            break;
        case 1:
            point.table = 0x7F;
            break;
        case 2:
            point.table = 0xFC;
            break;
        case 3:
            point.table = 0xFC;
            break;
        }
    } else if (em->type == EM_KIND_FATALIS_C) {
        switch (gQuest.header->tier) {
        default:
            point.table = 0x7A;
            break;
        case 1:
            point.table = 0xF7;
            break;
        case 2:
            point.table = 0x173;
            break;
        case 3:
            point.table = 0x173;
            break;
        }
    } else {
        switch (gQuest.header->tier) {
        default:
            point.table = 0x30;
            break;
        case 1:
            point.table = 0xAC;
            break;
        case 2:
            point.table = 0x129;
            break;
        case 3:
            point.table = 0x129;
            break;
        }
    }
    for (i = 0; i < 3; i++) {
        point.radius = EmFatalis_MinePtRadius[i];
        point.area = (u8) em->area;
        point.timer = 0;
        point.flags = 1;
        Em_BonePos(em, EmFatalis_MinePtBones[i][0], pos);
        pos[1] = em->floorY;
        point.unk14 = 2;
        point.count = EmFatalis_MinePtBones[i][1];
        func_3A6120(&point);
        w->minePoints[i] = MinePoint_AddUntimed(&point, pos, 0);
    }
}

/*
 * Keeps the carcass's mine points under their bones, and removes each one
 * that has no gathers left.
 */
void EmFatalis_UpdateMinePoints(Em *em)
{
    EmFatalisWork *w = (EmFatalisWork *)&em->work;
    s32 i;
    s8 *bone;
    f32 *posY;
    s8 *id;
    f32 pos[3];

    bone = EmFatalis_MinePtBones[0];
    posY = &pos[1];
    for (i = 0; i < 3; i++) {
        id = &w->minePoints[i];
        if (*id != -1) {
            Em_BonePos(em, *bone, pos);
            *posY = em->floorY;
            MinePoint_SetPos(*id, pos);
            if (MinePoint_Count(*id) <= 0) {
                MinePoint_Remove(*id);
                *id = -1;
            }
        }
        bone += 2;
    }
}

/* Removes the carcass's mine points that are left. */
void EmFatalis_RemoveMinePoints(Em *em)
{
    EmFatalisWork *w = (EmFatalisWork *)&em->work;
    s32 i;
    s8 *id;

    for (i = 0; i < 3; i++) {
        id = &w->minePoints[i];
        if (*id != -1) {
            MinePoint_Remove(*id);
            *id = -1;
        }
    }
}

/*
 * A fireball's objects: for EM_KIND_FATALIS_C a stage object of kind 0x1C
 * (type 0x26) on the monster, with the two angles; for the others two stage
 * objects of kind 0x11 (types 1 and 2) at bone 0x33, as Rathalos's
 * fireballs make at its bone 0x24.
 */
void EmFatalis_SpawnFireball(Em *em, s32 angleB, s32 angle)
{
    if (em->type == EM_KIND_FATALIS_C) {
        StageObj_K1C_CreateOnEmB(em, 0x26, angleB, angle, 1.0f);
        return;
    }
    StageObj_K11_CreateAtBone(em, 0x33, 1, 1);
    StageObj_K11_CreateAtBone(em, 0x33, 2, 1);
}

/*
 * A fireball's effect, pitched and turned by the arguments: EffEm33 kind
 * 0xD for EM_KIND_FATALIS_C, EffEm08 type 0xB at bone 0x33 for the others.
 */
void EmFatalis_SpawnFireballEffect(Em *em, s32 pitch, s32 yaw)
{
    if (em->type == EM_KIND_FATALIS_C) {
        EffEm33_Create(em, 0xD, 0, pitch, yaw);
        return;
    }
    EffEm08_CreateAtBone(em, 0x33, 0xB, 0, pitch, yaw);
}

/*
 * One of a sweep's effects, at the spot the argument names (a value of an
 * EmFatalis_SweepSpots row): EffEm33 kind 0xC for EM_KIND_FATALIS_C,
 * EffEm08 type 0xC from bone 0 pitched 0x4000 for the others.
 */
void EmFatalis_SpawnSweepEffect(Em *em, s32 spot)
{
    if (em->type == EM_KIND_FATALIS_C) {
        EffEm33_Create(em, 0xC, spot, 0, 0);
    } else {
        EffEm08_CreateAtBone(em, 0, 0xC, spot, 0x4000, 0);
    }
}

/*
 * Ends a ground state: asks the script for the next one (scriptPending) and
 * stands idle (FAT_IDLE) meanwhile.
 */
void EmFatalis_EndState(Em *em)
{
    em->scriptPending = 1;
    em->motionSpeed = 1.0f;
    EmFatalis_ChangeState(em, EM_GROUP_IDLE, FAT_IDLE, EM_SYNC_LOCAL);
}

/*
 * Ends an air state: asks the script for the next one and hovers
 * (FAT_FLY_HOVER, next 0) meanwhile.
 */
void EmFatalis_EndAirState(Em *em, u8 next)
{
    em->scriptPending = 1;
    em->motionSpeed = 1.0f;
    switch (next) {
    case 0:
        EmFatalis_ChangeState(em, EM_GROUP_FLY, FAT_FLY_HOVER, EM_SYNC_LOCAL);
        break;
    }
}

/*
 * The per-frame update: the rage (the rage timer; Em_CheckRageByHp below
 * 65% hp, held there once enraged) and, for EM_KIND_FATALIS_B and _C, the
 * hitzone set by hp; then the reaction Em_UpdateDamage asks for; then the
 * scenes (FAT_EVENT_*) the three kinds play in their areas until an event
 * flag is set, or else the monster script; then the state.
 */
void EmFatalis_Update(Em *em)
{
    EmWork *work = (EmWork *)&em->work;
    u8 buf[4];

    EmFatalis_UpdateRageTimer(em);
    em->painted = 0;
    em->paintTimer = 0;
    Em_RaiseTrigger80Lax(em);
    Em_HoldPosNoPlayers(em, 0, 1);
    if (em->stateGroup != EM_GROUP_DEFEAT) {
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
    if (em->type == EM_KIND_FATALIS_B || em->type == EM_KIND_FATALIS_C) {
        if (em->hp <= (s16) (0.2f * em->hpMax)) {
            em->hitzoneSet = 0;
        } else if (em->hp <= (s16) (0.5f * em->hpMax)) {
            em->hitzoneSet = 1;
        } else {
            em->hitzoneSet = 0;
        }
    }
    switch (Em_UpdateDamage(em, buf) & 0xFF) {
    case EM_DMG_NONE:
    case EM_DMG_DEAD:
    case EM_DMG_ATTACK6:
    case EM_DMG_CAPTURED:
    case EM_DMG_FLASH:
    case EM_DMG_HIT:
    case EM_DMG_ATTACK5:
    case EM_DMG_ATTACK5_FLINCH:
    case EM_DMG_ATTACK13:
        break;
    case EM_DMG_KILLED:
        if (em->stance == EM_STANCE_AIR) {
            EmFatalis_ChangeState(em, EM_GROUP_DEFEAT, FAT_DEFEAT_DIE_FALLING, EM_SYNC_OWNER);
        } else if (em->lastAttackKind == 9) {
            EmFatalis_ChangeState(em, EM_GROUP_DEFEAT, FAT_DEFEAT_DIE_B, EM_SYNC_OWNER);
        } else {
            EmFatalis_ChangeState(em, EM_GROUP_DEFEAT, FAT_DEFEAT_DIE, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_ATTACK9:
        EmFatalis_ChangeState(em, EM_GROUP_REACT, FAT_REACT_KNOCK_DOWN, EM_SYNC_OWNER);
        break;
    case EM_DMG_STATUS_IN_AIR:
        if (em->stateGroup != EM_GROUP_REACT || em->state != FAT_REACT_FALL) {
            EmFatalis_ChangeState(em, EM_GROUP_REACT, FAT_REACT_FALL, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_STATUS4:
        if ((em->stateGroup != EM_GROUP_REACT || em->state != FAT_REACT_STATUS4) && (em->stateGroup != EM_GROUP_REACT || em->state != FAT_REACT_FALL)) {
            Em_SetStatus4Timer(em);
            EmFatalis_ChangeState(em, EM_GROUP_REACT, FAT_REACT_STATUS4, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_STATUS2:
        if ((em->stateGroup != EM_GROUP_IDLE || em->state != FAT_IDLE_STATUS2) && (em->stateGroup != EM_GROUP_REACT || em->state != FAT_REACT_FALL)) {
            Em_SetStatus2Timer(em);
            EmFatalis_ChangeState(em, EM_GROUP_IDLE, FAT_IDLE_STATUS2, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_HIT_LYING:
        if (em->stateGroup == EM_GROUP_IDLE && em->state == FAT_IDLE_STATUS2) {
            EmFatalis_ChangeState(em, EM_GROUP_IDLE, FAT_IDLE_STATUS2_END, EM_SYNC_OWNER);
            em->scriptPending = 0;
        }
        break;
    case EM_DMG_PART8_DOWN:
        EmFatalis_ChangeState(em, EM_GROUP_REACT, FAT_REACT_KNOCK_DOWN, EM_SYNC_OWNER);
        break;
    case EM_DMG_PART_FLINCH:
        Player_ClearFlag((Player *) em, 0x20000);
        if (em->stance == EM_STANCE_AIR) {
            EmFatalis_ChangeState(em, EM_GROUP_REACT, FAT_REACT_FALL, EM_SYNC_OWNER);
        } else {
            switch (em->flinchPart) {
            case 0:
            case 1:
            case 4:
            case 5:
                EmFatalis_ChangeState(em, EM_GROUP_REACT, FAT_REACT_FLINCH, EM_SYNC_OWNER);
                break;
            case 6:
                EmFatalis_ChangeState(em, EM_GROUP_REACT, FAT_REACT_FLINCH_B, EM_SYNC_OWNER);
                break;
            case 7:
            default:
                if (em->flinchPart != 3) {
                    EmFatalis_ChangeState(em, EM_GROUP_REACT, FAT_REACT_FLINCH_TURN_RIGHT, EM_SYNC_OWNER);
                } else {
                    EmFatalis_ChangeState(em, EM_GROUP_REACT, FAT_REACT_FLINCH_TURN_LEFT, EM_SYNC_OWNER);
                }
                break;
            }
        }
        break;
    case EM_DMG_HIT_MODE3:
        if (em->stance != EM_STANCE_AIR) {
            EmFatalis_ChangeState(em, EM_GROUP_REACT, FAT_REACT_FLINCH, EM_SYNC_OWNER);
            em->scriptPending = 0;
        }
        break;
    case EM_DMG_STATUS5:
        Em_EndStatus5(em);
        break;
    }
    /*
     * The scenes: each kind has one in its area (by day id) while its event
     * flag is unset, started while an event runs; EM_KIND_FATALIS leaves its
     * area once the quest runs there. Everything else runs the monster
     * script; the gotos share that call.
     */
    if (em->type == EM_KIND_FATALIS) {
        if (Area_DayId(em->area) == 0x19) {
            if (Save_TestEventFlag(0xB0) == 0) {
                if (gScene.eventRunning == 1 && em->stateGroup != EM_GROUP_EVENT) {
                    EmFatalis_ChangeState(em, EM_GROUP_EVENT, FAT_EVENT_FLY_IN, EM_SYNC_LOCAL);
                }
            } else if (Quest_IsRunning(0) == 1) {
                if (em->stateGroup != EM_GROUP_EVENT && em->stateGroup != EM_GROUP_DEFEAT) {
                    em->scriptPending = 0;
                    if (gScene.area == em->area) {
                        CamReq_ForMonster(em);
                    }
                    if (em->stance == EM_STANCE_AIR) {
                        EmFatalis_ChangeState(em, EM_GROUP_EVENT, FAT_EVENT_LEAVE_FROM_AIR, EM_SYNC_LOCAL);
                    } else {
                        EmFatalis_ChangeState(em, EM_GROUP_EVENT, FAT_EVENT_LEAVE, EM_SYNC_LOCAL);
                    }
                }
            } else {
                goto runScript;
            }
        } else {
            goto runScript;
        }
    } else if (em->type == EM_KIND_FATALIS_B) {
        if (Area_DayId(em->area) == 0x58 && Save_TestEventFlag(0xB1) == 0) {
            if (gScene.eventRunning == 1 && em->stateGroup != EM_GROUP_EVENT) {
                EmFatalis_ChangeState(em, EM_GROUP_EVENT, FAT_EVENT_GET_UP, EM_SYNC_LOCAL);
            }
        } else {
            goto runScript;
        }
    } else if (Area_DayId(em->area) == 0x7C && em->type == EM_KIND_FATALIS_C && Save_TestEventFlag(0xB2) == 0) {
        if (gScene.eventRunning == 1 && em->stateGroup != EM_GROUP_EVENT) {
            EmFatalis_ChangeState(em, EM_GROUP_EVENT, FAT_EVENT_DESCEND, EM_SYNC_LOCAL);
        }
    } else {
    runScript:
        Em_UpdatePendingScript(em);
    }
    Em_ClearHitReactMode(em);
    EmFatalis_RunState(em, work);
    if (em->stateRequest != 0) {
        EmFatalis_RunState(em, work);
        em->stateRequest = 0;
    }
}

/*
 * FAT_IDLE_STAND: idles on all three layers until the timer runs out, then
 * asks the script for the next state.
 */
void EmFatalis_Stand(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        if (em->motionId[0] != EM_MOTION_ID(0, FAT_MOT_IDLE)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(0, FAT_MOT_IDLE), 0xA, 0, 0);
        }
        if (em->motionId[1] != EM_MOTION_ID(0, 0x65)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(1, FAT_MOT_IDLE), 0xA, 0, 1);
        }
        if (em->motionId[2] != EM_MOTION_ID(1, FAT_MOT_IDLE)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(2, FAT_MOT_IDLE), 0xA, 0, 2);
        }
        break;
    case 1:
        if (--em->timer <= 0) {
            em->scriptPending = 1;
        }
        break;
    }
}

/* FAT_IDLE: plays the idle motion once. */
void EmFatalis_Idle(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, FAT_MOT_IDLE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmFatalis_EndState(em);
        }
        break;
    }
}

/* FAT_IDLE_ACTION: plays motion 0x1E once. */
void EmFatalis_IdleAction(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
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

/* FAT_IDLE_ROAR: roars once. */
void EmFatalis_IdleRoar(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, FAT_MOT_ROAR, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmFatalis_EndState(em);
        }
        break;
    }
}

/*
 * FAT_IDLE_STATUS2: lies down under status 2, blind and without flinching,
 * with a periodic effect at bone 0x33 once lying (frame 296), until the
 * status timer ends (then FAT_IDLE_STATUS2_END).
 */
void EmFatalis_Status2Lie(Em *em, EmWork *work)
{
    f32 pos[3];

    Em_SetHitReactMode(em, 2U);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, FAT_MOT_LIE_DOWN, 0, 0);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmFatalis_ChangeState(em, EM_GROUP_IDLE, FAT_IDLE_STATUS2_END, EM_SYNC_OWNER);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmFatalis_ChangeState(em, EM_GROUP_IDLE, FAT_IDLE_STATUS2_END, EM_SYNC_OWNER);
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

/* FAT_IDLE_STATUS2_END: gets up (FAT_MOT_RECOVER) and sees again. */
void EmFatalis_Status2End(Em *em, EmWork *work)
{
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, FAT_MOT_RECOVER, 0, 0);
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

/* FAT_IDLE_STATUS4_END: gets up after FAT_REACT_STATUS4. */
void EmFatalis_Status4End(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, FAT_MOT_RECOVER, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmFatalis_EndState(em);
        }
        break;
    }
}

/* FAT_MOVE_WALK: walks to the goal (Em_StepToGoal). */
void EmFatalis_Walk(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, FAT_MOT_WALK, 0, 0);
        break;
    case 1:
        if (Em_StepToGoal(em, 0x40, 0) == 1) {
            em->step++;
            EmFatalis_EndState(em);
        }
        break;
    }
}

/*
 * FAT_MOVE_TURN_TO_TARGET: turns to the target with the turn motions of
 * EmFatalis_TurnParams.
 */
void EmFatalis_TurnToTarget(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
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

/*
 * FAT_MOVE_TURN_TO_GOAL: turns to the goal: FAT_MOT_TURN_SMALL for less
 * than 0x2000 (91 a frame), else FAT_MOT_TURN_LEFT or _RIGHT (372 a frame
 * until frame 89), each speed scaled by Em.motionSpeed.
 */
void EmFatalis_TurnToGoal(Em *em, EmWork *work)
{
    u32 diff;
    u32 turnStep;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        work->goalYaw = Math_AngleXZ(&em->posX, em->goal);
        diff = (work->goalYaw - em->rotY) & 0xFFFF;
        if (diff <= 0x2000U || diff >= 0xE000U) {
            Em_SetMotion(em, FAT_MOT_TURN_SMALL, 0, 0);
        } else if (diff >= 0x8000U) {
            Em_SetMotion(em, FAT_MOT_TURN_LEFT, 0, 0);
        } else {
            Em_SetMotion(em, FAT_MOT_TURN_RIGHT, 0, 0);
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
        if (em->motionId[0] == EM_MOTION_ID(0, FAT_MOT_TURN_SMALL)) {
            turnStep = 91.0f * em->motionSpeed;
            diff = (work->goalYaw - (em->rotY & 0xFFFF)) & 0xFFFF;
            if ((u32) ((diff + turnStep) & 0xFFFF) < turnStep * 2) {
                em->rotY = work->goalYaw;
            } else if (diff < 0x8000U) {
                em->rotY = (em->rotY + turnStep) & 0xFFFF;
            } else {
                em->rotY = (em->rotY - turnStep) & 0xFFFF;
            }
        } else if (EmMotion_ReachedFrame(em, 0, 89.0f) == 0) {
            turnStep = 372.0f * em->motionSpeed;
            diff = (work->goalYaw - (em->rotY & 0xFFFF)) & 0xFFFF;
            if ((u32) ((diff + turnStep) & 0xFFFF) < turnStep * 2) {
                em->rotY = work->goalYaw;
            } else if (diff < 0x8000U) {
                em->rotY = (em->rotY + turnStep) & 0xFFFF;
            } else {
                em->rotY = (em->rotY - turnStep) & 0xFFFF;
            }
        }
        break;
    }
}

/*
 * FAT_FLY_TAKE_OFF (variant 0) and FAT_FLY_TAKE_OFF_HIGH (variant 1,
 * airborne from the start): takes off (airborne from frame 134) along curve
 * 0, then hovers.
 */
void EmFatalis_TakeOff(Em *em, EmWork *work, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        if (variant == 0) {
            em->stance = EM_STANCE_GROUND;
        } else {
            em->stance = EM_STANCE_AIR;
            Player_ClearFlag(em, 0x20000);
        }
        em->lookAtTarget = 0;
        Em_SetMotion(em, FAT_MOT_TAKE_OFF, 0, 0);
        EmFatalis_StartCurves(em, 0);
        work->flyGuard = 0;
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 134.0f, 0U) != 0) {
            em->step++;
            em->stance = EM_STANCE_AIR;
            EmFatalis_UpdateCurves(em);
        }
        break;
    case 2:
        if (EmFatalis_UpdateCurves(em) != 0) {
            em->step++;
            EmFatalis_EndAirState(em, 0);
        }
        break;
    }
}

/*
 * FAT_FLY_LAND: comes down from the hover (velY -10, then -20 below 700
 * above the floor) and touches down with a camera shake.
 */
void EmFatalis_Land(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        em->rotX = 0;
        em->rotZ = 0;
        Em_SetMotion(em, FAT_MOT_DESCEND, 0, 0);
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
            Em_SetMotion(em, FAT_MOT_TOUCH_DOWN, 0, 0);
            em->posY = em->floorY;
            em->rotX = 0;
            em->rotZ = 0;
            em->stance = EM_STANCE_GROUND;
            CamShake_ReqAIf(em, 1);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, FAT_MOT_RECOVER, 0, 0);
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

/*
 * FAT_FLY_HOVER: hovers on all three layers, moving along velY curve 1 but
 * never above the height it started at, until the timer runs out.
 */
void EmFatalis_Hover(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        work->turnSpeed = 0x100;
        if (em->motionId[0] != EM_MOTION_ID(0, FAT_MOT_FLY)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(0, FAT_MOT_FLY), 0, 0, 0);
        }
        if (em->motionId[1] != EM_MOTION_ID(1, FAT_MOT_FLY)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(1, FAT_MOT_FLY), 0, 0, 1);
        }
        if (em->motionId[2] != EM_MOTION_ID(2, FAT_MOT_FLY)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(2, FAT_MOT_FLY), 0, 0, 2);
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
            EmFatalis_EndAirState(em, 0);
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

/*
 * FAT_FLY_HOVER_TURN (variant 0) and FAT_FLY_HOVER_TURN_STOP (variant 1,
 * stopping first): hovers and turns to the goal, for at most 300 frames.
 */
void EmFatalis_HoverTurn(Em *em, EmWork *work, s32 variant)
{
    f32 floorY;
    s32 done;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        work->turnSpeed = 0x100;
        Em_SetMotion(em, FAT_MOT_FLY, 0, 0);
        work->flyGuard = 0;
        em->timer = 0x12C;
        if (variant != 0) {
            Player_ClearVelocity(em);
        }
        break;
    case 1:
        EmFatalis_MoveByVelYCurve(em, 1);
        done = Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 1) & 0xFF;
        if (--em->timer <= 0 || done != 0) {
            em->step++;
            EmFatalis_EndAirState(em, 0);
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

/*
 * FAT_FLY_TO_GOAL: flies at the goal, turning and pitching to it, until the
 * timer runs out or the goal is within 100 frames of flight.
 */
void EmFatalis_FlyToGoal(Em *em, EmWork *work)
{
    f32 dist;
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        work->turnSpeed = 0x100;
        Em_SetMotion(em, FAT_MOT_FLY, 0, 0);
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
        if (--em->timer <= 0 || dist <= 100.0f * em->velZ) {
            em->step++;
            EmFatalis_EndAirState(em, 0);
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

/*
 * FAT_FLY_RISE: rises (velY 10) until 1500 above the goal or the timer runs
 * out. Its heading comes from pos, which nothing sets: the stack's leftover
 * contents.
 */
void EmFatalis_FlyRise(Em *em, EmWork *work)
{
    f32 pos[3];
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        work->turnSpeed = 0x100;
        Em_SetMotion(em, FAT_MOT_FLY, 0, 0);
        work->flyGuard = 0;
        work->moveAngX = 0;
        work->moveAngZ = 0;
        Player_ClearVelocity(em);
        em->velY = 10.0f;
        break;
    case 1:
        work->moveAngY = Math_AngleXZ(&em->posX, pos) & 0xFFFF;
        Em_MoveRotated(em, &work->moveAngX);
        if (--em->timer <= 0 || em->posY > 1500.0f + em->goal[1]) {
            em->step++;
            Player_ClearVelocity(em);
            EmFatalis_EndAirState(em, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * FAT_FLY_STRAIGHT_TO_GOAL: flies straight at the goal (heading set to it,
 * pitched), until the timer runs out or the goal is within 100 frames of
 * flight.
 */
void EmFatalis_FlyStraightToGoal(Em *em, EmWork *work)
{
    f32 dist;
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        work->turnSpeed = 0x100;
        Em_SetMotion(em, FAT_MOT_FLY, 0, 0);
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
        if (--em->timer <= 0 || dist <= 100.0f * em->velZ) {
            em->step++;
            EmFatalis_EndAirState(em, 0);
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

/*
 * FAT_FLY_DESCEND: comes down (velY -10) until 500 above the floor or the
 * timer runs out. Its heading comes from pos, which nothing sets.
 */
void EmFatalis_FlyDescend(Em *em, EmWork *work)
{
    f32 pos[3];
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        work->turnSpeed = 0x100;
        Em_SetMotion(em, FAT_MOT_FLY, 0, 0);
        work->flyGuard = 0;
        work->moveAngX = 0;
        work->moveAngZ = 0;
        Player_ClearVelocity(em);
        em->velY = -10.0f;
        break;
    case 1:
        work->moveAngY = Math_AngleXZ(&em->posX, pos) & 0xFFFF;
        Em_MoveRotated(em, &work->moveAngX);
        if (--em->timer <= 0 || em->posY <= 500.0f + em->floorY) {
            em->step++;
            Player_ClearVelocity(em);
            EmFatalis_EndAirState(em, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * FAT_FLY_RISE_HIGH: rises (velY 10) to the height 6000. Its heading comes
 * from pos, which nothing sets.
 */
void EmFatalis_FlyRiseHigh(Em *em, EmWork *work)
{
    f32 pos[3];
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        work->turnSpeed = 0x100;
        Em_SetMotion(em, FAT_MOT_FLY, 0, 0);
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
            EmFatalis_EndAirState(em, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * FAT_FLY_LEVEL_TO_GOAL: flies level at the goal, turning to it, until the
 * timer runs out or the goal is within 100 frames of flight.
 */
void EmFatalis_FlyLevelToGoal(Em *em, EmWork *work)
{
    f32 dist;
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        work->turnSpeed = 0x100;
        Em_SetMotion(em, FAT_MOT_FLY, 0, 0);
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
        if (--em->timer <= 0 || dist <= 100.0f * em->velZ) {
            em->step++;
            EmFatalis_EndAirState(em, 0);
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

/*
 * FAT_FLY_CLIMB_TO_GOAL (variant 0) and FAT_FLY_CLIMB_TO_GOAL_NEAR (variant
 * 1): flies at the goal, turning and pitching to it and climbing 20 a frame
 * while a wall is hit, until the timer runs out or the goal is within 100
 * frames of flight (10 for the variant 1).
 */
void EmFatalis_FlyClimbToGoal(Em *em, EmWork *work, s32 variant)
{
    f32 dist;
    f32 floorY;
    f32 frames;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        work->turnSpeed = 0x100;
        Em_SetMotion(em, FAT_MOT_FLY, 0, 0);
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
        if (--em->timer <= 0 || dist <= em->velZ * frames) {
            em->step++;
            EmFatalis_EndAirState(em, 0);
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

/*
 * FAT_FLY_TAKE_OFF_AND_RISE: takes off along curve 6, then rises to the
 * height 6000. The rise's heading comes from pos, which nothing sets.
 */
void EmFatalis_TakeOffAndRise(Em *em, EmWork *work)
{
    f32 pos[3];

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, FAT_MOT_TAKE_OFF, 0, 0);
        EmFatalis_StartCurves(em, 6);
        work->flyGuard = 0;
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 134.0f, 0U) != 0) {
            em->step++;
            em->stance = EM_STANCE_AIR;
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
            EmFatalis_EndAirState(em, 0);
        }
        break;
    }
}

/*
 * FAT_FLY_LAND_AT_HEIGHT: comes down to the height 3584 (not the floor)
 * with a camera shake, then goes on to FAT_ATK_BLASTS.
 */
void EmFatalis_LandAtHeight(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        em->rotX = 0;
        em->rotZ = 0;
        Em_SetMotion(em, FAT_MOT_DESCEND, 0, 0);
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
            Em_SetMotion(em, FAT_MOT_TOUCH_DOWN, 0, 0);
            em->posY = 3584.0f;
            em->rotX = 0;
            em->rotZ = 0;
            CamShake_ReqAIf(em, 1);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, FAT_MOT_RECOVER, 0, 0);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmFatalis_ChangeState(em, EM_GROUP_ATTACK, FAT_ATK_BLASTS, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 5:
        if (em->remote == 0) {
            EmFatalis_ChangeState(em, EM_GROUP_ATTACK, FAT_ATK_BLASTS, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * FAT_ATK_SINGLE_A (variant 0: motion 0x12) and FAT_ATK_SINGLE_C (variant
 * 1: motion 0x29): plays the motion once.
 */
void EmFatalis_SingleAttackA(Em *em, EmWork *work, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
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

/* FAT_ATK_SINGLE_B: plays motion 0x13 once. */
void EmFatalis_SingleAttackB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
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

/* FAT_ATK_FIREBALL: a fireball at frame 102. */
void EmFatalis_Fireball(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, FAT_MOT_FIREBALL, 0, 0);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 102.0f, 0U) != 0) {
            EmFatalis_SpawnFireball(em, 0, 0);
            EmFatalis_SpawnFireballEffect(em, 0, 0);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmFatalis_EndState(em);
        }
        break;
    }
}

/* FAT_ATK_FIREBALL_B: a fireball at frame 98. */
void EmFatalis_FireballB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, FAT_MOT_FIREBALL_B, 0, 0);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 98.0f, 0U) != 0) {
            EmFatalis_SpawnFireball(em, 0, 0);
            EmFatalis_SpawnFireballEffect(em, 0, 0);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmFatalis_EndState(em);
        }
        break;
    }
}

/*
 * FAT_ATK_SWEEP_NO_EFFECT (variant 0) and FAT_ATK_SWEEP_A to _D (variants
 * 1-4): the sweep motion, with an effect at frames 80, 140, 200, 260 and
 * 320 at the spots of the row of EmFatalis_SweepSpots the variant names
 * (none for the variant 0).
 */
void EmFatalis_Sweep(Em *em, EmWork *work, s32 variant)
{
    s32 k;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, FAT_MOT_SWEEP, 0, 0);
        break;
    case 1:
        k = variant & 0xFF;
        if (k != 0) {
            if (Player_PassedFrameScaled(em, 80.0f, 0U) != 0 || Player_PassedFrameScaled(em, 140.0f, 0U) != 0 || Player_PassedFrameScaled(em, 200.0f, 0U) != 0 ||
                Player_PassedFrameScaled(em, 260.0f, 0U) != 0 || Player_PassedFrameScaled(em, 320.0f, 0U) != 0) {
                EmFatalis_SpawnSweepEffect(em, EmFatalis_SweepSpots[k][em->stateVar0]);
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

/*
 * FAT_ATK_AIR_FIREBALL, _B and _C (variants 0-2: pitches 0x38E4, 0x2AAB,
 * 0x1E94): hovers 90 frames turning to the goal, then breathes fireballs at
 * frame 30 of the fireball motion until EmFatalisWork.countdown runs out
 * (set to 1 by EmFatalis_SetAttackState), then hovers.
 */
void EmFatalis_AirFireball(Em *em, EmWork *work, s32 variant)
{
    f32 floorY;
    s32 k;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        work->turnSpeed = 0x100;
        Em_SetMotion(em, FAT_MOT_AIR_FIREBALL_AIM, 0, 0);
        em->timer = 0x5A;
        break;
    case 1:
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 2);
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, FAT_MOT_AIR_FIREBALL, 0, 0);
        }
        break;
    case 2:
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 2);
        if (Player_PassedFrameScaled(em, 30.0f, 0U) != 0) {
            EmFatalis_SpawnFireball(em, 0, 0);
            k = variant & 0xFF;
            switch (k) {
            case 0:
                EmFatalis_SpawnFireballEffect(em, 0x38E4, 0);
                break;
            case 1:
                EmFatalis_SpawnFireballEffect(em, 0x2AAB, 0);
                break;
            case 2:
                EmFatalis_SpawnFireballEffect(em, 0x1E94, 0);
                break;
            }
            work->countdown = (s8) work->countdown - 1;
        }
        if (((s8) work->countdown <= 0) && (Player_PassedFrameScaled(em, 76.0f, 0U) != 0)) {
            em->step++;
            Em_SetMotion(em, FAT_MOT_FLY, 0xA, 0x1E);
        }
        if (em->motion[0].playing == 0) {
            Em_SetMotion(em, FAT_MOT_AIR_FIREBALL, 4, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmFatalis_EndAirState(em, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * FAT_ATK_CHARGE: charges at the goal (Em_StepToGoal), hits unable to
 * flinch or kill it for its length (Em.guardTimer), then recovers.
 */
void EmFatalis_Charge(Em *em)
{
    em->guardTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, FAT_MOT_CHARGE_START, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, FAT_MOT_CHARGE, 0, 0);
            Eff04_Create(em, 2);
        }
        break;
    case 2:
        if (Em_StepToGoal(em, 0x40, 0) == 1) {
            em->step++;
            Em_SetMotion(em, FAT_MOT_RECOVER, 0, 0);
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

/*
 * FAT_ATK_HOP_FIREBALL: hops up (airborne from frame 70, along curve 3) and
 * breathes a fireball (pitch 0x38E4) at frame 94, then hovers.
 */
void EmFatalis_HopFireball(Em *em)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, FAT_MOT_HOP_FIREBALL, 0, 0);
        EmFatalis_StartCurves(em, 3);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 70.0f, 0U) != 0) {
            em->step++;
            em->stance = EM_STANCE_AIR;
            EmFatalis_UpdateCurves(em);
        }
        break;
    case 2:
        if (Player_PassedFrameScaled(em, 94.0f, 0U) != 0) {
            EmFatalis_SpawnFireball(em, 0, 0);
            EmFatalis_SpawnFireballEffect(em, 0x38E4, 0);
        }
        EmFatalis_UpdateCurves(em);
        if (em->motion[0].playing == 0) {
            em->step++;
            EmFatalis_EndAirState(em, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/* FAT_ATK_CHARGE_START: plays the charge's first motion only, then recovers. */
void EmFatalis_ChargeStart(Em *em)
{
    em->guardTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, FAT_MOT_CHARGE_START, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, FAT_MOT_RECOVER, 0, 0);
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

/*
 * FAT_ATK_AIR_FIREBALL_TRIPLE: EmFatalis_AirFireball with three fireballs
 * (EmFatalisWork.countdown 3), pitched 0x1E94, 0x2AAB, then 0x38E4.
 */
void EmFatalis_AirFireballTriple(Em *em, EmWork *work)
{
    f32 floorY;
    s8 countdown;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_SetMotion(em, FAT_MOT_AIR_FIREBALL_AIM, 0, 0);
        em->timer = 0x5A;
        work->turnSpeed = 0x100;
        break;
    case 1:
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 2);
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, FAT_MOT_AIR_FIREBALL, 0, 0);
        }
        break;
    case 2:
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 2);
        if (Player_PassedFrameScaled(em, 30.0f, 0U) != 0) {
            EmFatalis_SpawnFireball(em, 0, 0);
            work->countdown = (s8) work->countdown - 1;
            countdown = (s8) work->countdown;
            switch (countdown) {
            case 2:
                EmFatalis_SpawnFireballEffect(em, 0x38E4, 0);
                break;
            case 1:
                EmFatalis_SpawnFireballEffect(em, 0x2AAB, 0);
                break;
            case 0:
                EmFatalis_SpawnFireballEffect(em, 0x1E94, 0);
                break;
            }
        }
        if (((s8) work->countdown <= 0) && (Player_PassedFrameScaled(em, 76.0f, 0U) != 0)) {
            em->step++;
            Em_SetMotion(em, FAT_MOT_FLY, 0xA, 0x1E);
        }
        if (em->motion[0].playing == 0) {
            Em_SetMotion(em, FAT_MOT_AIR_FIREBALL, 4, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmFatalis_EndAirState(em, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * FAT_ATK_GLIDE: glides low at the goal along curve 4 until Em.goalDist is
 * covered, then lands.
 */
void EmFatalis_Glide(Em *em)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, FAT_MOT_GLIDE_START, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stance = EM_STANCE_AIR;
            Em_SetMotion(em, FAT_MOT_GLIDE, 0, 0);
            EmFatalis_StartCurves(em, 4);
        }
        break;
    case 2:
        EmFatalis_UpdateCurves(em);
        em->goalDist -= em->velZ;
        if (em->goalDist <= 0.0f) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            Em_SetMotion(em, FAT_MOT_LAND_SLIDE, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, FAT_MOT_RECOVER, 0, 0);
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

/*
 * FAT_ATK_DIVE: rises pitching up along curve 5, dives at the floor (velZ
 * 80, accelZ 9), pulls out 850 above the floor and slides along it (pushed
 * back up to the floor while under it), then recovers.
 */
void EmFatalis_Dive(Em *em, EmWork *work)
{
    f32 push[4];
    f32 pushWorld[4];
    f32 mtx[16];

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, FAT_MOT_DIVE_RISE, 0, 0);
        EmFatalis_StartCurves(em, 5);
        em->rotX = 0;
        break;
    case 1:
        em->rotX += 0x1A8;
        EmFatalis_UpdateCurves(em);
        ((EmFatalisWork *)work)->runDist -= em->velZ;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stance = EM_STANCE_AIR;
            Em_SetMotion(em, FAT_MOT_DIVE, 0, 0);
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
            Em_SetMotion(em, FAT_MOT_DIVE_PULL_UP, 0, 0);
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
            push[0] = (em->floorY - em->posY) / 3.0f;
            push[1] = 0.0f;
            push[2] = 0.0f;
            Mtx_FromAnglesYXZ(&work->moveAngX, mtx);
            Vec_Rotate(pushWorld, push, mtx);
            em->posX += pushWorld[0];
            em->posY += pushWorld[1];
            em->posZ += pushWorld[2];
        }
        em->posY -= 50.0f;
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, FAT_MOT_LAND_SLIDE, 0, 0);
            em->rotX = 0;
            em->stance = EM_STANCE_GROUND;
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, FAT_MOT_RECOVER, 0, 0);
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

/*
 * FAT_ATK_SWEEP_DOUBLE: EmFatalis_Sweep twice along row 5 of
 * EmFatalis_SweepSpots (ten spots).
 */
void EmFatalis_SweepDouble(Em *em, EmWork *work, s32 variant)
{
    s32 k;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, FAT_MOT_SWEEP, 0, 0);
        break;
    case 1:
        k = variant & 0xFF;
        if (k != 0) {
            if (Player_PassedFrameScaled(em, 80.0f, 0U) != 0 || Player_PassedFrameScaled(em, 140.0f, 0U) != 0 || Player_PassedFrameScaled(em, 200.0f, 0U) != 0 ||
                Player_PassedFrameScaled(em, 260.0f, 0U) != 0 || Player_PassedFrameScaled(em, 320.0f, 0U) != 0) {
                EmFatalis_SpawnSweepEffect(em, EmFatalis_SweepSpots[5][em->stateVar0]);
                em->stateVar0++;
            }
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, FAT_MOT_SWEEP, 0, 0);
        }
        break;
    case 2:
        k = variant & 0xFF;
        if (k != 0) {
            if (Player_PassedFrameScaled(em, 80.0f, 0U) != 0 || Player_PassedFrameScaled(em, 140.0f, 0U) != 0 || Player_PassedFrameScaled(em, 200.0f, 0U) != 0 ||
                Player_PassedFrameScaled(em, 260.0f, 0U) != 0 || Player_PassedFrameScaled(em, 320.0f, 0U) != 0) {
                EmFatalis_SpawnSweepEffect(em, EmFatalis_SweepSpots[5][em->stateVar0]);
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

/* FAT_ATK_DIVE_B: EmFatalis_Dive, pulling out at the height 1050 instead. */
void EmFatalis_DiveB(Em *em, EmWork *work)
{
    f32 push[4];
    f32 pushWorld[4];
    f32 mtx[16];

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, FAT_MOT_DIVE_RISE, 0, 0);
        EmFatalis_StartCurves(em, 5);
        em->rotX = 0;
        break;
    case 1:
        em->rotX += 0x1A8;
        EmFatalis_UpdateCurves(em);
        ((EmFatalisWork *)work)->runDist -= em->velZ;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stance = EM_STANCE_AIR;
            Em_SetMotion(em, FAT_MOT_DIVE, 0, 0);
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
            Em_SetMotion(em, FAT_MOT_DIVE_PULL_UP, 0, 0);
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
            push[0] = (em->floorY - em->posY) / 3.0f;
            push[1] = 0.0f;
            push[2] = 0.0f;
            Mtx_FromAnglesYXZ(&work->moveAngX, mtx);
            Vec_Rotate(pushWorld, push, mtx);
            em->posX += pushWorld[0];
            em->posY += pushWorld[1];
            em->posZ += pushWorld[2];
        }
        em->posY -= 50.0f;
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, FAT_MOT_LAND_SLIDE, 0, 0);
            em->rotX = 0;
            em->stance = EM_STANCE_GROUND;
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, FAT_MOT_RECOVER, 0, 0);
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

/*
 * FAT_ATK_BLASTS: roars three times on the raised floor of
 * FAT_FLY_LAND_AT_HEIGHT, with an effect (EffEm33 kind 0xE) every 10 frames
 * at the next spot of row 6 or 7 (picked by Em.stateRand) of
 * EmFatalis_SweepSpots, 24 in all; then takes off (FAT_FLY_TAKE_OFF_HIGH).
 */
void EmFatalis_Blasts(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_SetMotion(em, FAT_MOT_ROAR, 0, 0);
        Player_SetFlag((Player *) em, 0x20000U);
        em->timer = 10;
        break;
    case 1:
        if (em->stateVar0 < 8) {
            if (--em->timer <= 0) {
                EffEm33_Create(em, 0xE, EmFatalis_SweepSpots[6 + (em->stateRand & 1)][em->stateVar0], 0, 0);
                em->stateVar0++;
                em->timer = 10;
            }
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, FAT_MOT_ROAR, 0, 0);
            em->timer = 10;
        }
        break;
    case 2:
        if (em->stateVar0 < 16) {
            if (--em->timer <= 0) {
                EffEm33_Create(em, 0xE, EmFatalis_SweepSpots[6 + (em->stateRand & 1)][em->stateVar0], 0, 0);
                em->stateVar0++;
                em->timer = 10;
            }
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, FAT_MOT_ROAR, 0, 0);
            em->timer = 10;
        }
        break;
    case 3:
        if (em->stateVar0 < 24) {
            if (--em->timer <= 0) {
                EffEm33_Create(em, 0xE, EmFatalis_SweepSpots[6 + (em->stateRand & 1)][em->stateVar0], 0, 0);
                em->stateVar0++;
                em->timer = 10;
            }
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmFatalis_ChangeState(em, EM_GROUP_FLY, FAT_FLY_TAKE_OFF_HIGH, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 4:
        if (em->remote == 0) {
            EmFatalis_ChangeState(em, EM_GROUP_FLY, FAT_FLY_TAKE_OFF_HIGH, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/* FAT_REACT_FLINCH: flinches (restarting the monster script). */
void EmFatalis_Flinch(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, FAT_MOT_FLINCH, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmFatalis_EndState(em);
        }
        break;
    }
}

/* FAT_REACT_FLINCH_B: the second flinch motion. */
void EmFatalis_FlinchB(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, FAT_MOT_FLINCH_B, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmFatalis_EndState(em);
        }
        break;
    }
}

/*
 * FAT_REACT_FLINCH_TURN_RIGHT: flinches turning, rotY rising 0x28E a frame
 * up to frame 50.
 */
void EmFatalis_FlinchTurnRight(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, FAT_MOT_FLINCH_TURN_RIGHT, 0, 0);
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

/*
 * FAT_REACT_FALL: falls out of the air (velY at least -50, accelY -10) and
 * hits the floor.
 */
void EmFatalis_Fall(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_SetMotion(em, FAT_MOT_FALL, 0, 0);
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
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, FAT_MOT_FALL_LAND, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, FAT_MOT_RECOVER, 4, 0);
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

/*
 * FAT_REACT_KNOCK_DOWN: lies knocked down for 120 frames (hitReactMode 3
 * meanwhile: hits give EM_DMG_HIT_MODE3; sets bit 1 of gScene.emEventFlags
 * for the area objects), then gets up.
 */
void EmFatalis_KnockDown(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, FAT_MOT_DOWN, 0, 0);
        em->timer = 120;
        Em_RestartScript(em);
        gScene.emEventFlags |= 2;
        em->hitReactMode = 3;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, FAT_MOT_DOWN_GET_UP, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->hitReactMode = 0;
            Em_SetMotion(em, FAT_MOT_RECOVER, 0, 0);
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

/*
 * FAT_REACT_STATUS4: collapses and lies under status 4
 * (Em_SpawnEffect06Bone each frame once lying) until the timer runs out,
 * then FAT_IDLE_STATUS4_END.
 */
void EmFatalis_Status4(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, FAT_MOT_LIE_DOWN, 0, 0);
        em->noReact = 1;
        Em_RestartScript(em);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 296.0f, 0U) != 0) {
            em->step++;
            Em_SetMotion(em, FAT_MOT_DOWN, 0xC, 0x124);
        }
        break;
    case 2:
        Em_SpawnEffect06Bone(em, 2);
        if (--em->timer <= 0) {
            em->step++;
            EmFatalis_ChangeState(em, EM_GROUP_IDLE, FAT_IDLE_STATUS4_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmFatalis_ChangeState(em, EM_GROUP_IDLE, FAT_IDLE_STATUS4_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * FAT_REACT_FLINCH_TURN_LEFT: flinches turning, rotY falling 0x28E a frame
 * up to frame 50.
 */
void EmFatalis_FlinchTurnLeft(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, FAT_MOT_FLINCH_TURN_LEFT, 0, 0);
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

/*
 * FAT_REACT_FLINCH_C: EmFatalis_Flinch, setting the motion before
 * restarting the script.
 */
void EmFatalis_FlinchC(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, FAT_MOT_FLINCH, 0, 0);
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

/*
 * FAT_REACT_FALL_B: EmFatalis_Fall, restarting the script after the fall
 * starts and stopping at the end.
 */
void EmFatalis_FallB(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_SetMotion(em, FAT_MOT_FALL, 0, 0);
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
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, FAT_MOT_FALL_LAND, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, FAT_MOT_RECOVER, 0, 0);
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

/*
 * FAT_DEFEAT_DIE: dies (QuestEm_Hunted, the boss camera) and lays its
 * carcass's mine points, keeping them under their bones. Steps 3 and 99 are
 * set elsewhere: at 3 it fades out as the timer runs down (450 frames from
 * the death) and is removed; at 99 it gets up when the motion ends.
 */
void EmFatalis_Die(Em *em, EmWork *work)
{
    em->noPushTimer = 5;
    em->combat = 0;
    switch (em->step) {
    case 0x0:
        em->step++;
        Em_SetMotion(em, FAT_MOT_DIE, 0, 0);
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_DEAD;
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
            em->stance = EM_STANCE_GROUND;
            EmFatalis_EndState(em);
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
        }
        break;
    }
}

/*
 * FAT_DEFEAT_DIE_B: EmFatalis_Die with its own death motion, setting bit 1
 * of gScene.emEventFlags.
 */
void EmFatalis_DieB(Em *em, EmWork *work)
{
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, FAT_MOT_DIE_B, 0, 0);
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_DEAD;
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
            em->stance = EM_STANCE_GROUND;
            EmFatalis_EndState(em);
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
        }
        break;
    }
}

/*
 * FAT_DEFEAT_DIE_FALLING: falls dead out of the air, then dies on the floor
 * as EmFatalis_Die.
 */
void EmFatalis_DieFalling(Em *em, EmWork *work)
{
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_SetMotion(em, FAT_MOT_FALL, 0, 0);
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
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, FAT_MOT_FALL_LAND, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    case 2:
        if (Player_PassedFrameScaled(em, 100.0f, 0) != 0) {
            em->step++;
            em->motionSpeed = 0.0f;
            em->timer = 0;
            em->stance = EM_STANCE_DEAD;
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
            em->stance = EM_STANCE_GROUND;
            EmFatalis_EndState(em);
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
        }
        break;
    }
}

/* Runs the state of group 0 (EM_GROUP_IDLE). */
void EmFatalis_RunIdleState(Em *em, EmWork *work)
{
    switch (em->state) {
    case FAT_IDLE_STAND:
        EmFatalis_Stand(em, work);
        break;
    case FAT_IDLE:
        EmFatalis_Idle(em, work);
        break;
    case FAT_IDLE_ACTION:
        EmFatalis_IdleAction(em, work);
        break;
    case FAT_IDLE_ROAR:
        EmFatalis_IdleRoar(em, work);
        break;
    case FAT_IDLE_STATUS2:
        EmFatalis_Status2Lie(em, work);
        break;
    case FAT_IDLE_STATUS2_END:
        EmFatalis_Status2End(em, work);
        break;
    case FAT_IDLE_STATUS4_END:
        EmFatalis_Status4End(em, work);
        
    default:
        break;
    }
}

/* Runs the state of group 1 (EM_GROUP_MOVE). */
void EmFatalis_RunMoveState(Em *em, EmWork *work)
{
    switch (em->state) {
    case FAT_MOVE_WALK:
        EmFatalis_Walk(em, work);
        break;
    case FAT_MOVE_TURN_TO_TARGET:
        EmFatalis_TurnToTarget(em, work);
        break;
    case FAT_MOVE_TURN_TO_GOAL:
        EmFatalis_TurnToGoal(em, work);
        break;
    }
}

/*
 * Runs the state of group 2 (EM_GROUP_FLY); some states share a function,
 * told apart by its last argument.
 */
void EmFatalis_RunFlyState(Em *em, EmWork *work)
{
    switch (em->state) {
    case FAT_FLY_TAKE_OFF:
        EmFatalis_TakeOff(em, work, 0);
        break;
    case FAT_FLY_LAND:
        EmFatalis_Land(em, work);
        break;
    case FAT_FLY_HOVER:
        EmFatalis_Hover(em, work);
        break;
    case FAT_FLY_HOVER_TURN:
        EmFatalis_HoverTurn(em, work, 0);
        break;
    case FAT_FLY_TO_GOAL:
        EmFatalis_FlyToGoal(em, work);
        break;
    case FAT_FLY_RISE:
        EmFatalis_FlyRise(em, work);
        break;
    case FAT_FLY_STRAIGHT_TO_GOAL:
        EmFatalis_FlyStraightToGoal(em, work);
        break;
    case FAT_FLY_DESCEND:
        EmFatalis_FlyDescend(em, work);
        break;
    case FAT_FLY_RISE_HIGH:
        EmFatalis_FlyRiseHigh(em, work);
        break;
    case FAT_FLY_LEVEL_TO_GOAL:
        EmFatalis_FlyLevelToGoal(em, work);
        break;
    case FAT_FLY_CLIMB_TO_GOAL:
        EmFatalis_FlyClimbToGoal(em, work, 0);
        break;
    case FAT_FLY_TAKE_OFF_AND_RISE:
        EmFatalis_TakeOffAndRise(em, work);
        break;
    case FAT_FLY_CLIMB_TO_GOAL_NEAR:
        EmFatalis_FlyClimbToGoal(em, work, 1);
        break;
    case FAT_FLY_HOVER_TURN_STOP:
        EmFatalis_HoverTurn(em, work, 1);
        break;
    case FAT_FLY_LAND_AT_HEIGHT:
        EmFatalis_LandAtHeight(em, work);
        break;
    case FAT_FLY_TAKE_OFF_HIGH:
        EmFatalis_TakeOff(em, work, 1);
        
    default:
        break;
    }
}

/*
 * Runs the state of group 3 (EM_GROUP_ATTACK); some states share a
 * function, told apart by its last argument.
 */
void EmFatalis_RunAttackState(Em *em, EmWork *work)
{
    switch (em->state) {
    case FAT_ATK_SINGLE_A:
        EmFatalis_SingleAttackA(em, work, 0);
        break;
    case FAT_ATK_SINGLE_B:
        EmFatalis_SingleAttackB(em);
        break;
    case FAT_ATK_FIREBALL:
        EmFatalis_Fireball(em);
        break;
    case FAT_ATK_FIREBALL_B:
        EmFatalis_FireballB(em);
        break;
    case FAT_ATK_SWEEP_NO_EFFECT:
        EmFatalis_Sweep(em, work, 0);
        break;
    case FAT_ATK_AIR_FIREBALL:
        EmFatalis_AirFireball(em, work, 0);
        break;
    case FAT_ATK_CHARGE:
        EmFatalis_Charge(em);
        break;
    case FAT_ATK_HOP_FIREBALL:
        EmFatalis_HopFireball(em);
        break;
    case FAT_ATK_CHARGE_START:
        EmFatalis_ChargeStart(em);
        break;
    case FAT_ATK_AIR_FIREBALL_TRIPLE:
        EmFatalis_AirFireballTriple(em, work);
        break;
    case FAT_ATK_AIR_FIREBALL_B:
        EmFatalis_AirFireball(em, work, 1);
        break;
    case FAT_ATK_AIR_FIREBALL_C:
        EmFatalis_AirFireball(em, work, 2);
        break;
    case FAT_ATK_GLIDE:
        EmFatalis_Glide(em);
        break;
    case FAT_ATK_DIVE:
        EmFatalis_Dive(em, work);
        break;
    case FAT_ATK_SWEEP_A:
        EmFatalis_Sweep(em, work, 1);
        break;
    case FAT_ATK_SWEEP_B:
        EmFatalis_Sweep(em, work, 2);
        break;
    case FAT_ATK_SWEEP_C:
        EmFatalis_Sweep(em, work, 3);
        break;
    case FAT_ATK_SWEEP_D:
        EmFatalis_Sweep(em, work, 4);
        break;
    case FAT_ATK_SWEEP_DOUBLE:
        EmFatalis_SweepDouble(em, work, 4);
        break;
    case FAT_ATK_SINGLE_C:
        EmFatalis_SingleAttackA(em, work, 1);
        break;
    case FAT_ATK_DIVE_B:
        EmFatalis_DiveB(em, work);
        break;
    case FAT_ATK_BLASTS:
        EmFatalis_Blasts(em);
        
    default:
        break;
    }
}

/* Runs the state of group 4 (EM_GROUP_REACT). */
void EmFatalis_RunReactState(Em *em, EmWork *work)
{
    switch (em->state) {
    case FAT_REACT_FLINCH:
        EmFatalis_Flinch(em, work);
        break;
    case FAT_REACT_FLINCH_B:
        EmFatalis_FlinchB(em, work);
        break;
    case FAT_REACT_FLINCH_TURN_RIGHT:
        EmFatalis_FlinchTurnRight(em, work);
        break;
    case FAT_REACT_FALL:
        EmFatalis_Fall(em, work);
        break;
    case FAT_REACT_KNOCK_DOWN:
        EmFatalis_KnockDown(em, work);
        break;
    case FAT_REACT_STATUS4:
        EmFatalis_Status4(em, work);
        break;
    case FAT_REACT_FLINCH_TURN_LEFT:
        EmFatalis_FlinchTurnLeft(em, work);
        break;
    case FAT_REACT_FLINCH_C:
        EmFatalis_FlinchC(em, work);
        break;
    case FAT_REACT_FALL_B:
        EmFatalis_FallB(em, work);
        
    default:
        break;
    }
}

/* Runs the state of group 5 (EM_GROUP_DEFEAT). */
void EmFatalis_RunDefeatState(Em *em, EmWork *work)
{
    switch (em->state) {
    case FAT_DEFEAT_DIE:
        EmFatalis_Die(em, work);
        break;
    case FAT_DEFEAT_DIE_B:
        EmFatalis_DieB(em, work);
        break;
    case FAT_DEFEAT_DIE_FALLING:
        EmFatalis_DieFalling(em, work);
        break;
    }
}

/*
 * FAT_EVENT_LEAVE: unhittable, flinches, roars, takes off and rises to the
 * height 7500.
 */
void EmFatalis_EventLeave(Em *em, EmWork *work)
{

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, FAT_MOT_FLINCH, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, FAT_MOT_ROAR, 0, 0);
            break;
        }
    default:
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, FAT_MOT_TAKE_OFF, 0, 0);
            EmFatalis_StartCurves(em, 0U);
        }
        break;
    case 3:
        if (Player_PassedFrameScaled(em, 134.0f, 0U) != 0) {
            em->step++;
            em->stance = EM_STANCE_AIR;
            EmFatalis_UpdateCurves(em);
        }
        break;
    case 4:
        if (EmFatalis_UpdateCurves(em) != 0) {
            em->step++;
            Em_SetMotion(em, FAT_MOT_FLY, 0, 0);
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

/*
 * FAT_EVENT_FLY_IN, EM_KIND_FATALIS's scene in area 0x19: from a fixed spot
 * it takes off, climbs to the height 5860, flies to a second point, lands
 * with a camera shake, roars and breathes a fireball, then idles until
 * event flag 0xB0 is set.
 */
void EmFatalis_EventFlyIn(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        GmEm_ResetSize(em);
        em->step++;
        em->stance = EM_STANCE_GROUND;
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
        Em_SetMotion(em, FAT_MOT_IDLE, 0, 0);
        em->timer = 150;
        Player_ClearVelocity(em);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, FAT_MOT_TAKE_OFF, 0, 0);
            EmFatalis_StartCurves(em, 0);
            work->flyGuard = 0;
        }
        break;
    case 2:
        if (Player_PassedFrameScaled(em, 134.0f, 0U) != 0) {
            em->step++;
            em->stance = EM_STANCE_AIR;
            EmFatalis_UpdateCurves(em);
        }
        break;
    case 3:
        if (EmFatalis_UpdateCurves(em) != 0) {
            em->step++;
            Em_SetMotion(em, FAT_MOT_FLY, 0, 0);
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
            Em_SetMotion(em, FAT_MOT_TOUCH_DOWN, 0, 0);
            em->posY = em->floorY;
            em->rotX = 0;
            em->rotZ = 0;
            em->stance = EM_STANCE_GROUND;
            CamShake_ReqAIf(em, 1);
        }
        break;
    case 8:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, FAT_MOT_RECOVER, 0, 0);
        }
        break;
    case 9:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, FAT_MOT_ROAR, 0, 0);
        }
        break;
    case 10:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, FAT_MOT_FIREBALL, 0, 0);
        }
        break;
    case 11:
        if (Player_PassedFrameScaled(em, 102.0f, 0U) != 0) {
            EmFatalis_SpawnFireball(em, 0, 0);
            EmFatalis_SpawnFireballEffect(em, 0, 0);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, FAT_MOT_IDLE, 0, 0);
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

/*
 * FAT_EVENT_LEAVE_FROM_AIR: from the hover, breathes a fireball, then rises
 * to the height 7500.
 */
void EmFatalis_EventLeaveFromAir(Em *em)
{

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_SetMotion(em, FAT_MOT_AIR_FIREBALL_AIM, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, FAT_MOT_AIR_FIREBALL, 0, 0);
        }
        break;
    case 2:
        if (Player_PassedFrameScaled(em, 30.0f, 0U) != 0) {
            EmFatalis_SpawnFireball(em, 0, 0);
            EmFatalis_SpawnFireballEffect(em, 0x38E4, 0);
        }
        if (Player_PassedFrameScaled(em, 76.0f, 0U) != 0) {
            em->step++;
            Em_SetMotion(em, FAT_MOT_FLY, 0xA, 0x1E);
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

/*
 * FAT_EVENT_GET_UP, EM_KIND_FATALIS_B's scene in area 0x58: lying at a
 * fixed spot (FAT_MOT_DIE_B from frame 638), it gets up, idles, roars
 * (enraged meanwhile), glides to a second point and lands, then idles until
 * event flag 0xB1 is set.
 */
void EmFatalis_EventGetUp(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        GmEm_ResetSize(em);
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->posX = 15779.0f;
        em->posY = 571.0f;
        em->posZ = 3495.0f;
        em->goal[0] = 9896.0f;
        em->goal[1] = 46.0f;
        em->goal[2] = 15107.0f;
        em->rotX = 0;
        em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        em->rotZ = 0;
        Em_SetMotion(em, FAT_MOT_DIE_B, 0, 0x27E);
        em->timer = 90;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, FAT_MOT_DOWN_GET_UP, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, FAT_MOT_RECOVER, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, FAT_MOT_IDLE, 0x12, 0);
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
            Em_SetMotion(em, FAT_MOT_IDLE, 0, 0);
            em->timer = 180;
        }
        break;
    case 6:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, FAT_MOT_ROAR, 0, 0);
            em->rage = 1;
            em->rageTimer = 1800;
        }
        break;
    case 7:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, FAT_MOT_GLIDE_START, 0, 0);
        }
        break;
    case 8:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stance = EM_STANCE_AIR;
            Em_SetMotion(em, FAT_MOT_GLIDE, 0, 0);
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
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            Em_SetMotion(em, FAT_MOT_LAND_SLIDE, 0, 0);
        }
        break;
    case 11:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, FAT_MOT_IDLE, 0, 0);
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

/*
 * FAT_EVENT_DESCEND, EM_KIND_FATALIS_C's scene in area 0x7C: glides in high
 * (hidden for its first 150 frames), comes down at a fixed spot, lands,
 * roars enraged with 15 effects (EffEm33 kind 0xC), one every 10 frames,
 * then idles until event flag 0xB2 is set.
 */
void EmFatalis_EventDescend(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        GmEm_ResetSize(em);
        em->step++;
        em->stance = EM_STANCE_AIR;
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
        Em_SetMotion(em, FAT_MOT_GLIDE, 0, 0);
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
            Em_SetMotionRow(em, FAT_MOT_FLY, 0, 0);
            em->timer = 150;
        }
        break;
    case 3:
        Em_MoveRotated(em, &work->moveAngX);
        if (--em->timer <= 0 || em->posY < 1500.0f + em->floorY) {
            em->step++;
            Em_SetMotion(em, FAT_MOT_DESCEND, 0, 0);
            em->timer = 150;
        }
        break;
    case 4:
        Em_MoveRotated(em, &work->moveAngX);
        if (--em->timer <= 0 || em->posY < em->floorY) {
            em->step++;
            em->posY = em->floorY;
            Em_SetMotion(em, FAT_MOT_TOUCH_DOWN, 0, 0);
            Player_ClearVelocity(em);
            em->timer = 90;
        }
        break;
    case 5:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotionRow(em, FAT_MOT_IDLE, 0, 0);
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
            Em_SetMotionRow(em, FAT_MOT_ROAR, 0, 0);
        }
        break;
    case 8:
        if ((em->motionId[0] == EM_MOTION_ID(0, FAT_MOT_ROAR) && EmMotion_ReachedFrame(em, 0, 60.0f) != 0) || em->motionId[0] != EM_MOTION_ID(0, FAT_MOT_ROAR)) {
            if (em->stateVar1++ % 10 == 0) {
                EffEm33_CreatePlus18(em, 0xC, em->stateVar0++, 0, 0);
            }
        }
        if (em->motion[0].playing == 0) {
            Em_SetMotionRow(em, FAT_MOT_IDLE, 0, 0);
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

/* Runs the state of groups 6 and 7 (EM_GROUP_EVENT). */
void EmFatalis_RunEventState(Em *em, EmWork *work)
{
    em->motionSpeed = 1.0f;
    switch (em->state) {
    case FAT_EVENT_LEAVE:
        EmFatalis_EventLeave(em, work);
        break;
    case FAT_EVENT_FLY_IN:
        EmFatalis_EventFlyIn(em, work);
        break;
    case FAT_EVENT_LEAVE_FROM_AIR:
        EmFatalis_EventLeaveFromAir(em);
        break;
    case FAT_EVENT_GET_UP:
        EmFatalis_EventGetUp(em, work);
        break;
    case FAT_EVENT_DESCEND:
        EmFatalis_EventDescend(em, work);
        break;
    }
}

/*
 * Runs the monster's state: the group's dispatcher with the monster and its
 * work area.
 */
void EmFatalis_RunState(Em *em, EmWork *work)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->stateGroup) {
    case EM_GROUP_IDLE:
        EmFatalis_RunIdleState(em, work);
        break;
    case EM_GROUP_MOVE:
        EmFatalis_RunMoveState(em, work);
        break;
    case EM_GROUP_FLY:
        EmFatalis_RunFlyState(em, work);
        break;
    case EM_GROUP_ATTACK:
        EmFatalis_RunAttackState(em, work);
        break;
    case EM_GROUP_REACT:
        EmFatalis_RunReactState(em, work);
        break;
    case EM_GROUP_DEFEAT:
        EmFatalis_RunDefeatState(em, work);
        break;
    case EM_GROUP_EVENT:
        EmFatalis_RunEventState(em, work);
        break;
    case EM_GROUP_EVENT2:
        EmFatalis_RunEventState(em, work);
        
    default:
        break;
    }
}

/*
 * While enraged (EM_KIND_FATALIS and _B), every fourth frame: a stage
 * object of kind 0x14, type 0x1A (at random 0 or 1);
 * EmFatalis_MotionEffects calls it in frame ranges of the motions.
 */
void EmFatalis_SpawnRageObject(Em *em)
{
    if (em->type != EM_KIND_FATALIS_C && em->rage != 0 && !(gScene.frame & 3)) {
        StageObj_K14_Create(em, 0x1A, (s16) ((u16) Rand_Next(1) & 1), 1.0f);
    }
}

/*
 * EM_KIND_FATALIS_C while enraged: every 30 frames of the listed motions, a
 * stage object of kind 0x1C (type 0x28) on the monster.
 */
void EmFatalis_SpawnRageObjectInMotions(Em *em, EmWork *work)
{
    if (em->type == EM_KIND_FATALIS_C && em->rage != 0) {
        switch (work->sndMotion) {
        case EM_MOTION_ID(0, FAT_MOT_IDLE):
        case EM_MOTION_ID(0, FAT_MOT_WALK):
        case EM_MOTION_ID(0, FAT_MOT_TURN_LEFT):
        case EM_MOTION_ID(0, FAT_MOT_TURN_RIGHT):
        case EM_MOTION_ID(0, FAT_MOT_CHARGE_START):
        case EM_MOTION_ID(0, FAT_MOT_TURN_SMALL):
        case EM_MOTION_ID(0, FAT_MOT_RECOVER):
        case EM_MOTION_ID(0, FAT_MOT_TAKE_OFF):
        case EM_MOTION_ID(0, FAT_MOT_FLY):
        case EM_MOTION_ID(0, FAT_MOT_DESCEND):
        case EM_MOTION_ID(0, FAT_MOT_TOUCH_DOWN):
        case EM_MOTION_ID(0, FAT_MOT_SWEEP):
        case EM_MOTION_ID(0, 0x13):
        case EM_MOTION_ID(0, 0x1E):
        case EM_MOTION_ID(0, FAT_MOT_AIR_FIREBALL_AIM):
        case EM_MOTION_ID(0, FAT_MOT_GLIDE_START):
        case EM_MOTION_ID(0, FAT_MOT_LAND_SLIDE):
        case EM_MOTION_ID(0, FAT_MOT_DIVE_RISE):
        case EM_MOTION_ID(0, FAT_MOT_DIVE):
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

/*
 * Adds the effects, camera shakes and stage objects of the motion on layer 0
 * at its frames (work->fxMotion keeps the motion id), after its sounds
 * (EmFatalis_MotionSounds) and EM_KIND_FATALIS_C's rage objects.
 */
void EmFatalis_MotionEffects(Em *em, EmFatalisWork *work)
{
    u8 stateGroup;
    u8 type;

    if (em->motionId[0] != work->fxMotion) {
        work->fxMotion = em->motionId[0];
    }
    EmFatalis_MotionSounds(em, (EmWork *) work);
    EmFatalis_SpawnRageObjectInMotions(em, (EmWork *) work);
    switch (work->fxMotion) {
    case EM_MOTION_ID(0, FAT_MOT_IDLE):
        if (EmMotion_InFrameRange(em, 0, 28.0f, 80.0f) || EmMotion_InFrameRange(em, 0, 278.0f, 490.0f)) {
            EmFatalis_SpawnRageObject(em);
        }
        break;
    case EM_MOTION_ID(0, FAT_MOT_WALK):
        EmFatalis_ShakeOnFrame(em, 0xE, 2);
        EmFatalis_ShakeOnFrame(em, 0x54, 2);
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
            EmFatalis_SpawnRageObject(em);
        }
        break;
    case EM_MOTION_ID(0, FAT_MOT_TURN_LEFT):
        EmFatalis_ShakeOnFrame(em, 0x1A, 2);
        EmFatalis_ShakeOnFrame(em, 0x60, 2);
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
            EmFatalis_SpawnRageObject(em);
        }
        if (EmMotion_InFrameRange(em, 0, 46.0f, 70.0f) || EmMotion_InFrameRange(em, 0, 110.0f, 162.0f)) {
            EmFatalis_SpawnRageObject(em);
        }
        break;
    case EM_MOTION_ID(0, FAT_MOT_TURN_RIGHT):
        EmFatalis_ShakeOnFrame(em, 0x20, 2);
        EmFatalis_ShakeOnFrame(em, 0x5E, 2);
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
            EmFatalis_SpawnRageObject(em);
        }
        if (EmMotion_InFrameRange(em, 0, 46.0f, 70.0f) || EmMotion_InFrameRange(em, 0, 110.0f, 162.0f)) {
            EmFatalis_SpawnRageObject(em);
        }
        break;
    case EM_MOTION_ID(0, FAT_MOT_CHARGE_START):
        EmFatalis_ShakeOnFrame(em, 0x10, 2);
        EmFatalis_ShakeOnFrame(em, 0x1C, 2);
        EmFatalis_ShakeOnFrame(em, 0x2C, 2);
        EmFatalis_ShakeOnFrame(em, 0x70, 5);
        if (Player_PassedFrameScaled(em, 38.0f, 0) != 0) {
            if (em->type == EM_KIND_FATALIS_C) {
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
            EmFatalis_SpawnRageObject(em);
        }
        break;
    case EM_MOTION_ID(0, FAT_MOT_CHARGE):
        EmFatalis_ShakeOnFrame(em, 0x1A, 2);
        EmFatalis_ShakeOnFrame(em, 0x26, 2);
        EmFatalis_ShakeOnFrame(em, 0x3C, 2);
        EmFatalis_ShakeOnFrame(em, 0x58, 2);
        EmFatalis_ShakeOnFrame(em, 0x7E, 2);
        EmFatalis_ShakeOnFrame(em, 0x84, 2);
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
            EmFatalis_SpawnRageObject(em);
        }
        break;
    case EM_MOTION_ID(0, FAT_MOT_TURN_SMALL):
        if (Player_PassedFrameScaled(em, 32.0f, 0) != 0) {
            Eff04_Create(em, 0x21);
        }
        if (Player_PassedFrameScaled(em, 114.0f, 0) != 0) {
            Eff04_Create(em, 0x22);
        }
        break;
    case EM_MOTION_ID(0, FAT_MOT_RECOVER):
        EmFatalis_ShakeOnFrame(em, 0x5A, 2);
        if (EmMotion_InFrameRange(em, 0, 20.0f, 70.0f) || EmMotion_InFrameRange(em, 0, 84.0f, 140.0f)) {
            EmFatalis_SpawnRageObject(em);
        }
        break;
    case EM_MOTION_ID(0, FAT_MOT_TAKE_OFF):
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
            EmFatalis_SpawnRageObject(em);
        }
        break;
    case EM_MOTION_ID(0, FAT_MOT_FLY):
        if (Player_PassedFrameScaled(em, 32.0f, 0) != 0) {
            Eff04_Create(em, 0x19);
        }
        if (((Player_PassedFrameScaled(em, 40.0f, 0) != 0) || (Player_PassedFrameScaled(em, 110.0f, 0) != 0)) && (em->posY <= (1000.0f + em->floorY))) {
            StageObj_K14_Create(em, 1, 0, 1.2f);
        }
        if (EmMotion_InFrameRange(em, 0, 44.0f, 68.0f) != 0) {
            EmFatalis_SpawnRageObject(em);
        }
        break;
    case EM_MOTION_ID(0, FAT_MOT_DESCEND):
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
    case EM_MOTION_ID(0, FAT_MOT_TOUCH_DOWN):
        EmFatalis_ShakeOnFrame(em, 0x12, 2);
        EmFatalis_ShakeOnFrame(em, 0x28, 2);
        EmFatalis_ShakeOnFrame(em, 0x86, 5);
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
            EmFatalis_SpawnRageObject(em);
        }
        break;
    case EM_MOTION_ID(0, FAT_MOT_FIREBALL):
        if (EmMotion_InFrameRange(em, 0, 10.0f, 32.0f) || EmMotion_InFrameRange(em, 0, 54.0f, 72.0f) || EmMotion_InFrameRange(em, 0, 158.0f, 212.0f)) {
            if (!(gScene.frame & 3)) {
                StageObj_K14_Create(em, 0x1A, (u16) Rand_Next(1) & 1, 1.0f);
            }
        }
        if (Player_PassedFrameScaled(em, 80.0f, 0) != 0) {
            Eff04_Create(em, 0x11);
        }
        break;
    case EM_MOTION_ID(0, FAT_MOT_FIREBALL_B):
        if (EmMotion_InFrameRange(em, 0, 10.0f, 32.0f) || EmMotion_InFrameRange(em, 0, 54.0f, 72.0f) || EmMotion_InFrameRange(em, 0, 158.0f, 212.0f)) {
            if (!(gScene.frame & 3)) {
                StageObj_K14_Create(em, 0x1A, (u16) Rand_Next(1) & 1, 1.0f);
            }
        }
        if (Player_PassedFrameScaled(em, 80.0f, 0) != 0) {
            Eff04_Create(em, 0x11);
        }
        break;
    case EM_MOTION_ID(0, FAT_MOT_SWEEP):
        EmFatalis_ShakeOnFrame(em, 0x26, 2);
        EmFatalis_ShakeOnFrame(em, 0x56, 2);
        EmFatalis_ShakeOnFrame(em, 0xF4, 2);
        EmFatalis_ShakeOnFrame(em, 0x124, 2);
        if (Player_PassedFrameScaled(em, 80.0f, 0) != 0) {
            Eff04_Create(em, 3);
        }
        if (Player_PassedFrameScaled(em, 70.0f, 0) != 0) {
            Eff04_Create(em, 0x16);
        }
        break;
    case EM_MOTION_ID(0, FAT_MOT_AIR_FIREBALL):
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
    case EM_MOTION_ID(0, 0x12):
        EmFatalis_ShakeOnFrame(em, 0x14, 2);
        EmFatalis_ShakeOnFrame(em, 0x92, 2);
        if (em->type == EM_KIND_FATALIS) {
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
            if ((em->type == EM_KIND_FATALIS_C) && (Player_PassedFrameScaled(em, 34.0f, 0) != 0)) {
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
    case EM_MOTION_ID(0, 0x29):
        EmFatalis_ShakeOnFrame(em, 0x14, 2);
        EmFatalis_ShakeOnFrame(em, 0x92, 2);
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
            if ((em->type == EM_KIND_FATALIS_C) && (Player_PassedFrameScaled(em, 34.0f, 0) != 0)) {
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
    case EM_MOTION_ID(0, 0x13):
        EmFatalis_ShakeOnFrame(em, 0x44, 2);
        EmFatalis_ShakeOnFrame(em, 0x118, 2);
        if (Player_PassedFrameScaled(em, 60.0f, 0) != 0) {
            Eff04_Create(em, 1);
        }
        break;
    case EM_MOTION_ID(0, FAT_MOT_FLINCH):
        EmFatalis_ShakeOnFrame(em, 0x12, 2);
        EmFatalis_ShakeOnFrame(em, 0xBE, 2);
        break;
    case EM_MOTION_ID(0, FAT_MOT_FLINCH_B):
        EmFatalis_ShakeOnFrame(em, 0x1A, 2);
        EmFatalis_ShakeOnFrame(em, 0x2E, 2);
        EmFatalis_ShakeOnFrame(em, 0x42, 2);
        EmFatalis_ShakeOnFrame(em, 0xC2, 2);
        EmFatalis_ShakeOnFrame(em, 0x120, 2);
        if (Player_PassedFrameScaled(em, 64.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x1E, 7, 1.2f);
        }
        if (Player_PassedFrameScaled(em, 66.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x19, 7, 1.2f);
        }
        break;
    case EM_MOTION_ID(0, FAT_MOT_FLINCH_TURN_RIGHT):
        EmFatalis_ShakeOnFrame(em, 0x20, 2);
        EmFatalis_ShakeOnFrame(em, 0x3A, 2);
        EmFatalis_ShakeOnFrame(em, 0xAA, 2);
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
    case EM_MOTION_ID(0, FAT_MOT_FLINCH_TURN_LEFT):
        EmFatalis_ShakeOnFrame(em, 0x20, 2);
        EmFatalis_ShakeOnFrame(em, 0x44, 2);
        EmFatalis_ShakeOnFrame(em, 0xA0, 2);
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
    case EM_MOTION_ID(0, FAT_MOT_DIE):
        EmFatalis_ShakeOnFrame(em, 0x14, 2);
        EmFatalis_ShakeOnFrame(em, 0x82, 2);
        EmFatalis_ShakeOnFrame(em, 0x11C, 2);
        EmFatalis_ShakeOnFrame(em, 0x140, 2);
        EmFatalis_ShakeOnFrame(em, 0x15E, 2);
        EmFatalis_ShakeOnFrame(em, 0x238, 2);
        EmFatalis_ShakeOnFrame(em, 0x2AC, 2);
        if (Player_PassedFrameScaled(em, 314.0f, 0) != 0) {
            StageObj_K14_Create(em, 0, 0, 1.2f);
        }
        if (Player_PassedFrameScaled(em, 418.0f, 0) != 0) {
            StageObj_K14_Create(em, 1, 0, 0.9f);
        }
        break;
    case EM_MOTION_ID(0, FAT_MOT_FALL_LAND):
        EmFatalis_ShakeOnFrame(em, 0xC, 2);
        EmFatalis_ShakeOnFrame(em, 0x20, 2);
        if (Player_PassedFrameScaled(em, 10.0f, 0) != 0) {
            StageObj_K14_Create(em, 0, 1, 1.2f);
        }
        break;
    case EM_MOTION_ID(0, FAT_MOT_DOWN):
        EmFatalis_ShakeOnFrame(em, 0x2E, 5);
        if (Player_PassedFrameScaled(em, 46.0f, 0) || Player_PassedFrameScaled(em, 112.0f, 0) || Player_PassedFrameScaled(em, 320.0f, 0)) {
            StageObj_K14_Create(em, 1, 0x80, 0.6f);
        }
        break;
    case EM_MOTION_ID(0, FAT_MOT_DOWN_GET_UP):
        EmFatalis_ShakeOnFrame(em, 0xDC, 2);
        if (Player_PassedFrameScaled(em, 216.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x1E, 7, 1.2f);
            Effect0D_SpawnForEmScaled(em, 0x19, 7, 1.2f);
        }
        break;
    case EM_MOTION_ID(0, 0x1E):
        if (EmMotion_InFrameRange(em, 0, 190.0f, 198.0f) != 0) {
            EmFatalis_SpawnRageObject(em);
        }
        break;
    case EM_MOTION_ID(0, FAT_MOT_AIR_FIREBALL_AIM):
        if (EmMotion_InFrameRange(em, 0, 10.0f, 70.0f) || EmMotion_InFrameRange(em, 0, 114.0f, 122.0f)) {
            EmFatalis_SpawnRageObject(em);
        }
        if ((Player_PassedFrameScaled(em, 36.0f, 0) != 0) || (Player_PassedFrameScaled(em, 122.0f, 0) != 0)) {
            Eff04_Create(em, 0x13);
        }
        break;
    case EM_MOTION_ID(0, FAT_MOT_ROAR):
        /* the roar's effect, but not in FAT_EVENT_LEAVE or FAT_EVENT_GET_UP;
           the goto keeps the original's block order (a plain || doesn't match) */
        stateGroup = em->stateGroup;
        if (stateGroup != EM_GROUP_EVENT || em->state != FAT_EVENT_LEAVE) {
            if (stateGroup == EM_GROUP_EVENT) {
                if (em->state != FAT_EVENT_GET_UP) {
                    goto roarEffect;
                }
            } else {
roarEffect:
                if (Player_PassedFrameScaled(em, 66.0f, 0) != 0) {
                    Eff04_Create(em, 0x10);
                    StageObj_K0F_CreateOnEm(em, 5, 3, 1.0f);
                    break;
                }
            }
        }
        break;
    case EM_MOTION_ID(0, FAT_MOT_HOP_FIREBALL):
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
    case EM_MOTION_ID(0, FAT_MOT_LIE_DOWN):
        EmFatalis_ShakeOnFrame(em, 0xF2, 2);
        if (Player_PassedFrameScaled(em, 242.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x1E, 7, 1.2f);
        }
        if (Player_PassedFrameScaled(em, 244.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x19, 7, 1.2f);
        }
        break;
    case EM_MOTION_ID(0, FAT_MOT_GLIDE_START):
        if (Player_PassedFrameScaled(em, 68.0f, 0) != 0) {
            Eff04_Create(em, 0x1E);
        }
        if ((Player_PassedFrameScaled(em, 34.0f, 0) != 0) && (em->posY <= (1000.0f + em->floorY))) {
            StageObj_K14_Create(em, 0x13, 0, 3.0f);
        }
        break;
    case EM_MOTION_ID(0, FAT_MOT_LAND_SLIDE):
        if ((EmMotion_InFrameRange(em, 0, 30.0f, 70.0f) != 0) && (em->posY <= (1000.0f + em->floorY)) && !(gScene.frame & 3)) {
            StageObj_K14_Create(em, 0xA, 0x80, 1.0f);
        }
        if ((Player_PassedFrameScaled(em, 96.0f, 0) != 0) && (em->posY <= (1000.0f + em->floorY))) {
            StageObj_K14_Create(em, 0xB, 0, 2.0f);
        }
        break;
    case EM_MOTION_ID(0, FAT_MOT_DIVE_RISE):
        if ((Player_PassedFrameScaled(em, 52.0f, 0) != 0) && (em->posY <= (1000.0f + em->floorY))) {
            StageObj_K14_Create(em, 1, 0, 1.5f);
        }
        break;
    case EM_MOTION_ID(0, FAT_MOT_GLIDE):
        if ((em->posY <= (1000.0f + em->floorY)) && !(gScene.frame & 3)) {
            StageObj_K14_Create(em, 0xA, 0x80, 0.8f);
        }
        break;
    default:
        EmFatalis_MotionEffectsDefault(em);
        break;
    case EM_MOTION_ID(0, FAT_MOT_FALL):
    case EM_MOTION_ID(0, FAT_MOT_DIE_B):
    case EM_MOTION_ID(0, FAT_MOT_DIVE):
    case EM_MOTION_ID(0, FAT_MOT_DIVE_PULL_UP):
        break;
    }
}

/*
 * Shakes the camera (CamShake_ReqAIf with the strength given) when the
 * motion passes the frame given.
 */
void EmFatalis_ShakeOnFrame(void *em, s32 frame, s32 strength)
{
    if (Player_PassedFrameScaled((Em *) em, (f32) frame, 0U) != 0) {
        CamShake_ReqAIf(em, strength);
    }
}

/*
 * Runs EmFatalis_MotionEffects from the second frame on
 * (EmFatalisWork.fxStep).
 */
void EmFatalis_UpdateMotionEffects(Em *em)
{
    EmFatalisWork *w = (EmFatalisWork *)&em->work;

    switch (w->fxStep) {
    case 0:
        w->fxStep++;
        break;
    case 1:
        EmFatalis_MotionEffects(em, w);
        break;
    }
}

/*
 * The family's callbacks 0 and 1 (EmFatalis_Callbacks): attached effect 1
 * (Effect01_Spawn).
 */
void EmFatalis_SpawnEffect01(u8 *em)
{
    Effect01_Spawn((Player *) em, 0);
}

void EmFatalis_TableNop(void)
{
}

/*
 * Plays the sounds of the motion on layer 0 at its frames
 * (Em_PlaySeAtBoneOnFrame: frame, sound id, bone) for Blangonga and Blango
 * (EmBlangonga_MotionEffects calls it each frame), switching on the motion
 * id in EmWork.prevMotion.
 */
void EmBlangonga_MotionSounds(Em *em, EmWork *work)
{
    s16 prevMotion;

    prevMotion = work->prevMotion;
    switch (prevMotion) {
    case EM_MOTION_ID(0, 0x03):
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x9E, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x5E, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x90, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, 0x05):
        Em_PlaySeAtBoneOnFrame(em, 0x16, Sound_RandomSe(0x19, 2, 0x19, 2), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x52, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, 0x06):
        Em_PlaySeAtBoneOnFrame(em, 0x16, Sound_RandomSe(0x19, 2, 0x19, 2), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x52, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 3, 0x17);
        break;
    case EM_MOTION_ID(0, 0x07):
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x2C, 1, 0x2C, 2), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 3, 0x17);
        break;
    case EM_MOTION_ID(0, 0x08):
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x2C, 1, 0x2C, 2), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, 0x09):
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
    case EM_MOTION_ID(0, 0x0A):
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
    case EM_MOTION_ID(0, 0x0C):
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x1D, 1, 0x1E, 1), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x14, 3);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 5, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 5, 0x1B);
        break;
    case EM_MOTION_ID(0, 0x0E):
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x1D, 1, 0x1E, 1), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x14, 3);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 5, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 5, 0x17);
        break;
    case EM_MOTION_ID(0, 0x10):
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
    case EM_MOTION_ID(0, 0x11):
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
    case EM_MOTION_ID(0, 0x13):
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
    case EM_MOTION_ID(0, 0x14):
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x37, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 8, 8, 3);
        Em_PlaySeAtBoneOnFrame(em, 6, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x5E, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 3, 0x17);
        break;
    case EM_MOTION_ID(0, 0x15):
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0x10, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 0x10, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x5A, 0x11, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x68, 0x11, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x76, 0x11, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x84, 0x11, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x92, 0x12, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x9E, 0x12, 9);
        break;
    case EM_MOTION_ID(0, 0x16):
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
    case EM_MOTION_ID(0, 0x17):
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, 0x1F):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x2A, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 0x29, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xA2, 0x2A, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x104, 0x29, 0x13);
        break;
    case EM_MOTION_ID(0, 0x20):
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
    case EM_MOTION_ID(0, 0x21):
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
    case EM_MOTION_ID(0, 0x22):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x32, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0x33, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 4, 6, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xA2, 0x10, 0xB);
        Em_PlaySeAtBoneOnFrame(em, 0xB4, 7, 3);
        break;
    case EM_MOTION_ID(0, 0x28):
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
    case EM_MOTION_ID(0, 0x29):
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0x38, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 4, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 4, 5, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x36, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 3, 0x17);
        break;
    case EM_MOTION_ID(0, 0x2A):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x5C, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x13, 0xE);
        break;
    case EM_MOTION_ID(0, 0x2B):
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
    case EM_MOTION_ID(0, 0x2C):
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
    case EM_MOTION_ID(0, 0x2E):
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
    case EM_MOTION_ID(0, 0x2F):
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
    case EM_MOTION_ID(0, 0x30):
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
    case EM_MOTION_ID(0, 0x33):
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
    case EM_MOTION_ID(0, 0x34):
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
    case EM_MOTION_ID(0, 0x35):
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
    case EM_MOTION_ID(0, 0x36):
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
    case EM_MOTION_ID(0, 0x37):
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
    case EM_MOTION_ID(0, 0x38):
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x2C, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x7A, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, 0x39):
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
    case EM_MOTION_ID(0, 0x3A):
        Em_PlaySeAtBoneOnFrame(em, 0x36, 0x2C, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x7C, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, 0x3B):
        Em_PlaySeAtBoneVolOnFrame(em, 0x3A, 0x4C, 0x13, 7);
        Em_PlaySeAtBoneVolOnFrame(em, 0xAA, 0x3F, 0x13, 7);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x94, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xA2, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x94, 1, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0xA2, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, 0x3C):
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
    case EM_MOTION_ID(0, 0x3D):
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x48, 4, 0x47, 4), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 0x23, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 4, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, 0x3E):
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
    case EM_MOTION_ID(0, 0x3F):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x51, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x20, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 0x11, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 7, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x74, 4, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0x12, 0x17);
        break;
    case EM_MOTION_ID(0, 0x40):
        Em_PlaySeAtBoneOnFrame(em, 6, 0x54, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x12, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, 0x41):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x48, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x4E, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x7C, 0x25, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x8A, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0xB4, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, 0x42):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x4F, 0x13);
        break;
    case EM_MOTION_ID(0, 0x43):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x47, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 8, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 7, 3);
        break;
    case EM_MOTION_ID(0, 0x44):
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
    case EM_MOTION_ID(0, 0x45):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x47, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 7, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x10, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0x12, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 7, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 2, 0xE);
        break;
    case EM_MOTION_ID(0, 0x46):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x47, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 7, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x10, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0x12, 0xB);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 7, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 2, 9);
        break;
    case EM_MOTION_ID(0, 0x47):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x49, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x11, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x88, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x12, 0x17);
        break;
    case EM_MOTION_ID(0, 0x49):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x48, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 8, 0x10, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x12, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 9, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 7, 0x1B);
        break;
    case EM_MOTION_ID(0, 0x4A):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x58, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x31, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 6, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 2, 9);
        break;
    case EM_MOTION_ID(0, 0x4B):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x4F, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x60, 0x22, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x56, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x66, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x68, 7, 0x13);
        break;
    case EM_MOTION_ID(0, 0x4C):
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x4B, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x68, 1, 9);
        break;
    case EM_MOTION_ID(0, 0x4D):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x51, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x20, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 0x11, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 7, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x74, 4, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0x12, 0x1B);
        break;
    case EM_MOTION_ID(0, 0x4E):
        Em_PlaySeAtBoneOnFrame(em, 6, 0x54, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x12, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 3, 0x17);
        break;
    case EM_MOTION_ID(0, 0x50):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x5C, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x5E, 0x61, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 0x18, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 0x18, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 2, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x5A, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x5E, 2, 0xE);
        break;
    case EM_MOTION_ID(0, 0x51):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x2C, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x5C, 3, 0x17);
        break;
    case EM_MOTION_ID(0, 0x55):
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
    case EM_MOTION_ID(0, 0x56):
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
    case EM_MOTION_ID(0, 0x57):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x22, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 0x23, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x54, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x96, 3, 0x17);
        break;
    case EM_MOTION_ID(0, 0x58):
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
    case EM_MOTION_ID(0, 0x59):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x36, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x42, 0x35, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x8C, 0x39, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x92, 2, 0xE);
        
    case EM_MOTION_ID(0, 0x01):
    default:
        break;
    }
}

/* EmBlangonga_MotionSounds for Congalala and Conga. */
void EmBlangonga_MotionSoundsCongalala(Em *em, EmWork *work)
{
    s16 prevMotion;

    prevMotion = work->prevMotion;
    switch (prevMotion) {
    case EM_MOTION_ID(0, 0x01):
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0x53, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xA0, 0x3F, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x11E, 0x40, 0x13);
        break;
    case EM_MOTION_ID(0, 0x03):
        Em_PlaySeAtBoneOnFrame(em, 0x34, Sound_RandomSe(0x3F, 2, 0x3F, 2), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x76, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, 0x05):
        Em_PlaySeAtBoneOnFrame(em, 0x38, Sound_RandomSe(0x40, 2, 0x41, 2), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x52, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, 0x06):
        Em_PlaySeAtBoneOnFrame(em, 0x38, Sound_RandomSe(0x40, 2, 0x41, 2), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x52, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 3, 0x17);
        break;
    case EM_MOTION_ID(0, 0x07):
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x3C, 1, 0x3D, 2), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x36, Sound_RandomSe(0x41, 1, 0x41, 2), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 3, 0x17);
        break;
    case EM_MOTION_ID(0, 0x08):
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x3C, 1, 0x3D, 2), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x36, Sound_RandomSe(0x41, 1, 0x41, 2), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, 0x09):
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
    case EM_MOTION_ID(0, 0x0A):
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
    case EM_MOTION_ID(0, 0x10):
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
    case EM_MOTION_ID(0, 0x11):
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
    case EM_MOTION_ID(0, 0x12):
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x60, 0x13);
        break;
    case EM_MOTION_ID(0, 0x13):
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
    case EM_MOTION_ID(0, 0x14):
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x42, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 0x2E, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 8, 8, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 9, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x74, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x80, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 3, 0x17);
        break;
    case EM_MOTION_ID(0, 0x1F):
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x51, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 0x52, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xAA, 0x51, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xFA, 0x52, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x13C, 0x35, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x1E6, 0x3F, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xFA, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x110, 6, 0xE);
        break;
    case EM_MOTION_ID(0, 0x20):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x10, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 0x3D, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 0x12, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x8C, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xA0, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x6E, 3, 0x17);
        break;
    case EM_MOTION_ID(0, 0x21):
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
    case EM_MOTION_ID(0, 0x22):
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0x53, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xC8, 0x35, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 0x10, 9);
        Em_PlaySeAtBoneOnFrame(em, 4, 6, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0xA2, 0x11, 0xB);
        Em_PlaySeAtBoneOnFrame(em, 0xB4, 7, 3);
        break;
    case EM_MOTION_ID(0, 0x28):
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0x22, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0x5C, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 0x58, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x112, 0x3D, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x162, 0x40, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x12A, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 0x19, 0x13);
        break;
    case EM_MOTION_ID(0, 0x29):
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
    case EM_MOTION_ID(0, 0x2A):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x23, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x13, 0xE);
        break;
    case EM_MOTION_ID(0, 0x2B):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x21, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0xB, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x5A, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x5C, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 4, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x5C, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, 0x2C):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x21, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0xB, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x5A, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x5C, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 4, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x5C, 3, 0x17);
        break;
    case EM_MOTION_ID(0, 0x2D):
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
    case EM_MOTION_ID(0, 0x2E):
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
    case EM_MOTION_ID(0, 0x30):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x39, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0x42, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x60, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 0x13, 3);
        Em_PlaySeAtBoneOnFrame(em, 2, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 4, 0x1B);
        break;
    case EM_MOTION_ID(0, 0x31):
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
    case EM_MOTION_ID(0, 0x33):
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
    case EM_MOTION_ID(0, 0x34):
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
    case EM_MOTION_ID(0, 0x35):
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
    case EM_MOTION_ID(0, 0x36):
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
    case EM_MOTION_ID(0, 0x37):
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
    case EM_MOTION_ID(0, 0x38):
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x43, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x7A, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, 0x39):
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
    case EM_MOTION_ID(0, 0x3C):
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
    case EM_MOTION_ID(0, 0x3D):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x46, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 4, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 4, 0x1B);
        break;
    case EM_MOTION_ID(0, 0x3E):
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
    case EM_MOTION_ID(0, 0x3F):
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x4F, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 0x11, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 7, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x74, 4, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0x12, 0x17);
        break;
    case EM_MOTION_ID(0, 0x4D):
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x4F, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 0x11, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 7, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x74, 4, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0x12, 0x1B);
        break;
    case EM_MOTION_ID(0, 0x40):
        Em_PlaySeAtBoneOnFrame(em, 6, 0x4E, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x12, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, 0x4E):
        Em_PlaySeAtBoneOnFrame(em, 6, 0x4E, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x12, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 3, 0x17);
        break;
    case EM_MOTION_ID(0, 0x41):
        Em_PlaySeAtBoneOnFrame(em, 6, 0x46, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 0x50, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x7A, 0x12, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0xB8, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, 0x42):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x60, 0x13);
        break;
    case EM_MOTION_ID(0, 0x43):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x47, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 8, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 7, 3);
        break;
    case EM_MOTION_ID(0, 0x44):
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
    case EM_MOTION_ID(0, 0x45):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x48, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 7, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x10, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0x12, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 7, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 2, 0xE);
        break;
    case EM_MOTION_ID(0, 0x46):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x48, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 7, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x10, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0x12, 0xB);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 7, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 2, 9);
        break;
    case EM_MOTION_ID(0, 0x47):
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x47, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 0x41, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x11, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x88, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x12, 0x17);
        break;
    case EM_MOTION_ID(0, 0x49):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x48, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x10, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 6, 0x12, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 9, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 7, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 5, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 4, 0x1B);
        break;
    case EM_MOTION_ID(0, 0x4A):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x5E, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 4, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 5, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x36, 4, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 5, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 4, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x70, 5, 0x1B);
        break;
    case EM_MOTION_ID(0, 0x4B):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x48, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x68, 7, 0x13);
        break;
    case EM_MOTION_ID(0, 0x4C):
        Em_PlaySeAtBoneOnFrame(em, 0x22, 0x12, 3);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 5, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 4, 0x17);
        break;
    case EM_MOTION_ID(0, 0x4F):
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
    case EM_MOTION_ID(0, 0x50):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x10, 3);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x35, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 0x43, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 4, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 4, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x5C, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, 0x51):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x2D, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x5C, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x35, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x5C, 0x35, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 4, 6, 9);
        break;
    case EM_MOTION_ID(0, 0x52):
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0x57, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x5E, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x62, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x84, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x5E, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x8E, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, 0x53):
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
    case EM_MOTION_ID(0, 0x54):
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
    case EM_MOTION_ID(0, 0x55):
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
    case EM_MOTION_ID(0, 0x56):
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
    case EM_MOTION_ID(0, 0x57):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x22, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0x43, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x54, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x96, 3, 0x17);
        break;
    case EM_MOTION_ID(0, 0x58):
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

/* EmBlangonga_MotionSounds for Rajang. */
void EmBlangonga_MotionSoundsRajang(Em *em, EmWork *work)
{
    s16 prevMotion;
    u8 stateGroup;

    prevMotion = work->prevMotion;
    switch (prevMotion) {
    case EM_MOTION_ID(0, 0x03):
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x9E, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x5E, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x90, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, 0x05):
        Em_PlaySeAtBoneOnFrame(em, 0x16, Sound_RandomSe(0x2C, 2, 0x2C, 2), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x52, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, 0x06):
        Em_PlaySeAtBoneOnFrame(em, 0x16, Sound_RandomSe(0x2C, 2, 0x2C, 2), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x52, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 3, 0x17);
        break;
    case EM_MOTION_ID(0, 0x07):
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x39, 1, 0x39, 2), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 3, 0x17);
        break;
    case EM_MOTION_ID(0, 0x08):
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x39, 1, 0x39, 2), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, 0x09):
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
    case EM_MOTION_ID(0, 0x0A):
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
    case EM_MOTION_ID(0, 0x0C):
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x1D, 1, 0x1E, 1), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x14, 3);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 5, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 5, 0x1B);
        break;
    case EM_MOTION_ID(0, 0x0E):
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x1D, 1, 0x1E, 1), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x14, 3);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 5, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 5, 0x17);
        break;
    case EM_MOTION_ID(0, 0x10):
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
    case EM_MOTION_ID(0, 0x11):
        Em_PlaySeAtBoneVolOnFrame(em, 0x16, 0x36, 0x13, 7);
        Em_PlaySeAtBoneVolOnFrame(em, 0x36, 0x24, 0x13, 7);
        Em_PlaySeAtBoneOnFrame(em, 4, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 8, 0x12, 9);
        Em_PlaySeAtBoneOnFrame(em, 8, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 5, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 4, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 0x14, 3);
        break;
    case EM_MOTION_ID(0, 0x12):
        Em_PlaySeAtBoneOnFrame(em, 0x22, 0x14, 3);
        break;
    case EM_MOTION_ID(0, 0x13):
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
    case EM_MOTION_ID(0, 0x14):
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x37, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 0x39, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 8, 9, 3);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 7, 3);
        Em_PlaySeAtBoneOnFrame(em, 6, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x6E, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x84, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 3, 0x17);
        break;
    case EM_MOTION_ID(0, 0x16):
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
    case EM_MOTION_ID(0, 0x17):
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, 0x1F):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x2A, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 0x29, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xA2, 0x2A, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x104, 0x29, 0x13);
        break;
    case EM_MOTION_ID(0, 0x20):
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
    case EM_MOTION_ID(0, 0x21):
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
    case EM_MOTION_ID(0, 0x22):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x2A, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0x33, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 4, 6, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xA2, 0x10, 0xB);
        Em_PlaySeAtBoneOnFrame(em, 0xB4, 7, 3);
        break;
    case EM_MOTION_ID(0, 0x28):
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
    case EM_MOTION_ID(0, 0x29):
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x60, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 4, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 4, 5, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x36, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 3, 0x17);
        break;
    case EM_MOTION_ID(0, 0x2A):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x34, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x13, 0xE);
        break;
    case EM_MOTION_ID(0, 0x2B):
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
    case EM_MOTION_ID(0, 0x2C):
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
    case EM_MOTION_ID(0, 0x30):
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
    case EM_MOTION_ID(0, 0x33):
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
    case EM_MOTION_ID(0, 0x34):
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
    case EM_MOTION_ID(0, 0x35):
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
    case EM_MOTION_ID(0, 0x36):
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
    case EM_MOTION_ID(0, 0x37):
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
    case EM_MOTION_ID(0, 0x38):
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x2C, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x7A, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, 0x39):
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
    case EM_MOTION_ID(0, 0x3A):
        Em_PlaySeAtBoneOnFrame(em, 0x36, 0x54, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x7C, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, 0x3B):
        Em_PlaySeAtBoneVolOnFrame(em, 0x3A, 0x4C, 0x13, 7);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x94, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0xA2, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x94, 1, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0xA2, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, 0x3C):
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
    case EM_MOTION_ID(0, 0x3D):
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x48, 4, 0x47, 4), 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 0x23, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 4, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, 0x3E):
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
    case EM_MOTION_ID(0, 0x3F):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x51, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x20, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 0x11, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 7, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x74, 4, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0x12, 0x17);
        break;
    case EM_MOTION_ID(0, 0x40):
        Em_PlaySeAtBoneOnFrame(em, 6, 0x61, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x12, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, 0x41):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x48, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x26, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x7C, 0x25, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x8A, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0xB4, 3, 0x1B);
        break;
    case EM_MOTION_ID(0, 0x42):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x4F, 0x13);
        break;
    case EM_MOTION_ID(0, 0x43):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x47, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 8, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 7, 3);
        break;
    case EM_MOTION_ID(0, 0x44):
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
    case EM_MOTION_ID(0, 0x45):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x47, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 7, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x10, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0x12, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 7, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 2, 0xE);
        break;
    case EM_MOTION_ID(0, 0x46):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x47, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 7, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x10, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0x12, 0xB);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 7, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 2, 9);
        break;
    case EM_MOTION_ID(0, 0x47):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x49, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x11, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x88, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x12, 0x17);
        break;
    case EM_MOTION_ID(0, 0x49):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x48, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 8, 0x10, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x12, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 9, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 7, 0x1B);
        break;
    case EM_MOTION_ID(0, 0x4A):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x58, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x59, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 6, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 2, 9);
        break;
    case EM_MOTION_ID(0, 0x4B):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x4F, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x60, 0x22, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x56, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x66, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x68, 7, 0x13);
        break;
    case EM_MOTION_ID(0, 0x4C):
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x4B, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 6, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x68, 1, 9);
        break;
    case EM_MOTION_ID(0, 0x4D):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x51, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x20, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 0x11, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 7, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x74, 4, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0x12, 0x1B);
        break;
    case EM_MOTION_ID(0, 0x4E):
        Em_PlaySeAtBoneOnFrame(em, 6, 0x61, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0, 9);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x12, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 3, 0x17);
        break;
    case EM_MOTION_ID(0, 0x50):
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
    case EM_MOTION_ID(0, 0x51):
        Em_PlaySeAtBoneOnFrame(em, 0x34, 0x22, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x66, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x84, 2, 0xE);
        break;
    case EM_MOTION_ID(0, 0x52):
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0x1A, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 4, 6, 9);
        Em_PlaySeAtBoneOnFrame(em, 8, 0x12, 9);
        Em_PlaySeAtBoneOnFrame(em, 8, 3, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 5, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 4, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x14, 3);
        break;
    case EM_MOTION_ID(0, 0x53):
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0x16, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x13, 3);
        break;
    case EM_MOTION_ID(0, 0x54):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x37, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x39, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 8, 8, 3);
        Em_PlaySeAtBoneOnFrame(em, 6, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 2, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 1, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 5, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 4, 0x1B);
        break;
    case EM_MOTION_ID(0, 0x55):
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
    case EM_MOTION_ID(0, 0x56):
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
    case EM_MOTION_ID(0, 0x57):
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
    case EM_MOTION_ID(0, 0x58):
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
    case EM_MOTION_ID(0, 0x5A):
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x1D, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 5, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 5, 0x1B);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 0x14, 3);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0x37, 0x13);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 2, 0xE);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 1, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 8, 3);
        
    case EM_MOTION_ID(0, 0x01):
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
    func_0090A9C0, EmRathalos_Reinit, EmRathalos_AllowPartBreak, NULL,
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
