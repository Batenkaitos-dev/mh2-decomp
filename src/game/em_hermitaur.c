/*
 * Monster family: Daimyo Hermitaur and Shogun Ceanataur, and the small
 * Hermitaur and Ceanataur (EM_KIND_DAIMYO_HERMITAUR, EM_KIND_SHOGUN_CEANATAUR,
 * EM_KIND_HERMITAUR, EM_KIND_CEANATAUR), from the family's first function,
 * EmHermitaur_MotionSounds, to the next family's. EmHermitaur_Setup, which
 * the per-kind dispatch Em_TypeSetup calls, sets a monster up, and
 * EmHermitaur_Start starts it.
 *
 * EmHermitaur_Update runs every frame: it runs the Shogun's rage
 * bookkeeping, picks the reaction to Em_UpdateDamage's result, starts the
 * first-sighting scenes, runs the current state through EmHermitaur_RunState
 * and one dispatcher per state group, and then does the part-break and
 * shell bookkeeping. The state
 * functions take the monster and its work area (EmWork; EmHermitaurWork
 * below names this family's own bytes); Em.step is the step within the
 * current state. EmHermitaur_ChangeState does each state's entry work.
 * The states, motions, bones and sounds are named in em_hermitaur.h (a bone
 * or sound no code explains stays a number): the crabs dig in and
 * move under the ground, the Shogun also climbs to the ceiling and changes
 * its shell, the Daimyo guards with its shell (Em.hermitTimer).
 *
 * After the states: the callbacks (gGmEmCallbackTbl's row,
 * EmHermitaur_Callbacks): the motions' effects and attack hit zones
 * (EmHermitaur_MotionEffects), the script's queries and the part-break check.
 */
#include "common.h"
#include "quest.h"
#include "em.h"
#include "shadow.h"
#include "em_hermitaur.h"
#include "player.h"
#include "objattach.h"
#include "scene.h"
#include "area.h"
#include "capsule.h"
#include "save.h"
#include "sound.h"
#include "cam_shake.h"
#include "dust_flash_effects.h"
#include "engine_math.h"
#include "gm_em.h"
#include "monster_effects.h"
#include "part_effects.h"
#include "rand.h"
#include "vec_math.h"
#include "em_script.h"
/* prototypes of callees */
void EmRangedAttack_CreateAtBone(Em *em, s32 a, s32 b, s32 c, s32 d, s32 e);

/* prototypes */
/* unit's own declaration: declared here as the callers use it; with the definition's void (Em *em, s16 n), the calls in EmHermitaur_Feed compile differently */
void Em_AddHp(Em *em, s32 n);
/* unit's own declaration: declared here as the callers use it; with the definition's s32 (Em *em, s16 n, s16 max, s16 interval), the calls in EmHermitaur_LieDown compile differently */
s32 Em_RegenHp(Em *em, s32 n, s32 max, s32 interval);
/* unit's own declaration: declared here as the callers use it; with the definition's void (Em *em, u8 k, s16 v, s16 t, f32 y, f32 scale), the calls in EmHermitaur_Stun compile differently */
void StunStars_CreateEm(Em *, s32, s32, s32, f32, f32);
s32 EmMotion_PassedFrameScaled(void *p, f32 frame, s32 layer);
/* unit's own declaration: declared here as the callers use it; with the definition's void (void *obj, s16 bone, f32 *out), the calls in EmHermitaur_SpawnGroundEffect compile differently */
void Em_BonePos(void *obj, s32 a, f32 *out);
extern Vec3f EmHermitaur_CarveThrowPt __attribute__((aligned(8)));
void Em_EndAllStatus(Em *em);
/* unit's own declaration: declared here as the callers use it; with the definition's void (Em *em, s32 group, s32 state, u8 mode), the calls in EmHermitaur_ChangeState compile differently */
void Em_SetStateSync(Em *em, u16 group, u16 state, u8 mode);
extern EmTurnParams EmHermitaur_TurnParams[1];
struct AreaTri;
u8 AreaGrid2_GetCeiling(f32 *pos, f32 *y, struct AreaTri *info);
void *EmEffectAttack_Create(Em *em, s32 kind, s32 c, s32 a, s32 b);
extern f32 EmHermitaur_ThrustOffsets[];
u16 Area_IdForCurrentTime(u16);
void Em_SpawnLyingPuffs(Em *em, s32 bone, f32 *pos, f32 scale);
/* end of prototypes */

/* Hermitaur's own meanings of the work area's bytes (EmWork, include/em.h, has the shared ones). */
typedef struct EmHermitaurWork {
    u8 fxStep;                  /* 0x000: UpdateMotionEffects: 0, then 1 */
    u8 unk001[48 - 1];
    f32 distLeft;               /* 0x030: ChangeState: the distance to the goal (Vec_DistanceXZ, or 1000); the states take the steps from it */
    u16 goalAngle;              /* 0x034: Math_AngleXZ to the goal (EmHermitaur_TurnInPlace, EmHermitaur_TurnInPlaceAlert, EmHermitaur_BuriedTurnToGoal) */
    u8 unk036[55 - 54];
    u8 steerToGoal;             /* 0x037: ChangeState: 1 when the move has a goal, 0 when wandering; the walks and the buried move steer to the goal only while it is set */
    u8 unk038[57 - 56];
    u8 lowHp;                   /* 0x039: Update: 1 while hp is under 20% of the maximum, else 0 (no reader) */
    s16 rageFxTimer;            /* 0x03A: Update: 8 (calm) or 5 (enraged) when the Shogun's rage changes, counted down; HERM_SE_RAGE_CLAW at the start and again at 2; EmHermitaur_RageFxRatio */
    f32 thrustOrigin[3];        /* 0x03C: EmHermitaur_UnderThrustSeries: where the series started */
    u8 unk048[76 - 72];   /* 0x048: Update passes it as AreaGrid2_GetCeiling's triangle */
    s16 guardClankTimer;        /* 0x04C: Update: 30 when a hit lands on the guard (HERM_SE_GUARD_HIT), counted down (no reader) */
    u8 lastRage;                /* 0x04E: Update: the rage it last saw */
    u8 shellBreakCount;         /* 0x04F: the Shogun: +1 each time its shell (part 2) breaks; Setup and EmHermitaur_ScriptQuery compare it with 2 */
    u8 lastPartState;           /* 0x050: Init and Update: parts[2].state as last seen */
    u8 unk051[256 - 81];
} EmHermitaurWork;
typedef char EmHermitaurWork_size_check[(sizeof(EmHermitaurWork) == sizeof(EmWork)) ? 1 : -1];

/*
 * Plays the sounds of the current motion (work->fxMotion) at bones on given
 * frames; one switch for the Daimyo and the Shogun (Shogun variants inside
 * some cases), a second, smaller one for the small kinds Hermitaur and
 * Ceanataur.
 */
