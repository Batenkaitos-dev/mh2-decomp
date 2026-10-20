/*
 * The Lao-Shan Lung family: Lao-Shan Lung and Ashen Lao-Shan Lung
 * (EM_KIND_LAO_SHAN_LUNG and EM_KIND_ASHEN_LAO_SHAN_LUNG), the giant that
 * walks through the fortress areas to the fort. The file holds its motion
 * sounds and effects, its setup and Init (EmLaoshan_Setup, which the per-kind
 * dispatch Em_TypeSetup calls), the mine points carved from the living
 * monster and from its carcass, the state functions of each group with their
 * dispatchers (EmLaoshan_Run*State), the per-frame update (EmLaoshan_Update)
 * and the state changes (EmLaoshan_Set*State, EmLaoshan_ChangeState).
 *
 * The state functions take the monster; Em.step is the step within the
 * current state, and the family keeps its own values in the monster's work
 * area (EmLaoshanWork: the fight's stage, the hits on the fort, the mine
 * points, the held turn). em_laoshan.h names the motions, the states and the
 * two stances (on all fours, or reared on the hind legs). On the host the
 * update sends the monster's state to the other players every 150 frames.
 */
#include "common.h"
#include "scene.h"
#include "quest.h"
#include "mine_point.h"
#include "em.h"
#include "player.h"
#include "objattach.h"
#include "area.h"
#include "qsync.h"
#include "em_laoshan.h"
/* prototypes of callees */
void Em_TargetPlayerInRange(Em *);

/* prototypes */
void EmLaoshan_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void PartFx_SpawnForEmScaled(Em *em, s32 a, s32 b, f32 scale);
void ElementBurst_CreateOnEm(Em *em, u8 k, u8 c, f32 scale);
void Em_PlaySeAtBoneOnFrame(Em *em, s32 frame, s32 se, s32 bone);
void Em_SetLayerMotion(Em *em, s32 motion, s32 blend, s32 frame, s32 layer);
void Em_SetMotion(Em *em, s32 motion, s32 a, s32 b);
u16 Math_AngleXZ(f32 *from, f32 *to);
void Player_ClearFlag(void *, u32 flag);
void Player_SetFlag(void *obj, u32 flag);
void LaoShanAttack_Create(Em *, s8);
void LaoShanAttack_CreateSpan(Em *, s32, s32);
void StageShot_CreateAtSpot(Em *, s32, s32);
void LaoshanDust_Create(Em *em, u8 k, u8 c, f32 f);
void LaoShanAttack_CreateAt(Em *em, f32 *pos, s8 kind);
/* unit's own declaration: declared here as the callers use it; with the definition's void (Em *em, s32 group, s32 state, u8 mode), the calls in EmLaoshan_ChangeState, EmLaoshan_SetIdleState and others compile differently */
void Em_SetStateSync(Em *em, u16 group, u16 state, u8 mode);
void Em_PlaySeTypeAtBoneInFrames(Em *em, s32 frame, s32 frame2, s32 se, s32 bone, s32 vol, s32 a);
void Em_PlaySeTypeAtBoneOnFrame(Em *em, s32 frame, s32 se, s32 bone);
s32 Em_StepToGoal(Em *em, u16 step, u8 flags);
void Em_RestartScript(Em *em);
void EmLaoshan_GetMinePointPos(Em *em, f32 *out);
static s32 EmLaoshan_LayerMotionDone(Em *em, s32 layer);
void EmLaoshan_TurnToGoal(Em *);
void EmLaoshan_TurnRightEnd(Em *);
void EmLaoshan_TurnLeftEnd(Em *);
void EmLaoshan_WalkOn(Em *);
void EmLaoshan_WalkBackOn(Em *);
void EmLaoshan_RamFort(Em *);
void EmLaoshan_RamFortStage3(Em *);
void EmLaoshan_ReactPushedBack(Em *);
void EmLaoshan_Die(Em *);
void EmLaoshan_DieReared(Em *);
void EmLaoshan_IntroScene(Em *);
void EmLaoshan_MotionSounds(Em *em, struct EmWork *);
void EmLaoshan_MotionEffects(Em *, EmLaoshanWork *);
void EmLaoshan_SpawnSmashImpactEffects(Em *em);
void EmLaoshan_SpawnRamImpactEffects(Em *);
s32 Sound_RandomSe(s32 id, s16 lo, s32 alt, s16 n);
s32 EmMotion_PassedFrameScaled(void *p, f32 frame, s32 layer);
s32 EmMotion_ReachedFrame(void *obj, s32 a, f32 frame);
void CamShake_ReqAIf(Em *obj, s32 i);
void Vec_Set(f32 *out, f32 x, f32 y, f32 z);
f32 Vec_DistanceXZ(f32 *a, f32 *b);
s32 Debug_IsOn(s32);
u16 Area_DayId(u16 id);
void MinePoint_SetArea(s32 slot, u16 area);
void Shadow_Spawn(void *obj, s16 type);
void Em_SetMotionRow(Em *em, s32 row, s32 arg2, s32 arg3);
void Em_SetPartScaled(Em *em, s32 bit);
void Em_SetStateRand(Em *em, s32 arg1, s32 arg2);
s32 Em_ComputeMaxHp(Em *em, s16 idx);
void EmLaoshan_AddMinePoint(Em *em);
void Em_BonePos(void *obj, s16 bone, f32 *out);
EmBone *Em_Bone(void *obj, s16 bone);
void Vec_Rotate(f32 *out, f32 *v, f32 *m);
void EmLaoshan_SetMoveState(Em *em, u16 state, u8 mode);
void Em_SetSpeedScaled(Em *em, u16 flags, f32 x, f32 y);
void Em_MotionDelta(Em *em, f32 *out);
void Quest_TriggerEvent(Em *em);
void Player_ClearVelocity(void *);
void GmEm_ResetSize(); /* unit's own declaration: GmEm_ResetSize reads a0 = em (the incoming argument, still in a0); the call passes no argument */
s32 Save_TestEventFlag(s32);
s32 Em_PlaySeChecked(Em *em, s32 id, s32 a, void *pos, s32 b, s32 c);
void HitSpark_SpawnForOwner(void *owner, s32 ang, s32 type, s32 sub, f32 *pos, f32 scale);
void PartFx_SpawnIfHere(void *o, f32 *pos, s32 type, f32 scale);
s32 Em_SetCombat(Em *em, u8 a, s16 b);
void QuestEm_Hunted(Em *em);
void CamReq_BossHunted(void);
void EmLaoshan_AddCarcassMinePoints(Em *em);
void EmLaoshan_UpdateCarcassMinePoints(Em *em);
s32 MinePoint_Count(s32);
extern s8 EmLaoshan_MinePtBones[][2];
extern f32 EmLaoshan_MinePtRadius[];
void MinePoint_PrepareStub(MinePoint *pt);
void Em_MoveRotated(Em *em, void *src);
void QSync_SendEm(Em *em, u8 kind, u8 arg);
void CamReq_ForMonster(Em *em);
s32 Quest_GetOutcome(s32 withLoss);
void Em_EndStatus5(Em *em);
void Em_UpdatePendingScript(Em *em);
void Em_ClearHitReactMode(Em *em);
u8 Em_UpdateDamage(Em *em, u8 *out);
void EmLaoshan_RunState(Em *em);
void Em_PlaySeAtBoneVolOnFrame(Em *em, s32 frame, s32 se, s32 bone, s32 vol);
/* end of prototypes */

/*
 * Lao-Shan Lung's motion sounds: footsteps, roars and thuds at fixed
 * frames of the layer-0 motion kept in EmLaoshanWork.fxMotion, some over a
 * span of frames. Called every frame by the family's motion effects
 * (EmLaoshan_MotionEffects).
 */
