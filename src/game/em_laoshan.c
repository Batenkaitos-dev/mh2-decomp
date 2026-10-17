/*
 * Monster family: Lao-Shan Lung and Ashen Lao-Shan Lung
 * (EM_KIND_LAO_SHAN_LUNG and EM_KIND_ASHEN_LAO_SHAN_LUNG). The code from the
 * family's setup function (EmLaoshan_Setup, called by the per-kind dispatch
 * Em_TypeSetup) to the next family's. The state functions take the monster
 * and the work area its state dispatcher passes on (EmWork); Em.step is the
 * step within the current state. See include/em.h.
 */
#include "common.h"
#include "scene.h"
#include "quest.h"
#include "mine_point.h"
#include "em.h"
#include "player.h"
#include "area.h"
#include "qsync.h"
/* prototypes of callees (round 13) */
void Em_TargetPlayerInRange(Em *);

/* A mining point (as in sub_main's mine_point.c). */
/* prototypes */
void EmLaoshan_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void Effect0D_SpawnForEmScaled(Em *em, s32 a, s32 b, f32 scale);
void StageObj_K0F_CreateOnEm(Em *em, u8 k, u8 c, f32 scale);
void Em_PlaySeAtBoneOnFrame(Em *em, s32 frame, s32 se, s32 bone);
void Em_SetLayerMotion(Em *em, s32 motion, s32 blend, s32 frame, s32 layer);
void Em_SetMotion(Em *em, s32 motion, s32 a, s32 b);
u16 Math_AngleXZ(f32 *from, f32 *to);
void Player_ClearFlag(void *, u32 flag);
void Player_SetFlag(void *obj, u32 flag);
void LaoShanAttack_Create(Em *, s8);
void LaoShanAttack_CreateSpan(Em *, s32, s32);
void StageShot_CreateAtSpot(Em *, s32, s32);
void StageObj_K0A_Create(Em *em, u8 k, u8 c, f32 f);
void LaoShanAttack_CreateAt(Em *em, f32 *pos, s8 kind);
/* unit's own declaration: with the definition's void (Em *em, s32 group, s32 state, u8 mode) (game/unk_701810.c), EmLaoshan_ChangeState: 23 of 73 words differ, EmLaoshan_SetGroup0State: 1 of 6 words differ ... */
void Em_SetStateSync(Em *em, u16 group, u16 state, u8 mode);
void Em_PlaySeTypeAtBoneInFrames(Em *em, s32 frame, s32 frame2, s32 se, s32 bone, s32 vol, s32 a);
void Em_PlaySeTypeAtBoneOnFrame(Em *em, s32 frame, s32 se, s32 bone);
s32 Em_StepToGoal(Em *em, u16 step, u8 flags);
void Em_RestartScript(Em *em);
void EmLaoshan_GetMinePointPos(Em *em, f32 *out);
static s32 EmLaoshan_LayerMotionDone(Em *em, s32 layer);
void EmLaoshan_State1_02(Em *);
void EmLaoshan_State1_06(Em *);
void EmLaoshan_State1_0A(Em *);
void EmLaoshan_State1_12(Em *);
void EmLaoshan_State1_13(Em *);
void EmLaoshan_State3_02(Em *);
void EmLaoshan_State3_05(Em *);
void EmLaoshan_State4_02(Em *);
void EmLaoshan_State5_00_Hunted(Em *);
void EmLaoshan_State5_01_Hunted(Em *);
void EmLaoshan_State6_00(Em *);
void EmLaoshan_MotionSounds(Em *em, struct EmWork *);
void EmLaoshan_MotionEffects(Em *, EmLaoshanWork *);
void EmLaoshan_SpawnAreaImpactEffects(Em *em);
void EmLaoshan_SpawnAreaImpactEffects2(Em *);
s32 Sound_RandomSe(s32 id, s16 lo, s32 alt, s16 n);
s32 EmMotion_PassedFrameScaled(void *p, f32 frame, s32 layer);
s32 EmMotion_ReachedFrame(void *obj, s32 a, f32 frame);
void CamShake_ReqAIf(Em *obj, s32 i);
void Vec_Set(f32 *out, f32 x, f32 y, f32 z);
f32 Vec_DistanceXZ(f32 *a, f32 *b);
s32 Debug_IsOn(s32);
u16 Area_DayId(u16 id);
void MinePoint_SetArea(s32 slot, u16 area);
void Effect01_Spawn(void *obj, s16 type);
void Em_SetMotionRow(Em *em, s32 row, s32 arg2, s32 arg3);
void Em_SetPartScaled(Em *em, s32 bit);
void Em_SetStateRand(Em *em, s32 arg1, s32 arg2);
s32 Em_ComputeMaxHp(Em *em, s16 idx);
void EmLaoshan_AddMinePoint(Em *em);
void Em_BonePos(void *obj, s16 bone, f32 *out);
EmBone *Em_Bone(void *obj, s16 bone);
void Vec_Rotate(f32 *out, f32 *v, f32 *m);
void EmLaoshan_SetGroup1State(Em *em, u16 state, u8 mode);
void Em_SetSpeedScaled(Em *em, u16 flags, f32 x, f32 y);
void Em_MotionDelta(Em *em, f32 *out);
void Quest_TriggerEvent(Em *em);
void Player_ClearVelocity(void *);
void GmEm_ResetSize(); /* unit's own declaration: GmEm_ResetSize reads a0 = em (the incoming argument, still in a0); the call passes no argument */
s32 Save_TestEventFlag(s32);
s32 Em_PlaySeChecked(Em *em, s32 id, s32 a, void *pos, s32 b, s32 c);
void Effect02_SpawnForOwner(void *owner, s32 ang, s32 type, s32 sub, f32 *pos, f32 scale);
void Effect0D_SpawnIfHere(void *o, f32 *pos, s32 type, f32 scale);
s32 Em_SetCombat(Em *em, u8 a, s16 b);
void QuestEm_Hunted(Em *em);
void CamReq_BossHunted(void);
void EmLaoshan_AddMinePoints(Em *em);
void EmLaoshan_UpdateMinePoints(Em *em);
s32 MinePoint_Count(s32);
extern s8 EmLaoshan_MinePtBones[][2];
extern f32 EmLaoshan_MinePtRadius[];
void MinePoint_PrepareStub(MinePoint *pt);
void Em_MoveRotated(Em *em, void *src);
void QSync_SendEm(Em *em, u8 kind, u8 arg);
void CamReq_ForMonster(Em *em);
s32 Quest_IsRunning(s32 ending);
void Em_EndStatus5(Em *em);
void Em_UpdatePendingScript(Em *em);
void Em_ClearHitReactMode(Em *em);
u8 Em_UpdateDamage(Em *em, u8 *out);
void EmLaoshan_RunState(Em *em);
/* end of prototypes */

void EmLaoshan_Setup(Em *em)
{
    EmLaoshanWork *w = (EmLaoshanWork *)&em->work;

    f32 pos[4];
    s8 slot;

    w->unk000 = 0;
    slot = w->mineSlot;
    if (slot != -1) {
        MinePoint_SetArea((s32) slot, em->area);
        EmLaoshan_GetMinePointPos(em, pos);
        MinePoint_SetPos(w->mineSlot, pos);
    }
}

void EmLaoshan_SpawnEffect01(u8 *em)
{
    Effect01_Spawn((Player *) em, 0x3);
}

void EmLaoshan_GetMinePointPos(Em *em, f32 *out)
{
    f32 ofs[3];
    f32 b[3];
    f32 a[3];

    ofs[1] = 250.0f;
    ofs[2] = 120.0f;
    ofs[0] = 0.0f;
    Em_BonePos(em, 3, a);
    Vec_Rotate((f32 *) b, (f32 *) ofs, (f32 *) (Em_Bone(em, 3)));
    out[0] = a[0] + b[0];
    out[1] = a[1] + b[1];
    out[2] = a[2] + b[2];
}

void EmLaoshan_AddMinePoint(Em *em)
{
    EmLaoshanWork *w = (EmLaoshanWork *)&em->work;
    MinePoint pt;
    f32 pos[3];

    if (w->mineCount <= 0) {
        w->mineSlot = -1;
    }
    pt.radius = 340.0f;
    if (em->type == 7) {
        switch (gQuest.header->tier) {
        default:
            pt.table = 0xA;
            break;
        case 1:
            pt.table = 0x86;
            break;
        case 2:
            pt.table = 0x103;
            break;
        case 3:
            pt.table = 0x103;
            break;
        }
    } else {
        switch (gQuest.header->tier) {
        default:
            pt.table = 0x54;
            break;
        case 1:
            pt.table = 0xD0;
            break;
        case 2:
            pt.table = 0x14D;
            break;
        case 3:
            pt.table = 0x14D;
            break;
        }
    }
    pt.action = MINE_ACTION_CARVE;
    pt.count = w->mineCount;
    pt.area = (u8) em->area;
    pt.flags = 1;
    pt.timer = 0;
    EmLaoshan_GetMinePointPos(em, pos);
    w->mineSlot = MinePoint_AddUntimed(&pt, pos, 0);
}

void EmLaoshan_AddMinePoints(Em *em)
{
    EmLaoshanWork *w = (EmLaoshanWork *)&em->work;
    s32 i;
    MinePoint pt;
    f32 pos[3];

    if (em->type == 7) {
        switch (gQuest.header->tier) {
        default:
            pt.table = 9;
            break;
        case 1:
            pt.table = 0x85;
            break;
        case 2:
            pt.table = 0x102;
            break;
        case 3:
            pt.table = 0x102;
            break;
        }
    } else {
        switch (gQuest.header->tier) {
        default:
            pt.table = 0x53;
            break;
        case 1:
            pt.table = 0xCF;
            break;
        case 2:
            pt.table = 0x14C;
            break;
        case 3:
            pt.table = 0x14C;
            break;
        }
    }
    for (i = 0; i < 3; i++) {
        pt.radius = EmLaoshan_MinePtRadius[i];
        pt.area = (u8) em->area;
        pt.timer = 0;
        pt.flags = 1;
        Em_BonePos(em, EmLaoshan_MinePtBones[i][0], pos);
        pos[1] = em->floorY;
        pt.action = MINE_ACTION_CARVE;
        pt.count = EmLaoshan_MinePtBones[i][1];
        MinePoint_PrepareStub(&pt);
        w->minePts[i] = MinePoint_AddUntimed(&pt, pos, 0);
    }
}

void EmLaoshan_UpdateMinePoints(Em *em)
{
    EmLaoshanWork *w = (EmLaoshanWork *)&em->work;
    s32 i;
    s8 *t;
    f32 *y;
    s8 *p;
    f32 pos[3];

    t = EmLaoshan_MinePtBones[0];
    y = &pos[1];
    for (i = 0; i < 3; i++) {
        p = &w->minePts[i];
        if (*p != -1) {
            Em_BonePos(em, *t, pos);
            *y = em->floorY;
            MinePoint_SetPos(*p, pos);
            if (MinePoint_Count(*p) <= 0) {
                MinePoint_Remove(*p);
                *p = -1;
            }
        }
        t += 2;
    }
}

void EmLaoshan_RemoveMinePoints(Em *em)
{
    EmLaoshanWork *w = (EmLaoshanWork *)&em->work;
    s32 i;
    s8 *p;

    for (i = 0; i < 3; i++) {
        p = &w->minePts[i];
        if (*p != -1) {
            MinePoint_Remove(*p);
            *p = -1;
        }
    }
}

