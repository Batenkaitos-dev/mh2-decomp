/*
 * The Blangonga family (src/game/em_blangonga.c): Blangonga, Congalala and
 * Rajang, and the small Blango and Conga (EM_KIND_BLANGONGA, _CONGALALA,
 * _RAJANG, _BLANGO, _CONGA): the code from the family's setup function,
 * EmBlangonga_Setup (0x80CDF0), to the next family's, and after it three
 * pieces of the Shen Gaoren and Kushala Daora families' code.
 *
 * A monster of the family runs a state machine (include/em_blangonga.h
 * names the states and motions; include/em.h the groups): EmBlangonga_Update
 * reads the hits (Em_UpdateDamage) and picks the reactions, then
 * EmBlangonga_RunState calls the group's dispatcher (EmBlangonga_RunIdleState
 * ... RunVersusState), which calls one function per state with the monster,
 * its work area (EmWork; the family's view is EmBlangongaWork) and, for the
 * functions several states share, a variant. A state function switches on
 * Em.step; EmBlangonga_ChangeState starts the next state and
 * EmBlangonga_EndState hands the choice back to the monster script, which
 * asks its questions through EmBlangonga_ScriptQuery. The family doesn't
 * fly: its group 2 holds the jumps, the leaps between areas and the
 * Blangonga kinds' burrowing under the snow. Congalala carries one of three
 * items it can drop as a mine point, and eats to change its breath
 * (EmBlangongaWork.carryHidden, eatenKind); Blangonga's and Congalala's
 * story quests play scenes (group 6). EmBlangonga_MotionEffects adds each
 * motion's hits and effects; the kinds' sounds are in em_rathalos.c.
 *
 * After the family: Shen Gaoren's Set*State and ChangeState
 * (EmShenGaoren_*), Kushala Daora's wind, Set*State, ChangeState and curves
 * (EmKushala_*), and Shen Gaoren's motion sounds; em_shengaoren.c and
 * em_kushala.c hold the rest of those families.
 */
#include "common.h"
#include "scene.h"
#include "quest.h"
#include "em.h"
#include "em_blangonga.h"
#include "player.h"
#include "area.h"
/* prototypes of callees */
s32 EmBlangonga_RageTimerFull(Em *em);

/* prototypes */
void EmBlangonga_ChangeState(Em *em, u16 group, u16 state, u8 mode);
s8 EmMine_Create(Em *em, s32 idx);
s8 EmMine_CreateTimed(Em *em, f32 *pos, s32 idx);
s8 EmMine_CreateTimed0(Em *em, f32 *pos);
void EmMine_Remove(Em *em);
s32 EmMine_Update(Em *em);
/* unit's own declaration: with the definition's void (Em *em, s16 n) (game/unk_708850.c), EmBlangonga_EatNeed2: 35 of 75 words differ */
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
/* unit's own declaration: with the definition's void (Em *em, s32 bone, s32 a, s32 b, s32 c, s32 d) (game/unk_6CADF0.c), EmBlangonga_DigThrow: 30 of 89 words differ */
void EmRangedAttack_CreateAtBone(Em *, s32, s32, s32, u16, s32);
void BlangongaAttack_Create(Em *em, s32 kind);
/* unit's own declaration: with the definition's void *(Em *em, s32 kind, s32 c, s32 a, s32 b) (game/unk_6E3B00.c), EmBlangonga_CongalalaEatenEmission: 24 of 73 words differ */
void * EmEffectAttack_Create(Em *, s16, s16, s32, s32);
void Em_SelectSightRow(Em *em, u8 idx);
void Em_MoveRotatedAccel(Em *em, void *ang);
/* unit's own declaration: with the definition's void (Em *em, s32 group, s32 state, u8 mode) (game/unk_701810.c), EmBlangonga_ChangeState: 3 of 671 words differ, EmKushala_ChangeState: 27 of 217 words differ ... */
void Em_SetStateSync(Em *em, u16 group, u16 state, u8 mode);
void Em_CheckRageByHp(Em *em, s16 n);
void Em_LoadReactionSet(Em *em, u8 k);
void Em_ClearLowHpNeed(Em *em);
void Em_ClearNeed2State(Em *em);
void Em_SpawnStatus4Effect(Em *em, s32 a);
void Em_AddNeed2(Em *em, s32 n);
void Em_SetHitReactMode(Em *em, u8 a);
void Em_SetTrapTimeGoal(Em *em);
/* unit's own declaration: with the definition's s32 (Em *em, s16 n, s16 max, s16 interval) (game/unk_708850.c), EmBlangonga_Rest: 82 of 132 words differ */
s32 Em_RegenHp(Em *em, s32 n, s32 max, s32 interval);
void Em_EndRage(Em *em);
void Em_ClearPoison(Em *em);
void Em_SetHold2(Em *em);
void Em_SetCaptured(Em *em, s32 a);
void Em_PlaySeTypeAtBoneInFrames(Em *em, s32 frame, s32 frame2, s32 se, s32 bone, s32 vol, s32 a);
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
s32 Em_ComputeMaxHp(Em *em, s16 idx);
void Em_InitParts(Em *em);
s32 Quest_IsVersus(void);
s32 Versus_TeamHasFlag(Em *em, s32 kind);
void EmBlangonga_ShowCarried(Em *em, u16 part);
f32 Math_Absf(f32 x);
f32 Math_Sin(f32 x);
void EmBlangonga_SpawnPeriodicEffect06(Em *em, s32 kind);
/* unit's own declaration: with the definition's void (Em *em, s32 bone, f32 *pos, f32 scale) (game/unk_7077A0.c), EmBlangonga_SpawnPeriodicEffect06: 7 of 32 words differ */
void Em_SpawnLyingPuffs(Em *em, u8 bone, f32 *pos, f32 scale);
void Em_ApplyPartBreaks(Em *em);
void GmEm_ResetSize(Em *em);
EmBone *Em_Bone(void *obj, s16 bone);
s32 Em_PlaySeChecked(Em *em, s32 id, s32 a, void *pos, s32 b, s32 c);
extern EmBlangongaVec EmShenGaoren_SeOffsets[];
u16 EmBlangonga_PickCarried(EmBlangongaPickRow *p, u16 r);
extern EmBlangongaTurnData EmBlangonga_TurnData;
extern EmBlangongaVec EmBlangonga_CarveThrowPt __attribute__((aligned(8)));
void StageObj_K25_Create(Player *p, Em *em, u8 k, s32 unused, f32 *pos);
void Versus_RouteEmByTeam(Em *em);
s32 Versus_DefeatCamera(Em *pl);
void Em_FlagPartBreak(Em *em, s32 arg1);
void EmBlangonga_StartCurves(Em *em, u8 kind);
void EmBlangonga_RunIdleState(Em *em, EmWork *work);
void EmBlangonga_Turn(Em *, s32, s32);
void EmBlangonga_Approach(Em *, s32, s32);
void EmBlangonga_Jump(Em *, EmWork *, u8);
void EmBlangonga_Hop(Em *, EmWork *, s32);
void EmBlangonga_DigOut(Em *, EmWork *, s32);
void EmBlangonga_BuriedTurn(Em *);
void EmBlangonga_LeapOutOfArea(Em *, EmWork *);
void EmBlangonga_DropIn(Em *, EmWork *);
void EmBlangonga_CrossArea(Em *, EmWork *);
void EmBlangonga_RunAttackState(Em *em, EmWork *work);
void EmBlangonga_RunReactState(Em *em, EmWork *work);
void EmBlangonga_Die(Em *, s32);
void EmBlangonga_DieHeld(Em *, s32);
void EmBlangonga_DieFalling(Em *, s32);
void EmBlangonga_Reappear(Em *em, s32 arg1);
void EmBlangonga_RunEventState(Em *em, EmWork *work);
void EmBlangonga_VersusEatBait(Em *, EmWork *);
void EmBlangonga_TurnAbout(Em *em, EmWork *work, s32 variant);
void EmBlangonga_EatNeed1(Em *em, EmWork *work, s32 variant);
void EmBlangonga_Slam(Em *em, EmWork *work, s32 variant);
void EmBlangonga_Strike(Em *em, EmWork *work, s32 variant);
void EmBlangonga_DigThrow(Em *em, EmWork *work, s32 variant);
void EmBlangonga_RajangComboSlam(Em *em, EmWork *work, s32 variant);
void EmBlangonga_RajangShot(Em *em, EmWork *work, s32 variant);
void EmBlangonga_RajangBeam(Em *em, EmWork *work, s32 variant);
void EmBlangonga_KnockDown(Em *em, EmWork *work, u8 variant);
/* unit's own declaration: with the definition's void (Em *em, u8 k, s16 v, s16 t, f32 y, f32 scale) (game/unk_874B90.c), EmBlangonga_KnockDown: 116 of 165 words differ, EmBlangonga_HeldDowned: 109 of 175 words differ */
void StageObj_Stun_CreateEm(Em *, s32, s32, s32, f32, f32);
void EmBlangonga_HeldDowned(Em *em, EmWork *work, u8 variant);
void EmBlangonga_BlangoEmerge(Em *em, EmWork *work, u8 variant);
void EmBlangonga_CongalalaEatenEmission(Em *em, EmWork *work);
void EmBlangonga_CongalalaCombo(Em *em, EmWork *work);
void EmBlangonga_EffectStrike(Em *em, EmWork *work);
void EmBlangonga_CongalalaScene(Em *em, EmWork *work);
void EmBlangonga_StartBlangongaScene(Em *em, EmWork *work);
void EmBlangonga_BlangongaLeapDown(Em *em, EmWork *work);
void EmBlangonga_MotionEffects(Em *, EmWork *);
u8 EmBlangonga_ScriptQuery(Em *, u8);
void EmShenGaoren_SetGroup1State(Em *em, u16 state, u8 mode);
void EmShenGaoren_SetGroup4State(Em *em, u16 state, u8 mode);
void EmShenGaoren_PlaySeAtOffset(Em *em, s32 se, s32 idx, s32 vol);
void StageObj_K0F_CreateOnEm(Em *em, u8 k, u8 c, f32 scale);
void StageObj_K1C_CreateOnEmB(Em *em, u8 k, u16 w, s16 v, f32 scale);
s32 Sound_RandomSe(s32 id, s16 lo, s32 alt, s16 n);
s32 EmMotion_PassedFrameScaled(void *p, f32 frame, s32 layer);
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
void Em_RefillNeed3(Em *em);
void Em_BeginStatus2(Em *em);
void Em_EndStatus1(Em *em);
void Em_EndStatus2(Em *em);
void Em_EndStatus4(Em *em);
void Em_EndStatus5(Em *em);
void Em_StartGoalAction(Em *em, s32 arg1);
void Em_AddNeed1(Em *em, s32 n);
void Em_ClearNeed1State(Em *em);
void Vec_Copy(f32 *out, f32 *in);
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
static s32 EmBlangonga_IsBlangonga(Em *em);
static s32 EmBlangonga_IsCongalala(Em *em);
s32 Em_IsSyncFree(Em *em, u8 mode);
u16 Map_DayId(u16);
u16 Area_DayId(u16 id);
void ObjListA70_Remove(EmObj *obj);
void EmBlangonga_HideCarried(Em *em, u16 part);
u16 Quest_Random(void);
u16 Area_IdForCurrentTime(u16);
void StageObj_K1C_CreateOnEmC(Em *em, u8 k, u8 c, f32 scale);
struct StageObj *StageObj_K1C_CreateOnEm(struct Em *em, u8 k, f32 scale);
void EmImpactAttack_CreateAt(Em *em, f32 *pos, s8 kind);
void EmRangedAttack_CreateAtBone2(Em *em, s32 bone, s32 a, s32 b, s32 c, s32 d, s32 e);
void Vec_RotateSelf(f32 *v, f32 *m);
void Mtx_GetTrans(f32 *out, f32 *m);
void Vec_Add(f32 *, f32 *, f32 *);
s32 Area_ContainsPos(s32, f32 *);
void Em_EnterNextArea(Em *);
void Em_MoveRotated(Em *em, void *src);
void EmBlangonga_EnterNewArea(Em *);
void EmBlangonga_DropFallingObjects(Em *em);
void StageObj_K11_CreateAtScaled(f32 *pos, s32 v, s16 k, f32 scale);
s32 Em_UpdateCombatTimer(Em *em);
void Em_RaisePlayerAreaTrigger(Em *em);
void Em_HoldPosNoPlayers(Em *em, u16 a, u16 b);
u8 Em_SenseObjects(Em *em, EmObj *self);
s32 Em_ScriptFlagsFrom(Em *em, u8 mask);
void Em_CheckLowHp(Em *em, f32 ratio);
void Em_DecayNeed1(Em *em);
void Em_DecayNeed1Only(Em *em);
void Em_DecayNeed2(Em *em);
u8 Em_UpdateDamage(Em *em, u8 *out);
void Em_SetTrapTime(Em *em, u8 *p);
void Em_SetStatus1Timer(Em *em);
void Em_SetStatus2Timer(Em *em);
void Em_SetStatus4Timer(Em *em);
void Em_SetStatus5Timer(Em *em);
s32 Em_IsHeld(Em *em);
void Em_ResetKindTimer(Em *em);
s32 Em_DamageResultIsReaction(Em *em, s32 arg1);
s32 Save_TestEventFlag(s32 flag);
void Em_UpdatePendingScript(Em *em);
void Em_ClearHitReactMode(Em *em);
void EmBlangonga_RunState(Em *em, s32 arg1);
void EmBlangonga_BreakParts(Em *em, s32 apply);
/* end of prototypes */

/*
 * The family's setup (Em_TypeSetup's entry for the Blangonga kinds):
 * nothing to do.
 */
void EmBlangonga_Setup(Em *em)
{
}

/*
 * Callback slots 0 and 1: creates the family's effect 01 (type 3 for the
 * large kinds, 0 for the Blango and the Conga). The monster comes as an
 * s32.
 */
void EmBlangonga_SpawnEffect01(s32 em)
{
    if (EmBlangonga_IsSmallKind((Em *) em) == 0) {
        Effect01_Spawn((Player *) em, 3);
        return;
    }
    Effect01_Spawn((Player *) em, 0);
}

/*
 * Sets a monster of the family up: the scaled parts (0-2 for the Blangonga
 * kinds, 2-3 for the Congalala kinds); in the quest-less stage
 * (gQuest.stage 0) a fixed start position per slot; the idle motion and
 * full hp from the size (the Blango and the Conga are always run locally);
 * the rage timer and work cleared; every carried item hidden, then the
 * Congalala kinds show the one EmBlangonga_PickCarried picks; eatenKind
 * from the versus team flags; the kind's body colour; the parts.
 */
void EmBlangonga_Init(Em *em)
{
    EmBlangongaWork *w = (EmBlangongaWork *)&em->work;
    u16 part;

    if (em->type == EM_KIND_BLANGONGA) {
        Em_SetPartScaled(em, 0);
        Em_SetPartScaled(em, 1);
        Em_SetPartScaled(em, 2);
    } else if (em->type == EM_KIND_CONGALALA) {
        Em_SetPartScaled(em, 2);
        Em_SetPartScaled(em, 3);
    }
    if (gQuest.stage == 0) {
        em->posX = 2400.0f + 150.0f * (u32) em->slotNo;
        em->posY = 0.0f;
        em->posZ = 3600.0f;
    }
    Em_SetMotionRow(em, BLANG_MOT_IDLE, 0, 0);
    em->stance = EM_STANCE_GROUND;
    em->hpMax = em->hp = Em_ComputeMaxHp(em, 0);
    em->scriptPending = 1;
    em->blind = 0;
    em->unk_InitFlag = 1;
    if (EmBlangonga_IsSmallKind(em) == 1) {
        em->remote = 0;
    }
    w->rageTimer = 0;
    w->unk03A = 0;
    w->senseListed = 0;
    Em_BonePos(em, 0xF, &w->bonePos[0]);
    w->moveCount = 0;
    w->carryHidden[0] = 1;
    w->carryHidden[1] = 1;
    w->carryHidden[2] = 1;
    if (em->type == EM_KIND_CONGALALA) {
        part = EmBlangonga_PickCarried((EmBlangongaPickRow *) em, w->pickSeed);
        if (part != 0xFFFF) {
            EmBlangonga_ShowCarried(em, part);
        }
    }
    if (em->type == EM_KIND_CONGALALA) {
        if (Quest_IsVersus() == 1) {
            if (Versus_TeamHasFlag(em, 3)) {
                w->eatenKind = 2;
            } else if (Versus_TeamHasFlag(em, 4)) {
                w->eatenKind = 1;
            } else if (Versus_TeamHasFlag(em, 5)) {
                w->eatenKind = 3;
            } else {
                w->eatenKind = 0;
            }
        } else {
            w->eatenKind = 0;
        }
    } else {
        w->eatenKind = 0;
    }
    switch (em->type) {
    case EM_KIND_BLANGONGA:
        w->tint[0] = 0x73;
        w->tint[1] = 0xEB;
        w->tint[2] = 0xFF;
        break;
    case EM_KIND_BLANGO:
        w->tint[0] = 0xCD;
        w->tint[1] = 0x91;
        w->tint[2] = 0xC3;
        break;
    case EM_KIND_CONGALALA:
    case EM_KIND_CONGA:
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
    w->hitSeCooldown = 0;
    Em_InitParts(em);
}

/*
 * Starts a monster: the Congalala kinds first draw pickSeed so that Init
 * and Reinit pick the same carried item; Init; in Congalala's two story
 * quests, before event flag 0x9B, places it at a fixed point of AREA_JUNGLE_3;
 * then BLANG_IDLE_BURIED for a Blango of variant 1, BLANG_IDLE for the
 * others.
 */
void EmBlangonga_Start(Em *em)
{
    EmBlangongaWork *w = (EmBlangongaWork *)&em->work;

    if (em->type == EM_KIND_CONGALALA) {
        w->pickSeed = Quest_Random();
    }
    EmBlangonga_Init(em);
    if (em->type == EM_KIND_CONGALALA) {
        switch (gQuest.stage) {
        case 0x3F3:
        case 0x3F4:
            if (Save_TestEventFlag(0x9B) == 0) {
                em->area = Area_IdForCurrentTime(AREA_JUNGLE_3);
                em->posX = 11870.0f;
                em->posY = 0.0f;
                em->posZ = 10100.0f;
                em->rotY = 0x9000;
            }
            break;
        }
    }
    if (em->type == EM_KIND_BLANGO && em->variant == 1) {
        EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_BURIED, EM_SYNC_LOCAL);
        return;
    }
    EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE, EM_SYNC_LOCAL);
}

/*
 * Callback slot 5: sets the monster up again (Init) without changing its
 * state.
 */
void EmBlangonga_Reinit(Em *em)
{
    EmBlangonga_Init(em);
}

/*
 * Starts state `state` of group `group` (EM_SYNC_* `mode`, passed on to
 * Em_SetStateSync): takes the roar's sensed object off the list, does what
 * the state needs on entry (statuses begun or ended, speeds and stop
 * distances, the move count, the low-hp limp variants below 30% (Blangonga
 * kinds), 20% (Congalala kinds) or 15% (Rajang) of hpMax, the carried items
 * shown again, the part break of BLANG_REACT_PART_BREAK), then sets it. A
 * remote copy ignores the owner's modes.
 */
void EmBlangonga_ChangeState(Em *em, u16 group, u16 state, u8 mode)
{
    EmBlangongaWork *w = (EmBlangongaWork *)&em->work;
    u8 count = 0;
    f32 lowHpRatio;
    u16 area;

    if (w->senseListed != 0) {
        ObjListA70_Remove((EmObj *)((&w->sense)));
        w->senseListed = 0;
    }
    if (em->remote == 0) {
        Em_TargetPlayerInRange(em);
    } else if (mode == EM_SYNC_OWNER || mode == EM_SYNC_OWNER_FLAGGED) {
        return;
    }
    if (EmBlangonga_IsBlangonga(em) == 1) {
        lowHpRatio = 0.3f;
    } else if (EmBlangonga_IsCongalala(em) == 1) {
        lowHpRatio = 0.2f;
    } else {
        lowHpRatio = 0.15f;
    }
    em->hitzoneSet = 0;
    switch (group) {
    case EM_GROUP_IDLE:
        switch (state) {
        case BLANG_IDLE:
            if (mode == EM_SYNC_LOCAL) {
                count = w->moveCount;
            }
            if (em->combat != 0) {
                state = BLANG_IDLE_ALERT;
            }
            break;
        case BLANG_IDLE_REST:
            Em_ResetNotice(em);
            break;
        case BLANG_IDLE_STATUS2_END:
            Em_EndStatus2(em);
            break;
        case BLANG_IDLE_STATUS2_COLLAPSE:
        case BLANG_IDLE_STATUS2_DOWNED:
            Em_BeginStatus2(em);
            break;
        case BLANG_IDLE_STATUS4_SHAKE_OFF:
            Em_EndStatus4(em);
            break;
        case BLANG_IDLE_BURIED:
            em->senseDelay = 5;
            em->stateArmor = 1;
            em->scriptPending = 0;
            break;
        case BLANG_IDLE_STUN_SHAKE_OFF:
            Em_EndStatus5(em);
            break;
        case BLANG_IDLE_REST_END:
            Em_RefillNeed3(em);
            break;
        case BLANG_IDLE_HOLD2_SHAKE_OFF:
            em->holdTimer2 = 0;
            break;
        case BLANG_IDLE_EAT_KIND_A:
        case BLANG_IDLE_EAT_KIND_B:
        case BLANG_IDLE_EAT_KIND_C:
        case BLANG_IDLE_EAT_KIND_D:
        case BLANG_IDLE_EAT_NEED1_B:
            switch (state) {
            case BLANG_IDLE_EAT_KIND_A:
                w->eatenKind = 1;
                break;
            case BLANG_IDLE_EAT_KIND_B:
                w->eatenKind = 2;
                break;
            case BLANG_IDLE_EAT_KIND_C:
                w->eatenKind = 3;
                break;
            case BLANG_IDLE_EAT_KIND_D:
                w->eatenKind = 4;
                break;
            }
            if (em->type == EM_KIND_CONGALALA) {
                EmBlangonga_HideCarried(em, 0);
                EmBlangonga_HideCarried(em, 1);
                EmBlangonga_HideCarried(em, 2);
            }
            break;
        case BLANG_IDLE_EAT:
            Em_StartGoalAction(em, 10000);
            break;
        case BLANG_IDLE_STATUS1_DOWNED:
        case BLANG_IDLE_STATUS1_COLLAPSE:
            Em_BeginStatus1(em);
            break;
        case BLANG_IDLE_STATUS1_END:
            Em_EndStatus1(em);
            break;
        case BLANG_IDLE_ROAR_RAGE:
            Em_StartRage(em);
            break;
        case BLANG_IDLE_ROAR_PENDING_RAGE:
            Em_StartPendingRage(em);
            break;
        }
        break;
    case EM_GROUP_MOVE:
        switch (state) {
        case BLANG_MOVE_WALK:
        case BLANG_MOVE_START_AND_WALK:
            if (em->hp < (s16) (em->hpMax * lowHpRatio)) {
                state = BLANG_MOVE_LIMP;
            }
        case BLANG_MOVE_LIMP:
        walk: /* BLANG_MOVE_ACTION_E at low hp joins the limp here */
            if (em->curGoalKind == 7) {
                Em_SetSpeedScaled(em, 0, 1000.0f, 350.0f);
            } else {
                Em_SetSpeedScaled(em, 0, 1000.0f, 100.0f);
            }
            break;
        case BLANG_MOVE_ACTION_A:
        case BLANG_MOVE_ACTION_B:
        case BLANG_MOVE_ACTION_C:
        case BLANG_MOVE_ACTION_D:
            if (Em_IsSyncFree(em, mode) != 0) {
                w->moveCount++;
            }
            count = w->moveCount;
            break;
        case BLANG_MOVE_TURN_TO_EXIT:
            if (em->remote == 0 && em->curGoalKind != 3) {
                EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE, EM_SYNC_SEND);
                return;
            }
            break;
        case BLANG_MOVE_START_AND_APPROACH:
        case BLANG_MOVE_APPROACH:
            if (em->hp < (s16) (em->hpMax * lowHpRatio)) {
                state = BLANG_MOVE_LIMP_APPROACH;
            }
        case BLANG_MOVE_LIMP_APPROACH:
            if (em->curGoalKind == 0) {
                em->hasGoal = 0;
            } else {
                em->hasGoal = 1;
            }
            break;
        case BLANG_MOVE_ACTION_E:
            if (em->hp < (s16) (em->hpMax * lowHpRatio)) {
                state = BLANG_MOVE_LIMP;
                goto walk;
            }
            break;
        case BLANG_MOVE_TURN:
        case BLANG_MOVE_TURN_QUICK:
        case BLANG_MOVE_TURN_AWAY:
        case BLANG_MOVE_TURN_AWAY_QUICK:
            break;
        }
        break;
    case EM_GROUP_FLY:
        switch (state) {
        case 2:
            if ((EmBlangonga_ScriptQuery(em, BLANG_QUERY_ON_SNOW) & 0xFF) == 0) {
                EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_ALERT_BRIEF, EM_SYNC_SEND);
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
                switch (Map_DayId(gScene.map)) {
                case MAP_SNOWY_MOUNTAINS:
                default:
                    area = Area_DayId(AREA_SNOWY_MOUNTAINS_8);
                    em->destArea = area;
                    em->nextArea = area;
                    em->goalNo = area;
                    break;
                case MAP_JUNGLE:
                    area = Area_DayId(AREA_JUNGLE_3);
                    em->destArea = area;
                    em->nextArea = area;
                    em->goalNo = area;
                    break;
                case MAP_SWAMP:
                    area = Area_DayId(AREA_SWAMP_9);
                    em->destArea = area;
                    em->nextArea = area;
                    em->goalNo = area;
                    break;
                case MAP_VOLCANO:
                    area = Area_DayId(AREA_VOLCANO_5);
                    em->destArea = area;
                    em->nextArea = area;
                    em->goalNo = area;
                    break;
                }
                em->goalKind = 3;
                Em_ComputeGoal(em, em->goal);
                EmBlangonga_ChangeState(em, EM_GROUP_MOVE, BLANG_MOVE_TURN_TO_EXIT, EM_SYNC_SEND);
                return;
            }
            break;
        }
        break;
    case EM_GROUP_ATTACK:
        switch (state) {
        case BLANG_ATK_CHARGE:
            Em_SetSpeedScaled(em, 0, 1000.0f, 1100.0f);
            break;
        case BLANG_ATK_RAJANG_CHARGE:
            Em_SetSpeedScaled(em, 0, 1000.0f, 500.0f);
            break;
        case BLANG_ATK_CONGALALA_CHARGE:
            if (em->curGoalKind != 1) {
                Em_SetSpeedScaled(em, 0, 1000.0f, 350.0f);
            } else {
                Em_SetSpeedScaled(em, 0, 1000.0f, -200.0f);
            }
            break;
        }
        break;
    case EM_GROUP_REACT:
        switch (state) {
        case BLANG_REACT_STATUS4:
        case BLANG_REACT_STATUS4_DOWNED:
            Em_BeginStatus4(em);
            break;
        case BLANG_REACT_TRAPPED:
            Em_EndAllStatus(em);
            break;
        case BLANG_REACT_STATUS4_DOWNED_END:
            Em_EndStatus4(em);
            break;
        case BLANG_REACT_STATUS2_DOWNED_END:
            Em_EndStatus2(em);
            break;
        case BLANG_REACT_STATUS1_DOWNED_END:
            Em_EndStatus1(em);
            break;
        case BLANG_REACT_STUN:
        case BLANG_REACT_STUN_B:
        case BLANG_REACT_STUN_DOWNED:
            Em_BeginStatus5(em);
            break;
        case BLANG_REACT_STUN_DOWNED_END:
            Em_EndStatus5(em);
            break;
        case BLANG_REACT_HOLD2:
            Em_EndAllStatus(em);
            break;
        case BLANG_REACT_DROP_CARRIED:
        case BLANG_REACT_DROP_CARRIED_A:
        case BLANG_REACT_DROP_CARRIED_B:
        case BLANG_REACT_DROP_CARRIED_C:
            if (em->type == EM_KIND_CONGALALA) {
                EmBlangonga_HideCarried(em, 0);
                EmBlangonga_HideCarried(em, 1);
                EmBlangonga_HideCarried(em, 2);
            }
            break;
        case BLANG_REACT_PART_BREAK:
            em->partBreakCount++;
            if (em->type == EM_KIND_BLANGONGA) {
                Em_BreakPart(em, 1);
            } else if (em->type == EM_KIND_RAJANG) {
                Em_EndRage(em);
                Em_BreakPart(em, 2);
            }
            break;
        case BLANG_REACT_TRAP_ESCAPE:
            Em_EndAllStatus(em);
            em->guardTimer = 2;
            break;
        case BLANG_REACT_TRAPPED_STRUGGLE:
        case BLANG_REACT_FALL:
        case BLANG_REACT_NONE:
        case BLANG_REACT_FLINCH_ATTACK:
            break;
        }
        break;
    case EM_GROUP_DEFEAT:
        switch (state) {
        case BLANG_DEFEAT_DIE:
            if (EmBlangonga_IsSmallKind(em) == 1 && em->stance == EM_STANCE_AIR) {
                EmBlangonga_ChangeState(em, EM_GROUP_DEFEAT, BLANG_DEFEAT_DIE_FALLING, mode);
                return;
            }
            break;
        case BLANG_DEFEAT_REAPPEAR:
            if (QuestEm_CanAppear(em) == 1) {
                QuestEm_Appear(em);
            }
            break;
        }
        break;
    case EM_GROUP_VERSUS:
        if (state == 0x98) {
            Versus_MarkDefeat(em);
        }
        em->motionSpeed = 1.0f;
        break;
    case EM_GROUP_EVENT:
    case EM_GROUP_EVENT2:
        break;
    }
    w->moveCount = count;
    Em_SetStateSync(em, group, state, mode);
}