void EmLaoshan_MotionSounds(Em *em, EmWork *work)
{
    s16 motion;

    motion = ((EmLaoshanWork *)work)->fxMotion;
    switch (motion) {
    case EM_MOTION_ID(0, LAOSHAN_MOT_IDLE):
        Em_PlaySeTypeAtBoneInFrames(em, 0x1CC, 0x24C, 0x46, 0x25, 6, 0);
        Em_PlaySeAtBoneVolOnFrame(em, 0xA0, 0x16, 0x30, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0x17, 0x32, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0x13, 0x21, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x122, 0x13, 0x21, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0xD, 3, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xBE, 0xF, 3, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1B8, 0x11, 3, 8);
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_WALK_BACK):
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0x16, 0x30, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x5A, 0xB, 0x17, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x4C, 0x6B, 0x17, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x4C, 3, 0x17, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x32, 0xC, 0xB, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xB2, 8, 0xE, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xA4, 0x69, 0xE, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xA4, 1, 0xE, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x8A, 0xE, 0x13, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x116, 8, 0x1D, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x108, 0x68, 0x1D, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x108, 0, 0x1D, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0xEE, 0x10, 5, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x17A, 8, 8, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x16C, 0x69, 8, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x16C, 1, 8, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x152, 0x13, 0x19, 8);
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_WALK):
        Em_PlaySeTypeAtBoneInFrames(em, 0x60, 0x1C2, 0x47, 0x25, 6, 0);
        Em_PlaySeAtBoneVolOnFrame(em, 0xDE, 0x16, 0x30, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x178, 0x17, 0x32, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x2A, 8, 0xE, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1C, 0x69, 0xE, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1C, 1, 0xE, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0xE, 0x13, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x9A, 8, 0x17, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x8C, 0x68, 0x17, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x8C, 0, 0x17, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x72, 0xC, 0xB, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xFA, 8, 8, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xEC, 0x69, 8, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xEC, 1, 8, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0xC8, 0x13, 0x19, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x16A, 8, 0x1D, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x15C, 0x68, 0x1D, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x15C, 0, 0x1D, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x136, 0x10, 5, 8);
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_WALK_TURN_RIGHT):
        Em_PlaySeAtBoneVolOnFrame(em, 0xC2, 0x15, 0x30, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x18A, 0x14, 0x32, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x4E, 8, 8, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x40, 0x69, 8, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x40, 1, 8, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x26, 0x13, 0x19, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xC2, 8, 0x1D, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xB4, 0x68, 0x1D, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xB4, 0, 0x1D, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x9A, 0x10, 5, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x138, 8, 0xE, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x12A, 0x69, 0xE, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x12A, 1, 0xE, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x110, 0xE, 0x13, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1A2, 8, 0x17, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x194, 0x68, 0x17, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x194, 0, 0x17, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x17A, 0xC, 0xB, 8);
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_WALK_TURN_LEFT):
        Em_PlaySeAtBoneVolOnFrame(em, 0xC2, 0x15, 0x30, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x18A, 0x14, 0x32, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x4E, 8, 0xE, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x40, 0x69, 0xE, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x40, 1, 0xE, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x26, 0x13, 0x13, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xC2, 8, 0x17, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xB4, 0x68, 0x17, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xB4, 0, 0x17, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x9A, 0x10, 0xB, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x138, 8, 8, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x12A, 0x69, 8, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x12A, 1, 8, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x110, 0xE, 0x19, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1A2, 8, 0x1D, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x194, 0x68, 0x1D, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x194, 0, 0x1D, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x17A, 0xC, 5, 8);
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_STOMP_START):
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0x17, 0x23, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0x13, 0x21, 8);
        Em_PlaySeTypeAtBoneInFrames(em, 0x9C, 0xC6, 0x22, 0x25, 8, 0);
        Em_PlaySeAtBoneVolOnFrame(em, 0x42, 0xB, 0x17, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x34, 0x6B, 0x17, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x34, 3, 0x17, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0xA, 0xC, 0xB, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x98, 0x6B, 0xE, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x98, 3, 0xE, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x8C, 0xC, 0x13, 8);
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_STOMP):
        Em_PlaySeTypeAtBoneInFrames(em, 2, 0x1E0, 0x48, 0x25, 9, 0);
        Em_PlaySeAtBoneVolOnFrame(em, 0x3C, 0x1E, 3, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x16E, 0xB, 0x17, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x160, 0x6B, 0x17, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x160, 3, 0x17, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x138, 0xC, 0xB, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1B6, 0x6B, 0xE, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1B6, 3, 0xE, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x186, 0xC, 0x13, 8);
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_REAR_UP):
        Em_PlaySeTypeAtBoneInFrames(em, 0x108, 0x294, 0x48, 0x25, 9, 0);
        Em_PlaySeTypeAtBoneInFrames(em, 0x108, 0x230, 0x1F, 3, 8, 0);
        Em_PlaySeAtBoneVolOnFrame(em, 0x118, 0x17, 0x23, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0xE, 0x23, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xCC, 0xC, 0x23, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x186, 0x10, 0x13, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x186, 0x12, 0x19, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x5A, 0x17, 0x30, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x10E, 0x16, 0x32, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xFC, 8, 0xE, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xFC, 0x69, 0xE, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xFC, 1, 0xE, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x100, 8, 8, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x100, 0x69, 8, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x100, 1, 8, 6);
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_RAM):
        Em_PlaySeAtBoneVolOnFrame(em, 0x13C, 0x17, 0x32, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xC8, 0x17, 0x32, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xC4, 0x17, 0x23, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xC4, 0, 0xB, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xD2, 0x18, 0xB, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 0xD6, 0x19, 0xB, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xD6, 0x1B, 0x21, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xD6, 8, 0x21, 4);
        Em_PlaySeTypeAtBoneInFrames(em, 0x140, 0x1E0, 0x26, 0x25, 8, 0);
        Em_PlaySeAtBoneVolOnFrame(em, 0x38, 8, 8, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x2A, 0x69, 8, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x2A, 1, 8, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x10, 0xE, 0x19, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x64, 8, 0xE, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x56, 0x69, 0xE, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x56, 1, 0xE, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x3C, 0x13, 0x13, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x8C, 8, 0x1D, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x7E, 0x68, 0x1D, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x7E, 0, 0x1D, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x64, 0xC, 5, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xC2, 8, 0x17, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xB4, 0x68, 0x17, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xB4, 0, 0x17, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x9A, 0x10, 0xB, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x198, 9, 0x17, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x18A, 0x68, 0x17, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x18A, 0, 0x17, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x170, 0x10, 0xB, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1AA, 9, 0xE, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x19C, 0x69, 0xE, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x19C, 1, 0xE, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x182, 0x13, 0x13, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1D2, 9, 8, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1C4, 0x69, 8, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1C4, 1, 8, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1AA, 0xE, 0x19, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1FA, 9, 0x1D, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1EC, 0x68, 0x1D, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1EC, 0, 0x1D, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1D2, 0xC, 5, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x212, 0x69, 0xE, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x212, 1, 0xE, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1F8, 0x13, 0x13, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x228, 0x68, 0x17, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x228, 0, 0x17, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x20E, 0x10, 0xB, 8);
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_REARED_IDLE):
        Em_PlaySeTypeAtBoneInFrames(em, 6, 0x168, 0x47, 0x25, 6, 0);
        Em_PlaySeAtBoneVolOnFrame(em, 0x18, 0x13, 3, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x88, 0x14, 0x32, 8);
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_REARED_STRIKE):
        Em_PlaySeAtBoneVolOnFrame(em, 6, 0x22, 0x25, 8);
        Em_PlaySeTypeAtBoneInFrames(em, 0x8C, 0x12C, 0x23, 0x25, 9, 0);
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0xD, 3, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x112, 0xD, 3, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0x15, 0x32, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x96, 0x17, 0x30, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xA0, 7, 0x30, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xA0, 2, 0x30, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x4A, 8, 0x17, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x3C, 0x68, 0x17, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x3C, 0, 0x17, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x12, 0x13, 0xB, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1FC, 8, 0x17, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1EE, 0x68, 0x17, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1EE, 0, 0x17, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1C6, 0x13, 0xB, 8);
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_REARED_SMASH):
        Em_PlaySeAtBoneVolOnFrame(em, 0x8E, 0x17, 0x23, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x88, 0, 0x23, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x8A, 0x68, 0x23, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x88, 0x18, 0x23, 9);
        Em_PlaySeAtBoneVolOnFrame(em, 0x96, 0x19, 0x23, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x96, 8, 0x23, 9);
        Em_PlaySeAtBoneVolOnFrame(em, 0x10, 0x12, 0x21, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xBA, 0xD, 3, 8);
        Em_PlaySeTypeAtBoneInFrames(em, 0xF6, 0x190, 0x4E, 0x25, 8, 0);
        Em_PlaySeAtBoneVolOnFrame(em, 0x54, 8, 0x1D, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x46, 0x68, 0x1D, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x46, 0, 0x1D, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1E, 0xE, 0x19, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xFA, 0xA, 0x1D, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xEC, 0x68, 0x1D, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xEC, 0, 0x1D, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0xC4, 0xE, 0x19, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x44, 0x16, 0x32, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x58, 0x17, 0x30, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xC4, 0x14, 0x30, 8);
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_REAR_DOWN):
        Em_PlaySeAtBoneVolOnFrame(em, 0x3A, 0x26, 0x25, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x140, 0x23, 0x25, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0xF, 3, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x122, 0x10, 3, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x78, 0xA, 0x17, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x6A, 0x6B, 0x17, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x6A, 3, 0x17, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x3C, 0xC, 0x13, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x118, 0xA, 0x1D, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x10A, 0x68, 0x1D, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x10A, 0, 0x1D, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0xF6, 0xE, 0x19, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x19A, 0xA, 0x17, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x18C, 0x68, 0x17, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x18C, 0, 0x17, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x172, 0x13, 0xB, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1DC, 8, 0xE, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1DC, 0x69, 0xE, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1DC, 1, 0xE, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1E0, 8, 8, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1E0, 0x69, 8, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1E0, 1, 8, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1FE, 0x11, 3, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x216, 0x18, 0x21, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1FA, 0x17, 0x32, 8);
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_REARED_WALK):
        Em_PlaySeAtBoneVolOnFrame(em, 0xC8, 0x1E, 0x25, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x18, 0xA, 0x1D, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xA, 0x68, 0x1D, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xA, 0, 0x1D, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0xE, 0x13, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xA4, 0xA, 0x17, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x96, 0x68, 0x17, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x96, 0, 0x17, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x90, 0xE, 0x19, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1A, 0x14, 0x32, 8);
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_TURN_RIGHT_START):
        Em_PlaySeAtBoneVolOnFrame(em, 0x4E, 9, 8, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x40, 0x69, 8, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x40, 1, 8, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x26, 0xE, 0x19, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xA4, 9, 0xE, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x96, 0x69, 0xE, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x96, 1, 0xE, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x7C, 0x13, 0x13, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x5E, 0x17, 0x32, 8);
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_TURN_RIGHT_HOLD):
    case EM_MOTION_ID(0, LAOSHAN_MOT_TURN_LEFT_HOLD):
        Em_PlaySeTypeAtBoneInFrames(em, 0x120, 0x18E, 0x4E, 0x25, 6, 0);
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0x16, 0x30, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x140, 0x17, 0x32, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0x13, 0x21, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x122, 0x13, 0x21, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0xD, 3, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xBE, 0xF, 3, 8);
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_TURN_RIGHT_BACK):
        Em_PlaySeAtBoneVolOnFrame(em, 0x4E, 8, 0xE, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x40, 0x69, 0xE, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x40, 1, 0xE, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x26, 0x13, 0x13, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x8A, 8, 8, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x7C, 0x69, 8, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x7C, 1, 8, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x62, 0xE, 0x19, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x30, 0x16, 0x30, 8);
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_TURN_RIGHT_END):
        Em_PlaySeAtBoneVolOnFrame(em, 0x48, 8, 0x1D, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x3A, 0x68, 0x1D, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x3A, 0, 0x1D, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x20, 0x10, 5, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x9A, 8, 0x17, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x8C, 0x68, 0x17, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x8C, 0, 0x17, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x72, 0xC, 0xB, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0x16, 0x30, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x2A, 0x17, 0x32, 8);
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_QUARTER_TURN_RIGHT):
        Em_PlaySeAtBoneVolOnFrame(em, 0x52, 8, 0x1D, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x44, 0x68, 0x1D, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x44, 0, 0x1D, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x2A, 0x10, 5, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xA4, 8, 0x17, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x96, 0x68, 0x17, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x96, 0, 0x17, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x7C, 0xC, 0xB, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xF8, 8, 8, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xEA, 0x69, 8, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xEA, 1, 8, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0xD0, 0xE, 0x19, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x14E, 8, 0xE, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x140, 0x69, 0xE, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x140, 1, 0xE, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x12A, 0x13, 0x13, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x38, 0x17, 0x32, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x11E, 0x17, 0x32, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x90, 0x16, 0x30, 8);
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_HALF_TURN_RIGHT):
        Em_PlaySeAtBoneVolOnFrame(em, 0x4A, 8, 0x17, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x3C, 0x68, 0x17, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x3C, 0, 0x17, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x22, 0xC, 0xB, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x9C, 8, 8, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x8E, 0x69, 8, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x8E, 1, 8, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x74, 0xE, 0x19, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xCC, 8, 0xE, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xBE, 0x69, 0xE, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xBE, 1, 0xE, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0xA4, 0x13, 0x13, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x116, 8, 0x1D, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x108, 0x68, 0x1D, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x108, 0, 0x1D, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0xEE, 0x10, 5, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x166, 8, 0x17, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x158, 0x68, 0x17, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x158, 0, 0x17, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x13E, 0xC, 0xB, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x66, 0x17, 0x32, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x154, 0x17, 0x32, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x2C, 0x16, 0x30, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xF4, 0x16, 0x30, 8);
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_TURN_LEFT_START):
        Em_PlaySeAtBoneVolOnFrame(em, 0x4E, 9, 0xE, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x40, 0x69, 0xE, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x40, 1, 0xE, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x26, 0x13, 0x13, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xA4, 9, 8, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x96, 0x69, 8, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x96, 1, 8, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x7C, 0xE, 0x19, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x5E, 0x17, 0x32, 8);
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_TURN_LEFT_BACK):
        Em_PlaySeAtBoneVolOnFrame(em, 0x4E, 8, 8, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x40, 0x69, 8, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x40, 1, 8, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x26, 0xE, 0x19, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x8A, 8, 0xE, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x7C, 0x69, 0xE, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x7C, 1, 0xE, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x62, 0x13, 0x13, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x30, 0x16, 0x30, 8);
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_TURN_LEFT_END):
        Em_PlaySeAtBoneVolOnFrame(em, 0x48, 8, 0x17, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x3A, 0x68, 0x17, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x3A, 0, 0x17, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x20, 0xC, 0xB, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x9A, 8, 0x1D, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x8C, 0x68, 0x1D, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x8C, 0, 0x1D, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x72, 0x10, 5, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0x16, 0x30, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x2A, 0x17, 0x32, 8);
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_QUARTER_TURN_LEFT):
        Em_PlaySeAtBoneVolOnFrame(em, 0x52, 8, 0x17, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x44, 0x68, 0x17, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x44, 0, 0x17, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x2A, 0xC, 0xB, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xA4, 8, 0x1D, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x96, 0x68, 0x1D, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x96, 0, 0x1D, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x7C, 0x10, 5, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xF8, 8, 0xE, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xEA, 0x69, 0xE, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xEA, 1, 0xE, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0xD0, 0x13, 0x13, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x14E, 8, 8, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x140, 0x69, 8, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x140, 1, 8, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x12A, 0xE, 0x19, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x38, 0x17, 0x32, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x11E, 0x17, 0x32, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x90, 0x16, 0x30, 8);
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_HALF_TURN_LEFT):
        Em_PlaySeAtBoneVolOnFrame(em, 0x4A, 8, 0x1D, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x3C, 0x68, 0x1D, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x3C, 0, 0x1D, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x22, 0x10, 5, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x9C, 8, 0xE, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x8E, 0x69, 0xE, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x8E, 1, 0xE, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x74, 0x13, 0x13, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xCC, 8, 8, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xBE, 0x69, 8, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xBE, 1, 8, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0xA4, 0xE, 0x19, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x116, 8, 0x17, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x108, 0x68, 0x17, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x108, 0, 0x17, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0xEE, 0xC, 0xB, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x166, 8, 0x1D, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x158, 0x68, 0x1D, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x158, 0, 0x1D, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x13E, 0x10, 5, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x66, 0x17, 0x32, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x154, 0x17, 0x32, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x2C, 0x16, 0x30, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xF4, 0x16, 0x30, 8);
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_FALL):
        Em_PlaySeTypeAtBoneInFrames(em, 0xE4, 0x19A, 0x46, 0x25, 8, 0);
        Em_PlaySeAtBoneVolOnFrame(em, 0xB2, 0x18, 0x23, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xAC, 0x19, 0x23, 9);
        Em_PlaySeAtBoneVolOnFrame(em, 0xAE, 0x6B, 0x23, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xAC, 8, 0x23, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x56, 8, 0xE, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x48, 0x69, 0xE, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x48, 1, 0xE, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x2E, 0x13, 0x13, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xA4, 0xE, 0x13, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xA4, 0x10, 0x19, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x154, 0x17, 0x32, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x2C, 0x16, 0x30, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xE4, 0xD, 0x21, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x104, 0x13, 3, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x48, 0xA, 0x1D, 8);
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_GET_UP):
        Em_PlaySeTypeAtBoneInFrames(em, 4, 0xD2, 0x46, 0x25, 8, 0);
        Em_PlaySeTypeAtBoneInFrames(em, 0x104, 0x19A, 0x4B, 0x25, 9, 0);
        Em_PlaySeAtBoneVolOnFrame(em, 0x60, 9, 0xE, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x52, 0x69, 0xE, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x52, 1, 0xE, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x38, 0x13, 0x13, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xA4, 9, 8, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x96, 0x69, 8, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x96, 1, 8, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x7C, 0xE, 0x19, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xEA, 9, 0x17, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xDC, 0x68, 0x17, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xDC, 0, 0x17, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0xC2, 0x10, 0xB, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x13A, 9, 0x1D, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x12C, 0x68, 0x1D, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x12C, 0, 0x1D, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x112, 0xC, 5, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x38, 0x17, 0x32, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x38, 0x16, 0x30, 8);
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_REARED_FLINCH):
        Em_PlaySeTypeAtBoneInFrames(em, 4, 0xD2, 0x4C, 0x25, 9, 0);
        Em_PlaySeAtBoneVolOnFrame(em, 0xAC, 0x17, 0x23, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x38, 0x17, 0x32, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x38, 0x16, 0x30, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0xE, 3, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0x13, 3, 8);
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_PUSHED_BACK):
        Em_PlaySeTypeAtBoneInFrames(em, 4, 0x11C, 0x4C, 0x25, 9, 0);
        Em_PlaySeAtBoneVolOnFrame(em, 0x2C, 0x17, 0x32, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x2C, 0x16, 0x30, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x16, 9, 0xE, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 8, 0x69, 0xE, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 8, 1, 0xE, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0xE, 0x13, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1C, 9, 0x1D, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xE, 0x68, 0x1D, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xE, 0, 0x1D, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0x10, 5, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x68, 9, 0x17, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x5A, 0x68, 0x17, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x5A, 0, 0x17, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x40, 0x10, 0xB, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xB4, 9, 0xE, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xA6, 0x69, 0xE, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xA6, 1, 0xE, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x8C, 0xE, 0x13, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xD6, 8, 0x1D, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xC8, 0x68, 0x1D, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xC8, 0, 0x1D, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0xAE, 0x10, 5, 8);
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_LEAVE_START):
        Em_PlaySeTypeAtBoneInFrames(em, 0x8A, 0xBC, 0x4C, 0x25, 9, 0);
        Em_PlaySeAtBoneVolOnFrame(em, 0x34, 8, 0xE, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x34, 0x69, 0xE, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xFC, 1, 0xE, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x38, 8, 8, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x38, 0x69, 8, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x38, 1, 8, 6);
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_LEAVE_TURN):
        Em_PlaySeTypeAtBoneInFrames(em, 4, 0x18C, 0x49, 0x25, 9, 0);
        Em_PlaySeAtBoneVolOnFrame(em, 0x26, 9, 0x17, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x18, 0x68, 0x17, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x18, 0, 0x17, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 4, 0x10, 0xB, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xA8, 9, 0xE, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x9A, 0x69, 0xE, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x9A, 1, 0xE, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x80, 0x13, 0x13, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xB4, 9, 8, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xA6, 0x69, 8, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xA6, 1, 8, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x8C, 0xE, 0x19, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x124, 9, 0x1D, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x116, 0x68, 0x1D, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x116, 0, 0x1D, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0xFC, 0xC, 5, 8);
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_WALK_ATTACK):
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_DIE):
        Em_PlaySeTypeAtBoneInFrames(em, 6, 0x274, 0x4D, 0x25, 9, 0);
        Em_PlaySeAtBoneVolOnFrame(em, 0x1A8, 0x19, 0x23, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x154, 0x1A, 0x13, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x190, 0x19, 0x13, 9);
        Em_PlaySeAtBoneVolOnFrame(em, 0x190, 0x18, 0x13, 9);
        Em_PlaySeAtBoneVolOnFrame(em, 0x192, 0x6B, 0x13, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x19A, 0x18, 0x30, 9);
        Em_PlaySeAtBoneVolOnFrame(em, 0xD8, 9, 0x17, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xCA, 0x68, 0x17, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xCA, 0, 0x17, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0xAE, 0x10, 0xB, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x144, 0x17, 0x32, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x140, 0x16, 0x30, 8);
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_FLINCH):
        Em_PlaySeTypeAtBoneInFrames(em, 4, 0xD2, 0x4C, 0x25, 9, 0);
        Em_PlaySeAtBoneVolOnFrame(em, 0xAC, 0x17, 0x23, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x38, 0x17, 0x32, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x38, 0x16, 0x30, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x58, 8, 0xE, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x4A, 0x69, 0xE, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0x4A, 1, 0xE, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0x30, 0x13, 0x13, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xF6, 9, 0xE, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0xE8, 0x69, 0xE, 0x16);
        Em_PlaySeAtBoneVolOnFrame(em, 0xE8, 1, 0xE, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 0xCE, 0x13, 0x13, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x3C, 0x17, 0x32, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x3C, 0x16, 0x30, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 0x30, 0x19, 0xE, 8);

        break;
    }
}

