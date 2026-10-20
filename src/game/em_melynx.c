/*
 * The Melynx family (EM_KIND_MELYNX; the per-kind hooks also send kind 9 to
 * its ChangeState): a small cat that wanders, pounces on a hunter to steal
 * an item and burrows away with it, or takes out a bomb and throws it. The
 * file runs from the family's motion sounds (EmMelynx_MotionSounds) to its
 * hook table's empty hook:
 * - Init (EmMelynx_Reset), Start and Reinit;
 * - the theft: EmMelynx_StealItem takes an item from a player's pouch,
 *   ReturnItem gives it back, DeductReward takes it from the robbed
 *   player's quest reward;
 * - one function per state, by group (include/em_melynx.h names the states
 *   and the motions), a dispatcher per group (EmMelynx_Run*State) and
 *   ChangeState, which enters a state;
 * - EmMelynx_Update, which senses what is near and turns Em_UpdateDamage's
 *   result into reactions, the motion effects and the face (the blinking
 *   eyes, EmMelynx_UpdateFace and SetMeshSetAlpha).
 * The state functions take the monster and its work area, read as
 * EmMelynxWork; Em.step is the step within the current state.
 */
#include "common.h"
#include "item.h"
#include "scene.h"
#include "quest.h"
#include "em.h"
#include "em_melynx.h"
#include "player.h"
#include "objattach.h"
#include "area.h"
#include "chat_log.h"
/* prototypes of callees */
void Em_SetMotionRow(Em *em, s32 row, s32 arg2, s32 arg3);
s32 Em_ComputeMaxHp(Em *em, s16 idx);
void EmMelynx_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void Em_BeginStatus2(Em *em);
void Em_BeginStatus4(Em *em);
void Em_BeginStatus5(Em *em);
void Em_EndStatus2(Em *em);
void Em_EndStatus4(Em *em);
void Em_EndStatus5(Em *em);
void Em_SetMotion(Em *em, s32 motion, s32 a, s32 b);
/* unit's own declaration: declared here as the callers use it; with the definition's void (Em *em, s32 group, s32 state, u8 mode), the call in EmMelynx_ChangeState compiles differently */
void Em_SetStateSync(Em *em, u16 group, u16 state, u8 mode);
u16 Math_AngleXZ(f32 *from, f32 *to);
void Player_ClearFlag(void *, u32 flag);
void Player_SetFlag(void *obj, u32 flag);
s8 QuestEm_Appear(Em *em);
ObjAttach *EmContactAttack_Create(Em *em, s8 kind);
void EmBomb_Create(Em *, s8);
void Em_ComputeGoal2(Em *em, f32 *target);
void Em_SetRequest(Em *em, u8 a, u8 prio);
void Em_SpawnStatus4Effect(Em *em, s32 a);
void Em_SetHitReactMode(Em *em, u8 a);
void Em_PlaySeOnFrame(Em *em, s32 frame, s32 se);
void Em_RestartScript(Em *em);
void EmMelynx_Reset(Em *);
void EmMelynx_CapTimerOffTarget(Em *em);
void EmMelynx_Status2Lie(Em *);
void EmMelynx_BurrowAway(Em *);
void EmMelynx_ChaseGoal(Em *);
void EmMelynx_WalkLong(Em *);
void EmMelynx_RunToGoalA(Em *);
void EmMelynx_RunToGoalB(Em *);
void EmMelynx_WalkToGoal(Em *);
void EmMelynx_StealAttack(Em *em);
void EmMelynx_KnockBack(Em *);
void EmMelynx_Status4(Em *);
void EmMelynx_HuntedKnockBack(Em *);
void EmMelynx_HuntedFlinch(Em *);
void DustCloud_Create(Em *em, s16 k, s16 c, f32 scale);
void StunStars_CreateEm(Em *em, u8 k, s16 v, s16 t, f32 y, f32 scale);
s32 Sound_RandomSe(s32 id, s16 lo, s32 alt, s16 n);
s32 EmMotion_PassedFrameScaled(void *p, f32 frame, s32 layer);
void Vec_RotateY(f32 *v, f32 angle);
f32 Vec_Distance(f32 *a, f32 *b);
f32 *Mtx_FromAnglesXYZ(s32 *angles, f32 *m);
void Vec_Set(f32 *out, f32 x, f32 y, f32 z);
f32 Vec_DistanceXZ(f32 *a, f32 *b);
s32 Angle_TurnToward(s32 *cur, s32 *target, s32 step);
u16 Area_DayId(u16 id);
s32 QuestEm_CanAppear(Em *em);
void Quest_DeductReward(Player *p, u16 item, s16 n);
void Player_Integrate(void *);
void Shadow_Spawn(void *obj, s16 type);
void QuestEm_Hunted(Em *em);
void EmMine_Remove(Em *em);
void EmMelynx_ReturnItem(Em *em, EmMelynxWork *w);
void ObjListA70_Remove(EmObj *obj);
void EmMine_Drop(Em *em, f32 *pos);
s32 Player_MoveFall(Player *p);
void Vec_Rotate(f32 *out, f32 *in, f32 *mtx);
void Em_SpawnLyingPuffs(Em *em, s32 bone, f32 *pos, f32 scale);
void Player_ClearAccel2(void *);
/* unit's own declaration: declared here as the callers use it; with the definition's u8 (Player *p, u16 item, s16 n), the calls in EmMelynx_ReturnItem compile differently */
s32 Pouch_Add(Player *p, u16 item, s16 n);
s32 Player_IsLocal(void *obj);
u8 Notice_Push(u8 kind, s16 a, s16 b);
void EmMelynx_MotionEffects(Em *em, EmWork *work);
s32 Quest_GetOutcome(s32 withLoss);
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
u8 Em_UpdateDamage(Em *em, u8 *out);
void Em_UpdatePendingScript(Em *em);
void Em_ClearHitReactMode(Em *em);
void EmMelynx_RunState(Em *em);
void PartFx_SpawnForEm(Em *em, s32 a, s32 b);
/* A set of meshes (0x8C bytes). */
typedef struct EmMelynxMeshSet {
    s32 unk0;
    s32 count;          /* 0x04 */
    s32 mesh[33];       /* 0x08 */
} EmMelynxMeshSet;
s32 Gs_SetState(u8 reg, u32 v);
/* end of prototypes */

/*
 * Plays the motions' sounds: switches on work->fxMotion (the layer-0 motion
 * id MotionEffects keeps) and plays sounds on given frames
 * (Em_PlaySeOnFrame, some picked by Sound_RandomSe); motion
 * MELYNX_MOT_BOMB_TAKE_OUT has its own set while in
 * MELYNX_IDLE_HOLD_BIG_BOMB.
 */