/*
 * Ends a state: back to BLANG_IDLE with the motion speed, stance and look
 * reset, the script asked for the next state (scriptPending). Both
 * arguments are unused.
 */
static void EmBlangonga_EndState(Em *em, s32 unused1, s32 unused2)
{
    em->motionSpeed = 1.0f;
    em->stance = EM_STANCE_GROUND;
    em->lookAtTarget = 0;
    em->scriptPending = 1;
    EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE, EM_SYNC_LOCAL);
}

/*
 * Ends a buried state: as EmBlangonga_EndState, but stays under the snow
 * (stateArmor) and goes back to BLANG_LEAP_BURIED.
 */
static void EmBlangonga_EndBuriedState(Em *em)
{
    em->motionSpeed = 1.0f;
    em->stance = EM_STANCE_GROUND;
    em->stateArmor = 1;
    em->noPushTimer = 2;
    em->lookAtTarget = 0;
    em->scriptPending = 1;
    EmBlangonga_ChangeState(em, EM_GROUP_FLY, BLANG_LEAP_BURIED, EM_SYNC_LOCAL);
}

/*
 * Picks the carried item for the current map (MAP_JUNGLE or MAP_SWAMP: the two
 * weight tables of EmBlangonga_TurnData) from `seed`: the value of the row
 * whose running weight passes seed modulo the total; 0xFFFF on other maps.
 * The first argument is unused (its register is the loop pointer: a local
 * of its own gives other registers).
 */
u16 EmBlangonga_PickCarried(EmBlangongaPickRow *row, u16 seed)
{
    u16 sum;
    u16 acc;
    EmBlangongaPickRow *table;
    u16 roll;

    switch (Map_DayId(gScene.map)) {
    case MAP_JUNGLE:
        table = EmBlangonga_TurnData.pick0;
        break;
    case MAP_SWAMP:
        table = EmBlangonga_TurnData.pick1;
        break;
    default:
        return 0xFFFF;
    }
    sum = 0;
    row = table;
    if (table->weight != 0xFFFF) {
        do {
            acc = row->weight;
            sum += acc;
            row++;
        } while (row->weight != 0xFFFF);
    }
    acc = 0;
    roll = seed % sum;
    if (table->weight != 0xFFFF) {
        do {
            acc += *(u16 *) table; /* the weight; read as a u16 the match needs (table->weight compiles differently) */
            if (acc > roll) {
                return table->value;
            }
            table++;
        } while (table->weight != 0xFFFF);
    }
    return 0xFFFF;
}

/* Hides carried item `part` (0-2): sets its carryHidden flag. */
void EmBlangonga_HideCarried(Em *em, u16 part)
{
    EmBlangongaWork *w = (EmBlangongaWork *)&em->work;

    switch (part) {
    case 0:
        w->carryHidden[0] = 1;
        break;
    case 1:
        w->carryHidden[1] = 1;
        break;
    case 2:
        w->carryHidden[2] = 1;
        break;
    }
}

/* Shows carried item `part` (0-2): clears its carryHidden flag. */
void EmBlangonga_ShowCarried(Em *em, u16 part)
{
    EmBlangongaWork *w = (EmBlangongaWork *)&em->work;

    switch (part) {
    case 0:
        w->carryHidden[0] = 0;
        break;
    case 1:
        w->carryHidden[1] = 0;
        break;
    case 2:
        w->carryHidden[2] = 0;
        break;
    }
}

/*
 * Returns 1 when carried item `part` (0-2) is shown; for `part` 6, 1 while
 * part 2 is unbroken (Em_CheckRageByHp and gm_em.c ask that).
 */