/*
 * Per-kind setup (Em_TypeSetup): clears EmLaoshanWork.stage and, when the
 * living monster has its mine point (mineSlot), moves the point to the
 * monster area and to EmLaoshan_GetMinePointPos.
 */
void EmLaoshan_Setup(Em *em)
{
    EmLaoshanWork *w = (EmLaoshanWork *)&em->work;

    f32 pos[4];
    s8 slot;

    w->stage = 0;
    slot = w->mineSlot;
    if (slot != -1) {
        MinePoint_SetArea((s32) slot, em->area);
        EmLaoshan_GetMinePointPos(em, pos);
        MinePoint_SetPos(w->mineSlot, pos);
    }
}

/* Callback table entry: spawns Effect01 type 3 for the monster. */
void EmLaoshan_SpawnEffect01(u8 *em)
{
    Shadow_Spawn((Player *) em, 0x3);
}

/*
 * Writes to out the position of the living monster mine point: bone 3
 * position plus a fixed offset (up 250, forward 120) rotated by the bone 3
 * matrix.
 */
void EmLaoshan_GetMinePointPos(Em *em, f32 *out)
{
    f32 ofs[3];
    f32 rotOfs[3];
    f32 bonePos[3];

    ofs[1] = 250.0f;
    ofs[2] = 120.0f;
    ofs[0] = 0.0f;
    Em_BonePos(em, 3, bonePos);
    Vec_Rotate((f32 *) rotOfs, (f32 *) ofs, (f32 *) (Em_Bone(em, 3)));
    out[0] = bonePos[0] + rotOfs[0];
    out[1] = bonePos[1] + rotOfs[1];
    out[2] = bonePos[2] + rotOfs[2];
}

/*
 * Adds the carving mine point on the living monster (radius 340,
 * MINE_ACTION_CARVE, count mineCount, mine table by kind and quest tier) at
 * EmLaoshan_GetMinePointPos and keeps its slot in mineSlot (set to -1 first
 * when mineCount is 0 or less). Called by Init.
 */
void EmLaoshan_AddMinePoint(Em *em)
{
    EmLaoshanWork *w = (EmLaoshanWork *)&em->work;
    MinePoint point;
    f32 pos[3];

    if (w->mineCount <= 0) {
        w->mineSlot = -1;
    }
    point.radius = 340.0f;
    if (em->type == EM_KIND_LAO_SHAN_LUNG) {
        switch (gQuest.header->tier) {
        default:
            point.table = 0xA;
            break;
        case 1:
            point.table = 0x86;
            break;
        case 2:
            point.table = 0x103;
            break;
        case 3:
            point.table = 0x103;
            break;
        }
    } else {
        switch (gQuest.header->tier) {
        default:
            point.table = 0x54;
            break;
        case 1:
            point.table = 0xD0;
            break;
        case 2:
            point.table = 0x14D;
            break;
        case 3:
            point.table = 0x14D;
            break;
        }
    }
    point.action = MINE_ACTION_CARVE;
    point.count = w->mineCount;
    point.area = (u8) em->area;
    point.flags = 1;
    point.timer = 0;
    EmLaoshan_GetMinePointPos(em, pos);
    w->mineSlot = MinePoint_AddUntimed(&point, pos, 0);
}

/*
 * Adds the three carving mine points of the defeated monster on the floor
 * below the bones of EmLaoshan_MinePtBones (radius from
 * EmLaoshan_MinePtRadius, count from the bones table, mine table by kind
 * and tier) into minePts. Called by the defeat states after the collapse.
 */
void EmLaoshan_AddCarcassMinePoints(Em *em)
{
    EmLaoshanWork *w = (EmLaoshanWork *)&em->work;
    s32 i;
    MinePoint point;
    f32 pos[3];

    if (em->type == EM_KIND_LAO_SHAN_LUNG) {
        switch (gQuest.header->tier) {
        default:
            point.table = 9;
            break;
        case 1:
            point.table = 0x85;
            break;
        case 2:
            point.table = 0x102;
            break;
        case 3:
            point.table = 0x102;
            break;
        }
    } else {
        switch (gQuest.header->tier) {
        default:
            point.table = 0x53;
            break;
        case 1:
            point.table = 0xCF;
            break;
        case 2:
            point.table = 0x14C;
            break;
        case 3:
            point.table = 0x14C;
            break;
        }
    }
    for (i = 0; i < 3; i++) {
        point.radius = EmLaoshan_MinePtRadius[i];
        point.area = (u8) em->area;
        point.timer = 0;
        point.flags = 1;
        Em_BonePos(em, EmLaoshan_MinePtBones[i][0], pos);
        pos[1] = em->floorY;
        point.action = MINE_ACTION_CARVE;
        point.count = EmLaoshan_MinePtBones[i][1];
        MinePoint_PrepareStub(&point);
        w->minePts[i] = MinePoint_AddUntimed(&point, pos, 0);
    }
}

/*
 * Keeps each of the three minePts at its bone (y at floorY) and removes a
 * point (slot -1) once its MinePoint_Count is 0 or less.
 */
void EmLaoshan_UpdateCarcassMinePoints(Em *em)
{
    EmLaoshanWork *w = (EmLaoshanWork *)&em->work;
    s32 i;
    s8 *bone;
    f32 *posY;
    s8 *slot;
    f32 pos[3];

    bone = EmLaoshan_MinePtBones[0];
    posY = &pos[1];
    for (i = 0; i < 3; i++) {
        slot = &w->minePts[i];
        if (*slot != -1) {
            Em_BonePos(em, *bone, pos);
            *posY = em->floorY;
            MinePoint_SetPos(*slot, pos);
            if (MinePoint_Count(*slot) <= 0) {
                MinePoint_Remove(*slot);
                *slot = -1;
            }
        }
        bone += 2;
    }
}

/* Removes the minePts that still exist and sets them to -1. */
void EmLaoshan_RemoveCarcassMinePoints(Em *em)
{
    EmLaoshanWork *w = (EmLaoshanWork *)&em->work;
    s32 i;
    s8 *slot;

    for (i = 0; i < 3; i++) {
        slot = &w->minePts[i];
        if (*slot != -1) {
            MinePoint_Remove(*slot);
            *slot = -1;
        }
    }
}

/*
 * Starts the monster: on quest stage 0 it is placed by its area (the
 * fortress areas and two others); sets hp and hpMax from Em_ComputeMaxHp,
 * scriptPending, blind and combat with all four players, the motion
 * LAOSHAN_MOT_IDLE and the state LAOSHAN_IDLE (Em_SetStateRand); resets the
 * work area (syncTimer 150, mineCount 3) and adds the mine point carved from
 * the living monster.
 */
void EmLaoshan_Init(Em *em)
{
    EmLaoshanWork *w = (EmLaoshanWork *)&em->work;

    s16 maxHp;
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
    maxHp = Em_ComputeMaxHp(em, 0);
    em->hp = maxHp;
    em->hpMax = maxHp;
    em->scriptPending = 1;
    em->blind = 1;
    em->unk_InitFlag = 1;
    em->combat = 1;
    em->combatPlayers = 0xF;
    Em_SetMotionRow(em, LAOSHAN_MOT_IDLE, 0, 0);
    Em_SetStateRand(em, 0, 1);
    w->unk00C = 1500.0f;
    w->turnHeld = 0;
    w->turnHeldFrames = 0;
    w->stage = 0;
    w->syncTimer = 0x96;
    w->unk014 = -1;
    w->unk018 = -1;
    w->mineCount = 3;
    EmLaoshan_AddMinePoint(em);
}

/*
 * Ends a state: motion speed 1, lookAtTarget off, scriptPending set; in
 * LAOSHAN_STANCE_REARED goes to LAOSHAN_IDLE_REARED, else sets
 * LAOSHAN_STANCE_ALL_FOURS and goes to LAOSHAN_IDLE.
 */
void EmLaoshan_EndState(Em *em)
{
    em->motionSpeed = 1.0f;
    em->lookAtTarget = 0;
    em->scriptPending = 1;
    if (em->stance == LAOSHAN_STANCE_REARED) {
        em->stance = LAOSHAN_STANCE_REARED;
        EmLaoshan_ChangeState(em, EM_GROUP_IDLE, LAOSHAN_IDLE_REARED, EM_SYNC_LOCAL);
        return;
    }
    em->stance = LAOSHAN_STANCE_ALL_FOURS;
    EmLaoshan_ChangeState(em, EM_GROUP_IDLE, LAOSHAN_IDLE, EM_SYNC_LOCAL);
}

/*
 * Returns 1 and sets scriptPending when the layer motion has stopped (or
 * layer is 0xFF), else 0; the turn states use it to let the script pick the
 * next state while their pose holds.
 */
static s32 EmLaoshan_LayerMotionDone(Em *em, s32 layer)
{
    if (em->motion[layer].playing == 0 || layer == 0xFF) {
        em->scriptPending = 1;
        return 1;
    }
    return 0;
}

/*
 * LAOSHAN_IDLE_HOLD: unless debug switch 0x4D is on, replays
 * LAOSHAN_MOT_IDLE whenever a layer's motion id differs from the idle's;
 * never ends by itself. The ids it expects on layers 1 and 2 are 1101 and
 * 1201, as if the layers' ids were 100 apart (EM_MOTION_ID puts them 200
 * apart), so once LAOSHAN_IDLE has set the layers the tests fail and the
 * idle restarts every frame.
 */
void EmLaoshan_IdleHold(Em *em)
{
    if (Debug_IsOn(0x4D) == 0) {
        if (em->motionId[0] != EM_MOTION_ID(0, LAOSHAN_MOT_IDLE)) {
            Em_SetMotion(em, LAOSHAN_MOT_IDLE, 0xA, 0);
        }
        if (em->motionId[1] != 1101) {
            Em_SetMotion(em, LAOSHAN_MOT_IDLE, 0xA, 0);
        }
        if (em->motionId[2] != 1201) {
            Em_SetMotion(em, LAOSHAN_MOT_IDLE, 0xA, 0);
        }
    }
}

/*
 * LAOSHAN_IDLE: on all fours (LAOSHAN_STANCE_ALL_FOURS), plays the idle motion on
 * layers 0 to 2 (unless already playing) and ends with the layer-0 motion
 * (EmLaoshan_EndState).
 */
void EmLaoshan_Idle(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = LAOSHAN_STANCE_ALL_FOURS;
        if (em->motionId[0] != EM_MOTION_ID(0, LAOSHAN_MOT_IDLE)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(0, LAOSHAN_MOT_IDLE), 0xA, 0, 0);
        }
        if (em->motionId[1] != EM_MOTION_ID(1, LAOSHAN_MOT_IDLE)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(1, LAOSHAN_MOT_IDLE), 0xA, 0, 1);
        }
        if (em->motionId[2] != EM_MOTION_ID(2, LAOSHAN_MOT_IDLE)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(2, LAOSHAN_MOT_IDLE), 0xA, 0, 2);
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

/*
 * LAOSHAN_IDLE_STOMP: plays LAOSHAN_MOT_STOMP_START then LAOSHAN_MOT_STOMP,
 * keeping quakeTimer at 5 (players near it fall over) until the motion
 * ends.
 */
void EmLaoshan_IdleStomp(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = LAOSHAN_STANCE_ALL_FOURS;
        Em_SetMotion(em, LAOSHAN_MOT_STOMP_START, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, LAOSHAN_MOT_STOMP, 0, 0);
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

/*
 * LAOSHAN_IDLE_REAR_UP: plays LAOSHAN_MOT_REAR_UP under guardTimer and
 * noReact, then takes LAOSHAN_STANCE_REARED and ends.
 */
void EmLaoshan_IdleRearUp(Em *em)
{
    em->guardTimer = 5;
    em->noReact = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = LAOSHAN_STANCE_ALL_FOURS;
        Em_SetMotion(em, LAOSHAN_MOT_REAR_UP, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stance = LAOSHAN_STANCE_REARED;
            em->guardTimer = 0;
            em->noReact = 0;
            EmLaoshan_EndState(em);
        }
        break;
    }
}