void EmLaoshan_Init(Em *em)
{
    EmLaoshanWork *w = (EmLaoshanWork *)&em->work;

    s16 size;
    s32 dayId;

    Em_SetPartScaled(em, 2);
    Em_SetPartScaled(em, 3);
    if (gQuest.stage == 0) {
        dayId = Area_DayId(gScene.area) & 0xFFFF;
        switch (dayId) {
        case AREA_FORTRESS_4:
            em->posX = 33000.0f;
            em->posY = 0.0f;
            em->posZ = 30200.0f;
            em->rotY = 0xC000;
            break;
        case AREA_FORTRESS_5:
            em->posX = 7400.0f;
            em->posY = 0.0f;
            em->posZ = 26300.0f;
            em->rotY = 0x4000;
            break;
        case 13:
            em->posX = 5000.0f;
            em->posY = 0.0f;
            em->posZ = 8000.0f;
            em->rotY = 0x8000;
            break;
        case AREA_FORTRESS_1:
            em->posX = 2400.0f;
            em->posY = 0.0f;
            em->posZ = 11000.0f;
            em->rotY = 0xE39;
            break;
        case AREA_SNOWY_MOUNTAINS_NIGHT_9:
            em->posX = 7000.0f;
            em->posY = 580.0f;
            em->posZ = 7000.0f;
            em->rotY = 0;
            break;
        case AREA_FORTRESS_3:
            em->posX = 30800.0f;
            em->posY = 0.0f;
            em->posZ = 32600.0f;
            em->rotY = 0x6C00;
            break;
        case AREA_FORTRESS_2:
            em->posX = 23500.0f;
            em->posY = 0.0f;
            em->posZ = 3500.0f;
            em->rotY = 0xBA06;
            break;
        default:
            em->posX = 10000.0f;
            em->posY = 0.0f;
            em->posZ = 10000.0f;
            em->rotY = 0x8000;
            break;
        }
    }
    size = Em_ComputeMaxHp(em, 0);
    em->hp = size;
    em->hpMax = size;
    em->scriptPending = 1;
    em->blind = 1;
    em->unk_InitFlag = 1;
    em->combat = 1;
    em->combatPlayers = 0xF;
    Em_SetMotionRow(em, 1, 0, 0);
    Em_SetStateRand(em, 0, 1);
    w->unk00C = 1500.0f;
    w->unk01C = 0;
    w->unk010 = 0;
    w->unk000 = 0;
    w->syncTimer = 0x96;
    w->unk014 = -1;
    w->unk018 = -1;
    w->mineCount = 3;
    EmLaoshan_AddMinePoint(em);
}

void EmLaoshan_EndState(Em *em)
{
    em->motionSpeed = 1.0f;
    em->lookAtTarget = 0;
    em->scriptPending = 1;
    if (em->stance == 0) {
        em->stance = 0;
        EmLaoshan_ChangeState(em, 0, 4, 0);
        return;
    }
    em->stance = 1;
    EmLaoshan_ChangeState(em, 0, 1, 0);
}

static s32 EmLaoshan_LayerMotionDone(Em *em, s32 layer)
{
    if (em->motion[layer].playing == 0 || layer == 0xFF) {
        em->scriptPending = 1;
        return 1;
    }
    return 0;
}

void EmLaoshan_State0_00(Em *em)
{
    if (Debug_IsOn(0x4D) == 0) {
        if (em->motionId[0] != 0x3E9) {
            Em_SetMotion(em, 1, 0xA, 0);
        }
        if (em->motionId[1] != 0x44D) {
            Em_SetMotion(em, 1, 0xA, 0);
        }
        if (em->motionId[2] != 0x4B1) {
            Em_SetMotion(em, 1, 0xA, 0);
        }
    }
}

void EmLaoshan_State0_01(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 1;
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
        if (em->motion[0].playing == 0) {
            em->step++;
            EmLaoshan_EndState(em);
        }
        break;
    }
}

void EmLaoshan_State0_02(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 1;
        Em_SetMotion(em, 6, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 7, 0, 0);
        }
        break;
    case 2:
        em->quakeTimer = 5;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->quakeTimer = 0;
            EmLaoshan_EndState(em);
        }
        break;
    }
}

void EmLaoshan_State0_03(Em *em)
{
    em->guardTimer = 5;
    em->noReact = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 1;
        Em_SetMotion(em, 8, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stance = 0;
            em->guardTimer = 0;
            em->noReact = 0;
            EmLaoshan_EndState(em);
        }
        break;
    }
}

void EmLaoshan_State0_04(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 0;
        Em_SetMotion(em, 0xC, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmLaoshan_EndState(em);
        }
        break;
    }
}

void EmLaoshan_State0_05(Em *em)
{
    em->guardTimer = 5;
    em->noReact = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 0;
        Em_SetMotion(em, 0xF, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->guardTimer = 0;
            em->noReact = 0;
            em->stance = 1;
            EmLaoshan_EndState(em);
        }
        break;
    }
}

void EmLaoshan_State0_06(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 1;
        if (em->motionId[0] != 0x3E9) {
            Em_SetLayerMotion(em, 0x3E9, 0xA, 0, 0);
        }
        if (em->motionId[1] != 0x4B1) {
            Em_SetLayerMotion(em, 0x4B1, 0xA, 0, 1);
        }
        if (em->motionId[2] != 0x579) {
            Em_SetLayerMotion(em, 0x579, 0xA, 0, 2);
        }
        em->timer = 0x1E;
        break;
    case 1:
        if (em->motion[0].playing != 0) {
            if (--em->timer <= 0) {
                goto block_15;
            }
        } else {
block_15:
            em->step++;
            EmLaoshan_EndState(em);
        }
        break;
    }
}

void EmLaoshan_State0_07(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 0;
        Em_SetMotion(em, 0xC, 0, 0);
        em->timer = 0x1E;
        break;
    case 1:
        if (em->motion[0].playing != 0) {
            if (--em->timer <= 0) {
                goto block_9;
            }
        } else {
block_9:
            em->step++;
            EmLaoshan_EndState(em);
        }
        break;
    }
}

void EmLaoshan_RunGroup0(Em *em)
{
    switch (em->state) {
    case 0:
        EmLaoshan_State0_00(em);
        break;
    case 1:
        EmLaoshan_State0_01(em);
        break;
    case 2:
        EmLaoshan_State0_02(em);
        break;
    case 3:
        EmLaoshan_State0_03(em);
        break;
    case 4:
        EmLaoshan_State0_04(em);
        break;
    case 5:
        EmLaoshan_State0_05(em);
        break;
    case 6:
        EmLaoshan_State0_06(em);
        break;
    case 7:
        EmLaoshan_State0_07(em);
        
    default:
        break;
    }
}

void EmLaoshan_State1_00(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 1;
        Em_SetMotion(em, 3, 0, 0);
        break;
    case 1:
        if (Em_StepToGoal(em, 0x40, 0) == 1) {
            em->step++;
            Player_SetFlag(em, EM_FLAG_NO_ROOT_MOTION);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
            EmLaoshan_EndState(em);
        }
        break;
    }
}

void EmLaoshan_State1_01(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 1;
        Em_SetMotion(em, 2, 0, 0);
        break;
    case 1:
        if (Em_StepToGoal(em, 0x40, 4) == 1) {
            em->step++;
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmLaoshan_EndState(em);
        }
        break;
    }
}

void EmLaoshan_State1_02(Em *em)
{
    u32 d;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 1;
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        d = (em->aimAngle - em->rotY) & 0xFFFF;
        if (d <= 0x11C7 || d >= 0xEE39) {
            Player_SetFlag(em, EM_FLAG_NO_ROOT_MOTION);
            Em_SetMotion(em, 3, 0, 0);
        } else if (d >= 0x8000) {
            Em_SetMotion(em, 5, 0, 0);
        } else {
            Em_SetMotion(em, 4, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].blend == 0) {
            d = (em->aimAngle - (u16) em->rotY) & 0xFFFF;
            if (em->motion[0].playing == 0) {
                if (((d + 0x22) & 0xFFFF) <= 0x43) {
                    em->step++;
                    Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
                    EmLaoshan_EndState(em);
                } else if (d <= 0x11C7 || d >= 0xEE39) {
                    Player_SetFlag(em, EM_FLAG_NO_ROOT_MOTION);
                    Em_SetMotion(em, 3, 0, 0);
                } else {
                    Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
                    if (d >= 0x8000) {
                        Em_SetMotion(em, 5, 0, 0);
                    } else {
                        Em_SetMotion(em, 4, 0, 0);
                    }
                }
            } else if (EmMotion_ReachedFrame(em, 0, 162.0f) != 0 && EmMotion_ReachedFrame(em, 0, 462.0f) == 0) {
                if (((d + 0x22) & 0xFFFF) <= 0x43) {
                    em->rotY = em->aimAngle;
                } else if (d <= 0x7FFF) {
                    em->rotY = (u16) (em->rotY + 0x22);
                } else {
                    em->rotY = (u16) (em->rotY - 0x22);
                }
            }
        }
        break;
    }
}

void EmLaoshan_State1_03(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 1;
        Em_SetMotion(em, 4, 0, 0);
        break;
    case 1:
        if ((EmMotion_ReachedFrame(em, 0, 162.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 462.0f) == 0)) {
            em->rotY += 0x22;
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmLaoshan_EndState(em);
        }
        break;
    }
}

void EmLaoshan_State1_04(Em *em)
{
    EmLaoshanWork *w = (EmLaoshanWork *)&em->work;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 1;
        Em_SetMotion(em, 0x14, 0, 0);
        break;
    case 1:
        if (em->motion[0].blend == 0) {
            if ((EmMotion_ReachedFrame(em, 0, 28.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 156.0f) == 0)) {
                em->rotY += 0x80;
            }
            if (EmLaoshan_LayerMotionDone(em, 0) != 0) {
                w->unk01C = 1;
            }
        }
        break;
    }
}

void EmLaoshan_State1_05(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 1;
        Em_SetMotion(em, 0x15, 0, 0);
        break;
    case 1:
        EmLaoshan_LayerMotionDone(em, 0);
        break;
    }
}

void EmLaoshan_State1_06(Em *em)
{
    EmLaoshanWork *w = (EmLaoshanWork *)&em->work;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 1;
        Em_SetMotion(em, 0x17, 0, 0);
        break;
    case 1:
        if (EmLaoshan_LayerMotionDone(em, 0) != 0) {
            w->unk01C = 0;
        }
        break;
    }
}

void EmLaoshan_State1_07(Em *em)
{
    EmLaoshanWork *w = (EmLaoshanWork *)&em->work;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 1;
        Em_SetMotion(em, 0x16, 0, 0);
        break;
    case 1:
        if (em->motion[0].blend == 0) {
            if ((EmMotion_ReachedFrame(em, 0, 30.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 158.0f) == 0)) {
                em->rotY -= 0x80;
            }
            if (EmLaoshan_LayerMotionDone(em, 0) != 0) {
                w->unk01C = 0;
            }
        }
        break;
    }
}

void EmLaoshan_State1_08(Em *em)
{
    EmLaoshanWork *w = (EmLaoshanWork *)&em->work;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 1;
        Em_SetMotion(em, 0x1A, 0, 0);
        break;
    case 1:
        if (em->motion[0].blend == 0) {
            if ((EmMotion_ReachedFrame(em, 0, 28.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 156.0f) == 0)) {
                em->rotY -= 0x80;
            }
            if (EmLaoshan_LayerMotionDone(em, 0) != 0) {
                w->unk01C = 2;
            }
        }
        break;
    }
}

void EmLaoshan_State1_09(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 1;
        Em_SetMotion(em, 0x1B, 0, 0);
        break;
    case 1:
        EmLaoshan_LayerMotionDone(em, 0);
        break;
    }
}

void EmLaoshan_State1_0A(Em *em)
{
    EmLaoshanWork *w = (EmLaoshanWork *)&em->work;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 1;
        Em_SetMotion(em, 0x1D, 0, 0);
        break;
    case 1:
        if (EmLaoshan_LayerMotionDone(em, 0) != 0) {
            w->unk01C = 0;
        }
        break;
    }
}