void EmHermitaur_MotionSounds(Em *em, EmWork *work)
{
    u8 type;

    type = em->type;
    if ((type == EM_KIND_DAIMYO_HERMITAUR) || (type == EM_KIND_SHOGUN_CEANATAUR)) {
                switch (work->fxMotion) {
        case EM_MOTION_ID(0, HERM_MOT_IDLE):
            Em_PlaySeAtBoneVolOnFrame(em, 4, Sound_RandomSe(60, 6, 61, 2), 14, 10);
            Em_PlaySeAtBoneVolOnFrame(em, 34, Sound_RandomSe(62, 6, 60, 2), 14, 10);
            Em_PlaySeAtBoneVolOnFrame(em, 90, Sound_RandomSe(61, 6, 62, 2), 14, 10);
            Em_PlaySeAtBoneVolOnFrame(em, 150, Sound_RandomSe(60, 6, 61, 2), 14, 10);
            Em_PlaySeAtBoneVolOnFrame(em, 222, Sound_RandomSe(62, 6, 60, 2), 14, 10);
            Em_PlaySeAtBoneVolOnFrame(em, 314, Sound_RandomSe(61, 6, 62, 2), 14, 10);
            Em_PlaySeAtBoneOnFrame(em, 168, 4, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 290, 5, HERM_BONE_LEG_A2);
            break;
        case EM_MOTION_ID(0, HERM_MOT_WALK):
            Em_PlaySeAtBoneVolOnFrame(em, 22, Sound_RandomSe(60, 4, 61, 4), 14, 10);
            Em_PlaySeAtBoneVolOnFrame(em, 64, Sound_RandomSe(62, 4, 60, 4), 14, 10);
            Em_PlaySeAtBoneOnFrame(em, 50, Sound_RandomSe(15, 4, 17, 4), HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 42, 0, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 84, 1, HERM_BONE_LEG_A2);
            Em_PlaySeAtBoneOnFrame(em, 92, 0, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 32, 1, HERM_BONE_LEG_B2);
            break;
        case EM_MOTION_ID(0, HERM_MOT_WALK_BACK):
            Em_PlaySeAtBoneVolOnFrame(em, 20, Sound_RandomSe(60, 4, 61, 4), 14, 10);
            Em_PlaySeAtBoneVolOnFrame(em, 72, Sound_RandomSe(62, 4, 60, 4), 14, 10);
            Em_PlaySeAtBoneOnFrame(em, 50, Sound_RandomSe(15, 4, 17, 4), HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 2, 0, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 60, 1, HERM_BONE_LEG_A2);
            Em_PlaySeAtBoneOnFrame(em, 52, 0, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 8, 1, HERM_BONE_LEG_B2);
            break;
        case EM_MOTION_ID(0, HERM_MOT_TURN_RIGHT):
            Em_PlaySeAtBoneVolOnFrame(em, 18, Sound_RandomSe(60, 4, 61, 4), 14, 10);
            Em_PlaySeAtBoneVolOnFrame(em, 64, Sound_RandomSe(62, 6, 60, 2), 14, 10);
            Em_PlaySeAtBoneOnFrame(em, 36, Sound_RandomSe(15, 4, 17, 4), HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 28, 0, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 40, 1, HERM_BONE_LEG_A2);
            Em_PlaySeAtBoneOnFrame(em, 72, 0, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 84, 1, HERM_BONE_LEG_A2);
            Em_PlaySeAtBoneOnFrame(em, 18, 0, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 32, 1, HERM_BONE_LEG_B2);
            Em_PlaySeAtBoneOnFrame(em, 62, 0, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 78, 1, HERM_BONE_LEG_B2);
            break;
        case EM_MOTION_ID(0, HERM_MOT_TURN_LEFT):
            Em_PlaySeAtBoneVolOnFrame(em, 18, Sound_RandomSe(60, 4, 61, 4), 14, 10);
            Em_PlaySeAtBoneVolOnFrame(em, 64, Sound_RandomSe(62, 6, 60, 2), 14, 10);
            Em_PlaySeAtBoneOnFrame(em, 36, Sound_RandomSe(15, 4, 17, 4), HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 28, 0, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 40, 1, HERM_BONE_LEG_B2);
            Em_PlaySeAtBoneOnFrame(em, 72, 0, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 84, 1, HERM_BONE_LEG_B2);
            Em_PlaySeAtBoneOnFrame(em, 18, 0, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 32, 1, HERM_BONE_LEG_A2);
            Em_PlaySeAtBoneOnFrame(em, 62, 0, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 78, 1, HERM_BONE_LEG_A2);
            break;
        case EM_MOTION_ID(0, HERM_MOT_TURN_RIGHT_ALERT):
            Em_PlaySeAtBoneVolOnFrame(em, 4, Sound_RandomSe(63, 4, 64, 4), 14, 10);
            Em_PlaySeAtBoneVolOnFrame(em, 46, Sound_RandomSe(61, 6, 63, 2), 14, 10);
            Em_PlaySeAtBoneOnFrame(em, 2, 8, HERM_BONE_CLAW_B);
            Em_PlaySeAtBoneOnFrame(em, 8, 9, HERM_BONE_CLAW_A);
            Em_PlaySeAtBoneOnFrame(em, 12, 0, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 26, 1, HERM_BONE_LEG_B2);
            Em_PlaySeAtBoneOnFrame(em, 44, 0, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 56, 1, HERM_BONE_LEG_B2);
            Em_PlaySeAtBoneOnFrame(em, 18, 0, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 30, 1, HERM_BONE_LEG_A2);
            Em_PlaySeAtBoneOnFrame(em, 52, 0, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 62, 1, HERM_BONE_LEG_A2);
            break;
        case EM_MOTION_ID(0, HERM_MOT_TURN_LEFT_ALERT):
            Em_PlaySeAtBoneVolOnFrame(em, 4, Sound_RandomSe(63, 4, 64, 4), 14, 10);
            Em_PlaySeAtBoneVolOnFrame(em, 46, Sound_RandomSe(61, 6, 63, 2), 14, 10);
            Em_PlaySeAtBoneOnFrame(em, 2, 8, HERM_BONE_CLAW_A);
            Em_PlaySeAtBoneOnFrame(em, 8, 9, HERM_BONE_CLAW_B);
            Em_PlaySeAtBoneOnFrame(em, 12, 0, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 26, 1, HERM_BONE_LEG_A2);
            Em_PlaySeAtBoneOnFrame(em, 44, 0, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 56, 1, HERM_BONE_LEG_A2);
            Em_PlaySeAtBoneOnFrame(em, 18, 0, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 30, 1, HERM_BONE_LEG_B2);
            Em_PlaySeAtBoneOnFrame(em, 54, 0, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 62, 1, HERM_BONE_LEG_B2);
            break;
        case EM_MOTION_ID(0, HERM_MOT_CEIL_HANG):
            Em_PlaySeAtBoneVolOnFrame(em, 4, Sound_RandomSe(60, 6, 61, 2), 14, 10);
            Em_PlaySeAtBoneVolOnFrame(em, 34, Sound_RandomSe(62, 6, 60, 2), 14, 10);
            Em_PlaySeAtBoneVolOnFrame(em, 90, Sound_RandomSe(61, 6, 62, 2), 14, 10);
            Em_PlaySeAtBoneVolOnFrame(em, 150, Sound_RandomSe(60, 6, 61, 2), 14, 10);
            Em_PlaySeAtBoneVolOnFrame(em, 222, Sound_RandomSe(62, 6, 60, 2), 14, 10);
            Em_PlaySeAtBoneVolOnFrame(em, 314, Sound_RandomSe(61, 6, 62, 2), 14, 10);
            break;
        case EM_MOTION_ID(0, HERM_MOT_LEAP):
            if (type == EM_KIND_SHOGUN_CEANATAUR) {
                Em_PlaySeAtBoneOnFrame(em, 44, 44, 14);
                Em_PlaySeAtBoneOnFrame(em, 110, 36, 14);
                Em_PlaySeAtBoneOnFrame(em, 110, 11, HERM_BONE_BODY);
                Em_PlaySeAtBoneOnFrame(em, 112, 18, HERM_BONE_BODY);
                Em_PlaySeAtBoneOnFrame(em, 32, 14, HERM_BONE_BODY);
                Em_PlaySeAtBoneOnFrame(em, 74, 16, HERM_BONE_BODY);
                Em_PlaySeAtBoneOnFrame(em, 36, 0, HERM_BONE_LEG_B1);
                Em_PlaySeAtBoneOnFrame(em, 82, 1, HERM_BONE_LEG_B2);
                Em_PlaySeAtBoneOnFrame(em, 98, 0, HERM_BONE_LEG_A1);
                Em_PlaySeAtBoneOnFrame(em, 50, 1, HERM_BONE_LEG_A2);
            } else {
                Em_PlaySeAtBoneOnFrame(em, 48, 44, 14);
                Em_PlaySeAtBoneOnFrame(em, 94, 36, 14);
                Em_PlaySeAtBoneOnFrame(em, 92, 11, HERM_BONE_BODY);
                Em_PlaySeAtBoneOnFrame(em, 90, 18, HERM_BONE_BODY);
                Em_PlaySeAtBoneOnFrame(em, 32, 14, HERM_BONE_BODY);
                Em_PlaySeAtBoneOnFrame(em, 58, 16, HERM_BONE_BODY);
                Em_PlaySeAtBoneOnFrame(em, 12, 0, HERM_BONE_LEG_B1);
                Em_PlaySeAtBoneOnFrame(em, 48, 1, HERM_BONE_LEG_B2);
                Em_PlaySeAtBoneOnFrame(em, 36, 0, HERM_BONE_LEG_A1);
                Em_PlaySeAtBoneOnFrame(em, 24, 1, HERM_BONE_LEG_A2);
            }
            break;
        case EM_MOTION_ID(0, HERM_MOT_LEAP_LAND):
            if (type == EM_KIND_SHOGUN_CEANATAUR) {
                Em_PlaySeAtBoneOnFrame(em, 40, Sound_RandomSe(60, 4, 61, 4), 14);
                Em_PlaySeAtBoneOnFrame(em, 110, Sound_RandomSe(60, 4, 61, 4), 14);
                Em_PlaySeAtBoneOnFrame(em, 186, Sound_RandomSe(60, 4, 61, 4), 14);
                Em_PlaySeAtBoneVolOnFrame(em, 8, 101, HERM_BONE_BODY, 22);
                Em_PlaySeAtBoneOnFrame(em, 6, 19, HERM_BONE_BODY);
                Em_PlaySeAtBoneOnFrame(em, 116, 0, HERM_BONE_LEG_B1);
                Em_PlaySeAtBoneOnFrame(em, 158, 0, HERM_BONE_LEG_A1);
                Em_PlaySeAtBoneOnFrame(em, 132, 1, HERM_BONE_LEG_A2);
                Em_PlaySeAtBoneOnFrame(em, 174, 1, HERM_BONE_LEG_B2);
                Em_PlaySeAtBoneOnFrame(em, 194, 20, HERM_BONE_CLAW_B);
                Em_PlaySeAtBoneOnFrame(em, 196, 29, HERM_BONE_CLAW_A);
                Em_PlaySeAtBoneOnFrame(em, 62, 16, HERM_BONE_BODY);
                Em_PlaySeAtBoneOnFrame(em, 88, 17, HERM_BONE_BODY);
                Em_PlaySeAtBoneOnFrame(em, 158, 14, HERM_BONE_BODY);
            } else {
                Em_PlaySeAtBoneOnFrame(em, 54, Sound_RandomSe(60, 4, 61, 4), 14);
                Em_PlaySeAtBoneOnFrame(em, 6, 19, HERM_BONE_BODY);
                Em_PlaySeAtBoneOnFrame(em, 4, 18, HERM_BONE_BODY);
                Em_PlaySeAtBoneOnFrame(em, 8, 27, HERM_BONE_BODY);
                Em_PlaySeAtBoneVolOnFrame(em, 10, 101, HERM_BONE_BODY, 22);
                Em_PlaySeAtBoneOnFrame(em, 136, 14, HERM_BONE_BODY);
                Em_PlaySeAtBoneOnFrame(em, 182, 4, HERM_BONE_LEG_A1);
                Em_PlaySeAtBoneOnFrame(em, 166, 5, HERM_BONE_LEG_A2);
                Em_PlaySeAtBoneOnFrame(em, 148, 2, HERM_BONE_LEG_B1);
                Em_PlaySeAtBoneOnFrame(em, 196, 5, HERM_BONE_LEG_B2);
                Em_PlaySeAtBoneOnFrame(em, 98, 21, HERM_BONE_CLAW_B);
                Em_PlaySeAtBoneOnFrame(em, 130, 4, HERM_BONE_CLAW_A);
            }
            break;
        case EM_MOTION_ID(0, HERM_MOT_CEIL_CRAWL):
            Em_PlaySeAtBoneVolOnFrame(em, 22, Sound_RandomSe(60, 4, 61, 4), 14, 10);
            Em_PlaySeAtBoneVolOnFrame(em, 64, Sound_RandomSe(62, 4, 60, 4), 14, 10);
            Em_PlaySeAtBoneOnFrame(em, 10, Sound_RandomSe(16, 4, 17, 4), HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 40, Sound_RandomSe(16, 4, 17, 4), HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 34, 1, HERM_BONE_LEG_B2);
            Em_PlaySeAtBoneOnFrame(em, 66, 1, HERM_BONE_LEG_A2);
            Em_PlaySeAtBoneOnFrame(em, 62, 0, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 32, 0, HERM_BONE_LEG_A1);
            break;
        case EM_MOTION_ID(0, 13):
            Em_PlaySeAtBoneVolOnFrame(em, 8, Sound_RandomSe(60, 4, 61, 4), 14, 10);
            Em_PlaySeAtBoneVolOnFrame(em, 72, Sound_RandomSe(62, 6, 60, 2), 14, 10);
            Em_PlaySeAtBoneOnFrame(em, 30, Sound_RandomSe(16, 4, 17, 4), HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 50, Sound_RandomSe(16, 4, 17, 4), HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 86, Sound_RandomSe(16, 4, 17, 4), HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 18, 0, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 46, 0, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 72, 0, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 96, 0, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 30, 1, HERM_BONE_LEG_B2);
            Em_PlaySeAtBoneOnFrame(em, 52, 1, HERM_BONE_LEG_A2);
            Em_PlaySeAtBoneOnFrame(em, 84, 1, HERM_BONE_LEG_B2);
            Em_PlaySeAtBoneOnFrame(em, 106, 1, HERM_BONE_LEG_A2);
            break;
        case EM_MOTION_ID(0, 14):
            Em_PlaySeAtBoneVolOnFrame(em, 8, Sound_RandomSe(60, 4, 61, 4), 14, 10);
            Em_PlaySeAtBoneVolOnFrame(em, 72, Sound_RandomSe(62, 6, 60, 2), 14, 10);
            Em_PlaySeAtBoneOnFrame(em, 30, Sound_RandomSe(16, 4, 17, 4), HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 50, Sound_RandomSe(16, 4, 17, 4), HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 86, Sound_RandomSe(16, 4, 17, 4), HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 18, 0, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 46, 0, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 72, 0, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 96, 0, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 30, 1, HERM_BONE_LEG_A2);
            Em_PlaySeAtBoneOnFrame(em, 52, 1, HERM_BONE_LEG_B2);
            Em_PlaySeAtBoneOnFrame(em, 84, 1, HERM_BONE_LEG_A2);
            Em_PlaySeAtBoneOnFrame(em, 106, 1, HERM_BONE_LEG_B2);
            break;
        case EM_MOTION_ID(0, 15):
            Em_PlaySeAtBoneVolOnFrame(em, 4, Sound_RandomSe(60, 3, 61, 3), 14, 10);
            Em_PlaySeAtBoneOnFrame(em, 52, 0, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 10, 1, HERM_BONE_LEG_A2);
            Em_PlaySeAtBoneOnFrame(em, 2, 0, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 52, 1, HERM_BONE_LEG_B2);
            break;
        case EM_MOTION_ID(0, 16):
            Em_PlaySeAtBoneVolOnFrame(em, 4, Sound_RandomSe(60, 3, 61, 3), 14, 10);
            Em_PlaySeAtBoneOnFrame(em, 52, 0, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 10, 1, HERM_BONE_LEG_B2);
            Em_PlaySeAtBoneOnFrame(em, 2, 0, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 52, 1, HERM_BONE_LEG_A2);
            break;
        case EM_MOTION_ID(0, 18):
            Em_PlaySeAtBoneVolOnFrame(em, 4, 30, 14, 10);
            Em_PlaySeAtBoneOnFrame(em, 4, 17, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 22, 9, HERM_BONE_CLAW_B);
            break;
        case EM_MOTION_ID(0, HERM_MOT_STRIKE_C):
            if (type == EM_KIND_SHOGUN_CEANATAUR) {
                Em_PlaySeAtBoneVolOnFrame(em, 4, 31, 14, 10);
                Em_PlaySeAtBoneOnFrame(em, 12, 12, HERM_BONE_CLAW_B);
                Em_PlaySeAtBoneOnFrame(em, 20, 29, HERM_BONE_CLAW_B);
                Em_PlaySeAtBoneOnFrame(em, 24, 22, HERM_BONE_CLAW_B);
                Em_PlaySeAtBoneOnFrame(em, 58, 16, HERM_BONE_BODY);
                Em_PlaySeAtBoneOnFrame(em, 110, 17, HERM_BONE_CLAW_B);
            } else {
                Em_PlaySeAtBoneVolOnFrame(em, 4, 31, 14, 10);
                Em_PlaySeAtBoneOnFrame(em, 12, 12, HERM_BONE_CLAW_B);
                Em_PlaySeAtBoneOnFrame(em, 20, 29, HERM_BONE_CLAW_B);
                Em_PlaySeAtBoneOnFrame(em, 58, 16, HERM_BONE_BODY);
                Em_PlaySeAtBoneOnFrame(em, 88, 9, HERM_BONE_CLAW_A);
                Em_PlaySeAtBoneOnFrame(em, 110, 17, HERM_BONE_CLAW_B);
            }
            break;
        case EM_MOTION_ID(0, HERM_MOT_SIDESTEP_A):
            Em_PlaySeAtBoneOnFrame(em, 22, 2, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 6, 3, HERM_BONE_LEG_A2);
            Em_PlaySeAtBoneOnFrame(em, 24, 3, HERM_BONE_LEG_B2);
            Em_PlaySeAtBoneOnFrame(em, 2, 2, HERM_BONE_LEG_B1);
            break;
        case EM_MOTION_ID(0, HERM_MOT_STRIKE_B):
            if (type == EM_KIND_SHOGUN_CEANATAUR) {
                Em_PlaySeAtBoneOnFrame(em, 4, 32, 14);
                Em_PlaySeAtBoneOnFrame(em, 88, 16, HERM_BONE_BODY);
                Em_PlaySeAtBoneOnFrame(em, 12, 12, HERM_BONE_CLAW_B);
                Em_PlaySeAtBoneOnFrame(em, 22, 29, HERM_BONE_CLAW_B);
                Em_PlaySeAtBoneOnFrame(em, 26, 22, HERM_BONE_CLAW_B);
                Em_PlaySeAtBoneOnFrame(em, 40, 2, HERM_BONE_LEG_B1);
                Em_PlaySeAtBoneOnFrame(em, 84, 0, HERM_BONE_LEG_B1);
                Em_PlaySeAtBoneOnFrame(em, 22, 2, HERM_BONE_LEG_A1);
                Em_PlaySeAtBoneOnFrame(em, 60, 0, HERM_BONE_LEG_A1);
                Em_PlaySeAtBoneOnFrame(em, 46, 1, HERM_BONE_LEG_A2);
                Em_PlaySeAtBoneOnFrame(em, 102, 5, HERM_BONE_LEG_A2);
                Em_PlaySeAtBoneOnFrame(em, 22, 3, HERM_BONE_LEG_B2);
                Em_PlaySeAtBoneOnFrame(em, 68, 1, HERM_BONE_LEG_B2);
            } else {
                Em_PlaySeAtBoneOnFrame(em, 4, 32, 14);
                Em_PlaySeAtBoneOnFrame(em, 88, 16, HERM_BONE_BODY);
                Em_PlaySeAtBoneOnFrame(em, 12, 12, HERM_BONE_CLAW_B);
                Em_PlaySeAtBoneOnFrame(em, 22, 29, HERM_BONE_CLAW_B);
                Em_PlaySeAtBoneOnFrame(em, 26, 9, HERM_BONE_CLAW_A);
                Em_PlaySeAtBoneOnFrame(em, 88, 8, HERM_BONE_CLAW_A);
                Em_PlaySeAtBoneOnFrame(em, 116, 9, HERM_BONE_CLAW_B);
                Em_PlaySeAtBoneOnFrame(em, 40, 2, HERM_BONE_LEG_B1);
                Em_PlaySeAtBoneOnFrame(em, 84, 0, HERM_BONE_LEG_B1);
                Em_PlaySeAtBoneOnFrame(em, 22, 2, HERM_BONE_LEG_A1);
                Em_PlaySeAtBoneOnFrame(em, 60, 0, HERM_BONE_LEG_A1);
                Em_PlaySeAtBoneOnFrame(em, 46, 1, HERM_BONE_LEG_A2);
                Em_PlaySeAtBoneOnFrame(em, 102, 5, HERM_BONE_LEG_A2);
                Em_PlaySeAtBoneOnFrame(em, 22, 3, HERM_BONE_LEG_B2);
                Em_PlaySeAtBoneOnFrame(em, 68, 1, HERM_BONE_LEG_B2);
            }
            break;
        case EM_MOTION_ID(0, HERM_MOT_SIDESTEP_B):
            Em_PlaySeAtBoneOnFrame(em, 22, 2, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 6, 3, HERM_BONE_LEG_B2);
            Em_PlaySeAtBoneOnFrame(em, 24, 3, HERM_BONE_LEG_A2);
            Em_PlaySeAtBoneOnFrame(em, 2, 2, HERM_BONE_LEG_A1);
            break;
        case EM_MOTION_ID(0, HERM_MOT_STRIKE_B_MIRROR):
            if (type == EM_KIND_SHOGUN_CEANATAUR) {
                Em_PlaySeAtBoneOnFrame(em, 4, 32, 14);
                Em_PlaySeAtBoneOnFrame(em, 88, 16, HERM_BONE_BODY);
                Em_PlaySeAtBoneOnFrame(em, 12, 12, HERM_BONE_CLAW_A);
                Em_PlaySeAtBoneOnFrame(em, 22, 29, HERM_BONE_CLAW_A);
                Em_PlaySeAtBoneOnFrame(em, 26, 22, HERM_BONE_CLAW_A);
                Em_PlaySeAtBoneOnFrame(em, 40, 2, HERM_BONE_LEG_A1);
                Em_PlaySeAtBoneOnFrame(em, 84, 0, HERM_BONE_LEG_A1);
                Em_PlaySeAtBoneOnFrame(em, 22, 2, HERM_BONE_LEG_B1);
                Em_PlaySeAtBoneOnFrame(em, 60, 0, HERM_BONE_LEG_B1);
                Em_PlaySeAtBoneOnFrame(em, 46, 1, HERM_BONE_LEG_B2);
                Em_PlaySeAtBoneOnFrame(em, 102, 5, HERM_BONE_LEG_B2);
                Em_PlaySeAtBoneOnFrame(em, 22, 3, HERM_BONE_LEG_A2);
                Em_PlaySeAtBoneOnFrame(em, 68, 1, HERM_BONE_LEG_A2);
            } else {
                Em_PlaySeAtBoneOnFrame(em, 4, 32, 14);
                Em_PlaySeAtBoneOnFrame(em, 88, 16, HERM_BONE_BODY);
                Em_PlaySeAtBoneOnFrame(em, 12, 12, HERM_BONE_CLAW_A);
                Em_PlaySeAtBoneOnFrame(em, 22, 29, HERM_BONE_CLAW_A);
                Em_PlaySeAtBoneOnFrame(em, 26, 9, HERM_BONE_CLAW_B);
                Em_PlaySeAtBoneOnFrame(em, 88, 8, HERM_BONE_CLAW_B);
                Em_PlaySeAtBoneOnFrame(em, 116, 9, HERM_BONE_CLAW_A);
                Em_PlaySeAtBoneOnFrame(em, 40, 2, HERM_BONE_LEG_A1);
                Em_PlaySeAtBoneOnFrame(em, 84, 0, HERM_BONE_LEG_A1);
                Em_PlaySeAtBoneOnFrame(em, 22, 2, HERM_BONE_LEG_B1);
                Em_PlaySeAtBoneOnFrame(em, 60, 0, HERM_BONE_LEG_B1);
                Em_PlaySeAtBoneOnFrame(em, 46, 1, HERM_BONE_LEG_B2);
                Em_PlaySeAtBoneOnFrame(em, 102, 5, HERM_BONE_LEG_B2);
                Em_PlaySeAtBoneOnFrame(em, 22, 3, HERM_BONE_LEG_A2);
                Em_PlaySeAtBoneOnFrame(em, 68, 1, HERM_BONE_LEG_A2);
            }
            break;
        case EM_MOTION_ID(0, 24):
            Em_PlaySeAtBoneOnFrame(em, 4, 30, 14);
            Em_PlaySeAtBoneOnFrame(em, 4, 17, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 22, 9, HERM_BONE_CLAW_A);
            break;
        case EM_MOTION_ID(0, HERM_MOT_STRIKE_C_MIRROR):
            if (type == EM_KIND_SHOGUN_CEANATAUR) {
                Em_PlaySeAtBoneVolOnFrame(em, 4, 31, 14, 10);
                Em_PlaySeAtBoneOnFrame(em, 12, 12, HERM_BONE_CLAW_A);
                Em_PlaySeAtBoneOnFrame(em, 20, 29, HERM_BONE_CLAW_A);
                Em_PlaySeAtBoneOnFrame(em, 24, 22, HERM_BONE_CLAW_A);
                Em_PlaySeAtBoneOnFrame(em, 58, 16, HERM_BONE_BODY);
                Em_PlaySeAtBoneOnFrame(em, 110, 17, HERM_BONE_CLAW_A);
            } else {
                Em_PlaySeAtBoneVolOnFrame(em, 4, 31, 14, 10);
                Em_PlaySeAtBoneOnFrame(em, 12, 12, HERM_BONE_CLAW_A);
                Em_PlaySeAtBoneOnFrame(em, 20, 29, HERM_BONE_CLAW_A);
                Em_PlaySeAtBoneOnFrame(em, 58, 16, HERM_BONE_BODY);
                Em_PlaySeAtBoneOnFrame(em, 88, 9, HERM_BONE_CLAW_B);
                Em_PlaySeAtBoneOnFrame(em, 110, 17, HERM_BONE_CLAW_A);
            }
            break;
        case EM_MOTION_ID(0, HERM_MOT_LIE):
            Em_PlaySeAtBoneOnFrame(em, 4, 33, 14);
            Em_PlaySeAtBoneVolOnFrame(em, 140, Sound_RandomSe(67, 6, 66, 2), 14, 10);
            Em_PlaySeAtBoneVolOnFrame(em, 188, Sound_RandomSe(66, 6, 67, 2), 14, 10);
            break;
        case EM_MOTION_ID(0, HERM_MOT_GET_UP):
            Em_PlaySeAtBoneOnFrame(em, 12, 34, 14);
            Em_PlaySeAtBoneOnFrame(em, 58, 14, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 102, 13, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 126, 8, HERM_BONE_CLAW_B);
            Em_PlaySeAtBoneOnFrame(em, 128, 9, HERM_BONE_CLAW_A);
            break;
        case EM_MOTION_ID(0, HERM_MOT_LIE_DOWN):
            Em_PlaySeAtBoneVolOnFrame(em, 10, Sound_RandomSe(66, 6, 67, 2), 14, 10);
            Em_PlaySeAtBoneOnFrame(em, 78, 33, 14);
            Em_PlaySeAtBoneOnFrame(em, 16, 14, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 98, 14, HERM_BONE_BODY);
            break;
        case EM_MOTION_ID(0, HERM_MOT_EMERGE):
            if (type == EM_KIND_SHOGUN_CEANATAUR) {
                Em_PlaySeAtBoneOnFrame(em, 94, 44, 14);
                Em_PlaySeAtBoneOnFrame(em, 4, 27, HERM_BONE_BODY);
                Em_PlaySeAtBoneOnFrame(em, 2, 26, HERM_BONE_BODY);
                Em_PlaySeAtBoneVolOnFrame(em, 4, 101, HERM_BONE_BODY, 22);
                Em_PlaySeAtBoneOnFrame(em, 16, 22, HERM_BONE_CLAW_B);
                Em_PlaySeAtBoneOnFrame(em, 28, 22, HERM_BONE_CLAW_A);
                Em_PlaySeAtBoneOnFrame(em, 26, 29, HERM_BONE_CLAW_B);
                Em_PlaySeAtBoneOnFrame(em, 34, 29, HERM_BONE_CLAW_A);
                Em_PlaySeAtBoneOnFrame(em, 92, 6, HERM_BONE_LEG_B1);
                Em_PlaySeAtBoneOnFrame(em, 100, 4, HERM_BONE_LEG_A1);
                Em_PlaySeAtBoneOnFrame(em, 136, 3, HERM_BONE_LEG_A2);
                Em_PlaySeAtBoneOnFrame(em, 144, 1, HERM_BONE_LEG_B2);
                Em_PlaySeAtBoneOnFrame(em, 134, 0, HERM_BONE_LEG_B1);
                Em_PlaySeAtBoneOnFrame(em, 156, 0, HERM_BONE_LEG_A1);
                Em_PlaySeAtBoneOnFrame(em, 180, 1, HERM_BONE_LEG_A2);
                Em_PlaySeAtBoneOnFrame(em, 42, 15, HERM_BONE_BODY);
                Em_PlaySeAtBoneOnFrame(em, 66, 15, HERM_BONE_BODY);
                Em_PlaySeAtBoneOnFrame(em, 88, 15, HERM_BONE_BODY);
                Em_PlaySeAtBoneOnFrame(em, 158, 14, HERM_BONE_BODY);
                Em_PlaySeAtBoneOnFrame(em, 82, 25, HERM_BONE_BODY);
                Em_PlaySeAtBoneOnFrame(em, 122, 26, HERM_BONE_BODY);
                Em_PlaySeAtBoneOnFrame(em, 118, 22, HERM_BONE_CLAW_B);
                Em_PlaySeAtBoneOnFrame(em, 130, 22, HERM_BONE_CLAW_A);
                Em_PlaySeAtBoneOnFrame(em, 122, 29, HERM_BONE_CLAW_B);
                Em_PlaySeAtBoneOnFrame(em, 138, 29, HERM_BONE_CLAW_A);
            } else {
                Em_PlaySeAtBoneOnFrame(em, 126, 44, 14);
                Em_PlaySeAtBoneOnFrame(em, 10, 26, HERM_BONE_BODY);
                Em_PlaySeAtBoneVolOnFrame(em, 2, 101, HERM_BONE_BODY, 22);
                Em_PlaySeAtBoneOnFrame(em, 8, 15, HERM_BONE_BODY);
                Em_PlaySeAtBoneOnFrame(em, 42, 15, HERM_BONE_BODY);
                Em_PlaySeAtBoneOnFrame(em, 66, 15, HERM_BONE_BODY);
                Em_PlaySeAtBoneOnFrame(em, 88, 15, HERM_BONE_BODY);
                Em_PlaySeAtBoneOnFrame(em, 158, 14, HERM_BONE_BODY);
                Em_PlaySeAtBoneOnFrame(em, 82, 25, HERM_BONE_CLAW_B);
                Em_PlaySeAtBoneOnFrame(em, 122, 25, HERM_BONE_CLAW_B);
                Em_PlaySeAtBoneOnFrame(em, 98, 7, HERM_BONE_LEG_B2);
                Em_PlaySeAtBoneOnFrame(em, 118, 6, HERM_BONE_LEG_B1);
                Em_PlaySeAtBoneOnFrame(em, 168, 1, HERM_BONE_LEG_B2);
                Em_PlaySeAtBoneOnFrame(em, 198, 0, HERM_BONE_LEG_B1);
            }
            break;
        case EM_MOTION_ID(0, HERM_MOT_CEIL_JET):
            Em_PlaySeAtBoneOnFrame(em, 6, 35, 14);
            Em_PlaySeAtBoneOnFrame(em, 6, 14, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 46, 16, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 72, 16, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 90, 16, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 104, 17, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 120, 16, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 136, 16, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 154, 17, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 168, 15, HERM_BONE_BODY);
            break;
        case EM_MOTION_ID(0, HERM_MOT_BACK_ATTACK_WALK):
            Em_PlaySeAtBoneVolOnFrame(em, 4, Sound_RandomSe(63, 4, 64, 4), 14, 10);
            Em_PlaySeAtBoneOnFrame(em, 2, 2, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 18, 2, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 36, 3, HERM_BONE_LEG_A2);
            Em_PlaySeAtBoneOnFrame(em, 16, 3, HERM_BONE_LEG_B2);
            break;
        case EM_MOTION_ID(0, HERM_MOT_WATER_JET_END):
            Em_PlaySeAtBoneVolOnFrame(em, 4, Sound_RandomSe(60, 6, 61, 2), 14, 10);
            Em_PlaySeAtBoneOnFrame(em, 36, 0, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 42, 17, HERM_BONE_BODY);
            break;
        case EM_MOTION_ID(0, HERM_MOT_WATER_JET):
            Em_PlaySeAtBoneOnFrame(em, 4, 35, 14);
            Em_PlaySeAtBoneOnFrame(em, 78, 36, 14);
            Em_PlaySeAtBoneOnFrame(em, 80, 24, 14);
            Em_PlaySeAtBoneOnFrame(em, 4, 15, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 42, 16, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 18, 9, HERM_BONE_CLAW_B);
            Em_PlaySeAtBoneOnFrame(em, 24, 13, HERM_BONE_CLAW_A);
            Em_PlaySeAtBoneOnFrame(em, 18, 1, HERM_BONE_LEG_B2);
            Em_PlaySeAtBoneOnFrame(em, 36, 0, HERM_BONE_LEG_A1);
            break;
        case EM_MOTION_ID(0, HERM_MOT_GUARD):
            if (type == EM_KIND_SHOGUN_CEANATAUR) {
                Em_PlaySeAtBoneOnFrame(em, 8, 53, 14);
                Em_PlaySeAtBoneOnFrame(em, 60, 43, 14);
                Em_PlaySeAtBoneVolOnFrame(em, 150, Sound_RandomSe(60, 4, 61, 4), 14, 10);
                Em_PlaySeAtBoneOnFrame(em, 4, 16, HERM_BONE_BODY);
                Em_PlaySeAtBoneOnFrame(em, 126, 14, HERM_BONE_BODY);
                Em_PlaySeAtBoneOnFrame(em, 62, 12, HERM_BONE_CLAW_B);
                Em_PlaySeAtBoneOnFrame(em, 66, 22, HERM_BONE_CLAW_B);
                Em_PlaySeAtBoneOnFrame(em, 70, 18, HERM_BONE_CLAW_B);
                Em_PlaySeAtBoneOnFrame(em, 12, 2, HERM_BONE_LEG_B1);
                Em_PlaySeAtBoneOnFrame(em, 46, 2, HERM_BONE_LEG_A1);
                Em_PlaySeAtBoneOnFrame(em, 32, 3, HERM_BONE_LEG_B2);
                Em_PlaySeAtBoneOnFrame(em, 24, 3, HERM_BONE_LEG_A2);
            } else {
                Em_PlaySeAtBoneOnFrame(em, 4, 54, 14);
                Em_PlaySeAtBoneOnFrame(em, 56, 14, HERM_BONE_BODY);
                Em_PlaySeAtBoneOnFrame(em, 50, 29, HERM_BONE_CLAW_B);
                Em_PlaySeAtBoneOnFrame(em, 48, 9, HERM_BONE_CLAW_B);
                Em_PlaySeAtBoneOnFrame(em, 48, 8, HERM_BONE_CLAW_A);
                Em_PlaySeAtBoneOnFrame(em, 12, 2, HERM_BONE_LEG_B1);
                Em_PlaySeAtBoneOnFrame(em, 50, 6, HERM_BONE_LEG_B1);
                Em_PlaySeAtBoneOnFrame(em, 66, 3, HERM_BONE_LEG_B2);
                Em_PlaySeAtBoneOnFrame(em, 36, 3, HERM_BONE_LEG_A2);
                Em_PlaySeAtBoneOnFrame(em, 56, 6, HERM_BONE_LEG_A1);
            }
            break;
        case EM_MOTION_ID(0, HERM_MOT_GUARD_END):
            if (type == EM_KIND_SHOGUN_CEANATAUR) {
                Em_PlaySeAtBoneOnFrame(em, 8, 53, 14);
                Em_PlaySeAtBoneOnFrame(em, 60, 43, 14);
                Em_PlaySeAtBoneVolOnFrame(em, 150, Sound_RandomSe(60, 4, 61, 4), 14, 10);
                Em_PlaySeAtBoneOnFrame(em, 4, 16, HERM_BONE_BODY);
                Em_PlaySeAtBoneOnFrame(em, 126, 14, HERM_BONE_BODY);
                Em_PlaySeAtBoneOnFrame(em, 62, 12, HERM_BONE_CLAW_A);
                Em_PlaySeAtBoneOnFrame(em, 66, 22, HERM_BONE_CLAW_A);
                Em_PlaySeAtBoneOnFrame(em, 70, 18, HERM_BONE_CLAW_A);
                Em_PlaySeAtBoneOnFrame(em, 12, 2, HERM_BONE_LEG_A1);
                Em_PlaySeAtBoneOnFrame(em, 46, 2, HERM_BONE_LEG_B1);
                Em_PlaySeAtBoneOnFrame(em, 32, 3, HERM_BONE_LEG_A2);
                Em_PlaySeAtBoneOnFrame(em, 24, 3, HERM_BONE_LEG_B2);
            } else {
                Em_PlaySeAtBoneOnFrame(em, 8, 53, 14);
                Em_PlaySeAtBoneOnFrame(em, 138, 42, 14);
                Em_PlaySeAtBoneOnFrame(em, 4, 21, HERM_BONE_CLAW_B);
                Em_PlaySeAtBoneOnFrame(em, 80, 14, HERM_BONE_BODY);
                Em_PlaySeAtBoneOnFrame(em, 118, 2, HERM_BONE_LEG_B1);
                Em_PlaySeAtBoneOnFrame(em, 140, 3, HERM_BONE_LEG_B2);
                Em_PlaySeAtBoneOnFrame(em, 130, 2, HERM_BONE_LEG_A2);
                Em_PlaySeAtBoneOnFrame(em, 150, 0, HERM_BONE_LEG_A1);
            }
            break;
        case EM_MOTION_ID(0, HERM_MOT_STRIKE_A):
            if (type == EM_KIND_SHOGUN_CEANATAUR) {
                Em_PlaySeAtBoneOnFrame(em, 2, 43, 14);
                Em_PlaySeAtBoneOnFrame(em, 44, 31, 14);
                Em_PlaySeAtBoneOnFrame(em, 4, 14, HERM_BONE_BODY);
                Em_PlaySeAtBoneOnFrame(em, 44, 22, HERM_BONE_CLAW_B);
                Em_PlaySeAtBoneOnFrame(em, 52, 22, HERM_BONE_CLAW_A);
                Em_PlaySeAtBoneOnFrame(em, 50, 29, HERM_BONE_CLAW_B);
                Em_PlaySeAtBoneOnFrame(em, 56, 29, HERM_BONE_CLAW_A);
            } else {
                Em_PlaySeAtBoneOnFrame(em, 2, 43, 14);
                Em_PlaySeAtBoneOnFrame(em, 44, 31, 14);
                Em_PlaySeAtBoneOnFrame(em, 4, 14, HERM_BONE_BODY);
                Em_PlaySeAtBoneOnFrame(em, 44, 22, HERM_BONE_CLAW_B);
            }
            break;
        case EM_MOTION_ID(0, HERM_MOT_WINDUP_STRIKE):
            if (type == EM_KIND_SHOGUN_CEANATAUR) {
                Em_PlaySeAtBoneOnFrame(em, 20, 32, 14);
                Em_PlaySeAtBoneOnFrame(em, 12, 22, HERM_BONE_CLAW_B);
                Em_PlaySeAtBoneOnFrame(em, 22, 22, HERM_BONE_CLAW_A);
                Em_PlaySeAtBoneOnFrame(em, 20, 13, HERM_BONE_CLAW_B);
                Em_PlaySeAtBoneOnFrame(em, 30, 13, HERM_BONE_CLAW_A);
                Em_PlaySeAtBoneOnFrame(em, 28, 20, HERM_BONE_BODY);
                Em_PlaySeAtBoneOnFrame(em, 60, 14, HERM_BONE_BODY);
                Em_PlaySeAtBoneOnFrame(em, 60, 16, HERM_BONE_BODY);
                Em_PlaySeAtBoneOnFrame(em, 43, 0, HERM_BONE_LEG_A1);
                Em_PlaySeAtBoneOnFrame(em, 100, 1, HERM_BONE_LEG_B2);
                Em_PlaySeAtBoneOnFrame(em, 130, 0, HERM_BONE_LEG_B1);
            } else {
                Em_PlaySeAtBoneOnFrame(em, 20, 32, 14);
                Em_PlaySeAtBoneOnFrame(em, 12, 22, HERM_BONE_CLAW_B);
                Em_PlaySeAtBoneOnFrame(em, 22, 22, HERM_BONE_CLAW_A);
                Em_PlaySeAtBoneOnFrame(em, 20, 13, HERM_BONE_CLAW_B);
                Em_PlaySeAtBoneOnFrame(em, 30, 13, HERM_BONE_CLAW_A);
                Em_PlaySeAtBoneOnFrame(em, 28, 20, HERM_BONE_BODY);
                Em_PlaySeAtBoneOnFrame(em, 60, 14, HERM_BONE_BODY);
                Em_PlaySeAtBoneOnFrame(em, 60, 16, HERM_BONE_BODY);
                Em_PlaySeAtBoneOnFrame(em, 94, 0, HERM_BONE_LEG_A1);
                Em_PlaySeAtBoneOnFrame(em, 108, 1, HERM_BONE_LEG_B2);
                Em_PlaySeAtBoneOnFrame(em, 126, 0, HERM_BONE_LEG_B1);
                Em_PlaySeAtBoneOnFrame(em, 134, 1, HERM_BONE_LEG_A2);
            }
            break;
        case EM_MOTION_ID(0, HERM_MOT_WINDUP):
            if (type == EM_KIND_SHOGUN_CEANATAUR) {
                Em_PlaySeAtBoneOnFrame(em, 41, 31, 14);
                Em_PlaySeAtBoneOnFrame(em, 44, 14, HERM_BONE_BODY);
                Em_PlaySeAtBoneOnFrame(em, 38, 5, HERM_BONE_LEG_A2);
            } else {
                Em_PlaySeAtBoneOnFrame(em, 41, 31, 14);
                Em_PlaySeAtBoneOnFrame(em, 44, 14, HERM_BONE_BODY);
                Em_PlaySeAtBoneOnFrame(em, 12, 2, HERM_BONE_LEG_A1);
                Em_PlaySeAtBoneOnFrame(em, 12, 1, HERM_BONE_LEG_B2);
                Em_PlaySeAtBoneOnFrame(em, 46, 0, HERM_BONE_LEG_B1);
                Em_PlaySeAtBoneOnFrame(em, 32, 1, HERM_BONE_LEG_A2);
            }
            break;
        case EM_MOTION_ID(0, HERM_MOT_WALK_ALERT):
            if (type == EM_KIND_SHOGUN_CEANATAUR) {
                Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(60, 4, 61, 4), 14);
                Em_PlaySeAtBoneOnFrame(em, 18, 2, HERM_BONE_LEG_A1);
                Em_PlaySeAtBoneOnFrame(em, 36, 3, HERM_BONE_LEG_A2);
                Em_PlaySeAtBoneOnFrame(em, 14, 3, HERM_BONE_LEG_B2);
                Em_PlaySeAtBoneOnFrame(em, 2, 2, HERM_BONE_LEG_B1);
            } else {
                Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(60, 4, 61, 4), 14);
                Em_PlaySeAtBoneOnFrame(em, 40, Sound_RandomSe(61, 4, 62, 4), 14);
                Em_PlaySeAtBoneOnFrame(em, 28, 2, HERM_BONE_LEG_A1);
                Em_PlaySeAtBoneOnFrame(em, 58, 3, HERM_BONE_LEG_A2);
                Em_PlaySeAtBoneOnFrame(em, 24, 3, HERM_BONE_LEG_B2);
                Em_PlaySeAtBoneOnFrame(em, 64, 2, HERM_BONE_LEG_B1);
            }
            break;
        case EM_MOTION_ID(0, HERM_MOT_WALK_BACK_ALERT):
            if (type == EM_KIND_SHOGUN_CEANATAUR) {
                Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(63, 4, 64, 4), 14);
                Em_PlaySeAtBoneOnFrame(em, 30, Sound_RandomSe(64, 4, 61, 4), 14);
                Em_PlaySeAtBoneOnFrame(em, 2, 2, HERM_BONE_LEG_A1);
                Em_PlaySeAtBoneOnFrame(em, 42, 3, HERM_BONE_LEG_A2);
                Em_PlaySeAtBoneOnFrame(em, 4, 3, HERM_BONE_LEG_B2);
                Em_PlaySeAtBoneOnFrame(em, 36, 2, HERM_BONE_LEG_B1);
            } else {
                Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(63, 4, 64, 4), 14);
                Em_PlaySeAtBoneOnFrame(em, 40, Sound_RandomSe(64, 4, 61, 4), 14);
                Em_PlaySeAtBoneOnFrame(em, 2, 2, HERM_BONE_LEG_A1);
                Em_PlaySeAtBoneOnFrame(em, 24, 3, HERM_BONE_LEG_A2);
                Em_PlaySeAtBoneOnFrame(em, 4, 3, HERM_BONE_LEG_B2);
                Em_PlaySeAtBoneOnFrame(em, 22, 2, HERM_BONE_LEG_B1);
            }
            break;
        case EM_MOTION_ID(0, HERM_MOT_SPIN_A):
            if (type == EM_KIND_SHOGUN_CEANATAUR) {
                Em_PlaySeAtBoneOnFrame(em, 26, 44, 14);
                Em_PlaySeAtBoneOnFrame(em, 74, 31, 14);
                Em_PlaySeAtBoneOnFrame(em, 78, 12, HERM_BONE_CLAW_A);
                Em_PlaySeAtBoneOnFrame(em, 84, 22, HERM_BONE_CLAW_A);
                Em_PlaySeAtBoneOnFrame(em, 82, 29, HERM_BONE_CLAW_A);
                Em_PlaySeAtBoneOnFrame(em, 48, 2, HERM_BONE_LEG_A1);
                Em_PlaySeAtBoneOnFrame(em, 62, 3, HERM_BONE_LEG_A2);
                Em_PlaySeAtBoneOnFrame(em, 78, 2, HERM_BONE_LEG_A1);
                Em_PlaySeAtBoneOnFrame(em, 62, 3, HERM_BONE_LEG_A2);
                Em_PlaySeAtBoneOnFrame(em, 52, 2, HERM_BONE_LEG_B2);
                Em_PlaySeAtBoneOnFrame(em, 60, 3, HERM_BONE_LEG_B1);
                Em_PlaySeAtBoneOnFrame(em, 76, 2, HERM_BONE_LEG_B2);
                Em_PlaySeAtBoneOnFrame(em, 84, 3, HERM_BONE_LEG_B1);
            } else {
                Em_PlaySeAtBoneOnFrame(em, 26, 44, 14);
                Em_PlaySeAtBoneOnFrame(em, 74, 31, 14);
                Em_PlaySeAtBoneOnFrame(em, 80, 22, HERM_BONE_CLAW_A);
                Em_PlaySeAtBoneOnFrame(em, 106, 13, HERM_BONE_CLAW_A);
                Em_PlaySeAtBoneOnFrame(em, 48, 2, HERM_BONE_LEG_A1);
                Em_PlaySeAtBoneOnFrame(em, 62, 3, HERM_BONE_LEG_A2);
                Em_PlaySeAtBoneOnFrame(em, 78, 2, HERM_BONE_LEG_A1);
                Em_PlaySeAtBoneOnFrame(em, 62, 3, HERM_BONE_LEG_A2);
                Em_PlaySeAtBoneOnFrame(em, 52, 2, HERM_BONE_LEG_B2);
                Em_PlaySeAtBoneOnFrame(em, 60, 3, HERM_BONE_LEG_B1);
                Em_PlaySeAtBoneOnFrame(em, 76, 2, HERM_BONE_LEG_B2);
                Em_PlaySeAtBoneOnFrame(em, 84, 3, HERM_BONE_LEG_B1);
            }
            break;
        case EM_MOTION_ID(0, HERM_MOT_UNDER_THRUST):
            if (type == EM_KIND_SHOGUN_CEANATAUR) {
                Em_PlaySeAtBoneOnFrame(em, 6, 26, HERM_BONE_BODY);
                Em_PlaySeAtBoneOnFrame(em, 6, 27, HERM_BONE_BODY);
                Em_PlaySeAtBoneVolOnFrame(em, 8, 101, HERM_BONE_BODY, 22);
                Em_PlaySeAtBoneOnFrame(em, 14, 18, 14);
                Em_PlaySeAtBoneOnFrame(em, 48, 25, HERM_BONE_BODY);
                Em_PlaySeAtBoneOnFrame(em, 60, 26, HERM_BONE_BODY);
                Em_PlaySeAtBoneOnFrame(em, 18, 22, HERM_BONE_CLAW_A);
                Em_PlaySeAtBoneOnFrame(em, 32, 22, HERM_BONE_CLAW_B);
            } else {
                Em_PlaySeAtBoneOnFrame(em, 6, 26, HERM_BONE_BODY);
                Em_PlaySeAtBoneOnFrame(em, 6, 27, HERM_BONE_BODY);
                Em_PlaySeAtBoneVolOnFrame(em, 8, 101, HERM_BONE_BODY, 22);
                Em_PlaySeAtBoneOnFrame(em, 14, 18, 14);
                Em_PlaySeAtBoneOnFrame(em, 48, 25, HERM_BONE_BODY);
                Em_PlaySeAtBoneOnFrame(em, 60, 26, HERM_BONE_BODY);
            }
            break;
        case EM_MOTION_ID(0, HERM_MOT_SPIN_B):
            if (type == EM_KIND_SHOGUN_CEANATAUR) {
                Em_PlaySeAtBoneOnFrame(em, 26, 44, 14);
                Em_PlaySeAtBoneOnFrame(em, 74, 31, 14);
                Em_PlaySeAtBoneOnFrame(em, 78, 12, HERM_BONE_CLAW_B);
                Em_PlaySeAtBoneOnFrame(em, 84, 22, HERM_BONE_CLAW_B);
                Em_PlaySeAtBoneOnFrame(em, 82, 29, HERM_BONE_CLAW_B);
                Em_PlaySeAtBoneOnFrame(em, 48, 2, HERM_BONE_LEG_B1);
                Em_PlaySeAtBoneOnFrame(em, 62, 3, HERM_BONE_LEG_B2);
                Em_PlaySeAtBoneOnFrame(em, 78, 2, HERM_BONE_LEG_B1);
                Em_PlaySeAtBoneOnFrame(em, 62, 3, HERM_BONE_LEG_B2);
                Em_PlaySeAtBoneOnFrame(em, 52, 2, HERM_BONE_LEG_A2);
                Em_PlaySeAtBoneOnFrame(em, 60, 3, HERM_BONE_LEG_A1);
                Em_PlaySeAtBoneOnFrame(em, 76, 2, HERM_BONE_LEG_A2);
                Em_PlaySeAtBoneOnFrame(em, 84, 3, HERM_BONE_LEG_A1);
            } else {
                Em_PlaySeAtBoneOnFrame(em, 26, 44, 14);
                Em_PlaySeAtBoneOnFrame(em, 74, 31, 14);
                Em_PlaySeAtBoneOnFrame(em, 80, 22, HERM_BONE_CLAW_B);
                Em_PlaySeAtBoneOnFrame(em, 106, 13, HERM_BONE_CLAW_B);
                Em_PlaySeAtBoneOnFrame(em, 48, 2, HERM_BONE_LEG_B1);
                Em_PlaySeAtBoneOnFrame(em, 62, 3, HERM_BONE_LEG_B2);
                Em_PlaySeAtBoneOnFrame(em, 78, 2, HERM_BONE_LEG_B1);
                Em_PlaySeAtBoneOnFrame(em, 62, 3, HERM_BONE_LEG_B2);
                Em_PlaySeAtBoneOnFrame(em, 52, 2, HERM_BONE_LEG_A2);
                Em_PlaySeAtBoneOnFrame(em, 60, 3, HERM_BONE_LEG_A1);
                Em_PlaySeAtBoneOnFrame(em, 76, 2, HERM_BONE_LEG_A2);
                Em_PlaySeAtBoneOnFrame(em, 84, 3, HERM_BONE_LEG_A1);
            }
            break;
        case EM_MOTION_ID(0, HERM_MOT_SHELL_SWAP):
            Em_PlaySeAtBoneVolOnFrame(em, 70, 30, 14, 10);
            Em_PlaySeAtBoneVolOnFrame(em, 172, 34, 14, 10);
            Em_PlaySeAtBoneOnFrame(em, 26, 25, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 34, 25, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 56, 26, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 168, 2, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 172, 2, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 76, 14, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 144, 16, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 174, 20, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 176, 14, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 178, 15, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 180, 16, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 182, 17, HERM_BONE_BODY);
            break;
        case EM_MOTION_ID(0, HERM_MOT_IDLE_ALERT):
            Em_PlaySeAtBoneOnFrame(em, 74, 55, 14);
            Em_PlaySeAtBoneOnFrame(em, 114, 56, 14);
            Em_PlaySeAtBoneOnFrame(em, 166, 56, 14);
            Em_PlaySeAtBoneOnFrame(em, 76, 16, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 112, 16, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 30, 13, HERM_BONE_CLAW_A);
            Em_PlaySeAtBoneOnFrame(em, 38, 13, HERM_BONE_CLAW_B);
            Em_PlaySeAtBoneOnFrame(em, 86, 13, HERM_BONE_CLAW_A);
            Em_PlaySeAtBoneOnFrame(em, 94, 13, HERM_BONE_CLAW_B);
            Em_PlaySeAtBoneOnFrame(em, 128, 13, HERM_BONE_CLAW_A);
            Em_PlaySeAtBoneOnFrame(em, 134, 13, HERM_BONE_CLAW_B);
            Em_PlaySeAtBoneOnFrame(em, 190, 13, HERM_BONE_CLAW_A);
            Em_PlaySeAtBoneOnFrame(em, 184, 13, HERM_BONE_CLAW_B);
            break;
        case EM_MOTION_ID(0, HERM_MOT_THREAT):
            Em_PlaySeAtBoneOnFrame(em, 4, 42, 14);
            Em_PlaySeAtBoneVolOnFrame(em, 152, Sound_RandomSe(60, 6, 61, 2), 14, 10);
            Em_PlaySeAtBoneVolOnFrame(em, 222, Sound_RandomSe(61, 2, 62, 6), 14, 10);
            Em_PlaySeAtBoneOnFrame(em, 26, 15, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 48, 14, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 26, 9, HERM_BONE_CLAW_A);
            Em_PlaySeAtBoneOnFrame(em, 28, 13, HERM_BONE_CLAW_B);
            Em_PlaySeAtBoneOnFrame(em, 106, 2, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 108, 8, HERM_BONE_CLAW_A);
            Em_PlaySeAtBoneOnFrame(em, 110, 9, HERM_BONE_CLAW_B);
            Em_PlaySeAtBoneOnFrame(em, 166, 0, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 212, 0, HERM_BONE_LEG_B1);
            break;
        case EM_MOTION_ID(0, HERM_MOT_IDLE_ACTION_A):
        case EM_MOTION_ID(0, HERM_MOT_IDLE_ACTION_B):
            Em_PlaySeAtBoneOnFrame(em, 40, 15, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 88, 54, 14);
            Em_PlaySeAtBoneOnFrame(em, 268, 55, 14);
            Em_PlaySeAtBoneVolOnFrame(em, 152, Sound_RandomSe(60, 6, 61, 2), 14, 10);
            Em_PlaySeAtBoneVolOnFrame(em, 222, Sound_RandomSe(61, 2, 62, 6), 14, 10);
            Em_PlaySeAtBoneVolOnFrame(em, 282, Sound_RandomSe(60, 2, 61, 6), 14, 10);
            Em_PlaySeAtBoneOnFrame(em, 92, 17, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 270, 14, HERM_BONE_BODY);
            break;
        case EM_MOTION_ID(0, HERM_MOT_EAT):
            Em_PlaySeAtBoneOnFrame(em, 30, 56, 14);
            Em_PlaySeAtBoneVolOnFrame(em, 64, Sound_RandomSe(64, 4, 61, 4), 14, 10);
            Em_PlaySeAtBoneVolOnFrame(em, 114, Sound_RandomSe(60, 4, 61, 4), 14, 10);
            Em_PlaySeAtBoneOnFrame(em, 404, 23, 14);
            Em_PlaySeAtBoneOnFrame(em, 470, 23, 14);
            Em_PlaySeAtBoneOnFrame(em, 518, 23, 14);
            Em_PlaySeAtBoneVolOnFrame(em, 500, Sound_RandomSe(63, 4, 64, 4), 14, 10);
            Em_PlaySeAtBoneVolOnFrame(em, 540, Sound_RandomSe(64, 4, 61, 4), 14, 10);
            Em_PlaySeAtBoneVolOnFrame(em, 580, Sound_RandomSe(63, 4, 64, 4), 14, 10);
            Em_PlaySeAtBoneVolOnFrame(em, 620, Sound_RandomSe(64, 4, 61, 4), 14, 10);
            Em_PlaySeAtBoneOnFrame(em, 286, 29, HERM_BONE_CLAW_A);
            Em_PlaySeAtBoneOnFrame(em, 228, 13, HERM_BONE_CLAW_B);
            Em_PlaySeAtBoneOnFrame(em, 252, 29, HERM_BONE_CLAW_B);
            Em_PlaySeAtBoneOnFrame(em, 372, 21, HERM_BONE_CLAW_A);
            Em_PlaySeAtBoneOnFrame(em, 374, 17, HERM_BONE_CLAW_B);
            Em_PlaySeAtBoneOnFrame(em, 448, 17, HERM_BONE_CLAW_A);
            Em_PlaySeAtBoneOnFrame(em, 480, 17, HERM_BONE_CLAW_B);
            Em_PlaySeAtBoneOnFrame(em, 168, 2, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 266, 1, HERM_BONE_LEG_A2);
            Em_PlaySeAtBoneOnFrame(em, 662, 1, HERM_BONE_LEG_A2);
            Em_PlaySeAtBoneOnFrame(em, 704, 2, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 192, 1, HERM_BONE_LEG_B2);
            Em_PlaySeAtBoneOnFrame(em, 220, 0, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 680, 0, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 718, 3, HERM_BONE_LEG_B2);
            Em_PlaySeAtBoneOnFrame(em, 690, 16, HERM_BONE_BODY);
            break;
        case EM_MOTION_ID(0, HERM_MOT_BURROW):
            Em_PlaySeAtBoneOnFrame(em, 4, 30, 14);
            Em_PlaySeAtBoneOnFrame(em, 32, 25, HERM_BONE_CLAW_A);
            Em_PlaySeAtBoneOnFrame(em, 36, 25, HERM_BONE_CLAW_B);
            Em_PlaySeAtBoneOnFrame(em, 54, 25, HERM_BONE_CLAW_A);
            Em_PlaySeAtBoneOnFrame(em, 66, 25, HERM_BONE_CLAW_B);
            Em_PlaySeAtBoneOnFrame(em, 80, 25, HERM_BONE_CLAW_A);
            Em_PlaySeAtBoneOnFrame(em, 90, 25, HERM_BONE_CLAW_B);
            Em_PlaySeAtBoneOnFrame(em, 104, 25, HERM_BONE_CLAW_A);
            Em_PlaySeAtBoneOnFrame(em, 130, 26, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 160, 26, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 24, 6, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 48, 7, HERM_BONE_LEG_B2);
            Em_PlaySeAtBoneOnFrame(em, 38, 1, HERM_BONE_LEG_A2);
            Em_PlaySeAtBoneOnFrame(em, 56, 2, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 26, 14, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 48, 16, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 70, 14, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 90, 14, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 110, 14, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 130, 14, HERM_BONE_BODY);
            break;
        case EM_MOTION_ID(0, HERM_MOT_CEIL_GRAB):
            Em_PlaySeAtBoneVolOnFrame(em, 14, 53, 14, 10);
            Em_PlaySeAtBoneVolOnFrame(em, 64, Sound_RandomSe(60, 4, 61, 4), 14, 10);
            Em_PlaySeAtBoneOnFrame(em, 4, 5, HERM_BONE_LEG_A2);
            Em_PlaySeAtBoneOnFrame(em, 6, 7, HERM_BONE_LEG_B2);
            Em_PlaySeAtBoneOnFrame(em, 4, 4, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 6, 6, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 18, 14, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 20, 17, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 40, 16, HERM_BONE_BODY);
            break;
        case EM_MOTION_ID(0, HERM_MOT_CEIL_DROP):
            Em_PlaySeAtBoneOnFrame(em, 18, 44, 14);
            Em_PlaySeAtBoneOnFrame(em, 108, 44, 14);
            Em_PlaySeAtBoneOnFrame(em, 4, 14, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 56, 17, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 82, 17, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 108, 10, HERM_BONE_BODY);
            break;
        case EM_MOTION_ID(0, HERM_MOT_FLINCH):
            Em_PlaySeAtBoneOnFrame(em, 2, 38, 14);
            Em_PlaySeAtBoneVolOnFrame(em, 110, Sound_RandomSe(66, 4, 67, 4), 14, 10);
            Em_PlaySeAtBoneOnFrame(em, 14, 7, HERM_BONE_LEG_A2);
            Em_PlaySeAtBoneOnFrame(em, 58, 3, HERM_BONE_LEG_A2);
            Em_PlaySeAtBoneOnFrame(em, 74, 3, HERM_BONE_LEG_B2);
            Em_PlaySeAtBoneOnFrame(em, 42, 0, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 96, 0, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 46, 21, HERM_BONE_CLAW_A);
            Em_PlaySeAtBoneOnFrame(em, 56, 29, HERM_BONE_CLAW_B);
            Em_PlaySeAtBoneOnFrame(em, 26, 16, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 26, 17, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 54, 14, HERM_BONE_BODY);
            break;
        case EM_MOTION_ID(0, HERM_MOT_FLINCH_CLAW):
            Em_PlaySeAtBoneOnFrame(em, 2, 37, 14);
            Em_PlaySeAtBoneVolOnFrame(em, 118, Sound_RandomSe(60, 4, 61, 4), 14, 10);
            Em_PlaySeAtBoneOnFrame(em, 56, 29, HERM_BONE_CLAW_B);
            Em_PlaySeAtBoneOnFrame(em, 46, 0, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 84, 3, HERM_BONE_LEG_B2);
            Em_PlaySeAtBoneOnFrame(em, 28, 14, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 84, 16, HERM_BONE_BODY);
            break;
        case EM_MOTION_ID(0, HERM_MOT_KNOCK_DOWN):
            Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(38, 4, 39, 4), 14);
            Em_PlaySeAtBoneOnFrame(em, 28, 19, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 36, 29, HERM_BONE_CLAW_A);
            Em_PlaySeAtBoneOnFrame(em, 50, 14, HERM_BONE_BODY);
            break;
        case EM_MOTION_ID(0, HERM_MOT_DOWN):
            Em_PlaySeAtBoneOnFrame(em, 4, 49, 14);
            Em_PlaySeAtBoneOnFrame(em, 12, 14, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 4, 0, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 52, 0, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 2, 1, HERM_BONE_LEG_A2);
            Em_PlaySeAtBoneOnFrame(em, 38, 1, HERM_BONE_LEG_A2);
            Em_PlaySeAtBoneOnFrame(em, 4, 15, HERM_BONE_CLAW_A);
            Em_PlaySeAtBoneOnFrame(em, 54, 15, HERM_BONE_CLAW_A);
            break;
        case EM_MOTION_ID(0, HERM_MOT_HELD):
            Em_PlaySeAtBoneOnFrame(em, 4, 14, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 14, 0, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 16, 51, 14);
            Em_PlaySeAtBoneOnFrame(em, 130, 52, 14);
            Em_PlaySeAtBoneOnFrame(em, 40, 17, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 60, 17, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 86, 17, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 110, 17, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 130, 17, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 148, 17, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 182, 17, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 202, 17, HERM_BONE_BODY);
            break;
        case EM_MOTION_ID(0, HERM_MOT_FALL):
            Em_PlaySeAtBoneOnFrame(em, 2, 38, 14);
            Em_PlaySeAtBoneOnFrame(em, 36, 14, HERM_BONE_BODY);
            break;
        case EM_MOTION_ID(0, HERM_MOT_FALL_LAND):
            Em_PlaySeAtBoneOnFrame(em, 46, 38, 14);
            Em_PlaySeAtBoneOnFrame(em, 4, 19, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 24, 29, HERM_BONE_CLAW_B);
            Em_PlaySeAtBoneOnFrame(em, 46, 14, HERM_BONE_BODY);
            break;
        case EM_MOTION_ID(0, HERM_MOT_DIE):
            Em_PlaySeAtBoneOnFrame(em, 2, 40, 14);
            Em_PlaySeAtBoneOnFrame(em, 136, 33, 14);
            Em_PlaySeAtBoneOnFrame(em, 68, 14, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 158, 19, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 168, 18, HERM_BONE_CLAW_A);
            Em_PlaySeAtBoneOnFrame(em, 174, 29, HERM_BONE_CLAW_B);
            Em_PlaySeAtBoneOnFrame(em, 58, 1, HERM_BONE_LEG_B2);
            Em_PlaySeAtBoneOnFrame(em, 69, 1, HERM_BONE_LEG_A2);
            Em_PlaySeAtBoneOnFrame(em, 238, 15, HERM_BONE_LEG_A2);
            Em_PlaySeAtBoneOnFrame(em, 300, 15, HERM_BONE_LEG_A2);
            Em_PlaySeAtBoneOnFrame(em, 364, 17, HERM_BONE_LEG_A2);
            break;
        case EM_MOTION_ID(0, HERM_MOT_SPIN_FLINCH):
            Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(37, 4, 39, 4), 14);
            Em_PlaySeAtBoneOnFrame(em, 64, 19, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 92, 16, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 114, 16, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 166, 14, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 70, 29, HERM_BONE_CLAW_A);
            Em_PlaySeAtBoneOnFrame(em, 74, 29, HERM_BONE_CLAW_B);
            Em_PlaySeAtBoneOnFrame(em, 98, 21, HERM_BONE_CLAW_A);
            Em_PlaySeAtBoneOnFrame(em, 108, 21, HERM_BONE_CLAW_B);
            Em_PlaySeAtBoneOnFrame(em, 54, 0, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 76, 1, HERM_BONE_LEG_B2);
            Em_PlaySeAtBoneOnFrame(em, 90, 2, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 100, 3, HERM_BONE_LEG_B2);
            Em_PlaySeAtBoneOnFrame(em, 118, 2, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 126, 3, HERM_BONE_LEG_B2);
            Em_PlaySeAtBoneOnFrame(em, 142, 2, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 150, 3, HERM_BONE_LEG_B2);
            Em_PlaySeAtBoneOnFrame(em, 182, 3, HERM_BONE_LEG_B2);
            Em_PlaySeAtBoneOnFrame(em, 204, 0, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 82, 1, HERM_BONE_LEG_B2);
            Em_PlaySeAtBoneOnFrame(em, 52, 0, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 48, 1, HERM_BONE_LEG_A2);
            Em_PlaySeAtBoneOnFrame(em, 84, 3, HERM_BONE_LEG_A2);
            Em_PlaySeAtBoneOnFrame(em, 98, 2, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 104, 3, HERM_BONE_LEG_A2);
            Em_PlaySeAtBoneOnFrame(em, 124, 2, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 134, 3, HERM_BONE_LEG_A2);
            Em_PlaySeAtBoneOnFrame(em, 148, 2, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 156, 3, HERM_BONE_LEG_A2);
            Em_PlaySeAtBoneOnFrame(em, 192, 0, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 218, 1, HERM_BONE_LEG_A2);
            Em_PlaySeAtBoneOnFrame(em, 192, 9, HERM_BONE_CLAW_A);
            Em_PlaySeAtBoneOnFrame(em, 194, 13, HERM_BONE_CLAW_B);
            break;
        case EM_MOTION_ID(0, HERM_MOT_DOWN_GET_UP):
            Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(66, 6, 67, 2), 14);
            Em_PlaySeAtBoneOnFrame(em, 66, Sound_RandomSe(67, 2, 67, 6), 14);
            Em_PlaySeAtBoneOnFrame(em, 12, 0, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 32, 1, HERM_BONE_LEG_A2);
            Em_PlaySeAtBoneOnFrame(em, 42, 6, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 22, 7, HERM_BONE_LEG_B2);
            Em_PlaySeAtBoneOnFrame(em, 26, 13, HERM_BONE_CLAW_A);
            Em_PlaySeAtBoneOnFrame(em, 56, 16, HERM_BONE_CLAW_A);
            Em_PlaySeAtBoneOnFrame(em, 66, 20, HERM_BONE_CLAW_B);
            Em_PlaySeAtBoneOnFrame(em, 60, 14, HERM_BONE_BODY);
            break;
        case EM_MOTION_ID(0, HERM_MOT_KNOCK_DOWN_B):
            Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(38, 4, 39, 4), 14);
            Em_PlaySeAtBoneOnFrame(em, 28, 19, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 36, 29, HERM_BONE_CLAW_B);
            Em_PlaySeAtBoneOnFrame(em, 50, 14, HERM_BONE_BODY);
            break;
        case EM_MOTION_ID(0, HERM_MOT_DOWN_B):
            Em_PlaySeAtBoneOnFrame(em, 4, 49, 14);
            Em_PlaySeAtBoneOnFrame(em, 12, 14, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 4, 0, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 52, 0, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 2, 1, HERM_BONE_LEG_B2);
            Em_PlaySeAtBoneOnFrame(em, 38, 1, HERM_BONE_LEG_B2);
            Em_PlaySeAtBoneOnFrame(em, 4, 15, HERM_BONE_CLAW_B);
            Em_PlaySeAtBoneOnFrame(em, 54, 15, HERM_BONE_CLAW_B);
            break;
        case EM_MOTION_ID(0, HERM_MOT_DOWN_B_GET_UP):
            Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(66, 6, 67, 2), 14);
            Em_PlaySeAtBoneOnFrame(em, 66, Sound_RandomSe(66, 2, 67, 6), 14);
            Em_PlaySeAtBoneOnFrame(em, 12, 0, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 32, 1, HERM_BONE_LEG_B2);
            Em_PlaySeAtBoneOnFrame(em, 42, 6, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 22, 7, HERM_BONE_LEG_A2);
            Em_PlaySeAtBoneOnFrame(em, 26, 13, HERM_BONE_CLAW_B);
            Em_PlaySeAtBoneOnFrame(em, 56, 16, HERM_BONE_CLAW_B);
            Em_PlaySeAtBoneOnFrame(em, 66, 20, HERM_BONE_CLAW_A);
            Em_PlaySeAtBoneOnFrame(em, 60, 14, HERM_BONE_BODY);
            break;
        case EM_MOTION_ID(0, HERM_MOT_RECOIL):
            if (type == EM_KIND_SHOGUN_CEANATAUR) {
                Em_PlaySeAtBoneOnFrame(em, 4, 38, 14);
                Em_PlaySeAtBoneOnFrame(em, 80, 49, 14);
                Em_PlaySeAtBoneOnFrame(em, 198, 55, 14);
                Em_PlaySeAtBoneOnFrame(em, 48, 0, HERM_BONE_LEG_A1);
                Em_PlaySeAtBoneOnFrame(em, 88, 0, HERM_BONE_LEG_B1);
                Em_PlaySeAtBoneOnFrame(em, 126, 0, HERM_BONE_LEG_A1);
                Em_PlaySeAtBoneOnFrame(em, 156, 0, HERM_BONE_LEG_B1);
                Em_PlaySeAtBoneOnFrame(em, 60, 1, HERM_BONE_LEG_B2);
                Em_PlaySeAtBoneOnFrame(em, 94, 1, HERM_BONE_LEG_A2);
                Em_PlaySeAtBoneOnFrame(em, 134, 1, HERM_BONE_LEG_B2);
                Em_PlaySeAtBoneOnFrame(em, 174, 1, HERM_BONE_LEG_A2);
                Em_PlaySeAtBoneOnFrame(em, 18, 15, HERM_BONE_BODY);
                Em_PlaySeAtBoneOnFrame(em, 58, 14, HERM_BONE_BODY);
                Em_PlaySeAtBoneOnFrame(em, 88, 17, HERM_BONE_BODY);
                Em_PlaySeAtBoneOnFrame(em, 174, 14, HERM_BONE_BODY);
            } else {
                Em_PlaySeAtBoneOnFrame(em, 4, 38, 14);
                Em_PlaySeAtBoneOnFrame(em, 80, 49, 14);
                Em_PlaySeAtBoneOnFrame(em, 198, Sound_RandomSe(60, 4, 62, 4), 14);
                Em_PlaySeAtBoneOnFrame(em, 122, 18, HERM_BONE_BODY);
                Em_PlaySeAtBoneOnFrame(em, 58, 14, HERM_BONE_BODY);
                Em_PlaySeAtBoneOnFrame(em, 18, 1, HERM_BONE_LEG_B2);
                Em_PlaySeAtBoneOnFrame(em, 68, 5, HERM_BONE_LEG_B2);
                Em_PlaySeAtBoneOnFrame(em, 96, 0, HERM_BONE_LEG_B1);
                Em_PlaySeAtBoneOnFrame(em, 48, 6, HERM_BONE_LEG_A1);
                Em_PlaySeAtBoneOnFrame(em, 116, 1, HERM_BONE_LEG_A2);
                Em_PlaySeAtBoneOnFrame(em, 118, 29, HERM_BONE_CLAW_A);
                Em_PlaySeAtBoneOnFrame(em, 128, 29, HERM_BONE_CLAW_B);
            }
            break;
        case EM_MOTION_ID(0, HERM_MOT_RECOIL_END):
            Em_PlaySeAtBoneOnFrame(em, 4, 34, 14);
            Em_PlaySeAtBoneOnFrame(em, 66, Sound_RandomSe(60, 4, 62, 4), 14);
            Em_PlaySeAtBoneOnFrame(em, 28, 16, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 92, 14, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 4, 13, HERM_BONE_CLAW_A);
            Em_PlaySeAtBoneOnFrame(em, 16, 21, HERM_BONE_CLAW_B);
            Em_PlaySeAtBoneOnFrame(em, 90, 15, HERM_BONE_CLAW_A);
            Em_PlaySeAtBoneOnFrame(em, 94, 16, HERM_BONE_CLAW_B);
            break;
        case EM_MOTION_ID(0, HERM_MOT_FLINCH_END):
            Em_PlaySeAtBoneOnFrame(em, 14, 55, 14);
            Em_PlaySeAtBoneOnFrame(em, 4, 16, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 4, 14, HERM_BONE_BODY);
            break;
        }
    } else {
                switch (work->fxMotion) {
        case EM_MOTION_ID(0, HERM_MOT_IDLE):
            Em_PlaySeAtBoneOnFrame(em, 14, Sound_RandomSe(10, 2, 11, 3), 14);
            Em_PlaySeAtBoneOnFrame(em, 60, Sound_RandomSe(11, 2, 12, 2), 14);
            Em_PlaySeAtBoneOnFrame(em, 120, Sound_RandomSe(12, 3, 10, 2), 14);
            Em_PlaySeAtBoneOnFrame(em, 180, Sound_RandomSe(10, 3, 11, 2), 14);
            Em_PlaySeAtBoneOnFrame(em, 240, Sound_RandomSe(11, 2, 12, 2), 14);
            Em_PlaySeAtBoneOnFrame(em, 168, 4, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 290, 5, HERM_BONE_LEG_A2);
            break;
        case EM_MOTION_ID(0, HERM_MOT_WALK):
            Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(10, 2, 11, 2), 14);
            Em_PlaySeAtBoneOnFrame(em, 38, Sound_RandomSe(11, 2, 12, 2), 14);
            Em_PlaySeAtBoneOnFrame(em, 70, Sound_RandomSe(12, 2, 10, 2), 14);
            Em_PlaySeAtBoneOnFrame(em, 40, 0, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 92, 1, HERM_BONE_LEG_A2);
            break;
        case EM_MOTION_ID(0, HERM_MOT_WALK_BACK):
            Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(10, 2, 11, 2), 14);
            Em_PlaySeAtBoneOnFrame(em, 38, Sound_RandomSe(11, 2, 12, 2), 14);
            Em_PlaySeAtBoneOnFrame(em, 70, Sound_RandomSe(12, 2, 10, 2), 14);
            Em_PlaySeAtBoneOnFrame(em, 98, 0, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 48, 1, HERM_BONE_LEG_A2);
            Em_PlaySeAtBoneOnFrame(em, 12, 1, HERM_BONE_LEG_B2);
            break;
        case EM_MOTION_ID(0, HERM_MOT_TURN_RIGHT):
            Em_PlaySeAtBoneOnFrame(em, 24, Sound_RandomSe(10, 4, 11, 4), 14);
            Em_PlaySeAtBoneOnFrame(em, 80, Sound_RandomSe(11, 4, 12, 4), 14);
            Em_PlaySeAtBoneOnFrame(em, 28, 0, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 40, 1, HERM_BONE_LEG_A2);
            Em_PlaySeAtBoneOnFrame(em, 72, 0, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 84, 1, HERM_BONE_LEG_A2);
            Em_PlaySeAtBoneOnFrame(em, 18, 0, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 62, 0, HERM_BONE_LEG_B1);
            break;
        case EM_MOTION_ID(0, HERM_MOT_TURN_LEFT):
            Em_PlaySeAtBoneOnFrame(em, 24, Sound_RandomSe(10, 4, 11, 4), 14);
            Em_PlaySeAtBoneOnFrame(em, 80, Sound_RandomSe(11, 4, 12, 4), 14);
            Em_PlaySeAtBoneOnFrame(em, 28, 0, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 40, 1, HERM_BONE_LEG_B2);
            Em_PlaySeAtBoneOnFrame(em, 72, 0, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 84, 1, HERM_BONE_LEG_B2);
            Em_PlaySeAtBoneOnFrame(em, 18, 0, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 62, 0, HERM_BONE_LEG_A1);
            break;
        case EM_MOTION_ID(0, HERM_MOT_TURN_RIGHT_ALERT):
            Em_PlaySeAtBoneOnFrame(em, 16, Sound_RandomSe(10, 4, 11, 4), 14);
            Em_PlaySeAtBoneOnFrame(em, 40, Sound_RandomSe(11, 4, 12, 4), 14);
            Em_PlaySeAtBoneOnFrame(em, 2, 13, HERM_BONE_CLAW_B);
            Em_PlaySeAtBoneOnFrame(em, 12, 0, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 44, 0, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 56, 1, HERM_BONE_LEG_B2);
            Em_PlaySeAtBoneOnFrame(em, 18, 0, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 30, 1, HERM_BONE_LEG_A2);
            Em_PlaySeAtBoneOnFrame(em, 62, 1, HERM_BONE_LEG_A2);
            break;
        case EM_MOTION_ID(0, HERM_MOT_TURN_LEFT_ALERT):
            Em_PlaySeAtBoneOnFrame(em, 16, Sound_RandomSe(10, 4, 11, 4), 14);
            Em_PlaySeAtBoneOnFrame(em, 40, Sound_RandomSe(11, 4, 12, 4), 14);
            Em_PlaySeAtBoneOnFrame(em, 2, 13, HERM_BONE_CLAW_A);
            Em_PlaySeAtBoneOnFrame(em, 12, 0, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 44, 0, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 56, 1, HERM_BONE_LEG_A2);
            Em_PlaySeAtBoneOnFrame(em, 18, 0, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 30, 1, HERM_BONE_LEG_B2);
            Em_PlaySeAtBoneOnFrame(em, 62, 1, HERM_BONE_LEG_B2);
            break;
        case EM_MOTION_ID(0, HERM_MOT_LEAP):
            Em_PlaySeAtBoneOnFrame(em, 92, 16, 14);
            Em_PlaySeAtBoneOnFrame(em, 50, 17, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 80, 14, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 20, 0, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 44, 1, HERM_BONE_LEG_B2);
            Em_PlaySeAtBoneOnFrame(em, 44, 0, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 20, 1, HERM_BONE_LEG_A2);
            break;
        case EM_MOTION_ID(0, HERM_MOT_LEAP_LAND):
            Em_PlaySeAtBoneOnFrame(em, 54, Sound_RandomSe(10, 4, 11, 4), 14);
            Em_PlaySeAtBoneOnFrame(em, 40, Sound_RandomSe(11, 4, 12, 4), 14);
            Em_PlaySeAtBoneOnFrame(em, 6, 18, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 10, 19, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 10, 16, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 136, 16, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 74, 20, HERM_BONE_CLAW_A);
            Em_PlaySeAtBoneOnFrame(em, 78, 20, HERM_BONE_CLAW_B);
            Em_PlaySeAtBoneOnFrame(em, 82, 4, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 72, 5, HERM_BONE_LEG_A2);
            Em_PlaySeAtBoneOnFrame(em, 124, 0, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 138, 1, HERM_BONE_LEG_A2);
            Em_PlaySeAtBoneOnFrame(em, 80, 4, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 84, 5, HERM_BONE_LEG_B2);
            Em_PlaySeAtBoneOnFrame(em, 138, 0, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 122, 2, HERM_BONE_LEG_B2);
            break;
        case EM_MOTION_ID(0, 15):
            Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(10, 2, 11, 2), 14);
            Em_PlaySeAtBoneOnFrame(em, 37, Sound_RandomSe(11, 2, 12, 2), 14);
            Em_PlaySeAtBoneOnFrame(em, 70, Sound_RandomSe(12, 2, 10, 2), 14);
            Em_PlaySeAtBoneOnFrame(em, 52, 0, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 2, 0, HERM_BONE_LEG_B1);
            break;
        case EM_MOTION_ID(0, 16):
            Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(10, 2, 11, 2), 14);
            Em_PlaySeAtBoneOnFrame(em, 37, Sound_RandomSe(11, 2, 12, 2), 14);
            Em_PlaySeAtBoneOnFrame(em, 70, Sound_RandomSe(12, 2, 10, 2), 14);
            Em_PlaySeAtBoneOnFrame(em, 52, 0, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 2, 0, HERM_BONE_LEG_A1);
            break;
        case EM_MOTION_ID(0, 18):
            Em_PlaySeAtBoneOnFrame(em, 4, 35, 14);
            Em_PlaySeAtBoneOnFrame(em, 4, 17, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 22, 13, HERM_BONE_LEG_B1);
            break;
        case EM_MOTION_ID(0, HERM_MOT_STRIKE_C):
            Em_PlaySeAtBoneOnFrame(em, 4, 36, 14);
            Em_PlaySeAtBoneOnFrame(em, 16, 22, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 68, 16, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 20, 23, HERM_BONE_LEG_B1);
            break;
        case EM_MOTION_ID(0, HERM_MOT_SIDESTEP_A):
            Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(10, 3, 11, 2), 14);
            Em_PlaySeAtBoneOnFrame(em, 20, 0, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 6, 1, HERM_BONE_LEG_A2);
            Em_PlaySeAtBoneOnFrame(em, 40, 0, HERM_BONE_CLAW_B);
            break;
        case EM_MOTION_ID(0, HERM_MOT_STRIKE_B):
            Em_PlaySeAtBoneOnFrame(em, 4, 30, 14);
            Em_PlaySeAtBoneOnFrame(em, 24, 0, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 4, 0, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 40, 0, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 20, 23, HERM_BONE_CLAW_B);
            Em_PlaySeAtBoneOnFrame(em, 16, 22, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 58, 0, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 66, 1, HERM_BONE_LEG_B2);
            Em_PlaySeAtBoneOnFrame(em, 86, 0, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 100, 1, HERM_BONE_LEG_A2);
            break;
        case EM_MOTION_ID(0, HERM_MOT_SIDESTEP_B):
            Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(10, 2, 11, 2), 14);
            Em_PlaySeAtBoneOnFrame(em, 20, 0, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 6, 1, HERM_BONE_LEG_B2);
            Em_PlaySeAtBoneOnFrame(em, 40, 0, HERM_BONE_LEG_A1);
            break;
        case EM_MOTION_ID(0, HERM_MOT_STRIKE_B_MIRROR):
            Em_PlaySeAtBoneOnFrame(em, 4, 30, 14);
            Em_PlaySeAtBoneOnFrame(em, 24, 0, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 4, 0, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 40, 0, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 20, 23, HERM_BONE_CLAW_A);
            Em_PlaySeAtBoneOnFrame(em, 16, 22, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 58, 0, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 66, 1, HERM_BONE_LEG_A2);
            Em_PlaySeAtBoneOnFrame(em, 86, 0, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 100, 1, HERM_BONE_LEG_B2);
            break;
        case EM_MOTION_ID(0, 24):
            Em_PlaySeAtBoneOnFrame(em, 4, 35, 14);
            Em_PlaySeAtBoneOnFrame(em, 4, 17, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 22, 13, HERM_BONE_LEG_B1);
            break;
        case EM_MOTION_ID(0, HERM_MOT_STRIKE_C_MIRROR):
            Em_PlaySeAtBoneOnFrame(em, 4, 36, 14);
            Em_PlaySeAtBoneOnFrame(em, 16, 22, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 68, 16, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 20, 23, HERM_BONE_CLAW_A);
            break;
        case EM_MOTION_ID(0, HERM_MOT_LIE):
            Em_PlaySeAtBoneOnFrame(em, 4, 37, 14);
            Em_PlaySeAtBoneOnFrame(em, 74, Sound_RandomSe(10, 4, 11, 4), 14);
            Em_PlaySeAtBoneOnFrame(em, 154, 37, 14);
            Em_PlaySeAtBoneOnFrame(em, 224, Sound_RandomSe(10, 4, 11, 4), 14);
            break;
        case EM_MOTION_ID(0, HERM_MOT_GET_UP):
            Em_PlaySeAtBoneOnFrame(em, 74, Sound_RandomSe(10, 4, 11, 4), 14);
            Em_PlaySeAtBoneOnFrame(em, 54, 36, 14);
            Em_PlaySeAtBoneOnFrame(em, 60, 16, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 102, 17, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 122, 13, HERM_BONE_CLAW_B);
            break;
        case EM_MOTION_ID(0, HERM_MOT_LIE_DOWN):
            Em_PlaySeAtBoneOnFrame(em, 4, 33, 14);
            Em_PlaySeAtBoneOnFrame(em, 60, 16, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 102, 17, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 142, 6, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 142, 12, 14);
            break;
        case EM_MOTION_ID(0, HERM_MOT_EMERGE):
            Em_PlaySeAtBoneOnFrame(em, 6, 22, HERM_BONE_CLAW_B);
            Em_PlaySeAtBoneOnFrame(em, 10, 22, HERM_BONE_CLAW_A);
            Em_PlaySeAtBoneOnFrame(em, 26, 13, HERM_BONE_CLAW_B);
            Em_PlaySeAtBoneOnFrame(em, 58, 13, HERM_BONE_CLAW_A);
            Em_PlaySeAtBoneOnFrame(em, 82, 0, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 82, 0, HERM_BONE_LEG_B2);
            Em_PlaySeAtBoneOnFrame(em, 92, 0, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 134, 0, HERM_BONE_LEG_B2);
            Em_PlaySeAtBoneOnFrame(em, 2, 25, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 64, 26, HERM_BONE_BODY);
            break;
        case EM_MOTION_ID(0, HERM_MOT_WATER_JET_END):
            Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(10, 6, 11, 2), 14);
            Em_PlaySeAtBoneOnFrame(em, 36, 0, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 42, 17, HERM_BONE_BODY);
            break;
        case EM_MOTION_ID(0, HERM_MOT_WATER_JET):
            Em_PlaySeAtBoneOnFrame(em, 4, 35, 14);
            Em_PlaySeAtBoneOnFrame(em, 4, 15, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 18, 1, HERM_BONE_LEG_B2);
            Em_PlaySeAtBoneOnFrame(em, 36, 0, HERM_BONE_LEG_A1);
            break;
        case EM_MOTION_ID(0, HERM_MOT_STRIKE_A):
            Em_PlaySeAtBoneOnFrame(em, 2, 35, 14);
            Em_PlaySeAtBoneOnFrame(em, 44, 36, 14);
            Em_PlaySeAtBoneOnFrame(em, 4, 16, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 6, 17, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 44, 22, HERM_BONE_CLAW_B);
            break;
        case EM_MOTION_ID(0, HERM_MOT_WINDUP_STRIKE):
            Em_PlaySeAtBoneOnFrame(em, 6, 30, 14);
            Em_PlaySeAtBoneOnFrame(em, 12, 22, HERM_BONE_CLAW_B);
            Em_PlaySeAtBoneOnFrame(em, 22, 22, HERM_BONE_CLAW_A);
            Em_PlaySeAtBoneOnFrame(em, 20, 13, HERM_BONE_CLAW_B);
            Em_PlaySeAtBoneOnFrame(em, 30, 13, HERM_BONE_CLAW_A);
            Em_PlaySeAtBoneOnFrame(em, 18, 20, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 60, 16, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 60, 17, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 28, 2, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 100, 1, HERM_BONE_LEG_B2);
            Em_PlaySeAtBoneOnFrame(em, 122, 0, HERM_BONE_LEG_B1);
            break;
        case EM_MOTION_ID(0, HERM_MOT_WINDUP):
            Em_PlaySeAtBoneOnFrame(em, 44, 31, 14);
            Em_PlaySeAtBoneOnFrame(em, 44, 17, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 12, 2, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 46, 0, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 32, 1, HERM_BONE_LEG_A2);
            break;
        case EM_MOTION_ID(0, HERM_MOT_WALK_ALERT):
            Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(10, 4, 11, 4), 14);
            Em_PlaySeAtBoneOnFrame(em, 40, Sound_RandomSe(11, 4, 12, 4), 14);
            Em_PlaySeAtBoneOnFrame(em, 30, 0, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 56, 1, HERM_BONE_LEG_A2);
            Em_PlaySeAtBoneOnFrame(em, 2, 0, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 26, 1, HERM_BONE_LEG_B2);
            break;
        case EM_MOTION_ID(0, HERM_MOT_WALK_BACK_ALERT):
            Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(10, 4, 11, 4), 14);
            Em_PlaySeAtBoneOnFrame(em, 40, Sound_RandomSe(11, 4, 12, 4), 14);
            Em_PlaySeAtBoneOnFrame(em, 2, 0, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 22, 1, HERM_BONE_LEG_A2);
            Em_PlaySeAtBoneOnFrame(em, 40, 0, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 62, 1, HERM_BONE_LEG_A2);
            Em_PlaySeAtBoneOnFrame(em, 2, 1, HERM_BONE_LEG_B2);
            Em_PlaySeAtBoneOnFrame(em, 22, 0, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 42, 1, HERM_BONE_LEG_B2);
            Em_PlaySeAtBoneOnFrame(em, 62, 0, HERM_BONE_LEG_B1);
            break;
        case EM_MOTION_ID(0, HERM_MOT_SPIN_A):
            Em_PlaySeAtBoneOnFrame(em, 4, 31, 14);
            Em_PlaySeAtBoneOnFrame(em, 70, 30, 14);
            Em_PlaySeAtBoneOnFrame(em, 80, 22, HERM_BONE_CLAW_A);
            Em_PlaySeAtBoneOnFrame(em, 82, 23, HERM_BONE_CLAW_A);
            Em_PlaySeAtBoneOnFrame(em, 48, 2, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 62, 1, HERM_BONE_LEG_A2);
            Em_PlaySeAtBoneOnFrame(em, 76, 1, HERM_BONE_LEG_B2);
            Em_PlaySeAtBoneOnFrame(em, 84, 2, HERM_BONE_LEG_B1);
            break;
        case EM_MOTION_ID(0, HERM_MOT_SPIN_B):
            Em_PlaySeAtBoneOnFrame(em, 4, 31, 14);
            Em_PlaySeAtBoneOnFrame(em, 70, 30, 14);
            Em_PlaySeAtBoneOnFrame(em, 80, 22, HERM_BONE_CLAW_B);
            Em_PlaySeAtBoneOnFrame(em, 82, 23, HERM_BONE_CLAW_B);
            Em_PlaySeAtBoneOnFrame(em, 48, 2, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 62, 3, HERM_BONE_LEG_B2);
            Em_PlaySeAtBoneOnFrame(em, 76, 3, HERM_BONE_LEG_A2);
            Em_PlaySeAtBoneOnFrame(em, 84, 2, HERM_BONE_LEG_A1);
            break;
        case EM_MOTION_ID(0, HERM_MOT_IDLE_ALERT):
            Em_PlaySeAtBoneOnFrame(em, 74, 35, 14);
            Em_PlaySeAtBoneOnFrame(em, 114, 30, 14);
            Em_PlaySeAtBoneOnFrame(em, 30, 13, HERM_BONE_CLAW_A);
            Em_PlaySeAtBoneOnFrame(em, 38, 13, HERM_BONE_CLAW_B);
            Em_PlaySeAtBoneOnFrame(em, 86, 13, HERM_BONE_CLAW_A);
            Em_PlaySeAtBoneOnFrame(em, 94, 13, HERM_BONE_CLAW_B);
            Em_PlaySeAtBoneOnFrame(em, 128, 13, HERM_BONE_CLAW_A);
            Em_PlaySeAtBoneOnFrame(em, 134, 13, HERM_BONE_CLAW_B);
            Em_PlaySeAtBoneOnFrame(em, 190, 13, HERM_BONE_CLAW_A);
            Em_PlaySeAtBoneOnFrame(em, 184, 13, HERM_BONE_CLAW_B);
            break;
        case EM_MOTION_ID(0, HERM_MOT_THREAT):
            Em_PlaySeAtBoneOnFrame(em, 12, 31, 14);
            Em_PlaySeAtBoneOnFrame(em, 16, 13, HERM_BONE_CLAW_A);
            Em_PlaySeAtBoneOnFrame(em, 20, 13, HERM_BONE_CLAW_B);
            Em_PlaySeAtBoneOnFrame(em, 122, 13, HERM_BONE_CLAW_A);
            Em_PlaySeAtBoneOnFrame(em, 126, 13, HERM_BONE_CLAW_B);
            Em_PlaySeAtBoneOnFrame(em, 106, 2, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 166, 0, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 212, 0, HERM_BONE_LEG_B1);
            break;
        case EM_MOTION_ID(0, HERM_MOT_IDLE_ACTION_A):
            break;
        case EM_MOTION_ID(0, HERM_MOT_EAT):
            Em_PlaySeAtBoneOnFrame(em, 30, 36, 14);
            Em_PlaySeAtBoneOnFrame(em, 64, Sound_RandomSe(10, 4, 11, 4), 14);
            Em_PlaySeAtBoneOnFrame(em, 114, Sound_RandomSe(11, 4, 10, 4), 14);
            Em_PlaySeAtBoneOnFrame(em, 404, 23, 14);
            Em_PlaySeAtBoneOnFrame(em, 470, 23, 14);
            Em_PlaySeAtBoneOnFrame(em, 518, 23, 14);
            Em_PlaySeAtBoneOnFrame(em, 500, Sound_RandomSe(11, 4, 12, 4), 14);
            Em_PlaySeAtBoneOnFrame(em, 540, Sound_RandomSe(11, 4, 10, 4), 14);
            Em_PlaySeAtBoneOnFrame(em, 580, Sound_RandomSe(10, 4, 12, 4), 14);
            Em_PlaySeAtBoneOnFrame(em, 620, Sound_RandomSe(12, 4, 10, 4), 14);
            Em_PlaySeAtBoneOnFrame(em, 286, 23, HERM_BONE_CLAW_A);
            Em_PlaySeAtBoneOnFrame(em, 228, 13, HERM_BONE_CLAW_B);
            Em_PlaySeAtBoneOnFrame(em, 252, 23, HERM_BONE_CLAW_B);
            Em_PlaySeAtBoneOnFrame(em, 372, 20, HERM_BONE_CLAW_A);
            Em_PlaySeAtBoneOnFrame(em, 374, 5, HERM_BONE_CLAW_B);
            Em_PlaySeAtBoneOnFrame(em, 448, 17, HERM_BONE_CLAW_A);
            Em_PlaySeAtBoneOnFrame(em, 480, 17, HERM_BONE_CLAW_B);
            Em_PlaySeAtBoneOnFrame(em, 168, 2, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 266, 1, HERM_BONE_LEG_A2);
            Em_PlaySeAtBoneOnFrame(em, 662, 1, HERM_BONE_LEG_A2);
            Em_PlaySeAtBoneOnFrame(em, 704, 2, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 192, 1, HERM_BONE_LEG_B2);
            Em_PlaySeAtBoneOnFrame(em, 220, 0, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 680, 0, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 718, 3, HERM_BONE_LEG_B2);
            Em_PlaySeAtBoneOnFrame(em, 690, 16, HERM_BONE_BODY);
            break;
        case EM_MOTION_ID(0, HERM_MOT_BURROW):
            Em_PlaySeAtBoneOnFrame(em, 12, 35, 14);
            Em_PlaySeAtBoneOnFrame(em, 26, 23, HERM_BONE_CLAW_A);
            Em_PlaySeAtBoneOnFrame(em, 36, 23, HERM_BONE_CLAW_B);
            Em_PlaySeAtBoneOnFrame(em, 54, 23, HERM_BONE_CLAW_A);
            Em_PlaySeAtBoneOnFrame(em, 66, 23, HERM_BONE_CLAW_B);
            Em_PlaySeAtBoneOnFrame(em, 80, 23, HERM_BONE_CLAW_A);
            Em_PlaySeAtBoneOnFrame(em, 90, 23, HERM_BONE_CLAW_B);
            Em_PlaySeAtBoneOnFrame(em, 104, 23, HERM_BONE_CLAW_A);
            Em_PlaySeAtBoneOnFrame(em, 38, 1, HERM_BONE_LEG_A2);
            Em_PlaySeAtBoneOnFrame(em, 56, 2, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 96, 16, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 104, 21, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 120, 26, HERM_BONE_BODY);
            break;
        case EM_MOTION_ID(0, HERM_MOT_FLINCH):
            Em_PlaySeAtBoneOnFrame(em, 2, 32, 14);
            Em_PlaySeAtBoneOnFrame(em, 120, Sound_RandomSe(11, 4, 12, 4), 14);
            Em_PlaySeAtBoneOnFrame(em, 94, 2, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 14, 1, HERM_BONE_LEG_A2);
            Em_PlaySeAtBoneOnFrame(em, 76, 3, HERM_BONE_LEG_B2);
            Em_PlaySeAtBoneOnFrame(em, 40, 0, HERM_BONE_LEG_B1);
            Em_PlaySeAtBoneOnFrame(em, 58, 19, HERM_BONE_CLAW_B);
            Em_PlaySeAtBoneOnFrame(em, 32, 13, HERM_BONE_CLAW_A);
            break;
        case EM_MOTION_ID(0, HERM_MOT_FLINCH_CLAW):
            Em_PlaySeAtBoneOnFrame(em, 2, 33, 14);
            Em_PlaySeAtBoneOnFrame(em, 46, 2, HERM_BONE_LEG_A1);
            Em_PlaySeAtBoneOnFrame(em, 88, 3, HERM_BONE_LEG_B2);
            Em_PlaySeAtBoneOnFrame(em, 52, 17, HERM_BONE_BODY);
            break;
        case EM_MOTION_ID(0, HERM_MOT_HELD):
            Em_PlaySeAtBoneOnFrame(em, 16, 38, 14);
            Em_PlaySeAtBoneOnFrame(em, 120, 38, 14);
            Em_PlaySeAtBoneOnFrame(em, 40, 17, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 62, 13, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 110, 17, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 132, 13, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 180, 17, HERM_BONE_BODY);
            break;
        case EM_MOTION_ID(0, HERM_MOT_DIE):
            Em_PlaySeAtBoneOnFrame(em, 220, Sound_RandomSe(10, 4, 11, 4), 14);
            Em_PlaySeAtBoneOnFrame(em, 270, Sound_RandomSe(11, 4, 12, 4), 14);
            Em_PlaySeAtBoneOnFrame(em, 2, 34, 14);
            Em_PlaySeAtBoneOnFrame(em, 168, 18, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 176, 19, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 190, 16, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 192, 17, HERM_BONE_BODY);
            Em_PlaySeAtBoneOnFrame(em, 58, 1, HERM_BONE_LEG_B2);
            Em_PlaySeAtBoneOnFrame(em, 69, 1, HERM_BONE_LEG_A2);
            break;
        case EM_MOTION_ID(0, HERM_MOT_DOWN_GET_UP):
            break;
        case EM_MOTION_ID(0, HERM_MOT_FLINCH_END):
            Em_PlaySeAtBoneOnFrame(em, 2, 31, 14);
            Em_PlaySeAtBoneOnFrame(em, 32, 13, HERM_BONE_CLAW_A);
            Em_PlaySeAtBoneOnFrame(em, 192, 16, HERM_BONE_BODY);
        default:
            break;
        }
    }
}

