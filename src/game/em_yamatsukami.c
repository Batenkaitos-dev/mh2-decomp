/*
 * Monster family: Yama Tsukami (EM_KIND_YAMA_TSUKAMI and
 * EM_KIND_YAMA_TSUKAMI_B). The code from the family's setup function
 * (EmYamaTsukami_Setup, called by the per-kind dispatch Em_TypeSetup) to the
 * next family's. The state functions take the monster and the work area its
 * state dispatcher passes on (EmWork); Em.step is the step within the current
 * state. See include/em.h.
 */
#include "common.h"
#include "scene.h"
#include "quest.h"
#include "mine_point.h"
#include "em.h"
#include "player.h"
#include "area.h"
#include "qsync.h"

/* The work area at Em +0x438 as EmYamaTsukami_InitBoneFloorFlags sees it. */
typedef struct EmYamaWork {
    u8 unk00[0x8];
    s16 unk08;          /* 0x08: the motion EmYamaTsukami_MotionEffects last saw */
    s8 unk0A;           /* 0x0A */
    u8 unk0B[0x3C - 0xB];
    s8 flags[4][2];     /* 0x3C */
} EmYamaWork;



/* prototypes */
u16 Area_IdForCurrentTime(u16);
s8 EmMine_CreateTimed0(Em *em, f32 *pos);
void EmYamaTsukami_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void Em_PlaySeAtBoneOnFrame(Em *em, s32 frame, s32 se, s32 bone);
void Em_SetMotion(Em *em, s32 motion, s32 a, s32 b);
u16 Math_AngleXZ(f32 *from, f32 *to);
s32 Save_TestEventFlag(s32);
void YamaTsukamiAttack_Create(Em *, s8);
void * EmEffectAttack_Create(Em *, s32, s32, s32, s32);
void Em_MoveRotated(Em *, void *);
void Em_RestartScript(Em *em);
void EmYamaTsukami_StartCurves(Em *em, u8 kind);
u8 EmYamaTsukami_UpdateCurves(Em *em);
void EmYamaTsukami_GetMinePointPos(Em *em, f32 *out);
void EmYamaTsukami_RunGroup0(Em *);
void EmYamaTsukami_State1_00(Em *);
void EmYamaTsukami_State1_01(Em *);
void EmYamaTsukami_State1_02(Em *, s32);
void EmYamaTsukami_State2_01(Em *);
void EmYamaTsukami_State2_04(Em *, s32);
void EmYamaTsukami_State2_07(Em *, s32);
void EmYamaTsukami_State2_09(Em *, s32);
void EmYamaTsukami_State2_0A(Em *, s32);
void EmYamaTsukami_State2_0B(Em *, s32);
void EmYamaTsukami_State2_0C(Em *, s32);
void EmYamaTsukami_State2_14(Em *);
void EmYamaTsukami_State2_16(Em *em);
void EmYamaTsukami_State3_00(Em *);
void EmYamaTsukami_State3_01(Em *, s32);
void EmYamaTsukami_State3_07(Em *, s32);
void EmYamaTsukami_State5_00_Hunted(Em *);
void EmYamaTsukami_State5_01_HuntedFalling(Em *);
void EmYamaTsukami_State6_01(Em *);
void Em_SetLayerMotion(Em *em, s32 motion, s32 blend, s32 frame, s32 layer);
void EmYamaTsukami_MotionEffects(Em *, EmWork *);
void EmYamaTsukami_EaseToTarget(Em *em, EmWork *work);
void EmYamaTsukami_InitBoneFloorFlags(Em *em);
extern s8 EmYamaTsukami_BoneFloorBones[4][2];
f32 AreaGrid2_GetFloorY(f32 *pos);
void EmYamaTsukami_SpawnBoneFloorEffects(Em *);
void EmFxTri_Spawn(Em *em, s16 kind);
s32 EmMotion_ReachedFrame(void *obj, s32 a, f32 frame);
f32 Vec_Distance(f32 *a, f32 *b);
u16 Math_Heading(f32 x1, f32 z1, f32 x2, f32 z2);
f32 Vec_DistanceXZ(f32 *a, f32 *b);
s32 Debug_IsOn(s32);
void MinePoint_SetArea(s32 slot, u16 area);
void Player_Integrate(void *);
void Player_ClearVelocity(void *);
void Effect01_Spawn(void *obj, s16 type);
void GmEm_ResetSize(); /* unit's own declaration: GmEm_ResetSize reads a0 = em (the incoming argument, still in a0); the call passes no argument */
extern f32 EmYamatsukami_EaseTargets[];
extern f32 D_008CAAB0;
extern f32 D_008CAAB4;
extern f32 D_008CAAB8;
extern f32 D_008CAABC;
extern f32 D_008CAAC0;
extern f32 D_008CAAC4;
extern f32 D_008CAAC8;
extern f32 D_008CAACC;
typedef struct { s32 t, v; } EmYamaKey;
extern EmYamaKey *EmYamaTsukami_Keys[];
static u8 EmYamaTsukami_InterpKeys(Em *em, EmYamaTsukamiWork *w, s32 idx);
f32 Math_Absf(f32 x);
extern f32 D_008CAAD0;
u16 Area_DayId(u16 id);
f32 Math_Sin(f32 x);
void Em_BonePos(void *obj, s16 bone, f32 *out);
void EmImpactAttack_CreateAt(Em *em, f32 *pos, s8 kind);
void StageObj_K14_Create(Em *em, s16 k, s16 c, f32 scale);
void StageObj_K14_CreateAt(f32 *pos, u8 k, u8 c, f32 scale);
u8 Player_InCurrentArea(void *obj);
void EmYamaTsukami_MotionSounds(Em *, EmWork *);
static void EmYamaTsukami_MotionEffectsDefault(Em *em);
EmBone *Em_Bone(void *obj, s16 bone);
void Vec_Rotate(f32 *out, f32 *v, f32 *m);
void EmYamaTsukami_SpawnAreaImpactEffects(Em *);
s32 EmMotion_PassedFrameScaled(void *p, f32 frame, s32 layer);
void Quest_TriggerEvent(Em *em);
void Em_BreakPart(Em *em, s32 part);
void Em_EndStatus5(Em *em);
void Em_UpdatePendingScript(Em *em);
void Em_ClearHitReactMode(Em *em);
u8 Em_UpdateDamage(Em *em, u8 *out);
s32 MinePoint_Count(s32);
void QSync_SendEm(Em *em, u8 kind, u8 arg);
void EmYamaTsukami_RunState(Em *em);
s32 func_00849C20(Em *em);
s32 EmMine_Update(Em *em);
s8 EmMine_Create(Em *em, s32 idx);
void QuestEm_Hunted(Em *em);
void CamReq_BossHunted(void);
void EmYamaTsukami_RemoveMinePoints(Em *em);
s32 Em_PlaySeChecked(Em *em, s32 id, s32 a, void *pos, s32 b, s32 c);
void Effect02_SpawnForOwner(void *owner, s32 ang, s32 type, s32 sub, f32 *pos, f32 scale);
void Effect0D_SpawnIfHere(void *o, f32 *pos, s32 type, f32 scale);
void CamShake_ReqARange(Em *obj, s32 i, s32 flag);
void Em_MotionDelta(Em *em, f32 *out);
void EmKind38_SpawnAtEm(Em *em);
void Effect0D_SpawnAt(f32 *pos, s32 a, f32 scale);
s32 Em_ComputeMaxHp(Em *em, s16 idx);
void Em_SetMotionRow(Em *em, s32 row, s32 arg2, s32 arg3);
void Em_SetStateRand(Em *em, s32 arg1, s32 arg2);
extern f32 D_008CA9A8;
extern f32 D_008CA9AC;
extern f32 D_008CA9B0;
extern f32 gStartPos[];
extern f32 D_43F0A4[];
extern f32 D_43F0A8[];
void EmYamaTsukami_AddMinePoint(Em *em);
/* end of prototypes */

void EmYamaTsukami_Setup(Em *em)
{
    f32 pos[4];
    EmYamaTsukamiWork *w = (EmYamaTsukamiWork *)&em->work;

    w->unk000 = 0;
    if (w->mineSlot != -1) {
        MinePoint_SetArea(w->mineSlot, em->area);
        EmYamaTsukami_GetMinePointPos(em, pos);
        MinePoint_SetPos(w->mineSlot, pos);
    }
}

void EmYamaTsukami_SpawnEffects(Em *em)
{
    Effect01_Spawn((Player *) &em->active, 3);
    if (em->type == 0x48) {
        EmFxTri_Spawn(em, 5);
    }
}

void EmYamaTsukami_GetMinePointPos(Em *em, f32 *out)
{
    f32 ofs[3];
    f32 b[3];
    f32 a[3];

    ofs[1] = 400.0f;
    ofs[0] = 0.0f;
    ofs[2] = 0.0f;
    Em_BonePos(em, 2, a);
    Vec_Rotate((f32 *) b, (f32 *) ofs, (f32 *) (Em_Bone(em, 2)));
    out[0] = a[0] + b[0];
    out[1] = a[1] + b[1];
    out[2] = a[2] + b[2];
}

void EmYamaTsukami_AddMinePoint(Em *em)
{
    EmYamaTsukamiWork *w = (EmYamaTsukamiWork *)&em->work;
    MinePoint pt;
    f32 pos[3];

    if (w->mineCount <= 0) {
        w->mineSlot = -1;
        return;
    }
    pt.radius = 800.0f;
    switch (gQuest.header->tier) {
    default:
        pt.table = 0x63;
        break;
    case 1:
        pt.table = 0xE0;
        break;
    case 2:
        pt.table = 0x15C;
        break;
    case 3:
        pt.table = 0x15C;
        break;
    }
    pt.action = MINE_ACTION_CARVE;
    pt.count = w->mineCount;
    pt.area = (u8) em->area;
    pt.flags = 1;
    pt.timer = 0;
    EmYamaTsukami_GetMinePointPos(em, pos);
    w->mineSlot = MinePoint_AddUntimed(&pt, pos, 0);
}