void EmLaoshan_State1_0B(Em *em)
{
    EmLaoshanWork *w = (EmLaoshanWork *)&em->work;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 1;
        Em_SetMotion(em, 0x1C, 0, 0);
        break;
    case 1:
        if (em->motion[0].blend == 0) {
            if ((EmMotion_ReachedFrame(em, 0, 30.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 158.0f) == 0)) {
                em->rotY += 0x80;
            }
            if (EmLaoshan_LayerMotionDone(em, 0) != 0) {
                w->unk01C = 0;
            }
        }
        break;
    }
}

void EmLaoshan_State1_0C(Em *em)
{
    EmLaoshanWork *w = (EmLaoshanWork *)&em->work;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 1;
        Em_SetMotion(em, 0x19, 0, 0);
        break;
    case 1:
        if (em->motion[0].blend == 0) {
            if ((EmMotion_ReachedFrame(em, 0, 82.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 210.0f) == 0)) {
                em->rotY += 0x100;
            }
            if (EmLaoshan_LayerMotionDone(em, 0) != 0) {
                w->unk01C = 0;
            }
        }
        break;
    }
}

void EmLaoshan_State1_0D(Em *em)
{
    EmLaoshanWork *w = (EmLaoshanWork *)&em->work;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 1;
        Em_SetMotion(em, 0x1F, 0, 0);
        break;
    case 1:
        if (em->motion[0].blend == 0) {
            if ((EmMotion_ReachedFrame(em, 0, 82.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 210.0f) == 0)) {
                em->rotY -= 0x100;
            }
            if (EmLaoshan_LayerMotionDone(em, 0) != 0) {
                w->unk01C = 0;
            }
        }
        break;
    }
}

void EmLaoshan_State1_0E(Em *em)
{
    EmLaoshanWork *w = (EmLaoshanWork *)&em->work;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 1;
        Em_SetMotion(em, 0x18, 0, 0);
        break;
    case 1:
        if (em->motion[0].blend == 0) {
            if ((EmMotion_ReachedFrame(em, 0, 204.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 334.0f) == 0)) {
                em->rotY += 0x7E;
            }
            if (EmLaoshan_LayerMotionDone(em, 0) != 0) {
                w->unk01C = 0;
                em->rotY += 8;
            }
        }
        break;
    }
}

void EmLaoshan_State1_0F(Em *em)
{
    EmLaoshanWork *w = (EmLaoshanWork *)&em->work;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 1;
        Em_SetMotion(em, 0x1E, 0, 0);
        break;
    case 1:
        if (em->motion[0].blend == 0) {
            if ((EmMotion_ReachedFrame(em, 0, 204.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 334.0f) == 0)) {
                em->rotY -= 0x7E;
            }
            if (EmLaoshan_LayerMotionDone(em, 0) != 0) {
                w->unk01C = 0;
                em->rotY -= 8;
            }
        }
        break;
    }
}

void EmLaoshan_State1_10(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 0;
        Em_SetMotion(em, 0x10, 0, 0);
        break;
    case 1:
        if (Em_StepToGoal(em, 0x40, 0) == 1) {
            em->step++;
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmLaoshan_EndState(em);
        }
        break;
    }
}

void EmLaoshan_State1_11(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 1;
        Em_SetMotion(em, 3, 0, 0);
        Em_SetMotion(em, 0x42, 0, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 34.0f, 0U) != 0) {
            LaoShanAttack_Create(em, 0x42);
        }
        if (EmMotion_PassedFrameScaled(em, 410.0f, 0U) != 0) {
            LaoShanAttack_Create(em, 0x43);
        }
        if (Em_StepToGoal(em, 0x40, 0) == 1) {
            em->step++;
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmLaoshan_EndState(em);
        }
        break;
    }
    Em_PlaySeTypeAtBoneInFrames(em, 0xAA, 0x168, 0x4F, 0x25, 9, 1);
}

void EmLaoshan_State1_12(Em *em)
{
    f32 pos[3];
    s32 rot[3];

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 1;
        Em_SetMotion(em, 3, 0, 0);
        break;
    case 1:
        if (Em_StepToGoal(em, 0x40, 0) == 1) {
            em->step++;
        }
        break;
    case 2:
        Em_MotionDelta(em, pos);
        Vec_Set(&em->velX, pos[0], pos[1], -pos[2]);
        rot[0] = 0;
        rot[1] = em->rotY;
        rot[2] = 0;
        Em_MoveRotated(em, rot);
        if (em->motion[0].playing == 0) {
            em->step++;
            EmLaoshan_EndState(em);
        }
        break;
    }
}

void EmLaoshan_State1_13(Em *em)
{
    f32 pos[3];
    s32 rot[3];

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 1;
        Em_SetMotion(em, 2, 0, 0);
        break;
    case 1:
        if (Em_StepToGoal(em, 0x40, 4) == 1) {
            em->step++;
        }
        break;
    case 2:
        Em_MotionDelta(em, pos);
        Vec_Set(&em->velX, pos[0], pos[1], -pos[2]);
        rot[0] = 0;
        rot[1] = em->rotY;
        rot[2] = 0;
        Em_MoveRotated(em, rot);
        if (em->motion[0].playing == 0) {
            em->step++;
            EmLaoshan_EndState(em);
        }
        break;
    }
}

void EmLaoshan_RunGroup1(Em *em)
{
    switch (em->state) {
    case 0:
        EmLaoshan_State1_00(em);
        break;
    case 1:
        EmLaoshan_State1_01(em);
        break;
    case 2:
        EmLaoshan_State1_02(em);
        break;
    case 3:
        EmLaoshan_State1_03(em);
        break;
    case 4:
        EmLaoshan_State1_04(em);
        break;
    case 5:
        EmLaoshan_State1_05(em);
        break;
    case 6:
        EmLaoshan_State1_06(em);
        break;
    case 7:
        EmLaoshan_State1_07(em);
        break;
    case 8:
        EmLaoshan_State1_08(em);
        break;
    case 9:
        EmLaoshan_State1_09(em);
        break;
    case 10:
        EmLaoshan_State1_0A(em);
        break;
    case 11:
        EmLaoshan_State1_0B(em);
        break;
    case 12:
        EmLaoshan_State1_0C(em);
        break;
    case 13:
        EmLaoshan_State1_0D(em);
        break;
    case 14:
        EmLaoshan_State1_0E(em);
        break;
    case 15:
        EmLaoshan_State1_0F(em);
        break;
    case 16:
        EmLaoshan_State1_10(em);
        break;
    case 17:
        EmLaoshan_State1_11(em);
        break;
    case 18:
        EmLaoshan_State1_12(em);
        break;
    case 19:
        EmLaoshan_State1_13(em);
        
    default:
        break;
    }
}

void EmLaoshan_RunGroup2(void)
{
}

void EmLaoshan_State3_00(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 0;
        Em_SetMotion(em, 0xD, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmLaoshan_EndState(em);
        }
        break;
    }
}

void EmLaoshan_State3_01(Em *em)
{
    f32 pos[3];

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 0;
        Em_SetMotion(em, 0xE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmLaoshan_EndState(em);
        }
        if (EmMotion_PassedFrameScaled(em, 160.0f, 0U) != 0) {
            if ((Area_DayId(em->area) & 0xFFFF) == AREA_FORTRESS_4) {
                Vec_Set(pos, 13300.0f, 1700.0f, 20100.0f);
                LaoShanAttack_CreateAt(em, (f32 *) pos, 0x44);
                EmLaoshan_SpawnAreaImpactEffects(em);
            }
            if ((Area_DayId(em->area) & 0xFFFF) == AREA_FORTRESS_5) {
                Vec_Set(pos, 7300.0f, 1700.0f, 11700.0f);
                LaoShanAttack_CreateAt(em, (f32 *) pos, 0x45);
                EmLaoshan_SpawnAreaImpactEffects(em);
            }
            if ((Area_DayId(em->area) & 0xFFFF) == AREA_FORTRESS_2) {
                Vec_Set(pos, 14600.0f, 1700.0f, 23700.0f);
                LaoShanAttack_CreateAt(em, (f32 *) pos, 0x46);
                EmLaoshan_SpawnAreaImpactEffects(em);
            }
        }
        break;
    }
}

void EmLaoshan_State3_02(Em *em)
{
    EmLaoshanWork *w = (EmLaoshanWork *)&em->work;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 1;
        Em_SetMotion(em, 0xA, 0, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 210.0f, 0U) != 0) {
            EmLaoshan_SpawnAreaImpactEffects2(em);
            if (em->remote == 0) {
                if (w->fortHits < 0xA) {
                    w->fortHits++;
                    if (w->fortHits >= 0xA) {
                        Quest_TriggerEvent(em);
                    }
                    gScene.fortStage = w->fortHits;
                }
            }
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmLaoshan_EndState(em);
        }
        break;
    }
}

void EmLaoshan_State3_03(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 0;
        Em_SetMotion(em, 0xD, 0, 0);
        break;
    case 1:
        if ((EmMotion_ReachedFrame(em, 0, 36.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 78.0f) == 0)) {
            em->timer = 0x15;
            em->rotY += (0x13E9 / (s32) em->timer) & 0xFFFF;
        }
        if ((EmMotion_ReachedFrame(em, 0, 434.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 510.0f) == 0)) {
            em->timer = 0x26;
            em->rotY -= (0x13E9 / (s32) em->timer) & 0xFFFF;
        }
        if (EmMotion_PassedFrameScaled(em, 510.0f, 0U) != 0) {
            em->rotY += 0xA;
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmLaoshan_EndState(em);
        }
        break;
    }
}

void EmLaoshan_State3_04(Em *em)
{
    EmLaoshanWork *w = (EmLaoshanWork *)&em->work;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 1;
        Em_SetMotion(em, 0xA, 0, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 210.0f, 0U) != 0) {
            if (em->remote == 0) {
                w->unk000 = 2;
            }
            EmLaoshan_SpawnAreaImpactEffects2(em);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmLaoshan_EndState(em);
        }
        break;
    }
}

void EmLaoshan_State3_05(Em *em)
{
    EmLaoshanWork *w = (EmLaoshanWork *)&em->work;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 1;
        Em_SetMotion(em, 0xA, 0, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 210.0f, 0U) != 0) {
            if (em->remote == 0) {
                w->unk000 = 3;
            }
            gScene.emEventFlags |= 1;
            EmLaoshan_SpawnAreaImpactEffects2(em);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmLaoshan_EndState(em);
        }
        break;
    }
}

void EmLaoshan_RunGroup3(Em *em)
{
    switch (em->state) {
    case 0:
        EmLaoshan_State3_00(em);
        break;
    case 1:
        EmLaoshan_State3_01(em);
        break;
    case 2:
        EmLaoshan_State3_02(em);
        break;
    case 3:
        EmLaoshan_State3_03(em);
        break;
    case 4:
        EmLaoshan_State3_04(em);
        break;
    case 5:
        EmLaoshan_State3_05(em);
        
    default:
        break;
    }
}

void EmLaoshan_State4_00(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 1;
        Em_SetMotion(em, 0x44, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 28.0f, 0U) != 0) {
            em->step++;
            Em_SetMotion(em, 0x3C, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x3D, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmLaoshan_EndState(em);
        }
        break;
    }
}

void EmLaoshan_State4_01(Em *em)
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
            EmLaoshan_EndState(em);
        }
        break;
    }
}

