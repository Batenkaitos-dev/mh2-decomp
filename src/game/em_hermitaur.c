/*
 * Monster family: Daimyo Hermitaur and Shogun Ceanataur (kinds 0x30, 0x43).
 * The code from the family's setup function (0x758450, called by the per-kind
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
void EffEm08_CreateAtBone(Em *em, s32 a, s32 b, s32 c, s32 d, s32 e);
u8 Player_InCurrentArea(void *obj);

/* prototypes */
void EmHermitaur_ChangeState(Em *em, u16 group, u16 state, u8 mode);
s8 EmMine_CreateTimed0(Em *em, f32 *pos);
/* unit's own declaration: with the definition's void (Em *em, s16 n) (game/unk_708850.c), EmHermitaur_State0_03: 34 of 73 words differ */
void Em_AddHp(Em *em, s32 n);
void Em_BeginStatus1(Em *em);
void Em_PlaySeAtBone(Em *em, s32 se, s32 bone);
void Em_SetMotion(Em *em, s32 motion, s32 a, s32 b);
u16 Math_AngleXZ(f32 *from, f32 *to);
void Player_ClearFlag(void *, u32 flag);
void Player_SetFlag(void *obj, u32 flag);
void QuestEm_Captured(Em *em);
void QuestEm_Leave(Em *em);
void EffHermitaur26_Create(Em *em, s32 kind);
void EffEm33_CreateV(Em *em, s32 a1, s32 a2, s32 a3, s32 a4, s16 v);
void Em_SelectSightRow(Em *em, u8 idx);
void Em_MoveRotatedAccel(Em *em, void *ang);
void Em_LoadReactionSet(Em *em, u8 k);
void Em_ClearLowHpNeed(Em *em);
void Em_ClearNeed1State(Em *em);
void Em_SpawnEffect06Bone(Em *em, s32 a);
void Em_AddNeed1(Em *em, s32 n);
void Em_SetHitReactMode(Em *em, u8 a);
/* unit's own declaration: with the definition's s32 (Em *em, s16 n, s16 max, s16 interval) (game/unk_708850.c), EmHermitaur_State0_08: 99 of 149 words differ */
s32 Em_RegenHp(Em *em, s32 n, s32 max, s32 interval);
void Em_ClearPoison(Em *em);
void Em_SetHold2(Em *em);
void Em_SetCaptured(Em *em, s32 a);
s32 Em_StepToGoal(Em *em, u16 step, u8 flags);
void Em_RestartScript(Em *em);
void EmHermitaur_Init(Em *em, s16 kind);
void EmHermitaur_State1_00(Em *, EmWork *, s32);
void EmHermitaur_State1_01(Em *, EmWork *, s32);
void EmHermitaur_State1_02(Em *, EmWork *, s32, s32);
void EmHermitaur_State1_06(Em *, EmWork *, s32);
void EmHermitaur_State1_14(Em *, EmWork *, s32);
void EmHermitaur_State2_01(Em *, EmWork *, s32);
void EmHermitaur_State2_02(Em *, EmWork *, s32);
void EmHermitaur_State2_07(Em *, s32, s32);
void EmHermitaur_State5_00_Hunted(Em *, EmWork *);
void EmHermitaur_State5_01(Em *em, EmWork *work);
void EmHermitaur_State6_02_DaimyoEntrance(Em *em);
void EmHermitaur_State6_03_ShogunEntrance(Em *em);
void EmHermitaur_State8_00(Em *, EmWork *);
void EmHermitaur_State8_01(Em *, EmWork *);
void EmHermitaur_State8_03(Em *, EmWork *);
void EmHermitaur_EndState(Em *em);
void EmHermitaur_EndStateTo0_02(Em *em);
void EmHermitaur_EndState2(Em *em);
void EmHermitaur_EndState2To2_05(Em *em);
void EmHermitaur_MotionEffects(Em *, EmWork *);
/* unit's own declaration: with the definition's void (Em *em, u8 k, s16 v, s16 t, f32 y, f32 scale) (game/unk_874B90.c), EmHermitaur_State4_09: 77 of 123 words differ */
void StageObj_Stun_CreateEm(Em *, s32, s32, s32, f32, f32);
void StageObj_Gun_CreateType5(void *owner);
s32 Player_PassedFrameScaled(void *p, f32 frame, s32 layer);
s32 Motion_ReachedFrame(void *, s32, f32);
s32 EmMotion_ReachedFrame(void *obj, s32 a, f32 frame);
s32 EmMotion_InFrameRange(void *em, s32 layer, f32 from, f32 to);
s32 Versus_BothDefeated(void);
s32 QuestEm_CanAppear(Em *em);
void Player_ClearVelocity(void *);
void Effect01_Spawn(void *obj, s16 type);
void Em_SetMotionRow(Em *em, s32 row, s32 arg2, s32 arg3);
u16 Area_DayId(u16 id);
/* unit's own declaration: with the definition's void (void *obj, s16 bone, f32 *out) (sub_main/unk_3DDBA0.c), EmHermitaur_SpawnGroundEffect: 11 of 31 words differ */
void Em_BonePos(void *obj, s32 a, f32 *out);
void Effect0D_SpawnOnGround(f32 *pos, s32 a, f32 scale);
f32 *Mtx_FromAnglesXYZ(s32 *angles, f32 *m);
void StageObj_K25_Create(Player *p, Em *em, u8 k, s32 unused, f32 *pos);
void Versus_RouteEmByTeam(Em *em);
s32 Versus_DefeatCamera(Em *pl);
typedef struct EmHermitaurVec {
    f32 x, y, z;
} EmHermitaurVec;
extern EmHermitaurVec EmHermitaur_CarveThrowPt __attribute__((aligned(8)));
void Em_BeginStatus2(Em *em);
void Em_BeginStatus4(Em *em);
void Em_BeginStatus5(Em *em);
void Em_EndStatus1(Em *em);
void Em_EndStatus2(Em *em);
void Em_EndStatus4(Em *em);
void Em_EndStatus5(Em *em);
void Em_EndAllStatus(Em *em);
void Em_BreakPart(Em *em, s32 part);
void Em_ResetPartHp(Em *em, u32 i);
/* unit's own declaration: with the definition's void (Em *em, s32 group, s32 state, u8 mode) (game/unk_701810.c), EmHermitaur_ChangeState: 3 of 876 words differ */
void Em_SetStateSync(Em *em, u16 group, u16 state, u8 mode);
s8 QuestEm_Appear(Em *em);
u16 Rand_Next(s32 stream);
void Vec_Rotate(f32 *out, f32 *in, f32 *mtx);
void Em_StartGoalAction(Em *em, s32 arg1);
void Em_StartRage(Em *em);
void Em_StartPendingRage(Em *em);
void Em_ComputeGoal2(Em *em, f32 *target);
void Em_RefillNeed3(Em *em);
void Em_ResetNotice(Em *em);
void Em_SetSpeedScaled(Em *em, u16 flags, f32 x, f32 y);
void Em_TargetPlayerInRange(Em *);
f32 *Mtx_FromAnglesYXZ(s32 *angles, f32 *m);
f32 Vec_DistanceXZ(f32 *a, f32 *b);
void Versus_MarkDefeat(Em *);
void EmHermitaur_State4_02(Em *em, EmWork *work, s16 variant);
void EmHermitaur_State3_03(Em *em, EmWork *work);
void EmHermitaur_State3_04(Em *em, EmWork *work, s32 variant);
void EmHermitaur_State3_07(Em *em, EmWork *work);
void EmHermitaur_State3_11(Em *em, EmWork *work);
void EmHermitaur_State3_12(Em *em, EmWork *work, s32 arg2);
void EmHermitaur_State3_16(Em *em, EmWork *work, s32 arg2);
void EmHermitaur_State3_17(Em *em, EmWork *work, s32 arg2);
void QuestEm_Hunted(Em *em);
s8 EmMine_Create(Em *em, s32 idx);
s32 EmMine_Update(Em *em);
void EmMine_Remove(Em *em);
void CamShake_ReqAIf(Em *obj, s32 i);
extern EmTurnParams EmHermitaur_TurnParams[1];
void Em_StartTurnToTarget(Em *em, EmTurnParams *t);
s32 Em_TurnStep(Em *em, EmTurnParams *t);
s32 Motion_PassedFrame(void *, s32 layer, f32 frame);
s32 Em_UpdateCombatTimer(Em *em);
s32 Em_PlaySeChecked(Em *em, s32 id, s32 a, void *pos, s32 b, s32 c);
struct AreaTri;
u8 AreaGrid2_GetCeiling(f32 *pos, f32 *y, struct AreaTri *info);
s32 EmHermitaur_InSpecialArea(u16 arg0);
void Em_RaiseTrigger80Lax(Em *em);
void Em_HoldPosNoPlayers(Em *em, u16 a, u16 b);
u16 Area_BaseId(u16 area);
void Em_CheckLowHp(Em *em, f32 ratio);
void Em_DecayNeed1Only(Em *em);
void Em_DecayNeed1(Em *em);
void Em_CheckRageByHp(Em *em, s16 n);
u8 Em_UpdateDamage(Em *em, u8 *out);
void Em_SetStatus4Timer(Em *em);
void Em_SetStatus2Timer(Em *em);
void Em_SetStatus5Timer(Em *em);
void Em_ResetKindTimer(Em *em);
s32 Save_TestEventFlag(s32 flag);
void Em_UpdatePendingScript(Em *em);
void Em_ClearHitReactMode(Em *em);
void EmHermitaur_RunState(Em *em);
void StageObj_K1C_CreateOnEmC(Em *em, u8 k, u8 c, f32 scale);
void *EffEm33_Create(Em *em, s32 kind, s32 c, s32 a, s32 b);
f32 Vec_Distance(f32 *a, f32 *b);
void Em_MotionDelta(Em *em, f32 *out);
f32 Math_Absf(f32 x);
void Vec_Copy(f32 *out, f32 *in);
extern f32 EmHermitaur_State3_12Ofs[];
void GmEm_ResetSize(Em *em);
s32 Em_ComputeSize(Em *em, s16 idx);
void Em_InitParts(Em *em);
u16 Area_IdForCurrentTime(u16);
void EmHermitaur_MotionSounds(Em *em, EmWork *work);
void StageObj_K14_Create(Em *em, s16 k, s16 c, f32 scale);
void EffEm07_CreateAt(Em *em, f32 *pos, s8 kind);
void Em_SpawnEffect06Periodic(Em *em, s32 bone, f32 *pos, f32 scale);
void EmHermitaur_SpawnGroundEffect(Em *em, s32 a1, s32 a2, f32 scale);
void EmHermitaur_MotionEffectsDefault(Em *em);
/* end of prototypes */

/* Hermitaur's own meanings of the work area's bytes (EmWork, include/em.h, has the shared ones). */
typedef struct EmHermitaurWork {
    u8 fxStep;                  /* 0x000: UpdateMotionEffects: 0, then 1 */
    u8 unk001[0x030 - 0x001];
    f32 distLeft;               /* 0x030: ChangeState: the distance to the goal (Vec_DistanceXZ, or 1000); the states take the steps from it */
    u16 goalAngle;              /* 0x034: Math_AngleXZ to the goal (State2_01 and others) */
    u8 unk036[0x037 - 0x036];
    u8 unk037;                  /* 0x037: the states of groups 1 and 3 test it */
    u8 unk038[0x039 - 0x038];
    u8 unk039;                  /* 0x039: Init: 0; Update: 1 */
    s16 unk03A;                 /* 0x03A: Update: 8 or 5 */
    u8 unk03C[0x04C - 0x03C];
    s16 unk04C;                 /* 0x04C: Init: 0; Update counts it down */
    u8 lastRage;                /* 0x04E: Update: the rage it last saw */
    u8 unk04F;                  /* 0x04F: Init: 0; Setup compares it with 2 */
    u8 lastPartState;           /* 0x050: Init and Update: parts[2].state as last seen */
    u8 unk051[0x100 - 0x051];
} EmHermitaurWork;
typedef char EmHermitaurWork_size_check[(sizeof(EmHermitaurWork) == sizeof(EmWork)) ? 1 : -1];

void EmHermitaur_Setup(Em *em)
{
    EmHermitaurWork *w = (EmHermitaurWork *)&em->work;

    if ((em->type == 0x43) && ((s32) em->parts[2].state >= 2) && ((s32) w->unk04F < 2)) {
        em->areaTimer2 = 0x708;
    }
}

void EmHermitaur_Init(Em *em, s16 kind)
{
    EmHermitaurWork *w = (EmHermitaurWork *)&em->work;
    s16 hp;

    if (gQuest.stage == 0) {
        switch (gScene.area) {
        case 0x25:
            em->posX = 9500.0f + 150.0f * (f32) (em->slotNo / 8);
            em->posY = 0.0f;
            em->posZ = 10000.0f + 150.0f * (f32) (em->slotNo & 7);
            break;
        default:
            em->posX = 5000.0f - 80.0f * (f32) (u32) em->slotNo;
            em->posY = 0.0f;
            em->posZ = 5000.0f - 80.0f * (f32) (u32) em->slotNo;
            break;
        }
    }
    em->blind = 0;
    em->stance = 0;
    em->unk_InitFlag = 1;
    em->gender = 0;
    hp = Em_ComputeSize(em, 0);
    em->hp = hp;
    em->hpMax = hp;
    w->unk039 = 0;
    w->unk04C = 0;
    w->unk04F = 0;
    em->scriptPending = 1;
    em->surfaceY = 1500.0f;
    em->motionSpeed = 1.0f;
    if (em->type == 0x43) {
        em->variant = em->level % 3;
    }
    if (em->type == 0x43) {
        switch (gQuest.stage) {
        case 0x42E:
        case 0x42F:
            if (Save_TestEventFlag(0xA5) == 0) {
                em->area = Area_IdForCurrentTime(0xA6);
                em->posX = 10100.0f;
                em->posY = 1500.0f;
                em->posZ = 6580.0f;
                em->rotY = 0;
                em->stance = 2;
                em->surfaceY = 1500.0f;
                em->ceilingTimer = 5;
            }
            break;
        }
    }
    if (kind == 0) {
        if (em->variant == 0) {
            if (em->type == 0x42 || em->type == 0x49) {
                Em_SetMotionRow(em, 0x23, 0, 0);
                EmHermitaur_ChangeState(em, 3, 0x11, 0);
            } else {
                Em_SetMotionRow(em, 1, 0, 0);
                EmHermitaur_ChangeState(em, 0, 1, 0);
            }
        } else {
            Em_SetMotionRow(em, 1, 0, 0);
            EmHermitaur_ChangeState(em, 0, 1, 0);
        }
    }
    Em_InitParts(em);
    w->lastPartState = em->parts[2].state;
    em->fxFlags = 0;
    switch (em->type) {
    case 0x30:
        em->fxFlags |= 6;
        break;
    case 0x42:
    case 0x49:
    default:
        em->fxFlags |= 7;
        break;
    case 0x43:
        em->fxFlags = 0;
        break;
    }
}

void EmHermitaur_Reinit(Em *em)
{
    EmHermitaur_Init(em, 0x1);
}

void EmHermitaur_Init0(Em *em)
{
    EmHermitaur_Init(em, 0);
}

f32 func_00758860(Em *em)
{
    u8 *w = &em->work;

    if (em->rage == 0) {
        return (f32) *(s16 *) (w + 0x3A) / 8.0f;
    }
    return (f32) *(s16 *) (w + 0x3A) / 5.0f;
}