void EmMelynx_MotionSounds(Em *em, EmWork *work)
{
    s16 motion;

    motion = work->fxMotion;
    switch (motion) {
    case EM_MOTION_ID(0, MELYNX_MOT_IDLE):
        Em_PlaySeOnFrame(em, 4, Sound_RandomSe(0x12, 1, 0x12, 1));
        break;
    case EM_MOTION_ID(0, MELYNX_MOT_IDLE_ACTION_A):
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
    case EM_MOTION_ID(0, MELYNX_MOT_IDLE_ACTION_B):
        Em_PlaySeOnFrame(em, 0x26, 0x12);
        Em_PlaySeOnFrame(em, 6, 0x1E);
        Em_PlaySeOnFrame(em, 0x16, 0x1E);
        Em_PlaySeOnFrame(em, 0x9A, 0x1E);
        Em_PlaySeOnFrame(em, 0xA4, 0x1E);
        break;
    case EM_MOTION_ID(0, MELYNX_MOT_IDLE_ACTION_C):
        Em_PlaySeOnFrame(em, 0xE, Sound_RandomSe(9, 2, 9, 2));
        Em_PlaySeOnFrame(em, 0x4C, Sound_RandomSe(0xC, 2, 0xC, 2));
        Em_PlaySeOnFrame(em, 0xA0, Sound_RandomSe(9, 2, 9, 2));
        Em_PlaySeOnFrame(em, 0xC8, Sound_RandomSe(0xA, 2, 0xA, 2));
        Em_PlaySeOnFrame(em, 8, 0x1E);
        Em_PlaySeOnFrame(em, 0x50, 0x1E);
        Em_PlaySeOnFrame(em, 0x8C, 0x1E);
        Em_PlaySeOnFrame(em, 0xC8, 0x1E);
        break;
    case EM_MOTION_ID(0, MELYNX_MOT_IDLE_ACTION_D):
        Em_PlaySeOnFrame(em, 4, 0x17);
        Em_PlaySeOnFrame(em, 0xE, 3);
        break;
    case EM_MOTION_ID(0, MELYNX_MOT_WALK):
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
    case EM_MOTION_ID(0, MELYNX_MOT_RUN_A):
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
    case EM_MOTION_ID(0, MELYNX_MOT_FALL):
        Em_PlaySeOnFrame(em, 8, Sound_RandomSe(0xD, 2, 0xD, 2));
        Em_PlaySeOnFrame(em, 0xC, 0x21);
        Em_PlaySeOnFrame(em, 0xC, 0x19);
        Em_PlaySeOnFrame(em, 0x2A, 0x17);
        Em_PlaySeOnFrame(em, 0x2E, 0x18);
        break;
    case EM_MOTION_ID(0, MELYNX_MOT_POSTURE_MOVE):
        Em_PlaySeOnFrame(em, 8, 0x1E);
        Em_PlaySeOnFrame(em, 0x18, 0x1E);
        Em_PlaySeOnFrame(em, 0x28, 0x1E);
        break;
    case EM_MOTION_ID(0, MELYNX_MOT_RUN_B):
        Em_PlaySeOnFrame(em, 8, Sound_RandomSe(0, 2, 0, 2));
        Em_PlaySeOnFrame(em, 0xC, 0x18);
        Em_PlaySeOnFrame(em, 0x12, 0x22);
        Em_PlaySeOnFrame(em, 0x22, 0x18);
        Em_PlaySeOnFrame(em, 0x28, 0x22);
        break;
    case EM_MOTION_ID(0, MELYNX_MOT_DASH):
        Em_PlaySeOnFrame(em, 0x2E, Sound_RandomSe(0x11, 1, 0x11, 1));
        Em_PlaySeOnFrame(em, 0xA, 0x17);
        Em_PlaySeOnFrame(em, 0x18, 0x17);
        Em_PlaySeOnFrame(em, 0x26, 0x17);
        Em_PlaySeOnFrame(em, 0x32, 0x17);
        break;
    case EM_MOTION_ID(0, MELYNX_MOT_BURROW):
        Em_PlaySeOnFrame(em, 0x10, 0x1A);
        Em_PlaySeOnFrame(em, 0x24, 0x1A);
        Em_PlaySeOnFrame(em, 0x3A, 0x1A);
        Em_PlaySeOnFrame(em, 0x4E, 0x1A);
        Em_PlaySeOnFrame(em, 0x7E, 0x14);
        Em_PlaySeOnFrame(em, 0x80, 0x19);
        Em_PlaySeOnFrame(em, 0x78, 6);
        Em_PlaySeOnFrame(em, 0x9C, 0x1B);
        break;
    case EM_MOTION_ID(0, MELYNX_MOT_POUNCE_START):
        Em_PlaySeOnFrame(em, 2, 0x16);
        Em_PlaySeOnFrame(em, 0xE, 0x17);
        Em_PlaySeOnFrame(em, 0x18, 3);
        Em_PlaySeOnFrame(em, 0x48, 0x17);
        break;
    case EM_MOTION_ID(0, MELYNX_MOT_LEAP):
        Em_PlaySeOnFrame(em, 2, 0x18);
        Em_PlaySeOnFrame(em, 6, 0x21);
        Em_PlaySeOnFrame(em, 0x10, 4);
        break;
    case EM_MOTION_ID(0, MELYNX_MOT_SNATCH):
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
    case EM_MOTION_ID(0, MELYNX_MOT_LEAP_LAND):
        Em_PlaySeOnFrame(em, 0x10, 0x1E);
        Em_PlaySeOnFrame(em, 0x1A, 0x1E);
        Em_PlaySeOnFrame(em, 0x24, 0x1E);
        Em_PlaySeOnFrame(em, 0x30, 0x1E);
        Em_PlaySeOnFrame(em, 0x3A, 0x1E);
        Em_PlaySeOnFrame(em, 0x46, 0x1E);
        Em_PlaySeOnFrame(em, 0x52, 0x1E);
        Em_PlaySeOnFrame(em, 0x92, 0x1E);
        break;
    case EM_MOTION_ID(0, MELYNX_MOT_DIZZY):
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
    case EM_MOTION_ID(0, MELYNX_MOT_POSTURE_LOOP):
        Em_PlaySeOnFrame(em, 4, 0x15);
        Em_PlaySeOnFrame(em, 0xC, 0x18);
        Em_PlaySeOnFrame(em, 8, Sound_RandomSe(5, 4, 7, 4));
        Em_PlaySeOnFrame(em, 0x1E, 0x15);
        Em_PlaySeOnFrame(em, 0x36, 0x1A);
        break;
    case EM_MOTION_ID(0, MELYNX_MOT_POSTURE_START):
        Em_PlaySeOnFrame(em, 8, 0x15);
        Em_PlaySeOnFrame(em, 6, 0x19);
        Em_PlaySeOnFrame(em, 0x26, 0x21);
        Em_PlaySeOnFrame(em, 0x26, 0x1A);
        break;
    case EM_MOTION_ID(0, MELYNX_MOT_POSTURE_END):
        Em_PlaySeOnFrame(em, 8, 0x17);
        Em_PlaySeOnFrame(em, 0xC, 0x21);
        Em_PlaySeOnFrame(em, 0x26, 0x21);
        break;
    case EM_MOTION_ID(0, MELYNX_MOT_IDLE_ACTION_G):
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
    case EM_MOTION_ID(0, MELYNX_MOT_BOMB_TAKE_OUT):
        if ((em->stateGroup == EM_GROUP_IDLE) && (em->state == 0xF)) {
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
    case EM_MOTION_ID(0, MELYNX_MOT_IDLE_ACTION_E):
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
    case EM_MOTION_ID(0, MELYNX_MOT_IDLE_ACTION_F):
        Em_PlaySeOnFrame(em, 0x12, Sound_RandomSe(8, 4, 0xA, 4));
        Em_PlaySeOnFrame(em, 0x3C, Sound_RandomSe(9, 4, 0xC, 4));
        Em_PlaySeOnFrame(em, 0x10, 0x15);
        Em_PlaySeOnFrame(em, 0x40, 0x15);
        Em_PlaySeOnFrame(em, 0x10, 0x18);
        Em_PlaySeOnFrame(em, 0x40, 0x18);
        break;
    case EM_MOTION_ID(0, MELYNX_MOT_FLINCH):
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
    case EM_MOTION_ID(0, MELYNX_MOT_KNOCK_BACK):
        Em_PlaySeOnFrame(em, 4, Sound_RandomSe(5, 4, 7, 4));
        break;
    case EM_MOTION_ID(0, MELYNX_MOT_LIE):
        Em_PlaySeOnFrame(em, 4, Sound_RandomSe(4, 2, 4, 2));
        Em_PlaySeOnFrame(em, 4, 0x1C);
        Em_PlaySeOnFrame(em, 0x2E, 0x1D);
        Em_PlaySeOnFrame(em, 4, Sound_RandomSe(6, 3, 6, 3));
        Em_PlaySeOnFrame(em, 0x64, 0x1C);
        Em_PlaySeOnFrame(em, 0xA, 0x1A);
        Em_PlaySeOnFrame(em, 0x34, 0x1A);
        break;
    case EM_MOTION_ID(0, MELYNX_MOT_GET_UP):
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
    case EM_MOTION_ID(0, MELYNX_MOT_FLINCH_END):
        Em_PlaySeOnFrame(em, 0x14, 0x1E);
        Em_PlaySeOnFrame(em, 0x22, 0x1E);
        Em_PlaySeOnFrame(em, 0x2A, 0x1E);
        Em_PlaySeOnFrame(em, 0x46, 0x1E);
        Em_PlaySeOnFrame(em, 0x54, 0x1E);
        Em_PlaySeOnFrame(em, 0x6C, 0x1E);
        Em_PlaySeOnFrame(em, 0x8C, 0x13);
        break;
    case EM_MOTION_ID(0, MELYNX_MOT_STATUS4_START):
        Em_PlaySeOnFrame(em, 0x30, 0x13);
        Em_PlaySeOnFrame(em, 4, 6);
        break;
    case EM_MOTION_ID(0, MELYNX_MOT_STATUS4_LOOP):
        Em_PlaySeOnFrame(em, 4, 0x13);
        Em_PlaySeOnFrame(em, 0x40, 0x1A);
        Em_PlaySeOnFrame(em, 0x5A, 0x1D);
        break;
    case EM_MOTION_ID(0, MELYNX_MOT_STATUS2_LIE):
        Em_PlaySeOnFrame(em, 4, 0x12);
        Em_PlaySeOnFrame(em, 0x9C, 0x1D);
        break;
    case EM_MOTION_ID(0, 0x46):
        Em_PlaySeOnFrame(em, 4, 0xC);
        Em_PlaySeOnFrame(em, 0x100, 1);
        break;
    case EM_MOTION_ID(0, 0x47):
        Em_PlaySeOnFrame(em, 0x10, 0x18);
        Em_PlaySeOnFrame(em, 0x12, 0x18);
        Em_PlaySeOnFrame(em, 0x24, 0xC);
        break;
    case EM_MOTION_ID(0, 0x48):
        Em_PlaySeOnFrame(em, 0x30, 0xD);
        Em_PlaySeOnFrame(em, 8, 0x1E);
        break;
    case EM_MOTION_ID(0, 0x49):
        Em_PlaySeOnFrame(em, 0xA, 0x12);
        break;
    case EM_MOTION_ID(0, MELYNX_MOT_BOMB_THROW):
        Em_PlaySeOnFrame(em, 8, 0xA);
        Em_PlaySeOnFrame(em, 0x34, 4);
        Em_PlaySeOnFrame(em, 0x36, 0x21);
        Em_PlaySeOnFrame(em, 0x36, 0x20);
        Em_PlaySeOnFrame(em, 0x16, 0x18);
        Em_PlaySeOnFrame(em, 0x1E, 0x18);
        Em_PlaySeOnFrame(em, 0x80, 0x18);
        Em_PlaySeOnFrame(em, 0x94, 0x18);
        break;
    case EM_MOTION_ID(0, 0x4B):
        Em_PlaySeOnFrame(em, 0xE, 0xB);
        break;
    }
}

/*
 * Sets the monster up: in the quest's stage 0 places it by its slot number
 * (its own spots on the snowy mountains at night), clears blind, remote,
 * stance and painted, plays motion row 1, sets hp from Em_ComputeMaxHp,
 * clears holdBomb, spareTimer and alertState, stores home, marks no stolen
 * item, grows variant bit 0x80 to scale 10, enters combat when spawnParam
 * is 1, and inits the parts.
 */
void EmMelynx_Reset(Em *em)
{
    EmMelynxWork *w = (EmMelynxWork *)&em->work;
    f32 slotF;
    u8 slotNo;
    s16 hp;

    if (gQuest.stage == 0) {
        if (Area_DayId(gScene.area) == AREA_SNOWY_MOUNTAINS_NIGHT_9) {
            em->posX = 9500.0f + (150.0f * (f32) (em->slotNo / 8));
            em->posY = 0.0f;
            em->posZ = 9200.0f + (150.0f * (f32) (em->slotNo & 7));
        } else {
            slotNo = em->slotNo;
            slotF = (f32) (u32) slotNo;
            em->posX = 2400.0f + (150.0f * slotF);
            em->posY = 0.0f;
            em->posZ = 3600.0f;
        }
    }
    em->blind = 0;
    em->remote = 0;
    Em_SetMotionRow(em, MELYNX_MOT_IDLE, 0, 0);
    em->stance = EM_STANCE_GROUND;
    hp = Em_ComputeMaxHp(em, 0);
    em->hp = hp;
    em->hpMax = hp;
    w->holdBomb = 0;
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

/*
 * Resets the monster (EmMelynx_Reset) and starts it: variant 0xC and the
 * desert stand (MELYNX_IDLE_STAND; desert variants other than 0 unhittable
 * for 10 frames), the others wander (MELYNX_MOVE_WANDER).
 */
void EmMelynx_Start(Em *em)
{
    EmMelynx_Reset(em);
    if (em->variant == 0xC) {
        EmMelynx_ChangeState(em, EM_GROUP_IDLE, MELYNX_IDLE_STAND, EM_SYNC_LOCAL);
        return;
    }
    if ((Area_DayId(em->area) & 0xFFFF) == AREA_DESERT_8) {
        if (em->variant != 0) {
            em->noHitTimer = 0xA;
            em->noPushTimer = 0xA;
        }
        EmMelynx_ChangeState(em, EM_GROUP_IDLE, MELYNX_IDLE_STAND, EM_SYNC_LOCAL);
        return;
    }
    EmMelynx_ChangeState(em, EM_GROUP_MOVE, MELYNX_MOVE_WANDER, EM_SYNC_LOCAL);
}

/* The table's re-init hook: EmMelynx_Reset. */
void EmMelynx_Reinit(Em *em)
{
    EmMelynx_Reset(em);
}

/*
 * When the Melynx still holds a stolen item, takes it from the quest's
 * reward of the player robbed (Quest_DeductReward with stolenFrom,
 * stolenItem, stolenCount); called by the quest code.
 */
void EmMelynx_DeductReward(Em *em)
{
    EmMelynxWork *w = (EmMelynxWork *)&em->work;
    s16 item = w->stolenItem;

    if (item != -1) {
        Quest_DeductReward(w->stolenFrom, item, w->stolenCount);
    }
}

/*
 * Cuts em->timer to 15 when it is above 15 and there is no target or the
 * target player is in another area, so a chase ends soon.
 */
void EmMelynx_CapTimerOffTarget(Em *em)
{
    s8 targetIdx;

    if (em->timer > 0xF) {
        targetIdx = em->targetNo;
        if (targetIdx == -1) {
            em->timer = 0xF;
        } else if (em->area != ((Player *) &gPlayers[targetIdx])->area) {
            em->timer = 0xF;
        }
    }
}

/*
 * Moves to (group, state) with Em_SetStateSync after the entry work: clears
 * unk446; groups 0, 1 and 3 stand (stance 0); MELYNX_IDLE_STUN_END ends
 * status 5; the move states set holdBomb (all but TURN, RUN_A and
 * WALK_LONG) and compute the goal (Em_ComputeGoal2; all but WALK_LONG,
 * which gets 600 frames); MELYNX_REACT_STUN begins status 5;
 * MELYNX_DEFEAT_REAPPEAR calls QuestEm_Appear when the quest lets it.
 */
void EmMelynx_ChangeState(Em *em, u16 group, u16 state, u8 mode)
{
    EmMelynxWork *w = (EmMelynxWork *)&em->work;

    em->unk446 = 0;
    switch (group) {
    case EM_GROUP_IDLE:
        em->stance = EM_STANCE_GROUND;
        switch (state) {
        case MELYNX_IDLE_HOLD_BOMB:
            break;
        case MELYNX_IDLE_HOLD_BIG_BOMB:
            break;
        case MELYNX_IDLE_STUN_END:
            Em_EndStatus5(em);
            break;
        }
        break;
    case EM_GROUP_MOVE:
        em->stance = EM_STANCE_GROUND;
        switch (state) {
        case MELYNX_MOVE_CHARGE:
        default:
            w->holdBomb = 1;
        case MELYNX_MOVE_TURN:
        case MELYNX_MOVE_RUN_A:
            Em_ComputeGoal2(em, em->goal);
            break;
        case MELYNX_MOVE_WALK_LONG:
            em->timer = 600;
            break;
        }
        break;
    case EM_GROUP_ATTACK:
        em->stance = EM_STANCE_GROUND;
        break;
    case EM_GROUP_REACT:
        switch (state) {
        case MELYNX_REACT_KNOCK_BACK:
            break;
        case MELYNX_REACT_STUN:
            em->stance = EM_STANCE_GROUND;
            Em_BeginStatus5(em);
            break;
        default:
            em->stance = EM_STANCE_GROUND;
            break;
        }
        break;
    case EM_GROUP_DEFEAT:
        if (state == 2 && QuestEm_CanAppear(em) == 1) {
            QuestEm_Appear(em);
        }
        break;
    case EM_GROUP_FLY:
    case EM_GROUP_EVENT:
        break;
    }
    Em_SetStateSync(em, group, state, mode);
}

/*
 * Ends the current state: motion speed 1, hands the choice back to the
 * script (scriptPending), goes to MELYNX_IDLE_STAND keeping holdBomb, and
 * records the state as the previous one.
 */
void EmMelynx_EndState(Em *em)
{
    EmMelynxWork *w = (EmMelynxWork *)&em->work;

    u16 savedHoldBomb;

    em->motionSpeed = 1.0f;
    em->scriptPending = 1;
    savedHoldBomb = (u16) w->holdBomb;
    EmMelynx_ChangeState(em, EM_GROUP_IDLE, MELYNX_IDLE_STAND, EM_SYNC_LOCAL);
    w->holdBomb = (s16) savedHoldBomb;
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
}

/*
 * Tells the Melynx's bombs (EmBomb_RunHeld, EmBomb_RunMelynxThrown) what
 * their owner does: returns -1 when it is off the ground, in groups 4 to 6
 * or inactive, -2 when one of its attacks touched something (contactHit), 1
 * while it holds the bomb (holdBomb), else 0 (let go).
 */
s32 EmMelynx_BombState(Em *em)
{
    u8 *w = &em->work;

    if (em->stance != EM_STANCE_GROUND || (u32) (em->stateGroup - 4) <= 2 || em->active == 0) {
        return -1;
    }
    if (em->contactHit != 0) {
        return -2;
    }
    if (*(u16 *) (w + 0xE) != 0) {
        return 1;
    }
    return 0;
}

/*
 * Steals one item from player p on the local machine, unless the quest is
 * over or p has SKILL_THEFT_GUARD: the item 0xBD or 0xF9 first, else a random one
 * (stateRand) among the pouch's items of type 0 and rarity below 4; takes
 * it from the pouch, records it in w (stolenItem, stolenCount -1,
 * stolenFrom) and shows the stolen notice. Returns 1 when it stole, else 0.
 */
s32 EmMelynx_StealItem(Em *em, Player *victim, EmMelynxWork *work)
{
    s32 candidates[20];
    s32 i;
    s32 count;

    if (victim->index != gScene.localPlayer) {
        return 0;
    }
    if (Quest_GetOutcome(1)) {
        return 0;
    }
    if (Player_HasSkill(victim, SKILL_THEFT_GUARD) == 1) {
        return 0;
    }
    if (Pouch_Count(victim, 0xBD) > 0) {
        work->stolenItem = 0xBD;
        work->stolenCount = -1;
    } else if (Pouch_Count(victim, 0xF9) > 0) {
        work->stolenItem = 0xF9;
        work->stolenCount = -1;
    } else {
        for (i = 0, count = 0; i < 20; i++) {
            if (victim->pouch[i].item != 0 && gItemData[victim->pouch[i].item].type == 0 && gItemData[victim->pouch[i].item].rarity < 4) {
                candidates[count] = i;
                count++;
            }
        }
        if (count == 0) {
            return 0;
        }
        work->stolenItem = victim->pouch[candidates[em->stateRand % count]].item;
        work->stolenCount = -1;
    }
    work->stolenFrom = victim;
    Pouch_Add(victim, (u16)work->stolenItem, work->stolenCount);
    Notice_Push(NOTICE_KIND_ITEM, NOTICE_ITEM_STOLEN, work->stolenItem);
    return 1;
}

/*
 * Gives the stolen item back to the player robbed (Pouch_Add) and shows
 * that player's notice (returned, pouch full, or message 0xC), then marks
 * no stolen item.
 */
void EmMelynx_ReturnItem(Em *em, EmMelynxWork *work)
{
    if (work->stolenItem != -1) {
        switch ((u16) Pouch_Add(work->stolenFrom, work->stolenItem, -work->stolenCount)) {
        case 0:
        case 1:
        case 2:
            if (Player_IsLocal(work->stolenFrom) == 1) {
                Notice_Push(NOTICE_KIND_ITEM, NOTICE_ITEM_RETURNED, work->stolenItem);
            }
            break;
        case 3:
            if (Player_IsLocal(work->stolenFrom) == 1) {
                Notice_Push(NOTICE_KIND_ITEM, NOTICE_ITEM_POUCH_FULL, work->stolenItem);
            }
            break;
        default:
            if (Player_IsLocal(work->stolenFrom) == 1) {
                Notice_Push(NOTICE_KIND_MESSAGE, 0xC, 0);
            }
            break;
        }
        work->stolenItem = -1;
    }
}

/*
 * MELYNX_IDLE_WAIT: stands (MELYNX_MOT_IDLE unless already playing) for 5
 * to 20 frames, then hands the choice back to the script without leaving
 * the state.
 */
void EmMelynx_IdleWait(Em *em, s32 unused)
{
    switch (em->step) {
    case 0:
        em->step++;
        if (em->motionId[0] != EM_MOTION_ID(0, MELYNX_MOT_IDLE)) {
            Em_SetMotion(em, MELYNX_MOT_IDLE, 0, 0);
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

/*
 * MELYNX_IDLE_STAND: plays MELYNX_MOT_IDLE (unless already playing) once
 * and ends (EmMelynx_EndState); the state EndState and Start go to.
 */
void EmMelynx_IdleStand(Em *em, s32 unused)
{
    switch (em->step) {
    case 0:
        em->step++;
        if (em->motionId[0] != EM_MOTION_ID(0, MELYNX_MOT_IDLE)) {
            Em_SetMotion(em, MELYNX_MOT_IDLE, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmMelynx_EndState(em);
        }
        break;
    }
}

/*
 * MELYNX_IDLE_ACTION_A, _B, _C: plays MELYNX_MOT_IDLE_ACTION_A, _B or _C by variant (0, 1, 2)
 * and ends with it; ACTION_B also follows a missed pounce or leap.
 */
void EmMelynx_IdleAction(Em *em, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        switch (variant) {
        case 0:
            Em_SetMotion(em, MELYNX_MOT_IDLE_ACTION_A, 0, 0);
            break;
        case 1:
            Em_SetMotion(em, MELYNX_MOT_IDLE_ACTION_B, 0, 0);
            break;
        case 2:
            Em_SetMotion(em, MELYNX_MOT_IDLE_ACTION_C, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmMelynx_EndState(em);
        }
        break;
    }
}

/*
 * MELYNX_IDLE_ACTION_D: plays MELYNX_MOT_IDLE_ACTION_D for 120 frames, then
 * ends.
 */
void EmMelynx_IdleActionD(Em *em, s32 unused)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, MELYNX_MOT_IDLE_ACTION_D, 0, 0);
        em->timer = 0x78;
        break;
    case 1:
        if (--em->timer <= 0) {
            EmMelynx_EndState(em);
        }
        break;
    }
}

/*
 * MELYNX_IDLE_DIZZY: reels (MELYNX_MOT_DIZZY) under stun stars for 60
 * frames (StunStars_CreateEm), then gets up (MELYNX_MOT_GET_UP) and ends.
 */
void EmMelynx_IdleDizzy(Em *em, s32 unused)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, MELYNX_MOT_DIZZY, 0, 0);
        StunStars_CreateEm(em, 1, 2, 0x3C, 30.0f, 1.0f);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, MELYNX_MOT_GET_UP, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmMelynx_EndState(em);
        }
        break;
    }
}