s32 EmBlangonga_CarriedShown(Em *em, u16 part)
{
    EmBlangongaWork *w = (EmBlangongaWork *)&em->work;

    switch (part) {
    case 0:
        if (w->carryHidden[0]) {
            return 0;
        }
        break;
    case 1:
        if (w->carryHidden[1]) {
            return 0;
        }
        break;
    case 2:
        if (w->carryHidden[2]) {
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

/* Returns 1 for the small kinds, the Blango and the Conga. */
s32 EmBlangonga_IsSmallKind(Em *em)
{
    u8 type;

    type = em->type;
    if ((type == 0x3D) || (type == 0x3E)) {
        return 1;
    }
    return 0;
}

/* Returns 1 for Blangonga and the Blango. */
static s32 EmBlangonga_IsBlangonga(Em *em)
{
    u8 type;

    type = em->type;
    if ((type == 0x33) || (type == 0x3D)) {
        return 1;
    }
    return 0;
}

/* Returns 1 for Congalala and the Conga. */
static s32 EmBlangonga_IsCongalala(Em *em)
{
    u8 type;

    type = em->type;
    if ((type == 0x34) || (type == 0x3E)) {
        return 1;
    }
    return 0;
}

/* Returns 1 for Rajang. */
static u64 EmBlangonga_IsRajang(Em *em)
{
    return em->type == EM_KIND_RAJANG;
}

/*
 * Returns 1 in the states a flash doesn't affect (Em_ApplyFlash): while
 * stateArmor, lying in a hold under status 1 or 2, and the trapped,
 * status-4 and stun states of EM_GROUP_REACT.
 */
s32 EmBlangonga_IgnoresFlash(Em *em)
{
    if (em->stateArmor == 1
        || (em->stateGroup == EM_GROUP_IDLE && em->state == BLANG_IDLE_STATUS2_DOWNED)
        || (em->stateGroup == EM_GROUP_IDLE && em->state == BLANG_IDLE_STATUS1_DOWNED)
        || (em->stateGroup == EM_GROUP_REACT && em->state == BLANG_REACT_TRAPPED)
        || (em->stateGroup == EM_GROUP_REACT && em->state == BLANG_REACT_TRAPPED_STRUGGLE)
        || (em->stateGroup == EM_GROUP_REACT && em->state == BLANG_REACT_STATUS4_DOWNED)
        || (em->stateGroup == EM_GROUP_REACT && em->state == BLANG_REACT_STATUS4_DOWNED_END)
        || (em->stateGroup == EM_GROUP_REACT && em->state == BLANG_REACT_STATUS2_DOWNED_END)
        || (em->stateGroup == EM_GROUP_REACT && em->state == BLANG_REACT_STUN_DOWNED)
        || (em->stateGroup == EM_GROUP_REACT && em->state == BLANG_REACT_STUN_DOWNED_END)) {
        return 1;
    }
    return 0;
}

/* Returns 1 while Em.stateArmor is set (buried, or out of reach in a scene). */
s32 EmBlangonga_InStateArmor(Em *em)
{
    return em->stateArmor != 0;
}

/*
 * The shadow's brightness (sub_main's Effect01_Draw): fades from 1 at 2000
 * above the floor to 0 at 3500 during BLANG_LEAP_OUT_OF_AREA and _DROP_IN,
 * 0 during BLANG_LEAP_CROSS_AREA, else 1.
 */
f32 EmBlangonga_ShadowFade(Em *em)
{
    f32 height;
    f32 fade;

    if (em->stateGroup == EM_GROUP_FLY) {
        switch (em->state) {
        case BLANG_LEAP_OUT_OF_AREA:
        case BLANG_LEAP_DROP_IN:
            height = em->posY - em->floorY;
            if (height < 2000.0f) {
                return 1.0f;
            }
            if (height < 3500.0f) {
                fade = 1.0f - ((height - 2000.0f) / 1500.0f);
            } else {
                fade = 0.0f;
            }
            return fade;
        case BLANG_LEAP_CROSS_AREA:
            return 0.0f;
        }
    }
    return 1.0f;
}

/*
 * Counts Em.rage's timer (EmBlangongaWork.rageTimer) up to its limit while
 * enraged and down to 0 otherwise (the limit: 15 frames, Congalala 60
 * enraged or 150); Rajang's rage begins with stage object 0x18.
 */
void EmBlangonga_UpdateRageTimer(Em *em)
{
    EmBlangongaWork *w = (EmBlangongaWork *)&em->work;
    s16 lim;

    switch (em->type) {
    case EM_KIND_RAJANG:
    default:
        lim = 0xF;
        break;
    case EM_KIND_CONGALALA:
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

/*
 * For gm_em.c's Congalala meshes: 1 while Congalala holds its food (frames
 * 36-382 of BLANG_MOT_EAT, or in its story scene once it has picked it up),
 * 2 in the scene before that, else 0.
 */
s32 EmBlangonga_EatPropMode(Em *em)
{
    EmBlangongaWork *w = (EmBlangongaWork *)&em->work;

    if (em->stateGroup == EM_GROUP_EVENT && em->state == BLANG_EVENT_CONGALALA_SCENE) {
        return w->sceneEating ? 1 : 2;
    }
    switch (em->motionId[0]) {
    case EM_MOTION_ID(0, BLANG_MOT_EAT):
        if (EmMotion_InFrameRange(em, 0, 36.0f, 382.0f)) {
            return 1;
        }
        break;
    }
    return 0;
}

/*
 * Steps the body colour (EmBlangongaWork.tint) toward the kind's target:
 * the Blangonga kinds' rage colour, or Blangonga's own colour above 30% hp
 * and a second one below; Congalala's pulsing rage colour or its calm one
 * (at once when the rage timer is full). 12 per frame (Congalala calming:
 * 6). Rajang and the Conga keep theirs.
 */
void EmBlangonga_UpdateTint(Em *em)
{
    EmBlangongaWork *w = (EmBlangongaWork *)&em->work;
    u8 col[3];
    s16 i;
    s16 diff;
    u8 *target;
    u8 *channel;
    u8 step;
    f32 pulse;
    u8 *target2;
    u8 *channel2;
    s16 j;
    s16 diff2;

    switch (em->type) {
    case EM_KIND_BLANGONGA:
    case EM_KIND_BLANGO:
        if (em->rage != 0) {
            col[0] = 0xFF;
            col[1] = 0x64;
            col[2] = 0x5A;
        } else if (em->type == EM_KIND_BLANGONGA) {
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
        for (i = 0, target = col; i < 3; i++, target++) {
            channel = &w->tint[i];
            diff = *target - *channel;
            if (Math_Absf(diff) <= 12.0f) {
                *channel = *target;
            } else if (diff < 0) {
                *channel -= 12;
            } else {
                *channel += 12;
            }
        }
        break;
    case EM_KIND_CONGALALA:
        if (em->rage != 0) {
            pulse = 0.5f * (1.0f + Math_Sin((6.2831855f * (f32) (gScene.frame << 10)) / 65536.0f));
            col[0] = 255.0f + -100.0f * pulse;
            col[1] = 40.0f + 10.0f * pulse;
            col[2] = 10.0f + 5.0f * pulse;
        } else {
            col[0] = 0x99;
            col[1] = 0x80;
            col[2] = 0x73;
        }
        for (j = 0, target2 = col; j < 3; j++, target2++) {
            if (EmBlangonga_RageTimerFull(em) == 1) {
                w->tint[j] = *target2;
            } else {
                channel2 = &w->tint[j];
                diff2 = *target2 - *channel2;
                if (em->rage != 0) {
                    step = 12;
                } else {
                    step = 6;
                }
                if (Math_Absf(diff2) <= step) {
                    *channel2 = *target2;
                } else if (diff2 < 0) {
                    *channel2 -= step;
                } else {
                    *channel2 += step;
                }
            }
        }
        break;
    default:
        return;
    }
}

/*
 * Returns 1 when the rage timer has reached its limit
 * (EmBlangonga_UpdateRageTimer's); also read by sub_main for Rajang's mesh.
 */
s32 EmBlangonga_RageTimerFull(Em *em)
{
    EmBlangongaWork *w = (EmBlangongaWork *)&em->work;
    s16 k;

    switch (em->type) {
    case EM_KIND_RAJANG:
    default:
        k = 0xF;
        break;
    case EM_KIND_CONGALALA:
        if (em->rage != 0) {
            k = 0x3C;
        } else {
            k = 0x96;
        }
        break;
    }
    return w->rageTimer == k;
}

/*
 * Creates the periodic effect 06 of a lying monster at scale 1.6: at bone 1
 * for `kind` 1 (Congalala), at bone 0x12 for the others.
 */
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
    Em_SpawnLyingPuffs(em, bone, pos, 1.6f);
}

/*
 * Returns 1 when a monster under stateArmor in the player's area is off the
 * snow (ScriptQuery 1 is 0): EmBlangonga_Update then ends its state.
 */
s32 EmBlangonga_ShouldEndState(Em *em)
{
    if (!((s32)Player_InCurrentArea(em) & 0xFF)) {
        return 0;
    }
    if ((em->stateArmor == 1) && !(EmBlangonga_ScriptQuery(em, BLANG_QUERY_ON_SNOW) & 0xFF)) {
        return 1;
    }
    return 0;
}

/*
 * Starts the family's movement curves (EmBlangonga_VelYCurves and
 * SpeedCurves) for `kind`: 0 the vertical speed from frame 0, 1 from frame
 * 24, 2 both speeds; the velocity is cleared.
 */
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

/*
 * Sets Em.velY from the curve keys around the curve frame (curve
 * curveKind); at the table's end applies gravity or stops, and returns 2.
 */
static u8 EmBlangonga_CurveVelY(Em *em, EmWork *work)
{
    EmBlangongaWork *w = (EmBlangongaWork *) work;
    EmCurveKey *key;
    f32 frame;
    f32 keyFrame;
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

extern EmCurveKey *EmBlangonga_SpeedCurves[];

/*
 * Sets Em.velZ from the speed curve keys around the curve frame; returns 4
 * at the table's end.
 */
static u8 EmBlangonga_CurveSpeed(Em *em, EmWork *work)
{
    EmBlangongaWork *w = (EmBlangongaWork *) work;
    EmCurveKey *key;
    f32 frame;
    f32 keyFrame;
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
 * Runs the started curves, advances the curve frame by the motion's step
 * and, in the air, moves along rotY (Em_MoveRotatedAccel). Returns the
 * ended curves' bits (2, 4).
 */
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
    if (em->stance == EM_STANCE_AIR) {
        w->accelAngX = 0;
        w->accelAngY = em->rotY;
        w->accelAngZ = 0;
        Em_MoveRotatedAccel(em, &w->accelAngX);
    }
    return ret;
}

/* The family's entry points per area (an entry of gm_sub gm_em.c's D_00921A20). */
extern EmBlangongaAreaEntry *EmBlangonga_AreaEntries;

/*
 * Moves the monster into Em.nextArea: from the area's entry list (gm_sub's
 * per-area table) it picks the entry point and the point to face by the
 * area it came from, or uses the start position; it starts 5000 above the
 * floor.
 */
void EmBlangonga_EnterNewArea(Em *em)
{
    f32 *pts;
    s16 entryNo;
    s16 faceNo;

    em->prevArea = em->area;
    em->area = em->nextArea;
    pts = ((EmBlangongaAreaEntry *)Em_FindAreaEntry(em, (struct EmIdEntry *)EmBlangonga_AreaEntries, em->area))->pts;
    if (pts == NULL) {
        Vec_Copy(&em->posX, &gStartPos[em->area * 3]);
        em->rotY = 0;
    } else {
        switch (Area_DayId(em->area)) {
        case AREA_SNOWY_MOUNTAINS_3:
        default:
        case AREA_SNOWY_MOUNTAINS_4:
        case AREA_SNOWY_MOUNTAINS_5:
        case AREA_SNOWY_MOUNTAINS_6:
        case AREA_SNOWY_MOUNTAINS_7:
        case AREA_SNOWY_MOUNTAINS_8:
        case AREA_JUNGLE_9:
        case AREA_VOLCANO_1:
        case AREA_VOLCANO_4:
        case AREA_VOLCANO_5:
        case AREA_VOLCANO_7:
            entryNo = 0;
            faceNo = 1;
            break;
        case AREA_JUNGLE_1:
            switch (Area_DayId(em->prevArea)) {
            case AREA_JUNGLE_2:
                entryNo = 5;
                faceNo = 4;
                break;
            default:
                entryNo = 0;
                faceNo = 1;
                break;
            }
            break;
        case AREA_JUNGLE_2:
            switch (Area_DayId(em->prevArea)) {
            case AREA_JUNGLE_3:
                entryNo = 4;
                faceNo = 3;
                break;
            default:
                entryNo = 0;
                faceNo = 1;
                break;
            }
            break;
        case AREA_JUNGLE_3:
            switch (Area_DayId(em->prevArea)) {
            case AREA_JUNGLE_2:
            case AREA_JUNGLE_6:
                entryNo = 4;
                faceNo = 3;
                break;
            default:
                entryNo = 0;
                faceNo = 1;
                break;
            }
            break;
        case AREA_JUNGLE_6:
            switch (Area_DayId(em->prevArea)) {
            case AREA_JUNGLE_1:
            case AREA_JUNGLE_9:
                entryNo = 8;
                faceNo = 7;
                break;
            default:
                entryNo = 0;
                faceNo = 1;
                break;
            }
            break;
        case AREA_JUNGLE_10:
            entryNo = 4;
            faceNo = 3;
            break;
        case AREA_SWAMP_2:
            entryNo = 0;
            faceNo = 1;
            break;
        case AREA_SWAMP_5:
            switch (Area_DayId(em->prevArea)) {
            case AREA_SWAMP_6:
                entryNo = 5;
                faceNo = 4;
                break;
            default:
                entryNo = 0;
                faceNo = 1;
                break;
            }
            break;
        case AREA_SWAMP_6:
            switch (Area_DayId(em->prevArea)) {
            case AREA_SWAMP_7:
                entryNo = 6;
                faceNo = 5;
                break;
            default:
                entryNo = 0;
                faceNo = 1;
                break;
            }
            break;
        case AREA_SWAMP_7:
            switch (Area_DayId(em->prevArea)) {
            case AREA_SWAMP_9:
                entryNo = 2;
                faceNo = 1;
                break;
            default:
                entryNo = 0;
                faceNo = 1;
                break;
            }
            break;
        case AREA_SWAMP_8:
            entryNo = 0;
            faceNo = 1;
            break;
        case AREA_SWAMP_9:
            switch (Area_DayId(em->prevArea)) {
            case AREA_SWAMP_2:
                entryNo = 4;
                faceNo = 3;
                break;
            case AREA_SWAMP_8:
                entryNo = 6;
                faceNo = 5;
                break;
            default:
                entryNo = 0;
                faceNo = 1;
                break;
            }
            break;
        case AREA_VOLCANO_6:
            switch (Area_DayId(em->prevArea)) {
            case AREA_VOLCANO_5:
                entryNo = 4;
                faceNo = 3;
                break;
            case AREA_VOLCANO_7:
            default:
                entryNo = 0;
                faceNo = 1;
                break;
            }
            break;
        }
        Vec_Copy(&em->posX, &pts[entryNo * 3]);
        em->rotY = Math_AngleXZ(&em->posX, &pts[faceNo * 3]);
    }
    em->posY += 5000.0f;
    Vec_Copy(&em->prevPosX, &em->posX);
}

/* BLANG_IDLE_NONE: does nothing. */
void EmBlangonga_IdleNone(void)
{
}

/*
 * BLANG_IDLE: plays BLANG_MOT_IDLE once, then asks the script for the next
 * state.
 */
void EmBlangonga_Idle(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_IDLE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->scriptPending = 1;
        }
        break;
    }
}

/*
 * BLANG_IDLE_REST: lies down and lies for 5400 frames, blind and without
 * flinching, regaining hp (1 every 6 frames up to 40% of hpMax; the
 * Congalala kinds every 8 frames and Rajang every 6, up to 50%) with the
 * periodic effect; then BLANG_IDLE_REST_END.
 */
void EmBlangonga_Rest(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_LIE_DOWN, 0, 0);
        em->timer = 0x1518;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_LIE, 0, 0);
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
            EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_REST_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_REST_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * BLANG_IDLE_STATUS2_END, _STATUS1_END and _REST_END: gets up (blind
 * meanwhile) and ends the state.
 */
void EmBlangonga_GetUp(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_GET_UP, 0, 0);
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

/*
 * BLANG_IDLE_STATUS2_COLLAPSE (variant 0) and _STATUS1_COLLAPSE (1): falls
 * over and lies, blind and without flinching, until the status timer runs
 * out, then the status's _END state.
 */
void EmBlangonga_StatusCollapse(Em *em, EmWork *work, s32 variant)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_COLLAPSE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_LIE, 0, 0);
        }
        break;
    case 2:
        EmBlangonga_SpawnPeriodicEffect06(em, 0);
        if (--em->timer <= 0) {
            em->step++;
            if (!(variant & 0xFF)) {
                EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_STATUS2_END, EM_SYNC_OWNER_FLAGGED);
                break;
            }
            EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_STATUS1_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 3:
        if (em->remote == 0) {
            if (!(variant & 0xFF)) {
                EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_STATUS2_END, EM_SYNC_OWNER_FLAGGED);
                break;
            }
            EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_STATUS1_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/* BLANG_IDLE_ALERT: plays BLANG_MOT_ALERT_IDLE once and ends the state. */
void EmBlangonga_AlertIdle(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_ALERT_IDLE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * The roars: BLANG_IDLE_ROAR, _ROAR_SIGNAL, _ROAR_RAGE and
 * _ROAR_PENDING_RAGE (variant 0), _ROAR_CALL (1: at frame 120 adds the
 * sensed object of kind 3 other monsters react to, until the first part
 * break), _ROAR_RAISE_RAGE (2: at frame 50 raises the rage by hp and cuts a
 * flash's blindness short).
 */
void EmBlangonga_Roar(Em *em, EmWork *work, s32 variant)
{
    s32 k;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, BLANG_MOT_ROAR, 0, 0);
        break;
    case 1:
        k = variant & 0xFF;
        switch (k) {
        case 0:
            break;
        case 1:
            if (((s8) em->partBreakCount <= 0) && (EmMotion_PassedFrameScaled(em, 120.0f, 0U) != 0)) {
                Vec_Set((f32 *) &work->moveAngBX, em->posX, em->posY, em->posZ);
                work->moveAngCX = 0x453B8000;
                ((EmBlangongaWork *)work)->sense.area = (s16) *(u8 *)&em->area;
                ((EmBlangongaWork *)work)->sense.kind = 3;
                ObjListA70_Add((EmObj *)&work->moveAngBX);
                ((EmBlangongaWork *)work)->senseListed = 1;
            }
            break;
        case 2:
            if (EmMotion_PassedFrameScaled(em, 50.0f, 0U) != 0) {
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

/*
 * BLANG_IDLE_TURN_ABOUT (variant 0) and _B (1, the other way): turns half
 * round over frames 102-202 and back over 402-452, then ends the state.
 */
void EmBlangonga_TurnAbout(Em *em, EmWork *work, s32 variant)
{
    u16 turn = 0;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        switch (variant & 0xFF) {
        case 0:
        default:
            Em_SetMotion(em, BLANG_MOT_TURN_ABOUT, 0, 0);
            break;
        case 1:
            Em_SetMotion(em, BLANG_MOT_TURN_ABOUT_B, 0, 0);
            break;
        }
        break;
    case 1:
        if (EmMotion_InFrameRange(em, 0, 102.0f, 202.0f)) {
            turn = -327.68f * em->motionSpeed;
        } else if (EmMotion_InFrameRange(em, 0, 402.0f, 452.0f)) {
            turn = 655.36f * em->motionSpeed;
        }
        switch (variant & 0xFF) {
        case 1:
            turn = -turn;
            break;
        }
        em->rotY += turn;
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_IDLE_STATUS4_SHAKE_OFF, _STUN_SHAKE_OFF and _HOLD2_SHAKE_OFF: plays
 * BLANG_MOT_SHAKE_OFF once and ends the state.
 */
void EmBlangonga_ShakeOff(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, BLANG_MOT_SHAKE_OFF, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_IDLE_EAT_NEED2: eats without flinching for 300 frames, filling need
 * 2, then regains 5% of hpMax and clears need 2's state.
 */
void EmBlangonga_EatNeed2(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_NO_FLINCH);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, BLANG_MOT_EAT_NEED2, 0, 0);
        em->timer = 0x12C;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_EAT_NEED2_END, 0, 0);
            Em_AddHp(em, (s32) (0.05f * (f32) em->hpMax));
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_ClearNeed2State(em);
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
    Em_AddNeed2(em, 0xDE);
}

/*
 * BLANG_IDLE_EAT_NEED1, _EAT_NEED1_B (variant 0) and _EAT_KIND_A..D (1):
 * eats without flinching, turning in frames 4-26 and back in 370-418, then
 * regains 5% (variant 1: 1%) of hpMax, refills need 1 and clears its state.
 */
void EmBlangonga_EatNeed1(Em *em, EmWork *work, s32 variant)
{
    u16 turn = 0;

    Em_SetHitReactMode(em, EM_HIT_REACT_NO_FLINCH);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_EAT, 0, 0);
        em->timer = 0xF0;
        break;
    case 1:
        if (EmMotion_InFrameRange(em, 0, 4.0f, 26.0f)) {
            turn = 1638.4f * em->motionSpeed;
        } else if (EmMotion_InFrameRange(em, 0, 370.0f, 418.0f)) {
            turn = -712.34784f * em->motionSpeed;
        }
        em->rotY += turn;
        if (em->motion[0].playing == 0) {
            em->step++;
            if ((variant & 0xFF) == 0) {
                Em_AddHp(em, 0.05f * em->hpMax);
            } else {
                Em_AddHp(em, 0.01f * em->hpMax);
            }
            Em_AddNeed1(em, em->need1Max);
            Em_ClearNeed1State(em);
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_IDLE_CLEAR_PAINT: plays its motion and clears the paint mark
 * (Em.painted, paintTimer) when it ends.
 */
void EmBlangonga_ClearPaint(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_IDLE_ACTION_B_END, 0, 0);
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

/*
 * BLANG_IDLE_LEAVE_TRAP: climbs out of the trap (the Blangonga kinds and
 * Rajang with BLANG_MOT_EMERGE from frame 14, the Congalala kinds with
 * their own motion), then ends the state.
 */
void EmBlangonga_LeaveTrap(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        switch (em->type) {
        case EM_KIND_BLANGONGA:
        default:
        case EM_KIND_BLANGO:
            Em_SetMotion(em, BLANG_MOT_EMERGE, 6, 0xE);
            break;
        case EM_KIND_CONGALALA:
        case EM_KIND_CONGA:
            Em_SetMotion(em, BLANG_MOT_CONGALALA_CLIMB_OUT, 0, 0);
            break;
        }
        em->stance = EM_STANCE_GROUND;
        em->noReact = 1;
        break;
    case EM_KIND_RATHIAN:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_IDLE_EAT_START (variant 0, BLANG_MOT_EAT to frame 300) and
 * _EAT_START_B (1, Congalala's motion to frame 200): starts eating, then
 * BLANG_IDLE_EAT while the motion plays on.
 */
void EmBlangonga_EatStart(Em *em, EmWork *work, s32 variant)
{
    f32 frame;
    s32 k;

    k = variant & 0xFF;
    if (k == 0) {
        Em_SetHitReactMode(em, EM_HIT_REACT_NO_FLINCH);
    }
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        if (k == 0) {
            Em_SetMotion(em, BLANG_MOT_EAT, 0, 0);
            break;
        }
        Em_SetMotion(em, BLANG_MOT_CONGALALA_EAT, 0, 0);
        break;
    case 1:
        if (k == 0) {
            frame = 300.0f;
        } else {
            frame = 200.0f;
        }
        if (EmMotion_PassedFrameScaled(em, frame, 0U) != 0) {
            em->step++;
            EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_EAT, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_EAT, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/* BLANG_IDLE_ACTION_A: plays BLANG_MOT_BACK_STRIKE once and ends the state. */
void EmBlangonga_IdleActionA(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_BACK_STRIKE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_IDLE_ACTION_B: plays BLANG_MOT_IDLE_ACTION_B (a hit and stage
 * object 0x0F at frame 128), then its end, and ends the state.
 */
void EmBlangonga_IdleActionB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_IDLE_ACTION_B, 0, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 128.0f, 0U) != 0) {
            StageObj_K0F_CreateOnEm(em, 5, 1, 0.4f * em->scaleX);
            BlangongaAttack_Create(em, 0x22);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_IDLE_ACTION_B_END, 0, 0);
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

/*
 * BLANG_IDLE_ALERT_BRIEF: BLANG_MOT_ALERT_IDLE for 10 frames, then ends the
 * state.
 */
void EmBlangonga_AlertBrief(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, BLANG_MOT_ALERT_IDLE, 0, 0);
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

/*
 * BLANG_IDLE_STATUS2_DOWNED (variant 0) and _STATUS1_DOWNED (1): lies in
 * the hold (holdTimer kept at 5), blind and without flinching, with the
 * periodic effect and a sound every 135 frames, until the status timer runs
 * out; then BLANG_REACT_STATUS2_DOWNED_END or _STATUS1_DOWNED_END.
 * Blangonga first comes up out of the pit (BLANG_MOT_EMERGE to frame 14).
 */
void EmBlangonga_StatusDowned(Em *em, EmWork *work, s32 variant)
{
    u16 motion;

    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->holdTimer = 5;
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        if ((em->type == EM_KIND_BLANGONGA) && (motion = em->motionId[0], (motion != EM_MOTION_ID(0, BLANG_MOT_EMERGE))) && (motion != EM_MOTION_ID(0, BLANG_MOT_STRUGGLE)) && (motion != EM_MOTION_ID(0, BLANG_MOT_DOWNED))) {
            Em_SetMotion(em, BLANG_MOT_EMERGE, 0, 0);
            em->stateArmor = 1;
            em->noPushTimer = 5;
        } else {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_DOWNED, 0, 0);
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
            Em_SetMotion(em, BLANG_MOT_DOWNED, 0, 0);
        }
        break;
    case 2:
        if (((s32) em->timer % 135) == 0) {
            Em_PlaySeAtBone(em, 0x52, 0x13);
        }
        if (em->type == EM_KIND_CONGALALA) {
            EmBlangonga_SpawnPeriodicEffect06(em, 1);
        } else {
            EmBlangonga_SpawnPeriodicEffect06(em, 0);
        }
        if (--em->timer <= 0) {
            em->step++;
            if (!(variant & 0xFF)) {
                EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_STATUS2_DOWNED_END, EM_SYNC_OWNER);
                break;
            }
            EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_STATUS1_DOWNED_END, EM_SYNC_OWNER);
        }
        break;
    case 3:
        if (em->type == EM_KIND_CONGALALA) {
            EmBlangonga_SpawnPeriodicEffect06(em, 1);
        } else {
            EmBlangonga_SpawnPeriodicEffect06(em, 0);
        }
        if (em->remote == 0) {
            if (!(variant & 0xFF)) {
                EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_STATUS2_DOWNED_END, EM_SYNC_OWNER);
                break;
            }
            EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_STATUS1_DOWNED_END, EM_SYNC_OWNER);
        }
        break;
    }
}

/*
 * BLANG_IDLE_ROAR_B: roars (Blangonga with BLANG_MOT_ROAR_B, the others
 * BLANG_MOT_ROAR) and ends the state.
 */
void EmBlangonga_RoarB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        if (em->type == EM_KIND_BLANGONGA) {
            Em_SetMotion(em, BLANG_MOT_ROAR_B, 0, 0);
            break;
        }
        Em_SetMotion(em, BLANG_MOT_ROAR, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_IDLE_ACTION_C: plays BLANG_MOT_IDLE_ACTION_C once and ends the
 * state.
 */
void EmBlangonga_IdleActionC(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, BLANG_MOT_IDLE_ACTION_C, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_IDLE_BURIED: waits under the snow, unhittable, unpushable, sensing
 * nothing and under stateArmor, until the script moves it on.
 */
void EmBlangonga_WaitBuried(Em *em)
{
    em->senseDelay = 5;
    em->noPushTimer = 5;
    em->noHitTimer = 5;
    em->stateArmor = 1;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotionRow(em, BLANG_MOT_BURIED, 0, 0);
        break;
    case 1:
        break;
    }
}

/*
 * BLANG_IDLE_LEAVE_MINE_POINT: plays BLANG_MOT_IDLE_ACTION_B and at frame
 * 184 leaves a timed mine point where it stands, then ends the state.
 */
void EmBlangonga_LeaveMinePoint(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_IDLE_ACTION_B, 0, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 184.0f, 0U) != 0) {
            EmMine_CreateTimed0(em, &em->posX);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_IDLE_EAT: selects sight row 0 and reaction set 0, waits for the
 * eating motion to end, and ends the state.
 */
void EmBlangonga_Eat(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SelectSightRow(em, 0);
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

/*
 * Runs the current EM_GROUP_IDLE state, passing the shared functions their
 * variant.
 */
void EmBlangonga_RunIdleState(Em *em, EmWork *work)
{
    switch (em->state) {
    case BLANG_IDLE_NONE:
        EmBlangonga_IdleNone();
        break;
    case BLANG_IDLE:
        EmBlangonga_Idle(em);
        break;
    case BLANG_IDLE_REST:
        EmBlangonga_Rest(em);
        break;
    case BLANG_IDLE_STATUS2_END:
        EmBlangonga_GetUp(em);
        break;
    case BLANG_IDLE_STATUS2_COLLAPSE:
        EmBlangonga_StatusCollapse(em, work, 0);
        break;
    case BLANG_IDLE_ALERT:
        EmBlangonga_AlertIdle(em);
        break;
    case BLANG_IDLE_ROAR:
        EmBlangonga_Roar(em, work, 0);
        break;
    case BLANG_IDLE_TURN_ABOUT:
        EmBlangonga_TurnAbout(em, work, 0);
        break;
    case BLANG_IDLE_TURN_ABOUT_B:
        EmBlangonga_TurnAbout(em, work, 1);
        break;
    case BLANG_IDLE_STATUS4_SHAKE_OFF:
        EmBlangonga_ShakeOff(em);
        break;
    case BLANG_IDLE_EAT_NEED2:
        EmBlangonga_EatNeed2(em);
        break;
    case BLANG_IDLE_EAT_NEED1:
        EmBlangonga_EatNeed1(em, work, 0);
        break;
    case BLANG_IDLE_CLEAR_PAINT:
        EmBlangonga_ClearPaint(em);
        break;
    case BLANG_IDLE_LEAVE_TRAP:
        EmBlangonga_LeaveTrap(em);
        break;
    case BLANG_IDLE_EAT_START_B:
        EmBlangonga_EatStart(em, work, 1);
        break;
    case BLANG_IDLE_ACTION_A:
        EmBlangonga_IdleActionA(em);
        break;
    case BLANG_IDLE_ACTION_B:
        EmBlangonga_IdleActionB(em);
        break;
    case BLANG_IDLE_ALERT_BRIEF:
        EmBlangonga_AlertBrief(em);
        break;
    case BLANG_IDLE_STATUS2_DOWNED:
        EmBlangonga_StatusDowned(em, work, 0);
        break;
    case BLANG_IDLE_ROAR_CALL:
        EmBlangonga_Roar(em, work, 1);
        break;
    case BLANG_IDLE_ROAR_RAISE_RAGE:
        EmBlangonga_Roar(em, work, 2);
        break;
    case BLANG_IDLE_ROAR_B:
        EmBlangonga_RoarB(em);
        break;
    case BLANG_IDLE_ACTION_C:
        EmBlangonga_IdleActionC(em);
        break;
    case BLANG_IDLE_BURIED:
        EmBlangonga_WaitBuried(em);
        break;
    case BLANG_IDLE_STUN_SHAKE_OFF:
        EmBlangonga_ShakeOff(em);
        break;
    case BLANG_IDLE_REST_END:
        EmBlangonga_GetUp(em);
        break;
    case BLANG_IDLE_HOLD2_SHAKE_OFF:
        EmBlangonga_ShakeOff(em);
        break;
    case BLANG_IDLE_EAT_KIND_A:
        EmBlangonga_EatNeed1(em, work, 1);
        break;
    case BLANG_IDLE_EAT_KIND_B:
        EmBlangonga_EatNeed1(em, work, 1);
        break;
    case BLANG_IDLE_EAT_KIND_C:
        EmBlangonga_EatNeed1(em, work, 1);
        break;
    case BLANG_IDLE_EAT_KIND_D:
        EmBlangonga_EatNeed1(em, work, 1);
        break;
    case BLANG_IDLE_LEAVE_MINE_POINT:
        EmBlangonga_LeaveMinePoint(em);
        break;
    case BLANG_IDLE_EAT_NEED1_B:
        EmBlangonga_EatNeed1(em, work, 0);
        break;
    case BLANG_IDLE_EAT:
        EmBlangonga_Eat(em);
        break;
    case BLANG_IDLE_ROAR_SIGNAL:
        EmBlangonga_Roar(em, work, 0);
        break;
    case BLANG_IDLE_STATUS1_DOWNED:
        EmBlangonga_StatusDowned(em, work, 1);
        break;
    case BLANG_IDLE_STATUS1_COLLAPSE:
        EmBlangonga_StatusCollapse(em, work, 1);
        break;
    case BLANG_IDLE_STATUS1_END:
        EmBlangonga_GetUp(em);
        break;
    case BLANG_IDLE_EAT_START:
        EmBlangonga_EatStart(em, work, 0);
        break;
    case BLANG_IDLE_ROAR_RAGE:
        EmBlangonga_Roar(em, work, 0);
        break;
    case BLANG_IDLE_ROAR_PENDING_RAGE:
        EmBlangonga_Roar(em, work, 0);
        break;
    }
}

/*
 * BLANG_MOVE_TURN (variant 0: to the target) and _TURN_AWAY (1): turns with
 * the slow turn parameters until the turn is done, then ends the state.
 */
void EmBlangonga_Turn(Em *em, s32 unused, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        switch (variant & 0xFF) {
        case 0:
        default:
            Em_StartTurnToTarget(em, &EmBlangonga_TurnData.turn[0]);
            break;
        case 1:
            Em_StartTurnAway(em, &EmBlangonga_TurnData.turn[0]);
            break;
        }
        break;
    case 1:
        if (Em_TurnStep(em, &EmBlangonga_TurnData.turn[0]) == 1) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_MOVE_WALK (variant 0; the Blango walks faster) and _LIMP (1): steps
 * to the goal (Em_StepToGoal), then ends the state.
 */
void EmBlangonga_WalkToGoal(Em *em, s32 unused, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        switch (variant & 0xFF) {
        case 0:
        default:
            if (em->type == EM_KIND_BLANGO) {
                em->speedBonus = 0x19;
            }
            Em_SetMotion(em, BLANG_MOT_WALK, 0, 0);
            break;
        case 1:
            Em_SetMotion(em, BLANG_MOT_LIMP, 0, 0);
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

/*
 * BLANG_MOVE_TURN_QUICK (variant 0), _TURN_TO_EXIT (1) and _TURN_AWAY_QUICK
 * (2): turns with the quick turn parameters and ends the state;
 * _TURN_TO_EXIT first takes the area's exit point as its goal and then
 * leaps out (BLANG_LEAP_OUT_OF_AREA).
 */
void EmBlangonga_TurnQuick(Em *em, s32 unused, s32 variant)
{
    s32 k;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        k = variant & 0xFF;
        em->lookAtTarget = 0;
        switch (k) {
        case 1:
            Em_AreaExitPoint(em, (f32 *) em->goal);
            Em_StartTurnToTarget(em, &EmBlangonga_TurnData.turn[1]);
            break;
        case 2:
            Em_StartTurnAway(em, &EmBlangonga_TurnData.turn[1]);
            break;
        default:
            Em_StartTurnToTarget(em, &EmBlangonga_TurnData.turn[1]);
        }
        break;
    case 1:
        if (Em_TurnStep(em, &EmBlangonga_TurnData.turn[1]) == 1) {
            if ((variant & 0xFF) == 1) {
                em->step++;
                EmBlangonga_ChangeState(em, EM_GROUP_FLY, BLANG_LEAP_OUT_OF_AREA, EM_SYNC_OWNER_FLAGGED);
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

/*
 * BLANG_MOVE_ACTION_A: plays BLANG_MOT_MOVE_ACTION_A once and ends the
 * state.
 */
void EmBlangonga_MoveActionA(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_MOVE_ACTION_A, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_MOVE_ACTION_B: plays BLANG_MOT_MOVE_ACTION_B once and ends the
 * state.
 */
void EmBlangonga_MoveActionB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_MOVE_ACTION_B, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_MOVE_ACTION_C: plays BLANG_MOT_MOVE_ACTION_C once and ends the
 * state.
 */
void EmBlangonga_MoveActionC(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_MOVE_ACTION_C, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_MOVE_ACTION_D and _ACTION_E: plays BLANG_MOT_MOVE_ACTION_D once and
 * ends the state.
 */
void EmBlangonga_MoveActionD(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_MOVE_ACTION_D, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_MOVE_START_AND_APPROACH (variant 0), _LIMP_APPROACH (1), _APPROACH
 * (2) and _START_AND_WALK (3): walks (0 and 3 start with
 * BLANG_MOT_WALK_START); 0-2 turn to the goal and stop after 120 frames or
 * within 700 (scaled) of it, 3 steps all the way to the goal.
 */
void EmBlangonga_Approach(Em *em, s32 unused, s32 variant)
{
    switch (em->step) {
    case 0:
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        em->timer = 0x78;
        switch (variant & 0xFF) {
        case 0:
        case 3:
        default:
            em->step++;
            Em_SetMotion(em, BLANG_MOT_WALK_START, 0, 0);
            break;
        case 1:
            em->step = 2;
            Em_SetMotion(em, BLANG_MOT_LIMP, 0, 0);
            break;
        case 2:
            em->step = 2;
            Em_SetMotion(em, BLANG_MOT_WALK, 0, 0);
            break;
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_WALK, 0, 0x44);
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

/* Runs the current EM_GROUP_MOVE state. The work area comes as an s32. */
void EmBlangonga_RunMoveState(Em *em, s32 work)
{
    switch (em->state) {
    case BLANG_MOVE_TURN:
        EmBlangonga_Turn(em, work, 0);
        break;
    case BLANG_MOVE_WALK:
        EmBlangonga_WalkToGoal(em, work, 0);
        break;
    case BLANG_MOVE_LIMP:
        EmBlangonga_WalkToGoal(em, work, 1);
        break;
    case BLANG_MOVE_TURN_QUICK:
        EmBlangonga_TurnQuick(em, work, 0);
        break;
    case BLANG_MOVE_ACTION_A:
        EmBlangonga_MoveActionA(em);
        break;
    case BLANG_MOVE_ACTION_B:
        EmBlangonga_MoveActionB(em);
        break;
    case BLANG_MOVE_ACTION_C:
        EmBlangonga_MoveActionC(em);
        break;
    case BLANG_MOVE_ACTION_D:
        EmBlangonga_MoveActionD(em);
        break;
    case BLANG_MOVE_TURN_TO_EXIT:
        EmBlangonga_TurnQuick(em, work, 1);
        break;
    case BLANG_MOVE_TURN_AWAY:
        EmBlangonga_Turn(em, work, 1);
        break;
    case BLANG_MOVE_TURN_AWAY_QUICK:
        EmBlangonga_TurnQuick(em, work, 2);
        break;
    case BLANG_MOVE_START_AND_APPROACH:
        EmBlangonga_Approach(em, work, 0);
        break;
    case BLANG_MOVE_LIMP_APPROACH:
        EmBlangonga_Approach(em, work, 1);
        break;
    case BLANG_MOVE_APPROACH:
        EmBlangonga_Approach(em, work, 2);
        break;
    case BLANG_MOVE_START_AND_WALK:
        EmBlangonga_Approach(em, work, 3);
        break;
    case BLANG_MOVE_ACTION_E:
        EmBlangonga_MoveActionD(em);
        
    default:
        break;
    }
}

/*
 * BLANG_LEAP_JUMP (variant 0), _JUMP_HIGH (1) and _JUMP_TO_GOAL (2):
 * crouches, takes off (frame 52, 56 or 48 by kind) on a curve, falls after
 * the apex and lands with a camera shake. The variant gives the speeds (2:
 * those that land on the goal in 60 frames); the Congalala kinds jump
 * lower.
 */
void EmBlangonga_Jump(Em *em, EmWork *work, u8 variant)
{
    f32 frame;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, BLANG_MOT_JUMP, 0, 0);
        Player_ClearVelocity(em);
        break;
    case 1:
        switch (em->type) {
        case EM_KIND_BLANGONGA:
        default:
        case EM_KIND_BLANGO:
            frame = 52.0f;
            break;
        case EM_KIND_CONGALALA:
        case EM_KIND_CONGA:
            frame = 56.0f;
            break;
        case EM_KIND_RAJANG:
            frame = 48.0f;
            break;
        }
        if (EmMotion_ReachedFrame(em, 0, frame)) {
            em->noReact = 1;
            em->stance = EM_STANCE_AIR;
            em->step++;
            if (variant == 2) {
                em->accelY = -6.4f;
                em->velY = (em->goal[1] - em->posY) / 60.0f - 60.0f * (0.5f * em->accelY);
                em->velZ = Vec_DistanceXZ(em->goal, &em->posX) / 60.0f;
            } else {
                switch (em->type) {
                case EM_KIND_BLANGONGA:
                default:
                case EM_KIND_BLANGO:
                case EM_KIND_RAJANG:
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
            Em_SetMotion(em, BLANG_MOT_JUMP_FALL, 0, 0);
            em->step++;
        }
        break;
    case 3:
        Em_MoveRotatedAccel(em, &work->accelAngX);
        if (em->posY <= em->floorY) {
            em->posY = em->floorY;
            em->step++;
            em->stance = EM_STANCE_GROUND;
            CamShake_ReqAIf(em, 5);
            switch (em->type) {
            case EM_KIND_BLANGONGA:
            default:
            case EM_KIND_BLANGO:
            case EM_KIND_RAJANG:
                Em_SetMotion(em, BLANG_MOT_JUMP_LAND, 0, 0);
                break;
            case EM_KIND_CONGALALA:
            case EM_KIND_CONGA:
                Em_SetMotion(em, BLANG_MOT_JUMP_LAND, 8, 0);
                break;
            }
        }
        break;
    case EM_KIND_MOSSWINE:
        if (em->motion[0].playing == 0) {
            em->noReact = 0;
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_LEAP_HOP_SHORT (variant 0), _HOP (1) and _HOP_LONG (2), the
 * Congalala kinds: takes off at frame 26 with a hit at frame 50, lands with
 * a camera shake. The variant gives the forward speed.
 */
void EmBlangonga_Hop(Em *em, EmWork *work, s32 variant)
{
    if (EmBlangonga_IsCongalala(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
        return;
    }
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, BLANG_MOT_CONGALALA_HOP, 0, 0);
        Player_ClearVelocity(em);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 26.0f)) {
            em->step++;
            em->stance = EM_STANCE_AIR;
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
        if (EmMotion_PassedFrameScaled(em, 50.0f, 0)) {
            BlangongaAttack_Create(em, 0x21);
        }
        Em_MoveRotatedAccel(em, &work->accelAngX);
        if (em->posY <= em->floorY) {
            em->posY = em->floorY;
            em->step++;
            em->stance = EM_STANCE_GROUND;
            if (EmBlangonga_IsSmallKind(em) == 1) {
                CamShake_ReqAIf(em, 2);
            } else {
                CamShake_ReqAIf(em, 5);
            }
            Em_SetMotion(em, BLANG_MOT_HOP_LAND, 0, 0);
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

/*
 * BLANG_LEAP_DIG_IN (the Blangonga kinds): digs into the snow (the shadow
 * goes from frame 80), then BLANG_LEAP_BURIED.
 */
void EmBlangonga_DigIn(Em *em)
{
    if (EmBlangonga_IsBlangonga(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
        return;
    }
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        if (em->motionId[0] != EM_MOTION_ID(0, BLANG_MOT_DIG_IN)) {
            Em_SetMotion(em, BLANG_MOT_DIG_IN, 0, 0);
        }
        em->noReact = 1;
        em->stance = EM_STANCE_GROUND;
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
            EmBlangonga_EndBuriedState(em);
        }
        break;
    }
    em->guardTimer = 2;
}

/*
 * BLANG_LEAP_BURIED: stays under the snow (stateArmor); when its motion
 * ends the owner hands the choice back to the script
 * (EmBlangonga_EndBuriedState).
 */
void EmBlangonga_Buried(Em *em)
{
    if (EmBlangonga_IsBlangonga(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
    } else {
        em->stateArmor = 1;
        em->noPushTimer = 2;
        switch (em->step) {
        case 0:
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->lookAtTarget = 0;
            Em_SetMotion(em, BLANG_MOT_BURIED, 0, 0);
            break;
        case 1:
            if ((em->remote == 0) && (em->motion[0].playing == 0)) {
                em->step++;
                EmBlangonga_EndBuriedState(em);
            }
            break;
        }
    }
}

/*
 * BLANG_LEAP_DIG_OUT (variant 0) and _DIG_OUT_RAGE (1: the rage raised by
 * hp on entry): comes up out of the snow with two hits (frames 2 and 52),
 * the shadow back from frame 40, then ends the state.
 */
void EmBlangonga_DigOut(Em *em, EmWork *work, s32 variant)
{
    if (EmBlangonga_IsBlangonga(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
        return;
    }
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        if (em->motionId[0] != EM_MOTION_ID(0, BLANG_MOT_EMERGE)) {
            Em_SetMotion(em, BLANG_MOT_EMERGE, 0, 0);
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
            if (EmMotion_PassedFrameScaled(em, 2.0f, 0)) {
                BlangongaAttack_Create(em, 6);
            }
            if (EmMotion_PassedFrameScaled(em, 52.0f, 0)) {
                BlangongaAttack_Create(em, 7);
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

/*
 * BLANG_LEAP_BURIED_TURN: turns toward the goal under the snow, checking
 * every 40 frames whether it faces it.
 */
void EmBlangonga_BuriedTurn(Em *em)
{
    u32 spd;
    u32 angleDiff;

    if (EmBlangonga_IsBlangonga(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
        return;
    }
    em->stateArmor = 1;
    em->noPushTimer = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        Em_SetMotion(em, BLANG_MOT_BURIED, 0, 0);
        em->timer = 0x28;
        break;
    case 1:
        spd = 409.6f * em->motionSpeed;
        angleDiff = (em->aimAngle - *(u16 *) &em->rotY) & 0xFFFF;
        if (--em->timer <= 0) {
            if (((angleDiff + spd) & 0xFFFF) < spd * 2) {
                em->step++;
                EmBlangonga_EndBuriedState(em);
            } else {
                em->timer = 0x28;
            }
        } else {
            if (((angleDiff + spd) & 0xFFFF) < spd * 2) {
                em->rotY = em->aimAngle;
            } else if (angleDiff < 0x8000) {
                em->rotY = (em->rotY + spd) & 0xFFFF;
            } else {
                em->rotY = (em->rotY - spd) & 0xFFFF;
            }
        }
        break;
    }
}

/*
 * BLANG_LEAP_BURIED_MOVE: moves under the snow to the goal, rumbling the
 * local player's pad when within 1000 of it.
 */
void EmBlangonga_BuriedMove(Em *em, EmWork *work)
{
    Player *player;

    player = &gPlayers[gScene.localPlayer];
    if (EmBlangonga_IsBlangonga(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
    } else {
        em->stateArmor = 1;
        em->noPushTimer = 2;
        switch (em->step) {
        case 0:
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->lookAtTarget = 0;
            Em_SetMotion(em, BLANG_MOT_BURIED, 0, 0x64);
            Player_ClearVelocity(em);
            em->velZ = 70.0f;
            work->accelAngX = em->rotX;
            work->accelAngY = em->rotY;
            work->accelAngZ = 0;
            break;
        case 1:
            Em_MoveRotatedAccel(em, &work->accelAngX);
            Em_StepToGoal(em, 0x10, 2);
            if (!(gScene.frame & 7) && (Obj_SameArea(em, player) != 0) && (Obj_InCurrentArea(player) != 0) && (Vec_Distance((f32 *) &em->posX, (f32 *) player->pos) < 1000.0f)) {
                Vib_Start(0, 2);
            }
            if (em->goalDist <= 0.0f) {
                em->step++;
                EmBlangonga_EndBuriedState(em);
            }
            break;
        }
    }
}

/*
 * BLANG_LEAP_BURIED_WAIT: waits 90 frames under the snow, then
 * BLANG_LEAP_BURIED.
 */
void EmBlangonga_BuriedWait(Em *em)
{
    em->stateArmor = 1;
    em->noPushTimer = 2;
    if (EmBlangonga_IsBlangonga(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->lookAtTarget = 0;
            Em_SetMotion(em, BLANG_MOT_BURIED, 0, 0);
            em->timer = 0x5A;
            break;
        case 1:
            if (--em->timer <= 0) {
                em->step++;
                EmBlangonga_EndBuriedState(em);
            }
            break;
        }
    }
}

/*
 * BLANG_LEAP_OUT_OF_AREA: takes off and rises for 150 frames, then drops
 * into the destination area (BLANG_LEAP_DROP_IN) or crosses to the next one
 * (BLANG_LEAP_CROSS_AREA). A remote copy waits for the owner's change.
 */
void EmBlangonga_LeapOutOfArea(Em *em, EmWork *work)
{
    f32 takeOffFrame;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, BLANG_MOT_JUMP, 0, 0);
        Player_ClearVelocity(em);
        break;
    case 1:
        switch (em->type) {
        case EM_KIND_BLANGONGA:
        default:
        case EM_KIND_BLANGO:
            takeOffFrame = 52.0f;
            break;
        case EM_KIND_CONGALALA:
        case EM_KIND_CONGA:
            takeOffFrame = 56.0f;
            break;
        case EM_KIND_RAJANG:
            takeOffFrame = 48.0f;
            break;
        }
        if (EmMotion_ReachedFrame(em, 0, takeOffFrame)) {
            em->step++;
            em->noReact = 1;
            em->stance = EM_STANCE_AIR;
            em->velY = 230.0f;
            em->velZ = 70.0f;
            em->accelY = -1.55f;
            work->accelAngX = em->rotX;
            work->accelAngY = em->rotY;
            work->accelAngZ = 0;
            em->timer = 150;
        }
        break;
    case EM_KIND_FATALIS:
        Em_MoveRotatedAccel(em, &work->accelAngX);
        if (--em->timer <= 0) {
            em->step++;
            if (em->nextArea == *(u16 *) &em->destArea || *(u16 *) &em->destArea == AREA_NONE) {
                EmBlangonga_EnterNewArea(em);
                EmBlangonga_ChangeState(em, EM_GROUP_FLY, BLANG_LEAP_DROP_IN, EM_SYNC_SEND);
                break;
            }
            Em_EnterNextArea(em);
            if (em->remote == 0) {
                em->goalNo = em->nextArea = *(u16 *) &em->destArea;
                em->goalKind = 3;
                Em_ComputeGoal(em, em->goal);
            }
            EmBlangonga_ChangeState(em, EM_GROUP_FLY, BLANG_LEAP_CROSS_AREA, EM_SYNC_SEND);
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

/*
 * BLANG_LEAP_DROP_IN: falls into the area from above and lands with a
 * camera shake; Blangonga and Rajang then restart their script.
 */
void EmBlangonga_DropIn(Em *em, EmWork *work)
{
    u16 leftArea;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        em->noReact = 1;
        Em_SetMotionRow(em, BLANG_MOT_JUMP_FALL, 0, 0);
        Player_ClearVelocity(em);
        em->velY = -30.0f;
        em->velZ = 0.0f;
        em->accelY = -4.7f;
        work->accelAngX = em->rotX;
        work->accelAngY = em->rotY;
        work->accelAngZ = 0;
        if (em->type == EM_KIND_BLANGONGA || em->type == EM_KIND_RAJANG) {
            leftArea = em->prevArea;
            Em_RestartScript(em);
            em->prevArea = leftArea;
        }
        break;
    case 1:
        Em_MoveRotatedAccel(em, &work->accelAngX);
        if (em->posY <= em->floorY) {
            em->posY = em->floorY;
            em->step++;
            em->stance = EM_STANCE_GROUND;
            CamShake_ReqAIf(em, 5);
            switch (em->type) {
            case EM_KIND_BLANGONGA:
            default:
            case EM_KIND_BLANGO:
            case EM_KIND_RAJANG:
                Em_SetMotion(em, BLANG_MOT_JUMP_LAND, 0, 0);
                break;
            case EM_KIND_CONGALALA:
            case EM_KIND_CONGA:
                Em_SetMotion(em, BLANG_MOT_JUMP_LAND, 8, 0);
                break;
            }
        }
        break;
    case EM_KIND_FATALIS:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->noReact = 0;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_LEAP_CROSS_AREA: unseen and unhittable, runs to the area's exit
 * until it leaves (or 300 frames), then changes area as
 * BLANG_LEAP_OUT_OF_AREA does.
 */
void EmBlangonga_CrossArea(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noPushTimer = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        em->noReact = 1;
        Em_SetMotion(em, BLANG_MOT_JUMP, 0, 0x38);
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
        if (em->nextArea == (u16) em->destArea || (u16) em->destArea == AREA_NONE) {
            EmBlangonga_EnterNewArea(em);
            EmBlangonga_ChangeState(em, EM_GROUP_FLY, BLANG_LEAP_DROP_IN, EM_SYNC_SEND);
            break;
        }
        Em_EnterNextArea(em);
        if (em->remote == 0) {
            em->goalNo = em->nextArea = *(u16 *) &em->destArea;
            em->goalKind = 3;
            Em_ComputeGoal(em, em->goal);
        }
        EmBlangonga_ChangeState(em, EM_GROUP_FLY, BLANG_LEAP_CROSS_AREA, EM_SYNC_SEND);
        break;
    case 3:
        if (em->remote == 0) {
            em->step = 2;
        }
        break;
    }
}

/* BLANG_LEAP_BURIED_WAIT_RANDOM: waits 1-16 random frames under the snow. */
void EmBlangonga_BuriedWaitRandom(Em *em)
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
            em->stance = EM_STANCE_GROUND;
            em->lookAtTarget = 0;
            Em_SetMotion(em, BLANG_MOT_BURIED, 0, 0);
            em->timer = (Rand_Next(1) & 0xFFFF & 0xF) + 1;
            break;
        case 1:
            if (--em->timer <= 0) {
                em->step++;
                EmBlangonga_EndBuriedState(em);
            }
            break;
        }
    }
}

/*
 * Runs the current group-2 state (the family's EM_GROUP_FLY: jumps, leaps
 * and burrowing).
 */
void EmBlangonga_RunLeapState(Em *em, EmWork *work)
{
    switch (em->state) {
    case BLANG_LEAP_JUMP:
        EmBlangonga_Jump(em, work, 0);
        break;
    case BLANG_LEAP_HOP_SHORT:
        EmBlangonga_Hop(em, work, 0);
        break;
    case BLANG_LEAP_DIG_IN:
        EmBlangonga_DigIn(em);
        break;
    case BLANG_LEAP_BURIED:
        EmBlangonga_Buried(em);
        break;
    case BLANG_LEAP_DIG_OUT:
        EmBlangonga_DigOut(em, work, 0);
        break;
    case BLANG_LEAP_BURIED_TURN:
        EmBlangonga_BuriedTurn(em);
        break;
    case BLANG_LEAP_BURIED_MOVE:
        EmBlangonga_BuriedMove(em, work);
        break;
    case BLANG_LEAP_BURIED_WAIT:
        EmBlangonga_BuriedWait(em);
        break;
    case BLANG_LEAP_OUT_OF_AREA:
        EmBlangonga_LeapOutOfArea(em, work);
        break;
    case BLANG_LEAP_DROP_IN:
        EmBlangonga_DropIn(em, work);
        break;
    case BLANG_LEAP_DIG_OUT_RAGE:
        EmBlangonga_DigOut(em, work, 1);
        break;
    case BLANG_LEAP_CROSS_AREA:
        EmBlangonga_CrossArea(em, work);
        break;
    case BLANG_LEAP_JUMP_HIGH:
        EmBlangonga_Jump(em, work, 1);
        break;
    case BLANG_LEAP_BURIED_WAIT_RANDOM:
        EmBlangonga_BuriedWaitRandom(em);
        break;
    case BLANG_LEAP_HOP:
        EmBlangonga_Hop(em, work, 1);
        break;
    case BLANG_LEAP_HOP_LONG:
        EmBlangonga_Hop(em, work, 2);
        break;
    case BLANG_LEAP_JUMP_TO_GOAL:
        EmBlangonga_Jump(em, work, 2);
        
    default:
        break;
    }
}

/*
 * BLANG_ATK_TRIPLE_HIT (the Blangonga kinds): turns to the goal for 40
 * frames and hits three times (frames 36, 64 and 86; the first behind it).
 */
void EmBlangonga_TripleHit(Em *em)
{
    if (EmBlangonga_IsBlangonga(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, BLANG_MOT_TRIPLE_HIT, 0, 0);
            break;
        case 1:
            if (EmMotion_ReachedFrame(em, 0, 40.0f) == 0) {
                Em_TurnToGoal(em, 0x60, 0);
            }
            if (EmMotion_PassedFrameScaled(em, 36.0f, 0U) != 0) {
                BlangongaAttack_Create(em, 0x12);
            }
            if (EmMotion_PassedFrameScaled(em, 64.0f, 0U) != 0) {
                BlangongaAttack_Create(em, 0x18);
            }
            if (EmMotion_PassedFrameScaled(em, 86.0f, 0U) != 0) {
                BlangongaAttack_Create(em, 1);
            }
            if (em->motion[0].playing == 0) {
                em->step++;
                Em_SetMotion(em, BLANG_MOT_TRIPLE_HIT_END, 0, 0);
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

/*
 * BLANG_ATK_SLAM (variant 0) and _SLAM_HEAVY (1): winds up for 20 or 40
 * frames and slams (a hit at frame 20); at frame 30 the large kinds add a
 * heavier hit and a camera shake, and the heavy slam also brings down the
 * area's falling objects (Blangonga and Rajang).
 */
void EmBlangonga_Slam(Em *em, EmWork *work, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_SLAM_WINDUP, 0, 0);
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
            Em_SetMotion(em, BLANG_MOT_SLAM, 0, 0);
        }
        break;
    case 2:
        if (EmMotion_PassedFrameScaled(em, 20.0f, 0)) {
            BlangongaAttack_Create(em, 2);
        }
        if (EmBlangonga_IsSmallKind(em) == 0 && EmMotion_PassedFrameScaled(em, 30.0f, 0)) {
            switch (variant & 0xFF) {
            case 1:
                BlangongaAttack_Create(em, 8);
                CamShake_ReqAIf(em, 5);
                if (EmBlangonga_IsBlangonga(em) == 1 || EmBlangonga_IsRajang(em) == 1) {
                    EmBlangonga_DropFallingObjects(em);
                }
                break;
            default:
                CamShake_ReqAIf(em, 3);
                BlangongaAttack_Create(em, 0x19);
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

/*
 * BLANG_ATK_STRIKE_A (variant 0) and _B (1), any kind: one blow with a hit
 * at frame 14 (the small kinds' weaker hit).
 */
void EmBlangonga_Strike(Em *em, EmWork *work, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        switch (variant & 0xFF) {
        case 0:
        default:
            Em_SetMotion(em, BLANG_MOT_STRIKE_A, 0, 0);
            break;
        case 1:
            Em_SetMotion(em, BLANG_MOT_STRIKE_B, 0, 0);
            break;
        }
        break;
    case 1:
        switch (variant & 0xFF) {
        case 0:
        default:
            if (EmMotion_PassedFrameScaled(em, 14.0f, 0)) {
                if (EmBlangonga_IsSmallKind(em) == 0) {
                    BlangongaAttack_Create(em, 3);
                } else {
                    BlangongaAttack_Create(em, 0x15);
                }
            }
            break;
        case 1:
            if (EmMotion_PassedFrameScaled(em, 14.0f, 0)) {
                if (EmBlangonga_IsSmallKind(em) == 0) {
                    BlangongaAttack_Create(em, 4);
                } else {
                    BlangongaAttack_Create(em, 0x16);
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

/*
 * BLANG_ATK_CONGALALA_EMISSION (the Congalala kinds): plays its motion and
 * at frame 110 creates effect 33 of kind 6 at bone 0x1C.
 */
void EmBlangonga_CongalalaEmission(Em *em)
{
    if (EmBlangonga_IsCongalala(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, BLANG_MOT_CONGALALA_EMISSION, 0, 0);
            break;
        case 1:
            if (EmMotion_PassedFrameScaled(em, 110.0f, 0U) != 0) {
                EmEffectAttack_Create(em, 6, 0, 0, 0);
            }
            if (em->motion[0].playing == 0) {
                em->step++;
                EmBlangonga_EndState(em, 0, 0);
            }
            break;
        }
    }
}

/*
 * BLANG_ATK_BLANGO_THROW (the Blango): throws a projectile (EmRangedAttack type
 * 0x11) from bone 0xE at frame 74.
 */
void EmBlangonga_BlangoThrow(Em *em)
{
    if (em->type != EM_KIND_BLANGO) {
        EmBlangonga_EndState(em, 0, 0);
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, BLANG_MOT_THROW, 0, 0);
            break;
        case 1:
            if (EmMotion_PassedFrameScaled(em, 74.0f, 0U) != 0) {
                EmRangedAttack_CreateAtBone(em, 0xE, 0x11, 0, 0xFC00, 0);
            }
            if (em->motion[0].playing == 0) {
                em->step++;
                EmBlangonga_EndState(em, 0, 0);
            }
            break;
        }
    }
}

/*
 * BLANG_ATK_CHARGE (not the Congalala kinds): runs at the goal with a hit
 * along, then finishes with BLANG_MOT_CHARGE_END and its hits.
 */
void EmBlangonga_Charge(Em *em)
{
    if (EmBlangonga_IsCongalala(em) == 1) {
        EmBlangonga_EndState(em, 0, 0);
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            em->stateVar0 = 0;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, BLANG_MOT_CHARGE, 0, 0);
            break;
        case 1:
            if (EmMotion_PassedFrameScaled(em, 6.0f, 0U) != 0) {
                BlangongaAttack_Create(em, 0xF);
            }
            if ((em->stateVar0 == 0) && (Em_StepToGoal(em, 0x80, 0) == 1)) {
                em->step++;
                em->stateVar0++;
                Em_SetMotion(em, BLANG_MOT_CHARGE_END, 0, 0);
            }
            break;
        case 2:
            if (EmBlangonga_IsBlangonga(em) == 1) {
                if (EmMotion_PassedFrameScaled(em, 10.0f, 0U) != 0) {
                    BlangongaAttack_Create(em, 0x11);
                }
                if (EmMotion_PassedFrameScaled(em, 20.0f, 0U) != 0) {
                    if (EmBlangonga_IsSmallKind(em) == 1) {
                        BlangongaAttack_Create(em, 0x1A);
                    } else {
                        BlangongaAttack_Create(em, 5);
                    }
                }
            } else if (EmMotion_PassedFrameScaled(em, 22.0f, 0U) != 0) {
                BlangongaAttack_Create(em, 0x25);
            }
            if (em->motion[0].playing == 0) {
                em->step++;
                EmBlangonga_EndState(em, 0, 0);
            }
            break;
        }
    }
}

/*
 * BLANG_ATK_DIG_THROW_A, _B and _C (variants 0-2; Blangonga): digs up a
 * lump with a hit at frame 98 and throws it (EmRangedAttack type 0xF from bone
 * 0xD) at frame 102; the variant sets the throw's pitch.
 */
void EmBlangonga_DigThrow(Em *em, EmWork *work, s32 variant)
{
    u16 ang;

    if (em->type != EM_KIND_BLANGONGA) {
        EmBlangonga_EndState(em, 0, 0);
        return;
    }
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_DIG_THROW, 0, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 98.0f, 0)) {
            BlangongaAttack_Create(em, 0x13);
        }
        if (EmMotion_PassedFrameScaled(em, 102.0f, 0)) {
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
            EmRangedAttack_CreateAtBone(em, 0xD, 0xF, 0, ang, 0);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_ATK_CONGALALA_EATEN_EMISSION (Congalala): at frame 100, when it has
 * eaten (eatenKind), creates stage object 0x17 and effect 33 of kind 1 by
 * what it ate.
 */
void EmBlangonga_CongalalaEatenEmission(Em *em, EmWork *work)
{
    EmBlangongaWork *w = (EmBlangongaWork *)work;

    if (em->type != EM_KIND_CONGALALA) {
        EmBlangonga_EndState(em, 0, 0);
        return;
    }
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_CONGALALA_EATEN_EMISSION, 0, 0);
        break;
    case 1:
        if (w->eatenKind != 0 && EmMotion_PassedFrameScaled(em, 100.0f, 0)) {
            StageObj_K1C_CreateOnEmC(em, 0x17, w->eatenKind - 1, 1.0f);
            EmEffectAttack_Create(em, 1, w->eatenKind - 1, 0, 0);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_ATK_CONGALALA_COMBO (the Congalala kinds): a long combo with a hit
 * from frame 26, half a turn over frames 170-180 and an impact at frame 200
 * (the large kind), then BLANG_MOT_CONGALALA_COMBO_END.
 */
void EmBlangonga_CongalalaCombo(Em *em, EmWork *work)
{
    if (EmBlangonga_IsCongalala(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
        return;
    }
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_CONGALALA_COMBO, 0, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 26.0f, 0)) {
            BlangongaAttack_Create(em, 0x1F);
        }
        if (EmMotion_InFrameRange(em, 0, 170.0f, 180.0f)) {
            em->rotY += 0xCCC;
        }
        if (EmBlangonga_IsSmallKind(em) == 0 && EmMotion_PassedFrameScaled(em, 200.0f, 0)) {
            EmImpactAttack_CreateAt(em, &em->posX, 0x10);
            CamShake_ReqAIf(em, 5);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_CONGALALA_COMBO_END, 0, 0);
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

/*
 * BLANG_ATK_CONGALALA_HITZONE_COMBO (Congalala): a hit from frame 40 with
 * the second damage table (hitzoneSet 1) until the first motion ends, then
 * a second blow with a hit at frame 88.
 */
void EmBlangonga_CongalalaHitzoneCombo(Em *em)
{
    if (em->type != EM_KIND_CONGALALA) {
        EmBlangonga_EndState(em, 0, 0);
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, BLANG_MOT_CONGALALA_HITZONE_COMBO, 0, 0);
            break;
        case 1:
            if (EmMotion_PassedFrameScaled(em, 40.0f, 0U) != 0) {
                BlangongaAttack_Create(em, 0x20);
                em->hitzoneSet = 1;
            }
            if (em->motion[0].playing == 0) {
                em->step++;
                Em_SetMotion(em, BLANG_MOT_CONGALALA_HITZONE_COMBO_END, 0, 0);
                em->hitzoneSet = 0;
            }
            break;
        case 2:
            if (EmMotion_PassedFrameScaled(em, 88.0f, 0U) != 0) {
                BlangongaAttack_Create(em, 0x29);
            }
            if (em->motion[0].playing == 0) {
                em->step++;
                EmBlangonga_EndState(em, 0, 0);
            }
            break;
        }
    }
}

/*
 * BLANG_ATK_CONGALALA_THROW_A (variant 0) and _B (1), the Congalala kinds:
 * throws a projectile (EmRangedAttack type 0x13) from bone 0x21 at frame 66; the
 * variant sets its pitch.
 */
void EmBlangonga_CongalalaThrow(Em *em, EmWork *work, s32 variant)
{
    if (EmBlangonga_IsCongalala(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, BLANG_MOT_THROW, 0, 0);
            break;
        case 1:
            if (!(variant & 0xFF)) {
                if (EmMotion_PassedFrameScaled(em, 66.0f, 0U) != 0) {
                    EmRangedAttack_CreateAtBone(em, 0x21, 0x13, 0, 0x93F, 0);
                }
            } else if (EmMotion_PassedFrameScaled(em, 66.0f, 0U) != 0) {
                EmRangedAttack_CreateAtBone(em, 0x21, 0x13, 0, 0x2000, 0);
            }
            if (em->motion[0].playing == 0) {
                em->step++;
                EmBlangonga_EndState(em, 0, 0);
            }
            break;
        }
    }
}

/*
 * BLANG_ATK_BACK_STRIKE: plays BLANG_MOT_BACK_STRIKE to its end; its hits
 * come from EmBlangonga_MotionEffects.
 */
void EmBlangonga_BackStrike(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_BACK_STRIKE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_ATK_EFFECT_STRIKE (the Blangonga kinds): at frame 98 creates stage
 * object 8 on itself and a hit, then plays the end motion. It also works
 * out a point in front of bone 1 that nothing reads.
 */
void EmBlangonga_EffectStrike(Em *em, EmWork *work)
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
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_EFFECT_STRIKE, 0, 0);
        break;
    case 1:
        if (em->stateVar1 == 0 && EmMotion_PassedFrameScaled(em, 98.0f, 0)) {
            StageObj_K1C_CreateOnEm(em, 8, 0.75f * em->scaleX);
            BlangongaAttack_Create(em, 0xA);
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
            Em_SetMotion(em, BLANG_MOT_EFFECT_STRIKE_END, 0, 0);
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

/*
 * BLANG_ATK_RAJANG_COMBO_SLAM_A (variant 0) and _B (1): a blow with a hit,
 * then 3 (or 1) frames after its end a slam whose impact at frame 26 shakes
 * the camera and brings down the area's falling objects.
 */
void EmBlangonga_RajangComboSlam(Em *em, EmWork *work, s32 variant)
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
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_STRIKE_A, 0, 0);
        if ((variant & 0xFF) == 0) {
            em->timer = 3;
        } else {
            em->timer = 1;
        }
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 16.0f, 0) && em->stateVar0 == 0) {
            em->stateVar0++;
            BlangongaAttack_Create(em, 0x24);
        }
        if (em->motion[0].playing == 0 && --em->timer <= 0) {
            em->step++;
            em->stateVar0 = 0;
            Em_SetMotion(em, BLANG_MOT_STRIKE_B, 0, 0);
        }
        break;
    case 2:
        if (EmMotion_PassedFrameScaled(em, 26.0f, 0) && em->stateVar0 == 0) {
            em->stateVar0++;
            Em_BonePos(em, 0xD, pos);
            pos[1] = em->floorY;
            EmImpactAttack_CreateAt(em, pos, 0xF);
            CamShake_ReqAIf(em, 5);
            EmBlangonga_DropFallingObjects(em);
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

/*
 * BLANG_ATK_RAJANG_SHOT_A (variant 0) and _B (1): aims and at frame 84
 * fires effect 33 of kind 2 with stage object 0x25; the variant sets the
 * pitch.
 */
void EmBlangonga_RajangShot(Em *em, EmWork *work, s32 variant)
{
    if (EmBlangonga_IsRajang(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
        return;
    }
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_RAJANG_AIM, 2, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 84.0f, 0)) {
            em->step++;
            switch (variant & 0xFF) {
            case 0:
            default:
                EmEffectAttack_Create(em, 2, 0, 0x71C, 0);
                StageObj_K1C_CreateOnEmB(em, 0x25, 0x71C, 0, 1.0f);
                break;
            case 1:
                EmEffectAttack_Create(em, 2, 0, 0xF8E5, 0);
                StageObj_K1C_CreateOnEmB(em, 0x25, 0xF8E5, 0, 1.0f);
                break;
            }
            Em_SetMotion(em, BLANG_MOT_RAJANG_AIM_END, 2, 0);
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

/*
 * BLANG_ATK_RAJANG_BEAM_A (variant 0) and _B (1): aims and at frame 80
 * fires the beam (EmRangedAttack type 0x15, the variant passed on) from bone 1 for
 * 0x28 frames.
 */
void EmBlangonga_RajangBeam(Em *em, EmWork *work, s32 variant)
{
    if (EmBlangonga_IsRajang(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
        return;
    }
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_RAJANG_AIM, 0, 0);
        em->timer = 2;
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 80.0f, 0)) {
            switch (variant & 0xFF) {
            case 0:
            default:
                EmRangedAttack_CreateAtBone2(em, 1, 0x15, 0, 0, 0, 0x28);
                break;
            case 1:
                EmRangedAttack_CreateAtBone2(em, 1, 0x15, 1, 0, 0, 0x28);
                break;
            }
        }
        if (em->motion[0].playing == 0) {
            em->timer--;
        }
        if (em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_RAJANG_AIM_END, 0, 0);
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

/*
 * BLANG_ATK_RAJANG_JUMP_SHOT (variant 0) and BLANG_REACT_TRAP_ESCAPE (1:
 * unguarded hits and the trap's hold set): jumps, fires a shot downward
 * from the air (frame 22) while held above the floor, and lands when the
 * curves end.
 */
void EmBlangonga_RajangJumpShot(Em *em, EmWork *work, s32 variant)
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
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, BLANG_MOT_RAJANG_JUMP, 0, 0);
            if (k == 1) {
                em->holdTimer = 0x32;
            }
            break;
        case 1:
            if (EmMotion_PassedFrameScaled(em, 24.0f, 0U) != 0) {
                em->step++;
                em->stance = EM_STANCE_AIR;
                em->posY += 170.0f;
                EmBlangonga_StartCurves(em, 1U);
            }
            break;
        case 2:
            EmBlangonga_UpdateCurves(em);
            if (EmMotion_PassedFrameScaled(em, 30.0f, 0U) != 0) {
                em->step++;
                Em_SetMotion(em, BLANG_MOT_RAJANG_JUMP_SHOT, 6, 0);
                EmBlangonga_StartCurves(em, 2U);
            }
            break;
        case 3:
            if (EmMotion_PassedFrameScaled(em, 22.0f, 0U) != 0) {
                EmEffectAttack_Create(em, 2, 1, 0x31C7, 0);
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
                em->stance = EM_STANCE_GROUND;
                Em_SetMotion(em, BLANG_MOT_RAJANG_JUMP_SHOT_LAND, 0, 0);
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

/* BLANG_ATK_RAJANG_STRIKE: one blow with a hit at frame 56. */
void EmBlangonga_RajangStrike(Em *em)
{
    if (EmBlangonga_IsRajang(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, BLANG_MOT_RAJANG_STRIKE, 2, 0);
            break;
        case 1:
            if (EmMotion_PassedFrameScaled(em, 56.0f, 0U) != 0) {
                BlangongaAttack_Create(em, 0x26);
            }
            if (em->motion[0].playing == 0) {
                em->step++;
                EmBlangonga_EndState(em, 0, 0);
            }
            break;
        }
    }
}

/*
 * BLANG_ATK_RAJANG_CHARGE: runs at the goal with a hit that lasts into the
 * end motion.
 */
void EmBlangonga_RajangCharge(Em *em)
{
    u8 stateVar0;

    if (EmBlangonga_IsRajang(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            em->stateVar0 = 0;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, BLANG_MOT_RAJANG_CHARGE, 2, 0);
            break;
        case 1:
            if (EmMotion_PassedFrameScaled(em, 6.0f, 0U) != 0) {
                stateVar0 = em->stateVar0;
                if (stateVar0 == 0) {
                    em->stateVar0 = stateVar0 + 1;
                    BlangongaAttack_Create(em, 0x27);
                }
            }
            if (Em_StepToGoal(em, 0x80, 0) == 1) {
                em->step++;
                Em_SetMotion(em, BLANG_MOT_RAJANG_CHARGE_END, 0, 0);
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

/*
 * BLANG_ATK_CONGALALA_CHARGE (the Congalala kinds): runs at the goal with a
 * hit along (the small kind's weaker), then ends the state.
 */
void EmBlangonga_CongalalaCharge(Em *em)
{
    if (EmBlangonga_IsCongalala(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, BLANG_MOT_CHARGE, 0, 0);
            if (EmBlangonga_IsSmallKind(em) == 1) {
                BlangongaAttack_Create(em, 0x23);
                break;
            }
            BlangongaAttack_Create(em, 0x1D);
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

/*
 * Runs the current EM_GROUP_ATTACK state, passing the shared functions
 * their variant.
 */
void EmBlangonga_RunAttackState(Em *em, EmWork *work)
{
    switch (em->state) {
    case BLANG_ATK_TRIPLE_HIT:
        EmBlangonga_TripleHit(em);
        break;
    case BLANG_ATK_SLAM:
        EmBlangonga_Slam(em, work, 0);
        break;
    case BLANG_ATK_STRIKE_A:
        EmBlangonga_Strike(em, work, 0);
        break;
    case BLANG_ATK_STRIKE_B:
        EmBlangonga_Strike(em, work, 1);
        break;
    case BLANG_ATK_CONGALALA_EMISSION:
        EmBlangonga_CongalalaEmission(em);
        break;
    case BLANG_ATK_BLANGO_THROW:
        EmBlangonga_BlangoThrow(em);
        break;
    case BLANG_ATK_CHARGE:
        EmBlangonga_Charge(em);
        break;
    case BLANG_ATK_DIG_THROW_A:
        EmBlangonga_DigThrow(em, work, 0);
        break;
    case BLANG_ATK_CONGALALA_EATEN_EMISSION:
        EmBlangonga_CongalalaEatenEmission(em, work);
        break;
    case BLANG_ATK_CONGALALA_COMBO:
        EmBlangonga_CongalalaCombo(em, work);
        break;
    case BLANG_ATK_CONGALALA_HITZONE_COMBO:
        EmBlangonga_CongalalaHitzoneCombo(em);
        break;
    case BLANG_ATK_CONGALALA_THROW_A:
        EmBlangonga_CongalalaThrow(em, work, 0);
        break;
    case BLANG_ATK_BACK_STRIKE:
        EmBlangonga_BackStrike(em);
        break;
    case BLANG_ATK_SLAM_HEAVY:
        EmBlangonga_Slam(em, work, 1);
        break;
    case BLANG_ATK_EFFECT_STRIKE:
        EmBlangonga_EffectStrike(em, work);
        break;
    case BLANG_ATK_DIG_THROW_B:
        EmBlangonga_DigThrow(em, work, 1);
        break;
    case BLANG_ATK_DIG_THROW_C:
        EmBlangonga_DigThrow(em, work, 2);
        break;
    case BLANG_ATK_RAJANG_COMBO_SLAM_A:
        EmBlangonga_RajangComboSlam(em, work, 0);
        break;
    case BLANG_ATK_RAJANG_SHOT_A:
        EmBlangonga_RajangShot(em, work, 0);
        break;
    case BLANG_ATK_RAJANG_BEAM_A:
        EmBlangonga_RajangBeam(em, work, 0);
        break;
    case BLANG_ATK_RAJANG_JUMP_SHOT:
        EmBlangonga_RajangJumpShot(em, work, 0);
        break;
    case BLANG_ATK_RAJANG_STRIKE:
        EmBlangonga_RajangStrike(em);
        break;
    case BLANG_ATK_RAJANG_CHARGE:
        EmBlangonga_RajangCharge(em);
        break;
    case BLANG_ATK_CONGALALA_CHARGE:
        EmBlangonga_CongalalaCharge(em);
        break;
    case BLANG_ATK_RAJANG_BEAM_B:
        EmBlangonga_RajangBeam(em, work, 1);
        break;
    case BLANG_ATK_CONGALALA_THROW_B:
        EmBlangonga_CongalalaThrow(em, work, 1);
        break;
    case BLANG_ATK_RAJANG_COMBO_SLAM_B:
        EmBlangonga_RajangComboSlam(em, work, 1);
        break;
    case BLANG_ATK_RAJANG_SHOT_B:
        EmBlangonga_RajangShot(em, work, 1);
        break;
    }
}

/* BLANG_REACT_FLINCH: flinches (BLANG_MOT_FLINCH) and ends the state. */
void EmBlangonga_Flinch(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_FLINCH, 0, 0);
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

/*
 * BLANG_REACT_FLINCH_B and _PART_BREAK: flinches (BLANG_MOT_FLINCH_B) and
 * ends the state.
 */
void EmBlangonga_FlinchB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_FLINCH_B, 0, 0);
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

/*
 * BLANG_REACT_KNOCK_DOWN (variant 0), _KNOCK_DOWN_B (1), _STUN (2) and
 * _STUN_B (3): is knocked down (the odd variants from the other side), lies
 * and gets up. The knock-downs lie 90 frames; the stuns add the stun's
 * stage object, lie until the status-5 timer runs out and then shake it off
 * (BLANG_IDLE_STUN_SHAKE_OFF).
 */
void EmBlangonga_KnockDown(Em *em, EmWork *work, u8 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        if (!(variant & 1)) {
            Em_SetMotion(em, BLANG_MOT_KNOCK_DOWN, 0, 0);
        } else {
            Em_SetMotion(em, BLANG_MOT_KNOCK_DOWN_B, 0, 0);
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
                Em_SetMotion(em, BLANG_MOT_DOWN, 0, 0);
            } else {
                Em_SetMotion(em, BLANG_MOT_DOWN_B, 0, 0);
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
                Em_SetMotion(em, BLANG_MOT_DOWN_GET_UP, 0, 0);
            } else {
                Em_SetMotion(em, BLANG_MOT_DOWN_GET_UP_B, 0, 0);
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
                EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_STUN_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
                break;
            }
        }
        break;
    case 4:
        if (em->remote == 0) {
            EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_STUN_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * BLANG_REACT_STATUS4: hit-react mode 4 with the periodic effect at bone 2
 * until the status-4 timer runs out, then BLANG_IDLE_STATUS4_SHAKE_OFF.
 */
void EmBlangonga_Status4(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_STATUS);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_STATUS4, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_STATUS4_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_STATUS4_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
    Em_SpawnStatus4Effect(em, 2);
}

/*
 * BLANG_REACT_TRAPPED: caught in a trap (hit-react mode 3, holdTimer 50)
 * while BLANG_MOT_TRAPPED plays, then BLANG_REACT_TRAPPED_STRUGGLE.
 */
void EmBlangonga_Trapped(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_NO_FLINCH);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, BLANG_MOT_TRAPPED, 0, 0);
        em->noReact = 1;
        em->holdTimer = 0x32;
        break;
    case 1:
        if ((s8) em->holdTimer < 5) {
            em->holdTimer = 5;
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_TRAPPED_STRUGGLE, EM_SYNC_OWNER);
        }
        break;
    case 2:
        if ((s8) em->holdTimer < 5) {
            em->holdTimer = 5;
        }
        if (em->remote == 0) {
            EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_TRAPPED_STRUGGLE, EM_SYNC_OWNER);
        }
        break;
    }
}

/*
 * BLANG_REACT_TRAPPED_STRUGGLE, _STATUS4_DOWNED_END and _STUN_DOWNED_END:
 * struggles in the hold (holdTimer kept at 5) until the trap's timer runs
 * out, then BLANG_IDLE_LEAVE_TRAP. An enraged Congalala makes effect 33 of
 * kind 7 in some loops.
 */
void EmBlangonga_TrappedStruggle(Em *em)
{
    u8 stateVar1;

    em->holdTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_STRUGGLE, 0, 0);
        em->noReact = 1;
        em->stateVar1 = 0;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            Em_SetMotion(em, BLANG_MOT_STRUGGLE, 0, 0);
            em->stateVar1++;
        }
        if ((em->type == EM_KIND_CONGALALA) && (em->rage != 0) && (Motion_PassedFrame(em, 0, 104.0f) != 0)) {
            stateVar1 = em->stateVar1;
            if ((stateVar1 & 7) == (((s32) em->stateRand >> (stateVar1 & 3)) & 7)) {
                EmEffectAttack_Create(em, 7, 0, 0, 0);
            }
        }
        if (--em->trapTimer <= 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_STRUGGLE, 0, 0);
            EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_LEAVE_TRAP, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_LEAVE_TRAP, EM_SYNC_OWNER_FLAGGED);
        }
        if (em->motion[0].playing == 0) {
            Em_SetMotion(em, BLANG_MOT_STRUGGLE, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_REACT_FALL: falls out of the air on curve 0 and lands within 180
 * (scaled) of the floor, lies 60 frames, gets up and ends the state.
 */
void EmBlangonga_Fall(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, BLANG_MOT_FALL, 0, 0);
        EmBlangonga_StartCurves(em, 0U);
        break;
    case 1:
        EmBlangonga_UpdateCurves(em);
        if (em->posY <= (em->floorY + (180.0f * em->scaleX))) {
            em->step++;
            em->posY = em->floorY;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, BLANG_MOT_FALL_LAND, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 0x3C;
            Em_SetMotion(em, BLANG_MOT_DOWN, 0, 0);
        }
        break;
    case 3:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_DOWN_GET_UP, 0, 0);
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

/* BLANG_REACT_NONE: does nothing. */
void EmBlangonga_ReactNop(void)
{
}

/*
 * BLANG_REACT_STATUS4_DOWNED (variant 0) and _STUN_DOWNED (1): lies in the
 * hold until the status timer runs out, then the matching _DOWNED_END
 * state; variant 0 adds the periodic effect, variant 1 the stun's stage
 * object. Blangonga first comes up out of the pit (BLANG_MOT_EMERGE to
 * frame 14).
 */
void EmBlangonga_HeldDowned(Em *em, EmWork *work, u8 variant)
{
    u16 curMotion;

    em->holdTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        if (em->type == EM_KIND_BLANGONGA && (curMotion = em->motionId[0], curMotion != EM_MOTION_ID(0, BLANG_MOT_EMERGE)) && curMotion != EM_MOTION_ID(0, BLANG_MOT_STRUGGLE) && curMotion != EM_MOTION_ID(0, BLANG_MOT_DOWNED)) {
            Em_SetMotion(em, BLANG_MOT_EMERGE, 0, 0);
            em->stateArmor = 1;
            em->noPushTimer = 5;
        } else {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_DOWNED, 0, 0);
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
            Em_SetMotion(em, BLANG_MOT_DOWNED, 0, 0);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetTrapTimeGoal(em);
            switch (variant) {
            case 0:
            default:
                EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_STATUS4_DOWNED_END, EM_SYNC_OWNER);
                break;
            case 1:
                EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_STUN_DOWNED_END, EM_SYNC_OWNER);
                break;
            }
        }
        break;
    case 3:
        if (em->remote == 0) {
            switch (variant) {
            case 0:
            default:
                EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_STATUS4_DOWNED_END, EM_SYNC_OWNER);
                break;
            case 1:
                EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_STUN_DOWNED_END, EM_SYNC_OWNER);
                break;
            }
        }
        break;
    }
    if (variant != 1) {
        Em_SpawnStatus4Effect(em, 2);
    }
}

/*
 * BLANG_REACT_STATUS2_DOWNED_END and _STATUS1_DOWNED_END: blinded briefly,
 * struggles once, then BLANG_REACT_TRAPPED_STRUGGLE.
 */
void EmBlangonga_HeldStatusEnd(Em *em)
{
    em->holdTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, BLANG_MOT_STRUGGLE, 0, 0);
        em->noReact = 1;
        em->blind = 2;
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetTrapTimeGoal(em);
            em->blind = 0;
            EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_TRAPPED_STRUGGLE, EM_SYNC_OWNER);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_TRAPPED_STRUGGLE, EM_SYNC_OWNER);
        }
        break;
    }
}

/*
 * BLANG_REACT_HOLD2: held by the second hold (Em_SetHold2 every frame, root
 * motion off) until the kind timer runs out, then
 * BLANG_IDLE_HOLD2_SHAKE_OFF.
 */
void EmBlangonga_Hold2(Em *em)
{
    Em_SetHold2(em);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, BLANG_MOT_STATUS4, 0, 0);
        Player_SetFlag((Player *) em, EM_FLAG_NO_ROOT_MOTION);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_HOLD2_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_HOLD2_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * BLANG_REACT_DROP_CARRIED (variant 0) and _DROP_CARRIED_A..C (1-3):
 * Congalala drops its carried item; variants 1-3 leave it as timed mine
 * point variant - 1.
 */
void EmBlangonga_DropCarried(Em *em, EmWork *work, s32 variant)
{
    s32 k;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_DROP_CARRIED, 0, 0);
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

/*
 * BLANG_REACT_FLINCH_ATTACK (Congalala): flinches and at frame 94 turns it
 * into the hitzone combo's two blows.
 */
void EmBlangonga_FlinchAttack(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_FLINCH_B, 0, 0x32);
        Em_RestartScript(em);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 94.0f, 0U) != 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_CONGALALA_HITZONE_COMBO, 0xC, 0x14);
        }
        break;
    case 2:
        if (EmMotion_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            BlangongaAttack_Create(em, 0x20);
            em->hitzoneSet = 1;
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_CONGALALA_HITZONE_COMBO_END, 0, 0);
            em->hitzoneSet = 0;
        }
        break;
    case 3:
        if (EmMotion_PassedFrameScaled(em, 88.0f, 0U) != 0) {
            BlangongaAttack_Create(em, 0x29);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_REACT_TRAP_ESCAPE: Rajang leaps out of the trap
 * (EmBlangonga_RajangJumpShot, variant 1); the other kinds just end the
 * state.
 */
void EmBlangonga_TrapEscape(Em *em, EmWork *work)
{
    EmBlangonga_RajangJumpShot(em, work, 0x1);
}

/*
 * Runs the current EM_GROUP_REACT state, passing the shared functions their
 * variant.
 */
void EmBlangonga_RunReactState(Em *em, EmWork *work)
{
    switch (em->state) {
    case BLANG_REACT_FLINCH:
        EmBlangonga_Flinch(em);
        break;
    case BLANG_REACT_FLINCH_B:
        EmBlangonga_FlinchB(em);
        break;
    case BLANG_REACT_KNOCK_DOWN:
        EmBlangonga_KnockDown(em, work, 0);
        break;
    case BLANG_REACT_KNOCK_DOWN_B:
        EmBlangonga_KnockDown(em, work, 1);
        break;
    case BLANG_REACT_STATUS4:
        EmBlangonga_Status4(em);
        break;
    case BLANG_REACT_TRAPPED:
        EmBlangonga_Trapped(em);
        break;
    case BLANG_REACT_TRAPPED_STRUGGLE:
        EmBlangonga_TrappedStruggle(em);
        break;
    case BLANG_REACT_FALL:
        EmBlangonga_Fall(em);
        break;
    case BLANG_REACT_NONE:
        EmBlangonga_ReactNop();
        break;
    case BLANG_REACT_STATUS4_DOWNED:
        EmBlangonga_HeldDowned(em, work, 0);
        break;
    case BLANG_REACT_STATUS4_DOWNED_END:
        EmBlangonga_TrappedStruggle(em);
        break;
    case BLANG_REACT_STATUS2_DOWNED_END:
        EmBlangonga_HeldStatusEnd(em);
        break;
    case BLANG_REACT_STUN:
        EmBlangonga_KnockDown(em, work, 2);
        break;
    case BLANG_REACT_STUN_B:
        EmBlangonga_KnockDown(em, work, 3);
        break;
    case BLANG_REACT_STUN_DOWNED:
        EmBlangonga_HeldDowned(em, work, 1);
        break;
    case BLANG_REACT_STUN_DOWNED_END:
        EmBlangonga_TrappedStruggle(em);
        break;
    case BLANG_REACT_HOLD2:
        EmBlangonga_Hold2(em);
        break;
    case BLANG_REACT_DROP_CARRIED:
        EmBlangonga_DropCarried(em, work, 0);
        break;
    case BLANG_REACT_FLINCH_ATTACK:
        EmBlangonga_FlinchAttack(em);
        break;
    case BLANG_REACT_STATUS1_DOWNED_END:
        EmBlangonga_HeldStatusEnd(em);
        break;
    case BLANG_REACT_PART_BREAK:
        EmBlangonga_FlinchB(em);
        break;
    case BLANG_REACT_DROP_CARRIED_A:
        EmBlangonga_DropCarried(em, work, 1);
        break;
    case BLANG_REACT_DROP_CARRIED_B:
        EmBlangonga_DropCarried(em, work, 2);
        break;
    case BLANG_REACT_DROP_CARRIED_C:
        EmBlangonga_DropCarried(em, work, 3);
        break;
    case BLANG_REACT_TRAP_ESCAPE:
        EmBlangonga_TrapEscape(em, work);
        break;
    }
}

/*
 * BLANG_DEFEAT_DIE: hunted (QuestEm_Hunted, the rage ended): falls dead and
 * leaves a carcass mine point (the large kinds for 1800 frames once the
 * quest lets them appear again, the small ones for 1350 frames or until
 * carved), fades out over 60 frames, then BLANG_DEFEAT_REAPPEAR.
 */
void EmBlangonga_Die(Em *em, s32 unused)
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
        Em_SetMotion(em, BLANG_MOT_DIE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_DIE_END, 0, 0);
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
            EmBlangonga_ChangeState(em, EM_GROUP_DEFEAT, BLANG_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * BLANG_DEFEAT_DIE_HELD: as EmBlangonga_Die, lying in the hold (holdTimer
 * kept at 5).
 */
void EmBlangonga_DieHeld(Em *em, s32 unused)
{
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, BLANG_MOT_DOWNED, 0, 0);
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_DEAD;
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
            EmBlangonga_ChangeState(em, EM_GROUP_DEFEAT, BLANG_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * BLANG_DEFEAT_DIE_ARMORED: as EmBlangonga_Die for a monster under
 * stateArmor: comes up (BLANG_MOT_EMERGE to frame 14) and lies down.
 */
void EmBlangonga_DieArmored(Em *em, s32 unused)
{
    f32 alpha;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->stateArmor = 1;
        em->noPushTimer = 5;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, BLANG_MOT_EMERGE, 0, 0);
        QuestEm_Hunted(em);
        Em_EndRage(em);
        em->noReact = 1;
        break;
    case 1:
        if (em->motion[0].blend != 0) {
            em->stateArmor = 1;
            em->noPushTimer = 5;
        }
        if (EmMotion_PassedFrameScaled(em, 14.0f, 0U) != 0) {
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, BLANG_MOT_DOWNED, 0, 0);
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
            EmBlangonga_ChangeState(em, EM_GROUP_DEFEAT, BLANG_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * BLANG_DEFEAT_DIE_FALLING: as EmBlangonga_Die for a monster in the air:
 * falls on curve 0 and lands dead within 180 of the floor.
 */
void EmBlangonga_DieFalling(Em *em, s32 unused)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, BLANG_MOT_FALL, 0, 0);
        QuestEm_Hunted(em);
        Em_EndRage(em);
        EmBlangonga_StartCurves(em, 0);
        break;
    case 1:
        EmBlangonga_UpdateCurves(em);
        if (em->posY <= 180.0f + em->floorY) {
            em->step++;
            em->posY = em->floorY;
            em->stance = EM_STANCE_DEAD;
            Em_SetMotion(em, BLANG_MOT_FALL_LAND, 0, 0);
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
            EmBlangonga_ChangeState(em, EM_GROUP_DEFEAT, BLANG_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * BLANG_DEFEAT_REAPPEAR: when the quest lets the monster appear again, a
 * Blango of spawnParam 1 hides under the snow again, the other small kinds
 * go to BLANG_IDLE and the large ones drop in from above
 * (BLANG_LEAP_DROP_IN); otherwise the monster's task is finished
 * (taskPhase). EmBlangonga_Start tests Em.variant for the same hidden
 * start: QuestEm_Spawn copies the two from different fields of the quest's
 * monster entry, so the two tests are the game's own.
 */
void EmBlangonga_Reappear(Em *em, s32 unused)
{
    switch (em->step) {
    case 0:
        if (QuestEm_CanAppear(em) == 1) {
            if (EmBlangonga_IsSmallKind(em) == 1) {
                if ((em->type == EM_KIND_BLANGO) && (em->spawnParam == 1)) {
                    em->stance = EM_STANCE_GROUND;
                    Em_SetMotionRow(em, BLANG_MOT_BURIED, 0, 0);
                    EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_BURIED, EM_SYNC_LOCAL);
                    break;
                }
                EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE, EM_SYNC_LOCAL);
                break;
            }
            em->posY = gAreaGroundY[em->area];
            em->stance = EM_STANCE_AIR;
            EmBlangonga_ChangeState(em, EM_GROUP_FLY, BLANG_LEAP_DROP_IN, EM_SYNC_LOCAL);
            break;
        }
        em->taskPhase++;
        break;
    }
}

/*
 * Runs the current EM_GROUP_DEFEAT state, unpushable (and the small kinds
 * unhittable) meanwhile.
 */
void EmBlangonga_RunDefeatState(Em *em, s32 work)
{
    if (EmBlangonga_IsSmallKind(em) == 1) {
        em->noHitTimer = 0xA;
    }
    em->noPushTimer = 0xA;
    switch (em->state) {
    case BLANG_DEFEAT_DIE:
        EmBlangonga_Die(em, work);
        break;
    case BLANG_DEFEAT_DIE_HELD:
        EmBlangonga_DieHeld(em, work);
        break;
    case BLANG_DEFEAT_DIE_ARMORED:
        EmBlangonga_DieArmored(em, work);
        break;
    case BLANG_DEFEAT_DIE_FALLING:
        EmBlangonga_DieFalling(em, work);
        break;
    case BLANG_DEFEAT_REAPPEAR:
        EmBlangonga_Reappear(em, work);
        break;
    }
}

/*
 * BLANG_EVENT_CAPTURED: captured in holdTimer's hold (Em_SetCaptured 0,
 * status 1, poison ended): lies downed (Blangonga first comes up out of the
 * pit), QuestEm_Captured, a sound every 135 frames and the periodic effect;
 * 1800 frames once the quest lets it appear again, then a 150-frame fade
 * and BLANG_DEFEAT_REAPPEAR.
 */
void EmBlangonga_Captured(Em *em)
{
    s32 timer;
    s32 prevTimer;
    u16 motion;

    Em_SetCaptured(em, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        if ((em->type == EM_KIND_BLANGONGA) && (motion = em->motionId[0], (motion != EM_MOTION_ID(0, BLANG_MOT_EMERGE))) && (motion != EM_MOTION_ID(0, BLANG_MOT_STRUGGLE)) && (motion != EM_MOTION_ID(0, BLANG_MOT_DOWNED))) {
            Em_SetMotion(em, BLANG_MOT_EMERGE, 0, 0);
            em->stateArmor = 1;
            em->noPushTimer = 5;
        } else {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_DOWNED, 0, 0);
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
            Em_SetMotion(em, BLANG_MOT_DOWNED, 0, 0);
            QuestEm_Captured(em);
            em->timer = 0x708;
        }
        break;
    case 2:
        if (((s32) em->timer % 135) == 0) {
            Em_PlaySeAtBone(em, 0x52, 0x13);
        }
        if (em->type == EM_KIND_CONGALALA) {
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
        prevTimer = em->timer;
        if (prevTimer > 0) {
            em->timer = prevTimer - 1;
        }
        timer = em->timer;
        if (timer <= 0) {
            if (em->remote == 0) {
                EmBlangonga_ChangeState(em, EM_GROUP_DEFEAT, BLANG_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
                break;
            }
        } else {
            em->alpha = (f32) timer / 150.0f;
        }
        break;
    }
}

/*
 * BLANG_EVENT_CAPTURED_HOLD2: as EmBlangonga_Captured in the second hold
 * (Em_SetCaptured 1), collapsing and lying instead, without the sound.
 */
void EmBlangonga_CapturedHold2(Em *em)
{
    s32 timer;
    s32 prevTimer;

    Em_SetCaptured(em, 1);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_COLLAPSE, 0, 0);
        Em_BeginStatus1(em);
        Em_ClearPoison(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_LIE, 0, 0);
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
        prevTimer = em->timer;
        if (prevTimer > 0) {
            em->timer = prevTimer - 1;
        }
        timer = em->timer;
        if (timer <= 0) {
            if (em->remote == 0) {
                EmBlangonga_ChangeState(em, EM_GROUP_DEFEAT, BLANG_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
                break;
            }
        } else {
            em->alpha = (f32) timer / 150.0f;
        }
        break;
    }
}

/*
 * BLANG_EVENT_CONGALALA_SCENE: Congalala's story scene at a fixed point:
 * idles 180 frames, eats twice and roars, then idles until event flag 0x9B
 * is set; senseDelay and noHitTimer are kept up meanwhile.
 */
void EmBlangonga_CongalalaScene(Em *em, EmWork *work)
{
    EmBlangongaWork *w = (EmBlangongaWork *)work;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        GmEm_ResetSize(em);
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->posX = 12212.0f;
        em->posY = 0.0f;
        em->posZ = 9484.0f;
        em->rotX = 0;
        em->rotY = 0x4000;
        em->rotZ = 0;
        Em_SetMotion(em, BLANG_MOT_IDLE, 0, 0);
        em->timer = 180;
        w->sceneEating = 0;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_EAT, 0, 0);
        }
        break;
    case 2:
        if (EmMotion_InFrameRange(em, 0, 4.0f, 26.0f)) {
            em->rotY += (u32) (1638.4f * em->motionSpeed);
        }
        if (w->sceneEating == 0 && EmMotion_ReachedFrame(em, 0, 34.0f)) {
            w->sceneEating = 1;
        }
        if (EmMotion_PassedFrameScaled(em, 300.0f, 0)) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_EAT, 0, 0x96);
        }
        break;
    case 3:
        if (EmMotion_PassedFrameScaled(em, 250.0f, 0)) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_BACK_STRIKE, 0, 0);
            w->sceneEating = 0;
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_ROAR, 0, 0);
        }
        break;
    case 5:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_IDLE, 0, 0);
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