void EmYamaTsukami_RemoveMinePoints(Em *em)
{
    u8 *w = &em->work;
    s32 i;
    s8 *p;

    for (i = 0; i < 3; i++) {
        p = (s8 *) (w + i) + 0x39;
        if (*p != -1) {
            MinePoint_Remove(*p);
            *p = -1;
        }
    }
}

void EmYamaTsukami_AnglesToPoint(Em *em, EmWork *work)
{
    work->accelAngZ = 0x10000 - (Math_Heading(Vec_Distance((f32 *) &em->posX, (f32 *) em->goal), em->posY, 0.0f, em->goal[1]) & 0xFFFF);
    ((EmYamaTsukamiWork *)work)->accelAngY = (s32) ((Math_Heading(em->posX, em->posZ, em->goal[0], em->goal[2]) & 0xFFFF) - 0x4000);
    ((EmYamaTsukamiWork *)work)->accelAngZ = 0;
}

void EmYamaTsukami_Start(Em *em)
{
    EmYamaTsukamiWork *w = (EmYamaTsukamiWork *)&em->work;
    s16 hp;

    if (gQuest.stage == 0) {
        em->posX = gStartPos[gScene.area * 3];
        em->posY = D_43F0A4[gScene.area * 3];
        em->posZ = D_43F0A8[gScene.area * 3];
    }
    if (em->type == 0x48) {
        Area_DayId(em->area);
        em->posX = D_008CA9A8;
        em->posY = D_008CA9AC;
        em->posZ = D_008CA9B0;
        em->blind = 2;
        em->unk_InitFlag = 0;
        em->combat = 1;
        w->mineCount = 0;
        w->mineSlot = -1;
        w->unk038 = 1;
    } else {
        em->unk_InitFlag = 1;
        em->combat = 1;
        em->combatPlayers = 0xF;
        em->blind = 0;
        w->unk038 = 0;
        w->mineCount = 3;
        EmYamaTsukami_AddMinePoint(em);
    }
    hp = Em_ComputeMaxHp(em, 0);
    em->hp = hp;
    em->hpMax = hp;
    em->scriptPending = 1;
    Em_SetMotionRow(em, 1, 0, 0);
    Em_SetStateRand(em, 2, 0);
    w->unk02E = 0;
    w->unk000 = 0;
    w->unk033 = 0xF;
    w->easeIdx = 2;
    w->easeMode = 0;
    w->ease[0] = 1.0f;
    w->ease[1] = 1.0f;
    w->ease[2] = 1.0f;
    w->syncTimer = 0x96;
    em->stance = 2;
}

void EmYamaTsukami_EndState(Em *em)
{
    em->motionSpeed = 1.0f;
    em->lookAtTarget = 0;
    em->scriptPending = 1;
    em->stance = 0;
    EmYamaTsukami_ChangeState(em, 0, 1, 0);
}

void EmYamaTsukami_EndState2(Em *em)
{
    em->motionSpeed = 1.0f;
    em->lookAtTarget = 0;
    em->scriptPending = 1;
    em->stance = 2;
    EmYamaTsukami_ChangeState(em, 2, 0, 0);
}

void EmYamaTsukami_State0_00(Em *em, EmWork *work)
{
    if (Debug_IsOn(0x4D) == 0) {
        if (em->motionId[0] != 0x3E9) {
            Em_SetMotion(em, 1, 0xA, 0);
        }
        if (em->motionId[1] != 0x44D) {
            Em_SetMotion(em, 1, 0xA, 0);
        }
    }
}

void EmYamaTsukami_State0_01(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 0;
        Em_SetMotion(em, 1, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_EndState(em);
        }
        break;
    }
}

void EmYamaTsukami_State0_02(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 0;
        Em_SetMotion(em, 0x14, 0, 0);
        em->timer = 5;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            if (--em->timer <= 0) {
                em->step++;
                EmYamaTsukami_ChangeState(em, 0, 3, 4);
                break;
            }
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmYamaTsukami_ChangeState(em, 0, 3, 4);
        }
        break;
    }
}

void EmYamaTsukami_State0_03(Em *em, EmWork *work)
{
    em->guardTimer = 5;
    em->noReact = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 0;
        Em_SetMotion(em, 0x15, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_ChangeState(em, 0, 4, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmYamaTsukami_ChangeState(em, 0, 4, 4);
        }
        break;
    }
}

void EmYamaTsukami_State0_04(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 0;
        Em_SetMotion(em, 0x7, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_EndState(em);
        }
        break;
    }
}

void EmYamaTsukami_RunGroup0(Em *em)
{
    EmWork *work = (EmWork *) &em->work;

    switch (em->state) {
    case 0:
        EmYamaTsukami_State0_00(em, work);
        break;
    case 1:
        EmYamaTsukami_State0_01(em, work);
        break;
    case 2:
        EmYamaTsukami_State0_02(em, work);
        break;
    case 3:
        EmYamaTsukami_State0_03(em, work);
        break;
    case 4:
        EmYamaTsukami_State0_04(em, work);
        break;
    }
}

void EmYamaTsukami_State1_00(Em *em)
{
    f32 pos[3];
    s32 d;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 0;
        Em_SetMotion(em, 0x16, 0, 0);
        break;
    case 1:
        if (em->hasGoal != 0) {
            d = ((Math_AngleXZ(&em->posX, em->goal) & 0xFFFF) - em->rotY) & 0xFFFF;
            if (d <= 0x8000) {
                if (d <= 0x3F) {
                    em->rotY += d;
                } else {
                    em->rotY += 0x40;
                }
            } else {
                if (d > 0xFFC0) {
                    em->rotY += d;
                } else {
                    em->rotY -= 0x40;
                }
            }
        }
        Em_MotionDelta(em, pos);
        if ((em->goalDist -= pos[2]) <= 0.0f) {
            em->step++;
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_EndState(em);
        }
        break;
    }
}

void EmYamaTsukami_State1_01(Em *em)
{
    f32 pos[3];
    s32 d;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 0;
        Em_SetMotion(em, 0x16, 0, 0);
        break;
    case 1:
        if (em->hasGoal != 0) {
            d = ((((Math_AngleXZ(&em->posX, em->goal) & 0xFFFF) + 0x8000) & 0xFFFF) - em->rotY) & 0xFFFF;
            if (d <= 0x8000) {
                if (d <= 0x3F) {
                    em->rotY += d;
                } else {
                    em->rotY += 0x40;
                }
            } else {
                if (d > 0xFFC0) {
                    em->rotY += d;
                } else {
                    em->rotY -= 0x40;
                }
            }
        }
        Em_MotionDelta(em, pos);
        if ((em->goalDist += pos[2]) <= 0.0f) {
            em->step++;
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_EndState(em);
        }
        break;
    }
}

void EmYamaTsukami_State1_02(Em *em, s32 variant)
{
    EmYamaTsukamiWork *w = (EmYamaTsukamiWork *)&em->work;
    u32 d;
    u16 t;
    s32 s;
    s32 a;

    switch (em->step) {
    case 0:
        em->step++;
        em->stateVar0 = 0;
        switch (variant) {
        case 0:
            em->stance = 0;
            if (em->motionId[1] == 0x3E9) {
                Em_SetMotion(em, 1, 0, 0);
            }
            break;
        case 2:
            w->unk038 = 1;
        case 1:
            em->stance = 2;
            Em_SetMotion(em, 0x16, 0, 0);
            break;
        }
        em->lookAtTarget = 0;
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->timer = 0;
        d = (em->aimAngle - *(u16 *) &em->rotY) & 0xFFFF;
        if (d < 0x8000) {
            w->unk00C = d;
        } else {
            w->unk00C = 0x10000 - d;
        }
        w->unk00C /= 2;
        w->unk01C = 0;
        break;
    case 1:
        if (em->stateVar0 == 0) {
            w->unk01C += 0x20;
            w->unk00C -= w->unk01C;
            if (w->unk00C <= 0) {
                em->stateVar0 = 1;
            }
        } else {
            w->unk01C -= 0x20;
            if (w->unk01C <= 0) {
                w->unk01C = 0x20;
            }
        }
        a = em->rotY;
        t = em->aimAngle;
        s = w->unk01C;
        d = (t - (a & 0xFFFF)) & 0xFFFF;
        if (((d + s) & 0xFFFF) < s * 2) {
            em->step++;
            em->rotY = em->aimAngle;
        } else if (d < 0x8000) {
            em->rotY = (a + s) & 0xFFFF;
        } else {
            em->rotY = (a - s) & 0xFFFF;
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            switch (variant) {
            case 0:
                EmYamaTsukami_EndState(em);
                break;
            case 1:
            case 2:
                EmYamaTsukami_EndState2(em);
                break;
            }
        }
        break;
    }
}

void EmYamaTsukami_RunGroup1(Em *em)
{
    switch (em->state) {
    case 0:
        EmYamaTsukami_State1_00(em);
        break;
    case 1:
        EmYamaTsukami_State1_01(em);
        break;
    case 2:
        EmYamaTsukami_State1_02(em, 0);
        break;
    }
}

void EmYamaTsukami_State2_00(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        Em_SetMotion(em, 1, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_EndState2(em);
        }
        break;
    }
}

void EmYamaTsukami_State2_01(Em *em)
{
    EmYamaTsukamiWork *w = (EmYamaTsukamiWork *)&em->work;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        Em_SetMotion(em, 0xE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            if (--w->unk033 <= 0) {
                em->step++;
                EmYamaTsukami_ChangeState(em, 2, 0x12, 4);
            }
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmYamaTsukami_ChangeState(em, 2, 0x12, 4);
        }
        break;
    }
}

