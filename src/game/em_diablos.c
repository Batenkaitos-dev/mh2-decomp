/*
 * The Diablos family: Diablos, Black Diablos, Monoblos and White Monoblos
 * (EM_KIND_DIABLOS, EM_KIND_BLACK_DIABLOS, EM_KIND_MONOBLOS,
 * EM_KIND_WHITE_MONOBLOS) share all of this code. It runs from the family's
 * first function, EmDiablos_MotionSounds (its motions' sounds), to the next
 * family's. EmDiablos_Setup, called by the per-kind dispatch Em_TypeSetup,
 * sets a monster up; EmDiablos_Update runs it every frame: it turns
 * Em_UpdateDamage's result into a reaction, starts the desert scenes, runs
 * the current state through EmDiablos_RunState and keeps the monster on or
 * under the sand. EmDiablos_ChangeState moves to a state, through the
 * groups' setters (EmDiablos_SetIdleState ...).
 *
 * Each state function takes the monster and its work area (EmWork; the
 * family's own meanings are EmDiablosWork); Em.step is the step within the
 * state. em_diablos.h names the motions (DIAB_MOT_*), the bones (DIAB_BONE_*),
 * the sounds the code shows (DIAB_SE_*) and the states of each
 * group. The burrowing takes the flyers' group (EM_GROUP_FLY): under the
 * sand the monster is in EM_STANCE_UNDER, and its leap out has a few states
 * in the air. A charge that hits a wall leaves the horn stuck
 * (DIAB_IDLE_HORN_STUCK); the sand struggles and downs in the sand
 * (EmDiablos_InSandReactState) start half out of the sand.
 */
#include "common.h"
#include "scene.h"
#include "quest.h"
#include "em.h"
#include "shadow.h"
#include "em_diablos.h"
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
#include "vibration.h"
#include "hit_spark.h"
#include "vec_math.h"
#include "em_script.h"
#include "sound.h"
/* prototypes of callees */

/* prototypes */
void Em_AddHp(Em *em, s16 n);
/* unit's own declaration: declared here as the callers use it; with the definition's void (Em *em, s32 group, s32 state, u8 mode), the calls in EmDiablos_ChangeState, EmDiablos_SetIdleState and others compile differently */
void Em_SetStateSync(Em *em, u16 group, u16 state, u8 mode);
void Em_SpawnLyingPuffs(Em *em, s32 bone, f32 *pos, f32 scale);
s32 EmMotion_PassedFrameScaled(void *p, f32 frame, s32 layer);
/* the calls that mask the value again (u8 or & 0xFF) cast it to s32 first: MWCC keeps the game's andi that way */
extern EmTurnParams EmDiablos_TurnParams[3];
void Em_EndAllStatus(Em *em);
s16 Player_IsState(void *obj, u8 state, u8 sub);
void EmTail_Create(Em *);
/* A position copied whole: the versus bait's position is copied from
 * EmDiablos_CarveThrowPt as one struct, as the game's code does. */
extern Vec3f EmDiablos_CarveThrowPt __attribute__((aligned(8)));
extern EmCurveKey *EmDiablos_MoveVelYCurves[];
EmBone *Em_Bone(void *obj, s16 bone);
void StunStars_CreateEm(Em *em, u8 k, s16 v, s16 t, f32 y, f32 scale);
/* end of prototypes */

/*
 * Plays the family's motion sounds on their frames, switching on
 * work->sndMotion (the layer-0 motion id). Walk, the two turn motions and the
 * run (DIAB_MOT_WALK, DIAB_MOT_TURN_RIGHT, DIAB_MOT_TURN_LEFT, DIAB_MOT_RUN)
 * stay silent under the sand (EM_STANCE_UNDER); many listed ids are empty
 * cases.
 */
