/*
 * Monster family: Kushala Daora, its rusted form, Lunastra and Teostra
 * (EmKushala_*; include/em_kushala.h names their motions and states): elder
 * dragons that walk, hover low (stance 1) and fly, and raise a wind
 * (Kushala Daora) or an aura (Lunastra and Teostra, "Tesca" in the code)
 * that changes their hitzones. EmKushala_Update turns the damage into
 * reactions and runs the state functions through the Run*State
 * dispatchers. EmKushala_ChangeState, the Set*State functions, the wind and
 * the flight curves come earlier in the game's code, before Shen Gaoren's,
 * and have a file of their own. The file starts with the motion sounds
 * (EmKushala_MotionSounds, MotionSoundsTesca). The state functions take the
 * monster and its work area (EmKushalaWork, the family's view of EmWork);
 * Em.step is the step within the current state.
 *
 * Motions are written EM_MOTION_ID(layer, KUSH_MOT_*) where they are
 * compared with a layer's motion id, and by name where they are played;
 * bones (KUSH_BONE_*) and sounds (KUSH_SE_*) by name where the code shows
 * what they are. The other motion, bone and sound numbers stay numbers.
 * Lunastra and Teostra have the same skeleton numbering (their attack hit
 * shapes and sounds use the same bones) but their own sound bank.
 */
#include "common.h"
#include "scene.h"
#include "quest.h"
#include "em.h"
#include "em_kushala.h"
#include "player.h"
#include "stageobj.h"
#include "objattach.h"
#include "area.h"
#include "shadow.h"
#include "capsule.h"
#include "save.h"
#include "sound.h"
#include "cam_shake.h"
#include "elder_attacks.h"
#include "engine_math.h"
#include "gm_em.h"
#include "gm_load.h"
#include "hit_effects.h"
#include "part_effects.h"
#include "shakalaka_teostra_bulldrome.h"
#include "stage_data.h"
#include "hit_spark.h"
#include "vec_math.h"
#include "camera.h"
#include "em_script.h"
#include "hud.h"
/* The functions this file calls */

/* prototypes */
StageObj *MonsterFx_CreateAtYaw(f32 *pos, u16 v, u8 k, f32 scale);
/* unit's own declaration: the last two arguments are declared s32; with the definition's u16 the compiler masks them to 16 bits before the jump to it, which EmKushala_CreateBreathAtBone doesn't do */
void KushalaBreath_CreateAtBone(Em *, s32, u8, u8, s32, s32);
/* unit's own declaration: the kind is declared s32; with the definition's u8 the compiler masks it to a byte before the call, which EmKushala_BlastRing doesn't do */
void TeostraBlastRing_Create(Em *, s32);
/* unit's own declaration: the arguments are declared s32; with the definition's s16 the compiler narrows them before the call, which EmKushala_Rest doesn't do */
s32 Em_RegenHp(Em *em, s32 n, s32 max, s32 interval);
void AlertMark_Create(void *arg, s16 v, u8 k);
/* unit's own declaration: the arguments are declared s32; with the definition's s16 the compiler narrows them before the call, which EmKushala_SpawnK14NearGround doesn't do */
void DustCloud_Create(Em *em, s32, s32, f32);
/* unit's own declaration: the arguments are declared s32; with the definition's u8 and s16 the compiler narrows them before the call, which EmKushala_Stun doesn't do */
void StunStars_CreateEm(Em *, s32, s32, s32, f32, f32);
StageObj *MonsterFx_CreateOnEm(Em *em, u8 k, f32 scale);
/* unit's own declaration: the third argument is declared s16 and the fourth u16, as the callers pass them: with the definition's u16 and s16 the yaw 61896 of EmKushala_BreathAttack would be passed sign-extended */
void MonsterFx_CreateOnEmAngled(Em *em, s8, s16, u16, f32);
s32 EmMotion_PassedFrameScaled(void *p, f32 frame, s32 layer);
/* the calls that mask the value again (u8 or & 0xFF) cast it to s32 first: MWCC keeps the game's andi that way */
/* unit's own declaration: the bone is declared s32; with the definition's s16 the compiler narrows it before the call, which the ground-effect functions don't do */
void Em_BonePos(void *obj, s32 a, f32 *out);
extern EmFlyParam EmKushala_FlyParam;
extern EmFlyParam EmKushala_FlyParam2;
extern EmTurnParams EmKushala_TurnParams[1];
void Em_SpawnLyingPuffs(Em *em, s32 bone, f32 *pos, f32 scale);
EmBone *Em_Bone(void *obj, s16 bone);
extern u16 EmKushala_IdleTbl[];
u16 Area_IdForCurrentTime(u16);
typedef struct { f32 x, y; } EmKPair;
extern s32 EmKushala_BreathTimers[][2];
extern s32 EmKushala_BreathCounts[][2];
void FallingDebris_Create(Em *em, u8 k);
struct EmIdEntry;
f32 *Em_AreaEntryValue(Em *em, struct EmIdEntry *entry);
s32 Em_PartBroken(Em *em, s32 arg1);
void EmTail_Create(Em *);
/* end of prototypes */

/*
 * Kushala Daora's and the rusted form's motion sounds (EmKushala_MotionEffects
 * calls it each frame for those kinds): per motion on layer 0
 * (work->fxMotion), the sounds to play at bones on given frames.
 */
