/*
 * The Chameleos family (EM_KIND_CHAMELEOS): the elder dragon that walks,
 * flies, attacks with its tongue (which can steal an item), spits balls and
 * breath, and cloaks itself. The file holds its motion sounds and effects,
 * its setup and Init (EmChameleos_Setup, which the per-kind dispatch
 * Em_TypeSetup calls), the state functions of each group with their
 * dispatchers (EmChameleos_Run*State) and the per-frame update
 * (EmChameleos_Update), which also starts the forest scene and the leaving
 * states. The family's state changes, flight curves and cloak helpers come
 * after the Remobra family's code in the game's, and are compiled there.
 *
 * The state functions take the monster and the work area its state
 * dispatcher passes on; Em.step is the step within the current state, and
 * the family keeps its own values in the work area (EmChameleosWork: the
 * route's legs, the flight guard, the curves, the cloak's shimmer).
 * em_chameleos.h names the motions and the states.
 */
#include "common.h"
#include "item.h"
#include "scene.h"
#include "quest.h"
#include "em.h"
#include "shadow.h"
#include "em_chameleos.h"
#include "player.h"
#include "objattach.h"
#include "area.h"
#include "area_draw.h"
#include "capsule.h"
#include "em_bulldrome.h"
#include "fishing.h"
#include "save.h"
#include "sound.h"
#include "cam_shake.h"
#include "engine_math.h"
#include "gm_em.h"
#include "hit_effects.h"
#include "monster_effects.h"
#include "part_effects.h"
#include "shakalaka_teostra_bulldrome.h"
#include "stage_data.h"
#include "spark_fx.h"
#include "vec_math.h"
#include "camera.h"
#include "em_script.h"
#include "hud.h"
/* prototypes of callees */
struct EmIdEntry;
f32 *Em_AreaEntryValue(Em *em, struct EmIdEntry *entry);

/* prototypes */
void * EmEffectAttack_Create(Em *, s32, s32, s32, s32);
/* unit's own declaration: declared here as the callers use it; with the definition's s32 (Em *em, s16 n, s16 max, s16 interval), the calls in EmChameleos_Rest compile differently */
s32 Em_RegenHp(Em *em, s32 n, s32 max, s32 interval);
void EmChameleos_StealItem(Em *em, Player *p);
void AlertMark_Create(void *owner, s16 v, u8 k);
void StunStars_CreateEm(Em *em, u8 k, s16 v, s16 t, f32 y, f32 scale);
s32 EmMotion_PassedFrameScaled(void *p, f32 frame, s32 layer);
extern EmFlyParam EmChameleos_FlyParam;
extern EmFlyParam EmChameleos_FlyParam2;
s32 Em_PartBroken(Em *em, s32 arg1);
void EmTail_Create(Em *);
/* unit's own declaration: declared here as the callers use it; with the definition's void (void *obj, s16 bone, f32 *out), the calls in EmChameleos_SpawnGroundEffect, EmChameleos_SpawnGroundEffectOffset and others compile differently */
void Em_BonePos(void *obj, s32 a, f32 *out);
void Em_AddHp(Em *em, s16 n);
/* unit's own declaration: declared here as the callers use it; with the definition's void (Em *em, s16 k, s16 c, f32 scale), the calls in EmChameleos_SpawnDustNearGround compile differently */
void DustCloud_Create(Em *em, s32, s32, f32);
extern EmTurnParams EmChameleos_TurnParams[1];
/* the calls that mask the value again (u8 or & 0xFF) cast it to s32 first: MWCC keeps the game's andi that way */
s32 Item_IsStealable(s32 arg0);
u8 Pouch_Add(Player *p, u16 item, s16 n);
s32 EmChameleos_CanTurnInvisible(Em *em);
/* end of prototypes */

/*
 * Plays the sounds of Chameleos's motion on layer 0 at its frames
 * (Em_PlaySeAtBoneOnFrame: frame, sound id, bone), switching on the motion
 * id in EmWork.sndMotion; two attack states add theirs in the default case.
 */