void EmYamaTsukami_State2_04(Em *em, s32 variant)
{
    EmYamaTsukamiWork *w = (EmYamaTsukamiWork *)&em->work;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        Player_ClearVelocity(em);
        em->goalDist = Vec_Distance((f32 *) &em->posX, (f32 *) em->goal);
        EmYamaTsukami_AnglesToPoint(em, (EmWork *) w);
        if (em->curGoalKind == 0) {
            em->hasGoal = 0;
        } else {
            em->hasGoal = 1;
        }
        em->timer = 0;
        switch (variant) {
        case 1:
            w->unk038 = 1;
            Em_SetMotion(em, 0x16, 0, 0);
            w->unk00B = 30;
            break;
        case 2:
            w->unk038 = 1;
        case 0:
        case 3:
            Em_SetMotion(em, 0x16, 0, 0);
            em->step++;
            break;
        }
        break;
    case 1:
        if (--w->unk00B <= 0) {
            em->step++;
            Em_SetMotion(em, 0x17, 0, 0);
        }
    case 2:
        switch (variant) {
        case 1:
            em->velZ = 20.0f + 2.0f * Math_Sin((6.2831855f * (f32) (em->timer << 10)) / 65536.0f);
            break;
        case 2:
        case 0:
            em->velZ = 10.0f + Math_Sin((6.2831855f * (f32) (em->timer << 10)) / 65536.0f);
            break;
        case 3:
            em->velZ = 15.0f + 1.5f * Math_Sin((6.2831855f * (f32) (em->timer << 10)) / 65536.0f);
            break;
        }
        em->timer++;
        EmYamaTsukami_AnglesToPoint(em, (EmWork *) w);
        Em_MoveRotated(em, &w->accelAngX);
        if (Vec_DistanceXZ(&em->posX, em->goal) < 10.0f) {
            em->posX = em->goal[0];
            em->posZ = em->goal[2];
        }
        if (Math_Absf(em->goal[1] - em->posY) < 10.0f) {
            em->posY = em->goal[1];
        }
        if ((em->goalDist -= em->velZ) <= 0.0f) {
            em->step = 3;
            if (variant == 1) {
                Em_SetMotion(em, 0x18, 0, 0);
            }
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_EndState2(em);
        }
        break;
    }
}

void EmYamaTsukami_State2_07(Em *em, s32 variant)
{
    EmYamaTsukamiWork *w = (EmYamaTsukamiWork *)&em->work;

    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        Em_SetMotion(em, 4, 0, 0);
        EmYamaTsukami_StartCurves(em, 2U);
        if (variant != 0) {
            w->unk038 = 1;
        }
        break;
    case 1:
        EmYamaTsukami_UpdateCurves(em);
        if ((em->motion[0].playing == 0) || !(em->posY <= em->goal[1])) {
            em->step++;
            Em_SetMotion(em, 0xB, 0, 0);
            EmYamaTsukami_StartCurves(em, 3U);
        }
        break;
    case 2:
        EmYamaTsukami_UpdateCurves(em);
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_EndState2(em);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmYamaTsukami_State2_09(Em *em, s32 variant)
{
    EmYamaTsukamiWork *w = (EmYamaTsukamiWork *)&em->work;

    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        Em_SetMotion(em, 2, 0, 0);
        EmYamaTsukami_StartCurves(em, 0U);
        if (variant != 0) {
            w->unk038 = 1;
        }
        break;
    case 1:
        EmYamaTsukami_UpdateCurves(em);
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_EndState2(em);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmYamaTsukami_State2_0A(Em *em, s32 variant)
{
    EmYamaTsukamiWork *w = (EmYamaTsukamiWork *)&em->work;

    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        Em_SetMotion(em, 3, 0, 0);
        EmYamaTsukami_StartCurves(em, 1U);
        if (variant != 0) {
            w->unk038 = 1;
        }
        break;
    case 1:
        EmYamaTsukami_UpdateCurves(em);
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_EndState2(em);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmYamaTsukami_State2_0B(Em *em, s32 variant)
{
    EmYamaTsukamiWork *w = (EmYamaTsukamiWork *)&em->work;

    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        Em_SetMotion(em, 5, 0, 0);
        EmYamaTsukami_StartCurves(em, 4U);
        if (variant != 0) {
            w->unk038 = 1;
        }
        break;
    case 1:
        EmYamaTsukami_UpdateCurves(em);
        if (Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) < 500.0f) {
            em->step++;
            EmYamaTsukami_EndState2(em);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmYamaTsukami_State2_0C(Em *em, s32 variant)
{
    EmYamaTsukamiWork *w = (EmYamaTsukamiWork *)&em->work;

    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        Em_SetMotion(em, 6, 0, 0);
        EmYamaTsukami_StartCurves(em, 5U);
        if (variant != 0) {
            w->unk038 = 1;
        }
        break;
    case 1:
        EmYamaTsukami_UpdateCurves(em);
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_EndState2(em);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmYamaTsukami_State2_11(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        Em_SetMotion(em, 7, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_EndState2(em);
        }
        break;
    }
}

void EmYamaTsukami_State2_12(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        Em_SetMotion(em, 0xC, 0, 0);
        EmYamaTsukami_StartCurves(em, 0xC);
        em->timer = 5;
        break;
    case 1:
        EmYamaTsukami_UpdateCurves(em);
        if (em->motion[0].playing == 0) {
            EmYamaTsukami_EndState2(em);
        }
        break;
    }
}

void EmYamaTsukami_State2_14(Em *em)
{
    s32 i;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        Em_SetMotion(em, 1, 0, 0);
        break;
    case 1:
        if (em->remote == 0) {
            for (i = 0; i < gScene.playerCount; i++) {
                if (gScene.inQuest[i] == 1 && em->area == gPlayers[i].area) {
                    EmYamaTsukami_EndState2(em);
                    break;
                }
            }
        }
        break;
    }
}

void EmYamaTsukami_State2_16(Em *em)
{
    EmYamaTsukamiWork *w = (EmYamaTsukamiWork *)&em->work;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        Player_ClearVelocity(em);
        em->goalDist = Vec_Distance((f32 *) &em->posX, (f32 *) em->goal);
        em->timer = 0;
        Em_SetMotion(em, 0x16, 0, 0);
        break;
    case 1:
        em->velY = -10.0f + Math_Sin((6.2831855f * (f32) (em->timer << 0xA)) / 65536.0f);
        em->timer++;
        Em_MoveRotated(em, &w->accelAngX);
        if (!(em->floorY < em->posY)) {
            em->step++;
            em->posY = em->floorY;
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_EndState2(em);
        }
        break;
    }
    if (em->posY < em->floorY) {
        em->posY = em->floorY;
    }
}

void EmYamaTsukami_RunGroup2(Em *em)
{
    switch (em->state) {
    case 0:
        EmYamaTsukami_State2_00(em);
        break;
    case 1:
        EmYamaTsukami_State2_01(em);
        break;
    case 2:
        EmYamaTsukami_State1_02(em, 1);
        break;
    case 3:
        EmYamaTsukami_State1_02(em, 2);
        break;
    case 4:
        EmYamaTsukami_State2_04(em, 0);
        break;
    case 5:
        EmYamaTsukami_State2_04(em, 2);
        break;
    case 6:
        EmYamaTsukami_State2_04(em, 1);
        break;
    case 7:
        EmYamaTsukami_State2_07(em, 0);
        break;
    case 8:
        EmYamaTsukami_State2_07(em, 1);
        break;
    case 9:
        EmYamaTsukami_State2_09(em, 0);
        break;
    case 10:
        EmYamaTsukami_State2_0A(em, 0);
        break;
    case 11:
        EmYamaTsukami_State2_0B(em, 0);
        break;
    case 12:
        EmYamaTsukami_State2_0C(em, 0);
        break;
    case 13:
        EmYamaTsukami_State2_09(em, 1);
        break;
    case 14:
        EmYamaTsukami_State2_0A(em, 1);
        break;
    case 15:
        EmYamaTsukami_State2_0B(em, 1);
        break;
    case 16:
        EmYamaTsukami_State2_0C(em, 1);
        break;
    case 17:
        EmYamaTsukami_State2_11(em);
        break;
    case 18:
        EmYamaTsukami_State2_12(em);
        break;
    case 19:
        EmYamaTsukami_State2_04(em, 3);
        break;
    case 20:
        EmYamaTsukami_State2_14(em);
        break;
    case 21:
        EmYamaTsukami_State2_09(em, 0);
        break;
    case 22:
        EmYamaTsukami_State2_16(em);
        
    default:
        break;
    }
}

void EmYamaTsukami_State3_00(Em *em)
{
    s32 i;
    Em *m;

    switch (em->step) {
    case 0:
        em->step++;
        em->stateVar1 = 5;
        em->stateVar0 = 5;
        em->lookAtTarget = 0;
        em->stance = 2;
        em->timer = 0x8C;
        Em_SetMotion(em, 0x31, 0, 0);
        for (i = 0, m = gEms; i < 20; i++, m++) {
            if (m->type == 0x38 && m->active != 0 && em->stateVar0 > 0) {
                em->stateVar0--;
            }
        }
        break;
    case 1:
        if (--em->timer < 0) {
            if (em->stateVar0 > 0) {
                em->stateVar0--;
                EmKind38_SpawnAtEm(em);
            }
            if (em->stateVar1 > 0) {
                em->stateVar1--;
                em->timer = 0x14;
            } else {
                em->step++;
                Em_SetMotion(em, 0x32, 0, 0);
            }
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_EndState2(em);
        }
        break;
    }
}

static u8 EmYamaTsukami_InterpKeys(Em *em, EmYamaTsukamiWork *w, s32 idx)
{
    u8 ret;
    EmYamaKey *p;
    s32 i;
    EmYamaKey *tbl;
    s32 n;
    s32 t;
    s32 d;

    n = w->curveFrame;
    ret = 0;
    tbl = EmYamaTsukami_Keys[idx];
    d = 0;
    if ((u32) n > 1) {
        i = 1;
        while (i != 0) {
            p = &tbl[i];
            t = p->t;
            if (n > t && t > 0.0f) {
                i++;
            } else if (t == 0.0f) {
                ret = 2;
                break;
            } else if (t < 0) {
                w->curveFrame = 0;
                ret = 2;
                break;
            } else {
                i = 0;
                d = (p->v - p[-1].v) / ((t - p[-1].t) / em->motion[0].step);
            }
        }
    }
    em->rotY += (u16) (s32) (0.5f + 65536.0f * d / 360.0f);
    return ret;
}

void EmYamaTsukami_State3_01(Em *em, s32 variant)
{
    EmYamaTsukamiWork *w = (EmYamaTsukamiWork *)&em->work;
    f32 dy;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        Em_SetMotion(em, 0x2E, 0, 0);
        EmYamaTsukami_StartCurves(em, 6);
        break;
    case 1:
        EmYamaTsukami_UpdateCurves(em);
        EmYamaTsukami_InterpKeys(em, w, 0);
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x2F, 0, 0);
            em->timer = 10;
            w->unk01C = -0x555;
            if (variant == 0) {
                em->velY = (em->goal[1] - em->posY) / 160.0f;
            }
            YamaTsukamiAttack_Create(em, 0);
        }
        break;
    case 2:
        em->rotY += w->unk01C;
        if (variant == 0) {
            dy = em->goal[1] - em->posY;
            if (em->velY > dy) {
                em->velY = dy;
            }
            em->posY += em->velY;
        }
        if (em->motion[0].playing == 0 && em->timer > 0) {
            em->timer--;
            w->unk01C -= 0x71;
        }
        if (variant == 0) {
            if (Math_Absf(em->goal[1] - em->posY) < 100.0f && em->timer <= 0) {
                em->step++;
                em->timer = 10;
            }
        } else if (em->timer <= 0) {
            em->step++;
            em->timer = 10;
        }
        break;
    case 3:
        em->rotY += w->unk01C;
        if (em->motion[0].playing == 0) {
            if (--em->timer < 0) {
                em->step++;
                Em_SetMotion(em, 0x30, 0, 0);
                EmYamaTsukami_StartCurves(em, 7);
            } else {
                w->unk01C += 0x71;
                Em_SetMotion(em, 0x2F, 0, 0);
            }
        }
        break;
    case 4:
        EmYamaTsukami_UpdateCurves(em);
        EmYamaTsukami_InterpKeys(em, w, 1);
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_EndState2(em);
        }
        break;
    }
    if (em->posY < em->floorY) {
        em->posY = em->floorY;
    }
}