/*
 * BLANG_EVENT_BLANGONGA_SCENE: hands each monster its part of Blangonga's
 * story scene: Blangonga leaps down (BLANG_EVENT_BLANGONGA_LEAP_DOWN), the
 * first three Blangos in gEms emerge (_EMERGE_A..C) and the rest stay
 * hidden.
 */
void EmBlangonga_StartBlangongaScene(Em *em, EmWork *work)
{
    s16 i;
    u8 n = 0;
    Em *other;

    if (em->type == EM_KIND_BLANGONGA) {
        EmBlangonga_ChangeState(em, EM_GROUP_EVENT, BLANG_EVENT_BLANGONGA_LEAP_DOWN, EM_SYNC_LOCAL);
        return;
    }
    for (i = 0, other = gEms; i < 20; i++, other++) {
        if (other->type == 0x3D) {
            if (em->slotNo == other->slotNo) {
                switch (n) {
                case 0:
                    EmBlangonga_ChangeState(em, EM_GROUP_EVENT, BLANG_EVENT_BLANGO_EMERGE_A, EM_SYNC_LOCAL);
                    return;
                case 1:
                    EmBlangonga_ChangeState(em, EM_GROUP_EVENT, BLANG_EVENT_BLANGO_EMERGE_B, EM_SYNC_LOCAL);
                    return;
                case 2:
                    EmBlangonga_ChangeState(em, EM_GROUP_EVENT, BLANG_EVENT_BLANGO_EMERGE_C, EM_SYNC_LOCAL);
                    return;
                default:
                    EmBlangonga_ChangeState(em, EM_GROUP_EVENT, BLANG_EVENT_BLANGO_HIDDEN, EM_SYNC_LOCAL);
                    return;
                }
            }
            n++;
        }
    }
}