void EmLaoshan_State4_02(Em *em)
{
    s32 rot[3];

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 1;
        Em_SetMotion(em, 0x3F, 0, 0);
        Em_RestartScript(em);
        Player_ClearVelocity(em);
        em->velZ = -26.913044f;
        break;
    case 1:
        if (em->motion[0].blend == 0) {
            rot[0] = 0;
            rot[1] = em->rotY;
            rot[2] = 0;
            Em_MoveRotated(em, rot);
            if (EmMotion_ReachedFrame(em, 0, 46.0f) != 0) {
                em->step++;
            }
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmLaoshan_EndState(em);
        }
        break;
    }
}

void EmLaoshan_State4_03(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 1;
        Em_SetMotion(em, 0x44, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmLaoshan_EndState(em);
        }
        break;
    }
}

void EmLaoshan_State4_04(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 0;
        Em_SetMotion(em, 0xF, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stance = 1;
            EmLaoshan_EndState(em);
        }
        break;
    }
}

void EmLaoshan_State4_05(Em *em)
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
            Em_SetMotion(em, 0xF, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stance = 1;
            EmLaoshan_EndState(em);
        }
        break;
    }
}

void EmLaoshan_State4_06(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 1;
        Em_SetMotion(em, 0x44, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 28.0f, 0U) != 0) {
            em->step++;
            Em_SetMotion(em, 0x3C, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 0x12C;
        }
        break;
    case 3:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x3D, 0, 0);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmLaoshan_EndState(em);
        }
        break;
    }
}

void EmLaoshan_RunGroup4(Em *em)
{
    switch (em->state) {
    case 0:
        EmLaoshan_State4_00(em);
        break;
    case 1:
        EmLaoshan_State4_01(em);
        break;
    case 2:
        EmLaoshan_State4_02(em);
        break;
    case 3:
        EmLaoshan_State4_03(em);
        break;
    case 4:
        EmLaoshan_State4_04(em);
        break;
    case 5:
        EmLaoshan_State4_05(em);
        break;
    case 6:
        EmLaoshan_State4_06(em);
        
    default:
        break;
    }
}

void EmLaoshan_State5_00_Hunted(Em *em)
{
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 8, 0, 0);
        em->timer = 0;
        em->lookAtTarget = 0;
        em->stance = 1;
        QuestEm_Hunted(em);
        CamReq_BossHunted();
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 450.0f, 0) != 0) {
            em->step++;
            em->stance = 0;
            Em_SetMotion(em, 0x43, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stance = 3;
            EmLaoshan_AddMinePoints(em);
            em->fieldCollide = 0;
        }
        break;
    case 3:
        if (Debug_IsOn(0x42) != 0) {
            em->step = 0x63;
        } else {
            EmLaoshan_UpdateMinePoints(em);
        }
        break;
    case 0x63:
        em->step++;
        EmLaoshan_RemoveMinePoints(em);
        em->hp = 0x3415;
        EmLaoshan_EndState(em);
        break;
    }
}

void EmLaoshan_State5_01_Hunted(Em *em)
{
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x43, 0, 0);
        QuestEm_Hunted(em);
        CamReq_BossHunted();
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stance = 3;
            EmLaoshan_AddMinePoints(em);
            em->fieldCollide = 0;
        }
        break;
    case 2:
        if (Debug_IsOn(0x42) != 0) {
            em->step = 0x63;
        } else {
            EmLaoshan_UpdateMinePoints(em);
        }
        break;
    case 0x63:
        em->step++;
        em->hp = 0x3415;
        EmLaoshan_RemoveMinePoints(em);
        EmLaoshan_EndState(em);
        break;
    }
}

void EmLaoshan_RunGroup5(Em *em)
{
    switch (em->state) {
    case 0:
        EmLaoshan_State5_00_Hunted(em);
        break;
    case 1:
        EmLaoshan_State5_01_Hunted(em);
        break;
    }
}

void EmLaoshan_State6_00(Em *em)
{
    EmLaoshanWork *w = (EmLaoshanWork *)&em->work;
    f32 pos[3];

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        GmEm_ResetSize();
        em->step++;
        em->stance = 1;
        Em_SetMotion(em, 1, 0, 0);
        em->timer = 0xF0;
        em->posX = 8150.0f;
        em->posY = 0.0f;
        em->posZ = 9310.0f;
        em->goal[0] = 10660.0f;
        em->goal[1] = 0.0f;
        em->goal[2] = 9250.0f;
        em->rotY = Math_AngleXZ(&em->posX, em->goal);
        em->goalDist = Vec_DistanceXZ(&em->posX, em->goal);
        w->unk000 = 1;
        em->scriptPending = 0;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 3, 0, 0);
        }
        break;
    case 2:
        Em_MotionDelta(em, pos);
        em->goalDist -= pos[2];
        if (em->goalDist <= 0.0f) {
            em->step++;
            em->posX = 13600.0f;
            em->posY = 0.0f;
            em->posZ = 10750.0f;
            em->rotY = 0x25E0;
            Em_SetMotion(em, 8, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0xC, 0, 0);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0xD, 0, 0);
        }
        break;
    case 5:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0xF, 0, 0);
        }
        break;
    case 6:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 1, 0, 0);
        }
        break;
    case 7:
        if (em->type == 7) {
            if (Save_TestEventFlag(0xAD) == 1) {
                em->step++;
                EmLaoshan_EndState(em);
            }
        } else {
            if (Save_TestEventFlag(0xB5) == 1) {
                em->step++;
                EmLaoshan_EndState(em);
            }
        }
        break;
    }
}

void EmLaoshan_State6_01(Em *em)
{
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
        em->rotY = (Math_AngleXZ(&em->posX, em->goal) & 0xFFFF) - 0x4000;
        em->hasGoal = 1;
        em->goalDist = Vec_DistanceXZ(&em->posX, em->goal) - 500.0f;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x41, 0, 0);
            break;
        }
    case 0x5:
    default:
        break;
    case 2:
        if (em->motion[0].blend == 0) {
            if (EmMotion_ReachedFrame(em, 0, 64.0f) == 0) {
                em->rotY += 0x210;
            }
            if (em->motion[0].playing == 0) {
                em->step++;
                Em_SetMotion(em, 3, 0, 0);
                Em_SetMotion(em, 0x42, 0, 0);
                break;
            }
        }
        break;
    case 3:
        if ((em->motion[1].playing == 0) && (em->motionId[1] == 0x4F2)) {
            Em_SetLayerMotion(em, 0x4B3, 0, 0, 1);
        }
        if (Em_StepToGoal(em, 0x40, 0) == 1) {
            em->goal[0] = 430.0f;
            em->goal[1] = 0.0f;
            em->goal[2] = 23930.0f;
            em->step++;
            em->goalDist = Vec_DistanceXZ(&em->posX, em->goal);
        }
        break;
    case 4:
        if (Em_StepToGoal(em, 0x40, 0) == 1) {
            em->step++;
            Em_SetMotion(em, 3, 0, 0);
        }
        break;
    }
}

void EmLaoshan_RunGroup6(Em *em)
{
    switch (em->state) {
    case 0:
        EmLaoshan_State6_00(em);
        break;
    case 1:
        EmLaoshan_State6_01(em);
        break;
    }
}

void EmLaoshan_Update(Em *em)
{
    EmLaoshanWork *w = (EmLaoshanWork *)&em->work;
    f32 pos[3];
    u8 buf[4];
    s8 n;

    if (w->mineSlot != -1) {
        EmLaoshan_GetMinePointPos(em, pos);
        MinePoint_SetPos(w->mineSlot, pos);
        n = MinePoint_Count(w->mineSlot);
        w->mineCount = n;
        if (n <= 0) {
            MinePoint_Remove(w->mineSlot);
            w->mineSlot = -1;
        }
    }
    if (w->unk01C != 0) {
        (w->unk010)++;
    }
    if (em->quakeTimer != 0) {
        if (--em->quakeTimer <= 0) {
            em->quakeTimer = 0;
        }
    }
    if (w->syncTimer != 0 && em->stateGroup != 6 && em->remote == 0) {
        if (--w->syncTimer <= 0) {
            w->syncTimer = 150;
            QSync_SendEm(em, QSYNC_EMMSG_STATE_NO_PARTS, 0);
        }
    }
    switch (Em_UpdateDamage(em, buf) & 0xFF) {
    case 0:
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
        EmLaoshan_ChangeState(em, 5, 0, 2);
        break;
    case 3:
        if (em->stance == 0) {
            EmLaoshan_ChangeState(em, 4, 5, 2);
        } else {
            EmLaoshan_ChangeState(em, 4, 2, 2);
        }
        break;
    case 17:
        Em_EndStatus5(em);
        break;
    case 12:
        if (em->stance == 0) {
            EmLaoshan_ChangeState(em, 4, 1, 2);
        } else {
            switch (em->flinchPart) {
            case 0:
            case 1:
            case 4:
            case 5:
            case 6:
            case 7:
                EmLaoshan_ChangeState(em, 4, 3, 2);
                break;
            case 2:
                if (em->parts[em->flinchPart].state >= 2) {
                    EmLaoshan_ChangeState(em, 4, 0, 2);
                } else {
                    EmLaoshan_ChangeState(em, 4, 3, 2);
                }
                break;
            case 3:
                if (em->parts[em->flinchPart].state >= 2) {
                    EmLaoshan_ChangeState(em, 4, 6, 2);
                } else {
                    EmLaoshan_ChangeState(em, 4, 3, 2);
                }
                break;
            }
        }
        break;
    }
    switch (Area_DayId(em->area)) {
    case AREA_FORTRESS_1:
        if (em->type == 7) {
            if (Save_TestEventFlag(0xAD) != 0) {
                goto normal;
            }
            if (gScene.eventRunning == 1 && em->stateGroup != 6) {
                EmLaoshan_ChangeState(em, 6, 0, 0);
            }
        } else {
            if (Save_TestEventFlag(0xB5) != 0) {
                goto normal;
            }
            if (gScene.eventRunning == 1 && em->stateGroup != 6) {
                EmLaoshan_ChangeState(em, 6, 0, 0);
            }
        }
        break;
    case AREA_FORTRESS_5:
        if (Quest_IsRunning(0) == 1 && em->stateGroup != 6 && em->stateGroup != 5) {
            em->scriptPending = 0;
            if (gScene.area == em->area) {
                CamReq_ForMonster(em);
            }
            EmLaoshan_ChangeState(em, 6, 1, 0);
        }
    default:
    normal:
        Em_UpdatePendingScript(em);
        break;
    }
    Em_ClearHitReactMode(em);
    EmLaoshan_RunState(em);
    if (em->stateRequest != 0) {
        EmLaoshan_RunState(em);
        em->stateRequest = 0;
    }
}

void EmLaoshan_RunState(Em *em)
{
    switch (em->stateGroup) {
    case 0:
        EmLaoshan_RunGroup0(em);
        break;
    case 1:
        EmLaoshan_RunGroup1(em);
        break;
    case 2:
        EmLaoshan_RunGroup2();
        break;
    case 3:
        EmLaoshan_RunGroup3(em);
        break;
    case 4:
        EmLaoshan_RunGroup4(em);
        break;
    case 5:
        EmLaoshan_RunGroup5(em);
        break;
    case 6:
        EmLaoshan_RunGroup6(em);
        break;
    case 7:
        EmLaoshan_RunGroup6(em);
        
    default:
        break;
    }
}

void EmLaoshan_RequestOnFrame(s32 *em, s32 frame, s32 i)
{
    if (EmMotion_PassedFrameScaled((Em *) em, (f32) frame, 0U) != 0) {
        CamShake_ReqAIf((Em *) em, i);
    }
}

static void EmLaoshan_MotionEffectsDefault(Em *em)
{
}