/*
 * Per-kind setup hook (Em_TypeSetup): a Shogun whose shell (part 2) is
 * broken (state >= 2) with fewer than two shell breaks gets areaTimer2 =
 * 1800 frames.
 */
void EmHermitaur_Setup(Em *em)
{
    EmHermitaurWork *w = (EmHermitaurWork *)&em->work;

    if ((em->type == EM_KIND_SHOGUN_CEANATAUR) && ((s32) em->parts[2].state >= 2) && ((s32) w->shellBreakCount < 2)) {
        em->areaTimer2 = 1800;
    }
}

/*
 * Places the monster (a test layout when the quest stage is 0; in the
 * Shogun's intro quests, while EVENT_FLAG_SHOGUN_CEANATAUR_SCENE is clear, on the
 * AREA_VOLCANO_5 ceiling in EM_STANCE_AIR), sets hp to the computed maximum,
 * resets lowHp, guardClankTimer and shellBreakCount, and gives the Shogun
 * its shell variant (level % 3). Unless keepState is set it also sets the
 * first motion and state (small kinds of variant 0: HERM_MOT_BURIED and
 * HERM_ATK_AMBUSH_WAIT; otherwise HERM_MOT_IDLE and HERM_IDLE). Then it sets
 * up the parts, lastPartState and each kind's fxFlags.
 */
