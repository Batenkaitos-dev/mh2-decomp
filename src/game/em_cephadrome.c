/*
 * The Cephadrome family: the Cephadrome and the Cephalos (EM_KIND_CEPHADROME,
 * EM_KIND_CEPHALOS), sand wyverns that swim under the sand, surface to breathe
 * sand at the hunters, leap out of the sand and back, and lie beached on
 * their side after a leap ashore. The file runs from the family's motion
 * sounds (EmCephadrome_MotionSounds) to its area-point helpers:
 * - EmCephadrome_Setup (the per-kind setup hook) and Init;
 * - one function per state, by group (include/em_cephadrome.h names the
 *   states and the motions; group 2 holds the sand-swimming states), a
 *   dispatcher per group (EmCephadrome_Run*State) and the Set*State and
 *   ChangeState functions that enter a state;
 * - EmCephadrome_Update, which turns Em_UpdateDamage's result into reactions;
 * - the motion effects, the four fins' fold and wave
 *   (EmCephadrome_UpdateFins, the same code as the Plesioth's), the movement
 *   curves of the leaps (StartCurves, UpdateCurves) and the area's points the
 *   round swims follow (NextAreaPoint, StepAreaPoint).
 * The state functions take the monster and its work area, read as
 * EmCephadromeWork; Em.step is the step within the current state.
 */
#include "common.h"
#include "scene.h"
#include "quest.h"
#include "em.h"
#include "shadow.h"
#include "em_cephadrome.h"
#include "player.h"
#include "objattach.h"
#include "area.h"
#include "capsule.h"
#include "fishing.h"
#include "sound.h"
#include "cam_shake.h"
#include "dust_flash_effects.h"
#include "engine_math.h"
#include "gm_load.h"
#include "hit_effects.h"
#include "part_effects.h"
#include "rand.h"
#include "vibration.h"
#include "spark_fx.h"
#include "vec_math.h"
#include "em_script.h"
/* prototypes of callees */
void EmTail_Create(Em *);

/* prototypes */
/* unit's own declaration: declared here as the callers use it; with the definition's void (Em *em, s16 n), the calls in EmCephadrome_EatNeed2 compile differently */
void Em_AddHp(Em *em, s32 n);
void Em_EndAllStatus(Em *em);
/* unit's own declaration: declared here as the callers use it; with the definition's void (Em *em, s32 group, s32 state, u8 mode), the calls in EmCephadrome_ChangeState, EmCephadrome_SetIdleState and others compile differently */
void Em_SetStateSync(Em *em, u16 group, u16 state, u8 mode);
void EmRangedAttack_CreateAtBone(Em *, s32, s32, s32, s32, s32);
/* unit's own declaration: declared here as the callers use it; with the definition's s32 (Em *em, s16 n, s16 max, s16 interval), the calls in EmCephadrome_Rest compile differently */
s32 Em_RegenHp(Em *em, s32 n, s32 max, s32 interval);
void StunStars_CreateEm(Em *em, u8 k, s16 v, s16 t, f32 y, f32 scale);
s32 EmMotion_PassedFrameScaled(void *p, f32 frame, s32 layer);
extern EmTurnParams EmCephadrome_TurnParams[3];
extern EmCurveKey *EmCephadrome_VelYCurves[];
extern EmCurveKey EmCephadrome_SpeedCurves_2[];
extern EmCurveKey *EmCephadrome_SpeedCurves[];
void Em_SpawnLyingPuffs(Em *em, s32 bone, f32 *pos, f32 scale);
struct EmIdEntry;
struct EmIdEntry *Em_FindAreaEntry(Em *em, struct EmIdEntry *entry, s16 id);
/* end of prototypes */

/*
 * The motions' sounds: switches on work->prevMotion (the layer-0 motion id
 * the motion effects keep) and plays sounds at bones on given frames
 * (Em_PlaySeAtBoneOnFrame, Em_PlaySeTypeAtBoneOnFrame,
 * Em_PlaySeTypeAtBoneInFrames; some picked at random by Sound_RandomSe).
 */
void EmCephadrome_MotionSounds(Em *em, EmWork *work)
{
    s16 prevMotion;

    prevMotion = work->prevMotion;
    switch (prevMotion) {
    case EM_MOTION_ID(0, CEPH_MOT_IDLE):
        Em_PlaySeAtBoneOnFrame(em, 64, 32, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 200, Sound_RandomSe(45, 2, 45, 2), CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 410, Sound_RandomSe(33, 4, 33, 4), CEPH_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_WALK):
        Em_PlaySeAtBoneOnFrame(em, 52, 1, CEPH_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 116, 1, CEPH_BONE_STEP_B);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_RUN):
        Em_PlaySeTypeAtBoneOnFrame(em, 10, 86, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 28, 6, CEPH_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 52, 6, CEPH_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 76, 6, CEPH_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 100, 6, CEPH_BONE_STEP_B);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_TURN_RIGHT):
        Em_PlaySeAtBoneOnFrame(em, 4, 33, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 26, 1, CEPH_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 56, 1, CEPH_BONE_STEP_B);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_TURN_LEFT):
        Em_PlaySeAtBoneOnFrame(em, 4, 33, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 26, 1, CEPH_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 56, 1, CEPH_BONE_STEP_B);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_LIMP):
        Em_PlaySeTypeAtBoneOnFrame(em, 6, 89, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 6, 4, CEPH_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 76, 0, CEPH_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 28, 18, CEPH_BONE_STEP_A);
        break;
    case EM_MOTION_ID(0, 11):
        Em_PlaySeAtBoneOnFrame(em, 4, 32, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 14, 11, 6);
        Em_PlaySeAtBoneOnFrame(em, 18, 11, 12);
        Em_PlaySeAtBoneOnFrame(em, 70, 11, 6);
        Em_PlaySeAtBoneOnFrame(em, 66, 11, 12);
        Em_PlaySeAtBoneOnFrame(em, 122, 11, 6);
        Em_PlaySeAtBoneOnFrame(em, 126, 11, 12);
        break;
    case EM_MOTION_ID(0, 17):
        Em_PlaySeTypeAtBoneOnFrame(em, 6, 79, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 28, 6, CEPH_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 56, 6, CEPH_BONE_STEP_B);
        break;
    case EM_MOTION_ID(0, 18):
        Em_PlaySeAtBoneOnFrame(em, 56, 7, CEPH_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 26, 13, 6);
        Em_PlaySeAtBoneOnFrame(em, 30, 13, 12);
        Em_PlaySeAtBoneOnFrame(em, 82, 11, 6);
        Em_PlaySeAtBoneOnFrame(em, 86, 11, 12);
        Em_PlaySeAtBoneOnFrame(em, 136, 11, 6);
        Em_PlaySeAtBoneOnFrame(em, 140, 11, 12);
        Em_PlaySeAtBoneOnFrame(em, 182, 11, 6);
        Em_PlaySeAtBoneOnFrame(em, 186, 11, 12);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_TOUCH_DOWN):
        Em_PlaySeAtBoneOnFrame(em, 2, 11, 6);
        Em_PlaySeAtBoneOnFrame(em, 6, 11, 12);
        Em_PlaySeAtBoneOnFrame(em, 12, 4, CEPH_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 4, 1, CEPH_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 14, 9, CEPH_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_IDLE_ACTION):
        Em_PlaySeAtBoneOnFrame(em, 4, 31, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 108, 34, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 170, 32, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 214, 34, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 276, 39, CEPH_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_REST):
        Em_PlaySeTypeAtBoneOnFrame(em, 2, 85, CEPH_BONE_HEAD);
        Em_PlaySeTypeAtBoneOnFrame(em, 182, 85, CEPH_BONE_HEAD);
        Em_PlaySeTypeAtBoneOnFrame(em, 364, 85, CEPH_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_REST_START):
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 87, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 70, 0, CEPH_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 142, 3, CEPH_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 4, 30, CEPH_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 4, 19, CEPH_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_REST_END):
        Em_PlaySeAtBoneOnFrame(em, 4, 32, CEPH_BONE_HEAD);
        Em_PlaySeTypeAtBoneOnFrame(em, 70, 85, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 126, 3, CEPH_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 182, 3, CEPH_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 56, 29, CEPH_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_STATUS_LIE):
        Em_PlaySeTypeAtBoneOnFrame(em, 36, 87, CEPH_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_LAND_ATTACK_A):
        Em_PlaySeAtBoneOnFrame(em, 36, 36, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 20, 0, CEPH_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 40, 0, CEPH_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 60, 21, CEPH_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame(em, 60, 12, 12);
        Em_PlaySeAtBoneOnFrame(em, 12, 12, 6);
        Em_PlaySeAtBoneOnFrame(em, 166, 0, CEPH_BONE_STEP_B);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_STATUS_GET_UP):
        Em_PlaySeAtBoneOnFrame(em, 30, 32, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 12, 12, CEPH_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 28, 2, CEPH_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 46, 3, CEPH_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 72, 1, CEPH_BONE_STEP_B);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_STATUS_COLLAPSE):
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 87, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 72, 22, CEPH_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 164, 9, CEPH_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 144, 3, 12);
        Em_PlaySeAtBoneOnFrame(em, 136, 14, 6);
        Em_PlaySeAtBoneOnFrame(em, 178, 4, CEPH_BONE_BREATH);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_SPIN):
        Em_PlaySeAtBoneOnFrame(em, 8, 35, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 14, 15, 6);
        Em_PlaySeAtBoneOnFrame(em, 30, 20, CEPH_BONE_TAIL_B);
        Em_PlaySeAtBoneOnFrame(em, 42, 0, CEPH_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 72, 1, CEPH_BONE_STEP_A);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_BREATH):
        Em_PlaySeAtBoneOnFrame(em, 2, 18, CEPH_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 80, CEPH_SE_BREATH, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 20, 40, CEPH_BONE_HEAD);
        Em_PlaySeTypeAtBoneOnFrame(em, 74, 81, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 76, 13, 6);
        Em_PlaySeAtBoneOnFrame(em, 72, 13, 12);
        Em_PlaySeAtBoneOnFrame(em, 180, 19, 6);
        Em_PlaySeAtBoneOnFrame(em, 187, 19, 12);
        Em_PlaySeAtBoneOnFrame(em, 240, 18, CEPH_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, 48):
        Em_PlaySeAtBoneOnFrame(em, 2, 34, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 34, 34, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 64, 34, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 114, 32, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 16, 1, CEPH_BONE_STEP_A);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_FEED):
        Em_PlaySeAtBoneOnFrame(em, 12, 29, CEPH_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 6, 34, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 104, 3, CEPH_BONE_STEP_B);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_FLINCH):
        Em_PlaySeTypeAtBoneOnFrame(em, 6, 83, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 148, 34, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 72, 19, CEPH_BONE_TAIL_B);
        Em_PlaySeAtBoneOnFrame(em, 48, 6, CEPH_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 48, 17, CEPH_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 48, 3, CEPH_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 116, 3, CEPH_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 148, 3, CEPH_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 192, 3, CEPH_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 52, 12, 6);
        Em_PlaySeAtBoneOnFrame(em, 56, 13, 12);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_FLINCH_BLIND):
        Em_PlaySeTypeAtBoneOnFrame(em, 6, 83, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 10, 18, CEPH_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 38, 6, CEPH_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 78, 4, CEPH_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 110, 3, CEPH_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 90, 21, CEPH_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 172, 3, CEPH_BONE_STEP_B);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_SHAKE_OFF):
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 79, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 86, 31, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 16, 3, CEPH_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 4, 13, 6);
        Em_PlaySeAtBoneOnFrame(em, 8, 13, 12);
        Em_PlaySeAtBoneOnFrame(em, 98, 19, CEPH_BONE_TAIL_B);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_FLINCH_C):
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 82, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 4, 19, CEPH_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 14, 1, CEPH_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 76, 31, CEPH_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_FLINCH_D):
        Em_PlaySeAtBoneOnFrame(em, 4, 42, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 4, 19, CEPH_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 68, 32, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 122, 3, CEPH_BONE_STEP_B);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_FLINCH_B):
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 82, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 4, 19, CEPH_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, 67):
        Em_PlaySeTypeAtBoneOnFrame(em, 70, 85, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 70, 30, CEPH_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_DIE):
        Em_PlaySeAtBoneOnFrame(em, 26, 48, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 46, 9, CEPH_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 4, 20, CEPH_BONE_ROOT);
        Em_PlaySeTypeAtBoneOnFrame(em, 26, 86, CEPH_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_FALL_A):
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 82, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 4, 19, CEPH_BONE_TAIL_B);
        Em_PlaySeAtBoneOnFrame(em, 24, 9, CEPH_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 24, 17, CEPH_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_LIE_A):
        Em_PlaySeAtBoneOnFrame(em, 20, 8, CEPH_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 36, 4, CEPH_BONE_BREATH);
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 84, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 70, 8, CEPH_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 42, 18, CEPH_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_GET_UP_A):
        Em_PlaySeAtBoneOnFrame(em, 4, 32, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 62, 1, CEPH_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 56, 6, CEPH_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 4, 29, CEPH_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_FALL_B):
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 82, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 4, 19, CEPH_BONE_TAIL_B);
        Em_PlaySeAtBoneOnFrame(em, 24, 9, CEPH_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 24, 17, CEPH_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_LIE_DOWN):
        Em_PlaySeAtBoneOnFrame(em, 20, 8, CEPH_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 36, 4, CEPH_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame(em, 4, 44, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 70, 8, CEPH_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 42, 18, CEPH_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_STAND_STILL):
        Em_PlaySeTypeAtBoneOnFrame(em, 16, 82, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 102, 43, CEPH_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_HEAD_DOWN):
        Em_PlaySeAtBoneOnFrame(em, 12, 36, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 36, 34, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 102, 34, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 164, 34, CEPH_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, 95):
        break;
    case EM_MOTION_ID(0, 96):
        break;
    case EM_MOTION_ID(0, CEPH_MOT_SAND_IDLE):
        break;
    case EM_MOTION_ID(0, CEPH_MOT_SAND_SWIM_START):
        Em_PlaySeAtBoneOnFrame(em, 4, 101, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 4, 22, CEPH_BONE_TAIL_B);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_SAND_SWIM):
        Em_PlaySeAtBoneOnFrame(em, 4, 23, CEPH_BONE_TAIL_B);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_SAND_SWIM_FAST):
        Em_PlaySeAtBoneOnFrame(em, 4, 36, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 4, 22, CEPH_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 32, 21, CEPH_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 36, 21, CEPH_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 70, 23, CEPH_BONE_TAIL_B);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_SAND_RISE):
        Em_PlaySeTypeAtBoneInFrames(em, 20, 140, 86, CEPH_BONE_HEAD, 6, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 21, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 34, 48, CEPH_BONE_TAIL_B);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_SAND_DIVE):
        Em_PlaySeAtBoneOnFrame(em, 4, 21, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 30, 48, CEPH_BONE_TAIL_B);
        Em_PlaySeAtBoneOnFrame(em, 48, 48, CEPH_BONE_TAIL_B);
        Em_PlaySeAtBoneOnFrame(em, 4, 29, CEPH_BONE_BREATH);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_SAND_TURN_LEFT):
        Em_PlaySeAtBoneOnFrame(em, 4, 22, CEPH_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_SAND_TURN_RIGHT):
        Em_PlaySeAtBoneOnFrame(em, 4, 22, CEPH_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_LEAP_ATTACK):
        Em_PlaySeTypeAtBoneInFrames(em, 4, 124, 86, CEPH_BONE_HEAD, 6, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 22, CEPH_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 48, 48, CEPH_BONE_TAIL_B);
        Em_PlaySeAtBoneOnFrame(em, 4, 22, CEPH_BONE_BREATH);
        break;
    case EM_MOTION_ID(0, 111):
        break;
    case EM_MOTION_ID(0, CEPH_MOT_SURFACE_BREATH_C):
        Em_PlaySeTypeAtBoneOnFrame(em, 52, 81, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 52, CEPH_SE_BREATH, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 106, 22, CEPH_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 4, 22, CEPH_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 4, 25, CEPH_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_SURFACE_LUNGE):
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 91, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 2, 22, CEPH_BONE_TAIL_B);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_SAND_CIRCLE_RIGHT):
        Em_PlaySeAtBoneOnFrame(em, 4, 22, CEPH_BONE_TAIL_B);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_SAND_CIRCLE_LEFT):
        Em_PlaySeAtBoneOnFrame(em, 4, 22, CEPH_BONE_TAIL_B);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_SURFACE_BREATH_A):
        Em_PlaySeAtBoneOnFrame(em, 122, CEPH_SE_BREATH, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 162, CEPH_SE_BREATH, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 192, CEPH_SE_BREATH, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 26, 25, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 26, CEPH_SE_SPLASH, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 4, 40, CEPH_BONE_HEAD);
        Em_PlaySeTypeAtBoneOnFrame(em, 122, 81, CEPH_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_SURFACE_BREATH_B):
        Em_PlaySeAtBoneOnFrame(em, 122, CEPH_SE_BREATH, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 162, CEPH_SE_BREATH, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 192, CEPH_SE_BREATH, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 26, 25, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 26, CEPH_SE_SPLASH, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 4, 40, CEPH_BONE_HEAD);
        Em_PlaySeTypeAtBoneOnFrame(em, 122, 81, CEPH_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, 118):
        Em_PlaySeTypeAtBoneOnFrame(em, 38, 76, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 4, 22, CEPH_BONE_STEP_A);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_LEAP):
        Em_PlaySeAtBoneOnFrame(em, 4, 35, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 66, 19, CEPH_BONE_BREATH);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_LEAP_CROUCH):
        Em_PlaySeAtBoneOnFrame(em, 4, 22, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 30, 18, CEPH_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 30, 21, CEPH_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 30, 20, CEPH_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_FLOP_A):
        Em_PlaySeAtBoneOnFrame(em, 14, 16, CEPH_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 10, 9, CEPH_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 56, 8, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 56, 18, CEPH_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 30, 32, CEPH_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_BEACHED):
        Em_PlaySeAtBoneOnFrame(em, 4, 53, CEPH_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 4, 22, CEPH_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 50, 53, CEPH_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 70, 53, CEPH_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_FLOP_B):
        Em_PlaySeAtBoneOnFrame(em, 8, 52, CEPH_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 28, 21, CEPH_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 56, 9, CEPH_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_GET_UP):
        Em_PlaySeAtBoneOnFrame(em, 24, 18, CEPH_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 24, 21, CEPH_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 54, 0, CEPH_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 60, 4, CEPH_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 4, 22, CEPH_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_TUMBLE):
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 83, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 90, 48, CEPH_BONE_TAIL_B);
        break;
    case EM_MOTION_ID(0, 127):
        Em_PlaySeTypeAtBoneOnFrame(em, 26, 84, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 46, 8, CEPH_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 4, 20, CEPH_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 26, 46, CEPH_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_LIE_FLAP):
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 84, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 4, 16, CEPH_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 50, 9, CEPH_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 4, 29, CEPH_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_LIE_FLAP_B):
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 82, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 4, 16, CEPH_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 50, 9, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 4, 22, CEPH_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, 130):
        Em_PlaySeAtBoneOnFrame(em, 4, 29, CEPH_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 134, 29, CEPH_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 134, 47, CEPH_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_SAND_RECOVER):
        Em_PlaySeAtBoneOnFrame(em, 4, 23, CEPH_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 32, 22, CEPH_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 110, 21, CEPH_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, 132):
        break;
    case EM_MOTION_ID(0, CEPH_MOT_RUN_STOP):
        Em_PlaySeAtBoneOnFrame(em, 10, 8, CEPH_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 4, 31, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 10, 18, CEPH_BONE_STEP_A);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_SAND_SURFACED):
        Em_PlaySeAtBoneOnFrame(em, 4, 52, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 30, 48, CEPH_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 4, 22, CEPH_BONE_BREATH);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_SAND_BOTTOMED):
        Em_PlaySeAtBoneOnFrame(em, 4, 52, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 30, 48, CEPH_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 4, 22, CEPH_BONE_BREATH);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_LAND_ATTACK_B):
        Em_PlaySeAtBoneOnFrame(em, 4, 18, CEPH_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 18, 6, CEPH_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 102, 4, CEPH_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 216, 1, CEPH_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 264, 3, CEPH_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 56, 21, CEPH_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 56, 37, CEPH_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 4, 31, CEPH_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 84, 29, CEPH_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_SIDE_LIE):
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 87, CEPH_BONE_HEAD);
        Em_PlaySeTypeAtBoneOnFrame(em, 290, 85, CEPH_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_STATUS4_LYING):
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 82, CEPH_BONE_HEAD);
        Em_PlaySeTypeAtBoneOnFrame(em, 80, 81, CEPH_BONE_HEAD);
        Em_PlaySeTypeAtBoneOnFrame(em, 158, 84, CEPH_BONE_HEAD);
        
        break;
    }
}

/*
 * The family setup (the Em_TypeSetup entry for both kinds): a Cephadrome
 * (EM_KIND_CEPHADROME) in AREA_DESERT_6 or AREA_DESERT_9 leaves combat
 * (Em_SetCombat 0), reacts to nothing (noReact) and gets 5 frames of
 * senseDelay, noHitTimer and noPushTimer; otherwise nothing.
 */
void EmCephadrome_Setup(Em *em)
{
    if (em->type == EM_KIND_CEPHADROME) {
        switch (Area_DayId(em->area)) {
        case AREA_DESERT_6:
        case AREA_DESERT_9:
            Em_SetCombat(em, 0, 0);
            em->noReact = 1;
            em->senseDelay = 5;
            em->noHitTimer = 5;
            em->noPushTimer = 5;
            break;
        }
    }
}

/*
 * Callbacks slots 0 and 1: creates the alert mark (AlertMark_Create type 0
 * at CEPH_BONE_BREATH) and attached effect 0, as EmPlesioth_SpawnEffects.
 */
void EmCephadrome_SpawnEffects(void *em)
{
    AlertMark_Create(em, CEPH_BONE_BREATH, 0);
    Shadow_Spawn((Player *) em, SHADOW_MODEL);
}

/*
 * Sets a Cephadrome or Cephalos up (Start, Reinit): idle motion row 1,
 * scaled parts 3 and 4, full hp from the size (Em_ComputeMaxHp), the work
 * defaults and the four fins still, the depth below the sand surface
 * (surfaceDepth: 450 for the Cephadrome, 250 for the Cephalos); a Cephalos
 * is also made local (remote 0); the first time (fxFlags bit clear) hides
 * mesh 1 and creates the stage object of kind 9. Same role as
 * EmPlesioth_Init.
 */
void EmCephadrome_Init(Em *em)
{
    EmCephadromeWork *w = (EmCephadromeWork *)&em->work;

    s16 maxHp;
    u8 fxFlags;

    Em_SetMotionRow(em, CEPH_MOT_IDLE, 0, 0);
    Em_SetPartScaled(em, 3);
    Em_SetPartScaled(em, 4);
    w->unk010 = 0;
    em->scriptPending = 1;
    em->blind = 0;
    maxHp = Em_ComputeMaxHp(em, 0);
    em->hp = maxHp;
    em->hpMax = maxHp;
    if (em->type == EM_KIND_CEPHADROME) {
        em->unk_InitFlag = 1;
    } else {
        em->remote = 0;
        em->unk_InitFlag = 0;
    }
    em->noAlert = 0;
    em->painted = 0;
    w->turnYaw = ANGLE_TURN / 4;
    w->unk028 = 256;
    w->unk02C = 512;
    w->unk030 = 256;
    w->unk018 = 8192;
    w->unk014 = 0;
    w->unk038 = 0;
    w->unk039 = 0;
    w->unk03A = 0;
    w->finMode = 0;
    w->finWait[0] = 0;
    w->finState[0][0] = 0;
    w->finState[0][1] = 0;
    w->finRot[0].fold = 0;
    w->finRot[0].wave = 0;
    w->finWait[1] = 0;
    w->finState[1][0] = 0;
    w->finState[1][1] = 0;
    w->finRot[1].fold = 0;
    w->finRot[1].wave = 0;
    w->finWait[2] = 0;
    w->finState[2][0] = 0;
    w->finState[2][1] = 0;
    w->finRot[2].fold = 0;
    w->finRot[2].wave = 0;
    w->finWait[3] = 0;
    w->finState[3][0] = 0;
    w->finState[3][1] = 0;
    w->finRot[3].fold = 0;
    w->finRot[3].wave = 0;
    if (em->type == EM_KIND_CEPHADROME) {
        *(f32 *) &em->surfaceDepth = 450.0f;
    } else {
        *(f32 *) &em->surfaceDepth = 250.0f;
    }
    fxFlags = em->fxFlags & 1;
    em->fxFlags = fxFlags;
    if (fxFlags == 0) {
        switch (em->type) {
        case EM_KIND_CEPHADROME:
        case EM_KIND_CEPHALOS:
            em->meshShown[1] = 0;
            EmTail_Create(em);
            break;
        }
    }
}

/*
 * Starts a monster of the family (the per-kind start in gm_em): Init, then
 * in the quest-less stage (gQuest.stage 0) a fixed spot 1500 apart per slot
 * on the ground (EM_STANCE_GROUND), otherwise under the sand
 * (EM_STANCE_UNDER); either way into the sand idle (CEPH_SAND_IDLE).
 */
