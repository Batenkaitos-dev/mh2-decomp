/*
 * The Yian Garuga family (EM_KIND_YIAN_GARUGA): the code from the family's
 * first function, EmGaruga_MotionSounds, to the next family's.
 *
 * Yian Garuga follows the Rathalos family's template: a state machine
 * (include/em_garuga.h names the states and motions, the bones
 * (GARUGA_BONE_*) and the sounds the code shows (GARUGA_SE_*); include/em.h
 * the groups). EmGaruga_Update reads the hits (Em_UpdateDamage) and picks
 * reactions, then EmGaruga_RunState calls the group's dispatcher
 * (EmGaruga_RunIdleState ... RunVersusState), which calls one function per
 * state with the monster and its work area (EmWork; the flying families' view
 * is EmFlyerWork). A state function switches on Em.step; EmGaruga_ChangeState
 * starts the next state through the group's Set function, and
 * EmGaruga_EndState / EndAirState hand the choice back to the monster script.
 * Its own moves are the pecks, the back flip, the hops and the tail spin.
 * Flights move along curves (EmGaruga_StartCurves, UpdateCurves) and
 * Em_UpdateFlight; EmGaruga_MotionEffects adds each motion's attacks, effects
 * and camera shakes, and EmGaruga_MotionSounds its sounds.
 */
#include "common.h"
#include "scene.h"
#include "quest.h"
#include "em.h"
#include "shadow.h"
#include "em_garuga.h"
#include "objattach.h"
#include "player.h"
#include "area.h"
#include "capsule.h"
#include "save.h"
#include "sound.h"
#include "cam_shake.h"
#include "dust_flash_effects.h"
#include "engine_math.h"
#include "gm_em.h"
#include "gm_load.h"
#include "hit_effects.h"
#include "part_effects.h"
#include "rand.h"
#include "stage_data.h"
#include "vec_math.h"
#include "em_script.h"
#include "hud.h"
/* prototypes of callees */

/* prototypes */
u16 Area_IdForCurrentTime(u16);
void Em_EndAllStatus(Em *em);
void EmRangedAttack_CreateAtBone(Em *, s32, s32, s32, s32, s32);
/* unit's own declaration: declared here as the callers use it; with the definition's void (Em *em, s32 group, s32 state, u8 mode), the calls in EmGaruga_ChangeState, EmGaruga_SetIdleState and others compile differently */
void Em_SetStateSync(Em *em, u16 group, u16 state, u8 mode);
/* unit's own declaration: declared here as the callers use it; with the definition's s32 (Em *em, s16 n, s16 max, s16 interval), the calls in EmGaruga_Rest and EmGaruga_Status2Regen compile differently */
s32 Em_RegenHp(Em *em, s32 n, s32 max, s32 interval);
/* unit's own declaration: declared here as the callers use it; with the definition's void (Em *em, u8 k, s16 v, s16 t, f32 y, f32 scale), the calls in EmGaruga_Stun and EmGaruga_StunDowned compile differently */
void StunStars_CreateEm(Em *, s32, s32, s32, f32, f32);
s32 EmMotion_PassedFrameScaled(void *p, f32 frame, s32 layer);
extern EmFlyParam EmGaruga_FlyParam;
extern EmFlyParam EmGaruga_FlyParam2;
extern EmTurnParams EmGaruga_TurnParams[2];
/* A position copied whole (EmGaruga_CarveThrowPt): the original copies it as one struct. */
extern Vec3f EmGaruga_CarveThrowPt __attribute__((aligned(8)));
extern EmCurveKey *EmGaruga_MoveVelYCurves[];
void Em_SpawnLyingPuffs(Em *em, s32 bone, f32 *pos, f32 scale);
struct EmIdEntry;
f32 *Em_AreaEntryValue(Em *em, struct EmIdEntry *entry);
void EmTail_Create(Em *);
/* end of prototypes */

void EmGaruga_MotionSounds(Em *em, EmWork *work)
{
    s16 motion;

    motion = work->sndMotion;
    switch (motion) {
    case EM_MOTION_ID(0, GARUGA_MOT_WALK):
        Em_PlaySeAtBoneOnFrame(em, 52, GARUGA_SE_STEP, GARUGA_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 116, GARUGA_SE_STEP, GARUGA_BONE_STEP_B);
        break;
    case EM_MOTION_ID(0, 4):
        Em_PlaySeAtBoneOnFrame(em, 40, 6, GARUGA_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 76, 6, GARUGA_BONE_STEP_B);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_TURN_RIGHT):
        Em_PlaySeAtBoneOnFrame(em, 25, GARUGA_SE_STEP, GARUGA_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 56, GARUGA_SE_STEP, GARUGA_BONE_STEP_B);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_TURN_LEFT):
        Em_PlaySeAtBoneOnFrame(em, 25, GARUGA_SE_STEP, GARUGA_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 56, GARUGA_SE_STEP, GARUGA_BONE_STEP_A);
        break;
    case EM_MOTION_ID(0, 7):
        Em_PlaySeAtBoneOnFrame(em, 6, 32, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 14, GARUGA_SE_STEP, GARUGA_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 38, GARUGA_SE_STEP, GARUGA_BONE_STEP_A);
        break;
    case EM_MOTION_ID(0, 8):
        Em_PlaySeAtBoneOnFrame(em, 6, 32, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 14, GARUGA_SE_STEP, GARUGA_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 38, GARUGA_SE_STEP, GARUGA_BONE_STEP_B);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_LIMP):
        Em_PlaySeTypeAtBoneOnFrame(em, 6, 89, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 6, 4, GARUGA_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 76, 0, GARUGA_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 28, 18, GARUGA_BONE_STEP_A);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_DESCEND):
        Em_PlaySeAtBoneOnFrame(em, 14, 11, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 18, 11, GARUGA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 70, 11, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 66, 11, GARUGA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 122, 11, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 126, 11, GARUGA_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_GLIDE):
        Em_PlaySeAtBoneOnFrame(em, 6, 12, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 4, 12, GARUGA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 64, 12, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 60, 12, GARUGA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 4, 26, GARUGA_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame(em, 6, 25, GARUGA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 10, 25, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 36, 26, GARUGA_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame(em, 38, 25, GARUGA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 40, 25, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 70, 26, GARUGA_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame(em, 72, 25, GARUGA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 76, 25, GARUGA_BONE_WING_A);
        break;
    case EM_MOTION_ID(0, 13):
        Em_PlaySeAtBoneOnFrame(em, 14, 15, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 18, 15, GARUGA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 44, 15, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 40, 15, GARUGA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 66, 15, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 70, 15, GARUGA_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_FLY):
    case EM_MOTION_ID(0, 85):
    case EM_MOTION_ID(0, 86):
    case EM_MOTION_ID(0, 88):
    case EM_MOTION_ID(0, 89):
        Em_PlaySeAtBoneOnFrame(em, 10, 26, GARUGA_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame(em, 14, 25, GARUGA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 18, 25, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 42, 25, GARUGA_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame(em, 46, 25, GARUGA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 50, 25, GARUGA_BONE_WING_A);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_HOVER):
        Em_PlaySeAtBoneOnFrame(em, 4, 12, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 8, 12, GARUGA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 58, 11, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 62, 11, GARUGA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 142, 11, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 138, 11, GARUGA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 212, 11, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 216, 11, GARUGA_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_RUN):
        Em_PlaySeAtBoneOnFrame(em, 10, 18, GARUGA_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 28, 6, GARUGA_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 62, 6, GARUGA_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 4, 41, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 46, 41, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 82, 41, GARUGA_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_TAKE_OFF):
        Em_PlaySeAtBoneOnFrame(em, 26, 13, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 30, 13, GARUGA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 82, 11, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 86, 11, GARUGA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 136, 11, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 140, 11, GARUGA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 182, 11, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 186, 11, GARUGA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 42, 7, GARUGA_BONE_STEP_B);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_TOUCH_DOWN):
        Em_PlaySeAtBoneOnFrame(em, 2, 11, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 6, 11, GARUGA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 4, GARUGA_SE_STEP, GARUGA_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 12, 9, GARUGA_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 98, 3, GARUGA_BONE_STEP_A);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_IDLE_ACTION_I):
        Em_PlaySeAtBoneOnFrame(em, 30, 19, GARUGA_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 2, 32, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 4, 14, GARUGA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 8, 14, GARUGA_BONE_WING_A);
        break;
    case EM_MOTION_ID(0, 31):
        Em_PlaySeAtBoneOnFrame(em, 2, 45, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 182, 45, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 364, 45, GARUGA_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, 32):
        Em_PlaySeAtBoneOnFrame(em, 4, 47, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 70, 0, GARUGA_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 142, 3, GARUGA_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 4, 23, GARUGA_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 4, 19, GARUGA_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, 33):
        Em_PlaySeAtBoneOnFrame(em, 4, 47, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 70, 45, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 126, 3, GARUGA_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 182, 3, GARUGA_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 56, 22, GARUGA_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_LIE):
        Em_PlaySeTypeAtBoneOnFrame(em, 36, 87, GARUGA_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, 35):
        Em_PlaySeAtBoneOnFrame(em, 50, 2, GARUGA_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 58, 6, GARUGA_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 84, GARUGA_SE_STEP, GARUGA_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 104, 4, GARUGA_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 124, 17, GARUGA_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 204, 6, GARUGA_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 210, 2, GARUGA_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 254, 18, GARUGA_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 288, 2, GARUGA_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 32, 15, GARUGA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 36, 15, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 128, 15, GARUGA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 132, 15, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 290, 14, GARUGA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 294, 14, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 2, 21, GARUGA_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 2, 35, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 48, 33, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 128, 34, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 184, 36, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 288, 34, GARUGA_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_STARE):
        Em_PlaySeAtBoneOnFrame(em, 36, 36, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 70, 34, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 20, 0, GARUGA_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 40, 0, GARUGA_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 60, 21, GARUGA_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame(em, 60, 12, GARUGA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 12, 12, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 166, 0, GARUGA_BONE_STEP_B);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_GET_UP):
        Em_PlaySeTypeAtBoneOnFrame(em, 30, 72, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 12, 12, GARUGA_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 28, 2, GARUGA_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 46, 3, GARUGA_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 72, GARUGA_SE_STEP, GARUGA_BONE_STEP_B);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_COLLAPSE):
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 87, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 72, 22, GARUGA_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 164, 9, GARUGA_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 144, 3, GARUGA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 136, 14, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 178, 4, GARUGA_BONE_BREATH);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_LIE_DOWN):
        Em_PlaySeTypeAtBoneOnFrame(em, 2, 87, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 2, 23, GARUGA_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 64, 3, GARUGA_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 102, 16, GARUGA_BONE_STEP_B);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_ROAR):
        Em_PlaySeTypeAtBoneInFrames(em, 52, 200, GARUGA_SE_ROAR, GARUGA_BONE_HEAD, 6, 0);
        Em_PlaySeAtBoneOnFrame(em, 150, 32, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 2, 22, GARUGA_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 22, 13, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 26, 13, GARUGA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 140, 12, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 144, 12, GARUGA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 34, 5, GARUGA_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 34, 5, GARUGA_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 134, GARUGA_SE_STEP, GARUGA_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 166, GARUGA_SE_STEP, GARUGA_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 194, 0, GARUGA_BONE_STEP_B);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_FLIP):
        Em_PlaySeAtBoneOnFrame(em, 30, GARUGA_SE_STEP, GARUGA_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 52, GARUGA_SE_STEP, GARUGA_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 4, 22, GARUGA_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 70, 13, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 74, 13, GARUGA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 106, 20, GARUGA_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 106, 19, GARUGA_BONE_ROOT);
        Em_PlaySeTypeAtBoneOnFrame(em, 84, 79, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 160, 15, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 164, 15, GARUGA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 210, 12, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 214, 12, GARUGA_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_TAIL_SPIN):
        Em_PlaySeTypeAtBoneOnFrame(em, 8, 75, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 14, 15, GARUGA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 30, 20, GARUGA_BONE_TAIL_B);
        Em_PlaySeAtBoneOnFrame(em, 42, 0, GARUGA_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 72, GARUGA_SE_STEP, GARUGA_BONE_STEP_B);
        break;
    case EM_MOTION_ID(0, 44):
        Em_PlaySeAtBoneOnFrame(em, 28, 11, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 32, 11, GARUGA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 6, 26, GARUGA_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame(em, 10, 27, GARUGA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 14, 27, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 38, 26, GARUGA_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame(em, 42, 27, GARUGA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 50, 27, GARUGA_BONE_WING_A);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_FIREBALL):
        Em_PlaySeAtBoneOnFrame(em, 2, 18, GARUGA_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 80, GARUGA_SE_FIREBALL, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 20, 54, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 74, 41, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 220, 32, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 76, 13, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 72, 13, GARUGA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 180, 19, GARUGA_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 260, 3, GARUGA_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 288, 3, GARUGA_BONE_STEP_B);
        break;
    case EM_MOTION_ID(0, 48):
        Em_PlaySeAtBoneOnFrame(em, 2, 34, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 34, 34, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 64, 34, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 114, 32, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 16, GARUGA_SE_STEP, GARUGA_BONE_STEP_A);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_ALERT_IDLE):
        Em_PlaySeAtBoneOnFrame(em, 56, 18, GARUGA_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 88, 6, GARUGA_BONE_STEP_B);
        Em_PlaySeTypeAtBoneOnFrame(em, 150, 94, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 252, 39, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 238, 22, GARUGA_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_BACK_HOP):
        if ((em->stateGroup == EM_GROUP_FLY) && (em->state == GARUGA_FLY_BACK_HOP)) {
            Em_PlaySeAtBoneOnFrame(em, 22, 40, GARUGA_BONE_HEAD);
            Em_PlaySeTypeAtBoneInFrames(em, 72, 178, GARUGA_SE_ROAR, GARUGA_BONE_HEAD, 6, 0);
            Em_PlaySeAtBoneOnFrame(em, 6, 20, GARUGA_BONE_BREATH);
            Em_PlaySeAtBoneOnFrame(em, 22, 0, GARUGA_BONE_STEP_B);
            Em_PlaySeAtBoneOnFrame(em, 58, 15, GARUGA_BONE_WING_A);
            Em_PlaySeAtBoneOnFrame(em, 62, 15, GARUGA_BONE_WING_B);
            Em_PlaySeAtBoneOnFrame(em, 106, 11, GARUGA_BONE_WING_A);
            Em_PlaySeAtBoneOnFrame(em, 110, 11, GARUGA_BONE_WING_B);
            Em_PlaySeAtBoneOnFrame(em, 172, 11, GARUGA_BONE_WING_A);
            Em_PlaySeAtBoneOnFrame(em, 176, 11, GARUGA_BONE_WING_B);
        }
        Em_PlaySeAtBoneOnFrame(em, 22, 40, GARUGA_BONE_HEAD);
        Em_PlaySeTypeAtBoneInFrames(em, 72, 178, 79, GARUGA_BONE_HEAD, 6, 0);
        Em_PlaySeAtBoneOnFrame(em, 6, 20, GARUGA_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame(em, 22, 0, GARUGA_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 58, 15, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 62, 15, GARUGA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 106, 11, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 110, 11, GARUGA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 172, 11, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 176, 11, GARUGA_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_TURN_IN_PLACE):
        Em_PlaySeAtBoneOnFrame(em, 2, 18, GARUGA_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 28, 4, GARUGA_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 56, 4, GARUGA_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 28, 5, GARUGA_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 120, 4, GARUGA_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 264, 4, GARUGA_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 4, 20, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 20, 54, GARUGA_BONE_HEAD);
        Em_PlaySeTypeAtBoneOnFrame(em, 124, 79, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 132, 41, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 10, 12, GARUGA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 16, 12, GARUGA_BONE_WING_A);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_BACK_HOP_LAND):
        Em_PlaySeAtBoneOnFrame(em, 76, 32, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 12, 18, GARUGA_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 46, 8, GARUGA_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 2, 13, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 4, 13, GARUGA_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_FEED):
        Em_PlaySeAtBoneOnFrame(em, 12, 22, GARUGA_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 6, 34, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 104, 3, GARUGA_BONE_STEP_B);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_FLINCH_A):
        Em_PlaySeTypeAtBoneOnFrame(em, 6, 83, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 72, 19, GARUGA_BONE_TAIL_B);
        Em_PlaySeAtBoneOnFrame(em, 48, 6, GARUGA_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 48, 17, GARUGA_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 48, 3, GARUGA_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 116, 3, GARUGA_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 148, 3, GARUGA_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 196, 3, GARUGA_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 52, 12, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 56, 13, GARUGA_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_FLINCH_E):
        Em_PlaySeTypeAtBoneOnFrame(em, 6, 83, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 10, 18, GARUGA_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 38, 6, GARUGA_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 78, 4, GARUGA_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 172, 3, GARUGA_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 110, 3, GARUGA_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 90, 21, GARUGA_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_SHAKE_OFF):
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 79, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 86, 31, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 16, 3, GARUGA_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 4, 13, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 8, 13, GARUGA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 98, 19, GARUGA_BONE_TAIL_B);
        Em_PlaySeAtBoneOnFrame(em, 126, 3, GARUGA_BONE_STEP_B);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_FLINCH_C):
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 82, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 4, 19, GARUGA_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 76, 31, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 14, 0, GARUGA_BONE_STEP_B);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_FLINCH_D):
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 82, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 4, 19, GARUGA_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 118, 32, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 122, 0, GARUGA_BONE_STEP_B);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_PART_BREAK):
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 84, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 168, 42, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 4, 39, GARUGA_BONE_TAIL_D);
        Em_PlaySeAtBoneOnFrame(em, 252, 32, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 304, 39, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 62, 4, GARUGA_BONE_TAIL_D);
        Em_PlaySeAtBoneOnFrame(em, 18, 18, GARUGA_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 44, 6, GARUGA_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 58, 16, GARUGA_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 210, 3, GARUGA_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 322, 3, GARUGA_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 62, 9, GARUGA_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 74, 9, GARUGA_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 100, 18, GARUGA_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 162, 18, GARUGA_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 14, 12, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 18, 12, GARUGA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 286, 13, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 290, 13, GARUGA_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_FLINCH_B):
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 82, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 4, 19, GARUGA_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, 67):
        Em_PlaySeAtBoneOnFrame(em, 70, 45, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 70, 23, GARUGA_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_DIE):
        Em_PlaySeAtBoneOnFrame(em, 28, 2, GARUGA_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 48, 4, GARUGA_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 220, 16, GARUGA_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 30, 22, GARUGA_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 264, 9, GARUGA_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 264, 7, GARUGA_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 276, 23, GARUGA_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 384, 22, GARUGA_BONE_ROOT);
        Em_PlaySeTypeAtBoneOnFrame(em, 24, 82, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 140, 40, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 180, 44, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 314, 47, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 402, 43, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 372, 15, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 374, 14, GARUGA_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_KNOCK_DOWN):
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 82, GARUGA_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 4, 19, GARUGA_BONE_TAIL_B);
        Em_PlaySeAtBoneOnFrame(em, 24, 9, GARUGA_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 24, 17, GARUGA_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_DOWN):
    case EM_MOTION_ID(0, GARUGA_MOT_KNOCK_DOWN_B_LIE):
        Em_PlaySeAtBoneOnFrame(em, 8, 22, GARUGA_BONE_ROOT);
        Em_PlaySeTypeAtBoneOnFrame(em, 58, 82, GARUGA_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_DOWN_GET_UP):
        Em_PlaySeAtBoneOnFrame(em, 4, 22, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 30, 32, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 48, GARUGA_SE_STEP, GARUGA_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 70, GARUGA_SE_STEP, GARUGA_BONE_STEP_B);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_KNOCK_DOWN_B_GET_UP):
        Em_PlaySeAtBoneOnFrame(em, 4, 22, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 30, 32, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 48, GARUGA_SE_STEP, GARUGA_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 70, GARUGA_SE_STEP, GARUGA_BONE_STEP_A);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_KNOCK_DOWN_B):
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 82, GARUGA_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 4, 19, GARUGA_BONE_TAIL_B);
        Em_PlaySeAtBoneOnFrame(em, 24, 9, GARUGA_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 24, 17, GARUGA_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_STATUS4):
        Em_PlaySeTypeAtBoneOnFrame(em, 16, 95, GARUGA_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_FALL):
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 83, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 20, 11, GARUGA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 58, 20, GARUGA_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 88, 28, GARUGA_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame(em, 88, 27, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 92, 27, GARUGA_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_FALL_LAND):
        Em_PlaySeAtBoneOnFrame(em, 10, 9, GARUGA_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 10, 7, GARUGA_BONE_TAIL_B);
        Em_PlaySeAtBoneOnFrame(em, 12, 17, GARUGA_BONE_ROOT);
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 84, GARUGA_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_HEAD_DOWN):
        Em_PlaySeAtBoneOnFrame(em, 12, 36, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 36, 34, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 102, 34, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 164, 34, GARUGA_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_IDLE_SIDE_A):
        Em_PlaySeAtBoneOnFrame(em, 2, 22, GARUGA_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame(em, 100, 23, GARUGA_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame(em, 38, 45, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 200, 32, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 28, 3, GARUGA_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 320, 3, GARUGA_BONE_STEP_B);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_IDLE_SIDE_B):
        Em_PlaySeAtBoneOnFrame(em, 2, 22, GARUGA_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame(em, 100, 23, GARUGA_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame(em, 38, 45, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 200, 32, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 28, 3, GARUGA_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 320, 3, GARUGA_BONE_STEP_A);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_DIE_FALLING):
        Em_PlaySeAtBoneOnFrame(em, 4, 22, GARUGA_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, 83):
        Em_PlaySeAtBoneOnFrame(em, 4, 22, GARUGA_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 4, 23, GARUGA_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 10, 16, GARUGA_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 10, 0, GARUGA_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 28, 0, GARUGA_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 52, 0, GARUGA_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 88, 0, GARUGA_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 128, 0, GARUGA_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 198, 0, GARUGA_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 4, 83, GARUGA_BONE_HEAD);
        Em_PlaySeTypeAtBoneOnFrame(em, 168, 79, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 162, 13, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 166, 13, GARUGA_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_HOP_IN_PLACE):
        Em_PlaySeAtBoneOnFrame(em, 22, 13, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 26, 13, GARUGA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 52, 10, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 56, 14, GARUGA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 78, 14, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 74, 10, GARUGA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 88, 12, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 92, 12, GARUGA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 32, 5, GARUGA_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 28, 0, GARUGA_BONE_STEP_A);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_FLIP_LAND):
        Em_PlaySeAtBoneOnFrame(em, 8, 32, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 8, 8, GARUGA_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 34, 22, GARUGA_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 104, 5, GARUGA_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 108, GARUGA_SE_STEP, GARUGA_BONE_STEP_A);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_LEVEL_OUT):
        Em_PlaySeAtBoneOnFrame(em, 32, 13, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 36, 13, GARUGA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 74, 13, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 78, 13, GARUGA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 124, 11, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 128, 11, GARUGA_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_CHARGE_TRIP):
        Em_PlaySeAtBoneOnFrame(em, 22, 53, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 16, 6, GARUGA_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 56, 6, GARUGA_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 176, 3, GARUGA_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 214, 3, GARUGA_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 264, GARUGA_SE_STEP, GARUGA_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 304, GARUGA_SE_STEP, GARUGA_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 100, 9, GARUGA_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 92, 16, GARUGA_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 140, 17, GARUGA_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 16, 15, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 20, 15, GARUGA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 276, 12, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 280, 12, GARUGA_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_TRAPPED):
        Em_PlaySeAtBoneOnFrame(em, 4, 16, GARUGA_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 10, 18, GARUGA_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 20, 9, GARUGA_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 4, 39, GARUGA_BONE_HEAD);
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 82, GARUGA_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_STRUGGLE):
        Em_PlaySeAtBoneOnFrame(em, 22, 18, GARUGA_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 72, 18, GARUGA_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 4, 12, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 8, 12, GARUGA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 48, 19, GARUGA_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 4, 40, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 4, 22, GARUGA_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_DOWNED):
        Em_PlaySeAtBoneOnFrame(em, 34, 22, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 38, 22, GARUGA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 98, 4, GARUGA_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame(em, 108, 3, GARUGA_BONE_BREATH);
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 82, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 4, 43, GARUGA_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_DOWNED_GET_UP):
        Em_PlaySeAtBoneOnFrame(em, 4, 22, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 8, 22, GARUGA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 108, 3, GARUGA_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame(em, 4, 40, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 70, 32, GARUGA_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_STOMPS):
        Em_PlaySeAtBoneOnFrame(em, 4, 22, GARUGA_BONE_HEAD);
        Em_PlaySeTypeAtBoneOnFrame(em, 46, 79, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 36, 13, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 40, 13, GARUGA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 146, 52, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 178, 51, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 210, 52, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 38, 18, GARUGA_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 108, 0, GARUGA_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 132, 0, GARUGA_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 142, 6, GARUGA_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 186, 6, GARUGA_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 220, 6, GARUGA_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 224, 0, GARUGA_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 252, 0, GARUGA_BONE_STEP_A);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_TURN_STOMP):
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 79, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 4, 13, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 8, 13, GARUGA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 324, 13, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 36, 13, GARUGA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 54, 6, GARUGA_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 54, 7, GARUGA_BONE_STEP_A);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_PECK_COMBO):
        Em_PlaySeAtBoneOnFrame(em, 4, 52, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 28, 50, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 50, 51, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 70, 50, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 90, 51, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 28, 5, GARUGA_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 28, 18, GARUGA_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 64, GARUGA_SE_STEP, GARUGA_BONE_STEP_B);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_RUN_B):
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 94, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 78, 50, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 104, 51, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 28, 2, GARUGA_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 52, 6, GARUGA_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 76, 6, GARUGA_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 100, 6, GARUGA_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 36, 13, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 40, 13, GARUGA_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_FIREBALL_B):
        Em_PlaySeAtBoneOnFrame(em, 20, 46, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 78, 41, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 78, GARUGA_SE_FIREBALL, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 120, GARUGA_SE_FIREBALL, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 164, GARUGA_SE_FIREBALL, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 4, 18, GARUGA_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 4, 19, GARUGA_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame(em, 64, 15, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 64, 15, GARUGA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 250, 32, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 298, 18, GARUGA_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 286, 19, GARUGA_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_PECK_COMBO_B):
        Em_PlaySeAtBoneOnFrame(em, 4, 18, GARUGA_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 18, 36, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 50, 50, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 74, 51, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 96, 50, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 116, 51, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 8, 6, GARUGA_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 48, 6, GARUGA_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 90, GARUGA_SE_STEP, GARUGA_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 16, 13, GARUGA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 20, 12, GARUGA_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, 108):
        Em_PlaySeAtBoneOnFrame(em, 0, 22, GARUGA_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 14, 18, GARUGA_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 18, 44, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 412, 32, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 200, 55, GARUGA_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 304, 22, GARUGA_BONE_BREATH);
        
    case EM_MOTION_ID(0, GARUGA_MOT_IDLE):
    default:
        break;
    }
}

void EmGaruga_Setup(Em *em)
{
}

/*
 * Callback slots 0 and 1: creates the alert mark at GARUGA_BONE_BREATH
 * (AlertMark_Create) and the monster's shadow (SHADOW_MODEL).
 */
void EmGaruga_SpawnEffects(void *em)
{
    AlertMark_Create(em, GARUGA_BONE_BREATH, 0);
    Shadow_Spawn((Player *) em, SHADOW_MODEL);
}

