/*
 * The Blangonga family: Blangonga, Congalala and Rajang, and the small
 * Blango and Conga (EM_KIND_BLANGONGA, _CONGALALA, _RAJANG, _BLANGO,
 * _CONGA): the code from the family's first function,
 * EmBlangonga_MotionSounds, to the next family's (EmBlangonga_Setup, called
 * by the per-kind dispatch Em_TypeSetup, sets a monster up), to Shen
 * Gaoren's state changes.
 *
 * A monster of the family runs a state machine (include/em_blangonga.h
 * names the states and motions; include/em.h the groups): EmBlangonga_Update
 * reads the hits (Em_UpdateDamage) and picks the reactions, then
 * EmBlangonga_RunState calls the group's dispatcher (EmBlangonga_RunIdleState
 * ... RunVersusState), which calls one function per state with the monster,
 * its work area (EmWork; the family's view is EmBlangongaWork) and, for the
 * functions several states share, a variant. A state function switches on
 * Em.step; EmBlangonga_ChangeState starts the next state and
 * EmBlangonga_EndState hands the choice back to the monster script, which
 * asks its questions through EmBlangonga_ScriptQuery. The family doesn't
 * fly: its group 2 holds the jumps, the leaps between areas and the
 * Blangonga kinds' burrowing under the snow. Congalala carries one of three
 * items it can drop as a mine point, and eats to change its breath
 * (EmBlangongaWork.carryHidden, eatenKind); Blangonga's and Congalala's
 * story quests play scenes (group 6). EmBlangonga_MotionEffects adds each
 * motion's hits and effects, and EmBlangonga_MotionSounds,
 * MotionSoundsCongalala and MotionSoundsRajang its sounds.
 */
#include "common.h"
#include "scene.h"
#include "quest.h"
#include "em.h"
#include "shadow.h"
#include "em_blangonga.h"
#include "player.h"
#include "objattach.h"
#include "area.h"
#include "capsule.h"
#include "save.h"
#include "sound.h"
#include "cam_shake.h"
#include "dust_flash_effects.h"
#include "engine_math.h"
#include "game_util.h"
#include "gm_em.h"
#include "hit_effects.h"
#include "monster_effects.h"
#include "part_effects.h"
#include "rand.h"
#include "vibration.h"
#include "vec_math.h"
#include "em_script.h"
/* prototypes of callees */

/* prototypes */
/* unit's own declaration: declared here as the callers use it; with the definition's void (Em *em, s16 n), the calls in EmBlangonga_EatNeed2 compile differently */
void Em_AddHp(Em *em, s32 n);
/* unit's own declaration: declared here as the callers use it; with the definition's void (Em *em, s32 bone, s32 a, s32 b, s32 c, s32 d), the calls in EmBlangonga_DigThrow compile differently */
void EmRangedAttack_CreateAtBone(Em *, s32, s32, s32, u16, s32);
/* unit's own declaration: declared here as the callers use it; with the definition's void *(Em *em, s32 kind, s32 c, s32 a, s32 b), the calls in EmBlangonga_CongalalaEatenEmission compile differently */
void * EmEffectAttack_Create(Em *, s16, s16, s32, s32);
/* unit's own declaration: declared here as the callers use it; with the definition's void (Em *em, s32 group, s32 state, u8 mode), the calls in EmBlangonga_ChangeState, EmKushala_ChangeState and others compile differently */
void Em_SetStateSync(Em *em, u16 group, u16 state, u8 mode);
/* unit's own declaration: declared here as the callers use it; with the definition's s32 (Em *em, s16 n, s16 max, s16 interval), the calls in EmBlangonga_Rest compile differently */
s32 Em_RegenHp(Em *em, s32 n, s32 max, s32 interval);
s32 Em_PartBroken(Em *em, s32 arg1);
/* unit's own declaration: declared here as the callers use it; with the definition's void (Em *em, s32 bone, f32 *pos, f32 scale), the calls in EmBlangonga_SpawnLyingPuffs compile differently */
void Em_SpawnLyingPuffs(Em *em, u8 bone, f32 *pos, f32 scale);
EmBone *Em_Bone(void *obj, s16 bone);
extern EmBlangongaTurnData EmBlangonga_TurnData;
extern Vec3f EmBlangonga_CarveThrowPt __attribute__((aligned(8)));
/* unit's own declaration: declared here as the callers use it; with the definition's void (Em *em, u8 k, s16 v, s16 t, f32 y, f32 scale), the calls in EmBlangonga_KnockDown and EmBlangonga_HeldDowned compile differently */
void StunStars_CreateEm(Em *, s32, s32, s32, f32, f32);
s32 EmMotion_PassedFrameScaled(void *p, f32 frame, s32 layer);
/* the calls that mask the value again (u8 or & 0xFF) cast it to s32 first: MWCC keeps the game's andi that way */
/* unit's own declaration: Em_EndAllStatus reads a0 = em at EmKushala_SetReactState state 6 (left by EmKushala_SetWind(em, ...), which keeps a0) */
void Em_EndAllStatus();
/* unit's own declaration: declared here as the callers use it; with the definition's void (void *obj, s16 bone, f32 *out), the calls in EmBlangonga_SpawnGroundEffect compile differently */
void Em_BonePos(void *obj, s32 a, f32 *out);
struct EmIdEntry;
struct EmIdEntry *Em_FindAreaEntry(Em *em, struct EmIdEntry *entry, s16 id);
extern f32 gStartPos[];
/* unit's own declaration: declared here as the callers use it; with the definition's s16 (void *obj, u8 state, u8 sub), the calls in EmBlangonga_Update compile differently */
s32 Player_IsState(void *obj, s32 group, s32 state);
static s32 EmBlangonga_IsBlangonga(Em *em);
static s32 EmBlangonga_IsCongalala(Em *em);
u16 Area_IdForCurrentTime(u16);
/* end of prototypes */

/*
 * Plays the sounds of the motion on layer 0 at its frames
 * (Em_PlaySeAtBoneOnFrame: frame, sound id, bone) for Blangonga and Blango
 * (EmBlangonga_MotionEffects calls it each frame), switching on the motion
 * id in EmWork.prevMotion.
 */
void EmBlangonga_MotionSounds(Em *em, EmWork *work)
{
    s16 prevMotion;

    prevMotion = work->prevMotion;
    switch (prevMotion) {
    case EM_MOTION_ID(0, BLANG_MOT_WALK):
        Em_PlaySeAtBoneOnFrame(em, 44, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 106, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 158, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 48, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 94, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 144, 3, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TURN_LEFT):
        Em_PlaySeAtBoneOnFrame(em, 22, Sound_RandomSe(25, 2, 25, 2), BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 18, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 50, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 82, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 14, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 50, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 80, 3, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TURN_RIGHT):
        Em_PlaySeAtBoneOnFrame(em, 22, Sound_RandomSe(25, 2, 25, 2), BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 18, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 50, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 82, 6, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 14, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 50, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 80, 3, BLANG_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TURN_LEFT_QUICK):
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(44, 1, 44, 2), BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 20, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 26, 2, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 14, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 28, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 38, 3, BLANG_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TURN_RIGHT_QUICK):
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(44, 1, 44, 2), BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 20, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 26, 2, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 14, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 28, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 38, 3, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_LIMP):
        Em_PlaySeAtBoneOnFrame(em, 10, 45, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 70, 45, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 10, 1, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 46, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 68, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 104, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 14, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 18, 18, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 44, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 74, 18, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 76, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 108, 3, BLANG_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_MOVE_ACTION_D):
        Em_PlaySeAtBoneOnFrame(em, 4, 29, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 12, 20, 3);
        Em_PlaySeAtBoneOnFrame(em, 14, 5, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 28, 2, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 32, 1, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 66, 1, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 84, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 96, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 34, 5, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 38, 4, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 62, 4, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 82, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 96, 3, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_MOVE_ACTION_A):
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(29, 1, 30, 1), BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 12, 20, 3);
        Em_PlaySeAtBoneOnFrame(em, 14, 5, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 32, 2, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 64, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 30, 5, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_MOVE_ACTION_B):
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(29, 1, 30, 1), BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 12, 20, 3);
        Em_PlaySeAtBoneOnFrame(em, 14, 5, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 32, 2, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 64, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 30, 5, BLANG_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_MOVE_ACTION_C):
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(29, 1, 30, 1), BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 12, 20, 3);
        Em_PlaySeAtBoneOnFrame(em, 14, 5, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 42, 3, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 46, 1, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 88, 1, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 38, 5, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 42, 4, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 52, 4, BLANG_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_JUMP):
        if (em->type == EM_KIND_BLANGO) {
            Em_PlaySeAtBoneVolOnFrame(em, 22, 54, BLANG_BONE_HEAD, 7);
            Em_PlaySeAtBoneOnFrame(em, 4, 6, BLANG_BONE_HAND_A);
            Em_PlaySeAtBoneOnFrame(em, 8, 18, BLANG_BONE_HAND_A);
            Em_PlaySeAtBoneOnFrame(em, 8, 3, BLANG_BONE_FOOT_A);
            Em_PlaySeAtBoneOnFrame(em, 50, 5, BLANG_BONE_FOOT_A);
            Em_PlaySeAtBoneOnFrame(em, 52, 4, BLANG_BONE_FOOT_B);
            Em_PlaySeAtBoneOnFrame(em, 52, 20, 3);
            break;
        }
        Em_PlaySeAtBoneVolOnFrame(em, 22, 54, BLANG_BONE_HEAD, 7);
        Em_PlaySeAtBoneVolOnFrame(em, 54, 36, BLANG_BONE_HEAD, 7);
        Em_PlaySeAtBoneVolOnFrame(em, 54, 63, BLANG_BONE_HEAD, 7);
        Em_PlaySeAtBoneVolOnFrame(em, 70, 64, BLANG_BONE_HEAD, 7);
        Em_PlaySeAtBoneOnFrame(em, 4, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 8, 18, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 8, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 50, 5, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 52, 4, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 52, 20, 3);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_CHARGE):
        Em_PlaySeAtBoneOnFrame(em, 2, 30, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 40, Sound_RandomSe(29, 3, 30, 1), BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 22, 2, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 24, 1, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 56, 2, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 58, 1, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 30, 5, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 32, 4, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 8, 5, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 36, 5, BLANG_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_JUMP_LAND):
        Em_PlaySeAtBoneOnFrame(em, 12, 55, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 8, BLANG_SE_THUD, 3);
        Em_PlaySeAtBoneOnFrame(em, 6, 2, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 68, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 94, 6, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 100, 3, BLANG_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DIG_IN):
        Em_PlaySeAtBoneOnFrame(em, 18, 16, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 64, 16, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 90, 17, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 104, 17, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 118, 17, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 132, 17, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 146, 18, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 158, 18, BLANG_BONE_HAND_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_EMERGE):
        if (em->type == EM_KIND_BLANGO) {
            Em_PlaySeAtBoneVolOnFrame(em, 4, 13, BLANG_BONE_BREATH, 6);
            Em_PlaySeAtBoneVolOnFrame(em, 4, 26, BLANG_BONE_HEAD, 6);
            Em_PlaySeAtBoneOnFrame(em, 88, 44, BLANG_BONE_HEAD);
            Em_PlaySeAtBoneOnFrame(em, 4, 20, BLANG_BONE_BREATH);
            Em_PlaySeAtBoneOnFrame(em, 52, 2, BLANG_BONE_HAND_A);
            Em_PlaySeAtBoneOnFrame(em, 60, 1, BLANG_BONE_HAND_B);
            Em_PlaySeAtBoneOnFrame(em, 130, 6, BLANG_BONE_HAND_A);
            Em_PlaySeAtBoneOnFrame(em, 146, 0, BLANG_BONE_HAND_B);
            Em_PlaySeAtBoneOnFrame(em, 56, 5, BLANG_BONE_FOOT_B);
            Em_PlaySeAtBoneOnFrame(em, 60, 4, BLANG_BONE_FOOT_A);
            Em_PlaySeAtBoneOnFrame(em, 122, 3, BLANG_BONE_FOOT_A);
            Em_PlaySeAtBoneOnFrame(em, 56, BLANG_SE_THUD, BLANG_BONE_FOOT_A);
            break;
        }
        if ((em->stateGroup == 0) && (em->state == 14)) {
            Em_PlaySeAtBoneOnFrame(em, 20, 20, BLANG_BONE_BREATH);
            Em_PlaySeAtBoneOnFrame(em, 18, BLANG_SE_FALL, BLANG_BONE_BREATH);
            Em_PlaySeAtBoneOnFrame(em, 18, 26, BLANG_BONE_HEAD);
            Em_PlaySeAtBoneOnFrame(em, 88, 44, BLANG_BONE_HEAD);
            Em_PlaySeAtBoneOnFrame(em, 18, 20, BLANG_BONE_BREATH);
            Em_PlaySeAtBoneOnFrame(em, 52, 2, BLANG_BONE_HAND_A);
            Em_PlaySeAtBoneOnFrame(em, 60, 1, BLANG_BONE_HAND_B);
            Em_PlaySeAtBoneOnFrame(em, 130, 6, BLANG_BONE_HAND_A);
            Em_PlaySeAtBoneOnFrame(em, 146, 0, BLANG_BONE_HAND_B);
            Em_PlaySeAtBoneOnFrame(em, 56, 5, BLANG_BONE_FOOT_B);
            Em_PlaySeAtBoneOnFrame(em, 60, 4, BLANG_BONE_FOOT_A);
            Em_PlaySeAtBoneOnFrame(em, 122, 3, BLANG_BONE_FOOT_A);
            Em_PlaySeAtBoneOnFrame(em, 56, BLANG_SE_THUD, BLANG_BONE_FOOT_A);
            break;
        }
        Em_PlaySeAtBoneOnFrame(em, 6, 23, BLANG_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame(em, 4, BLANG_SE_FALL, BLANG_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame(em, 4, 26, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 88, 44, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 4, 20, BLANG_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame(em, 52, 2, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 60, 1, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 130, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 146, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 56, 5, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 60, 4, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 122, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 56, BLANG_SE_THUD, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_WALK_START):
        Em_PlaySeAtBoneOnFrame(em, 44, 1, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 36, 3, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_LIE):
        Em_PlaySeAtBoneOnFrame(em, 2, 42, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 100, 41, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 162, 42, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 260, 41, BLANG_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_GET_UP):
        Em_PlaySeAtBoneOnFrame(em, 2, 16, 3);
        Em_PlaySeAtBoneOnFrame(em, 2, 43, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 130, 53, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 30, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 78, 15, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 140, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 160, 6, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 76, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 110, 3, BLANG_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_COLLAPSE):
        Em_PlaySeAtBoneOnFrame(em, 40, 51, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 160, 41, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 220, 42, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 2, 15, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 62, 6, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 66, 18, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 84, BLANG_SE_THUD, 11);
        Em_PlaySeAtBoneOnFrame(em, 118, 16, 11);
        Em_PlaySeAtBoneOnFrame(em, 148, 1, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 164, 4, BLANG_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_LIE_DOWN):
        Em_PlaySeAtBoneOnFrame(em, 4, 50, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 60, 51, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 4, 6, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 58, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 162, 16, 11);
        Em_PlaySeAtBoneOnFrame(em, 180, BLANG_SE_FALL, 3);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TRIPLE_HIT):
        Em_PlaySeAtBoneOnFrame(em, 2, 28, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 60, 97, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 28, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 40, 1, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 40, 1, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 66, 5, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 66, 20, 3);
        Em_PlaySeAtBoneOnFrame(em, 88, 12, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 88, 61, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 94, BLANG_SE_THUD, 11);
        Em_PlaySeAtBoneOnFrame(em, 114, 17, 11);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TRIPLE_HIT_END):
        Em_PlaySeAtBoneOnFrame(em, 20, 56, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 4, 2, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 4, 5, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 54, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 70, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 36, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 44, 3, BLANG_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_SLAM_WINDUP):
        Em_PlaySeAtBoneOnFrame(em, 2, 92, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 4, 19, BLANG_BONE_HAND_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_STRIKE_A):
        Em_PlaySeAtBoneOnFrame(em, 2, 54, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 46, 53, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 8, 21, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 18, 10, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 88, 6, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 46, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 96, 1, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 34, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 78, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 88, 3, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_STRIKE_B):
        Em_PlaySeAtBoneOnFrame(em, 2, 54, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 46, 53, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 8, 21, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 18, 10, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 88, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 46, 6, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 96, 1, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 34, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 78, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 88, 3, BLANG_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_THROW):
        Em_PlaySeAtBoneOnFrame(em, 55, 57, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 20, 1, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 50, 19, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 62, 21, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 100, 1, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 156, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 174, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 10, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 44, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 96, 6, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DIG_THROW):
        Em_PlaySeAtBoneOnFrame(em, 4, 47, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 88, 46, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 186, 44, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 98, BLANG_SE_THUD, 3);
        Em_PlaySeAtBoneOnFrame(em, 98, 9, 3);
        Em_PlaySeAtBoneOnFrame(em, 60, 18, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 98, 19, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 100, 21, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 140, 5, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 194, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 20, 4, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 24, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 140, 5, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 142, 4, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 186, 3, BLANG_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_CHARGE_END):
        Em_PlaySeAtBoneOnFrame(em, 8, 27, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 6, 20, 3);
        Em_PlaySeAtBoneOnFrame(em, 26, 2, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 30, 1, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 56, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 72, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 84, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 36, 4, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 40, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 80, 3, BLANG_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_ALERT_IDLE):
        if (em->type == EM_KIND_BLANGO) {
            Em_PlaySeAtBoneOnFrame(em, 24, 55, BLANG_BONE_HEAD);
            Em_PlaySeAtBoneOnFrame(em, 72, 56, BLANG_BONE_HEAD);
            Em_PlaySeAtBoneOnFrame(em, 104, 54, BLANG_BONE_HEAD);
            Em_PlaySeAtBoneOnFrame(em, 168, 60, BLANG_BONE_HEAD);
            Em_PlaySeAtBoneOnFrame(em, 82, 0, BLANG_BONE_HAND_A);
            Em_PlaySeAtBoneOnFrame(em, 86, 0, BLANG_BONE_HAND_B);
            Em_PlaySeAtBoneOnFrame(em, 136, 2, BLANG_BONE_HAND_A);
            Em_PlaySeAtBoneOnFrame(em, 142, 2, BLANG_BONE_HAND_B);
            Em_PlaySeAtBoneOnFrame(em, 194, 1, BLANG_BONE_HAND_A);
            Em_PlaySeAtBoneOnFrame(em, 28, 3, BLANG_BONE_FOOT_A);
            Em_PlaySeAtBoneOnFrame(em, 192, 4, BLANG_BONE_FOOT_A);
            break;
        }
        Em_PlaySeAtBoneOnFrame(em, 8, 55, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 72, 56, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 104, 54, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 168, 27, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 82, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 86, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 136, 2, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 142, 2, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 194, 1, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 28, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 192, 4, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_ROAR):
        if (em->type == EM_KIND_BLANGO) {
            Em_PlaySeAtBoneOnFrame(em, 4, 54, BLANG_BONE_HEAD);
            Em_PlaySeAtBoneOnFrame(em, 50, 60, BLANG_BONE_HEAD);
            Em_PlaySeAtBoneOnFrame(em, 56, 3, BLANG_BONE_FOOT_A);
            Em_PlaySeAtBoneOnFrame(em, 58, 2, BLANG_BONE_HAND_A);
            Em_PlaySeAtBoneOnFrame(em, 64, 1, BLANG_BONE_HAND_B);
            break;
        }
        Em_PlaySeAtBoneVolOnFrame(em, 30, 99, BLANG_BONE_HEAD, 7);
        Em_PlaySeAtBoneVolOnFrame(em, 60, 63, BLANG_BONE_HEAD, 7);
        Em_PlaySeAtBoneOnFrame(em, 120, 44, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 8, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 114, 1, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 118, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 154, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 156, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 20, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 142, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 164, 3, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TURN_ABOUT):
        Em_PlaySeAtBoneOnFrame(em, 4, 43, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 106, 53, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 256, 25, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 360, 43, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 138, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 176, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 218, 6, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 240, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 432, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 456, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 102, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 140, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 458, 3, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TURN_ABOUT_B):
        Em_PlaySeAtBoneOnFrame(em, 4, 43, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 106, 53, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 256, 25, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 360, 43, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 138, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 176, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 218, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 240, 6, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 432, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 456, 6, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 102, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 140, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 458, 3, BLANG_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_EAT_NEED2):
        Em_PlaySeAtBoneOnFrame(em, 60, 35, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 16, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 36, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 120, 6, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 26, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 46, 3, BLANG_BONE_FOOT_B);
        Em_PlayCommonSeAtBoneOnFrame(em, 54, 46, BLANG_BONE_HEAD);
        Em_PlayCommonSeAtBoneOnFrame(em, 124, 46, BLANG_BONE_HEAD);
        Em_PlayCommonSeAtBoneOnFrame(em, 162, 46, BLANG_BONE_HEAD);
        Em_PlayCommonSeAtBoneOnFrame(em, 198, 46, BLANG_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_EAT_NEED2_END):
        Em_PlaySeAtBoneOnFrame(em, 50, 44, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 62, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 108, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 74, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 122, 3, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_EAT):
        Em_PlaySeAtBoneOnFrame(em, 4, 43, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 24, 62, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 54, 61, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 94, 61, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 126, 61, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 184, 62, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 290, 61, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 320, 61, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 390, 62, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 18, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 34, 6, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 56, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 364, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 10, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 52, 3, BLANG_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_EAT_END):
        Em_PlaySeAtBoneOnFrame(em, 54, 44, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 60, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 100, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 108, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 124, 3, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_ROAR_B):
        Em_PlaySeAtBoneVolOnFrame(em, 58, 76, BLANG_BONE_HEAD, 7);
        Em_PlaySeAtBoneVolOnFrame(em, 170, 63, BLANG_BONE_HEAD, 7);
        Em_PlaySeAtBoneOnFrame(em, 36, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 148, 1, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 162, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 148, 1, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 162, 3, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_FLINCH_B):
        Em_PlaySeAtBoneOnFrame(em, 4, 80, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 142, 44, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 86, 9, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 94, 2, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 160, 2, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 162, 1, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 22, 4, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 28, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 66, 4, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 76, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 138, 4, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 166, 3, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_FLINCH):
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(72, 4, 71, 4), BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 40, 35, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 22, 1, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 24, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 68, 1, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 78, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 22, 4, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 62, 3, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_SHAKE_OFF):
        Em_PlaySeAtBoneOnFrame(em, 4, 72, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 40, 38, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 80, 35, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 164, 25, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 6, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 24, 1, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 76, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 80, 6, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 178, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 198, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 10, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 164, 3, BLANG_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DOWN):
        Em_PlaySeAtBoneOnFrame(em, 4, 81, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 44, 32, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 68, 17, 6);
        Em_PlaySeAtBoneOnFrame(em, 106, 2, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 106, BLANG_SE_FALL, 3);
        Em_PlaySeAtBoneOnFrame(em, 116, 4, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 36, 18, BLANG_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DOWN_GET_UP):
        Em_PlaySeAtBoneOnFrame(em, 6, 84, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 16, 6, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 42, 2, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 56, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 4, 18, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 44, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 56, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 78, 3, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_STATUS4):
        Em_PlaySeAtBoneOnFrame(em, 4, 72, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 50, 78, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 124, 37, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 26, 1, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 138, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 26, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 180, 3, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_FALL):
        Em_PlaySeAtBoneOnFrame(em, 4, 79, BLANG_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_FALL_LAND):
        Em_PlaySeAtBoneOnFrame(em, 4, 71, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 10, BLANG_SE_THUD, 3);
        Em_PlaySeAtBoneOnFrame(em, 20, BLANG_SE_FALL, 3);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DIE):
        Em_PlaySeAtBoneOnFrame(em, 4, 79, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 80, 33, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 10, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 172, 2, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 176, 17, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 194, 2, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 22, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 58, 4, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 126, 4, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 176, BLANG_SE_THUD, 3);
        Em_PlaySeAtBoneOnFrame(em, 188, BLANG_SE_FALL, 3);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_KNOCK_DOWN):
        Em_PlaySeAtBoneOnFrame(em, 2, 71, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 26, BLANG_SE_FALL, 3);
        Em_PlaySeAtBoneOnFrame(em, 26, 16, 3);
        Em_PlaySeAtBoneOnFrame(em, 48, 18, 6);
        Em_PlaySeAtBoneOnFrame(em, 76, BLANG_SE_FALL, 3);
        Em_PlaySeAtBoneOnFrame(em, 76, 2, BLANG_BONE_HAND_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_KNOCK_DOWN_B):
        Em_PlaySeAtBoneOnFrame(em, 2, 71, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 26, BLANG_SE_FALL, 3);
        Em_PlaySeAtBoneOnFrame(em, 26, 16, 3);
        Em_PlaySeAtBoneOnFrame(em, 48, 18, 11);
        Em_PlaySeAtBoneOnFrame(em, 76, BLANG_SE_FALL, 3);
        Em_PlaySeAtBoneOnFrame(em, 76, 2, BLANG_BONE_HAND_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DIE_END):
        Em_PlaySeAtBoneOnFrame(em, 2, 73, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 12, 17, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 52, 2, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 70, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 136, 1, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 26, 18, BLANG_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TRAPPED):
        Em_PlaySeAtBoneOnFrame(em, 2, 72, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 8, 16, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 12, 18, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 28, 9, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 32, BLANG_SE_FALL, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_STRUGGLE):
        Em_PlaySeAtBoneOnFrame(em, 4, 88, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 44, 49, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 6, 2, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 16, 2, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 56, 2, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 64, 2, BLANG_BONE_HAND_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DOWNED):
        Em_PlaySeAtBoneOnFrame(em, 4, 79, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 96, 34, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 32, 1, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 86, 2, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 102, 2, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 104, BLANG_SE_FALL, BLANG_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, 76):
        Em_PlaySeAtBoneOnFrame(em, 26, 75, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 22, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 40, 6, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 104, 1, BLANG_BONE_HAND_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DOWN_B):
        Em_PlaySeAtBoneOnFrame(em, 4, 81, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 44, 32, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 68, 17, 6);
        Em_PlaySeAtBoneOnFrame(em, 106, 2, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 106, BLANG_SE_FALL, 3);
        Em_PlaySeAtBoneOnFrame(em, 116, 4, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 36, 18, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DOWN_GET_UP_B):
        Em_PlaySeAtBoneOnFrame(em, 6, 84, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 16, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 42, 2, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 56, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 4, 18, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 44, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 56, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 78, 3, BLANG_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_EFFECT_STRIKE):
        Em_PlaySeAtBoneOnFrame(em, 4, 92, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 94, 97, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 100, 24, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 100, 24, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 2, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 90, 2, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 94, 2, BLANG_BONE_HAND_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_EFFECT_STRIKE_END):
        Em_PlaySeAtBoneOnFrame(em, 4, 44, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 58, 1, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 88, 1, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 64, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 92, 3, BLANG_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_BACK_STRIKE):
        Em_PlaySeAtBoneOnFrame(em, 4, 54, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 50, 55, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 36, 2, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 52, 20, 3);
        Em_PlaySeAtBoneOnFrame(em, 34, 4, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 38, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 70, 5, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 74, 4, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 106, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 112, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 74, 2, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 78, 1, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 106, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 112, 6, BLANG_BONE_HAND_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_ROAR_C):
        Em_PlaySeAtBoneOnFrame(em, 2, 25, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 134, 53, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 20, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 198, 6, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 220, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 52, 17, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 88, 17, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 6, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 40, 4, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 204, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 226, 3, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_ROAR_C_END):
        Em_PlaySeAtBoneOnFrame(em, 2, 34, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 108, 35, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 74, 1, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 84, 1, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 12, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 150, 3, BLANG_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_SLAM):
        Em_PlaySeAtBoneOnFrame(em, 2, 97, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 136, 44, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 22, 9, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 34, BLANG_SE_THUD, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 38, 23, 3);
        Em_PlaySeAtBoneOnFrame(em, 124, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 56, 4, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 72, 4, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 118, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 164, 3, BLANG_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_IDLE_ACTION_C):
        Em_PlaySeAtBoneOnFrame(em, 2, 54, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 66, 53, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 140, 57, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 16, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 146, 2, BLANG_BONE_HAND_B);
        
    case EM_MOTION_ID(0, BLANG_MOT_IDLE):
    default:
        break;
    }
}