/*
 * LAOSHAN_IDLE_REARED: the reared idle (LAOSHAN_MOT_REARED_IDLE,
 * LAOSHAN_STANCE_REARED) until the motion ends.
 */
void EmLaoshan_IdleReared(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = LAOSHAN_STANCE_REARED;
        Em_SetMotion(em, LAOSHAN_MOT_REARED_IDLE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmLaoshan_EndState(em);
        }
        break;
    }
}

/*
 * LAOSHAN_IDLE_REAR_DOWN: plays LAOSHAN_MOT_REAR_DOWN under guardTimer and
 * noReact, then returns to LAOSHAN_STANCE_ALL_FOURS and ends.
 */
void EmLaoshan_IdleRearDown(Em *em)
{
    em->guardTimer = 5;
    em->noReact = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = LAOSHAN_STANCE_REARED;
        Em_SetMotion(em, LAOSHAN_MOT_REAR_DOWN, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->guardTimer = 0;
            em->noReact = 0;
            em->stance = LAOSHAN_STANCE_ALL_FOURS;
            EmLaoshan_EndState(em);
        }
        break;
    }
}

/* LAOSHAN_IDLE_SHORT: as LAOSHAN_IDLE but ends after at most 30 frames. */
void EmLaoshan_IdleShort(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = LAOSHAN_STANCE_ALL_FOURS;
        if (em->motionId[0] != EM_MOTION_ID(0, LAOSHAN_MOT_IDLE)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(0, LAOSHAN_MOT_IDLE), 0xA, 0, 0);
        }
        if (em->motionId[1] != EM_MOTION_ID(1, LAOSHAN_MOT_IDLE)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(1, LAOSHAN_MOT_IDLE), 0xA, 0, 1);
        }
        if (em->motionId[2] != EM_MOTION_ID(2, LAOSHAN_MOT_IDLE)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(2, LAOSHAN_MOT_IDLE), 0xA, 0, 2);
        }
        em->timer = 0x1E;
        break;
    case 1:
        if (em->motion[0].playing == 0 || --em->timer <= 0) {
            em->step++;
            EmLaoshan_EndState(em);
        }
        break;
    }
}

/*
 * LAOSHAN_IDLE_REARED_SHORT: as LAOSHAN_IDLE_REARED but ends after at most
 * 30 frames.
 */
void EmLaoshan_IdleRearedShort(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = LAOSHAN_STANCE_REARED;
        Em_SetMotion(em, LAOSHAN_MOT_REARED_IDLE, 0, 0);
        em->timer = 0x1E;
        break;
    case 1:
        if (em->motion[0].playing == 0 || --em->timer <= 0) {
            em->step++;
            EmLaoshan_EndState(em);
        }
        break;
    }
}

/* Runs the idle state (EM_GROUP_IDLE) by em->state. */
void EmLaoshan_RunIdleState(Em *em)
{
    switch (em->state) {
    case LAOSHAN_IDLE_HOLD:
        EmLaoshan_IdleHold(em);
        break;
    case LAOSHAN_IDLE:
        EmLaoshan_Idle(em);
        break;
    case LAOSHAN_IDLE_STOMP:
        EmLaoshan_IdleStomp(em);
        break;
    case LAOSHAN_IDLE_REAR_UP:
        EmLaoshan_IdleRearUp(em);
        break;
    case LAOSHAN_IDLE_REARED:
        EmLaoshan_IdleReared(em);
        break;
    case LAOSHAN_IDLE_REAR_DOWN:
        EmLaoshan_IdleRearDown(em);
        break;
    case LAOSHAN_IDLE_SHORT:
        EmLaoshan_IdleShort(em);
        break;
    case LAOSHAN_IDLE_REARED_SHORT:
        EmLaoshan_IdleRearedShort(em);

    default:
        break;
    }
}

/*
 * LAOSHAN_MOVE_WALK: walks to the goal (LAOSHAN_MOT_WALK, Em_StepToGoal),
 * then stops the root motion (EM_FLAG_NO_ROOT_MOTION) until the motion
 * ends.
 */
void EmLaoshan_Walk(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = LAOSHAN_STANCE_ALL_FOURS;
        Em_SetMotion(em, LAOSHAN_MOT_WALK, 0, 0);
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

/*
 * LAOSHAN_MOVE_WALK_BACK: walks to a goal behind (LAOSHAN_MOT_WALK_BACK,
 * Em_StepToGoal flag 4 so backward movement counts too) and ends with the
 * motion.
 */
void EmLaoshan_WalkBack(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = LAOSHAN_STANCE_ALL_FOURS;
        Em_SetMotion(em, LAOSHAN_MOT_WALK_BACK, 0, 0);
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

/*
 * LAOSHAN_MOVE_TURN_TO_GOAL: aims at the goal and repeats LAOSHAN_MOT_WALK
 * in place (goal within about 25 degrees, no root motion) or a walking turn
 * (LAOSHAN_MOT_WALK_TURN_RIGHT or _LEFT, rotY 0x22 a frame between frames
 * 162 and 462) until facing the goal, then ends.
 */
void EmLaoshan_TurnToGoal(Em *em)
{
    u32 angleDiff;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = LAOSHAN_STANCE_ALL_FOURS;
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        angleDiff = (em->aimAngle - em->rotY) & 0xFFFF;
        if (angleDiff <= 0x11C7 || angleDiff >= 0xEE39) {
            Player_SetFlag(em, EM_FLAG_NO_ROOT_MOTION);
            Em_SetMotion(em, LAOSHAN_MOT_WALK, 0, 0);
        } else if (angleDiff >= 0x8000) {
            Em_SetMotion(em, LAOSHAN_MOT_WALK_TURN_LEFT, 0, 0);
        } else {
            Em_SetMotion(em, LAOSHAN_MOT_WALK_TURN_RIGHT, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].blend == 0) {
            angleDiff = (em->aimAngle - (u16) em->rotY) & 0xFFFF;
            if (em->motion[0].playing == 0) {
                if (((angleDiff + 0x22) & 0xFFFF) <= 0x43) {
                    em->step++;
                    Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
                    EmLaoshan_EndState(em);
                } else if (angleDiff <= 0x11C7 || angleDiff >= 0xEE39) {
                    Player_SetFlag(em, EM_FLAG_NO_ROOT_MOTION);
                    Em_SetMotion(em, LAOSHAN_MOT_WALK, 0, 0);
                } else {
                    Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
                    if (angleDiff >= 0x8000) {
                        Em_SetMotion(em, LAOSHAN_MOT_WALK_TURN_LEFT, 0, 0);
                    } else {
                        Em_SetMotion(em, LAOSHAN_MOT_WALK_TURN_RIGHT, 0, 0);
                    }
                }
            } else if (EmMotion_ReachedFrame(em, 0, 162.0f) != 0 && EmMotion_ReachedFrame(em, 0, 462.0f) == 0) {
                if (((angleDiff + 0x22) & 0xFFFF) <= 0x43) {
                    em->rotY = em->aimAngle;
                } else if (angleDiff <= 0x7FFF) {
                    em->rotY = (u16) (em->rotY + 0x22);
                } else {
                    em->rotY = (u16) (em->rotY - 0x22);
                }
            }
        }
        break;
    }
}

/*
 * LAOSHAN_MOVE_WALK_TURN_RIGHT: plays LAOSHAN_MOT_WALK_TURN_RIGHT, rotY
 * rising 0x22 a frame between frames 162 and 462, and ends with the motion.
 */
void EmLaoshan_WalkTurnRight(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = LAOSHAN_STANCE_ALL_FOURS;
        Em_SetMotion(em, LAOSHAN_MOT_WALK_TURN_RIGHT, 0, 0);
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

/*
 * LAOSHAN_MOVE_TURN_RIGHT_START: plays LAOSHAN_MOT_TURN_RIGHT_START (rotY
 * rising 0x80 a frame over 128 frames, a quarter turn); when it ends sets
 * turnHeld 1 and lets the script pick the next state.
 */
void EmLaoshan_TurnRightStart(Em *em)
{
    EmLaoshanWork *w = (EmLaoshanWork *)&em->work;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = LAOSHAN_STANCE_ALL_FOURS;
        Em_SetMotion(em, LAOSHAN_MOT_TURN_RIGHT_START, 0, 0);
        break;
    case 1:
        if (em->motion[0].blend == 0) {
            if ((EmMotion_ReachedFrame(em, 0, 28.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 156.0f) == 0)) {
                em->rotY += 0x80;
            }
            if (EmLaoshan_LayerMotionDone(em, 0) != 0) {
                w->turnHeld = 1;
            }
        }
        break;
    }
}

/*
 * LAOSHAN_MOVE_TURN_RIGHT_HOLD: plays LAOSHAN_MOT_TURN_RIGHT_HOLD and lets
 * the script pick the next state when it ends.
 */
void EmLaoshan_TurnRightHold(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = LAOSHAN_STANCE_ALL_FOURS;
        Em_SetMotion(em, LAOSHAN_MOT_TURN_RIGHT_HOLD, 0, 0);
        break;
    case 1:
        EmLaoshan_LayerMotionDone(em, 0);
        break;
    }
}

/*
 * LAOSHAN_MOVE_TURN_RIGHT_END: plays LAOSHAN_MOT_TURN_RIGHT_END, keeping
 * the new heading; clears turnHeld when it ends.
 */
void EmLaoshan_TurnRightEnd(Em *em)
{
    EmLaoshanWork *w = (EmLaoshanWork *)&em->work;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = LAOSHAN_STANCE_ALL_FOURS;
        Em_SetMotion(em, LAOSHAN_MOT_TURN_RIGHT_END, 0, 0);
        break;
    case 1:
        if (EmLaoshan_LayerMotionDone(em, 0) != 0) {
            w->turnHeld = 0;
        }
        break;
    }
}

/*
 * LAOSHAN_MOVE_TURN_RIGHT_BACK: plays LAOSHAN_MOT_TURN_RIGHT_BACK, turning
 * back (rotY falling 0x80 a frame, a quarter turn); clears turnHeld when it
 * ends.
 */
void EmLaoshan_TurnRightBack(Em *em)
{
    EmLaoshanWork *w = (EmLaoshanWork *)&em->work;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = LAOSHAN_STANCE_ALL_FOURS;
        Em_SetMotion(em, LAOSHAN_MOT_TURN_RIGHT_BACK, 0, 0);
        break;
    case 1:
        if (em->motion[0].blend == 0) {
            if ((EmMotion_ReachedFrame(em, 0, 30.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 158.0f) == 0)) {
                em->rotY -= 0x80;
            }
            if (EmLaoshan_LayerMotionDone(em, 0) != 0) {
                w->turnHeld = 0;
            }
        }
        break;
    }
}

/*
 * LAOSHAN_MOVE_TURN_LEFT_START: plays LAOSHAN_MOT_TURN_LEFT_START (rotY
 * falling 0x80 a frame, a quarter turn); when it ends sets turnHeld 2 and
 * lets the script pick the next state.
 */
void EmLaoshan_TurnLeftStart(Em *em)
{
    EmLaoshanWork *w = (EmLaoshanWork *)&em->work;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = LAOSHAN_STANCE_ALL_FOURS;
        Em_SetMotion(em, LAOSHAN_MOT_TURN_LEFT_START, 0, 0);
        break;
    case 1:
        if (em->motion[0].blend == 0) {
            if ((EmMotion_ReachedFrame(em, 0, 28.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 156.0f) == 0)) {
                em->rotY -= 0x80;
            }
            if (EmLaoshan_LayerMotionDone(em, 0) != 0) {
                w->turnHeld = 2;
            }
        }
        break;
    }
}

/*
 * LAOSHAN_MOVE_TURN_LEFT_HOLD: plays LAOSHAN_MOT_TURN_LEFT_HOLD and lets
 * the script pick the next state when it ends.
 */
void EmLaoshan_TurnLeftHold(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = LAOSHAN_STANCE_ALL_FOURS;
        Em_SetMotion(em, LAOSHAN_MOT_TURN_LEFT_HOLD, 0, 0);
        break;
    case 1:
        EmLaoshan_LayerMotionDone(em, 0);
        break;
    }
}

/*
 * LAOSHAN_MOVE_TURN_LEFT_END: plays LAOSHAN_MOT_TURN_LEFT_END, keeping the
 * new heading; clears turnHeld when it ends.
 */
void EmLaoshan_TurnLeftEnd(Em *em)
{
    EmLaoshanWork *w = (EmLaoshanWork *)&em->work;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = LAOSHAN_STANCE_ALL_FOURS;
        Em_SetMotion(em, LAOSHAN_MOT_TURN_LEFT_END, 0, 0);
        break;
    case 1:
        if (EmLaoshan_LayerMotionDone(em, 0) != 0) {
            w->turnHeld = 0;
        }
        break;
    }
}

/*
 * LAOSHAN_MOVE_TURN_LEFT_BACK: plays LAOSHAN_MOT_TURN_LEFT_BACK, turning
 * back (rotY rising 0x80 a frame); clears turnHeld when it ends.
 */
void EmLaoshan_TurnLeftBack(Em *em)
{
    EmLaoshanWork *w = (EmLaoshanWork *)&em->work;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = LAOSHAN_STANCE_ALL_FOURS;
        Em_SetMotion(em, LAOSHAN_MOT_TURN_LEFT_BACK, 0, 0);
        break;
    case 1:
        if (em->motion[0].blend == 0) {
            if ((EmMotion_ReachedFrame(em, 0, 30.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 158.0f) == 0)) {
                em->rotY += 0x80;
            }
            if (EmLaoshan_LayerMotionDone(em, 0) != 0) {
                w->turnHeld = 0;
            }
        }
        break;
    }
}

/*
 * LAOSHAN_MOVE_HALF_TURN_RIGHT: plays LAOSHAN_MOT_HALF_TURN_RIGHT, rotY
 * rising 0x100 a frame between frames 82 and 210 (a half turn); clears
 * turnHeld when it ends.
 */
