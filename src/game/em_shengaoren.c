/*
 * Monster family: Shen Gaoren (EM_KIND_SHEN_GAOREN). The code from the
 * family's setup function (EmShenGaoren_Setup, called by the per-kind
 * dispatch Em_TypeSetup) to the next family's. The state functions take the
 * monster and the work area its state dispatcher passes on (EmWork); Em.step
 * is the step within the current state. See include/em.h.
 */
#include "common.h"
#include "scene.h"
#include "quest.h"
#include "mine_point.h"
#include "em.h"
#include "stageobj.h"
#include "player.h"
#include "area.h"
#include "qsync.h"
/* prototypes of callees (round 13) */
void GmEm_ResetSize(Em *em);



/* prototypes */
void EmShenGaoren_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void Em_PlaySeAtBoneOnFrame(Em *em, s32 frame, s32 se, s32 bone);
void Em_SetLayerMotion(Em *em, s32 motion, s32 blend, s32 frame, s32 layer);
void Em_SetMotion(Em *em, s32 motion, s32 a, s32 b);
void Player_ClearFlag(void *, u32 flag);
void * EmEffectAttack_Create(Em *, s32, s32, s32, s32);
void Em_PlaySeTypeAtBoneInFrames(Em *em, s32 frame, s32 frame2, s32 se, s32 bone, s32 vol, s32 a);
void Em_PlaySeTypeAtBoneOnFrame(Em *em, s32 frame, s32 se, s32 bone);
void Em_PlaySeAtBoneVolOnFrame(Em *em, s32 frame, s32 se, s32 bone, s32 vol);
void Em_StartTurnToTarget(Em *em, EmTurnParams *t);
void Em_StartTurnAway(Em *em, EmTurnParams *t);
s32 Em_TurnStep(Em *em, EmTurnParams *t);
void Em_RestartScript(Em *em);
void EmShenGaoren_GetMinePointPos(Em *em, f32 *out);
s32 Save_TestEventFlag(s32);
void CamReq_ForMonster(Em *em);
void Em_EndStatus5(Em *em);
u8 Em_UpdateDamage(Em *em, u8 *out);
void Em_UpdatePendingScript(Em *em);
void Em_ClearHitReactMode(Em *em);
s32 MinePoint_Count(s32);
void QSync_SendEm(Em *em, u8 kind, u8 arg);
s32 Quest_IsRunning(s32 ending);
void EmShenGaoren_RunState(Em *em);
void EmShenGaoren_State0_02(Em *);
void EmShenGaoren_State1_00(Em *);
void EmShenGaoren_State1_01(Em *);
void EmShenGaoren_State1_02(Em *);
void EmShenGaoren_State1_03(Em *);
void EmShenGaoren_State3_01(Em *, s32);
void EmShenGaoren_State3_02(Em *);
void EmShenGaoren_State3_03(Em *, s32);
void EmShenGaoren_State5_00_Hunted(Em *);
void EmShenGaoren_State5_01_Hunted(Em *);
void EmShenGaoren_State6_00(Em *);
void EmShenGaoren_State6_01(Em *);
void EmShenGaoren_MotionEffects(Em *, EmWork *);
void EmShenGaoren_MotionSounds(Em *, EmWork *work);
void StageObj_K14_Create(Em *em, s16 k, s16 c, f32 scale);
StageObj *StageObj_K1C_CreateOnEm(Em *em, u8 k, f32 scale);
void Vec_RotateY(f32 *v, f32 angle);
StageObj *StageObj_K1C_CreateAtYaw(f32 *pos, u16 v, u8 k, f32 scale);
s32 Sound_RandomSe(s32 id, s16 lo, s32 alt, s16 n);
s32 EmMotion_PassedFrameScaled(void *p, f32 frame, s32 layer);
s32 Debug_IsOn(s32);
void MinePoint_SetArea(s32 slot, u16 area);
void Effect01_Spawn(void *obj, s16 type);
extern EmTurnParams EmShenGaoren_TurnParams[2];
void Em_SetMotionRow(Em *em, s32 row, s32 arg2, s32 arg3);
void Em_SetStateRand(Em *em, s32 arg1, s32 arg2);
s32 Em_ComputeMaxHp(Em *em, s16 idx);
void EmShenGaoren_AddMinePoint(Em *, s32);
u16 Area_DayId(u16 id);
/* unit's own declaration: with the definition's void (void *obj, s16 bone, f32 *out) (sub_main/unk_3DDBA0.c), EmShenGaoren_SpawnFootEffect: 21 of 29 words differ, EmShenGaoren_SpawnGroundEffect: 18 of 98 words differ */
void Em_BonePos(void *obj, s32 bone, f32 *out);
EmBone *Em_Bone(void *obj, s16 bone);
void Vec_Rotate(f32 *out, f32 *v, f32 *m);
u16 Area_IdForCurrentTime(u16);
void EmEffectAttack_CreateShengaoren(Em *em, s32 arg1, u16 arg2, s32 arg3, s32 arg4);
void Quest_TriggerEvent(Em *em);
void EmImpactAttack_CreateAt(Em *em, f32 *pos, s8 kind);
void CamShake_ReqARange(Em *obj, s32 i, s32 flag);
u8 Player_InCurrentArea(void *obj);
s32 EmMine_Update(Em *em);
s8 EmMine_Create(Em *em, s32 idx);
void QuestEm_Hunted(Em *em);
void CamReq_BossHunted(void);
void EmShenGaoren_SpawnAreaImpactEffects(Em *em, s32 a);
void Em_MotionDelta(Em *em, f32 *out);
s32 EmMotion_ReachedFrame(void *obj, s32 a, f32 frame);
extern f32 D_008CA0C0[][4];
void Effect0D_SpawnOnGround(f32 *pos, s32 a, f32 scale);
void ShenGaorenAttack_Create(Em *em, s8 kind);
s32 Em_PlaySeChecked(Em *em, s32 id, s32 a, void *pos, s32 b, s32 c);
void Effect02_SpawnForOwner(void *owner, s32 ang, s32 type, s32 sub, f32 *pos, f32 scale);
void Effect0D_SpawnIfHere(void *o, f32 *pos, s32 type, f32 scale);
f32 Vec_DistanceXZ(f32 *a, f32 *b);
u16 Math_AngleXZ(f32 *from, f32 *to);
/* end of prototypes */

void EmShenGaoren_Setup(Em *em)
{
    f32 pos[4];
    EmShenGaorenWork *w = (EmShenGaorenWork *)&em->work;

    w->unk000 = 0;
    if (w->mineSlot != -1) {
        MinePoint_SetArea(w->mineSlot, em->area);
        EmShenGaoren_GetMinePointPos(em, pos);
        MinePoint_SetPos(w->mineSlot, pos);
    }
}

void EmShenGaoren_SpawnEffect01(u8 *em)
{
    Effect01_Spawn((Player *) em, 0x3);
}

void EmShenGaoren_GetMinePointPos(Em *em, f32 *out)
{
    f32 ofs[3];
    f32 b[3];
    f32 a[3];

    ofs[1] = -50.0f;
    ofs[2] = -150.0f;
    ofs[0] = 0.0f;
    Em_BonePos(em, 0xE, a);
    Vec_Rotate((f32 *) b, (f32 *) ofs, (f32 *) (Em_Bone(em, 0xE)));
    out[0] = a[0] + b[0];
    out[1] = a[1] + b[1];
    out[2] = a[2] + b[2];
}

void EmShenGaoren_AddMinePoint(Em *em, s32 unused)
{
    EmShenGaorenWork *w = (EmShenGaorenWork *)&em->work;
    MinePoint pt;
    f32 pos[3];

    if (w->mineCount <= 0) {
        w->mineSlot = -1;
    }
    pt.radius = 460.0f;
    switch (gQuest.header->tier) {
    default:
        pt.table = 0x60;
        break;
    case 1:
        pt.table = 0xDC;
        break;
    case 2:
        pt.table = 0x159;
        break;
    case 3:
        pt.table = 0x159;
        break;
    }
    pt.action = MINE_ACTION_CARVE;
    pt.count = w->mineCount;
    pt.area = (u8) em->area;
    pt.flags = 1;
    pt.timer = 0;
    EmShenGaoren_GetMinePointPos(em, pos);
    w->mineSlot = MinePoint_AddUntimed(&pt, pos, 0);
}

void EmShenGaoren_RemoveMinePoints(Em *em)
{
    EmShenGaorenWork *w = (EmShenGaorenWork *)&em->work;
    s32 i;
    s8 *p;

    for (i = 0; i < 3; i++) {
        p = &w->partFlinch[i];
        if (*p != -1) {
            MinePoint_Remove(*p);
            *p = -1;
        }
    }
}

void EmShenGaoren_Init(Em *em)
{
    EmShenGaorenWork *w = (EmShenGaorenWork *)&em->work;

    s16 size;
    s32 dayId;

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
    em->blind = 0;
    em->unk_InitFlag = 1;
    em->combat = 1;
    em->combatPlayers = 0xF;
    if ((Area_DayId(em->area) & 0xFFFF) == AREA_BATTLE_TOWN_1) {
        em->stance = 0;
        Em_SetMotionRow(em, 0x1E, 0, 0);
        Em_SetStateRand(em, 0, 2);
    } else {
        em->stance = 1;
        Em_SetMotionRow(em, 1, 0, 0);
        Em_SetStateRand(em, 0, 1);
    }
    w->unk00C = 1500.0f;
    w->unk022 = 0;
    w->unk000 = 0;
    w->syncTimer = 0x96;
    w->ease[0] = 1.0f;
    w->ease[1] = 1.0f;
    w->ease[2] = 1.0f;
    w->ease[3] = 1.0f;
    w->mineCount = 3;
    EmShenGaoren_AddMinePoint(em, 0x96);
}

void EmShenGaoren_EndState(Em *em)
{
    em->motionSpeed = 1.0f;
    em->lookAtTarget = 0;
    em->scriptPending = 1;
    if (em->stance == 0) {
        em->stance = 0;
        EmShenGaoren_ChangeState(em, 0, 2, 0);
        return;
    }
    em->stance = 1;
    EmShenGaoren_ChangeState(em, 0, 1, 0);
}