/* EmBlangonga_MotionSounds for Congalala and Conga. */
void EmBlangonga_MotionSoundsCongalala(Em *em, EmWork *work)
{
    s16 prevMotion;

    prevMotion = work->prevMotion;
    switch (prevMotion) {
    case EM_MOTION_ID(0, BLANG_MOT_IDLE):
        Em_PlaySeAtBoneOnFrame(em, 48, 83, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 160, 63, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 286, 64, BLANG_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_WALK):
        Em_PlaySeAtBoneOnFrame(em, 52, Sound_RandomSe(63, 2, 63, 2), BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 28, 1, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 80, 1, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 20, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 60, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 118, 3, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TURN_LEFT):
        Em_PlaySeAtBoneOnFrame(em, 56, Sound_RandomSe(64, 2, 65, 2), BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 18, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 50, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 82, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 14, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 50, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 80, 3, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TURN_RIGHT):
        Em_PlaySeAtBoneOnFrame(em, 56, Sound_RandomSe(64, 2, 65, 2), BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 18, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 50, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 82, 6, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 14, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 50, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 80, 3, BLANG_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TURN_LEFT_QUICK):
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(60, 1, 61, 2), BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 54, Sound_RandomSe(65, 1, 65, 2), BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 20, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 26, 2, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 14, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 28, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 38, 3, BLANG_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TURN_RIGHT_QUICK):
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(60, 1, 61, 2), BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 54, Sound_RandomSe(65, 1, 65, 2), BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 20, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 26, 2, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 14, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 28, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 38, 3, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_LIMP):
        Em_PlaySeAtBoneOnFrame(em, 10, 79, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 10, 1, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 46, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 68, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 104, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 14, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 18, 18, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 44, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 74, 18, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 76, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 108, 3, BLANG_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_MOVE_ACTION_D):
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(45, 6, 46, 2), BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 12, 20, 3);
        Em_PlaySeAtBoneOnFrame(em, 14, 5, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 40, 2, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 44, 9, 3);
        Em_PlaySeAtBoneOnFrame(em, 88, 1, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 104, 1, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 116, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 46, 5, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 52, 4, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 82, 4, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 102, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 114, 3, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_MOVE_ACTION_C):
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(45, 6, 46, 2), BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 92, Sound_RandomSe(60, 4, 61, 4), BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 12, 20, 3);
        Em_PlaySeAtBoneOnFrame(em, 54, 9, 3);
        Em_PlaySeAtBoneOnFrame(em, 14, 5, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 52, 2, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 74, 1, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 108, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 46, 5, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 62, 4, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 116, 4, BLANG_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_JUMP):
        Em_PlaySeAtBoneOnFrame(em, 4, 35, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneVolOnFrame(em, 54, 88, BLANG_BONE_HEAD, 6);
        Em_PlaySeAtBoneVolOnFrame(em, 78, 15, BLANG_BONE_HEAD, 8);
        Em_PlaySeAtBoneOnFrame(em, 4, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 8, 18, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 8, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 50, 5, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 52, 4, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 52, 20, 3);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_JUMP_FALL):
        Em_PlaySeAtBoneOnFrame(em, 10, 96, BLANG_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_CHARGE):
        Em_PlaySeAtBoneOnFrame(em, 2, 61, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 12, 60, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 30, 61, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 46, 60, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 64, 61, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 22, 2, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 30, 2, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 54, 2, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 64, 2, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 32, 5, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 36, 5, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 64, 5, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 70, 5, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_JUMP_LAND):
        Em_PlaySeAtBoneOnFrame(em, 10, 66, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 40, 46, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 8, BLANG_SE_THUD, 3);
        Em_PlaySeAtBoneOnFrame(em, 52, 9, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 116, 1, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 128, 1, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 100, 3, BLANG_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_LIE):
        Em_PlaySeAtBoneOnFrame(em, 30, 81, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 108, 82, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 170, 81, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 250, 82, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 316, 53, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 486, 63, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 250, 6, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 272, 6, BLANG_BONE_HAND_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_GET_UP):
        Em_PlaySeAtBoneOnFrame(em, 2, 16, 3);
        Em_PlaySeAtBoneOnFrame(em, 34, 61, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 30, 2, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 78, 18, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 140, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 160, 6, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 76, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 110, 3, BLANG_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_COLLAPSE):
        Em_PlaySeAtBoneOnFrame(em, 24, 83, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 190, 41, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 2, 18, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 62, 6, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 66, 18, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 84, BLANG_SE_THUD, 11);
        Em_PlaySeAtBoneOnFrame(em, 118, 16, 11);
        Em_PlaySeAtBoneOnFrame(em, 148, 1, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 164, 4, BLANG_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_LIE_DOWN):
        Em_PlaySeAtBoneOnFrame(em, 60, 83, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 200, 53, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 38, 16, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 4, 6, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 162, 17, 11);
        Em_PlaySeAtBoneOnFrame(em, 180, BLANG_SE_FALL, 3);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_CONGALALA_EATEN_EMISSION):
        Em_PlaySeAtBoneOnFrame(em, 18, 34, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 60, 92, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 100, 88, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 274, 61, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 354, 64, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 20, 1, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 298, 6, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 100, 25, BLANG_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_CONGALALA_COMBO):
        Em_PlaySeAtBoneOnFrame(em, 22, 45, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 66, 46, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 112, 45, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 156, 87, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 238, 60, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 254, 61, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 276, 41, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 170, 18, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 182, 19, 3);
        Em_PlaySeAtBoneOnFrame(em, 200, BLANG_SE_THUD, 3);
        Em_PlaySeAtBoneOnFrame(em, 220, BLANG_SE_FALL, 3);
        Em_PlaySeAtBoneOnFrame(em, 32, 4, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 80, 4, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 130, 4, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 32, 11, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 72, 11, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 118, 11, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 36, 18, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 76, 18, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 122, 18, BLANG_BONE_HAND_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_SLAM_WINDUP):
        Em_PlaySeAtBoneOnFrame(em, 2, 35, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 4, 19, BLANG_BONE_HAND_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_STRIKE_A):
        Em_PlaySeAtBoneOnFrame(em, 2, 33, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 18, 11, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 90, 6, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 92, 1, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 34, 4, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 80, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 92, 3, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_STRIKE_B):
        Em_PlaySeAtBoneOnFrame(em, 2, 33, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 18, 11, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 90, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 92, 1, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 34, 4, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 80, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 92, 3, BLANG_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_CONGALALA_EMISSION):
        if (em->type == EM_KIND_CONGA) {
            Em_PlaySeAtBoneOnFrame(em, 2, 34, BLANG_BONE_HEAD);
            Em_PlaySeAtBoneOnFrame(em, 32, 35, BLANG_BONE_HEAD);
            Em_PlaySeAtBoneOnFrame(em, 90, 36, BLANG_BONE_HEAD);
            Em_PlaySeAtBoneOnFrame(em, 114, 37, BLANG_BONE_HEAD);
            Em_PlaySeAtBoneOnFrame(em, 168, 78, BLANG_BONE_HEAD);
            Em_PlaySeAtBoneOnFrame(em, 266, 63, BLANG_BONE_HEAD);
            Em_PlaySeAtBoneOnFrame(em, 22, 1, BLANG_BONE_HAND_B);
            Em_PlaySeAtBoneOnFrame(em, 28, 0, BLANG_BONE_HAND_A);
            Em_PlaySeAtBoneOnFrame(em, 310, 1, BLANG_BONE_HAND_B);
            Em_PlaySeAtBoneOnFrame(em, 320, 0, BLANG_BONE_HAND_A);
            Em_PlaySeAtBoneOnFrame(em, 22, 3, BLANG_BONE_FOOT_A);
            Em_PlaySeAtBoneOnFrame(em, 320, 3, BLANG_BONE_FOOT_A);
            break;
        }
        Em_PlaySeAtBoneOnFrame(em, 2, 34, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 32, 35, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 90, 36, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 114, 22, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 122, 24, 31);
        Em_PlaySeAtBoneOnFrame(em, 168, 78, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 266, 63, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 22, 1, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 28, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 310, 1, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 320, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 22, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 320, 3, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_THROW):
        Em_PlaySeAtBoneOnFrame(em, 12, 57, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 52, 45, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 100, 65, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 52, 1, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 58, 18, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 138, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 36, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 122, 4, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 138, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 54, 12, 31);
        Em_PlaySeAtBoneOnFrame(em, 48, 21, 31);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_CONGALALA_HOP):
        Em_PlaySeAtBoneOnFrame(em, 4, 57, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 24, 66, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 44, 96, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 28, 19, 3);
        Em_PlaySeAtBoneOnFrame(em, 2, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 12, 1, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 12, 4, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_HOP_LAND):
        Em_PlaySeAtBoneOnFrame(em, 14, 36, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 72, 53, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 192, 64, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 8, 9, 3);
        Em_PlaySeAtBoneOnFrame(em, 58, BLANG_SE_THUD, 3);
        Em_PlaySeAtBoneOnFrame(em, 6, 20, 3);
        Em_PlaySeAtBoneOnFrame(em, 168, 1, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 190, 1, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 82, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 96, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 144, 4, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 118, 4, BLANG_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_ALERT_IDLE):
        Em_PlaySeAtBoneOnFrame(em, 32, 65, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 64, 45, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 100, 46, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 170, 95, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 82, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 86, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 136, 2, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 142, 2, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 170, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 266, 2, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 28, 3, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_ROAR):
        Em_PlaySeAtBoneOnFrame(em, 20, 49, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 68, 90, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 160, 51, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 222, 64, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 190, BLANG_SE_FALL, 3);
        Em_PlaySeAtBoneOnFrame(em, 2, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 8, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 174, 2, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 174, 5, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 56, 4, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 224, 3, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TURN_ABOUT):
        Em_PlaySeAtBoneOnFrame(em, 4, 34, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 68, 64, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 318, 65, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 366, 67, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 138, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 176, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 218, 6, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 240, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 432, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 456, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 102, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 140, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 458, 3, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TURN_ABOUT_B):
        Em_PlaySeAtBoneOnFrame(em, 4, 34, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 68, 64, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 318, 65, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 366, 67, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 138, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 176, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 218, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 240, 6, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 432, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 456, 6, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 102, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 140, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 458, 3, BLANG_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_EAT_NEED2):
        Em_PlaySeAtBoneOnFrame(em, 60, 35, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 16, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 36, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 120, 6, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 26, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 46, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 60, 63, BLANG_BONE_HEAD);
        Em_PlayCommonSeAtBoneOnFrame(em, 120, 46, BLANG_BONE_HEAD);
        Em_PlayCommonSeAtBoneOnFrame(em, 124, 46, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 160, 63, BLANG_BONE_HEAD);
        Em_PlayCommonSeAtBoneOnFrame(em, 162, 46, BLANG_BONE_HEAD);
        Em_PlayCommonSeAtBoneOnFrame(em, 198, 46, BLANG_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_EAT_NEED2_END):
        Em_PlaySeAtBoneOnFrame(em, 50, 67, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 62, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 108, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 74, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 122, 3, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_EAT):
        Em_PlaySeAtBoneOnFrame(em, 4, 67, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 68, 45, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 128, 62, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 162, 62, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 194, 62, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 218, 62, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 248, 62, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 280, 62, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 312, 62, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 340, 23, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 378, 68, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 450, 65, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 62, 18, 28);
        Em_PlaySeAtBoneOnFrame(em, 74, BLANG_SE_THUD, 28);
        Em_PlaySeAtBoneOnFrame(em, 386, 18, 28);
        Em_PlaySeAtBoneOnFrame(em, 4, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 16, 1, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 36, 1, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 98, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 102, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 430, 1, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 464, 1, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 488, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 16, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 44, 4, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 86, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 90, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 378, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 394, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 464, 3, BLANG_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_FLINCH_B):
        Em_PlaySeAtBoneOnFrame(em, 6, 71, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 100, 60, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 124, 61, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 4, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 86, 2, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 96, 1, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 160, 2, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 162, 1, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 24, 4, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 28, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 66, 5, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 76, 4, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 136, 4, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 166, 4, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_FLINCH):
        Em_PlaySeAtBoneOnFrame(em, 4, 70, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 4, 6, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 24, 1, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 70, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 24, 4, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_SHAKE_OFF):
        Em_PlaySeAtBoneOnFrame(em, 4, 85, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 30, 67, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 48, 61, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 62, 60, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 80, 44, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 144, 64, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 166, 63, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 6, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 24, 1, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 76, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 80, 6, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 178, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 198, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 10, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 164, 3, BLANG_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DOWN):
        Em_PlaySeAtBoneOnFrame(em, 44, 79, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 68, 17, 6);
        Em_PlaySeAtBoneOnFrame(em, 106, 2, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 106, BLANG_SE_FALL, 3);
        Em_PlaySeAtBoneOnFrame(em, 116, 4, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 36, 18, BLANG_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DOWN_B):
        Em_PlaySeAtBoneOnFrame(em, 44, 79, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 68, 17, 6);
        Em_PlaySeAtBoneOnFrame(em, 106, 2, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 106, BLANG_SE_FALL, 3);
        Em_PlaySeAtBoneOnFrame(em, 116, 4, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 36, 18, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DOWN_GET_UP):
        Em_PlaySeAtBoneOnFrame(em, 6, 78, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 16, 6, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 42, 2, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 56, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 4, 18, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 44, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 56, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 78, 3, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DOWN_GET_UP_B):
        Em_PlaySeAtBoneOnFrame(em, 6, 78, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 16, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 42, 2, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 56, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 4, 18, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 44, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 56, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 78, 3, BLANG_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_STATUS4):
        Em_PlaySeAtBoneOnFrame(em, 6, 70, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 32, 80, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 26, 1, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 122, 18, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 26, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 184, 3, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_FALL):
        Em_PlaySeAtBoneOnFrame(em, 4, 96, BLANG_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_FALL_LAND):
        Em_PlaySeAtBoneOnFrame(em, 4, 71, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 10, BLANG_SE_THUD, 3);
        Em_PlaySeAtBoneOnFrame(em, 20, BLANG_SE_FALL, 3);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DIE):
        if (em->type == EM_KIND_CONGA) {
            Em_PlaySeAtBoneOnFrame(em, 4, 71, BLANG_BONE_HEAD);
            Em_PlaySeAtBoneOnFrame(em, 96, 51, BLANG_BONE_HEAD);
            Em_PlaySeAtBoneOnFrame(em, 120, 87, BLANG_BONE_HEAD);
            Em_PlaySeAtBoneOnFrame(em, 10, 6, BLANG_BONE_HAND_A);
            Em_PlaySeAtBoneOnFrame(em, 172, 2, BLANG_BONE_HAND_A);
            Em_PlaySeAtBoneOnFrame(em, 176, 17, BLANG_BONE_HAND_A);
            Em_PlaySeAtBoneOnFrame(em, 194, 2, BLANG_BONE_HAND_B);
            Em_PlaySeAtBoneOnFrame(em, 22, 3, BLANG_BONE_FOOT_B);
            Em_PlaySeAtBoneOnFrame(em, 58, 4, BLANG_BONE_FOOT_A);
            Em_PlaySeAtBoneOnFrame(em, 126, 4, BLANG_BONE_FOOT_A);
            Em_PlaySeAtBoneOnFrame(em, 176, BLANG_SE_THUD, 3);
            Em_PlaySeAtBoneOnFrame(em, 188, BLANG_SE_FALL, 3);
            break;
        }
        Em_PlaySeAtBoneOnFrame(em, 4, 96, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 96, 31, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 120, 47, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 230, 61, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 10, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 172, 2, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 176, 17, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 194, 2, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 22, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 58, 4, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 126, 4, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 176, BLANG_SE_THUD, 3);
        Em_PlaySeAtBoneOnFrame(em, 188, BLANG_SE_FALL, 3);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_KNOCK_DOWN):
        Em_PlaySeAtBoneOnFrame(em, 2, 72, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 26, BLANG_SE_FALL, 3);
        Em_PlaySeAtBoneOnFrame(em, 26, 16, 3);
        Em_PlaySeAtBoneOnFrame(em, 48, 18, 6);
        Em_PlaySeAtBoneOnFrame(em, 76, BLANG_SE_FALL, 3);
        Em_PlaySeAtBoneOnFrame(em, 76, 2, BLANG_BONE_HAND_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_KNOCK_DOWN_B):
        Em_PlaySeAtBoneOnFrame(em, 2, 72, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 26, BLANG_SE_FALL, 3);
        Em_PlaySeAtBoneOnFrame(em, 26, 16, 3);
        Em_PlaySeAtBoneOnFrame(em, 48, 18, 11);
        Em_PlaySeAtBoneOnFrame(em, 76, BLANG_SE_FALL, 3);
        Em_PlaySeAtBoneOnFrame(em, 76, 2, BLANG_BONE_HAND_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DIE_END):
        Em_PlaySeAtBoneOnFrame(em, 12, 71, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 106, 65, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 12, 17, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 52, 2, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 70, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 136, 1, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 26, 18, BLANG_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TRAPPED):
        Em_PlaySeAtBoneOnFrame(em, 2, 72, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 4, 16, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 6, 18, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 28, 9, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 32, BLANG_SE_FALL, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 52, 5, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 56, 4, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_STRUGGLE):
        Em_PlaySeAtBoneOnFrame(em, 4, 94, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 12, 4, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 36, 5, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 54, 4, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 76, 5, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 106, 4, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 112, 5, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DOWNED):
        Em_PlaySeAtBoneOnFrame(em, 4, 72, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 104, BLANG_SE_FALL, BLANG_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, 76):
        Em_PlaySeAtBoneOnFrame(em, 34, 18, 3);
        Em_PlaySeAtBoneOnFrame(em, 14, 5, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 26, 4, BLANG_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DROP_CARRIED):
        Em_PlaySeAtBoneOnFrame(em, 4, 72, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 56, 49, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 32, BLANG_SE_THUD, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 16, 18, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 34, 2, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 58, 2, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 150, 1, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 172, 1, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 62, 4, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 68, 4, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 116, 4, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 136, 4, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 160, 4, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_CONGALALA_COMBO_END):
        Em_PlaySeAtBoneOnFrame(em, 2, 16, 3);
        Em_PlaySeAtBoneOnFrame(em, 4, 53, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 80, 67, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 46, 2, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 108, 1, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 38, 4, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 70, 4, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 92, 3, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_CONGALALA_HITZONE_COMBO):
        Em_PlaySeAtBoneOnFrame(em, 4, 45, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 26, 92, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 50, 53, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 92, 53, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 4, 6, BLANG_BONE_HAND_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_CONGALALA_HITZONE_COMBO_END):
        Em_PlaySeAtBoneOnFrame(em, 20, 87, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 94, 2, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 98, 2, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 132, 1, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 94, 2, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 142, 3, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_CONGALALA_CLIMB_OUT):
        Em_PlaySeAtBoneOnFrame(em, 4, 35, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 58, 36, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 90, 45, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 134, 87, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 244, 61, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 60, 18, 3);
        Em_PlaySeAtBoneOnFrame(em, 60, 20, 3);
        Em_PlaySeAtBoneOnFrame(em, 94, BLANG_SE_THUD, 28);
        Em_PlaySeAtBoneOnFrame(em, 212, BLANG_SE_THUD, 28);
        Em_PlaySeAtBoneOnFrame(em, 194, 2, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 196, 1, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 224, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 244, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 266, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 228, 4, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 252, 4, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 268, 3, BLANG_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_CONGALALA_EAT):
        Em_PlaySeAtBoneOnFrame(em, 26, 44, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 84, 62, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 106, 62, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 130, 62, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 162, 62, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 198, 68, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 2, 6, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 14, 1, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 74, 1, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 268, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 274, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 14, 4, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 46, 4, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 228, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 250, 3, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_BACK_STRIKE):
        Em_PlaySeAtBoneOnFrame(em, 24, 44, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 78, 44, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 156, 64, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 192, 65, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 24, 4, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 70, 4, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 152, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 210, 4, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 180, 1, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 186, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 228, 0, BLANG_BONE_HAND_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_ROAR_C):
        Em_PlaySeAtBoneOnFrame(em, 2, 67, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 42, 49, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 138, 90, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 140, 15, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 210, 15, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 2, 6, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 12, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 12, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 28, 3, BLANG_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_ROAR_C_END):
        Em_PlaySeAtBoneOnFrame(em, 2, 34, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 60, 67, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 74, 1, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 84, 1, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 12, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 150, 3, BLANG_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_SLAM):
        Em_PlaySeAtBoneOnFrame(em, 10, 33, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 142, 67, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 28, 9, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 40, BLANG_SE_THUD, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 38, 24, 3);
        Em_PlaySeAtBoneOnFrame(em, 138, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 66, 4, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 78, 4, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 128, 4, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 176, 3, BLANG_BONE_FOOT_B);
        
    default:
        break;
    }
}

/* EmBlangonga_MotionSounds for Rajang. */
void EmBlangonga_MotionSoundsRajang(Em *em, EmWork *work)
{
    s16 prevMotion;
    u8 stateGroup;

    prevMotion = work->prevMotion;
    switch (prevMotion) {
    case EM_MOTION_ID(0, BLANG_MOT_WALK):
        Em_PlaySeAtBoneOnFrame(em, 44, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 106, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 158, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 48, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 94, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 144, 3, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TURN_LEFT):
        Em_PlaySeAtBoneOnFrame(em, 22, Sound_RandomSe(44, 2, 44, 2), BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 18, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 50, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 82, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 14, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 50, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 80, 3, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TURN_RIGHT):
        Em_PlaySeAtBoneOnFrame(em, 22, Sound_RandomSe(44, 2, 44, 2), BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 18, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 50, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 82, 6, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 14, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 50, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 80, 3, BLANG_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TURN_LEFT_QUICK):
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(57, 1, 57, 2), BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 20, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 26, 2, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 14, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 28, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 38, 3, BLANG_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TURN_RIGHT_QUICK):
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(57, 1, 57, 2), BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 20, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 26, 2, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 14, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 28, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 38, 3, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_LIMP):
        Em_PlaySeAtBoneOnFrame(em, 10, 85, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 70, 85, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 10, 1, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 46, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 68, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 104, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 14, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 18, 18, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 44, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 74, 18, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 76, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 108, 3, BLANG_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_MOVE_ACTION_D):
        Em_PlaySeAtBoneOnFrame(em, 4, 29, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 12, 20, 3);
        Em_PlaySeAtBoneOnFrame(em, 28, 11, BLANG_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame(em, 14, 5, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 28, 2, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 32, 1, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 66, 1, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 84, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 96, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 34, 5, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 38, 4, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 62, 4, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 82, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 96, 3, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_MOVE_ACTION_A):
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(29, 1, 30, 1), BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 12, 20, 3);
        Em_PlaySeAtBoneOnFrame(em, 14, 5, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 32, 2, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 64, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 30, 5, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_MOVE_ACTION_B):
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(29, 1, 30, 1), BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 12, 20, 3);
        Em_PlaySeAtBoneOnFrame(em, 14, 5, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 32, 2, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 64, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 30, 5, BLANG_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_MOVE_ACTION_C):
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(29, 1, 30, 1), BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 12, 20, 3);
        Em_PlaySeAtBoneOnFrame(em, 14, 5, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 42, 3, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 46, 1, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 88, 1, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 38, 5, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 42, 4, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 52, 4, BLANG_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_JUMP):
        Em_PlaySeAtBoneVolOnFrame(em, 22, 54, BLANG_BONE_HEAD, 7);
        Em_PlaySeAtBoneVolOnFrame(em, 54, 36, BLANG_BONE_HEAD, 7);
        Em_PlaySeAtBoneOnFrame(em, 4, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 8, 18, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 8, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 50, 5, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 52, 4, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 52, 20, 3);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_JUMP_FALL):
        Em_PlaySeAtBoneOnFrame(em, 34, 20, 3);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_CHARGE):
        Em_PlaySeAtBoneOnFrame(em, 2, 30, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 40, Sound_RandomSe(29, 3, 30, 1), BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 22, 2, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 24, 1, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 56, 2, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 58, 1, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 30, 5, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 32, 4, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 8, 5, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 36, 5, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 10, 21, 3);
        Em_PlaySeAtBoneOnFrame(em, 40, 21, 3);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_JUMP_LAND):
        Em_PlaySeAtBoneOnFrame(em, 12, 55, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 108, 57, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 8, 9, 3);
        Em_PlaySeAtBoneOnFrame(em, 12, BLANG_SE_FALL, 3);
        Em_PlaySeAtBoneOnFrame(em, 6, 2, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 110, 1, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 132, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 108, 3, BLANG_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_EMERGE):
        Em_PlaySeAtBoneOnFrame(em, 20, 20, BLANG_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame(em, 18, BLANG_SE_FALL, BLANG_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame(em, 18, 26, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 88, 44, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 18, 20, BLANG_BONE_BREATH);
        Em_PlaySeAtBoneOnFrame(em, 52, 2, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 60, 1, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 130, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 146, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 56, 5, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 60, 4, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 122, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 56, BLANG_SE_THUD, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_WALK_START):
        Em_PlaySeAtBoneOnFrame(em, 44, 1, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 36, 3, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_LIE):
        Em_PlaySeAtBoneOnFrame(em, 2, 42, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 100, 41, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 162, 42, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 260, 41, BLANG_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_GET_UP):
        Em_PlaySeAtBoneOnFrame(em, 2, 16, 3);
        Em_PlaySeAtBoneOnFrame(em, 2, 43, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 130, 53, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 30, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 78, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 140, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 160, 6, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 76, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 110, 3, BLANG_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_COLLAPSE):
        Em_PlaySeAtBoneOnFrame(em, 40, 51, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 160, 41, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 220, 42, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 2, 6, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 62, 6, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 66, 18, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 84, BLANG_SE_THUD, 11);
        Em_PlaySeAtBoneOnFrame(em, 118, 16, 11);
        Em_PlaySeAtBoneOnFrame(em, 148, 1, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 164, 4, BLANG_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_LIE_DOWN):
        Em_PlaySeAtBoneOnFrame(em, 4, 42, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 60, 51, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 4, 6, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 58, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 162, 16, 11);
        Em_PlaySeAtBoneOnFrame(em, 180, BLANG_SE_FALL, 3);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TRIPLE_HIT):
        Em_PlaySeAtBoneOnFrame(em, 2, 28, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 60, 25, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 28, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 40, 1, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 40, 1, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 66, 5, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 66, 20, 3);
        Em_PlaySeAtBoneOnFrame(em, 88, 12, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 88, 61, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 94, 9, 11);
        Em_PlaySeAtBoneOnFrame(em, 96, BLANG_SE_FALL, 11);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TRIPLE_HIT_END):
        Em_PlaySeAtBoneOnFrame(em, 10, 96, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 4, 2, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 4, 5, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 54, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 70, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 36, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 44, 3, BLANG_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_SLAM_WINDUP):
        Em_PlaySeAtBoneOnFrame(em, 2, 52, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 4, 19, BLANG_BONE_HAND_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_STRIKE_A):
        Em_PlaySeAtBoneOnFrame(em, 2, 28, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 68, 30, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 44, 29, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 20, 15, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 44, 15, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 68, 15, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 48, 11, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 72, 11, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 60, 5, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 82, 5, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_STRIKE_B):
        Em_PlaySeAtBoneOnFrame(em, 10, 27, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 16, 2, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 24, 5, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 28, 2, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 28, 13, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 30, 9, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 30, BLANG_SE_FALL, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 32, 5, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 66, 4, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 68, 6, BLANG_BONE_HAND_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_CHARGE_END):
        Em_PlaySeAtBoneOnFrame(em, 4, 6, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 4, 29, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 20, 55, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 70, 27, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 28, 20, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 28, 18, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 48, 1, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 58, 2, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 60, BLANG_SE_THUD, 3);
        Em_PlaySeAtBoneOnFrame(em, 62, 17, 3);
        Em_PlaySeAtBoneOnFrame(em, 130, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 150, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 66, 11, BLANG_BONE_BREATH);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_ALERT_IDLE):
        Em_PlaySeAtBoneOnFrame(em, 24, 95, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 72, 56, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 104, 54, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 168, 27, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 82, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 86, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 136, 2, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 142, 2, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 194, 1, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 28, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 192, 4, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_ROAR):
        Em_PlaySeAtBoneVolOnFrame(em, 30, 99, BLANG_BONE_HEAD, 7);
        Em_PlaySeAtBoneOnFrame(em, 120, 44, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 8, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 114, 1, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 118, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 154, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 156, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 20, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 142, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 164, 3, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TURN_ABOUT):
        Em_PlaySeAtBoneOnFrame(em, 4, 83, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 106, 53, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 256, 44, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 360, 43, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 138, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 176, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 218, 6, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 240, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 432, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 456, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 102, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 140, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 458, 3, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TURN_ABOUT_B):
        Em_PlaySeAtBoneOnFrame(em, 4, 83, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 106, 53, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 256, 44, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 360, 43, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 138, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 176, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 218, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 240, 6, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 432, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 456, 6, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 102, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 140, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 458, 3, BLANG_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_EAT_NEED2):
        Em_PlaySeAtBoneOnFrame(em, 60, 75, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 16, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 36, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 120, 6, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 26, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 46, 3, BLANG_BONE_FOOT_B);
        Em_PlayCommonSeAtBoneOnFrame(em, 54, 46, BLANG_BONE_HEAD);
        Em_PlayCommonSeAtBoneOnFrame(em, 124, 46, BLANG_BONE_HEAD);
        Em_PlayCommonSeAtBoneOnFrame(em, 162, 46, BLANG_BONE_HEAD);
        Em_PlayCommonSeAtBoneOnFrame(em, 198, 46, BLANG_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_EAT_NEED2_END):
        Em_PlaySeAtBoneOnFrame(em, 50, 44, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 62, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 108, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 74, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 122, 3, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_EAT):
        Em_PlaySeAtBoneOnFrame(em, 4, 83, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 24, 62, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 54, 61, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 94, 61, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 126, 61, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 184, 62, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 290, 61, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 320, 61, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 390, 62, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 18, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 34, 6, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 56, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 364, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 10, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 52, 3, BLANG_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_EAT_END):
        Em_PlaySeAtBoneOnFrame(em, 54, 84, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 60, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 100, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 108, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 124, 3, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_ROAR_B):
        Em_PlaySeAtBoneVolOnFrame(em, 58, 76, BLANG_BONE_HEAD, 7);
        Em_PlaySeAtBoneOnFrame(em, 36, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 148, 1, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 162, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 148, 1, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 162, 3, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_FLINCH_B):
        Em_PlaySeAtBoneOnFrame(em, 4, 80, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 142, 44, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 86, 9, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 94, 2, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 160, 2, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 162, 1, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 22, 4, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 28, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 66, 4, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 76, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 138, 4, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 166, 3, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_FLINCH):
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(72, 4, 71, 4), BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 40, 35, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 22, 1, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 24, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 68, 1, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 78, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 22, 4, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 62, 3, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_SHAKE_OFF):
        Em_PlaySeAtBoneOnFrame(em, 4, 72, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 40, 38, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 80, 35, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 164, 44, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 6, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 24, 1, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 76, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 80, 6, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 178, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 198, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 10, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 164, 3, BLANG_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DOWN):
        Em_PlaySeAtBoneOnFrame(em, 4, 81, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 44, 32, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 68, 17, 6);
        Em_PlaySeAtBoneOnFrame(em, 106, 2, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 106, BLANG_SE_FALL, 3);
        Em_PlaySeAtBoneOnFrame(em, 116, 4, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 36, 18, BLANG_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DOWN_GET_UP):
        Em_PlaySeAtBoneOnFrame(em, 6, 97, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 16, 6, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 42, 2, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 56, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 4, 18, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 44, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 56, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 78, 3, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_STATUS4):
        Em_PlaySeAtBoneOnFrame(em, 4, 72, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 50, 38, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 124, 37, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 26, 1, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 138, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 26, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 180, 3, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_FALL):
        Em_PlaySeAtBoneOnFrame(em, 4, 79, BLANG_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_FALL_LAND):
        Em_PlaySeAtBoneOnFrame(em, 4, 71, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 10, BLANG_SE_THUD, 3);
        Em_PlaySeAtBoneOnFrame(em, 20, BLANG_SE_FALL, 3);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DIE):
        Em_PlaySeAtBoneOnFrame(em, 4, 79, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 80, 33, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 10, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 172, 2, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 176, 17, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 194, 2, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 22, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 58, 4, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 126, 4, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 176, BLANG_SE_THUD, 3);
        Em_PlaySeAtBoneOnFrame(em, 188, BLANG_SE_FALL, 3);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_KNOCK_DOWN):
        Em_PlaySeAtBoneOnFrame(em, 2, 71, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 26, BLANG_SE_FALL, 3);
        Em_PlaySeAtBoneOnFrame(em, 26, 16, 3);
        Em_PlaySeAtBoneOnFrame(em, 48, 18, 6);
        Em_PlaySeAtBoneOnFrame(em, 76, BLANG_SE_FALL, 3);
        Em_PlaySeAtBoneOnFrame(em, 76, 2, BLANG_BONE_HAND_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_KNOCK_DOWN_B):
        Em_PlaySeAtBoneOnFrame(em, 2, 71, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 26, BLANG_SE_FALL, 3);
        Em_PlaySeAtBoneOnFrame(em, 26, 16, 3);
        Em_PlaySeAtBoneOnFrame(em, 48, 18, 11);
        Em_PlaySeAtBoneOnFrame(em, 76, BLANG_SE_FALL, 3);
        Em_PlaySeAtBoneOnFrame(em, 76, 2, BLANG_BONE_HAND_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DIE_END):
        Em_PlaySeAtBoneOnFrame(em, 2, 73, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 12, 17, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 52, 2, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 70, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 136, 1, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 26, 18, BLANG_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TRAPPED):
        Em_PlaySeAtBoneOnFrame(em, 2, 72, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 8, 16, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 12, 18, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 28, 9, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 32, BLANG_SE_FALL, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_STRUGGLE):
        Em_PlaySeAtBoneOnFrame(em, 4, 88, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 44, 89, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 6, 2, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 16, 2, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 56, 2, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 64, 2, BLANG_BONE_HAND_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DOWNED):
        Em_PlaySeAtBoneOnFrame(em, 4, 79, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 96, 34, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 32, 1, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 86, 2, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 102, 2, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 104, BLANG_SE_FALL, BLANG_BONE_HEAD);
        break;
    case EM_MOTION_ID(0, 76):
        Em_PlaySeAtBoneOnFrame(em, 26, 75, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 22, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 40, 6, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 104, 1, BLANG_BONE_HAND_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DOWN_B):
        Em_PlaySeAtBoneOnFrame(em, 4, 81, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 44, 32, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 68, 17, 6);
        Em_PlaySeAtBoneOnFrame(em, 106, 2, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 106, BLANG_SE_FALL, 3);
        Em_PlaySeAtBoneOnFrame(em, 116, 4, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 36, 18, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DOWN_GET_UP_B):
        Em_PlaySeAtBoneOnFrame(em, 6, 97, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 16, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 42, 2, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 56, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 4, 18, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 44, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 56, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 78, 3, BLANG_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_RAJANG_AIM):
        stateGroup = em->stateGroup;
        if ((stateGroup == 3) && (em->state == 19)) {
            Em_PlaySeAtBoneOnFrame(em, 102, 22, BLANG_BONE_HEAD);
            Em_PlaySeAtBoneOnFrame(em, 90, 23, BLANG_BONE_HEAD);
            Em_PlaySeAtBoneOnFrame(em, 102, 24, BLANG_BONE_HEAD);
            Em_PlaySeAtBoneOnFrame(em, 28, 86, BLANG_BONE_HEAD);
            Em_PlaySeAtBoneOnFrame(em, 72, 76, BLANG_BONE_HEAD);
            Em_PlaySeAtBoneOnFrame(em, 4, 6, BLANG_BONE_HAND_A);
            Em_PlaySeAtBoneOnFrame(em, 78, 2, BLANG_BONE_HAND_B);
            Em_PlaySeAtBoneOnFrame(em, 82, 1, BLANG_BONE_HAND_A);
            break;
        }
        if ((stateGroup == 3) && (em->state == 24)) {
            Em_PlaySeAtBoneOnFrame(em, 102, 22, BLANG_BONE_HEAD);
            Em_PlaySeAtBoneOnFrame(em, 90, 24, BLANG_BONE_HEAD);
            Em_PlaySeAtBoneOnFrame(em, 28, 86, BLANG_BONE_HEAD);
            Em_PlaySeAtBoneOnFrame(em, 72, 25, BLANG_BONE_HEAD);
            Em_PlaySeAtBoneOnFrame(em, 4, 6, BLANG_BONE_HAND_A);
            Em_PlaySeAtBoneOnFrame(em, 78, 2, BLANG_BONE_HAND_B);
            Em_PlaySeAtBoneOnFrame(em, 82, 1, BLANG_BONE_HAND_A);
            break;
        }
        Em_PlaySeAtBoneOnFrame(em, 80, 22, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 34, 95, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 4, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 78, 2, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 82, 1, BLANG_BONE_HAND_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_RAJANG_AIM_END):
        Em_PlaySeAtBoneOnFrame(em, 52, 34, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 102, 2, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 132, 2, BLANG_BONE_HAND_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_RAJANG_JUMP):
        Em_PlaySeAtBoneOnFrame(em, 22, 26, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 4, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 8, 18, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 8, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 24, 5, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 24, 4, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 26, 20, 3);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_RAJANG_JUMP_SHOT):
        Em_PlaySeAtBoneOnFrame(em, 18, 22, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 50, 19, 3);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_RAJANG_JUMP_SHOT_LAND):
        Em_PlaySeAtBoneOnFrame(em, 4, 55, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 50, 57, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 8, BLANG_SE_THUD, 3);
        Em_PlaySeAtBoneOnFrame(em, 6, 2, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 72, 2, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 78, 1, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 74, 5, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 78, 4, BLANG_BONE_FOOT_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_BACK_STRIKE):
        Em_PlaySeAtBoneOnFrame(em, 4, 54, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 50, 55, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 36, 2, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 52, 20, 3);
        Em_PlaySeAtBoneOnFrame(em, 34, 4, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 38, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 70, 5, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 74, 4, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 106, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 112, 3, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 74, 2, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 78, 1, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 106, 0, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 112, 6, BLANG_BONE_HAND_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_RAJANG_STRIKE):
        Em_PlaySeAtBoneOnFrame(em, 4, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 8, 55, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 116, 44, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 160, 56, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 62, 19, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 50, 18, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 72, 19, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 80, 0, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 96, 1, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 96, 18, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 154, 2, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 162, 1, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 172, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 58, 12, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 70, 11, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 74, 20, BLANG_BONE_HAND_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_RAJANG_CHARGE):
        Em_PlaySeAtBoneOnFrame(em, 12, 29, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 16, 5, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 18, 5, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 18, 20, 3);
        Em_PlaySeAtBoneOnFrame(em, 28, 2, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 32, 1, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 38, 4, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 40, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 50, 30, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 52, 5, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 50, 5, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 52, 20, 3);
        Em_PlaySeAtBoneOnFrame(em, 64, 2, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 66, 1, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 76, 4, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 78, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 86, 29, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 84, 5, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 86, 5, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 84, 20, 3);
        Em_PlaySeAtBoneOnFrame(em, 100, 2, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 102, 1, BLANG_BONE_HAND_A);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_SLAM):
        Em_PlaySeAtBoneOnFrame(em, 2, 25, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 136, 44, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 22, 9, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 34, BLANG_SE_FALL, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 34, 9, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 124, 6, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 56, 4, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 72, 4, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 118, 3, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 164, 3, BLANG_BONE_FOOT_B);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_RAJANG_CHARGE_END):
        Em_PlaySeAtBoneOnFrame(em, 10, 29, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 14, 5, BLANG_BONE_FOOT_B);
        Em_PlaySeAtBoneOnFrame(em, 16, 5, BLANG_BONE_FOOT_A);
        Em_PlaySeAtBoneOnFrame(em, 16, 20, 3);
        Em_PlaySeAtBoneOnFrame(em, 70, 55, BLANG_BONE_HEAD);
        Em_PlaySeAtBoneOnFrame(em, 30, 2, BLANG_BONE_HAND_B);
        Em_PlaySeAtBoneOnFrame(em, 34, 1, BLANG_BONE_HAND_A);
        Em_PlaySeAtBoneOnFrame(em, 36, BLANG_SE_THUD, 3);
        
    case EM_MOTION_ID(0, BLANG_MOT_IDLE):
    default:
        break;
    }
}

