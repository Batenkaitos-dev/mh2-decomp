/*
 * The Shen Gaoren family (EM_KIND_SHEN_GAOREN), the giant crab that crosses
 * the fortress areas to the fort, and shoots at, strikes and smashes it.
 * The file holds its motion sounds and effects, its setup and Init
 * (EmShenGaoren_Setup, which the per-kind dispatch Em_TypeSetup calls), the
 * mine point carved from the living monster, the state functions of each
 * group with their dispatchers (EmShenGaoren_Run*State) and the per-frame
 * update (EmShenGaoren_Update). The family's state changes
 * (EmShenGaoren_ChangeState and the Set*State functions) come earlier in the
 * game's code, after the Blangonga family's, and are compiled there.
 *
 * The state functions take the monster; Em.step is the step within the
 * current state, and the family keeps its own values in the monster's work
 * area (EmShenGaorenWork: the fight's stage, the hits on the fort, the mine
 * point, the legs' flinch counts, the eased pose of the lying motions).
 * em_shengaoren.h names the motions, the states and the two stances (tall
 * on its legs, or lowered). On the host the update sends the monster's
 * state to the other players every 150 frames.
 */
#include "common.h"
#include "scene.h"
#include "quest.h"
#include "mine_point.h"
#include "em.h"
#include "stageobj.h"
#include "player.h"
#include "objattach.h"
#include "area.h"
#include "qsync.h"
#include "em_shengaoren.h"
/* prototypes of callees */
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
s32 Quest_GetOutcome(s32 withLoss);
void EmShenGaoren_RunState(Em *em);
void EmShenGaoren_IdleLow(Em *);
void EmShenGaoren_WalkTall(Em *);
void EmShenGaoren_WalkBackTall(Em *);
void EmShenGaoren_WalkLow(Em *);
void EmShenGaoren_WalkBackLow(Em *);
void EmShenGaoren_ShotLow(Em *, s32);
void EmShenGaoren_StrikeFort(Em *);
void EmShenGaoren_Smash(Em *, s32);
void EmShenGaoren_DieTall(Em *);
void EmShenGaoren_DieLow(Em *);
void EmShenGaoren_EventCutsceneShot(Em *);
void EmShenGaoren_EventLeave(Em *);
void EmShenGaoren_MotionEffects(Em *, EmWork *);
void EmShenGaoren_MotionSounds(Em *, EmWork *work);
void DustCloud_Create(Em *em, s16 k, s16 c, f32 scale);
StageObj *MonsterFx_CreateOnEm(Em *em, u8 k, f32 scale);
void Vec_RotateY(f32 *v, f32 angle);
StageObj *MonsterFx_CreateAtYaw(f32 *pos, u16 v, u8 k, f32 scale);
s32 Sound_RandomSe(s32 id, s16 lo, s32 alt, s16 n);
s32 EmMotion_PassedFrameScaled(void *p, f32 frame, s32 layer);
s32 Debug_IsOn(s32);
void MinePoint_SetArea(s32 slot, u16 area);
void Shadow_Spawn(void *obj, s16 type);
extern EmTurnParams EmShenGaoren_TurnParams[2];
void Em_SetMotionRow(Em *em, s32 row, s32 arg2, s32 arg3);
void Em_SetStateRand(Em *em, s32 arg1, s32 arg2);
s32 Em_ComputeMaxHp(Em *em, s16 idx);
void EmShenGaoren_AddMinePoint(Em *, s32);
u16 Area_DayId(u16 id);
/* unit's own declaration: declared here as the callers use it; with the definition's void (void *obj, s16 bone, f32 *out), the calls in EmShenGaoren_SpawnFootEffect and EmShenGaoren_SpawnGroundEffect compile differently */
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
extern f32 EmShenGaoren_DownEaseTargets[][4];
void PartFx_SpawnOnGround(f32 *pos, s32 a, f32 scale);
void ShenGaorenAttack_Create(Em *em, s8 kind);
s32 Em_PlaySeChecked(Em *em, s32 id, s32 a, void *pos, s32 b, s32 c);
void HitSpark_SpawnForOwner(void *owner, s32 ang, s32 type, s32 sub, f32 *pos, f32 scale);
void PartFx_SpawnIfHere(void *o, f32 *pos, s32 type, f32 scale);
f32 Vec_DistanceXZ(f32 *a, f32 *b);
u16 Math_AngleXZ(f32 *from, f32 *to);
extern f32 EmShenGaoren_SeOffsets[4][3];
/* end of prototypes */

/*
 * Plays sound se at bone 0xE position plus row idx of
 * EmShenGaoren_SeOffsets rotated by the bone matrix (Em_PlaySeChecked with
 * volume vol).
 */
void EmShenGaoren_PlaySeAtOffset(Em *em, s32 seId, s32 idx, s32 vol)
{
    f32 ofs[3];
    f32 *ofsRow;
    f32 rot[3];
    f32 pos[3];

    ofsRow = EmShenGaoren_SeOffsets[idx];
    ofs[0] = *ofsRow++;
    ofs[1] = *ofsRow++;
    ofs[2] = *ofsRow;
    Em_BonePos(em, 0xE, pos);
    Vec_Rotate(rot, ofs, (f32 *)Em_Bone(em, 0xE));
    pos[0] = pos[0] + rot[0];
    pos[1] = pos[1] + rot[1];
    pos[2] = pos[2] + rot[2];
    Em_PlaySeChecked(em, seId, 0, pos, vol, 0);
}

/*
 * When the layer 0 motion passes the frame, plays se through
 * EmShenGaoren_PlaySeAtOffset with offset row idx and volume vol (the
 * monster comes in as an s32).
 */
void EmShenGaoren_PlaySeOnFrame(s32 em, s32 frame, s32 seId, s32 idx, s32 vol)
{
    if (EmMotion_PassedFrameScaled((Em *) em, (f32) frame, 0U) != 0) {
        EmShenGaoren_PlaySeAtOffset((Em *) em, seId, idx, vol);
    }
}

/*
 * Plays the footstep, body and shell sounds of the motion in fxMotion on
 * their frames (Em_PlaySeAtBoneOnFrame and the like,
 * EmShenGaoren_PlaySeOnFrame for the death motion).
 */