void EmShenGaoren_State0_00(Em *em)
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

void EmShenGaoren_State0_01(Em *em)
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
            EmShenGaoren_EndState(em);
        }
        break;
    }
}

void EmShenGaoren_State0_02(Em *em)
{
    s32 m;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 0;
        m = em->motionId[0];
        if (m == 0x413 || m == 0x414 || m == 0x415) {
            Em_SetMotion(em, 0x1E, 0xC, 0);
        } else {
            Em_SetMotion(em, 0x1E, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmShenGaoren_EndState(em);
        }
        break;
    }
}

void EmShenGaoren_State0_03(Em *em)
{
    em->guardTimer = 5;
    em->noReact = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 0;
        Em_SetMotion(em, 6, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->guardTimer = 0;
            em->noReact = 0;
            EmShenGaoren_EndState(em);
        }
        break;
    }
}

void EmShenGaoren_State0_04(Em *em)
{
    em->guardTimer = 5;
    em->noReact = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 1;
        Em_SetMotion(em, 0x23, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->guardTimer = 0;
            em->noReact = 0;
            EmShenGaoren_EndState(em);
        }
        break;
    }
}

void EmShenGaoren_State0_05(Em *em)
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
            EmShenGaoren_EndState(em);
        }
        break;
    }
}

void EmShenGaoren_State0_06(Em *em)
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
            EmShenGaoren_EndState(em);
        }
        break;
    }
}

void EmShenGaoren_State0_07(Em *em)
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
            EmShenGaoren_EndState(em);
        }
        break;
    }
}

void EmShenGaoren_RunGroup0(Em *em)
{
    switch (em->state) {
    case 0:
        EmShenGaoren_State0_00(em);
        break;
    case 1:
        EmShenGaoren_State0_01(em);
        break;
    case 2:
        EmShenGaoren_State0_02(em);
        break;
    case 3:
        EmShenGaoren_State0_03(em);
        break;
    case 4:
        EmShenGaoren_State0_04(em);
        break;
    case 5:
        EmShenGaoren_State0_05(em);
        break;
    case 6:
        EmShenGaoren_State0_06(em);
        break;
    case 7:
        EmShenGaoren_State0_07(em);
        
    default:
        break;
    }
}

void EmShenGaoren_State1_00(Em *em)
{
    f32 pos[3];
    s32 d;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 1;
        Em_SetMotion(em, 2, 0, 0);
        em->speedBonus = 0x1E;
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
            EmShenGaoren_EndState(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmShenGaoren_EndState(em);
        }
        break;
    }
}

void EmShenGaoren_State1_01(Em *em)
{
    f32 pos[3];
    s32 d;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 1;
        Em_SetMotion(em, 3, 0, 0);
        em->speedBonus = 0x1E;
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
            EmShenGaoren_EndState(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmShenGaoren_EndState(em);
        }
        break;
    }
}

void EmShenGaoren_State1_02(Em *em)
{
    f32 pos[3];
    s32 d;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 0;
        Em_SetMotion(em, 0x1F, 0, 0);
        em->speedBonus = 0x1E;
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
            EmShenGaoren_EndState(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmShenGaoren_EndState(em);
        }
        break;
    }
}

void EmShenGaoren_State1_03(Em *em)
{
    f32 pos[3];
    s32 d;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 0;
        Em_SetMotion(em, 0x20, 0, 0);
        em->speedBonus = 0x1E;
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
            EmShenGaoren_EndState(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmShenGaoren_EndState(em);
        }
        break;
    }
}

void EmShenGaoren_State1_04(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_StartTurnToTarget(em, &EmShenGaoren_TurnParams[1]);
        em->speedBonus = 0x1E;
        break;
    case 1:
        if (Em_TurnStep(em, &EmShenGaoren_TurnParams[1]) == 1) {
            Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
            EmShenGaoren_EndState(em);
        }
        break;
    }
}

void EmShenGaoren_State1_05(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 1;
        Em_StartTurnToTarget(em, &EmShenGaoren_TurnParams[0]);
        em->speedBonus = 0x1E;
        break;
    case 1:
        if (Em_TurnStep(em, &EmShenGaoren_TurnParams[0]) == 1) {
            Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
            EmShenGaoren_EndState(em);
        }
        break;
    }
}

void EmShenGaoren_State1_06(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 1;
        Em_StartTurnAway(em, &EmShenGaoren_TurnParams[0]);
        em->speedBonus = 0x1E;
        break;
    case 1:
        if (Em_TurnStep(em, &EmShenGaoren_TurnParams[0]) == 1) {
            Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
            EmShenGaoren_EndState(em);
        }
        break;
    }
}

void EmShenGaoren_State1_07(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_StartTurnAway(em, &EmShenGaoren_TurnParams[1]);
        em->speedBonus = 0x1E;
        break;
    case 1:
        if (Em_TurnStep(em, &EmShenGaoren_TurnParams[1]) == 1) {
            Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
            EmShenGaoren_EndState(em);
        }
        break;
    }
}

void EmShenGaoren_RunGroup1(Em *em)
{
    switch (em->state) {
    case 0:
        EmShenGaoren_State1_00(em);
        break;
    case 1:
        EmShenGaoren_State1_01(em);
        break;
    case 2:
        EmShenGaoren_State1_02(em);
        break;
    case 3:
        EmShenGaoren_State1_03(em);
        break;
    case 4:
        EmShenGaoren_State1_04(em);
        break;
    case 5:
        EmShenGaoren_State1_05(em);
        break;
    case 6:
        EmShenGaoren_State1_06(em);
        break;
    case 7:
        EmShenGaoren_State1_07(em);
        
    default:
        break;
    }
}

void EmShenGaoren_RunGroup2(void)
{
}

void EmShenGaoren_State3_00(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 1;
        em->timer = 0x12C;
        Em_SetMotion(em, 0xD, 0, 0);
        break;
    case 1:
        if (--em->timer < 0) {
            em->step++;
            em->timer = 1;
            Em_SetMotion(em, 0xE, 0, 0);
        }
        break;
    case 2:
        if (EmMotion_PassedFrameScaled(em, 4.0f, 0U) != 0) {
            EmEffectAttack_Create(em, 3, 0, 0xF800, 0x8000);
        }
        if (em->motion[0].playing == 0) {
            if (--em->timer <= 0) {
                em->step++;
                Em_SetMotion(em, 0xF, 0, 0);
                break;
            }
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmShenGaoren_EndState(em);
        }
        break;
    }
}

void EmShenGaoren_State3_01(Em *em, s32 variant)
{
    EmShenGaorenWork *w = (EmShenGaorenWork *)&em->work;

    s8 n;
    u16 area;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 0;
        em->timer = 0x12C;
        Em_SetMotion(em, 0x2B, 0, 0);
        break;
    case 1:
        if (--em->timer < 0) {
            em->step++;
            em->timer = 1;
            Em_SetMotion(em, 0x2C, 0, 0);
        }
        break;
    case 2:
        if (EmMotion_PassedFrameScaled(em, 4.0f, 0U) != 0) {
            switch (variant) {
            case 0:
                area = em->area;
                if (area != (Area_IdForCurrentTime(AREA_FORTRESS_5) & 0xFFFF)) {
                    EmEffectAttack_Create(em, 5, 0, 0xE800, 0x8000);
                    EmEffectAttack_CreateShengaoren(em, 8, AREA_FORTRESS_5, 0x6000, 0);
                } else {
                    EmEffectAttack_Create(em, 3, 0, 0xF800, 0x8000);
                }
                if (em->remote == 0) {
                    n = w->fortHits;
                    if (n < 0xA) {
                        w->fortHits = n + 1;
                        if (w->fortHits >= 0xA) {
                            Quest_TriggerEvent(em);
                        }
                        gScene.fortStage = w->fortHits;
                    }
                }
                break;
            case 1:
                EmEffectAttack_Create(em, 5, 0, 0xE800, 0x8000);
                EmEffectAttack_CreateShengaoren(em, 8, AREA_BATTLE_TOWN_2, 0x6000, 0);
                break;
            case 2:
                EmEffectAttack_Create(em, 5, 0, 0xE800, 0x8000);
                EmEffectAttack_CreateShengaoren(em, 8, AREA_BATTLE_TOWN_3, 0x6000, 0);
                break;
            }
        }
        if (em->motion[0].playing == 0) {
            if (--em->timer <= 0) {
                em->step++;
                Em_SetMotion(em, 0x2D, 0, 0);
                break;
            }
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmShenGaoren_EndState(em);
        }
        break;
    }
}

void EmShenGaoren_State3_02(Em *em)
{
    EmShenGaorenWork *w = (EmShenGaorenWork *)&em->work;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 0;
        Em_SetMotion(em, 0x29, 0, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 200.0f, 0)) {
            if (em->remote == 0 && w->fortHits <= 9) {
                w->fortHits++;
                if (w->fortHits >= 10) {
                    Quest_TriggerEvent(em);
                }
                gScene.fortStage = w->fortHits;
            }
            EmShenGaoren_SpawnAreaImpactEffects(em, 1);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmShenGaoren_EndState(em);
        }
        break;
    }
}

void EmShenGaoren_State3_03(Em *em, s32 variant)
{
    EmShenGaorenWork *w = (EmShenGaorenWork *)&em->work;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 1;
        Em_SetMotion(em, 0x10, 0, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 134.0f, 0)) {
            if (variant != 0) {
                if (em->remote == 0 && w->fortHits <= 9) {
                    w->fortHits++;
                    if (w->fortHits >= 10) {
                        Quest_TriggerEvent(em);
                    }
                    gScene.fortStage = w->fortHits;
                }
                EmShenGaoren_SpawnAreaImpactEffects(em, 0);
            } else {
                if (em->remote == 0 && (Area_DayId(em->area) & 0xFFFF) == AREA_FORTRESS_3 && w->unk000 <= 7) {
                    w->unk000 = 8;
                }
                gScene.emEventFlags |= 1;
                EmShenGaoren_SpawnAreaImpactEffects(em, 0);
            }
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmShenGaoren_EndState(em);
        }
        break;
    }
}