/*
 * The family's setup (Em_TypeSetup's entry for the Blangonga kinds):
 * nothing to do.
 */
void EmBlangonga_Setup(Em *em)
{
}

/*
 * Callback slots 0 and 1: creates the family's effect 01 (type 3 for the
 * large kinds, 0 for the Blango and the Conga). The monster comes as an
 * s32.
 */
void EmBlangonga_SpawnShadow(s32 em)
{
    if (EmBlangonga_IsSmallKind((Em *) em) == 0) {
        Shadow_Spawn((Player *) em, SHADOW_GROUND_BONES);
        return;
    }
    Shadow_Spawn((Player *) em, SHADOW_MODEL);
}

/*
 * Sets a monster of the family up: the scaled parts (0-2 for the Blangonga
 * kinds, 2-3 for the Congalala kinds); in the quest-less stage
 * (gQuest.stage 0) a fixed start position per slot; the idle motion and
 * full hp from the size (the Blango and the Conga are always run locally);
 * the rage timer and work cleared; every carried item hidden, then the
 * Congalala kinds show the one EmBlangonga_PickCarried picks; eatenKind
 * from the versus team flags; the kind's body colour; the parts.
 */
void EmBlangonga_Init(Em *em)
{
    EmBlangongaWork *w = (EmBlangongaWork *)&em->work;
    u16 part;

    if (em->type == EM_KIND_BLANGONGA) {
        Em_SetPartScaled(em, 0);
        Em_SetPartScaled(em, 1);
        Em_SetPartScaled(em, 2);
    } else if (em->type == EM_KIND_CONGALALA) {
        Em_SetPartScaled(em, 2);
        Em_SetPartScaled(em, 3);
    }
    if (gQuest.stage == 0) {
        em->posX = 2400.0f + 150.0f * (u32) em->slotNo;
        em->posY = 0.0f;
        em->posZ = 3600.0f;
    }
    Em_SetMotionRow(em, BLANG_MOT_IDLE, 0, 0);
    em->stance = EM_STANCE_GROUND;
    em->hpMax = em->hp = Em_ComputeMaxHp(em, 0);
    em->scriptPending = 1;
    em->blind = 0;
    em->unk_InitFlag = 1;
    if (EmBlangonga_IsSmallKind(em) == 1) {
        em->remote = 0;
    }
    w->rageTimer = 0;
    w->unk03A = 0;
    w->senseListed = 0;
    Em_BonePos(em, 15, &w->bonePos[0]);
    w->moveCount = 0;
    w->carryHidden[0] = 1;
    w->carryHidden[1] = 1;
    w->carryHidden[2] = 1;
    if (em->type == EM_KIND_CONGALALA) {
        part = EmBlangonga_PickCarried((EmBlangongaPickRow *) em, w->pickSeed);
        if (part != 0xFFFF) {
            EmBlangonga_ShowCarried(em, part);
        }
    }
    if (em->type == EM_KIND_CONGALALA) {
        if (Quest_IsVersus() == 1) {
            if (Versus_TeamHasFlag(em, 3)) {
                w->eatenKind = 2;
            } else if (Versus_TeamHasFlag(em, 4)) {
                w->eatenKind = 1;
            } else if (Versus_TeamHasFlag(em, 5)) {
                w->eatenKind = 3;
            } else {
                w->eatenKind = 0;
            }
        } else {
            w->eatenKind = 0;
        }
    } else {
        w->eatenKind = 0;
    }
    switch (em->type) {
    case EM_KIND_BLANGONGA:
        w->tint[0] = 115;
        w->tint[1] = 235;
        w->tint[2] = 255;
        break;
    case EM_KIND_BLANGO:
        w->tint[0] = 205;
        w->tint[1] = 145;
        w->tint[2] = 195;
        break;
    case EM_KIND_CONGALALA:
    case EM_KIND_CONGA:
        w->tint[0] = 153;
        w->tint[1] = 128;
        w->tint[2] = 115;
        break;
    default:
        w->tint[0] = 255;
        w->tint[1] = 255;
        w->tint[2] = 255;
        break;
    }
    w->hitSeCooldown = 0;
    Em_InitParts(em);
}

/*
 * Starts a monster: the Congalala kinds first draw pickSeed so that Init
 * and Reinit pick the same carried item; Init; in Congalala's two story
 * quests, before EVENT_FLAG_CONGALALA_SCENE, places it at a fixed point of AREA_JUNGLE_3;
 * then BLANG_IDLE_BURIED for a Blango of variant 1, BLANG_IDLE for the
 * others.
 */
void EmBlangonga_Start(Em *em)
{
    EmBlangongaWork *w = (EmBlangongaWork *)&em->work;

    if (em->type == EM_KIND_CONGALALA) {
        w->pickSeed = Quest_Random();
    }
    EmBlangonga_Init(em);
    if (em->type == EM_KIND_CONGALALA) {
        switch (gQuest.stage) {
        case QUEST_ID_CONGALALA_SCENE_1:
        case QUEST_ID_CONGALALA_SCENE_2:
            if (Save_TestEventFlag(EVENT_FLAG_CONGALALA_SCENE) == 0) {
                em->area = Area_IdForCurrentTime(AREA_JUNGLE_3);
                em->posX = 11870.0f;
                em->posY = 0.0f;
                em->posZ = 10100.0f;
                em->rotY = ANGLE_TURN * 9 / 16;
            }
            break;
        }
    }
    if (em->type == EM_KIND_BLANGO && em->variant == 1) {
        EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_BURIED, EM_SYNC_LOCAL);
        return;
    }
    EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE, EM_SYNC_LOCAL);
}

/*
 * Callback slot 5: sets the monster up again (Init) without changing its
 * state.
 */
void EmBlangonga_Reinit(Em *em)
{
    EmBlangonga_Init(em);
}

/*
 * Starts state `state` of group `group` (EM_SYNC_* `mode`, passed on to
 * Em_SetStateSync): takes the roar's sensed object off the list, does what
 * the state needs on entry (statuses begun or ended, speeds and stop
 * distances, the move count, the low-hp limp variants below 30% (Blangonga
 * kinds), 20% (Congalala kinds) or 15% (Rajang) of hpMax, the carried items
 * shown again, the part break of BLANG_REACT_PART_BREAK), then sets it. A
 * remote copy ignores the owner's modes.
 */
void EmBlangonga_ChangeState(Em *em, u16 group, u16 state, u8 mode)
{
    EmBlangongaWork *w = (EmBlangongaWork *)&em->work;
    u8 count = 0;
    f32 lowHpRatio;
    u16 area;

    if (w->senseListed != 0) {
        EmSenseObjs_Remove((EmObj *)((&w->sense)));
        w->senseListed = 0;
    }
    if (em->remote == 0) {
        Em_TargetPlayerInRange(em);
    } else if (mode == EM_SYNC_OWNER || mode == EM_SYNC_OWNER_FLAGGED) {
        return;
    }
    if (EmBlangonga_IsBlangonga(em) == 1) {
        lowHpRatio = 0.3f;
    } else if (EmBlangonga_IsCongalala(em) == 1) {
        lowHpRatio = 0.2f;
    } else {
        lowHpRatio = 0.15f;
    }
    em->hitzoneSet = 0;
    switch (group) {
    case EM_GROUP_IDLE:
        switch (state) {
        case BLANG_IDLE:
            if (mode == EM_SYNC_LOCAL) {
                count = w->moveCount;
            }
            if (em->combat != 0) {
                state = BLANG_IDLE_ALERT;
            }
            break;
        case BLANG_IDLE_REST:
            Em_ResetNotice(em);
            break;
        case BLANG_IDLE_STATUS2_END:
            Em_EndStatus2(em);
            break;
        case BLANG_IDLE_STATUS2_COLLAPSE:
        case BLANG_IDLE_STATUS2_DOWNED:
            Em_BeginStatus2(em);
            break;
        case BLANG_IDLE_STATUS4_SHAKE_OFF:
            Em_EndStatus4(em);
            break;
        case BLANG_IDLE_BURIED:
            em->senseDelay = 5;
            em->stateArmor = 1;
            em->scriptPending = 0;
            break;
        case BLANG_IDLE_STUN_SHAKE_OFF:
            Em_EndStatus5(em);
            break;
        case BLANG_IDLE_REST_END:
            Em_RefillNeed3(em);
            break;
        case BLANG_IDLE_HOLD2_SHAKE_OFF:
            em->holdTimer2 = 0;
            break;
        case BLANG_IDLE_EAT_KIND_A:
        case BLANG_IDLE_EAT_KIND_B:
        case BLANG_IDLE_EAT_KIND_C:
        case BLANG_IDLE_EAT_KIND_D:
        case BLANG_IDLE_EAT_NEED1_B:
            switch (state) {
            case BLANG_IDLE_EAT_KIND_A:
                w->eatenKind = 1;
                break;
            case BLANG_IDLE_EAT_KIND_B:
                w->eatenKind = 2;
                break;
            case BLANG_IDLE_EAT_KIND_C:
                w->eatenKind = 3;
                break;
            case BLANG_IDLE_EAT_KIND_D:
                w->eatenKind = 4;
                break;
            }
            if (em->type == EM_KIND_CONGALALA) {
                EmBlangonga_HideCarried(em, 0);
                EmBlangonga_HideCarried(em, 1);
                EmBlangonga_HideCarried(em, 2);
            }
            break;
        case BLANG_IDLE_EAT:
            Em_StartGoalAction(em, 10000);
            break;
        case BLANG_IDLE_STATUS1_DOWNED:
        case BLANG_IDLE_STATUS1_COLLAPSE:
            Em_BeginStatus1(em);
            break;
        case BLANG_IDLE_STATUS1_END:
            Em_EndStatus1(em);
            break;
        case BLANG_IDLE_ROAR_RAGE:
            Em_StartRage(em);
            break;
        case BLANG_IDLE_ROAR_PENDING_RAGE:
            Em_StartPendingRage(em);
            break;
        }
        break;
    case EM_GROUP_MOVE:
        switch (state) {
        case BLANG_MOVE_WALK:
        case BLANG_MOVE_START_AND_WALK:
            if (em->hp < (s16) (em->hpMax * lowHpRatio)) {
                state = BLANG_MOVE_LIMP;
            }
        case BLANG_MOVE_LIMP:
        walk: /* BLANG_MOVE_ACTION_E at low hp joins the limp here */
            if (em->curGoalKind == 7) {
                Em_SetSpeedScaled(em, 0, 1000.0f, 350.0f);
            } else {
                Em_SetSpeedScaled(em, 0, 1000.0f, 100.0f);
            }
            break;
        case BLANG_MOVE_ACTION_A:
        case BLANG_MOVE_ACTION_B:
        case BLANG_MOVE_ACTION_C:
        case BLANG_MOVE_ACTION_D:
            if (Em_IsSyncFree(em, mode) != 0) {
                w->moveCount++;
            }
            count = w->moveCount;
            break;
        case BLANG_MOVE_TURN_TO_EXIT:
            if (em->remote == 0 && em->curGoalKind != 3) {
                EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE, EM_SYNC_SEND);
                return;
            }
            break;
        case BLANG_MOVE_START_AND_APPROACH:
        case BLANG_MOVE_APPROACH:
            if (em->hp < (s16) (em->hpMax * lowHpRatio)) {
                state = BLANG_MOVE_LIMP_APPROACH;
            }
        case BLANG_MOVE_LIMP_APPROACH:
            if (em->curGoalKind == 0) {
                em->hasGoal = 0;
            } else {
                em->hasGoal = 1;
            }
            break;
        case BLANG_MOVE_ACTION_E:
            if (em->hp < (s16) (em->hpMax * lowHpRatio)) {
                state = BLANG_MOVE_LIMP;
                goto walk;
            }
            break;
        case BLANG_MOVE_TURN:
        case BLANG_MOVE_TURN_QUICK:
        case BLANG_MOVE_TURN_AWAY:
        case BLANG_MOVE_TURN_AWAY_QUICK:
            break;
        }
        break;
    case EM_GROUP_FLY:
        switch (state) {
        case 2:
            if ((EmBlangonga_ScriptQuery(em, BLANG_QUERY_ON_SNOW) & 0xFF) == 0) {
                EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_ALERT_BRIEF, EM_SYNC_SEND);
                return;
            }
            break;
        case 6:
            Em_SetSpeedScaled(em, 0, 1000.0f, 250.0f);
            break;
        case 9:
            Em_TypeSetup(em);
            break;
        case 11:
            Em_TypeSetup(em);
            if (em->remote == 0 && em->curGoalKind != 3) {
                switch (Map_DayId(gScene.map)) {
                case MAP_SNOWY_MOUNTAINS:
                default:
                    area = Area_DayId(AREA_SNOWY_MOUNTAINS_8);
                    em->destArea = area;
                    em->nextArea = area;
                    em->goalNo = area;
                    break;
                case MAP_JUNGLE:
                    area = Area_DayId(AREA_JUNGLE_3);
                    em->destArea = area;
                    em->nextArea = area;
                    em->goalNo = area;
                    break;
                case MAP_SWAMP:
                    area = Area_DayId(AREA_SWAMP_9);
                    em->destArea = area;
                    em->nextArea = area;
                    em->goalNo = area;
                    break;
                case MAP_VOLCANO:
                    area = Area_DayId(AREA_VOLCANO_5);
                    em->destArea = area;
                    em->nextArea = area;
                    em->goalNo = area;
                    break;
                }
                em->goalKind = 3;
                Em_ComputeGoal(em, em->goal);
                EmBlangonga_ChangeState(em, EM_GROUP_MOVE, BLANG_MOVE_TURN_TO_EXIT, EM_SYNC_SEND);
                return;
            }
            break;
        }
        break;
    case EM_GROUP_ATTACK:
        switch (state) {
        case BLANG_ATK_CHARGE:
            Em_SetSpeedScaled(em, 0, 1000.0f, 1100.0f);
            break;
        case BLANG_ATK_RAJANG_CHARGE:
            Em_SetSpeedScaled(em, 0, 1000.0f, 500.0f);
            break;
        case BLANG_ATK_CONGALALA_CHARGE:
            if (em->curGoalKind != 1) {
                Em_SetSpeedScaled(em, 0, 1000.0f, 350.0f);
            } else {
                Em_SetSpeedScaled(em, 0, 1000.0f, -200.0f);
            }
            break;
        }
        break;
    case EM_GROUP_REACT:
        switch (state) {
        case BLANG_REACT_STATUS4:
        case BLANG_REACT_STATUS4_DOWNED:
            Em_BeginStatus4(em);
            break;
        case BLANG_REACT_TRAPPED:
            Em_EndAllStatus(em);
            break;
        case BLANG_REACT_STATUS4_DOWNED_END:
            Em_EndStatus4(em);
            break;
        case BLANG_REACT_STATUS2_DOWNED_END:
            Em_EndStatus2(em);
            break;
        case BLANG_REACT_STATUS1_DOWNED_END:
            Em_EndStatus1(em);
            break;
        case BLANG_REACT_STUN:
        case BLANG_REACT_STUN_B:
        case BLANG_REACT_STUN_DOWNED:
            Em_BeginStatus5(em);
            break;
        case BLANG_REACT_STUN_DOWNED_END:
            Em_EndStatus5(em);
            break;
        case BLANG_REACT_HOLD2:
            Em_EndAllStatus(em);
            break;
        case BLANG_REACT_DROP_CARRIED:
        case BLANG_REACT_DROP_CARRIED_A:
        case BLANG_REACT_DROP_CARRIED_B:
        case BLANG_REACT_DROP_CARRIED_C:
            if (em->type == EM_KIND_CONGALALA) {
                EmBlangonga_HideCarried(em, 0);
                EmBlangonga_HideCarried(em, 1);
                EmBlangonga_HideCarried(em, 2);
            }
            break;
        case BLANG_REACT_PART_BREAK:
            em->partBreakCount++;
            if (em->type == EM_KIND_BLANGONGA) {
                Em_BreakPart(em, 1);
            } else if (em->type == EM_KIND_RAJANG) {
                Em_EndRage(em);
                Em_BreakPart(em, 2);
            }
            break;
        case BLANG_REACT_TRAP_ESCAPE:
            Em_EndAllStatus(em);
            em->guardTimer = 2;
            break;
        case BLANG_REACT_TRAPPED_STRUGGLE:
        case BLANG_REACT_FALL:
        case BLANG_REACT_NONE:
        case BLANG_REACT_FLINCH_ATTACK:
            break;
        }
        break;
    case EM_GROUP_DEFEAT:
        switch (state) {
        case BLANG_DEFEAT_DIE:
            if (EmBlangonga_IsSmallKind(em) == 1 && em->stance == EM_STANCE_AIR) {
                EmBlangonga_ChangeState(em, EM_GROUP_DEFEAT, BLANG_DEFEAT_DIE_FALLING, mode);
                return;
            }
            break;
        case BLANG_DEFEAT_REAPPEAR:
            if (QuestEm_CanAppear(em) == 1) {
                QuestEm_Appear(em);
            }
            break;
        }
        break;
    case EM_GROUP_VERSUS:
        if (state == 152) {
            Versus_MarkDefeat(em);
        }
        em->motionSpeed = 1.0f;
        break;
    case EM_GROUP_EVENT:
    case EM_GROUP_EVENT2:
        break;
    }
    w->moveCount = count;
    Em_SetStateSync(em, group, state, mode);
}

/*
 * Ends a state: back to BLANG_IDLE with the motion speed, stance and look
 * reset, the script asked for the next state (scriptPending). Both
 * arguments are unused.
 */
static void EmBlangonga_EndState(Em *em, s32 unused1, s32 unused2)
{
    em->motionSpeed = 1.0f;
    em->stance = EM_STANCE_GROUND;
    em->lookAtTarget = 0;
    em->scriptPending = 1;
    EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE, EM_SYNC_LOCAL);
}

/*
 * Ends a buried state: as EmBlangonga_EndState, but stays under the snow
 * (stateArmor) and goes back to BLANG_LEAP_BURIED.
 */
static void EmBlangonga_EndBuriedState(Em *em)
{
    em->motionSpeed = 1.0f;
    em->stance = EM_STANCE_GROUND;
    em->stateArmor = 1;
    em->noPushTimer = 2;
    em->lookAtTarget = 0;
    em->scriptPending = 1;
    EmBlangonga_ChangeState(em, EM_GROUP_FLY, BLANG_LEAP_BURIED, EM_SYNC_LOCAL);
}

/*
 * Picks the carried item for the current map (MAP_JUNGLE or MAP_SWAMP: the two
 * weight tables of EmBlangonga_TurnData) from `seed`: the value of the row
 * whose running weight passes seed modulo the total; 0xFFFF on other maps.
 * The first argument is unused (its register is the loop pointer: a local
 * of its own gives other registers).
 */
u16 EmBlangonga_PickCarried(EmBlangongaPickRow *row, u16 seed)
{
    u16 sum;
    u16 acc;
    EmBlangongaPickRow *table;
    u16 roll;

    switch (Map_DayId(gScene.map)) {
    case MAP_JUNGLE:
        table = EmBlangonga_TurnData.pick0;
        break;
    case MAP_SWAMP:
        table = EmBlangonga_TurnData.pick1;
        break;
    default:
        return 65535;
    }
    sum = 0;
    row = table;
    if (table->weight != 0xFFFF) {
        do {
            acc = row->weight;
            sum += acc;
            row++;
        } while (row->weight != 0xFFFF);
    }
    acc = 0;
    roll = seed % sum;
    if (table->weight != 0xFFFF) {
        do {
            acc += *(u16 *) table; /* the weight; read as a u16 the match needs (table->weight compiles differently) */
            if (acc > roll) {
                return table->value;
            }
            table++;
        } while (table->weight != 0xFFFF);
    }
    return 65535;
}

/* Hides carried item `part` (0-2): sets its carryHidden flag. */
void EmBlangonga_HideCarried(Em *em, u16 part)
{
    EmBlangongaWork *w = (EmBlangongaWork *)&em->work;

    switch (part) {
    case 0:
        w->carryHidden[0] = 1;
        break;
    case 1:
        w->carryHidden[1] = 1;
        break;
    case 2:
        w->carryHidden[2] = 1;
        break;
    }
}

/* Shows carried item `part` (0-2): clears its carryHidden flag. */
void EmBlangonga_ShowCarried(Em *em, u16 part)
{
    EmBlangongaWork *w = (EmBlangongaWork *)&em->work;

    switch (part) {
    case 0:
        w->carryHidden[0] = 0;
        break;
    case 1:
        w->carryHidden[1] = 0;
        break;
    case 2:
        w->carryHidden[2] = 0;
        break;
    }
}

/*
 * Returns 1 when carried item `part` (0-2) is shown; for `part` 6, 1 while
 * part 2 is unbroken (Em_CheckRageByHp and gm_em.c ask that).
 */
s32 EmBlangonga_CarriedShown(Em *em, u16 part)
{
    EmBlangongaWork *w = (EmBlangongaWork *)&em->work;

    switch (part) {
    case 0:
        if (w->carryHidden[0]) {
            return 0;
        }
        break;
    case 1:
        if (w->carryHidden[1]) {
            return 0;
        }
        break;
    case 2:
        if (w->carryHidden[2]) {
            return 0;
        }
        break;
    case 6:
        if (Em_PartBroken(em, 2) == 1) {
            return 0;
        }
        break;
    }
    return 1;
}

/* Returns 1 for the small kinds, the Blango and the Conga. */
s32 EmBlangonga_IsSmallKind(Em *em)
{
    u8 type;

    type = em->type;
    if ((type == 61) || (type == 62)) {
        return 1;
    }
    return 0;
}

/* Returns 1 for Blangonga and the Blango. */
static s32 EmBlangonga_IsBlangonga(Em *em)
{
    u8 type;

    type = em->type;
    if ((type == 51) || (type == 61)) {
        return 1;
    }
    return 0;
}

/* Returns 1 for Congalala and the Conga. */
static s32 EmBlangonga_IsCongalala(Em *em)
{
    u8 type;

    type = em->type;
    if ((type == 52) || (type == 62)) {
        return 1;
    }
    return 0;
}

/* Returns 1 for Rajang. */
static u64 EmBlangonga_IsRajang(Em *em)
{
    return em->type == EM_KIND_RAJANG;
}

/*
 * Returns 1 in the states a flash doesn't affect (Em_ApplyFlash): while
 * stateArmor, lying in a hold under status 1 or 2, and the trapped,
 * status-4 and stun states of EM_GROUP_REACT.
 */
s32 EmBlangonga_IgnoresFlash(Em *em)
{
    if (em->stateArmor == 1
        || (em->stateGroup == EM_GROUP_IDLE && em->state == BLANG_IDLE_STATUS2_DOWNED)
        || (em->stateGroup == EM_GROUP_IDLE && em->state == BLANG_IDLE_STATUS1_DOWNED)
        || (em->stateGroup == EM_GROUP_REACT && em->state == BLANG_REACT_TRAPPED)
        || (em->stateGroup == EM_GROUP_REACT && em->state == BLANG_REACT_TRAPPED_STRUGGLE)
        || (em->stateGroup == EM_GROUP_REACT && em->state == BLANG_REACT_STATUS4_DOWNED)
        || (em->stateGroup == EM_GROUP_REACT && em->state == BLANG_REACT_STATUS4_DOWNED_END)
        || (em->stateGroup == EM_GROUP_REACT && em->state == BLANG_REACT_STATUS2_DOWNED_END)
        || (em->stateGroup == EM_GROUP_REACT && em->state == BLANG_REACT_STUN_DOWNED)
        || (em->stateGroup == EM_GROUP_REACT && em->state == BLANG_REACT_STUN_DOWNED_END)) {
        return 1;
    }
    return 0;
}

/* Returns 1 while Em.stateArmor is set (buried, or out of reach in a scene). */
s32 EmBlangonga_InStateArmor(Em *em)
{
    return em->stateArmor != 0;
}

/*
 * The shadow's brightness (sub_main's Shadow_Draw): fades from 1 at 2000
 * above the floor to 0 at 3500 during BLANG_LEAP_OUT_OF_AREA and _DROP_IN,
 * 0 during BLANG_LEAP_CROSS_AREA, else 1.
 */
f32 EmBlangonga_ShadowFade(Em *em)
{
    f32 height;
    f32 fade;

    if (em->stateGroup == EM_GROUP_FLY) {
        switch (em->state) {
        case BLANG_LEAP_OUT_OF_AREA:
        case BLANG_LEAP_DROP_IN:
            height = em->posY - em->floorY;
            if (height < 2000.0f) {
                return 1.0f;
            }
            if (height < 3500.0f) {
                fade = 1.0f - ((height - 2000.0f) / 1500.0f);
            } else {
                fade = 0.0f;
            }
            return fade;
        case BLANG_LEAP_CROSS_AREA:
            return 0.0f;
        }
    }
    return 1.0f;
}

/*
 * Counts Em.rage's timer (EmBlangongaWork.rageTimer) up to its limit while
 * enraged and down to 0 otherwise (the limit: 15 frames, Congalala 60
 * enraged or 150); Rajang's rage begins with stage object 24.
 */
void EmBlangonga_UpdateRageTimer(Em *em)
{
    EmBlangongaWork *w = (EmBlangongaWork *)&em->work;
    s16 lim;

    switch (em->type) {
    case EM_KIND_RAJANG:
    default:
        lim = 15;
        break;
    case EM_KIND_CONGALALA:
        if (em->rage != 0) {
            lim = 60;
        } else {
            lim = 150;
        }
        break;
    }
    if (em->rage != 0) {
        switch (em->rageStep) {
        case 0:
            if (EmBlangonga_IsRajang(em) == 1) {
                MonsterFx_CreateOnEm(em, 24, 1.0f);
            }
            em->rageStep++;
        case 1:
            if (++w->rageTimer > lim) {
                w->rageTimer = lim;
                em->rageStep++;
            }
            break;
        case 2:
            w->rageTimer = lim;
            break;
        }
    } else {
        em->rageStep = 0;
        if (--w->rageTimer < 0) {
            w->rageTimer = 0;
        }
    }
}

/*
 * For gm_em.c's Congalala meshes: 1 while Congalala holds its food (frames
 * 36-382 of BLANG_MOT_EAT, or in its story scene once it has picked it up),
 * 2 in the scene before that, else 0.
 */
s32 EmBlangonga_EatPropMode(Em *em)
{
    EmBlangongaWork *w = (EmBlangongaWork *)&em->work;

    if (em->stateGroup == EM_GROUP_EVENT && em->state == BLANG_EVENT_CONGALALA_SCENE) {
        return w->sceneEating ? 1 : 2;
    }
    switch (em->motionId[0]) {
    case EM_MOTION_ID(0, BLANG_MOT_EAT):
        if (EmMotion_InFrameRange(em, 0, 36.0f, 382.0f)) {
            return 1;
        }
        break;
    }
    return 0;
}

/*
 * Steps the body colour (EmBlangongaWork.tint) toward the kind's target:
 * the Blangonga kinds' rage colour, or Blangonga's own colour above 30% hp
 * and a second one below; Congalala's pulsing rage colour or its calm one
 * (at once when the rage timer is full). 12 per frame (Congalala calming:
 * 6). Rajang and the Conga keep theirs.
 */
void EmBlangonga_UpdateTint(Em *em)
{
    EmBlangongaWork *w = (EmBlangongaWork *)&em->work;
    u8 col[3];
    s16 i;
    s16 diff;
    u8 *target;
    u8 *channel;
    u8 step;
    f32 pulse;
    u8 *target2;
    u8 *channel2;
    s16 j;
    s16 diff2;

    switch (em->type) {
    case EM_KIND_BLANGONGA:
    case EM_KIND_BLANGO:
        if (em->rage != 0) {
            col[0] = 255;
            col[1] = 100;
            col[2] = 90;
        } else if (em->type == EM_KIND_BLANGONGA) {
            if (em->hp > (s16) (0.3f * em->hpMax)) {
                col[0] = 115;
                col[1] = 235;
                col[2] = 255;
            } else {
                col[0] = 205;
                col[1] = 145;
                col[2] = 195;
            }
        } else {
            col[0] = 205;
            col[1] = 145;
            col[2] = 195;
        }
        for (i = 0, target = col; i < 3; i++, target++) {
            channel = &w->tint[i];
            diff = *target - *channel;
            if (Math_Absf(diff) <= 12.0f) {
                *channel = *target;
            } else if (diff < 0) {
                *channel -= 12;
            } else {
                *channel += 12;
            }
        }
        break;
    case EM_KIND_CONGALALA:
        if (em->rage != 0) {
            pulse = 0.5f * (1.0f + Math_Sin((6.2831855f * (f32) (gScene.frame << 10)) / 65536.0f));
            col[0] = 255.0f + -100.0f * pulse;
            col[1] = 40.0f + 10.0f * pulse;
            col[2] = 10.0f + 5.0f * pulse;
        } else {
            col[0] = 153;
            col[1] = 128;
            col[2] = 115;
        }
        for (j = 0, target2 = col; j < 3; j++, target2++) {
            if (EmBlangonga_RageTimerFull(em) == 1) {
                w->tint[j] = *target2;
            } else {
                channel2 = &w->tint[j];
                diff2 = *target2 - *channel2;
                if (em->rage != 0) {
                    step = 12;
                } else {
                    step = 6;
                }
                if (Math_Absf(diff2) <= step) {
                    *channel2 = *target2;
                } else if (diff2 < 0) {
                    *channel2 -= step;
                } else {
                    *channel2 += step;
                }
            }
        }
        break;
    default:
        return;
    }
}

/*
 * Returns 1 when the rage timer has reached its limit
 * (EmBlangonga_UpdateRageTimer's); also read by sub_main for Rajang's mesh.
 */
s32 EmBlangonga_RageTimerFull(Em *em)
{
    EmBlangongaWork *w = (EmBlangongaWork *)&em->work;
    s16 k;

    switch (em->type) {
    case EM_KIND_RAJANG:
    default:
        k = 15;
        break;
    case EM_KIND_CONGALALA:
        if (em->rage != 0) {
            k = 60;
        } else {
            k = 150;
        }
        break;
    }
    return w->rageTimer == k;
}

/*
 * Creates the lying puffs of a lying monster (Em_SpawnLyingPuffs) at scale
 * 1.6: at bone 1 for `kind` 1 (Congalala), at BLANG_BONE_BREATH for the
 * others.
 */
void EmBlangonga_SpawnLyingPuffs(Em *em, s32 kind)
{
    f32 pos[3];
    s16 bone;

    switch ((u8) kind) {
    default:
        pos[1] = -55.0f;
        bone = BLANG_BONE_BREATH;
        pos[0] = 0.0f;
        pos[2] = 50.0f;
        break;
    case 1:
        bone = 1;
        pos[1] = -30.0f;
        pos[0] = 0.0f;
        pos[2] = 100.0f;
        break;
    }
    Em_SpawnLyingPuffs(em, bone, pos, 1.6f);
}

/*
 * Returns 1 when a monster under stateArmor in the player's area is off the
 * snow (ScriptQuery 1 is 0): EmBlangonga_Update then ends its state.
 */
s32 EmBlangonga_ShouldEndState(Em *em)
{
    if (!((s32)Player_InCurrentArea(em) & 0xFF)) {
        return 0;
    }
    if ((em->stateArmor == 1) && !(EmBlangonga_ScriptQuery(em, BLANG_QUERY_ON_SNOW) & 0xFF)) {
        return 1;
    }
    return 0;
}

/*
 * Starts the family's movement curves (EmBlangonga_VelYCurves and
 * SpeedCurves) for `kind`: 0 the vertical speed from frame 0, 1 from frame
 * 24, 2 both speeds; the velocity is cleared.
 */
void EmBlangonga_StartCurves(Em *em, u8 kind)
{
    EmBlangongaWork *w = (EmBlangongaWork *)&em->work;

    w->curveNone = 0;
    w->curveVelY = 0;
    w->curveSpeed = 0;
    w->curveKind = kind;
    switch (w->curveKind) {
    case 0:
        w->curveVelY = 1;
        w->curveFrame = 0;
        break;
    case 1:
        w->curveVelY = 1;
        w->curveFrame = 24;
        break;
    case 2:
        w->curveVelY = 1;
        w->curveSpeed = 1;
        w->curveFrame = 0;
        break;
    }
    Player_ClearVelocity(em);
}

extern EmCurveKey *EmBlangonga_VelYCurves[];

/*
 * Sets Em.velY from the curve keys around the curve frame (curve
 * curveKind); at the table's end applies gravity or stops, and returns 2.
 */