void EmCephadrome_Start(Em *em)
{
    f32 fslot;
    u8 slotNo;

    EmCephadrome_Init(em);
    if (gQuest.stage == 0) {
        slotNo = em->slotNo;
        fslot = (f32) (u32) slotNo;
        em->posX = 5000.0f + (1500.0f * fslot);
        em->posY = 0.0f;
        em->posZ = 9000.0f;
        em->stance = EM_STANCE_GROUND;
        EmCephadrome_ChangeState(em, EM_GROUP_FLY, CEPH_SAND_IDLE, EM_SYNC_LOCAL);
        return;
    }
    em->stance = EM_STANCE_UNDER;
    EmCephadrome_ChangeState(em, EM_GROUP_FLY, CEPH_SAND_IDLE, EM_SYNC_LOCAL);
}

/* Callbacks slot 5: Init again without changing the state. */
void EmCephadrome_Reinit(Em *em)
{
    EmCephadrome_Init(em);
}

/*
 * Ends a state on land: speed 1, stance ground, no look-at, asks the script
 * for the next state (scriptPending) and goes to CEPH_IDLE (EM_SYNC_LOCAL).
 * Also called by Update when EmCephadrome_IsUnderOffSand says so.
 */
void EmCephadrome_EndGroundState(Em *em)
{
    em->motionSpeed = 1.0f;
    em->stance = EM_STANCE_GROUND;
    em->lookAtTarget = 0;
    em->scriptPending = 1;
    EmCephadrome_ChangeState(em, EM_GROUP_IDLE, CEPH_IDLE, EM_SYNC_LOCAL);
}

/*
 * Ends a state under the sand: speed 1, stance EM_STANCE_UNDER, no look-at,
 * asks the script (scriptPending) and goes to the sand idle (CEPH_SAND_IDLE).
 */
void EmCephadrome_EndSandState(Em *em)
{
    em->motionSpeed = 1.0f;
    em->stance = EM_STANCE_UNDER;
    em->lookAtTarget = 0;
    em->scriptPending = 1;
    EmCephadrome_ChangeState(em, EM_GROUP_FLY, CEPH_SAND_IDLE, EM_SYNC_LOCAL);
}

/*
 * CEPH_IDLE_STAND: puts the three layers on their idle loops
 * (EM_MOTION_ID(layer, CEPH_MOT_IDLE), blend 10) and stands until the timer
 * runs out, then asks the script for the next state (scriptPending); never
 * ends by itself.
 */
void EmCephadrome_IdleStand(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        if (em->motionId[0] != EM_MOTION_ID(0, CEPH_MOT_IDLE)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(0, CEPH_MOT_IDLE), 10, 0, 0);
        }
        if (em->motionId[1] != EM_MOTION_ID(1, CEPH_MOT_IDLE)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(1, CEPH_MOT_IDLE), 10, 0, 1);
        }
        if (em->motionId[2] != EM_MOTION_ID(2, CEPH_MOT_IDLE)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(2, CEPH_MOT_IDLE), 10, 0, 2);
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
 * CEPH_IDLE: on the ground, the idle loops of the three layers once (no
 * blend), then EndGroundState (back to CEPH_IDLE with the script asked)
 * once the monster is local; the state every land action falls back to.
 */
void EmCephadrome_Idle(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        if (em->motionId[0] != EM_MOTION_ID(0, CEPH_MOT_IDLE)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(0, CEPH_MOT_IDLE), 0, 0, 0);
        }
        if (em->motionId[1] != EM_MOTION_ID(1, CEPH_MOT_IDLE)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(1, CEPH_MOT_IDLE), 0, 0, 1);
        }
        if (em->motionId[2] != EM_MOTION_ID(2, CEPH_MOT_IDLE)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(2, CEPH_MOT_IDLE), 0, 0, 2);
        }
        break;
    case 1:
        if ((em->remote == 0) && (em->motion[0].playing == 0)) {
            em->step++;
            EmCephadrome_EndGroundState(em);
        }
        break;
    }
}

/*
 * CEPH_IDLE_ACTION: plays CEPH_MOT_IDLE_ACTION, then the idle motion once,
 * then ends on land.
 */
void EmCephadrome_IdleAction(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, CEPH_MOT_IDLE_ACTION, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, CEPH_MOT_IDLE, 0, 92);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmCephadrome_EndGroundState(em);
        }
        break;
    }
}

/*
 * CEPH_IDLE_EAT_NEED2: lowers its head (CEPH_MOT_HEAD_DOWN, sight row 1,
 * reaction set 1, no flinch) with dust clouds and a sound on given frames,
 * then feeds (CEPH_MOT_FEED) gaining 5% of max hp; refills need 2's gauge
 * (Em_AddNeed2) every frame; at the end restores sight row and reaction set
 * 0, clears need 2's state (Em_ClearNeed2State) and ends on land.
 */
void EmCephadrome_EatNeed2(Em *em, EmWork *work)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_NO_FLINCH);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, CEPH_MOT_HEAD_DOWN, 0, 0);
        Em_SelectSightRow(em, 1);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if ((EmMotion_PassedFrameScaled(em, 56.0f, 0U) != 0) || (EmMotion_PassedFrameScaled(em, 108.0f, 0U) != 0) || (EmMotion_PassedFrameScaled(em, 126.0f, 0U) != 0) || (EmMotion_PassedFrameScaled(em, 138.0f, 0U) != 0) || (EmMotion_PassedFrameScaled(em, 170.0f, 0U) != 0)) {
            Em_PlaySeAtBone(em, 38, CEPH_BONE_HEAD);
            DustCloud_Create(em, 7, 0, 5.0f);
        }
        if ((EmMotion_PassedFrameScaled(em, 210.0f, 0U) != 0) || (EmMotion_PassedFrameScaled(em, 218.0f, 0U) != 0)) {
            DustCloud_Create(em, 7, 0, 2.5f);
        }
        if (EmMotion_PassedFrameScaled(em, 52.0f, 0U) != 0) {
            DustCloud_Create(em, 8, 0, 1.0f);
        }
        if ((EmMotion_PassedFrameScaled(em, 106.0f, 0U) != 0) || (EmMotion_PassedFrameScaled(em, 130.0f, 0U) != 0)) {
            DustCloud_Create(em, 6, 0, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 124.0f, 0U) != 0) {
            DustCloud_Create(em, 6, 1, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 170.0f, 0U) != 0) {
            DustCloud_Create(em, 15, 0, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 192.0f, 0U) != 0) {
            DustCloud_Create(em, 16, 0, 1.0f);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, CEPH_MOT_FEED, 0, 0);
            Em_AddHp(em, (s32) (0.05f * (f32) em->hpMax));
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SelectSightRow(em, 0);
            Em_LoadReactionSet(em, 0);
            Em_ClearNeed2State(em);
            EmCephadrome_EndGroundState(em);
        }
        break;
    }
    Em_AddNeed2(em, 222);
}

/*
 * CEPH_IDLE_LAND_FLOP_A, CEPH_IDLE_LAND_FLOP_B: landed from a leap: a flop
 * (CEPH_MOT_FLOP_A, or CEPH_MOT_FLOP_B for variant 1), lies beached on its
 * side (CEPH_MOT_BEACHED, stance EM_STANCE_FAMILY), gets up
 * (CEPH_MOT_GET_UP, stance ground) and ends on land.
 */
void EmCephadrome_LandFlop(Em *em, EmWork *work, u8 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        switch (variant) {
        default:
            Em_SetMotion(em, CEPH_MOT_FLOP_A, 0, 0);
            break;
        case 1:
            Em_SetMotion(em, CEPH_MOT_FLOP_B, 0, 0);
            break;
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stance = EM_STANCE_FAMILY;
            Em_SetMotion(em, CEPH_MOT_BEACHED, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, CEPH_MOT_GET_UP, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmCephadrome_EndGroundState(em);
        }
        break;
    }
}

/*
 * CEPH_IDLE_STATUS2_COLLAPSE, CEPH_IDLE_STATUS1_COLLAPSE: collapses
 * (CEPH_MOT_STATUS_COLLAPSE, reaction set 1) and lies (CEPH_MOT_STATUS_LIE)
 * blind, woken by a hit (EM_HIT_REACT_WAKE), until the timer runs out; then
 * CEPH_IDLE_STATUS2_END (variant 0) or CEPH_IDLE_STATUS1_END (variant 1); a
 * remote copy retries the change once it is local (remote 0).
 */
void EmCephadrome_StatusCollapse(Em *em, s32 work, s32 variant)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, CEPH_MOT_STATUS_COLLAPSE, 0, 0);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, CEPH_MOT_STATUS_LIE, 0, 0);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            if (variant == 0) {
                EmCephadrome_ChangeState(em, EM_GROUP_IDLE, CEPH_IDLE_STATUS2_END, EM_SYNC_OWNER_FLAGGED);
                break;
            }
            EmCephadrome_ChangeState(em, EM_GROUP_IDLE, CEPH_IDLE_STATUS1_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 3:
        if (em->remote == 0) {
            if (variant == 0) {
                EmCephadrome_ChangeState(em, EM_GROUP_IDLE, CEPH_IDLE_STATUS2_END, EM_SYNC_OWNER_FLAGGED);
                break;
            }
            EmCephadrome_ChangeState(em, EM_GROUP_IDLE, CEPH_IDLE_STATUS1_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * CEPH_IDLE_STATUS2_END, CEPH_IDLE_STATUS1_END: gets up after the status
 * (CEPH_MOT_STATUS_GET_UP, reaction set 0) kept blind, then
 * CEPH_IDLE_SHAKE_OFF.
 */
void EmCephadrome_StatusGetUp(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, CEPH_MOT_STATUS_GET_UP, 0, 0);
        Em_LoadReactionSet(em, 0);
        em->blind = 2;
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            EmCephadrome_ChangeState(em, EM_GROUP_IDLE, CEPH_IDLE_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmCephadrome_ChangeState(em, EM_GROUP_IDLE, CEPH_IDLE_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * CEPH_IDLE_SHAKE_OFF, CEPH_IDLE_STATUS4_SHAKE_OFF,
 * CEPH_IDLE_HOLD2_SHAKE_OFF, CEPH_IDLE_STUN_SHAKE_OFF: shakes off
 * (CEPH_MOT_SHAKE_OFF) and ends on land.
 */
void EmCephadrome_ShakeOff(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, CEPH_MOT_SHAKE_OFF, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmCephadrome_EndGroundState(em);
        }
        break;
    }
}

/*
 * CEPH_IDLE_STATUS2_COLLAPSE_BEACHED: under status 2 while beached (stance
 * EM_STANCE_FAMILY): lies down (CEPH_MOT_LIE_DOWN), flaps
 * (CEPH_MOT_LIE_FLAP) and lies on its side (CEPH_MOT_SIDE_LIE) blind, woken
 * by a hit, with sand puffs at CEPH_BONE_BREATH (Em_SpawnLyingPuffs) and a sound
 * every 135 frames, until the timer runs out; then
 * CEPH_IDLE_STATUS2_END_BEACHED.
 */
void EmCephadrome_Status2CollapseBeached(Em *em, EmWork *work)
{
    f32 puffOfs[3];
    f32 puffOfs2[3];

    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_FAMILY;
        em->lookAtTarget = 0;
        Em_SetMotion(em, CEPH_MOT_LIE_DOWN, 0, 0);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, CEPH_MOT_LIE_FLAP, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, CEPH_MOT_SIDE_LIE, 0, 0);
        }
        break;
    case 3:
        if (--em->timer <= 0) {
            em->step++;
            EmCephadrome_ChangeState(em, EM_GROUP_IDLE, CEPH_IDLE_STATUS2_END_BEACHED, EM_SYNC_OWNER_FLAGGED);
        }
        puffOfs[0] = 0.0f;
        puffOfs[1] = 10.0f;
        puffOfs[2] = 140.0f;
        Em_SpawnLyingPuffs(em, CEPH_BONE_BREATH, puffOfs, 1.6f);
        if (em->timer % 135 == 0) {
            Em_PlaySeAtBone(em, 87, CEPH_BONE_HEAD);
        }
        break;
    case 4:
        if (em->remote == 0) {
            EmCephadrome_ChangeState(em, EM_GROUP_IDLE, CEPH_IDLE_STATUS2_END_BEACHED, EM_SYNC_OWNER_FLAGGED);
        }
        puffOfs2[0] = 0.0f;
        puffOfs2[1] = 10.0f;
        puffOfs2[2] = 140.0f;
        Em_SpawnLyingPuffs(em, CEPH_BONE_BREATH, puffOfs2, 1.6f);
        if (em->timer % 135 == 0) {
            Em_PlaySeAtBone(em, 87, CEPH_BONE_HEAD);
        }
        break;
    }
}

/*
 * CEPH_IDLE_STATUS2_END_BEACHED, CEPH_IDLE_STATUS1_END_BEACHED: gets up
 * from its side (CEPH_MOT_GET_UP, reaction set 0) kept blind, then
 * CEPH_IDLE_SHAKE_OFF.
 */
void EmCephadrome_StatusGetUpBeached(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, CEPH_MOT_GET_UP, 0, 0);
        Em_LoadReactionSet(em, 0);
        em->blind = 2;
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            EmCephadrome_ChangeState(em, EM_GROUP_IDLE, CEPH_IDLE_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmCephadrome_ChangeState(em, EM_GROUP_IDLE, CEPH_IDLE_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * CEPH_IDLE_STATUS1_COLLAPSE_BEACHED: as Status2CollapseBeached under
 * status 1 (stance ground), then CEPH_IDLE_STATUS1_END_BEACHED.
 */
void EmCephadrome_Status1CollapseBeached(Em *em, EmWork *work)
{
    f32 puffOfs[3];
    f32 puffOfs2[3];

    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, CEPH_MOT_LIE_DOWN, 0, 0);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, CEPH_MOT_LIE_FLAP, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, CEPH_MOT_SIDE_LIE, 0, 0);
        }
        break;
    case 3:
        if (--em->timer <= 0) {
            em->step++;
            EmCephadrome_ChangeState(em, EM_GROUP_IDLE, CEPH_IDLE_STATUS1_END_BEACHED, EM_SYNC_OWNER_FLAGGED);
        }
        puffOfs[0] = 0.0f;
        puffOfs[1] = 10.0f;
        puffOfs[2] = 140.0f;
        Em_SpawnLyingPuffs(em, CEPH_BONE_BREATH, puffOfs, 1.6f);
        if (em->timer % 135 == 0) {
            Em_PlaySeAtBone(em, 87, CEPH_BONE_HEAD);
        }
        break;
    case 4:
        if (em->remote == 0) {
            EmCephadrome_ChangeState(em, EM_GROUP_IDLE, CEPH_IDLE_STATUS1_END_BEACHED, EM_SYNC_OWNER_FLAGGED);
        }
        puffOfs2[0] = 0.0f;
        puffOfs2[1] = 10.0f;
        puffOfs2[2] = 140.0f;
        Em_SpawnLyingPuffs(em, CEPH_BONE_BREATH, puffOfs2, 1.6f);
        if (em->timer % 135 == 0) {
            Em_PlaySeAtBone(em, 87, CEPH_BONE_HEAD);
        }
        break;
    }
}

/*
 * CEPH_IDLE_REST: lies down to rest (CEPH_MOT_REST_START, reaction set 1)
 * and rests (CEPH_MOT_REST) blind for 9000 frames, regaining 1 hp every 6
 * frames up to 30% of max hp (Em_RegenHp); then clears the low-hp need
 * (Em_ClearLowHpNeed) and goes to CEPH_IDLE_REST_END; a hit
 * (EM_DMG_HIT_LYING in Update) ends it early.
 */
void EmCephadrome_Rest(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, CEPH_MOT_REST_START, 0, 0);
        Em_LoadReactionSet(em, 1);
        em->timer = 9000;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, CEPH_MOT_REST, 0, 0);
        }
        break;
    case 2:
        Em_RegenHp(em, 1, (s32) (0.3f * (f32) em->hpMax), 6);
        if (--em->timer <= 0) {
            em->step++;
            Em_ClearLowHpNeed(em);
            EmCephadrome_ChangeState(em, EM_GROUP_IDLE, CEPH_IDLE_REST_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmCephadrome_ChangeState(em, EM_GROUP_IDLE, CEPH_IDLE_REST_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * CEPH_IDLE_REST_END: gets up from the rest (CEPH_MOT_REST_END, reaction
 * set 0) kept blind, then CEPH_IDLE_SHAKE_OFF.
 */
void EmCephadrome_RestEnd(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, CEPH_MOT_REST_END, 0, 0);
        Em_LoadReactionSet(em, 0);
        em->blind = 2;
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            EmCephadrome_ChangeState(em, EM_GROUP_IDLE, CEPH_IDLE_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmCephadrome_ChangeState(em, EM_GROUP_IDLE, CEPH_IDLE_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * CEPH_IDLE_EAT_START: lowers its head (CEPH_MOT_HEAD_DOWN, sight row 1,
 * reaction set 1, no flinch) and at frame 200 goes on with CEPH_IDLE_EAT,
 * the motion still playing.
 */
void EmCephadrome_EatStart(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_NO_FLINCH);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, CEPH_MOT_HEAD_DOWN, 0, 0);
        Em_SelectSightRow(em, 1);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 200.0f, 0U) != 0) {
            em->step++;
            EmCephadrome_ChangeState(em, EM_GROUP_IDLE, CEPH_IDLE_EAT, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmCephadrome_ChangeState(em, EM_GROUP_IDLE, CEPH_IDLE_EAT, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * CEPH_IDLE_EAT: restores sight row and reaction set 0, lets
 * CEPH_MOT_HEAD_DOWN finish, feeds once (CEPH_MOT_FEED) and ends on land;
 * entering it starts the goal action (Em_StartGoalAction).
 */
void EmCephadrome_Eat(Em *em)
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
            Em_SetMotion(em, CEPH_MOT_FEED, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmCephadrome_EndGroundState(em);
        }
        break;
    }
}

/*
 * CEPH_MOVE_WALK: walks (CEPH_MOT_WALK) to the goal (Em_StepToGoal) and
 * ends on land.
 */
void EmCephadrome_Walk(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, CEPH_MOT_WALK, 0, 0);
        break;
    case 1:
        if (Em_StepToGoal(em, 64, 0) == 1) {
            em->step++;
            EmCephadrome_EndGroundState(em);
        }
        break;
    }
}

/*
 * CEPH_MOVE_RUN: runs (CEPH_MOT_RUN) to the goal (Em_StepToGoal), stops
 * (CEPH_MOT_RUN_STOP) and ends on land.
 */
void EmCephadrome_RunAndStop(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, CEPH_MOT_RUN, 0, 0);
        break;
    case 1:
        if (Em_StepToGoal(em, 64, 0) == 1) {
            em->step++;
            Em_SetMotion(em, CEPH_MOT_RUN_STOP, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmCephadrome_EndGroundState(em);
        }
        break;
    }
}

/*
 * CEPH_MOVE_TURN: turns toward the target with the land turn row
 * (EmCephadrome_TurnParams[0]: Em_StartTurnToTarget, Em_TurnStep), clears
 * EM_FLAG_NO_ROOT_MOTION and ends on land.
 */
void EmCephadrome_TurnToTarget(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        Em_StartTurnToTarget(em, &EmCephadrome_TurnParams[0]);
        break;
    case 1:
        if (Em_TurnStep(em, &EmCephadrome_TurnParams[0]) == 1) {
            em->step++;
            Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
            EmCephadrome_EndGroundState(em);
        }
        break;
    }
}

/*
 * CEPH_MOVE_LIMP: limps (CEPH_MOT_LIMP) to the goal (Em_StepToGoal) and
 * ends on land; SetMoveState turns a walk into it below a tenth of max
 * hp.
 */
void EmCephadrome_Limp(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, CEPH_MOT_LIMP, 0, 0);
        break;
    case 1:
        if (Em_StepToGoal(em, 64, 0) == 1) {
            em->step++;
            EmCephadrome_EndGroundState(em);
        }
        break;
    }
}

/*
 * CEPH_SAND_IDLE: lies under the sand (stance EM_STANCE_UNDER) playing
 * CEPH_MOT_SAND_IDLE once, then (owner only) EndSandState, which comes back
 * here with the script asked; the start state (EmCephadrome_Start) and the
 * state every sand action falls back to.
 */
void EmCephadrome_SandIdle(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, CEPH_MOT_SAND_IDLE, 0, 0);
        em->stance = EM_STANCE_UNDER;
        break;
    case 1:
        if ((em->remote == 0) && (em->motion[0].playing == 0)) {
            em->step++;
            EmCephadrome_EndSandState(em);
        }
        break;
    }
}

/* CEPH_SAND_IDLE_B: as SandIdle with CEPH_MOT_SAND_IDLE_B, then EndSandState. */
void EmCephadrome_SandIdleB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_UNDER;
        Em_SetMotion(em, CEPH_MOT_SAND_IDLE_B, 0, 0);
        break;
    case 1:
        if ((em->remote == 0) && (em->motion[0].playing == 0)) {
            em->step++;
            EmCephadrome_EndSandState(em);
        }
        break;
    }
}

/*
 * CEPH_SAND_CIRCLE_RIGHT_B: as SandCircleRight (132 frames at speed 40,
 * heading +0x1F0 a frame, CEPH_MOT_SAND_CIRCLE_RIGHT, pitched toward the
 * goal, sand wake), but rotY is not wrapped to 16 bits; then EndSandState.
 */
void EmCephadrome_SandCircleRightB(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_UNDER;
        Em_SetMotion(em, CEPH_MOT_SAND_CIRCLE_RIGHT, 0, 0);
        em->timer = 132;
        Player_ClearVelocity(em);
        em->velZ = 40.0f;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmCephadrome_EndSandState(em);
        }
        em->rotY += 0x1F0;
        Em_GetPitchToGoal(em, (s32 *) &work->moveAngBX, 0.0f, 0.0f);
        work->moveAngBY = em->rotY;
        work->moveAngBZ = 0;
        Em_MoveRotated(em, &work->moveAngBX);
        EmCephadrome_SpawnWakeEvery4(em, 8.0f);
        break;
    }
}

/*
 * CEPH_SAND_TURN (variant 0), CEPH_SAND_TURN_NO_REACT (variant 1): turns
 * under the sand toward the target (EmCephadrome_TurnParams[1]) with a sand
 * splash (SpawnSurfaceSplash) on entry, then EndSandState; variant 1 sets
 * noReact and 5 frames of senseDelay, noHitTimer and noPushTimer every
 * frame.
 */
void EmCephadrome_SandTurn(Em *em, s32 unused, s32 variant)
{
    if ((variant & 0xFF) == 1) {
        em->noReact = 1;
        em->senseDelay = 5;
        em->noHitTimer = 5;
        em->noPushTimer = 5;
    }
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_UNDER;
        Em_StartTurnToTarget(em, &EmCephadrome_TurnParams[1]);
        EmCephadrome_SpawnSurfaceSplash(em, 8.0f);
        break;
    case 1:
        if (Em_TurnStep(em, &EmCephadrome_TurnParams[1]) == 1) {
            em->step++;
            EmCephadrome_EndSandState(em);
        }
        break;
    }
}

/*
 * CEPH_SAND_FORWARD: swims under the sand toward the goal pitched to it
 * (Em_GetPitchToGoal, Em_MoveRotated): CEPH_MOT_SAND_SWIM_START
 * accelerating from 5 to at most 50, then CEPH_MOT_SAND_SWIM; ends at the
 * goal (Em_StepToGoal) or once goalDist is used up (EndSandState).
 * SetSandState sets the speed scale on entry.
 */
void EmCephadrome_SandForward(Em *em, EmWork *work)
{

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_UNDER;
        Em_SetMotion(em, CEPH_MOT_SAND_SWIM_START, 0, 0);
        Player_ClearVelocity(em);
        em->velZ = 5.0f;
        em->accelZ = 1.0f;
        break;
    case 1:
        Em_GetPitchToGoal(em, (s32 *) &work->moveAngBX, 0.0f, 0.0f);
        work->moveAngBY = em->rotY;
        work->moveAngBZ = 0;
        Em_MoveRotated(em, &work->moveAngBX);
        if (!(em->velZ < 50.0f)) {
            em->velZ = 50.0f;
        }
        if (Em_StepToGoal(em, 64, 2) == 1) {
            em->step = 10;
            EmCephadrome_EndSandState(em);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, CEPH_MOT_SAND_SWIM, 0, 0);
        }
        break;
    case 2:
        Em_GetPitchToGoal(em, (s32 *) &work->moveAngBX, 0.0f, 0.0f);
        work->moveAngBY = em->rotY;
        work->moveAngBZ = 0;
        Em_MoveRotated(em, &work->moveAngBX);
        if (!(em->velZ < 50.0f)) {
            em->velZ = 50.0f;
        }
        em->goalDist -= em->velZ;
        if (em->goalDist <= 0.0f) {
            em->step++;
            EmCephadrome_EndSandState(em);
        }
        break;
    }
}

/*
 * CEPH_SAND_DASH (variant 0), CEPH_SAND_DASH_NO_REACT (variant 1): fast
 * swim (CEPH_MOT_SAND_SWIM_FAST) toward the goal from motion frame 30 at up
 * to 100 with the sand wake (SpawnWakeEvery4), ending at the goal;
 * variant 1 sets noReact and the 5-frame sense, hit and push timers every
 * frame.
 */