/*
 * Sets a Yian Garuga up: the scaled parts 3, 4 and 8; in the quest-less stage
 * (gQuest.stage 0) a fixed start position per area (Area_DayId), 1500 apart
 * per slot (the desert night area 11 also faces it ANGLE_TURN / 4); the idle
 * motion (GARUGA_MOT_IDLE) on the ground, full hp from Em_ComputeMaxHp, a
 * pending script, aimAngle ANGLE_TURN / 4 and the flight work pitch, turn and
 * roll speeds; then keeps only bit 0 of fxFlags and, when it is clear, hides
 * mesh 1 and creates the stage object of kind 9 (EmTail_Create).
 */
void EmGaruga_Init(Em *em)
{
    EmFlyerWork *w = (EmFlyerWork *)&em->work;
    s16 hp;

    Em_SetPartScaled(em, 3);
    Em_SetPartScaled(em, 4);
    Em_SetPartScaled(em, 8);
    if (gQuest.stage == 0) {
        em->rotY = 0;
        switch (Area_DayId(gScene.area)) {
        case 0:
            em->posX = 8000.0f + 1500.0f * (f32) em->slotNo;
            em->posY = 0.0f;
            em->posZ = 6000.0f;
            break;
        case AREA_SNOWY_MOUNTAINS_NIGHT_9:
            em->posX = 7900.0f + 1500.0f * (f32) em->slotNo;
            em->posY = 1050.0f;
            em->posZ = 13900.0f;
            break;
        case AREA_JUNGLE_NIGHT_5:
            em->posX = 10000.0f + 1500.0f * (f32) em->slotNo;
            em->posY = 200.0f;
            em->posZ = 8000.0f;
            break;
        case AREA_JUNGLE_NIGHT_7:
            em->posX = 9900.0f + 1500.0f * (f32) em->slotNo;
            em->posY = 300.0f;
            em->posZ = 10350.0f;
            break;
        case AREA_DESERT_NIGHT_11:
            em->posX = 10000.0f + 1500.0f * (f32) em->slotNo;
            em->posY = 0.0f;
            em->posZ = 5700.0f;
            em->rotY = ANGLE_TURN / 4;
            break;
        case AREA_VOLCANO_NIGHT_9:
            em->posX = 14300.0f + 1500.0f * (f32) em->slotNo;
            em->posY = 0.0f;
            em->posZ = 7100.0f;
            break;
        case AREA_FOREST_HILLS_3:
            em->posX = 17300.0f;
            em->posY = 0.0f;
            em->posZ = 18000.0f;
            break;
        case AREA_FOREST_HILLS_4:
            em->posX = 9750.0f + 1500.0f * (f32) em->slotNo;
            em->posY = 300.0f;
            em->posZ = 7750.0f;
            break;
        case AREA_FOREST_HILLS_5:
            em->posX = 10500.0f + 1500.0f * (f32) em->slotNo;
            em->posY = 0.0f;
            em->posZ = 9500.0f;
            break;
        default:
            em->posX = 5000.0f + 1500.0f * (f32) em->slotNo;
            em->posY = 0.0f;
            em->posZ = 9000.0f;
            break;
        }
    }
    Em_SetMotionRow(em, GARUGA_MOT_IDLE, 0, 0);
    em->stance = EM_STANCE_GROUND;
    w->fxMotion = 0;
    hp = Em_ComputeMaxHp(em, 0);
    em->hp = hp;
    em->hpMax = hp;
    em->scriptPending = 1;
    em->blind = 0;
    em->unk_InitFlag = 1;
    em->noAlert = 0;
    em->painted = 0;
    em->aimAngle = ANGLE_TURN / 4;
    w->pitchStep = 0x100;
    ((EmWork *)w)->turnSpeed = 512;
    w->rollSpeed = 0x100;
    w->moveAngBY = ANGLE_TURN / 8;
    *(s32 *) &em->flightTurn = 0;
    if ((em->fxFlags &= 1) == 0) {
        switch (em->type) {
        case EM_KIND_YIAN_GARUGA:
            em->meshShown[1] = 0;
            EmTail_Create(em);
            break;
        }
    }
}

/*
 * Starts a Yian Garuga: Init, then GARUGA_IDLE; in story stage
 * QUEST_ID_GARUGA_SCENE, until EVENT_FLAG_GARUGA_SCENE is set, places it in
 * AREA_JUNGLE_9 (for the current time) at a fixed position with no rotation.
 */
void EmGaruga_Start(Em *em)
{
    EmGaruga_Init(em);
    EmGaruga_ChangeState(em, EM_GROUP_IDLE, GARUGA_IDLE, EM_SYNC_LOCAL);
    switch (gQuest.stage) {
    case QUEST_ID_GARUGA_SCENE:
        if (Save_TestEventFlag(EVENT_FLAG_GARUGA_SCENE) == 0) {
            em->area = Area_IdForCurrentTime(AREA_JUNGLE_9);
            em->posX = 10051.0f;
            em->posY = 1040.0f;
            em->posZ = 8395.0f;
            em->rotX = 0;
            em->rotY = 0;
            em->rotZ = 0;
        }
        break;
    }
}

/*
 * Sets the monster up again (Init) without changing its state; on the Great
 * Arena map it moves it into arena area 1, at one of two fixed positions
 * and facings chosen by em->variant (0 or not).
 */
