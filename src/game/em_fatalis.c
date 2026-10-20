/*
 * The Fatalis family (EM_KIND_FATALIS, EM_KIND_FATALIS_B and
 * EM_KIND_FATALIS_C; include/em_fatalis.h names the states and motions):
 * the code from the family's first function, EmFatalis_MotionSounds, to the
 * Blangonga family's first. EmFatalis_Init sets a monster up.
 *
 * Fatalis has the Rathalos family's shape: EmFatalis_Update reads the hits
 * (Em_UpdateDamage) and picks the reactions, then EmFatalis_RunState calls
 * the group's dispatcher (EmFatalis_RunIdleState ... RunEventState), which
 * calls one function per state with the monster and its work area (EmWork);
 * a state function switches on Em.step, and EmFatalis_EndState hands the
 * choice back to the monster script. Each Fatalis kind also plays a scene in
 * its own area (EmFatalis_Update), and leaves a carcass with three mine
 * points. The family's Set*State, ChangeState and curve functions sit apart
 * in the game's code, after the Bulldrome family's.
 */
#include "common.h"
#include "scene.h"
#include "quest.h"
#include "mine_point.h"
#include "em.h"
#include "em_fatalis.h"
#include "player.h"
#include "objattach.h"
#include "stageobj.h"
#include "area.h"
u16 Rand_Next(s32 stream);
/* prototypes */
void EmFatalis_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void Em_PlaySeAtBoneOnFrame(Em *em, s32 frame, s32 se, s32 bone);
void Em_SetLayerMotion(Em *em, s32 motion, s32 blend, s32 frame, s32 layer);
void Em_SetMotion(Em *em, s32 motion, s32 a, s32 b);
u16 Math_AngleXZ(f32 *from, f32 *to);
void Player_ClearFlag(void *, u32 flag);
void Player_SetFlag(void *obj, u32 flag);
void FatalisAttack_Create(Em *, s8);
void EmRangedAttack_CreateAtBone(Em *em, s32 a, s32 b, s32 c, s32 d, s32 e);
void *EmEffectAttack_Create(Em *em, s32 kind, s32 c, s32 a, s32 b);
void Em_MoveRotated(Em *em, void *src);
void Em_MoveRotatedAccel(Em *em, void *ang);
void Em_GetPitchToGoal(Em *em, s32 *out, f32 a, f32 b);
void Em_SpawnStatus4Effect(Em *em, s32 a);
void Em_SetHitReactMode(Em *em, u8 a);
void Em_PlaySeAtBoneVolOnFrame(Em *em, s32 frame, s32 se, s32 bone, s32 vol);
s32 Em_StepToGoal(Em *em, u16 step, u8 flags);
u8 Em_TurnToGoal(Em *em, u16 step, u8 flags);
void Em_StartTurnToTarget(Em *em, EmTurnParams *t);
s32 Em_TurnStep(Em *em, EmTurnParams *t);
void Em_RestartScript(Em *em);
void PartFx_SpawnForEmScaled(Em *em, s32 a, s32 b, f32 scale);
void ElementBurst_CreateOnEm(Em *em, u8 k, u8 c, f32 scale);
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
void MonsterFx_CreateOnEmVariant(Em *em, u8 k, u8 c, f32 scale);
void EmFatalis_MotionEffects(Em *em, EmFatalisWork *work);
void EmFatalis_MoveByVelYCurve(Em *, s32);
void EmFatalis_StartCurves(Em *em, u8 arg1);
/* unit's own declaration: declared returning s32; with the definition's u8 result the callers (EmFatalis_TakeOff, EmFatalis_TakeOffAndRise ...) mask the result to a byte before testing it, which the game's code doesn't do */
s32 EmFatalis_UpdateCurves(Em *);
void AttackBurst_CreateAtBone(Em *em, s16 bone, s16 k, u16 c);
void DustCloud_Create(Em *em, s16 k, s16 c, f32 scale);
/* unit's own declaration: the arguments are declared s32, as EmFatalis_SpawnFireball passes them; with the definition's u8, u16 and s16 parameters the compiler narrows them before the call, which the game's code doesn't do */
void MonsterFx_CreateOnEmAngled(Em *em, s32, s32, s32, f32);
s32 EmMotion_PassedFrameScaled(void *p, f32 frame, s32 layer);
s32 EmMotion_ReachedFrame(void *obj, s32 a, f32 frame);
s32 EmMotion_InFrameRange(void *em, s32 layer, f32 from, f32 to);
void CamShake_ReqAIf(Em *obj, s32 i);
f32 Vec_DistanceXZ(f32 *a, f32 *b);
void Player_ClearVelocity(void *);
void Shadow_Spawn(void *obj, s16 type);
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
void MinePoint_PrepareStub(MinePoint *pt);
void Vec_Set(f32 *out, f32 x, f32 y, f32 z);
void Em_EndStatus5(Em *em);
void Em_CheckRageByHp(Em *em, s16 n);
void Em_SpawnLyingPuffs(Em *em, s32 bone, f32 *pos, f32 scale);
void Em_SetMotionRow(Em *em, s32 row, s32 arg2, s32 arg3);
s32 Em_ComputeMaxHp(Em *em, s16 idx);
StageObj *MonsterFx_CreateOnEm(Em *em, u8 k, f32 scale);
s32 Save_TestEventFlag(s32);
/*
 * The spots of Fatalis's sweeps and blasts: eight rows (row 0 empty), each a
 * list of spot numbers the effects take in turn (EmFatalis_Sweep: rows 1-5;
 * EmFatalis_Blasts: rows 6 and 7). The table has no definition of its own:
 * the linker places it 0x1C bytes into a block of spot lists at the end of
 * this file, after row 7's list, and its rows 5 to 7 fill the next block.
 */