void EmLaoshan_MotionEffects(Em *em, EmLaoshanWork *work)
{
    if (em->motionId[0] != work->fxMotion) {
        work->unk00A = 0;
        work->fxMotion = em->motionId[0];
    }
    EmLaoshan_MotionSounds(em, (struct EmWork *)work);
    switch (work->fxMotion) {
    case 0x3E9:
        if (EmMotion_PassedFrameScaled(em, 4.0f, 0) != 0) {
            LaoShanAttack_CreateSpan(em, 0x47, 0x26);
        }
        if (EmMotion_PassedFrameScaled(em, 74.0f, 0) != 0) {
            LaoShanAttack_CreateSpan(em, 0x47, 0x38);
        }
        if (EmMotion_PassedFrameScaled(em, 172.0f, 0) != 0) {
            LaoShanAttack_CreateSpan(em, 0x47, 0x94);
            break;
        }
        break;
    case 0x3EA:
        EmLaoshan_RequestOnFrame((s32 *) em, 0x92, 2);
        if (EmMotion_PassedFrameScaled(em, 146.0f, 0) != 0) {
            StageShot_CreateAtSpot(em, 4, work->fxSpot);
            work->fxSpot += 1;
            work->fxSpot &= 3;
        }
        EmLaoshan_RequestOnFrame((s32 *) em, 0xB2, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0x122, 2);
        if (EmMotion_PassedFrameScaled(em, 290.0f, 0) != 0) {
            StageShot_CreateAtSpot(em, 4, work->fxSpot);
            work->fxSpot += 1;
            work->fxSpot &= 3;
        }
        EmLaoshan_RequestOnFrame((s32 *) em, 0x17E, 2);
        if (EmMotion_PassedFrameScaled(em, 68.0f, 0) != 0) {
            LaoShanAttack_Create(em, 5);
        }
        if (EmMotion_PassedFrameScaled(em, 246.0f, 0) != 0) {
            LaoShanAttack_Create(em, 6);
        }
        if (EmMotion_PassedFrameScaled(em, 350.0f, 0) != 0) {
            LaoShanAttack_Create(em, 7);
        }
        if (EmMotion_PassedFrameScaled(em, 144.0f, 0) != 0) {
            LaoShanAttack_Create(em, 8);
        }
        if (EmMotion_PassedFrameScaled(em, 176.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0xE, 0x13, 0.5f);
        }
        if (EmMotion_PassedFrameScaled(em, 380.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 8, 0x13, 0.5f);
        }
        if (EmMotion_PassedFrameScaled(em, 284.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x1D, 0x13, 0.6f);
        }
        if (EmMotion_PassedFrameScaled(em, 136.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x17, 0x13, 0.6f);
            break;
        }
        break;
    case 0x3EB:
        EmLaoshan_RequestOnFrame((s32 *) em, 0x2C, 2);
        if (EmMotion_PassedFrameScaled(em, 44.0f, 0) != 0) {
            StageShot_CreateAtSpot(em, 4, work->fxSpot);
            work->fxSpot += 1;
            work->fxSpot &= 3;
        }
        EmLaoshan_RequestOnFrame((s32 *) em, 0x9A, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0xF8, 2);
        if (EmMotion_PassedFrameScaled(em, 248.0f, 0) != 0) {
            StageShot_CreateAtSpot(em, 4, work->fxSpot);
            work->fxSpot += 1;
            work->fxSpot &= 3;
        }
        EmLaoshan_RequestOnFrame((s32 *) em, 0x16E, 2);
        if (EmMotion_PassedFrameScaled(em, 130.0f, 0) != 0) {
            LaoShanAttack_Create(em, 1);
        }
        if (EmMotion_PassedFrameScaled(em, 314.0f, 0) != 0) {
            LaoShanAttack_Create(em, 2);
        }
        if (EmMotion_PassedFrameScaled(em, 222.0f, 0) != 0) {
            LaoShanAttack_Create(em, 3);
        }
        if (EmMotion_PassedFrameScaled(em, 18.0f, 0) != 0) {
            LaoShanAttack_Create(em, 4);
        }
        if (EmMotion_PassedFrameScaled(em, 108.0f, 0) != 0) {
            LaoShanAttack_CreateSpan(em, 0x47, 0x42);
        }
        if (EmMotion_PassedFrameScaled(em, 250.0f, 0) != 0) {
            LaoShanAttack_CreateSpan(em, 0x47, 0x22);
        }
        if (EmMotion_PassedFrameScaled(em, 346.0f, 0) != 0) {
            LaoShanAttack_CreateSpan(em, 0x47, 0x3E);
        }
        if (EmMotion_PassedFrameScaled(em, 442.0f, 0) != 0) {
            LaoShanAttack_CreateSpan(em, 0x47, 0x14);
        }
        if (EmMotion_PassedFrameScaled(em, 44.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0xE, 0x13, 0.5f);
        }
        if (EmMotion_PassedFrameScaled(em, 254.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 8, 0x13, 0.5f);
        }
        if (EmMotion_PassedFrameScaled(em, 360.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x1D, 0x13, 0.6f);
        }
        if (EmMotion_PassedFrameScaled(em, 152.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x17, 0x13, 0.6f);
            break;
        }
        break;
    case 0x3EC:
        EmLaoshan_RequestOnFrame((s32 *) em, 0x52, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0xC4, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0x13A, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0x1AA, 2);
        if (EmMotion_PassedFrameScaled(em, 380.0f, 0) != 0) {
            LaoShanAttack_Create(em, 9);
        }
        if (EmMotion_PassedFrameScaled(em, 170.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0xA);
        }
        if (EmMotion_PassedFrameScaled(em, 50.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0xB);
        }
        if (EmMotion_PassedFrameScaled(em, 276.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0xC);
        }
        if (EmMotion_PassedFrameScaled(em, 96.0f, 0) != 0) {
            LaoShanAttack_CreateSpan(em, 0x47, 0x1A);
        }
        if (EmMotion_PassedFrameScaled(em, 158.0f, 0) != 0) {
            LaoShanAttack_CreateSpan(em, 0x47, 0x3C);
        }
        if (EmMotion_PassedFrameScaled(em, 352.0f, 0) != 0) {
            LaoShanAttack_CreateSpan(em, 0x47, 0x1A);
        }
        if (EmMotion_PassedFrameScaled(em, 394.0f, 0) != 0) {
            LaoShanAttack_CreateSpan(em, 0x47, 0x38);
        }
        if (EmMotion_PassedFrameScaled(em, 310.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0xE, 0x13, 0.5f);
        }
        if (EmMotion_PassedFrameScaled(em, 80.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 8, 0x13, 0.5f);
        }
        if (EmMotion_PassedFrameScaled(em, 192.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x1D, 0x13, 0.6f);
        }
        if (EmMotion_PassedFrameScaled(em, 424.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x17, 0x13, 0.6f);
            break;
        }
        break;
    case 0x3ED:
        EmLaoshan_RequestOnFrame((s32 *) em, 0x52, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0xC4, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0x13A, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0x1AA, 2);
        if (EmMotion_PassedFrameScaled(em, 170.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0xD);
        }
        if (EmMotion_PassedFrameScaled(em, 380.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0xE);
        }
        if (EmMotion_PassedFrameScaled(em, 276.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0xF);
        }
        if (EmMotion_PassedFrameScaled(em, 50.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x10);
        }
        if (EmMotion_PassedFrameScaled(em, 96.0f, 0) != 0) {
            LaoShanAttack_CreateSpan(em, 0x47, 0x1A);
        }
        if (EmMotion_PassedFrameScaled(em, 158.0f, 0) != 0) {
            LaoShanAttack_CreateSpan(em, 0x47, 0x3C);
        }
        if (EmMotion_PassedFrameScaled(em, 352.0f, 0) != 0) {
            LaoShanAttack_CreateSpan(em, 0x47, 0x1A);
        }
        if (EmMotion_PassedFrameScaled(em, 394.0f, 0) != 0) {
            LaoShanAttack_CreateSpan(em, 0x47, 0x38);
        }
        if (EmMotion_PassedFrameScaled(em, 80.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0xE, 0x13, 0.5f);
        }
        if (EmMotion_PassedFrameScaled(em, 310.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 8, 0x13, 0.5f);
        }
        if (EmMotion_PassedFrameScaled(em, 424.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x1D, 0x13, 0.6f);
        }
        if (EmMotion_PassedFrameScaled(em, 192.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x17, 0x13, 0.6f);
            break;
        }
        break;
    case 0x3EE:
        EmLaoshan_RequestOnFrame((s32 *) em, 0x52, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0xAA, 2);
        if (EmMotion_PassedFrameScaled(em, 40.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x11);
        }
        if (EmMotion_PassedFrameScaled(em, 80.0f, 0) != 0) {
            LaoShanAttack_CreateSpan(em, 0x47, 0x64);
        }
        if (EmMotion_PassedFrameScaled(em, 158.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0xE, 0x13, 0.5f);
        }
        if (EmMotion_PassedFrameScaled(em, 70.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x17, 0x13, 0.6f);
            break;
        }
        break;
    case 0x3EF:
        EmLaoshan_RequestOnFrame((s32 *) em, 0x17C, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0x1CC, 2);
        if (EmMotion_PassedFrameScaled(em, 326.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x12);
        }
        if (EmMotion_PassedFrameScaled(em, 80.0f, 0) != 0) {
            LaoShanAttack_CreateSpan(em, 0x47, 0xC8);
        }
        if (EmMotion_PassedFrameScaled(em, 450.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0xE, 0x13, 0.5f);
        }
        if (EmMotion_PassedFrameScaled(em, 368.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x17, 0x13, 0.6f);
        }
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x51);
            StageObj_K0F_CreateOnEm(em, 5, 0x14, 1.5f);
            break;
        }
        break;
    case 0x3F0:
        if (EmMotion_PassedFrameScaled(em, 326.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x13);
        }
        if (EmMotion_PassedFrameScaled(em, 326.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x14);
            break;
        }
        break;
    case 0x3F2:
        EmLaoshan_RequestOnFrame((s32 *) em, 0x3E, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0x6A, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0x96, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0xD2, 5);
        EmLaoshan_RequestOnFrame((s32 *) em, 0x1A4, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0x1DC, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0x1FE, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0x222, 2);
        if (EmMotion_PassedFrameScaled(em, 150.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x15);
        }
        if (EmMotion_PassedFrameScaled(em, 102.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x16);
        }
        if (EmMotion_PassedFrameScaled(em, 24.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x17);
        }
        if (EmMotion_PassedFrameScaled(em, 64.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x18);
        }
        if (EmMotion_PassedFrameScaled(em, 184.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x19);
        }
        if (EmMotion_PassedFrameScaled(em, 322.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x1A);
        }
        if (EmMotion_PassedFrameScaled(em, 434.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x1A);
        }
        if (EmMotion_PassedFrameScaled(em, 210.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x55);
        }
        if (EmMotion_PassedFrameScaled(em, 98.0f, 0) || EmMotion_PassedFrameScaled(em, 424.0f, 0) || EmMotion_PassedFrameScaled(em, 540.0f, 0)) {
            Effect0D_SpawnForEmScaled(em, 0xE, 0x13, 0.5f);
        }
        if ((EmMotion_PassedFrameScaled(em, 58.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 468.0f, 0) != 0)) {
            Effect0D_SpawnForEmScaled(em, 8, 0x13, 0.5f);
        }
        if ((EmMotion_PassedFrameScaled(em, 136.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 504.0f, 0) != 0)) {
            Effect0D_SpawnForEmScaled(em, 0x1D, 0x13, 0.6f);
        }
        if ((EmMotion_PassedFrameScaled(em, 192.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 404.0f, 0) != 0)) {
            Effect0D_SpawnForEmScaled(em, 0x17, 0x13, 0.6f);
            break;
        }
        break;
    case 0x3F4:
        if (EmMotion_PassedFrameScaled(em, 130.0f, 0) != 0) {
            LaoShanAttack_CreateSpan(em, 0x47, 0x4C);
        }
        if (EmMotion_PassedFrameScaled(em, 320.0f, 0) != 0) {
            LaoShanAttack_CreateSpan(em, 0x47, 0x42);
            break;
        }
        break;
    case 0x3F5:
        EmLaoshan_RequestOnFrame((s32 *) em, 0x4C, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0x1FA, 2);
        if (EmMotion_PassedFrameScaled(em, 162.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x1B);
        }
        if (EmMotion_PassedFrameScaled(em, 48.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x1C);
        }
        if (EmMotion_PassedFrameScaled(em, 442.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x1D);
        }
        if (EmMotion_PassedFrameScaled(em, 152.0f, 0) != 0) {
            LaoShanAttack_CreateSpan(em, 0x47, 0xA);
        }
        if (EmMotion_PassedFrameScaled(em, 364.0f, 0) != 0) {
            LaoShanAttack_CreateSpan(em, 0x47, 0x4C);
        }
        if (EmMotion_PassedFrameScaled(em, 500.0f, 0) != 0) {
            LaoShanAttack_CreateSpan(em, 0x47, 0x3A);
        }
        if (EmMotion_PassedFrameScaled(em, 74.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x17, 0x13, 0.6f);
        }
        if (EmMotion_PassedFrameScaled(em, 494.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x17, 0x13, 0.6f);
            break;
        }
        break;
    case 0x3F6:
        EmLaoshan_RequestOnFrame((s32 *) em, 0x56, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0x9A, 5);
        EmLaoshan_RequestOnFrame((s32 *) em, 0x106, 2);
        if (EmMotion_PassedFrameScaled(em, 134.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x1E);
        }
        if (EmMotion_PassedFrameScaled(em, 60.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x1F);
        }
        if (EmMotion_PassedFrameScaled(em, 216.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x20);
        }
        if (EmMotion_PassedFrameScaled(em, 156.0f, 0) != 0) {
            LaoShanAttack_CreateSpan(em, 0x47, 0x2C);
        }
        if (EmMotion_PassedFrameScaled(em, 252.0f, 0) != 0) {
            LaoShanAttack_CreateSpan(em, 0x47, 0x1C);
        }
        if (EmMotion_PassedFrameScaled(em, 82.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x1D, 0x13, 0.5f);
        }
        if (EmMotion_PassedFrameScaled(em, 258.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x1D, 0x13, 0.5f);
            break;
        }
        break;
    case 0x3F7:
        EmLaoshan_RequestOnFrame((s32 *) em, 0x84, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0x11C, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0x1DA, 5);
        if (EmMotion_PassedFrameScaled(em, 70.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x21);
        }
        if (EmMotion_PassedFrameScaled(em, 348.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x22);
        }
        if (EmMotion_PassedFrameScaled(em, 204.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x23);
        }
        if (EmMotion_PassedFrameScaled(em, 458.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x24);
        }
        if (EmMotion_PassedFrameScaled(em, 458.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x25);
        }
        if (EmMotion_PassedFrameScaled(em, 64.0f, 0) != 0) {
            LaoShanAttack_CreateSpan(em, 0x47, 0x2A);
        }
        if (EmMotion_PassedFrameScaled(em, 250.0f, 0) != 0) {
            LaoShanAttack_CreateSpan(em, 0x47, 0x32);
        }
        if (EmMotion_PassedFrameScaled(em, 370.0f, 0) != 0) {
            LaoShanAttack_CreateSpan(em, 0x47, 0x32);
        }
        if (EmMotion_PassedFrameScaled(em, 464.0f, 0) != 0) {
            LaoShanAttack_CreateSpan(em, 0x47, 0x12);
        }
        if (EmMotion_PassedFrameScaled(em, 472.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0xE, 0x13, 0.8f);
        }
        if (EmMotion_PassedFrameScaled(em, 470.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 8, 0x13, 0.8f);
        }
        if (EmMotion_PassedFrameScaled(em, 274.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x1D, 0x13, 0.6f);
        }
        if ((EmMotion_PassedFrameScaled(em, 114.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 394.0f, 0) != 0)) {
            Effect0D_SpawnForEmScaled(em, 0x17, 0x13, 0.6f);
            break;
        }
        break;
    case 0x3F8:
        EmLaoshan_RequestOnFrame((s32 *) em, 0x1E, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0xA8, 2);
        if (EmMotion_PassedFrameScaled(em, 26.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x1D, 0x13, 0.5f);
        }
        if (EmMotion_PassedFrameScaled(em, 164.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x17, 0x13, 0.6f);
        }
        if (EmMotion_PassedFrameScaled(em, 148.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x48);
        }
        if (EmMotion_PassedFrameScaled(em, 12.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x49);
        }
        if (EmMotion_PassedFrameScaled(em, 182.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x4A);
        }
        if (EmMotion_PassedFrameScaled(em, 24.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x4B);
        }
        if (EmMotion_PassedFrameScaled(em, 24.0f, 0) != 0) {
            LaoShanAttack_CreateSpan(em, 0x47, 0x24);
        }
        if (EmMotion_PassedFrameScaled(em, 170.0f, 0) != 0) {
            LaoShanAttack_CreateSpan(em, 0x47, 0x28);
            break;
        }
        break;
    case 0x3FC:
        EmLaoshan_RequestOnFrame((s32 *) em, 0x52, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0xA6, 2);
        if (EmMotion_PassedFrameScaled(em, 60.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x26);
        }
        if (EmMotion_PassedFrameScaled(em, 156.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x27);
        }
        if (EmMotion_PassedFrameScaled(em, 100.0f, 0) != 0) {
            LaoShanAttack_CreateSpan(em, 0x47, 0x24);
        }
        if (EmMotion_PassedFrameScaled(em, 154.0f, 0) != 0) {
            LaoShanAttack_CreateSpan(em, 0x47, 0x2E);
        }
        if (EmMotion_PassedFrameScaled(em, 162.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0xE, 0x13, 0.5f);
        }
        if (EmMotion_PassedFrameScaled(em, 76.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 8, 0x13, 0.5f);
            break;
        }
        break;
    case 0x3FD:
    case 0x403:
        if (EmMotion_PassedFrameScaled(em, 28.0f, 0) != 0) {
            LaoShanAttack_CreateSpan(em, 0x47, 0x34);
        }
        if (EmMotion_PassedFrameScaled(em, 154.0f, 0) != 0) {
            LaoShanAttack_CreateSpan(em, 0x47, 0x32);
            break;
        }
        break;
    case 0x3FE:
        EmLaoshan_RequestOnFrame((s32 *) em, 0x50, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0x8C, 2);
        if (EmMotion_PassedFrameScaled(em, 104.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x28);
        }
        if (EmMotion_PassedFrameScaled(em, 50.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x29);
        }
        if (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0) {
            LaoShanAttack_CreateSpan(em, 0x47, 0x2C);
        }
        if (EmMotion_PassedFrameScaled(em, 74.0f, 0) != 0) {
            LaoShanAttack_CreateSpan(em, 0x47, 0x48);
        }
        if (EmMotion_PassedFrameScaled(em, 78.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0xE, 0x13, 0.5f);
        }
        if (EmMotion_PassedFrameScaled(em, 138.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 8, 0x13, 0.5f);
            break;
        }
        break;
    case 0x3FF:
        EmLaoshan_RequestOnFrame((s32 *) em, 0x4A, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0x9E, 2);
        if (EmMotion_PassedFrameScaled(em, 132.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x2A);
        }
        if (EmMotion_PassedFrameScaled(em, 46.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x2B);
        }
        if (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) {
            LaoShanAttack_CreateSpan(em, 0x47, 0x1E);
        }
        if (EmMotion_PassedFrameScaled(em, 82.0f, 0) != 0) {
            LaoShanAttack_CreateSpan(em, 0x47, 0x4E);
        }
        if (EmMotion_PassedFrameScaled(em, 70.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x1D, 0x13, 0.6f);
        }
        if (EmMotion_PassedFrameScaled(em, 154.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x17, 0x13, 0.6f);
            break;
        }
        break;
    case 0x400:
        EmLaoshan_RequestOnFrame((s32 *) em, 0x52, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0xA6, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0xFC, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0x150, 2);
        if (EmMotion_PassedFrameScaled(em, 120.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x2C);
        }
        if (EmMotion_PassedFrameScaled(em, 48.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x2D);
        }
        if (EmMotion_PassedFrameScaled(em, 216.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x2E);
        }
        if (EmMotion_PassedFrameScaled(em, 306.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x2F);
        }
        if (EmMotion_PassedFrameScaled(em, 50.0f, 0) != 0) {
            LaoShanAttack_CreateSpan(em, 0x47, 0x26);
        }
        if (EmMotion_PassedFrameScaled(em, 120.0f, 0) != 0) {
            LaoShanAttack_CreateSpan(em, 0x47, 0x7A);
        }
        if (EmMotion_PassedFrameScaled(em, 266.0f, 0) != 0) {
            LaoShanAttack_CreateSpan(em, 0x47, 0x40);
        }
        if (EmMotion_PassedFrameScaled(em, 334.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0xE, 0x13, 0.5f);
        }
        if (EmMotion_PassedFrameScaled(em, 248.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 8, 0x13, 0.5f);
        }
        if (EmMotion_PassedFrameScaled(em, 78.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x1D, 0x13, 0.6f);
        }
        if (EmMotion_PassedFrameScaled(em, 162.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x17, 0x13, 0.6f);
            break;
        }
        break;
    case 0x401:
        EmLaoshan_RequestOnFrame((s32 *) em, 0x50, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0xA2, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0xCE, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0x11C, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0x16A, 2);
        if (EmMotion_PassedFrameScaled(em, 34.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x30);
        }
        if (EmMotion_PassedFrameScaled(em, 208.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x31);
        }
        if (EmMotion_PassedFrameScaled(em, 96.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x32);
        }
        if (EmMotion_PassedFrameScaled(em, 162.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x33);
        }
        if (EmMotion_PassedFrameScaled(em, 300.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x4C);
        }
        if (EmMotion_PassedFrameScaled(em, 368.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x4D);
        }
        if (EmMotion_PassedFrameScaled(em, 18.0f, 0) != 0) {
            LaoShanAttack_CreateSpan(em, 0x47, 0x56);
        }
        if (EmMotion_PassedFrameScaled(em, 280.0f, 0) != 0) {
            LaoShanAttack_CreateSpan(em, 0x47, 0x8C);
        }
        if (EmMotion_PassedFrameScaled(em, 204.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0xE, 0x13, 0.5f);
        }
        if (EmMotion_PassedFrameScaled(em, 158.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 8, 0x13, 0.5f);
        }
        if (EmMotion_PassedFrameScaled(em, 280.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x1D, 0x13, 0.6f);
        }
        if ((EmMotion_PassedFrameScaled(em, 76.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 358.0f, 0) != 0)) {
            Effect0D_SpawnForEmScaled(em, 0x17, 0x13, 0.6f);
            break;
        }
        break;
    case 0x402:
        EmLaoshan_RequestOnFrame((s32 *) em, 0x52, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0xA6, 2);
        if (EmMotion_PassedFrameScaled(em, 156.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x34);
        }
        if (EmMotion_PassedFrameScaled(em, 60.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x35);
        }
        if (EmMotion_PassedFrameScaled(em, 100.0f, 0) != 0) {
            LaoShanAttack_CreateSpan(em, 0x47, 0x24);
        }
        if (EmMotion_PassedFrameScaled(em, 154.0f, 0) != 0) {
            LaoShanAttack_CreateSpan(em, 0x47, 0x2E);
        }
        if (EmMotion_PassedFrameScaled(em, 76.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0xE, 0x13, 0.5f);
        }
        if (EmMotion_PassedFrameScaled(em, 162.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 8, 0x13, 0.5f);
            break;
        }
        break;
    case 0x404:
        EmLaoshan_RequestOnFrame((s32 *) em, 0x50, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0x8C, 2);
        if (EmMotion_PassedFrameScaled(em, 50.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x36);
        }
        if (EmMotion_PassedFrameScaled(em, 104.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x37);
        }
        if (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0) {
            LaoShanAttack_CreateSpan(em, 0x47, 0x2C);
        }
        if (EmMotion_PassedFrameScaled(em, 74.0f, 0) != 0) {
            LaoShanAttack_CreateSpan(em, 0x47, 0x48);
        }
        if (EmMotion_PassedFrameScaled(em, 138.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0xE, 0x13, 0.5f);
        }
        if (EmMotion_PassedFrameScaled(em, 78.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 8, 0x13, 0.5f);
            break;
        }
        break;
    case 0x405:
        EmLaoshan_RequestOnFrame((s32 *) em, 0x4A, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0x9E, 2);
        if (EmMotion_PassedFrameScaled(em, 46.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x38);
        }
        if (EmMotion_PassedFrameScaled(em, 132.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x39);
        }
        if (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) {
            LaoShanAttack_CreateSpan(em, 0x47, 0x1E);
        }
        if (EmMotion_PassedFrameScaled(em, 82.0f, 0) != 0) {
            LaoShanAttack_CreateSpan(em, 0x47, 0x4E);
        }
        if (EmMotion_PassedFrameScaled(em, 154.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x1D, 0x13, 0.6f);
        }
        if (EmMotion_PassedFrameScaled(em, 70.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x17, 0x13, 0.6f);
            break;
        }
        break;
    case 0x406:
        EmLaoshan_RequestOnFrame((s32 *) em, 0x52, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0xA6, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0xFC, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0x150, 2);
        if (EmMotion_PassedFrameScaled(em, 48.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x3A);
        }
        if (EmMotion_PassedFrameScaled(em, 120.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x3B);
        }
        if (EmMotion_PassedFrameScaled(em, 306.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x3C);
        }
        if (EmMotion_PassedFrameScaled(em, 216.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x3D);
        }
        if (EmMotion_PassedFrameScaled(em, 50.0f, 0) != 0) {
            LaoShanAttack_CreateSpan(em, 0x47, 0x26);
        }
        if (EmMotion_PassedFrameScaled(em, 120.0f, 0) != 0) {
            LaoShanAttack_CreateSpan(em, 0x47, 0x7A);
        }
        if (EmMotion_PassedFrameScaled(em, 286.0f, 0) != 0) {
            LaoShanAttack_CreateSpan(em, 0x47, 0x40);
        }
        if (EmMotion_PassedFrameScaled(em, 248.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0xE, 0x13, 0.5f);
        }
        if (EmMotion_PassedFrameScaled(em, 334.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 8, 0x13, 0.5f);
        }
        if (EmMotion_PassedFrameScaled(em, 162.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x1D, 0x13, 0.6f);
        }
        if (EmMotion_PassedFrameScaled(em, 78.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x17, 0x13, 0.6f);
            break;
        }
        break;
    case 0x407:
        EmLaoshan_RequestOnFrame((s32 *) em, 0x50, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0xA2, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0xCE, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0x11C, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0x16A, 2);
        if (EmMotion_PassedFrameScaled(em, 208.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x3E);
        }
        if (EmMotion_PassedFrameScaled(em, 34.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x3F);
        }
        if (EmMotion_PassedFrameScaled(em, 162.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x40);
        }
        if (EmMotion_PassedFrameScaled(em, 96.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x41);
        }
        if (EmMotion_PassedFrameScaled(em, 368.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x4E);
        }
        if (EmMotion_PassedFrameScaled(em, 300.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x4F);
        }
        if (EmMotion_PassedFrameScaled(em, 18.0f, 0) != 0) {
            LaoShanAttack_CreateSpan(em, 0x47, 0x56);
        }
        if (EmMotion_PassedFrameScaled(em, 280.0f, 0) != 0) {
            LaoShanAttack_CreateSpan(em, 0x47, 0x8C);
        }
        if (EmMotion_PassedFrameScaled(em, 158.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0xE, 0x13, 0.5f);
        }
        if (EmMotion_PassedFrameScaled(em, 204.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 8, 0x13, 0.5f);
        }
        if ((EmMotion_PassedFrameScaled(em, 76.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 358.0f, 0) != 0)) {
            Effect0D_SpawnForEmScaled(em, 0x1D, 0x13, 0.6f);
        }
        if (EmMotion_PassedFrameScaled(em, 280.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x17, 0x13, 0.6f);
            break;
        }
        break;
    case 0x424:
        EmLaoshan_RequestOnFrame((s32 *) em, 0x56, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0x5A, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0x90, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0xAA, 5);
        EmLaoshan_RequestOnFrame((s32 *) em, 0xDA, 2);
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x52);
        }
        if (EmMotion_PassedFrameScaled(em, 58.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x53);
        }
        if (EmMotion_PassedFrameScaled(em, 110.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x54);
        }
        if (EmMotion_PassedFrameScaled(em, 150.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x56);
        }
        if (EmMotion_PassedFrameScaled(em, 130.0f, 0) != 0) {
            StageObj_K0A_Create(em, 0, 3, 1.2f);
        }
        if (EmMotion_PassedFrameScaled(em, 128.0f, 0) != 0) {
            StageObj_K0A_Create(em, 0, 1, 1.4f);
        }
        if (EmMotion_PassedFrameScaled(em, 146.0f, 0) != 0) {
            StageObj_K0A_Create(em, 0, 4, 1.2f);
            break;
        }
        break;
    case 0x425:
        EmLaoshan_RequestOnFrame((s32 *) em, 0x64, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0xAE, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0xF0, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0x146, 2);
        if (EmMotion_PassedFrameScaled(em, 94.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0xE, 0x13, 0.5f);
        }
        if (EmMotion_PassedFrameScaled(em, 156.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 8, 0x13, 0.5f);
        }
        if (EmMotion_PassedFrameScaled(em, 310.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x1D, 0x13, 0.6f);
        }
        if (EmMotion_PassedFrameScaled(em, 230.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x17, 0x13, 0.6f);
            break;
        }
        break;
    case 0x426:
        break;
    case 0x427:
        EmLaoshan_RequestOnFrame((s32 *) em, 0x14, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0x18, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0x40, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0x68, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0xB4, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0xD6, 2);
        if (EmMotion_PassedFrameScaled(em, 186.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0xE, 0x13, 0.5f);
        }
        if ((EmMotion_PassedFrameScaled(em, 58.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 158.0f, 0) != 0)) {
            Effect0D_SpawnForEmScaled(em, 8, 0x13, 0.5f);
        }
        if ((EmMotion_PassedFrameScaled(em, 32.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 210.0f, 0) != 0)) {
            Effect0D_SpawnForEmScaled(em, 0x1D, 0x13, 0.6f);
        }
        if (EmMotion_PassedFrameScaled(em, 100.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x17, 0x13, 0.6f);
            break;
        }
        break;
    case 0x428:
        break;
    case 0x429:
        EmLaoshan_RequestOnFrame((s32 *) em, 0x26, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0xA4, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0xBC, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0x124, 2);
        if (EmMotion_PassedFrameScaled(em, 162.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0xE, 0x13, 0.8f);
        }
        if (EmMotion_PassedFrameScaled(em, 180.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 8, 0x13, 0.8f);
        }
        if (EmMotion_PassedFrameScaled(em, 288.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0x1D, 0x13, 0.6f);
            break;
        }
        break;
    case 0x42A:
        break;
    case 0x42B:
        EmLaoshan_RequestOnFrame((s32 *) em, 0xD4, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0x154, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0x180, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0x1A6, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0x1D8, 2);
        if (EmMotion_PassedFrameScaled(em, 338.0f, 0) != 0) {
            StageObj_K0A_Create(em, 0, 0, 1.2f);
        }
        if (EmMotion_PassedFrameScaled(em, 400.0f, 0) != 0) {
            StageObj_K0A_Create(em, 0, 1, 1.4f);
        }
        if (EmMotion_PassedFrameScaled(em, 406.0f, 0) != 0) {
            StageObj_K0A_Create(em, 0, 2, 1.2f);
            break;
        }
        break;
    case 0x42C:
        EmLaoshan_RequestOnFrame((s32 *) em, 0x5A, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0x6A, 2);
        EmLaoshan_RequestOnFrame((s32 *) em, 0xB4, 2);
        if (EmMotion_PassedFrameScaled(em, 94.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x57);
        }
        if (EmMotion_PassedFrameScaled(em, 82.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0xE, 0x13, 0.8f);
        }
        if (EmMotion_PassedFrameScaled(em, 236.0f, 0) != 0) {
            Effect0D_SpawnForEmScaled(em, 0xE, 0x13, 0.5f);
            break;
        }
        break;
    default:
        EmLaoshan_MotionEffectsDefault(em);
        break;
    }
}