void EmYamaTsukami_State3_02(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        Em_SetMotion(em, 0x26, 0, 0);
        em->timer = 0xC8;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x27, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_EndState2(em);
        }
        break;
    }
}

void EmYamaTsukami_State3_03(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        Em_SetMotion(em, 0x1E, 0, 0);
        EmYamaTsukami_InitBoneFloorFlags(em);
        break;
    case 1:
        EmYamaTsukami_SpawnBoneFloorEffects(em);
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_EndState2(em);
        }
        break;
    }
}

void EmYamaTsukami_State3_04(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        Em_SetMotion(em, 0x1F, 0, 0);
        EmYamaTsukami_InitBoneFloorFlags(em);
        break;
    case 1:
        EmYamaTsukami_SpawnBoneFloorEffects(em);
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_EndState2(em);
        }
        break;
    }
}

void EmYamaTsukami_State3_05(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        Em_SetMotion(em, 0x22, 0, 0);
        EmYamaTsukami_InitBoneFloorFlags(em);
        break;
    case 1:
        EmYamaTsukami_SpawnBoneFloorEffects(em);
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_EndState2(em);
        }
        break;
    }
}

void EmYamaTsukami_State3_06(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        Em_SetMotion(em, 0x23, 0, 0);
        EmYamaTsukami_InitBoneFloorFlags(em);
        break;
    case 1:
        EmYamaTsukami_SpawnBoneFloorEffects(em);
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_EndState2(em);
        }
        break;
    }
}

void EmYamaTsukami_State3_07(Em *em, s32 variant)
{
    struct {
        u8 unk0;
        s8 unk1;
    } *w = (void *) &em->work;
    s8 n;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        Em_SetMotion(em, 0x24, 0, 0);
        em->timer = 0x1C2;
        EmYamaTsukami_InitBoneFloorFlags(em);
        break;
    case 1:
        EmYamaTsukami_SpawnBoneFloorEffects(em);
        if ((variant == 1) && (EmMotion_PassedFrameScaled(em, 170.0f, 0U) != 0)) {
            if (em->remote == 0) {
                n = w->unk1;
                if (n < 0xA) {
                    w->unk1 = n + 1;
                    if (w->unk1 >= 0xA) {
                        Quest_TriggerEvent(em);
                    }
                    gScene.fortStage = w->unk1;
                }
            }
            EmYamaTsukami_SpawnAreaImpactEffects(em);
        }
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x25, 0, 0);
        }
        break;
    case 2:
        EmYamaTsukami_SpawnBoneFloorEffects(em);
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_EndState2(em);
        }
        break;
    }
}

void EmYamaTsukami_State3_08(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        Em_SetMotion(em, 0x34, 0, 0);
        EmYamaTsukami_StartCurves(em, 8);
        break;
    case 1:
        EmYamaTsukami_UpdateCurves(em);
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_EndState2(em);
        }
        break;
    }
}

void EmYamaTsukami_State3_09(Em *em)
{
    em->guardTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        Em_SetMotion(em, 0x33, 0, 0);
        em->noReact = 1;
        if (!(em->velY <= -50.0f)) {
            em->velY = -50.0f;
        }
        em->accelX = 0.0f;
        em->accelY = -10.0f;
        em->accelZ = 0.0f;
        break;
    case 1:
        Player_Integrate(em);
        if (em->posY <= em->floorY) {
            em->step++;
            Em_SetMotion(em, 0x35, 0, 0);
            em->posY = em->floorY;
            em->stance = 0;
            Player_ClearVelocity(em);
            em->timer = 0x1C2;
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x36, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_EndState(em);
        }
        break;
    }
}

void EmYamaTsukami_State3_0A(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        Em_SetMotion(em, 0x20, 0, 0);
        EmYamaTsukami_InitBoneFloorFlags(em);
        break;
    case 1:
        EmYamaTsukami_SpawnBoneFloorEffects(em);
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_EndState2(em);
        }
        break;
    }
}

void EmYamaTsukami_State3_0B(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        Em_SetMotion(em, 0x21, 0, 0);
        EmYamaTsukami_InitBoneFloorFlags(em);
        break;
    case 1:
        EmYamaTsukami_SpawnBoneFloorEffects(em);
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_EndState2(em);
        }
        break;
    }
}

void EmYamaTsukami_State3_0C(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        Em_SetMotion(em, 0x2A, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 0xF0;
            Em_SetMotion(em, 0x28, 0, 0);
        }
        break;
    case 2:
        if (EmMotion_ReachedFrame(em, 0, 100.0f) != 0) {
            em->step++;
            YamaTsukamiAttack_Create(em, 0x10);
            EmEffectAttack_Create(em, 0, 0, 0, 0);
        }
        break;
    case 3:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x29, 0, 0);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_EndState2(em);
        }
        break;
    }
}

void EmYamaTsukami_State3_0D(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        Em_SetMotion(em, 0x2A, 0, 0);
        em->timer = 0xA;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            if (--em->timer <= 0) {
                em->step++;
                Em_SetMotion(em, 0x2D, 0, 0);
                break;
            }
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_EndState2(em);
        }
        break;
    }
}

void EmYamaTsukami_State3_0E(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        em->timer = 0x78;
        Em_SetMotion(em, 0x2B, 0, 0);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            em->timer = 0xC8;
            Em_SetMotion(em, 0x2C, 0, 0);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x2D, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_EndState2(em);
        }
        break;
    }
}

void EmYamaTsukami_RunGroup3(Em *em)
{
    switch (em->state) {
    case 0:
        EmYamaTsukami_State3_00(em);
        break;
    case 1:
        EmYamaTsukami_State3_01(em, 0);
        break;
    case 2:
        EmYamaTsukami_State3_02(em);
        break;
    case 3:
        EmYamaTsukami_State3_03(em);
        break;
    case 4:
        EmYamaTsukami_State3_04(em);
        break;
    case 5:
        EmYamaTsukami_State3_05(em);
        break;
    case 6:
        EmYamaTsukami_State3_06(em);
        break;
    case 7:
        EmYamaTsukami_State3_07(em, 0);
        break;
    case 8:
        EmYamaTsukami_State3_08(em);
        break;
    case 9:
        EmYamaTsukami_State3_09(em);
        break;
    case 10:
        EmYamaTsukami_State3_0A(em);
        break;
    case 11:
        EmYamaTsukami_State3_0B(em);
        break;
    case 12:
        EmYamaTsukami_State3_0C(em);
        break;
    case 13:
        EmYamaTsukami_State3_0D(em);
        break;
    case 14:
        EmYamaTsukami_State3_0E(em);
        break;
    case 15:
        EmYamaTsukami_State3_01(em, 1);
        break;
    case 16:
        EmYamaTsukami_State3_07(em, 1);
        
    default:
        break;
    }
}

void EmYamaTsukami_State4_00(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 8, 0, 0);
        if (!(em->velY <= -50.0f)) {
            em->velY = -50.0f;
        }
        em->accelX = 0.0f;
        em->accelY = -10.0f;
        em->accelZ = 0.0f;
        break;
    case 1:
        Player_Integrate(em);
        if (em->posY <= em->floorY) {
            em->step++;
            em->posY = em->floorY;
            em->stance = 0;
            Em_SetMotion(em, 9, 0, 0);
            Player_ClearVelocity(em);
            EmMine_CreateTimed0(em, &em->posX);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Player_ClearVelocity(em);
            EmYamaTsukami_ChangeState(em, 0, 2, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmYamaTsukami_ChangeState(em, 0, 2, 4);
        }
        break;
    }
}

void EmYamaTsukami_State4_01(Em *em)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, 8, 0, 0);
        if (!(em->velY <= -50.0f)) {
            em->velY = -50.0f;
        }
        em->goalDist = 1000.0f;
        em->accelX = 0.0f;
        em->accelY = -10.0f;
        em->accelZ = 0.0f;
        break;
    case 1:
        Player_Integrate(em);
        em->goalDist -= em->velY;
        if (!(em->goalDist <= 0.0f)) {
            if (em->posY <= em->floorY) {
                goto block_12;
            }
        } else {
block_12:
            em->step++;
            em->stance = 0;
            Em_SetMotion(em, 0xA, 0, 0);
            Player_ClearVelocity(em);
            EmYamaTsukami_StartCurves(em, 0xB);
        }
        break;
    case 2:
        EmYamaTsukami_UpdateCurves(em);
        if (em->motion[0].playing == 0) {
            em->step++;
            Player_ClearVelocity(em);
            EmYamaTsukami_ChangeState(em, 2, 1, 4);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmYamaTsukami_ChangeState(em, 2, 1, 4);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

void EmYamaTsukami_State4_02(Em *em, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        if (variant == 0) {
            em->stance = 2;
        } else {
            em->stance = 0;
        }
        Em_SetMotion(em, 0x3C, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            if (variant == 0) {
                EmYamaTsukami_EndState2(em);
                break;
            }
            EmYamaTsukami_EndState(em);
        } else {
        }
        break;
    }
}