void EmDiablos_MotionSounds(Em *em, EmWork *work)
{
    s16 motion;

    motion = work->sndMotion;
    switch (motion) {
    case EM_MOTION_ID(0, DIAB_MOT_IDLE):
        break;
    case EM_MOTION_ID(0, DIAB_MOT_WALK):
        if (em->stance != EM_STANCE_UNDER) {
            Em_PlaySeAtBoneOnFrame(em, 52, DIAB_SE_STEP, DIAB_BONE_STEP_A);
            Em_PlaySeAtBoneOnFrame(em, 116, DIAB_SE_STEP, DIAB_BONE_STEP_B);
            break;
        }
        break;
    case EM_MOTION_ID(0, DIAB_MOT_TURN_RIGHT):
        if (em->stance != EM_STANCE_UNDER) {
            Em_PlaySeAtBoneOnFrame(em, 25, DIAB_SE_STEP, DIAB_BONE_STEP_A);
            Em_PlaySeAtBoneOnFrame(em, 56, DIAB_SE_STEP, DIAB_BONE_STEP_B);
            break;
        }
        break;
    case EM_MOTION_ID(0, DIAB_MOT_TURN_LEFT):
        if (em->stance != EM_STANCE_UNDER) {
            Em_PlaySeAtBoneOnFrame(em, 25, DIAB_SE_STEP, DIAB_BONE_STEP_B);
            Em_PlaySeAtBoneOnFrame(em, 56, DIAB_SE_STEP, DIAB_BONE_STEP_A);
            break;
        }
        break;
    case EM_MOTION_ID(0, 7):
        Em_PlaySeTypeAtBoneOnFrame(em, 6, 72, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 14, DIAB_SE_STEP, DIAB_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 38, DIAB_SE_STEP, DIAB_BONE_STEP_A);
        break;
    case EM_MOTION_ID(0, 8):
        Em_PlaySeTypeAtBoneOnFrame(em, 6, 72, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 14, DIAB_SE_STEP, DIAB_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 38, DIAB_SE_STEP, DIAB_BONE_STEP_B);
        break;
    case EM_MOTION_ID(0, DIAB_MOT_LIMP):
        Em_PlaySeTypeAtBoneOnFrame(em, 6, 83, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 6, 4, DIAB_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 76, 0, DIAB_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 28, 18, DIAB_BONE_STEP_A);
        break;
    case EM_MOTION_ID(0, DIAB_MOT_DESCEND):
        Em_PlaySeAtBoneOnFrame(em, 14, 11, DIAB_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 18, 11, DIAB_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 70, 11, DIAB_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 66, 11, DIAB_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 122, 11, DIAB_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 126, 11, DIAB_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, 12):
        Em_PlaySeAtBoneOnFrame(em, 6, 12, DIAB_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 4, 12, DIAB_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 64, 12, DIAB_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 60, 12, DIAB_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, 13):
        Em_PlaySeAtBoneOnFrame(em, 14, 15, DIAB_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 18, 15, DIAB_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 44, 15, DIAB_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 40, 15, DIAB_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 66, 15, DIAB_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 70, 15, DIAB_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, DIAB_MOT_AIR):
        Em_PlaySeAtBoneOnFrame(em, 4, 12, DIAB_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 8, 12, DIAB_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 58, 11, DIAB_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 62, 11, DIAB_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 142, 11, DIAB_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 138, 11, DIAB_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 212, 11, DIAB_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 216, 11, DIAB_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, DIAB_MOT_RUN):
        if (em->stance != EM_STANCE_UNDER) {
            Em_PlaySeAtBoneOnFrame(em, 4, 39, DIAB_BONE_HEAD);
            Em_PlaySeAtBoneOnFrame(em, 4, 22, DIAB_BONE_ROOT);
            Em_PlaySeAtBoneOnFrame(em, 4, 11, DIAB_BONE_WING_A);
            Em_PlaySeAtBoneOnFrame(em, 8, 11, DIAB_BONE_WING_B);
            Em_PlaySeAtBoneOnFrame(em, 26, DIAB_SE_STEP, DIAB_BONE_STEP_A);
            Em_PlaySeAtBoneOnFrame(em, 98, 30, DIAB_BONE_STEP_A);
            Em_PlaySeAtBoneOnFrame(em, 120, 6, DIAB_BONE_STEP_A);
            Em_PlaySeAtBoneOnFrame(em, 128, 56, DIAB_BONE_STEP_A);
            Em_PlaySeAtBoneOnFrame(em, 144, 6, DIAB_BONE_STEP_B);
            Em_PlaySeAtBoneOnFrame(em, 150, 56, DIAB_BONE_STEP_B);
            Em_PlaySeAtBoneVolOnFrame(em, 120, 101, DIAB_BONE_STEP_A, 8);
            Em_PlaySeAtBoneVolOnFrame(em, 144, 101, DIAB_BONE_STEP_B, 8);
            break;
        }
        break;
    case EM_MOTION_ID(0, DIAB_MOT_LEAP):
        Em_PlaySeAtBoneOnFrame(em, 4, 35, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 12, 4, DIAB_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 4, 22, DIAB_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 72, 8, DIAB_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 56, 15, DIAB_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 60, 15, DIAB_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 138, 11, DIAB_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 142, 11, DIAB_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 190, 11, DIAB_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 194, 11, DIAB_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 238, 11, DIAB_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 242, 11, DIAB_BONE_WING_B);
        Em_PlaySeAtBoneVolOnFrame(em, 72, 102, DIAB_BONE_ROOT, 8);
        break;
    case EM_MOTION_ID(0, DIAB_MOT_LAND):
        Em_PlaySeAtBoneOnFrame(em, 2, 11, DIAB_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 6, 11, DIAB_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 12, 4, DIAB_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 4, DIAB_SE_STEP, DIAB_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 14, 9, DIAB_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 58, 22, DIAB_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 116, 0, DIAB_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 150, DIAB_SE_STEP, DIAB_BONE_STEP_A);
        Em_PlaySeAtBoneVolOnFrame(em, 14, 102, DIAB_BONE_ROOT, 8);
        break;
    case EM_MOTION_ID(0, 29):
        Em_PlaySeAtBoneOnFrame(em, 56, 31, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 4, 22, DIAB_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 4, 15, DIAB_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 8, 15, DIAB_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 120, 14, DIAB_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 124, 14, DIAB_BONE_WING_A);
        break;
    case EM_MOTION_ID(0, DIAB_MOT_REST):
        Em_PlaySeTypeAtBoneOnFrame(em, 2, 85, DIAB_BONE_HEAD);
        Em_PlaySeTypeAtBoneOnFrame(em, 182, 85, DIAB_BONE_HEAD);
        Em_PlaySeTypeAtBoneOnFrame(em, 364, 85, DIAB_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, DIAB_MOT_REST_LIE_DOWN):
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 87, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 108, 31, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 70, 0, DIAB_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 142, 3, DIAB_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 4, 23, DIAB_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 4, 19, DIAB_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, DIAB_MOT_REST_GET_UP):
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 87, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 70, 45, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 126, 3, DIAB_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 182, 3, DIAB_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 56, 22, DIAB_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, DIAB_MOT_LIE):
        Em_PlaySeTypeAtBoneOnFrame(em, 36, 87, DIAB_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, DIAB_MOT_STARE):
        Em_PlaySeTypeAtBoneOnFrame(em, 36, 76, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 20, 0, DIAB_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 40, 0, DIAB_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 60, 21, DIAB_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame(em, 60, 12, DIAB_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 12, 12, DIAB_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 166, 0, DIAB_BONE_STEP_B);
        break;
    case EM_MOTION_ID(0, DIAB_MOT_GET_UP):
        Em_PlaySeTypeAtBoneOnFrame(em, 30, 72, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 12, 12, DIAB_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 28, 2, DIAB_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 46, 3, DIAB_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 72, DIAB_SE_STEP, DIAB_BONE_STEP_B);
        break;
    case EM_MOTION_ID(0, DIAB_MOT_COLLAPSE):
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 87, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 72, 22, DIAB_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 164, 9, DIAB_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 144, 3, DIAB_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 136, 14, DIAB_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 178, 4, DIAB_BONE_BREATH);
        break;
    case EM_MOTION_ID(0, DIAB_MOT_LIE_DOWN):
        Em_PlaySeTypeAtBoneOnFrame(em, 2, 87, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 2, 23, DIAB_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 64, 3, DIAB_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 102, 16, DIAB_BONE_STEP_B);
        break;
    case EM_MOTION_ID(0, DIAB_MOT_TAIL_SPIN):
        Em_PlaySeTypeAtBoneOnFrame(em, 8, 75, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 14, 15, DIAB_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 30, 20, DIAB_BONE_TAIL_B);
        Em_PlaySeAtBoneOnFrame(em, 42, 0, DIAB_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 72, DIAB_SE_STEP, DIAB_BONE_STEP_A);
        break;
    case EM_MOTION_ID(0, DIAB_MOT_ALERT_IDLE):
        Em_PlaySeAtBoneOnFrame(em, 4, 19, DIAB_BONE_TAIL_B);
        Em_PlaySeAtBoneOnFrame(em, 38, 16, DIAB_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 56, 18, DIAB_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 88, 6, DIAB_BONE_STEP_B);
        Em_PlaySeTypeAtBoneOnFrame(em, 150, 86, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 238, 22, DIAB_BONE_ROOT);
        Em_PlaySeAtBoneVolOnFrame(em, 88, 101, DIAB_BONE_STEP_B, 8);
        break;
    case EM_MOTION_ID(0, DIAB_MOT_TURN_IN_PLACE):
        Em_PlaySeAtBoneOnFrame(em, 2, 18, DIAB_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 28, 4, DIAB_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 56, 4, DIAB_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 28, 5, DIAB_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 120, 4, DIAB_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 264, 4, DIAB_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 4, 20, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 6, 35, DIAB_BONE_HEAD);
        Em_PlaySeTypeAtBoneOnFrame(em, 78, 86, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 10, 12, DIAB_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 16, 12, DIAB_BONE_WING_A);
        break;
    case EM_MOTION_ID(0, DIAB_MOT_FLINCH):
        Em_PlaySeTypeAtBoneOnFrame(em, 6, 83, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 72, 19, DIAB_BONE_TAIL_B);
        Em_PlaySeAtBoneOnFrame(em, 48, 6, DIAB_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 48, 17, DIAB_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 66, 3, DIAB_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 116, 3, DIAB_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 144, 3, DIAB_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 52, 12, DIAB_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 56, 13, DIAB_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 194, 3, DIAB_BONE_STEP_B);
        Em_PlaySeAtBoneVolOnFrame(em, 48, 101, DIAB_BONE_STEP_B, 8);
        break;
    case EM_MOTION_ID(0, DIAB_MOT_FLINCH_E):
        Em_PlaySeTypeAtBoneOnFrame(em, 6, 83, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 10, 18, DIAB_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 38, 6, DIAB_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 78, 4, DIAB_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 110, 3, DIAB_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 90, 21, DIAB_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 170, 3, DIAB_BONE_STEP_B);
        Em_PlaySeAtBoneVolOnFrame(em, 36, 101, DIAB_BONE_STEP_B, 8);
        break;
    case EM_MOTION_ID(0, DIAB_MOT_SHAKE_OFF):
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 79, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 86, 31, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 16, 3, DIAB_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 4, 13, DIAB_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 8, 13, DIAB_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 98, 19, DIAB_BONE_TAIL_B);
        Em_PlaySeAtBoneOnFrame(em, 126, 3, DIAB_BONE_STEP_B);
        break;
    case EM_MOTION_ID(0, DIAB_MOT_FLINCH_C):
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 82, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 4, 19, DIAB_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 76, 31, DIAB_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, DIAB_MOT_FLINCH_D):
        Em_PlaySeAtBoneOnFrame(em, 4, 18, DIAB_BONE_STEP_B);
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 82, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 4, 19, DIAB_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 68, 32, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 122, 3, DIAB_BONE_STEP_B);
        break;
    case EM_MOTION_ID(0, DIAB_MOT_PART_BREAK):
        Em_PlaySeTypeAtBoneInFrames(em, 4, 244, 84, DIAB_BONE_HEAD, 6, 0);
        Em_PlaySeAtBoneOnFrame(em, 168, 42, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 252, 32, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 304, 39, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 66, 4, DIAB_BONE_TAIL_C);
        Em_PlaySeAtBoneOnFrame(em, 18, 18, DIAB_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 44, 6, DIAB_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 58, 16, DIAB_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 210, 3, DIAB_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 322, 3, DIAB_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 62, 9, DIAB_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 74, 7, DIAB_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 100, 18, DIAB_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 162, 18, DIAB_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 14, 12, DIAB_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 18, 12, DIAB_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 286, 13, DIAB_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 290, 13, DIAB_BONE_WING_B);
        Em_PlaySeAtBoneVolOnFrame(em, 44, 101, DIAB_BONE_STEP_B, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 74, 102, DIAB_BONE_ROOT, 8);
        break;
    case EM_MOTION_ID(0, DIAB_MOT_FLINCH_B):
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 82, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 4, 19, DIAB_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, 67):
        Em_PlaySeAtBoneOnFrame(em, 70, 45, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 70, 23, DIAB_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, DIAB_MOT_DIE):
        Em_PlaySeAtBoneOnFrame(em, 28, 2, DIAB_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 48, 4, DIAB_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 220, 16, DIAB_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 30, 22, DIAB_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 264, 9, DIAB_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 264, 7, DIAB_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 276, 23, DIAB_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 384, 22, DIAB_BONE_ROOT);
        Em_PlaySeTypeAtBoneOnFrame(em, 24, 82, DIAB_BONE_HEAD);
        Em_PlaySeTypeAtBoneInFrames(em, 140, 380, 44, DIAB_BONE_HEAD, 6, 0);
        Em_PlaySeAtBoneOnFrame(em, 402, 43, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 372, 15, DIAB_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 374, 14, DIAB_BONE_WING_B);
        Em_PlaySeAtBoneVolOnFrame(em, 26, 102, DIAB_BONE_ROOT, 8);
        break;
    case EM_MOTION_ID(0, DIAB_MOT_KNOCK_DOWN):
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 82, DIAB_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 4, 19, DIAB_BONE_TAIL_B);
        Em_PlaySeAtBoneOnFrame(em, 24, 9, DIAB_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 24, 17, DIAB_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, DIAB_MOT_DOWN):
    case EM_MOTION_ID(0, DIAB_MOT_DOWN_B):
        Em_PlaySeAtBoneOnFrame(em, 8, 22, DIAB_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 50, 31, DIAB_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, DIAB_MOT_DOWN_GET_UP):
        Em_PlaySeAtBoneOnFrame(em, 4, 22, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 30, 32, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 48, DIAB_SE_STEP, DIAB_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 70, DIAB_SE_STEP, DIAB_BONE_STEP_B);
        break;
    case EM_MOTION_ID(0, DIAB_MOT_DOWN_GET_UP_B):
        Em_PlaySeAtBoneOnFrame(em, 4, 22, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 30, 32, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 48, DIAB_SE_STEP, DIAB_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 70, DIAB_SE_STEP, DIAB_BONE_STEP_A);
        break;
    case EM_MOTION_ID(0, 73):
        Em_PlaySeAtBoneOnFrame(em, 4, 30, DIAB_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame(em, 4, 29, DIAB_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 8, 29, DIAB_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 10, 42, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 76, 32, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 24, 18, DIAB_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 30, 16, DIAB_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 30, 9, DIAB_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 34, 7, DIAB_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, DIAB_MOT_KNOCK_DOWN_B):
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 82, DIAB_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 4, 19, DIAB_BONE_TAIL_B);
        Em_PlaySeAtBoneOnFrame(em, 22, 9, DIAB_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 24, 17, DIAB_BONE_ROOT);
        Em_PlaySeAtBoneVolOnFrame(em, 24, 102, DIAB_BONE_ROOT, 8);
        break;
    case EM_MOTION_ID(0, DIAB_MOT_STATUS4):
        Em_PlaySeTypeAtBoneOnFrame(em, 16, 90, DIAB_BONE_HEAD);
        Em_PlaySeTypeAtBoneOnFrame(em, 110, 95, DIAB_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, DIAB_MOT_FALL):
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 83, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 20, 11, DIAB_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 58, 20, DIAB_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, DIAB_MOT_FALL_LAND):
        Em_PlaySeAtBoneOnFrame(em, 10, 9, DIAB_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 10, 7, DIAB_BONE_TAIL_B);
        Em_PlaySeAtBoneOnFrame(em, 12, 17, DIAB_BONE_ROOT);
        Em_PlaySeTypeAtBoneOnFrame(em, 16, 83, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneVolOnFrame(em, 10, 102, DIAB_BONE_ROOT, 8);
        break;
    case EM_MOTION_ID(0, DIAB_MOT_IDLE_PAIR_A):
        Em_PlaySeAtBoneOnFrame(em, 2, 22, DIAB_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame(em, 100, 23, DIAB_BONE_BREATH);
        Em_PlaySeTypeAtBoneOnFrame(em, 38, 85, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 200, 32, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 28, 3, DIAB_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 320, 3, DIAB_BONE_STEP_B);
        break;
    case EM_MOTION_ID(0, DIAB_MOT_IDLE_PAIR_B):
        Em_PlaySeAtBoneOnFrame(em, 2, 22, DIAB_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame(em, 100, 23, DIAB_BONE_BREATH);
        Em_PlaySeTypeAtBoneOnFrame(em, 38, 85, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 200, 32, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 28, 3, DIAB_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 320, 3, DIAB_BONE_STEP_A);
        break;
    case EM_MOTION_ID(0, DIAB_MOT_DIE_FALLING):
        Em_PlaySeAtBoneOnFrame(em, 4, 55, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 4, 22, DIAB_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, 83):
        Em_PlaySeAtBoneOnFrame(em, 4, 22, DIAB_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 4, 23, DIAB_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 10, 16, DIAB_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 10, 0, DIAB_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 28, 0, DIAB_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 52, 0, DIAB_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 88, 0, DIAB_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 128, 0, DIAB_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 198, 0, DIAB_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 4, 43, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 168, 39, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 162, 13, DIAB_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 166, 13, DIAB_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, 90):
        Em_PlaySeAtBoneOnFrame(em, 32, 13, DIAB_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 36, 13, DIAB_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 74, 13, DIAB_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 78, 13, DIAB_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 124, 11, DIAB_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 128, 11, DIAB_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, DIAB_MOT_SAND_STRUGGLE):
        Em_PlaySeAtBoneOnFrame(em, 22, 18, DIAB_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 72, 18, DIAB_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 4, 12, DIAB_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 8, 12, DIAB_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 48, 19, DIAB_BONE_ROOT);
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 80, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 4, 22, DIAB_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, DIAB_MOT_DOWNED):
        Em_PlaySeAtBoneOnFrame(em, 34, 22, DIAB_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 38, 22, DIAB_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 98, 4, DIAB_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame(em, 108, 3, DIAB_BONE_BREATH);
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 82, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 4, 43, DIAB_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, DIAB_MOT_DOWNED_GET_UP):
        Em_PlaySeAtBoneOnFrame(em, 4, 22, DIAB_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 8, 22, DIAB_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 108, 3, DIAB_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame(em, 4, 80, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 70, 32, DIAB_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, DIAB_MOT_CHARGE_STOP):
        Em_PlaySeAtBoneOnFrame(em, 20, 6, DIAB_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 22, 16, DIAB_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 26, 17, DIAB_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 142, 0, DIAB_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 154, 0, DIAB_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 80, 32, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 104, 22, DIAB_BONE_ROOT);
        Em_PlaySeAtBoneVolOnFrame(em, 20, 101, DIAB_BONE_STEP_B, 8);
        break;
    case EM_MOTION_ID(0, DIAB_MOT_BURROW):
        Em_PlaySeAtBoneOnFrame(em, 16, 17, DIAB_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 78, 16, DIAB_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 14, 29, DIAB_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 44, 29, DIAB_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 62, 29, DIAB_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 104, 30, DIAB_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 130, 22, DIAB_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, DIAB_MOT_EMERGE):
        Em_PlaySeAtBoneOnFrame(em, 12, 29, DIAB_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame(em, 14, 30, DIAB_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame(em, 20, 46, DIAB_BONE_HEAD);
        Em_PlaySeTypeAtBoneOnFrame(em, 168, 72, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 60, 8, DIAB_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 60, 6, DIAB_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 64, 2, DIAB_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 178, DIAB_SE_STEP, DIAB_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 232, 0, DIAB_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 174, 12, DIAB_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 178, 12, DIAB_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 12, 21, DIAB_BONE_BREATH);
        Em_PlaySeAtBoneVolOnFrame(em, 14, 102, DIAB_BONE_ROOT, 8);
        break;
    case EM_MOTION_ID(0, DIAB_MOT_POP_OUT):
        Em_PlaySeAtBoneOnFrame(em, 6, 7, DIAB_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame(em, 6, 29, DIAB_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame(em, 68, 12, DIAB_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 72, 12, DIAB_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 4, 54, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 12, 21, DIAB_BONE_BREATH);
        Em_PlaySeAtBoneVolOnFrame(em, 8, 102, DIAB_BONE_ROOT, 8);
        break;
    case EM_MOTION_ID(0, DIAB_MOT_ATTACK_A):
        Em_PlaySeAtBoneOnFrame(em, 8, DIAB_SE_STEP, DIAB_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 4, 22, DIAB_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 138, 22, DIAB_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 18, 21, DIAB_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame(em, 44, 48, DIAB_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame(em, 148, 0, DIAB_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 180, 0, DIAB_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 48, 31, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneVolOnFrame(em, 8, 101, DIAB_BONE_STEP_B, 8);
        break;
    case EM_MOTION_ID(0, DIAB_MOT_ATTACK_B):
        Em_PlaySeAtBoneOnFrame(em, 4, 52, DIAB_BONE_HEAD);
        Em_PlaySeTypeAtBoneOnFrame(em, 50, 93, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 50, 21, DIAB_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame(em, 10, 6, DIAB_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 168, 0, DIAB_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 216, 0, DIAB_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 4, 22, DIAB_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 116, 12, DIAB_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 120, 12, DIAB_BONE_WING_B);
        Em_PlaySeAtBoneVolOnFrame(em, 10, 101, DIAB_BONE_STEP_B, 8);
        break;
    case EM_MOTION_ID(0, DIAB_MOT_ATTACK_C):
        Em_PlaySeAtBoneOnFrame(em, 22, 20, DIAB_BONE_TAIL_B);
        Em_PlaySeAtBoneOnFrame(em, 44, 21, DIAB_BONE_TAIL_C);
        Em_PlaySeAtBoneOnFrame(em, 26, 36, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 60, 20, DIAB_BONE_TAIL_B);
        Em_PlaySeAtBoneOnFrame(em, 92, 21, DIAB_BONE_TAIL_C);
        Em_PlaySeAtBoneOnFrame(em, 52, 30, DIAB_BONE_TAIL_C);
        Em_PlaySeAtBoneOnFrame(em, 102, 30, DIAB_BONE_TAIL_C);
        Em_PlaySeAtBoneOnFrame(em, 198, 0, DIAB_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 242, 0, DIAB_BONE_STEP_A);
        Em_PlaySeAtBoneVolOnFrame(em, 52, 101, DIAB_BONE_TAIL_C, 8);
        Em_PlaySeAtBoneVolOnFrame(em, 102, 101, DIAB_BONE_TAIL_C, 8);
        break;
    case EM_MOTION_ID(0, DIAB_MOT_ATTACK_D):
        Em_PlaySeAtBoneOnFrame(em, 4, 18, DIAB_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 4, 53, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 10, 3, DIAB_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 112, 4, DIAB_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 216, DIAB_SE_STEP, DIAB_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 264, 3, DIAB_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 56, 29, DIAB_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 56, 20, DIAB_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 84, 22, DIAB_BONE_ROOT);
        Em_PlaySeAtBoneVolOnFrame(em, 58, 102, DIAB_BONE_ROOT, 8);
        break;
    case EM_MOTION_ID(0, DIAB_MOT_ROAR):
        Em_PlaySeAtBoneOnFrame(em, 14, 2, DIAB_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 318, 4, DIAB_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 368, DIAB_SE_STEP, DIAB_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 4, 22, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 4, 31, DIAB_BONE_HEAD);
        Em_PlaySeTypeAtBoneInFrames(em, 100, 320, DIAB_SE_ROAR, DIAB_BONE_HEAD, 8, 0);
        Em_PlaySeAtBoneOnFrame(em, 304, 22, DIAB_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, DIAB_MOT_PART6_BREAK):
        Em_PlaySeAtBoneOnFrame(em, 4, 22, DIAB_BONE_ROOT);
        Em_PlaySeTypeAtBoneOnFrame(em, 4, 94, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 266, 43, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 4, 17, DIAB_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 54, 4, DIAB_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 134, 0, DIAB_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 220, 3, DIAB_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 296, 3, DIAB_BONE_STEP_B);
        break;
    case EM_MOTION_ID(0, DIAB_MOT_HORN_STUCK):
        Em_PlaySeAtBoneOnFrame(em, 4, 55, DIAB_BONE_HEAD);
        Em_PlaySeTypeAtBoneOnFrame(em, 102, 92, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 34, 6, DIAB_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 30, 15, DIAB_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 34, 15, DIAB_BONE_WING_B);
        Em_PlaySeAtBoneVolOnFrame(em, 34, 101, DIAB_BONE_TAIL_C, 8);
        break;
    case EM_MOTION_ID(0, DIAB_MOT_HORN_PULL_FREE):
        Em_PlaySeAtBoneOnFrame(em, 4, 55, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 92, 40, DIAB_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 4, 22, DIAB_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 82, 13, DIAB_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 86, 13, DIAB_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 72, 29, DIAB_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame(em, 108, 6, DIAB_BONE_STEP_B);
        Em_PlaySeAtBoneOnFrame(em, 142, 5, DIAB_BONE_STEP_A);
        Em_PlaySeAtBoneOnFrame(em, 166, 0, DIAB_BONE_STEP_B);
        Em_PlaySeAtBoneVolOnFrame(em, 86, 102, DIAB_BONE_BREATH, 8);
        break;
    }
}

/*
 * An empty per-kind setup hook, called by the per-kind dispatch
 * Em_TypeSetup.
 */
void EmDiablos_Setup(Em *em)
{
}

/*
 * Kind callback slots 0 and 1 (EmDiablos_Callbacks): creates the alert mark
 * at DIAB_BONE_BREATH (AlertMark_Create) and the monster's shadow
 * (SHADOW_MODEL), the same as EmKutku/EmRathalos_SpawnEffects.
 */
void EmDiablos_SpawnEffects(void *em)
{
    AlertMark_Create(em, DIAB_BONE_BREATH, 0);
    Shadow_Spawn((Player *) em, SHADOW_MODEL);
}

/*
 * Scales parts 3, 4 and 8. On quest stage 0 it turns the monster by
 * ANGLE_TURN / 2 and places it at x 5000 + 1500 * slotNo, z 9000. Then the
 * idle motion, EM_STANCE_UNDER (under the sand), landTimer 0, hp from
 * Em_ComputeMaxHp, goalYaw ANGLE_TURN / 4, the turn, pitch and roll speeds,
 * rageFade 0, and surfaceDepth 1000.0. Unless fxFlags bit 0 is set, every
 * family kind hides mesh 1 and creates stage object K09.
 */
void EmDiablos_Init(Em *em)
{
    EmDiablosWork *diabWork = (EmDiablosWork *)&em->work;

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
    Em_SetMotionRow(em, DIAB_MOT_IDLE, 0, 0);
    em->stance = EM_STANCE_UNDER;
    diabWork->landTimer = 0;
    maxHp = Em_ComputeMaxHp(em, 0);
    em->hp = maxHp;
    em->hpMax = maxHp;
    em->scriptPending = 1;
    em->blind = 0;
    em->unk_InitFlag = 1;
    em->noAlert = 0;
    em->painted = 0;
    diabWork->goalYaw = ANGLE_TURN / 4;
    diabWork->pitchStep = 0x100;
    diabWork->turnSpeed = 512;
    diabWork->rollSpeed = 0x100;
    diabWork->moveAngBY = ANGLE_TURN / 8;
    diabWork->moveAngBX = 0;
    diabWork->rageFade = 0;
    em->surfaceDepth = 0x447A0000;
    fxFlags = em->fxFlags & 1;
    em->fxFlags = fxFlags;
    if (fxFlags == 0) {
        switch (em->type) {
        case EM_KIND_DIABLOS:
        case EM_KIND_BLACK_DIABLOS:
        case EM_KIND_MONOBLOS:
        case EM_KIND_WHITE_MONOBLOS:
            em->meshShown[1] = 0;
            EmTail_Create(em);
            break;
        }
    }
}

/*
 * Runs EmDiablos_Init, then starts under the sand (EM_STANCE_UNDER) in group
 * DIAB_BURROW_IDLE (the monster task calls it).
 */
void EmDiablos_Start(Em *em)
{
    EmDiablos_Init(em);
    em->stance = EM_STANCE_UNDER;
    EmDiablos_ChangeState(em, EM_GROUP_FLY, DIAB_BURROW_IDLE, EM_SYNC_LOCAL);
}

/*
 * Kind callback slot 5: runs EmDiablos_Init again.
 */
void EmDiablos_Reinit(Em *em)
{
    EmDiablos_Init(em);
}

/*
 * The ordinary end of a ground state: motionSpeed 1, EM_STANCE_GROUND,
 * lookAtTarget off, scriptPending, then DIAB_IDLE (EM_SYNC_LOCAL). Both
 * extra arguments are unused.
 */
void EmDiablos_EndState(Em *em, s32 unused1, s32 unused2)
{
    em->motionSpeed = 1.0f;
    em->stance = EM_STANCE_GROUND;
    em->lookAtTarget = 0;
    em->scriptPending = 1;
    EmDiablos_ChangeState(em, EM_GROUP_IDLE, DIAB_IDLE, EM_SYNC_LOCAL);
}

/*
 * Ends a state under the sand: motionSpeed 1, EM_STANCE_UNDER, lookAtTarget
 * off, scriptPending, then DIAB_BURROW_IDLE (EM_SYNC_LOCAL). Called at the
 * end of DIAB_BURROW_DIG and of several group-2 states.
 */
void EmDiablos_EndStateUnderground(Em *em)
{
    em->motionSpeed = 1.0f;
    em->stance = EM_STANCE_UNDER;
    em->lookAtTarget = 0;
    em->scriptPending = 1;
    EmDiablos_ChangeState(em, EM_GROUP_FLY, DIAB_BURROW_IDLE, EM_SYNC_LOCAL);
}

/*
 * Ends a state in the air after a leap out of the sand: scriptPending,
 * motionSpeed 1, EM_STANCE_AIR (air, not under the sand), lookAtTarget off.
 * With variant 0 (the only one handled) it goes to DIAB_BURROW_AIR_LOOP
 * through Em_SetStateRand, which draws a new stateRand and skips
 * EmDiablos_SetBurrowState. Callers are group-2 states (lines 1986, 2019,
 * 2088, 2147, 2233).
 */
void EmDiablos_EndStateInAir(Em *em, u8 variant)
{
    em->scriptPending = 1;
    em->motionSpeed = 1.0f;
    em->stance = EM_STANCE_AIR;
    em->lookAtTarget = 0;
    switch (variant) {
    case 0:
        Em_SetStateRand(em, EM_GROUP_FLY, DIAB_BURROW_AIR_LOOP);
        break;
    }
}

/*
 * When the given layer (0-2) has stopped, restarts DIAB_MOT_IDLE on it
 * (blend 10). The upper-body gesture states call it each frame for the
 * layers they leave idle.
 */
void EmDiablos_RestartLayerMotion(Em *em, s32 layer)
{
    if (em->motion[layer].playing == 0) {
        switch (layer) {
        case 0:
            Em_SetLayerMotion(em, EM_MOTION_ID(0, DIAB_MOT_IDLE), 10, 0, 0);
            break;
        case 1:
            Em_SetLayerMotion(em, EM_MOTION_ID(1, DIAB_MOT_IDLE), 10, 0, 1);
            break;
        case 2:
            Em_SetLayerMotion(em, EM_MOTION_ID(2, DIAB_MOT_IDLE), 10, 0, 2);
            break;
        }
    }
}

/*
 * Puts DIAB_MOT_IDLE on layers 0-2, then asks for the next script action
 * (scriptPending) when em->timer runs out. The stance is left as it was.
 */
void EmDiablos_IdleStand(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        if (em->motionId[0] != EM_MOTION_ID(0, DIAB_MOT_IDLE)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(0, DIAB_MOT_IDLE), 10, 0, 0);
        }
        if (em->motionId[1] != EM_MOTION_ID(0, 101)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(1, DIAB_MOT_IDLE), 10, 0, 1);
        }
        if (em->motionId[2] != EM_MOTION_ID(1, DIAB_MOT_IDLE)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(2, DIAB_MOT_IDLE), 10, 0, 2);
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
 * Stance 0 and DIAB_MOT_IDLE once on layer 0. When the motion ends (and the
 * monster is not remote), it calls EmDiablos_EndState, which enters this
 * state again with scriptPending set.
 */
void EmDiablos_Idle(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, DIAB_MOT_IDLE, 0, 0);
        break;
    case 1:
        if ((em->remote == 0) && (em->motion[0].playing == 0)) {
            em->step++;
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * The horn is stuck in a wall after a charge hit it (EmDiablos_ChargeHitWall
 * in DIAB_MOVE_RUN, DIAB_MOVE_RUN_AND_STOP and group-3 charges). Plays
 * DIAB_MOT_HORN_STUCK with an impact hit (EmImpactAttack kind 8) at
 * DIAB_BONE_BREATH offset (0, 270, 270), and a timed mine point
 * (EmMine_CreateTimed0) on the floor 320 ahead of bone 1. After 240 frames it
 * plays DIAB_MOT_HORN_PULL_FREE, then ends.
 */
void EmDiablos_HornStuck(Em *em, EmWork *work)
{
    f32 ofs[4];
    f32 hitPos[4];
    void *boneMtx;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, DIAB_MOT_HORN_STUCK, 0, 0);
        em->timer = 240;
        ofs[0] = 0.0f;
        ofs[1] = 270.0f;
        ofs[2] = 270.0f;
        boneMtx = Em_Bone(em, DIAB_BONE_BREATH);
        Vec_RotateSelf(ofs, boneMtx);
        Mtx_GetTrans(hitPos, boneMtx);
        hitPos[0] += ofs[0];
        hitPos[1] += ofs[1];
        hitPos[2] += ofs[2];
        EmImpactAttack_CreateAt(em, hitPos, 8);
        ofs[0] = 0.0f;
        ofs[1] = 0.0f;
        ofs[2] = 320.0f;
        boneMtx = Em_Bone(em, 1);
        Vec_RotateSelf(ofs, boneMtx);
        Mtx_GetTrans(hitPos, boneMtx);
        hitPos[0] += ofs[0];
        hitPos[1] = em->floorY;
        hitPos[2] += ofs[2];
        EmMine_CreateTimed0(em, hitPos);
        break;
    case 1:
        if (--em->timer == 0) {
            em->step++;
            Em_SetMotion(em, DIAB_MOT_HORN_PULL_FREE, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * Plays DIAB_MOT_ALERT_IDLE and ends after 10 frames, without waiting for
 * the motion. EmDiablos_SetBurrowState goes here instead of DIAB_BURROW_DIG
 * when the floor is not sand (surfaceKind 0) in the player's area.
 */
void EmDiablos_AlertBrief(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, DIAB_MOT_ALERT_IDLE, 0, 0);
        em->timer = 10;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * Plays one DIAB_MOT_IDLE loop on layer 0 and keeps layers 1 and 2 idle,
 * then ends.
 */
void EmDiablos_IdleOneLoop(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, DIAB_MOT_IDLE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
    EmDiablos_RestartLayerMotion(em, 1);
    EmDiablos_RestartLayerMotion(em, 2);
}

/*
 * Plays the layer-1 gesture DIAB_MOT_UPPER_A with layers 0 and 2 idle and a
 * sound on layer-1 frame 36, then ends when layer 1 stops.
 */
void EmDiablos_UpperGestureA(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, DIAB_MOT_UPPER_A, 0, 0);
        Em_SetLayerMotion(em, EM_MOTION_ID(0, DIAB_MOT_IDLE), 10, 0, 0);
        Em_SetLayerMotion(em, EM_MOTION_ID(2, DIAB_MOT_IDLE), 10, 0, 2);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
    EmDiablos_RestartLayerMotion(em, 0);
    EmDiablos_RestartLayerMotion(em, 2);
    Em_PlaySeAtBoneOnLayerFrame(em, 36, 47, DIAB_BONE_HEAD, 1);
}

/*
 * Plays the layer-1 gesture DIAB_MOT_UPPER_B with two layer-1 sounds, then
 * ends when layer 1 stops.
 */
void EmDiablos_UpperGestureB(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, DIAB_MOT_UPPER_B, 0, 0);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
    EmDiablos_RestartLayerMotion(em, 0);
    EmDiablos_RestartLayerMotion(em, 2);
    Em_PlaySeAtBoneOnLayerFrame(em, 62, 31, DIAB_BONE_HEAD, 1);
    Em_PlaySeAtBoneOnLayerFrame(em, 4, 23, DIAB_BONE_ROOT, 1);
}

/*
 * Plays the layer-1 gesture DIAB_MOT_UPPER_E with no sounds, then ends when
 * layer 1 stops.
 */
void EmDiablos_UpperGestureE(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, DIAB_MOT_UPPER_E, 0, 0);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
    EmDiablos_RestartLayerMotion(em, 0);
    EmDiablos_RestartLayerMotion(em, 2);
}

/*
 * Empty, so the state never ends by itself.
 */
void EmDiablos_IdleNopA(void)
{
}

/*
 * Plays DIAB_MOT_IDLE_PAIR_A (stateRand odd) or DIAB_MOT_IDLE_PAIR_B once,
 * then ends.
 */
void EmDiablos_IdlePairA(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        if (em->stateRand & 1) {
            Em_SetMotion(em, DIAB_MOT_IDLE_PAIR_A, 0, 0);
            break;
        }
        Em_SetMotion(em, DIAB_MOT_IDLE_PAIR_B, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * Plays DIAB_MOT_TURN_IN_PLACE with goalYaw = Math_AngleXZ to the goal.
 * Until frame 60 it snaps rotY to goalYaw once within 0x200 but never steps
 * rotY itself. Then it ends when layer 1 stops.
 */
void EmDiablos_IdleTurnToGoal(Em *em, EmWork *work)
{
    u16 goalYaw;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, DIAB_MOT_TURN_IN_PLACE, 0, 0);
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
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * Plays the layer-1 gesture DIAB_MOT_UPPER_C with sounds on layer-1 frames 2
 * and 110, then ends.
 */
void EmDiablos_UpperGestureC(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, DIAB_MOT_UPPER_C, 0, 0);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
    EmDiablos_RestartLayerMotion(em, 0);
    EmDiablos_RestartLayerMotion(em, 2);
    Em_PlaySeAtBoneOnLayerFrame(em, 2, 33, DIAB_BONE_HEAD, 1);
    Em_PlaySeAtBoneOnLayerFrame(em, 110, 31, DIAB_BONE_HEAD, 1);
}

/*
 * Empty.
 */
void EmDiablos_IdleNopB(void)
{
}

/*
 * Plays the layer-1 gesture DIAB_MOT_UPPER_D with sounds on layer-1 frames 2
 * and 74, then ends.
 */
void EmDiablos_UpperGestureD(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, DIAB_MOT_UPPER_D, 0, 0);
        break;
    case 1:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
    EmDiablos_RestartLayerMotion(em, 0);
    EmDiablos_RestartLayerMotion(em, 2);
    Em_PlaySeAtBoneOnLayerFrame(em, 2, 32, DIAB_BONE_HEAD, 1);
    Em_PlaySeAtBoneOnLayerFrame(em, 74, 31, DIAB_BONE_HEAD, 1);
}

/*
 * Empty.
 */
void EmDiablos_IdleNopC(void)
{
}

/*
 * Plays DIAB_MOT_IDLE_PAIR_A, blends to idle, plays DIAB_MOT_IDLE_PAIR_B,
 * blends to idle, then ends.
 */
void EmDiablos_IdlePairB(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, DIAB_MOT_IDLE_PAIR_A, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, DIAB_MOT_IDLE, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].blend == 0) {
            em->step++;
            Em_SetMotion(em, DIAB_MOT_IDLE_PAIR_B, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, DIAB_MOT_IDLE, 0, 0);
        }
        break;
    case 4:
        if (em->motion[0].blend == 0) {
            em->step++;
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * The same as EmDiablos_IdlePairB with the order swapped
 * (DIAB_MOT_IDLE_PAIR_B first).
 */
void EmDiablos_IdlePairC(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, DIAB_MOT_IDLE_PAIR_B, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, DIAB_MOT_IDLE, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].blend == 0) {
            em->step++;
            Em_SetMotion(em, DIAB_MOT_IDLE_PAIR_A, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, DIAB_MOT_IDLE, 0, 0);
        }
        break;
    case 4:
        if (em->motion[0].blend == 0) {
            em->step++;
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * Plays the combat idle DIAB_MOT_ALERT_IDLE once, then ends. It also serves
 * DIAB_IDLE_ALERT_RAGE (EmDiablos_SetIdleState: Em_StartRage) and
 * DIAB_IDLE_ALERT_PENDING_RAGE (Em_StartPendingRage).
 */
void EmDiablos_Alert(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, DIAB_MOT_ALERT_IDLE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * Rests while lying: hit react mode WAKE, blind, and the lying reaction set
 * (Em_LoadReactionSet 1). Plays DIAB_MOT_REST_LIE_DOWN, then loops
 * DIAB_MOT_REST for 1800 frames with +1 hp a frame. Then full hp,
 * Em_ClearLowHpNeed and DIAB_IDLE_REST_END. The setter calls Em_ResetNotice.
 */
void EmDiablos_Rest(Em *em, EmWork *work)
{
    s16 maxHp;

    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, DIAB_MOT_REST_LIE_DOWN, 0, 0);
        Em_LoadReactionSet(em, 1);
        em->timer = 1800;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, DIAB_MOT_REST, 0, 0);
        }
        break;
    case 2:
        maxHp = em->hpMax;
        if (em->hp < maxHp) {
            Em_AddHp(em, 1);
        } else {
            Em_AddHp(em, maxHp);
        }
        if (--em->timer <= 0) {
            em->step++;
            Em_AddHp(em, em->hpMax);
            Em_ClearLowHpNeed(em);
            EmDiablos_ChangeState(em, EM_GROUP_IDLE, DIAB_IDLE_REST_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmDiablos_ChangeState(em, EM_GROUP_IDLE, DIAB_IDLE_REST_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * Empty.
 */
void EmDiablos_IdleNopD(void)
{
}

/*
 * Under status 2 (the setter calls Em_BeginStatus2; EM_DMG_STATUS2 on the
 * ground): WAKE mode, lying reaction set, DIAB_MOT_COLLAPSE, then
 * DIAB_MOT_LIE until em->timer runs out (set by Em_SetStatus2Timer in the
 * damage handler). Then DIAB_IDLE_STATUS2_END.
 */
void EmDiablos_Status2Collapse(Em *em, EmWork *work)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, DIAB_MOT_COLLAPSE, 0, 0);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, DIAB_MOT_LIE, 0, 0);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            EmDiablos_ChangeState(em, EM_GROUP_IDLE, DIAB_IDLE_STATUS2_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmDiablos_ChangeState(em, EM_GROUP_IDLE, DIAB_IDLE_STATUS2_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * Status 2 (Em_BeginStatus2 in the setter) as a lie-down: plays
 * DIAB_MOT_LIE_DOWN, then DIAB_MOT_LIE for 600 frames with hp regen as in
 * the rest. Then full hp, Em_ClearLowHpNeed and DIAB_IDLE_STATUS2_END.
 */
void EmDiablos_Status2LieDown(Em *em, EmWork *work)
{
    s16 maxHp;

    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, DIAB_MOT_LIE_DOWN, 0, 0);
        em->timer = 600;
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, DIAB_MOT_LIE, 0, 0);
        }
        break;
    case 2:
        maxHp = em->hpMax;
        if (em->hp < maxHp) {
            Em_AddHp(em, 1);
        } else {
            Em_AddHp(em, maxHp);
        }
        if (--em->timer <= 0) {
            em->step++;
            Em_AddHp(em, em->hpMax);
            Em_ClearLowHpNeed(em);
            EmDiablos_ChangeState(em, EM_GROUP_IDLE, DIAB_IDLE_STATUS2_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmDiablos_ChangeState(em, EM_GROUP_IDLE, DIAB_IDLE_STATUS2_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * Plays DIAB_MOT_IDLE_PAIR_A once, then ends.
 */
void EmDiablos_IdlePairD(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, DIAB_MOT_IDLE_PAIR_A, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * After the rest: plays DIAB_MOT_REST_GET_UP with the standing reaction set
 * (0), blind while it plays, then DIAB_IDLE_SHAKE_OFF. The setter calls
 * Em_RefillNeed3.
 */
void EmDiablos_RestGetUp(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, DIAB_MOT_REST_GET_UP, 0, 0);
        Em_LoadReactionSet(em, 0);
        em->blind = 2;
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            EmDiablos_ChangeState(em, EM_GROUP_IDLE, DIAB_IDLE_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmDiablos_ChangeState(em, EM_GROUP_IDLE, DIAB_IDLE_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * Plays DIAB_MOT_GET_UP with the standing reaction set and blind while it
 * plays, then DIAB_IDLE_SHAKE_OFF. It serves DIAB_IDLE_STATUS2_END (setter:
 * Em_EndStatus2) and DIAB_IDLE_STATUS1_END (Em_EndStatus1).
 */
void EmDiablos_StatusGetUp(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, DIAB_MOT_GET_UP, 0, 0);
        Em_LoadReactionSet(em, 0);
        em->blind = 2;
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            EmDiablos_ChangeState(em, EM_GROUP_IDLE, DIAB_IDLE_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmDiablos_ChangeState(em, EM_GROUP_IDLE, DIAB_IDLE_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * Plays DIAB_MOT_SHAKE_OFF once, then ends. It serves DIAB_IDLE_SHAKE_OFF
 * and the setter's three shake-offs: STATUS4 (Em_EndStatus4), HOLD2
 * (holdTimer2 = 0) and STUN (Em_EndStatus5).
 */
void EmDiablos_ShakeOff(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, DIAB_MOT_SHAKE_OFF, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * Empty.
 */
void EmDiablos_IdleNopE(void)
{
}

/*
 * Under status 1 (setter: Em_BeginStatus1; EM_DMG_CAPTURED when not held, on
 * the ground): the same as EmDiablos_Status2Collapse, with the timer from
 * Em_SetStatus1Timer, then DIAB_IDLE_STATUS1_END.
 */
void EmDiablos_Status1Collapse(Em *em, EmWork *work)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, DIAB_MOT_COLLAPSE, 0, 0);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, DIAB_MOT_LIE, 0, 0);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            EmDiablos_ChangeState(em, EM_GROUP_IDLE, DIAB_IDLE_STATUS1_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmDiablos_ChangeState(em, EM_GROUP_IDLE, DIAB_IDLE_STATUS1_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * Downed by status 1 or 2 while under the sand or in a sand state: WAKE mode,
 * blind, lying reaction set. If the trap/downed motions
 * (DIAB_MOT_SAND_STRUGGLE to DIAB_MOT_DOWNED_GET_UP,
 * EmDiablos_InSandStuckMotion) already play, it goes straight to
 * DIAB_MOT_DOWNED. Otherwise it is EM_STANCE_UNDER while DIAB_MOT_POP_OUT
 * blends in and EM_STANCE_GROUND after, then DIAB_MOT_DOWNED. When em->timer
 * runs out it goes to DIAB_REACT_STATUS2_SAND_GET_UP. It makes lying puffs at
 * DIAB_BONE_BREATH every frame and a sound every 135 timer frames.
 */
void EmDiablos_StatusDownedInSand(Em *em, EmWork *work)
{
    f32 puffOfs[3];

    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        if (EmDiablos_InSandStuckMotion(em) == 1) {
            em->stance = EM_STANCE_GROUND;
            em->step++;
            Em_SetMotion(em, DIAB_MOT_DOWNED, 0, 0);
        } else {
            em->stance = EM_STANCE_UNDER;
            if (em->motionId[0] != EM_MOTION_ID(0, DIAB_MOT_POP_OUT)) {
                Em_SetMotion(em, DIAB_MOT_POP_OUT, 0, 0);
            }
        }
        em->lookAtTarget = 0;
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (em->motion[0].blend != 0) {
            em->stance = EM_STANCE_UNDER;
        } else {
            em->stance = EM_STANCE_GROUND;
        }
        if (em->motion[0].playing == 0) {
            em->stance = EM_STANCE_GROUND;
            em->step++;
            Em_SetMotion(em, DIAB_MOT_DOWNED, 0, 0);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            EmDiablos_ChangeState(em, EM_GROUP_REACT, DIAB_REACT_STATUS2_SAND_GET_UP, EM_SYNC_OWNER);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmDiablos_ChangeState(em, EM_GROUP_REACT, DIAB_REACT_STATUS2_SAND_GET_UP, EM_SYNC_OWNER);
        }
        break;
    }
    puffOfs[0] = 0.0f;
    puffOfs[1] = 10.0f;
    puffOfs[2] = 140.0f;
    Em_SpawnLyingPuffs(em, DIAB_BONE_BREATH, puffOfs, 1.6f);
    if (em->timer % 135 == 0) {
        Em_PlaySeAtBone(em, 87, DIAB_BONE_HEAD);
    }
}

/*
 * Walks with DIAB_MOT_WALK to the goal (Em_StepToGoal 64) or, with no
 * goal, until em->timer runs out, then ends. The setter turns it into
 * DIAB_MOVE_LIMP below 30% hp.
 */
void EmDiablos_Walk(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, DIAB_MOT_WALK, 0, 0);
        break;
    case 1:
        if (em->hasGoal != 0) {
            if (Em_StepToGoal(em, 64, 1) == 1) {
                em->step++;
                EmDiablos_EndState(em, 0, 0);
                break;
            }
        } else {
            if (--em->timer <= 0) {
                em->step++;
                EmDiablos_EndState(em, 0, 0);
            }
        }
        break;
    }
}

/*
 * Calls EmDiablos_EndState at once. The setter only sets scriptPending and
 * returns, so it never enters this state.
 */
void EmDiablos_MoveEnd(Em *em, EmWork *work)
{
    EmDiablos_EndState(em, 0, 0);
}

/*
 * Runs with DIAB_MOT_RUN to the goal or until the timer runs out, then ends.
 * With a goal, a wall hit (EmDiablos_ChargeHitWall: the horn, part 6,
 * unbroken) goes to DIAB_IDLE_HORN_STUCK. The setter makes it LIMP below 30%
 * hp and RUN_AND_STOP in combat.
 */
void EmDiablos_Run(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, DIAB_MOT_RUN, 0, 0);
        break;
    case 1:
        if (em->hasGoal != 0) {
            if (Em_StepToGoal(em, 64, 1) == 1) {
                em->step++;
                EmDiablos_EndState(em, 0, 0);
                break;
            }
            if (EmDiablos_ChargeHitWall(em) != 0) {
                EmDiablos_ChangeState(em, EM_GROUP_IDLE, DIAB_IDLE_HORN_STUCK, EM_SYNC_SEND);
                break;
            }
        } else {
            if (--em->timer <= 0) {
                em->step++;
                EmDiablos_EndState(em, 0, 0);
            }
        }
        break;
    }
}

/*
 * Turns toward the target with EmDiablos_TurnParams[0]
 * (Em_StartTurnToTarget/Em_TurnStep), clears EM_FLAG_NO_ROOT_MOTION, then
 * ends. The setter makes it DIAB_MOVE_TURN_IN_COMBAT in combat.
 */
void EmDiablos_Turn(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_StartTurnToTarget(em, &EmDiablos_TurnParams[0]);
        break;
    case 1:
        if (Em_TurnStep(em, &EmDiablos_TurnParams[0]) == 1) {
            em->step++;
            Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * Runs as in EmDiablos_Run (including the wall check), and on reaching the
 * goal or at the timeout plays DIAB_MOT_CHARGE_STOP, then ends. It serves
 * DIAB_MOVE_RUN_AND_STOP and DIAB_MOVE_RUN_AND_STOP_SLOW (the setter sets
 * motionSpeed 0.7). The setter sets speed with Em_SetSpeedScaled flag 2.
 */
void EmDiablos_RunAndStop(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, DIAB_MOT_RUN, 0, 0);
        break;
    case 1:
        if (em->hasGoal != 0) {
            if (Em_StepToGoal(em, 64, 1) == 1) {
                em->step++;
                Em_SetMotion(em, DIAB_MOT_CHARGE_STOP, 0, 0);
                break;
            }
            if (EmDiablos_ChargeHitWall(em) != 0) {
                EmDiablos_ChangeState(em, EM_GROUP_IDLE, DIAB_IDLE_HORN_STUCK, EM_SYNC_SEND);
                break;
            }
        } else {
            if (--em->timer <= 0) {
                em->step++;
                Em_SetMotion(em, DIAB_MOT_CHARGE_STOP, 0, 0);
            }
            break;
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * The same as EmDiablos_Turn with EmDiablos_TurnParams[1].
 */
void EmDiablos_TurnInCombat(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_StartTurnToTarget(em, &EmDiablos_TurnParams[1]);
        break;
    case 1:
        if (Em_TurnStep(em, &EmDiablos_TurnParams[1]) == 1) {
            em->step++;
            Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * Walks with DIAB_MOT_LIMP to the goal or until the timer runs out (capped
 * at 300 by the setter), then ends.
 */
void EmDiablos_Limp(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, DIAB_MOT_LIMP, 0, 0);
        break;
    case 1:
        if (em->hasGoal != 0) {
            if (Em_StepToGoal(em, 64, 1) == 1) {
                em->step++;
                EmDiablos_EndState(em, 0, 0);
                break;
            }
        } else {
            if (--em->timer <= 0) {
                em->step++;
                EmDiablos_EndState(em, 0, 0);
            }
        }
        break;
    }
}

/*
 * Plays DIAB_MOT_TURN_IN_PLACE and turns rotY toward goalYaw by 0x200 a
 * frame (snapping within 512) until frame 60. Then it ends when layer 1
 * stops.
 */
void EmDiablos_TurnToGoal(Em *em, EmWork *work)
{
    u16 goalYaw;
    u32 diff;
    s32 rotY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, DIAB_MOT_TURN_IN_PLACE, 0, 0);
        work->goalYaw = Math_AngleXZ(&em->posX, em->goal);
        break;
    case 1:
        rotY = em->rotY;
        goalYaw = work->goalYaw;
        diff = (goalYaw - (rotY & 0xFFFF)) & 0xFFFF;
        if ((u32) ((diff + 512) & 0xFFFF) < 1024U) {
            em->rotY = (s32) goalYaw;
        } else if (diff < 0x8000U) {
            em->rotY = (rotY + 0x200) & 0xFFFF;
        } else {
            em->rotY = (rotY - 0x200) & 0xFFFF;
        }
        if (EmMotion_ReachedFrame(em, 0, 60.0f) != 0) {
            em->step++;
            break;
        }
        break;
    case 2:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * Digs into the sand: DIAB_MOT_BURROW unless it already plays, noReact, and
 * guardTimer kept at 2. From frame 100 the shadow is hidden
 * (hideShadow/hideShadowHold 2). At the motion's end it clears noReact and
 * calls EmDiablos_EndStateUnderground (EM_STANCE_UNDER,
 * DIAB_BURROW_IDLE).
 */
void EmDiablos_Burrow(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        if (em->motionId[0] != EM_MOTION_ID(0, DIAB_MOT_BURROW)) {
            Em_SetMotion(em, DIAB_MOT_BURROW, 0, 0);
        }
        em->noReact = 1;
        em->stance = EM_STANCE_GROUND;
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 100.0f) != 0) {
            em->hideShadowHold = 2;
            em->hideShadow = 2;
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            em->noReact = 0;
            EmDiablos_EndStateUnderground(em);
        }
        break;
    }
    em->guardTimer = 2;
}

/*
 * Under the sand (EM_STANCE_UNDER): plays the idle motion once, then (owner
 * only) EmDiablos_EndStateUnderground, which re-enters this state.
 * EmDiablos_Start enters it; every burrow state ends here.
 */
void EmDiablos_BurrowIdle(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_UNDER;
        em->lookAtTarget = 0;
        Em_SetMotion(em, DIAB_MOT_IDLE, 0, 0);
        break;
    case 1:
        if ((em->remote == 0) && (em->motion[0].playing == 0)) {
            em->step++;
            EmDiablos_EndStateUnderground(em);
        }
        break;
    }
}

/*
 * Surfaces: plays DIAB_MOT_EMERGE unless it already runs; EM_STANCE_UNDER
 * while the motion blends in, then ground; shadow hidden before frame 100,
 * noReact and guardTimer 2 while it plays; then EmDiablos_EndState (ground
 * idle).
 */
void EmDiablos_Emerge(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        if (em->motionId[0] != EM_MOTION_ID(0, DIAB_MOT_EMERGE)) {
            Em_SetMotion(em, DIAB_MOT_EMERGE, 0, 0);
            em->stance = EM_STANCE_UNDER;
        } else if (em->motion[0].blend != 0) {
            em->stance = EM_STANCE_UNDER;
        } else {
            em->stance = EM_STANCE_GROUND;
        }
        em->noReact = 1;
        break;
    case 1:
        if (em->motion[0].blend != 0) {
            em->stance = EM_STANCE_UNDER;
        } else {
            em->stance = EM_STANCE_GROUND;
            if (EmMotion_ReachedFrame(em, 0, 100.0f) == 0) {
                em->hideShadowHold = 2;
                em->hideShadow = 2;
            }
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            em->noReact = 0;
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
    em->guardTimer = 2;
}

/*
 * Under the sand: turns to the target with EmDiablos_TurnParams[2], then
 * EmDiablos_EndStateUnderground.
 */
void EmDiablos_BurrowTurn(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_UNDER;
        em->lookAtTarget = 0;
        Em_StartTurnToTarget(em, &EmDiablos_TurnParams[2]);
        break;
    case 1:
        if (Em_TurnStep(em, &EmDiablos_TurnParams[2]) == 1) {
            em->step++;
            EmDiablos_EndStateUnderground(em);
        }
        break;
    }
}

/*
 * Under the sand: DIAB_MOT_RUN (sand trail), camera shake every 8 frames, pad
 * vibration for the local player within 2000 every 8 frames; steps to the
 * goal until frame 150, then subtracts the root motion from goalDist; ends at
 * the goal (or by timer with no goal) with EmDiablos_EndStateUnderground.
 */
void EmDiablos_BurrowTravelRumble(Em *em, EmWork *work)
{
    EmDiablosWork *diabWork = (EmDiablosWork *) work;
    Player *pl = &gPlayers[gScene.localPlayer];
    f32 rootDelta[3];

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_UNDER;
        em->lookAtTarget = 0;
        Em_SetMotion(em, DIAB_MOT_RUN, 0, 100);
        diabWork->quakeTimer = 2;
        break;
    case 1:
        if (--diabWork->quakeTimer == 0) {
            CamShake_ReqAIf(em, 5);
            diabWork->quakeTimer = 8;
        }
        if (em->hasGoal != 0) {
            if (EmMotion_ReachedFrame(em, 0, 150.0f) == 0) {
                Em_StepToGoal(em, 64, 1);
            } else {
                Em_MotionDelta(em, rootDelta);
                em->goalDist -= rootDelta[2];
            }
            if (em->goalDist <= 0.0f) {
                em->step++;
                EmDiablos_EndStateUnderground(em);
            }
        }
        if (!(gScene.frame & 7) && Obj_SameArea(em, pl) && Obj_InCurrentArea(pl) &&
            Vec_Distance((f32 *) &em->posX, (f32 *) pl->pos) < 2000.0f) {
            Vib_Start(0, 2);
        }
        if (em->hasGoal == 0) {
            if (--em->timer <= 0) {
                em->step++;
                EmDiablos_EndStateUnderground(em);
            }
        }
        break;
    }
}

/*
 * Under the sand: DIAB_MOT_WALK, Em_StepToGoal; reaching the goal sets timer
 * 1; ends on the timer with EmDiablos_EndStateUnderground.
 */
void EmDiablos_BurrowWalk(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_UNDER;
        em->lookAtTarget = 0;
        Em_SetMotion(em, DIAB_MOT_WALK, 0, 0);
        break;
    case 1:
        if (Em_StepToGoal(em, 64, 1) == 1) {
            em->timer = 1;
        }
        if (--em->timer <= 0) {
            em->step++;
            EmDiablos_EndStateUnderground(em);
        }
        break;
    }
}

/*
 * From the ground: DIAB_MOT_LEAP with velY curve kind 2; airborne
 * (EM_STANCE_AIR) from scaled frame 70; when the curves end,
 * EmDiablos_EndStateInAir (DIAB_BURROW_AIR_LOOP).
 */
void EmDiablos_Leap(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, DIAB_MOT_LEAP, 0, 0);
        EmDiablos_StartCurves(em, 2U);
        work->flyGuard = 0;
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 70.0f, 0U) != 0) {
            em->step++;
            em->stance = EM_STANCE_AIR;
            EmDiablos_UpdateCurves(em);
        }
        break;
    case 2:
        if (EmDiablos_UpdateCurves(em)) {
            em->step++;
            EmDiablos_EndStateInAir(em, 0);
        }
        break;
    }
}

/*
 * Airborne DIAB_MOT_AIR, velY curve 1, turns to the goal at turnSpeed 256
 * for up to 150 frames, then the air loop; posY kept at or above the floor.
 * Flyer HOVER_TURN copy.
 */
void EmDiablos_AirTurn(Em *em, EmWork *work)
{
    f32 floorY;
    s32 done;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, DIAB_MOT_AIR, 0, 0);
        work->turnSpeed = 256;
        work->goalYaw = Math_AngleXZ(&em->posX, em->goal);
        work->goalYaw = work->goalYaw - em->rotY;
        work->flyGuard = 0;
        em->timer = 150;
        break;
    case 1:
        EmDiablos_MoveByVelYCurve(em, 1);
        done = Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 1) & 0xFF;
        if (--em->timer <= 0 || done != 0) {
            em->step++;
            EmDiablos_EndStateInAir(em, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * Under the sand: DIAB_MOT_RUN with the 8-frame camera shake, steps to
 * the goal (no vibration, no root-motion phase); ends at the goal or on the
 * timer with EmDiablos_EndStateUnderground.
 */
void EmDiablos_BurrowTravel(Em *em, EmWork *work)
{
    EmDiablosWork *diabWork = (EmDiablosWork *) work;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_UNDER;
        em->lookAtTarget = 0;
        Em_SetMotion(em, DIAB_MOT_RUN, 0, 100);
        diabWork->quakeTimer = 2;
        break;
    case 1:
        if (--diabWork->quakeTimer == 0) {
            CamShake_ReqAIf(em, 5);
            diabWork->quakeTimer = 8;
        }
        if (em->hasGoal != 0) {
            if (Em_StepToGoal(em, 64, 1) == 1) {
                em->step++;
                EmDiablos_EndStateUnderground(em);
                break;
            }
        } else {
            if (--em->timer <= 0) {
                em->step++;
                EmDiablos_EndStateUnderground(em);
            }
        }
        break;
    }
}

/*
 * Airborne DIAB_MOT_AIR, velY 20, moving until the timer ends or posY passes
 * goal height + 1000, then the air loop. It aims at a local
 * position that is never set (the yaw comes from whatever the stack holds).
 */
void EmDiablos_AirRise(Em *em, EmWork *work)
{
    f32 aimPos[3];
    f32 floorY;
    s32 timer;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, DIAB_MOT_AIR, 0, 0);
        work->flyGuard = 0;
        work->moveAngX = 0;
        work->moveAngZ = 0;
        Player_ClearVelocity(em);
        em->velY = 20.0f;
        break;
    case 1:
        work->moveAngY = Math_AngleXZ(&em->posX, aimPos) & 0xFFFF;
        Em_MoveRotated(em, &work->moveAngX);
        timer = em->timer - 1;
        em->timer = timer;
        if ((timer <= 0) || !(em->posY <= (1000.0f + em->goal[1]))) {
            em->step++;
            Player_ClearVelocity(em);
            EmDiablos_EndStateInAir(em, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * Under the sand: idle motion, waits 90 frames, then
 * EmDiablos_EndStateUnderground.
 */
void EmDiablos_BurrowWait(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_UNDER;
        em->lookAtTarget = 0;
        Em_SetMotion(em, DIAB_MOT_IDLE, 0, 0);
        em->timer = 90;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmDiablos_EndStateUnderground(em);
        }
        break;
    }
}

/*
 * Airborne DIAB_MOT_AIR: turns and pitches to the goal, moves, velY curve 1,
 * until within 10*velZ of the goal (XZ) or the timer ends; clears velocity,
 * then the air loop. Flyer HOVER_TO_GOAL copy.
 */
void EmDiablos_AirToGoal(Em *em, EmWork *work)
{
    f32 dist;
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        work->turnSpeed = 256;
        Em_SetMotion(em, DIAB_MOT_AIR, 0, 0);
        work->flyGuard = 0;
        break;
    case 1:
        Em_TurnToGoal(em, (s32) (u16) work->turnSpeed, 1);
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_GetPitchToGoal(em, &work->moveAngX, 0.0f, 0.0f);
        Em_MoveRotated(em, &work->moveAngX);
        EmDiablos_MoveByVelYCurve(em, 1);
        dist = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer <= 0 || dist <= (10.0f * em->velZ)) {
            em->step++;
            Player_ClearVelocity(em);
            EmDiablos_EndStateInAir(em, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * Airborne DIAB_MOT_AIR sinking at velY -10; below floor + 630 at frames
 * 10/86/160 of the loop it plays DIAB_MOT_DESCEND for 30 frames, then
 * DIAB_MOT_LAND on the floor (stance ground, camera shake 1), then
 * EmDiablos_EndState. EmDiablos_Reappear drops the monster in through it.
 */
void EmDiablos_AirLand(Em *em, EmWork *work)
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
        Em_SetMotion(em, DIAB_MOT_AIR, 0, 0);
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
            Em_SetMotion(em, DIAB_MOT_DESCEND, 0, 0);
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
            Em_SetMotion(em, DIAB_MOT_LAND, 0, 0);
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
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * Airborne DIAB_MOT_AIR once, then the air loop again
 * (EmDiablos_EndStateInAir targets this state).
 */
void EmDiablos_AirLoop(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, DIAB_MOT_AIR, 0, 0);
        work->flyGuard = 0;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmDiablos_EndStateInAir(em, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * Leaps out after the sand struggles (DIAB_REACT_POP_OUT,
 * DIAB_REACT_SAND_STRUGGLE, DIAB_REACT_STATUS4_SAND_STRUGGLE): DIAB_MOT_LEAP
 * with curve kind 2, stance air, noReact; below floor + 800 after the curves,
 * DIAB_MOT_DESCEND with velY to reach the floor in 30 frames; on the floor
 * DIAB_MOT_LAND, camera shake 2; then landTimer 150 (frames since landing)
 * and EmDiablos_EndState.
 */
void EmDiablos_SandEscape(Em *em, EmWork *work)
{
    f32 floorY;
    f32 velY;
    f32 y;

    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, DIAB_MOT_LEAP, 10, 96);
        EmDiablos_StartCurves(em, 2U);
        em->stance = EM_STANCE_AIR;
        em->noReact = 1;
        break;
    case 1:
        EmDiablos_UpdateCurves(em);
        if (em->motion[0].playing == 0) {
            em->step++;
        }
        break;
    case 2:
        if ((EmDiablos_UpdateCurves(em) & 0xFF) && (em->posY <= (800.0f + em->floorY))) {
            em->step++;
            Em_SetMotion(em, DIAB_MOT_DESCEND, 0, 0);
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
            Em_SetMotion(em, DIAB_MOT_LAND, 0, 0);
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            CamShake_ReqAIf(em, 2);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            ((EmDiablosWork *) work)->landTimer = 150;
            em->step++;
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
    y = em->floorY;
    if (em->posY < y) {
        em->posY = y;
    }
}

/*
 * Turns to the target within 20 degrees (Em_TurnToTargetMotion with
 * WALK/TURN_LEFT/TURN_RIGHT), then DIAB_MOT_STARE once, then
 * EmDiablos_EndState.
 */
void EmDiablos_TurnAndStare(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        if (Em_TurnToTargetMotion(em, 3640, DIAB_MOT_WALK, DIAB_MOT_TURN_LEFT, DIAB_MOT_TURN_RIGHT) != 0) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->lookAtTarget = 0;
            Em_SetMotion(em, DIAB_MOT_STARE, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * DIAB_MOT_STARE once with lookAtTarget set, then EmDiablos_EndAttack.
 */
void EmDiablos_Stare(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 1;
        Em_SetMotion(em, DIAB_MOT_STARE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->lookAtTarget = 0;
            em->step++;
            EmDiablos_EndAttack(em, work);
        }
        break;
    }
}

/*
 * DIAB_MOT_TAIL_SPIN twice, rotY + 0x200 a frame, then EmDiablos_EndAttack.
 * It sets the motion on layer 0 but waits on layer 1's playing flag.
 */
void EmDiablos_TailSpin(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, DIAB_MOT_TAIL_SPIN, 0, 0);
        Motion_InitTime(em, 0);
        break;
    case 1:
        em->rotY += 0x200;
        if (em->motion[1].playing == 0) {
            em->step++;
            Em_SetMotion(em, DIAB_MOT_TAIL_SPIN, 0, 0);
        }
        break;
    case 2:
        em->rotY += 0x200;
        if (em->motion[1].playing == 0) {
            em->step++;
            EmDiablos_EndAttack(em, work);
        }
        break;
    }
}

/*
 * DIAB_MOT_RUN to the goal (EmDiablos_SetAttackState puts the goal 500
 * past the target, or 150 frames without a target); a wall hit
 * (EmDiablos_ChargeHitWall) goes to DIAB_IDLE_HORN_STUCK; at the goal
 * DIAB_MOT_CHARGE_STOP, then EmDiablos_EndState. Same code as
 * EmDiablos_RunAndStop.
 */
void EmDiablos_Charge(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, DIAB_MOT_RUN, 0, 0);
        break;
    case 1:
        if (em->hasGoal != 0) {
            if (Em_StepToGoal(em, 64, 1) == 1) {
                Em_SetMotion(em, DIAB_MOT_CHARGE_STOP, 0, 0);
                em->step++;
                break;
            }
            if (EmDiablos_ChargeHitWall(em) != 0) {
                EmDiablos_ChangeState(em, EM_GROUP_IDLE, DIAB_IDLE_HORN_STUCK, EM_SYNC_SEND);
                break;
            }
        } else {
            if (--em->timer <= 0) {
                Em_SetMotion(em, DIAB_MOT_CHARGE_STOP, 0, 0);
                em->step++;
            }
            break;
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * As EmDiablos_Charge (timer dist/30 + 30) but ends with DIAB_MOT_ATTACK_A.
 */
void EmDiablos_ChargeAttackA(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, DIAB_MOT_RUN, 0, 0);
        break;
    case 1:
        if (em->hasGoal != 0) {
            if (Em_StepToGoal(em, 64, 1) == 1) {
                Em_SetMotion(em, DIAB_MOT_ATTACK_A, 0, 0);
                em->step++;
                break;
            }
            if (EmDiablos_ChargeHitWall(em) != 0) {
                EmDiablos_ChangeState(em, EM_GROUP_IDLE, DIAB_IDLE_HORN_STUCK, EM_SYNC_SEND);
                break;
            }
        } else {
            if (--em->timer <= 0) {
                Em_SetMotion(em, DIAB_MOT_ATTACK_A, 0, 0);
                em->step++;
            }
            break;
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * As EmDiablos_Charge (timer dist/30 + 30) but ends with DIAB_MOT_ATTACK_B.
 */
void EmDiablos_ChargeAttackB(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, DIAB_MOT_RUN, 0, 0);
        break;
    case 1:
        if (em->hasGoal != 0) {
            if (Em_StepToGoal(em, 64, 1) == 1) {
                Em_SetMotion(em, DIAB_MOT_ATTACK_B, 0, 0);
                em->step++;
                break;
            }
            if (EmDiablos_ChargeHitWall(em) != 0) {
                EmDiablos_ChangeState(em, EM_GROUP_IDLE, DIAB_IDLE_HORN_STUCK, EM_SYNC_SEND);
                break;
            }
        } else {
            if (--em->timer <= 0) {
                Em_SetMotion(em, DIAB_MOT_ATTACK_B, 0, 0);
                em->step++;
            }
            break;
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * Turns to the target (20 degrees), then DIAB_MOT_ATTACK_A, then
 * EmDiablos_EndState.
 */
void EmDiablos_TurnAttackA(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        if (Em_TurnToTargetMotion(em, 3640, DIAB_MOT_WALK, DIAB_MOT_TURN_LEFT, DIAB_MOT_TURN_RIGHT) != 0) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->lookAtTarget = 0;
            Em_SetMotion(em, DIAB_MOT_ATTACK_A, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * Turns to the target, then DIAB_MOT_ATTACK_B, then EmDiablos_EndState.
 */
void EmDiablos_TurnAttackB(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        if (Em_TurnToTargetMotion(em, 3640, DIAB_MOT_WALK, DIAB_MOT_TURN_LEFT, DIAB_MOT_TURN_RIGHT) != 0) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->lookAtTarget = 0;
            Em_SetMotion(em, DIAB_MOT_ATTACK_B, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * DIAB_MOT_ATTACK_C without turning, then EmDiablos_EndState; serves
 * DIAB_ATK_ATTACK_C and DIAB_ATK_ATTACK_C_B.
 */
void EmDiablos_AttackC(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, DIAB_MOT_ATTACK_C, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * Turns to the target, then DIAB_MOT_ATTACK_D, then EmDiablos_EndState.
 */
void EmDiablos_TurnAttackD(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        if (Em_TurnToTargetMotion(em, 3640, DIAB_MOT_WALK, DIAB_MOT_TURN_LEFT, DIAB_MOT_TURN_RIGHT) != 0) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->lookAtTarget = 0;
            Em_SetMotion(em, DIAB_MOT_ATTACK_D, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * DIAB_MOT_ROAR once, then EmDiablos_EndState.
 */
void EmDiablos_Roar(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, DIAB_MOT_ROAR, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * Em_RestartScript, DIAB_MOT_FLINCH once, then EmDiablos_EndState (parts
 * 0/7, EM_DMG_HIT_MODE3).
 */
void EmDiablos_Flinch(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, DIAB_MOT_FLINCH, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * DIAB_MOT_FLINCH_B once (unbroken part 3/4 and the default parts), then
 * EmDiablos_EndState.
 */
void EmDiablos_FlinchB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, DIAB_MOT_FLINCH_B, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * DIAB_MOT_FLINCH_C once (part 5, or part 6 not yet broken), then
 * EmDiablos_EndState.
 */
void EmDiablos_FlinchC(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, DIAB_MOT_FLINCH_C, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * DIAB_MOT_FLINCH_D once (parts 1/2), then EmDiablos_EndState.
 */
void EmDiablos_FlinchD(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, DIAB_MOT_FLINCH_D, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * EM_DMG_PART8_DOWN (EmDiablos_ChangeState: fxFlags |= 1, partBreakCount++,
 * all statuses end); noHitTimer 5; DIAB_MOT_PART_BREAK hides mesh 1 from
 * frame 6, turns rotY back by 936*motionSpeed a frame in frames 52-122; at
 * the end rotY -= 8 and DIAB_REACT_PART_BREAK_END (owner).
 */
void EmDiablos_PartBreak(Em *em)
{
    f32 turnStepF;
    u32 turnStep;
    u8 kind;

    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, DIAB_MOT_PART_BREAK, 0, 0);
        break;
    case 1:
        if ((EmMotion_PassedFrameScaled(em, 6.0f, 0U) != 0) && ((s8) em->meshShown[1] != 0)) {
            kind = em->type;
            if ((kind != 20) && (kind != 39)) {
                em->meshShown[1] = 0;
            }
        }
        if ((EmMotion_ReachedFrame(em, 0, 52.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 122.0f) == 0)) {
            turnStepF = 936.0f * em->motionSpeed;
            turnStep = (u32) turnStepF;
            em->rotY -= (s32) turnStep & 0xFFFF;
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            em->rotY -= 8;
            EmDiablos_ChangeState(em, EM_GROUP_REACT, DIAB_REACT_PART_BREAK_END, EM_SYNC_OWNER);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmDiablos_ChangeState(em, EM_GROUP_REACT, DIAB_REACT_PART_BREAK_END, EM_SYNC_OWNER);
        }
        break;
    }
}

/*
 * Shared knock-down/stun: even variants play KNOCK_DOWN, DOWN, DOWN_GET_UP,
 * odd ones KNOCK_DOWN_B, DOWN_B, DOWN_GET_UP_B; variants 0/1 lie 120 frames
 * and end with EmDiablos_EndState; 2/3 (stun) add stun stars, lie for the
 * status-5 timer and go to DIAB_IDLE_STUN_SHAKE_OFF (shake-off), keeping
 * prevStateGroup/prevState.
 */
void EmDiablos_KnockDown(Em *em, EmWork *work, u8 variant)
{
    s32 k;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        k = variant;
        if ((k & 1) == 0) {
            Em_SetMotion(em, DIAB_MOT_KNOCK_DOWN, 0, 0);
        } else {
            Em_SetMotion(em, DIAB_MOT_KNOCK_DOWN_B, 0, 0);
        }
        switch (k) {
        case 2:
        case 3:
            StunStars_CreateEm(em, 1, 34, (f32) em->timer + em->motion[0].frameCount / 2.0f, 150.0f, 3.0f);
            break;
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            k = variant;
            em->step++;
            if ((k & 1) == 0) {
                Em_SetMotion(em, DIAB_MOT_DOWN, 0, 0);
            } else {
                Em_SetMotion(em, DIAB_MOT_DOWN_B, 0, 0);
            }
            if (k != 3 && k != 2) {
                em->timer = 120;
            }
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            if ((variant & 1) == 0) {
                Em_SetMotion(em, DIAB_MOT_DOWN_GET_UP, 0, 0);
            } else {
                Em_SetMotion(em, DIAB_MOT_DOWN_GET_UP_B, 0, 0);
            }
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
            switch (variant) {
            case 0:
            default:
            case 1:
                EmDiablos_EndState(em, 0, 0);
                break;
            case 2:
            case 3:
                em->step++;
                EmDiablos_ChangeState(em, EM_GROUP_IDLE, DIAB_IDLE_STUN_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
                break;
            }
        }
        break;
    case 4:
        if (em->remote == 0) {
            EmDiablos_ChangeState(em, EM_GROUP_IDLE, DIAB_IDLE_STUN_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * Attack kind 5 (or a part flinch) under the sand while not raging:
 * DIAB_MOT_POP_OUT (EM_STANCE_UNDER while blending), then
 * DIAB_MOT_SAND_STRUGGLE looped for trapTimer 208 with Em_SetTrapTimeGoal,
 * noReact; then DIAB_BURROW_SAND_ESCAPE (owner).
 */
void EmDiablos_PopOutOfSand(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_UNDER;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, DIAB_MOT_POP_OUT, 0, 0);
        em->noReact = 1;
        break;
    case 1:
        if (em->motion[0].blend != 0) {
            em->stance = EM_STANCE_UNDER;
        } else {
            em->stance = EM_STANCE_GROUND;
        }
        if (em->motion[0].playing == 0) {
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, DIAB_MOT_SAND_STRUGGLE, 0, 0);
            em->trapTimer = 208;
            Em_SetTrapTimeGoal(em);
            em->step++;
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            Em_SetMotion(em, DIAB_MOT_SAND_STRUGGLE, 0, 0);
        }
        if (--em->trapTimer <= 0) {
            em->step++;
            em->noReact = 0;
            EmDiablos_ChangeState(em, EM_GROUP_FLY, DIAB_BURROW_SAND_ESCAPE, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmDiablos_ChangeState(em, EM_GROUP_FLY, DIAB_BURROW_SAND_ESCAPE, EM_SYNC_OWNER_FLAGGED);
        }
        if (em->motion[0].playing == 0) {
            Em_SetMotion(em, DIAB_MOT_SAND_STRUGGLE, 0, 0);
        }
        break;
    }
}

/*
 * DIAB_MOT_PART6_BREAK once, then EmDiablos_EndState. Update picks it for a
 * part-6 flinch when the part-6 state is 1 or 2 (Diablos, Black Diablos) or
 * 2 (Monoblos kinds); part 6 at 2 also stops EmDiablos_ChargeHitWall (likely
 * the horns).
 */
void EmDiablos_Part6Break(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, DIAB_MOT_PART6_BREAK, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * EM_DMG_STATUS_IN_AIR or a flinch in stance air: DIAB_MOT_FALL with velY at
 * most -50 and accelY -10, DIAB_MOT_FALL_LAND on the floor,
 * DIAB_MOT_DOWN_GET_UP, then EmDiablos_EndState.
 */
void EmDiablos_FallFromAir(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, DIAB_MOT_FALL, 0, 0);
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
            Em_SetMotion(em, DIAB_MOT_FALL_LAND, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, DIAB_MOT_DOWN_GET_UP, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmDiablos_EndState(em, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    }
}

/*
 * As EmDiablos_FallFromAir with blind 2 every frame; no caller in this file.
 */
void EmDiablos_FallFromAirBlind(Em *em, EmWork *work)
{
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, DIAB_MOT_FALL, 0, 0);
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
            Em_SetMotion(em, DIAB_MOT_FALL_LAND, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, DIAB_MOT_DOWN_GET_UP, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmDiablos_EndState(em, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    }
}

/*
 * DIAB_MOT_FLINCH_E once, then EmDiablos_EndState; no caller in this file.
 */
void EmDiablos_FlinchE(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, DIAB_MOT_FLINCH_E, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * EM_DMG_STATUS4 on the ground (EmDiablos_ChangeState: Em_BeginStatus4):
 * DIAB_MOT_STATUS4 until the timer, Em_SpawnStatus4Effect each frame, then
 * DIAB_IDLE_STATUS4_SHAKE_OFF (shake-off).
 */
void EmDiablos_Status4(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, DIAB_MOT_STATUS4, 0, 0);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmDiablos_ChangeState(em, EM_GROUP_IDLE, DIAB_IDLE_STATUS4_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmDiablos_ChangeState(em, EM_GROUP_IDLE, DIAB_IDLE_STATUS4_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
    Em_SpawnStatus4Effect(em, 2);
}

/*
 * holdTimer 5 each frame; DIAB_MOT_SAND_STRUGGLE looped, noReact, until
 * trapTimer ends; clears trapHit and noReact; then DIAB_BURROW_SAND_ESCAPE.
 * Follows DIAB_REACT_STATUS2_SAND_GET_UP.
 */
void EmDiablos_SandStruggle(Em *em)
{
    em->holdTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, DIAB_MOT_SAND_STRUGGLE, 0, 0);
        Em_RestartScript(em);
        em->noReact = 1;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            Em_SetMotion(em, DIAB_MOT_SAND_STRUGGLE, 0, 0);
        }
        if (--em->trapTimer <= 0) {
            em->step++;
            em->trapHit = 0;
            em->noReact = 0;
            EmDiablos_ChangeState(em, EM_GROUP_FLY, DIAB_BURROW_SAND_ESCAPE, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmDiablos_ChangeState(em, EM_GROUP_FLY, DIAB_BURROW_SAND_ESCAPE, EM_SYNC_OWNER_FLAGGED);
        }
        if (em->motion[0].playing == 0) {
            Em_SetMotion(em, DIAB_MOT_SAND_STRUGGLE, 0, 0);
        }
        break;
    }
}

/*
 * Downed half out of the sand: if a struggle motion
 * (EmDiablos_InSandStuckMotion) already runs, DIAB_MOT_DOWNED at once, else
 * DIAB_MOT_POP_OUT (EM_STANCE_UNDER while blending) first; lies for the
 * timer, then Em_SetTrapTimeGoal and the matching struggle. variant 0 (status
 * 4: hit-react mode STATUS, status-4 effect each frame, then
 * DIAB_REACT_STATUS4_SAND_STRUGGLE); variant 1 (stun: stun stars, then
 * DIAB_REACT_STUN_SAND_STRUGGLE).
 */
void EmDiablos_HitDownedInSand(Em *em, EmWork *work, u8 variant)
{
    s32 k = variant;

    if (k == 0) {
        Em_SetHitReactMode(em, EM_HIT_REACT_STATUS);
    }
    switch (em->step) {
    case 0:
        em->step++;
        if (EmDiablos_InSandStuckMotion(em) == 1) {
            em->stance = EM_STANCE_GROUND;
            em->step++;
            Em_SetMotion(em, DIAB_MOT_DOWNED, 0, 0);
            if (k == 1) {
                StunStars_CreateEm(em, 1, 34, (f32) em->timer + em->motion[0].frameCount / 2.0f, 150.0f, 3.0f);
            }
        } else {
            em->stance = EM_STANCE_UNDER;
            if (em->motionId[0] != EM_MOTION_ID(0, DIAB_MOT_POP_OUT)) {
                Em_SetMotion(em, DIAB_MOT_POP_OUT, 0, 0);
            }
        }
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        em->noReact = 1;
        break;
    case 1:
        if (em->motion[0].blend != 0) {
            em->stance = EM_STANCE_UNDER;
        } else {
            em->stance = EM_STANCE_GROUND;
        }
        if (em->motion[0].playing == 0) {
            em->stance = EM_STANCE_GROUND;
            em->step++;
            Em_SetMotion(em, DIAB_MOT_DOWNED, 0, 0);
            if (k == 1) {
                StunStars_CreateEm(em, 1, 34, (f32) em->timer + em->motion[0].frameCount / 2.0f, 150.0f, 3.0f);
            }
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetTrapTimeGoal(em);
            switch (k) {
            default:
                EmDiablos_ChangeState(em, EM_GROUP_REACT, DIAB_REACT_STATUS4_SAND_STRUGGLE, EM_SYNC_OWNER);
                break;
            case 1:
                EmDiablos_ChangeState(em, EM_GROUP_REACT, DIAB_REACT_STUN_SAND_STRUGGLE, EM_SYNC_OWNER);
                break;
            }
        }
        break;
    case 3:
        if (em->remote == 0) {
            switch (k) {
            case 0:
            default:
                EmDiablos_ChangeState(em, EM_GROUP_REACT, DIAB_REACT_STATUS4_SAND_STRUGGLE, EM_SYNC_OWNER);
                break;
            case 1:
                EmDiablos_ChangeState(em, EM_GROUP_REACT, DIAB_REACT_STUN_SAND_STRUGGLE, EM_SYNC_OWNER);
                break;
            }
        }
        break;
    }
    switch (k) {
    default:
        Em_SpawnStatus4Effect(em, 2);
        break;
    case 1:
        break;
    }
}

/*
 * EmDiablos_ChangeState resets the base pose and breaks part 2 on entry; sets
 * no motion, waits for the current one to stop, then EmDiablos_EndState.
 */
void EmDiablos_PartBreakEnd(Em *em)
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
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * DIAB_MOT_SAND_STRUGGLE looped with noReact until trapTimer ends, then
 * DIAB_BURROW_SAND_ESCAPE: DIAB_REACT_STATUS4_SAND_STRUGGLE and
 * DIAB_REACT_STUN_SAND_STRUGGLE (EmDiablos_ChangeState ends status 4 or 5
 * on entry).
 */
void EmDiablos_SandDownedStruggle(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, DIAB_MOT_SAND_STRUGGLE, 0, 0);
        em->noReact = 1;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            Em_SetMotion(em, DIAB_MOT_SAND_STRUGGLE, 0, 0);
        }
        if (--em->trapTimer <= 0) {
            em->step++;
            EmDiablos_ChangeState(em, EM_GROUP_FLY, DIAB_BURROW_SAND_ESCAPE, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmDiablos_ChangeState(em, EM_GROUP_FLY, DIAB_BURROW_SAND_ESCAPE, EM_SYNC_OWNER_FLAGGED);
        }
        if (em->motion[0].playing == 0) {
            Em_SetMotion(em, DIAB_MOT_SAND_STRUGGLE, 0, 0);
        }
        break;
    }
}

/*
 * EmDiablos_ChangeState ends status 2; DIAB_MOT_DOWNED_GET_UP with blind 2,
 * then Em_SetTrapTimeGoal and DIAB_REACT_SAND_STRUGGLE. Entered from
 * DIAB_IDLE_SAND_DOWNED (status 1/2 in the sand) by its timer or
 * EM_DMG_HIT_LYING.
 */
void EmDiablos_Status2SandGetUp(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, DIAB_MOT_DOWNED_GET_UP, 0, 0);
        em->blind = 2;
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            Em_SetTrapTimeGoal(em);
            EmDiablos_ChangeState(em, EM_GROUP_REACT, DIAB_REACT_SAND_STRUGGLE, EM_SYNC_OWNER);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmDiablos_ChangeState(em, EM_GROUP_REACT, DIAB_REACT_SAND_STRUGGLE, EM_SYNC_OWNER);
        }
        break;
    }
}

/*
 * EM_DMG_ATTACK13 (EmDiablos_ChangeState ends all statuses): Em_SetHold2 each
 * frame; DIAB_MOT_FLINCH without root motion to frame 52, then
 * DIAB_MOT_STATUS4 until the timer, then DIAB_IDLE_HOLD2_SHAKE_OFF
 * (shake-off).
 */
void EmDiablos_Hold2Flinch(Em *em)
{
    Em_SetHold2(em);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, DIAB_MOT_FLINCH, 0, 0);
        Player_SetFlag((Player *) em, EM_FLAG_NO_ROOT_MOTION);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 52.0f) != 0) {
            em->step++;
            Em_SetMotion(em, DIAB_MOT_STATUS4, 12, 0);
            Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            EmDiablos_ChangeState(em, EM_GROUP_IDLE, DIAB_IDLE_HOLD2_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmDiablos_ChangeState(em, EM_GROUP_IDLE, DIAB_IDLE_HOLD2_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * The Diablos story scene (Update: Diablos kinds, quest stage
 * QUEST_ID_DIABLOS_SCENE, AREA_DESERT_2, EVENT_FLAG_DIABLOS_SCENE unset):
 * unhittable; placed at a fixed spot, idles 300 frames, DIAB_MOT_ALERT_IDLE,
 * DIAB_MOT_ROAR, DIAB_MOT_BURROW, then under the sand travels (DIAB_MOT_RUN)
 * to a fixed goal (300 frames or within 200), idles until
 * EVENT_FLAG_DIABLOS_SCENE is set, then EmDiablos_EndStateUnderground.
 */
void EmDiablos_DiablosDesertScene(Em *em, EmWork *work)
{
    f32 goalDist;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        GmEm_ResetSize(em);
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->posX = 6660.0f;
        em->posY = 50.0f;
        em->posZ = 5737.0f;
        em->rotX = 0;
        em->rotY = 0x2400;
        em->rotZ = 0;
        Em_SetMotion(em, DIAB_MOT_IDLE, 0, 0);
        em->timer = 300;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, DIAB_MOT_ALERT_IDLE, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, DIAB_MOT_ROAR, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, DIAB_MOT_BURROW, 0, 0);
        }
        break;
    case 4:
        if (EmMotion_ReachedFrame(em, 0, 100.0f) != 0) {
            em->hideShadowHold = 2;
            em->hideShadow = 2;
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stance = EM_STANCE_UNDER;
            em->goal[0] = 11000.0f;
            em->goal[1] = 50.0f;
            em->goal[2] = 8813.0f;
            em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
            em->timer = 300;
            Em_SetMotion(em, DIAB_MOT_RUN, 0, 100);
        }
        break;
    case 5:
        goalDist = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer <= 0 || goalDist <= 200.0f) {
            em->step++;
            Em_SetMotion(em, DIAB_MOT_IDLE, 0, 0);
        }
        break;
    case 6:
        if (Save_TestEventFlag(EVENT_FLAG_DIABLOS_SCENE) == 1) {
            em->step++;
            EmDiablos_EndStateUnderground(em);
        }
        break;
    }
}

/*
 * The Monoblos story scene (Monoblos kinds, quest stage
 * QUEST_ID_MONOBLOS_SCENE, EVENT_FLAG_MONOBLOS_SCENE): starts under the sand
 * at a fixed spot, travels to a goal, DIAB_MOT_EMERGE, DIAB_MOT_ROAR, is
 * moved to a second spot and charges on the ground to another goal, idles
 * until EVENT_FLAG_MONOBLOS_SCENE, then EmDiablos_EndState.
 */
void EmDiablos_MonoblosDesertScene(Em *em, EmWork *work)
{
    f32 goalDist;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        GmEm_ResetSize(em);
        em->step++;
        em->stance = EM_STANCE_UNDER;
        em->posX = 8492.0f;
        em->posY = 50.0f;
        em->posZ = 5247.0f;
        em->goal[0] = 6040.0f;
        em->goal[1] = 50.0f;
        em->goal[2] = 8236.0f;
        em->rotX = 0;
        em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        em->rotZ = 0;
        Em_SetMotion(em, DIAB_MOT_IDLE, 0, 0);
        em->timer = 500;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stance = EM_STANCE_UNDER;
            em->goal[0] = 5237.0f;
            em->goal[1] = 50.0f;
            em->goal[2] = 10800.0f;
            em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
            em->timer = 300;
            Em_SetMotion(em, DIAB_MOT_RUN, 0, 100);
        }
        break;
    case 2:
        goalDist = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer <= 0 || goalDist <= 200.0f) {
            em->step++;
            Em_SetMotion(em, DIAB_MOT_EMERGE, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].blend != 0) {
            em->stance = EM_STANCE_UNDER;
        } else {
            em->stance = EM_STANCE_GROUND;
            if (EmMotion_ReachedFrame(em, 0, 100.0f) == 0) {
                em->hideShadowHold = 2;
                em->hideShadow = 2;
            }
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, DIAB_MOT_ROAR, 0, 0);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->posX = 7100.0f;
            em->posY = 50.0f;
            em->posZ = 13545.0f;
            em->goal[0] = 10000.0f;
            em->goal[1] = 50.0f;
            em->goal[2] = 9100.0f;
            em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
            Em_SetMotion(em, DIAB_MOT_RUN, 0, 0);
        }
        break;
    case 5:
        goalDist = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer <= 0 || goalDist <= 200.0f) {
            em->step++;
            Em_SetMotion(em, DIAB_MOT_IDLE, 0, 0);
        }
        break;
    case 6:
        if (Save_TestEventFlag(EVENT_FLAG_MONOBLOS_SCENE) == 1) {
            em->step++;
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * Captured (EM_DMG_CAPTURED with holdTimer2): Em_SetCaptured each frame;
 * QuestEm_Captured, Em_BeginStatus1, Em_ClearPoison, reaction set 1;
 * DIAB_MOT_COLLAPSE then DIAB_MOT_LIE; counts 2400 frames while
 * QuestEm_CanAppear, fades alpha over 150 frames, then DIAB_DEFEAT_REAPPEAR
 * (reappear, owner).
 */
void EmDiablos_Captured(Em *em, EmWork *work)
{
    s32 timer;
    s32 prevTimer;

    Em_SetCaptured(em, 1);
    switch (em->step) {
    case 0:
        em->step++;
        QuestEm_Captured(em);
        em->timer = 2400;
        Em_BeginStatus1(em);
        Em_ClearPoison(em);
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, DIAB_MOT_COLLAPSE, 0, 0);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, DIAB_MOT_LIE, 0, 0);
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
        timer = em->timer;
        if (timer <= 0) {
            if (em->remote == 0) {
                EmDiablos_ChangeState(em, EM_GROUP_DEFEAT, DIAB_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
                break;
            }
        } else {
            em->alpha = (f32) timer / 150.0f;
        }
        break;
    }
}

/*
 * Versus end: once Versus_DefeatCamera is 1, VersusBait_Create for the
 * player of its side at EmDiablos_CarveThrowPt, DIAB_MOT_STARE, then
 * DIAB_MOT_ALERT_IDLE and after 30 frames Versus_RouteEmByTeam every frame;
 * unhittable throughout.
 */
void EmDiablos_VsBait(Em *em, EmWork *work)
{
    Vec3f baitPos;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->stance = EM_STANCE_GROUND;
        if (Versus_DefeatCamera(em) == 1) {
            baitPos = EmDiablos_CarveThrowPt;
            VersusBait_Create(&gPlayers[(u8) em->side - 1], em, 0, 0, &baitPos.x);
            em->step++;
            Em_SetMotion(em, DIAB_MOT_STARE, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 30;
            Em_SetMotion(em, DIAB_MOT_ALERT_IDLE, 0, 0);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            Versus_RouteEmByTeam(em);
        }
        break;
    }
}

/*
 * DIAB_MOT_FLINCH_E once, idle motion, then DIAB_VS_WAIT_BOTH.
 */
void EmDiablos_VsPoseA(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, DIAB_MOT_FLINCH_E, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, DIAB_MOT_IDLE, 0, 0);
            EmDiablos_ChangeState(em, EM_GROUP_VERSUS, DIAB_VS_WAIT_BOTH, EM_SYNC_SEND);
        }
        break;
    }
}

/*
 * DIAB_MOT_TURN_IN_PLACE once, idle motion, then DIAB_VS_WAIT_BOTH.
 */
void EmDiablos_VsPoseB(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, DIAB_MOT_TURN_IN_PLACE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, DIAB_MOT_IDLE, 0, 0);
            EmDiablos_ChangeState(em, EM_GROUP_VERSUS, DIAB_VS_WAIT_BOTH, EM_SYNC_SEND);
        }
        break;
    }
}

/*
 * DIAB_MOT_ROAR once, idle motion, then DIAB_VS_WAIT_BOTH.
 */
void EmDiablos_VsRoar(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, DIAB_MOT_ROAR, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, DIAB_MOT_IDLE, 0, 0);
            EmDiablos_ChangeState(em, EM_GROUP_VERSUS, DIAB_VS_WAIT_BOTH, EM_SYNC_SEND);
        }
        break;
    }
}

/*
 * EmDiablos_ChangeState calls Versus_MarkDefeat on entry; idle motion; waits
 * Versus_BothDefeated, then DIAB_VS_END.
 */
void EmDiablos_VsWaitBoth(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        if (em->motionId[0] != EM_MOTION_ID(0, DIAB_MOT_IDLE)) {
            Em_SetMotion(em, DIAB_MOT_IDLE, 0, 0);
        }
        em->stance = EM_STANCE_GROUND;
        
    case 1:
        if (Versus_BothDefeated() == 1) {
            em->step++;
            EmDiablos_ChangeState(em, EM_GROUP_VERSUS, DIAB_VS_END, EM_SYNC_SEND);
        }
        break;
    }
}

/*
 * Idle 90 frames, clears senseDelay and noHitTimer, then EmDiablos_EndState.
 */
void EmDiablos_VsEnd(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->timer = 90;
        if (em->motionId[0] != EM_MOTION_ID(0, DIAB_MOT_IDLE)) {
            Em_SetMotion(em, DIAB_MOT_IDLE, 0, 0);
        }
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            em->senseDelay = 0;
            em->noHitTimer = 0;
            EmDiablos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * Dies on the ground: EM_STANCE_DEAD (dead), DIAB_MOT_DIE with
 * QuestEm_Hunted and a camera shake at frame 212, then the carve point
 * (EmMine_Create) for 1800 frames once the quest lets it go, a 150-frame
 * fade-out (alpha) and, on the owner, DIAB_DEFEAT_REAPPEAR (sync mode 4).
 * Its step 99 (back to idle) is never set in this file.
 */
void EmDiablos_Die(Em *em, EmWork *work)
{
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, DIAB_MOT_DIE, 0, 0);
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
                EmDiablos_ChangeState(em, EM_GROUP_DEFEAT, DIAB_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    case 99:
        if (em->motion[0].playing == 0) {
            em->stance = EM_STANCE_GROUND;
            EmDiablos_EndState(em, 0, 0);
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
        }
        break;
    }
}

/*
 * Dies under the sand: if it already lies or struggles out of the sand
 * (EmDiablos_InSandStuckMotion) it plays DIAB_MOT_DOWNED at once, else it is
 * pushed out (DIAB_MOT_POP_OUT, EM_STANCE_UNDER while blending, unless
 * already playing it) and then lies dead in DIAB_MOT_DOWNED; QuestEm_Hunted,
 * the carve point for 1800 frames, the fade-out and DIAB_DEFEAT_REAPPEAR as
 * EmDiablos_Die.
 */
void EmDiablos_DieInSand(Em *em, EmWork *work)
{
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        if (EmDiablos_InSandStuckMotion(em) == 1) {
            QuestEm_Hunted(em);
            em->stance = EM_STANCE_DEAD;
            em->step++;
            Em_SetMotion(em, DIAB_MOT_DOWNED, 0, 0);
        } else {
            em->stance = EM_STANCE_UNDER;
            if (em->motionId[0] != EM_MOTION_ID(0, DIAB_MOT_POP_OUT)) {
                Em_SetMotion(em, DIAB_MOT_POP_OUT, 0, 0);
            }
        }
        em->lookAtTarget = 0;
        break;
    case 1:
        if (em->motion[0].blend != 0) {
            em->stance = EM_STANCE_UNDER;
        } else {
            em->stance = EM_STANCE_DEAD;
        }
        if (em->motion[0].playing == 0) {
            em->stance = EM_STANCE_DEAD;
            QuestEm_Hunted(em);
            em->step++;
            Em_SetMotion(em, DIAB_MOT_DOWNED, 0, 0);
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
    case 3:
        EmMine_Update(em);
        if (QuestEm_CanAppear(em) != 0) {
            if (--em->timer <= 0) {
                em->step++;
                em->timer = 150;
                EmMine_Remove(em);
            }
        }
        break;
    case 4:
        if (em->timer > 0) {
            em->timer--;
        }
        if (em->timer <= 0) {
            if (em->remote == 0) {
                EmDiablos_ChangeState(em, EM_GROUP_DEFEAT, DIAB_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    }
}

/*
 * Dies in the air (EM_STANCE_AIR, a leap out of the sand): falls
 * (DIAB_MOT_FALL, velY at most -50, accelY -10, Em_MoveRotatedAccel) to the
 * floor, lands (DIAB_MOT_FALL_LAND, EM_STANCE_GROUND, velocity cleared), and
 * at frame 60 lies dead (DIAB_MOT_DIE_FALLING, EM_STANCE_DEAD,
 * QuestEm_Hunted); then the carve point, fade-out and DIAB_DEFEAT_REAPPEAR
 * as EmDiablos_Die.
 */
void EmDiablos_DieFalling(Em *em, EmWork *work)
{
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_SetMotion(em, DIAB_MOT_FALL, 0, 0);
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
            Em_SetMotion(em, DIAB_MOT_FALL_LAND, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    case 2:
        if (EmMotion_PassedFrameScaled(em, 60.0f, 0) != 0) {
            em->step++;
            em->timer = 0;
            Em_SetMotion(em, DIAB_MOT_DIE_FALLING, 6, 0);
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
                em->timer = 150;
                EmMine_Remove(em);
            }
        }
        break;
    case 5:
        if (em->timer > 0) {
            em->timer--;
        }
        if (em->timer <= 0) {
            if (em->remote == 0) {
                EmDiablos_ChangeState(em, EM_GROUP_DEFEAT, DIAB_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    }
}

/*
 * When the quest lets the monster come back (QuestEm_CanAppear 1;
 * EmDiablos_ChangeState already called QuestEm_Appear) it is put at the
 * area's ground height (gAreaGroundY) in the air (EM_STANCE_AIR) and goes to
 * DIAB_BURROW_AIR_LAND; otherwise it ends its task (Em.taskPhase 1 -> 2,
 * finish).
 */
void EmDiablos_Reappear(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        if (QuestEm_CanAppear(em) == 1) {
            em->posY = gAreaGroundY[em->area];
            em->stance = EM_STANCE_AIR;
            EmDiablos_ChangeState(em, EM_GROUP_FLY, DIAB_BURROW_AIR_LAND, EM_SYNC_LOCAL);
            break;
        }
        em->taskPhase++;
    case 1:
        break;
    }
}

/*
 * Group 0's dispatcher (EM_GROUP_IDLE). Shares: DIAB_IDLE_STATUS1_END runs
 * EmDiablos_StatusGetUp's function (as DIAB_IDLE_STATUS2_END),
 * DIAB_IDLE_STATUS4_SHAKE_OFF, _HOLD2_SHAKE_OFF and _STUN_SHAKE_OFF run
 * EmDiablos_ShakeOff's (the shake-offs after statuses 4, hold 2 and 5 by
 * EmDiablos_SetIdleState's entry actions), DIAB_IDLE_ALERT_RAGE and
 * _ALERT_PENDING_RAGE run EmDiablos_Alert's (entered with Em_StartRage or
 * Em_StartPendingRage); 30-32 have no case.
 */
void EmDiablos_RunIdleState(Em *em, EmWork *work)
{
    switch (em->state) {
    case DIAB_IDLE_STAND:
        EmDiablos_IdleStand(em, work);
        break;
    case DIAB_IDLE:
        EmDiablos_Idle(em, work);
        break;
    case DIAB_IDLE_HORN_STUCK:
        EmDiablos_HornStuck(em, work);
        break;
    case DIAB_IDLE_ALERT_BRIEF:
        EmDiablos_AlertBrief(em, work);
        break;
    case DIAB_IDLE_ONE_LOOP:
        EmDiablos_IdleOneLoop(em, work);
        break;
    case DIAB_IDLE_UPPER_A:
        EmDiablos_UpperGestureA(em, work);
        break;
    case DIAB_IDLE_UPPER_B:
        EmDiablos_UpperGestureB(em, work);
        break;
    case DIAB_IDLE_UPPER_E:
        EmDiablos_UpperGestureE(em, work);
        break;
    case DIAB_IDLE_NOP_A:
        EmDiablos_IdleNopA();
        break;
    case DIAB_IDLE_PAIR_A:
        EmDiablos_IdlePairA(em, work);
        break;
    case DIAB_IDLE_TURN_TO_GOAL:
        EmDiablos_IdleTurnToGoal(em, work);
        break;
    case DIAB_IDLE_UPPER_C:
        EmDiablos_UpperGestureC(em, work);
        break;
    case DIAB_IDLE_NOP_B:
        EmDiablos_IdleNopB();
        break;
    case DIAB_IDLE_UPPER_D:
        EmDiablos_UpperGestureD(em, work);
        break;
    case DIAB_IDLE_NOP_C:
        EmDiablos_IdleNopC();
        break;
    case DIAB_IDLE_PAIR_B:
        EmDiablos_IdlePairB(em, work);
        break;
    case DIAB_IDLE_PAIR_C:
        EmDiablos_IdlePairC(em, work);
        break;
    case DIAB_IDLE_ALERT:
        EmDiablos_Alert(em, work);
        break;
    case DIAB_IDLE_REST:
        EmDiablos_Rest(em, work);
        break;
    case DIAB_IDLE_NOP_D:
        EmDiablos_IdleNopD();
        break;
    case DIAB_IDLE_STATUS2_COLLAPSE:
        EmDiablos_Status2Collapse(em, work);
        break;
    case DIAB_IDLE_STATUS2_LIE_DOWN:
        EmDiablos_Status2LieDown(em, work);
        break;
    case DIAB_IDLE_PAIR_D:
        EmDiablos_IdlePairD(em, work);
        break;
    case DIAB_IDLE_REST_END:
        EmDiablos_RestGetUp(em, work);
        break;
    case DIAB_IDLE_STATUS2_END:
        EmDiablos_StatusGetUp(em, work);
        break;
    case DIAB_IDLE_SHAKE_OFF:
        EmDiablos_ShakeOff(em, work);
        break;
    case DIAB_IDLE_NOP_E:
        EmDiablos_IdleNopE();
        break;
    case DIAB_IDLE_STATUS1_COLLAPSE:
        EmDiablos_Status1Collapse(em, work);
        break;
    case DIAB_IDLE_STATUS1_END:
        EmDiablos_StatusGetUp(em, work);
        break;
    case DIAB_IDLE_SAND_DOWNED:
        EmDiablos_StatusDownedInSand(em, work);
        break;
    case DIAB_IDLE_STATUS4_SHAKE_OFF:
        EmDiablos_ShakeOff(em, work);
        break;
    case DIAB_IDLE_HOLD2_SHAKE_OFF:
        EmDiablos_ShakeOff(em, work);
        break;
    case DIAB_IDLE_STUN_SHAKE_OFF:
        EmDiablos_ShakeOff(em, work);
        break;
    case DIAB_IDLE_ALERT_RAGE:
        EmDiablos_Alert(em, work);
        break;
    case DIAB_IDLE_ALERT_PENDING_RAGE:
        EmDiablos_Alert(em, work);
        
    default:
        break;
    }
}

/*
 * Group 1's dispatcher (EM_GROUP_MOVE); DIAB_MOVE_RUN_AND_STOP_SLOW runs
 * DIAB_MOVE_RUN_AND_STOP's function (the run, at motion speed 0.7 from
 * EmDiablos_SetMoveState).
 */
void EmDiablos_RunMoveState(Em *em, EmWork *work)
{
    switch (em->state) {
    case DIAB_MOVE_WALK:
        EmDiablos_Walk(em, work);
        break;
    case DIAB_MOVE_END:
        EmDiablos_MoveEnd(em, work);
        break;
    case DIAB_MOVE_RUN:
        EmDiablos_Run(em, work);
        break;
    case DIAB_MOVE_TURN:
        EmDiablos_Turn(em, work);
        break;
    case DIAB_MOVE_RUN_AND_STOP:
        EmDiablos_RunAndStop(em, work);
        break;
    case DIAB_MOVE_TURN_IN_COMBAT:
        EmDiablos_TurnInCombat(em, work);
        break;
    case DIAB_MOVE_LIMP:
        EmDiablos_Limp(em, work);
        break;
    case DIAB_MOVE_TURN_TO_GOAL:
        EmDiablos_TurnToGoal(em, work);
        break;
    case DIAB_MOVE_RUN_AND_STOP_SLOW:
        EmDiablos_RunAndStop(em, work);
        
    default:
        break;
    }
}

/*
 * Group 2's dispatcher (EM_GROUP_FLY's slot: under the sand,
 * EM_STANCE_UNDER); no case for 13, 15-22.
 */
void EmDiablos_RunBurrowState(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmDiablos_Burrow(em, work);
        break;
    case 1:
        EmDiablos_BurrowIdle(em, work);
        break;
    case 2:
        EmDiablos_Emerge(em, work);
        break;
    case 3:
        EmDiablos_BurrowTurn(em, work);
        break;
    case 4:
        EmDiablos_BurrowTravelRumble(em, work);
        break;
    case 5:
        EmDiablos_BurrowWalk(em, work);
        break;
    case 6:
        EmDiablos_Leap(em, work);
        break;
    case 7:
        EmDiablos_AirTurn(em, work);
        break;
    case 8:
        EmDiablos_BurrowTravel(em, work);
        break;
    case 9:
        EmDiablos_AirRise(em, work);
        break;
    case 10:
        EmDiablos_BurrowWait(em, work);
        break;
    case 11:
        EmDiablos_AirToGoal(em, work);
        break;
    case 12:
        EmDiablos_AirLand(em, work);
        break;
    case 14:
        EmDiablos_AirLoop(em, work);
        break;
    case 23:
        EmDiablos_SandEscape(em, work);
        break;
    }
}

/*
 * Group 3's dispatcher (EM_GROUP_ATTACK); DIAB_ATK_ATTACK_C_B runs
 * DIAB_ATK_ATTACK_C's function; only the named states (0, 3, 6, 15-17, 26-31)
 * have cases.
 */
void EmDiablos_RunAttackState(Em *em, EmWork *work)
{
    switch (em->state) {
    case DIAB_ATK_TURN_AND_STARE:
        EmDiablos_TurnAndStare(em, work);
        break;
    case DIAB_ATK_STARE:
        EmDiablos_Stare(em, work);
        break;
    case DIAB_ATK_TAIL_SPIN:
        EmDiablos_TailSpin(em, work);
        break;
    case DIAB_ATK_CHARGE:
        EmDiablos_Charge(em, work);
        break;
    case DIAB_ATK_CHARGE_ATTACK_A:
        EmDiablos_ChargeAttackA(em, work);
        break;
    case DIAB_ATK_CHARGE_ATTACK_B:
        EmDiablos_ChargeAttackB(em, work);
        break;
    case DIAB_ATK_TURN_ATTACK_A:
        EmDiablos_TurnAttackA(em, work);
        break;
    case DIAB_ATK_TURN_ATTACK_B:
        EmDiablos_TurnAttackB(em, work);
        break;
    case DIAB_ATK_ATTACK_C:
        EmDiablos_AttackC(em, work);
        break;
    case DIAB_ATK_TURN_ATTACK_D:
        EmDiablos_TurnAttackD(em, work);
        break;
    case DIAB_ATK_ROAR:
        EmDiablos_Roar(em, work);
        break;
    case DIAB_ATK_ATTACK_C_B:
        EmDiablos_AttackC(em, work);
        break;
    }
}

/*
 * Group 4's dispatcher (EM_GROUP_REACT): EmDiablos_KnockDown serves
 * DIAB_REACT_KNOCK_DOWN, DIAB_REACT_KNOCK_DOWN_PART3, DIAB_REACT_STUN,
 * DIAB_REACT_STUN_B (variants 0-3), EmDiablos_HitDownedInSand serves
 * DIAB_REACT_STATUS4_IN_SAND and DIAB_REACT_STUN_IN_SAND (variants 0, 1),
 * EmDiablos_SandDownedStruggle serves DIAB_REACT_STATUS4_SAND_STRUGGLE and
 * DIAB_REACT_STUN_SAND_STRUGGLE; no case for 12.
 */
void EmDiablos_RunReactState(Em *em, EmWork *work)
{
    switch (em->state) {
    case DIAB_REACT_FLINCH:
        EmDiablos_Flinch(em);
        break;
    case DIAB_REACT_FLINCH_B:
        EmDiablos_FlinchB(em);
        break;
    case DIAB_REACT_FLINCH_C:
        EmDiablos_FlinchC(em);
        break;
    case DIAB_REACT_FLINCH_D:
        EmDiablos_FlinchD(em);
        break;
    case DIAB_REACT_PART_BREAK:
        EmDiablos_PartBreak(em);
        break;
    case DIAB_REACT_KNOCK_DOWN:
        EmDiablos_KnockDown(em, work, 0);
        break;
    case DIAB_REACT_POP_OUT:
        EmDiablos_PopOutOfSand(em);
        break;
    case DIAB_REACT_PART6_BREAK:
        EmDiablos_Part6Break(em);
        break;
    case DIAB_REACT_FALL:
        EmDiablos_FallFromAir(em, work);
        break;
    case DIAB_REACT_FALL_B:
        EmDiablos_FallFromAirBlind(em, work);
        break;
    case DIAB_REACT_FLINCH_E:
        EmDiablos_FlinchE(em);
        break;
    case DIAB_REACT_STATUS4:
        EmDiablos_Status4(em);
        break;
    case DIAB_REACT_SAND_STRUGGLE:
        EmDiablos_SandStruggle(em);
        break;
    case DIAB_REACT_STATUS4_IN_SAND:
        EmDiablos_HitDownedInSand(em, work, 0);
        break;
    case DIAB_REACT_PART_BREAK_END:
        EmDiablos_PartBreakEnd(em);
        break;
    case DIAB_REACT_KNOCK_DOWN_PART3:
        EmDiablos_KnockDown(em, work, 1);
        break;
    case DIAB_REACT_STATUS4_SAND_STRUGGLE:
        EmDiablos_SandDownedStruggle(em);
        break;
    case DIAB_REACT_STATUS2_SAND_GET_UP:
        EmDiablos_Status2SandGetUp(em);
        break;
    case DIAB_REACT_HOLD2:
        EmDiablos_Hold2Flinch(em);
        break;
    case DIAB_REACT_STUN:
        EmDiablos_KnockDown(em, work, 2);
        break;
    case DIAB_REACT_STUN_B:
        EmDiablos_KnockDown(em, work, 3);
        break;
    case DIAB_REACT_STUN_IN_SAND:
        EmDiablos_HitDownedInSand(em, work, 1);
        break;
    case DIAB_REACT_STUN_SAND_STRUGGLE:
        EmDiablos_SandDownedStruggle(em);
        break;
    }
}

/*
 * Group 5's dispatcher (EM_GROUP_DEFEAT): keeps noPushTimer at 5 and runs
 * DIE, DIE_IN_SAND, DIE_FALLING, REAPPEAR.
 */
void EmDiablos_RunDefeatState(Em *em, EmWork *work)
{
    em->noPushTimer = 5;
    switch (em->state) {
    case DIAB_DEFEAT_DIE:
        EmDiablos_Die(em, work);
        break;
    case DIAB_DEFEAT_DIE_IN_SAND:
        EmDiablos_DieInSand(em, work);
        break;
    case DIAB_DEFEAT_DIE_FALLING:
        EmDiablos_DieFalling(em, work);
        break;
    case DIAB_DEFEAT_REAPPEAR:
        EmDiablos_Reappear(em, work);
        break;
    }
}

/*
 * Groups 6 and 7's dispatcher (EM_GROUP_EVENT): 0 Diablos's and 1 Monoblos's
 * first-meeting scenes (Update starts them), 2 captured.
 */
void EmDiablos_RunEventState(Em *em, EmWork *work)
{
    switch (em->state) {
    case DIAB_EVENT_DIABLOS_SCENE:
        EmDiablos_DiablosDesertScene(em, work);
        break;
    case DIAB_EVENT_MONOBLOS_SCENE:
        EmDiablos_MonoblosDesertScene(em, work);
        break;
    case DIAB_EVENT_CAPTURED:
        EmDiablos_Captured(em, work);
        break;
    }
}

/*
 * Group 8's dispatcher (EM_GROUP_VERSUS): DIAB_VS_BAIT, DIAB_VS_POSE_A,
 * DIAB_VS_POSE_B, DIAB_VS_ROAR, DIAB_VS_WAIT_BOTH, DIAB_VS_END.
 */
void EmDiablos_RunVersusState(Em *em, EmWork *work)
{
    switch (em->state) {
    case DIAB_VS_BAIT:
        EmDiablos_VsBait(em, work);
        break;
    case DIAB_VS_POSE_A:
        EmDiablos_VsPoseA(em, work);
        break;
    case DIAB_VS_POSE_B:
        EmDiablos_VsPoseB(em, work);
        break;
    case DIAB_VS_ROAR:
        EmDiablos_VsRoar(em, work);
        break;
    case DIAB_VS_WAIT_BOTH:
        EmDiablos_VsWaitBoth(em, work);
        break;
    case DIAB_VS_END:
        EmDiablos_VsEnd(em, work);
        break;
    }
}

/*
 * The family's per-frame update (the monster task's per-kind dispatch): fades
 * the rage tint (rageFade up to 60 while enraged, down to 0 otherwise), holds
 * the position while no hunter is near (Em_HoldPosNoPlayers, another argument
 * under the sand), ends the state when it is buried where the floor is no
 * longer sand, counts the landing timer down, turns Em_UpdateDamage's result
 * into a reaction state, starts the Diablos (quest QUEST_ID_DIABLOS_SCENE) or
 * Monoblos (quest QUEST_ID_MONOBLOS_SCENE) first-meeting scene in the desert,
 * runs the state (twice on a state request) and keeps posY/floorY on the sand
 * surface: under it at most surfaceY - surfaceDepth with floorY 1000 below,
 * standing or dead on it. Outside the scene's conditions both quests' cases
 * go on to the default (the goto), as the game's code does.
 */
void EmDiablos_Update(Em *em)
{
    u8 dmgInfo[4];
    EmDiablosWork *w = (EmDiablosWork *)&em->work;

    if (em->rage != 0) {
        switch (em->rageStep) {
        case 0:
            if (++w->rageFade > 60) {
                w->rageFade = 60;
                em->rageStep++;
            }
            break;
        case 1:
            w->rageFade = 60;
            break;
        }
    } else {
        em->rageStep = 0;
        if (--w->rageFade < 0) {
            w->rageFade = 0;
        }
    }
    Em_UpdateCombatTimer(em);
    Em_RaisePlayerAreaTriggerLax(em);
    if (em->stance == EM_STANCE_UNDER) {
        Em_HoldPosNoPlayers(em, 0, 3);
    } else {
        Em_HoldPosNoPlayers(em, 0, 1);
    }
    if (EmDiablos_IsBuriedOffSand(em) == 1) {
        EmDiablos_EndState(em, 0, 0);
    }
    if (w->landTimer != 0) {
        w->landTimer -= 1;
    }
    switch (Em_UpdateDamage(em, dmgInfo) & 0xFF) {
    case EM_DMG_NONE:
    case EM_DMG_ATTACK9:
    case EM_DMG_ATTACK6:
    case EM_DMG_FLASH:
    case EM_DMG_HIT:
        break;
    case EM_DMG_KILLED:
    case EM_DMG_DEAD:
        if (em->stance == EM_STANCE_AIR) {
            EmDiablos_ChangeState(em, EM_GROUP_DEFEAT, DIAB_DEFEAT_DIE_FALLING, EM_SYNC_OWNER);
        } else if (em->stance == EM_STANCE_UNDER || EmDiablos_InSandReactState(em) == 1) {
            EmDiablos_ChangeState(em, EM_GROUP_DEFEAT, DIAB_DEFEAT_DIE_IN_SAND, EM_SYNC_OWNER);
        } else {
            EmDiablos_ChangeState(em, EM_GROUP_DEFEAT, DIAB_DEFEAT_DIE, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_STATUS5:
        if (em->stateGroup != EM_GROUP_REACT || em->state != DIAB_REACT_STUN_IN_SAND) {
            if (em->stance == EM_STANCE_UNDER || EmDiablos_InSandReactState(em) == 1) {
                Em_SetStatus5Timer(em);
                EmDiablos_ChangeState(em, EM_GROUP_REACT, DIAB_REACT_STUN_IN_SAND, EM_SYNC_OWNER);
            } else if ((em->stateGroup != EM_GROUP_REACT || em->state != DIAB_REACT_STUN) && (em->stateGroup != EM_GROUP_REACT || em->state != DIAB_REACT_STUN_B) &&
                       (em->stateGroup != EM_GROUP_REACT || em->state != DIAB_REACT_FALL)) {
                Em_SetStatus5Timer(em);
                if (((em->hitAngle - em->rotY) & 0xFFFF) > 0x8000) {
                    EmDiablos_ChangeState(em, EM_GROUP_REACT, DIAB_REACT_STUN, EM_SYNC_OWNER);
                } else {
                    EmDiablos_ChangeState(em, EM_GROUP_REACT, DIAB_REACT_STUN_B, EM_SYNC_OWNER);
                }
            }
        }
        break;
    case EM_DMG_ATTACK13:
        if (EmDiablos_InSandReactState(em) == 0) {
            Em_ResetKindTimer(em);
            EmDiablos_ChangeState(em, EM_GROUP_REACT, DIAB_REACT_HOLD2, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_ATTACK5:
        if (em->rage == 0 && em->stance == EM_STANCE_UNDER && (em->stateGroup != EM_GROUP_REACT || em->state != DIAB_REACT_POP_OUT)) {
            EmDiablos_ChangeState(em, EM_GROUP_REACT, DIAB_REACT_POP_OUT, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_STATUS_IN_AIR:
        if (em->stateGroup != EM_GROUP_REACT || em->state != DIAB_REACT_FALL) {
            EmDiablos_ChangeState(em, EM_GROUP_REACT, DIAB_REACT_FALL, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_STATUS4:
        if (em->stateGroup != EM_GROUP_REACT || em->state != DIAB_REACT_STATUS4_IN_SAND) {
            if (em->stance == EM_STANCE_UNDER || EmDiablos_InSandReactState(em) == 1) {
                Em_SetStatus4Timer(em);
                EmDiablos_ChangeState(em, EM_GROUP_REACT, DIAB_REACT_STATUS4_IN_SAND, EM_SYNC_OWNER);
            } else if ((em->stateGroup != EM_GROUP_REACT || em->state != DIAB_REACT_STATUS4) && (em->stateGroup != EM_GROUP_REACT || em->state != DIAB_REACT_FALL)) {
                Em_SetStatus4Timer(em);
                EmDiablos_ChangeState(em, EM_GROUP_REACT, DIAB_REACT_STATUS4, EM_SYNC_OWNER);
            }
        }
        break;
    case EM_DMG_CAPTURED:
        if ((s8) em->holdTimer2 != 0) {
            EmDiablos_ChangeState(em, EM_GROUP_EVENT, DIAB_EVENT_CAPTURED, EM_SYNC_OWNER);
        } else if (em->stateGroup != EM_GROUP_IDLE || em->state != DIAB_IDLE_SAND_DOWNED) {
            if (em->stance == EM_STANCE_UNDER || EmDiablos_InSandReactState(em) == 1) {
                Em_SetStatus1Timer(em);
                EmDiablos_ChangeState(em, EM_GROUP_IDLE, DIAB_IDLE_SAND_DOWNED, EM_SYNC_OWNER);
            } else if ((em->stateGroup != EM_GROUP_IDLE || em->state != DIAB_IDLE_STATUS1_COLLAPSE) && (em->stateGroup != EM_GROUP_REACT || em->state != DIAB_REACT_FALL)) {
                Em_SetStatus1Timer(em);
                EmDiablos_ChangeState(em, EM_GROUP_IDLE, DIAB_IDLE_STATUS1_COLLAPSE, EM_SYNC_OWNER);
            }
        }
        break;
    case EM_DMG_STATUS2:
        if (em->stateGroup != EM_GROUP_IDLE || em->state != DIAB_IDLE_SAND_DOWNED) {
            if (em->stance == EM_STANCE_UNDER || EmDiablos_InSandReactState(em) == 1) {
                Em_SetStatus2Timer(em);
                EmDiablos_ChangeState(em, EM_GROUP_IDLE, DIAB_IDLE_SAND_DOWNED, EM_SYNC_OWNER);
            } else if ((em->stateGroup != EM_GROUP_IDLE || em->state != DIAB_IDLE_STATUS2_COLLAPSE) && (em->stateGroup != EM_GROUP_REACT || em->state != DIAB_REACT_FALL)) {
                Em_SetStatus2Timer(em);
                EmDiablos_ChangeState(em, EM_GROUP_IDLE, DIAB_IDLE_STATUS2_COLLAPSE, EM_SYNC_OWNER);
            }
        }
        break;
    case EM_DMG_HIT_LYING:
        switch (em->state) {
        case DIAB_IDLE_STATUS2_COLLAPSE:
            EmDiablos_ChangeState(em, EM_GROUP_IDLE, DIAB_IDLE_STATUS2_END, EM_SYNC_OWNER);
            em->scriptPending = 0;
            break;
        case DIAB_IDLE_STATUS1_COLLAPSE:
            EmDiablos_ChangeState(em, EM_GROUP_IDLE, DIAB_IDLE_STATUS1_END, EM_SYNC_OWNER);
            em->scriptPending = 0;
            break;
        case DIAB_IDLE_SAND_DOWNED:
            EmDiablos_ChangeState(em, EM_GROUP_REACT, DIAB_REACT_STATUS2_SAND_GET_UP, EM_SYNC_OWNER);
            em->scriptPending = 0;
            break;
        }
        break;
    case EM_DMG_PART8_DOWN:
        EmDiablos_ChangeState(em, EM_GROUP_REACT, DIAB_REACT_PART_BREAK, EM_SYNC_OWNER);
        break;
    case EM_DMG_ATTACK5_FLINCH:
        if (em->rage == 0 && em->stance == EM_STANCE_UNDER && (em->stateGroup != EM_GROUP_REACT || em->state != DIAB_REACT_POP_OUT)) {
            EmDiablos_ChangeState(em, EM_GROUP_REACT, DIAB_REACT_POP_OUT, EM_SYNC_OWNER);
            break;
        }
    case EM_DMG_PART_FLINCH:
        Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
        if (em->stance == EM_STANCE_AIR) {
            EmDiablos_ChangeState(em, EM_GROUP_REACT, DIAB_REACT_FALL, EM_SYNC_OWNER);
        } else if (em->stance == EM_STANCE_UNDER) {
            EmDiablos_ChangeState(em, EM_GROUP_REACT, DIAB_REACT_POP_OUT, EM_SYNC_OWNER);
        } else {
            switch (em->flinchPart) {
            case 0:
            case 7:
                EmDiablos_ChangeState(em, EM_GROUP_REACT, DIAB_REACT_FLINCH, EM_SYNC_OWNER);
                break;
            case 6:
                if (em->type == EM_KIND_DIABLOS || em->type == EM_KIND_BLACK_DIABLOS) {
                    if (em->parts[em->flinchPart].state == 1 || em->parts[em->flinchPart].state == 2) {
                        EmDiablos_ChangeState(em, EM_GROUP_REACT, DIAB_REACT_PART6_BREAK, EM_SYNC_OWNER);
                    } else {
                        EmDiablos_ChangeState(em, EM_GROUP_REACT, DIAB_REACT_FLINCH_C, EM_SYNC_OWNER);
                    }
                } else if (em->parts[em->flinchPart].state == 2) {
                    EmDiablos_ChangeState(em, EM_GROUP_REACT, DIAB_REACT_PART6_BREAK, EM_SYNC_OWNER);
                } else {
                    EmDiablos_ChangeState(em, EM_GROUP_REACT, DIAB_REACT_FLINCH_C, EM_SYNC_OWNER);
                }
                break;
            case 5:
                EmDiablos_ChangeState(em, EM_GROUP_REACT, DIAB_REACT_FLINCH_C, EM_SYNC_OWNER);
                break;
            case 1:
            case 2:
                EmDiablos_ChangeState(em, EM_GROUP_REACT, DIAB_REACT_FLINCH_D, EM_SYNC_OWNER);
                break;
            case 3:
            case 4:
            default:
                if (em->parts[em->flinchPart].state >= 2) {
                    if (em->flinchPart != 3) {
                        EmDiablos_ChangeState(em, EM_GROUP_REACT, DIAB_REACT_KNOCK_DOWN, EM_SYNC_OWNER);
                    } else {
                        EmDiablos_ChangeState(em, EM_GROUP_REACT, DIAB_REACT_KNOCK_DOWN_PART3, EM_SYNC_OWNER);
                    }
                } else {
                    EmDiablos_ChangeState(em, EM_GROUP_REACT, DIAB_REACT_FLINCH_B, EM_SYNC_OWNER);
                }
                break;
            }
        }
        break;
    case EM_DMG_HIT_MODE3:
        if (em->stance != EM_STANCE_AIR && em->stance != EM_STANCE_UNDER) {
            EmDiablos_ChangeState(em, EM_GROUP_REACT, DIAB_REACT_FLINCH, EM_SYNC_OWNER);
            em->scriptPending = 0;
        }
        break;
    }
    switch (gQuest.stage) {
    case QUEST_ID_DIABLOS_SCENE:
        if ((em->type == EM_KIND_DIABLOS || em->type == EM_KIND_BLACK_DIABLOS) && Area_DayId(em->area) == AREA_DESERT_2 && Save_TestEventFlag(EVENT_FLAG_DIABLOS_SCENE) == 0) {
            if (Area_DayId(gScene.area) == AREA_DESERT_2 && gScene.eventRunning == 1 && em->stateGroup != EM_GROUP_EVENT) {
                EmDiablos_ChangeState(em, EM_GROUP_EVENT, DIAB_EVENT_DIABLOS_SCENE, EM_SYNC_SEND);
            }
            break;
        }
        goto noScene;
    case QUEST_ID_MONOBLOS_SCENE:
        if ((em->type == EM_KIND_MONOBLOS || em->type == EM_KIND_WHITE_MONOBLOS) && Area_DayId(em->area) == AREA_DESERT_2 && Save_TestEventFlag(EVENT_FLAG_MONOBLOS_SCENE) == 0) {
            if (Area_DayId(gScene.area) == AREA_DESERT_2 && gScene.eventRunning == 1 && em->stateGroup != EM_GROUP_EVENT) {
                EmDiablos_ChangeState(em, EM_GROUP_EVENT, DIAB_EVENT_MONOBLOS_SCENE, EM_SYNC_SEND);
            }
            break;
        }
    default:
    noScene:
        Em_UpdatePendingScript(em);
        break;
    }
    Em_ClearHitReactMode(em);
    EmDiablos_RunState(em, (EmWork *) w);
    if (em->stateRequest != 0) {
        EmDiablos_RunState(em, (EmWork *) w);
        em->stateRequest = 0;
    }
    if (em->surfaceKind != 0 && gScene.area == em->area) {
        if (em->stance == EM_STANCE_UNDER) {
            f32 limitY = em->surfaceY - *(f32 *) &em->surfaceDepth;
            if (!(em->posY <= limitY)) {
                em->posY = limitY;
            }
            limitY = em->surfaceY - 1000.0f;
            if (!(em->floorY < limitY)) {
                em->floorY = limitY;
            }
            if (em->posY < em->floorY) {
                em->posY = em->floorY;
            }
        }
        if (em->stance == EM_STANCE_GROUND || em->stance == EM_STANCE_DEAD) {
            em->floorY = em->surfaceY;
            if (em->posY < em->floorY) {
                em->posY = em->floorY;
            }
        }
    }
}

/*
 * Stores prevStateGroup/prevState and runs the group's dispatcher;
 * EM_GROUP_EVENT2 runs EM_GROUP_EVENT's.
 */
void EmDiablos_RunState(Em *em, EmWork *work)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->stateGroup) {
    case EM_GROUP_IDLE:
        EmDiablos_RunIdleState(em, work);
        break;
    case EM_GROUP_MOVE:
        EmDiablos_RunMoveState(em, work);
        break;
    case EM_GROUP_FLY:
        EmDiablos_RunBurrowState(em, work);
        break;
    case EM_GROUP_ATTACK:
        EmDiablos_RunAttackState(em, work);
        break;
    case EM_GROUP_REACT:
        EmDiablos_RunReactState(em, work);
        break;
    case EM_GROUP_DEFEAT:
        EmDiablos_RunDefeatState(em, work);
        break;
    case EM_GROUP_EVENT:
        EmDiablos_RunEventState(em, work);
        break;
    case EM_GROUP_EVENT2:
        EmDiablos_RunEventState(em, work);
        break;
    case EM_GROUP_VERSUS:
        EmDiablos_RunVersusState(em, work);
        
    default:
        break;
    }
}

/*
 * Requests camera shake kind shake (CamShake_ReqAIf) on the frame layer 0's
 * motion passes frame.
 */
void EmDiablos_ShakeCamOnFrame(void *em, s32 frame, s32 shake)
{
    if (EmMotion_PassedFrameScaled((Em *) em, (f32) frame, 0U) != 0) {
        CamShake_ReqAIf(em, shake);
    }
}

/*
 * Does nothing: EmDiablos_MotionEffects' default case (motions without
 * effects).
 */
static void EmDiablos_MotionEffectsDefault(Em *em)
{
}

/*
 * Keeps the current motion id (EmWork.sndMotion), runs
 * EmDiablos_MotionSounds, then per layer-0 motion creates on fixed frames
 * its attack objects (DiablosAttack_Create kinds), camera shakes, dust
 * clouds (DustCloud_Create; the rage's type 26 every 4th frame in frame
 * windows while enraged), sand effects (Effect0D) and debris; the walk's and
 * run's attacks are skipped under the sand (EM_STANCE_UNDER), where the run
 * sprays sand at the surface instead.
 */
void EmDiablos_MotionEffects(Em *em, EmWork *work)
{
    f32 pos[3];
    f32 ofs[3];
    f32 puffOfs[3];
    f32 puffOfsB[3];

    if (em->motionId[0] != work->sndMotion) {
        work->sndMotion = em->motionId[0];
    }
    EmDiablos_MotionSounds(em, (struct EmWork *)work);
    switch (work->sndMotion) {
    case EM_MOTION_ID(0, DIAB_MOT_IDLE):
        break;
    case EM_MOTION_ID(0, DIAB_MOT_WALK):
        if (em->stance != EM_STANCE_UNDER) {
            EmDiablos_ShakeCamOnFrame(em, 52, 1);
            EmDiablos_ShakeCamOnFrame(em, 116, 1);
            if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
                DiablosAttack_Create(em, 2);
            }
            if (EmMotion_PassedFrameScaled(em, 50.0f, 0) != 0) {
                DiablosAttack_Create(em, 3);
            }
            if (EmMotion_PassedFrameScaled(em, 120.0f, 0) != 0) {
                DiablosAttack_Create(em, 4);
                break;
            }
        } else {
            break;
        }
        break;
    case EM_MOTION_ID(0, DIAB_MOT_TURN_RIGHT):
        if (em->stance != EM_STANCE_UNDER) {
            EmDiablos_ShakeCamOnFrame(em, 60, 1);
            if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
                DiablosAttack_Create(em, 5);
            }
            if (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) {
                DiablosAttack_Create(em, 6);
            }
            if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 2.0f, 78.0f) != 0) && !(gScene.frame & 3)) {
                DustCloud_Create(em, 26, 1, 1.0f);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, DIAB_MOT_TURN_LEFT):
        if (em->stance != EM_STANCE_UNDER) {
            EmDiablos_ShakeCamOnFrame(em, 60, 1);
            if (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) {
                DiablosAttack_Create(em, 7);
            }
            if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
                DiablosAttack_Create(em, 8);
            }
            if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 2.0f, 78.0f) != 0) && !(gScene.frame & 3)) {
                DustCloud_Create(em, 26, 0, 1.0f);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, 7):
        if (EmMotion_PassedFrameScaled(em, 18.0f, 0) != 0) {
            DiablosAttack_Create(em, 9);
        }
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            DiablosAttack_Create(em, 10);
        }
        if (em->rage != 0) {
            if ((EmMotion_InFrameRange(em, 0, 2.0f, 18.0f) != 0) && !(gScene.frame & 3)) {
                DustCloud_Create(em, 26, 1, 1.0f);
            }
            if ((EmMotion_InFrameRange(em, 0, 20.0f, 70.0f) != 0) && !(gScene.frame & 3)) {
                DustCloud_Create(em, 26, 0, 1.0f);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, 8):
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            DiablosAttack_Create(em, 11);
        }
        if (EmMotion_PassedFrameScaled(em, 18.0f, 0) != 0) {
            DiablosAttack_Create(em, 12);
        }
        if (em->rage != 0) {
            if ((EmMotion_InFrameRange(em, 0, 2.0f, 18.0f) != 0) && !(gScene.frame & 3)) {
                DustCloud_Create(em, 26, 0, 1.0f);
            }
            if ((EmMotion_InFrameRange(em, 0, 20.0f, 70.0f) != 0) && !(gScene.frame & 3)) {
                DustCloud_Create(em, 26, 1, 1.0f);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, DIAB_MOT_LIMP):
        if (EmMotion_PassedFrameScaled(em, 42.0f, 0) != 0) {
            DiablosAttack_Create(em, 25);
        }
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            DiablosAttack_Create(em, 26);
        }
        if (EmMotion_PassedFrameScaled(em, 74.0f, 0) != 0) {
            DiablosAttack_Create(em, 26);
            break;
        }
        break;
    case EM_MOTION_ID(0, DIAB_MOT_DESCEND):
        if (EmMotion_PassedFrameScaled(em, 52.0f, 0) || EmMotion_PassedFrameScaled(em, 104.0f, 0) || EmMotion_PassedFrameScaled(em, 162.0f, 0)) {
            PartFx_SpawnForEmScaled(em, DIAB_BONE_BODY, 7, 5.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, 12):
        break;
    case EM_MOTION_ID(0, 13):
        if ((EmMotion_PassedFrameScaled(em, 26.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 52.0f, 0) != 0)) {
            PartFx_SpawnForEmScaled(em, 1, 7, 5.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, DIAB_MOT_AIR):
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
    case EM_MOTION_ID(0, DIAB_MOT_RUN):
        if (em->stance == EM_STANCE_UNDER) {
            if (gScene.area == em->area) {
                Vec_Set(pos, em->posX, em->surfaceY, em->posZ);
                if (!(gScene.frame & 3)) {
                    PartFx_SpawnAt(pos, 20, 20.0f);
                    Em_PlaySeChecked(em, 34, 0, pos, 24, 0);
                    break;
                }
            }
        } else {
            if (EmMotion_PassedFrameScaled(em, 106.0f, 0) != 0) {
                DiablosAttack_Create(em, 14);
            }
            if ((EmMotion_PassedFrameScaled(em, 100.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 154.0f, 0) != 0)) {
                DustCloud_Create(em, 2, 0, 1.0f);
            }
            if ((EmMotion_PassedFrameScaled(em, 88.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 132.0f, 0) != 0)) {
                DustCloud_Create(em, 3, 0, 1.0f);
            }
            if (em->rage != 0) {
                if ((EmMotion_PassedFrameScaled(em, 106.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 132.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 158.0f, 0) != 0)) {
                    DustCloud_Create(em, 25, (Rand_Next(1) & 1) + 2, 1.0f);
                }
                if ((EmMotion_InFrameRange(em, 0, 4.0f, 50.0f) != 0) && !(gScene.frame & 3)) {
                    DustCloud_Create(em, 26, (s16) (Rand_Next(1) & 1), 1.0f);
                    break;
                }
            }
        }
        break;
    case EM_MOTION_ID(0, DIAB_MOT_LEAP):
        if (((EmMotion_PassedFrameScaled(em, 74.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 140.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 194.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 240.0f, 0) != 0)) && (em->posY <= (1000.0f + em->floorY))) {
            DustCloud_Create(em, 1, 0, 1.0f);
        }
        if (em->rage != 0) {
            if ((EmMotion_InFrameRange(em, 0, 10.0f, 64.0f) != 0) && !(gScene.frame & 3)) {
                DustCloud_Create(em, 26, (s16) (Rand_Next(1) & 1), 1.0f);
            }
            if (EmMotion_InFrameRange(em, 0, 136.0f, 140.0f) || EmMotion_InFrameRange(em, 0, 188.0f, 192.0f) || EmMotion_InFrameRange(em, 0, 136.0f, 234.0f)) {
                DustCloud_Create(em, 26, (s16) (Rand_Next(1) & 1), 1.0f);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, DIAB_MOT_LAND):
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            DiablosAttack_Create(em, 13);
        }
        if (EmMotion_PassedFrameScaled(em, 16.0f, 0) != 0) {
            DustCloud_Create(em, 11, 0, 1.0f);
        }
        if (em->rage != 0) {
            if (EmMotion_PassedFrameScaled(em, 14.0f, 0) != 0) {
                DustCloud_Create(em, 25, 2, 1.0f);
                DustCloud_Create(em, 25, 3, 1.0f);
            }
            if ((EmMotion_InFrameRange(em, 0, 18.0f, 44.0f) != 0) && !(gScene.frame & 3)) {
                DustCloud_Create(em, 26, (s16) (Rand_Next(1) & 1), 1.0f);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, 29):
        break;
    case EM_MOTION_ID(0, DIAB_MOT_REST):
        puffOfs[1] = 10.0f;
        puffOfs[2] = 140.0f;
        puffOfs[0] = 0.0f;
        Em_SpawnLyingPuffs(em, DIAB_BONE_BREATH, puffOfs, 1.6f);
        break;
    case EM_MOTION_ID(0, DIAB_MOT_REST_LIE_DOWN):
    case EM_MOTION_ID(0, DIAB_MOT_REST_GET_UP):
        break;
    case EM_MOTION_ID(0, DIAB_MOT_LIE):
        puffOfsB[1] = 10.0f;
        puffOfsB[2] = 140.0f;
        puffOfsB[0] = 0.0f;
        Em_SpawnLyingPuffs(em, DIAB_BONE_BREATH, puffOfsB, 1.6f);
        break;
    case EM_MOTION_ID(0, DIAB_MOT_STARE):
        if (EmMotion_PassedFrameScaled(em, 50.0f, 0) != 0) {
            DiablosAttack_Create(em, 1);
        }
        if (EmMotion_PassedFrameScaled(em, 46.0f, 0) != 0) {
            DiablosAttack_Create(em, 31);
            break;
        }
        break;
    case EM_MOTION_ID(0, DIAB_MOT_GET_UP):
        break;
    case EM_MOTION_ID(0, DIAB_MOT_COLLAPSE):
        if (EmMotion_PassedFrameScaled(em, 148.0f, 0) != 0) {
            DiablosAttack_Create(em, 15);
        }
        if (EmMotion_PassedFrameScaled(em, 156.0f, 0) != 0) {
            DustCloud_Create(em, 0, 0, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, DIAB_MOT_LIE_DOWN):
        break;
    case EM_MOTION_ID(0, DIAB_MOT_TAIL_SPIN):
        EmDiablos_ShakeCamOnFrame(em, 44, 1);
        EmDiablos_ShakeCamOnFrame(em, 73, 1);
        if (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) {
            DiablosAttack_Create(em, 17);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 6.0f, 38.0f) != 0) && !(gScene.frame & 3)) {
            DustCloud_Create(em, 26, 0, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, DIAB_MOT_ALERT_IDLE):
        if (EmMotion_PassedFrameScaled(em, 54.0f, 0) != 0) {
            DustCloud_Create(em, 2, 2, 0.8f);
        }
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            DiablosAttack_Create(em, 27);
        }
        if (EmMotion_PassedFrameScaled(em, 168.0f, 0) != 0) {
            DiablosAttack_Create(em, 27);
        }
        if (EmMotion_PassedFrameScaled(em, 56.0f, 0) != 0) {
            DiablosAttack_Create(em, 28);
        }
        if (em->rage != 0) {
            if (EmMotion_InFrameRange(em, 0, 156.0f, 248.0f) || EmMotion_InFrameRange(em, 0, 276.0f, 346.0f)) {
                DustCloud_Create(em, 26, (s16) (Rand_Next(1) & 1), 1.0f);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, DIAB_MOT_TURN_IN_PLACE):
        if (EmMotion_PassedFrameScaled(em, 102.0f, 0) != 0) {
            DiablosAttack_Create(em, 18);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 16.0f, 26.0f) != 0) && !(gScene.frame & 3)) {
            DustCloud_Create(em, 26, (s16) (Rand_Next(1) & 1), 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, DIAB_MOT_FLINCH):
        if (EmMotion_PassedFrameScaled(em, 44.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, 27, 7, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 62.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, DIAB_BONE_FOOT_A, 7, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 80.0f, 0) != 0) {
            DustCloud_Create(em, 1, 128, 0.6f);
            break;
        }
        break;
    case EM_MOTION_ID(0, DIAB_MOT_FLINCH_E):
        if (EmMotion_PassedFrameScaled(em, 24.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, DIAB_BONE_STEP_B, 7, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, DIAB_MOT_SHAKE_OFF):
    case EM_MOTION_ID(0, DIAB_MOT_FLINCH_C):
    case EM_MOTION_ID(0, DIAB_MOT_FLINCH_D):
        break;
    case EM_MOTION_ID(0, DIAB_MOT_PART_BREAK):
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
    case EM_MOTION_ID(0, DIAB_MOT_FLINCH_B):
    case EM_MOTION_ID(0, 67):
        break;
    case EM_MOTION_ID(0, DIAB_MOT_DIE):
        if (EmMotion_PassedFrameScaled(em, 268.0f, 0) != 0) {
            DustCloud_Create(em, 0, 0, 0.9f);
            break;
        }
        break;
    case EM_MOTION_ID(0, DIAB_MOT_KNOCK_DOWN):
        EmDiablos_ShakeCamOnFrame(em, 26, 2);
        if (EmMotion_PassedFrameScaled(em, 26.0f, 0) != 0) {
            DustCloud_Create(em, 18, 128, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, DIAB_MOT_DOWN):
    case EM_MOTION_ID(0, DIAB_MOT_DOWN_B):
    case EM_MOTION_ID(0, DIAB_MOT_DOWN_GET_UP):
    case EM_MOTION_ID(0, DIAB_MOT_DOWN_GET_UP_B):
        break;
    case EM_MOTION_ID(0, 73):
        EmDiablos_ShakeCamOnFrame(em, 26, 2);
        if ((EmMotion_ReachedFrame(em, 0, 28.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 52.0f) == 0) && !(gScene.frame & 3)) {
            DustCloud_Create(em, 10, 128, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, DIAB_MOT_KNOCK_DOWN_B):
        EmDiablos_ShakeCamOnFrame(em, 26, 2);
        if (EmMotion_PassedFrameScaled(em, 26.0f, 0) != 0) {
            DustCloud_Create(em, 18, 129, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, DIAB_MOT_STATUS4):
    case EM_MOTION_ID(0, DIAB_MOT_FALL):
        break;
    case EM_MOTION_ID(0, DIAB_MOT_FALL_LAND):
        if (EmMotion_PassedFrameScaled(em, 8.0f, 0) != 0) {
            DustCloud_Create(em, 11, 0, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, DIAB_MOT_IDLE_PAIR_A):
    case EM_MOTION_ID(0, DIAB_MOT_IDLE_PAIR_B):
    case EM_MOTION_ID(0, DIAB_MOT_DIE_FALLING):
    case EM_MOTION_ID(0, 83):
    case EM_MOTION_ID(0, 90):
        break;
    case EM_MOTION_ID(0, DIAB_MOT_SAND_STRUGGLE):
        if (em->rage != 0) {
            if (EmMotion_InFrameRange(em, 0, 12.0f, 24.0f) || EmMotion_InFrameRange(em, 0, 46.0f, 90.0f)) {
                if (!(gScene.frame & 3)) {
                    DustCloud_Create(em, 26, (s16) (Rand_Next(1) & 1), 1.0f);
                    break;
                }
            }
        }
        break;
    case EM_MOTION_ID(0, DIAB_MOT_DOWNED):
    case EM_MOTION_ID(0, DIAB_MOT_DOWNED_GET_UP):
        break;
    case EM_MOTION_ID(0, DIAB_MOT_CHARGE_STOP):
        if (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0) {
            DiablosAttack_Create(em, 22);
        }
        if (EmMotion_PassedFrameScaled(em, 18.0f, 0) != 0) {
            DustCloud_Create(em, 2, 6, 1.8f);
        }
        if ((EmMotion_InFrameRange(em, 0, 40.0f, 80.0f) != 0) && !(gScene.frame & 3)) {
            PartFx_SpawnForEmScaled(em, DIAB_BONE_BODY, 3, 7.0f);
        }
        if (em->rage != 0) {
            if (EmMotion_PassedFrameScaled(em, 22.0f, 0) != 0) {
                DustCloud_Create(em, 25, 2, 1.0f);
                DustCloud_Create(em, 25, 3, 1.0f);
            }
            if ((EmMotion_InFrameRange(em, 0, 72.0f, 116.0f) != 0) && !(gScene.frame & 3)) {
                DustCloud_Create(em, 26, (s16) (Rand_Next(1) & 1), 1.0f);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, DIAB_MOT_BURROW):
        EmDiablos_ShakeCamOnFrame(em, 104, 4);
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            DiablosAttack_Create(em, 32);
        }
        if (EmMotion_PassedFrameScaled(em, 20.0f, 0) != 0) {
            DustCloud_Create(em, 2, 0, 1.2f);
        }
        if (EmMotion_PassedFrameScaled(em, 82.0f, 0) != 0) {
            DustCloud_Create(em, 3, 0, 1.2f);
        }
        if (EmMotion_PassedFrameScaled(em, 104.0f, 0) != 0) {
            DustCloud_Create(em, 1, 0, 1.0f);
        }
        if ((EmMotion_InFrameRange(em, 0, 40.0f, 62.0f) != 0) && !(gScene.frame & 3)) {
            PartFx_SpawnForEmScaled(em, 15, 3, 5.0f);
        }
        if ((EmMotion_PassedFrameScaled(em, 18.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 22.0f, 0) != 0)) {
            PartFx_SpawnForEmScaled(em, 9, 3, 5.0f);
        }
        if ((EmMotion_PassedFrameScaled(em, 148.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 160.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 172.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 184.0f, 0) != 0)) {
            PartFx_SpawnForEmScaled(em, DIAB_BONE_TAIL_A, 3, 6.0f);
        }
        if (!(em->fxFlags & 1) && (EmMotion_PassedFrameScaled(em, 180.0f, 0) != 0)) {
            PartFx_SpawnForEmScaled(em, DIAB_BONE_TAIL_B, 19, 0.7f);
            break;
        }
        break;
    case EM_MOTION_ID(0, DIAB_MOT_EMERGE):
        EmDiablos_ShakeCamOnFrame(em, 2, 5);
        if (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0) {
            DiablosAttack_Create(em, 21);
        }
        if (EmMotion_PassedFrameScaled(em, 40.0f, 0) != 0) {
            DiablosAttack_Create(em, 33);
        }
        if (EmMotion_PassedFrameScaled(em, 12.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, DIAB_BONE_BREATH, 19, 1.4f);
            if (Player_InCurrentArea(em)) {
                Em_BonePos(em, DIAB_BONE_BREATH, pos);
                pos[1] = em->floorY;
                AttackBurst_CreateAtScaled(pos, (u16) em->rotY, 8, 1.0f);
            }
        }
        if (em->rage != 0) {
            if (EmMotion_InFrameRange(em, 0, 12.0f, 36.0f) || EmMotion_InFrameRange(em, 0, 66.0f, 130.0f) || EmMotion_InFrameRange(em, 0, 168.0f, 204.0f)) {
                if (!(gScene.frame & 3)) {
                    DustCloud_Create(em, 26, (s16) (Rand_Next(1) & 1), 1.0f);
                    break;
                }
            }
        }
        break;
    case EM_MOTION_ID(0, DIAB_MOT_POP_OUT):
        EmDiablos_ShakeCamOnFrame(em, 12, 3);
        if (EmMotion_PassedFrameScaled(em, 12.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, DIAB_BONE_BODY, 19, 1.4f);
            if (Player_InCurrentArea(em)) {
                Em_BonePos(em, DIAB_BONE_BODY, pos);
                pos[1] = em->floorY;
                AttackBurst_CreateAtScaled(pos, (u16) em->rotY, 8, 1.0f);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, DIAB_MOT_ATTACK_A):
        if (EmMotion_PassedFrameScaled(em, 48.0f, 0) != 0) {
            DiablosAttack_Create(em, 24);
        }
        if (EmMotion_PassedFrameScaled(em, 50.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, DIAB_BONE_STEP_A, 3, 5.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 54.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, DIAB_BONE_STEP_A, 3, 5.0f);
        }
        if ((em->rage != 0) && (EmMotion_InFrameRange(em, 0, 86.0f, 152.0f) != 0) && !(gScene.frame & 3)) {
            DustCloud_Create(em, 26, (s16) (Rand_Next(1) & 1), 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, DIAB_MOT_ATTACK_B):
        if (EmMotion_PassedFrameScaled(em, 50.0f, 0) != 0) {
            DiablosAttack_Create(em, 16);
        }
        if (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, DIAB_BONE_STEP_B, 7, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 44.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, DIAB_BONE_STEP_A, 3, 5.0f);
        }
        if (em->rage != 0) {
            if (EmMotion_InFrameRange(em, 0, 8.0f, 24.0f) || EmMotion_InFrameRange(em, 0, 136.0f, 172.0f)) {
                if (!(gScene.frame & 3)) {
                    DustCloud_Create(em, 26, (s16) (Rand_Next(1) & 1), 1.0f);
                    break;
                }
            }
        }
        break;
    case EM_MOTION_ID(0, DIAB_MOT_ATTACK_C):
        if (EmMotion_PassedFrameScaled(em, 46.0f, 0) != 0) {
            DiablosAttack_Create(em, 20);
        }
        if (EmMotion_PassedFrameScaled(em, 94.0f, 0) != 0) {
            DiablosAttack_Create(em, 29);
        }
        if (!(em->fxFlags & 1)) {
            if (EmMotion_PassedFrameScaled(em, 54.0f, 0) != 0) {
                PartFx_SpawnForEmScaled(em, 43, 7, 2.0f);
            }
            if (EmMotion_PassedFrameScaled(em, 104.0f, 0) != 0) {
                PartFx_SpawnForEmScaled(em, 43, 7, 3.0f);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, DIAB_MOT_ATTACK_D):
        if (EmMotion_PassedFrameScaled(em, 54.0f, 0) != 0) {
            DiablosAttack_Create(em, 30);
        }
        if (EmMotion_PassedFrameScaled(em, 54.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, DIAB_BONE_STEP_B, 3, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 52.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, DIAB_BONE_STEP_A, 3, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 56.0f, 0) != 0) {
            PartFx_SpawnForEmScaled(em, DIAB_BONE_STEP_A, 3, 2.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, DIAB_MOT_ROAR):
        if (EmMotion_PassedFrameScaled(em, 112.0f, 0) != 0) {
            DiablosAttack_Create(em, 19);
            ElementBurst_CreateOnEm(em, 5, 6, 1.0f);
            break;
        }
        break;
    case EM_MOTION_ID(0, DIAB_MOT_PART6_BREAK):
        break;
    case EM_MOTION_ID(0, DIAB_MOT_HORN_STUCK):
        EmDiablos_ShakeCamOnFrame(em, 4, 3);
        EmDiablos_ShakeCamOnFrame(em, 104, 0);
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            ofs[1] = 30.0f;
            ofs[0] = 0.0f;
            ofs[2] = 100.0f;
            Vec_RotateSelf((f32 *) ofs, (f32 *) (Em_Bone(em, DIAB_BONE_HORN)));
            Em_BonePos(em, DIAB_BONE_HORN, pos);
            Vec_Add(pos, pos, ofs);
            HitSpark_SpawnForOwner(em, 0, 9, 0, pos, 1.0f);
            HitSpark_SpawnForOwner(em, 0, 9, 1, pos, 1.0f);
            PartFx_SpawnIfHere(em, pos, 34, 10.0f);
        }
        if (!(gScene.frame & 0x1F)) {
            FallingDebris_Create(em, 2);
            break;
        }
        break;
    case EM_MOTION_ID(0, DIAB_MOT_HORN_PULL_FREE):
        if (EmMotion_PassedFrameScaled(em, 68.0f, 0) != 0) {
            FallingDebris_Create(em, 2);
            break;
        }
        break;
    default:
        EmDiablos_MotionEffectsDefault(em);
        break;
    }
}

/*
 * Callback slot 3 (Em_RunMotionFx) of EmDiablos_Callbacks: skips the first
 * frame (fxStep 0 -> 1), then runs EmDiablos_MotionEffects every frame.
 */
void EmDiablos_UpdateMotionEffects(Em *em)
{
    EmDiablosWork *w = (EmDiablosWork *)&em->work;

    switch (w->fxStep) {
    case 0:
        w->fxStep++;
        break;
    case 1:
        EmDiablos_MotionEffects(em, (EmWork *) w);
        break;
    }
}

/*
 * Ends an attack: EmDiablos_EndState (back to DIAB_IDLE, script pending);
 * called by two group-3 states when their motion ends.
 */
void EmDiablos_EndAttack(Em *em, EmWork *work)
{
    EmDiablos_EndState(em, 0, 0);
}

/*
 * Callback slot 2 (EmVt.unk08) of EmDiablos_Callbacks: does nothing.
 */
void EmDiablos_TableNop(void)
{
}

/*
 * Callback slot 6 (EmVt.part8DownQuery): returns 1 (part 8 may go down:
 * EM_DMG_PART8_DOWN, DIAB_REACT_PART_BREAK) when it is not in a sand
 * reaction state, not in the air (EM_STANCE_AIR) and the part break has not
 * run yet (partBreakCount 0), else 0.
 */
s32 EmDiablos_AllowHit8(Em *em)
{
    if ((EmDiablos_InSandReactState(em) == 0) && (em->stance != EM_STANCE_AIR) && ((s8) em->partBreakCount == 0)) {
        return 1;
    }
    return 0;
}

/*
 * Enters an idle state: motion speed 1, EM_STANCE_GROUND, then the state's
 * own call (the rest resets the notice, the status states begin or end
 * their status, DIAB_IDLE_REST_END refills need 3, DIAB_IDLE_HOLD2_SHAKE_OFF
 * clears holdTimer2, the two alerts start the rage); then Em_SetStateSync.
 */
void EmDiablos_SetIdleState(Em *em, u16 state, u8 mode)
{
    em->motionSpeed = 1.0f;
    switch (state) {
    case DIAB_IDLE_REST:
        Em_ResetNotice(em);
        break;
    case DIAB_IDLE_STATUS2_COLLAPSE:
    case DIAB_IDLE_STATUS2_LIE_DOWN:
    case DIAB_IDLE_SAND_DOWNED:
        Em_BeginStatus2(em);
        break;
    case DIAB_IDLE_REST_END:
        Em_RefillNeed3(em);
        break;
    case DIAB_IDLE_STATUS2_END:
        Em_EndStatus2(em);
        break;
    case DIAB_IDLE_STATUS1_COLLAPSE:
        Em_BeginStatus1(em);
        break;
    case DIAB_IDLE_STATUS1_END:
        Em_EndStatus1(em);
        break;
    case DIAB_IDLE_STATUS4_SHAKE_OFF:
        Em_EndStatus4(em);
        break;
    case DIAB_IDLE_HOLD2_SHAKE_OFF:
        em->holdTimer2 = 0;
        break;
    case DIAB_IDLE_STUN_SHAKE_OFF:
        Em_EndStatus5(em);
        break;
    case DIAB_IDLE_ALERT_RAGE:
        Em_StartRage(em);
        break;
    case DIAB_IDLE_ALERT_PENDING_RAGE:
        Em_StartPendingRage(em);
        break;
    }
    em->stance = EM_STANCE_GROUND;
    Em_SetStateSync(em, EM_GROUP_IDLE, state, mode);
}

/*
 * Enters a move state (EM_STANCE_GROUND). DIAB_MOVE_WALK and DIAB_MOVE_RUN
 * become DIAB_MOVE_LIMP below 30% hp, and the run becomes
 * DIAB_MOVE_RUN_AND_STOP in combat; otherwise walk speed and, without a
 * goal, a random timer (90 to 149 frames). DIAB_MOVE_END only sets
 * scriptPending (no state change). DIAB_MOVE_TURN becomes
 * DIAB_MOVE_TURN_IN_COMBAT in combat. DIAB_MOVE_RUN_AND_STOP_SLOW sets motion
 * speed 0.7, then as DIAB_MOVE_RUN_AND_STOP: run speed and, without a goal
 * kind, the random timer. DIAB_MOVE_LIMP keeps the random timer (at most
 * 300) or measures goalDist to the goal. Then Em_SetStateSync. The gotos
 * join the cases they name, as the game's code does.
 */
void EmDiablos_SetMoveState(Em *em, u16 state, u8 mode)
{
    em->stance = EM_STANCE_GROUND;
    em->motionSpeed = 1.0f;
    switch (state) {
    case DIAB_MOVE_WALK:
    case DIAB_MOVE_RUN:
        if (em->hp < (s16) (0.3f * em->hpMax)) {
            state = DIAB_MOVE_LIMP;
            goto limp;
        }
        if (state == DIAB_MOVE_RUN && em->combat == 1) {
            state = DIAB_MOVE_RUN_AND_STOP;
            goto runAndStop;
        }
        Em_SetSpeedScaled(em, 0, 1000.0f, 500.0f);
        if (em->remote == 0 && em->hasGoal == 0) {
            em->timer = (em->stateRand % 120 + 180) / 2;
        }
        break;
    case DIAB_MOVE_END:
        em->scriptPending = 1;
        return;
    case DIAB_MOVE_TURN:
        if (em->combat == 1) {
            state = DIAB_MOVE_TURN_IN_COMBAT;
        }
        break;
    case DIAB_MOVE_RUN_AND_STOP_SLOW:
        em->motionSpeed = 0.7f;
    case DIAB_MOVE_RUN_AND_STOP:
    runAndStop:
        Em_SetSpeedScaled(em, 2, 1000.0f, 500.0f);
        if (em->curGoalKind == 0) {
            em->timer = (em->stateRand % 120 + 180) / 2;
        }
        break;
    case DIAB_MOVE_LIMP:
    limp:
        if (em->curGoalKind == 0) {
            em->hasGoal = 0;
            em->timer = (em->stateRand % 120 + 180) / 2;
        } else {
            em->hasGoal = 1;
            em->goalDist = Vec_DistanceXZ(&em->posX, em->goal);
        }
        if (em->timer > 300) {
            em->timer = 300;
        }
        break;
    case DIAB_MOVE_TURN_IN_COMBAT:
    case DIAB_MOVE_TURN_TO_GOAL:
        break;
    }
    Em_SetStateSync(em, EM_GROUP_MOVE, state, mode);
}

/*
 * Enters a burrow state: 0 (burrowing) where the floor is not sand in the
 * player's area changes to DIAB_IDLE_ALERT_BRIEF instead; 4 and 8 run speed
 * and the random timer; 5 walk speed and the random timer or goalDist / 7; 7
 * and 11 clear the velocity, velZ 20 and a timer of three times the frames to
 * the goal; 9 velZ at least 20 and (owner) the frames to the goal; then
 * Em_SetStateSync.
 */
void EmDiablos_SetBurrowState(Em *em, u16 state, u8 mode)
{
    em->motionSpeed = 1.0f;
    switch (state) {
    case 0:
        if (em->surfaceKind == 0 && gScene.area == em->area) {
            EmDiablos_ChangeState(em, EM_GROUP_IDLE, DIAB_IDLE_ALERT_BRIEF, EM_SYNC_SEND);
            return;
        }
        break;
    case 4:
    case 8:
        Em_SetSpeedScaled(em, 2, 1000.0f, 500.0f);
        if (em->curGoalKind == 0) {
            em->timer = (em->stateRand % 120 + 180) / 2;
        }
        break;
    case 5:
        Em_SetSpeedScaled(em, 0, 1000.0f, 500.0f);
        if (em->goalKind == 0) {
            em->timer = (em->stateRand % 120 + 180) / 2;
        } else {
            em->timer = (s32) em->goalDist / 7;
        }
        break;
    case 7:
        Player_ClearVelocity(em);
        em->velZ = 20.0f;
        em->timer = (s32) (Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) / em->velZ) * 3;
        break;
    case 9:
        if (em->velZ <= 0.0f) {
            em->velZ = 20.0f;
        }
        if (em->remote == 0) {
            em->timer = Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) / em->velZ;
        }
        break;
    case 11:
        Player_ClearVelocity(em);
        em->velZ = 20.0f;
        em->timer = (s32) (Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) / em->velZ) * 3;
        break;
    case 1:
    case 2:
    case 3:
    case 6:
    case 10:
    case 12:
        break;
    }
    Em_SetStateSync(em, EM_GROUP_FLY, state, mode);
}

/*
 * Enters an attack: DIAB_ATK_TURN_AND_STARE, DIAB_ATK_TURN_ATTACK_A,
 * DIAB_ATK_TURN_ATTACK_B, DIAB_ATK_TURN_ATTACK_D face the target
 * (Em_SetTargetAngle); DIAB_ATK_CHARGE goalDist 500 past the goal, or 150
 * frames with no target; DIAB_ATK_CHARGE_ATTACK_A, DIAB_ATK_CHARGE_ATTACK_B
 * goalDist and a timer of dist / 30 + 30 frames, or 150 with no target; then
 * Em_SetStateSync.
 */
void EmDiablos_SetAttackState(Em *em, u16 state, u8 mode)
{
    f32 dist;

    em->motionSpeed = 1.0f;
    switch (state) {
    case DIAB_ATK_TURN_AND_STARE:
    case DIAB_ATK_TURN_ATTACK_A:
    case DIAB_ATK_TURN_ATTACK_B:
    case DIAB_ATK_TURN_ATTACK_D:
        Em_SetTargetAngle(em);
        break;
    case DIAB_ATK_STARE:
    case DIAB_ATK_TAIL_SPIN:
        break;
    case DIAB_ATK_CHARGE:
        if (em->targetNo == -1) {
            em->timer = 150;
            em->hasGoal = 0;
        } else {
            em->hasGoal = 1;
            em->goalDist = 500.0f + Vec_DistanceXZ(&em->posX, em->goal);
        }
        break;
    case DIAB_ATK_CHARGE_ATTACK_A:
    case DIAB_ATK_CHARGE_ATTACK_B:
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
    }
    Em_SetStateSync(em, EM_GROUP_ATTACK, state, mode);
}

/*
 * Moves to (group, state): the owner aims at a player in range, a remote
 * copy ignores EM_SYNC_OWNER and EM_SYNC_OWNER_FLAGGED; loads reaction set
 * 0, runs the group's setter (groups 0-3), marks a versus defeat on
 * DIAB_VS_WAIT_BOTH, calls QuestEm_Appear on DIAB_DEFEAT_REAPPEAR when the
 * quest lets it. The reactions: DIAB_REACT_PART_BREAK sets fxFlags bit 1,
 * counts partBreakCount and ends all statuses; DIAB_REACT_HOLD2 ends all
 * statuses; DIAB_REACT_STATUS4 and _STATUS4_IN_SAND begin status 4;
 * DIAB_REACT_PART_BREAK_END resets the pose and breaks part 2; the sand
 * struggles end status 4, 2 or 5; the stuns begin status 5. Then (falling
 * into EM_GROUP_EVENT's case) Em_SetStateSync. EM_GROUP_EVENT2 sets nothing.
 */
void EmDiablos_ChangeState(Em *em, u16 group, u16 state, u8 mode)
{
    if (em->remote == 0) {
        Em_TargetPlayerInRange(em);
    } else if (mode == EM_SYNC_OWNER || mode == EM_SYNC_OWNER_FLAGGED) {
        return;
    }
    Em_LoadReactionSet(em, 0);
    switch (group) {
    case EM_GROUP_IDLE:
        EmDiablos_SetIdleState(em, state, mode);
        break;
    case EM_GROUP_MOVE:
        EmDiablos_SetMoveState(em, state, mode);
        break;
    case EM_GROUP_FLY:
        EmDiablos_SetBurrowState(em, state, mode);
        break;
    case EM_GROUP_ATTACK:
        EmDiablos_SetAttackState(em, state, mode);
        break;
    case EM_GROUP_VERSUS:
        if (state == 152) {
            Versus_MarkDefeat(em);
        }
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        break;
    case EM_GROUP_DEFEAT:
        if (state == 3 && QuestEm_CanAppear(em) == 1) {
            QuestEm_Appear(em);
        }
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        break;
    case EM_GROUP_REACT:
        switch (state) {
        case DIAB_REACT_PART_BREAK:
            em->fxFlags |= 1;
            em->partBreakCount++;
            Em_EndAllStatus(em);
            break;
        case DIAB_REACT_HOLD2:
            Em_EndAllStatus(em);
            break;
        case DIAB_REACT_PART6_BREAK:
            break;
        case DIAB_REACT_STATUS4:
            Em_BeginStatus4(em);
            break;
        case DIAB_REACT_STATUS4_IN_SAND:
            Em_BeginStatus4(em);
            break;
        case DIAB_REACT_PART_BREAK_END:
            Em_ResetToBasePose(em);
            Em_BreakPart(em, 2);
            break;
        case DIAB_REACT_STATUS4_SAND_STRUGGLE:
            Em_EndStatus4(em);
            break;
        case DIAB_REACT_STATUS2_SAND_GET_UP:
            Em_EndStatus2(em);
            break;
        case DIAB_REACT_STUN:
        case DIAB_REACT_STUN_B:
        case DIAB_REACT_STUN_IN_SAND:
            Em_BeginStatus5(em);
            break;
        case DIAB_REACT_STUN_SAND_STRUGGLE:
            Em_EndStatus5(em);
            break;
        }
    case EM_GROUP_EVENT:
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        break;
    case EM_GROUP_EVENT2:
        break;
    }
}

/*
 * Unless layer 0 is blending, sets velY from the key table
 * EmDiablos_MoveVelYCurves[kind] at the motion's frame (the first key's
 * value on frames 0 and 1, else the slope between the keys around the frame,
 * per motion step) and moves along the heading (Em_MoveRotated); called by
 * two burrow states with kind 1.
 */
void EmDiablos_MoveByVelYCurve(Em *em, s32 kind)
{
    EmWork *work = (EmWork *) &em->work;
    EmCurveKey *keys;
    EmCurveKey *key;
    f32 frame;
    f32 keyTime;
    s32 i;

    keys = EmDiablos_MoveVelYCurves[kind];
    if (keys != NULL && em->motion[0].blend == 0) {
        frame = em->motion[0].frame;
        if (frame == 0.0f || frame == 1.0f) {
            em->velY = keys->v;
        } else {
            i = 1;
            while (i != 0) {
                key = &keys[i];
                keyTime = key->t;
                if (frame > keyTime && keyTime != 0.0f) {
                    i++;
                } else if (keyTime == 0.0f) {
                    break;
                } else {
                    i = 0;
                    em->velY = (key->v - key[-1].v) / ((keyTime - key[-1].t) / em->motion[0].step);
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
 * Starts curve set kind (the tables
 * EmDiablos_VelYCurves/EmDiablos_SpeedCurves): sets the velY flag (kinds 2,
 * 3, 5, 8-13) and the speed flag (5, 8, 9, 10) with the kind's start frame,
 * and clears the velocity; called by two states leaping out of the sand.
 */
void EmDiablos_StartCurves(Em *em, u8 kind)
{
    EmDiablosWork *w = (EmDiablosWork *)&em->work;

    w->curveNone = 0;
    w->curveVelY = 0;
    w->curveSpeed = 0;
    w->curveKind = kind;
    switch (w->curveKind) {
    case 2:
        w->curveVelY = 1;
        w->curveFrame = 70;
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
 * The first curve slot (flag curveNone, never set by EmDiablos_StartCurves):
 * returns 0.
 */
static s32 EmDiablos_CurveNone(Em *em, EmWork *work)
{
    return 0;
}


extern EmCurveKey *EmDiablos_VelYCurves[];

/*
 * Sets velY from EmDiablos_VelYCurves[curveKind] at curveFrame (the slope
 * between the keys around it; nothing on frames 0 and 1); past the last key
 * (time 0) it lets velY fall by 1 a frame down to -50, and a negative key
 * time resets curveFrame and velY to 0; returns 2 in both end cases, else 0.
 */
static u8 EmDiablos_CurveVelY(Em *em, EmWork *work)
{
    EmDiablosWork *w = (EmDiablosWork *) work;
    EmCurveKey *key;
    f32 frame;
    f32 keyTime;
    s32 i;
    EmCurveKey *keys;
    u8 ret;

    keys = EmDiablos_VelYCurves[((EmDiablosWork *)work)->curveKind];
    ret = 0;
    if (keys == NULL) {
        return ret;
    }
    frame = w->curveFrame;
    if (frame != 0.0f && frame != 1.0f) {
        i = 1;
        while (i != 0) {
            key = &keys[i];
            keyTime = key->t;
            if (frame > keyTime && keyTime > 0.0f) {
                i++;
            } else if (keyTime == 0.0f) {
                ret = 2;
                if (em->velY > -50.0f) {
                    em->velY -= 1.0f;
                }
                break;
            } else if (keyTime < 0.0f) {
                w->curveFrame = 0;
                ret = 2;
                em->velY = 0.0f;
                break;
            } else {
                i = 0;
                em->velY = (key->v - key[-1].v) / ((keyTime - key[-1].t) / em->motion[0].step);
            }
        }
    }
    return ret;
}

extern EmCurveKey *EmDiablos_SpeedCurves[];

/*
 * Sets velZ from EmDiablos_SpeedCurves[curveKind] at curveFrame (the slope
 * between the keys around it); returns 4 past the last key (time 0), else 0.
 */
static u8 EmDiablos_CurveSpeed(Em *em, EmWork *work)
{
    EmDiablosWork *w = (EmDiablosWork *) work;
    EmCurveKey *key;
    f32 frame;
    f32 keyTime;
    s32 i;
    EmCurveKey *keys;
    u8 ret;

    keys = EmDiablos_SpeedCurves[((EmDiablosWork *)work)->curveKind];
    ret = 0;
    if (keys == NULL) {
        return ret;
    }
    frame = w->curveFrame;
    if (frame != 0.0f && frame != 1.0f) {
        i = 1;
        while (i != 0) {
            key = &keys[i];
            keyTime = key->t;
            if (frame > keyTime && keyTime != 0.0f) {
                i++;
            } else if (keyTime == 0.0f) {
                ret = 4;
                break;
            } else {
                i = 0;
                em->velZ = (key->v - key[-1].v) / ((keyTime - key[-1].t) / em->motion[0].step);
            }
        }
    }
    return ret;
}

/*
 * Runs the curves whose flags EmDiablos_StartCurves set, advances curveFrame
 * by the motion step and, in the air (EM_STANCE_AIR), moves along the heading
 * with acceleration (Em_MoveRotatedAccel); returns the curves' results or'ed
 * (2 vertical curve done, 4 speed curve done).
 */
u8 EmDiablos_UpdateCurves(Em *em)
{
    EmDiablosWork *w = (EmDiablosWork *)&em->work;
    u8 ret = 0;

    if (w->curveNone != 0) {
        ret = EmDiablos_CurveNone(em, (EmWork *) w);
    }
    if (w->curveVelY != 0) {
        ret |= EmDiablos_CurveVelY(em, (EmWork *) w);
    }
    if (w->curveSpeed != 0) {
        ret |= EmDiablos_CurveSpeed(em, (EmWork *) w);
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

/*
 * Returns 1 when the monster is in the player's area under the sand
 * (EM_STANCE_UNDER) on a floor that is not sand (surfaceKind 0), else 0;
 * Update then ends the state.
 */
s32 EmDiablos_IsBuriedOffSand(Em *em)
{
    if (!((s32)Player_InCurrentArea(em) & 0xFF)) {
        return 0;
    }
    if ((em->surfaceKind == 0) && (em->stance == EM_STANCE_UNDER)) {
        return 1;
    }
    return 0;
}

/*
 * Returns 1 when the charge drives the horn into a wall: part 6 not broken
 * (state < 2), the motion past frame 60, a wall straight ahead (wallHitDirs
 * 0xC0000003) with the special wall flag (wallHitSpecial); the charge states
 * then go to DIAB_IDLE_HORN_STUCK. Else 0.
 */
s32 EmDiablos_ChargeHitWall(Em *em)
{
    if ((s32) em->parts[6].state >= 2) {
        return 0;
    }
    if ((EmMotion_ReachedFrame(em, 0, 60.0f) != 0) && (em->wallHitDirs & 0xC0000003) && ((s8) em->wallHitSpecial != 0)) {
        return 1;
    }
    return 0;
}

/*
 * Returns 1 while layer 0 plays DIAB_MOT_SAND_STRUGGLE, DIAB_MOT_DOWNED or
 * DIAB_MOT_DOWNED_GET_UP (out of the sand already), else 0; the in-sand
 * death and status states then skip the push-out motion.
 */
s32 EmDiablos_InSandStuckMotion(Em *em)
{
    u16 motion = em->motionId[0];

    if ((u32) (motion - EM_MOTION_ID(0, DIAB_MOT_SAND_STRUGGLE)) <= 1 || motion == EM_MOTION_ID(0, DIAB_MOT_DOWNED_GET_UP)) {
        return 1;
    }
    return 0;
}

/*
 * Returns 1 in the states that start from under the sand:
 * DIAB_IDLE_SAND_DOWNED and the reactions DIAB_REACT_POP_OUT, the sand
 * struggles and the downs in the sand. EmDiablos_Update treats them as under
 * the sand when it picks a death or status state, and EmDiablos_AllowHit8
 * refuses the part break in them.
 */
s32 EmDiablos_InSandReactState(Em *em)
{
    u8 stateGroup;

    stateGroup = em->stateGroup;
    if ((stateGroup == EM_GROUP_IDLE && em->state == DIAB_IDLE_SAND_DOWNED) ||
        (stateGroup == EM_GROUP_REACT && em->state == DIAB_REACT_POP_OUT) ||
        (stateGroup == EM_GROUP_REACT && em->state == DIAB_REACT_SAND_STRUGGLE) ||
        (stateGroup == EM_GROUP_REACT && em->state == DIAB_REACT_STATUS4_IN_SAND) ||
        (stateGroup == EM_GROUP_REACT && em->state == DIAB_REACT_STATUS4_SAND_STRUGGLE) ||
        (stateGroup == EM_GROUP_REACT && em->state == DIAB_REACT_STATUS2_SAND_GET_UP) ||
        (stateGroup == EM_GROUP_REACT && em->state == DIAB_REACT_STUN_IN_SAND) ||
        (stateGroup == EM_GROUP_REACT && em->state == DIAB_REACT_STUN_SAND_STRUGGLE)) {
        return 1;
    }
    return 0;
}


/* The file's initialised data, in the game's order */
EmTurnParams EmDiablos_TurnParams[3] = {
#include "game/008C7340.inc"
};
Vec3f EmDiablos_CarveThrowPt = {
#include "game/008C7398.inc"
};
void *EmDiablos_Callbacks[8] = {
    EmDiablos_SpawnEffects, EmDiablos_SpawnEffects, EmDiablos_TableNop, EmDiablos_UpdateMotionEffects,
    GmEm_ReturnZero, EmDiablos_Reinit, EmDiablos_AllowHit8, NULL,
};
EmCurveKey EmDiablos_MoveVelYCurves_0[8] = {
#include "game/008C73D0.inc"
};
EmCurveKey EmDiablos_MoveVelYCurves_1[20] = {
#include "game/008C7410.inc"
};
EmCurveKey EmDiablos_MoveVelYCurves_3[10] = {
#include "game/008C74B0.inc"
};
EmCurveKey EmDiablos_MoveVelYCurves_5[6] = {
#include "game/008C7500.inc"
};
EmCurveKey EmDiablos_MoveVelYCurves_6[10] = {
#include "game/008C7530.inc"
};
EmCurveKey *EmDiablos_MoveVelYCurves[16] = {
    EmDiablos_MoveVelYCurves_0, EmDiablos_MoveVelYCurves_1, NULL, EmDiablos_MoveVelYCurves_3,
    NULL, EmDiablos_MoveVelYCurves_5, EmDiablos_MoveVelYCurves_6, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
};
EmCurveKey EmDiablos_VelYCurves_2[20] = {
#include "game/008C75C0.inc"
};
EmCurveKey EmDiablos_VelYCurves_3[10] = {
#include "game/008C7660.inc"
};
EmCurveKey EmDiablos_VelYCurves_5[14] = {
#include "game/008C76B0.inc"
};
EmCurveKey EmDiablos_VelYCurves_8[8] = {
#include "game/008C7720.inc"
};
EmCurveKey EmDiablos_VelYCurves_9[18] = {
#include "game/008C7760.inc"
};
EmCurveKey EmDiablos_VelYCurves_10[14] = {
#include "game/008C77F0.inc"
};
EmCurveKey EmDiablos_VelYCurves_11[4] = {
#include "game/008C7860.inc"
};
EmCurveKey EmDiablos_VelYCurves_12[8] = {
#include "game/008C7880.inc"
};
EmCurveKey EmDiablos_VelYCurves_13[24] = {
#include "game/008C78C0.inc"
};
EmCurveKey *EmDiablos_VelYCurves[16] = {
    NULL, NULL, EmDiablos_VelYCurves_2, EmDiablos_VelYCurves_3,
    NULL, EmDiablos_VelYCurves_5, NULL, NULL,
    EmDiablos_VelYCurves_8, EmDiablos_VelYCurves_9, EmDiablos_VelYCurves_10, EmDiablos_VelYCurves_11,
    EmDiablos_VelYCurves_12, EmDiablos_VelYCurves_13, NULL, NULL,
};
EmCurveKey EmDiablos_SpeedCurves_5[14] = {
#include "game/008C79C0.inc"
};
EmCurveKey EmDiablos_SpeedCurves_8[8] = {
#include "game/008C7A30.inc"
};
EmCurveKey EmDiablos_SpeedCurves_9[14] = {
#include "game/008C7A70.inc"
};
EmCurveKey EmDiablos_SpeedCurves_10[12] = {
#include "game/008C7AE0.inc"
};
EmCurveKey *EmDiablos_SpeedCurves[16] = {
    NULL, NULL, NULL, NULL,
    NULL, EmDiablos_SpeedCurves_5, NULL, NULL,
    EmDiablos_SpeedCurves_8, EmDiablos_SpeedCurves_9, EmDiablos_SpeedCurves_10, NULL,
    NULL, NULL, NULL, NULL,
};