void EmLaoshan_HalfTurnRight(Em *em)
{
    EmLaoshanWork *w = (EmLaoshanWork *)&em->work;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = LAOSHAN_STANCE_ALL_FOURS;
        Em_SetMotion(em, LAOSHAN_MOT_HALF_TURN_RIGHT, 0, 0);
        break;
    case 1:
        if (em->motion[0].blend == 0) {
            if ((EmMotion_ReachedFrame(em, 0, 82.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 210.0f) == 0)) {
                em->rotY += 0x100;
            }
            if (EmLaoshan_LayerMotionDone(em, 0) != 0) {
                w->turnHeld = 0;
            }
        }
        break;
    }
}

/*
 * LAOSHAN_MOVE_HALF_TURN_LEFT: plays LAOSHAN_MOT_HALF_TURN_LEFT, rotY
 * falling 0x100 a frame (a half turn); clears turnHeld when it ends.
 */
void EmLaoshan_HalfTurnLeft(Em *em)
{
    EmLaoshanWork *w = (EmLaoshanWork *)&em->work;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = LAOSHAN_STANCE_ALL_FOURS;
        Em_SetMotion(em, LAOSHAN_MOT_HALF_TURN_LEFT, 0, 0);
        break;
    case 1:
        if (em->motion[0].blend == 0) {
            if ((EmMotion_ReachedFrame(em, 0, 82.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 210.0f) == 0)) {
                em->rotY -= 0x100;
            }
            if (EmLaoshan_LayerMotionDone(em, 0) != 0) {
                w->turnHeld = 0;
            }
        }
        break;
    }
}

/*
 * LAOSHAN_MOVE_QUARTER_TURN_RIGHT: plays LAOSHAN_MOT_QUARTER_TURN_RIGHT,
 * rotY rising 0x7E a frame between frames 204 and 334 plus 8 at the end (a
 * quarter turn); clears turnHeld.
 */
void EmLaoshan_QuarterTurnRight(Em *em)
{
    EmLaoshanWork *w = (EmLaoshanWork *)&em->work;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = LAOSHAN_STANCE_ALL_FOURS;
        Em_SetMotion(em, LAOSHAN_MOT_QUARTER_TURN_RIGHT, 0, 0);
        break;
    case 1:
        if (em->motion[0].blend == 0) {
            if ((EmMotion_ReachedFrame(em, 0, 204.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 334.0f) == 0)) {
                em->rotY += 0x7E;
            }
            if (EmLaoshan_LayerMotionDone(em, 0) != 0) {
                w->turnHeld = 0;
                em->rotY += 8;
            }
        }
        break;
    }
}

/*
 * LAOSHAN_MOVE_QUARTER_TURN_LEFT: the mirror of
 * LAOSHAN_MOVE_QUARTER_TURN_RIGHT (rotY falling) with
 * LAOSHAN_MOT_QUARTER_TURN_LEFT; clears turnHeld.
 */
void EmLaoshan_QuarterTurnLeft(Em *em)
{
    EmLaoshanWork *w = (EmLaoshanWork *)&em->work;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = LAOSHAN_STANCE_ALL_FOURS;
        Em_SetMotion(em, LAOSHAN_MOT_QUARTER_TURN_LEFT, 0, 0);
        break;
    case 1:
        if (em->motion[0].blend == 0) {
            if ((EmMotion_ReachedFrame(em, 0, 204.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 334.0f) == 0)) {
                em->rotY -= 0x7E;
            }
            if (EmLaoshan_LayerMotionDone(em, 0) != 0) {
                w->turnHeld = 0;
                em->rotY -= 8;
            }
        }
        break;
    }
}

/*
 * LAOSHAN_MOVE_WALK_REARED: in LAOSHAN_STANCE_REARED walks to
 * the goal with LAOSHAN_MOT_REARED_WALK (Em_StepToGoal) and ends with the
 * motion.
 */
void EmLaoshan_WalkReared(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = LAOSHAN_STANCE_REARED;
        Em_SetMotion(em, LAOSHAN_MOT_REARED_WALK, 0, 0);
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

/*
 * LAOSHAN_MOVE_WALK_ATTACK: walks to the goal with LAOSHAN_MOT_WALK_ATTACK,
 * creating LaoShanAttack kinds 0x42 (frame 34) and 0x43 (frame 410) and a
 * sound over frames 0xAA to 0x168, and ends with the motion.
 */
void EmLaoshan_WalkAttack(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = LAOSHAN_STANCE_ALL_FOURS;
        Em_SetMotion(em, LAOSHAN_MOT_WALK, 0, 0);
        Em_SetMotion(em, LAOSHAN_MOT_WALK_ATTACK, 0, 0);
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

/*
 * LAOSHAN_MOVE_WALK_ON: walks to the goal (LAOSHAN_MOT_WALK), then keeps
 * moving by the motion delta (Em_MotionDelta, Em_MoveRotated) until the
 * motion ends.
 */
void EmLaoshan_WalkOn(Em *em)
{
    f32 delta[3];
    s32 moveAng[3];

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = LAOSHAN_STANCE_ALL_FOURS;
        Em_SetMotion(em, LAOSHAN_MOT_WALK, 0, 0);
        break;
    case 1:
        if (Em_StepToGoal(em, 0x40, 0) == 1) {
            em->step++;
        }
        break;
    case 2:
        Em_MotionDelta(em, delta);
        Vec_Set(&em->velX, delta[0], delta[1], -delta[2]);
        moveAng[0] = 0;
        moveAng[1] = em->rotY;
        moveAng[2] = 0;
        Em_MoveRotated(em, moveAng);
        if (em->motion[0].playing == 0) {
            em->step++;
            EmLaoshan_EndState(em);
        }
        break;
    }
}

/*
 * LAOSHAN_MOVE_WALK_BACK_ON: as LAOSHAN_MOVE_WALK_ON with
 * LAOSHAN_MOT_WALK_BACK and Em_StepToGoal flag 4.
 */
void EmLaoshan_WalkBackOn(Em *em)
{
    f32 delta[3];
    s32 moveAng[3];

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = LAOSHAN_STANCE_ALL_FOURS;
        Em_SetMotion(em, LAOSHAN_MOT_WALK_BACK, 0, 0);
        break;
    case 1:
        if (Em_StepToGoal(em, 0x40, 4) == 1) {
            em->step++;
        }
        break;
    case 2:
        Em_MotionDelta(em, delta);
        Vec_Set(&em->velX, delta[0], delta[1], -delta[2]);
        moveAng[0] = 0;
        moveAng[1] = em->rotY;
        moveAng[2] = 0;
        Em_MoveRotated(em, moveAng);
        if (em->motion[0].playing == 0) {
            em->step++;
            EmLaoshan_EndState(em);
        }
        break;
    }
}

/* Runs the move state (EM_GROUP_MOVE) by em->state. */
void EmLaoshan_RunMoveState(Em *em)
{
    switch (em->state) {
    case LAOSHAN_MOVE_WALK:
        EmLaoshan_Walk(em);
        break;
    case LAOSHAN_MOVE_WALK_BACK:
        EmLaoshan_WalkBack(em);
        break;
    case LAOSHAN_MOVE_TURN_TO_GOAL:
        EmLaoshan_TurnToGoal(em);
        break;
    case LAOSHAN_MOVE_WALK_TURN_RIGHT:
        EmLaoshan_WalkTurnRight(em);
        break;
    case LAOSHAN_MOVE_TURN_RIGHT_START:
        EmLaoshan_TurnRightStart(em);
        break;
    case LAOSHAN_MOVE_TURN_RIGHT_HOLD:
        EmLaoshan_TurnRightHold(em);
        break;
    case LAOSHAN_MOVE_TURN_RIGHT_END:
        EmLaoshan_TurnRightEnd(em);
        break;
    case LAOSHAN_MOVE_TURN_RIGHT_BACK:
        EmLaoshan_TurnRightBack(em);
        break;
    case LAOSHAN_MOVE_TURN_LEFT_START:
        EmLaoshan_TurnLeftStart(em);
        break;
    case LAOSHAN_MOVE_TURN_LEFT_HOLD:
        EmLaoshan_TurnLeftHold(em);
        break;
    case LAOSHAN_MOVE_TURN_LEFT_END:
        EmLaoshan_TurnLeftEnd(em);
        break;
    case LAOSHAN_MOVE_TURN_LEFT_BACK:
        EmLaoshan_TurnLeftBack(em);
        break;
    case LAOSHAN_MOVE_HALF_TURN_RIGHT:
        EmLaoshan_HalfTurnRight(em);
        break;
    case LAOSHAN_MOVE_HALF_TURN_LEFT:
        EmLaoshan_HalfTurnLeft(em);
        break;
    case LAOSHAN_MOVE_QUARTER_TURN_RIGHT:
        EmLaoshan_QuarterTurnRight(em);
        break;
    case LAOSHAN_MOVE_QUARTER_TURN_LEFT:
        EmLaoshan_QuarterTurnLeft(em);
        break;
    case LAOSHAN_MOVE_WALK_REARED:
        EmLaoshan_WalkReared(em);
        break;
    case LAOSHAN_MOVE_WALK_ATTACK:
        EmLaoshan_WalkAttack(em);
        break;
    case LAOSHAN_MOVE_WALK_ON:
        EmLaoshan_WalkOn(em);
        break;
    case LAOSHAN_MOVE_WALK_BACK_ON:
        EmLaoshan_WalkBackOn(em);

    default:
        break;
    }
}

/* The empty fly dispatcher (EM_GROUP_FLY): the family has no fly states. */
void EmLaoshan_RunFlyState(void)
{
}

/*
 * LAOSHAN_ATK_REARED_STRIKE: the reared attack LAOSHAN_MOT_REARED_STRIKE
 * (its hits come from EmLaoshan_MotionEffects), ending with the motion.
 */
void EmLaoshan_RearedStrike(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = LAOSHAN_STANCE_REARED;
        Em_SetMotion(em, LAOSHAN_MOT_REARED_STRIKE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmLaoshan_EndState(em);
        }
        break;
    }
}

/*
 * LAOSHAN_ATK_REARED_SMASH_FORT: plays LAOSHAN_MOT_REARED_SMASH; at frame
 * 160, in fortress areas 4, 5 and 2, creates a LaoShanAttack at the fixed
 * high fort point of the area (kinds 0x44, 0x45, 0x46) with
 * EmLaoshan_SpawnSmashImpactEffects.
 */
void EmLaoshan_RearedSmashFort(Em *em)
{
    f32 pos[3];

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = LAOSHAN_STANCE_REARED;
        Em_SetMotion(em, LAOSHAN_MOT_REARED_SMASH, 0, 0);
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
                EmLaoshan_SpawnSmashImpactEffects(em);
            }
            if ((Area_DayId(em->area) & 0xFFFF) == AREA_FORTRESS_5) {
                Vec_Set(pos, 7300.0f, 1700.0f, 11700.0f);
                LaoShanAttack_CreateAt(em, (f32 *) pos, 0x45);
                EmLaoshan_SpawnSmashImpactEffects(em);
            }
            if ((Area_DayId(em->area) & 0xFFFF) == AREA_FORTRESS_2) {
                Vec_Set(pos, 14600.0f, 1700.0f, 23700.0f);
                LaoShanAttack_CreateAt(em, (f32 *) pos, 0x46);
                EmLaoshan_SpawnSmashImpactEffects(em);
            }
        }
        break;
    }
}

/*
 * LAOSHAN_ATK_RAM_FORT: plays LAOSHAN_MOT_RAM; at frame 210 spawns
 * EmLaoshan_SpawnRamImpactEffects and, on the host, counts fortHits up to
 * 10 (the tenth calls Quest_TriggerEvent) and copies it to
 * gScene.fortStage.
 */
void EmLaoshan_RamFort(Em *em)
{
    EmLaoshanWork *w = (EmLaoshanWork *)&em->work;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = LAOSHAN_STANCE_ALL_FOURS;
        Em_SetMotion(em, LAOSHAN_MOT_RAM, 0, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 210.0f, 0U) != 0) {
            EmLaoshan_SpawnRamImpactEffects(em);
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

/*
 * LAOSHAN_ATK_REARED_STRIKE_TURN: LAOSHAN_MOT_REARED_STRIKE with turns:
 * rotY rising 0x13E9/21 a frame between frames 36 and 78, falling 0x13E9/38
 * a frame between 434 and 510, then rising 10 a frame.
 */
void EmLaoshan_RearedStrikeTurn(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = LAOSHAN_STANCE_REARED;
        Em_SetMotion(em, LAOSHAN_MOT_REARED_STRIKE, 0, 0);
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

/*
 * LAOSHAN_ATK_RAM_FORT_STAGE2: LAOSHAN_MOT_RAM; at frame 210 sets stage 2
 * (host) and spawns EmLaoshan_SpawnRamImpactEffects; no fortHits.
 */
void EmLaoshan_RamFortStage2(Em *em)
{
    EmLaoshanWork *w = (EmLaoshanWork *)&em->work;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = LAOSHAN_STANCE_ALL_FOURS;
        Em_SetMotion(em, LAOSHAN_MOT_RAM, 0, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 210.0f, 0U) != 0) {
            if (em->remote == 0) {
                w->stage = 2;
            }
            EmLaoshan_SpawnRamImpactEffects(em);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmLaoshan_EndState(em);
        }
        break;
    }
}

/*
 * LAOSHAN_ATK_RAM_FORT_STAGE3: LAOSHAN_MOT_RAM; at frame 210 sets stage 3
 * (host), sets gScene.emEventFlags bit 0 (tested by area objects) and
 * spawns EmLaoshan_SpawnRamImpactEffects.
 */
void EmLaoshan_RamFortStage3(Em *em)
{
    EmLaoshanWork *w = (EmLaoshanWork *)&em->work;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = LAOSHAN_STANCE_ALL_FOURS;
        Em_SetMotion(em, LAOSHAN_MOT_RAM, 0, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 210.0f, 0U) != 0) {
            if (em->remote == 0) {
                w->stage = 3;
            }
            gScene.emEventFlags |= 1;
            EmLaoshan_SpawnRamImpactEffects(em);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmLaoshan_EndState(em);
        }
        break;
    }
}