static u8 EmBlangonga_CurveVelY(Em *em, EmWork *work)
{
    EmBlangongaWork *w = (EmBlangongaWork *) work;
    EmCurveKey *key;
    f32 frame;
    f32 keyFrame;
    s32 i;
    EmCurveKey *tbl;
    u8 ret;

    tbl = EmBlangonga_VelYCurves[((EmBlangongaWork *)work)->curveKind];
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

extern EmCurveKey *EmBlangonga_SpeedCurves[];

/*
 * Sets Em.velZ from the speed curve keys around the curve frame; returns 4
 * at the table's end.
 */
static u8 EmBlangonga_CurveSpeed(Em *em, EmWork *work)
{
    EmBlangongaWork *w = (EmBlangongaWork *) work;
    EmCurveKey *key;
    f32 frame;
    f32 keyFrame;
    s32 i;
    EmCurveKey *tbl;
    u8 ret;

    tbl = EmBlangonga_SpeedCurves[((EmBlangongaWork *)work)->curveKind];
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

/*
 * Runs the started curves, advances the curve frame by the motion's step
 * and, in the air, moves along rotY (Em_MoveRotatedAccel). Returns the
 * ended curves' bits (2, 4).
 */
u8 EmBlangonga_UpdateCurves(Em *em)
{
    EmBlangongaWork *w = (EmBlangongaWork *)&em->work;
    u8 ret = 0;

    if (w->curveVelY != 0) {
        ret |= EmBlangonga_CurveVelY(em, (EmWork *) w);
    }
    if (w->curveSpeed != 0) {
        ret |= EmBlangonga_CurveSpeed(em, (EmWork *) w);
    }
    w->curveFrame += (s16) em->motion[0].step;
    if (em->stance == EM_STANCE_AIR) {
        w->accelAngX = 0;
        w->accelAngY = em->rotY;
        w->accelAngZ = 0;
        Em_MoveRotatedAccel(em, &w->accelAngX);
    }
    return ret;
}

/* The family's entry points per area: its entry of gm_sub's table of every family's area entry points. */
extern EmAreaPoints *EmBlangonga_AreaEntries;

/*
 * Moves the monster into Em.nextArea: from the area's entry list (gm_sub's
 * per-area table) it picks the entry point and the point to face by the
 * area it came from, or uses the start position; it starts 5000 above the
 * floor.
 */
void EmBlangonga_EnterNewArea(Em *em)
{
    f32 *pts;
    s16 entryNo;
    s16 faceNo;

    em->prevArea = em->area;
    em->area = em->nextArea;
    pts = ((EmAreaPoints *)Em_FindAreaEntry(em, (struct EmIdEntry *)EmBlangonga_AreaEntries, em->area))->pts;
    if (pts == NULL) {
        Vec_Copy(&em->posX, &gStartPos[em->area * 3]);
        em->rotY = 0;
    } else {
        switch (Area_DayId(em->area)) {
        case AREA_SNOWY_MOUNTAINS_3:
        default:
        case AREA_SNOWY_MOUNTAINS_4:
        case AREA_SNOWY_MOUNTAINS_5:
        case AREA_SNOWY_MOUNTAINS_6:
        case AREA_SNOWY_MOUNTAINS_7:
        case AREA_SNOWY_MOUNTAINS_8:
        case AREA_JUNGLE_9:
        case AREA_VOLCANO_1:
        case AREA_VOLCANO_4:
        case AREA_VOLCANO_5:
        case AREA_VOLCANO_7:
            entryNo = 0;
            faceNo = 1;
            break;
        case AREA_JUNGLE_1:
            switch (Area_DayId(em->prevArea)) {
            case AREA_JUNGLE_2:
                entryNo = 5;
                faceNo = 4;
                break;
            default:
                entryNo = 0;
                faceNo = 1;
                break;
            }
            break;
        case AREA_JUNGLE_2:
            switch (Area_DayId(em->prevArea)) {
            case AREA_JUNGLE_3:
                entryNo = 4;
                faceNo = 3;
                break;
            default:
                entryNo = 0;
                faceNo = 1;
                break;
            }
            break;
        case AREA_JUNGLE_3:
            switch (Area_DayId(em->prevArea)) {
            case AREA_JUNGLE_2:
            case AREA_JUNGLE_6:
                entryNo = 4;
                faceNo = 3;
                break;
            default:
                entryNo = 0;
                faceNo = 1;
                break;
            }
            break;
        case AREA_JUNGLE_6:
            switch (Area_DayId(em->prevArea)) {
            case AREA_JUNGLE_1:
            case AREA_JUNGLE_9:
                entryNo = 8;
                faceNo = 7;
                break;
            default:
                entryNo = 0;
                faceNo = 1;
                break;
            }
            break;
        case AREA_JUNGLE_10:
            entryNo = 4;
            faceNo = 3;
            break;
        case AREA_SWAMP_2:
            entryNo = 0;
            faceNo = 1;
            break;
        case AREA_SWAMP_5:
            switch (Area_DayId(em->prevArea)) {
            case AREA_SWAMP_6:
                entryNo = 5;
                faceNo = 4;
                break;
            default:
                entryNo = 0;
                faceNo = 1;
                break;
            }
            break;
        case AREA_SWAMP_6:
            switch (Area_DayId(em->prevArea)) {
            case AREA_SWAMP_7:
                entryNo = 6;
                faceNo = 5;
                break;
            default:
                entryNo = 0;
                faceNo = 1;
                break;
            }
            break;
        case AREA_SWAMP_7:
            switch (Area_DayId(em->prevArea)) {
            case AREA_SWAMP_9:
                entryNo = 2;
                faceNo = 1;
                break;
            default:
                entryNo = 0;
                faceNo = 1;
                break;
            }
            break;
        case AREA_SWAMP_8:
            entryNo = 0;
            faceNo = 1;
            break;
        case AREA_SWAMP_9:
            switch (Area_DayId(em->prevArea)) {
            case AREA_SWAMP_2:
                entryNo = 4;
                faceNo = 3;
                break;
            case AREA_SWAMP_8:
                entryNo = 6;
                faceNo = 5;
                break;
            default:
                entryNo = 0;
                faceNo = 1;
                break;
            }
            break;
        case AREA_VOLCANO_6:
            switch (Area_DayId(em->prevArea)) {
            case AREA_VOLCANO_5:
                entryNo = 4;
                faceNo = 3;
                break;
            case AREA_VOLCANO_7:
            default:
                entryNo = 0;
                faceNo = 1;
                break;
            }
            break;
        }
        Vec_Copy(&em->posX, &pts[entryNo * 3]);
        em->rotY = Math_AngleXZ(&em->posX, &pts[faceNo * 3]);
    }
    em->posY += 5000.0f;
    Vec_Copy(&em->prevPosX, &em->posX);
}

/* BLANG_IDLE_NONE: does nothing. */
void EmBlangonga_IdleNone(void)
{
}

/*
 * BLANG_IDLE: plays BLANG_MOT_IDLE once, then asks the script for the next
 * state.
 */
void EmBlangonga_Idle(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_IDLE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->scriptPending = 1;
        }
        break;
    }
}

/*
 * BLANG_IDLE_REST: lies down and lies for 5400 frames, blind and without
 * flinching, regaining hp (1 every 6 frames up to 40% of hpMax; the
 * Congalala kinds every 8 frames and Rajang every 6, up to 50%) with the
 * periodic effect; then BLANG_IDLE_REST_END.
 */
void EmBlangonga_Rest(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_LIE_DOWN, 0, 0);
        em->timer = 5400;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_LIE, 0, 0);
        }
        break;
    case 2:
        if (EmBlangonga_IsBlangonga(em) == 1) {
            Em_RegenHp(em, 1, (s32) (0.4f * (f32) em->hpMax), 6);
        } else if (EmBlangonga_IsCongalala(em) == 1) {
            Em_RegenHp(em, 1, (s32) (0.5f * (f32) em->hpMax), 8);
        } else {
            Em_RegenHp(em, 1, (s32) (0.5f * (f32) em->hpMax), 6);
        }
        EmBlangonga_SpawnLyingPuffs(em, 0);
        if (--em->timer <= 0) {
            em->step++;
            Em_ClearLowHpNeed(em);
            EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_REST_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_REST_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * BLANG_IDLE_STATUS2_END, _STATUS1_END and _REST_END: gets up (blind
 * meanwhile) and ends the state.
 */
void EmBlangonga_GetUp(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_GET_UP, 0, 0);
        em->blind = 2;
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_IDLE_STATUS2_COLLAPSE (variant 0) and _STATUS1_COLLAPSE (1): falls
 * over and lies, blind and without flinching, until the status timer runs
 * out, then the status's _END state.
 */
void EmBlangonga_StatusCollapse(Em *em, EmWork *work, s32 variant)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_COLLAPSE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_LIE, 0, 0);
        }
        break;
    case 2:
        EmBlangonga_SpawnLyingPuffs(em, 0);
        if (--em->timer <= 0) {
            em->step++;
            if (!(variant & 0xFF)) {
                EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_STATUS2_END, EM_SYNC_OWNER_FLAGGED);
                break;
            }
            EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_STATUS1_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 3:
        if (em->remote == 0) {
            if (!(variant & 0xFF)) {
                EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_STATUS2_END, EM_SYNC_OWNER_FLAGGED);
                break;
            }
            EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_STATUS1_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/* BLANG_IDLE_ALERT: plays BLANG_MOT_ALERT_IDLE once and ends the state. */
void EmBlangonga_AlertIdle(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_ALERT_IDLE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * The roars: BLANG_IDLE_ROAR, _ROAR_SIGNAL, _ROAR_RAGE and
 * _ROAR_PENDING_RAGE (variant 0), _ROAR_CALL (1: at frame 120 adds the
 * sensed object of kind 3 other monsters react to, until the first part
 * break), _ROAR_RAISE_RAGE (2: at frame 50 raises the rage by hp and cuts a
 * flash's blindness short).
 */
void EmBlangonga_Roar(Em *em, EmWork *work, s32 variant)
{
    s32 k;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, BLANG_MOT_ROAR, 0, 0);
        break;
    case 1:
        k = variant & 0xFF;
        switch (k) {
        case 0:
            break;
        case 1:
            if (((s8) em->partBreakCount <= 0) && (EmMotion_PassedFrameScaled(em, 120.0f, 0U) != 0)) {
                Vec_Set((f32 *) &work->moveAngBX, em->posX, em->posY, em->posZ);
                work->moveAngCX = 0x453B8000;
                ((EmBlangongaWork *)work)->sense.area = (s16) *(u8 *)&em->area;
                ((EmBlangongaWork *)work)->sense.kind = 3;
                EmSenseObjs_Add((EmObj *)&work->moveAngBX);
                ((EmBlangongaWork *)work)->senseListed = 1;
            }
            break;
        case 2:
            if (EmMotion_PassedFrameScaled(em, 50.0f, 0U) != 0) {
                em->noReact = 1;
                Em_CheckRageByHp(em, (s32) em->rageThreshold);
                if (em->blindTimer > 0) {
                    em->blindTimer = 1;
                }
            }
            break;
        }
        if (em->motion[0].playing == 0) {
            em->noReact = 0;
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_IDLE_TURN_ABOUT (variant 0) and _B (1, the other way): turns half
 * round over frames 102-202 and back over 402-452, then ends the state.
 */
void EmBlangonga_TurnAbout(Em *em, EmWork *work, s32 variant)
{
    u16 turn = 0;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        switch (variant & 0xFF) {
        case 0:
        default:
            Em_SetMotion(em, BLANG_MOT_TURN_ABOUT, 0, 0);
            break;
        case 1:
            Em_SetMotion(em, BLANG_MOT_TURN_ABOUT_B, 0, 0);
            break;
        }
        break;
    case 1:
        if (EmMotion_InFrameRange(em, 0, 102.0f, 202.0f)) {
            turn = -327.68f * em->motionSpeed;
        } else if (EmMotion_InFrameRange(em, 0, 402.0f, 452.0f)) {
            turn = 655.36f * em->motionSpeed;
        }
        switch (variant & 0xFF) {
        case 1:
            turn = -turn;
            break;
        }
        em->rotY += turn;
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_IDLE_STATUS4_SHAKE_OFF, _STUN_SHAKE_OFF and _HOLD2_SHAKE_OFF: plays
 * BLANG_MOT_SHAKE_OFF once and ends the state.
 */
void EmBlangonga_ShakeOff(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, BLANG_MOT_SHAKE_OFF, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_IDLE_EAT_NEED2: eats without flinching for 300 frames, filling need
 * 2, then regains 5% of hpMax and clears need 2's state.
 */
void EmBlangonga_EatNeed2(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_NO_FLINCH);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, BLANG_MOT_EAT_NEED2, 0, 0);
        em->timer = 300;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_EAT_NEED2_END, 0, 0);
            Em_AddHp(em, (s32) (0.05f * (f32) em->hpMax));
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_ClearNeed2State(em);
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
    Em_AddNeed2(em, 222);
}

/*
 * BLANG_IDLE_EAT_NEED1, _EAT_NEED1_B (variant 0) and _EAT_KIND_A..D (1):
 * eats without flinching, turning in frames 4-26 and back in 370-418, then
 * regains 5% (variant 1: 1%) of hpMax, refills need 1 and clears its state.
 */
void EmBlangonga_EatNeed1(Em *em, EmWork *work, s32 variant)
{
    u16 turn = 0;

    Em_SetHitReactMode(em, EM_HIT_REACT_NO_FLINCH);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_EAT, 0, 0);
        em->timer = 240;
        break;
    case 1:
        if (EmMotion_InFrameRange(em, 0, 4.0f, 26.0f)) {
            turn = 1638.4f * em->motionSpeed;
        } else if (EmMotion_InFrameRange(em, 0, 370.0f, 418.0f)) {
            turn = -712.34784f * em->motionSpeed;
        }
        em->rotY += turn;
        if (em->motion[0].playing == 0) {
            em->step++;
            if ((variant & 0xFF) == 0) {
                Em_AddHp(em, 0.05f * em->hpMax);
            } else {
                Em_AddHp(em, 0.01f * em->hpMax);
            }
            Em_AddNeed1(em, em->need1Max);
            Em_ClearNeed1State(em);
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_IDLE_CLEAR_PAINT: plays its motion and clears the paint mark
 * (Em.painted, paintTimer) when it ends.
 */
void EmBlangonga_ClearPaint(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_ROAR_C_END, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->painted = 0;
            em->paintTimer = 0;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_IDLE_LEAVE_TRAP: climbs out of the trap (the Blangonga kinds and
 * Rajang with BLANG_MOT_EMERGE from frame 14, the Congalala kinds with
 * their own motion), then ends the state.
 */
void EmBlangonga_LeaveTrap(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        switch (em->type) {
        case EM_KIND_BLANGONGA:
        default:
        case EM_KIND_BLANGO:
            Em_SetMotion(em, BLANG_MOT_EMERGE, 6, 14);
            break;
        case EM_KIND_CONGALALA:
        case EM_KIND_CONGA:
            Em_SetMotion(em, BLANG_MOT_CONGALALA_CLIMB_OUT, 0, 0);
            break;
        }
        em->stance = EM_STANCE_GROUND;
        em->noReact = 1;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_IDLE_EAT_START (variant 0, BLANG_MOT_EAT to frame 300) and
 * _EAT_START_B (1, Congalala's motion to frame 200): starts eating, then
 * BLANG_IDLE_EAT while the motion plays on.
 */
void EmBlangonga_EatStart(Em *em, EmWork *work, s32 variant)
{
    f32 frame;
    s32 k;

    k = variant & 0xFF;
    if (k == 0) {
        Em_SetHitReactMode(em, EM_HIT_REACT_NO_FLINCH);
    }
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        if (k == 0) {
            Em_SetMotion(em, BLANG_MOT_EAT, 0, 0);
            break;
        }
        Em_SetMotion(em, BLANG_MOT_CONGALALA_EAT, 0, 0);
        break;
    case 1:
        if (k == 0) {
            frame = 300.0f;
        } else {
            frame = 200.0f;
        }
        if (EmMotion_PassedFrameScaled(em, frame, 0U) != 0) {
            em->step++;
            EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_EAT, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_EAT, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/* BLANG_IDLE_ACTION_A: plays BLANG_MOT_BACK_STRIKE once and ends the state. */
void EmBlangonga_IdleActionA(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_BACK_STRIKE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_IDLE_ROAR_C: plays BLANG_MOT_ROAR_C (at frame 128 the roar's
 * rings, ElementBurst type 5, and attack row 34, the roar's wide sphere of
 * no power), then BLANG_MOT_ROAR_C_END, and ends the state.
 */
void EmBlangonga_RoarC(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_ROAR_C, 0, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 128.0f, 0U) != 0) {
            ElementBurst_CreateOnEm(em, 5, 1, 0.4f * em->scaleX);
            BlangongaAttack_Create(em, 34);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_ROAR_C_END, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_IDLE_ALERT_BRIEF: BLANG_MOT_ALERT_IDLE for 10 frames, then ends the
 * state.
 */
void EmBlangonga_AlertBrief(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, BLANG_MOT_ALERT_IDLE, 0, 0);
        em->timer = 10;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_IDLE_STATUS2_DOWNED (variant 0) and _STATUS1_DOWNED (1): lies in
 * the hold (holdTimer kept at 5), blind and without flinching, with the
 * periodic effect and a sound every 135 frames, until the status timer runs
 * out; then BLANG_REACT_STATUS2_DOWNED_END or _STATUS1_DOWNED_END.
 * Blangonga first comes up out of the pit (BLANG_MOT_EMERGE to frame 14).
 */
void EmBlangonga_StatusDowned(Em *em, EmWork *work, s32 variant)
{
    u16 motion;

    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->holdTimer = 5;
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        if ((em->type == EM_KIND_BLANGONGA) && (motion = em->motionId[0], (motion != EM_MOTION_ID(0, BLANG_MOT_EMERGE))) && (motion != EM_MOTION_ID(0, BLANG_MOT_STRUGGLE)) && (motion != EM_MOTION_ID(0, BLANG_MOT_DOWNED))) {
            Em_SetMotion(em, BLANG_MOT_EMERGE, 0, 0);
            em->stateArmor = 1;
            em->noPushTimer = 5;
        } else {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_DOWNED, 0, 0);
        }
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].blend != 0) {
            em->stateArmor = 1;
            em->noPushTimer = 5;
        }
        if (EmMotion_ReachedFrame(em, 0, 14.0f) != 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_DOWNED, 0, 0);
        }
        break;
    case 2:
        if (((s32) em->timer % 135) == 0) {
            Em_PlaySeAtBone(em, 82, BLANG_BONE_HEAD);
        }
        if (em->type == EM_KIND_CONGALALA) {
            EmBlangonga_SpawnLyingPuffs(em, 1);
        } else {
            EmBlangonga_SpawnLyingPuffs(em, 0);
        }
        if (--em->timer <= 0) {
            em->step++;
            if (!(variant & 0xFF)) {
                EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_STATUS2_DOWNED_END, EM_SYNC_OWNER);
                break;
            }
            EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_STATUS1_DOWNED_END, EM_SYNC_OWNER);
        }
        break;
    case 3:
        if (em->type == EM_KIND_CONGALALA) {
            EmBlangonga_SpawnLyingPuffs(em, 1);
        } else {
            EmBlangonga_SpawnLyingPuffs(em, 0);
        }
        if (em->remote == 0) {
            if (!(variant & 0xFF)) {
                EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_STATUS2_DOWNED_END, EM_SYNC_OWNER);
                break;
            }
            EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_STATUS1_DOWNED_END, EM_SYNC_OWNER);
        }
        break;
    }
}

/*
 * BLANG_IDLE_ROAR_B: roars (Blangonga with BLANG_MOT_ROAR_B, the others
 * BLANG_MOT_ROAR) and ends the state.
 */