void EmShenGaoren_State3_04(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 1;
        Em_SetMotion(em, 0xA, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmShenGaoren_EndState(em);
        }
        break;
    }
}

void EmShenGaoren_State3_05(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 1;
        Em_SetMotion(em, 0xB, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmShenGaoren_EndState(em);
        }
        break;
    }
}

void EmShenGaoren_State3_06(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 1;
        Em_SetMotion(em, 0xC, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmShenGaoren_EndState(em);
        }
        break;
    }
}

void EmShenGaoren_State3_07(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 0;
        Em_SetMotion(em, 0x2A, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmShenGaoren_EndState(em);
        }
        break;
    }
}

void EmShenGaoren_RunGroup3(Em *em)
{
    switch (em->state) {
    case 0:
        EmShenGaoren_State3_00(em);
        break;
    case 1:
        EmShenGaoren_State3_01(em, 0);
        break;
    case 2:
        EmShenGaoren_State3_02(em);
        break;
    case 3:
        EmShenGaoren_State3_03(em, 0);
        break;
    case 4:
        EmShenGaoren_State3_04(em);
        break;
    case 5:
        EmShenGaoren_State3_05(em);
        break;
    case 6:
        EmShenGaoren_State3_06(em);
        break;
    case 7:
        EmShenGaoren_State3_07(em);
        break;
    case 8:
        EmShenGaoren_State3_03(em, 1);
        break;
    case 9:
        EmShenGaoren_State3_01(em, 1);
        break;
    case 10:
        EmShenGaoren_State3_01(em, 2);
        
    default:
        break;
    }
}

void EmShenGaoren_State4_00(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 1;
        Em_SetMotion(em, 0x15, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmShenGaoren_EndState(em);
        }
        break;
    }
}

void EmShenGaoren_State4_01(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 0;
        Em_SetMotion(em, 0x33, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmShenGaoren_EndState(em);
        }
        break;
    }
}

void EmShenGaoren_State4_02(Em *em, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 0;
        Em_SetMotion(em, variant + 0x37, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmShenGaoren_EndState(em);
        }
        break;
    }
}

void EmShenGaoren_State4_06(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 0;
        Em_SetMotion(em, 0x34, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 0x12C;
            em->stance = 1;
            Em_SetMotion(em, 0x35, 0, 0);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            em->stance = 1;
            Em_SetMotion(em, 0x36, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmShenGaoren_EndState(em);
        }
        break;
    }
}

void EmShenGaoren_State4_07(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 1;
        Em_SetMotion(em, 0x16, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 0x12C;
            Em_SetMotion(em, 0x17, 0, 0);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, 0x18, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmShenGaoren_EndState(em);
        }
        break;
    }
}

void EmShenGaoren_State4_08(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = 0;
        Em_SetMotion(em, 0x31, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stance = 1;
            Em_SetMotion(em, 0x34, 0, 0x52);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmShenGaoren_EndState(em);
        }
        break;
    }
}

void EmShenGaoren_RunGroup4(Em *em)
{
    switch (em->state) {
    case 0:
        EmShenGaoren_State4_00(em);
        break;
    case 1:
        EmShenGaoren_State4_01(em);
        break;
    case 2:
        EmShenGaoren_State4_02(em, 1);
        break;
    case 3:
        EmShenGaoren_State4_02(em, 3);
        break;
    case 4:
        EmShenGaoren_State4_02(em, 0);
        break;
    case 5:
        EmShenGaoren_State4_02(em, 2);
        break;
    case 6:
        EmShenGaoren_State4_06(em);
        break;
    case 7:
        EmShenGaoren_State4_07(em);
        break;
    case 8:
        EmShenGaoren_State4_08(em);
        break;
    case 9:
        EmShenGaoren_State4_00(em);
        break;
    case 10:
        EmShenGaoren_State4_00(em);
        break;
    case 11:
        EmShenGaoren_State4_00(em);
        break;
    case 12:
        EmShenGaoren_State4_00(em);
        break;
    case 13:
        EmShenGaoren_State4_07(em);
        break;
    case 14:
        EmShenGaoren_State4_07(em);
        break;
    case 15:
        EmShenGaoren_State4_07(em);
        break;
    case 16:
        EmShenGaoren_State4_07(em);
        break;
    case 17:
        EmShenGaoren_State4_02(em, 1);
        break;
    case 18:
        EmShenGaoren_State4_02(em, 3);
        break;
    case 19:
        EmShenGaoren_State4_02(em, 0);
        break;
    case 20:
        EmShenGaoren_State4_02(em, 2);
        break;
    case 21:
        EmShenGaoren_State4_06(em);
        
    default:
        break;
    }
}

void EmShenGaoren_State5_00_Hunted(Em *em)
{
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, 0x19, 0, 0);
        em->timer = 0;
        em->lookAtTarget = 0;
        em->stance = 1;
        QuestEm_Hunted(em);
        CamReq_BossHunted();
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, 0x3B, 0, 0xC8);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stance = 3;
            EmMine_Create(em, 0);
            em->fieldCollide = 0;
        }
        break;
    case 3:
        EmMine_Update(em);
        break;
    case 0x63:
        em->step++;
        EmShenGaoren_RemoveMinePoints(em);
        em->hp = 0x3415;
        EmShenGaoren_EndState(em);
        break;
    }
}

void EmShenGaoren_State5_01_Hunted(Em *em)
{
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 0;
        Em_SetMotion(em, 0x3B, 0, 0);
        QuestEm_Hunted(em);
        CamReq_BossHunted();
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stance = 3;
            EmMine_Create(em, 0);
            em->fieldCollide = 0;
        }
        break;
    case 2:
        EmMine_Update(em);
        break;
    case 0x63:
        em->step++;
        em->hp = 0x3415;
        EmShenGaoren_RemoveMinePoints(em);
        EmShenGaoren_EndState(em);
        break;
    }
}

void EmShenGaoren_RunGroup5(Em *em)
{
    switch (em->state) {
    case 0:
        EmShenGaoren_State5_00_Hunted(em);
        break;
    case 1:
        EmShenGaoren_State5_01_Hunted(em);
        break;
    }
}

void EmShenGaoren_State6_00(Em *em)
{
    f32 pos[3];

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 1;
        Em_SetMotion(em, 3, 0, 0);
        em->timer = 150;
        GmEm_ResetSize(em);
        em->posX = 5875.0f;
        em->posY = -266.0f;
        em->posZ = 10253.0f;
        em->goal[0] = 7127.0f;
        em->goal[1] = -266.0f;
        em->goal[2] = 10005.0f;
        em->rotY = (u16) (Math_AngleXZ(&em->posX, em->goal) + 0x8000);
        em->goalDist = Vec_DistanceXZ(&em->posX, em->goal);
        em->scriptPending = 0;
        break;
    case 1:
        Em_MotionDelta(em, pos);
        if ((em->goalDist += pos[2]) <= 0.0f) {
            em->step++;
            em->posX = 9907.0f;
            em->posY = -266.0f;
            em->posZ = 9164.0f;
            em->goal[0] = 12961.0f;
            em->goal[1] = -266.0f;
            em->goal[2] = 10152.0f;
            em->rotY = (u16) (Math_AngleXZ(&em->posX, em->goal) + 0x8000);
            em->goalDist = Vec_DistanceXZ(&em->posX, em->goal);
        }
        break;
    case 2:
        Em_MotionDelta(em, pos);
        if ((em->goalDist += pos[2]) <= 0.0f) {
            em->step++;
            em->goal[0] = 11711.0f;
            em->goal[1] = -266.0f;
            em->goal[2] = 12920.0f;
            Em_StartTurnAway(em, &EmShenGaoren_TurnParams[0]);
        }
        break;
    case 3:
        if (Em_TurnStep(em, &EmShenGaoren_TurnParams[0]) == 1) {
            em->step++;
            Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
            em->timer = 300;
            Em_SetMotion(em, 0xD, 0, 0);
        }
        break;
    case 4:
        if (--em->timer < 0) {
            em->step++;
            Em_SetMotion(em, 0xE, 0, 0);
        }
        break;
    case 5:
        if (EmMotion_PassedFrameScaled(em, 4.0f, 0)) {
            EmEffectAttack_Create(em, 3, 0, 0xE800, 0x8000);
        }
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
        if (Save_TestEventFlag(0xAE) == 1) {
            em->step++;
            EmShenGaoren_EndState(em);
        }
        break;
    }
}

void EmShenGaoren_State6_01(Em *em)
{
    f32 pos[3];
    s32 d;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = 1;
        Em_SetMotion(em, 1, 0, 0);
        em->posX = 7248.0f;
        em->posY = -2.0f;
        em->posZ = 14498.0f;
        em->goal[0] = 5481.0f;
        em->goal[1] = -2.0f;
        em->goal[2] = 16020.0f;
        em->rotY = 0x7000;
        Em_StartTurnToTarget(em, &EmShenGaoren_TurnParams[0]);
        break;
    case 1:
        if (Em_TurnStep(em, &EmShenGaoren_TurnParams[0]) == 1) {
            em->step++;
            Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
            em->timer = 300;
            Em_SetMotion(em, 2, 0, 0);
        }
        break;
    case 2:
        Em_MotionDelta(em, pos);
        if ((em->goalDist -= pos[2]) <= 0.0f) {
            em->goal[0] = 3264.0f;
            em->goal[1] = -2.0f;
            em->goal[2] = 19269.0f;
            em->step++;
            em->goalDist = Vec_DistanceXZ(&em->posX, em->goal);
        }
        break;
    case 3:
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
    case 4:
        break;
    }
}

void EmShenGaoren_RunGroup6(Em *em)
{
    switch (em->state) {
    case 0:
        EmShenGaoren_State6_00(em);
        break;
    case 1:
        EmShenGaoren_State6_01(em);
        break;
    }
}