/* Runs the attack state (EM_GROUP_ATTACK) by em->state. */
void EmLaoshan_RunAttackState(Em *em)
{
    switch (em->state) {
    case LAOSHAN_ATK_REARED_STRIKE:
        EmLaoshan_RearedStrike(em);
        break;
    case LAOSHAN_ATK_REARED_SMASH_FORT:
        EmLaoshan_RearedSmashFort(em);
        break;
    case LAOSHAN_ATK_RAM_FORT:
        EmLaoshan_RamFort(em);
        break;
    case LAOSHAN_ATK_REARED_STRIKE_TURN:
        EmLaoshan_RearedStrikeTurn(em);
        break;
    case LAOSHAN_ATK_RAM_FORT_STAGE2:
        EmLaoshan_RamFortStage2(em);
        break;
    case LAOSHAN_ATK_RAM_FORT_STAGE3:
        EmLaoshan_RamFortStage3(em);

    default:
        break;
    }
}

/*
 * LAOSHAN_REACT_FALL: LAOSHAN_MOT_FLINCH to frame 28, then falls
 * (LAOSHAN_MOT_FALL) and gets up (LAOSHAN_MOT_GET_UP); restarts the script.
 * Update enters it when a flinch of part 2 finds the part at state 2 or
 * more.
 */
void EmLaoshan_ReactFall(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = LAOSHAN_STANCE_ALL_FOURS;
        Em_SetMotion(em, LAOSHAN_MOT_FLINCH, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 28.0f, 0U) != 0) {
            em->step++;
            Em_SetMotion(em, LAOSHAN_MOT_FALL, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, LAOSHAN_MOT_GET_UP, 0, 0);
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

/*
 * LAOSHAN_REACT_REARED_FLINCH: the reared flinch LAOSHAN_MOT_REARED_FLINCH
 * (EM_DMG_PART_FLINCH while reared); restarts the script.
 */
void EmLaoshan_ReactRearedFlinch(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = LAOSHAN_STANCE_REARED;
        Em_SetMotion(em, LAOSHAN_MOT_REARED_FLINCH, 0, 0);
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

/*
 * LAOSHAN_REACT_PUSHED_BACK: LAOSHAN_MOT_PUSHED_BACK moving backward (velZ
 * about -26.9, Em_MoveRotated) until frame 46, then ends with the motion
 * (EM_DMG_ATTACK9 on all fours); restarts the script.
 */
void EmLaoshan_ReactPushedBack(Em *em)
{
    s32 moveAng[3];

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = LAOSHAN_STANCE_ALL_FOURS;
        Em_SetMotion(em, LAOSHAN_MOT_PUSHED_BACK, 0, 0);
        Em_RestartScript(em);
        Player_ClearVelocity(em);
        em->velZ = -26.913044f;
        break;
    case 1:
        if (em->motion[0].blend == 0) {
            moveAng[0] = 0;
            moveAng[1] = em->rotY;
            moveAng[2] = 0;
            Em_MoveRotated(em, moveAng);
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

/*
 * LAOSHAN_REACT_FLINCH: LAOSHAN_MOT_FLINCH on all fours
 * (EM_DMG_PART_FLINCH); restarts the script.
 */
void EmLaoshan_ReactFlinch(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = LAOSHAN_STANCE_ALL_FOURS;
        Em_SetMotion(em, LAOSHAN_MOT_FLINCH, 0, 0);
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

/*
 * LAOSHAN_REACT_REAR_DOWN: drops from reared to all fours
 * (LAOSHAN_MOT_REAR_DOWN, then LAOSHAN_STANCE_ALL_FOURS) as a reaction; Update does
 * not enter it.
 */
void EmLaoshan_ReactRearDown(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = LAOSHAN_STANCE_REARED;
        Em_SetMotion(em, LAOSHAN_MOT_REAR_DOWN, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stance = LAOSHAN_STANCE_ALL_FOURS;
            EmLaoshan_EndState(em);
        }
        break;
    }
}

/*
 * LAOSHAN_REACT_REARED_FLINCH_DOWN: LAOSHAN_MOT_REARED_FLINCH then
 * LAOSHAN_MOT_REAR_DOWN back to all fours (EM_DMG_ATTACK9 while reared);
 * restarts the script.
 */
void EmLaoshan_ReactRearedFlinchDown(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = LAOSHAN_STANCE_REARED;
        Em_SetMotion(em, LAOSHAN_MOT_REARED_FLINCH, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, LAOSHAN_MOT_REAR_DOWN, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stance = LAOSHAN_STANCE_ALL_FOURS;
            EmLaoshan_EndState(em);
        }
        break;
    }
}

/*
 * LAOSHAN_REACT_FALL_LONG: as LAOSHAN_REACT_FALL but stays down 300 frames
 * before LAOSHAN_MOT_GET_UP. Update enters it when a flinch of part 3 finds
 * the part at state 2 or more.
 */
void EmLaoshan_ReactFallLong(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = LAOSHAN_STANCE_ALL_FOURS;
        Em_SetMotion(em, LAOSHAN_MOT_FLINCH, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 28.0f, 0U) != 0) {
            em->step++;
            Em_SetMotion(em, LAOSHAN_MOT_FALL, 0, 0);
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
            Em_SetMotion(em, LAOSHAN_MOT_GET_UP, 0, 0);
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

/* Runs the reaction state (EM_GROUP_REACT) by em->state. */
void EmLaoshan_RunReactState(Em *em)
{
    switch (em->state) {
    case LAOSHAN_REACT_FALL:
        EmLaoshan_ReactFall(em);
        break;
    case LAOSHAN_REACT_REARED_FLINCH:
        EmLaoshan_ReactRearedFlinch(em);
        break;
    case LAOSHAN_REACT_PUSHED_BACK:
        EmLaoshan_ReactPushedBack(em);
        break;
    case LAOSHAN_REACT_FLINCH:
        EmLaoshan_ReactFlinch(em);
        break;
    case LAOSHAN_REACT_REAR_DOWN:
        EmLaoshan_ReactRearDown(em);
        break;
    case LAOSHAN_REACT_REARED_FLINCH_DOWN:
        EmLaoshan_ReactRearedFlinchDown(em);
        break;
    case LAOSHAN_REACT_FALL_LONG:
        EmLaoshan_ReactFallLong(em);

    default:
        break;
    }
}

/*
 * LAOSHAN_DEFEAT_DIE: QuestEm_Hunted and CamReq_BossHunted, rears up
 * (LAOSHAN_MOT_REAR_UP to frame 450), collapses (LAOSHAN_MOT_DIE), becomes
 * EM_STANCE_DEAD without field collision, adds the carcass mine points and
 * keeps them updated. Debug switch 0x42 revives it (hp 0x3415, points
 * removed, EmLaoshan_EndState).
 */
void EmLaoshan_Die(Em *em)
{
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, LAOSHAN_MOT_REAR_UP, 0, 0);
        em->timer = 0;
        em->lookAtTarget = 0;
        em->stance = LAOSHAN_STANCE_ALL_FOURS;
        QuestEm_Hunted(em);
        CamReq_BossHunted();
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 450.0f, 0) != 0) {
            em->step++;
            em->stance = LAOSHAN_STANCE_REARED;
            Em_SetMotion(em, LAOSHAN_MOT_DIE, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stance = EM_STANCE_DEAD;
            EmLaoshan_AddCarcassMinePoints(em);
            em->fieldCollide = 0;
        }
        break;
    case 3:
        if (Debug_IsOn(0x42) != 0) {
            em->step = 0x63;
        } else {
            EmLaoshan_UpdateCarcassMinePoints(em);
        }
        break;
    case 0x63:
        em->step++;
        EmLaoshan_RemoveCarcassMinePoints(em);
        em->hp = 0x3415;
        EmLaoshan_EndState(em);
        break;
    }
}

/*
 * LAOSHAN_DEFEAT_DIE_REARED: as LAOSHAN_DEFEAT_DIE but collapses at once
 * (LAOSHAN_MOT_DIE) from the reared stance; Update does not enter it.
 */
void EmLaoshan_DieReared(Em *em)
{
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = LAOSHAN_STANCE_REARED;
        Em_SetMotion(em, LAOSHAN_MOT_DIE, 0, 0);
        QuestEm_Hunted(em);
        CamReq_BossHunted();
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stance = EM_STANCE_DEAD;
            EmLaoshan_AddCarcassMinePoints(em);
            em->fieldCollide = 0;
        }
        break;
    case 2:
        if (Debug_IsOn(0x42) != 0) {
            em->step = 0x63;
        } else {
            EmLaoshan_UpdateCarcassMinePoints(em);
        }
        break;
    case 0x63:
        em->step++;
        em->hp = 0x3415;
        EmLaoshan_RemoveCarcassMinePoints(em);
        EmLaoshan_EndState(em);
        break;
    }
}

/* Runs the defeat state (EM_GROUP_DEFEAT) by em->state. */
void EmLaoshan_RunDefeatState(Em *em)
{
    switch (em->state) {
    case LAOSHAN_DEFEAT_DIE:
        EmLaoshan_Die(em);
        break;
    case LAOSHAN_DEFEAT_DIE_REARED:
        EmLaoshan_DieReared(em);
        break;
    }
}

/*
 * LAOSHAN_EVENT_INTRO: the opening scene in fortress area 1 (no hits, no
 * sensing): resets the size, idles 240 frames, walks a fixed path, moves to
 * a fixed spot, rears up, strikes and drops (REAR_UP, REARED_IDLE,
 * REARED_STRIKE, REAR_DOWN motions), sets stage 1, idles until the save
 * event flag of the kind is set, then ends.
 */
void EmLaoshan_IntroScene(Em *em)
{
    EmLaoshanWork *w = (EmLaoshanWork *)&em->work;
    f32 delta[3];

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        GmEm_ResetSize();
        em->step++;
        em->stance = LAOSHAN_STANCE_ALL_FOURS;
        Em_SetMotion(em, LAOSHAN_MOT_IDLE, 0, 0);
        em->timer = 0xF0;
        em->posX = 8150.0f;
        em->posY = 0.0f;
        em->posZ = 9310.0f;
        em->goal[0] = 10660.0f;
        em->goal[1] = 0.0f;
        em->goal[2] = 9250.0f;
        em->rotY = Math_AngleXZ(&em->posX, em->goal);
        em->goalDist = Vec_DistanceXZ(&em->posX, em->goal);
        w->stage = 1;
        em->scriptPending = 0;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, LAOSHAN_MOT_WALK, 0, 0);
        }
        break;
    case 2:
        Em_MotionDelta(em, delta);
        em->goalDist -= delta[2];
        if (em->goalDist <= 0.0f) {
            em->step++;
            em->posX = 13600.0f;
            em->posY = 0.0f;
            em->posZ = 10750.0f;
            em->rotY = 0x25E0;
            Em_SetMotion(em, LAOSHAN_MOT_REAR_UP, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, LAOSHAN_MOT_REARED_IDLE, 0, 0);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, LAOSHAN_MOT_REARED_STRIKE, 0, 0);
        }
        break;
    case 5:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, LAOSHAN_MOT_REAR_DOWN, 0, 0);
        }
        break;
    case 6:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, LAOSHAN_MOT_IDLE, 0, 0);
        }
        break;
    case 7:
        if (em->type == EM_KIND_LAO_SHAN_LUNG) {
            if (Save_TestEventFlag(EVENT_FLAG_LAO_SHAN_SCENE) == 1) {
                em->step++;
                EmLaoshan_EndState(em);
            }
        } else {
            if (Save_TestEventFlag(EVENT_FLAG_ASHEN_LAO_SHAN_SCENE) == 1) {
                em->step++;
                EmLaoshan_EndState(em);
            }
        }
        break;
    }
}

/*
 * LAOSHAN_EVENT_REPELLED_LEAVE: in fortress area 5 once the quest goal is
 * met: from a fixed spot plays LAOSHAN_MOT_LEAVE_START, turns
 * (LAOSHAN_MOT_LEAVE_TURN, rotY rising 0x210 a frame to frame 64), walks
 * with LAOSHAN_MOT_WALK_ATTACK to two fixed goals, then stays in step 5 for
 * good.
 */
void EmLaoshan_RepelledLeave(Em *em)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = LAOSHAN_STANCE_ALL_FOURS;
        Em_SetMotion(em, LAOSHAN_MOT_LEAVE_START, 0, 0);
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
            Em_SetMotion(em, LAOSHAN_MOT_LEAVE_TURN, 0, 0);
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
                Em_SetMotion(em, LAOSHAN_MOT_WALK, 0, 0);
                Em_SetMotion(em, LAOSHAN_MOT_WALK_ATTACK, 0, 0);
                break;
            }
        }
        break;
    case 3:
        if ((em->motion[1].playing == 0) && (em->motionId[1] == EM_MOTION_ID(1, LAOSHAN_MOT_WALK_ATTACK))) {
            Em_SetLayerMotion(em, EM_MOTION_ID(1, LAOSHAN_MOT_WALK), 0, 0, 1);
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
            Em_SetMotion(em, LAOSHAN_MOT_WALK, 0, 0);
        }
        break;
    }
}

/* Runs the event state (groups 6 and 7) by em->state. */
void EmLaoshan_RunEventState(Em *em)
{
    switch (em->state) {
    case LAOSHAN_EVENT_INTRO:
        EmLaoshan_IntroScene(em);
        break;
    case LAOSHAN_EVENT_REPELLED_LEAVE:
        EmLaoshan_RepelledLeave(em);
        break;
    }
}

/*
 * Per-frame update: keeps the living mine point at its spot (removed when
 * emptied), counts turnHeldFrames, counts quakeTimer down, sends the state
 * every 150 frames on the host outside EM_GROUP_EVENT (QSync_SendEm), maps the
 * Em_UpdateDamage result to states, starts the intro scene or the repelled
 * exit by area, runs the pending script, then runs the state (twice on a
 * stateRequest).
 */