void EmBlangonga_RoarB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        if (em->type == EM_KIND_BLANGONGA) {
            Em_SetMotion(em, BLANG_MOT_ROAR_B, 0, 0);
            break;
        }
        Em_SetMotion(em, BLANG_MOT_ROAR, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_IDLE_ACTION_C: plays BLANG_MOT_IDLE_ACTION_C once and ends the
 * state.
 */
void EmBlangonga_IdleActionC(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, BLANG_MOT_IDLE_ACTION_C, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_IDLE_BURIED: waits under the snow, unhittable, unpushable, sensing
 * nothing and under stateArmor, until the script moves it on.
 */
void EmBlangonga_WaitBuried(Em *em)
{
    em->senseDelay = 5;
    em->noPushTimer = 5;
    em->noHitTimer = 5;
    em->stateArmor = 1;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotionRow(em, BLANG_MOT_BURIED, 0, 0);
        break;
    case 1:
        break;
    }
}

/*
 * BLANG_IDLE_LEAVE_MINE_POINT: plays BLANG_MOT_ROAR_C and at frame
 * 184 leaves a timed mine point where it stands, then ends the state.
 */
void EmBlangonga_LeaveMinePoint(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_ROAR_C, 0, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 184.0f, 0U) != 0) {
            EmMine_CreateTimed0(em, &em->posX);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_IDLE_EAT: selects sight row 0 and reaction set 0, waits for the
 * eating motion to end, and ends the state.
 */
void EmBlangonga_Eat(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SelectSightRow(em, 0);
        Em_LoadReactionSet(em, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * Runs the current EM_GROUP_IDLE state, passing the shared functions their
 * variant.
 */
void EmBlangonga_RunIdleState(Em *em, EmWork *work)
{
    switch (em->state) {
    case BLANG_IDLE_NONE:
        EmBlangonga_IdleNone();
        break;
    case BLANG_IDLE:
        EmBlangonga_Idle(em);
        break;
    case BLANG_IDLE_REST:
        EmBlangonga_Rest(em);
        break;
    case BLANG_IDLE_STATUS2_END:
        EmBlangonga_GetUp(em);
        break;
    case BLANG_IDLE_STATUS2_COLLAPSE:
        EmBlangonga_StatusCollapse(em, work, 0);
        break;
    case BLANG_IDLE_ALERT:
        EmBlangonga_AlertIdle(em);
        break;
    case BLANG_IDLE_ROAR:
        EmBlangonga_Roar(em, work, 0);
        break;
    case BLANG_IDLE_TURN_ABOUT:
        EmBlangonga_TurnAbout(em, work, 0);
        break;
    case BLANG_IDLE_TURN_ABOUT_B:
        EmBlangonga_TurnAbout(em, work, 1);
        break;
    case BLANG_IDLE_STATUS4_SHAKE_OFF:
        EmBlangonga_ShakeOff(em);
        break;
    case BLANG_IDLE_EAT_NEED2:
        EmBlangonga_EatNeed2(em);
        break;
    case BLANG_IDLE_EAT_NEED1:
        EmBlangonga_EatNeed1(em, work, 0);
        break;
    case BLANG_IDLE_CLEAR_PAINT:
        EmBlangonga_ClearPaint(em);
        break;
    case BLANG_IDLE_LEAVE_TRAP:
        EmBlangonga_LeaveTrap(em);
        break;
    case BLANG_IDLE_EAT_START_B:
        EmBlangonga_EatStart(em, work, 1);
        break;
    case BLANG_IDLE_ACTION_A:
        EmBlangonga_IdleActionA(em);
        break;
    case BLANG_IDLE_ROAR_C:
        EmBlangonga_RoarC(em);
        break;
    case BLANG_IDLE_ALERT_BRIEF:
        EmBlangonga_AlertBrief(em);
        break;
    case BLANG_IDLE_STATUS2_DOWNED:
        EmBlangonga_StatusDowned(em, work, 0);
        break;
    case BLANG_IDLE_ROAR_CALL:
        EmBlangonga_Roar(em, work, 1);
        break;
    case BLANG_IDLE_ROAR_RAISE_RAGE:
        EmBlangonga_Roar(em, work, 2);
        break;
    case BLANG_IDLE_ROAR_B:
        EmBlangonga_RoarB(em);
        break;
    case BLANG_IDLE_ACTION_C:
        EmBlangonga_IdleActionC(em);
        break;
    case BLANG_IDLE_BURIED:
        EmBlangonga_WaitBuried(em);
        break;
    case BLANG_IDLE_STUN_SHAKE_OFF:
        EmBlangonga_ShakeOff(em);
        break;
    case BLANG_IDLE_REST_END:
        EmBlangonga_GetUp(em);
        break;
    case BLANG_IDLE_HOLD2_SHAKE_OFF:
        EmBlangonga_ShakeOff(em);
        break;
    case BLANG_IDLE_EAT_KIND_A:
        EmBlangonga_EatNeed1(em, work, 1);
        break;
    case BLANG_IDLE_EAT_KIND_B:
        EmBlangonga_EatNeed1(em, work, 1);
        break;
    case BLANG_IDLE_EAT_KIND_C:
        EmBlangonga_EatNeed1(em, work, 1);
        break;
    case BLANG_IDLE_EAT_KIND_D:
        EmBlangonga_EatNeed1(em, work, 1);
        break;
    case BLANG_IDLE_LEAVE_MINE_POINT:
        EmBlangonga_LeaveMinePoint(em);
        break;
    case BLANG_IDLE_EAT_NEED1_B:
        EmBlangonga_EatNeed1(em, work, 0);
        break;
    case BLANG_IDLE_EAT:
        EmBlangonga_Eat(em);
        break;
    case BLANG_IDLE_ROAR_SIGNAL:
        EmBlangonga_Roar(em, work, 0);
        break;
    case BLANG_IDLE_STATUS1_DOWNED:
        EmBlangonga_StatusDowned(em, work, 1);
        break;
    case BLANG_IDLE_STATUS1_COLLAPSE:
        EmBlangonga_StatusCollapse(em, work, 1);
        break;
    case BLANG_IDLE_STATUS1_END:
        EmBlangonga_GetUp(em);
        break;
    case BLANG_IDLE_EAT_START:
        EmBlangonga_EatStart(em, work, 0);
        break;
    case BLANG_IDLE_ROAR_RAGE:
        EmBlangonga_Roar(em, work, 0);
        break;
    case BLANG_IDLE_ROAR_PENDING_RAGE:
        EmBlangonga_Roar(em, work, 0);
        break;
    }
}

/*
 * BLANG_MOVE_TURN (variant 0: to the target) and _TURN_AWAY (1): turns with
 * the slow turn parameters until the turn is done, then ends the state.
 */
void EmBlangonga_Turn(Em *em, s32 unused, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        switch (variant & 0xFF) {
        case 0:
        default:
            Em_StartTurnToTarget(em, &EmBlangonga_TurnData.turn[0]);
            break;
        case 1:
            Em_StartTurnAway(em, &EmBlangonga_TurnData.turn[0]);
            break;
        }
        break;
    case 1:
        if (Em_TurnStep(em, &EmBlangonga_TurnData.turn[0]) == 1) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_MOVE_WALK (variant 0; the Blango walks faster) and _LIMP (1): steps
 * to the goal (Em_StepToGoal), then ends the state.
 */
void EmBlangonga_WalkToGoal(Em *em, s32 unused, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        switch (variant & 0xFF) {
        case 0:
        default:
            if (em->type == EM_KIND_BLANGO) {
                em->speedBonus = 25;
            }
            Em_SetMotion(em, BLANG_MOT_WALK, 0, 0);
            break;
        case 1:
            Em_SetMotion(em, BLANG_MOT_LIMP, 0, 0);
            break;
        }
        break;
    case 1:
        if (Em_StepToGoal(em, 128, 0) == 1) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_MOVE_TURN_QUICK (variant 0), _TURN_TO_EXIT (1) and _TURN_AWAY_QUICK
 * (2): turns with the quick turn parameters and ends the state;
 * _TURN_TO_EXIT first takes the area's exit point as its goal and then
 * leaps out (BLANG_LEAP_OUT_OF_AREA).
 */
void EmBlangonga_TurnQuick(Em *em, s32 unused, s32 variant)
{
    s32 k;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        k = variant & 0xFF;
        em->lookAtTarget = 0;
        switch (k) {
        case 1:
            Em_AreaExitPoint(em, (f32 *) em->goal);
            Em_StartTurnToTarget(em, &EmBlangonga_TurnData.turn[1]);
            break;
        case 2:
            Em_StartTurnAway(em, &EmBlangonga_TurnData.turn[1]);
            break;
        default:
            Em_StartTurnToTarget(em, &EmBlangonga_TurnData.turn[1]);
        }
        break;
    case 1:
        if (Em_TurnStep(em, &EmBlangonga_TurnData.turn[1]) == 1) {
            if ((variant & 0xFF) == 1) {
                em->step++;
                EmBlangonga_ChangeState(em, EM_GROUP_FLY, BLANG_LEAP_OUT_OF_AREA, EM_SYNC_OWNER_FLAGGED);
                break;
            }
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    case 2:
        if (em->remote == 0) {
            em->step = 1;
        }
        break;
    }
}

/*
 * BLANG_MOVE_ACTION_A: plays BLANG_MOT_MOVE_ACTION_A once and ends the
 * state.
 */
void EmBlangonga_MoveActionA(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_MOVE_ACTION_A, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_MOVE_ACTION_B: plays BLANG_MOT_MOVE_ACTION_B once and ends the
 * state.
 */
void EmBlangonga_MoveActionB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_MOVE_ACTION_B, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_MOVE_ACTION_C: plays BLANG_MOT_MOVE_ACTION_C once and ends the
 * state.
 */
void EmBlangonga_MoveActionC(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_MOVE_ACTION_C, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_MOVE_ACTION_D and _ACTION_E: plays BLANG_MOT_MOVE_ACTION_D once and
 * ends the state.
 */
void EmBlangonga_MoveActionD(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_MOVE_ACTION_D, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_MOVE_START_AND_APPROACH (variant 0), _LIMP_APPROACH (1), _APPROACH
 * (2) and _START_AND_WALK (3): walks (0 and 3 start with
 * BLANG_MOT_WALK_START); 0-2 turn to the goal and stop after 120 frames or
 * within 700 (scaled) of it, 3 steps all the way to the goal.
 */
void EmBlangonga_Approach(Em *em, s32 unused, s32 variant)
{
    switch (em->step) {
    case 0:
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        em->timer = 120;
        switch (variant & 0xFF) {
        case 0:
        case 3:
        default:
            em->step++;
            Em_SetMotion(em, BLANG_MOT_WALK_START, 0, 0);
            break;
        case 1:
            em->step = 2;
            Em_SetMotion(em, BLANG_MOT_LIMP, 0, 0);
            break;
        case 2:
            em->step = 2;
            Em_SetMotion(em, BLANG_MOT_WALK, 0, 0);
            break;
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_WALK, 0, 68);
        }
    case 2:
        switch (variant & 0xFF) {
        case 3:
            if (Em_StepToGoal(em, 128, 0) == 1) {
                em->step++;
                EmBlangonga_EndState(em, 0, 0);
            }
            break;
        default:
            if (em->hasGoal != 0) {
                Em_TurnToGoal(em, 128, 0);
            }
            if (--em->timer <= 0 || (em->hasGoal != 0 && Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) < 700.0f * em->scaleX)) {
                em->step++;
                EmBlangonga_EndState(em, 0, 0);
            }
            break;
        }
        break;
    }
}

/* Runs the current EM_GROUP_MOVE state. The work area comes as an s32. */
void EmBlangonga_RunMoveState(Em *em, s32 work)
{
    switch (em->state) {
    case BLANG_MOVE_TURN:
        EmBlangonga_Turn(em, work, 0);
        break;
    case BLANG_MOVE_WALK:
        EmBlangonga_WalkToGoal(em, work, 0);
        break;
    case BLANG_MOVE_LIMP:
        EmBlangonga_WalkToGoal(em, work, 1);
        break;
    case BLANG_MOVE_TURN_QUICK:
        EmBlangonga_TurnQuick(em, work, 0);
        break;
    case BLANG_MOVE_ACTION_A:
        EmBlangonga_MoveActionA(em);
        break;
    case BLANG_MOVE_ACTION_B:
        EmBlangonga_MoveActionB(em);
        break;
    case BLANG_MOVE_ACTION_C:
        EmBlangonga_MoveActionC(em);
        break;
    case BLANG_MOVE_ACTION_D:
        EmBlangonga_MoveActionD(em);
        break;
    case BLANG_MOVE_TURN_TO_EXIT:
        EmBlangonga_TurnQuick(em, work, 1);
        break;
    case BLANG_MOVE_TURN_AWAY:
        EmBlangonga_Turn(em, work, 1);
        break;
    case BLANG_MOVE_TURN_AWAY_QUICK:
        EmBlangonga_TurnQuick(em, work, 2);
        break;
    case BLANG_MOVE_START_AND_APPROACH:
        EmBlangonga_Approach(em, work, 0);
        break;
    case BLANG_MOVE_LIMP_APPROACH:
        EmBlangonga_Approach(em, work, 1);
        break;
    case BLANG_MOVE_APPROACH:
        EmBlangonga_Approach(em, work, 2);
        break;
    case BLANG_MOVE_START_AND_WALK:
        EmBlangonga_Approach(em, work, 3);
        break;
    case BLANG_MOVE_ACTION_E:
        EmBlangonga_MoveActionD(em);
        
    default:
        break;
    }
}

/*
 * BLANG_LEAP_JUMP (variant 0), _JUMP_HIGH (1) and _JUMP_TO_GOAL (2):
 * crouches, takes off (frame 52, 56 or 48 by kind) on a curve, falls after
 * the apex and lands with a camera shake. The variant gives the speeds (2:
 * those that land on the goal in 60 frames); the Congalala kinds jump
 * lower.
 */
void EmBlangonga_Jump(Em *em, EmWork *work, u8 variant)
{
    f32 frame;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, BLANG_MOT_JUMP, 0, 0);
        Player_ClearVelocity(em);
        break;
    case 1:
        switch (em->type) {
        case EM_KIND_BLANGONGA:
        default:
        case EM_KIND_BLANGO:
            frame = 52.0f;
            break;
        case EM_KIND_CONGALALA:
        case EM_KIND_CONGA:
            frame = 56.0f;
            break;
        case EM_KIND_RAJANG:
            frame = 48.0f;
            break;
        }
        if (EmMotion_ReachedFrame(em, 0, frame)) {
            em->noReact = 1;
            em->stance = EM_STANCE_AIR;
            em->step++;
            if (variant == 2) {
                em->accelY = -6.4f;
                em->velY = (em->goal[1] - em->posY) / 60.0f - 60.0f * (0.5f * em->accelY);
                em->velZ = Vec_DistanceXZ(em->goal, &em->posX) / 60.0f;
            } else {
                switch (em->type) {
                case EM_KIND_BLANGONGA:
                default:
                case EM_KIND_BLANGO:
                case EM_KIND_RAJANG:
                    switch (variant) {
                    case 0:
                    default:
                        em->velY = 105.0f;
                        em->velZ = 30.0f;
                        em->accelY = -6.4f;
                        break;
                    case 1:
                        em->velY = 150.0f;
                        em->velZ = 45.0f;
                        em->accelY = -5.0f;
                        break;
                    }
                    break;
                case 52:
                case 62:
                    em->velY = 80.0f;
                    em->velZ = 45.0f;
                    em->accelY = -4.0f;
                    break;
                }
            }
            work->accelAngX = em->rotX;
            work->accelAngY = em->rotY;
            work->accelAngZ = 0;
        }
        break;
    case 2:
        Em_MoveRotatedAccel(em, &work->accelAngX);
        if (em->velY <= 0.0f) {
            Em_SetMotion(em, BLANG_MOT_JUMP_FALL, 0, 0);
            em->step++;
        }
        break;
    case 3:
        Em_MoveRotatedAccel(em, &work->accelAngX);
        if (em->posY <= em->floorY) {
            em->posY = em->floorY;
            em->step++;
            em->stance = EM_STANCE_GROUND;
            CamShake_ReqAIf(em, 5);
            switch (em->type) {
            case EM_KIND_BLANGONGA:
            default:
            case EM_KIND_BLANGO:
            case EM_KIND_RAJANG:
                Em_SetMotion(em, BLANG_MOT_JUMP_LAND, 0, 0);
                break;
            case EM_KIND_CONGALALA:
            case EM_KIND_CONGA:
                Em_SetMotion(em, BLANG_MOT_JUMP_LAND, 8, 0);
                break;
            }
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->noReact = 0;
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_LEAP_HOP_SHORT (variant 0), _HOP (1) and _HOP_LONG (2), the
 * Congalala kinds: takes off at frame 26 with a hit at frame 50, lands with
 * a camera shake. The variant gives the forward speed.
 */
void EmBlangonga_Hop(Em *em, EmWork *work, s32 variant)
{
    if (EmBlangonga_IsCongalala(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
        return;
    }
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, BLANG_MOT_CONGALALA_HOP, 0, 0);
        Player_ClearVelocity(em);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 26.0f)) {
            em->step++;
            em->stance = EM_STANCE_AIR;
            switch (variant & 0xFF) {
            case 0:
                em->velZ = 4.0f;
                break;
            case 1:
            default:
                em->velZ = 44.0f;
                break;
            case 2:
                em->velZ = 76.0f;
                break;
            }
            if (em->scaleY < 1.0f) {
                em->velY = 70.0f * em->scaleY;
                em->accelY = -5.5f * em->scaleY;
            } else {
                em->velY = 70.0f;
                em->accelY = -5.5f;
            }
            work->accelAngX = em->rotX;
            work->accelAngY = em->rotY;
            work->accelAngZ = 0;
            em->noReact = 1;
        }
        break;
    case 2:
        if (EmMotion_PassedFrameScaled(em, 50.0f, 0)) {
            BlangongaAttack_Create(em, 33);
        }
        Em_MoveRotatedAccel(em, &work->accelAngX);
        if (em->posY <= em->floorY) {
            em->posY = em->floorY;
            em->step++;
            em->stance = EM_STANCE_GROUND;
            if (EmBlangonga_IsSmallKind(em) == 1) {
                CamShake_ReqAIf(em, 2);
            } else {
                CamShake_ReqAIf(em, 5);
            }
            Em_SetMotion(em, BLANG_MOT_HOP_LAND, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_LEAP_DIG_IN (the Blangonga kinds): digs into the snow (the shadow
 * goes from frame 80), then BLANG_LEAP_BURIED.
 */
void EmBlangonga_DigIn(Em *em)
{
    if (EmBlangonga_IsBlangonga(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
        return;
    }
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        if (em->motionId[0] != EM_MOTION_ID(0, BLANG_MOT_DIG_IN)) {
            Em_SetMotion(em, BLANG_MOT_DIG_IN, 0, 0);
        }
        em->noReact = 1;
        em->stance = EM_STANCE_GROUND;
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 80.0f) != 0) {
            em->hideShadowHold = 2;
            em->hideShadow = 2;
            em->noPushTimer = 2;
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            em->noReact = 0;
            EmBlangonga_EndBuriedState(em);
        }
        break;
    }
    em->guardTimer = 2;
}

/*
 * BLANG_LEAP_BURIED: stays under the snow (stateArmor); when its motion
 * ends the owner hands the choice back to the script
 * (EmBlangonga_EndBuriedState).
 */
void EmBlangonga_Buried(Em *em)
{
    if (EmBlangonga_IsBlangonga(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
    } else {
        em->stateArmor = 1;
        em->noPushTimer = 2;
        switch (em->step) {
        case 0:
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->lookAtTarget = 0;
            Em_SetMotion(em, BLANG_MOT_BURIED, 0, 0);
            break;
        case 1:
            if ((em->remote == 0) && (em->motion[0].playing == 0)) {
                em->step++;
                EmBlangonga_EndBuriedState(em);
            }
            break;
        }
    }
}

/*
 * BLANG_LEAP_DIG_OUT (variant 0) and _DIG_OUT_RAGE (1: the rage raised by
 * hp on entry): comes up out of the snow with two hits (frames 2 and 52),
 * the shadow back from frame 40, then ends the state.
 */
void EmBlangonga_DigOut(Em *em, EmWork *work, s32 variant)
{
    if (EmBlangonga_IsBlangonga(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
        return;
    }
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        if (em->motionId[0] != EM_MOTION_ID(0, BLANG_MOT_EMERGE)) {
            Em_SetMotion(em, BLANG_MOT_EMERGE, 0, 0);
            em->stateArmor = 1;
            em->noPushTimer = 2;
        } else if (em->motion[0].blend != 0) {
            em->stateArmor = 1;
            em->noPushTimer = 2;
        }
        em->noReact = 1;
        switch (variant & 0xFF) {
        case 1:
            Em_CheckRageByHp(em, em->rageThreshold);
            break;
        }
        break;
    case 1:
        if (em->motion[0].blend != 0) {
            em->stateArmor = 1;
            em->noPushTimer = 2;
        } else {
            if (EmMotion_PassedFrameScaled(em, 2.0f, 0)) {
                BlangongaAttack_Create(em, 6);
            }
            if (EmMotion_PassedFrameScaled(em, 52.0f, 0)) {
                BlangongaAttack_Create(em, 7);
            }
            if (!EmMotion_ReachedFrame(em, 0, 40.0f)) {
                em->hideShadowHold = 2;
                em->hideShadow = 2;
            }
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            em->noReact = 0;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
    em->guardTimer = 2;
}

/*
 * BLANG_LEAP_BURIED_TURN: turns toward the goal under the snow, checking
 * every 40 frames whether it faces it.
 */
void EmBlangonga_BuriedTurn(Em *em)
{
    u32 spd;
    u32 angleDiff;

    if (EmBlangonga_IsBlangonga(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
        return;
    }
    em->stateArmor = 1;
    em->noPushTimer = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        Em_SetMotion(em, BLANG_MOT_BURIED, 0, 0);
        em->timer = 40;
        break;
    case 1:
        spd = 409.6f * em->motionSpeed;
        angleDiff = (em->aimAngle - *(u16 *) &em->rotY) & 0xFFFF;
        if (--em->timer <= 0) {
            if (((angleDiff + spd) & 0xFFFF) < spd * 2) {
                em->step++;
                EmBlangonga_EndBuriedState(em);
            } else {
                em->timer = 40;
            }
        } else {
            if (((angleDiff + spd) & 0xFFFF) < spd * 2) {
                em->rotY = em->aimAngle;
            } else if (angleDiff < (ANGLE_TURN / 2)) {
                em->rotY = (em->rotY + spd) & 0xFFFF;
            } else {
                em->rotY = (em->rotY - spd) & 0xFFFF;
            }
        }
        break;
    }
}

/*
 * BLANG_LEAP_BURIED_MOVE: moves under the snow to the goal, rumbling the
 * local player's pad when within 1000 of it.
 */
void EmBlangonga_BuriedMove(Em *em, EmWork *work)
{
    Player *player;

    player = &gPlayers[gScene.localPlayer];
    if (EmBlangonga_IsBlangonga(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
    } else {
        em->stateArmor = 1;
        em->noPushTimer = 2;
        switch (em->step) {
        case 0:
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->lookAtTarget = 0;
            Em_SetMotion(em, BLANG_MOT_BURIED, 0, 100);
            Player_ClearVelocity(em);
            em->velZ = 70.0f;
            work->accelAngX = em->rotX;
            work->accelAngY = em->rotY;
            work->accelAngZ = 0;
            break;
        case 1:
            Em_MoveRotatedAccel(em, &work->accelAngX);
            Em_StepToGoal(em, 16, 2);
            if (!(gScene.frame & 7) && (Obj_SameArea(em, player) != 0) && (Obj_InCurrentArea(player) != 0) && (Vec_Distance((f32 *) &em->posX, (f32 *) player->pos) < 1000.0f)) {
                Vib_Start(0, 2);
            }
            if (em->goalDist <= 0.0f) {
                em->step++;
                EmBlangonga_EndBuriedState(em);
            }
            break;
        }
    }
}

/*
 * BLANG_LEAP_BURIED_WAIT: waits 90 frames under the snow, then
 * BLANG_LEAP_BURIED.
 */
void EmBlangonga_BuriedWait(Em *em)
{
    em->stateArmor = 1;
    em->noPushTimer = 2;
    if (EmBlangonga_IsBlangonga(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->lookAtTarget = 0;
            Em_SetMotion(em, BLANG_MOT_BURIED, 0, 0);
            em->timer = 90;
            break;
        case 1:
            if (--em->timer <= 0) {
                em->step++;
                EmBlangonga_EndBuriedState(em);
            }
            break;
        }
    }
}

/*
 * BLANG_LEAP_OUT_OF_AREA: takes off and rises for 150 frames, then drops
 * into the destination area (BLANG_LEAP_DROP_IN) or crosses to the next one
 * (BLANG_LEAP_CROSS_AREA). A remote copy waits for the owner's change.
 */
void EmBlangonga_LeapOutOfArea(Em *em, EmWork *work)
{
    f32 takeOffFrame;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, BLANG_MOT_JUMP, 0, 0);
        Player_ClearVelocity(em);
        break;
    case 1:
        switch (em->type) {
        case EM_KIND_BLANGONGA:
        default:
        case EM_KIND_BLANGO:
            takeOffFrame = 52.0f;
            break;
        case EM_KIND_CONGALALA:
        case EM_KIND_CONGA:
            takeOffFrame = 56.0f;
            break;
        case EM_KIND_RAJANG:
            takeOffFrame = 48.0f;
            break;
        }
        if (EmMotion_ReachedFrame(em, 0, takeOffFrame)) {
            em->step++;
            em->noReact = 1;
            em->stance = EM_STANCE_AIR;
            em->velY = 230.0f;
            em->velZ = 70.0f;
            em->accelY = -1.55f;
            work->accelAngX = em->rotX;
            work->accelAngY = em->rotY;
            work->accelAngZ = 0;
            em->timer = 150;
        }
        break;
    case 2:
        Em_MoveRotatedAccel(em, &work->accelAngX);
        if (--em->timer <= 0) {
            em->step++;
            if (em->nextArea == *(u16 *) &em->destArea || *(u16 *) &em->destArea == AREA_NONE) {
                EmBlangonga_EnterNewArea(em);
                EmBlangonga_ChangeState(em, EM_GROUP_FLY, BLANG_LEAP_DROP_IN, EM_SYNC_SEND);
                break;
            }
            Em_EnterNextArea(em);
            if (em->remote == 0) {
                em->goalNo = em->nextArea = *(u16 *) &em->destArea;
                em->goalKind = 3;
                Em_ComputeGoal(em, em->goal);
            }
            EmBlangonga_ChangeState(em, EM_GROUP_FLY, BLANG_LEAP_CROSS_AREA, EM_SYNC_SEND);
        }
        break;
    case 3:
        if (em->remote == 0) {
            em->step = 2;
        }
        break;
    }
    em->guardTimer = 2;
}

/*
 * BLANG_LEAP_DROP_IN: falls into the area from above and lands with a
 * camera shake; Blangonga and Rajang then restart their script.
 */
void EmBlangonga_DropIn(Em *em, EmWork *work)
{
    u16 leftArea;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        em->noReact = 1;
        Em_SetMotionRow(em, BLANG_MOT_JUMP_FALL, 0, 0);
        Player_ClearVelocity(em);
        em->velY = -30.0f;
        em->velZ = 0.0f;
        em->accelY = -4.7f;
        work->accelAngX = em->rotX;
        work->accelAngY = em->rotY;
        work->accelAngZ = 0;
        if (em->type == EM_KIND_BLANGONGA || em->type == EM_KIND_RAJANG) {
            leftArea = em->prevArea;
            Em_RestartScript(em);
            em->prevArea = leftArea;
        }
        break;
    case 1:
        Em_MoveRotatedAccel(em, &work->accelAngX);
        if (em->posY <= em->floorY) {
            em->posY = em->floorY;
            em->step++;
            em->stance = EM_STANCE_GROUND;
            CamShake_ReqAIf(em, 5);
            switch (em->type) {
            case EM_KIND_BLANGONGA:
            default:
            case EM_KIND_BLANGO:
            case EM_KIND_RAJANG:
                Em_SetMotion(em, BLANG_MOT_JUMP_LAND, 0, 0);
                break;
            case EM_KIND_CONGALALA:
            case EM_KIND_CONGA:
                Em_SetMotion(em, BLANG_MOT_JUMP_LAND, 8, 0);
                break;
            }
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->noReact = 0;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_LEAP_CROSS_AREA: unseen and unhittable, runs to the area's exit
 * until it leaves (or 300 frames), then changes area as
 * BLANG_LEAP_OUT_OF_AREA does.
 */
void EmBlangonga_CrossArea(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noPushTimer = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        em->noReact = 1;
        Em_SetMotion(em, BLANG_MOT_JUMP, 0, 56);
        Player_ClearVelocity(em);
        em->velZ = 70.0f;
        Em_AreaExitPoint(em, (f32 *) em->goal);
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->rotY = em->aimAngle;
        em->timer = 300;
        work->accelAngX = em->rotX;
        work->accelAngY = em->rotY;
        work->accelAngZ = 0;
        break;
    case 1:
        Em_MoveRotated(em, &work->accelAngX);
        if (!Area_ContainsPos(em->area, &em->posX) || --em->timer <= 0) {
            em->step++;
        }
        break;
    case 2:
        em->step++;
        em->timer = 600;
        if (em->nextArea == (u16) em->destArea || (u16) em->destArea == AREA_NONE) {
            EmBlangonga_EnterNewArea(em);
            EmBlangonga_ChangeState(em, EM_GROUP_FLY, BLANG_LEAP_DROP_IN, EM_SYNC_SEND);
            break;
        }
        Em_EnterNextArea(em);
        if (em->remote == 0) {
            em->goalNo = em->nextArea = *(u16 *) &em->destArea;
            em->goalKind = 3;
            Em_ComputeGoal(em, em->goal);
        }
        EmBlangonga_ChangeState(em, EM_GROUP_FLY, BLANG_LEAP_CROSS_AREA, EM_SYNC_SEND);
        break;
    case 3:
        if (em->remote == 0) {
            em->step = 2;
        }
        break;
    }
}

/* BLANG_LEAP_BURIED_WAIT_RANDOM: waits 1-16 random frames under the snow. */
void EmBlangonga_BuriedWaitRandom(Em *em)
{
    em->stateArmor = 1;
    em->noPushTimer = 2;
    em->noReact = 1;
    if (EmBlangonga_IsBlangonga(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->lookAtTarget = 0;
            Em_SetMotion(em, BLANG_MOT_BURIED, 0, 0);
            em->timer = (Rand_Next(1) & 0xFFFF & 0xF) + 1;
            break;
        case 1:
            if (--em->timer <= 0) {
                em->step++;
                EmBlangonga_EndBuriedState(em);
            }
            break;
        }
    }
}

/*
 * Runs the current group-2 state (the family's EM_GROUP_FLY: jumps, leaps
 * and burrowing).
 */
void EmBlangonga_RunLeapState(Em *em, EmWork *work)
{
    switch (em->state) {
    case BLANG_LEAP_JUMP:
        EmBlangonga_Jump(em, work, 0);
        break;
    case BLANG_LEAP_HOP_SHORT:
        EmBlangonga_Hop(em, work, 0);
        break;
    case BLANG_LEAP_DIG_IN:
        EmBlangonga_DigIn(em);
        break;
    case BLANG_LEAP_BURIED:
        EmBlangonga_Buried(em);
        break;
    case BLANG_LEAP_DIG_OUT:
        EmBlangonga_DigOut(em, work, 0);
        break;
    case BLANG_LEAP_BURIED_TURN:
        EmBlangonga_BuriedTurn(em);
        break;
    case BLANG_LEAP_BURIED_MOVE:
        EmBlangonga_BuriedMove(em, work);
        break;
    case BLANG_LEAP_BURIED_WAIT:
        EmBlangonga_BuriedWait(em);
        break;
    case BLANG_LEAP_OUT_OF_AREA:
        EmBlangonga_LeapOutOfArea(em, work);
        break;
    case BLANG_LEAP_DROP_IN:
        EmBlangonga_DropIn(em, work);
        break;
    case BLANG_LEAP_DIG_OUT_RAGE:
        EmBlangonga_DigOut(em, work, 1);
        break;
    case BLANG_LEAP_CROSS_AREA:
        EmBlangonga_CrossArea(em, work);
        break;
    case BLANG_LEAP_JUMP_HIGH:
        EmBlangonga_Jump(em, work, 1);
        break;
    case BLANG_LEAP_BURIED_WAIT_RANDOM:
        EmBlangonga_BuriedWaitRandom(em);
        break;
    case BLANG_LEAP_HOP:
        EmBlangonga_Hop(em, work, 1);
        break;
    case BLANG_LEAP_HOP_LONG:
        EmBlangonga_Hop(em, work, 2);
        break;
    case BLANG_LEAP_JUMP_TO_GOAL:
        EmBlangonga_Jump(em, work, 2);
        
    default:
        break;
    }
}

/*
 * BLANG_ATK_TRIPLE_HIT (the Blangonga kinds): turns to the goal for 40
 * frames and hits three times (frames 36, 64 and 86; the first behind it).
 */
void EmBlangonga_TripleHit(Em *em)
{
    if (EmBlangonga_IsBlangonga(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, BLANG_MOT_TRIPLE_HIT, 0, 0);
            break;
        case 1:
            if (EmMotion_ReachedFrame(em, 0, 40.0f) == 0) {
                Em_TurnToGoal(em, 96, 0);
            }
            if (EmMotion_PassedFrameScaled(em, 36.0f, 0U) != 0) {
                BlangongaAttack_Create(em, 18);
            }
            if (EmMotion_PassedFrameScaled(em, 64.0f, 0U) != 0) {
                BlangongaAttack_Create(em, 24);
            }
            if (EmMotion_PassedFrameScaled(em, 86.0f, 0U) != 0) {
                BlangongaAttack_Create(em, 1);
            }
            if (em->motion[0].playing == 0) {
                em->step++;
                Em_SetMotion(em, BLANG_MOT_TRIPLE_HIT_END, 0, 0);
            }
            break;
        case 2:
            if (em->motion[0].playing == 0) {
                em->step++;
                EmBlangonga_EndState(em, 0, 0);
            }
            break;
        }
    }
}

/*
 * BLANG_ATK_SLAM (variant 0) and _SLAM_HEAVY (1): winds up for 20 or 40
 * frames and slams (a hit at frame 20); at frame 30 the large kinds add a
 * heavier hit and a camera shake, and the heavy slam also brings down the
 * area's falling objects (Blangonga and Rajang).
 */
void EmBlangonga_Slam(Em *em, EmWork *work, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_SLAM_WINDUP, 0, 0);
        switch (variant & 0xFF) {
        case 0:
        default:
            em->timer = 20;
            break;
        case 1:
            em->timer = 40;
            break;
        }
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_SLAM, 0, 0);
        }
        break;
    case 2:
        if (EmMotion_PassedFrameScaled(em, 20.0f, 0)) {
            BlangongaAttack_Create(em, 2);
        }
        if (EmBlangonga_IsSmallKind(em) == 0 && EmMotion_PassedFrameScaled(em, 30.0f, 0)) {
            switch (variant & 0xFF) {
            case 1:
                BlangongaAttack_Create(em, 8);
                CamShake_ReqAIf(em, 5);
                if (EmBlangonga_IsBlangonga(em) == 1 || EmBlangonga_IsRajang(em) == 1) {
                    EmBlangonga_DropFallingObjects(em);
                }
                break;
            default:
                CamShake_ReqAIf(em, 3);
                BlangongaAttack_Create(em, 25);
                break;
            }
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_ATK_STRIKE_A (variant 0) and _B (1), any kind: one blow with a hit
 * at frame 14 (the small kinds' weaker hit).
 */
void EmBlangonga_Strike(Em *em, EmWork *work, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        switch (variant & 0xFF) {
        case 0:
        default:
            Em_SetMotion(em, BLANG_MOT_STRIKE_A, 0, 0);
            break;
        case 1:
            Em_SetMotion(em, BLANG_MOT_STRIKE_B, 0, 0);
            break;
        }
        break;
    case 1:
        switch (variant & 0xFF) {
        case 0:
        default:
            if (EmMotion_PassedFrameScaled(em, 14.0f, 0)) {
                if (EmBlangonga_IsSmallKind(em) == 0) {
                    BlangongaAttack_Create(em, 3);
                } else {
                    BlangongaAttack_Create(em, 21);
                }
            }
            break;
        case 1:
            if (EmMotion_PassedFrameScaled(em, 14.0f, 0)) {
                if (EmBlangonga_IsSmallKind(em) == 0) {
                    BlangongaAttack_Create(em, 4);
                } else {
                    BlangongaAttack_Create(em, 22);
                }
            }
            break;
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_ATK_CONGALALA_EMISSION (the Congalala kinds): plays its motion and
 * at frame 110 starts Congalala's gas (EMFX_CONGALALA_GAS).
 */
void EmBlangonga_CongalalaEmission(Em *em)
{
    if (EmBlangonga_IsCongalala(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, BLANG_MOT_CONGALALA_EMISSION, 0, 0);
            break;
        case 1:
            if (EmMotion_PassedFrameScaled(em, 110.0f, 0U) != 0) {
                EmEffectAttack_Create(em, EMFX_CONGALALA_GAS, 0, 0, 0);
            }
            if (em->motion[0].playing == 0) {
                em->step++;
                EmBlangonga_EndState(em, 0, 0);
            }
            break;
        }
    }
}

/*
 * BLANG_ATK_BLANGO_THROW (the Blango): throws a projectile
 * (EMRANGED_BLANGO_THROW) from BLANG_BONE_HAND_B at frame 74.
 */
void EmBlangonga_BlangoThrow(Em *em)
{
    if (em->type != EM_KIND_BLANGO) {
        EmBlangonga_EndState(em, 0, 0);
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, BLANG_MOT_THROW, 0, 0);
            break;
        case 1:
            if (EmMotion_PassedFrameScaled(em, 74.0f, 0U) != 0) {
                EmRangedAttack_CreateAtBone(em, BLANG_BONE_HAND_B, EMRANGED_BLANGO_THROW, 0, 64512, 0);
            }
            if (em->motion[0].playing == 0) {
                em->step++;
                EmBlangonga_EndState(em, 0, 0);
            }
            break;
        }
    }
}

/*
 * BLANG_ATK_CHARGE (not the Congalala kinds): runs at the goal with a hit
 * along, then finishes with BLANG_MOT_CHARGE_END and its hits.
 */
void EmBlangonga_Charge(Em *em)
{
    if (EmBlangonga_IsCongalala(em) == 1) {
        EmBlangonga_EndState(em, 0, 0);
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            em->stateVar0 = 0;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, BLANG_MOT_CHARGE, 0, 0);
            break;
        case 1:
            if (EmMotion_PassedFrameScaled(em, 6.0f, 0U) != 0) {
                BlangongaAttack_Create(em, 15);
            }
            if ((em->stateVar0 == 0) && (Em_StepToGoal(em, 128, 0) == 1)) {
                em->step++;
                em->stateVar0++;
                Em_SetMotion(em, BLANG_MOT_CHARGE_END, 0, 0);
            }
            break;
        case 2:
            if (EmBlangonga_IsBlangonga(em) == 1) {
                if (EmMotion_PassedFrameScaled(em, 10.0f, 0U) != 0) {
                    BlangongaAttack_Create(em, 17);
                }
                if (EmMotion_PassedFrameScaled(em, 20.0f, 0U) != 0) {
                    if (EmBlangonga_IsSmallKind(em) == 1) {
                        BlangongaAttack_Create(em, 26);
                    } else {
                        BlangongaAttack_Create(em, 5);
                    }
                }
            } else if (EmMotion_PassedFrameScaled(em, 22.0f, 0U) != 0) {
                BlangongaAttack_Create(em, 37);
            }
            if (em->motion[0].playing == 0) {
                em->step++;
                EmBlangonga_EndState(em, 0, 0);
            }
            break;
        }
    }
}

/*
 * BLANG_ATK_DIG_THROW_A, _B and _C (variants 0-2; Blangonga): digs up a
 * lump with a hit at frame 98 and throws it (EMRANGED_BLANGONGA_LUMP from
 * BLANG_BONE_FIST) at frame 102; the variant sets the throw's pitch.
 */
void EmBlangonga_DigThrow(Em *em, EmWork *work, s32 variant)
{
    u16 ang;

    if (em->type != EM_KIND_BLANGONGA) {
        EmBlangonga_EndState(em, 0, 0);
        return;
    }
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_DIG_THROW, 0, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 98.0f, 0)) {
            BlangongaAttack_Create(em, 19);
        }
        if (EmMotion_PassedFrameScaled(em, 102.0f, 0)) {
            switch (variant & 0xFF) {
            default:
                ang = 0xC300;
                break;
            case 1:
                ang = 0xC500;
                break;
            case 2:
                ang = 0xC900;
                break;
            }
            EmRangedAttack_CreateAtBone(em, BLANG_BONE_FIST, EMRANGED_BLANGONGA_LUMP, 0, ang, 0);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_ATK_CONGALALA_EATEN_EMISSION (Congalala): at frame 100, when it has
 * eaten (eatenKind), creates stage object 23 and effect 33 of kind 1 by
 * what it ate.
 */
void EmBlangonga_CongalalaEatenEmission(Em *em, EmWork *work)
{
    EmBlangongaWork *w = (EmBlangongaWork *)work;

    if (em->type != EM_KIND_CONGALALA) {
        EmBlangonga_EndState(em, 0, 0);
        return;
    }
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_CONGALALA_EATEN_EMISSION, 0, 0);
        break;
    case 1:
        if (w->eatenKind != 0 && EmMotion_PassedFrameScaled(em, 100.0f, 0)) {
            MonsterFx_CreateOnEmVariant(em, 23, w->eatenKind - 1, 1.0f);
            EmEffectAttack_Create(em, EMFX_CONGALALA_BREATH, w->eatenKind - 1, 0, 0);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_ATK_CONGALALA_COMBO (the Congalala kinds): a long combo with a hit
 * from frame 26, half a turn over frames 170-180 and an impact at frame 200
 * (the large kind), then BLANG_MOT_CONGALALA_COMBO_END.
 */
void EmBlangonga_CongalalaCombo(Em *em, EmWork *work)
{
    if (EmBlangonga_IsCongalala(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
        return;
    }
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_CONGALALA_COMBO, 0, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 26.0f, 0)) {
            BlangongaAttack_Create(em, 31);
        }
        if (EmMotion_InFrameRange(em, 0, 170.0f, 180.0f)) {
            em->rotY += 0xCCC;
        }
        if (EmBlangonga_IsSmallKind(em) == 0 && EmMotion_PassedFrameScaled(em, 200.0f, 0)) {
            EmImpactAttack_CreateAt(em, &em->posX, 16);
            CamShake_ReqAIf(em, 5);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_CONGALALA_COMBO_END, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_ATK_CONGALALA_HITZONE_COMBO (Congalala): a hit from frame 40 with
 * the second damage table (hitzoneSet 1) until the first motion ends, then
 * a second blow with a hit at frame 88.
 */
void EmBlangonga_CongalalaHitzoneCombo(Em *em)
{
    if (em->type != EM_KIND_CONGALALA) {
        EmBlangonga_EndState(em, 0, 0);
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, BLANG_MOT_CONGALALA_HITZONE_COMBO, 0, 0);
            break;
        case 1:
            if (EmMotion_PassedFrameScaled(em, 40.0f, 0U) != 0) {
                BlangongaAttack_Create(em, 32);
                em->hitzoneSet = 1;
            }
            if (em->motion[0].playing == 0) {
                em->step++;
                Em_SetMotion(em, BLANG_MOT_CONGALALA_HITZONE_COMBO_END, 0, 0);
                em->hitzoneSet = 0;
            }
            break;
        case 2:
            if (EmMotion_PassedFrameScaled(em, 88.0f, 0U) != 0) {
                BlangongaAttack_Create(em, 41);
            }
            if (em->motion[0].playing == 0) {
                em->step++;
                EmBlangonga_EndState(em, 0, 0);
            }
            break;
        }
    }
}

/*
 * BLANG_ATK_CONGALALA_THROW_A (variant 0) and _B (1), the Congalala kinds:
 * throws a projectile (EMRANGED_CONGALALA_THROW) from
 * BLANG_BONE_CONGALALA_THROW at frame 66; the variant sets its pitch.
 */
void EmBlangonga_CongalalaThrow(Em *em, EmWork *work, s32 variant)
{
    if (EmBlangonga_IsCongalala(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, BLANG_MOT_THROW, 0, 0);
            break;
        case 1:
            if (!(variant & 0xFF)) {
                if (EmMotion_PassedFrameScaled(em, 66.0f, 0U) != 0) {
                    EmRangedAttack_CreateAtBone(em, BLANG_BONE_CONGALALA_THROW, EMRANGED_CONGALALA_THROW, 0, 2367, 0);
                }
            } else if (EmMotion_PassedFrameScaled(em, 66.0f, 0U) != 0) {
                EmRangedAttack_CreateAtBone(em, BLANG_BONE_CONGALALA_THROW, EMRANGED_CONGALALA_THROW, 0, 8192, 0);
            }
            if (em->motion[0].playing == 0) {
                em->step++;
                EmBlangonga_EndState(em, 0, 0);
            }
            break;
        }
    }
}

/*
 * BLANG_ATK_BACK_STRIKE: plays BLANG_MOT_BACK_STRIKE to its end; its hits
 * come from EmBlangonga_MotionEffects.
 */
void EmBlangonga_BackStrike(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_BACK_STRIKE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_ATK_EFFECT_STRIKE (the Blangonga kinds): at frame 98 creates stage
 * object 8 on itself and a hit, then plays the end motion. It also works
 * out a point in front of bone 1 that nothing reads.
 */
void EmBlangonga_EffectStrike(Em *em, EmWork *work)
{
    f32 pos[3];
    f32 ofs[3];
    void *bone;

    if (EmBlangonga_IsBlangonga(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
        return;
    }
    switch (em->step) {
    case 0:
        em->step++;
        em->stateVar1 = 0;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_EFFECT_STRIKE, 0, 0);
        break;
    case 1:
        if (em->stateVar1 == 0 && EmMotion_PassedFrameScaled(em, 98.0f, 0)) {
            MonsterFx_CreateOnEm(em, 8, 0.75f * em->scaleX);
            BlangongaAttack_Create(em, 10);
            if (Player_InCurrentArea(em)) {
                ofs[1] = 145.0f;
                ofs[0] = 0.0f;
                ofs[2] = 485.0f;
                bone = Em_Bone(em, 1);
                Vec_RotateSelf(ofs, bone);
                Mtx_GetTrans(pos, bone);
                Vec_Add(pos, pos, ofs);
            }
            em->stateVar1++;
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_EFFECT_STRIKE_END, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_ATK_RAJANG_COMBO_SLAM_A (variant 0) and _B (1): a blow with a hit,
 * then 3 (or 1) frames after its end a slam whose impact at frame 26 shakes
 * the camera and brings down the area's falling objects.
 */
void EmBlangonga_RajangComboSlam(Em *em, EmWork *work, s32 variant)
{
    f32 pos[3];

    if (EmBlangonga_IsRajang(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
        return;
    }
    switch (em->step) {
    case 0:
        em->step++;
        em->stateVar0 = 0;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_STRIKE_A, 0, 0);
        if ((variant & 0xFF) == 0) {
            em->timer = 3;
        } else {
            em->timer = 1;
        }
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 16.0f, 0) && em->stateVar0 == 0) {
            em->stateVar0++;
            BlangongaAttack_Create(em, 36);
        }
        if (em->motion[0].playing == 0 && --em->timer <= 0) {
            em->step++;
            em->stateVar0 = 0;
            Em_SetMotion(em, BLANG_MOT_STRIKE_B, 0, 0);
        }
        break;
    case 2:
        if (EmMotion_PassedFrameScaled(em, 26.0f, 0) && em->stateVar0 == 0) {
            em->stateVar0++;
            Em_BonePos(em, BLANG_BONE_FIST, pos);
            pos[1] = em->floorY;
            EmImpactAttack_CreateAt(em, pos, 15);
            CamShake_ReqAIf(em, 5);
            EmBlangonga_DropFallingObjects(em);
            if ((s32)Player_InCurrentArea(em) & 0xFF) {
                AttackBurst_CreateAtScaled(pos, *(u16 *) &em->rotY, 10, 0.4f * em->scaleX);
            }
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_ATK_RAJANG_SHOT_A (variant 0) and _B (1): aims and at frame 84
 * fires effect 33 of kind 2 with stage object 37; the variant sets the
 * pitch.
 */
void EmBlangonga_RajangShot(Em *em, EmWork *work, s32 variant)
{
    if (EmBlangonga_IsRajang(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
        return;
    }
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_RAJANG_AIM, 2, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 84.0f, 0)) {
            em->step++;
            switch (variant & 0xFF) {
            case 0:
            default:
                EmEffectAttack_Create(em, EMFX_RAJANG_SHOT, 0, 1820, 0);
                MonsterFx_CreateOnEmAngled(em, 37, 1820, 0, 1.0f);
                break;
            case 1:
                EmEffectAttack_Create(em, EMFX_RAJANG_SHOT, 0, 63717, 0);
                MonsterFx_CreateOnEmAngled(em, 37, 63717, 0, 1.0f);
                break;
            }
            Em_SetMotion(em, BLANG_MOT_RAJANG_AIM_END, 2, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_ATK_RAJANG_BEAM_A (variant 0) and _B (1): aims and at frame 80
 * fires the beam (EMRANGED_RAJANG_BEAM, the variant passed on) from bone 1 for
 * 40 frames.
 */
void EmBlangonga_RajangBeam(Em *em, EmWork *work, s32 variant)
{
    if (EmBlangonga_IsRajang(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
        return;
    }
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_RAJANG_AIM, 0, 0);
        em->timer = 2;
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 80.0f, 0)) {
            switch (variant & 0xFF) {
            case 0:
            default:
                EmRangedAttack_CreateAtBoneTimed(em, 1, EMRANGED_RAJANG_BEAM, 0, 0, 0, 40);
                break;
            case 1:
                EmRangedAttack_CreateAtBoneTimed(em, 1, EMRANGED_RAJANG_BEAM, 1, 0, 0, 40);
                break;
            }
        }
        if (em->motion[0].playing == 0) {
            em->timer--;
        }
        if (em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_RAJANG_AIM_END, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_ATK_RAJANG_JUMP_SHOT (variant 0) and BLANG_REACT_TRAP_ESCAPE (1:
 * unguarded hits and the trap's hold set): jumps, fires a shot downward
 * from the air (frame 22) while held above the floor, and lands when the
 * curves end.
 */
void EmBlangonga_RajangJumpShot(Em *em, EmWork *work, s32 variant)
{
    f32 posY;
    s32 ended;
    s32 k;

    if (EmBlangonga_IsRajang(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
    } else {
        k = variant & 0xFF;
        if (k == 1) {
            em->guardTimer = 2;
        }
        switch (em->step) {
        case 0:
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, BLANG_MOT_RAJANG_JUMP, 0, 0);
            if (k == 1) {
                em->holdTimer = 50;
            }
            break;
        case 1:
            if (EmMotion_PassedFrameScaled(em, 24.0f, 0U) != 0) {
                em->step++;
                em->stance = EM_STANCE_AIR;
                em->posY += 170.0f;
                EmBlangonga_StartCurves(em, 1U);
            }
            break;
        case 2:
            EmBlangonga_UpdateCurves(em);
            if (EmMotion_PassedFrameScaled(em, 30.0f, 0U) != 0) {
                em->step++;
                Em_SetMotion(em, BLANG_MOT_RAJANG_JUMP_SHOT, 6, 0);
                EmBlangonga_StartCurves(em, 2U);
            }
            break;
        case 3:
            if (EmMotion_PassedFrameScaled(em, 22.0f, 0U) != 0) {
                EmEffectAttack_Create(em, EMFX_RAJANG_SHOT, 1, 12743, 0);
                MonsterFx_CreateOnEmAngled(em, 37, 12743, 0, 1.0f);
            }
            ended = EmBlangonga_UpdateCurves(em) & 0xFF;
            posY = 345.0f + em->floorY;
            if (ended == 0) {
                if (em->posY <= posY) {
                    em->posY = posY;
                    break;
                }
            } else if ((ended != 0) && (em->posY <= posY)) {
                em->step++;
                em->posY = em->floorY;
                em->stance = EM_STANCE_GROUND;
                Em_SetMotion(em, BLANG_MOT_RAJANG_JUMP_SHOT_LAND, 0, 0);
                Player_ClearVelocity(em);
            }
            break;
        case 4:
            if (em->motion[0].playing == 0) {
                em->step++;
                EmBlangonga_EndState(em, 0, 0);
            }
            break;
        }
    }
}

/* BLANG_ATK_RAJANG_STRIKE: one blow with a hit at frame 56. */
void EmBlangonga_RajangStrike(Em *em)
{
    if (EmBlangonga_IsRajang(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, BLANG_MOT_RAJANG_STRIKE, 2, 0);
            break;
        case 1:
            if (EmMotion_PassedFrameScaled(em, 56.0f, 0U) != 0) {
                BlangongaAttack_Create(em, 38);
            }
            if (em->motion[0].playing == 0) {
                em->step++;
                EmBlangonga_EndState(em, 0, 0);
            }
            break;
        }
    }
}

/*
 * BLANG_ATK_RAJANG_CHARGE: runs at the goal with a hit that lasts into the
 * end motion.
 */
void EmBlangonga_RajangCharge(Em *em)
{
    u8 stateVar0;

    if (EmBlangonga_IsRajang(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            em->stateVar0 = 0;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, BLANG_MOT_RAJANG_CHARGE, 2, 0);
            break;
        case 1:
            if (EmMotion_PassedFrameScaled(em, 6.0f, 0U) != 0) {
                stateVar0 = em->stateVar0;
                if (stateVar0 == 0) {
                    em->stateVar0 = stateVar0 + 1;
                    BlangongaAttack_Create(em, 39);
                }
            }
            if (Em_StepToGoal(em, 128, 0) == 1) {
                em->step++;
                Em_SetMotion(em, BLANG_MOT_RAJANG_CHARGE_END, 0, 0);
            }
            break;
        case 2:
            if (em->motion[0].playing == 0) {
                em->step++;
                EmBlangonga_EndState(em, 0, 0);
            }
            break;
        }
    }
}

/*
 * BLANG_ATK_CONGALALA_CHARGE (the Congalala kinds): runs at the goal with a
 * hit along (the small kind's weaker), then ends the state.
 */
void EmBlangonga_CongalalaCharge(Em *em)
{
    if (EmBlangonga_IsCongalala(em) == 0) {
        EmBlangonga_EndState(em, 0, 0);
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, BLANG_MOT_CHARGE, 0, 0);
            if (EmBlangonga_IsSmallKind(em) == 1) {
                BlangongaAttack_Create(em, 35);
                break;
            }
            BlangongaAttack_Create(em, 29);
            break;
        case 1:
            if (Em_StepToGoal(em, 64, 0) == 1) {
                em->step++;
                EmBlangonga_EndState(em, 0, 0);
            }
            break;
        }
    }
}

/*
 * Runs the current EM_GROUP_ATTACK state, passing the shared functions
 * their variant.
 */
void EmBlangonga_RunAttackState(Em *em, EmWork *work)
{
    switch (em->state) {
    case BLANG_ATK_TRIPLE_HIT:
        EmBlangonga_TripleHit(em);
        break;
    case BLANG_ATK_SLAM:
        EmBlangonga_Slam(em, work, 0);
        break;
    case BLANG_ATK_STRIKE_A:
        EmBlangonga_Strike(em, work, 0);
        break;
    case BLANG_ATK_STRIKE_B:
        EmBlangonga_Strike(em, work, 1);
        break;
    case BLANG_ATK_CONGALALA_EMISSION:
        EmBlangonga_CongalalaEmission(em);
        break;
    case BLANG_ATK_BLANGO_THROW:
        EmBlangonga_BlangoThrow(em);
        break;
    case BLANG_ATK_CHARGE:
        EmBlangonga_Charge(em);
        break;
    case BLANG_ATK_DIG_THROW_A:
        EmBlangonga_DigThrow(em, work, 0);
        break;
    case BLANG_ATK_CONGALALA_EATEN_EMISSION:
        EmBlangonga_CongalalaEatenEmission(em, work);
        break;
    case BLANG_ATK_CONGALALA_COMBO:
        EmBlangonga_CongalalaCombo(em, work);
        break;
    case BLANG_ATK_CONGALALA_HITZONE_COMBO:
        EmBlangonga_CongalalaHitzoneCombo(em);
        break;
    case BLANG_ATK_CONGALALA_THROW_A:
        EmBlangonga_CongalalaThrow(em, work, 0);
        break;
    case BLANG_ATK_BACK_STRIKE:
        EmBlangonga_BackStrike(em);
        break;
    case BLANG_ATK_SLAM_HEAVY:
        EmBlangonga_Slam(em, work, 1);
        break;
    case BLANG_ATK_EFFECT_STRIKE:
        EmBlangonga_EffectStrike(em, work);
        break;
    case BLANG_ATK_DIG_THROW_B:
        EmBlangonga_DigThrow(em, work, 1);
        break;
    case BLANG_ATK_DIG_THROW_C:
        EmBlangonga_DigThrow(em, work, 2);
        break;
    case BLANG_ATK_RAJANG_COMBO_SLAM_A:
        EmBlangonga_RajangComboSlam(em, work, 0);
        break;
    case BLANG_ATK_RAJANG_SHOT_A:
        EmBlangonga_RajangShot(em, work, 0);
        break;
    case BLANG_ATK_RAJANG_BEAM_A:
        EmBlangonga_RajangBeam(em, work, 0);
        break;
    case BLANG_ATK_RAJANG_JUMP_SHOT:
        EmBlangonga_RajangJumpShot(em, work, 0);
        break;
    case BLANG_ATK_RAJANG_STRIKE:
        EmBlangonga_RajangStrike(em);
        break;
    case BLANG_ATK_RAJANG_CHARGE:
        EmBlangonga_RajangCharge(em);
        break;
    case BLANG_ATK_CONGALALA_CHARGE:
        EmBlangonga_CongalalaCharge(em);
        break;
    case BLANG_ATK_RAJANG_BEAM_B:
        EmBlangonga_RajangBeam(em, work, 1);
        break;
    case BLANG_ATK_CONGALALA_THROW_B:
        EmBlangonga_CongalalaThrow(em, work, 1);
        break;
    case BLANG_ATK_RAJANG_COMBO_SLAM_B:
        EmBlangonga_RajangComboSlam(em, work, 1);
        break;
    case BLANG_ATK_RAJANG_SHOT_B:
        EmBlangonga_RajangShot(em, work, 1);
        break;
    }
}

/* BLANG_REACT_FLINCH: flinches (BLANG_MOT_FLINCH) and ends the state. */
void EmBlangonga_Flinch(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_FLINCH, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_REACT_FLINCH_B and _PART_BREAK: flinches (BLANG_MOT_FLINCH_B) and
 * ends the state.
 */
void EmBlangonga_FlinchB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_FLINCH_B, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_REACT_KNOCK_DOWN (variant 0), _KNOCK_DOWN_B (1), _STUN (2) and
 * _STUN_B (3): is knocked down (the odd variants from the other side), lies
 * and gets up. The knock-downs lie 90 frames; the stuns add the stun's
 * stage object, lie until the status-5 timer runs out and then shake it off
 * (BLANG_IDLE_STUN_SHAKE_OFF).
 */
void EmBlangonga_KnockDown(Em *em, EmWork *work, u8 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        if (!(variant & 1)) {
            Em_SetMotion(em, BLANG_MOT_KNOCK_DOWN, 0, 0);
        } else {
            Em_SetMotion(em, BLANG_MOT_KNOCK_DOWN_B, 0, 0);
        }
        switch (variant) {
        case 2:
        case 3:
            StunStars_CreateEm(em, 1, 17, (s32) ((f32) em->timer + (em->motion[0].frameCount / 2.0f)), 100.0f, 2.0f);
            break;
        }
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            if (!(variant & 1)) {
                Em_SetMotion(em, BLANG_MOT_DOWN, 0, 0);
            } else {
                Em_SetMotion(em, BLANG_MOT_DOWN_B, 0, 0);
            }
            switch (variant) {
            case 0:
            default:
            case 1:
                em->timer = 90;
                break;
            case 2:
            case 3:
                break;
            }
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            if (!(variant & 1)) {
                Em_SetMotion(em, BLANG_MOT_DOWN_GET_UP, 0, 0);
            } else {
                Em_SetMotion(em, BLANG_MOT_DOWN_GET_UP_B, 0, 0);
            }
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            switch (variant) {
            case 0:
            default:
            case 1:
                EmBlangonga_EndState(em, 0, 0);
                break;
            case 2:
            case 3:
                em->step++;
                EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_STUN_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
                break;
            }
        }
        break;
    case 4:
        if (em->remote == 0) {
            EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_STUN_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * BLANG_REACT_STATUS4: hit-react mode 4 with the periodic effect at bone 2
 * until the status-4 timer runs out, then BLANG_IDLE_STATUS4_SHAKE_OFF.
 */
void EmBlangonga_Status4(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_STATUS);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_STATUS4, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_STATUS4_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_STATUS4_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
    Em_SpawnStatus4Effect(em, 2);
}

/*
 * BLANG_REACT_TRAPPED: caught in a trap (hit-react mode 3, holdTimer 50)
 * while BLANG_MOT_TRAPPED plays, then BLANG_REACT_TRAPPED_STRUGGLE.
 */
void EmBlangonga_Trapped(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_NO_FLINCH);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, BLANG_MOT_TRAPPED, 0, 0);
        em->noReact = 1;
        em->holdTimer = 50;
        break;
    case 1:
        if ((s8) em->holdTimer < 5) {
            em->holdTimer = 5;
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_TRAPPED_STRUGGLE, EM_SYNC_OWNER);
        }
        break;
    case 2:
        if ((s8) em->holdTimer < 5) {
            em->holdTimer = 5;
        }
        if (em->remote == 0) {
            EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_TRAPPED_STRUGGLE, EM_SYNC_OWNER);
        }
        break;
    }
}

/*
 * BLANG_REACT_TRAPPED_STRUGGLE, _STATUS4_DOWNED_END and _STUN_DOWNED_END:
 * struggles in the hold (holdTimer kept at 5) until the trap's timer runs
 * out, then BLANG_IDLE_LEAVE_TRAP. An enraged Congalala makes effect 33 of
 * kind 7 in some loops.
 */
void EmBlangonga_TrappedStruggle(Em *em)
{
    u8 stateVar1;

    em->holdTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_STRUGGLE, 0, 0);
        em->noReact = 1;
        em->stateVar1 = 0;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            Em_SetMotion(em, BLANG_MOT_STRUGGLE, 0, 0);
            em->stateVar1++;
        }
        if ((em->type == EM_KIND_CONGALALA) && (em->rage != 0) && (Motion_PassedFrame(em, 0, 104.0f) != 0)) {
            stateVar1 = em->stateVar1;
            if ((stateVar1 & 7) == (((s32) em->stateRand >> (stateVar1 & 3)) & 7)) {
                EmEffectAttack_Create(em, EMFX_CONGALALA_RAGE_GAS, 0, 0, 0);
            }
        }
        if (--em->trapTimer <= 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_STRUGGLE, 0, 0);
            EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_LEAVE_TRAP, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_LEAVE_TRAP, EM_SYNC_OWNER_FLAGGED);
        }
        if (em->motion[0].playing == 0) {
            Em_SetMotion(em, BLANG_MOT_STRUGGLE, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_REACT_FALL: falls out of the air on curve 0 and lands within 180
 * (scaled) of the floor, lies 60 frames, gets up and ends the state.
 */
void EmBlangonga_Fall(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, BLANG_MOT_FALL, 0, 0);
        EmBlangonga_StartCurves(em, 0U);
        break;
    case 1:
        EmBlangonga_UpdateCurves(em);
        if (em->posY <= (em->floorY + (180.0f * em->scaleX))) {
            em->step++;
            em->posY = em->floorY;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, BLANG_MOT_FALL_LAND, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 60;
            Em_SetMotion(em, BLANG_MOT_DOWN, 0, 0);
        }
        break;
    case 3:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_DOWN_GET_UP, 0, 0);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    }
}