void EmCephadrome_SandDash(Em *em, EmWork *work, s32 variant)
{
    if ((variant & 0xFF) == 1) {
        em->noReact = 1;
        em->senseDelay = 5;
        em->noHitTimer = 5;
        em->noPushTimer = 5;
    }
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_UNDER;
        Em_SetMotion(em, CEPH_MOT_SAND_SWIM_FAST, 0, 0);
        Player_ClearVelocity(em);
        em->velZ = 30.0f;
        em->accelZ = 10.0f;
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 30.0f) != 0) {
            Em_GetPitchToGoal(em, (s32 *) &work->moveAngBX, 0.0f, 0.0f);
            work->moveAngBY = em->rotY;
            work->moveAngBZ = 0;
            Em_MoveRotated(em, &work->moveAngBX);
            if (!(em->velZ < 100.0f)) {
                em->velZ = 100.0f;
            }
            if (Em_StepToGoal(em, 64, 2) == 1) {
                em->step++;
                EmCephadrome_EndSandState(em);
            }
            EmCephadrome_SpawnWakeEvery4(em, 8.0f);
        }
        break;
    }
}

/*
 * CEPH_SAND_CURVE: plays CEPH_MOT_SAND_CURVE moved by curve set 0
 * (StartCurves, UpdateCurves), then EndSandState; SetSandState turns this
 * state into CEPH_SAND_DIVE, so it never runs through ChangeState.
 */
void EmCephadrome_SandCurve(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_UNDER;
        Em_SetMotion(em, CEPH_MOT_SAND_CURVE, 0, 0);
        EmCephadrome_StartCurves(em, 0U);
        break;
    case 1:
        if (EmCephadrome_UpdateCurves(em)) {
            em->step++;
            EmCephadrome_EndSandState(em);
        }
        break;
    }
}

/*
 * CEPH_SAND_RISE: rises (CEPH_MOT_SAND_RISE, velY up to 50) to the swim
 * height under the sand top (surfaceY - surfaceDepth), then plays
 * CEPH_MOT_SAND_SURFACED with one sand wake call and ends under the sand
 * (EndSandState).
 */
void EmCephadrome_SandRise(Em *em, EmWork *work)
{
    f32 swimY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_UNDER;
        Em_SetMotion(em, CEPH_MOT_SAND_RISE, 0, 0);
        Player_ClearVelocity(em);
        em->velZ = 20.0f;
        em->velY = 2.0f;
        em->accelY = 4.0f;
        work->moveAngBX = 0;
        work->moveAngBZ = 0;
        break;
    case 1:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, (s32 *) &work->moveAngBX);
        if (!(em->velY <= 50.0f)) {
            em->velY = 50.0f;
            em->accelY = 0.0f;
        }
        swimY = em->surfaceY - *(f32 *)&em->surfaceDepth;
        if (!(em->posY <= swimY)) {
            em->posY = swimY;
            em->step++;
            Em_SetMotion(em, CEPH_MOT_SAND_SURFACED, 0, 0);
            EmCephadrome_SpawnWakeEvery4(em, 8.0f);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmCephadrome_EndSandState(em);
        }
        break;
    }
}

/*
 * CEPH_SAND_DIVE: sinks (CEPH_MOT_SAND_DIVE, velY down to -50) to floorY,
 * plays CEPH_MOT_SAND_BOTTOMED and ends under the sand.
 */
void EmCephadrome_SandDive(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_UNDER;
        Em_SetMotion(em, CEPH_MOT_SAND_DIVE, 0, 0);
        Player_ClearVelocity(em);
        em->velZ = 20.0f;
        em->velY = -2.0f;
        em->accelY = -4.0f;
        work->moveAngBX = 0;
        work->moveAngBZ = 0;
        break;
    case 1:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, &work->moveAngBX);
        if (em->velY < -50.0f) {
            em->velY = -50.0f;
            em->accelY = 0.0f;
        }
        floorY = em->floorY;
        if (em->posY < floorY) {
            em->posY = floorY;
            em->step++;
            Em_SetMotion(em, CEPH_MOT_SAND_BOTTOMED, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmCephadrome_EndSandState(em);
        }
        break;
    }
}

/*
 * CEPH_SAND_ROUND_TURN: turns toward the next area point
 * (EmCephadrome_TurnParams[2]) with a sand splash, then
 * CEPH_SAND_ROUND_NEXT; SetSandState picks it for a round whose goal is
 * not ahead. A third step re-sends ROUND_NEXT every 300 frames (owner
 * only).
 */
void EmCephadrome_SandRoundTurn(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_UNDER;
        Em_StartTurnToTarget(em, &EmCephadrome_TurnParams[2]);
        EmCephadrome_SpawnSurfaceSplash(em, 8.0f);
        break;
    case 1:
        if (Em_TurnStep(em, &EmCephadrome_TurnParams[2]) == 1) {
            em->step++;
            em->timer = 300;
            EmCephadrome_ChangeState(em, EM_GROUP_FLY, CEPH_SAND_ROUND_NEXT, EM_SYNC_SEND);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->timer = 300;
            if (em->remote == 0) {
                EmCephadrome_ChangeState(em, EM_GROUP_FLY, CEPH_SAND_ROUND_NEXT, EM_SYNC_SEND);
            }
        }
        break;
    }
}

/*
 * CEPH_SAND_ROUND_NEXT, CEPH_SAND_ROUND_NEXT_LEG: fast swim
 * (CEPH_MOT_SAND_SWIM_FAST, at least speed 50, 40 for half of the Cephalos
 * by stateRand) turning to the goal with the sand wake; each leg ends
 * within 300 of the goal or when em->timer runs out, counts roundLegsLeft
 * down and, while legs are left, steps the area point forward
 * (StepAreaPoint 0), times the next leg (1.5 x distance / 50) and enters
 * CEPH_SAND_ROUND_NEXT_LEG; ends under the sand at 0.
 */
void EmCephadrome_SandRoundNext(Em *em, EmWork *work)
{
    f32 swimSpeed;
    u8 legsLeft;

    switch (em->step) {
    case 0:
        em->stance = EM_STANCE_UNDER;
        Em_SetMotion(em, CEPH_MOT_SAND_SWIM_FAST, 0, 0);
        em->step++;
        em->stateVar1 = 0;
        em->lookAtTarget = 0;
        em->velX = 0.0f;
        em->velY = 0.0f;
        Player_ClearAccel2(em);
        swimSpeed = 50.0f;
        if (em->type == EM_KIND_CEPHALOS && (em->stateRand & 0x1F) < 16) {
            swimSpeed *= 0.8f;
        }
        if (em->velZ <= swimSpeed) {
            em->velZ = swimSpeed;
        }
        break;
    case 1:
        if (Vec_DistanceXZ(&em->posX, em->goal) <= 300.0f || --em->timer <= 0) {
            legsLeft = ((EmCephadromeWork *)work)->roundLegsLeft - 1;
            ((EmCephadromeWork *)work)->roundLegsLeft = legsLeft;
            if ((legsLeft & 0xFF) <= 0) {
                em->step++;
                EmCephadrome_EndSandState(em);
            } else {
                EmCephadrome_StepAreaPoint(em, 0);
                Em_ComputeGoal2(em, em->goal);
                em->timer = (s32) ((1.5f * Vec_DistanceXZ(&em->posX, em->goal)) / 50.0f);
                EmCephadrome_ChangeState(em, EM_GROUP_FLY, CEPH_SAND_ROUND_NEXT_LEG, EM_SYNC_LOCAL);
            }
        }
        Em_TurnToGoal(em, 512, 0);
        Em_GetPitchToGoal(em, (s32 *) &work->moveAngBX, 0.0f, 0.0f);
        work->moveAngBY = em->rotY;
        work->moveAngBZ = 0;
        Em_MoveRotated(em, &work->moveAngBX);
        EmCephadrome_SpawnWakeEvery4(em, 8.0f);
        break;
    }
}

/*
 * CEPH_SAND_ROUND_PREV, CEPH_SAND_ROUND_PREV_LEG: as SandRoundNext,
 * stepping the area point backward (StepAreaPoint 1) and continuing in
 * CEPH_SAND_ROUND_PREV_LEG.
 */
void EmCephadrome_SandRoundPrev(Em *em, EmWork *work)
{
    f32 swimSpeed;
    u8 legsLeft;

    switch (em->step) {
    case 0:
        em->stance = EM_STANCE_UNDER;
        Em_SetMotion(em, CEPH_MOT_SAND_SWIM_FAST, 0, 0);
        em->step++;
        em->stateVar1 = 0;
        em->lookAtTarget = 0;
        em->velX = 0.0f;
        em->velY = 0.0f;
        Player_ClearAccel2(em);
        swimSpeed = 50.0f;
        if (em->type == EM_KIND_CEPHALOS && (em->stateRand & 0x1F) < 16) {
            swimSpeed *= 0.8f;
        }
        if (em->velZ <= swimSpeed) {
            em->velZ = swimSpeed;
        }
        break;
    case 1:
        if (Vec_DistanceXZ(&em->posX, em->goal) <= 300.0f || --em->timer <= 0) {
            legsLeft = ((EmCephadromeWork *)work)->roundLegsLeft - 1;
            ((EmCephadromeWork *)work)->roundLegsLeft = legsLeft;
            if ((legsLeft & 0xFF) <= 0) {
                em->step++;
                EmCephadrome_EndSandState(em);
            } else {
                EmCephadrome_StepAreaPoint(em, 1);
                Em_ComputeGoal2(em, em->goal);
                em->timer = (s32) ((1.5f * Vec_DistanceXZ(&em->posX, em->goal)) / 50.0f);
                EmCephadrome_ChangeState(em, EM_GROUP_FLY, CEPH_SAND_ROUND_PREV_LEG, EM_SYNC_LOCAL);
            }
        }
        Em_TurnToGoal(em, 512, 0);
        Em_GetPitchToGoal(em, (s32 *) &work->moveAngBX, 0.0f, 0.0f);
        work->moveAngBY = em->rotY;
        work->moveAngBZ = 0;
        Em_MoveRotated(em, &work->moveAngBX);
        EmCephadrome_SpawnWakeEvery4(em, 8.0f);
        break;
    }
}

/*
 * CEPH_SAND_CIRCLE_RIGHT: swims a circle for 132 frames at speed 40
 * (CEPH_MOT_SAND_CIRCLE_RIGHT), the heading rising 0x1F0 a frame, pitched
 * toward the goal, with the sand wake; then EndSandState.
 */
void EmCephadrome_SandCircleRight(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_UNDER;
        Em_SetMotion(em, CEPH_MOT_SAND_CIRCLE_RIGHT, 0, 0);
        em->timer = 132;
        Player_ClearVelocity(em);
        em->velZ = 40.0f;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmCephadrome_EndSandState(em);
        }
        em->rotY += 0x1F0;
        em->rotY = (s32) (u16) em->rotY;
        Em_GetPitchToGoal(em, (s32 *) &work->moveAngBX, 0.0f, 0.0f);
        work->moveAngBY = em->rotY;
        work->moveAngBZ = 0;
        Em_MoveRotated(em, &work->moveAngBX);
        EmCephadrome_SpawnWakeEvery4(em, 8.0f);
        break;
    }
}

/*
 * CEPH_SAND_CIRCLE_LEFT: as SandCircleRight with CEPH_MOT_SAND_CIRCLE_LEFT,
 * the heading falling 0x1F0 a frame.
 */
void EmCephadrome_SandCircleLeft(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_UNDER;
        Em_SetMotion(em, CEPH_MOT_SAND_CIRCLE_LEFT, 0, 0);
        em->timer = 132;
        Player_ClearVelocity(em);
        em->velZ = 40.0f;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmCephadrome_EndSandState(em);
        }
        em->rotY -= 0x1F0;
        em->rotY = (s32) (u16) em->rotY;
        Em_GetPitchToGoal(em, (s32 *) &work->moveAngBX, 0.0f, 0.0f);
        work->moveAngBY = em->rotY;
        work->moveAngBZ = 0;
        Em_MoveRotated(em, &work->moveAngBX);
        EmCephadrome_SpawnWakeEvery4(em, 8.0f);
        break;
    }
}

/*
 * CEPH_SAND_LEAP_ASHORE: leaps up out of the sand (CEPH_MOT_LEAP, velY 132,
 * velZ 60); on breaking the sand top it turns airborne (stance
 * EM_STANCE_AIR, velY 50, gravity -5) with a splash, the nearby pad
 * vibration and a camera shake; it always comes down on floorY and goes to
 * CEPH_IDLE_LAND_FLOP_A (EmCephadrome_LandFlop), never
 * back into the sand.
 */
void EmCephadrome_LeapAshore(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_UNDER;
        Player_ClearVelocity(em);
        Em_SetMotion(em, CEPH_MOT_LEAP, 0, 0);
        em->velY = 132.0f;
        em->velZ = 60.0f;
        work->moveAngBX = 0;
        work->moveAngBZ = 0;
        break;
    case 1:
        work->moveAngBY = em->rotY;
        Em_MoveRotated(em, (s32 *) &work->moveAngBX);
        if (!(em->posY < (em->surfaceY - *(f32 *)&em->surfaceDepth))) {
            em->step++;
            em->stance = EM_STANCE_AIR;
            em->velY = 50.0f;
            em->accelY = -5.0f;
            EmCephadrome_SpawnSurfaceSplash(em, 8.0f);
            EmCephadrome_VibrateNearby(em);
            CamShake_ReqAIf(em, 3);
        }
        break;
    case 2:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, (s32 *) &work->moveAngBX);
        if ((em->velY < 0.0f) && (em->posY < em->floorY)) {
            em->step++;
            em->posY = em->floorY;
            EmCephadrome_ChangeState(em, EM_GROUP_IDLE, CEPH_IDLE_LAND_FLOP_A, EM_SYNC_OWNER_FLAGGED);
            EmCephadrome_SpawnSurfaceSplash(em, 8.0f);
        }
        break;
    case 3:
        if (em->remote == 0) {
            em->posY = em->floorY;
            EmCephadrome_ChangeState(em, EM_GROUP_IDLE, CEPH_IDLE_LAND_FLOP_A, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * CEPH_SAND_SURFACE_FLOP: held at the swim height under the sand top, plays
 * CEPH_MOT_FLOP_A and ends under the sand from motion frame 10.
 */
void EmCephadrome_SandSurfaceFlop(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_UNDER;
        Em_SetMotion(em, CEPH_MOT_FLOP_A, 0, 0);
        Player_ClearVelocity(em);
        em->posY = em->surfaceY - *(f32 *)&em->surfaceDepth;
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 10.0f, 0U) != 0) {
            em->step++;
            EmCephadrome_EndSandState(em);
        }
        break;
    }
}

/*
 * CEPH_SAND_LEAP_IN: on land crouches (CEPH_MOT_LEAP_CROUCH), leaps
 * (CEPH_MOT_LEAP, velY 50, velZ 80, gravity -4, splash); coming down over
 * sand (or out of the player's area) it enters at the swim height, plays
 * CEPH_MOT_SAND_SWIM_FAST and glides 1000 forward decelerating over 60
 * frames (distLeft), then ends under the sand; over hard ground it lands on
 * floorY and goes to CEPH_IDLE_LAND_FLOP_A.
 */
void EmCephadrome_LeapIntoSand(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, CEPH_MOT_LEAP_CROUCH, 0, 0);
        Player_ClearVelocity(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stance = EM_STANCE_AIR;
            Em_SetMotion(em, CEPH_MOT_LEAP, 0, 0);
            em->velY = 50.0f;
            em->velZ = 80.0f;
            em->accelY = -4.0f;
            EmCephadrome_SpawnSurfaceSplash(em, 8.0f);
        }
        break;
    case 2:
        work->moveAngBX = 0;
        work->moveAngBY = em->rotY;
        work->moveAngBZ = 0;
        Em_MoveRotatedAccel(em, (s32 *) &work->moveAngBX);
        if (em->posY <= (em->surfaceY - *(f32 *)&em->surfaceDepth)) {
            em->stance = EM_STANCE_UNDER;
        }
        if (em->velY < 0.0f) {
            if (gScene.area == em->area) {
                if (em->surfaceKind != 0) {
                    if (em->posY < (em->surfaceY - *(f32 *)&em->surfaceDepth)) {
                        em->step++;
                        em->stance = EM_STANCE_UNDER;
                        em->posY = em->surfaceY - *(f32 *)&em->surfaceDepth;
                        em->velY = 0.0f;
                        em->accelY = 0.0f;
                        Em_SetMotion(em, CEPH_MOT_SAND_SWIM_FAST, 10, 0);
                        ((EmCephadromeWork *)work)->distLeft = 1000.0f;
                        em->timer = 60;
                        em->accelZ = (((EmCephadromeWork *)work)->distLeft - em->timer * em->velZ) / ((em->timer * em->timer) / 2);
                        EmCephadrome_SpawnSurfaceSplash(em, 8.0f);
                    }
                } else if (em->posY < em->floorY) {
                    em->step = 99;
                    em->posY = em->floorY;
                    EmCephadrome_ChangeState(em, EM_GROUP_IDLE, CEPH_IDLE_LAND_FLOP_A, EM_SYNC_OWNER_FLAGGED);
                    EmCephadrome_SpawnSurfaceSplash(em, 8.0f);
                }
            } else {
                if (em->posY < (em->surfaceY - *(f32 *)&em->surfaceDepth)) {
                    em->step++;
                    em->stance = EM_STANCE_UNDER;
                    em->posY = em->surfaceY - *(f32 *)&em->surfaceDepth;
                    em->velY = 0.0f;
                    em->accelY = 0.0f;
                    Em_SetMotion(em, CEPH_MOT_SAND_SWIM_FAST, 10, 0);
                    ((EmCephadromeWork *)work)->distLeft = 1000.0f;
                    em->timer = 60;
                    em->accelZ = (((EmCephadromeWork *)work)->distLeft - em->timer * em->velZ) / ((em->timer * em->timer) / 2);
                    EmCephadrome_SpawnSurfaceSplash(em, 8.0f);
                }
            }
        }
        break;
    case 3:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, (s32 *) &work->moveAngBX);
        ((EmCephadromeWork *)work)->distLeft -= em->velZ;
        if (((EmCephadromeWork *)work)->distLeft <= 0.0f || em->velZ <= 0.0f) {
            em->step++;
            EmCephadrome_EndSandState(em);
        }
        break;
    case 99:
        if (em->remote == 0) {
            em->posY = em->floorY;
            EmCephadrome_ChangeState(em, EM_GROUP_IDLE, CEPH_IDLE_LAND_FLOP_A, EM_SYNC_OWNER_FLAGGED);
            EmCephadrome_SpawnSurfaceSplash(em, 8.0f);
        }
        break;
    }
}

/*
 * CEPH_SAND_LEAP: from under the sand leaps (CEPH_MOT_LEAP, velY 100, velZ
 * 60, gravity -8), airborne with splash, vibration and camera shake on
 * breaking the top; falling back over sand it re-enters and glides 1000
 * (CEPH_MOT_SAND_SWIM_FAST) as LeapIntoSand, over hard ground it lands and
 * goes to CEPH_IDLE_LAND_FLOP_A.
 */
void EmCephadrome_LeapFromSand(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_UNDER;
        Player_ClearVelocity(em);
        Em_SetMotion(em, CEPH_MOT_LEAP, 0, 0);
        em->velY = 100.0f;
        em->velZ = 60.0f;
        em->accelY = -8.0f;
        work->moveAngBX = 0;
        work->moveAngBZ = 0;
        break;
    case 1:
        work->moveAngBY = em->rotY;
        Em_MoveRotated(em, &work->moveAngBX);
        if (!(em->posY <= (em->surfaceY - *(f32 *)&em->surfaceDepth))) {
            em->stance = EM_STANCE_AIR;
            EmCephadrome_SpawnSurfaceSplash(em, 8.0f);
            em->step++;
            EmCephadrome_VibrateNearby(em);
            CamShake_ReqAIf(em, 3);
        }
        break;
    case 2:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, (s32 *) &work->moveAngBX);
        if (em->posY < (em->surfaceY - *(f32 *)&em->surfaceDepth)) {
            em->stance = EM_STANCE_UNDER;
        }
        if (em->velY < 0.0f) {
            if (gScene.area == em->area) {
                if (em->surfaceKind != 0) {
                    if (em->posY < (em->surfaceY - *(f32 *)&em->surfaceDepth)) {
                        em->step++;
                        em->stance = EM_STANCE_UNDER;
                        em->posY = em->surfaceY - *(f32 *)&em->surfaceDepth;
                        em->velY = 0.0f;
                        em->accelY = 0.0f;
                        Em_SetMotion(em, CEPH_MOT_SAND_SWIM_FAST, 10, 0);
                        ((EmCephadromeWork *)work)->distLeft = 1000.0f;
                        em->timer = 60;
                        em->accelZ = (((EmCephadromeWork *)work)->distLeft - em->timer * em->velZ) / ((em->timer * em->timer) / 2);
                        EmCephadrome_SpawnSurfaceSplash(em, 8.0f);
                    }
                } else if (em->posY < em->floorY) {
                    em->step = 99;
                    em->posY = em->floorY;
                    EmCephadrome_ChangeState(em, EM_GROUP_IDLE, CEPH_IDLE_LAND_FLOP_A, EM_SYNC_OWNER_FLAGGED);
                    EmCephadrome_SpawnSurfaceSplash(em, 8.0f);
                }
            } else {
                if (em->posY < (em->surfaceY - *(f32 *)&em->surfaceDepth)) {
                    em->step++;
                    em->posY = em->surfaceY - *(f32 *)&em->surfaceDepth;
                    em->stance = EM_STANCE_UNDER;
                    em->velY = 0.0f;
                    em->accelY = 0.0f;
                    Em_SetMotion(em, CEPH_MOT_SAND_SWIM_FAST, 10, 0);
                    ((EmCephadromeWork *)work)->distLeft = 1000.0f;
                    em->timer = 60;
                    em->accelZ = (((EmCephadromeWork *)work)->distLeft - em->timer * em->velZ) / ((em->timer * em->timer) / 2);
                    EmCephadrome_SpawnSurfaceSplash(em, 8.0f);
                }
            }
        }
        break;
    case 3:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, (s32 *) &work->moveAngBX);
        ((EmCephadromeWork *)work)->distLeft -= em->velZ;
        if (((EmCephadromeWork *)work)->distLeft <= 0.0f || em->velZ <= 0.0f) {
            em->step++;
            EmCephadrome_EndSandState(em);
        }
        break;
    case 99:
        if (em->remote == 0) {
            em->posY = em->floorY;
            EmCephadrome_ChangeState(em, EM_GROUP_IDLE, CEPH_IDLE_LAND_FLOP_A, EM_SYNC_OWNER_FLAGGED);
            EmCephadrome_SpawnSurfaceSplash(em, 8.0f);
        }
        break;
    }
}

/*
 * CEPH_SAND_LEAP_ON_LAND: on land crouches (CEPH_MOT_LEAP_CROUCH) and leaps
 * (CEPH_MOT_LEAP, velY 50, velZ 100, gravity -4, splash), always coming
 * down on floorY and going to CEPH_IDLE_LAND_FLOP_A.
 */
void EmCephadrome_LeapOnLand(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, CEPH_MOT_LEAP_CROUCH, 0, 0);
        Player_ClearVelocity(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stance = EM_STANCE_AIR;
            Em_SetMotion(em, CEPH_MOT_LEAP, 0, 0);
            em->velY = 50.0f;
            em->velZ = 100.0f;
            em->accelY = -4.0f;
            EmCephadrome_SpawnSurfaceSplash(em, 8.0f);
        }
        break;
    case 2:
        work->moveAngBX = 0;
        work->moveAngBY = em->rotY;
        work->moveAngBZ = 0;
        Em_MoveRotatedAccel(em, &work->moveAngBX);
        if ((em->velY < 0.0f) && (em->posY < em->floorY)) {
            em->step++;
            em->posY = em->floorY;
            EmCephadrome_ChangeState(em, EM_GROUP_IDLE, CEPH_IDLE_LAND_FLOP_A, EM_SYNC_OWNER_FLAGGED);
            EmCephadrome_SpawnSurfaceSplash(em, 8.0f);
        }
        break;
    case 3:
        if (em->remote == 0) {
            em->posY = em->floorY;
            EmCephadrome_ChangeState(em, EM_GROUP_IDLE, CEPH_IDLE_LAND_FLOP_A, EM_SYNC_OWNER_FLAGGED);
            EmCephadrome_SpawnSurfaceSplash(em, 8.0f);
        }
        break;
    }
}

/*
 * CEPH_SAND_STATUS4_RECOVER: SetSandState ends status 4 on entry; plays
 * CEPH_MOT_SAND_RECOVER under the sand, then EndSandState.
 */
void EmCephadrome_SandStatus4Recover(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_UNDER;
        Em_SetMotion(em, CEPH_MOT_SAND_RECOVER, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmCephadrome_EndSandState(em);
        }
        break;
    }
}