void EmShenGaoren_MotionSounds(Em *em, EmWork *work)
{
    s16 motion;

    motion = ((EmShenGaorenWork *)work)->fxMotion;
    switch (motion) {
    case EM_MOTION_ID(0, SHEN_MOT_IDLE_TALL):
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x3A, 4, 0x3B, 4), 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0xB8, Sound_RandomSe(0x3C, 4, 0x3A, 4), 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x162, Sound_RandomSe(0x3B, 4, 0x3C, 4), 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x1A4, Sound_RandomSe(0x3A, 4, 0x3B, 4), 0x1F);
        break;
    case EM_MOTION_ID(0, SHEN_MOT_WALK_TALL):
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
    case EM_MOTION_ID(0, SHEN_MOT_WALK_BACK_TALL):
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
    case EM_MOTION_ID(0, 0x4):
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
    case EM_MOTION_ID(0, 0x5):
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
    case EM_MOTION_ID(0, SHEN_MOT_LOWER):
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
    case EM_MOTION_ID(0, SHEN_MOT_STRIKE_A):
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
    case EM_MOTION_ID(0, SHEN_MOT_STRIKE_B):
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
    case EM_MOTION_ID(0, SHEN_MOT_TRAMPLE_TALL):
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
    case EM_MOTION_ID(0, SHEN_MOT_AIM_TALL):
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
    case EM_MOTION_ID(0, SHEN_MOT_FIRE_TALL):
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
    case EM_MOTION_ID(0, SHEN_MOT_SHOT_END_TALL):
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
    case EM_MOTION_ID(0, SHEN_MOT_SMASH):
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
    case EM_MOTION_ID(0, 0x14):
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
    case EM_MOTION_ID(0, SHEN_MOT_FLINCH_TALL):
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
    case EM_MOTION_ID(0, SHEN_MOT_FALL_TALL):
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
    case EM_MOTION_ID(0, SHEN_MOT_DOWN_TALL):
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0x5F, 0x1F, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x12, Sound_RandomSe(0xC, 2, 0x16, 2), 0x19);
        Em_PlaySeAtBoneOnFrame(em, 0x18, Sound_RandomSe(0xE, 2, 0x17, 2), 0x15);
        Em_PlaySeAtBoneOnFrame(em, 0x7A, Sound_RandomSe(0xD, 2, 0x17, 2), 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x82, Sound_RandomSe(0xF, 2, 0x16, 2), 0x1D);
        break;
    case EM_MOTION_ID(0, SHEN_MOT_GET_UP_TALL):
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
    case EM_MOTION_ID(0, SHEN_MOT_DIE_TALL):
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
    case EM_MOTION_ID(0, SHEN_MOT_IDLE_LOW):
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x3A, 4, 0x3B, 4), 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0xB8, Sound_RandomSe(0x3C, 4, 0x3A, 4), 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x162, Sound_RandomSe(0x3B, 4, 0x3C, 4), 0x1F);
        Em_PlaySeAtBoneOnFrame(em, 0x1A4, Sound_RandomSe(0x3A, 4, 0x3B, 4), 0x1F);
        break;
    case EM_MOTION_ID(0, SHEN_MOT_WALK_LOW):
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
    case EM_MOTION_ID(0, SHEN_MOT_WALK_BACK_LOW):
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
    case EM_MOTION_ID(0, 0x21):
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
    case EM_MOTION_ID(0, 0x22):
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
    case EM_MOTION_ID(0, SHEN_MOT_RISE):
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
    case EM_MOTION_ID(0, SHEN_MOT_STRIKE_FORT):
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
    case EM_MOTION_ID(0, SHEN_MOT_TRAMPLE_LOW):
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
    case EM_MOTION_ID(0, SHEN_MOT_AIM_LOW):
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
    case EM_MOTION_ID(0, SHEN_MOT_FIRE_LOW):
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
    case EM_MOTION_ID(0, SHEN_MOT_SHOT_END_LOW):
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
    case EM_MOTION_ID(0, SHEN_MOT_FLINCH_SHOT_LOW):
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
    case EM_MOTION_ID(0, 0x32):
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
    case EM_MOTION_ID(0, SHEN_MOT_FLINCH_LOW):
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
    case EM_MOTION_ID(0, SHEN_MOT_COLLAPSE):
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
    case EM_MOTION_ID(0, SHEN_MOT_DOWN_LOW):
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0x5F, 0x1F, 9);
        Em_PlaySeAtBoneOnFrame(em, 0x12, Sound_RandomSe(0xC, 2, 0x16, 2), 0x19);
        Em_PlaySeAtBoneOnFrame(em, 0x18, Sound_RandomSe(0xE, 2, 0x17, 2), 0x15);
        Em_PlaySeAtBoneOnFrame(em, 0x7A, Sound_RandomSe(0xD, 2, 0x17, 2), 0x11);
        Em_PlaySeAtBoneOnFrame(em, 0x82, Sound_RandomSe(0xF, 2, 0x16, 2), 0x1D);
        Em_PlaySeAtBoneOnFrame(em, 0x44, Sound_RandomSe(0xD, 2, 0x17, 2), 5);
        Em_PlaySeAtBoneOnFrame(em, 0x60, Sound_RandomSe(0xF, 2, 0x16, 2), 9);
        break;
    case EM_MOTION_ID(0, SHEN_MOT_GET_UP_LOW):
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
    case EM_MOTION_ID(0, SHEN_MOT_FLINCH_LEG_C):
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
    case EM_MOTION_ID(0, SHEN_MOT_FLINCH_LEG_A):
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
    case EM_MOTION_ID(0, SHEN_MOT_FLINCH_LEG_D):
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
    case EM_MOTION_ID(0, SHEN_MOT_FLINCH_LEG_B):
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
    case EM_MOTION_ID(0, SHEN_MOT_DIE_LOW):
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

/*
 * Clears scriptStage and, when a mine point exists, moves it to the monster
 * area and to EmShenGaoren_GetMinePointPos.
 */
void EmShenGaoren_Setup(Em *em)
{
    f32 pos[4];
    EmShenGaorenWork *w = (EmShenGaorenWork *)&em->work;

    w->stage = 0;
    if (w->mineSlot != -1) {
        MinePoint_SetArea(w->mineSlot, em->area);
        EmShenGaoren_GetMinePointPos(em, pos);
        MinePoint_SetPos(w->mineSlot, pos);
    }
}

/*
 * Callback slots 0 and 1: spawns effect 01 of type 3 on the monster
 * (Shadow_Spawn).
 */
void EmShenGaoren_SpawnEffect01(u8 *em)
{
    Shadow_Spawn((Player *) em, 0x3);
}

/*
 * Writes the carving point position: bone 0xE position plus a fixed offset
 * (below and behind) rotated by the bone matrix.
 */
void EmShenGaoren_GetMinePointPos(Em *em, f32 *out)
{
    f32 ofs[3];
    f32 rotOfs[3];
    f32 bonePos[3];

    ofs[1] = -50.0f;
    ofs[2] = -150.0f;
    ofs[0] = 0.0f;
    Em_BonePos(em, 0xE, bonePos);
    Vec_Rotate((f32 *) rotOfs, (f32 *) ofs, (f32 *) (Em_Bone(em, 0xE)));
    out[0] = bonePos[0] + rotOfs[0];
    out[1] = bonePos[1] + rotOfs[1];
    out[2] = bonePos[2] + rotOfs[2];
}

/*
 * Makes the untimed carve mine point (radius 460, table by the quest tier,
 * count mineCount) at EmShenGaoren_GetMinePointPos and stores it in
 * mineSlot; the second argument is unused.
 */
void EmShenGaoren_AddMinePoint(Em *em, s32 unused)
{
    EmShenGaorenWork *w = (EmShenGaorenWork *)&em->work;
    MinePoint point;
    f32 pos[3];

    if (w->mineCount <= 0) {
        w->mineSlot = -1;
    }
    point.radius = 460.0f;
    switch (gQuest.header->tier) {
    default:
        point.table = 0x60;
        break;
    case 1:
        point.table = 0xDC;
        break;
    case 2:
        point.table = 0x159;
        break;
    case 3:
        point.table = 0x159;
        break;
    }
    point.action = MINE_ACTION_CARVE;
    point.count = w->mineCount;
    point.area = (u8) em->area;
    point.flags = 1;
    point.timer = 0;
    EmShenGaoren_GetMinePointPos(em, pos);
    w->mineSlot = MinePoint_AddUntimed(&point, pos, 0);
}

/*
 * Removes the mine points held in partFlinch entries 0 to 2 and sets them
 * to -1; a copy of the Lao-Shan function (whose bytes there are mine
 * points), see NOTE.
 */
void EmShenGaoren_RemoveMinePoints(Em *em)
{
    EmShenGaorenWork *w = (EmShenGaorenWork *)&em->work;
    s32 i;
    s8 *slot;

    for (i = 0; i < 3; i++) {
        slot = &w->partFlinch[i];
        if (*slot != -1) {
            MinePoint_Remove(*slot);
            *slot = -1;
        }
    }
}

/*
 * Places the monster by area on the first quest stage, sets hp from
 * Em_ComputeMaxHp, starts low in SHEN_IDLE_LOW (AREA_BATTLE_TOWN_1) or tall
 * in SHEN_IDLE_TALL, sets the work area (syncTimer 150, ease 1.0, mineCount
 * 3) and adds the mine point.
 */
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
        em->stance = SHEN_STANCE_LOW;
        Em_SetMotionRow(em, SHEN_MOT_IDLE_LOW, 0, 0);
        Em_SetStateRand(em, 0, 2);
    } else {
        em->stance = SHEN_STANCE_TALL;
        Em_SetMotionRow(em, SHEN_MOT_IDLE_TALL, 0, 0);
        Em_SetStateRand(em, 0, 1);
    }
    w->unk00C = 1500.0f;
    w->unk022 = 0;
    w->stage = 0;
    w->syncTimer = 0x96;
    w->ease[0] = 1.0f;
    w->ease[1] = 1.0f;
    w->ease[2] = 1.0f;
    w->ease[3] = 1.0f;
    w->mineCount = 3;
    EmShenGaoren_AddMinePoint(em, 0x96);
}

/*
 * Ends a state: resets motion speed and look-at, asks for the script, and
 * goes to SHEN_IDLE_LOW in SHEN_STANCE_LOW (SHEN_STANCE_LOW) or SHEN_IDLE_TALL in
 * SHEN_STANCE_TALL (SHEN_STANCE_TALL).
 */
void EmShenGaoren_EndState(Em *em)
{
    em->motionSpeed = 1.0f;
    em->lookAtTarget = 0;
    em->scriptPending = 1;
    if (em->stance == SHEN_STANCE_LOW) {
        em->stance = SHEN_STANCE_LOW;
        EmShenGaoren_ChangeState(em, EM_GROUP_IDLE, SHEN_IDLE_LOW, EM_SYNC_LOCAL);
        return;
    }
    em->stance = SHEN_STANCE_TALL;
    EmShenGaoren_ChangeState(em, EM_GROUP_IDLE, SHEN_IDLE_TALL, EM_SYNC_LOCAL);
}