/* MELYNX_IDLE_ACTION_E: plays MELYNX_MOT_IDLE_ACTION_E once and ends. */
void EmMelynx_IdleActionE(Em *em, s32 unused)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, MELYNX_MOT_IDLE_ACTION_E, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmMelynx_EndState(em);
        }
        break;
    }
}

/*
 * MELYNX_IDLE_ACTION_F: plays MELYNX_MOT_IDLE_ACTION_F for 60 to 123
 * frames, then ends.
 */
void EmMelynx_IdleActionF(Em *em, s32 unused)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = (em->stateRand & 0x3F) + 0x3C;
        Em_SetMotion(em, MELYNX_MOT_IDLE_ACTION_F, 0, 0);
        break;
    case 1:
        if (--em->timer <= 0) {
            EmMelynx_EndState(em);
        }
        break;
    }
}

/*
 * MELYNX_IDLE_POSTURE: holds MELYNX_MOT_POSTURE_LOOP for 150 frames, then
 * leaves it (MELYNX_MOT_POSTURE_END) and ends.
 */
void EmMelynx_IdlePosture(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 0x96;
        Em_SetMotion(em, MELYNX_MOT_POSTURE_LOOP, 0, 0);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, MELYNX_MOT_POSTURE_END, 4, 4);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmMelynx_EndState(em);
        }
        break;
    }
}