void EmChameleos_MotionSounds(Em *em, EmWork *work)
{
    s16 motion;

    motion = work->sndMotion;
    switch (motion) {
    case EM_MOTION_ID(0, CHAM_MOT_IDLE):
        Em_PlaySeAtBoneOnFrame(em, 94, 57, 38);
        Em_PlaySeAtBoneOnFrame(em, 258, 56, 38);
        Em_PlaySeAtBoneOnFrame(em, 386, 49, 38);
        Em_PlaySeAtBoneOnFrame(em, 590, 56, 38);
        break;
    case EM_MOTION_ID(0, CHAM_MOT_WALK):
        Em_PlaySeAtBoneOnFrame(em, 140, 57, 38);
        Em_PlaySeAtBoneOnFrame(em, 128, 1, CHAM_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 284, 1, CHAM_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 298, 4, CHAM_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 138, 4, CHAM_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 120, 3, CHAM_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 42, 3, CHAM_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 142, 3, CHAM_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 328, 3, CHAM_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, CHAM_MOT_RUN):
        Em_PlaySeAtBoneOnFrame(em, 8, 73, 38);
        Em_PlaySeAtBoneOnFrame(em, 50, 2, CHAM_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 70, 2, CHAM_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 42, 5, CHAM_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 62, 5, CHAM_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, CHAM_MOT_RUN_STOP):
        Em_PlaySeAtBoneOnFrame(em, 8, 50, 38);
        Em_PlaySeAtBoneOnFrame(em, 12, 2, CHAM_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 32, 2, CHAM_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 38, 5, CHAM_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, 5):
        Em_PlaySeAtBoneOnFrame(em, 40, 38, 38);
        Em_PlaySeAtBoneOnFrame(em, 80, 2, CHAM_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 112, 1, CHAM_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 66, 3, CHAM_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 148, 4, CHAM_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 10, 23, CHAM_BONE_TAIL_D);
        break;
    case EM_MOTION_ID(0, 6):
        Em_PlaySeAtBoneOnFrame(em, 40, 38, 38);
        Em_PlaySeAtBoneOnFrame(em, 80, 2, CHAM_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 112, 1, CHAM_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 66, 3, CHAM_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 148, 4, CHAM_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 10, 23, CHAM_BONE_TAIL_D);
        break;
    case EM_MOTION_ID(0, CHAM_MOT_SPIN_TURN):
        Em_PlaySeAtBoneOnFrame(em, 8, 49, 38);
        Em_PlaySeAtBoneOnFrame(em, 42, 4, CHAM_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 106, 1, CHAM_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 110, CHAM_SE_THUD, CHAM_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 4, 22, CHAM_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame(em, 100, 23, CHAM_BONE_TAIL_D);
        Em_PlaySeAtBoneOnFrame(em, 150, 4, CHAM_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 186, 4, CHAM_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, CHAM_MOT_BACK_HOP):
        Em_PlaySeAtBoneOnFrame(em, 44, 34, 38);
        Em_PlaySeAtBoneOnFrame(em, 80, 38, 38);
        Em_PlaySeAtBoneOnFrame(em, 16, 1, CHAM_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 26, 1, CHAM_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 46, 20, 2);
        Em_PlaySeAtBoneOnFrame(em, 70, 11, 50);
        Em_PlaySeAtBoneOnFrame(em, 74, 11, 46);
        Em_PlaySeAtBoneOnFrame(em, 84, 2, CHAM_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 88, 1, CHAM_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 90, 3, CHAM_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 92, 4, CHAM_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 138, 1, CHAM_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 156, 1, CHAM_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 160, 4, CHAM_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 150, 4, CHAM_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, CHAM_MOT_TAKE_OFF):
        Em_PlaySeAtBoneOnFrame(em, 4, 32, 38);
        Em_PlaySeAtBoneOnFrame(em, 54, 91, 38);
        Em_PlaySeAtBoneOnFrame(em, 50, 19, 2);
        Em_PlaySeAtBoneOnFrame(em, 50, CHAM_SE_STOMP, 2);
        Em_PlaySeAtBoneOnFrame(em, 24, 1, CHAM_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 34, 3, CHAM_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, 11):
        Em_PlaySeAtBoneOnFrame(em, 40, 91, 38);
        Em_PlaySeAtBoneOnFrame(em, 2, 15, 46);
        Em_PlaySeAtBoneOnFrame(em, 6, 15, 50);
        Em_PlaySeAtBoneOnFrame(em, 50, 28, CHAM_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame(em, 52, 29, 50);
        Em_PlaySeAtBoneOnFrame(em, 58, 29, 46);
        break;
    case EM_MOTION_ID(0, CHAM_MOT_FLY):
        Em_PlaySeAtBoneOnFrame(em, 10, 26, CHAM_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame(em, 14, 25, 50);
        Em_PlaySeAtBoneOnFrame(em, 18, 25, 46);
        Em_PlaySeAtBoneOnFrame(em, 42, 25, CHAM_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame(em, 46, 25, 50);
        Em_PlaySeAtBoneOnFrame(em, 50, 25, 46);
        break;
    case EM_MOTION_ID(0, CHAM_MOT_FLY_START):
        Em_PlaySeAtBoneOnFrame(em, 38, 14, 46);
        Em_PlaySeAtBoneOnFrame(em, 42, 14, 50);
        Em_PlaySeAtBoneOnFrame(em, 2, 26, CHAM_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame(em, 6, 25, 50);
        Em_PlaySeAtBoneOnFrame(em, 8, 25, 46);
        break;
    case EM_MOTION_ID(0, 14):
    case EM_MOTION_ID(0, CHAM_MOT_HOVER_SETTLE):
        Em_PlaySeAtBoneOnFrame(em, 13, 26, CHAM_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame(em, 18, 25, 50);
        Em_PlaySeAtBoneOnFrame(em, 23, 25, 46);
        Em_PlaySeAtBoneOnFrame(em, 44, 25, CHAM_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame(em, 58, 25, 50);
        Em_PlaySeAtBoneOnFrame(em, 65, 25, 46);
        break;
    case EM_MOTION_ID(0, CHAM_MOT_BRAKE):
        Em_PlaySeAtBoneOnFrame(em, 4, 50, 38);
        Em_PlaySeAtBoneOnFrame(em, 12, 13, 46);
        Em_PlaySeAtBoneOnFrame(em, 16, 13, 50);
        Em_PlaySeAtBoneOnFrame(em, 56, 11, 46);
        Em_PlaySeAtBoneOnFrame(em, 60, 11, 50);
        break;
    case EM_MOTION_ID(0, CHAM_MOT_LAND):
        Em_PlaySeAtBoneOnFrame(em, 64, 38, 38);
        Em_PlaySeAtBoneOnFrame(em, 8, 13, 46);
        Em_PlaySeAtBoneOnFrame(em, 12, 13, 50);
        Em_PlaySeAtBoneOnFrame(em, 42, 11, 46);
        Em_PlaySeAtBoneOnFrame(em, 46, 11, 50);
        Em_PlaySeAtBoneOnFrame(em, 102, 12, 46);
        Em_PlaySeAtBoneOnFrame(em, 106, 12, 50);
        Em_PlaySeAtBoneOnFrame(em, 58, CHAM_SE_THUD, 2);
        Em_PlaySeAtBoneOnFrame(em, 40, 2, CHAM_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 44, 2, CHAM_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 16, 5, CHAM_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 20, 5, CHAM_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 140, 1, CHAM_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 178, 1, CHAM_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 160, 1, CHAM_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, CHAM_MOT_HOVER):
        Em_PlaySeAtBoneOnFrame(em, 24, 13, 46);
        Em_PlaySeAtBoneOnFrame(em, 28, 13, 50);
        break;
    case EM_MOTION_ID(0, CHAM_MOT_TAKE_OFF_END):
        Em_PlaySeAtBoneOnFrame(em, 24, 13, 46);
        Em_PlaySeAtBoneOnFrame(em, 28, 13, 50);
        Em_PlaySeAtBoneOnFrame(em, 30, 15, CHAM_BONE_TAIL_D);
        break;
    case EM_MOTION_ID(0, 20):
        Em_PlaySeAtBoneOnFrame(em, 18, 1, CHAM_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 38, 1, CHAM_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 46, 4, CHAM_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 30, 4, CHAM_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, CHAM_MOT_IDLE_ALERT):
        Em_PlaySeAtBoneOnFrame(em, 44, 42, CHAM_BONE_TONGUE);
        Em_PlaySeAtBoneOnFrame(em, 50, 56, CHAM_BONE_TONGUE);
        Em_PlaySeAtBoneOnFrame(em, 156, 49, CHAM_BONE_TONGUE);
        Em_PlaySeAtBoneOnFrame(em, 264, 40, CHAM_BONE_TONGUE);
        Em_PlaySeAtBoneOnFrame(em, 240, 57, CHAM_BONE_TONGUE);
        Em_PlaySeAtBoneOnFrame(em, 412, 56, CHAM_BONE_TONGUE);
        Em_PlaySeAtBoneOnFrame(em, 90, 58, CHAM_BONE_TAIL_D);
        Em_PlaySeAtBoneOnFrame(em, 240, 58, CHAM_BONE_TAIL_D);
        Em_PlaySeAtBoneOnFrame(em, 420, 58, CHAM_BONE_TAIL_D);
        break;
    case EM_MOTION_ID(0, CHAM_MOT_TURN_TO_GOAL):
        Em_PlaySeAtBoneOnFrame(em, 18, 55, 38);
        Em_PlaySeAtBoneOnFrame(em, 62, 55, 38);
        Em_PlaySeAtBoneOnFrame(em, 290, 31, 38);
        Em_PlaySeAtBoneOnFrame(em, 62, 40, CHAM_BONE_TONGUE);
        Em_PlaySeAtBoneOnFrame(em, 134, 42, CHAM_BONE_TONGUE);
        Em_PlaySeAtBoneOnFrame(em, 228, 40, CHAM_BONE_TONGUE);
        Em_PlaySeAtBoneOnFrame(em, 18, 1, CHAM_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 64, 0, CHAM_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 276, 1, CHAM_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 248, 14, 46);
        Em_PlaySeAtBoneOnFrame(em, 252, 14, 50);
        Em_PlaySeAtBoneOnFrame(em, 252, 22, CHAM_BONE_TAIL_D);
        break;
    case EM_MOTION_ID(0, CHAM_MOT_IDLE_ACTION_A):
        Em_PlaySeAtBoneOnFrame(em, 12, 56, 38);
        Em_PlaySeAtBoneOnFrame(em, 60, 49, 38);
        Em_PlaySeAtBoneOnFrame(em, 140, 57, 38);
        break;
    case EM_MOTION_ID(0, CHAM_MOT_CLOAK):
        Em_PlaySeAtBoneOnFrame(em, 16, 91, 38);
        Em_PlaySeAtBoneOnFrame(em, 36, 1, CHAM_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 88, 1, CHAM_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 90, 4, CHAM_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 48, 4, CHAM_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 38, 12, 46);
        Em_PlaySeAtBoneOnFrame(em, 42, 12, 50);
        break;
    case EM_MOTION_ID(0, CHAM_MOT_LIMP):
        Em_PlaySeAtBoneOnFrame(em, 84, 88, 38);
        Em_PlaySeAtBoneOnFrame(em, 4, 88, 38);
        Em_PlaySeAtBoneOnFrame(em, 76, 40, CHAM_BONE_TONGUE);
        Em_PlaySeAtBoneOnFrame(em, 10, 0, CHAM_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 40, 1, CHAM_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 78, 1, CHAM_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 4, 3, CHAM_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 50, 4, CHAM_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, CHAM_MOT_REST):
        Em_PlaySeAtBoneOnFrame(em, 16, 93, 38);
        Em_PlaySeAtBoneOnFrame(em, 158, 38, 38);
        Em_PlaySeAtBoneOnFrame(em, 288, 92, 38);
        Em_PlaySeAtBoneOnFrame(em, 108, 1, CHAM_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 58, 4, CHAM_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 136, 3, CHAM_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 154, 23, CHAM_BONE_TAIL_D);
        break;
    case EM_MOTION_ID(0, CHAM_MOT_REST_END):
        Em_PlaySeAtBoneOnFrame(em, 16, 95, 38);
        Em_PlaySeAtBoneOnFrame(em, 126, 40, 38);
        Em_PlaySeAtBoneOnFrame(em, 116, 1, CHAM_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 98, 4, CHAM_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 168, 4, CHAM_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 64, 14, 46);
        Em_PlaySeAtBoneOnFrame(em, 68, 14, 50);
        Em_PlaySeAtBoneOnFrame(em, 86, 10, CHAM_BONE_TAIL_D);
        break;
    case EM_MOTION_ID(0, CHAM_MOT_TONGUE_LOW):
        Em_PlaySeAtBoneOnFrame(em, 4, 32, 38);
        Em_PlaySeAtBoneOnFrame(em, 148, 38, 38);
        Em_PlaySeAtBoneOnFrame(em, 14, 0, CHAM_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 56, 1, CHAM_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 18, 4, CHAM_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 180, 1, CHAM_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 174, 3, CHAM_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 202, 3, CHAM_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 230, 1, CHAM_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 74, CHAM_SE_TONGUE_STRIKE, CHAM_BONE_TONGUE);
        Em_PlaySeAtBoneOnFrame(em, 78, 41, CHAM_BONE_TONGUE);
        Em_PlaySeAtBoneOnFrame(em, 106, CHAM_SE_TONGUE_STRIKE, CHAM_BONE_TONGUE);
        Em_PlaySeAtBoneOnFrame(em, 110, 41, CHAM_BONE_TONGUE);
        Em_PlaySeAtBoneOnFrame(em, 128, 40, CHAM_BONE_TONGUE);
        break;
    case EM_MOTION_ID(0, CHAM_MOT_SPIT_START):
        Em_PlaySeAtBoneOnFrame(em, 4, 73, 38);
        Em_PlaySeAtBoneOnFrame(em, 12, 0, CHAM_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 4, 3, CHAM_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 36, 4, CHAM_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, CHAM_MOT_SPIT_A):
        if ((em->stateGroup == EM_GROUP_ATTACK) && (em->state == 6)) {
            Em_PlaySeAtBoneOnFrame(em, 12, 24, 38);
            Em_PlaySeAtBoneOnFrame(em, 4, 75, 38);
            Em_PlaySeAtBoneOnFrame(em, 2, 2, CHAM_BONE_FOOT_D);
            Em_PlaySeAtBoneOnFrame(em, 6, 2, CHAM_BONE_FOOT_C);
            break;
        }
        Em_PlaySeAtBoneOnFrame(em, 4, 79, 38);
        Em_PlaySeAtBoneOnFrame(em, 6, 61, 38);
        Em_PlaySeAtBoneOnFrame(em, 2, 2, CHAM_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 6, 2, CHAM_BONE_FOOT_C);
        break;
    case EM_MOTION_ID(0, CHAM_MOT_SPIT_A_END):
        Em_PlaySeAtBoneOnFrame(em, 20, 78, 38);
        Em_PlaySeAtBoneOnFrame(em, 12, 4, 46);
        Em_PlaySeAtBoneOnFrame(em, 16, CHAM_SE_POUND, 50);
        Em_PlaySeAtBoneOnFrame(em, 122, 1, CHAM_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 4, 22, CHAM_BONE_TAIL_D);
        break;
    case EM_MOTION_ID(0, CHAM_MOT_STOMP):
        Em_PlaySeAtBoneOnFrame(em, 12, 31, 38);
        Em_PlaySeAtBoneOnFrame(em, 42, 74, 38);
        Em_PlaySeAtBoneOnFrame(em, 44, CHAM_SE_STOMP, CHAM_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 134, 1, CHAM_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 10, 3, CHAM_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, CHAM_MOT_TAIL_SLAM):
        Em_PlaySeAtBoneOnFrame(em, 4, 33, 38);
        Em_PlaySeAtBoneOnFrame(em, 76, 75, 38);
        Em_PlaySeAtBoneOnFrame(em, 150, 37, 38);
        Em_PlaySeAtBoneOnFrame(em, 86, CHAM_SE_TAIL_SLAM, CHAM_BONE_TAIL_D);
        Em_PlaySeAtBoneOnFrame(em, 100, 22, CHAM_BONE_TAIL_D);
        Em_PlaySeAtBoneOnFrame(em, 4, 0, CHAM_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 38, 0, CHAM_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 32, 0, CHAM_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 144, 0, CHAM_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, CHAM_MOT_BACK_HOP_BALL):
        Em_PlaySeAtBoneOnFrame(em, 24, 31, 38);
        Em_PlaySeAtBoneOnFrame(em, 88, 75, 38);
        Em_PlaySeAtBoneOnFrame(em, 90, 24, 38);
        Em_PlaySeAtBoneOnFrame(em, 4, 3, CHAM_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 22, 5, CHAM_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 70, 1, CHAM_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 74, 1, CHAM_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 102, 11, 46);
        Em_PlaySeAtBoneOnFrame(em, 106, 11, 50);
        break;
    case EM_MOTION_ID(0, CHAM_MOT_TAIL_POUNDS):
        Em_PlaySeAtBoneOnFrame(em, 64, 56, 38);
        Em_PlaySeAtBoneOnFrame(em, 164, 55, 38);
        Em_PlaySeAtBoneOnFrame(em, 70, CHAM_SE_POUND, CHAM_BONE_TAIL_D);
        Em_PlaySeAtBoneOnFrame(em, 162, CHAM_SE_POUND, CHAM_BONE_TAIL_D);
        Em_PlaySeAtBoneOnFrame(em, 258, CHAM_SE_POUND, CHAM_BONE_TAIL_D);
        Em_PlaySeAtBoneOnFrame(em, 356, CHAM_SE_POUND, CHAM_BONE_TAIL_D);
        Em_PlaySeAtBoneOnFrame(em, 54, 13, CHAM_BONE_TAIL_D);
        Em_PlaySeAtBoneOnFrame(em, 148, 13, CHAM_BONE_TAIL_D);
        Em_PlaySeAtBoneOnFrame(em, 242, 13, CHAM_BONE_TAIL_D);
        Em_PlaySeAtBoneOnFrame(em, 342, 13, CHAM_BONE_TAIL_D);
        break;
    case EM_MOTION_ID(0, CHAM_MOT_TONGUE):
        Em_PlaySeAtBoneOnFrame(em, 4, 32, 38);
        Em_PlaySeAtBoneOnFrame(em, 38, CHAM_SE_TONGUE_STRIKE, CHAM_BONE_TONGUE);
        Em_PlaySeAtBoneOnFrame(em, 40, 41, CHAM_BONE_TONGUE);
        Em_PlaySeAtBoneOnFrame(em, 48, 40, CHAM_BONE_TONGUE);
        break;
    case EM_MOTION_ID(0, CHAM_MOT_TONGUE_B):
        Em_PlaySeAtBoneOnFrame(em, 4, 32, 38);
        Em_PlaySeAtBoneOnFrame(em, 38, CHAM_SE_TONGUE_STRIKE, CHAM_BONE_TONGUE);
        Em_PlaySeAtBoneOnFrame(em, 40, 41, CHAM_BONE_TONGUE);
        Em_PlaySeAtBoneOnFrame(em, 48, 40, CHAM_BONE_TONGUE);
        Em_PlaySeAtBoneOnFrame(em, 20, CHAM_SE_STOMP, CHAM_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 76, 1, CHAM_BONE_FOOT_D);
        break;
    case EM_MOTION_ID(0, CHAM_MOT_TONGUE_HIGH):
        Em_PlaySeAtBoneOnFrame(em, 4, 32, 38);
        Em_PlaySeAtBoneOnFrame(em, 30, CHAM_SE_TONGUE_STRIKE, CHAM_BONE_TONGUE);
        Em_PlaySeAtBoneOnFrame(em, 34, 41, CHAM_BONE_TONGUE);
        Em_PlaySeAtBoneOnFrame(em, 48, 40, CHAM_BONE_TONGUE);
        break;
    case EM_MOTION_ID(0, CHAM_MOT_SPIT_B):
        if ((em->stateGroup == EM_GROUP_ATTACK) && (em->state == 7)) {
            Em_PlaySeAtBoneOnFrame(em, 12, 24, 38);
            Em_PlaySeAtBoneOnFrame(em, 4, 75, 38);
            Em_PlaySeAtBoneOnFrame(em, 2, 2, CHAM_BONE_FOOT_D);
            Em_PlaySeAtBoneOnFrame(em, 6, 2, CHAM_BONE_FOOT_C);
            break;
        }
        Em_PlaySeAtBoneOnFrame(em, 4, 79, 38);
        Em_PlaySeAtBoneOnFrame(em, 6, 61, 38);
        Em_PlaySeAtBoneOnFrame(em, 2, 2, CHAM_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 6, 2, CHAM_BONE_FOOT_C);
        break;
    case EM_MOTION_ID(0, CHAM_MOT_SPIT_B_END):
        Em_PlaySeAtBoneOnFrame(em, 20, 38, 38);
        Em_PlaySeAtBoneOnFrame(em, 12, 4, 46);
        Em_PlaySeAtBoneOnFrame(em, 16, CHAM_SE_POUND, 50);
        Em_PlaySeAtBoneOnFrame(em, 122, 1, CHAM_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 4, 22, CHAM_BONE_TAIL_D);
        break;
    case EM_MOTION_ID(0, CHAM_MOT_FLINCH_B):
        Em_PlaySeAtBoneOnFrame(em, 4, 83, 38);
        Em_PlaySeAtBoneOnFrame(em, 100, 46, 38);
        Em_PlaySeAtBoneOnFrame(em, 4, 22, 2);
        Em_PlaySeAtBoneOnFrame(em, 124, 0, CHAM_BONE_FOOT_C);
        break;
    case EM_MOTION_ID(0, CHAM_MOT_RECOVER):
        Em_PlaySeAtBoneOnFrame(em, 4, 84, 38);
        Em_PlaySeAtBoneOnFrame(em, 130, 38, 38);
        Em_PlaySeAtBoneOnFrame(em, 74, CHAM_SE_THUD, 38);
        Em_PlaySeAtBoneOnFrame(em, 56, 2, CHAM_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 66, 2, CHAM_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 50, 18, CHAM_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 100, 5, CHAM_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 4, 13, 46);
        Em_PlaySeAtBoneOnFrame(em, 8, 13, 50);
        Em_PlaySeAtBoneOnFrame(em, 94, 10, 46);
        Em_PlaySeAtBoneOnFrame(em, 98, 10, 50);
        break;
    case EM_MOTION_ID(0, CHAM_MOT_FLINCH_A):
        Em_PlaySeAtBoneOnFrame(em, 4, 84, 38);
        Em_PlaySeAtBoneOnFrame(em, 180, 46, 38);
        Em_PlaySeAtBoneOnFrame(em, 130, 40, CHAM_BONE_TONGUE);
        Em_PlaySeAtBoneOnFrame(em, 176, 42, CHAM_BONE_TONGUE);
        Em_PlaySeAtBoneOnFrame(em, 4, 23, CHAM_BONE_TAIL_D);
        Em_PlaySeAtBoneOnFrame(em, 30, 2, CHAM_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 82, 1, CHAM_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 28, 3, CHAM_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 88, 4, CHAM_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 4, 13, 46);
        Em_PlaySeAtBoneOnFrame(em, 8, 13, 50);
        Em_PlaySeAtBoneOnFrame(em, 42, 11, 46);
        Em_PlaySeAtBoneOnFrame(em, 46, 11, 50);
        Em_PlaySeAtBoneOnFrame(em, 94, 11, 46);
        Em_PlaySeAtBoneOnFrame(em, 98, 11, 50);
        break;
    case EM_MOTION_ID(0, CHAM_MOT_FALL):
        Em_PlaySeAtBoneOnFrame(em, 4, 85, 38);
        Em_PlaySeAtBoneOnFrame(em, 6, 11, 46);
        Em_PlaySeAtBoneOnFrame(em, 10, 11, 50);
        break;
    case EM_MOTION_ID(0, CHAM_MOT_FALL_LAND):
        Em_PlaySeAtBoneOnFrame(em, 42, 40, CHAM_BONE_TONGUE);
        Em_PlaySeAtBoneOnFrame(em, 8, 9, CHAM_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame(em, 24, CHAM_SE_THUD, 2);
        Em_PlaySeAtBoneOnFrame(em, 26, 15, 50);
        break;
    case EM_MOTION_ID(0, CHAM_MOT_DOWN):
        Em_PlaySeAtBoneOnFrame(em, 24, 87, 38);
        Em_PlaySeAtBoneOnFrame(em, 38, 16, 38);
        Em_PlaySeAtBoneOnFrame(em, 26, 23, 38);
        Em_PlaySeAtBoneOnFrame(em, 60, 42, CHAM_BONE_TONGUE);
        break;
    case EM_MOTION_ID(0, CHAM_MOT_GET_UP):
        Em_PlaySeAtBoneOnFrame(em, 22, 40, CHAM_BONE_TONGUE);
        Em_PlaySeAtBoneOnFrame(em, 50, 86, CHAM_BONE_TONGUE);
        Em_PlaySeAtBoneOnFrame(em, 4, 22, 38);
        Em_PlaySeAtBoneOnFrame(em, 2, 18, CHAM_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 22, 1, CHAM_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 130, 0, CHAM_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 74, 2, CHAM_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 114, 4, CHAM_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 134, 3, CHAM_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 4, 11, 50);
        Em_PlaySeAtBoneOnFrame(em, 82, 14, 46);
        Em_PlaySeAtBoneOnFrame(em, 86, 14, 50);
        break;
    case EM_MOTION_ID(0, CHAM_MOT_DIE):
        Em_PlaySeAtBoneOnFrame(em, 4, 85, 38);
        Em_PlaySeAtBoneOnFrame(em, 70, 40, CHAM_BONE_TONGUE);
        Em_PlaySeAtBoneOnFrame(em, 112, 47, 38);
        Em_PlaySeAtBoneOnFrame(em, 190, 53, 38);
        Em_PlaySeAtBoneOnFrame(em, 366, 42, CHAM_BONE_TONGUE);
        Em_PlaySeAtBoneOnFrame(em, 400, 44, CHAM_BONE_TONGUE);
        Em_PlaySeAtBoneOnFrame(em, 96, 23, 2);
        Em_PlaySeAtBoneOnFrame(em, 192, 22, 2);
        Em_PlaySeAtBoneOnFrame(em, 18, 0, CHAM_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 12, 1, CHAM_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 24, 3, CHAM_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 434, 15, 46);
        Em_PlaySeAtBoneOnFrame(em, 438, 15, 50);
        Em_PlaySeAtBoneOnFrame(em, 472, CHAM_SE_THUD, 2);
        break;
    case EM_MOTION_ID(0, CHAM_MOT_STATUS4):
        Em_PlaySeAtBoneOnFrame(em, 4, 83, 38);
        Em_PlaySeAtBoneOnFrame(em, 40, 87, 38);
        Em_PlaySeAtBoneOnFrame(em, 12, 22, 38);
        Em_PlaySeAtBoneOnFrame(em, 18, 0, CHAM_BONE_FOOT_C);
        break;
    case EM_MOTION_ID(0, CHAM_MOT_SHAKE_OFF):
        Em_PlaySeAtBoneOnFrame(em, 10, 95, 38);
        Em_PlaySeAtBoneOnFrame(em, 24, 91, 38);
        Em_PlaySeAtBoneOnFrame(em, 126, 38, 38);
        Em_PlaySeAtBoneOnFrame(em, 30, 22, 2);
        Em_PlaySeAtBoneOnFrame(em, 4, 13, CHAM_BONE_TAIL_D);
        Em_PlaySeAtBoneOnFrame(em, 22, 13, 46);
        Em_PlaySeAtBoneOnFrame(em, 26, 13, 50);
        Em_PlaySeAtBoneOnFrame(em, 84, 14, 46);
        Em_PlaySeAtBoneOnFrame(em, 88, 14, 50);
        break;
    case EM_MOTION_ID(0, CHAM_MOT_STATUS_DOWN):
        Em_PlaySeAtBoneOnFrame(em, 4, 83, 38);
        Em_PlaySeAtBoneOnFrame(em, 78, 93, 38);
        Em_PlaySeAtBoneOnFrame(em, 194, 42, CHAM_BONE_TONGUE);
        Em_PlaySeAtBoneOnFrame(em, 70, 23, 38);
        Em_PlaySeAtBoneOnFrame(em, 252, 52, 38);
        Em_PlaySeAtBoneOnFrame(em, 4, 0, CHAM_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 18, 1, CHAM_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 18, 4, CHAM_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 42, 18, CHAM_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 160, 16, 2);
        Em_PlaySeAtBoneOnFrame(em, 184, CHAM_SE_THUD, 2);
        Em_PlaySeAtBoneOnFrame(em, 4, 14, 46);
        Em_PlaySeAtBoneOnFrame(em, 8, 14, 50);
        Em_PlaySeAtBoneOnFrame(em, 168, 11, 50);
        break;
    case EM_MOTION_ID(0, CHAM_MOT_COLLAPSE):
        Em_PlaySeAtBoneOnFrame(em, 4, 85, 38);
        Em_PlaySeAtBoneOnFrame(em, 130, 44, 38);
        Em_PlaySeAtBoneOnFrame(em, 66, 40, CHAM_BONE_TONGUE);
        Em_PlaySeAtBoneOnFrame(em, 112, 42, CHAM_BONE_TONGUE);
        Em_PlaySeAtBoneOnFrame(em, 226, 42, CHAM_BONE_TONGUE);
        Em_PlaySeAtBoneOnFrame(em, 24, 23, 38);
        Em_PlaySeAtBoneOnFrame(em, 16, 13, 46);
        Em_PlaySeAtBoneOnFrame(em, 20, 13, 50);
        Em_PlaySeAtBoneOnFrame(em, 98, 12, 46);
        Em_PlaySeAtBoneOnFrame(em, 102, 12, 50);
        Em_PlaySeAtBoneOnFrame(em, 168, CHAM_SE_THUD, 2);
        Em_PlaySeAtBoneOnFrame(em, 182, 9, CHAM_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame(em, 16, 1, CHAM_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 30, 1, CHAM_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 160, 2, CHAM_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 68, 4, CHAM_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 140, 16, CHAM_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, CHAM_MOT_PART_BREAK):
        Em_PlaySeAtBoneOnFrame(em, 4, 83, 38);
        Em_PlaySeAtBoneOnFrame(em, 6, 40, CHAM_BONE_TONGUE);
        Em_PlaySeAtBoneOnFrame(em, 10, 22, 38);
        Em_PlaySeAtBoneOnFrame(em, 56, CHAM_SE_THUD, 2);
        Em_PlaySeAtBoneOnFrame(em, 60, 16, 2);
        Em_PlaySeAtBoneOnFrame(em, 76, 9, CHAM_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame(em, 8, 13, 46);
        Em_PlaySeAtBoneOnFrame(em, 12, 13, 50);
        break;
    case EM_MOTION_ID(0, CHAM_MOT_PART_BREAK_SPIN):
        Em_PlaySeAtBoneOnFrame(em, 4, 95, 38);
        Em_PlaySeAtBoneOnFrame(em, 4, 42, CHAM_BONE_TONGUE);
        Em_PlaySeAtBoneOnFrame(em, 56, 51, 38);
        Em_PlaySeAtBoneOnFrame(em, 12, 23, 38);
        Em_PlaySeAtBoneOnFrame(em, 4, 14, 50);
        Em_PlaySeAtBoneOnFrame(em, 96, 14, 46);
        Em_PlaySeAtBoneOnFrame(em, 100, 14, 50);
        Em_PlaySeAtBoneOnFrame(em, 122, 2, CHAM_BONE_FOOT_C);
        Em_PlaySeAtBoneOnFrame(em, 138, 0, CHAM_BONE_FOOT_D);
        Em_PlaySeAtBoneOnFrame(em, 84, 4, CHAM_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 152, 4, CHAM_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 106, 4, CHAM_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 190, 4, CHAM_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 166, 1, CHAM_BONE_FOOT_D);

    case EM_MOTION_ID(0, 76):
    default:
        break;
    }
}

/*
 * Per-kind setup (Em_TypeSetup): allows the full cloak
 * (EmChameleos_SetFullCloak).
 */
void EmChameleos_Setup(Em *em)
{
    EmChameleos_SetFullCloak(em);
}

/* Creates the alert mark (AlertMark_Create) and effect 01 on the monster. */
void EmChameleos_SpawnEffects(void *em)
{
    AlertMark_Create(em, 34, 0);
    Shadow_Spawn((Player *) em, SHADOW_MODEL);
}

/*
 * Scales parts 3, 4 and 8, places the monster in a row when no quest runs,
 * plays CHAM_MOT_IDLE on the ground and sets hp (Em_ComputeMaxHp, or the
 * quest's persistent hp for a special monster). Sets the flight steps and
 * invisibility work (invisFade none, shimmerTimer 900, wantInvisible 2),
 * allows the full cloak, and creates its horn object (EmTail_Create) unless
 * part 1 is already broken, in which case it marks the horn broken (fxFlags
 * 1, partBreakCount 1).
 */
void EmChameleos_Init(Em *em)
{
    EmChameleosWork *w = (EmChameleosWork *)&em->work;

    f32 fslot;
    s16 maxHp;
    u8 slotNo;

    Em_SetPartScaled(em, 3);
    Em_SetPartScaled(em, 4);
    Em_SetPartScaled(em, 8);
    if (gQuest.stage == 0) {
        em->rotY = 0;
        slotNo = em->slotNo;
        fslot = (f32) (u32) slotNo;
        em->posX = 5000.0f + (1500.0f * fslot);
        em->posY = 0.0f;
        em->posZ = 9000.0f;
    }
    Em_SetMotionRow(em, CHAM_MOT_IDLE, 0, 0);
    em->stance = EM_STANCE_GROUND;
    if ((Quest_FindSpecialMonster(em->type) & 0xFF) == 255) {
        maxHp = Em_ComputeMaxHp(em, 0);
        em->hp = maxHp;
        em->hpMax = maxHp;
    } else {
        QuestEm_LoadPersistentHp(em);
    }
    em->scriptPending = 1;
    em->blind = 0;
    em->unk_InitFlag = 1;
    em->aimAngle = ANGLE_TURN / 4;
    w->moveAngY = 0x100;
    w->moveAngZ = 0x200;
    w->pitchStep = 0x100;
    w->unk018 = 8192;
    EmChameleos_SetFullCloak(em);
    w->unk014 = 0;
    w->invisFade = 65535;
    w->shimmerTimer = 900;
    w->wantInvisible = 2;
    em->meshShown[1] = 0;
    if (Em_PartBroken(em, 1) == 0) {
        EmTail_Create(em);
        return;
    }
    em->fxFlags |= 1;
    em->partBreakCount = 1;
}

/* Init, then CHAM_IDLE (or CHAM_IDLE_ALERT in combat). */
void EmChameleos_Start(Em *em)
{
    EmChameleos_Init(em);
    EmChameleos_ChangeState(em, EM_GROUP_IDLE, CHAM_IDLE, EM_SYNC_LOCAL);
}

/* Init only. */
void EmChameleos_Reinit(Em *em)
{
    EmChameleos_Init(em);
}

/*
 * Ends a ground state: clears lookAtTarget, asks the script for the next
 * state (scriptPending), speed 1, stance ground, and enters CHAM_IDLE.
 */
void EmChameleos_EndState(Em *em, s32 unused1, s32 unused2)
{
    em->lookAtTarget = 0;
    em->scriptPending = 1;
    em->motionSpeed = 1.0f;
    em->stance = EM_STANCE_GROUND;
    EmChameleos_ChangeState(em, EM_GROUP_IDLE, CHAM_IDLE, EM_SYNC_LOCAL);
}

/*
 * Ends an air state: asks the script for the next state, stance air, and
 * enters CHAM_FLY_HOVER (next 0), CHAM_FLY_FORWARD (next 1) or
 * CHAM_FLY_NONE_A (next 2).
 */
void EmChameleos_EndAirState(Em *em, u8 nextKind)
{
    s32 next;

    next = nextKind & 0xFF;
    em->lookAtTarget = 0;
    em->scriptPending = 1;
    em->motionSpeed = 1.0f;
    em->stance = EM_STANCE_AIR;
    switch (next) {
    case 0:
        EmChameleos_ChangeState(em, EM_GROUP_FLY, CHAM_FLY_HOVER, EM_SYNC_LOCAL);
        break;
    case 1:
        EmChameleos_ChangeState(em, EM_GROUP_FLY, CHAM_FLY_FORWARD, EM_SYNC_LOCAL);
        break;
    case 2:
        EmChameleos_ChangeState(em, EM_GROUP_FLY, CHAM_FLY_NONE_A, EM_SYNC_LOCAL);
        break;
    }
}

/*
 * The tongue's theft: for the local player, while the quest runs and
 * without SKILL_THEFT_GUARD, takes one of a random pouch item of type 0 and rarity
 * below 4, preferring items Item_IsStealable accepts, and shows the
 * stolen-item notice. Called by the tongue attacks when EmChameleos's
 * tongue-hit test (EmChameleos_TongueHitPlayer) passes, with Em.contact.
 */
void EmChameleos_StealItem(Em *em, Player *pl)
{
    s32 i;
    s32 nStealable;
    s32 nOther;
    s16 item;
    s16 count;
    s32 stealable[20];
    s32 other[20];

    if (pl->index == gScene.localPlayer && Quest_GetOutcome(1) == 0 && Player_HasSkill(pl, SKILL_THEFT_GUARD) != 1) {
        nStealable = 0;
        nOther = 0;
        for (i = 0; i < 20; i++) {
            if (pl->pouch[i].item != 0 && gItemData[pl->pouch[i].item].type == 0 && gItemData[pl->pouch[i].item].rarity < 4) {
                if (Item_IsStealable(pl->pouch[i].item) == 1) {
                    stealable[nStealable++] = i;
                } else {
                    other[nOther++] = i;
                }
            }
        }
        if (nStealable > 0) {
            item = pl->pouch[stealable[em->stateRand % nStealable]].item;
            count = -1;
        } else if (nOther > 0) {
            item = pl->pouch[other[em->stateRand % nOther]].item;
            count = -1;
        } else {
            return;
        }
        Pouch_Add(pl, item, count);
        if (pl->index == gScene.localPlayer) {
            Notice_Push(1, 6, item);
        }
    }
}

/*
 * CHAM_IDLE_STAND: keeps layers 0 and 1 on their idle loops
 * (EM_MOTION_ID(0,1), EM_MOTION_ID(1,1)) looking at the target until the
 * timer runs out, then asks the script for the next state.
 */
void EmChameleos_Stand(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        if (em->motionId[0] != EM_MOTION_ID(0, CHAM_MOT_IDLE)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(0, CHAM_MOT_IDLE), 10, 0, 0);
        }
        if (em->motionId[1] != EM_MOTION_ID(1, CHAM_MOT_IDLE)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(1, CHAM_MOT_IDLE), 10, 0, 1);
        }
        em->lookAtTarget = 1;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->scriptPending = 1;
        }
        break;
    }
}

/*
 * CHAM_IDLE: plays CHAM_MOT_IDLE once looking at the target, then the owner
 * ends the state (EmChameleos_EndState).
 */
void EmChameleos_Idle(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 1;
        Em_SetMotion(em, CHAM_MOT_IDLE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
        case 2:
            if (em->remote == 0) {
                em->step++;
                EmChameleos_EndState(em, 0, 0);
            }
        }
        break;
    }
}

/*
 * CHAM_IDLE_ALERT: CHAM_IDLE in combat (EmChameleos_SetIdleState's
 * choice); plays CHAM_MOT_IDLE_ALERT once, then the owner ends the state.
 */
void EmChameleos_IdleAlert(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 1;
        Em_SetMotion(em, CHAM_MOT_IDLE_ALERT, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
        case 2:
            if (em->remote == 0) {
                em->step++;
                EmChameleos_EndState(em, 0, 0);
            }
        }
        break;
    }
}

/*
 * CHAM_IDLE_FACE_GOAL: plays CHAM_MOT_TURN_TO_GOAL aimed at the goal,
 * snapping rotY onto the aim when within 0x200, and ends when layer 1's
 * motion stops.
 */
void EmChameleos_FaceGoal(Em *em)
{
    u16 aimAngle;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, CHAM_MOT_TURN_TO_GOAL, 0, 0);
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
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * CHAM_IDLE_CLOAK_FULL: entered with EmChameleos_SetFullCloak (the full
 * cloak allowed); plays CHAM_MOT_CLOAK once and ends.
 */
void EmChameleos_CloakFull(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, CHAM_MOT_CLOAK, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * CHAM_IDLE_CLOAK_SHIMMER: entered with EmChameleos_ClearFullCloak (only the
 * shimmering cloak); plays CHAM_MOT_CLOAK once and ends. Same body as
 * EmChameleos_CloakFull.
 */
void EmChameleos_CloakShimmer(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, CHAM_MOT_CLOAK, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
}

/* CHAM_IDLE_ACTION_A: plays CHAM_MOT_IDLE_ACTION_A once and ends. */
void EmChameleos_IdleActionA(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, CHAM_MOT_IDLE_ACTION_A, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * CHAM_IDLE_EAT_NEED1 (variant 0, one CHAM_MOT_TONGUE) and
 * CHAM_IDLE_EAT_NEED2 (variant 1, three, each with effect 08 and a sound on
 * the floor 200 ahead): eats with the tongue (no flinch, sight row 1,
 * reaction set 1), then gains 5% hp, refills need 1 or need 2 and ends.
 */
void EmChameleos_Eat(Em *em, EmWork *work, u8 variant)
{
    f32 pos[3];
    f32 ofs[3];

    Em_SetHitReactMode(em, EM_HIT_REACT_NO_FLINCH);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, CHAM_MOT_TONGUE, 0, 0);
        Em_SelectSightRow(em, 1);
        Em_LoadReactionSet(em, 1);
        switch (variant) {
        case 0:
        default:
            em->timer = 1;
            break;
        case 1:
            em->timer = 3;
            break;
        }
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 36.0f, 0) != 0) {
            switch (variant) {
            default:
                EmChameleos_SpawnGroundEffect(em, CHAM_BONE_TONGUE, 20, 4.0f);
                break;
            case 1:
                if (Player_InCurrentArea(em)) {
                    ofs[0] = 0.0f;
                    ofs[1] = 0.0f;
                    ofs[2] = 200.0f * em->scaleZ;
                    Em_BonePos(em, CHAM_BONE_TONGUE, pos);
                    Vec_RotateY(ofs, 6.2831855f * em->rotY / 65536.0f);
                    pos[0] += ofs[0];
                    pos[1] = 5.0f + em->floorY;
                    pos[2] += ofs[2];
                    SparkFx_Spawn(pos, 0, 0, 2.0f);
                    Sound_PlaySeAtPosChecked(1, 46, 0, pos, 1, 0);
                }
                break;
            }
        }
        if (em->motion[0].playing == 0) {
            if (--em->timer <= 0) {
                em->step++;
                Em_AddHp(em, 0.05f * em->hpMax);
                switch (variant) {
                default:
                    Em_AddNeed1(em, em->need1Max);
                    Em_ClearNeed1State(em);
                    break;
                case 1:
                    Em_AddNeed2(em, em->need2Max);
                    Em_ClearNeed2State(em);
                    break;
                }
                Em_SelectSightRow(em, 0);
                Em_LoadReactionSet(em, 0);
                EmChameleos_EndState(em, 0, 0);
            } else {
                Em_SetMotion(em, CHAM_MOT_TONGUE, 0, 0);
            }
        }
        break;
    }
}

/*
 * CHAM_IDLE_REST: lies resting (CHAM_MOT_REST, blind, a hit wakes it) for
 * 9000 frames regenerating hp up to half (Em_RegenHp) with the periodic
 * puffs, then clears the low-hp need and enters CHAM_IDLE_REST_END;
 * EM_DMG_HIT_LYING ends it the same way (Update).
 */
void EmChameleos_Rest(Em *em)
{
    em->blind = 2;
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, CHAM_MOT_REST, 0, 0);
        Em_LoadReactionSet(em, 1);
        em->timer = 9000;
        break;
    case 1:
        EmChameleos_SpawnLyingPuffs(em);
        Em_RegenHp(em, 1, (s32) (0.5f * (f32) em->hpMax), 6);
        if (--em->timer <= 0) {
            em->step++;
            Em_ClearLowHpNeed(em);
            EmChameleos_ChangeState(em, EM_GROUP_IDLE, CHAM_IDLE_REST_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmChameleos_ChangeState(em, EM_GROUP_IDLE, CHAM_IDLE_REST_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * CHAM_IDLE_STATUS2_DOWN: under status 2 (entered with Em_BeginStatus2),
 * lies (CHAM_MOT_STATUS_DOWN, blind, a hit wakes it) with the periodic
 * puffs until the status timer runs out, then CHAM_IDLE_STATUS2_END.
 */
void EmChameleos_Status2Down(Em *em)
{
    em->blind = 2;
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, CHAM_MOT_STATUS_DOWN, 0, 0);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        EmChameleos_SpawnLyingPuffs(em);
        if (--em->timer <= 0) {
            em->step++;
            EmChameleos_ChangeState(em, EM_GROUP_IDLE, CHAM_IDLE_STATUS2_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmChameleos_ChangeState(em, EM_GROUP_IDLE, CHAM_IDLE_STATUS2_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * CHAM_IDLE_REST_END: gets up from rest (CHAM_MOT_REST_END, entered with
 * Em_RefillNeed3), blind meanwhile, and ends.
 */
void EmChameleos_RestEnd(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, CHAM_MOT_REST_END, 0, 0);
        Em_LoadReactionSet(em, 0);
        em->blind = 2;
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->blind = 0;
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * CHAM_IDLE_STATUS2_END: gets up (CHAM_MOT_GET_UP, entered with
 * Em_EndStatus2), blind meanwhile, and ends.
 */
void EmChameleos_Status2End(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, CHAM_MOT_GET_UP, 0, 0);
        Em_LoadReactionSet(em, 0);
        em->blind = 2;
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->blind = 0;
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
}

/* CHAM_IDLE_SHAKE_OFF: plays CHAM_MOT_SHAKE_OFF once and ends. */
void EmChameleos_ShakeOff(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, CHAM_MOT_SHAKE_OFF, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * CHAM_IDLE_STATUS1_DOWN: under status 1 (entered with Em_BeginStatus1),
 * lies as EmChameleos_Status2Down until the timer runs out, then
 * CHAM_IDLE_STATUS1_END.
 */
void EmChameleos_Status1Down(Em *em)
{
    em->blind = 2;
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, CHAM_MOT_STATUS_DOWN, 0, 0);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        EmChameleos_SpawnLyingPuffs(em);
        if (--em->timer <= 0) {
            em->step++;
            EmChameleos_ChangeState(em, EM_GROUP_IDLE, CHAM_IDLE_STATUS1_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmChameleos_ChangeState(em, EM_GROUP_IDLE, CHAM_IDLE_STATUS1_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * CHAM_IDLE_STATUS1_END: gets up (CHAM_MOT_GET_UP, entered with
 * Em_EndStatus1), blind meanwhile, and ends.
 */
void EmChameleos_Status1End(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, CHAM_MOT_GET_UP, 0, 0);
        Em_LoadReactionSet(em, 0);
        em->blind = 2;
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->blind = 0;
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * CHAM_IDLE_STUN_SHAKE_OFF (entered with Em_EndStatus5) and
 * CHAM_IDLE_STATUS4_SHAKE_OFF (entered with Em_EndStatus4): plays
 * CHAM_MOT_SHAKE_OFF once and ends.
 */
void EmChameleos_StatusShakeOff(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, CHAM_MOT_SHAKE_OFF, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * CHAM_MOVE_WALK: walks to the goal (CHAM_MOT_WALK, Em_StepToGoal) and ends
 * there; below 15% hp EmChameleos_SetMoveState makes it CHAM_MOVE_LIMP.
 */
void EmChameleos_Walk(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, CHAM_MOT_WALK, 0, 0);
        break;
    case 1:
        if (Em_StepToGoal(em, 64, 0) == 1) {
            em->step++;
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * CHAM_MOVE_LIMP: walks on CHAM_MOT_LIMP's root motion, turning 64 a
 * frame toward the goal when it has one and counting the distance down
 * (Em_MotionDelta); ends when the goal is reached or the timer set from the
 * distance runs out.
 */
void EmChameleos_Limp(Em *em, EmWork *work)
{
    f32 pos[3];
    s32 angDiff;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, CHAM_MOT_LIMP, 0, 0);
        break;
    case 1:
        if (em->hasGoal != 0) {
            angDiff = (Math_AngleXZ(&em->posX, em->goal) - em->rotY) & 0xFFFF;
            if (angDiff <= ANGLE_TURN / 2) {
                if (angDiff <= 63) {
                    em->rotY += angDiff;
                } else {
                    em->rotY += 64;
                }
            } else if (angDiff > 0xFFC0) {
                em->rotY += angDiff;
            } else {
                em->rotY -= 64;
            }
            Em_MotionDelta(em, pos);
            em->goalDist -= pos[2];
            if (em->goalDist <= 0.0f) {
                em->timer = 1;
            }
        }
        if (--em->timer <= 0) {
            em->step++;
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * CHAM_MOVE_TURN (variant 0) and CHAM_MOVE_TURN_RANDOM (variant 1, first
 * sets a goal 500 away in a random direction): turns to the target with
 * EmChameleos_TurnParams (Em_StartTurnToTarget, Em_TurnStep), then clears
 * EM_FLAG_NO_ROOT_MOTION and ends.
 */
void EmChameleos_Turn(Em *em, EmWork *work, u8 variant)
{
    f32 ofs[3];

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        em->speedBonus = 30;
        switch (variant) {
        case 0:
            break;
        case 1:
            ofs[0] = 0.0f;
            ofs[1] = 0.0f;
            ofs[2] = 500.0f;
            Vec_RotateY(ofs, 6.2831855f * em->stateRand / 65536.0f);
            em->goal[0] = em->posX + ofs[0];
            em->goal[1] = em->posY;
            em->goal[2] = em->posZ + ofs[2];
            break;
        }
        Em_StartTurnToTarget(em, &EmChameleos_TurnParams[0]);
        break;
    case 1:
        if (Em_TurnStep(em, &EmChameleos_TurnParams[0]) == 1) {
            em->step++;
            Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * CHAM_MOVE_RUN (variant 0, a ChameleosAttack of type 6 at frame 34) and
 * CHAM_MOVE_RUN_NO_HIT (variant 1): runs to the goal (CHAM_MOT_RUN,
 * Em_StepToGoal), then plays CHAM_MOT_RUN_STOP and ends when layer 1's
 * motion stops.
 */
void EmChameleos_Run(Em *em, s32 unused, s32 variant)
{
    s32 variant8;

    switch (em->step) {
    case 0:
        em->step++;
        em->stateVar0 = 0;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, CHAM_MOT_RUN, 0, 0);
        break;
    case 1:
        variant8 = variant & 0xFF;
        switch (variant8) {
        case 0:
            if ((em->stateVar0 == 0) && (EmMotion_PassedFrameScaled(em, 34.0f, 0U) != 0)) {
                ChameleosAttack_Create(em, 6);
                em->stateVar0++;
            }
            break;
        case 1:
            break;
        }
        if (Em_StepToGoal(em, 64, 0) == 1) {
            em->step++;
            Em_SetMotion(em, CHAM_MOT_RUN_STOP, 0, 0);
        }
        break;
    case 2:
        if (em->motion[1].playing == 0) {
            em->step++;
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * CHAM_MOVE_SPIN_TURN: plays CHAM_MOT_SPIN_TURN, rotY falling 0x590 a frame
 * between frames 44 and 90, and ends.
 */
void EmChameleos_SpinTurn(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, CHAM_MOT_SPIN_TURN, 0, 0);
        break;
    case 1:
        if (EmMotion_InFrameRange(em, 0, 44.0f, 90.0f) != 0) {
            em->rotY -= 0x590;
        }
        if (em->motion[1].playing == 0) {
            em->step++;
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * CHAM_MOVE_BACK_HOP: plays CHAM_MOT_BACK_HOP, airborne (stance air) and
 * moved back along its facing between frames 44 and 82, and ends.
 */
void EmChameleos_BackHop(Em *em, EmWork *work)
{
    f32 back[3];
    f32 delta[3];
    s32 angles[3];
    f32 mtx[16];

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, CHAM_MOT_BACK_HOP, 0, 0);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 44.0f) != 0 && EmMotion_ReachedFrame(em, 0, 82.0f) == 0) {
            em->stance = EM_STANCE_AIR;
            angles[0] = 0;
            angles[1] = em->rotY;
            angles[2] = 0;
            Vec_Set(back, 0.0f, 0.0f, -139.46706f);
            Mtx_FromAnglesYXZ(angles, mtx);
            Vec_Rotate(delta, back, mtx);
            em->posX += delta[0];
            em->posZ += delta[2];
        } else {
            em->stance = EM_STANCE_GROUND;
        }
        if (em->motion[1].playing == 0) {
            em->stance = EM_STANCE_GROUND;
            em->step++;
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * CHAM_FLY_TAKE_OFF: plays CHAM_MOT_TAKE_OFF with height curve 0, airborne
 * from frame 58, then rises 30 a frame to 600 (scaled) above the floor,
 * plays CHAM_MOT_TAKE_OFF_END and enters CHAM_FLY_HOVER
 * (EmChameleos_EndAirState 0).
 */
void EmChameleos_TakeOff(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, CHAM_MOT_TAKE_OFF, 0, 0);
        EmChameleos_StartCurves(em, 0U);
        ((EmChameleosWork *)work)->flyGuard = 0;
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 58.0f, 0U) != 0) {
            em->step++;
            em->stance = EM_STANCE_AIR;
        }
        break;
    case 2:
        EmChameleos_UpdateCurves(em);
        if (EmMotion_PassedFrameScaled(em, 72.0f, 0U) != 0) {
            em->step++;
        }
        break;
    case 3:
        em->posY += 30.0f;
        if (!(em->posY < (em->floorY + (600.0f * em->scaleY)))) {
            em->step++;
            Em_SetMotion(em, CHAM_MOT_TAKE_OFF_END, 0, 0);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmChameleos_EndAirState(em, 0U);
        }
        break;
    }
}

/*
 * CHAM_FLY_LAND: hovers (CHAM_MOT_HOVER) and sinks, its fall speed chosen
 * to reach 1000 above the floor, until 500 (scaled) above it, then drops to
 * the floor with CHAM_MOT_LAND and a camera shake, stance ground, and ends.
 * EmChameleos_SetFlyState turns it into CHAM_FLY_BRAKE_AND_LAND when
 * flying at speed 50 or more.
 */
void EmChameleos_Land(Em *em, EmWork *work)
{
    f32 floorY;
    f32 dy;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        em->rotX = 0;
        em->rotZ = 0;
        Em_SetMotion(em, CHAM_MOT_HOVER, 0, 0);
        Player_ClearVelocity(em);
        em->velY = -10.0f;
        ((EmChameleosWork *)work)->flyGuard = 0;
        work->moveAngCX = 0;
        work->moveAngX = 0;
        floorY = em->floorY;
        dy = em->posY - (1000.0f + floorY);
        if (dy > 0.0f) {
            do {
                em->velY -= 0.75f;
                dy += em->velY;
            } while (dy > 0.0f && !(em->velY < -200.0f));
        }
        break;
    case 1:
        work->moveAngCY = em->rotY;
        Em_MoveRotated(em, &work->moveAngCX);
        if (em->velY < -10.0f) {
            em->velY += 0.75f;
        }
        if (!((em->floorY + (500.0f * em->scaleY)) <= em->posY)) {
            em->step++;
            Em_SetMotion(em, CHAM_MOT_LAND, 0, 0);
            em->posY = em->floorY;
            em->rotX = 0;
            em->rotZ = 0;
            em->stance = EM_STANCE_GROUND;
            CamShake_ReqAIf(em, 1);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * CHAM_FLY_HOVER: plays CHAM_MOT_HOVER once and enters CHAM_FLY_HOVER again
 * (the script picks what follows).
 */
void EmChameleos_Hover(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, CHAM_MOT_HOVER, 0, 0);
        ((EmChameleosWork *)work)->flyGuard = 0;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmChameleos_EndAirState(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/* CHAM_FLY_NONE_A: does nothing (EmChameleos_EndAirState 2 enters it). */
void EmChameleos_FlyNoneA(void)
{
}

/* CHAM_FLY_NONE_B: does nothing. */
void EmChameleos_FlyNoneB(void)
{
}

/* CHAM_FLY_NONE_C: does nothing. */
void EmChameleos_FlyNoneC(void)
{
}

/*
 * CHAM_FLY_ROUTE: flies (CHAM_MOT_FLY_START, then CHAM_MOT_FLY;
 * Em_UpdateFlight with EmChameleos_FlyParam) from area point to area point;
 * each leg ends within 3000 or when its timer runs out, picks the next
 * point (curGoalNo + 1, Em_ComputeGoal2) and restarts the state, and after
 * the legs EmChameleos_SetFlyState gave (4) it enters CHAM_FLY_FORWARD.
 */
void EmChameleos_FlyRoute(Em *em, EmWork *work)
{
    f32 floorY;
    s32 legTimer;
    u8 legs;
    u8 stateVar1;

    switch (em->step) {
    case 0:
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, CHAM_MOT_FLY_START, 0, 0);
        em->step++;
        em->stateVar1 = 0;
        em->lookAtTarget = 0;
        em->velX = 0.0f;
        em->velY = 0.0f;
        Player_ClearAccel2(em);
        if (em->velZ <= 50.0f) {
            em->velZ = 50.0f;
        }
        ((EmChameleosWork *)work)->flyGuard = 1;
        break;
    case 1:
        stateVar1 = em->stateVar1;
        if ((stateVar1 == 0) && (em->motion[0].playing == 0)) {
            em->stateVar1 = stateVar1 + 1;
            Em_SetMotion(em, CHAM_MOT_FLY, 0, 0);
        }
        if ((Vec_DistanceXZ(&em->posX, em->goal) <= 3000.0f) || (legTimer = em->timer - 1, em->timer = legTimer, (legTimer <= 0))) {
            legs = ((EmChameleosWork *)work)->routeLegs - 1;
            ((EmChameleosWork *)work)->routeLegs = legs;
            if ((legs & 0xFF) <= 0) {
                EmChameleos_EndAirState(em, 1U);
            } else {
                em->curGoalNo++;
                em->curGoalNo &= 3;
                Em_ComputeGoal2(em, em->goal);
                em->timer = (s32) ((1.5f * Vec_DistanceXZ(&em->posX, em->goal)) / 50.0f);
                EmChameleos_ChangeState(em, EM_GROUP_FLY, CHAM_FLY_ROUTE, EM_SYNC_LOCAL);
            }
        }
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 225, &EmChameleos_FlyParam);
        work->moveAngCX = em->rotX;
        work->moveAngCY = em->rotY;
        work->moveAngX = 0;
        Em_MoveRotated(em, &work->moveAngCX);
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * CHAM_FLY_HOVER_TO_GOAL: hovers (CHAM_MOT_HOVER) to the goal at speed 20,
 * turning to it (Em_TurnToGoal 0x100) and pitching toward it, and enters
 * CHAM_FLY_HOVER near the goal or when the timer runs out.
 */
void EmChameleos_HoverToGoal(Em *em, EmWork *work)
{
    f32 dist;
    f32 floorY;
    s32 timer;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        work->moveAngZ = 0x100;
        Em_SetMotion(em, CHAM_MOT_HOVER, 0, 0);
        ((EmChameleosWork *)work)->flyGuard = 0;
        break;
    case 1:
        Em_TurnToGoal(em, (s32) (u16) work->moveAngZ, 1);
        work->moveAngCY = em->rotY;
        work->moveAngX = 0;
        Em_GetPitchToGoal(em, &work->moveAngCX, 0.0f, 0.0f);
        Em_MoveRotated(em, &work->moveAngCX);
        dist = Vec_DistanceXZ(&em->posX, em->goal);
        timer = em->timer - 1;
        em->timer = timer;
        if ((timer <= 0) || (dist <= (10.0f * em->velZ))) {
            em->step++;
            Player_ClearVelocity(em);
            EmChameleos_EndAirState(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * CHAM_FLY_OUT_OF_AREA: flies (CHAM_MOT_FLY, speed 50) to the area's exit
 * point until out of the area, then enters the next area
 * (Em_EnterNextArea): CHAM_FLY_INTO_AREA when it is the destination (or
 * there is none), else CHAM_FLY_THROUGH_AREA toward the next one; the last
 * step retries the change on the owner. EmChameleos_SetFlyState lands it
 * instead (CHAM_FLY_LAND) when its goal is not an area.
 */
void EmChameleos_FlyOutOfArea(Em *em, EmWork *work)
{
    f32 floorY;
    u16 next;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Player_ClearVelocity(em);
        em->velZ = 50.0f;
        Em_SetMotion(em, CHAM_MOT_FLY, 0, 0);
        Em_AreaExitPoint(em, (f32 *) em->goal);
        ((EmChameleosWork *)work)->flyGuard = 1;
        break;
    case 1:
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 225, &EmChameleos_FlyParam);
        work->moveAngCX = em->rotX;
        work->moveAngCY = em->rotY;
        work->moveAngX = 0;
        Em_MoveRotated(em, &work->moveAngCX);
        if (Area_ContainsPos((s32) em->area, &em->posX) == 0) {
            em->step++;
        }
        break;
    case 2:
        em->step++;
        break;
    case 3:
        em->step++;
        em->timer = 600;
        Em_EnterNextArea(em);
        next = *(u16 *) &em->destArea;
        if (em->nextArea == next || next == AREA_NONE) {
            Hud_TriggerGlow(em);
            EmChameleos_ChangeState(em, EM_GROUP_FLY, CHAM_FLY_INTO_AREA, EM_SYNC_SEND);
        } else {
            if (em->remote == 0) {
                em->nextArea = next;
                em->goalNo = next;
                em->goalKind = 3;
                Em_ComputeGoal(em, em->goal);
            }
            EmChameleos_ChangeState(em, EM_GROUP_FLY, CHAM_FLY_THROUGH_AREA, EM_SYNC_SEND);
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
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * CHAM_FLY_INTO_AREA: faces the goal (an area point from
 * EmChameleos_SetFlyState) and flies to it at speed 80 (Em_UpdateFlight
 * 229), clearing destArea, then enters CHAM_FLY_LAND within 1000 or when
 * the timer runs out (retried on the owner every 600 frames).
 */
void EmChameleos_FlyIntoArea(Em *em, EmWork *work)
{
    f32 goalDist;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        work->moveAngZ = 0x100;
        Em_SetMotion(em, CHAM_MOT_HOVER, 0, 0);
        Player_ClearVelocity(em);
        em->velZ = 80.0f;
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->rotY = (s32) em->aimAngle;
        *(u16 *) &em->destArea = AREA_NONE;
        ((EmChameleosWork *)work)->flyGuard = 1;
        break;
    case 1:
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 229, &EmChameleos_FlyParam);
        work->moveAngCX = em->rotX;
        work->moveAngCY = em->rotY;
        work->moveAngX = 0;
        Em_MoveRotated(em, &work->moveAngCX);
        goalDist = Vec_DistanceXZ(&em->posX, em->goal);
        em->timer--;
        if (goalDist <= 1000.0f || em->timer < 0) {
            em->step++;
            em->timer = 600;
            EmChameleos_ChangeState(em, EM_GROUP_FLY, CHAM_FLY_LAND, EM_SYNC_SEND);
        }
        break;
    case 2:
        if (em->remote == 0) {
            if (--em->timer <= 0) {
                em->timer = 600;
                EmChameleos_ChangeState(em, EM_GROUP_FLY, CHAM_FLY_LAND, EM_SYNC_SEND);
            }
        }
        break;
    }
    if (em->posY < em->floorY) {
        em->posY = em->floorY;
    }
}

/* CHAM_FLY_NONE_D: does nothing. */
void EmChameleos_FlyNoneD(void)
{
}

/*
 * CHAM_FLY_TO_GOAL_AND_LAND: flies (CHAM_MOT_FLY, at most speed 100) to the
 * goal, slowing to 50 within 1000, and enters CHAM_FLY_LAND there or when
 * the timer runs out (retried every 300 frames).
 */
void EmChameleos_FlyToGoalAndLand(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_SetMotion(em, CHAM_MOT_FLY, 0, 0);
        if (!(em->velZ <= 100.0f)) {
            em->velZ = 100.0f;
        }
        ((EmChameleosWork *)work)->flyGuard = 1;
        break;
    case 1:
        if (Vec_DistanceXZ(&em->posX, em->goal) <= 1000.0f) {
            if (!(em->velZ <= 50.0f)) {
                em->velZ = 50.0f;
            }
            em->step++;
            em->timer = 300;
            EmChameleos_ChangeState(em, EM_GROUP_FLY, CHAM_FLY_LAND, EM_SYNC_SEND);
        } else {
            em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
            em->aimAngle = em->aimAngle - em->rotY;
            Em_UpdateFlight(em, 225, &EmChameleos_FlyParam);
            work->moveAngCX = em->rotX;
            work->moveAngCY = em->rotY;
            work->moveAngX = 0;
            Em_MoveRotated(em, &work->moveAngCX);
            if (--em->timer <= 0) {
                em->step++;
                em->timer = 300;
                EmChameleos_ChangeState(em, EM_GROUP_FLY, CHAM_FLY_LAND, EM_SYNC_SEND);
            }
        }
        break;
    case 2:
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 225, &EmChameleos_FlyParam);
        work->moveAngCX = em->rotX;
        work->moveAngCY = em->rotY;
        work->moveAngX = 0;
        Em_MoveRotated(em, &work->moveAngCX);
        if (--em->timer <= 0) {
            em->timer = 300;
            if (em->remote == 0) {
                EmChameleos_ChangeState(em, EM_GROUP_FLY, CHAM_FLY_LAND, EM_SYNC_SEND);
            }
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/* CHAM_FLY_NONE_E: does nothing. */
void EmChameleos_FlyNoneE(void)
{
}

/*
 * CHAM_FLY_THROUGH_AREA: crosses an area on the way: flies at speed 80 to
 * the area's entry point (or its centre, at its bottom height), then to its
 * exit point, and on leaving it enters the next area as
 * EmChameleos_FlyOutOfArea does; each leg also ends after 900 frames.
 */
void EmChameleos_FlyThroughArea(Em *em, EmWork *work)
{
    AreaRecord *area;
    f32 *entryPt;
    u16 next;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        work->moveAngZ = 0x100;
        Em_SetMotion(em, CHAM_MOT_HOVER, 0, 0);
        Player_ClearVelocity(em);
        em->velZ = 80.0f;
        area = Stage_GetAreaRecord(em->area);
        em->goal[1] = area->bottom;
        entryPt = Em_AreaEntryValue(em, *em->tables);
        if (entryPt == NULL) {
            em->goal[0] = area->sizeX / 2.0f;
            em->goal[2] = area->sizeZ / 2.0f;
        } else {
            em->goal[0] = entryPt[0];
            em->goal[2] = entryPt[2];
        }
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->rotY = em->aimAngle;
        ((EmChameleosWork *)work)->flyGuard = 1;
        em->timer = 900;
        break;
    case 1:
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 229, &EmChameleos_FlyParam);
        work->moveAngCX = em->rotX;
        work->moveAngCY = em->rotY;
        work->moveAngX = 0;
        Em_MoveRotated(em, &work->moveAngCX);
        if (Vec_DistanceXZ(&em->posX, em->goal) <= 1000.0f || --em->timer <= 0) {
            em->step++;
            em->timer = 900;
            Em_AreaExitPoint(em, (f32 *) em->goal);
        }
        break;
    case 2:
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 225, &EmChameleos_FlyParam);
        work->moveAngCX = em->rotX;
        work->moveAngCY = em->rotY;
        work->moveAngX = 0;
        Em_MoveRotated(em, &work->moveAngCX);
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
            EmChameleos_ChangeState(em, EM_GROUP_FLY, CHAM_FLY_INTO_AREA, EM_SYNC_SEND);
        } else {
            if (em->remote == 0) {
                em->nextArea = next;
                em->goalNo = next;
                em->goalKind = 3;
                Em_ComputeGoal(em, em->goal);
            }
            EmChameleos_ChangeState(em, EM_GROUP_FLY, CHAM_FLY_THROUGH_AREA, EM_SYNC_SEND);
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
 * CHAM_FLY_ASCEND: hovers (CHAM_MOT_HOVER) and rises, speeding up to 100
 * once 500 above the floor, until the area's height (gAreaGroundY), then
 * enters CHAM_FLY_HOVER when the motion ends.
 */
void EmChameleos_Ascend(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, CHAM_MOT_HOVER, 0, 0);
        ((EmChameleosWork *)work)->flyGuard = 0;
        Player_ClearVelocity(em);
        em->velY = 10.0f;
        break;
    case 1:
        Em_MoveRotated(em, &em->rotX);
        if (!(em->posY <= (500.0f + em->floorY))) {
            if (em->velY < 100.0f) {
                em->velY += 0.75f;
            }
        }
        if (!(em->posY < gAreaGroundY[em->area])) {
            em->step++;
            Player_ClearVelocity(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmChameleos_EndAirState(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * CHAM_FLY_FORWARD: flies straight on at its current forward speed (velX,
 * velY and the acceleration cleared), plays CHAM_MOT_FLY once and enters
 * CHAM_FLY_FORWARD again.
 */
void EmChameleos_FlyForward(Em *em, EmWork *work)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, CHAM_MOT_FLY, 0, 0);
        Player_ClearAccel2(em);
        em->velY = 0.0f;
        em->velX = 0.0f;
        ((EmChameleosWork *)work)->flyGuard = 1;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmChameleos_EndAirState(em, 1U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * CHAM_FLY_TURN_TO_GOAL: hovers turning to the goal (Em_TurnToGoal 0x180)
 * and enters CHAM_FLY_HOVER when it faces it or after 150 frames.
 */
void EmChameleos_HoverTurnToGoal(Em *em, EmWork *work)
{
    f32 floorY;
    s32 done;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, CHAM_MOT_HOVER, 0, 0);
        work->moveAngZ = 0x180;
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        ((EmChameleosWork *)work)->flyGuard = 0;
        em->timer = 150;
        break;
    case 1:
        done = Em_TurnToGoal(em, (s32) (u16) work->moveAngZ, 1) & 0xFF;
        if (--em->timer <= 0 || done != 0) {
            em->step++;
            EmChameleos_EndAirState(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * CHAM_FLY_ROUTE_LOOP: flies (CHAM_MOT_FLY_START unless already flying,
 * then CHAM_MOT_FLY; EmChameleos_FlyParam2) to the area points one after
 * another without end, restarting itself at each point (within 1000 or on
 * the timer).
 */
void EmChameleos_FlyRouteLoop(Em *em, EmWork *work)
{
    f32 floorY;
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
            Em_SetMotion(em, CHAM_MOT_FLY_START, 0, 0);
        } else {
            em->stateVar1 = 1;
            if ((u32) (em->motionId[0] - 1012) > 1) {
                Em_SetMotion(em, CHAM_MOT_FLY_START, 0, 0);
            }
        }
        ((EmChameleosWork *)work)->flyGuard = 1;
        break;
    case 1:
        stateVar1 = em->stateVar1;
        if ((stateVar1 == 0) && (em->motion[0].playing == 0)) {
            em->stateVar1 = stateVar1 + 1;
            Em_SetMotion(em, CHAM_MOT_FLY, 0, 0);
        }
        if (Vec_DistanceXZ(&em->posX, em->goal) <= 1000.0f || --em->timer <= 0) {
            em->curGoalNo++;
            em->curGoalNo &= 3;
            Em_ComputeGoal2(em, em->goal);
            em->timer = (s32) ((1.5f * Vec_DistanceXZ(&em->posX, em->goal)) / 50.0f) + 150;
            em->step++;
            EmChameleos_ChangeState(em, EM_GROUP_FLY, CHAM_FLY_ROUTE_LOOP, EM_SYNC_LOCAL);
        }
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 225, &EmChameleos_FlyParam2);
        work->moveAngCX = em->rotX;
        work->moveAngCY = em->rotY;
        work->moveAngX = 0;
        Em_MoveRotated(em, &work->moveAngCX);
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * CHAM_FLY_BRAKE_AND_LAND: brakes from fast flight (CHAM_MOT_BRAKE, curve
 * 2) levelling rotX and rotZ, then plays CHAM_MOT_HOVER_SETTLE and enters
 * CHAM_FLY_LAND (retried on the owner every 300 frames).
 */
void EmChameleos_BrakeAndLand(Em *em, EmWork *work)
{
    f32 floorY;
    s32 rotX;
    s32 rotZ;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_SetMotion(em, CHAM_MOT_BRAKE, 0, 0);
        EmChameleos_StartCurves(em, 2U);
        ((EmChameleosWork *)work)->flyGuard = 0;
        break;
    case 1:
        rotX = em->rotX;
        if (rotX != 0) {
            if (rotX < (ANGLE_TURN / 2)) {
                em->rotX = rotX - work->moveAngY;
            } else {
                em->rotX = rotX + work->moveAngY;
            }
            em->rotX = (s32) (u16) em->rotX;
        }
        rotZ = em->rotZ;
        if (rotZ != 0) {
            if (rotZ < (ANGLE_TURN / 2)) {
                em->rotZ = rotZ - work->pitchStep;
            } else {
                em->rotZ = rotZ + work->pitchStep;
            }
            em->rotZ = (s32) (u16) em->rotZ;
        }
        if (EmChameleos_UpdateCurves(em) & 0xFF) {
            em->step++;
            Player_ClearVelocity(em);
            Em_SetMotion(em, CHAM_MOT_HOVER_SETTLE, 0, 0);
            em->timer = 300;
            EmChameleos_ChangeState(em, EM_GROUP_FLY, CHAM_FLY_LAND, EM_SYNC_SEND);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->timer = 300;
            Em_SetMotion(em, CHAM_MOT_HOVER, 0, 0);
            if (em->remote == 0) {
                EmChameleos_ChangeState(em, EM_GROUP_FLY, CHAM_FLY_LAND, EM_SYNC_SEND);
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
 * CHAM_FLY_TO_GOAL: flies (CHAM_MOT_FLY_START unless already flying, then
 * CHAM_MOT_FLY; EmChameleos_FlyParam2) to the goal and enters
 * CHAM_FLY_FORWARD within 1000 or when the timer runs out.
 */
void EmChameleos_FlyToGoal(Em *em, EmWork *work)
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
            Em_SetMotion(em, CHAM_MOT_FLY_START, 0, 0);
        } else {
            em->stateVar1 = 1;
            motionId = em->motionId[0];
            if ((motionId != EM_MOTION_ID(0, CHAM_MOT_FLY)) && (motionId != EM_MOTION_ID(0, 14))) {
                Em_SetMotion(em, CHAM_MOT_FLY_START, 0, 0);
            }
        }
        ((EmChameleosWork *)work)->flyGuard = 1;
        break;
    case 1:
        stateVar1 = em->stateVar1;
        if ((stateVar1 == 0) && (em->motion[0].playing == 0)) {
            em->stateVar1 = stateVar1 + 1;
            Em_SetMotion(em, CHAM_MOT_FLY, 0, 0);
        }
        if (Vec_DistanceXZ(&em->posX, em->goal) <= 1000.0f || --em->timer <= 0) {
            em->step++;
            EmChameleos_EndAirState(em, 1U);
        }
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->aimAngle = em->aimAngle - em->rotY;
        Em_UpdateFlight(em, 225, &EmChameleos_FlyParam2);
        work->moveAngCX = em->rotX;
        work->moveAngCY = em->rotY;
        work->moveAngX = 0;
        Em_MoveRotated(em, &work->moveAngCX);
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/* CHAM_FLY_NONE_F: does nothing. */
void EmChameleos_FlyNoneF(void)
{
}

/*
 * CHAM_FLY_STRAFE_TO_GOAL: hovers (CHAM_MOT_HOVER) straight at the goal at
 * speed 20 without turning its body (yaw and pitch of the move aimed at the
 * goal), and enters CHAM_FLY_HOVER near it or when the timer runs out.
 */
void EmChameleos_StrafeToGoal(Em *em, EmWork *work)
{
    f32 dist;
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        work->moveAngZ = 0x100;
        Em_SetMotion(em, CHAM_MOT_HOVER, 0, 0);
        ((EmChameleosWork *)work)->flyGuard = 0;
        break;
    case 1:
        work->moveAngCY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        work->moveAngX = 0;
        Em_GetPitchToGoal(em, &work->moveAngCX, 0.0f, 0.0f);
        Em_MoveRotated(em, &work->moveAngCX);
        dist = Vec_Distance((f32 *) &em->posX, (f32 *) em->goal);
        if (--em->timer <= 0 || dist <= (10.0f * em->velZ)) {
            em->step++;
            Player_ClearVelocity(em);
            EmChameleos_EndAirState(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * CHAM_FLY_BRAKE: brakes (CHAM_MOT_BRAKE, curve 2) levelling rotX and rotZ
 * until the curves end, then enters CHAM_FLY_HOVER.
 */
void EmChameleos_Brake(Em *em, EmWork *work)
{
    f32 floorY;
    s32 rotX;
    s32 rotZ;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_SetMotion(em, CHAM_MOT_BRAKE, 0, 0);
        EmChameleos_StartCurves(em, 2U);
        ((EmChameleosWork *)work)->flyGuard = 0;
        break;
    case 1:
        rotX = em->rotX;
        if (rotX != 0) {
            if (rotX < (ANGLE_TURN / 2)) {
                em->rotX = rotX - work->moveAngY;
            } else {
                em->rotX = rotX + work->moveAngY;
            }
            em->rotX = (s32) (u16) em->rotX;
        }
        rotZ = em->rotZ;
        if (rotZ != 0) {
            if (rotZ < (ANGLE_TURN / 2)) {
                em->rotZ = rotZ - work->pitchStep;
            } else {
                em->rotZ = rotZ + work->pitchStep;
            }
            em->rotZ = (s32) (u16) em->rotZ;
        }
        if (EmChameleos_UpdateCurves(em) & 0xFF) {
            em->step++;
            Player_ClearVelocity(em);
            EmChameleos_EndAirState(em, 0U);
        }
        break;
    case 2:
        EmChameleos_UpdateCurves(em);
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/* CHAM_FLY_NONE_G: does nothing. */
void EmChameleos_FlyNoneG(void)
{
}

/*
 * CHAM_FLY_RISE: hovers (CHAM_MOT_HOVER) rising at 20 a frame until 1000
 * above the goal's height or the timer runs out, then enters
 * CHAM_FLY_HOVER.
 */
void EmChameleos_Rise(Em *em, EmWork *work)
{
    f32 pos[3];
    f32 floorY;
    s32 timer;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, CHAM_MOT_HOVER, 0, 0);
        ((EmChameleosWork *)work)->flyGuard = 0;
        work->moveAngCX = 0;
        work->moveAngX = 0;
        Player_ClearVelocity(em);
        em->velY = 20.0f;
        break;
    case 1:
        work->moveAngCY = Math_AngleXZ(&em->posX, pos) & 0xFFFF;
        Em_MoveRotated(em, &work->moveAngCX);
        timer = em->timer - 1;
        em->timer = timer;
        if ((timer <= 0) || !(em->posY <= (1000.0f + em->goal[1]))) {
            em->step++;
            Player_ClearVelocity(em);
            EmChameleos_EndAirState(em, 0U);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * CHAM_ATK_TONGUE (variant 0: CHAM_MOT_TAIL_POUNDS first) and
 * CHAM_ATK_TONGUE_QUICK (variant 1): lashes the tongue along the floor
 * (CHAM_MOT_TONGUE, ground effect ahead of CHAM_BONE_TONGUE at frame 36)
 * and steals from the player the tongue hits (EmChameleos_StealItem), then
 * ends.
 */
void EmChameleos_Tongue(Em *em, s32 unused, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 1;
        if (!(variant & 0xFF)) {
            Em_SetMotion(em, CHAM_MOT_TAIL_POUNDS, 0, 0);
            break;
        }
        Em_SetMotion(em, CHAM_MOT_TONGUE, 0, 0);
        em->step++;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, CHAM_MOT_TONGUE, 0, 0);
        }
        break;
    case 2:
        if (EmMotion_PassedFrameScaled(em, 36.0f, 0U) != 0) {
            EmChameleos_SpawnGroundEffect(em, CHAM_BONE_TONGUE, 20, 4.0f);
        }
        if (EmChameleos_TongueHitPlayer(em) == 1) {
            em->step++;
            EmChameleos_StealItem(em, em->contact);
        }

    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * CHAM_ATK_TONGUE_LOW: CHAM_MOT_TONGUE_LOW, stealing from the player the
 * tongue hits; EmChameleos_SetAttackState picks it for CHAM_ATK_TONGUE_HIGH
 * when the target is no more than 50 above it.
 */
void EmChameleos_TongueLow(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, CHAM_MOT_TONGUE_LOW, 0, 0);
        break;
    case 1:
        if (EmChameleos_TongueHitPlayer(em) == 1) {
            em->step++;
            EmChameleos_StealItem(em, em->contact);
        }

    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * CHAM_ATK_STOMP: plays CHAM_MOT_STOMP once and ends (its attack and ground
 * effects come from EmChameleos_MotionEffects).
 */
void EmChameleos_Stomp(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 100;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, CHAM_MOT_STOMP, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * CHAM_ATK_TAIL_SLAM: plays CHAM_MOT_TAIL_SLAM once and ends (the backward
 * attack and impact come from EmChameleos_MotionEffects).
 */
void EmChameleos_TailSlam(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 100;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, CHAM_MOT_TAIL_SLAM, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * CHAM_ATK_TONGUE_B: CHAM_MOT_TAIL_POUNDS, then CHAM_MOT_TONGUE_B, stealing
 * from the player the tongue hits, and ends.
 */
void EmChameleos_TongueB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 1;
        Em_SetMotion(em, CHAM_MOT_TAIL_POUNDS, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, CHAM_MOT_TONGUE_B, 0, 0);
        }
        break;
    case 2:
        if (EmChameleos_TongueHitPlayer(em) == 1) {
            em->step++;
            EmChameleos_StealItem(em, em->contact);
        }

    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * CHAM_ATK_TONGUE_HIGH: CHAM_MOT_TONGUE_HIGH looking at the target,
 * stealing from the player the tongue hits, and ends.
 */
void EmChameleos_TongueHigh(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 1;
        Em_SetMotion(em, CHAM_MOT_TONGUE_HIGH, 0, 0);
        break;
    case 1:
        if (EmChameleos_TongueHitPlayer(em) == 1) {
            em->step++;
            EmChameleos_StealItem(em, em->contact);
        }

    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * CHAM_ATK_BALL_PITCHED (variant 0: a poison ball, EMFX_CHAMELEOS_BALL,
 * after 10 frames) and CHAM_ATK_BREATH_PITCHED (variant 1: the mist breath,
 * EMFX_CHAMELEOS_BREATH, for 60 frames): CHAM_MOT_SPIT_START, then
 * CHAM_MOT_SPIT_A with the attack pitched 1820 from frame 10, then
 * CHAM_MOT_SPIT_A_END, and ends.
 */
void EmChameleos_MistPitched(Em *em, s32 unused, u8 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, CHAM_MOT_SPIT_START, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            if (variant == 0) {
                em->timer = 10;
            } else {
                em->timer = 60;
            }
            Em_SetMotion(em, CHAM_MOT_SPIT_A, 0, 0);
        }
        break;
    case 2:
        if (variant == 0) {
            if (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0) {
                EmEffectAttack_Create(em, EMFX_CHAMELEOS_BALL, 0, 1820, 0);
            }
        } else {
            if (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0) {
                EmEffectAttack_CreateTimed(em, EMFX_CHAMELEOS_BREATH, 0, 2731, 0, em->timer - 20);
            }
        }
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, CHAM_MOT_SPIT_A_END, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * CHAM_ATK_BALL_LEVEL (variant 0) and CHAM_ATK_BREATH_LEVEL (variant 1): as
 * EmChameleos_MistPitched with CHAM_MOT_SPIT_B and CHAM_MOT_SPIT_B_END, the
 * attack level (pitch 0) from frame 6.
 */
void EmChameleos_MistLevel(Em *em, s32 unused, u8 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, CHAM_MOT_SPIT_START, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            if (variant == 0) {
                em->timer = 10;
            } else {
                em->timer = 60;
            }
            Em_SetMotion(em, CHAM_MOT_SPIT_B, 0, 0);
        }
        break;
    case 2:
        if (variant == 0) {
            if (EmMotion_PassedFrameScaled(em, 6.0f, 0) != 0) {
                EmEffectAttack_Create(em, EMFX_CHAMELEOS_BALL, 0, 0, 0);
            }
        } else {
            if (EmMotion_PassedFrameScaled(em, 6.0f, 0) != 0) {
                EmEffectAttack_CreateTimed(em, EMFX_CHAMELEOS_BREATH, 0, 0, 0, em->timer - 20);
            }
        }
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, CHAM_MOT_SPIT_B_END, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * CHAM_ATK_BACK_HOP_BALL: CHAM_MOT_BACK_HOP_BALL; at frame 88 leaps up and
 * back (stance air) firing a poison ball (EMFX_CHAMELEOS_BALL variant 1,
 * pitch a quarter turn), and enters CHAM_FLY_HOVER when layer 1's motion ends.
 */
void EmChameleos_BackHopBall(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, CHAM_MOT_BACK_HOP_BALL, 0, 0);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 88.0f) != 0) {
            em->step++;
            em->stance = EM_STANCE_AIR;
            ((EmChameleosWork *)work)->flyGuard = 0;
            work->moveAngCX = 0;
            work->moveAngCY = em->rotY;
            work->moveAngX = 0;
            Player_ClearVelocity(em);
            em->velY = 12.0f;
            em->velZ = -7.0f;
            EmEffectAttack_Create(em, EMFX_CHAMELEOS_BALL, 1, 16384, 0);
        }
        break;
    case 2:
        Em_MoveRotated(em, &work->moveAngCX);
        if (em->motion[1].playing == 0) {
            em->step++;
            EmChameleos_EndAirState(em, 0U);
        }
        break;
    }
}

/*
 * CHAM_ATK_TAIL_POUNDS: plays CHAM_MOT_TAIL_POUNDS once looking at the
 * target and ends.
 */
void EmChameleos_TailPounds(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 1;
        Em_SetMotion(em, CHAM_MOT_TAIL_POUNDS, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * CHAM_REACT_FLINCH_A: a flinch of part 0 or 6 not yet broken (Update);
 * restarts the script, plays CHAM_MOT_FLINCH_A and ends.
 */
void EmChameleos_FlinchA(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, CHAM_MOT_FLINCH_A, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * CHAM_REACT_FLINCH_B: a flinch of parts 1 to 5 not yet broken (Update);
 * restarts the script, plays CHAM_MOT_FLINCH_B and ends.
 */
void EmChameleos_FlinchB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, CHAM_MOT_FLINCH_B, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * CHAM_REACT_STAGGER (variant 0: a broken part 0, 1, 2, 5 or 6's flinch)
 * and CHAM_REACT_DECLOAK (variant 1: an attack-kind-5 hit while visible,
 * entered with EmChameleos_ClearFullCloak; drops a timed mine point,
 * EmMine_CreateTimed0): CHAM_MOT_COLLAPSE, then CHAM_MOT_RECOVER from frame
 * 96, and ends.
 */
void EmChameleos_Stagger(Em *em, s32 unused, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, CHAM_MOT_COLLAPSE, 0, 0);
        if ((variant & 0xFF) == 1) {
            EmMine_CreateTimed0(em, &em->posX);
        }
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 96.0f, 0U) != 0) {
            em->step++;
            Em_SetMotion(em, CHAM_MOT_RECOVER, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * CHAM_REACT_KNOCK_DOWN (variant 0: a broken part 3 or 4's flinch; lies 120
 * frames) and CHAM_REACT_STUN (variant 1: entered with Em_BeginStatus5,
 * stun stars, lies for the stun timer, then CHAM_IDLE_STUN_SHAKE_OFF):
 * CHAM_MOT_COLLAPSE, CHAM_MOT_DOWN, CHAM_MOT_GET_UP.
 */
void EmChameleos_KnockDown(Em *em, EmWork *work, u8 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, CHAM_MOT_COLLAPSE, 0, 0);
        switch (variant) {
        case 1:
            StunStars_CreateEm(em, 1, 37, em->timer + em->motion[0].frameCount / 2.0f, 200.0f, 3.5f);
            break;
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            switch (variant) {
            default:
                em->timer = 120;
            case 1:
                break;
            }
            Em_SetMotion(em, CHAM_MOT_DOWN, 0, 24);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, CHAM_MOT_GET_UP, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            switch (variant) {
            case 0:
            default:
                EmChameleos_EndState(em, 0, 0);
                break;
            case 1:
                em->step++;
                EmChameleos_ChangeState(em, EM_GROUP_IDLE, CHAM_IDLE_STUN_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
                break;
            }
        }
        break;
    case 4:
        if (em->remote == 0) {
            EmChameleos_ChangeState(em, EM_GROUP_IDLE, CHAM_IDLE_STUN_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * CHAM_REACT_PART_BREAK: the horn breaks (EM_DMG_PART8_DOWN; ChangeState
 * counts partBreakCount, sets fxFlags 1 and breaks part 1); cannot be hit
 * meanwhile (noHitTimer), plays CHAM_MOT_PART_BREAK, then
 * CHAM_MOT_PART_BREAK_SPIN turning 630 (times the motion speed) a frame to
 * frame 104, and ends.
 */
void EmChameleos_PartBreak(Em *em)
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
        Em_SetMotion(em, CHAM_MOT_PART_BREAK, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, CHAM_MOT_PART_BREAK_SPIN, 0, 0);
        }
        break;
    case 2:
        if (EmMotion_InFrameRange(em, 0, 0.0f, 104.0f) != 0) {
            turnF = 630.0f * em->motionSpeed;
            turn = (u32) turnF;
            em->rotY -= (s32) turn & 0xFFFF;
        }
        if (em->motion[0].playing == 0) {
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
}

/* CHAM_REACT_NONE_A: does nothing. */
void EmChameleos_ReactNoneA(void)
{
}

/* CHAM_REACT_NONE_B: does nothing. */
void EmChameleos_ReactNoneB(void)
{
}

/*
 * CHAM_REACT_FALL (variant 0: a status or flinch in the air),
 * CHAM_REACT_FALL_BLIND (variant 1: blind throughout) and
 * CHAM_REACT_DECLOAK_FALL (variant 2: an attack-kind-5 hit while flying
 * visible; drops a timed mine point on landing): falls (CHAM_MOT_FALL,
 * accelY -10) to the floor, CHAM_MOT_FALL_LAND, lies 60 frames
 * (CHAM_MOT_DOWN), gets up (CHAM_MOT_GET_UP) and ends.
 */
void EmChameleos_Fall(Em *em, EmWork *work, s32 variant)
{
    s32 variant8;

    variant8 = variant & 0xFF;
    if (variant8 == 1) {
        em->blind = 2;
    }
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, CHAM_MOT_FALL, 0, 0);
        if (!(em->velY <= -50.0f)) {
            em->velY = -50.0f;
        }
        em->accelX = 0.0f;
        em->accelY = -10.0f;
        em->accelZ = 0.0f;
        work->moveAngCX = 0;
        work->moveAngX = 0;
        break;
    case 1:
        work->moveAngCY = em->rotY;
        Em_MoveRotatedAccel(em, &work->moveAngCX);
        if (em->posY <= em->floorY) {
            em->step++;
            em->posY = em->floorY;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, CHAM_MOT_FALL_LAND, 0, 0);
            Player_ClearVelocity(em);
            if (variant8 == 2) {
                EmMine_CreateTimed0(em, &em->posX);
                break;
            }
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 60;
            Em_SetMotion(em, CHAM_MOT_DOWN, 0, 0);
        }
        break;
    case 3:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, CHAM_MOT_GET_UP, 0, 0);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmChameleos_EndState(em, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    }
}

/*
 * CHAM_REACT_FLINCH_C: same body as EmChameleos_FlinchB
 * (CHAM_MOT_FLINCH_B); Update never enters it.
 */
void EmChameleos_FlinchC(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, CHAM_MOT_FLINCH_B, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * CHAM_REACT_STATUS4: under status 4 (EM_DMG_STATUS4, entered with
 * Em_BeginStatus4): CHAM_MOT_STATUS4 with the status-4 effect until the
 * status timer runs out, then CHAM_IDLE_STATUS4_SHAKE_OFF.
 */
void EmChameleos_Status4(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_STATUS);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, CHAM_MOT_STATUS4, 0, 0);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmChameleos_ChangeState(em, EM_GROUP_IDLE, CHAM_IDLE_STATUS4_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmChameleos_ChangeState(em, EM_GROUP_IDLE, CHAM_IDLE_STATUS4_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
    Em_SpawnStatus4Effect(em, 2);
}

/*
 * CHAM_EVENT_FOREST: the scene in AREA_FOREST_HILLS_9 (Update starts it in
 * quests QUEST_ID_CHAMELEOS_SCENE_1 and QUEST_ID_CHAMELEOS_SCENE_2 until the save's EVENT_FLAG_CHAMELEOS_FOREST_SCENE is set): placed at a fixed spot,
 * walks a fixed path (CHAM_MOT_WALK, CHAM_MOT_IDLE), turning invisible at
 * frame 50 of the idle and visible again 1300 from the second point, then
 * waits for EVENT_FLAG_CHAMELEOS_FOREST_SCENE and ends. Senses and hits are held off
 * throughout.
 */
void EmChameleos_ForestEvent(Em *em, EmWork *work)
{
    f32 goalDist;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        GmEm_ResetSize(em);
        em->posX = 9712.0f;
        em->posY = 0.0f;
        em->posZ = 13303.0f;
        em->goal[0] = 10091.0f;
        em->goal[1] = 0.0f;
        em->goal[2] = 7312.0f;
        em->rotX = 0;
        em->rotY = Math_AngleXZ(&em->posX, em->goal);
        em->rotZ = 0;
        EmChameleos_SetFullCloak(em);
        Em_SetMotion(em, CHAM_MOT_WALK, 0, 0);
        em->timer = 600;
        break;
    case 1:
        goalDist = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer <= 0 || goalDist <= 200.0f) {
            em->step++;
            em->goal[0] = 10384.0f;
            em->goal[1] = 0.0f;
            em->goal[2] = 5541.0f;
            em->rotX = 0;
            em->rotY = Math_AngleXZ(&em->posX, em->goal);
            em->rotZ = 0;
            Em_SetMotion(em, CHAM_MOT_IDLE, 0, 0);
        }
        break;
    case 2:
        if (EmMotion_PassedFrameScaled(em, 50.0f, 0) != 0) {
            EmChameleos_ClearFullCloak(em);
            em->step++;
        }
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 600;
            Em_SetMotion(em, CHAM_MOT_WALK, 0, 0);
        }
        break;
    case 4:
        goalDist = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer <= 0 || goalDist <= 1300.0f) {
            em->step++;
            em->timer = 600;
            EmChameleos_SetFullCloak(em);
        }
        break;
    case 5:
        goalDist = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer <= 0 || goalDist <= 2000.0f) {
            em->step++;
            Em_SetMotion(em, CHAM_MOT_IDLE, 0, 0);
        }
        break;
    case 6:
        if (Save_TestEventFlag(EVENT_FLAG_CHAMELEOS_FOREST_SCENE) == 1) {
            em->step++;
            EmChameleos_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * CHAM_EVENT_LEAVE: the quest is over (Update) on the ground: turns
 * invisible, CHAM_MOT_COLLAPSE and CHAM_MOT_RECOVER, takes off (as
 * EmChameleos_TakeOff), leaves combat and rises out of sight (to height
 * 50000), then drifts on.
 */
void EmChameleos_LeaveQuest(Em *em, EmWork *work)
{
    f32 groundY;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, CHAM_MOT_COLLAPSE, 0, 0);
        ((EmChameleosWork *)work)->flyGuard = 0;
        EmChameleos_ClearFullCloak(em);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 96.0f, 0) != 0) {
            em->step++;
            Em_SetMotion(em, CHAM_MOT_RECOVER, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->lookAtTarget = 0;
            Em_SetMotion(em, CHAM_MOT_TAKE_OFF, 0, 0);
            EmChameleos_StartCurves(em, 0);
        }
        break;
    case 3:
        if (EmMotion_PassedFrameScaled(em, 58.0f, 0) != 0) {
            em->step++;
            em->stance = EM_STANCE_AIR;
        }
        break;
    case 4:
        EmChameleos_UpdateCurves(em);
        if (EmMotion_PassedFrameScaled(em, 72.0f, 0) != 0) {
            em->step++;
        }
        break;
    case 5:
        em->posY += 30.0f;
        if (!(em->posY < em->floorY + 600.0f * em->scaleY)) {
            em->step++;
            Em_SetMotion(em, CHAM_MOT_TAKE_OFF_END, 0, 0);
        }
    case 6:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, CHAM_MOT_HOVER, 0, 0);
            ((EmChameleosWork *)work)->flyGuard = 0;
            Player_ClearVelocity(em);
            em->velY = 10.0f;
            Em_SetCombat(em, 0, 0);
        }
        break;
    case 7:
        Em_MoveRotated(em, &em->rotX);
        if (!(em->posY <= 500.0f + em->floorY)) {
            if (em->velY < 100.0f) {
                em->velY += 0.75f;
            }
        }
        if (!(em->posY < 50000.0f)) {
            em->step++;
            Player_ClearVelocity(em);
            em->velZ = 10.0f;
        }
        break;
    case 8:
        Em_MoveRotated(em, &em->rotX);
        break;
    }
    groundY = em->floorY;
    if (em->posY < groundY) {
        em->posY = groundY;
    }
}

/*
 * CHAM_EVENT_LEAVE_AIR: the quest is over while flying: hovers, turns
 * invisible, leaves combat and rises out of sight as EmChameleos_LeaveQuest
 * does.
 */
void EmChameleos_LeaveQuestFromAir(Em *em, EmWork *work)
{
    f32 groundY;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, CHAM_MOT_HOVER, 0, 0);
        ((EmChameleosWork *)work)->flyGuard = 0;
        Player_ClearVelocity(em);
        em->velY = 10.0f;
        Em_SetCombat(em, 0, 0);
        EmChameleos_ClearFullCloak(em);
        break;
    case 1:
        Em_MoveRotated(em, &em->rotX);
        if (!(em->posY <= 500.0f + em->floorY)) {
            if (em->velY < 100.0f) {
                em->velY += 0.75f;
            }
        }
        if (!(em->posY < 50000.0f)) {
            em->step++;
            Player_ClearVelocity(em);
            em->velZ = 10.0f;
        }
        break;
    case 2:
        Em_MoveRotated(em, &em->rotX);
        break;
    }
    groundY = em->floorY;
    if (em->posY < groundY) {
        em->posY = groundY;
    }
}

/*
 * CHAM_DEFEAT_DIE: dies on the ground (CHAM_MOT_DIE, stance dead,
 * QuestEm_Hunted, EmChameleos_ClearFullCloak; invisibility is blocked in this
 * group anyway), then leaves a carcass mine point (EmMine_Create) for 1800
 * frames once it may reappear, removes it and fades out over 150 frames,
 * then the owner enters CHAM_DEFEAT_REAPPEAR.
 */
void EmChameleos_Die(Em *em, EmWork *work)
{
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, CHAM_MOT_DIE, 0, 0);
        EmChameleos_ClearFullCloak(em);
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_DEAD;
        QuestEm_Hunted(em);
        break;
    case 1:
        EmMotion_PassedFrameScaled(em, 212.0f, 0);
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
                EmChameleos_ChangeState(em, EM_GROUP_DEFEAT, CHAM_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    }
}

/* CHAM_DEFEAT_NONE: does nothing. */
void EmChameleos_DefeatNone(void)
{
}

/*
 * CHAM_DEFEAT_DIE_FALLING: killed in the air: falls (CHAM_MOT_FALL) to the
 * floor, CHAM_MOT_FALL_LAND, stance dead and QuestEm_Hunted, then makes the
 * carcass mine point; its following steps are numbered so that they are
 * never reached (see NOTE).
 */
void EmChameleos_DieFalling(Em *em, EmWork *work)
{
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_SetMotion(em, CHAM_MOT_FALL, 0, 0);
        EmChameleos_ClearFullCloak(em);
        if (em->velY > -50.0f) {
            em->velY = -50.0f;
        }
        em->accelY = -10.0f;
        work->moveAngCX = 0;
        work->moveAngX = 0;
        break;
    case 1:
        work->moveAngCY = em->rotY;
        Em_MoveRotatedAccel(em, &work->moveAngCX);
        if (em->posY <= em->floorY) {
            em->step++;
            em->posY = em->floorY;
            Em_SetMotion(em, CHAM_MOT_FALL_LAND, 0, 0);
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
        if (em->timer <= 0) {
            if (em->remote == 0) {
                EmChameleos_ChangeState(em, EM_GROUP_DEFEAT, CHAM_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    case 99:
        if (em->motion[0].playing == 0) {
            em->stance = EM_STANCE_GROUND;
            EmChameleos_EndState(em, 0, 0);
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
        }
        break;
    }
}

/*
 * CHAM_DEFEAT_REAPPEAR: when QuestEm_CanAppear allows (ChangeState also
 * calls QuestEm_Appear), places the monster at a fixed spot and enters
 * CHAM_IDLE; otherwise advances taskPhase and waits.
 */
void EmChameleos_Reappear(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        if (QuestEm_CanAppear(em) == 1) {
            em->rotY = 0x4861;
            em->posX = 3743.2f;
            em->posY = 1010.0f;
            em->posZ = 11415.0f;
            EmChameleos_ChangeState(em, EM_GROUP_IDLE, CHAM_IDLE, EM_SYNC_LOCAL);
            break;
        }
        em->taskPhase++;
    case 1:
        break;
    }
}

/*
 * Dispatcher of EM_GROUP_IDLE: runs the idle state function of em->state,
 * passing the work area to CHAM_IDLE_EAT_NEED1/_NEED2 with variant 0/1;
 * CHAM_IDLE_STUN_SHAKE_OFF and CHAM_IDLE_STATUS4_SHAKE_OFF share one
 * function.
 */
void EmChameleos_RunIdleState(Em *em, EmWork *work)
{
    switch (em->state) {
    case CHAM_IDLE_STAND:
        EmChameleos_Stand(em);
        break;
    case CHAM_IDLE:
        EmChameleos_Idle(em);
        break;
    case CHAM_IDLE_ALERT:
        EmChameleos_IdleAlert(em);
        break;
    case CHAM_IDLE_FACE_GOAL:
        EmChameleos_FaceGoal(em);
        break;
    case CHAM_IDLE_CLOAK_FULL:
        EmChameleos_CloakFull(em);
        break;
    case CHAM_IDLE_CLOAK_SHIMMER:
        EmChameleos_CloakShimmer(em);
        break;
    case CHAM_IDLE_ACTION_A:
        EmChameleos_IdleActionA(em);
        break;
    case CHAM_IDLE_EAT_NEED1:
        EmChameleos_Eat(em, work, 0);
        break;
    case CHAM_IDLE_EAT_NEED2:
        EmChameleos_Eat(em, work, 1);
        break;
    case CHAM_IDLE_REST:
        EmChameleos_Rest(em);
        break;
    case CHAM_IDLE_STATUS2_DOWN:
        EmChameleos_Status2Down(em);
        break;
    case CHAM_IDLE_REST_END:
        EmChameleos_RestEnd(em);
        break;
    case CHAM_IDLE_STATUS2_END:
        EmChameleos_Status2End(em);
        break;
    case CHAM_IDLE_SHAKE_OFF:
        EmChameleos_ShakeOff(em);
        break;
    case CHAM_IDLE_STATUS1_DOWN:
        EmChameleos_Status1Down(em);
        break;
    case CHAM_IDLE_STATUS1_END:
        EmChameleos_Status1End(em);
        break;
    case CHAM_IDLE_STUN_SHAKE_OFF:
        EmChameleos_StatusShakeOff(em);
        break;
    case CHAM_IDLE_STATUS4_SHAKE_OFF:
        EmChameleos_StatusShakeOff(em);
        break;
    }
}

/*
 * Dispatcher of EM_GROUP_MOVE: CHAM_MOVE_TURN/_TURN_RANDOM share a
 * function (variant 0/1), as do CHAM_MOVE_RUN/_RUN_NO_ATTACK;
 * CHAM_MOVE_NONE runs nothing.
 */
void EmChameleos_RunMoveState(Em *em, EmWork *work)
{
    switch (em->state) {
    case CHAM_MOVE_WALK:
        EmChameleos_Walk(em);
        break;
    case CHAM_MOVE_LIMP:
        EmChameleos_Limp(em, work);
        break;
    case CHAM_MOVE_TURN:
        EmChameleos_Turn(em, work, 0);
        break;
    case CHAM_MOVE_NONE:
        break;
    case CHAM_MOVE_RUN:
        EmChameleos_Run(em, (s32) work, 0);
        break;
    case CHAM_MOVE_SPIN_TURN:
        EmChameleos_SpinTurn(em);
        break;
    case CHAM_MOVE_BACK_HOP:
        EmChameleos_BackHop(em, work);
        break;
    case CHAM_MOVE_RUN_NO_HIT:
        EmChameleos_Run(em, (s32) work, 1);
        break;
    case CHAM_MOVE_TURN_RANDOM:
        EmChameleos_Turn(em, work, 1);
        break;
    }
}

/*
 * Dispatcher of EM_GROUP_FLY: one function per state; the
 * CHAM_FLY_NONE_* states run empty functions.
 */
void EmChameleos_RunFlyState(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmChameleos_TakeOff(em, work);
        break;
    case 1:
        EmChameleos_Land(em, work);
        break;
    case 2:
        EmChameleos_Hover(em, work);
        break;
    case 3:
        EmChameleos_FlyNoneA();
        break;
    case 4:
        EmChameleos_FlyNoneB();
        break;
    case 5:
        EmChameleos_FlyNoneC();
        break;
    case 6:
        EmChameleos_FlyRoute(em, work);
        break;
    case 7:
        EmChameleos_HoverToGoal(em, work);
        break;
    case 8:
        EmChameleos_FlyOutOfArea(em, work);
        break;
    case 9:
        EmChameleos_FlyIntoArea(em, work);
        break;
    case 10:
        EmChameleos_FlyNoneD();
        break;
    case 11:
        EmChameleos_FlyToGoalAndLand(em, work);
        break;
    case 12:
        EmChameleos_FlyNoneE();
        break;
    case 13:
        EmChameleos_FlyThroughArea(em, work);
        break;
    case 14:
        EmChameleos_Ascend(em, work);
        break;
    case 15:
        EmChameleos_FlyForward(em, work);
        break;
    case 16:
        EmChameleos_HoverTurnToGoal(em, work);
        break;
    case 17:
        EmChameleos_FlyRouteLoop(em, work);
        break;
    case 18:
        EmChameleos_BrakeAndLand(em, work);
        break;
    case 19:
        EmChameleos_FlyToGoal(em, work);
        break;
    case 20:
        EmChameleos_FlyNoneF();
        break;
    case 21:
        EmChameleos_StrafeToGoal(em, work);
        break;
    case 22:
        EmChameleos_Brake(em, work);
        break;
    case 23:
        EmChameleos_FlyNoneG();
        break;
    case 24:
        EmChameleos_Rise(em, work);

    default:
        break;
    }
}

/*
 * Dispatcher of EM_GROUP_ATTACK: the tongue, ball and breath attacks share
 * functions by variant (CHAM_ATK_TONGUE/_TONGUE_QUICK,
 * CHAM_ATK_BALL_PITCHED/_BREATH_A, CHAM_ATK_BALL_LEVEL/_BREATH_B). Takes
 * the work area as s32, as RunState passes it.
 */
void EmChameleos_RunAttackState(Em *em, s32 work)
{
    switch (em->state) {
    case CHAM_ATK_TONGUE:
        EmChameleos_Tongue(em, work, 0);
        break;
    case CHAM_ATK_TONGUE_LOW:
        EmChameleos_TongueLow(em);
        break;
    case CHAM_ATK_STOMP:
        EmChameleos_Stomp(em);
        break;
    case CHAM_ATK_TAIL_SLAM:
        EmChameleos_TailSlam(em);
        break;
    case CHAM_ATK_TONGUE_B:
        EmChameleos_TongueB(em);
        break;
    case CHAM_ATK_TONGUE_HIGH:
        EmChameleos_TongueHigh(em);
        break;
    case CHAM_ATK_BALL_PITCHED:
        EmChameleos_MistPitched(em, work, 0);
        break;
    case CHAM_ATK_BALL_LEVEL:
        EmChameleos_MistLevel(em, work, 0);
        break;
    case CHAM_ATK_BACK_HOP_BALL:
        EmChameleos_BackHopBall(em, (EmWork *) work);
        break;
    case CHAM_ATK_BREATH_PITCHED:
        EmChameleos_MistPitched(em, work, 1);
        break;
    case CHAM_ATK_BREATH_LEVEL:
        EmChameleos_MistLevel(em, work, 1);
        break;
    case CHAM_ATK_TONGUE_QUICK:
        EmChameleos_Tongue(em, work, 1);
        break;
    case CHAM_ATK_TAIL_POUNDS:
        EmChameleos_TailPounds(em);

    default:
        break;
    }
}

/*
 * Dispatcher of EM_GROUP_REACT: CHAM_REACT_STAGGER/_DECLOAK,
 * CHAM_REACT_KNOCK_DOWN/_STUN and CHAM_REACT_FALL/_FALL_BLIND/_DECLOAK_FALL
 * share functions by variant; CHAM_REACT_NONE_A/_B run empty functions.
 */
void EmChameleos_RunReactState(Em *em, EmWork *work)
{
    switch (em->state) {
    case CHAM_REACT_FLINCH_A:
        EmChameleos_FlinchA(em);
        break;
    case CHAM_REACT_FLINCH_B:
        EmChameleos_FlinchB(em);
        break;
    case CHAM_REACT_STAGGER:
        EmChameleos_Stagger(em, (s32) work, 0);
        break;
    case CHAM_REACT_KNOCK_DOWN:
        EmChameleos_KnockDown(em, work, 0);
        break;
    case CHAM_REACT_PART_BREAK:
        EmChameleos_PartBreak(em);
        break;
    case CHAM_REACT_STUN:
        EmChameleos_KnockDown(em, work, 1);
        break;
    case CHAM_REACT_NONE_A:
        EmChameleos_ReactNoneA();
        break;
    case CHAM_REACT_NONE_B:
        EmChameleos_ReactNoneB();
        break;
    case CHAM_REACT_FALL:
        EmChameleos_Fall(em, work, 0);
        break;
    case CHAM_REACT_FALL_BLIND:
        EmChameleos_Fall(em, work, 1);
        break;
    case CHAM_REACT_FLINCH_C:
        EmChameleos_FlinchC(em);
        break;
    case CHAM_REACT_STATUS4:
        EmChameleos_Status4(em);
        break;
    case CHAM_REACT_DECLOAK:
        EmChameleos_Stagger(em, (s32) work, 1);
        break;
    case CHAM_REACT_DECLOAK_FALL:
        EmChameleos_Fall(em, work, 2);
        break;
    }
}

/*
 * Dispatcher of EM_GROUP_DEFEAT: CHAM_DEFEAT_DIE, the empty
 * CHAM_DEFEAT_NONE, CHAM_DEFEAT_DIE_FALLING and CHAM_DEFEAT_REAPPEAR.
 */
void EmChameleos_RunDefeatState(Em *em, EmWork *work)
{
    switch (em->state) {
    case CHAM_DEFEAT_DIE:
        EmChameleos_Die(em, work);
        break;
    case CHAM_DEFEAT_NONE:
        EmChameleos_DefeatNone();
        break;
    case CHAM_DEFEAT_DIE_FALLING:
        EmChameleos_DieFalling(em, work);
        break;
    case CHAM_DEFEAT_REAPPEAR:
        EmChameleos_Reappear(em, work);
        break;
    }
}

/*
 * Dispatcher of EM_GROUP_EVENT: CHAM_EVENT_FOREST, CHAM_EVENT_LEAVE and
 * CHAM_EVENT_LEAVE_AIR; RunState also runs it for group 7.
 */
void EmChameleos_RunEventState(Em *em, EmWork *work)
{
    switch (em->state) {
    case CHAM_EVENT_FOREST:
        EmChameleos_ForestEvent(em, work);
        break;
    case CHAM_EVENT_LEAVE:
        EmChameleos_LeaveQuest(em, work);
        break;
    case CHAM_EVENT_LEAVE_AIR:
        EmChameleos_LeaveQuestFromAir(em, work);
        break;
    }
}

/*
 * The per-frame update: combat timer and area trigger, the low-hp check
 * (15%) and need-2 decay by map while calm, then maps Em_UpdateDamage's
 * result to a defeat, react or idle state, starts the Forest Hills scene
 * (CHAM_EVENT_FOREST) or the leaving states when the quest is won, runs the
 * state (twice on a state request), keeps guardTimer at 2 while flying with
 * flyGuard set, and updates the invisibility (hitzoneSet 1 while
 * invisible).
 */
void EmChameleos_Update(Em *em)
{
    u8 dmgOut[4];
    EmChameleosWork *w = (EmChameleosWork *)&em->work;
    u32 dmgResult;

    Em_UpdateCombatTimer(em);
    Em_RaisePlayerAreaTriggerLax(em);
    if (em->noAlert != 1 && em->rage == 0) {
        switch ((u16) Map_DayId(gScene.map)) {
        case MAP_FOREST_HILLS:
            Em_CheckLowHp(em, 0.15f);
            Em_DecayNeed2(em);
            break;
        case MAP_SWAMP:
            Em_CheckLowHp(em, 0.15f);
            break;
        case MAP_JUNGLE:
            Em_CheckLowHp(em, 0.15f);
            Em_DecayNeed2(em);
            break;
        }
    }
    dmgResult = Em_UpdateDamage(em, dmgOut) & 0xFF;
    switch (dmgResult) {
    case EM_DMG_NONE:
    case EM_DMG_ATTACK9:
    case EM_DMG_ATTACK6:
    case EM_DMG_CAPTURED:
    case EM_DMG_FLASH:
    case EM_DMG_HIT_MODE3:
    case EM_DMG_HIT:
    case EM_DMG_ATTACK13:
        break;
    case EM_DMG_KILLED:
    case EM_DMG_DEAD:
        if (em->stance == EM_STANCE_AIR) {
            EmChameleos_ChangeState(em, EM_GROUP_DEFEAT, CHAM_DEFEAT_DIE_FALLING, EM_SYNC_OWNER);
        } else {
            EmChameleos_ChangeState(em, EM_GROUP_DEFEAT, CHAM_DEFEAT_DIE, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_STATUS_IN_AIR:
        if (em->stateGroup != EM_GROUP_REACT || em->state != CHAM_REACT_FALL) {
            EmChameleos_ChangeState(em, EM_GROUP_REACT, CHAM_REACT_FALL, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_STATUS4:
        if ((em->stateGroup != EM_GROUP_REACT || em->state != CHAM_REACT_STATUS4) && (em->stateGroup != EM_GROUP_REACT || em->state != CHAM_REACT_FALL)) {
            Em_SetStatus4Timer(em);
            EmChameleos_ChangeState(em, EM_GROUP_REACT, CHAM_REACT_STATUS4, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_STATUS2:
        if ((em->stateGroup != EM_GROUP_IDLE || em->state != CHAM_IDLE_STATUS2_DOWN) && (em->stateGroup != EM_GROUP_REACT || em->state != CHAM_REACT_FALL)) {
            Em_SetStatus2Timer(em);
            EmChameleos_ChangeState(em, EM_GROUP_IDLE, CHAM_IDLE_STATUS2_DOWN, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_HIT_LYING:
        switch (em->state) {
        case 18:
            EmChameleos_ChangeState(em, EM_GROUP_IDLE, CHAM_IDLE_REST_END, EM_SYNC_OWNER);
            em->scriptPending = 0;
            break;
        case 20:
            EmChameleos_ChangeState(em, EM_GROUP_IDLE, CHAM_IDLE_STATUS2_END, EM_SYNC_OWNER);
            em->scriptPending = 0;
            break;
        }
        break;
    case EM_DMG_PART8_DOWN:
        EmChameleos_ChangeState(em, EM_GROUP_REACT, CHAM_REACT_PART_BREAK, EM_SYNC_OWNER);
        break;
    case EM_DMG_ATTACK5:
        if (EmChameleos_HasFullCloak(em) == 1 && (EmChameleos_WantsInvisible(em) & 0xFF) == 0 && (EmChameleos_CanTurnInvisible(em) & 0xFF) == 2) {
            if (em->stance == EM_STANCE_AIR) {
                EmChameleos_ChangeState(em, EM_GROUP_REACT, CHAM_REACT_DECLOAK_FALL, EM_SYNC_OWNER);
            } else {
                EmChameleos_ChangeState(em, EM_GROUP_REACT, CHAM_REACT_DECLOAK, EM_SYNC_OWNER);
            }
        }
        break;
    case EM_DMG_ATTACK5_FLINCH:
        if (EmChameleos_HasFullCloak(em) == 1 && EmChameleos_IsInvisible(em) == 0) {
            if (em->stance == EM_STANCE_AIR) {
                EmChameleos_ChangeState(em, EM_GROUP_REACT, CHAM_REACT_DECLOAK_FALL, EM_SYNC_OWNER);
            } else {
                EmChameleos_ChangeState(em, EM_GROUP_REACT, CHAM_REACT_DECLOAK, EM_SYNC_OWNER);
            }
            break;
        }
    case EM_DMG_PART_FLINCH:
        if (em->stance == EM_STANCE_AIR) {
            EmChameleos_ChangeState(em, EM_GROUP_REACT, CHAM_REACT_FALL, EM_SYNC_OWNER);
        } else {
            EmChameleos_BreakParts(em, 1);
            switch (em->flinchPart) {
            case 1:
            case 2:
            case 5:
                if (em->parts[em->flinchPart].state >= 2) {
                    EmChameleos_ChangeState(em, EM_GROUP_REACT, CHAM_REACT_STAGGER, EM_SYNC_OWNER);
                } else {
                    EmChameleos_ChangeState(em, EM_GROUP_REACT, CHAM_REACT_FLINCH_B, EM_SYNC_OWNER);
                }
                break;
            case 0:
            case 6:
                if (em->parts[em->flinchPart].state >= 2) {
                    EmChameleos_ChangeState(em, EM_GROUP_REACT, CHAM_REACT_STAGGER, EM_SYNC_OWNER);
                } else {
                    EmChameleos_ChangeState(em, EM_GROUP_REACT, CHAM_REACT_FLINCH_A, EM_SYNC_OWNER);
                }
                break;
            case 3:
            case 4:
            default:
                if (em->parts[em->flinchPart].state >= 2) {
                    EmChameleos_ChangeState(em, EM_GROUP_REACT, CHAM_REACT_KNOCK_DOWN, EM_SYNC_OWNER);
                } else {
                    EmChameleos_ChangeState(em, EM_GROUP_REACT, CHAM_REACT_FLINCH_B, EM_SYNC_OWNER);
                }
                break;
            }
        }
        break;
    case EM_DMG_STATUS5:
        if ((em->stateGroup != EM_GROUP_REACT || em->state != CHAM_REACT_FALL) && (em->stateGroup != EM_GROUP_REACT || em->state != CHAM_REACT_STUN) && (em->stateGroup != EM_GROUP_REACT || em->state != CHAM_REACT_STUN)) {
            Em_SetStatus5Timer(em);
            EmChameleos_ChangeState(em, EM_GROUP_REACT, CHAM_REACT_STUN, EM_SYNC_OWNER);
        }
        break;
    }
    switch (gQuest.stage) {
    case QUEST_ID_CHAMELEOS_SCENE_1:
    case QUEST_ID_CHAMELEOS_SCENE_2:
        if (Area_DayId(em->area) == AREA_FOREST_HILLS_9 && Save_TestEventFlag(EVENT_FLAG_CHAMELEOS_FOREST_SCENE) == 0) {
            if (Area_DayId(gScene.area) == AREA_FOREST_HILLS_9 && gScene.eventRunning == 1 && em->stateGroup != EM_GROUP_EVENT) {
                EmChameleos_ChangeState(em, EM_GROUP_EVENT, CHAM_EVENT_FOREST, EM_SYNC_SEND);
            }
            break;
        }
    default:
        if (Em_DamageResultIsReaction(em, dmgResult) == 1) {
            EmChameleos_BreakParts(em, 0);
        }
        if (Quest_GetOutcome(0) == 1 && em->stateGroup != EM_GROUP_EVENT && em->stateGroup != EM_GROUP_DEFEAT) {
            em->scriptPending = 0;
            if (em->stance == EM_STANCE_AIR) {
                EmChameleos_ChangeState(em, EM_GROUP_EVENT, CHAM_EVENT_LEAVE_AIR, EM_SYNC_LOCAL);
            } else {
                if (gScene.area == em->area) {
                    CamReq_ForMonster(em);
                }
                EmChameleos_ChangeState(em, EM_GROUP_EVENT, CHAM_EVENT_LEAVE, EM_SYNC_LOCAL);
            }
        }
        Em_UpdatePendingScript(em);
        break;
    }
    Em_ClearHitReactMode(em);
    EmChameleos_RunState(em, (s32) w);
    if (em->stateRequest != 0) {
        EmChameleos_RunState(em, (s32) w);
        em->stateRequest = 0;
    }
    if (em->stateGroup == EM_GROUP_FLY && (s8) w->flyGuard == 1) {
        em->guardTimer = 2;
    }
    EmChameleos_UpdateInvisibility(em);
    if (EmChameleos_IsInvisible(em) == 1) {
        em->hitzoneSet = 1;
    } else {
        em->hitzoneSet = 0;
    }
}

/*
 * Saves the previous group and state, then runs the dispatcher of
 * em->stateGroup (group 7 runs the event dispatcher).
 */
void EmChameleos_RunState(Em *em, s32 work)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->stateGroup) {
    case EM_GROUP_IDLE:
        EmChameleos_RunIdleState(em, (EmWork *) work);
        break;
    case EM_GROUP_MOVE:
        EmChameleos_RunMoveState(em, (EmWork *) work);
        break;
    case EM_GROUP_FLY:
        EmChameleos_RunFlyState(em, (EmWork *) work);
        break;
    case EM_GROUP_ATTACK:
        EmChameleos_RunAttackState(em, work);
        break;
    case EM_GROUP_REACT:
        EmChameleos_RunReactState(em, (EmWork *) work);
        break;
    case EM_GROUP_DEFEAT:
        EmChameleos_RunDefeatState(em, (EmWork *) work);
        break;
    case EM_GROUP_EVENT:
        EmChameleos_RunEventState(em, (EmWork *) work);
        break;
    case EM_GROUP_EVENT2:
        EmChameleos_RunEventState(em, (EmWork *) work);

    default:
        break;
    }
}

/*
 * When the flinching part (Em.flinchPart) is part 0, has state above 0 and
 * partFlags bit 0x20, flags it to break (Em_FlagPartBreak); with apply
 * nonzero also applies the flagged breaks (Em_ApplyPartBreaks).
 */
void EmChameleos_BreakParts(Em *em, s32 apply)
{
    u8 i = em->flinchPart;

    switch (i) {
    case 0:
        if ((s32) em->parts[i].state > 0 && (*(u8 *) (i + (u32) em + 1748) & 0x20)) {
            Em_FlagPartBreak(em, 0);
        }
        break;
    }
    if (apply != 0) {
        Em_ApplyPartBreaks(em);
    }
}

/*
 * When a player is in the monster's area, spawns ground effect kind
 * (PartFx_SpawnOnGround) under the given bone, at floor height, scaled by
 * scaleX.
 */
void EmChameleos_SpawnGroundEffect(Em *em, s32 bone, s32 kind, f32 scale)
{
    f32 pos[3];

    if (Player_InCurrentArea(em)) {
        Em_BonePos(em, bone, pos);
        pos[1] = em->floorY;
        PartFx_SpawnOnGround(pos, kind, scale * em->scaleX);
    }
}

/*
 * As SpawnGroundEffect, with an offset rotated by the monster's yaw and
 * scaled by its size added to the bone position (height from the floor).
 */
void EmChameleos_SpawnGroundEffectOffset(Em *em, s32 bone, s32 kind, f32 *ofs, f32 scale)
{
    f32 pos[3];
    f32 rotOfs[3];

    if (Player_InCurrentArea(em)) {
        Em_BonePos(em, bone, pos);
        Vec_Copy(rotOfs, ofs);
        Vec_RotateY(rotOfs, 6.2831855f * em->rotY / 65536.0f);
        pos[0] += rotOfs[0] * em->scaleX;
        pos[1] = em->floorY + rotOfs[1] * em->scaleY;
        pos[2] += rotOfs[2] * em->scaleZ;
        PartFx_SpawnOnGround(pos, kind, scale * em->scaleX);
    }
}

/*
 * While within 1000 of the floor, spawns the ground effect
 * (SpawnGroundEffect) and, with withImpact set, an EmImpactAttack of kind 3
 * on the floor under the monster.
 */
void EmChameleos_SpawnGroundEffectNearGround(Em *em, s32 bone, s32 kind, s32 withImpact, f32 scale)
{
    f32 pos[3];

    if (em->posY <= 1000.0f + em->floorY) {
        EmChameleos_SpawnGroundEffect(em, bone, kind, scale);
        if (withImpact & 0xFF) {
            pos[0] = em->posX;
            pos[1] = em->floorY;
            pos[2] = em->posZ;
            EmImpactAttack_CreateAt(em, (f32 *) pos, 3);
        }
    }
}

/*
 * While within 1000 of the floor, makes a dust cloud (DustCloud_Create,
 * scaled by scaleX) and, with withImpact set, an EmImpactAttack of kind 3
 * on the floor under the monster.
 */
void EmChameleos_SpawnDustNearGround(Em *em, s32 dustKind, f32 scale, s32 dustArg, s32 withImpact)
{
    f32 pos[3];

    if (em->posY <= 1000.0f + em->floorY) {
        DustCloud_Create(em, dustKind, dustArg, scale * em->scaleX);
        if (withImpact & 0xFF) {
            pos[0] = em->posX;
            pos[1] = em->floorY;
            pos[2] = em->posZ;
            EmImpactAttack_CreateAt(em, (f32 *) pos, 3);
        }
    }
}

/*
 * In the walking, idle, limp and some attack and recover motions, while
 * enraged, makes MonsterFx 13 on the monster every 45 frames.
 */
void EmChameleos_SpawnRageFx(Em *em, s16 motion)
{
    switch (motion) {
    case EM_MOTION_ID(0, CHAM_MOT_IDLE):
    case EM_MOTION_ID(0, CHAM_MOT_WALK):
    case EM_MOTION_ID(0, 5):
    case EM_MOTION_ID(0, 6):
    case EM_MOTION_ID(0, CHAM_MOT_SPIN_TURN):
    case EM_MOTION_ID(0, 20):
    case EM_MOTION_ID(0, CHAM_MOT_IDLE_ALERT):
    case EM_MOTION_ID(0, CHAM_MOT_TURN_TO_GOAL):
    case EM_MOTION_ID(0, CHAM_MOT_IDLE_ACTION_A):
    case EM_MOTION_ID(0, CHAM_MOT_CLOAK):
    case EM_MOTION_ID(0, CHAM_MOT_LIMP):
    case EM_MOTION_ID(0, CHAM_MOT_SPIT_A_END):
    case EM_MOTION_ID(0, CHAM_MOT_TAIL_POUNDS):
    case EM_MOTION_ID(0, CHAM_MOT_SPIT_B_END):
    case EM_MOTION_ID(0, CHAM_MOT_SHAKE_OFF):
    case EM_MOTION_ID(0, CHAM_MOT_PART_BREAK_SPIN):
        if (em->rage != 0 && gScene.frame % 45 == 0) {
            MonsterFx_CreateOnEm(em, 13, 2.0f * em->scaleX);
        }
        break;
    }
}

/* Empty: the default case of MotionEffects. */
static void EmChameleos_MotionEffectsDefault(Em *em)
{
}

/*
 * Keeps the layer-0 motion id in fxMotion, plays its sounds (MotionSounds),
 * the rage effect (SpawnRageFx), and per motion at set frames makes the
 * ChameleosAttack hit objects, ground effects, dust clouds and impact
 * attacks of the attacks, landings and steps.
 */
void EmChameleos_MotionEffects(Em *em, EmChameleosWork *work)
{
    f32 pos[3];
    f32 ofs[3];

    if (em->motionId[0] != work->fxMotion) {
        work->fxMotion = em->motionId[0];
    }
    EmChameleos_MotionSounds(em, (EmWork *) work);
    EmChameleos_SpawnRageFx(em, work->fxMotion);
    switch (work->fxMotion) {
    case EM_MOTION_ID(0, CHAM_MOT_RUN):
        if (EmMotion_PassedFrameScaled(em, 52.0f, 0)) {
            EmChameleos_SpawnGroundEffect(em, CHAM_BONE_FOOT_A, 20, 4.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 70.0f, 0)) {
            EmChameleos_SpawnGroundEffect(em, CHAM_BONE_FOOT_B, 20, 4.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 66.0f, 0)) {
            EmChameleos_SpawnGroundEffect(em, CHAM_BONE_FOOT_C, 20, 4.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 48.0f, 0)) {
            EmChameleos_SpawnGroundEffect(em, CHAM_BONE_FOOT_D, 20, 4.0f);
        }
        break;
    case EM_MOTION_ID(0, CHAM_MOT_RUN_STOP):
        if (EmMotion_PassedFrameScaled(em, 24.0f, 0)) {
            EmChameleos_SpawnGroundEffect(em, 1, 7, 8.0f);
        }
        break;
    case EM_MOTION_ID(0, CHAM_MOT_SPIN_TURN):
        if (EmMotion_PassedFrameScaled(em, 110.0f, 0)) {
            EmChameleos_SpawnGroundEffect(em, 1, 7, 8.0f);
        }
        break;
    case EM_MOTION_ID(0, CHAM_MOT_BACK_HOP):
        if (EmMotion_PassedFrameScaled(em, 50.0f, 0)) {
            DustCloud_Create(em, 2, 41, 1.5f * em->scaleX);
        }
        if (EmMotion_PassedFrameScaled(em, 86.0f, 0)) {
            EmChameleos_SpawnGroundEffect(em, 1, 7, 8.0f);
        }
        break;
    case EM_MOTION_ID(0, CHAM_MOT_TAKE_OFF):
        if (EmMotion_PassedFrameScaled(em, 60.0f, 0)) {
            DustCloud_Create(em, 19, 0, em->scaleX);
        }
        break;
    case EM_MOTION_ID(0, 11):
        if (EmMotion_PassedFrameScaled(em, 64.0f, 0)) {
            EmChameleos_SpawnGroundEffectNearGround(em, 1, 7, 0, 8.0f);
        }
        break;
    case EM_MOTION_ID(0, CHAM_MOT_BRAKE):
        if (EmMotion_PassedFrameScaled(em, 22.0f, 0)) {
            EmChameleos_SpawnDustNearGround(em, 1, 1.0f, 0, 0);
        }
        if (EmMotion_PassedFrameScaled(em, 92.0f, 0)) {
            EmChameleos_SpawnGroundEffectNearGround(em, 1, 7, 0, 8.0f);
        }
        break;
    case EM_MOTION_ID(0, CHAM_MOT_LAND):
        if (EmMotion_PassedFrameScaled(em, 32.0f, 0)) {
            EmChameleos_SpawnDustNearGround(em, 11, 1.0f, 0, 1);
        }
        break;
    case EM_MOTION_ID(0, CHAM_MOT_HOVER):
        if (EmMotion_PassedFrameScaled(em, 36.0f, 0)) {
            EmChameleos_SpawnDustNearGround(em, 1, 1.0f, 0, 1);
        }
        break;
    case EM_MOTION_ID(0, CHAM_MOT_TAKE_OFF_END):
        if (EmMotion_PassedFrameScaled(em, 40.0f, 0)) {
            EmChameleos_SpawnGroundEffectNearGround(em, 1, 7, 0, 8.0f);
        }
        break;
    case EM_MOTION_ID(0, CHAM_MOT_TONGUE_LOW):
        if (EmMotion_PassedFrameScaled(em, 66.0f, 0)) {
            ChameleosAttack_Create(em, 2);
        }
        if (EmMotion_PassedFrameScaled(em, 68.0f, 0)) {
            ChameleosAttack_Create(em, 8);
        }
        if (EmMotion_PassedFrameScaled(em, 74.0f, 0) || EmMotion_PassedFrameScaled(em, 102.0f, 0)) {
            DustCloud_Create(em, 2, 43, 1.5f * em->scaleX);
        }
        break;
    case EM_MOTION_ID(0, CHAM_MOT_SPIT_START):
        if (EmMotion_PassedFrameScaled(em, 86.0f, 0)) {
            ChameleosAttack_Create(em, 9);
        }
        break;
    case EM_MOTION_ID(0, CHAM_MOT_STOMP):
        if (EmMotion_PassedFrameScaled(em, 42.0f, 0)) {
            ChameleosAttack_Create(em, 3);
        }
        if (EmMotion_PassedFrameScaled(em, 44.0f, 0) || EmMotion_PassedFrameScaled(em, 48.0f, 0) || EmMotion_PassedFrameScaled(em, 52.0f, 0)) {
            EmChameleos_SpawnGroundEffect(em, CHAM_BONE_FOOT_D, 20, 8.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 50.0f, 0)) {
            DustCloud_Create(em, 2, 47, 1.5f * em->scaleX);
        }
        break;
    case EM_MOTION_ID(0, CHAM_MOT_TAIL_SLAM):
        if (EmMotion_PassedFrameScaled(em, 72.0f, 0)) {
            ChameleosAttack_Create(em, 4);
        }
        if (EmMotion_PassedFrameScaled(em, 84.0f, 0)) {
            EmChameleos_SpawnGroundEffect(em, CHAM_BONE_TAIL_C, 12, 8.0f);
            Em_BonePos(em, CHAM_BONE_TAIL_C, pos);
            pos[1] = em->floorY;
            if ((u8)(s32)Player_InCurrentArea(em)) {
                AttackBurst_CreateAtScaled(pos, *(u16 *) &em->rotY, 10, 1.0f);
            }
            EmImpactAttack_CreateAt(em, (f32 *) pos, 3);
        }
        break;
    case EM_MOTION_ID(0, CHAM_MOT_TAIL_POUNDS):
        if (EmMotion_PassedFrameScaled(em, 66.0f, 0) || EmMotion_PassedFrameScaled(em, 162.0f, 0) || EmMotion_PassedFrameScaled(em, 258.0f, 0) || EmMotion_PassedFrameScaled(em, 356.0f, 0)) {
            EmChameleos_SpawnGroundEffect(em, CHAM_BONE_TAIL_C, 7, 8.5f);
            Em_BonePos(em, CHAM_BONE_TAIL_C, pos);
            pos[1] = em->floorY;
            if ((u8)(s32)Player_InCurrentArea(em)) {
                AttackBurst_CreateAtScaled(pos, *(u16 *) &em->rotY, 10, 1.0f);
            }
            EmImpactAttack_CreateAt(em, (f32 *) pos, 3);
        }
        if (EmMotion_PassedFrameScaled(em, 64.0f, 0) || EmMotion_PassedFrameScaled(em, 160.0f, 0) || EmMotion_PassedFrameScaled(em, 256.0f, 0) || EmMotion_PassedFrameScaled(em, 354.0f, 0)) {
            ChameleosAttack_Create(em, 7);
        }
        break;
    case EM_MOTION_ID(0, CHAM_MOT_TONGUE):
        if (EmMotion_PassedFrameScaled(em, 28.0f, 0)) {
            ChameleosAttack_Create(em, 1);
        }
        if (EmMotion_PassedFrameScaled(em, 38.0f, 0)) {
            ofs[0] = 0.0f;
            ofs[1] = 0.0f;
            ofs[2] = 200.0f;
            EmChameleos_SpawnGroundEffectOffset(em, CHAM_BONE_TONGUE, 20, ofs, 8.0f);
        }
        break;
    case EM_MOTION_ID(0, CHAM_MOT_TONGUE_B):
        if (EmMotion_PassedFrameScaled(em, 22.0f, 0)) {
            ChameleosAttack_Create(em, 5);
        }
        if (EmMotion_PassedFrameScaled(em, 28.0f, 0)) {
            ChameleosAttack_Create(em, 1);
        }
        if (EmMotion_PassedFrameScaled(em, 28.0f, 0)) {
            EmChameleos_SpawnGroundEffect(em, CHAM_BONE_FOOT_D, 20, 4.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 38.0f, 0)) {
            ofs[0] = 0.0f;
            ofs[1] = 0.0f;
            ofs[2] = 200.0f;
            EmChameleos_SpawnGroundEffectOffset(em, CHAM_BONE_TONGUE, 20, ofs, 8.0f);
        }
        break;
    case EM_MOTION_ID(0, CHAM_MOT_TONGUE_HIGH):
        if (EmMotion_PassedFrameScaled(em, 26.0f, 0)) {
            ChameleosAttack_Create(em, 1);
        }
        if (EmMotion_PassedFrameScaled(em, 30.0f, 0)) {
            ofs[0] = 0.0f;
            ofs[1] = 0.0f;
            ofs[2] = 350.0f;
            EmChameleos_SpawnGroundEffectOffset(em, CHAM_BONE_TONGUE, 20, ofs, 4.0f);
        }
        break;
    case EM_MOTION_ID(0, CHAM_MOT_RECOVER):
        if (EmMotion_PassedFrameScaled(em, 64.0f, 0)) {
            EmChameleos_SpawnGroundEffect(em, 1, 7, 4.0f);
        }
        break;
    case EM_MOTION_ID(0, CHAM_MOT_FALL_LAND):
        if (EmMotion_PassedFrameScaled(em, 12.0f, 0)) {
            DustCloud_Create(em, 11, 0, em->scaleX);
        }
        break;
    case EM_MOTION_ID(0, CHAM_MOT_DOWN):
        if (EmMotion_PassedFrameScaled(em, 62.0f, 0)) {
            EmChameleos_SpawnGroundEffect(em, 1, 7, 8.0f);
        }
        break;
    case EM_MOTION_ID(0, CHAM_MOT_DIE):
        if (EmMotion_PassedFrameScaled(em, 468.0f, 0)) {
            DustCloud_Create(em, 0, 0, em->scaleX);
        }
        break;
    case EM_MOTION_ID(0, CHAM_MOT_STATUS_DOWN):
        if (EmMotion_PassedFrameScaled(em, 182.0f, 0)) {
            DustCloud_Create(em, 0, 0, em->scaleX);
        }
        break;
    case EM_MOTION_ID(0, CHAM_MOT_COLLAPSE):
        if (EmMotion_PassedFrameScaled(em, 166.0f, 0)) {
            DustCloud_Create(em, 0, 0, em->scaleX);
        }
        break;
    case EM_MOTION_ID(0, CHAM_MOT_PART_BREAK):
        if (EmMotion_PassedFrameScaled(em, 54.0f, 0)) {
            DustCloud_Create(em, 0, 0, em->scaleX);
        }
        break;
    default:
        EmChameleos_MotionEffectsDefault(em);
        break;
    }
}

/*
 * Skips the first frame (fxStep 0 to 1), then runs MotionEffects every
 * frame.
 */
void EmChameleos_UpdateMotionEffects(Em *em)
{
    EmChameleosWork *w = (EmChameleosWork *)&em->work;

    switch (w->fxStep) {
    case 0:
        w->fxStep++;
        break;
    case 1:
        EmChameleos_MotionEffects(em, (EmChameleosWork *) w);
        break;
    }
}

/* Empty callback of the family's callback table. */
void EmChameleos_TableNop(void)
{
}

/*
 * Answers the script's query k: 0 whether the full cloak is on
 * (HasFullCloak), 1 the current map's day id, 2 the invisibility mode
 * (WantsInvisible); returns 0 otherwise.
 */
s32 EmChameleos_ScriptQuery(Em *em, u8 query)
{
    switch (query) {
    case 0:
        return EmChameleos_HasFullCloak(em) == 1;
    case 1:
        return (u8) Map_DayId(gScene.map);
    case 2:
        return EmChameleos_WantsInvisible(em);
    }
    return 0;
}

/*
 * Part-break callback: returns 1 when not in the air, hp is at most 30% of
 * the maximum and the part-break reaction has not run yet (partBreakCount
 * 0), else 0.
 */
s32 EmChameleos_AllowPartBreak(Em *em)
{
    if (em->stance != EM_STANCE_AIR && em->hp <= (s16) (0.3f * em->hpMax) && em->partBreakCount == 0) {
        return 1;
    }
    return 0;
}

/* The file's initialised data, in the game's order */
EmFlyParam EmChameleos_FlyParam = {
#include "game/008CAB00.inc"
};
EmFlyParam EmChameleos_FlyParam2 = {
#include "game/008CAB60.inc"
};
EmTurnParams EmChameleos_TurnParams[1] = {
#include "game/008CABC0.inc"
};
void *EmChameleos_Callbacks[8] = {
    EmChameleos_SpawnEffects, EmChameleos_SpawnEffects, EmChameleos_TableNop, EmChameleos_UpdateMotionEffects,
    EmChameleos_ScriptQuery, EmChameleos_Reinit, EmChameleos_AllowPartBreak, NULL,
};