void EmYamaTsukami_State4_03(Em *em, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        Em_SetMotion(em, 0x3D, 0, 0);
        if (variant == 0) {
            em->stance = 2;
        } else {
            em->stance = 0;
        }
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            if (variant == 0) {
                EmYamaTsukami_EndState2(em);
                break;
            }
            EmYamaTsukami_EndState(em);
        } else {
        }
        break;
    }
}

void EmYamaTsukami_State4_04(Em *em, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        Em_SetMotion(em, 0x43, 0, 0);
        if (variant == 0) {
            em->stance = 2;
        } else {
            em->stance = 0;
        }
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            if (variant == 0) {
                EmYamaTsukami_EndState2(em);
                break;
            }
            EmYamaTsukami_EndState(em);
        } else {
        }
        break;
    }
}

void EmYamaTsukami_State4_06(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        Em_SetMotion(em, 0x40, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_EndState2(em);
        }
        break;
    }
}

void EmYamaTsukami_State4_07(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        Em_SetMotion(em, 0x40, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_EndState2(em);
        }
        break;
    }
}

void EmYamaTsukami_State4_08(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 0;
        Em_SetMotion(em, 0x3E, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_ChangeState(em, 0, 2, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmYamaTsukami_ChangeState(em, 0, 2, 4);
        }
        break;
    }
}

void EmYamaTsukami_State4_0D(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 2;
        Em_SetMotion(em, 0x42, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_ChangeState(em, 2, 1, 4);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmYamaTsukami_ChangeState(em, 2, 1, 4);
        }
        break;
    }
}

void EmYamaTsukami_RunGroup4(Em *em)
{
    switch (em->state) {
    case 0:
        EmYamaTsukami_State4_00(em);
        break;
    case 1:
        EmYamaTsukami_State4_01(em);
        break;
    case 2:
        EmYamaTsukami_State4_02(em, 0);
        break;
    case 3:
        EmYamaTsukami_State4_03(em, 0);
        break;
    case 4:
        EmYamaTsukami_State4_04(em, 0);
        break;
    case 5:
        EmYamaTsukami_State4_04(em, 0);
        break;
    case 6:
        EmYamaTsukami_State4_06(em);
        break;
    case 7:
        EmYamaTsukami_State4_07(em);
        break;
    case 8:
        EmYamaTsukami_State4_08(em);
        break;
    case 9:
        EmYamaTsukami_State4_02(em, 1);
        break;
    case 10:
        EmYamaTsukami_State4_03(em, 1);
        break;
    case 11:
        EmYamaTsukami_State4_04(em, 1);
        break;
    case 12:
        EmYamaTsukami_State4_04(em, 1);
        break;
    case 13:
        EmYamaTsukami_State4_0D(em);
        
    default:
        break;
    }
}

void EmYamaTsukami_State5_00_Hunted(Em *em)
{
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x3F, 0, 0);
        em->timer = 0;
        em->lookAtTarget = 0;
        em->stance = 3;
        QuestEm_Hunted(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmMine_Create(em, 0);
            em->fieldCollide = 0;
        }
        break;
    case 2:
        EmMine_Update(em);
        break;
    case 0x63:
        em->step++;
        EmYamaTsukami_RemoveMinePoints(em);
        em->hp = 0x3415;
        EmYamaTsukami_EndState(em);
        break;
    }
}

void EmYamaTsukami_State5_01_HuntedFalling(Em *em)
{
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 2;
        Em_SetMotion(em, 8, 0, 0);
        QuestEm_Hunted(em);
        EmYamaTsukami_StartCurves(em, 0xA);
        em->accelX = 0.0f;
        em->accelY = -10.0f;
        *(s32 *) &em->accelZ = 0;
        break;
    case 1:
        EmYamaTsukami_UpdateCurves(em);
        if (em->posY <= em->floorY) {
            em->step++;
            em->posY = em->floorY;
            em->stance = 3;
            Em_SetMotion(em, 9, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmMine_Create(em, 0);
            em->fieldCollide = 0;
            Em_SetMotion(em, 0x3F, 0, 0);
        }
        break;
    case 3:
        EmMine_Update(em);
        break;
    case 0x63:
        em->step++;
        em->hp = 0x3415;
        EmYamaTsukami_RemoveMinePoints(em);
        EmYamaTsukami_EndState(em);
        break;
    }
}

void EmYamaTsukami_RunGroup5(Em *em)
{
    switch (em->state) {
    case 0:
        EmYamaTsukami_State5_00_Hunted(em);
        break;
    case 1:
        EmYamaTsukami_State5_01_HuntedFalling(em);
        break;
    }
}

void EmYamaTsukami_State6_00(Em *em)
{
    f32 dist;
    f32 d;
    f32 d2;
    u8 step;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    step = em->step;
    switch (step) {
    case 0:
        GmEm_ResetSize();
        em->step++;
        em->scriptPending = 0;
        em->stance = 2;
        Em_SetMotion(em, 0x16, 0, 0);
        em->timer = 0x12C;
        Player_ClearVelocity(em);
        em->velZ = 20.0f;
        em->posX = 12650.0f;
        em->posY = 3360.0f;
        em->posZ = 8900.0f;
        em->goal[0] = 4010.0f;
        em->goal[1] = 3360.0f;
        em->goal[2] = 10544.0f;
        em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        break;
    case 1:
        Em_MoveRotated(em, &em->rotX);
        dist = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer > 0) {
            if (dist <= 100.0f) {
                goto block_10;
            }
            break;
        }
block_10:
        em->step++;
        em->timer = 0x12C;
        em->posX = 12650.0f;
        em->posY = 3360.0f;
        em->posZ = 8900.0f;
        em->goal[0] = 695.0f;
        em->goal[1] = 3360.0f;
        em->goal[2] = 11203.0f;
        em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        break;
    case 2:
        Em_MoveRotated(em, &em->rotX);
        d = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer > 0) {
            if (d <= 100.0f) {
                goto block_14;
            }
        } else {
block_14:
            em->step++;
            em->timer = 0x384;
            em->posX = 4123.0f;
            em->posY = 5000.0f;
            em->posZ = 14785.0f;
            em->goal[0] = -3000.0f;
            em->goal[1] = 5000.0f;
            em->goal[2] = 8036.0f;
            em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        }
        break;
    case 3:
        Em_MoveRotated(em, &em->rotX);
        d2 = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer > 0) {
            if (d2 <= 100.0f) {
                goto block_19;
            }
        } else {
block_19:
            em->step++;
            Em_SetMotion(em, 1, 0, 0);
        }
        break;
    case 4:
        if (Save_TestEventFlag(0xAF) == 1) {
            em->area = Area_IdForCurrentTime(AREA_TOWER_3);
            em->rotY = 0x4FE0;
            em->posX = 4510.0f;
            em->posY = -2619.0f;
            em->posZ = 12860.0f;
            EmYamaTsukami_Setup(em);
            EmYamaTsukami_ChangeState(em, 2, 0x14, 0);
        }
        break;
    }
}

void EmYamaTsukami_State6_01(Em *em)
{
    f32 pos[3];
    s32 d;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 1;
        Em_SetMotion(em, 0x40, 0, 0);
        em->posX = 6070.0f;
        em->posY = 0.0f;
        em->posZ = 17350.0f;
        em->goal[0] = 3600.0f;
        em->goal[1] = 0.0f;
        em->goal[2] = 19500.0f;
        em->rotY = Math_AngleXZ(&em->posX, em->goal) - 0x4000;
        em->goalDist = Vec_DistanceXZ(&em->posX, em->goal) - 500.0f;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x41, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].blend == 0) {
            if (!EmMotion_ReachedFrame(em, 0, 64.0f)) {
                em->rotY += 0x210;
            }
            if (em->motion[0].playing == 0) {
                em->step++;
                Em_SetMotion(em, 3, 0, 0);
                Em_SetMotion(em, 0x42, 0, 0);
            }
        }
        break;
    case 3:
        if (em->motion[1].playing == 0 && em->motionId[1] == 0x4F2) {
            Em_SetLayerMotion(em, 0x4B3, 0, 0, 1);
        }
        d = ((Math_AngleXZ(&em->posX, em->goal) & 0xFFFF) - em->rotY) & 0xFFFF;
        if (d <= 0x8000) {
            if (d <= 0x3F) {
                em->rotY += d;
            } else {
                em->rotY += 0x40;
            }
        } else {
            if (d > 0xFFC0) {
                em->rotY += d;
            } else {
                em->rotY -= 0x40;
            }
        }
        Em_MotionDelta(em, pos);
        if ((em->goalDist -= pos[2]) <= 0.0f) {
            em->goal[0] = 430.0f;
            em->goal[1] = 0.0f;
            em->goal[2] = 23930.0f;
            em->step++;
            em->goalDist = Vec_DistanceXZ(&em->posX, em->goal);
        }
        break;
    case 4:
        d = ((Math_AngleXZ(&em->posX, em->goal) & 0xFFFF) - em->rotY) & 0xFFFF;
        if (d <= 0x8000) {
            if (d <= 0x3F) {
                em->rotY += d;
            } else {
                em->rotY += 0x40;
            }
        } else {
            if (d > 0xFFC0) {
                em->rotY += d;
            } else {
                em->rotY -= 0x40;
            }
        }
        Em_MotionDelta(em, pos);
        if ((em->goalDist -= pos[2]) <= 0.0f) {
            em->step++;
            Em_SetMotion(em, 3, 0, 0);
        }
        break;
    case 5:
        em->step++;
        EmYamaTsukami_EndState2(em);
        break;
    }
}

void EmYamaTsukami_RunGroup6(Em *em)
{
    switch (em->state) {
    case 0:
        EmYamaTsukami_State6_00(em);
        break;
    case 1:
        EmYamaTsukami_State6_01(em);
        break;
    }
}