/*
 * MELYNX_IDLE_ACTION_G: plays MELYNX_MOT_IDLE_ACTION_G once with sight back
 * (blind cleared) and ends; the 120-frame timer it sets is not used.
 */
void EmMelynx_IdleActionG(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 0x78;
        Em_SetMotion(em, MELYNX_MOT_IDLE_ACTION_G, 0, 0);
        em->blind = 0;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmMelynx_EndState(em);
        }
        break;
    }
}

/*
 * MELYNX_IDLE_HOLD_BOMB: takes out a bomb (MELYNX_MOT_BOMB_TAKE_OUT); at
 * frame 20 makes the held bomb (EmBomb_Create type 0) and sets holdBomb,
 * then ends 110 frames later still holding it.
 */
void EmMelynx_HoldBomb(Em *em)
{
    EmMelynxWork *w = (EmMelynxWork *)&em->work;

    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 0x14;
        Em_SetMotion(em, MELYNX_MOT_BOMB_TAKE_OUT, 0, 0);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            em->timer = 0x6E;
            w->holdBomb = 1;
            EmBomb_Create(em, 0);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            EmMelynx_EndState(em);
        }
        break;
    }
}

/*
 * MELYNX_IDLE_STATUS2_LIE: lies under status 2 (Em_BeginStatus2,
 * MELYNX_MOT_STATUS2_LIE, blind, a hit wakes it: EM_HIT_REACT_WAKE) with
 * puffs at bone 10, for the timer Em_SetStatus2Timer set, then
 * MELYNX_IDLE_STATUS2_END.
 */
void EmMelynx_Status2Lie(Em *em)
{
    f32 puffOffset[3];

    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, MELYNX_MOT_STATUS2_LIE, 0, 0);
        Em_BeginStatus2(em);
        break;
    case 1:
        puffOffset[0] = 0.0f;
        puffOffset[1] = 0.0f;
        puffOffset[2] = 10.0f;
        Em_SpawnLyingPuffs(em, 0xA, puffOffset, 0.6f);
        if (--em->timer <= 0) {
            em->step++;
            EmMelynx_ChangeState(em, EM_GROUP_IDLE, MELYNX_IDLE_STATUS2_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * MELYNX_IDLE_STATUS2_END: ends status 2 and gets up (MELYNX_MOT_GET_UP),
 * blind until the motion ends, then ends.
 */
void EmMelynx_Status2End(Em *em)
{
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        Em_EndStatus2(em);
        Em_SetMotion(em, MELYNX_MOT_GET_UP, 0, 0);
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

/*
 * MELYNX_IDLE_BURROW_AWAY: burrows (MELYNX_MOT_BURROW), unhittable from
 * frame 144; at the end drops its sense object, takes the stolen item from
 * the quest's reward (Quest_DeductReward) and leaves for another spot
 * (QuestEm_Appear).
 */
void EmMelynx_BurrowAway(Em *em)
{
    EmMelynxWork *w;
    s16 item;

    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, MELYNX_MOT_BURROW, 0, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 144.0f, 0) != 0) {
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
            item = w->stolenItem;
            if (item != -1) {
                Quest_DeductReward(w->stolenFrom, item, w->stolenCount);
            }
            QuestEm_Appear(em);
        }
        break;
    }
}

/*
 * MELYNX_IDLE_HOLD_BIG_BOMB: as MELYNX_IDLE_HOLD_BOMB with the big bomb
 * (EmBomb_Create type 1).
 */
void EmMelynx_HoldBigBomb(Em *em)
{
    EmMelynxWork *w = (EmMelynxWork *)&em->work;

    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 0x14;
        Em_SetMotion(em, MELYNX_MOT_BOMB_TAKE_OUT, 0, 0);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            em->timer = 0x6E;
            w->holdBomb = 1;
            EmBomb_Create(em, 1);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            EmMelynx_EndState(em);
        }
        break;
    }
}

/*
 * MELYNX_IDLE_STUN_END: stands up after the stun (ChangeState ended status
 * 5; MELYNX_MOT_GET_UP) and ends.
 */
void EmMelynx_StunEnd(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, MELYNX_MOT_GET_UP, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmMelynx_EndState(em);
        }
        break;
    }
}

/*
 * Runs the idle state (group 0, and group 7) by Em.state after recording it
 * as the previous one; ACTION_A..C share EmMelynx_IdleAction with variants
 * 0..2.
 */
void EmMelynx_RunIdleState(Em *em)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->state) {
    case MELYNX_IDLE_WAIT:
        EmMelynx_IdleWait(em, 0);
        break;
    case MELYNX_IDLE_STAND:
        EmMelynx_IdleStand(em, 0);
        break;
    case MELYNX_IDLE_ACTION_A:
        EmMelynx_IdleAction(em, 0);
        break;
    case MELYNX_IDLE_ACTION_B:
        EmMelynx_IdleAction(em, 1);
        break;
    case MELYNX_IDLE_ACTION_C:
        EmMelynx_IdleAction(em, 2);
        break;
    case MELYNX_IDLE_ACTION_D:
        EmMelynx_IdleActionD(em, 2);
        break;
    case MELYNX_IDLE_DIZZY:
        EmMelynx_IdleDizzy(em, 0);
        break;
    case MELYNX_IDLE_ACTION_E:
        EmMelynx_IdleActionE(em, 0);
        break;
    case MELYNX_IDLE_ACTION_F:
        EmMelynx_IdleActionF(em, 0);
        break;
    case MELYNX_IDLE_POSTURE:
        EmMelynx_IdlePosture(em);
        break;
    case MELYNX_IDLE_ACTION_G:
        EmMelynx_IdleActionG(em);
        break;
    case MELYNX_IDLE_HOLD_BOMB:
        EmMelynx_HoldBomb(em);
        break;
    case MELYNX_IDLE_STATUS2_LIE:
        EmMelynx_Status2Lie(em);
        break;
    case MELYNX_IDLE_STATUS2_END:
        EmMelynx_Status2End(em);
        break;
    case MELYNX_IDLE_BURROW_AWAY:
        EmMelynx_BurrowAway(em);
        break;
    case MELYNX_IDLE_HOLD_BIG_BOMB:
        EmMelynx_HoldBigBomb(em);
        break;
    case MELYNX_IDLE_STUN_END:
        EmMelynx_StunEnd(em);
        
    default:
        break;
    }
}

/*
 * MELYNX_MOVE_CHASE: runs (MELYNX_MOT_RUN_A) to the goal, turning to it
 * each frame, until within 100 or after 300 frames (cut short when the
 * target leaves: EmMelynx_CapTimerOffTarget), then ends.
 */
void EmMelynx_ChaseGoal(Em *em)
{
    EmMelynxWork *w = (EmMelynxWork *)&em->work;

    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 300;
        w->stateTimer = 0x8A;
        Em_SetMotion(em, MELYNX_MOT_RUN_A, 0, 0);
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

/*
 * MELYNX_MOVE_POSTURE: takes the posture (MELYNX_MOT_POSTURE_START), from
 * frame 46 moves in it (MELYNX_MOT_POSTURE_MOVE) until within 200 of the
 * goal or after 300 frames, then leaves it (MELYNX_MOT_POSTURE_END) and
 * ends.
 */
void EmMelynx_PostureMove(Em *em)
{
    u8 step;

    step = em->step;
    switch (step) {
    case 0:
        em->step = step + 1;
        Em_SetMotion(em, MELYNX_MOT_POSTURE_START, 0, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 46.0f, 0U) != 0) {
            em->step++;
            Em_SetMotion(em, MELYNX_MOT_POSTURE_MOVE, 0, 0);
            em->timer = 0x12C;
        }
        break;
    case 2:
        EmMelynx_CapTimerOffTarget(em);
        if (Vec_DistanceXZ(&em->posX, em->goal) <= 200.0f || --em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, MELYNX_MOT_POSTURE_END, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            EmMelynx_EndState(em);
        }
        break;
    }
}