void EmKushala_MotionSounds(void *em, EmWork *work)
{
    s16 motion;

    motion = work->fxMotion;
    switch (motion) {
    case EM_MOTION_ID(0, KUSH_MOT_WALK):
        Em_PlaySeAtBoneOnFrame((Em *) em, 36, 1, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 78, 1, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame((Em *) em, 132, 1, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 178, 1, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame((Em *) em, 62, 4, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 98, 4, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame((Em *) em, 150, 4, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 196, 4, KUSH_BONE_FOOT_D);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_TURN_LEFT):
        Em_PlaySeAtBoneOnFrame((Em *) em, 12, 0, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame((Em *) em, 40, 0, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 66, 1, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame((Em *) em, 22, 3, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame((Em *) em, 36, 3, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 64, 4, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame((Em *) em, 72, 4, KUSH_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_TURN_RIGHT):
        Em_PlaySeAtBoneOnFrame((Em *) em, 12, 0, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 40, 0, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame((Em *) em, 66, 1, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 22, 3, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 36, 3, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame((Em *) em, 64, 4, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 72, 4, KUSH_BONE_FOOT_D);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_TURN_SMALL):
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 22, KUSH_BONE_BODY);
        Em_PlaySeAtBoneOnFrame((Em *) em, 22, 0, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 38, 0, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame((Em *) em, 34, 3, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 48, 3, KUSH_BONE_FOOT_D);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_TURN_BIG_RIGHT):
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 50, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 21, KUSH_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame((Em *) em, 44, 21, 17);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 3, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 40, 2, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame((Em *) em, 44, 0, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 38, 5, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 42, 3, KUSH_BONE_FOOT_D);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_TURN_BIG_LEFT):
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 50, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 21, KUSH_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame((Em *) em, 44, 21, 17);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 3, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame((Em *) em, 40, 2, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 44, 0, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame((Em *) em, 38, 5, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame((Em *) em, 42, 3, KUSH_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_TAKE_OFF):
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 50, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 38, 39, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 44, 20, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 20, 0, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 38, 0, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame((Em *) em, 14, 4, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame((Em *) em, 26, 3, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 32, 15, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 36, 15, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 40, 22, KUSH_BONE_BODY);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_HOVER):
        Em_PlaySeAtBoneOnFrame((Em *) em, 2, 11, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 11, KUSH_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_FLY_FAST):
    case EM_MOTION_ID(0, KUSH_MOT_BANK_A):
    case EM_MOTION_ID(0, KUSH_MOT_BANK_B):
        Em_PlaySeAtBoneOnFrame((Em *) em, 13, 26, KUSH_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame((Em *) em, 18, 25, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 23, 25, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 44, 25, KUSH_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame((Em *) em, 58, 25, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 65, 25, KUSH_BONE_WING_A);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_DASH_START):
        Em_PlaySeTypeAtBoneOnFrame((Em *) em, 40, 76, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 2, 15, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 15, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 50, 28, KUSH_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame((Em *) em, 52, 29, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 58, 29, KUSH_BONE_WING_A);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_DASH_END):
        Em_PlaySeAtBoneOnFrame((Em *) em, 12, 13, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 16, 13, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 56, 11, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 60, 11, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 44, 21, 17);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_IDLE_ACTION_B):
        Em_PlaySeAtBoneOnFrame((Em *) em, 40, 14, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 44, 14, KUSH_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_FLY_CLIMB):
        Em_PlaySeAtBoneOnFrame((Em *) em, 38, 14, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 42, 14, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 2, 26, KUSH_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 25, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 25, KUSH_BONE_WING_A);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_LAND):
        Em_PlaySeAtBoneOnFrame((Em *) em, 60, 50, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 74, 7, KUSH_BONE_BODY);
        Em_PlaySeAtBoneOnFrame((Em *) em, 2, 15, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 15, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 54, 13, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 58, 13, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 70, 0, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 66, 2, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame((Em *) em, 72, 5, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame((Em *) em, 68, 4, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 124, 1, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 142, 1, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame((Em *) em, 146, 5, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame((Em *) em, 138, 4, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 174, 1, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 196, 0, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame((Em *) em, 194, 3, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame((Em *) em, 184, 4, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 76, 22, KUSH_BONE_BODY);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_HOVER_TURN):
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 49, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 20, KUSH_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 13, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 13, KUSH_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_SIDE_DASH_A):
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 50, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 20, KUSH_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 11, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 11, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 13, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 13, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 22, KUSH_BONE_BODY);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_SIDE_DASH_B):
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 50, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 20, KUSH_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 11, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 11, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 13, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 13, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 22, KUSH_BONE_BODY);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_RUN_START):
        Em_PlaySeAtBoneOnFrame((Em *) em, 24, 38, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 10, 2, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 32, 2, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame((Em *) em, 50, 2, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 62, 2, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame((Em *) em, 22, 5, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 48, 5, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame((Em *) em, 56, 5, KUSH_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_RUN_B):
        Em_PlaySeAtBoneOnFrame((Em *) em, 16, 40, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 26, 2, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 62, 2, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame((Em *) em, 32, 5, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame((Em *) em, 2, 21, KUSH_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_RUN_C):
        Em_PlaySeAtBoneOnFrame((Em *) em, 20, 2, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 28, 2, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame((Em *) em, 34, 5, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame((Em *) em, 2, 21, KUSH_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_RUN_STOP):
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 49, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 10, 0, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 26, 0, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame((Em *) em, 40, 0, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 30, 3, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame((Em *) em, 44, 4, KUSH_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_REAR_UP):
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 49, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 40, 35, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 16, 0, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame((Em *) em, 30, 1, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 20, 4, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame((Em *) em, 40, 15, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 44, 15, KUSH_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_STOMP):
        Em_PlaySeAtBoneOnFrame((Em *) em, 28, 7, KUSH_BONE_BODY);
        Em_PlaySeAtBoneOnFrame((Em *) em, 26, 0, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame((Em *) em, 30, 1, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 22, 5, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame((Em *) em, 24, 3, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 28, 12, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 32, 12, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 36, 22, KUSH_BONE_BODY);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_LIFT_TO_HOVER):
        Em_PlaySeAtBoneOnFrame((Em *) em, 14, 13, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 20, 13, KUSH_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_WING_FLAP):
        Em_PlaySeAtBoneOnFrame((Em *) em, 36, 0, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame((Em *) em, 42, 1, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 24, 5, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame((Em *) em, 36, 3, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 22, 14, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 26, 14, KUSH_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_ALERT):
        Em_PlaySeTypeAtBoneOnFrame((Em *) em, 10, 71, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 20, 2, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 112, 0, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 20, 4, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 14, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 10, 14, KUSH_BONE_WING_A);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_ROAR):
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 32, 38);
        Em_PlaySeTypeAtBoneInFrames((Em *) em, 60, 190, 74, 38, 7, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 15, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 10, 15, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 96, 14, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 100, 14, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 188, 8, KUSH_BONE_BODY);
        Em_PlaySeAtBoneOnFrame((Em *) em, 184, 9, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame((Em *) em, 192, 2, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 242, 1, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 248, 1, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame((Em *) em, 18, 4, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame((Em *) em, 22, 3, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 246, 3, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 186, 22, KUSH_BONE_BODY);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_IDLE_ACTION_A):
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 49, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 138, 50, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 22, KUSH_BONE_BODY);
        Em_PlaySeAtBoneOnFrame((Em *) em, 18, 2, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame((Em *) em, 22, 1, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 158, 1, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame((Em *) em, 202, 1, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 16, 5, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame((Em *) em, 30, 4, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 180, 3, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame((Em *) em, 132, 4, KUSH_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_EAT_START):
        Em_PlaySeAtBoneOnFrame((Em *) em, 54, 58, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 100, 57, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 26, 50, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 20, 0, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 20, 4, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 26, 4, KUSH_BONE_FOOT_D);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_EAT):
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 59, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 20, 4, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame((Em *) em, 26, 4, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 40, 1, KUSH_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_LIMP):
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 47, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 84, 47, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 22, 0, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 64, 1, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame((Em *) em, 96, 0, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 128, 1, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame((Em *) em, 54, 4, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 88, 3, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame((Em *) em, 22, 18, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 16, 22, KUSH_BONE_BODY);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_REST):
        Em_PlaySeTypeAtBoneOnFrame((Em *) em, 4, 86, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 224, 59, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 32, 1, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame((Em *) em, 106, 16, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 30, 3, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 70, 4, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 124, 14, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 128, 14, KUSH_BONE_WING_A);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_REST_END):
        Em_PlaySeTypeAtBoneOnFrame((Em *) em, 4, 99, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 70, 50, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 74, 1, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 110, 1, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame((Em *) em, 132, 1, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 98, 3, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 128, 4, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame((Em *) em, 48, 14, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 52, 14, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 108, 23, KUSH_BONE_BODY);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_STRIKE_BREATH):
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 32, 38);
        Em_PlaySeTypeAtBoneOnFrame((Em *) em, 64, 76, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 74, KUSH_SE_BREATH, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 30, 14, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 34, 14, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 64, 0, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 70, 2, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame((Em *) em, 138, 0, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 156, 0, KUSH_BONE_FOOT_C);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_BREATH_WINDUP):
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 32, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 2, 12, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 12, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 48, 11, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 52, 11, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 108, 11, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 112, 11, KUSH_BONE_WING_A);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_BREATH_STREAM):
        Em_PlaySeTypeAtBoneInFrames((Em *) em, 22, 284, 73, 38, 7, 0);
        Em_PlaySeAtBoneOnFrame((Em *) em, 2, 11, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 11, KUSH_BONE_WING_A);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_BREATH):
        Em_PlaySeTypeAtBoneOnFrame((Em *) em, 22, 76, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 18, KUSH_SE_BREATH, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 14, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 10, 14, KUSH_BONE_WING_A);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_BREATH_STREAM_END):
        Em_PlaySeAtBoneOnFrame((Em *) em, 18, 14, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 22, 14, KUSH_BONE_WING_A);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_BREATH_FINISH):
        Em_PlaySeAtBoneOnFrame((Em *) em, 20, 14, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 24, 14, KUSH_BONE_WING_A);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_LUNGE):
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 37, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 54, 35, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 2, 14, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 14, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 42, 11, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 46, 11, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 58, 60, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame((Em *) em, 62, 9, KUSH_BONE_FOOT_C);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_HOVER_ACTION_A):
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 38, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 50, 39, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 2, 14, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 14, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 42, 15, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 46, 15, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 48, 21, 17);
        Em_PlaySeAtBoneOnFrame((Em *) em, 56, 9, 17);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_HOVER_ACTION_B):
        Em_PlaySeTypeAtBoneOnFrame((Em *) em, 4, 71, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 50, 35, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 2, 10, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 10, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 42, 15, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 46, 15, KUSH_BONE_WING_A);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_TURN_SWEEP_A):
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 49, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 40, 39, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 22, 0, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 126, 0, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 48, 21, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame((Em *) em, 118, 2, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame((Em *) em, 18, 3, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame((Em *) em, 60, 5, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame((Em *) em, 112, 3, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame((Em *) em, 70, 5, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 128, 3, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 72, 21, 17);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_TURN_SWEEP_B):
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 49, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 40, 39, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 22, 0, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame((Em *) em, 126, 9, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame((Em *) em, 44, 21, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 64, 21, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 118, 2, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 18, 3, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 60, 5, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 112, 3, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 70, 5, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame((Em *) em, 128, 3, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame((Em *) em, 74, 21, 17);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_HOVER_SWEEP_A):
    case EM_MOTION_ID(0, KUSH_MOT_HOVER_SWEEP_B):
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 38, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 44, 40, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 36, 20, KUSH_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame((Em *) em, 2, 13, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 13, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 42, 15, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 46, 15, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 128, 12, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 132, 12, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 56, 22, KUSH_BONE_BODY);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_FLINCH):
        Em_PlaySeTypeAtBoneOnFrame((Em *) em, 4, 83, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 56, 35, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 22, KUSH_BONE_BODY);
        Em_PlaySeAtBoneOnFrame((Em *) em, 14, 0, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame((Em *) em, 22, 1, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 20, 3, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 88, 4, KUSH_BONE_FOOT_D);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_TOPPLE_RECOVER):
        Em_PlaySeTypeAtBoneOnFrame((Em *) em, 4, 84, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 28, 7, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 14, 2, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 98, 0, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 20, 2, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame((Em *) em, 92, 0, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame((Em *) em, 82, 3, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 104, 3, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame((Em *) em, 22, 14, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 26, 14, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 28, 22, KUSH_BONE_BODY);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_FLINCH_HOVER):
        Em_PlaySeTypeAtBoneOnFrame((Em *) em, 4, 83, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 14, 13, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 18, 13, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 58, 11, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 62, 11, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 110, 11, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 114, 11, KUSH_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_FALL):
        Em_PlaySeTypeAtBoneOnFrame((Em *) em, 6, 84, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 12, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 10, 12, KUSH_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_FALL_LAND):
        Em_PlaySeTypeAtBoneOnFrame((Em *) em, 6, 83, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 9, KUSH_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame((Em *) em, 24, 7, KUSH_BONE_BODY);
        Em_PlaySeAtBoneOnFrame((Em *) em, 26, 15, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 20, 23, KUSH_BONE_BODY);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_DOWN):
        Em_PlaySeTypeAtBoneOnFrame((Em *) em, 24, 87, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 38, 16, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 26, 23, 38);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_DOWN_GET_UP):
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 22, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 2, 18, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 22, 0, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 90, 0, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 58, 2, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame((Em *) em, 86, 3, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame((Em *) em, 98, 3, KUSH_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_DIE):
        Em_PlaySeTypeAtBoneOnFrame((Em *) em, 4, 85, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 218, 47, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 282, 46, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 366, 44, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 96, 23, KUSH_BONE_BODY);
        Em_PlaySeAtBoneOnFrame((Em *) em, 192, 22, KUSH_BONE_BODY);
        Em_PlaySeAtBoneOnFrame((Em *) em, 282, 23, KUSH_BONE_BODY);
        Em_PlaySeAtBoneOnFrame((Em *) em, 30, 0, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame((Em *) em, 44, 1, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 12, 3, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame((Em *) em, 28, 4, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 400, 16, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 412, 7, KUSH_BONE_BODY);
        Em_PlaySeAtBoneOnFrame((Em *) em, 438, 7, KUSH_BONE_BREATH);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_STATUS4):
        Em_PlaySeTypeAtBoneOnFrame((Em *) em, 12, 88, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 12, 22, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 26, 18, KUSH_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_SHAKE_OFF):
        Em_PlaySeTypeAtBoneOnFrame((Em *) em, 30, 79, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 30, 22, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 22, 13, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 26, 13, KUSH_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_COLLAPSE):
        Em_PlaySeTypeAtBoneOnFrame((Em *) em, 4, 86, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 70, 23, KUSH_BONE_BODY);
        Em_PlaySeAtBoneOnFrame((Em *) em, 266, 59, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 20, 0, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 156, 18, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 58, 18, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 160, 16, KUSH_BONE_BODY);
        Em_PlaySeAtBoneOnFrame((Em *) em, 184, 7, KUSH_BONE_BODY);
        Em_PlaySeAtBoneOnFrame((Em *) em, 48, 14, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 52, 14, KUSH_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_TOPPLE):
        Em_PlaySeTypeAtBoneOnFrame((Em *) em, 4, 85, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 46, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 24, 23, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 16, 13, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 20, 13, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 64, 12, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 68, 12, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 136, 14, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 88, 3, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 110, 18, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame((Em *) em, 134, 7, KUSH_BONE_BODY);
        Em_PlaySeAtBoneOnFrame((Em *) em, 150, 5, KUSH_BONE_BREATH);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_PART_BREAK):
        Em_PlaySeTypeAtBoneOnFrame((Em *) em, 4, 83, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 10, 22, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 56, 7, KUSH_BONE_BODY);
        Em_PlaySeAtBoneOnFrame((Em *) em, 76, 5, KUSH_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 13, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 12, 13, KUSH_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_PART_BREAK_TURN):
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 50, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 12, 23, 38);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 14, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 86, 14, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 90, 14, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 122, 0, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame((Em *) em, 138, 0, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame((Em *) em, 84, 4, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame((Em *) em, 128, 4, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame((Em *) em, 94, 4, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame((Em *) em, 138, 0, KUSH_BONE_FOOT_B);
        
    case EM_MOTION_ID(0, KUSH_MOT_IDLE):
    default:
        break;
    }
}

/*
 * EmKushala_MotionSounds for Lunastra and Teostra: the same motions and
 * bones, with the sounds of their own bank.
 */
void EmKushala_MotionSoundsTesca(Em *em, EmWork *work)
{
    s16 motion;

    motion = work->fxMotion;
    switch (motion) {
    case EM_MOTION_ID(0, KUSH_MOT_WALK):
        Em_PlaySeAtBoneOnFrame(em, 36, 1, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 78, 1, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 132, 1, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 178, 1, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 62, 4, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 98, 4, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 150, 4, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 196, 4, KUSH_BONE_FOOT_D);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_TURN_LEFT):
        Em_PlaySeAtBoneOnFrame(em, 8, 50, 38);
        Em_PlaySeAtBoneOnFrame(em, 12, 0, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 40, 0, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 66, 1, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 22, 3, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 36, 3, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 64, 4, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 72, 4, KUSH_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_TURN_RIGHT):
        Em_PlaySeAtBoneOnFrame(em, 8, 50, 38);
        Em_PlaySeAtBoneOnFrame(em, 12, 0, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 40, 0, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 66, 1, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 22, 3, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 36, 3, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 64, 4, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 72, 4, KUSH_BONE_FOOT_D);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_TURN_SMALL):
        Em_PlaySeAtBoneOnFrame(em, 4, 60, 38);
        Em_PlaySeAtBoneOnFrame(em, 22, 0, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 38, 0, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 34, 3, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 48, 3, KUSH_BONE_FOOT_D);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_TURN_BIG_RIGHT):
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(60, 2, 60, 2), 38);
        Em_PlaySeAtBoneOnFrame(em, 56, 1, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 54, 2, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 20, 3, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 48, 4, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 50, 4, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 80, 3, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 50, 14, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 54, 14, KUSH_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_TURN_BIG_LEFT):
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(60, 2, 60, 2), 38);
        Em_PlaySeAtBoneOnFrame(em, 56, 1, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 54, 2, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 20, 3, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 48, 4, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 50, 4, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 80, 3, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 50, 14, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 54, 14, KUSH_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_TAKE_OFF):
        Em_PlaySeAtBoneOnFrame(em, 4, 60, 38);
        Em_PlaySeTypeAtBoneOnFrame(em, 38, 91, 38);
        Em_PlaySeAtBoneOnFrame(em, 44, 20, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 20, 0, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 38, 0, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 14, 4, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 26, 3, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 32, 15, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 36, 15, KUSH_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_HOVER):
        Em_PlaySeAtBoneOnFrame(em, 2, 11, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 6, 11, KUSH_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_FLY_FAST):
    case EM_MOTION_ID(0, KUSH_MOT_BANK_A):
    case EM_MOTION_ID(0, KUSH_MOT_BANK_B):
        Em_PlaySeAtBoneOnFrame(em, 13, 26, KUSH_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame(em, 18, 25, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 23, 25, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 44, 25, KUSH_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame(em, 58, 25, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 65, 25, KUSH_BONE_WING_A);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_DASH_START):
        Em_PlaySeTypeAtBoneOnFrame(em, 40, 75, 38);
        Em_PlaySeAtBoneOnFrame(em, 2, 15, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 6, 15, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 50, 28, KUSH_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame(em, 52, 29, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 58, 29, KUSH_BONE_WING_A);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_DASH_END):
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 81, 38);
        Em_PlaySeAtBoneOnFrame(em, 12, 13, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 16, 13, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 56, 11, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 60, 11, KUSH_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_IDLE_ACTION_B):
        Em_PlaySeAtBoneOnFrame(em, 28, 49, 38);
        Em_PlaySeAtBoneOnFrame(em, 48, 62, 38);
        Em_PlaySeAtBoneOnFrame(em, 40, 14, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 44, 14, KUSH_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_FLY_CLIMB):
        Em_PlaySeAtBoneOnFrame(em, 38, 14, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 42, 14, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 2, 26, KUSH_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame(em, 6, 25, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 8, 25, KUSH_BONE_WING_A);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_LAND):
        Em_PlaySeAtBoneOnFrame(em, 74, 7, KUSH_BONE_BODY);
        Em_PlaySeAtBoneOnFrame(em, 2, 15, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 6, 15, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 54, 13, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 58, 13, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 70, 0, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 66, 2, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 72, 5, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 68, 4, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 124, 1, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 142, 1, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 146, 5, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 138, 4, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 174, 1, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 196, 0, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 194, 3, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 184, 4, KUSH_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_HOVER_TURN):
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 81, 38);
        Em_PlaySeAtBoneOnFrame(em, 8, 20, KUSH_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame(em, 4, 13, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 8, 13, KUSH_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_RUN_START):
        Em_PlaySeAtBoneOnFrame(em, 10, 2, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 32, 2, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 50, 2, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 62, 2, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 22, 5, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 48, 5, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 56, 5, KUSH_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_RUN_B):
        Em_PlaySeAtBoneOnFrame(em, 26, 2, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 62, 2, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 32, 5, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 2, 21, KUSH_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_RUN_C):
        Em_PlaySeAtBoneOnFrame(em, 20, 2, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 28, 2, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 34, 5, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 2, 21, KUSH_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_RUN_STOP):
        Em_PlaySeAtBoneOnFrame(em, 10, 0, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 26, 0, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 40, 0, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 30, 3, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 44, 4, KUSH_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_REAR_UP):
        Em_PlaySeAtBoneOnFrame(em, 4, 54, 38);
        Em_PlaySeAtBoneOnFrame(em, 16, 0, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 30, 1, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 20, 4, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 40, 15, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 44, 15, KUSH_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_STOMP):
        Em_PlaySeAtBoneOnFrame(em, 28, 7, KUSH_BONE_BODY);
        Em_PlaySeAtBoneOnFrame(em, 26, 0, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 30, 1, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 22, 5, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 24, 3, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 28, 12, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 32, 12, KUSH_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_LIFT_TO_HOVER):
        Em_PlaySeAtBoneOnFrame(em, 14, 13, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 20, 13, KUSH_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_WING_FLAP):
        Em_PlaySeAtBoneOnFrame(em, 32, 61, 38);
        Em_PlaySeAtBoneOnFrame(em, 36, 0, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 42, 1, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 24, 4, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 36, 3, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 22, 14, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 26, 14, KUSH_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_ALERT):
        Em_PlaySeAtBoneOnFrame(em, 10, 51, 38);
        Em_PlaySeAtBoneOnFrame(em, 100, 50, 38);
        Em_PlaySeAtBoneOnFrame(em, 20, 2, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 112, 0, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 20, 4, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 4, 14, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 10, 14, KUSH_BONE_WING_A);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_ROAR):
        Em_PlaySeAtBoneOnFrame(em, 4, 37, 38);
        Em_PlaySeTypeAtBoneInFrames(em, 56, 170, 74, 38, 7, 0);
        Em_PlaySeAtBoneOnFrame(em, 6, 15, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 10, 15, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 96, 14, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 100, 14, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 188, 8, KUSH_BONE_BODY);
        Em_PlaySeAtBoneOnFrame(em, 184, 2, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 192, 0, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 242, 1, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 248, 1, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 18, 4, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 22, 3, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 246, 3, KUSH_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_IDLE_ACTION_A):
        Em_PlaySeAtBoneOnFrame(em, 18, 41, 38);
        Em_PlaySeAtBoneOnFrame(em, 34, 62, 38);
        Em_PlaySeAtBoneOnFrame(em, 120, 54, KUSH_BONE_BODY);
        Em_PlaySeAtBoneOnFrame(em, 18, 2, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 22, 1, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 158, 1, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 202, 1, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 16, 5, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 30, 4, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 180, 3, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 132, 4, KUSH_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_EAT_START):
        Em_PlaySeAtBoneOnFrame(em, 4, 60, 38);
        Em_PlaySeAtBoneOnFrame(em, 50, 57, 38);
        Em_PlaySeAtBoneOnFrame(em, 100, 58, 38);
        Em_PlaySeAtBoneOnFrame(em, 130, 59, 38);
        Em_PlaySeAtBoneOnFrame(em, 168, 35, 38);
        Em_PlaySeAtBoneOnFrame(em, 20, 0, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 20, 4, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 26, 4, KUSH_BONE_FOOT_D);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_EAT):
        Em_PlaySeAtBoneOnFrame(em, 4, 50, 38);
        Em_PlaySeAtBoneOnFrame(em, 20, 4, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 26, 4, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 40, 1, KUSH_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_LIMP):
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 85, 38);
        Em_PlaySeTypeAtBoneOnFrame(em, 84, 85, 38);
        Em_PlaySeAtBoneOnFrame(em, 18, 18, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 72, 18, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 128, 18, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 22, 0, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 64, 1, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 96, 0, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 128, 1, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 54, 4, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 88, 3, KUSH_BONE_FOOT_D);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_REST):
        Em_PlaySeAtBoneOnFrame(em, 80, 38, 38);
        Em_PlaySeAtBoneOnFrame(em, 224, 54, 38);
        if (em->stateGroup == 6) {
            if (em->state != 7) {
                goto block_49;
            }
        default:
            break;
        }
block_49:
        Em_PlaySeAtBoneOnFrame(em, 32, 1, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 106, 16, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 30, 3, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 70, 4, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 124, 14, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 128, 14, KUSH_BONE_WING_A);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_REST_END):
        Em_PlaySeAtBoneOnFrame(em, 4, 50, 38);
        Em_PlaySeAtBoneOnFrame(em, 68, 59, 38);
        Em_PlaySeAtBoneOnFrame(em, 88, 62, 38);
        Em_PlaySeAtBoneOnFrame(em, 74, 1, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 110, 1, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 132, 1, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 98, 3, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 128, 4, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 48, 14, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 52, 14, KUSH_BONE_WING_A);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_TESCA_ACTION_C):
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 91, 38);
        Em_PlaySeAtBoneOnFrame(em, 6, 0, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 12, 0, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 8, 3, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 22, 20, KUSH_BONE_BODY);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_TESCA_ACTION_C_END):
        Em_PlaySeTypeAtBoneOnFrame(em, 10, 81, 38);
        Em_PlaySeAtBoneOnFrame(em, 4, 21, KUSH_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame(em, 14, 9, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 18, 7, KUSH_BONE_BODY);
        Em_PlaySeAtBoneOnFrame(em, 16, 2, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 70, 1, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 18, 4, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 20, 3, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 60, 3, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 70, 4, KUSH_BONE_FOOT_D);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_TESCA_ACTION_B):
        Em_PlaySeTypeAtBoneOnFrame(em, 10, 79, 38);
        Em_PlaySeAtBoneOnFrame(em, 110, 49, 38);
        Em_PlaySeAtBoneOnFrame(em, 6, 0, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 36, 21, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 126, 1, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 132, 0, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 28, 14, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 32, 14, KUSH_BONE_WING_A);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_TESCA_ACTION_A):
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 77, 38);
        Em_PlaySeTypeAtBoneOnFrame(em, 184, 94, 38);
        Em_PlaySeAtBoneOnFrame(em, 120, 21, 17);
        Em_PlaySeAtBoneOnFrame(em, 152, 21, 17);
        Em_PlaySeAtBoneOnFrame(em, 18, 1, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 38, 0, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 212, 1, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 228, 0, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 26, 4, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 40, 3, KUSH_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_BREATH_CHAIN):
        Em_PlaySeAtBoneOnFrame(em, 4, 60, 38);
        Em_PlaySeTypeAtBoneOnFrame(em, 50, 72, 38);
        Em_PlaySeTypeAtBoneOnFrame(em, 98, 73, 38);
        Em_PlaySeAtBoneOnFrame(em, 108, 65, 38);
        Em_PlaySeAtBoneOnFrame(em, 54, 18, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 100, 2, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 104, 1, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 26, 3, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 60, 3, KUSH_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_BREATH_CHAIN_END):
        Em_PlaySeTypeAtBoneOnFrame(em, 6, 81, 38);
        Em_PlaySeAtBoneOnFrame(em, 62, 51, 38);
        Em_PlaySeAtBoneOnFrame(em, 18, 14, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 22, 14, KUSH_BONE_WING_A);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_BREATH_WINDUP):
        Em_PlaySeAtBoneOnFrame(em, 4, 32, 38);
        Em_PlaySeAtBoneOnFrame(em, 2, 12, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 6, 12, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 48, 11, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 52, 11, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 108, 11, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 112, 11, KUSH_BONE_WING_A);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_BREATH_STREAM):
        Em_PlaySeTypeAtBoneInFrames(em, 22, 284, 73, 38, 7, 0);
        Em_PlaySeAtBoneOnFrame(em, 22, 65, 38);
        Em_PlaySeAtBoneOnFrame(em, 2, 11, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 6, 11, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 76, 11, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 80, 11, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 140, 11, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 144, 11, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 202, 11, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 206, 11, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 268, 11, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 272, 11, KUSH_BONE_WING_A);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_BREATH_STREAM_END):
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 91, 38);
        Em_PlaySeAtBoneOnFrame(em, 18, 14, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 22, 14, KUSH_BONE_WING_A);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_HOVER_ACTION_A):
        Em_PlaySeAtBoneOnFrame(em, 4, 54, 38);
        Em_PlaySeAtBoneOnFrame(em, 46, 41, 38);
        Em_PlaySeAtBoneOnFrame(em, 2, 14, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 6, 14, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 42, 15, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 46, 15, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 46, 21, 17);
        Em_PlaySeAtBoneOnFrame(em, 56, 9, 17);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_TURN_SWEEP_A):
        Em_PlaySeAtBoneOnFrame(em, 4, 49, 38);
        Em_PlaySeAtBoneOnFrame(em, 40, 39, 38);
        Em_PlaySeAtBoneOnFrame(em, 90, 60, 38);
        Em_PlaySeAtBoneOnFrame(em, 22, 0, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 126, 0, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 48, 21, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 118, 2, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 18, 3, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 60, 5, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 112, 3, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 70, 5, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 128, 3, KUSH_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_TURN_SWEEP_B):
        Em_PlaySeAtBoneOnFrame(em, 4, 49, 38);
        Em_PlaySeAtBoneOnFrame(em, 40, 39, 38);
        Em_PlaySeAtBoneOnFrame(em, 90, 60, 38);
        Em_PlaySeAtBoneOnFrame(em, 22, 0, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 126, 0, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 48, 21, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 118, 2, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 18, 3, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 60, 5, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 112, 3, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 70, 5, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 128, 3, KUSH_BONE_FOOT_D);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_BLAST_CHARGE):
        Em_PlaySeAtBoneOnFrame(em, 2, 32, 38);
        Em_PlaySeTypeAtBoneOnFrame(em, 2, 77, 38);
        Em_PlaySeAtBoneOnFrame(em, 80, 62, 38);
        Em_PlaySeAtBoneOnFrame(em, 60, 50, 38);
        Em_PlaySeAtBoneOnFrame(em, 4, 11, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 8, 11, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 58, 12, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 62, 12, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 100, 12, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 104, 12, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 58, 36, KUSH_BONE_BODY);
        Em_PlaySeAtBoneVolOnFrame(em, 62, Sound_RandomSe(67, 1, 68, 1), KUSH_BONE_WING_A, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 70, Sound_RandomSe(69, 2, 67, 1), KUSH_BONE_WING_A, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 74, Sound_RandomSe(68, 4, 69, 2), KUSH_BONE_WING_A, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 78, Sound_RandomSe(67, 2, 68, 2), KUSH_BONE_WING_A, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 88, Sound_RandomSe(69, 2, 67, 1), KUSH_BONE_WING_A, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 94, Sound_RandomSe(67, 4, 68, 1), KUSH_BONE_WING_A, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 108, Sound_RandomSe(69, 2, 67, 2), KUSH_BONE_WING_A, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 116, Sound_RandomSe(68, 3, 68, 2), KUSH_BONE_WING_A, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 124, Sound_RandomSe(69, 3, 69, 4), KUSH_BONE_WING_A, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 132, Sound_RandomSe(67, 3, 68, 3), KUSH_BONE_WING_A, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 144, Sound_RandomSe(67, 4, 69, 2), KUSH_BONE_WING_A, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 68, Sound_RandomSe(69, 1, 67, 1), KUSH_BONE_WING_B, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 74, Sound_RandomSe(69, 1, 67, 1), KUSH_BONE_WING_B, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 84, Sound_RandomSe(68, 1, 67, 3), KUSH_BONE_WING_B, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 90, Sound_RandomSe(69, 2, 67, 2), KUSH_BONE_WING_B, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 98, Sound_RandomSe(69, 1, 68, 1), KUSH_BONE_WING_B, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 108, Sound_RandomSe(67, 1, 67, 2), KUSH_BONE_WING_B, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 112, Sound_RandomSe(69, 2, 67, 2), KUSH_BONE_WING_B, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 118, Sound_RandomSe(69, 2, 68, 2), KUSH_BONE_WING_B, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 124, Sound_RandomSe(69, 4, 67, 3), KUSH_BONE_WING_B, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 132, Sound_RandomSe(69, 2, 69, 4), KUSH_BONE_WING_B, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 146, Sound_RandomSe(67, 4, 68, 2), KUSH_BONE_WING_B, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 64, Sound_RandomSe(68, 1, 69, 2), 17, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 72, Sound_RandomSe(69, 1, 68, 1), 17, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 80, Sound_RandomSe(68, 1, 69, 2), 17, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 90, Sound_RandomSe(68, 2, 68, 1), 17, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 98, Sound_RandomSe(67, 4, 69, 2), 17, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 104, Sound_RandomSe(68, 2, 68, 2), 17, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 110, Sound_RandomSe(67, 2, 69, 2), 17, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 120, Sound_RandomSe(68, 2, 68, 2), 17, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 134, Sound_RandomSe(67, 3, 69, 4), 17, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 140, Sound_RandomSe(68, 4, 67, 2), 17, 4);
        Em_PlaySeAtBoneVolOnFrame(em, 148, Sound_RandomSe(67, 3, 69, 4), 17, 4);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_BLAST):
        Em_PlaySeTypeAtBoneInFrames(em, 88, 260, 80, 38, 7, 0);
        Em_PlaySeAtBoneOnFrame(em, 2, 51, 38);
        Em_PlaySeAtBoneOnFrame(em, 24, 58, 38);
        Em_PlaySeAtBoneOnFrame(em, 30, 63, 38);
        Em_PlaySeAtBoneOnFrame(em, 196, 60, 38);
        Em_PlaySeAtBoneOnFrame(em, 4, 13, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 8, 13, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 40, 15, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 44, 15, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 14, 0, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 210, 0, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 226, 1, KUSH_BONE_FOOT_C);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_FLINCH):
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 83, 38);
        Em_PlaySeAtBoneOnFrame(em, 56, 35, 38);
        Em_PlaySeAtBoneOnFrame(em, 14, 0, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 22, 1, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 20, 3, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 88, 4, KUSH_BONE_FOOT_D);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_TOPPLE_RECOVER):
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 84, 38);
        Em_PlaySeAtBoneOnFrame(em, 62, 41, 38);
        Em_PlaySeAtBoneOnFrame(em, 28, 7, 38);
        Em_PlaySeAtBoneOnFrame(em, 14, 2, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 98, 0, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 20, 2, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 92, 0, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 82, 3, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 104, 3, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 22, 14, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 26, 14, KUSH_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_FLINCH_HOVER):
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 83, 38);
        Em_PlaySeAtBoneOnFrame(em, 14, 13, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 18, 13, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 58, 11, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 62, 11, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 110, 11, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 114, 11, KUSH_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_FALL):
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 82, 38);
        Em_PlaySeAtBoneOnFrame(em, 6, 12, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 10, 12, KUSH_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_FALL_LAND):
        Em_PlaySeAtBoneOnFrame(em, 8, 9, KUSH_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame(em, 24, 7, KUSH_BONE_BODY);
        Em_PlaySeAtBoneOnFrame(em, 26, 15, KUSH_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_DOWN):
        Em_PlaySeTypeAtBoneOnFrame(em, 24, 87, 38);
        Em_PlaySeAtBoneOnFrame(em, 38, 16, 38);
        Em_PlaySeAtBoneOnFrame(em, 26, 23, 38);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_DOWN_GET_UP):
        Em_PlaySeAtBoneOnFrame(em, 16, 41, 38);
        Em_PlaySeAtBoneOnFrame(em, 2, 18, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 22, 0, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 90, 0, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 58, 2, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 86, 3, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 98, 3, KUSH_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_DIE):
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 87, 38);
        Em_PlaySeAtBoneOnFrame(em, 68, 48, 38);
        Em_PlaySeAtBoneOnFrame(em, 170, 59, 38);
        Em_PlaySeAtBoneOnFrame(em, 238, 45, 38);
        Em_PlaySeAtBoneOnFrame(em, 282, 38, 38);
        Em_PlaySeAtBoneOnFrame(em, 30, 0, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 44, 1, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 12, 3, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 28, 4, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 400, 16, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 412, 7, KUSH_BONE_BODY);
        Em_PlaySeAtBoneOnFrame(em, 438, 7, KUSH_BONE_BREATH);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_STATUS4):
        Em_PlaySeTypeAtBoneOnFrame(em, 12, 88, 38);
        Em_PlaySeAtBoneOnFrame(em, 26, 18, KUSH_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_SHAKE_OFF):
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 81, 38);
        Em_PlaySeAtBoneOnFrame(em, 24, 51, 38);
        Em_PlaySeAtBoneOnFrame(em, 64, 59, 38);
        Em_PlaySeAtBoneOnFrame(em, 22, 13, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 26, 13, KUSH_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_COLLAPSE):
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 85, 38);
        Em_PlaySeAtBoneOnFrame(em, 78, 60, 38);
        Em_PlaySeTypeAtBoneOnFrame(em, 62, 78, 38);
        Em_PlaySeAtBoneOnFrame(em, 270, 54, 38);
        Em_PlaySeAtBoneOnFrame(em, 20, 0, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 156, 18, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 58, 18, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 160, 16, KUSH_BONE_BODY);
        Em_PlaySeAtBoneOnFrame(em, 184, 7, KUSH_BONE_BODY);
        Em_PlaySeAtBoneOnFrame(em, 48, 14, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 52, 14, KUSH_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_TOPPLE):
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 84, 38);
        Em_PlaySeAtBoneOnFrame(em, 44, 38, 38);
        Em_PlaySeAtBoneOnFrame(em, 72, 42, 38);
        Em_PlaySeAtBoneOnFrame(em, 16, 13, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 20, 13, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 64, 12, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 68, 12, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 136, 14, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 88, 3, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 110, 18, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 134, 5, KUSH_BONE_BODY);
        Em_PlaySeAtBoneOnFrame(em, 150, 9, KUSH_BONE_BREATH);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_PART_BREAK):
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 84, 38);
        Em_PlaySeAtBoneOnFrame(em, 56, 5, KUSH_BONE_BODY);
        Em_PlaySeAtBoneOnFrame(em, 76, 9, KUSH_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame(em, 8, 13, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 12, 13, KUSH_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_PART_BREAK_TURN):
        Em_PlaySeAtBoneOnFrame(em, 4, 60, 38);
        Em_PlaySeAtBoneOnFrame(em, 52, 51, 38);
        Em_PlaySeAtBoneOnFrame(em, 4, 14, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 86, 14, KUSH_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 90, 14, KUSH_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 122, 0, KUSH_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 138, 0, KUSH_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 84, 4, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 128, 4, KUSH_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 94, 4, KUSH_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 138, 0, KUSH_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, KUSH_MOT_IDLE):
        break;
    }
}

/*
 * The family's per-kind setup hook (Em_TypeSetup calls it for the four
 * kinds): nothing to set up.
 */
void EmKushala_Setup(Em *em)
{
}

/*
 * Callback-table slots 0 and 1: creates the alert mark (AlertMark_Create)
 * and the monster's shadow (SHADOW_MODEL), then its aura object
 * (MonsterFx_CreateOnEm type 12 for Kushala Daora, 29 for Lunastra and
 * Teostra).
 */
void EmKushala_SpawnEffects(Em *em)
{
    AlertMark_Create(em, 34, 0);
    Shadow_Spawn((Player *) em, SHADOW_MODEL);
    if (em->type == EM_KIND_KUSHALA_DAORA || em->type == EM_KIND_RUSTED_KUSHALA_DAORA) {
        MonsterFx_CreateOnEm(em, 12, em->scaleX);
    } else if (em->type == EM_KIND_LUNASTRA || em->type == EM_KIND_TEOSTRA) {
        MonsterFx_CreateOnEm(em, 29, em->scaleX);
    }
}

/*
 * Sets the monster up: parts, the idle motion, the ground stance, the wind
 * off and the flight defaults. At the quest's start it places the monster
 * by area; hp is fixed in two story quests, else Em_ComputeMaxHp (or the
 * kept hp of a monster that comes back). A part 2 already broken is counted
 * in partBreakCount.
 */
void EmKushala_Init(Em *em)
{
    EmKushalaWork *w = (EmKushalaWork *)&em->work;

    Em_SetPartScaled(em, 3);
    Em_SetPartScaled(em, 4);
    if (gQuest.stage == 0) {
        em->rotY = ANGLE_TURN / 2;
        switch (gScene.area) {
        case 0:
            em->posX = 8000.0f + 1500.0f * (u32) em->slotNo;
            em->posY = 0.0f;
            em->posZ = 6000.0f;
            break;
        case AREA_SNOWY_MOUNTAINS_NIGHT_9:
            em->posX = 7900.0f + 1500.0f * (u32) em->slotNo;
            em->posY = 1050.0f;
            em->posZ = 13900.0f;
            break;
        case AREA_JUNGLE_NIGHT_5:
            em->posX = 10000.0f + 1500.0f * (u32) em->slotNo;
            em->posY = 200.0f;
            em->posZ = 8000.0f;
            break;
        case AREA_JUNGLE_NIGHT_7:
            em->posX = 9900.0f + 1500.0f * (u32) em->slotNo;
            em->posY = 300.0f;
            em->posZ = 10350.0f;
            break;
        case AREA_DESERT_NIGHT_11:
            em->posX = 10000.0f + 1500.0f * (u32) em->slotNo;
            em->posY = 0.0f;
            em->posZ = 5700.0f;
            em->rotY = ANGLE_TURN / 4;
            break;
        case AREA_VOLCANO_NIGHT_9:
            em->posX = 14300.0f + 1500.0f * (u32) em->slotNo;
            em->posY = 0.0f;
            em->posZ = 7100.0f;
            break;
        case AREA_FOREST_HILLS_4:
            em->posX = 8500.0f;
            em->posY = 0.0f;
            em->posZ = 10000.0f;
            em->rotY = ANGLE_TURN * 5 / 16;
            break;
        case AREA_FOREST_HILLS_5:
            em->posX = 10500.0f + 1500.0f * (u32) em->slotNo;
            em->posY = 0.0f;
            em->posZ = 9500.0f;
            break;
        default:
            em->posX = 5000.0f + 1500.0f * (u32) em->slotNo;
            em->posY = 0.0f;
            em->posZ = 9000.0f;
            break;
        }
    }
    Em_SetMotionRow(em, KUSH_MOT_IDLE, 0, 0);
    em->stance = EM_STANCE_GROUND;
    w->windFrames = 0;
    if ((Quest_FindSpecialMonster(em->type) & 0xFF) == 255) {
        switch (gQuest.stage) {
        case QUEST_ID_KUSHALA_SNOW_SCENE:
        case QUEST_ID_LUNASTRA_TEOSTRA_TOWER_SCENE:
            em->hp = 2000;
            em->hpMax = 4000;
            break;
        default:
            em->hpMax = em->hp = Em_ComputeMaxHp(em, 0);
            break;
        }
    } else {
        QuestEm_LoadPersistentHp(em);
    }
    em->scriptPending = 1;
    em->blind = 0;
    em->unk_InitFlag = 1;
    em->aimAngle = ANGLE_TURN / 4;
    em->spawnParam = 0;
    w->windOn = 0;
    w->pitchStep = 0x100;
    w->turnSpeed = 512;
    w->rollSpeed = 0x100;
    w->moveAngBY = ANGLE_TURN / 8;
    w->moveAngBX = 0;
    w->unk01B = 0;
    em->meshShown[1] = 0;
    if (Em_PartBroken(em, 2) == 0) {
        EmTail_Create(em);
    } else {
        em->fxFlags |= 1;
        if (em->type == EM_KIND_KUSHALA_DAORA || em->type == EM_KIND_RUSTED_KUSHALA_DAORA) {
            em->partBreakCount = 1;
        } else {
            em->partBreakCount = 2;
        }
    }
}