extern s8 *EmFatalis_SweepSpots[];
void GmEm_ResetSize(Em *em);
void *EmEffectAttack_CreateDescentBlast(Em *em, s16 a, s16 b, s32 c, s32 d);
void Em_RaisePlayerAreaTriggerLax(Em *em);
void Em_HoldPosNoPlayers(Em *em, u16 a, u16 b);
u16 Area_DayId(u16 id);
s32 Quest_GetOutcome(s32 withLoss);
void CamReq_ForMonster(Em *em);
u8 Em_UpdateDamage(Em *em, u8 *out);
void Em_UpdatePendingScript(Em *em);
void Em_ClearHitReactMode(Em *em);
void Em_SetStatus4Timer(Em *em);
void Em_SetStatus2Timer(Em *em);
void EmFatalis_RunState(Em *em, EmWork *work);
void EmFatalis_UpdateRageTimer(Em *em);
f32 *Mtx_FromAnglesYXZ(s32 *angles, f32 *m);
void Vec_Rotate(f32 *out, f32 *in, f32 *mtx);
/* end of prototypes */

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
 * AREA_CASTLE_1 it stands at a point set by its slot number; its hp comes from
 * Em_ComputeMaxHp; it starts in FAT_IDLE, in combat with every player, with
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
        case AREA_CASTLE_1:
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
    size = Em_ComputeMaxHp(em, 0);
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
        point.action = MINE_ACTION_CARVE;
        point.count = EmFatalis_MinePtBones[i][1];
        MinePoint_PrepareStub(&point);
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
        MonsterFx_CreateOnEmAngled(em, 0x26, angleB, angle, 1.0f);
        return;
    }
    AttackBurst_CreateAtBone(em, 0x33, 1, 1);
    AttackBurst_CreateAtBone(em, 0x33, 2, 1);
}

/*
 * A fireball's effect, pitched and turned by the arguments: EmEffectAttack kind
 * 0xD for EM_KIND_FATALIS_C, EmRangedAttack type 0xB at bone 0x33 for the others.
 */
void EmFatalis_SpawnFireballEffect(Em *em, s32 pitch, s32 yaw)
{
    if (em->type == EM_KIND_FATALIS_C) {
        EmEffectAttack_Create(em, EMFX_FATALIS_FIREBALL, 0, pitch, yaw);
        return;
    }
    EmRangedAttack_CreateAtBone(em, 0x33, 0xB, 0, pitch, yaw);
}

/*
 * One of a sweep's effects, at the spot the argument names (a value of an
 * EmFatalis_SweepSpots row): EmEffectAttack kind 0xC for EM_KIND_FATALIS_C,
 * EmRangedAttack type 0xC from bone 0 pitched 0x4000 for the others.
 */