/*
 * MELYNX_MOVE_WALK_LONG: walks (MELYNX_MOT_WALK) toward the goal heading
 * taken on entry, to within 100 or for the 600 frames ChangeState gives it,
 * then ends.
 */
void EmMelynx_WalkLong(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, MELYNX_MOT_WALK, 0, 0);
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

/*
 * MELYNX_MOVE_CHARGE: dashes (MELYNX_MOT_DASH) at the goal with a contact
 * attack (EmContactAttack_Create 4), turning to it each frame; ends within
 * 100, when it touches a player, or after 140 frames (cut short off
 * target).
 */
void EmMelynx_Charge(Em *em)
{
    s32 timeLeft;

    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 0x8C;
        Em_SetMotion(em, MELYNX_MOT_DASH, 0, 0);
        em->targetAngle = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        EmContactAttack_Create(em, 4);
        break;
    case 1:
        if ((Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) < 100.0f) || ((em->contactHit != 0) && (em->contact->isEm == 0)) || (timeLeft = em->timer - 1, em->timer = timeLeft, (timeLeft <= 0))) {
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

/*
 * MELYNX_MOVE_TURN: turns in place to the goal (walk motion without root
 * motion, EM_FLAG_NO_ROOT_MOTION) and ends once within one step.
 */
void EmMelynx_TurnToGoal(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->targetAngle = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        Em_SetMotion(em, MELYNX_MOT_WALK, 0, 0);
        Player_SetFlag((Player *) em, EM_FLAG_NO_ROOT_MOTION);
        break;
    case 1:
        if (Angle_TurnToward((s32 *) &em->rotY, (s32 *) &em->targetAngle, 0x800) < 0x800) {
            Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
            EmMelynx_EndState(em);
        }
        break;
    }
    Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
}

/*
 * MELYNX_MOVE_RUN_A: runs (MELYNX_MOT_RUN_A) toward the goal heading taken
 * on entry, to within 100 or for 300 frames, then ends.
 */
void EmMelynx_RunToGoalA(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 300;
        Em_SetMotion(em, MELYNX_MOT_RUN_A, 0, 0);
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

/* MELYNX_MOVE_RUN_B: as MELYNX_MOVE_RUN_A with MELYNX_MOT_RUN_B. */
void EmMelynx_RunToGoalB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 300;
        Em_SetMotion(em, MELYNX_MOT_RUN_B, 0, 0);
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

/*
 * MELYNX_MOVE_WANDER: walks (MELYNX_MOT_WALK) turning slowly to a random
 * heading for 120 to 247 frames, then ends (on the machine that runs it);
 * the state Start picks.
 */
void EmMelynx_Wander(Em *em)
{
    s32 timer;

    switch (em->step) {
    case 0:
        em->step++;
        em->timer = (em->stateRand & 0x7F) + 0x78;
        Em_SetMotion(em, MELYNX_MOT_WALK, 0, 0);
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

/*
 * MELYNX_MOVE_DASH: as MELYNX_MOVE_CHARGE without the contact attack, for
 * 100 frames.
 */
void EmMelynx_Dash(Em *em)
{
    s32 timeLeft;

    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 0x64;
        Em_SetMotion(em, MELYNX_MOT_DASH, 0, 0);
        em->targetAngle = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        break;
    case 1:
        if ((Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) < 100.0f) || ((em->contactHit != 0) && (em->contact->isEm == 0)) || (timeLeft = em->timer - 1, em->timer = timeLeft, (timeLeft <= 0))) {
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

/*
 * MELYNX_MOVE_WANDER_RUN: runs (MELYNX_MOT_RUN_B) turning slowly to a
 * random heading for 120 frames, then ends.
 */
void EmMelynx_WanderRun(Em *em)
{
    s32 timer;

    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 0x78;
        Em_SetMotion(em, MELYNX_MOT_RUN_B, 0, 0);
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

/*
 * MELYNX_MOVE_WALK: walks (MELYNX_MOT_WALK) toward the goal heading taken
 * on entry, to within 100 or for 300 frames, then ends.
 */
void EmMelynx_WalkToGoal(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 300;
        Em_SetMotion(em, MELYNX_MOT_WALK, 0, 0);
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

/*
 * MELYNX_MOVE_WALK_HOME: walks (MELYNX_MOT_WALK) turning slowly toward its
 * home (the spawn position) for 120 to 247 frames, then ends.
 */
void EmMelynx_WalkHome(Em *em)
{
    EmMelynxWork *w = (EmMelynxWork *)&em->work;
    s32 timer;

    switch (em->step) {
    case 0:
        em->step++;
        em->timer = (em->stateRand & 0x7F) + 0x78;
        Em_SetMotion(em, MELYNX_MOT_WALK, 0, 0);
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

/* Runs the move state (group 1) by Em.state; state 3 has no function. */
void EmMelynx_RunMoveState(Em *em)
{
    switch (em->state) {
    case MELYNX_MOVE_CHASE:
        EmMelynx_ChaseGoal(em);
        break;
    case MELYNX_MOVE_POSTURE:
        EmMelynx_PostureMove(em);
        break;
    case MELYNX_MOVE_WALK_LONG:
        EmMelynx_WalkLong(em);
        break;
    case MELYNX_MOVE_CHARGE:
        EmMelynx_Charge(em);
        break;
    case MELYNX_MOVE_TURN:
        EmMelynx_TurnToGoal(em);
        break;
    case MELYNX_MOVE_RUN_A:
        EmMelynx_RunToGoalA(em);
        break;
    case MELYNX_MOVE_RUN_B:
        EmMelynx_RunToGoalB(em);
        break;
    case MELYNX_MOVE_WANDER:
        EmMelynx_Wander(em);
        break;
    case MELYNX_MOVE_DASH:
        EmMelynx_Dash(em);
        break;
    case MELYNX_MOVE_WANDER_RUN:
        EmMelynx_WanderRun(em);
        break;
    case MELYNX_MOVE_WALK:
        EmMelynx_WalkToGoal(em);
        break;
    case MELYNX_MOVE_WALK_HOME:
        EmMelynx_WalkHome(em);
        
    default:
        break;
    }
}

/*
 * MELYNX_FLY_FALL: falls (MELYNX_MOT_FALL, gravity 0.5) to the floor,
 * stands and ends.
 */
void EmMelynx_Fall(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, MELYNX_MOT_FALL, 0, 0);
        em->velY = 0.0f;
        em->accelY = -0.5f;
        break;
    case 1:
        Player_Integrate(em);
        if (em->posY <= em->floorY) {
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            EmMelynx_EndState(em);
        }
        break;
    }
}

/* Runs the fall, the only state of group 2. */
void EmMelynx_RunFlyState(Em *em)
{
    switch (em->state) {
    case MELYNX_FLY_FALL:
        EmMelynx_Fall(em);
        break;
    }
}

/*
 * MELYNX_ATK_POUNCE: crouches (MELYNX_MOT_POUNCE_START), then leaps forward
 * (MELYNX_MOT_LEAP, contact attack 1); landing after a hit plays
 * MELYNX_MOT_SNATCH and ends, after a miss MELYNX_MOT_LEAP_LAND and then
 * MELYNX_IDLE_ACTION_B.
 */
void EmMelynx_Pounce(Em *em)
{
    f32 floorY;
    u8 stance;

    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, MELYNX_MOT_POUNCE_START, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stance = EM_STANCE_AIR;
            em->velX = 0.0f;
            em->velY = 16.0f;
            em->velY = 30.0f;
            em->velZ = 5.0f;
            Vec_RotateY(&em->velX, (6.2831855f * (f32) em->rotY) / 65536.0f);
            em->accelX = 0.0f;
            em->accelY = -1.0f;
            em->accelY = -3.0f;
            em->accelZ = 0.0f;
            Em_SetMotion(em, MELYNX_MOT_LEAP, 0, 0);
            EmContactAttack_Create(em, 1);
            em->stateVar0 = 0;
        }
        break;
    case 2:
        Player_Integrate(em);
        if ((em->motion[0].playing == 0) && ((em->posY - 50.0f) <= em->floorY)) {
            if (em->hitPending == 0) {
                em->step++;
                Em_SetMotion(em, MELYNX_MOT_LEAP_LAND, 0, 0);
                em->velY *= 0.5f;
                em->accelY = -0.5f;
                em->stateVar1 = 0;
                break;
            }
            em->step++;
            Em_SetMotion(em, MELYNX_MOT_SNATCH, 0, 0);
            em->stateVar1 = 1;
        }
        break;
    case 3:
        stance = em->stance;
        if (stance != 0) {
            floorY = em->floorY;
            if ((em->posY <= floorY) && (stance != 0)) {
                em->posY = floorY;
                em->stance = EM_STANCE_GROUND;
            } else {
                Player_Integrate(em);
            }
        }
        if (em->motion[0].playing == 0) {
            em->posY = em->floorY;
            em->stance = EM_STANCE_GROUND;
            if (em->stateVar1 != 0) {
                em->stateVar1 = 0;
                EmMelynx_EndState(em);
                break;
            }
            EmMelynx_ChangeState(em, EM_GROUP_IDLE, MELYNX_IDLE_ACTION_B, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * MELYNX_ATK_STEAL, MELYNX_ATK_STEAL_CHASE: runs at the goal
 * (MELYNX_MOT_RUN_B) with a contact attack (STEAL: kind 2, 60 frames, aims
 * once; STEAL_CHASE: kind 7, 120 frames, speed 1.2, aims each frame);
 * touching an active player steals an item (EmMelynx_StealItem), raises
 * request 1 and plays MELYNX_MOT_SNATCH; out of time it plays
 * MELYNX_MOT_IDLE_ACTION_B; ends with the motion.
 */
void EmMelynx_StealAttack(Em *em)
{
    EmMelynxWork *w = (EmMelynxWork *)&em->work;
    Player *victim;

    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, MELYNX_MOT_RUN_B, 0, 0);
        em->targetAngle = Math_AngleXZ(&em->posX, em->goal);
        if (em->state == 3) {
            em->timer = 120;
            w->eff = EmContactAttack_Create(em, 7);
            em->motionSpeed = 1.2f;
        } else {
            em->timer = 60;
            w->eff = EmContactAttack_Create(em, 2);
            em->motionSpeed = 1.0f;
        }
        break;
    case 1:
        if (em->contactHit != 0 && (victim = em->contact) != NULL && victim->isEm == 0 && victim == &gPlayers[victim->index] && victim->active) {
            EmMelynx_StealItem(em, victim, w);
            Em_SetRequest(em, 1, 0x90);
            em->step++;
            Em_SetMotion(em, MELYNX_MOT_SNATCH, 0, 0);
        } else if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, MELYNX_MOT_IDLE_ACTION_B, 0, 0);
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

/*
 * MELYNX_ATK_LEAP: leaps forward at once (MELYNX_MOT_LEAP, contact attack
 * 1) and lands (MELYNX_MOT_LEAP_LAND); after a hit it ends, after a miss
 * goes to MELYNX_IDLE_ACTION_B.
 */
void EmMelynx_Leap(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->velX = 0.0f;
        em->velY = 16.0f;
        em->velY = 30.0f;
        em->velZ = 5.0f;
        Vec_RotateY(&em->velX, 6.2831855f * em->rotY / 65536.0f);
        em->accelX = 0.0f;
        em->accelY = -1.0f;
        em->accelY = -3.0f;
        em->accelZ = 0.0f;
        Em_SetMotion(em, MELYNX_MOT_LEAP, 0, 0);
        EmContactAttack_Create(em, 1);
        em->stateVar0 = 0;
        break;
    case 1:
        Player_Integrate(em);
        if (em->motion[0].playing == 0 && em->posY - 50.0f <= em->floorY) {
            if (em->hitPending == 0) {
                em->step++;
                Em_SetMotion(em, MELYNX_MOT_LEAP_LAND, 0, 0);
                em->velY *= 0.5f;
                em->accelY = -0.5f;
                em->stateVar1 = 0;
            } else {
                em->step++;
                Em_SetMotion(em, MELYNX_MOT_LEAP_LAND, 0, 0);
                em->stateVar1 = 1;
            }
        }
        break;
    case 2:
        if (em->stance != EM_STANCE_GROUND) {
            if (em->posY <= em->floorY && em->stance != EM_STANCE_GROUND) {
                em->posY = em->floorY;
                em->stance = EM_STANCE_GROUND;
            } else {
                Player_Integrate(em);
            }
        }
        if (em->motion[0].playing == 0) {
            em->posY = em->floorY;
            em->stance = EM_STANCE_GROUND;
            if (em->stateVar1 != 0) {
                em->stateVar1 = 0;
                EmMelynx_EndState(em);
                break;
            }
            EmMelynx_ChangeState(em, EM_GROUP_IDLE, MELYNX_IDLE_ACTION_B, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * MELYNX_ATK_THROW_BOMB, MELYNX_ATK_THROW_BOMB_FAR: takes out a bomb
 * (MELYNX_MOT_BOMB_TAKE_OUT; at frame 20 EmBomb_Create type 2, or type 3
 * thrown farther, and holdBomb set), 60 frames later throws
 * (MELYNX_MOT_BOMB_THROW), letting go at frame 48 (holdBomb 0), and ends.
 */
void EmMelynx_ThrowBomb(Em *em)
{
    EmMelynxWork *w = (EmMelynxWork *)&em->work;

    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 0x14;
        Em_SetMotion(em, MELYNX_MOT_BOMB_TAKE_OUT, 0, 0);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            em->timer = 0x3C;
            w->holdBomb = 1;
            if (em->state == 4) {
                EmBomb_Create(em, 2);
                break;
            }
            EmBomb_Create(em, 3);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, MELYNX_MOT_BOMB_THROW, 0, 0);
        }
        break;
    case 3:
        if (EmMotion_PassedFrameScaled(em, 48.0f, 0U) != 0) {
            em->step++;
            w->holdBomb = 0;
        }
        
    case 4:
        if (em->motion[0].playing == 0) {
            EmMelynx_EndState(em);
        }
        break;
    }
}

/*
 * Runs the attack state (group 3) by Em.state; the two steals and the two
 * throws share a function, which tests Em.state.
 */
void EmMelynx_RunAttackState(Em *em)
{
    switch (em->state) {
    case MELYNX_ATK_POUNCE:
        EmMelynx_Pounce(em);
        break;
    case MELYNX_ATK_STEAL:
        EmMelynx_StealAttack(em);
        break;
    case MELYNX_ATK_LEAP:
        EmMelynx_Leap(em);
        break;
    case MELYNX_ATK_STEAL_CHASE:
        EmMelynx_StealAttack(em);
        break;
    case MELYNX_ATK_THROW_BOMB:
        EmMelynx_ThrowBomb(em);
        break;
    case MELYNX_ATK_THROW_BOMB_FAR:
        EmMelynx_ThrowBomb(em);
        
    default:
        break;
    }
}

/*
 * MELYNX_REACT_FLINCH: faces the hit, flinches (MELYNX_MOT_FLINCH, then
 * MELYNX_MOT_FLINCH_END), restarts the script and ends.
 */
void EmMelynx_Flinch(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, MELYNX_MOT_FLINCH, 0, 0);
        em->rotY = em->hitAngle + 0x7FFF + 1;
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, MELYNX_MOT_FLINCH_END, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmMelynx_EndState(em);
        }
        break;
    }
}

/*
 * MELYNX_REACT_KNOCK_BACK: faces the hit and is thrown back
 * (MELYNX_MOT_KNOCK_BACK, 10 up and 12 back), lands (Player_MoveFall), lies
 * 90 frames (MELYNX_MOT_LIE), gets up (MELYNX_MOT_GET_UP) and ends.
 */
void EmMelynx_KnockBack(Em *em)
{
    f32 launchLocal[3];
    f32 launchVel[3];
    s32 angles[3];
    f32 rotMtx[16];

    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, MELYNX_MOT_KNOCK_BACK, 0, 0);
        angles[0] = 0;
        em->rotY = angles[1] = em->hitAngle + 0x7FFF + 1;
        angles[2] = 0;
        Mtx_FromAnglesXYZ((s32 *) angles, rotMtx);
        launchLocal[0] = 0.0f;
        launchLocal[1] = 10.0f;
        launchLocal[2] = -12.0f;
        Vec_Rotate(launchVel, launchLocal, rotMtx);
        Player_ClearAccel2(em);
        em->velX = launchVel[0];
        em->velY = launchVel[1];
        em->velZ = launchVel[2];
        em->accelY = -0.4f;
        em->accelZ = 0.15f;
        em->stance = EM_STANCE_AIR;
        Em_RestartScript(em);
        break;
    case 1:
        if (!(em->velZ * em->accelZ < 0.0f)) {
            em->accelZ = 0.0f;
        }
        if (Player_MoveFall((Player *) em) != 0) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, MELYNX_MOT_LIE, 0, 0);
            em->timer = 90;
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, MELYNX_MOT_GET_UP, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            EmMelynx_EndState(em);
        }
        break;
    }
}

