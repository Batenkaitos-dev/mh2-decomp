/*
 * Monster family: the Gravios, the Black Gravios and the Basarios
 * (EM_KIND_GRAVIOS, EM_KIND_BLACK_GRAVIOS and EM_KIND_BASARIOS), which run
 * the same code. From the family's first function, EmGravios_PlaySeOnFrame
 * (a sound at a motion frame), to the next family's: the motions' sounds,
 * EmGravios_Setup (which the per-kind dispatch Em_TypeSetup calls), the
 * states of its groups and their dispatchers, the setters, Update,
 * EmGravios_ChangeState, the effects made with its motions and the curves
 * of its flight. em_gravios.h holds its motions and states. The state
 * functions take the monster and the work area its state dispatcher passes
 * on (EmWork); Em.step is the step within the current state.
 */
#include "common.h"
#include "scene.h"
#include "quest.h"
#include "em.h"
#include "shadow.h"
#include "em_gravios.h"
#include "objattach.h"
#include "player.h"
#include "area.h"
#include "capsule.h"
#include "card.h"
#include "save.h"
#include "cam_shake.h"
#include "dust_flash_effects.h"
#include "engine_math.h"
#include "game_util.h"
#include "gm_em.h"
#include "gm_load.h"
#include "hit_effects.h"
#include "part_effects.h"
#include "rand.h"
#include "hit_spark.h"
#include "vec_math.h"
#include "em_script.h"
/* prototypes of callees */

/* prototypes */
void Em_AddHp(Em *em, s16 n);
/* unit's own declaration: declared here as the callers use it; with the definition's void (Em *em, u8 k, s16 v, s16 t, f32 y, f32 scale), the calls in EmGravios_DownedWhileHeld and EmGravios_Stun compile differently */
void StunStars_CreateEm(Em *, s32, s32, s32, f32, f32);
s32 EmMotion_PassedFrameScaled(void *p, f32 frame, s32 layer);
/* unit's own declaration: declared here as the callers use it; with the definition's s16 (void *obj, u8 state, u8 sub), the calls in EmGravios_IsHiding compile differently */
s32 Player_IsState(void *obj, s32 group, s32 state);
void Em_SpawnLyingPuffs(Em *em, s32 bone, f32 *ofs, f32 scale);
/* unit's own declaration: declared here as the callers use it; with the definition's void (Em *em, s32 group, s32 state, u8 mode), the calls in EmGravios_ChangeState, EmGravios_SetIdleState and others compile differently */
void Em_SetStateSync(Em *em, u16 group, u16 state, u8 mode);
void Em_EndAllStatus(Em *em);
extern EmTurnParams EmGravios_TurnParams[1];
void EmTail_Create(Em *);
void EmRangedAttack_CreateAtBone(Em *, s32, s32, s32, s32, s32);
extern u8 EmGravios_BasariosShotFrame;
extern u8 EmGravios_BasariosShotPitch;
extern Vec3f EmGravios_CarveThrowPt __attribute__((aligned(8)));
extern EmCurveKey *EmGravios_MoveVelYCurves[];
struct EmIdEntry;
struct EmIdEntry *Em_FindAreaEntry(Em *em, struct EmIdEntry *entry, s16 id);
extern void **gEmKindTables[];
extern f32 gStartPos[];
/* the calls that mask the value again (u8 or & 0xFF) cast it to s32 first: MWCC keeps the game's andi that way */
/* end of prototypes */

/*
 * Plays sound seId at bone when the layer-0 motion passes frame
 * (EmMotion_PassedFrameScaled). On a flagged floor
 * (EmGraviosWork.onFlaggedFloor) the sounds at GRAV_BONE_STEP_A and
 * GRAV_BONE_STEP_B play seId + 53 instead. Only EmGravios_MotionSounds calls it.
 */
void EmGravios_PlaySeOnFrame(Em *em, s32 frame, s32 seId, s32 bone)
{
    s32 seOfs;

    seOfs = 0;
    if (EmMotion_PassedFrameScaled(em, (f32) frame, 0U) != 0) {
        if (((EmGraviosWork *)&em->work)->onFlaggedFloor == 1 && (bone == GRAV_BONE_STEP_A || bone == GRAV_BONE_STEP_B)) {
            seOfs = 53;
        }
        Em_PlaySeAtBone(em, seId + seOfs, bone);
    }
}

/*
 * Switches on work->sndMotion (the layer-0 motion id) and plays the sounds
 * of that motion at their frames through EmGravios_PlaySeOnFrame (steps on
 * GRAV_BONE_STEP_A/_B, voice at GRAV_BONE_HEAD). GRAV_MOT_BEAM_SHORT has its own set
 * in GRAV_VS_POSE_C. GRAV_MOT_BEAM has one set for the Gravios and the Black
 * Gravios in EM_GROUP_IDLE and EM_GROUP_EVENT, another for them in the other
 * groups, and a third for the
 * Basarios.
 */
void EmGravios_MotionSounds(Em *em, EmWork *work)
{
    s16 motion;
    u8 type;
    u8 stateGroup;

    motion = work->sndMotion;
    switch (motion) {
    case EM_MOTION_ID(0, GRAV_MOT_IDLE):
        break;
    case EM_MOTION_ID(0, GRAV_MOT_WALK):
        EmGravios_PlaySeOnFrame(em, 44, GRAV_SE_STEP, GRAV_BONE_STEP_A);
        EmGravios_PlaySeOnFrame(em, 116, GRAV_SE_STEP, GRAV_BONE_STEP_B);
        break;
    case EM_MOTION_ID(0, GRAV_MOT_TURN_RIGHT):
        EmGravios_PlaySeOnFrame(em, 6, 72, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 25, GRAV_SE_STEP, GRAV_BONE_STEP_A);
        EmGravios_PlaySeOnFrame(em, 56, GRAV_SE_STEP, GRAV_BONE_STEP_B);
        break;
    case EM_MOTION_ID(0, GRAV_MOT_TURN_LEFT):
        EmGravios_PlaySeOnFrame(em, 6, 72, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 25, GRAV_SE_STEP, GRAV_BONE_STEP_B);
        EmGravios_PlaySeOnFrame(em, 56, GRAV_SE_STEP, GRAV_BONE_STEP_A);
        break;
    case EM_MOTION_ID(0, GRAV_MOT_LIMP):
        EmGravios_PlaySeOnFrame(em, 6, 83, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 6, 4, GRAV_BONE_STEP_B);
        EmGravios_PlaySeOnFrame(em, 76, 0, GRAV_BONE_STEP_A);
        EmGravios_PlaySeOnFrame(em, 28, 18, GRAV_BONE_STEP_A);
        break;
    case EM_MOTION_ID(0, GRAV_MOT_DESCEND):
        EmGravios_PlaySeOnFrame(em, 14, 11, GRAV_BONE_WING_A);
        EmGravios_PlaySeOnFrame(em, 18, 11, GRAV_BONE_WING_B);
        EmGravios_PlaySeOnFrame(em, 70, 11, GRAV_BONE_WING_A);
        EmGravios_PlaySeOnFrame(em, 66, 11, GRAV_BONE_WING_B);
        EmGravios_PlaySeOnFrame(em, 122, 11, GRAV_BONE_WING_A);
        EmGravios_PlaySeOnFrame(em, 126, 11, GRAV_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, 13):
        EmGravios_PlaySeOnFrame(em, 14, 15, GRAV_BONE_WING_A);
        EmGravios_PlaySeOnFrame(em, 18, 15, GRAV_BONE_WING_B);
        EmGravios_PlaySeOnFrame(em, 44, 15, GRAV_BONE_WING_A);
        EmGravios_PlaySeOnFrame(em, 40, 15, GRAV_BONE_WING_B);
        EmGravios_PlaySeOnFrame(em, 66, 15, GRAV_BONE_WING_A);
        EmGravios_PlaySeOnFrame(em, 70, 15, GRAV_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, GRAV_MOT_HOVER):
        EmGravios_PlaySeOnFrame(em, 4, 12, GRAV_BONE_WING_A);
        EmGravios_PlaySeOnFrame(em, 8, 12, GRAV_BONE_WING_B);
        EmGravios_PlaySeOnFrame(em, 58, 11, GRAV_BONE_WING_A);
        EmGravios_PlaySeOnFrame(em, 62, 11, GRAV_BONE_WING_B);
        EmGravios_PlaySeOnFrame(em, 142, 11, GRAV_BONE_WING_A);
        EmGravios_PlaySeOnFrame(em, 138, 11, GRAV_BONE_WING_B);
        EmGravios_PlaySeOnFrame(em, 212, 11, GRAV_BONE_WING_A);
        EmGravios_PlaySeOnFrame(em, 216, 11, GRAV_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, 16):
        break;
    case EM_MOTION_ID(0, GRAV_MOT_CHARGE):
        EmGravios_PlaySeOnFrame(em, 6, 79, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 28, GRAV_SE_STEP, GRAV_BONE_STEP_A);
        EmGravios_PlaySeOnFrame(em, 160, 6, GRAV_BONE_STEP_A);
        EmGravios_PlaySeOnFrame(em, 192, 6, GRAV_BONE_STEP_B);
        break;
    case EM_MOTION_ID(0, GRAV_MOT_TAKE_OFF):
        EmGravios_PlaySeOnFrame(em, 96, GRAV_SE_STEP, GRAV_BONE_STEP_A);
        EmGravios_PlaySeOnFrame(em, 22, GRAV_SE_STEP, GRAV_BONE_STEP_A);
        EmGravios_PlaySeOnFrame(em, 4, 35, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 80, 15, GRAV_BONE_WING_A);
        EmGravios_PlaySeOnFrame(em, 84, 15, GRAV_BONE_WING_B);
        EmGravios_PlaySeOnFrame(em, 98, 7, GRAV_BONE_STEP_B);
        EmGravios_PlaySeOnFrame(em, 168, 11, GRAV_BONE_WING_A);
        EmGravios_PlaySeOnFrame(em, 172, 11, GRAV_BONE_WING_B);
        EmGravios_PlaySeOnFrame(em, 222, 11, GRAV_BONE_WING_A);
        EmGravios_PlaySeOnFrame(em, 226, 11, GRAV_BONE_WING_B);
        EmGravios_PlaySeOnFrame(em, 272, 11, GRAV_BONE_WING_A);
        EmGravios_PlaySeOnFrame(em, 276, 11, GRAV_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, GRAV_MOT_TOUCH_DOWN):
        EmGravios_PlaySeOnFrame(em, 2, 11, GRAV_BONE_WING_A);
        EmGravios_PlaySeOnFrame(em, 6, 11, GRAV_BONE_WING_B);
        EmGravios_PlaySeOnFrame(em, 12, 4, GRAV_BONE_STEP_B);
        EmGravios_PlaySeOnFrame(em, 4, GRAV_SE_STEP, GRAV_BONE_STEP_A);
        EmGravios_PlaySeOnFrame(em, 168, 3, GRAV_BONE_STEP_A);
        EmGravios_PlaySeOnFrame(em, 200, 0, GRAV_BONE_STEP_B);
        EmGravios_PlaySeOnFrame(em, 14, 9, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 124, 22, GRAV_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, GRAV_MOT_IDLE_LEAD_IN):
        EmGravios_PlaySeOnFrame(em, 30, 19, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 2, 72, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 4, 14, GRAV_BONE_WING_B);
        EmGravios_PlaySeOnFrame(em, 8, 14, GRAV_BONE_WING_A);
        break;
    case EM_MOTION_ID(0, 30):
        break;
    case EM_MOTION_ID(0, GRAV_MOT_LIE):
        EmGravios_PlaySeOnFrame(em, 36, 87, GRAV_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, 35):
        EmGravios_PlaySeOnFrame(em, 50, 2, GRAV_BONE_STEP_A);
        EmGravios_PlaySeOnFrame(em, 58, 6, GRAV_BONE_STEP_B);
        EmGravios_PlaySeOnFrame(em, 84, GRAV_SE_STEP, GRAV_BONE_STEP_B);
        EmGravios_PlaySeOnFrame(em, 104, 4, GRAV_BONE_STEP_A);
        EmGravios_PlaySeOnFrame(em, 124, 17, GRAV_BONE_STEP_B);
        EmGravios_PlaySeOnFrame(em, 204, 6, GRAV_BONE_STEP_B);
        EmGravios_PlaySeOnFrame(em, 210, 2, GRAV_BONE_STEP_B);
        EmGravios_PlaySeOnFrame(em, 254, 18, GRAV_BONE_STEP_A);
        EmGravios_PlaySeOnFrame(em, 288, 2, GRAV_BONE_STEP_B);
        EmGravios_PlaySeOnFrame(em, 32, 15, GRAV_BONE_WING_B);
        EmGravios_PlaySeOnFrame(em, 36, 15, GRAV_BONE_WING_A);
        EmGravios_PlaySeOnFrame(em, 128, 15, GRAV_BONE_WING_B);
        EmGravios_PlaySeOnFrame(em, 132, 15, GRAV_BONE_WING_A);
        EmGravios_PlaySeOnFrame(em, 290, 14, GRAV_BONE_WING_B);
        EmGravios_PlaySeOnFrame(em, 294, 14, GRAV_BONE_WING_A);
        EmGravios_PlaySeOnFrame(em, 2, 21, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 2, 35, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 48, 33, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 128, 34, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 184, 36, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 288, 34, GRAV_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, GRAV_MOT_ATTACK_A):
        EmGravios_PlaySeOnFrame(em, 36, 36, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 20, 0, GRAV_BONE_STEP_B);
        EmGravios_PlaySeOnFrame(em, 40, 0, GRAV_BONE_STEP_A);
        EmGravios_PlaySeOnFrame(em, 60, 21, GRAV_BONE_BREATH);
        EmGravios_PlaySeOnFrame(em, 60, 12, GRAV_BONE_WING_B);
        EmGravios_PlaySeOnFrame(em, 12, 12, GRAV_BONE_WING_A);
        EmGravios_PlaySeOnFrame(em, 166, 0, GRAV_BONE_STEP_B);
        break;
    case EM_MOTION_ID(0, GRAV_MOT_GET_UP):
        EmGravios_PlaySeOnFrame(em, 30, 72, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 12, 12, GRAV_BONE_STEP_B);
        EmGravios_PlaySeOnFrame(em, 28, 2, GRAV_BONE_STEP_B);
        EmGravios_PlaySeOnFrame(em, 46, 3, GRAV_BONE_STEP_A);
        EmGravios_PlaySeOnFrame(em, 72, GRAV_SE_STEP, GRAV_BONE_STEP_B);
        break;
    case EM_MOTION_ID(0, GRAV_MOT_COLLAPSE):
        EmGravios_PlaySeOnFrame(em, 4, 87, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 72, 22, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 164, 9, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 144, 3, GRAV_BONE_WING_B);
        EmGravios_PlaySeOnFrame(em, 136, 14, GRAV_BONE_WING_A);
        EmGravios_PlaySeOnFrame(em, 178, 4, GRAV_BONE_BREATH);
        break;
    case EM_MOTION_ID(0, GRAV_MOT_REST_LIE_DOWN):
        EmGravios_PlaySeOnFrame(em, 2, 87, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 2, 23, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 64, 3, GRAV_BONE_STEP_B);
        EmGravios_PlaySeOnFrame(em, 102, 16, GRAV_BONE_STEP_B);
        break;
    case EM_MOTION_ID(0, GRAV_MOT_SPIN_BASARIOS):
        EmGravios_PlaySeOnFrame(em, 8, 35, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 14, 15, GRAV_BONE_WING_B);
        EmGravios_PlaySeOnFrame(em, 30, 20, 42);
        EmGravios_PlaySeOnFrame(em, 42, 0, GRAV_BONE_STEP_A);
        EmGravios_PlaySeOnFrame(em, 72, GRAV_SE_STEP, GRAV_BONE_STEP_B);
        break;
    case EM_MOTION_ID(0, GRAV_MOT_SPIN):
        EmGravios_PlaySeOnFrame(em, 8, 35, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 14, 15, GRAV_BONE_WING_A);
        EmGravios_PlaySeOnFrame(em, 30, 20, 42);
        EmGravios_PlaySeOnFrame(em, 42, 0, GRAV_BONE_STEP_B);
        EmGravios_PlaySeOnFrame(em, 72, GRAV_SE_STEP, GRAV_BONE_STEP_A);
        break;
    case EM_MOTION_ID(0, 44):
        EmGravios_PlaySeOnFrame(em, 28, 11, GRAV_BONE_WING_A);
        EmGravios_PlaySeOnFrame(em, 32, 11, GRAV_BONE_WING_B);
        EmGravios_PlaySeOnFrame(em, 6, 26, GRAV_BONE_BREATH);
        EmGravios_PlaySeOnFrame(em, 10, 27, GRAV_BONE_WING_B);
        EmGravios_PlaySeOnFrame(em, 14, 27, GRAV_BONE_WING_A);
        EmGravios_PlaySeOnFrame(em, 38, 26, GRAV_BONE_BREATH);
        EmGravios_PlaySeOnFrame(em, 42, 27, GRAV_BONE_WING_B);
        EmGravios_PlaySeOnFrame(em, 50, 27, GRAV_BONE_WING_A);
        break;
    case EM_MOTION_ID(0, 47):
        EmGravios_PlaySeOnFrame(em, 2, 18, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 80, 24, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 20, 40, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 74, 41, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 76, 13, GRAV_BONE_WING_A);
        EmGravios_PlaySeOnFrame(em, 72, 13, GRAV_BONE_WING_B);
        EmGravios_PlaySeOnFrame(em, 180, 19, GRAV_BONE_WING_A);
        EmGravios_PlaySeOnFrame(em, 187, 19, GRAV_BONE_WING_B);
        EmGravios_PlaySeOnFrame(em, 240, 18, GRAV_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, 48):
        EmGravios_PlaySeOnFrame(em, 2, 34, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 34, 34, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 64, 34, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 114, 32, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 16, GRAV_SE_STEP, GRAV_BONE_STEP_A);
        break;
    case EM_MOTION_ID(0, 49):
        EmGravios_PlaySeOnFrame(em, 32, 15, GRAV_BONE_WING_A);
        EmGravios_PlaySeOnFrame(em, 36, 15, GRAV_BONE_WING_B);
        EmGravios_PlaySeOnFrame(em, 116, 11, GRAV_BONE_WING_A);
        EmGravios_PlaySeOnFrame(em, 120, 11, GRAV_BONE_WING_B);
        EmGravios_PlaySeOnFrame(em, 190, 11, GRAV_BONE_WING_A);
        EmGravios_PlaySeOnFrame(em, 194, 11, GRAV_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, 50):
        EmGravios_PlaySeOnFrame(em, 78, 41, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 78, 24, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 120, 24, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 164, 24, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 4, 18, GRAV_BONE_STEP_B);
        EmGravios_PlaySeOnFrame(em, 4, 19, GRAV_BONE_BREATH);
        EmGravios_PlaySeOnFrame(em, 64, 15, GRAV_BONE_WING_A);
        EmGravios_PlaySeOnFrame(em, 64, 15, GRAV_BONE_WING_B);
        EmGravios_PlaySeOnFrame(em, 250, 32, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 298, 18, GRAV_BONE_STEP_A);
        EmGravios_PlaySeOnFrame(em, 286, 19, GRAV_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, 51):
        EmGravios_PlaySeOnFrame(em, 4, 19, 42);
        EmGravios_PlaySeOnFrame(em, 38, 16, GRAV_BONE_STEP_B);
        EmGravios_PlaySeOnFrame(em, 56, 18, GRAV_BONE_STEP_B);
        EmGravios_PlaySeOnFrame(em, 88, 6, GRAV_BONE_STEP_B);
        EmGravios_PlaySeOnFrame(em, 150, 46, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 154, 46, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 238, 22, GRAV_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, 52):
        EmGravios_PlaySeOnFrame(em, 22, 40, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 72, 41, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 82, 24, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 6, 20, GRAV_BONE_BREATH);
        EmGravios_PlaySeOnFrame(em, 22, 0, GRAV_BONE_STEP_B);
        EmGravios_PlaySeOnFrame(em, 58, 15, GRAV_BONE_WING_A);
        EmGravios_PlaySeOnFrame(em, 62, 15, GRAV_BONE_WING_B);
        EmGravios_PlaySeOnFrame(em, 106, 11, GRAV_BONE_WING_A);
        EmGravios_PlaySeOnFrame(em, 110, 11, GRAV_BONE_WING_B);
        EmGravios_PlaySeOnFrame(em, 172, 11, GRAV_BONE_WING_A);
        EmGravios_PlaySeOnFrame(em, 176, 11, GRAV_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, GRAV_MOT_TURN_ROAR):
        EmGravios_PlaySeOnFrame(em, 2, 18, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 28, 4, GRAV_BONE_STEP_B);
        EmGravios_PlaySeOnFrame(em, 56, 4, GRAV_BONE_STEP_A);
        EmGravios_PlaySeOnFrame(em, 28, 5, GRAV_BONE_STEP_B);
        EmGravios_PlaySeOnFrame(em, 120, 4, GRAV_BONE_STEP_A);
        EmGravios_PlaySeOnFrame(em, 264, 4, GRAV_BONE_STEP_B);
        EmGravios_PlaySeOnFrame(em, 22, 32, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 32, 40, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 132, 39, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 132, 81, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 10, 12, GRAV_BONE_WING_B);
        EmGravios_PlaySeOnFrame(em, 16, 12, GRAV_BONE_WING_A);
        break;
    case EM_MOTION_ID(0, GRAV_MOT_FLINCH_B):
        EmGravios_PlaySeOnFrame(em, 6, 83, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 212, 35, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 72, 19, 42);
        EmGravios_PlaySeOnFrame(em, 48, 6, GRAV_BONE_STEP_B);
        EmGravios_PlaySeOnFrame(em, 48, 17, GRAV_BONE_STEP_A);
        EmGravios_PlaySeOnFrame(em, 48, 3, GRAV_BONE_STEP_A);
        EmGravios_PlaySeOnFrame(em, 174, GRAV_SE_STEP, GRAV_BONE_STEP_B);
        EmGravios_PlaySeOnFrame(em, 222, 0, GRAV_BONE_STEP_A);
        EmGravios_PlaySeOnFrame(em, 52, 12, GRAV_BONE_WING_A);
        EmGravios_PlaySeOnFrame(em, 56, 13, GRAV_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, GRAV_MOT_FLINCH_F):
        EmGravios_PlaySeOnFrame(em, 6, 83, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 10, 18, GRAV_BONE_STEP_B);
        EmGravios_PlaySeOnFrame(em, 38, 6, GRAV_BONE_STEP_B);
        EmGravios_PlaySeOnFrame(em, 78, 4, GRAV_BONE_STEP_A);
        EmGravios_PlaySeOnFrame(em, 110, 3, GRAV_BONE_STEP_A);
        EmGravios_PlaySeOnFrame(em, 172, 0, GRAV_BONE_STEP_B);
        EmGravios_PlaySeOnFrame(em, 90, 21, GRAV_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, GRAV_MOT_SHAKE_OFF):
        EmGravios_PlaySeOnFrame(em, 4, 79, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 86, 31, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 16, 3, GRAV_BONE_STEP_A);
        EmGravios_PlaySeOnFrame(em, 4, 13, GRAV_BONE_WING_A);
        EmGravios_PlaySeOnFrame(em, 8, 13, GRAV_BONE_WING_B);
        EmGravios_PlaySeOnFrame(em, 126, 0, GRAV_BONE_STEP_B);
        EmGravios_PlaySeOnFrame(em, 98, 19, 42);
        break;
    case EM_MOTION_ID(0, GRAV_MOT_FLINCH_D):
        EmGravios_PlaySeOnFrame(em, 14, GRAV_SE_STEP, GRAV_BONE_STEP_B);
        EmGravios_PlaySeOnFrame(em, 4, 82, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 4, 19, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 76, 31, GRAV_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, GRAV_MOT_FLINCH_E):
        EmGravios_PlaySeOnFrame(em, 4, 82, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 4, 19, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 68, 32, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 120, GRAV_SE_STEP, GRAV_BONE_STEP_B);
        break;
    case EM_MOTION_ID(0, GRAV_MOT_PART_BREAK):
        EmGravios_PlaySeOnFrame(em, 4, 84, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 62, 4, GRAV_BONE_TAIL_B);
        EmGravios_PlaySeOnFrame(em, 18, 18, GRAV_BONE_STEP_A);
        EmGravios_PlaySeOnFrame(em, 44, 6, GRAV_BONE_STEP_B);
        EmGravios_PlaySeOnFrame(em, 58, 16, GRAV_BONE_STEP_B);
        EmGravios_PlaySeOnFrame(em, 62, 9, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 74, 9, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 100, 18, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 14, 12, GRAV_BONE_WING_A);
        EmGravios_PlaySeOnFrame(em, 18, 12, GRAV_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, GRAV_MOT_FLINCH_C):
        EmGravios_PlaySeOnFrame(em, 4, 82, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 4, 19, GRAV_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, 67):
        EmGravios_PlaySeOnFrame(em, 70, 85, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 70, 23, GRAV_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, GRAV_MOT_DIE):
        EmGravios_PlaySeOnFrame(em, 28, 2, GRAV_BONE_STEP_B);
        EmGravios_PlaySeOnFrame(em, 48, 4, GRAV_BONE_STEP_A);
        EmGravios_PlaySeOnFrame(em, 220, 16, GRAV_BONE_STEP_B);
        EmGravios_PlaySeOnFrame(em, 30, 22, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 264, 9, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 264, 7, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 276, 23, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 384, 22, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 24, 82, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 140, 40, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 180, 44, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 314, 47, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 402, 43, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 372, 15, GRAV_BONE_WING_A);
        EmGravios_PlaySeOnFrame(em, 374, 14, GRAV_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, GRAV_MOT_KNOCK_DOWN_PART3):
    case EM_MOTION_ID(0, GRAV_MOT_KNOCK_DOWN):
        EmGravios_PlaySeOnFrame(em, 4, 82, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 22, 6, 42);
        EmGravios_PlaySeOnFrame(em, 4, 18, GRAV_BONE_STEP_B);
        EmGravios_PlaySeOnFrame(em, 16, 12, GRAV_BONE_WING_A);
        EmGravios_PlaySeOnFrame(em, 26, 13, GRAV_BONE_WING_B);
        EmGravios_PlaySeOnFrame(em, 146, 9, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 146, 7, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 186, 7, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 238, 7, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 284, 8, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 284, 7, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 108, 22, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 242, 22, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 410, 22, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 458, GRAV_SE_STEP, GRAV_BONE_STEP_B);
        EmGravios_PlaySeOnFrame(em, 494, 0, GRAV_BONE_STEP_A);
        break;
    case EM_MOTION_ID(0, 70):
    case EM_MOTION_ID(0, 75):
        EmGravios_PlaySeOnFrame(em, 8, 22, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 4, 31, GRAV_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, 71):
        EmGravios_PlaySeOnFrame(em, 4, 22, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 30, 32, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 48, GRAV_SE_STEP, GRAV_BONE_STEP_A);
        EmGravios_PlaySeOnFrame(em, 70, GRAV_SE_STEP, GRAV_BONE_STEP_B);
        break;
    case EM_MOTION_ID(0, 72):
        EmGravios_PlaySeOnFrame(em, 4, 22, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 30, 32, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 48, GRAV_SE_STEP, GRAV_BONE_STEP_B);
        EmGravios_PlaySeOnFrame(em, 70, GRAV_SE_STEP, GRAV_BONE_STEP_A);
        break;
    case EM_MOTION_ID(0, 73):
        EmGravios_PlaySeOnFrame(em, 4, 30, GRAV_BONE_BREATH);
        EmGravios_PlaySeOnFrame(em, 4, 29, GRAV_BONE_WING_A);
        EmGravios_PlaySeOnFrame(em, 8, 29, GRAV_BONE_WING_B);
        EmGravios_PlaySeOnFrame(em, 10, 42, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 76, 32, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 24, 18, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 30, 16, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 30, 9, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 34, 7, GRAV_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, GRAV_MOT_STATUS4):
        EmGravios_PlaySeOnFrame(em, 16, 90, GRAV_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, GRAV_MOT_FALL):
        EmGravios_PlaySeOnFrame(em, 4, 83, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 20, 11, GRAV_BONE_WING_B);
        EmGravios_PlaySeOnFrame(em, 58, 20, GRAV_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, GRAV_MOT_FALL_LAND):
        EmGravios_PlaySeOnFrame(em, 10, 9, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 10, 7, 42);
        EmGravios_PlaySeOnFrame(em, 12, 17, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 16, 84, GRAV_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, 79):
        EmGravios_PlaySeOnFrame(em, 12, 36, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 36, 34, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 102, 34, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 164, 34, GRAV_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, GRAV_MOT_IDLE_PAIR_A):
        EmGravios_PlaySeOnFrame(em, 2, 22, GRAV_BONE_BREATH);
        EmGravios_PlaySeOnFrame(em, 100, 23, GRAV_BONE_BREATH);
        EmGravios_PlaySeOnFrame(em, 38, 85, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 200, 32, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 28, 3, GRAV_BONE_STEP_B);
        EmGravios_PlaySeOnFrame(em, 320, 3, GRAV_BONE_STEP_B);
        break;
    case EM_MOTION_ID(0, GRAV_MOT_IDLE_PAIR_B):
        EmGravios_PlaySeOnFrame(em, 2, 22, GRAV_BONE_BREATH);
        EmGravios_PlaySeOnFrame(em, 100, 23, GRAV_BONE_BREATH);
        EmGravios_PlaySeOnFrame(em, 38, 85, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 200, 32, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 28, 3, GRAV_BONE_STEP_A);
        EmGravios_PlaySeOnFrame(em, 320, 3, GRAV_BONE_STEP_A);
        break;
    case EM_MOTION_ID(0, GRAV_MOT_DIE_FALLING):
        EmGravios_PlaySeOnFrame(em, 4, 22, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 50, 44, GRAV_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, 83):
        EmGravios_PlaySeOnFrame(em, 4, 22, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 4, 23, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 10, 16, GRAV_BONE_STEP_B);
        EmGravios_PlaySeOnFrame(em, 10, 0, GRAV_BONE_STEP_B);
        EmGravios_PlaySeOnFrame(em, 28, 0, GRAV_BONE_STEP_A);
        EmGravios_PlaySeOnFrame(em, 52, 0, GRAV_BONE_STEP_B);
        EmGravios_PlaySeOnFrame(em, 88, 0, GRAV_BONE_STEP_A);
        EmGravios_PlaySeOnFrame(em, 128, 0, GRAV_BONE_STEP_B);
        EmGravios_PlaySeOnFrame(em, 198, 0, GRAV_BONE_STEP_A);
        EmGravios_PlaySeOnFrame(em, 4, 83, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 168, 39, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 162, 13, GRAV_BONE_WING_A);
        EmGravios_PlaySeOnFrame(em, 166, 13, GRAV_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, 84):
        EmGravios_PlaySeOnFrame(em, 22, 13, GRAV_BONE_WING_A);
        EmGravios_PlaySeOnFrame(em, 26, 13, GRAV_BONE_WING_B);
        EmGravios_PlaySeOnFrame(em, 52, 10, GRAV_BONE_WING_A);
        EmGravios_PlaySeOnFrame(em, 56, 14, GRAV_BONE_WING_B);
        EmGravios_PlaySeOnFrame(em, 78, 14, GRAV_BONE_WING_A);
        EmGravios_PlaySeOnFrame(em, 74, 10, GRAV_BONE_WING_B);
        EmGravios_PlaySeOnFrame(em, 88, 12, GRAV_BONE_WING_A);
        EmGravios_PlaySeOnFrame(em, 92, 12, GRAV_BONE_WING_B);
        EmGravios_PlaySeOnFrame(em, 32, 5, GRAV_BONE_STEP_B);
        EmGravios_PlaySeOnFrame(em, 28, 0, GRAV_BONE_STEP_A);
        break;
    case EM_MOTION_ID(0, 87):
        EmGravios_PlaySeOnFrame(em, 4, 26, GRAV_BONE_BREATH);
        EmGravios_PlaySeOnFrame(em, 8, 27, GRAV_BONE_WING_B);
        EmGravios_PlaySeOnFrame(em, 4, 27, GRAV_BONE_WING_A);
        EmGravios_PlaySeOnFrame(em, 34, 28, GRAV_BONE_BREATH);
        EmGravios_PlaySeOnFrame(em, 64, 28, GRAV_BONE_BREATH);
        break;
    case EM_MOTION_ID(0, 90):
        EmGravios_PlaySeOnFrame(em, 32, 13, GRAV_BONE_WING_A);
        EmGravios_PlaySeOnFrame(em, 36, 13, GRAV_BONE_WING_B);
        EmGravios_PlaySeOnFrame(em, 74, 13, GRAV_BONE_WING_A);
        EmGravios_PlaySeOnFrame(em, 78, 13, GRAV_BONE_WING_B);
        EmGravios_PlaySeOnFrame(em, 124, 11, GRAV_BONE_WING_A);
        EmGravios_PlaySeOnFrame(em, 128, 11, GRAV_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, GRAV_MOT_TRAPPED):
        EmGravios_PlaySeOnFrame(em, 4, 16, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 10, 18, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 20, 9, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 4, 39, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 4, 82, GRAV_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, GRAV_MOT_STRUGGLE):
        EmGravios_PlaySeOnFrame(em, 22, 18, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 72, 18, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 4, 12, GRAV_BONE_WING_A);
        EmGravios_PlaySeOnFrame(em, 8, 12, GRAV_BONE_WING_B);
        EmGravios_PlaySeOnFrame(em, 48, 19, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 4, 40, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 40, 83, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 4, 22, GRAV_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, GRAV_MOT_DOWNED):
        EmGravios_PlaySeOnFrame(em, 34, 22, GRAV_BONE_WING_A);
        EmGravios_PlaySeOnFrame(em, 38, 22, GRAV_BONE_WING_B);
        EmGravios_PlaySeOnFrame(em, 98, 4, GRAV_BONE_BREATH);
        EmGravios_PlaySeOnFrame(em, 108, 3, GRAV_BONE_BREATH);
        EmGravios_PlaySeOnFrame(em, 4, 82, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 4, 44, GRAV_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, GRAV_MOT_DOWNED_GET_UP):
        EmGravios_PlaySeOnFrame(em, 4, 22, GRAV_BONE_WING_A);
        EmGravios_PlaySeOnFrame(em, 8, 22, GRAV_BONE_WING_B);
        EmGravios_PlaySeOnFrame(em, 108, 3, GRAV_BONE_BREATH);
        EmGravios_PlaySeOnFrame(em, 4, 80, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 70, 32, GRAV_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, GRAV_MOT_CHARGE_STOP):
        EmGravios_PlaySeOnFrame(em, 22, 4, GRAV_BONE_STEP_B);
        EmGravios_PlaySeOnFrame(em, 24, 8, GRAV_BONE_STEP_A);
        EmGravios_PlaySeOnFrame(em, 26, 16, GRAV_BONE_STEP_B);
        EmGravios_PlaySeOnFrame(em, 200, GRAV_SE_STEP, GRAV_BONE_STEP_B);
        EmGravios_PlaySeOnFrame(em, 224, 0, GRAV_BONE_STEP_A);
        EmGravios_PlaySeOnFrame(em, 166, 22, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 100, 32, GRAV_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, GRAV_MOT_IDLE_ENRAGED):
        EmGravios_PlaySeOnFrame(em, 4, 23, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 396, 22, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 50, 32, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 126, 87, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 376, 31, GRAV_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, GRAV_MOT_IDLE_ACTION_A):
        EmGravios_PlaySeOnFrame(em, 4, 22, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 104, 22, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 4, 35, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 146, 0, GRAV_BONE_STEP_B);
        EmGravios_PlaySeOnFrame(em, 206, 0, GRAV_BONE_STEP_A);
        break;
    case EM_MOTION_ID(0, GRAV_MOT_EAT):
        EmGravios_PlaySeOnFrame(em, 56, 34, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 174, 34, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 280, 34, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 350, 31, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 28, GRAV_SE_STEP, GRAV_BONE_STEP_B);
        EmGravios_PlaySeOnFrame(em, 366, 3, GRAV_BONE_STEP_A);
        EmGravios_PlaySeOnFrame(em, 406, 0, GRAV_BONE_STEP_B);
        break;
    case EM_MOTION_ID(0, GRAV_MOT_BEAM_SHORT):
        if ((em->stateGroup == EM_GROUP_VERSUS) && (em->state == 3)) {
            EmGravios_PlaySeOnFrame(em, 114, 39, GRAV_BONE_HEAD);
            EmGravios_PlaySeOnFrame(em, 62, 36, GRAV_BONE_HEAD);
            EmGravios_PlaySeOnFrame(em, 14, 13, GRAV_BONE_WING_A);
            EmGravios_PlaySeOnFrame(em, 18, 13, GRAV_BONE_WING_B);
            EmGravios_PlaySeOnFrame(em, 20, 6, GRAV_BONE_STEP_A);
            EmGravios_PlaySeOnFrame(em, 254, GRAV_SE_STEP, GRAV_BONE_STEP_A);
            EmGravios_PlaySeOnFrame(em, 290, 0, GRAV_BONE_STEP_A);
            break;
        }
        EmGravios_PlaySeOnFrame(em, 114, 81, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 62, 39, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 94, 27, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 120, 26, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 120, 24, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 14, 13, GRAV_BONE_WING_A);
        EmGravios_PlaySeOnFrame(em, 18, 13, GRAV_BONE_WING_B);
        EmGravios_PlaySeOnFrame(em, 20, 6, GRAV_BONE_STEP_A);
        EmGravios_PlaySeOnFrame(em, 254, GRAV_SE_STEP, GRAV_BONE_STEP_A);
        EmGravios_PlaySeOnFrame(em, 290, 0, GRAV_BONE_STEP_A);
        break;
    case EM_MOTION_ID(0, GRAV_MOT_ROAR):
        EmGravios_PlaySeOnFrame(em, 48, 32, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 108, GRAV_SE_ROAR, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 4, 18, GRAV_BONE_STEP_A);
        EmGravios_PlaySeOnFrame(em, 406, 0, GRAV_BONE_STEP_B);
        break;
    case EM_MOTION_ID(0, GRAV_MOT_RISE_BURST):
        EmGravios_PlaySeOnFrame(em, 104, 81, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 104, 21, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 4, 22, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 38, 22, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 72, 9, GRAV_BONE_WING_B);
        EmGravios_PlaySeOnFrame(em, 78, 9, GRAV_BONE_WING_B);
        EmGravios_PlaySeOnFrame(em, 78, 16, GRAV_BONE_BREATH);
        EmGravios_PlaySeOnFrame(em, 118, 4, GRAV_BONE_STEP_A);
        EmGravios_PlaySeOnFrame(em, 126, 3, GRAV_BONE_STEP_B);
        EmGravios_PlaySeOnFrame(em, 88, 15, GRAV_BONE_STEP_A);
        EmGravios_PlaySeOnFrame(em, 92, 15, GRAV_BONE_WING_B);
        EmGravios_PlaySeOnFrame(em, 248, 0, GRAV_BONE_STEP_B);
        break;
    case EM_MOTION_ID(0, GRAV_MOT_ATTACK_B):
        EmGravios_PlaySeOnFrame(em, 4, 31, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 22, 18, GRAV_BONE_STEP_B);
        EmGravios_PlaySeOnFrame(em, 26, 3, GRAV_BONE_STEP_B);
        EmGravios_PlaySeOnFrame(em, 142, 4, GRAV_BONE_STEP_A);
        EmGravios_PlaySeOnFrame(em, 292, GRAV_SE_STEP, GRAV_BONE_STEP_B);
        EmGravios_PlaySeOnFrame(em, 354, 3, GRAV_BONE_STEP_A);
        EmGravios_PlaySeOnFrame(em, 90, 19, 42);
        EmGravios_PlaySeOnFrame(em, 74, 21, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 74, 8, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 152, 22, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 86, 8, GRAV_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, GRAV_MOT_BEAM):
        type = em->type;
        if ((type == 17) || (type == 47)) {
            stateGroup = em->stateGroup;
            if ((stateGroup != EM_GROUP_IDLE) && (stateGroup != EM_GROUP_EVENT)) {
                EmGravios_PlaySeOnFrame(em, 130, 81, GRAV_BONE_HEAD);
                EmGravios_PlaySeOnFrame(em, 14, 13, GRAV_BONE_WING_A);
                EmGravios_PlaySeOnFrame(em, 18, 13, GRAV_BONE_WING_B);
                EmGravios_PlaySeOnFrame(em, 26, 6, GRAV_BONE_STEP_A);
                EmGravios_PlaySeOnFrame(em, 62, 39, GRAV_BONE_HEAD);
                EmGravios_PlaySeOnFrame(em, 104, 27, GRAV_BONE_HEAD);
                EmGravios_PlaySeOnFrame(em, 104, 25, GRAV_BONE_HEAD);
                EmGravios_PlaySeOnFrame(em, 144, 26, GRAV_BONE_HEAD);
                EmGravios_PlaySeOnFrame(em, 144, 24, GRAV_BONE_HEAD);
                break;
            }
            EmGravios_PlaySeOnFrame(em, 110, 84, GRAV_BONE_HEAD);
            EmGravios_PlaySeOnFrame(em, 14, 13, GRAV_BONE_WING_A);
            EmGravios_PlaySeOnFrame(em, 18, 13, GRAV_BONE_WING_B);
            EmGravios_PlaySeOnFrame(em, 26, 6, GRAV_BONE_STEP_A);
            EmGravios_PlaySeOnFrame(em, 62, 39, GRAV_BONE_HEAD);
            EmGravios_PlaySeOnFrame(em, 130, 81, GRAV_BONE_HEAD);
            EmGravios_PlaySeOnFrame(em, 104, 27, GRAV_BONE_HEAD);
            EmGravios_PlaySeOnFrame(em, 104, 25, GRAV_BONE_HEAD);
            EmGravios_PlaySeOnFrame(em, 144, 26, GRAV_BONE_HEAD);
            EmGravios_PlaySeOnFrame(em, 144, 24, GRAV_BONE_HEAD);
            EmGravios_PlaySeOnFrame(em, 146, 29, GRAV_BONE_HEAD);
            break;
        }
        EmGravios_PlaySeOnFrame(em, 20, 6, GRAV_BONE_STEP_A);
        EmGravios_PlaySeOnFrame(em, 60, 39, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 14, 13, GRAV_BONE_WING_A);
        EmGravios_PlaySeOnFrame(em, 18, 13, GRAV_BONE_WING_B);
        EmGravios_PlaySeOnFrame(em, 64, 38, GRAV_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, GRAV_MOT_GAS):
        EmGravios_PlaySeOnFrame(em, 20, 35, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 88, 36, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 126, GRAV_SE_ROAR, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 20, GRAV_SE_STEP, GRAV_BONE_STEP_B);
        EmGravios_PlaySeOnFrame(em, 68, 2, GRAV_BONE_STEP_A);
        break;
    case EM_MOTION_ID(0, GRAV_MOT_REAR_ATTACK):
        EmGravios_PlaySeOnFrame(em, 50, 40, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 104, 39, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 56, 4, GRAV_BONE_STEP_A);
        EmGravios_PlaySeOnFrame(em, 248, GRAV_SE_STEP, GRAV_BONE_STEP_A);
        EmGravios_PlaySeOnFrame(em, 46, 20, 42);
        EmGravios_PlaySeOnFrame(em, 104, 20, 42);
        EmGravios_PlaySeOnFrame(em, 156, 20, 42);
        EmGravios_PlaySeOnFrame(em, 210, 20, 42);
        EmGravios_PlaySeOnFrame(em, 274, 19, 42);
        break;
    case EM_MOTION_ID(0, GRAV_MOT_GAS_END):
        EmGravios_PlaySeOnFrame(em, 64, 22, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 72, 79, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 112, GRAV_SE_STEP, GRAV_BONE_STEP_A);
        EmGravios_PlaySeOnFrame(em, 152, 0, GRAV_BONE_STEP_B);
        break;
    case EM_MOTION_ID(0, GRAV_MOT_BEAM_END):
        EmGravios_PlaySeOnFrame(em, 10, 32, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 62, 28, GRAV_BONE_WING_A);
        EmGravios_PlaySeOnFrame(em, 66, 28, GRAV_BONE_WING_B);
        EmGravios_PlaySeOnFrame(em, 122, 22, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 120, 31, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 202, GRAV_SE_STEP, GRAV_BONE_STEP_A);
        EmGravios_PlaySeOnFrame(em, 230, 0, GRAV_BONE_STEP_B);
        break;
    case EM_MOTION_ID(0, GRAV_MOT_FLINCH):
        EmGravios_PlaySeOnFrame(em, 4, 22, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 50, 71, GRAV_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, GRAV_MOT_STUN):
        EmGravios_PlaySeOnFrame(em, 4, 83, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 52, 41, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 348, 82, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 442, 82, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 32, 22, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 152, 22, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 192, 16, GRAV_BONE_STEP_A);
        EmGravios_PlaySeOnFrame(em, 210, 22, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 214, 8, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 300, 7, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 334, 13, GRAV_BONE_WING_A);
        EmGravios_PlaySeOnFrame(em, 348, 22, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 456, 22, GRAV_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, GRAV_MOT_BEAM_LONG):
        EmGravios_PlaySeOnFrame(em, 114, 81, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 4, 13, GRAV_BONE_WING_A);
        EmGravios_PlaySeOnFrame(em, 8, 13, GRAV_BONE_WING_B);
        EmGravios_PlaySeOnFrame(em, 22, 6, GRAV_BONE_STEP_A);
        EmGravios_PlaySeOnFrame(em, 10, 39, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 66, 27, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 66, 25, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 114, 26, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 114, 24, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 256, 32, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 310, 14, GRAV_BONE_WING_A);
        EmGravios_PlaySeOnFrame(em, 314, 14, GRAV_BONE_WING_B);
        EmGravios_PlaySeOnFrame(em, 272, 22, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 302, GRAV_SE_STEP, GRAV_BONE_STEP_A);
        EmGravios_PlaySeOnFrame(em, 338, 0, GRAV_BONE_STEP_B);
        break;
    case EM_MOTION_ID(0, GRAV_MOT_GET_UP_B):
        EmGravios_PlaySeOnFrame(em, 24, 17, GRAV_BONE_STEP_B);
        EmGravios_PlaySeOnFrame(em, 112, GRAV_SE_STEP, GRAV_BONE_STEP_B);
        EmGravios_PlaySeOnFrame(em, 166, 0, GRAV_BONE_STEP_A);
        EmGravios_PlaySeOnFrame(em, 40, 22, GRAV_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, GRAV_MOT_LEAP):
        EmGravios_PlaySeOnFrame(em, 22, GRAV_SE_STEP, GRAV_BONE_STEP_A);
        EmGravios_PlaySeOnFrame(em, 18, 36, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 12, 22, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 60, 15, GRAV_BONE_WING_A);
        EmGravios_PlaySeOnFrame(em, 64, 15, GRAV_BONE_WING_B);
        EmGravios_PlaySeOnFrame(em, 62, 7, GRAV_BONE_STEP_B);
        EmGravios_PlaySeOnFrame(em, 138, 14, GRAV_BONE_WING_A);
        EmGravios_PlaySeOnFrame(em, 142, 14, GRAV_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, GRAV_MOT_BURROW):
        EmGravios_PlaySeOnFrame(em, 16, 17, GRAV_BONE_STEP_B);
        EmGravios_PlaySeOnFrame(em, 78, 16, GRAV_BONE_STEP_A);
        EmGravios_PlaySeOnFrame(em, 14, 49, GRAV_BONE_WING_A);
        EmGravios_PlaySeOnFrame(em, 44, 49, GRAV_BONE_WING_B);
        EmGravios_PlaySeOnFrame(em, 62, 49, GRAV_BONE_WING_A);
        EmGravios_PlaySeOnFrame(em, 104, 51, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 130, 22, GRAV_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, GRAV_MOT_SURFACE):
        EmGravios_PlaySeOnFrame(em, 46, 35, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 110, 45, GRAV_BONE_HEAD);
        EmGravios_PlaySeOnFrame(em, 4, 51, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 6, 22, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 90, 23, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 174, 16, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 218, 22, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 278, 22, GRAV_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, GRAV_MOT_LEAP_LAND):
        EmGravios_PlaySeOnFrame(em, 2, 9, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 14, 7, GRAV_BONE_ROOT);
        EmGravios_PlaySeOnFrame(em, 130, 0, GRAV_BONE_STEP_B);
        EmGravios_PlaySeOnFrame(em, 172, 0, GRAV_BONE_STEP_A);

        break;
    }
}