void EmHermitaur_Update(Em *em)
{
    f32 pos[3];
    u8 buf[4];
    EmHermitaurWork *w = (EmHermitaurWork *)&em->work;
    s32 r;
    u8 n;
    f32 rate;

    if (w->unk04C > 0) {
        w->unk04C -= 1;
    }
    Em_UpdateCombatTimer(em);
    if (em->type == 0x43) {
        if (w->lastRage != em->rage) {
            if (em->rage == 0) {
                w->unk03A = 8;
            } else {
                w->unk03A = 5;
                Em_BonePos(em, 0xA, pos);
                Em_PlaySeChecked(em, 0x2E, 0, pos, 6, 0);
            }
        } else if (w->unk03A > 0) {
            if (em->rage != 0 && w->unk03A == 2) {
                Em_BonePos(em, 6, pos);
                Em_PlaySeChecked(em, 0x2E, 0, pos, 6, 0);
            }
            w->unk03A -= 1;
        }
        w->lastRage = em->rage;
    }
    if (em->type != 0x42 && em->type != 0x49) {
        if (em->type == 0x43) {
            if (gScene.area == em->area) {
                em->hasCeiling = AreaGrid2_GetCeiling(&em->posX, &em->surfaceY, (struct AreaTri *) ((u8 *)w + 0x48));
            } else if (EmHermitaur_InSpecialArea(em->area) == 1) {
                em->hasCeiling = 1;
                em->surfaceY = 1500.0f;
            } else {
                em->hasCeiling = 0;
            }
        } else {
            em->hasCeiling = 0;
        }
        Em_RaiseTrigger80Lax(em);
        Em_HoldPosNoPlayers(em, 0, 1);
        if (em->noAlert != 1 && em->rage == 0) {
            switch ((u16) Area_BaseId(gScene.map)) {
            case 3:
                if (em->type == 0x30) {
                    Em_CheckLowHp(em, 0.2f);
                    Em_DecayNeed1Only(em);
                }
                break;
            case 6:
                if (em->type == 0x30) {
                    Em_CheckLowHp(em, 0.2f);
                    Em_DecayNeed1(em);
                }
                break;
            case 4:
                if (em->type == 0x43) {
                    Em_DecayNeed1(em);
                }
                break;
            case 5:
                break;
            }
        }
        rate = 0.2f;
        if (em->hp < (s16) (em->hpMax * rate)) {
            w->unk039 = 1;
        } else {
            w->unk039 = 0;
        }
    }
    switch (em->stateGroup) {
    default:
        if (em->type == 0x43 && (em->parts[5].state > 0 || em->parts[6].state > 0)) {
            if (em->rage == 0) {
                Em_CheckRageByHp(em, em->rageThreshold);
            } else {
                em->rage = 1;
                em->rageTimer = 0x708;
            }
        }
        break;
    case 5:
        em->rage = 0;
        break;
    }
    r = Em_UpdateDamage(em, buf) & 0xFF;
    switch (r) {
    case 0:
    case 3:
    case 4:
    case 9:
        break;
    case 1:
    case 2:
        EmHermitaur_ChangeState(em, 5, 0, 3);
        break;
    case 5:
        if (em->stateGroup != 4 || em->state != 0xC) {
            EmHermitaur_ChangeState(em, 4, 0xC, 3);
        }
        break;
    case 6:
        if (em->stateGroup != 4 || em->state != 0xC) {
            Em_SetStatus4Timer(em);
            EmHermitaur_ChangeState(em, 4, 4, 3);
        }
        break;
    case 7:
        EmHermitaur_ChangeState(em, 6, 4, 3);
        break;
    case 8:
        if (em->stateGroup != 4 || em->state != 0xC) {
            Em_SetStatus2Timer(em);
            EmHermitaur_ChangeState(em, 0, 8, 3);
        }
        break;
    case 10:
        switch (em->state) {
        case 0xE:
            EmHermitaur_ChangeState(em, 0, 0xF, 3);
            break;
        default:
            EmHermitaur_ChangeState(em, 0, 9, 3);
            break;
        }
        break;
    case 11:
        EmHermitaur_ChangeState(em, 4, 0xD, 3);
        break;
    case 15:
        if (em->type == 0x43) {
            break;
        }
    case 16:
        if (em->type == 0x42 || em->type == 0x49) {
            if (em->stance != 2 && em->stateArmor == 0) {
                EmHermitaur_ChangeState(em, 4, 7, 3);
                break;
            }
        } else if (em->stance != 2 && em->stateArmor == 0 && (s8) em->hermitTimer != 0) {
            EmHermitaur_ChangeState(em, 4, 6, 3);
            break;
        }
        if ((u8) r == 0xF) {
            break;
        }
    case 12:
        Player_ClearFlag(em, 0x20000);
        switch (em->flinchPart) {
        default:
        case 0:
        case 1:
        case 7:
            if (em->stance == 2) {
                EmHermitaur_ChangeState(em, 4, 0xC, 3);
            } else {
                EmHermitaur_ChangeState(em, 4, 0, 3);
            }
            break;
        case 2:
            switch (em->type) {
            case 0x30:
                if (em->stance == 2) {
                    EmHermitaur_ChangeState(em, 4, 0xC, 3);
                } else {
                    EmHermitaur_ChangeState(em, 4, 0, 3);
                }
                break;
            default:
            case 0x42:
            case 0x49:
                EmHermitaur_ChangeState(em, 4, 0, 3);
                break;
            case 0x43:
                if (em->stance == 2) {
                    EmHermitaur_ChangeState(em, 4, 0xC, 3);
                } else if (em->parts[2].state == 2) {
                    EmHermitaur_ChangeState(em, 4, 5, 3);
                } else {
                    EmHermitaur_ChangeState(em, 4, 0, 3);
                }
                break;
            }
            break;
        case 3:
            if (em->stance == 2) {
                EmHermitaur_ChangeState(em, 4, 0xC, 3);
            } else {
                EmHermitaur_ChangeState(em, 4, 2, 3);
            }
            break;
        case 4:
            if (em->stance == 2) {
                EmHermitaur_ChangeState(em, 4, 0xC, 3);
            } else {
                EmHermitaur_ChangeState(em, 4, 3, 3);
            }
            break;
        case 5:
            if (em->stance == 2) {
                EmHermitaur_ChangeState(em, 4, 0xC, 3);
            } else {
                n = 2;
                if (em->type == 0x43) {
                    n = 1;
                }
                if (em->parts[5].state >= n) {
                    switch (em->type) {
                    case 0x30:
                        EmHermitaur_ChangeState(em, 4, 0xF, 3);
                        break;
                    default:
                    case 0x42:
                    case 0x49:
                        EmHermitaur_ChangeState(em, 4, 1, 3);
                        break;
                    case 0x43:
                        EmHermitaur_ChangeState(em, 4, 0xB, 3);
                        break;
                    }
                } else {
                    EmHermitaur_ChangeState(em, 4, 1, 3);
                }
            }
            break;
        case 6:
            if (em->stance == 2) {
                EmHermitaur_ChangeState(em, 4, 0xC, 3);
            } else {
                n = 2;
                if (em->type == 0x43) {
                    n = 1;
                }
                if (em->parts[6].state >= n) {
                    switch (em->type) {
                    case 0x30:
                        EmHermitaur_ChangeState(em, 4, 0x10, 3);
                        break;
                    default:
                    case 0x42:
                    case 0x49:
                        EmHermitaur_ChangeState(em, 4, 1, 3);
                        break;
                    case 0x43:
                        EmHermitaur_ChangeState(em, 4, 0xE, 3);
                        break;
                    }
                } else {
                    EmHermitaur_ChangeState(em, 4, 1, 3);
                }
            }
            break;
        }
        break;
    case 13:
        if (em->stance == 2) {
            EmHermitaur_ChangeState(em, 4, 0xC, 3);
        } else {
            EmHermitaur_ChangeState(em, 4, 0, 3);
            em->scriptPending = 0;
        }
        break;
    case 14:
        if (em->type == 0x30 && (s8) em->hermitTimer != 0) {
            Em_PlaySeChecked(em, 0x2D, 0, &em->posX, 6, 0);
            w->unk04C = 30;
        }
        break;
    case 17:
        if (em->stateGroup != 4 || (em->state != 0xC && em->state != 0xA && em->state != 9)) {
            Em_SetStatus5Timer(em);
            if (em->hitAngle - em->rotY > 0x8000) {
                EmHermitaur_ChangeState(em, 4, 9, 3);
            } else {
                EmHermitaur_ChangeState(em, 4, 0xA, 3);
            }
        }
        break;
    case 18:
        if (em->type != 0x42 && em->type != 0x49) {
            Em_ResetKindTimer(em);
            EmHermitaur_ChangeState(em, 4, 8, 3);
        }
        break;
    }
    switch (gQuest.stage) {
    case 0x400:
    case 0x1389:
        if (em->type == 0x30 && Area_DayId(em->area) == 0x95 && Save_TestEventFlag(0x9A) == 0) {
            if (Area_DayId(gScene.area) == 0x95 && gScene.eventRunning == 1 && em->stateGroup != 6) {
                EmHermitaur_ChangeState(em, 6, 2, 1);
            }
            break;
        }
        goto other;
    case 0x42E:
    case 0x42F:
        if (em->type == 0x43 && Area_DayId(em->area) == 0xA6 && Save_TestEventFlag(0xA5) == 0) {
            if (Area_DayId(gScene.area) == 0xA6 && gScene.eventRunning == 1 && em->stateGroup != 6) {
                EmHermitaur_ChangeState(em, 6, 3, 1);
            }
            break;
        }
    default:
    other:
        Em_UpdatePendingScript(em);
        break;
    }
    Em_ClearHitReactMode(em);
    EmHermitaur_RunState(em);
    if (em->stateRequest != 0) {
        EmHermitaur_RunState(em);
        em->stateRequest = 0;
    }
    if (em->type == 0x30 && em->parts[5].state >= 2 && em->parts[6].state >= 2) {
        Em_BreakPart(em, 2);
    }
    if (em->type == 0x43) {
        if (em->parts[2].state >= 2) {
            em->hitzoneSet = 1;
        }
        if (w->lastPartState != em->parts[2].state) {
            w->lastPartState = em->parts[2].state;
            if (em->parts[2].state == 2) {
                if (w->unk04F == 0) {
                    em->areaTimer2 = 0x708;
                }
                w->unk04F++;
                em->fxFlags |= 1;
                switch (em->variant) {
                case 0:
                    Em_BreakPart(em, 1);
                    break;
                case 1:
                    Em_BreakPart(em, 2);
                    break;
                case 2:
                    Em_BreakPart(em, 3);
                    break;
                }
            }
        }
        if (em->parts[5].state > 0 || em->parts[6].state > 0) {
            Em_BreakPart(em, 4);
        }
        if (em->parts[5].state > 0) {
            em->fxFlags |= 2;
        }
        if (em->parts[6].state > 0) {
            em->fxFlags |= 4;
        }
    }
}

void EmHermitaur_State0_00(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 1, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmHermitaur_EndState(em);
        }
        break;
    }
}

void EmHermitaur_State0_02(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x33, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmHermitaur_EndState(em);
        }
        break;
    }
}

void EmHermitaur_State0_03(Em *em, s32 unused, s32 variant)
{
    if (em->type == 0x49) {
        EmHermitaur_EndState(em);
    }
    Em_SetHitReactMode(em, 3U);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x37, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            if (variant == 1) {
                Em_AddHp(em, (s32) (0.05f * (f32) em->hpMax));
                Em_AddNeed1(em, em->need1Max);
                Em_ClearNeed1State(em);
            }
            if (em->type == 0x30) {
                EmMine_CreateTimed0(em, &em->posX);
            }
            EmHermitaur_EndState(em);
        }
        break;
    }
}

void EmHermitaur_State0_04(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x34, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmHermitaur_EndStateTo0_02(em);
        }
        break;
    }
}

void EmHermitaur_State0_05(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        if (em->stateRand & 1) {
            Em_SetMotion(em, 0x35, 0, 0);
            break;
        }
        Em_SetMotion(em, 0x36, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmHermitaur_EndState(em);
        }
        break;
    }
}

void EmHermitaur_State0_06(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 6, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmHermitaur_EndState(em);
        }
        break;
    }
}

void EmHermitaur_State0_07(Em *em)
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
            EmHermitaur_EndState(em);
        }
        break;
    }
}

void EmHermitaur_State0_08(Em *em, s32 unused, s32 variant)
{
    Em_SetHitReactMode(em, 2U);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x21, 0, 0);
        if (variant == 1) {
            em->timer = 0x1518;
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x1F, 0, 0);
        }
        break;
    case 2:
        if (variant == 1) {
            Em_RegenHp(em, 1, (s32) (0.5f * (f32) em->hpMax), 0xA);
        }
        if (--em->timer <= 0) {
            em->step++;
            if (variant == 1) {
                Em_ClearLowHpNeed(em);
            }
            switch (variant) {
            case 0:
                EmHermitaur_ChangeState(em, 0, 9, 4);
                break;
            case 1:
                EmHermitaur_ChangeState(em, 0, 0x10, 4);
                break;
            case 2:
                EmHermitaur_ChangeState(em, 0, 0xF, 4);
                break;
            }
        }
        break;
    case 3:
        if (em->remote == 0) {
            switch (variant) {
            case 0:
                EmHermitaur_ChangeState(em, 0, 9, 4);
                break;
            case 1:
                EmHermitaur_ChangeState(em, 0, 0x10, 4);
                break;
            case 2:
                EmHermitaur_ChangeState(em, 0, 0xF, 4);
                break;
            }
        }
        break;
    }
}

void EmHermitaur_State0_09(Em *em)
{
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x20, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            EmHermitaur_EndState(em);
        }
        break;
    }
}

void EmHermitaur_State0_0A(Em *em, s32 unused1, s32 unused2)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x34, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->noReact = 0;
            em->step++;
            EmHermitaur_EndState(em);
        }
        break;
    }
}

void EmHermitaur_State0_13(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 1, 0, 0);
        em->timer = 0x1E;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmHermitaur_EndState(em);
        }
        break;
    }
}

void EmHermitaur_State0_14(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x33, 0, 0);
        em->timer = 0x1E;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmHermitaur_EndState(em);
        }
        break;
    }
}

void EmHermitaur_State0_15(Em *em)
{
    Em_SetHitReactMode(em, 3U);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x37, 0, 0);
        Em_SelectSightRow(em, 1);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 476.0f, 0U) != 0) {
            em->step++;
            EmHermitaur_ChangeState(em, 0, 0x16, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmHermitaur_ChangeState(em, 0, 0x16, 4);
        }
        break;
    }
}

void EmHermitaur_State0_16(Em *em)
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
            EmHermitaur_EndState(em);
        }
        break;
    }
}

void EmHermitaur_RunGroup0(Em *em, EmWork *work)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->state) {
    case 0:
        EmHermitaur_State0_00(em);
        break;
    case 1:
        EmHermitaur_State0_00(em);
        break;
    case 2:
        EmHermitaur_State0_02(em);
        break;
    case 3:
        EmHermitaur_State0_03(em, (s32) work, 0);
        break;
    case 4:
        EmHermitaur_State0_04(em);
        break;
    case 5:
        EmHermitaur_State0_05(em);
        break;
    case 6:
        EmHermitaur_State0_06(em);
        break;
    case 7:
        EmHermitaur_State0_07(em);
        break;
    case 8:
        EmHermitaur_State0_08(em, (s32) work, 0);
        break;
    case 9:
        EmHermitaur_State0_09(em);
        break;
    case 10:
        EmHermitaur_State0_0A(em, (s32) work, 0);
        break;
    case 11:
        EmHermitaur_State0_0A(em, (s32) work, 1);
        break;
    case 12:
        EmHermitaur_State0_0A(em, (s32) work, 2);
        break;
    case 13:
        EmHermitaur_State0_08(em, (s32) work, 1);
        break;
    case 14:
        EmHermitaur_State0_08(em, (s32) work, 2);
        break;
    case 15:
        EmHermitaur_State0_09(em);
        break;
    case 16:
        EmHermitaur_State0_09(em);
        break;
    case 17:
        EmHermitaur_State0_04(em);
        break;
    case 18:
        EmHermitaur_State0_04(em);
        break;
    case 19:
        EmHermitaur_State0_13(em);
        break;
    case 20:
        EmHermitaur_State0_14(em);
        break;
    case 21:
        EmHermitaur_State0_15(em);
        break;
    case 22:
        EmHermitaur_State0_16(em);
        break;
    case 23:
        EmHermitaur_State0_03(em, (s32) work, 1);
        break;
    }
}

void EmHermitaur_State1_00(Em *em, EmWork *work, s32 variant)
{
    EmHermitaurWork *cw = (EmHermitaurWork *)work;
    u32 speed;
    u32 d;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        switch (variant) {
        case 0:
            cw->goalAngle = Math_AngleXZ(&em->posX, em->goal);
            break;
        case 1:
            cw->goalAngle = Math_AngleXZ(&em->posX, em->goal) + 0x8000;
            break;
        case 2:
            cw->goalAngle = Math_AngleXZ(&em->posX, em->goal) + 0x4000;
            break;
        case 3:
            cw->goalAngle = Math_AngleXZ(&em->posX, em->goal) + 0xC000;
            break;
        }
        if (((cw->goalAngle - em->rotY) & 0xFFFF) >= 0x8000U) {
            Em_SetMotion(em, 5, 0, 0);
        } else {
            Em_SetMotion(em, 6, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].blend == 0 && EmMotion_ReachedFrame(em, 0, 8.0f)) {
            speed = 409.6f * em->motionSpeed;
            d = (u16) (cw->goalAngle - *(u16 *) &em->rotY);
            if (em->motion[0].playing == 0 || EmMotion_ReachedFrame(em, 0, 88.0f)) {
                if ((u16) (d + speed) < speed * 2) {
                    em->step++;
                    Player_ClearFlag(em, 0x20000);
                    EmHermitaur_EndState(em);
                } else {
                    Player_ClearFlag(em, 0x20000);
                    if (d >= 0x8000) {
                        Em_SetMotion(em, 5, 6, 0);
                    } else {
                        Em_SetMotion(em, 6, 6, 0);
                    }
                }
            } else if ((u16) (d + speed) < speed * 2) {
                em->rotY = cw->goalAngle;
            } else if (d < 0x8000) {
                em->rotY = (u16) (em->rotY + speed);
            } else {
                em->rotY = (u16) (em->rotY - speed);
            }
        }
        break;
    }
    Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
}

void EmHermitaur_State1_01(Em *em, EmWork *work, s32 variant)
{
    f32 v[3];
    u16 speed;
    u16 d;

    switch (em->step) {
    case 0:
        em->step++;
        if (variant == 0) {
            Em_SetMotion(em, 2, 0, 0);
        } else {
            Em_SetMotion(em, 3, 0, 0);
        }
        break;
    case 1:
        if (((EmHermitaurWork *)work)->unk037 != 0) {
            if (em->type == 0x42 || em->type == 0x49) {
                speed = 0x400;
            } else {
                speed = 0x40;
            }
            d = Math_AngleXZ(&em->posX, em->goal) - em->rotY;
            if (variant != 0) {
                d += 0x8000;
            }
            if (d <= 0x8000) {
                if (d < speed) {
                    em->rotY += d;
                } else {
                    em->rotY += speed;
                }
            } else if (0x10000 - speed < d) {
                em->rotY += d;
            } else {
                em->rotY -= speed;
            }
        }
        if ((em->type == 0x42 || em->type == 0x49) && em->curGoalKind == 1 && em->curGoalArg == 0) {
            if (Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) < 200.0f || --em->timer <= 0) {
                EmHermitaur_EndState(em);
            }
        } else {
            Em_MotionDelta(em, v);
            if (variant != 0) {
                v[2] *= -1.0f;
            }
            ((EmHermitaurWork *)work)->distLeft -= v[2];
            if (((EmHermitaurWork *)work)->distLeft <= 0.0f) {
                em->step++;
                EmHermitaur_EndState(em);
            }
        }
        break;
    }
}

void EmHermitaur_State1_02(Em *em, EmWork *work, s32 kind, s32 mode)
{
    f32 v[3];
    u16 speed;
    u16 d;

    switch (em->step) {
    case 0:
        em->step++;
        switch (kind) {
        case 0:
            Em_SetMotion(em, 0x2D, 0, 0);
            break;
        case 1:
            Em_SetMotion(em, 0x2E, 0, 0);
            EffHermitaur26_Create(em, 9);
            break;
        case 2:
            Em_SetMotion(em, 0x25, 0, 0);
            EffHermitaur26_Create(em, 0x1B);
            break;
        }
        if (mode == 1) {
            if (em->type == 0x42 || em->type == 0x49) {
                em->speedBonus = 200;
            } else {
                em->speedBonus = 10;
            }
        } else if (em->type == 0x42 || em->type == 0x49) {
            em->speedBonus = 100;
        } else {
            em->speedBonus = 10;
        }
        break;
    case 1:
        if (((EmHermitaurWork *)work)->unk037 != 0) {
            if (em->type == 0x42 || em->type == 0x49) {
                speed = 0x400;
            } else {
                speed = 0x40;
            }
            d = Math_AngleXZ(&em->posX, em->goal) - em->rotY;
            if (kind != 0) {
                d += 0x8000;
            }
            if (d <= 0x8000) {
                if (d < speed) {
                    em->rotY += d;
                } else {
                    em->rotY += speed;
                }
            } else if (0x10000 - speed < d) {
                em->rotY += d;
            } else {
                em->rotY -= speed;
            }
        }
        if ((em->type == 0x42 || em->type == 0x49) && em->curGoalKind == 1 && em->curGoalArg == 0) {
            if (Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) < 200.0f || --em->timer <= 0) {
                EmHermitaur_EndState(em);
            }
        } else {
            Em_MotionDelta(em, v);
            if (kind == 1) {
                v[2] *= -1.0f;
            }
            ((EmHermitaurWork *)work)->distLeft -= v[2];
            if (((EmHermitaurWork *)work)->distLeft <= 0.0f) {
                em->step++;
                EmHermitaur_EndState(em);
            }
        }
        break;
    }
}

void EmHermitaur_State1_06(Em *em, EmWork *work, s32 variant)
{
    f32 v[3];
    u16 speed;
    u16 a;
    u16 d;

    switch (em->step) {
    case 0:
        em->step++;
        if (variant == 0 || variant == 2 || variant == 4) {
            Em_SetMotion(em, 0x14, 0, 0);
        } else {
            Em_SetMotion(em, 0x16, 0, 0);
        }
        switch (variant) {
        case 2:
        case 3:
        case 4:
        case 5:
            if (em->type == 0x42 || em->type == 0x49) {
                em->speedBonus = 200;
            } else {
                em->speedBonus = 10;
            }
            break;
        }
        break;
    case 1:
        if (((EmHermitaurWork *)work)->unk037 != 0) {
            if (em->type == 0x42 || em->type == 0x49) {
                speed = 0x400;
            } else {
                speed = 0x40;
            }
            if (variant == 0 || variant == 2) {
                a = Math_AngleXZ(&em->posX, em->goal) + 0x4000;
            } else {
                a = Math_AngleXZ(&em->posX, em->goal) + 0xC000;
            }
            d = a - em->rotY;
            if (d <= 0x8000) {
                if (d < speed) {
                    em->rotY += d;
                } else {
                    em->rotY += speed;
                }
            } else if (0x10000 - speed < d) {
                em->rotY += d;
            } else {
                em->rotY -= speed;
            }
        }
        if ((em->type == 0x42 || em->type == 0x49) && em->curGoalKind == 1 && em->curGoalArg == 0) {
            if (Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) < 200.0f || --em->timer <= 0) {
                EmHermitaur_EndState(em);
            }
        } else {
            Em_MotionDelta(em, v);
            if (variant != 0) {
                v[0] *= -1.0f;
            }
            *(f32 *) &work->moveAngX -= Math_Absf(v[0]);
            if (*(f32 *) &work->moveAngX <= 0.0f) {
                em->step++;
                EmHermitaur_EndState(em);
            }
        }
        break;
    }
}