/*
 * MELYNX_REACT_DAZE: reels (MELYNX_MOT_DIZZY), raises request 0 and
 * restarts the script, for the timer it finds, then gets up and ends.
 */
void EmMelynx_Daze(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, MELYNX_MOT_DIZZY, 0, 0);
        Em_SetRequest(em, 0U, 0x90U);
        Em_RestartScript(em);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, MELYNX_MOT_GET_UP, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmMelynx_EndState(em);
        }
        break;
    }
}

/*
 * MELYNX_REACT_STATUS4: under status 4 (Em_BeginStatus4, its effect, hits
 * kept: EM_HIT_REACT_STATUS, noReact) faces the hit and flinches, from
 * frame 120 plays MELYNX_MOT_STATUS4_START for 58 frames, then
 * MELYNX_MOT_STATUS4_LOOP until the timer (Em_SetStatus4Timer) runs out;
 * ends status 4, recovers (MELYNX_MOT_FLINCH_END) and ends.
 */
void EmMelynx_Status4(Em *em)
{
    EmMelynxWork *w = (EmMelynxWork *)&em->work;

    if (em->timer > 0) {
        em->timer--;
    }
    if (em->step <= 3) {
        Em_SpawnStatus4Effect(em, 2);
        Em_SetHitReactMode(em, EM_HIT_REACT_STATUS);
    }
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, MELYNX_MOT_FLINCH, 0, 0);
        em->rotY = em->hitAngle + 0x7FFF + 1;
        Em_BeginStatus4(em);
        em->noReact = 1;
        Em_RestartScript(em);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 120.0f, 0) != 0) {
            em->step++;
            Em_SetMotion(em, MELYNX_MOT_STATUS4_START, 0, 0);
            w->stateTimer = 0x3A;
        }
        break;
    case 2:
        if (--w->stateTimer <= 0) {
            em->step++;
            Em_SetMotion(em, MELYNX_MOT_STATUS4_LOOP, 0, 0);
        }
        break;
    case 3:
        if (em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, MELYNX_MOT_FLINCH_END, 0, 0);
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

/*
 * MELYNX_REACT_STUN: reels (MELYNX_MOT_DIZZY) under stun stars for the
 * timer Em_SetStatus5Timer set, then MELYNX_IDLE_STUN_END.
 */