void EmGaruga_Reinit(Em *em)
{
    EmGaruga_Init(em);
    if ((Map_DayId((u16) gScene.map) & 0xFFFF) == MAP_GREAT_ARENA) {
        em->area = Area_IdForCurrentTime(AREA_GREAT_ARENA_1);
        if (em->variant == 0) {
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
}

/*
 * Ends a state: asks the script for the next one (scriptPending) and goes
 * to GARUGA_IDLE on the ground, or to GARUGA_FLY_HOVER in the air
 * (EM_SYNC_LOCAL); what EmGaruga_EndState and EmGaruga_EndReactState call.
 */
void EmGaruga_ToIdle(Em *em)
{
    em->scriptPending = 1;
    if (em->stance == EM_STANCE_GROUND) {
        EmGaruga_ChangeState(em, EM_GROUP_IDLE, GARUGA_IDLE, EM_SYNC_LOCAL);
        return;
    }
    EmGaruga_ChangeState(em, EM_GROUP_FLY, GARUGA_FLY_HOVER, EM_SYNC_LOCAL);
}

void EmGaruga_EndState(Em *em, s32 unused1, s32 unused2)
{
    EmGaruga_ToIdle(em);
}

void EmGaruga_EndReactState(Em *em, s32 unused1, s32 unused2)
{
    EmGaruga_EndState(em, unused1, unused2);
}

/*
 * Ends an air state with a short flight: sets the air stance, a pending
 * script and motion speed 1.0, then picks hover once (next 0,
 * GARUGA_FLY_HOVER_ONCE) or glide once (next 1, GARUGA_FLY_GLIDE_ONCE)
 * through Em_SetStateRand; other values pick no state.
 */
void EmGaruga_EndAirState(Em *em, u8 next)
{
    s32 k;

    k = next & 0xFF;
    em->stance = EM_STANCE_AIR;
    em->scriptPending = 1;
    em->motionSpeed = 1.0f;
    switch (k) {
    case 0:
        Em_SetStateRand(em, EM_GROUP_FLY, GARUGA_FLY_HOVER_ONCE);
        break;
    case 1:
        Em_SetStateRand(em, EM_GROUP_FLY, GARUGA_FLY_GLIDE_ONCE);
        break;
    }
}

/*
 * Restarts a motion layer idle loop (GARUGA_MOT_IDLE on layer 0, 1 or 2,
 * 10-frame blend) once that layer has no motion playing.
 */
void EmGaruga_RestartLayerMotion(Em *em, s32 layer)
{
    if (em->motion[layer].playing == 0) {
        switch (layer) {
        case 0:
            Em_SetLayerMotion(em, EM_MOTION_ID(0, GARUGA_MOT_IDLE), 10, 0, 0);
            break;
        case 1:
            Em_SetLayerMotion(em, EM_MOTION_ID(1, GARUGA_MOT_IDLE), 10, 0, 1);
            break;
        case 2:
            Em_SetLayerMotion(em, EM_MOTION_ID(2, GARUGA_MOT_IDLE), 10, 0, 2);
            break;
        }
    }
}

/*
 * GARUGA_IDLE_STAND: puts the three layers on GARUGA_MOT_IDLE (each unless
 * it already plays the tested motion), then counts Em.timer down and, once
 * it runs out, asks the script for the next state (scriptPending); it never
 * ends the state itself.
 */
void EmGaruga_Stand(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        if (em->motionId[0] != EM_MOTION_ID(0, GARUGA_MOT_IDLE)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(0, GARUGA_MOT_IDLE), 10, 0, 0);
        }
        if (em->motionId[1] != EM_MOTION_ID(0, GARUGA_MOT_TURN_STOMP)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(1, GARUGA_MOT_IDLE), 10, 0, 1);
        }
        if (em->motionId[2] != EM_MOTION_ID(1, GARUGA_MOT_IDLE)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(2, GARUGA_MOT_IDLE), 10, 0, 2);
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
 * GARUGA_IDLE: plays GARUGA_MOT_IDLE once on the ground, then, on the
 * console that runs the monster (em->remote 0), ends the state; the state
 * every action falls back to on the ground.
 */
void EmGaruga_Idle(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GARUGA_MOT_IDLE, 0, 0);
        break;
    case 1:
        if ((em->remote == 0) && (em->motion[0].playing == 0)) {
            em->step++;
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * GARUGA_IDLE_STEP_PECK: plays GARUGA_MOT_PECK_COMBO_B once and ends the
 * state.
 */
void EmGaruga_IdleStepPeck(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GARUGA_MOT_PECK_COMBO_B, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->lookAtTarget = 0;
            em->step++;
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
}

/* GARUGA_IDLE_PECK: plays GARUGA_MOT_PECK_COMBO once and ends the state. */
void EmGaruga_IdlePeck(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GARUGA_MOT_PECK_COMBO, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * GARUGA_IDLE_ACTION_C: the idle motion (GARUGA_MOT_IDLE) once, layers 1
 * and 2 kept on their idle loops (EmGaruga_RestartLayerMotion), then ends
 * the state.
 */
void EmGaruga_IdleActionC(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GARUGA_MOT_IDLE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
    EmGaruga_RestartLayerMotion(em, 1);
    EmGaruga_RestartLayerMotion(em, 2);
}

void EmGaruga_IdleSkipA(Em *em)
{
    EmGaruga_EndState(em, 0, 0);
}

void EmGaruga_IdleSkipB(Em *em)
{
    EmGaruga_EndState(em, 0, 0);
}

void EmGaruga_IdleSkipC(Em *em)
{
    EmGaruga_EndState(em, 0, 0);
}

/*
 * GARUGA_IDLE_PAIR_A: GARUGA_MOT_IDLE_SIDE_A or GARUGA_MOT_IDLE_SIDE_B (bit
 * 0 of stateRand set or clear) once, then ends the state.
 */
void EmGaruga_IdlePairA(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        if (em->stateRand & 1) {
            Em_SetMotion(em, GARUGA_MOT_IDLE_SIDE_A, 0, 0);
            break;
        }
        Em_SetMotion(em, GARUGA_MOT_IDLE_SIDE_B, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * GARUGA_IDLE_TURN_TO_GOAL: aims at the goal (aimAngle from Math_AngleXZ)
 * and plays GARUGA_MOT_TURN_STOMP; until frame 60 it sets rotY to the aim
 * only when within 0x200 of it (seen: no per-frame step here), then ends
 * the state when the layer 1 motion ends.
 */
void EmGaruga_IdleTurnToGoal(Em *em)
{
    u16 aimAngle;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GARUGA_MOT_TURN_STOMP, 0, 0);
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        break;
    case 1:
        aimAngle = em->aimAngle;
        if ((u32) ((((aimAngle - *(u16 *)&em->rotY) & 0xFFFF) + 0x200) & 0xFFFF) < 0x400U) {
            em->rotY = (s32) aimAngle;
        }
        if (EmMotion_ReachedFrame(em, 0, 60.0f) != 0) {
            em->step++;
        }
        break;
    case 2:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * GARUGA_IDLE_ACTION_H: plays GARUGA_MOT_IDLE_ACTION_H with layers 0 and 2
 * kept on their idle loops, sounds at GARUGA_BONE_HEAD at layer 1 frames 56
 * and 104, and ends the state when the layer 1 motion ends.
 */
void EmGaruga_IdleActionH(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GARUGA_MOT_IDLE_ACTION_H, 0, 0);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
    EmGaruga_RestartLayerMotion(em, 0);
    EmGaruga_RestartLayerMotion(em, 2);
    Em_PlaySeAtBoneOnLayerFrame(em, 56, 33, GARUGA_BONE_HEAD, 1);
    Em_PlaySeAtBoneOnLayerFrame(em, 104, 31, GARUGA_BONE_HEAD, 1);
}

/*
 * GARUGA_IDLE_ACTION_I: plays GARUGA_MOT_IDLE_ACTION_I once and ends the
 * state.
 */
void EmGaruga_IdleActionI(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GARUGA_MOT_IDLE_ACTION_I, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * GARUGA_IDLE_ACTION_J: plays GARUGA_MOT_IDLE_ACTION_J with layers 0 and 2
 * kept on their idle loops, sounds at GARUGA_BONE_HEAD at layer 1 frames 2
 * and 74, and ends the state when the layer 1 motion ends.
 */
void EmGaruga_IdleActionJ(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GARUGA_MOT_IDLE_ACTION_J, 0, 0);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
    EmGaruga_RestartLayerMotion(em, 0);
    EmGaruga_RestartLayerMotion(em, 2);
    Em_PlaySeAtBoneOnLayerFrame(em, 2, 32, GARUGA_BONE_HEAD, 1);
    Em_PlaySeAtBoneOnLayerFrame(em, 74, 31, GARUGA_BONE_HEAD, 1);
}

void EmGaruga_IdleSkipD(Em *em)
{
    EmGaruga_EndState(em, 0, 0);
}

/*
 * GARUGA_IDLE_PAIR_B: GARUGA_MOT_IDLE_SIDE_A, the idle motion,
 * GARUGA_MOT_IDLE_SIDE_B, the idle motion (each idle step waits for its
 * blend to finish), then ends the state.
 */
void EmGaruga_IdlePairB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GARUGA_MOT_IDLE_SIDE_A, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, GARUGA_MOT_IDLE, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].blend == 0) {
            em->step++;
            Em_SetMotion(em, GARUGA_MOT_IDLE_SIDE_B, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, GARUGA_MOT_IDLE, 0, 0);
        }
        break;
    case 4:
        if (em->motion[0].blend == 0) {
            em->step++;
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * GARUGA_IDLE_PAIR_C: as GARUGA_IDLE_PAIR_B with GARUGA_MOT_IDLE_SIDE_B
 * first, then SIDE_A.
 */
void EmGaruga_IdlePairC(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GARUGA_MOT_IDLE_SIDE_B, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, GARUGA_MOT_IDLE, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].blend == 0) {
            em->step++;
            Em_SetMotion(em, GARUGA_MOT_IDLE_SIDE_A, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, GARUGA_MOT_IDLE, 0, 0);
        }
        break;
    case 4:
        if (em->motion[0].blend == 0) {
            em->step++;
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * GARUGA_IDLE_ALERT: plays the alert idle (GARUGA_MOT_ALERT_IDLE) once and
 * ends the state.
 */
void EmGaruga_AlertIdle(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GARUGA_MOT_ALERT_IDLE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * GARUGA_IDLE_REST: lies down (GARUGA_MOT_LIE_DOWN, then GARUGA_MOT_LIE)
 * and rests for 5400 frames, blind and with EM_HIT_REACT_WAKE so a hit
 * gives EM_DMG_HIT_LYING, regaining 1 hp every 9 frames up to 35 percent of
 * hpMax (Em_RegenHp); when the timer runs out it clears lowHpNeed and goes
 * to GARUGA_IDLE_REST_END.
 */
void EmGaruga_Rest(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GARUGA_MOT_LIE_DOWN, 0, 0);
        Em_LoadReactionSet(em, 1);
        em->timer = 5400;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, GARUGA_MOT_LIE, 0, 0);
        }
        break;
    case 2:
        Em_RegenHp(em, 1, (s32) (0.35f * (f32) em->hpMax), 9);
        if (--em->timer <= 0) {
            em->step++;
            Em_ClearLowHpNeed(em);
            EmGaruga_ChangeState(em, EM_GROUP_IDLE, GARUGA_IDLE_REST_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmGaruga_ChangeState(em, EM_GROUP_IDLE, GARUGA_IDLE_REST_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * GARUGA_IDLE_EAT_START: lowers its head (GARUGA_MOT_HEAD_DOWN) with
 * EM_HIT_REACT_NO_FLINCH, sight row 1 and reaction set 1; at motion frame
 * 200 it goes on with GARUGA_IDLE_EAT.
 */
void EmGaruga_EatStart(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_NO_FLINCH);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GARUGA_MOT_HEAD_DOWN, 0, 0);
        Em_SelectSightRow(em, 1);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 200.0f, 0U) != 0) {
            em->step++;
            EmGaruga_ChangeState(em, EM_GROUP_IDLE, GARUGA_IDLE_EAT, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmGaruga_ChangeState(em, EM_GROUP_IDLE, GARUGA_IDLE_EAT, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * GARUGA_IDLE_EAT: restores the normal hit reaction, sight row 0 and
 * reaction set 0, lets the head-down motion finish, feeds once
 * (GARUGA_MOT_FEED) and ends the state.
 */
void EmGaruga_Eat(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->hitReactMode = 0;
        Em_SelectSightRow(em, 0);
        Em_LoadReactionSet(em, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, GARUGA_MOT_FEED, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * GARUGA_IDLE_STATUS2_COLLAPSE: status 2 began: falls over
 * (GARUGA_MOT_COLLAPSE) and lies (GARUGA_MOT_LIE), blind and with
 * EM_HIT_REACT_WAKE, until Em.timer runs out, then GARUGA_IDLE_STATUS2_END.
 */
void EmGaruga_Status2Collapse(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GARUGA_MOT_COLLAPSE, 0, 0);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, GARUGA_MOT_LIE, 0, 0);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            EmGaruga_ChangeState(em, EM_GROUP_IDLE, GARUGA_IDLE_STATUS2_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmGaruga_ChangeState(em, EM_GROUP_IDLE, GARUGA_IDLE_STATUS2_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * GARUGA_IDLE_STATUS2_REGEN: as GARUGA_IDLE_STATUS2_COLLAPSE, but lies for
 * 5400 frames regaining 1 hp every 9 frames up to 35 percent of hpMax
 * (Em_RegenHp), then GARUGA_IDLE_STATUS2_END.
 */
void EmGaruga_Status2Regen(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GARUGA_MOT_COLLAPSE, 0, 0);
        Em_LoadReactionSet(em, 1);
        em->timer = 5400;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, GARUGA_MOT_LIE, 0, 0);
        }
        break;
    case 2:
        Em_RegenHp(em, 1, (s32) (0.35f * (f32) em->hpMax), 9);
        if (--em->timer <= 0) {
            em->step++;
            EmGaruga_ChangeState(em, EM_GROUP_IDLE, GARUGA_IDLE_STATUS2_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmGaruga_ChangeState(em, EM_GROUP_IDLE, GARUGA_IDLE_STATUS2_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * GARUGA_IDLE_ACTION_K: plays GARUGA_MOT_IDLE_SIDE_A once and ends the
 * state.
 */
void EmGaruga_IdleActionK(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GARUGA_MOT_IDLE_SIDE_A, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * GARUGA_IDLE_REST_END: gets up after resting (GARUGA_MOT_GET_UP, reaction
 * set 0, blind while it rises), then GARUGA_IDLE_SHAKE_OFF.
 */
void EmGaruga_RestEnd(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, GARUGA_MOT_GET_UP, 0, 0);
        Em_LoadReactionSet(em, 0);
        em->blind = 2;
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            EmGaruga_ChangeState(em, EM_GROUP_IDLE, GARUGA_IDLE_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmGaruga_ChangeState(em, EM_GROUP_IDLE, GARUGA_IDLE_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * GARUGA_IDLE_STATUS2_END: gets up after status 2 (GARUGA_MOT_GET_UP, blind
 * while it rises), then GARUGA_IDLE_SHAKE_OFF.
 */
void EmGaruga_Status2End(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, GARUGA_MOT_GET_UP, 0, 0);
        Em_LoadReactionSet(em, 0);
        em->blind = 2;
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            EmGaruga_ChangeState(em, EM_GROUP_IDLE, GARUGA_IDLE_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmGaruga_ChangeState(em, EM_GROUP_IDLE, GARUGA_IDLE_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * GARUGA_IDLE_SHAKE_OFF: shakes off (GARUGA_MOT_SHAKE_OFF) and ends the
 * state.
 */
void EmGaruga_ShakeOff(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, GARUGA_MOT_SHAKE_OFF, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGaruga_IdleSkipE(Em *em)
{
    EmGaruga_EndState(em, 0, 0);
}

/*
 * GARUGA_IDLE_STATUS1_COLLAPSE: status 1 began: falls over
 * (GARUGA_MOT_COLLAPSE) and lies (GARUGA_MOT_LIE), blind and with
 * EM_HIT_REACT_WAKE, until Em.timer runs out, then GARUGA_IDLE_STATUS1_END.
 */
void EmGaruga_Status1Collapse(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GARUGA_MOT_COLLAPSE, 0, 0);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, GARUGA_MOT_LIE, 0, 0);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            EmGaruga_ChangeState(em, EM_GROUP_IDLE, GARUGA_IDLE_STATUS1_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmGaruga_ChangeState(em, EM_GROUP_IDLE, GARUGA_IDLE_STATUS1_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * GARUGA_IDLE_STATUS1_END: gets up after status 1 (GARUGA_MOT_GET_UP, blind
 * while it rises), then GARUGA_IDLE_SHAKE_OFF.
 */
void EmGaruga_Status1End(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, GARUGA_MOT_GET_UP, 0, 0);
        em->blind = 2;
        Em_LoadReactionSet(em, 0);
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            EmGaruga_ChangeState(em, EM_GROUP_IDLE, GARUGA_IDLE_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmGaruga_ChangeState(em, EM_GROUP_IDLE, GARUGA_IDLE_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * GARUGA_IDLE_STATUS2_DOWNED: status 2 in a hold: lies downed
 * (GARUGA_MOT_DOWNED), holdTimer kept at 5, blind and with
 * EM_HIT_REACT_WAKE, with lying puffs at GARUGA_BONE_BREATH and a sound at
 * GARUGA_BONE_HEAD
 * every 135 frames, until Em.timer runs out, then
 * GARUGA_REACT_STATUS2_DOWNED_END.
 */
void EmGaruga_Status2Downed(Em *em, EmWork *work)
{
    f32 pos[3];

    em->holdTimer = 5;
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GARUGA_MOT_DOWNED, 0, 0);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmGaruga_ChangeState(em, EM_GROUP_REACT, GARUGA_REACT_STATUS2_DOWNED_END, EM_SYNC_OWNER);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmGaruga_ChangeState(em, EM_GROUP_REACT, GARUGA_REACT_STATUS2_DOWNED_END, EM_SYNC_OWNER);
        }
        break;
    }
    pos[0] = 0.0f;
    pos[1] = 10.0f;
    pos[2] = 140.0f;
    Em_SpawnLyingPuffs(em, GARUGA_BONE_BREATH, pos, 1.6f);
    if (em->timer % 135 == 0) {
        Em_PlaySeAtBone(em, 87, GARUGA_BONE_HEAD);
    }
}

/*
 * GARUGA_IDLE_STATUS1_DOWNED: status 1 in a hold, as
 * GARUGA_IDLE_STATUS2_DOWNED, then GARUGA_REACT_STATUS1_DOWNED_END.
 */
void EmGaruga_Status1Downed(Em *em, EmWork *work)
{
    f32 pos[3];

    em->holdTimer = 5;
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GARUGA_MOT_DOWNED, 0, 0);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmGaruga_ChangeState(em, EM_GROUP_REACT, GARUGA_REACT_STATUS1_DOWNED_END, EM_SYNC_OWNER);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmGaruga_ChangeState(em, EM_GROUP_REACT, GARUGA_REACT_STATUS1_DOWNED_END, EM_SYNC_OWNER);
        }
        break;
    }
    pos[0] = 0.0f;
    pos[1] = 10.0f;
    pos[2] = 140.0f;
    Em_SpawnLyingPuffs(em, GARUGA_BONE_BREATH, pos, 1.6f);
    if (em->timer % 135 == 0) {
        Em_PlaySeAtBone(em, 87, GARUGA_BONE_HEAD);
    }
}

/*
 * GARUGA_IDLE_STOMPS, GARUGA_IDLE_STOMPS_RAGE_BY_HP and
 * GARUGA_IDLE_STOMPS_PENDING_RAGE: stomps (GARUGA_MOT_STOMPS) and ends the
 * state when the layer 1 motion ends.
 */
void EmGaruga_Stomps(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GARUGA_MOT_STOMPS, 0, 0);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * The shake-off after status 4, the stun or the second hold (variant 0, 1,
 * 2: GARUGA_IDLE_STATUS4_SHAKE_OFF, GARUGA_IDLE_STUN_SHAKE_OFF,
 * GARUGA_IDLE_HOLD2_SHAKE_OFF): GARUGA_MOT_SHAKE_OFF, then ends the state;
 * variant and work are not used.
 */
void EmGaruga_ShakeOffAfterStatus(Em *em, EmWork *work, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, GARUGA_MOT_SHAKE_OFF, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * GARUGA_MOVE_WALK: walks (GARUGA_MOT_WALK) to the goal (Em_StepToGoal,
 * step 64) and ends there; with no goal it walks until Em.timer runs out.
 */
void EmGaruga_Walk(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GARUGA_MOT_WALK, 0, 0);
        break;
    case 1:
        if (em->hasGoal != 0) {
            if (Em_StepToGoal(em, 64, 1) == 1) {
                em->step++;
                EmGaruga_EndState(em, 0, 0);
                break;
            }
        } else {
            if (--em->timer <= 0) {
                em->step++;
                EmGaruga_EndState(em, 0, 0);
            }
        }
        break;
    }
}

void EmGaruga_MoveEnd(Em *em)
{
    em->step++;
    EmGaruga_EndState(em, 0, 0);
}

/* GARUGA_MOVE_RUN_B: as GARUGA_MOVE_WALK with GARUGA_MOT_RUN_B. */
void EmGaruga_RunB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GARUGA_MOT_RUN_B, 0, 0);
        break;
    case 1:
        if (em->hasGoal != 0) {
            if (Em_StepToGoal(em, 64, 1) == 1) {
                em->step++;
                EmGaruga_EndState(em, 0, 0);
                break;
            }
        } else {
            if (--em->timer <= 0) {
                em->step++;
                EmGaruga_EndState(em, 0, 0);
            }
        }
        break;
    }
}

/*
 * GARUGA_MOVE_TURN: turns to the target with the first turn parameters
 * (Em_StartTurnToTarget, Em_TurnStep on EmGaruga_TurnParams[0]), then
 * clears EM_FLAG_NO_ROOT_MOTION and ends the state.
 */
void EmGaruga_Turn(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_StartTurnToTarget(em, &EmGaruga_TurnParams[0]);
        break;
    case 1:
        if (Em_TurnStep(em, &EmGaruga_TurnParams[0]) == 1) {
            em->step++;
            Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * GARUGA_MOVE_RUN (and GARUGA_MOVE_RUN_SLOW): as GARUGA_MOVE_WALK with
 * GARUGA_MOT_RUN.
 */
void EmGaruga_Run(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GARUGA_MOT_RUN, 0, 0);
        break;
    case 1:
        if (em->hasGoal != 0) {
            if (Em_StepToGoal(em, 64, 1) == 1) {
                em->step++;
                EmGaruga_EndState(em, 0, 0);
                break;
            }
        } else {
            if (--em->timer <= 0) {
                em->step++;
                EmGaruga_EndState(em, 0, 0);
            }
        }
        break;
    }
}

/*
 * GARUGA_MOVE_TURN_IN_COMBAT: as GARUGA_MOVE_TURN with the second turn
 * parameters (EmGaruga_TurnParams[1]).
 */
void EmGaruga_TurnInCombat(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_StartTurnToTarget(em, &EmGaruga_TurnParams[1]);
        break;
    case 1:
        if (Em_TurnStep(em, &EmGaruga_TurnParams[1]) == 1) {
            em->step++;
            Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
}

/* GARUGA_MOVE_LIMP: as GARUGA_MOVE_WALK with GARUGA_MOT_LIMP (low hp). */
void EmGaruga_Limp(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GARUGA_MOT_LIMP, 0, 0);
        break;
    case 1:
        if (em->hasGoal != 0) {
            if (Em_StepToGoal(em, 64, 1) == 1) {
                em->step++;
                EmGaruga_EndState(em, 0, 0);
                break;
            }
        } else {
            if (--em->timer <= 0) {
                em->step++;
                EmGaruga_EndState(em, 0, 0);
            }
        }
        break;
    }
}

/*
 * GARUGA_MOVE_TURN_TO_GOAL: turns in place to the goal with
 * GARUGA_MOT_TURN_IN_PLACE, 0x200 a frame (snapping when within 0x200),
 * until motion frame 60, then ends the state when the layer 1 motion ends.
 */
void EmGaruga_TurnToGoal(Em *em, EmWork *work)
{
    u16 aim;
    u32 diff;
    s32 yaw;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GARUGA_MOT_TURN_IN_PLACE, 0, 0);
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        break;
    case 1:
        yaw = em->rotY;
        aim = em->aimAngle;
        diff = (aim - (yaw & 0xFFFF)) & 0xFFFF;
        if (((diff + 512) & 0xFFFF) < 1024U) {
            em->rotY = aim;
        } else if (diff < 0x8000U) {
            em->rotY = (yaw + 0x200) & 0xFFFF;
        } else {
            em->rotY = (yaw - 0x200) & 0xFFFF;
        }
        if (EmMotion_ReachedFrame(em, 0, 60.0f) != 0) {
            em->step++;
        }
        break;
    case 2:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
}

void EmGaruga_MoveNop(void)
{
}

/*
 * GARUGA_MOVE_RUN_B_AND_TURN: runs (GARUGA_MOT_RUN_B) to the goal, or until
 * Em.timer runs out with no goal, then turns in place
 * (GARUGA_MOT_TURN_STOMP) and ends the state when that motion ends.
 */
void EmGaruga_RunBAndTurn(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GARUGA_MOT_RUN_B, 0, 0);
        break;
    case 1:
        if (em->hasGoal != 0) {
            if (Em_StepToGoal(em, 64, 1) == 1) {
                em->step++;
                Em_SetMotion(em, GARUGA_MOT_TURN_STOMP, 0, 0);
                break;
            }
        } else {
            if (--em->timer <= 0) {
                em->step++;
                Em_SetMotion(em, GARUGA_MOT_TURN_STOMP, 0, 0);
            }
            break;
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * GARUGA_FLY_TAKE_OFF: takes off (GARUGA_MOT_TAKE_OFF) along take-off curve
 * 2 (EmGaruga_StartCurves), airborne from frame 38, and when
 * EmGaruga_UpdateCurves reports the curve done ends with a hover once
 * (EmGaruga_EndAirState 0).
 */
void EmGaruga_TakeOff(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GARUGA_MOT_TAKE_OFF, 0, 0);
        EmGaruga_StartCurves(em, 2);
        work->flyGuard = 0;
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 38.0f, 0U) != 0) {
            em->step++;
            em->stance = EM_STANCE_AIR;
            EmGaruga_UpdateCurves(em);
        }
        break;
    case 2:
        if (EmGaruga_UpdateCurves(em)) {
            em->step++;
            EmGaruga_EndAirState(em, 0U);
        }
        break;
    }
}

/*
 * GARUGA_FLY_LAND: hovers down (GARUGA_MOT_HOVER) from a starting velY
 * sized to the height above floor+1000, eased back toward -10; below
 * floor+630, at a wing beat (motion frame 10, 86 or 160), descends
 * (GARUGA_MOT_DESCEND) to reach the floor in 30 frames, then touches down
 * (GARUGA_MOT_TOUCH_DOWN) on the ground with a camera shake
 * (CamShake_ReqAIf) and ends the state; never goes below floorY.
 */
void EmGaruga_Land(Em *em, EmWork *work)
{
    f32 height;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        em->rotX = 0;
        em->rotZ = 0;
        Em_SetMotion(em, GARUGA_MOT_HOVER, 0, 0);
        Player_ClearVelocity(em);
        em->velY = -10.0f;
        height = em->floorY;
        height = em->posY - (1000.0f + height);
        if (height > 0.0f) {
            do {
                em->velY -= 0.75f;
                height += em->velY;
            } while (height > 0.0f && !(em->velY < -200.0f));
        }
        work->flyGuard = 0;
        work->moveAngX = 0;
        work->moveAngZ = 0;
        break;
    case 1:
        work->moveAngY = em->rotY;
        Em_MoveRotated(em, &work->moveAngX);
        if (em->velY < -10.0f) {
            em->velY += 0.75f;
        }
        if (!((630.0f + em->floorY) <= em->posY) && ((EmMotion_PassedFrameScaled(em, 10.0f, 0U) != 0) || (EmMotion_PassedFrameScaled(em, 86.0f, 0U) != 0) || (EmMotion_PassedFrameScaled(em, 160.0f, 0U) != 0))) {
            em->step++;
            Em_SetMotion(em, GARUGA_MOT_DESCEND, 0, 0);
            em->velY = (em->floorY - em->posY) / 30.0f;
            if (!(em->velY < 0.0f)) {
                em->velY = -10.0f;
            }
            em->timer = 30;
        }
        break;
    case 2:
        work->moveAngX = 0;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
        if (--em->timer < 0) {
            em->step++;
            Em_SetMotion(em, GARUGA_MOT_TOUCH_DOWN, 0, 0);
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
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
    height = em->floorY;
    if (em->posY < height) {
        em->posY = height;
    }
}

/*
 * GARUGA_FLY_HOVER: hovers (GARUGA_MOT_HOVER, vertical curve 1 via
 * EmGaruga_MoveByVelYCurve), turning to the goal at 256 a frame, until
 * Em.timer runs out, then GARUGA_FLY_LAND; at timer 300 it asks for a hover
 * once instead (EmGaruga_EndAirState 0); never goes below floorY.
 */
void EmGaruga_Hover(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        work->turnSpeed = 256;
        Em_SetMotion(em, GARUGA_MOT_HOVER, 0, 0);
        work->flyGuard = 0;
        break;
    case 1:
        EmGaruga_MoveByVelYCurve(em, 1U);
        if (--em->timer <= 0) {
            em->step++;
            EmGaruga_ChangeState(em, EM_GROUP_FLY, GARUGA_FLY_LAND, EM_SYNC_SEND);
        }
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 1);
        if (em->timer == 300) {
            EmGaruga_EndAirState(em, 0U);
        }
        break;
    case 2:
        EmGaruga_MoveByVelYCurve(em, 1U);
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 1);
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * GARUGA_FLY_HOP_TO_GOAL: takes off (GARUGA_MOT_TAKE_OFF, airborne from
 * frame 30, turning to the goal between frames 50 and 114) and flies curve
 * 2; once the curve ends within 800 of the floor it descends
 * (GARUGA_MOT_DESCEND, velY set to reach the floor in 30 frames, at least
 * -10), touches down (GARUGA_MOT_TOUCH_DOWN, camera shake) and ends the
 * state, setting fxMotion to 150 frames since landing.
 */
void EmGaruga_HopToGoal(Em *em, EmWork *work)
{
    f32 velY;
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GARUGA_MOT_TAKE_OFF, 0, 0);
        Player_ClearVelocity(em);
        EmGaruga_StartCurves(em, 2);
        work->flyGuard = 0;
        work->turnSpeed = 256;
        break;
    case 1:
        if ((EmMotion_ReachedFrame(em, 0, 50.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 114.0f) == 0)) {
            Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 1);
        }
        if (EmMotion_ReachedFrame(em, 0, 30.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            EmGaruga_UpdateCurves(em);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
        }
        break;
    case 2:
        if ((EmGaruga_UpdateCurves(em) & 0xFF) && (em->posY <= (800.0f + em->floorY))) {
            em->step++;
            Em_SetMotion(em, GARUGA_MOT_DESCEND, 0, 0);
            Player_ClearVelocity(em);
            velY = (em->floorY - em->posY) / 30.0f;
            em->velY = velY;
            if (!(velY < 0.0f)) {
                em->velY = -10.0f;
            }
        }
        break;
    case 3:
        work->moveAngX = 0;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
        if (em->posY <= em->floorY) {
            em->step++;
            Em_SetMotion(em, GARUGA_MOT_TOUCH_DOWN, 0, 0);
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            CamShake_ReqAIf(em, 2);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            work->fxMotion = 150;
            em->step++;
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * GARUGA_FLY_HOP_FORWARD: a short hop forward (velZ 20) along curve 3:
 * GARUGA_MOT_TAKE_OFF, airborne from frame 30, then GARUGA_MOT_TOUCH_DOWN
 * with a camera shake on reaching the floor; ends the state when the
 * landing motion ends (fxMotion 150).
 */
void EmGaruga_HopForward(Em *em, EmWork *work)
{
    f32 floorY;
    s32 ended;

    ended = 0;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GARUGA_MOT_TAKE_OFF, 0, 0);
        EmGaruga_StartCurves(em, 3);
        em->velZ = 20.0f;
        work->flyGuard = 0;
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 30.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            ended = EmGaruga_UpdateCurves(em) & 0xFF;
        }
        if ((ended != 0) && (em->posY <= em->floorY)) {
            Em_SetMotion(em, GARUGA_MOT_TOUCH_DOWN, 0, 0);
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            CamShake_ReqAIf(em, 2);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            work->fxMotion = 150;
            em->step++;
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/* GARUGA_FLY_HOP_BACK: as EmGaruga_HopForward, hopping backward (velZ -20). */
void EmGaruga_HopBack(Em *em, EmWork *work)
{
    f32 floorY;
    s32 ended;

    ended = 0;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GARUGA_MOT_TAKE_OFF, 0, 0);
        EmGaruga_StartCurves(em, 3);
        em->velZ = -20.0f;
        work->flyGuard = 0;
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 30.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            ended = EmGaruga_UpdateCurves(em) & 0xFF;
        }
        if ((ended != 0) && (em->posY <= em->floorY)) {
            Em_SetMotion(em, GARUGA_MOT_TOUCH_DOWN, 0, 0);
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            CamShake_ReqAIf(em, 2);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            work->fxMotion = 150;
            em->step++;
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * GARUGA_FLY_ROUTE: glides (GARUGA_MOT_GLIDE, then GARUGA_MOT_FLY; velZ at
 * least 50) through the goal route with Em_UpdateFlight; when a goal is
 * within 3000 or Em.timer runs out it takes one of legsLeft: with none left
 * it glides once (EmGaruga_EndAirState 1), else steps curGoalNo (0-3),
 * computes the next goal (Em_ComputeGoal2), sets the timer to 1.5 times the
 * flight time at velZ 50 and sets GARUGA_FLY_ROUTE again (Em_SetStateRand).
 */
void EmGaruga_FlyRoute(Em *em, EmWork *work)
{
    f32 floorY;
    s32 timeLeft;
    u8 n;
    u8 stateVar1;

    switch (em->step) {
    case 0:
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, GARUGA_MOT_GLIDE, 0, 0);
        em->step++;
        em->stateVar1 = 0;
        em->lookAtTarget = 0;
        em->velX = 0.0f;
        em->velY = 0.0f;
        Player_ClearAccel2(em);
        if (em->velZ <= 50.0f) {
            em->velZ = 50.0f;
        }
        work->flyGuard = 1;
        break;
    case 1:
        stateVar1 = em->stateVar1;
        if ((stateVar1 == 0) && (em->motion[0].playing == 0)) {
            em->stateVar1 = stateVar1 + 1;
            Em_SetMotion(em, GARUGA_MOT_FLY, 0, 0);
        }
        if ((Vec_DistanceXZ(&em->posX, em->goal) <= 3000.0f) || (timeLeft = em->timer - 1, em->timer = timeLeft, (timeLeft <= 0))) {
            n = ((EmFlyerWork *)work)->legsLeft - 1;
            ((EmFlyerWork *)work)->legsLeft = n;
            if ((n & 0xFF) <= 0) {
                EmGaruga_EndAirState(em, 1U);
            } else {
                em->curGoalNo++;
                em->curGoalNo &= 3;
                Em_ComputeGoal2(em, em->goal);
                em->timer = (s32) ((1.5f * Vec_DistanceXZ(&em->posX, em->goal)) / 50.0f);
                Em_SetStateRand(em, EM_GROUP_FLY, GARUGA_FLY_ROUTE);
            }
        }
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 225, &EmGaruga_FlyParam);
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
 * GARUGA_FLY_HOVER_TO_GOAL: hovers (GARUGA_MOT_HOVER) toward the goal,
 * turning to it and pitching at it, with the velY curve; when Em.timer runs
 * out or the goal is within 10 times velZ it stops and hovers once
 * (EmGaruga_EndAirState 0).
 */
void EmGaruga_HoverToGoal(Em *em, EmWork *work)
{
    f32 dist;
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        work->turnSpeed = 256;
        Em_SetMotion(em, GARUGA_MOT_HOVER, 0, 0);
        work->flyGuard = 0;
        break;
    case 1:
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 1);
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_GetPitchToGoal(em, &work->moveAngX, 0.0f, 0.0f);
        Em_MoveRotated(em, &work->moveAngX);
        EmGaruga_MoveByVelYCurve(em, 1U);
        dist = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer <= 0 || dist <= (10.0f * em->velZ)) {
            em->step++;
            Player_ClearVelocity(em);
            EmGaruga_EndAirState(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * GARUGA_FLY_OUT_OF_AREA: hovers up (velY 20, growing by 0.75 above 500
 * over the floor up to 100) to the height of the exit point of the area
 * (Em_AreaExitPoint), then glides at velZ 50 (GARUGA_MOT_GLIDE) to it until
 * it leaves the area; then enters the next area (Em_EnterNextArea,
 * Hud_TriggerGlow, Em.timer 600) and flies on: GARUGA_FLY_TO_GOAL_AND_LAND
 * when that area is destArea (or there is none), else GARUGA_FLY_INTO_AREA
 * toward destArea (goalKind 3, set on the host only).
 */
void EmGaruga_FlyOutOfArea(Em *em, EmWork *work)
{
    u16 next;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, GARUGA_MOT_HOVER, 0, 0);
        work->flyGuard = 0;
        work->moveAngX = 0;
        work->moveAngZ = 0;
        Player_ClearVelocity(em);
        em->velY = 20.0f;
        Em_AreaExitPoint(em, (f32 *) em->goal);
        break;
    case 1:
        Em_MoveRotated(em, &em->rotX);
        if (em->posY > 500.0f + em->floorY && em->velY < 100.0f) {
            em->velY += 0.75f;
        }
        if (!(em->posY < em->goal[1])) {
            em->step++;
            em->stance = EM_STANCE_AIR;
            em->lookAtTarget = 0;
            Player_ClearVelocity(em);
            em->velZ = 50.0f;
            Em_SetMotion(em, GARUGA_MOT_GLIDE, 0, 0);
            work->flyGuard = 1;
        }
        break;
    case 2:
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 1249, &EmGaruga_FlyParam);
        work->moveAngX = em->rotX;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
        if (Area_ContainsPos(em->area, &em->posX) == 0) {
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
        next = *(u16 *)&em->destArea;
        if (em->nextArea == next || next == AREA_NONE) {
            Hud_TriggerGlow(em);
            EmGaruga_ChangeState(em, EM_GROUP_FLY, GARUGA_FLY_TO_GOAL_AND_LAND, EM_SYNC_SEND);
        } else {
            if (em->remote == 0) {
                em->nextArea = next;
                em->goalNo = next;
                em->goalKind = 3;
                Em_ComputeGoal(em, em->goal);
            }
            EmGaruga_ChangeState(em, EM_GROUP_FLY, GARUGA_FLY_INTO_AREA, EM_SYNC_SEND);
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
 * GARUGA_FLY_TO_GOAL_AND_LAND: faces the goal and flies to it at velZ 80
 * (Em_UpdateFlight), clearing destArea; within 1000 of the goal or when
 * Em.timer runs out it changes to GARUGA_FLY_LAND, the host retrying every
 * 600 frames until the change takes.
 */
void EmGaruga_FlyToGoalAndLand(Em *em, EmWork *work)
{
    f32 dist;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        work->turnSpeed = 256;
        Em_SetMotion(em, GARUGA_MOT_HOVER, 0, 0);
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
        Em_UpdateFlight(em, 229, &EmGaruga_FlyParam);
        work->moveAngX = em->rotX;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
        dist = Vec_DistanceXZ(&em->posX, em->goal);
        em->timer--;
        if (dist <= 1000.0f || em->timer < 0) {
            em->step++;
            em->timer = 600;
            EmGaruga_ChangeState(em, EM_GROUP_FLY, GARUGA_FLY_LAND, EM_SYNC_SEND);
        }
        break;
    case 2:
        if (em->remote == 0) {
            if (--em->timer <= 0) {
                em->timer = 600;
                EmGaruga_ChangeState(em, EM_GROUP_FLY, GARUGA_FLY_LAND, EM_SYNC_SEND);
            }
        }
        break;
    }
}

void EmGaruga_EndFlightToGlide(Em *em)
{
    EmGaruga_EndAirState(em, 1);
}

/*
 * GARUGA_FLY_GLIDE_TO_GOAL: glides (GARUGA_MOT_GLIDE, velZ at most 100) to
 * the goal; within 1000 (velZ cut to 50) or when Em.timer runs out it
 * changes to GARUGA_FLY_LAND, gliding on and retrying every 300 frames on
 * the host until the change takes.
 */
void EmGaruga_GlideToGoal(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GARUGA_MOT_GLIDE, 0, 0);
        if (!(em->velZ <= 100.0f)) {
            em->velZ = 100.0f;
        }
        work->flyGuard = 1;
        break;
    case 1:
        if (Vec_DistanceXZ(&em->posX, em->goal) <= 1000.0f) {
            if (!(em->velZ <= 50.0f)) {
                em->velZ = 50.0f;
            }
            em->step++;
            em->timer = 300;
            EmGaruga_ChangeState(em, EM_GROUP_FLY, GARUGA_FLY_LAND, EM_SYNC_SEND);
        } else {
            em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
            em->aimAngle = em->aimAngle - em->rotY;
            Em_UpdateFlight(em, 225, &EmGaruga_FlyParam);
            work->moveAngX = em->rotX;
            work->moveAngY = em->rotY;
            work->moveAngZ = 0;
            Em_MoveRotated(em, &work->moveAngX);
            if (--em->timer <= 0) {
                em->step++;
                EmGaruga_ChangeState(em, EM_GROUP_FLY, GARUGA_FLY_LAND, EM_SYNC_SEND);
            }
        }
        break;
    case 2:
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 225, &EmGaruga_FlyParam);
        work->moveAngX = em->rotX;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
        if (--em->timer <= 0) {
            em->timer = 300;
            if (em->remote == 0) {
                EmGaruga_ChangeState(em, EM_GROUP_FLY, GARUGA_FLY_LAND, EM_SYNC_SEND);
            }
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * GARUGA_FLY_BACK_HOP: hops backward along curve 14 (GARUGA_MOT_BACK_HOP,
 * airborne from frame 76), lands with GARUGA_MOT_BACK_HOP_LAND, a dust
 * cloud (EmGaruga_SpawnLandingDust) and a camera shake, and ends the state
 * when that motion ends. Takes only em.
 */
void EmGaruga_BackHop(Em *em)
{
    f32 floorY;
    s32 ended;

    ended = 0;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GARUGA_MOT_BACK_HOP, 0, 0);
        EmGaruga_StartCurves(em, 14);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 76.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            ended = EmGaruga_UpdateCurves(em) & 0xFF;
        }
        if ((ended != 0) && (em->posY <= em->floorY)) {
            Em_SetMotion(em, GARUGA_MOT_BACK_HOP_LAND, 0, 0);
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            EmGaruga_SpawnLandingDust(em);
            CamShake_ReqAIf(em, 2);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * GARUGA_FLY_INTO_AREA: flies into the new area at velZ 80
 * (GARUGA_MOT_HOVER) toward its entry point (Em_AreaEntryValue) at the
 * bottom of the area, or the middle of the area when it has none; within
 * 1000 or when Em.timer runs out it turns to the exit point and flies out,
 * then goes on as GARUGA_FLY_OUT_OF_AREA (GARUGA_FLY_TO_GOAL_AND_LAND or
 * GARUGA_FLY_INTO_AREA again), the host retrying every 600 frames.
 */
void EmGaruga_FlyIntoArea(Em *em, EmWork *work)
{
    AreaRecord *area;
    f32 *entry;
    f32 dist;
    u16 next;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        work->turnSpeed = 256;
        Em_SetMotion(em, GARUGA_MOT_HOVER, 0, 0);
        Player_ClearVelocity(em);
        em->velZ = 80.0f;
        area = Stage_GetAreaRecord(em->area);
        em->goal[1] = area->bottom;
        entry = Em_AreaEntryValue(em, *em->tables);
        if (entry == NULL) {
            em->goal[0] = area->sizeX / 2.0f;
            em->goal[2] = area->sizeZ / 2.0f;
        } else {
            em->goal[0] = entry[0];
            em->goal[2] = entry[2];
        }
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->rotY = em->aimAngle;
        work->flyGuard = 1;
        break;
    case 1:
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 229, &EmGaruga_FlyParam);
        work->moveAngX = em->rotX;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
        dist = Vec_DistanceXZ(&em->posX, em->goal);
        em->timer--;
        if (dist <= 1000.0f || em->timer < 0) {
            em->step++;
            Em_AreaExitPoint(em, (f32 *) em->goal);
        }
        break;
    case 2:
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 225, &EmGaruga_FlyParam);
        work->moveAngX = em->rotX;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
        if (Area_ContainsPos(em->area, &em->posX) == 0) {
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
            EmGaruga_ChangeState(em, EM_GROUP_FLY, GARUGA_FLY_TO_GOAL_AND_LAND, EM_SYNC_SEND);
        } else {
            if (em->remote == 0) {
                em->nextArea = next;
                em->goalNo = next;
                em->goalKind = 3;
                Em_ComputeGoal(em, em->goal);
            }
            EmGaruga_ChangeState(em, EM_GROUP_FLY, GARUGA_FLY_INTO_AREA, EM_SYNC_SEND);
            Hud_TriggerGlow(em);
        }
        break;
    case 4:
        if (em->remote == 0) {
            if (--em->timer <= 0) {
                em->step = 3;
            }
        }
        break;
    }
    if (em->posY < em->floorY) {
        em->posY = em->floorY;
    }
}

/*
 * GARUGA_FLY_HOVER_ONCE: hovers (GARUGA_MOT_HOVER, velY curve) for one
 * motion, then hovers once more (EmGaruga_EndAirState 0), the script
 * picking what follows.
 */
void EmGaruga_HoverOnce(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, GARUGA_MOT_HOVER, 0, 0);
        work->flyGuard = 0;
        break;
    case 1:
        EmGaruga_MoveByVelYCurve(em, 1U);
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGaruga_EndAirState(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * GARUGA_FLY_GLIDE_ONCE: glides (GARUGA_MOT_GLIDE, velX and velY cleared)
 * for one motion, then glides once more (EmGaruga_EndAirState 1), the
 * script picking what follows.
 */
void EmGaruga_GlideOnce(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, GARUGA_MOT_GLIDE, 0, 0);
        Player_ClearAccel2(em);
        em->velY = 0.0f;
        em->velX = 0.0f;
        work->flyGuard = 1;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGaruga_EndAirState(em, 1U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * GARUGA_FLY_HOVER_TURN: hovers (GARUGA_MOT_HOVER, velY curve) turning to
 * the goal, for 150 frames at most or until facing it, then hovers once
 * (EmGaruga_EndAirState 0).
 */
void EmGaruga_HoverTurn(Em *em, EmWork *work)
{
    f32 floorY;
    s32 done;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, GARUGA_MOT_HOVER, 0, 0);
        work->turnSpeed = 256;
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        work->flyGuard = 0;
        em->timer = 150;
        break;
    case 1:
        EmGaruga_MoveByVelYCurve(em, 1U);
        done = Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 1) & 0xFF;
        if (--em->timer <= 0 || done != 0) {
            em->step++;
            EmGaruga_EndAirState(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * GARUGA_FLY_ROUTE_B: flies the goal route as GARUGA_FLY_ROUTE with the
 * second flight parameters (EmGaruga_FlyParam2), keeping a glide or fly
 * motion already running when velZ is 50 or more; at each goal (within
 * 1000, or Em.timer out) it steps curGoalNo, computes the next goal, sets
 * the timer to the flight time plus 150 and sets GARUGA_FLY_ROUTE_B again
 * (Em_SetStateRand); it has no leg count.
 */
void EmGaruga_FlyRouteB(Em *em, EmWork *work)
{
    f32 floorY;
    u16 motionId;
    u8 stateVar1;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        em->velX = 0.0f;
        em->velY = 0.0f;
        Player_ClearAccel2(em);
        if (em->velZ < 50.0f) {
            em->stateVar1 = 0;
            em->velZ = 50.0f;
            Em_SetMotion(em, GARUGA_MOT_GLIDE, 0, 0);
        } else {
            em->stateVar1 = 1;
            motionId = em->motionId[0];
            if ((motionId != EM_MOTION_ID(0, GARUGA_MOT_GLIDE)) && (motionId != EM_MOTION_ID(0, GARUGA_MOT_FLY))) {
                Em_SetMotion(em, GARUGA_MOT_GLIDE, 0, 0);
            }
        }
        work->flyGuard = 1;
        break;
    case 1:
        stateVar1 = em->stateVar1;
        if ((stateVar1 == 0) && (em->motion[0].playing == 0)) {
            em->stateVar1 = stateVar1 + 1;
            Em_SetMotion(em, GARUGA_MOT_FLY, 0, 0);
        }
        if (Vec_DistanceXZ(&em->posX, em->goal) <= 1000.0f || --em->timer <= 0) {
            em->curGoalNo++;
            em->curGoalNo &= 3;
            Em_ComputeGoal2(em, em->goal);
            em->timer = (s32) ((1.5f * Vec_DistanceXZ(&em->posX, em->goal)) / 50.0f) + 150;
            em->step++;
            Em_SetStateRand(em, EM_GROUP_FLY, GARUGA_FLY_ROUTE_B);
        }
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 801, &EmGaruga_FlyParam2);
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
 * GARUGA_FLY_LEVEL_OUT_AND_LAND: brings pitch and roll back to 0 (by
 * pitchStep and rollSpeed) while playing GARUGA_MOT_LEVEL_OUT along curve
 * 9; when the curve ends it hovers (GARUGA_MOT_HOVER) and changes to
 * GARUGA_FLY_LAND, the host retrying every 300 frames.
 */
void EmGaruga_LevelOutAndLand(Em *em, EmWork *work)
{
    f32 floorY;
    s32 rotX;
    s32 rotZ;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GARUGA_MOT_LEVEL_OUT, 0, 0);
        EmGaruga_StartCurves(em, 9);
        work->flyGuard = 0;
        break;
    case 1:
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
        if (EmGaruga_UpdateCurves(em) & 0xFF) {
            em->step++;
            Player_ClearVelocity(em);
            em->timer = 300;
            Em_SetMotion(em, GARUGA_MOT_HOVER, 0, 0);
            EmGaruga_ChangeState(em, EM_GROUP_FLY, GARUGA_FLY_LAND, EM_SYNC_SEND);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->timer = 300;
            Em_SetMotion(em, GARUGA_MOT_HOVER, 0, 0);
            if (em->remote == 0) {
                EmGaruga_ChangeState(em, EM_GROUP_FLY, GARUGA_FLY_LAND, EM_SYNC_SEND);
            }
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * GARUGA_FLY_GLIDE_TO_GOAL_B: glides to the goal with EmGaruga_FlyParam2
 * (starting as EmGaruga_FlyRouteB), then, within 1000 or when Em.timer runs
 * out, sets GARUGA_FLY_GLIDE_ONCE (Em_SetStateRand).
 */
void EmGaruga_GlideToGoalB(Em *em, EmWork *work)
{
    f32 floorY;
    u16 motionId;
    u8 stateVar1;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        em->velX = 0.0f;
        em->velY = 0.0f;
        Player_ClearAccel2(em);
        if (em->velZ < 50.0f) {
            em->stateVar1 = 0;
            em->velZ = 50.0f;
            Em_SetMotion(em, GARUGA_MOT_GLIDE, 0, 0);
        } else {
            em->stateVar1 = 1;
            motionId = em->motionId[0];
            if ((motionId != EM_MOTION_ID(0, GARUGA_MOT_GLIDE)) && (motionId != EM_MOTION_ID(0, GARUGA_MOT_FLY))) {
                Em_SetMotion(em, GARUGA_MOT_GLIDE, 0, 0);
            }
        }
        work->flyGuard = 1;
        break;
    case 1:
        stateVar1 = em->stateVar1;
        if ((stateVar1 == 0) && (em->motion[0].playing == 0)) {
            em->stateVar1 = stateVar1 + 1;
            Em_SetMotion(em, GARUGA_MOT_FLY, 0, 0);
        }
        if (Vec_DistanceXZ(&em->posX, em->goal) <= 1000.0f || --em->timer <= 0) {
            em->step++;
            Em_SetStateRand(em, EM_GROUP_FLY, GARUGA_FLY_GLIDE_ONCE);
        }
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 289, &EmGaruga_FlyParam2);
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
 * GARUGA_FLY_HOP_IN_PLACE: a hop in place along curve 12
 * (GARUGA_MOT_HOP_IN_PLACE, airborne from frame 36), landing with a dust
 * cloud (EmGaruga_SpawnLandingDust) and a camera shake; ends the state when
 * the motion ends.
 */
void EmGaruga_HopInPlace(Em *em, EmWork *work)
{
    f32 floorY;
    s32 ended;

    ended = 0;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GARUGA_MOT_HOP_IN_PLACE, 0, 0);
        EmGaruga_StartCurves(em, 12);
        work->flyGuard = 0;
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 36.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            ended = EmGaruga_UpdateCurves(em) & 0xFF;
        }
        if ((ended != 0) && (em->posY <= em->floorY)) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            EmGaruga_SpawnLandingDust(em);
            CamShake_ReqAIf(em, 2);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * GARUGA_FLY_HOVER_STRAIGHT_TO_GOAL: hovers (GARUGA_MOT_HOVER) straight at
 * the goal in three dimensions (heading and pitch to it, without turning
 * the body); when Em.timer runs out or the goal is within 10 times velZ (3D
 * distance) it stops and hovers once (EmGaruga_EndAirState 0).
 */
void EmGaruga_HoverStraightToGoal(Em *em, EmWork *work)
{
    f32 dist;
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        work->turnSpeed = 256;
        Em_SetMotion(em, GARUGA_MOT_HOVER, 0, 0);
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
            EmGaruga_EndAirState(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * GARUGA_FLY_LEVEL_OUT: brings pitch and roll back to 0 while playing
 * GARUGA_MOT_LEVEL_OUT along curve 9, then stops and hovers once
 * (EmGaruga_EndAirState 0).
 */
void EmGaruga_LevelOut(Em *em, EmWork *work)
{
    s32 rotX;
    s32 rotZ;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GARUGA_MOT_LEVEL_OUT, 0, 0);
        EmGaruga_StartCurves(em, 9);
        work->flyGuard = 0;
        break;
    case 1:
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
        if (EmGaruga_UpdateCurves(em) & 0xFF) {
            em->step++;
            Player_ClearVelocity(em);
            EmGaruga_EndAirState(em, 0U);
        }
        break;
    case 2:
        EmGaruga_UpdateCurves(em);
        break;
    }
}

/*
 * GARUGA_FLY_ESCAPE_UPWARD: breaks out of a trap: takes off
 * (GARUGA_MOT_TAKE_OFF from frame 10, curve 2; holdTimer and trapHit
 * cleared, hitReactMode 0, noReact 1), then, once the curve ends within 800
 * of the floor, descends and touches down as EmGaruga_HopToGoal and ends
 * the state (fxMotion 150).
 */
void EmGaruga_EscapeUpward(Em *em, EmWork *work)
{
    f32 floorY;
    f32 velY;
    f32 y;

    switch (em->step) {
    case 0:
        em->step++;
        em->hitReactMode = 0;
        Em_SetMotion(em, GARUGA_MOT_TAKE_OFF, 10, 30);
        EmGaruga_StartCurves(em, 2);
        em->stance = EM_STANCE_AIR;
        em->holdTimer = 0;
        em->trapHit = 0;
        em->noReact = 1;
        break;
    case 1:
        EmGaruga_UpdateCurves(em);
        if (em->motion[0].playing == 0) {
            em->step++;
        }
        break;
    case 2:
        if ((EmGaruga_UpdateCurves(em) & 0xFF) && (em->posY <= (800.0f + em->floorY))) {
            em->step++;
            Em_SetMotion(em, GARUGA_MOT_DESCEND, 0, 0);
            Player_ClearVelocity(em);
            floorY = em->floorY;
            if (em->posY < floorY) {
                em->posY = floorY;
            }
            velY = (em->floorY - em->posY) / 30.0f;
            em->velY = velY;
            if (!(velY < 0.0f)) {
                em->velY = -10.0f;
            }
        }
        break;
    case 3:
        work->moveAngX = 0;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
        if (em->posY <= em->floorY) {
            em->step++;
            Em_SetMotion(em, GARUGA_MOT_TOUCH_DOWN, 0, 0);
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            CamShake_ReqAIf(em, 2);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            work->fxMotion = 150;
            em->step++;
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
    y = em->floorY;
    if (em->posY < y) {
        em->posY = y;
    }
}

/*
 * GARUGA_FLY_HOVER_RISE: hovers upward (GARUGA_MOT_HOVER, velY 20) until
 * 1000 above the height of the goal or Em.timer runs out, then hovers once
 * (EmGaruga_EndAirState 0). Seen: the heading is taken toward the local
 * pos[3], which is never set.
 */
void EmGaruga_HoverRise(Em *em, EmWork *work)
{
    f32 pos[3];
    f32 floorY;
    s32 timer;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, GARUGA_MOT_HOVER, 0, 0);
        work->flyGuard = 0;
        work->moveAngX = 0;
        work->moveAngZ = 0;
        Player_ClearVelocity(em);
        em->velY = 20.0f;
        break;
    case 1:
        work->moveAngY = Math_AngleXZ(&em->posX, pos) & 0xFFFF;
        Em_MoveRotated(em, &work->moveAngX);
        timer = em->timer - 1;
        em->timer = timer;
        if ((timer <= 0) || !(em->posY <= (1000.0f + em->goal[1]))) {
            em->step++;
            Player_ClearVelocity(em);
            EmGaruga_EndAirState(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * GARUGA_ATK_TURN_AND_STARE: turns to the target (Em_TurnToTargetMotion),
 * then plays GARUGA_MOT_STARE and ends the state.
 */
void EmGaruga_TurnAndStare(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        if (Em_TurnToTargetMotion(em, 3640, GARUGA_MOT_WALK, GARUGA_MOT_TURN_LEFT, GARUGA_MOT_TURN_RIGHT) != 0) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->lookAtTarget = 0;
            Em_SetMotion(em, GARUGA_MOT_STARE, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * GARUGA_ATK_STARE: plays GARUGA_MOT_STARE in place, bending toward the
 * target (lookAtTarget 1), then ends the attack (EmGaruga_EndAttackState).
 */
void EmGaruga_Stare(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 1;
        Em_SetMotion(em, GARUGA_MOT_STARE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->lookAtTarget = 0;
            em->step++;
            EmGaruga_EndAttackState(em, work);
        }
        break;
    }
}

/*
 * GARUGA_ATK_TAIL_SPIN: plays GARUGA_MOT_TAIL_SPIN twice, turning rotY by
 * 0x200 every frame, each one ending with motion layer 1; then ends the
 * attack (EmGaruga_EndAttackState).
 */
void EmGaruga_TailSpin(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GARUGA_MOT_TAIL_SPIN, 0, 0);
        Motion_InitTime(em, 0);
        break;
    case 1:
        em->rotY += 0x200;
        if (em->motion[1].playing == 0) {
            em->step++;
            Em_SetMotion(em, GARUGA_MOT_TAIL_SPIN, 0, 0);
        }
        break;
    case 2:
        em->rotY += 0x200;
        if (em->motion[1].playing == 0) {
            em->step++;
            EmGaruga_EndAttackState(em, work);
        }
        break;
    }
}

/*
 * GARUGA_ATK_TURN_AND_PECK_A: turns to the target (Em_TurnToTargetMotion),
 * then plays GARUGA_MOT_PECK_COMBO and ends the state.
 */
void EmGaruga_TurnAndPeckA(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        if (Em_TurnToTargetMotion(em, 3640, GARUGA_MOT_WALK, GARUGA_MOT_TURN_LEFT, GARUGA_MOT_TURN_RIGHT) != 0) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->lookAtTarget = 0;
            Em_SetMotion(em, GARUGA_MOT_PECK_COMBO, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * GARUGA_ATK_CHARGE_TRIP: runs at the goal (GARUGA_MOT_RUN, Em_StepToGoal)
 * until it is within 500 or Em.timer runs out, then trips
 * (GARUGA_MOT_CHARGE_TRIP, GarugaAttack_Create row 36) and ends the
 * state when that motion ends.
 */
void EmGaruga_ChargeTrip(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GARUGA_MOT_RUN, 0, 0);
        break;
    case 1:
        if (em->hasGoal != 0) {
            Em_StepToGoal(em, 64, 1);
            if (em->goalDist <= 500.0f) {
                em->timer = 1;
            }
        }
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, GARUGA_MOT_CHARGE_TRIP, 0, 0);
            GarugaAttack_Create(em, 36);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * GARUGA_ATK_TURN_AND_PECK_B: turns to the target (Em_TurnToTargetMotion),
 * then plays GARUGA_MOT_PECK_COMBO_B and ends the state.
 */
void EmGaruga_TurnAndPeckB(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        
    case 1:
        if (Em_TurnToTargetMotion(em, 3640, GARUGA_MOT_WALK, GARUGA_MOT_TURN_LEFT, GARUGA_MOT_TURN_RIGHT) != 0) {
            em->step++;
            em->lookAtTarget = 0;
            Em_SetMotion(em, GARUGA_MOT_PECK_COMBO_B, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->lookAtTarget = 0;
            em->step++;
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * GARUGA_ATK_ROAR: plays GARUGA_MOT_ROAR in place and
 * ends the state.
 */
void EmGaruga_Roar(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GARUGA_MOT_ROAR, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->lookAtTarget = 0;
            em->step++;
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * GARUGA_ATK_FIREBALL: turns to the target, then breathes a fireball
 * (GARUGA_MOT_FIREBALL): at frame 78 two AttackBurst_CreateAtBone bursts at
 * GARUGA_BONE_FIREBALL_BURST and a fireball (EMRANGED_FIREBALL) from
 * GARUGA_BONE_BREATH, with no angle offsets; dust clouds (type 26) every 4th
 * frame from frame 154 to 276. When the motion ends it counts down countdown:
 * at 0 it ends the state, else it re-aims (Em_SetTargetAngle) and sets
 * GARUGA_ATK_FIREBALL again (Em_SetStateRand).
 */
void EmGaruga_Fireball(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
    case 1:
        if (Em_TurnToTargetMotion(em, 3640, GARUGA_MOT_WALK, GARUGA_MOT_TURN_LEFT, GARUGA_MOT_TURN_RIGHT) != 0) {
            em->step++;
            em->lookAtTarget = 0;
            Em_SetMotion(em, GARUGA_MOT_FIREBALL, 0, 0);
        }
        break;
    case 2:
        if (EmMotion_ReachedFrame(em, 0, 154.0f) != 0 && EmMotion_ReachedFrame(em, 0, 276.0f) == 0) {
            if (!(gScene.frame & 3)) {
                DustCloud_Create((Em *) em, 26, (u16) Rand_Next(1) & 1, 1.0f);
            }
        } else if (EmMotion_PassedFrameScaled(em, 78.0f, 0U) != 0) {
            AttackBurst_CreateAtBone(em, GARUGA_BONE_FIREBALL_BURST, 1, 0);
            AttackBurst_CreateAtBone(em, GARUGA_BONE_FIREBALL_BURST, 2, 0);
            EmRangedAttack_CreateAtBone(em, GARUGA_BONE_BREATH, EMRANGED_FIREBALL, 0, 0, 0);
        }
        if (em->motion[0].playing == 0) {
            em->lookAtTarget = 0;
            em->step++;
            if (--((EmFlyerWork *)work)->countdown <= 0) {
                EmGaruga_EndState(em, 0, 0);
            } else {
                Em_SetTargetAngle(em);
                Em_SetStateRand(em, EM_GROUP_ATTACK, GARUGA_ATK_FIREBALL);
            }
        }
        break;
    }
}

/*
 * GARUGA_ATK_FIREBALL_LOW: as EmGaruga_Fireball, the ranged attack taking
 * angle argument 3641, repeating as GARUGA_ATK_FIREBALL_LOW.
 */
void EmGaruga_FireballLow(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        break;
    case 1:
        if (Em_TurnToTargetMotion(em, 3640, GARUGA_MOT_WALK, GARUGA_MOT_TURN_LEFT, GARUGA_MOT_TURN_RIGHT) != 0) {
            em->step++;
            em->lookAtTarget = 0;
            Em_SetMotion(em, GARUGA_MOT_FIREBALL, 0, 0);
        }
        break;
    case 2:
        if (EmMotion_ReachedFrame(em, 0, 154.0f) != 0 && EmMotion_ReachedFrame(em, 0, 276.0f) == 0) {
            if (!(gScene.frame & 3)) {
                DustCloud_Create((Em *) em, 26, (u16) Rand_Next(1) & 1, 1.0f);
            }
        } else if (EmMotion_PassedFrameScaled(em, 78.0f, 0U) != 0) {
            AttackBurst_CreateAtBone(em, GARUGA_BONE_FIREBALL_BURST, 1, 0);
            AttackBurst_CreateAtBone(em, GARUGA_BONE_FIREBALL_BURST, 2, 0);
            EmRangedAttack_CreateAtBone(em, GARUGA_BONE_BREATH, EMRANGED_FIREBALL, 0, 3641, 0);
        }
        if (em->motion[0].playing == 0) {
            em->lookAtTarget = 0;
            em->step++;
            if (--((EmFlyerWork *)work)->countdown <= 0) {
                EmGaruga_EndState(em, 0, 0);
            } else {
                Em_SetTargetAngle(em);
                Em_SetStateRand(em, EM_GROUP_ATTACK, GARUGA_ATK_FIREBALL_LOW);
            }
        }
        break;
    }
}

/*
 * GARUGA_ATK_FIREBALL_HIGH: as EmGaruga_Fireball, the ranged attack taking
 * angle argument 61896, repeating as GARUGA_ATK_FIREBALL_HIGH.
 */
void EmGaruga_FireballHigh(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        break;
    case 1:
        if (Em_TurnToTargetMotion(em, 3640, GARUGA_MOT_WALK, GARUGA_MOT_TURN_LEFT, GARUGA_MOT_TURN_RIGHT) != 0) {
            em->step++;
            em->lookAtTarget = 0;
            Em_SetMotion(em, GARUGA_MOT_FIREBALL, 0, 0);
        }
        break;
    case 2:
        if (EmMotion_ReachedFrame(em, 0, 154.0f) != 0 && EmMotion_ReachedFrame(em, 0, 276.0f) == 0) {
            if (!(gScene.frame & 3)) {
                DustCloud_Create((Em *) em, 26, (u16) Rand_Next(1) & 1, 1.0f);
            }
        } else if (EmMotion_PassedFrameScaled(em, 78.0f, 0U) != 0) {
            AttackBurst_CreateAtBone(em, GARUGA_BONE_FIREBALL_BURST, 1, 0);
            AttackBurst_CreateAtBone(em, GARUGA_BONE_FIREBALL_BURST, 2, 0);
            EmRangedAttack_CreateAtBone(em, GARUGA_BONE_BREATH, EMRANGED_FIREBALL, 0, 61896, 0);
        }
        if (em->motion[0].playing == 0) {
            em->lookAtTarget = 0;
            em->step++;
            if (--((EmFlyerWork *)work)->countdown <= 0) {
                EmGaruga_EndState(em, 0, 0);
            } else {
                Em_SetTargetAngle(em);
                Em_SetStateRand(em, EM_GROUP_ATTACK, GARUGA_ATK_FIREBALL_HIGH);
            }
        }
        break;
    }
}

/*
 * GARUGA_ATK_FIREBALL_B: turns to the target and breathes three fireballs
 * (GARUGA_MOT_FIREBALL_B) at frames 78, 120 and 164, each with the two
 * GARUGA_BONE_FIREBALL_BURST bursts and a fireball (EMRANGED_FIREBALL) with
 * its own pair of angle arguments; dust clouds every 4th frame from frame 256
 * to 316; ends the state when the motion ends (no countdown).
 */
void EmGaruga_FireballB(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        break;
    case 1:
        if (Em_TurnToTargetMotion(em, 3640, GARUGA_MOT_WALK, GARUGA_MOT_TURN_LEFT, GARUGA_MOT_TURN_RIGHT) != 0) {
            em->step++;
            em->lookAtTarget = 0;
            Em_SetMotion(em, GARUGA_MOT_FIREBALL_B, 0, 0);
        }
        break;
    case 2:
        if ((EmMotion_ReachedFrame(em, 0, 256.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 316.0f) == 0) && !(gScene.frame & 3)) {
            DustCloud_Create((Em *) em, 26, 1, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 78.0f, 0U) != 0) {
            AttackBurst_CreateAtBone(em, GARUGA_BONE_FIREBALL_BURST, 1, 0);
            AttackBurst_CreateAtBone(em, GARUGA_BONE_FIREBALL_BURST, 2, 0);
            EmRangedAttack_CreateAtBone(em, GARUGA_BONE_BREATH, EMRANGED_FIREBALL, 0, 1456, 0);
        }
        if (EmMotion_PassedFrameScaled(em, 120.0f, 0U) != 0) {
            AttackBurst_CreateAtBone(em, GARUGA_BONE_FIREBALL_BURST, 1, 0);
            AttackBurst_CreateAtBone(em, GARUGA_BONE_FIREBALL_BURST, 2, 0);
            EmRangedAttack_CreateAtBone(em, GARUGA_BONE_BREATH, EMRANGED_FIREBALL, 0, 1274, 61896);
        }
        if (EmMotion_PassedFrameScaled(em, 164.0f, 0U) != 0) {
            AttackBurst_CreateAtBone(em, GARUGA_BONE_FIREBALL_BURST, 1, 0);
            AttackBurst_CreateAtBone(em, GARUGA_BONE_FIREBALL_BURST, 2, 0);
            EmRangedAttack_CreateAtBone(em, GARUGA_BONE_BREATH, EMRANGED_FIREBALL, 0, 1092, 2549);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            em->lookAtTarget = 0;
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * GARUGA_ATK_SOMERSAULT: the back flip: GARUGA_MOT_FLIP along curve 10
 * (airborne from frame 94); once that curve ends within 372 of the floor,
 * GARUGA_MOT_FLIP_FALL along curve 11 down to the floor, then
 * GARUGA_MOT_FLIP_LAND with a dust cloud (EmGaruga_SpawnLandingDust) and a
 * camera shake; ends the state when that motion ends.
 */
void EmGaruga_Somersault(Em *em, EmWork *work)
{
    f32 floorY;
    s32 ended;

    ended = 0;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GARUGA_MOT_FLIP, 0, 0);
        EmGaruga_StartCurves(em, 10);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 94.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            ended = EmGaruga_UpdateCurves(em) & 0xFF;
        }
        if ((ended != 0) && (em->posY <= (372.0f + em->floorY))) {
            em->step++;
            Em_SetMotion(em, GARUGA_MOT_FLIP_FALL, 0, 0);
            EmGaruga_StartCurves(em, 11);
        }
        break;
    case 2:
        EmGaruga_UpdateCurves(em);
        if (em->posY <= em->floorY) {
            em->step++;
            Em_SetMotion(em, GARUGA_MOT_FLIP_LAND, 0, 0);
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            EmGaruga_SpawnLandingDust(em);
            CamShake_ReqAIf(em, 2);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * GARUGA_ATK_PECK_A: plays GARUGA_MOT_PECK_COMBO in place, without turning
 * first, and ends the state.
 */
void EmGaruga_PeckA(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GARUGA_MOT_PECK_COMBO, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * GARUGA_REACT_FLINCH: flinches (GARUGA_MOT_FLINCH_A); the script restarts
 * (Em_RestartScript); ends the reaction when the motion ends.
 */
void EmGaruga_Flinch(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, GARUGA_MOT_FLINCH_A, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGaruga_EndReactState(em, 0, 0);
        }
        break;
    }
}

/* GARUGA_REACT_FLINCH_B: flinches (GARUGA_MOT_FLINCH_B), as EmGaruga_Flinch. */
void EmGaruga_FlinchB(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, GARUGA_MOT_FLINCH_B, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGaruga_EndReactState(em, 0, 0);
        }
        break;
    }
}

/* GARUGA_REACT_FLINCH_C: flinches (GARUGA_MOT_FLINCH_C), as EmGaruga_Flinch. */
void EmGaruga_FlinchC(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, GARUGA_MOT_FLINCH_C, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGaruga_EndReactState(em, 0, 0);
        }
        break;
    }
}

/* GARUGA_REACT_FLINCH_D: flinches (GARUGA_MOT_FLINCH_D), as EmGaruga_Flinch. */
void EmGaruga_FlinchD(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, GARUGA_MOT_FLINCH_D, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGaruga_EndReactState(em, 0, 0);
        }
        break;
    }
}

/*
 * GARUGA_REACT_PART_BREAK: the once-only reaction (GARUGA_MOT_PART_BREAK,
 * unhittable while it runs: noHitTimer 5 every frame): hides mesh 1 at
 * frame 6 and turns rotY back by 936 per frame (scaled by motionSpeed)
 * between frames 52 and 122; then GARUGA_REACT_PART_BREAK_END, the host
 * retrying until the change takes.
 */
void EmGaruga_PartBreak(Em *em, EmWork *work)
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
        Em_SetMotion(em, GARUGA_MOT_PART_BREAK, 0, 0);
        break;
    case 1:
        if ((EmMotion_PassedFrameScaled(em, 6.0f, 0U) != 0) && ((s8) em->meshShown[1] != 0)) {
            em->meshShown[1] = 0;
        }
        if ((EmMotion_ReachedFrame(em, 0, 52.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 122.0f) == 0)) {
            turnF = 936.0f * em->motionSpeed;
            turn = (u32) turnF;
            em->rotY -= (s32) turn & 0xFFFF;
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            em->rotY -= 8;
            EmGaruga_ChangeState(em, EM_GROUP_REACT, GARUGA_REACT_PART_BREAK_END, EM_SYNC_OWNER);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmGaruga_ChangeState(em, EM_GROUP_REACT, GARUGA_REACT_PART_BREAK_END, EM_SYNC_OWNER);
        }
        break;
    }
}

/*
 * GARUGA_REACT_KNOCK_DOWN: is knocked down (GARUGA_MOT_KNOCK_DOWN_B), lies
 * for 120 frames (GARUGA_MOT_KNOCK_DOWN_B_LIE), gets up
 * (GARUGA_MOT_KNOCK_DOWN_B_GET_UP) and ends the state, then stores the new
 * state as prevStateGroup/prevState.
 */
void EmGaruga_KnockDown(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, GARUGA_MOT_KNOCK_DOWN_B, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 0;
            Em_SetMotion(em, GARUGA_MOT_KNOCK_DOWN_B_LIE, 0, 0);
        }
        break;
    case 2:
        em->timer++;
        if (em->timer >= 120) {
            em->step++;
            Em_SetMotion(em, GARUGA_MOT_KNOCK_DOWN_B_GET_UP, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGaruga_EndState(em, 0, 0);
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
        }
        break;
    }
}

/*
 * GARUGA_REACT_KNOCK_DOWN_PART3: the reaction to a hit on the broken part
 * 3: as EmGaruga_KnockDown with GARUGA_MOT_KNOCK_DOWN, GARUGA_MOT_DOWN (120
 * frames) and GARUGA_MOT_DOWN_GET_UP.
 */
void EmGaruga_KnockDownPart3(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, GARUGA_MOT_KNOCK_DOWN, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 0;
            Em_SetMotion(em, GARUGA_MOT_DOWN, 0, 0);
        }
        break;
    case 2:
        em->timer++;
        if (em->timer >= 120) {
            em->step++;
            Em_SetMotion(em, GARUGA_MOT_DOWN_GET_UP, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGaruga_EndState(em, 0, 0);
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
        }
        break;
    }
}

/*
 * GARUGA_REACT_FALL_THEN_GLIDE: drops in the air (GARUGA_MOT_FALL, velY at
 * most -50; script restarted) and at frame 14 catches itself and glides
 * once (EmGaruga_EndAirState 1, velY 0).
 */
void EmGaruga_FallThenGlide(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GARUGA_MOT_FALL, 0, 0);
        if (em->velY > -50.0f) {
            em->velY = -50.0f;
        }
        Em_RestartScript(em);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 14.0f, 0) != 0) {
            em->step++;
            EmGaruga_EndAirState(em, 1);
            em->velY = 0.0f;
        }
        Em_MoveRotated(em, &work->moveAngX);
        break;
    }
}