/*
 * BLANG_EVENT_BLANGONGA_LEAP_DOWN: waits on a high point, roars, jumps down
 * and lands on the floor (frame 70 of the fall), walks to its goal (1200
 * frames or within 100), then idles until event flag 0xA4.
 */
void EmBlangonga_BlangongaLeapDown(Em *em, EmWork *work)
{
    f32 goalDist;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        GmEm_ResetSize(em);
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->posX = 10747.0f;
        em->posY = 2468.0f;
        em->posZ = 8181.0f;
        Vec_Copy(&em->prevPosX, &em->posX);
        em->rotX = 0;
        em->rotY = 0xFC00;
        em->rotZ = 0;
        Em_SetMotion(em, BLANG_MOT_IDLE, 0, 0);
        em->timer = 300;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_ROAR_B, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 190;
            Em_SetMotion(em, BLANG_MOT_IDLE, 0, 0);
        }
        break;
    case 3:
        if (--em->timer <= 0) {
            em->step++;
            em->stance = EM_STANCE_AIR;
            Em_SetMotion(em, BLANG_MOT_MOVE_ACTION_D, 0, 0);
        }
        break;
    case 4:
        if (EmMotion_PassedFrameScaled(em, 26.0f, 0)) {
            em->step++;
            Player_ClearVelocity(em);
            Em_SetMotion(em, BLANG_MOT_JUMP_FALL, 0, 0x32);
        }
        break;
    case 5:
        if (EmMotion_PassedFrameScaled(em, 70.0f, 0)) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posX = 10682.0f;
            em->posY = 288.0f;
            em->posZ = 9620.0f;
            em->goal[0] = 10682.0f;
            em->goal[1] = 288.0f;
            em->goal[2] = 12400.0f;
            em->rotX = 0;
            em->rotY = Math_AngleXZ(&em->posX, em->goal);
            em->rotZ = 0;
            Em_SetMotion(em, BLANG_MOT_JUMP_LAND, 0, 0);
        }
        break;
    case 6:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 1200;
            Em_SetMotion(em, BLANG_MOT_WALK, 0, 0);
        }
        break;
    case 7:
        goalDist = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer <= 0 || goalDist <= 100.0f) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_IDLE, 0, 0);
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