void EmMelynx_Stun(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, MELYNX_MOT_DIZZY, 0, 0);
        StunStars_CreateEm(em, 1, 0, *(s16 *)&em->timer, 90.0f, 1.0f); /* the timer's low half */
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmMelynx_ChangeState(em, EM_GROUP_IDLE, MELYNX_IDLE_STUN_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmMelynx_ChangeState(em, EM_GROUP_IDLE, MELYNX_IDLE_STUN_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/* Runs the reaction state (group 4) by Em.state. */
void EmMelynx_RunReactState(Em *em)
{
    switch (em->state) {
    case MELYNX_REACT_FLINCH:
        EmMelynx_Flinch(em);
        break;
    case MELYNX_REACT_KNOCK_BACK:
        EmMelynx_KnockBack(em);
        break;
    case MELYNX_REACT_DAZE:
        EmMelynx_Daze(em);
        break;
    case MELYNX_REACT_STATUS4:
        EmMelynx_Status4(em);
        break;
    case MELYNX_REACT_STUN:
        EmMelynx_Stun(em);
        break;
    }
}

/*
 * MELYNX_DEFEAT_KNOCK_BACK: hunted while off the ground: unhittable and out
 * of combat, gives the item back (EmMelynx_ReturnItem), counts the hunt
 * (QuestEm_Hunted), is thrown back, lies 300 frames, gets up, burrows away
 * (MELYNX_MOT_BURROW), hides, drops its carve point (EmMine_Drop, Melynx
 * kind only) and goes to MELYNX_DEFEAT_REAPPEAR.
 */
void EmMelynx_HuntedKnockBack(Em *em)
{
    EmMelynxWork *w = (EmMelynxWork *)&em->work;
    f32 launchLocal[3];
    f32 launchVel[3];
    s32 angles[3];
    f32 rotMtx[16];

    em->noHitTimer = 10;
    em->noPushTimer = 10;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, MELYNX_MOT_KNOCK_BACK, 0, 0);
        launchLocal[0] = 0.0f;
        launchLocal[1] = 10.0f;
        angles[0] = 0;
        launchLocal[2] = -12.0f;
        em->rotY = angles[1] = em->hitAngle + 0x7FFF + 1;
        angles[2] = 0;
        Mtx_FromAnglesXYZ((s32 *) angles, rotMtx);
        Vec_Rotate(launchVel, launchLocal, rotMtx);
        Player_ClearAccel2(em);
        em->velX = launchVel[0];
        em->velY = launchVel[1];
        em->velZ = launchVel[2];
        em->accelY = -0.4f;
        em->accelZ = 0.15f;
        em->stance = EM_STANCE_AIR;
        EmMelynx_ReturnItem(em, w);
        QuestEm_Hunted(em);
        break;
    case 1:
        if (!(em->velZ * em->accelZ < 0.0f)) {
            em->accelZ = 0.0f;
        }
        if (Player_MoveFall((Player *) em) != 0) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, MELYNX_MOT_LIE, 0, 0);
            em->timer = 300;
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, MELYNX_MOT_GET_UP, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, MELYNX_MOT_BURROW, 0, 0);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            if (w->alertState == 1) {
                ObjListA70_Remove(&w->sense);
                w->alertState = 2;
            }
            em->visible = 0;
            if (em->type == EM_KIND_MELYNX) {
                EmMine_Drop(em, &em->posX);
                EmMelynx_ChangeState(em, EM_GROUP_DEFEAT, MELYNX_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
            } else {
                EmMelynx_ChangeState(em, EM_GROUP_DEFEAT, MELYNX_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
            }
        }
        break;
    case 5:
        if (--em->timer <= 0) {
            EmMine_Remove(em);
            EmMelynx_ChangeState(em, EM_GROUP_DEFEAT, MELYNX_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * MELYNX_DEFEAT_FLINCH: hunted on the ground: as MELYNX_DEFEAT_KNOCK_BACK,
 * but faces the hit and flinches (MELYNX_MOT_FLINCH, MELYNX_MOT_FLINCH_END)
 * before burrowing away.
 */
void EmMelynx_HuntedFlinch(Em *em)
{
    EmMelynxWork *w = (EmMelynxWork *)&em->work;

    em->noHitTimer = 10;
    em->noPushTimer = 10;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, MELYNX_MOT_FLINCH, 0, 0);
        em->rotY = em->hitAngle + 0x7FFF + 1;
        em->stance = EM_STANCE_GROUND;
        EmMelynx_ReturnItem(em, w);
        QuestEm_Hunted(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, MELYNX_MOT_FLINCH_END, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, MELYNX_MOT_BURROW, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            if (w->alertState == 1) {
                ObjListA70_Remove(&w->sense);
                w->alertState = 2;
            }
            em->visible = 0;
            if (em->type == EM_KIND_MELYNX) {
                EmMine_Drop(em, &em->posX);
                EmMelynx_ChangeState(em, EM_GROUP_DEFEAT, MELYNX_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
            } else {
                EmMelynx_ChangeState(em, EM_GROUP_DEFEAT, MELYNX_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
            }
        }
        break;
    case 4:
        if (--em->timer <= 0) {
            EmMine_Remove(em);
            EmMelynx_ChangeState(em, EM_GROUP_DEFEAT, MELYNX_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * MELYNX_DEFEAT_REAPPEAR: when the quest lets it appear again
 * (QuestEm_CanAppear; ChangeState already moved it with QuestEm_Appear)
 * starts over as Start does (variant 0xC: MELYNX_IDLE_STAND, else
 * MELYNX_MOVE_WANDER); otherwise moves its task on (taskPhase) to be
 * finished.
 */
void EmMelynx_Reappear(Em *em)
{
    switch (em->step) {
    case 0:
        if (QuestEm_CanAppear(em) == 1) {
            if (em->variant == 0xC) {
                EmMelynx_ChangeState(em, EM_GROUP_IDLE, MELYNX_IDLE_STAND, EM_SYNC_LOCAL);
                break;
            }
            EmMelynx_ChangeState(em, EM_GROUP_MOVE, MELYNX_MOVE_WANDER, EM_SYNC_LOCAL);
            break;
        }
        em->taskPhase++;
    case 1:
        break;
    }
}

/* Runs the defeat state (group 5) by Em.state. */
void EmMelynx_RunDefeatState(Em *em)
{
    switch (em->state) {
    case MELYNX_DEFEAT_KNOCK_BACK:
        EmMelynx_HuntedKnockBack(em);
        break;
    case MELYNX_DEFEAT_FLINCH:
        EmMelynx_HuntedFlinch(em);
        break;
    case MELYNX_DEFEAT_REAPPEAR:
        EmMelynx_Reappear(em);
        break;
    }
}

/*
 * MELYNX_EVENT_APPEAR: hidden for one frame (MELYNX_MOT_IDLE, no sensing,
 * unhittable, no look-at, script not pending), then shown and ends.
 */
void EmMelynx_Appear(Em *em)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, MELYNX_MOT_IDLE, 0, 0);
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

/* Runs the event state, the only state of group 6. */
void EmMelynx_RunEventState(Em *em)
{
    switch (em->state) {
    case MELYNX_EVENT_APPEAR:
        EmMelynx_Appear(em);
        break;
    }
}

/*
 * The per-frame update: keeps desert variants unhittable, counts spareTimer
 * down, checks rage by hp in combat, senses objects (calm, standing and out
 * of combat: raises EM_TRIGGER_SENSED, alertState 2), reads the hits
 * (Em_UpdateDamage) and picks the reaction or defeat state; any result
 * drops the bomb (holdBomb 0), clears blind and, when calm, lists its sense
 * object at its position (alertState 1); then runs the state (twice after a
 * state request).
 */
void EmMelynx_Update(Em *em)
{
    u8 damageOut[4];
    EmMelynxWork *w = (EmMelynxWork *)&em->work;
    u32 result;

    if (Area_DayId(em->area) == AREA_DESERT_8 && em->variant != 0) {
        em->noHitTimer = 10;
        em->noPushTimer = 10;
    }
    if (w->unk00C != 0) {
        w->unk00C -= 1;
    }
    if (em->combat == 1 && em->rage == 0) {
        Em_CheckRageByHp(em, (s16) (em->rageThreshold / 1800));
    }
    result = (u8) Em_SenseObjects(em, &w->sense);
    if (w->alertState == 0 && em->stance == EM_STANCE_GROUND && em->combat == 0 && result != 0 && Em_ScriptFlagsFrom(em, 4) == 0) {
        em->triggers |= 4;
        w->alertState = 2;
        em->blind = 0;
    }
    if (w->alertState == 1) {
        ObjListA70_Remove(&w->sense);
        w->alertState = 2;
    }
    result = Em_UpdateDamage(em, damageOut) & 0xFF;
    switch (result) {
    case EM_DMG_NONE:
    case EM_DMG_ATTACK9:
    case EM_DMG_ATTACK6:
    case EM_DMG_FLASH:
    case EM_DMG_PART8_DOWN:
    case EM_DMG_ATTACK5:
    case EM_DMG_ATTACK5_FLINCH:
    case EM_DMG_ATTACK13:
        break;
    case EM_DMG_KILLED:
    case EM_DMG_DEAD:
        if (em->stance == EM_STANCE_AIR) {
            EmMelynx_ChangeState(em, EM_GROUP_DEFEAT, MELYNX_DEFEAT_KNOCK_BACK, EM_SYNC_OWNER);
        }
        EmMelynx_ChangeState(em, EM_GROUP_DEFEAT, MELYNX_DEFEAT_FLINCH, EM_SYNC_OWNER);
        break;
    case EM_DMG_STATUS_IN_AIR:
        if (Player_IsState((Player *) em, 4, 1) == 0) {
            EmMelynx_ChangeState(em, EM_GROUP_REACT, MELYNX_REACT_KNOCK_BACK, EM_SYNC_OWNER);
            Em_ResetPartHp(em, 0);
        }
        break;
    case EM_DMG_STATUS4:
        if (Player_IsState((Player *) em, 4, 1) == 0) {
            Em_SetStatus4Timer(em);
            EmMelynx_ChangeState(em, EM_GROUP_REACT, MELYNX_REACT_STATUS4, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_CAPTURED:
        EmMelynx_ChangeState(em, EM_GROUP_REACT, MELYNX_REACT_KNOCK_BACK, EM_SYNC_OWNER);
        Em_ResetPartHp(em, 0);
        break;
    case EM_DMG_STATUS2:
        if (Player_IsState((Player *) em, 4, 1) == 0) {
            Em_SetStatus2Timer(em);
            EmMelynx_ChangeState(em, EM_GROUP_IDLE, MELYNX_IDLE_STATUS2_LIE, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_HIT_LYING:
        em->blind = 0;
        Em_EndStatus2(em);
        EmMelynx_ChangeState(em, EM_GROUP_REACT, MELYNX_REACT_KNOCK_BACK, EM_SYNC_OWNER);
        break;
    case EM_DMG_PART_FLINCH:
    case EM_DMG_HIT_MODE3:
        EmMelynx_ChangeState(em, EM_GROUP_REACT, MELYNX_REACT_KNOCK_BACK, EM_SYNC_OWNER);
        Em_ResetPartHp(em, 0);
        break;
    case EM_DMG_HIT:
        if (em->stance == EM_STANCE_AIR) {
            EmMelynx_ChangeState(em, EM_GROUP_REACT, MELYNX_REACT_KNOCK_BACK, EM_SYNC_OWNER);
            Em_ResetPartHp(em, 0);
        } else {
            EmMelynx_ChangeState(em, EM_GROUP_REACT, MELYNX_REACT_FLINCH, EM_SYNC_OWNER);
        }
        em->scriptPending = 0;
        break;
    case EM_DMG_STATUS5:
        if ((em->stateGroup != EM_GROUP_REACT || em->state != MELYNX_REACT_STUN) && (em->stateGroup != EM_GROUP_REACT || em->state != MELYNX_REACT_KNOCK_BACK)) {
            Em_SetStatus5Timer(em);
            EmMelynx_ChangeState(em, EM_GROUP_REACT, MELYNX_REACT_STUN, EM_SYNC_OWNER);
        }
        break;
    }
    if ((u8) result != 0) {
        w->holdBomb = 0;
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
    Em_ClearHitReactMode(em);
    EmMelynx_RunState(em);
    if (em->stateRequest != 0) {
        EmMelynx_RunState(em);
        em->stateRequest = 0;
    }
}

/*
 * Runs the dispatcher of Em.stateGroup (groups 0 to 6; group 7 runs the
 * idle dispatcher).
 */
void EmMelynx_RunState(Em *em)
{
    switch (em->stateGroup) {
    case EM_GROUP_IDLE:
        EmMelynx_RunIdleState(em);
        break;
    case EM_GROUP_MOVE:
        EmMelynx_RunMoveState(em);
        break;
    case EM_GROUP_FLY:
        EmMelynx_RunFlyState(em);
        break;
    case EM_GROUP_ATTACK:
        EmMelynx_RunAttackState(em);
        break;
    case EM_GROUP_REACT:
        EmMelynx_RunReactState(em);
        break;
    case EM_GROUP_DEFEAT:
        EmMelynx_RunDefeatState(em);
        break;
    case EM_GROUP_EVENT:
        EmMelynx_RunEventState(em);
        break;
    case EM_GROUP_EVENT2:
        EmMelynx_RunIdleState(em);
        
    default:
        break;
    }
}

/* The motion effects default case: does nothing. */
void EmMelynx_MotionEffectsDefault(Em *em)
{
}

/*
 * Keeps work->fxMotion as the layer-0 motion, plays its sounds
 * (EmMelynx_MotionSounds) and adds its effects: dust clouds and Effect0D
 * bursts on given frames of MELYNX_MOT_BURROW.
 */
void EmMelynx_MotionEffects(Em *em, EmWork *work)
{
    if (em->motionId[0] != work->fxMotion) {
        work->fxMotion = em->motionId[0];
    }
    EmMelynx_MotionSounds(em, work);
    switch (work->fxMotion) {
    case EM_MOTION_ID(0, MELYNX_MOT_IDLE):
    case EM_MOTION_ID(0, MELYNX_MOT_IDLE_ACTION_A):
    case EM_MOTION_ID(0, MELYNX_MOT_IDLE_ACTION_B):
    case EM_MOTION_ID(0, MELYNX_MOT_IDLE_ACTION_C):
    case EM_MOTION_ID(0, MELYNX_MOT_IDLE_ACTION_D):
    case EM_MOTION_ID(0, MELYNX_MOT_WALK):
    case EM_MOTION_ID(0, MELYNX_MOT_RUN_A):
    case EM_MOTION_ID(0, MELYNX_MOT_FALL):
    case EM_MOTION_ID(0, MELYNX_MOT_POSTURE_MOVE):
    case EM_MOTION_ID(0, MELYNX_MOT_RUN_B):
    case EM_MOTION_ID(0, MELYNX_MOT_DASH):
        break;
    case EM_MOTION_ID(0, MELYNX_MOT_BURROW):
        if (EmMotion_PassedFrameScaled(em, 18.0f, 0)) {
            DustCloud_Create(em, 3, 4, 0.5f);
        }
        if (EmMotion_PassedFrameScaled(em, 58.0f, 0)) {
            DustCloud_Create(em, 3, 4, 0.5f);
        }
        if (EmMotion_PassedFrameScaled(em, 38.0f, 0)) {
            DustCloud_Create(em, 2, 4, 0.5f);
        }
        if (EmMotion_PassedFrameScaled(em, 126.0f, 0)) {
            PartFx_SpawnForEm(em, 2, 7);
        }
        if (EmMotion_PassedFrameScaled(em, 158.0f, 0)) {
            PartFx_SpawnForEm(em, 2, 3);
        }
        if (EmMotion_PassedFrameScaled(em, 166.0f, 0)) {
            PartFx_SpawnForEm(em, 2, 3);
        }
        if (EmMotion_PassedFrameScaled(em, 176.0f, 0)) {
            PartFx_SpawnForEm(em, 2, 3);
        }
        break;
    case EM_MOTION_ID(0, MELYNX_MOT_POUNCE_START):
    case EM_MOTION_ID(0, MELYNX_MOT_LEAP):
    case EM_MOTION_ID(0, MELYNX_MOT_SNATCH):
    case EM_MOTION_ID(0, MELYNX_MOT_LEAP_LAND):
    case EM_MOTION_ID(0, MELYNX_MOT_DIZZY):
    case EM_MOTION_ID(0, MELYNX_MOT_POSTURE_LOOP):
    case EM_MOTION_ID(0, MELYNX_MOT_POSTURE_START):
    case EM_MOTION_ID(0, MELYNX_MOT_POSTURE_END):
    case EM_MOTION_ID(0, MELYNX_MOT_IDLE_ACTION_G):
    case EM_MOTION_ID(0, MELYNX_MOT_BOMB_TAKE_OUT):
    case EM_MOTION_ID(0, MELYNX_MOT_IDLE_ACTION_E):
    case EM_MOTION_ID(0, MELYNX_MOT_IDLE_ACTION_F):
    case EM_MOTION_ID(0, MELYNX_MOT_FLINCH):
    case EM_MOTION_ID(0, MELYNX_MOT_KNOCK_BACK):
    case EM_MOTION_ID(0, MELYNX_MOT_LIE):
    case EM_MOTION_ID(0, MELYNX_MOT_GET_UP):
    case EM_MOTION_ID(0, MELYNX_MOT_FLINCH_END):
    case EM_MOTION_ID(0, MELYNX_MOT_STATUS4_START):
    case EM_MOTION_ID(0, MELYNX_MOT_STATUS4_LOOP):
    case EM_MOTION_ID(0, MELYNX_MOT_STATUS2_LIE):
    case EM_MOTION_ID(0, 0x46):
    case EM_MOTION_ID(0, 0x47):
    case EM_MOTION_ID(0, 0x48):
    case EM_MOTION_ID(0, 0x49):
    case EM_MOTION_ID(0, MELYNX_MOT_BOMB_THROW):
    case EM_MOTION_ID(0, 0x4B):
        break;
    default:
        EmMelynx_MotionEffectsDefault(em);
        break;
    }
}

/*
 * The face hook of the table: seeds blinkTimer once, then sets eyeFrame (3
 * hurt in groups 4 and 5, 2 closed in MELYNX_MOT_STATUS2_LIE, else the
 * blink cycle from blinkTimer) and runs EmMelynx_MotionEffects.
 */
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
        if (em->stateGroup == EM_GROUP_REACT || em->stateGroup == EM_GROUP_DEFEAT) {
            w->eyeFrame = 3;
            w->blinkTimer = 2;
        } else if (em->motionId[0] != EM_MOTION_ID(0, MELYNX_MOT_STATUS2_LIE)) {
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

/*
 * Gives the meshes of mesh set kind the monster alpha; in sets 0 and 1
 * hides the eye meshes 1 to 4 but the one eyeFrame picks (open, half shut,
 * closed, hurt), and sends each mesh to the GS state.
 */
void EmMelynx_SetMeshSetAlpha(Em *em, s32 kind, EmMelynxMeshSet *sets)
{
    ModelMat *meshes;
    s32 i;
    EmMelynxWork *w;
    ModelMat *mat;

    meshes = em->model->mats;
    w = (EmMelynxWork *)&em->work;
    for (i = 0; i < sets[kind].count; i++) {
        mat = &meshes[sets[kind].mesh[i]];
        mat->alpha = em->alpha;
        switch (kind) {
        case 0:
        case 1:
            switch (w->eyeFrame) {
            case 0:
            default:
                switch (i) {
                case 2:
                case 3:
                case 4:
                    mat->alpha = 0.0f;
                    break;
                }
                break;
            case 1:
                switch (i) {
                case 1:
                case 3:
                case 4:
                    mat->alpha = 0.0f;
                    break;
                }
                break;
            case 2:
                switch (i) {
                case 1:
                case 2:
                case 4:
                    mat->alpha = 0.0f;
                    break;
                }
                break;
            case 3:
                switch (i) {
                case 1:
                case 2:
                case 3:
                    mat->alpha = 0.0f;
                    break;
                }
                break;
            }
            break;
        }
        Gs_SetState((i + 0x3A) & 0xFF, (s32) mat);
    }
}

/* The two effect hooks of the table: Shadow_Spawn with type 0. */
void EmMelynx_SpawnEffect01(u8 *em)
{
    Shadow_Spawn((Player *) em, 0);
}

/* The empty hook of the table. */
void EmMelynx_TableNop(void)
{
}

/* The file's initialised data, in the game's order */
s32 GmEm_ReturnZero(void); /* gm_sub */
void *EmMelynx_Callbacks[8] = {
    EmMelynx_SpawnEffect01, EmMelynx_SpawnEffect01, EmMelynx_TableNop, EmMelynx_UpdateFace,
    GmEm_ReturnZero, EmMelynx_Reinit, NULL, NULL,
};