/*
 * GARUGA_REACT_FALL: falls out of the air (GARUGA_MOT_FALL, velY at most
 * -50, accelY -10; script restarted), hits the floor
 * (GARUGA_MOT_FALL_LAND), gets up (GARUGA_MOT_DOWN_GET_UP) and ends the
 * reaction.
 */
void EmGaruga_Fall(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, GARUGA_MOT_FALL, 0, 0);
        if (em->velY > -50.0f) {
            em->velY = -50.0f;
        }
        em->accelY = -10.0f;
        break;
    case 1:
        Em_MoveRotatedAccel(em, &work->moveAngX);
        if (em->posY <= em->floorY) {
            em->step++;
            em->posY = em->floorY;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, GARUGA_MOT_FALL_LAND, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, GARUGA_MOT_DOWN_GET_UP, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGaruga_EndReactState(em, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    }
}

/*
 * GARUGA_REACT_FALL_B: as EmGaruga_Fall, falling straight down along rotY
 * with accelX and accelZ cleared, and blind (sees no player) the whole
 * time.
 */
void EmGaruga_FallB(Em *em, EmWork *work)
{
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, GARUGA_MOT_FALL, 0, 0);
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
            Em_SetMotion(em, GARUGA_MOT_FALL_LAND, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, GARUGA_MOT_DOWN_GET_UP, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGaruga_EndReactState(em, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    }
}