/*
 * The start hook: runs EmKushala_Init, then in the story quests holds the
 * monster at its scene's spot until the scene's event flag is set, and goes
 * to KUSH_IDLE.
 */
void EmKushala_Start(Em *em)
{
    EmKushala_Init(em);
    if (em->type == EM_KIND_KUSHALA_DAORA || em->type == EM_KIND_RUSTED_KUSHALA_DAORA) {
        switch (gQuest.stage) {
        case QUEST_ID_KUSHALA_JUNGLE_SCENE:
            if (Save_TestEventFlag(EVENT_FLAG_KUSHALA_JUNGLE_SCENE) == 0) {
                em->area = Area_IdForCurrentTime(AREA_JUNGLE_3);
                em->posX = 7146.0f;
                em->posY = 200.0f;
                em->posZ = 11034.0f;
                em->rotY = ANGLE_TURN * 11 / 32;
            }
            break;
        }
    } else {
        switch (gQuest.stage) {
        case QUEST_ID_LUNASTRA_TEOSTRA_TOWER_SCENE:
            if (Save_TestEventFlag(EVENT_FLAG_LUNASTRA_TEOSTRA_TOWER_SCENE) == 0) {
                em->area = Area_IdForCurrentTime(AREA_TOWER_0x7C);
                em->posX = 11600.0f;
                em->posY = -20.0f;
                em->posZ = 13000.0f;
                em->rotY = ANGLE_TURN * 13 / 32;
            }
            break;
        case QUEST_ID_LUNASTRA_TEOSTRA_VOLCANO_7_SCENE_1:
        case QUEST_ID_LUNASTRA_TEOSTRA_VOLCANO_7_SCENE_2:
            if (Save_TestEventFlag(EVENT_FLAG_LUNASTRA_TEOSTRA_VOLCANO_7_SCENE) == 0) {
                em->area = Area_IdForCurrentTime(AREA_VOLCANO_7);
                em->posX = 9329.0f;
                em->posY = 3000.0f;
                em->posZ = 9973.0f;
                em->rotY = 0xFC00;
            }
            break;
        }
    }
    EmKushala_ChangeState(em, EM_GROUP_IDLE, KUSH_IDLE, EM_SYNC_LOCAL);
}

/*
 * Callback slot 5: runs EmKushala_Init, then puts the monster at a fixed
 * spot. The kind test names Rathian kinds, which never run this function,
 * so the family always takes the second spot.
 */
void EmKushala_Reinit(Em *em)
{
    u8 type;

    EmKushala_Init(em);
    type = em->type;
    if ((type == EM_KIND_RATHIAN) || (type == EM_KIND_GOLD_RATHIAN) || (type == EM_KIND_PINK_RATHIAN)) {
        em->rotY = 0x8A76;
        em->posX = 11948.9f;
        em->posY = 1010.0f;
        em->posZ = 15923.8f;
        return;
    }
    em->rotY = 0x4861;
    em->posX = 3743.2f;
    em->posY = 1010.0f;
    em->posZ = 11415.0f;
}

/*
 * Picks the idle for the stance: KUSH_FLY_HOVER in the air, KUSH_IDLE_HOVER
 * in the hover; on the ground KUSH_IDLE_SETTLE out of combat after a motion
 * of EmKushala_IdleTbl, else KUSH_IDLE with the script asked for its next
 * state.
 */
void EmKushala_ToIdle(Em *em)
{
    u16 *idleEntry = EmKushala_IdleTbl;
    s32 found = 0;

    switch (em->stance) {
    case EM_STANCE_AIR:
        em->scriptPending = 1;
        EmKushala_ChangeState(em, EM_GROUP_FLY, KUSH_FLY_HOVER, EM_SYNC_LOCAL);
        break;
    case EM_STANCE_FAMILY:
        em->scriptPending = 1;
        EmKushala_ChangeState(em, EM_GROUP_IDLE, KUSH_IDLE_HOVER, EM_SYNC_LOCAL);
        break;
    case EM_STANCE_GROUND:
    default:
        if (em->stateGroup != EM_GROUP_IDLE || em->state != KUSH_IDLE_SETTLE) {
            if (em->combat == 0) {
                for (; *idleEntry != 0xFFFF; idleEntry++) {
                    if (em->motionId[0] == *idleEntry) {
                        found = 1;
                        break;
                    }
                }
            }
        }
        if (found) {
            EmKushala_ChangeState(em, EM_GROUP_IDLE, KUSH_IDLE_SETTLE, EM_SYNC_LOCAL);
        } else {
            em->scriptPending = 1;
            EmKushala_ChangeState(em, EM_GROUP_IDLE, KUSH_IDLE, EM_SYNC_LOCAL);
        }
        break;
    }
}

/* Ends a state through EmKushala_ToIdle; the two extra arguments are unused. */
void EmKushala_EndState(Em *em, s32 unused1, s32 unused2)
{
    EmKushala_ToIdle(em);
}

/*
 * Ends a state in the air: next 0 goes to KUSH_FLY_HOVER, 1 to
 * KUSH_FLY_HOVER_GUARDED, any other value only asks the script for its next
 * state.
 */
void EmKushala_EndFlyState(Em *em, u8 next)
{
    s32 k;

    k = next & 0xFF;
    em->stance = EM_STANCE_AIR;
    em->scriptPending = 1;
    em->motionSpeed = 1.0f;
    switch (k) {
    case 0:
        Em_SetStateRand(em, 2, 2);
        break;
    case 1:
        Em_SetStateRand(em, 2, 3);
        break;
    }
}

/*
 * Marks the part breaks a flinch earns: part 0 below 60% hp once it is
 * damaged (Lunastra and Teostra also need partFlags 0x20), part 5 once
 * damaged. With apply set, applies them (Em_ApplyPartBreaks).
 */
void EmKushala_BreakParts(Em *em, s32 apply)
{
    u8 k;

    if (em->type == EM_KIND_LUNASTRA || em->type == EM_KIND_TEOSTRA) {
        k = em->flinchPart;
        switch (k) {
        case 0:
            if (em->hp < (s16) (0.6f * em->hpMax) && em->parts[k].state >= 2 && (em->partFlags[k] & 0x20)) {
                Em_FlagPartBreak(em, 0);
            }
            break;
        case 5:
            if (em->parts[k].state >= 2) {
                Em_FlagPartBreak(em, 1);
            }
            break;
        }
    } else {
        k = em->flinchPart;
        switch (k) {
        case 0:
            if (em->hp < (s16) (0.6f * em->hpMax) && em->parts[k].state >= 2) {
                Em_FlagPartBreak(em, 0);
            }
            break;
        case 5:
            if (em->parts[k].state >= 2) {
                Em_FlagPartBreak(em, 1);
            }
            break;
        }
    }
    if (apply != 0) {
        Em_ApplyPartBreaks(em);
    }
}

/*
 * Creates a breath attack at a bone (KushalaBreath_CreateAtBone); on the
 * snowy mountains the size is larger (+3 for breath kinds 0 and 2, +1 for
 * kind 1).
 */
void EmKushala_CreateBreathAtBone(Em *em, s32 bone, u8 kind, u8 size, s32 pitch, s32 yaw)
{
    if (gScene.map == MAP_SNOWY_MOUNTAINS || gScene.map == MAP_SNOWY_MOUNTAINS_NIGHT) {
        switch (kind) {
        case 0:
        case 2:
            size += 3;
            break;
        case 1:
            size += 1;
            break;
        }
    }
    KushalaBreath_CreateAtBone(em, bone, kind, size, pitch, yaw);
}

/*
 * KUSH_IDLE_STAND: the idle motion on all three layers until Em.timer runs
 * out, then asks the script.
 */
void EmKushala_Stand(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        if (em->motionId[0] != EM_MOTION_ID(0, KUSH_MOT_IDLE)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(0, KUSH_MOT_IDLE), 10, 0, 0);
        }
        if (em->motionId[1] != EM_MOTION_ID(1, KUSH_MOT_IDLE)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(1, KUSH_MOT_IDLE), 10, 0, 1);
        }
        if (em->motionId[2] != EM_MOTION_ID(2, KUSH_MOT_IDLE)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(2, KUSH_MOT_IDLE), 10, 0, 2);
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
 * KUSH_IDLE: plays the idle motion once, then ends the state; a remote copy
 * waits for the owner.
 */
void EmKushala_Idle(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_IDLE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
        case 2:
            if (em->remote == 0) {
                em->step++;
                EmKushala_EndState(em, 0, 0);
            }
        }
        break;
    }
}

/*
 * KUSH_IDLE_HOVER: one hover cycle in stance 1, then ends the state
 * (EmKushala_ToIdle comes back here while hovering).
 */
void EmKushala_HoverIdle(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_HOVER, 0, 0);
        em->stance = EM_STANCE_FAMILY;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/* KUSH_IDLE_ALERT: plays the alert motion once, then ends the state. */
void EmKushala_AlertIdle(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_ALERT, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * KUSH_IDLE_EAT_NEED1_START and KUSH_IDLE_EAT_NEED2_START (variant 0, 1):
 * lowers the head without flinching; variant 0 makes the bite effects at
 * the mouth at frames 56 and 102 when a player is in the area. Then
 * KUSH_IDLE_EAT_NEED1 or KUSH_IDLE_EAT_NEED2.
 */
void EmKushala_EatNeedStart(Em *em, EmWork *work, s32 variant)
{
    f32 pos[3];
    f32 ofs[3];
    f32 boneMtx[16];

    Em_SetHitReactMode(em, EM_HIT_REACT_NO_FLINCH);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_EAT_START, 0, 0);
        em->timer = 3;
        break;
    case 1:
        if ((EmMotion_PassedFrameScaled(em, 56.0f, 0) || EmMotion_PassedFrameScaled(em, 102.0f, 0)) && variant == 0 && ((s32)Player_InCurrentArea(em) & 0xFF)) {
            FallingDebris_Create(em, 1);
            Mtx_Copy(boneMtx, (f32 *)Em_Bone(em, KUSH_BONE_BREATH));
            ofs[0] = 0.0f;
            ofs[1] = -45.0f;
            ofs[2] = 165.0f;
            Vec_RotateSelf(ofs, boneMtx);
            Mtx_GetTrans(pos, boneMtx);
            pos[0] += ofs[0];
            pos[1] += ofs[1];
            pos[2] += ofs[2];
            HitSpark_SpawnForOwner(em, 0, 9, 0, pos, 1.0f);
            PartFx_SpawnAt(pos, 20, 4.0f);
        }
        if (em->motion[0].playing == 0 && --em->timer <= 0) {
            em->step++;
            if (variant == 0) {
                EmKushala_ChangeState(em, EM_GROUP_IDLE, KUSH_IDLE_EAT_NEED1, EM_SYNC_OWNER_FLAGGED);
            } else {
                EmKushala_ChangeState(em, EM_GROUP_IDLE, KUSH_IDLE_EAT_NEED2, EM_SYNC_OWNER_FLAGGED);
            }
        }
        break;
    case 2:
        if (em->remote == 0) {
            if (variant == 0) {
                EmKushala_ChangeState(em, EM_GROUP_IDLE, KUSH_IDLE_EAT_NEED1, EM_SYNC_OWNER_FLAGGED);
            } else {
                EmKushala_ChangeState(em, EM_GROUP_IDLE, KUSH_IDLE_EAT_NEED2, EM_SYNC_OWNER_FLAGGED);
            }
        }
        break;
    }
}

/*
 * KUSH_IDLE_EAT_NEED1 and KUSH_IDLE_EAT_NEED2: the eating motion, then ends
 * the state; Kushala Daora's variant 0 also drops a timed mine point where
 * it stands.
 */