void EmLaoshan_Update(Em *em)
{
    EmLaoshanWork *w = (EmLaoshanWork *)&em->work;
    f32 pos[3];
    u8 dmgInfo[4];
    s8 count;

    if (w->mineSlot != -1) {
        EmLaoshan_GetMinePointPos(em, pos);
        MinePoint_SetPos(w->mineSlot, pos);
        count = MinePoint_Count(w->mineSlot);
        w->mineCount = count;
        if (count <= 0) {
            MinePoint_Remove(w->mineSlot);
            w->mineSlot = -1;
        }
    }
    if (w->turnHeld != 0) {
        (w->turnHeldFrames)++;
    }
    if (em->quakeTimer != 0) {
        if (--em->quakeTimer <= 0) {
            em->quakeTimer = 0;
        }
    }
    if (w->syncTimer != 0 && em->stateGroup != EM_GROUP_EVENT && em->remote == 0) {
        if (--w->syncTimer <= 0) {
            w->syncTimer = 150;
            QSync_SendEm(em, QSYNC_EMMSG_STATE_NO_PARTS, 0);
        }
    }
    switch (Em_UpdateDamage(em, dmgInfo) & 0xFF) {
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
        EmLaoshan_ChangeState(em, EM_GROUP_DEFEAT, LAOSHAN_DEFEAT_DIE, EM_SYNC_SEND_AND_SET);
        break;
    case EM_DMG_ATTACK9:
        if (em->stance == LAOSHAN_STANCE_REARED) {
            EmLaoshan_ChangeState(em, EM_GROUP_REACT, LAOSHAN_REACT_REARED_FLINCH_DOWN, EM_SYNC_SEND_AND_SET);
        } else {
            EmLaoshan_ChangeState(em, EM_GROUP_REACT, LAOSHAN_REACT_PUSHED_BACK, EM_SYNC_SEND_AND_SET);
        }
        break;
    case EM_DMG_STATUS5:
        Em_EndStatus5(em);
        break;
    case EM_DMG_PART_FLINCH:
        if (em->stance == LAOSHAN_STANCE_REARED) {
            EmLaoshan_ChangeState(em, EM_GROUP_REACT, LAOSHAN_REACT_REARED_FLINCH, EM_SYNC_SEND_AND_SET);
        } else {
            switch (em->flinchPart) {
            case 0:
            case 1:
            case 4:
            case 5:
            case 6:
            case 7:
                EmLaoshan_ChangeState(em, EM_GROUP_REACT, LAOSHAN_REACT_FLINCH, EM_SYNC_SEND_AND_SET);
                break;
            case 2:
                if (em->parts[em->flinchPart].state >= 2) {
                    EmLaoshan_ChangeState(em, EM_GROUP_REACT, LAOSHAN_REACT_FALL, EM_SYNC_SEND_AND_SET);
                } else {
                    EmLaoshan_ChangeState(em, EM_GROUP_REACT, LAOSHAN_REACT_FLINCH, EM_SYNC_SEND_AND_SET);
                }
                break;
            case 3:
                if (em->parts[em->flinchPart].state >= 2) {
                    EmLaoshan_ChangeState(em, EM_GROUP_REACT, LAOSHAN_REACT_FALL_LONG, EM_SYNC_SEND_AND_SET);
                } else {
                    EmLaoshan_ChangeState(em, EM_GROUP_REACT, LAOSHAN_REACT_FLINCH, EM_SYNC_SEND_AND_SET);
                }
                break;
            }
        }
        break;
    }
    switch (Area_DayId(em->area)) {
    case AREA_FORTRESS_1:
        if (em->type == EM_KIND_LAO_SHAN_LUNG) {
            if (Save_TestEventFlag(EVENT_FLAG_LAO_SHAN_SCENE) != 0) {
                /* the scene was seen: the script runs as in any area */
                goto normal;
            }
            if (gScene.eventRunning == 1 && em->stateGroup != EM_GROUP_EVENT) {
                EmLaoshan_ChangeState(em, EM_GROUP_EVENT, LAOSHAN_EVENT_INTRO, EM_SYNC_LOCAL);
            }
        } else {
            if (Save_TestEventFlag(EVENT_FLAG_ASHEN_LAO_SHAN_SCENE) != 0) {
                /* the scene was seen: the script runs as in any area */
                goto normal;
            }
            if (gScene.eventRunning == 1 && em->stateGroup != EM_GROUP_EVENT) {
                EmLaoshan_ChangeState(em, EM_GROUP_EVENT, LAOSHAN_EVENT_INTRO, EM_SYNC_LOCAL);
            }
        }
        break;
    case AREA_FORTRESS_5:
        if (Quest_GetOutcome(0) == 1 && em->stateGroup != EM_GROUP_EVENT && em->stateGroup != EM_GROUP_DEFEAT) {
            em->scriptPending = 0;
            if (gScene.area == em->area) {
                CamReq_ForMonster(em);
            }
            EmLaoshan_ChangeState(em, EM_GROUP_EVENT, LAOSHAN_EVENT_REPELLED_LEAVE, EM_SYNC_LOCAL);
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

/*
 * Runs the state dispatcher of em->stateGroup (group 7 also runs the event
 * dispatcher; ChangeState has no case for group 7).
 */
void EmLaoshan_RunState(Em *em)
{
    switch (em->stateGroup) {
    case EM_GROUP_IDLE:
        EmLaoshan_RunIdleState(em);
        break;
    case EM_GROUP_MOVE:
        EmLaoshan_RunMoveState(em);
        break;
    case EM_GROUP_FLY:
        EmLaoshan_RunFlyState();
        break;
    case EM_GROUP_ATTACK:
        EmLaoshan_RunAttackState(em);
        break;
    case EM_GROUP_REACT:
        EmLaoshan_RunReactState(em);
        break;
    case EM_GROUP_DEFEAT:
        EmLaoshan_RunDefeatState(em);
        break;
    case EM_GROUP_EVENT:
        EmLaoshan_RunEventState(em);
        break;
    case EM_GROUP_EVENT2:
        EmLaoshan_RunEventState(em);

    default:
        break;
    }
}

/*
 * Requests camera shake kind i (CamShake_ReqAIf) when the layer-0 motion
 * passes the frame.
 */
void EmLaoshan_CamShakeOnFrame(s32 *em, s32 frame, s32 shakeKind)
{
    if (EmMotion_PassedFrameScaled((Em *) em, (f32) frame, 0U) != 0) {
        CamShake_ReqAIf((Em *) em, shakeKind);
    }
}

/* The default case of the motion effects: does nothing. */
static void EmLaoshan_MotionEffectsDefault(Em *em)
{
}

/*
 * Per frame: tracks the layer-0 motion (fxMotion, clearing unk00A on a
 * change), plays EmLaoshan_MotionSounds, and by motion creates the step and
 * body hits (LaoShanAttack_Create, LaoShanAttack_CreateSpan), falling rocks
 * at the spots in turn (StageShot_CreateAtSpot, fxSpot), dust effects,
 * camera shakes and stage objects at fixed frames.
 */
void EmLaoshan_MotionEffects(Em *em, EmLaoshanWork *work)
{
    if (em->motionId[0] != work->fxMotion) {
        work->unk00A = 0;
        work->fxMotion = em->motionId[0];
    }
    EmLaoshan_MotionSounds(em, (struct EmWork *)work);
    switch (work->fxMotion) {
    case EM_MOTION_ID(0, LAOSHAN_MOT_IDLE):
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
    case EM_MOTION_ID(0, LAOSHAN_MOT_WALK_BACK):
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x92, 2);
        if (EmMotion_PassedFrameScaled(em, 146.0f, 0) != 0) {
            StageShot_CreateAtSpot(em, STAGESHOT_LAOSHAN_ROCK, work->fxSpot);
            work->fxSpot += 1;
            work->fxSpot &= 3;
        }
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0xB2, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x122, 2);
        if (EmMotion_PassedFrameScaled(em, 290.0f, 0) != 0) {
            StageShot_CreateAtSpot(em, STAGESHOT_LAOSHAN_ROCK, work->fxSpot);
            work->fxSpot += 1;
            work->fxSpot &= 3;
        }
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x17E, 2);
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
            PartFx_SpawnForEmScaled(em, 0xE, 0x13, 0.5f);
        }
        if (EmMotion_PassedFrameScaled(em, 380.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 8, 0x13, 0.5f);
        }
        if (EmMotion_PassedFrameScaled(em, 284.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0x1D, 0x13, 0.6f);
        }
        if (EmMotion_PassedFrameScaled(em, 136.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0x17, 0x13, 0.6f);
            break;
        }
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_WALK):
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x2C, 2);
        if (EmMotion_PassedFrameScaled(em, 44.0f, 0) != 0) {
            StageShot_CreateAtSpot(em, STAGESHOT_LAOSHAN_ROCK, work->fxSpot);
            work->fxSpot += 1;
            work->fxSpot &= 3;
        }
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x9A, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0xF8, 2);
        if (EmMotion_PassedFrameScaled(em, 248.0f, 0) != 0) {
            StageShot_CreateAtSpot(em, STAGESHOT_LAOSHAN_ROCK, work->fxSpot);
            work->fxSpot += 1;
            work->fxSpot &= 3;
        }
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x16E, 2);
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
            PartFx_SpawnForEmScaled(em, 0xE, 0x13, 0.5f);
        }
        if (EmMotion_PassedFrameScaled(em, 254.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 8, 0x13, 0.5f);
        }
        if (EmMotion_PassedFrameScaled(em, 360.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0x1D, 0x13, 0.6f);
        }
        if (EmMotion_PassedFrameScaled(em, 152.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0x17, 0x13, 0.6f);
            break;
        }
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_WALK_TURN_RIGHT):
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x52, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0xC4, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x13A, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x1AA, 2);
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
            PartFx_SpawnForEmScaled(em, 0xE, 0x13, 0.5f);
        }
        if (EmMotion_PassedFrameScaled(em, 80.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 8, 0x13, 0.5f);
        }
        if (EmMotion_PassedFrameScaled(em, 192.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0x1D, 0x13, 0.6f);
        }
        if (EmMotion_PassedFrameScaled(em, 424.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0x17, 0x13, 0.6f);
            break;
        }
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_WALK_TURN_LEFT):
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x52, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0xC4, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x13A, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x1AA, 2);
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
            PartFx_SpawnForEmScaled(em, 0xE, 0x13, 0.5f);
        }
        if (EmMotion_PassedFrameScaled(em, 310.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 8, 0x13, 0.5f);
        }
        if (EmMotion_PassedFrameScaled(em, 424.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0x1D, 0x13, 0.6f);
        }
        if (EmMotion_PassedFrameScaled(em, 192.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0x17, 0x13, 0.6f);
            break;
        }
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_STOMP_START):
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x52, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0xAA, 2);
        if (EmMotion_PassedFrameScaled(em, 40.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x11);
        }
        if (EmMotion_PassedFrameScaled(em, 80.0f, 0) != 0) {
            LaoShanAttack_CreateSpan(em, 0x47, 0x64);
        }
        if (EmMotion_PassedFrameScaled(em, 158.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0xE, 0x13, 0.5f);
        }
        if (EmMotion_PassedFrameScaled(em, 70.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0x17, 0x13, 0.6f);
            break;
        }
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_STOMP):
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x17C, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x1CC, 2);
        if (EmMotion_PassedFrameScaled(em, 326.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x12);
        }
        if (EmMotion_PassedFrameScaled(em, 80.0f, 0) != 0) {
            LaoShanAttack_CreateSpan(em, 0x47, 0xC8);
        }
        if (EmMotion_PassedFrameScaled(em, 450.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0xE, 0x13, 0.5f);
        }
        if (EmMotion_PassedFrameScaled(em, 368.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0x17, 0x13, 0.6f);
        }
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x51);
            ElementBurst_CreateOnEm(em, 5, 0x14, 1.5f);
            break;
        }
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_REAR_UP):
        if (EmMotion_PassedFrameScaled(em, 326.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x13);
        }
        if (EmMotion_PassedFrameScaled(em, 326.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x14);
            break;
        }
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_RAM):
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x3E, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x6A, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x96, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0xD2, 5);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x1A4, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x1DC, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x1FE, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x222, 2);
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
            PartFx_SpawnForEmScaled(em, 0xE, 0x13, 0.5f);
        }
        if ((EmMotion_PassedFrameScaled(em, 58.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 468.0f, 0) != 0)) {
            PartFx_SpawnForEmScaled(em, 8, 0x13, 0.5f);
        }
        if ((EmMotion_PassedFrameScaled(em, 136.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 504.0f, 0) != 0)) {
            PartFx_SpawnForEmScaled(em, 0x1D, 0x13, 0.6f);
        }
        if ((EmMotion_PassedFrameScaled(em, 192.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 404.0f, 0) != 0)) {
            PartFx_SpawnForEmScaled(em, 0x17, 0x13, 0.6f);
            break;
        }
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_REARED_IDLE):
        if (EmMotion_PassedFrameScaled(em, 130.0f, 0) != 0) {
            LaoShanAttack_CreateSpan(em, 0x47, 0x4C);
        }
        if (EmMotion_PassedFrameScaled(em, 320.0f, 0) != 0) {
            LaoShanAttack_CreateSpan(em, 0x47, 0x42);
            break;
        }
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_REARED_STRIKE):
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x4C, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x1FA, 2);
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
            PartFx_SpawnForEmScaled(em, 0x17, 0x13, 0.6f);
        }
        if (EmMotion_PassedFrameScaled(em, 494.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0x17, 0x13, 0.6f);
            break;
        }
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_REARED_SMASH):
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x56, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x9A, 5);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x106, 2);
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
            PartFx_SpawnForEmScaled(em, 0x1D, 0x13, 0.5f);
        }
        if (EmMotion_PassedFrameScaled(em, 258.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0x1D, 0x13, 0.5f);
            break;
        }
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_REAR_DOWN):
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x84, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x11C, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x1DA, 5);
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
            PartFx_SpawnForEmScaled(em, 0xE, 0x13, 0.8f);
        }
        if (EmMotion_PassedFrameScaled(em, 470.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 8, 0x13, 0.8f);
        }
        if (EmMotion_PassedFrameScaled(em, 274.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0x1D, 0x13, 0.6f);
        }
        if ((EmMotion_PassedFrameScaled(em, 114.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 394.0f, 0) != 0)) {
            PartFx_SpawnForEmScaled(em, 0x17, 0x13, 0.6f);
            break;
        }
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_REARED_WALK):
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x1E, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0xA8, 2);
        if (EmMotion_PassedFrameScaled(em, 26.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0x1D, 0x13, 0.5f);
        }
        if (EmMotion_PassedFrameScaled(em, 164.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0x17, 0x13, 0.6f);
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
    case EM_MOTION_ID(0, LAOSHAN_MOT_TURN_RIGHT_START):
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x52, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0xA6, 2);
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
            PartFx_SpawnForEmScaled(em, 0xE, 0x13, 0.5f);
        }
        if (EmMotion_PassedFrameScaled(em, 76.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 8, 0x13, 0.5f);
            break;
        }
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_TURN_RIGHT_HOLD):
    case EM_MOTION_ID(0, LAOSHAN_MOT_TURN_LEFT_HOLD):
        if (EmMotion_PassedFrameScaled(em, 28.0f, 0) != 0) {
            LaoShanAttack_CreateSpan(em, 0x47, 0x34);
        }
        if (EmMotion_PassedFrameScaled(em, 154.0f, 0) != 0) {
            LaoShanAttack_CreateSpan(em, 0x47, 0x32);
            break;
        }
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_TURN_RIGHT_BACK):
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x50, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x8C, 2);
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
            PartFx_SpawnForEmScaled(em, 0xE, 0x13, 0.5f);
        }
        if (EmMotion_PassedFrameScaled(em, 138.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 8, 0x13, 0.5f);
            break;
        }
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_TURN_RIGHT_END):
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x4A, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x9E, 2);
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
            PartFx_SpawnForEmScaled(em, 0x1D, 0x13, 0.6f);
        }
        if (EmMotion_PassedFrameScaled(em, 154.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0x17, 0x13, 0.6f);
            break;
        }
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_QUARTER_TURN_RIGHT):
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x52, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0xA6, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0xFC, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x150, 2);
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
            PartFx_SpawnForEmScaled(em, 0xE, 0x13, 0.5f);
        }
        if (EmMotion_PassedFrameScaled(em, 248.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 8, 0x13, 0.5f);
        }
        if (EmMotion_PassedFrameScaled(em, 78.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0x1D, 0x13, 0.6f);
        }
        if (EmMotion_PassedFrameScaled(em, 162.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0x17, 0x13, 0.6f);
            break;
        }
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_HALF_TURN_RIGHT):
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x50, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0xA2, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0xCE, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x11C, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x16A, 2);
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
            PartFx_SpawnForEmScaled(em, 0xE, 0x13, 0.5f);
        }
        if (EmMotion_PassedFrameScaled(em, 158.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 8, 0x13, 0.5f);
        }
        if (EmMotion_PassedFrameScaled(em, 280.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0x1D, 0x13, 0.6f);
        }
        if ((EmMotion_PassedFrameScaled(em, 76.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 358.0f, 0) != 0)) {
            PartFx_SpawnForEmScaled(em, 0x17, 0x13, 0.6f);
            break;
        }
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_TURN_LEFT_START):
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x52, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0xA6, 2);
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
            PartFx_SpawnForEmScaled(em, 0xE, 0x13, 0.5f);
        }
        if (EmMotion_PassedFrameScaled(em, 162.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 8, 0x13, 0.5f);
            break;
        }
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_TURN_LEFT_BACK):
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x50, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x8C, 2);
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
            PartFx_SpawnForEmScaled(em, 0xE, 0x13, 0.5f);
        }
        if (EmMotion_PassedFrameScaled(em, 78.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 8, 0x13, 0.5f);
            break;
        }
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_TURN_LEFT_END):
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x4A, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x9E, 2);
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
            PartFx_SpawnForEmScaled(em, 0x1D, 0x13, 0.6f);
        }
        if (EmMotion_PassedFrameScaled(em, 70.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0x17, 0x13, 0.6f);
            break;
        }
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_QUARTER_TURN_LEFT):
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x52, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0xA6, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0xFC, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x150, 2);
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
            PartFx_SpawnForEmScaled(em, 0xE, 0x13, 0.5f);
        }
        if (EmMotion_PassedFrameScaled(em, 334.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 8, 0x13, 0.5f);
        }
        if (EmMotion_PassedFrameScaled(em, 162.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0x1D, 0x13, 0.6f);
        }
        if (EmMotion_PassedFrameScaled(em, 78.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0x17, 0x13, 0.6f);
            break;
        }
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_HALF_TURN_LEFT):
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x50, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0xA2, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0xCE, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x11C, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x16A, 2);
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
            PartFx_SpawnForEmScaled(em, 0xE, 0x13, 0.5f);
        }
        if (EmMotion_PassedFrameScaled(em, 204.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 8, 0x13, 0.5f);
        }
        if ((EmMotion_PassedFrameScaled(em, 76.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 358.0f, 0) != 0)) {
            PartFx_SpawnForEmScaled(em, 0x1D, 0x13, 0.6f);
        }
        if (EmMotion_PassedFrameScaled(em, 280.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0x17, 0x13, 0.6f);
            break;
        }
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_FALL):
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x56, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x5A, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x90, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0xAA, 5);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0xDA, 2);
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
            LaoshanDust_Create(em, 0, 3, 1.2f);
        }
        if (EmMotion_PassedFrameScaled(em, 128.0f, 0) != 0) {
            LaoshanDust_Create(em, 0, 1, 1.4f);
        }
        if (EmMotion_PassedFrameScaled(em, 146.0f, 0) != 0) {
            LaoshanDust_Create(em, 0, 4, 1.2f);
            break;
        }
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_GET_UP):
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x64, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0xAE, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0xF0, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x146, 2);
        if (EmMotion_PassedFrameScaled(em, 94.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0xE, 0x13, 0.5f);
        }
        if (EmMotion_PassedFrameScaled(em, 156.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 8, 0x13, 0.5f);
        }
        if (EmMotion_PassedFrameScaled(em, 310.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0x1D, 0x13, 0.6f);
        }
        if (EmMotion_PassedFrameScaled(em, 230.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0x17, 0x13, 0.6f);
            break;
        }
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_REARED_FLINCH):
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_PUSHED_BACK):
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x14, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x18, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x40, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x68, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0xB4, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0xD6, 2);
        if (EmMotion_PassedFrameScaled(em, 186.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0xE, 0x13, 0.5f);
        }
        if ((EmMotion_PassedFrameScaled(em, 58.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 158.0f, 0) != 0)) {
            PartFx_SpawnForEmScaled(em, 8, 0x13, 0.5f);
        }
        if ((EmMotion_PassedFrameScaled(em, 32.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 210.0f, 0) != 0)) {
            PartFx_SpawnForEmScaled(em, 0x1D, 0x13, 0.6f);
        }
        if (EmMotion_PassedFrameScaled(em, 100.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0x17, 0x13, 0.6f);
            break;
        }
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_LEAVE_START):
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_LEAVE_TURN):
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x26, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0xA4, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0xBC, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x124, 2);
        if (EmMotion_PassedFrameScaled(em, 162.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0xE, 0x13, 0.8f);
        }
        if (EmMotion_PassedFrameScaled(em, 180.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 8, 0x13, 0.8f);
        }
        if (EmMotion_PassedFrameScaled(em, 288.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0x1D, 0x13, 0.6f);
            break;
        }
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_WALK_ATTACK):
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_DIE):
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0xD4, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x154, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x180, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x1A6, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x1D8, 2);
        if (EmMotion_PassedFrameScaled(em, 338.0f, 0) != 0) {
            LaoshanDust_Create(em, 0, 0, 1.2f);
        }
        if (EmMotion_PassedFrameScaled(em, 400.0f, 0) != 0) {
            LaoshanDust_Create(em, 0, 1, 1.4f);
        }
        if (EmMotion_PassedFrameScaled(em, 406.0f, 0) != 0) {
            LaoshanDust_Create(em, 0, 2, 1.2f);
            break;
        }
        break;
    case EM_MOTION_ID(0, LAOSHAN_MOT_FLINCH):
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x5A, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0x6A, 2);
        EmLaoshan_CamShakeOnFrame((s32 *) em, 0xB4, 2);
        if (EmMotion_PassedFrameScaled(em, 94.0f, 0) != 0) {
            LaoShanAttack_Create(em, 0x57);
        }
        if (EmMotion_PassedFrameScaled(em, 82.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0xE, 0x13, 0.8f);
        }
        if (EmMotion_PassedFrameScaled(em, 236.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0xE, 0x13, 0.5f);
            break;
        }
        break;
    default:
        EmLaoshan_MotionEffectsDefault(em);
        break;
    }
}