/*
 * GARUGA_REACT_FLINCH_E: flinches (GARUGA_MOT_FLINCH_E; script restarted)
 * and ends the state (EmGaruga_EndState, not EndReactState).
 */
void EmGaruga_FlinchE(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, GARUGA_MOT_FLINCH_E, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * GARUGA_REACT_STATUS4: under status 4 (GARUGA_MOT_STATUS4, hitReactMode
 * EM_HIT_REACT_STATUS, noReact 1, Em_SpawnStatus4Effect every frame) until
 * Em.timer runs out, then GARUGA_IDLE_STATUS4_SHAKE_OFF, the host retrying
 * until the change takes.
 */
void EmGaruga_Status4(Em *em, EmWork *work)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_STATUS);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, GARUGA_MOT_STATUS4, 0, 0);
        em->noReact = 1;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            em->noReact = 0;
            EmGaruga_ChangeState(em, EM_GROUP_IDLE, GARUGA_IDLE_STATUS4_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmGaruga_ChangeState(em, EM_GROUP_IDLE, GARUGA_IDLE_STATUS4_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
    Em_SpawnStatus4Effect(em, 2);
}

/*
 * GARUGA_REACT_TRAPPED: caught in the trap (no flinch, GARUGA_MOT_TRAPPED,
 * blind cleared), keeping holdTimer at 5 or more; when the motion ends,
 * GARUGA_REACT_TRAPPED_STRUGGLE.
 */
void EmGaruga_Trapped(Em *em, EmWork *work)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_NO_FLINCH);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, GARUGA_MOT_TRAPPED, 0, 0);
        em->noReact = 1;
        em->blind = 0;
        break;
    case 1:
        if ((s8) em->holdTimer < 5) {
            em->holdTimer = 5;
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGaruga_ChangeState(em, EM_GROUP_REACT, GARUGA_REACT_TRAPPED_STRUGGLE, EM_SYNC_OWNER);
        }
        break;
    case 2:
        if ((s8) em->holdTimer < 5) {
            em->holdTimer = 5;
        }
        if (em->remote == 0) {
            EmGaruga_ChangeState(em, EM_GROUP_REACT, GARUGA_REACT_TRAPPED_STRUGGLE, EM_SYNC_OWNER);
        }
        break;
    }
}

/*
 * GARUGA_REACT_TRAPPED_STRUGGLE: held (holdTimer 5 every frame), loops
 * GARUGA_MOT_STRUGGLE until Em.trapTimer runs out, then flies out
 * (GARUGA_FLY_ESCAPE_UPWARD).
 */
void EmGaruga_TrappedStruggle(Em *em, EmWork *work)
{
    em->holdTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, GARUGA_MOT_STRUGGLE, 0, 0);
        em->noReact = 1;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            Em_SetMotion(em, GARUGA_MOT_STRUGGLE, 0, 0);
        }
        if (--em->trapTimer <= 0) {
            em->step++;
            Em_SetMotion(em, GARUGA_MOT_STRUGGLE, 0, 0);
            EmGaruga_ChangeState(em, EM_GROUP_FLY, GARUGA_FLY_ESCAPE_UPWARD, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if ((em->motion[0].playing == 0) && (em->remote == 0)) {
            Em_SetMotion(em, GARUGA_MOT_STRUGGLE, 0, 0);
            EmGaruga_ChangeState(em, EM_GROUP_FLY, GARUGA_FLY_ESCAPE_UPWARD, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * GARUGA_REACT_STATUS4_DOWNED: status 4 in a hold: lies downed
 * (GARUGA_MOT_DOWNED) with the periodic effect (Em_SpawnStatus4Effect)
 * until Em.timer runs out, then sets the trap time (Em_SetTrapTimeGoal) and
 * GARUGA_REACT_STATUS4_DOWNED_END.
 */
void EmGaruga_Status4Downed(Em *em, EmWork *work)
{
    em->holdTimer = 5;
    Em_SetHitReactMode(em, EM_HIT_REACT_STATUS);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, GARUGA_MOT_DOWNED, 0, 0);
        em->noReact = 1;
        Em_RestartScript(em);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetTrapTimeGoal(em);
            EmGaruga_ChangeState(em, EM_GROUP_REACT, GARUGA_REACT_STATUS4_DOWNED_END, EM_SYNC_OWNER);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmGaruga_ChangeState(em, EM_GROUP_REACT, GARUGA_REACT_STATUS4_DOWNED_END, EM_SYNC_OWNER);
        }
        break;
    }
    Em_SpawnStatus4Effect(em, 2);
}

/*
 * GARUGA_REACT_TRAP_ESCAPE_HOP: the answer of Update to a trap while
 * enraged and free of statuses: guarded (guardTimer 2), hops back out of it
 * (GARUGA_MOT_BACK_HOP, curve set 14 from frame 76) and creates
 * EmMine_CreateTimed0 at its position; on reaching the floor
 * GARUGA_MOT_BACK_HOP_LAND with the landing dust (EmGaruga_SpawnLandingDust)
 * and a camera shake, then ends the state. Keeps posY at or above the
 * floor.
 */
void EmGaruga_TrapEscapeHop(Em *em, EmWork *work)
{
    f32 floorY;
    s32 ended;

    ended = 0;
    em->guardTimer = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GARUGA_MOT_BACK_HOP, 0, 70);
        em->holdTimer = 50;
        EmGaruga_StartCurves(em, 14);
        EmMine_CreateTimed0(em, &em->posX);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 76.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            ended = EmGaruga_UpdateCurves(em) & 0xFF;
        }
        if ((ended != 0) && (em->posY <= em->floorY)) {
            Em_SetMotion(em, GARUGA_MOT_BACK_HOP_LAND, 0, 0);
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            EmGaruga_SpawnLandingDust(em);
            CamShake_ReqAIf(em, 2);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * GARUGA_REACT_STATUS4_DOWNED_END: as GARUGA_REACT_TRAPPED_STRUGGLE: loops
 * GARUGA_MOT_STRUGGLE until Em.trapTimer runs out, then flies out
 * (GARUGA_FLY_ESCAPE_UPWARD).
 */
void EmGaruga_Status4DownedEnd(Em *em, EmWork *work)
{
    em->holdTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, GARUGA_MOT_STRUGGLE, 0, 0);
        em->noReact = 1;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            Em_SetMotion(em, GARUGA_MOT_STRUGGLE, 0, 0);
        }
        if (--em->trapTimer <= 0) {
            em->step++;
            Em_SetMotion(em, GARUGA_MOT_STRUGGLE, 0, 0);
            EmGaruga_ChangeState(em, EM_GROUP_FLY, GARUGA_FLY_ESCAPE_UPWARD, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if ((em->motion[0].playing == 0) && (em->remote == 0)) {
            Em_SetMotion(em, GARUGA_MOT_STRUGGLE, 0, 0);
            EmGaruga_ChangeState(em, EM_GROUP_FLY, GARUGA_FLY_ESCAPE_UPWARD, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * GARUGA_REACT_STATUS2_DOWNED_END: gets up from the downed lying
 * (GARUGA_MOT_DOWNED_GET_UP, blind meanwhile), sets the trap time
 * (Em_SetTrapTimeGoal), then the trap struggle
 * (GARUGA_REACT_TRAPPED_STRUGGLE).
 */
void EmGaruga_Status2DownedEnd(Em *em, EmWork *work)
{
    em->holdTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GARUGA_MOT_DOWNED_GET_UP, 0, 0);
        em->blind = 2;
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetTrapTimeGoal(em);
            em->blind = 0;
            EmGaruga_ChangeState(em, EM_GROUP_REACT, GARUGA_REACT_TRAPPED_STRUGGLE, EM_SYNC_OWNER);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmGaruga_ChangeState(em, EM_GROUP_REACT, GARUGA_REACT_TRAPPED_STRUGGLE, EM_SYNC_OWNER);
        }
        break;
    }
}

/*
 * GARUGA_REACT_STATUS1_DOWNED_END: as GARUGA_REACT_STATUS2_DOWNED_END after
 * status 1.
 */
void EmGaruga_Status1DownedEnd(Em *em, EmWork *work)
{
    em->holdTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GARUGA_MOT_DOWNED_GET_UP, 0, 0);
        em->blind = 2;
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetTrapTimeGoal(em);
            em->blind = 0;
            EmGaruga_ChangeState(em, EM_GROUP_REACT, GARUGA_REACT_TRAPPED_STRUGGLE, EM_SYNC_OWNER);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmGaruga_ChangeState(em, EM_GROUP_REACT, GARUGA_REACT_TRAPPED_STRUGGLE, EM_SYNC_OWNER);
        }
        break;
    }
}

/*
 * GARUGA_REACT_PART_BREAK_END: restarts the script and ends the state when
 * the current motion ends.
 */
void EmGaruga_PartBreakEnd(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * GARUGA_REACT_STUN: knocked down (GARUGA_MOT_KNOCK_DOWN) with the stun
 * object (StunStars_CreateEm, lasting Em.timer plus half the motion), lies
 * (GARUGA_MOT_DOWN) until Em.timer runs out, gets up
 * (GARUGA_MOT_DOWN_GET_UP), then the shake-off
 * (GARUGA_IDLE_STUN_SHAKE_OFF).
 */
void EmGaruga_Stun(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, GARUGA_MOT_KNOCK_DOWN, 0, 0);
        StunStars_CreateEm(em, 1, 34, (s32) ((f32) em->timer + (em->motion[0].frameCount / 2.0f)), 150.0f, 3.0f);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, GARUGA_MOT_DOWN, 0, 0);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, GARUGA_MOT_DOWN_GET_UP, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGaruga_ChangeState(em, EM_GROUP_IDLE, GARUGA_IDLE_STUN_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 4:
        if (em->remote == 0) {
            EmGaruga_ChangeState(em, EM_GROUP_IDLE, GARUGA_IDLE_STUN_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * GARUGA_REACT_STUN_DOWNED: the stun in a hold: lies downed
 * (GARUGA_MOT_DOWNED) with the stun object until Em.timer runs out, then
 * sets the trap time (Em_SetTrapTimeGoal) and GARUGA_REACT_STUN_DOWNED_END.
 */
void EmGaruga_StunDowned(Em *em, EmWork *work)
{
    em->holdTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, GARUGA_MOT_DOWNED, 0, 0);
        Em_RestartScript(em);
        em->noReact = 1;
        StunStars_CreateEm(em, 1, 34, (s32) ((f32) em->timer + (em->motion[0].frameCount / 2.0f)), 150.0f, 3.0f);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetTrapTimeGoal(em);
            EmGaruga_ChangeState(em, EM_GROUP_REACT, GARUGA_REACT_STUN_DOWNED_END, EM_SYNC_OWNER);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmGaruga_ChangeState(em, EM_GROUP_REACT, GARUGA_REACT_STUN_DOWNED_END, EM_SYNC_OWNER);
        }
        break;
    }
}

/*
 * GARUGA_REACT_STUN_DOWNED_END: loops GARUGA_MOT_STRUGGLE until
 * Em.trapTimer runs out, then flies out (GARUGA_FLY_ESCAPE_UPWARD).
 */
void EmGaruga_StunDownedEnd(Em *em, EmWork *work)
{
    em->holdTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, GARUGA_MOT_STRUGGLE, 0, 0);
        em->noReact = 1;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            Em_SetMotion(em, GARUGA_MOT_STRUGGLE, 0, 0);
        }
        if (--em->trapTimer <= 0) {
            em->step++;
            EmGaruga_ChangeState(em, EM_GROUP_FLY, GARUGA_FLY_ESCAPE_UPWARD, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmGaruga_ChangeState(em, EM_GROUP_FLY, GARUGA_FLY_ESCAPE_UPWARD, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * GARUGA_REACT_HOLD2: held by the second hold (Em_SetHold2 every frame):
 * GARUGA_MOT_FLINCH_A without root motion (EM_FLAG_NO_ROOT_MOTION) to frame
 * 52, then GARUGA_MOT_STATUS4 until Em.timer runs out, then the shake-off
 * (GARUGA_IDLE_HOLD2_SHAKE_OFF).
 */
void EmGaruga_Hold2(Em *em, EmWork *work)
{
    Em_SetHold2(em);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, GARUGA_MOT_FLINCH_A, 0, 0);
        Player_SetFlag((Player *) em, EM_FLAG_NO_ROOT_MOTION);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 52.0f) != 0) {
            em->step++;
            Em_SetMotion(em, GARUGA_MOT_STATUS4, 12, 0);
            Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            EmGaruga_ChangeState(em, EM_GROUP_IDLE, GARUGA_IDLE_HOLD2_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmGaruga_ChangeState(em, EM_GROUP_IDLE, GARUGA_IDLE_HOLD2_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * GARUGA_EVENT_STORY_INTRO: the story quest scene (Update starts it in quest
 * stage QUEST_ID_GARUGA_SCENE, AREA_JUNGLE_9, while EVENT_FLAG_GARUGA_SCENE
 * is clear), untouchable throughout (senseDelay, noHitTimer): flies in
 * between fixed points (GARUGA_MOT_FLY, then GARUGA_MOT_GLIDE, each until
 * within 500 of the goal or 300 frames), hovers down and touches down
 * (GARUGA_MOT_TOUCH_DOWN), turns (GARUGA_MOT_TURN_LEFT), idles with layer
 * motions, then GARUGA_MOT_FIREBALL_B firing three fireballs
 * (EmRangedAttack_CreateAtBone with AttackBurst_CreateAtBone at frames 78,
 * 120 and 164) with dust; waits in GARUGA_MOT_ALERT_IDLE until
 * EVENT_FLAG_GARUGA_SCENE is set, then ends the state. Keeps posY at or above
 * the floor.
 */
void EmGaruga_StoryIntro(Em *em, EmWork *work)
{
    f32 dist;
    f32 hoverDist;
    f32 turnF;
    f32 floorY;
    u32 turn;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        em->posX = 10051.0f;
        em->posY = 1040.0f;
        em->posZ = 8395.0f;
        em->goal[0] = 12865.0f;
        em->goal[1] = 1040.0f;
        em->goal[2] = 9646.0f;
        em->rotX = 0;
        em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        em->rotZ = 0;
        GmEm_ResetSize(em);
        Em_SetMotion(em, GARUGA_MOT_FLY, 0, 0);
        Player_ClearVelocity(em);
        em->velZ = 50.0f;
        work->moveAngX = 0;
        work->moveAngZ = 0;
        em->timer = 300;
        em->fieldCollide = 0;
        break;
    case 1:
        work->moveAngY = em->rotY;
        Em_MoveRotated(em, &work->moveAngX);
        dist = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer <= 0 || dist <= 500.0f) {
            em->step++;
            em->posX = 11375.0f;
            em->posY = 1840.0f;
            em->posZ = 8983.0f;
            em->goal[0] = 12865.0f;
            em->goal[1] = 1840.0f;
            em->goal[2] = 9646.0f;
            em->rotX = 0;
            em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
            em->rotZ = 0;
            Em_SetMotion(em, GARUGA_MOT_GLIDE, 0, 0);
            Player_ClearVelocity(em);
            em->velZ = 50.0f;
            work->moveAngX = 0;
            work->moveAngZ = 0;
            em->timer = 300;
            em->fieldCollide = 0;
        }
        break;
    case 2:
        work->moveAngY = em->rotY;
        Em_MoveRotated(em, &work->moveAngX);
        hoverDist = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer <= 0 || hoverDist <= 500.0f) {
            em->step++;
            em->fieldCollide = 1;
            Em_SetMotion(em, GARUGA_MOT_HOVER, 0, 0);
            Player_ClearVelocity(em);
            em->velY = -8.0f;
        }
        break;
    case 3:
        work->moveAngY = em->rotY;
        Em_MoveRotated(em, &work->moveAngX);
        if (!(em->floorY <= em->posY)) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, GARUGA_MOT_TOUCH_DOWN, 0, 0);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, GARUGA_MOT_TURN_LEFT, 0, 0);
            em->timer = 0;
            em->posX = 12426.0f;
            em->posY = 1040.0f;
            em->posZ = 9451.0f;
            em->goal[0] = 10051.0f;
            em->goal[1] = 1040.0f;
            em->goal[2] = 8395.0f;
            em->rotX = 0;
            em->rotY = (Math_AngleXZ(&em->posX, em->goal) & 0xFFFF) - ANGLE_TURN / 4;
            em->rotZ = 0;
        }
        break;
    case 5:
        turnF = (16384.0f / (em->motion[0].frameCount / 2.0f)) * em->motionSpeed;
        turn = (u32) turnF;
        em->rotY += (s32) turn;
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, GARUGA_MOT_IDLE, 0, 0);
            em->timer = 0;
        }
        break;
    case 6:
        em->timer++;
        if (em->timer == 40) {
            Em_SetLayerMotion(em, EM_MOTION_ID(1, GARUGA_MOT_IDLE_ACTION_J), 10, 0, 1);
        }
        if (em->timer == 10) {
            Em_SetLayerMotion(em, EM_MOTION_ID(2, 27), 10, 0, 2);
        }
        if (em->motion[1].playing == 0) {
            em->step++;
            Em_SetMotion(em, GARUGA_MOT_ALERT_IDLE, 0, 0);
            em->timer = 1;
        }
        break;
    case 7:
        if (em->motion[0].playing == 0) {
            if (--em->timer <= 0) {
                em->step++;
                Em_SetMotion(em, GARUGA_MOT_FIREBALL_B, 0, 0);
            }
        }
        break;
    case 8:
        if ((EmMotion_ReachedFrame(em, 0, 256.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 316.0f) == 0) && !(gScene.frame & 3)) {
            DustCloud_Create((Em *) em, 26, 1, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 78.0f, 0U) != 0) {
            AttackBurst_CreateAtBone(em, GARUGA_BONE_FIREBALL_BURST, 1, 0);
            AttackBurst_CreateAtBone(em, GARUGA_BONE_FIREBALL_BURST, 2, 0);
            EmRangedAttack_CreateAtBone(em, GARUGA_BONE_BREATH, EMRANGED_FIREBALL, 0, 1456, 0);
        }
        if (EmMotion_PassedFrameScaled(em, 120.0f, 0U) != 0) {
            AttackBurst_CreateAtBone(em, GARUGA_BONE_FIREBALL_BURST, 1, 0);
            AttackBurst_CreateAtBone(em, GARUGA_BONE_FIREBALL_BURST, 2, 0);
            EmRangedAttack_CreateAtBone(em, GARUGA_BONE_BREATH, EMRANGED_FIREBALL, 0, 1274, 61896);
        }
        if (EmMotion_PassedFrameScaled(em, 164.0f, 0U) != 0) {
            AttackBurst_CreateAtBone(em, GARUGA_BONE_FIREBALL_BURST, 1, 0);
            AttackBurst_CreateAtBone(em, GARUGA_BONE_FIREBALL_BURST, 2, 0);
            EmRangedAttack_CreateAtBone(em, GARUGA_BONE_BREATH, EMRANGED_FIREBALL, 0, 1092, 2549);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, GARUGA_MOT_ALERT_IDLE, 0, 0);
        }
        break;
    case 9:
        if (Save_TestEventFlag(EVENT_FLAG_GARUGA_SCENE) == 1) {
            em->step++;
            em->senseDelay = 0;
            em->noHitTimer = 0;
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * GARUGA_EVENT_CAPTURED: the capture while held (Em_SetCaptured with 0):
 * lies downed (GARUGA_MOT_DOWNED) under status 1, poison cleared; when the
 * motion ends, the capture (QuestEm_Captured); once QuestEm_CanAppear
 * allows, 1800 frames, then fades out over 150 frames and reappears
 * (GARUGA_DEFEAT_REAPPEAR). Until the fade it makes the lying puffs
 * (Em_SpawnLyingPuffs) and a sound every 135 frames.
 */
void EmGaruga_Captured(Em *em)
{
    f32 pos[3];

    Em_SetCaptured(em, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->stateVar0 = 0;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GARUGA_MOT_DOWNED, 0, 0);
        Em_LoadReactionSet(em, 1);
        Em_BeginStatus1(em);
        Em_ClearPoison(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            QuestEm_Captured(em);
            em->timer = 1800;
        }
        break;
    case 2:
        if (QuestEm_CanAppear(em) != 0) {
            if (--em->timer <= 0) {
                em->step++;
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
                EmGaruga_ChangeState(em, EM_GROUP_DEFEAT, GARUGA_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    }
    if (em->step < 3) {
        pos[0] = 0.0f;
        pos[1] = 10.0f;
        pos[2] = 140.0f;
        Em_SpawnLyingPuffs(em, GARUGA_BONE_BREATH, pos, 1.6f);
        if (++em->stateVar0 % 135 == 0) {
            Em_PlaySeAtBone(em, 87, GARUGA_BONE_HEAD);
        }
    }
}

/*
 * GARUGA_EVENT_CAPTURED_HOLD2: the capture in the second hold
 * (Em_SetCaptured with 1): collapses (GARUGA_MOT_COLLAPSE) under status 1,
 * lies (GARUGA_MOT_LIE) and is captured, then waits, fades and reappears as
 * GARUGA_EVENT_CAPTURED (without the puffs and sound).
 */
void EmGaruga_CapturedHold2(Em *em)
{
    s32 timer;
    s32 timeLeft;

    Em_SetCaptured(em, 1);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GARUGA_MOT_COLLAPSE, 0, 0);
        Em_LoadReactionSet(em, 1);
        Em_BeginStatus1(em);
        Em_ClearPoison(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, GARUGA_MOT_LIE, 0, 0);
            QuestEm_Captured(em);
            em->timer = 1800;
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
        timeLeft = em->timer;
        if (timeLeft > 0) {
            em->timer = timeLeft - 1;
        }
        timer = em->timer;
        if (timer <= 0) {
            if (em->remote == 0) {
                EmGaruga_ChangeState(em, EM_GROUP_DEFEAT, GARUGA_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
                break;
            }
        } else {
            em->alpha = (f32) timer / 150.0f;
        }
        break;
    }
}

/*
 * GARUGA_VS_EAT_BAIT: at the end of the versus match, after the defeat
 * camera (Versus_DefeatCamera), GARUGA_MOT_VERSUS_EAT_START with the bait
 * thrown at frame 10 from the player of its side (VersusBait_Create), then
 * lowers the head and eats (GARUGA_MOT_HEAD_DOWN, GARUGA_MOT_FEED), idles
 * 30 frames, then routes the monster by team (Versus_RouteEmByTeam).
 * Untouchable meanwhile.
 */
void EmGaruga_VersusEatBait(Em *em, EmWork *work)
{
    Vec3f throwPoint;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        if (Versus_DefeatCamera(em) == 1) {
            em->step++;
            Em_SetMotion(em, GARUGA_MOT_VERSUS_EAT_START, 0, 0);
            Em_SetLayerMotion(em, EM_MOTION_ID(0, GARUGA_MOT_IDLE), 0, 0, 0);
            Em_SetLayerMotion(em, EM_MOTION_ID(0, GARUGA_MOT_IDLE), 0, 0, 2);
            em->timer = 30;
        }
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0) {
            throwPoint = EmGaruga_CarveThrowPt;
            VersusBait_Create(&gPlayers[(u8) em->side - 1], em, 0, 0, &throwPoint.x);
        }
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, GARUGA_MOT_HEAD_DOWN, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, GARUGA_MOT_FEED, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 30;
            Em_SetMotion(em, GARUGA_MOT_ALERT_IDLE, 0, 0);
        }
        break;
    case 4:
        if (--em->timer <= 0) {
            Versus_RouteEmByTeam(em);
        }
        break;
    }
}

/*
 * GARUGA_VS_POSE_A: GARUGA_MOT_FLINCH_E, then idles and waits for both
 * sides (GARUGA_VS_WAIT_BOTH).
 */
void EmGaruga_VersusPoseA(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, GARUGA_MOT_FLINCH_E, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, GARUGA_MOT_IDLE, 0, 0);
            EmGaruga_ChangeState(em, EM_GROUP_VERSUS, GARUGA_VS_WAIT_BOTH, EM_SYNC_SEND);
        }
        break;
    }
}

/*
 * GARUGA_VS_POSE_B: GARUGA_MOT_TURN_IN_PLACE, then idles and waits for both
 * sides (GARUGA_VS_WAIT_BOTH).
 */
void EmGaruga_VersusPoseB(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, GARUGA_MOT_TURN_IN_PLACE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, GARUGA_MOT_IDLE, 0, 0);
            EmGaruga_ChangeState(em, EM_GROUP_VERSUS, GARUGA_VS_WAIT_BOTH, EM_SYNC_SEND);
        }
        break;
    }
}

/*
 * GARUGA_VS_POSE_C: the back flip (GARUGA_MOT_FLIP with curve set 10 from
 * frame 94, GARUGA_MOT_FLIP_FALL with set 11 once within 372 of the floor,
 * GARUGA_MOT_FLIP_LAND with the landing dust and a camera shake), then
 * idles and waits for both sides (GARUGA_VS_WAIT_BOTH).
 */
void EmGaruga_VersusPoseC(Em *em, EmWork *work)
{
    f32 floorY;
    s32 ended;
    u8 step;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    step = em->step;
    ended = 0;
    switch (step) {
    case 0:
        em->step = step + 1;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GARUGA_MOT_FLIP, 0, 0);
        EmGaruga_StartCurves(em, 10);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 94.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            ended = EmGaruga_UpdateCurves(em) & 0xFF;
        }
        if ((ended != 0) && (em->posY <= (372.0f + em->floorY))) {
            em->step++;
            Em_SetMotion(em, GARUGA_MOT_FLIP_FALL, 0, 0);
            EmGaruga_StartCurves(em, 11);
        }
        break;
    case 2:
        EmGaruga_UpdateCurves(em);
        if (em->posY <= em->floorY) {
            em->step++;
            Em_SetMotion(em, GARUGA_MOT_FLIP_LAND, 0, 0);
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            EmGaruga_SpawnLandingDust(em);
            CamShake_ReqAIf(em, 2);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step = step + 1;
            Em_SetMotion(em, GARUGA_MOT_IDLE, 0, 0);
            EmGaruga_ChangeState(em, EM_GROUP_VERSUS, GARUGA_VS_WAIT_BOTH, EM_SYNC_SEND);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * GARUGA_VS_WAIT_BOTH: idles until both sides are defeated
 * (Versus_BothDefeated), then GARUGA_VS_END.
 */
void EmGaruga_VersusWaitBoth(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        if (em->motionId[0] != EM_MOTION_ID(0, GARUGA_MOT_IDLE)) {
            Em_SetMotion(em, GARUGA_MOT_IDLE, 0, 0);
        }
        
    case 1:
        if (Versus_BothDefeated() == 1) {
            em->step++;
            EmGaruga_ChangeState(em, EM_GROUP_VERSUS, GARUGA_VS_END, EM_SYNC_SEND);
        }
        break;
    }
}

/*
 * GARUGA_VS_END: idles 90 frames, makes it touchable again (senseDelay,
 * noHitTimer 0) and ends the state.
 */