/* BLANG_REACT_NONE: does nothing. */
void EmBlangonga_ReactNop(void)
{
}

/*
 * BLANG_REACT_STATUS4_DOWNED (variant 0) and _STUN_DOWNED (1): lies in the
 * hold until the status timer runs out, then the matching _DOWNED_END
 * state; variant 0 adds the periodic effect, variant 1 the stun's stage
 * object. Blangonga first comes up out of the pit (BLANG_MOT_EMERGE to
 * frame 14).
 */
void EmBlangonga_HeldDowned(Em *em, EmWork *work, u8 variant)
{
    u16 curMotion;

    em->holdTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        if (em->type == EM_KIND_BLANGONGA && (curMotion = em->motionId[0], curMotion != EM_MOTION_ID(0, BLANG_MOT_EMERGE)) && curMotion != EM_MOTION_ID(0, BLANG_MOT_STRUGGLE) && curMotion != EM_MOTION_ID(0, BLANG_MOT_DOWNED)) {
            Em_SetMotion(em, BLANG_MOT_EMERGE, 0, 0);
            em->stateArmor = 1;
            em->noPushTimer = 5;
        } else {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_DOWNED, 0, 0);
        }
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        switch (variant) {
        case 0:
            break;
        case 1:
            StunStars_CreateEm(em, 1, 17, (s32) ((f32) em->timer + (em->motion[0].frameCount / 2.0f)), 100.0f, 2.0f);
            break;
        }
        em->noReact = 1;
        break;
    case 1:
        if (em->motion[0].blend != 0) {
            em->stateArmor = 1;
            em->noPushTimer = 5;
        }
        if (EmMotion_ReachedFrame(em, 0, 14.0f)) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_DOWNED, 0, 0);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetTrapTimeGoal(em);
            switch (variant) {
            case 0:
            default:
                EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_STATUS4_DOWNED_END, EM_SYNC_OWNER);
                break;
            case 1:
                EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_STUN_DOWNED_END, EM_SYNC_OWNER);
                break;
            }
        }
        break;
    case 3:
        if (em->remote == 0) {
            switch (variant) {
            case 0:
            default:
                EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_STATUS4_DOWNED_END, EM_SYNC_OWNER);
                break;
            case 1:
                EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_STUN_DOWNED_END, EM_SYNC_OWNER);
                break;
            }
        }
        break;
    }
    if (variant != 1) {
        Em_SpawnStatus4Effect(em, 2);
    }
}

/*
 * BLANG_REACT_STATUS2_DOWNED_END and _STATUS1_DOWNED_END: blinded briefly,
 * struggles once, then BLANG_REACT_TRAPPED_STRUGGLE.
 */
void EmBlangonga_HeldStatusEnd(Em *em)
{
    em->holdTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, BLANG_MOT_STRUGGLE, 0, 0);
        em->noReact = 1;
        em->blind = 2;
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetTrapTimeGoal(em);
            em->blind = 0;
            EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_TRAPPED_STRUGGLE, EM_SYNC_OWNER);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_TRAPPED_STRUGGLE, EM_SYNC_OWNER);
        }
        break;
    }
}

/*
 * BLANG_REACT_HOLD2: held by the second hold (Em_SetHold2 every frame, root
 * motion off) until the kind timer runs out, then
 * BLANG_IDLE_HOLD2_SHAKE_OFF.
 */
void EmBlangonga_Hold2(Em *em)
{
    Em_SetHold2(em);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, BLANG_MOT_STATUS4, 0, 0);
        Player_SetFlag((Player *) em, EM_FLAG_NO_ROOT_MOTION);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_HOLD2_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_HOLD2_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * BLANG_REACT_DROP_CARRIED (variant 0) and _DROP_CARRIED_A..C (1-3):
 * Congalala drops its carried item; variants 1-3 leave it as timed mine
 * point variant - 1.
 */
void EmBlangonga_DropCarried(Em *em, EmWork *work, s32 variant)
{
    s32 k;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_DROP_CARRIED, 0, 0);
        Em_RestartScript(em);
        k = variant & 0xFF;
        if (k > 0) {
            EmMine_CreateTimed(em, &em->posX, k - 1);
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_REACT_FLINCH_ATTACK (Congalala): flinches and at frame 94 turns it
 * into the hitzone combo's two blows.
 */
void EmBlangonga_FlinchAttack(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_FLINCH_B, 0, 50);
        Em_RestartScript(em);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 94.0f, 0U) != 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_CONGALALA_HITZONE_COMBO, 12, 20);
        }
        break;
    case 2:
        if (EmMotion_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            BlangongaAttack_Create(em, 32);
            em->hitzoneSet = 1;
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_CONGALALA_HITZONE_COMBO_END, 0, 0);
            em->hitzoneSet = 0;
        }
        break;
    case 3:
        if (EmMotion_PassedFrameScaled(em, 88.0f, 0U) != 0) {
            BlangongaAttack_Create(em, 41);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_REACT_TRAP_ESCAPE: Rajang leaps out of the trap
 * (EmBlangonga_RajangJumpShot, variant 1); the other kinds just end the
 * state.
 */
void EmBlangonga_TrapEscape(Em *em, EmWork *work)
{
    EmBlangonga_RajangJumpShot(em, work, 1);
}

/*
 * Runs the current EM_GROUP_REACT state, passing the shared functions their
 * variant.
 */
void EmBlangonga_RunReactState(Em *em, EmWork *work)
{
    switch (em->state) {
    case BLANG_REACT_FLINCH:
        EmBlangonga_Flinch(em);
        break;
    case BLANG_REACT_FLINCH_B:
        EmBlangonga_FlinchB(em);
        break;
    case BLANG_REACT_KNOCK_DOWN:
        EmBlangonga_KnockDown(em, work, 0);
        break;
    case BLANG_REACT_KNOCK_DOWN_B:
        EmBlangonga_KnockDown(em, work, 1);
        break;
    case BLANG_REACT_STATUS4:
        EmBlangonga_Status4(em);
        break;
    case BLANG_REACT_TRAPPED:
        EmBlangonga_Trapped(em);
        break;
    case BLANG_REACT_TRAPPED_STRUGGLE:
        EmBlangonga_TrappedStruggle(em);
        break;
    case BLANG_REACT_FALL:
        EmBlangonga_Fall(em);
        break;
    case BLANG_REACT_NONE:
        EmBlangonga_ReactNop();
        break;
    case BLANG_REACT_STATUS4_DOWNED:
        EmBlangonga_HeldDowned(em, work, 0);
        break;
    case BLANG_REACT_STATUS4_DOWNED_END:
        EmBlangonga_TrappedStruggle(em);
        break;
    case BLANG_REACT_STATUS2_DOWNED_END:
        EmBlangonga_HeldStatusEnd(em);
        break;
    case BLANG_REACT_STUN:
        EmBlangonga_KnockDown(em, work, 2);
        break;
    case BLANG_REACT_STUN_B:
        EmBlangonga_KnockDown(em, work, 3);
        break;
    case BLANG_REACT_STUN_DOWNED:
        EmBlangonga_HeldDowned(em, work, 1);
        break;
    case BLANG_REACT_STUN_DOWNED_END:
        EmBlangonga_TrappedStruggle(em);
        break;
    case BLANG_REACT_HOLD2:
        EmBlangonga_Hold2(em);
        break;
    case BLANG_REACT_DROP_CARRIED:
        EmBlangonga_DropCarried(em, work, 0);
        break;
    case BLANG_REACT_FLINCH_ATTACK:
        EmBlangonga_FlinchAttack(em);
        break;
    case BLANG_REACT_STATUS1_DOWNED_END:
        EmBlangonga_HeldStatusEnd(em);
        break;
    case BLANG_REACT_PART_BREAK:
        EmBlangonga_FlinchB(em);
        break;
    case BLANG_REACT_DROP_CARRIED_A:
        EmBlangonga_DropCarried(em, work, 1);
        break;
    case BLANG_REACT_DROP_CARRIED_B:
        EmBlangonga_DropCarried(em, work, 2);
        break;
    case BLANG_REACT_DROP_CARRIED_C:
        EmBlangonga_DropCarried(em, work, 3);
        break;
    case BLANG_REACT_TRAP_ESCAPE:
        EmBlangonga_TrapEscape(em, work);
        break;
    }
}

/*
 * BLANG_DEFEAT_DIE: hunted (QuestEm_Hunted, the rage ended): falls dead and
 * leaves a carcass mine point (the large kinds for 1800 frames once the
 * quest lets them appear again, the small ones for 1350 frames or until
 * carved), fades out over 60 frames, then BLANG_DEFEAT_REAPPEAR.
 */
void EmBlangonga_Die(Em *em, s32 unused)
{
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        if (EmBlangonga_IsSmallKind(em) == 1) {
            em->step = 2;
        } else {
            em->step++;
        }
        QuestEm_Hunted(em);
        Em_EndRage(em);
        Em_SetMotion(em, BLANG_MOT_DIE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_DIE_END, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->fieldCollide = 0;
            if (EmBlangonga_IsSmallKind(em) == 1) {
                em->timer = 1350;
            } else {
                em->timer = 1800;
            }
            EmMine_Create(em, 0);
        }
        break;
    case 3:
        EmMine_Update(em);
        if (EmBlangonga_IsSmallKind(em) == 1) {
            if (EmMine_Update(em) <= 0 || --em->timer <= 0) {
                em->step++;
                EmMine_Remove(em);
            }
        } else if (QuestEm_CanAppear(em) && --em->timer <= 0) {
            em->step++;
            em->timer = 150;
            EmMine_Remove(em);
        }
        break;
    case 4:
        if ((em->alpha -= 0.016666668f) <= 0.0f) {
            em->visible = 0;
            em->alpha = 0.0f;
            em->step++;
        }
        break;
    case 5:
        if (em->remote == 0) {
            em->visible = 0;
            EmBlangonga_ChangeState(em, EM_GROUP_DEFEAT, BLANG_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * BLANG_DEFEAT_DIE_HELD: as EmBlangonga_Die, lying in the hold (holdTimer
 * kept at 5).
 */
void EmBlangonga_DieHeld(Em *em, s32 unused)
{
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, BLANG_MOT_DOWNED, 0, 0);
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_DEAD;
        QuestEm_Hunted(em);
        Em_EndRage(em);
        em->holdTimer = 5;
        break;
    case 1:
        em->holdTimer = 5;
        if (em->motion[0].playing == 0) {
            em->step++;
            EmMine_Create(em, 0);
            em->fieldCollide = 0;
            if (EmBlangonga_IsSmallKind(em) == 1) {
                em->timer = 1350;
            } else {
                em->timer = 1800;
            }
        }
        break;
    case 2:
        EmMine_Update(em);
        em->holdTimer = 5;
        if ((EmBlangonga_IsSmallKind(em) || QuestEm_CanAppear(em)) && --em->timer <= 0) {
            em->step++;
            EmMine_Remove(em);
        }
        break;
    case 3:
        if ((em->alpha -= 0.016666668f) <= 0.0f) {
            em->visible = 0;
            em->alpha = 0.0f;
            em->step++;
        }
        break;
    case 4:
        if (em->remote == 0) {
            em->visible = 0;
            EmBlangonga_ChangeState(em, EM_GROUP_DEFEAT, BLANG_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * BLANG_DEFEAT_DIE_ARMORED: as EmBlangonga_Die for a monster under
 * stateArmor: comes up (BLANG_MOT_EMERGE to frame 14) and lies down.
 */
void EmBlangonga_DieArmored(Em *em, s32 unused)
{
    f32 alpha;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->stateArmor = 1;
        em->noPushTimer = 5;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, BLANG_MOT_EMERGE, 0, 0);
        QuestEm_Hunted(em);
        Em_EndRage(em);
        em->noReact = 1;
        break;
    case 1:
        if (em->motion[0].blend != 0) {
            em->stateArmor = 1;
            em->noPushTimer = 5;
        }
        if (EmMotion_PassedFrameScaled(em, 14.0f, 0U) != 0) {
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, BLANG_MOT_DOWNED, 0, 0);
            em->step++;
            break;
        }
    default:
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmMine_Create(em, 0);
            em->fieldCollide = 0;
            if (EmBlangonga_IsSmallKind(em) == 1) {
                em->timer = 1350;
                break;
            }
            em->timer = 1800;
        }
        break;
    case 3:
        EmMine_Update(em);
        if ((EmBlangonga_IsSmallKind(em) != 0) || (QuestEm_CanAppear(em) != 0)) {
            if (--em->timer <= 0) {
                em->step++;
                EmMine_Remove(em);
                break;
            }
        }
        break;
    case 4:
        alpha = em->alpha - 0.016666668f;
        em->alpha = alpha;
        if (alpha <= 0.0f) {
            em->visible = 0;
            em->alpha = 0.0f;
            em->step++;
        }
        break;
    case 5:
        if (em->remote == 0) {
            em->visible = 0;
            EmBlangonga_ChangeState(em, EM_GROUP_DEFEAT, BLANG_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * BLANG_DEFEAT_DIE_FALLING: as EmBlangonga_Die for a monster in the air:
 * falls on curve 0 and lands dead within 180 of the floor.
 */
void EmBlangonga_DieFalling(Em *em, s32 unused)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, BLANG_MOT_FALL, 0, 0);
        QuestEm_Hunted(em);
        Em_EndRage(em);
        EmBlangonga_StartCurves(em, 0);
        break;
    case 1:
        EmBlangonga_UpdateCurves(em);
        if (em->posY <= 180.0f + em->floorY) {
            em->step++;
            em->posY = em->floorY;
            em->stance = EM_STANCE_DEAD;
            Em_SetMotion(em, BLANG_MOT_FALL_LAND, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->fieldCollide = 0;
            if (EmBlangonga_IsSmallKind(em) == 1) {
                em->timer = 1350;
            } else {
                em->timer = 1800;
            }
            EmMine_Create(em, 0);
        }
        break;
    case 3:
        EmMine_Update(em);
        if ((EmBlangonga_IsSmallKind(em) || QuestEm_CanAppear(em)) && --em->timer <= 0) {
            em->step++;
            EmMine_Remove(em);
        }
        break;
    case 4:
        if ((em->alpha -= 0.016666668f) <= 0.0f) {
            em->visible = 0;
            em->alpha = 0.0f;
            em->step++;
        }
        break;
    case 5:
        if (em->remote == 0) {
            em->visible = 0;
            EmBlangonga_ChangeState(em, EM_GROUP_DEFEAT, BLANG_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * BLANG_DEFEAT_REAPPEAR: when the quest lets the monster appear again, a
 * Blango of spawnParam 1 hides under the snow again, the other small kinds
 * go to BLANG_IDLE and the large ones drop in from above
 * (BLANG_LEAP_DROP_IN); otherwise the monster's task is finished
 * (taskPhase). EmBlangonga_Start tests Em.variant for the same hidden
 * start: QuestEm_Spawn copies the two from different fields of the quest's
 * monster entry, so the two tests are the game's own.
 */
void EmBlangonga_Reappear(Em *em, s32 unused)
{
    switch (em->step) {
    case 0:
        if (QuestEm_CanAppear(em) == 1) {
            if (EmBlangonga_IsSmallKind(em) == 1) {
                if ((em->type == EM_KIND_BLANGO) && (em->spawnParam == 1)) {
                    em->stance = EM_STANCE_GROUND;
                    Em_SetMotionRow(em, BLANG_MOT_BURIED, 0, 0);
                    EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_BURIED, EM_SYNC_LOCAL);
                    break;
                }
                EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE, EM_SYNC_LOCAL);
                break;
            }
            em->posY = gAreaGroundY[em->area];
            em->stance = EM_STANCE_AIR;
            EmBlangonga_ChangeState(em, EM_GROUP_FLY, BLANG_LEAP_DROP_IN, EM_SYNC_LOCAL);
            break;
        }
        em->taskPhase++;
        break;
    }
}

/*
 * Runs the current EM_GROUP_DEFEAT state, unpushable (and the small kinds
 * unhittable) meanwhile.
 */
void EmBlangonga_RunDefeatState(Em *em, s32 work)
{
    if (EmBlangonga_IsSmallKind(em) == 1) {
        em->noHitTimer = 10;
    }
    em->noPushTimer = 10;
    switch (em->state) {
    case BLANG_DEFEAT_DIE:
        EmBlangonga_Die(em, work);
        break;
    case BLANG_DEFEAT_DIE_HELD:
        EmBlangonga_DieHeld(em, work);
        break;
    case BLANG_DEFEAT_DIE_ARMORED:
        EmBlangonga_DieArmored(em, work);
        break;
    case BLANG_DEFEAT_DIE_FALLING:
        EmBlangonga_DieFalling(em, work);
        break;
    case BLANG_DEFEAT_REAPPEAR:
        EmBlangonga_Reappear(em, work);
        break;
    }
}

/*
 * BLANG_EVENT_CAPTURED: captured in holdTimer's hold (Em_SetCaptured 0,
 * status 1, poison ended): lies downed (Blangonga first comes up out of the
 * pit), QuestEm_Captured, a sound every 135 frames and the periodic effect;
 * 1800 frames once the quest lets it appear again, then a 150-frame fade
 * and BLANG_DEFEAT_REAPPEAR.
 */
void EmBlangonga_Captured(Em *em)
{
    s32 timer;
    s32 prevTimer;
    u16 motion;

    Em_SetCaptured(em, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        if ((em->type == EM_KIND_BLANGONGA) && (motion = em->motionId[0], (motion != EM_MOTION_ID(0, BLANG_MOT_EMERGE))) && (motion != EM_MOTION_ID(0, BLANG_MOT_STRUGGLE)) && (motion != EM_MOTION_ID(0, BLANG_MOT_DOWNED))) {
            Em_SetMotion(em, BLANG_MOT_EMERGE, 0, 0);
            em->stateArmor = 1;
            em->noPushTimer = 5;
        } else {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_DOWNED, 0, 0);
            QuestEm_Captured(em);
            em->timer = 1800;
        }
        em->lookAtTarget = 0;
        Em_BeginStatus1(em);
        Em_ClearPoison(em);
        break;
    case 1:
        if (em->motion[0].blend != 0) {
            em->stateArmor = 1;
            em->noPushTimer = 5;
        }
        if (EmMotion_ReachedFrame(em, 0, 14.0f) != 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_DOWNED, 0, 0);
            QuestEm_Captured(em);
            em->timer = 1800;
        }
        break;
    case 2:
        if (((s32) em->timer % 135) == 0) {
            Em_PlaySeAtBone(em, 82, BLANG_BONE_HEAD);
        }
        if (em->type == EM_KIND_CONGALALA) {
            EmBlangonga_SpawnLyingPuffs(em, 1);
        } else {
            EmBlangonga_SpawnLyingPuffs(em, 0);
        }
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
                EmBlangonga_ChangeState(em, EM_GROUP_DEFEAT, BLANG_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
                break;
            }
        } else {
            em->alpha = (f32) timer / 150.0f;
        }
        break;
    }
}

/*
 * BLANG_EVENT_CAPTURED_HOLD2: as EmBlangonga_Captured in the second hold
 * (Em_SetCaptured 1), collapsing and lying instead, without the sound.
 */
void EmBlangonga_CapturedHold2(Em *em)
{
    s32 timer;
    s32 prevTimer;

    Em_SetCaptured(em, 1);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_COLLAPSE, 0, 0);
        Em_BeginStatus1(em);
        Em_ClearPoison(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_LIE, 0, 0);
            QuestEm_Captured(em);
            em->timer = 1800;
        }
        break;
    case 2:
        EmBlangonga_SpawnLyingPuffs(em, 0);
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
                EmBlangonga_ChangeState(em, EM_GROUP_DEFEAT, BLANG_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
                break;
            }
        } else {
            em->alpha = (f32) timer / 150.0f;
        }
        break;
    }
}