/*
 * The family entry in Em_TypeSetup (also run by EmGravios_SetIdleState for
 * GRAV_IDLE_SURFACE). A Basarios starts out of combat (Em_SetCombat(em, 0,
 * 0)). The other kinds start out of combat only in an area with day id
 * AREA_VOLCANO_7, where they also get noReact and
 * senseDelay/noHitTimer/noPushTimer 5.
 */
void EmGravios_Setup(Em *em)
{
    if (em->type == EM_KIND_BASARIOS) {
        Em_SetCombat(em, 0, 0);
        return;
    }
    if (Area_DayId(em->area) == AREA_VOLCANO_7) {
        Em_SetCombat(em, 0, 0);
        em->noReact = 1;
        em->senseDelay = 5;
        em->noHitTimer = 5;
        em->noPushTimer = 5;
    }
}

/*
 * Callbacks[0] and [1]: creates the family's attached objects, the alert
 * mark (AlertMark_Create(em, GRAV_BONE_BREATH, 0)) and the shadow (Shadow_Spawn(em, SHADOW_MODEL)),
 * as EmRathalos_SpawnEffects does.
 */
void EmGravios_SpawnEffects(void *em)
{
    AlertMark_Create(em, GRAV_BONE_BREATH, 0);
    Shadow_Spawn((Player *) em, SHADOW_MODEL);
}

/*
 * Sets a monster up: the scaled parts 3, 4 and 8; in the stage with no quest
 * (gQuest.stage 0) it faces backward (rotY a half turn) at x 5000 + 1500 per
 * slot, z 9000. It starts in GRAV_IDLE_STAND with full hp (Em_ComputeMaxHp),
 * in the ground stance, with the flight work's goalYaw, pitch, turn and roll
 * speeds and moveAngB set. It keeps only bit 0 of fxFlags (set once the part
 * has broken); while that bit is clear it hides mesh 1 and creates the tail
 * object (EmTail_Create), for all three kinds.
 */
void EmGravios_Reset(Em *em)
{
    EmFlyerWork *w = (EmFlyerWork *)&em->work;

    f32 slotF;
    s16 maxHp;
    u8 slotNo;
    u8 fxFlags;

    Em_SetPartScaled(em, 3);
    Em_SetPartScaled(em, 4);
    Em_SetPartScaled(em, 8);
    if (gQuest.stage == 0) {
        em->rotY = ANGLE_TURN / 2;
        slotNo = em->slotNo;
        slotF = (f32) (u32) slotNo;
        em->posX = 5000.0f + (1500.0f * slotF);
        em->posY = 0.0f;
        em->posZ = 9000.0f;
    }
    em->stateGroup = EM_GROUP_IDLE;
    em->state = 0;
    maxHp = Em_ComputeMaxHp(em, 0);
    em->hp = maxHp;
    em->hpMax = maxHp;
    em->stance = EM_STANCE_GROUND;
    w->fxMotion = 0;
    em->blind = 0;
    em->unk_InitFlag = 1;
    em->noAlert = 0;
    em->painted = 0;
    ((EmWork *)w)->goalYaw = 0x4000;
    w->pitchStep = 0x100;
    ((EmWork *)w)->turnSpeed = 512;
    w->rollSpeed = 0x100;
    w->moveAngBY = ANGLE_TURN / 8;
    w->moveAngBX = 0;
    w->unk048 = 0;
    w->unk049 = 0;
    w->unk04A = 0;
    fxFlags = em->fxFlags & 1;
    em->fxFlags = fxFlags;
    if (fxFlags == 0) {
        switch (em->type) {
        case EM_KIND_GRAVIOS:
        case EM_KIND_BLACK_GRAVIOS:
        case EM_KIND_BASARIOS:
            em->meshShown[1] = 0;
            EmTail_Create(em);
            break;
        }
    }
}

/*
 * Called by gm_em to start a monster of the family: EmGravios_Reset, then
 * the Gravios and the Black Gravios idle (GRAV_MOT_IDLE, GRAV_IDLE,
 * scriptPending 1) and the Basarios hides as a rock (GRAV_MOT_ROCK_POSE,
 * GRAV_IDLE_HIDE_AS_ROCK, scriptPending 0).
 */
void EmGravios_Start(Em *em)
{
    EmGravios_Reset(em);
    if (em->type == EM_KIND_GRAVIOS || em->type == EM_KIND_BLACK_GRAVIOS) {
        Em_SetMotionRow(em, GRAV_MOT_IDLE, 0, 0);
        EmGravios_ChangeState(em, EM_GROUP_IDLE, GRAV_IDLE, EM_SYNC_LOCAL);
        em->scriptPending = 1;
    } else {
        Em_SetMotionRow(em, GRAV_MOT_ROCK_POSE, 0, 0);
        EmGravios_ChangeState(em, EM_GROUP_IDLE, GRAV_IDLE_HIDE_AS_ROCK, EM_SYNC_LOCAL);
        em->scriptPending = 0;
    }
}

/*
 * Callbacks[5]: just EmGravios_Reset.
 */
void EmGravios_Reinit(Em *em)
{
    EmGravios_Reset(em);
}

/*
 * Ends a ground state: asks the script for the next state (scriptPending),
 * sets motionSpeed 1, the ground stance and lookAtTarget 0, then goes to
 * GRAV_IDLE (the setter picks GRAV_IDLE_ENRAGED while em->rage). Its two
 * arguments are unused (always 0).
 */
void EmGravios_EndState(Em *em, s32 unused1, s32 unused2)
{
    em->scriptPending = 1;
    em->motionSpeed = 1.0f;
    em->stance = EM_STANCE_GROUND;
    em->lookAtTarget = 0;
    EmGravios_ChangeState(em, EM_GROUP_IDLE, GRAV_IDLE, EM_SYNC_LOCAL);
}

/*
 * Ends a flight state: asks the script for the next state (scriptPending),
 * sets motionSpeed 1, EM_STANCE_AIR and lookAtTarget 0, then goes to
 * GRAV_FLY_HOVER_ONCE. Its argument is unused (callers pass 0 or 1).
 */
void EmGravios_EndStateInAir(Em *em, s32 unused)
{
    em->scriptPending = 1;
    em->motionSpeed = 1.0f;
    em->stance = EM_STANCE_AIR;
    em->lookAtTarget = 0;
    EmGravios_ChangeState(em, EM_GROUP_FLY, GRAV_FLY_HOVER_ONCE, EM_SYNC_LOCAL);
}

/*
 * When layer 0, 1 or 2 has stopped playing, restarts the idle of that layer
 * (GRAV_MOT_IDLE on that layer, blended over 10 frames), so a layer-1 gesture can play while
 * layers 0 and 2 idle (like EmKutku_KeepLayerIdle).
 */
void EmGravios_KeepLayerIdle(Em *em, s32 layer)
{
    if (em->motion[layer].playing == 0) {
        switch (layer) {
        case 0:
            Em_SetLayerMotion(em, EM_MOTION_ID(0, GRAV_MOT_IDLE), 10, 0, 0);
            break;
        case 1:
            Em_SetLayerMotion(em, EM_MOTION_ID(1, GRAV_MOT_IDLE), 10, 0, 1);
            break;
        case 2:
            Em_SetLayerMotion(em, EM_MOTION_ID(2, GRAV_MOT_IDLE), 10, 0, 2);
            break;
        }
    }
}