/*
 * CEPH_SAND_LEAP_TO_FIXED_POINT: airborne (CEPH_MOT_LEAP) jumps to the
 * fixed point (6000, -1000, 10000) in 150 frames: faces it, gravity -2,
 * velY and velZ computed to arrive on time; then puts posY at -1000 and
 * enters CEPH_SAND_IDLE (EM_SYNC_OWNER_FLAGGED).
 */
void EmCephadrome_LeapToFixedPoint(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Player_ClearVelocity(em);
        Em_SetMotion(em, CEPH_MOT_LEAP, 0, 0);
        em->timer = 150;
        em->goal[0] = 6000.0f;
        em->goal[1] = -1000.0f;
        em->goal[2] = 10000.0f;
        em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        em->accelY = -2.0f;
        em->velY = ((em->goal[1] - em->posY) / (f32) em->timer) - ((em->accelY * (f32) em->timer) / 2.0f);
        em->velZ = Vec_DistanceXZ(&em->posX, em->goal) / (f32) em->timer;
        work->moveAngBX = 0;
        work->moveAngBZ = 0;
        break;
    case 1:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, (s32 *) &work->moveAngBX);
        if (--em->timer <= 0) {
            em->step++;
            em->posY = -1000.0f;
            EmCephadrome_ChangeState(em, EM_GROUP_FLY, CEPH_SAND_IDLE, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            em->posY = -1000.0f;
            EmCephadrome_ChangeState(em, EM_GROUP_FLY, CEPH_SAND_IDLE, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * CEPH_SAND_STATUS2_END, CEPH_SAND_STATUS1_END: SetSandState ends status
 * 2 or 1 on entry; plays CEPH_MOT_SAND_RECOVER blind (Em.blind 2 every
 * frame, 0 at the end), then EndSandState.
 */
void EmCephadrome_SandStatusRecover(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_UNDER;
        em->blind = 2;
        Em_SetMotion(em, CEPH_MOT_SAND_RECOVER, 0, 0);
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            EmCephadrome_EndSandState(em);
        }
        break;
    }
}

/*
 * CEPH_ATK_LAND_A: on land (stance EM_STANCE_GROUND) plays
 * CEPH_MOT_LAND_ATTACK_A once (its motion effects make attack row 1 at
 * frame 50), then land idle (EndGroundState).
 */
void EmCephadrome_LandAttackA(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, CEPH_MOT_LAND_ATTACK_A, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmCephadrome_EndGroundState(em);
        }
        break;
    }
}

/*
 * CEPH_ATK_BREATH: on land plays CEPH_MOT_BREATH; at frame 78 an attack
 * burst at CEPH_BONE_BREATH and the ranged attack from there
 * (EmRangedAttack_CreateAtBone EMRANGED_CEPHADROME_SHOT, variant 0 for
 * Cephadrome, 1 for Cephalos, pitched 10 degrees), more bursts at 80 and 82 (attack row 25
 * at 66 from the motion effects); then land idle. SetAttackState sets
 * EmCephadromeWork.unk013 to 1 on entry.
 */
void EmCephadrome_Breath(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, CEPH_MOT_BREATH, 0, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 78.0f, 0U) != 0) {
            AttackBurst_CreateAtBoneTilted((Em *) em, CEPH_BONE_BREATH, 12, 1820);
            if (em->type == EM_KIND_CEPHADROME) {
                EmRangedAttack_CreateAtBone(em, CEPH_BONE_BREATH, EMRANGED_CEPHADROME_SHOT, 0, 1820, 0);
            } else {
                EmRangedAttack_CreateAtBone(em, CEPH_BONE_BREATH, EMRANGED_CEPHADROME_SHOT, 1, 1820, 0);
            }
        }
        if (EmMotion_PassedFrameScaled(em, 80.0f, 0U) != 0) {
            AttackBurst_CreateAtBoneTilted((Em *) em, CEPH_BONE_BREATH, 12, 1820);
        }
        if (EmMotion_PassedFrameScaled(em, 82.0f, 0U) != 0) {
            AttackBurst_CreateAtBoneTilted((Em *) em, CEPH_BONE_BREATH, 12, 1820);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmCephadrome_EndGroundState(em);
        }
        break;
    }
}

/*
 * CEPH_ATK_SPIN: on land plays CEPH_MOT_SPIN twice (waiting on layer 1)
 * while turning rotY by -512 a frame (attack row 16 at frame 30, dust
 * when enraged), then land idle.
 */
void EmCephadrome_SpinAttack(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, CEPH_MOT_SPIN, 0, 0);
        Motion_InitTime(em, 0);
        break;
    case 1:
        em->rotY -= 0x200;
        if (em->motion[1].playing == 0) {
            em->step++;
            Em_SetMotion(em, CEPH_MOT_SPIN, 0, 0);
        }
        break;
    case 2:
        em->rotY -= 0x200;
        if (em->motion[1].playing == 0) {
            em->step++;
            EmCephadrome_EndGroundState(em);
        }
        break;
    }
}

/*
 * CEPH_ATK_SURFACE_BREATH_A: under the sand plays
 * CEPH_MOT_SURFACE_BREATH_A: sand splashes at frames 38 and 230 and three
 * ranged attacks from CEPH_BONE_BREATH (EMRANGED_CEPHADROME_SHOT,
 * variant 0 for both kinds, pitched 25 degrees) at frames 122, 162 and
 * 192, yawed +40, 0 and -40 degrees, each with three attack bursts the
 * same way; then ends under the sand (EndSandState).
 */
void EmCephadrome_SurfaceBreathA(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, CEPH_MOT_SURFACE_BREATH_A, 0, 0);
        em->stance = EM_STANCE_UNDER;
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 122.0f, 0U) != 0) {
            AttackBurst_CreateAtBoneTilted((Em *) em, CEPH_BONE_BREATH, 12, 7282);
            EmRangedAttack_CreateAtBone(em, CEPH_BONE_BREATH, EMRANGED_CEPHADROME_SHOT, 0, 4551, 7282);
        }
        if (EmMotion_PassedFrameScaled(em, 162.0f, 0U) != 0) {
            AttackBurst_CreateAtBoneTilted((Em *) em, CEPH_BONE_BREATH, 12, 0);
            EmRangedAttack_CreateAtBone(em, CEPH_BONE_BREATH, EMRANGED_CEPHADROME_SHOT, 0, 4551, 0);
        }
        if (EmMotion_PassedFrameScaled(em, 192.0f, 0U) != 0) {
            AttackBurst_CreateAtBoneTilted((Em *) em, CEPH_BONE_BREATH, 12, 58255);
            EmRangedAttack_CreateAtBone(em, CEPH_BONE_BREATH, EMRANGED_CEPHADROME_SHOT, 0, 4551, 58255);
        }
        if (EmMotion_PassedFrameScaled(em, 38.0f, 0U) != 0) {
            EmCephadrome_SpawnSurfaceSplash(em, 8.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 230.0f, 0U) != 0) {
            EmCephadrome_SpawnSurfaceSplash(em, 8.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 126.0f, 0U) != 0) {
            AttackBurst_CreateAtBoneTilted((Em *) em, CEPH_BONE_BREATH, 12, 7282);
        }
        if (EmMotion_PassedFrameScaled(em, 166.0f, 0U) != 0) {
            AttackBurst_CreateAtBoneTilted((Em *) em, CEPH_BONE_BREATH, 12, 0);
        }
        if (EmMotion_PassedFrameScaled(em, 196.0f, 0U) != 0) {
            AttackBurst_CreateAtBoneTilted((Em *) em, CEPH_BONE_BREATH, 12, 58255);
        }
        if (EmMotion_PassedFrameScaled(em, 130.0f, 0U) != 0) {
            AttackBurst_CreateAtBoneTilted((Em *) em, CEPH_BONE_BREATH, 12, 7282);
        }
        if (EmMotion_PassedFrameScaled(em, 170.0f, 0U) != 0) {
            AttackBurst_CreateAtBoneTilted((Em *) em, CEPH_BONE_BREATH, 12, 0);
        }
        if (EmMotion_PassedFrameScaled(em, 200.0f, 0U) != 0) {
            AttackBurst_CreateAtBoneTilted((Em *) em, CEPH_BONE_BREATH, 12, 58255);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmCephadrome_EndSandState(em);
        }
        break;
    }
}

/*
 * CEPH_ATK_SURFACE_BREATH_B: as SurfaceBreathA with
 * CEPH_MOT_SURFACE_BREATH_B and the sweep reversed (ranged attacks yawed
 * -40, 0, +40 degrees); then ends under the sand.
 */
void EmCephadrome_SurfaceBreathB(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, CEPH_MOT_SURFACE_BREATH_B, 0, 0);
        em->stance = EM_STANCE_UNDER;
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 122.0f, 0U) != 0) {
            AttackBurst_CreateAtBoneTilted((Em *) em, CEPH_BONE_BREATH, 12, 7282);
            EmRangedAttack_CreateAtBone(em, CEPH_BONE_BREATH, EMRANGED_CEPHADROME_SHOT, 0, 4551, 58255);
        }
        if (EmMotion_PassedFrameScaled(em, 162.0f, 0U) != 0) {
            AttackBurst_CreateAtBoneTilted((Em *) em, CEPH_BONE_BREATH, 12, 0);
            EmRangedAttack_CreateAtBone(em, CEPH_BONE_BREATH, EMRANGED_CEPHADROME_SHOT, 0, 4551, 0);
        }
        if (EmMotion_PassedFrameScaled(em, 192.0f, 0U) != 0) {
            AttackBurst_CreateAtBoneTilted((Em *) em, CEPH_BONE_BREATH, 12, 58255);
            EmRangedAttack_CreateAtBone(em, CEPH_BONE_BREATH, EMRANGED_CEPHADROME_SHOT, 0, 4551, 7282);
        }
        if (EmMotion_PassedFrameScaled(em, 38.0f, 0U) != 0) {
            EmCephadrome_SpawnSurfaceSplash(em, 8.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 230.0f, 0U) != 0) {
            EmCephadrome_SpawnSurfaceSplash(em, 8.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 126.0f, 0U) != 0) {
            AttackBurst_CreateAtBoneTilted((Em *) em, CEPH_BONE_BREATH, 12, 58255);
        }
        if (EmMotion_PassedFrameScaled(em, 166.0f, 0U) != 0) {
            AttackBurst_CreateAtBoneTilted((Em *) em, CEPH_BONE_BREATH, 12, 0);
        }
        if (EmMotion_PassedFrameScaled(em, 196.0f, 0U) != 0) {
            AttackBurst_CreateAtBoneTilted((Em *) em, CEPH_BONE_BREATH, 12, 7282);
        }
        if (EmMotion_PassedFrameScaled(em, 130.0f, 0U) != 0) {
            AttackBurst_CreateAtBoneTilted((Em *) em, CEPH_BONE_BREATH, 12, 58255);
        }
        if (EmMotion_PassedFrameScaled(em, 170.0f, 0U) != 0) {
            AttackBurst_CreateAtBoneTilted((Em *) em, CEPH_BONE_BREATH, 12, 0);
        }
        if (EmMotion_PassedFrameScaled(em, 200.0f, 0U) != 0) {
            AttackBurst_CreateAtBoneTilted((Em *) em, CEPH_BONE_BREATH, 12, 7282);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmCephadrome_EndSandState(em);
        }
        break;
    }
}

/*
 * CEPH_ATK_SURFACE_BREATH_C: under the sand plays
 * CEPH_MOT_SURFACE_BREATH_C: splashes at frames 26 and 98, one ranged
 * attack from CEPH_BONE_BREATH at frame 58 (variant by kind, pitched 20 degrees)
 * with bursts at 58, 60 and 62 (the motion effects add attack row 29 and
 * the pad vibration at 26); then ends under the sand.
 */
void EmCephadrome_SurfaceBreathC(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, CEPH_MOT_SURFACE_BREATH_C, 0, 0);
        em->stance = EM_STANCE_UNDER;
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 58.0f, 0U) != 0) {
            AttackBurst_CreateAtBoneTilted((Em *) em, CEPH_BONE_BREATH, 12, 0);
            if (em->type == EM_KIND_CEPHADROME) {
                EmRangedAttack_CreateAtBone(em, CEPH_BONE_BREATH, EMRANGED_CEPHADROME_SHOT, 0, 3641, 0);
            } else {
                EmRangedAttack_CreateAtBone(em, CEPH_BONE_BREATH, EMRANGED_CEPHADROME_SHOT, 1, 3641, 0);
            }
        }
        if (EmMotion_PassedFrameScaled(em, 26.0f, 0U) != 0) {
            EmCephadrome_SpawnSurfaceSplash(em, 8.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 98.0f, 0U) != 0) {
            EmCephadrome_SpawnSurfaceSplash(em, 8.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 60.0f, 0U) != 0) {
            AttackBurst_CreateAtBoneTilted((Em *) em, CEPH_BONE_BREATH, 12, 0);
        }
        if (EmMotion_PassedFrameScaled(em, 62.0f, 0U) != 0) {
            AttackBurst_CreateAtBoneTilted((Em *) em, CEPH_BONE_BREATH, 12, 0);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmCephadrome_EndSandState(em);
        }
        break;
    }
}

/*
 * CEPH_ATK_LEAP: from under the sand leaps forward and up
 * (CEPH_MOT_LEAP_ATTACK, velZ 100, velY 80, attack row 14 at frame 10);
 * breaking the top it turns airborne with splash, vibration and camera
 * shake, stops rising at the sand top and hangs 30 frames, then falls
 * (gravity -4.5); over sand (or out of the player's area) it re-enters and
 * glides 1000 with CEPH_MOT_SAND_SWIM_FAST and ends under the sand, over
 * hard ground it lands and goes to CEPH_IDLE_LAND_FLOP_A.
 */
void EmCephadrome_LeapAttack(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, CEPH_MOT_LEAP_ATTACK, 0, 0);
        em->stance = EM_STANCE_UNDER;
        Player_ClearVelocity(em);
        em->velZ = 100.0f;
        em->velY = 80.0f;
        em->accelY = -4.5f;
        work->moveAngBX = 0;
        work->moveAngBZ = 0;
        break;
    case 1:
        work->moveAngBY = em->rotY;
        Em_MoveRotated(em, &work->moveAngBX);
        if (!(em->posY <= (em->surfaceY - *(f32 *)&em->surfaceDepth))) {
            em->step++;
            em->stance = EM_STANCE_AIR;
            EmCephadrome_SpawnSurfaceSplash(em, 8.0f);
            EmCephadrome_VibrateNearby(em);
            CamShake_ReqAIf(em, 3);
        }
        break;
    case 2:
        work->moveAngBY = em->rotY;
        Em_MoveRotated(em, &work->moveAngBX);
        if (!(em->posY <= em->surfaceY)) {
            em->step++;
            em->velY = 0.0f;
            em->timer = 30;
        }
        break;
    case 3:
        work->moveAngBY = em->rotY;
        Em_MoveRotated(em, &work->moveAngBX);
        if (--em->timer <= 0) {
            em->step++;
        }
        break;
    case 4:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, (s32 *) &work->moveAngBX);
        if ((gScene.area != em->area) || (em->surfaceKind != 0)) {
            if ((em->posY <= (em->surfaceY - *(f32 *)&em->surfaceDepth)) && (em->velY < 0.0f)) {
                em->step++;
                em->stance = EM_STANCE_UNDER;
                em->velY = 0.0f;
                em->accelY = 0.0f;
                Em_SetMotion(em, CEPH_MOT_SAND_SWIM_FAST, 10, 0);
                ((EmCephadromeWork *)work)->distLeft = 1000.0f;
                em->timer = 60;
                em->accelZ = (((EmCephadromeWork *)work)->distLeft - em->timer * em->velZ) / ((em->timer * em->timer) / 2);
                EmCephadrome_SpawnSurfaceSplash(em, 8.0f);
                break;
            }
        } else if (em->posY < em->floorY) {
            em->step = 99;
            em->posY = em->floorY;
            EmCephadrome_ChangeState(em, EM_GROUP_IDLE, CEPH_IDLE_LAND_FLOP_A, EM_SYNC_OWNER_FLAGGED);
            EmCephadrome_SpawnSurfaceSplash(em, 8.0f);
        }
        break;
    case 5:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, (s32 *) &work->moveAngBX);
        ((EmCephadromeWork *)work)->distLeft -= em->velZ;
        if (((EmCephadromeWork *)work)->distLeft <= 0.0f || em->velZ <= 0.0f) {
            em->step++;
            EmCephadrome_EndSandState(em);
        }
        break;
    case 99:
        if (em->remote == 0) {
            em->posY = em->floorY;
            EmCephadrome_ChangeState(em, EM_GROUP_IDLE, CEPH_IDLE_LAND_FLOP_A, EM_SYNC_OWNER_FLAGGED);
            EmCephadrome_SpawnSurfaceSplash(em, 8.0f);
        }
        break;
    }
}

/*
 * CEPH_ATK_LAND_B: on land plays CEPH_MOT_LAND_ATTACK_B once (attack row
 * 22 at frame 60), then land idle.
 */
void EmCephadrome_LandAttackB(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, CEPH_MOT_LAND_ATTACK_B, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmCephadrome_EndGroundState(em);
        }
        break;
    }
}

/*
 * CEPH_ATK_SURFACE_LUNGE: from under the sand lunges forward along the top
 * (CEPH_MOT_SURFACE_LUNGE, stance EM_STANCE_AIR, velZ 100, velY 16, gravity
 * -0.8, attack row 27 at frame 28) with vibration and camera shake on
 * entry; from frame 54 brakes to a stop in 18 frames, from frame 84 stops
 * sinking at the swim height; at the motion's end goes back under the sand
 * (EndSandState).
 */
void EmCephadrome_SurfaceLunge(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, CEPH_MOT_SURFACE_LUNGE, 0, 0);
        Player_ClearVelocity(em);
        em->velY = 16.0f;
        em->velZ = 100.0f;
        em->accelY = -0.8f;
        work->moveAngBX = 0;
        work->moveAngBZ = 0;
        EmCephadrome_VibrateNearby(em);
        CamShake_ReqAIf(em, 3);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 54.0f, 0U) != 0) {
            em->timer = 18;
            em->accelZ = (-em->velZ / (f32) em->timer);
        }
        if ((EmMotion_ReachedFrame(em, 0, 84.0f) != 0) && (em->posY <= (em->surfaceY - *(f32 *)&em->surfaceDepth))) {
            em->velY = 0.0f;
            em->accelY = 0.0f;
        }
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, (s32 *) &work->moveAngBX);
        if (em->velZ <= 0.0f) {
            em->velZ = 0.0f;
            em->accelZ = 0.0f;
        }
        if (em->motion[0].playing == 0) {
            em->stance = EM_STANCE_UNDER;
            em->step++;
            EmCephadrome_EndSandState(em);
        }
        break;
    }
}

/*
 * CEPH_REACT_FLINCH: a flinch of parts 0 and 7 on land (EM_DMG_PART_FLINCH,
 * EM_DMG_ATTACK5_FLINCH), or EM_DMG_HIT_MODE3 when not airborne: plays
 * CEPH_MOT_FLINCH (restarting CEPH_MOT_IDLE when layers 1 and 2 stop) and
 * returns to land idle.
 */
void EmCephadrome_Flinch(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, CEPH_MOT_FLINCH, 0, 0);
        break;
    case 1:
        if (em->motion[em->stateVar1].playing == 0) {
            em->step++;
            EmCephadrome_EndGroundState(em);
            break;
        }
        if (em->motion[1].playing == 0) {
            Em_SetMotion(em, CEPH_MOT_IDLE, 0, 0);
        }
        if (em->motion[2].playing == 0) {
            Em_SetMotion(em, CEPH_MOT_IDLE, 0, 0);
        }
        break;
    }
}

/*
 * CEPH_REACT_FLINCH_B: a flinch of an unbroken part 3 or 4 (or a higher
 * part) on land: CEPH_MOT_FLINCH_B, then land idle.
 */
void EmCephadrome_FlinchB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, CEPH_MOT_FLINCH_B, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmCephadrome_EndGroundState(em);
        }
        break;
    }
}

/*
 * CEPH_REACT_FLINCH_C: a flinch of parts 5 and 6 on land:
 * CEPH_MOT_FLINCH_C, then land idle.
 */
void EmCephadrome_FlinchC(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, CEPH_MOT_FLINCH_C, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmCephadrome_EndGroundState(em);
        }
        break;
    }
}

/*
 * CEPH_REACT_FLINCH_D: a flinch of parts 1 and 2 on land:
 * CEPH_MOT_FLINCH_D, then land idle.
 */
void EmCephadrome_FlinchD(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, CEPH_MOT_FLINCH_D, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmCephadrome_EndGroundState(em);
        }
        break;
    }
}

/*
 * CEPH_REACT_FLINCH_LYING: a part flinch while beached on land (stance
 * EM_STANCE_FAMILY); sets stance EM_STANCE_GROUND and plays
 * CEPH_MOT_LIE_DOWN, CEPH_MOT_LIE_FLAP, CEPH_MOT_GET_UP, then land idle.
 */
void EmCephadrome_FlinchLying(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        Em_RestartScript(em);
        Em_SetMotion(em, CEPH_MOT_LIE_DOWN, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, CEPH_MOT_LIE_FLAP, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, CEPH_MOT_GET_UP, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmCephadrome_EndGroundState(em);
        }
        break;
    }
}

/*
 * CEPH_REACT_KNOCK_DOWN (variant 1), CEPH_REACT_KNOCK_DOWN_PART3 (0),
 * CEPH_REACT_STUN_A (3), CEPH_REACT_STUN_B (2): falls over, lies and gets
 * up; odd variants use CEPH_MOT_FALL_A, LIE_A, LIE_FLAP, GET_UP_A, even
 * ones FALL_B, LIE_DOWN, LIE_FLAP_B, GET_UP. The stun variants show stun
 * stars at CEPH_BONE_BREATH, lie until em->timer runs out and end in
 * CEPH_IDLE_STUN_SHAKE_OFF (which ends the stun); the others end in land idle. Records
 * the state in prevStateGroup/prevState before leaving.
 */
void EmCephadrome_KnockDown(Em *em, EmWork *work, s16 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, (s16) ((variant & 1) == 0 ? 74 : 69), 0, 0);
        switch (variant) {
        case 2:
        case 3:
            StunStars_CreateEm(em, 1, CEPH_BONE_BREATH, em->timer + em->motion[0].frameCount / 2.0f, 150.0f, 3.0f);
            break;
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, (s16) ((variant & 1) == 0 ? 75 : 70), 0, 0);
            em->stateVar1 = 0;
        }
        break;
    case 2:
        switch (variant) {
        case 2:
        case 3:
            if (--em->timer <= 0) {
                em->stateVar1 = 1;
            }
            break;
        default:
            if (em->motion[0].playing == 0) {
                em->stateVar1 = 1;
            }
            break;
        }
        if (em->stateVar1) {
            em->step++;
            Em_SetMotion(em, (s16) ((variant & 1) == 0 ? 129 : 128), 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 0;
            Em_SetMotion(em, (s16) ((variant & 1) == 0 ? 124 : 71), 0, 0);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
            switch (variant) {
            case 2:
            case 3:
                em->step++;
                EmCephadrome_ChangeState(em, EM_GROUP_IDLE, CEPH_IDLE_STUN_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
                break;
            default:
                EmCephadrome_EndGroundState(em);
                break;
            }
        }
        break;
    case 5:
        if (em->remote == 0) {
            EmCephadrome_ChangeState(em, EM_GROUP_IDLE, CEPH_IDLE_STUN_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * CEPH_REACT_FALL: EM_DMG_STATUS_IN_AIR, or a part flinch while airborne
 * (stance EM_STANCE_AIR): falls under gravity -3 with CEPH_MOT_TUMBLE
 * (stance air above the sand top, under below it); on reaching floorY goes
 * to CEPH_REACT_LAND, never back into the sand.
 */
void EmCephadrome_FallFromAir(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, CEPH_MOT_TUMBLE, 0, 0);
        em->stance = EM_STANCE_AIR;
        em->accelY = -3.0f;
        em->accelX = 0.0f;
        em->accelZ = 0.0f;
        work->moveAngBX = 0;
        work->moveAngBZ = 0;
        break;
    case 1:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, &work->moveAngBX);
        if (!(em->posY < em->surfaceY)) {
            em->stance = EM_STANCE_AIR;
        } else {
            em->stance = EM_STANCE_UNDER;
        }
        if (em->posY < em->floorY) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            EmCephadrome_ChangeState(em, EM_GROUP_REACT, CEPH_REACT_LAND, EM_SYNC_OWNER);
        }
        break;
    }
}

/*
 * CEPH_REACT_STATUS4_LYING: status 4 (EM_DMG_STATUS4) while beached (stance
 * EM_STANCE_FAMILY); ChangeState begins status 4 on entry; hit reaction
 * kept (EM_HIT_REACT_STATUS): CEPH_MOT_LIE_DOWN, CEPH_MOT_LIE_FLAP, then
 * CEPH_MOT_STATUS4_LYING with the status 4 effect until em->timer runs out,
 * then CEPH_REACT_STATUS4_LYING_END.
 */