void EmShenGaoren_Update(Em *em)
{
    EmShenGaorenWork *w = (EmShenGaorenWork *)&em->work;
    f32 pos[3];
    u8 buf[4];
    s32 flag;
    s32 i;
    s32 k;

    if (w->mineSlot != -1) {
        EmShenGaoren_GetMinePointPos(em, pos);
        MinePoint_SetPos(w->mineSlot, pos);
        if ((w->mineCount = MinePoint_Count(w->mineSlot)) <= 0) {
            MinePoint_Remove(w->mineSlot);
            w->mineSlot = -1;
        }
    }
    if (*(s8 *) &em->quakeTimer != 0) {
        if (--*(s8 *) &em->quakeTimer <= 0) {
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
        EmShenGaoren_ChangeState(em, 5, 0, 3);
        break;
    case 3:
        if (em->stance == 1) {
            if (em->stateGroup != 4 || em->state != 7) {
                EmShenGaoren_ChangeState(em, 4, 7, 3);
            }
        } else {
            if (em->stateGroup != 4 || em->state != 0x15) {
                EmShenGaoren_ChangeState(em, 4, 0x15, 3);
            }
        }
        break;
    case 17:
        Em_EndStatus5(em);
        break;
    case 12:
        flag = 1;
        if (em->stance == 1) {
            k = em->flinchPart;
            switch (k) {
            case 0:
            case 1:
                EmShenGaoren_ChangeState(em, 4, 7, 3);
                break;
            case 3:
            case 4:
            case 5:
            case 6:
                if (w->partFlinch[k] > 0) {
                    EmShenGaoren_ChangeState(em, 4, k + 10, 3);
                } else {
                    EmShenGaoren_ChangeState(em, 4, k + 6, 3);
                }
                break;
            case 2:
            case 7:
                if (em->parts[k].state >= 2) {
                    EmShenGaoren_ChangeState(em, 4, 7, 3);
                } else {
                    EmShenGaoren_ChangeState(em, 4, 0, 3);
                }
                break;
            }
        } else {
            for (i = 0; i < 4; i++) {
                if (w->partFlinch[3 + i] <= 1) {
                    flag = 0;
                }
            }
            if (flag == 1) {
                if (em->stateGroup == 3 && em->state == 1) {
                    EmShenGaoren_ChangeState(em, 4, 8, 3);
                } else {
                    EmShenGaoren_ChangeState(em, 4, 6, 3);
                }
            } else {
                k = em->flinchPart;
                switch (k) {
                case 0:
                case 1:
                case 2:
                case 7:
                    EmShenGaoren_ChangeState(em, 4, 1, 3);
                    break;
                case 3:
                case 4:
                case 5:
                case 6:
                    if (w->partFlinch[k] > 0) {
                        EmShenGaoren_ChangeState(em, 4, k + 14, 3);
                    } else {
                        EmShenGaoren_ChangeState(em, 4, k - 1, 3);
                    }
                    break;
                }
            }
        }
        break;
    }
    if (em->parts[2].state >= 4) {
        em->fxFlags |= 1;
    }
    if (em->parts[2].state >= 2) {
        em->fxFlags |= 8;
    }
    switch (Area_DayId(em->area)) {
    case AREA_FORTRESS_1:
        if (Save_TestEventFlag(0xAE) == 0) {
            if (Area_DayId(gScene.area) == AREA_FORTRESS_1 && gScene.eventRunning == 1 && em->stateGroup != 6) {
                EmShenGaoren_ChangeState(em, 6, 0, 0);
            }
            goto skip;
        }
        break;
    case AREA_FORTRESS_5:
        if (Quest_IsRunning(0) == 1 && em->stateGroup != 6 && em->stateGroup != 5) {
            em->scriptPending = 0;
            if (gScene.area == em->area) {
                CamReq_ForMonster(em);
            }
            EmShenGaoren_ChangeState(em, 6, 1, 0);
        }
        break;
    }
    Em_UpdatePendingScript(em);
skip:
    Em_ClearHitReactMode(em);
    EmShenGaoren_RunState(em);
    if (em->stateRequest != 0) {
        EmShenGaoren_RunState(em);
        em->stateRequest = 0;
    }
    if (em->stateGroup == 5 || (em->stateGroup == 3 && (em->state == 0 || em->state == 1))) {
        em->quakeTimer = 0;
    } else {
        em->quakeTimer = 5;
    }
}

void EmShenGaoren_RunState(Em *em)
{
    switch (em->stateGroup) {
    case 0:
        EmShenGaoren_RunGroup0(em);
        break;
    case 1:
        EmShenGaoren_RunGroup1(em);
        break;
    case 2:
        EmShenGaoren_RunGroup2();
        break;
    case 3:
        EmShenGaoren_RunGroup3(em);
        break;
    case 4:
        EmShenGaoren_RunGroup4(em);
        break;
    case 5:
        EmShenGaoren_RunGroup5(em);
        break;
    case 6:
        EmShenGaoren_RunGroup6(em);
        break;
    case 7:
        EmShenGaoren_RunGroup6(em);
        
    default:
        break;
    }
}

void func_008261F0(Em *em, EmWork *work)
{
    f32 *v;
    EmShenGaorenWork *w = (EmShenGaorenWork *)work;
    s32 k = 0;

    switch (w->fxMotion) {
    case 0x3FE:
        if (EmMotion_ReachedFrame(em, 0, 86.0f)) {
            k = 1;
        }
        break;
    case 0x3FF:
        k = 1;
        break;
    case 0x400:
        if (!EmMotion_ReachedFrame(em, 0, 94.0f)) {
            k = 1;
        }
        break;
    case 0x41C:
        if (EmMotion_ReachedFrame(em, 0, 134.0f)) {
            k = 1;
        }
        break;
    case 0x41D:
        k = 1;
        break;
    case 0x41E:
        if (!EmMotion_ReachedFrame(em, 0, 94.0f)) {
            k = 1;
        }
        break;
    }
    v = D_008CA0C0[k];
    w->ease[0] = w->ease[0] + (v[0] - w->ease[0]) / 120.0f;
    w->ease[1] = w->ease[1] + (v[1] - w->ease[1]) / 120.0f;
    w->ease[2] = w->ease[2] + (v[2] - w->ease[2]) / 120.0f;
    w->ease[3] = w->ease[3] + (v[3] - w->ease[3]) / 120.0f;
}

void EmShenGaoren_SpawnGroundEffect(Em *em, s32 bone, s32 kind, s32 flags, f32 scale)
{
    f32 pos[3];

    if (Player_InCurrentArea(em)) {
        Em_BonePos(em, bone, pos);
        pos[1] = em->floorY;
        Effect0D_SpawnOnGround(pos, kind, scale * em->scaleX);
        if (flags & 2) {
            CamShake_ReqARange(em, 2, 2);
        }
        if (flags & 1) {
            EmImpactAttack_CreateAt(em, (f32 *) pos, 9);
            switch ((s16) bone) {
            case 0x1A:
                ShenGaorenAttack_Create(em, 0x12);
                break;
            case 0x16:
                ShenGaorenAttack_Create(em, 0x13);
                break;
            case 0x12:
                ShenGaorenAttack_Create(em, 0x14);
                break;
            case 0x1E:
                ShenGaorenAttack_Create(em, 0x15);
                break;
            case 6:
                ShenGaorenAttack_Create(em, 0x16);
                break;
            case 0xA:
                ShenGaorenAttack_Create(em, 0x17);
                break;
            }
        }
    }
}

void EmShenGaoren_SpawnFootEffect(Em *em, s32 bone)
{
    f32 pos[3];

    if (Player_InCurrentArea(em)) {
        Em_BonePos(em, bone, pos);
        pos[1] = em->floorY;
        EmImpactAttack_CreateAt(em, (f32 *) pos, 9);
        CamShake_ReqARange(em, 2, 2);
    }
}

static void EmShenGaoren_MotionEffectsDefault(Em *em)
{
}

void EmShenGaoren_MotionEffects(Em *em, EmWork *work)
{
    f32 pos[3];
    f32 v[3];
    EmShenGaorenWork *sw = (EmShenGaorenWork *)work;

    if (em->motionId[0] != sw->fxMotion) {
        sw->unk020 = 0;
        sw->fxMotion = em->motionId[0];
    }
    EmShenGaoren_MotionSounds(em, work);
    func_008261F0(em, work);
    switch (sw->fxMotion) {
    case 0x3EA:
    case 0x407:
        if (EmMotion_PassedFrameScaled(em, 46.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0x1A, 7, 3, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 148.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0x16, 7, 3, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 256.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0x12, 7, 3, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 358.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0x1E, 7, 3, 2.0f);
            break;
        }
        break;
    case 0x3EB:
    case 0x408:
        if (EmMotion_PassedFrameScaled(em, 94.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0x1E, 7, 3, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 196.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0x12, 7, 3, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 304.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0x16, 7, 3, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 406.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0x1A, 7, 3, 2.0f);
            break;
        }
        break;
    case 0x3EC:
    case 0x409:
        if (EmMotion_PassedFrameScaled(em, 38.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0x1A, 7, 3, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 142.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0x16, 7, 3, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 248.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0x12, 7, 3, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 352.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0x1E, 7, 3, 2.0f);
            break;
        }
        break;
    case 0x3ED:
    case 0x40A:
        if (EmMotion_PassedFrameScaled(em, 38.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0x12, 7, 3, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 142.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0x1E, 7, 3, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 248.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0x1A, 7, 3, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 352.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0x16, 7, 3, 2.0f);
            break;
        }
        break;
    case 0x3EE:
        if (EmMotion_PassedFrameScaled(em, 68.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0x1A, 7, 1, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 136.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0x12, 7, 1, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 210.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0x1E, 7, 1, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 278.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0x16, 7, 1, 2.0f);
            break;
        }
        break;
    case 0x3F2:
        if (EmMotion_PassedFrameScaled(em, 124.0f, 0) != 0) {
            ShenGaorenAttack_Create(em, 3);
        }
        if (EmMotion_PassedFrameScaled(em, 58.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0x1A, 7, 0, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 160.0f, 0) != 0) {
            StageObj_K14_Create((Em *) em, 0xB, 0x26, em->scaleX);
            break;
        }
        break;
    case 0x3F3:
        if (EmMotion_PassedFrameScaled(em, 124.0f, 0) != 0) {
            ShenGaorenAttack_Create(em, 4);
        }
        if (EmMotion_PassedFrameScaled(em, 58.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0x12, 7, 0, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 160.0f, 0) != 0) {
            StageObj_K14_Create((Em *) em, 0xB, 0x27, em->scaleX);
            break;
        }
        break;
    case 0x3F4:
        if (EmMotion_PassedFrameScaled(em, 48.0f, 0) != 0) {
            ShenGaorenAttack_Create(em, 5);
        }
        if (EmMotion_PassedFrameScaled(em, 82.0f, 0) != 0) {
            ShenGaorenAttack_Create(em, 6);
        }
        if (EmMotion_PassedFrameScaled(em, 110.0f, 0) != 0) {
            ShenGaorenAttack_Create(em, 7);
        }
        if (EmMotion_PassedFrameScaled(em, 160.0f, 0) != 0) {
            ShenGaorenAttack_Create(em, 8);
        }
        if (EmMotion_PassedFrameScaled(em, 190.0f, 0) != 0) {
            ShenGaorenAttack_Create(em, 9);
        }
        if (EmMotion_PassedFrameScaled(em, 240.0f, 0) != 0) {
            ShenGaorenAttack_Create(em, 0xA);
        }
        if (EmMotion_PassedFrameScaled(em, 334.0f, 0) != 0) {
            ShenGaorenAttack_Create(em, 0xB);
        }
        if ((EmMotion_PassedFrameScaled(em, 64.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 206.0f, 0) != 0)) {
            EmShenGaoren_SpawnGroundEffect(em, 6, 7, 0, 2.0f);
            EmShenGaoren_SpawnFootEffect(em, 6);
        }
        if (EmMotion_PassedFrameScaled(em, 96.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0x1A, 7, 0, 2.0f);
            EmShenGaoren_SpawnFootEffect(em, 0x1A);
        }
        if (EmMotion_PassedFrameScaled(em, 126.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0xA, 7, 0, 2.0f);
            EmShenGaoren_SpawnFootEffect(em, 0xA);
        }
        if (EmMotion_PassedFrameScaled(em, 176.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0x16, 7, 0, 2.0f);
            EmShenGaoren_SpawnFootEffect(em, 0x16);
        }
        if (EmMotion_PassedFrameScaled(em, 256.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0x12, 7, 0, 2.0f);
            EmShenGaoren_SpawnFootEffect(em, 0x12);
        }
        if (EmMotion_PassedFrameScaled(em, 346.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0x1E, 7, 0, 2.0f);
            EmShenGaoren_SpawnFootEffect(em, 0x1E);
            break;
        }
        break;
    case 0x3F5:
        if (EmMotion_PassedFrameScaled(em, 88.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0xA, 7, 0, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 140.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0x16, 7, 0, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 158.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 6, 7, 0, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 240.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0x1E, 7, 0, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 380.0f, 0) != 0) {
            StageObj_K1C_CreateOnEm(em, 0x19, 1.0f);
            break;
        }
        break;
    case 0x3F6:
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            StageObj_K1C_CreateOnEm(em, 0x1A, 1.0f);
            break;
        }
        break;
    case 0x3F7:
        if (EmMotion_PassedFrameScaled(em, 170.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0x1E, 7, 0, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 198.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 6, 4, 0, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 270.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0x16, 7, 0, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 292.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0xA, 4, 0, 2.0f);
            break;
        }
        break;
    case 0x3F8:
        if (EmMotion_PassedFrameScaled(em, 120.0f, 0) != 0) {
            ShenGaorenAttack_Create(em, 2);
        }
        if (EmMotion_PassedFrameScaled(em, 144.0f, 0) != 0) {
            StageObj_K14_Create((Em *) em, 0xB, 0x26, em->scaleX);
            break;
        }
        break;
    case 0x3FC:
        if ((EmMotion_PassedFrameScaled(em, 74.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 178.0f, 0) != 0)) {
            EmShenGaoren_SpawnGroundEffect(em, 0x12, 7, 0, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 122.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0x16, 7, 0, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 236.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0x1A, 7, 0, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 288.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0x1E, 7, 0, 2.0f);
            break;
        }
        break;
    case 0x3FE:
        if (EmMotion_PassedFrameScaled(em, 94.0f, 0) != 0) {
            StageObj_K14_Create((Em *) em, 0, 0, em->scaleX);
        }
        if (EmMotion_PassedFrameScaled(em, 50.0f, 0) != 0) {
            ShenGaorenAttack_Create(em, 0x18);
            break;
        }
        break;
    case 0x400:
        if (EmMotion_PassedFrameScaled(em, 126.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 6, 7, 0, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 238.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0x1A, 7, 0, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 338.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0x12, 7, 0, 2.0f);
            break;
        }
        break;
    case 0x401:
        if (EmMotion_PassedFrameScaled(em, 34.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0x1A, 7, 0, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 86.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0x16, 7, 0, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 132.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0x12, 7, 0, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 184.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0x1E, 7, 0, 2.0f);
            break;
        }
        break;
    case 0x40B:
        if (EmMotion_PassedFrameScaled(em, 140.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0x12, 7, 1, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 238.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0x1A, 7, 1, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 340.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0x16, 7, 1, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 438.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0x1E, 7, 1, 2.0f);
            break;
        }
        break;
    case 0x411:
        if (EmMotion_PassedFrameScaled(em, 170.0f, 0) != 0) {
            ShenGaorenAttack_Create(em, 1);
        }
        if ((EmMotion_PassedFrameScaled(em, 204.0f, 0) != 0) && Player_InCurrentArea(em)) {
            v[0] = 0.0f;
            v[1] = 0.0f;
            v[2] = 1100.0f * em->scaleZ;
            Vec_RotateY(v, (6.2831855f * (f32) em->rotY) / 65536.0f);
            pos[0] = em->posX + v[0];
            pos[1] = em->floorY;
            pos[2] = em->posZ + v[2];
            StageObj_K1C_CreateAtYaw(pos, (u16) em->rotY, 0x11, 1.0f);
            break;
        }
        break;
    case 0x412:
        if (EmMotion_PassedFrameScaled(em, 82.0f, 0) != 0) {
            ShenGaorenAttack_Create(em, 0xC);
        }
        if (EmMotion_PassedFrameScaled(em, 146.0f, 0) != 0) {
            ShenGaorenAttack_Create(em, 0xD);
        }
        if (EmMotion_PassedFrameScaled(em, 160.0f, 0) != 0) {
            ShenGaorenAttack_Create(em, 0xE);
        }
        if (EmMotion_PassedFrameScaled(em, 166.0f, 0) != 0) {
            ShenGaorenAttack_Create(em, 0xF);
        }
        if (EmMotion_PassedFrameScaled(em, 242.0f, 0) != 0) {
            ShenGaorenAttack_Create(em, 0x10);
        }
        if (EmMotion_PassedFrameScaled(em, 334.0f, 0) != 0) {
            ShenGaorenAttack_Create(em, 0x11);
        }
        if (EmMotion_PassedFrameScaled(em, 96.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0x1A, 7, 0, 2.0f);
            EmShenGaoren_SpawnFootEffect(em, 0x1A);
        }
        if (EmMotion_PassedFrameScaled(em, 160.0f, 0) != 0) {
            StageObj_K14_Create((Em *) em, 2, 0x26, em->scaleX);
        }
        if (EmMotion_PassedFrameScaled(em, 176.0f, 0) != 0) {
            StageObj_K14_Create((Em *) em, 3, 0x27, em->scaleX);
            EmShenGaoren_SpawnGroundEffect(em, 0x16, 7, 0, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 256.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0x12, 7, 0, 2.0f);
            EmShenGaoren_SpawnFootEffect(em, 0x12);
        }
        if (EmMotion_PassedFrameScaled(em, 346.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0x1E, 7, 0, 2.0f);
            EmShenGaoren_SpawnFootEffect(em, 0x1E);
            break;
        }
        break;
    case 0x413:
        if (EmMotion_PassedFrameScaled(em, 88.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0xA, 7, 0, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 148.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0x16, 7, 0, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 158.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 6, 7, 0, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 248.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0x1E, 7, 0, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 380.0f, 0) != 0) {
            StageObj_K1C_CreateOnEm(em, 0x19, 1.0f);
            break;
        }
        break;
    case 0x414:
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            StageObj_K1C_CreateOnEm(em, 0x1A, 1.0f);
            break;
        }
        break;
    case 0x415:
        if (EmMotion_PassedFrameScaled(em, 176.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0x1E, 7, 0, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 192.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 6, 4, 0, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 270.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0x16, 7, 0, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 278.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0xA, 4, 0, 2.0f);
            break;
        }
        break;
    case 0x41A:
        if (EmMotion_PassedFrameScaled(em, 122.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0x16, 7, 0, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 178.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0x12, 7, 0, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 236.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0x1A, 7, 0, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 288.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0x1E, 7, 0, 2.0f);
            break;
        }
        break;
    case 0x41C:
        if (EmMotion_PassedFrameScaled(em, 116.0f, 0) != 0) {
            StageObj_K14_Create((Em *) em, 0, 0, em->scaleX);
        }
        if (EmMotion_PassedFrameScaled(em, 90.0f, 0) != 0) {
            ShenGaorenAttack_Create(em, 0x19);
            break;
        }
        break;
    case 0x41E:
        if (EmMotion_PassedFrameScaled(em, 80.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0xA, 7, 0, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 126.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 6, 7, 0, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 238.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0x1E, 7, 0, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 338.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0x12, 7, 0, 2.0f);
            break;
        }
        break;
    case 0x41F:
        if ((EmMotion_PassedFrameScaled(em, 62.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 188.0f, 0) != 0)) {
            EmShenGaoren_SpawnGroundEffect(em, 0x1A, 7, 0, 2.0f);
            break;
        }
        break;
    case 0x420:
        if ((EmMotion_PassedFrameScaled(em, 62.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 188.0f, 0) != 0)) {
            EmShenGaoren_SpawnGroundEffect(em, 0x12, 7, 0, 2.0f);
            break;
        }
        break;
    case 0x421:
        if ((EmMotion_PassedFrameScaled(em, 62.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 188.0f, 0) != 0)) {
            EmShenGaoren_SpawnGroundEffect(em, 0x1E, 7, 0, 2.0f);
            break;
        }
        break;
    case 0x422:
        if ((EmMotion_PassedFrameScaled(em, 62.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 188.0f, 0) != 0)) {
            EmShenGaoren_SpawnGroundEffect(em, 0x16, 7, 0, 2.0f);
            break;
        }
        break;
    case 0x423:
        if (EmMotion_PassedFrameScaled(em, 414.0f, 0) != 0) {
            StageObj_K14_Create((Em *) em, 0, 0x28, em->scaleX);
            break;
        }
        break;
    default:
        EmShenGaoren_MotionEffectsDefault(em);
        break;
    }
}