void EmYamaTsukami_Update(Em *em)
{
    EmYamaTsukamiWork *w = (EmYamaTsukamiWork *)&em->work;
    f32 pos[3];
    u8 buf[4];

    if (w->mineSlot != -1) {
        EmYamaTsukami_GetMinePointPos(em, pos);
        MinePoint_SetPos(w->mineSlot, pos);
        if ((w->mineCount = MinePoint_Count(w->mineSlot)) <= 0) {
            MinePoint_Remove(w->mineSlot);
            w->mineSlot = -1;
        }
    }
    if (em->type == 0x48) {
        em->blind = 2;
    } else if (w->syncTimer != 0 && em->stateGroup != 6 && em->remote == 0) {
        if (--w->syncTimer <= 0) {
            w->syncTimer = 150;
            QSync_SendEm(em, QSYNC_EMMSG_STATE_NO_PARTS, 0);
        }
    }
    if (*(s8 *) &em->quakeTimer != 0) {
        if (--*(s8 *) &em->quakeTimer <= 0) {
            em->quakeTimer = 0;
        }
    }
    switch (Em_UpdateDamage(em, buf) & 0xFF) {
    case 0:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 13:
    case 14:
    case 15:
    case 16:
    case 18:
        break;
    case 1:
    case 2:
        if (em->stance == 0) {
            EmYamaTsukami_ChangeState(em, 5, 0, 3);
        } else {
            EmYamaTsukami_ChangeState(em, 5, 1, 3);
        }
        break;
    case 17:
        Em_EndStatus5(em);
        break;
    case 12:
        if (em->stance == 0) {
            if (em->stateGroup == 0 && em->state == 2) {
                EmYamaTsukami_ChangeState(em, 4, 8, 3);
            } else {
                switch (w->unk02E) {
                case 0:
                    EmYamaTsukami_ChangeState(em, 4, 9, 3);
                    break;
                case 1:
                    EmYamaTsukami_ChangeState(em, 4, 0xA, 3);
                    break;
                case 2:
                    switch (w->easeIdx) {
                    case 0:
                        EmYamaTsukami_ChangeState(em, 4, 0, 3);
                        break;
                    case 1:
                        EmYamaTsukami_ChangeState(em, 4, 0xC, 3);
                        break;
                    case 2:
                        EmYamaTsukami_ChangeState(em, 4, 0xB, 3);
                        break;
                    }
                    break;
                }
            }
        } else if (em->stateGroup == 2 && em->state == 1) {
            EmYamaTsukami_ChangeState(em, 4, 0xD, 3);
        } else {
            switch (w->unk02E) {
            case 0:
                EmYamaTsukami_ChangeState(em, 4, 2, 3);
                break;
            case 1:
                EmYamaTsukami_ChangeState(em, 4, 3, 3);
                break;
            case 2:
                switch (w->easeIdx) {
                case 0:
                    if (!(em->posY <= 2000.0f + em->floorY)) {
                        EmYamaTsukami_ChangeState(em, 4, 1, 3);
                    } else {
                        EmYamaTsukami_ChangeState(em, 4, 0, 3);
                    }
                    break;
                case 1:
                    EmYamaTsukami_ChangeState(em, 4, 5, 3);
                    break;
                case 2:
                    EmYamaTsukami_ChangeState(em, 4, 4, 3);
                    break;
                }
                break;
            }
        }
        break;
    }
    if (em->parts[6].state >= 8) {
        Em_BreakPart(em, 0);
    }
    if (em->type == 0x3A && Area_DayId(em->area) == AREA_TOWER_0 && Save_TestEventFlag(0xAF) == 0) {
        if (Area_DayId(gScene.area) == AREA_TOWER_0 && gScene.eventRunning == 1 && em->stateGroup != 6) {
            EmYamaTsukami_ChangeState(em, 6, 0, 0);
        }
    } else {
        Em_UpdatePendingScript(em);
    }
    Em_ClearHitReactMode(em);
    EmYamaTsukami_RunState(em);
    if (em->stateRequest != 0) {
        EmYamaTsukami_RunState(em);
        em->stateRequest = 0;
    }
    if (w->unk038 != 0) {
        em->guardTimer = 2;
    }
    if (func_00849C20(em) == 1) {
        em->quakeTimer = 0;
    } else {
        em->quakeTimer = 5;
    }
}

void EmYamaTsukami_RunState(Em *em)
{
    switch (em->stateGroup) {
    case 0:
        EmYamaTsukami_RunGroup0(em);
        break;
    case 1:
        EmYamaTsukami_RunGroup1(em);
        break;
    case 2:
        EmYamaTsukami_RunGroup2(em);
        break;
    case 3:
        EmYamaTsukami_RunGroup3(em);
        break;
    case 4:
        EmYamaTsukami_RunGroup4(em);
        break;
    case 5:
        EmYamaTsukami_RunGroup5(em);
        break;
    case 6:
        EmYamaTsukami_RunGroup6(em);
        break;
    case 7:
        EmYamaTsukami_RunGroup6(em);
        
    default:
        break;
    }
}

static void EmYamaTsukami_MotionEffectsDefault(Em *em)
{
}

void EmYamaTsukami_MotionEffects(Em *em, EmWork *work)
{
    EmYamaWork *w = (EmYamaWork *) work;
    f32 pos[3];

    if (em->motionId[0] != w->unk08) {
        w->unk0A = 0;
        w->unk08 = (s16) em->motionId[0];
    }
    EmYamaTsukami_MotionSounds(em, (EmWork *) w);
    switch (w->unk08) {
    case 0x3EE:
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            EmImpactAttack_CreateAt(em, &em->posX, 0xA);
        }
        if (EmMotion_PassedFrameScaled(em, 100.0f, 0) != 0) {
            EmImpactAttack_CreateAt(em, &em->posX, 0xA);
        }
        break;
    case 0x3F1:
        if (EmMotion_PassedFrameScaled(em, 36.0f, 0) != 0) {
            StageObj_K14_Create(em, 0, 0, 4.0f * em->scaleX);
            YamaTsukamiAttack_Create(em, 0x14);
        }
        break;
    case 0x3F2:
        break;
    case 0x3FD:
        if (EmMotion_PassedFrameScaled(em, 74.0f, 0) != 0) {
            StageObj_K14_Create(em, 0x13, 0, 4.0f * em->scaleX);
        }
        break;
    case 0x406:
        if (EmMotion_PassedFrameScaled(em, 44.0f, 0) != 0) {
            YamaTsukamiAttack_Create(em, 2);
        }
        if (EmMotion_PassedFrameScaled(em, 134.0f, 0) != 0) {
            YamaTsukamiAttack_Create(em, 3);
        }
        break;
    case 0x407:
        if (EmMotion_PassedFrameScaled(em, 44.0f, 0) != 0) {
            YamaTsukamiAttack_Create(em, 4);
        }
        if (EmMotion_PassedFrameScaled(em, 134.0f, 0) != 0) {
            YamaTsukamiAttack_Create(em, 5);
        }
        break;
    case 0x40A:
        if (EmMotion_PassedFrameScaled(em, 44.0f, 0) != 0) {
            YamaTsukamiAttack_Create(em, 6);
        }
        if (EmMotion_PassedFrameScaled(em, 124.0f, 0) != 0) {
            YamaTsukamiAttack_Create(em, 7);
        }
        break;
    case 0x40B:
        if (EmMotion_PassedFrameScaled(em, 44.0f, 0) != 0) {
            YamaTsukamiAttack_Create(em, 8);
        }
        if (EmMotion_PassedFrameScaled(em, 124.0f, 0) != 0) {
            YamaTsukamiAttack_Create(em, 9);
        }
        break;
    case 0x40C:
        if (EmMotion_PassedFrameScaled(em, 156.0f, 0) != 0) {
            YamaTsukamiAttack_Create(em, 0xA);
            YamaTsukamiAttack_Create(em, 0x15);
        }
        break;
    case 0x41C:
        if (EmMotion_PassedFrameScaled(em, 70.0f, 0) != 0) {
            YamaTsukamiAttack_Create(em, 0x16);
        }
        if ((EmMotion_PassedFrameScaled(em, 156.0f, 0) != 0) && (Player_InCurrentArea(em) & 0xFF)) {
            Em_BonePos(em, 2, pos);
            pos[1] -= 300.0f * em->scaleY;
            StageObj_K14_CreateAt(pos, 0x15, 0, 4.3f * em->scaleX);
            StageObj_K14_CreateAt(pos, 0xD, 0, 4.3f * em->scaleX);
            EmImpactAttack_CreateAt(em, pos, 0xB);
        }
        break;
    case 0x41D:
        if (EmMotion_PassedFrameScaled(em, 86.0f, 0) != 0) {
            YamaTsukamiAttack_Create(em, 0xB);
        }
        if (EmMotion_PassedFrameScaled(em, 122.0f, 0) != 0) {
            StageObj_K14_Create(em, 0xB, 0, 4.0f * em->scaleX);
            YamaTsukamiAttack_Create(em, 0x13);
        }
        break;
    case 0x41E:
        if (EmMotion_PassedFrameScaled(em, 84.0f, 0) != 0) {
            StageObj_K14_Create(em, 0x13, 0, 4.0f * em->scaleX);
        }
        break;
    case 0x427:
        if (EmMotion_PassedFrameScaled(em, 182.0f, 0) != 0) {
            StageObj_K14_Create(em, 0, 0, 4.0f * em->scaleX);
        }
        break;
    case 0x408:
        if (EmMotion_PassedFrameScaled(em, 56.0f, 0) != 0) {
            YamaTsukamiAttack_Create(em, 0xC);
        }
        break;
    case 0x409:
        if (EmMotion_PassedFrameScaled(em, 56.0f, 0) != 0) {
            YamaTsukamiAttack_Create(em, 0xD);
        }
        break;
    case 0x412:
        if (EmMotion_PassedFrameScaled(em, 50.0f, 0) != 0) {
            YamaTsukamiAttack_Create(em, 0xE);
            YamaTsukamiAttack_Create(em, 0x15);
        }
        break;
    case 0x413:
        if (EmMotion_PassedFrameScaled(em, 42.0f, 0) != 0) {
            YamaTsukamiAttack_Create(em, 0xF);
            YamaTsukamiAttack_Create(em, 0x15);
        }
        break;
    default:
        EmYamaTsukami_MotionEffectsDefault(em);
        break;
    }
}