/*
 * BLANG_EVENT_BLANGO_EMERGE_A..C (variants 0-2): waits buried (stateArmor)
 * at the variant's place and facing for 440, 480 or 510 frames, comes out,
 * then idles and roars in turn until event flag 0xA4.
 */
void EmBlangonga_BlangoEmerge(Em *em, EmWork *work, u8 variant)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        GmEm_ResetSize(em);
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->stateArmor = 1;
        Em_SetMotion(em, BLANG_MOT_BURIED, 0, 0);
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
            Em_SetMotion(em, BLANG_MOT_EMERGE, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_ALERT_IDLE, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stateVar1 = 0;
            Em_SetMotion(em, BLANG_MOT_ROAR, 0, 0);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            if (em->stateVar1 == 0) {
                em->stateVar1 = 1;
                Em_SetMotion(em, BLANG_MOT_ALERT_IDLE, 0, 0);
            } else {
                em->stateVar1 = 0;
                Em_SetMotion(em, BLANG_MOT_ROAR, 0, 0);
            }
        }
        if (Save_TestEventFlag(0xA4) == 1) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_EVENT_BLANGO_HIDDEN: stays invisible until event flag 0xA4, then
 * shows itself and ends the state.
 */
void EmBlangonga_BlangoHidden(Em *em)
{
    switch (em->step) {
    case 0:
        GmEm_ResetSize(em);
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_IDLE, 0, 0);
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

/* Runs the current EM_GROUP_EVENT (or EVENT2) state. */
void EmBlangonga_RunEventState(Em *em, EmWork *work)
{
    switch (em->state) {
    case BLANG_EVENT_CAPTURED:
        EmBlangonga_Captured(em);
        break;
    case BLANG_EVENT_CAPTURED_HOLD2:
        EmBlangonga_CapturedHold2(em);
        break;
    case BLANG_EVENT_CONGALALA_SCENE:
        EmBlangonga_CongalalaScene(em, work);
        break;
    case BLANG_EVENT_BLANGONGA_SCENE:
        EmBlangonga_StartBlangongaScene(em, work);
        break;
    case BLANG_EVENT_BLANGONGA_LEAP_DOWN:
        EmBlangonga_BlangongaLeapDown(em, work);
        break;
    case BLANG_EVENT_BLANGO_EMERGE_A:
        EmBlangonga_BlangoEmerge(em, work, 0);
        break;
    case BLANG_EVENT_BLANGO_EMERGE_B:
        EmBlangonga_BlangoEmerge(em, work, 1);
        break;
    case BLANG_EVENT_BLANGO_EMERGE_C:
        EmBlangonga_BlangoEmerge(em, work, 2);
        break;
    case BLANG_EVENT_BLANGO_HIDDEN:
        EmBlangonga_BlangoHidden(em);
        break;
    }
}

/*
 * BLANG_VS_EAT_BAIT: after the defeat camera (Versus_DefeatCamera) the bait
 * comes from the winner's side (stage object 0x25) and the monster eats it
 * (Congalala with its own motion), then Versus_RouteEmByTeam.
 */
void EmBlangonga_VersusEatBait(Em *em, EmWork *work)
{
    EmBlangongaVec throwPoint;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        if (Versus_DefeatCamera(em) == 1) {
            throwPoint = EmBlangonga_CarveThrowPt;
            StageObj_K25_Create(&gPlayers[(u8) em->side - 1], em, 0, 0, &throwPoint.x);
            em->step++;
            if (em->type == EM_KIND_CONGALALA) {
                em->step++;
                Em_SetMotion(em, BLANG_MOT_CONGALALA_EAT, 0, 0);
            } else {
                Em_SetMotion(em, BLANG_MOT_EAT, 0, 0);
            }
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_EAT_END, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            Versus_RouteEmByTeam(em);
        }
        break;
    }
}

/*
 * BLANG_VS_POSE_A: one pose by kind (Blangonga, Congalala, the others),
 * then idles and BLANG_VS_WAIT_BOTH.
 */
void EmBlangonga_VersusPoseA(Em *em, EmWork *work)
{
    u8 blangongaStep;
    u8 congalalaStep;
    u8 otherStep;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->type) {
    case EM_KIND_BLANGONGA:
        blangongaStep = em->step;
        switch (blangongaStep) {
        case 0:
            em->step = blangongaStep + 1;
            Em_SetMotion(em, BLANG_MOT_IDLE_ACTION_C, 0, 0);
            break;
        case 1:
            if (em->motion[0].playing == 0) {
                em->step = blangongaStep + 1;
                Em_SetMotion(em, BLANG_MOT_IDLE, 0, 0);
                EmBlangonga_ChangeState(em, EM_GROUP_VERSUS, BLANG_VS_WAIT_BOTH, EM_SYNC_SEND);
            }
            break;
        }
        break;
    case 0x34:
        congalalaStep = em->step;
        switch (congalalaStep) {
        case 0:
            em->step = congalalaStep + 1;
            Em_SetMotion(em, BLANG_MOT_FLINCH_B, 0, 0);
            break;
        case 1:
            if (em->motion[0].playing == 0) {
                em->step = congalalaStep + 1;
                Em_SetMotion(em, BLANG_MOT_IDLE, 0, 0);
                EmBlangonga_ChangeState(em, EM_GROUP_VERSUS, BLANG_VS_WAIT_BOTH, EM_SYNC_SEND);
            }
            break;
        }
        break;
    default:
        otherStep = em->step;
        switch (otherStep) {
        case 0:
            em->step = otherStep + 1;
            Em_SetMotion(em, BLANG_MOT_FLINCH, 0, 0);
            break;
        case 1:
            if (em->motion[0].playing == 0) {
                em->step = otherStep + 1;
                Em_SetMotion(em, BLANG_MOT_IDLE, 0, 0);
                EmBlangonga_ChangeState(em, EM_GROUP_VERSUS, BLANG_VS_WAIT_BOTH, EM_SYNC_SEND);
            }
        }
        break;
    }
}

/* BLANG_VS_POSE_B: as BLANG_VS_POSE_A with the second poses. */
void EmBlangonga_VersusPoseB(Em *em, EmWork *work)
{
    u8 blangongaStep;
    u8 otherStep;
    u8 congalalaStep;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->type) {
    case EM_KIND_BLANGONGA:
        blangongaStep = em->step;
        switch (blangongaStep) {
        case 0:
            em->step = blangongaStep + 1;
            Em_SetMotion(em, BLANG_MOT_ROAR_B, 0, 0);
            break;
        case 1:
            if (em->motion[0].playing == 0) {
                em->step = blangongaStep + 1;
                Em_SetMotion(em, BLANG_MOT_IDLE, 0, 0);
                EmBlangonga_ChangeState(em, EM_GROUP_VERSUS, BLANG_VS_WAIT_BOTH, EM_SYNC_SEND);
            }
            break;
        }
        break;
    case 0x34:
        congalalaStep = em->step;
        switch (congalalaStep) {
        case 0:
            em->step = congalalaStep + 1;
            Em_SetMotion(em, BLANG_MOT_ROAR, 0, 0);
            break;
        case 1:
            if (em->motion[0].playing == 0) {
                em->step = congalalaStep + 1;
                Em_SetMotion(em, BLANG_MOT_IDLE, 0, 0);
                EmBlangonga_ChangeState(em, EM_GROUP_VERSUS, BLANG_VS_WAIT_BOTH, EM_SYNC_SEND);
            }
            break;
        }
        break;
    default:
        otherStep = em->step;
        switch (otherStep) {
        case 0:
            em->step = otherStep + 1;
            Em_SetMotion(em, BLANG_MOT_ROAR_B, 0, 0);
            break;
        case 1:
            if (em->motion[0].playing == 0) {
                em->step = otherStep + 1;
                Em_SetMotion(em, BLANG_MOT_IDLE, 0, 0);
                EmBlangonga_ChangeState(em, EM_GROUP_VERSUS, BLANG_VS_WAIT_BOTH, EM_SYNC_SEND);
            }
        }
        break;
    }
}

/*
 * BLANG_VS_POSE_C: as BLANG_VS_POSE_A with the third poses (Congalala's in
 * two motions).
 */