void EmShenGaoren_UpdateMotionEffects(Em *em)
{
    EmShenGaorenWork *w = (EmShenGaorenWork *)&em->work;

    switch (w->fxStep) {
    case 0:
        w->fxStep++;
        break;
    case 1:
        EmShenGaoren_MotionEffects(em, (EmWork *) w);
        break;
    }
}

void EmShenGaoren_SpawnAreaImpactEffects(Em *em, s32 flag)
{
    f32 pos[3];

    switch (Area_DayId(em->area)) {
    case AREA_FORTRESS_5:
        pos[0] = 7300.0f;
        if (flag != 0) {
            pos[1] = 1500.0f;
        } else {
            pos[1] = 500.0f;
        }
        pos[2] = 12400.0f;
        break;
    case AREA_FORTRESS_3:
        pos[0] = 10900.0f;
        pos[1] = 500.0f;
        pos[2] = 20158.0f;
        break;
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

void EmShenGaoren_TableNop(void)
{
}

u8 EmShenGaoren_ScriptQuery(Em *em)
{
    return em->work;
}

void EmKushala_MotionSounds(void *em, EmWork *work)
{
    s16 motion;

    motion = work->fxMotion;
    switch (motion) {
    case 0x3EA:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x24, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x4E, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x84, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xB2, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3E, 4, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x62, 4, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x96, 4, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xC4, 4, 8);
        break;
    case 0x3ED:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xC, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x28, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x42, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 3, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x24, 3, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x40, 4, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x48, 4, 0xD);
        break;
    case 0x3EE:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xC, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x28, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x42, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 3, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x24, 3, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x40, 4, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x48, 4, 8);
        break;
    case 0x3EF:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x16, 2);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x26, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x22, 3, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x30, 3, 8);
        break;
    case 0x3F0:
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 0x32, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x15, 0x25);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2C, 0x15, 0x11);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 3, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x28, 2, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2C, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x26, 5, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2A, 3, 8);
        break;
    case 0x3F1:
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 0x32, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x15, 0x25);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2C, 0x15, 0x11);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 3, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x28, 2, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2C, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x26, 5, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2A, 3, 0xD);
        break;
    case 0x3F2:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x32, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x26, 0x27, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2C, 0x14, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x14, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x26, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xE, 4, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1A, 3, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x20, 0xF, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x24, 0xF, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x28, 0x16, 2);
        break;
    case 0x3F3:
        Em_PlaySeAtBoneOnFrame((Em *) em, 2, 0xB, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 0xB, 0x32);
        break;
    case 0x3F4:
    case 0x404:
    case 0x405:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xD, 0x1A, 0x25);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x12, 0x19, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x17, 0x19, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2C, 0x19, 0x25);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3A, 0x19, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x41, 0x19, 0x2E);
        break;
    case 0x3F5:
        Em_PlaySeTypeAtBoneOnFrame((Em *) em, 0x28, 0x4C, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 2, 0xF, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 0xF, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x32, 0x1C, 0x25);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x34, 0x1D, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3A, 0x1D, 0x2E);
        break;
    case 0x3F6:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xC, 0xD, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x10, 0xD, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x38, 0xB, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3C, 0xB, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2C, 0x15, 0x11);
        break;
    case 0x3F7:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x28, 0xE, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2C, 0xE, 0x32);
        break;
    case 0x3F8:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x26, 0xE, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2A, 0xE, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 2, 0x1A, 0x25);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 0x19, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 0x19, 0x2E);
        break;
    case 0x3F9:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3C, 0x32, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x4A, 7, 2);
        Em_PlaySeAtBoneOnFrame((Em *) em, 2, 0xF, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 0xF, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x36, 0xD, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3A, 0xD, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x46, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x42, 2, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x48, 5, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x44, 4, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x7C, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x8E, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x92, 5, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x8A, 4, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xAE, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xC4, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xC2, 3, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xB8, 4, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x4C, 0x16, 2);
        break;
    case 0x3FA:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x31, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 0x14, 0x25);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0xD, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 0xD, 0x32);
        break;
    case 0x3FB:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x32, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 0x14, 0x25);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0xB, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 0xB, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0xD, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 0xD, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x16, 2);
        break;
    case 0x3FC:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x32, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 0x14, 0x25);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0xB, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 0xB, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0xD, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 0xD, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x16, 2);
        break;
    case 0x3FD:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x18, 0x26, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA, 2, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x20, 2, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x32, 2, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3E, 2, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 5, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x30, 5, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x38, 5, 0xD);
        break;
    case 0x3FE:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x10, 0x28, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1A, 2, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3E, 2, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x20, 5, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 2, 0x15, 0xD);
        break;
    case 0x3FF:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x14, 2, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1C, 2, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x22, 5, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 2, 0x15, 0xD);
        break;
    case 0x400:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x31, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1A, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x28, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1E, 3, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2C, 4, 0xD);
        break;
    case 0x401:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x31, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x28, 0x23, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x10, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1E, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x14, 4, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x28, 0xF, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2C, 0xF, 0x32);
        break;
    case 0x402:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1C, 7, 2);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1A, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1E, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 5, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x18, 3, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1C, 0xC, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x20, 0xC, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x24, 0x16, 2);
        break;
    case 0x403:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xE, 0xD, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x14, 0xD, 0x32);
        break;
    case 0x406:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x24, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2A, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x18, 5, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x24, 3, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 0xE, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1A, 0xE, 0x32);
        break;
    case 0x407:
        Em_PlaySeTypeAtBoneOnFrame((Em *) em, 0xA, 0x47, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x14, 2, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x70, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x14, 4, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0xE, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA, 0xE, 0x2E);
        break;
    case 0x408:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x20, 0x26);
        Em_PlaySeTypeAtBoneInFrames((Em *) em, 0x3C, 0xBE, 0x4A, 0x26, 7, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 0xF, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA, 0xF, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x60, 0xE, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x64, 0xE, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xBC, 8, 2);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xB8, 9, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xC0, 2, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xF2, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xF8, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x12, 4, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 3, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xF6, 3, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xBA, 0x16, 2);
        break;
    case 0x40A:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x31, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x8A, 0x32, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x16, 2);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x12, 2, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x9E, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xCA, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x10, 5, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1E, 4, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xB4, 3, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x84, 4, 0xD);
        break;
    case 0x40B:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x36, 0x3A, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x64, 0x39, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1A, 0x32, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x14, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x14, 4, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1A, 4, 8);
        break;
    case 0x40C:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x3B, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x14, 4, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1A, 4, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x28, 1, 0x1A);
        break;
    case 0x40D:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x2F, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x54, 0x2F, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x40, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x60, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x80, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x36, 4, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x58, 3, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 0x12, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x10, 0x16, 2);
        break;
    case 0x40E:
        Em_PlaySeTypeAtBoneOnFrame((Em *) em, 4, 0x56, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xE0, 0x3B, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x20, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x6A, 0x10, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1E, 3, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x46, 4, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x7C, 0xE, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x80, 0xE, 0x2E);
        break;
    case 0x40F:
        Em_PlaySeTypeAtBoneOnFrame((Em *) em, 4, 0x63, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x46, 0x32, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x4A, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x6E, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x84, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x62, 3, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x80, 4, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x30, 0xE, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x34, 0xE, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x6C, 0x17, 2);
        break;
    case 0x416:
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 0x20, 0x26);
        Em_PlaySeTypeAtBoneOnFrame((Em *) em, 0x40, 0x4C, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x4A, 0x18, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1E, 0xE, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x22, 0xE, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x40, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x46, 2, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x8A, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x9C, 0, 0x1F);
        break;
    case 0x419:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x20, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 2, 0xC, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 0xC, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x30, 0xB, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x34, 0xB, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x6C, 0xB, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x70, 0xB, 0x2E);
        break;
    case 0x41A:
        Em_PlaySeTypeAtBoneInFrames((Em *) em, 0x16, 0x11C, 0x49, 0x26, 7, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 2, 0xB, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 0xB, 0x2E);
        break;
    case 0x41B:
        Em_PlaySeTypeAtBoneOnFrame((Em *) em, 0x16, 0x4C, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x12, 0x18, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 0xE, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA, 0xE, 0x2E);
        break;
    case 0x410:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x12, 0xE, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 0xE, 0x2E);
        break;
    case 0x415:
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x14, 0xE, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x18, 0xE, 0x2E);
        break;
    case 0x41C:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x25, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x36, 0x23, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 2, 0xE, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 0xE, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2A, 0xB, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2E, 0xB, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3A, 0x3C, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3E, 9, 0x1F);
        break;
    case 0x41D:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x26, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x32, 0x27, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 2, 0xE, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 0xE, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2A, 0xF, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2E, 0xF, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x30, 0x15, 0x11);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x38, 9, 0x11);
        break;
    case 0x41E:
        Em_PlaySeTypeAtBoneOnFrame((Em *) em, 4, 0x47, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x32, 0x23, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 2, 0xA, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 0xA, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2A, 0xF, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2E, 0xF, 0x2E);
        break;
    case 0x41F:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x31, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x28, 0x27, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x7E, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x30, 0x15, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x76, 2, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x12, 3, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3C, 5, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x70, 3, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x46, 5, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x80, 3, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x48, 0x15, 0x11);
        break;
    case 0x420:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x31, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x28, 0x27, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x7E, 9, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2C, 0x15, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x40, 0x15, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x76, 2, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x12, 3, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3C, 5, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x70, 3, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x46, 5, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x80, 3, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x4A, 0x15, 0x11);
        break;
    case 0x421:
    case 0x422:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x26, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2C, 0x28, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x24, 0x14, 0x25);
        Em_PlaySeAtBoneOnFrame((Em *) em, 2, 0xD, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 0xD, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2A, 0xF, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2E, 0xF, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x80, 0xC, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x84, 0xC, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x38, 0x16, 2);
        break;
    case 0x425:
        Em_PlaySeTypeAtBoneOnFrame((Em *) em, 4, 0x53, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x38, 0x23, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x16, 2);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xE, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x14, 3, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x58, 4, 8);
        break;
    case 0x426:
        Em_PlaySeTypeAtBoneOnFrame((Em *) em, 4, 0x54, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1C, 7, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xE, 2, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x62, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x14, 2, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x5C, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x52, 3, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x68, 3, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 0xE, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1A, 0xE, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1C, 0x16, 2);
        break;
    case 0x427:
        Em_PlaySeTypeAtBoneOnFrame((Em *) em, 4, 0x53, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xE, 0xD, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x12, 0xD, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3A, 0xB, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3E, 0xB, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x6E, 0xB, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x72, 0xB, 0x32);
        break;
    case 0x428:
        Em_PlaySeTypeAtBoneOnFrame((Em *) em, 6, 0x54, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 0xC, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA, 0xC, 0x32);
        break;
    case 0x429:
        Em_PlaySeTypeAtBoneOnFrame((Em *) em, 6, 0x53, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 9, 0x25);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x18, 7, 2);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1A, 0xF, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x14, 0x17, 2);
        break;
    case 0x42A:
        Em_PlaySeTypeAtBoneOnFrame((Em *) em, 0x18, 0x57, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x26, 0x10, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1A, 0x17, 0x26);
        break;
    case 0x42B:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x16, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 2, 0x12, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x5A, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3A, 2, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x56, 3, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x62, 3, 0xD);
        break;
    case 0x42C:
        Em_PlaySeTypeAtBoneOnFrame((Em *) em, 4, 0x55, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xDA, 0x2F, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x11A, 0x2E, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16E, 0x2C, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x60, 0x17, 2);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xC0, 0x16, 2);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x11A, 0x17, 2);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1E, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x2C, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xC, 3, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1C, 4, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x190, 0x10, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x19C, 7, 2);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1B6, 7, 0x25);
        break;
    case 0x42D:
        Em_PlaySeTypeAtBoneOnFrame((Em *) em, 0xC, 0x58, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xC, 0x16, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1A, 0x12, 0x1A);
        break;
    case 0x42E:
        Em_PlaySeTypeAtBoneOnFrame((Em *) em, 0x1E, 0x4F, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1E, 0x16, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x16, 0xD, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x1A, 0xD, 0x32);
        break;
    case 0x42F:
        Em_PlaySeTypeAtBoneOnFrame((Em *) em, 4, 0x56, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x46, 0x17, 2);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x10A, 0x3B, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x14, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x9C, 0x12, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x3A, 0x12, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA0, 0x10, 2);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xB8, 7, 2);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x30, 0xE, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x34, 0xE, 0x32);
        break;
    case 0x431:
        Em_PlaySeTypeAtBoneOnFrame((Em *) em, 4, 0x55, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x2E, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x18, 0x17, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x10, 0xD, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x14, 0xD, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x40, 0xC, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x44, 0xC, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x88, 0xE, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x58, 3, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x6E, 0x12, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x86, 7, 2);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x96, 5, 0x25);
        break;
    case 0x432:
        Em_PlaySeTypeAtBoneOnFrame((Em *) em, 4, 0x53, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xA, 0x16, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x38, 7, 2);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x4C, 5, 0x25);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 0xD, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xC, 0xD, 0x32);
        break;
    case 0x433:
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0x32, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0xC, 0x17, 0x26);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 0xE, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x56, 0xE, 0x2E);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x5A, 0xE, 0x32);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x7A, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x8A, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x54, 4, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x80, 4, 8);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x5E, 4, 0xD);
        Em_PlaySeAtBoneOnFrame((Em *) em, 0x8A, 0, 0xD);
        
    case 0x3E9:
    default:
        break;
    }
}