void EmHermitaur_Init(Em *em, s16 keepState)
{
    EmHermitaurWork *w = (EmHermitaurWork *)&em->work;
    s16 maxHp;

    if (gQuest.stage == 0) {
        switch (gScene.area) {
        case AREA_FOREST_HILLS_4:
            em->posX = 9500.0f + 150.0f * (f32) (em->slotNo / 8);
            em->posY = 0.0f;
            em->posZ = 10000.0f + 150.0f * (f32) (em->slotNo & 7);
            break;
        default:
            em->posX = 5000.0f - 80.0f * (f32) (u32) em->slotNo;
            em->posY = 0.0f;
            em->posZ = 5000.0f - 80.0f * (f32) (u32) em->slotNo;
            break;
        }
    }
    em->blind = 0;
    em->stance = EM_STANCE_GROUND;
    em->unk_InitFlag = 1;
    em->gender = 0;
    maxHp = Em_ComputeMaxHp(em, 0);
    em->hp = maxHp;
    em->hpMax = maxHp;
    w->lowHp = 0;
    w->guardClankTimer = 0;
    w->shellBreakCount = 0;
    em->scriptPending = 1;
    em->surfaceY = 1500.0f;
    em->motionSpeed = 1.0f;
    if (em->type == EM_KIND_SHOGUN_CEANATAUR) {
        em->variant = em->level % 3;
    }
    if (em->type == EM_KIND_SHOGUN_CEANATAUR) {
        switch (gQuest.stage) {
        case QUEST_ID_SHOGUN_CEANATAUR_SCENE_1:
        case QUEST_ID_SHOGUN_CEANATAUR_SCENE_2:
            if (Save_TestEventFlag(EVENT_FLAG_SHOGUN_CEANATAUR_SCENE) == 0) {
                em->area = Area_IdForCurrentTime(AREA_VOLCANO_5);
                em->posX = 10100.0f;
                em->posY = 1500.0f;
                em->posZ = 6580.0f;
                em->rotY = 0;
                em->stance = EM_STANCE_AIR;
                em->surfaceY = 1500.0f;
                em->ceilingTimer = 5;
            }
            break;
        }
    }
    if (keepState == 0) {
        if (em->variant == 0) {
            if (em->type == EM_KIND_HERMITAUR || em->type == EM_KIND_CEANATAUR) {
                Em_SetMotionRow(em, HERM_MOT_BURIED, 0, 0);
                EmHermitaur_ChangeState(em, EM_GROUP_ATTACK, HERM_ATK_AMBUSH_WAIT, EM_SYNC_LOCAL);
            } else {
                Em_SetMotionRow(em, HERM_MOT_IDLE, 0, 0);
                EmHermitaur_ChangeState(em, EM_GROUP_IDLE, HERM_IDLE, EM_SYNC_LOCAL);
            }
        } else {
            Em_SetMotionRow(em, HERM_MOT_IDLE, 0, 0);
            EmHermitaur_ChangeState(em, EM_GROUP_IDLE, HERM_IDLE, EM_SYNC_LOCAL);
        }
    }
    Em_InitParts(em);
    w->lastPartState = em->parts[2].state;
    em->fxFlags = 0;
    switch (em->type) {
    case EM_KIND_DAIMYO_HERMITAUR:
        em->fxFlags |= 6;
        break;
    case EM_KIND_HERMITAUR:
    case EM_KIND_CEANATAUR:
    default:
        em->fxFlags |= 7;
        break;
    case EM_KIND_SHOGUN_CEANATAUR:
        em->fxFlags = 0;
        break;
    }
}

/*
 * Callback slot 5 (EmVt.appear): EmHermitaur_Init(em, 1), which keeps
 * motion and state.
 */
void EmHermitaur_Reinit(Em *em)
{
    EmHermitaur_Init(em, 1);
}

/*
 * The family's start entry: EmHermitaur_Init(em, 0), which also sets the
 * first motion and state.
 */
void EmHermitaur_Start(Em *em)
{
    EmHermitaur_Init(em, 0);
}

/*
 * Returns rageFxTimer as a fraction of its start value: divided by 8 when
 * calm, by 5 when enraged. No caller seen.
 */
f32 EmHermitaur_RageFxRatio(Em *em)
{
    EmHermitaurWork *hermWork = (EmHermitaurWork *)&em->work;

    if (em->rage == 0) {
        return (f32) hermWork->rageFxTimer / 8.0f;
    }
    return (f32) hermWork->rageFxTimer / 5.0f;
}

/*
 * Per-frame update: counts guardClankTimer down and runs the Shogun's
 * rage-change countdown and sound. For the Daimyo and the Shogun only, it
 * also runs the ceiling probe (Shogun), the area trigger and hold, the
 * per-map need decay (while not enraged and noAlert is not 1) and the lowHp
 * flag. Then the Shogun's rage from broken claws (parts 5, 6), the reaction
 * to Em_UpdateDamage's result, the quest intro scenes or the pending script,
 * the state (run twice on stateRequest), and last the part-break bookkeeping
 * (the Daimyo's shell breaks when both claws are broken; the Shogun's shell
 * breaks, shellBreakCount and fxFlags).
 */
void EmHermitaur_Update(Em *em)
{
    f32 pos[3];
    u8 dmgInfo[4];
    EmHermitaurWork *w = (EmHermitaurWork *)&em->work;
    s32 dmg;
    u8 brokenAt;
    f32 lowHpRatio;

    if (w->guardClankTimer > 0) {
        w->guardClankTimer -= 1;
    }
    Em_UpdateCombatTimer(em);
    if (em->type == EM_KIND_SHOGUN_CEANATAUR) {
        if (w->lastRage != em->rage) {
            if (em->rage == 0) {
                w->rageFxTimer = 8;
            } else {
                w->rageFxTimer = 5;
                Em_BonePos(em, HERM_BONE_CLAW_B, pos);
                Em_PlaySeChecked(em, HERM_SE_RAGE_CLAW, 0, pos, 6, 0);
            }
        } else if (w->rageFxTimer > 0) {
            if (em->rage != 0 && w->rageFxTimer == 2) {
                Em_BonePos(em, HERM_BONE_CLAW_A, pos);
                Em_PlaySeChecked(em, HERM_SE_RAGE_CLAW, 0, pos, 6, 0);
            }
            w->rageFxTimer -= 1;
        }
        w->lastRage = em->rage;
    }
    if (em->type != EM_KIND_HERMITAUR && em->type != EM_KIND_CEANATAUR) {
        if (em->type == EM_KIND_SHOGUN_CEANATAUR) {
            if (gScene.area == em->area) {
                em->hasCeiling = AreaGrid2_GetCeiling(&em->posX, &em->surfaceY, (struct AreaTri *) ((u8 *)w + 72));
            } else if (EmHermitaur_AreaHasCeiling(em->area) == 1) {
                em->hasCeiling = 1;
                em->surfaceY = 1500.0f;
            } else {
                em->hasCeiling = 0;
            }
        } else {
            em->hasCeiling = 0;
        }
        Em_RaisePlayerAreaTriggerLax(em);
        Em_HoldPosNoPlayers(em, 0, 1);
        if (em->noAlert != 1 && em->rage == 0) {
            switch ((u16) Map_DayId(gScene.map)) {
            case MAP_DESERT:
                if (em->type == EM_KIND_DAIMYO_HERMITAUR) {
                    Em_CheckLowHp(em, 0.2f);
                    Em_DecayNeed1Only(em);
                }
                break;
            case MAP_JUNGLE:
                if (em->type == EM_KIND_DAIMYO_HERMITAUR) {
                    Em_CheckLowHp(em, 0.2f);
                    Em_DecayNeed1(em);
                }
                break;
            case MAP_SWAMP:
                if (em->type == EM_KIND_SHOGUN_CEANATAUR) {
                    Em_DecayNeed1(em);
                }
                break;
            case MAP_VOLCANO:
                break;
            }
        }
        lowHpRatio = 0.2f;
        if (em->hp < (s16) (em->hpMax * lowHpRatio)) {
            w->lowHp = 1;
        } else {
            w->lowHp = 0;
        }
    }
    switch (em->stateGroup) {
    default:
        if (em->type == EM_KIND_SHOGUN_CEANATAUR && (em->parts[5].state > 0 || em->parts[6].state > 0)) {
            if (em->rage == 0) {
                Em_CheckRageByHp(em, em->rageThreshold);
            } else {
                em->rage = 1;
                em->rageTimer = 1800;
            }
        }
        break;
    case 5:
        em->rage = 0;
        break;
    }
    dmg = Em_UpdateDamage(em, dmgInfo) & 0xFF;
    switch (dmg) {
    case EM_DMG_NONE:
    case EM_DMG_ATTACK9:
    case EM_DMG_ATTACK6:
    case EM_DMG_FLASH:
        break;
    case EM_DMG_KILLED:
    case EM_DMG_DEAD:
        EmHermitaur_ChangeState(em, EM_GROUP_DEFEAT, HERM_DEFEAT_DIE, EM_SYNC_OWNER);
        break;
    case EM_DMG_STATUS_IN_AIR:
        if (em->stateGroup != EM_GROUP_REACT || em->state != HERM_REACT_FALL) {
            EmHermitaur_ChangeState(em, EM_GROUP_REACT, HERM_REACT_FALL, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_STATUS4:
        if (em->stateGroup != EM_GROUP_REACT || em->state != HERM_REACT_FALL) {
            Em_SetStatus4Timer(em);
            EmHermitaur_ChangeState(em, EM_GROUP_REACT, HERM_REACT_STATUS4, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_CAPTURED:
        EmHermitaur_ChangeState(em, EM_GROUP_EVENT, HERM_EVENT_CAPTURED, EM_SYNC_OWNER);
        break;
    case EM_DMG_STATUS2:
        if (em->stateGroup != EM_GROUP_REACT || em->state != HERM_REACT_FALL) {
            Em_SetStatus2Timer(em);
            EmHermitaur_ChangeState(em, EM_GROUP_IDLE, HERM_IDLE_STATUS2_COLLAPSE, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_HIT_LYING:
        switch (em->state) {
        case 14:
            EmHermitaur_ChangeState(em, EM_GROUP_IDLE, HERM_IDLE_STATUS1_END, EM_SYNC_OWNER);
            break;
        default:
            EmHermitaur_ChangeState(em, EM_GROUP_IDLE, HERM_IDLE_STATUS2_END, EM_SYNC_OWNER);
            break;
        }
        break;
    case EM_DMG_PART8_DOWN:
        EmHermitaur_ChangeState(em, EM_GROUP_REACT, HERM_REACT_PART_BREAK, EM_SYNC_OWNER);
        break;
    case EM_DMG_ATTACK5:
        if (em->type == EM_KIND_SHOGUN_CEANATAUR) {
            break;
        }
    case EM_DMG_ATTACK5_FLINCH:
        if (em->type == EM_KIND_HERMITAUR || em->type == EM_KIND_CEANATAUR) {
            if (em->stance != EM_STANCE_AIR && em->stateArmor == 0) {
                EmHermitaur_ChangeState(em, EM_GROUP_REACT, HERM_REACT_WIND_FLINCH, EM_SYNC_OWNER);
                break;
            }
        } else if (em->stance != EM_STANCE_AIR && em->stateArmor == 0 && (s8) em->hermitTimer != 0) {
            EmHermitaur_ChangeState(em, EM_GROUP_REACT, HERM_REACT_GUARD_RECOIL, EM_SYNC_OWNER);
            break;
        }
        if ((u8) dmg == 15) {
            break;
        }
    case EM_DMG_PART_FLINCH:
        Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
        switch (em->flinchPart) {
        default:
        case 0:
        case 1:
        case 7:
            if (em->stance == EM_STANCE_AIR) {
                EmHermitaur_ChangeState(em, EM_GROUP_REACT, HERM_REACT_FALL, EM_SYNC_OWNER);
            } else {
                EmHermitaur_ChangeState(em, EM_GROUP_REACT, HERM_REACT_FLINCH, EM_SYNC_OWNER);
            }
            break;
        case 2:
            switch (em->type) {
            case EM_KIND_DAIMYO_HERMITAUR:
                if (em->stance == EM_STANCE_AIR) {
                    EmHermitaur_ChangeState(em, EM_GROUP_REACT, HERM_REACT_FALL, EM_SYNC_OWNER);
                } else {
                    EmHermitaur_ChangeState(em, EM_GROUP_REACT, HERM_REACT_FLINCH, EM_SYNC_OWNER);
                }
                break;
            default:
            case EM_KIND_HERMITAUR:
            case EM_KIND_CEANATAUR:
                EmHermitaur_ChangeState(em, EM_GROUP_REACT, HERM_REACT_FLINCH, EM_SYNC_OWNER);
                break;
            case EM_KIND_SHOGUN_CEANATAUR:
                if (em->stance == EM_STANCE_AIR) {
                    EmHermitaur_ChangeState(em, EM_GROUP_REACT, HERM_REACT_FALL, EM_SYNC_OWNER);
                } else if (em->parts[2].state == 2) {
                    EmHermitaur_ChangeState(em, EM_GROUP_REACT, HERM_REACT_SPIN_FLINCH, EM_SYNC_OWNER);
                } else {
                    EmHermitaur_ChangeState(em, EM_GROUP_REACT, HERM_REACT_FLINCH, EM_SYNC_OWNER);
                }
                break;
            }
            break;
        case 3:
            if (em->stance == EM_STANCE_AIR) {
                EmHermitaur_ChangeState(em, EM_GROUP_REACT, HERM_REACT_FALL, EM_SYNC_OWNER);
            } else {
                EmHermitaur_ChangeState(em, EM_GROUP_REACT, HERM_REACT_KNOCK_DOWN_A, EM_SYNC_OWNER);
            }
            break;
        case 4:
            if (em->stance == EM_STANCE_AIR) {
                EmHermitaur_ChangeState(em, EM_GROUP_REACT, HERM_REACT_FALL, EM_SYNC_OWNER);
            } else {
                EmHermitaur_ChangeState(em, EM_GROUP_REACT, HERM_REACT_KNOCK_DOWN_B, EM_SYNC_OWNER);
            }
            break;
        case 5:
            if (em->stance == EM_STANCE_AIR) {
                EmHermitaur_ChangeState(em, EM_GROUP_REACT, HERM_REACT_FALL, EM_SYNC_OWNER);
            } else {
                brokenAt = 2;
                if (em->type == EM_KIND_SHOGUN_CEANATAUR) {
                    brokenAt = 1;
                }
                if (em->parts[5].state >= brokenAt) {
                    switch (em->type) {
                    case EM_KIND_DAIMYO_HERMITAUR:
                        EmHermitaur_ChangeState(em, EM_GROUP_REACT, HERM_REACT_FLINCH_CLAW_BROKEN_A, EM_SYNC_OWNER);
                        break;
                    default:
                    case EM_KIND_HERMITAUR:
                    case EM_KIND_CEANATAUR:
                        EmHermitaur_ChangeState(em, EM_GROUP_REACT, HERM_REACT_FLINCH_CLAW, EM_SYNC_OWNER);
                        break;
                    case EM_KIND_SHOGUN_CEANATAUR:
                        EmHermitaur_ChangeState(em, EM_GROUP_REACT, HERM_REACT_RECOIL_CLAW_A, EM_SYNC_OWNER);
                        break;
                    }
                } else {
                    EmHermitaur_ChangeState(em, EM_GROUP_REACT, HERM_REACT_FLINCH_CLAW, EM_SYNC_OWNER);
                }
            }
            break;
        case 6:
            if (em->stance == EM_STANCE_AIR) {
                EmHermitaur_ChangeState(em, EM_GROUP_REACT, HERM_REACT_FALL, EM_SYNC_OWNER);
            } else {
                brokenAt = 2;
                if (em->type == EM_KIND_SHOGUN_CEANATAUR) {
                    brokenAt = 1;
                }
                if (em->parts[6].state >= brokenAt) {
                    switch (em->type) {
                    case EM_KIND_DAIMYO_HERMITAUR:
                        EmHermitaur_ChangeState(em, EM_GROUP_REACT, HERM_REACT_FLINCH_CLAW_BROKEN_B, EM_SYNC_OWNER);
                        break;
                    default:
                    case EM_KIND_HERMITAUR:
                    case EM_KIND_CEANATAUR:
                        EmHermitaur_ChangeState(em, EM_GROUP_REACT, HERM_REACT_FLINCH_CLAW, EM_SYNC_OWNER);
                        break;
                    case EM_KIND_SHOGUN_CEANATAUR:
                        EmHermitaur_ChangeState(em, EM_GROUP_REACT, HERM_REACT_RECOIL_CLAW_B, EM_SYNC_OWNER);
                        break;
                    }
                } else {
                    EmHermitaur_ChangeState(em, EM_GROUP_REACT, HERM_REACT_FLINCH_CLAW, EM_SYNC_OWNER);
                }
            }
            break;
        }
        break;
    case EM_DMG_HIT_MODE3:
        if (em->stance == EM_STANCE_AIR) {
            EmHermitaur_ChangeState(em, EM_GROUP_REACT, HERM_REACT_FALL, EM_SYNC_OWNER);
        } else {
            EmHermitaur_ChangeState(em, EM_GROUP_REACT, HERM_REACT_FLINCH, EM_SYNC_OWNER);
            em->scriptPending = 0;
        }
        break;
    case EM_DMG_HIT:
        if (em->type == EM_KIND_DAIMYO_HERMITAUR && (s8) em->hermitTimer != 0) {
            Em_PlaySeChecked(em, HERM_SE_GUARD_HIT, 0, &em->posX, 6, 0);
            w->guardClankTimer = 30;
        }
        break;
    case EM_DMG_STATUS5:
        if (em->stateGroup != EM_GROUP_REACT || (em->state != HERM_REACT_FALL && em->state != HERM_REACT_STUN_B && em->state != HERM_REACT_STUN_A)) {
            Em_SetStatus5Timer(em);
            if (em->hitAngle - em->rotY > (ANGLE_TURN / 2)) {
                EmHermitaur_ChangeState(em, EM_GROUP_REACT, HERM_REACT_STUN_A, EM_SYNC_OWNER);
            } else {
                EmHermitaur_ChangeState(em, EM_GROUP_REACT, HERM_REACT_STUN_B, EM_SYNC_OWNER);
            }
        }
        break;
    case EM_DMG_ATTACK13:
        if (em->type != EM_KIND_HERMITAUR && em->type != EM_KIND_CEANATAUR) {
            Em_ResetKindTimer(em);
            EmHermitaur_ChangeState(em, EM_GROUP_REACT, HERM_REACT_HOLD2, EM_SYNC_OWNER);
        }
        break;
    }
    switch (gQuest.stage) {
    case QUEST_ID_DAIMYO_HERMITAUR_SCENE_1:
    case QUEST_ID_DAIMYO_HERMITAUR_SCENE_2:
        if (em->type == EM_KIND_DAIMYO_HERMITAUR && Area_DayId(em->area) == AREA_DESERT_9 && Save_TestEventFlag(EVENT_FLAG_DAIMYO_HERMITAUR_SCENE) == 0) {
            if (Area_DayId(gScene.area) == AREA_DESERT_9 && gScene.eventRunning == 1 && em->stateGroup != EM_GROUP_EVENT) {
                EmHermitaur_ChangeState(em, EM_GROUP_EVENT, HERM_EVENT_DAIMYO_INTRO, EM_SYNC_SEND);
            }
            break;
        }
        goto other;
    case QUEST_ID_SHOGUN_CEANATAUR_SCENE_1:
    case QUEST_ID_SHOGUN_CEANATAUR_SCENE_2:
        if (em->type == EM_KIND_SHOGUN_CEANATAUR && Area_DayId(em->area) == AREA_VOLCANO_5 && Save_TestEventFlag(EVENT_FLAG_SHOGUN_CEANATAUR_SCENE) == 0) {
            if (Area_DayId(gScene.area) == AREA_VOLCANO_5 && gScene.eventRunning == 1 && em->stateGroup != EM_GROUP_EVENT) {
                EmHermitaur_ChangeState(em, EM_GROUP_EVENT, HERM_EVENT_SHOGUN_INTRO, EM_SYNC_SEND);
            }
            break;
        }
    default:
    other:
        Em_UpdatePendingScript(em);
        break;
    }
    Em_ClearHitReactMode(em);
    EmHermitaur_RunState(em);
    if (em->stateRequest != 0) {
        EmHermitaur_RunState(em);
        em->stateRequest = 0;
    }
    if (em->type == EM_KIND_DAIMYO_HERMITAUR && em->parts[5].state >= 2 && em->parts[6].state >= 2) {
        Em_BreakPart(em, 2);
    }
    if (em->type == EM_KIND_SHOGUN_CEANATAUR) {
        if (em->parts[2].state >= 2) {
            em->hitzoneSet = 1;
        }
        if (w->lastPartState != em->parts[2].state) {
            w->lastPartState = em->parts[2].state;
            if (em->parts[2].state == 2) {
                if (w->shellBreakCount == 0) {
                    em->areaTimer2 = 1800;
                }
                w->shellBreakCount++;
                em->fxFlags |= 1;
                switch (em->variant) {
                case 0:
                    Em_BreakPart(em, 1);
                    break;
                case 1:
                    Em_BreakPart(em, 2);
                    break;
                case 2:
                    Em_BreakPart(em, 3);
                    break;
                }
            }
        }
        if (em->parts[5].state > 0 || em->parts[6].state > 0) {
            Em_BreakPart(em, 4);
        }
        if (em->parts[5].state > 0) {
            em->fxFlags |= 2;
        }
        if (em->parts[6].state > 0) {
            em->fxFlags |= 4;
        }
    }
}

/*
 * Plays HERM_MOT_IDLE once, then EmHermitaur_EndToIdle (back to HERM_IDLE,
 * which EmHermitaur_ChangeState turns into HERM_IDLE_ALERT in combat).
 */
void EmHermitaur_Idle(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, HERM_MOT_IDLE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmHermitaur_EndToIdle(em);
        }
        break;
    }
}

/* Plays HERM_MOT_IDLE_ALERT once, then EmHermitaur_EndToIdle. */
void EmHermitaur_IdleAlert(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, HERM_MOT_IDLE_ALERT, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmHermitaur_EndToIdle(em);
        }
        break;
    }
}

/*
 * No flinch; plays HERM_MOT_EAT, and at its end variant 1 adds 5% of max hp,
 * fills need 1 and clears need1State, and the Daimyo leaves a timed mine
 * (EmMine_CreateTimed0); then EmHermitaur_EndToIdle. A Ceanataur calls
 * EmHermitaur_EndToIdle at the top of every call, not only on entry.
 */
void EmHermitaur_Feed(Em *em, s32 work, s32 variant)
{
    if (em->type == EM_KIND_CEANATAUR) {
        EmHermitaur_EndToIdle(em);
    }
    Em_SetHitReactMode(em, EM_HIT_REACT_NO_FLINCH);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, HERM_MOT_EAT, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            if (variant == 1) {
                Em_AddHp(em, (s32) (0.05f * (f32) em->hpMax));
                Em_AddNeed1(em, em->need1Max);
                Em_ClearNeed1State(em);
            }
            if (em->type == EM_KIND_DAIMYO_HERMITAUR) {
                EmMine_CreateTimed0(em, &em->posX);
            }
            EmHermitaur_EndToIdle(em);
        }
        break;
    }
}

/*
 * Plays HERM_MOT_THREAT once, then EmHermitaur_EndToIdleAlert (the alert
 * idle). Serves the rage starts.
 */
void EmHermitaur_Threat(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, HERM_MOT_THREAT, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmHermitaur_EndToIdleAlert(em);
        }
        break;
    }
}

/*
 * EM_STANCE_GROUND, no look-at; plays HERM_MOT_IDLE_ACTION_A or _B by
 * stateRand bit 0, then EmHermitaur_EndToIdle.
 */
void EmHermitaur_IdleAction(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        if (em->stateRand & 1) {
            Em_SetMotion(em, HERM_MOT_IDLE_ACTION_A, 0, 0);
            break;
        }
        Em_SetMotion(em, HERM_MOT_IDLE_ACTION_B, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmHermitaur_EndToIdle(em);
        }
        break;
    }
}