void EmLaoshan_UpdateMotionEffects(Em *em)
{
    EmLaoshanWork *w = (EmLaoshanWork *)&em->work;

    switch (w->fxStep) {
    case 0:
        w->fxStep++;
        break;
    case 1:
        EmLaoshan_MotionEffects(em, w);
        break;
    }
}

void EmLaoshan_SpawnAreaImpactEffects(Em *em)
{
    f32 pos[3];

    switch (Area_DayId(em->area) & 0xFFFF) {
    case AREA_FORTRESS_4:
        pos[0] = 14000.0f;
        pos[1] = 1700.0f;
        pos[2] = 20500.0f;
        break;
    case AREA_FORTRESS_5:
        pos[0] = 7300.0f;
        pos[1] = 1500.0f;
        pos[2] = 12400.0f;
        break;
    case AREA_FORTRESS_2:
        pos[0] = 15000.0f;
        pos[1] = 1700.0f;
        pos[2] = 23000.0f;
        break;
    default:
        pos[0] = 7300.0f;
        pos[1] = 1500.0f;
        pos[2] = 12400.0f;
        break;
    }
    Effect02_SpawnForOwner(em, 0, 9, 0, pos, 1.0f);
    Effect02_SpawnForOwner(em, 0, 9, 1, pos, 1.0f);
    Effect0D_SpawnIfHere(em, pos, 0x22, 10.0f);
}