void EmCephadrome_Status4Lying(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_STATUS);
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, CEPH_MOT_LIE_DOWN, 0, 0);
        em->stance = EM_STANCE_GROUND;
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, CEPH_MOT_LIE_FLAP, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, CEPH_MOT_STATUS4_LYING, 0, 0);
        }
        break;
    case 3:
        if (--em->timer <= 0) {
            em->step++;
            EmCephadrome_ChangeState(em, EM_GROUP_REACT, CEPH_REACT_STATUS4_LYING_END, EM_SYNC_OWNER);
        }
        Em_SpawnStatus4Effect(em, 2);
        break;
    case 4:
        if (em->remote == 0) {
            EmCephadrome_ChangeState(em, EM_GROUP_REACT, CEPH_REACT_STATUS4_LYING_END, EM_SYNC_OWNER);
        }
        Em_SpawnStatus4Effect(em, 2);
        break;
    }
}

/*
 * CEPH_REACT_FALL_BLIND: as FallFromAir but blinded (Em.blind 2 every
 * frame); no C code enters it.
 */
void EmCephadrome_FallBlinded(Em *em, EmWork *work)
{
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, CEPH_MOT_TUMBLE, 0, 0);
        em->stance = EM_STANCE_AIR;
        em->accelY = -3.0f;
        em->accelX = 0.0f;
        em->accelZ = 0.0f;
        work->moveAngBX = 0;
        work->moveAngBZ = 0;
        break;
    case 1:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, &work->moveAngBX);
        if (!(em->posY < em->surfaceY)) {
            em->stance = EM_STANCE_AIR;
        } else {
            em->stance = EM_STANCE_UNDER;
        }
        if (em->posY < em->floorY) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            EmCephadrome_ChangeState(em, EM_GROUP_REACT, CEPH_REACT_LAND, EM_SYNC_OWNER);
        }
        break;
    }
}

/*
 * CEPH_REACT_FLINCH_BLIND: blinded (Em.blind 2) on land, plays
 * CEPH_MOT_FLINCH_BLIND once, then land idle; no C code enters it.
 */
void EmCephadrome_FlinchBlinded(Em *em)
{
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, CEPH_MOT_FLINCH_BLIND, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmCephadrome_EndGroundState(em);
        }
        break;
    }
}

/*
 * CEPH_REACT_STATUS4: status 4 on land (stance EM_STANCE_GROUND);
 * ChangeState begins status 4 on entry; hit reaction kept:
 * CEPH_MOT_STAND_STILL with the status 4 effect until em->timer runs out,
 * then CEPH_IDLE_STATUS4_SHAKE_OFF (which ends status 4).
 */
void EmCephadrome_Status4(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_STATUS);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, CEPH_MOT_STAND_STILL, 0, 0);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmCephadrome_ChangeState(em, EM_GROUP_IDLE, CEPH_IDLE_STATUS4_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmCephadrome_ChangeState(em, EM_GROUP_IDLE, CEPH_IDLE_STATUS4_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
    Em_SpawnStatus4Effect(em, 2);
}

/*
 * CEPH_REACT_LAND: came down on land from a fall: CEPH_MOT_FLOP_A (blended
 * in), CEPH_MOT_GET_UP, then land idle, clearing the velocity.
 */
void EmCephadrome_LandAfterFall(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, CEPH_MOT_FLOP_A, 12, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, CEPH_MOT_GET_UP, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmCephadrome_EndGroundState(em);
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
            Player_ClearVelocity(em);
        }
        break;
    }
}

/*
 * CEPH_REACT_THROWN_OUT (variant 0), CEPH_REACT_THROWN_OUT_ATTACK5 (variant
 * 1): under the sand (stance EM_STANCE_UNDER) a part flinch, EM_DMG_STATUS4
 * or EM_DMG_STATUS2 (variant 0) or EM_DMG_ATTACK5 (variant 1) throws it up
 * out of the sand (velY 100, CEPH_MOT_TUMBLE, splash on leaving; a
 * Cephadrome hit by variant 1 leaves a timed mine point at the sand top,
 * EmMine_CreateTimed0); it always lands on floorY: CEPH_MOT_FLOP_A,
 * CEPH_MOT_LIE_DOWN, CEPH_MOT_GET_UP_A, then land idle.
 */
void EmCephadrome_ThrownOutOfSand(Em *em, EmWork *work, s32 variant)
{
    f32 pos[3];

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, CEPH_MOT_TUMBLE, 0, 0);
        Player_ClearVelocity(em);
        em->velY = 100.0f;
        em->accelY = -2.75f;
        work->moveAngBX = 0;
        work->moveAngBZ = 0;
        if ((em->type == EM_KIND_CEPHADROME) && ((variant & 0xFF) == 1)) {
            pos[0] = em->posX;
            pos[1] = em->surfaceY;
            pos[2] = em->posZ;
            EmMine_CreateTimed0(em, pos);
        }
        break;
    case 1:
        work->moveAngBY = em->rotY;
        if (!(em->posY < (em->surfaceY - *(f32 *)&em->surfaceDepth))) {
            em->stance = EM_STANCE_AIR;
            em->velY = 80.0f;
            Em_MoveRotatedAccel(em, (s32 *) &work->moveAngBX);
            em->step++;
            EmCephadrome_SpawnSurfaceSplash(em, 8.0f);
            break;
        }
        em->stance = EM_STANCE_UNDER;
        Em_MoveRotated(em, &work->moveAngBX);
        break;
    case 2:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, (s32 *) &work->moveAngBX);
        if ((em->posY < em->surfaceY) && (em->velY < 0.0f)) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            Em_SetMotion(em, CEPH_MOT_FLOP_A, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, CEPH_MOT_LIE_DOWN, 0, 0);
        }
        break;
    case 4:
    case 5:
        if (em->motion[0].playing == 0) {
            em->step++;
        }
        break;
    case 6:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, CEPH_MOT_GET_UP_A, 0, 0);
        }
        break;
    case 7:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmCephadrome_EndGroundState(em);
        }
        break;
    }
}

/*
 * CEPH_REACT_STATUS4_LYING_END: ChangeState ends status 4 on entry; still
 * beached (stance EM_STANCE_FAMILY) gets up (CEPH_MOT_GET_UP), then land
 * idle.
 */
void EmCephadrome_Status4LyingEnd(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, CEPH_MOT_GET_UP, 0, 0);
        em->stance = EM_STANCE_FAMILY;
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmCephadrome_EndGroundState(em);
        }
        break;
    }
}

/*
 * CEPH_REACT_HOLD2: EM_DMG_ATTACK13 (Update resets the kind timer);
 * ChangeState ends all statuses on entry; the second hold every frame
 * (Em_SetHold2): CEPH_MOT_FLINCH without root motion up to frame 52, then
 * CEPH_MOT_STAND_STILL until em->timer runs out, then
 * CEPH_IDLE_STATUS4_SHAKE_OFF.
 */
void EmCephadrome_Hold2(Em *em)
{
    Em_SetHold2(em);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, CEPH_MOT_FLINCH, 0, 0);
        Player_SetFlag((Player *) em, EM_FLAG_NO_ROOT_MOTION);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 52.0f) != 0) {
            em->step++;
            Em_SetMotion(em, CEPH_MOT_STAND_STILL, 12, 0);
            Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            EmCephadrome_ChangeState(em, EM_GROUP_IDLE, CEPH_IDLE_STATUS4_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmCephadrome_ChangeState(em, EM_GROUP_IDLE, CEPH_IDLE_STATUS4_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * CEPH_EVENT_ENTRANCE: a scripted entrance at fixed positions. It swims
 * under the sand (CEPH_MOT_SWIM) along a fixed route of goals, turning in
 * place toward work->turnAngle between legs (CEPH_MOT_SAND_TURN_RIGHT, then
 * _LEFT) with sand wakes, then leaps out (CEPH_MOT_LEAP) with surface
 * splashes, lands (FLOP_A, BEACHED, GET_UP), does the idle action and
 * returns to land idle (EmCephadrome_EndGroundState). Keeps senseDelay and
 * noHitTimer at 5; no C code enters it.
 */
void EmCephadrome_EntranceScene(Em *em, EmWork *work)
{
    EmCephadromeWork *cephWork = (EmCephadromeWork *)work;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step = 3;
        em->stance = EM_STANCE_UNDER;
        em->posX = 11900.0f;
        em->posY = -*(f32 *)&em->surfaceDepth;
        em->posZ = 5220.0f;
        em->goal[0] = 16700.0f;
        em->goal[1] = -*(f32 *)&em->surfaceDepth;
        em->goal[2] = 8450.0f;
        em->rotX = 0;
        em->rotY = Math_AngleXZ(&em->posX, em->goal);
        em->rotZ = 0;
        cephWork->distLeft = Vec_DistanceXZ(&em->posX, em->goal);
        Player_ClearVelocity(em);
        em->velZ = 50.0f;
        Em_SetMotion(em, CEPH_MOT_SAND_SWIM, 0, 0);
        work->moveAngBX = 0;
        work->moveAngBZ = 0;
        break;
    case 1:
        Em_TurnToGoal(em, 64, 0);
        work->moveAngBY = em->rotY;
        Em_MoveRotated(em, &work->moveAngBX);
        cephWork->distLeft -= em->velZ;
        if (cephWork->distLeft <= 0.0f) {
            em->step++;
            em->goal[0] = 16700.0f;
            em->goal[1] = -*(f32 *)&em->surfaceDepth;
            em->goal[2] = 8450.0f;
            cephWork->turnYaw = Math_AngleXZ(&em->posX, em->goal);
            cephWork->distLeft = Vec_DistanceXZ(&em->posX, em->goal);
            Em_SetMotion(em, CEPH_MOT_SAND_TURN_RIGHT, 0, 0);
        }
        break;
    case 2:
        {
        u32 turnStep;
        u16 targetYaw;
        u32 yawDiff;
        s32 yaw;
        turnStep = (32768.0f / (em->motion[0].frameCount / 2.0f)) * em->motionSpeed;
        yaw = em->rotY;
        targetYaw = cephWork->turnYaw;
        yawDiff = (targetYaw - (yaw & 0xFFFF)) & 0xFFFF;
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, CEPH_MOT_SAND_SWIM, 0, 0);
        } else if (((yawDiff + turnStep) & 0xFFFF) < turnStep * 2) {
            em->rotY = targetYaw;
        } else if (yawDiff < 0x8000U) {
            em->rotY = (yaw + turnStep) & 0xFFFF;
        } else {
            em->rotY = (yaw - turnStep) & 0xFFFF;
        }
        }
        break;
    case 3:
        Em_TurnToGoal(em, 64, 0);
        work->moveAngBY = em->rotY;
        Em_MoveRotated(em, &work->moveAngBX);
        cephWork->distLeft -= em->velZ;
        if (cephWork->distLeft <= 0.0f) {
            em->step++;
            em->goal[0] = 12728.0f;
            em->goal[1] = -*(f32 *)&em->surfaceDepth;
            em->goal[2] = 13160.0f;
            cephWork->turnYaw = Math_AngleXZ(&em->posX, em->goal);
            cephWork->distLeft = Vec_DistanceXZ(&em->posX, em->goal);
            Em_SetMotion(em, CEPH_MOT_SAND_TURN_LEFT, 0, 0);
        }
        EmCephadrome_SpawnWakeEvery4(em, 8.0f);
        break;
    case 4:
        {
        s32 yaw;
        u32 turnStep;
        u16 targetYaw;
        u32 yawDiff;
        turnStep = (32768.0f / (em->motion[0].frameCount / 2.0f)) * em->motionSpeed;
        yaw = em->rotY;
        targetYaw = cephWork->turnYaw;
        yawDiff = (targetYaw - (yaw & 0xFFFF)) & 0xFFFF;
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, CEPH_MOT_SAND_SWIM, 0, 0);
        } else if (((yawDiff + turnStep) & 0xFFFF) < turnStep * 2) {
            em->rotY = targetYaw;
        } else if (yawDiff < 0x8000U) {
            em->rotY = (yaw + turnStep) & 0xFFFF;
        } else {
            em->rotY = (yaw - turnStep) & 0xFFFF;
        }
        }
        break;
    case 5:
        Em_TurnToGoal(em, 64, 0);
        work->moveAngBY = em->rotY;
        Em_MoveRotated(em, &work->moveAngBX);
        cephWork->distLeft -= em->velZ;
        if (cephWork->distLeft <= 0.0f) {
            em->step++;
            em->goal[0] = 14380.0f;
            em->goal[1] = 0.0f;
            em->goal[2] = 14930.0f;
            em->rotY = Math_AngleXZ(&em->posX, em->goal);
            Em_SetMotion(em, CEPH_MOT_LEAP, 0, 0);
            em->velY = 132.0f;
            em->velZ = 60.0f;
        }
        EmCephadrome_SpawnWakeEvery4(em, 8.0f);
        break;
    case 6:
        work->moveAngBY = em->rotY;
        Em_MoveRotated(em, &work->moveAngBX);
        if (!(em->posY < (em->surfaceY - *(f32 *)&em->surfaceDepth))) {
            em->step++;
            em->stance = EM_STANCE_AIR;
            em->velY = 50.0f;
            em->accelY = -5.0f;
            EmCephadrome_SpawnSurfaceSplash(em, 8.0f);
        }
        break;
    case 7:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, (s32 *) &work->moveAngBX);
        if (em->velY < 0.0f && em->posY < em->floorY) {
            em->step++;
            em->posY = em->floorY;
            EmCephadrome_SpawnSurfaceSplash(em, 8.0f);
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, CEPH_MOT_FLOP_A, 0, 0);
        }
        break;
    case 8:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stance = EM_STANCE_FAMILY;
            Em_SetMotion(em, CEPH_MOT_BEACHED, 0, 0);
        }
        break;
    case 9:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, CEPH_MOT_GET_UP, 0, 0);
        }
        break;
    case 10:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, CEPH_MOT_IDLE_ACTION, 0, 0);
        }
        break;
    case 11:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, CEPH_MOT_IDLE, 0, 0);
        }
        break;
    case 12:
        em->step++;
        EmCephadrome_EndGroundState(em);
        break;
    }
}

/*
 * CEPH_EVENT_TO_IDLE: hidden for one frame (visible 0, stance ground,
 * CEPH_MOT_IDLE, scriptPending 0), then shown again and back to land idle
 * (EmCephadrome_EndGroundState). Keeps senseDelay and noHitTimer at 5; no C code
 * enters it.
 */
void EmCephadrome_HiddenToIdle(Em *em)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, CEPH_MOT_IDLE, 0, 0);
        em->visible = 0;
        em->scriptPending = 0;
        break;
    case 1:
        em->step++;
        em->visible = 1;
        EmCephadrome_EndGroundState(em);
        break;
    }
}

/*
 * CEPH_EVENT_CAPTURED: captured (EM_DMG_CAPTURED): Em_SetCaptured every
 * frame, reports the capture (QuestEm_Captured), begins status 1, clears
 * poison, loads reaction set 1, collapses and lies
 * (CEPH_MOT_STATUS_COLLAPSE, CEPH_MOT_STATUS_LIE), waits 1800 frames
 * counted only while QuestEm_CanAppear, fades out over 150 frames, then the
 * owner moves to CEPH_DEFEAT_REAPPEAR.
 */
void EmCephadrome_Captured(Em *em)
{
    s32 timerLeft;
    s32 timerPrev;

    Em_SetCaptured(em, 1);
    switch (em->step) {
    case 0:
        em->step++;
        QuestEm_Captured(em);
        em->timer = 1800;
        Em_BeginStatus1(em);
        Em_ClearPoison(em);
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, CEPH_MOT_STATUS_COLLAPSE, 0, 0);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, CEPH_MOT_STATUS_LIE, 0, 0);
        }
        break;
    case 2:
        if (QuestEm_CanAppear(em) != 0) {
            if (--em->timer <= 0) {
                em->step++;
                em->timer = 150;
                break;
            }
        }
        break;
    case 3:
        timerPrev = em->timer;
        if (timerPrev > 0) {
            em->timer = timerPrev - 1;
        }
        timerLeft = em->timer;
        if (timerLeft <= 0) {
            if (em->remote == 0) {
                EmCephadrome_ChangeState(em, EM_GROUP_DEFEAT, CEPH_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
                break;
            }
        } else {
            em->alpha = (f32) timerLeft / 150.0f;
        }
        break;
    }
}

/*
 * CEPH_DEFEAT_DIE: killed on the ground (EM_DMG_KILLED or DEAD, not held):
 * out of combat, stance dead, QuestEm_Hunted, CEPH_MOT_DIE (camera shake at
 * frame 212), then a carve mine point (EmMine_Create) for 900 frames
 * (Cephadrome counts them only while QuestEm_CanAppear, Cephalos always),
 * fades out over 150 frames, then the owner moves to CEPH_DEFEAT_REAPPEAR.
 * Cephalos keeps noHitTimer at 10; both keep noPushTimer at 10.
 */
void EmCephadrome_Die(Em *em, EmWork *work)
{
    if (em->type == EM_KIND_CEPHALOS) {
        em->noHitTimer = 10;
    }
    em->noPushTimer = 10;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, CEPH_MOT_DIE, 0, 0);
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_DEAD;
        QuestEm_Hunted(em);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 212.0f, 0) != 0) {
            CamShake_ReqAIf(em, 2);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 900;
            EmMine_Create(em, 0);
            em->fieldCollide = 0;
        }
        break;
    case 2:
        EmMine_Update(em);
        if (em->type != EM_KIND_CEPHADROME || QuestEm_CanAppear(em) != 0) {
            if (--em->timer <= 0) {
                em->step++;
                EmMine_Remove(em);
                em->timer = 150;
            }
        }
        break;
    case 3:
        if (em->timer > 0) {
            em->timer--;
        }
        if (em->timer <= 0) {
            if (em->remote == 0) {
                EmCephadrome_ChangeState(em, EM_GROUP_DEFEAT, CEPH_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    }
}

/*
 * CEPH_DEFEAT_DIE_HELD: killed while held (holdTimer): as EmCephadrome_Die
 * with CEPH_MOT_DIE_HELD and no camera shake.
 */
void EmCephadrome_DieHeld(Em *em, EmWork *work)
{
    if (em->type == EM_KIND_CEPHALOS) {
        em->noHitTimer = 10;
    }
    em->noPushTimer = 10;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, CEPH_MOT_DIE_HELD, 0, 0);
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_DEAD;
        QuestEm_Hunted(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 900;
            EmMine_Create(em, 0);
            em->fieldCollide = 0;
        }
        break;
    case 2:
        EmMine_Update(em);
        if (em->type != EM_KIND_CEPHADROME || QuestEm_CanAppear(em) != 0) {
            if (--em->timer <= 0) {
                em->step++;
                EmMine_Remove(em);
                em->timer = 150;
            }
        }
        break;
    case 3:
        if (em->timer > 0) {
            em->timer--;
        }
        if (em->timer <= 0) {
            if (em->remote == 0) {
                EmCephadrome_ChangeState(em, EM_GROUP_DEFEAT, CEPH_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    }
}

/*
 * CEPH_DEFEAT_DIE_UNDER: killed under the sand, lying on its side, or in
 * the air below the surface: thrown up (CEPH_MOT_TUMBLE, velY 100) through
 * the sand until it clears surfaceY less surfaceDepth, then in the air
 * (velY 80) falls to the floor, flops (CEPH_MOT_FLOP_A), and then goes on
 * as EmCephadrome_Die (CEPH_MOT_DIE, mine point, fade,
 * CEPH_DEFEAT_REAPPEAR).
 */
void EmCephadrome_DieUnder(Em *em, EmWork *work)
{
    if (em->type == EM_KIND_CEPHALOS) {
        em->noHitTimer = 10;
    }
    em->noPushTimer = 10;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, CEPH_MOT_TUMBLE, 0, 0);
        Player_ClearVelocity(em);
        em->velY = 100.0f;
        em->accelY = -2.75f;
        work->moveAngBX = 0;
        work->moveAngBZ = 0;
        break;
    case 1:
        work->moveAngBY = em->rotY;
        if (!(em->posY < em->surfaceY - *(f32 *) &em->surfaceDepth)) {
            em->stance = EM_STANCE_AIR;
            em->velY = 80.0f;
            Em_MoveRotatedAccel(em, &work->moveAngBX);
            em->step++;
        } else {
            em->stance = EM_STANCE_UNDER;
            Em_MoveRotated(em, &work->moveAngBX);
        }
        break;
    case 2:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, &work->moveAngBX);
        if (em->posY < em->surfaceY) {
            if (em->velY < 0.0f) {
                em->step++;
                em->stance = EM_STANCE_GROUND;
                em->posY = em->floorY;
                Em_SetMotion(em, CEPH_MOT_FLOP_A, 0, 0);
            }
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, CEPH_MOT_DIE, 0, 0);
            em->lookAtTarget = 0;
            em->stance = EM_STANCE_DEAD;
            QuestEm_Hunted(em);
        }
        break;
    case 4:
        if (EmMotion_PassedFrameScaled(em, 212.0f, 0) != 0) {
            CamShake_ReqAIf(em, 2);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 900;
            EmMine_Create(em, 0);
            em->fieldCollide = 0;
        }
        break;
    case 5:
        EmMine_Update(em);
        if (em->type != EM_KIND_CEPHADROME || QuestEm_CanAppear(em) != 0) {
            if (--em->timer <= 0) {
                em->step++;
                EmMine_Remove(em);
                em->timer = 150;
            }
        }
        break;
    case 6:
        if (em->timer > 0) {
            em->timer--;
        }
        if (em->timer <= 0) {
            if (em->remote == 0) {
                EmCephadrome_ChangeState(em, EM_GROUP_DEFEAT, CEPH_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    }
}

/*
 * CEPH_DEFEAT_REAPPEAR: when the quest lets the monster appear again
 * (QuestEm_CanAppear 1; ChangeState already called QuestEm_Appear) it
 * starts again under the sand (stance EM_STANCE_UNDER, CEPH_SAND_IDLE);
 * otherwise its task is finished (taskPhase).
 */
void EmCephadrome_Reappear(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        if (QuestEm_CanAppear(em) == 1) {
            em->stance = EM_STANCE_UNDER;
            EmCephadrome_ChangeState(em, EM_GROUP_FLY, CEPH_SAND_IDLE, EM_SYNC_LOCAL);
            break;
        }
        em->taskPhase++;
        break;
    }
}

/*
 * CEPH_DEFEAT_DIE_FALLING: killed in the air above the sand surface: falls
 * (CEPH_MOT_TUMBLE, accelY -3) to the floor, flops (CEPH_MOT_FLOP_A), then
 * as EmCephadrome_Die. Keeps noPushTimer at 5 (no noHitTimer for Cephalos
 * here).
 */