/*
 * Plays HERM_MOT_TURN_LEFT (the calm left turn) once, without rotating, then
 * EmHermitaur_EndToIdle.
 */
void EmHermitaur_IdleTurnLeftMotion(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, HERM_MOT_TURN_LEFT, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmHermitaur_EndToIdle(em);
        }
        break;
    }
}

/*
 * Sets targetAngle to the goal and plays HERM_MOT_TURN_RIGHT_ALERT or
 * _LEFT_ALERT by the side (root motion turns), then EmHermitaur_EndToIdle.
 */
void EmHermitaur_TurnToGoalOnce(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->targetAngle = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        if (((em->targetAngle - em->rotY) & 0xFFFF) < 0x8001) {
            Em_SetMotion(em, HERM_MOT_TURN_RIGHT_ALERT, 0, 0);
            break;
        }
        Em_SetMotion(em, HERM_MOT_TURN_LEFT_ALERT, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmHermitaur_EndToIdle(em);
        }
        break;
    }
}

/*
 * blind 2, HERM_MOT_LIE_DOWN then HERM_MOT_LIE until em->timer runs out;
 * variant 1 sets the timer to 5400 frames, regains up to half the hp and
 * clears the low-hp need at the end. Then to the matching end state (0:
 * HERM_IDLE_STATUS2_END, 1: HERM_IDLE_REST_END, 2: HERM_IDLE_STATUS1_END).
 */
void EmHermitaur_LieDown(Em *em, s32 work, s32 variant)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, HERM_MOT_LIE_DOWN, 0, 0);
        if (variant == 1) {
            em->timer = 5400;
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, HERM_MOT_LIE, 0, 0);
        }
        break;
    case 2:
        if (variant == 1) {
            Em_RegenHp(em, 1, (s32) (0.5f * (f32) em->hpMax), 10);
        }
        if (--em->timer <= 0) {
            em->step++;
            if (variant == 1) {
                Em_ClearLowHpNeed(em);
            }
            switch (variant) {
            case 0:
                EmHermitaur_ChangeState(em, EM_GROUP_IDLE, HERM_IDLE_STATUS2_END, EM_SYNC_OWNER_FLAGGED);
                break;
            case 1:
                EmHermitaur_ChangeState(em, EM_GROUP_IDLE, HERM_IDLE_REST_END, EM_SYNC_OWNER_FLAGGED);
                break;
            case 2:
                EmHermitaur_ChangeState(em, EM_GROUP_IDLE, HERM_IDLE_STATUS1_END, EM_SYNC_OWNER_FLAGGED);
                break;
            }
        }
        break;
    case 3:
        if (em->remote == 0) {
            switch (variant) {
            case 0:
                EmHermitaur_ChangeState(em, EM_GROUP_IDLE, HERM_IDLE_STATUS2_END, EM_SYNC_OWNER_FLAGGED);
                break;
            case 1:
                EmHermitaur_ChangeState(em, EM_GROUP_IDLE, HERM_IDLE_REST_END, EM_SYNC_OWNER_FLAGGED);
                break;
            case 2:
                EmHermitaur_ChangeState(em, EM_GROUP_IDLE, HERM_IDLE_STATUS1_END, EM_SYNC_OWNER_FLAGGED);
                break;
            }
        }
        break;
    }
}

/*
 * blind 2; plays HERM_MOT_GET_UP, clears blind, then EmHermitaur_EndToIdle.
 * Ends status 2, status 1 and the rest.
 */
void EmHermitaur_GetUp(Em *em)
{
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, HERM_MOT_GET_UP, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            EmHermitaur_EndToIdle(em);
        }
        break;
    }
}

/*
 * Plays HERM_MOT_THREAT, clears noReact at its end, then
 * EmHermitaur_EndToIdle. Its extra arguments are ignored.
 */
void EmHermitaur_ShakeOff(Em *em, s32 work, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, HERM_MOT_THREAT, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->noReact = 0;
            em->step++;
            EmHermitaur_EndToIdle(em);
        }
        break;
    }
}

/*
 * EM_STANCE_GROUND, no look-at; HERM_MOT_IDLE for 30 frames, then
 * EmHermitaur_EndToIdle.
 */
void EmHermitaur_Wait(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, HERM_MOT_IDLE, 0, 0);
        em->timer = 30;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmHermitaur_EndToIdle(em);
        }
        break;
    }
}

/*
 * EM_STANCE_GROUND, no look-at; HERM_MOT_IDLE_ALERT for 30 frames, then
 * EmHermitaur_EndToIdle.
 */
void EmHermitaur_WaitAlert(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, HERM_MOT_IDLE_ALERT, 0, 0);
        em->timer = 30;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmHermitaur_EndToIdle(em);
        }
        break;
    }
}

/*
 * No flinch; EM_STANCE_GROUND, HERM_MOT_EAT with sight row 1 and reaction
 * set 1; when motion frame 476 passes it changes to HERM_IDLE_EAT_END (whose
 * entry in EmHermitaur_ChangeState calls Em_StartGoalAction).
 */
void EmHermitaur_EatStart(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_NO_FLINCH);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, HERM_MOT_EAT, 0, 0);
        Em_SelectSightRow(em, 1);
        Em_LoadReactionSet(em, 1);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 476.0f, 0U) != 0) {
            em->step++;
            EmHermitaur_ChangeState(em, EM_GROUP_IDLE, HERM_IDLE_EAT_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmHermitaur_ChangeState(em, EM_GROUP_IDLE, HERM_IDLE_EAT_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * Restores sight row 0 and reaction set 0, lets the eating motion run out,
 * then EmHermitaur_EndToIdle.
 */
void EmHermitaur_EatEnd(Em *em)
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
            EmHermitaur_EndToIdle(em);
        }
        break;
    }
}

/*
 * EM_GROUP_IDLE dispatcher: saves prevStateGroup and prevState, then runs
 * the current state's function.
 */
void EmHermitaur_RunIdleState(Em *em, EmWork *work)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->state) {
    case HERM_IDLE_STAND:
        EmHermitaur_Idle(em);
        break;
    case HERM_IDLE:
        EmHermitaur_Idle(em);
        break;
    case HERM_IDLE_ALERT:
        EmHermitaur_IdleAlert(em);
        break;
    case HERM_IDLE_FEED:
        EmHermitaur_Feed(em, (s32) work, 0);
        break;
    case HERM_IDLE_THREAT:
        EmHermitaur_Threat(em);
        break;
    case HERM_IDLE_ACTION:
        EmHermitaur_IdleAction(em);
        break;
    case HERM_IDLE_ACTION_B:
        EmHermitaur_IdleTurnLeftMotion(em);
        break;
    case HERM_IDLE_TURN_TO_GOAL:
        EmHermitaur_TurnToGoalOnce(em);
        break;
    case HERM_IDLE_STATUS2_COLLAPSE:
        EmHermitaur_LieDown(em, (s32) work, 0);
        break;
    case HERM_IDLE_STATUS2_END:
        EmHermitaur_GetUp(em);
        break;
    case HERM_IDLE_STATUS4_SHAKE_OFF:
        EmHermitaur_ShakeOff(em, (s32) work, 0);
        break;
    case HERM_IDLE_STUN_SHAKE_OFF:
        EmHermitaur_ShakeOff(em, (s32) work, 1);
        break;
    case HERM_IDLE_HOLD2_SHAKE_OFF:
        EmHermitaur_ShakeOff(em, (s32) work, 2);
        break;
    case HERM_IDLE_REST:
        EmHermitaur_LieDown(em, (s32) work, 1);
        break;
    case HERM_IDLE_STATUS1_COLLAPSE:
        EmHermitaur_LieDown(em, (s32) work, 2);
        break;
    case HERM_IDLE_STATUS1_END:
        EmHermitaur_GetUp(em);
        break;
    case HERM_IDLE_REST_END:
        EmHermitaur_GetUp(em);
        break;
    case HERM_IDLE_THREAT_RAGE:
        EmHermitaur_Threat(em);
        break;
    case HERM_IDLE_THREAT_PENDING_RAGE:
        EmHermitaur_Threat(em);
        break;
    case HERM_IDLE_WAIT:
        EmHermitaur_Wait(em);
        break;
    case HERM_IDLE_WAIT_ALERT:
        EmHermitaur_WaitAlert(em);
        break;
    case HERM_IDLE_EAT_START:
        EmHermitaur_EatStart(em);
        break;
    case HERM_IDLE_EAT_END:
        EmHermitaur_EatEnd(em);
        break;
    case HERM_IDLE_FEED_REFILL:
        EmHermitaur_Feed(em, (s32) work, 1);
        break;
    }
}

/*
 * Turns to the goal angle plus 0, ANGLE_TURN / 2, ANGLE_TURN / 4 or ANGLE_TURN * 3 / 4 (variant 0-3) with
 * HERM_MOT_TURN_RIGHT/_LEFT, ANGLE_TURN / 2 per 80 frames between motion frames 8
 * and 88, restarting the motion until within a step; then
 * EmHermitaur_EndToIdle.
 */
void EmHermitaur_TurnInPlace(Em *em, EmWork *work, s32 variant)
{
    EmHermitaurWork *hermWork = (EmHermitaurWork *)work;
    u32 turnStep;
    u32 angleDiff;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        switch (variant) {
        case 0:
            hermWork->goalAngle = Math_AngleXZ(&em->posX, em->goal);
            break;
        case 1:
            hermWork->goalAngle = Math_AngleXZ(&em->posX, em->goal) + ANGLE_TURN / 2;
            break;
        case 2:
            hermWork->goalAngle = Math_AngleXZ(&em->posX, em->goal) + ANGLE_TURN / 4;
            break;
        case 3:
            hermWork->goalAngle = Math_AngleXZ(&em->posX, em->goal) + ANGLE_TURN * 3 / 4;
            break;
        }
        if (((hermWork->goalAngle - em->rotY) & 0xFFFF) >= 0x8000U) {
            Em_SetMotion(em, HERM_MOT_TURN_RIGHT, 0, 0);
        } else {
            Em_SetMotion(em, HERM_MOT_TURN_LEFT, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].blend == 0 && EmMotion_ReachedFrame(em, 0, 8.0f)) {
            turnStep = 409.6f * em->motionSpeed;
            angleDiff = (u16) (hermWork->goalAngle - *(u16 *) &em->rotY);
            if (em->motion[0].playing == 0 || EmMotion_ReachedFrame(em, 0, 88.0f)) {
                if ((u16) (angleDiff + turnStep) < turnStep * 2) {
                    em->step++;
                    Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
                    EmHermitaur_EndToIdle(em);
                } else {
                    Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
                    if (angleDiff >= ANGLE_TURN / 2) {
                        Em_SetMotion(em, HERM_MOT_TURN_RIGHT, 6, 0);
                    } else {
                        Em_SetMotion(em, HERM_MOT_TURN_LEFT, 6, 0);
                    }
                }
            } else if ((u16) (angleDiff + turnStep) < turnStep * 2) {
                em->rotY = hermWork->goalAngle;
            } else if (angleDiff < (ANGLE_TURN / 2)) {
                em->rotY = (u16) (em->rotY + turnStep);
            } else {
                em->rotY = (u16) (em->rotY - turnStep);
            }
        }
        break;
    }
    Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
}

/*
 * HERM_MOT_WALK (variant 0) or HERM_MOT_WALK_BACK (1); steers to the goal
 * (backward for 1) while steerToGoal is set and ends when the root motion
 * has used up distLeft (Hermitaur/Ceanataur chasing a target: within 200 or
 * on the timer).
 */
void EmHermitaur_Walk(Em *em, EmWork *work, s32 variant)
{
    f32 delta[3];
    u16 turnStep;
    u16 angleDiff;

    switch (em->step) {
    case 0:
        em->step++;
        if (variant == 0) {
            Em_SetMotion(em, HERM_MOT_WALK, 0, 0);
        } else {
            Em_SetMotion(em, HERM_MOT_WALK_BACK, 0, 0);
        }
        break;
    case 1:
        if (((EmHermitaurWork *)work)->steerToGoal != 0) {
            if (em->type == EM_KIND_HERMITAUR || em->type == EM_KIND_CEANATAUR) {
                turnStep = 1024;
            } else {
                turnStep = 64;
            }
            angleDiff = Math_AngleXZ(&em->posX, em->goal) - em->rotY;
            if (variant != 0) {
                angleDiff += ANGLE_TURN / 2;
            }
            if (angleDiff <= ANGLE_TURN / 2) {
                if (angleDiff < turnStep) {
                    em->rotY += angleDiff;
                } else {
                    em->rotY += turnStep;
                }
            } else if (0x10000 - turnStep < angleDiff) {
                em->rotY += angleDiff;
            } else {
                em->rotY -= turnStep;
            }
        }
        if ((em->type == EM_KIND_HERMITAUR || em->type == EM_KIND_CEANATAUR) && em->curGoalKind == 1 && em->curGoalArg == 0) {
            if (Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) < 200.0f || --em->timer <= 0) {
                EmHermitaur_EndToIdle(em);
            }
        } else {
            Em_MotionDelta(em, delta);
            if (variant != 0) {
                delta[2] *= -1.0f;
            }
            ((EmHermitaurWork *)work)->distLeft -= delta[2];
            if (((EmHermitaurWork *)work)->distLeft <= 0.0f) {
                em->step++;
                EmHermitaur_EndToIdle(em);
            }
        }
        break;
    }
}

/*
 * direction 0 plays HERM_MOT_WALK_ALERT; 1 plays HERM_MOT_WALK_BACK_ALERT
 * and creates HermitaurAttack 9; 2 plays HERM_MOT_BACK_ATTACK_WALK and
 * creates HermitaurAttack 27. speedBonus: 10 for the big kinds, and for
 * Hermitaur/Ceanataur 200 when fast is 1, else 100. Steers and ends as
 * EmHermitaur_Walk does, except that only direction 1 counts its root motion
 * backward.
 */
void EmHermitaur_WalkAlert(Em *em, EmWork *work, s32 direction, s32 fast)
{
    f32 delta[3];
    u16 turnStep;
    u16 angleDiff;

    switch (em->step) {
    case 0:
        em->step++;
        switch (direction) {
        case 0:
            Em_SetMotion(em, HERM_MOT_WALK_ALERT, 0, 0);
            break;
        case 1:
            Em_SetMotion(em, HERM_MOT_WALK_BACK_ALERT, 0, 0);
            HermitaurAttack_Create(em, 9);
            break;
        case 2:
            Em_SetMotion(em, HERM_MOT_BACK_ATTACK_WALK, 0, 0);
            HermitaurAttack_Create(em, 27);
            break;
        }
        if (fast == 1) {
            if (em->type == EM_KIND_HERMITAUR || em->type == EM_KIND_CEANATAUR) {
                em->speedBonus = 200;
            } else {
                em->speedBonus = 10;
            }
        } else if (em->type == EM_KIND_HERMITAUR || em->type == EM_KIND_CEANATAUR) {
            em->speedBonus = 100;
        } else {
            em->speedBonus = 10;
        }
        break;
    case 1:
        if (((EmHermitaurWork *)work)->steerToGoal != 0) {
            if (em->type == EM_KIND_HERMITAUR || em->type == EM_KIND_CEANATAUR) {
                turnStep = 1024;
            } else {
                turnStep = 64;
            }
            angleDiff = Math_AngleXZ(&em->posX, em->goal) - em->rotY;
            if (direction != 0) {
                angleDiff += ANGLE_TURN / 2;
            }
            if (angleDiff <= ANGLE_TURN / 2) {
                if (angleDiff < turnStep) {
                    em->rotY += angleDiff;
                } else {
                    em->rotY += turnStep;
                }
            } else if (0x10000 - turnStep < angleDiff) {
                em->rotY += angleDiff;
            } else {
                em->rotY -= turnStep;
            }
        }
        if ((em->type == EM_KIND_HERMITAUR || em->type == EM_KIND_CEANATAUR) && em->curGoalKind == 1 && em->curGoalArg == 0) {
            if (Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) < 200.0f || --em->timer <= 0) {
                EmHermitaur_EndToIdle(em);
            }
        } else {
            Em_MotionDelta(em, delta);
            if (direction == 1) {
                delta[2] *= -1.0f;
            }
            ((EmHermitaurWork *)work)->distLeft -= delta[2];
            if (((EmHermitaurWork *)work)->distLeft <= 0.0f) {
                em->step++;
                EmHermitaur_EndToIdle(em);
            }
        }
        break;
    }
}

/*
 * Plays HERM_MOT_SIDESTEP_A (variant 0, 2, 4) or HERM_MOT_SIDESTEP_B. While
 * steerToGoal is set it faces 90 degrees off the goal (goal angle +0x4000
 * for variants 0 and 2, else +0xC000). It ends when the sideways root motion
 * has used up distLeft (small kinds chasing a target: within 200 or when the
 * timer runs out). Variants 2-5 set speedBonus (200 for small kinds, 10 for
 * big ones).
 */
void EmHermitaur_Sidestep(Em *em, EmWork *work, s32 variant)
{
    f32 delta[3];
    u16 turnStep;
    u16 faceAngle;
    u16 angleDiff;

    switch (em->step) {
    case 0:
        em->step++;
        if (variant == 0 || variant == 2 || variant == 4) {
            Em_SetMotion(em, HERM_MOT_SIDESTEP_A, 0, 0);
        } else {
            Em_SetMotion(em, HERM_MOT_SIDESTEP_B, 0, 0);
        }
        switch (variant) {
        case 2:
        case 3:
        case 4:
        case 5:
            if (em->type == EM_KIND_HERMITAUR || em->type == EM_KIND_CEANATAUR) {
                em->speedBonus = 200;
            } else {
                em->speedBonus = 10;
            }
            break;
        }
        break;
    case 1:
        if (((EmHermitaurWork *)work)->steerToGoal != 0) {
            if (em->type == EM_KIND_HERMITAUR || em->type == EM_KIND_CEANATAUR) {
                turnStep = 1024;
            } else {
                turnStep = 64;
            }
            if (variant == 0 || variant == 2) {
                faceAngle = Math_AngleXZ(&em->posX, em->goal) + ANGLE_TURN / 4;
            } else {
                faceAngle = Math_AngleXZ(&em->posX, em->goal) + ANGLE_TURN * 3 / 4;
            }
            angleDiff = faceAngle - em->rotY;
            if (angleDiff <= ANGLE_TURN / 2) {
                if (angleDiff < turnStep) {
                    em->rotY += angleDiff;
                } else {
                    em->rotY += turnStep;
                }
            } else if (0x10000 - turnStep < angleDiff) {
                em->rotY += angleDiff;
            } else {
                em->rotY -= turnStep;
            }
        }
        if ((em->type == EM_KIND_HERMITAUR || em->type == EM_KIND_CEANATAUR) && em->curGoalKind == 1 && em->curGoalArg == 0) {
            if (Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) < 200.0f || --em->timer <= 0) {
                EmHermitaur_EndToIdle(em);
            }
        } else {
            Em_MotionDelta(em, delta);
            if (variant != 0) {
                delta[0] *= -1.0f;
            }
            *(f32 *) &work->moveAngX -= Math_Absf(delta[0]);
            if (*(f32 *) &work->moveAngX <= 0.0f) {
                em->step++;
                EmHermitaur_EndToIdle(em);
            }
        }
        break;
    }
}

/*
 * As EmHermitaur_TurnInPlace with HERM_MOT_TURN_RIGHT_ALERT/_LEFT_ALERT,
 * ANGLE_TURN / 2 per 58 frames between motion frames 6 and 64.
 */
void EmHermitaur_TurnInPlaceAlert(Em *em, EmWork *work, s32 variant)
{
    EmHermitaurWork *hermWork = (EmHermitaurWork *)work;
    u32 turnStep;
    u32 angleDiff;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        switch (variant) {
        case 0:
            hermWork->goalAngle = Math_AngleXZ(&em->posX, em->goal);
            break;
        case 1:
            hermWork->goalAngle = Math_AngleXZ(&em->posX, em->goal) + ANGLE_TURN / 2;
            break;
        case 2:
            hermWork->goalAngle = Math_AngleXZ(&em->posX, em->goal) + ANGLE_TURN / 4;
            break;
        case 3:
            hermWork->goalAngle = Math_AngleXZ(&em->posX, em->goal) + ANGLE_TURN * 3 / 4;
            break;
        }
        if (((hermWork->goalAngle - em->rotY) & 0xFFFF) >= 0x8000U) {
            Em_SetMotion(em, HERM_MOT_TURN_RIGHT_ALERT, 0, 0);
        } else {
            Em_SetMotion(em, HERM_MOT_TURN_LEFT_ALERT, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].blend == 0 && EmMotion_ReachedFrame(em, 0, 6.0f)) {
            turnStep = 564.9655f * em->motionSpeed;
            angleDiff = (u16) (hermWork->goalAngle - *(u16 *) &em->rotY);
            if (em->motion[0].playing == 0 || EmMotion_ReachedFrame(em, 0, 64.0f)) {
                if ((u16) (angleDiff + turnStep) < turnStep * 2) {
                    em->step++;
                    Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
                    EmHermitaur_EndToIdle(em);
                } else {
                    Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
                    if (angleDiff >= ANGLE_TURN / 2) {
                        Em_SetMotion(em, HERM_MOT_TURN_RIGHT_ALERT, 4, 0);
                    } else {
                        Em_SetMotion(em, HERM_MOT_TURN_LEFT_ALERT, 4, 0);
                    }
                }
            } else if ((u16) (angleDiff + turnStep) < turnStep * 2) {
                em->rotY = hermWork->goalAngle;
            } else if (angleDiff < (ANGLE_TURN / 2)) {
                em->rotY = (u16) (em->rotY + turnStep);
            } else {
                em->rotY = (u16) (em->rotY - turnStep);
            }
        }
        break;
    }
    Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
}

/*
 * EM_GROUP_MOVE dispatcher: saves prevStateGroup and prevState, then runs
 * the current state's function.
 */
void EmHermitaur_RunMoveState(Em *em, EmWork *work)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->state) {
    case HERM_MOVE_TURN_TO_GOAL:
        EmHermitaur_TurnInPlace(em, work, 0);
        break;
    case HERM_MOVE_WALK:
        EmHermitaur_Walk(em, work, 0);
        break;
    case HERM_MOVE_WALK_ALERT_FAST:
        EmHermitaur_WalkAlert(em, work, 0, 1);
        break;
    case HERM_MOVE_TURN_AWAY:
        EmHermitaur_TurnInPlace(em, work, 1);
        break;
    case HERM_MOVE_WALK_BACK:
        EmHermitaur_Walk(em, work, 1);
        break;
    case HERM_MOVE_WALK_BACK_ALERT_FAST:
        EmHermitaur_WalkAlert(em, work, 1, 1);
        break;
    case HERM_MOVE_SIDESTEP_A:
        EmHermitaur_Sidestep(em, work, 0);
        break;
    case HERM_MOVE_SIDESTEP_B:
        EmHermitaur_Sidestep(em, work, 1);
        break;
    case HERM_MOVE_TURN_SIDE_A:
        EmHermitaur_TurnInPlace(em, work, 2);
        break;
    case HERM_MOVE_TURN_SIDE_B:
        EmHermitaur_TurnInPlace(em, work, 3);
        break;
    case HERM_MOVE_SIDESTEP_FAST_A:
        EmHermitaur_Sidestep(em, work, 2);
        break;
    case HERM_MOVE_SIDESTEP_FAST_B:
        EmHermitaur_Sidestep(em, work, 3);
        break;
    case HERM_MOVE_SIDESTEP_SHORT_A:
        EmHermitaur_Sidestep(em, work, 0);
        break;
    case HERM_MOVE_SIDESTEP_SHORT_B:
        EmHermitaur_Sidestep(em, work, 1);
        break;
    case HERM_MOVE_SIDESTEP_LONG_A:
        EmHermitaur_Sidestep(em, work, 0);
        break;
    case HERM_MOVE_SIDESTEP_LONG_B:
        EmHermitaur_Sidestep(em, work, 1);
        break;
    case HERM_MOVE_SIDESTEP_SHORT_C:
        EmHermitaur_Sidestep(em, work, 0);
        break;
    case HERM_MOVE_SIDESTEP_SHORT_D:
        EmHermitaur_Sidestep(em, work, 1);
        break;
    case HERM_MOVE_WALK_ALERT:
        EmHermitaur_WalkAlert(em, work, 0, 0);
        break;
    case HERM_MOVE_WALK_BACK_ALERT:
        EmHermitaur_WalkAlert(em, work, 1, 0);
        break;
    case HERM_MOVE_TURN_TO_GOAL_ALERT:
        EmHermitaur_TurnInPlaceAlert(em, work, 0);
        break;
    case HERM_MOVE_TURN_AWAY_ALERT:
        EmHermitaur_TurnInPlaceAlert(em, work, 1);
        break;
    case HERM_MOVE_TURN_SIDE_A_ALERT:
        EmHermitaur_TurnInPlaceAlert(em, work, 2);
        break;
    case HERM_MOVE_TURN_SIDE_B_ALERT:
        EmHermitaur_TurnInPlaceAlert(em, work, 3);
        break;
    case HERM_MOVE_BACK_ATTACK:
        EmHermitaur_WalkAlert(em, work, 2, 0);
        
    default:
        break;
    }
}

/*
 * No push, guard, noReact; plays HERM_MOT_BURROW, then
 * EmHermitaur_EndToBuried (HERM_DIG_BURIED).
 */
void EmHermitaur_Burrow(Em *em, s32 work, s32 variant)
{
    em->noPushTimer = 2;
    em->guardTimer = 2;
    em->noReact = 1;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, HERM_MOT_BURROW, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmHermitaur_EndToBuried(em);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmHermitaur_EndToBuried(em);
        }
        break;
    }
}

/*
 * Buried (stateArmor, no shadow, HERM_MOT_BURIED); turns 1024 a frame to
 * the goal angle, then EmHermitaur_EndToBuried.
 */
void EmHermitaur_BuriedTurnToGoal(Em *em, EmWork *work, s32 variant)
{
    u32 angleDiff;
    u16 toGoal;
    s32 rotY;

    em->stateArmor = 1;
    em->noPushTimer = 2;
    em->guardTimer = 2;
    em->noReact = 1;
    em->hideShadowHold = 2;
    em->hideShadow = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        ((EmHermitaurWork *)work)->goalAngle = Math_AngleXZ(&em->posX, em->goal);
        Em_SetMotion(em, HERM_MOT_BURIED, 0, 0);
        break;
    case 1:
        rotY = em->rotY;
        toGoal = ((EmHermitaurWork *)work)->goalAngle;
        angleDiff = (toGoal - (rotY & 0xFFFF)) & 0xFFFF;
        if ((u32) ((angleDiff + 0x400) & 0xFFFF) < 0x800U) {
            em->rotY = (s32) toGoal;
            em->step++;
            Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
            EmHermitaur_EndToBuried(em);
        } else if (angleDiff < 0x8000U) {
            em->rotY = (rotY + 0x400) & 0xFFFF;
        } else {
            em->rotY = (rotY - 0x400) & 0xFFFF;
        }
        break;
    }
    Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
}

/*
 * Buried, HERM_MOT_BURIED; steers 64 a frame and moves 80 a frame forward
 * until distLeft is used up, then EmHermitaur_EndToBuried. Variant 1 also
 * drops field collision and makes it unhittable and senseless for 5 frames.
 */
void EmHermitaur_BuriedMoveToGoal(Em *em, EmWork *work, s32 variant)
{
    f32 stepVec[3];
    f32 worldStep[3];
    s32 yawAngles[3];
    f32 yawMtx[16];
    s32 angleDiff;

    em->stateArmor = 1;
    em->noPushTimer = 2;
    em->guardTimer = 2;
    em->noReact = 1;
    em->hideShadowHold = 2;
    em->hideShadow = 2;
    if (variant == 1) {
        em->fieldCollide = 0;
        em->noHitTimer = 5;
        em->senseDelay = 5;
    }
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, HERM_MOT_BURIED, 0, 0);
        break;
    case 1:
        if (((EmHermitaurWork *)work)->steerToGoal != 0) {
            angleDiff = (u16) (Math_AngleXZ(&em->posX, em->goal) - em->rotY);
            if (angleDiff <= ANGLE_TURN / 2) {
                if (angleDiff < 64) {
                    em->rotY += angleDiff;
                } else {
                    em->rotY += 64;
                }
            } else if (angleDiff > 0xFFC0) {
                em->rotY += angleDiff;
            } else {
                em->rotY -= 64;
            }
        }
        Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
        stepVec[0] = 0.0f;
        stepVec[1] = 0.0f;
        stepVec[2] = 80.0f;
        yawAngles[0] = 0;
        yawAngles[1] = em->rotY;
        yawAngles[2] = 0;
        Mtx_FromAnglesYXZ(yawAngles, yawMtx);
        Vec_Rotate(worldStep, stepVec, yawMtx);
        em->posX += worldStep[0];
        em->posZ += worldStep[2];
        ((EmHermitaurWork *)work)->distLeft -= stepVec[2];
        if (((EmHermitaurWork *)work)->distLeft <= 0.0f) {
            em->step++;
            EmHermitaur_EndToBuried(em);
        }
        break;
    case 2:
        EmHermitaur_EndToBuried(em);
        break;
    }
}

/*
 * Buried (no shadow, stateArmor, no push): HERM_MOT_BURIED for 90 frames,
 * then EmHermitaur_EndToBuried, which sets scriptPending and re-enters this
 * state (HERM_DIG_BURIED).
 */
void EmHermitaur_BuriedWait(Em *em, s32 work, s32 variant)
{
    em->hideShadowHold = 2;
    em->hideShadow = 2;
    em->stateArmor = 1;
    em->noPushTimer = 2;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, HERM_MOT_BURIED, 0, 0);
        em->timer = 90;
        break;
    case 1:
        if (--em->timer <= 0) {
            EmHermitaur_EndToBuried(em);
        }
        break;
    }
}

/*
 * Plays HERM_MOT_LEAP in EM_STANCE_GROUND. When frame 114 passes it switches
 * to EM_STANCE_AIR, moves up 200 and rises (velY 100, slowing to no less
 * than 50) until it is within 760 of surfaceY; then it snaps to surfaceY and
 * plays HERM_MOT_CEIL_GRAB with ceilingTimer kept at 5, then
 * EmHermitaur_EndToCeiling.
 */
void EmHermitaur_ClimbToCeiling(Em *em, s32 work, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, HERM_MOT_LEAP, 0, 0);
        em->stance = EM_STANCE_GROUND;
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 114.0f, 0U) != 0) {
            em->step++;
            em->stance = EM_STANCE_AIR;
            em->posY += 200.0f;
            Player_ClearVelocity(em);
            em->velY = 100.0f;
            em->accelY = -5.0f;
        }
        break;
    case 2:
        Em_MoveRotatedAccel(em, &em->rotX);
        if (em->velY < 50.0f) {
            em->velY = 50.0f;
            em->accelY = 0.0f;
        }
        if (!(em->posY <= (em->surfaceY - 760.0f))) {
            em->step++;
            em->posY = em->surfaceY;
            Em_SetMotion(em, HERM_MOT_CEIL_GRAB, 0, 0);
            em->ceilingTimer = 5;
        }
        break;
    case 3:
        em->ceilingTimer = 5;
        if (em->motion[0].playing == 0) {
            em->step++;
            EmHermitaur_EndToCeiling(em);
        }
        break;
    }
}

/*
 * Keeps ceilingTimer at 5 and posY at surfaceY, sets EM_STANCE_AIR, plays
 * HERM_MOT_CEIL_HANG unless it is already playing, then
 * EmHermitaur_EndToCeiling (which re-enters this state).
 */
void EmHermitaur_CeilingHang(Em *em, s32 work, s32 variant)
{
    em->ceilingTimer = 5;
    em->posY = em->surfaceY;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        if (em->motionId[0] != EM_MOTION_ID(0, HERM_MOT_CEIL_HANG)) {
            Em_SetMotion(em, HERM_MOT_CEIL_HANG, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmHermitaur_EndToCeiling(em);
        }
        break;
    }
}

/*
 * EM_STANCE_AIR, HERM_MOT_CEIL_DROP on the ceiling, velY -50 with accelY -5
 * set but unused until frame 108. At frame 108 it moves down 400, leaves the
 * ceiling and creates HermitaurAttack 33, then falls to floorY, takes
 * EM_STANCE_GROUND, plays HERM_MOT_LEAP_LAND and ends with
 * EmHermitaur_EndToIdle.
 */
void EmHermitaur_CeilingDrop(Em *em, s32 work, s32 variant)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, HERM_MOT_CEIL_DROP, 0, 0);
        Player_ClearVelocity(em);
        em->velY = -50.0f;
        em->accelY = -5.0f;
        em->ceilingTimer = 5;
        break;
    case 1:
        em->ceilingTimer = 5;
        if (EmMotion_PassedFrameScaled(em, 108.0f, 0U) != 0) {
            em->step++;
            em->posY -= 400.0f;
            em->ceilingTimer = 0;
            HermitaurAttack_Create(em, 33);
        }
        break;
    case 2:
        Em_MoveRotatedAccel(em, &em->rotX);
        floorY = em->floorY;
        if (em->posY <= floorY) {
            em->posY = floorY;
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, HERM_MOT_LEAP_LAND, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmHermitaur_EndToIdle(em);
        }
        break;
    }
}

/*
 * On the ceiling, Em_StartTurnToTarget/Em_TurnStep with
 * EmHermitaur_TurnParams[0] from motion frame 8, then
 * EmHermitaur_EndToCeiling.
 */