void EmFatalis_SpawnSweepEffect(Em *em, s32 spot)
{
    if (em->type == EM_KIND_FATALIS_C) {
        EmEffectAttack_Create(em, EMFX_FATALIS_SWEEP_BLAST, spot, 0, 0);
    } else {
        EmRangedAttack_CreateAtBone(em, 0, 0xC, spot, 0x4000, 0);
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
    Em_RaisePlayerAreaTriggerLax(em);
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
        Player_ClearFlag((Player *) em, EM_FLAG_NO_ROOT_MOTION);
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
     * area once the quest's goal is met there. Everything else runs the monster
     * script; the gotos share that call.
     */
    if (em->type == EM_KIND_FATALIS) {
        if (Area_DayId(em->area) == AREA_CASTLE_1) {
            if (Save_TestEventFlag(EVENT_FLAG_FATALIS_CASTLE_SCENE) == 0) {
                if (gScene.eventRunning == 1 && em->stateGroup != EM_GROUP_EVENT) {
                    EmFatalis_ChangeState(em, EM_GROUP_EVENT, FAT_EVENT_FLY_IN, EM_SYNC_LOCAL);
                }
            } else if (Quest_GetOutcome(0) == 1) {
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
        if (Area_DayId(em->area) == AREA_BATTLEFIELD_1 && Save_TestEventFlag(EVENT_FLAG_FATALIS_BATTLEFIELD_SCENE) == 0) {
            if (gScene.eventRunning == 1 && em->stateGroup != EM_GROUP_EVENT) {
                EmFatalis_ChangeState(em, EM_GROUP_EVENT, FAT_EVENT_GET_UP, EM_SYNC_LOCAL);
            }
        } else {
            goto runScript;
        }
    } else if (Area_DayId(em->area) == AREA_TOWER_0x7C && em->type == EM_KIND_FATALIS_C && Save_TestEventFlag(EVENT_FLAG_FATALIS_TOWER_SCENE) == 0) {
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

    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
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
        Em_SpawnLyingPuffs(em, 0x33, pos, 1.6f);
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
            Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
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
            Player_ClearFlag((Player *) em, EM_FLAG_NO_ROOT_MOTION);
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
            Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
        }
        em->lookAtTarget = 0;
        Em_SetMotion(em, FAT_MOT_TAKE_OFF, 0, 0);
        EmFatalis_StartCurves(em, 0);
        work->flyGuard = 0;
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 134.0f, 0U) != 0) {
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
        if (EmMotion_PassedFrameScaled(em, 134.0f, 0U) != 0) {
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
        Player_SetFlag((Player *) em, EM_FLAG_NO_ROOT_MOTION);
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
        if (EmMotion_PassedFrameScaled(em, 102.0f, 0U) != 0) {
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
        if (EmMotion_PassedFrameScaled(em, 98.0f, 0U) != 0) {
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
            if (EmMotion_PassedFrameScaled(em, 80.0f, 0U) != 0 || EmMotion_PassedFrameScaled(em, 140.0f, 0U) != 0 || EmMotion_PassedFrameScaled(em, 200.0f, 0U) != 0 ||
                EmMotion_PassedFrameScaled(em, 260.0f, 0U) != 0 || EmMotion_PassedFrameScaled(em, 320.0f, 0U) != 0) {
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
        if (EmMotion_PassedFrameScaled(em, 30.0f, 0U) != 0) {
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
        if (((s8) work->countdown <= 0) && (EmMotion_PassedFrameScaled(em, 76.0f, 0U) != 0)) {
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
            FatalisAttack_Create(em, 2);
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
        if (EmMotion_PassedFrameScaled(em, 70.0f, 0U) != 0) {
            em->step++;
            em->stance = EM_STANCE_AIR;
            EmFatalis_UpdateCurves(em);
        }
        break;
    case 2:
        if (EmMotion_PassedFrameScaled(em, 94.0f, 0U) != 0) {
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
        if (EmMotion_PassedFrameScaled(em, 30.0f, 0U) != 0) {
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
        if (((s8) work->countdown <= 0) && (EmMotion_PassedFrameScaled(em, 76.0f, 0U) != 0)) {
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
            FatalisAttack_Create(em, 0x1E);
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
            if (EmMotion_PassedFrameScaled(em, 80.0f, 0U) != 0 || EmMotion_PassedFrameScaled(em, 140.0f, 0U) != 0 || EmMotion_PassedFrameScaled(em, 200.0f, 0U) != 0 ||
                EmMotion_PassedFrameScaled(em, 260.0f, 0U) != 0 || EmMotion_PassedFrameScaled(em, 320.0f, 0U) != 0) {
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
            if (EmMotion_PassedFrameScaled(em, 80.0f, 0U) != 0 || EmMotion_PassedFrameScaled(em, 140.0f, 0U) != 0 || EmMotion_PassedFrameScaled(em, 200.0f, 0U) != 0 ||
                EmMotion_PassedFrameScaled(em, 260.0f, 0U) != 0 || EmMotion_PassedFrameScaled(em, 320.0f, 0U) != 0) {
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
            FatalisAttack_Create(em, 0x1E);
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
 * FAT_FLY_LAND_AT_HEIGHT, with an effect (EmEffectAttack kind 0xE) every 10 frames
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
        Player_SetFlag((Player *) em, EM_FLAG_NO_ROOT_MOTION);
        em->timer = 10;
        break;
    case 1:
        if (em->stateVar0 < 8) {
            if (--em->timer <= 0) {
                EmEffectAttack_Create(em, EMFX_FATALIS_BLAST, EmFatalis_SweepSpots[6 + (em->stateRand & 1)][em->stateVar0], 0, 0);
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
                EmEffectAttack_Create(em, EMFX_FATALIS_BLAST, EmFatalis_SweepSpots[6 + (em->stateRand & 1)][em->stateVar0], 0, 0);
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
                EmEffectAttack_Create(em, EMFX_FATALIS_BLAST, EmFatalis_SweepSpots[6 + (em->stateRand & 1)][em->stateVar0], 0, 0);
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
 * (Em_SpawnStatus4Effect each frame once lying) until the timer runs out,
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
        if (EmMotion_PassedFrameScaled(em, 296.0f, 0U) != 0) {
            em->step++;
            Em_SetMotion(em, FAT_MOT_DOWN, 0xC, 0x124);
        }
        break;
    case 2:
        Em_SpawnStatus4Effect(em, 2);
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
        if (EmMotion_PassedFrameScaled(em, 100.0f, 0) != 0) {
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
        if (EmMotion_PassedFrameScaled(em, 134.0f, 0U) != 0) {
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
 * FAT_EVENT_FLY_IN, EM_KIND_FATALIS's scene in AREA_CASTLE_1: from a fixed spot
 * it takes off, climbs to the height 5860, flies to a second point, lands
 * with a camera shake, roars and breathes a fireball, then idles until
 * EVENT_FLAG_FATALIS_CASTLE_SCENE is set.
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
        if (EmMotion_PassedFrameScaled(em, 134.0f, 0U) != 0) {
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
        if (EmMotion_PassedFrameScaled(em, 102.0f, 0U) != 0) {
            EmFatalis_SpawnFireball(em, 0, 0);
            EmFatalis_SpawnFireballEffect(em, 0, 0);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, FAT_MOT_IDLE, 0, 0);
        }
        break;
    case 12:
        if (Save_TestEventFlag(EVENT_FLAG_FATALIS_CASTLE_SCENE) == 1) {
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
        if (EmMotion_PassedFrameScaled(em, 30.0f, 0U) != 0) {
            EmFatalis_SpawnFireball(em, 0, 0);
            EmFatalis_SpawnFireballEffect(em, 0x38E4, 0);
        }
        if (EmMotion_PassedFrameScaled(em, 76.0f, 0U) != 0) {
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
 * FAT_EVENT_GET_UP, EM_KIND_FATALIS_B's scene in AREA_BATTLEFIELD_1: lying at a
 * fixed spot (FAT_MOT_DIE_B from frame 638), it gets up, idles, roars
 * (enraged meanwhile), glides to a second point and lands, then idles until
 * EVENT_FLAG_FATALIS_BATTLEFIELD_SCENE is set.
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
        if (Save_TestEventFlag(EVENT_FLAG_FATALIS_BATTLEFIELD_SCENE) == 1) {
            em->step++;
            EmFatalis_EndState(em);
        }
        break;
    }
}

/*
 * FAT_EVENT_DESCEND, EM_KIND_FATALIS_C's scene in AREA_TOWER_0x7C: glides in high
 * (hidden for its first 150 frames), comes down at a fixed spot, lands,
 * roars enraged with 15 effects (EmEffectAttack kind 0xC), one every 10 frames,
 * then idles until EVENT_FLAG_FATALIS_TOWER_SCENE is set.
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
                EmEffectAttack_CreateDescentBlast(em, EMFX_FATALIS_SWEEP_BLAST, em->stateVar0++, 0, 0);
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
        if (Save_TestEventFlag(EVENT_FLAG_FATALIS_TOWER_SCENE) == 1) {
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
        DustCloud_Create(em, 0x1A, (s16) ((u16) Rand_Next(1) & 1), 1.0f);
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
                MonsterFx_CreateOnEm(em, 0x28, 1.0f);
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
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            FatalisAttack_Create(em, 5);
        }
        if (EmMotion_PassedFrameScaled(em, 54.0f, 0) != 0) {
            FatalisAttack_Create(em, 6);
        }
        if (EmMotion_PassedFrameScaled(em, 130.0f, 0) != 0) {
            FatalisAttack_Create(em, 7);
        }
        if (EmMotion_PassedFrameScaled(em, 116.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0xA, 0x14, 5.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 46.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 6, 0x14, 5.0f);
        }
        if (EmMotion_InFrameRange(em, 0, 34.0f, 52.0f) || EmMotion_InFrameRange(em, 0, 108.0f, 130.0f)) {
            EmFatalis_SpawnRageObject(em);
        }
        break;
    case EM_MOTION_ID(0, FAT_MOT_TURN_LEFT):
        EmFatalis_ShakeOnFrame(em, 0x1A, 2);
        EmFatalis_ShakeOnFrame(em, 0x60, 2);
        if (EmMotion_PassedFrameScaled(em, 4.0f, 0) != 0) {
            FatalisAttack_Create(em, 8);
        }
        if (EmMotion_PassedFrameScaled(em, 56.0f, 0) != 0) {
            FatalisAttack_Create(em, 9);
        }
        if (EmMotion_PassedFrameScaled(em, 60.0f, 0) != 0) {
            FatalisAttack_Create(em, 0x17);
        }
        if (EmMotion_PassedFrameScaled(em, 20.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 6, 7, 1.2f);
        }
        if (EmMotion_PassedFrameScaled(em, 94.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0xA, 7, 1.2f);
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
        if (EmMotion_PassedFrameScaled(em, 4.0f, 0) != 0) {
            FatalisAttack_Create(em, 0xA);
        }
        if (EmMotion_PassedFrameScaled(em, 56.0f, 0) != 0) {
            FatalisAttack_Create(em, 0xB);
        }
        if (EmMotion_PassedFrameScaled(em, 60.0f, 0) != 0) {
            FatalisAttack_Create(em, 0x17);
        }
        if (EmMotion_PassedFrameScaled(em, 20.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0xA, 7, 1.2f);
        }
        if (EmMotion_PassedFrameScaled(em, 94.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 6, 7, 1.2f);
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
        if (EmMotion_PassedFrameScaled(em, 38.0f, 0) != 0) {
            if (em->type == EM_KIND_FATALIS_C) {
                FatalisAttack_Create(em, 0x23);
            } else {
                FatalisAttack_Create(em, 0xC);
            }
        }
        if (EmMotion_PassedFrameScaled(em, 36.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0x1E, 7, 1.2f);
        }
        if (EmMotion_PassedFrameScaled(em, 38.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0x19, 7, 1.2f);
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
        if ((EmMotion_PassedFrameScaled(em, 24.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 28.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 86.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 90.0f, 0) != 0)) {
            PartFx_SpawnForEmScaled(em, 0x19, 3, 2.2f);
        }
        if ((EmMotion_PassedFrameScaled(em, 36.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 40.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 96.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 100.0f, 0) != 0)) {
            PartFx_SpawnForEmScaled(em, 0xA, 3, 2.2f);
        }
        if ((EmMotion_PassedFrameScaled(em, 58.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 62.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 126.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 130.0f, 0) != 0)) {
            PartFx_SpawnForEmScaled(em, 0x1E, 3, 2.2f);
        }
        if ((EmMotion_PassedFrameScaled(em, 66.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 70.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 130.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 134.0f, 0) != 0)) {
            PartFx_SpawnForEmScaled(em, 6, 3, 2.2f);
        }
        if (EmMotion_InFrameRange(em, 0, 12.0f, 40.0f) || EmMotion_InFrameRange(em, 0, 76.0f, 106.0f)) {
            EmFatalis_SpawnRageObject(em);
        }
        break;
    case EM_MOTION_ID(0, FAT_MOT_TURN_SMALL):
        if (EmMotion_PassedFrameScaled(em, 32.0f, 0) != 0) {
            FatalisAttack_Create(em, 0x21);
        }
        if (EmMotion_PassedFrameScaled(em, 114.0f, 0) != 0) {
            FatalisAttack_Create(em, 0x22);
        }
        break;
    case EM_MOTION_ID(0, FAT_MOT_RECOVER):
        EmFatalis_ShakeOnFrame(em, 0x5A, 2);
        if (EmMotion_InFrameRange(em, 0, 20.0f, 70.0f) || EmMotion_InFrameRange(em, 0, 84.0f, 140.0f)) {
            EmFatalis_SpawnRageObject(em);
        }
        break;
    case EM_MOTION_ID(0, FAT_MOT_TAKE_OFF):
        if (EmMotion_PassedFrameScaled(em, 196.0f, 0) || EmMotion_PassedFrameScaled(em, 250.0f, 0) || EmMotion_PassedFrameScaled(em, 316.0f, 0)) {
            FatalisAttack_Create(em, 0x18);
        }
        if (((EmMotion_PassedFrameScaled(em, 38.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 90.0f, 0) != 0)) && (em->posY <= (1000.0f + em->floorY))) {
            DustCloud_Create(em, 1, 0, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 138.0f, 0) || EmMotion_PassedFrameScaled(em, 196.0f, 0) || EmMotion_PassedFrameScaled(em, 256.0f, 0) || EmMotion_PassedFrameScaled(em, 322.0f, 0) || EmMotion_PassedFrameScaled(em, 394.0f, 0)) {
            if (em->posY <= (1000.0f + em->floorY)) {
                DustCloud_Create(em, 1, 0, 1.2f);
            }
        }
        if (EmMotion_InFrameRange(em, 0, 6.0f, 34.0f) || EmMotion_InFrameRange(em, 0, 84.0f, 92.0f) || EmMotion_InFrameRange(em, 0, 124.0f, 130.0f) || EmMotion_InFrameRange(em, 0, 178.0f, 198.0f) || EmMotion_InFrameRange(em, 0, 252.0f, 284.0f) || EmMotion_InFrameRange(em, 0, 312.0f, 328.0f)) {
            EmFatalis_SpawnRageObject(em);
        }
        break;
    case EM_MOTION_ID(0, FAT_MOT_FLY):
        if (EmMotion_PassedFrameScaled(em, 32.0f, 0) != 0) {
            FatalisAttack_Create(em, 0x19);
        }
        if (((EmMotion_PassedFrameScaled(em, 40.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 110.0f, 0) != 0)) && (em->posY <= (1000.0f + em->floorY))) {
            DustCloud_Create(em, 1, 0, 1.2f);
        }
        if (EmMotion_InFrameRange(em, 0, 44.0f, 68.0f) != 0) {
            EmFatalis_SpawnRageObject(em);
        }
        break;
    case EM_MOTION_ID(0, FAT_MOT_DESCEND):
        if ((EmMotion_PassedFrameScaled(em, 122.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 164.0f, 0) != 0)) {
            FatalisAttack_Create(em, 0x1A);
        }
        if (EmMotion_PassedFrameScaled(em, 42.0f, 0) || EmMotion_PassedFrameScaled(em, 102.0f, 0) || EmMotion_PassedFrameScaled(em, 156.0f, 0)) {
            if (em->posY <= (1000.0f + em->floorY)) {
                DustCloud_Create(em, 1, 0, 1.2f);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, FAT_MOT_TOUCH_DOWN):
        EmFatalis_ShakeOnFrame(em, 0x12, 2);
        EmFatalis_ShakeOnFrame(em, 0x28, 2);
        EmFatalis_ShakeOnFrame(em, 0x86, 5);
        if (EmMotion_PassedFrameScaled(em, 26.0f, 0) != 0) {
            FatalisAttack_Create(em, 0xD);
        }
        if (EmMotion_PassedFrameScaled(em, 40.0f, 0) != 0) {
            FatalisAttack_Create(em, 0xE);
        }
        if (EmMotion_PassedFrameScaled(em, 42.0f, 0) != 0) {
            DustCloud_Create(em, 1, 0, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 190.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0x19, 7, 1.2f);
        }
        if (EmMotion_InFrameRange(em, 0, 68.0f, 118.0f) || EmMotion_InFrameRange(em, 0, 160.0f, 252.0f) || EmMotion_InFrameRange(em, 0, 298.0f, 330.0f)) {
            EmFatalis_SpawnRageObject(em);
        }
        break;
    case EM_MOTION_ID(0, FAT_MOT_FIREBALL):
        if (EmMotion_InFrameRange(em, 0, 10.0f, 32.0f) || EmMotion_InFrameRange(em, 0, 54.0f, 72.0f) || EmMotion_InFrameRange(em, 0, 158.0f, 212.0f)) {
            if (!(gScene.frame & 3)) {
                DustCloud_Create(em, 0x1A, (u16) Rand_Next(1) & 1, 1.0f);
            }
        }
        if (EmMotion_PassedFrameScaled(em, 80.0f, 0) != 0) {
            FatalisAttack_Create(em, 0x11);
        }
        break;
    case EM_MOTION_ID(0, FAT_MOT_FIREBALL_B):
        if (EmMotion_InFrameRange(em, 0, 10.0f, 32.0f) || EmMotion_InFrameRange(em, 0, 54.0f, 72.0f) || EmMotion_InFrameRange(em, 0, 158.0f, 212.0f)) {
            if (!(gScene.frame & 3)) {
                DustCloud_Create(em, 0x1A, (u16) Rand_Next(1) & 1, 1.0f);
            }
        }
        if (EmMotion_PassedFrameScaled(em, 80.0f, 0) != 0) {
            FatalisAttack_Create(em, 0x11);
        }
        break;
    case EM_MOTION_ID(0, FAT_MOT_SWEEP):
        EmFatalis_ShakeOnFrame(em, 0x26, 2);
        EmFatalis_ShakeOnFrame(em, 0x56, 2);
        EmFatalis_ShakeOnFrame(em, 0xF4, 2);
        EmFatalis_ShakeOnFrame(em, 0x124, 2);
        if (EmMotion_PassedFrameScaled(em, 80.0f, 0) != 0) {
            FatalisAttack_Create(em, 3);
        }
        if (EmMotion_PassedFrameScaled(em, 70.0f, 0) != 0) {
            FatalisAttack_Create(em, 0x16);
        }
        break;
    case EM_MOTION_ID(0, FAT_MOT_AIR_FIREBALL):
        if ((EmMotion_InFrameRange(em, 0, 98.0f, 130.0f) != 0) && !(gScene.frame & 3)) {
            DustCloud_Create(em, 0x1A, (u16) Rand_Next(1) & 1, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            FatalisAttack_Create(em, 0x12);
        }
        if ((EmMotion_PassedFrameScaled(em, 34.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 112.0f, 0) != 0)) {
            FatalisAttack_Create(em, 0x14);
        }
        break;
    case EM_MOTION_ID(0, 0x12):
        EmFatalis_ShakeOnFrame(em, 0x14, 2);
        EmFatalis_ShakeOnFrame(em, 0x92, 2);
        if (em->type == EM_KIND_FATALIS) {
            if (EmMotion_PassedFrameScaled(em, 48.0f, 0) != 0) {
                FatalisAttack_Create(em, 4);
                break;
            }
        } else {
            if (EmMotion_PassedFrameScaled(em, 38.0f, 0) != 0) {
                FatalisAttack_Create(em, 0x1C);
            }
            if (EmMotion_PassedFrameScaled(em, 34.0f, 0) != 0) {
                FatalisAttack_Create(em, 0x1D);
            }
            if ((em->type == EM_KIND_FATALIS_C) && (EmMotion_PassedFrameScaled(em, 34.0f, 0) != 0)) {
                MonsterFx_CreateOnEmVariant(em, 0x28, 1, 0.4f);
            }
            if (EmMotion_PassedFrameScaled(em, 46.0f, 0) != 0) {
                DustCloud_Create(em, 0xA, 0x8B, 0.3f);
            }
            if (EmMotion_PassedFrameScaled(em, 52.0f, 0) != 0) {
                DustCloud_Create(em, 0xA, 0x8B, 0.3f);
            }
            if (EmMotion_PassedFrameScaled(em, 58.0f, 0) != 0) {
                DustCloud_Create(em, 0xA, 0x8B, 0.3f);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, 0x29):
        EmFatalis_ShakeOnFrame(em, 0x14, 2);
        EmFatalis_ShakeOnFrame(em, 0x92, 2);
        type = em->type;
        if (type == 2) {
            if (EmMotion_PassedFrameScaled(em, 48.0f, 0) != 0) {
                FatalisAttack_Create(em, 4);
                break;
            }
        } else {
            if (type == 0x24) {
                if (EmMotion_PassedFrameScaled(em, 38.0f, 0) != 0) {
                    FatalisAttack_Create(em, 0x1C);
                }
                if (EmMotion_PassedFrameScaled(em, 34.0f, 0) != 0) {
                    FatalisAttack_Create(em, 0x1D);
                }
            } else {
                if (EmMotion_PassedFrameScaled(em, 38.0f, 0) != 0) {
                    FatalisAttack_Create(em, 0x1F);
                }
                if (EmMotion_PassedFrameScaled(em, 34.0f, 0) != 0) {
                    FatalisAttack_Create(em, 0x20);
                }
            }
            if ((em->type == EM_KIND_FATALIS_C) && (EmMotion_PassedFrameScaled(em, 34.0f, 0) != 0)) {
                MonsterFx_CreateOnEmVariant(em, 0x28, 2, 0.4f);
            }
            if (EmMotion_PassedFrameScaled(em, 46.0f, 0) != 0) {
                DustCloud_Create(em, 0xA, 0xB0, 0.3f);
            }
            if (EmMotion_PassedFrameScaled(em, 52.0f, 0) != 0) {
                DustCloud_Create(em, 0xA, 0xB0, 0.3f);
            }
            if (EmMotion_PassedFrameScaled(em, 58.0f, 0) != 0) {
                DustCloud_Create(em, 0xA, 0xB0, 0.3f);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, 0x13):
        EmFatalis_ShakeOnFrame(em, 0x44, 2);
        EmFatalis_ShakeOnFrame(em, 0x118, 2);
        if (EmMotion_PassedFrameScaled(em, 60.0f, 0) != 0) {
            FatalisAttack_Create(em, 1);
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
        if (EmMotion_PassedFrameScaled(em, 64.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0x1E, 7, 1.2f);
        }
        if (EmMotion_PassedFrameScaled(em, 66.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0x19, 7, 1.2f);
        }
        break;
    case EM_MOTION_ID(0, FAT_MOT_FLINCH_TURN_RIGHT):
        EmFatalis_ShakeOnFrame(em, 0x20, 2);
        EmFatalis_ShakeOnFrame(em, 0x3A, 2);
        EmFatalis_ShakeOnFrame(em, 0xAA, 2);
        if (EmMotion_PassedFrameScaled(em, 52.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0x19, 7, 1.2f);
        }
        if (EmMotion_PassedFrameScaled(em, 58.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0x1E, 7, 1.2f);
        }
        if (EmMotion_PassedFrameScaled(em, 162.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0xA, 7, 1.2f);
        }
        break;
    case EM_MOTION_ID(0, FAT_MOT_FLINCH_TURN_LEFT):
        EmFatalis_ShakeOnFrame(em, 0x20, 2);
        EmFatalis_ShakeOnFrame(em, 0x44, 2);
        EmFatalis_ShakeOnFrame(em, 0xA0, 2);
        if (EmMotion_PassedFrameScaled(em, 52.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0x1E, 7, 1.2f);
        }
        if (EmMotion_PassedFrameScaled(em, 58.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0x19, 7, 1.2f);
        }
        if (EmMotion_PassedFrameScaled(em, 162.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 6, 7, 1.2f);
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
        if (EmMotion_PassedFrameScaled(em, 314.0f, 0) != 0) {
            DustCloud_Create(em, 0, 0, 1.2f);
        }
        if (EmMotion_PassedFrameScaled(em, 418.0f, 0) != 0) {
            DustCloud_Create(em, 1, 0, 0.9f);
        }
        break;
    case EM_MOTION_ID(0, FAT_MOT_FALL_LAND):
        EmFatalis_ShakeOnFrame(em, 0xC, 2);
        EmFatalis_ShakeOnFrame(em, 0x20, 2);
        if (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0) {
            DustCloud_Create(em, 0, 1, 1.2f);
        }
        break;
    case EM_MOTION_ID(0, FAT_MOT_DOWN):
        EmFatalis_ShakeOnFrame(em, 0x2E, 5);
        if (EmMotion_PassedFrameScaled(em, 46.0f, 0) || EmMotion_PassedFrameScaled(em, 112.0f, 0) || EmMotion_PassedFrameScaled(em, 320.0f, 0)) {
            DustCloud_Create(em, 1, 0x80, 0.6f);
        }
        break;
    case EM_MOTION_ID(0, FAT_MOT_DOWN_GET_UP):
        EmFatalis_ShakeOnFrame(em, 0xDC, 2);
        if (EmMotion_PassedFrameScaled(em, 216.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0x1E, 7, 1.2f);
            PartFx_SpawnForEmScaled(em, 0x19, 7, 1.2f);
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
        if ((EmMotion_PassedFrameScaled(em, 36.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 122.0f, 0) != 0)) {
            FatalisAttack_Create(em, 0x13);
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
                if (EmMotion_PassedFrameScaled(em, 66.0f, 0) != 0) {
                    FatalisAttack_Create(em, 0x10);
                    ElementBurst_CreateOnEm(em, 5, 3, 1.0f);
                    break;
                }
            }
        }
        break;
    case EM_MOTION_ID(0, FAT_MOT_HOP_FIREBALL):
        if (EmMotion_PassedFrameScaled(em, 66.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0x1F, 7, 8.0f);
        }
        if ((EmMotion_PassedFrameScaled(em, 76.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 252.0f, 0) != 0)) {
            FatalisAttack_Create(em, 0x1B);
        }
        if (EmMotion_InFrameRange(em, 0, 124.0f, 132.0f) || EmMotion_InFrameRange(em, 0, 154.0f, 170.0f)) {
            if (!(gScene.frame & 3)) {
                DustCloud_Create(em, 0x1A, (u16) Rand_Next(1) & 1, 1.0f);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, FAT_MOT_LIE_DOWN):
        EmFatalis_ShakeOnFrame(em, 0xF2, 2);
        if (EmMotion_PassedFrameScaled(em, 242.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0x1E, 7, 1.2f);
        }
        if (EmMotion_PassedFrameScaled(em, 244.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 0x19, 7, 1.2f);
        }
        break;
    case EM_MOTION_ID(0, FAT_MOT_GLIDE_START):
        if (EmMotion_PassedFrameScaled(em, 68.0f, 0) != 0) {
            FatalisAttack_Create(em, 0x1E);
        }
        if ((EmMotion_PassedFrameScaled(em, 34.0f, 0) != 0) && (em->posY <= (1000.0f + em->floorY))) {
            DustCloud_Create(em, 0x13, 0, 3.0f);
        }
        break;
    case EM_MOTION_ID(0, FAT_MOT_LAND_SLIDE):
        if ((EmMotion_InFrameRange(em, 0, 30.0f, 70.0f) != 0) && (em->posY <= (1000.0f + em->floorY)) && !(gScene.frame & 3)) {
            DustCloud_Create(em, 0xA, 0x80, 1.0f);
        }
        if ((EmMotion_PassedFrameScaled(em, 96.0f, 0) != 0) && (em->posY <= (1000.0f + em->floorY))) {
            DustCloud_Create(em, 0xB, 0, 2.0f);
        }
        break;
    case EM_MOTION_ID(0, FAT_MOT_DIVE_RISE):
        if ((EmMotion_PassedFrameScaled(em, 52.0f, 0) != 0) && (em->posY <= (1000.0f + em->floorY))) {
            DustCloud_Create(em, 1, 0, 1.5f);
        }
        break;
    case EM_MOTION_ID(0, FAT_MOT_GLIDE):
        if ((em->posY <= (1000.0f + em->floorY)) && !(gScene.frame & 3)) {
            DustCloud_Create(em, 0xA, 0x80, 0.8f);
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
    if (EmMotion_PassedFrameScaled((Em *) em, (f32) frame, 0U) != 0) {
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
 * (Shadow_Spawn).
 */
void EmFatalis_SpawnEffect01(u8 *em)
{
    Shadow_Spawn((Player *) em, 0);
}

void EmFatalis_TableNop(void)
{
}

/* The file's initialised data, in the game's order */
s32 GmEm_ReturnZero(void); /* gm_sub */
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
    GmEm_ReturnZero, EmFatalis_TableNop, NULL, NULL,
};