/*
 * SHEN_IDLE_HOLD: unless debug switch 0x4D is on, keeps SHEN_MOT_IDLE_TALL
 * playing; never ends. It sets the motion again whenever a layer's id
 * differs from the idle's; the ids it expects on layers 1 and 2 are 1101 and
 * 1201, as if the layers' ids were 100 apart (EM_MOTION_ID puts them 200
 * apart), so once SHEN_IDLE_TALL has set the layers the tests fail and the
 * idle restarts every frame.
 */
void EmShenGaoren_IdleHold(Em *em)
{
    if (Debug_IsOn(0x4D) == 0) {
        if (em->motionId[0] != EM_MOTION_ID(0, SHEN_MOT_IDLE_TALL)) {
            Em_SetMotion(em, SHEN_MOT_IDLE_TALL, 0xA, 0);
        }
        if (em->motionId[1] != 1101) {
            Em_SetMotion(em, SHEN_MOT_IDLE_TALL, 0xA, 0);
        }
        if (em->motionId[2] != 1201) {
            Em_SetMotion(em, SHEN_MOT_IDLE_TALL, 0xA, 0);
        }
    }
}

/*
 * SHEN_IDLE_TALL: SHEN_STANCE_TALL, plays SHEN_MOT_IDLE_TALL on layers 0 to 2
 * unless already playing, and ends (EmShenGaoren_EndState) when the layer 0
 * motion ends.
 */
void EmShenGaoren_IdleTall(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = SHEN_STANCE_TALL;
        if (em->motionId[0] != EM_MOTION_ID(0, SHEN_MOT_IDLE_TALL)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(0, SHEN_MOT_IDLE_TALL), 0xA, 0, 0);
        }
        if (em->motionId[1] != EM_MOTION_ID(1, SHEN_MOT_IDLE_TALL)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(1, SHEN_MOT_IDLE_TALL), 0xA, 0, 1);
        }
        if (em->motionId[2] != EM_MOTION_ID(2, SHEN_MOT_IDLE_TALL)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(2, SHEN_MOT_IDLE_TALL), 0xA, 0, 2);
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

/*
 * SHEN_IDLE_LOW: SHEN_STANCE_LOW, plays SHEN_MOT_IDLE_LOW (blended in when coming
 * from the low shot motions) and ends when it ends.
 */
void EmShenGaoren_IdleLow(Em *em)
{
    s32 curMotion;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = SHEN_STANCE_LOW;
        curMotion = em->motionId[0];
        if (curMotion == EM_MOTION_ID(0, SHEN_MOT_AIM_LOW) || curMotion == EM_MOTION_ID(0, SHEN_MOT_FIRE_LOW) || curMotion == EM_MOTION_ID(0, SHEN_MOT_SHOT_END_LOW)) {
            Em_SetMotion(em, SHEN_MOT_IDLE_LOW, 0xC, 0);
        } else {
            Em_SetMotion(em, SHEN_MOT_IDLE_LOW, 0, 0);
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

/*
 * SHEN_IDLE_LOWER: SHEN_STANCE_LOW, plays SHEN_MOT_LOWER guarded and without
 * reactions, then ends.
 */
void EmShenGaoren_Lower(Em *em)
{
    em->guardTimer = 5;
    em->noReact = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = SHEN_STANCE_LOW;
        Em_SetMotion(em, SHEN_MOT_LOWER, 0, 0);
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

/*
 * SHEN_IDLE_RISE_A: SHEN_STANCE_TALL, plays SHEN_MOT_RISE guarded and without
 * reactions, then ends.
 */
void EmShenGaoren_RiseA(Em *em)
{
    em->guardTimer = 5;
    em->noReact = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = SHEN_STANCE_TALL;
        Em_SetMotion(em, SHEN_MOT_RISE, 0, 0);
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

/*
 * SHEN_IDLE_RISE_B: SHEN_STANCE_LOW, plays SHEN_MOT_SHOT_END_TALL guarded and
 * without reactions, then sets SHEN_STANCE_TALL and ends.
 */
void EmShenGaoren_RiseB(Em *em)
{
    em->guardTimer = 5;
    em->noReact = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = SHEN_STANCE_LOW;
        Em_SetMotion(em, SHEN_MOT_SHOT_END_TALL, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->guardTimer = 0;
            em->noReact = 0;
            em->stance = SHEN_STANCE_TALL;
            EmShenGaoren_EndState(em);
        }
        break;
    }
}

/* SHEN_IDLE_TALL_BRIEF: as SHEN_IDLE_TALL, but ends after 30 frames at most. */
void EmShenGaoren_IdleTallBrief(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = SHEN_STANCE_TALL;
        if (em->motionId[0] != EM_MOTION_ID(0, SHEN_MOT_IDLE_TALL)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(0, SHEN_MOT_IDLE_TALL), 0xA, 0, 0);
        }
        if (em->motionId[1] != EM_MOTION_ID(1, SHEN_MOT_IDLE_TALL)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(1, SHEN_MOT_IDLE_TALL), 0xA, 0, 1);
        }
        if (em->motionId[2] != EM_MOTION_ID(2, SHEN_MOT_IDLE_TALL)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(2, SHEN_MOT_IDLE_TALL), 0xA, 0, 2);
        }
        em->timer = 0x1E;
        break;
    case 1:
        if (em->motion[0].playing == 0 || --em->timer <= 0) {
            em->step++;
            EmShenGaoren_EndState(em);
        }
        break;
    }
}

/*
 * SHEN_IDLE_TRAMPLE_BRIEF: SHEN_STANCE_LOW, plays SHEN_MOT_TRAMPLE_TALL and ends
 * after 30 frames at most.
 */
void EmShenGaoren_TrampleBrief(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = SHEN_STANCE_LOW;
        Em_SetMotion(em, SHEN_MOT_TRAMPLE_TALL, 0, 0);
        em->timer = 0x1E;
        break;
    case 1:
        if (em->motion[0].playing == 0 || --em->timer <= 0) {
            em->step++;
            EmShenGaoren_EndState(em);
        }
        break;
    }
}

/* Runs the idle state in em->state. */
void EmShenGaoren_RunIdleState(Em *em)
{
    switch (em->state) {
    case SHEN_IDLE_HOLD:
        EmShenGaoren_IdleHold(em);
        break;
    case SHEN_IDLE_TALL:
        EmShenGaoren_IdleTall(em);
        break;
    case SHEN_IDLE_LOW:
        EmShenGaoren_IdleLow(em);
        break;
    case SHEN_IDLE_LOWER:
        EmShenGaoren_Lower(em);
        break;
    case SHEN_IDLE_RISE_A:
        EmShenGaoren_RiseA(em);
        break;
    case SHEN_IDLE_RISE_B:
        EmShenGaoren_RiseB(em);
        break;
    case SHEN_IDLE_TALL_BRIEF:
        EmShenGaoren_IdleTallBrief(em);
        break;
    case SHEN_IDLE_TRAMPLE_BRIEF:
        EmShenGaoren_TrampleBrief(em);

    default:
        break;
    }
}

/*
 * SHEN_MOVE_WALK_TALL: SHEN_STANCE_TALL, plays SHEN_MOT_WALK_TALL, turns toward the
 * goal by 0x40 a frame when it has one, and ends when the forward travel of
 * the motion uses up goalDist.
 */