void EmHermitaur_State1_14(Em *em, EmWork *work, s32 variant)
{
    EmHermitaurWork *cw = (EmHermitaurWork *)work;
    u32 speed;
    u32 d;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        switch (variant) {
        case 0:
            cw->goalAngle = Math_AngleXZ(&em->posX, em->goal);
            break;
        case 1:
            cw->goalAngle = Math_AngleXZ(&em->posX, em->goal) + 0x8000;
            break;
        case 2:
            cw->goalAngle = Math_AngleXZ(&em->posX, em->goal) + 0x4000;
            break;
        case 3:
            cw->goalAngle = Math_AngleXZ(&em->posX, em->goal) + 0xC000;
            break;
        }
        if (((cw->goalAngle - em->rotY) & 0xFFFF) >= 0x8000U) {
            Em_SetMotion(em, 7, 0, 0);
        } else {
            Em_SetMotion(em, 8, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].blend == 0 && EmMotion_ReachedFrame(em, 0, 6.0f)) {
            speed = 564.9655f * em->motionSpeed;
            d = (u16) (cw->goalAngle - *(u16 *) &em->rotY);
            if (em->motion[0].playing == 0 || EmMotion_ReachedFrame(em, 0, 64.0f)) {
                if ((u16) (d + speed) < speed * 2) {
                    em->step++;
                    Player_ClearFlag(em, 0x20000);
                    EmHermitaur_EndState(em);
                } else {
                    Player_ClearFlag(em, 0x20000);
                    if (d >= 0x8000) {
                        Em_SetMotion(em, 7, 4, 0);
                    } else {
                        Em_SetMotion(em, 8, 4, 0);
                    }
                }
            } else if ((u16) (d + speed) < speed * 2) {
                em->rotY = cw->goalAngle;
            } else if (d < 0x8000) {
                em->rotY = (u16) (em->rotY + speed);
            } else {
                em->rotY = (u16) (em->rotY - speed);
            }
        }
        break;
    }
    Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
}

void EmHermitaur_RunGroup1(Em *em, EmWork *work)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->state) {
    case 0:
        EmHermitaur_State1_00(em, work, 0);
        break;
    case 1:
        EmHermitaur_State1_01(em, work, 0);
        break;
    case 2:
        EmHermitaur_State1_02(em, work, 0, 1);
        break;
    case 3:
        EmHermitaur_State1_00(em, work, 1);
        break;
    case 4:
        EmHermitaur_State1_01(em, work, 1);
        break;
    case 5:
        EmHermitaur_State1_02(em, work, 1, 1);
        break;
    case 6:
        EmHermitaur_State1_06(em, work, 0);
        break;
    case 7:
        EmHermitaur_State1_06(em, work, 1);
        break;
    case 8:
        EmHermitaur_State1_00(em, work, 2);
        break;
    case 9:
        EmHermitaur_State1_00(em, work, 3);
        break;
    case 10:
        EmHermitaur_State1_06(em, work, 2);
        break;
    case 11:
        EmHermitaur_State1_06(em, work, 3);
        break;
    case 12:
        EmHermitaur_State1_06(em, work, 0);
        break;
    case 13:
        EmHermitaur_State1_06(em, work, 1);
        break;
    case 14:
        EmHermitaur_State1_06(em, work, 0);
        break;
    case 15:
        EmHermitaur_State1_06(em, work, 1);
        break;
    case 16:
        EmHermitaur_State1_06(em, work, 0);
        break;
    case 17:
        EmHermitaur_State1_06(em, work, 1);
        break;
    case 18:
        EmHermitaur_State1_02(em, work, 0, 0);
        break;
    case 19:
        EmHermitaur_State1_02(em, work, 1, 0);
        break;
    case 20:
        EmHermitaur_State1_14(em, work, 0);
        break;
    case 21:
        EmHermitaur_State1_14(em, work, 1);
        break;
    case 22:
        EmHermitaur_State1_14(em, work, 2);
        break;
    case 23:
        EmHermitaur_State1_14(em, work, 3);
        break;
    case 24:
        EmHermitaur_State1_02(em, work, 2, 0);
        
    default:
        break;
    }
}

void EmHermitaur_State2_00(Em *em, s32 unused1, s32 unused2)
{
    em->noPushTimer = 2;
    em->guardTimer = 2;
    em->noReact = 1;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x39, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmHermitaur_EndState2(em);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmHermitaur_EndState2(em);
        }
        break;
    }
}

void EmHermitaur_State2_01(Em *em, EmWork *work, s32 unused)
{
    u32 diff;
    u16 ang;
    s32 rotY;

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
        em->lookAtTarget = 0;
        ((EmHermitaurWork *)work)->goalAngle = Math_AngleXZ(&em->posX, em->goal);
        Em_SetMotion(em, 0x23, 0, 0);
        break;
    case 1:
        rotY = em->rotY;
        ang = ((EmHermitaurWork *)work)->goalAngle;
        diff = (ang - (rotY & 0xFFFF)) & 0xFFFF;
        if ((u32) ((diff + 0x400) & 0xFFFF) < 0x800U) {
            em->rotY = (s32) ang;
            em->step++;
            Player_ClearFlag(em, 0x20000);
            EmHermitaur_EndState2(em);
        } else if (diff < 0x8000U) {
            em->rotY = (rotY + 0x400) & 0xFFFF;
        } else {
            em->rotY = (rotY - 0x400) & 0xFFFF;
        }
        break;
    }
    Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
}

void EmHermitaur_State2_02(Em *em, EmWork *work, s32 variant)
{
    f32 v[3];
    f32 out[3];
    s32 ang[3];
    f32 mtx[16];
    s32 d;

    em->stateArmor = 1;
    em->noPushTimer = 2;
    em->guardTimer = 2;
    em->noReact = 1;
    em->hideShadowHold = 2;
    em->hideShadow = 2;
    if (variant == 1) {
        em->fieldCollide = 0;
        em->noHitTimer = 5;
        em->senseDelay = 5;
    }
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x23, 0, 0);
        break;
    case 1:
        if (((EmHermitaurWork *)work)->unk037 != 0) {
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
        ((EmHermitaurWork *)work)->distLeft -= v[2];
        if (((EmHermitaurWork *)work)->distLeft <= 0.0f) {
            em->step++;
            EmHermitaur_EndState2(em);
        }
        break;
    case 2:
        EmHermitaur_EndState2(em);
        break;
    }
}

void EmHermitaur_State2_03(Em *em, s32 unused1, s32 unused2)
{
    em->hideShadowHold = 2;
    em->hideShadow = 2;
    em->stateArmor = 1;
    em->noPushTimer = 2;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x23, 0, 0);
        em->timer = 0x5A;
        break;
    case 1:
        if (--em->timer <= 0) {
            EmHermitaur_EndState2(em);
        }
        break;
    }
}

void EmHermitaur_State2_04(Em *em, s32 unused1, s32 unused2)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0xA, 0, 0);
        em->stance = 0;
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 114.0f, 0U) != 0) {
            em->step++;
            em->stance = 2;
            em->posY += 200.0f;
            Player_ClearVelocity(em);
            em->velY = 100.0f;
            em->accelY = -5.0f;
        }
        break;
    case 2:
        Em_MoveRotatedAccel(em, &em->rotX);
        if (em->velY < 50.0f) {
            em->velY = 50.0f;
            em->accelY = 0.0f;
        }
        if (!(em->posY <= (em->surfaceY - 760.0f))) {
            em->step++;
            em->posY = em->surfaceY;
            Em_SetMotion(em, 0x3A, 0, 0);
            em->ceilingTimer = 5;
        }
        break;
    case 3:
        em->ceilingTimer = 5;
        if (em->motion[0].playing == 0) {
            em->step++;
            EmHermitaur_EndState2To2_05(em);
        }
        break;
    }
}

void EmHermitaur_State2_05(Em *em, s32 unused1, s32 unused2)
{
    em->ceilingTimer = 5;
    em->posY = em->surfaceY;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        if (em->motionId[0] != 0x3F1) {
            Em_SetMotion(em, 9, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmHermitaur_EndState2To2_05(em);
        }
        break;
    }
}

void EmHermitaur_State2_06(Em *em, s32 unused1, s32 unused2)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        Em_SetMotion(em, 0x3B, 0, 0);
        Player_ClearVelocity(em);
        em->velY = -50.0f;
        em->accelY = -5.0f;
        em->ceilingTimer = 5;
        break;
    case 1:
        em->ceilingTimer = 5;
        if (Player_PassedFrameScaled(em, 108.0f, 0U) != 0) {
            em->step++;
            em->posY -= 400.0f;
            em->ceilingTimer = 0;
            EffHermitaur26_Create(em, 0x21);
        }
        break;
    case 2:
        Em_MoveRotatedAccel(em, &em->rotX);
        floorY = em->floorY;
        if (em->posY <= floorY) {
            em->posY = floorY;
            em->step++;
            em->stance = 0;
            Em_SetMotion(em, 0xB, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmHermitaur_EndState(em);
        }
        break;
    }
}

void EmHermitaur_State2_07(Em *em, s32 unused1, s32 unused2)
{
    em->ceilingTimer = 5;
    em->posY = em->surfaceY;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        Em_StartTurnToTarget(em, &EmHermitaur_TurnParams[0]);
        break;
    case 1:
        if (!(EmMotion_ReachedFrame(em, 0, 8.0f) != 0) == 0) {
            em->step++;
        case 2:
            if (Em_TurnStep(em, &EmHermitaur_TurnParams[0]) == 1) {
                em->step++;
                Player_ClearFlag(em, 0x20000);
                EmHermitaur_EndState2To2_05(em);
            }
        }
        break;
    }
}

void EmHermitaur_State2_08(Em *em, s32 unused, s32 variant)
{
    em->ceilingTimer = 5;
    em->posY = em->surfaceY;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0xC, 0, 0);
        if (variant == 1) {
            em->motionSpeed = 50.0f;
        }
        if ((em->variant == 0) && ((s32) em->parts[2].state < 2)) {
            EffEm33_CreateV(em, 0xA, 0, 0, 0x8000, 0x64);
        }
        break;
    case 1:
        if (Em_StepToGoal(em, 0x40, 0) == 1) {
            em->step++;
            EmHermitaur_EndState2To2_05(em);
        }
        break;
    }
}

void EmHermitaur_State2_09(Em *em, s32 unused1, s32 unused2)
{
    em->stateArmor = 1;
    em->noPushTimer = 2;
    em->guardTimer = 2;
    em->noReact = 1;
    em->hideShadowHold = 2;
    em->hideShadow = 2;
    em->noHitTimer = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, 0x32, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            if (em->type == 0x43) {
                EmMine_CreateTimed0(em, &em->posX);
            }
            em->noHitTimer = 0;
            EmHermitaur_EndState2(em);
        }
        break;
    }
}

void EmHermitaur_RunGroup2(Em *em, s32 work)
{
    switch (em->state) {
    case 0:
        EmHermitaur_State2_00(em, work, 0);
        break;
    case 1:
        EmHermitaur_State2_01(em, (EmWork *) work, 0);
        break;
    case 2:
        EmHermitaur_State2_02(em, (EmWork *) work, 0);
        break;
    case 3:
        EmHermitaur_State2_03(em, work, 0);
        break;
    case 4:
        EmHermitaur_State2_04(em, work, 0);
        break;
    case 5:
        EmHermitaur_State2_05(em, work, 0);
        break;
    case 6:
        EmHermitaur_State2_06(em, work, 0);
        break;
    case 7:
        EmHermitaur_State2_07(em, work, 0);
        break;
    case 8:
        EmHermitaur_State2_08(em, work, 0);
        break;
    case 9:
        EmHermitaur_State2_09(em, work, 0);
        break;
    case 10:
        EmHermitaur_State2_02(em, (EmWork *) work, 1);
        break;
    case 11:
        EmHermitaur_State2_08(em, work, 1);
        break;
    case 12:
        EmHermitaur_State2_09(em, work, 0);
        break;
    case 13:
        EmHermitaur_State2_09(em, work, 1);
        break;
    case 14:
        EmHermitaur_State2_09(em, work, 2);
        
    default:
        break;
    }
}

void EmHermitaur_State3_00(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x2A, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmHermitaur_EndState(em);
        }
        break;
    }
}

void EmHermitaur_State3_01(Em *em, s32 unused, s32 variant)
{
    s32 k;

    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x2C, 0, 0);
        k = variant & 0xFF;
        em->speedBonus = 0x32;
        switch (k) {
        case 0:
            em->timer = 1;
            break;
        case 1:
            em->timer = 5;
            break;
        case 2:
            em->timer = 0xA;
        }
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 56.0f) != 0) {
            if (--em->timer <= 0) {
                em->step++;
                Em_SetMotion(em, 0x2B, 0, 0);
                break;
            }
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmHermitaur_EndState(em);
        }
        break;
    }
}

void EmHermitaur_State3_03(Em *em, EmWork *work)
{
    switch (em->type) {
    default:
        EmHermitaur_EndState(em);
        break;
    case 0x30:
    case 0x49:
        switch (em->step) {
        case 0:
            em->step++;
            Em_SetMotion(em, 0x27, 0, 0);
            break;
        case 1:
            if (Motion_ReachedFrame(em, 0, 82.0f) != 0) {
                em->step++;
                if (em->type == 0x30) {
                    EffEm33_Create(em, 4, 0, 0x38E, 0);
                    break;
                }
                EffEm08_CreateAtBone(em, 0x1F, 0x16, 0, 0xF8E5, 0);
            }
            break;
        case 2:
            if (Motion_PassedFrame(em, 0, 86.0f) != 0) {
                if (em->type == 0x30) {
                    EffHermitaur26_Create(em, 0xC);
                } else {
                    EffHermitaur26_Create(em, 0x20);
                }
            }
            if (em->motion[0].playing == 0) {
                em->step++;
                Em_SetMotion(em, 0x26, 0, 0);
            }
            break;
        case 3:
            if (em->motion[0].playing == 0) {
                em->step++;
                EmHermitaur_EndState(em);
            }
            break;
        }
        break;
    }
}

void EmHermitaur_State3_04(Em *em, EmWork *work, s32 variant)
{
    if (em->type != 0x30) {
        EmHermitaur_EndState(em);
    } else {
        em->hermitTimer = 0xA;
        if ((s32) em->parts[5].state >= 2 && (s32) em->parts[6].state >= 2) {
            em->hitzoneSet = 1;
        } else {
            em->hitzoneSet = 2;
        }
        switch (em->step) {
        case 0:
            em->step++;
            Em_SetMotion(em, 0x28, 0, 0);
            if (variant == 0) {
                em->timer = 0x168;
                break;
            }
            em->timer = 0xB4;
            break;
        case 1:
            if (--em->timer <= 0) {
                em->step++;
                EmHermitaur_ChangeState(em, 3, 5, 1);
            }
            break;
        case 2:
            if (em->remote == 0) {
                EmHermitaur_ChangeState(em, 3, 5, 1);
            }
            break;
        }
    }
}

void EmHermitaur_State3_05(Em *em)
{
    if (em->type != 0x30) {
        EmHermitaur_EndState(em);
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            Em_SetMotion(em, 0x29, 0, 0);
            break;
        case 1:
            if (em->motion[0].playing == 0) {
                em->step++;
                EmHermitaur_EndState(em);
            }
            break;
        }
    }
}

void EmHermitaur_State3_07(Em *em, EmWork *work)
{
    f32 v[3];
    f32 out[3];
    s32 ang[3];
    f32 mtx[16];

    em->noPushTimer = 2;
    em->guardTimer = 2;
    em->noReact = 1;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x39, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stateArmor = 1;
            em->noReact = 0;
            switch (em->type) {
            case 0x30:
            case 0x43:
                v[0] = 0.0f;
                v[1] = 0.0f;
                v[2] = 300.0f + ((f32) (em->stateRand % 500) - 300.0f);
                ang[0] = 0;
                ang[1] = em->rotY;
                ang[2] = 0;
                Mtx_FromAnglesYXZ(ang, mtx);
                Vec_Rotate(out, v, mtx);
                em->posX += out[0];
                em->posZ += out[2];
                EmHermitaur_ChangeState(em, 3, 0x11, 1);
                break;
            default:
                EmHermitaur_ChangeState(em, 3, 0x11, 1);
                break;
            }
        }
        break;
    case 2:
        if (em->remote == 0) {
            em->stateArmor = 1;
            em->noReact = 0;
            EmHermitaur_ChangeState(em, 3, 0x11, 1);
        }
        break;
    }
}

void EmHermitaur_State3_08(Em *em, s32 unused, s32 variant)
{
    em->noPushTimer = 2;
    em->guardTimer = 2;
    em->noReact = 1;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x30, 0, 0);
        if (em->type != 0x43) {
            EffHermitaur26_Create(em, 0xA);
        }
        em->hideShadowHold = 2;
        em->hideShadow = 2;
        em->stateArmor = 0;
        break;
    case 1:
        if (em->type == 0x43) {
            if (variant == 0) {
                if (em->motion[0].playing == 0) {
                    em->step++;
                    EmHermitaur_ChangeState(em, 3, 9, 1);
                    break;
                }
            } else if (em->motion[0].playing == 0) {
                em->step++;
                EmHermitaur_EndState2(em);
                break;
            }
        } else if (variant == 0) {
            if (EmMotion_ReachedFrame(em, 0, 46.0f) != 0) {
                em->step++;
                EmHermitaur_ChangeState(em, 3, 9, 1);
                break;
            }
        } else {
            if (em->motion[0].playing == 0) {
                em->step++;
                EmHermitaur_EndState2(em);
            }
            break;
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmHermitaur_ChangeState(em, 3, 9, 1);
        }
        break;
    }
}

void EmHermitaur_State3_09(Em *em)
{
    u8 type;

    em->noPushTimer = 2;
    em->guardTimer = 2;
    em->noReact = 1;
    switch (em->step) {
    case 0:
        em->step++;
        type = em->type;
        if ((type == 0x42) || (type == 0x49)) {
            Em_SetMotionRow(em, 0x22, 0, 0);
        } else {
            Em_SetMotion(em, 0x22, 0, 0);
        }
        em->stateArmor = 0;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stateArmor = 0;
            em->noPushTimer = 0;
            em->guardTimer = 0;
            em->noReact = 0;
            EmHermitaur_EndState(em);
        }
        break;
    }
}

void EmHermitaur_State3_0A(Em *em, s32 unused, s32 variant)
{
    u8 stateVar0;
    u8 temp_v1_2;

    if (em->type != 0x30) {
        EmHermitaur_EndState(em);
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            em->stateVar0 = 0;
            Em_SetMotion(em, 0xA, 0, 0);
            Player_ClearVelocity(em);
            em->stance = 0;
            break;
        case 1:
            if (Motion_ReachedFrame(em, 0, 98.0f) != 0) {
                em->step++;
                em->stance = 2;
                em->posY += 500.0f * em->scaleX;
                switch (variant) {
                case 0:
                    em->velY = 120.0f;
                    em->accelY = -10.0f;
                    break;
                case 1:
                    em->velY = 140.0f;
                    em->accelY = -10.0f;
                    break;
                case 2:
                    em->velY = 160.0f;
                    em->accelY = -10.0f;
                    break;
                }
            } else {
            }
            break;
        case 2:
            Em_MoveRotatedAccel(em, &em->rotX);
            if (!((200.0f + em->floorY) <= em->posY)) {
                em->step++;
                em->posY = em->floorY;
                em->stance = 0;
                Em_SetMotion(em, 0xB, 0, 0);
                stateVar0 = em->stateVar0;
                if (stateVar0 == 0) {
                    em->stateVar0 = stateVar0 + 1;
                    EffHermitaur26_Create(em, 0xB);
                    break;
                }
            } else if (Player_PassedFrameScaled(em, 154.0f, 0U) != 0) {
                temp_v1_2 = em->stateVar0;
                if (temp_v1_2 == 0) {
                    em->stateVar0 = temp_v1_2 + 1;
                    EffHermitaur26_Create(em, 0xB);
                    break;
                }
            }
            break;
        case 3:
            if (em->motion[0].playing == 0) {
                em->step++;
                EmHermitaur_EndState(em);
            }
            break;
        }
    }
}