void EmCephadrome_DieFalling(Em *em, EmWork *work)
{
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, CEPH_MOT_TUMBLE, 0, 0);
        em->stance = EM_STANCE_AIR;
        em->accelY = -3.0f;
        em->accelX = 0.0f;
        em->accelZ = 0;
        work->moveAngBX = 0;
        work->moveAngBZ = 0;
        break;
    case 1:
        work->moveAngBY = em->rotY;
        Em_MoveRotatedAccel(em, &work->moveAngBX);
        if (em->posY < em->floorY) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            Em_SetMotion(em, CEPH_MOT_FLOP_A, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, CEPH_MOT_DIE, 0, 0);
            em->lookAtTarget = 0;
            em->stance = EM_STANCE_DEAD;
            QuestEm_Hunted(em);
        }
        break;
    case 3:
        if (EmMotion_PassedFrameScaled(em, 212.0f, 0) != 0) {
            CamShake_ReqAIf(em, 2);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 900;
            EmMine_Create(em, 0);
            em->fieldCollide = 0;
        }
        break;
    case 4:
        EmMine_Update(em);
        if (em->type != EM_KIND_CEPHADROME || QuestEm_CanAppear(em) != 0) {
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
        if (em->timer <= 0) {
            if (em->remote == 0) {
                EmCephadrome_ChangeState(em, EM_GROUP_DEFEAT, CEPH_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    }
}

/*
 * Runs the current group-0 state (EM_GROUP_IDLE); LandFlop serves 4 and 9
 * (variant 0, 1), StatusCollapse serves 5 and 41 (variant 0, 1), StatusGetUp
 * serves 6 and 42, StatusGetUpBeached serves 15 and 17, ShakeOff serves 7 and
 * 21-23.
 */
void EmCephadrome_RunIdleState(Em *em, EmWork *work)
{
    switch (em->state) {
    case CEPH_IDLE_STAND:
        EmCephadrome_IdleStand(em, work);
        break;
    case CEPH_IDLE:
        EmCephadrome_Idle(em, work);
        break;
    case CEPH_IDLE_ACTION:
        EmCephadrome_IdleAction(em, work);
        break;
    case CEPH_IDLE_EAT_NEED2:
        EmCephadrome_EatNeed2(em, work);
        break;
    case CEPH_IDLE_LAND_FLOP_A:
        EmCephadrome_LandFlop(em, work, 0);
        break;
    case CEPH_IDLE_STATUS2_COLLAPSE:
        EmCephadrome_StatusCollapse(em, (s32) work, 0);
        break;
    case CEPH_IDLE_STATUS2_END:
        EmCephadrome_StatusGetUp(em);
        break;
    case CEPH_IDLE_SHAKE_OFF:
        EmCephadrome_ShakeOff(em);
        break;
    case CEPH_IDLE_LAND_FLOP_B:
        EmCephadrome_LandFlop(em, work, 1);
        break;
    case CEPH_IDLE_STATUS2_COLLAPSE_BEACHED:
        EmCephadrome_Status2CollapseBeached(em, work);
        break;
    case CEPH_IDLE_STATUS2_END_BEACHED:
        EmCephadrome_StatusGetUpBeached(em);
        break;
    case CEPH_IDLE_STATUS1_COLLAPSE_BEACHED:
        EmCephadrome_Status1CollapseBeached(em, work);
        break;
    case CEPH_IDLE_STATUS1_END_BEACHED:
        EmCephadrome_StatusGetUpBeached(em);
        break;
    case CEPH_IDLE_REST:
        EmCephadrome_Rest(em);
        break;
    case CEPH_IDLE_REST_END:
        EmCephadrome_RestEnd(em);
        break;
    case CEPH_IDLE_EAT_START:
        EmCephadrome_EatStart(em);
        break;
    case CEPH_IDLE_STATUS4_SHAKE_OFF:
        EmCephadrome_ShakeOff(em);
        break;
    case CEPH_IDLE_HOLD2_SHAKE_OFF:
        EmCephadrome_ShakeOff(em);
        break;
    case CEPH_IDLE_STUN_SHAKE_OFF:
        EmCephadrome_ShakeOff(em);
        break;
    case CEPH_IDLE_EAT:
        EmCephadrome_Eat(em);
        break;
    case CEPH_IDLE_STATUS1_COLLAPSE:
        EmCephadrome_StatusCollapse(em, (s32) work, 1);
        break;
    case CEPH_IDLE_STATUS1_END:
        EmCephadrome_StatusGetUp(em);
        break;
    }
}

/* Runs the current group-1 state (EM_GROUP_MOVE): states 0-3. */
void EmCephadrome_RunMoveState(Em *em, EmWork *work)
{
    switch (em->state) {
    case CEPH_MOVE_WALK:
        EmCephadrome_Walk(em, work);
        break;
    case CEPH_MOVE_RUN:
        EmCephadrome_RunAndStop(em, work);
        break;
    case CEPH_MOVE_TURN:
        EmCephadrome_TurnToTarget(em, work);
        break;
    case CEPH_MOVE_LIMP:
        EmCephadrome_Limp(em, work);
        break;
    }
}

/*
 * Runs the current group-2 state (EM_GROUP_FLY: for this family swimming
 * under the sand, surfacing and leaping); SandTurn serves 3 and 26
 * (variant 0, 1), SandDash serves 5 and 27, SandRoundNext serves 10 and 19,
 * SandRoundPrev serves 11 and 20, SandStatusRecover serves 24 and 25; 23 runs nothing.
 */
void EmCephadrome_RunSandState(Em *em, EmWork *work)
{
    switch (em->state) {
    case CEPH_SAND_IDLE:
        EmCephadrome_SandIdle(em);
        break;
    case CEPH_SAND_IDLE_B:
        EmCephadrome_SandIdleB(em);
        break;
    case CEPH_SAND_CIRCLE_RIGHT_B:
        EmCephadrome_SandCircleRightB(em, work);
        break;
    case CEPH_SAND_TURN:
        EmCephadrome_SandTurn(em, (s32) work, 0);
        break;
    case CEPH_SAND_FORWARD:
        EmCephadrome_SandForward(em, work);
        break;
    case CEPH_SAND_DASH:
        EmCephadrome_SandDash(em, work, 0);
        break;
    case CEPH_SAND_CURVE:
        EmCephadrome_SandCurve(em);
        break;
    case CEPH_SAND_RISE:
        EmCephadrome_SandRise(em, work);
        break;
    case CEPH_SAND_DIVE:
        EmCephadrome_SandDive(em, work);
        break;
    case CEPH_SAND_ROUND_TURN:
        EmCephadrome_SandRoundTurn(em);
        break;
    case CEPH_SAND_ROUND_NEXT:
        EmCephadrome_SandRoundNext(em, work);
        break;
    case CEPH_SAND_ROUND_PREV:
        EmCephadrome_SandRoundPrev(em, work);
        break;
    case CEPH_SAND_CIRCLE_RIGHT:
        EmCephadrome_SandCircleRight(em, work);
        break;
    case CEPH_SAND_CIRCLE_LEFT:
        EmCephadrome_SandCircleLeft(em, work);
        break;
    case CEPH_SAND_LEAP_ASHORE:
        EmCephadrome_LeapAshore(em, work);
        break;
    case CEPH_SAND_SURFACE_FLOP:
        EmCephadrome_SandSurfaceFlop(em);
        break;
    case CEPH_SAND_LEAP_IN:
        EmCephadrome_LeapIntoSand(em, work);
        break;
    case CEPH_SAND_LEAP:
        EmCephadrome_LeapFromSand(em, work);
        break;
    case CEPH_SAND_LEAP_ON_LAND:
        EmCephadrome_LeapOnLand(em, work);
        break;
    case CEPH_SAND_ROUND_NEXT_LEG:
        EmCephadrome_SandRoundNext(em, work);
        break;
    case CEPH_SAND_ROUND_PREV_LEG:
        EmCephadrome_SandRoundPrev(em, work);
        break;
    case CEPH_SAND_STATUS4_RECOVER:
        EmCephadrome_SandStatus4Recover(em);
        break;
    case CEPH_SAND_LEAP_TO_FIXED_POINT:
        EmCephadrome_LeapToFixedPoint(em, work);
        break;
    case CEPH_SAND_EMPTY:
        break;
    case CEPH_SAND_STATUS2_END:
        EmCephadrome_SandStatusRecover(em);
        break;
    case CEPH_SAND_STATUS1_END:
        EmCephadrome_SandStatusRecover(em);
        break;
    case CEPH_SAND_TURN_NO_REACT:
        EmCephadrome_SandTurn(em, (s32) work, 1);
        break;
    case CEPH_SAND_DASH_NO_REACT:
        EmCephadrome_SandDash(em, work, 1);
        break;
    }
}

/* Runs the current group-3 state (EM_GROUP_ATTACK): states 0-8. */
void EmCephadrome_RunAttackState(Em *em, EmWork *work)
{
    switch (em->state) {
    case CEPH_ATK_LAND_A:
        EmCephadrome_LandAttackA(em, work);
        break;
    case CEPH_ATK_BREATH:
        EmCephadrome_Breath(em, work);
        break;
    case CEPH_ATK_SPIN:
        EmCephadrome_SpinAttack(em, work);
        break;
    case CEPH_ATK_SURFACE_BREATH_A:
        EmCephadrome_SurfaceBreathA(em, work);
        break;
    case CEPH_ATK_SURFACE_BREATH_B:
        EmCephadrome_SurfaceBreathB(em, work);
        break;
    case CEPH_ATK_SURFACE_BREATH_C:
        EmCephadrome_SurfaceBreathC(em, work);
        break;
    case CEPH_ATK_LEAP:
        EmCephadrome_LeapAttack(em, work);
        break;
    case CEPH_ATK_LAND_B:
        EmCephadrome_LandAttackB(em, work);
        break;
    case CEPH_ATK_SURFACE_LUNGE:
        EmCephadrome_SurfaceLunge(em, work);
        
    default:
        break;
    }
}

/*
 * Runs the current group-4 state (EM_GROUP_REACT); KnockDown serves 5, 20,
 * 22, 23 (variants 1, 0, 3, 2), ThrownOutOfSand serves 17 and 18 (variant 0, 1);
 * 8 and 12-15 run nothing.
 */
void EmCephadrome_RunReactState(Em *em, EmWork *work)
{
    switch (em->state) {
    case CEPH_REACT_FLINCH:
        EmCephadrome_Flinch(em);
        break;
    case CEPH_REACT_FLINCH_B:
        EmCephadrome_FlinchB(em);
        break;
    case CEPH_REACT_FLINCH_C:
        EmCephadrome_FlinchC(em);
        break;
    case CEPH_REACT_FLINCH_D:
        EmCephadrome_FlinchD(em);
        break;
    case CEPH_REACT_FLINCH_LYING:
        EmCephadrome_FlinchLying(em);
        break;
    case CEPH_REACT_KNOCK_DOWN:
        EmCephadrome_KnockDown(em, work, 1);
        break;
    case CEPH_REACT_FALL:
        EmCephadrome_FallFromAir(em, work);
        break;
    case CEPH_REACT_STATUS4_LYING:
        EmCephadrome_Status4Lying(em);
        break;
    case CEPH_REACT_EMPTY_A:
        break;
    case CEPH_REACT_FALL_BLIND:
        EmCephadrome_FallBlinded(em, work);
        break;
    case CEPH_REACT_FLINCH_BLIND:
        EmCephadrome_FlinchBlinded(em);
        break;
    case CEPH_REACT_STATUS4:
        EmCephadrome_Status4(em);
        break;
    case CEPH_REACT_EMPTY_B:
        break;
    case CEPH_REACT_EMPTY_C:
        break;
    case CEPH_REACT_EMPTY_D:
        break;
    case CEPH_REACT_EMPTY_E:
        break;
    case CEPH_REACT_LAND:
        EmCephadrome_LandAfterFall(em);
        break;
    case CEPH_REACT_THROWN_OUT:
        EmCephadrome_ThrownOutOfSand(em, work, 0);
        break;
    case CEPH_REACT_THROWN_OUT_ATTACK5:
        EmCephadrome_ThrownOutOfSand(em, work, 1);
        break;
    case CEPH_REACT_STATUS4_LYING_END:
        EmCephadrome_Status4LyingEnd(em);
        break;
    case CEPH_REACT_KNOCK_DOWN_PART3:
        EmCephadrome_KnockDown(em, work, 0);
        break;
    case CEPH_REACT_HOLD2:
        EmCephadrome_Hold2(em);
        break;
    case CEPH_REACT_STUN_A:
        EmCephadrome_KnockDown(em, work, 3);
        break;
    case CEPH_REACT_STUN_B:
        EmCephadrome_KnockDown(em, work, 2);
        break;
    }
}

/*
 * Runs the current group-5 state (EM_GROUP_DEFEAT): die, die held, die
 * under the sand, reappear, die falling (no case 3).
 */
void EmCephadrome_RunDefeatState(Em *em, EmWork *work)
{
    switch (em->state) {
    case CEPH_DEFEAT_DIE:
        EmCephadrome_Die(em, work);
        break;
    case CEPH_DEFEAT_DIE_HELD:
        EmCephadrome_DieHeld(em, work);
        break;
    case CEPH_DEFEAT_DIE_UNDER:
        EmCephadrome_DieUnder(em, work);
        break;
    case CEPH_DEFEAT_REAPPEAR:
        EmCephadrome_Reappear(em, work);
        break;
    case CEPH_DEFEAT_DIE_FALLING:
        EmCephadrome_DieFalling(em, work);
        break;
    }
}

/*
 * Runs the current group-6 state (EM_GROUP_EVENT; RunState also uses it for
 * group 7): the entrance, hidden-to-idle, captured.
 */
void EmCephadrome_RunEventState(Em *em, EmWork *work)
{
    switch (em->state) {
    case CEPH_EVENT_ENTRANCE:
        EmCephadrome_EntranceScene(em, work);
        break;
    case CEPH_EVENT_TO_IDLE:
        EmCephadrome_HiddenToIdle(em);
        break;
    case CEPH_EVENT_CAPTURED:
        EmCephadrome_Captured(em);
        break;
    }
}

/*
 * The family's frame: the combat timer, the area trigger, a monster under
 * the sand off sand in the hunters' area goes to land idle
 * (EmCephadrome_IsUnderOffSand, EndGroundState); a calm Cephadrome of variant
 * other than 1 checks low hp at a tenth (Em_CheckLowHp). Then the reaction
 * Em_UpdateDamage's result picks (see the NOTE lines), the pending script,
 * the current state (twice when a state was requested), and on sand in the
 * hunters' area: under the sand posY is kept between floorY (at most 1000
 * below surfaceY) and surfaceY less surfaceDepth, on the ground floorY is
 * surfaceY.
 */
void EmCephadrome_Update(Em *em)
{
    u8 dmgOut[4];
    u8 *w = &em->work;
    f32 limitY;

    Em_UpdateCombatTimer(em);
    Em_RaisePlayerAreaTriggerLax(em);
    if (EmCephadrome_IsUnderOffSand(em) == 1) {
        EmCephadrome_EndGroundState(em);
    }
    if (em->noAlert != 1 && em->rage == 0 && em->type == EM_KIND_CEPHADROME && em->variant != 1) {
        Em_CheckLowHp(em, 0.1f);
    }
    switch (Em_UpdateDamage(em, dmgOut) & 0xFF) {
    case EM_DMG_NONE:
    case EM_DMG_ATTACK9:
    case EM_DMG_ATTACK6:
    case EM_DMG_FLASH:
    case EM_DMG_PART8_DOWN:
    case EM_DMG_HIT:
        break;
    case EM_DMG_KILLED:
    case EM_DMG_DEAD:
        if ((s8) em->holdTimer != 0) {
            EmCephadrome_ChangeState(em, EM_GROUP_DEFEAT, CEPH_DEFEAT_DIE_HELD, EM_SYNC_OWNER);
        } else {
            switch (em->stance) {
            case EM_STANCE_UNDER:
            case EM_STANCE_FAMILY:
                EmCephadrome_ChangeState(em, EM_GROUP_DEFEAT, CEPH_DEFEAT_DIE_UNDER, EM_SYNC_OWNER);
                break;
            case EM_STANCE_AIR:
                if (em->posY >= em->surfaceY) {
                    EmCephadrome_ChangeState(em, EM_GROUP_DEFEAT, CEPH_DEFEAT_DIE_FALLING, EM_SYNC_OWNER);
                } else {
                    EmCephadrome_ChangeState(em, EM_GROUP_DEFEAT, CEPH_DEFEAT_DIE_UNDER, EM_SYNC_OWNER);
                }
                break;
            default:
            case EM_STANCE_GROUND:
                EmCephadrome_ChangeState(em, EM_GROUP_DEFEAT, CEPH_DEFEAT_DIE, EM_SYNC_OWNER);
                break;
            }
        }
        break;
    case EM_DMG_STATUS5:
        if (em->stance == EM_STANCE_UNDER) {
            Em_EndStatus5(em);
        } else if ((em->stateGroup != EM_GROUP_REACT || em->state != CEPH_REACT_STUN_A) && (em->stateGroup != EM_GROUP_REACT || em->state != CEPH_REACT_STUN_B) && (em->stateGroup != EM_GROUP_REACT || em->state != CEPH_REACT_FALL)) {
            Em_SetStatus5Timer(em);
            if ((u16) (em->hitAngle - em->rotY) > (ANGLE_TURN / 2)) {
                EmCephadrome_ChangeState(em, EM_GROUP_REACT, CEPH_REACT_STUN_A, EM_SYNC_OWNER);
            } else {
                EmCephadrome_ChangeState(em, EM_GROUP_REACT, CEPH_REACT_STUN_B, EM_SYNC_OWNER);
            }
        }
        break;
    case EM_DMG_ATTACK13:
        Em_ResetKindTimer(em);
        EmCephadrome_ChangeState(em, EM_GROUP_REACT, CEPH_REACT_HOLD2, EM_SYNC_OWNER);
        break;
    case EM_DMG_ATTACK5:
        if (em->stance == EM_STANCE_UNDER) {
            EmCephadrome_ChangeState(em, EM_GROUP_REACT, CEPH_REACT_THROWN_OUT_ATTACK5, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_STATUS_IN_AIR:
        if ((em->stateGroup != EM_GROUP_REACT || em->state != CEPH_REACT_FALL) && (em->stateGroup != EM_GROUP_REACT || em->state != CEPH_REACT_THROWN_OUT) && (em->stateGroup != EM_GROUP_REACT || em->state != CEPH_REACT_THROWN_OUT_ATTACK5)) {
            EmCephadrome_ChangeState(em, EM_GROUP_REACT, CEPH_REACT_FALL, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_STATUS4:
        if ((em->stateGroup != EM_GROUP_REACT || em->state != CEPH_REACT_STATUS4) && (em->stateGroup != EM_GROUP_REACT || em->state != CEPH_REACT_FALL) && (em->stateGroup != EM_GROUP_REACT || em->state != CEPH_REACT_STATUS4_LYING) && em->stance == EM_STANCE_FAMILY) {
            Em_SetStatus4Timer(em);
            EmCephadrome_ChangeState(em, EM_GROUP_REACT, CEPH_REACT_STATUS4_LYING, EM_SYNC_OWNER);
        } else if ((em->stateGroup != EM_GROUP_REACT || em->state != CEPH_REACT_STATUS4) && (em->stateGroup != EM_GROUP_REACT || em->state != CEPH_REACT_FALL) && (em->stateGroup != EM_GROUP_REACT || em->state != CEPH_REACT_STATUS4_LYING) && em->stance == EM_STANCE_GROUND) {
            Em_SetStatus4Timer(em);
            EmCephadrome_ChangeState(em, EM_GROUP_REACT, CEPH_REACT_STATUS4, EM_SYNC_OWNER);
        } else if ((em->stateGroup != EM_GROUP_REACT || em->state != CEPH_REACT_FALL) && (em->stateGroup != EM_GROUP_REACT || em->state != CEPH_REACT_THROWN_OUT) && (em->stateGroup != EM_GROUP_REACT || em->state != CEPH_REACT_THROWN_OUT_ATTACK5) && em->stance == EM_STANCE_UNDER) {
            Em_SetStatus4Timer(em);
            EmCephadrome_ChangeState(em, EM_GROUP_REACT, CEPH_REACT_THROWN_OUT, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_CAPTURED:
        EmCephadrome_ChangeState(em, EM_GROUP_EVENT, CEPH_EVENT_CAPTURED, EM_SYNC_OWNER);
        break;
    case EM_DMG_STATUS2:
        if ((em->stateGroup != EM_GROUP_REACT || em->state != CEPH_REACT_FALL) && (em->stateGroup != EM_GROUP_REACT || em->state != CEPH_REACT_THROWN_OUT) && (em->stateGroup != EM_GROUP_REACT || em->state != CEPH_REACT_THROWN_OUT_ATTACK5) && em->stance == EM_STANCE_UNDER) {
            EmCephadrome_ChangeState(em, EM_GROUP_REACT, CEPH_REACT_THROWN_OUT, EM_SYNC_OWNER);
        } else if ((em->stateGroup != EM_GROUP_IDLE || em->state != CEPH_IDLE_STATUS2_COLLAPSE_BEACHED) && (em->stateGroup != EM_GROUP_REACT || em->state != CEPH_REACT_FALL) && em->stance == EM_STANCE_FAMILY) {
            Em_SetStatus2Timer(em);
            EmCephadrome_ChangeState(em, EM_GROUP_IDLE, CEPH_IDLE_STATUS2_COLLAPSE_BEACHED, EM_SYNC_OWNER);
        } else if ((em->stateGroup != EM_GROUP_IDLE || em->state != CEPH_IDLE_STATUS2_COLLAPSE) && (em->stateGroup != EM_GROUP_REACT || em->state != CEPH_REACT_FALL) && em->stance == EM_STANCE_GROUND) {
            Em_SetStatus2Timer(em);
            EmCephadrome_ChangeState(em, EM_GROUP_IDLE, CEPH_IDLE_STATUS2_COLLAPSE, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_HIT_LYING:
        switch (em->state) {
        case CEPH_IDLE_STATUS2_COLLAPSE:
            EmCephadrome_ChangeState(em, EM_GROUP_IDLE, CEPH_IDLE_STATUS2_END, EM_SYNC_OWNER);
            em->scriptPending = 0;
            break;
        case CEPH_IDLE_STATUS2_COLLAPSE_BEACHED:
            EmCephadrome_ChangeState(em, EM_GROUP_IDLE, CEPH_IDLE_STATUS2_END_BEACHED, EM_SYNC_OWNER);
            em->scriptPending = 0;
            break;
        case CEPH_IDLE_REST:
            EmCephadrome_ChangeState(em, EM_GROUP_IDLE, CEPH_IDLE_REST_END, EM_SYNC_OWNER);
            em->scriptPending = 0;
            break;
        case CEPH_IDLE_STATUS1_COLLAPSE:
            EmCephadrome_ChangeState(em, EM_GROUP_IDLE, CEPH_IDLE_STATUS1_END, EM_SYNC_OWNER);
            em->scriptPending = 0;
            break;
        }
        break;
    case EM_DMG_PART_FLINCH:
    case EM_DMG_ATTACK5_FLINCH:
        switch (em->stance) {
        case EM_STANCE_GROUND:
        case EM_STANCE_DEAD:
            switch (em->flinchPart) {
            case 0:
            case 7:
                EmCephadrome_ChangeState(em, EM_GROUP_REACT, CEPH_REACT_FLINCH, EM_SYNC_OWNER);
                break;
            case 5:
            case 6:
                EmCephadrome_ChangeState(em, EM_GROUP_REACT, CEPH_REACT_FLINCH_C, EM_SYNC_OWNER);
                break;
            case 1:
            case 2:
                EmCephadrome_ChangeState(em, EM_GROUP_REACT, CEPH_REACT_FLINCH_D, EM_SYNC_OWNER);
                break;
            case 3:
            case 4:
            default:
                if (em->parts[em->flinchPart].state >= 2) {
                    if (em->flinchPart != 3) {
                        EmCephadrome_ChangeState(em, EM_GROUP_REACT, CEPH_REACT_KNOCK_DOWN, EM_SYNC_OWNER);
                    } else {
                        EmCephadrome_ChangeState(em, EM_GROUP_REACT, CEPH_REACT_KNOCK_DOWN_PART3, EM_SYNC_OWNER);
                    }
                } else {
                    EmCephadrome_ChangeState(em, EM_GROUP_REACT, CEPH_REACT_FLINCH_B, EM_SYNC_OWNER);
                }
                break;
            }
            break;
        case EM_STANCE_FAMILY:
            EmCephadrome_ChangeState(em, EM_GROUP_REACT, CEPH_REACT_FLINCH_LYING, EM_SYNC_OWNER);
            break;
        case EM_STANCE_AIR:
            EmCephadrome_ChangeState(em, EM_GROUP_REACT, CEPH_REACT_FALL, EM_SYNC_OWNER);
            break;
        case EM_STANCE_UNDER:
            EmCephadrome_ChangeState(em, EM_GROUP_REACT, CEPH_REACT_THROWN_OUT, EM_SYNC_OWNER);
            break;
        }
        break;
    case EM_DMG_HIT_MODE3:
        if (em->stance != EM_STANCE_AIR) {
            EmCephadrome_ChangeState(em, EM_GROUP_REACT, CEPH_REACT_FLINCH, EM_SYNC_OWNER);
            em->scriptPending = 0;
        }
        break;
    }
    Em_UpdatePendingScript(em);
    Em_ClearHitReactMode(em);
    EmCephadrome_RunState(em, (s32) w);
    if (em->stateRequest != 0) {
        EmCephadrome_RunState(em, (s32) w);
        em->stateRequest = 0;
    }
    if (em->surfaceKind != 0 && gScene.area == em->area) {
        if (em->stance == EM_STANCE_UNDER) {
            limitY = em->surfaceY - *(f32 *) &em->surfaceDepth;
            if (em->posY > limitY) {
                em->posY = limitY;
            }
            limitY = em->surfaceY - 1000.0f;
            if (em->floorY >= limitY) {
                em->floorY = limitY;
            }
            if (em->posY < em->floorY) {
                em->posY = em->floorY;
            }
        }
        if (em->stance == EM_STANCE_GROUND) {
            em->floorY = em->surfaceY;
            if (em->posY < em->floorY) {
                em->posY = em->floorY;
            }
        }
    }
}

/*
 * Runs the current state through its group's dispatcher (group 7 through
 * group 6's), keeping the group and state as the previous ones.
 */
void EmCephadrome_RunState(Em *em, s32 work)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->stateGroup) {
    case EM_GROUP_IDLE:
        EmCephadrome_RunIdleState(em, (EmWork *) work);
        break;
    case EM_GROUP_MOVE:
        EmCephadrome_RunMoveState(em, (EmWork *) work);
        break;
    case EM_GROUP_FLY:
        EmCephadrome_RunSandState(em, (EmWork *) work);
        break;
    case EM_GROUP_ATTACK:
        EmCephadrome_RunAttackState(em, (EmWork *) work);
        break;
    case EM_GROUP_REACT:
        EmCephadrome_RunReactState(em, (EmWork *) work);
        break;
    case EM_GROUP_DEFEAT:
        EmCephadrome_RunDefeatState(em, (EmWork *) work);
        break;
    case EM_GROUP_EVENT:
        EmCephadrome_RunEventState(em, (EmWork *) work);
        break;
    case EM_GROUP_EVENT2:
        EmCephadrome_RunEventState(em, (EmWork *) work);
        
    default:
        break;
    }
}

/*
 * Requests camera shake level `level` (CamShake_ReqAIf) when the motion
 * passes frame `frame` (as EmPlesioth_CamShakeOnFrame).
 */
void EmCephadrome_CamShakeOnFrame(s32 em, s32 frame, s32 level)
{
    if (EmMotion_PassedFrameScaled((Em *) em, (f32) frame, 0U) != 0) {
        CamShake_ReqAIf((void *) em, level);
    }
}

/* MotionEffects' default case: does nothing. */
static void EmCephadrome_MotionEffectsDefault(Em *em)
{
}

/*
 * The effects of each motion: keeps the layer-0 motion id in
 * work->prevMotion, plays its sounds (EmCephadrome_MotionSounds), sets the
 * fin mode by motion (EmCephadrome_SetFinMode: 1 for the walks, land
 * attacks and leaps, 0 for the swims, turns and motions under the sand, 3
 * for resting, lying and the status motions) and on given frames creates
 * the attack volumes (EmCephadrome_CreateAttack), ground clouds
 * (DustCloud_Create; extra every 4th frame while enraged), sand sprays
 * (PartFx_SpawnForEmScaled), the lying puffs (Em_SpawnLyingPuffs), the
 * landing dust (CEPH_MOT_TOUCH_DOWN at frame 16) and for motion 88 a camera shake
 * at frame 26 with a pad vibration (EmCephadrome_VibrateNearby).
 */
void EmCephadrome_MotionEffects(Em *em, EmWork *work)
{
    f32 puffOfsA[3];
    f32 puffOfsB[3];
    f32 puffOfsC[3];

    if (em->motionId[0] != work->prevMotion) {
        work->prevMotion = em->motionId[0];
    }
    EmCephadrome_MotionSounds(em, work);
    switch (work->prevMotion) {
    case EM_MOTION_ID(0, CEPH_MOT_IDLE):
        EmCephadrome_SetFinMode(em, work, 3);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_WALK):
        EmCephadrome_SetFinMode(em, work, 1);
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            EmCephadrome_CreateAttack(em, 2);
        }
        if (EmMotion_PassedFrameScaled(em, 50.0f, 0) != 0) {
            EmCephadrome_CreateAttack(em, 3);
        }
        if (EmMotion_PassedFrameScaled(em, 120.0f, 0) != 0) {
            EmCephadrome_CreateAttack(em, 4);
            break;
        }
        break;
    case EM_MOTION_ID(0, CEPH_MOT_RUN):
        EmCephadrome_SetFinMode(em, work, 0);
        if (EmMotion_PassedFrameScaled(em, 22.0f, 0) != 0) {
            EmCephadrome_CreateAttack(em, 19);
        }
        if (EmMotion_PassedFrameScaled(em, 16.0f, 0) || EmMotion_PassedFrameScaled(em, 56.0f, 0) || EmMotion_PassedFrameScaled(em, 102.0f, 0)) {
            EmCephadrome_CreateAttack(em, 20);
        }
        if ((EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 78.0f, 0) != 0)) {
            EmCephadrome_CreateAttack(em, 21);
        }
        if ((EmMotion_PassedFrameScaled(em, 18.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 66.0f, 0) != 0)) {
            PartFx_SpawnForEmScaled(em, CEPH_BONE_STEP_B, 3, 4.0f);
        }
        if ((EmMotion_PassedFrameScaled(em, 42.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 90.0f, 0) != 0)) {
            PartFx_SpawnForEmScaled(em, CEPH_BONE_STEP_A, 3, 4.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, CEPH_MOT_TURN_RIGHT):
        EmCephadrome_SetFinMode(em, work, 1);
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            EmCephadrome_CreateAttack(em, 5);
        }
        if (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) {
            EmCephadrome_CreateAttack(em, 6);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 2.0f, 78.0f) != 0) && !(gScene.frame & 3)) {
            DustCloud_Create(em, 26, 1, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, CEPH_MOT_TURN_LEFT):
        EmCephadrome_SetFinMode(em, work, 1);
        if (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) {
            EmCephadrome_CreateAttack(em, 7);
        }
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            EmCephadrome_CreateAttack(em, 8);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 2.0f, 78.0f) != 0) && !(gScene.frame & 3)) {
            DustCloud_Create(em, 26, 0, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, CEPH_MOT_LIMP):
        EmCephadrome_SetFinMode(em, work, 1);
        if (EmMotion_PassedFrameScaled(em, 42.0f, 0) != 0) {
            EmCephadrome_CreateAttack(em, 17);
        }
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            EmCephadrome_CreateAttack(em, 18);
        }
        if (EmMotion_PassedFrameScaled(em, 74.0f, 0) != 0) {
            EmCephadrome_CreateAttack(em, 18);
            break;
        }
        break;
    case EM_MOTION_ID(0, 11):
        EmCephadrome_SetFinMode(em, work, 1);
        break;
    case EM_MOTION_ID(0, 17):
        break;
    case EM_MOTION_ID(0, 18):
        EmCephadrome_SetFinMode(em, work, 1);
        if (EmMotion_PassedFrameScaled(em, 42.0f, 0) || EmMotion_PassedFrameScaled(em, 102.0f, 0) || EmMotion_PassedFrameScaled(em, 156.0f, 0)) {
            if (em->posY <= (1000.0f + em->floorY)) {
                DustCloud_Create(em, 1, 0, 1.0f);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, CEPH_MOT_TOUCH_DOWN):
        EmCephadrome_SetFinMode(em, work, 1);
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            EmCephadrome_CreateAttack(em, 13);
        }
        if (EmMotion_PassedFrameScaled(em, 16.0f, 0) != 0) {
            EmCephadrome_SpawnLandingDust(em);
            break;
        }
        break;
    case EM_MOTION_ID(0, CEPH_MOT_IDLE_ACTION):
        EmCephadrome_SetFinMode(em, work, 3);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_REST):
        puffOfsA[0] = 0.0f;
        puffOfsA[1] = 10.0f;
        puffOfsA[2] = 140.0f;
        Em_SpawnLyingPuffs(em, CEPH_BONE_BREATH, puffOfsA, 1.6f);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_REST_START):
    case EM_MOTION_ID(0, CEPH_MOT_REST_END):
        break;
    case EM_MOTION_ID(0, CEPH_MOT_STATUS_LIE):
        EmCephadrome_SetFinMode(em, work, 3);
        puffOfsB[0] = 0.0f;
        puffOfsB[1] = 10.0f;
        puffOfsB[2] = 140.0f;
        Em_SpawnLyingPuffs(em, CEPH_BONE_BREATH, puffOfsB, 1.6f);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_LAND_ATTACK_A):
        EmCephadrome_SetFinMode(em, work, 3);
        if (EmMotion_PassedFrameScaled(em, 50.0f, 0) != 0) {
            EmCephadrome_CreateAttack(em, 1);
            break;
        }
        break;
    case EM_MOTION_ID(0, CEPH_MOT_STATUS_GET_UP):
        EmCephadrome_SetFinMode(em, work, 3);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_STATUS_COLLAPSE):
        EmCephadrome_SetFinMode(em, work, 3);
        if (EmMotion_PassedFrameScaled(em, 148.0f, 0) != 0) {
            EmCephadrome_CreateAttack(em, 15);
        }
        puffOfsC[0] = 0.0f;
        puffOfsC[1] = 10.0f;
        puffOfsC[2] = 140.0f;
        Em_SpawnLyingPuffs(em, CEPH_BONE_BREATH, puffOfsC, 1.6f);
        if (EmMotion_PassedFrameScaled(em, 156.0f, 0) != 0) {
            DustCloud_Create(em, 0, 0, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, CEPH_MOT_SPIN):
        EmCephadrome_SetFinMode(em, work, 1);
        if (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) {
            EmCephadrome_CreateAttack(em, 16);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 6.0f, 38.0f) != 0) && !(gScene.frame & 3)) {
            DustCloud_Create(em, 26, 1, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, CEPH_MOT_BREATH):
        EmCephadrome_SetFinMode(em, work, 1);
        if (EmMotion_PassedFrameScaled(em, 66.0f, 0) != 0) {
            EmCephadrome_CreateAttack(em, 25);
            break;
        }
        break;
    case EM_MOTION_ID(0, 48):
    case EM_MOTION_ID(0, CEPH_MOT_FEED):
        break;
    case EM_MOTION_ID(0, CEPH_MOT_FLINCH):
        EmCephadrome_SetFinMode(em, work, 1);
        if (EmMotion_PassedFrameScaled(em, 44.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 27, 7, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 62.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, CEPH_BONE_FOOT_A, 7, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 80.0f, 0) != 0) {
            DustCloud_Create(em, 1, 128, 0.5f);
            break;
        }
        break;
    case EM_MOTION_ID(0, CEPH_MOT_FLINCH_BLIND):
        if (EmMotion_PassedFrameScaled(em, 24.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, CEPH_BONE_STEP_B, 7, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, CEPH_MOT_SHAKE_OFF):
        EmCephadrome_SetFinMode(em, work, 3);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_FLINCH_C):
        EmCephadrome_SetFinMode(em, work, 1);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_FLINCH_D):
        EmCephadrome_SetFinMode(em, work, 1);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_FLINCH_B):
        EmCephadrome_SetFinMode(em, work, 1);
        if (EmMotion_PassedFrameScaled(em, 268.0f, 0) != 0) {
            DustCloud_Create(em, 0, 0, 0.9f);
            break;
        }
        break;
    case EM_MOTION_ID(0, 67):
        EmCephadrome_SetFinMode(em, work, 1);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_DIE):
        EmCephadrome_SetFinMode(em, work, 0);
        if (EmMotion_PassedFrameScaled(em, 50.0f, 0) != 0) {
            DustCloud_Create(em, 0, 0, 0.8f);
            break;
        }
        break;
    case EM_MOTION_ID(0, CEPH_MOT_FALL_A):
        EmCephadrome_SetFinMode(em, work, 1);
        if (EmMotion_PassedFrameScaled(em, 26.0f, 0) != 0) {
            DustCloud_Create(em, 18, 128, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, CEPH_MOT_LIE_A):
        EmCephadrome_SetFinMode(em, work, 3);
        if ((EmMotion_PassedFrameScaled(em, 16.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 66.0f, 0) != 0)) {
            PartFx_SpawnForEmScaled(em, CEPH_BONE_BODY, 6, 4.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, CEPH_MOT_GET_UP_A):
        EmCephadrome_SetFinMode(em, work, 3);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_FALL_B):
        EmCephadrome_SetFinMode(em, work, 1);
        if (EmMotion_PassedFrameScaled(em, 26.0f, 0) != 0) {
            DustCloud_Create(em, 18, 129, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, CEPH_MOT_LIE_DOWN):
        EmCephadrome_SetFinMode(em, work, 3);
        if ((EmMotion_PassedFrameScaled(em, 16.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 66.0f, 0) != 0)) {
            PartFx_SpawnForEmScaled(em, CEPH_BONE_BODY, 6, 4.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, CEPH_MOT_STAND_STILL):
        EmCephadrome_SetFinMode(em, work, 3);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_HEAD_DOWN):
        break;
    case EM_MOTION_ID(0, 95):
        EmCephadrome_SetFinMode(em, work, 1);
        break;
    case EM_MOTION_ID(0, 96):
        EmCephadrome_SetFinMode(em, work, 1);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_SAND_IDLE):
        EmCephadrome_SetFinMode(em, work, 0);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_SAND_SWIM_START):
        EmCephadrome_SetFinMode(em, work, 0);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_SAND_SWIM):
        EmCephadrome_SetFinMode(em, work, 0);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_SAND_SWIM_FAST):
        EmCephadrome_SetFinMode(em, work, 0);
        if (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0) {
            EmCephadrome_CreateAttack(em, 26);
            break;
        }
        break;
    case EM_MOTION_ID(0, CEPH_MOT_SAND_RISE):
        EmCephadrome_SetFinMode(em, work, 0);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_SAND_DIVE):
        EmCephadrome_SetFinMode(em, work, 0);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_SAND_TURN_LEFT):
        EmCephadrome_SetFinMode(em, work, 0);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_SAND_TURN_RIGHT):
        EmCephadrome_SetFinMode(em, work, 0);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_LEAP_ATTACK):
        EmCephadrome_SetFinMode(em, work, 0);
        if (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0) {
            EmCephadrome_CreateAttack(em, 14);
            break;
        }
        break;
    case EM_MOTION_ID(0, 111):
        EmCephadrome_SetFinMode(em, work, 0);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_SURFACE_BREATH_C):
        EmCephadrome_SetFinMode(em, work, 0);
        EmCephadrome_CamShakeOnFrame((s32) em, 26, 3);
        if (EmMotion_PassedFrameScaled(em, 26.0f, 0) != 0) {
            EmCephadrome_VibrateNearby(em);
            EmCephadrome_CreateAttack(em, 29);
            break;
        }
        break;
    case EM_MOTION_ID(0, CEPH_MOT_SURFACE_LUNGE):
        if (EmMotion_PassedFrameScaled(em, 28.0f, 0) != 0) {
            EmCephadrome_CreateAttack(em, 27);
        }
        EmCephadrome_SetFinMode(em, work, 0);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_SAND_CIRCLE_RIGHT):
        EmCephadrome_SetFinMode(em, work, 0);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_SAND_CIRCLE_LEFT):
        EmCephadrome_SetFinMode(em, work, 0);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_SURFACE_BREATH_A):
        EmCephadrome_SetFinMode(em, work, 0);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_SURFACE_BREATH_B):
        EmCephadrome_SetFinMode(em, work, 0);
        break;
    case EM_MOTION_ID(0, 118):
        break;
    case EM_MOTION_ID(0, CEPH_MOT_LEAP):
        EmCephadrome_SetFinMode(em, work, 0);
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            EmCephadrome_CreateAttack(em, 24);
            break;
        }
        break;
    case EM_MOTION_ID(0, CEPH_MOT_LEAP_CROUCH):
        EmCephadrome_SetFinMode(em, work, 0);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_FLOP_A):
        EmCephadrome_SetFinMode(em, work, 0);
        if (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0) {
            DustCloud_Create(em, 0, 137, 0.7f);
            break;
        }
        break;
    case EM_MOTION_ID(0, CEPH_MOT_BEACHED):
        EmCephadrome_SetFinMode(em, work, 0);
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            EmCephadrome_CreateAttack(em, 23);
        }
        if ((EmMotion_PassedFrameScaled(em, 24.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 42.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 62.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 82.0f, 0) != 0)) {
            PartFx_SpawnForEmScaled(em, 6, 3, 8.0f + (0.0005f * (f32) ((u16) Rand_Next(1) & 0x3FF)));
        }
        if ((EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 32.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 52.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 74.0f, 0) != 0)) {
            PartFx_SpawnForEmScaled(em, 12, 3, 8.0f + (0.0005f * (f32) ((u16) Rand_Next(1) & 0x3FF)));
            break;
        }
        break;
    case EM_MOTION_ID(0, CEPH_MOT_GET_UP):
        EmCephadrome_SetFinMode(em, work, 0);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_TUMBLE):
        EmCephadrome_SetFinMode(em, work, 3);
        break;
    case EM_MOTION_ID(0, 127):
        EmCephadrome_SetFinMode(em, work, 0);
        if (EmMotion_PassedFrameScaled(em, 42.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, CEPH_BONE_BODY, 6, 4.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, CEPH_MOT_LIE_FLAP):
        EmCephadrome_SetFinMode(em, work, 3);
        if (EmMotion_PassedFrameScaled(em, 42.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, CEPH_BONE_BODY, 6, 4.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, CEPH_MOT_LIE_FLAP_B):
        EmCephadrome_SetFinMode(em, work, 3);
        if (EmMotion_PassedFrameScaled(em, 46.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, CEPH_BONE_BODY, 6, 4.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, 130):
        EmCephadrome_SetFinMode(em, work, 0);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_SAND_RECOVER):
        EmCephadrome_SetFinMode(em, work, 0);
        break;
    case EM_MOTION_ID(0, 132):
        EmCephadrome_SetFinMode(em, work, 0);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_RUN_STOP):
        EmCephadrome_SetFinMode(em, work, 0);
        if ((EmMotion_PassedFrameScaled(em, 6.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0)) {
            PartFx_SpawnForEmScaled(em, CEPH_BONE_BODY, 3, 7.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, CEPH_MOT_SAND_SURFACED):
        EmCephadrome_SetFinMode(em, work, 0);
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            EmCephadrome_CreateAttack(em, 28);
            break;
        }
        break;
    case EM_MOTION_ID(0, CEPH_MOT_SAND_BOTTOMED):
        EmCephadrome_SetFinMode(em, work, 0);
        break;
    case EM_MOTION_ID(0, CEPH_MOT_LAND_ATTACK_B):
        EmCephadrome_SetFinMode(em, work, 0);
        if (EmMotion_PassedFrameScaled(em, 60.0f, 0) != 0) {
            EmCephadrome_CreateAttack(em, 22);
            break;
        }
        break;
    case EM_MOTION_ID(0, CEPH_MOT_SIDE_LIE):
    case EM_MOTION_ID(0, CEPH_MOT_STATUS4_LYING):
        break;
    default:
        EmCephadrome_MotionEffectsDefault(em);
        break;
    }
}