/*
 * GRAV_IDLE_STAND: restarts the idle on the three layers (layers 1 and 2
 * always restart: their tests never hold), then counts em->timer down and
 * sets scriptPending when it runs out.
 */
void EmGravios_Stand(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        if (em->motionId[0] != EM_MOTION_ID(0, GRAV_MOT_IDLE)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(0, GRAV_MOT_IDLE), 10, 0, 0);
        }
        /* Layers 1 and 2 are tested against 1101 and 1201, their idle's ids
         * at 100 a layer where the ids take 200 (EM_MOTION_ID), so the tests
         * always hold and both layers always restart. The Kut-Ku's stand
         * state has the same slip. */
        if (em->motionId[1] != 1101) {
            Em_SetLayerMotion(em, EM_MOTION_ID(1, GRAV_MOT_IDLE), 10, 0, 1);
        }
        if (em->motionId[2] != 1201) {
            Em_SetLayerMotion(em, EM_MOTION_ID(2, GRAV_MOT_IDLE), 10, 0, 2);
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
 * GRAV_IDLE: plays GRAV_MOT_IDLE_LEAD_IN, then the idle once, then
 * EmGravios_EndState (on the local machine only).
 */
void EmGravios_Idle(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GRAV_MOT_IDLE_LEAD_IN, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, GRAV_MOT_IDLE, 0, 0);
        }
        break;
    case 2:
        if ((em->remote == 0) && (em->motion[0].playing == 0)) {
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * GRAV_IDLE_ENRAGED (GRAV_IDLE while enraged): plays GRAV_MOT_IDLE_ENRAGED
 * once, then EmGravios_EndState.
 */
void EmGravios_IdleEnraged(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GRAV_MOT_IDLE_ENRAGED, 0, 0);
        break;
    case 1:
        if ((em->remote == 0) && (em->motion[0].playing == 0)) {
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * GRAV_IDLE_TIMED_LOOP: plays GRAV_MOT_IDLE_LOOP_B for (stateRand & 3) * 300
 * frames, then EmGravios_EndState.
 */
void EmGravios_IdleTimedLoop(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GRAV_MOT_IDLE_LOOP_B, 0, 0);
        em->timer = (em->stateRand & 3) * 300;
        break;
    case 1:
        if (em->remote == 0) {
            if (--em->timer <= 0) {
                em->step++;
                EmGravios_EndState(em, 0, 0);
            }
        }
        break;
    }
}

/*
 * GRAV_IDLE_ACTION_A: plays GRAV_MOT_IDLE_ACTION_A once, then
 * EmGravios_EndState.
 */
void EmGravios_IdleActionA(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GRAV_MOT_IDLE_ACTION_A, 0, 0);
        break;
    case 1:
        if ((em->remote == 0) && (em->motion[0].playing == 0)) {
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * GRAV_IDLE_UPPER_A: plays the layer-1 gesture GRAV_MOT_UPPER_A while layers
 * 0 and 2 idle (EmGravios_KeepLayerIdle), with a voice sound at a layer-1 frame. Calls
 * EmGravios_EndState when layer 1 stops.
 */
void EmGravios_UpperGestureA(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GRAV_MOT_UPPER_A, 0, 0);
        Em_SetLayerMotion(em, EM_MOTION_ID(0, GRAV_MOT_IDLE), 10, 0, 0);
        Em_SetLayerMotion(em, EM_MOTION_ID(2, GRAV_MOT_IDLE), 10, 0, 2);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
    EmGravios_KeepLayerIdle(em, 0);
    EmGravios_KeepLayerIdle(em, 2);
    Em_PlaySeAtBoneOnLayerFrame(em, 32, 87, GRAV_BONE_HEAD, 1);
}

/*
 * GRAV_IDLE_UPPER_B: plays the layer-1 gesture GRAV_MOT_UPPER_B with two
 * voice sounds, as EmGravios_UpperGestureA.
 */
void EmGravios_UpperGestureB(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GRAV_MOT_UPPER_B, 0, 0);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
    EmGravios_KeepLayerIdle(em, 0);
    EmGravios_KeepLayerIdle(em, 2);
    Em_PlaySeAtBoneOnLayerFrame(em, 32, 32, GRAV_BONE_HEAD, 1);
    Em_PlaySeAtBoneOnLayerFrame(em, 160, 23, GRAV_BONE_HEAD, 1);
}

/*
 * GRAV_IDLE_UPPER_E: plays the layer-1 gesture GRAV_MOT_UPPER_E with one
 * voice sound, as EmGravios_UpperGestureA.
 */
void EmGravios_UpperGestureE(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GRAV_MOT_UPPER_E, 0, 0);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
    EmGravios_KeepLayerIdle(em, 0);
    EmGravios_KeepLayerIdle(em, 2);
    Em_PlaySeAtBoneOnLayerFrame(em, 4, 85, GRAV_BONE_HEAD, 1);
}

/*
 * GRAV_IDLE_ROCK_POSE: blind every frame. Holds GRAV_MOT_ROCK_POSE with
 * reaction set 1 for (stateRand & 3) * 300 frames, then calls
 * EmGravios_EndState. Unlike the hide states, it does not hide the shadow.
 */
void EmGravios_RockPose(Em *em, EmWork *work)
{
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GRAV_MOT_ROCK_POSE, 0, 0);
        Em_LoadReactionSet(em, 1);
        em->timer = (em->stateRand & 3) * 300;
        break;
    case 1:
        if (em->remote == 0) {
            if (--em->timer <= 0) {
                em->step++;
                EmGravios_EndState(em, 0, 0);
            }
        }
        break;
    }
}

/*
 * GRAV_IDLE_PAIR_A: plays GRAV_MOT_IDLE_PAIR_A or GRAV_MOT_IDLE_PAIR_B
 * (stateRand bit 0) once, then EmGravios_EndState.
 */
void EmGravios_IdlePairA(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        if (em->stateRand & 1) {
            Em_SetMotion(em, GRAV_MOT_IDLE_PAIR_A, 0, 0);
            break;
        }
        Em_SetMotion(em, GRAV_MOT_IDLE_PAIR_B, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * GRAV_IDLE_TURN_TO_GOAL: plays GRAV_MOT_TURN_ROAR with goalYaw set to the
 * angle to the goal, and snaps rotY to it once within 0x200. From frame 60
 * it waits for layer 1 to stop, then calls EmGravios_EndState.
 */
void EmGravios_IdleTurnToGoal(Em *em, EmWork *work)
{
    u16 goalYaw;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GRAV_MOT_TURN_ROAR, 0, 0);
        work->goalYaw = Math_AngleXZ(&em->posX, em->goal);
        break;
    case 1:
        goalYaw = work->goalYaw;
        if ((u32) ((((goalYaw - *(u16 *)&em->rotY) & 0xFFFF) + 0x200) & 0xFFFF) < 0x400U) {
            em->rotY = (s32) goalYaw;
        }
        if (EmMotion_ReachedFrame(em, 0, 60.0f) != 0) {
            em->step++;
        }
        break;
    case 2:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * GRAV_IDLE_UPPER_C: plays the layer-1 gesture GRAV_MOT_UPPER_C with two
 * voice sounds, as EmGravios_UpperGestureA.
 */
void EmGravios_UpperGestureC(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GRAV_MOT_UPPER_C, 0, 0);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
    EmGravios_KeepLayerIdle(em, 0);
    EmGravios_KeepLayerIdle(em, 2);
    Em_PlaySeAtBoneOnLayerFrame(em, 50, 35, GRAV_BONE_HEAD, 1);
    Em_PlaySeAtBoneOnLayerFrame(em, 104, 31, GRAV_BONE_HEAD, 1);
}

/*
 * GRAV_IDLE_ROCK_BURST: guardTimer 2. Plays GRAV_MOT_RISE_BURST, clears
 * blind, leaves a timed gathering point at its position
 * (EmMine_CreateTimed0), then EmGravios_EndState. EmGravios_ChangeState
 * forces this state for any change made during GRAV_IDLE_HIDE_AS_ROCK or
 * GRAV_IDLE_REHIDE_AS_ROCK (EmGravios_IsHiding).
 */
void EmGravios_BurstFromRock(Em *em, EmWork *work)
{
    em->guardTimer = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GRAV_MOT_RISE_BURST, 0, 0);
        em->blind = 0;
        EmMine_CreateTimed0(em, &em->posX);
        break;
    case 1:
        if ((em->remote == 0) && (em->motion[0].playing == 0)) {
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * GRAV_IDLE_UPPER_D: plays the layer-1 gesture GRAV_MOT_UPPER_D with two
 * voice sounds, as EmGravios_UpperGestureA.
 */
void EmGravios_UpperGestureD(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GRAV_MOT_UPPER_D, 0, 0);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
    EmGravios_KeepLayerIdle(em, 0);
    EmGravios_KeepLayerIdle(em, 2);
    Em_PlaySeAtBoneOnLayerFrame(em, 2, 32, GRAV_BONE_HEAD, 1);
    Em_PlaySeAtBoneOnLayerFrame(em, 74, 31, GRAV_BONE_HEAD, 1);
}

/*
 * GRAV_IDLE_TURN_AND_BEAM (setter: Em_SetTargetAngle): turns with
 * Em_TurnToTargetMotion (range 3640; walk, turn left, turn right), then
 * plays GRAV_MOT_BEAM, then GRAV_MOT_BEAM_END (Gravios and Black Gravios
 * only). Ends with EmGravios_EndState. The same steps as
 * EmGravios_TurnAndBeamOrShot without its EmRangedAttack shots;
 * EmGravios_MotionEffects still creates the GraviosAttack hitboxes of the motions.
 */
void EmGravios_IdleTurnAndBeam(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        break;
    case 1:
        if (Em_TurnToTargetMotion(em, 3640, 2, 6, 5) != 0) {
            if (em->type == EM_KIND_BASARIOS) {
                em->step = 3;
            } else {
                em->step++;
            }
            em->lookAtTarget = 0;
            Em_SetMotion(em, GRAV_MOT_BEAM, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->lookAtTarget = 0;
            em->step++;
            Em_SetMotion(em, GRAV_MOT_BEAM_END, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->lookAtTarget = 0;
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * GRAV_IDLE_PAIR_B: plays GRAV_MOT_IDLE_PAIR_A, the idle,
 * GRAV_MOT_IDLE_PAIR_B, the idle (each idle until its blend ends), with two
 * voice sounds; then EmGravios_EndState.
 */
void EmGravios_IdlePairB(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GRAV_MOT_IDLE_PAIR_A, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, GRAV_MOT_IDLE, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].blend == 0) {
            em->step++;
            Em_SetMotion(em, GRAV_MOT_IDLE_PAIR_B, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, GRAV_MOT_IDLE, 0, 0);
        }
        break;
    case 4:
        if (em->motion[0].blend == 0) {
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
    Em_PlaySeAtBoneOnLayerFrame(em, 32, 87, GRAV_BONE_HEAD, 1);
    Em_PlaySeAtBoneOnLayerFrame(em, 282, 32, GRAV_BONE_HEAD, 1);
}

/*
 * GRAV_IDLE_PAIR_C: plays GRAV_MOT_IDLE_PAIR_B, the idle,
 * GRAV_MOT_IDLE_PAIR_A, the idle; then EmGravios_EndState.
 */
void EmGravios_IdlePairC(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GRAV_MOT_IDLE_PAIR_B, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, GRAV_MOT_IDLE, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].blend == 0) {
            em->step++;
            Em_SetMotion(em, GRAV_MOT_IDLE_PAIR_A, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, GRAV_MOT_IDLE, 0, 0);
        }
        break;
    case 4:
        if (em->motion[0].blend == 0) {
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * GRAV_IDLE_THREAT: plays GRAV_MOT_ROAR once, then EmGravios_EndState.
 */
void EmGravios_Threat(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GRAV_MOT_ROAR, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * GRAV_IDLE_ACTION_C: hit react NO_FLINCH. Plays GRAV_MOT_IDLE_ACTION_C with
 * sight row 1 and reaction set 1, sets both back to 0 at its end, then calls
 * EmGravios_EndState.
 */
void EmGravios_IdleActionC(Em *em, EmWork *work)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_NO_FLINCH);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GRAV_MOT_EAT, 0, 0);
        Em_SelectSightRow(em, 1);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SelectSightRow(em, 0);
            Em_LoadReactionSet(em, 0);
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * GRAV_IDLE_REST (setter: Em_ResetNotice): hit react WAKE, blind. Plays
 * GRAV_MOT_REST_LIE_DOWN (reaction set 1), then GRAV_MOT_LIE for 1800 frames
 * at +1 hp a frame. Then sets full hp, calls Em_ClearLowHpNeed and goes to
 * GRAV_IDLE_REST_END (mode 4).
 */
void EmGravios_Rest(Em *em, EmWork *work)
{
    s16 hpMax;

    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GRAV_MOT_REST_LIE_DOWN, 0, 0);
        Em_LoadReactionSet(em, 1);
        em->timer = 1800;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, GRAV_MOT_LIE, 0, 0);
        }
        break;
    case 2:
        hpMax = em->hpMax;
        if (em->hp < hpMax) {
            Em_AddHp(em, 1);
        } else {
            Em_AddHp(em, hpMax);
        }
        if (--em->timer <= 0) {
            em->step++;
            Em_AddHp(em, em->hpMax);
            Em_ClearLowHpNeed(em);
            EmGravios_ChangeState(em, EM_GROUP_IDLE, GRAV_IDLE_REST_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmGravios_ChangeState(em, EM_GROUP_IDLE, GRAV_IDLE_REST_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * GRAV_IDLE_STATUS2_COLLAPSE (setter: Em_BeginStatus2): hit react WAKE,
 * blind. Plays GRAV_MOT_COLLAPSE (reaction set 1), then GRAV_MOT_LIE until
 * em->timer runs out, then goes to GRAV_IDLE_STATUS2_END (mode 4).
 */
void EmGravios_Status2Collapse(Em *em, EmWork *work)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GRAV_MOT_COLLAPSE, 0, 0);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, GRAV_MOT_LIE, 0, 0);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            EmGravios_ChangeState(em, EM_GROUP_IDLE, GRAV_IDLE_STATUS2_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmGravios_ChangeState(em, EM_GROUP_IDLE, GRAV_IDLE_STATUS2_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * GRAV_IDLE_HIDE_AS_ROCK (the Basarios start; setter: motionSpeed 0): shadow
 * hidden, blind, hit react NO_FLINCH. Holds GRAV_MOT_ROCK_POSE (reaction set
 * 1) for 5400 frames, then goes to GRAV_IDLE_ROCK_BURST.
 */
void EmGravios_HideAsRock(Em *em, EmWork *work)
{
    em->hideShadowHold = 2;
    em->hideShadow = 2;
    em->blind = 2;
    Em_SetHitReactMode(em, EM_HIT_REACT_NO_FLINCH);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GRAV_MOT_ROCK_POSE, 0, 0);
        Em_LoadReactionSet(em, 1);
        em->timer = 5400;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmGravios_ChangeState(em, EM_GROUP_IDLE, GRAV_IDLE_ROCK_BURST, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmGravios_ChangeState(em, EM_GROUP_IDLE, GRAV_IDLE_ROCK_BURST, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * GRAV_IDLE_REST_END, GRAV_IDLE_STATUS2_END and GRAV_IDLE_STATUS1_END
 * (setter: Em_RefillNeed3, Em_EndStatus2, Em_EndStatus1): plays
 * GRAV_MOT_GET_UP with reaction set 0, blind while it plays, and leaves a
 * timed gathering point (EmMine_CreateTimed0). Then goes to
 * GRAV_IDLE_SHAKE_OFF.
 */
void EmGravios_GetUp(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, GRAV_MOT_GET_UP, 0, 0);
        Em_LoadReactionSet(em, 0);
        em->blind = 2;
        EmMine_CreateTimed0(em, &em->posX);
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            EmGravios_ChangeState(em, EM_GROUP_IDLE, GRAV_IDLE_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmGravios_ChangeState(em, EM_GROUP_IDLE, GRAV_IDLE_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * GRAV_IDLE_SHAKE_OFF, STATUS4_SHAKE_OFF, HOLD2_SHAKE_OFF and STUN_SHAKE_OFF
 * (only the per-state work of the setter tells them apart): plays
 * GRAV_MOT_SHAKE_OFF once, then EmGravios_EndState.
 */
void EmGravios_ShakeOff(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, GRAV_MOT_SHAKE_OFF, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * GRAV_IDLE_REHIDE_AS_ROCK (setter: motionSpeed 0, Em_SetCombat(em, 0, 0)):
 * works like EmGravios_HideAsRock but clears noReact, and holds 600 + 5400 frames
 * before GRAV_IDLE_ROCK_BURST.
 */
void EmGravios_RehideAsRock(Em *em, EmWork *work)
{
    em->hideShadowHold = 2;
    em->hideShadow = 2;
    em->blind = 2;
    Em_SetHitReactMode(em, EM_HIT_REACT_NO_FLINCH);
    switch (em->step) {
    case 0:
        em->noReact = 0;
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GRAV_MOT_ROCK_POSE, 0, 0);
        Em_LoadReactionSet(em, 1);
        em->timer = 600;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            em->timer = 5400;
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            EmGravios_ChangeState(em, EM_GROUP_IDLE, GRAV_IDLE_ROCK_BURST, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmGravios_ChangeState(em, EM_GROUP_IDLE, GRAV_IDLE_ROCK_BURST, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * GRAV_IDLE_STATUS1_COLLAPSE (setter: Em_BeginStatus1): plays
 * GRAV_MOT_COLLAPSE, then GRAV_MOT_LIE until em->timer runs out, as
 * EmGravios_Status2Collapse. Then goes to GRAV_IDLE_STATUS1_END.
 */
void EmGravios_Status1Collapse(Em *em, EmWork *work)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GRAV_MOT_COLLAPSE, 0, 0);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, GRAV_MOT_LIE, 0, 0);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            EmGravios_ChangeState(em, EM_GROUP_IDLE, GRAV_IDLE_STATUS1_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmGravios_ChangeState(em, EM_GROUP_IDLE, GRAV_IDLE_STATUS1_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * GRAV_IDLE_STATUS2_DOWNED (setter: Em_BeginStatus2): hit react WAKE,
 * holdTimer 5, blind. Holds GRAV_MOT_DOWNED (reaction set 1) with lying
 * puffs and a periodic sound (EmGravios_DownedPuffsAndSe) until em->timer
 * runs out, then goes to GRAV_REACT_STATUS2_DOWNED_END (Em_EndStatus2) in
 * mode 3.
 */
void EmGravios_Status2Downed(Em *em, EmWork *work)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->holdTimer = 5;
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GRAV_MOT_DOWNED, 0, 0);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmGravios_ChangeState(em, EM_GROUP_REACT, GRAV_REACT_STATUS2_DOWNED_END, EM_SYNC_OWNER);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmGravios_ChangeState(em, EM_GROUP_REACT, GRAV_REACT_STATUS2_DOWNED_END, EM_SYNC_OWNER);
        }
        break;
    }
    EmGravios_DownedPuffsAndSe(em);
}

/*
 * GRAV_IDLE_STATUS1_DOWNED: the same as EmGravios_Status2Downed, but it
 * leaves to GRAV_REACT_STATUS1_DOWNED_END, which ends status 1. Its setter
 * begins status 2, as for GRAV_IDLE_STATUS2_DOWNED.
 */
void EmGravios_Status1Downed(Em *em, EmWork *work)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->holdTimer = 5;
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GRAV_MOT_DOWNED, 0, 0);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmGravios_ChangeState(em, EM_GROUP_REACT, GRAV_REACT_STATUS1_DOWNED_END, EM_SYNC_OWNER);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmGravios_ChangeState(em, EM_GROUP_REACT, GRAV_REACT_STATUS1_DOWNED_END, EM_SYNC_OWNER);
        }
        break;
    }
    EmGravios_DownedPuffsAndSe(em);
}

/*
 * GRAV_IDLE_BURROW_TO_NEXT_AREA: shadow hidden, blind, noReact, senseDelay,
 * guardTimer. Plays GRAV_MOT_BURROW, then sets EM_FLAG_NO_ROOT_MOTION and
 * waits 60 frames with noHitTimer. Then it moves to em->nextArea at a point
 * of the area table of the kind (chosen by variant or stateRand plus a
 * per-area offset for the volcano areas; the start position of the area when
 * the table has none), sets the goal there and goes to GRAV_IDLE_SURFACE
 * (mode 1).
 */
void EmGravios_BurrowToNextArea(Em *em, EmWork *work)
{
    f32 *pts;
    u16 pointOfs;
    u16 pointBase;

    em->hideShadowHold = 2;
    em->hideShadow = 2;
    em->blind = 2;
    em->noReact = 1;
    em->senseDelay = 5;
    em->guardTimer = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GRAV_MOT_BURROW, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 60;
            Player_SetFlag((Player *) em, EM_FLAG_NO_ROOT_MOTION);
        }
        break;
    case 2:
        em->noHitTimer = 2;
        if (--em->timer <= 0) {
            em->prevArea = em->area;
            em->area = em->nextArea;
            pts = ((EmAreaPoints *)Em_FindAreaEntry(em, (struct EmIdEntry *)gEmKindTables[em->type][1], em->area))->pts;
            if (pts == NULL) {
                Vec_Copy(&em->posX, &gStartPos[em->area * 3]);
                em->rotY = 0;
            } else {
                switch (em->variant) {
                default:
                    pointOfs = em->stateRand & 1;
                    break;
                case 1:
                    pointOfs = 0;
                    break;
                case 2:
                    pointOfs = 1;
                    break;
                }
                switch (Area_DayId(em->area)) {
                case AREA_VOLCANO_1:
                    pointBase = 3;
                    break;
                case AREA_VOLCANO_4:
                    pointBase = 0;
                    break;
                case AREA_VOLCANO_6:
                    pointBase = 5;
                    break;
                case AREA_VOLCANO_7:
                    pointBase = 1;
                    break;
                default:
                    pointBase = 0;
                    break;
                }
                pointBase += pointOfs;
                Vec_Copy(&em->posX, &pts[pointBase * 3]);
            }
            Vec_Copy(&em->prevPosX, &em->posX);
            Vec_Copy(em->goal, &em->posX);
            EmGravios_ChangeState(em, EM_GROUP_IDLE, GRAV_IDLE_SURFACE, EM_SYNC_SEND);
        }
        break;
    }
}

/*
 * GRAV_IDLE_SURFACE_AT_GOAL and GRAV_IDLE_SURFACE (setter: position copied
 * from the goal; Em_TypeSetup): sets the same hidden/unhittable flags as
 * EmGravios_BurrowToNextArea, plays GRAV_MOT_SURFACE, then goes to
 * GRAV_IDLE_REHIDE_AS_ROCK.
 */
void EmGravios_Surface(Em *em, EmWork *work)
{
    em->hideShadowHold = 2;
    em->hideShadow = 2;
    em->blind = 2;
    em->noReact = 1;
    em->senseDelay = 5;
    em->guardTimer = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GRAV_MOT_SURFACE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGravios_ChangeState(em, EM_GROUP_IDLE, GRAV_IDLE_REHIDE_AS_ROCK, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmGravios_ChangeState(em, EM_GROUP_IDLE, GRAV_IDLE_REHIDE_AS_ROCK, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * GRAV_IDLE_RAGE_START and GRAV_IDLE_PENDING_RAGE_START (setter:
 * Em_StartRage, Em_StartPendingRage): plays GRAV_MOT_TURN_ROAR and calls
 * EmGravios_EndState when layer 1 stops.
 */
void EmGravios_Enrage(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GRAV_MOT_TURN_ROAR, 0, 0);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * GRAV_MOVE_WALK (variant 0), GRAV_MOVE_WALK_NO_HIT (1: noReact, senseDelay,
 * noHitTimer and noPushTimer every frame) and GRAV_MOVE_WALK_FAST (2:
 * speedBonus 150). Plays GRAV_MOT_WALK and steps with Em_StepToGoal(128) to
 * the goal, or until em->timer runs out with no goal; then
 * EmGravios_EndState.
 */
void EmGravios_Walk(Em *em, s32 unused, s32 variant)
{
    s32 mode;

    mode = variant & 0xFF;
    if (mode == 1) {
        em->noReact = 1;
        em->senseDelay = 5;
        em->noHitTimer = 5;
        em->noPushTimer = 5;
    }
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        if (mode == 2) {
            em->speedBonus = 150;
        }
        Em_SetMotion(em, GRAV_MOT_WALK, 0, 0);
        break;
    case 1:
        if (em->hasGoal != 0) {
            if (Em_StepToGoal(em, 128, 1) == 1) {
                em->step++;
                EmGravios_EndState(em, 0, 0);
                break;
            }
        } else {
            if (--em->timer <= 0) {
                em->step++;
                EmGravios_EndState(em, 0, 0);
            }
        }
        break;
    }
}

/*
 * GRAV_MOVE_END: calls EmGravios_EndState at once.
 */
void EmGravios_EndMove(Em *em)
{
    EmGravios_EndState(em, 0, 0);
}

/*
 * GRAV_MOVE_CHARGE_STOP: plays GRAV_MOT_CHARGE_STOP once, then
 * EmGravios_EndState.
 */
void EmGravios_ChargeStop(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GRAV_MOT_CHARGE_STOP, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * GRAV_MOVE_TURN (variant 0) and GRAV_MOVE_TURN_NO_HIT (1: the unhittable
 * flags every frame): turns with Em_StartTurnToTarget/Em_TurnStep using
 * EmGravios_TurnParams[0]. At the end it clears EM_FLAG_NO_ROOT_MOTION and
 * calls EmGravios_EndState.
 */
void EmGravios_Turn(Em *em, s32 unused, s32 variant)
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
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_StartTurnToTarget(em, &EmGravios_TurnParams[0]);
        break;
    case 1:
        if (Em_TurnStep(em, &EmGravios_TurnParams[0]) == 1) {
            em->step++;
            Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * GRAV_MOVE_CHARGE and GRAV_MOVE_CHARGE_SLOW (setter: motionSpeed 0.7):
 * plays GRAV_MOT_CHARGE. From frame 150 (where EmGravios_MotionEffects creates the
 * charge GraviosAttack) it steps with Em_StepToGoal(64) to
 * the goal, or until em->timer runs out. Then it plays GRAV_MOT_CHARGE_STOP
 * and calls EmGravios_EndState.
 */
void EmGravios_Charge(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GRAV_MOT_CHARGE, 0, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 150.0f, 0U) != 0) {
            em->step++;
        }
        break;
    case 2:
        if (em->hasGoal != 0) {
            if (Em_StepToGoal(em, 64, 1) == 1) {
                em->step++;
                Em_SetMotion(em, GRAV_MOT_CHARGE_STOP, 0, 0);
                break;
            }
        } else {
            if (--em->timer <= 0) {
                em->step++;
                Em_SetMotion(em, GRAV_MOT_CHARGE_STOP, 0, 0);
                break;
            }
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * GRAV_MOVE_WALK_B: plays GRAV_MOT_WALK with
 * Em_StepToGoal(64) (GRAV_MOVE_WALK uses 128) to the goal or
 * until em->timer runs out; then EmGravios_EndState.
 */
void EmGravios_WalkB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GRAV_MOT_WALK, 0, 0);
        break;
    case 1:
        if (em->hasGoal != 0) {
            if (Em_StepToGoal(em, 64, 1) == 1) {
                em->step++;
                EmGravios_EndState(em, 0, 0);
                break;
            }
        } else {
            if (--em->timer <= 0) {
                em->step++;
                EmGravios_EndState(em, 0, 0);
            }
        }
        break;
    }
}

/*
 * GRAV_MOVE_LIMP (the setter picks it instead of WALK, WALK_B or WALK_FAST
 * below 30% hp): plays GRAV_MOT_LIMP with Em_StepToGoal(64)
 * to the goal or until em->timer runs out; then EmGravios_EndState.
 */
void EmGravios_Limp(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GRAV_MOT_LIMP, 0, 0);
        break;
    case 1:
        if (em->hasGoal != 0) {
            if (Em_StepToGoal(em, 64, 1) == 1) {
                em->step++;
                EmGravios_EndState(em, 0, 0);
                break;
            }
        } else {
            if (--em->timer <= 0) {
                em->step++;
                EmGravios_EndState(em, 0, 0);
            }
        }
        break;
    }
}

/*
 * GRAV_MOVE_TURN_TO_GOAL and GRAV_MOVE_TURN_TO_GOAL_RAGE_CHECK (setter:
 * Em_CheckRageByHp): plays GRAV_MOT_TURN_ROAR and steps rotY 0x200 a frame
 * toward the goal (snapping within 0x200) up to frame 60. Then it waits for
 * layer 1 to stop and calls EmGravios_EndState.
 */
void EmGravios_TurnToGoal(Em *em, EmWork *work)
{
    u16 goalYaw;
    u32 yawDiff;
    s32 rotY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GRAV_MOT_TURN_ROAR, 0, 0);
        work->goalYaw = Math_AngleXZ(&em->posX, em->goal);
        break;
    case 1:
        rotY = em->rotY;
        goalYaw = work->goalYaw;
        yawDiff = (goalYaw - (rotY & 0xFFFF)) & 0xFFFF;
        if (((yawDiff + 0x200) & 0xFFFF) < 0x400U) {
            em->rotY = goalYaw;
        } else if (yawDiff < 0x8000U) {
            em->rotY = (rotY + 0x200) & 0xFFFF;
        } else {
            em->rotY = (rotY - 0x200) & 0xFFFF;
        }
        if (EmMotion_ReachedFrame(em, 0, 60.0f) != 0) {
            em->step++;
        }
        break;
    case 2:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * GRAV_MOVE_BURROW: plays GRAV_MOT_BURROW and calls EmGravios_EndState when
 * layer 1 stops.
 */
void EmGravios_Burrow(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GRAV_MOT_BURROW, 0, 0);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * GRAV_MOVE_SURFACE: plays GRAV_MOT_SURFACE and never leaves the state
 * itself: only another state change ends it.
 */
void EmGravios_SurfaceHold(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GRAV_MOT_SURFACE, 0, 0);

    case 1:
        break;
    }
}