void EmLaoshan_SpawnAreaImpactEffects2(Em *em)
{
    f32 pos[3];

    switch (Area_DayId(em->area) & 0xFFFF) {
    case AREA_FORTRESS_5:
        pos[0] = 7300.0f;
        pos[1] = 500.0f;
        pos[2] = 12400.0f;
        break;
    case AREA_FORTRESS_3:
        pos[0] = 10900.0f;
        pos[1] = 500.0f;
        pos[2] = 20158.0f;
        break;
    default:
        pos[0] = 7300.0f;
        pos[1] = 1500.0f;
        pos[2] = 12400.0f;
        break;
    }
    Em_PlaySeChecked(em, 0x45, 0, pos, 9, 0);
    Em_PlaySeChecked(em, 0x44, 0, pos, 9, 0);
    Effect02_SpawnForOwner(em, 0, 9, 0, pos, 2.0f);
    Effect02_SpawnForOwner(em, 0, 9, 1, pos, 2.0f);
    Effect0D_SpawnIfHere(em, pos, 0x22, 20.0f);
}

void EmLaoshan_TableNop(void)
{
}

void EmLaoshan_SetGroup0State(Em *em, u16 state, u8 mode)
{
    em->motionSpeed = 1.0f;
    Em_SetStateSync(em, 0, state, mode);
}