void EmYamaTsukami_EaseToTarget(Em *em, EmWork *work)
{
    EmYamaTsukamiWork *w = (EmYamaTsukamiWork *)work;

    switch (w->easeMode) {
    case 0:
        w->ease[0] = w->ease[0] + (((f32 (*)[3]) EmYamatsukami_EaseTargets)[w->easeIdx][0] - w->ease[0]) / 85.0f;
        w->ease[1] = w->ease[1] + (((f32 (*)[3]) EmYamatsukami_EaseTargets)[w->easeIdx][1] - w->ease[1]) / 85.0f;
        w->ease[2] = w->ease[2] + (((f32 (*)[3]) EmYamatsukami_EaseTargets)[w->easeIdx][2] - w->ease[2]) / 85.0f;
        break;
    }
}

/* Grows the scale (pose[0..2]) of bones 6, 4 and 0x23 by D_008CAAB0..D_008CAAD0 times the
 * eased factors ease[0], ease[1] and ease[2] (EmYamaTsukami_EaseToTarget); no caller in C. */
void func_00849700(Em *em, EmBone *bones)
{
    EmYamaTsukamiWork *w = (EmYamaTsukamiWork *)&em->work;

    bones[0x6].pose[0] = bones[0x6].pose[0] + D_008CAAB0 * w->ease[0];
    bones[0x6].pose[1] = bones[0x6].pose[1] + D_008CAAB4 * w->ease[0];
    bones[0x6].pose[2] = bones[0x6].pose[2] + D_008CAAB8 * w->ease[0];
    bones[0x4].pose[0] = bones[0x4].pose[0] + D_008CAABC * w->ease[1];
    bones[0x4].pose[1] = bones[0x4].pose[1] + D_008CAAC0 * w->ease[1];
    bones[0x4].pose[2] = bones[0x4].pose[2] + D_008CAAC4 * w->ease[1];
    bones[0x23].pose[0] = bones[0x23].pose[0] + D_008CAAC8 * w->ease[2];
    bones[0x23].pose[1] = bones[0x23].pose[1] + D_008CAACC * w->ease[2];
    bones[0x23].pose[2] = bones[0x23].pose[2] + D_008CAAD0 * w->ease[2];
}

void EmYamaTsukami_UpdateMotionEffects(Em *em)
{
    EmYamaTsukamiWork *work = (EmYamaTsukamiWork *)&em->work;

    switch (work->fxStep) {
    case 0:
        work->fxStep++;
        break;
    case 1:
        EmYamaTsukami_MotionEffects(em, (EmWork *) work);
        EmYamaTsukami_EaseToTarget(em, (EmWork *) work);
        break;
    }
}

void EmYamaTsukami_SpawnAreaImpactEffects(Em *em)
{
    f32 pos[3];

    switch (Area_DayId(em->area)) {
    case AREA_BATTLE_TOWN_1:
        pos[0] = 10069.0f;
        pos[1] = 1630.0f;
        pos[2] = 16847.0f;
        break;
    default:
        pos[0] = 7300.0f;
        pos[1] = 1500.0f;
        pos[2] = 12400.0f;
        break;
    }
    Em_PlaySeChecked(em, 0x45, 0, pos, 9, 0);
    Em_PlaySeChecked(em, 0x44, 0, pos, 9, 0);
    Effect02_SpawnForOwner(em, 0, 9, 0, pos, 3.0f);
    Effect02_SpawnForOwner(em, 0, 9, 1, pos, 3.0f);
    Effect0D_SpawnIfHere(em, pos, 0x22, 30.0f);
    CamShake_ReqARange(em, 2, 2);
}

void EmYamaTsukami_InitBoneFloorFlags(Em *em)
{
    EmYamaWork *w = (EmYamaWork *) &em->work;
    s32 i;
    s32 j;
    f32 pos[3];

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 2; j++) {
            if (gScene.area != em->area) {
                w->flags[i][j] = 0;
            } else {
                Em_BonePos(em, EmYamaTsukami_BoneFloorBones[i][j], pos);
                if (pos[1] <= AreaGrid2_GetFloorY(pos)) {
                    w->flags[i][j] = 0;
                } else {
                    w->flags[i][j] = 1;
                }
            }
        }
    }
}

void EmYamaTsukami_SpawnBoneFloorEffects(Em *em)
{
    EmYamaWork *w = (EmYamaWork *) &em->work;
    s32 i;
    s32 j;
    f32 pos[3];
    f32 g;
    f32 y;
    s8 *f;

    if (gScene.area != em->area) {
        return;
    }
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 2; j++) {
            Em_BonePos(em, EmYamaTsukami_BoneFloorBones[i][j], pos);
            g = AreaGrid2_GetFloorY(pos);
            f = &w->flags[i][j];
            switch (*f) {
            case 0:
                if (!(pos[1] <= g)) {
                    *f = 1;
                }
                break;
            case 1:
                y = pos[1];
                if (!(y <= g)) {
                    pos[1] = 1000.0f + em->posY;
                    g = AreaGrid2_GetFloorY(pos);
                    pos[1] = y;
                }
                if (pos[1] <= g) {
                    pos[1] = g;
                    Effect0D_SpawnAt(pos, 3, 10.0f * em->scaleX);
                    w->flags[i][1] = 2;
                    w->flags[i][0] = 2;
                }
                break;
            case 2:
                break;
            }
        }
    }
}

s32 func_00849C20(Em *em)
{
    u16 motionId;

    motionId = em->motionId[0];
    if ((motionId == 0x3F6) || (motionId == 0x42A)) {
        return 1;
    }
    return 0;
}

void func_00849C50(void)
{
}

u8 EmYamaTsukami_ScriptQuery(Em *em, s32 kind)
{
    EmYamaTsukamiWork *w = (EmYamaTsukamiWork *)&em->work;

    if ((kind & 0xFF) != 1) {
        return (u8)w->unk000;
    }
    switch (Area_DayId(em->area) & 0xFFFF) {
    case AREA_TOWER_0x53:
    case AREA_TOWER_3:
        return 1U;
    }
    return 0U;
}