void EmHermitaur_State3_0D(Em *em, s32 unused, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        if (variant == 0) {
            Em_SetMotion(em, 0x15, 0, 0);
            break;
        }
        Em_SetMotion(em, 0x17, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmHermitaur_EndState(em);
        }
        break;
    }
}

void EmHermitaur_State3_0F(Em *em, s32 unused, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        if (variant == 0) {
            Em_SetMotion(em, 0x13, 0, 0);
            break;
        }
        Em_SetMotion(em, 0x19, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmHermitaur_EndState(em);
        }
        break;
    }
}

void EmHermitaur_State3_11(Em *em, EmWork *work)
{
    s16 i;
    Player *p;

    em->stateArmor = 1;
    em->noPushTimer = 2;
    em->senseDelay = 5;
    em->noHitTimer = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->hideShadowHold = 2;
        em->hideShadow = 2;
        Em_SetMotion(em, 0x23, 0, 0);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            if (em->remote == 0) {
                if (em->type == 0x42 || em->type == 0x49) {
                    EmHermitaur_ChangeState(em, 3, 9, 1);
                } else {
                    EmHermitaur_ChangeState(em, 3, 8, 1);
                }
            }
        } else if (em->remote == 0) {
            for (i = 0, p = gPlayers; i < 4; i++, p++) {
                if (p->active && p->area == em->area && p->visible && Vec_Distance((f32 *) &em->posX, (f32 *) p->pos) < 200.0f) {
                    if (em->type == 0x42 || em->type == 0x49) {
                        EmHermitaur_ChangeState(em, 3, 9, 1);
                    } else {
                        EmHermitaur_ChangeState(em, 3, 8, 1);
                    }
                }
            }
        }
        break;
    case 2:
        if (em->remote == 0) {
            if (em->type == 0x42 || em->type == 0x49) {
                EmHermitaur_ChangeState(em, 3, 9, 1);
            } else {
                EmHermitaur_ChangeState(em, 3, 8, 1);
            }
        }
        break;
    }
}

void EmHermitaur_State3_12(Em *em, EmWork *work, s32 variant)
{
    f32 v[3];
    f32 out[3];
    s32 ang[3];
    f32 mtx[16];

    em->noPushTimer = 2;
    em->guardTimer = 2;
    em->noReact = 1;
    switch (em->step) {
    case 0:
        em->step++;
        em->stateVar0 = 0;
        Vec_Copy((f32 *) &work->pitchStep, &em->posX);
        Em_SetMotion(em, 0x30, 0, 0);
        if (em->type != 0x43) {
            EffHermitaur26_Create(em, 0xA);
        }
        em->hideShadowHold = 2;
        em->hideShadow = 2;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->stateVar0++;
            if (em->stateVar0 > 4 || variant == 2) {
                Vec_Copy(&em->posX, (f32 *) &work->pitchStep);
                EmHermitaur_EndState2(em);
                break;
            }
            em->step++;
            em->timer = 4;
            Vec_Copy(&em->posX, (f32 *) &work->pitchStep);
            Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
            v[0] = EmHermitaur_State3_12Ofs[variant * 10 + em->stateVar0 * 2];
            v[1] = 0.0f;
            v[2] = EmHermitaur_State3_12Ofs[variant * 10 + em->stateVar0 * 2 + 1];
            ang[0] = 0;
            ang[1] = em->rotY;
            ang[2] = 0;
            Mtx_FromAnglesYXZ(ang, mtx);
            Vec_Rotate(out, v, mtx);
            em->velX = out[0] / (f32) em->timer;
            em->velZ = out[2] / (f32) em->timer;
            em->posX += out[0];
            em->posZ += out[2];
        }
        break;
    case 2:
        em->posX += em->velX;
        em->posZ += em->velZ;
        if (--em->timer <= 0) {
            em->step++;
            em->timer = 10;
            CamShake_ReqAIf(em, 4);
        }
        break;
    case 3:
        if (--em->timer <= 0) {
            em->step = 1;
            Em_SetMotion(em, 0x30, 0, 0);
            if (em->type != 0x43) {
                EffHermitaur26_Create(em, 0xA);
            }
            em->hideShadowHold = 2;
            em->hideShadow = 2;
        }
        break;
    }
}

void EmHermitaur_State3_16(Em *em, EmWork *work, s32 variant)
{
    if (em->type != 0x43) {
        EmHermitaur_EndState(em);
        return;
    }
    em->ceilingTimer = 5;
    em->posY = em->surfaceY;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x24, 0, 0);
        em->stance = 2;
        break;
    case 1:
        if (!EmMotion_ReachedFrame(em, 0, 16.0f) || EmMotion_ReachedFrame(em, 0, 210.0f)) {
            if (!(gScene.frame & 7)) {
                StageObj_K1C_CreateOnEmC(em, 0x22, 0, 1.0f);
            }
        } else if (!EmMotion_ReachedFrame(em, 0, 40.0f) || EmMotion_ReachedFrame(em, 0, 164.0f)) {
            if (gScene.frame % 12 == 0) {
                StageObj_K1C_CreateOnEmC(em, 0x22, 1, 1.0f);
            }
        }
        if (Player_PassedFrameScaled(em, 50.0f, 0) && em->variant == 0 && em->parts[2].state < 2) {
            EffEm33_Create(em, 0xA, 0, 0, 0x8000);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmHermitaur_EndState2To2_05(em);
        }
        break;
    }
}

void EmHermitaur_State3_17(Em *em, EmWork *work, s32 variant)
{
    EmHermitaurWork *cw = (EmHermitaurWork *)work;
    u32 speed;
    u32 d;

    if (em->type != 0x43) {
        EmHermitaur_EndState(em);
        return;
    }
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        cw->goalAngle = Math_AngleXZ(&em->posX, em->goal);
        if (((cw->goalAngle - em->rotY) & 0xFFFF) >= 0x8000U) {
            Em_SetMotion(em, 0x28, 0, 0);
        } else {
            Em_SetMotion(em, 0x29, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].blend == 0) {
            if (em->motion[0].playing == 0) {
                em->step++;
                EmHermitaur_EndState(em);
            } else {
                speed = 744.7273f * em->motionSpeed;
                d = (u16) (cw->goalAngle - *(u16 *) &em->rotY);
                if (!EmMotion_ReachedFrame(em, 0, 44.0f)) {
                    if ((u16) (d + speed) < speed * 2) {
                        em->rotY = cw->goalAngle;
                    } else if (d < 0x8000) {
                        em->rotY = (u16) (em->rotY + speed);
                    } else {
                        em->rotY = (u16) (em->rotY - speed);
                    }
                }
            }
        }
        break;
    case 2:
        EmHermitaur_EndState(em);
        break;
    }
    Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
}

void EmHermitaur_State3_1A(Em *em, s32 unused, s32 variant)
{
    if (em->type != 0x43) {
        EmHermitaur_EndState(em);
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            em->stance = 0;
            if (variant == 0) {
                Em_SetMotion(em, 0x2F, 0, 0);
                break;
            }
            Em_SetMotion(em, 0x31, 0, 0);
            break;
        case 1:
            if (em->motion[0].playing == 0) {
                em->step++;
                EmHermitaur_EndState(em);
                break;
            }
            if (EmMotion_InFrameRange(em, 0, 46.0f, 86.0f) != 0) {
                if (variant == 0) {
                    em->rotY -= 0x666;
                    break;
                }
                em->rotY += 0x666;
            } else {
            }
            break;
        }
    }
}

void EmHermitaur_RunGroup3(Em *em, EmWork *work)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->state) {
    case 0:
        EmHermitaur_State3_00(em);
        break;
    case 1:
        EmHermitaur_State3_01(em, (s32) work, 0);
        break;
    case 2:
        EmHermitaur_State3_01(em, (s32) work, 1);
        break;
    case 3:
        EmHermitaur_State3_03(em, work);
        break;
    case 4:
        EmHermitaur_State3_04(em, work, 0);
        break;
    case 5:
        EmHermitaur_State3_05(em);
        break;
    case 6:
        EmHermitaur_State3_01(em, (s32) work, 2);
        break;
    case 7:
        EmHermitaur_State3_07(em, work);
        break;
    case 8:
        EmHermitaur_State3_08(em, (s32) work, 0);
        break;
    case 9:
        EmHermitaur_State3_09(em);
        break;
    case 10:
        EmHermitaur_State3_0A(em, (s32) work, 0);
        break;
    case 11:
        EmHermitaur_State3_0A(em, (s32) work, 1);
        break;
    case 12:
        EmHermitaur_State3_0A(em, (s32) work, 2);
        break;
    case 13:
        EmHermitaur_State3_0D(em, (s32) work, 0);
        break;
    case 14:
        EmHermitaur_State3_0D(em, (s32) work, 1);
        break;
    case 15:
        EmHermitaur_State3_0F(em, (s32) work, 0);
        break;
    case 16:
        EmHermitaur_State3_0F(em, (s32) work, 1);
        break;
    case 17:
        EmHermitaur_State3_11(em, work);
        break;
    case 18:
        EmHermitaur_State3_12(em, work, 0);
        break;
    case 19:
        EmHermitaur_State3_08(em, (s32) work, 1);
        break;
    case 20:
        EmHermitaur_State3_12(em, work, 1);
        break;
    case 21:
        EmHermitaur_State3_12(em, work, 2);
        break;
    case 22:
        EmHermitaur_State3_16(em, work, 0);
        break;
    case 23:
        EmHermitaur_State3_17(em, work, 0);
        break;
    case 24:
        EmHermitaur_State3_17(em, work, 1);
        break;
    case 25:
        EmHermitaur_State3_04(em, work, 1);
        break;
    case 26:
        EmHermitaur_State3_1A(em, (s32) work, 0);
        break;
    case 27:
        EmHermitaur_State3_1A(em, (s32) work, 1);
        break;
    }
}

void EmHermitaur_State4_00(Em *em)
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
            Em_SetMotion(em, 0x4C, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmHermitaur_EndState(em);
        }
        break;
    }
}

void EmHermitaur_State4_01(Em *em)
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
            EmHermitaur_EndState(em);
        }
        break;
    }
}

void EmHermitaur_State4_02(Em *em, EmWork *work, s16 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        if (variant == 0) {
            Em_SetMotion(em, 0x3F, 0, 0);
        } else {
            Em_SetMotion(em, 0x47, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 0;
            if (variant == 0) {
                Em_SetMotion(em, 0x40, 0, 0);
            } else {
                Em_SetMotion(em, 0x48, 0, 0);
            }
        }
        break;
    case 2:
        em->timer++;
        if (em->timer >= (s16) (em->type == 0x43 ? 180 : 120)) {
            em->step++;
            if (variant == 0) {
                Em_SetMotion(em, 0x46, 0, 0);
            } else {
                Em_SetMotion(em, 0x49, 0, 0);
            }
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmHermitaur_EndState(em);
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
        }
        break;
    }
}

void EmHermitaur_State4_04(Em *em)
{
    Em_SetHitReactMode(em, 4U);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x41, 0, 0);
        em->noReact = 1;
        Em_RestartScript(em);
        em->stance = 0;
        break;
    case 1:
        Em_SpawnEffect06Bone(em, 2);
        if (--em->timer <= 0) {
            em->step++;
            EmHermitaur_ChangeState(em, 0, 0xA, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmHermitaur_ChangeState(em, 0, 0xA, 4);
        }
        break;
    }
}

void EmHermitaur_State4_05(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x45, 0, 0);
        Em_RestartScript(em);
        em->stance = 0;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmHermitaur_EndState(em);
            break;
        }
        if (EmMotion_InFrameRange(em, 0, 82.0f, 156.0f) != 0) {
            em->rotY += 0x376;
        }
        break;
    }
}

void EmHermitaur_State4_06(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x4A, 0, 0);
        Em_RestartScript(em);
        em->timer = 0;
        em->stance = 0;
        break;
    case 1:
        em->timer++;
        if (em->timer >= 0xB4) {
            em->step++;
            Em_SetMotion(em, 0x4B, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmHermitaur_EndState(em);
        }
        break;
    }
}

void EmHermitaur_State4_07(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x3C, 0, 0);
        Em_RestartScript(em);
        em->timer = 0;
        em->stance = 0;
        break;
    case 1:
        em->timer++;
        if (em->timer >= 0x14A) {
            em->step++;
            Em_SetMotion(em, 0x4C, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmHermitaur_EndState(em);
        }
        break;
    }
}

void EmHermitaur_State4_08(Em *em)
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
        if (EmMotion_ReachedFrame(em, 0, 40.0f) != 0) {
            em->step++;
            Em_SetMotion(em, 0x41, 0xC, 0);
            Player_ClearFlag(em, 0x20000);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            EmHermitaur_ChangeState(em, 0, 0xC, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmHermitaur_ChangeState(em, 0, 0xC, 4);
        }
        break;
    }
}

void EmHermitaur_State4_09(Em *em, s32 unused, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        if (variant == 0) {
            Em_SetMotion(em, 0x3F, 0, 0);
        } else {
            Em_SetMotion(em, 0x47, 0, 0);
        }
        StageObj_Stun_CreateEm(em, 1, 0x25, (s32) ((f32) em->timer + (em->motion[0].frameCount / 2.0f)), 150.0f, 3.0f);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            if (variant == 0) {
                Em_SetMotion(em, 0x40, 0, 0);
                break;
            }
            Em_SetMotion(em, 0x48, 0, 0);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            if (variant == 0) {
                Em_SetMotion(em, 0x46, 0, 0);
                break;
            }
            Em_SetMotion(em, 0x49, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmHermitaur_ChangeState(em, 0, 0xB, 4);
        }
        break;
    case 4:
        if (em->remote == 0) {
            EmHermitaur_ChangeState(em, 0, 0xB, 4);
        }
        break;
    }
}

void EmHermitaur_State4_0B(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x4A, 0, 0);
        Em_RestartScript(em);
        em->stance = 0;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmHermitaur_EndState(em);
        }
        break;
    }
}

void EmHermitaur_State4_0C(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 0x42, 0, 0);
        if (!(em->velY <= -50.0f)) {
            em->velY = -50.0f;
        }
        em->accelX = 0.0f;
        em->accelY = -10.0f;
        em->accelZ = 0.0f;
        break;
    case 1:
        Em_MoveRotatedAccel(em, &em->rotX);
        if (em->posY <= em->floorY) {
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
            Em_SetMotion(em, 0x48, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x49, 0, 0);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmHermitaur_EndState(em);
            Player_ClearVelocity(em);
        }
        break;
    }
}

void EmHermitaur_State4_0D(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x45, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmHermitaur_EndState(em);
            break;
        }
        if (EmMotion_InFrameRange(em, 0, 82.0f, 156.0f) != 0) {
            em->rotY += 0x376;
        }
        break;
    }
}

void EmHermitaur_RunGroup4(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmHermitaur_State4_00(em);
        break;
    case 1:
        EmHermitaur_State4_01(em);
        break;
    case 2:
        EmHermitaur_State4_02(em, work, 0);
        break;
    case 3:
        EmHermitaur_State4_02(em, work, 1);
        break;
    case 4:
        EmHermitaur_State4_04(em);
        break;
    case 5:
        EmHermitaur_State4_05(em);
        break;
    case 6:
        EmHermitaur_State4_06(em);
        break;
    case 7:
        EmHermitaur_State4_07(em);
        break;
    case 8:
        EmHermitaur_State4_08(em);
        break;
    case 9:
        EmHermitaur_State4_09(em, (s32) work, 0);
        break;
    case 10:
        EmHermitaur_State4_09(em, (s32) work, 1);
        break;
    case 11:
        EmHermitaur_State4_0B(em);
        break;
    case 12:
        EmHermitaur_State4_0C(em);
        break;
    case 13:
        EmHermitaur_State4_0D(em);
        break;
    case 14:
        EmHermitaur_State4_0B(em);
        break;
    case 15:
        EmHermitaur_State4_01(em);
        break;
    case 16:
        EmHermitaur_State4_01(em);
        break;
    }
}

void EmHermitaur_State5_00_Hunted(Em *em, EmWork *work)
{
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->stateVar0 = 0;
        Em_SetMotion(em, 0x44, 0, 0);
        em->lookAtTarget = 0;
        em->stance = 3;
        QuestEm_Hunted(em);
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 212.0f, 0) != 0 && em->type == 0x30) {
            CamShake_ReqAIf(em, 2);
        }
        if (Player_PassedFrameScaled(em, 290.0f, 0) != 0 && em->stateVar0 == 0) {
            em->stateVar0 = 1;
            EmMine_Create(em, 0);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            if (em->type == 0x42 || em->type == 0x49) {
                em->timer = 300;
            } else {
                em->timer = 1800;
            }
            em->fieldCollide = 0;
        }
        break;
    case 2:
        EmMine_Update(em);
        if (em->type == 0x42 || em->type == 0x49 || QuestEm_CanAppear(em) == 1) {
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
                EmHermitaur_ChangeState(em, 5, 1, 4);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    }
}

void EmHermitaur_State5_01(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        if (QuestEm_CanAppear(em) == 1) {
            Em_SetMotionRow(em, 0x22, 0, 0);
            EmHermitaur_ChangeState(em, 7, 0, 0);
            break;
        }
        em->taskPhase++;
        break;
    }
}

void EmHermitaur_RunGroup5(Em *em, EmWork *work)
{
    u8 type;

    type = em->type;
    if ((type == 0x42) || (type == 0x49)) {
        em->noHitTimer = 0xA;
        em->noPushTimer = 0xA;
    }
    switch (em->state) {
    case 0:
        EmHermitaur_State5_00_Hunted(em, work);
        break;
    case 1:
        EmHermitaur_State5_01(em, work);
        break;
    }
}

void EmHermitaur_State6_00(Em *em)
{
    f32 alpha;

    switch (em->step) {
    case 0:
        em->step++;
        break;
    case 1:
        alpha = em->alpha - 0.016666668f;
        em->alpha = alpha;
        if (alpha <= 0.0f) {
            QuestEm_Leave(em);
            em->taskPhase++;
            em->visible = 0;
        }
        break;
    }
}

void EmHermitaur_State6_01(Em *em)
{
    EmHermitaur_EndState(em);
}

void EmHermitaur_State6_02_DaimyoEntrance(Em *em)
{
    if (em->type != 0x30) {
        EmHermitaur_EndState(em);
        return;
    }
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stateVar0 = 0;
        em->stance = 0;
        em->stateArmor = 0;
        GmEm_ResetSize(em);
        Em_SetCombat(em, 1, 0);
        em->posX = 10335.0f;
        em->posY = 0.0f;
        em->posZ = 9788.0f;
        em->rotY = 0;
        em->hideShadowHold = 2;
        em->hideShadow = 2;
        em->stateArmor = 1;
        em->noPushTimer = 2;
        em->timer = 180;
        Em_SetMotion(em, 0x23, 0, 0);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x30, 0, 0);
            em->hideShadowHold = 2;
            em->hideShadow = 2;
        }
        break;
    case 2:
        switch (em->stateVar0) {
        case 0:
        case 1:
            if (EmMotion_ReachedFrame(em, 0, 100.0f)) {
                em->stateVar0++;
                Em_SetMotion(em, 0x30, 0, 0);
            }
            break;
        case 2:
            if (EmMotion_ReachedFrame(em, 0, 46.0f)) {
                em->step++;
                Em_SetMotion(em, 0x22, 0, 0);
            }
            break;
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x35, 0, 0);
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
            Em_SetMotion(em, 0x34, 0, 0);
        }
        break;
    case 6:
        if (Save_TestEventFlag(0x9A) == 1) {
            em->senseDelay = 0;
            em->noHitTimer = 0;
            EmHermitaur_EndState(em);
        }
        break;
    }
}