/*
 * BLANG_EVENT_CONGALALA_SCENE: Congalala's story scene at a fixed point:
 * idles 180 frames, eats twice and roars, then idles until EVENT_FLAG_CONGALALA_SCENE
 * is set; senseDelay and noHitTimer are kept up meanwhile.
 */
void EmBlangonga_CongalalaScene(Em *em, EmWork *work)
{
    EmBlangongaWork *w = (EmBlangongaWork *)work;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        GmEm_ResetSize(em);
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->posX = 12212.0f;
        em->posY = 0.0f;
        em->posZ = 9484.0f;
        em->rotX = 0;
        em->rotY = ANGLE_TURN / 4;
        em->rotZ = 0;
        Em_SetMotion(em, BLANG_MOT_IDLE, 0, 0);
        em->timer = 180;
        w->sceneEating = 0;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_EAT, 0, 0);
        }
        break;
    case 2:
        if (EmMotion_InFrameRange(em, 0, 4.0f, 26.0f)) {
            em->rotY += (u32) (1638.4f * em->motionSpeed);
        }
        if (w->sceneEating == 0 && EmMotion_ReachedFrame(em, 0, 34.0f)) {
            w->sceneEating = 1;
        }
        if (EmMotion_PassedFrameScaled(em, 300.0f, 0)) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_EAT, 0, 150);
        }
        break;
    case 3:
        if (EmMotion_PassedFrameScaled(em, 250.0f, 0)) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_BACK_STRIKE, 0, 0);
            w->sceneEating = 0;
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_ROAR, 0, 0);
        }
        break;
    case 5:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_IDLE, 0, 0);
        }
        break;
    case 6:
        if (Save_TestEventFlag(EVENT_FLAG_CONGALALA_SCENE) == 1) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_EVENT_BLANGONGA_SCENE: hands each monster its part of Blangonga's
 * story scene: Blangonga leaps down (BLANG_EVENT_BLANGONGA_LEAP_DOWN), the
 * first three Blangos in gEms emerge (_EMERGE_A..C) and the rest stay
 * hidden.
 */
void EmBlangonga_StartBlangongaScene(Em *em, EmWork *work)
{
    s16 i;
    u8 n = 0;
    Em *other;

    if (em->type == EM_KIND_BLANGONGA) {
        EmBlangonga_ChangeState(em, EM_GROUP_EVENT, BLANG_EVENT_BLANGONGA_LEAP_DOWN, EM_SYNC_LOCAL);
        return;
    }
    for (i = 0, other = gEms; i < 20; i++, other++) {
        if (other->type == EM_KIND_BLANGO) {
            if (em->slotNo == other->slotNo) {
                switch (n) {
                case 0:
                    EmBlangonga_ChangeState(em, EM_GROUP_EVENT, BLANG_EVENT_BLANGO_EMERGE_A, EM_SYNC_LOCAL);
                    return;
                case 1:
                    EmBlangonga_ChangeState(em, EM_GROUP_EVENT, BLANG_EVENT_BLANGO_EMERGE_B, EM_SYNC_LOCAL);
                    return;
                case 2:
                    EmBlangonga_ChangeState(em, EM_GROUP_EVENT, BLANG_EVENT_BLANGO_EMERGE_C, EM_SYNC_LOCAL);
                    return;
                default:
                    EmBlangonga_ChangeState(em, EM_GROUP_EVENT, BLANG_EVENT_BLANGO_HIDDEN, EM_SYNC_LOCAL);
                    return;
                }
            }
            n++;
        }
    }
}

/*
 * BLANG_EVENT_BLANGONGA_LEAP_DOWN: waits on a high point, roars, jumps down
 * and lands on the floor (frame 70 of the fall), walks to its goal (1200
 * frames or within 100), then idles until EVENT_FLAG_BLANGONGA_SCENE.
 */
void EmBlangonga_BlangongaLeapDown(Em *em, EmWork *work)
{
    f32 goalDist;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        GmEm_ResetSize(em);
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->posX = 10747.0f;
        em->posY = 2468.0f;
        em->posZ = 8181.0f;
        Vec_Copy(&em->prevPosX, &em->posX);
        em->rotX = 0;
        em->rotY = 0xFC00;
        em->rotZ = 0;
        Em_SetMotion(em, BLANG_MOT_IDLE, 0, 0);
        em->timer = 300;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_ROAR_B, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 190;
            Em_SetMotion(em, BLANG_MOT_IDLE, 0, 0);
        }
        break;
    case 3:
        if (--em->timer <= 0) {
            em->step++;
            em->stance = EM_STANCE_AIR;
            Em_SetMotion(em, BLANG_MOT_MOVE_ACTION_D, 0, 0);
        }
        break;
    case 4:
        if (EmMotion_PassedFrameScaled(em, 26.0f, 0)) {
            em->step++;
            Player_ClearVelocity(em);
            Em_SetMotion(em, BLANG_MOT_JUMP_FALL, 0, 50);
        }
        break;
    case 5:
        if (EmMotion_PassedFrameScaled(em, 70.0f, 0)) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posX = 10682.0f;
            em->posY = 288.0f;
            em->posZ = 9620.0f;
            em->goal[0] = 10682.0f;
            em->goal[1] = 288.0f;
            em->goal[2] = 12400.0f;
            em->rotX = 0;
            em->rotY = Math_AngleXZ(&em->posX, em->goal);
            em->rotZ = 0;
            Em_SetMotion(em, BLANG_MOT_JUMP_LAND, 0, 0);
        }
        break;
    case 6:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 1200;
            Em_SetMotion(em, BLANG_MOT_WALK, 0, 0);
        }
        break;
    case 7:
        goalDist = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer <= 0 || goalDist <= 100.0f) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_IDLE, 0, 0);
        }
        break;
    case 8:
        if (Save_TestEventFlag(EVENT_FLAG_BLANGONGA_SCENE) == 1) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_EVENT_BLANGO_EMERGE_A..C (variants 0-2): waits buried (stateArmor)
 * at the variant's place and facing for 440, 480 or 510 frames, comes out,
 * then idles and roars in turn until EVENT_FLAG_BLANGONGA_SCENE.
 */
void EmBlangonga_BlangoEmerge(Em *em, EmWork *work, u8 variant)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        GmEm_ResetSize(em);
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->stateArmor = 1;
        Em_SetMotion(em, BLANG_MOT_BURIED, 0, 0);
        switch (variant) {
        default:
            em->posX = 11000.0f;
            em->posY = 288.0f;
            em->posZ = 11500.0f;
            em->rotX = 0;
            em->rotY = ANGLE_TURN / 16;
            em->rotZ = 0;
            em->timer = 440;
            break;
        case 1:
            em->posX = 10000.0f;
            em->posY = 288.0f;
            em->posZ = 10380.0f;
            em->rotX = 0;
            em->rotY = 0x1400;
            em->rotZ = 0;
            em->timer = 480;
            break;
        case 2:
            em->posX = 9900.0f;
            em->posY = 288.0f;
            em->posZ = 11819.0f;
            em->rotX = 0;
            em->rotY = ANGLE_TURN / 8;
            em->rotZ = 0;
            em->timer = 510;
            break;
        }
        break;
    case 1:
        em->stateArmor = 1;
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_EMERGE, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_ALERT_IDLE, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stateVar1 = 0;
            Em_SetMotion(em, BLANG_MOT_ROAR, 0, 0);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            if (em->stateVar1 == 0) {
                em->stateVar1 = 1;
                Em_SetMotion(em, BLANG_MOT_ALERT_IDLE, 0, 0);
            } else {
                em->stateVar1 = 0;
                Em_SetMotion(em, BLANG_MOT_ROAR, 0, 0);
            }
        }
        if (Save_TestEventFlag(EVENT_FLAG_BLANGONGA_SCENE) == 1) {
            em->step++;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * BLANG_EVENT_BLANGO_HIDDEN: stays invisible until EVENT_FLAG_BLANGONGA_SCENE, then
 * shows itself and ends the state.
 */
void EmBlangonga_BlangoHidden(Em *em)
{
    switch (em->step) {
    case 0:
        GmEm_ResetSize(em);
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BLANG_MOT_IDLE, 0, 0);
        em->visible = 0;
        em->scriptPending = 0;
        break;
    case 1:
        if (Save_TestEventFlag(EVENT_FLAG_BLANGONGA_SCENE) == 1) {
            em->step++;
            em->visible = 1;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/* Runs the current EM_GROUP_EVENT (or EVENT2) state. */
void EmBlangonga_RunEventState(Em *em, EmWork *work)
{
    switch (em->state) {
    case BLANG_EVENT_CAPTURED:
        EmBlangonga_Captured(em);
        break;
    case BLANG_EVENT_CAPTURED_HOLD2:
        EmBlangonga_CapturedHold2(em);
        break;
    case BLANG_EVENT_CONGALALA_SCENE:
        EmBlangonga_CongalalaScene(em, work);
        break;
    case BLANG_EVENT_BLANGONGA_SCENE:
        EmBlangonga_StartBlangongaScene(em, work);
        break;
    case BLANG_EVENT_BLANGONGA_LEAP_DOWN:
        EmBlangonga_BlangongaLeapDown(em, work);
        break;
    case BLANG_EVENT_BLANGO_EMERGE_A:
        EmBlangonga_BlangoEmerge(em, work, 0);
        break;
    case BLANG_EVENT_BLANGO_EMERGE_B:
        EmBlangonga_BlangoEmerge(em, work, 1);
        break;
    case BLANG_EVENT_BLANGO_EMERGE_C:
        EmBlangonga_BlangoEmerge(em, work, 2);
        break;
    case BLANG_EVENT_BLANGO_HIDDEN:
        EmBlangonga_BlangoHidden(em);
        break;
    }
}

/*
 * BLANG_VS_EAT_BAIT: after the defeat camera (Versus_DefeatCamera) the bait
 * comes from the winner's side (stage object 37) and the monster eats it
 * (Congalala with its own motion), then Versus_RouteEmByTeam.
 */
void EmBlangonga_VersusEatBait(Em *em, EmWork *work)
{
    Vec3f throwPoint;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        if (Versus_DefeatCamera(em) == 1) {
            throwPoint = EmBlangonga_CarveThrowPt;
            VersusBait_Create(&gPlayers[(u8) em->side - 1], em, 0, 0, &throwPoint.x);
            em->step++;
            if (em->type == EM_KIND_CONGALALA) {
                em->step++;
                Em_SetMotion(em, BLANG_MOT_CONGALALA_EAT, 0, 0);
            } else {
                Em_SetMotion(em, BLANG_MOT_EAT, 0, 0);
            }
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, BLANG_MOT_EAT_END, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            Versus_RouteEmByTeam(em);
        }
        break;
    }
}

/*
 * BLANG_VS_POSE_A: one pose by kind (Blangonga, Congalala, the others),
 * then idles and BLANG_VS_WAIT_BOTH.
 */
void EmBlangonga_VersusPoseA(Em *em, EmWork *work)
{
    u8 blangongaStep;
    u8 congalalaStep;
    u8 otherStep;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->type) {
    case EM_KIND_BLANGONGA:
        blangongaStep = em->step;
        switch (blangongaStep) {
        case 0:
            em->step = blangongaStep + 1;
            Em_SetMotion(em, BLANG_MOT_IDLE_ACTION_C, 0, 0);
            break;
        case 1:
            if (em->motion[0].playing == 0) {
                em->step = blangongaStep + 1;
                Em_SetMotion(em, BLANG_MOT_IDLE, 0, 0);
                EmBlangonga_ChangeState(em, EM_GROUP_VERSUS, BLANG_VS_WAIT_BOTH, EM_SYNC_SEND);
            }
            break;
        }
        break;
    case 52:
        congalalaStep = em->step;
        switch (congalalaStep) {
        case 0:
            em->step = congalalaStep + 1;
            Em_SetMotion(em, BLANG_MOT_FLINCH_B, 0, 0);
            break;
        case 1:
            if (em->motion[0].playing == 0) {
                em->step = congalalaStep + 1;
                Em_SetMotion(em, BLANG_MOT_IDLE, 0, 0);
                EmBlangonga_ChangeState(em, EM_GROUP_VERSUS, BLANG_VS_WAIT_BOTH, EM_SYNC_SEND);
            }
            break;
        }
        break;
    default:
        otherStep = em->step;
        switch (otherStep) {
        case 0:
            em->step = otherStep + 1;
            Em_SetMotion(em, BLANG_MOT_FLINCH, 0, 0);
            break;
        case 1:
            if (em->motion[0].playing == 0) {
                em->step = otherStep + 1;
                Em_SetMotion(em, BLANG_MOT_IDLE, 0, 0);
                EmBlangonga_ChangeState(em, EM_GROUP_VERSUS, BLANG_VS_WAIT_BOTH, EM_SYNC_SEND);
            }
        }
        break;
    }
}

/* BLANG_VS_POSE_B: as BLANG_VS_POSE_A with the second poses. */
void EmBlangonga_VersusPoseB(Em *em, EmWork *work)
{
    u8 blangongaStep;
    u8 otherStep;
    u8 congalalaStep;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->type) {
    case EM_KIND_BLANGONGA:
        blangongaStep = em->step;
        switch (blangongaStep) {
        case 0:
            em->step = blangongaStep + 1;
            Em_SetMotion(em, BLANG_MOT_ROAR_B, 0, 0);
            break;
        case 1:
            if (em->motion[0].playing == 0) {
                em->step = blangongaStep + 1;
                Em_SetMotion(em, BLANG_MOT_IDLE, 0, 0);
                EmBlangonga_ChangeState(em, EM_GROUP_VERSUS, BLANG_VS_WAIT_BOTH, EM_SYNC_SEND);
            }
            break;
        }
        break;
    case 52:
        congalalaStep = em->step;
        switch (congalalaStep) {
        case 0:
            em->step = congalalaStep + 1;
            Em_SetMotion(em, BLANG_MOT_ROAR, 0, 0);
            break;
        case 1:
            if (em->motion[0].playing == 0) {
                em->step = congalalaStep + 1;
                Em_SetMotion(em, BLANG_MOT_IDLE, 0, 0);
                EmBlangonga_ChangeState(em, EM_GROUP_VERSUS, BLANG_VS_WAIT_BOTH, EM_SYNC_SEND);
            }
            break;
        }
        break;
    default:
        otherStep = em->step;
        switch (otherStep) {
        case 0:
            em->step = otherStep + 1;
            Em_SetMotion(em, BLANG_MOT_ROAR_B, 0, 0);
            break;
        case 1:
            if (em->motion[0].playing == 0) {
                em->step = otherStep + 1;
                Em_SetMotion(em, BLANG_MOT_IDLE, 0, 0);
                EmBlangonga_ChangeState(em, EM_GROUP_VERSUS, BLANG_VS_WAIT_BOTH, EM_SYNC_SEND);
            }
        }
        break;
    }
}

/*
 * BLANG_VS_POSE_C: as BLANG_VS_POSE_A with the third poses (Congalala's in
 * two motions).
 */
void EmBlangonga_VersusPoseC(Em *em, EmWork *work)
{
    u8 congalalaStep;
    u8 blangongaStep;
    u8 otherStep;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->type) {
    case EM_KIND_BLANGONGA:
        blangongaStep = em->step;
        switch (blangongaStep) {
        case 0:
            em->step = blangongaStep + 1;
            Em_SetMotion(em, BLANG_MOT_ROAR, 0, 0);
            break;
        case 1:
            if (em->motion[0].playing == 0) {
                em->step = blangongaStep + 1;
                Em_SetMotion(em, BLANG_MOT_IDLE, 0, 0);
                EmBlangonga_ChangeState(em, EM_GROUP_VERSUS, BLANG_VS_WAIT_BOTH, EM_SYNC_SEND);
            }
            break;
        }
        break;
    case 52:
        congalalaStep = em->step;
        switch (congalalaStep) {
        case 0:
            em->step = congalalaStep + 1;
            Em_SetMotion(em, BLANG_MOT_CONGALALA_HITZONE_COMBO, 0, 0);
            break;
        case 1:
            if (em->motion[0].playing == 0) {
                em->step = congalalaStep + 1;
                Em_SetMotion(em, BLANG_MOT_CONGALALA_HITZONE_COMBO_END, 0, 0);
            }
            break;
        case 2:
            if (em->motion[0].playing == 0) {
                em->step = congalalaStep + 1;
                Em_SetMotion(em, BLANG_MOT_IDLE, 0, 0);
                EmBlangonga_ChangeState(em, EM_GROUP_VERSUS, BLANG_VS_WAIT_BOTH, EM_SYNC_SEND);
            }
            break;
        }
        break;
    default:
        otherStep = em->step;
        switch (otherStep) {
        case 0:
            em->step = otherStep + 1;
            Em_SetMotion(em, BLANG_MOT_ROAR, 0, 0);
            break;
        case 1:
            if (em->motion[0].playing == 0) {
                em->step = otherStep + 1;
                Em_SetMotion(em, BLANG_MOT_IDLE, 0, 0);
                EmBlangonga_ChangeState(em, EM_GROUP_VERSUS, BLANG_VS_WAIT_BOTH, EM_SYNC_SEND);
            }
        }
        break;
    }
}

/*
 * BLANG_VS_WAIT_BOTH: idles until both sides are defeated
 * (Versus_BothDefeated), then BLANG_VS_END. Step 0 falls into step 1 to
 * test on the same frame.
 */
void EmBlangonga_VersusWaitBoth(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        if (em->motionId[0] != EM_MOTION_ID(0, BLANG_MOT_IDLE)) {
            Em_SetMotion(em, BLANG_MOT_IDLE, 0, 0);
        }
        
    case 1:
        if (Versus_BothDefeated() == 1) {
            em->step++;
            EmBlangonga_ChangeState(em, EM_GROUP_VERSUS, BLANG_VS_END, EM_SYNC_SEND);
        }
        break;
    }
}

/*
 * BLANG_VS_END: idles 90 frames, lets the monster be sensed and hit again,
 * and ends the state.
 */
void EmBlangonga_VersusEnd(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 90;
        if (em->motionId[0] != EM_MOTION_ID(0, BLANG_MOT_IDLE)) {
            Em_SetMotion(em, BLANG_MOT_IDLE, 0, 0);
        }
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            em->senseDelay = 0;
            em->noHitTimer = 0;
            EmBlangonga_EndState(em, 0, 0);
        }
        break;
    }
}

/* Runs the current EM_GROUP_VERSUS state. */
void EmBlangonga_RunVersusState(Em *em, EmWork *work)
{
    switch (em->state) {
    case BLANG_VS_EAT_BAIT:
        EmBlangonga_VersusEatBait(em, work);
        break;
    case BLANG_VS_POSE_A:
        EmBlangonga_VersusPoseA(em, work);
        break;
    case BLANG_VS_POSE_B:
        EmBlangonga_VersusPoseB(em, work);
        break;
    case BLANG_VS_POSE_C:
        EmBlangonga_VersusPoseC(em, work);
        break;
    case BLANG_VS_WAIT_BOTH:
        EmBlangonga_VersusWaitBoth(em, work);
        break;
    case BLANG_VS_END:
        EmBlangonga_VersusEnd(em, work);
        break;
    }
}

/*
 * The family's frame: the rage timer, visibility and combat timer; the
 * Blango's sensing under the snow (EM_TRIGGER_SENSED); the needs' decay by
 * map and kind; then Em_UpdateDamage's result picks a reaction
 * (EM_GROUP_REACT), a death or a capture, or a status state; Congalala's
 * and Blangonga's story quests start their scenes; then the state runs
 * (twice when stateRequest is set), and stateArmor keeps the shadow hidden
 * and the guard up.
 */
void EmBlangonga_Update(Em *em)
{
    EmBlangongaWork *w = (EmBlangongaWork *)&em->work;
    u8 savedSenseDelay;
    s16 partStateSum;
    s32 result;
    u8 mapBase;
    s32 sensed;
    s32 i;
    u8 damageOut[4];

    EmBlangonga_UpdateRageTimer(em);
    if (em->stateGroup <= EM_GROUP_REACT) {
        em->alpha = 1.0f;
        em->visible = 1;
    }
    if (em->senseDelay != 0) {
        em->noHitTimer = 5;
    }
    if (w->hitSeCooldown > 0) {
        w->hitSeCooldown--;
    }
    Em_UpdateCombatTimer(em);
    if (EmBlangonga_IsSmallKind(em) == 0) {
        Em_RaisePlayerAreaTrigger(em);
    }
    if ((em->stateGroup != EM_GROUP_FLY || em->state != BLANG_LEAP_OUT_OF_AREA) && (em->stateGroup != EM_GROUP_FLY || em->state != BLANG_LEAP_DROP_IN) &&
        (em->stateGroup != EM_GROUP_FLY || em->state != BLANG_LEAP_CROSS_AREA)) {
        Em_HoldPosNoPlayers(em, 0, 1);
    }
    if (EmBlangonga_ShouldEndState(em) == 1) {
        EmBlangonga_EndState(em, 0, 0);
    }
    if ((s16) Player_IsState(em, EM_GROUP_IDLE, BLANG_IDLE_BURIED)) {
        savedSenseDelay = em->senseDelay;
        em->senseDelay = 0;
    }
    if (em->type == EM_KIND_BLANGO) {
        sensed = Em_SenseObjects(em, &w->sense) & 0xFF;
        if (em->senseDelay == 0 && em->noReact == 0 && sensed != 0 && Em_ScriptFlagsFrom(em, 4) == 0) {
            em->triggers |= EM_TRIGGER_SENSED;
            em->scriptPending = 1;
        } else if ((s16) Player_IsState(em, EM_GROUP_IDLE, BLANG_IDLE_BURIED)) {
            em->senseDelay = savedSenseDelay;
        }
    }
    if (em->noAlert != 1 && em->rage == 0) {
        mapBase = Map_DayId(gScene.map);
        switch (em->type) {
        case EM_KIND_BLANGONGA:
            switch (mapBase) {
            case MAP_SNOWY_MOUNTAINS:
                Em_DecayNeed1Only(em);
                Em_CheckLowHp(em, 0.3f);
                break;
            }
            break;
        case 52:
            switch (mapBase) {
            case MAP_JUNGLE:
                Em_DecayNeed1(em);
                Em_DecayNeed2(em);
                Em_CheckLowHp(em, 0.2f);
                break;
            case MAP_SWAMP:
                Em_DecayNeed1(em);
                Em_CheckLowHp(em, 0.2f);
                break;
            case MAP_GREAT_ARENA:
            case MAP_ARENA:
                Em_DecayNeed1Only(em);
                break;
            }
            break;
        }
    }
    result = Em_UpdateDamage(em, damageOut) & 0xFF;
    switch (result) {
    case EM_DMG_NONE:
    case EM_DMG_FLASH:
    case EM_DMG_HIT_MODE3:
    case EM_DMG_ATTACK5:
    case EM_DMG_ATTACK5_FLINCH:
        break;
    case EM_DMG_KILLED:
    case EM_DMG_DEAD:
        if ((s8) em->holdTimer != 0) {
            EmBlangonga_ChangeState(em, EM_GROUP_DEFEAT, BLANG_DEFEAT_DIE_HELD, EM_SYNC_OWNER);
        } else if (EmBlangonga_InStateArmor(em) == 1) {
            EmBlangonga_ChangeState(em, EM_GROUP_DEFEAT, BLANG_DEFEAT_DIE_ARMORED, EM_SYNC_OWNER);
        } else {
            switch (em->stance) {
            case EM_STANCE_AIR:
                EmBlangonga_ChangeState(em, EM_GROUP_DEFEAT, BLANG_DEFEAT_DIE_FALLING, EM_SYNC_OWNER);
                break;
            default:
                EmBlangonga_ChangeState(em, EM_GROUP_DEFEAT, BLANG_DEFEAT_DIE, EM_SYNC_OWNER);
                break;
            }
        }
        break;
    case EM_DMG_ATTACK9:
    case EM_DMG_ATTACK6:
        if ((s8) em->holdTimer == 0) {
            if (EmBlangonga_IsRajang(em) == 1 && em->rage == 0 && em->status == EM_STATUS_NONE) {
                EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_TRAP_ESCAPE, EM_SYNC_OWNER);
            } else {
                Em_SetTrapTime(em, damageOut);
                Em_SetTrapTimeGoal(em);
                EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_TRAPPED, EM_SYNC_OWNER);
            }
        }
        break;
    case EM_DMG_STATUS_IN_AIR:
        if (em->stateGroup != EM_GROUP_REACT || em->state != BLANG_REACT_FALL) {
            EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_FALL, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_STATUS4:
        if (em->stateGroup != EM_GROUP_REACT || em->state != BLANG_REACT_STATUS4_DOWNED) {
            if ((s8) em->holdTimer != 0) {
                Em_SetStatus4Timer(em);
                EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_STATUS4_DOWNED, EM_SYNC_OWNER);
            } else if ((em->stateGroup != EM_GROUP_REACT || em->state != BLANG_REACT_STATUS4) && (em->stateGroup != EM_GROUP_REACT || em->state != BLANG_REACT_FALL)) {
                Em_SetStatus4Timer(em);
                EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_STATUS4, EM_SYNC_OWNER);
            }
        }
        break;
    case EM_DMG_CAPTURED:
        if (Em_IsHeld(em) == 1) {
            Em_SetStatus1Timer(em);
            if (EmBlangonga_IsBlangonga(em) != 1) {
                EmBlangonga_IsCongalala(em);
            }
            if ((s8) em->holdTimer != 0) {
                EmBlangonga_ChangeState(em, EM_GROUP_EVENT, BLANG_EVENT_CAPTURED, EM_SYNC_OWNER);
                break;
            }
            if ((s8) em->holdTimer2 != 0) {
                EmBlangonga_ChangeState(em, EM_GROUP_EVENT, BLANG_EVENT_CAPTURED_HOLD2, EM_SYNC_OWNER);
                break;
            }
        }
        if ((em->stateGroup != EM_GROUP_IDLE || em->state != BLANG_IDLE_STATUS1_COLLAPSE) && (em->stateGroup != EM_GROUP_REACT || em->state != BLANG_REACT_FALL)) {
            Em_SetStatus1Timer(em);
            EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_STATUS1_COLLAPSE, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_STATUS2:
        if ((s8) em->holdTimer != 0) {
            Em_SetStatus2Timer(em);
            EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_STATUS2_DOWNED, EM_SYNC_OWNER);
        } else if ((em->stateGroup != EM_GROUP_IDLE || em->state != BLANG_IDLE_STATUS2_COLLAPSE) && (em->stateGroup != EM_GROUP_REACT || em->state != BLANG_REACT_FALL)) {
            Em_SetStatus2Timer(em);
            EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_STATUS2_COLLAPSE, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_HIT_LYING:
        Em_RestartScript(em);
        switch (em->state) { /* the lying states that set hitReactMode 2 are all in EM_GROUP_IDLE */
        case BLANG_IDLE_REST:
            EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_REST_END, EM_SYNC_OWNER);
            break;
        case BLANG_IDLE_STATUS2_COLLAPSE:
            EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_STATUS2_END, EM_SYNC_OWNER);
            break;
        case BLANG_IDLE_STATUS1_COLLAPSE:
            EmBlangonga_ChangeState(em, EM_GROUP_IDLE, BLANG_IDLE_STATUS1_END, EM_SYNC_OWNER);
            break;
        case BLANG_IDLE_STATUS2_DOWNED:
            EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_STATUS2_DOWNED_END, EM_SYNC_OWNER);
            break;
        case BLANG_IDLE_STATUS1_DOWNED:
            EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_STATUS1_DOWNED_END, EM_SYNC_OWNER);
            break;
        }
        break;
    case EM_DMG_PART8_DOWN:
        EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_PART_BREAK, EM_SYNC_OWNER);
        break;
    case EM_DMG_PART_FLINCH:
        EmBlangonga_BreakParts(em, 1);
        switch (em->stance) {
        case EM_STANCE_AIR:
            EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_FALL, EM_SYNC_OWNER);
            break;
        default:
            partStateSum = 0;
            if (EmBlangonga_IsCongalala(em) == 1) {
                if (em->type == EM_KIND_CONGALALA) {
                    switch (em->flinchPart) {
                    case 4:
                        if (em->parts[em->flinchPart].state >= 2) {
                            if (EmBlangonga_CarriedShown(em, 1) == 1) {
                                EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_DROP_CARRIED_B, EM_SYNC_OWNER);
                                goto done;
                            }
                            if (EmBlangonga_CarriedShown(em, 2) == 1) {
                                EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_DROP_CARRIED_C, EM_SYNC_OWNER);
                                goto done;
                            }
                            if (EmBlangonga_CarriedShown(em, 0) == 1) {
                                EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_DROP_CARRIED_A, EM_SYNC_OWNER);
                                goto done;
                            }
                        }
                    }
                }
                for (i = 0; i < 8; i++) {
                    partStateSum += em->parts[i].state;
                }
                if (partStateSum <= 3) {
                    EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_FLINCH, EM_SYNC_OWNER);
                    break;
                }
                switch (em->flinchPart) {
                case 0:
                    if (EmBlangonga_IsSmallKind(em) == 0) {
                        EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_FLINCH_ATTACK, EM_SYNC_OWNER);
                    } else {
                        EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_FLINCH_B, EM_SYNC_OWNER);
                    }
                    break;
                case 2:
                    EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_KNOCK_DOWN, EM_SYNC_OWNER);
                    break;
                case 3:
                    EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_KNOCK_DOWN_B, EM_SYNC_OWNER);
                    break;
                case 4:
                default:
                    EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_FLINCH_B, EM_SYNC_OWNER);
                    break;
                }
            } else {
                if (EmBlangonga_IsSmallKind(em) == 0) {
                    for (i = 0; i < 8; i++) {
                        partStateSum += em->parts[i].state;
                    }
                    if (partStateSum <= 3) {
                        EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_FLINCH, EM_SYNC_OWNER);
                        break;
                    }
                }
                switch (em->flinchPart) {
                case 1:
                    EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_KNOCK_DOWN_B, EM_SYNC_OWNER);
                    break;
                case 2:
                    EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_KNOCK_DOWN, EM_SYNC_OWNER);
                    break;
                default:
                    EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_FLINCH_B, EM_SYNC_OWNER);
                    break;
                }
            }
            break;
        }
        break;
    case EM_DMG_HIT:
        if (em->type == EM_KIND_BLANGO) {
            if (em->stance == EM_STANCE_GROUND) {
                em->scriptPending = 0;
                EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_FLINCH, EM_SYNC_OWNER);
            }
        } else if (em->type == EM_KIND_CONGALALA && em->stateGroup == EM_GROUP_ATTACK && em->state == BLANG_ATK_CONGALALA_HITZONE_COMBO && (s8) em->hitzoneSet == 1 &&
                   w->hitSeCooldown == 0) {
            Em_PlaySeChecked(em, 27, 0, &em->posX, 6, 0);
            w->hitSeCooldown = 30;
        }
        break;
    case EM_DMG_ATTACK13:
        Em_ResetKindTimer(em);
        EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_HOLD2, EM_SYNC_OWNER);
        break;
    case EM_DMG_STATUS5:
        if ((em->stateGroup != EM_GROUP_REACT || em->state != BLANG_REACT_FALL) && (em->stateGroup != EM_GROUP_REACT || em->state != BLANG_REACT_STUN_DOWNED)) {
            if ((s8) em->holdTimer != 0) {
                Em_SetStatus5Timer(em);
                EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_STUN_DOWNED, EM_SYNC_OWNER);
            } else if ((em->stateGroup != EM_GROUP_REACT || em->state != BLANG_REACT_STUN) && (em->stateGroup != EM_GROUP_REACT || em->state != BLANG_REACT_STUN_B)) {
                Em_SetStatus5Timer(em);
                if (((em->hitAngle - em->rotY) & 0xFFFF) > 0x8000) {
                    EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_STUN, EM_SYNC_OWNER);
                } else {
                    EmBlangonga_ChangeState(em, EM_GROUP_REACT, BLANG_REACT_STUN_B, EM_SYNC_OWNER);
                }
            }
        }
        break;
    }