void EmLaoshan_SetGroup1State(Em *em, u16 state, u8 mode)
{
    s32 a;

    em->motionSpeed = 1.0f;
    switch (state) {
    case 0:
    case 16:
        Em_SetSpeedScaled(em, 6, 500.0f, 0.0f);
        break;
    case 1:
        Em_SetSpeedScaled(em, 6, 500.0f, 0.0f);
        a = (u16) (Math_AngleXZ(&em->posX, em->goal) - em->rotY);
        if ((0 <= a && a <= 0x4000) || (a >= 0xC000 && a <= 0xFFFF)) {
            state = 0;
        }
        break;
    case 2:
        Math_AngleXZ(&em->posX, em->goal);
        break;
    case 17:
        Em_SetSpeedScaled(em, 6, 500.0f, 0.0f);
        break;
    case 19:
        Em_SetSpeedScaled(em, 6, 500.0f, 0.0f);
        a = (u16) (Math_AngleXZ(&em->posX, em->goal) - em->rotY);
        if ((0 <= a && a <= 0x4000) || (a >= 0xC000 && a <= 0xFFFF)) {
            state = 18;
        }
        break;
    }
    Em_SetStateSync(em, 1, state, mode);
}

void EmLaoshan_SetGroup2State(Em *em, u16 state, u8 mode)
{
    em->motionSpeed = 1.0f;
    Em_SetStateSync(em, 2, state, mode);
}

void EmLaoshan_SetGroup3State(Em *em, u16 state, u8 mode)
{
    em->motionSpeed = 1.0f;
    Em_SetStateSync(em, 3, state, mode);
}

void EmLaoshan_ChangeState(Em *em, u16 group, u16 state, u8 mode)
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
        EmLaoshan_SetGroup0State(em, state, mode);
        return;
    case 1:
        EmLaoshan_SetGroup1State(em, state, mode);
        return;
    case 2:
        EmLaoshan_SetGroup2State(em, state, mode);
        return;
    case 3:
        EmLaoshan_SetGroup3State(em, state, mode);
        return;
    case 5:
    case 6:
    case 4:
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        
    default:
        return;
    }
}

void EmCephadrome_MotionSounds(Em *em, EmWork *work)
{
    s16 prevMotion;

    prevMotion = work->prevMotion;
    switch (prevMotion) {
    case 0x3E9:
        Em_PlaySeAtBoneOnFrame(em, 0x40, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xC8, Sound_RandomSe(0x2D, 2, 0x2D, 2), 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x19A, Sound_RandomSe(0x21, 4, 0x21, 4), 0x23);
        break;
    case 0x3EB:
        Em_PlaySeAtBoneOnFrame(em, 0x34, 1, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x74, 1, 0x1A);
        break;
    case 0x3EC:
        Em_PlaySeTypeAtBoneOnFrame(em, 0xA, 0x56, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 6, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 6, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 6, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 6, 0x1A);
        break;
    case 0x3ED:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x21, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 1, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 1, 0x1A);
        break;
    case 0x3EE:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x21, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 1, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 1, 0x1A);
        break;
    case 0x3F2:
        Em_PlaySeTypeAtBoneOnFrame(em, 6, 0x59, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 6, 4, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 0, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 0x12, 0x14);
        break;
    case 0x3F3:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0xB, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x42, 0xB, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x7A, 0xB, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x7E, 0xB, 0xC);
        break;
    case 0x3F9:
        Em_PlaySeTypeAtBoneOnFrame(em, 6, 0x4F, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 6, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 6, 0x1A);
        break;
    case 0x3FA:
        Em_PlaySeAtBoneOnFrame(em, 0x38, 7, 0x1A);
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
        Em_PlaySeAtBoneOnFrame(em, 0x114, 0x27, 0x23);
        break;
    case 0x407:
        Em_PlaySeTypeAtBoneOnFrame(em, 2, 0x55, 0x23);
        Em_PlaySeTypeAtBoneOnFrame(em, 0xB6, 0x55, 0x23);
        Em_PlaySeTypeAtBoneOnFrame(em, 0x16C, 0x55, 0x23);
        break;
    case 0x408:
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 0x57, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x8E, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x1E, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x13, 0);
        break;
    case 0x409:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x20, 0x23);
        Em_PlaySeTypeAtBoneOnFrame(em, 0x46, 0x55, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x7E, 3, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0xB6, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0x1D, 0);
        break;
    case 0x40A:
        Em_PlaySeTypeAtBoneOnFrame(em, 0x24, 0x57, 0x23);
        break;
    case 0x40C:
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0x24, 0x23);
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
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 0x57, 0x23);
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
        Em_PlaySeTypeAtBoneOnFrame(em, 0x4A, 0x51, 0x23);
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
    case 0x423:
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x1D, 0);
        Em_PlaySeAtBoneOnFrame(em, 6, 0x22, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x68, 3, 0x1A);
        break;
    case 0x424:
        Em_PlaySeTypeAtBoneOnFrame(em, 6, 0x53, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x94, 0x22, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 0x13, 0x2A);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 6, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0x11, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x74, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x94, 3, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0xC0, 3, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 0xC, 6);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0xD, 0xC);
        break;
    case 0x425:
        Em_PlaySeTypeAtBoneOnFrame(em, 6, 0x53, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x12, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 6, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 4, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x6E, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x5A, 0x15, 0);
        Em_PlaySeAtBoneOnFrame(em, 0xAC, 3, 0x1A);
        break;
    case 0x426:
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 0x4F, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x56, 0x1F, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 4, 0xD, 6);
        Em_PlaySeAtBoneOnFrame(em, 8, 0xD, 0xC);
        Em_PlaySeAtBoneOnFrame(em, 0x62, 0x13, 0x2A);
        break;
    case 0x427:
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 0x52, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 0x1F, 0x23);
        break;
    case 0x428:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x2A, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x7A, 3, 0x1A);
        break;
    case 0x42A:
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 0x52, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x13, 0);
        break;
    case 0x42B:
        Em_PlaySeTypeAtBoneOnFrame(em, 0x46, 0x55, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0x1E, 0);
        break;
    case 0x42C:
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x30, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 9, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x14, 0);
        Em_PlaySeTypeAtBoneOnFrame(em, 0x1A, 0x56, 0x23);
        break;
    case 0x42D:
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 0x52, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x13, 0x2A);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 9, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0x11, 0);
        break;
    case 0x42E:
        Em_PlaySeAtBoneOnFrame(em, 0x14, 8, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 4, 0x22);
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 0x54, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 8, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 0x12, 0);
        break;
    case 0x42F:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x20, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 1, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 6, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x1D, 0);
        break;
    case 0x432:
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 0x52, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x13, 0x2A);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 9, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0x11, 0);
        break;
    case 0x433:
        Em_PlaySeAtBoneOnFrame(em, 0x14, 8, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 4, 0x22);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x2C, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 8, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 0x12, 0);
        break;
    case 0x434:
        Em_PlaySeTypeAtBoneOnFrame(em, 0x10, 0x52, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x66, 0x2B, 0x23);
        break;
    case 0x437:
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x24, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0x22, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x66, 0x22, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xA4, 0x22, 0x23);
        break;
    case 0x447:
        break;
    case 0x448:
        break;
    case 0x44C:
        break;
    case 0x44D:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x65, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0x2A);
        break;
    case 0x44E:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x17, 0x2A);
        break;
    case 0x44F:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x24, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 0x15, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0x15, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0x17, 0x2A);
        break;
    case 0x450:
        Em_PlaySeTypeAtBoneInFrames(em, 0x14, 0x8C, 0x56, 0x23, 6, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x15, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 0x30, 0x2A);
        break;
    case 0x451:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x15, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x30, 0x2A);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0x30, 0x2A);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x1D, 0x22);
        break;
    case 0x452:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0);
        break;
    case 0x453:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0);
        break;
    case 0x454:
        Em_PlaySeTypeAtBoneInFrames(em, 4, 0x7C, 0x56, 0x23, 6, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0x30, 0x2A);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0x22);
        break;
    case 0x457:
        break;
    case 0x458:
        Em_PlaySeTypeAtBoneOnFrame(em, 0x34, 0x51, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 0x18, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x19, 0);
        break;
    case 0x459:
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 0x5B, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 2, 0x16, 0x2A);
        break;
    case 0x45A:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0x2A);
        break;
    case 0x45B:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0x2A);
        break;
    case 0x45C:
        Em_PlaySeAtBoneOnFrame(em, 0x7A, 0x18, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xA2, 0x18, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xC0, 0x18, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x19, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x1A, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x28, 0x23);
        Em_PlaySeTypeAtBoneOnFrame(em, 0x7A, 0x51, 0x23);
        break;
    case 0x45D:
        Em_PlaySeAtBoneOnFrame(em, 0x7A, 0x18, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xA2, 0x18, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xC0, 0x18, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x19, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x1A, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x28, 0x23);
        Em_PlaySeTypeAtBoneOnFrame(em, 0x7A, 0x51, 0x23);
        break;
    case 0x45E:
        Em_PlaySeTypeAtBoneOnFrame(em, 0x26, 0x4C, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0x14);
        break;
    case 0x45F:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x23, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x42, 0x13, 0x22);
        break;
    case 0x460:
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
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x35, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0x35, 0);
        break;
    case 0x463:
        Em_PlaySeAtBoneOnFrame(em, 8, 0x34, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 0x15, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 9, 0);
        break;
    case 0x464:
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0x12, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0x15, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x36, 0, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 4, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0);
        break;
    case 0x466:
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 0x53, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x5A, 0x30, 0x2A);
        break;
    case 0x467:
        Em_PlaySeTypeAtBoneOnFrame(em, 0x1A, 0x54, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 8, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x14, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x2E, 0x23);
        break;
    case 0x468:
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 0x54, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x10, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 9, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x1D, 0);
        break;
    case 0x469:
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 0x52, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x10, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 9, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0x23);
        break;
    case 0x46A:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x1D, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x86, 0x1D, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x86, 0x2F, 0x23);
        break;
    case 0x46B:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x17, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x6E, 0x15, 0x23);
        break;
    case 0x46C:
        break;
    case 0x46D:
        Em_PlaySeAtBoneOnFrame(em, 0xA, 8, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x1F, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x12, 0x14);
        break;
    case 0x46E:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x34, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x30, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0x22);
        break;
    case 0x46F:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x34, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x30, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x16, 0x22);
        break;
    case 0x470:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x12, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 6, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x66, 4, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0xD8, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x108, 3, 0x14);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0x15, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0x25, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x1F, 0x23);
        Em_PlaySeAtBoneOnFrame(em, 0x54, 0x1D, 0);
        break;
    case 0x471:
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 0x57, 0x23);
        Em_PlaySeTypeAtBoneOnFrame(em, 0x122, 0x55, 0x23);
        break;
    case 0x472:
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 0x52, 0x23);
        Em_PlaySeTypeAtBoneOnFrame(em, 0x50, 0x51, 0x23);
        Em_PlaySeTypeAtBoneOnFrame(em, 0x9E, 0x54, 0x23);
        
        break;
    }
}

/* em_laoshan's .data (0x8C8BD0-0x8C8C10) */
s8 EmLaoshan_MinePtBones[4][2] = {
#include "game/008C8BD0.inc"
};
f32 EmLaoshan_MinePtRadius[3] = {
#include "game/008C8BD8.inc"
};
s32 GmEm_ReturnZero(void); /* gm_sub */
void *EmLaoshan_Callbacks[8] = {
    EmLaoshan_SpawnEffect01, EmLaoshan_SpawnEffect01, EmLaoshan_TableNop, EmLaoshan_UpdateMotionEffects,
    GmEm_ReturnZero, EmLaoshan_TableNop, NULL, NULL,
};