void EmBlangonga_VersusPoseC(Em *em, EmWork *work)
{
    u8 congalalaStep;
    u8 blangongaStep;
    u8 otherStep;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->type) {
    case EM_KIND_BLANGONGA:
        blangongaStep = em->step;
        switch (blangongaStep) {
        case 0:
            em->step = blangongaStep + 1;
            Em_SetMotion(em, BLANG_MOT_ROAR, 0, 0);
            break;
        case 1:
            if (em->motion[0].playing == 0) {
                em->step = blangongaStep + 1;
                Em_SetMotion(em, BLANG_MOT_IDLE, 0, 0);
                EmBlangonga_ChangeState(em, EM_GROUP_VERSUS, BLANG_VS_WAIT_BOTH, EM_SYNC_SEND);
            }
            break;
        }
        break;
    case 0x34:
        congalalaStep = em->step;
        switch (congalalaStep) {
        case 0:
            em->step = congalalaStep + 1;
            Em_SetMotion(em, BLANG_MOT_CONGALALA_HITZONE_COMBO, 0, 0);
            break;
        case 1:
            if (em->motion[0].playing == 0) {
                em->step = congalalaStep + 1;
                Em_SetMotion(em, BLANG_MOT_CONGALALA_HITZONE_COMBO_END, 0, 0);
            }
            break;
        case 2:
            if (em->motion[0].playing == 0) {
                em->step = congalalaStep + 1;
                Em_SetMotion(em, BLANG_MOT_IDLE, 0, 0);
                EmBlangonga_ChangeState(em, EM_GROUP_VERSUS, BLANG_VS_WAIT_BOTH, EM_SYNC_SEND);
            }
            break;
        }
        break;
    default:
        otherStep = em->step;
        switch (otherStep) {
        case 0:
            em->step = otherStep + 1;
            Em_SetMotion(em, BLANG_MOT_ROAR, 0, 0);
            break;
        case 1:
            if (em->motion[0].playing == 0) {
                em->step = otherStep + 1;
                Em_SetMotion(em, BLANG_MOT_IDLE, 0, 0);
                EmBlangonga_ChangeState(em, EM_GROUP_VERSUS, BLANG_VS_WAIT_BOTH, EM_SYNC_SEND);
            }
        }
        break;
    }
}

/*
 * BLANG_VS_WAIT_BOTH: idles until both sides are defeated
 * (Versus_BothDefeated), then BLANG_VS_END. Step 0 falls into step 1 to
 * test on the same frame.
 */
void EmBlangonga_VersusWaitBoth(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        if (em->motionId[0] != EM_MOTION_ID(0, BLANG_MOT_IDLE)) {
            Em_SetMotion(em, BLANG_MOT_IDLE, 0, 0);
        }
        
    case 1:
        if (Versus_BothDefeated() == 1) {
            em->step++;
            EmBlangonga_ChangeState(em, EM_GROUP_VERSUS, BLANG_VS_END, EM_SYNC_SEND);
        }
        break;
    }
}

/*
 * BLANG_VS_END: idles 90 frames, lets the monster be sensed and hit again,
 * and ends the state.
 */
void EmBlangonga_VersusEnd(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 0x5A;
        if (em->motionId[0] != EM_MOTION_ID(0, BLANG_MOT_IDLE)) {
            Em_SetMotion(em, BLANG_MOT_IDLE, 0, 0);
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

/* Runs the current EM_GROUP_VERSUS state. */
void EmBlangonga_RunVersusState(Em *em, EmWork *work)
{
    switch (em->state) {
    case BLANG_VS_EAT_BAIT:
        EmBlangonga_VersusEatBait(em, work);
        break;
    case BLANG_VS_POSE_A:
        EmBlangonga_VersusPoseA(em, work);
        break;
    case BLANG_VS_POSE_B:
        EmBlangonga_VersusPoseB(em, work);
        break;
    case BLANG_VS_POSE_C:
        EmBlangonga_VersusPoseC(em, work);
        break;
    case BLANG_VS_WAIT_BOTH:
        EmBlangonga_VersusWaitBoth(em, work);
        break;
    case BLANG_VS_END:
        EmBlangonga_VersusEnd(em, work);
        break;
    }
}

/*
 * The family's frame: the rage timer, visibility and combat timer; the
 * Blango's sensing under the snow (EM_TRIGGER_SENSED); the needs' decay by
 * map and kind; then Em_UpdateDamage's result picks a reaction
 * (EM_GROUP_REACT), a death or a capture, or a status state; Congalala's
 * and Blangonga's story quests start their scenes; then the state runs
 * (twice when stateRequest is set), and stateArmor keeps the shadow hidden
 * and the guard up.
 */
void EmBlangonga_Update(Em *em)
{
    EmBlangongaWork *w = (EmBlangongaWork *)&em->work;
    u8 savedSenseDelay;
    s16 partStateSum;
    s32 result;
    u8 mapBase;
    s32 sensed;
    s32 i;
    u8 damageOut[4];

    EmBlangonga_UpdateRageTimer(em);
    if (em->stateGroup <= EM_GROUP_REACT) {
        em->alpha = 1.0f;
        em->visible = 1;
    }
    if (em->senseDelay != 0) {
        em->noHitTimer = 5;
    }
    if (w->hitSeCooldown > 0) {
        w->hitSeCooldown--;
    }
    Em_UpdateCombatTimer(em);
    if (EmBlangonga_IsSmallKind(em) == 0) {
        Em_RaisePlayerAreaTrigger(em);
    }
    if ((em->stateGroup != EM_GROUP_FLY || em->state != BLANG_LEAP_OUT_OF_AREA) && (em->stateGroup != EM_GROUP_FLY || em->state != BLANG_LEAP_DROP_IN) &&
        (em->stateGroup != EM_GROUP_FLY || em->state != BLANG_LEAP_CROSS_AREA)) {
        Em_HoldPosNoPlayers(em, 0, 1);
    }
    if (EmBlangonga_ShouldEndState(em) == 1) {
        EmBlangonga_EndState(em, 0, 0);
    }
    if ((s16) Player_IsState(em, EM_GROUP_IDLE, BLANG_IDLE_BURIED)) {
        savedSenseDelay = em->senseDelay;
        em->senseDelay = 0;
    }
    if (em->type == EM_KIND_BLANGO) {
        sensed = Em_SenseObjects(em, &w->sense) & 0xFF;
        if (em->senseDelay == 0 && em->noReact == 0 && sensed != 0 && Em_ScriptFlagsFrom(em, 4) == 0) {
            em->triggers |= EM_TRIGGER_SENSED;
            em->scriptPending = 1;
        } else if ((s16) Player_IsState(em, EM_GROUP_IDLE, BLANG_IDLE_BURIED)) {
            em->senseDelay = savedSenseDelay;
        }
    }
    if (em->noAlert != 1 && em->rage == 0) {
        mapBase = Map_DayId(gScene.map);
        switch (em->type) {
        case EM_KIND_BLANGONGA:
            switch (mapBase) {
            case MAP_SNOWY_MOUNTAINS:
                Em_DecayNeed1Only(em);
                Em_CheckLowHp(em, 0.3f);
                break;
            }
            break;
        case 0x34:
            switch (mapBase) {
            case MAP_JUNGLE:
                Em_DecayNeed1(em);
                Em_DecayNeed2(em);
                Em_CheckLowHp(em, 0.2f);
                break;
            case MAP_SWAMP:
                Em_DecayNeed1(em);
                Em_CheckLowHp(em, 0.2f);
                break;
            case MAP_GREAT_ARENA:
            case MAP_ARENA:
                Em_DecayNeed1Only(em);
                break;
            }
            break;
        }
    }
    result = Em_UpdateDamage(em, damageOut) & 0xFF;
    switch (result) {
    case EM_DMG_NONE:
    case EM_DMG_FLASH:
    case EM_DMG_HIT_MODE3:
    case EM_DMG_ATTACK5:
    case EM_DMG_ATTACK5_FLINCH:
        break;
    case EM_DMG_KILLED:
    case EM_DMG_DEAD:
        if ((s8) em->holdTimer != 0) {
            EmBlangonga_ChangeState(em, EM_GROUP_DEFEAT, BLANG_DEFEAT_DIE_HELD, EM_SYNC_OWNER);
        } else if (EmBlangonga_InStateArmor(em) == 1) {
            EmBlangonga_ChangeState(em, EM_GROUP_DEFEAT, BLANG_DEFEAT_DIE_ARMORED, EM_SYNC_OWNER);
        } else {
            switch (em->stance) {
            case EM_STANCE_AIR:
                EmBlangonga_ChangeState(em, EM_GROUP_DEFEAT, BLANG_DEFEAT_DIE_FALLING, EM_SYNC_OWNER);
                break;
            default:
                EmBlangonga_ChangeState(em, EM_GROUP_DEFEAT, BLANG_DEFEAT_DIE, EM_SYNC_OWNER);
                break;
            }
        }
        break;
    case EM_DMG_ATTACK9:
    case EM_DMG_ATTACK6:
        if ((s8) em->holdTimer == 0) {
            if (EmBlangonga_IsRajang(em) == 1 && em->rage == 0 && em->status == EM_STATUS_NONE) {
                EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_TRAP_ESCAPE, EM_SYNC_OWNER);
            } else {
                Em_SetTrapTime(em, damageOut);
                Em_SetTrapTimeGoal(em);
                EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_TRAPPED, EM_SYNC_OWNER);
            }
        }
        break;
    case EM_DMG_STATUS_IN_AIR:
        if (em->stateGroup != EM_GROUP_REACT || em->state != BLANG_REACT_FALL) {
            EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_FALL, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_STATUS4:
        if (em->stateGroup != EM_GROUP_REACT || em->state != BLANG_REACT_STATUS4_DOWNED) {
            if ((s8) em->holdTimer != 0) {
                Em_SetStatus4Timer(em);
                EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_STATUS4_DOWNED, EM_SYNC_OWNER);
            } else if ((em->stateGroup != EM_GROUP_REACT || em->state != BLANG_REACT_STATUS4) && (em->stateGroup != EM_GROUP_REACT || em->state != BLANG_REACT_FALL)) {
                Em_SetStatus4Timer(em);
                EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_STATUS4, EM_SYNC_OWNER);
            }
        }
        break;
    case EM_DMG_CAPTURED:
        if (Em_IsHeld(em) == 1) {
            Em_SetStatus1Timer(em);
            if (EmBlangonga_IsBlangonga(em) != 1) {
                EmBlangonga_IsCongalala(em);
            }
            if ((s8) em->holdTimer != 0) {
                EmBlangonga_ChangeState(em, EM_GROUP_EVENT, BLANG_EVENT_CAPTURED, EM_SYNC_OWNER);
                break;
            }
            if ((s8) em->holdTimer2 != 0) {
                EmBlangonga_ChangeState(em, EM_GROUP_EVENT, BLANG_EVENT_CAPTURED_HOLD2, EM_SYNC_OWNER);
                break;
            }
        }
        if ((em->stateGroup != EM_GROUP_IDLE || em->state != BLANG_IDLE_STATUS1_COLLAPSE) && (em->stateGroup != EM_GROUP_REACT || em->state != BLANG_REACT_FALL)) {
            Em_SetStatus1Timer(em);
            EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_STATUS1_COLLAPSE, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_STATUS2:
        if ((s8) em->holdTimer != 0) {
            Em_SetStatus2Timer(em);
            EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_STATUS2_DOWNED, EM_SYNC_OWNER);
        } else if ((em->stateGroup != EM_GROUP_IDLE || em->state != BLANG_IDLE_STATUS2_COLLAPSE) && (em->stateGroup != EM_GROUP_REACT || em->state != BLANG_REACT_FALL)) {
            Em_SetStatus2Timer(em);
            EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_STATUS2_COLLAPSE, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_HIT_LYING:
        Em_RestartScript(em);
        switch (em->state) { /* the lying states that set hitReactMode 2 are all in EM_GROUP_IDLE */
        case BLANG_IDLE_REST:
            EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_REST_END, EM_SYNC_OWNER);
            break;
        case BLANG_IDLE_STATUS2_COLLAPSE:
            EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_STATUS2_END, EM_SYNC_OWNER);
            break;
        case BLANG_IDLE_STATUS1_COLLAPSE:
            EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_STATUS1_END, EM_SYNC_OWNER);
            break;
        case BLANG_IDLE_STATUS2_DOWNED:
            EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_STATUS2_DOWNED_END, EM_SYNC_OWNER);
            break;
        case BLANG_IDLE_STATUS1_DOWNED:
            EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_STATUS1_DOWNED_END, EM_SYNC_OWNER);
            break;
        }
        break;
    case EM_DMG_PART8_DOWN:
        EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_PART_BREAK, EM_SYNC_OWNER);
        break;
    case EM_DMG_PART_FLINCH:
        EmBlangonga_BreakParts(em, 1);
        switch (em->stance) {
        case EM_STANCE_AIR:
            EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_FALL, EM_SYNC_OWNER);
            break;
        default:
            partStateSum = 0;
            if (EmBlangonga_IsCongalala(em) == 1) {
                if (em->type == EM_KIND_CONGALALA) {
                    switch (em->flinchPart) {
                    case 4:
                        if (em->parts[em->flinchPart].state >= 2) {
                            if (EmBlangonga_CarriedShown(em, 1) == 1) {
                                EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_DROP_CARRIED_B, EM_SYNC_OWNER);
                                goto done;
                            }
                            if (EmBlangonga_CarriedShown(em, 2) == 1) {
                                EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_DROP_CARRIED_C, EM_SYNC_OWNER);
                                goto done;
                            }
                            if (EmBlangonga_CarriedShown(em, 0) == 1) {
                                EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_DROP_CARRIED_A, EM_SYNC_OWNER);
                                goto done;
                            }
                        }
                    }
                }
                for (i = 0; i < 8; i++) {
                    partStateSum += em->parts[i].state;
                }
                if (partStateSum <= 3) {
                    EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_FLINCH, EM_SYNC_OWNER);
                    break;
                }
                switch (em->flinchPart) {
                case 0:
                    if (EmBlangonga_IsSmallKind(em) == 0) {
                        EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_FLINCH_ATTACK, EM_SYNC_OWNER);
                    } else {
                        EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_FLINCH_B, EM_SYNC_OWNER);
                    }
                    break;
                case 2:
                    EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_KNOCK_DOWN, EM_SYNC_OWNER);
                    break;
                case 3:
                    EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_KNOCK_DOWN_B, EM_SYNC_OWNER);
                    break;
                case 4:
                default:
                    EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_FLINCH_B, EM_SYNC_OWNER);
                    break;
                }
            } else {
                if (EmBlangonga_IsSmallKind(em) == 0) {
                    for (i = 0; i < 8; i++) {
                        partStateSum += em->parts[i].state;
                    }
                    if (partStateSum <= 3) {
                        EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_FLINCH, EM_SYNC_OWNER);
                        break;
                    }
                }
                switch (em->flinchPart) {
                case 1:
                    EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_KNOCK_DOWN_B, EM_SYNC_OWNER);
                    break;
                case 2:
                    EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_KNOCK_DOWN, EM_SYNC_OWNER);
                    break;
                default:
                    EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_FLINCH_B, EM_SYNC_OWNER);
                    break;
                }
            }
            break;
        }
        break;
    case EM_DMG_HIT:
        if (em->type == EM_KIND_BLANGO) {
            if (em->stance == EM_STANCE_GROUND) {
                em->scriptPending = 0;
                EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_FLINCH, EM_SYNC_OWNER);
            }
        } else if (em->type == EM_KIND_CONGALALA && em->stateGroup == EM_GROUP_ATTACK && em->state == BLANG_ATK_CONGALALA_HITZONE_COMBO && (s8) em->hitzoneSet == 1 &&
                   w->hitSeCooldown == 0) {
            Em_PlaySeChecked(em, 0x1B, 0, &em->posX, 6, 0);
            w->hitSeCooldown = 30;
        }
        break;
    case EM_DMG_ATTACK13:
        Em_ResetKindTimer(em);
        EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_HOLD2, EM_SYNC_OWNER);
        break;
    case EM_DMG_STATUS5:
        if ((em->stateGroup != EM_GROUP_REACT || em->state != BLANG_REACT_FALL) && (em->stateGroup != EM_GROUP_REACT || em->state != BLANG_REACT_STUN_DOWNED)) {
            if ((s8) em->holdTimer != 0) {
                Em_SetStatus5Timer(em);
                EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_STUN_DOWNED, EM_SYNC_OWNER);
            } else if ((em->stateGroup != EM_GROUP_REACT || em->state != BLANG_REACT_STUN) && (em->stateGroup != EM_GROUP_REACT || em->state != BLANG_REACT_STUN_B)) {
                Em_SetStatus5Timer(em);
                if (((em->hitAngle - em->rotY) & 0xFFFF) > 0x8000) {
                    EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_STUN, EM_SYNC_OWNER);
                } else {
                    EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_STUN_B, EM_SYNC_OWNER);
                }
            }
        }
        break;
    }
done: /* the Congalala drops leave the result switch from inside the part switch */
    if (Em_DamageResultIsReaction(em, result) == 1) {
        EmBlangonga_BreakParts(em, 0);
    }
    switch (gQuest.stage) {
    case 0x3F3:
    case 0x3F4:
        if (em->type == EM_KIND_CONGALALA && Area_DayId(em->area) == AREA_JUNGLE_3 && Save_TestEventFlag(0x9B) == 0) {
            if (Area_DayId(gScene.area) == AREA_JUNGLE_3 && gScene.eventRunning == 1 && em->stateGroup != EM_GROUP_EVENT) {
                EmBlangonga_ChangeState(em, EM_GROUP_EVENT, BLANG_EVENT_CONGALALA_SCENE, EM_SYNC_SEND);
            }
            goto skip;
        }
        break;
    case 0x423:
    case 0x424:
        if ((em->type == EM_KIND_BLANGONGA || em->type == EM_KIND_BLANGO) && Area_DayId(em->area) == AREA_SNOWY_MOUNTAINS_6 && Save_TestEventFlag(0xA4) == 0) {
            if (Area_DayId(gScene.area) == AREA_SNOWY_MOUNTAINS_6 && gScene.eventRunning == 1 && em->stateGroup != EM_GROUP_EVENT) {
                EmBlangonga_ChangeState(em, EM_GROUP_EVENT, BLANG_EVENT_BLANGONGA_SCENE, EM_SYNC_SEND);
            }
            goto skip;
        }
        break;
    }
    Em_UpdatePendingScript(em);
skip: /* a story scene keeps the pending script from starting */
    Em_ClearHitReactMode(em);
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

/*
 * Runs the current state through its group's dispatcher (EM_GROUP_EVENT2
 * uses EM_GROUP_EVENT's) after keeping the group and state as the previous
 * ones.
 */
void EmBlangonga_RunState(Em *em, s32 work)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->stateGroup) {
    case EM_GROUP_IDLE:
        EmBlangonga_RunIdleState(em, (EmWork *) work);
        break;
    case EM_GROUP_MOVE:
        EmBlangonga_RunMoveState(em, work);
        break;
    case EM_GROUP_FLY:
        EmBlangonga_RunLeapState(em, (EmWork *) work);
        break;
    case EM_GROUP_ATTACK:
        EmBlangonga_RunAttackState(em, (EmWork *) work);
        break;
    case EM_GROUP_REACT:
        EmBlangonga_RunReactState(em, (EmWork *) work);
        break;
    case EM_GROUP_DEFEAT:
        EmBlangonga_RunDefeatState(em, work);
        break;
    case EM_GROUP_EVENT:
        EmBlangonga_RunEventState(em, (EmWork *) work);
        break;
    case EM_GROUP_EVENT2:
        EmBlangonga_RunEventState(em, (EmWork *) work);
        break;
    case EM_GROUP_VERSUS:
        EmBlangonga_RunVersusState(em, (EmWork *) work);
        
    default:
        break;
    }
}

/*
 * Blangonga: a flinch of the damaged part 4 whose flags have bit 2 flags
 * part 2's break (Em_FlagPartBreak); then, when `apply`, applies the part
 * breaks (Em_ApplyPartBreaks).
 */