void EmHermitaur_CeilingTurn(Em *em, s32 work, s32 variant)
{
    em->ceilingTimer = 5;
    em->posY = em->surfaceY;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_StartTurnToTarget(em, &EmHermitaur_TurnParams[0]);
        break;
    case 1:
        if (!(EmMotion_ReachedFrame(em, 0, 8.0f) != 0) == 0) {
            em->step++;
        case 2:
            if (Em_TurnStep(em, &EmHermitaur_TurnParams[0]) == 1) {
                em->step++;
                Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
                EmHermitaur_EndToCeiling(em);
            }
        }
        break;
    }
}

/*
 * On the ceiling, HERM_MOT_CEIL_CRAWL to the goal (Em_StepToGoal 64),
 * then EmHermitaur_EndToCeiling; variant 1 sets motionSpeed 50; with shell
 * variant 0 and part 2 unbroken it starts the timed EMFX_SHOGUN_JET effect.
 */
void EmHermitaur_CeilingCrawl(Em *em, s32 work, s32 variant)
{
    em->ceilingTimer = 5;
    em->posY = em->surfaceY;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_SetMotion(em, HERM_MOT_CEIL_CRAWL, 0, 0);
        if (variant == 1) {
            em->motionSpeed = 50.0f;
        }
        if ((em->variant == 0) && ((s32) em->parts[2].state < 2)) {
            EmEffectAttack_CreateTimed(em, EMFX_SHOGUN_JET, 0, 0, 32768, 100);
        }
        break;
    case 1:
        if (Em_StepToGoal(em, 64, 0) == 1) {
            em->step++;
            EmHermitaur_EndToCeiling(em);
        }
        break;
    }
}

/*
 * Buried and unhittable, EM_STANCE_GROUND; plays HERM_MOT_SHELL_SWAP, and at
 * its end the Shogun leaves a timed mine (EmMine_CreateTimed0) and it goes
 * to EmHermitaur_EndToBuried (HERM_DIG_BURIED). EmHermitaur_ChangeState
 * picks the new shell (em->variant) and resets part 2 on entry.
 */
void EmHermitaur_SwapShell(Em *em, s32 work, s32 variant)
{
    em->stateArmor = 1;
    em->noPushTimer = 2;
    em->guardTimer = 2;
    em->noReact = 1;
    em->hideShadowHold = 2;
    em->hideShadow = 2;
    em->noHitTimer = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, HERM_MOT_SHELL_SWAP, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            if (em->type == EM_KIND_SHOGUN_CEANATAUR) {
                EmMine_CreateTimed0(em, &em->posX);
            }
            em->noHitTimer = 0;
            EmHermitaur_EndToBuried(em);
        }
        break;
    }
}

/* Group 2 dispatcher (no prevState save, unlike groups 0 and 1). */
void EmHermitaur_RunDigState(Em *em, s32 work)
{
    switch (em->state) {
    case HERM_DIG_BURROW:
        EmHermitaur_Burrow(em, work, 0);
        break;
    case HERM_DIG_TURN_TO_GOAL:
        EmHermitaur_BuriedTurnToGoal(em, (EmWork *) work, 0);
        break;
    case HERM_DIG_MOVE_TO_GOAL:
        EmHermitaur_BuriedMoveToGoal(em, (EmWork *) work, 0);
        break;
    case HERM_DIG_BURIED:
        EmHermitaur_BuriedWait(em, work, 0);
        break;
    case HERM_DIG_CEIL_CLIMB:
        EmHermitaur_ClimbToCeiling(em, work, 0);
        break;
    case HERM_DIG_CEIL_HANG:
        EmHermitaur_CeilingHang(em, work, 0);
        break;
    case HERM_DIG_CEIL_DROP:
        EmHermitaur_CeilingDrop(em, work, 0);
        break;
    case HERM_DIG_CEIL_TURN:
        EmHermitaur_CeilingTurn(em, work, 0);
        break;
    case HERM_DIG_CEIL_CRAWL:
        EmHermitaur_CeilingCrawl(em, work, 0);
        break;
    case HERM_DIG_SHELL_SWAP:
        EmHermitaur_SwapShell(em, work, 0);
        break;
    case HERM_DIG_MOVE_THROUGH:
        EmHermitaur_BuriedMoveToGoal(em, (EmWork *) work, 1);
        break;
    case HERM_DIG_CEIL_CRAWL_FAST:
        EmHermitaur_CeilingCrawl(em, work, 1);
        break;
    case HERM_DIG_SHELL_SWAP_A:
        EmHermitaur_SwapShell(em, work, 0);
        break;
    case HERM_DIG_SHELL_SWAP_B:
        EmHermitaur_SwapShell(em, work, 1);
        break;
    case HERM_DIG_SHELL_SWAP_C:
        EmHermitaur_SwapShell(em, work, 2);
        
    default:
        break;
    }
}

/*
 * Plays HERM_MOT_STRIKE_A once (its hit, HermitaurAttack row 1, or 17 for
 * Shogun, comes from MotionEffects at frame 44), then
 * EmHermitaur_EndToIdle.
 */
void EmHermitaur_StrikeA(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, HERM_MOT_STRIKE_A, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmHermitaur_EndToIdle(em);
        }
        break;
    }
}

/*
 * Loops HERM_MOT_WINDUP (speedBonus 50) 1, 5 or 10 times by variant (passes
 * of frame 56 counted in timer), then HERM_MOT_WINDUP_STRIKE (MotionEffects
 * makes HermitaurAttack kind 2, 3 for the Daimyo, 18 for the Shogun, at
 * frame 18 and a ground effect at 32) and EmHermitaur_EndToIdle.
 */
void EmHermitaur_WindUpStrike(Em *em, s32 work, s32 variant)
{
    s32 variantByte;

    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, HERM_MOT_WINDUP, 0, 0);
        variantByte = variant & 0xFF;
        em->speedBonus = 50;
        switch (variantByte) {
        case 0:
            em->timer = 1;
            break;
        case 1:
            em->timer = 5;
            break;
        case 2:
            em->timer = 10;
        }
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 56.0f) != 0) {
            if (--em->timer <= 0) {
                em->step++;
                Em_SetMotion(em, HERM_MOT_WINDUP_STRIKE, 0, 0);
                break;
            }
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmHermitaur_EndToIdle(em);
        }
        break;
    }
}

/*
 * Daimyo and Ceanataur only, others EmHermitaur_EndToIdle:
 * HERM_MOT_WATER_JET, at frame 82 the jet (Daimyo EMFX_DAIMYO_JET,
 * Ceanataur EMRANGED_HERMITAUR_SHOT at HERM_BONE_BREATH), attack row 12
 * (Daimyo) or 32 after frame 86, then HERM_MOT_WATER_JET_END and
 * EmHermitaur_EndToIdle.
 */
void EmHermitaur_WaterJet(Em *em, EmWork *work)
{
    switch (em->type) {
    default:
        EmHermitaur_EndToIdle(em);
        break;
    case EM_KIND_DAIMYO_HERMITAUR:
    case EM_KIND_CEANATAUR:
        switch (em->step) {
        case 0:
            em->step++;
            Em_SetMotion(em, HERM_MOT_WATER_JET, 0, 0);
            break;
        case 1:
            if (Motion_ReachedFrame(em, 0, 82.0f) != 0) {
                em->step++;
                if (em->type == EM_KIND_DAIMYO_HERMITAUR) {
                    EmEffectAttack_Create(em, EMFX_DAIMYO_JET, 0, 910, 0);
                    break;
                }
                EmRangedAttack_CreateAtBone(em, HERM_BONE_BREATH, EMRANGED_HERMITAUR_SHOT, 0, 63717, 0);
            }
            break;
        case 2:
            if (Motion_PassedFrame(em, 0, 86.0f) != 0) {
                if (em->type == EM_KIND_DAIMYO_HERMITAUR) {
                    HermitaurAttack_Create(em, 12);
                } else {
                    HermitaurAttack_Create(em, 32);
                }
            }
            if (em->motion[0].playing == 0) {
                em->step++;
                Em_SetMotion(em, HERM_MOT_WATER_JET_END, 0, 0);
            }
            break;
        case 3:
            if (em->motion[0].playing == 0) {
                em->step++;
                EmHermitaur_EndToIdle(em);
            }
            break;
        }
        break;
    }
}

/*
 * Daimyo only: holds HERM_MOT_GUARD 360 (variant 0) or 180 (variant 1)
 * frames, keeping hermitTimer at 10 and hitzoneSet at 1 (parts 5 and 6 both
 * broken) or 2, then goes to HERM_ATK_GUARD_END.
 */
void EmHermitaur_Guard(Em *em, EmWork *work, s32 variant)
{
    if (em->type != EM_KIND_DAIMYO_HERMITAUR) {
        EmHermitaur_EndToIdle(em);
    } else {
        em->hermitTimer = 10;
        if ((s32) em->parts[5].state >= 2 && (s32) em->parts[6].state >= 2) {
            em->hitzoneSet = 1;
        } else {
            em->hitzoneSet = 2;
        }
        switch (em->step) {
        case 0:
            em->step++;
            Em_SetMotion(em, HERM_MOT_GUARD, 0, 0);
            if (variant == 0) {
                em->timer = 360;
                break;
            }
            em->timer = 180;
            break;
        case 1:
            if (--em->timer <= 0) {
                em->step++;
                EmHermitaur_ChangeState(em, EM_GROUP_ATTACK, HERM_ATK_GUARD_END, EM_SYNC_SEND);
            }
            break;
        case 2:
            if (em->remote == 0) {
                EmHermitaur_ChangeState(em, EM_GROUP_ATTACK, HERM_ATK_GUARD_END, EM_SYNC_SEND);
            }
            break;
        }
    }
}

/* Daimyo only: plays HERM_MOT_GUARD_END, then EmHermitaur_EndToIdle. */
void EmHermitaur_GuardEnd(Em *em)
{
    if (em->type != EM_KIND_DAIMYO_HERMITAUR) {
        EmHermitaur_EndToIdle(em);
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            Em_SetMotion(em, HERM_MOT_GUARD_END, 0, 0);
            break;
        case 1:
            if (em->motion[0].playing == 0) {
                em->step++;
                EmHermitaur_EndToIdle(em);
            }
            break;
        }
    }
}

/*
 * Guarded (noPush, guardTimer, noReact); plays HERM_MOT_BURROW, then (the
 * Daimyo and the Shogun) moves stateRand % 500 forward along rotY and goes
 * to HERM_ATK_AMBUSH_WAIT with stateArmor 1.
 */
void EmHermitaur_BurrowToAmbush(Em *em, EmWork *work)
{
    f32 offset[3];
    f32 worldOffset[3];
    s32 yawAngles[3];
    f32 yawMtx[16];

    em->noPushTimer = 2;
    em->guardTimer = 2;
    em->noReact = 1;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, HERM_MOT_BURROW, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stateArmor = 1;
            em->noReact = 0;
            switch (em->type) {
            case EM_KIND_DAIMYO_HERMITAUR:
            case EM_KIND_SHOGUN_CEANATAUR:
                offset[0] = 0.0f;
                offset[1] = 0.0f;
                offset[2] = 300.0f + ((f32) (em->stateRand % 500) - 300.0f);
                yawAngles[0] = 0;
                yawAngles[1] = em->rotY;
                yawAngles[2] = 0;
                Mtx_FromAnglesYXZ(yawAngles, yawMtx);
                Vec_Rotate(worldOffset, offset, yawMtx);
                em->posX += worldOffset[0];
                em->posZ += worldOffset[2];
                EmHermitaur_ChangeState(em, EM_GROUP_ATTACK, HERM_ATK_AMBUSH_WAIT, EM_SYNC_SEND);
                break;
            default:
                EmHermitaur_ChangeState(em, EM_GROUP_ATTACK, HERM_ATK_AMBUSH_WAIT, EM_SYNC_SEND);
                break;
            }
        }
        break;
    case 2:
        if (em->remote == 0) {
            em->stateArmor = 1;
            em->noReact = 0;
            EmHermitaur_ChangeState(em, EM_GROUP_ATTACK, HERM_ATK_AMBUSH_WAIT, EM_SYNC_SEND);
        }
        break;
    }
}

/*
 * Guarded; HERM_MOT_UNDER_THRUST with HermitaurAttack kind 10 (not for the
 * Shogun, whose hits come from MotionEffects), shadow hidden, stateArmor
 * cleared. Variant 0 goes on to HERM_ATK_EMERGE (at frame 46; the Shogun at
 * the motion end); variant 1 returns underground (EmHermitaur_EndToBuried)
 * at the motion end.
 */
void EmHermitaur_UnderThrust(Em *em, s32 work, s32 variant)
{
    em->noPushTimer = 2;
    em->guardTimer = 2;
    em->noReact = 1;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, HERM_MOT_UNDER_THRUST, 0, 0);
        if (em->type != EM_KIND_SHOGUN_CEANATAUR) {
            HermitaurAttack_Create(em, 10);
        }
        em->hideShadowHold = 2;
        em->hideShadow = 2;
        em->stateArmor = 0;
        break;
    case 1:
        if (em->type == EM_KIND_SHOGUN_CEANATAUR) {
            if (variant == 0) {
                if (em->motion[0].playing == 0) {
                    em->step++;
                    EmHermitaur_ChangeState(em, EM_GROUP_ATTACK, HERM_ATK_EMERGE, EM_SYNC_SEND);
                    break;
                }
            } else if (em->motion[0].playing == 0) {
                em->step++;
                EmHermitaur_EndToBuried(em);
                break;
            }
        } else if (variant == 0) {
            if (EmMotion_ReachedFrame(em, 0, 46.0f) != 0) {
                em->step++;
                EmHermitaur_ChangeState(em, EM_GROUP_ATTACK, HERM_ATK_EMERGE, EM_SYNC_SEND);
                break;
            }
        } else {
            if (em->motion[0].playing == 0) {
                em->step++;
                EmHermitaur_EndToBuried(em);
            }
            break;
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmHermitaur_ChangeState(em, EM_GROUP_ATTACK, HERM_ATK_EMERGE, EM_SYNC_SEND);
        }
        break;
    }
}

/*
 * Guarded; plays HERM_MOT_EMERGE (as a row without blend for Hermitaur and
 * Ceanataur), then clears the guards and stateArmor and
 * EmHermitaur_EndToIdle.
 */
void EmHermitaur_Emerge(Em *em)
{
    u8 type;

    em->noPushTimer = 2;
    em->guardTimer = 2;
    em->noReact = 1;
    switch (em->step) {
    case 0:
        em->step++;
        type = em->type;
        if ((type == EM_KIND_HERMITAUR) || (type == EM_KIND_CEANATAUR)) {
            Em_SetMotionRow(em, HERM_MOT_EMERGE, 0, 0);
        } else {
            Em_SetMotion(em, HERM_MOT_EMERGE, 0, 0);
        }
        em->stateArmor = 0;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stateArmor = 0;
            em->noPushTimer = 0;
            em->guardTimer = 0;
            em->noReact = 0;
            EmHermitaur_EndToIdle(em);
        }
        break;
    }
}

/*
 * Daimyo only (others EmHermitaur_EndToIdle): HERM_MOT_LEAP; at frame 98
 * airborne (EM_STANCE_AIR), lifted 500*scaleX with velY 120/140/160 by
 * variant and accelY -10, falling with Em_MoveRotatedAccel. Below floorY+200
 * it lands on the floor (EM_STANCE_GROUND, HERM_MOT_LEAP_LAND);
 * HermitaurAttack kind 11 is made once (stateVar0) on landing or at frame
 * 154; EmHermitaur_EndToIdle when the landing ends.
 */
void EmHermitaur_LeapPress(Em *em, s32 work, s32 variant)
{
    u8 hitDone;
    u8 hitDone2;

    if (em->type != EM_KIND_DAIMYO_HERMITAUR) {
        EmHermitaur_EndToIdle(em);
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            em->stateVar0 = 0;
            Em_SetMotion(em, HERM_MOT_LEAP, 0, 0);
            Player_ClearVelocity(em);
            em->stance = EM_STANCE_GROUND;
            break;
        case 1:
            if (Motion_ReachedFrame(em, 0, 98.0f) != 0) {
                em->step++;
                em->stance = EM_STANCE_AIR;
                em->posY += 500.0f * em->scaleX;
                switch (variant) {
                case 0:
                    em->velY = 120.0f;
                    em->accelY = -10.0f;
                    break;
                case 1:
                    em->velY = 140.0f;
                    em->accelY = -10.0f;
                    break;
                case 2:
                    em->velY = 160.0f;
                    em->accelY = -10.0f;
                    break;
                }
            } else {
            }
            break;
        case 2:
            Em_MoveRotatedAccel(em, &em->rotX);
            if (!((200.0f + em->floorY) <= em->posY)) {
                em->step++;
                em->posY = em->floorY;
                em->stance = EM_STANCE_GROUND;
                Em_SetMotion(em, HERM_MOT_LEAP_LAND, 0, 0);
                hitDone = em->stateVar0;
                if (hitDone == 0) {
                    em->stateVar0 = hitDone + 1;
                    HermitaurAttack_Create(em, 11);
                    break;
                }
            } else if (EmMotion_PassedFrameScaled(em, 154.0f, 0U) != 0) {
                hitDone2 = em->stateVar0;
                if (hitDone2 == 0) {
                    em->stateVar0 = hitDone2 + 1;
                    HermitaurAttack_Create(em, 11);
                    break;
                }
            }
            break;
        case 3:
            if (em->motion[0].playing == 0) {
                em->step++;
                EmHermitaur_EndToIdle(em);
            }
            break;
        }
    }
}

/*
 * Plays HERM_MOT_STRIKE_B (variant 0) or HERM_MOT_STRIKE_B_MIRROR (variant
 * 1) on the ground, then EmHermitaur_EndToIdle.
 */
void EmHermitaur_StrikeB(Em *em, s32 work, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        if (variant == 0) {
            Em_SetMotion(em, HERM_MOT_STRIKE_B, 0, 0);
            break;
        }
        Em_SetMotion(em, HERM_MOT_STRIKE_B_MIRROR, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmHermitaur_EndToIdle(em);
        }
        break;
    }
}

/*
 * Plays HERM_MOT_STRIKE_C (variant 0) or HERM_MOT_STRIKE_C_MIRROR (variant
 * 1) on the ground, then EmHermitaur_EndToIdle.
 */
void EmHermitaur_StrikeC(Em *em, s32 work, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        if (variant == 0) {
            Em_SetMotion(em, HERM_MOT_STRIKE_C, 0, 0);
            break;
        }
        Em_SetMotion(em, HERM_MOT_STRIKE_C_MIRROR, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmHermitaur_EndToIdle(em);
        }
        break;
    }
}

/*
 * Underground (HERM_MOT_BURIED, stateArmor, no hit, shadow hidden) until
 * timer (set by EmHermitaur_ChangeState: 300-420, 600-1170 for Hermitaur
 * and Ceanataur) runs out or a visible player in the area is within 200;
 * then HERM_ATK_AMBUSH_THRUST (the Daimyo and the Shogun) or
 * HERM_ATK_EMERGE (Hermitaur and Ceanataur).
 */
void EmHermitaur_AmbushWait(Em *em, EmWork *work)
{
    s16 i;
    Player *player;

    em->stateArmor = 1;
    em->noPushTimer = 2;
    em->senseDelay = 5;
    em->noHitTimer = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->hideShadowHold = 2;
        em->hideShadow = 2;
        Em_SetMotion(em, HERM_MOT_BURIED, 0, 0);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            if (em->remote == 0) {
                if (em->type == EM_KIND_HERMITAUR || em->type == EM_KIND_CEANATAUR) {
                    EmHermitaur_ChangeState(em, EM_GROUP_ATTACK, HERM_ATK_EMERGE, EM_SYNC_SEND);
                } else {
                    EmHermitaur_ChangeState(em, EM_GROUP_ATTACK, HERM_ATK_AMBUSH_THRUST, EM_SYNC_SEND);
                }
            }
        } else if (em->remote == 0) {
            for (i = 0, player = gPlayers; i < 4; i++, player++) {
                if (player->active && player->area == em->area && player->visible && Vec_Distance((f32 *) &em->posX, (f32 *) player->pos) < 200.0f) {
                    if (em->type == EM_KIND_HERMITAUR || em->type == EM_KIND_CEANATAUR) {
                        EmHermitaur_ChangeState(em, EM_GROUP_ATTACK, HERM_ATK_EMERGE, EM_SYNC_SEND);
                    } else {
                        EmHermitaur_ChangeState(em, EM_GROUP_ATTACK, HERM_ATK_AMBUSH_THRUST, EM_SYNC_SEND);
                    }
                }
            }
        }
        break;
    case 2:
        if (em->remote == 0) {
            if (em->type == EM_KIND_HERMITAUR || em->type == EM_KIND_CEANATAUR) {
                EmHermitaur_ChangeState(em, EM_GROUP_ATTACK, HERM_ATK_EMERGE, EM_SYNC_SEND);
            } else {
                EmHermitaur_ChangeState(em, EM_GROUP_ATTACK, HERM_ATK_AMBUSH_THRUST, EM_SYNC_SEND);
            }
        }
        break;
    }
}

/*
 * Saves its position in the work area and repeats HERM_MOT_UNDER_THRUST
 * (HermitaurAttack kind 10 except for the Shogun; shadow hidden). After
 * each thrust it goes back to the saved point, jumps by an offset from
 * EmHermitaur_ThrustOffsets (row = variant, pair = stateVar0) turned by
 * rotY, moves the same offset again over 4 frames (velX/velZ), shakes the
 * camera and waits 10 frames. After 5 thrusts (1 for variant 2) it returns
 * to the saved point and calls EmHermitaur_EndToBuried.
 */
void EmHermitaur_UnderThrustSeries(Em *em, EmWork *work, s32 variant)
{
    f32 offset[3];
    f32 worldOffset[3];
    s32 yawAngles[3];
    f32 yawMtx[16];

    em->noPushTimer = 2;
    em->guardTimer = 2;
    em->noReact = 1;
    switch (em->step) {
    case 0:
        em->step++;
        em->stateVar0 = 0;
        Vec_Copy((f32 *) &work->pitchStep, &em->posX);
        Em_SetMotion(em, HERM_MOT_UNDER_THRUST, 0, 0);
        if (em->type != EM_KIND_SHOGUN_CEANATAUR) {
            HermitaurAttack_Create(em, 10);
        }
        em->hideShadowHold = 2;
        em->hideShadow = 2;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->stateVar0++;
            if (em->stateVar0 > 4 || variant == 2) {
                Vec_Copy(&em->posX, (f32 *) &work->pitchStep);
                EmHermitaur_EndToBuried(em);
                break;
            }
            em->step++;
            em->timer = 4;
            Vec_Copy(&em->posX, (f32 *) &work->pitchStep);
            Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
            offset[0] = EmHermitaur_ThrustOffsets[variant * 10 + em->stateVar0 * 2];
            offset[1] = 0.0f;
            offset[2] = EmHermitaur_ThrustOffsets[variant * 10 + em->stateVar0 * 2 + 1];
            yawAngles[0] = 0;
            yawAngles[1] = em->rotY;
            yawAngles[2] = 0;
            Mtx_FromAnglesYXZ(yawAngles, yawMtx);
            Vec_Rotate(worldOffset, offset, yawMtx);
            em->velX = worldOffset[0] / (f32) em->timer;
            em->velZ = worldOffset[2] / (f32) em->timer;
            em->posX += worldOffset[0];
            em->posZ += worldOffset[2];
        }
        break;
    case 2:
        em->posX += em->velX;
        em->posZ += em->velZ;
        if (--em->timer <= 0) {
            em->step++;
            em->timer = 10;
            CamShake_ReqAIf(em, 4);
        }
        break;
    case 3:
        if (--em->timer <= 0) {
            em->step = 1;
            Em_SetMotion(em, HERM_MOT_UNDER_THRUST, 0, 0);
            if (em->type != EM_KIND_SHOGUN_CEANATAUR) {
                HermitaurAttack_Create(em, 10);
            }
            em->hideShadowHold = 2;
            em->hideShadow = 2;
        }
        break;
    }
}

/*
 * Shogun only (others EmHermitaur_EndToIdle), on the ceiling (EM_STANCE_AIR,
 * posY = surfaceY, ceilingTimer 5): HERM_MOT_CEIL_JET with MonsterFx 34
 * (variant 0 every 8 frames before frame 16 and from 210, variant 1 every 12
 * frames in frames 16-40 and 164-210) and EMFX_SHOGUN_JET at frame 50 if the
 * shell variant is 0 and part 2 is unbroken; then EmHermitaur_EndToCeiling.
 */
void EmHermitaur_CeilingJet(Em *em, EmWork *work, s32 variant)
{
    if (em->type != EM_KIND_SHOGUN_CEANATAUR) {
        EmHermitaur_EndToIdle(em);
        return;
    }
    em->ceilingTimer = 5;
    em->posY = em->surfaceY;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, HERM_MOT_CEIL_JET, 0, 0);
        em->stance = EM_STANCE_AIR;
        break;
    case 1:
        if (!EmMotion_ReachedFrame(em, 0, 16.0f) || EmMotion_ReachedFrame(em, 0, 210.0f)) {
            if (!(gScene.frame & 7)) {
                MonsterFx_CreateOnEmVariant(em, 34, 0, 1.0f);
            }
        } else if (!EmMotion_ReachedFrame(em, 0, 40.0f) || EmMotion_ReachedFrame(em, 0, 164.0f)) {
            if (gScene.frame % 12 == 0) {
                MonsterFx_CreateOnEmVariant(em, 34, 1, 1.0f);
            }
        }
        if (EmMotion_PassedFrameScaled(em, 50.0f, 0) && em->variant == 0 && em->parts[2].state < 2) {
            EmEffectAttack_Create(em, EMFX_SHOGUN_JET, 0, 0, 32768);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmHermitaur_EndToCeiling(em);
        }
        break;
    }
}

/*
 * Shogun only: plays HERM_MOT_GUARD or HERM_MOT_GUARD_END (its turns) by
 * the side of the goal, turning rotY toward goalAngle up to frame 44 at
 * ANGLE_TURN / 2 / 44 per frame (times motionSpeed); EmHermitaur_EndToIdle at the
 * motion end. Variant unused.
 */
void EmHermitaur_TurnToGoal(Em *em, EmWork *work, s32 variant)
{
    EmHermitaurWork *hermWork = (EmHermitaurWork *)work;
    u32 turnStep;
    u32 angleToGoal;

    if (em->type != EM_KIND_SHOGUN_CEANATAUR) {
        EmHermitaur_EndToIdle(em);
        return;
    }
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        hermWork->goalAngle = Math_AngleXZ(&em->posX, em->goal);
        if (((hermWork->goalAngle - em->rotY) & 0xFFFF) >= 0x8000U) {
            Em_SetMotion(em, HERM_MOT_GUARD, 0, 0);
        } else {
            Em_SetMotion(em, HERM_MOT_GUARD_END, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].blend == 0) {
            if (em->motion[0].playing == 0) {
                em->step++;
                EmHermitaur_EndToIdle(em);
            } else {
                turnStep = 744.7273f * em->motionSpeed;
                angleToGoal = (u16) (hermWork->goalAngle - *(u16 *) &em->rotY);
                if (!EmMotion_ReachedFrame(em, 0, 44.0f)) {
                    if ((u16) (angleToGoal + turnStep) < turnStep * 2) {
                        em->rotY = hermWork->goalAngle;
                    } else if (angleToGoal < (ANGLE_TURN / 2)) {
                        em->rotY = (u16) (em->rotY + turnStep);
                    } else {
                        em->rotY = (u16) (em->rotY - turnStep);
                    }
                }
            }
        }
        break;
    case 2:
        EmHermitaur_EndToIdle(em);
        break;
    }
    Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
}

/*
 * Shogun only: HERM_MOT_SPIN_A (variant 0, rotY -0x666 a frame) or
 * HERM_MOT_SPIN_B (variant 1, +0x666) over frames 46-86, a full turn; hits
 * 30/31 from MotionEffects at 76; then EmHermitaur_EndToIdle.
 */
void EmHermitaur_SpinStrike(Em *em, s32 work, s32 variant)
{
    if (em->type != EM_KIND_SHOGUN_CEANATAUR) {
        EmHermitaur_EndToIdle(em);
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            em->stance = EM_STANCE_GROUND;
            if (variant == 0) {
                Em_SetMotion(em, HERM_MOT_SPIN_A, 0, 0);
                break;
            }
            Em_SetMotion(em, HERM_MOT_SPIN_B, 0, 0);
            break;
        case 1:
            if (em->motion[0].playing == 0) {
                em->step++;
                EmHermitaur_EndToIdle(em);
                break;
            }
            if (EmMotion_InFrameRange(em, 0, 46.0f, 86.0f) != 0) {
                if (variant == 0) {
                    em->rotY -= 0x666;
                    break;
                }
                em->rotY += 0x666;
            } else {
            }
            break;
        }
    }
}

/* Group 3 dispatcher; also saves prevStateGroup/prevState. */
void EmHermitaur_RunAttackState(Em *em, EmWork *work)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->state) {
    case HERM_ATK_STRIKE_A:
        EmHermitaur_StrikeA(em);
        break;
    case HERM_ATK_WINDUP_STRIKE:
        EmHermitaur_WindUpStrike(em, (s32) work, 0);
        break;
    case HERM_ATK_WINDUP_STRIKE_MID:
        EmHermitaur_WindUpStrike(em, (s32) work, 1);
        break;
    case HERM_ATK_WATER_JET:
        EmHermitaur_WaterJet(em, work);
        break;
    case HERM_ATK_GUARD:
        EmHermitaur_Guard(em, work, 0);
        break;
    case HERM_ATK_GUARD_END:
        EmHermitaur_GuardEnd(em);
        break;
    case HERM_ATK_WINDUP_STRIKE_LONG:
        EmHermitaur_WindUpStrike(em, (s32) work, 2);
        break;
    case HERM_ATK_BURROW:
        EmHermitaur_BurrowToAmbush(em, work);
        break;
    case HERM_ATK_AMBUSH_THRUST:
        EmHermitaur_UnderThrust(em, (s32) work, 0);
        break;
    case HERM_ATK_EMERGE:
        EmHermitaur_Emerge(em);
        break;
    case HERM_ATK_LEAP_LOW:
        EmHermitaur_LeapPress(em, (s32) work, 0);
        break;
    case HERM_ATK_LEAP_MID:
        EmHermitaur_LeapPress(em, (s32) work, 1);
        break;
    case HERM_ATK_LEAP_HIGH:
        EmHermitaur_LeapPress(em, (s32) work, 2);
        break;
    case HERM_ATK_STRIKE_B:
        EmHermitaur_StrikeB(em, (s32) work, 0);
        break;
    case HERM_ATK_STRIKE_B_MIRROR:
        EmHermitaur_StrikeB(em, (s32) work, 1);
        break;
    case HERM_ATK_STRIKE_C:
        EmHermitaur_StrikeC(em, (s32) work, 0);
        break;
    case HERM_ATK_STRIKE_C_MIRROR:
        EmHermitaur_StrikeC(em, (s32) work, 1);
        break;
    case HERM_ATK_AMBUSH_WAIT:
        EmHermitaur_AmbushWait(em, work);
        break;
    case HERM_ATK_THRUST_SERIES_A:
        EmHermitaur_UnderThrustSeries(em, work, 0);
        break;
    case HERM_ATK_THRUST_RETURN:
        EmHermitaur_UnderThrust(em, (s32) work, 1);
        break;
    case HERM_ATK_THRUST_SERIES_B:
        EmHermitaur_UnderThrustSeries(em, work, 1);
        break;
    case HERM_ATK_THRUST_ONCE:
        EmHermitaur_UnderThrustSeries(em, work, 2);
        break;
    case HERM_ATK_CEIL_JET:
        EmHermitaur_CeilingJet(em, work, 0);
        break;
    case HERM_ATK_TURN_TO_GOAL:
        EmHermitaur_TurnToGoal(em, work, 0);
        break;
    case HERM_ATK_TURN_TO_GOAL_B:
        EmHermitaur_TurnToGoal(em, work, 1);
        break;
    case HERM_ATK_GUARD_SHORT:
        EmHermitaur_Guard(em, work, 1);
        break;
    case HERM_ATK_SPIN_A:
        EmHermitaur_SpinStrike(em, (s32) work, 0);
        break;
    case HERM_ATK_SPIN_B:
        EmHermitaur_SpinStrike(em, (s32) work, 1);
        break;
    }
}

/*
 * HERM_MOT_FLINCH then HERM_MOT_FLINCH_END, then EmHermitaur_EndToIdle
 * (script restarted).
 */
void EmHermitaur_Flinch(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, HERM_MOT_FLINCH, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, HERM_MOT_FLINCH_END, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmHermitaur_EndToIdle(em);
        }
        break;
    }
}

/* HERM_MOT_FLINCH_CLAW once, then EmHermitaur_EndToIdle. */
void EmHermitaur_FlinchClaw(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, HERM_MOT_FLINCH_CLAW, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmHermitaur_EndToIdle(em);
        }
        break;
    }
}

/*
 * On the ground: knock-down motion, lying motion for 120 frames (180 for
 * Shogun), get-up motion; side A (variant 0) or B (variant 1);
 * EmHermitaur_EndToIdle, then overwrites prevStateGroup/prevState with the
 * new state.
 */