void EmShenGaoren_WalkTall(Em *em)
{
    f32 pos[3];
    s32 turn;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = SHEN_STANCE_TALL;
        Em_SetMotion(em, SHEN_MOT_WALK_TALL, 0, 0);
        em->speedBonus = 0x1E;
        break;
    case 1:
        if (em->hasGoal != 0) {
            turn = ((Math_AngleXZ(&em->posX, em->goal) & 0xFFFF) - em->rotY) & 0xFFFF;
            if (turn <= 0x8000) {
                if (turn <= 0x3F) {
                    em->rotY += turn;
                } else {
                    em->rotY += 0x40;
                }
            } else {
                if (turn > 0xFFC0) {
                    em->rotY += turn;
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

/*
 * SHEN_MOVE_BACK_TALL: SHEN_STANCE_TALL, plays SHEN_MOT_WALK_BACK_TALL with its
 * back turned toward the goal and ends when the backward travel covers
 * goalDist.
 */
void EmShenGaoren_WalkBackTall(Em *em)
{
    f32 pos[3];
    s32 turn;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = SHEN_STANCE_TALL;
        Em_SetMotion(em, SHEN_MOT_WALK_BACK_TALL, 0, 0);
        em->speedBonus = 0x1E;
        break;
    case 1:
        if (em->hasGoal != 0) {
            turn = ((((Math_AngleXZ(&em->posX, em->goal) & 0xFFFF) + 0x8000) & 0xFFFF) - em->rotY) & 0xFFFF;
            if (turn <= 0x8000) {
                if (turn <= 0x3F) {
                    em->rotY += turn;
                } else {
                    em->rotY += 0x40;
                }
            } else {
                if (turn > 0xFFC0) {
                    em->rotY += turn;
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

/*
 * SHEN_MOVE_WALK_LOW: as SHEN_MOVE_WALK_TALL in SHEN_STANCE_LOW with
 * SHEN_MOT_WALK_LOW.
 */
void EmShenGaoren_WalkLow(Em *em)
{
    f32 pos[3];
    s32 turn;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = SHEN_STANCE_LOW;
        Em_SetMotion(em, SHEN_MOT_WALK_LOW, 0, 0);
        em->speedBonus = 0x1E;
        break;
    case 1:
        if (em->hasGoal != 0) {
            turn = ((Math_AngleXZ(&em->posX, em->goal) & 0xFFFF) - em->rotY) & 0xFFFF;
            if (turn <= 0x8000) {
                if (turn <= 0x3F) {
                    em->rotY += turn;
                } else {
                    em->rotY += 0x40;
                }
            } else {
                if (turn > 0xFFC0) {
                    em->rotY += turn;
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

/*
 * SHEN_MOVE_BACK_LOW: as SHEN_MOVE_BACK_TALL in SHEN_STANCE_LOW with
 * SHEN_MOT_WALK_BACK_LOW.
 */
void EmShenGaoren_WalkBackLow(Em *em)
{
    f32 pos[3];
    s32 turn;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = SHEN_STANCE_LOW;
        Em_SetMotion(em, SHEN_MOT_WALK_BACK_LOW, 0, 0);
        em->speedBonus = 0x1E;
        break;
    case 1:
        if (em->hasGoal != 0) {
            turn = ((((Math_AngleXZ(&em->posX, em->goal) & 0xFFFF) + 0x8000) & 0xFFFF) - em->rotY) & 0xFFFF;
            if (turn <= 0x8000) {
                if (turn <= 0x3F) {
                    em->rotY += turn;
                } else {
                    em->rotY += 0x40;
                }
            } else {
                if (turn > 0xFFC0) {
                    em->rotY += turn;
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

/*
 * SHEN_MOVE_TURN_LOW: SHEN_STANCE_LOW, turns toward the target with
 * EmShenGaoren_TurnParams row 1 (Em_StartTurnToTarget, Em_TurnStep), then
 * ends.
 */
void EmShenGaoren_TurnLow(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = SHEN_STANCE_LOW;
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

/*
 * SHEN_MOVE_TURN_TALL: SHEN_STANCE_TALL, turns toward the target with TurnParams
 * row 0, then ends.
 */
void EmShenGaoren_TurnTall(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = SHEN_STANCE_TALL;
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

/*
 * SHEN_MOVE_TURN_AWAY_TALL: SHEN_STANCE_TALL, turns its back to the target
 * (Em_StartTurnAway) with TurnParams row 0, then ends.
 */
void EmShenGaoren_TurnAwayTall(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = SHEN_STANCE_TALL;
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

/*
 * SHEN_MOVE_TURN_AWAY_LOW: SHEN_STANCE_LOW, turns its back to the target with
 * TurnParams row 1, then ends.
 */
void EmShenGaoren_TurnAwayLow(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = SHEN_STANCE_LOW;
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

/* Runs the move state in em->state. */
void EmShenGaoren_RunMoveState(Em *em)
{
    switch (em->state) {
    case SHEN_MOVE_WALK_TALL:
        EmShenGaoren_WalkTall(em);
        break;
    case SHEN_MOVE_BACK_TALL:
        EmShenGaoren_WalkBackTall(em);
        break;
    case SHEN_MOVE_WALK_LOW:
        EmShenGaoren_WalkLow(em);
        break;
    case SHEN_MOVE_BACK_LOW:
        EmShenGaoren_WalkBackLow(em);
        break;
    case SHEN_MOVE_TURN_LOW:
        EmShenGaoren_TurnLow(em);
        break;
    case SHEN_MOVE_TURN_TALL:
        EmShenGaoren_TurnTall(em);
        break;
    case SHEN_MOVE_TURN_AWAY_TALL:
        EmShenGaoren_TurnAwayTall(em);
        break;
    case SHEN_MOVE_TURN_AWAY_LOW:
        EmShenGaoren_TurnAwayLow(em);

    default:
        break;
    }
}

/* The empty fly dispatcher (EM_GROUP_FLY): the family has no fly states. */
void EmShenGaoren_RunFlyState(void)
{
}

/*
 * SHEN_ATK_SHOT_TALL: SHEN_STANCE_TALL, aims (SHEN_MOT_AIM_TALL) for 300 frames,
 * fires an EMFX_SHEN_SHOT at frame 4 of SHEN_MOT_FIRE_TALL, then plays
 * SHEN_MOT_SHOT_END_TALL and ends.
 */
void EmShenGaoren_ShotTall(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = SHEN_STANCE_TALL;
        em->timer = 0x12C;
        Em_SetMotion(em, SHEN_MOT_AIM_TALL, 0, 0);
        break;
    case 1:
        if (--em->timer < 0) {
            em->step++;
            em->timer = 1;
            Em_SetMotion(em, SHEN_MOT_FIRE_TALL, 0, 0);
        }
        break;
    case 2:
        if (EmMotion_PassedFrameScaled(em, 4.0f, 0U) != 0) {
            EmEffectAttack_Create(em, EMFX_SHEN_SHOT, 0, 0xF800, 0x8000);
        }
        if (em->motion[0].playing == 0) {
            if (--em->timer <= 0) {
                em->step++;
                Em_SetMotion(em, SHEN_MOT_SHOT_END_TALL, 0, 0);
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

/*
 * SHEN_ATK_SHOT_FORT, SHEN_ATK_SHOT_TOWN_A, SHEN_ATK_SHOT_TOWN_B: SHEN_STANCE_LOW,
 * aims 300 frames (SHEN_MOT_AIM_LOW) and fires (SHEN_MOT_FIRE_LOW): variant
 * 0 sends the shot away to land in AREA_FORTRESS_5 (or fires it locally
 * there) and counts a hit on the fort (fortHits, gScene.fortStage,
 * Quest_TriggerEvent at 10); variants 1 and 2 land it in AREA_BATTLE_TOWN_2
 * or _3. Ends after SHEN_MOT_SHOT_END_LOW.
 */
void EmShenGaoren_ShotLow(Em *em, s32 variant)
{
    EmShenGaorenWork *w = (EmShenGaorenWork *)&em->work;

    s8 hits;
    u16 area;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = SHEN_STANCE_LOW;
        em->timer = 0x12C;
        Em_SetMotion(em, SHEN_MOT_AIM_LOW, 0, 0);
        break;
    case 1:
        if (--em->timer < 0) {
            em->step++;
            em->timer = 1;
            Em_SetMotion(em, SHEN_MOT_FIRE_LOW, 0, 0);
        }
        break;
    case 2:
        if (EmMotion_PassedFrameScaled(em, 4.0f, 0U) != 0) {
            switch (variant) {
            case 0:
                area = em->area;
                if (area != (Area_IdForCurrentTime(AREA_FORTRESS_5) & 0xFFFF)) {
                    EmEffectAttack_Create(em, EMFX_SHEN_SHOT_AWAY, 0, 0xE800, 0x8000);
                    EmEffectAttack_CreateShengaoren(em, EMFX_SHEN_SHOT_DROP, AREA_FORTRESS_5, 0x6000, 0);
                } else {
                    EmEffectAttack_Create(em, EMFX_SHEN_SHOT, 0, 0xF800, 0x8000);
                }
                if (em->remote == 0) {
                    hits = w->fortHits;
                    if (hits < 0xA) {
                        w->fortHits = hits + 1;
                        if (w->fortHits >= 0xA) {
                            Quest_TriggerEvent(em);
                        }
                        gScene.fortStage = w->fortHits;
                    }
                }
                break;
            case 1:
                EmEffectAttack_Create(em, EMFX_SHEN_SHOT_AWAY, 0, 0xE800, 0x8000);
                EmEffectAttack_CreateShengaoren(em, EMFX_SHEN_SHOT_DROP, AREA_BATTLE_TOWN_2, 0x6000, 0);
                break;
            case 2:
                EmEffectAttack_Create(em, EMFX_SHEN_SHOT_AWAY, 0, 0xE800, 0x8000);
                EmEffectAttack_CreateShengaoren(em, EMFX_SHEN_SHOT_DROP, AREA_BATTLE_TOWN_3, 0x6000, 0);
                break;
            }
        }
        if (em->motion[0].playing == 0) {
            if (--em->timer <= 0) {
                em->step++;
                Em_SetMotion(em, SHEN_MOT_SHOT_END_LOW, 0, 0);
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

/*
 * SHEN_ATK_STRIKE_FORT: SHEN_STANCE_LOW, plays SHEN_MOT_STRIKE_FORT; at frame 200
 * counts a hit on the fort and makes the area impact
 * (EmShenGaoren_SpawnAreaImpactEffects, high), then ends.
 */
void EmShenGaoren_StrikeFort(Em *em)
{
    EmShenGaorenWork *w = (EmShenGaorenWork *)&em->work;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = SHEN_STANCE_LOW;
        Em_SetMotion(em, SHEN_MOT_STRIKE_FORT, 0, 0);
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

/*
 * SHEN_ATK_SMASH, SHEN_ATK_SMASH_FORT: SHEN_STANCE_TALL, plays SHEN_MOT_SMASH; at
 * frame 134 variant 1 counts a hit on the fort, variant 0 sets
 * gScene.emEventFlags bit 0 (and scriptStage 8 in AREA_FORTRESS_3); both
 * make the low area impact, then it ends.
 */
void EmShenGaoren_Smash(Em *em, s32 variant)
{
    EmShenGaorenWork *w = (EmShenGaorenWork *)&em->work;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = SHEN_STANCE_TALL;
        Em_SetMotion(em, SHEN_MOT_SMASH, 0, 0);
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
                if (em->remote == 0 && (Area_DayId(em->area) & 0xFFFF) == AREA_FORTRESS_3 && w->stage <= 7) {
                    w->stage = 8;
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

/* SHEN_ATK_STRIKE_A: SHEN_STANCE_TALL, plays SHEN_MOT_STRIKE_A and ends. */
void EmShenGaoren_StrikeA(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = SHEN_STANCE_TALL;
        Em_SetMotion(em, SHEN_MOT_STRIKE_A, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmShenGaoren_EndState(em);
        }
        break;
    }
}

/* SHEN_ATK_STRIKE_B: SHEN_STANCE_TALL, plays SHEN_MOT_STRIKE_B and ends. */
void EmShenGaoren_StrikeB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = SHEN_STANCE_TALL;
        Em_SetMotion(em, SHEN_MOT_STRIKE_B, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmShenGaoren_EndState(em);
        }
        break;
    }
}

/* SHEN_ATK_TRAMPLE_TALL: SHEN_STANCE_TALL, plays SHEN_MOT_TRAMPLE_TALL and ends. */
void EmShenGaoren_TrampleTall(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = SHEN_STANCE_TALL;
        Em_SetMotion(em, SHEN_MOT_TRAMPLE_TALL, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmShenGaoren_EndState(em);
        }
        break;
    }
}

/* SHEN_ATK_TRAMPLE_LOW: SHEN_STANCE_LOW, plays SHEN_MOT_TRAMPLE_LOW and ends. */
void EmShenGaoren_TrampleLow(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = SHEN_STANCE_LOW;
        Em_SetMotion(em, SHEN_MOT_TRAMPLE_LOW, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmShenGaoren_EndState(em);
        }
        break;
    }
}

/* Runs the attack state in em->state, passing the shot and smash variants. */
void EmShenGaoren_RunAttackState(Em *em)
{
    switch (em->state) {
    case SHEN_ATK_SHOT_TALL:
        EmShenGaoren_ShotTall(em);
        break;
    case SHEN_ATK_SHOT_FORT:
        EmShenGaoren_ShotLow(em, 0);
        break;
    case SHEN_ATK_STRIKE_FORT:
        EmShenGaoren_StrikeFort(em);
        break;
    case SHEN_ATK_SMASH:
        EmShenGaoren_Smash(em, 0);
        break;
    case SHEN_ATK_STRIKE_A:
        EmShenGaoren_StrikeA(em);
        break;
    case SHEN_ATK_STRIKE_B:
        EmShenGaoren_StrikeB(em);
        break;
    case SHEN_ATK_TRAMPLE_TALL:
        EmShenGaoren_TrampleTall(em);
        break;
    case SHEN_ATK_TRAMPLE_LOW:
        EmShenGaoren_TrampleLow(em);
        break;
    case SHEN_ATK_SMASH_FORT:
        EmShenGaoren_Smash(em, 1);
        break;
    case SHEN_ATK_SHOT_TOWN_A:
        EmShenGaoren_ShotLow(em, 1);
        break;
    case SHEN_ATK_SHOT_TOWN_B:
        EmShenGaoren_ShotLow(em, 2);

    default:
        break;
    }
}

/*
 * SHEN_REACT_FLINCH_TALL, SHEN_REACT_LEG_A_TALL to SHEN_REACT_LEG_D_TALL:
 * SHEN_STANCE_TALL, plays SHEN_MOT_FLINCH_TALL, restarts the script and ends.
 */
void EmShenGaoren_FlinchTall(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = SHEN_STANCE_TALL;
        Em_SetMotion(em, SHEN_MOT_FLINCH_TALL, 0, 0);
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

/*
 * SHEN_REACT_FLINCH_LOW: SHEN_STANCE_LOW, plays SHEN_MOT_FLINCH_LOW, restarts the
 * script and ends.
 */
void EmShenGaoren_FlinchLow(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = SHEN_STANCE_LOW;
        Em_SetMotion(em, SHEN_MOT_FLINCH_LOW, 0, 0);
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

/*
 * SHEN_REACT_LEG_A_LOW to SHEN_REACT_LEG_D_LOW, SHEN_REACT_LEG_A_AGAIN_LOW
 * to SHEN_REACT_LEG_D_AGAIN_LOW: SHEN_STANCE_LOW, plays the leg flinch motion
 * (SHEN_MOT_FLINCH_LEG_C plus the variant), restarts the script and ends.
 */
void EmShenGaoren_FlinchLegLow(Em *em, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = SHEN_STANCE_LOW;
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

/*
 * SHEN_REACT_LEGS_COLLAPSE, SHEN_REACT_KNOCKDOWN_LOW: SHEN_STANCE_LOW, falls
 * (SHEN_MOT_COLLAPSE), lies 300 frames in SHEN_STANCE_TALL (SHEN_MOT_DOWN_LOW),
 * gets up (SHEN_MOT_GET_UP_LOW) and ends.
 */
void EmShenGaoren_CollapseLow(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = SHEN_STANCE_LOW;
        Em_SetMotion(em, SHEN_MOT_COLLAPSE, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 0x12C;
            em->stance = SHEN_STANCE_TALL;
            Em_SetMotion(em, SHEN_MOT_DOWN_LOW, 0, 0);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            em->stance = SHEN_STANCE_TALL;
            Em_SetMotion(em, SHEN_MOT_GET_UP_LOW, 0, 0);
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

/*
 * SHEN_REACT_KNOCKDOWN_TALL, SHEN_REACT_LEG_A_DOWN_TALL to
 * SHEN_REACT_LEG_D_DOWN_TALL: SHEN_STANCE_TALL, falls (SHEN_MOT_FALL_TALL), lies
 * 300 frames (SHEN_MOT_DOWN_TALL), gets up (SHEN_MOT_GET_UP_TALL) and ends.
 */
void EmShenGaoren_KnockdownTall(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = SHEN_STANCE_TALL;
        Em_SetMotion(em, SHEN_MOT_FALL_TALL, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 0x12C;
            Em_SetMotion(em, SHEN_MOT_DOWN_TALL, 0, 0);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, SHEN_MOT_GET_UP_TALL, 0, 0);
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

/*
 * SHEN_REACT_LEGS_COLLAPSE_SHOT: SHEN_STANCE_LOW, plays SHEN_MOT_FLINCH_SHOT_LOW,
 * then SHEN_MOT_COLLAPSE from frame 82 in SHEN_STANCE_TALL, and ends without lying
 * down.
 */
void EmShenGaoren_CollapseFromShot(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = SHEN_STANCE_LOW;
        Em_SetMotion(em, SHEN_MOT_FLINCH_SHOT_LOW, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stance = SHEN_STANCE_TALL;
            Em_SetMotion(em, SHEN_MOT_COLLAPSE, 0, 0x52);
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

/*
 * Runs the react state in em->state; the leg flinches pass State4_02 the
 * leg motion variant.
 */
void EmShenGaoren_RunReactState(Em *em)
{
    switch (em->state) {
    case SHEN_REACT_FLINCH_TALL:
        EmShenGaoren_FlinchTall(em);
        break;
    case SHEN_REACT_FLINCH_LOW:
        EmShenGaoren_FlinchLow(em);
        break;
    case SHEN_REACT_LEG_A_LOW:
        EmShenGaoren_FlinchLegLow(em, 1);
        break;
    case SHEN_REACT_LEG_B_LOW:
        EmShenGaoren_FlinchLegLow(em, 3);
        break;
    case SHEN_REACT_LEG_C_LOW:
        EmShenGaoren_FlinchLegLow(em, 0);
        break;
    case SHEN_REACT_LEG_D_LOW:
        EmShenGaoren_FlinchLegLow(em, 2);
        break;
    case SHEN_REACT_LEGS_COLLAPSE:
        EmShenGaoren_CollapseLow(em);
        break;
    case SHEN_REACT_KNOCKDOWN_TALL:
        EmShenGaoren_KnockdownTall(em);
        break;
    case SHEN_REACT_LEGS_COLLAPSE_SHOT:
        EmShenGaoren_CollapseFromShot(em);
        break;
    case SHEN_REACT_LEG_A_TALL:
        EmShenGaoren_FlinchTall(em);
        break;
    case SHEN_REACT_LEG_B_TALL:
        EmShenGaoren_FlinchTall(em);
        break;
    case SHEN_REACT_LEG_C_TALL:
        EmShenGaoren_FlinchTall(em);
        break;
    case SHEN_REACT_LEG_D_TALL:
        EmShenGaoren_FlinchTall(em);
        break;
    case SHEN_REACT_LEG_A_DOWN_TALL:
        EmShenGaoren_KnockdownTall(em);
        break;
    case SHEN_REACT_LEG_B_DOWN_TALL:
        EmShenGaoren_KnockdownTall(em);
        break;
    case SHEN_REACT_LEG_C_DOWN_TALL:
        EmShenGaoren_KnockdownTall(em);
        break;
    case SHEN_REACT_LEG_D_DOWN_TALL:
        EmShenGaoren_KnockdownTall(em);
        break;
    case SHEN_REACT_LEG_A_AGAIN_LOW:
        EmShenGaoren_FlinchLegLow(em, 1);
        break;
    case SHEN_REACT_LEG_B_AGAIN_LOW:
        EmShenGaoren_FlinchLegLow(em, 3);
        break;
    case SHEN_REACT_LEG_C_AGAIN_LOW:
        EmShenGaoren_FlinchLegLow(em, 0);
        break;
    case SHEN_REACT_LEG_D_AGAIN_LOW:
        EmShenGaoren_FlinchLegLow(em, 2);
        break;
    case SHEN_REACT_KNOCKDOWN_LOW:
        EmShenGaoren_CollapseLow(em);

    default:
        break;
    }
}

/*
 * SHEN_DEFEAT_TALL: reports the hunt (QuestEm_Hunted, CamReq_BossHunted),
 * plays SHEN_MOT_DIE_TALL then SHEN_MOT_DIE_LOW from frame 200, then
 * becomes the carcass (stance 3, EmMine_Create) and runs EmMine_Update;
 * step 99 revives it (see NOTE).
 */
void EmShenGaoren_DieTall(Em *em)
{
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, SHEN_MOT_DIE_TALL, 0, 0);
        em->timer = 0;
        em->lookAtTarget = 0;
        em->stance = SHEN_STANCE_TALL;
        QuestEm_Hunted(em);
        CamReq_BossHunted();
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, SHEN_MOT_DIE_LOW, 0, 0xC8);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stance = EM_STANCE_DEAD;
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

/*
 * SHEN_DEFEAT_LOW: as SHEN_DEFEAT_TALL from SHEN_STANCE_LOW, playing only
 * SHEN_MOT_DIE_LOW before the carcass.
 */
void EmShenGaoren_DieLow(Em *em)
{
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = SHEN_STANCE_LOW;
        Em_SetMotion(em, SHEN_MOT_DIE_LOW, 0, 0);
        QuestEm_Hunted(em);
        CamReq_BossHunted();
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stance = EM_STANCE_DEAD;
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

/* Runs the defeat state in em->state. */
void EmShenGaoren_RunDefeatState(Em *em)
{
    switch (em->state) {
    case SHEN_DEFEAT_TALL:
        EmShenGaoren_DieTall(em);
        break;
    case SHEN_DEFEAT_LOW:
        EmShenGaoren_DieLow(em);
        break;
    }
}

/*
 * SHEN_EVENT_CUTSCENE_SHOT: unhittable, walks backward along two fixed legs
 * (SHEN_MOT_WALK_BACK_TALL), turns its back to a fixed spot, aims 300
 * frames, fires an EMFX_SHEN_SHOT, idles (SHEN_MOT_IDLE_TALL) and ends once
 * EVENT_FLAG_SHEN_GAOREN_SCENE is set.
 */
void EmShenGaoren_EventCutsceneShot(Em *em)
{
    f32 pos[3];

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = SHEN_STANCE_TALL;
        Em_SetMotion(em, SHEN_MOT_WALK_BACK_TALL, 0, 0);
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
            Em_SetMotion(em, SHEN_MOT_AIM_TALL, 0, 0);
        }
        break;
    case 4:
        if (--em->timer < 0) {
            em->step++;
            Em_SetMotion(em, SHEN_MOT_FIRE_TALL, 0, 0);
        }
        break;
    case 5:
        if (EmMotion_PassedFrameScaled(em, 4.0f, 0)) {
            EmEffectAttack_Create(em, EMFX_SHEN_SHOT, 0, 0xE800, 0x8000);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, SHEN_MOT_SHOT_END_TALL, 0, 0);
        }
        break;
    case 6:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, SHEN_MOT_IDLE_TALL, 0, 0);
        }
        break;
    case 7:
        if (Save_TestEventFlag(EVENT_FLAG_SHEN_GAOREN_SCENE) == 1) {
            em->step++;
            EmShenGaoren_EndState(em);
        }
        break;
    }
}

/*
 * SHEN_EVENT_LEAVE: unhittable, placed at a fixed spot, turns to a first
 * waypoint and walks (SHEN_MOT_WALK_TALL) to it and on to a second one,
 * then plays SHEN_MOT_WALK_BACK_TALL and stays in that step for good.
 */
void EmShenGaoren_EventLeave(Em *em)
{
    f32 pos[3];
    s32 turn;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = SHEN_STANCE_TALL;
        Em_SetMotion(em, SHEN_MOT_IDLE_TALL, 0, 0);
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
            Em_SetMotion(em, SHEN_MOT_WALK_TALL, 0, 0);
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
        turn = ((Math_AngleXZ(&em->posX, em->goal) & 0xFFFF) - em->rotY) & 0xFFFF;
        if (turn <= 0x8000) {
            if (turn <= 0x3F) {
                em->rotY += turn;
            } else {
                em->rotY += 0x40;
            }
        } else {
            if (turn > 0xFFC0) {
                em->rotY += turn;
            } else {
                em->rotY -= 0x40;
            }
        }
        Em_MotionDelta(em, pos);
        if ((em->goalDist -= pos[2]) <= 0.0f) {
            em->step++;
            Em_SetMotion(em, SHEN_MOT_WALK_BACK_TALL, 0, 0);
        }
        break;
    case 4:
        break;
    }
}

/* Runs the event state in em->state. */
void EmShenGaoren_RunEventState(Em *em)
{
    switch (em->state) {
    case SHEN_EVENT_CUTSCENE_SHOT:
        EmShenGaoren_EventCutsceneShot(em);
        break;
    case SHEN_EVENT_LEAVE:
        EmShenGaoren_EventLeave(em);
        break;
    }
}

/*
 * Per frame: keeps the mine point on the body and drops it when used up,
 * counts quakeTimer and syncTimer down (on the host, sending the state to the
 * other players every 150 frames), maps the Em_UpdateDamage result to a state, sets fxFlags from
 * part 2 state, starts the area events (SHEN_EVENT_CUTSCENE_SHOT,
 * SHEN_EVENT_LEAVE), runs the state (twice on a state request) and sets
 * quakeTimer to 5 unless dying or in SHEN_ATK_SHOT_TALL or
 * SHEN_ATK_SHOT_FORT.
 */
void EmShenGaoren_Update(Em *em)
{
    EmShenGaorenWork *w = (EmShenGaorenWork *)&em->work;
    f32 pos[3];
    u8 dmgOut[4];
    s32 legsSpent;
    s32 i;
    s32 part;

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
    if (w->syncTimer != 0 && em->stateGroup != EM_GROUP_EVENT && em->remote == 0) {
        if (--w->syncTimer <= 0) {
            w->syncTimer = 150;
            QSync_SendEm(em, QSYNC_EMMSG_STATE_NO_PARTS, 0);
        }
    }
    switch (Em_UpdateDamage(em, dmgOut) & 0xFF) {
    case EM_DMG_NONE:
    case EM_DMG_ATTACK6:
    case EM_DMG_STATUS_IN_AIR:
    case EM_DMG_STATUS4:
    case EM_DMG_CAPTURED:
    case EM_DMG_STATUS2:
    case EM_DMG_FLASH:
    case EM_DMG_HIT_LYING:
    case EM_DMG_PART8_DOWN:
    case EM_DMG_HIT_MODE3:
    case EM_DMG_HIT:
    case EM_DMG_ATTACK5:
    case EM_DMG_ATTACK5_FLINCH:
    case EM_DMG_ATTACK13:
        break;
    case EM_DMG_KILLED:
    case EM_DMG_DEAD:
        EmShenGaoren_ChangeState(em, EM_GROUP_DEFEAT, SHEN_DEFEAT_TALL, EM_SYNC_OWNER);
        break;
    case EM_DMG_ATTACK9:
        if (em->stance == SHEN_STANCE_TALL) {
            if (em->stateGroup != EM_GROUP_REACT || em->state != SHEN_REACT_KNOCKDOWN_TALL) {
                EmShenGaoren_ChangeState(em, EM_GROUP_REACT, SHEN_REACT_KNOCKDOWN_TALL, EM_SYNC_OWNER);
            }
        } else {
            if (em->stateGroup != EM_GROUP_REACT || em->state != SHEN_REACT_KNOCKDOWN_LOW) {
                EmShenGaoren_ChangeState(em, EM_GROUP_REACT, SHEN_REACT_KNOCKDOWN_LOW, EM_SYNC_OWNER);
            }
        }
        break;
    case EM_DMG_STATUS5:
        Em_EndStatus5(em);
        break;
    case EM_DMG_PART_FLINCH:
        legsSpent = 1;
        if (em->stance == SHEN_STANCE_TALL) {
            part = em->flinchPart;
            switch (part) {
            case 0:
            case 1:
                EmShenGaoren_ChangeState(em, EM_GROUP_REACT, SHEN_REACT_KNOCKDOWN_TALL, EM_SYNC_OWNER);
                break;
            case 3:
            case 4:
            case 5:
            case 6:
                if (w->partFlinch[part] > 0) {
                    EmShenGaoren_ChangeState(em, 4, part + 10, EM_SYNC_OWNER);
                } else {
                    EmShenGaoren_ChangeState(em, 4, part + 6, EM_SYNC_OWNER);
                }
                break;
            case 2:
            case 7:
                if (em->parts[part].state >= 2) {
                    EmShenGaoren_ChangeState(em, EM_GROUP_REACT, SHEN_REACT_KNOCKDOWN_TALL, EM_SYNC_OWNER);
                } else {
                    EmShenGaoren_ChangeState(em, EM_GROUP_REACT, SHEN_REACT_FLINCH_TALL, EM_SYNC_OWNER);
                }
                break;
            }
        } else {
            for (i = 0; i < 4; i++) {
                if (w->partFlinch[3 + i] <= 1) {
                    legsSpent = 0;
                }
            }
            if (legsSpent == 1) {
                if (em->stateGroup == EM_GROUP_ATTACK && em->state == SHEN_ATK_SHOT_FORT) {
                    EmShenGaoren_ChangeState(em, EM_GROUP_REACT, SHEN_REACT_LEGS_COLLAPSE_SHOT, EM_SYNC_OWNER);
                } else {
                    EmShenGaoren_ChangeState(em, EM_GROUP_REACT, SHEN_REACT_LEGS_COLLAPSE, EM_SYNC_OWNER);
                }
            } else {
                part = em->flinchPart;
                switch (part) {
                case 0:
                case 1:
                case 2:
                case 7:
                    EmShenGaoren_ChangeState(em, EM_GROUP_REACT, SHEN_REACT_FLINCH_LOW, EM_SYNC_OWNER);
                    break;
                case 3:
                case 4:
                case 5:
                case 6:
                    if (w->partFlinch[part] > 0) {
                        EmShenGaoren_ChangeState(em, 4, part + 14, EM_SYNC_OWNER);
                    } else {
                        EmShenGaoren_ChangeState(em, 4, part - 1, EM_SYNC_OWNER);
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
        if (Save_TestEventFlag(EVENT_FLAG_SHEN_GAOREN_SCENE) == 0) {
            if (Area_DayId(gScene.area) == AREA_FORTRESS_1 && gScene.eventRunning == 1 && em->stateGroup != EM_GROUP_EVENT) {
                EmShenGaoren_ChangeState(em, EM_GROUP_EVENT, SHEN_EVENT_CUTSCENE_SHOT, EM_SYNC_LOCAL);
            }
            /* until the scene has been seen, the script doesn't run here */
            goto skip;
        }
        break;
    case AREA_FORTRESS_5:
        if (Quest_GetOutcome(0) == 1 && em->stateGroup != EM_GROUP_EVENT && em->stateGroup != EM_GROUP_DEFEAT) {
            em->scriptPending = 0;
            if (gScene.area == em->area) {
                CamReq_ForMonster(em);
            }
            EmShenGaoren_ChangeState(em, EM_GROUP_EVENT, SHEN_EVENT_LEAVE, EM_SYNC_LOCAL);
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
    if (em->stateGroup == EM_GROUP_DEFEAT || (em->stateGroup == EM_GROUP_ATTACK && (em->state == 0 || em->state == 1))) {
        em->quakeTimer = 0;
    } else {
        em->quakeTimer = 5;
    }
}

/* Runs the state group in em->stateGroup (group 7 runs the event states). */
void EmShenGaoren_RunState(Em *em)
{
    switch (em->stateGroup) {
    case EM_GROUP_IDLE:
        EmShenGaoren_RunIdleState(em);
        break;
    case EM_GROUP_MOVE:
        EmShenGaoren_RunMoveState(em);
        break;
    case EM_GROUP_FLY:
        EmShenGaoren_RunFlyState();
        break;
    case EM_GROUP_ATTACK:
        EmShenGaoren_RunAttackState(em);
        break;
    case EM_GROUP_REACT:
        EmShenGaoren_RunReactState(em);
        break;
    case EM_GROUP_DEFEAT:
        EmShenGaoren_RunDefeatState(em);
        break;
    case EM_GROUP_EVENT:
        EmShenGaoren_RunEventState(em);
        break;
    case EM_GROUP_EVENT2:
        EmShenGaoren_RunEventState(em);

    default:
        break;
    }
}

/*
 * Eases the four ease values a 120th of the way a frame toward the lying row (1) of
 * EmShenGaoren_DownEaseTargets while fxMotion has it lying
 * (SHEN_MOT_FALL_TALL after frame 86, SHEN_MOT_DOWN_TALL,
 * SHEN_MOT_GET_UP_TALL before frame 94, and the low SHEN_MOT_COLLAPSE,
 * SHEN_MOT_DOWN_LOW, SHEN_MOT_GET_UP_LOW alike), else toward the standing row (0).
 */
void EmShenGaoren_EaseDownPose(Em *em, EmWork *work)
{
    f32 *target;
    EmShenGaorenWork *w = (EmShenGaorenWork *)work;
    s32 row = 0;

    switch (w->fxMotion) {
    case 0x3FE:
        if (EmMotion_ReachedFrame(em, 0, 86.0f)) {
            row = 1;
        }
        break;
    case 0x3FF:
        row = 1;
        break;
    case 0x400:
        if (!EmMotion_ReachedFrame(em, 0, 94.0f)) {
            row = 1;
        }
        break;
    case 0x41C:
        if (EmMotion_ReachedFrame(em, 0, 134.0f)) {
            row = 1;
        }
        break;
    case 0x41D:
        row = 1;
        break;
    case 0x41E:
        if (!EmMotion_ReachedFrame(em, 0, 94.0f)) {
            row = 1;
        }
        break;
    }
    target = EmShenGaoren_DownEaseTargets[row];
    w->ease[0] = w->ease[0] + (target[0] - w->ease[0]) / 120.0f;
    w->ease[1] = w->ease[1] + (target[1] - w->ease[1]) / 120.0f;
    w->ease[2] = w->ease[2] + (target[2] - w->ease[2]) / 120.0f;
    w->ease[3] = w->ease[3] + (target[3] - w->ease[3]) / 120.0f;
}

/*
 * If a player is in the area, spawns ground effect kind at the bone
 * position on the floor; flags bit 2 shakes the camera, bit 1 makes an
 * impact attack there and the leg stomp ShenGaorenAttack (kinds 0x12 to
 * 0x17 by bone).
 */
void EmShenGaoren_SpawnGroundEffect(Em *em, s32 bone, s32 kind, s32 flags, f32 scale)
{
    f32 pos[3];

    if (Player_InCurrentArea(em)) {
        Em_BonePos(em, bone, pos);
        pos[1] = em->floorY;
        PartFx_SpawnOnGround(pos, kind, scale * em->scaleX);
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

/*
 * If a player is in the area, makes an impact attack at the bone position
 * on the floor and shakes the camera.
 */
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

/* The default case of the motion effects: does nothing. */
static void EmShenGaoren_MotionEffectsDefault(Em *em)
{
}

/*
 * Tracks the motion in fxMotion (clearing unk020 on a new one), plays its
 * sounds, runs EmShenGaoren_EaseDownPose, then makes the motion effects on
 * their frames: leg stomps and ground effects, the ShenGaorenAttack hits,
 * dust clouds, the charge and muzzle effects.
 */
void EmShenGaoren_MotionEffects(Em *em, EmWork *work)
{
    f32 pos[3];
    f32 ofs[3];
    EmShenGaorenWork *w = (EmShenGaorenWork *)work;

    if (em->motionId[0] != w->fxMotion) {
        w->unk020 = 0;
        w->fxMotion = em->motionId[0];
    }
    EmShenGaoren_MotionSounds(em, work);
    EmShenGaoren_EaseDownPose(em, work);
    switch (w->fxMotion) {
    case EM_MOTION_ID(0, SHEN_MOT_WALK_TALL):
    case EM_MOTION_ID(0, SHEN_MOT_WALK_LOW):
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
    case EM_MOTION_ID(0, SHEN_MOT_WALK_BACK_TALL):
    case EM_MOTION_ID(0, SHEN_MOT_WALK_BACK_LOW):
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
    case EM_MOTION_ID(0, 0x4):
    case EM_MOTION_ID(0, 0x21):
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
    case EM_MOTION_ID(0, 0x5):
    case EM_MOTION_ID(0, 0x22):
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
    case EM_MOTION_ID(0, SHEN_MOT_LOWER):
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
    case EM_MOTION_ID(0, SHEN_MOT_STRIKE_A):
        if (EmMotion_PassedFrameScaled(em, 124.0f, 0) != 0) {
            ShenGaorenAttack_Create(em, 3);
        }
        if (EmMotion_PassedFrameScaled(em, 58.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0x1A, 7, 0, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 160.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 0xB, 0x26, em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, SHEN_MOT_STRIKE_B):
        if (EmMotion_PassedFrameScaled(em, 124.0f, 0) != 0) {
            ShenGaorenAttack_Create(em, 4);
        }
        if (EmMotion_PassedFrameScaled(em, 58.0f, 0) != 0) {
            EmShenGaoren_SpawnGroundEffect(em, 0x12, 7, 0, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 160.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 0xB, 0x27, em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, SHEN_MOT_TRAMPLE_TALL):
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
    case EM_MOTION_ID(0, SHEN_MOT_AIM_TALL):
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
            MonsterFx_CreateOnEm(em, 0x19, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, SHEN_MOT_FIRE_TALL):
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            MonsterFx_CreateOnEm(em, 0x1A, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, SHEN_MOT_SHOT_END_TALL):
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
    case EM_MOTION_ID(0, SHEN_MOT_SMASH):
        if (EmMotion_PassedFrameScaled(em, 120.0f, 0) != 0) {
            ShenGaorenAttack_Create(em, 2);
        }
        if (EmMotion_PassedFrameScaled(em, 144.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 0xB, 0x26, em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, 0x14):
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
    case EM_MOTION_ID(0, SHEN_MOT_FALL_TALL):
        if (EmMotion_PassedFrameScaled(em, 94.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 0, 0, em->scaleX);
        }
        if (EmMotion_PassedFrameScaled(em, 50.0f, 0) != 0) {
            ShenGaorenAttack_Create(em, 0x18);
            break;
        }
        break;
    case EM_MOTION_ID(0, SHEN_MOT_GET_UP_TALL):
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
    case EM_MOTION_ID(0, SHEN_MOT_DIE_TALL):
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
    case EM_MOTION_ID(0, SHEN_MOT_RISE):
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
    case EM_MOTION_ID(0, SHEN_MOT_STRIKE_FORT):
        if (EmMotion_PassedFrameScaled(em, 170.0f, 0) != 0) {
            ShenGaorenAttack_Create(em, 1);
        }
        if ((EmMotion_PassedFrameScaled(em, 204.0f, 0) != 0) && Player_InCurrentArea(em)) {
            ofs[0] = 0.0f;
            ofs[1] = 0.0f;
            ofs[2] = 1100.0f * em->scaleZ;
            Vec_RotateY(ofs, (6.2831855f * (f32) em->rotY) / 65536.0f);
            pos[0] = em->posX + ofs[0];
            pos[1] = em->floorY;
            pos[2] = em->posZ + ofs[2];
            MonsterFx_CreateAtYaw(pos, (u16) em->rotY, 0x11, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, SHEN_MOT_TRAMPLE_LOW):
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
            DustCloud_Create((Em *) em, 2, 0x26, em->scaleX);
        }
        if (EmMotion_PassedFrameScaled(em, 176.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 3, 0x27, em->scaleX);
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
    case EM_MOTION_ID(0, SHEN_MOT_AIM_LOW):
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
            MonsterFx_CreateOnEm(em, 0x19, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, SHEN_MOT_FIRE_LOW):
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            MonsterFx_CreateOnEm(em, 0x1A, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, SHEN_MOT_SHOT_END_LOW):
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
    case EM_MOTION_ID(0, 0x32):
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
    case EM_MOTION_ID(0, SHEN_MOT_COLLAPSE):
        if (EmMotion_PassedFrameScaled(em, 116.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 0, 0, em->scaleX);
        }
        if (EmMotion_PassedFrameScaled(em, 90.0f, 0) != 0) {
            ShenGaorenAttack_Create(em, 0x19);
            break;
        }
        break;
    case EM_MOTION_ID(0, SHEN_MOT_GET_UP_LOW):
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
    case EM_MOTION_ID(0, SHEN_MOT_FLINCH_LEG_C):
        if ((EmMotion_PassedFrameScaled(em, 62.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 188.0f, 0) != 0)) {
            EmShenGaoren_SpawnGroundEffect(em, 0x1A, 7, 0, 2.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, SHEN_MOT_FLINCH_LEG_A):
        if ((EmMotion_PassedFrameScaled(em, 62.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 188.0f, 0) != 0)) {
            EmShenGaoren_SpawnGroundEffect(em, 0x12, 7, 0, 2.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, SHEN_MOT_FLINCH_LEG_D):
        if ((EmMotion_PassedFrameScaled(em, 62.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 188.0f, 0) != 0)) {
            EmShenGaoren_SpawnGroundEffect(em, 0x1E, 7, 0, 2.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, SHEN_MOT_FLINCH_LEG_B):
        if ((EmMotion_PassedFrameScaled(em, 62.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 188.0f, 0) != 0)) {
            EmShenGaoren_SpawnGroundEffect(em, 0x16, 7, 0, 2.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, SHEN_MOT_DIE_LOW):
        if (EmMotion_PassedFrameScaled(em, 414.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 0, 0x28, em->scaleX);
            break;
        }
        break;
    default:
        EmShenGaoren_MotionEffectsDefault(em);
        break;
    }
}

/*
 * Callback slot 3: steps fxStep from 0 to 1 and runs
 * EmShenGaoren_MotionEffects.
 */
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

/*
 * Plays the impact sounds and spawns the impact effects and camera shake at
 * the fixed fort spot of the area (in AREA_FORTRESS_5 high when flag is
 * set, low otherwise).
 */
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
    HitSpark_SpawnForOwner(em, 0, 9, 0, pos, 3.0f);
    HitSpark_SpawnForOwner(em, 0, 9, 1, pos, 3.0f);
    PartFx_SpawnIfHere(em, pos, 0x22, 30.0f);
    CamShake_ReqARange(em, 2, 2);
}

/* Callback slots 2 and 5: does nothing. */
void EmShenGaoren_TableNop(void)
{
}

/* Callback slot 4: returns the first work byte (scriptStage) to the script. */
u8 EmShenGaoren_ScriptQuery(Em *em)
{
    return em->work;
}


/* The file's initialised data, in the game's order */
f32 EmShenGaoren_SeOffsets[4][3] = {
#include "game/008CA050.inc"
};
EmTurnParams EmShenGaoren_TurnParams[2] = {
#include "game/008CA080.inc"
};
f32 EmShenGaoren_DownEaseTargets[2][4] = {
#include "game/008CA0C0.inc"
};
void *EmShenGaoren_Callbacks[8] = {
    EmShenGaoren_SpawnEffect01, EmShenGaoren_SpawnEffect01, EmShenGaoren_TableNop, EmShenGaoren_UpdateMotionEffects,
    EmShenGaoren_ScriptQuery, EmShenGaoren_TableNop, NULL, NULL,
};