void EmKushala_MotionSoundsTesca(Em *em, EmWork *work)
{
    s16 motion;

    motion = work->fxMotion;
    switch (motion) {
    case 0x3EA:
        Em_PlaySeAtBoneOnFrame(em, 0x24, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x84, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0xB2, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 4, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x62, 4, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x96, 4, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0xC4, 4, 8);
        break;
    case 0x3ED:
        Em_PlaySeAtBoneOnFrame(em, 8, 0x32, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x42, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 3, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 3, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 4, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 4, 0xD);
        break;
    case 0x3EE:
        Em_PlaySeAtBoneOnFrame(em, 8, 0x32, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x42, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 3, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 3, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 4, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 4, 8);
        break;
    case 0x3EF:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x3C, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 3, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 3, 8);
        break;
    case 0x3F0:
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x3C, 2, 0x3C, 2), 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x36, 2, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 3, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 4, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 4, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 3, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0xE, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x36, 0xE, 0x32);
        break;
    case 0x3F1:
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x3C, 2, 0x3C, 2), 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x36, 2, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 3, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 4, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 4, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 3, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0xE, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x36, 0xE, 0x32);
        break;
    case 0x3F2:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x3C, 0x26);
        Em_PlaySeTypeAtBoneOnFrame(em, 0x26, 0x5B, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x14, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 4, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 3, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 0xF, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0xF, 0x32);
        break;
    case 0x3F3:
        Em_PlaySeAtBoneOnFrame(em, 2, 0xB, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 6, 0xB, 0x32);
        break;
    case 0x3F4:
    case 0x404:
    case 0x405:
        Em_PlaySeAtBoneOnFrame(em, 0xD, 0x1A, 0x25);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0x19, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x17, 0x19, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x19, 0x25);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 0x19, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x41, 0x19, 0x2E);
        break;
    case 0x3F5:
        Em_PlaySeTypeAtBoneOnFrame(em, 0x28, 0x4B, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 2, 0xF, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 6, 0xF, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x1C, 0x25);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 0x1D, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 0x1D, 0x2E);
        break;
    case 0x3F6:
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 0x51, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0xD, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 0xD, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0xB, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0xB, 0x32);
        break;
    case 0x3F7:
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 0x31, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0x3E, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 0xE, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0xE, 0x32);
        break;
    case 0x3F8:
        Em_PlaySeAtBoneOnFrame(em, 0x26, 0xE, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 0xE, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 2, 0x1A, 0x25);
        Em_PlaySeAtBoneOnFrame(em, 6, 0x19, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 8, 0x19, 0x2E);
        break;
    case 0x3F9:
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 7, 2);
        Em_PlaySeAtBoneOnFrame(em, 2, 0xF, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 6, 0xF, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x36, 0xD, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 0xD, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x42, 2, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 5, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 4, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x7C, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x8E, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x92, 5, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x8A, 4, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0xAE, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0xC4, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0xC2, 3, 8);
        Em_PlaySeAtBoneOnFrame(em, 0xB8, 4, 0xD);
        break;
    case 0x3FA:
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 0x51, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 8, 0x14, 0x25);
        Em_PlaySeAtBoneOnFrame(em, 4, 0xD, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 8, 0xD, 0x32);
        break;
    case 0x3FD:
        Em_PlaySeAtBoneOnFrame(em, 0xA, 2, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 2, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 2, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 2, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 5, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 5, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 5, 0xD);
        break;
    case 0x3FE:
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 2, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 2, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 5, 8);
        Em_PlaySeAtBoneOnFrame(em, 2, 0x15, 0xD);
        break;
    case 0x3FF:
        Em_PlaySeAtBoneOnFrame(em, 0x14, 2, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 2, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 5, 8);
        Em_PlaySeAtBoneOnFrame(em, 2, 0x15, 0xD);
        break;
    case 0x400:
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 3, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 4, 0xD);
        break;
    case 0x401:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x36, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 4, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 0xF, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0xF, 0x32);
        break;
    case 0x402:
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 7, 2);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 5, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 3, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 0xC, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 0xC, 0x32);
        break;
    case 0x403:
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0xD, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0xD, 0x32);
        break;
    case 0x406:
        Em_PlaySeAtBoneOnFrame(em, 0x20, 0x3D, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 4, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 3, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0xE, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0xE, 0x32);
        break;
    case 0x407:
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x33, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 0x32, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 2, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x70, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 4, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 4, 0xE, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0xE, 0x2E);
        break;
    case 0x408:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x25, 0x26);
        Em_PlaySeTypeAtBoneInFrames(em, 0x38, 0xAA, 0x4A, 0x26, 7, 0);
        Em_PlaySeAtBoneOnFrame(em, 6, 0xF, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0xF, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x60, 0xE, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 0xE, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0xBC, 8, 2);
        Em_PlaySeAtBoneOnFrame(em, 0xB8, 2, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0xC0, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0xF2, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0xF8, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 4, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 3, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0xF6, 3, 0xD);
        break;
    case 0x40A:
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0x29, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 0x3E, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x78, 0x36, 2);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 2, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x9E, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0xCA, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 5, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 4, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0xB4, 3, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x84, 4, 0xD);
        break;
    case 0x40B:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x3C, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0x39, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 0x3A, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x82, 0x3B, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0xA8, 0x23, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 4, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 4, 8);
        break;
    case 0x40C:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x32, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 4, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 4, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 1, 0x1A);
        break;
    case 0x40D:
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 0x55, 0x26);
        Em_PlaySeTypeAtBoneOnFrame(em, 0x54, 0x55, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0x12, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 0x12, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x80, 0x12, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x60, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x80, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x36, 4, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 3, 8);
        break;
    case 0x40E:
        Em_PlaySeAtBoneOnFrame(em, 0x50, 0x26, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0xE0, 0x36, 0x26);
        if (em->stateGroup == 6) {
            if (em->state != 7) {
                goto block_49;
            }
        default:
            break;
        }