void EmBlangonga_BreakParts(Em *em, s32 apply)
{
    u8 k;

    if (em->type == EM_KIND_BLANGONGA) {
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

/*
 * In the player's area, creates the ground effect (Effect0D) of `kind` at
 * bone `bone` dropped to the floor, scaled by the monster's size.
 */
void EmBlangonga_SpawnGroundEffect(Em *em, s32 bone, s32 kind, f32 scale)
{
    f32 pos[3];

    if (Player_InCurrentArea(em)) {
        Em_BonePos(em, bone, pos);
        pos[1] = em->floorY;
        Effect0D_SpawnOnGround(pos, kind, scale * em->scaleX);
    }
}

/*
 * While enraged, in the motions of its list: Rajang creates stage object
 * 0x27 every 16 frames, the others stage object 7 every 45 frames
 * (staggered by the monster's slot).
 */
void EmBlangonga_SpawnK1CInMotions(Em *em, s16 motion)
{
    switch (motion) {
    case EM_MOTION_ID(0, BLANG_MOT_IDLE):
    case EM_MOTION_ID(0, BLANG_MOT_WALK):
    case EM_MOTION_ID(0, BLANG_MOT_TURN_LEFT):
    case EM_MOTION_ID(0, BLANG_MOT_TURN_RIGHT):
    case EM_MOTION_ID(0, BLANG_MOT_TURN_LEFT_QUICK):
    case EM_MOTION_ID(0, BLANG_MOT_TURN_RIGHT_QUICK):
    case EM_MOTION_ID(0, BLANG_MOT_LIMP):
    case EM_MOTION_ID(0, BLANG_MOT_SLAM_WINDUP):
    case EM_MOTION_ID(0, BLANG_MOT_STRIKE_A):
    case EM_MOTION_ID(0, BLANG_MOT_STRIKE_B):
    case EM_MOTION_ID(0, BLANG_MOT_ALERT_IDLE):
    case EM_MOTION_ID(0, BLANG_MOT_ROAR):
    case EM_MOTION_ID(0, BLANG_MOT_TURN_ABOUT):
    case EM_MOTION_ID(0, BLANG_MOT_TURN_ABOUT_B):
    case EM_MOTION_ID(0, BLANG_MOT_SHAKE_OFF):
    case EM_MOTION_ID(0, BLANG_MOT_IDLE_ACTION_B_END):
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

/*
 * Shakes the camera (CamShake_ReqAIf level `level`) when the layer-0 motion
 * passes `frame`.
 */
void EmBlangonga_CamShakeOnFrame(s32 em, s32 frame, s32 i)
{
    if (EmMotion_PassedFrameScaled((Em *) em, (f32) frame, 0U) != 0) {
        CamShake_ReqAIf((void *) em, i);
    }
}

/* Does nothing. */
void EmBlangonga_MotionEffectsDefault(Em *em)
{
}

void StageObj_K14_Create(Em *em, s16 k, s16 c, f32 scale);
void StageObj_K1C_CreateAt(f32 *src, u8 k, f32 f);
void Vec_RotateY(f32 *v, f32 angle);
void EmBlangonga_MotionSounds(Em *em, EmWork *work);
void EmBlangonga_MotionSoundsCongalala(Em *em, EmWork *work);
void EmBlangonga_MotionSoundsRajang(Em *em, EmWork *work);

/*
 * The motions' effects: keeps the layer-0 motion id (EmWork.prevMotion),
 * plays the kind's sounds (EmBlangonga_MotionSounds, ...Congalala,
 * ...Rajang in em_rathalos.c) and the rage's stage objects, then per motion
 * and frame the hits (BlangongaAttack), ground effects, stage objects and
 * camera shakes.
 */
void EmBlangonga_MotionEffects(Em *em, EmWork *work)
{
    f32 ofs[3];
    f32 pos[3];

    if (em->motionId[0] != work->prevMotion) {
        work->prevMotion = em->motionId[0];
    }
    switch (em->type) {
    case EM_KIND_BLANGONGA:
    default:
    case EM_KIND_BLANGO:
        EmBlangonga_MotionSounds(em, work);
        break;
    case EM_KIND_CONGALALA:
    case EM_KIND_CONGA:
        EmBlangonga_MotionSoundsCongalala(em, work);
        break;
    case EM_KIND_RAJANG:
        EmBlangonga_MotionSoundsRajang(em, work);
        break;
    }
    EmBlangonga_SpawnK1CInMotions(em, work->prevMotion);
    switch (work->prevMotion) {
    case EM_MOTION_ID(0, BLANG_MOT_TURN_LEFT_QUICK):
        if ((EmBlangonga_IsSmallKind(em) == 0) && (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0)) {
            BlangongaAttack_Create(em, 0xE);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TURN_RIGHT_QUICK):
        if ((EmBlangonga_IsSmallKind(em) == 0) && (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0)) {
            BlangongaAttack_Create(em, 0x10);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_MOVE_ACTION_D):
        if ((EmBlangonga_IsSmallKind(em) == 0) && (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0)) {
            BlangongaAttack_Create(em, 0xB);
        }
        if (EmMotion_PassedFrameScaled(em, 14.0f, 0) != 0) {
            StageObj_K14_Create((Em *) em, 2, 0xF, em->scaleX);
        }
        break;
    case EM_MOTION_ID(0, 0xB):
        if (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0) {
            EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_MOVE_ACTION_A):
        if ((EmBlangonga_IsSmallKind(em) == 0) && (EmMotion_PassedFrameScaled(em, 18.0f, 0) != 0)) {
            BlangongaAttack_Create(em, 0xC);
        }
        if (EmMotion_PassedFrameScaled(em, 14.0f, 0) != 0) {
            StageObj_K14_Create((Em *) em, 2, 0xF, em->scaleX);
        }
        break;
    case EM_MOTION_ID(0, 0xD):
        if (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0) {
            EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_MOVE_ACTION_B):
        if ((EmBlangonga_IsSmallKind(em) == 0) && (EmMotion_PassedFrameScaled(em, 18.0f, 0) != 0)) {
            BlangongaAttack_Create(em, 0xD);
        }
        if (EmMotion_PassedFrameScaled(em, 14.0f, 0) != 0) {
            StageObj_K14_Create((Em *) em, 3, 0x10, em->scaleX);
        }
        break;
    case EM_MOTION_ID(0, 0xF):
        if (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0) {
            EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_MOVE_ACTION_C):
        if (EmMotion_PassedFrameScaled(em, 18.0f, 0) != 0) {
            StageObj_K14_Create((Em *) em, 2, 0x11, em->scaleX);
        }
        if (EmMotion_PassedFrameScaled(em, 22.0f, 0) != 0) {
            StageObj_K14_Create((Em *) em, 3, 0x12, em->scaleX);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_JUMP):
        if (EmBlangonga_IsRajang(em) == 1) {
            if (EmMotion_PassedFrameScaled(em, 48.0f, 0) != 0) {
                StageObj_K14_Create((Em *) em, 0x13, 0, em->scaleX);
                break;
            }
        } else if (EmBlangonga_IsCongalala(em) == 1) {
            if (EmMotion_PassedFrameScaled(em, 54.0f, 0) != 0) {
                StageObj_K14_Create((Em *) em, 0x13, 0, em->scaleX);
                break;
            }
        } else if (EmMotion_PassedFrameScaled(em, 50.0f, 0) != 0) {
            StageObj_K14_Create((Em *) em, 0x13, 0, em->scaleX);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_CHARGE):
        if (EmBlangonga_IsCongalala(em) == 1) {
            if (EmMotion_PassedFrameScaled(em, 14.0f, 0) != 0) {
                StageObj_K14_Create((Em *) em, 2, 0xF, em->scaleX);
            }
            if (EmMotion_PassedFrameScaled(em, 42.0f, 0) != 0) {
                StageObj_K14_Create((Em *) em, 3, 0x10, em->scaleX);
                break;
            }
        } else {
            if (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0) {
                StageObj_K14_Create((Em *) em, 3, 0x10, em->scaleX);
            }
            if (EmMotion_PassedFrameScaled(em, 38.0f, 0) != 0) {
                StageObj_K14_Create((Em *) em, 2, 0x11, em->scaleX);
            }
            if (EmMotion_PassedFrameScaled(em, 42.0f, 0) != 0) {
                StageObj_K14_Create((Em *) em, 3, 0x10, em->scaleX);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_JUMP_LAND):
        if (EmBlangonga_IsRajang(em) == 1) {
            if (EmMotion_PassedFrameScaled(em, 12.0f, 0) != 0) {
                StageObj_K14_Create((Em *) em, 0xB, 0, 0.6f * em->scaleX);
                break;
            }
        } else if (EmMotion_PassedFrameScaled(em, 8.0f, 0) != 0) {
            EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DIG_IN):
        if (EmMotion_PassedFrameScaled(em, 22.0f, 0) || EmMotion_PassedFrameScaled(em, 92.0f, 0) || EmMotion_PassedFrameScaled(em, 160.0f, 0) || EmMotion_PassedFrameScaled(em, 210.0f, 0) || EmMotion_PassedFrameScaled(em, 246.0f, 0)) {
            StageObj_K14_Create((Em *) em, 2, 0x11, em->scaleX);
        }
        if ((EmMotion_PassedFrameScaled(em, 64.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 134.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 170.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 200.0f, 0) != 0)) {
            StageObj_K14_Create((Em *) em, 3, 0x12, em->scaleX);
        }
        if (EmMotion_PassedFrameScaled(em, 254.0f, 0) != 0) {
            StageObj_K14_Create((Em *) em, 2, 0, em->scaleX);
        }
        if (EmMotion_PassedFrameScaled(em, 42.0f, 0) != 0) {
            BlangongaAttack_Create(em, 0x1B);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_EMERGE):
        if (EmBlangonga_IsSmallKind(em) == 0) {
            EmBlangonga_CamShakeOnFrame((s32) em, 4, 5);
        }
        if ((EmMotion_PassedFrameScaled(em, 8.0f, 0) != 0) && Player_InCurrentArea(em)) {
            Em_BonePos(em, 1, pos);
            pos[1] = em->floorY;
            StageObj_K1C_CreateAt(pos, 6, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 52.0f, 0) != 0) {
            EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_COLLAPSE):
        if (EmMotion_PassedFrameScaled(em, 84.0f, 0) != 0) {
            EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_LIE_DOWN):
        if (EmMotion_PassedFrameScaled(em, 170.0f, 0) != 0) {
            EmBlangonga_SpawnGroundEffect(em, 0x10, 7, 3.5f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TRIPLE_HIT):
        if (EmBlangonga_IsBlangonga(em) == 1) {
            if (EmMotion_PassedFrameScaled(em, 70.0f, 0) != 0) {
                StageObj_K14_Create((Em *) em, 2, 0xF, em->scaleX);
            }
            if (EmMotion_PassedFrameScaled(em, 92.0f, 0) != 0) {
                StageObj_K14_Create((Em *) em, 2, 0x11, em->scaleX);
            }
            if (EmMotion_PassedFrameScaled(em, 104.0f, 0) != 0) {
                EmBlangonga_SpawnGroundEffect(em, 0x10, 7, 3.5f);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TRIPLE_HIT_END):
        if (EmBlangonga_IsCongalala(em) == 1) {
            if ((EmMotion_PassedFrameScaled(em, 38.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 124.0f, 0) != 0)) {
                StageObj_K14_Create((Em *) em, 3, 0x22, em->scaleX);
            }
            if ((EmMotion_PassedFrameScaled(em, 76.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 172.0f, 0) != 0)) {
                StageObj_K14_Create((Em *) em, 2, 0x21, em->scaleX);
            }
            if (EmMotion_PassedFrameScaled(em, 200.0f, 0) != 0) {
                EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_STRIKE_A):
        if (EmBlangonga_IsRajang(em) == 1) {
            if (EmMotion_PassedFrameScaled(em, 48.0f, 0) != 0) {
                StageObj_K14_Create((Em *) em, 3, 0x22, em->scaleX);
            }
            if ((EmMotion_PassedFrameScaled(em, 22.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 74.0f, 0) != 0)) {
                StageObj_K14_Create((Em *) em, 2, 0x21, em->scaleX);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_STRIKE_B):
        if ((EmBlangonga_IsRajang(em) == 1) && (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0)) {
            EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DIG_THROW):
        if ((EmMotion_PassedFrameScaled(em, 102.0f, 0) != 0) && Player_InCurrentArea(em)) {
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
    case EM_MOTION_ID(0, BLANG_MOT_CHARGE_END):
        if (EmBlangonga_IsCongalala(em) == 1) {
            if (EmMotion_PassedFrameScaled(em, 34.0f, 0) != 0) {
                StageObj_K14_Create((Em *) em, 3, 0x10, em->scaleX);
                break;
            }
        } else if (EmBlangonga_IsRajang(em) != 0) {
            if (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) {
                StageObj_K14_Create((Em *) em, 2, 0xF, em->scaleX);
            }
            if (EmMotion_PassedFrameScaled(em, 56.0f, 0) != 0) {
                EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
                break;
            }
        } else {
            if (EmMotion_PassedFrameScaled(em, 12.0f, 0) != 0) {
                StageObj_K14_Create((Em *) em, 2, 0xF, em->scaleX);
            }
            if (EmMotion_PassedFrameScaled(em, 38.0f, 0) != 0) {
                EmBlangonga_SpawnGroundEffect(em, 0x10, 7, 3.5f);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_HOP_LAND):
        if ((EmBlangonga_IsCongalala(em) == 1) && (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0)) {
            EmBlangonga_SpawnGroundEffect(em, 1, 0x28, 1.0f);
            if ((s32)Player_InCurrentArea(em) & 0xFF) {
                StageObj_K11_CreateAtScaled(&em->posX, (u16) em->rotY, 0xAU, 0.4f * em->scaleX);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_ROAR):
        if (EmBlangonga_IsBlangonga(em) == 1) {
            if (em->type == EM_KIND_BLANGONGA) {
                if ((em->partBreakCount <= 0) && (EmMotion_PassedFrameScaled(em, 38.0f, 0) != 0)) {
                    BlangongaAttack_Create(em, 0x17);
                    StageObj_K0F_CreateOnEm(em, 5, 2, 0.4f * em->scaleX);
                    break;
                }
            } else if (EmMotion_PassedFrameScaled(em, 60.0f, 0) != 0) {
                EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
                break;
            }
        } else if (em->type == EM_KIND_CONGALALA) {
            if ((gScene.eventRunning == 0) && (EmMotion_PassedFrameScaled(em, 70.0f, 0) != 0)) {
                EmEffectAttack_Create(em, 7, 0, 0, 0);
                break;
            }
        } else if ((EmBlangonga_IsRajang(em) == 1) && (EmMotion_PassedFrameScaled(em, 38.0f, 0) != 0)) {
            BlangongaAttack_Create(em, 0x17);
            StageObj_K0F_CreateOnEm(em, 5, 2, 0.4f * em->scaleX);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_EAT):
        if ((EmBlangonga_IsCongalala(em) == 1) && (EmMotion_PassedFrameScaled(em, 66.0f, 0) != 0)) {
            EmBlangonga_SpawnGroundEffect(em, 2, 7, 3.5f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_ROAR_B):
        if ((EmBlangonga_IsSmallKind(em) == 0) && (EmMotion_PassedFrameScaled(em, 64.0f, 0) != 0)) {
            BlangongaAttack_Create(em, 0x17);
            StageObj_K0F_CreateOnEm(em, 5, 2, 0.4f * em->scaleX);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DOWN):
        if (EmMotion_PassedFrameScaled(em, 108.0f, 0) != 0) {
            EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_FALL_LAND):
        if (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0) {
            EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DIE):
        if (EmMotion_PassedFrameScaled(em, 180.0f, 0) != 0) {
            EmBlangonga_SpawnGroundEffect(em, 0x10, 7, 3.5f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_KNOCK_DOWN):
        if ((EmMotion_PassedFrameScaled(em, 28.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 78.0f, 0) != 0)) {
            EmBlangonga_SpawnGroundEffect(em, 4, 7, 3.5f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_KNOCK_DOWN_B):
        if ((EmMotion_PassedFrameScaled(em, 28.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 78.0f, 0) != 0)) {
            EmBlangonga_SpawnGroundEffect(em, 4, 7, 3.5f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DIE_END):
        if (EmMotion_PassedFrameScaled(em, 18.0f, 0) != 0) {
            BlangongaAttack_Create(em, 0x1C);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TRAPPED):
        if (EmBlangonga_IsCongalala(em) == 1) {
            if (EmMotion_PassedFrameScaled(em, 16.0f, 0) != 0) {
                EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
                break;
            }
        } else if (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) {
            EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_STRUGGLE):
        if ((EmBlangonga_IsCongalala(em) == 1) && (em->rage != 0) && ((EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 40.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 80.0f, 0) != 0))) {
            StageObj_K1C_CreateOnEm(em, 0x14, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DOWNED):
        if (EmMotion_PassedFrameScaled(em, 92.0f, 0) != 0) {
            EmBlangonga_SpawnGroundEffect(em, 4, 7, 3.5f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DOWN_B):
        if (EmMotion_PassedFrameScaled(em, 108.0f, 0) != 0) {
            EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DROP_CARRIED):
        if ((EmBlangonga_IsCongalala(em) == 1) && (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0)) {
            EmBlangonga_SpawnGroundEffect(em, 0x10, 7, 3.5f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_EFFECT_STRIKE):
        if ((EmBlangonga_IsRajang(em) == 1) && (EmMotion_PassedFrameScaled(em, 74.0f, 0) != 0)) {
            BlangongaAttack_Create(em, 0x28);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_EFFECT_STRIKE_END):
        if ((EmBlangonga_IsCongalala(em) == 1) && (EmMotion_PassedFrameScaled(em, 44.0f, 0) != 0)) {
            EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_CONGALALA_HITZONE_COMBO_END):
        if (EmBlangonga_IsCongalala(em) == 1) {
            if (EmMotion_PassedFrameScaled(em, 96.0f, 0) != 0) {
                EmBlangonga_SpawnGroundEffect(em, 0x10, 7, 3.5f);
                break;
            }
        } else {
            if ((EmBlangonga_IsRajang(em) == 1) && (EmMotion_PassedFrameScaled(em, 26.0f, 0) != 0)) {
                StageObj_K14_Create((Em *) em, 0x13, 0, em->scaleX);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_RAJANG_JUMP_SHOT):
        if (EmBlangonga_IsCongalala(em) == 1) {
            if (EmMotion_PassedFrameScaled(em, 56.0f, 0) != 0) {
                EmBlangonga_SpawnGroundEffect(em, 0x10, 7, 3.5f);
            }
            if (EmMotion_PassedFrameScaled(em, 90.0f, 0) != 0) {
                EmBlangonga_SpawnGroundEffect(em, 2, 7, 3.5f);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_RAJANG_JUMP_SHOT_LAND):
        if ((EmBlangonga_IsRajang(em) == 1) && (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0)) {
            StageObj_K14_Create((Em *) em, 0xB, 0, 0.6f * em->scaleX);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_BACK_STRIKE):
        if (EmBlangonga_IsBlangonga(em) == 1 || EmBlangonga_IsRajang(em) == 1) {
            if (EmMotion_PassedFrameScaled(em, 44.0f, 0) != 0) {
                BlangongaAttack_Create(em, 9);
            }
            if (EmMotion_PassedFrameScaled(em, 50.0f, 0) != 0) {
                BlangongaAttack_Create(em, 0x14);
            }
            if ((EmMotion_PassedFrameScaled(em, 48.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 78.0f, 0) != 0)) {
                EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_IDLE_ACTION_B):
        if (EmBlangonga_IsBlangonga(em) == 1) {
            if ((EmMotion_PassedFrameScaled(em, 60.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 94.0f, 0) != 0)) {
                StageObj_K14_Create((Em *) em, 3, 0x12, em->scaleX);
                break;
            }
        } else {
            if (EmBlangonga_IsRajang(em) == 1) {
                if (EmMotion_PassedFrameScaled(em, 52.0f, 0) != 0) {
                    StageObj_K14_Create((Em *) em, 2, 0xF, em->scaleX);
                }
                if (EmMotion_PassedFrameScaled(em, 62.0f, 0) != 0) {
                    StageObj_K14_Create((Em *) em, 3, 0x10, em->scaleX);
                }
                if (EmMotion_PassedFrameScaled(em, 96.0f, 0) != 0) {
                    EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
                    break;
                }
            }
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_IDLE_ACTION_B_END):
        if (EmBlangonga_IsRajang(em) == 1) {
            if (EmMotion_PassedFrameScaled(em, 16.0f, 0) || EmMotion_PassedFrameScaled(em, 52.0f, 0) || EmMotion_PassedFrameScaled(em, 84.0f, 0)) {
                StageObj_K14_Create((Em *) em, 2, 0xF, em->scaleX);
            }
            if ((EmMotion_PassedFrameScaled(em, 46.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 90.0f, 0) != 0)) {
                StageObj_K14_Create((Em *) em, 3, 0x10, em->scaleX);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_SLAM):
        if (EmBlangonga_IsCongalala(em) == 1) {
            if (EmMotion_PassedFrameScaled(em, 36.0f, 0) != 0) {
                EmBlangonga_SpawnGroundEffect(em, 0x10, 0x13, 1.0f);
            }
        } else {
            if (EmMotion_PassedFrameScaled(em, 22.0f, 0) != 0) {
                EmBlangonga_SpawnGroundEffect(em, 0x10, 0x13, 1.0f);
            }
            if ((EmMotion_PassedFrameScaled(em, 24.0f, 0) != 0) && Player_InCurrentArea(em)) {
                Em_BonePos(em, 0x10, pos);
                pos[1] = em->floorY += 20.0f;
                StageObj_K11_CreateAtScaled(pos, (u16) em->rotY, 0xAU, 0.6f * em->scaleX);
            }
        }
    case EM_MOTION_ID(0, BLANG_MOT_RAJANG_CHARGE_END):
        if ((EmBlangonga_IsRajang(em) == 1) && (EmMotion_PassedFrameScaled(em, 36.0f, 0) != 0)) {
            EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
        }
        break;
    default:
        EmBlangonga_MotionEffectsDefault(em);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_IDLE):
    case EM_MOTION_ID(0, 0x2):
    case EM_MOTION_ID(0, BLANG_MOT_WALK):
    case EM_MOTION_ID(0, BLANG_MOT_TURN_LEFT):
    case EM_MOTION_ID(0, BLANG_MOT_TURN_RIGHT):
    case EM_MOTION_ID(0, BLANG_MOT_LIMP):
    case EM_MOTION_ID(0, BLANG_MOT_JUMP_FALL):
    case EM_MOTION_ID(0, BLANG_MOT_LIE):
    case EM_MOTION_ID(0, BLANG_MOT_GET_UP):
    case EM_MOTION_ID(0, BLANG_MOT_SLAM_WINDUP):
    case EM_MOTION_ID(0, BLANG_MOT_THROW):
    case EM_MOTION_ID(0, BLANG_MOT_ALERT_IDLE):
    case EM_MOTION_ID(0, BLANG_MOT_TURN_ABOUT):
    case EM_MOTION_ID(0, BLANG_MOT_TURN_ABOUT_B):
    case EM_MOTION_ID(0, BLANG_MOT_EAT_NEED2):
    case EM_MOTION_ID(0, BLANG_MOT_EAT_NEED2_END):
    case EM_MOTION_ID(0, BLANG_MOT_EAT_END):
    case EM_MOTION_ID(0, BLANG_MOT_FLINCH_B):
    case EM_MOTION_ID(0, BLANG_MOT_FLINCH):
    case EM_MOTION_ID(0, BLANG_MOT_SHAKE_OFF):
    case EM_MOTION_ID(0, BLANG_MOT_DOWN_GET_UP):
    case EM_MOTION_ID(0, BLANG_MOT_STATUS4):
    case EM_MOTION_ID(0, BLANG_MOT_FALL):
    case EM_MOTION_ID(0, 0x4C):
    case EM_MOTION_ID(0, BLANG_MOT_DOWN_GET_UP_B):
        break;
    }
}

/*
 * Callback slot 3: the first frame only marks the effects started (fxStep);
 * from then on runs EmBlangonga_MotionEffects every frame.
 */
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

/* Callback slot 2: does nothing. */
void EmBlangonga_TableNop(void)
{
}

/*
 * Callback slot 4, the monster script's questions (`kind`): 0 whether
 * moveCount reached 4; 1 whether it stands on snow (floor attribute 1 in
 * AREA_SNOWY_MOUNTAINS_5, AREA_SNOWY_MOUNTAINS_6 and AREA_SNOWY_MOUNTAINS_8,
 * while in the player's area); 2 stateArmor; 3 eatenKind; 4 whether a visible Blangonga in its area is in
 * BLANG_IDLE_ROAR_SIGNAL; 5 the destination area's index among the family's
 * twelve areas (0xFF if none); 6 the first carried item shown (item 0 gives
 * 1, item 2 gives 2, item 1 gives 3; 0 if none).
 */
u8 EmBlangonga_ScriptQuery(Em *em, u8 query)
{
    EmBlangongaWork *w = (EmBlangongaWork *)&em->work;
    Em *other;
    s16 i;

    switch (query) {
    case BLANG_QUERY_MOVE_COUNT:
        return w->moveCount >= 4;
    case BLANG_QUERY_ON_SNOW:
        switch (Area_DayId(em->area)) {
        case AREA_SNOWY_MOUNTAINS_5:
        case AREA_SNOWY_MOUNTAINS_6:
        case AREA_SNOWY_MOUNTAINS_8:
            if (Player_InCurrentArea(em)) {
                if (*(u8 *) &em->floorAttr == 1) {
                    return 1;
                }
                return 0;
            }
            return 0;
        }
        return 0;
    case BLANG_QUERY_ARMOR:
        return EmBlangonga_InStateArmor(em) == 1;
    case BLANG_QUERY_EATEN_KIND:
        return w->eatenKind;
    case BLANG_QUERY_ROAR_SIGNAL:
        for (other = gEms, i = 0; i < EM_COUNT; i++, other++) {
            if (other->active != 0 && other->visible != 0 && (Obj_SameArea(em, (Player *) other) & 0xFF) && other->type == EM_KIND_BLANGONGA && Player_IsState(other, EM_GROUP_IDLE, BLANG_IDLE_ROAR_SIGNAL)) {
                return 1;
            }
        }
        return 0;
    case BLANG_QUERY_DEST_AREA:
        switch (Area_DayId(*(u16 *) &em->destArea)) {
        case AREA_JUNGLE_1:
            return 0;
        case AREA_JUNGLE_2:
            return 1;
        case AREA_JUNGLE_3:
            return 2;
        case AREA_JUNGLE_6:
            return 3;
        case AREA_JUNGLE_9:
            return 4;
        case AREA_JUNGLE_10:
            return 5;
        case AREA_SWAMP_2:
            return 6;
        case AREA_SWAMP_5:
            return 7;
        case AREA_SWAMP_6:
            return 8;
        case AREA_SWAMP_7:
            return 9;
        case AREA_SWAMP_8:
            return 10;
        case AREA_SWAMP_9:
            return 11;
        default:
            return 0xFF;
        }
    case BLANG_QUERY_CARRIED:
        if (EmBlangonga_CarriedShown(em, 0) == 1) {
            return 1;
        }
        if (EmBlangonga_CarriedShown(em, 2) == 1) {
            return 2;
        }
        if (EmBlangonga_CarriedShown(em, 1) == 1) {
            return 3;
        }
        return 0;
    default:
        return 0;
    }
}

/*
 * Callback slot 6: returns 1 while no part break has happened (Em.partBreakCount
 * 0) and Blangonga is on the ground, or Rajang enraged on the ground: the
 * condition for BLANG_REACT_PART_BREAK.
 */
s32 EmBlangonga_AllowPartBreak(Em *em)
{
    if ((s8) em->partBreakCount != 0) {
        return 0;
    }
    if (em->type == EM_KIND_BLANGONGA) {
        if (em->stance == EM_STANCE_GROUND) {
            return 1;
        }
    } else if (em->type == EM_KIND_RAJANG && em->rage != 0 && em->stance == EM_STANCE_GROUND) {
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

    if ((em->blindTimer > 0) || (em->poisonDamage > 0) || (em->hitReactMode == EM_HIT_REACT_STATUS) || ((em->stateGroup == 4) && (em->state == 0xC))) {
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

    if ((em->blindTimer > 0) || (em->poisonDamage > 0) || (em->hitReactMode == EM_HIT_REACT_STATUS) || ((em->stateGroup == 4) && (em->state == 0xC))) {
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
        Em_AddNeed1(em, em->need1Max);
        Em_ClearNeed1State(em);
        break;
    case 7:
        Em_EndStatus4(em);
        break;
    case 10:
        Em_RefillNeed3(em);
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
        Em_AddNeed2(em, em->need2Max);
        Em_ClearNeed2State(em);
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
        em->partBreakCount = (s8) em->partBreakCount + 1;
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
        em->partBreakCount = (s8) em->partBreakCount + 1;
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
    if (EmMotion_PassedFrameScaled((Em *) em, (f32) frame, 0U) != 0) {
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
        Em_PlaySeTypeAtBoneInFrames(em, 0xA, 0x168, 0x14, 2, 8, 0);
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
        Em_PlaySeTypeAtBoneInFrames(em, 4, 0x164, 0x14, 2, 8, 0);
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
EmBlangongaVec EmBlangonga_CarveThrowPt = {
#include "game/008C9E28.inc"
};
void *EmBlangonga_Callbacks[8] = {
    EmBlangonga_SpawnEffect01, EmBlangonga_SpawnEffect01, EmBlangonga_TableNop, EmBlangonga_UpdateMotionEffects,
    EmBlangonga_ScriptQuery, EmBlangonga_Reinit, EmBlangonga_AllowPartBreak, NULL,
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
EmBlangongaVec EmShenGaoren_SeOffsets[4] = {
#include "game/008CA050.inc"
};