extern u16 EmCephadrome_FinFoldTarget[];

/*
 * Fin i's fold (finRot[i].fold, a turn about x): in fin modes 2 and 3 folds
 * it to EmCephadrome_FinFoldTarget[i] over 20 frames, in mode 1 opens it back
 * to 0 over 20 frames; mode 0 leaves it as it is.
 */
static void EmCephadrome_UpdateFinFold(Em *em, EmWork *work, s32 i)
{
    EmCephadromeWork *fins = (EmCephadromeWork *) work;
    u8 *foldState;

    /* the cast (a no-op) gives the game's operand order (&p->finState[i][1] swaps an addu) */
    switch (*(foldState = &((u8 (*)[2]) fins->finState)[i][1])) {
    case 0:
        if (fins->finMode == 2 || fins->finMode == 3) {
            if (fins->finRot[i].fold != EmCephadrome_FinFoldTarget[i]) {
                *foldState = 2;
                fins->finCount[i] = 20;
            }
        } else if (fins->finMode == 1) {
            if (fins->finRot[i].fold != 0) {
                *foldState = 1;
                fins->finCount[i] = 20;
            }
        }
        break;
    case 1:
        if (--fins->finCount[i] <= 0) {
            fins->finRot[i].fold = 0;
            *foldState = 0;
        } else {
            fins->finRot[i].fold += (u16) ((ANGLE_TURN - fins->finRot[i].fold) / fins->finCount[i]);
        }
        break;
    case 2:
        if (--fins->finCount[i] <= 0) {
            fins->finRot[i].fold = EmCephadrome_FinFoldTarget[i];
            *foldState = 0;
        } else {
            fins->finRot[i].fold -= (u16) ((u16) (0x10000 - (EmCephadrome_FinFoldTarget[i] - fins->finRot[i].fold)) / fins->finCount[i]);
        }
        break;
    }
}

typedef struct EmFinKey {
    u16 f;
    u16 v;
} EmFinKey;

extern EmFinKey *EmCephadrome_FinKeys2[];
extern EmFinKey *EmCephadrome_FinKeys1[];
extern s16 EmCephadrome_FinWait1[];
extern s16 EmCephadrome_FinWait2[];
extern s16 EmCephadrome_FinRewait1[];
extern s16 EmCephadrome_FinRewait2[];

/*
 * Returns the index of the first fin key whose frame is at least f; when it
 * reaches the table's end (a key with frame 0 after the first) it returns
 * that key's index with bit 0x80 set.
 */
static inline u8 EmCephadrome_FinFindKey(EmFinKey *keys, u16 frame)
{
    s32 idx = 0;

    for (;;) {
        if (!(idx != 0) == 0 && keys->f == 0) {
            idx = (u8) (idx | 0x80);
            break;
        }
        if (keys->f < frame) {
            idx = (u8) (idx + 1);
            keys++;
        } else {
            break;
        }
    }
    return idx;
}

#define FIN_S16(base, j) (*(s16 *) ((u8 *) (base) + (j)))

/* j: the first argument (the caller passes em) reused as fin i's byte
   offset into the s16 arrays; only this gives the game's registers. */
static void EmCephadrome_UpdateFinWave(s32 j, EmWork *work, s32 i)
{
    EmCephadromeWork *fins = (EmCephadromeWork *) work;
    s16 keyFrame;
    EmFinKey *keys;
    u8 keyIdx;
    u8 *waveState;

    j = i * 2;
    switch (*(waveState = (u8 *) fins->finState + j)) {
    case 0:
        if (fins->finMode == 2) {
            (*waveState)++;
            FIN_S16(fins->finWait, j) = FIN_S16(EmCephadrome_FinWait2, j);
        } else if (fins->finMode == 1) {
            (*waveState)++;
            FIN_S16(fins->finWait, j) = FIN_S16(EmCephadrome_FinWait1, j);
        }
        break;
    case 1:
        if (--FIN_S16(fins->finWait, j) <= 0) {
            if (fins->finMode == 2) {
                *waveState = 3;
            } else {
                *waveState = 2;
            }
            FIN_S16(fins->finWait, j) = 0;
            FIN_S16(fins->finFrame, j) = 0;
        }
        break;
    case 2:
        keyFrame = FIN_S16(fins->finFrame, j)++;
        keys = EmCephadrome_FinKeys2[i];
        keyIdx = EmCephadrome_FinFindKey(keys, keyFrame);
        if (keyIdx & 0x80) {
            if (fins->finMode == 2) {
                *waveState = 1;
                FIN_S16(fins->finWait, j) = FIN_S16(EmCephadrome_FinRewait2, j);
            } else if (fins->finMode == 1) {
                *waveState = 1;
                FIN_S16(fins->finWait, j) = FIN_S16(EmCephadrome_FinRewait1, j);
            } else {
                *waveState = 0;
            }
            fins->finRot[i].wave = keys[keyIdx & 0x7F].v;
        } else {
            fins->finRot[i].wave += keys[keyIdx].v;
        }
        break;
    case 3:
        keyFrame = FIN_S16(fins->finFrame, j)++;
        keys = EmCephadrome_FinKeys1[i];
        keyIdx = EmCephadrome_FinFindKey(keys, keyFrame);
        if (keyIdx & 0x80) {
            if (fins->finMode == 2) {
                *waveState = 1;
                FIN_S16(fins->finWait, j) = FIN_S16(EmCephadrome_FinRewait2, j);
            } else if (fins->finMode == 1) {
                *waveState = 1;
                FIN_S16(fins->finWait, j) = FIN_S16(EmCephadrome_FinRewait1, j);
            } else {
                *waveState = 0;
            }
            fins->finRot[i].wave = keys[keyIdx & 0x7F].v;
        } else {
            fins->finRot[i].wave += keys[keyIdx].v;
        }
        break;
    }
}

/*
 * Runs the wave and the fold of the four fins (read by Em_AdjustKindBones;
 * see the NOTE on its offsets).
 */
void EmCephadrome_UpdateFins(Em *em, EmWork *work)
{
    s32 i;

    for (i = 0; i < 4; i++) {
        EmCephadrome_UpdateFinWave((s32) em, work, i);
        EmCephadrome_UpdateFinFold(em, work, i);
    }
}

/*
 * Runs EmCephadrome_MotionEffects and EmCephadrome_UpdateFins once the work
 * area's fxStep is set (the first frame only sets it); slot 3 of
 * EmCephadrome_Callbacks.
 */
void EmCephadrome_UpdateMotionEffects(Em *em)
{
    EmCephadromeWork *work = (EmCephadromeWork *)&em->work;

    switch (work->fxStep) {
    case 0:
        work->fxStep++;
        break;
    case 1:
        EmCephadrome_MotionEffects(em, (EmWork *) work);
        EmCephadrome_UpdateFins(em, (EmWork *) work);
        break;
    }
}

/*
 * Sets the fin mode: 0 still, 1 waving, 2 waving folded, 3 folded; mode 1
 * becomes 2 at a tenth of hpMax or less, mode 3 becomes 0 above it (the
 * fins fold only when weak; for Plesioth the threshold is a fifth).
 */
void EmCephadrome_SetFinMode(Em *em, EmWork *work, s32 mode)
{
    EmCephadromeWork *w = (EmCephadromeWork *)work;

    w->finMode = mode;
    if (w->finMode == 1) {
        if (em->hp <= (s16) (0.1f * em->hpMax)) {
            w->finMode = 2;
        }
    }
    if (w->finMode == 3) {
        if (em->hp > (s16) (0.1f * em->hpMax)) {
            w->finMode = 0;
        }
    }
}

/*
 * Creates the ground effect of kind 20 type 11 (DustCloud_Create) on
 * frame 16 of CEPH_MOT_TOUCH_DOWN (as EmPlesioth_SpawnLandingDust).
 */
void EmCephadrome_SpawnLandingDust(Em *em)
{
    DustCloud_Create((Em *) em, 11, 0, 1.0f);
}

/*
 * While the monster is in the hunters' area and no deeper than 100 below
 * surfaceY less surfaceDepth, every 4th frame spawns SparkFx type 2 at the
 * sand surface above it, scaled by scale times scaleX (no sound): the wake
 * of the states that swim under the sand (as EmPlesioth_SpawnWakeEvery4).
 */
void EmCephadrome_SpawnWakeEvery4(Em *em, f32 scale)
{
    f32 pos[3];
    f32 y;

    if ((gScene.area == em->area) && !(gScene.frame & 3)) {
        y = em->surfaceY;
        if (!(em->posY < ((y - *(f32 *)&em->surfaceDepth) - 100.0f))) {
            Vec_Set(pos, em->posX, y, em->posZ);
            SparkFx_Spawn(pos, 2, 2, scale * em->scaleX);
        }
    }
}