void EmGaruga_VersusEnd(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 90;
        if (em->motionId[0] != EM_MOTION_ID(0, GARUGA_MOT_IDLE)) {
            Em_SetMotion(em, GARUGA_MOT_IDLE, 0, 0);
        }
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            em->senseDelay = 0;
            em->noHitTimer = 0;
            EmGaruga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * GARUGA_DEFEAT_DIE: falls dead (GARUGA_MOT_DIE, QuestEm_Hunted, a camera
 * shake at frame 212) and leaves a carving mining point (EmMine_Create)
 * while it lies; once QuestEm_CanAppear allows, after 1800 frames removes
 * it, fades out over 150 frames and reappears (GARUGA_DEFEAT_REAPPEAR).
 * Step 99 ends the state on the ground once the motion ends.
 */
void EmGaruga_Die(Em *em, EmWork *work)
{
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, GARUGA_MOT_DIE, 0, 0);
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_DEAD;
        QuestEm_Hunted(em);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 212.0f, 0) != 0) {
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
        if (QuestEm_CanAppear(em) != 0) {
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
                EmGaruga_ChangeState(em, EM_GROUP_DEFEAT, GARUGA_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    case 99:
        if (em->motion[0].playing == 0) {
            em->stance = EM_STANCE_GROUND;
            EmGaruga_EndState(em, 0, 0);
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
        }
        break;
    }
}

/*
 * GARUGA_DEFEAT_DIE_HELD: dies lying downed in a hold (GARUGA_MOT_DOWNED,
 * holdTimer kept at 5 until the fade), as GARUGA_DEFEAT_DIE.
 */
void EmGaruga_DieHeld(Em *em, EmWork *work)
{
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, GARUGA_MOT_DOWNED, 0, 0);
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_DEAD;
        QuestEm_Hunted(em);
        em->holdTimer = 5;
        break;
    case 1:
        em->holdTimer = 5;
        if (em->motion[0].playing == 0) {
            em->step++;
            EmMine_Create(em, 0);
            em->fieldCollide = 0;
            em->timer = 1800;
        }
        break;
    case 2:
        EmMine_Update(em);
        em->holdTimer = 5;
        if (QuestEm_CanAppear(em) != 0) {
            if (--em->timer <= 0) {
                em->step++;
                em->holdTimer = 0;
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
                EmGaruga_ChangeState(em, EM_GROUP_DEFEAT, GARUGA_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    case 99:
        if (em->motion[0].playing == 0) {
            em->stance = EM_STANCE_GROUND;
            EmGaruga_EndState(em, 0, 0);
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
        }
        break;
    }
}

/*
 * GARUGA_DEFEAT_DIE_FALLING: dies in the air: falls (GARUGA_MOT_FALL, velY
 * at most -50, accelY -10) to the floor, GARUGA_MOT_FALL_LAND, at its frame
 * 60 GARUGA_MOT_DIE_FALLING and QuestEm_Hunted, then lies as
 * GARUGA_DEFEAT_DIE.
 */
void EmGaruga_DieFalling(Em *em, EmWork *work)
{
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GARUGA_MOT_FALL, 0, 0);
        if (em->velY > -50.0f) {
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
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, GARUGA_MOT_FALL_LAND, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    case 2:
        if (EmMotion_PassedFrameScaled(em, 60.0f, 0) != 0) {
            em->step++;
            em->timer = 0;
            Em_SetMotion(em, GARUGA_MOT_DIE_FALLING, 6, 0);
            em->stance = EM_STANCE_DEAD;
            QuestEm_Hunted(em);
        }
        break;
    case 3:
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
        if (em->timer <= 0) {
            if (em->remote == 0) {
                EmGaruga_ChangeState(em, EM_GROUP_DEFEAT, GARUGA_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    case 99:
        if (em->motion[0].playing == 0) {
            em->stance = EM_STANCE_GROUND;
            EmGaruga_EndState(em, 0, 0);
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
        }
        break;
    }
}

/*
 * GARUGA_DEFEAT_REAPPEAR: when QuestEm_CanAppear allows, puts itself in the
 * air at the ground height of the area (gAreaGroundY) and lands
 * (GARUGA_FLY_LAND); otherwise advances Em.taskPhase (the next phase of
 * GmEm_Task).
 */
void EmGaruga_Reappear(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        if (QuestEm_CanAppear(em) == 1) {
            em->posY = gAreaGroundY[em->area];
            em->stance = EM_STANCE_AIR;
            EmGaruga_ChangeState(em, EM_GROUP_FLY, GARUGA_FLY_LAND, EM_SYNC_LOCAL);
            break;
        }
        em->taskPhase++;
        break;
    }
}

/*
 * Runs the current group-0 state (EM_GROUP_IDLE). The three shake-offs
 * share EmGaruga_ShakeOffAfterStatus (kind 0 status 4, 1 stun, 2 second
 * hold) and the three stomp states EmGaruga_Stomps.
 */
void EmGaruga_RunIdleState(Em *em, EmWork *work)
{
    switch (em->state) {
    case GARUGA_IDLE_STAND:
        EmGaruga_Stand(em);
        break;
    case GARUGA_IDLE:
        EmGaruga_Idle(em);
        break;
    case GARUGA_IDLE_STEP_PECK:
        EmGaruga_IdleStepPeck(em);
        break;
    case GARUGA_IDLE_PECK:
        EmGaruga_IdlePeck(em);
        break;
    case GARUGA_IDLE_ACTION_C:
        EmGaruga_IdleActionC(em);
        break;
    case GARUGA_IDLE_SKIP_A:
        EmGaruga_IdleSkipA(em);
        break;
    case GARUGA_IDLE_SKIP_B:
        EmGaruga_IdleSkipB(em);
        break;
    case GARUGA_IDLE_SKIP_C:
        EmGaruga_IdleSkipC(em);
        break;
    case GARUGA_IDLE_HOLD2_SHAKE_OFF:
        EmGaruga_ShakeOffAfterStatus(em, work, 2);
        break;
    case GARUGA_IDLE_PAIR_A:
        EmGaruga_IdlePairA(em);
        break;
    case GARUGA_IDLE_TURN_TO_GOAL:
        EmGaruga_IdleTurnToGoal(em);
        break;
    case GARUGA_IDLE_ACTION_H:
        EmGaruga_IdleActionH(em);
        break;
    case GARUGA_IDLE_ACTION_I:
        EmGaruga_IdleActionI(em);
        break;
    case GARUGA_IDLE_ACTION_J:
        EmGaruga_IdleActionJ(em);
        break;
    case GARUGA_IDLE_SKIP_D:
        EmGaruga_IdleSkipD(em);
        break;
    case GARUGA_IDLE_PAIR_B:
        EmGaruga_IdlePairB(em);
        break;
    case GARUGA_IDLE_PAIR_C:
        EmGaruga_IdlePairC(em);
        break;
    case GARUGA_IDLE_ALERT:
        EmGaruga_AlertIdle(em);
        break;
    case GARUGA_IDLE_REST:
        EmGaruga_Rest(em);
        break;
    case GARUGA_IDLE_EAT_START:
        EmGaruga_EatStart(em);
        break;
    case GARUGA_IDLE_STATUS2_COLLAPSE:
        EmGaruga_Status2Collapse(em);
        break;
    case GARUGA_IDLE_STATUS2_REGEN:
        EmGaruga_Status2Regen(em);
        break;
    case GARUGA_IDLE_ACTION_K:
        EmGaruga_IdleActionK(em);
        break;
    case GARUGA_IDLE_REST_END:
        EmGaruga_RestEnd(em);
        break;
    case GARUGA_IDLE_STATUS2_END:
        EmGaruga_Status2End(em);
        break;
    case GARUGA_IDLE_SHAKE_OFF:
        EmGaruga_ShakeOff(em);
        break;
    case GARUGA_IDLE_SKIP_E:
        EmGaruga_IdleSkipE(em);
        break;
    case GARUGA_IDLE_STATUS1_COLLAPSE:
        EmGaruga_Status1Collapse(em);
        break;
    case GARUGA_IDLE_STATUS1_END:
        EmGaruga_Status1End(em);
        break;
    case GARUGA_IDLE_STATUS2_DOWNED:
        EmGaruga_Status2Downed(em, work);
        break;
    case GARUGA_IDLE_STATUS1_DOWNED:
        EmGaruga_Status1Downed(em, work);
        break;
    case GARUGA_IDLE_STOMPS:
        EmGaruga_Stomps(em);
        break;
    case GARUGA_IDLE_STATUS4_SHAKE_OFF:
        EmGaruga_ShakeOffAfterStatus(em, work, 0);
        break;
    case GARUGA_IDLE_STOMPS_RAGE_BY_HP:
        EmGaruga_Stomps(em);
        break;
    case GARUGA_IDLE_STUN_SHAKE_OFF:
        EmGaruga_ShakeOffAfterStatus(em, work, 1);
        break;
    case GARUGA_IDLE_STOMPS_PENDING_RAGE:
        EmGaruga_Stomps(em);
        break;
    case GARUGA_IDLE_EAT:
        EmGaruga_Eat(em);
        break;
    }
}

/*
 * Runs the current group-1 state (EM_GROUP_MOVE); GARUGA_MOVE_RUN_SLOW runs
 * EmGaruga_Run.
 */
void EmGaruga_RunMoveState(Em *em, EmWork *work)
{
    switch (em->state) {
    case GARUGA_MOVE_WALK:
        EmGaruga_Walk(em);
        break;
    case GARUGA_MOVE_END:
        EmGaruga_MoveEnd(em);
        break;
    case GARUGA_MOVE_RUN_B:
        EmGaruga_RunB(em);
        break;
    case GARUGA_MOVE_TURN:
        EmGaruga_Turn(em);
        break;
    case GARUGA_MOVE_RUN:
        EmGaruga_Run(em);
        break;
    case GARUGA_MOVE_TURN_IN_COMBAT:
        EmGaruga_TurnInCombat(em);
        break;
    case GARUGA_MOVE_LIMP:
        EmGaruga_Limp(em);
        break;
    case GARUGA_MOVE_TURN_TO_GOAL:
        EmGaruga_TurnToGoal(em, work);
        break;
    case GARUGA_MOVE_RUN_SLOW:
        EmGaruga_Run(em);
        break;
    case GARUGA_MOVE_NOP:
        EmGaruga_MoveNop();
        break;
    case GARUGA_MOVE_RUN_B_AND_TURN:
        EmGaruga_RunBAndTurn(em);
        
    default:
        break;
    }
}

/* Runs the current group-2 state (EM_GROUP_FLY). */
void EmGaruga_RunFlyState(Em *em, EmWork *work)
{
    switch (em->state) {
    case GARUGA_FLY_TAKE_OFF:
        EmGaruga_TakeOff(em, work);
        break;
    case GARUGA_FLY_LAND:
        EmGaruga_Land(em, work);
        break;
    case GARUGA_FLY_HOVER:
        EmGaruga_Hover(em, work);
        break;
    case GARUGA_FLY_HOP_TO_GOAL:
        EmGaruga_HopToGoal(em, work);
        break;
    case GARUGA_FLY_HOP_FORWARD:
        EmGaruga_HopForward(em, work);
        break;
    case GARUGA_FLY_HOP_BACK:
        EmGaruga_HopBack(em, work);
        break;
    case GARUGA_FLY_ROUTE:
        EmGaruga_FlyRoute(em, work);
        break;
    case GARUGA_FLY_HOVER_TO_GOAL:
        EmGaruga_HoverToGoal(em, work);
        break;
    case GARUGA_FLY_OUT_OF_AREA:
        EmGaruga_FlyOutOfArea(em, work);
        break;
    case GARUGA_FLY_TO_GOAL_AND_LAND:
        EmGaruga_FlyToGoalAndLand(em, work);
        break;
    case GARUGA_FLY_END_FLIGHT_TO_GLIDE:
        EmGaruga_EndFlightToGlide(em);
        break;
    case GARUGA_FLY_GLIDE_TO_GOAL:
        EmGaruga_GlideToGoal(em, work);
        break;
    case GARUGA_FLY_BACK_HOP:
        EmGaruga_BackHop(em);
        break;
    case GARUGA_FLY_INTO_AREA:
        EmGaruga_FlyIntoArea(em, work);
        break;
    case GARUGA_FLY_HOVER_ONCE:
        EmGaruga_HoverOnce(em, work);
        break;
    case GARUGA_FLY_GLIDE_ONCE:
        EmGaruga_GlideOnce(em, work);
        break;
    case GARUGA_FLY_HOVER_TURN:
        EmGaruga_HoverTurn(em, work);
        break;
    case GARUGA_FLY_ROUTE_B:
        EmGaruga_FlyRouteB(em, work);
        break;
    case GARUGA_FLY_LEVEL_OUT_AND_LAND:
        EmGaruga_LevelOutAndLand(em, work);
        break;
    case GARUGA_FLY_GLIDE_TO_GOAL_B:
        EmGaruga_GlideToGoalB(em, work);
        break;
    case GARUGA_FLY_HOP_IN_PLACE:
        EmGaruga_HopInPlace(em, work);
        break;
    case GARUGA_FLY_HOVER_STRAIGHT_TO_GOAL:
        EmGaruga_HoverStraightToGoal(em, work);
        break;
    case GARUGA_FLY_LEVEL_OUT:
        EmGaruga_LevelOut(em, work);
        break;
    case GARUGA_FLY_ESCAPE_UPWARD:
        EmGaruga_EscapeUpward(em, work);
        break;
    case GARUGA_FLY_HOVER_RISE:
        EmGaruga_HoverRise(em, work);
        
    default:
        break;
    }
}

/*
 * Runs the current group-3 state (EM_GROUP_ATTACK); both tail spins run
 * EmGaruga_TailSpin.
 */
void EmGaruga_RunAttackState(Em *em, EmWork *work)
{
    switch (em->state) {
    case GARUGA_ATK_TURN_AND_STARE:
        EmGaruga_TurnAndStare(em, work);
        break;
    case GARUGA_ATK_STARE:
        EmGaruga_Stare(em, work);
        break;
    case GARUGA_ATK_TAIL_SPIN:
        EmGaruga_TailSpin(em, work);
        break;
    case GARUGA_ATK_TURN_AND_PECK_A:
        EmGaruga_TurnAndPeckA(em, work);
        break;
    case GARUGA_ATK_CHARGE_TRIP:
        EmGaruga_ChargeTrip(em, work);
        break;
    case GARUGA_ATK_TURN_AND_PECK_B:
        EmGaruga_TurnAndPeckB(em, work);
        break;
    case GARUGA_ATK_ROAR:
        EmGaruga_Roar(em, work);
        break;
    case GARUGA_ATK_FIREBALL:
        EmGaruga_Fireball(em, work);
        break;
    case GARUGA_ATK_FIREBALL_LOW:
        EmGaruga_FireballLow(em, work);
        break;
    case GARUGA_ATK_FIREBALL_HIGH:
        EmGaruga_FireballHigh(em, work);
        break;
    case GARUGA_ATK_FIREBALL_B:
        EmGaruga_FireballB(em, work);
        break;
    case GARUGA_ATK_SOMERSAULT:
        EmGaruga_Somersault(em, work);
        break;
    case GARUGA_ATK_TAIL_SPIN_B:
        EmGaruga_TailSpin(em, work);
        break;
    case GARUGA_ATK_PECK_A:
        EmGaruga_PeckA(em, work);
        
    default:
        break;
    }
}

/* Runs the current group-4 state (EM_GROUP_REACT). */
void EmGaruga_RunReactState(Em *em, EmWork *work)
{
    switch (em->state) {
    case GARUGA_REACT_FLINCH:
        EmGaruga_Flinch(em, work);
        break;
    case GARUGA_REACT_FLINCH_B:
        EmGaruga_FlinchB(em, work);
        break;
    case GARUGA_REACT_FLINCH_C:
        EmGaruga_FlinchC(em, work);
        break;
    case GARUGA_REACT_FLINCH_D:
        EmGaruga_FlinchD(em, work);
        break;
    case GARUGA_REACT_PART_BREAK:
        EmGaruga_PartBreak(em, work);
        break;
    case GARUGA_REACT_KNOCK_DOWN:
        EmGaruga_KnockDown(em, work);
        break;
    case GARUGA_REACT_KNOCK_DOWN_PART3:
        EmGaruga_KnockDownPart3(em, work);
        break;
    case GARUGA_REACT_FALL_THEN_GLIDE:
        EmGaruga_FallThenGlide(em, work);
        break;
    case GARUGA_REACT_FALL:
        EmGaruga_Fall(em, work);
        break;
    case GARUGA_REACT_FALL_B:
        EmGaruga_FallB(em, work);
        break;
    case GARUGA_REACT_FLINCH_E:
        EmGaruga_FlinchE(em, work);
        break;
    case GARUGA_REACT_STATUS4:
        EmGaruga_Status4(em, work);
        break;
    case GARUGA_REACT_TRAPPED:
        EmGaruga_Trapped(em, work);
        break;
    case GARUGA_REACT_TRAPPED_STRUGGLE:
        EmGaruga_TrappedStruggle(em, work);
        break;
    case GARUGA_REACT_STATUS4_DOWNED:
        EmGaruga_Status4Downed(em, work);
        break;
    case GARUGA_REACT_TRAP_ESCAPE_HOP:
        EmGaruga_TrapEscapeHop(em, work);
        break;
    case GARUGA_REACT_STATUS4_DOWNED_END:
        EmGaruga_Status4DownedEnd(em, work);
        break;
    case GARUGA_REACT_STATUS2_DOWNED_END:
        EmGaruga_Status2DownedEnd(em, work);
        break;
    case GARUGA_REACT_STATUS1_DOWNED_END:
        EmGaruga_Status1DownedEnd(em, work);
        break;
    case GARUGA_REACT_PART_BREAK_END:
        EmGaruga_PartBreakEnd(em, work);
        break;
    case GARUGA_REACT_STUN:
        EmGaruga_Stun(em, work);
        break;
    case GARUGA_REACT_STUN_DOWNED:
        EmGaruga_StunDowned(em, work);
        break;
    case GARUGA_REACT_STUN_DOWNED_END:
        EmGaruga_StunDownedEnd(em, work);
        break;
    case GARUGA_REACT_HOLD2:
        EmGaruga_Hold2(em, work);
        
    default:
        break;
    }
}

/*
 * Runs the current group-5 state (EM_GROUP_DEFEAT), keeping noPushTimer at
 * 5.
 */
void EmGaruga_RunDefeatState(Em *em, EmWork *work)
{
    em->noPushTimer = 5;
    switch (em->state) {
    case GARUGA_DEFEAT_DIE:
        EmGaruga_Die(em, work);
        break;
    case GARUGA_DEFEAT_DIE_HELD:
        EmGaruga_DieHeld(em, work);
        break;
    case GARUGA_DEFEAT_DIE_FALLING:
        EmGaruga_DieFalling(em, work);
        break;
    case GARUGA_DEFEAT_REAPPEAR:
        EmGaruga_Reappear(em, work);
        break;
    }
}

/*
 * Runs the current group-6 or group-7 state (EM_GROUP_EVENT,
 * EM_GROUP_EVENT2); work is the work area passed as an s32.
 */
void EmGaruga_RunEventState(Em *em, s32 work)
{
    switch (em->state) {
    case GARUGA_EVENT_STORY_INTRO:
        EmGaruga_StoryIntro(em, (EmWork *) work);
        break;
    case GARUGA_EVENT_CAPTURED:
        EmGaruga_Captured(em);
        break;
    case GARUGA_EVENT_CAPTURED_HOLD2:
        EmGaruga_CapturedHold2(em);
        break;
    }
}

/* Runs the current group-8 state (EM_GROUP_VERSUS). */
void EmGaruga_RunVersusState(Em *em, EmWork *work)
{
    switch (em->state) {
    case GARUGA_VS_EAT_BAIT:
        EmGaruga_VersusEatBait(em, work);
        break;
    case GARUGA_VS_POSE_A:
        EmGaruga_VersusPoseA(em, work);
        break;
    case GARUGA_VS_POSE_B:
        EmGaruga_VersusPoseB(em, work);
        break;
    case GARUGA_VS_POSE_C:
        EmGaruga_VersusPoseC(em, work);
        break;
    case GARUGA_VS_WAIT_BOTH:
        EmGaruga_VersusWaitBoth(em, work);
        break;
    case GARUGA_VS_END:
        EmGaruga_VersusEnd(em, work);
        break;
    }
}

/*
 * The family frame: the combat timer and area trigger, counts down fxMotion
 * of the work area (frames since landing), then the result of
 * Em_UpdateDamage picks a state: death (GARUGA_DEFEAT_DIE_FALLING in the
 * air, DIE_HELD while held, else DIE); a trap (GARUGA_REACT_TRAP_ESCAPE_HOP
 * when enraged without a status, else GARUGA_REACT_TRAPPED); the stun,
 * status 4, status 1 and 2 and the captures, by hold; a hit while lying
 * ends the rest or status state; flinches by flinchPart and the state of
 * that part (GARUGA_REACT_FALL in the air). It then breaks parts by their
 * state, starts the story intro in quest stage QUEST_ID_GARUGA_SCENE
 * (GARUGA_EVENT_STORY_INTRO) or else runs the pending script, and runs the
 * current state (twice when a state was requested). In flight states with
 * flyGuard set it keeps guardTimer at 2.
 */
void EmGaruga_Update(Em *em)
{
    u8 buf[4];
    EmFlyerWork *w = (EmFlyerWork *)&em->work;

    Em_UpdateCombatTimer(em);
    Em_RaisePlayerAreaTriggerLax(em);
    if (em->noAlert != 1 && em->rage == 0) {
        Map_DayId(gScene.map);
    }
    if (w->fxMotion != 0) {
        w->fxMotion -= 1;
    }
    switch (Em_UpdateDamage(em, buf) & 0xFF) {
    case EM_DMG_NONE:
    case EM_DMG_FLASH:
    case EM_DMG_HIT:
    case EM_DMG_ATTACK5:
        break;
    case EM_DMG_KILLED:
    case EM_DMG_DEAD:
        if (em->stance == EM_STANCE_AIR) {
            EmGaruga_ChangeState(em, EM_GROUP_DEFEAT, GARUGA_DEFEAT_DIE_FALLING, EM_SYNC_OWNER);
        } else if ((s8) em->holdTimer != 0) {
            EmGaruga_ChangeState(em, EM_GROUP_DEFEAT, GARUGA_DEFEAT_DIE_HELD, EM_SYNC_OWNER);
        } else {
            EmGaruga_ChangeState(em, EM_GROUP_DEFEAT, GARUGA_DEFEAT_DIE, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_ATTACK9:
    case EM_DMG_ATTACK6:
        if ((s8) em->holdTimer == 0) {
            if (em->rage != 0 && em->status == EM_STATUS_NONE) {
                EmGaruga_ChangeState(em, EM_GROUP_REACT, GARUGA_REACT_TRAP_ESCAPE_HOP, EM_SYNC_OWNER);
            } else {
                Em_SetTrapTime(em, buf);
                Em_SetTrapTimeGoal(em);
                EmGaruga_ChangeState(em, EM_GROUP_REACT, GARUGA_REACT_TRAPPED, EM_SYNC_OWNER);
            }
        }
        break;
    case EM_DMG_STATUS5:
        if ((s8) em->holdTimer != 0) {
            Em_SetStatus5Timer(em);
            EmGaruga_ChangeState(em, EM_GROUP_REACT, GARUGA_REACT_STUN_DOWNED, EM_SYNC_OWNER);
        } else if ((em->stateGroup != EM_GROUP_REACT || em->state != GARUGA_REACT_STUN) && (em->stateGroup != EM_GROUP_REACT || em->state != GARUGA_REACT_FALL)) {
            Em_SetStatus5Timer(em);
            EmGaruga_ChangeState(em, EM_GROUP_REACT, GARUGA_REACT_STUN, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_ATTACK13:
        Em_ResetKindTimer(em);
        EmGaruga_ChangeState(em, EM_GROUP_REACT, GARUGA_REACT_HOLD2, EM_SYNC_OWNER);
        break;
    case EM_DMG_STATUS_IN_AIR:
        if (em->stateGroup != EM_GROUP_REACT || em->state != GARUGA_REACT_FALL) {
            EmGaruga_ChangeState(em, EM_GROUP_REACT, GARUGA_REACT_FALL, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_STATUS4:
        if ((s8) em->holdTimer != 0) {
            Em_SetStatus4Timer(em);
            EmGaruga_ChangeState(em, EM_GROUP_REACT, GARUGA_REACT_STATUS4_DOWNED, EM_SYNC_OWNER);
        } else if ((em->stateGroup != EM_GROUP_REACT || em->state != GARUGA_REACT_STATUS4) && (em->stateGroup != EM_GROUP_REACT || em->state != GARUGA_REACT_FALL)) {
            Em_SetStatus4Timer(em);
            EmGaruga_ChangeState(em, EM_GROUP_REACT, GARUGA_REACT_STATUS4, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_CAPTURED:
        if (Em_IsHeld(em) == 1) {
            if ((s8) em->holdTimer != 0) {
                Em_SetStatus1Timer(em);
                EmGaruga_ChangeState(em, EM_GROUP_EVENT, GARUGA_EVENT_CAPTURED, EM_SYNC_OWNER);
                break;
            }
            if ((s8) em->holdTimer2 != 0) {
                EmGaruga_ChangeState(em, EM_GROUP_EVENT, GARUGA_EVENT_CAPTURED_HOLD2, EM_SYNC_OWNER);
                break;
            }
        }
        if ((em->stateGroup != EM_GROUP_IDLE || em->state != GARUGA_IDLE_STATUS1_COLLAPSE) && (em->stateGroup != EM_GROUP_REACT || em->state != GARUGA_REACT_FALL)) {
            Em_SetStatus1Timer(em);
            EmGaruga_ChangeState(em, EM_GROUP_IDLE, GARUGA_IDLE_STATUS1_COLLAPSE, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_STATUS2:
        if ((s8) em->holdTimer != 0) {
            Em_SetStatus2Timer(em);
            EmGaruga_ChangeState(em, EM_GROUP_IDLE, GARUGA_IDLE_STATUS2_DOWNED, EM_SYNC_OWNER);
        } else if ((s8) em->holdTimer2 != 0) {
            Em_SetStatus2Timer(em);
            EmGaruga_ChangeState(em, EM_GROUP_IDLE, GARUGA_IDLE_STATUS2_COLLAPSE, EM_SYNC_OWNER);
        } else if ((em->stateGroup != EM_GROUP_IDLE || em->state != GARUGA_IDLE_STATUS2_COLLAPSE) && (em->stateGroup != EM_GROUP_REACT || em->state != GARUGA_REACT_FALL)) {
            Em_SetStatus2Timer(em);
            EmGaruga_ChangeState(em, EM_GROUP_IDLE, GARUGA_IDLE_STATUS2_COLLAPSE, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_HIT_LYING:
        switch (em->state) {
        case GARUGA_IDLE_REST:
            EmGaruga_ChangeState(em, EM_GROUP_IDLE, GARUGA_IDLE_REST_END, EM_SYNC_OWNER);
            em->scriptPending = 0;
            Em_CheckRageByHp(em, em->rageThreshold);
            break;
        case GARUGA_IDLE_STATUS2_COLLAPSE:
        case GARUGA_IDLE_STATUS2_REGEN:
            EmGaruga_ChangeState(em, EM_GROUP_IDLE, GARUGA_IDLE_STATUS2_END, EM_SYNC_OWNER);
            em->scriptPending = 0;
            break;
        case GARUGA_IDLE_STATUS1_COLLAPSE:
            EmGaruga_ChangeState(em, EM_GROUP_IDLE, GARUGA_IDLE_STATUS1_END, EM_SYNC_OWNER);
            em->scriptPending = 0;
            break;
        case GARUGA_IDLE_STATUS2_DOWNED:
            EmGaruga_ChangeState(em, EM_GROUP_REACT, GARUGA_REACT_STATUS2_DOWNED_END, EM_SYNC_OWNER);
            em->scriptPending = 0;
            break;
        case GARUGA_IDLE_STATUS1_DOWNED:
            EmGaruga_ChangeState(em, EM_GROUP_REACT, GARUGA_REACT_STATUS1_DOWNED_END, EM_SYNC_OWNER);
            em->scriptPending = 0;
            break;
        }
        break;
    case EM_DMG_PART8_DOWN:
        EmGaruga_ChangeState(em, EM_GROUP_REACT, GARUGA_REACT_PART_BREAK, EM_SYNC_OWNER);
        break;
    case EM_DMG_PART_FLINCH:
    case EM_DMG_ATTACK5_FLINCH:
        Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
        if (em->stance == EM_STANCE_AIR) {
            EmGaruga_ChangeState(em, EM_GROUP_REACT, GARUGA_REACT_FALL, EM_SYNC_OWNER);
        } else {
            switch (em->flinchPart) {
            case 0:
            case 7:
                EmGaruga_ChangeState(em, EM_GROUP_REACT, GARUGA_REACT_FLINCH, EM_SYNC_OWNER);
                break;
            case 5:
            case 6:
                EmGaruga_ChangeState(em, EM_GROUP_REACT, GARUGA_REACT_FLINCH_C, EM_SYNC_OWNER);
                break;
            case 1:
            case 2:
                EmGaruga_ChangeState(em, EM_GROUP_REACT, GARUGA_REACT_FLINCH_D, EM_SYNC_OWNER);
                break;
            case 3:
            case 4:
            default:
                if (em->parts[em->flinchPart].state >= 2) {
                    if (em->flinchPart != 3) {
                        EmGaruga_ChangeState(em, EM_GROUP_REACT, GARUGA_REACT_KNOCK_DOWN, EM_SYNC_OWNER);
                    } else {
                        EmGaruga_ChangeState(em, EM_GROUP_REACT, GARUGA_REACT_KNOCK_DOWN_PART3, EM_SYNC_OWNER);
                    }
                } else {
                    EmGaruga_ChangeState(em, EM_GROUP_REACT, GARUGA_REACT_FLINCH_B, EM_SYNC_OWNER);
                }
                break;
            }
        }
        break;
    case EM_DMG_HIT_MODE3:
        if (em->stance != EM_STANCE_AIR) {
            EmGaruga_ChangeState(em, EM_GROUP_REACT, GARUGA_REACT_FLINCH, EM_SYNC_OWNER);
            em->scriptPending = 0;
        }
        break;
    }
    if (em->parts[6].state >= 2) {
        Em_BreakPart(em, 1);
    }
    if (em->parts[6].state >= 3) {
        Em_BreakPart(em, 2);
    }
    if (em->parts[0].state >= 2) {
        Em_BreakPart(em, 3);
    }
    if (em->parts[1].state >= 2) {
        Em_BreakPart(em, 4);
    }
    if (em->parts[2].state >= 2) {
        Em_BreakPart(em, 4);
    }
    switch (gQuest.stage) {
    case QUEST_ID_GARUGA_SCENE:
        if (Area_DayId(em->area) == AREA_JUNGLE_9 && Save_TestEventFlag(EVENT_FLAG_GARUGA_SCENE) == 0) {
            if (Area_DayId(gScene.area) == AREA_JUNGLE_9 && gScene.eventRunning == 1 && em->stateGroup != EM_GROUP_EVENT) {
                EmGaruga_ChangeState(em, EM_GROUP_EVENT, GARUGA_EVENT_STORY_INTRO, EM_SYNC_SEND);
            }
            break;
        }
    default:
        Em_UpdatePendingScript(em);
        break;
    }
    Em_ClearHitReactMode(em);
    EmGaruga_RunState(em, (s32) w);
    if (em->stateRequest != 0) {
        EmGaruga_RunState(em, (s32) w);
        em->stateRequest = 0;
    }
    if (em->stateGroup == EM_GROUP_FLY && (s8) w->flyGuard == 1) {
        em->guardTimer = 2;
    }
}

/*
 * Runs the current state through the dispatcher of its group, keeping the
 * group and state as the previous ones.
 */
void EmGaruga_RunState(Em *em, s32 work)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->stateGroup) {
    case EM_GROUP_IDLE:
        EmGaruga_RunIdleState(em, (EmWork *) work);
        break;
    case EM_GROUP_MOVE:
        EmGaruga_RunMoveState(em, (EmWork *) work);
        break;
    case EM_GROUP_FLY:
        EmGaruga_RunFlyState(em, (EmWork *) work);
        break;
    case EM_GROUP_ATTACK:
        EmGaruga_RunAttackState(em, (EmWork *) work);
        break;
    case EM_GROUP_REACT:
        EmGaruga_RunReactState(em, (EmWork *) work);
        break;
    case EM_GROUP_DEFEAT:
        EmGaruga_RunDefeatState(em, (EmWork *) work);
        break;
    case EM_GROUP_EVENT:
        EmGaruga_RunEventState(em, work);
        break;
    case EM_GROUP_EVENT2:
        EmGaruga_RunEventState(em, work);
        break;
    case EM_GROUP_VERSUS:
        EmGaruga_RunVersusState(em, (EmWork *) work);
        
    default:
        break;
    }
}

/* Requests camera shake level i when the motion passes frame frame. */
void EmGaruga_CamShakeOnFrame(void *em, s32 frame, s32 level)
{
    if (EmMotion_PassedFrameScaled((Em *) em, (f32) frame, 0U) != 0) {
        CamShake_ReqAIf(em, level);
    }
}

static void EmGaruga_MotionEffectsDefault(Em *em)
{
}

/*
 * The effects of each motion, switched on the layer-0 motion id kept in
 * work->sndMotion: plays its sounds (EmGaruga_MotionSounds), then on given
 * frames creates the hit zones of the attacks (GarugaAttack_Create by row
 * kind; GarugaAttack_CreateForEffects with a fire burst at the head bone in
 * GARUGA_MOT_ROAR and in the GARUGA_FLY_BACK_HOP hop), dust and
 * ground effects (DustCloud_Create, extra random ones on many motions while
 * enraged), Effect0D bursts on flinches, the lying puffs
 * (Em_SpawnLyingPuffs) and camera shakes (EmGaruga_CamShakeOnFrame). A list
 * of motions has no effects.
 */
void EmGaruga_MotionEffects(Em *em, EmFlyerWork *work)
{
    f32 puffOffsetA[3];
    f32 puffOffsetB[3];
    f32 bonePosA[3];
    f32 bonePosB[3];

    if (em->motionId[0] != work->sndMotion) {
        work->sndMotion = em->motionId[0];
    }
    EmGaruga_MotionSounds(em, (EmWork *) work);
    switch (work->sndMotion) {
    case EM_MOTION_ID(0, GARUGA_MOT_WALK):
        EmGaruga_CamShakeOnFrame(em, 52, 1);
        EmGaruga_CamShakeOnFrame(em, 116, 1);
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            GarugaAttack_Create(em, 2);
        }
        if (EmMotion_PassedFrameScaled(em, 50.0f, 0) != 0) {
            GarugaAttack_Create(em, 3);
        }
        if (EmMotion_PassedFrameScaled(em, 120.0f, 0) != 0) {
            GarugaAttack_Create(em, 4);
        }
        break;
    case EM_MOTION_ID(0, 4):
        if (EmMotion_PassedFrameScaled(em, 38.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 3, 0, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 74.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 2, 0, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_TURN_RIGHT):
        EmGaruga_CamShakeOnFrame(em, 60, 1);
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            GarugaAttack_Create(em, 5);
        }
        if (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) {
            GarugaAttack_Create(em, 6);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 2.0f, 78.0f) != 0) && !(gScene.frame & 3)) {
            DustCloud_Create((Em *) em, 26, 1, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_TURN_LEFT):
        EmGaruga_CamShakeOnFrame(em, 60, 1);
        if (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) {
            GarugaAttack_Create(em, 7);
        }
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            GarugaAttack_Create(em, 8);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 2.0f, 78.0f) != 0) && !(gScene.frame & 3)) {
            DustCloud_Create((Em *) em, 26, 0, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, 7):
        if (EmMotion_PassedFrameScaled(em, 18.0f, 0) != 0) {
            GarugaAttack_Create(em, 9);
        }
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            GarugaAttack_Create(em, 10);
        }
        if (em->rage != 0) {
            if ((EmMotion_InFrameRange(em, 0, 2.0f, 18.0f) != 0) && !(gScene.frame & 3)) {
                DustCloud_Create((Em *) em, 26, 1, 1.0f);
            }
            if ((EmMotion_InFrameRange(em, 0, 20.0f, 70.0f) != 0) && !(gScene.frame & 3)) {
                DustCloud_Create((Em *) em, 26, 0, 1.0f);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, 8):
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            GarugaAttack_Create(em, 11);
        }
        if (EmMotion_PassedFrameScaled(em, 18.0f, 0) != 0) {
            GarugaAttack_Create(em, 12);
        }
        if (em->rage != 0) {
            if ((EmMotion_InFrameRange(em, 0, 2.0f, 18.0f) != 0) && !(gScene.frame & 3)) {
                DustCloud_Create((Em *) em, 26, 0, 1.0f);
            }
            if ((EmMotion_InFrameRange(em, 0, 20.0f, 70.0f) != 0) && !(gScene.frame & 3)) {
                DustCloud_Create((Em *) em, 26, 1, 1.0f);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_LIMP):
        if (EmMotion_PassedFrameScaled(em, 42.0f, 0) != 0) {
            GarugaAttack_Create(em, 25);
        }
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            GarugaAttack_Create(em, 26);
        }
        if (EmMotion_PassedFrameScaled(em, 74.0f, 0) != 0) {
            GarugaAttack_Create(em, 26);
        }
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_DESCEND):
        if (EmMotion_PassedFrameScaled(em, 52.0f, 0) || EmMotion_PassedFrameScaled(em, 104.0f, 0) || EmMotion_PassedFrameScaled(em, 162.0f, 0)) {
            PartFx_SpawnForEmScaled(em, GARUGA_BONE_BODY, 7, 5.0f);
        }
        break;
    case EM_MOTION_ID(0, 13):
        if ((EmMotion_PassedFrameScaled(em, 26.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 52.0f, 0) != 0)) {
            PartFx_SpawnForEmScaled(em, 1, 7, 5.0f);
        }
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_FLY):
    case EM_MOTION_ID(0, 85):
    case EM_MOTION_ID(0, 86):
    case EM_MOTION_ID(0, 88):
    case EM_MOTION_ID(0, 89):
        if ((em->rage != 0) && (((s32) gScene.frame % 10) == 0)) {
            DustCloud_Create((Em *) em, 25, ((u16) Rand_Next(1) & 1) + 2, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_HOVER):
        if (EmMotion_PassedFrameScaled(em, 8.0f, 0) || EmMotion_PassedFrameScaled(em, 86.0f, 0) || EmMotion_PassedFrameScaled(em, 160.0f, 0)) {
            if (em->posY <= (1000.0f + em->floorY)) {
                DustCloud_Create((Em *) em, 1, 0, 1.0f);
            }
        }
        if (em->rage != 0) {
            if (EmMotion_InFrameRange(em, 0, 70.0f, 84.0f) || EmMotion_InFrameRange(em, 0, 146.0f, 156.0f) || EmMotion_InFrameRange(em, 0, 220.0f, 234.0f)) {
                if (!(gScene.frame & 3)) {
                    DustCloud_Create((Em *) em, 26, (u16) Rand_Next(1) & 1, 1.0f);
                    break;
                }
            }
        }
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_RUN):
        if (EmMotion_PassedFrameScaled(em, 18.0f, 0) != 0) {
            GarugaAttack_Create(em, 19);
        }
        if ((EmMotion_PassedFrameScaled(em, 12.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 72.0f, 0) != 0)) {
            DustCloud_Create((Em *) em, 2, 0, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 40.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 3, 0, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_TAKE_OFF):
        if (EmMotion_PassedFrameScaled(em, 42.0f, 0) || EmMotion_PassedFrameScaled(em, 102.0f, 0) || EmMotion_PassedFrameScaled(em, 156.0f, 0)) {
            if (em->posY <= (1000.0f + em->floorY)) {
                DustCloud_Create((Em *) em, 1, 0, 1.0f);
            }
        }
        if ((em->rage != 0) && (EmMotion_PassedFrameScaled(em, 38.0f, 0) != 0)) {
            DustCloud_Create((Em *) em, 25, 5, 1.0f);
            DustCloud_Create((Em *) em, 25, 4, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_TOUCH_DOWN):
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            GarugaAttack_Create(em, 13);
        }
        if (EmMotion_PassedFrameScaled(em, 16.0f, 0) != 0) {
            EmGaruga_SpawnLandingDust(em);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 18.0f, 44.0f) != 0) && !(gScene.frame & 3)) {
            DustCloud_Create((Em *) em, 26, (u16) Rand_Next(1) & 1, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, 31):
        puffOffsetA[1] = 10.0f;
        puffOffsetA[2] = 140.0f;
        puffOffsetA[0] = 0.0f;
        Em_SpawnLyingPuffs(em, GARUGA_BONE_BREATH, puffOffsetA, 1.6f);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_LIE):
        puffOffsetB[1] = 10.0f;
        puffOffsetB[2] = 140.0f;
        puffOffsetB[0] = 0.0f;
        Em_SpawnLyingPuffs(em, GARUGA_BONE_BREATH, puffOffsetB, 1.6f);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_STARE):
        if (EmMotion_PassedFrameScaled(em, 50.0f, 0) != 0) {
            GarugaAttack_Create(em, 1);
        }
        if (EmMotion_PassedFrameScaled(em, 46.0f, 0) != 0) {
            GarugaAttack_Create(em, 33);
        }
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_COLLAPSE):
        if (EmMotion_PassedFrameScaled(em, 148.0f, 0) != 0) {
            GarugaAttack_Create(em, 15);
        }
        if (EmMotion_PassedFrameScaled(em, 156.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 0, 0, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_ROAR):
        if (EmMotion_PassedFrameScaled(em, 60.0f, 0) != 0) {
            Mtx_GetTrans(bonePosA, em->model->bones[GARUGA_BONE_HEAD].world);
            ElementBurst_CreateOnEm(em, 5, 1, 1.0f);
            GarugaAttack_CreateForEffects(em, 22);
        }
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_FLIP):
        if (EmMotion_PassedFrameScaled(em, 108.0f, 0) != 0) {
            GarugaAttack_Create(em, 37);
        }
        if (EmMotion_PassedFrameScaled(em, 104.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 1, 0, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 200.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 1, 0, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_TAIL_SPIN):
        EmGaruga_CamShakeOnFrame(em, 44, 1);
        EmGaruga_CamShakeOnFrame(em, 73, 1);
        if (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) {
            if (em->state == GARUGA_ATK_TAIL_SPIN_B) {
                GarugaAttack_Create(em, 39);
            } else {
                GarugaAttack_Create(em, 17);
            }
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 6.0f, 38.0f) != 0) && !(gScene.frame & 3)) {
            DustCloud_Create((Em *) em, 26, 0, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, 43):
        EmGaruga_CamShakeOnFrame(em, 44, 1);
        EmGaruga_CamShakeOnFrame(em, 73, 1);
        if (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) {
            if (em->state == GARUGA_ATK_TAIL_SPIN_B) {
                GarugaAttack_Create(em, 39);
            } else {
                GarugaAttack_Create(em, 17);
            }
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 6.0f, 38.0f) != 0) && !(gScene.frame & 3)) {
            DustCloud_Create((Em *) em, 26, 1, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_FIREBALL):
        if (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) {
            GarugaAttack_Create(em, 34);
        }
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_ALERT_IDLE):
        if (em->rage != 0) {
            if (EmMotion_InFrameRange(em, 0, 72.0f, 80.0f) || EmMotion_InFrameRange(em, 0, 132.0f, 140.0f) || EmMotion_InFrameRange(em, 0, 184.0f, 192.0f)) {
                DustCloud_Create((Em *) em, 26, (u16) Rand_Next(1) & 1, 1.0f);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_BACK_HOP):
        if ((em->stateGroup == EM_GROUP_FLY) && (em->state == GARUGA_FLY_BACK_HOP) && (EmMotion_PassedFrameScaled(em, 70.0f, 0) != 0)) {
            Mtx_GetTrans(bonePosB, em->model->bones[GARUGA_BONE_HEAD].world);
            if (Player_InCurrentArea(em) & 0xFF) {
                ElementBurst_CreateAtSilent(bonePosB, 4, 0, 1.0f);
            }
            GarugaAttack_CreateForEffects(em, 22);
        }
        if (EmMotion_PassedFrameScaled(em, 72.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 4, 0, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 80.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 1, 0, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 84.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 4, 1, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_BACK_HOP_LAND):
        if (EmMotion_PassedFrameScaled(em, 12.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 4, 0, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 5, 0, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 28.0f, 0) != 0) {
            GarugaAttack_Create(em, 27);
        }
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            GarugaAttack_Create(em, 31);
        }
        if (EmMotion_PassedFrameScaled(em, 36.0f, 0) != 0) {
            GarugaAttack_Create(em, 32);
        }
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_FLINCH_A):
        if (EmMotion_PassedFrameScaled(em, 44.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 27, 7, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 62.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, GARUGA_BONE_FOOT_A, 7, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 80.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 1, 128, 0.5f);
        }
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_FLINCH_E):
        if (EmMotion_PassedFrameScaled(em, 24.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, GARUGA_BONE_STEP_B, 7, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_PART_BREAK):
        if (EmMotion_PassedFrameScaled(em, 20.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 3, 128, 1.0f);
        }
        if ((EmMotion_PassedFrameScaled(em, 128.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 164.0f, 0) != 0)) {
            DustCloud_Create((Em *) em, 3, 128, 0.6f);
        }
        if (EmMotion_PassedFrameScaled(em, 60.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 1, 128, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_DIE):
        if (EmMotion_PassedFrameScaled(em, 270.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 18, 128, 0.9f);
        }
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_KNOCK_DOWN):
        EmGaruga_CamShakeOnFrame(em, 26, 2);
        if (EmMotion_PassedFrameScaled(em, 26.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 18, 128, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_KNOCK_DOWN_B):
        EmGaruga_CamShakeOnFrame(em, 26, 2);
        if (EmMotion_PassedFrameScaled(em, 26.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 18, 129, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_FALL_LAND):
        if (EmMotion_PassedFrameScaled(em, 8.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 11, 0, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_HEAD_DOWN):
        if (EmMotion_PassedFrameScaled(em, 48.0f, 0) || EmMotion_PassedFrameScaled(em, 106.0f, 0) || EmMotion_PassedFrameScaled(em, 174.0f, 0)) {
            GarugaAttack_Create(em, 16);
        }
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_HOP_IN_PLACE):
        if (EmMotion_PassedFrameScaled(em, 42.0f, 0) || EmMotion_PassedFrameScaled(em, 60.0f, 0) || EmMotion_PassedFrameScaled(em, 80.0f, 0)) {
            DustCloud_Create((Em *) em, 1, 0, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_FLIP_LAND):
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            GarugaAttack_Create(em, 38);
        }
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_LEVEL_OUT):
        if ((EmMotion_PassedFrameScaled(em, 50.0f, 0) != 0) && (em->posY <= (1000.0f + em->floorY))) {
            DustCloud_Create((Em *) em, 9, 0, 1.0f);
            DustCloud_Create((Em *) em, 14, 0, 1.0f);
        }
        if ((EmMotion_PassedFrameScaled(em, 102.0f, 0) != 0) && (em->posY <= (1000.0f + em->floorY))) {
            DustCloud_Create((Em *) em, 9, 0, 1.0f);
            DustCloud_Create((Em *) em, 14, 0, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_CHARGE_TRIP):
        if (EmMotion_PassedFrameScaled(em, 34.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 3, 0, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 70.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 2, 0, 1.0f);
        }
        if ((EmMotion_ReachedFrame(em, 0, 96.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 120.0f) == 0) && (((s32) gScene.frame % 6) == 0)) {
            DustCloud_Create((Em *) em, 18, 0, 1.0f);
        }
        if ((em->rage != 0) && (((s32) gScene.frame % 19) == 0)) {
            DustCloud_Create((Em *) em, 25, ((u16) Rand_Next(1) & 1) + 2, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_TRAPPED):
        if (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 11, 128, 1.0f);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 4.0f, 60.0f) != 0) && !(gScene.frame & 3)) {
            DustCloud_Create((Em *) em, 26, (u16) Rand_Next(1) & 1, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_STRUGGLE):
        if (em->rage != 0) {
            if (EmMotion_InFrameRange(em, 0, 12.0f, 24.0f) || EmMotion_InFrameRange(em, 0, 46.0f, 90.0f)) {
                if (!(gScene.frame & 3)) {
                    DustCloud_Create((Em *) em, 26, (u16) Rand_Next(1) & 1, 1.0f);
                    break;
                }
            }
        }
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_STOMPS):
        if (EmMotion_PassedFrameScaled(em, 118.0f, 0) || EmMotion_PassedFrameScaled(em, 164.0f, 0) || EmMotion_PassedFrameScaled(em, 204.0f, 0)) {
            GarugaAttack_Create(em, 35);
        }
        if (em->rage != 0) {
            if (EmMotion_InFrameRange(em, 0, 54.0f, 250.0f) || EmMotion_InFrameRange(em, 0, 282.0f, 296.0f)) {
                if (!(gScene.frame & 3)) {
                    DustCloud_Create((Em *) em, 26, (u16) Rand_Next(1) & 1, 1.0f);
                    break;
                }
            }
        }
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_TURN_STOMP):
        if (EmMotion_PassedFrameScaled(em, 52.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 1, 128, 1.0f);
        }
        if (em->rage != 0) {
            if (EmMotion_InFrameRange(em, 0, 10.0f, 68.0f) || EmMotion_InFrameRange(em, 0, 84.0f, 96.0f)) {
                if (!(gScene.frame & 3)) {
                    DustCloud_Create((Em *) em, 26, (u16) Rand_Next(1) & 1, 1.0f);
                    break;
                }
            }
        }
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_PECK_COMBO):
        if ((EmMotion_PassedFrameScaled(em, 32.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 54.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 74.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 92.0f, 0) != 0)) {
            GarugaAttack_Create(em, 24);
        }
        if (em->rage != 0) {
            if ((EmMotion_InFrameRange(em, 0, 10.0f, 20.0f) != 0) && !(gScene.frame & 3)) {
                DustCloud_Create((Em *) em, 26, (u16) Rand_Next(1) & 1, 1.0f);
            }
            if ((EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 54.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 72.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 92.0f, 0) != 0)) {
                DustCloud_Create((Em *) em, 26, (u16) Rand_Next(1) & 1, 1.0f);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_RUN_B):
        if (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) {
            GarugaAttack_Create(em, 23);
        }
        if ((EmMotion_PassedFrameScaled(em, 18.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 60.0f, 0) != 0)) {
            PartFx_SpawnForEmScaled(em, GARUGA_BONE_STEP_B, 3, 4.0f);
        }
        if ((EmMotion_PassedFrameScaled(em, 38.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 80.0f, 0) != 0)) {
            PartFx_SpawnForEmScaled(em, GARUGA_BONE_STEP_A, 3, 4.0f);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 54.0f, 112.0f) != 0) && !(gScene.frame & 3)) {
            DustCloud_Create((Em *) em, 26, (u16) Rand_Next(1) & 1, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_PECK_COMBO_B):
        if (EmMotion_PassedFrameScaled(em, 20.0f, 0) != 0) {
            GarugaAttack_Create(em, 18);
        }
        if ((EmMotion_PassedFrameScaled(em, 50.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 76.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 96.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 116.0f, 0) != 0)) {
            GarugaAttack_Create(em, 24);
        }
        if (EmMotion_PassedFrameScaled(em, 20.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 3, 0, 1.0f);
        }
        if (em->rage != 0) {
            if ((EmMotion_InFrameRange(em, 0, 24.0f, 42.0f) != 0) && !(gScene.frame & 3)) {
                DustCloud_Create((Em *) em, 26, (u16) Rand_Next(1) & 1, 1.0f);
            }
            if ((EmMotion_PassedFrameScaled(em, 54.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 80.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 100.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 122.0f, 0) != 0)) {
                DustCloud_Create((Em *) em, 26, (u16) Rand_Next(1) & 1, 1.0f);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, 109):
        if (EmMotion_PassedFrameScaled(em, 20.0f, 0) != 0) {
            GarugaAttack_Create(em, 14);
        }
        if (EmMotion_PassedFrameScaled(em, 20.0f, 0) || EmMotion_PassedFrameScaled(em, 92.0f, 0) || EmMotion_PassedFrameScaled(em, 152.0f, 0)) {
            PartFx_SpawnForEmScaled(em, GARUGA_BONE_BODY, 7, 3.0f);
        }
        break;
    default:
        EmGaruga_MotionEffectsDefault(em);
        break;
    case EM_MOTION_ID(0, GARUGA_MOT_IDLE):
    case EM_MOTION_ID(0, GARUGA_MOT_GLIDE):
    case EM_MOTION_ID(0, GARUGA_MOT_IDLE_ACTION_I):
    case EM_MOTION_ID(0, 32):
    case EM_MOTION_ID(0, 33):
    case EM_MOTION_ID(0, GARUGA_MOT_GET_UP):
    case EM_MOTION_ID(0, GARUGA_MOT_LIE_DOWN):
    case EM_MOTION_ID(0, GARUGA_MOT_FLIP_FALL):
    case EM_MOTION_ID(0, 48):
    case EM_MOTION_ID(0, GARUGA_MOT_TURN_IN_PLACE):
    case EM_MOTION_ID(0, GARUGA_MOT_FEED):
    case EM_MOTION_ID(0, GARUGA_MOT_SHAKE_OFF):
    case EM_MOTION_ID(0, GARUGA_MOT_FLINCH_C):
    case EM_MOTION_ID(0, GARUGA_MOT_FLINCH_D):
    case EM_MOTION_ID(0, GARUGA_MOT_FLINCH_B):
    case EM_MOTION_ID(0, 67):
    case EM_MOTION_ID(0, GARUGA_MOT_DOWN):
    case EM_MOTION_ID(0, GARUGA_MOT_DOWN_GET_UP):
    case EM_MOTION_ID(0, GARUGA_MOT_KNOCK_DOWN_B_GET_UP):
    case EM_MOTION_ID(0, GARUGA_MOT_KNOCK_DOWN_B_LIE):
    case EM_MOTION_ID(0, GARUGA_MOT_STATUS4):
    case EM_MOTION_ID(0, GARUGA_MOT_FALL):
    case EM_MOTION_ID(0, GARUGA_MOT_IDLE_SIDE_A):
    case EM_MOTION_ID(0, GARUGA_MOT_IDLE_SIDE_B):
    case EM_MOTION_ID(0, GARUGA_MOT_DIE_FALLING):
    case EM_MOTION_ID(0, GARUGA_MOT_DOWNED):
    case EM_MOTION_ID(0, GARUGA_MOT_DOWNED_GET_UP):
    case EM_MOTION_ID(0, GARUGA_MOT_FIREBALL_B):
    case EM_MOTION_ID(0, 108):
        break;
    }
}