void EmHermitaur_State6_03_ShogunEntrance(Em *em)
{
    f32 d;

    if (em->type != 0x43) {
        EmHermitaur_EndState(em);
        return;
    }
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stateVar0 = 0;
        em->stance = 2;
        em->stateArmor = 0;
        GmEm_ResetSize(em);
        Em_SetCombat(em, 1, 0);
        em->posX = 10100.0f;
        em->posY = 1500.0f;
        em->posZ = 6580.0f;
        em->rotY = 0;
        em->goal[0] = 7600.0f;
        em->goal[1] = 1500.0f;
        em->goal[2] = 7253.0f;
        em->rotY = Math_AngleXZ(&em->posX, em->goal);
        em->surfaceY = 1500.0f;
        em->ceilingTimer = 5;
        Em_SetMotionRow(em, 0xC, 0, 0);
        em->timer = 300;
        break;
    case 1:
        em->ceilingTimer = 5;
        d = Vec_DistanceXZ(&em->posX, em->goal);
        em->rotY = Math_AngleXZ(&em->posX, em->goal);
        if (--em->timer <= 0 || d <= 150.0f) {
            em->step++;
            em->timer = 100;
            Em_SetMotion(em, 0x3B, 0, 0);
            Player_ClearVelocity(em);
            em->velY = -50.0f;
            em->accelY = -5.0f;
        }
        break;
    case 2:
        em->ceilingTimer = 5;
        if (Player_PassedFrameScaled(em, 108.0f, 0)) {
            em->step++;
            em->posY -= 400.0f;
            em->ceilingTimer = 0;
        }
        break;
    case 3:
        Em_MoveRotatedAccel(em, &em->rotX);
        if (em->posY <= em->floorY) {
            em->posY = em->floorY;
            em->step++;
            em->stance = 0;
            Em_SetMotion(em, 0xB, 0, 0);
            em->rotY = 0xD400;
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x34, 0, 0);
        }
        break;
    case 5:
        if (Player_PassedFrameScaled(em, 40.0f, 0)) {
            em->step++;
            Em_SetMotion(em, 0x34, 0, 0);
        }
        break;
    case 6:
        if (Player_PassedFrameScaled(em, 170.0f, 0)) {
            em->step++;
            Em_SetMotion(em, 0x2A, 0, 0);
        }
        break;
    case 7:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x28, 0, 0);
        }
        break;
    case 8:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x2C, 0, 0);
            em->timer = 40;
        }
        break;
    case 9:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x2B, 0, 0);
        }
        break;
    case 10:
        if (em->motion[0].playing == 0) {
            if (Save_TestEventFlag(0xA5) == 1) {
                em->senseDelay = 0;
                em->noHitTimer = 0;
                EmHermitaur_EndState(em);
                break;
            }
            em->step++;
            Em_SetMotion(em, 1, 0, 0);
        }
        break;
    case 11:
        if (Save_TestEventFlag(0xA5) == 1) {
            em->senseDelay = 0;
            em->noHitTimer = 0;
            EmHermitaur_EndState(em);
        }
        break;
    }
}

void EmHermitaur_State6_04_Captured(Em *em)
{
    s32 timer;
    s32 t;

    Em_SetCaptured(em, 1);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x21, 0, 0);
        em->timer = 0x708;
        Em_BeginStatus1(em);
        Em_ClearPoison(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x1F, 0, 0);
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
                EmHermitaur_ChangeState(em, 5, 1, 4);
                break;
            }
        } else {
            em->alpha = (f32) timer / 150.0f;
        }
        break;
    }
}

void EmHermitaur_RunGroup6(Em *em)
{
    switch (em->state) {
    case 0:
        EmHermitaur_State6_00(em);
        break;
    case 1:
        EmHermitaur_State6_01(em);
        break;
    case 2:
        EmHermitaur_State6_02_DaimyoEntrance(em);
        break;
    case 3:
        EmHermitaur_State6_03_ShogunEntrance(em);
        break;
    case 4:
        EmHermitaur_State6_04_Captured(em);
        break;
    }
}

void EmHermitaur_State7_00(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x22, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmHermitaur_EndState(em);
        }
        break;
    }
}

void EmHermitaur_RunGroup7(Em *em)
{
    switch (em->state) {
    case 0:
        EmHermitaur_State7_00(em);
        break;
    }
}

void EmHermitaur_State8_00(Em *em, EmWork *work)
{
    EmHermitaurVec v;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        if (Versus_DefeatCamera(em) == 1) {
            em->step++;
            Em_SetMotion(em, 0x37, 0, 0);
        }
        break;
    case 1:
        if (Player_PassedFrameScaled(em, 70.0f, 0) != 0) {
            v = EmHermitaur_CarveThrowPt;
            StageObj_K25_Create(&gPlayers[(u8) em->side - 1], em, 0, 0, &v.x);
        }
        if (em->motion[0].playing == 0) {
            Versus_RouteEmByTeam(em);
        }
        break;
    }
}

void EmHermitaur_State8_01(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    if (em->type == 0x30) {
        switch (em->step) {
        case 0:
            em->step++;
            Em_SetMotion(em, 0x28, 0, 0);
            break;
        case 1:
            if (em->motion[0].playing == 0) {
                em->step++;
                Em_SetMotion(em, 0x29, 0, 0);
            }
            break;
        case 2:
            if (em->motion[0].playing == 0) {
                em->step++;
                Em_SetMotion(em, 1, 0, 0);
                EmHermitaur_ChangeState(em, 8, 0x98, 1);
            }
            break;
        }
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            Em_SetMotion(em, 0x3D, 0, 0);
            break;
        case 1:
            if (em->motion[0].playing == 0) {
                em->step++;
                Em_SetMotion(em, 1, 0, 0);
                EmHermitaur_ChangeState(em, 8, 0x98, 1);
            }
            break;
        }
    }
}

void EmHermitaur_State8_02(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x34, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 1, 0, 0);
            EmHermitaur_ChangeState(em, 8, 0x98, 1);
        }
        break;
    }
}

void EmHermitaur_State8_03(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    if (em->type == 0x30) {
        switch (em->step) {
        case 0:
            em->step++;
            Em_SetMotion(em, 0x2B, 0, 0);
            break;
        case 1:
            if (em->motion[0].playing == 0) {
                em->step++;
                Em_SetMotion(em, 1, 0, 0);
                EmHermitaur_ChangeState(em, 8, 0x98, 1);
            }
            break;
        }
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            Em_SetMotion(em, 0x2F, 0, 0);
            break;
        case 1:
            if (em->motion[0].playing == 0) {
                em->step++;
                Em_SetMotion(em, 0x31, 0, 0);
            }
            break;
        case 2:
            if (em->motion[0].playing == 0) {
                em->step++;
                Em_SetMotion(em, 1, 0, 0);
                EmHermitaur_ChangeState(em, 8, 0x98, 1);
            }
            break;
        }
    }
}

void EmHermitaur_State8_98(Em *em, EmWork *work)
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
            EmHermitaur_ChangeState(em, 8, 0x99, 1);
        }
        break;
    }
}

void EmHermitaur_State8_99(Em *em, EmWork *work)
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
            EmHermitaur_EndState(em);
        }
        break;
    }
}

void EmHermitaur_RunGroup8(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0x0:
        EmHermitaur_State8_00(em, work);
        break;
    case 0x1:
        EmHermitaur_State8_01(em, work);
        break;
    case 0x2:
        EmHermitaur_State8_02(em, work);
        break;
    case 0x3:
        EmHermitaur_State8_03(em, work);
        break;
    case 0x98:
        EmHermitaur_State8_98(em, work);
        break;
    case 0x99:
        EmHermitaur_State8_99(em, work);
        break;
    }
}

void EmHermitaur_RunState(Em *em)
{
    u8 *w = &em->work;
    u8 stateGroup;

    stateGroup = em->stateGroup;
    switch (stateGroup) {
    case 0:
        EmHermitaur_RunGroup0(em, (EmWork *) w);
        break;
    case 1:
        EmHermitaur_RunGroup1(em, (EmWork *) w);
        break;
    case 2:
        EmHermitaur_RunGroup2(em, (s32) w);
        break;
    case 3:
        EmHermitaur_RunGroup3(em, (EmWork *) w);
        break;
    case 4:
        EmHermitaur_RunGroup4(em, (EmWork *) w);
        break;
    case 5:
        EmHermitaur_RunGroup5(em, (EmWork *) w);
        break;
    case 6:
        EmHermitaur_RunGroup6(em);
        break;
    case 7:
        EmHermitaur_RunGroup7(em);
        break;
    case 8:
        EmHermitaur_RunGroup8(em, (EmWork *) w);
        
    default:
        break;
    }
}

void EmHermitaur_ChangeState(Em *em, u16 group, u16 state, u8 mode)
{
    EmHermitaurWork *w = (EmHermitaurWork *)&em->work;
    f32 in[3];
    f32 out[3];
    s32 ang[3];
    f32 mtx[16];
    f32 f;
    u8 k;

    if (em->remote == 0) {
        Em_TargetPlayerInRange(em);
    } else if (mode == 3 || mode == 4) {
        return;
    }
    em->hitzoneSet = 0;
    switch (group) {
    case 0:
        switch (state) {
        case 1:
            if (em->combat == 1) {
                state = 2;
            }
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
        case 11:
            Em_EndStatus5(em);
            break;
        case 12:
            em->holdTimer2 = 0;
            break;
        case 13:
            Em_ResetNotice(em);
            break;
        case 14:
            Em_BeginStatus1(em);
            break;
        case 15:
            Em_EndStatus1(em);
            break;
        case 16:
            Em_RefillNeed3(em);
            break;
        case 17:
            Em_StartRage(em);
            break;
        case 18:
            Em_StartPendingRage(em);
            break;
        case 19:
            if (em->combat == 1) {
                state = 0x14;
            }
            break;
        case 22:
            Em_StartGoalAction(em, 10000);
            break;
        }
        break;
    case 1:
        switch (state) {
        case 0:
            if (em->combat == 1) {
                state = 0x14;
            }
            break;
        case 3:
            if (em->combat == 1) {
                state = 0x15;
            }
            break;
        case 8:
            if (em->combat == 1) {
                state = 0x16;
            }
            break;
        case 9:
            if (em->combat == 1) {
                state = 0x17;
            }
            break;
        case 1:
            if (em->combat == 1) {
                state = 0x12;
            }
            f = 300.0f;
            goto walk;
        case 4:
            if (em->combat == 1) {
                state = 0x13;
            }
            f = -400.0f;
            goto walk;
        case 5:
        case 19:
            f = -400.0f;
            goto walk;
        case 2:
        case 18:
            f = 300.0f;
            if (em->type == 0x42 || em->type == 0x49) {
                f *= 0.66f;
            }
            goto walk;
        case 24:
            f = -400.0f;
            goto walk;
        case 6:
        case 7:
        case 10:
        case 11:
            f = 450.0f;
            if (em->type == 0x42 || em->type == 0x49) {
                f = 200.0f;
            }
        walk:
            if (em->remote != 0) {
                if (em->curGoalKind == 0) {
                    w->unk037 = 0;
                    break;
                }
                w->unk037 = 1;
                Em_ComputeGoal2(em, em->goal);
                w->distLeft = Vec_DistanceXZ(&em->posX, em->goal);
                if (em->curGoalKind == 1 && em->curGoalArg == 0) {
                    if (em->targetNo == -1) {
                        w->unk037 = 0;
                        w->distLeft = 1000.0f;
                        break;
                    }
                    w->distLeft -= f;
                    if (w->distLeft <= 0.0f) {
                        w->distLeft = 0.0f;
                    }
                }
                if (em->curGoalKind == 7) {
                    w->distLeft -= f;
                    if (w->distLeft <= 0.0f) {
                        w->distLeft = 0.0f;
                    }
                }
                break;
            }
            if (em->curGoalKind == 0) {
                w->unk037 = 0;
                em->timer = (em->stateRand % 120 + 180) / 2;
                w->distLeft = 1000.0f;
                break;
            }
            w->unk037 = 1;
            w->distLeft = Vec_DistanceXZ(&em->posX, em->goal);
            if (em->curGoalKind == 1 && em->curGoalArg == 0) {
                if (em->targetNo == -1) {
                    w->unk037 = 0;
                    w->distLeft = 1000.0f;
                    break;
                }
                w->distLeft -= f;
                if (w->distLeft <= 0.0f) {
                    w->distLeft = 0.0f;
                }
            }
            if (em->curGoalKind == 7) {
                w->distLeft -= f;
                if (w->distLeft <= 0.0f) {
                    w->distLeft = 0.0f;
                }
            }
            if (em->type == 0x42 || em->type == 0x49) {
                em->timer = 500;
            } else {
                em->timer = (s32) w->distLeft / 7;
            }
            break;
        case 12:
        case 16:
            in[0] = 300.0f;
            goto turn;
        case 13:
        case 17:
            in[0] = -300.0f;
            goto turn;
        case 14:
            in[0] = 600.0f;
            goto turn;
        case 15:
            in[0] = -600.0f;
        turn:
            in[1] = 0.0f;
            in[2] = 0.0f;
            ang[0] = 0;
            ang[1] = em->rotY;
            ang[2] = 0;
            Mtx_FromAnglesYXZ(ang, mtx);
            Vec_Rotate(out, in, mtx);
            em->goal[0] = em->posX + out[0];
            em->goal[1] = em->posY + out[1];
            em->goal[2] = em->posZ + out[2];
            w->distLeft = Vec_DistanceXZ(&em->posX, em->goal);
            w->unk037 = 0;
            if (em->type == 0x42 || em->type == 0x49) {
                em->timer = 600;
            } else {
                em->timer = (s32) w->distLeft / 7;
            }
            break;
        }
        break;
    case 2:
        switch (state) {
        case 2:
            f = 0.0f;
            if (em->remote != 0) {
                if (em->curGoalKind == 0) {
                    w->unk037 = 0;
                    break;
                }
                w->unk037 = 1;
                Em_ComputeGoal2(em, em->goal);
                w->distLeft = Vec_DistanceXZ(&em->posX, em->goal);
                if (em->curGoalKind == 1 && em->curGoalArg == 0) {
                    if (em->targetNo == -1) {
                        w->unk037 = 0;
                        w->distLeft = 1000.0f;
                        break;
                    }
                    w->distLeft -= f;
                    if (w->distLeft <= 0.0f) {
                        w->distLeft = 0.0f;
                    }
                }
                if (em->curGoalKind == 7) {
                    w->distLeft -= f;
                    if (w->distLeft <= 0.0f) {
                        w->distLeft = 0.0f;
                    }
                }
                break;
            }
            if (em->curGoalKind == 0) {
                w->unk037 = 0;
                em->timer = (em->stateRand % 120 + 180) / 2;
                w->distLeft = 1000.0f;
                break;
            }
            w->unk037 = 1;
            w->distLeft = Vec_DistanceXZ(&em->posX, em->goal);
            if (em->curGoalKind == 1 && em->curGoalArg == 0) {
                if (em->targetNo == -1) {
                    w->unk037 = 0;
                    w->distLeft = 1000.0f;
                    break;
                }
                w->distLeft -= f;
                if (w->distLeft <= 0.0f) {
                    w->distLeft = 0.0f;
                }
            }
            if (em->curGoalKind == 7) {
                w->distLeft -= f;
                if (w->distLeft <= 0.0f) {
                    w->distLeft = 0.0f;
                }
            }
            if (em->type == 0x42 || em->type == 0x49) {
                em->timer = 600;
            } else {
                em->timer = (s32) w->distLeft / 7;
            }
            break;
        case 4:
            if (em->hasCeiling == 0 && em->remote == 0) {
                EmHermitaur_ChangeState(em, 0, 0x13, 1);
                return;
            }
            break;
        case 8:
        case 11:
            Player_ClearFlag(em, 0x20000);
            Em_SetSpeedScaled(em, 2, 1000.0f, 400.0f);
            break;
        case 9:
            if (em->remote == 0) {
                if (em->parts[2].state < 2) {
                    EmHermitaur_ChangeState(em, 2, 3, 1);
                    return;
                }
                switch (em->variant) {
                case 0:
                    if (em->stateRand % 100 < 76) {
                        k = 2;
                    } else {
                        k = 1;
                    }
                    break;
                case 1:
                    if (em->stateRand % 100 < 51) {
                        k = 0;
                    } else {
                        k = 2;
                    }
                    break;
                case 2:
                    if (em->stateRand % 100 < 61) {
                        k = 0;
                    } else {
                        k = 1;
                    }
                    break;
                }
                switch (k) {
                case 0:
                    EmHermitaur_ChangeState(em, 2, 12, 1);
                    return;
                case 1:
                    EmHermitaur_ChangeState(em, 2, 13, 1);
                    return;
                case 2:
                    EmHermitaur_ChangeState(em, 2, 14, 1);
                    return;
                }
            }
            break;
        case 12:
            em->variant = 0;
            goto reset;
        case 13:
            em->variant = 1;
            goto reset;
        case 14:
            em->variant = 2;
        reset:
            em->parts[2].state = 0;
            w->lastPartState = em->parts[2].state;
            em->fxFlags &= 0xFE;
            Em_ResetPartHp(em, 2);
            break;
        }
        break;
    case 3:
        switch (state) {
        case 17:
            em->scriptPending = 0;
            em->senseDelay = 5;
            if (em->remote == 0) {
                switch (em->type) {
                case 0x49:
                case 0x42:
                    em->timer = (Rand_Next(1) % 20) * 30 + 600;
                    break;
                case 0x43:
                case 0x30:
                    em->timer = (Rand_Next(1) % 5) * 30 + 300;
                    break;
                }
            }
            break;
        case 18:
            break;
        }
        break;
    case 4:
        switch (state) {
        case 4:
            Em_BeginStatus4(em);
            break;
        case 8:
            Em_EndAllStatus(em);
            break;
        case 9:
        case 10:
            Em_BeginStatus5(em);
            break;
        case 13:
            em->hit8Count++;
            if (em->hit8Count >= 2) {
                em->fxFlags |= 1;
                if (em->type == 0x30) {
                    Em_BreakPart(em, 1);
                }
            }
            break;
        case 5:
        case 6:
        case 7:
        case 16:
            break;
        }
        break;
    case 5:
        if (state == 1 && QuestEm_CanAppear(em) == 1) {
            QuestEm_Appear(em);
        }
        break;
    case 8:
        if (state == 0x98) {
            Versus_MarkDefeat(em);
        }
        em->motionSpeed = 1.0f;
        break;
    }
    Em_SetStateSync(em, group, state, mode);
}

void EmHermitaur_EndState(Em *em)
{
    em->motionSpeed = 1.0f;
    em->stance = 0;
    em->lookAtTarget = 0;
    em->scriptPending = 1;
    em->fieldCollide = 1;
    EmHermitaur_ChangeState(em, 0, 1, 0);
}

void EmHermitaur_EndStateTo0_02(Em *em)
{
    em->motionSpeed = 1.0f;
    em->stance = 0;
    em->lookAtTarget = 0;
    em->scriptPending = 1;
    em->fieldCollide = 1;
    EmHermitaur_ChangeState(em, 0, 2, 0);
}

void EmHermitaur_EndState2(Em *em)
{
    em->motionSpeed = 1.0f;
    em->lookAtTarget = 0;
    em->scriptPending = 1;
    em->stateArmor = 1;
    em->fieldCollide = 1;
    EmHermitaur_ChangeState(em, 2, 3, 0);
}

void EmHermitaur_EndState2To2_05(Em *em)
{
    em->motionSpeed = 1.0f;
    em->stance = 2;
    em->lookAtTarget = 0;
    em->scriptPending = 1;
    em->ceilingTimer = 5;
    em->fieldCollide = 1;
    EmHermitaur_ChangeState(em, 2, 5, 0);
}