void EmKushala_EatNeed(Em *em, EmWork *work, s32 variant)
{
    u8 type;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_EAT, 0, 0);
        type = em->type;
        if (((type == EM_KIND_KUSHALA_DAORA) || (type == EM_KIND_RUSTED_KUSHALA_DAORA)) && (variant == 0)) {
            EmMine_CreateTimed0(em, &em->posX);
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/* KUSH_IDLE_ACTION_A: plays its motion once, then ends the state. */
void EmKushala_IdleActionA(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_IDLE_ACTION_A, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * KUSH_IDLE_STATUS4_SHAKE_OFF and KUSH_IDLE_STUN_SHAKE_OFF: shakes itself,
 * then ends the state.
 */
void EmKushala_StatusShakeOff(Em *em, EmWork *work, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, KUSH_MOT_SHAKE_OFF, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/* KUSH_IDLE_SHAKE_OFF: the same shake, after a status ends. */
void EmKushala_ShakeOff(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, KUSH_MOT_SHAKE_OFF, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * KUSH_IDLE_REST: lies down and regains 1 hp every 8 frames up to 40% of
 * its maximum (60% for Lunastra and Teostra) for 9000 frames, a hit waking
 * it; then KUSH_IDLE_REST_END.
 */
void EmKushala_Rest(Em *em)
{
    u8 type;

    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_REST, 0, 0);
        Em_LoadReactionSet(em, 1);
        em->timer = 9000;
        break;
    case 1:
        type = em->type;
        if ((type == EM_KIND_KUSHALA_DAORA) || (type == EM_KIND_RUSTED_KUSHALA_DAORA)) {
            Em_RegenHp(em, 1, (s32) (0.4f * (f32) em->hpMax), 8);
        } else {
            Em_RegenHp(em, 1, (s32) (0.6f * (f32) em->hpMax), 8);
        }
        if (--em->timer <= 0) {
            em->step++;
            Em_ClearLowHpNeed(em);
            EmKushala_ChangeState(em, EM_GROUP_IDLE, KUSH_IDLE_REST_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmKushala_ChangeState(em, EM_GROUP_IDLE, KUSH_IDLE_REST_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * KUSH_IDLE_REST_END: gets up (Lunastra and Teostra drop a timed mine
 * point); in combat it flaps its wings before the state ends.
 */
void EmKushala_RestEnd(Em *em)
{
    u8 type;

    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, KUSH_MOT_REST_END, 0, 0);
        Em_LoadReactionSet(em, 0);
        type = em->type;
        if ((type == EM_KIND_LUNASTRA) || (type == EM_KIND_TEOSTRA)) {
            EmMine_CreateTimed0(em, &em->posX);
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            if (em->combat == 1) {
                em->step++;
                Em_SetMotion(em, KUSH_MOT_WING_FLAP, 0, 0);
                break;
            }
            em->blind = 0;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->blind = 0;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * KUSH_IDLE_STATUS2_COLLAPSE: collapses under status 2 and lies until its
 * timer runs out, then KUSH_IDLE_STATUS2_END.
 */
void EmKushala_Status2Collapse(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_COLLAPSE, 0, 0);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            EmKushala_ChangeState(em, EM_GROUP_IDLE, KUSH_IDLE_STATUS2_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmKushala_ChangeState(em, EM_GROUP_IDLE, KUSH_IDLE_STATUS2_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * KUSH_IDLE_STATUS2_END: lies a moment longer, gets up, then
 * KUSH_IDLE_SHAKE_OFF.
 */
void EmKushala_Status2End(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, KUSH_MOT_DOWN, 0, 0);
        Em_LoadReactionSet(em, 0);
        em->blind = 2;
        em->timer = 2;
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            if (--em->timer <= 0) {
                em->step++;
                Em_SetMotion(em, KUSH_MOT_DOWN_GET_UP, 0, 0);
                break;
            }
        }
        break;
    case 2:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            EmKushala_ChangeState(em, EM_GROUP_IDLE, KUSH_IDLE_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmKushala_ChangeState(em, EM_GROUP_IDLE, KUSH_IDLE_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * KUSH_IDLE_EAT_START: lowers the head without flinching (sight row 1,
 * reaction set 1), then KUSH_IDLE_EAT.
 */
void EmKushala_EatStart(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_NO_FLINCH);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_EAT_START, 0, 0);
        Em_SelectSightRow(em, 1);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_ChangeState(em, EM_GROUP_IDLE, KUSH_IDLE_EAT, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmKushala_ChangeState(em, EM_GROUP_IDLE, KUSH_IDLE_EAT, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * KUSH_IDLE_EAT: the eating motion with the normal sight and reactions,
 * then ends the state.
 */
void EmKushala_Eat(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_EAT, 0, 0);
        Em_SelectSightRow(em, 0);
        Em_LoadReactionSet(em, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * The roars (KUSH_IDLE_WIND_ROAR to _C and KUSH_IDLE_WIND_ROAR_OR_SWEEP):
 * turns toward the goal until frame 60 of the roar (from the alert stance
 * it starts at frame 30), then lets the motion end.
 */
void EmKushala_Roar(Em *em, EmWork *work)
{
    u16 targetYaw;
    u32 yawLeft;
    s32 yaw;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        if (em->motionId[0] == EM_MOTION_ID(0, KUSH_MOT_ALERT)) {
            Em_SetMotion(em, KUSH_MOT_ROAR, 10, 30);
        } else {
            Em_SetMotion(em, KUSH_MOT_ROAR, 0, 0);
        }
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->noReact = 1;
        break;
    case 1:
        yaw = em->rotY;
        targetYaw = em->aimAngle;
        yawLeft = (targetYaw - (yaw & 0xFFFF)) & 0xFFFF;
        if (((yawLeft + 0x200) & 0xFFFF) < 0x400U) {
            em->rotY = targetYaw;
        } else if (yawLeft < 0x8000U) {
            em->rotY = (yaw + 0x200) & 0xFFFF;
        } else {
            em->rotY = (yaw - 0x200) & 0xFFFF;
        }
        if (EmMotion_ReachedFrame(em, 0, 60.0f) != 0) {
            em->step++;
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->noReact = 0;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/* KUSH_IDLE_ACTION_B: plays its motion once, then ends the state. */
void EmKushala_IdleActionB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_IDLE_ACTION_B, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/* KUSH_IDLE_ALERT_B: the same as EmKushala_AlertIdle. */
void EmKushala_AlertIdleB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_ALERT, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * KUSH_IDLE_STATUS1_COLLAPSE: collapses under status 1 and lies until its
 * timer runs out, then KUSH_IDLE_STATUS1_END.
 */
void EmKushala_Status1Collapse(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_COLLAPSE, 0, 0);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            EmKushala_ChangeState(em, EM_GROUP_IDLE, KUSH_IDLE_STATUS1_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmKushala_ChangeState(em, EM_GROUP_IDLE, KUSH_IDLE_STATUS1_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * KUSH_IDLE_STATUS1_END: lies a moment longer, gets up, then
 * KUSH_IDLE_SHAKE_OFF.
 */
void EmKushala_Status1End(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, KUSH_MOT_DOWN, 0, 0);
        Em_LoadReactionSet(em, 0);
        em->blind = 2;
        em->timer = 2;
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            if (--em->timer <= 0) {
                em->step++;
                Em_SetMotion(em, KUSH_MOT_DOWN_GET_UP, 0, 0);
                break;
            }
        }
        break;
    case 2:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            EmKushala_ChangeState(em, EM_GROUP_IDLE, KUSH_IDLE_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmKushala_ChangeState(em, EM_GROUP_IDLE, KUSH_IDLE_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * KUSH_IDLE_SETTLE: plays the end of the landing motion (from frame 146),
 * then ends the state.
 */
void EmKushala_Settle(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, KUSH_MOT_LAND, 0, 146);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * KUSH_IDLE_ROAR: the same roar as EmKushala_Roar; entering it leaves the
 * wind as it is.
 */
void EmKushala_RoarB(Em *em, EmWork *work)
{
    u16 targetYaw;
    u32 yawLeft;
    s32 yaw;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        if (em->motionId[0] == EM_MOTION_ID(0, KUSH_MOT_ALERT)) {
            Em_SetMotion(em, KUSH_MOT_ROAR, 10, 30);
        } else {
            Em_SetMotion(em, KUSH_MOT_ROAR, 0, 0);
        }
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->noReact = 1;
        break;
    case 1:
        yaw = em->rotY;
        targetYaw = em->aimAngle;
        yawLeft = (targetYaw - (yaw & 0xFFFF)) & 0xFFFF;
        if (((yawLeft + 0x200) & 0xFFFF) < 0x400U) {
            em->rotY = targetYaw;
        } else if (yawLeft < 0x8000U) {
            em->rotY = (yaw + 0x200) & 0xFFFF;
        } else {
            em->rotY = (yaw - 0x200) & 0xFFFF;
        }
        if (EmMotion_ReachedFrame(em, 0, 60.0f) != 0) {
            em->step++;
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->noReact = 0;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/* KUSH_MOVE_WALK: walks to the goal (Em_StepToGoal), then ends the state. */
void EmKushala_Walk(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_WALK, 0, 0);
        break;
    case 1:
        if (Em_StepToGoal(em, 64, 0) == 1) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * KUSH_MOVE_LIMP: limps to the goal; out of combat it then settles with the
 * end of the landing motion.
 */
void EmKushala_Limp(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_LIMP, 0, 0);
        break;
    case 1:
        if (Em_StepToGoal(em, 64, 0) == 1) {
            if (em->combat == 0) {
                em->step++;
                Em_SetMotion(em, KUSH_MOT_LAND, 0, 146);
                break;
            }
            EmKushala_EndState(em, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * KUSH_MOVE_TURN: turns toward the target with EmKushala_TurnParams[0],
 * then ends the state.
 */
void EmKushala_Turn(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_StartTurnToTarget(em, &EmKushala_TurnParams[0]);
        break;
    case 1:
        if (Em_TurnStep(em, &EmKushala_TurnParams[0]) == 1) {
            em->step++;
            Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * KUSH_MOVE_TURN_ALERT: picks the small or a big turn by the angle to the
 * goal and turns rotY by a 19th of it a frame over frames 16-54.
 */
void EmKushala_TurnAlert(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        work->turnSpeed = (em->aimAngle - em->rotY) & 0xFFFF;
        if (work->turnSpeed <= 9102 || work->turnSpeed >= 0xDC72) {
            Em_SetMotion(em, KUSH_MOT_TURN_SMALL, 0, 0);
        } else if (work->turnSpeed >= 0x8000) {
            Em_SetMotion(em, KUSH_MOT_TURN_BIG_LEFT, 0, 0);
        } else {
            Em_SetMotion(em, KUSH_MOT_TURN_BIG_RIGHT, 0, 0);
        }
        if (work->turnSpeed >= 0x8000) {
            work->turnSpeed -= 0x10000;
        }
        break;
    case 1:
        switch (em->motionId[0]) {
        case EM_MOTION_ID(0, KUSH_MOT_TURN_SMALL):
        case EM_MOTION_ID(0, KUSH_MOT_TURN_BIG_LEFT):
        case EM_MOTION_ID(0, KUSH_MOT_TURN_BIG_RIGHT):
            if (EmMotion_InFrameRange(em, 0, 16.0f, 54.0f)) {
                em->rotY = (em->rotY + work->turnSpeed / 19) & 0xFFFF;
            }
            break;
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/* KUSH_MOVE_WALK_B: a start motion, then walks to the goal and stops. */
void EmKushala_WalkB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_WALK_B_START, 0, 0);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_SIDE_DASH_B, 0, 0);
        }
        
    case 2:
        if (Em_StepToGoal(em, 64, 0) == 1) {
            em->step = 3;
            Em_SetMotion(em, KUSH_MOT_RUN_STOP, 0, 0);
        }
        break;
    case 3:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * The runs (KUSH_MOVE_RUN, KUSH_MOVE_RUN_PAST and its arcs): runs to the
 * goal with a contact attack from frame 12; variants 1-4 curve, turning
 * rotY by a fixed step a frame; then stops.
 */
void EmKushala_Run(Em *em, EmWork *work, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_RUN_START, 0, 0);
        break;
    case 1:
        switch (variant) {
        case 1:
            em->rotY += 91;
            break;
        case 2:
            em->rotY += 227;
            break;
        case 3:
            em->rotY -= 91;
            break;
        case 4:
            em->rotY -= 227;
            break;
        }
        if (Em_StepToGoal(em, 64, 0) == 1) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_RUN_STOP, 0, 0);
            break;
        }
        if (em->motionId[0] == EM_MOTION_ID(0, KUSH_MOT_RUN_START) && EmMotion_PassedFrameScaled(em, 12.0f, 0)) {
            EmKushala_CreateAttack(em, 8);
        }
        if (em->motion[0].playing == 0) {
            if (em->motionId[0] == EM_MOTION_ID(0, KUSH_MOT_RUN_START)) {
                Em_SetMotion(em, KUSH_MOT_RUN_B, 0, 0);
            } else if (em->motionId[0] == EM_MOTION_ID(0, KUSH_MOT_RUN_B)) {
                Em_SetMotion(em, KUSH_MOT_RUN_C, 0, 0);
            }
        }
        break;
    case 2:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * KUSH_MOVE_STOMP: rears up, stomps (its attack comes from
 * EmKushala_MotionEffects) and takes the alert stance.
 */
void EmKushala_Stomp(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_REAR_UP, 0, 0);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_STOMP, 0, 0);
        }
        break;
    case 2:
        if (em->motion[1].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_ALERT, 0, 0);
        }
        break;
    case 3:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/* KUSH_MOVE_LIFT_TO_HOVER: rears up and lifts into the hover (stance 1). */
void EmKushala_LiftToHover(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_REAR_UP, 0, 0);
        em->stance = EM_STANCE_GROUND;
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            em->stance = EM_STANCE_FAMILY;
            Em_SetMotion(em, KUSH_MOT_LIFT_TO_HOVER, 0, 0);
        }
        break;
    case 2:
        if (em->motion[1].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_HOVER, 0, 28);
        }
        break;
    case 3:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * KUSH_MOVE_TAKE_OFF_TO_HOVER: flaps first if it was idle, then takes off
 * into the hover, stance 1 from frame 48.
 */
void EmKushala_TakeOffToHover(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        if (em->motionId[0] == EM_MOTION_ID(0, KUSH_MOT_IDLE)) {
            Em_SetMotion(em, KUSH_MOT_WING_FLAP, 0, 0);
            break;
        }
        em->step++;
        Em_SetMotion(em, KUSH_MOT_TAKE_OFF, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_TAKE_OFF, 0, 0);
        }
        break;
    case 2:
        if (EmMotion_ReachedFrame(em, 0, 48.0f) != 0) {
            em->step++;
            em->stance = EM_STANCE_FAMILY;
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * KUSH_MOVE_SIDE_DASH_A and _B (variant 0, 1): slides sideways 50 a frame
 * for 88 frames while turning to keep the goal at its side.
 */
void EmKushala_SideDash(Em *em, EmWork *work, s32 variant)
{
    f32 ofs[3];
    f32 out[3];
    s32 ang[3];
    f32 rotMtx[16];
    s32 targetYaw;
    s32 yawDiff;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        if (variant == 0) {
            Em_SetMotion(em, KUSH_MOT_SIDE_DASH_A, 0, 0);
        } else {
            Em_SetMotion(em, KUSH_MOT_SIDE_DASH_B, 0, 0);
        }
        Player_ClearVelocity(em);
        em->stance = EM_STANCE_FAMILY;
        break;
    case 1:
        if (EmMotion_InFrameRange(em, 0, 0.0f, 88.0f)) {
            ang[0] = 0;
            ang[1] = em->rotY;
            ang[2] = 0;
            if (variant == 0) {
                ofs[0] = 50.0f;
            } else {
                ofs[0] = -50.0f;
            }
            ofs[1] = 0.0f;
            ofs[2] = 0.0f;
            Mtx_FromAnglesYXZ(ang, rotMtx);
            Vec_Rotate(out, ofs, rotMtx);
            em->posX += out[0];
            em->posZ += out[2];
            if (variant == 0) {
                targetYaw = ((Math_AngleXZ(&em->posX, em->goal) & 0xFFFF) + ANGLE_TURN * 3 / 4) & 0xFFFF;
            } else {
                targetYaw = ((Math_AngleXZ(&em->posX, em->goal) & 0xFFFF) + ANGLE_TURN / 4) & 0xFFFF;
            }
            yawDiff = ((targetYaw & 0xFFFF) - em->rotY) & 0xFFFF;
            if (yawDiff <= ANGLE_TURN / 2) {
                if (yawDiff <= 0x173) {
                    em->rotY += yawDiff;
                } else {
                    em->rotY += 0x174;
                }
            } else {
                if (yawDiff > 0xFE8C) {
                    em->rotY += yawDiff;
                } else {
                    em->rotY -= 0x174;
                }
            }
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * KUSH_MOVE_HOVER_FORWARD, _PAST and KUSH_MOVE_HOVER_BACKWARD (variant 1:
 * backwards, facing away): hovers to the goal at speed 30.
 */
void EmKushala_HoverForward(Em *em, EmWork *work, s32 variant)
{
    u8 flags;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_HOVER, 0, 0);
        em->stance = EM_STANCE_FAMILY;
        em->velX = 0.0f;
        em->velY = 0.0f;
        em->velZ = 30.0f;
        Player_ClearAccel2(em);
        if (variant == 1) {
            em->speedBonus = 50;
        }
        break;
    case 1:
        work->moveAngX = 0;
        work->moveAngY = em->rotY;
        if (variant == 1) {
            flags = 4;
            work->moveAngY = (work->moveAngY + 0x7FFF + 1) & 0xFFFF;
        } else {
            flags = 0;
        }
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
        em->goalDist -= em->velZ;
        if (em->hasGoal != 0) {
            Em_TurnToGoal(em, 64, flags);
        }
        if (em->goalDist <= 0.0f) {
            em->step++;
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * KUSH_MOVE_HOVER_DASH: a fast flight in the hover with attacks at frame
 * 58, a long leg at speed 80 if the goal is far, then the brake.
 */
void EmKushala_HoverDash(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_DASH_START, 0, 0);
        EmKushala_StartCurves(em, 1U);
        em->stance = EM_STANCE_FAMILY;
        break;
    case 1:
        EmKushala_UpdateCurves(em);
        em->goalDist -= em->velZ;
        if ((em->motionId[0] == EM_MOTION_ID(0, KUSH_MOT_DASH_START)) && (EmMotion_PassedFrameScaled(em, 58.0f, 0U) != 0)) {
            EmKushala_CreateAttack(em, 9);
            EmKushala_CreateAttack(em, 26);
        }
        if (em->motion[0].playing == 0) {
            if (em->goalDist <= 0.0f) {
                em->step += 2;
                Em_SetMotion(em, KUSH_MOT_DASH_END, 0, 0);
                EmKushala_StartCurves(em, 2U);
                break;
            }
            em->step++;
            em->velX = 0.0f;
            em->velY = 0.0f;
            em->velZ = 80.0f;
            Player_ClearAccel2(em);
            Em_SetMotion(em, KUSH_MOT_FLY_FAST, 0, 0);
        }
        break;
    case 2:
        work->moveAngX = 0;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
        em->goalDist -= em->velZ;
        if (Em_StepToGoal(em, 64, 0) == 1) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_DASH_END, 0, 0);
            EmKushala_StartCurves(em, 2U);
        }
        break;
    case 3:
        EmKushala_UpdateCurves(em);
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * KUSH_MOVE_HOVER_TURN: turns to the goal in the hover, spread over 31
 * frames (a small angle) or 27 (a large one with the hover turn motion).
 */
void EmKushala_HoverTurn(Em *em, EmWork *work)
{
    s32 turnStep;
    s32 yawLeft;
    s32 yaw;
    u16 targetYaw;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_FAMILY;
        em->lookAtTarget = 0;
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        work->turnSpeed = (em->aimAngle - em->rotY) & 0xFFFF;
        if (work->turnSpeed <= 16384 || work->turnSpeed >= 0xC000) {
            Em_SetMotion(em, KUSH_MOT_HOVER, 0, 0);
            em->timer = 31;
        } else {
            Em_SetMotion(em, KUSH_MOT_HOVER_TURN, 0, 0);
            em->timer = 27;
        }
        if (work->turnSpeed >= 0x8000) {
            work->turnSpeed -= 0x10000;
        }
        break;
    case 1:
        if (em->motion[0].blend == 0) {
            turnStep = work->turnSpeed / em->timer;
            yaw = em->rotY;
            targetYaw = em->aimAngle;
            yawLeft = (targetYaw - yaw) & 0xFFFF;
            if (((yawLeft + turnStep) & 0xFFFF) < turnStep * 2) {
                em->rotY = targetYaw & 0xFFFF;
            } else {
                em->rotY = (yaw + turnStep) & 0xFFFF;
            }
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * KUSH_MOVE_LAND and KUSH_MOVE_LAND_QUICK (variant 1, which ends at frame
 * 146): sets down from the hover, on the ground from frame 70.
 */
void EmKushala_LandFromHover(Em *em, EmWork *work, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_FAMILY;
        Em_SetMotion(em, KUSH_MOT_LAND, 0, 0);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 70.0f) != 0) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
        }
        break;
    case 2:
        if (variant == 0) {
            if (em->motion[0].playing == 0) {
                em->step++;
                EmKushala_EndState(em, 0, 0);
                break;
            }
        } else if (EmMotion_ReachedFrame(em, 0, 146.0f) != 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * KUSH_MOVE_HOVER_SPIN: one full turn on the spot in the hover (rotY -0x555
 * a frame over frames 6-54).
 */
void EmKushala_HoverSpin(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, KUSH_MOT_HOVER_TURN, 0, 0);
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_FAMILY;
        break;
    case 1:
        if ((EmMotion_ReachedFrame(em, 0, 6.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 54.0f) == 0)) {
            em->rotY -= 0x555;
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * KUSH_FLY_TAKE_OFF: flaps first if it was idle, then takes off with the
 * lift curve; in the air (stance 2) from frame 48.
 */
void EmKushala_TakeOff(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        work->flyGuard = 0;
        if (em->motionId[0] == EM_MOTION_ID(0, KUSH_MOT_IDLE)) {
            Em_SetMotion(em, KUSH_MOT_WING_FLAP, 0, 0);
            break;
        }
        em->step++;
        Em_SetMotion(em, KUSH_MOT_TAKE_OFF, 0, 0);
        EmKushala_StartCurves(em, 0U);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_TAKE_OFF, 0, 0);
            EmKushala_StartCurves(em, 0U);
        }
        break;
    case 2:
        if (EmMotion_PassedFrameScaled(em, 48.0f, 0U) != 0) {
            em->step++;
            em->stance = EM_STANCE_AIR;
            EmKushala_UpdateCurves(em);
        }
        break;
    case 3:
        EmKushala_UpdateCurves(em);
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndFlyState(em, 0U);
        }
        break;
    }
}

/*
 * KUSH_FLY_LAND and KUSH_FLY_LAND_IN_COMBAT (variant 1, which ends at frame
 * 146): descends on a planned vertical speed, touches down (stance 0, a
 * camera shake) and goes to the idle.
 */
void EmKushala_Land(Em *em, EmWork *work, s32 variant)
{
    f32 excessHeight;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        em->rotX = 0;
        em->rotZ = 0;
        Em_SetMotion(em, KUSH_MOT_HOVER, 0, 0);
        Player_ClearVelocity(em);
        em->velY = -10.0f;
        excessHeight = em->floorY;
        excessHeight = em->posY - (1000.0f + excessHeight);
        if (excessHeight > 0.0f) {
            do {
                em->velY -= 0.75f;
                excessHeight += em->velY;
            } while (excessHeight > 0.0f && !(em->velY < -200.0f));
        }
        work->flyGuard = 0;
        work->moveAngX = 0;
        work->moveAngZ = 0;
        break;
    case 1:
        work->moveAngX = 0;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
        if (em->velY < -10.0f) {
            em->velY += 0.75f;
        }
        if (!(em->floorY < em->posY)) {
            em->velY = 0.0f;
            em->posY = em->floorY;
        }
        if (em->motion[0].playing == 0 && em->velY == 0.0f) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_LAND, 0, 0);
            em->rotX = 0;
            em->rotZ = 0;
            em->stance = EM_STANCE_GROUND;
            CamShake_ReqAIf(em, 1);
        }
        break;
    case 2:
        if (variant == 0) {
            if (em->motion[0].playing == 0) {
                em->step++;
                EmKushala_EndState(em, 0, 0);
            }
        } else if (EmMotion_ReachedFrame(em, 0, 146.0f)) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
    if (em->posY < em->floorY) {
        em->posY = em->floorY;
    }
}

/* KUSH_FLY_HOVER: one hover cycle in the air, then asks the script again. */
void EmKushala_Hover(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, KUSH_MOT_HOVER, 0, 0);
        work->flyGuard = 0;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndFlyState(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * KUSH_FLY_HOVER_GUARDED: hovers in place with the flight guard set (hits
 * can't flinch or kill it).
 */
void EmKushala_HoverGuarded(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, KUSH_MOT_FLY_FAST, 0, 0);
        Player_ClearAccel2(em);
        em->velY = 0.0f;
        em->velX = 0.0f;
        work->flyGuard = 1;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndFlyState(em, 1U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * KUSH_FLY_DRIFT_TO_GOAL: drifts level toward the goal at speed 20, turning
 * to it, until within ten frames' travel of it or out of time.
 */
void EmKushala_DriftToGoal(Em *em, EmWork *work)
{
    f32 dist;
    f32 floorY;
    s32 timer;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        work->turnSpeed = 256;
        Em_SetMotion(em, KUSH_MOT_HOVER, 0, 0);
        work->flyGuard = 0;
        break;
    case 1:
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 1);
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        work->moveAngX = 0;
        Em_MoveRotated(em, &work->moveAngX);
        dist = Vec_DistanceXZ(&em->posX, em->goal);
        timer = em->timer - 1;
        em->timer = timer;
        if ((timer <= 0) || (dist <= (10.0f * em->velZ))) {
            em->step++;
            Player_ClearVelocity(em);
            EmKushala_EndFlyState(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * KUSH_FLY_DRIFT_TO_GOAL_PITCHED: the same drift, facing the goal and
 * pitched toward it, by the distance in 3D.
 */
void EmKushala_DriftToGoalPitched(Em *em, EmWork *work)
{
    f32 dist;
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        work->turnSpeed = 256;
        Em_SetMotion(em, KUSH_MOT_HOVER, 0, 0);
        work->flyGuard = 0;
        break;
    case 1:
        work->moveAngY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        work->moveAngZ = 0;
        Em_GetPitchToGoal(em, &work->moveAngX, 0.0f, 0.0f);
        Em_MoveRotated(em, &work->moveAngX);
        dist = Vec_Distance((f32 *) &em->posX, (f32 *) em->goal);
        if (--em->timer <= 0 || dist <= (10.0f * em->velZ)) {
            em->step++;
            Player_ClearVelocity(em);
            EmKushala_EndFlyState(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * KUSH_FLY_SPIN_IN_PLACE: one full turn on the spot in the air, then asks
 * the script again.
 */
void EmKushala_SpinInPlace(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, KUSH_MOT_HOVER_TURN, 0, 0);
        em->lookAtTarget = 0;
        work->flyGuard = 0;
        break;
    case 1:
        if ((EmMotion_ReachedFrame(em, 0, 6.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 54.0f) == 0)) {
            em->rotY -= 0x555;
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndFlyState(em, 2U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * KUSH_FLY_TURN_TO_GOAL: hovers and turns to the goal for at most 150
 * frames.
 */
void EmKushala_TurnToGoal(Em *em, EmWork *work)
{
    f32 floorY;
    s32 done;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, KUSH_MOT_HOVER, 0, 0);
        work->turnSpeed = 384;
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        work->flyGuard = 0;
        em->timer = 150;
        break;
    case 1:
        done = Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 1) & 0xFF;
        if (--em->timer <= 0 || done != 0) {
            em->step++;
            EmKushala_EndFlyState(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * KUSH_FLY_LEAVE_AREA: dashes out through the area's exit
 * (Em_AreaExitPoint) and into the next area, then
 * KUSH_FLY_ENTER_AREA_AND_LAND if that is the goal's area, else
 * KUSH_FLY_CROSS_AREA.
 */
void EmKushala_LeaveArea(Em *em, EmWork *work)
{
    u16 destAreaId;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Player_ClearVelocity(em);
        Em_SetMotion(em, KUSH_MOT_DASH_START, 0, 0);
        EmKushala_StartCurves(em, 1);
        Em_AreaExitPoint(em, (f32 *) em->goal);
        work->flyGuard = 1;
        break;
    case 1:
        EmKushala_UpdateCurves(em);
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_FLY_FAST, 0, 0);
            Player_ClearVelocity(em);
            em->velZ = 80.0f;
        }
        break;
    case 2:
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 1249, &EmKushala_FlyParam);
        work->moveAngX = em->rotX;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
        if (!Area_ContainsPos(em->area, &em->posX)) {
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
        destAreaId = *(u16 *) &em->destArea;
        if (em->nextArea == destAreaId || destAreaId == AREA_NONE) {
            Hud_TriggerGlow(em);
            EmKushala_ChangeState(em, EM_GROUP_FLY, KUSH_FLY_ENTER_AREA_AND_LAND, EM_SYNC_SEND);
        } else {
            if (em->remote == 0) {
                em->nextArea = destAreaId;
                em->goalNo = destAreaId;
                em->goalKind = 3;
                Em_ComputeGoal(em, em->goal);
            }
            EmKushala_ChangeState(em, EM_GROUP_FLY, KUSH_FLY_CROSS_AREA, EM_SYNC_SEND);
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

/*
 * KUSH_FLY_ENTER_AREA_AND_LAND: flies to its point in the new area, brakes
 * and levels out, then lands.
 */
void EmKushala_EnterAreaAndLand(Em *em, EmWork *work)
{
    f32 goalDistXZ;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        work->turnSpeed = 256;
        Em_SetMotion(em, KUSH_MOT_FLY_FAST, 0, 0);
        Player_ClearVelocity(em);
        em->velZ = 80.0f;
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->rotY = em->aimAngle;
        *(u16 *) &em->destArea = AREA_NONE;
        work->flyGuard = 1;
        break;
    case 1:
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 229, &EmKushala_FlyParam);
        work->moveAngX = em->rotX;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
        goalDistXZ = Vec_DistanceXZ(&em->posX, em->goal);
        em->timer--;
        if (goalDistXZ <= 1000.0f || em->timer < 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_DASH_END, 0, 0);
            EmKushala_StartCurves(em, 2);
        }
        break;
    case 2:
        EmKushala_UpdateCurves(em);
        if (em->rotX != 0) {
            if (em->rotX < (ANGLE_TURN / 2)) {
                em->rotX -= work->pitchStep;
            } else {
                em->rotX += work->pitchStep;
            }
            em->rotX = (u16) em->rotX;
        }
        if (em->rotZ != 0) {
            if (em->rotZ < (ANGLE_TURN / 2)) {
                em->rotZ -= work->rollSpeed;
            } else {
                em->rotZ += work->rollSpeed;
            }
            em->rotZ = (u16) em->rotZ;
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 600;
            EmKushala_ChangeState(em, EM_GROUP_FLY, KUSH_FLY_LAND, EM_SYNC_SEND);
        }
        break;
    case 3:
        if (em->remote == 0) {
            em->timer = 600;
            EmKushala_ChangeState(em, EM_GROUP_FLY, KUSH_FLY_LAND, EM_SYNC_SEND);
        }
        break;
    }
    if (em->posY < em->floorY) {
        em->posY = em->floorY;
    }
}

/*
 * KUSH_FLY_CROSS_AREA: flies through an area on the way, from its entry
 * point to its exit, then on as EmKushala_LeaveArea does.
 */
void EmKushala_CrossArea(Em *em, EmWork *work)
{
    AreaRecord *area;
    f32 *entryPoint;
    u16 next;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        work->turnSpeed = 256;
        Em_SetMotion(em, KUSH_MOT_FLY_FAST, 0, 0);
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
        Em_UpdateFlight(em, 229, &EmKushala_FlyParam);
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
        Em_UpdateFlight(em, 225, &EmKushala_FlyParam);
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
        if (em->nextArea == next || next == AREA_NONE) {
            Hud_TriggerGlow(em);
            EmKushala_ChangeState(em, EM_GROUP_FLY, KUSH_FLY_ENTER_AREA_AND_LAND, EM_SYNC_SEND);
        } else {
            if (em->remote == 0) {
                em->nextArea = next;
                em->goalNo = next;
                em->goalKind = 3;
                Em_ComputeGoal(em, em->goal);
            }
            EmKushala_ChangeState(em, EM_GROUP_FLY, KUSH_FLY_CROSS_AREA, EM_SYNC_SEND);
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

/*
 * KUSH_FLY_DASH_TO_GOAL: a fast flight to the goal (Em_UpdateFlight), then
 * KUSH_FLY_DASH_BRAKE within 1000 of it or out of time.
 */
void EmKushala_DashToGoal(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        if (em->motionId[0] != EM_MOTION_ID(0, KUSH_MOT_DASH_START)) {
            Em_SetMotion(em, KUSH_MOT_DASH_START, 0, 0);
            EmKushala_StartCurves(em, 1U);
        } else {
            em->step++;
        }
        work->flyGuard = 1;
        break;
    case 1:
        EmKushala_UpdateCurves(em);
        if (em->motion[0].playing == 0) {
            em->step++;
            em->velX = 0.0f;
            em->velY = 0.0f;
            Player_ClearAccel2(em);
            em->velZ = 80.0f;
            Em_SetMotion(em, KUSH_MOT_FLY_FAST, 0, 0);
        }
        break;
    case 2:
        if (Vec_DistanceXZ(&em->posX, em->goal) <= 1000.0f || --em->timer <= 0) {
            em->step++;
            EmKushala_ChangeState(em, EM_GROUP_FLY, KUSH_FLY_DASH_BRAKE, EM_SYNC_SEND);
        }
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 289, &EmKushala_FlyParam2);
        work->moveAngX = em->rotX;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
        break;
    case 3:
        if (em->remote == 0) {
            EmKushala_ChangeState(em, EM_GROUP_FLY, KUSH_FLY_DASH_BRAKE, EM_SYNC_SEND);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * KUSH_FLY_DASH_BRAKE: brakes out of the dash, levelling pitch and roll,
 * then hovers.
 */
void EmKushala_BrakeDash(Em *em, EmWork *work)
{
    f32 floorY;
    s32 rotX;
    s32 rotZ;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_DASH_END, 0, 0);
        work->flyGuard = 1;
        EmKushala_StartCurves(em, 2U);
        break;
    case 1:
        EmKushala_UpdateCurves(em);
        rotX = em->rotX;
        if (rotX != 0) {
            if (rotX < (ANGLE_TURN / 2)) {
                em->rotX = rotX - work->pitchStep;
            } else {
                em->rotX = rotX + work->pitchStep;
            }
            em->rotX = (s32) (u16) em->rotX;
        }
        rotZ = em->rotZ;
        if (rotZ != 0) {
            if (rotZ < (ANGLE_TURN / 2)) {
                em->rotZ = rotZ - work->rollSpeed;
            } else {
                em->rotZ = rotZ + work->rollSpeed;
            }
            em->rotZ = (s32) (u16) em->rotZ;
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndFlyState(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * KUSH_FLY_DASH_CIRCUIT: flies a lap of the area's four points, legsLeft
 * counting the legs, then KUSH_FLY_HOVER_GUARDED.
 */
void EmKushala_DashCircuit(Em *em, EmWork *work)
{
    f32 floorY;
    s32 framesLeft;
    u8 n;
    u8 stateVar1;

    switch (em->step) {
    case 0:
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, KUSH_MOT_FLY_FAST, 0, 0);
        em->step++;
        em->stateVar1 = 0;
        em->lookAtTarget = 0;
        em->velX = 0.0f;
        em->velY = 0.0f;
        Player_ClearAccel2(em);
        if (em->velZ <= 80.0f) {
            em->velZ = 80.0f;
        }
        work->flyGuard = 1;
        break;
    case 1:
        stateVar1 = em->stateVar1;
        if ((stateVar1 == 0) && (em->motion[0].playing == 0)) {
            em->stateVar1 = stateVar1 + 1;
            Em_SetMotion(em, KUSH_MOT_DASH_END, 0, 0);
        }
        if ((Vec_DistanceXZ(&em->posX, em->goal) <= 3000.0f) || (framesLeft = em->timer - 1, em->timer = framesLeft, (framesLeft <= 0))) {
            n = ((EmKushalaWork *)work)->legsLeft - 1;
            ((EmKushalaWork *)work)->legsLeft = n;
            if ((n & 0xFF) <= 0) {
                EmKushala_EndFlyState(em, 1U);
            } else {
                em->curGoalNo++;
                em->curGoalNo &= 3;
                Em_ComputeGoal2(em, em->goal);
                em->timer = (s32) ((1.5f * Vec_DistanceXZ(&em->posX, em->goal)) / 50.0f);
                Em_SetStateRand(em, 2, 13);
            }
        }
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 225, &EmKushala_FlyParam);
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
 * KUSH_FLY_RISE_TO_AREA_HEIGHT, _FIXED_HEIGHT and _GOAL_HEIGHT (variants
 * 0-2): climbs until it reaches the area's ground height, a height fixed
 * per area, or the goal's height.
 */
void EmKushala_RiseToHeight(Em *em, EmWork *work, s32 variant)
{
    s32 reached = 0;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        work->turnSpeed = 256;
        Em_SetMotion(em, KUSH_MOT_HOVER, 0, 0);
        work->flyGuard = 0;
        Player_ClearVelocity(em);
        em->velY = 10.0f;
        break;
    case 1:
        Em_MoveRotated(em, &em->rotX);
        if (!(em->posY <= 500.0f + em->floorY) && em->velY < 100.0f) {
            em->velY += 0.75f;
        }
        switch (variant) {
        case 0:
            if (!(em->posY < gAreaGroundY[em->area])) {
                reached = 1;
            }
            break;
        case 1:
            switch (Area_DayId(em->area)) {
            case AREA_BATTLE_TOWN_1:
                if (!(em->posY < 2200.0f)) {
                    reached = 1;
                }
                break;
            case AREA_SNOWY_MOUNTAINS_3:
            case AREA_SNOWY_MOUNTAINS_6:
            default:
                if (!(em->posY < 4500.0f)) {
                    reached = 1;
                }
                break;
            }
            break;
        case 2:
            if (!(em->posY < em->goal[1])) {
                reached = 1;
            }
            break;
        }
        if (reached) {
            em->step++;
            Player_ClearVelocity(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndFlyState(em, 0);
        }
        break;
    }
    if (em->posY < em->floorY) {
        em->posY = em->floorY;
    }
}

/*
 * KUSH_ATK_TURN_SWEEP_A: a half turn on the ground (rotY +0x666 a frame
 * over frames 40-59) with attacks at frames 48 and 54.
 */
void EmKushala_TurnSweepA(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_TURN_SWEEP_A, 0, 0);
        break;
    case 1:
        if ((EmMotion_ReachedFrame(em, 0, 40.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 60.0f) == 0)) {
            em->rotY += 0x666;
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/* KUSH_ATK_TURN_SWEEP_B: the mirrored sweep. */
void EmKushala_TurnSweepB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_TURN_SWEEP_B, 0, 0);
        break;
    case 1:
        if ((EmMotion_ReachedFrame(em, 0, 40.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 60.0f) == 0)) {
            em->rotY -= 0x666;
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * KUSH_ATK_STRIKE_BREATH: an attack at frame 60; Kushala Daora adds a gust
 * (stage object and attack at frame 74, a breath from KUSH_BONE_BREATH at frame
 * 78).
 */
void EmKushala_StrikeBreath(Em *em)
{
    u8 type;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_STRIKE_BREATH, 0, 0);
        break;
    case 1:
        type = em->type;
        if ((type == EM_KIND_KUSHALA_DAORA) || (type == EM_KIND_RUSTED_KUSHALA_DAORA)) {
            if (EmMotion_PassedFrameScaled(em, 74.0f, 0U) != 0) {
                MonsterFx_CreateOnEmAngled(em, 10, 0, 0, 1.0f);
                EmKushala_CreateAttack(em, 27);
            }
            if (EmMotion_PassedFrameScaled(em, 78.0f, 0U) != 0) {
                EmKushala_CreateBreathAtBone(em, KUSH_BONE_BREATH, 0, 0, 0, 0);
            }
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * KUSH_ATK_BREATH_CHAIN: a chain of fire from the mouth (EMFX_BREATH_CHAIN)
 * at frame 104, then its end motion.
 */
void EmKushala_BreathChain(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 1;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_BREATH_CHAIN, 0, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 104.0f, 0U) != 0) {
            EmEffectAttack_CreateChain(em, EMFX_BREATH_CHAIN, 0, 0, 0);
        }
        if (em->motion[0].playing == 0) {
            if (--em->timer <= 0) {
                em->step++;
                Em_SetMotion(em, KUSH_MOT_BREATH_CHAIN_END, 0, 0);
                break;
            }
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * KUSH_ATK_TESCA_ACTION_A: plays its motion; the attacks and effects are
 * Lunastra's and Teostra's (EmKushala_MotionEffects).
 */
void EmKushala_TescaActionA(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_TESCA_ACTION_A, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * The blast rings (KUSH_ATK_BLAST_RING_A to _D; variant: the ring type):
 * charges for 90 frames, then blasts, with the ring
 * (TeostraBlastRing_Create) at frame 36.
 */
void EmKushala_BlastRing(Em *em, EmWork *work, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 100;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_BLAST_CHARGE, 0, 0);
        em->timer = 90;
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 56.0f, 0U) != 0) {
            TeostraBlastCharge_Create(em);
        }
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_BLAST, 0, 0);
        }
        break;
    case 2:
        if (EmMotion_PassedFrameScaled(em, 36.0f, 0U) != 0) {
            EmKushala_SpawnBlastBurst(em);
            TeostraBlastRing_Create(em, variant);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * KUSH_ATK_TESCA_ACTION_B: plays its motion; the effects are Lunastra's and
 * Teostra's.
 */
void EmKushala_TescaActionB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_TESCA_ACTION_B, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * KUSH_ATK_TESCA_ACTION_C: plays its motion and the one after it; the
 * effects are Lunastra's and Teostra's.
 */
void EmKushala_TescaActionC(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 100;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_TESCA_ACTION_C, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_TESCA_ACTION_C_END, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/* KUSH_ATK_HOVER_ACTION_A: plays its motion in the hover (stance 1 at once). */
void EmKushala_HoverActionA(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_HOVER_ACTION_A, 0, 0);
        em->stance = EM_STANCE_FAMILY;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * Moves the monster 15 forward along the work area's moveAngY (20 for
 * Lunastra and Teostra).
 */
void EmKushala_StepForward(Em *em, EmWork *work)
{
    f32 ofs[3];
    f32 out[3];
    s32 ang[3];
    f32 mtx[16];

    ang[0] = 0;
    ang[1] = work->moveAngY;
    ang[2] = 0;
    if (em->type == EM_KIND_LUNASTRA || em->type == EM_KIND_TEOSTRA) {
        Vec_Set(ofs, 0.0f, 0.0f, 20.0f);
    } else {
        Vec_Set(ofs, 0.0f, 0.0f, 15.0f);
    }
    Mtx_FromAnglesYXZ(ang, mtx);
    Vec_Rotate(out, ofs, mtx);
    em->posX += out[0];
    em->posZ += out[2];
}

/*
 * The breath streams (KUSH_ATK_BREATH_STREAM_ADVANCE, _WEAVE, _SPIN;
 * variants 0-2) in the hover: winds up, then breathes a stream while
 * stepping forward, weaving 45 degrees each way, or spinning in place.
 */
void EmKushala_BreathStream(Em *em, EmWork *work, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 1;
        Em_SetMotion(em, KUSH_MOT_BREATH_WINDUP, 0, 0);
        em->stance = EM_STANCE_FAMILY;
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            if (--em->timer <= 0) {
                em->step++;
                em->timer = 1;
                Em_SetMotion(em, KUSH_MOT_BREATH_STREAM, 0, 0);
                work->moveAngY = em->rotY;
                work->moveAngX = 0;
                break;
            }
        }
        break;
    case 2:
        switch (variant) {
        case 1:
            em->rotY = (s32) ((f32) work->moveAngY + (8192.0f * Math_Sin((6.2831855f * (f32) work->moveAngX) / 360.0f)));
            work->moveAngX += 5;
            
        case 0:
            EmKushala_StepForward(em, work);
            break;
        case 2:
            em->rotY += 0x1CA;
            break;
        }
        if (em->motion[0].playing == 0) {
            if (--em->timer <= 0) {
                em->step++;
                Em_SetMotion(em, KUSH_MOT_BREATH_STREAM_END, 0, 0);
                break;
            }
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * KUSH_ATK_LUNGE_BLAST: lunges at the goal (300 to 1000 away, over 20
 * frames) and makes an explosion ahead at frame 52.
 */
void EmKushala_LungeBlast(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 200;
        Em_SetMotion(em, KUSH_MOT_LUNGE, 0, 0);
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_FAMILY;
        Player_ClearVelocity(em);
        if (em->goalDist < 300.0f) {
            em->goalDist = 300.0f;
        }
        if (!(em->goalDist <= 1000.0f)) {
            em->goalDist = 1000.0f;
        }
        em->velZ = em->goalDist / 20.0f;
        break;
    case 1:
        work->moveAngX = 0;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
        em->goalDist -= em->velZ;
        if (EmMotion_ReachedFrame(em, 0, 40.0f) != 0) {
            em->step++;
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/* KUSH_ATK_HOVER_ACTION_B: plays its motion in the hover. */
void EmKushala_HoverActionB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_HOVER_ACTION_B, 0, 0);
        em->stance = EM_STANCE_FAMILY;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * KUSH_ATK_HOVER_SWEEP_A and _B (variant 0, 1): a half turn in the hover
 * over frames 50-80 with three attacks.
 */
void EmKushala_HoverSweep(Em *em, EmWork *work, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_FAMILY;
        em->lookAtTarget = 0;
        if (variant == 0) {
            Em_SetMotion(em, KUSH_MOT_HOVER_SWEEP_A, 0, 0);
            break;
        }
        Em_SetMotion(em, KUSH_MOT_HOVER_SWEEP_B, 0, 0);
        break;
    case 1:
        if (EmMotion_InFrameRange(em, 0, 50.0f, 80.0f) != 0) {
            if (variant == 0) {
                em->rotY += 0x444;
            } else {
                em->rotY -= 0x444;
            }
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * The breaths (KUSH_ATK_BREATH_QUICK to KUSH_ATK_AIR_BREATH_SINGLE):
 * variant & 0xF picks the pattern (EmKushala_BreathTimers,
 * EmKushala_BreathCounts), 0x10 means in the air. Winds up, breathes
 * (Kushala Daora's breath objects), finishes, and counts a hit on the fort
 * in the triple.
 */
void EmKushala_BreathAttack(Em *em, EmWork *work, s32 variant)
{
    EmKushalaWork *w;
    s32 k;

    switch (em->step) {
    case 0:
        em->step++;
        em->timer = EmKushala_BreathTimers[variant & 0xF][0];
        Em_SetMotion(em, KUSH_MOT_BREATH_WINDUP, 0, 0);
        if (variant & 0x10) {
            em->stance = EM_STANCE_AIR;
        } else {
            em->stance = EM_STANCE_FAMILY;
        }
        break;
    case 1:
        k = variant & 0xF;
        if (k == 0) {
            if (EmMotion_PassedFrameScaled(em, 40.0f, 0)) {
                em->step++;
                em->timer = EmKushala_BreathCounts[k][0];
                Em_SetMotion(em, KUSH_MOT_BREATH, 0, 0);
            }
        } else if (EmMotion_PassedFrameScaled(em, 106.0f, 0) && --em->timer <= 0) {
            em->step++;
            em->timer = EmKushala_BreathCounts[k][0];
            Em_SetMotion(em, KUSH_MOT_BREATH, 0, 0);
        }
        break;
    case 2:
        if (EmMotion_PassedFrameScaled(em, 20.0f, 0) && (em->type == EM_KIND_KUSHALA_DAORA || em->type == EM_KIND_RUSTED_KUSHALA_DAORA)) {
            switch (variant & 0xF) {
            case 0:
                MonsterFx_CreateOnEmAngled(em, 10, 8192, 0, 1.0f);
                EmKushala_CreateBreathAtBone(em, KUSH_BONE_BREATH, 2, 1, 8192, 0);
                break;
            case 1:
                switch (em->timer) {
                case 3:
                    MonsterFx_CreateOnEmAngled(em, 18, 5461, 61896, 1.0f);
                    EmKushala_CreateBreathAtBone(em, KUSH_BONE_BREATH, 1, 0, 5461, 61896);
                    break;
                case 2:
                    MonsterFx_CreateOnEmAngled(em, 18, 4551, 0, 1.0f);
                    EmKushala_CreateBreathAtBone(em, KUSH_BONE_BREATH, 1, 0, 4551, 0);
                    break;
                case 1:
                    MonsterFx_CreateOnEmAngled(em, 18, 5461, 3641, 1.0f);
                    EmKushala_CreateBreathAtBone(em, KUSH_BONE_BREATH, 1, 0, 5461, 3641);
                    break;
                }
                break;
            case 2:
                MonsterFx_CreateOnEmAngled(em, 10, 8192, 0, 1.0f);
                EmKushala_CreateBreathAtBone(em, KUSH_BONE_BREATH, 2, 2, 8192, 0);
                break;
            case 3:
                switch (em->timer) {
                case 3:
                    w = (EmKushalaWork *) work;
                    if (em->remote == 0 && w->fortHits < 10) {
                        w->fortHits++;
                        if (w->fortHits >= 10) {
                            Quest_TriggerEvent(em);
                        }
                        gScene.fortStage = w->fortHits;
                    }
                case 1:
                case 2:
                    MonsterFx_CreateOnEmAngled(em, 10, 4551, 0, 1.0f);
                    EmKushala_CreateBreathAtBone(em, KUSH_BONE_BREATH, 1, 0, 4551, 0);
                    break;
                }
                break;
            case 4:
                MonsterFx_CreateOnEmAngled(em, 10, 4551, 0, 1.0f);
                EmKushala_CreateBreathAtBone(em, KUSH_BONE_BREATH, 1, 0, 4551, 0);
                break;
            }
        }
        if (EmMotion_PassedFrameScaled(em, 46.0f, 0) && --em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_BREATH_FINISH, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            if (variant & 0x10) {
                EmKushala_EndFlyState(em, 0);
            } else {
                EmKushala_EndState(em, 0, 0);
            }
        }
        break;
    }
    if (em->posY < em->floorY) {
        em->posY = em->floorY;
    }
}

/*
 * KUSH_ATK_BLAST_RING_FORT: the blast ring (type 3) with a 150-frame
 * charge; each blast counts a hit on the fort (at 10 the quest's event).
 */
void EmKushala_BlastRingFort(Em *em, EmWork *work)
{
    EmKushalaWork *w = (EmKushalaWork *)work;

    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 100;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        em->timer = 150;
        if (em->motionId[0] == EM_MOTION_ID(0, KUSH_MOT_IDLE)) {
            Em_SetMotion(em, KUSH_MOT_WING_FLAP, 0, 0);
        } else {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_BLAST_CHARGE, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_BLAST_CHARGE, 0, 0);
        }
        break;
    case 2:
        if (EmMotion_PassedFrameScaled(em, 56.0f, 0)) {
            TeostraBlastCharge_Create(em);
        }
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_BLAST, 0, 0);
        }
        break;
    case 3:
        if (EmMotion_PassedFrameScaled(em, 36.0f, 0)) {
            EmKushala_SpawnBlastBurst(em);
            TeostraBlastRing_Create(em, 3);
            if (em->remote == 0 && w->fortHits <= 9) {
                w->fortHits++;
                if (w->fortHits >= 10) {
                    Quest_TriggerEvent(em);
                }
                gScene.fortStage = w->fortHits;
            }
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            if (em->combat == 1) {
                EmKushala_EndState(em, 0, 0);
            } else {
                Em_SetMotion(em, KUSH_MOT_LAND, 0, 146);
            }
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * KUSH_REACT_FLINCH (and KUSH_REACT_FIRST_BREAK_FLINCH): a flinch on the
 * ground, then ends the state.
 */
void EmKushala_Flinch(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, KUSH_MOT_FLINCH, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * KUSH_REACT_FLINCH_HEAVY: topples, recovers from frame 104, then ends the
 * state.
 */
void EmKushala_FlinchHeavy(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, KUSH_MOT_TOPPLE, 0, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 104.0f, 0U) != 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_TOPPLE_RECOVER, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/* KUSH_REACT_FLINCH_HOVER: a flinch in the hover, staying in stance 1. */
void EmKushala_FlinchHover(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_FAMILY;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, KUSH_MOT_FLINCH_HOVER, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * KUSH_REACT_KNOCK_DOWN: topples, lies for two passes of the lying motion,
 * gets up.
 */
void EmKushala_KnockDown(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, KUSH_MOT_TOPPLE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_DOWN, 0, 24);
            em->timer = 2;
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            if (--em->timer <= 0) {
                em->step++;
                Em_SetMotion(em, KUSH_MOT_DOWN_GET_UP, 0, 0);
                break;
            }
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/* KUSH_REACT_FALL: falls out of the air, lands, lies and gets up. */
void EmKushala_Fall(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, KUSH_MOT_FALL, 0, 0);
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
            Em_SetMotion(em, KUSH_MOT_FALL_LAND, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_DOWN, 0, 0);
            em->timer = 2;
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            if (--em->timer <= 0) {
                em->step++;
                Em_SetMotion(em, KUSH_MOT_DOWN_GET_UP, 0, 0);
                break;
            }
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    }
}

/*
 * KUSH_REACT_STATUS4: holds the status motion, its effect every 32 frames,
 * until the status ends, then KUSH_IDLE_STATUS4_SHAKE_OFF.
 */
void EmKushala_Status4(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_STATUS);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, KUSH_MOT_STATUS4, 0, 0);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmKushala_ChangeState(em, EM_GROUP_IDLE, KUSH_IDLE_STATUS4_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmKushala_ChangeState(em, EM_GROUP_IDLE, KUSH_IDLE_STATUS4_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
    Em_SpawnStatus4Effect(em, 2);
}

/*
 * KUSH_REACT_PART_BREAK: can't be hit for a moment, then turns once around
 * in the part-break motion.
 */
void EmKushala_PartBreak(Em *em)
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
        Em_SetMotion(em, KUSH_MOT_PART_BREAK, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_PART_BREAK_TURN, 0, 0);
        }
        break;
    case 2:
        if (EmMotion_InFrameRange(em, 0, 0.0f, 104.0f) != 0) {
            turnF = 630.0f * em->motionSpeed;
            turn = (u32) turnF;
            em->rotY -= (s32) turn & 0xFFFF;
        }
        if (em->motion[0].playing == 0) {
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/* KUSH_REACT_RETURN: ends the state at once. */
void EmKushala_ReactReturn(Em *em)
{
    EmKushala_EndState(em, 0, 0);
}

/* KUSH_REACT_AIR_KNOCK_DOWN: the same fall as EmKushala_Fall. */
void EmKushala_AirKnockDown(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, KUSH_MOT_FALL, 0, 0);
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
            Em_SetMotion(em, KUSH_MOT_FALL_LAND, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_DOWN, 0, 0);
            em->timer = 2;
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            if (--em->timer <= 0) {
                em->step++;
                Em_SetMotion(em, KUSH_MOT_DOWN_GET_UP, 0, 0);
                break;
            }
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    }
}

/* KUSH_REACT_FLASH_FALL: the fall, blinded every frame. */
void EmKushala_FlashFall(Em *em, EmWork *work)
{
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_FALL, 0, 0);
        Em_RestartScript(em);
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
            Em_SetMotion(em, KUSH_MOT_FALL_LAND, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_DOWN, 0, 0);
            em->timer = 2;
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            if (--em->timer <= 0) {
                em->step++;
                Em_SetMotion(em, KUSH_MOT_DOWN_GET_UP, 0, 0);
                break;
            }
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    }
}

/* KUSH_REACT_FLASH_FLINCH: the same flinch as EmKushala_Flinch. */
void EmKushala_FlashFlinch(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, KUSH_MOT_FLINCH, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * KUSH_REACT_HOVER_KNOCK_DOWN: drops from the hover onto the ground, lies
 * and gets up.
 */
void EmKushala_HoverKnockDown(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, KUSH_MOT_FALL_LAND, 0, 0);
        Player_ClearVelocity(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_DOWN, 0, 0);
            em->timer = 2;
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            if (--em->timer <= 0) {
                em->step++;
                Em_SetMotion(em, KUSH_MOT_DOWN_GET_UP, 0, 0);
                break;
            }
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    }
}

/*
 * KUSH_REACT_STUN: makes the stun's stage object, topples and lies until
 * the stun ends, gets up, then KUSH_IDLE_STUN_SHAKE_OFF.
 */
void EmKushala_Stun(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, KUSH_MOT_TOPPLE, 0, 0);
        StunStars_CreateEm(em, 1, 37, (s32) ((f32) em->timer + (em->motion[0].frameCount / 2.0f)), 150.0f, 3.0f);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_DOWN, 0, 24);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_DOWN_GET_UP, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKushala_ChangeState(em, EM_GROUP_IDLE, KUSH_IDLE_STUN_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 4:
        if (em->remote == 0) {
            EmKushala_ChangeState(em, EM_GROUP_IDLE, KUSH_IDLE_STUN_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/* KUSH_REACT_HOLD_RISE: held for 50 frames, then rears up into the hover. */
void EmKushala_HoldRise(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_REAR_UP, 0, 40);
        em->stance = EM_STANCE_GROUND;
        em->holdTimer = 50;
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            em->stance = EM_STANCE_FAMILY;
            Em_SetMotion(em, KUSH_MOT_LIFT_TO_HOVER, 0, 0);
        }
        break;
    case 2:
        if (em->motion[1].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_HOVER, 0, 28);
        }
        break;
    case 3:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * KUSH_REACT_HOLD_SETTLE: held for 50 frames, then rears up and settles on
 * the ground in the alert stance.
 */
void EmKushala_HoldSettle(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_REAR_UP, 0, 40);
        em->holdTimer = 50;
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_STOMP, 0, 0);
        }
        break;
    case 2:
        if (em->motion[1].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_ALERT, 0, 0);
        }
        break;
    case 3:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * KUSH_EVENT_LEAVE: staggers, takes off and climbs to height 20000, leaving
 * combat; the state never ends.
 */
void EmKushala_Leave(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_TOPPLE, 0, 0);
        work->flyGuard = 0;
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 104.0f, 0)) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_TOPPLE_RECOVER, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_TAKE_OFF, 0, 0);
            EmKushala_StartCurves(em, 0);
        }
        break;
    case 3:
        if (EmMotion_PassedFrameScaled(em, 48.0f, 0)) {
            em->step++;
            em->stance = EM_STANCE_AIR;
            EmKushala_UpdateCurves(em);
        }
        break;
    case 4:
        EmKushala_UpdateCurves(em);
        if (em->motion[0].playing == 0) {
            em->step++;
            work->turnSpeed = 256;
            Em_SetMotion(em, KUSH_MOT_HOVER, 0, 0);
            work->flyGuard = 0;
            Player_ClearVelocity(em);
            em->velY = 10.0f;
            Em_SetCombat(em, 0, 0);
        }
        break;
    case 5:
        Em_MoveRotated(em, &em->rotX);
        if (!(em->posY <= 500.0f + em->floorY) && em->velY < 30.0f) {
            em->velY += 0.75f;
        }
        if (!(em->posY < 20000.0f)) {
            em->step++;
            Player_ClearVelocity(em);
            em->velZ = 10.0f;
        }
        break;
    case 6:
        Em_MoveRotated(em, &em->rotX);
        break;
    }
    if (em->posY < em->floorY) {
        em->posY = em->floorY;
    }
}

/*
 * KUSH_EVENT_LEAVE_FROM_AIR: the climb of EmKushala_Leave, started in the
 * air.
 */
void EmKushala_LeaveFromAir(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        work->turnSpeed = 256;
        Em_SetMotion(em, KUSH_MOT_HOVER, 0, 0);
        work->flyGuard = 0;
        Player_ClearVelocity(em);
        em->velY = 10.0f;
        Em_SetCombat(em, 0, 0);
        break;
    case 1:
        Em_MoveRotated(em, &em->rotX);
        if (!(em->posY <= 500.0f + em->floorY) && em->velY < 30.0f) {
            em->velY += 0.75f;
        }
        if (!(em->posY < 20000.0f)) {
            em->step++;
            Player_ClearVelocity(em);
            em->velZ = 10.0f;
        }
        break;
    case 2:
        Em_MoveRotated(em, &em->rotX);
        break;
    }
    if (em->posY < em->floorY) {
        em->posY = em->floorY;
    }
}

/*
 * KUSH_EVENT_SCENE_WALK_IN: the scene in AREA_SNOWY_MOUNTAINS_6: placed by
 * hand, it walks in over two camera cuts, turns and waits for the scene's
 * event flag.
 */
void EmKushala_SceneWalkIn(Em *em)
{
    f32 goalDist;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        GmEm_ResetSize(em);
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, KUSH_MOT_IDLE, 0, 0);
        em->timer = 270;
        Player_ClearVelocity(em);
        em->posX = 9344.0f;
        em->posY = 305.0f;
        em->posZ = 9973.0f;
        em->rotY = 0xA400;
        em->goal[0] = 8061.0f;
        em->goal[1] = 314.0f;
        em->goal[2] = 8920.0f;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_WALK, 0, 0);
            em->timer = 1800;
        }
        break;
    case 2:
        goalDist = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer <= 0 || goalDist <= 100.0f) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_WALK, 0, 0);
            em->timer = 1800;
            em->posX = 12338.0f;
            em->posY = 291.0f;
            em->posZ = 11023.0f;
            em->rotY = ANGLE_TURN * 11 / 16;
            em->goal[0] = 9100.0f;
            em->goal[1] = 310.0f;
            em->goal[2] = 9683.0f;
        }
        break;
    case 3:
        goalDist = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer <= 0 || goalDist <= 100.0f) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_IDLE_ACTION_A, 0, 0);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_ROAR, 0, 0);
            em->rotY = ANGLE_TURN * 3 / 16;
        }
        break;
    case 5:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_ALERT, 0, 0);
        }
        break;
    case 6:
        if (em->motion[0].playing == 0 && Save_TestEventFlag(EVENT_FLAG_KUSHALA_SNOW_SCENE) == 1) {
            em->step++;
            em->senseDelay = 0;
            em->noHitTimer = 0;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * KUSH_EVENT_SCENE_RISE: the scene in AREA_VOLCANO_8: rises from below,
 * lands, roars and waits for the scene's event flag.
 */
void EmKushala_SceneRise(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        GmEm_ResetSize(em);
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, KUSH_MOT_HOVER, 0, 0);
        em->timer = 300;
        work->flyGuard = 0;
        Player_ClearVelocity(em);
        em->velY = 10.0f;
        em->posX = 9947.0f;
        em->posY = -3000.0f;
        em->posZ = 8326.0f;
        em->rotY = ANGLE_TURN / 2;
        em->goal[0] = 9947.0f;
        em->goal[1] = 0.0f;
        em->goal[2] = 8326.0f;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
        }
        break;
    case 2:
        Em_MoveRotated(em, &em->rotX);
        if (!(em->posY < em->goal[1])) {
            em->step++;
            em->timer = 170;
            em->posX = 9749.0f;
            em->posY = 0.0f;
            em->posZ = 8665.0f;
            em->rotY = ANGLE_TURN / 2;
        }
        break;
    case 3:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_LAND, 0, 0);
            em->posX = 9855.0f;
            em->posY = 0.0f;
            em->posZ = 4996.0f;
            em->rotY = ANGLE_TURN / 2;
        }
        break;
    case 4:
        if (EmMotion_ReachedFrame(em, 0, 146.0f)) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_BLAST, 0, 100);
        }
        break;
    case 5:
        if (EmMotion_ReachedFrame(em, 0, 170.0f)) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_BLAST, 0, 100);
        }
        break;
    case 6:
        if (EmMotion_ReachedFrame(em, 0, 200.0f)) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_ALERT, 0, 0);
        }
        break;
    case 7:
        if (em->motion[0].playing == 0 && Save_TestEventFlag(EVENT_FLAG_LUNASTRA_TEOSTRA_VOLCANO_8_SCENE) == 1) {
            em->step++;
            em->senseDelay = 0;
            em->noHitTimer = 0;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * KUSH_EVENT_CAPTURED: held and unhittable, lies captured (QuestEm_Captured
 * once) with the lying puffs.
 */
void EmKushala_Captured(Em *em)
{
    f32 pos[3];

    Em_SetCaptured(em, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_DOWNED, 0, 0);
        Em_LoadReactionSet(em, 1);
        Em_BeginStatus1(em);
        Em_ClearPoison(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            QuestEm_Captured(em);
        }
        break;
    case 2:
        break;
    }
    pos[1] = 10.0f;
    pos[2] = 140.0f;
    pos[0] = 0.0f;
    Em_SpawnLyingPuffs(em, 34, pos, 1.6f);
}