/*
 * Runs EmGaruga_MotionEffects once fxStep of the work area is set (the
 * first frame only sets it).
 */
void EmGaruga_UpdateMotionEffects(Em *em)
{
    EmFlyerWork *w = (EmFlyerWork *)&em->work;

    switch (w->fxStep) {
    case 0:
        w->fxStep++;
        break;
    case 1:
        EmGaruga_MotionEffects(em, w);
        break;
    }
}

void EmGaruga_SpawnLandingDust(Em *em)
{
    DustCloud_Create((Em *) em, 11, 0, 1.0f);
}

void EmGaruga_EndAttackState(Em *em, EmWork *work)
{
    EmGaruga_EndState(em, 0, 0);
}

void EmGaruga_TableNop(void)
{
}

/*
 * Whether the once-only reaction (GARUGA_REACT_PART_BREAK) may run: not in
 * the trapped, downed or downed-end states, not in the air, and not after
 * it ran once.
 */
s32 EmGaruga_AllowPartBreak(Em *em)
{
    u8 stateGroup;

    stateGroup = em->stateGroup;
    if ((stateGroup != EM_GROUP_REACT || em->state != GARUGA_REACT_TRAPPED)
        && (stateGroup != EM_GROUP_REACT || em->state != GARUGA_REACT_TRAPPED_STRUGGLE)
        && (stateGroup != EM_GROUP_REACT || em->state != GARUGA_REACT_STATUS4_DOWNED)
        && (stateGroup != EM_GROUP_REACT || em->state != GARUGA_REACT_STATUS4_DOWNED_END)
        && (stateGroup != EM_GROUP_REACT || em->state != GARUGA_REACT_STATUS2_DOWNED_END)
        && (stateGroup != EM_GROUP_REACT || em->state != GARUGA_REACT_STATUS1_DOWNED_END)
        && em->stance != EM_STANCE_AIR && (s8) em->partBreakCount == 0) {
        return 1;
    }
    return 0;
}