/*
 * GRAV_FLY_TAKE_OFF: plays GRAV_MOT_TAKE_OFF with the GRAV_CURVE_TAKE_OFF
 * climb, in EM_STANCE_AIR from frame 96; when the curve ends,
 * EmGravios_EndStateInAir (GRAV_FLY_HOVER_ONCE).
 */
void EmGravios_FlyTakeOff(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GRAV_MOT_TAKE_OFF, 0, 0);
        EmGravios_StartCurves(em, GRAV_CURVE_TAKE_OFF);
        work->flyGuard = 0;
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 96.0f, 0U) != 0) {
            em->step++;
            em->stance = EM_STANCE_AIR;
            EmGravios_UpdateCurves(em);
        }
        break;
    case 2:
        if (EmGravios_UpdateCurves(em)) {
            em->step++;
            EmGravios_EndStateInAir(em, 0);
        }
        break;
    }
}

/*
 * GRAV_FLY_LAND: hovers down at velY -10 straight ahead; below floorY + 630,
 * on a wingbeat frame (10, 86 or 160), it plays GRAV_MOT_DESCEND with velY
 * set to reach the floor in 30 frames, then GRAV_MOT_TOUCH_DOWN
 * (EM_STANCE_GROUND, camera shake 1) and EmGravios_EndState. Entered on
 * arrival in an area in the air (EmGravios_Reappear) and from
 * EmGravios_FlyHover.
 */