void EmHermitaur_KnockDown(Em *em, EmWork *work, s16 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        if (variant == 0) {
            Em_SetMotion(em, HERM_MOT_KNOCK_DOWN, 0, 0);
        } else {
            Em_SetMotion(em, HERM_MOT_KNOCK_DOWN_B, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 0;
            if (variant == 0) {
                Em_SetMotion(em, HERM_MOT_DOWN, 0, 0);
            } else {
                Em_SetMotion(em, HERM_MOT_DOWN_B, 0, 0);
            }
        }
        break;
    case 2:
        em->timer++;
        if (em->timer >= (s16) (em->type == EM_KIND_SHOGUN_CEANATAUR ? 180 : 120)) {
            em->step++;
            if (variant == 0) {
                Em_SetMotion(em, HERM_MOT_DOWN_GET_UP, 0, 0);
            } else {
                Em_SetMotion(em, HERM_MOT_DOWN_B_GET_UP, 0, 0);
            }
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmHermitaur_EndToIdle(em);
            em->prevStateGroup = em->stateGroup;
            em->prevState = em->state;
        }
        break;
    }
}

/*
 * Hit-react mode STATUS, noReact; HERM_MOT_HELD with the status-4 effect
 * until timer runs out, then HERM_IDLE_STATUS4_SHAKE_OFF.
 */
void EmHermitaur_Status4(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_STATUS);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, HERM_MOT_HELD, 0, 0);
        em->noReact = 1;
        Em_RestartScript(em);
        em->stance = EM_STANCE_GROUND;
        break;
    case 1:
        Em_SpawnStatus4Effect(em, 2);
        if (--em->timer <= 0) {
            em->step++;
            EmHermitaur_ChangeState(em, EM_GROUP_IDLE, HERM_IDLE_STATUS4_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmHermitaur_ChangeState(em, EM_GROUP_IDLE, HERM_IDLE_STATUS4_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * The Shogun flinch of a broken part 2: HERM_MOT_SPIN_FLINCH on the ground,
 * rotY +0x376 a frame over frames 82-156 (a full turn), then
 * EmHermitaur_EndToIdle.
 */
void EmHermitaur_SpinFlinch(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, HERM_MOT_SPIN_FLINCH, 0, 0);
        Em_RestartScript(em);
        em->stance = EM_STANCE_GROUND;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmHermitaur_EndToIdle(em);
            break;
        }
        if (EmMotion_InFrameRange(em, 0, 82.0f, 156.0f) != 0) {
            em->rotY += 0x376;
        }
        break;
    }
}

/*
 * Daimyo hit by attack kind 5 while guarding (hermitTimer): HERM_MOT_RECOIL
 * held 180 frames, HERM_MOT_RECOIL_END, then EmHermitaur_EndToIdle.
 */
void EmHermitaur_GuardRecoil(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, HERM_MOT_RECOIL, 0, 0);
        Em_RestartScript(em);
        em->timer = 0;
        em->stance = EM_STANCE_GROUND;
        break;
    case 1:
        em->timer++;
        if (em->timer >= 180) {
            em->step++;
            Em_SetMotion(em, HERM_MOT_RECOIL_END, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmHermitaur_EndToIdle(em);
        }
        break;
    }
}

/*
 * Hermitaur/Ceanataur hit by attack kind 5: HERM_MOT_FLINCH held 330
 * frames, HERM_MOT_FLINCH_END, then EmHermitaur_EndToIdle.
 */
void EmHermitaur_WindFlinch(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, HERM_MOT_FLINCH, 0, 0);
        Em_RestartScript(em);
        em->timer = 0;
        em->stance = EM_STANCE_GROUND;
        break;
    case 1:
        em->timer++;
        if (em->timer >= 330) {
            em->step++;
            Em_SetMotion(em, HERM_MOT_FLINCH_END, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmHermitaur_EndToIdle(em);
        }
        break;
    }
}

/*
 * Em_SetHold2 each frame; HERM_MOT_FLINCH without root motion up to frame
 * 40, then HERM_MOT_HELD (blend 12) until timer runs out, then
 * HERM_IDLE_HOLD2_SHAKE_OFF.
 */
void EmHermitaur_Hold2(Em *em)
{
    Em_SetHold2(em);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, HERM_MOT_FLINCH, 0, 0);
        Player_SetFlag((Player *) em, EM_FLAG_NO_ROOT_MOTION);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 40.0f) != 0) {
            em->step++;
            Em_SetMotion(em, HERM_MOT_HELD, 12, 0);
            Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            EmHermitaur_ChangeState(em, EM_GROUP_IDLE, HERM_IDLE_HOLD2_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmHermitaur_ChangeState(em, EM_GROUP_IDLE, HERM_IDLE_HOLD2_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * Stun: the knock-down motions of side A (variant 0) or B (variant 1) with
 * stun stars at HERM_BONE_STUN_STARS for timer + half the motion; lies until timer
 * runs out, gets up, then HERM_IDLE_STUN_SHAKE_OFF.
 */
void EmHermitaur_Stun(Em *em, s32 work, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        if (variant == 0) {
            Em_SetMotion(em, HERM_MOT_KNOCK_DOWN, 0, 0);
        } else {
            Em_SetMotion(em, HERM_MOT_KNOCK_DOWN_B, 0, 0);
        }
        StunStars_CreateEm(em, 1, HERM_BONE_STUN_STARS, (s32) ((f32) em->timer + (em->motion[0].frameCount / 2.0f)), 150.0f, 3.0f);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            if (variant == 0) {
                Em_SetMotion(em, HERM_MOT_DOWN, 0, 0);
                break;
            }
            Em_SetMotion(em, HERM_MOT_DOWN_B, 0, 0);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            if (variant == 0) {
                Em_SetMotion(em, HERM_MOT_DOWN_GET_UP, 0, 0);
                break;
            }
            Em_SetMotion(em, HERM_MOT_DOWN_B_GET_UP, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmHermitaur_ChangeState(em, EM_GROUP_IDLE, HERM_IDLE_STUN_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 4:
        if (em->remote == 0) {
            EmHermitaur_ChangeState(em, EM_GROUP_IDLE, HERM_IDLE_STUN_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/* HERM_MOT_RECOIL once on the ground, then EmHermitaur_EndToIdle. */
void EmHermitaur_Recoil(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, HERM_MOT_RECOIL, 0, 0);
        Em_RestartScript(em);
        em->stance = EM_STANCE_GROUND;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmHermitaur_EndToIdle(em);
        }
        break;
    }
}

/*
 * Stance 2, HERM_MOT_FALL with velY at most -50 and gravity -10 until the
 * floor; HERM_MOT_FALL_LAND, then side-B lying and get-up motions
 * (HERM_MOT_DOWN_B, HERM_MOT_DOWN_B_GET_UP), EmHermitaur_EndToIdle.
 */
void EmHermitaur_FallFromAir(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, HERM_MOT_FALL, 0, 0);
        if (!(em->velY <= -50.0f)) {
            em->velY = -50.0f;
        }
        em->accelX = 0.0f;
        em->accelY = -10.0f;
        em->accelZ = 0.0f;
        break;
    case 1:
        Em_MoveRotatedAccel(em, &em->rotX);
        if (em->posY <= em->floorY) {
            em->step++;
            em->posY = em->floorY;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, HERM_MOT_FALL_LAND, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, HERM_MOT_DOWN_B, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, HERM_MOT_DOWN_B_GET_UP, 0, 0);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmHermitaur_EndToIdle(em);
            Player_ClearVelocity(em);
        }
        break;
    }
}

/*
 * HERM_MOT_SPIN_FLINCH with the same full turn as EmHermitaur_SpinFlinch,
 * then EmHermitaur_EndToIdle. On entry EmHermitaur_ChangeState counts
 * partBreakCount and, from the second break, sets fxFlags bit 0 and (Daimyo)
 * breaks part 1.
 */
void EmHermitaur_PartBreak(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, HERM_MOT_SPIN_FLINCH, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmHermitaur_EndToIdle(em);
            break;
        }
        if (EmMotion_InFrameRange(em, 0, 82.0f, 156.0f) != 0) {
            em->rotY += 0x376;
        }
        break;
    }
}

/* Group 4 dispatcher. */
void EmHermitaur_RunReactState(Em *em, EmWork *work)
{
    switch (em->state) {
    case HERM_REACT_FLINCH:
        EmHermitaur_Flinch(em);
        break;
    case HERM_REACT_FLINCH_CLAW:
        EmHermitaur_FlinchClaw(em);
        break;
    case HERM_REACT_KNOCK_DOWN_A:
        EmHermitaur_KnockDown(em, work, 0);
        break;
    case HERM_REACT_KNOCK_DOWN_B:
        EmHermitaur_KnockDown(em, work, 1);
        break;
    case HERM_REACT_STATUS4:
        EmHermitaur_Status4(em);
        break;
    case HERM_REACT_SPIN_FLINCH:
        EmHermitaur_SpinFlinch(em);
        break;
    case HERM_REACT_GUARD_RECOIL:
        EmHermitaur_GuardRecoil(em);
        break;
    case HERM_REACT_WIND_FLINCH:
        EmHermitaur_WindFlinch(em);
        break;
    case HERM_REACT_HOLD2:
        EmHermitaur_Hold2(em);
        break;
    case HERM_REACT_STUN_A:
        EmHermitaur_Stun(em, (s32) work, 0);
        break;
    case HERM_REACT_STUN_B:
        EmHermitaur_Stun(em, (s32) work, 1);
        break;
    case HERM_REACT_RECOIL_CLAW_A:
        EmHermitaur_Recoil(em);
        break;
    case HERM_REACT_FALL:
        EmHermitaur_FallFromAir(em);
        break;
    case HERM_REACT_PART_BREAK:
        EmHermitaur_PartBreak(em);
        break;
    case HERM_REACT_RECOIL_CLAW_B:
        EmHermitaur_Recoil(em);
        break;
    case HERM_REACT_FLINCH_CLAW_BROKEN_A:
        EmHermitaur_FlinchClaw(em);
        break;
    case HERM_REACT_FLINCH_CLAW_BROKEN_B:
        EmHermitaur_FlinchClaw(em);
        break;
    }
}

/*
 * Out of combat, EM_STANCE_DEAD, QuestEm_Hunted; HERM_MOT_DIE (Daimyo camera
 * shake at 212), carve point (EmMine_Create) at 290; waits 1800 frames (300
 * for Hermitaur and Ceanataur, counted only while QuestEm_CanAppear is 1
 * for the big kinds), removes the carve point, fades out over 150 frames,
 * then HERM_DEFEAT_REAPPEAR.
 */
void EmHermitaur_Die(Em *em, EmWork *work)
{
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->stateVar0 = 0;
        Em_SetMotion(em, HERM_MOT_DIE, 0, 0);
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_DEAD;
        QuestEm_Hunted(em);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 212.0f, 0) != 0 && em->type == EM_KIND_DAIMYO_HERMITAUR) {
            CamShake_ReqAIf(em, 2);
        }
        if (EmMotion_PassedFrameScaled(em, 290.0f, 0) != 0 && em->stateVar0 == 0) {
            em->stateVar0 = 1;
            EmMine_Create(em, 0);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            if (em->type == EM_KIND_HERMITAUR || em->type == EM_KIND_CEANATAUR) {
                em->timer = 300;
            } else {
                em->timer = 1800;
            }
            em->fieldCollide = 0;
        }
        break;
    case 2:
        EmMine_Update(em);
        if (em->type == EM_KIND_HERMITAUR || em->type == EM_KIND_CEANATAUR || QuestEm_CanAppear(em) == 1) {
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
                EmHermitaur_ChangeState(em, EM_GROUP_DEFEAT, HERM_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    }
}

/*
 * If QuestEm_CanAppear: sets HERM_MOT_EMERGE as a row and goes to
 * HERM_EVENT2_EMERGE; else ends the task (taskPhase++).
 */
void EmHermitaur_Reappear(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        if (QuestEm_CanAppear(em) == 1) {
            Em_SetMotionRow(em, HERM_MOT_EMERGE, 0, 0);
            EmHermitaur_ChangeState(em, EM_GROUP_EVENT2, HERM_EVENT2_EMERGE, EM_SYNC_LOCAL);
            break;
        }
        em->taskPhase++;
        break;
    }
}

/*
 * Group 5 dispatcher; keeps Hermitaur and Ceanataur unhittable and
 * unpushable (10).
 */
void EmHermitaur_RunDefeatState(Em *em, EmWork *work)
{
    u8 type;

    type = em->type;
    if ((type == EM_KIND_HERMITAUR) || (type == EM_KIND_CEANATAUR)) {
        em->noHitTimer = 10;
        em->noPushTimer = 10;
    }
    switch (em->state) {
    case HERM_DEFEAT_DIE:
        EmHermitaur_Die(em, work);
        break;
    case HERM_DEFEAT_REAPPEAR:
        EmHermitaur_Reappear(em, work);
        break;
    }
}

/*
 * Fades alpha out by 1/60 a frame, then QuestEm_Leave, ends the task and
 * hides the monster.
 */
void EmHermitaur_Leave(Em *em)
{
    f32 alpha;

    switch (em->step) {
    case 0:
        em->step++;
        break;
    case 1:
        alpha = em->alpha - 0.016666668f;
        em->alpha = alpha;
        if (alpha <= 0.0f) {
            QuestEm_Leave(em);
            em->taskPhase++;
            em->visible = 0;
        }
        break;
    }
}

/* Only calls EmHermitaur_EndToIdle. */
void EmHermitaur_EventEnd(Em *em)
{
    EmHermitaur_EndToIdle(em);
}

/*
 * Daimyo only (others EmHermitaur_EndToIdle), the scene for EVENT_FLAG_DAIMYO_HERMITAUR_SCENE,
 * unhittable: placed underground (HERM_MOT_BURIED) at a fixed point, waits
 * 180 frames, three HERM_MOT_UNDER_THRUST (the third cut at frame 46),
 * HERM_MOT_EMERGE, HERM_MOT_IDLE_ACTION_A and HERM_MOT_THREAT twice; then
 * waits for the flag and EmHermitaur_EndToIdle.
 */
void EmHermitaur_DaimyoIntro(Em *em)
{
    if (em->type != EM_KIND_DAIMYO_HERMITAUR) {
        EmHermitaur_EndToIdle(em);
        return;
    }
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stateVar0 = 0;
        em->stance = EM_STANCE_GROUND;
        em->stateArmor = 0;
        GmEm_ResetSize(em);
        Em_SetCombat(em, 1, 0);
        em->posX = 10335.0f;
        em->posY = 0.0f;
        em->posZ = 9788.0f;
        em->rotY = 0;
        em->hideShadowHold = 2;
        em->hideShadow = 2;
        em->stateArmor = 1;
        em->noPushTimer = 2;
        em->timer = 180;
        Em_SetMotion(em, HERM_MOT_BURIED, 0, 0);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, HERM_MOT_UNDER_THRUST, 0, 0);
            em->hideShadowHold = 2;
            em->hideShadow = 2;
        }
        break;
    case 2:
        switch (em->stateVar0) {
        case 0:
        case 1:
            if (EmMotion_ReachedFrame(em, 0, 100.0f)) {
                em->stateVar0++;
                Em_SetMotion(em, HERM_MOT_UNDER_THRUST, 0, 0);
            }
            break;
        case 2:
            if (EmMotion_ReachedFrame(em, 0, 46.0f)) {
                em->step++;
                Em_SetMotion(em, HERM_MOT_EMERGE, 0, 0);
            }
            break;
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, HERM_MOT_IDLE_ACTION_A, 0, 0);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, HERM_MOT_THREAT, 0, 0);
        }
        break;
    case 5:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, HERM_MOT_THREAT, 0, 0);
        }
        break;
    case 6:
        if (Save_TestEventFlag(EVENT_FLAG_DAIMYO_HERMITAUR_SCENE) == 1) {
            em->senseDelay = 0;
            em->noHitTimer = 0;
            EmHermitaur_EndToIdle(em);
        }
        break;
    }
}

/*
 * Shogun only (others EmHermitaur_EndToIdle), the scene for EVENT_FLAG_SHOGUN_CEANATAUR_SCENE,
 * unhittable: crawls the ceiling (height 1500, HERM_MOT_CEIL_CRAWL) toward a
 * fixed goal for 300 frames or until within 150, drops (HERM_MOT_CEIL_DROP,
 * 400 lower at frame 108), falls and lands (HERM_MOT_LEAP_LAND) facing
 * 0xD400. Then HERM_MOT_THREAT, restarted at its frame 40, HERM_MOT_STRIKE_A
 * from frame 170, HERM_MOT_GUARD (a turn for the Shogun), HERM_MOT_WINDUP
 * for 40 frames and HERM_MOT_WINDUP_STRIKE; idles until the flag, then
 * EmHermitaur_EndToIdle.
 */
void EmHermitaur_ShogunIntro(Em *em)
{
    f32 goalDist;

    if (em->type != EM_KIND_SHOGUN_CEANATAUR) {
        EmHermitaur_EndToIdle(em);
        return;
    }
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stateVar0 = 0;
        em->stance = EM_STANCE_AIR;
        em->stateArmor = 0;
        GmEm_ResetSize(em);
        Em_SetCombat(em, 1, 0);
        em->posX = 10100.0f;
        em->posY = 1500.0f;
        em->posZ = 6580.0f;
        em->rotY = 0;
        em->goal[0] = 7600.0f;
        em->goal[1] = 1500.0f;
        em->goal[2] = 7253.0f;
        em->rotY = Math_AngleXZ(&em->posX, em->goal);
        em->surfaceY = 1500.0f;
        em->ceilingTimer = 5;
        Em_SetMotionRow(em, HERM_MOT_CEIL_CRAWL, 0, 0);
        em->timer = 300;
        break;
    case 1:
        em->ceilingTimer = 5;
        goalDist = Vec_DistanceXZ(&em->posX, em->goal);
        em->rotY = Math_AngleXZ(&em->posX, em->goal);
        if (--em->timer <= 0 || goalDist <= 150.0f) {
            em->step++;
            em->timer = 100;
            Em_SetMotion(em, HERM_MOT_CEIL_DROP, 0, 0);
            Player_ClearVelocity(em);
            em->velY = -50.0f;
            em->accelY = -5.0f;
        }
        break;
    case 2:
        em->ceilingTimer = 5;
        if (EmMotion_PassedFrameScaled(em, 108.0f, 0)) {
            em->step++;
            em->posY -= 400.0f;
            em->ceilingTimer = 0;
        }
        break;
    case 3:
        Em_MoveRotatedAccel(em, &em->rotX);
        if (em->posY <= em->floorY) {
            em->posY = em->floorY;
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, HERM_MOT_LEAP_LAND, 0, 0);
            em->rotY = 0xD400;
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, HERM_MOT_THREAT, 0, 0);
        }
        break;
    case 5:
        if (EmMotion_PassedFrameScaled(em, 40.0f, 0)) {
            em->step++;
            Em_SetMotion(em, HERM_MOT_THREAT, 0, 0);
        }
        break;
    case 6:
        if (EmMotion_PassedFrameScaled(em, 170.0f, 0)) {
            em->step++;
            Em_SetMotion(em, HERM_MOT_STRIKE_A, 0, 0);
        }
        break;
    case 7:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, HERM_MOT_GUARD, 0, 0);
        }
        break;
    case 8:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, HERM_MOT_WINDUP, 0, 0);
            em->timer = 40;
        }
        break;
    case 9:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, HERM_MOT_WINDUP_STRIKE, 0, 0);
        }
        break;
    case 10:
        if (em->motion[0].playing == 0) {
            if (Save_TestEventFlag(EVENT_FLAG_SHOGUN_CEANATAUR_SCENE) == 1) {
                em->senseDelay = 0;
                em->noHitTimer = 0;
                EmHermitaur_EndToIdle(em);
                break;
            }
            em->step++;
            Em_SetMotion(em, HERM_MOT_IDLE, 0, 0);
        }
        break;
    case 11:
        if (Save_TestEventFlag(EVENT_FLAG_SHOGUN_CEANATAUR_SCENE) == 1) {
            em->senseDelay = 0;
            em->noHitTimer = 0;
            EmHermitaur_EndToIdle(em);
        }
        break;
    }
}

/*
 * Em_SetCaptured; HERM_MOT_LIE_DOWN (status 1 begun, poison cleared), then
 * HERM_MOT_LIE and QuestEm_Captured; 1800 frames while QuestEm_CanAppear,
 * fades over 150, then HERM_DEFEAT_REAPPEAR.
 */
void EmHermitaur_Captured(Em *em)
{
    s32 timeLeft;
    s32 timeBefore;

    Em_SetCaptured(em, 1);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, HERM_MOT_LIE_DOWN, 0, 0);
        em->timer = 1800;
        Em_BeginStatus1(em);
        Em_ClearPoison(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, HERM_MOT_LIE, 0, 0);
            QuestEm_Captured(em);
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
        timeBefore = em->timer;
        if (timeBefore > 0) {
            em->timer = timeBefore - 1;
        }
        timeLeft = em->timer;
        if (timeLeft <= 0) {
            if (em->remote == 0) {
                EmHermitaur_ChangeState(em, EM_GROUP_DEFEAT, HERM_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
                break;
            }
        } else {
            em->alpha = (f32) timeLeft / 150.0f;
        }
        break;
    }
}

/* Group 6 dispatcher. */
void EmHermitaur_RunEventState(Em *em)
{
    switch (em->state) {
    case HERM_EVENT_LEAVE:
        EmHermitaur_Leave(em);
        break;
    case HERM_EVENT_END:
        EmHermitaur_EventEnd(em);
        break;
    case HERM_EVENT_DAIMYO_INTRO:
        EmHermitaur_DaimyoIntro(em);
        break;
    case HERM_EVENT_SHOGUN_INTRO:
        EmHermitaur_ShogunIntro(em);
        break;
    case HERM_EVENT_CAPTURED:
        EmHermitaur_Captured(em);
        break;
    }
}

/*
 * Plays HERM_MOT_EMERGE, then EmHermitaur_EndToIdle (entered from
 * HERM_DEFEAT_REAPPEAR).
 */
void EmHermitaur_ReappearEmerge(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, HERM_MOT_EMERGE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmHermitaur_EndToIdle(em);
        }
        break;
    }
}

/* Group 7 dispatcher. */
void EmHermitaur_RunEvent2State(Em *em)
{
    switch (em->state) {
    case HERM_EVENT2_EMERGE:
        EmHermitaur_ReappearEmerge(em);
        break;
    }
}

/*
 * Versus: after the defeat camera, HERM_MOT_EAT; at frame 70 a VersusBait
 * for the player gPlayers[side - 1] at EmHermitaur_CarveThrowPt;
 * Versus_RouteEmByTeam at the end.
 */
void EmHermitaur_VsEatBait(Em *em, EmWork *work)
{
    Vec3f throwPt;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        if (Versus_DefeatCamera(em) == 1) {
            em->step++;
            Em_SetMotion(em, HERM_MOT_EAT, 0, 0);
        }
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 70.0f, 0) != 0) {
            throwPt = EmHermitaur_CarveThrowPt;
            VersusBait_Create(&gPlayers[(u8) em->side - 1], em, 0, 0, &throwPt.x);
        }
        if (em->motion[0].playing == 0) {
            Versus_RouteEmByTeam(em);
        }
        break;
    }
}

/*
 * Versus pose: Daimyo HERM_MOT_GUARD then HERM_MOT_GUARD_END, others
 * HERM_MOT_FLINCH_CLAW; then idle and HERM_VS_WAIT_BOTH.
 */
void EmHermitaur_VsPoseA(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    if (em->type == EM_KIND_DAIMYO_HERMITAUR) {
        switch (em->step) {
        case 0:
            em->step++;
            Em_SetMotion(em, HERM_MOT_GUARD, 0, 0);
            break;
        case 1:
            if (em->motion[0].playing == 0) {
                em->step++;
                Em_SetMotion(em, HERM_MOT_GUARD_END, 0, 0);
            }
            break;
        case 2:
            if (em->motion[0].playing == 0) {
                em->step++;
                Em_SetMotion(em, HERM_MOT_IDLE, 0, 0);
                EmHermitaur_ChangeState(em, EM_GROUP_VERSUS, HERM_VS_WAIT_BOTH, EM_SYNC_SEND);
            }
            break;
        }
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            Em_SetMotion(em, HERM_MOT_FLINCH_CLAW, 0, 0);
            break;
        case 1:
            if (em->motion[0].playing == 0) {
                em->step++;
                Em_SetMotion(em, HERM_MOT_IDLE, 0, 0);
                EmHermitaur_ChangeState(em, EM_GROUP_VERSUS, HERM_VS_WAIT_BOTH, EM_SYNC_SEND);
            }
            break;
        }
    }
}

/* Versus pose: HERM_MOT_THREAT, then idle and HERM_VS_WAIT_BOTH. */
void EmHermitaur_VsPoseB(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, HERM_MOT_THREAT, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, HERM_MOT_IDLE, 0, 0);
            EmHermitaur_ChangeState(em, EM_GROUP_VERSUS, HERM_VS_WAIT_BOTH, EM_SYNC_SEND);
        }
        break;
    }
}

/*
 * Versus pose: Daimyo HERM_MOT_WINDUP_STRIKE, others HERM_MOT_SPIN_A then
 * HERM_MOT_SPIN_B; then idle and HERM_VS_WAIT_BOTH.
 */
void EmHermitaur_VsPoseC(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    if (em->type == EM_KIND_DAIMYO_HERMITAUR) {
        switch (em->step) {
        case 0:
            em->step++;
            Em_SetMotion(em, HERM_MOT_WINDUP_STRIKE, 0, 0);
            break;
        case 1:
            if (em->motion[0].playing == 0) {
                em->step++;
                Em_SetMotion(em, HERM_MOT_IDLE, 0, 0);
                EmHermitaur_ChangeState(em, EM_GROUP_VERSUS, HERM_VS_WAIT_BOTH, EM_SYNC_SEND);
            }
            break;
        }
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            Em_SetMotion(em, HERM_MOT_SPIN_A, 0, 0);
            break;
        case 1:
            if (em->motion[0].playing == 0) {
                em->step++;
                Em_SetMotion(em, HERM_MOT_SPIN_B, 0, 0);
            }
            break;
        case 2:
            if (em->motion[0].playing == 0) {
                em->step++;
                Em_SetMotion(em, HERM_MOT_IDLE, 0, 0);
                EmHermitaur_ChangeState(em, EM_GROUP_VERSUS, HERM_VS_WAIT_BOTH, EM_SYNC_SEND);
            }
            break;
        }
    }
}

/* Idles until Versus_BothDefeated, then HERM_VS_END. */
void EmHermitaur_VsWaitBoth(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        if (em->motionId[0] != EM_MOTION_ID(0, HERM_MOT_IDLE)) {
            Em_SetMotion(em, HERM_MOT_IDLE, 0, 0);
        }
        
    case 1:
        if (Versus_BothDefeated() == 1) {
            em->step++;
            EmHermitaur_ChangeState(em, EM_GROUP_VERSUS, HERM_VS_END, EM_SYNC_SEND);
        }
        break;
    }
}

/* Idles 90 frames, clears senseDelay and noHitTimer, EmHermitaur_EndToIdle. */
void EmHermitaur_VsEnd(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 90;
        if (em->motionId[0] != EM_MOTION_ID(0, HERM_MOT_IDLE)) {
            Em_SetMotion(em, HERM_MOT_IDLE, 0, 0);
        }
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            em->senseDelay = 0;
            em->noHitTimer = 0;
            EmHermitaur_EndToIdle(em);
        }
        break;
    }
}

/* Runs the versus state (group 8). */
void EmHermitaur_RunVersusState(Em *em, EmWork *work)
{
    switch (em->state) {
    case HERM_VS_EAT_BAIT:
        EmHermitaur_VsEatBait(em, work);
        break;
    case HERM_VS_POSE_A:
        EmHermitaur_VsPoseA(em, work);
        break;
    case HERM_VS_POSE_B:
        EmHermitaur_VsPoseB(em, work);
        break;
    case HERM_VS_POSE_C:
        EmHermitaur_VsPoseC(em, work);
        break;
    case HERM_VS_WAIT_BOTH:
        EmHermitaur_VsWaitBoth(em, work);
        break;
    case HERM_VS_END:
        EmHermitaur_VsEnd(em, work);
        break;
    }
}

/*
 * Dispatches em->stateGroup (0-8) to its group's runner,
 * EmHermitaur_RunIdleState through EmHermitaur_RunVersusState, passing the
 * work area to all but the event groups 6 and 7.
 */
void EmHermitaur_RunState(Em *em)
{
    u8 *workArea = &em->work;
    u8 stateGroup;

    stateGroup = em->stateGroup;
    switch (stateGroup) {
    case 0:
        EmHermitaur_RunIdleState(em, (EmWork *) workArea);
        break;
    case 1:
        EmHermitaur_RunMoveState(em, (EmWork *) workArea);
        break;
    case 2:
        EmHermitaur_RunDigState(em, (s32) workArea);
        break;
    case 3:
        EmHermitaur_RunAttackState(em, (EmWork *) workArea);
        break;
    case 4:
        EmHermitaur_RunReactState(em, (EmWork *) workArea);
        break;
    case 5:
        EmHermitaur_RunDefeatState(em, (EmWork *) workArea);
        break;
    case 6:
        EmHermitaur_RunEventState(em);
        break;
    case 7:
        EmHermitaur_RunEvent2State(em);
        break;
    case 8:
        EmHermitaur_RunVersusState(em, (EmWork *) workArea);
        
    default:
        break;
    }
}

/*
 * Does the new state's entry work, then Em_SetStateSync(group, state, mode);
 * the owner first calls Em_TargetPlayerInRange, a remote copy ignores
 * EM_SYNC_OWNER and EM_SYNC_OWNER_FLAGGED requests, and hitzoneSet is
 * cleared. By state it swaps in the combat variants, begins or ends statuses
 * and rage, sets the walk and sidestep goal distance and timer, the ceiling
 * crawl speed, the ambush timer, the Shogun's new shell (part 2 reset) and
 * the part-break count, and calls QuestEm_Appear or Versus_MarkDefeat.
 * HERM_DIG_SHELL_SWAP and HERM_DIG_CEIL_CLIMB without a ceiling recurse into
 * another state.
 */