void EmHermitaur_SpawnEffects(Em *em)
{
    u8 type;

    Effect01_Spawn((Player *) &em->active, 0);
    type = em->type;
    if ((type == 0x30) || (type == 0x43)) {
        StageObj_Gun_CreateType5(em);
    }
}

void EmHermitaur_TableNop(void)
{
}

void EmHermitaur_SpawnGroundEffect(Em *em, s32 a1, s32 a2, f32 scale)
{
    f32 pos[3];

    if (Player_InCurrentArea(em)) {
        Em_BonePos(em, a1, pos);
        pos[1] = em->floorY;
        Effect0D_SpawnOnGround(pos, a2, scale * em->scaleX);
    }
}

void EmHermitaur_MotionEffectsDefault(Em *em)
{
}

void EmHermitaur_MotionEffects(Em *em, EmWork *work)
{
    f32 v[3];

    if (em->motionId[0] != work->fxMotion) {
        work->fxMotion = em->motionId[0];
    }
    EmHermitaur_MotionSounds(em, work);
    switch (work->fxMotion) {
    case 0x3E9:
    case 0x3EA:
    case 0x3EB:
    case 0x3ED:
    case 0x3EE:
    case 0x3EF:
    case 0x3F0:
        break;
    case 0x3F2:
        if (em->type == 0x43) {
            if (Player_PassedFrameScaled(em, 112.0f, 0)) {
                StageObj_K14_Create(em, 0xB, 0x15, em->scaleX);
            }
        } else if (Player_PassedFrameScaled(em, 98.0f, 0)) {
            StageObj_K14_Create(em, 0x13, 0x15, em->scaleX);
        }
        break;
    case 0x3F3:
        if (Player_PassedFrameScaled(em, 8.0f, 0)) {
            EmHermitaur_SpawnGroundEffect(em, 1, 0x28, 2.0f * em->scaleX);
            EffEm07_CreateAt(em, &em->posX, 5);
            EffEm07_CreateAt(em, &em->posX, 6);
        }
        if (em->type == 0x43 && Player_PassedFrameScaled(em, 190.0f, 0)) {
            EmHermitaur_SpawnGroundEffect(em, 6, 4, 6.0f);
            EmHermitaur_SpawnGroundEffect(em, 0xA, 4, 6.0f);
        }
        break;
    case 0x3F7:
    case 0x3F8:
    case 0x3FA:
        break;
    case 0x3FB:
        if (Player_PassedFrameScaled(em, 14.0f, 0)) {
            switch (em->type) {
            case 0x49:
            case 0x42:
                EffHermitaur26_Create(em, 4);
                break;
            case 0x43:
                EffHermitaur26_Create(em, 0x17);
                break;
            case 0x30:
                EffHermitaur26_Create(em, 0xD);
                break;
            }
        }
        if (Player_PassedFrameScaled(em, 22.0f, 0)) {
            StageObj_K14_Create(em, 2, 0x23, em->scaleX);
        }
        break;
    case 0x3FC:
        break;
    case 0x3FD:
        if (Player_PassedFrameScaled(em, 14.0f, 0)) {
            switch (em->type) {
            case 0x49:
            case 0x42:
                EffHermitaur26_Create(em, 6);
                break;
            case 0x43:
                EffHermitaur26_Create(em, 0x15);
                break;
            case 0x30:
                EffHermitaur26_Create(em, 0xF);
                break;
            }
        }
        if (Player_PassedFrameScaled(em, 20.0f, 0)) {
            StageObj_K14_Create(em, 2, 0x23, em->scaleX);
        }
        break;
    case 0x3FE:
        break;
    case 0x3FF:
        if (Player_PassedFrameScaled(em, 14.0f, 0)) {
            switch (em->type) {
            case 0x49:
            case 0x42:
                EffHermitaur26_Create(em, 7);
                break;
            case 0x43:
                EffHermitaur26_Create(em, 0x16);
                break;
            case 0x30:
                EffHermitaur26_Create(em, 0x10);
                break;
            }
        }
        if (Player_PassedFrameScaled(em, 20.0f, 0)) {
            StageObj_K14_Create(em, 3, 0x24, em->scaleX);
        }
        break;
    case 0x400:
        break;
    case 0x401:
        if (Player_PassedFrameScaled(em, 14.0f, 0)) {
            switch (em->type) {
            case 0x49:
            case 0x42:
                EffHermitaur26_Create(em, 5);
                break;
            case 0x43:
                EffHermitaur26_Create(em, 0x18);
                break;
            case 0x30:
                EffHermitaur26_Create(em, 0xE);
                break;
            }
        }
        if (Player_PassedFrameScaled(em, 22.0f, 0)) {
            StageObj_K14_Create(em, 3, 0x24, em->scaleX);
        }
        break;
    case 0x407:
        v[0] = 0.0f;
        v[1] = 20.0f;
        v[2] = 50.0f;
        Em_SpawnEffect06Periodic(em, 0x1F, v, 1.6f);
        break;
    case 0x40A:
        if (em->type == 0x42 || em->type == 0x49) {
            if (Player_PassedFrameScaled(em, 20.0f, 0) || Player_PassedFrameScaled(em, 92.0f, 0)) {
                EmHermitaur_SpawnGroundEffect(em, 0xC, 7, 6.0f);
            }
            if (Player_PassedFrameScaled(em, 16.0f, 0)) {
                EffHermitaur26_Create(em, 8);
            }
            if (!EmMotion_ReachedFrame(em, 0, 120.0f)) {
                em->hideShadowHold = 2;
                em->hideShadow = 2;
            }
            break;
        }
        if (em->type == 0x43) {
            if (Player_PassedFrameScaled(em, 18.0f, 0)) {
                EffHermitaur26_Create(em, 0x13);
            }
            if (Player_PassedFrameScaled(em, 22.0f, 0)) {
                EffHermitaur26_Create(em, 0x14);
            }
            if (Player_PassedFrameScaled(em, 110.0f, 0)) {
                EffHermitaur26_Create(em, 0x1C);
            }
            if (Player_PassedFrameScaled(em, 128.0f, 0)) {
                EffHermitaur26_Create(em, 0x1D);
            }
        }
        if (Player_PassedFrameScaled(em, 2.0f, 0)) {
            EmHermitaur_SpawnGroundEffect(em, 0xC, 7, 6.0f);
        }
        if (Player_PassedFrameScaled(em, 84.0f, 0)) {
            StageObj_K14_Create(em, 0xB, 0x18, em->scaleX);
        }
        break;
    case 0x40B:
        em->hideShadowHold = 2;
        em->hideShadow = 2;
        if (em->type == 0x42 || em->type == 0x49) {
            if ((gScene.frame & 0x7F) == 0 && (Rand_Next(1) & 3)) {
                Effect0D_SpawnOnGround(&em->posX, 4, 8.0f * em->scaleX);
            }
        }
        break;
    case 0x410:
        if (em->type == 0x43) {
            if (Player_PassedFrameScaled(em, 72.0f, 0)) {
                StageObj_K14_Create(em, 2, 0x23, em->scaleX);
            }
            if (Player_PassedFrameScaled(em, 66.0f, 0)) {
                EffHermitaur26_Create(em, 0x19);
            }
        }
        break;
    case 0x411:
        if (em->type == 0x43) {
            if (Player_PassedFrameScaled(em, 72.0f, 0)) {
                StageObj_K14_Create(em, 3, 0x24, em->scaleX);
            }
            if (Player_PassedFrameScaled(em, 66.0f, 0)) {
                EffHermitaur26_Create(em, 0x1A);
            }
        }
        break;
    case 0x412:
        if (Player_PassedFrameScaled(em, 44.0f, 0)) {
            if (em->type == 0x43) {
                EffHermitaur26_Create(em, 0x11);
            } else {
                EffHermitaur26_Create(em, 1);
            }
        }
        if (em->type == 0x43) {
            if (Player_PassedFrameScaled(em, 48.0f, 0)) {
                StageObj_K14_Create(em, 2, 0x16, em->scaleX);
            }
            if (Player_PassedFrameScaled(em, 56.0f, 0)) {
                StageObj_K14_Create(em, 3, 0x17, em->scaleX);
            }
        }
        break;
    case 0x413:
        if (Player_PassedFrameScaled(em, 18.0f, 0)) {
            switch (em->type) {
            case 0x49:
            case 0x42:
                EffHermitaur26_Create(em, 2);
                break;
            case 0x43:
                EffHermitaur26_Create(em, 0x12);
                break;
            case 0x30:
                EffHermitaur26_Create(em, 3);
                break;
            }
        }
        if (Player_PassedFrameScaled(em, 32.0f, 0)) {
            EmHermitaur_SpawnGroundEffect(em, 0xB, 7, 6.0f);
        }
        break;
    case 0x414:
    case 0x415:
    case 0x416:
        break;
    case 0x417:
        if (Player_PassedFrameScaled(em, 76.0f, 0)) {
            EffHermitaur26_Create(em, 0x1E);
        }
        if (Player_PassedFrameScaled(em, 84.0f, 0)) {
            StageObj_K14_Create(em, 3, 0x24, em->scaleX);
        }
        break;
    case 0x418:
        if (Player_PassedFrameScaled(em, 14.0f, 0)) {
            EmHermitaur_SpawnGroundEffect(em, 0xB, 7, 6.0f);
        }
        if (!EmMotion_ReachedFrame(em, 0, 16.0f)) {
            em->hideShadowHold = 2;
            em->hideShadow = 2;
        } else if (EmMotion_ReachedFrame(em, 0, 66.0f)) {
            em->hideShadowHold = 2;
            em->hideShadow = 2;
        }
        if (em->type == 0x43) {
            if (Player_PassedFrameScaled(em, 18.0f, 0)) {
                EffHermitaur26_Create(em, 0x13);
            }
            if (Player_PassedFrameScaled(em, 22.0f, 0)) {
                EffHermitaur26_Create(em, 0x14);
            }
        }
        break;
    case 0x419:
        if (Player_PassedFrameScaled(em, 76.0f, 0)) {
            EffHermitaur26_Create(em, 0x1F);
        }
        if (Player_PassedFrameScaled(em, 84.0f, 0)) {
            StageObj_K14_Create(em, 2, 0x23, em->scaleX);
        }
        break;
    case 0x41A:
        if (em->type == 0x43 && Player_PassedFrameScaled(em, 50.0f, 0)) {
            StageObj_K14_Create(em, 0xB, 0x25, 0.5f * em->scaleX);
        }
        break;
    case 0x41B:
    case 0x41C:
        break;
    case 0x421:
        if (Player_PassedFrameScaled(em, 34.0f, 0)) {
            EmHermitaur_SpawnGroundEffect(em, 0xB, 7, 6.0f);
        }
        if (Player_PassedFrameScaled(em, 64.0f, 0)) {
            StageObj_K14_Create(em, 0xB, 0x15, em->scaleX);
        }
        if (EmMotion_ReachedFrame(em, 0, 130.0f)) {
            em->hideShadowHold = 2;
            em->hideShadow = 2;
        }
        break;
    case 0x422:
    case 0x424:
    case 0x425:
        break;
    case 0x427:
        if (Player_PassedFrameScaled(em, 28.0f, 0)) {
            EmHermitaur_SpawnGroundEffect(em, 0xB, 7, 6.0f);
        }
        break;
    case 0x42B:
        if (Player_PassedFrameScaled(em, 2.0f, 0)) {
            StageObj_K14_Create(em, 0xB, 0x15, em->scaleX);
        }
        break;
    case 0x42C:
        if (Player_PassedFrameScaled(em, 158.0f, 0)) {
            StageObj_K14_Create(em, 0, 0, 0.9f * em->scaleX);
        }
        break;
    case 0x42D:
        if (Player_PassedFrameScaled(em, 66.0f, 0)) {
            EmHermitaur_SpawnGroundEffect(em, 0xC, 7, 6.0f);
        }
        break;
    case 0x42F:
        if (Player_PassedFrameScaled(em, 28.0f, 0)) {
            EmHermitaur_SpawnGroundEffect(em, 0xC, 7, 6.0f);
        }
        break;
    case 0x432:
        if (em->type == 0x30 || em->type == 0x42 || em->type == 0x49) {
            if (Player_PassedFrameScaled(em, 122.0f, 0)) {
                EmHermitaur_SpawnGroundEffect(em, 0xC, 7, 6.0f);
            }
        }
        break;
    default:
        EmHermitaur_MotionEffectsDefault(em);
        break;
    }
}

void EmHermitaur_UpdateMotionEffects(Em *em)
{
    EmHermitaurWork *w = (EmHermitaurWork *)&em->work;

    switch (w->fxStep) {
    case 0:
        w->fxStep++;
        break;
    case 1:
        EmHermitaur_MotionEffects(em, (EmWork *) w);
        break;
    }
}

s32 EmHermitaur_ScriptQuery(Em *em, s32 kind)
{
    EmHermitaurWork *w = (EmHermitaurWork *)&em->work;

    s32 k;

    k = kind & 0xFF;
    switch (k) {
    case 1:
        return em->stateArmor != 0;
    case 2:
        if ((em->type == 0x43) && (em->hasCeiling == 0)) {
            return 0;
        }
        return 1;
    case 3:
        if ((em->type == 0x43) && (((s32) em->parts[2].state < 2) || ((s32) w->unk04F >= 2))) {
            return 0;
        }
        return 1;
    case 4:
        if ((em->type == 0x43) && (em->variant == 0)) {
            return 1;
        }
        return 0;
    default:
        return 0;
    }
}

s32 func_00761C70(Em *em)
{
    return em->unk471 != 0;
}

s32 EmHermitaur_InSpecialArea(u16 area)
{
    switch (Area_DayId(area) & 0xFFFF) {
    case 0x9A:
    case 0x9B:
    case 0x9F:
    case 0xA6:
        return 1;
    }
    return 0;
}

s32 EmHermitaur_AllowHit8(Em *em)
{
    if (em->type != 0x30) {
        return 0;
    }
    if ((em->stance != 2) && (em->stateArmor == 0) && ((s8) em->hit8Count < 2)) {
        return 1;
    }
    return 0;
}

void EmGravios_PlaySeOnFrame(Em *em, s32 frame, s32 se, s32 bone)
{
    s32 seOfs;

    seOfs = 0;
    if (Player_PassedFrameScaled(em, (f32) frame, 0U) != 0) {
        if (*(u8 *)((u8 *)em + 0x453) == 1) {
            if (bone != 0x14) {
                if (bone == 0x1A) {
                    goto block_5;
                }
            } else {
block_5:
                seOfs = 0x35;
            }
        }
        Em_PlaySeAtBone(em, se + seOfs, bone);
    }
}