void EmGravios_FlyLand(Em *em, EmWork *work)
{
    f32 velY;
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        em->rotX = 0;
        em->rotZ = 0;
        Em_SetMotion(em, GRAV_MOT_HOVER, 0, 0);
        Player_ClearVelocity(em);
        em->velY = -10.0f;
        work->flyGuard = 0;
        break;
    case 1:
        work->moveAngX = 0;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
        if (!((630.0f + em->floorY) <= em->posY) && ((EmMotion_PassedFrameScaled(em, 10.0f, 0U) != 0) || (EmMotion_PassedFrameScaled(em, 86.0f, 0U) != 0) || (EmMotion_PassedFrameScaled(em, 160.0f, 0U) != 0))) {
            em->step++;
            Em_SetMotion(em, GRAV_MOT_DESCEND, 0, 0);
            velY = (em->floorY - em->posY) / 30.0f;
            em->velY = velY;
            if (!(velY < 0.0f)) {
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
            Em_SetMotion(em, GRAV_MOT_TOUCH_DOWN, 0, 0);
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
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * GRAV_FLY_HOVER: hovers (the GRAV_CURVE_HOVER bob) turning toward the goal
 * at 0x100 a frame; when em->timer runs out it lands (GRAV_FLY_LAND,
 * EM_SYNC_SEND), and when the timer reads 300 it gives the script a turn
 * (EmGravios_EndStateInAir). Its setter gives it no timer: it keeps the
 * previous one.
 */
void EmGravios_FlyHover(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        work->turnSpeed = 256;
        Em_SetMotion(em, GRAV_MOT_HOVER, 0, 0);
        work->flyGuard = 0;
        break;
    case 1:
        EmGravios_MoveByVelYCurve(em, GRAV_CURVE_HOVER);
        if (--em->timer <= 0) {
            em->step++;
            EmGravios_ChangeState(em, EM_GROUP_FLY, GRAV_FLY_LAND, EM_SYNC_SEND);
        }
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 1);
        if (em->timer == 300) {
            EmGravios_EndStateInAir(em, 0);
        }
        break;
    case 2:
        EmGravios_MoveByVelYCurve(em, GRAV_CURVE_HOVER);
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 1);
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * GRAV_FLY_HOP_TO_GOAL: GRAV_MOT_TAKE_OFF with the GRAV_CURVE_TAKE_OFF climb
 * from frame 96, turning to the goal between frames 50 and 114; once the
 * curve ends at or below floorY + 800 it plays GRAV_MOT_DESCEND (timed to
 * reach the floor in 30 frames), touches down (camera shake 2), sets
 * fxMotion 150 (frames since landing) and calls EmGravios_EndState. It reads
 * turnSpeed as EmFlyerWork's signed halfword, where the other states read
 * EmWork's.
 */
void EmGravios_FlyHopToGoal(Em *em, EmWork *work)
{
    f32 velY;
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GRAV_MOT_TAKE_OFF, 0, 0);
        Player_ClearVelocity(em);

    case 1:
        if (EmMotion_PassedFrameScaled(em, 96.0f, 0U) != 0) {
            em->step++;
            Player_ClearVelocity(em);
            EmGravios_StartCurves(em, GRAV_CURVE_TAKE_OFF);
            work->flyGuard = 0;
        }
        break;
    case 2:
        if ((EmMotion_ReachedFrame(em, 0, 50.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 114.0f) == 0)) {
            Em_TurnToGoal(em, (s32) ((EmFlyerWork *)work)->turnSpeed, 1);
        }
        if (EmMotion_ReachedFrame(em, 0, 30.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            EmGravios_UpdateCurves(em);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
        }
        break;
    case 3:
        if ((EmGravios_UpdateCurves(em) != 0) && (em->posY <= (800.0f + em->floorY))) {
            em->step++;
            Em_SetMotion(em, GRAV_MOT_DESCEND, 0, 0);
            Player_ClearVelocity(em);
            velY = (em->floorY - em->posY) / 30.0f;
            em->velY = velY;
            if (!(velY < 0.0f)) {
                em->velY = -10.0f;
            }
        }
        break;
    case 4:
        work->moveAngX = 0;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
        if (em->posY <= em->floorY) {
            em->step++;
            Em_SetMotion(em, GRAV_MOT_TOUCH_DOWN, 0, 0);
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            CamShake_ReqAIf(em, 2);
        }
        break;
    case 5:
        if (em->motion[0].playing == 0) {
            work->fxMotion = 150;
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * GRAV_FLY_HOP_FORWARD: GRAV_MOT_TAKE_OFF with the GRAV_CURVE_HOP curve and
 * velZ 20, in the air from frame 30; when the curve ends on the floor it
 * plays GRAV_MOT_TOUCH_DOWN (camera shake 2), sets fxMotion 150 and calls
 * EmGravios_EndState.
 */
void EmGravios_FlyHopForward(Em *em, EmWork *work)
{
    f32 floorY;
    s32 ended;

    ended = 0;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GRAV_MOT_TAKE_OFF, 0, 0);
        EmGravios_StartCurves(em, GRAV_CURVE_HOP);
        em->velZ = 20.0f;
        work->flyGuard = 0;
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 30.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            ended = EmGravios_UpdateCurves(em) & 0xFF;
        }
        if ((ended != 0) && (em->posY <= em->floorY)) {
            Em_SetMotion(em, GRAV_MOT_TOUCH_DOWN, 0, 0);
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
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * GRAV_FLY_HOP_BACK: as EmGravios_FlyHopForward with velZ -20, a hop
 * backward.
 */
void EmGravios_FlyHopBack(Em *em, EmWork *work)
{
    f32 floorY;
    s32 ended;

    ended = 0;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GRAV_MOT_TAKE_OFF, 0, 0);
        EmGravios_StartCurves(em, GRAV_CURVE_HOP);
        em->velZ = -20.0f;
        work->flyGuard = 0;
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 30.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            ended = EmGravios_UpdateCurves(em) & 0xFF;
        }
        if ((ended != 0) && (em->posY <= em->floorY)) {
            Em_SetMotion(em, GRAV_MOT_TOUCH_DOWN, 0, 0);
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
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * GRAV_FLY_HOVER_TO_GOAL: hovers turning (0x100 a frame) and pitching toward
 * the goal at velZ 20 (the setter's timer: three times the travel frames),
 * with the GRAV_CURVE_HOVER bob; ends (EmGravios_EndStateInAir) when the
 * timer runs out or the goal is within 10 * velZ in XZ.
 */
void EmGravios_FlyHoverToGoal(Em *em, EmWork *work)
{
    f32 dist;
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        work->turnSpeed = 256;
        Em_SetMotion(em, GRAV_MOT_HOVER, 0, 0);
        work->flyGuard = 0;
        break;
    case 1:
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 1);
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_GetPitchToGoal(em, &work->moveAngX, 0.0f, 0.0f);
        Em_MoveRotated(em, &work->moveAngX);
        EmGravios_MoveByVelYCurve(em, GRAV_CURVE_HOVER);
        dist = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer <= 0 || dist <= (10.0f * em->velZ)) {
            em->step++;
            Player_ClearVelocity(em);
            EmGravios_EndStateInAir(em, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * GRAV_FLY_HOVER_ONCE: plays the hover once, then EmGravios_EndStateInAir,
 * which enters this state again with scriptPending set: the air state the
 * script waits in.
 */
void EmGravios_FlyHoverOnce(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, GRAV_MOT_HOVER, 0, 0);
        work->flyGuard = 0;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGravios_EndStateInAir(em, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * GRAV_FLY_HOVER_TURN: hovers (the GRAV_CURVE_HOVER bob) turning toward the
 * goal at 0x100 a frame for 150 frames, then EmGravios_EndStateInAir. It
 * stores goalYaw (the angle to the goal less rotY), which nothing here
 * reads.
 */
void EmGravios_FlyHoverTurn(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, GRAV_MOT_HOVER, 0, 0);
        work->turnSpeed = 256;
        work->goalYaw = Math_AngleXZ(&em->posX, em->goal);
        work->goalYaw = work->goalYaw - em->rotY;
        work->flyGuard = 0;
        em->timer = 150;
        break;
    case 1:
        EmGravios_MoveByVelYCurve(em, GRAV_CURVE_HOVER);
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 1);
        if (--em->timer <= 0) {
            em->step++;
            EmGravios_EndStateInAir(em, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * GRAV_FLY_RISE: the hover motion at velY 20 straight up until the timer
 * (the setter: the travel frames to the goal) runs out or posY passes the
 * goal's height + 1000; then EmGravios_EndStateInAir. Its heading is taken
 * toward aimPos, a local that is never written, so moveAngY is whatever the
 * stack held; the climb is nearly vertical.
 */
void EmGravios_FlyRise(Em *em, EmWork *work)
{
    f32 aimPos[3];
    f32 floorY;
    s32 timerLeft;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, GRAV_MOT_HOVER, 0, 0);
        work->flyGuard = 0;
        work->moveAngX = 0;
        work->moveAngZ = 0;
        Player_ClearVelocity(em);
        em->velY = 20.0f;
        break;
    case 1:
        work->moveAngY = Math_AngleXZ(&em->posX, aimPos) & 0xFFFF;
        Em_MoveRotated(em, &work->moveAngX);
        timerLeft = em->timer - 1;
        em->timer = timerLeft;
        if ((timerLeft <= 0) || !(em->posY <= (1000.0f + em->goal[1]))) {
            em->step++;
            Player_ClearVelocity(em);
            EmGravios_EndStateInAir(em, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * GRAV_FLY_STRAIGHT_TO_GOAL: as EmGravios_FlyHoverToGoal, but the yaw is set
 * straight at the goal (no turning) and the distance is measured in 3D
 * (Vec_Distance); no velY curve.
 */
void EmGravios_FlyStraightToGoal(Em *em, EmWork *work)
{
    f32 dist;
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        work->turnSpeed = 256;
        Em_SetMotion(em, GRAV_MOT_HOVER, 0, 0);
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
            EmGravios_EndStateInAir(em, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * GRAV_FLY_TRAP_ESCAPE: leaves a trap by flying: GRAV_MOT_TAKE_OFF from
 * frame 96 (blended over 10 frames) with the GRAV_CURVE_TAKE_OFF climb, in
 * the air at once; it clears holdTimer and trapHit and sets noReact; then
 * GRAV_MOT_DESCEND below floorY + 800, the touch-down (camera shake 2),
 * fxMotion 150 and EmGravios_EndState. Entered from EmGravios_TrapStruggle.
 */
void EmGravios_FlyTrapEscape(Em *em, EmWork *work)
{
    f32 floorY;
    f32 velY;
    f32 clampY;

    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, GRAV_MOT_TAKE_OFF, 10, 96);
        EmGravios_StartCurves(em, GRAV_CURVE_TAKE_OFF);
        em->stance = EM_STANCE_AIR;
        em->holdTimer = 0;
        em->trapHit = 0;
        em->noReact = 1;
        break;
    case 1:
        EmGravios_UpdateCurves(em);
        if (em->motion[0].playing == 0) {
            em->step++;
        }
        break;
    case 2:
        if ((EmGravios_UpdateCurves(em) & 0xFF) && (em->posY <= (800.0f + em->floorY))) {
            em->step++;
            Em_SetMotion(em, GRAV_MOT_DESCEND, 0, 0);
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
            Em_SetMotion(em, GRAV_MOT_TOUCH_DOWN, 0, 0);
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            CamShake_ReqAIf(em, 2);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            work->fxMotion = 150;
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
    clampY = em->floorY;
    if (em->posY < clampY) {
        em->posY = clampY;
    }
}

/*
 * GRAV_ATK_TURN_AND_ATTACK_A: turns to the target (Em_TurnToTargetMotion, 20
 * degrees), plays GRAV_MOT_ATTACK_A (EmGravios_MotionEffects creates
 * GraviosAttack 21 at frame 46, then 1 for the Gravios kinds or 25 for
 * the Basarios at frame 50), then EmGravios_EndState.
 */
void EmGravios_TurnAndAttackA(Em *em)
{
    switch (em->step) {
    case 0:
        if (Em_TurnToTargetMotion(em, 3640, 2, 6, 5) != 0) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->lookAtTarget = 0;
            Em_SetMotion(em, GRAV_MOT_ATTACK_A, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * GRAV_ATK_TURN_AND_BEAM: turns to the target, plays GRAV_MOT_BEAM_SHORT and
 * at frame 122 creates the beam (EMRANGED_GRAVIOS_BEAM at GRAV_BONE_BREATH, variant
 * 0, held 20 frames), then EmGravios_EndState.
 */
void EmGravios_TurnAndBeam(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        break;
    case 1:
        if (Em_TurnToTargetMotion(em, 3640, 2, 6, 5) != 0) {
            em->step++;
            em->lookAtTarget = 0;
            Em_SetMotion(em, GRAV_MOT_BEAM_SHORT, 0, 0);
        }
        break;
    case 2:
        if (EmMotion_PassedFrameScaled(em, 122.0f, 0U) != 0) {
            EmRangedAttack_CreateAtBoneTimed(em, GRAV_BONE_BREATH, EMRANGED_GRAVIOS_BEAM, 0, 0, 0, 20);
        }
        if (em->motion[0].playing == 0) {
            em->lookAtTarget = 0;
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * GRAV_ATK_LEAP_SLAM: crouches (GRAV_MOT_LEAP); at frame 70 it jumps
 * straight ahead (velY 73, accelY -3.38, EM_STANCE_AIR), caps the fall at
 * velY -100, and within 100 * scaleY of the floor plays GRAV_MOT_LEAP_LAND
 * (GraviosAttack 30 at its frame 2) on the floor; then EmGravios_EndState.
 */
void EmGravios_LeapSlam(Em *em, EmWork *work)
{
    f32 velY;
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GRAV_MOT_LEAP, 0, 0);
        Player_ClearVelocity(em);

    case 1:
        if (EmMotion_PassedFrameScaled(em, 70.0f, 0U) != 0) {
            em->step++;
            em->velY = 73.0f;
            em->accelY = -3.38f;
            work->flyGuard = 0;
            em->stance = EM_STANCE_AIR;
            work->moveAngX = 0;
            work->moveAngY = em->rotY;
            work->moveAngZ = 0;
        }
        break;
    case 2:
        Em_MoveRotatedAccel(em, &work->moveAngX);
        velY = em->velY;
        if (velY <= 0.0f) {
            if (velY < -100.0f) {
                em->velY = -100.0f;
            }
            if (em->posY <= (em->floorY + (100.0f * em->scaleY))) {
                Em_SetMotion(em, GRAV_MOT_LEAP_LAND, 0, 0);
                em->stance = EM_STANCE_GROUND;
                em->posY = em->floorY;
                em->step++;
                Player_ClearVelocity(em);
            }
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * GRAV_ATK_ATTACK_A_LOOKING: GRAV_MOT_ATTACK_A without the turn, with
 * lookAtTarget 1 (the body bends toward the target), then
 * EmGravios_EndState.
 */
void EmGravios_AttackALooking(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 1;
        Em_SetMotion(em, GRAV_MOT_ATTACK_A, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->lookAtTarget = 0;
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * GRAV_ATK_BEAM_OR_SHOT_A and _B (row 0 and 1): turns to the target and
 * plays GRAV_MOT_BEAM. The Gravios kinds breathe the beam (variant 1, held
 * 35 frames) at frame 136, then play GRAV_MOT_BEAM_END; the Basarios skips
 * both and fires one EMRANGED_GRAVIOS_SHOT from GRAV_BONE_BREATH at the frame and
 * pitch of its row of EmGravios_BasariosShots. Ends with EmGravios_EndState.
 */
void EmGravios_TurnAndBeamOrShot(Em *em, EmWork *work, s32 row)
{
    f32 shotFrame;
    s32 rowOfs;
    u16 rowFrame;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        break;
    case 1:
        if (Em_TurnToTargetMotion(em, 3640, 2, 6, 5) != 0) {
            if (em->type == EM_KIND_BASARIOS) {
                em->step = 3;
            } else {
                em->step++;
            }
            em->lookAtTarget = 0;
            Em_SetMotion(em, GRAV_MOT_BEAM, 0, 0);
        }
        break;
    case 2:
        if (EmMotion_PassedFrameScaled(em, 136.0f, 0U) != 0) {
            EmRangedAttack_CreateAtBoneTimed(em, GRAV_BONE_BREATH, EMRANGED_GRAVIOS_BEAM, 1, 0, 0, 35);
        }
        if (em->motion[0].playing == 0) {
            em->lookAtTarget = 0;
            em->step++;
            Em_SetMotion(em, GRAV_MOT_BEAM_END, 0, 0);
        }
        break;
    case 3:
        if (em->type == EM_KIND_BASARIOS) {
            /* The row's two fields are read as bytes at the first row's
             * fields plus the row's offset: the compiler then computes the
             * offset once for both reads, as the game's code does. */
            rowOfs = row * 4;
            rowFrame = *(u16 *) (&EmGravios_BasariosShotFrame + rowOfs);
            shotFrame = (f32) (u32) rowFrame;
            if (EmMotion_PassedFrameScaled(em, shotFrame, 0U) != 0) {
                EmRangedAttack_CreateAtBone(em, GRAV_BONE_BREATH, EMRANGED_GRAVIOS_SHOT, 0, (s32) *(u16 *) (&EmGravios_BasariosShotPitch + rowOfs), 0);
            }
        }
        if (em->motion[0].playing == 0) {
            em->lookAtTarget = 0;
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * GRAV_ATK_GAS_A, _B and _C (variant 0 to 2): GRAV_MOT_GAS held 120 frames,
 * with a GraviosAttack at frame 196 (14, 12 or 31 by variant) and, from
 * frame 124, a cloud every 24 frames (DustCloud kind 29 of the variant;
 * variant 2 at 1.1 times the scale); then GRAV_MOT_GAS_END with the clouds
 * up to its frame 50, then EmGravios_EndState.
 */
void EmGravios_Gas(Em *em, EmWork *work, s16 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        em->timer = 120;
        Em_SetMotion(em, GRAV_MOT_GAS, 0, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 196.0f, 0)) {
            switch (variant) {
            default:
                GraviosAttack_Create(em, 14);
                break;
            case 1:
                GraviosAttack_Create(em, 12);
                break;
            case 2:
                GraviosAttack_Create(em, 31);
                break;
            }
        }
        if (EmMotion_ReachedFrame(em, 0, 124.0f) && gScene.frame % 24 == 0) {
            switch (variant) {
            default:
                DustCloud_Create(em, 29, 0, em->scaleX);
                break;
            case 1:
                DustCloud_Create(em, 29, 1, em->scaleX);
                break;
            case 2:
                DustCloud_Create(em, 29, 2, 1.1f * em->scaleX);
                break;
            }
        }
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, GRAV_MOT_GAS_END, 0, 0);
        }
        break;
    case 2:
        if (!EmMotion_ReachedFrame(em, 0, 50.0f) && gScene.frame % 24 == 0) {
            switch (variant) {
            case 0:
                DustCloud_Create(em, 29, 0, em->scaleX);
                break;
            case 1:
                DustCloud_Create(em, 29, 1, em->scaleX);
                break;
            case 2:
                DustCloud_Create(em, 29, 2, 1.1f * em->scaleX);
                break;
            }
        }
        if (em->motion[0].playing == 0) {
            em->lookAtTarget = 0;
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * GRAV_ATK_SPIN: spins 0x200 a frame (the Basarios: GRAV_MOT_SPIN_BASARIOS,
 * rotY rising; the Gravios kinds: GRAV_MOT_SPIN, rotY falling), playing the
 * motion twice, then EmGravios_EndState. Each pass waits for motion layer 1
 * to stop, though the spin plays on layer 0, so a pass ends as soon as layer
 * 1 is idle.
 */
void EmGravios_Spin(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        if (em->type == EM_KIND_BASARIOS) {
            Em_SetMotion(em, GRAV_MOT_SPIN_BASARIOS, 0, 0);
        } else {
            Em_SetMotion(em, GRAV_MOT_SPIN, 0, 0);
        }
        Motion_InitTime(em, 0);
        break;
    case 1:
        if (em->type == EM_KIND_BASARIOS) {
            em->rotY += 0x200;
        } else {
            em->rotY -= 0x200;
        }
        if (em->motion[1].playing == 0) {
            em->step++;
            if (em->type == EM_KIND_BASARIOS) {
                Em_SetMotion(em, GRAV_MOT_SPIN_BASARIOS, 0, 0);
                break;
            }
            Em_SetMotion(em, GRAV_MOT_SPIN, 0, 0);
        }
        break;
    case 2:
        if (em->type == EM_KIND_BASARIOS) {
            em->rotY += 0x200;
        } else {
            em->rotY -= 0x200;
        }
        if (em->motion[1].playing == 0) {
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * GRAV_ATK_TURN_AND_ATTACK_B: turns to the target, plays GRAV_MOT_ATTACK_B
 * (GraviosAttack 2 for the Gravios kinds or 26 for the Basarios at frame
 * 68), then EmGravios_EndState.
 */
void EmGravios_TurnAndAttackB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;

    case 1:
        if (Em_TurnToTargetMotion(em, 3640, 2, 6, 5) != 0) {
            em->step++;
            em->lookAtTarget = 0;
            Em_SetMotion(em, GRAV_MOT_ATTACK_B, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->lookAtTarget = 0;
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * GRAV_ATK_REAR_ATTACK: plays GRAV_MOT_REAR_ATTACK (GraviosAttack 4, which
 * faces backward, at frame 46) without turning, then EmGravios_EndState.
 */
void EmGravios_RearAttack(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GRAV_MOT_REAR_ATTACK, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->lookAtTarget = 0;
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * GRAV_ATK_BEAM_LONG: without turning, plays GRAV_MOT_BEAM_LONG and at frame
 * 118 creates the beam (variant 0, held 40 frames), then EmGravios_EndState.
 */
void EmGravios_BeamLong(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GRAV_MOT_BEAM_LONG, 0, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 118.0f, 0U) != 0) {
            EmRangedAttack_CreateAtBoneTimed(em, GRAV_BONE_BREATH, EMRANGED_GRAVIOS_BEAM, 0, 0, 0, 40);
        }
        if (em->motion[0].playing == 0) {
            em->lookAtTarget = 0;
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * GRAV_REACT_FLINCH_PART6_BROKEN: GRAV_MOT_FLINCH_B once, then
 * EmGravios_EndState. EmGravios_StartPart6Flinch picks it when part 6
 * flinches while broken (parts[6].state above 1).
 */
void EmGravios_FlinchPart6Broken(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, GRAV_MOT_FLINCH_B, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * GRAV_REACT_FLINCH_B: GRAV_MOT_FLINCH_C once (an unbroken part 3, 4 or
 * other flinched), then EmGravios_EndState.
 */
void EmGravios_FlinchB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, GRAV_MOT_FLINCH_C, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * GRAV_REACT_FLINCH_C: GRAV_MOT_FLINCH_D once (part 5), then
 * EmGravios_EndState.
 */
void EmGravios_FlinchC(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, GRAV_MOT_FLINCH_D, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * GRAV_REACT_FLINCH_D: GRAV_MOT_FLINCH_E once (parts 1 and 2), then
 * EmGravios_EndState.
 */
void EmGravios_FlinchD(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, GRAV_MOT_FLINCH_E, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * GRAV_REACT_PART_BREAK, on EM_DMG_PART8_DOWN (EmGravios_ChangeState sets
 * fxFlags bit 0, counts partBreakCount and ends every status):
 * GRAV_MOT_PART_BREAK; it hides mesh 1 at frame 6, turns rotY by 936 *
 * motionSpeed a frame from frame 52 to 122 (about one full turn), then takes
 * 8 off rotY and goes to GRAV_REACT_PART_BREAK_END (EM_SYNC_OWNER; a remote
 * copy waits in step 2 for the owner). noHitTimer 5 every frame.
 */
void EmGravios_PartBreak(Em *em)
{
    f32 turnPerFrame;
    u32 turnStep;
    u8 type;

    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, GRAV_MOT_PART_BREAK, 0, 0);
        break;
    case 1:
        if ((EmMotion_PassedFrameScaled(em, 6.0f, 0U) != 0) && ((s8) em->meshShown[1] != 0)) {
            type = em->type;
            /* A test for the Gypceros kinds, which don't run this code: it
             * always holds here. */
            if ((type != EM_KIND_GYPCEROS) && (type != EM_KIND_PURPLE_GYPCEROS)) {
                em->meshShown[1] = 0;
            }
        }
        if ((EmMotion_ReachedFrame(em, 0, 52.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 122.0f) == 0)) {
            turnPerFrame = 936.0f * em->motionSpeed;
            turnStep = (u32) turnPerFrame;
            em->rotY -= (s32) turnStep & 0xFFFF;
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            em->rotY -= 8;
            EmGravios_ChangeState(em, EM_GROUP_REACT, GRAV_REACT_PART_BREAK_END, EM_SYNC_OWNER);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmGravios_ChangeState(em, EM_GROUP_REACT, GRAV_REACT_PART_BREAK_END, EM_SYNC_OWNER);
        }
        break;
    }
}

/*
 * GRAV_REACT_KNOCK_DOWN: GRAV_MOT_KNOCK_DOWN once (a broken part other than
 * 3 flinched), then EmGravios_EndState, and then it copies the new state
 * (GRAV_IDLE) into prevStateGroup and prevState.
 */
void EmGravios_KnockDown(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, GRAV_MOT_KNOCK_DOWN, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGravios_EndState(em, 0, 0);
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
        }
        break;
    }
}

/*
 * GRAV_REACT_KNOCK_DOWN_PART3: as EmGravios_KnockDown with
 * GRAV_MOT_KNOCK_DOWN_PART3 (broken part 3).
 */
void EmGravios_KnockDownPart3(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, GRAV_MOT_KNOCK_DOWN_PART3, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGravios_EndState(em, 0, 0);
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
        }
        break;
    }
}

/*
 * GRAV_REACT_AIR_FLINCH: in the air, plays GRAV_MOT_FALL (velY at most -50)
 * moving by moveAngX; at frame 14 it stops the fall (velY 0) and recovers
 * into the hover (EmGravios_EndStateInAir). Nothing in the file enters it.
 */
void EmGravios_AirFlinch(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GRAV_MOT_FALL, 0, 0);
        if (em->velY > -50.0f) {
            em->velY = -50.0f;
        }
        Em_RestartScript(em);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 14.0f, 0) != 0) {
            em->step++;
            EmGravios_EndStateInAir(em, 1);
            em->velY = 0.0f;
        }
        Em_MoveRotated(em, &work->moveAngX);
        break;
    }
}

/*
 * GRAV_REACT_FALL, on EM_DMG_STATUS_IN_AIR or a part flinch in the air:
 * falls (GRAV_MOT_FALL, velY at most -50, accelY -10) to the floor, then
 * GRAV_MOT_FALL_LAND (EM_STANCE_GROUND), GRAV_MOT_GET_UP and
 * EmGravios_EndState.
 */
void EmGravios_Fall(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, GRAV_MOT_FALL, 0, 0);
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
            Em_SetMotion(em, GRAV_MOT_FALL_LAND, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, GRAV_MOT_GET_UP_B, 6, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGravios_EndState(em, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    }
}

/*
 * GRAV_REACT_FALL_B: as EmGravios_Fall with blind 2 every frame (it sees
 * nobody). Nothing in the file enters it.
 */
void EmGravios_FallBlind(Em *em, EmWork *work)
{
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, GRAV_MOT_FALL, 0, 0);
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
            Em_SetMotion(em, GRAV_MOT_FALL_LAND, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, GRAV_MOT_GET_UP_B, 6, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGravios_EndState(em, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    }
}

/*
 * GRAV_REACT_FLINCH_E: GRAV_MOT_FLINCH_F once, then EmGravios_EndState.
 * Nothing in the file enters it.
 */
void EmGravios_FlinchE(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, GRAV_MOT_FLINCH_F, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * GRAV_REACT_STATUS4, on EM_DMG_STATUS4 outside a hold
 * (EmGravios_ChangeState: Em_BeginStatus4): hit-react mode
 * EM_HIT_REACT_STATUS and the status-4 effect every frame, GRAV_MOT_STATUS4
 * until em->timer runs out, then GRAV_IDLE_STATUS4_SHAKE_OFF
 * (Em_EndStatus4).
 */
void EmGravios_Status4(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_STATUS);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, GRAV_MOT_STATUS4, 0, 0);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmGravios_ChangeState(em, EM_GROUP_IDLE, GRAV_IDLE_STATUS4_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmGravios_ChangeState(em, EM_GROUP_IDLE, GRAV_IDLE_STATUS4_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
    Em_SpawnStatus4Effect(em, 2);
}

/*
 * GRAV_REACT_TRAPPED, on EM_DMG_ATTACK9 or ATTACK6 outside a hold (the
 * trap): EM_HIT_REACT_NO_FLINCH, GRAV_MOT_TRAPPED, noReact, holdTimer 50 and
 * then kept at 5 or more, blind 2; then GRAV_REACT_TRAPPED_STRUGGLE.
 */
void EmGravios_Trapped(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_NO_FLINCH);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, GRAV_MOT_TRAPPED, 0, 0);
        em->noReact = 1;
        em->holdTimer = 50;
        em->blind = 2;
        break;
    case 1:
        if ((s8) em->holdTimer < 5) {
            em->holdTimer = 5;
        }
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            EmGravios_ChangeState(em, EM_GROUP_REACT, GRAV_REACT_TRAPPED_STRUGGLE, EM_SYNC_OWNER);
        }
        break;
    case 2:
        if ((s8) em->holdTimer < 5) {
            em->holdTimer = 5;
        }
        if (em->remote == 0) {
            EmGravios_ChangeState(em, EM_GROUP_REACT, GRAV_REACT_TRAPPED_STRUGGLE, EM_SYNC_OWNER);
        }
        break;
    }
}

/*
 * Held (holdTimer 5 a frame, noReact), loops GRAV_MOT_STRUGGLE until
 * em->trapTimer runs out, then flies out (GRAV_FLY_TRAP_ESCAPE,
 * EM_SYNC_OWNER_FLAGGED). It runs GRAV_REACT_TRAPPED_STRUGGLE,
 * GRAV_REACT_STATUS4_DOWNED_END and GRAV_REACT_STUN_TRAPPED_STRUGGLE, which
 * differ only in what EmGravios_ChangeState does on entry (Em_EndStatus4 for
 * the second, Em_EndStatus5 for the third).
 */
void EmGravios_TrapStruggle(Em *em)
{
    em->holdTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, GRAV_MOT_STRUGGLE, 0, 0);
        em->noReact = 1;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            Em_SetMotion(em, GRAV_MOT_STRUGGLE, 0, 0);
        }
        if (--em->trapTimer <= 0) {
            em->step++;
            Em_SetMotion(em, GRAV_MOT_STRUGGLE, 0, 0);
            EmGravios_ChangeState(em, EM_GROUP_FLY, GRAV_FLY_TRAP_ESCAPE, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if ((em->motion[0].playing == 0) && (em->remote == 0)) {
            Em_SetMotion(em, GRAV_MOT_STRUGGLE, 0, 0);
            EmGravios_ChangeState(em, EM_GROUP_FLY, GRAV_FLY_TRAP_ESCAPE, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * GRAV_REACT_STATUS4_DOWNED (variant 0) and GRAV_REACT_STUN_TRAPPED (variant
 * 1), a status while held: GRAV_MOT_DOWNED, holdTimer 5 and noReact until
 * em->timer runs out, then Em_SetTrapTimeGoal and the struggle. Variant 0
 * (status 4): EM_HIT_REACT_STATUS and the status-4 effect, then
 * GRAV_REACT_STATUS4_DOWNED_END; variant 1 (the stun): stun stars for the
 * timer plus half the motion, then GRAV_REACT_STUN_TRAPPED_STRUGGLE. The
 * second argument is the work pointer, unused.
 */
void EmGravios_DownedWhileHeld(Em *em, s32 work, s32 stunVariant)
{
    s32 isStun;

    isStun = stunVariant & 0xFF;
    if (isStun == 0) {
        Em_SetHitReactMode(em, EM_HIT_REACT_STATUS);
        Em_SpawnStatus4Effect(em, 2);
    }
    em->holdTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, GRAV_MOT_DOWNED, 0, 0);
        em->noReact = 1;
        Em_RestartScript(em);
        if (isStun == 1) {
            StunStars_CreateEm(em, 1, 34, (s32) ((f32) em->timer + (em->motion[0].frameCount / 2.0f)), 150.0f, 3.0f);
        }
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetTrapTimeGoal(em);
            if (isStun == 0) {
                EmGravios_ChangeState(em, EM_GROUP_REACT, GRAV_REACT_STATUS4_DOWNED_END, EM_SYNC_OWNER);
                break;
            }
            EmGravios_ChangeState(em, EM_GROUP_REACT, GRAV_REACT_STUN_TRAPPED_STRUGGLE, EM_SYNC_OWNER);
        }
        break;
    case 2:
        if (em->remote == 0) {
            if (isStun == 0) {
                EmGravios_ChangeState(em, EM_GROUP_REACT, GRAV_REACT_STATUS4_DOWNED_END, EM_SYNC_OWNER);
                break;
            }
            EmGravios_ChangeState(em, EM_GROUP_REACT, GRAV_REACT_STUN_TRAPPED_STRUGGLE, EM_SYNC_OWNER);
        }
        break;
    }
}

/*
 * GRAV_REACT_FLINCH: GRAV_MOT_FLINCH once, then EmGravios_EndState: parts 0
 * and 7, and an unbroken part 6 (EmGravios_StartPart6Flinch).
 */
void EmGravios_Flinch(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, GRAV_MOT_FLINCH, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * GRAV_REACT_STUN, on EM_DMG_STATUS5 outside a hold (EmGravios_ChangeState:
 * Em_BeginStatus5): GRAV_MOT_STUN with stun stars until em->timer runs out,
 * GRAV_MOT_GET_UP, then GRAV_IDLE_STUN_SHAKE_OFF (Em_EndStatus5).
 */
void EmGravios_Stun(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, GRAV_MOT_STUN, 0, 0);
        StunStars_CreateEm(em, 1, 34, (s32) ((f32) em->timer + (em->motion[0].frameCount / 2.0f)), 150.0f, 3.0f);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, GRAV_MOT_GET_UP_B, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGravios_ChangeState(em, EM_GROUP_IDLE, GRAV_IDLE_STUN_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmGravios_ChangeState(em, EM_GROUP_IDLE, GRAV_IDLE_STUN_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * GRAV_REACT_PART_BREAK_END, after EmGravios_PartBreak
 * (EmGravios_ChangeState: Em_ResetToBasePose, Em_BreakPart(em, 2)): it sets
 * no motion and waits for motion layer 0 to stop, then EmGravios_EndState.
 * It starts once GRAV_MOT_PART_BREAK has stopped, so it ends at once unless
 * Em_ResetToBasePose restarted a motion.
 */
void EmGravios_PartBreakEnd(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * GRAV_REACT_STATUS2_DOWNED_END and GRAV_REACT_STATUS1_DOWNED_END, a lying
 * status ending while held: GRAV_MOT_DOWNED_GET_UP with blind 2 and
 * holdTimer 5, then Em_SetTrapTimeGoal and back to
 * GRAV_REACT_TRAPPED_STRUGGLE. The first comes from GRAV_IDLE_STATUS2_DOWNED
 * (Em_EndStatus2 on entry), the second from GRAV_IDLE_STATUS1_DOWNED
 * (Em_EndStatus1 on entry).
 */
void EmGravios_DownedGetUpHeld(Em *em)
{
    em->holdTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GRAV_MOT_DOWNED_GET_UP, 0, 0);
        em->blind = 2;
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetTrapTimeGoal(em);
            em->blind = 0;
            EmGravios_ChangeState(em, EM_GROUP_REACT, GRAV_REACT_TRAPPED_STRUGGLE, EM_SYNC_OWNER);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmGravios_ChangeState(em, EM_GROUP_REACT, GRAV_REACT_TRAPPED_STRUGGLE, EM_SYNC_OWNER);
        }
        break;
    }
}

/*
 * GRAV_REACT_HOLD2, on EM_DMG_ATTACK13 (EmGravios_ChangeState ends every
 * status): Em_SetHold2 every frame; GRAV_MOT_FLINCH_B without root motion to
 * frame 52, then GRAV_MOT_STATUS4 until em->timer runs out, then
 * GRAV_IDLE_HOLD2_SHAKE_OFF (holdTimer2 0).
 */
void EmGravios_Hold2(Em *em)
{
    Em_SetHold2(em);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, GRAV_MOT_FLINCH_B, 0, 0);
        Player_SetFlag((Player *) em, EM_FLAG_NO_ROOT_MOTION);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 52.0f) != 0) {
            em->step++;
            Em_SetMotion(em, GRAV_MOT_STATUS4, 12, 0);
            Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            EmGravios_ChangeState(em, EM_GROUP_IDLE, GRAV_IDLE_HOLD2_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmGravios_ChangeState(em, EM_GROUP_IDLE, GRAV_IDLE_HOLD2_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * GRAV_EVENT_INTRO to GRAV_EVENT_INTRO_D, the volcano's opening scene
 * (senseDelay and noHitTimer 5 each frame): placed at fixed coordinates
 * facing a fixed goal, it walks to the goal on three layers (the layer-1
 * gesture GRAV_MOT_UPPER_B looped) until within 200 or for 300 frames, eats
 * at the wall (GRAV_MOT_EAT to frame 250, then again from frame 80 to 400)
 * with FallingDebris, an Effect02 and a PartFx at bone 36 on frames 68, 94
 * and 204, idles, turns (GRAV_MOT_TURN_LEFT), breathes the beam (GRAV_BONE_BREATH,
 * held 55 frames) at frame 136 of GRAV_MOT_BEAM, plays GRAV_MOT_BEAM_END,
 * and idles until the scene's event flag is set; then EmGravios_EndState.
 */
void EmGravios_Intro(Em *em, EmWork *work)
{
    f32 bonePos[3];
    f32 goalDist;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        GmEm_ResetSize(em);
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        em->posX = 9840.0f;
        em->posY = 2.0f;
        em->posZ = 13000.0f;
        em->goal[0] = 10740.0f;
        em->goal[1] = 2.0f;
        em->goal[2] = 14275.0f;
        em->rotX = 0;
        em->rotY = Math_AngleXZ(&em->posX, em->goal);
        em->rotZ = 0;
        Em_SetMotion(em, GRAV_MOT_IDLE, 0, 0);
        em->timer = 300;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            em->timer = 300;
            Em_SetLayerMotion(em, EM_MOTION_ID(0, GRAV_MOT_WALK), 0, 0, 0);
            Em_SetLayerMotion(em, EM_MOTION_ID(1, GRAV_MOT_UPPER_B), 0, 0, 1);
            Em_SetLayerMotion(em, EM_MOTION_ID(2, GRAV_MOT_WALK), 0, 0, 2);
        }
        break;
    case 2:
        if (em->motion[1].playing == 0) {
            Em_SetLayerMotion(em, EM_MOTION_ID(1, GRAV_MOT_UPPER_B), 0, 0, 1);
        }
        goalDist = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer <= 0 || goalDist <= 200.0f) {
            em->step++;
            Em_SetMotion(em, GRAV_MOT_EAT, 0, 0);
        }
        break;
    case 3:
        if ((EmMotion_PassedFrameScaled(em, 68.0f, 0) || EmMotion_PassedFrameScaled(em, 94.0f, 0) || EmMotion_PassedFrameScaled(em, 204.0f, 0)) && ((s32)Player_InCurrentArea(em) & 0xFF)) {
            FallingDebris_Create(em, 1);
            Em_BonePos(em, 36, bonePos);
            HitSpark_SpawnForOwner(em, 0, 9, 0, bonePos, 1.0f);
            PartFx_SpawnAt(bonePos, 20, 4.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 250.0f, 0)) {
            em->step++;
            Em_SetMotion(em, GRAV_MOT_EAT, 0, 80);
        }
        break;
    case 4:
        if ((EmMotion_PassedFrameScaled(em, 68.0f, 0) || EmMotion_PassedFrameScaled(em, 94.0f, 0) || EmMotion_PassedFrameScaled(em, 204.0f, 0)) && ((s32)Player_InCurrentArea(em) & 0xFF)) {
            FallingDebris_Create(em, 1);
            Em_BonePos(em, 36, bonePos);
            HitSpark_SpawnForOwner(em, 0, 9, 0, bonePos, 1.0f);
            PartFx_SpawnAt(bonePos, 20, 4.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 400.0f, 0)) {
            em->step++;
            Em_SetLayerMotion(em, EM_MOTION_ID(0, GRAV_MOT_IDLE), 0, 0, 0);
            Em_SetLayerMotion(em, EM_MOTION_ID(1, GRAV_MOT_UPPER_B), 0, 0, 1);
            Em_SetLayerMotion(em, EM_MOTION_ID(2, GRAV_MOT_IDLE), 0, 0, 2);
        }
        break;
    case 5:
        if (em->motion[1].playing == 0) {
            em->step++;
            em->rotY -= ANGLE_TURN / 4;
            Em_SetMotion(em, GRAV_MOT_TURN_RIGHT, 0, 0);
        }
        break;
    case 6:
        em->rotY -= (u32) ((16384.0f / (em->motion[0].frameCount / 2.0f)) * em->motionSpeed);
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, GRAV_MOT_BEAM, 0, 0);
            em->timer = 4;
        }
        break;
    case 7:
        if (EmMotion_PassedFrameScaled(em, 136.0f, 0)) {
            em->step++;
            EmRangedAttack_CreateAtBoneTimed(em, GRAV_BONE_BREATH, EMRANGED_GRAVIOS_BEAM, 1, 0, 0, 55);
        }
    case 8:
        if (em->motion[0].playing == 0) {
            if (--em->timer <= 0) {
                em->lookAtTarget = 0;
                em->step++;
                Em_SetMotion(em, GRAV_MOT_BEAM_END, 0, 0);
            }
        }
        break;
    case 9:
        if (em->motion[1].playing == 0) {
            em->step++;
            Em_SetMotion(em, GRAV_MOT_IDLE, 0, 0);
        }
        break;
    case 10:
        if (Save_TestEventFlag(EVENT_FLAG_GRAVIOS_SCENE) == 1) {
            em->senseDelay = 0;
            em->noHitTimer = 0;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * GRAV_EVENT_CAPTURED, captured in holdTimer's hold (Em_SetCaptured(em, 0)
 * each frame; EmGravios_ChangeState began status 1): GRAV_MOT_DOWNED with
 * reaction set 1 and poison cleared, QuestEm_Captured when the motion ends,
 * then 1800 frames (while QuestEm_CanAppear) and a 150-frame fade (alpha) to
 * GRAV_DEFEAT_REAPPEAR; the lying puffs and their sound every frame
 * (EmGravios_DownedPuffsAndSe).
 */
void EmGravios_Captured(Em *em, EmWork *work)
{
    s32 prevTimer;
    s32 timeLeft;

    Em_SetCaptured(em, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GRAV_MOT_DOWNED, 0, 0);
        Em_LoadReactionSet(em, 1);
        Em_ClearPoison(em);
        em->timer = 1800;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            QuestEm_Captured(em);
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
        prevTimer = em->timer;
        if (prevTimer > 0) {
            em->timer = prevTimer - 1;
        }
        timeLeft = em->timer;
        if (timeLeft <= 0) {
            if (em->remote == 0) {
                EmGravios_ChangeState(em, EM_GROUP_DEFEAT, GRAV_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
            }
        } else {
            em->alpha = (f32) timeLeft / 150.0f;
        }
        break;
    }
    EmGravios_DownedPuffsAndSe(em);
}

/*
 * GRAV_EVENT_CAPTURED_HOLD2, captured in holdTimer2's hold
 * (Em_SetCaptured(em, 1)): it begins status 1 itself, EM_STANCE_GROUND,
 * GRAV_MOT_COLLAPSE, then GRAV_MOT_LIE with QuestEm_Captured, then the same
 * 1800-frame wait and 150-frame fade to GRAV_DEFEAT_REAPPEAR (no puffs).
 */
void EmGravios_CapturedHold2(Em *em, EmWork *work)
{
    s32 timeLeft;
    s32 prevTimer;

    Em_SetCaptured(em, 1);
    switch (em->step) {
    case 0:
        em->step++;
        Em_BeginStatus1(em);
        Em_ClearPoison(em);
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GRAV_MOT_COLLAPSE, 0, 0);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, GRAV_MOT_LIE, 0, 0);
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
        prevTimer = em->timer;
        if (prevTimer > 0) {
            em->timer = prevTimer - 1;
        }
        timeLeft = em->timer;
        if (timeLeft <= 0) {
            if (em->remote == 0) {
                EmGravios_ChangeState(em, EM_GROUP_DEFEAT, GRAV_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
                break;
            }
        } else {
            em->alpha = (f32) timeLeft / 150.0f;
        }
        break;
    }
}

/*
 * GRAV_VS_EAT_BAIT, the versus quests' end: waits for Versus_DefeatCamera,
 * idles 30 frames creating the versus bait once (5 frames in, at
 * EmGravios_CarveThrowPt, for the player of em->side), plays GRAV_MOT_EAT to
 * its end and GRAV_MOT_ROAR, then after 30 frames calls Versus_RouteEmByTeam
 * every frame.
 */
void EmGravios_VsEatBait(Em *em, EmWork *work)
{
    Vec3f baitPos = EmGravios_CarveThrowPt;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        if (Versus_DefeatCamera(em) == 1) {
            em->step++;
            em->stateVar0 = 5;
            em->stateVar1 = 0;
            Em_SetMotion(em, GRAV_MOT_IDLE, 0, 0);
            em->timer = 30;
        }
        break;
    case 1:
        if (em->stateVar0 == 0 && em->stateVar1 == 0) {
            em->stateVar1 = 1;
            VersusBait_Create(&gPlayers[(u8) em->side - 1], em, 0, 0, &baitPos.x);
        } else {
            em->stateVar0--;
        }
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, GRAV_MOT_EAT, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 30;
            Em_SetMotion(em, GRAV_MOT_ROAR, 0, 0);
        }
        break;
    case 3:
        if (--em->timer <= 0) {
            Versus_RouteEmByTeam(em);
        }
        break;
    }
}

/*
 * GRAV_VS_POSE_A: GRAV_MOT_FLINCH_F, then the idle and GRAV_VS_WAIT_BOTH.
 */
void EmGravios_VsPoseA(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, GRAV_MOT_FLINCH_F, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, GRAV_MOT_IDLE, 0, 0);
            EmGravios_ChangeState(em, EM_GROUP_VERSUS, GRAV_VS_WAIT_BOTH, EM_SYNC_SEND);
        }
        break;
    }
}

/*
 * GRAV_VS_POSE_B: GRAV_MOT_ROAR, then the idle and GRAV_VS_WAIT_BOTH.
 */
void EmGravios_VsPoseB(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, GRAV_MOT_ROAR, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, GRAV_MOT_IDLE, 0, 0);
            EmGravios_ChangeState(em, EM_GROUP_VERSUS, GRAV_VS_WAIT_BOTH, EM_SYNC_SEND);
        }
        break;
    }
}

/*
 * GRAV_VS_POSE_C: GRAV_MOT_BEAM_SHORT (no beam is created, and its motion
 * effect skips the attack in EM_GROUP_VERSUS), then GRAV_MOT_BEAM_END and
 * GRAV_VS_WAIT_BOTH; step 2 (the idle and the same change again) runs only
 * while the change it sent has not come back (mode 1 sets a large monster's
 * state when the sync returns).
 */
void EmGravios_VsPoseC(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, GRAV_MOT_BEAM_SHORT, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, GRAV_MOT_BEAM_END, 0, 0);
            EmGravios_ChangeState(em, EM_GROUP_VERSUS, GRAV_VS_WAIT_BOTH, EM_SYNC_SEND);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, GRAV_MOT_IDLE, 0, 0);
            EmGravios_ChangeState(em, EM_GROUP_VERSUS, GRAV_VS_WAIT_BOTH, EM_SYNC_SEND);
        }
        break;
    }
}

/*
 * GRAV_VS_WAIT_BOTH: idles (GRAV_MOT_IDLE unless already playing) until
 * Versus_BothDefeated, then GRAV_VS_END; EmGravios_ChangeState calls
 * Versus_MarkDefeat on entry.
 */
void EmGravios_VsWaitBoth(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        if (em->motionId[0] != EM_MOTION_ID(0, GRAV_MOT_IDLE)) {
            Em_SetMotion(em, GRAV_MOT_IDLE, 0, 0);
        }

    case 1:
        if (Versus_BothDefeated() == 1) {
            em->step++;
            EmGravios_ChangeState(em, EM_GROUP_VERSUS, GRAV_VS_END, EM_SYNC_SEND);
        }
        break;
    }
}

/*
 * GRAV_VS_END: idles 90 frames, clears senseDelay and noHitTimer, then
 * EmGravios_EndState.
 */
void EmGravios_VsEnd(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 90;
        if (em->motionId[0] != EM_MOTION_ID(0, GRAV_MOT_IDLE)) {
            Em_SetMotion(em, GRAV_MOT_IDLE, 0, 0);
        }
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            em->senseDelay = 0;
            em->noHitTimer = 0;
            EmGravios_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * GRAV_DEFEAT_DIE: GRAV_MOT_DIE, EM_STANCE_DEAD, QuestEm_Hunted, a camera
 * shake of level 2 at frame 212; after the motion, a carve point
 * (EmMine_Create(em, 0), updated each frame) and no wall collision for 1800
 * frames (while QuestEm_CanAppear); then the carve point is removed and a
 * 150-frame fade leads to GRAV_DEFEAT_REAPPEAR. noPushTimer 5 and combat off
 * each frame.
 */
void EmGravios_Die(Em *em, EmWork *work)
{
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, GRAV_MOT_DIE, 0, 0);
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
                EmGravios_ChangeState(em, EM_GROUP_DEFEAT, GRAV_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    }
}

/*
 * GRAV_DEFEAT_DIE_HELD: as EmGravios_Die while held (holdTimer kept at 5
 * until the fade), with GRAV_MOT_DOWNED instead of the death motion and no
 * camera shake.
 */
void EmGravios_DieHeld(Em *em, EmWork *work)
{
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, GRAV_MOT_DOWNED, 0, 0);
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_DEAD;
        QuestEm_Hunted(em);
        em->timer = 3600;
        em->holdTimer = 5;
        break;
    case 1:
        em->holdTimer = 5;
        em->timer--;
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
                EmGravios_ChangeState(em, EM_GROUP_DEFEAT, GRAV_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    }
}

/*
 * GRAV_DEFEAT_DIE_FALLING, killed in the air: GRAV_MOT_FALL with velY at
 * most -50 and accelY -10 (Em_MoveRotatedAccel) to the floor,
 * GRAV_MOT_FALL_LAND, and at its frame 60 GRAV_MOT_DIE_FALLING with
 * EM_STANCE_DEAD and QuestEm_Hunted; then the carve point, the wait and the
 * fade as EmGravios_Die.
 */