void EmHermitaur_ChangeState(Em *em, u16 group, u16 state, u8 mode)
{
    EmHermitaurWork *w = (EmHermitaurWork *)&em->work;
    f32 sideOfs[3];
    f32 sideOfsRot[3];
    s32 angles[3];
    f32 mtx[16];
    f32 stopDist;
    u8 newShell;

    if (em->remote == 0) {
        Em_TargetPlayerInRange(em);
    } else if (mode == EM_SYNC_OWNER || mode == EM_SYNC_OWNER_FLAGGED) {
        return;
    }
    em->hitzoneSet = 0;
    switch (group) {
    case EM_GROUP_IDLE:
        switch (state) {
        case HERM_IDLE:
            if (em->combat == 1) {
                state = HERM_IDLE_ALERT;
            }
            break;
        case HERM_IDLE_STATUS2_COLLAPSE:
            Em_BeginStatus2(em);
            break;
        case HERM_IDLE_STATUS2_END:
            Em_EndStatus2(em);
            break;
        case HERM_IDLE_STATUS4_SHAKE_OFF:
            Em_EndStatus4(em);
            break;
        case HERM_IDLE_STUN_SHAKE_OFF:
            Em_EndStatus5(em);
            break;
        case HERM_IDLE_HOLD2_SHAKE_OFF:
            em->holdTimer2 = 0;
            break;
        case HERM_IDLE_REST:
            Em_ResetNotice(em);
            break;
        case HERM_IDLE_STATUS1_COLLAPSE:
            Em_BeginStatus1(em);
            break;
        case HERM_IDLE_STATUS1_END:
            Em_EndStatus1(em);
            break;
        case HERM_IDLE_REST_END:
            Em_RefillNeed3(em);
            break;
        case HERM_IDLE_THREAT_RAGE:
            Em_StartRage(em);
            break;
        case HERM_IDLE_THREAT_PENDING_RAGE:
            Em_StartPendingRage(em);
            break;
        case HERM_IDLE_WAIT:
            if (em->combat == 1) {
                state = HERM_IDLE_WAIT_ALERT;
            }
            break;
        case HERM_IDLE_EAT_END:
            Em_StartGoalAction(em, 10000);
            break;
        }
        break;
    case EM_GROUP_MOVE:
        switch (state) {
        case HERM_MOVE_TURN_TO_GOAL:
            if (em->combat == 1) {
                state = HERM_MOVE_TURN_TO_GOAL_ALERT;
            }
            break;
        case HERM_MOVE_TURN_AWAY:
            if (em->combat == 1) {
                state = HERM_MOVE_TURN_AWAY_ALERT;
            }
            break;
        case HERM_MOVE_TURN_SIDE_A:
            if (em->combat == 1) {
                state = HERM_MOVE_TURN_SIDE_A_ALERT;
            }
            break;
        case HERM_MOVE_TURN_SIDE_B:
            if (em->combat == 1) {
                state = HERM_MOVE_TURN_SIDE_B_ALERT;
            }
            break;
        case HERM_MOVE_WALK:
            if (em->combat == 1) {
                state = HERM_MOVE_WALK_ALERT;
            }
            stopDist = 300.0f;
            goto walk;
        case HERM_MOVE_WALK_BACK:
            if (em->combat == 1) {
                state = HERM_MOVE_WALK_BACK_ALERT;
            }
            stopDist = -400.0f;
            goto walk;
        case HERM_MOVE_WALK_BACK_ALERT_FAST:
        case HERM_MOVE_WALK_BACK_ALERT:
            stopDist = -400.0f;
            goto walk;
        case HERM_MOVE_WALK_ALERT_FAST:
        case HERM_MOVE_WALK_ALERT:
            stopDist = 300.0f;
            if (em->type == EM_KIND_HERMITAUR || em->type == EM_KIND_CEANATAUR) {
                stopDist *= 0.66f;
            }
            goto walk;
        case HERM_MOVE_BACK_ATTACK:
            stopDist = -400.0f;
            goto walk;
        case HERM_MOVE_SIDESTEP_A:
        case HERM_MOVE_SIDESTEP_B:
        case HERM_MOVE_SIDESTEP_FAST_A:
        case HERM_MOVE_SIDESTEP_FAST_B:
            stopDist = 450.0f;
            if (em->type == EM_KIND_HERMITAUR || em->type == EM_KIND_CEANATAUR) {
                stopDist = 200.0f;
            }
        walk:
            if (em->remote != 0) {
                if (em->curGoalKind == 0) {
                    w->steerToGoal = 0;
                    break;
                }
                w->steerToGoal = 1;
                Em_ComputeGoal2(em, em->goal);
                w->distLeft = Vec_DistanceXZ(&em->posX, em->goal);
                if (em->curGoalKind == 1 && em->curGoalArg == 0) {
                    if (em->targetNo == -1) {
                        w->steerToGoal = 0;
                        w->distLeft = 1000.0f;
                        break;
                    }
                    w->distLeft -= stopDist;
                    if (w->distLeft <= 0.0f) {
                        w->distLeft = 0.0f;
                    }
                }
                if (em->curGoalKind == 7) {
                    w->distLeft -= stopDist;
                    if (w->distLeft <= 0.0f) {
                        w->distLeft = 0.0f;
                    }
                }
                break;
            }
            if (em->curGoalKind == 0) {
                w->steerToGoal = 0;
                em->timer = (em->stateRand % 120 + 180) / 2;
                w->distLeft = 1000.0f;
                break;
            }
            w->steerToGoal = 1;
            w->distLeft = Vec_DistanceXZ(&em->posX, em->goal);
            if (em->curGoalKind == 1 && em->curGoalArg == 0) {
                if (em->targetNo == -1) {
                    w->steerToGoal = 0;
                    w->distLeft = 1000.0f;
                    break;
                }
                w->distLeft -= stopDist;
                if (w->distLeft <= 0.0f) {
                    w->distLeft = 0.0f;
                }
            }
            if (em->curGoalKind == 7) {
                w->distLeft -= stopDist;
                if (w->distLeft <= 0.0f) {
                    w->distLeft = 0.0f;
                }
            }
            if (em->type == EM_KIND_HERMITAUR || em->type == EM_KIND_CEANATAUR) {
                em->timer = 500;
            } else {
                em->timer = (s32) w->distLeft / 7;
            }
            break;
        case HERM_MOVE_SIDESTEP_SHORT_A:
        case HERM_MOVE_SIDESTEP_SHORT_C:
            sideOfs[0] = 300.0f;
            goto turn;
        case HERM_MOVE_SIDESTEP_SHORT_B:
        case HERM_MOVE_SIDESTEP_SHORT_D:
            sideOfs[0] = -300.0f;
            goto turn;
        case HERM_MOVE_SIDESTEP_LONG_A:
            sideOfs[0] = 600.0f;
            goto turn;
        case HERM_MOVE_SIDESTEP_LONG_B:
            sideOfs[0] = -600.0f;
        turn:
            sideOfs[1] = 0.0f;
            sideOfs[2] = 0.0f;
            angles[0] = 0;
            angles[1] = em->rotY;
            angles[2] = 0;
            Mtx_FromAnglesYXZ(angles, mtx);
            Vec_Rotate(sideOfsRot, sideOfs, mtx);
            em->goal[0] = em->posX + sideOfsRot[0];
            em->goal[1] = em->posY + sideOfsRot[1];
            em->goal[2] = em->posZ + sideOfsRot[2];
            w->distLeft = Vec_DistanceXZ(&em->posX, em->goal);
            w->steerToGoal = 0;
            if (em->type == EM_KIND_HERMITAUR || em->type == EM_KIND_CEANATAUR) {
                em->timer = 600;
            } else {
                em->timer = (s32) w->distLeft / 7;
            }
            break;
        }
        break;
    case EM_GROUP_FLY:
        switch (state) {
        case HERM_DIG_MOVE_TO_GOAL:
            stopDist = 0.0f;
            if (em->remote != 0) {
                if (em->curGoalKind == 0) {
                    w->steerToGoal = 0;
                    break;
                }
                w->steerToGoal = 1;
                Em_ComputeGoal2(em, em->goal);
                w->distLeft = Vec_DistanceXZ(&em->posX, em->goal);
                if (em->curGoalKind == 1 && em->curGoalArg == 0) {
                    if (em->targetNo == -1) {
                        w->steerToGoal = 0;
                        w->distLeft = 1000.0f;
                        break;
                    }
                    w->distLeft -= stopDist;
                    if (w->distLeft <= 0.0f) {
                        w->distLeft = 0.0f;
                    }
                }
                if (em->curGoalKind == 7) {
                    w->distLeft -= stopDist;
                    if (w->distLeft <= 0.0f) {
                        w->distLeft = 0.0f;
                    }
                }
                break;
            }
            if (em->curGoalKind == 0) {
                w->steerToGoal = 0;
                em->timer = (em->stateRand % 120 + 180) / 2;
                w->distLeft = 1000.0f;
                break;
            }
            w->steerToGoal = 1;
            w->distLeft = Vec_DistanceXZ(&em->posX, em->goal);
            if (em->curGoalKind == 1 && em->curGoalArg == 0) {
                if (em->targetNo == -1) {
                    w->steerToGoal = 0;
                    w->distLeft = 1000.0f;
                    break;
                }
                w->distLeft -= stopDist;
                if (w->distLeft <= 0.0f) {
                    w->distLeft = 0.0f;
                }
            }
            if (em->curGoalKind == 7) {
                w->distLeft -= stopDist;
                if (w->distLeft <= 0.0f) {
                    w->distLeft = 0.0f;
                }
            }
            if (em->type == EM_KIND_HERMITAUR || em->type == EM_KIND_CEANATAUR) {
                em->timer = 600;
            } else {
                em->timer = (s32) w->distLeft / 7;
            }
            break;
        case HERM_DIG_CEIL_CLIMB:
            if (em->hasCeiling == 0 && em->remote == 0) {
                EmHermitaur_ChangeState(em, EM_GROUP_IDLE, HERM_IDLE_WAIT, EM_SYNC_SEND);
                return;
            }
            break;
        case HERM_DIG_CEIL_CRAWL:
        case HERM_DIG_CEIL_CRAWL_FAST:
            Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
            Em_SetSpeedScaled(em, 2, 1000.0f, 400.0f);
            break;
        case HERM_DIG_SHELL_SWAP:
            if (em->remote == 0) {
                if (em->parts[2].state < 2) {
                    EmHermitaur_ChangeState(em, EM_GROUP_FLY, HERM_DIG_BURIED, EM_SYNC_SEND);
                    return;
                }
                switch (em->variant) {
                case 0:
                    if (em->stateRand % 100 < 76) {
                        newShell = 2;
                    } else {
                        newShell = 1;
                    }
                    break;
                case 1:
                    if (em->stateRand % 100 < 51) {
                        newShell = 0;
                    } else {
                        newShell = 2;
                    }
                    break;
                case 2:
                    if (em->stateRand % 100 < 61) {
                        newShell = 0;
                    } else {
                        newShell = 1;
                    }
                    break;
                }
                switch (newShell) {
                case 0:
                    EmHermitaur_ChangeState(em, EM_GROUP_FLY, HERM_DIG_SHELL_SWAP_A, EM_SYNC_SEND);
                    return;
                case 1:
                    EmHermitaur_ChangeState(em, EM_GROUP_FLY, HERM_DIG_SHELL_SWAP_B, EM_SYNC_SEND);
                    return;
                case 2:
                    EmHermitaur_ChangeState(em, EM_GROUP_FLY, HERM_DIG_SHELL_SWAP_C, EM_SYNC_SEND);
                    return;
                }
            }
            break;
        case HERM_DIG_SHELL_SWAP_A:
            em->variant = 0;
            goto reset;
        case HERM_DIG_SHELL_SWAP_B:
            em->variant = 1;
            goto reset;
        case HERM_DIG_SHELL_SWAP_C:
            em->variant = 2;
        reset:
            em->parts[2].state = 0;
            w->lastPartState = em->parts[2].state;
            em->fxFlags &= 0xFE;
            Em_ResetPartHp(em, 2);
            break;
        }
        break;
    case EM_GROUP_ATTACK:
        switch (state) {
        case HERM_ATK_AMBUSH_WAIT:
            em->scriptPending = 0;
            em->senseDelay = 5;
            if (em->remote == 0) {
                switch (em->type) {
                case EM_KIND_CEANATAUR:
                case EM_KIND_HERMITAUR:
                    em->timer = (Rand_Next(1) % 20) * 30 + 600;
                    break;
                case EM_KIND_SHOGUN_CEANATAUR:
                case EM_KIND_DAIMYO_HERMITAUR:
                    em->timer = (Rand_Next(1) % 5) * 30 + 300;
                    break;
                }
            }
            break;
        case HERM_ATK_THRUST_SERIES_A:
            break;
        }
        break;
    case EM_GROUP_REACT:
        switch (state) {
        case HERM_REACT_STATUS4:
            Em_BeginStatus4(em);
            break;
        case HERM_REACT_HOLD2:
            Em_EndAllStatus(em);
            break;
        case HERM_REACT_STUN_A:
        case HERM_REACT_STUN_B:
            Em_BeginStatus5(em);
            break;
        case HERM_REACT_PART_BREAK:
            em->partBreakCount++;
            if (em->partBreakCount >= 2) {
                em->fxFlags |= 1;
                if (em->type == EM_KIND_DAIMYO_HERMITAUR) {
                    Em_BreakPart(em, 1);
                }
            }
            break;
        case HERM_REACT_SPIN_FLINCH:
        case HERM_REACT_GUARD_RECOIL:
        case HERM_REACT_WIND_FLINCH:
        case HERM_REACT_FLINCH_CLAW_BROKEN_B:
            break;
        }
        break;
    case EM_GROUP_DEFEAT:
        if (state == HERM_DEFEAT_REAPPEAR && QuestEm_CanAppear(em) == 1) {
            QuestEm_Appear(em);
        }
        break;
    case EM_GROUP_VERSUS:
        if (state == HERM_VS_WAIT_BOTH) {
            Versus_MarkDefeat(em);
        }
        em->motionSpeed = 1.0f;
        break;
    }
    Em_SetStateSync(em, group, state, mode);
}

/*
 * Resets motion speed, EM_STANCE_GROUND, look-at, collision, sets scriptPending,
 * goes to HERM_IDLE (HERM_IDLE_ALERT in combat).
 */
void EmHermitaur_EndToIdle(Em *em)
{
    em->motionSpeed = 1.0f;
    em->stance = EM_STANCE_GROUND;
    em->lookAtTarget = 0;
    em->scriptPending = 1;
    em->fieldCollide = 1;
    EmHermitaur_ChangeState(em, EM_GROUP_IDLE, HERM_IDLE, EM_SYNC_LOCAL);
}

/*
 * As EmHermitaur_EndToIdle but to HERM_IDLE_ALERT; only EmHermitaur_Threat
 * (threat) calls it.
 */
void EmHermitaur_EndToIdleAlert(Em *em)
{
    em->motionSpeed = 1.0f;
    em->stance = EM_STANCE_GROUND;
    em->lookAtTarget = 0;
    em->scriptPending = 1;
    em->fieldCollide = 1;
    EmHermitaur_ChangeState(em, EM_GROUP_IDLE, HERM_IDLE_ALERT, EM_SYNC_LOCAL);
}

/*
 * Resets motion speed and look-at, turns field collision on, sets
 * scriptPending and stateArmor 1 (stance untouched) and goes to
 * HERM_DIG_BURIED; called by the dig states, EmHermitaur_SwapShell,
 * EmHermitaur_UnderThrust and EmHermitaur_UnderThrustSeries.
 */
void EmHermitaur_EndToBuried(Em *em)
{
    em->motionSpeed = 1.0f;
    em->lookAtTarget = 0;
    em->scriptPending = 1;
    em->stateArmor = 1;
    em->fieldCollide = 1;
    EmHermitaur_ChangeState(em, EM_GROUP_FLY, HERM_DIG_BURIED, EM_SYNC_LOCAL);
}

/*
 * Sets EM_STANCE_AIR (on the ceiling), ceilingTimer 5, scriptPending, and goes
 * to HERM_DIG_CEIL_HANG; called by the ceiling states and
 * EmHermitaur_CeilingJet.
 */
void EmHermitaur_EndToCeiling(Em *em)
{
    em->motionSpeed = 1.0f;
    em->stance = EM_STANCE_AIR;
    em->lookAtTarget = 0;
    em->scriptPending = 1;
    em->ceilingTimer = 5;
    em->fieldCollide = 1;
    EmHermitaur_ChangeState(em, EM_GROUP_FLY, HERM_DIG_CEIL_HANG, EM_SYNC_LOCAL);
}

/*
 * Callback slots 0 and 1 (init, update): Shadow_Spawn type 0, and for the
 * Daimyo and the Shogun the spray effect GunFx type 5.
 */
void EmHermitaur_SpawnEffects(Em *em)
{
    u8 type;

    Shadow_Spawn((Player *) &em->active, SHADOW_MODEL);
    type = em->type;
    if ((type == EM_KIND_DAIMYO_HERMITAUR) || (type == EM_KIND_SHOGUN_CEANATAUR)) {
        GunFx_CreateType5(em);
    }
}

/* Empty callback (slot 2). */
void EmHermitaur_TableNop(void)
{
}

/*
 * When the monster is in the current area, spawns ground effect 13 of the
 * given kind at a bone's position dropped to floorY, scaled by scaleX.
 */
void EmHermitaur_SpawnGroundEffect(Em *em, s32 bone, s32 effectKind, f32 scale)
{
    f32 pos[3];

    if (Player_InCurrentArea(em)) {
        Em_BonePos(em, bone, pos);
        pos[1] = em->floorY;
        PartFx_SpawnOnGround(pos, effectKind, scale * em->scaleX);
    }
}

/* Empty default of the MotionEffects switch. */
void EmHermitaur_MotionEffectsDefault(Em *em)
{
}

/*
 * Keeps motionId[0] in work->fxMotion, plays MotionSounds, then per motion
 * creates attack hit boxes (HermitaurAttack_Create, kind per species),
 * impact attacks, dust clouds, ground effects, lying puffs, and hides the
 * shadow while underground.
 */
void EmHermitaur_MotionEffects(Em *em, EmWork *work)
{
    f32 puffOfs[3];

    if (em->motionId[0] != work->fxMotion) {
        work->fxMotion = em->motionId[0];
    }
    EmHermitaur_MotionSounds(em, work);
    switch (work->fxMotion) {
    case EM_MOTION_ID(0, HERM_MOT_IDLE):
    case EM_MOTION_ID(0, HERM_MOT_WALK):
    case EM_MOTION_ID(0, HERM_MOT_WALK_BACK):
    case EM_MOTION_ID(0, HERM_MOT_TURN_RIGHT):
    case EM_MOTION_ID(0, HERM_MOT_TURN_LEFT):
    case EM_MOTION_ID(0, HERM_MOT_TURN_RIGHT_ALERT):
    case EM_MOTION_ID(0, HERM_MOT_TURN_LEFT_ALERT):
        break;
    case EM_MOTION_ID(0, HERM_MOT_LEAP):
        if (em->type == EM_KIND_SHOGUN_CEANATAUR) {
            if (EmMotion_PassedFrameScaled(em, 112.0f, 0)) {
                DustCloud_Create(em, 11, 21, em->scaleX);
            }
        } else if (EmMotion_PassedFrameScaled(em, 98.0f, 0)) {
            DustCloud_Create(em, 19, 21, em->scaleX);
        }
        break;
    case EM_MOTION_ID(0, HERM_MOT_LEAP_LAND):
        if (EmMotion_PassedFrameScaled(em, 8.0f, 0)) {
            EmHermitaur_SpawnGroundEffect(em, HERM_BONE_LANDING, 40, 2.0f * em->scaleX);
            EmImpactAttack_CreateAt(em, &em->posX, 5);
            EmImpactAttack_CreateAt(em, &em->posX, 6);
        }
        if (em->type == EM_KIND_SHOGUN_CEANATAUR && EmMotion_PassedFrameScaled(em, 190.0f, 0)) {
            EmHermitaur_SpawnGroundEffect(em, HERM_BONE_CLAW_A, 4, 6.0f);
            EmHermitaur_SpawnGroundEffect(em, HERM_BONE_CLAW_B, 4, 6.0f);
        }
        break;
    case EM_MOTION_ID(0, 15):
    case EM_MOTION_ID(0, 16):
    case EM_MOTION_ID(0, 18):
        break;
    case EM_MOTION_ID(0, HERM_MOT_STRIKE_C):
        if (EmMotion_PassedFrameScaled(em, 14.0f, 0)) {
            switch (em->type) {
            case EM_KIND_CEANATAUR:
            case EM_KIND_HERMITAUR:
                HermitaurAttack_Create(em, 4);
                break;
            case EM_KIND_SHOGUN_CEANATAUR:
                HermitaurAttack_Create(em, 23);
                break;
            case EM_KIND_DAIMYO_HERMITAUR:
                HermitaurAttack_Create(em, 13);
                break;
            }
        }
        if (EmMotion_PassedFrameScaled(em, 22.0f, 0)) {
            DustCloud_Create(em, 2, 35, em->scaleX);
        }
        break;
    case EM_MOTION_ID(0, HERM_MOT_SIDESTEP_A):
        break;
    case EM_MOTION_ID(0, HERM_MOT_STRIKE_B):
        if (EmMotion_PassedFrameScaled(em, 14.0f, 0)) {
            switch (em->type) {
            case EM_KIND_CEANATAUR:
            case EM_KIND_HERMITAUR:
                HermitaurAttack_Create(em, 6);
                break;
            case EM_KIND_SHOGUN_CEANATAUR:
                HermitaurAttack_Create(em, 21);
                break;
            case EM_KIND_DAIMYO_HERMITAUR:
                HermitaurAttack_Create(em, 15);
                break;
            }
        }
        if (EmMotion_PassedFrameScaled(em, 20.0f, 0)) {
            DustCloud_Create(em, 2, 35, em->scaleX);
        }
        break;
    case EM_MOTION_ID(0, HERM_MOT_SIDESTEP_B):
        break;
    case EM_MOTION_ID(0, HERM_MOT_STRIKE_B_MIRROR):
        if (EmMotion_PassedFrameScaled(em, 14.0f, 0)) {
            switch (em->type) {
            case EM_KIND_CEANATAUR:
            case EM_KIND_HERMITAUR:
                HermitaurAttack_Create(em, 7);
                break;
            case EM_KIND_SHOGUN_CEANATAUR:
                HermitaurAttack_Create(em, 22);
                break;
            case EM_KIND_DAIMYO_HERMITAUR:
                HermitaurAttack_Create(em, 16);
                break;
            }
        }
        if (EmMotion_PassedFrameScaled(em, 20.0f, 0)) {
            DustCloud_Create(em, 3, 36, em->scaleX);
        }
        break;
    case EM_MOTION_ID(0, 24):
        break;
    case EM_MOTION_ID(0, HERM_MOT_STRIKE_C_MIRROR):
        if (EmMotion_PassedFrameScaled(em, 14.0f, 0)) {
            switch (em->type) {
            case EM_KIND_CEANATAUR:
            case EM_KIND_HERMITAUR:
                HermitaurAttack_Create(em, 5);
                break;
            case EM_KIND_SHOGUN_CEANATAUR:
                HermitaurAttack_Create(em, 24);
                break;
            case EM_KIND_DAIMYO_HERMITAUR:
                HermitaurAttack_Create(em, 14);
                break;
            }
        }
        if (EmMotion_PassedFrameScaled(em, 22.0f, 0)) {
            DustCloud_Create(em, 3, 36, em->scaleX);
        }
        break;
    case EM_MOTION_ID(0, HERM_MOT_LIE):
        puffOfs[0] = 0.0f;
        puffOfs[1] = 20.0f;
        puffOfs[2] = 50.0f;
        Em_SpawnLyingPuffs(em, HERM_BONE_BREATH, puffOfs, 1.6f);
        break;
    case EM_MOTION_ID(0, HERM_MOT_EMERGE):
        if (em->type == EM_KIND_HERMITAUR || em->type == EM_KIND_CEANATAUR) {
            if (EmMotion_PassedFrameScaled(em, 20.0f, 0) || EmMotion_PassedFrameScaled(em, 92.0f, 0)) {
                EmHermitaur_SpawnGroundEffect(em, HERM_BONE_SIDE_B, 7, 6.0f);
            }
            if (EmMotion_PassedFrameScaled(em, 16.0f, 0)) {
                HermitaurAttack_Create(em, 8);
            }
            if (!EmMotion_ReachedFrame(em, 0, 120.0f)) {
                em->hideShadowHold = 2;
                em->hideShadow = 2;
            }
            break;
        }
        if (em->type == EM_KIND_SHOGUN_CEANATAUR) {
            if (EmMotion_PassedFrameScaled(em, 18.0f, 0)) {
                HermitaurAttack_Create(em, 19);
            }
            if (EmMotion_PassedFrameScaled(em, 22.0f, 0)) {
                HermitaurAttack_Create(em, 20);
            }
            if (EmMotion_PassedFrameScaled(em, 110.0f, 0)) {
                HermitaurAttack_Create(em, 28);
            }
            if (EmMotion_PassedFrameScaled(em, 128.0f, 0)) {
                HermitaurAttack_Create(em, 29);
            }
        }
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0)) {
            EmHermitaur_SpawnGroundEffect(em, HERM_BONE_SIDE_B, 7, 6.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 84.0f, 0)) {
            DustCloud_Create(em, 11, 24, em->scaleX);
        }
        break;
    case EM_MOTION_ID(0, HERM_MOT_BURIED):
        em->hideShadowHold = 2;
        em->hideShadow = 2;
        if (em->type == EM_KIND_HERMITAUR || em->type == EM_KIND_CEANATAUR) {
            if ((gScene.frame & 0x7F) == 0 && (Rand_Next(1) & 3)) {
                PartFx_SpawnOnGround(&em->posX, 4, 8.0f * em->scaleX);
            }
        }
        break;
    case EM_MOTION_ID(0, HERM_MOT_GUARD):
        if (em->type == EM_KIND_SHOGUN_CEANATAUR) {
            if (EmMotion_PassedFrameScaled(em, 72.0f, 0)) {
                DustCloud_Create(em, 2, 35, em->scaleX);
            }
            if (EmMotion_PassedFrameScaled(em, 66.0f, 0)) {
                HermitaurAttack_Create(em, 25);
            }
        }
        break;
    case EM_MOTION_ID(0, HERM_MOT_GUARD_END):
        if (em->type == EM_KIND_SHOGUN_CEANATAUR) {
            if (EmMotion_PassedFrameScaled(em, 72.0f, 0)) {
                DustCloud_Create(em, 3, 36, em->scaleX);
            }
            if (EmMotion_PassedFrameScaled(em, 66.0f, 0)) {
                HermitaurAttack_Create(em, 26);
            }
        }
        break;
    case EM_MOTION_ID(0, HERM_MOT_STRIKE_A):
        if (EmMotion_PassedFrameScaled(em, 44.0f, 0)) {
            if (em->type == EM_KIND_SHOGUN_CEANATAUR) {
                HermitaurAttack_Create(em, 17);
            } else {
                HermitaurAttack_Create(em, 1);
            }
        }
        if (em->type == EM_KIND_SHOGUN_CEANATAUR) {
            if (EmMotion_PassedFrameScaled(em, 48.0f, 0)) {
                DustCloud_Create(em, 2, 22, em->scaleX);
            }
            if (EmMotion_PassedFrameScaled(em, 56.0f, 0)) {
                DustCloud_Create(em, 3, 23, em->scaleX);
            }
        }
        break;
    case EM_MOTION_ID(0, HERM_MOT_WINDUP_STRIKE):
        if (EmMotion_PassedFrameScaled(em, 18.0f, 0)) {
            switch (em->type) {
            case EM_KIND_CEANATAUR:
            case EM_KIND_HERMITAUR:
                HermitaurAttack_Create(em, 2);
                break;
            case EM_KIND_SHOGUN_CEANATAUR:
                HermitaurAttack_Create(em, 18);
                break;
            case EM_KIND_DAIMYO_HERMITAUR:
                HermitaurAttack_Create(em, 3);
                break;
            }
        }
        if (EmMotion_PassedFrameScaled(em, 32.0f, 0)) {
            EmHermitaur_SpawnGroundEffect(em, HERM_BONE_SIDE_A, 7, 6.0f);
        }
        break;
    case EM_MOTION_ID(0, HERM_MOT_WINDUP):
    case EM_MOTION_ID(0, HERM_MOT_WALK_ALERT):
    case EM_MOTION_ID(0, HERM_MOT_WALK_BACK_ALERT):
        break;
    case EM_MOTION_ID(0, HERM_MOT_SPIN_A):
        if (EmMotion_PassedFrameScaled(em, 76.0f, 0)) {
            HermitaurAttack_Create(em, 30);
        }
        if (EmMotion_PassedFrameScaled(em, 84.0f, 0)) {
            DustCloud_Create(em, 3, 36, em->scaleX);
        }
        break;
    case EM_MOTION_ID(0, HERM_MOT_UNDER_THRUST):
        if (EmMotion_PassedFrameScaled(em, 14.0f, 0)) {
            EmHermitaur_SpawnGroundEffect(em, HERM_BONE_SIDE_A, 7, 6.0f);
        }
        if (!EmMotion_ReachedFrame(em, 0, 16.0f)) {
            em->hideShadowHold = 2;
            em->hideShadow = 2;
        } else if (EmMotion_ReachedFrame(em, 0, 66.0f)) {
            em->hideShadowHold = 2;
            em->hideShadow = 2;
        }
        if (em->type == EM_KIND_SHOGUN_CEANATAUR) {
            if (EmMotion_PassedFrameScaled(em, 18.0f, 0)) {
                HermitaurAttack_Create(em, 19);
            }
            if (EmMotion_PassedFrameScaled(em, 22.0f, 0)) {
                HermitaurAttack_Create(em, 20);
            }
        }
        break;
    case EM_MOTION_ID(0, HERM_MOT_SPIN_B):
        if (EmMotion_PassedFrameScaled(em, 76.0f, 0)) {
            HermitaurAttack_Create(em, 31);
        }
        if (EmMotion_PassedFrameScaled(em, 84.0f, 0)) {
            DustCloud_Create(em, 2, 35, em->scaleX);
        }
        break;
    case EM_MOTION_ID(0, HERM_MOT_SHELL_SWAP):
        if (em->type == EM_KIND_SHOGUN_CEANATAUR && EmMotion_PassedFrameScaled(em, 50.0f, 0)) {
            DustCloud_Create(em, 11, 37, 0.5f * em->scaleX);
        }
        break;
    case EM_MOTION_ID(0, HERM_MOT_IDLE_ALERT):
    case EM_MOTION_ID(0, HERM_MOT_THREAT):
        break;
    case EM_MOTION_ID(0, HERM_MOT_BURROW):
        if (EmMotion_PassedFrameScaled(em, 34.0f, 0)) {
            EmHermitaur_SpawnGroundEffect(em, HERM_BONE_SIDE_A, 7, 6.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 64.0f, 0)) {
            DustCloud_Create(em, 11, 21, em->scaleX);
        }
        if (EmMotion_ReachedFrame(em, 0, 130.0f)) {
            em->hideShadowHold = 2;
            em->hideShadow = 2;
        }
        break;
    case EM_MOTION_ID(0, HERM_MOT_CEIL_GRAB):
    case EM_MOTION_ID(0, HERM_MOT_FLINCH):
    case EM_MOTION_ID(0, HERM_MOT_FLINCH_CLAW):
        break;
    case EM_MOTION_ID(0, HERM_MOT_KNOCK_DOWN):
        if (EmMotion_PassedFrameScaled(em, 28.0f, 0)) {
            EmHermitaur_SpawnGroundEffect(em, HERM_BONE_SIDE_A, 7, 6.0f);
        }
        break;
    case EM_MOTION_ID(0, HERM_MOT_FALL_LAND):
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0)) {
            DustCloud_Create(em, 11, 21, em->scaleX);
        }
        break;
    case EM_MOTION_ID(0, HERM_MOT_DIE):
        if (EmMotion_PassedFrameScaled(em, 158.0f, 0)) {
            DustCloud_Create(em, 0, 0, 0.9f * em->scaleX);
        }
        break;
    case EM_MOTION_ID(0, HERM_MOT_SPIN_FLINCH):
        if (EmMotion_PassedFrameScaled(em, 66.0f, 0)) {
            EmHermitaur_SpawnGroundEffect(em, HERM_BONE_SIDE_B, 7, 6.0f);
        }
        break;
    case EM_MOTION_ID(0, HERM_MOT_KNOCK_DOWN_B):
        if (EmMotion_PassedFrameScaled(em, 28.0f, 0)) {
            EmHermitaur_SpawnGroundEffect(em, HERM_BONE_SIDE_B, 7, 6.0f);
        }
        break;
    case EM_MOTION_ID(0, HERM_MOT_RECOIL):
        if (em->type == EM_KIND_DAIMYO_HERMITAUR || em->type == EM_KIND_HERMITAUR || em->type == EM_KIND_CEANATAUR) {
            if (EmMotion_PassedFrameScaled(em, 122.0f, 0)) {
                EmHermitaur_SpawnGroundEffect(em, HERM_BONE_SIDE_B, 7, 6.0f);
            }
        }
        break;
    default:
        EmHermitaur_MotionEffectsDefault(em);
        break;
    }
}

/*
 * Callback slot 3 (Em_RunMotionFx): skips the first frame (fxStep 0 -> 1),
 * then runs MotionEffects every frame.
 */
void EmHermitaur_UpdateMotionEffects(Em *em)
{
    EmHermitaurWork *w = (EmHermitaurWork *)&em->work;

    switch (w->fxStep) {
    case 0:
        w->fxStep++;
        break;
    case 1:
        EmHermitaur_MotionEffects(em, (EmWork *) w);
        break;
    }
}

/*
 * Callback slot 4: answers script queries: 1 stateArmor set; 2 can use a
 * ceiling (0 only for a Shogun without one); 3 shell swap allowed (0 for a
 * Shogun whose shell is intact or already broken twice); 4 Shogun with
 * shell variant 0; else 0.
 */
s32 EmHermitaur_ScriptQuery(Em *em, s32 query)
{
    EmHermitaurWork *w = (EmHermitaurWork *)&em->work;

    s32 q;

    q = query & 0xFF;
    switch (q) {
    case 1:
        return em->stateArmor != 0;
    case 2:
        if ((em->type == EM_KIND_SHOGUN_CEANATAUR) && (em->hasCeiling == 0)) {
            return 0;
        }
        return 1;
    case 3:
        if ((em->type == EM_KIND_SHOGUN_CEANATAUR) && (((s32) em->parts[2].state < 2) || ((s32) w->shellBreakCount >= 2))) {
            return 0;
        }
        return 1;
    case 4:
        if ((em->type == EM_KIND_SHOGUN_CEANATAUR) && (em->variant == 0)) {
            return 1;
        }
        return 0;
    default:
        return 0;
    }
}

/*
 * Returns 1 when Em.unk471 is nonzero; GunFx_Run (sprayLevel 1) and the
 * spray's check for the spraying motions use it.
 */
s32 EmHermitaur_SprayBoosted(Em *em)
{
    return em->unk471 != 0;
}

/*
 * Returns 1 for AREA_SWAMP_3, _4, _8 and AREA_VOLCANO_5 (day ids): Update
 * then assumes a ceiling at 1500 when the monster is not in the loaded
 * area.
 */
s32 EmHermitaur_AreaHasCeiling(u16 area)
{
    switch (Area_DayId(area) & 0xFFFF) {
    case AREA_SWAMP_3:
    case AREA_SWAMP_4:
    case AREA_SWAMP_8:
    case AREA_VOLCANO_5:
        return 1;
    }
    return 0;
}

/*
 * Callback slot 6 (EmVt.part8DownQuery): 1 only for the Daimyo when not
 * airborne (EM_STANCE_AIR), without stateArmor and with partBreakCount < 2;
 * else 0.
 */
s32 EmHermitaur_AllowPartBreak(Em *em)
{
    if (em->type != EM_KIND_DAIMYO_HERMITAUR) {
        return 0;
    }
    if ((em->stance != EM_STANCE_AIR) && (em->stateArmor == 0) && ((s8) em->partBreakCount < 2)) {
        return 1;
    }
    return 0;
}


/* The file's initialised data, in the game's order */
EmTurnParams EmHermitaur_TurnParams[1] = {
#include "game/008C4A90.inc"
};
f32 EmHermitaur_ThrustOffsets[20] = {
#include "game/008C4AB0.inc"
};
Vec3f EmHermitaur_CarveThrowPt = {
#include "game/008C4B00.inc"
};
void *EmHermitaur_Callbacks[8] = {
    EmHermitaur_SpawnEffects, EmHermitaur_SpawnEffects, EmHermitaur_TableNop, EmHermitaur_UpdateMotionEffects,
    EmHermitaur_ScriptQuery, EmHermitaur_Reinit, EmHermitaur_AllowPartBreak, NULL,
};