/*
 * KUSH_EVENT_SCENE_DESCEND: the scene in AREA_VOLCANO_7: descends, lands,
 * blasts and breathes the chain of fire, then waits for the scene's event
 * flag.
 */
void EmKushala_SceneDescend(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        GmEm_ResetSize(em);
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, KUSH_MOT_HOVER, 0, 0);
        em->timer = 200;
        work->flyGuard = 0;
        Player_ClearVelocity(em);
        em->velY = -10.0f;
        em->posX = 9329.0f;
        em->posY = 3000.0f;
        em->posZ = 9973.0f;
        em->rotY = 0xFC00;
        em->goal[0] = 9329.0f;
        em->goal[1] = 100.0f;
        em->goal[2] = 9973.0f;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
        }
        break;
    case 2:
        Em_MoveRotated(em, &em->rotX);
        if (em->posY <= em->goal[1]) {
            em->step++;
            em->timer = 170;
            em->posX = 9329.0f;
            em->posY = 100.0f;
            em->posZ = 9973.0f;
            em->rotY = 0xFC00;
        }
        break;
    case 3:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotionRow(em, KUSH_MOT_LAND, 0, 0);
            em->posX = 9329.0f;
            em->posY = 0.0f;
            em->posZ = 9973.0f;
            em->rotY = 0xFC00;
        }
        break;
    case 4:
        if (EmMotion_ReachedFrame(em, 0, 146.0f)) {
            em->step++;
            Em_SetMotionRow(em, KUSH_MOT_BLAST, 0, 0);
            em->stance = EM_STANCE_GROUND;
        }
        break;
    case 5:
        if (EmMotion_PassedFrameScaled(em, 36.0f, 0)) {
            EmKushala_SpawnBlastBurst(em);
        }
        if (EmMotion_ReachedFrame(em, 0, 200.0f)) {
            em->step++;
            Em_SetMotionRow(em, KUSH_MOT_BREATH_CHAIN, 0, 0);
        }
        break;
    case 6:
        if (EmMotion_PassedFrameScaled(em, 104.0f, 0)) {
            EmEffectAttack_CreateChain(em, EMFX_BREATH_CHAIN, 0, 0, 0);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_BREATH_CHAIN_END, 0, 0);
        }
        break;
    case 7:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_ALERT, 0, 0);
        }
        break;
    case 8:
        if (Save_TestEventFlag(EVENT_FLAG_LUNASTRA_TEOSTRA_VOLCANO_7_SCENE) == 1) {
            em->step++;
            em->senseDelay = 0;
            em->noHitTimer = 0;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * KUSH_EVENT_SCENE_FLY_IN: the scene in AREA_JUNGLE_3: glides in, descends
 * and hovers, then waits for the scene's event flag.
 */
void EmKushala_SceneFlyIn(Em *em, EmWork *work)
{
    f32 goalDist;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        GmEm_ResetSize(em);
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, KUSH_MOT_FLY_FAST, 0, 0);
        work->flyGuard = 0;
        Player_ClearVelocity(em);
        em->velZ = 80.0f;
        em->posX = 7146.0f;
        em->posY = 2000.0f;
        em->posZ = 11034.0f;
        em->rotY = ANGLE_TURN * 11 / 32;
        em->goal[0] = 9740.0f;
        em->goal[1] = 2000.0f;
        em->goal[2] = 9360.0f;
        em->timer = 180;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            em->timer = 300;
        }
        break;
    case 2:
        Em_MoveRotated(em, &em->rotX);
        goalDist = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer <= 0 || goalDist <= 100.0f) {
            em->step++;
            em->posX = 9740.0f;
            em->posY = 2000.0f;
            em->posZ = 9360.0f;
            em->rotY = ANGLE_TURN * 3 / 16;
            em->goal[0] = 13806.0f;
            em->goal[1] = 2000.0f;
            em->goal[2] = 11045.0f;
            em->timer = 300;
        }
        break;
    case 3:
        Em_MoveRotated(em, &em->rotX);
        goalDist = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer <= 0 || goalDist <= 100.0f) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_HOVER, 0, 0);
            em->posX = 13917.0f;
            em->posY = 2000.0f;
            em->posZ = 10375.0f;
            em->rotY = ANGLE_TURN * 11 / 16;
            Player_ClearVelocity(em);
            em->velY = -10.0f;
        }
        break;
    case 4:
        Em_MoveRotated(em, &em->rotX);
        if (em->posY <= 500.0f) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_HOVER, 0, 0);
            em->posX = 13917.0f;
            em->posY = 2000.0f;
            em->posZ = 10375.0f;
            em->rotY = ANGLE_TURN * 11 / 16;
            Player_ClearVelocity(em);
            em->velY = -10.0f;
        }
        break;
    case 5:
        Em_MoveRotated(em, &em->rotX);
        if (em->posY <= 500.0f) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_DASH_START, 0, 0);
            EmKushala_StartCurves(em, 1);
            em->posX = 13690.0f;
            em->posY = 200.0f;
            em->posZ = 10845.0f;
            em->rotY = ANGLE_TURN * 11 / 16;
        }
        break;
    case 6:
        EmKushala_UpdateCurves(em);
        if (em->motion[0].playing == 0) {
            em->step++;
            em->velX = 0.0f;
            em->velY = 0;
            Player_ClearAccel2(em);
            em->velZ = 80.0f;
            Em_SetMotion(em, KUSH_MOT_FLY_FAST, 0, 0);
            em->rotY = 0xA400;
            em->goal[0] = 8488.0f;
            em->goal[1] = 200.0f;
            em->goal[2] = 9339.0f;
            em->rotY = Math_AngleXZ(&em->posX, em->goal);
            em->timer = 300;
        }
        break;
    case 7:
        Em_MoveRotated(em, &em->rotX);
        goalDist = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer <= 0 || goalDist <= 100.0f) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_DASH_END, 0, 0);
            EmKushala_StartCurves(em, 2);
        }
        break;
    case 8:
        EmKushala_UpdateCurves(em);
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_HOVER, 0, 0);
        }
        break;
    case 9:
        if (Save_TestEventFlag(EVENT_FLAG_KUSHALA_JUNGLE_SCENE) == 1) {
            em->senseDelay = 0;
            em->noHitTimer = 0;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * KUSH_EVENT_SCENE_WAKE: the scene in AREA_TOWER_0x7C: lies, gets up and
 * blasts, then waits for the scene's event flag.
 */
void EmKushala_SceneWake(Em *em)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        GmEm_ResetSize(em);
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, KUSH_MOT_REST, 0, 0);
        em->timer = 180;
        Player_ClearVelocity(em);
        em->posX = 11600.0f;
        em->posY = -20.0f;
        em->posZ = 13000.0f;
        em->rotY = ANGLE_TURN * 13 / 32;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            em->timer = 2;
        }
        break;
    case 2:
        if (em->motion[0].playing == 0 && --em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_REST_END, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_BLAST_CHARGE, 0, 0);
            em->timer = 2;
            em->posX = 10018.0f;
            em->posY = 0.0f;
            em->posZ = 11470.0f;
            em->rotY = ANGLE_TURN * 11 / 16;
        }
        break;
    case 4:
        if (EmMotion_PassedFrameScaled(em, 56.0f, 0)) {
            TeostraBlastCharge_Create(em);
        }
        if (em->motion[0].playing == 0 && --em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, KUSH_MOT_BLAST, 0, 100);
        }
        break;
    case 5:
        if (EmMotion_ReachedFrame(em, 0, 230.0f)) {
            em->step++;
        }
        break;
    case 6:
        if (Save_TestEventFlag(EVENT_FLAG_LUNASTRA_TEOSTRA_TOWER_SCENE) == 1) {
            em->step++;
            em->senseDelay = 0;
            em->noHitTimer = 0;
            EmKushala_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * KUSH_DEFEAT_DIE: the death motion (a camera shake at frame 414),
 * QuestEm_Hunted, the carcass's mine point for 1800 frames, then a
 * 150-frame fade and KUSH_DEFEAT_REAPPEAR.
 */
void EmKushala_Die(Em *em, EmWork *work)
{
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, KUSH_MOT_DIE, 0, 0);
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_DEAD;
        ((EmKushalaWork *)work)->windOn = 0;
        QuestEm_Hunted(em);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 414.0f, 0)) {
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
        if (QuestEm_CanAppear(em) && --em->timer <= 0) {
            em->step++;
            em->timer = 150;
            EmMine_Remove(em);
        }
        break;
    case 3:
        if (em->timer > 0) {
            em->timer--;
        }
        if (em->timer <= 0) {
            if (em->remote == 0) {
                EmKushala_ChangeState(em, EM_GROUP_DEFEAT, KUSH_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    }
}

/*
 * KUSH_DEFEAT_DIE_FALLING: falls to the floor, then dies as EmKushala_Die
 * does.
 */
void EmKushala_DieFalling(Em *em, EmWork *work)
{
    s32 savedTimer;

    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_SetMotion(em, KUSH_MOT_FALL, 0, 0);
        ((EmKushalaWork *)work)->windOn = 0;
        if (!(em->velY <= -50.0f)) {
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
            Em_SetMotion(em, KUSH_MOT_FALL_LAND, 0, 0);
            Player_ClearVelocity(em);
            em->stance = EM_STANCE_DEAD;
            QuestEm_Hunted(em);
        }
        break;
    case 2:
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
        savedTimer = em->timer;
        if (savedTimer <= 0) {
            if (em->remote == 0) {
                EmKushala_ChangeState(em, EM_GROUP_DEFEAT, KUSH_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
            }
        } else {
            em->alpha = (f32) savedTimer / 150.0f;
        }
        break;
    case 99:
        if (em->motion[0].playing == 0) {
            em->stance = EM_STANCE_GROUND;
            EmKushala_EndState(em, 0, 0);
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
        }
        break;
    }
}

/*
 * KUSH_DEFEAT_REAPPEAR: comes back to the idle if the quest lets it appear
 * again (QuestEm_CanAppear), else ends the task.
 */
void EmKushala_Reappear(Em *em)
{
    switch (em->step) {
    case 0:
        if (QuestEm_CanAppear(em) == 1) {
            EmKushala_ChangeState(em, EM_GROUP_IDLE, KUSH_IDLE, EM_SYNC_LOCAL);
            break;
        }
        em->taskPhase++;
    case 1:
        break;
    }
}

/*
 * Runs the current group 0 (idle) state's function; some functions serve
 * several states, told apart by a variant.
 */
void EmKushala_RunIdleState(Em *em, EmWork *work)
{
    switch (em->state) {
    case KUSH_IDLE_STAND:
        EmKushala_Stand(em);
        break;
    case KUSH_IDLE:
        EmKushala_Idle(em);
        break;
    case KUSH_IDLE_HOVER:
        EmKushala_HoverIdle(em);
        break;
    case KUSH_IDLE_ALERT:
        EmKushala_AlertIdle(em);
        break;
    case KUSH_IDLE_EAT_NEED1_START:
        EmKushala_EatNeedStart(em, work, 0);
        break;
    case KUSH_IDLE_EAT_NEED1:
        EmKushala_EatNeed(em, work, 0);
        break;
    case KUSH_IDLE_ACTION_A:
        EmKushala_IdleActionA(em);
        break;
    case KUSH_IDLE_STATUS4_SHAKE_OFF:
        EmKushala_StatusShakeOff(em, work, 0);
        break;
    case KUSH_IDLE_SHAKE_OFF:
        EmKushala_ShakeOff(em);
        break;
    case KUSH_IDLE_REST:
        EmKushala_Rest(em);
        break;
    case KUSH_IDLE_REST_END:
        EmKushala_RestEnd(em);
        break;
    case KUSH_IDLE_STATUS2_COLLAPSE:
        EmKushala_Status2Collapse(em);
        break;
    case KUSH_IDLE_STATUS2_END:
        EmKushala_Status2End(em);
        break;
    case KUSH_IDLE_EAT_START:
        EmKushala_EatStart(em);
        break;
    case KUSH_IDLE_EAT:
        EmKushala_Eat(em);
        break;
    case KUSH_IDLE_WIND_ROAR:
        EmKushala_Roar(em, work);
        break;
    case KUSH_IDLE_WIND_ROAR_B:
        EmKushala_Roar(em, work);
        break;
    case KUSH_IDLE_WIND_ROAR_C:
        EmKushala_Roar(em, work);
        break;
    case KUSH_IDLE_ACTION_B:
        EmKushala_IdleActionB(em);
        break;
    case KUSH_IDLE_ALERT_B:
        EmKushala_AlertIdleB(em);
        break;
    case KUSH_IDLE_WIND_ROAR_OR_SWEEP:
        EmKushala_Roar(em, work);
        break;
    case KUSH_IDLE_STATUS1_COLLAPSE:
        EmKushala_Status1Collapse(em);
        break;
    case KUSH_IDLE_STATUS1_END:
        EmKushala_Status1End(em);
        break;
    case KUSH_IDLE_SETTLE:
        EmKushala_Settle(em);
        break;
    case KUSH_IDLE_STUN_SHAKE_OFF:
        EmKushala_StatusShakeOff(em, work, 1);
        break;
    case KUSH_IDLE_EAT_NEED2_START:
        EmKushala_EatNeedStart(em, work, 1);
        break;
    case KUSH_IDLE_EAT_NEED2:
        EmKushala_EatNeed(em, work, 1);
        break;
    case KUSH_IDLE_ROAR:
        EmKushala_RoarB(em, work);
        break;
    }
}

/* Runs the current group 1 (move) state's function. */
void EmKushala_RunMoveState(Em *em, EmWork *work)
{
    switch (em->state) {
    case KUSH_MOVE_WALK:
        EmKushala_Walk(em);
        break;
    case KUSH_MOVE_LIMP:
        EmKushala_Limp(em);
        break;
    case KUSH_MOVE_TURN:
        EmKushala_Turn(em);
        break;
    case KUSH_MOVE_TURN_ALERT:
        EmKushala_TurnAlert(em, work);
        break;
    case KUSH_MOVE_WALK_B:
        EmKushala_WalkB(em);
        break;
    case KUSH_MOVE_RUN:
        EmKushala_Run(em, work, 0);
        break;
    case KUSH_MOVE_STOMP:
        EmKushala_Stomp(em);
        break;
    case KUSH_MOVE_LIFT_TO_HOVER:
        EmKushala_LiftToHover(em);
        break;
    case KUSH_MOVE_RUN_PAST:
        EmKushala_Run(em, work, 0);
        break;
    case KUSH_MOVE_RUN_PAST_ARC_A:
        EmKushala_Run(em, work, 1);
        break;
    case KUSH_MOVE_RUN_PAST_ARC_B:
        EmKushala_Run(em, work, 2);
        break;
    case KUSH_MOVE_RUN_PAST_ARC_C:
        EmKushala_Run(em, work, 3);
        break;
    case KUSH_MOVE_RUN_PAST_ARC_D:
        EmKushala_Run(em, work, 4);
        break;
    case KUSH_MOVE_TAKE_OFF_TO_HOVER:
        EmKushala_TakeOffToHover(em);
        break;
    case KUSH_MOVE_SIDE_DASH_A:
        EmKushala_SideDash(em, work, 0);
        break;
    case KUSH_MOVE_SIDE_DASH_B:
        EmKushala_SideDash(em, work, 1);
        break;
    case KUSH_MOVE_HOVER_FORWARD:
        EmKushala_HoverForward(em, work, 0);
        break;
    case KUSH_MOVE_HOVER_DASH:
        EmKushala_HoverDash(em, work);
        break;
    case KUSH_MOVE_HOVER_TURN:
        EmKushala_HoverTurn(em, work);
        break;
    case KUSH_MOVE_LAND:
        EmKushala_LandFromHover(em, work, 0);
        break;
    case KUSH_MOVE_HOVER_SPIN:
        EmKushala_HoverSpin(em);
        break;
    case KUSH_MOVE_HOVER_FORWARD_PAST:
        EmKushala_HoverForward(em, work, 0);
        break;
    case KUSH_MOVE_HOVER_BACKWARD:
        EmKushala_HoverForward(em, work, 1);
        break;
    case KUSH_MOVE_LAND_QUICK:
        EmKushala_LandFromHover(em, work, 1);
        break;
    }
}

/* Runs the current group 2 (fly) state's function. */
void EmKushala_RunFlyState(Em *em, EmWork *work)
{
    switch (em->state) {
    case KUSH_FLY_TAKE_OFF:
        EmKushala_TakeOff(em, work);
        break;
    case KUSH_FLY_LAND:
        EmKushala_Land(em, work, 0);
        break;
    case KUSH_FLY_HOVER:
        EmKushala_Hover(em, work);
        break;
    case KUSH_FLY_HOVER_GUARDED:
        EmKushala_HoverGuarded(em, work);
        break;
    case KUSH_FLY_DRIFT_TO_GOAL:
        EmKushala_DriftToGoal(em, work);
        break;
    case KUSH_FLY_DRIFT_TO_GOAL_PITCHED:
        EmKushala_DriftToGoalPitched(em, work);
        break;
    case KUSH_FLY_SPIN_IN_PLACE:
        EmKushala_SpinInPlace(em, work);
        break;
    case KUSH_FLY_TURN_TO_GOAL:
        EmKushala_TurnToGoal(em, work);
        break;
    case KUSH_FLY_LEAVE_AREA:
        EmKushala_LeaveArea(em, work);
        break;
    case KUSH_FLY_ENTER_AREA_AND_LAND:
        EmKushala_EnterAreaAndLand(em, work);
        break;
    case KUSH_FLY_CROSS_AREA:
        EmKushala_CrossArea(em, work);
        break;
    case KUSH_FLY_DASH_TO_GOAL:
        EmKushala_DashToGoal(em, work);
        break;
    case KUSH_FLY_DASH_BRAKE:
        EmKushala_BrakeDash(em, work);
        break;
    case KUSH_FLY_DASH_CIRCUIT:
        EmKushala_DashCircuit(em, work);
        break;
    case KUSH_FLY_LAND_IN_COMBAT:
        EmKushala_Land(em, work, 1);
        break;
    case KUSH_FLY_RISE_TO_AREA_HEIGHT:
        EmKushala_RiseToHeight(em, work, 0);
        break;
    case KUSH_FLY_RISE_TO_FIXED_HEIGHT:
        EmKushala_RiseToHeight(em, work, 1);
        break;
    case KUSH_FLY_RISE_TO_GOAL_HEIGHT:
        EmKushala_RiseToHeight(em, work, 2);
        
    default:
        break;
    }
}

/* Runs the current group 3 (attack) state's function. */
void EmKushala_RunAttackState(Em *em, EmWork *work)
{
    switch (em->state) {
    case KUSH_ATK_TURN_SWEEP_A:
        EmKushala_TurnSweepA(em);
        break;
    case KUSH_ATK_TURN_SWEEP_B:
        EmKushala_TurnSweepB(em);
        break;
    case KUSH_ATK_STRIKE_BREATH:
        EmKushala_StrikeBreath(em);
        break;
    case KUSH_ATK_BREATH_CHAIN:
        EmKushala_BreathChain(em);
        break;
    case KUSH_ATK_TESCA_ACTION_A:
        EmKushala_TescaActionA(em);
        break;
    case KUSH_ATK_BLAST_RING_A:
        EmKushala_BlastRing(em, work, 0);
        break;
    case KUSH_ATK_TESCA_ACTION_B:
        EmKushala_TescaActionB(em);
        break;
    case KUSH_ATK_BLAST_RING_B:
        EmKushala_BlastRing(em, work, 1);
        break;
    case KUSH_ATK_TESCA_ACTION_C:
        EmKushala_TescaActionC(em);
        break;
    case KUSH_ATK_HOVER_ACTION_A:
        EmKushala_HoverActionA(em);
        break;
    case KUSH_ATK_BREATH_STREAM_ADVANCE:
        EmKushala_BreathStream(em, work, 0);
        break;
    case KUSH_ATK_LUNGE_BLAST:
        EmKushala_LungeBlast(em, work);
        break;
    case KUSH_ATK_HOVER_ACTION_B:
        EmKushala_HoverActionB(em);
        break;
    case KUSH_ATK_HOVER_SWEEP_A:
        EmKushala_HoverSweep(em, work, 0);
        break;
    case KUSH_ATK_HOVER_SWEEP_B:
        EmKushala_HoverSweep(em, work, 1);
        break;
    case KUSH_ATK_BREATH_QUICK:
        EmKushala_BreathAttack(em, work, 0);
        break;
    case KUSH_ATK_BREATH_FAN:
        EmKushala_BreathAttack(em, work, 1);
        break;
    case KUSH_ATK_BREATH_FULL:
        EmKushala_BreathAttack(em, work, 2);
        break;
    case KUSH_ATK_AIR_BREATH_QUICK:
        EmKushala_BreathAttack(em, work, 16);
        break;
    case KUSH_ATK_AIR_BREATH_FAN:
        EmKushala_BreathAttack(em, work, 17);
        break;
    case KUSH_ATK_AIR_BREATH_FULL:
        EmKushala_BreathAttack(em, work, 18);
        break;
    case KUSH_ATK_BREATH_STREAM_WEAVE:
        EmKushala_BreathStream(em, work, 1);
        break;
    case KUSH_ATK_BREATH_STREAM_SPIN:
        EmKushala_BreathStream(em, work, 2);
        break;
    case KUSH_ATK_AIR_BREATH_TRIPLE:
        EmKushala_BreathAttack(em, work, 19);
        break;
    case KUSH_ATK_BLAST_RING_FORT:
        EmKushala_BlastRingFort(em, work);
        break;
    case KUSH_ATK_AIR_BREATH_SINGLE:
        EmKushala_BreathAttack(em, work, 20);
        break;
    case KUSH_ATK_BLAST_RING_C:
        EmKushala_BlastRing(em, work, 2);
        break;
    case KUSH_ATK_BLAST_RING_D:
        EmKushala_BlastRing(em, work, 3);
        break;
    }
}

/*
 * Runs the current group 4 (react) state's function; a state it doesn't
 * list runs the flinch.
 */
void EmKushala_RunReactState(Em *em, EmWork *work)
{
    switch (em->state) {
    default:
        EmKushala_Flinch(em);
        break;
    case KUSH_REACT_FLINCH:
        EmKushala_Flinch(em);
        break;
    case KUSH_REACT_FLINCH_HEAVY:
        EmKushala_FlinchHeavy(em);
        break;
    case KUSH_REACT_FLINCH_HOVER:
        EmKushala_FlinchHover(em);
        break;
    case KUSH_REACT_KNOCK_DOWN:
        EmKushala_KnockDown(em);
        break;
    case KUSH_REACT_FALL:
        EmKushala_Fall(em, work);
        break;
    case KUSH_REACT_STATUS4:
        EmKushala_Status4(em);
        break;
    case KUSH_REACT_PART_BREAK:
        EmKushala_PartBreak(em);
        break;
    case KUSH_REACT_RETURN:
        EmKushala_ReactReturn(em);
        break;
    case KUSH_REACT_AIR_KNOCK_DOWN:
        EmKushala_AirKnockDown(em, work);
        break;
    case KUSH_REACT_FLASH_FALL:
        EmKushala_FlashFall(em, work);
        break;
    case KUSH_REACT_FLASH_FLINCH:
        EmKushala_FlashFlinch(em);
        break;
    case KUSH_REACT_HOVER_KNOCK_DOWN:
        EmKushala_HoverKnockDown(em);
        break;
    case KUSH_REACT_STUN:
        EmKushala_Stun(em);
        break;
    case KUSH_REACT_HOLD_RISE:
        EmKushala_HoldRise(em);
        break;
    case KUSH_REACT_HOLD_SETTLE:
        EmKushala_HoldSettle(em);
        break;
    case KUSH_REACT_FIRST_BREAK_FLINCH:
        EmKushala_Flinch(em);
        break;
    }
}

/* Runs the current group 5 (defeat) state's function. */
void EmKushala_RunDefeatState(Em *em, EmWork *work)
{
    switch (em->state) {
    case KUSH_DEFEAT_DIE:
        EmKushala_Die(em, work);
        break;
    case KUSH_DEFEAT_DIE_B:
        EmKushala_Die(em, work);
        break;
    case KUSH_DEFEAT_DIE_FALLING:
        EmKushala_DieFalling(em, work);
        break;
    case KUSH_DEFEAT_REAPPEAR:
        EmKushala_Reappear(em);
        break;
    }
}

/* Runs the current group 6 or 7 (event) state's function. */
void EmKushala_RunEventState(Em *em, EmWork *work)
{
    switch (em->state) {
    case KUSH_EVENT_LEAVE:
        EmKushala_Leave(em, work);
        break;
    case KUSH_EVENT_LEAVE_FROM_AIR:
        EmKushala_LeaveFromAir(em, work);
        break;
    case KUSH_EVENT_SCENE_WALK_IN:
        EmKushala_SceneWalkIn(em);
        break;
    case KUSH_EVENT_SCENE_RISE:
        EmKushala_SceneRise(em, work);
        break;
    case KUSH_EVENT_CAPTURED:
        EmKushala_Captured(em);
        break;
    case KUSH_EVENT_SCENE_DESCEND:
        EmKushala_SceneDescend(em, work);
        break;
    case KUSH_EVENT_SCENE_FLY_IN:
        EmKushala_SceneFlyIn(em, work);
        break;
    case KUSH_EVENT_SCENE_WAKE:
        EmKushala_SceneWake(em);
        
    default:
        break;
    }
}

/*
 * The family's per-frame update: the wind's hitzones and marker, the needs
 * and the low-hp check (outside the arenas), Em_UpdateDamage's result as a
 * reaction, a status or a death, the story scenes by quest stage, then the
 * state (twice when a state change is pending).
 */
void EmKushala_Update(Em *em)
{
    u8 buf[4];
    f32 ratio;
    EmKushalaWork *w = (EmKushalaWork *)&em->work;
    s32 damage;
    u16 k;

    Em_UpdateCombatTimer(em);
    Em_RaisePlayerAreaTriggerLax(em);
    if (em->type == EM_KIND_KUSHALA_DAORA || em->type == EM_KIND_RUSTED_KUSHALA_DAORA) {
        ratio = 0.12f;
        em->hitzoneSet = 0;
        em->windTimer = 0;
        if (EmKushala_WindActive(em)) {
            em->hitzoneSet = 1;
            em->windTimer = 2;
        }
    } else {
        if (em->type == EM_KIND_LUNASTRA) {
            ratio = 0.15f;
        } else {
            ratio = 0.2f;
        }
        em->hitzoneSet = 0;
        em->windTimer = 0;
        if (EmKushala_WindActiveTesca(em)) {
            em->hitzoneSet = 1;
            em->windTimer = 2;
        }
    }
    k = Map_DayId(gScene.map);
    if (em->noAlert != 1 && em->rage == 0 && k != 8 && k != 9 && k != 10) {
        if (em->type == EM_KIND_KUSHALA_DAORA || em->type == EM_KIND_RUSTED_KUSHALA_DAORA) {
            switch (k) {
            case MAP_JUNGLE:
                Em_CheckLowHp(em, ratio);
                Em_DecayNeed2(em);
                Em_DecayNeed1(em);
                Em_DecayNeed3(em);
                break;
            case MAP_DESERT:
                Em_DecayNeed2(em);
                Em_CheckLowHp(em, ratio);
                break;
            case MAP_SNOWY_MOUNTAINS:
                Em_CheckLowHp(em, ratio);
                break;
            }
        } else {
            switch (k) {
            case MAP_DESERT:
                Em_CheckLowHp(em, ratio);
                break;
            case MAP_VOLCANO:
                Em_CheckLowHp(em, ratio);
                Em_DecayNeed1(em);
                break;
            case MAP_SWAMP:
                Em_CheckLowHp(em, ratio);
                break;
            }
        }
    }
    damage = Em_UpdateDamage(em, buf) & 0xFF;
    switch (damage) {
    case EM_DMG_NONE:
    case EM_DMG_FLASH:
    case EM_DMG_HIT:
    case EM_DMG_ATTACK5:
    case EM_DMG_ATTACK5_FLINCH:
        break;
    case EM_DMG_KILLED:
    case EM_DMG_DEAD:
        switch (gQuest.stage) {
        case QUEST_ID_KUSHALA_JUNGLE_SCENE:
        case 5003:
        case QUEST_ID_LUNASTRA_TEOSTRA_VOLCANO_8_SCENE:
        case 5006:
            if (em->stateGroup != EM_GROUP_EVENT && em->stateGroup != EM_GROUP_DEFEAT) {
                em->scriptPending = 0;
                if (em->stance == EM_STANCE_AIR) {
                    EmKushala_ChangeState(em, EM_GROUP_EVENT, KUSH_EVENT_LEAVE_FROM_AIR, EM_SYNC_OWNER);
                } else {
                    if (gScene.area == em->area) {
                        CamReq_ForMonster(em);
                    }
                    EmKushala_ChangeState(em, EM_GROUP_EVENT, KUSH_EVENT_LEAVE, EM_SYNC_OWNER);
                }
            }
            break;
        default:
            if (em->stance == EM_STANCE_AIR) {
                EmKushala_ChangeState(em, EM_GROUP_DEFEAT, KUSH_DEFEAT_DIE_FALLING, EM_SYNC_OWNER);
            } else {
                EmKushala_ChangeState(em, EM_GROUP_DEFEAT, KUSH_DEFEAT_DIE, EM_SYNC_OWNER);
            }
            break;
        }
        break;
    case EM_DMG_ATTACK9:
        if (em->stance == EM_STANCE_AIR) {
            EmKushala_ChangeState(em, EM_GROUP_REACT, KUSH_REACT_AIR_KNOCK_DOWN, EM_SYNC_OWNER);
        } else if (em->stance == EM_STANCE_FAMILY) {
            EmKushala_ChangeState(em, EM_GROUP_REACT, KUSH_REACT_HOVER_KNOCK_DOWN, EM_SYNC_OWNER);
        } else {
            EmKushala_ChangeState(em, EM_GROUP_REACT, KUSH_REACT_KNOCK_DOWN, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_ATTACK6:
    case EM_DMG_ATTACK13:
        if (em->stance == EM_STANCE_GROUND) {
            if (em->type == EM_KIND_KUSHALA_DAORA || em->type == EM_KIND_RUSTED_KUSHALA_DAORA) {
                EmKushala_ChangeState(em, EM_GROUP_REACT, KUSH_REACT_HOLD_RISE, EM_SYNC_OWNER);
            } else {
                EmKushala_ChangeState(em, EM_GROUP_REACT, KUSH_REACT_HOLD_SETTLE, EM_SYNC_OWNER);
            }
        }
        break;
    case EM_DMG_STATUS_IN_AIR:
        if ((em->stateGroup != EM_GROUP_REACT || em->state != KUSH_REACT_FALL) && (em->stateGroup != EM_GROUP_REACT || em->state != KUSH_REACT_AIR_KNOCK_DOWN)) {
            EmKushala_ChangeState(em, EM_GROUP_REACT, KUSH_REACT_FALL, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_STATUS4:
        if ((em->stateGroup != EM_GROUP_REACT || em->state != KUSH_REACT_STATUS4) && (em->stateGroup != EM_GROUP_REACT || em->state != KUSH_REACT_FALL) &&
            (em->stateGroup != EM_GROUP_REACT || em->state != KUSH_REACT_AIR_KNOCK_DOWN)) {
            Em_SetStatus4Timer(em);
            EmKushala_ChangeState(em, EM_GROUP_REACT, KUSH_REACT_STATUS4, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_CAPTURED:
        if ((em->stateGroup != EM_GROUP_IDLE || em->state != KUSH_IDLE_STATUS1_COLLAPSE) && (em->stateGroup != EM_GROUP_REACT || em->state != KUSH_REACT_FALL) &&
            (em->stateGroup != EM_GROUP_REACT || em->state != KUSH_REACT_AIR_KNOCK_DOWN)) {
            Em_SetStatus1Timer(em);
            EmKushala_ChangeState(em, EM_GROUP_IDLE, KUSH_IDLE_STATUS1_COLLAPSE, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_STATUS2:
        if ((em->stateGroup != EM_GROUP_IDLE || em->state != KUSH_IDLE_STATUS2_COLLAPSE) && (em->stateGroup != EM_GROUP_REACT || em->state != KUSH_REACT_FALL) &&
            (em->stateGroup != EM_GROUP_REACT || em->state != KUSH_REACT_AIR_KNOCK_DOWN)) {
            Em_SetStatus2Timer(em);
            EmKushala_ChangeState(em, EM_GROUP_IDLE, KUSH_IDLE_STATUS2_COLLAPSE, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_HIT_LYING:
        switch (em->state) {
        case 9:
            EmKushala_ChangeState(em, EM_GROUP_IDLE, KUSH_IDLE_REST_END, EM_SYNC_OWNER);
            em->scriptPending = 0;
            Em_CheckRageByHp(em, em->rageThreshold);
            break;
        case 11:
            EmKushala_ChangeState(em, EM_GROUP_IDLE, KUSH_IDLE_STATUS2_END, EM_SYNC_OWNER);
            em->scriptPending = 0;
            break;
        case 21:
            EmKushala_ChangeState(em, EM_GROUP_IDLE, KUSH_IDLE_STATUS1_END, EM_SYNC_OWNER);
            em->scriptPending = 0;
            break;
        }
        break;
    case EM_DMG_PART8_DOWN:
        if (em->type == EM_KIND_KUSHALA_DAORA || em->type == EM_KIND_RUSTED_KUSHALA_DAORA) {
            EmKushala_ChangeState(em, EM_GROUP_REACT, KUSH_REACT_PART_BREAK, EM_SYNC_OWNER);
        } else if (em->partBreakCount > 0) {
            EmKushala_ChangeState(em, EM_GROUP_REACT, KUSH_REACT_PART_BREAK, EM_SYNC_OWNER);
        } else {
            EmKushala_ChangeState(em, EM_GROUP_REACT, KUSH_REACT_FIRST_BREAK_FLINCH, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_PART_FLINCH:
        if (em->stance == EM_STANCE_AIR) {
            if (em->flinchPart == 0) {
                EmKushala_ChangeState(em, EM_GROUP_REACT, KUSH_REACT_AIR_KNOCK_DOWN, EM_SYNC_OWNER);
            } else {
                EmKushala_ChangeState(em, EM_GROUP_REACT, KUSH_REACT_FALL, EM_SYNC_OWNER);
            }
        } else if (em->stance == EM_STANCE_FAMILY) {
            if (em->flinchPart == 0) {
                EmKushala_ChangeState(em, EM_GROUP_REACT, KUSH_REACT_HOVER_KNOCK_DOWN, EM_SYNC_OWNER);
            } else {
                EmKushala_ChangeState(em, EM_GROUP_REACT, KUSH_REACT_FLINCH_HOVER, EM_SYNC_OWNER);
            }
        } else {
            EmKushala_BreakParts(em, 1);
            switch (em->flinchPart) {
            case 0:
                EmKushala_ChangeState(em, EM_GROUP_REACT, KUSH_REACT_KNOCK_DOWN, EM_SYNC_OWNER);
                break;
            case 5:
            case 1:
            case 2:
                EmKushala_ChangeState(em, EM_GROUP_REACT, KUSH_REACT_FLINCH, EM_SYNC_OWNER);
                break;
            default:
                if (em->parts[em->flinchPart].state >= 2) {
                    EmKushala_ChangeState(em, EM_GROUP_REACT, KUSH_REACT_FLINCH_HEAVY, EM_SYNC_OWNER);
                } else {
                    EmKushala_ChangeState(em, EM_GROUP_REACT, KUSH_REACT_FLINCH, EM_SYNC_OWNER);
                }
                break;
            }
        }
        break;
    case EM_DMG_HIT_MODE3:
        if (em->stance != EM_STANCE_AIR) {
            EmKushala_ChangeState(em, EM_GROUP_REACT, KUSH_REACT_FLINCH, EM_SYNC_OWNER);
            em->scriptPending = 0;
        }
        break;
    case EM_DMG_STATUS5:
        if ((em->stateGroup != EM_GROUP_REACT || em->state != KUSH_REACT_STUN) && (em->stateGroup != EM_GROUP_REACT || em->state != KUSH_REACT_FALL) &&
            (em->stateGroup != EM_GROUP_REACT || em->state != KUSH_REACT_AIR_KNOCK_DOWN)) {
            Em_SetStatus5Timer(em);
            EmKushala_ChangeState(em, EM_GROUP_REACT, KUSH_REACT_STUN, EM_SYNC_OWNER);
        }
        break;
    }
    if (Em_DamageResultIsReaction(em, damage) == 1) {
        EmKushala_BreakParts(em, 0);
    }
    switch (gQuest.stage) {
    case QUEST_ID_LUNASTRA_TEOSTRA_VOLCANO_7_SCENE_1:
    case QUEST_ID_LUNASTRA_TEOSTRA_VOLCANO_7_SCENE_2:
        if ((em->type == EM_KIND_LUNASTRA || em->type == EM_KIND_TEOSTRA) && Area_DayId(em->area) == AREA_VOLCANO_7 && Save_TestEventFlag(EVENT_FLAG_LUNASTRA_TEOSTRA_VOLCANO_7_SCENE) == 0) {
            if (Area_DayId(gScene.area) == AREA_VOLCANO_7 && gScene.eventRunning == 1 && em->stateGroup != EM_GROUP_EVENT) {
                EmKushala_ChangeState(em, EM_GROUP_EVENT, KUSH_EVENT_SCENE_DESCEND, EM_SYNC_SEND);
            }
            break;
        }
        goto normal;
    case QUEST_ID_KUSHALA_JUNGLE_SCENE:
        if ((em->type == EM_KIND_KUSHALA_DAORA || em->type == EM_KIND_RUSTED_KUSHALA_DAORA) && Area_DayId(em->area) == AREA_JUNGLE_3 && Save_TestEventFlag(EVENT_FLAG_KUSHALA_JUNGLE_SCENE) == 0) {
            if (Area_DayId(gScene.area) == AREA_JUNGLE_3 && gScene.eventRunning == 1 && em->stateGroup != EM_GROUP_EVENT) {
                EmKushala_ChangeState(em, EM_GROUP_EVENT, KUSH_EVENT_SCENE_FLY_IN, EM_SYNC_SEND);
            }
            break;
        }
        goto normal;
    case QUEST_ID_KUSHALA_SNOW_SCENE:
        if ((em->type == EM_KIND_KUSHALA_DAORA || em->type == EM_KIND_RUSTED_KUSHALA_DAORA) && Area_DayId(em->area) == AREA_SNOWY_MOUNTAINS_6 && Save_TestEventFlag(EVENT_FLAG_KUSHALA_SNOW_SCENE) == 0) {
            if (Area_DayId(gScene.area) == AREA_SNOWY_MOUNTAINS_6 && gScene.eventRunning == 1 && em->stateGroup != EM_GROUP_EVENT) {
                EmKushala_ChangeState(em, EM_GROUP_EVENT, KUSH_EVENT_SCENE_WALK_IN, EM_SYNC_SEND);
            }
            break;
        }
        goto normal;
    case QUEST_ID_LUNASTRA_TEOSTRA_VOLCANO_8_SCENE:
        if ((em->type == EM_KIND_LUNASTRA || em->type == EM_KIND_TEOSTRA) && Area_DayId(em->area) == AREA_VOLCANO_8 && Save_TestEventFlag(EVENT_FLAG_LUNASTRA_TEOSTRA_VOLCANO_8_SCENE) == 0) {
            if (Area_DayId(gScene.area) == AREA_VOLCANO_8 && gScene.eventRunning == 1 && em->stateGroup != EM_GROUP_EVENT) {
                EmKushala_ChangeState(em, EM_GROUP_EVENT, KUSH_EVENT_SCENE_RISE, EM_SYNC_SEND);
            }
            break;
        }
        goto normal;
    case QUEST_ID_LUNASTRA_TEOSTRA_TOWER_SCENE:
        if ((em->type == EM_KIND_LUNASTRA || em->type == EM_KIND_TEOSTRA) && Area_DayId(em->area) == AREA_TOWER_0x7C && Save_TestEventFlag(EVENT_FLAG_LUNASTRA_TEOSTRA_TOWER_SCENE) == 0) {
            if (Area_DayId(gScene.area) == AREA_TOWER_0x7C && gScene.eventRunning == 1 && em->stateGroup != EM_GROUP_EVENT) {
                EmKushala_ChangeState(em, EM_GROUP_EVENT, KUSH_EVENT_SCENE_WAKE, EM_SYNC_SEND);
            }
            break;
        }
    default:
    normal: /* a story stage whose scene doesn't apply joins the default here */
        if (Quest_GetOutcome(0) == 1 && em->stateGroup != EM_GROUP_EVENT && em->stateGroup != EM_GROUP_DEFEAT) {
            em->scriptPending = 0;
            if (em->stance == EM_STANCE_AIR) {
                EmKushala_ChangeState(em, EM_GROUP_EVENT, KUSH_EVENT_LEAVE_FROM_AIR, EM_SYNC_LOCAL);
            } else {
                if (gScene.area == em->area) {
                    CamReq_ForMonster(em);
                }
                EmKushala_ChangeState(em, EM_GROUP_EVENT, KUSH_EVENT_LEAVE, EM_SYNC_LOCAL);
            }
        }
        Em_UpdatePendingScript(em);
        break;
    }
    Em_ClearHitReactMode(em);
    EmKushala_RunState(em, (s32) w);
    if (em->stateRequest != 0) {
        EmKushala_RunState(em, (s32) w);
        em->stateRequest = 0;
    }
    if (em->stateGroup == EM_GROUP_FLY && (s8) w->flyGuard == 1) {
        em->guardTimer = 2;
    }
}

/*
 * Remembers the current state as the previous one, then runs the state's
 * group dispatcher (group 7 shares group 6's).
 */
void EmKushala_RunState(Em *em, s32 work)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->stateGroup) {
    case EM_GROUP_IDLE:
        EmKushala_RunIdleState(em, (EmWork *) work);
        break;
    case EM_GROUP_MOVE:
        EmKushala_RunMoveState(em, (EmWork *) work);
        break;
    case EM_GROUP_FLY:
        EmKushala_RunFlyState(em, (EmWork *) work);
        break;
    case EM_GROUP_ATTACK:
        EmKushala_RunAttackState(em, (EmWork *) work);
        break;
    case EM_GROUP_REACT:
        EmKushala_RunReactState(em, (EmWork *) work);
        break;
    case EM_GROUP_DEFEAT:
        EmKushala_RunDefeatState(em, (EmWork *) work);
        break;
    case EM_GROUP_EVENT:
        EmKushala_RunEventState(em, (EmWork *) work);
        break;
    case EM_GROUP_EVENT2:
        EmKushala_RunEventState(em, (EmWork *) work);
        
    default:
        break;
    }
}

/*
 * Returns 1 in the ground states (groups 0, 1 and 3) where the wind's ring
 * replaces the footfall dust.
 */
s32 EmKushala_StateHasWindRing(Em *em)
{
    switch (em->stateGroup) {
    case 0:
        switch (em->state) {
        case 2:
        case 3:
        case 19:
            return 1;
        }
        break;
    case 1:
        switch (em->state) {
        case 3:
        case 5:
        case 8:
        case 23:
        case 25:
        case 27:
        case 28:
        case 29:
            return 1;
        }
        break;
    case 3:
        switch (em->state) {
        case 2:
        case 15:
        case 16:
        case 17:
            return 1;
        }
        break;
    }
    return 0;
}

/*
 * Puts the dust effect at a bone, at floor height, when a player is in the
 * area.
 */
void EmKushala_SpawnGroundEffect(Em *em, s32 bone, s32 effectKind, f32 scale)
{
    f32 pos[3];

    if (Player_InCurrentArea(em)) {
        Em_BonePos(em, bone, pos);
        pos[1] = em->floorY;
        PartFx_SpawnOnGround(pos, effectKind, scale * em->scaleX);
    }
}

/*
 * Puts the dust at a bone within 1000 of the floor, unless the wind's ring
 * replaces it; with withImpact also an impact attack at the monster.
 */
void EmKushala_SpawnDustNearGround(Em *em, s32 bone, s32 effectKind, s32 flag, f32 scale)
{
    if (EmKushala_WindActive(em) != 1 || EmKushala_StateHasWindRing(em) != 1) {
        if (em->posY <= (1000.0f + em->floorY)) {
            EmKushala_SpawnGroundEffect(em, bone, effectKind, scale);
            if (flag != 0) {
                EmImpactAttack_CreateAt(em, (f32 *) &em->posX, 1U);
            }
        }
    }
}

/*
 * The same as EmKushala_SpawnDustNearGround with the ground stage object
 * (StageObj_K14) instead of the dust.
 */
void EmKushala_SpawnK14NearGround(Em *em, s32 k, s32 variant, s32 flag, f32 scale)
{
    if (EmKushala_WindActive(em) != 1 || EmKushala_StateHasWindRing(em) != 1) {
        if (em->posY <= (1000.0f + em->floorY)) {
            DustCloud_Create((Em *) em, k, variant, scale);
            if (flag != 0) {
                EmImpactAttack_CreateAt(em, (f32 *) &em->posX, 1U);
            }
        }
    }
}

/* The default of EmKushala_MotionEffects: nothing. */
static void EmKushala_MotionEffectsNop(Em *em)
{
}

/*
 * While enraged, every 45 frames puts an aura puff on the monster in its
 * moving and idle motions.
 */
void EmKushala_SpawnRageAura(Em *em, s16 motion)
{
    switch (motion) {
    case EM_MOTION_ID(0, KUSH_MOT_IDLE):
    case EM_MOTION_ID(0, KUSH_MOT_WALK):
    case EM_MOTION_ID(0, 3):
    case EM_MOTION_ID(0, KUSH_MOT_TURN_LEFT):
    case EM_MOTION_ID(0, KUSH_MOT_TURN_RIGHT):
    case EM_MOTION_ID(0, KUSH_MOT_TURN_SMALL):
    case EM_MOTION_ID(0, KUSH_MOT_HOVER):
    case EM_MOTION_ID(0, KUSH_MOT_IDLE_ACTION_B):
    case EM_MOTION_ID(0, KUSH_MOT_RUN_STOP):
    case EM_MOTION_ID(0, KUSH_MOT_STOMP):
    case EM_MOTION_ID(0, KUSH_MOT_WING_FLAP):
    case EM_MOTION_ID(0, KUSH_MOT_ALERT):
    case EM_MOTION_ID(0, KUSH_MOT_IDLE_ACTION_A):
    case EM_MOTION_ID(0, KUSH_MOT_LIMP):
    case EM_MOTION_ID(0, KUSH_MOT_TESCA_ACTION_A):
    case EM_MOTION_ID(0, KUSH_MOT_SHAKE_OFF):
    case EM_MOTION_ID(0, KUSH_MOT_PART_BREAK_TURN):
        if (em->rage != 0 && gScene.frame % 45 == 0) {
            MonsterFx_CreateOnEm(em, 13, 2.0f * em->scaleX);
        }
        break;
    }
}

void Explosion_CreateOwned(Player *owner, f32 *pos, s32 kind);
/*
 * Each frame: keeps the current motion in fxMotion, plays its sounds
 * (EmKushala_MotionSounds or EmKushala_MotionSoundsTesca), the rage aura,
 * then the motion's dust, stage objects and attacks at their frames.
 */
void EmKushala_MotionEffects(Em *em, EmWork *work)
{
    f32 burstPos[3];
    f32 aheadOfs[3];
    f32 puffOfsA[3];
    f32 puffOfsB[3];

    if (em->motionId[0] != work->fxMotion) {
        work->fxMotion = em->motionId[0];
    }
    if ((em->type == EM_KIND_KUSHALA_DAORA) || (em->type == EM_KIND_RUSTED_KUSHALA_DAORA)) {
        EmKushala_MotionSounds(em, work);
    } else {
        EmKushala_MotionSoundsTesca(em, work);
    }
    EmKushala_SpawnRageAura(em, (s32) work->fxMotion);
    switch (work->fxMotion) {
    case EM_MOTION_ID(0, KUSH_MOT_TURN_BIG_RIGHT):
        if (EmMotion_PassedFrameScaled(em, 8.0f, 0) != 0) {
            EmKushala_SpawnGroundEffect(em, KUSH_BONE_FOOT_C, 3, 8.0f * em->scaleX);
        }
        if (EmMotion_PassedFrameScaled(em, 25.0f, 0) != 0) {
            EmKushala_SpawnGroundEffect(em, KUSH_BONE_LEG_A, 3, 8.0f * em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_TURN_BIG_LEFT):
        if (EmMotion_PassedFrameScaled(em, 8.0f, 0) != 0) {
            EmKushala_SpawnGroundEffect(em, KUSH_BONE_LEG_A, 3, 8.0f * em->scaleX);
        }
        if (EmMotion_PassedFrameScaled(em, 25.0f, 0) != 0) {
            EmKushala_SpawnGroundEffect(em, KUSH_BONE_FOOT_C, 3, 8.0f * em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_TAKE_OFF):
        if (EmMotion_PassedFrameScaled(em, 46.0f, 0) != 0) {
            EmKushala_SpawnK14NearGround(em, 1, 0, 0, 1.5f * em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_HOVER):
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            EmKushala_SpawnK14NearGround(em, 1, 0, 0, 1.5f * em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_DASH_START):
        if (EmMotion_PassedFrameScaled(em, 60.0f, 0) != 0) {
            EmKushala_SpawnK14NearGround(em, 1, 0, 0, 1.5f * em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_DASH_END):
        if ((EmMotion_PassedFrameScaled(em, 26.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 76.0f, 0) != 0)) {
            EmKushala_SpawnK14NearGround(em, 9, 0, 0, em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_LAND):
        if (EmMotion_PassedFrameScaled(em, 8.0f, 0) != 0) {
            EmKushala_SpawnDustNearGround(em, 1, 7, 0, 10.0f * em->scaleX);
        }
        if (EmMotion_PassedFrameScaled(em, 66.0f, 0) != 0) {
            EmKushala_SpawnDustNearGround(em, 1, 12, 0, 12.0f * em->scaleX);
        }
        if (EmMotion_PassedFrameScaled(em, 70.0f, 0) != 0) {
            EmImpactAttack_CreateAt(em, (f32 *) &em->posX, 2);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_HOVER_TURN):
        if (EmMotion_PassedFrameScaled(em, 8.0f, 0) != 0) {
            EmKushala_SpawnK14NearGround(em, 1, 0, 0, em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_SIDE_DASH_A):
        if (EmMotion_PassedFrameScaled(em, 4.0f, 0) != 0) {
            EmKushala_SpawnK14NearGround(em, 1, 0, 0, em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_SIDE_DASH_B):
        if (EmMotion_PassedFrameScaled(em, 4.0f, 0) != 0) {
            EmKushala_SpawnDustNearGround(em, 1, 7, 0, 10.0f * em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_RUN_START):
        if (EmMotion_PassedFrameScaled(em, 18.0f, 0) != 0) {
            EmKushala_SpawnGroundEffect(em, KUSH_BONE_FOOT_D, 3, 8.0f * em->scaleX);
        }
        if (EmMotion_PassedFrameScaled(em, 36.0f, 0) != 0) {
            EmKushala_SpawnGroundEffect(em, KUSH_BONE_FOOT_B, 3, 8.0f * em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_RUN_B):
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 2, 26, 1.5f * em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_RUN_C):
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 2, 26, 1.5f * em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_REAR_UP):
        if (EmMotion_PassedFrameScaled(em, 46.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 2, 27, 2.0f * em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_STOMP):
        if (EmMotion_PassedFrameScaled(em, 16.0f, 0) != 0) {
            EmKushala_CreateAttack(em, 11);
        }
        if (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) {
            EmKushala_SpawnGroundEffect(em, 1, 7, 10.0f * em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_LIFT_TO_HOVER):
        if (EmMotion_PassedFrameScaled(em, 24.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 1, 0, em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_ROAR):
        if ((em->type == EM_KIND_KUSHALA_DAORA) || (em->type == EM_KIND_RUSTED_KUSHALA_DAORA)) {
            if (EmMotion_PassedFrameScaled(em, 62.0f, 0) != 0) {
                EmKushala_CreateAttack(em, 28);
                ElementBurst_CreateOnEm(em, 5, 3, 1.0f);
            }
            if ((EmKushala_WindActive(em) == 1) && (EmMotion_PassedFrameScaled(em, 186.0f, 0) != 0)) {
                MonsterFx_CreateOnEm(em, 16, em->scaleX);
                EmImpactAttack_CreateAt(em, (f32 *) &em->posX, 4);
                break;
            }
        } else {
            if (EmMotion_PassedFrameScaled(em, 62.0f, 0) != 0) {
                EmKushala_CreateAttack(em, 28);
                ElementBurst_CreateOnEm(em, 5, 3, 1.0f);
            }
            if ((EmKushala_WindActiveTesca(em) == 1) && (EmMotion_PassedFrameScaled(em, 186.0f, 0) != 0)) {
                MonsterFx_CreateOnEm(em, 16, em->scaleX);
                EmImpactAttack_CreateAt(em, (f32 *) &em->posX, 4);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_TESCA_ACTION_C_END):
        if (((em->type == EM_KIND_LUNASTRA) || (em->type == EM_KIND_TEOSTRA)) && (EmMotion_PassedFrameScaled(em, 16.0f, 0) != 0)) {
            EmKushala_SpawnGroundEffect(em, 1, 7, 10.0f * em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_EAT_START):
        break;
    case EM_MOTION_ID(0, KUSH_MOT_REST):
        if (EmMotion_ReachedFrame(em, 0, 220.0f) != 0) {
            puffOfsA[0] = 30.0f;
            puffOfsA[1] = 30.0f;
            puffOfsA[2] = 200.0f;
            Em_SpawnLyingPuffs(em, KUSH_BONE_BREATH, puffOfsA, 1.6f);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_BREATH_STREAM_END):
        if (((em->type == EM_KIND_LUNASTRA) || (em->type == EM_KIND_TEOSTRA)) && (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0)) {
            EmKushala_SpawnK14NearGround(em, 1, 0, 0, em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_TESCA_ACTION_C):
        if ((em->type == EM_KIND_LUNASTRA) || (em->type == EM_KIND_TEOSTRA)) {
            if (EmMotion_PassedFrameScaled(em, 20.0f, 0) != 0) {
                EmKushala_SpawnK14NearGround(em, 1, 29, 0, em->scaleX);
            }
            if (EmMotion_PassedFrameScaled(em, 24.0f, 0) != 0) {
                EmKushala_SpawnK14NearGround(em, 1, 25, 0, em->scaleX);
            }
            if (EmMotion_PassedFrameScaled(em, 18.0f, 0) != 0) {
                EmKushala_CreateAttack(em, 29);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_TESCA_ACTION_B):
        if ((em->type == EM_KIND_LUNASTRA) || (em->type == EM_KIND_TEOSTRA)) {
            if (EmMotion_PassedFrameScaled(em, 40.0f, 0) != 0) {
                DustCloud_Create((Em *) em, 2, 30, 1.5f * em->scaleX);
            }
            if (EmMotion_PassedFrameScaled(em, 32.0f, 0) != 0) {
                EmKushala_CreateAttack(em, 30);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_TESCA_ACTION_A):
        if ((em->type == EM_KIND_LUNASTRA) || (em->type == EM_KIND_TEOSTRA)) {
            if (EmMotion_PassedFrameScaled(em, 122.0f, 0) != 0) {
                EmKushala_CreateAttack(em, 5);
            }
            if (EmMotion_PassedFrameScaled(em, 124.0f, 0) != 0) {
                DustCloud_Create((Em *) em, 2, 32, 2.0f * em->scaleX);
            }
            if (EmMotion_PassedFrameScaled(em, 162.0f, 0) != 0) {
                EmKushala_SpawnGroundEffect(em, 18, 12, 8.0f * em->scaleX);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, 44):
        if (EmMotion_PassedFrameScaled(em, 32.0f, 0) != 0) {
            EmKushala_CreateAttack(em, 5);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_BREATH_FINISH):
        if (EmMotion_PassedFrameScaled(em, 32.0f, 0) != 0) {
            EmKushala_CreateAttack(em, 6);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_STRIKE_BREATH):
        if (EmMotion_PassedFrameScaled(em, 60.0f, 0) != 0) {
            EmKushala_CreateAttack(em, 12);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_BREATH_CHAIN):
        if (EmMotion_PassedFrameScaled(em, 90.0f, 0) != 0) {
            EmKushala_CreateAttack(em, 17);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_BREATH_WINDUP):
        if ((EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 60.0f, 0) != 0)) {
            EmKushala_SpawnK14NearGround(em, 1, 0, 0, em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_BREATH_STREAM):
        if ((em->type == EM_KIND_KUSHALA_DAORA) || (em->type == EM_KIND_RUSTED_KUSHALA_DAORA)) {
            if (EmMotion_PassedFrameScaled(em, 24.0f, 0) != 0) {
                u8 map = gScene.map;

                if (map == MAP_SNOWY_MOUNTAINS || map == MAP_SNOWY_MOUNTAINS_NIGHT) {
                    EmRangedAttack_CreateAtBoneTimed(em, KUSH_BONE_BREATH, EMRANGED_KUSHALA_BREATH, 1, 8192, 0, (s32) (131.0f * em->motionSpeed));
                } else {
                    EmRangedAttack_CreateAtBoneTimed(em, KUSH_BONE_BREATH, EMRANGED_KUSHALA_BREATH, 0, 8192, 0, (s32) (131.0f * em->motionSpeed));
                }
            }
        } else if (EmMotion_PassedFrameScaled(em, 24.0f, 0) != 0) {
            EmEffectAttack_CreateChain(em, EMFX_BREATH_CHAIN, 0, 8192, 0);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_BREATH):
        break;
    case EM_MOTION_ID(0, KUSH_MOT_LUNGE):
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            EmKushala_SpawnDustNearGround(em, 1, 7, 0, 10.0f * em->scaleX);
        }
        if ((EmMotion_PassedFrameScaled(em, 52.0f, 0) != 0) && Player_InCurrentArea(em) && (em->posY <= (1000.0f + em->floorY))) {
            aheadOfs[0] = 0.0f;
            aheadOfs[1] = 0.0f;
            aheadOfs[2] = 400.0f * em->scaleZ;
            Vec_RotateY(aheadOfs, (6.2831855f * (f32) em->rotY) / 65536.0f);
            burstPos[0] = em->posX + aheadOfs[0];
            burstPos[1] = em->floorY;
            burstPos[2] = em->posZ + aheadOfs[2];
            MonsterFx_CreateAtYaw(burstPos, (u16) em->rotY, 17, 1.2f);
            Explosion_CreateOwned((Player *)em, burstPos, EXPLOSION_KUSHALA_GROUND_BLAST);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_HOVER_ACTION_A):
        if (EmMotion_PassedFrameScaled(em, 50.0f, 0) != 0) {
            EmKushala_CreateAttack(em, 2);
        }
        if (EmMotion_PassedFrameScaled(em, 56.0f, 0) != 0) {
            EmKushala_SpawnGroundEffect(em, 1, 12, 8.0f * em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_HOVER_ACTION_B):
        if (EmMotion_PassedFrameScaled(em, 42.0f, 0) != 0) {
            EmKushala_CreateAttack(em, 18);
        }
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            EmKushala_SpawnK14NearGround(em, 1, 0, 0, em->scaleX);
        }
        if (EmMotion_PassedFrameScaled(em, 64.0f, 0) != 0) {
            EmKushala_SpawnDustNearGround(em, 1, 7, 0, 10.0f * em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_TURN_SWEEP_A):
        if (EmMotion_PassedFrameScaled(em, 48.0f, 0) != 0) {
            EmKushala_CreateAttack(em, 3);
        }
        if (EmMotion_PassedFrameScaled(em, 54.0f, 0) != 0) {
            EmKushala_CreateAttack(em, 13);
        }
        if (EmMotion_PassedFrameScaled(em, 60.0f, 0) != 0) {
            EmKushala_SpawnGroundEffect(em, KUSH_BONE_FOOT_D, 4, 6.0f * em->scaleX);
        }
        if (EmMotion_PassedFrameScaled(em, 58.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 2, 30, 1.5f * em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_TURN_SWEEP_B):
        if (EmMotion_PassedFrameScaled(em, 48.0f, 0) != 0) {
            EmKushala_CreateAttack(em, 4);
        }
        if (EmMotion_PassedFrameScaled(em, 54.0f, 0) != 0) {
            EmKushala_CreateAttack(em, 15);
        }
        if (EmMotion_PassedFrameScaled(em, 60.0f, 0) != 0) {
            EmKushala_SpawnGroundEffect(em, KUSH_BONE_FOOT_B, 4, 6.0f * em->scaleX);
        }
        if (EmMotion_PassedFrameScaled(em, 70.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 3, 28, 1.5f * em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_HOVER_SWEEP_A):
        if (EmMotion_PassedFrameScaled(em, 50.0f, 0) != 0) {
            EmKushala_CreateAttack(em, 19);
        }
        if (EmMotion_PassedFrameScaled(em, 52.0f, 0) != 0) {
            EmKushala_CreateAttack(em, 20);
        }
        if (EmMotion_PassedFrameScaled(em, 82.0f, 0) != 0) {
            EmKushala_CreateAttack(em, 21);
        }
        if (EmMotion_PassedFrameScaled(em, 6.0f, 0) != 0) {
            EmKushala_SpawnGroundEffect(em, 1, 7, 10.0f * em->scaleX);
        }
        if (EmMotion_PassedFrameScaled(em, 48.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 3, 31, 1.5f * em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_HOVER_SWEEP_B):
        if (EmMotion_PassedFrameScaled(em, 50.0f, 0) != 0) {
            EmKushala_CreateAttack(em, 22);
        }
        if (EmMotion_PassedFrameScaled(em, 52.0f, 0) != 0) {
            EmKushala_CreateAttack(em, 23);
        }
        if (EmMotion_PassedFrameScaled(em, 82.0f, 0) != 0) {
            EmKushala_CreateAttack(em, 24);
        }
        if (EmMotion_PassedFrameScaled(em, 6.0f, 0) != 0) {
            EmKushala_SpawnDustNearGround(em, 1, 7, 0, 10.0f * em->scaleX);
        }
        if (EmMotion_PassedFrameScaled(em, 60.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 2, 31, 1.5f * em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_TOPPLE_RECOVER):
        if (EmMotion_PassedFrameScaled(em, 20.0f, 0) != 0) {
            EmKushala_SpawnGroundEffect(em, 1, 7, 8.0f * em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_FLINCH_HOVER):
        if ((EmMotion_PassedFrameScaled(em, 22.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 70.0f, 0) != 0)) {
            EmKushala_SpawnK14NearGround(em, 1, 0, 0, em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_FALL_LAND):
        if (EmMotion_PassedFrameScaled(em, 16.0f, 0) != 0) {
            EmKushala_SpawnK14NearGround(em, 11, 0, 0, em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_DOWN):
        if (EmMotion_PassedFrameScaled(em, 60.0f, 0) != 0) {
            EmKushala_SpawnGroundEffect(em, 1, 12, 8.0f * em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_DIE):
        if (EmMotion_PassedFrameScaled(em, 420.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 0, 0, em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_COLLAPSE):
        if (EmMotion_PassedFrameScaled(em, 190.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 0, 0, em->scaleX);
        }
        if (EmMotion_ReachedFrame(em, 0, 228.0f) != 0) {
            puffOfsB[0] = -30.0f;
            puffOfsB[1] = 30.0f;
            puffOfsB[2] = 200.0f;
            Em_SpawnLyingPuffs(em, KUSH_BONE_BREATH, puffOfsB, 1.6f);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_TOPPLE):
        if (EmMotion_PassedFrameScaled(em, 134.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 0, 0, em->scaleX);
            break;
        }
        break;
    case EM_MOTION_ID(0, KUSH_MOT_PART_BREAK):
        if (EmMotion_PassedFrameScaled(em, 60.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 0, 0, em->scaleX);
            break;
        }
        break;
    default:
        EmKushala_MotionEffectsNop(em);
        break;
    }
}

/*
 * Callback slot 3: skips the first frame, then runs EmKushala_MotionEffects
 * every frame.
 */
void EmKushala_UpdateMotionEffects(Em *em)
{
    EmKushalaWork *w = (EmKushalaWork *)&em->work;

    switch (w->fxStep) {
    case 0:
        w->fxStep++;
        break;
    case 1:
        EmKushala_MotionEffects(em, (EmWork *) w);
        break;
    }
}

/*
 * Creates the family's contact attack of the given kind (KushalaAttack for
 * Kushala Daora, TeostraAttack for Lunastra and Teostra).
 */
void EmKushala_CreateAttack(Em *em, s32 kind)
{
    u8 type;

    type = em->type;
    if ((type == EM_KIND_KUSHALA_DAORA) || (type == EM_KIND_RUSTED_KUSHALA_DAORA)) {
        KushalaAttack_Create(em, kind);
        return;
    }
    TeostraAttack_Create(em, kind);
}

/*
 * The blast's flash: when a player is in the area, an effect and a stage
 * object in front of KUSH_BONE_BLAST.
 */
void EmKushala_SpawnBlastBurst(Em *em)
{
    f32 pos[3];
    f32 ofs[3];
    void *bone;

    if ((s32)Player_InCurrentArea(em) & 0xFF) {
        bone = Em_Bone(em, KUSH_BONE_BLAST);
        Mtx_GetTrans(pos, bone);
        ofs[0] = 0.0f;
        ofs[1] = -25.0f;
        ofs[2] = 60.0f;
        Vec_RotateSelf(ofs, bone);
        pos[0] = pos[0] + ofs[0];
        pos[1] = pos[1] + ofs[1];
        pos[2] = pos[2] + ofs[2];
        HitSpark_SpawnAt(pos, 0, em->rotY);
        HitSplash_CreateAtScaled(pos, 6, 1.0f);
    }
}

/* Callback slot 2: nothing. */
void EmKushala_TableNop(void)
{
}

/*
 * Callback slot 6: returns 1 when part 8 may go down: on the ground, at 30%
 * hp or less with no break yet (Lunastra and Teostra: 40% or less, at most
 * one break).
 */
s32 EmKushala_AllowHit8(Em *em)
{
    if (em->stance != EM_STANCE_AIR && em->stance != EM_STANCE_FAMILY) {
        if (em->type == EM_KIND_KUSHALA_DAORA || em->type == EM_KIND_RUSTED_KUSHALA_DAORA) {
            if (em->hp <= (s16) (0.3f * em->hpMax) && em->partBreakCount == 0) {
                return 1;
            }
        } else {
            if (em->hp <= (s16) (0.4f * em->hpMax) && em->partBreakCount <= 1) {
                return 1;
            }
        }
    }
    return 0;
}

/* The file's initialised data, in the game's order */
EmFlyParam EmKushala_FlyParam = {
#include "game/008CA100.inc"
};
EmFlyParam EmKushala_FlyParam2 = {
#include "game/008CA160.inc"
};
u16 EmKushala_IdleTbl[24] = {
#include "game/008CA1C0.inc"
};
EmTurnParams EmKushala_TurnParams[1] = {
#include "game/008CA1F0.inc"
};
s32 EmKushala_BreathTimers[6][2] = {
#include "game/008CA210.inc"
};
void *EmKushala_Callbacks[8] = {
    EmKushala_SpawnEffects, EmKushala_SpawnEffects, EmKushala_TableNop, EmKushala_UpdateMotionEffects,
    GmEm_ReturnZero, EmKushala_Reinit, EmKushala_AllowHit8, NULL,
};