void EmGravios_DieFalling(Em *em, EmWork *work)
{
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_SetMotion(em, GRAV_MOT_FALL, 0, 0);
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
            Em_SetMotion(em, GRAV_MOT_FALL_LAND, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    case 2:
        if (EmMotion_PassedFrameScaled(em, 60.0f, 0) != 0) {
            em->step++;
            em->timer = 0;
            Em_SetMotion(em, GRAV_MOT_DIE_FALLING, 6, 0);
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
                EmGravios_ChangeState(em, EM_GROUP_DEFEAT, GRAV_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    }
}

/*
 * GRAV_DEFEAT_DIE_RISING, killed while hiding or rising (EmGravios_IsHiding,
 * GRAV_IDLE_ROCK_POSE or GRAV_IDLE_ROCK_BURST): EM_STANCE_DEAD and
 * QuestEm_Hunted at once, GRAV_MOT_RISE_BURST (always restarted), then
 * GRAV_MOT_DIE with the camera shake at frame 212, the carve point, the wait
 * and the fade as EmGravios_Die.
 */
void EmGravios_DieRising(Em *em, EmWork *work)
{
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_DEAD;
        em->lookAtTarget = 0;
        /* Tests the motion id against the row number, not the row's id
         * (EM_MOTION_ID), so the test always holds and the motion always
         * restarts. */
        if (em->motionId[0] != GRAV_MOT_RISE_BURST) {
            Em_SetMotion(em, GRAV_MOT_RISE_BURST, 0, 0);
        }
        QuestEm_Hunted(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, GRAV_MOT_DIE, 0, 0);
            em->lookAtTarget = 0;
        }
        break;
    case 2:
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
    case 3:
        EmMine_Update(em);
        if (QuestEm_CanAppear(em) != 0) {
            if (--em->timer <= 0) {
                em->step++;
                EmMine_Remove(em);
                em->timer = 150;
            }
        }
        break;
    case 4:
        if (em->timer > 0) {
            em->timer--;
        }
        if (em->timer <= 0) {
            if (em->remote == 0) {
                EmGravios_ChangeState(em, EM_GROUP_DEFEAT, GRAV_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    }
}

/*
 * GRAV_DEFEAT_REAPPEAR: when QuestEm_CanAppear, in the Great Arena it is
 * placed in AREA_GREAT_ARENA_1 at one of two fixed spots by em->variant and
 * goes to GRAV_IDLE; elsewhere it gets posY = gAreaGroundY[area] and
 * EM_STANCE_AIR and goes to GRAV_FLY_LAND; otherwise taskPhase++ (GmEm_Task
 * finishes the monster). EmGravios_ChangeState calls QuestEm_Appear on
 * entry.
 */
void EmGravios_Reappear(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        if (QuestEm_CanAppear(em) == 1) {
            switch (Map_DayId(gScene.map)) {
            case MAP_GREAT_ARENA:
                em->area = Area_DayId(AREA_GREAT_ARENA_1);
                switch (em->variant) {
                default:
                    em->posX = 15949.7f;
                    em->posY = 1010.0f;
                    em->posZ = 7225.8f;
                    em->rotY = 0xDA3D;
                    break;
                case 1:
                    em->posX = 11948.9f;
                    em->posY = 1010.0f;
                    em->posZ = 15923.8f;
                    em->rotY = 0x8A76;
                    break;
                }
                EmGravios_ChangeState(em, EM_GROUP_IDLE, GRAV_IDLE, EM_SYNC_LOCAL);
                break;
            default:
                em->posY = gAreaGroundY[em->area];
                em->stance = EM_STANCE_AIR;
                EmGravios_ChangeState(em, EM_GROUP_FLY, GRAV_FLY_LAND, EM_SYNC_LOCAL);
                break;
            }
        } else {
            em->taskPhase++;
        }
        break;
    }
}

/*
 * The EM_DMG_PART_FLINCH reaction for part 6: GRAV_REACT_FLINCH_PART6_BROKEN
 * when part 6 is broken (parts[6].state above 1), else GRAV_REACT_FLINCH
 * (GRAV_MOT_FLINCH, as for parts 0 and 7). Only Update calls it.
 */
void EmGravios_StartPart6Flinch(Em *em)
{
    if ((s32) em->parts[6].state > 1) {
        EmGravios_ChangeState(em, EM_GROUP_REACT, GRAV_REACT_FLINCH_PART6_BROKEN, EM_SYNC_OWNER);
        return;
    }
    EmGravios_ChangeState(em, EM_GROUP_REACT, GRAV_REACT_FLINCH, EM_SYNC_OWNER);
}

/*
 * Runs the monster's EM_GROUP_IDLE state (enum EmGraviosIdleState) through
 * its state function.
 */
void EmGravios_RunIdleState(Em *em, EmWork *work)
{
    switch (em->state) {
    case GRAV_IDLE_STAND:
        EmGravios_Stand(em, work);
        break;
    case GRAV_IDLE:
        EmGravios_Idle(em, work);
        break;
    case GRAV_IDLE_ENRAGED:
        EmGravios_IdleEnraged(em, work);
        break;
    case GRAV_IDLE_TIMED_LOOP:
        EmGravios_IdleTimedLoop(em, work);
        break;
    case GRAV_IDLE_ACTION_A:
        EmGravios_IdleActionA(em, work);
        break;
    case GRAV_IDLE_UPPER_A:
        EmGravios_UpperGestureA(em, work);
        break;
    case GRAV_IDLE_UPPER_B:
        EmGravios_UpperGestureB(em, work);
        break;
    case GRAV_IDLE_UPPER_E:
        EmGravios_UpperGestureE(em, work);
        break;
    case GRAV_IDLE_ROCK_POSE:
        EmGravios_RockPose(em, work);
        break;
    case GRAV_IDLE_PAIR_A:
        EmGravios_IdlePairA(em, work);
        break;
    case GRAV_IDLE_TURN_TO_GOAL:
        EmGravios_IdleTurnToGoal(em, work);
        break;
    case GRAV_IDLE_UPPER_C:
        EmGravios_UpperGestureC(em, work);
        break;
    case GRAV_IDLE_ROCK_BURST:
        EmGravios_BurstFromRock(em, work);
        break;
    case GRAV_IDLE_UPPER_D:
        EmGravios_UpperGestureD(em, work);
        break;
    case GRAV_IDLE_TURN_AND_BEAM:
        EmGravios_IdleTurnAndBeam(em, work);
        break;
    case GRAV_IDLE_PAIR_B:
        EmGravios_IdlePairB(em, work);
        break;
    case GRAV_IDLE_PAIR_C:
        EmGravios_IdlePairC(em, work);
        break;
    case GRAV_IDLE_THREAT:
        EmGravios_Threat(em, work);
        break;
    case GRAV_IDLE_REST:
        EmGravios_Rest(em, work);
        break;
    case GRAV_IDLE_ACTION_C:
        EmGravios_IdleActionC(em, work);
        break;
    case GRAV_IDLE_STATUS2_COLLAPSE:
        EmGravios_Status2Collapse(em, work);
        break;
    case GRAV_IDLE_HIDE_AS_ROCK:
        EmGravios_HideAsRock(em, work);
        break;
    case GRAV_IDLE_REST_END:
        EmGravios_GetUp(em, work);
        break;
    case GRAV_IDLE_STATUS2_END:
        EmGravios_GetUp(em, work);
        break;
    case GRAV_IDLE_SHAKE_OFF:
        EmGravios_ShakeOff(em, work);
        break;
    case GRAV_IDLE_REHIDE_AS_ROCK:
        EmGravios_RehideAsRock(em, work);
        break;
    case GRAV_IDLE_STATUS1_COLLAPSE:
        EmGravios_Status1Collapse(em, work);
        break;
    case GRAV_IDLE_STATUS1_END:
        EmGravios_GetUp(em, work);
        break;
    case GRAV_IDLE_STATUS2_DOWNED:
        EmGravios_Status2Downed(em, work);
        break;
    case GRAV_IDLE_STATUS1_DOWNED:
        EmGravios_Status1Downed(em, work);
        break;
    case GRAV_IDLE_STATUS4_SHAKE_OFF:
        EmGravios_ShakeOff(em, work);
        break;
    case GRAV_IDLE_HOLD2_SHAKE_OFF:
        EmGravios_ShakeOff(em, work);
        break;
    case GRAV_IDLE_STUN_SHAKE_OFF:
        EmGravios_ShakeOff(em, work);
        break;
    case GRAV_IDLE_BURROW_TO_NEXT_AREA:
        EmGravios_BurrowToNextArea(em, work);
        break;
    case GRAV_IDLE_SURFACE_AT_GOAL:
        EmGravios_Surface(em, work);
        break;
    case GRAV_IDLE_SURFACE:
        EmGravios_Surface(em, work);
        break;
    case GRAV_IDLE_RAGE_START:
        EmGravios_Enrage(em, work);
        break;
    case GRAV_IDLE_PENDING_RAGE_START:
        EmGravios_Enrage(em, work);

    default:
        break;
    }
}

/*
 * Runs the monster's EM_GROUP_MOVE state (enum EmGraviosMoveState) through
 * its state function.
 */
void EmGravios_RunMoveState(Em *em, s32 work)
{
    switch (em->state) {
    case GRAV_MOVE_WALK:
        EmGravios_Walk(em, work, 0);
        break;
    case GRAV_MOVE_END:
        EmGravios_EndMove(em);
        break;
    case GRAV_MOVE_CHARGE_STOP:
        EmGravios_ChargeStop(em);
        break;
    case GRAV_MOVE_TURN:
        EmGravios_Turn(em, work, 0);
        break;
    case GRAV_MOVE_CHARGE:
        EmGravios_Charge(em);
        break;
    case GRAV_MOVE_WALK_B:
        EmGravios_WalkB(em);
        break;
    case GRAV_MOVE_LIMP:
        EmGravios_Limp(em);
        break;
    case GRAV_MOVE_TURN_TO_GOAL:
        EmGravios_TurnToGoal(em, (EmWork *) work);
        break;
    case GRAV_MOVE_CHARGE_SLOW:
        EmGravios_Charge(em);
        break;
    case GRAV_MOVE_TURN_TO_GOAL_RAGE_CHECK:
        EmGravios_TurnToGoal(em, (EmWork *) work);
        break;
    case GRAV_MOVE_BURROW:
        EmGravios_Burrow(em);
        break;
    case GRAV_MOVE_SURFACE:
        EmGravios_SurfaceHold(em);
        break;
    case GRAV_MOVE_TURN_NO_HIT:
        EmGravios_Turn(em, work, 1);
        break;
    case GRAV_MOVE_WALK_NO_HIT:
        EmGravios_Walk(em, work, 1);
        break;
    case GRAV_MOVE_WALK_FAST:
        EmGravios_Walk(em, work, 2);

    default:
        break;
    }
}

/*
 * Runs the monster's EM_GROUP_FLY state (enum EmGraviosFlyState) through its
 * state function.
 */
void EmGravios_RunFlyState(Em *em, EmWork *work)
{
    switch (em->state) {
    case GRAV_FLY_TAKE_OFF:
        EmGravios_FlyTakeOff(em, work);
        break;
    case GRAV_FLY_LAND:
        EmGravios_FlyLand(em, work);
        break;
    case GRAV_FLY_HOVER:
        EmGravios_FlyHover(em, work);
        break;
    case GRAV_FLY_HOP_TO_GOAL:
        EmGravios_FlyHopToGoal(em, work);
        break;
    case GRAV_FLY_HOP_FORWARD:
        EmGravios_FlyHopForward(em, work);
        break;
    case GRAV_FLY_HOP_BACK:
        EmGravios_FlyHopBack(em, work);
        break;
    case GRAV_FLY_HOVER_TO_GOAL:
        EmGravios_FlyHoverToGoal(em, work);
        break;
    case GRAV_FLY_HOVER_ONCE:
        EmGravios_FlyHoverOnce(em, work);
        break;
    case GRAV_FLY_HOVER_TURN:
        EmGravios_FlyHoverTurn(em, work);
        break;
    case GRAV_FLY_RISE:
        EmGravios_FlyRise(em, work);
        break;
    case GRAV_FLY_STRAIGHT_TO_GOAL:
        EmGravios_FlyStraightToGoal(em, work);
        break;
    case GRAV_FLY_TRAP_ESCAPE:
        EmGravios_FlyTrapEscape(em, work);
        break;
    }
}

/*
 * Runs the monster's EM_GROUP_ATTACK state (enum EmGraviosAttackState)
 * through its state function.
 */
void EmGravios_RunAttackState(Em *em, EmWork *work)
{
    switch (em->state) {
    case GRAV_ATK_TURN_AND_ATTACK_A:
        EmGravios_TurnAndAttackA(em);
        break;
    case GRAV_ATK_TURN_AND_BEAM:
        EmGravios_TurnAndBeam(em);
        break;
    case GRAV_ATK_LEAP_SLAM:
        EmGravios_LeapSlam(em, work);
        break;
    case GRAV_ATK_ATTACK_A_LOOKING:
        EmGravios_AttackALooking(em);
        break;
    case GRAV_ATK_BEAM_OR_SHOT_A:
        EmGravios_TurnAndBeamOrShot(em, work, 0);
        break;
    case GRAV_ATK_GAS_A:
        EmGravios_Gas(em, work, 0);
        break;
    case GRAV_ATK_SPIN:
        EmGravios_Spin(em);
        break;
    case GRAV_ATK_GAS_B:
        EmGravios_Gas(em, work, 1);
        break;
    case GRAV_ATK_TURN_AND_ATTACK_B:
        EmGravios_TurnAndAttackB(em);
        break;
    case GRAV_ATK_REAR_ATTACK:
        EmGravios_RearAttack(em);
        break;
    case GRAV_ATK_GAS_C:
        EmGravios_Gas(em, work, 2);
        break;
    case GRAV_ATK_BEAM_OR_SHOT_B:
        EmGravios_TurnAndBeamOrShot(em, work, 1);
        break;
    case GRAV_ATK_BEAM_LONG:
        EmGravios_BeamLong(em);
        break;
    }
}

/*
 * Runs the monster's EM_GROUP_REACT state (enum EmGraviosReactState) through
 * its state function.
 */
void EmGravios_RunReactState(Em *em, EmWork *work)
{
    switch (em->state) {
    case GRAV_REACT_FLINCH_PART6_BROKEN:
        EmGravios_FlinchPart6Broken(em);
        break;
    case GRAV_REACT_FLINCH_B:
        EmGravios_FlinchB(em);
        break;
    case GRAV_REACT_FLINCH_C:
        EmGravios_FlinchC(em);
        break;
    case GRAV_REACT_FLINCH_D:
        EmGravios_FlinchD(em);
        break;
    case GRAV_REACT_PART_BREAK:
        EmGravios_PartBreak(em);
        break;
    case GRAV_REACT_KNOCK_DOWN:
        EmGravios_KnockDown(em);
        break;
    case GRAV_REACT_KNOCK_DOWN_PART3:
        EmGravios_KnockDownPart3(em);
        break;
    case GRAV_REACT_AIR_FLINCH:
        EmGravios_AirFlinch(em, work);
        break;
    case GRAV_REACT_FALL:
        EmGravios_Fall(em, work);
        break;
    case GRAV_REACT_FALL_B:
        EmGravios_FallBlind(em, work);
        break;
    case GRAV_REACT_FLINCH_E:
        EmGravios_FlinchE(em);
        break;
    case GRAV_REACT_STATUS4:
        EmGravios_Status4(em);
        break;
    case GRAV_REACT_TRAPPED:
        EmGravios_Trapped(em);
        break;
    case GRAV_REACT_TRAPPED_STRUGGLE:
        EmGravios_TrapStruggle(em);
        break;
    case GRAV_REACT_STATUS4_DOWNED:
        EmGravios_DownedWhileHeld(em, (s32) work, 0);
        break;
    case GRAV_REACT_FLINCH:
        EmGravios_Flinch(em);
        break;
    case GRAV_REACT_STUN:
        EmGravios_Stun(em);
        break;
    case GRAV_REACT_PART_BREAK_END:
        EmGravios_PartBreakEnd(em);
        break;
    case GRAV_REACT_STATUS4_DOWNED_END:
        EmGravios_TrapStruggle(em);
        break;
    case GRAV_REACT_STATUS2_DOWNED_END:
        EmGravios_DownedGetUpHeld(em);
        break;
    case GRAV_REACT_STATUS1_DOWNED_END:
        EmGravios_DownedGetUpHeld(em);
        break;
    case GRAV_REACT_HOLD2:
        EmGravios_Hold2(em);
        break;
    case GRAV_REACT_STUN_TRAPPED:
        EmGravios_DownedWhileHeld(em, (s32) work, 1);
        break;
    case GRAV_REACT_STUN_TRAPPED_STRUGGLE:
        EmGravios_TrapStruggle(em);
        break;
    }
}

/*
 * Runs the monster's EM_GROUP_DEFEAT state (enum EmGraviosDefeatState)
 * through its state function.
 */
void EmGravios_RunDefeatState(Em *em, EmWork *work)
{
    em->noPushTimer = 5;
    switch (em->state) {
    case GRAV_DEFEAT_DIE:
        EmGravios_Die(em, work);
        break;
    case GRAV_DEFEAT_DIE_HELD:
        EmGravios_DieHeld(em, work);
        break;
    case GRAV_DEFEAT_DIE_FALLING:
        EmGravios_DieFalling(em, work);
        break;
    case GRAV_DEFEAT_DIE_RISING:
        EmGravios_DieRising(em, work);
        break;
    case GRAV_DEFEAT_REAPPEAR:
        EmGravios_Reappear(em, work);
        break;
    }
}

/*
 * Runs the monster's EM_GROUP_EVENT state (enum EmGraviosEventState;
 * EM_GROUP_EVENT2 runs here too) through its state function: the opening
 * scene and the two captures.
 */
void EmGravios_RunEventState(Em *em, EmWork *work)
{
    switch (em->state) {
    case GRAV_EVENT_INTRO:
        EmGravios_Intro(em, work);
        break;
    case GRAV_EVENT_INTRO_B:
        EmGravios_Intro(em, work);
        break;
    case GRAV_EVENT_INTRO_C:
        EmGravios_Intro(em, work);
        break;
    case GRAV_EVENT_INTRO_D:
        EmGravios_Intro(em, work);
        break;
    case GRAV_EVENT_CAPTURED:
        EmGravios_Captured(em, work);
        break;
    case GRAV_EVENT_CAPTURED_HOLD2:
        EmGravios_CapturedHold2(em, work);

    default:
        break;
    }
}

/*
 * Runs the monster's EM_GROUP_VERSUS state (enum EmGraviosVersusState)
 * through its state function.
 */
void EmGravios_RunVersusState(Em *em, EmWork *work)
{
    switch (em->state) {
    case GRAV_VS_EAT_BAIT:
        EmGravios_VsEatBait(em, work);
        break;
    case GRAV_VS_POSE_A:
        EmGravios_VsPoseA(em, work);
        break;
    case GRAV_VS_POSE_B:
        EmGravios_VsPoseB(em, work);
        break;
    case GRAV_VS_POSE_C:
        EmGravios_VsPoseC(em, work);
        break;
    case GRAV_VS_WAIT_BOTH:
        EmGravios_VsWaitBoth(em, work);
        break;
    case GRAV_VS_END:
        EmGravios_VsEnd(em, work);
        break;
    }
}

/*
 * The per-frame update (GmEm_Task): the combat timer, the area trigger, the
 * hold while no player is near, and fxMotion (frames since landing) counted
 * down; then it acts on Em_UpdateDamage's result. Killed or dead:
 * GRAV_DEFEAT_DIE_FALLING in the air, DIE_HELD when held, DIE_RISING when
 * hiding or in GRAV_IDLE_ROCK_POSE or ROCK_BURST, else GRAV_DEFEAT_DIE.
 * EM_DMG_ATTACK9 or ATTACK6 (a trap) outside a hold: the trap time and
 * GRAV_REACT_TRAPPED. EM_DMG_STATUS_IN_AIR: GRAV_REACT_FALL. EM_DMG_STATUS4:
 * GRAV_REACT_STATUS4_DOWNED when held, else GRAV_REACT_STATUS4.
 * EM_DMG_CAPTURED: GRAV_EVENT_CAPTURED (hold), GRAV_EVENT_CAPTURED_HOLD2
 * (hold 2), else status 1 and GRAV_IDLE_STATUS1_COLLAPSE. EM_DMG_STATUS2:
 * GRAV_IDLE_STATUS2_DOWNED when held, else GRAV_IDLE_STATUS2_COLLAPSE.
 * EM_DMG_HIT_LYING ends the lying state: GRAV_IDLE_REST to REST_END with a
 * rage check, the collapses to their ends, the downed states to the react
 * states' get-ups. EM_DMG_PART8_DOWN: GRAV_REACT_PART_BREAK.
 * EM_DMG_PART_FLINCH: GRAV_REACT_FALL in the air, else by flinchPart (part 6
 * through EmGravios_StartPart6Flinch; parts 0 and 7 GRAV_REACT_FLINCH; 5
 * FLINCH_C; 1 and 2 FLINCH_D; a broken part 3 KNOCK_DOWN_PART3, another
 * broken part KNOCK_DOWN, an unbroken one FLINCH_B). EM_DMG_STATUS5:
 * GRAV_REACT_STUN_TRAPPED when held, else GRAV_REACT_STUN. EM_DMG_ATTACK13:
 * GRAV_REACT_HOLD2. In the volcano quest whose opening scene this family
 * plays, a Gravios or Black Gravios in AREA_VOLCANO_6 waits there for the
 * scene until its event flag is set, and enters GRAV_EVENT_INTRO when the
 * scene runs; the pending script waits meanwhile. Then the state runs (twice
 * on a state request), and the flight sets guardTimer while flyGuard is 1.
 */
void EmGravios_Update(Em *em)
{
    u8 dmgInfo[4];
    EmFlyerWork *flyWork = (EmFlyerWork *)&em->work;

    Em_UpdateCombatTimer(em);
    Em_RaisePlayerAreaTriggerLax(em);
    Em_HoldPosNoPlayers(em, 0, 1);
    if (flyWork->fxMotion != 0) {
        flyWork->fxMotion -= 1;
    }
    switch (Em_UpdateDamage(em, dmgInfo) & 0xFF) {
    case EM_DMG_NONE:
    case EM_DMG_FLASH:
    case EM_DMG_HIT_MODE3:
    case EM_DMG_HIT:
    case EM_DMG_ATTACK5:
    case EM_DMG_ATTACK5_FLINCH:
        break;
    case EM_DMG_KILLED:
    case EM_DMG_DEAD:
        if (em->stance == EM_STANCE_AIR) {
            EmGravios_ChangeState(em, EM_GROUP_DEFEAT, GRAV_DEFEAT_DIE_FALLING, EM_SYNC_OWNER);
        } else if ((s8) em->holdTimer != 0) {
            EmGravios_ChangeState(em, EM_GROUP_DEFEAT, GRAV_DEFEAT_DIE_HELD, EM_SYNC_OWNER);
        } else if (EmGravios_IsHiding(em) == 1 || (em->stateGroup == EM_GROUP_IDLE && em->state == GRAV_IDLE_ROCK_POSE) || (em->stateGroup == EM_GROUP_IDLE && em->state == GRAV_IDLE_ROCK_BURST)) {
            EmGravios_ChangeState(em, EM_GROUP_DEFEAT, GRAV_DEFEAT_DIE_RISING, EM_SYNC_OWNER);
        } else {
            EmGravios_ChangeState(em, EM_GROUP_DEFEAT, GRAV_DEFEAT_DIE, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_ATTACK9:
    case EM_DMG_ATTACK6:
        if ((s8) em->holdTimer == 0) {
            Em_SetTrapTime(em, dmgInfo);
            Em_SetTrapTimeGoal(em);
            EmGravios_ChangeState(em, EM_GROUP_REACT, GRAV_REACT_TRAPPED, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_STATUS_IN_AIR:
        if (em->stateGroup != EM_GROUP_REACT || em->state != GRAV_REACT_FALL) {
            EmGravios_ChangeState(em, EM_GROUP_REACT, GRAV_REACT_FALL, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_STATUS4:
        if ((s8) em->holdTimer != 0) {
            Em_SetStatus4Timer(em);
            EmGravios_ChangeState(em, EM_GROUP_REACT, GRAV_REACT_STATUS4_DOWNED, EM_SYNC_OWNER);
        } else if ((em->stateGroup != EM_GROUP_REACT || em->state != GRAV_REACT_STATUS4) && (em->stateGroup != EM_GROUP_REACT || em->state != GRAV_REACT_FALL)) {
            Em_SetStatus4Timer(em);
            EmGravios_ChangeState(em, EM_GROUP_REACT, GRAV_REACT_STATUS4, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_CAPTURED:
        if ((s8) em->holdTimer != 0) {
            EmGravios_ChangeState(em, EM_GROUP_EVENT, GRAV_EVENT_CAPTURED, EM_SYNC_OWNER);
        } else if ((s8) em->holdTimer2 != 0) {
            EmGravios_ChangeState(em, EM_GROUP_EVENT, GRAV_EVENT_CAPTURED_HOLD2, EM_SYNC_OWNER);
        } else if ((em->stateGroup != EM_GROUP_IDLE || em->state != GRAV_IDLE_STATUS1_COLLAPSE) && (em->stateGroup != EM_GROUP_REACT || em->state != GRAV_REACT_FALL)) {
            Em_SetStatus1Timer(em);
            EmGravios_ChangeState(em, EM_GROUP_IDLE, GRAV_IDLE_STATUS1_COLLAPSE, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_STATUS2:
        if ((s8) em->holdTimer != 0) {
            Em_SetStatus2Timer(em);
            EmGravios_ChangeState(em, EM_GROUP_IDLE, GRAV_IDLE_STATUS2_DOWNED, EM_SYNC_OWNER);
        } else if ((em->stateGroup != EM_GROUP_IDLE || em->state != GRAV_IDLE_STATUS2_COLLAPSE) && (em->stateGroup != EM_GROUP_REACT || em->state != GRAV_REACT_FALL)) {
            Em_SetStatus2Timer(em);
            EmGravios_ChangeState(em, EM_GROUP_IDLE, GRAV_IDLE_STATUS2_COLLAPSE, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_HIT_LYING:
        switch (em->state) {
        case GRAV_IDLE_REST:
            EmGravios_ChangeState(em, EM_GROUP_IDLE, GRAV_IDLE_REST_END, EM_SYNC_OWNER);
            em->scriptPending = 0;
            Em_CheckRageByHp(em, em->rageThreshold);
            break;
        case GRAV_IDLE_STATUS2_COLLAPSE:
            EmGravios_ChangeState(em, EM_GROUP_IDLE, GRAV_IDLE_STATUS2_END, EM_SYNC_OWNER);
            em->scriptPending = 0;
            break;
        case GRAV_IDLE_STATUS1_COLLAPSE:
            EmGravios_ChangeState(em, EM_GROUP_IDLE, GRAV_IDLE_STATUS1_END, EM_SYNC_OWNER);
            em->scriptPending = 0;
            break;
        case GRAV_IDLE_STATUS2_DOWNED:
            EmGravios_ChangeState(em, EM_GROUP_REACT, GRAV_REACT_STATUS2_DOWNED_END, EM_SYNC_OWNER);
            em->scriptPending = 0;
            break;
        case GRAV_IDLE_STATUS1_DOWNED:
            EmGravios_ChangeState(em, EM_GROUP_REACT, GRAV_REACT_STATUS1_DOWNED_END, EM_SYNC_OWNER);
            em->scriptPending = 0;
            break;
        }
        break;
    case EM_DMG_PART8_DOWN:
        EmGravios_ChangeState(em, EM_GROUP_REACT, GRAV_REACT_PART_BREAK, EM_SYNC_OWNER);
        break;
    case EM_DMG_PART_FLINCH:
        Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
        if (em->stance == EM_STANCE_AIR) {
            EmGravios_ChangeState(em, EM_GROUP_REACT, GRAV_REACT_FALL, EM_SYNC_OWNER);
        } else {
            switch (em->flinchPart) {
            case 6:
                EmGravios_StartPart6Flinch(em);
                break;
            case 0:
            case 7:
                EmGravios_ChangeState(em, EM_GROUP_REACT, GRAV_REACT_FLINCH, EM_SYNC_OWNER);
                break;
            case 5:
                EmGravios_ChangeState(em, EM_GROUP_REACT, GRAV_REACT_FLINCH_C, EM_SYNC_OWNER);
                break;
            case 1:
            case 2:
                EmGravios_ChangeState(em, EM_GROUP_REACT, GRAV_REACT_FLINCH_D, EM_SYNC_OWNER);
                break;
            case 3:
            case 4:
            default:
                if (em->parts[em->flinchPart].state >= 2) {
                    if (em->flinchPart != 3) {
                        EmGravios_ChangeState(em, EM_GROUP_REACT, GRAV_REACT_KNOCK_DOWN, EM_SYNC_OWNER);
                    } else {
                        EmGravios_ChangeState(em, EM_GROUP_REACT, GRAV_REACT_KNOCK_DOWN_PART3, EM_SYNC_OWNER);
                    }
                } else {
                    EmGravios_ChangeState(em, EM_GROUP_REACT, GRAV_REACT_FLINCH_B, EM_SYNC_OWNER);
                }
                break;
            }
        }
        break;
    case EM_DMG_STATUS5:
        if ((s8) em->holdTimer != 0) {
            Em_SetStatus5Timer(em);
            EmGravios_ChangeState(em, EM_GROUP_REACT, GRAV_REACT_STUN_TRAPPED, EM_SYNC_OWNER);
        } else if ((em->stateGroup != EM_GROUP_REACT || em->state != GRAV_REACT_STUN) && (em->stateGroup != EM_GROUP_REACT || em->state != GRAV_REACT_FALL)) {
            Em_SetStatus5Timer(em);
            EmGravios_ChangeState(em, EM_GROUP_REACT, GRAV_REACT_STUN, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_ATTACK13:
        Em_ResetKindTimer(em);
        EmGravios_ChangeState(em, EM_GROUP_REACT, GRAV_REACT_HOLD2, EM_SYNC_OWNER);
        break;
    }
    switch (gQuest.stage) {
    case QUEST_ID_GRAVIOS_SCENE:
        if ((em->type == EM_KIND_GRAVIOS || em->type == EM_KIND_BLACK_GRAVIOS) && Area_DayId(em->area) == AREA_VOLCANO_6 && Save_TestEventFlag(EVENT_FLAG_GRAVIOS_SCENE) == 0) {
            if (Area_DayId(gScene.area) == AREA_VOLCANO_6 && gScene.eventRunning == 1 && em->stateGroup != EM_GROUP_EVENT) {
                EmGravios_ChangeState(em, EM_GROUP_EVENT, GRAV_EVENT_INTRO, EM_SYNC_SEND);
            }
            break;
        }
    default:
        Em_UpdatePendingScript(em);
        break;
    }
    Em_ClearHitReactMode(em);
    EmGravios_RunState(em, (s32) flyWork);
    if (em->stateRequest != 0) {
        EmGravios_RunState(em, (s32) flyWork);
        em->stateRequest = 0;
    }
    if (em->stateGroup == EM_GROUP_FLY && (s8) flyWork->flyGuard == 1) {
        em->guardTimer = 2;
    }
}

/*
 * Stores prevStateGroup and prevState and runs the group's dispatcher
 * (EM_GROUP_EVENT2 runs EM_GROUP_EVENT's); the work pointer is passed as
 * s32.
 */
void EmGravios_RunState(Em *em, s32 work)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->stateGroup) {
    case EM_GROUP_IDLE:
        EmGravios_RunIdleState(em, (EmWork *) work);
        break;
    case EM_GROUP_MOVE:
        EmGravios_RunMoveState(em, work);
        break;
    case EM_GROUP_FLY:
        EmGravios_RunFlyState(em, (EmWork *) work);
        break;
    case EM_GROUP_ATTACK:
        EmGravios_RunAttackState(em, (EmWork *) work);
        break;
    case EM_GROUP_REACT:
        EmGravios_RunReactState(em, (EmWork *) work);
        break;
    case EM_GROUP_DEFEAT:
        EmGravios_RunDefeatState(em, (EmWork *) work);
        break;
    case EM_GROUP_EVENT:
        EmGravios_RunEventState(em, (EmWork *) work);
        break;
    case EM_GROUP_EVENT2:
        EmGravios_RunEventState(em, (EmWork *) work);
        break;
    case EM_GROUP_VERSUS:
        EmGravios_RunVersusState(em, (EmWork *) work);

    default:
        break;
    }
}

/*
 * Requests camera shake of the given level (CamShake_ReqAIf) when motion
 * layer 0 passes the frame (as EmRathalos_CamShakeOnFrame).
 */
void EmGravios_CamShakeOnFrame(void *em, s32 frame, s32 level)
{
    if (EmMotion_PassedFrameScaled((Em *) em, (f32) frame, 0U) != 0) {
        CamShake_ReqAIf(em, level);
    }
}

/*
 * Empty; the default case of EmGravios_MotionEffects.
 */
static void EmGravios_MotionEffectsDefault(Em *em)
{
}

/*
 * Keeps the layer-0 motion id in the work (sndMotion), stores whether the
 * monster stands on a flagged floor (onFlaggedFloor, for
 * EmGravios_PlaySeOnFrame), runs EmGravios_MotionSounds, then per motion
 * creates the GraviosAttack hit areas, dust clouds, PartFx, camera shakes
 * and roar bursts at fixed frames, and while enraged dust clouds of kind
 * 26 every 4th frame over frame ranges. The kinds differ: the Gravios and
 * the Black Gravios get GraviosAttack 11, 1, 23 and 2 where the Basarios
 * gets 27, 25, 29 and 26 (GRAV_MOT_CHARGE, GRAV_MOT_ATTACK_A,
 * GRAV_MOT_KNOCK_DOWN_PART3 and GRAV_MOT_KNOCK_DOWN, GRAV_MOT_ATTACK_B);
 * GRAV_MOT_BEAM (attack 24) and GRAV_MOT_GAS are the Gravios kinds' only,
 * GRAV_MOT_BURROW and GRAV_MOT_SURFACE the Basarios's only.
 */
void EmGravios_MotionEffects(Em *em, EmGraviosWork *work)
{
    f32 bonePos[3];
    f32 puffOfs[3];

    if (em->motionId[0] != work->sndMotion) {
        work->sndMotion = em->motionId[0];
    }
    work->onFlaggedFloor = AreaGrid2_OnFlaggedFloor(&em->posX);
    EmGravios_MotionSounds(em, (EmWork *) work);
    switch (work->sndMotion) {
    case EM_MOTION_ID(0, GRAV_MOT_IDLE):
        break;
    case EM_MOTION_ID(0, GRAV_MOT_WALK):
        EmGravios_CamShakeOnFrame(em, 44, 1);
        EmGravios_CamShakeOnFrame(em, 116, 1);
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            GraviosAttack_Create(em, 9);
        }
        if (EmMotion_PassedFrameScaled(em, 70.0f, 0) != 0) {
            GraviosAttack_Create(em, 10);
        }
        if (EmMotion_PassedFrameScaled(em, 148.0f, 0) != 0) {
            GraviosAttack_Create(em, 9);
            break;
        }
        break;
    case EM_MOTION_ID(0, GRAV_MOT_TURN_RIGHT):
        EmGravios_CamShakeOnFrame(em, 60, 1);
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            GraviosAttack_Create(em, 5);
        }
        if (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) {
            GraviosAttack_Create(em, 6);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 2.0f, 78.0f) != 0) && !(gScene.frame & 3)) {
            DustCloud_Create(em, 26, 1, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, GRAV_MOT_TURN_LEFT):
        EmGravios_CamShakeOnFrame(em, 60, 1);
        if (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) {
            GraviosAttack_Create(em, 7);
        }
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            GraviosAttack_Create(em, 8);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 2.0f, 78.0f) != 0) && !(gScene.frame & 3)) {
            DustCloud_Create(em, 26, 0, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, GRAV_MOT_LIMP):
        if (EmMotion_PassedFrameScaled(em, 42.0f, 0) != 0) {
            GraviosAttack_Create(em, 19);
        }
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            GraviosAttack_Create(em, 20);
        }
        if (EmMotion_PassedFrameScaled(em, 74.0f, 0) != 0) {
            GraviosAttack_Create(em, 20);
            break;
        }
        break;
    case EM_MOTION_ID(0, GRAV_MOT_DESCEND):
        if (EmMotion_PassedFrameScaled(em, 52.0f, 0) || EmMotion_PassedFrameScaled(em, 104.0f, 0) || EmMotion_PassedFrameScaled(em, 162.0f, 0)) {
            PartFx_SpawnForEmScaled(em, GRAV_BONE_BODY, 7, 5.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, 13):
        if ((EmMotion_PassedFrameScaled(em, 26.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 52.0f, 0) != 0)) {
            PartFx_SpawnForEmScaled(em, 1, 7, 5.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, GRAV_MOT_HOVER):
        if (EmMotion_PassedFrameScaled(em, 8.0f, 0) || EmMotion_PassedFrameScaled(em, 86.0f, 0) || EmMotion_PassedFrameScaled(em, 160.0f, 0)) {
            if (em->posY <= (1000.0f + em->floorY)) {
                DustCloud_Create(em, 1, 0, 1.0f);
            }
        }
        if (em->rage != 0) {
            if (EmMotion_InFrameRange(em, 0, 70.0f, 84.0f) || EmMotion_InFrameRange(em, 0, 146.0f, 156.0f) || EmMotion_InFrameRange(em, 0, 220.0f, 234.0f)) {
                if (!(gScene.frame & 3)) {
                    DustCloud_Create(em, 26, (s16) (Rand_Next(1) & 1), 1.0f);
                    break;
                }
            }
        }
        break;
    case EM_MOTION_ID(0, 16):
        break;
    case EM_MOTION_ID(0, GRAV_MOT_CHARGE):
        if (EmMotion_PassedFrameScaled(em, 150.0f, 0) != 0) {
            if (em->type == EM_KIND_GRAVIOS || em->type == EM_KIND_BLACK_GRAVIOS) {
                GraviosAttack_Create(em, 11);
            } else {
                GraviosAttack_Create(em, 27);
            }
        }
        if ((EmMotion_PassedFrameScaled(em, 140.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 214.0f, 0) != 0)) {
            DustCloud_Create(em, 2, 0, 1.0f);
        }
        if ((EmMotion_PassedFrameScaled(em, 128.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 182.0f, 0) != 0)) {
            DustCloud_Create(em, 3, 0, 1.0f);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 8.0f, 116.0f) != 0) && !(gScene.frame & 3)) {
            DustCloud_Create(em, 26, (s16) (Rand_Next(1) & 1), 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, GRAV_MOT_TAKE_OFF):
        if ((EmMotion_PassedFrameScaled(em, 98.0f, 0) != 0) && (em->posY <= (1000.0f + em->floorY))) {
            DustCloud_Create(em, 1, 0, 1.0f);
        }
        if ((EmMotion_PassedFrameScaled(em, 100.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 176.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 234.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 288.0f, 0) != 0)) {
            DustCloud_Create(em, 1, 0, 1.0f);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 14.0f, 86.0f) != 0) && !(gScene.frame & 3)) {
            DustCloud_Create(em, 26, (s16) (Rand_Next(1) & 1), 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, GRAV_MOT_TOUCH_DOWN):
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            GraviosAttack_Create(em, 13);
        }
        if (EmMotion_PassedFrameScaled(em, 16.0f, 0) != 0) {
            DustCloud_Create(em, 11, 0, 1.0f);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 18.0f, 44.0f) != 0) && !(gScene.frame & 3)) {
            DustCloud_Create(em, 26, (s16) (Rand_Next(1) & 1), 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, GRAV_MOT_IDLE_LEAD_IN):
    case EM_MOTION_ID(0, 30):
        break;
    case EM_MOTION_ID(0, GRAV_MOT_LIE):
        puffOfs[0] = 0.0f;
        puffOfs[1] = 10.0f;
        puffOfs[2] = 140.0f;
        Em_SpawnLyingPuffs(em, GRAV_BONE_BREATH, puffOfs, 1.6f);
        break;
    case EM_MOTION_ID(0, 35):
        break;
    case EM_MOTION_ID(0, GRAV_MOT_ATTACK_A):
        if (EmMotion_PassedFrameScaled(em, 50.0f, 0) != 0) {
            if (em->type == EM_KIND_GRAVIOS || em->type == EM_KIND_BLACK_GRAVIOS) {
                GraviosAttack_Create(em, 1);
            } else {
                GraviosAttack_Create(em, 25);
            }
        }
        if (EmMotion_PassedFrameScaled(em, 46.0f, 0) != 0) {
            GraviosAttack_Create(em, 21);
            break;
        }
        break;
    case EM_MOTION_ID(0, GRAV_MOT_GET_UP):
        break;
    case EM_MOTION_ID(0, GRAV_MOT_COLLAPSE):
        if (EmMotion_PassedFrameScaled(em, 148.0f, 0) != 0) {
            GraviosAttack_Create(em, 15);
        }
        if (EmMotion_PassedFrameScaled(em, 156.0f, 0) != 0) {
            DustCloud_Create(em, 0, 0, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, GRAV_MOT_REST_LIE_DOWN):
        break;
    case EM_MOTION_ID(0, GRAV_MOT_SPIN_BASARIOS):
        if (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) {
            GraviosAttack_Create(em, 28);
            break;
        }
        break;
    case EM_MOTION_ID(0, GRAV_MOT_SPIN):
        EmGravios_CamShakeOnFrame(em, 44, 1);
        EmGravios_CamShakeOnFrame(em, 73, 1);
        if (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) {
            GraviosAttack_Create(em, 16);
            break;
        }
        break;
    case EM_MOTION_ID(0, 44):
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 6.0f, 38.0f) != 0) && !(gScene.frame & 3)) {
            DustCloud_Create(em, 26, 1, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, 47):
    case EM_MOTION_ID(0, 48):
    case EM_MOTION_ID(0, 49):
    case EM_MOTION_ID(0, 50):
    case EM_MOTION_ID(0, 51):
        break;
    case EM_MOTION_ID(0, 52):
        if (EmMotion_PassedFrameScaled(em, 72.0f, 0) != 0) {
            DustCloud_Create(em, 4, 0, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 80.0f, 0) != 0) {
            DustCloud_Create(em, 1, 0, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 84.0f, 0) != 0) {
            DustCloud_Create(em, 4, 1, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, GRAV_MOT_TURN_ROAR):
        if (EmMotion_PassedFrameScaled(em, 102.0f, 0) != 0) {
            GraviosAttack_Create(em, 17);
        }
        if (EmMotion_PassedFrameScaled(em, 110.0f, 0) != 0) {
            GraviosAttack_Create(em, 18);
            ElementBurst_CreateOnEm(em, 5, 4, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, GRAV_MOT_FLINCH_B):
        if (EmMotion_PassedFrameScaled(em, 48.0f, 0) != 0) {
            PartFx_SpawnForEm(em, 27, 6);
            break;
        }
        break;
    case EM_MOTION_ID(0, GRAV_MOT_FLINCH_F):
        if (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, GRAV_BONE_STEP_B, 7, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, GRAV_MOT_SHAKE_OFF):
    case EM_MOTION_ID(0, GRAV_MOT_FLINCH_D):
    case EM_MOTION_ID(0, GRAV_MOT_FLINCH_E):
        break;
    case EM_MOTION_ID(0, GRAV_MOT_PART_BREAK):
        if (EmMotion_PassedFrameScaled(em, 20.0f, 0) != 0) {
            DustCloud_Create(em, 3, 128, 1.0f);
        }
        if ((EmMotion_PassedFrameScaled(em, 128.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 164.0f, 0) != 0)) {
            DustCloud_Create(em, 3, 128, 0.6f);
        }
        if (EmMotion_PassedFrameScaled(em, 60.0f, 0) != 0) {
            DustCloud_Create(em, 1, 128, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, GRAV_MOT_FLINCH_C):
    case EM_MOTION_ID(0, 67):
        break;
    case EM_MOTION_ID(0, GRAV_MOT_DIE):
        if (EmMotion_PassedFrameScaled(em, 268.0f, 0) != 0) {
            DustCloud_Create(em, 0, 0, 0.9f);
            break;
        }
        break;
    case EM_MOTION_ID(0, GRAV_MOT_KNOCK_DOWN_PART3):
    case EM_MOTION_ID(0, GRAV_MOT_KNOCK_DOWN):
        if (EmMotion_PassedFrameScaled(em, 162.0f, 0) != 0) {
            if (em->type == EM_KIND_GRAVIOS || em->type == EM_KIND_BLACK_GRAVIOS) {
                GraviosAttack_Create(em, 23);
            } else {
                GraviosAttack_Create(em, 29);
            }
        }
        if (work->sndMotion == EM_MOTION_ID(0, GRAV_MOT_KNOCK_DOWN)) {
            if (EmMotion_PassedFrameScaled(em, 142.0f, 0) != 0) {
                DustCloud_Create(em, 18, 129, 1.0f);
            }
        } else if (EmMotion_PassedFrameScaled(em, 142.0f, 0) != 0) {
            DustCloud_Create(em, 18, 128, 1.0f);
        }
        if ((EmMotion_PassedFrameScaled(em, 148.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 188.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 238.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 282.0f, 0) != 0)) {
            DustCloud_Create(em, 1, 128, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, 70):
    case EM_MOTION_ID(0, 75):
    case EM_MOTION_ID(0, 71):
    case EM_MOTION_ID(0, 72):
        break;
    case EM_MOTION_ID(0, 73):
        EmGravios_CamShakeOnFrame(em, 26, 2);
        if ((EmMotion_ReachedFrame(em, 0, 28.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 52.0f) == 0) && !(gScene.frame & 3)) {
            DustCloud_Create(em, 10, 128, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, GRAV_MOT_STATUS4):
    case EM_MOTION_ID(0, GRAV_MOT_FALL):
        break;
    case EM_MOTION_ID(0, GRAV_MOT_FALL_LAND):
        if (EmMotion_PassedFrameScaled(em, 8.0f, 0) != 0) {
            DustCloud_Create(em, 11, 0, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, 79):
    case EM_MOTION_ID(0, GRAV_MOT_IDLE_PAIR_A):
    case EM_MOTION_ID(0, GRAV_MOT_IDLE_PAIR_B):
    case EM_MOTION_ID(0, GRAV_MOT_DIE_FALLING):
    case EM_MOTION_ID(0, 83):
        break;
    case EM_MOTION_ID(0, 84):
        if (EmMotion_PassedFrameScaled(em, 42.0f, 0) || EmMotion_PassedFrameScaled(em, 60.0f, 0) || EmMotion_PassedFrameScaled(em, 80.0f, 0)) {
            DustCloud_Create(em, 1, 0, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, 87):
        break;
    case EM_MOTION_ID(0, 90):
        if ((EmMotion_PassedFrameScaled(em, 50.0f, 0) != 0) && (em->posY <= (1000.0f + em->floorY))) {
            DustCloud_Create(em, 9, 0, 1.0f);
            DustCloud_Create(em, 14, 0, 1.0f);
        }
        if ((EmMotion_PassedFrameScaled(em, 102.0f, 0) != 0) && (em->posY <= (1000.0f + em->floorY))) {
            DustCloud_Create(em, 9, 0, 1.0f);
            DustCloud_Create(em, 14, 0, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, GRAV_MOT_TRAPPED):
        if (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0) {
            DustCloud_Create(em, 11, 128, 1.0f);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 4.0f, 60.0f) != 0) && !(gScene.frame & 3)) {
            DustCloud_Create(em, 26, (s16) (Rand_Next(1) & 1), 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, GRAV_MOT_STRUGGLE):
        if (em->rage != 0) {
            if (EmMotion_InFrameRange(em, 0, 12.0f, 24.0f) || EmMotion_InFrameRange(em, 0, 46.0f, 90.0f)) {
                if (!(gScene.frame & 3)) {
                    DustCloud_Create(em, 26, (s16) (Rand_Next(1) & 1), 1.0f);
                    break;
                }
            }
        }
        break;
    case EM_MOTION_ID(0, GRAV_MOT_DOWNED):
    case EM_MOTION_ID(0, GRAV_MOT_DOWNED_GET_UP):
        break;
    case EM_MOTION_ID(0, GRAV_MOT_CHARGE_STOP):
        if (EmMotion_PassedFrameScaled(em, 28.0f, 0) != 0) {
            DustCloud_Create(em, 2, 6, 1.8f);
        }
        if ((EmMotion_InFrameRange(em, 0, 54.0f, 120.0f) != 0) && !(gScene.frame & 3)) {
            PartFx_SpawnForEmScaled(em, GRAV_BONE_BODY, 3, 7.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, GRAV_MOT_IDLE_ENRAGED):
        if (em->rage != 0) {
            if (EmMotion_InFrameRange(em, 0, 58.0f, 270.0f) || EmMotion_InFrameRange(em, 0, 410.0f, 440.0f)) {
                if (!(gScene.frame & 3)) {
                    DustCloud_Create(em, 26, (s16) (Rand_Next(1) & 1), 1.0f);
                    break;
                }
            }
        }
        break;
    case EM_MOTION_ID(0, GRAV_MOT_IDLE_ACTION_A):
    case EM_MOTION_ID(0, GRAV_MOT_EAT):
        break;
    case EM_MOTION_ID(0, GRAV_MOT_BEAM_SHORT):
        if (em->stateGroup != EM_GROUP_VERSUS) {
            if (EmMotion_PassedFrameScaled(em, 106.0f, 0) != 0) {
                GraviosAttack_Create(em, 24);
            }
            if ((EmMotion_InFrameRange(em, 0, 64.0f, 88.0f) != 0) && !(gScene.frame & 3)) {
                DustCloud_Create(em, 26, 0, 1.0f);
            }
            if ((EmMotion_InFrameRange(em, 0, 208.0f, 268.0f) != 0) && !(gScene.frame & 3)) {
                DustCloud_Create(em, 26, 1, 1.0f);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, GRAV_MOT_ROAR):
        if (EmMotion_PassedFrameScaled(em, 110.0f, 0) != 0) {
            GraviosAttack_Create(em, 18);
            ElementBurst_CreateOnEm(em, 5, 14, 1.0f);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 64.0f, 468.0f) != 0) && !(gScene.frame & 3)) {
            DustCloud_Create(em, 26, (s16) (Rand_Next(1) & 1), 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, GRAV_MOT_RISE_BURST):
        if (EmMotion_PassedFrameScaled(em, 70.0f, 0) != 0) {
            GraviosAttack_Create(em, 3);
        }
        if (EmMotion_PassedFrameScaled(em, 74.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, GRAV_BONE_BODY, 19, 1.4f);
            if (Player_InCurrentArea(em)) {
                Em_BonePos(em, GRAV_BONE_BODY, bonePos);
                bonePos[1] = em->floorY;
                AttackBurst_CreateAtScaled(bonePos, *(u16 *) &em->rotY, 8, 1.0f);
            }
        }
        if (EmMotion_PassedFrameScaled(em, 14.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, GRAV_BONE_WING_B, 20, 8.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 32.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, GRAV_BONE_WING_A, 20, 8.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, GRAV_MOT_ATTACK_B):
        if (EmMotion_PassedFrameScaled(em, 68.0f, 0) != 0) {
            if (em->type == EM_KIND_GRAVIOS || em->type == EM_KIND_BLACK_GRAVIOS) {
                GraviosAttack_Create(em, 2);
            } else {
                GraviosAttack_Create(em, 26);
            }
        }
        if (EmMotion_PassedFrameScaled(em, 76.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, GRAV_BONE_STEP_A, 3, 2.5f);
        }
        if ((EmMotion_PassedFrameScaled(em, 72.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 74.0f, 0) != 0)) {
            PartFx_SpawnForEmScaled(em, GRAV_BONE_STEP_A, 3, 2.5f);
            break;
        }
        break;
    case EM_MOTION_ID(0, GRAV_MOT_BEAM):
        if (em->type == EM_KIND_GRAVIOS || em->type == EM_KIND_BLACK_GRAVIOS) {
            if (EmMotion_PassedFrameScaled(em, 122.0f, 0) != 0) {
                GraviosAttack_Create(em, 24);
            }
            if ((EmMotion_InFrameRange(em, 0, 64.0f, 88.0f) != 0) && !(gScene.frame & 3)) {
                DustCloud_Create(em, 26, 0, 1.0f);
            }
            if ((EmMotion_InFrameRange(em, 0, 208.0f, 268.0f) != 0) && !(gScene.frame & 3)) {
                DustCloud_Create(em, 26, 1, 1.0f);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, GRAV_MOT_GAS):
        if ((em->type == EM_KIND_GRAVIOS || em->type == EM_KIND_BLACK_GRAVIOS) && !(em->fxFlags & 1) && (EmMotion_PassedFrameScaled(em, 56.0f, 0) != 0)) {
            PartFx_SpawnForEmScaled(em, GRAV_BONE_TAIL_B, 7, 2.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, GRAV_MOT_REAR_ATTACK):
        if (EmMotion_PassedFrameScaled(em, 46.0f, 0) != 0) {
            GraviosAttack_Create(em, 4);
            break;
        }
        break;
    case EM_MOTION_ID(0, GRAV_MOT_GAS_END):
        break;
    case EM_MOTION_ID(0, GRAV_MOT_BEAM_END):
        if ((gScene.eventRunning == 0) && (em->stateGroup != EM_GROUP_VERSUS)) {
            if (EmMotion_PassedFrameScaled(em, 94.0f, 0) != 0) {
                GraviosAttack_Create(em, 32);
            }
            if (EmMotion_PassedFrameScaled(em, 64.0f, 0) != 0) {
                DustCloud_Create(em, 13, 0, em->scaleX);
            }
            if (EmMotion_PassedFrameScaled(em, 66.0f, 0) != 0) {
                DustCloud_Create(em, 21, 0, em->scaleX);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, GRAV_MOT_FLINCH):
        break;
    case EM_MOTION_ID(0, GRAV_MOT_STUN):
        if ((EmMotion_PassedFrameScaled(em, 198.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 284.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 328.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 368.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 408.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 468.0f, 0) != 0)) {
            DustCloud_Create(em, 1, 0, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, GRAV_MOT_GET_UP_B):
    case EM_MOTION_ID(0, GRAV_MOT_LEAP):
        break;
    case EM_MOTION_ID(0, GRAV_MOT_BURROW):
        if (em->type == EM_KIND_BASARIOS) {
            if (EmMotion_PassedFrameScaled(em, 22.0f, 0) != 0) {
                DustCloud_Create(em, 2, 0, 1.5f);
            }
            if (EmMotion_PassedFrameScaled(em, 82.0f, 0) != 0) {
                DustCloud_Create(em, 3, 0, 1.5f);
            }
            if (EmMotion_PassedFrameScaled(em, 102.0f, 0) != 0) {
                PartFx_SpawnForEmScaled(em, GRAV_BONE_BODY, 19, 1.0f);
            }
            if (EmMotion_PassedFrameScaled(em, 170.0f, 0) != 0) {
                DustCloud_Create(em, 11, 128, 0.8f);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, GRAV_MOT_SURFACE):
        if (em->type == EM_KIND_BASARIOS) {
            if (EmMotion_PassedFrameScaled(em, 8.0f, 0) != 0) {
                PartFx_SpawnForEmScaled(em, GRAV_BONE_BODY, 19, 1.0f);
                if (Player_InCurrentArea(em)) {
                    Em_BonePos(em, GRAV_BONE_BODY, bonePos);
                    bonePos[1] = em->floorY;
                    AttackBurst_CreateAtScaled(bonePos, *(u16 *) &em->rotY, 8, 1.0f);
                }
            }
            if (EmMotion_PassedFrameScaled(em, 206.0f, 0) != 0) {
                DustCloud_Create(em, 11, 128, 0.5f);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, GRAV_MOT_LEAP_LAND):
        EmGravios_CamShakeOnFrame(em, 2, 5);
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            GraviosAttack_Create(em, 30);
        }
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, GRAV_BONE_BODY, 19, 1.4f);
            break;
        }
        break;
    default:
        EmGravios_MotionEffectsDefault(em);
        break;
    }
}

/*
 * Callback slot 3: skips the first frame (fxStep 0 to 1), then runs
 * EmGravios_MotionEffects every frame.
 */
void EmGravios_UpdateMotionEffects(Em *em)
{
    EmFlyerWork *flyWork = (EmFlyerWork *)&em->work;

    switch (flyWork->fxStep) {
    case 0:
        flyWork->fxStep++;
        break;
    case 1:
        EmGravios_MotionEffects(em, (EmGraviosWork *) flyWork);
        break;
    }
}

/*
 * Empty; callback slot 2.
 */
void EmGravios_TableNop(void)
{
}

/*
 * Callback slot 6 (as EmRathalos_AllowPartBreak): returns 1 when the
 * once-only part-break reaction (GRAV_REACT_PART_BREAK) may run: not trapped
 * or held (GRAV_REACT_TRAPPED, TRAPPED_STRUGGLE, PART_BREAK_END and the
 * three DOWNED_END states), not in the air, and partBreakCount still 0; else
 * 0.
 */
s32 EmGravios_AllowPartBreak(Em *em)
{
    u8 stateGroup;

    /* One test and one goto per state: in this form the compiler lays out
     * the branches as the game's code does. */
    stateGroup = em->stateGroup;
    if ((stateGroup != EM_GROUP_REACT) || (em->state != GRAV_REACT_TRAPPED)) {
        if (stateGroup == EM_GROUP_REACT) {
            if (em->state != GRAV_REACT_TRAPPED_STRUGGLE) {
                goto notStruggling;
            }
            goto deny;
        }
notStruggling:
        if (stateGroup == EM_GROUP_REACT) {
            if (em->state != GRAV_REACT_PART_BREAK_END) {
                goto notPartBreakEnd;
            }
            goto deny;
        }
notPartBreakEnd:
        if (stateGroup == EM_GROUP_REACT) {
            if (em->state != GRAV_REACT_STATUS4_DOWNED_END) {
                goto notStatus4End;
            }
            goto deny;
        }
notStatus4End:
        if (stateGroup == EM_GROUP_REACT) {
            if (em->state != GRAV_REACT_STATUS2_DOWNED_END) {
                goto notStatus2End;
            }
            goto deny;
        }
notStatus2End:
        if (stateGroup == EM_GROUP_REACT) {
            if (em->state != GRAV_REACT_STATUS1_DOWNED_END) {
                goto notHeldState;
            }
            goto deny;
        }
notHeldState:
        if ((em->stance != EM_STANCE_AIR) && ((s8) em->partBreakCount == 0)) {
            return 1;
        }
        goto deny;
    }
deny:
    return 0;
}

/*
 * Sets motionSpeed 1, then per state: GRAV_IDLE becomes GRAV_IDLE_ENRAGED
 * while enraged; TURN_AND_BEAM faces the target; REST calls Em_ResetNotice;
 * STATUS2_COLLAPSE, STATUS2_DOWNED and STATUS1_DOWNED begin status 2;
 * HIDE_AS_ROCK stops the motion (motionSpeed 0); REST_END calls
 * Em_RefillNeed3; STATUS2_END ends status 2; REHIDE_AS_ROCK stops the motion
 * and leaves combat; STATUS1_COLLAPSE begins status 1; STATUS1_END ends
 * status 1; STATUS4_SHAKE_OFF ends status 4; HOLD2_SHAKE_OFF clears
 * holdTimer2; STUN_SHAKE_OFF ends status 5; SURFACE_AT_GOAL moves to the
 * goal; SURFACE calls Em_TypeSetup; RAGE_START and PENDING_RAGE_START call
 * Em_StartRage and Em_StartPendingRage. Always EM_STANCE_GROUND, then
 * Em_SetStateSync(EM_GROUP_IDLE, state, mode).
 */
void EmGravios_SetIdleState(Em *em, u16 state, u8 mode)
{
    em->motionSpeed = 1.0f;
    switch (state) {
    case GRAV_IDLE:
        if (em->rage != 0) {
            state = GRAV_IDLE_ENRAGED;
        }
        break;
    case GRAV_IDLE_TURN_AND_BEAM:
        Em_SetTargetAngle(em);
        break;
    case GRAV_IDLE_REST:
        Em_ResetNotice(em);
        break;
    case GRAV_IDLE_STATUS2_COLLAPSE:
        Em_BeginStatus2(em);
        break;
    case GRAV_IDLE_HIDE_AS_ROCK:
        em->motionSpeed = 0.0f;
        break;
    case GRAV_IDLE_REST_END:
        Em_RefillNeed3(em);
        break;
    case GRAV_IDLE_STATUS2_END:
        Em_EndStatus2(em);
        break;
    case GRAV_IDLE_REHIDE_AS_ROCK:
        em->motionSpeed = 0.0f;
        Em_SetCombat(em, 0U, 0);
        break;
    case GRAV_IDLE_STATUS1_COLLAPSE:
        Em_BeginStatus1(em);
        break;
    case GRAV_IDLE_STATUS1_END:
        Em_EndStatus1(em);
        break;
    case GRAV_IDLE_STATUS2_DOWNED:
        Em_BeginStatus2(em);
        break;
    case GRAV_IDLE_STATUS1_DOWNED:
        Em_BeginStatus2(em);
        break;
    case GRAV_IDLE_STATUS4_SHAKE_OFF:
        Em_EndStatus4(em);
        break;
    case GRAV_IDLE_HOLD2_SHAKE_OFF:
        em->holdTimer2 = 0;
        break;
    case GRAV_IDLE_STUN_SHAKE_OFF:
        Em_EndStatus5(em);
        break;
    case GRAV_IDLE_SURFACE_AT_GOAL:
        Vec_Copy(&em->posX, em->goal);
        break;
    case GRAV_IDLE_SURFACE:
        Em_TypeSetup(em);
        break;
    case GRAV_IDLE_RAGE_START:
        Em_StartRage(em);
        break;
    case GRAV_IDLE_PENDING_RAGE_START:
        Em_StartPendingRage(em);
        break;
    }
    em->stance = EM_STANCE_GROUND;
    Em_SetStateSync(em, EM_GROUP_IDLE, state, mode);
}

/*
 * Sets EM_STANCE_GROUND and motionSpeed 1. The walks (GRAV_MOVE_WALK,
 * WALK_B, WALK_FAST) become GRAV_MOVE_LIMP below 30% hp; WALK_NO_HIT makes
 * the monster unreactive, unhittable and unpushable (noReact, 5-frame
 * timers); the walks take speed row 0 (1000, 500) and, with no goal, a 90 to
 * 149 frame timer. TURN_NO_HIT sets the same flags. CHARGE and CHARGE_SLOW
 * (motionSpeed 0.7) take speed row 2 (300), LIMP row 6, both with a timer
 * when there is no goal; TURN_TO_GOAL_RAGE_CHECK checks the rage by hp.
 * GRAV_MOVE_END only sets scriptPending and returns without changing the
 * state.
 */
void EmGravios_SetMoveState(Em *em, u16 state, u8 mode)
{
    em->stance = EM_STANCE_GROUND;
    em->motionSpeed = 1.0f;
    switch (state) {
    case GRAV_MOVE_WALK:
    case GRAV_MOVE_WALK_B:
    case GRAV_MOVE_WALK_NO_HIT:
    case GRAV_MOVE_WALK_FAST:
        switch (state) {
        case GRAV_MOVE_WALK:
        default:
        case GRAV_MOVE_WALK_B:
        case GRAV_MOVE_WALK_FAST:
            if (em->hp < (s16) (0.3f * em->hpMax)) {
                /* The walks join the limp's case through a label, as the
                 * game's code shares it. */
                state = GRAV_MOVE_LIMP;
                goto limp;
            }
            break;
        case GRAV_MOVE_WALK_NO_HIT:
            em->noReact = 1;
            em->senseDelay = 5;
            em->noHitTimer = 5;
            em->noPushTimer = 5;
            break;
        }
        Em_SetSpeedScaled(em, 0, 1000.0f, 500.0f);
        if (em->remote == 0 && em->goalKind == 0) {
            em->hasGoal = 0;
            em->timer = (em->stateRand % 120 + 180) / 2;
        }
        break;
    case GRAV_MOVE_END:
        em->scriptPending = 1;
        return;
    case GRAV_MOVE_CHARGE_SLOW:
        em->motionSpeed = 0.7f;
    case GRAV_MOVE_CHARGE:
        Em_SetSpeedScaled(em, 2, 300.0f, 0.0f);
        if (em->curGoalKind == 0) {
            em->timer = (em->stateRand % 120 + 180) / 2;
        }
        break;
    case GRAV_MOVE_LIMP:
    limp:
        Em_SetSpeedScaled(em, 6, 0.0f, 0.0f);
        if (em->curGoalKind == 0) {
            em->timer = (em->stateRand % 120 + 180) / 2;
        }
        break;
    case GRAV_MOVE_TURN_TO_GOAL_RAGE_CHECK:
        Em_CheckRageByHp(em, em->rageThreshold);
        break;
    case GRAV_MOVE_TURN_NO_HIT:
        em->noReact = 1;
        em->senseDelay = 5;
        em->noHitTimer = 5;
        em->noPushTimer = 5;
        break;
    case GRAV_MOVE_CHARGE_STOP:
    case GRAV_MOVE_TURN:
    case GRAV_MOVE_TURN_TO_GOAL:
    case GRAV_MOVE_BURROW:
    case GRAV_MOVE_SURFACE:
        break;
    }
    Em_SetStateSync(em, EM_GROUP_MOVE, state, mode);
}

/*
 * Sets motionSpeed 1; GRAV_FLY_HOVER_TO_GOAL and STRAIGHT_TO_GOAL clear the
 * velocity, set velZ 20 and a timer of three times the frames to the goal;
 * GRAV_FLY_TO_AREA_POINT aims at an area point (point 0 of the area sequence
 * while one runs) with a timer of 500; GRAV_FLY_RISE sets velZ to at least
 * 20 and a timer of the frames to the goal; then
 * Em_SetStateSync(EM_GROUP_FLY, state, mode).
 */
void EmGravios_SetFlyState(Em *em, u16 state, u8 mode)
{
    em->motionSpeed = 1.0f;
    switch (state) {
    case GRAV_FLY_TAKE_OFF:
    case GRAV_FLY_HOP_FORWARD:
    case GRAV_FLY_HOP_BACK:
    case GRAV_FLY_HOVER_ONCE:
    case GRAV_FLY_TRAP_ESCAPE:
    case GRAV_FLY_LAND:
    case GRAV_FLY_HOVER:
    case GRAV_FLY_HOP_TO_GOAL:
        break;
    case GRAV_FLY_HOVER_TO_GOAL:
    case GRAV_FLY_STRAIGHT_TO_GOAL:
        Player_ClearVelocity(em);
        em->velZ = 20.0f;
        em->timer = (s32) (Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) / em->velZ) * 3;
        break;
    case GRAV_FLY_TO_AREA_POINT:
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
        break;
    case GRAV_FLY_HOVER_TURN:
        break;
    case GRAV_FLY_RISE:
        if (em->velZ <= 0.0f) {
            em->velZ = 20.0f;
        }
        if (em->remote == 0) {
            em->timer = Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) / em->velZ;
        }
        break;
    }
    Em_SetStateSync(em, EM_GROUP_FLY, state, mode);
}

/*
 * Sets motionSpeed 1; the attacks that turn first (TURN_AND_ATTACK_A and _B,
 * TURN_AND_BEAM, BEAM_OR_SHOT_A and _B) face the target (Em_SetTargetAngle);
 * then Em_SetStateSync(EM_GROUP_ATTACK, state, mode).
 */
void EmGravios_SetAttackState(Em *em, u16 state, u8 mode)
{
    em->motionSpeed = 1.0f;
    switch ((u16) state) {
    case GRAV_ATK_TURN_AND_ATTACK_A:
    case GRAV_ATK_TURN_AND_ATTACK_B:
        Em_SetTargetAngle(em);
        break;
    case GRAV_ATK_TURN_AND_BEAM:
    case GRAV_ATK_BEAM_OR_SHOT_A:
    case GRAV_ATK_BEAM_OR_SHOT_B:
        Em_SetTargetAngle(em);
        break;
    case GRAV_ATK_LEAP_SLAM:
    case GRAV_ATK_ATTACK_A_LOOKING:
    case GRAV_ATK_GAS_A:
    case GRAV_ATK_SPIN:
    case GRAV_ATK_GAS_B:
    case GRAV_ATK_REAR_ATTACK:
    case GRAV_ATK_GAS_C:
        break;
    }
    Em_SetStateSync(em, EM_GROUP_ATTACK, state, mode);
}

/*
 * Changes the monster's state. The owner retargets (Em_TargetPlayerInRange)
 * and a remote copy ignores the owner's changes (EM_SYNC_OWNER,
 * EM_SYNC_OWNER_FLAGGED); the reaction set goes back to 0. While hiding
 * (EmGravios_IsHiding) any change becomes GRAV_IDLE_ROCK_BURST (the rise):
 * always offline (Game_IsOnline), and online for modes other than 0. Then it
 * runs the group's setter, or for EM_GROUP_REACT: PART_BREAK sets fxFlags
 * bit 0, counts partBreakCount and ends every status; TRAPPED and HOLD2 end
 * every status; STATUS4 and STATUS4_DOWNED begin status 4; PART_BREAK_END
 * resets the base pose and breaks part 2; STATUS4_DOWNED_END ends status 4,
 * STATUS2_DOWNED_END status 2, STATUS1_DOWNED_END status 1; STUN and
 * STUN_TRAPPED begin status 5; STUN_TRAPPED_STRUGGLE ends status 5.
 * GRAV_DEFEAT_REAPPEAR calls QuestEm_Appear when allowed,
 * GRAV_EVENT_CAPTURED begins status 1, and GRAV_VS_WAIT_BOTH calls
 * Versus_MarkDefeat; EM_GROUP_EVENT2 sets nothing.
 */
void EmGravios_ChangeState(Em *em, u16 group, u16 state, u8 mode)
{
    if (em->remote == 0) {
        Em_TargetPlayerInRange(em);
    } else if (mode == EM_SYNC_OWNER || mode == EM_SYNC_OWNER_FLAGGED) {
        return;
    }
    Em_LoadReactionSet(em, 0);
    if (Game_IsOnline() == 0) {
        if (EmGravios_IsHiding(em) == 1) {
            group = EM_GROUP_IDLE;
            state = GRAV_IDLE_ROCK_BURST;
        }
    } else if (mode != 0) {
        if (EmGravios_IsHiding(em) == 1) {
            group = EM_GROUP_IDLE;
            state = GRAV_IDLE_ROCK_BURST;
        }
    }
    switch (group) {
    case EM_GROUP_IDLE:
        EmGravios_SetIdleState(em, state, mode);
        break;
    case EM_GROUP_MOVE:
        EmGravios_SetMoveState(em, state, mode);
        break;
    case EM_GROUP_FLY:
        EmGravios_SetFlyState(em, state, mode);
        break;
    case EM_GROUP_ATTACK:
        EmGravios_SetAttackState(em, state, mode);
        break;
    case EM_GROUP_VERSUS:
        if (state == 152) {
            Versus_MarkDefeat(em);
        }
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        break;
    case EM_GROUP_REACT:
        switch (state) {
        case GRAV_REACT_PART_BREAK:
            em->fxFlags |= 1;
            em->partBreakCount++;
            Em_EndAllStatus(em);
            break;
        case GRAV_REACT_TRAPPED:
        case GRAV_REACT_HOLD2:
            Em_EndAllStatus(em);
            break;
        case GRAV_REACT_STATUS4:
            Em_BeginStatus4(em);
            break;
        case GRAV_REACT_STATUS4_DOWNED:
            Em_BeginStatus4(em);
            break;
        case GRAV_REACT_PART_BREAK_END:
            Em_ResetToBasePose(em);
            Em_BreakPart(em, 2);
            break;
        case GRAV_REACT_STATUS4_DOWNED_END:
            Em_EndStatus4(em);
            break;
        case GRAV_REACT_STATUS2_DOWNED_END:
            Em_EndStatus2(em);
            break;
        case GRAV_REACT_STATUS1_DOWNED_END:
            Em_EndStatus1(em);
            break;
        case GRAV_REACT_STUN:
        case GRAV_REACT_STUN_TRAPPED:
            Em_BeginStatus5(em);
            break;
        case GRAV_REACT_STUN_TRAPPED_STRUGGLE:
            Em_EndStatus5(em);
            break;
        }
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        break;
    case EM_GROUP_DEFEAT:
        if (state == 4 && QuestEm_CanAppear(em) == 1) {
            QuestEm_Appear(em);
        }
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        break;
    case EM_GROUP_EVENT:
        switch (state) {
        case GRAV_EVENT_CAPTURED:
            Em_BeginStatus1(em);
            break;
        }
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        break;
    case EM_GROUP_EVENT2:
        break;
    }
}

/*
 * When motion layer 0 is not blending, sets velY from the curve
 * EmGravios_MoveVelYCurves[kind] at the motion frame (the first key's value
 * on frames 0 and 1) and moves along rotY (Em_MoveRotated). The hovering
 * states call it with GRAV_CURVE_HOVER.
 */
void EmGravios_MoveByVelYCurve(Em *em, s32 kind)
{
    EmWork *work = (EmWork *) &em->work;
    EmCurveKey *curve;
    EmCurveKey *key;
    f32 frame;
    f32 keyFrame;
    s32 i;

    curve = EmGravios_MoveVelYCurves[kind];
    if (curve != NULL && em->motion[0].blend == 0) {
        frame = em->motion[0].frame;
        if (frame == 0.0f || frame == 1.0f) {
            em->velY = curve->v;
        } else {
            i = 1;
            while (i != 0) {
                key = &curve[i];
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
 * Clears the three curve flags, stores the curve kind, turns on the velY
 * curve (kinds 2, 3, 5 and 8 to 13) and the speed curve (5, 8, 9, 10) with
 * the kind's start frame, and clears the velocity. The flight states call it
 * with GRAV_CURVE_TAKE_OFF and GRAV_CURVE_HOP.
 */
void EmGravios_StartCurves(Em *em, u8 kind)
{
    EmFlyerWork *w = (EmFlyerWork *)&em->work;

    w->curveNone = 0;
    w->curveVelY = 0;
    w->curveSpeed = 0;
    w->curveKind = kind;
    switch (w->curveKind) {
    case 2:
        w->curveVelY = 1;
        w->curveFrame = 96;
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
        w->curveFrame = 44;
        break;
    case 13:
        w->curveVelY = 1;
        w->curveFrame = 0;
        break;
    }
    Player_ClearVelocity(em);
}

/*
 * The empty first curve; returns 0.
 */
static s32 EmGravios_CurveNone(Em *em, EmWork *work)
{
    return 0;
}

extern EmCurveKey *EmGravios_VelYCurves[];

/*
 * Sets velY from EmGravios_VelYCurves[curveKind]'s key pair around the curve
 * frame; returns 2 at the table's end (velY then drops 1 a frame to -50) or
 * at a negative key time (frame reset, velY 0), else 0.
 */
static u8 EmGravios_CurveVelY(Em *em, EmWork *work)
{
    EmFlyerWork *flyWork = (EmFlyerWork *) work;
    EmCurveKey *key;
    f32 frame;
    f32 keyFrame;
    s32 i;
    EmCurveKey *curve;
    u8 result;

    curve = EmGravios_VelYCurves[((EmFlyerWork *)work)->curveKind];
    result = 0;
    if (curve == NULL) {
        return result;
    }
    frame = flyWork->curveFrame;
    if (frame != 0.0f && frame != 1.0f) {
        i = 1;
        while (i != 0) {
            key = &curve[i];
            keyFrame = key->t;
            if (frame > keyFrame && keyFrame > 0.0f) {
                i++;
            } else if (keyFrame == 0.0f) {
                result = 2;
                if (em->velY > -50.0f) {
                    em->velY -= 1.0f;
                }
                break;
            } else if (keyFrame < 0.0f) {
                flyWork->curveFrame = 0;
                result = 2;
                em->velY = 0.0f;
                break;
            } else {
                i = 0;
                em->velY = (key->v - key[-1].v) / ((keyFrame - key[-1].t) / em->motion[0].step);
            }
        }
    }
    return result;
}

extern EmCurveKey *EmGravios_SpeedCurves[];

/*
 * Sets velZ from EmGravios_SpeedCurves[curveKind]'s key pair around the
 * curve frame; returns 4 at the table's end, else 0.
 */
static u8 EmGravios_CurveSpeed(Em *em, EmWork *work)
{
    EmFlyerWork *flyWork = (EmFlyerWork *) work;
    EmCurveKey *key;
    f32 frame;
    f32 keyFrame;
    s32 i;
    EmCurveKey *curve;
    u8 result;

    curve = EmGravios_SpeedCurves[((EmFlyerWork *)work)->curveKind];
    result = 0;
    if (curve == NULL) {
        return result;
    }
    frame = flyWork->curveFrame;
    if (frame != 0.0f && frame != 1.0f) {
        i = 1;
        while (i != 0) {
            key = &curve[i];
            keyFrame = key->t;
            if (frame > keyFrame && keyFrame != 0.0f) {
                i++;
            } else if (keyFrame == 0.0f) {
                result = 4;
                break;
            } else {
                i = 0;
                em->velZ = (key->v - key[-1].v) / ((keyFrame - key[-1].t) / em->motion[0].step);
            }
        }
    }
    return result;
}

/*
 * Runs the curves whose flags are set, ORs their results, advances the curve
 * frame by the motion step, and in EM_STANCE_AIR moves along rotY with
 * acceleration (Em_MoveRotatedAccel); returns the ORed bits (2: the velY
 * curve ended, 4: the speed curve ended).
 */
u8 EmGravios_UpdateCurves(Em *em)
{
    EmFlyerWork *flyWork = (EmFlyerWork *)&em->work;
    u8 result = 0;

    if (flyWork->curveNone != 0) {
        result = EmGravios_CurveNone(em, (EmWork *) flyWork);
    }
    if (flyWork->curveVelY != 0) {
        result |= EmGravios_CurveVelY(em, (EmWork *) flyWork);
    }
    if (flyWork->curveSpeed != 0) {
        result |= EmGravios_CurveSpeed(em, (EmWork *) flyWork);
    }
    flyWork->curveFrame += (s16) em->motion[0].step;
    if (em->stance == EM_STANCE_AIR) {
        flyWork->moveAngX = 0;
        flyWork->moveAngY = em->rotY;
        flyWork->moveAngZ = 0;
        Em_MoveRotatedAccel(em, &flyWork->moveAngX);
    }
    return result;
}

/*
 * Returns 1 while the monster is in GRAV_IDLE_HIDE_AS_ROCK or
 * GRAV_IDLE_REHIDE_AS_ROCK (the hiding pose: GRAV_MOT_ROCK_POSE, shadow
 * hidden, blind, no flinch; Player_IsState on the Em), else 0. Update and
 * EmGravios_ChangeState use it.
 */
s32 EmGravios_IsHiding(Em *em)
{
    if ((s16) Player_IsState(em, 0, 22) || (s16) Player_IsState(em, 0, 26)) {
        return 1;
    }
    return 0;
}

/*
 * The lying puffs at GRAV_BONE_BREATH (offset 0, 10, 140, scale 1.6) and sound 87
 * at GRAV_BONE_HEAD whenever em->timer is a multiple of 135;
 * GRAV_IDLE_STATUS2_DOWNED, GRAV_IDLE_STATUS1_DOWNED and EmGravios_Captured
 * call it every frame.
 */
void EmGravios_DownedPuffsAndSe(Em *em)
{
    f32 puffOfs[3];

    puffOfs[1] = 10.0f;
    puffOfs[2] = 140.0f;
    puffOfs[0] = 0.0f;
    Em_SpawnLyingPuffs(em, GRAV_BONE_BREATH, puffOfs, 1.6f);
    if (em->timer % 135 == 0) {
        Em_PlaySeAtBone(em, 87, GRAV_BONE_HEAD);
    }
}

/* The file's initialised data, in the game's order */
u16 EmGravios_BasariosShots[4][2] = {
#include "game/008C4B30.inc"
};
EmTurnParams EmGravios_TurnParams[1] = {
#include "game/008C4B40.inc"
};
Vec3f EmGravios_CarveThrowPt = {
#include "game/008C4B60.inc"
};
void *EmGravios_Callbacks[8] = {
    EmGravios_SpawnEffects, EmGravios_SpawnEffects, EmGravios_TableNop, EmGravios_UpdateMotionEffects,
    GmEm_ReturnZero, EmGravios_Reinit, EmGravios_AllowPartBreak, NULL,
};
EmCurveKey EmGravios_MoveVelYCurves_0[8] = {
#include "game/008C4B90.inc"
};
EmCurveKey EmGravios_MoveVelYCurves_1[20] = {
#include "game/008C4BD0.inc"
};
EmCurveKey EmGravios_MoveVelYCurves_3[10] = {
#include "game/008C4C70.inc"
};
EmCurveKey EmGravios_MoveVelYCurves_5[6] = {
#include "game/008C4CC0.inc"
};
EmCurveKey EmGravios_MoveVelYCurves_6[10] = {
#include "game/008C4CF0.inc"
};
EmCurveKey *EmGravios_MoveVelYCurves[16] = {
    EmGravios_MoveVelYCurves_0, EmGravios_MoveVelYCurves_1, NULL, EmGravios_MoveVelYCurves_3,
    NULL, EmGravios_MoveVelYCurves_5, EmGravios_MoveVelYCurves_6, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
};
EmCurveKey EmGravios_VelYCurves_2[22] = {
#include "game/008C4D80.inc"
};
EmCurveKey EmGravios_VelYCurves_3[10] = {
#include "game/008C4E30.inc"
};
EmCurveKey EmGravios_VelYCurves_5[14] = {
#include "game/008C4E80.inc"
};
EmCurveKey EmGravios_VelYCurves_8[8] = {
#include "game/008C4EF0.inc"
};
EmCurveKey EmGravios_VelYCurves_9[18] = {
#include "game/008C4F30.inc"
};
EmCurveKey EmGravios_VelYCurves_10[14] = {
#include "game/008C4FC0.inc"
};
EmCurveKey EmGravios_VelYCurves_11[4] = {
#include "game/008C5030.inc"
};
EmCurveKey EmGravios_VelYCurves_12[8] = {
#include "game/008C5050.inc"
};
EmCurveKey EmGravios_VelYCurves_13[24] = {
#include "game/008C5090.inc"
};
EmCurveKey *EmGravios_VelYCurves[16] = {
    NULL, NULL, EmGravios_VelYCurves_2, EmGravios_VelYCurves_3,
    NULL, EmGravios_VelYCurves_5, NULL, NULL,
    EmGravios_VelYCurves_8, EmGravios_VelYCurves_9, EmGravios_VelYCurves_10, EmGravios_VelYCurves_11,
    EmGravios_VelYCurves_12, EmGravios_VelYCurves_13, NULL, NULL,
};
EmCurveKey EmGravios_SpeedCurves_5[14] = {
#include "game/008C5190.inc"
};
EmCurveKey EmGravios_SpeedCurves_8[8] = {
#include "game/008C5200.inc"
};
EmCurveKey EmGravios_SpeedCurves_9[14] = {
#include "game/008C5240.inc"
};
EmCurveKey EmGravios_SpeedCurves_10[12] = {
#include "game/008C52B0.inc"
};
EmCurveKey *EmGravios_SpeedCurves[16] = {
    NULL, NULL, NULL, NULL,
    NULL, EmGravios_SpeedCurves_5, NULL, NULL,
    EmGravios_SpeedCurves_8, EmGravios_SpeedCurves_9, EmGravios_SpeedCurves_10, NULL,
    NULL, NULL, NULL, NULL,
};