/*
 * Starts a group-0 state on the ground: the alert idle instead of the idle
 * in combat, and the rage, statuses and needs each state begins or ends
 * (the rage checks of the stomps, the status 1/2/4/5 begin and end, the eat
 * goal, the rest).
 */
void EmGaruga_SetIdleState(Em *em, u16 state, u8 mode)
{
    em->motionSpeed = 1.0f;
    switch (state) {
    case GARUGA_IDLE:
        if (em->combat == 1) {
            state = GARUGA_IDLE_ALERT;
        }
        break;
    case GARUGA_IDLE_STOMPS_RAGE_BY_HP:
        Em_CheckRageByHp(em, (s32) em->rageThreshold);
        break;
    case GARUGA_IDLE_STOMPS_PENDING_RAGE:
        Em_StartPendingRage(em);
        break;
    case GARUGA_IDLE_HOLD2_SHAKE_OFF:
        em->holdTimer2 = 0;
        break;
    case GARUGA_IDLE_STATUS4_SHAKE_OFF:
        Em_EndStatus4(em);
        break;
    case GARUGA_IDLE_STUN_SHAKE_OFF:
        Em_EndStatus5(em);
        break;
    case GARUGA_IDLE_EAT:
        Em_StartGoalAction(em, 10000);
        break;
    case GARUGA_IDLE_REST:
        Em_ResetNotice(em);
        break;
    case GARUGA_IDLE_STATUS2_COLLAPSE:
        Em_BeginStatus2(em);
        break;
    case GARUGA_IDLE_STATUS2_REGEN:
        Em_BeginStatus2(em);
        break;
    case GARUGA_IDLE_REST_END:
        Em_RefillNeed3(em);
        break;
    case GARUGA_IDLE_STATUS2_END:
        Em_EndStatus2(em);
        break;
    case GARUGA_IDLE_STATUS1_COLLAPSE:
        Em_BeginStatus1(em);
        break;
    case GARUGA_IDLE_STATUS1_END:
        Em_EndStatus1(em);
        break;
    case GARUGA_IDLE_STATUS2_DOWNED:
        Em_BeginStatus2(em);
        break;
    case GARUGA_IDLE_STATUS1_DOWNED:
        Em_BeginStatus1(em);
        break;
    }
    em->stance = EM_STANCE_GROUND;
    Em_SetStateSync(em, EM_GROUP_IDLE, state, mode);
}

/*
 * Starts a group-1 state: the limp below 0.3 of hpMax, the turn in combat,
 * the walk and run speeds (GARUGA_MOVE_RUN_SLOW at 0.8 motion speed) and
 * their timers; GARUGA_MOVE_END only sets scriptPending.
 */
void EmGaruga_SetMoveState(Em *em, u16 state, u8 mode)
{
    em->stance = EM_STANCE_GROUND;
    em->motionSpeed = 1.0f;
    switch (state) {
    case GARUGA_MOVE_WALK:
        /* low on hp it limps: the goto joins that case */
        if (em->hp < (s16) (0.3f * em->hpMax)) {
            state = GARUGA_MOVE_LIMP;
            goto state6;
        }
        Em_SetSpeedScaled(em, 0, 1000.0f, 500.0f);
        if (em->remote == 0 && em->goalKind == 0) {
            em->timer = (em->stateRand % 120 + 180) / 2;
        }
        break;
    case GARUGA_MOVE_END:
        em->scriptPending = 1;
        return;
    case GARUGA_MOVE_TURN:
        if (em->combat == 1) {
            state = GARUGA_MOVE_TURN_IN_COMBAT;
        }
        break;
    case GARUGA_MOVE_RUN_SLOW:
        em->motionSpeed = 0.8f;
    case GARUGA_MOVE_RUN_B:
    case GARUGA_MOVE_RUN:
    case GARUGA_MOVE_RUN_B_AND_TURN:
        Em_SetSpeedScaled(em, 2, 1000.0f, 500.0f);
        if (em->curGoalKind == 0) {
            em->timer = (em->stateRand % 120 + 180) / 2;
        }
        break;
    case GARUGA_MOVE_LIMP:
    state6:
        Em_SetSpeedScaled(em, 6, em->goalDist, 0.0f);
        if (em->curGoalKind == 0) {
            em->timer = (em->stateRand % 120 + 180) / 2;
        }
        break;
    case GARUGA_MOVE_TURN_IN_COMBAT:
    case GARUGA_MOVE_TURN_TO_GOAL:
    case GARUGA_MOVE_NOP:
        break;
    }
    Em_SetStateSync(em, EM_GROUP_MOVE, state, mode);
}

/*
 * Starts a group-2 state: the landing as GARUGA_FLY_LEVEL_OUT_AND_LAND when
 * flying fast (velZ 50 or more), the goals and timers of the routes, hovers
 * and glides (the point of the area sequence when one runs), and a take-off
 * instead of flying out of or into the area without an exit goal (goalKind
 * 3).
 */
void EmGaruga_SetFlyState(Em *em, u16 state, u8 mode)
{
    EmFlyerWork *w = (EmFlyerWork *)&em->work;

    em->motionSpeed = 1.0f;
    switch (state) {
    case GARUGA_FLY_LAND:
        if (em->remote == 0 && !(em->velZ < 50.0f)) {
            state = GARUGA_FLY_LEVEL_OUT_AND_LAND;
        }
        break;
    case GARUGA_FLY_ROUTE:
    case GARUGA_FLY_ROUTE_B:
        w->legsLeft = 4;
        if (em->remote == 0) {
            em->goalNo = em->stateRand & 3;
            em->goalKind = 2;
            em->goalArg = 2;
            Em_ComputeGoal(em, em->goal);
            em->timer = 1.5f * Vec_DistanceXZ(&em->posX, em->goal) / 50.0f;
        }
        break;
    case GARUGA_FLY_HOVER_TO_GOAL:
    case GARUGA_FLY_HOVER_STRAIGHT_TO_GOAL:
        Player_ClearVelocity(em);
        em->velZ = 20.0f;
        em->timer = (s32) (Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) / em->velZ) * 3;
        break;
    case GARUGA_FLY_INTO_AREA:
        Em_TypeSetup(em);
    case GARUGA_FLY_OUT_OF_AREA:
        if (em->remote == 0 && em->goalKind != 3) {
            EmGaruga_ChangeState(em, EM_GROUP_FLY, GARUGA_FLY_TAKE_OFF, EM_SYNC_SEND);
            return;
        }
        break;
    case GARUGA_FLY_TO_GOAL_AND_LAND:
        if (em->remote == 0) {
            em->goalKind = 2;
            if (em->areaSeqActive != 0) {
                em->goalArg = 3;
                em->goalNo = 0;
            } else {
                em->goalArg = 0;
            }
            Em_ComputeGoal(em, em->goal);
        }
        em->timer = 500;
        Em_TypeSetup(em);
        break;
    case GARUGA_FLY_GLIDE_TO_GOAL:
        if (em->remote == 0) {
            if (em->areaSeqActive != 0) {
                em->goalKind = 2;
                em->goalArg = 3;
                em->goalNo = 0;
            }
            Em_ComputeGoal(em, em->goal);
        }
        em->timer = 500;
        break;
    case GARUGA_FLY_GLIDE_TO_GOAL_B:
        w->legsLeft = 1;
        if (em->remote == 0) {
            em->timer = 1.5f * Vec_DistanceXZ(&em->posX, em->goal) / 50.0f;
        }
        break;
    case GARUGA_FLY_HOVER_RISE:
        if (em->velZ <= 0.0f) {
            em->velZ = 20.0f;
        }
        if (em->remote == 0) {
            em->timer = Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) / em->velZ;
        }
        break;
    case GARUGA_FLY_TAKE_OFF:
    case GARUGA_FLY_HOVER:
    case GARUGA_FLY_HOP_TO_GOAL:
    case GARUGA_FLY_HOP_FORWARD:
    case GARUGA_FLY_HOP_BACK:
    case GARUGA_FLY_END_FLIGHT_TO_GLIDE:
    case GARUGA_FLY_BACK_HOP:
    case GARUGA_FLY_HOVER_ONCE:
    case GARUGA_FLY_GLIDE_ONCE:
    case GARUGA_FLY_HOVER_TURN:
    case GARUGA_FLY_LEVEL_OUT_AND_LAND:
    case GARUGA_FLY_HOP_IN_PLACE:
    case GARUGA_FLY_LEVEL_OUT:
    case GARUGA_FLY_ESCAPE_UPWARD:
        break;
    }
    Em_SetStateSync(em, EM_GROUP_FLY, state, mode);
}

/*
 * Starts a group-3 state: the target angle, the goal and timer of the
 * charge (150 frames without a target), and the fireball variant
 * (GARUGA_ATK_FIREBALL_LOW, FIREBALL, FIREBALL_HIGH) by the height and
 * distance of the target.
 */
void EmGaruga_SetAttackState(Em *em, u16 state, u8 mode)
{
    EmFlyerWork *w = (EmFlyerWork *)&em->work;
    f32 pos[3];
    f32 dist;

    em->motionSpeed = 1.0f;
    switch (state) {
    case GARUGA_ATK_TURN_AND_STARE:
    case GARUGA_ATK_TURN_AND_PECK_A:
    case GARUGA_ATK_TURN_AND_PECK_B:
        Em_SetTargetAngle(em);
        break;
    case GARUGA_ATK_FIREBALL_LOW:
    case GARUGA_ATK_FIREBALL_HIGH:
    case GARUGA_ATK_FIREBALL_B:
        w->countdown = 1;
        Em_SetTargetAngle(em);
        break;
    case GARUGA_ATK_CHARGE_TRIP:
        if (em->targetNo == -1) {
            em->timer = 150;
            em->hasGoal = 0;
        } else {
            em->hasGoal = 1;
            dist = Vec_DistanceXZ(&em->posX, em->goal);
            em->goalDist = dist;
            em->timer = (s32) (dist / 30.0f) + 30;
        }
        break;
    case GARUGA_ATK_FIREBALL:
        w->countdown = 1;
        if (em->remote == 0 && em->curGoalKind == 1 && em->curGoalArg == 0) {
            if (em->targetNo == -1) {
                Em_SetTargetAngle(em);
                break;
            }
            Em_GetTargetPos(em, em->targetNo, pos);
            dist = em->playerDistXZ[em->targetNo];
            if (dist <= 1000.0f) {
                dist = pos[1];
                if (dist <= em->posY + 50.0f) {
                    state = GARUGA_ATK_FIREBALL_LOW;
                } else if (dist <= em->posY + 150.0f) {
                    state = GARUGA_ATK_FIREBALL;
                } else {
                    state = GARUGA_ATK_FIREBALL_HIGH;
                }
            } else if (dist <= 2000.0f) {
                dist = pos[1];
                if (dist <= em->posY - 150.0f) {
                    state = GARUGA_ATK_FIREBALL_LOW;
                } else if (dist <= em->posY + 300.0f) {
                    state = GARUGA_ATK_FIREBALL;
                } else {
                    state = GARUGA_ATK_FIREBALL_HIGH;
                }
            } else {
                dist = pos[1];
                if (dist <= em->posY - 200.0f) {
                    state = GARUGA_ATK_FIREBALL_LOW;
                } else if (dist <= em->posY + 400.0f) {
                    state = GARUGA_ATK_FIREBALL;
                } else {
                    state = GARUGA_ATK_FIREBALL_HIGH;
                }
            }
        }
        Em_SetTargetAngle(em);
        break;
    case GARUGA_ATK_STARE:
    case GARUGA_ATK_TAIL_SPIN:
    case GARUGA_ATK_ROAR:
    case GARUGA_ATK_SOMERSAULT:
    case GARUGA_ATK_TAIL_SPIN_B:
    case GARUGA_ATK_PECK_A:
        break;
    }
    Em_SetStateSync(em, EM_GROUP_ATTACK, state, mode);
}

/*
 * Starts a group-4 state: the statuses each reaction begins or ends, the
 * count and effect flag of the part break, the hold of the trap (holdTimer
 * 50), the guard of the trap hop, and the part break of
 * GARUGA_REACT_PART_BREAK_END (Em_BreakPart 5).
 */
void EmGaruga_SetReactState(Em *em, u16 state, u8 mode)
{
    em->motionSpeed = 1.0f;
    switch (state) {
    case GARUGA_REACT_PART_BREAK:
        Em_EndAllStatus(em);
        em->fxFlags |= 1;
        em->partBreakCount = (s8) em->partBreakCount + 1;
        break;
    case GARUGA_REACT_STATUS4:
        Em_BeginStatus4(em);
        break;
    case GARUGA_REACT_TRAPPED:
        em->holdTimer = 50;
        Em_EndAllStatus(em);
        break;
    case GARUGA_REACT_STATUS4_DOWNED:
        Em_BeginStatus4(em);
        break;
    case GARUGA_REACT_TRAP_ESCAPE_HOP:
        em->guardTimer = 2;
        Em_EndAllStatus(em);
        break;
    case GARUGA_REACT_STATUS4_DOWNED_END:
        Em_EndStatus4(em);
        break;
    case GARUGA_REACT_STATUS2_DOWNED_END:
        Em_EndStatus2(em);
        break;
    case GARUGA_REACT_STATUS1_DOWNED_END:
        Em_EndStatus1(em);
        break;
    case GARUGA_REACT_PART_BREAK_END:
        Em_ResetToBasePose(em);
        Em_BreakPart(em, 5U);
        break;
    case GARUGA_REACT_STUN:
        Em_BeginStatus5(em);
        break;
    case GARUGA_REACT_STUN_DOWNED:
        Em_BeginStatus5(em);
        break;
    case GARUGA_REACT_STUN_DOWNED_END:
        Em_EndStatus5(em);
        break;
    case GARUGA_REACT_HOLD2:
        Em_EndAllStatus(em);
        break;
    }
    Em_SetStateSync(em, EM_GROUP_REACT, state, mode);
}

/*
 * Changes the state: group 0 to 4 states go through their Set function, the
 * others straight to Em_SetStateSync (GARUGA_DEFEAT_REAPPEAR first lets the
 * quest show it, GARUGA_VS_WAIT_BOTH marks the defeat). The owner targets
 * the nearest player in range; remote copies skip EM_SYNC_OWNER and
 * EM_SYNC_OWNER_FLAGGED.
 */
void EmGaruga_ChangeState(Em *em, u16 group, u16 state, u8 mode)
{
    if (em->remote == 0) {
        Em_TargetPlayerInRange(em);
    } else if (mode == EM_SYNC_OWNER || mode == EM_SYNC_OWNER_FLAGGED) {
        return;
    }
    Em_LoadReactionSet(em, 0);
    switch (group) {
    case EM_GROUP_IDLE:
        EmGaruga_SetIdleState(em, state, mode);
        return;
    case EM_GROUP_MOVE:
        EmGaruga_SetMoveState(em, state, mode);
        return;
    case EM_GROUP_FLY:
        EmGaruga_SetFlyState(em, state, mode);
        return;
    case EM_GROUP_ATTACK:
        EmGaruga_SetAttackState(em, state, mode);
        return;
    case EM_GROUP_REACT:
        EmGaruga_SetReactState(em, state, mode);
        return;
    case EM_GROUP_DEFEAT:
        if ((state == GARUGA_DEFEAT_REAPPEAR) && (QuestEm_CanAppear(em) == 1)) {
            QuestEm_Appear(em);
        }
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        return;
    case EM_GROUP_VERSUS:
        if (state == GARUGA_VS_WAIT_BOTH) {
            Versus_MarkDefeat(em);
        }
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        return;
    case EM_GROUP_EVENT:
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        
    default:
        return;
    }
}

/*
 * Sets velY from vertical-speed curve kind (EmGaruga_MoveVelYCurves) at the
 * frame of the motion, while the motion is not blending, and moves the
 * monster along its yaw.
 */
void EmGaruga_MoveByVelYCurve(Em *em, s32 kind)
{
    EmWork *work = (EmWork *) &em->work;
    EmCurveKey *tbl;
    EmCurveKey *key;
    f32 frame;
    f32 keyFrame;
    s32 i;

    tbl = EmGaruga_MoveVelYCurves[kind];
    if (tbl != NULL && em->motion[0].blend == 0) {
        frame = em->motion[0].frame;
        if (frame == 0.0f || frame == 1.0f) {
            em->velY = tbl->v;
        } else {
            i = 1;
            while (i != 0) {
                key = &tbl[i];
                keyFrame = key->t;
                if (frame > keyFrame && keyFrame != 0.0f) {
                    i++;
                } else if (keyFrame == 0.0f) {
                    break;
                } else {
                    i = 0;
                    em->velY = (key->v - key[-1].v) / ((keyFrame - key[-1].t) / em->motion[0].step);
                }
            }
        }
        work->moveAngX = 0;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
    }
}

/*
 * Starts curve set kind of the family movement curves: picks which curves
 * run (vertical speed, forward speed) and their start frame, and clears the
 * velocity.
 */
void EmGaruga_StartCurves(Em *em, u8 kind)
{
    EmFlyerWork *w = (EmFlyerWork *)&em->work;

    w->curveNone = 0;
    w->curveVelY = 0;
    w->curveSpeed = 0;
    w->curveKind = kind;
    switch (w->curveKind) {
    case 2:
        w->curveVelY = 1;
        w->curveFrame = 38;
        break;
    case 3:
        w->curveVelY = 1;
        w->curveFrame = 30;
        break;
    case 5:
        w->curveVelY = 1;
        w->curveSpeed = 1;
        w->curveFrame = 60;
        break;
    case 8:
    case 14:
        w->curveVelY = 1;
        w->curveSpeed = 1;
        w->curveFrame = 76;
        break;
    case 9:
        w->curveVelY = 1;
        w->curveSpeed = 1;
        w->curveFrame = 0;
        break;
    case 10:
        w->curveVelY = 1;
        w->curveSpeed = 1;
        w->curveFrame = 92;
        break;
    case 11:
        w->curveVelY = 1;
        w->curveFrame = 0;
        break;
    case 12:
        w->curveVelY = 1;
        w->curveFrame = 36;
        break;
    case 13:
        w->curveVelY = 1;
        w->curveFrame = 0;
        break;
    case 0:
        break;
    }
    Player_ClearVelocity(em);
}

static s32 EmGaruga_CurveNone(Em *em, EmWork *work)
{
    return 0;
}


extern EmCurveKey *EmGaruga_VelYCurves[];

/*
 * The value of curve curveKind (a table of keys) at the frame in the work
 * area: sets em->velY from the key pair around the frame, 2 at the table's
 * end.
 */
static u8 EmGaruga_CurveVelY(Em *em, EmWork *work)
{
    EmFlyerWork *w = (EmFlyerWork *) work;
    EmCurveKey *key;
    f32 frame;
    f32 keyFrame;
    s32 i;
    EmCurveKey *tbl;
    u8 ret;

    tbl = EmGaruga_VelYCurves[((EmFlyerWork *)work)->curveKind];
    ret = 0;
    if (tbl == NULL) {
        return ret;
    }
    frame = w->curveFrame;
    if (frame != 0.0f && frame != 1.0f) {
        i = 1;
        while (i != 0) {
            key = &tbl[i];
            keyFrame = key->t;
            if (frame > keyFrame && keyFrame > 0.0f) {
                i++;
            } else if (keyFrame == 0.0f) {
                ret = 2;
                if (em->velY > -50.0f) {
                    em->velY -= 1.0f;
                }
                break;
            } else if (keyFrame < 0.0f) {
                w->curveFrame = 0;
                ret = 2;
                em->velY = 0.0f;
                break;
            } else {
                i = 0;
                em->velY = (key->v - key[-1].v) / ((keyFrame - key[-1].t) / em->motion[0].step);
            }
        }
    }
    return ret;
}

extern EmCurveKey *EmGaruga_SpeedCurves[];

/*
 * The value of curve curveKind at the frame in the work area: sets em->velZ
 * from the key pair around the frame, 4 at the table's end.
 */
static u8 EmGaruga_CurveSpeed(Em *em, EmWork *work)
{
    EmFlyerWork *w = (EmFlyerWork *) work;
    EmCurveKey *key;
    f32 frame;
    f32 keyFrame;
    s32 i;
    EmCurveKey *tbl;
    u8 ret;

    tbl = EmGaruga_SpeedCurves[((EmFlyerWork *)work)->curveKind];
    ret = 0;
    if (tbl == NULL) {
        return ret;
    }
    frame = w->curveFrame;
    if (frame != 0.0f && frame != 1.0f) {
        i = 1;
        while (i != 0) {
            key = &tbl[i];
            keyFrame = key->t;
            if (frame > keyFrame && keyFrame != 0.0f) {
                i++;
            } else if (keyFrame == 0.0f) {
                ret = 4;
                break;
            } else {
                i = 0;
                em->velZ = (key->v - key[-1].v) / ((keyFrame - key[-1].t) / em->motion[0].step);
            }
        }
    }
    return ret;
}

/* Runs the family's three curves when their flags in the work area (+0x438) are set, advances
 * the curve frame by the motion's step, and turns toward the target while unk390 is 2. */
u8 EmGaruga_UpdateCurves(Em *em)
{
    EmFlyerWork *w = (EmFlyerWork *)&em->work;
    u8 ret = 0;

    if (w->curveNone != 0) {
        ret = EmGaruga_CurveNone(em, (EmWork *) w);
    }
    if (w->curveVelY != 0) {
        ret |= EmGaruga_CurveVelY(em, (EmWork *) w);
    }
    if (w->curveSpeed != 0) {
        ret |= EmGaruga_CurveSpeed(em, (EmWork *) w);
    }
    w->curveFrame += (s16) em->motion[0].step;
    if (em->stance == EM_STANCE_AIR) {
        w->moveAngX = 0;
        w->moveAngY = em->rotY;
        w->moveAngZ = 0;
        Em_MoveRotatedAccel(em, &w->moveAngX);
    }
    return ret;
}


/* The file's initialised data, in the game's order */
EmFlyParam EmGaruga_FlyParam = {
#include "game/008C41E0.inc"
};
EmFlyParam EmGaruga_FlyParam2 = {
#include "game/008C4240.inc"
};
EmTurnParams EmGaruga_TurnParams[2] = {
#include "game/008C42A0.inc"
};
Vec3f EmGaruga_CarveThrowPt = {
#include "game/008C42D8.inc"
};
extern EmCurveKey EmCephadrome_VelYCurves_3[];
void *EmGaruga_Callbacks[8] = {
    EmGaruga_SpawnEffects, EmGaruga_SpawnEffects, EmGaruga_TableNop, EmGaruga_UpdateMotionEffects,
    GmEm_ReturnZero, EmGaruga_Reinit, EmGaruga_AllowPartBreak, NULL,
};
EmCurveKey EmGaruga_MoveVelYCurves_0[8] = {
#include "game/008C4310.inc"
};
EmCurveKey EmGaruga_MoveVelYCurves_1[20] = {
#include "game/008C4350.inc"
};
EmCurveKey EmGaruga_MoveVelYCurves_3[10] = {
#include "game/008C43F0.inc"
};
EmCurveKey EmGaruga_MoveVelYCurves_5[6] = {
#include "game/008C4440.inc"
};
EmCurveKey EmGaruga_MoveVelYCurves_6[10] = {
#include "game/008C4470.inc"
};
EmCurveKey *EmGaruga_MoveVelYCurves[16] = {
    EmGaruga_MoveVelYCurves_0, EmGaruga_MoveVelYCurves_1, NULL, EmGaruga_MoveVelYCurves_3,
    NULL, EmGaruga_MoveVelYCurves_5, EmGaruga_MoveVelYCurves_6, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
};
EmCurveKey EmGaruga_VelYCurves_3[10] = {
#include "game/008C4500.inc"
};
EmCurveKey EmGaruga_VelYCurves_5[14] = {
#include "game/008C4550.inc"
};
EmCurveKey EmGaruga_VelYCurves_8[8] = {
#include "game/008C45C0.inc"
};
EmCurveKey EmGaruga_VelYCurves_14[8] = {
#include "game/008C4600.inc"
};
EmCurveKey EmGaruga_VelYCurves_9[18] = {
#include "game/008C4640.inc"
};
EmCurveKey EmGaruga_VelYCurves_10[14] = {
#include "game/008C46D0.inc"
};
EmCurveKey EmGaruga_VelYCurves_11[4] = {
#include "game/008C4740.inc"
};
EmCurveKey EmGaruga_VelYCurves_12[6] = {
#include "game/008C4760.inc"
};
EmCurveKey EmGaruga_VelYCurves_13[24] = {
#include "game/008C4790.inc"
};
EmCurveKey *EmGaruga_VelYCurves[16] = {
    NULL, NULL, EmCephadrome_VelYCurves_3, EmGaruga_VelYCurves_3,
    NULL, EmGaruga_VelYCurves_5, NULL, NULL,
    EmGaruga_VelYCurves_8, EmGaruga_VelYCurves_9, EmGaruga_VelYCurves_10, EmGaruga_VelYCurves_11,
    EmGaruga_VelYCurves_12, EmGaruga_VelYCurves_13, EmGaruga_VelYCurves_14, NULL,
};
EmCurveKey EmGaruga_SpeedCurves_5[14] = {
#include "game/008C4890.inc"
};
EmCurveKey EmGaruga_SpeedCurves_8[8] = {
#include "game/008C4900.inc"
};
EmCurveKey EmGaruga_SpeedCurves_14[8] = {
#include "game/008C4940.inc"
};
EmCurveKey EmGaruga_SpeedCurves_9[14] = {
#include "game/008C4980.inc"
};
EmCurveKey EmGaruga_SpeedCurves_10[12] = {
#include "game/008C49F0.inc"
};
EmCurveKey *EmGaruga_SpeedCurves[16] = {
    NULL, NULL, NULL, NULL,
    NULL, EmGaruga_SpeedCurves_5, NULL, NULL,
    EmGaruga_SpeedCurves_8, EmGaruga_SpeedCurves_9, EmGaruga_SpeedCurves_10, NULL,
    NULL, NULL, EmGaruga_SpeedCurves_14, NULL,
};