done: /* the Congalala drops leave the result switch from inside the part switch */
    if (Em_DamageResultIsReaction(em, result) == 1) {
        EmBlangonga_BreakParts(em, 0);
    }
    switch (gQuest.stage) {
    case QUEST_ID_CONGALALA_SCENE_1:
    case QUEST_ID_CONGALALA_SCENE_2:
        if (em->type == EM_KIND_CONGALALA && Area_DayId(em->area) == AREA_JUNGLE_3 && Save_TestEventFlag(EVENT_FLAG_CONGALALA_SCENE) == 0) {
            if (Area_DayId(gScene.area) == AREA_JUNGLE_3 && gScene.eventRunning == 1 && em->stateGroup != EM_GROUP_EVENT) {
                EmBlangonga_ChangeState(em, EM_GROUP_EVENT, BLANG_EVENT_CONGALALA_SCENE, EM_SYNC_SEND);
            }
            goto skip;
        }
        break;
    case QUEST_ID_BLANGONGA_SCENE_1:
    case QUEST_ID_BLANGONGA_SCENE_2:
        if ((em->type == EM_KIND_BLANGONGA || em->type == EM_KIND_BLANGO) && Area_DayId(em->area) == AREA_SNOWY_MOUNTAINS_6 && Save_TestEventFlag(EVENT_FLAG_BLANGONGA_SCENE) == 0) {
            if (Area_DayId(gScene.area) == AREA_SNOWY_MOUNTAINS_6 && gScene.eventRunning == 1 && em->stateGroup != EM_GROUP_EVENT) {
                EmBlangonga_ChangeState(em, EM_GROUP_EVENT, BLANG_EVENT_BLANGONGA_SCENE, EM_SYNC_SEND);
            }
            goto skip;
        }
        break;
    }
    Em_UpdatePendingScript(em);
skip: /* a story scene keeps the pending script from starting */
    Em_ClearHitReactMode(em);
    EmBlangonga_RunState(em, (s32) w);
    if (em->stateRequest != 0) {
        EmBlangonga_RunState(em, (s32) w);
        em->stateRequest = 0;
    }
    if (EmBlangonga_InStateArmor(em) == 1) {
        em->hideShadowHold = 2;
        em->hideShadow = 2;
        em->guardTimer = 2;
    }
    EmBlangonga_UpdateTint(em);
}

/*
 * Runs the current state through its group's dispatcher (EM_GROUP_EVENT2
 * uses EM_GROUP_EVENT's) after keeping the group and state as the previous
 * ones.
 */
void EmBlangonga_RunState(Em *em, s32 work)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->stateGroup) {
    case EM_GROUP_IDLE:
        EmBlangonga_RunIdleState(em, (EmWork *) work);
        break;
    case EM_GROUP_MOVE:
        EmBlangonga_RunMoveState(em, work);
        break;
    case EM_GROUP_FLY:
        EmBlangonga_RunLeapState(em, (EmWork *) work);
        break;
    case EM_GROUP_ATTACK:
        EmBlangonga_RunAttackState(em, (EmWork *) work);
        break;
    case EM_GROUP_REACT:
        EmBlangonga_RunReactState(em, (EmWork *) work);
        break;
    case EM_GROUP_DEFEAT:
        EmBlangonga_RunDefeatState(em, work);
        break;
    case EM_GROUP_EVENT:
        EmBlangonga_RunEventState(em, (EmWork *) work);
        break;
    case EM_GROUP_EVENT2:
        EmBlangonga_RunEventState(em, (EmWork *) work);
        break;
    case EM_GROUP_VERSUS:
        EmBlangonga_RunVersusState(em, (EmWork *) work);
        
    default:
        break;
    }
}

/*
 * Blangonga: a flinch of the damaged part 4 whose flags have bit 2 flags
 * part 2's break (Em_FlagPartBreak); then, when `apply`, applies the part
 * breaks (Em_ApplyPartBreaks).
 */
void EmBlangonga_BreakParts(Em *em, s32 apply)
{
    u8 k;

    if (em->type == EM_KIND_BLANGONGA) {
        k = em->flinchPart;
        switch (k) {
        case 4:
            if (em->parts[k].state > 0 && (em->partFlags[k] & 2)) {
                Em_FlagPartBreak(em, 2);
            }
            break;
        }
    }
    if (apply != 0) {
        Em_ApplyPartBreaks(em);
    }
}

/*
 * In the player's area, creates the ground effect (Effect0D) of `kind` at
 * bone `bone` dropped to the floor, scaled by the monster's size.
 */
void EmBlangonga_SpawnGroundEffect(Em *em, s32 bone, s32 kind, f32 scale)
{
    f32 pos[3];

    if (Player_InCurrentArea(em)) {
        Em_BonePos(em, bone, pos);
        pos[1] = em->floorY;
        PartFx_SpawnOnGround(pos, kind, scale * em->scaleX);
    }
}

/*
 * While enraged, in the motions of its list: Rajang creates stage object
 * 39 every 16 frames, the others stage object 7 every 45 frames
 * (staggered by the monster's slot).
 */
void EmBlangonga_SpawnK1CInMotions(Em *em, s16 motion)
{
    switch (motion) {
    case EM_MOTION_ID(0, BLANG_MOT_IDLE):
    case EM_MOTION_ID(0, BLANG_MOT_WALK):
    case EM_MOTION_ID(0, BLANG_MOT_TURN_LEFT):
    case EM_MOTION_ID(0, BLANG_MOT_TURN_RIGHT):
    case EM_MOTION_ID(0, BLANG_MOT_TURN_LEFT_QUICK):
    case EM_MOTION_ID(0, BLANG_MOT_TURN_RIGHT_QUICK):
    case EM_MOTION_ID(0, BLANG_MOT_LIMP):
    case EM_MOTION_ID(0, BLANG_MOT_SLAM_WINDUP):
    case EM_MOTION_ID(0, BLANG_MOT_STRIKE_A):
    case EM_MOTION_ID(0, BLANG_MOT_STRIKE_B):
    case EM_MOTION_ID(0, BLANG_MOT_ALERT_IDLE):
    case EM_MOTION_ID(0, BLANG_MOT_ROAR):
    case EM_MOTION_ID(0, BLANG_MOT_TURN_ABOUT):
    case EM_MOTION_ID(0, BLANG_MOT_TURN_ABOUT_B):
    case EM_MOTION_ID(0, BLANG_MOT_SHAKE_OFF):
    case EM_MOTION_ID(0, BLANG_MOT_ROAR_C_END):
        if (em->rage != 0) {
            if (EmBlangonga_IsRajang(em) == 1) {
                if ((gScene.frame & 0xF) == em->index * 2) {
                    MonsterFx_CreateOnEm(em, 39, 1.0f);
                }
            } else if (gScene.frame % 45 == em->index * 2) {
                MonsterFx_CreateOnEm(em, 7, em->scaleX);
            }
        }
        break;
    }
}

/*
 * Shakes the camera (CamShake_ReqAIf level `level`) when the layer-0 motion
 * passes `frame`.
 */
void EmBlangonga_CamShakeOnFrame(s32 em, s32 frame, s32 i)
{
    if (EmMotion_PassedFrameScaled((Em *) em, (f32) frame, 0U) != 0) {
        CamShake_ReqAIf((void *) em, i);
    }
}

/* Does nothing. */
void EmBlangonga_MotionEffectsDefault(Em *em)
{
}

/*
 * The motions' effects: keeps the layer-0 motion id (EmWork.prevMotion),
 * plays the kind's sounds (EmBlangonga_MotionSounds, ...Congalala,
 * ...Rajang) and the rage's stage objects, then per motion
 * and frame the hits (BlangongaAttack), ground effects, stage objects and
 * camera shakes.
 */
void EmBlangonga_MotionEffects(Em *em, EmWork *work)
{
    f32 ofs[3];
    f32 pos[3];

    if (em->motionId[0] != work->prevMotion) {
        work->prevMotion = em->motionId[0];
    }
    switch (em->type) {
    case EM_KIND_BLANGONGA:
    default:
    case EM_KIND_BLANGO:
        EmBlangonga_MotionSounds(em, work);
        break;
    case EM_KIND_CONGALALA:
    case EM_KIND_CONGA:
        EmBlangonga_MotionSoundsCongalala(em, work);
        break;
    case EM_KIND_RAJANG:
        EmBlangonga_MotionSoundsRajang(em, work);
        break;
    }
    EmBlangonga_SpawnK1CInMotions(em, work->prevMotion);
    switch (work->prevMotion) {
    case EM_MOTION_ID(0, BLANG_MOT_TURN_LEFT_QUICK):
        if ((EmBlangonga_IsSmallKind(em) == 0) && (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0)) {
            BlangongaAttack_Create(em, 14);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TURN_RIGHT_QUICK):
        if ((EmBlangonga_IsSmallKind(em) == 0) && (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0)) {
            BlangongaAttack_Create(em, 16);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_MOVE_ACTION_D):
        if ((EmBlangonga_IsSmallKind(em) == 0) && (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0)) {
            BlangongaAttack_Create(em, 11);
        }
        if (EmMotion_PassedFrameScaled(em, 14.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 2, 15, em->scaleX);
        }
        break;
    case EM_MOTION_ID(0, 11):
        if (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0) {
            EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_MOVE_ACTION_A):
        if ((EmBlangonga_IsSmallKind(em) == 0) && (EmMotion_PassedFrameScaled(em, 18.0f, 0) != 0)) {
            BlangongaAttack_Create(em, 12);
        }
        if (EmMotion_PassedFrameScaled(em, 14.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 2, 15, em->scaleX);
        }
        break;
    case EM_MOTION_ID(0, 13):
        if (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0) {
            EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_MOVE_ACTION_B):
        if ((EmBlangonga_IsSmallKind(em) == 0) && (EmMotion_PassedFrameScaled(em, 18.0f, 0) != 0)) {
            BlangongaAttack_Create(em, 13);
        }
        if (EmMotion_PassedFrameScaled(em, 14.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 3, 16, em->scaleX);
        }
        break;
    case EM_MOTION_ID(0, 15):
        if (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0) {
            EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_MOVE_ACTION_C):
        if (EmMotion_PassedFrameScaled(em, 18.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 2, 17, em->scaleX);
        }
        if (EmMotion_PassedFrameScaled(em, 22.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 3, 18, em->scaleX);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_JUMP):
        if (EmBlangonga_IsRajang(em) == 1) {
            if (EmMotion_PassedFrameScaled(em, 48.0f, 0) != 0) {
                DustCloud_Create((Em *) em, 19, 0, em->scaleX);
                break;
            }
        } else if (EmBlangonga_IsCongalala(em) == 1) {
            if (EmMotion_PassedFrameScaled(em, 54.0f, 0) != 0) {
                DustCloud_Create((Em *) em, 19, 0, em->scaleX);
                break;
            }
        } else if (EmMotion_PassedFrameScaled(em, 50.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 19, 0, em->scaleX);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_CHARGE):
        if (EmBlangonga_IsCongalala(em) == 1) {
            if (EmMotion_PassedFrameScaled(em, 14.0f, 0) != 0) {
                DustCloud_Create((Em *) em, 2, 15, em->scaleX);
            }
            if (EmMotion_PassedFrameScaled(em, 42.0f, 0) != 0) {
                DustCloud_Create((Em *) em, 3, 16, em->scaleX);
                break;
            }
        } else {
            if (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0) {
                DustCloud_Create((Em *) em, 3, 16, em->scaleX);
            }
            if (EmMotion_PassedFrameScaled(em, 38.0f, 0) != 0) {
                DustCloud_Create((Em *) em, 2, 17, em->scaleX);
            }
            if (EmMotion_PassedFrameScaled(em, 42.0f, 0) != 0) {
                DustCloud_Create((Em *) em, 3, 16, em->scaleX);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_JUMP_LAND):
        if (EmBlangonga_IsRajang(em) == 1) {
            if (EmMotion_PassedFrameScaled(em, 12.0f, 0) != 0) {
                DustCloud_Create((Em *) em, 11, 0, 0.6f * em->scaleX);
                break;
            }
        } else if (EmMotion_PassedFrameScaled(em, 8.0f, 0) != 0) {
            EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DIG_IN):
        if (EmMotion_PassedFrameScaled(em, 22.0f, 0) || EmMotion_PassedFrameScaled(em, 92.0f, 0) || EmMotion_PassedFrameScaled(em, 160.0f, 0) || EmMotion_PassedFrameScaled(em, 210.0f, 0) || EmMotion_PassedFrameScaled(em, 246.0f, 0)) {
            DustCloud_Create((Em *) em, 2, 17, em->scaleX);
        }
        if ((EmMotion_PassedFrameScaled(em, 64.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 134.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 170.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 200.0f, 0) != 0)) {
            DustCloud_Create((Em *) em, 3, 18, em->scaleX);
        }
        if (EmMotion_PassedFrameScaled(em, 254.0f, 0) != 0) {
            DustCloud_Create((Em *) em, 2, 0, em->scaleX);
        }
        if (EmMotion_PassedFrameScaled(em, 42.0f, 0) != 0) {
            BlangongaAttack_Create(em, 27);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_EMERGE):
        if (EmBlangonga_IsSmallKind(em) == 0) {
            EmBlangonga_CamShakeOnFrame((s32) em, 4, 5);
        }
        if ((EmMotion_PassedFrameScaled(em, 8.0f, 0) != 0) && Player_InCurrentArea(em)) {
            Em_BonePos(em, 1, pos);
            pos[1] = em->floorY;
            MonsterFx_CreateAt(pos, 6, 1.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 52.0f, 0) != 0) {
            EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_COLLAPSE):
        if (EmMotion_PassedFrameScaled(em, 84.0f, 0) != 0) {
            EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_LIE_DOWN):
        if (EmMotion_PassedFrameScaled(em, 170.0f, 0) != 0) {
            EmBlangonga_SpawnGroundEffect(em, 16, 7, 3.5f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TRIPLE_HIT):
        if (EmBlangonga_IsBlangonga(em) == 1) {
            if (EmMotion_PassedFrameScaled(em, 70.0f, 0) != 0) {
                DustCloud_Create((Em *) em, 2, 15, em->scaleX);
            }
            if (EmMotion_PassedFrameScaled(em, 92.0f, 0) != 0) {
                DustCloud_Create((Em *) em, 2, 17, em->scaleX);
            }
            if (EmMotion_PassedFrameScaled(em, 104.0f, 0) != 0) {
                EmBlangonga_SpawnGroundEffect(em, 16, 7, 3.5f);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TRIPLE_HIT_END):
        if (EmBlangonga_IsCongalala(em) == 1) {
            if ((EmMotion_PassedFrameScaled(em, 38.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 124.0f, 0) != 0)) {
                DustCloud_Create((Em *) em, 3, 34, em->scaleX);
            }
            if ((EmMotion_PassedFrameScaled(em, 76.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 172.0f, 0) != 0)) {
                DustCloud_Create((Em *) em, 2, 33, em->scaleX);
            }
            if (EmMotion_PassedFrameScaled(em, 200.0f, 0) != 0) {
                EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_STRIKE_A):
        if (EmBlangonga_IsRajang(em) == 1) {
            if (EmMotion_PassedFrameScaled(em, 48.0f, 0) != 0) {
                DustCloud_Create((Em *) em, 3, 34, em->scaleX);
            }
            if ((EmMotion_PassedFrameScaled(em, 22.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 74.0f, 0) != 0)) {
                DustCloud_Create((Em *) em, 2, 33, em->scaleX);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_STRIKE_B):
        if ((EmBlangonga_IsRajang(em) == 1) && (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0)) {
            EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DIG_THROW):
        if ((EmMotion_PassedFrameScaled(em, 102.0f, 0) != 0) && Player_InCurrentArea(em)) {
            Em_BonePos(em, 1, pos);
            pos[1] = em->floorY;
            ofs[0] = 0.0f;
            ofs[1] = 0.0f;
            ofs[2] = 330.0f * em->scaleX;
            Vec_RotateY(ofs, (6.2831855f * (f32) em->rotY) / 65536.0f);
            pos[0] += ofs[0];
            pos[1] += ofs[1];
            pos[2] += ofs[2];
            MonsterFx_CreateAt(pos, 6, 0.8f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_CHARGE_END):
        if (EmBlangonga_IsCongalala(em) == 1) {
            if (EmMotion_PassedFrameScaled(em, 34.0f, 0) != 0) {
                DustCloud_Create((Em *) em, 3, 16, em->scaleX);
                break;
            }
        } else if (EmBlangonga_IsRajang(em) != 0) {
            if (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) {
                DustCloud_Create((Em *) em, 2, 15, em->scaleX);
            }
            if (EmMotion_PassedFrameScaled(em, 56.0f, 0) != 0) {
                EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
                break;
            }
        } else {
            if (EmMotion_PassedFrameScaled(em, 12.0f, 0) != 0) {
                DustCloud_Create((Em *) em, 2, 15, em->scaleX);
            }
            if (EmMotion_PassedFrameScaled(em, 38.0f, 0) != 0) {
                EmBlangonga_SpawnGroundEffect(em, 16, 7, 3.5f);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_HOP_LAND):
        if ((EmBlangonga_IsCongalala(em) == 1) && (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0)) {
            EmBlangonga_SpawnGroundEffect(em, 1, 40, 1.0f);
            if ((s32)Player_InCurrentArea(em) & 0xFF) {
                AttackBurst_CreateAtScaled(&em->posX, (u16) em->rotY, 10U, 0.4f * em->scaleX);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_ROAR):
        if (EmBlangonga_IsBlangonga(em) == 1) {
            if (em->type == EM_KIND_BLANGONGA) {
                if ((em->partBreakCount <= 0) && (EmMotion_PassedFrameScaled(em, 38.0f, 0) != 0)) {
                    BlangongaAttack_Create(em, 23);
                    ElementBurst_CreateOnEm(em, 5, 2, 0.4f * em->scaleX);
                    break;
                }
            } else if (EmMotion_PassedFrameScaled(em, 60.0f, 0) != 0) {
                EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
                break;
            }
        } else if (em->type == EM_KIND_CONGALALA) {
            if ((gScene.eventRunning == 0) && (EmMotion_PassedFrameScaled(em, 70.0f, 0) != 0)) {
                EmEffectAttack_Create(em, EMFX_CONGALALA_RAGE_GAS, 0, 0, 0);
                break;
            }
        } else if ((EmBlangonga_IsRajang(em) == 1) && (EmMotion_PassedFrameScaled(em, 38.0f, 0) != 0)) {
            BlangongaAttack_Create(em, 23);
            ElementBurst_CreateOnEm(em, 5, 2, 0.4f * em->scaleX);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_EAT):
        if ((EmBlangonga_IsCongalala(em) == 1) && (EmMotion_PassedFrameScaled(em, 66.0f, 0) != 0)) {
            EmBlangonga_SpawnGroundEffect(em, 2, 7, 3.5f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_ROAR_B):
        if ((EmBlangonga_IsSmallKind(em) == 0) && (EmMotion_PassedFrameScaled(em, 64.0f, 0) != 0)) {
            BlangongaAttack_Create(em, 23);
            ElementBurst_CreateOnEm(em, 5, 2, 0.4f * em->scaleX);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DOWN):
        if (EmMotion_PassedFrameScaled(em, 108.0f, 0) != 0) {
            EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_FALL_LAND):
        if (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0) {
            EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DIE):
        if (EmMotion_PassedFrameScaled(em, 180.0f, 0) != 0) {
            EmBlangonga_SpawnGroundEffect(em, 16, 7, 3.5f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_KNOCK_DOWN):
        if ((EmMotion_PassedFrameScaled(em, 28.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 78.0f, 0) != 0)) {
            EmBlangonga_SpawnGroundEffect(em, BLANG_BONE_BODY, 7, 3.5f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_KNOCK_DOWN_B):
        if ((EmMotion_PassedFrameScaled(em, 28.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 78.0f, 0) != 0)) {
            EmBlangonga_SpawnGroundEffect(em, BLANG_BONE_BODY, 7, 3.5f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DIE_END):
        if (EmMotion_PassedFrameScaled(em, 18.0f, 0) != 0) {
            BlangongaAttack_Create(em, 28);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_TRAPPED):
        if (EmBlangonga_IsCongalala(em) == 1) {
            if (EmMotion_PassedFrameScaled(em, 16.0f, 0) != 0) {
                EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
                break;
            }
        } else if (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) {
            EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_STRUGGLE):
        if ((EmBlangonga_IsCongalala(em) == 1) && (em->rage != 0) && ((EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 40.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 80.0f, 0) != 0))) {
            MonsterFx_CreateOnEm(em, 20, 1.0f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DOWNED):
        if (EmMotion_PassedFrameScaled(em, 92.0f, 0) != 0) {
            EmBlangonga_SpawnGroundEffect(em, BLANG_BONE_BODY, 7, 3.5f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DOWN_B):
        if (EmMotion_PassedFrameScaled(em, 108.0f, 0) != 0) {
            EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_DROP_CARRIED):
        if ((EmBlangonga_IsCongalala(em) == 1) && (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0)) {
            EmBlangonga_SpawnGroundEffect(em, 16, 7, 3.5f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_EFFECT_STRIKE):
        if ((EmBlangonga_IsRajang(em) == 1) && (EmMotion_PassedFrameScaled(em, 74.0f, 0) != 0)) {
            BlangongaAttack_Create(em, 40);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_EFFECT_STRIKE_END):
        if ((EmBlangonga_IsCongalala(em) == 1) && (EmMotion_PassedFrameScaled(em, 44.0f, 0) != 0)) {
            EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_CONGALALA_HITZONE_COMBO_END):
        if (EmBlangonga_IsCongalala(em) == 1) {
            if (EmMotion_PassedFrameScaled(em, 96.0f, 0) != 0) {
                EmBlangonga_SpawnGroundEffect(em, 16, 7, 3.5f);
                break;
            }
        } else {
            if ((EmBlangonga_IsRajang(em) == 1) && (EmMotion_PassedFrameScaled(em, 26.0f, 0) != 0)) {
                DustCloud_Create((Em *) em, 19, 0, em->scaleX);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_RAJANG_JUMP_SHOT):
        if (EmBlangonga_IsCongalala(em) == 1) {
            if (EmMotion_PassedFrameScaled(em, 56.0f, 0) != 0) {
                EmBlangonga_SpawnGroundEffect(em, 16, 7, 3.5f);
            }
            if (EmMotion_PassedFrameScaled(em, 90.0f, 0) != 0) {
                EmBlangonga_SpawnGroundEffect(em, 2, 7, 3.5f);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_RAJANG_JUMP_SHOT_LAND):
        if ((EmBlangonga_IsRajang(em) == 1) && (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0)) {
            DustCloud_Create((Em *) em, 11, 0, 0.6f * em->scaleX);
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_BACK_STRIKE):
        if (EmBlangonga_IsBlangonga(em) == 1 || EmBlangonga_IsRajang(em) == 1) {
            if (EmMotion_PassedFrameScaled(em, 44.0f, 0) != 0) {
                BlangongaAttack_Create(em, 9);
            }
            if (EmMotion_PassedFrameScaled(em, 50.0f, 0) != 0) {
                BlangongaAttack_Create(em, 20);
            }
            if ((EmMotion_PassedFrameScaled(em, 48.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 78.0f, 0) != 0)) {
                EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_ROAR_C):
        if (EmBlangonga_IsBlangonga(em) == 1) {
            if ((EmMotion_PassedFrameScaled(em, 60.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 94.0f, 0) != 0)) {
                DustCloud_Create((Em *) em, 3, 18, em->scaleX);
                break;
            }
        } else {
            if (EmBlangonga_IsRajang(em) == 1) {
                if (EmMotion_PassedFrameScaled(em, 52.0f, 0) != 0) {
                    DustCloud_Create((Em *) em, 2, 15, em->scaleX);
                }
                if (EmMotion_PassedFrameScaled(em, 62.0f, 0) != 0) {
                    DustCloud_Create((Em *) em, 3, 16, em->scaleX);
                }
                if (EmMotion_PassedFrameScaled(em, 96.0f, 0) != 0) {
                    EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
                    break;
                }
            }
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_ROAR_C_END):
        if (EmBlangonga_IsRajang(em) == 1) {
            if (EmMotion_PassedFrameScaled(em, 16.0f, 0) || EmMotion_PassedFrameScaled(em, 52.0f, 0) || EmMotion_PassedFrameScaled(em, 84.0f, 0)) {
                DustCloud_Create((Em *) em, 2, 15, em->scaleX);
            }
            if ((EmMotion_PassedFrameScaled(em, 46.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 90.0f, 0) != 0)) {
                DustCloud_Create((Em *) em, 3, 16, em->scaleX);
                break;
            }
        }
        break;
    case EM_MOTION_ID(0, BLANG_MOT_SLAM):
        if (EmBlangonga_IsCongalala(em) == 1) {
            if (EmMotion_PassedFrameScaled(em, 36.0f, 0) != 0) {
                EmBlangonga_SpawnGroundEffect(em, 16, 19, 1.0f);
            }
        } else {
            if (EmMotion_PassedFrameScaled(em, 22.0f, 0) != 0) {
                EmBlangonga_SpawnGroundEffect(em, 16, 19, 1.0f);
            }
            if ((EmMotion_PassedFrameScaled(em, 24.0f, 0) != 0) && Player_InCurrentArea(em)) {
                Em_BonePos(em, 16, pos);
                pos[1] = em->floorY += 20.0f;
                AttackBurst_CreateAtScaled(pos, (u16) em->rotY, 10U, 0.6f * em->scaleX);
            }
        }
    case EM_MOTION_ID(0, BLANG_MOT_RAJANG_CHARGE_END):
        if ((EmBlangonga_IsRajang(em) == 1) && (EmMotion_PassedFrameScaled(em, 36.0f, 0) != 0)) {
            EmBlangonga_SpawnGroundEffect(em, 1, 7, 3.5f);
        }
        break;
    default:
        EmBlangonga_MotionEffectsDefault(em);
        break;
    case EM_MOTION_ID(0, BLANG_MOT_IDLE):
    case EM_MOTION_ID(0, 2):
    case EM_MOTION_ID(0, BLANG_MOT_WALK):
    case EM_MOTION_ID(0, BLANG_MOT_TURN_LEFT):
    case EM_MOTION_ID(0, BLANG_MOT_TURN_RIGHT):
    case EM_MOTION_ID(0, BLANG_MOT_LIMP):
    case EM_MOTION_ID(0, BLANG_MOT_JUMP_FALL):
    case EM_MOTION_ID(0, BLANG_MOT_LIE):
    case EM_MOTION_ID(0, BLANG_MOT_GET_UP):
    case EM_MOTION_ID(0, BLANG_MOT_SLAM_WINDUP):
    case EM_MOTION_ID(0, BLANG_MOT_THROW):
    case EM_MOTION_ID(0, BLANG_MOT_ALERT_IDLE):
    case EM_MOTION_ID(0, BLANG_MOT_TURN_ABOUT):
    case EM_MOTION_ID(0, BLANG_MOT_TURN_ABOUT_B):
    case EM_MOTION_ID(0, BLANG_MOT_EAT_NEED2):
    case EM_MOTION_ID(0, BLANG_MOT_EAT_NEED2_END):
    case EM_MOTION_ID(0, BLANG_MOT_EAT_END):
    case EM_MOTION_ID(0, BLANG_MOT_FLINCH_B):
    case EM_MOTION_ID(0, BLANG_MOT_FLINCH):
    case EM_MOTION_ID(0, BLANG_MOT_SHAKE_OFF):
    case EM_MOTION_ID(0, BLANG_MOT_DOWN_GET_UP):
    case EM_MOTION_ID(0, BLANG_MOT_STATUS4):
    case EM_MOTION_ID(0, BLANG_MOT_FALL):
    case EM_MOTION_ID(0, 76):
    case EM_MOTION_ID(0, BLANG_MOT_DOWN_GET_UP_B):
        break;
    }
}

/*
 * Callback slot 3: the first frame only marks the effects started (fxStep);
 * from then on runs EmBlangonga_MotionEffects every frame.
 */
void EmBlangonga_UpdateMotionEffects(Em *em)
{
    EmBlangongaWork *w = (EmBlangongaWork *)&em->work;

    switch (w->fxStep) {
    case 0:
        w->fxStep++;
        break;
    case 1:
        EmBlangonga_MotionEffects(em, (EmWork *) w);
        break;
    }
}

/* Callback slot 2: does nothing. */
void EmBlangonga_TableNop(void)
{
}

/*
 * Callback slot 4, the monster script's questions (`kind`): 0 whether
 * moveCount reached 4; 1 whether it stands on snow (floor attribute 1 in
 * AREA_SNOWY_MOUNTAINS_5, AREA_SNOWY_MOUNTAINS_6 and AREA_SNOWY_MOUNTAINS_8,
 * while in the player's area); 2 stateArmor; 3 eatenKind; 4 whether a visible Blangonga in its area is in
 * BLANG_IDLE_ROAR_SIGNAL; 5 the destination area's index among the family's
 * twelve areas (0xFF if none); 6 the first carried item shown (item 0 gives
 * 1, item 2 gives 2, item 1 gives 3; 0 if none).
 */
u8 EmBlangonga_ScriptQuery(Em *em, u8 query)
{
    EmBlangongaWork *w = (EmBlangongaWork *)&em->work;
    Em *other;
    s16 i;

    switch (query) {
    case BLANG_QUERY_MOVE_COUNT:
        return w->moveCount >= 4;
    case BLANG_QUERY_ON_SNOW:
        switch (Area_DayId(em->area)) {
        case AREA_SNOWY_MOUNTAINS_5:
        case AREA_SNOWY_MOUNTAINS_6:
        case AREA_SNOWY_MOUNTAINS_8:
            if (Player_InCurrentArea(em)) {
                if (*(u8 *) &em->floorAttr == 1) {
                    return 1;
                }
                return 0;
            }
            return 0;
        }
        return 0;
    case BLANG_QUERY_ARMOR:
        return EmBlangonga_InStateArmor(em) == 1;
    case BLANG_QUERY_EATEN_KIND:
        return w->eatenKind;
    case BLANG_QUERY_ROAR_SIGNAL:
        for (other = gEms, i = 0; i < EM_COUNT; i++, other++) {
            if (other->active != 0 && other->visible != 0 && (Obj_SameArea(em, (Player *) other) & 0xFF) && other->type == EM_KIND_BLANGONGA && Player_IsState(other, EM_GROUP_IDLE, BLANG_IDLE_ROAR_SIGNAL)) {
                return 1;
            }
        }
        return 0;
    case BLANG_QUERY_DEST_AREA:
        switch (Area_DayId(*(u16 *) &em->destArea)) {
        case AREA_JUNGLE_1:
            return 0;
        case AREA_JUNGLE_2:
            return 1;
        case AREA_JUNGLE_3:
            return 2;
        case AREA_JUNGLE_6:
            return 3;
        case AREA_JUNGLE_9:
            return 4;
        case AREA_JUNGLE_10:
            return 5;
        case AREA_SWAMP_2:
            return 6;
        case AREA_SWAMP_5:
            return 7;
        case AREA_SWAMP_6:
            return 8;
        case AREA_SWAMP_7:
            return 9;
        case AREA_SWAMP_8:
            return 10;
        case AREA_SWAMP_9:
            return 11;
        default:
            return 255;
        }
    case BLANG_QUERY_CARRIED:
        if (EmBlangonga_CarriedShown(em, 0) == 1) {
            return 1;
        }
        if (EmBlangonga_CarriedShown(em, 2) == 1) {
            return 2;
        }
        if (EmBlangonga_CarriedShown(em, 1) == 1) {
            return 3;
        }
        return 0;
    default:
        return 0;
    }
}

/*
 * Callback slot 6: returns 1 while no part break has happened (Em.partBreakCount
 * 0) and Blangonga is on the ground, or Rajang enraged on the ground: the
 * condition for BLANG_REACT_PART_BREAK.
 */
s32 EmBlangonga_AllowPartBreak(Em *em)
{
    if ((s8) em->partBreakCount != 0) {
        return 0;
    }
    if (em->type == EM_KIND_BLANGONGA) {
        if (em->stance == EM_STANCE_GROUND) {
            return 1;
        }
    } else if (em->type == EM_KIND_RAJANG && em->rage != 0 && em->stance == EM_STANCE_GROUND) {
        return 1;
    }
    return 0;
}

/* The file's initialised data, in the game's order */
EmCurveKey EmBlangonga_VelYCurves_0[10] = {
#include "game/008C9C60.inc"
};
EmCurveKey EmBlangonga_VelYCurves_1[4] = {
#include "game/008C9CB0.inc"
};
EmCurveKey EmBlangonga_VelYCurves_2[13] = {
#include "game/008C9CD0.inc"
};
EmCurveKey *EmBlangonga_VelYCurves[3] = {
    EmBlangonga_VelYCurves_0, EmBlangonga_VelYCurves_1, EmBlangonga_VelYCurves_2,
};
EmCurveKey EmBlangonga_SpeedCurves_2[13] = {
#include "game/008C9D50.inc"
};
EmCurveKey *EmBlangonga_SpeedCurves[3] = {
    NULL, NULL, EmBlangonga_SpeedCurves_2,
};
EmBlangongaTurnData EmBlangonga_TurnData = {
#include "game/008C9DD0.inc"
};
Vec3f EmBlangonga_CarveThrowPt = {
#include "game/008C9E28.inc"
};
void *EmBlangonga_Callbacks[8] = {
    EmBlangonga_SpawnShadow, EmBlangonga_SpawnShadow, EmBlangonga_TableNop, EmBlangonga_UpdateMotionEffects,
    EmBlangonga_ScriptQuery, EmBlangonga_Reinit, EmBlangonga_AllowPartBreak, NULL,
};