/*
 * Callback: skips the first frame (fxStep), then runs
 * EmLaoshan_MotionEffects every frame.
 */
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

/*
 * The impact effects of LAOSHAN_ATK_REARED_SMASH_FORT: two Effect02 and an
 * Effect0D (scale 10) at the fixed high fort point of the area (fortress
 * areas 4, 5, 2; else the area 5 point).
 */
void EmLaoshan_SpawnSmashImpactEffects(Em *em)
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
    HitSpark_SpawnForOwner(em, 0, 9, 0, pos, 1.0f);
    HitSpark_SpawnForOwner(em, 0, 9, 1, pos, 1.0f);
    PartFx_SpawnIfHere(em, pos, 0x22, 10.0f);
}

/*
 * The impact of the fort rams (LAOSHAN_ATK_RAM_FORT and its stage
 * variants): two sounds, two Effect02 (scale 2) and an Effect0D (scale 20)
 * at the fixed low fort point of the area (fortress areas 5 and 3, else a
 * default point).
 */
void EmLaoshan_SpawnRamImpactEffects(Em *em)
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
    HitSpark_SpawnForOwner(em, 0, 9, 0, pos, 2.0f);
    HitSpark_SpawnForOwner(em, 0, 9, 1, pos, 2.0f);
    PartFx_SpawnIfHere(em, pos, 0x22, 20.0f);
}

/* Callback table entry that does nothing. */
void EmLaoshan_TableNop(void)
{
}

/* Enters an idle state: motion speed 1, Em_SetStateSync. */
void EmLaoshan_SetIdleState(Em *em, u16 state, u8 mode)
{
    em->motionSpeed = 1.0f;
    Em_SetStateSync(em, EM_GROUP_IDLE, state, mode);
}

/*
 * Enters a move state: motion speed 1; the walks get speed 500
 * (Em_SetSpeedScaled); LAOSHAN_MOVE_WALK_BACK becomes LAOSHAN_MOVE_WALK and
 * LAOSHAN_MOVE_WALK_BACK_ON becomes LAOSHAN_MOVE_WALK_ON when the goal is
 * within a quarter turn ahead; then Em_SetStateSync.
 */
void EmLaoshan_SetMoveState(Em *em, u16 state, u8 mode)
{
    s32 goalAngle;

    em->motionSpeed = 1.0f;
    switch (state) {
    case LAOSHAN_MOVE_WALK:
    case LAOSHAN_MOVE_WALK_REARED:
        Em_SetSpeedScaled(em, 6, 500.0f, 0.0f);
        break;
    case LAOSHAN_MOVE_WALK_BACK:
        Em_SetSpeedScaled(em, 6, 500.0f, 0.0f);
        goalAngle = (u16) (Math_AngleXZ(&em->posX, em->goal) - em->rotY);
        if ((0 <= goalAngle && goalAngle <= 0x4000) || (goalAngle >= 0xC000 && goalAngle <= 0xFFFF)) {
            state = LAOSHAN_MOVE_WALK;
        }
        break;
    case LAOSHAN_MOVE_TURN_TO_GOAL:
        Math_AngleXZ(&em->posX, em->goal);
        break;
    case LAOSHAN_MOVE_WALK_ATTACK:
        Em_SetSpeedScaled(em, 6, 500.0f, 0.0f);
        break;
    case LAOSHAN_MOVE_WALK_BACK_ON:
        Em_SetSpeedScaled(em, 6, 500.0f, 0.0f);
        goalAngle = (u16) (Math_AngleXZ(&em->posX, em->goal) - em->rotY);
        if ((0 <= goalAngle && goalAngle <= 0x4000) || (goalAngle >= 0xC000 && goalAngle <= 0xFFFF)) {
            state = LAOSHAN_MOVE_WALK_ON;
        }
        break;
    }
    Em_SetStateSync(em, EM_GROUP_MOVE, state, mode);
}

/*
 * Enters a fly state (the family has none): motion speed 1,
 * Em_SetStateSync.
 */
void EmLaoshan_SetFlyState(Em *em, u16 state, u8 mode)
{
    em->motionSpeed = 1.0f;
    Em_SetStateSync(em, EM_GROUP_FLY, state, mode);
}

/* Enters an attack state: motion speed 1, Em_SetStateSync. */
void EmLaoshan_SetAttackState(Em *em, u16 state, u8 mode)
{
    em->motionSpeed = 1.0f;
    Em_SetStateSync(em, EM_GROUP_ATTACK, state, mode);
}

/*
 * Moves to (group, state): on the host first targets a player in range
 * (Em_TargetPlayerInRange); a remote copy ignores EM_SYNC_OWNER and
 * EM_SYNC_OWNER_FLAGGED; the idle, move, fly and attack groups go through
 * their Set functions, the reaction, defeat and event groups straight to
 * Em_SetStateSync with motion speed 1.
 */
void EmLaoshan_ChangeState(Em *em, u16 group, u16 state, u8 mode)
{
    if (em->remote == 0) {
        Em_TargetPlayerInRange(em);
    } else if (mode == EM_SYNC_OWNER || mode == EM_SYNC_OWNER_FLAGGED) {
        return;
    }
    switch (group) {
    case EM_GROUP_IDLE:
        EmLaoshan_SetIdleState(em, state, mode);
        return;
    case EM_GROUP_MOVE:
        EmLaoshan_SetMoveState(em, state, mode);
        return;
    case EM_GROUP_FLY:
        EmLaoshan_SetFlyState(em, state, mode);
        return;
    case EM_GROUP_ATTACK:
        EmLaoshan_SetAttackState(em, state, mode);
        return;
    case EM_GROUP_DEFEAT:
    case EM_GROUP_EVENT:
    case EM_GROUP_REACT:
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);

    default:
        return;
    }
}


/* The file's initialised data, in the game's order */
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