void EmGravios_MotionSounds(Em *em, EmWork *work)
{
    s16 motion;
    u8 type;
    u8 stateGroup;

    motion = work->sndMotion;
    switch (motion) {
    case 0x3E9:
        break;
    case 0x3EA:
        EmGravios_PlaySeOnFrame(em, 0x2C, 1, 0x14);
        EmGravios_PlaySeOnFrame(em, 0x74, 1, 0x1A);
        break;
    case 0x3ED:
        EmGravios_PlaySeOnFrame(em, 6, 0x48, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x19, 1, 0x14);
        EmGravios_PlaySeOnFrame(em, 0x38, 1, 0x1A);
        break;
    case 0x3EE:
        EmGravios_PlaySeOnFrame(em, 6, 0x48, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x19, 1, 0x1A);
        EmGravios_PlaySeOnFrame(em, 0x38, 1, 0x14);
        break;
    case 0x3F2:
        EmGravios_PlaySeOnFrame(em, 6, 0x53, 0x23);
        EmGravios_PlaySeOnFrame(em, 6, 4, 0x1A);
        EmGravios_PlaySeOnFrame(em, 0x4C, 0, 0x14);
        EmGravios_PlaySeOnFrame(em, 0x1C, 0x12, 0x14);
        break;
    case 0x3F3:
        EmGravios_PlaySeOnFrame(em, 0xE, 0xB, 6);
        EmGravios_PlaySeOnFrame(em, 0x12, 0xB, 0xC);
        EmGravios_PlaySeOnFrame(em, 0x46, 0xB, 6);
        EmGravios_PlaySeOnFrame(em, 0x42, 0xB, 0xC);
        EmGravios_PlaySeOnFrame(em, 0x7A, 0xB, 6);
        EmGravios_PlaySeOnFrame(em, 0x7E, 0xB, 0xC);
        break;
    case 0x3F5:
        EmGravios_PlaySeOnFrame(em, 0xE, 0xF, 6);
        EmGravios_PlaySeOnFrame(em, 0x12, 0xF, 0xC);
        EmGravios_PlaySeOnFrame(em, 0x2C, 0xF, 6);
        EmGravios_PlaySeOnFrame(em, 0x28, 0xF, 0xC);
        EmGravios_PlaySeOnFrame(em, 0x42, 0xF, 6);
        EmGravios_PlaySeOnFrame(em, 0x46, 0xF, 0xC);
        break;
    case 0x3F7:
        EmGravios_PlaySeOnFrame(em, 4, 0xC, 6);
        EmGravios_PlaySeOnFrame(em, 8, 0xC, 0xC);
        EmGravios_PlaySeOnFrame(em, 0x3A, 0xB, 6);
        EmGravios_PlaySeOnFrame(em, 0x3E, 0xB, 0xC);
        EmGravios_PlaySeOnFrame(em, 0x8E, 0xB, 6);
        EmGravios_PlaySeOnFrame(em, 0x8A, 0xB, 0xC);
        EmGravios_PlaySeOnFrame(em, 0xD4, 0xB, 6);
        EmGravios_PlaySeOnFrame(em, 0xD8, 0xB, 0xC);
        break;
    case 0x3F8:
        break;
    case 0x3F9:
        EmGravios_PlaySeOnFrame(em, 6, 0x4F, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x1C, 1, 0x14);
        EmGravios_PlaySeOnFrame(em, 0xA0, 6, 0x14);
        EmGravios_PlaySeOnFrame(em, 0xC0, 6, 0x1A);
        break;
    case 0x3FA:
        EmGravios_PlaySeOnFrame(em, 0x60, 1, 0x14);
        EmGravios_PlaySeOnFrame(em, 0x16, 1, 0x14);
        EmGravios_PlaySeOnFrame(em, 4, 0x23, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x50, 0xF, 6);
        EmGravios_PlaySeOnFrame(em, 0x54, 0xF, 0xC);
        EmGravios_PlaySeOnFrame(em, 0x62, 7, 0x1A);
        EmGravios_PlaySeOnFrame(em, 0xA8, 0xB, 6);
        EmGravios_PlaySeOnFrame(em, 0xAC, 0xB, 0xC);
        EmGravios_PlaySeOnFrame(em, 0xDE, 0xB, 6);
        EmGravios_PlaySeOnFrame(em, 0xE2, 0xB, 0xC);
        EmGravios_PlaySeOnFrame(em, 0x110, 0xB, 6);
        EmGravios_PlaySeOnFrame(em, 0x114, 0xB, 0xC);
        break;
    case 0x3FB:
        EmGravios_PlaySeOnFrame(em, 2, 0xB, 6);
        EmGravios_PlaySeOnFrame(em, 6, 0xB, 0xC);
        EmGravios_PlaySeOnFrame(em, 0xC, 4, 0x1A);
        EmGravios_PlaySeOnFrame(em, 4, 1, 0x14);
        EmGravios_PlaySeOnFrame(em, 0xA8, 3, 0x14);
        EmGravios_PlaySeOnFrame(em, 0xC8, 0, 0x1A);
        EmGravios_PlaySeOnFrame(em, 0xE, 9, 0);
        EmGravios_PlaySeOnFrame(em, 0x7C, 0x16, 0);
        break;
    case 0x405:
        EmGravios_PlaySeOnFrame(em, 0x1E, 0x13, 0);
        EmGravios_PlaySeOnFrame(em, 2, 0x48, 0x23);
        EmGravios_PlaySeOnFrame(em, 4, 0xE, 0xC);
        EmGravios_PlaySeOnFrame(em, 8, 0xE, 6);
        break;
    case 0x406:
        break;
    case 0x40A:
        EmGravios_PlaySeOnFrame(em, 0x24, 0x57, 0x23);
        break;
    case 0x40B:
        EmGravios_PlaySeOnFrame(em, 0x32, 2, 0x14);
        EmGravios_PlaySeOnFrame(em, 0x3A, 6, 0x1A);
        EmGravios_PlaySeOnFrame(em, 0x54, 1, 0x1A);
        EmGravios_PlaySeOnFrame(em, 0x68, 4, 0x14);
        EmGravios_PlaySeOnFrame(em, 0x7C, 0x11, 0x1A);
        EmGravios_PlaySeOnFrame(em, 0xCC, 6, 0x1A);
        EmGravios_PlaySeOnFrame(em, 0xD2, 2, 0x1A);
        EmGravios_PlaySeOnFrame(em, 0xFE, 0x12, 0x14);
        EmGravios_PlaySeOnFrame(em, 0x120, 2, 0x1A);
        EmGravios_PlaySeOnFrame(em, 0x20, 0xF, 0xC);
        EmGravios_PlaySeOnFrame(em, 0x24, 0xF, 6);
        EmGravios_PlaySeOnFrame(em, 0x80, 0xF, 0xC);
        EmGravios_PlaySeOnFrame(em, 0x84, 0xF, 6);
        EmGravios_PlaySeOnFrame(em, 0x122, 0xE, 0xC);
        EmGravios_PlaySeOnFrame(em, 0x126, 0xE, 6);
        EmGravios_PlaySeOnFrame(em, 2, 0x15, 0);
        EmGravios_PlaySeOnFrame(em, 2, 0x23, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x30, 0x21, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x80, 0x22, 0x23);
        EmGravios_PlaySeOnFrame(em, 0xB8, 0x24, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x120, 0x22, 0x23);
        break;
    case 0x40C:
        EmGravios_PlaySeOnFrame(em, 0x24, 0x24, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x14, 0, 0x1A);
        EmGravios_PlaySeOnFrame(em, 0x28, 0, 0x14);
        EmGravios_PlaySeOnFrame(em, 0x3C, 0x15, 0x22);
        EmGravios_PlaySeOnFrame(em, 0x3C, 0xC, 0xC);
        EmGravios_PlaySeOnFrame(em, 0xC, 0xC, 6);
        EmGravios_PlaySeOnFrame(em, 0xA6, 0, 0x1A);
        break;
    case 0x40D:
        EmGravios_PlaySeOnFrame(em, 0x1E, 0x48, 0x23);
        EmGravios_PlaySeOnFrame(em, 0xC, 0xC, 0x1A);
        EmGravios_PlaySeOnFrame(em, 0x1C, 2, 0x1A);
        EmGravios_PlaySeOnFrame(em, 0x2E, 3, 0x14);
        EmGravios_PlaySeOnFrame(em, 0x48, 1, 0x1A);
        break;
    case 0x40E:
        EmGravios_PlaySeOnFrame(em, 4, 0x57, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x48, 0x16, 0);
        EmGravios_PlaySeOnFrame(em, 0xA4, 9, 0);
        EmGravios_PlaySeOnFrame(em, 0x90, 3, 0xC);
        EmGravios_PlaySeOnFrame(em, 0x88, 0xE, 6);
        EmGravios_PlaySeOnFrame(em, 0xB2, 4, 0x22);
        break;
    case 0x40F:
        EmGravios_PlaySeOnFrame(em, 2, 0x57, 0x23);
        EmGravios_PlaySeOnFrame(em, 2, 0x17, 0);
        EmGravios_PlaySeOnFrame(em, 0x40, 3, 0x1A);
        EmGravios_PlaySeOnFrame(em, 0x66, 0x10, 0x1A);
        break;
    case 0x412:
        EmGravios_PlaySeOnFrame(em, 8, 0x23, 0x23);
        EmGravios_PlaySeOnFrame(em, 0xE, 0xF, 0xC);
        EmGravios_PlaySeOnFrame(em, 0x1E, 0x14, 0x2A);
        EmGravios_PlaySeOnFrame(em, 0x2A, 0, 0x14);
        EmGravios_PlaySeOnFrame(em, 0x48, 1, 0x1A);
        break;
    case 0x413:
        EmGravios_PlaySeOnFrame(em, 8, 0x23, 0x23);
        EmGravios_PlaySeOnFrame(em, 0xE, 0xF, 6);
        EmGravios_PlaySeOnFrame(em, 0x1E, 0x14, 0x2A);
        EmGravios_PlaySeOnFrame(em, 0x2A, 0, 0x1A);
        EmGravios_PlaySeOnFrame(em, 0x48, 1, 0x14);
        break;
    case 0x414:
        EmGravios_PlaySeOnFrame(em, 0x1C, 0xB, 6);
        EmGravios_PlaySeOnFrame(em, 0x20, 0xB, 0xC);
        EmGravios_PlaySeOnFrame(em, 6, 0x1A, 0x22);
        EmGravios_PlaySeOnFrame(em, 0xA, 0x1B, 0xC);
        EmGravios_PlaySeOnFrame(em, 0xE, 0x1B, 6);
        EmGravios_PlaySeOnFrame(em, 0x26, 0x1A, 0x22);
        EmGravios_PlaySeOnFrame(em, 0x2A, 0x1B, 0xC);
        EmGravios_PlaySeOnFrame(em, 0x32, 0x1B, 6);
        break;
    case 0x417:
        EmGravios_PlaySeOnFrame(em, 2, 0x12, 0);
        EmGravios_PlaySeOnFrame(em, 0x50, 0x18, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x14, 0x28, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x4A, 0x29, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x4C, 0xD, 6);
        EmGravios_PlaySeOnFrame(em, 0x48, 0xD, 0xC);
        EmGravios_PlaySeOnFrame(em, 0xB4, 0x13, 6);
        EmGravios_PlaySeOnFrame(em, 0xBB, 0x13, 0xC);
        EmGravios_PlaySeOnFrame(em, 0xF0, 0x12, 0);
        break;
    case 0x418:
        EmGravios_PlaySeOnFrame(em, 2, 0x22, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x22, 0x22, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x40, 0x22, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x72, 0x20, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x10, 1, 0x14);
        break;
    case 0x419:
        EmGravios_PlaySeOnFrame(em, 0x20, 0xF, 6);
        EmGravios_PlaySeOnFrame(em, 0x24, 0xF, 0xC);
        EmGravios_PlaySeOnFrame(em, 0x74, 0xB, 6);
        EmGravios_PlaySeOnFrame(em, 0x78, 0xB, 0xC);
        EmGravios_PlaySeOnFrame(em, 0xBE, 0xB, 6);
        EmGravios_PlaySeOnFrame(em, 0xC2, 0xB, 0xC);
        break;
    case 0x41A:
        EmGravios_PlaySeOnFrame(em, 0x4E, 0x29, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x4E, 0x18, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x78, 0x18, 0x23);
        EmGravios_PlaySeOnFrame(em, 0xA4, 0x18, 0x23);
        EmGravios_PlaySeOnFrame(em, 4, 0x12, 0x1A);
        EmGravios_PlaySeOnFrame(em, 4, 0x13, 0x22);
        EmGravios_PlaySeOnFrame(em, 0x40, 0xF, 6);
        EmGravios_PlaySeOnFrame(em, 0x40, 0xF, 0xC);
        EmGravios_PlaySeOnFrame(em, 0xFA, 0x20, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x12A, 0x12, 0x14);
        EmGravios_PlaySeOnFrame(em, 0x11E, 0x13, 0);
        break;
    case 0x41B:
        EmGravios_PlaySeOnFrame(em, 4, 0x13, 0x2A);
        EmGravios_PlaySeOnFrame(em, 0x26, 0x10, 0x1A);
        EmGravios_PlaySeOnFrame(em, 0x38, 0x12, 0x1A);
        EmGravios_PlaySeOnFrame(em, 0x58, 6, 0x1A);
        EmGravios_PlaySeOnFrame(em, 0x96, 0x2E, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x9A, 0x2E, 0x23);
        EmGravios_PlaySeOnFrame(em, 0xEE, 0x16, 0);
        break;
    case 0x41C:
        EmGravios_PlaySeOnFrame(em, 0x16, 0x28, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x48, 0x29, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x52, 0x18, 0x23);
        EmGravios_PlaySeOnFrame(em, 6, 0x14, 0x22);
        EmGravios_PlaySeOnFrame(em, 0x16, 0, 0x1A);
        EmGravios_PlaySeOnFrame(em, 0x3A, 0xF, 6);
        EmGravios_PlaySeOnFrame(em, 0x3E, 0xF, 0xC);
        EmGravios_PlaySeOnFrame(em, 0x6A, 0xB, 6);
        EmGravios_PlaySeOnFrame(em, 0x6E, 0xB, 0xC);
        EmGravios_PlaySeOnFrame(em, 0xAC, 0xB, 6);
        EmGravios_PlaySeOnFrame(em, 0xB0, 0xB, 0xC);
        break;
    case 0x41E:
        EmGravios_PlaySeOnFrame(em, 2, 0x12, 0);
        EmGravios_PlaySeOnFrame(em, 0x1C, 4, 0x1A);
        EmGravios_PlaySeOnFrame(em, 0x38, 4, 0x14);
        EmGravios_PlaySeOnFrame(em, 0x1C, 5, 0x1A);
        EmGravios_PlaySeOnFrame(em, 0x78, 4, 0x14);
        EmGravios_PlaySeOnFrame(em, 0x108, 4, 0x1A);
        EmGravios_PlaySeOnFrame(em, 0x16, 0x20, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x20, 0x28, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x84, 0x27, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x84, 0x51, 0x23);
        EmGravios_PlaySeOnFrame(em, 0xA, 0xC, 0xC);
        EmGravios_PlaySeOnFrame(em, 0x10, 0xC, 6);
        break;
    case 0x424:
        EmGravios_PlaySeOnFrame(em, 6, 0x53, 0x23);
        EmGravios_PlaySeOnFrame(em, 0xD4, 0x23, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x48, 0x13, 0x2A);
        EmGravios_PlaySeOnFrame(em, 0x30, 6, 0x1A);
        EmGravios_PlaySeOnFrame(em, 0x30, 0x11, 0x14);
        EmGravios_PlaySeOnFrame(em, 0x30, 3, 0x14);
        EmGravios_PlaySeOnFrame(em, 0xAE, 1, 0x1A);
        EmGravios_PlaySeOnFrame(em, 0xDE, 0, 0x14);
        EmGravios_PlaySeOnFrame(em, 0x34, 0xC, 6);
        EmGravios_PlaySeOnFrame(em, 0x38, 0xD, 0xC);
        break;
    case 0x425:
        EmGravios_PlaySeOnFrame(em, 6, 0x53, 0x23);
        EmGravios_PlaySeOnFrame(em, 0xA, 0x12, 0x1A);
        EmGravios_PlaySeOnFrame(em, 0x26, 6, 0x1A);
        EmGravios_PlaySeOnFrame(em, 0x4E, 4, 0x14);
        EmGravios_PlaySeOnFrame(em, 0x6E, 3, 0x14);
        EmGravios_PlaySeOnFrame(em, 0xAC, 0, 0x1A);
        EmGravios_PlaySeOnFrame(em, 0x5A, 0x15, 0);
        break;
    case 0x426:
        EmGravios_PlaySeOnFrame(em, 4, 0x4F, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x56, 0x1F, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x10, 3, 0x14);
        EmGravios_PlaySeOnFrame(em, 4, 0xD, 6);
        EmGravios_PlaySeOnFrame(em, 8, 0xD, 0xC);
        EmGravios_PlaySeOnFrame(em, 0x7E, 0, 0x1A);
        EmGravios_PlaySeOnFrame(em, 0x62, 0x13, 0x2A);
        break;
    case 0x427:
        EmGravios_PlaySeOnFrame(em, 0xE, 1, 0x1A);
        EmGravios_PlaySeOnFrame(em, 4, 0x52, 0x23);
        EmGravios_PlaySeOnFrame(em, 4, 0x13, 0);
        EmGravios_PlaySeOnFrame(em, 0x4C, 0x1F, 0x23);
        break;
    case 0x428:
        EmGravios_PlaySeOnFrame(em, 4, 0x52, 0x23);
        EmGravios_PlaySeOnFrame(em, 4, 0x13, 0);
        EmGravios_PlaySeOnFrame(em, 0x44, 0x20, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x78, 1, 0x1A);
        break;
    case 0x429:
        EmGravios_PlaySeOnFrame(em, 4, 0x54, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x3E, 4, 0x2C);
        EmGravios_PlaySeOnFrame(em, 0x12, 0x12, 0x14);
        EmGravios_PlaySeOnFrame(em, 0x2C, 6, 0x1A);
        EmGravios_PlaySeOnFrame(em, 0x3A, 0x10, 0x1A);
        EmGravios_PlaySeOnFrame(em, 0x3E, 9, 0);
        EmGravios_PlaySeOnFrame(em, 0x4A, 9, 0);
        EmGravios_PlaySeOnFrame(em, 0x64, 0x12, 0);
        EmGravios_PlaySeOnFrame(em, 0xE, 0xC, 6);
        EmGravios_PlaySeOnFrame(em, 0x12, 0xC, 0xC);
        break;
    case 0x42A:
        EmGravios_PlaySeOnFrame(em, 4, 0x52, 0x23);
        EmGravios_PlaySeOnFrame(em, 4, 0x13, 0);
        break;
    case 0x42B:
        EmGravios_PlaySeOnFrame(em, 0x46, 0x55, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x46, 0x17, 0);
        break;
    case 0x42C:
        EmGravios_PlaySeOnFrame(em, 0x1C, 2, 0x1A);
        EmGravios_PlaySeOnFrame(em, 0x30, 4, 0x14);
        EmGravios_PlaySeOnFrame(em, 0xDC, 0x10, 0x1A);
        EmGravios_PlaySeOnFrame(em, 0x1E, 0x16, 0);
        EmGravios_PlaySeOnFrame(em, 0x108, 9, 0);
        EmGravios_PlaySeOnFrame(em, 0x108, 7, 0);
        EmGravios_PlaySeOnFrame(em, 0x114, 0x17, 0);
        EmGravios_PlaySeOnFrame(em, 0x180, 0x16, 0);
        EmGravios_PlaySeOnFrame(em, 0x18, 0x52, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x8C, 0x28, 0x23);
        EmGravios_PlaySeOnFrame(em, 0xB4, 0x2C, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x13A, 0x2F, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x192, 0x2B, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x174, 0xF, 6);
        EmGravios_PlaySeOnFrame(em, 0x176, 0xE, 0xC);
        break;
    case 0x42D:
    case 0x432:
        EmGravios_PlaySeOnFrame(em, 4, 0x52, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x16, 6, 0x2A);
        EmGravios_PlaySeOnFrame(em, 4, 0x12, 0x1A);
        EmGravios_PlaySeOnFrame(em, 0x10, 0xC, 6);
        EmGravios_PlaySeOnFrame(em, 0x1A, 0xD, 0xC);
        EmGravios_PlaySeOnFrame(em, 0x92, 9, 0);
        EmGravios_PlaySeOnFrame(em, 0x92, 7, 0);
        EmGravios_PlaySeOnFrame(em, 0xBA, 7, 0);
        EmGravios_PlaySeOnFrame(em, 0xEE, 7, 0);
        EmGravios_PlaySeOnFrame(em, 0x11C, 8, 0);
        EmGravios_PlaySeOnFrame(em, 0x11C, 7, 0);
        EmGravios_PlaySeOnFrame(em, 0x6C, 0x16, 0);
        EmGravios_PlaySeOnFrame(em, 0xF2, 0x16, 0);
        EmGravios_PlaySeOnFrame(em, 0x19A, 0x16, 0);
        EmGravios_PlaySeOnFrame(em, 0x1CA, 1, 0x1A);
        EmGravios_PlaySeOnFrame(em, 0x1EE, 0, 0x14);
        break;
    case 0x42E:
    case 0x433:
        EmGravios_PlaySeOnFrame(em, 8, 0x16, 0);
        EmGravios_PlaySeOnFrame(em, 4, 0x1F, 0x23);
        break;
    case 0x42F:
        EmGravios_PlaySeOnFrame(em, 4, 0x16, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x1E, 0x20, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x30, 1, 0x14);
        EmGravios_PlaySeOnFrame(em, 0x46, 1, 0x1A);
        break;
    case 0x430:
        EmGravios_PlaySeOnFrame(em, 4, 0x16, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x1E, 0x20, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x30, 1, 0x1A);
        EmGravios_PlaySeOnFrame(em, 0x46, 1, 0x14);
        break;
    case 0x431:
        EmGravios_PlaySeOnFrame(em, 4, 0x1E, 0x22);
        EmGravios_PlaySeOnFrame(em, 4, 0x1D, 6);
        EmGravios_PlaySeOnFrame(em, 8, 0x1D, 0xC);
        EmGravios_PlaySeOnFrame(em, 0xA, 0x2A, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x4C, 0x20, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x18, 0x12, 0);
        EmGravios_PlaySeOnFrame(em, 0x1E, 0x10, 0);
        EmGravios_PlaySeOnFrame(em, 0x1E, 9, 0);
        EmGravios_PlaySeOnFrame(em, 0x22, 7, 0);
        break;
    case 0x434:
        EmGravios_PlaySeOnFrame(em, 0x10, 0x5A, 0x23);
        break;
    case 0x435:
        EmGravios_PlaySeOnFrame(em, 4, 0x53, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x14, 0xB, 0xC);
        EmGravios_PlaySeOnFrame(em, 0x3A, 0x14, 0);
        break;
    case 0x436:
        EmGravios_PlaySeOnFrame(em, 0xA, 9, 0);
        EmGravios_PlaySeOnFrame(em, 0xA, 7, 0x2A);
        EmGravios_PlaySeOnFrame(em, 0xC, 0x11, 0);
        EmGravios_PlaySeOnFrame(em, 0x10, 0x54, 0x23);
        break;
    case 0x437:
        EmGravios_PlaySeOnFrame(em, 0xC, 0x24, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x24, 0x22, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x66, 0x22, 0x23);
        EmGravios_PlaySeOnFrame(em, 0xA4, 0x22, 0x23);
        break;
    case 0x438:
        EmGravios_PlaySeOnFrame(em, 2, 0x16, 0x22);
        EmGravios_PlaySeOnFrame(em, 0x64, 0x17, 0x22);
        EmGravios_PlaySeOnFrame(em, 0x26, 0x55, 0x23);
        EmGravios_PlaySeOnFrame(em, 0xC8, 0x20, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x1C, 3, 0x1A);
        EmGravios_PlaySeOnFrame(em, 0x140, 3, 0x1A);
        break;
    case 0x439:
        EmGravios_PlaySeOnFrame(em, 2, 0x16, 0x22);
        EmGravios_PlaySeOnFrame(em, 0x64, 0x17, 0x22);
        EmGravios_PlaySeOnFrame(em, 0x26, 0x55, 0x23);
        EmGravios_PlaySeOnFrame(em, 0xC8, 0x20, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x1C, 3, 0x14);
        EmGravios_PlaySeOnFrame(em, 0x140, 3, 0x14);
        break;
    case 0x43A:
        EmGravios_PlaySeOnFrame(em, 4, 0x16, 0);
        EmGravios_PlaySeOnFrame(em, 0x32, 0x2C, 0x23);
        break;
    case 0x43B:
        EmGravios_PlaySeOnFrame(em, 4, 0x16, 0);
        EmGravios_PlaySeOnFrame(em, 4, 0x17, 0);
        EmGravios_PlaySeOnFrame(em, 0xA, 0x10, 0x1A);
        EmGravios_PlaySeOnFrame(em, 0xA, 0, 0x1A);
        EmGravios_PlaySeOnFrame(em, 0x1C, 0, 0x14);
        EmGravios_PlaySeOnFrame(em, 0x34, 0, 0x1A);
        EmGravios_PlaySeOnFrame(em, 0x58, 0, 0x14);
        EmGravios_PlaySeOnFrame(em, 0x80, 0, 0x1A);
        EmGravios_PlaySeOnFrame(em, 0xC6, 0, 0x14);
        EmGravios_PlaySeOnFrame(em, 4, 0x53, 0x23);
        EmGravios_PlaySeOnFrame(em, 0xA8, 0x27, 0x23);
        EmGravios_PlaySeOnFrame(em, 0xA2, 0xD, 6);
        EmGravios_PlaySeOnFrame(em, 0xA6, 0xD, 0xC);
        break;
    case 0x43C:
        EmGravios_PlaySeOnFrame(em, 0x16, 0xD, 6);
        EmGravios_PlaySeOnFrame(em, 0x1A, 0xD, 0xC);
        EmGravios_PlaySeOnFrame(em, 0x34, 0xA, 6);
        EmGravios_PlaySeOnFrame(em, 0x38, 0xE, 0xC);
        EmGravios_PlaySeOnFrame(em, 0x4E, 0xE, 6);
        EmGravios_PlaySeOnFrame(em, 0x4A, 0xA, 0xC);
        EmGravios_PlaySeOnFrame(em, 0x58, 0xC, 6);
        EmGravios_PlaySeOnFrame(em, 0x5C, 0xC, 0xC);
        EmGravios_PlaySeOnFrame(em, 0x20, 5, 0x1A);
        EmGravios_PlaySeOnFrame(em, 0x1C, 0, 0x14);
        break;
    case 0x43F:
        EmGravios_PlaySeOnFrame(em, 4, 0x1A, 0x22);
        EmGravios_PlaySeOnFrame(em, 8, 0x1B, 0xC);
        EmGravios_PlaySeOnFrame(em, 4, 0x1B, 6);
        EmGravios_PlaySeOnFrame(em, 0x22, 0x1C, 0x22);
        EmGravios_PlaySeOnFrame(em, 0x40, 0x1C, 0x22);
        break;
    case 0x442:
        EmGravios_PlaySeOnFrame(em, 0x20, 0xD, 6);
        EmGravios_PlaySeOnFrame(em, 0x24, 0xD, 0xC);
        EmGravios_PlaySeOnFrame(em, 0x4A, 0xD, 6);
        EmGravios_PlaySeOnFrame(em, 0x4E, 0xD, 0xC);
        EmGravios_PlaySeOnFrame(em, 0x7C, 0xB, 6);
        EmGravios_PlaySeOnFrame(em, 0x80, 0xB, 0xC);
        break;
    case 0x447:
        EmGravios_PlaySeOnFrame(em, 4, 0x10, 0);
        EmGravios_PlaySeOnFrame(em, 0xA, 0x12, 0);
        EmGravios_PlaySeOnFrame(em, 0x14, 9, 0);
        EmGravios_PlaySeOnFrame(em, 4, 0x27, 0x23);
        EmGravios_PlaySeOnFrame(em, 4, 0x52, 0x23);
        break;
    case 0x448:
        EmGravios_PlaySeOnFrame(em, 0x16, 0x12, 0);
        EmGravios_PlaySeOnFrame(em, 0x48, 0x12, 0);
        EmGravios_PlaySeOnFrame(em, 4, 0xC, 6);
        EmGravios_PlaySeOnFrame(em, 8, 0xC, 0xC);
        EmGravios_PlaySeOnFrame(em, 0x30, 0x13, 0);
        EmGravios_PlaySeOnFrame(em, 4, 0x28, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x28, 0x53, 0x23);
        EmGravios_PlaySeOnFrame(em, 4, 0x16, 0);
        break;
    case 0x449:
        EmGravios_PlaySeOnFrame(em, 0x22, 0x16, 6);
        EmGravios_PlaySeOnFrame(em, 0x26, 0x16, 0xC);
        EmGravios_PlaySeOnFrame(em, 0x62, 4, 0x22);
        EmGravios_PlaySeOnFrame(em, 0x6C, 3, 0x22);
        EmGravios_PlaySeOnFrame(em, 4, 0x52, 0x23);
        EmGravios_PlaySeOnFrame(em, 4, 0x2C, 0x23);
        break;
    case 0x44A:
        EmGravios_PlaySeOnFrame(em, 4, 0x16, 6);
        EmGravios_PlaySeOnFrame(em, 8, 0x16, 0xC);
        EmGravios_PlaySeOnFrame(em, 0x6C, 3, 0x22);
        EmGravios_PlaySeOnFrame(em, 4, 0x50, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x46, 0x20, 0x23);
        break;
    case 0x44C:
        EmGravios_PlaySeOnFrame(em, 0x16, 4, 0x1A);
        EmGravios_PlaySeOnFrame(em, 0x18, 8, 0x14);
        EmGravios_PlaySeOnFrame(em, 0x1A, 0x10, 0x1A);
        EmGravios_PlaySeOnFrame(em, 0xC8, 1, 0x1A);
        EmGravios_PlaySeOnFrame(em, 0xE0, 0, 0x14);
        EmGravios_PlaySeOnFrame(em, 0xA6, 0x16, 0);
        EmGravios_PlaySeOnFrame(em, 0x64, 0x20, 0x23);
        break;
    case 0x44D:
        EmGravios_PlaySeOnFrame(em, 4, 0x17, 0);
        EmGravios_PlaySeOnFrame(em, 0x18C, 0x16, 0);
        EmGravios_PlaySeOnFrame(em, 0x32, 0x20, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x7E, 0x57, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x178, 0x1F, 0x23);
        break;
    case 0x44F:
        EmGravios_PlaySeOnFrame(em, 4, 0x16, 0);
        EmGravios_PlaySeOnFrame(em, 0x68, 0x16, 0);
        EmGravios_PlaySeOnFrame(em, 4, 0x23, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x92, 0, 0x1A);
        EmGravios_PlaySeOnFrame(em, 0xCE, 0, 0x14);
        break;
    case 0x450:
        EmGravios_PlaySeOnFrame(em, 0x38, 0x22, 0x23);
        EmGravios_PlaySeOnFrame(em, 0xAE, 0x22, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x118, 0x22, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x15E, 0x1F, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x1C, 1, 0x1A);
        EmGravios_PlaySeOnFrame(em, 0x16E, 3, 0x14);
        EmGravios_PlaySeOnFrame(em, 0x196, 0, 0x1A);
        break;
    case 0x451:
        if ((em->stateGroup == 8) && (em->state == 3)) {
            EmGravios_PlaySeOnFrame(em, 0x72, 0x27, 0x23);
            EmGravios_PlaySeOnFrame(em, 0x3E, 0x24, 0x23);
            EmGravios_PlaySeOnFrame(em, 0xE, 0xD, 6);
            EmGravios_PlaySeOnFrame(em, 0x12, 0xD, 0xC);
            EmGravios_PlaySeOnFrame(em, 0x14, 6, 0x14);
            EmGravios_PlaySeOnFrame(em, 0xFE, 1, 0x14);
            EmGravios_PlaySeOnFrame(em, 0x122, 0, 0x14);
            break;
        }
        EmGravios_PlaySeOnFrame(em, 0x72, 0x51, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x3E, 0x27, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x5E, 0x1B, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x78, 0x1A, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x78, 0x18, 0x23);
        EmGravios_PlaySeOnFrame(em, 0xE, 0xD, 6);
        EmGravios_PlaySeOnFrame(em, 0x12, 0xD, 0xC);
        EmGravios_PlaySeOnFrame(em, 0x14, 6, 0x14);
        EmGravios_PlaySeOnFrame(em, 0xFE, 1, 0x14);
        EmGravios_PlaySeOnFrame(em, 0x122, 0, 0x14);
        break;
    case 0x452:
        EmGravios_PlaySeOnFrame(em, 0x30, 0x20, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x6C, 0x56, 0x23);
        EmGravios_PlaySeOnFrame(em, 4, 0x12, 0x14);
        EmGravios_PlaySeOnFrame(em, 0x196, 0, 0x1A);
        break;
    case 0x454:
        EmGravios_PlaySeOnFrame(em, 0x68, 0x51, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x68, 0x15, 0);
        EmGravios_PlaySeOnFrame(em, 4, 0x16, 0);
        EmGravios_PlaySeOnFrame(em, 0x26, 0x16, 0);
        EmGravios_PlaySeOnFrame(em, 0x48, 9, 0xC);
        EmGravios_PlaySeOnFrame(em, 0x4E, 9, 0xC);
        EmGravios_PlaySeOnFrame(em, 0x4E, 0x10, 0x22);
        EmGravios_PlaySeOnFrame(em, 0x76, 4, 0x14);
        EmGravios_PlaySeOnFrame(em, 0x7E, 3, 0x1A);
        EmGravios_PlaySeOnFrame(em, 0x58, 0xF, 0x14);
        EmGravios_PlaySeOnFrame(em, 0x5C, 0xF, 0xC);
        EmGravios_PlaySeOnFrame(em, 0xF8, 0, 0x1A);
        break;
    case 0x455:
        EmGravios_PlaySeOnFrame(em, 4, 0x1F, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x16, 0x12, 0x1A);
        EmGravios_PlaySeOnFrame(em, 0x1A, 3, 0x1A);
        EmGravios_PlaySeOnFrame(em, 0x8E, 4, 0x14);
        EmGravios_PlaySeOnFrame(em, 0x124, 1, 0x1A);
        EmGravios_PlaySeOnFrame(em, 0x162, 3, 0x14);
        EmGravios_PlaySeOnFrame(em, 0x5A, 0x13, 0x2A);
        EmGravios_PlaySeOnFrame(em, 0x4A, 0x15, 0);
        EmGravios_PlaySeOnFrame(em, 0x4A, 8, 0);
        EmGravios_PlaySeOnFrame(em, 0x98, 0x16, 0);
        EmGravios_PlaySeOnFrame(em, 0x56, 8, 0);
        break;
    case 0x456:
        type = em->type;
        if ((type == 0x11) || (type == 0x2F)) {
            stateGroup = em->stateGroup;
            if ((stateGroup != 0) && (stateGroup != 6)) {
                EmGravios_PlaySeOnFrame(em, 0x82, 0x51, 0x23);
                EmGravios_PlaySeOnFrame(em, 0xE, 0xD, 6);
                EmGravios_PlaySeOnFrame(em, 0x12, 0xD, 0xC);
                EmGravios_PlaySeOnFrame(em, 0x1A, 6, 0x14);
                EmGravios_PlaySeOnFrame(em, 0x3E, 0x27, 0x23);
                EmGravios_PlaySeOnFrame(em, 0x68, 0x1B, 0x23);
                EmGravios_PlaySeOnFrame(em, 0x68, 0x19, 0x23);
                EmGravios_PlaySeOnFrame(em, 0x90, 0x1A, 0x23);
                EmGravios_PlaySeOnFrame(em, 0x90, 0x18, 0x23);
                break;
            }
            EmGravios_PlaySeOnFrame(em, 0x6E, 0x54, 0x23);
            EmGravios_PlaySeOnFrame(em, 0xE, 0xD, 6);
            EmGravios_PlaySeOnFrame(em, 0x12, 0xD, 0xC);
            EmGravios_PlaySeOnFrame(em, 0x1A, 6, 0x14);
            EmGravios_PlaySeOnFrame(em, 0x3E, 0x27, 0x23);
            EmGravios_PlaySeOnFrame(em, 0x82, 0x51, 0x23);
            EmGravios_PlaySeOnFrame(em, 0x68, 0x1B, 0x23);
            EmGravios_PlaySeOnFrame(em, 0x68, 0x19, 0x23);
            EmGravios_PlaySeOnFrame(em, 0x90, 0x1A, 0x23);
            EmGravios_PlaySeOnFrame(em, 0x90, 0x18, 0x23);
            EmGravios_PlaySeOnFrame(em, 0x92, 0x1D, 0x23);
            break;
        }
        EmGravios_PlaySeOnFrame(em, 0x14, 6, 0x14);
        EmGravios_PlaySeOnFrame(em, 0x3C, 0x27, 0x23);
        EmGravios_PlaySeOnFrame(em, 0xE, 0xD, 6);
        EmGravios_PlaySeOnFrame(em, 0x12, 0xD, 0xC);
        EmGravios_PlaySeOnFrame(em, 0x40, 0x26, 0x23);
        break;
    case 0x457:
        EmGravios_PlaySeOnFrame(em, 0x14, 0x23, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x58, 0x24, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x7E, 0x56, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x14, 1, 0x1A);
        EmGravios_PlaySeOnFrame(em, 0x44, 2, 0x14);
        break;
    case 0x458:
        EmGravios_PlaySeOnFrame(em, 0x32, 0x28, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x68, 0x27, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x38, 4, 0x14);
        EmGravios_PlaySeOnFrame(em, 0xF8, 1, 0x14);
        EmGravios_PlaySeOnFrame(em, 0x2E, 0x14, 0x2A);
        EmGravios_PlaySeOnFrame(em, 0x68, 0x14, 0x2A);
        EmGravios_PlaySeOnFrame(em, 0x9C, 0x14, 0x2A);
        EmGravios_PlaySeOnFrame(em, 0xD2, 0x14, 0x2A);
        EmGravios_PlaySeOnFrame(em, 0x112, 0x13, 0x2A);
        break;
    case 0x459:
        EmGravios_PlaySeOnFrame(em, 0x40, 0x16, 0);
        EmGravios_PlaySeOnFrame(em, 0x48, 0x4F, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x70, 1, 0x14);
        EmGravios_PlaySeOnFrame(em, 0x98, 0, 0x1A);
        break;
    case 0x45A:
        EmGravios_PlaySeOnFrame(em, 0xA, 0x20, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x3E, 0x1C, 6);
        EmGravios_PlaySeOnFrame(em, 0x42, 0x1C, 0xC);
        EmGravios_PlaySeOnFrame(em, 0x7A, 0x16, 0);
        EmGravios_PlaySeOnFrame(em, 0x78, 0x1F, 0x23);
        EmGravios_PlaySeOnFrame(em, 0xCA, 1, 0x14);
        EmGravios_PlaySeOnFrame(em, 0xE6, 0, 0x1A);
        break;
    case 0x45B:
        EmGravios_PlaySeOnFrame(em, 4, 0x16, 0);
        EmGravios_PlaySeOnFrame(em, 0x32, 0x47, 0x23);
        break;
    case 0x45C:
        EmGravios_PlaySeOnFrame(em, 4, 0x53, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x34, 0x29, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x15C, 0x52, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x1BA, 0x52, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x20, 0x16, 0);
        EmGravios_PlaySeOnFrame(em, 0x98, 0x16, 0);
        EmGravios_PlaySeOnFrame(em, 0xC0, 0x10, 0x14);
        EmGravios_PlaySeOnFrame(em, 0xD2, 0x16, 0);
        EmGravios_PlaySeOnFrame(em, 0xD6, 8, 0);
        EmGravios_PlaySeOnFrame(em, 0x12C, 7, 0);
        EmGravios_PlaySeOnFrame(em, 0x14E, 0xD, 6);
        EmGravios_PlaySeOnFrame(em, 0x15C, 0x16, 0);
        EmGravios_PlaySeOnFrame(em, 0x1C8, 0x16, 0);
        break;
    case 0x45D:
        EmGravios_PlaySeOnFrame(em, 0x72, 0x51, 0x23);
        EmGravios_PlaySeOnFrame(em, 4, 0xD, 6);
        EmGravios_PlaySeOnFrame(em, 8, 0xD, 0xC);
        EmGravios_PlaySeOnFrame(em, 0x16, 6, 0x14);
        EmGravios_PlaySeOnFrame(em, 0xA, 0x27, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x42, 0x1B, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x42, 0x19, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x72, 0x1A, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x72, 0x18, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x100, 0x20, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x136, 0xE, 6);
        EmGravios_PlaySeOnFrame(em, 0x13A, 0xE, 0xC);
        EmGravios_PlaySeOnFrame(em, 0x110, 0x16, 0);
        EmGravios_PlaySeOnFrame(em, 0x12E, 1, 0x14);
        EmGravios_PlaySeOnFrame(em, 0x152, 0, 0x1A);
        break;
    case 0x45E:
        EmGravios_PlaySeOnFrame(em, 0x18, 0x11, 0x1A);
        EmGravios_PlaySeOnFrame(em, 0x70, 1, 0x1A);
        EmGravios_PlaySeOnFrame(em, 0xA6, 0, 0x14);
        EmGravios_PlaySeOnFrame(em, 0x28, 0x16, 0);
        break;
    case 0x45F:
        EmGravios_PlaySeOnFrame(em, 0x16, 1, 0x14);
        EmGravios_PlaySeOnFrame(em, 0x12, 0x24, 0x23);
        EmGravios_PlaySeOnFrame(em, 0xC, 0x16, 0);
        EmGravios_PlaySeOnFrame(em, 0x3C, 0xF, 6);
        EmGravios_PlaySeOnFrame(em, 0x40, 0xF, 0xC);
        EmGravios_PlaySeOnFrame(em, 0x3E, 7, 0x1A);
        EmGravios_PlaySeOnFrame(em, 0x8A, 0xE, 6);
        EmGravios_PlaySeOnFrame(em, 0x8E, 0xE, 0xC);
        break;
    case 0x460:
        EmGravios_PlaySeOnFrame(em, 0x10, 0x11, 0x1A);
        EmGravios_PlaySeOnFrame(em, 0x4E, 0x10, 0x14);
        EmGravios_PlaySeOnFrame(em, 0xE, 0x31, 6);
        EmGravios_PlaySeOnFrame(em, 0x2C, 0x31, 0xC);
        EmGravios_PlaySeOnFrame(em, 0x3E, 0x31, 6);
        EmGravios_PlaySeOnFrame(em, 0x68, 0x33, 0);
        EmGravios_PlaySeOnFrame(em, 0x82, 0x16, 0);
        break;
    case 0x461:
        EmGravios_PlaySeOnFrame(em, 0x2E, 0x23, 0x23);
        EmGravios_PlaySeOnFrame(em, 0x6E, 0x2D, 0x23);
        EmGravios_PlaySeOnFrame(em, 4, 0x33, 0);
        EmGravios_PlaySeOnFrame(em, 6, 0x16, 0);
        EmGravios_PlaySeOnFrame(em, 0x5A, 0x17, 0);
        EmGravios_PlaySeOnFrame(em, 0xAE, 0x10, 0);
        EmGravios_PlaySeOnFrame(em, 0xDA, 0x16, 0);
        EmGravios_PlaySeOnFrame(em, 0x116, 0x16, 0);
        break;
    case 0x462:
        EmGravios_PlaySeOnFrame(em, 2, 9, 0);
        EmGravios_PlaySeOnFrame(em, 0xE, 7, 0);
        EmGravios_PlaySeOnFrame(em, 0x82, 0, 0x1A);
        EmGravios_PlaySeOnFrame(em, 0xAC, 0, 0x14);
        
        break;
    }
}

/* em_hermitaur's .data (0x8C4A90-0x8C4B30) */
EmTurnParams EmHermitaur_TurnParams[1] = {
#include "game/008C4A90.inc"
};
f32 EmHermitaur_State3_12Ofs[20] = {
#include "game/008C4AB0.inc"
};
EmHermitaurVec EmHermitaur_CarveThrowPt = {
#include "game/008C4B00.inc"
};
void *EmHermitaur_Callbacks[8] = {
    EmHermitaur_SpawnEffects, EmHermitaur_SpawnEffects, EmHermitaur_TableNop, EmHermitaur_UpdateMotionEffects,
    EmHermitaur_ScriptQuery, EmHermitaur_Reinit, EmHermitaur_AllowHit8, NULL,
};