void EmChameleos_MotionSounds(Em *em, EmWork *work)
{
    s16 motion;

    motion = work->sndMotion;
    switch (motion) {
    case 0x3E9:
        Em_PlaySeAtBoneOnFrame(em, 0x5E, 0x39, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x102, 0x38, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x182, 0x31, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x24E, 0x38, 0x26);
        break;
    case 0x3EA:
        Em_PlaySeAtBoneOnFrame(em, 0x8C, 0x39, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x80, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x11C, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x12A, 4, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x8A, 4, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x78, 3, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 3, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x8E, 3, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x148, 3, 0xD);
        break;
    case 0x3EB:
        Em_PlaySeAtBoneOnFrame(em, 8, 0x49, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 2, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 2, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 5, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 5, 8);
        break;
    case 0x3EC:
        Em_PlaySeAtBoneOnFrame(em, 8, 0x32, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 2, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 2, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 5, 8);
        break;
    case 0x3ED:
        Em_PlaySeAtBoneOnFrame(em, 0x28, 0x26, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 2, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x70, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x42, 3, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x94, 4, 8);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x17, 0x11);
        break;
    case 0x3EE:
        Em_PlaySeAtBoneOnFrame(em, 0x28, 0x26, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 2, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x70, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x42, 3, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x94, 4, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x17, 0x11);
        break;
    case 0x3EF:
        Em_PlaySeAtBoneOnFrame(em, 8, 0x31, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 4, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x6E, 7, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0x25);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 0x17, 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x96, 4, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0xBA, 4, 8);
        break;
    case 0x3F1:
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x22, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 0x26, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 0x14, 2);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0xB, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 0xB, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x54, 2, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x5A, 3, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x5C, 4, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x8A, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x9C, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0xA0, 4, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x96, 4, 8);
        break;
    case 0x3F2:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x20, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x36, 0x5B, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x13, 2);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x15, 2);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 3, 0xD);
        break;
    case 0x3F3:
        Em_PlaySeAtBoneOnFrame(em, 0x28, 0x5B, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 2, 0xF, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 6, 0xF, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x1C, 0x25);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 0x1D, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 0x1D, 0x2E);
        break;
    case 0x3F4:
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x1A, 0x25);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0x19, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0x19, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 0x19, 0x25);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 0x19, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x19, 0x2E);
        break;
    case 0x3F5:
        Em_PlaySeAtBoneOnFrame(em, 0x26, 0xE, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 0xE, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 2, 0x1A, 0x25);
        Em_PlaySeAtBoneOnFrame(em, 6, 0x19, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 8, 0x19, 0x2E);
        break;
    case 0x3F6:
    case 0x3F7:
        Em_PlaySeAtBoneOnFrame(em, 0xD, 0x1A, 0x25);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0x19, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x17, 0x19, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x19, 0x25);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 0x19, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x41, 0x19, 0x2E);
        break;
    case 0x3F8:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x32, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0xD, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 0xD, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0xB, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0xB, 0x32);
        break;
    case 0x3F9:
        Em_PlaySeAtBoneOnFrame(em, 0x40, 0x26, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 8, 0xD, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0xD, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 0xB, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 0xB, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x66, 0xC, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 0xC, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 7, 2);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 2, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 2, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 5, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 5, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x8C, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0xB2, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0xA0, 1, 8);
        break;
    case 0x3FA:
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0xD, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 0xD, 0x32);
        break;
    case 0x3FB:
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0xD, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 0xD, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0xF, 0x11);
        break;
    case 0x3FC:
        Em_PlaySeAtBoneOnFrame(em, 0x12, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 4, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 4, 0xD);
        break;
    case 0x407:
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x2A, 0x29);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x38, 0x29);
        Em_PlaySeAtBoneOnFrame(em, 0x9C, 0x31, 0x29);
        Em_PlaySeAtBoneOnFrame(em, 0x108, 0x28, 0x29);
        Em_PlaySeAtBoneOnFrame(em, 0xF0, 0x39, 0x29);
        Em_PlaySeAtBoneOnFrame(em, 0x19C, 0x38, 0x29);
        Em_PlaySeAtBoneOnFrame(em, 0x5A, 0x3A, 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0xF0, 0x3A, 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x1A4, 0x3A, 0x11);
        break;
    case 0x408:
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0x37, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 0x37, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x122, 0x1F, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 0x28, 0x29);
        Em_PlaySeAtBoneOnFrame(em, 0x86, 0x2A, 0x29);
        Em_PlaySeAtBoneOnFrame(em, 0xE4, 0x28, 0x29);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x114, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0xF8, 0xE, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0xFC, 0xE, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0xFC, 0x16, 0x11);
        break;
    case 0x409:
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x38, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0x31, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x8C, 0x39, 0x26);
        break;
    case 0x40A:
        Em_PlaySeAtBoneOnFrame(em, 0x10, 0x5B, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x5A, 4, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 4, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 0xC, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 0xC, 0x32);
        break;
    case 0x40B:
        Em_PlaySeAtBoneOnFrame(em, 0x54, 0x58, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x58, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 0x28, 0x29);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 4, 3, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 4, 0xD);
        break;
    case 0x40C:
        Em_PlaySeAtBoneOnFrame(em, 0x10, 0x5D, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x9E, 0x26, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x120, 0x5C, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 4, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x88, 3, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x9A, 0x17, 0x11);
        break;
    case 0x40D:
        Em_PlaySeAtBoneOnFrame(em, 0x10, 0x5F, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x7E, 0x28, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x74, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x62, 4, 8);
        Em_PlaySeAtBoneOnFrame(em, 0xA8, 4, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 0xE, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 0xE, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x56, 0xA, 0x11);
        break;
    case 0x411:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x20, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x94, 0x26, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 4, 8);
        Em_PlaySeAtBoneOnFrame(em, 0xB4, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0xAE, 3, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0xCA, 3, 8);
        Em_PlaySeAtBoneOnFrame(em, 0xE6, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 0x24, 0x29);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 0x29, 0x29);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 0x24, 0x29);
        Em_PlaySeAtBoneOnFrame(em, 0x6E, 0x29, 0x29);
        Em_PlaySeAtBoneOnFrame(em, 0x80, 0x28, 0x29);
        break;
    case 0x412:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x49, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 4, 3, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 4, 0xD);
        break;
    case 0x413:
        if ((em->stateGroup == 3) && (em->state == 6)) {
            Em_PlaySeAtBoneOnFrame(em, 0xC, 0x18, 0x26);
            Em_PlaySeAtBoneOnFrame(em, 4, 0x4B, 0x26);
            Em_PlaySeAtBoneOnFrame(em, 2, 2, 0x1F);
            Em_PlaySeAtBoneOnFrame(em, 6, 2, 0x1A);
            break;
        }
        Em_PlaySeAtBoneOnFrame(em, 4, 0x4F, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 6, 0x3D, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 2, 2, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 6, 2, 0x1A);
        break;
    case 0x414:
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0x4E, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 4, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 8, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x7A, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0x11);
        break;
    case 0x415:
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x1F, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 0x4A, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x15, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x86, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 3, 0xD);
        break;
    case 0x416:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x21, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 0x4B, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x96, 0x25, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x56, 6, 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 0x16, 0x11);
        Em_PlaySeAtBoneOnFrame(em, 4, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 0, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x90, 0, 0xD);
        break;
    case 0x417:
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0x1F, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 0x4B, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x5A, 0x18, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 4, 3, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 5, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x66, 0xB, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 0xB, 0x32);
        break;
    case 0x418:
        Em_PlaySeAtBoneOnFrame(em, 0x40, 0x38, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0xA4, 0x37, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 8, 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0xA2, 8, 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x102, 8, 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x164, 8, 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x36, 0xD, 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x94, 0xD, 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0xF2, 0xD, 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x156, 0xD, 0x11);
        break;
    case 0x419:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x20, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 0x24, 0x29);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 0x29, 0x29);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0x28, 0x29);
        break;
    case 0x41A:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x20, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 0x24, 0x29);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 0x29, 0x29);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0x28, 0x29);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0x15, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 1, 0x1F);
        break;
    case 0x41B:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x20, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x24, 0x29);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 0x29, 0x29);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0x28, 0x29);
        break;
    case 0x41C:
        if ((em->stateGroup == 3) && (em->state == 7)) {
            Em_PlaySeAtBoneOnFrame(em, 0xC, 0x18, 0x26);
            Em_PlaySeAtBoneOnFrame(em, 4, 0x4B, 0x26);
            Em_PlaySeAtBoneOnFrame(em, 2, 2, 0x1F);
            Em_PlaySeAtBoneOnFrame(em, 6, 2, 0x1A);
            break;
        }
        Em_PlaySeAtBoneOnFrame(em, 4, 0x4F, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 6, 0x3D, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 2, 2, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 6, 2, 0x1A);
        break;
    case 0x41D:
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0x26, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 4, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 8, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x7A, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0x11);
        break;
    case 0x425:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x53, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 0x2E, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 2);
        Em_PlaySeAtBoneOnFrame(em, 0x7C, 0, 0x1A);
        break;
    case 0x426:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x54, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x82, 0x26, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 7, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 2, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x42, 2, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x12, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 5, 8);
        Em_PlaySeAtBoneOnFrame(em, 4, 0xD, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 8, 0xD, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x5E, 0xA, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x62, 0xA, 0x32);
        break;
    case 0x427:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x54, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0xB4, 0x2E, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x82, 0x28, 0x29);
        Em_PlaySeAtBoneOnFrame(em, 0xB0, 0x2A, 0x29);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x17, 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 2, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x52, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 3, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 4, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 4, 0xD, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 8, 0xD, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 0xB, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 0xB, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x5E, 0xB, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x62, 0xB, 0x32);
        break;
    case 0x428:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x55, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 6, 0xB, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0xB, 0x32);
        break;
    case 0x429:
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 0x28, 0x29);
        Em_PlaySeAtBoneOnFrame(em, 8, 9, 0x25);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 7, 2);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0xF, 0x32);
        break;
    case 0x42A:
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0x57, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 0x10, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x17, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0x2A, 0x29);
        break;
    case 0x42B:
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0x28, 0x29);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x56, 0x29);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 2, 0x12, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x82, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 2, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x72, 4, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x86, 3, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 4, 0xB, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x52, 0xE, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x56, 0xE, 0x32);
        break;
    case 0x42C:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x55, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0x28, 0x29);
        Em_PlaySeAtBoneOnFrame(em, 0x70, 0x2F, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0xBE, 0x35, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x16E, 0x2A, 0x29);
        Em_PlaySeAtBoneOnFrame(em, 0x190, 0x2C, 0x29);
        Em_PlaySeAtBoneOnFrame(em, 0x60, 0x17, 2);
        Em_PlaySeAtBoneOnFrame(em, 0xC0, 0x16, 2);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 3, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x1B2, 0xF, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x1B6, 0xF, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x1D8, 7, 2);
        break;
    case 0x42D:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x53, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 0x57, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x16, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0, 0x1A);
        break;
    case 0x42E:
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x5F, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0x5B, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x7E, 0x26, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x16, 2);
        Em_PlaySeAtBoneOnFrame(em, 4, 0xD, 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0xD, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0xD, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x54, 0xE, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 0xE, 0x32);
        break;
    case 0x42F:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x53, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 0x5D, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0xC2, 0x2A, 0x29);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0x17, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0xFC, 0x34, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 4, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 4, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 0x12, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0xA0, 0x10, 2);
        Em_PlaySeAtBoneOnFrame(em, 0xB8, 7, 2);
        Em_PlaySeAtBoneOnFrame(em, 4, 0xE, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 8, 0xE, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0xA8, 0xB, 0x32);
        break;
    case 0x431:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x55, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x82, 0x2C, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x42, 0x28, 0x29);
        Em_PlaySeAtBoneOnFrame(em, 0x70, 0x2A, 0x29);
        Em_PlaySeAtBoneOnFrame(em, 0xE2, 0x2A, 0x29);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0x17, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 0xD, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0xD, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x62, 0xC, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x66, 0xC, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0xA8, 7, 2);
        Em_PlaySeAtBoneOnFrame(em, 0xB6, 9, 0x25);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0xA0, 2, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 4, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x8C, 0x10, 8);
        break;
    case 0x432:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x53, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 6, 0x28, 0x29);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x16, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 7, 2);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0x10, 2);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 9, 0x25);
        Em_PlaySeAtBoneOnFrame(em, 8, 0xD, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0xD, 0x32);
        break;
    case 0x433:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x5F, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x2A, 0x29);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0x33, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x17, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 4, 0xE, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x60, 0xE, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 0xE, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x7A, 2, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x8A, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x54, 4, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x98, 4, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 4, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0xBE, 4, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0xA6, 1, 0x1F);
        
    case 0x434:
    default:
        break;
    }
}

/* em_yamatsukami's .data (0x8CA9A8-0x8CAB00) */
f32 EmYamatsukami_StartPos[3] = {
#include "game/008CA9A8.inc"
};
EmYamaKey EmYamaTsukami_Keys_0[12] = {
#include "game/008CA9C0.inc"
};
EmYamaKey EmYamaTsukami_Keys_1[11] = {
#include "game/008CAA20.inc"
};
EmYamaKey *EmYamaTsukami_Keys[2] = {
    EmYamaTsukami_Keys_0, EmYamaTsukami_Keys_1,
};
f32 EmYamatsukami_EaseTargets[12] = {
#include "game/008CAA80.inc"
};
f32 EmYamatsukami_Factors[8] = {
#include "game/008CAAB0.inc"
};
f32 D_008CAAD0 = {
#include "game/008CAAD0.inc"
};
s8 EmYamaTsukami_BoneFloorBones[4][2] = {
#include "game/008CAAD8.inc"
};
void *EmYamaTsukami_Callbacks[8] = {
    EmYamaTsukami_SpawnEffects, EmYamaTsukami_SpawnEffects, func_00849C50, EmYamaTsukami_UpdateMotionEffects,
    EmYamaTsukami_ScriptQuery, func_00849C50, NULL, NULL,
};