block_49:
        Em_PlaySeAtBoneOnFrame(em, 0x20, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 0x10, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 3, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 4, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x7C, 0xE, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x80, 0xE, 0x2E);
        break;
    case 0x40F:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x32, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 0x3B, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 0x3E, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x4A, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x6E, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x84, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x62, 3, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x80, 4, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0xE, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 0xE, 0x2E);
        break;
    case 0x411:
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 0x5B, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 6, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 8, 3, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0x14, 2);
        break;
    case 0x409:
        Em_PlaySeTypeAtBoneOnFrame(em, 0xA, 0x51, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x15, 0x25);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 9, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 7, 2);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 2, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 4, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 3, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 3, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 4, 8);
        break;
    case 0x412:
        Em_PlaySeTypeAtBoneOnFrame(em, 0xA, 0x4F, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x6E, 0x31, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 6, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0x15, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x7E, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x84, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 0xE, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 0xE, 0x2E);
        break;
    case 0x413:
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 0x4D, 0x26);
        Em_PlaySeTypeAtBoneOnFrame(em, 0xB8, 0x5E, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x78, 0x15, 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x98, 0x15, 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0xD4, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0xE4, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 4, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 3, 0xD);
        break;
    case 0x417:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x3C, 0x26);
        Em_PlaySeTypeAtBoneOnFrame(em, 0x32, 0x48, 0x26);
        Em_PlaySeTypeAtBoneOnFrame(em, 0x62, 0x49, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 0x41, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x36, 0x12, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 2, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x68, 1, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 3, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 3, 0xD);
        break;
    case 0x418:
        Em_PlaySeTypeAtBoneOnFrame(em, 6, 0x51, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 0x33, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0xE, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0xE, 0x2E);
        break;
    case 0x419:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x20, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 2, 0xC, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 6, 0xC, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0xB, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 0xB, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x6C, 0xB, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x70, 0xB, 0x2E);
        break;
    case 0x41A:
        Em_PlaySeTypeAtBoneInFrames(em, 0x16, 0x11C, 0x49, 0x26, 7, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0x41, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 2, 0xB, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 6, 0xB, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 0xB, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 0xB, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x8C, 0xB, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x90, 0xB, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0xCA, 0xB, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0xCE, 0xB, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x10C, 0xB, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x110, 0xB, 0x2E);
        break;
    case 0x410:
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 0x5B, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0xE, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0xE, 0x2E);
        break;
    case 0x41D:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x36, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 0x29, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 2, 0xE, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 6, 0xE, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 0xF, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 0xF, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 0x15, 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 9, 0x11);
        break;
    case 0x41F:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x31, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 0x27, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x5A, 0x3C, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x7E, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0x15, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x76, 2, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 3, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 5, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x70, 3, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 5, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x80, 3, 0xD);
        break;
    case 0x420:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x31, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 0x27, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x5A, 0x3C, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x7E, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0x15, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x76, 2, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 3, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 5, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x70, 3, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x46, 5, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x80, 3, 8);
        break;
    case 0x423:
        Em_PlaySeAtBoneOnFrame(em, 2, 0x20, 0x26);
        Em_PlaySeTypeAtBoneOnFrame(em, 2, 0x4D, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x50, 0x3E, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0x32, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 4, 0xB, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 8, 0xB, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 0xC, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 0xC, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x64, 0xC, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x68, 0xC, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 0x24, 2);
        Em_PlaySeAtBoneVolOnFrame(em, 0x3E, Sound_RandomSe(0x43, 1, 0x44, 1), 0x2E, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x46, Sound_RandomSe(0x45, 2, 0x43, 1), 0x2E, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x4A, Sound_RandomSe(0x44, 4, 0x45, 2), 0x2E, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x4E, Sound_RandomSe(0x43, 2, 0x44, 2), 0x2E, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x58, Sound_RandomSe(0x45, 2, 0x43, 1), 0x2E, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x5E, Sound_RandomSe(0x43, 4, 0x44, 1), 0x2E, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x6C, Sound_RandomSe(0x45, 2, 0x43, 2), 0x2E, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x74, Sound_RandomSe(0x44, 3, 0x44, 2), 0x2E, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x7C, Sound_RandomSe(0x45, 3, 0x45, 4), 0x2E, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x84, Sound_RandomSe(0x43, 3, 0x44, 3), 0x2E, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x90, Sound_RandomSe(0x43, 4, 0x45, 2), 0x2E, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x44, Sound_RandomSe(0x45, 1, 0x43, 1), 0x32, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x4A, Sound_RandomSe(0x45, 1, 0x43, 1), 0x32, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x54, Sound_RandomSe(0x44, 1, 0x43, 3), 0x32, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x5A, Sound_RandomSe(0x45, 2, 0x43, 2), 0x32, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x62, Sound_RandomSe(0x45, 1, 0x44, 1), 0x32, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x6C, Sound_RandomSe(0x43, 1, 0x43, 2), 0x32, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x70, Sound_RandomSe(0x45, 2, 0x43, 2), 0x32, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x76, Sound_RandomSe(0x45, 2, 0x44, 2), 0x32, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x7C, Sound_RandomSe(0x45, 4, 0x43, 3), 0x32, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x84, Sound_RandomSe(0x45, 2, 0x45, 4), 0x32, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x92, Sound_RandomSe(0x43, 4, 0x44, 2), 0x32, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x40, Sound_RandomSe(0x44, 1, 0x45, 2), 0x11, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x48, Sound_RandomSe(0x45, 1, 0x44, 1), 0x11, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x50, Sound_RandomSe(0x44, 1, 0x45, 2), 0x11, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x5A, Sound_RandomSe(0x44, 2, 0x44, 1), 0x11, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x62, Sound_RandomSe(0x43, 4, 0x45, 2), 0x11, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x68, Sound_RandomSe(0x44, 2, 0x44, 2), 0x11, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x6E, Sound_RandomSe(0x43, 2, 0x45, 2), 0x11, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x78, Sound_RandomSe(0x44, 2, 0x44, 2), 0x11, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x86, Sound_RandomSe(0x43, 3, 0x45, 4), 0x11, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x8C, Sound_RandomSe(0x44, 4, 0x43, 2), 0x11, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0x94, Sound_RandomSe(0x43, 3, 0x45, 4), 0x11, 4);
        break;
    case 0x424:
        Em_PlaySeTypeAtBoneInFrames(em, 0x58, 0x104, 0x50, 0x26, 7, 0);
        Em_PlaySeAtBoneOnFrame(em, 2, 0x33, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0x3A, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0x3F, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0xC4, 0x3C, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 4, 0xD, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 8, 0xD, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x28, 0xF, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0xF, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0xD2, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0xE2, 1, 0x1F);
        break;
    case 0x425:
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 0x53, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0x23, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 3, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 4, 8);
        break;
    case 0x426:
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 0x54, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 0x29, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 7, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 2, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x62, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 2, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x5C, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x52, 3, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x68, 3, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0xE, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0xE, 0x32);
        break;
    case 0x427:
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 0x53, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0xD, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0xD, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 0xB, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 0xB, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x6E, 0xB, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x72, 0xB, 0x32);
        break;
    case 0x428:
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 0x52, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 6, 0xC, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0xC, 0x32);
        break;
    case 0x429:
        Em_PlaySeAtBoneOnFrame(em, 8, 9, 0x25);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 7, 2);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0xF, 0x32);
        break;
    case 0x42A:
        Em_PlaySeTypeAtBoneOnFrame(em, 0x18, 0x57, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 0x10, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x17, 0x26);
        break;
    case 0x42B:
        Em_PlaySeAtBoneOnFrame(em, 0x10, 0x29, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 2, 0x12, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x5A, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 2, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x56, 3, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x62, 3, 0xD);
        break;
    case 0x42C:
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 0x57, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 0x30, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0xAA, 0x3B, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0xEE, 0x2D, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x11A, 0x26, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 1, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 3, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 4, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x190, 0x10, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x19C, 7, 2);
        Em_PlaySeAtBoneOnFrame(em, 0x1B6, 7, 0x25);
        break;
    case 0x42D:
        Em_PlaySeTypeAtBoneOnFrame(em, 0xC, 0x58, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x12, 0x1A);
        break;
    case 0x42E:
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 0x51, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0x33, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 0x3B, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0xD, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0xD, 0x32);
        break;
    case 0x42F:
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 0x55, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x4E, 0x3C, 0x26);
        Em_PlaySeTypeAtBoneOnFrame(em, 0x3E, 0x4E, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x10E, 0x36, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x9C, 0x12, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 0x12, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0xA0, 0x10, 2);
        Em_PlaySeAtBoneOnFrame(em, 0xB8, 7, 2);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0xE, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 0xE, 0x32);
        break;
    case 0x431:
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 0x54, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x26, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 0x2A, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 0xD, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0xD, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 0xC, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 0xC, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x88, 0xE, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 3, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x6E, 0x12, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x86, 5, 2);
        Em_PlaySeAtBoneOnFrame(em, 0x96, 9, 0x25);
        break;
    case 0x432:
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 0x54, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 5, 2);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 9, 0x25);
        Em_PlaySeAtBoneOnFrame(em, 8, 0xD, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0xD, 0x32);
        break;
    case 0x433:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x3C, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 0x33, 0x26);
        Em_PlaySeAtBoneOnFrame(em, 4, 0xE, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x56, 0xE, 0x2E);
        Em_PlaySeAtBoneOnFrame(em, 0x5A, 0xE, 0x32);
        Em_PlaySeAtBoneOnFrame(em, 0x7A, 0, 0x1A);
        Em_PlaySeAtBoneOnFrame(em, 0x8A, 0, 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x54, 4, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x80, 4, 8);
        Em_PlaySeAtBoneOnFrame(em, 0x5E, 4, 0xD);
        Em_PlaySeAtBoneOnFrame(em, 0x8A, 0, 0xD);
        break;
    case 0x3E9:
        break;
    }
}

/* em_shengaoren's .data (0x8CA080-0x8CA100) */
EmTurnParams EmShenGaoren_TurnParams[2] = {
#include "game/008CA080.inc"
};
f32 D_008CA0C0[2][4] = {
#include "game/008CA0C0.inc"
};
void *EmShenGaoren_Callbacks[8] = {
    EmShenGaoren_SpawnEffect01, EmShenGaoren_SpawnEffect01, EmShenGaoren_TableNop, EmShenGaoren_UpdateMotionEffects,
    EmShenGaoren_ScriptQuery, EmShenGaoren_TableNop, NULL, NULL,
};