/*
 * In the hunters' area spawns SparkFx type 5 at the sand surface above the
 * monster (scale times scaleX) with CEPH_SE_SPLASH: the burst where it breaks
 * the surface, leaping out or diving back in (as
 * EmPlesioth_SpawnSurfaceSplash).
 */
void EmCephadrome_SpawnSurfaceSplash(Em *em, f32 scale)
{
    f32 pos[3];

    if (gScene.area == em->area) {
        Vec_Set(pos, em->posX, em->surfaceY, em->posZ);
        SparkFx_Spawn(pos, 5, 2, scale * em->scaleX);
        Em_PlaySeAtBone(em, CEPH_SE_SPLASH, 6);
    }
}

/*
 * Creates attack row `kind`: CephadromeAttack_Create for
 * EM_KIND_CEPHADROME, CephalosAttack_Create for the Cephalos.
 */
void EmCephadrome_CreateAttack(Em *em, s32 kind)
{
    if (em->type == EM_KIND_CEPHADROME) {
        CephadromeAttack_Create(em, kind);
        return;
    }
    CephalosAttack_Create(em, kind);
}

/*
 * Starts pad vibration 3 (Vib_StartLocal) for the local player when active,
 * in the monster's area and within 1000 of it.
 */
void EmCephadrome_VibrateNearby(Em *em)
{
    Player *player;
    s8 i;

    for (player = gPlayers, i = 0; i < gScene.playerCount; i++, player++) {
        if (i == gScene.localPlayer && player->active && Obj_SameArea(em, player) && player->active && Vec_Distance((f32 *) player->pos, (f32 *) &em->posX) <= 1000.0f) {
            Vib_StartLocal(player, 3);
        }
    }
}

/* Slot 2 of EmCephadrome_Callbacks: does nothing (as EmPlesioth_TableNop). */
void EmCephadrome_TableNop(void)
{
}

/*
 * Enters idle state `state`: motion speed 1; begins status 2 in 5 and 14,
 * status 1 in 16 and 41; ends status 2 in 6 and 15, status 1 in 17 and 42,
 * status 4 in 21, status 5 in 23; 18 resets the notice (Em_ResetNotice), 19
 * refills need 3, 22 clears holdTimer2, 40 starts goal action 10000; stance
 * ground, then Em_SetStateSync.
 */
void EmCephadrome_SetIdleState(Em *em, u16 state, u8 mode)
{
    em->motionSpeed = 1.0f;
    switch (state) {
    case CEPH_IDLE:
        break;
    case CEPH_IDLE_STATUS2_COLLAPSE:
    case CEPH_IDLE_STATUS2_COLLAPSE_BEACHED:
        Em_BeginStatus2(em);
        break;
    case CEPH_IDLE_STATUS1_COLLAPSE_BEACHED:
        Em_BeginStatus1(em);
        break;
    case CEPH_IDLE_REST:
        Em_ResetNotice(em);
        break;
    case CEPH_IDLE_STATUS2_END:
    case CEPH_IDLE_STATUS2_END_BEACHED:
        Em_EndStatus2(em);
        break;
    case CEPH_IDLE_STATUS1_END_BEACHED:
        Em_EndStatus1(em);
        break;
    case CEPH_IDLE_REST_END:
        Em_RefillNeed3(em);
        break;
    case CEPH_IDLE_STATUS4_SHAKE_OFF:
        Em_EndStatus4(em);
        break;
    case CEPH_IDLE_HOLD2_SHAKE_OFF:
        em->holdTimer2 = 0;
        break;
    case CEPH_IDLE_STUN_SHAKE_OFF:
        Em_EndStatus5(em);
        break;
    case CEPH_IDLE_EAT:
        Em_StartGoalAction(em, 10000);
        break;
    case CEPH_IDLE_STATUS1_COLLAPSE:
        Em_BeginStatus1(em);
        break;
    case CEPH_IDLE_STATUS1_END:
        Em_EndStatus1(em);
        break;
    }
    em->stance = EM_STANCE_GROUND;
    Em_SetStateSync(em, 0, state, mode);
}

/*
 * Enters move state `state` on the ground at motion speed 1: CEPH_MOVE_WALK
 * runs as CEPH_MOVE_LIMP below a tenth of hpMax; WALK, RUN and LIMP take
 * speed set 6
 * (Em_SetSpeedScaled 1000, 550); then Em_SetStateSync.
 */
void EmCephadrome_SetMoveState(Em *em, u16 state, u8 mode)
{
    em->stance = EM_STANCE_GROUND;
    em->motionSpeed = 1.0f;
    switch (state) {
    case CEPH_MOVE_TURN:
        break;
    case CEPH_MOVE_WALK:
        if (em->hp < (s16) (em->hpMax * 0.1f)) {
            state = CEPH_MOVE_LIMP;
        }
    case CEPH_MOVE_RUN:
    case CEPH_MOVE_LIMP:
        Em_SetSpeedScaled(em, 6, 1000.0f, 550.0f);
        break;
    }
    Em_SetStateSync(em, 1, state, mode);
}

/*
 * Enters swim state `state` at motion speed 1: ends status 4 in 21, status
 * 2 in 24, status 1 in 25; 4, 5 and 27 take speed set 0 (1000, 500), 27
 * also sets noReact and 5 frames of senseDelay, noHitTimer and noPushTimer;
 * 6 runs as 8; 10 and 11 (rounds of the area points) set roundLegsLeft 4
 * and, for a local monster, the nearest area point as goal: within 45
 * degrees ahead it runs as 11 when the following point is behind, with a
 * timer of 0.06 frames per unit of distance, otherwise as 9 (a remote one
 * only computes its goal).
 */
void EmCephadrome_SetSandState(Em *em, u16 state, u8 mode)
{
    EmCephadromeWork *w = (EmCephadromeWork *)&em->work;
    f32 nextPoint[3];
    s32 yawDiff;

    em->motionSpeed = 1.0f;
    switch (state) {
    case CEPH_SAND_TURN_NO_REACT:
        if (state == CEPH_SAND_DASH_NO_REACT) {
            em->noReact = 1;
            em->senseDelay = 5;
            em->noHitTimer = 5;
            em->noPushTimer = 5;
        }
        break;
    case CEPH_SAND_STATUS4_RECOVER:
        Em_EndStatus4(em);
        break;
    case CEPH_SAND_STATUS2_END:
        Em_EndStatus2(em);
        break;
    case CEPH_SAND_STATUS1_END:
        Em_EndStatus1(em);
        break;
    case CEPH_SAND_FORWARD:
    case CEPH_SAND_DASH:
    case CEPH_SAND_DASH_NO_REACT:
        Em_SetSpeedScaled(em, 0, 1000.0f, 500.0f);
        if (state == CEPH_SAND_DASH_NO_REACT) {
            em->noReact = 1;
            em->senseDelay = 5;
            em->noHitTimer = 5;
            em->noPushTimer = 5;
        }
        break;
    case CEPH_SAND_CURVE:
        state = CEPH_SAND_DIVE;
        break;
    case CEPH_SAND_ROUND_NEXT:
    case CEPH_SAND_ROUND_PREV:
        w->roundLegsLeft = 4;
        if (em->remote == 0) {
            em->goalNo = EmCephadrome_NextAreaPoint(em, nextPoint);
            em->goalKind = 2;
            em->goalArg = 2;
            Em_ComputeGoal(em, em->goal);
            yawDiff = (u16) (Math_AngleXZ(&em->posX, em->goal) - em->rotY);
            if ((0 <= yawDiff && yawDiff <= ANGLE_TURN / 8) || (yawDiff >= (ANGLE_TURN * 7 / 8) && yawDiff <= 0xFFFF)) {
                yawDiff = (u16) (Math_AngleXZ(&em->posX, nextPoint) - em->rotY);
                if (yawDiff >= (ANGLE_TURN / 4) && yawDiff <= (ANGLE_TURN * 3 / 4) && state == CEPH_SAND_ROUND_NEXT) {
                    state = CEPH_SAND_ROUND_PREV;
                }
                em->timer = 1.5f * (Vec_DistanceXZ(&em->posX, em->goal) / 0.5f) / 50.0f;
            } else {
                state = CEPH_SAND_ROUND_TURN;
            }
        } else {
            Em_ComputeGoal2(em, em->goal);
        }
        break;
    case CEPH_SAND_IDLE:
    case CEPH_SAND_IDLE_B:
    case CEPH_SAND_CIRCLE_RIGHT_B:
    case CEPH_SAND_TURN:
    case CEPH_SAND_RISE:
    case CEPH_SAND_DIVE:
    case CEPH_SAND_ROUND_TURN:
    case CEPH_SAND_CIRCLE_RIGHT:
    case CEPH_SAND_CIRCLE_LEFT:
    case CEPH_SAND_LEAP_ASHORE:
    case CEPH_SAND_SURFACE_FLOP:
    case CEPH_SAND_LEAP_IN:
    case CEPH_SAND_LEAP:
    case CEPH_SAND_LEAP_ON_LAND:
    case CEPH_SAND_ROUND_NEXT_LEG:
    case CEPH_SAND_ROUND_PREV_LEG:
    case CEPH_SAND_LEAP_TO_FIXED_POINT:
    case CEPH_SAND_EMPTY:
        break;
    }
    Em_SetStateSync(em, 2, state, mode);
}

/*
 * Enters attack state `state` at motion speed 1; CEPH_ATK_BREATH sets
 * EmCephadromeWork.unk013.
 */
void EmCephadrome_SetAttackState(Em *em, u16 state, u8 mode)
{
    EmCephadromeWork *w = (EmCephadromeWork *)&em->work;

    em->motionSpeed = 1.0f;
    switch (state) {
    case CEPH_ATK_LAND_A:
    case CEPH_ATK_SPIN:
    case CEPH_ATK_SURFACE_BREATH_B:
    case CEPH_ATK_SURFACE_BREATH_C:
    case CEPH_ATK_LEAP:
    case CEPH_ATK_LAND_B:
    case CEPH_ATK_SURFACE_LUNGE:
    case CEPH_ATK_SURFACE_BREATH_A:
        break;
    case CEPH_ATK_BREATH:
        w->unk013 = 1;
        break;
    }
    Em_SetStateSync(em, EM_GROUP_ATTACK, state, mode);
}

/*
 * Moves to (group, state): a local monster first targets a player in range
 * (a remote one ignores sync modes 3 and 4), loads reaction set 0, then the
 * group's Set function; inline for EM_GROUP_REACT (begins status 4 in
 * CEPH_REACT_STATUS4_LYING and CEPH_REACT_STATUS4, ends it in
 * CEPH_REACT_STATUS4_LYING_END, ends all statuses in CEPH_REACT_HOLD2, begins
 * the stun in CEPH_REACT_STUN_A and _B; motion speed 1), EM_GROUP_DEFEAT
 * (CEPH_DEFEAT_REAPPEAR makes the monster appear again when the quest lets
 * it; motion speed kept), EM_GROUP_EVENT and EM_GROUP_VERSUS (motion speed
 * 1). Other groups do nothing.
 */
void EmCephadrome_ChangeState(Em *em, u16 group, u16 state, u8 mode)
{
    if (em->remote == 0) {
        Em_TargetPlayerInRange(em);
    } else if (mode == EM_SYNC_OWNER || mode == EM_SYNC_OWNER_FLAGGED) {
        return;
    }
    Em_LoadReactionSet(em, 0);
    switch (group) {
    case EM_GROUP_IDLE:
        EmCephadrome_SetIdleState(em, state, mode);
        return;
    case EM_GROUP_MOVE:
        EmCephadrome_SetMoveState(em, state, mode);
        return;
    case EM_GROUP_FLY:
        EmCephadrome_SetSandState(em, state, mode);
        return;
    case EM_GROUP_ATTACK:
        EmCephadrome_SetAttackState(em, state, mode);
        return;
    case EM_GROUP_REACT:
        switch (state) {
        case CEPH_REACT_STATUS4_LYING:
        case CEPH_REACT_STATUS4:
            Em_BeginStatus4(em);
            break;
        case CEPH_REACT_STATUS4_LYING_END:
            Em_EndStatus4(em);
            break;
        case CEPH_REACT_HOLD2:
            Em_EndAllStatus(em);
            break;
        case CEPH_REACT_STUN_A:
        case CEPH_REACT_STUN_B:
            Em_BeginStatus5(em);
            break;
        }
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        return;
    case EM_GROUP_DEFEAT:
        if ((state == CEPH_DEFEAT_REAPPEAR) && (QuestEm_CanAppear(em) == 1)) {
            QuestEm_Appear(em);
        }
        Em_SetStateSync(em, group, state, mode);
        return;
    case EM_GROUP_EVENT:
    case EM_GROUP_VERSUS:
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        /* falls through to the return: a `return;` here moves the switch's shared exit */
    default:
        return;
    }
}

/*
 * Starts curve set `kind` of the movement curves: kinds 0-2 the vertical
 * and forward speed curves from frame 0, kind 3 only the vertical one from
 * frame 38; clears the velocity (this file starts only kind 0, from
 * SandCurve).
 */
void EmCephadrome_StartCurves(Em *em, u8 kind)
{
    EmCephadromeWork *w = (EmCephadromeWork *)&em->work;

    w->curveNone = 0;
    w->curveVelY = 0;
    w->curveSpeed = 0;
    w->curveKind = kind;
    switch (w->curveKind) {
    case 0:
        w->curveVelY = 1;
        w->curveSpeed = 1;
        w->curveFrame = 0;
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
    case 3:
        w->curveVelY = 1;
        w->curveFrame = 38;
        break;
    }
    Player_ClearVelocity(em);
}

/* The curve step of a set without curves: does nothing and returns 0. */
static s32 EmCephadrome_CurveNone(Em *em, EmWork *work)
{
    return 0;
}

/*
 * The vertical-speed curve (EmCephadrome_VelYCurves[curveKind]): sets velY
 * from the key pair around curveFrame; at the table's end (a key with t 0)
 * eases velY down to -50 and returns 2, else 0.
 */
static u8 EmCephadrome_CurveVelY(Em *em, EmWork *work)
{
    EmCurveKey *key;
    f32 frame;
    f32 keyT;
    s32 i;
    EmCurveKey *keys;
    u8 endBits;

    keys = EmCephadrome_VelYCurves[((EmCephadromeWork *)work)->curveKind];
    endBits = 0;
    if (keys == NULL) {
        return endBits;
    }
    if (em->motion[0].blend != 0) {
        frame = 0.0f;
    } else {
        frame = ((EmCephadromeWork *)work)->curveFrame;
    }
    if (frame != 0.0f && frame != 1.0f) {
        i = 1;
        while (i != 0) {
            key = &keys[i];
            keyT = key->t;
            if (frame > keyT && keyT != 0.0f) {
                i++;
            } else if (keyT == 0.0f) {
                endBits = 2;
                if (em->velY > -50.0f) {
                    em->velY -= 1.0f;
                }
                break;
            } else {
                i = 0;
                em->velY = (key->v - key[-1].v) / ((keyT - key[-1].t) / em->motion[0].step);
            }
        }
    }
    return endBits;
}

/*
 * The forward-speed curve (EmCephadrome_SpeedCurves[curveKind]): sets velZ
 * from the key pair around curveFrame; returns 4 at the table's end, else
 * 0.
 */
static u8 EmCephadrome_CurveSpeed(Em *em, EmWork *work)
{
    EmCurveKey *key;
    f32 frame;
    f32 keyT;
    s32 i;
    EmCurveKey *keys;
    u8 endBits;

    keys = EmCephadrome_SpeedCurves[((EmCephadromeWork *)work)->curveKind];
    endBits = 0;
    if (keys == NULL) {
        return endBits;
    }
    if (em->motion[0].blend != 0) {
        frame = 0.0f;
    } else {
        frame = ((EmCephadromeWork *)work)->curveFrame;
    }
    if (frame != 0.0f && frame != 1.0f) {
        i = 1;
        while (i != 0) {
            key = &keys[i];
            keyT = key->t;
            if (frame > keyT && keyT != 0.0f) {
                i++;
            } else if (keyT == 0.0f) {
                endBits = 4;
                break;
            } else {
                i = 0;
                em->velZ = (key->v - key[-1].v) / ((keyT - key[-1].t) / em->motion[0].step);
            }
        }
    }
    return endBits;
}

/*
 * Runs the curves whose flags are set, advances curveFrame by the motion's
 * step, and in the air or under the sand moves the monster along its yaw
 * (Em_MoveRotatedAccel). Returns the curves' end bits (2 vertical, 4
 * forward).
 */
u8 EmCephadrome_UpdateCurves(Em *em)
{
    EmCephadromeWork *w = (EmCephadromeWork *)&em->work;
    u8 endBits = 0;

    if (w->curveNone != 0) {
        endBits = EmCephadrome_CurveNone(em, (EmWork *) w);
    }
    if (w->curveVelY != 0) {
        endBits |= EmCephadrome_CurveVelY(em, (EmWork *) w);
    }
    if (w->curveSpeed != 0) {
        endBits |= EmCephadrome_CurveSpeed(em, (EmWork *) w);
    }
    w->curveFrame += (s16) em->motion[0].step;
    if (em->stance == EM_STANCE_AIR || em->stance == EM_STANCE_UNDER) {
        w->moveAngBX = 0;
        w->moveAngBY = em->rotY;
        w->moveAngBZ = 0;
        Em_MoveRotatedAccel(em, &w->moveAngBX);
    }
    return endBits;
}

/*
 * Of the area's points (em->tables[2]), finds the one nearest the monster
 * and writes the following one (index + 1, wrapping) to out. Returns the
 * nearest one's index; without a table writes (1000, 0, 1000) and returns
 * 0.
 */
u8 EmCephadrome_NextAreaPoint(Em *em, f32 *out)
{
    EmAreaPoints *entry;
    f32 *point;
    u32 i;
    u8 nearest;
    f32 bestDist;
    f32 dist;
    u8 next;
    f32 (*sel)[3];

    nearest = 0;
    entry = (EmAreaPoints *)Em_FindAreaEntry(em, (struct EmIdEntry *)(((EmAreaPoints **) em->tables)[2]), em->area);
    if (entry == NULL || (point = entry->pts) == NULL) {
        out[0] = 1000.0f;
        out[1] = 0.0f;
        out[2] = 1000.0f;
        nearest = 0;
    } else {
        for (i = 0; i < entry->n; i++, point += 3) {
            dist = Vec_DistanceXZ(&em->posX, point);
            if (nearest == i) {
                bestDist = dist;
            } else if (bestDist > dist) {
                bestDist = dist;
                nearest = i;
            }
        }
        next = nearest + 1;
        if (next >= entry->n) {
            next -= entry->n;
        }
        sel = (f32 (*)[3]) entry->pts;
        out[0] = sel[next][0];
        out[1] = sel[next][1];
        out[2] = sel[next][2];
    }
    return nearest;
}

/*
 * Returns 1 when the monster is in the hunters' area
 * (Player_InCurrentArea), under the sand (EM_STANCE_UNDER) and the floor
 * under it is not sand (surfaceKind 0), else 0; Update then sends it to
 * land idle (EmCephadrome_EndGroundState).
 */
s32 EmCephadrome_IsUnderOffSand(Em *em)
{
    if (!(Player_InCurrentArea(em) & 0xFF)) {
        return 0;
    }
    if ((em->surfaceKind == 0) && (em->stance == EM_STANCE_UNDER)) {
        return 1;
    }
    return 0;
}

/*
 * Steps curGoalNo to the next area point (dir 0) or the previous one (dir
 * 1), wrapping by the area's point count (0 without a table). One time in 8
 * (stateRand) the Cephalos first jumps two points on in AREA_DESERT_1, _5
 * and _6, and the Cephadrome crosses to a fixed other point in
 * AREA_DESERT_1 and _5 instead of stepping. Called by the two round swims
 * (SandRoundNext forward, SandRoundPrev back).
 */
void EmCephadrome_StepAreaPoint(Em *em, u8 dir)
{
    EmAreaPoints *entry;

    if (em->type == EM_KIND_CEPHALOS) {
        switch (Area_DayId(em->area)) {
        case AREA_DESERT_1:
        case AREA_DESERT_5:
        case AREA_DESERT_6:
            if ((em->stateRand & 7) == 0) {
                em->curGoalNo += 2;
                em->curGoalNo &= 3;
            }
            break;
        case AREA_DESERT_2:
            break;
        }
    } else {
        switch (Area_DayId(em->area)) {
        case AREA_DESERT_1:
            if ((em->stateRand & 7) == 0) {
                switch (em->curGoalNo) {
                case 0:
                    em->curGoalNo = 3;
                    return;
                case 1:
                    em->curGoalNo = 4;
                    return;
                case 3:
                    em->curGoalNo = 0;
                    return;
                case 4:
                    em->curGoalNo = 1;
                    return;
                }
            }
            break;
        case AREA_DESERT_5:
            if ((em->stateRand & 7) == 0) {
                switch (em->curGoalNo) {
                case 0:
                    em->curGoalNo = 2;
                    return;
                case 1:
                    em->curGoalNo = 4;
                    return;
                case 2:
                    em->curGoalNo = 0;
                    return;
                case 4:
                    em->curGoalNo = 1;
                    return;
                }
            }
            break;
        case AREA_DESERT_2:
            break;
        }
    }
    if (dir == 0) {
        em->curGoalNo++;
    } else {
        em->curGoalNo--;
    }
    entry = (EmAreaPoints *)Em_FindAreaEntry(em, (struct EmIdEntry *)(((EmAreaPoints **) em->tables)[2]), em->area);
    if (entry != NULL) {
        while (em->curGoalNo >= entry->n) {
            em->curGoalNo -= entry->n;
        }
    } else {
        em->curGoalNo = 0;
    }
}

/* The file's initialised data, in the game's order */
extern u32 EmCephadrome_VelYCurves_3[];
EmTurnParams EmCephadrome_TurnParams[3] = {
#include "game/008C8C10.inc"
};
EmFinKey EmCephadrome_FinKeys2_0[8] = {
#include "game/008C8C70.inc"
};
EmFinKey EmCephadrome_FinKeys2_1[8] = {
#include "game/008C8C90.inc"
};
EmFinKey EmCephadrome_FinKeys2_2[8] = {
#include "game/008C8CB0.inc"
};
EmFinKey EmCephadrome_FinKeys2_3[8] = {
#include "game/008C8CD0.inc"
};
EmFinKey *EmCephadrome_FinKeys2[4] = {
    EmCephadrome_FinKeys2_0, EmCephadrome_FinKeys2_1, EmCephadrome_FinKeys2_2, EmCephadrome_FinKeys2_3,
};
EmFinKey EmCephadrome_FinKeys1_0[12] = {
#include "game/008C8D00.inc"
};
EmFinKey EmCephadrome_FinKeys1_1[12] = {
#include "game/008C8D30.inc"
};
EmFinKey EmCephadrome_FinKeys1_2[8] = {
#include "game/008C8D60.inc"
};
EmFinKey EmCephadrome_FinKeys1_3[8] = {
#include "game/008C8D80.inc"
};
EmFinKey *EmCephadrome_FinKeys1[4] = {
    EmCephadrome_FinKeys1_0, EmCephadrome_FinKeys1_1, EmCephadrome_FinKeys1_2, EmCephadrome_FinKeys1_3,
};
s16 EmCephadrome_FinWait1[4] = {
#include "game/008C8DB0.inc"
};
s16 EmCephadrome_FinWait2[4] = {
#include "game/008C8DB8.inc"
};
s16 EmCephadrome_FinRewait1[4] = {
#include "game/008C8DC0.inc"
};
s16 EmCephadrome_FinRewait2[4] = {
#include "game/008C8DC8.inc"
};
u16 EmCephadrome_FinFoldTarget[4] = {
#include "game/008C8DD0.inc"
};
void *EmCephadrome_Callbacks[8] = {
    EmCephadrome_SpawnEffects, EmCephadrome_SpawnEffects, EmCephadrome_TableNop, EmCephadrome_UpdateMotionEffects,
    GmEm_ReturnZero, EmCephadrome_Reinit, NULL, NULL,
};
EmCurveKey EmCephadrome_VelYCurves_0[16] = {
#include "game/008C8E00.inc"
};
EmCurveKey EmCephadrome_VelYCurves_1[16] = {
#include "game/008C8E80.inc"
};
EmCurveKey EmCephadrome_VelYCurves_2[12] = {
#include "game/008C8F00.inc"
};
EmCurveKey *EmCephadrome_VelYCurves[4] = {
    EmCephadrome_VelYCurves_0, EmCephadrome_VelYCurves_1, EmCephadrome_VelYCurves_2, (EmCurveKey *)EmCephadrome_VelYCurves_3,
};
EmCurveKey EmCephadrome_SpeedCurves_0[16] = {
#include "game/008C8F70.inc"
};
EmCurveKey EmCephadrome_SpeedCurves_1[16] = {
#include "game/008C8FF0.inc"
};
EmCurveKey EmCephadrome_SpeedCurves_2[11] = {
#include "game/008C9070.inc"
};
/* The curves EmCephadrome_CurveSpeed picks from. */
EmCurveKey *EmCephadrome_SpeedCurves[3] = {EmCephadrome_SpeedCurves_0, EmCephadrome_SpeedCurves_1, EmCephadrome_SpeedCurves_2};
