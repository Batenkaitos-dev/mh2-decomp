/*
 * Monster family: Velociprey, Genprey, Ioprey and Giaprey (EM_KIND_VELOCIPREY,
 * EM_KIND_GENPREY, EM_KIND_IOPREY and EM_KIND_GIAPREY), which share one set of
 * state functions (EmVelociprey_*). The code from the family's first
 * function, EmVelociprey_MotionSounds (its motions' sounds), to its movement
 * curves: Init and Start (with the points where a pack appears, by area),
 * one function per state, a dispatcher per state group (Run*State) and
 * ChangeState, the per-frame Update that turns Em_UpdateDamage's result into
 * reactions, the motion effects and the curves. The state functions take
 * the monster and its work area (EmVelocipreyWork, the family's view of
 * EmWork); Em.step is the step within the current state. include/
 * em_velociprey.h names the family's motions and states.
 */
#include "common.h"
#include "scene.h"
#include "quest.h"
#include "em.h"
#include "shadow.h"
#include "em_velociprey.h"
#include "em_drome.h"
#include "objattach.h"
#include "player.h"
#include "area.h"
#include "capsule.h"
#include "sound.h"
#include "engine_math.h"
#include "gm_em.h"
#include "gm_load.h"
#include "part_effects.h"
#include "vec_math.h"
#include "em_script.h"
/* unit's own declaration: declared here as the callers use it; with the definition's void (Em *em, s32 group, s32 state, u8 mode), the calls in EmAnteka_ChangeState, EmKhezu_ChangeState and others compile differently */
void Em_SetStateSync(Em *em, u16 group, u16 state, u8 mode);
void EmRangedAttack_CreateAtBone(Em *, s32, s32, s32, s32, s32);
/* unit's own declaration: declared here as the callers use it; with the definition's void (Em *em, u8 k, s16 v, s16 t, f32 y, f32 scale), the calls in EmKhezu_HeldDowned compile differently */
void StunStars_CreateEm(Em *, s32, s32, s32, f32, f32);
s32 EmMotion_PassedFrameScaled(void *p, f32 frame, s32 layer);
extern EmTurnParams EmVelociprey_TurnParams[2];
void Em_SpawnLyingPuffs(Em *em, s32 bone, f32 *pos, f32 scale);
extern s16 gEmCombatInit[];
extern EmVelocipreyAppearPoint EmVelociprey_AppearPtsFortress6[];
extern EmVelocipreyAppearPoint EmVelociprey_AppearPtsFortress6_PosY[];
extern EmVelocipreyAppearPoint EmVelociprey_AppearPtsFortress6_PosZ[];
extern EmVelocipreyAppearPoint EmVelociprey_AppearPtsFortress6_TargetX[];
extern EmVelocipreyAppearPoint EmVelociprey_AppearPtsFortress6_TargetY[];
extern EmVelocipreyAppearPoint EmVelociprey_AppearPtsFortress6_TargetZ[];
extern EmVelocipreyAppearPoint EmVelociprey_AppearPtsForestHills2[];
extern EmVelocipreyAppearPoint EmVelociprey_AppearPtsForestHills2_PosY[];
extern EmVelocipreyAppearPoint EmVelociprey_AppearPtsForestHills2_PosZ[];
extern EmVelocipreyAppearPoint EmVelociprey_AppearPtsForestHills2_TargetX[];
extern EmVelocipreyAppearPoint EmVelociprey_AppearPtsForestHills2_TargetY[];
extern EmVelocipreyAppearPoint EmVelociprey_AppearPtsForestHills2_TargetZ[];
extern EmVelocipreyAppearPoint EmVelociprey_AppearPtsForestHills1[];
extern EmVelocipreyAppearPoint EmVelociprey_AppearPtsForestHills1_PosY[];
extern EmVelocipreyAppearPoint EmVelociprey_AppearPtsForestHills1_PosZ[];
extern EmVelocipreyAppearPoint EmVelociprey_AppearPtsForestHills1_TargetX[];
extern EmVelocipreyAppearPoint EmVelociprey_AppearPtsForestHills1_TargetY[];
extern EmVelocipreyAppearPoint EmVelociprey_AppearPtsForestHills1_TargetZ[];
extern f32 EmVelociprey_AppearPtSnowyMountains3_PosX;
extern f32 EmVelociprey_AppearPtSnowyMountains3_PosY;
extern f32 EmVelociprey_AppearPtSnowyMountains3_PosZ;
extern f32 EmVelociprey_AppearPtSnowyMountains3_TargetX;
extern f32 EmVelociprey_AppearPtSnowyMountains3_TargetY;
extern f32 EmVelociprey_AppearPtSnowyMountains3_TargetZ;
extern EmVelocipreyAppearAt EmVelociprey_AppearPtsForestHills10[];
extern EmVelocipreyAppearAt EmVelociprey_AppearPtsForestHills9[];
extern EmVelocipreyAppearAt EmVelociprey_AppearPtsJungle2[];
extern EmVelocipreyAppearAt EmVelociprey_AppearPtsJungle3[];
extern EmVelocipreyAppearAt EmVelociprey_AppearPtsJungle4[];
extern EmVelocipreyAppearAt EmVelociprey_AppearPtsJungle6[];
extern EmVelocipreyAppearAt EmVelociprey_AppearPtsJungle9[];
extern EmVelocipreyAppearAt EmVelociprey_AppearPtsTower6[];
extern EmVelocipreyAppearAt EmVelociprey_AppearPtsTower7[];
extern EmVelocipreyAppearAt EmVelociprey_AppearPtsDesert2[];
extern EmVelocipreyAppearAt EmVelociprey_AppearPtsDesert4[];
extern EmVelocipreyAppearAt EmVelociprey_AppearPtsDesert7[];
extern EmVelocipreyAppearAt EmVelociprey_AppearPtsSwamp5[];
extern EmVelocipreyAppearAt EmVelociprey_AppearPtsSwamp8[];
extern EmVelocipreyAppearAt EmVelociprey_AppearPtsSwamp9[];
extern EmVelocipreyAppearAt EmVelociprey_AppearPtsVolcano3[];
extern EmVelocipreyAppearAt EmVelociprey_AppearPtsVolcano6[];
extern EmVelocipreyAppearAt EmVelociprey_AppearPtsBattleTown1[];
extern EmVelocipreyAppearAt EmVelociprey_AppearPtsBattleTown2[];
extern EmVelocipreyAppearAt EmVelociprey_AppearPtsBattleTown3[];
/* end of prototypes */

/*
 * The Velociprey family's motion sounds: footsteps, cries and thuds at
 * fixed frames of the layer-0 motion kept in work->prevMotion (one picked
 * at random by Sound_RandomSe). Called every frame by
 * EmVelociprey_MotionEffects. The monster comes as an s32 and is cast at
 * each call, which the match needs. EmDrome_MotionSounds is the dromes'
 * copy, with some other sounds.
 */
void EmVelociprey_MotionSounds(s32 em, EmWork *work)
{
    s16 prevMotion;

    prevMotion = work->prevMotion;
    switch (prevMotion) {
    case EM_MOTION_ID(0, VPREY_MOT_IDLE):
        Em_PlaySeAtBoneOnFrame((Em *) em, 200, Sound_RandomSe(14, 2, 14, 2), DROME_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, VPREY_MOT_WALK):
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 2, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 32, 2, DROME_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, VPREY_MOT_HOP):
        Em_PlaySeAtBoneOnFrame((Em *) em, 20, 8, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 16, 2, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 20, 3, DROME_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, VPREY_MOT_HOP_UP):
        Em_PlaySeAtBoneOnFrame((Em *) em, 22, 8, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 22, 2, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 22, 3, DROME_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, VPREY_MOT_HOP_LAND):
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 2, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 10, 1, DROME_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, VPREY_MOT_TURN_LEFT):
    case EM_MOTION_ID(0, VPREY_MOT_TURN_RIGHT):
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, Sound_RandomSe(5, 2, 5, 2), DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 26, 2, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 26, 4, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 42, 2, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 48, 1, DROME_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, VPREY_MOT_MOVE_ACTION_A):
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 2, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 4, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 26, 2, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 32, 1, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 64, 7, DROME_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, VPREY_MOT_MOVE_ACTION_B):
    case EM_MOTION_ID(0, VPREY_MOT_MOVE_ACTION_C):
        Em_PlaySeAtBoneOnFrame((Em *) em, 12, 2, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 12, 4, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 30, 2, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 36, 1, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 64, 7, DROME_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, VPREY_MOT_ALERT):
        Em_PlaySeAtBoneOnFrame((Em *) em, 68, 17, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 164, 18, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 34, 23, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 260, 23, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 288, 23, DROME_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, VPREY_MOT_IDLE_B):
        Em_PlaySeAtBoneOnFrame((Em *) em, 118, 14, DROME_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, VPREY_MOT_LEAP):
        Em_PlaySeAtBoneOnFrame((Em *) em, 22, 2, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 22, 3, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 22, 8, DROME_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, VPREY_MOT_LEAP_LAND):
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 2, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 1, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 36, 0, DROME_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, VPREY_MOT_STRIKE):
        Em_PlaySeAtBoneOnFrame((Em *) em, 40, 2, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 60, 2, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 118, 2, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 26, 16, DROME_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, VPREY_MOT_POUNCE):
        Em_PlaySeAtBoneOnFrame((Em *) em, 22, 2, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 22, 3, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 22, 8, DROME_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, VPREY_MOT_POUNCE_LAND):
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 2, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 1, DROME_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, VPREY_MOT_QUICK_STRIKE):
        Em_PlaySeAtBoneOnFrame((Em *) em, 2, 16, DROME_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, VPREY_MOT_SHOOT):
        Em_PlaySeAtBoneOnFrame((Em *) em, 60, 0, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 128, 2, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 72, 28, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 60, 8, DROME_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, VPREY_MOT_FLINCH):
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 9, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 30, 2, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 28, 1, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 54, 8, DROME_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, VPREY_MOT_KNOCKED_BACK):
    case EM_MOTION_ID(0, VPREY_MOT_KNOCKED_FORWARD):
    case EM_MOTION_ID(0, VPREY_MOT_KNOCKED_SIDE_B):
    case EM_MOTION_ID(0, VPREY_MOT_KNOCKED_SIDE_A):
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 11, DROME_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, VPREY_MOT_KNOCKED_BACK_LAND):
    case EM_MOTION_ID(0, VPREY_MOT_KNOCKED_FORWARD_LAND):
    case EM_MOTION_ID(0, VPREY_MOT_KNOCKED_SIDE_B_LAND):
    case EM_MOTION_ID(0, VPREY_MOT_KNOCKED_SIDE_A_LAND):
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 15, DROME_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, VPREY_MOT_GET_UP):
        Em_PlaySeAtBoneOnFrame((Em *) em, 24, 3, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 44, 1, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 54, 2, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 88, 0, DROME_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, VPREY_MOT_DAZED):
        Em_PlaySeAtBoneOnFrame((Em *) em, 180, 9, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 132, 11, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 52, 10, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 68, 1, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 86, 0, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 132, 1, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 140, 0, DROME_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, VPREY_MOT_COLLAPSE):
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 17, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 66, 17, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 130, 18, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 48, 23, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 122, 0, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 162, 15, DROME_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, VPREY_MOT_LIE):
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 14, DROME_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, VPREY_MOT_DIE):
        Em_PlaySeAtBoneOnFrame((Em *) em, 10, 13, DROME_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, VPREY_MOT_STATUS4):
        Em_PlaySeAtBoneOnFrame((Em *) em, 14, 25, DROME_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, VPREY_MOT_IDLE_ACTION_A):
        Em_PlaySeAtBoneOnFrame((Em *) em, 32, 12, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 68, 12, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 98, 6, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 20, 0, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 160, 23, DROME_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, VPREY_MOT_CALL):
        Em_PlaySeAtBoneOnFrame((Em *) em, 10, 7, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 68, 7, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 1, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 122, 1, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 134, 0, DROME_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, VPREY_MOT_SHAKE_OFF):
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 5, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 98, 5, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 22, 1, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 58, 0, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 106, 1, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame((Em *) em, 158, 0, DROME_BONE_ROOT);
        /* no break: the cases below are empty */
    case EM_MOTION_ID(0, VPREY_MOT_EVENT_IDLE):
    default:
        break;
    }
}

/*
 * Callback slots 0 and 1 (EmVt.init, update): makes the monster's effect
 * 01, as the other families do.
 */
void EmVelociprey_SpawnShadow(u8 *em)
{
    Shadow_Spawn((Player *) em, SHADOW_MODEL);
}

/*
 * Sets up a Velociprey, Genprey, Ioprey or Giaprey. Outside a quest
 * (gQuest.stage 0) it is placed on a test grid by its slot number (rows of
 * 8, 150 apart, from a corner that depends on the area). Then the kind's
 * motion speed, the idle motion, full hp from Em_ComputeMaxHp, the spawn
 * position and heading kept for EmVelociprey_Respawn, and the parts. A pack
 * member (Em.spawnParam 1) waits for senses and links to its leader
 * (Em_FindLink); an Ioprey of variant 1 starts in combat.
 */
void EmVelociprey_Init(Em *em)
{
    EmVelocipreyWork *w = (EmVelocipreyWork *)&em->work;
    s16 hp;

    if (gQuest.stage == 0) {
        switch (gScene.area) {
        case AREA_SNOWY_MOUNTAINS_NIGHT_9:
            em->posX = 9500.0f + 150.0f * (f32) (em->slotNo / 8);
            em->posY = 0.0f;
            em->posZ = 9200.0f + 150.0f * (f32) (em->slotNo & 7);
            break;
        case 13: /* an area AreaId doesn't name */
            em->posX = 5000.0f + 150.0f * (f32) (em->slotNo / 8);
            em->posY = 0.0f;
            em->posZ = 9000.0f + 150.0f * (f32) (em->slotNo & 7);
            break;
        case AREA_FOREST_HILLS_4:
            em->posX = 10000.0f + 150.0f * (f32) (em->slotNo / 8);
            em->posY = 0.0f;
            em->posZ = 10000.0f + 150.0f * (f32) (em->slotNo & 7);
            em->rotY = 0;
            break;
        case AREA_FOREST_HILLS_2:
            em->posX = 8400.0f + 150.0f * (f32) (em->slotNo / 8);
            em->posY = 0.0f;
            em->posZ = 9000.0f + 150.0f * (f32) (em->slotNo & 7);
            em->rotY = ANGLE_TURN * 3 / 16;
            break;
        default:
            em->posX = 2400.0f + 150.0f * (f32) em->slotNo;
            em->posY = 0.0f;
            em->posZ = 3600.0f;
            break;
        }
    }
    switch (em->type) {
    case EM_KIND_GENPREY:
        em->motionSpeed = 1.2f;
        break;
    case EM_KIND_VELOCIPREY:
    case EM_KIND_GIAPREY:
        em->motionSpeed = 1.0f;
        break;
    case EM_KIND_IOPREY:
        em->motionSpeed = 1.1f;
        break;
    }
    Em_SetMotionRow(em, VPREY_MOT_IDLE, 0, 0);
    em->stance = EM_STANCE_GROUND;
    hp = Em_ComputeMaxHp(em, 0);
    em->hp = hp;
    em->hpMax = hp;
    em->scriptPending = 0;
    em->blind = 0;
    w->callActive = 0;
    w->spawnPos[0] = em->posX;
    w->spawnPos[1] = em->posY;
    w->spawnPos[2] = em->posZ;
    w->spawnRotY = em->rotY;
    Em_InitParts(em);
    em->remote = 0;
    w->hunted = 0;
    switch (em->spawnParam) {
    case 1:
        em->senseDelay = 5;
        Em_FindLink(em);
        break;
    }
    if (em->type == EM_KIND_IOPREY && em->variant == 1) {
        Em_SetCombat(em, 1, gEmCombatInit[em->type]);
    }
}

/*
 * Init, then VPREY_IDLE_SPAWN, or VPREY_IDLE_PACK_WAIT for a pack member
 * (Em.spawnParam 1). GmEm's setup calls it for the four kinds.
 */
void EmVelociprey_Start(Em *em)
{
    EmVelociprey_Init(em);
    EmVelociprey_ChangeState(em, EM_GROUP_IDLE, VPREY_IDLE_SPAWN, EM_SYNC_LOCAL);
    switch (em->spawnParam) {
    case 1:
        EmVelociprey_ChangeState(em, EM_GROUP_IDLE, VPREY_IDLE_PACK_WAIT, EM_SYNC_LOCAL);
        break;
    }
}

/* Callback slot 5 (EmVt.appear): Init again, the state kept. */
void EmVelociprey_Reinit(Em *em)
{
    EmVelociprey_Init(em);
}

/*
 * Brings a hunted raptor back where it first stood
 * (VPREY_DEFEAT_FADE_RESPAWN's end): resets the monster (GmEm_ResetState),
 * full hp, the idle motion, VPREY_IDLE_SPAWN or, for a pack member,
 * VPREY_IDLE_PACK_WAIT, and the spawn position and heading Init kept.
 */
void EmVelociprey_Respawn(Em *em)
{
    EmVelocipreyWork *w = (EmVelocipreyWork *)&em->work;

    s16 fullHp;

    GmEm_ResetState(em);
    Em_SetMotionRow(em, VPREY_MOT_IDLE, 0, 0);
    em->stance = EM_STANCE_GROUND;
    EmVelociprey_ChangeState(em, EM_GROUP_IDLE, VPREY_IDLE_SPAWN, EM_SYNC_LOCAL);
    fullHp = Em_ComputeMaxHp(em, 0);
    em->hp = fullHp;
    em->hpMax = fullHp;
    em->blind = 0;
    w->callActive = 0;
    Em_InitParts(em);
    em->remote = 0;
    w->hunted = 0;
    switch (em->spawnParam) {
    case 1:
        em->senseDelay = 5;
        Em_FindLink(em);
        EmVelociprey_ChangeState(em, EM_GROUP_IDLE, VPREY_IDLE_PACK_WAIT, EM_SYNC_LOCAL);
        break;
    }
    em->posX = w->spawnPos[0];
    em->posY = w->spawnPos[1];
    em->posZ = w->spawnPos[2];
    em->prevPosX = em->posX;
    em->prevPosY = em->posY;
    em->prevPosZ = em->posZ;
    em->rotY = w->spawnRotY;
    em->scriptPending = 0;
}

/*
 * Ends a state: the kind's motion speed, on the ground, no look-at, and
 * VPREY_IDLE with the script asked for the next state. The two last
 * arguments are unused.
 */
void EmVelociprey_EndState(Em *em, s32 unused1, s32 unused2)
{
    switch (em->type) {
    case EM_KIND_GENPREY:
        em->motionSpeed = 1.2f;
        break;
    case EM_KIND_VELOCIPREY:
    case EM_KIND_GIAPREY:
        em->motionSpeed = 1.0f;
        break;
    case EM_KIND_IOPREY:
        em->motionSpeed = 1.1f;
        break;
    }
    em->stance = EM_STANCE_GROUND;
    em->lookAtTarget = 0;
    em->scriptPending = 1;
    EmVelociprey_ChangeState(em, EM_GROUP_IDLE, VPREY_IDLE, EM_SYNC_LOCAL);
}

/*
 * VPREY_IDLE_STAND: the idle motion for 30 to 120 frames (by Em.stateRand),
 * then asks the script.
 */
void EmVelociprey_IdleStand(Em *em, EmWork *work)
{
    em->motionSpeed = 1.2f;
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = ((em->stateRand & 3) * 30) + 30;
        if (em->motionId[0] != EM_MOTION_ID(0, VPREY_MOT_IDLE)) {
            Em_SetMotion(em, VPREY_MOT_IDLE, 0, 0);
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
 * VPREY_IDLE and VPREY_IDLE_WAIT: the idle motion (unless it is already
 * playing); once it ends, asks the script every frame until the script
 * changes the state.
 */
void EmVelociprey_Idle(Em *em, EmWork *work)
{
    em->motionSpeed = 1.2f;
    switch (em->step) {
    case 0:
        em->step++;
        if (em->motionId[0] != EM_MOTION_ID(0, VPREY_MOT_IDLE)) {
            Em_SetMotion(em, VPREY_MOT_IDLE, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->scriptPending = 1;
        }
        break;
    }
}

/* VPREY_IDLE_ACTION_A: one motion, then EmVelociprey_EndState. */
void EmVelociprey_IdleActionA(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_IDLE_ACTION_A, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * VPREY_IDLE_PACK_CALL: the call motion; at its frame 40 the monster puts
 * its call object (EmVelocipreyWork.callObj, radius 1000, kind 0) at its
 * position in the sense list, where the other raptors' Em_SenseObjects
 * notices it. The object is removed when the motion ends or the state
 * changes.
 */
void EmVelociprey_PackCall(Em *em, EmWork *work)
{
    EmVelocipreyWork *w = (EmVelocipreyWork *)work;

    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_CALL, 0, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            Vec_Set(w->callObj.pos, em->posX, em->posY, em->posZ);
            *(s32 *)&w->callObj.radius = 0x447A0000; /* 1000.0f, stored as its bits */
            w->callObj.area = (s16) *(u8 *)&em->area;
            w->callObj.kind = 0;
            EmSenseObjs_Add(&w->callObj);
            w->callActive = 1;
        }
        if (em->motion[0].playing == 0) {
            if (w->callActive != 0) {
                EmSenseObjs_Remove(&w->callObj);
                w->callActive = 0;
            }
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/* VPREY_IDLE_SHAKE_OFF: the shake-off motion, then EmVelociprey_EndState. */
void EmVelociprey_ShakeOff(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_SHAKE_OFF, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * VPREY_IDLE_ALERT, the combat idle: the alert motion once, then
 * EmVelociprey_EndState.
 */
void EmVelociprey_IdleAlert(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_ALERT, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * VPREY_IDLE_GET_UP, after the four knock-backs: the get-up motion, then
 * EmVelociprey_EndState.
 */
void EmVelociprey_GetUp(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_GET_UP, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * VPREY_IDLE_STATUS2: blinded and waking on a hit (EM_HIT_REACT_WAKE)
 * throughout; collapses (Em_BeginStatus2), then lies until the status timer
 * runs out and goes to VPREY_IDLE_STATUS2_END.
 */
void EmVelociprey_Status2Collapse(Em *em, EmWork *work)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_COLLAPSE, 0, 0);
        Em_BeginStatus2(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, VPREY_MOT_LIE, 0, 0);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            EmVelociprey_ChangeState(em, EM_GROUP_IDLE, VPREY_IDLE_STATUS2_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * VPREY_IDLE_STATUS2_END: ends status 2 and gets up, blinded until the
 * motion ends, then EmVelociprey_EndState.
 */
void EmVelociprey_Status2End(Em *em, EmWork *work)
{
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_GET_UP, 0, 0);
        Em_EndStatus2(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->blind = 0;
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * VPREY_IDLE_BLINDED: the dazed motion until Em.blindTimer runs out, then
 * EmVelociprey_EndState.
 */
void EmVelociprey_Blinded(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        if (em->motionId[0] != EM_MOTION_ID(0, VPREY_MOT_DAZED)) {
            Em_SetMotion(em, VPREY_MOT_DAZED, 0, 0);
        }
        break;
    case 1:
        if (em->blindTimer == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * VPREY_IDLE_PACK_WAIT: a pack member waiting for its leader, unaware and
 * unpushable (Em.senseDelay and Em.noPushTimer held; EmVelociprey_Update
 * then keeps it unhittable). Plays VPREY_MOT_IDLE_B and asks the script
 * when it ends.
 */
void EmVelociprey_PackWait(Em *em, EmWork *work)
{
    em->motionSpeed = 1.2f;
    em->senseDelay = 5;
    em->noPushTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        if (em->motionId[0] != EM_MOTION_ID(0, VPREY_MOT_IDLE_B)) {
            Em_SetMotion(em, VPREY_MOT_IDLE_B, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->scriptPending = 1;
        }
        break;
    }
}

/*
 * VPREY_IDLE_STATUS4_END: ends status 4 with the alert motion, then
 * EmVelociprey_EndState.
 */
void EmVelociprey_Status4End(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_ALERT, 0, 0);
        Em_EndStatus4(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * VPREY_IDLE_SPAWN, after Start, Respawn and a reappearance: plays
 * VPREY_MOT_IDLE_B unless the idle motion is playing, then asks the script.
 */
void EmVelociprey_IdleSpawn(Em *em, EmWork *work)
{
    em->motionSpeed = 1.2f;
    switch (em->step) {
    case 0:
        em->step++;
        if (em->motionId[0] != EM_MOTION_ID(0, VPREY_MOT_IDLE)) {
            Em_SetMotion(em, VPREY_MOT_IDLE_B, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->scriptPending = 1;
        }
        break;
    }
}

/*
 * VPREY_IDLE_STUN_END, when the stun ends (EmVelociprey_SetIdleState ends
 * status 5): the shake-off motion, then EmVelociprey_EndState.
 */
void EmVelociprey_StunEnd(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, VPREY_MOT_SHAKE_OFF, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/* Runs the state of group 0, EM_GROUP_IDLE. */
void EmVelociprey_RunIdleState(Em *em, EmWork *work)
{
    switch (em->state) {
    case VPREY_IDLE_STAND:
        EmVelociprey_IdleStand(em, work);
        break;
    case VPREY_IDLE:
        EmVelociprey_Idle(em, work);
        break;
    case VPREY_IDLE_ACTION_A:
        EmVelociprey_IdleActionA(em, work);
        break;
    case VPREY_IDLE_PACK_CALL:
        EmVelociprey_PackCall(em, work);
        break;
    case VPREY_IDLE_SHAKE_OFF:
        EmVelociprey_ShakeOff(em, work);
        break;
    case VPREY_IDLE_ALERT:
        EmVelociprey_IdleAlert(em, work);
        break;
    case VPREY_IDLE_GET_UP:
        EmVelociprey_GetUp(em, work);
        break;
    case VPREY_IDLE_STATUS2:
        EmVelociprey_Status2Collapse(em, work);
        break;
    case VPREY_IDLE_STATUS2_END:
        EmVelociprey_Status2End(em, work);
        break;
    case VPREY_IDLE_BLINDED:
        EmVelociprey_Blinded(em, work);
        break;
    case VPREY_IDLE_PACK_WAIT:
        EmVelociprey_PackWait(em, work);
        break;
    case VPREY_IDLE_STATUS4_END:
        EmVelociprey_Status4End(em, work);
        break;
    case VPREY_IDLE_SPAWN:
        EmVelociprey_IdleSpawn(em, work);
        break;
    case VPREY_IDLE_WAIT:
        EmVelociprey_Idle(em, work);
        break;
    case VPREY_IDLE_STUN_END:
        EmVelociprey_StunEnd(em, work);
        
    default:
        break;
    }
}

/*
 * VPREY_MOVE_TURN: turns to the target with the turn row
 * EmVelociprey_TurnParams[0], then EmVelociprey_EndState.
 */
void EmVelociprey_TurnToTarget(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_StartTurnToTarget(em, &EmVelociprey_TurnParams[0]);
        break;
    case 1:
        if (Em_TurnStep(em, &EmVelociprey_TurnParams[0]) == 1) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * VPREY_MOVE_WALK: walks to the goal (Em_StepToGoal), counting frames. At
 * the goal, a local monster that walked over 60 frames and ended within 100
 * of where it started (it made no headway) hops forward
 * (VPREY_HOP_FORWARD); otherwise EmVelociprey_EndState.
 */
void EmVelociprey_Walk(Em *em, EmWork *work)
{
    f32 start[3];

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, VPREY_MOT_WALK, 0, 0);
        work->startXB = em->posX;
        work->startZB = em->posZ;
        work->walkFramesB = 0;
        break;
    case 1:
        work->walkFramesB++;
        if (Em_StepToGoal(em, 64, 0) == 1) {
            if (em->remote == 0) {
                start[0] = work->startXB;
                start[1] = em->posY;
                start[2] = work->startZB;
                if (Vec_DistanceXZ(&em->posX, start) < 100.0f && work->walkFramesB > 60) {
                    EmVelociprey_ChangeState(em, EM_GROUP_FLY, VPREY_HOP_FORWARD, EM_SYNC_OWNER_FLAGGED);
                    break;
                }
                EmVelociprey_EndState(em, 0, 0);
            } else {
                EmVelociprey_EndState(em, 0, 0);
            }
        }
        break;
    }
}

/*
 * VPREY_MOVE_ACTION_A: one motion (dust at frame 12), then
 * EmVelociprey_EndState.
 */
void EmVelociprey_MoveActionA(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_MOVE_ACTION_A, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/* VPREY_MOVE_ACTION_B: one motion, then EmVelociprey_EndState. */
void EmVelociprey_MoveActionB(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_MOVE_ACTION_B, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/* VPREY_MOVE_ACTION_C: one motion, then EmVelociprey_EndState. */
void EmVelociprey_MoveActionC(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_MOVE_ACTION_C, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * VPREY_MOVE_TURN_ALERT, the turn in combat: as EmVelociprey_TurnToTarget
 * with the turn row EmVelociprey_TurnParams[1].
 */
void EmVelociprey_TurnToTargetAlert(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_StartTurnToTarget(em, &EmVelociprey_TurnParams[1]);
        break;
    case 1:
        if (Em_TurnStep(em, &EmVelociprey_TurnParams[1]) == 1) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/* Runs the state of group 1, EM_GROUP_MOVE. */
void EmVelociprey_RunMoveState(Em *em, EmWork *work)
{
    switch (em->state) {
    case VPREY_MOVE_TURN:
        EmVelociprey_TurnToTarget(em, work);
        break;
    case VPREY_MOVE_WALK:
        EmVelociprey_Walk(em, work);
        break;
    case VPREY_MOVE_ACTION_A:
        EmVelociprey_MoveActionA(em, work);
        break;
    case VPREY_MOVE_ACTION_B:
        EmVelociprey_MoveActionB(em, work);
        break;
    case VPREY_MOVE_ACTION_C:
        EmVelociprey_MoveActionC(em, work);
        break;
    case VPREY_MOVE_TURN_ALERT:
        EmVelociprey_TurnToTargetAlert(em, work);
        
    default:
        break;
    }
}

/*
 * VPREY_HOP_FORWARD: the hop motion; from its frame 10 the monster is
 * airborne on the curves VPREY_CURVE_HOP_FORWARD (up and forward) until
 * they end at floor height; it lands there, then EmVelociprey_EndState.
 */
void EmVelociprey_HopForward(Em *em, EmWork *work)
{
    s32 curveEnded;

    curveEnded = 0;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, VPREY_MOT_HOP, 0, 0);
        EmVelociprey_StartCurves(em, VPREY_CURVE_HOP_FORWARD);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 10.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            curveEnded = EmVelociprey_UpdateCurves(em) & 0xFF;
        }
        if ((curveEnded != 0) && (em->posY <= em->floorY)) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            Em_SetMotion(em, VPREY_MOT_HOP_LAND, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * VPREY_HOP_UP: as EmVelociprey_HopForward on the spot (VPREY_CURVE_HOP_UP,
 * airborne from frame 22), without setting the height on landing.
 */
void EmVelociprey_HopUp(Em *em, EmWork *work)
{
    s32 curveEnded;

    curveEnded = 0;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, VPREY_MOT_HOP_UP, 0, 0);
        EmVelociprey_StartCurves(em, VPREY_CURVE_HOP_UP);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 22.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            curveEnded = EmVelociprey_UpdateCurves(em) & 0xFF;
        }
        if ((curveEnded != 0) && (em->posY <= em->floorY)) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, VPREY_MOT_HOP_LAND, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * VPREY_HOP_ARC: a hop on a set arc (up 60, forward 50, gravity 4, drag 1;
 * falling at most 50, forward at least 20), unaware throughout; lands, then
 * EmVelociprey_EndState. The VPREY_EVENT2 hops are the same with other
 * speeds.
 */
void EmVelociprey_HopArc(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, VPREY_MOT_HOP, 0, 0);
        em->stance = EM_STANCE_GROUND;
        Player_ClearVelocity(em);
        em->velY = 60.0f;
        em->velZ = 50.0f;
        em->accelY = -4.0f;
        em->accelZ = -1.0f;
        work->accelAngX = 0;
        work->accelAngZ = 0;
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 10.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            work->accelAngY = em->rotY;
            Em_MoveRotatedAccel(em, &work->accelAngX);
        }
        if (em->velY < -50.0f) {
            em->velY = -50.0f;
            em->accelY = 0.0f;
        }
        if (em->velZ < 20.0f) {
            em->velZ = 20.0f;
            em->accelZ = 0.0f;
        }
        if ((em->velY < 0.0f) && (em->posY <= em->floorY)) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            Em_SetMotion(em, VPREY_MOT_HOP_LAND, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/* Runs the state of group 2 (EM_GROUP_FLY): the hops. */
void EmVelociprey_RunHopState(Em *em, EmWork *work)
{
    switch (em->state) {
    case VPREY_HOP_FORWARD:
        EmVelociprey_HopForward(em, work);
        break;
    case VPREY_HOP_UP:
        EmVelociprey_HopUp(em, work);
        break;
    case VPREY_HOP_ARC:
        EmVelociprey_HopArc(em, work);
        break;
    }
}

/*
 * VPREY_ATK_STRIKE_A: the strike motion with its hit at frame 40
 * (VelocipreyAttack row 1, the Ioprey's 11), turning to the goal until
 * frame 68.
 */
void EmVelociprey_AttackStrikeA(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_STRIKE, 0, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            if (em->type == EM_KIND_IOPREY) {
                VelocipreyAttack_Create(em, 11);
            } else {
                VelocipreyAttack_Create(em, 1);
            }
        }
        if ((em->hasGoal != 0) && (EmMotion_ReachedFrame(em, 0, 68.0f) == 0)) {
            Em_TurnToGoal(em, 128, 0);
        }
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/* VPREY_ATK_NONE: ends at once. */
void EmVelociprey_AttackNone(Em *em)
{
    EmVelociprey_EndState(em, 0, 0);
}

/*
 * VPREY_ATK_LEAP_MEDIUM_A: a leap (up 75, forward 50) airborne from frame
 * 22, turning to the goal in the air; the hit at frame 40 by kind
 * (VelocipreyAttack row 3, the Genprey's 12, the Ioprey's 13). A wall in
 * front holds the forward speed (EmVelocipreyWork.wallVelZ) for the move;
 * lands, then EmVelociprey_EndState.
 */
void EmVelociprey_AttackLeapMediumA(Em *em, EmWork *work)
{
    EmVelocipreyWork *w = (EmVelocipreyWork *)work;
    u8 inFlight;

    inFlight = 0;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_LEAP, 0, 0);
        Player_ClearVelocity(em);
        em->velY = 75.0f;
        em->accelY = -10.0f;
        em->velZ = 50.0f;
        em->accelZ = -1.5f;
        work->pitchStep = 0;
        work->turnSpeed = 0;
        work->rollSpeed = 0;
        work->unk048 = 0;
        work->unk04C = 0;
        w->unk050 = 0;
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            switch (em->type) {
            case EM_KIND_VELOCIPREY:
            case EM_KIND_GIAPREY:
                VelocipreyAttack_Create(em, 3);
                break;
            case EM_KIND_GENPREY:
                VelocipreyAttack_Create(em, 12);
                break;
            case EM_KIND_IOPREY:
                VelocipreyAttack_Create(em, 13);
                break;
            }
        }
        if (EmMotion_ReachedFrame(em, 0, 22.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            work->accelAngX = 0;
            work->accelAngY = em->rotY;
            work->accelAngZ = 0;
            inFlight = 1;
            if (em->wallHitDirs & 0xF000000F) {
                w->wallVelZ = em->velZ;
                em->velZ = 0.0f;
            }
            Em_MoveRotatedAccel(em, &work->accelAngX);
            if (w->wallVelZ != 0.0f) {
                em->velZ = w->wallVelZ;
                w->wallVelZ = 0.0f;
            }
            if (em->velZ < 10.0f) {
                em->velZ = 10.0f;
                em->accelZ = 0.0f;
            }
        }
        if (em->hasGoal != 0 && em->stance == EM_STANCE_AIR) {
            Em_TurnToGoal(em, 256, 0);
        }
        if (inFlight && em->posY <= em->floorY) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            Em_SetMotion(em, VPREY_MOT_LEAP_LAND, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * VPREY_ATK_QUICK_STRIKE_A: the quick strike with its hit at frame 16
 * (VelocipreyAttack row 5, the Ioprey's 14).
 */
void EmVelociprey_AttackQuickStrikeA(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_QUICK_STRIKE, 0, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 16.0f, 0U) != 0) {
            switch (em->type) {
            case EM_KIND_VELOCIPREY:
            case EM_KIND_GIAPREY:
                VelocipreyAttack_Create(em, 5);
                break;
            case EM_KIND_GENPREY:
                VelocipreyAttack_Create(em, 5);
                break;
            case EM_KIND_IOPREY:
                VelocipreyAttack_Create(em, 14);
                break;
            }
        }
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * VPREY_ATK_SHOT_A and VPREY_ATK_SHOT_A_FAST: at frame 80 of the shoot
 * motion, an EMRANGED_PREY_SPIT from DROME_BONE_MOUTH, aimed slightly
 * down; fastShot picks its faster variant.
 */
void EmVelociprey_AttackShotA(Em *em, EmWork *work, s32 fastShot)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_SHOOT, 0, 0);
        break;
    case 1:
        switch (fastShot) {
        case 0:
        default:
            if (EmMotion_PassedFrameScaled(em, 80.0f, 0U) != 0) {
                EmRangedAttack_CreateAtBone(em, DROME_BONE_MOUTH, EMRANGED_PREY_SPIT, 0, 63717, 0);
            }
            break;
        case 1:
            if (EmMotion_PassedFrameScaled(em, 80.0f, 0U) != 0) {
                EmRangedAttack_CreateAtBone(em, DROME_BONE_MOUTH, EMRANGED_PREY_SPIT, 2, 63717, 0);
            }
            break;
        }
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * VPREY_ATK_LEAP_LONG_A: as EmVelociprey_AttackLeapMediumA, longer (forward
 * 100, slowing by 0.75), with VelocipreyAttack row 5 (the Genprey's 12,
 * the Ioprey's 15).
 */
void EmVelociprey_AttackLeapLongA(Em *em, EmWork *work)
{
    EmVelocipreyWork *w = (EmVelocipreyWork *)work;
    u8 inFlight;

    inFlight = 0;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_LEAP, 0, 0);
        Player_ClearVelocity(em);
        em->velY = 75.0f;
        em->accelY = -10.0f;
        em->velZ = 100.0f;
        em->accelZ = -0.75f;
        work->pitchStep = 0;
        work->turnSpeed = 0;
        work->rollSpeed = 0;
        work->unk048 = 0;
        work->unk04C = 0;
        w->unk050 = 0;
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            switch (em->type) {
            case EM_KIND_VELOCIPREY:
            case EM_KIND_GIAPREY:
                VelocipreyAttack_Create(em, 5);
                break;
            case EM_KIND_GENPREY:
                VelocipreyAttack_Create(em, 12);
                break;
            case EM_KIND_IOPREY:
                VelocipreyAttack_Create(em, 15);
                break;
            }
        }
        if (EmMotion_ReachedFrame(em, 0, 22.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            work->accelAngX = 0;
            work->accelAngY = em->rotY;
            work->accelAngZ = 0;
            inFlight = 1;
            if (em->wallHitDirs & 0xF000000F) {
                w->wallVelZ = em->velZ;
                em->velZ = 0.0f;
            }
            Em_MoveRotatedAccel(em, &work->accelAngX);
            if (w->wallVelZ != 0.0f) {
                em->velZ = w->wallVelZ;
                w->wallVelZ = 0.0f;
            }
            if (em->velZ < 10.0f) {
                em->velZ = 10.0f;
                em->accelZ = 0.0f;
            }
        }
        if (em->hasGoal != 0 && em->stance == EM_STANCE_AIR) {
            Em_TurnToGoal(em, 256, 0);
        }
        if (inFlight && em->posY <= em->floorY) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            Em_SetMotion(em, VPREY_MOT_LEAP_LAND, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * VPREY_ATK_LEAP_HIGH_A: as EmVelociprey_AttackLeapMediumA, higher (up 150,
 * gravity 20, forward 25), with VelocipreyAttack row 5 (the Genprey's 12,
 * the Ioprey's 16).
 */
void EmVelociprey_AttackLeapHighA(Em *em, EmWork *work)
{
    EmVelocipreyWork *w = (EmVelocipreyWork *)work;
    u8 inFlight;

    inFlight = 0;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_LEAP, 0, 0);
        Player_ClearVelocity(em);
        em->velY = 150.0f;
        em->accelY = -20.0f;
        em->velZ = 25.0f;
        em->accelZ = -3.0f;
        work->pitchStep = 0;
        work->turnSpeed = 0;
        work->rollSpeed = 0;
        work->unk048 = 0;
        work->unk04C = 0;
        w->unk050 = 0;
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            switch (em->type) {
            case EM_KIND_VELOCIPREY:
            case EM_KIND_GIAPREY:
                VelocipreyAttack_Create(em, 5);
                break;
            case EM_KIND_GENPREY:
                VelocipreyAttack_Create(em, 12);
                break;
            case EM_KIND_IOPREY:
                VelocipreyAttack_Create(em, 16);
                break;
            }
        }
        if (EmMotion_ReachedFrame(em, 0, 22.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            work->accelAngX = 0;
            work->accelAngY = em->rotY;
            work->accelAngZ = 0;
            inFlight = 1;
            if (em->wallHitDirs & 0xF000000F) {
                w->wallVelZ = em->velZ;
                em->velZ = 0.0f;
            }
            Em_MoveRotatedAccel(em, &work->accelAngX);
            if (w->wallVelZ != 0.0f) {
                em->velZ = w->wallVelZ;
                w->wallVelZ = 0.0f;
            }
            if (em->velZ < 10.0f) {
                em->velZ = 10.0f;
                em->accelZ = 0.0f;
            }
        }
        if (em->hasGoal != 0 && em->stance == EM_STANCE_AIR) {
            Em_TurnToGoal(em, 256, 0);
        }
        if (inFlight && em->posY <= em->floorY) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            Em_SetMotion(em, VPREY_MOT_LEAP_LAND, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * VPREY_ATK_STRIKE_B: as EmVelociprey_AttackStrikeA with VelocipreyAttack
 * row 7, turning to the goal throughout.
 */
void EmVelociprey_AttackStrikeB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_STRIKE, 0, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            VelocipreyAttack_Create(em, 7);
        }
        if (em->hasGoal != 0) {
            Em_TurnToGoal(em, 128, 0);
        }
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * VPREY_ATK_LEAP_MEDIUM_B: as EmVelociprey_AttackLeapMediumA with
 * VelocipreyAttack row 9 for every kind.
 */
void EmVelociprey_AttackLeapMediumB(Em *em, EmWork *work)
{
    EmVelocipreyWork *w = (EmVelocipreyWork *)work;
    u8 inFlight;

    inFlight = 0;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_LEAP, 0, 0);
        Player_ClearVelocity(em);
        em->velY = 75.0f;
        em->accelY = -10.0f;
        em->velZ = 50.0f;
        em->accelZ = -1.5f;
        work->pitchStep = 0;
        work->turnSpeed = 0;
        work->rollSpeed = 0;
        work->unk048 = 0;
        work->unk04C = 0;
        w->unk050 = 0;
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            VelocipreyAttack_Create(em, 9);
        }
        if (EmMotion_ReachedFrame(em, 0, 22.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            work->accelAngX = 0;
            work->accelAngY = em->rotY;
            work->accelAngZ = 0;
            inFlight = 1;
            if (em->wallHitDirs & 0xF000000F) {
                w->wallVelZ = em->velZ;
                em->velZ = 0.0f;
            }
            Em_MoveRotatedAccel(em, &work->accelAngX);
            if (w->wallVelZ != 0.0f) {
                em->velZ = w->wallVelZ;
                w->wallVelZ = 0.0f;
            }
            if (em->velZ < 10.0f) {
                em->velZ = 10.0f;
                em->accelZ = 0.0f;
            }
        }
        if (em->hasGoal != 0 && em->stance == EM_STANCE_AIR) {
            Em_TurnToGoal(em, 256, 0);
        }
        if (inFlight && em->posY <= em->floorY) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            Em_SetMotion(em, VPREY_MOT_LEAP_LAND, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * VPREY_ATK_QUICK_STRIKE_B: as EmVelociprey_AttackQuickStrikeA with
 * VelocipreyAttack row 10 for every kind.
 */
void EmVelociprey_AttackQuickStrikeB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_QUICK_STRIKE, 0, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 16.0f, 0U) != 0) {
            VelocipreyAttack_Create(em, 10);
        }
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * VPREY_ATK_LEAP_LONG_B: as EmVelociprey_AttackLeapLongA with
 * VelocipreyAttack row 9 for every kind.
 */
void EmVelociprey_AttackLeapLongB(Em *em, EmWork *work)
{
    EmVelocipreyWork *w = (EmVelocipreyWork *)work;
    u8 inFlight;

    inFlight = 0;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_LEAP, 0, 0);
        Player_ClearVelocity(em);
        em->velY = 75.0f;
        em->accelY = -10.0f;
        em->velZ = 100.0f;
        em->accelZ = -0.75f;
        work->pitchStep = 0;
        work->turnSpeed = 0;
        work->rollSpeed = 0;
        work->unk048 = 0;
        work->unk04C = 0;
        w->unk050 = 0;
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            VelocipreyAttack_Create(em, 9);
        }
        if (EmMotion_ReachedFrame(em, 0, 22.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            work->accelAngX = 0;
            work->accelAngY = em->rotY;
            work->accelAngZ = 0;
            inFlight = 1;
            if (em->wallHitDirs & 0xF000000F) {
                w->wallVelZ = em->velZ;
                em->velZ = 0.0f;
            }
            Em_MoveRotatedAccel(em, &work->accelAngX);
            if (w->wallVelZ != 0.0f) {
                em->velZ = w->wallVelZ;
                w->wallVelZ = 0.0f;
            }
            if (em->velZ < 10.0f) {
                em->velZ = 10.0f;
                em->accelZ = 0.0f;
            }
        }
        if (em->hasGoal != 0 && em->stance == EM_STANCE_AIR) {
            Em_TurnToGoal(em, 256, 0);
        }
        if (inFlight && em->posY <= em->floorY) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            Em_SetMotion(em, VPREY_MOT_LEAP_LAND, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * VPREY_ATK_LEAP_HIGH_B: as EmVelociprey_AttackLeapHighA with
 * VelocipreyAttack row 9 for every kind.
 */
void EmVelociprey_AttackLeapHighB(Em *em, EmWork *work)
{
    EmVelocipreyWork *w = (EmVelocipreyWork *)work;
    u8 inFlight;

    inFlight = 0;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_LEAP, 0, 0);
        Player_ClearVelocity(em);
        em->velY = 150.0f;
        em->accelY = -20.0f;
        em->velZ = 25.0f;
        em->accelZ = -3.0f;
        work->pitchStep = 0;
        work->turnSpeed = 0;
        work->rollSpeed = 0;
        work->unk048 = 0;
        work->unk04C = 0;
        w->unk050 = 0;
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            VelocipreyAttack_Create(em, 9);
        }
        if (EmMotion_ReachedFrame(em, 0, 22.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            work->accelAngX = 0;
            work->accelAngY = em->rotY;
            work->accelAngZ = 0;
            inFlight = 1;
            if (em->wallHitDirs & 0xF000000F) {
                w->wallVelZ = em->velZ;
                em->velZ = 0.0f;
            }
            Em_MoveRotatedAccel(em, &work->accelAngX);
            if (w->wallVelZ != 0.0f) {
                em->velZ = w->wallVelZ;
                w->wallVelZ = 0.0f;
            }
            if (em->velZ < 10.0f) {
                em->velZ = 10.0f;
                em->accelZ = 0.0f;
            }
        }
        if (em->hasGoal != 0 && em->stance == EM_STANCE_AIR) {
            Em_TurnToGoal(em, 256, 0);
        }
        if (inFlight && em->posY <= em->floorY) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            Em_SetMotion(em, VPREY_MOT_LEAP_LAND, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * VPREY_ATK_STRIKE_C: as EmVelociprey_AttackStrikeA with VelocipreyAttack
 * row 17.
 */
void EmVelociprey_AttackStrikeC(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_STRIKE, 0, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            VelocipreyAttack_Create(em, 17);
        }
        if ((em->hasGoal != 0) && (EmMotion_ReachedFrame(em, 0, 68.0f) == 0)) {
            Em_TurnToGoal(em, 128, 0);
        }
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * VPREY_ATK_SHOT_B and VPREY_ATK_SHOT_B_FAST: as EmVelociprey_AttackShotA
 * with EMRANGED_PREY_SPIT_B.
 */
void EmVelociprey_AttackShotB(Em *em, EmWork *work, s32 fastShot)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_SHOOT, 0, 0);
        break;
    case 1:
        switch (fastShot) {
        case 0:
        default:
            if (EmMotion_PassedFrameScaled(em, 80.0f, 0U) != 0) {
                EmRangedAttack_CreateAtBone(em, DROME_BONE_MOUTH, EMRANGED_PREY_SPIT_B, 0, 63717, 0);
            }
            break;
        case 1:
            if (EmMotion_PassedFrameScaled(em, 80.0f, 0U) != 0) {
                EmRangedAttack_CreateAtBone(em, DROME_BONE_MOUTH, EMRANGED_PREY_SPIT_B, 1, 63717, 0);
            }
            break;
        }
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/* Runs the state of group 3, EM_GROUP_ATTACK. */
void EmVelociprey_RunAttackState(Em *em, EmWork *work)
{
    switch (em->state) {
    case VPREY_ATK_STRIKE_A:
        EmVelociprey_AttackStrikeA(em);
        break;
    case VPREY_ATK_NONE:
        EmVelociprey_AttackNone(em);
        break;
    case VPREY_ATK_LEAP_MEDIUM_A:
        EmVelociprey_AttackLeapMediumA(em, work);
        break;
    case VPREY_ATK_QUICK_STRIKE_A:
        EmVelociprey_AttackQuickStrikeA(em, work);
        break;
    case VPREY_ATK_SHOT_A:
        EmVelociprey_AttackShotA(em, work, 0);
        break;
    case VPREY_ATK_LEAP_LONG_A:
        EmVelociprey_AttackLeapLongA(em, work);
        break;
    case VPREY_ATK_LEAP_HIGH_A:
        EmVelociprey_AttackLeapHighA(em, work);
        break;
    case VPREY_ATK_STRIKE_B:
        EmVelociprey_AttackStrikeB(em);
        break;
    case VPREY_ATK_LEAP_MEDIUM_B:
        EmVelociprey_AttackLeapMediumB(em, work);
        break;
    case VPREY_ATK_QUICK_STRIKE_B:
        EmVelociprey_AttackQuickStrikeB(em);
        break;
    case VPREY_ATK_LEAP_LONG_B:
        EmVelociprey_AttackLeapLongB(em, work);
        break;
    case VPREY_ATK_LEAP_HIGH_B:
        EmVelociprey_AttackLeapHighB(em, work);
        break;
    case VPREY_ATK_STRIKE_C:
        EmVelociprey_AttackStrikeC(em);
        break;
    case VPREY_ATK_SHOT_A_FAST:
        EmVelociprey_AttackShotA(em, work, 1);
        break;
    case VPREY_ATK_SHOT_B:
        EmVelociprey_AttackShotB(em, work, 0);
        break;
    case VPREY_ATK_SHOT_B_FAST:
        EmVelociprey_AttackShotB(em, work, 1);
        break;
    }
}

/*
 * VPREY_REACT_FLINCH, a hit on the ground: the flinch motion, the script
 * restarted, then EmVelociprey_EndState.
 */
void EmVelociprey_Flinch(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_FLINCH, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * VPREY_REACT_KNOCKED_BACK, a hit from the front (or a status in the air):
 * thrown backward from frame 10, lands, then VPREY_IDLE_GET_UP.
 */
void EmVelociprey_KnockedBack(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_KNOCKED_BACK, 0, 0);
        work->accelAngX = 0;
        work->accelAngY = em->rotY;
        work->accelAngZ = 0;
        Player_ClearVelocity(em);
        em->velY = 23.35f;
        em->velZ = -27.0f;
        em->accelY = -2.0f;
        Em_RestartScript(em);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 10.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            Em_MoveRotatedAccel(em, &work->accelAngX);
            if (em->posY <= em->floorY) {
                em->step++;
                em->stance = EM_STANCE_GROUND;
                Em_SetMotion(em, VPREY_MOT_KNOCKED_BACK_LAND, 0, 0);
                break;
            }
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmVelociprey_ChangeState(em, EM_GROUP_IDLE, VPREY_IDLE_GET_UP, EM_SYNC_LOCAL);
        }
        break;
    }
}

/*
 * VPREY_REACT_KNOCKED_FORWARD, a hit from behind: thrown the other way (the
 * move's heading is rotY plus a half turn, written as 0x7FFF + 1 for the
 * match), turning while it lands, then VPREY_IDLE_GET_UP.
 */
void EmVelociprey_KnockedForward(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_KNOCKED_FORWARD, 0, 0);
        work->accelAngX = 0;
        work->accelAngY = (em->rotY + 0x7FFF + 1) & 0xFFFF;
        work->accelAngZ = 0;
        Player_ClearVelocity(em);
        em->velY = 23.35f;
        em->velZ = -27.0f;
        em->accelY = -2.0f;
        Em_RestartScript(em);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 10.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            Em_MoveRotatedAccel(em, &work->accelAngX);
            if (em->posY <= em->floorY) {
                em->step++;
                em->stance = EM_STANCE_GROUND;
                Em_SetMotion(em, VPREY_MOT_KNOCKED_FORWARD_LAND, 0, 0);
                break;
            }
        }
        break;
    case 2:
        if (EmMotion_ReachedFrame(em, 0, 26.0f) == 0) {
            em->rotY -= 0x555;
        }
        if (em->motion[0].playing == 0) {
            EmVelociprey_ChangeState(em, EM_GROUP_IDLE, VPREY_IDLE_GET_UP, EM_SYNC_LOCAL);
        }
        break;
    }
}

/*
 * VPREY_REACT_KNOCKED_SIDE_A: thrown sideways, turning a quarter while it
 * lands, then VPREY_IDLE_GET_UP.
 */
void EmVelociprey_KnockedSideA(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_KNOCKED_SIDE_A, 0, 0);
        work->accelAngX = 0;
        work->accelAngY = (em->rotY - ANGLE_TURN / 4) & 0xFFFF;
        work->accelAngZ = 0;
        Player_ClearVelocity(em);
        em->velY = 23.35f;
        em->velZ = -27.0f;
        em->accelY = -2.0f;
        Em_RestartScript(em);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 10.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            Em_MoveRotatedAccel(em, &work->accelAngX);
            if (em->posY <= em->floorY) {
                em->step++;
                em->stance = EM_STANCE_GROUND;
                Em_SetMotion(em, VPREY_MOT_KNOCKED_SIDE_A_LAND, 0, 0);
                break;
            }
        }
        break;
    case 2:
        if (EmMotion_ReachedFrame(em, 0, 26.0f) == 0) {
            em->rotY -= 0x2AA;
        }
        if (em->motion[0].playing == 0) {
            EmVelociprey_ChangeState(em, EM_GROUP_IDLE, VPREY_IDLE_GET_UP, EM_SYNC_LOCAL);
        }
        break;
    }
}

/* VPREY_REACT_KNOCKED_SIDE_B: the mirror of EmVelociprey_KnockedSideA. */
void EmVelociprey_KnockedSideB(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_KNOCKED_SIDE_B, 0, 0);
        work->accelAngX = 0;
        work->accelAngY = (em->rotY + ANGLE_TURN / 4) & 0xFFFF;
        work->accelAngZ = 0;
        Player_ClearVelocity(em);
        em->velY = 23.35f;
        em->velZ = -27.0f;
        em->accelY = -2.0f;
        Em_RestartScript(em);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 10.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            Em_MoveRotatedAccel(em, &work->accelAngX);
            if (em->posY <= em->floorY) {
                em->step++;
                em->stance = EM_STANCE_GROUND;
                Em_SetMotion(em, VPREY_MOT_KNOCKED_SIDE_B_LAND, 0, 0);
                break;
            }
        }
        break;
    case 2:
        if (EmMotion_ReachedFrame(em, 0, 26.0f) == 0) {
            em->rotY += 0x2AA;
        }
        if (em->motion[0].playing == 0) {
            EmVelociprey_ChangeState(em, EM_GROUP_IDLE, VPREY_IDLE_GET_UP, EM_SYNC_LOCAL);
        }
        break;
    }
}

/*
 * VPREY_REACT_DAZE: the dazed motion once, the script restarted, then
 * EmVelociprey_EndState. EmVelociprey_Update never picks it.
 */
void EmVelociprey_Daze(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_DAZED, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * VPREY_REACT_STATUS4: Em_BeginStatus4 and the status motion while Em.timer
 * counts down, with an effect on bone 2; then VPREY_IDLE_STATUS4_END.
 */
void EmVelociprey_Status4(Em *em, EmWork *work)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_STATUS);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, VPREY_MOT_STATUS4, 0, 0);
        Em_BeginStatus4(em);
        Em_RestartScript(em);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmVelociprey_ChangeState(em, EM_GROUP_IDLE, VPREY_IDLE_STATUS4_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
    Em_SpawnStatus4Effect(em, 2);
}

/*
 * VPREY_REACT_STUN (status 5): the dazed motion and the stun's stars
 * (StageObj_Stun, for the low half of Em.timer frames) until Em.timer runs
 * out, then VPREY_IDLE_STUN_END.
 */
void EmVelociprey_Stun(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, VPREY_MOT_DAZED, 0, 0);
        StunStars_CreateEm(em, 1, DROME_BONE_STUN_STARS, *(s16 *)&em->timer, 50.0f, 1.0f);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmVelociprey_ChangeState(em, EM_GROUP_IDLE, VPREY_IDLE_STUN_END, EM_SYNC_LOCAL);
        }
        break;
    }
}

/* Runs the state of group 4, EM_GROUP_REACT (no state 7). */
void EmVelociprey_RunReactState(Em *em, EmWork *work)
{
    switch (em->state) {
    case VPREY_REACT_FLINCH:
        EmVelociprey_Flinch(em, work);
        break;
    case VPREY_REACT_KNOCKED_BACK:
        EmVelociprey_KnockedBack(em, work);
        break;
    case VPREY_REACT_KNOCKED_FORWARD:
        EmVelociprey_KnockedForward(em, work);
        break;
    case VPREY_REACT_KNOCKED_SIDE_A:
        EmVelociprey_KnockedSideA(em, work);
        break;
    case VPREY_REACT_KNOCKED_SIDE_B:
        EmVelociprey_KnockedSideB(em, work);
        break;
    case VPREY_REACT_DAZE:
        EmVelociprey_Daze(em, work);
        break;
    case VPREY_REACT_STATUS4:
        EmVelociprey_Status4(em, work);
        break;
    case VPREY_REACT_STUN:
        EmVelociprey_Stun(em, work);
        
    default:
        break;
    }
}

/*
 * VPREY_DEFEAT_DIE_BACK, killed by a hit from the front: hunted
 * (QuestEm_Hunted), thrown backward, lands, dies and lies as a carcass with
 * a carve point (EmMine) for 300 frames, fades out over 60 frames, then
 * VPREY_DEFEAT_REAPPEAR.
 */
void EmVelociprey_DieBack(Em *em, EmWork *work)
{
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        work->huntedB = 1;
        QuestEm_Hunted(em);
        Em_SetMotion(em, VPREY_MOT_KNOCKED_BACK, 0, 0);
        work->accelAngX = 0;
        work->accelAngY = em->rotY;
        work->accelAngZ = 0;
        Player_ClearVelocity(em);
        em->velY = 23.35f;
        em->velZ = -27.0f;
        em->accelY = -2.0f;
        em->stance = EM_STANCE_AIR;
        break;
    case 1:
        Em_MoveRotatedAccel(em, &work->accelAngX);
        if (em->posY <= em->floorY) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, VPREY_MOT_KNOCKED_BACK_LAND, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, VPREY_MOT_DIE, 0, 0);
            em->fieldCollide = 0;
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, VPREY_MOT_DEAD, 0, 0);
            em->timer = 300;
            EmMine_Create(em, 0);
        }
        break;
    case 4:
        EmMine_Update(em);
        if (--em->timer <= 0) {
            em->step++;
            EmMine_Remove(em);
        }
        break;
    case 5:
        em->alpha -= 1.0f / 60.0f;
        if (em->alpha <= 0.0f) {
            em->visible = 0;
            EmVelociprey_ChangeState(em, EM_GROUP_DEFEAT, VPREY_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * VPREY_DEFEAT_DIE_FORWARD, killed by a hit from behind: as
 * EmVelociprey_DieBack, thrown forward.
 */
void EmVelociprey_DieForward(Em *em, EmWork *work)
{
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        work->huntedB = 1;
        QuestEm_Hunted(em);
        Em_SetMotion(em, VPREY_MOT_KNOCKED_FORWARD, 0, 0);
        work->accelAngX = 0;
        work->accelAngY = (em->rotY + ANGLE_TURN / 2) & 0xFFFF;
        work->accelAngZ = 0;
        Player_ClearVelocity(em);
        em->velY = 23.35f;
        em->velZ = -27.0f;
        em->accelY = -2.0f;
        em->stance = EM_STANCE_AIR;
        break;
    case 1:
        Em_MoveRotatedAccel(em, &work->accelAngX);
        if (em->posY <= em->floorY) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, VPREY_MOT_KNOCKED_FORWARD_LAND, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, VPREY_MOT_DIE, 0, 0);
            em->fieldCollide = 0;
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, VPREY_MOT_DEAD, 0, 0);
            em->timer = 300;
            EmMine_Create(em, 0);
        }
        break;
    case 4:
        EmMine_Update(em);
        if (--em->timer <= 0) {
            em->step++;
            EmMine_Remove(em);
        }
        break;
    case 5:
        em->alpha -= 1.0f / 60.0f;
        if (em->alpha <= 0.0f) {
            em->visible = 0;
            EmVelociprey_ChangeState(em, EM_GROUP_DEFEAT, VPREY_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * VPREY_DEFEAT_DIE_SIDE_A: as EmVelociprey_DieBack, thrown sideways (a
 * quarter turn from rotY).
 */
void EmVelociprey_DieSideA(Em *em, EmWork *work)
{
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        work->huntedB = 1;
        QuestEm_Hunted(em);
        Em_SetMotion(em, VPREY_MOT_KNOCKED_SIDE_A, 0, 0);
        work->accelAngX = 0;
        work->accelAngY = (em->rotY - ANGLE_TURN / 4) & 0xFFFF;
        work->accelAngZ = 0;
        Player_ClearVelocity(em);
        em->velY = 23.35f;
        em->velZ = -27.0f;
        em->accelY = -2.0f;
        em->stance = EM_STANCE_AIR;
        break;
    case 1:
        Em_MoveRotatedAccel(em, &work->accelAngX);
        if (em->posY <= em->floorY) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, VPREY_MOT_KNOCKED_SIDE_A_LAND, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, VPREY_MOT_DIE, 0, 0);
            em->fieldCollide = 0;
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, VPREY_MOT_DEAD, 0, 0);
            em->timer = 300;
            EmMine_Create(em, 0);
        }
        break;
    case 4:
        EmMine_Update(em);
        if (--em->timer <= 0) {
            em->step++;
            EmMine_Remove(em);
        }
        break;
    case 5:
        em->alpha -= 1.0f / 60.0f;
        if (em->alpha <= 0.0f) {
            em->visible = 0;
            EmVelociprey_ChangeState(em, EM_GROUP_DEFEAT, VPREY_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/* VPREY_DEFEAT_DIE_SIDE_B: the mirror of EmVelociprey_DieSideA. */
void EmVelociprey_DieSideB(Em *em, EmWork *work)
{
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        work->huntedB = 1;
        QuestEm_Hunted(em);
        Em_SetMotion(em, VPREY_MOT_KNOCKED_SIDE_B, 0, 0);
        work->accelAngX = 0;
        work->accelAngY = (em->rotY + ANGLE_TURN / 4) & 0xFFFF;
        work->accelAngZ = 0;
        Player_ClearVelocity(em);
        em->velY = 23.35f;
        em->velZ = -27.0f;
        em->accelY = -2.0f;
        em->stance = EM_STANCE_AIR;
        break;
    case 1:
        Em_MoveRotatedAccel(em, &work->accelAngX);
        if (em->posY <= em->floorY) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, VPREY_MOT_KNOCKED_SIDE_B_LAND, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, VPREY_MOT_DIE, 0, 0);
            em->fieldCollide = 0;
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, VPREY_MOT_DEAD, 0, 0);
            em->timer = 300;
            EmMine_Create(em, 0);
        }
        break;
    case 4:
        EmMine_Update(em);
        if (--em->timer <= 0) {
            em->step++;
            EmMine_Remove(em);
        }
        break;
    case 5:
        em->alpha -= 1.0f / 60.0f;
        if (em->alpha <= 0.0f) {
            em->visible = 0;
            EmVelociprey_ChangeState(em, EM_GROUP_DEFEAT, VPREY_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * VPREY_DEFEAT_REAPPEAR: when the quest lets the monster appear again
 * (QuestEm_CanAppear), puts it at one of its area's appear points (by
 * Em.stateRand), facing the point's target or its heading, and hops it in
 * (group 7: VPREY_EVENT2_HOP_IN, _LOW or _HIGH by area). In an area without
 * appear points it waits in VPREY_IDLE_SPAWN, or VPREY_IDLE_PACK_WAIT when
 * linked. Refused, it advances its task phase.
 */
void EmVelociprey_Reappear(Em *em)
{
    f32 facePoint[4];
    s16 pointCount;
    u8 hopState;
    EmVelocipreyAppearAt *points;
    u16 pointIdx;

    switch (em->step) {
    case 0:
        if (QuestEm_CanAppear(em) == 1) {
            switch (Area_DayId(em->area)) {
            case AREA_FORTRESS_6:
                pointIdx = em->stateRand % 4;
                Vec_Set(&em->posX, EmVelociprey_AppearPtsFortress6[pointIdx].pos[0], EmVelociprey_AppearPtsFortress6_PosY[pointIdx].pos[0], EmVelociprey_AppearPtsFortress6_PosZ[pointIdx].pos[0]);
                Vec_Set(&em->prevPosX, EmVelociprey_AppearPtsFortress6[pointIdx].pos[0], EmVelociprey_AppearPtsFortress6_PosY[pointIdx].pos[0], EmVelociprey_AppearPtsFortress6_PosZ[pointIdx].pos[0]);
                Vec_Set(facePoint, EmVelociprey_AppearPtsFortress6_TargetX[pointIdx].pos[0], EmVelociprey_AppearPtsFortress6_TargetY[pointIdx].pos[0], EmVelociprey_AppearPtsFortress6_TargetZ[pointIdx].pos[0]);
                em->rotY = Math_AngleXZ(&em->posX, facePoint) & 0xFFFF;
                EmVelociprey_ChangeState(em, EM_GROUP_EVENT2, VPREY_EVENT2_HOP_IN_LOW, EM_SYNC_LOCAL);
                break;
            case AREA_FOREST_HILLS_2:
                pointIdx = em->stateRand % 5;
                Vec_Set(&em->posX, EmVelociprey_AppearPtsForestHills2[pointIdx].pos[0], EmVelociprey_AppearPtsForestHills2_PosY[pointIdx].pos[0], EmVelociprey_AppearPtsForestHills2_PosZ[pointIdx].pos[0]);
                Vec_Set(&em->prevPosX, EmVelociprey_AppearPtsForestHills2[pointIdx].pos[0], EmVelociprey_AppearPtsForestHills2_PosY[pointIdx].pos[0], EmVelociprey_AppearPtsForestHills2_PosZ[pointIdx].pos[0]);
                Vec_Set(facePoint, EmVelociprey_AppearPtsForestHills2_TargetX[pointIdx].pos[0], EmVelociprey_AppearPtsForestHills2_TargetY[pointIdx].pos[0], EmVelociprey_AppearPtsForestHills2_TargetZ[pointIdx].pos[0]);
                em->rotY = Math_AngleXZ(&em->posX, facePoint) & 0xFFFF;
                EmVelociprey_ChangeState(em, EM_GROUP_EVENT2, VPREY_EVENT2_HOP_IN_HIGH, EM_SYNC_LOCAL);
                break;
            case AREA_FOREST_HILLS_1:
                pointIdx = em->stateRand % 4;
                Vec_Set(&em->posX, EmVelociprey_AppearPtsForestHills1[pointIdx].pos[0], EmVelociprey_AppearPtsForestHills1_PosY[pointIdx].pos[0], EmVelociprey_AppearPtsForestHills1_PosZ[pointIdx].pos[0]);
                Vec_Set(&em->prevPosX, EmVelociprey_AppearPtsForestHills1[pointIdx].pos[0], EmVelociprey_AppearPtsForestHills1_PosY[pointIdx].pos[0], EmVelociprey_AppearPtsForestHills1_PosZ[pointIdx].pos[0]);
                Vec_Set(facePoint, EmVelociprey_AppearPtsForestHills1_TargetX[pointIdx].pos[0], EmVelociprey_AppearPtsForestHills1_TargetY[pointIdx].pos[0], EmVelociprey_AppearPtsForestHills1_TargetZ[pointIdx].pos[0]);
                em->rotY = Math_AngleXZ(&em->posX, facePoint) & 0xFFFF;
                EmVelociprey_ChangeState(em, EM_GROUP_EVENT2, VPREY_EVENT2_HOP_IN, EM_SYNC_LOCAL);
                break;
            case AREA_SNOWY_MOUNTAINS_3:
                Vec_Set(&em->posX, EmVelociprey_AppearPtSnowyMountains3_PosX, EmVelociprey_AppearPtSnowyMountains3_PosY, EmVelociprey_AppearPtSnowyMountains3_PosZ);
                Vec_Set(&em->prevPosX, EmVelociprey_AppearPtSnowyMountains3_PosX, EmVelociprey_AppearPtSnowyMountains3_PosY, EmVelociprey_AppearPtSnowyMountains3_PosZ);
                Vec_Set(facePoint, EmVelociprey_AppearPtSnowyMountains3_TargetX, EmVelociprey_AppearPtSnowyMountains3_TargetY, EmVelociprey_AppearPtSnowyMountains3_TargetZ);
                em->rotY = Math_AngleXZ(&em->posX, facePoint) & 0xFFFF;
                EmVelociprey_ChangeState(em, EM_GROUP_EVENT2, VPREY_EVENT2_HOP_IN_LOW, EM_SYNC_LOCAL);
                break;
            case AREA_FOREST_HILLS_10:
            case AREA_FOREST_HILLS_9:
            case AREA_JUNGLE_2:
            case AREA_JUNGLE_3:
            case AREA_JUNGLE_4:
            case AREA_JUNGLE_6:
            case AREA_JUNGLE_9:
            case AREA_TOWER_6:
            case AREA_TOWER_7:
            case AREA_DESERT_2:
            case AREA_DESERT_4:
            case AREA_DESERT_7:
            case AREA_SWAMP_4:
            case AREA_SWAMP_5:
            case AREA_SWAMP_8:
            case AREA_SWAMP_9:
            case AREA_VOLCANO_3:
            case AREA_VOLCANO_6:
            case AREA_BATTLE_TOWN_1:
            case AREA_BATTLE_TOWN_2:
            case AREA_BATTLE_TOWN_3:
                hopState = 0;
                switch (Area_DayId(em->area)) {
                case AREA_FOREST_HILLS_10:
                    pointCount = 3;
                    points = EmVelociprey_AppearPtsForestHills10;
                    break;
                case AREA_FOREST_HILLS_9:
                    pointCount = 3;
                    points = EmVelociprey_AppearPtsForestHills9;
                    break;
                case AREA_JUNGLE_2:
                    pointCount = 4;
                    points = EmVelociprey_AppearPtsJungle2;
                    break;
                case AREA_JUNGLE_3:
                    pointCount = 3;
                    points = EmVelociprey_AppearPtsJungle3;
                    break;
                case AREA_JUNGLE_4:
                    pointCount = 4;
                    points = EmVelociprey_AppearPtsJungle4;
                    break;
                case AREA_JUNGLE_6:
                    pointCount = 3;
                    points = EmVelociprey_AppearPtsJungle6;
                    break;
                case AREA_JUNGLE_9:
                    pointCount = 2;
                    points = EmVelociprey_AppearPtsJungle9;
                    break;
                case AREA_TOWER_6:
                    pointCount = 2;
                    points = EmVelociprey_AppearPtsTower6;
                    break;
                case AREA_TOWER_7:
                    pointCount = 2;
                    points = EmVelociprey_AppearPtsTower7;
                    break;
                case AREA_DESERT_2:
                    hopState = 2;
                    pointCount = 3;
                    points = EmVelociprey_AppearPtsDesert2;
                    break;
                case AREA_DESERT_4:
                    if (em->area == AREA_DESERT_NIGHT_4) {
                        pointCount = 1;
                    } else {
                        pointCount = 2;
                    }
                    points = EmVelociprey_AppearPtsDesert4;
                    break;
                case AREA_DESERT_7:
                    pointCount = 1;
                    points = EmVelociprey_AppearPtsDesert7;
                    break;
                case AREA_SWAMP_4:
                case AREA_SWAMP_5:
                    pointCount = 2;
                    points = EmVelociprey_AppearPtsSwamp5;
                    break;
                case AREA_SWAMP_8:
                    pointCount = 1;
                    points = EmVelociprey_AppearPtsSwamp8;
                    break;
                case AREA_SWAMP_9:
                    pointCount = 3;
                    points = EmVelociprey_AppearPtsSwamp9;
                    break;
                case AREA_VOLCANO_3:
                    pointCount = 2;
                    points = EmVelociprey_AppearPtsVolcano3;
                    break;
                case AREA_VOLCANO_6:
                    pointCount = 1;
                    points = EmVelociprey_AppearPtsVolcano6;
                    break;
                case AREA_BATTLE_TOWN_1:
                    hopState = 2;
                    pointCount = 4;
                    points = EmVelociprey_AppearPtsBattleTown1;
                    break;
                case AREA_BATTLE_TOWN_2:
                    pointCount = 2;
                    points = EmVelociprey_AppearPtsBattleTown2;
                    break;
                case AREA_BATTLE_TOWN_3:
                    pointCount = 2;
                    points = EmVelociprey_AppearPtsBattleTown3;
                    break;
                }
                pointIdx = em->stateRand % pointCount;
                Vec_Copy(&em->posX, points[pointIdx].pos);
                Vec_Copy(&em->prevPosX, &em->posX);
                em->rotY = points[pointIdx].angle;
                EmVelociprey_ChangeState(em, EM_GROUP_EVENT2, hopState, EM_SYNC_LOCAL);
                break;
            default:
                if (em->link == NULL) {
                    EmVelociprey_ChangeState(em, EM_GROUP_IDLE, VPREY_IDLE_SPAWN, EM_SYNC_LOCAL);
                } else {
                    em->senseDelay = 5;
                    EmVelociprey_ChangeState(em, EM_GROUP_IDLE, VPREY_IDLE_PACK_WAIT, EM_SYNC_LOCAL);
                }
                break;
            }
            break;
        }
        em->taskPhase++;
        break;
    }
}

/*
 * VPREY_DEFEAT_FADE_RESPAWN: unhittable, fades out over 100 frames, then
 * EmVelociprey_Respawn. Nothing in the file enters it.
 */
void EmVelociprey_FadeAndRespawn(Em *em)
{
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 100;
        break;
    case 1:
        em->timer -= 1;
        em->alpha = (f32) em->timer / 100.0f;
        if (em->timer <= 0) {
            EmVelociprey_Respawn(em);
        }
        break;
    }
}

/*
 * VPREY_DEFEAT_DIE_IN_AIR_BACK, killed in the air: hunted, flung backward
 * with its shadow hidden, fading out over 40 frames (no carcass), then
 * VPREY_DEFEAT_REAPPEAR.
 */
void EmVelociprey_DieInAirBack(Em *em, EmWork *work)
{
    em->hideShadowHold = 2;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        work->huntedB = 1;
        QuestEm_Hunted(em);
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, VPREY_MOT_DIE_IN_AIR, 0, 0);
        em->hideShadow = 1;
        work->accelAngX = 0;
        work->accelAngY = em->rotY;
        work->accelAngZ = 0;
        Player_ClearVelocity(em);
        em->velY = 23.35f;
        em->velZ = -27.0f;
        em->accelY = -2.0f;
        break;
    case 1:
        Em_MoveRotatedAccel(em, &work->accelAngX);
        em->alpha -= 0.025f;
        if (em->alpha <= 0.0f) {
            em->visible = 0;
            em->step++;
            EmVelociprey_ChangeState(em, EM_GROUP_DEFEAT, VPREY_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * VPREY_DEFEAT_DIE_IN_AIR_FORWARD: as EmVelociprey_DieInAirBack, flung
 * forward.
 */
void EmVelociprey_DieInAirForward(Em *em, EmWork *work)
{
    em->hideShadowHold = 2;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        work->huntedB = 1;
        QuestEm_Hunted(em);
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, VPREY_MOT_DIE_IN_AIR, 0, 0);
        em->hideShadow = 1;
        work->accelAngX = 0;
        work->accelAngY = (em->rotY + ANGLE_TURN / 2) & 0xFFFF;
        work->accelAngZ = 0;
        Player_ClearVelocity(em);
        em->velY = 23.35f;
        em->velZ = -27.0f;
        em->accelY = -2.0f;
        break;
    case 1:
        Em_MoveRotatedAccel(em, &work->accelAngX);
        em->alpha -= 0.025f;
        if (em->alpha <= 0.0f) {
            em->visible = 0;
            em->step++;
            EmVelociprey_ChangeState(em, EM_GROUP_DEFEAT, VPREY_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * VPREY_DEFEAT_DIE_IN_AIR_SIDE_A: as EmVelociprey_DieInAirBack, flung
 * sideways.
 */
void EmVelociprey_DieInAirSideA(Em *em, EmWork *work)
{
    em->hideShadowHold = 2;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        work->huntedB = 1;
        QuestEm_Hunted(em);
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, VPREY_MOT_DIE_IN_AIR, 0, 0);
        em->hideShadow = 1;
        work->accelAngX = 0;
        work->accelAngY = (em->rotY - ANGLE_TURN / 4) & 0xFFFF;
        work->accelAngZ = 0;
        Player_ClearVelocity(em);
        em->velY = 23.35f;
        em->velZ = -27.0f;
        em->accelY = -2.0f;
        break;
    case 1:
        Em_MoveRotatedAccel(em, &work->accelAngX);
        em->alpha -= 0.025f;
        if (em->alpha <= 0.0f) {
            em->visible = 0;
            em->step++;
            EmVelociprey_ChangeState(em, EM_GROUP_DEFEAT, VPREY_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/* VPREY_DEFEAT_DIE_IN_AIR_SIDE_B: the mirror of EmVelociprey_DieInAirSideA. */
void EmVelociprey_DieInAirSideB(Em *em, EmWork *work)
{
    em->hideShadowHold = 2;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        work->huntedB = 1;
        QuestEm_Hunted(em);
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, VPREY_MOT_DIE_IN_AIR, 0, 0);
        em->hideShadow = 1;
        work->accelAngX = 0;
        work->accelAngY = (em->rotY + ANGLE_TURN / 4) & 0xFFFF;
        work->accelAngZ = 0;
        Player_ClearVelocity(em);
        em->velY = 23.35f;
        em->velZ = -27.0f;
        em->accelY = -2.0f;
        break;
    case 1:
        Em_MoveRotatedAccel(em, &work->accelAngX);
        em->alpha -= 0.025f;
        if (em->alpha <= 0.0f) {
            em->visible = 0;
            em->step++;
            EmVelociprey_ChangeState(em, EM_GROUP_DEFEAT, VPREY_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * Runs the state of group 5, EM_GROUP_DEFEAT, with the monster unhittable
 * and unpushable throughout.
 */
void EmVelociprey_RunDefeatState(Em *em, EmWork *work)
{
    em->noHitTimer = 10;
    em->noPushTimer = 10;
    switch (em->state) {
    case VPREY_DEFEAT_DIE_BACK:
        EmVelociprey_DieBack(em, work);
        break;
    case VPREY_DEFEAT_DIE_FORWARD:
        EmVelociprey_DieForward(em, work);
        break;
    case VPREY_DEFEAT_DIE_SIDE_A:
        EmVelociprey_DieSideA(em, work);
        break;
    case VPREY_DEFEAT_DIE_SIDE_B:
        EmVelociprey_DieSideB(em, work);
        break;
    case VPREY_DEFEAT_REAPPEAR:
        EmVelociprey_Reappear(em);
        break;
    case VPREY_DEFEAT_FADE_RESPAWN:
        EmVelociprey_FadeAndRespawn(em);
        break;
    case VPREY_DEFEAT_DIE_IN_AIR_BACK:
        EmVelociprey_DieInAirBack(em, work);
        break;
    case VPREY_DEFEAT_DIE_IN_AIR_FORWARD:
        EmVelociprey_DieInAirForward(em, work);
        break;
    case VPREY_DEFEAT_DIE_IN_AIR_SIDE_A:
        EmVelociprey_DieInAirSideA(em, work);
        break;
    case VPREY_DEFEAT_DIE_IN_AIR_SIDE_B:
        EmVelociprey_DieInAirSideB(em, work);
        
    default:
        break;
    }
}

/*
 * VPREY_EVENT_LOOK_AROUND, a scripted scene at a fixed spot: holds its
 * event motion for 450 frames, an idle action, two left turns toward the
 * heading in work->fxMotion (0x666 a frame between frames 24 and 44), the
 * call and the alert motions, then EmVelociprey_EndState. Nothing in the
 * file writes that heading.
 */
void EmVelociprey_EventLookAround(Em *em, EmWork *work)
{
    s32 headingDiff;
    s32 headingDiff2;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        em->posX = 8460.0f;
        em->posY = 0.0f;
        em->posZ = 9367.0f;
        em->goal[0] = 8212.0f;
        em->goal[1] = 0.0f;
        em->goal[2] = 9356.0f;
        em->rotX = 0;
        em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        em->rotZ = 0;
        Em_SetMotion(em, VPREY_MOT_EVENT_IDLE, 0, 0);
        em->timer = 450;
        break;
    case 1:
        if (--em->timer <= 0 && (em->motion[0].playing == 0)) {
            em->step++;
            Em_SetMotion(em, VPREY_MOT_IDLE, 0, 0);
            break;
        }
    default:
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, VPREY_MOT_IDLE_ACTION_A, 0, 0);
        }
        break;
    case 4:
        if (em->motion[0].blend == 0) {
            headingDiff = ((u16) work->fxMotion - (u16) em->rotY) & 0xFFFF;
            if ((EmMotion_ReachedFrame(em, 0, 24.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 44.0f) == 0)) {
                if ((((headingDiff & 0xFFFF) + 0x666) & 0xFFFF) < 0xCCC) {
                    em->rotY = (u16) work->fxMotion;
                } else {
                    em->rotY = (em->rotY - 0x666) & 0xFFFF;
                }
            }
        case 3:
            if (em->motion[0].playing == 0) {
                em->step++;
                Em_SetMotion(em, VPREY_MOT_TURN_LEFT, 0, 0);
                em->goal[0] = 8836.0f;
                em->goal[1] = 0.0f;
                em->goal[2] = 9382.0f;
                break;
            }
        }
        break;
    case 5:
        if (em->motion[0].blend == 0) {
            headingDiff2 = ((u16) work->fxMotion - (u16) em->rotY) & 0xFFFF;
            if ((EmMotion_ReachedFrame(em, 0, 24.0f) != 0) && (EmMotion_ReachedFrame(em, 0, 44.0f) == 0)) {
                if ((((headingDiff2 & 0xFFFF) + 0x666) & 0xFFFF) < 0xCCC) {
                    em->rotY = (u16) work->fxMotion;
                } else {
                    em->rotY = (em->rotY - 0x666) & 0xFFFF;
                }
            }
            if (em->motion[0].playing == 0) {
                em->step++;
                Em_SetMotion(em, VPREY_MOT_CALL, 0, 0);
                break;
            }
        }
        break;
    case 6:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, VPREY_MOT_ALERT, 0, 0);
        }
        break;
    case 7:
        em->step++;
        EmVelociprey_EndState(em, 0, 0);
        break;
    }
}

/*
 * VPREY_EVENT_WALK_TO_GOAL_A, a scripted scene: from a fixed start, without
 * field collision, idles 750 frames, then walks to a fixed goal until
 * within 100 of it or for 300 frames, idles, then EmVelociprey_EndState.
 */
void EmVelociprey_EventWalkToGoalA(Em *em)
{
    f32 goalDist;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        em->posX = 8200.0f;
        em->posY = 0.0f;
        em->posZ = 7300.0f;
        em->goal[0] = 7730.0f;
        em->goal[1] = 0.0f;
        em->goal[2] = 9300.0f;
        em->rotX = 0;
        em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        em->rotZ = 0;
        Em_SetMotion(em, VPREY_MOT_IDLE, 0, 0);
        em->timer = 750;
        em->fieldCollide = 0;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, VPREY_MOT_WALK, 0, 0);
            em->timer = 300;
        }
        break;
    case 2:
        goalDist = Vec_DistanceXZ(&em->posX, em->goal);
        em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        if (--em->timer <= 0 || goalDist <= 100.0f) {
            em->step++;
            Em_SetMotion(em, VPREY_MOT_IDLE, 0, 0);
        }
        break;
    case 3:
        em->step++;
        EmVelociprey_EndState(em, 0, 0);
        em->fieldCollide = 1;
        break;
    }
}

/*
 * VPREY_EVENT_WALK_TO_GOAL_B: as EmVelociprey_EventWalkToGoalA from another
 * start, with an idle action on arrival.
 */
void EmVelociprey_EventWalkToGoalB(Em *em)
{
    f32 goalDist;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        em->posX = 8340.0f;
        em->posY = 300.0f;
        em->posZ = 12470.0f;
        em->goal[0] = 8000.0f;
        em->goal[1] = 300.0f;
        em->goal[2] = 10850.0f;
        em->rotX = 0;
        em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        em->rotZ = 0;
        Em_SetMotion(em, VPREY_MOT_IDLE, 0, 0);
        em->timer = 750;
        em->fieldCollide = 0;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, VPREY_MOT_WALK, 0, 0);
            em->timer = 300;
        }
        break;
    case 2:
        goalDist = Vec_DistanceXZ(&em->posX, em->goal);
        em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        if (--em->timer <= 0 || goalDist <= 100.0f) {
            em->step++;
            Em_SetMotion(em, VPREY_MOT_IDLE_ACTION_A, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, VPREY_MOT_IDLE, 0, 0);
            em->timer = 300;
        }
        break;
    case 4:
        em->step++;
        em->fieldCollide = 1;
        EmVelociprey_EndState(em, 0, 0);
        break;
    }
}

/*
 * VPREY_EVENT_BY_VARIANT: one of the three scenes above, by Em.variant (0,
 * 1, 2).
 */
void EmVelociprey_EventByVariant(Em *em)
{
    switch (em->variant) {
    case 0:
        EmVelociprey_ChangeState(em, EM_GROUP_EVENT, VPREY_EVENT_LOOK_AROUND, EM_SYNC_LOCAL);
        break;
    case 1:
        EmVelociprey_ChangeState(em, EM_GROUP_EVENT, VPREY_EVENT_WALK_TO_GOAL_A, EM_SYNC_LOCAL);
        break;
    case 2:
        EmVelociprey_ChangeState(em, EM_GROUP_EVENT, VPREY_EVENT_WALK_TO_GOAL_B, EM_SYNC_LOCAL);
        break;
    }
}

/*
 * VPREY_EVENT_HIDE_A: hidden for one step (the script not asked), then
 * shown and EmVelociprey_EndState.
 */
void EmVelociprey_EventHideA(Em *em)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, VPREY_MOT_IDLE, 0, 0);
        em->visible = 0;
        em->scriptPending = 0;
        break;
    case 1:
        em->step++;
        em->visible = 1;
        EmVelociprey_EndState(em, 0, 0);
        break;
    }
}

/* VPREY_EVENT_HIDE_B: the same as EmVelociprey_EventHideA. */
void EmVelociprey_EventHideB(Em *em)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, VPREY_MOT_IDLE, 0, 0);
        em->visible = 0;
        em->scriptPending = 0;
        break;
    case 1:
        em->step++;
        em->visible = 1;
        EmVelociprey_EndState(em, 0, 0);
        break;
    }
}

/* VPREY_EVENT_HIDE_C: the same as EmVelociprey_EventHideA. */
void EmVelociprey_EventHideC(Em *em)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, VPREY_MOT_IDLE, 0, 0);
        em->visible = 0;
        em->scriptPending = 0;
        break;
    case 1:
        em->step++;
        em->visible = 1;
        EmVelociprey_EndState(em, 0, 0);
        break;
    }
}

/* Runs the state of group 6, EM_GROUP_EVENT. */
void EmVelociprey_RunEventState(Em *em, EmWork *work)
{
    switch (em->state) {
    case VPREY_EVENT_LOOK_AROUND:
        EmVelociprey_EventLookAround(em, work);
        break;
    case VPREY_EVENT_WALK_TO_GOAL_A:
        EmVelociprey_EventWalkToGoalA(em);
        break;
    case VPREY_EVENT_WALK_TO_GOAL_B:
        EmVelociprey_EventWalkToGoalB(em);
        break;
    case VPREY_EVENT_BY_VARIANT:
        EmVelociprey_EventByVariant(em);
        break;
    case VPREY_EVENT_HIDE_A:
        EmVelociprey_EventHideA(em);
        break;
    case VPREY_EVENT_HIDE_B:
        EmVelociprey_EventHideB(em);
        break;
    case VPREY_EVENT_HIDE_C:
        EmVelociprey_EventHideC(em);
        
    default:
        break;
    }
}

/*
 * VPREY_EVENT2_HOP_IN: hops into the area after a reappearance (up 70,
 * forward 50, airborne from frame 10), unaware until it lands, then
 * EmVelociprey_EndState.
 */
void EmVelociprey_HopIn(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, VPREY_MOT_HOP, 0, 0);
        Player_ClearVelocity(em);
        em->velY = 70.0f;
        em->velZ = 50.0f;
        em->accelY = -4.0f;
        em->accelZ = -1.0f;
        work->accelAngX = 0;
        work->accelAngZ = 0;
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 10.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            work->accelAngY = em->rotY;
            Em_MoveRotatedAccel(em, &work->accelAngX);
        }
        if (em->velY < -50.0f) {
            em->velY = -50.0f;
            em->accelY = 0.0f;
        }
        if (em->velZ < 20.0f) {
            em->velZ = 20.0f;
            em->accelZ = 0.0f;
        }
        if ((em->velY < 0.0f) && (em->posY <= em->floorY)) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            Em_SetMotion(em, VPREY_MOT_HOP_LAND, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->senseDelay = 0;
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/* VPREY_EVENT2_HOP_IN_LOW: as EmVelociprey_HopIn, lower (up 50, forward 30). */
void EmVelociprey_HopInLow(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, VPREY_MOT_HOP, 0, 0);
        Player_ClearVelocity(em);
        em->velY = 50.0f;
        em->velZ = 30.0f;
        em->accelY = -4.0f;
        em->accelZ = -1.0f;
        work->accelAngX = 0;
        work->accelAngZ = 0;
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 10.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            work->accelAngY = em->rotY;
            Em_MoveRotatedAccel(em, &work->accelAngX);
        }
        if (em->velY < -50.0f) {
            em->velY = -50.0f;
            em->accelY = 0.0f;
        }
        if (em->velZ < 10.0f) {
            em->velZ = 10.0f;
            em->accelZ = 0.0f;
        }
        if ((em->velY < 0.0f) && (em->posY <= em->floorY)) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            Em_SetMotion(em, VPREY_MOT_HOP_LAND, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->senseDelay = 0;
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * VPREY_EVENT2_HOP_IN_HIGH: as EmVelociprey_HopIn, higher (up 80, forward
 * 50).
 */
void EmVelociprey_HopInHigh(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, VPREY_MOT_HOP, 0, 0);
        Player_ClearVelocity(em);
        em->velY = 80.0f;
        em->velZ = 50.0f;
        em->accelY = -4.0f;
        em->accelZ = -0.5f;
        work->accelAngX = 0;
        work->accelAngZ = 0;
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 10.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            work->accelAngY = em->rotY;
            Em_MoveRotatedAccel(em, &work->accelAngX);
        }
        if (em->velY < -50.0f) {
            em->velY = -50.0f;
            em->accelY = 0.0f;
        }
        if (em->velZ < 30.0f) {
            em->velZ = 30.0f;
            em->accelZ = 0.0f;
        }
        if ((em->velY < 0.0f) && (em->posY <= em->floorY)) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            Em_SetMotion(em, VPREY_MOT_HOP_LAND, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->senseDelay = 0;
            EmVelociprey_EndState(em, 0, 0);
        }
        break;
    }
}

/* Runs the state of group 7, EM_GROUP_EVENT2: the hops in. */
void EmVelociprey_RunEvent2State(Em *em, EmWork *work)
{
    switch (em->state) {
    case VPREY_EVENT2_HOP_IN:
        EmVelociprey_HopIn(em, work);
        break;
    case VPREY_EVENT2_HOP_IN_LOW:
        EmVelociprey_HopInLow(em, work);
        break;
    case VPREY_EVENT2_HOP_IN_HIGH:
        EmVelociprey_HopInHigh(em, work);
        break;
    }
}

/*
 * The family's update, run every frame by GmEm_Update: the motion speed;
 * notices a pack call (EM_TRIGGER_SENSED, unless it is still waiting to
 * sense or in combat); then Em_UpdateDamage's result picks the reaction: by
 * the hit's side a knock-back (or, killed, a death: in the air with the
 * part flag, the airborne one), status 2 and 4 unless knocked back or
 * getting up, a flinch on the ground. Then runs the state (twice when one
 * asked for it).
 */
void EmVelociprey_Update(Em *em)
{
    EmVelocipreyWork *w = (EmVelocipreyWork *)&em->work;
    u8 hitInfo[4];
    s32 sensed;
    u32 result;

    em->motionSpeed = 1.2f;
    if (em->senseDelay != 0) {
        em->noHitTimer = 5;
    }
    sensed = Em_SenseObjects(em, &w->callObj);
    if (em->senseDelay == 0 && em->combat == 0 && sensed != 0 && Em_ScriptFlagsFrom(em, 4) == 0) {
        em->triggers |= EM_TRIGGER_SENSED;
    }
    Em_UpdateCombatTimer(em);
    result = Em_UpdateDamage(em, hitInfo) & 0xFF;
    switch (result) {
    case EM_DMG_NONE:
    case EM_DMG_ATTACK9:
    case EM_DMG_ATTACK6:
    case EM_DMG_CAPTURED:
    case EM_DMG_FLASH:
    case EM_DMG_PART8_DOWN:
    case EM_DMG_HIT_MODE3:
    case EM_DMG_ATTACK5:
    case EM_DMG_ATTACK5_FLINCH:
    case EM_DMG_ATTACK13:
        break;
    case EM_DMG_KILLED:
    case EM_DMG_DEAD:
        if (em->stance == EM_STANCE_AIR && result == EM_DMG_KILLED && (em->partFlags[0] & 2)) {
            u16 angle = em->hitAngle;
            s32 relAngle = (angle - em->rotY) & 0xFFFF;
            if (relAngle >= (ANGLE_TURN * 3 / 8) && relAngle < (ANGLE_TURN * 5 / 8)) {
                em->rotY = angle + ANGLE_TURN / 2;
                EmVelociprey_ChangeState(em, EM_GROUP_DEFEAT, VPREY_DEFEAT_DIE_IN_AIR_BACK, EM_SYNC_OWNER);
            } else if (relAngle >= (ANGLE_TURN / 8) && relAngle < (ANGLE_TURN * 3 / 8)) {
                em->rotY = angle - ANGLE_TURN / 4;
                EmVelociprey_ChangeState(em, EM_GROUP_DEFEAT, VPREY_DEFEAT_DIE_IN_AIR_SIDE_A, EM_SYNC_OWNER);
            } else if (relAngle >= (ANGLE_TURN * 5 / 8) && relAngle < (ANGLE_TURN * 7 / 8)) {
                em->rotY = angle + ANGLE_TURN / 4;
                EmVelociprey_ChangeState(em, EM_GROUP_DEFEAT, VPREY_DEFEAT_DIE_IN_AIR_SIDE_B, EM_SYNC_OWNER);
            } else {
                em->rotY = angle & 0xFFFF;
                EmVelociprey_ChangeState(em, EM_GROUP_DEFEAT, VPREY_DEFEAT_DIE_IN_AIR_FORWARD, EM_SYNC_OWNER);
            }
        } else {
            u16 angle = em->hitAngle;
            s32 relAngle = (angle - em->rotY) & 0xFFFF;
            if (relAngle >= (ANGLE_TURN * 3 / 8) && relAngle < (ANGLE_TURN * 5 / 8)) {
                em->rotY = angle + ANGLE_TURN / 2;
                EmVelociprey_ChangeState(em, EM_GROUP_DEFEAT, VPREY_DEFEAT_DIE_BACK, EM_SYNC_OWNER);
            } else if (relAngle >= (ANGLE_TURN / 8) && relAngle < (ANGLE_TURN * 3 / 8)) {
                em->rotY = angle - ANGLE_TURN / 4;
                EmVelociprey_ChangeState(em, EM_GROUP_DEFEAT, VPREY_DEFEAT_DIE_SIDE_A, EM_SYNC_OWNER);
            } else if (relAngle >= (ANGLE_TURN * 5 / 8) && relAngle < (ANGLE_TURN * 7 / 8)) {
                em->rotY = angle + ANGLE_TURN / 4;
                EmVelociprey_ChangeState(em, EM_GROUP_DEFEAT, VPREY_DEFEAT_DIE_SIDE_B, EM_SYNC_OWNER);
            } else {
                em->rotY = angle & 0xFFFF;
                EmVelociprey_ChangeState(em, EM_GROUP_DEFEAT, VPREY_DEFEAT_DIE_FORWARD, EM_SYNC_OWNER);
            }
        }
        break;
    case EM_DMG_STATUS_IN_AIR:
        if (em->stateGroup != EM_GROUP_REACT || em->state != VPREY_REACT_KNOCKED_BACK) {
            EmVelociprey_ChangeState(em, EM_GROUP_REACT, VPREY_REACT_KNOCKED_BACK, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_STATUS4:
        if ((em->stateGroup != EM_GROUP_REACT || em->state != VPREY_REACT_KNOCKED_BACK) && (em->stateGroup != EM_GROUP_IDLE || em->state != VPREY_IDLE_GET_UP)) {
            Em_SetStatus4Timer(em);
            EmVelociprey_ChangeState(em, EM_GROUP_REACT, VPREY_REACT_STATUS4, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_STATUS2:
        if ((em->stateGroup != EM_GROUP_REACT || em->state != VPREY_REACT_KNOCKED_BACK) && (em->stateGroup != EM_GROUP_IDLE || em->state != VPREY_IDLE_GET_UP)) {
            Em_SetStatus2Timer(em);
            EmVelociprey_ChangeState(em, EM_GROUP_IDLE, VPREY_IDLE_STATUS2, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_HIT_LYING:
        EmVelociprey_ChangeState(em, EM_GROUP_IDLE, VPREY_IDLE_STATUS2_END, EM_SYNC_OWNER);
        break;
    case EM_DMG_PART_FLINCH: {
        u16 angle = em->hitAngle;
        s32 relAngle = (angle - em->rotY) & 0xFFFF;
        if (relAngle >= (ANGLE_TURN * 3 / 8) && relAngle < (ANGLE_TURN * 5 / 8)) {
            em->rotY = angle + ANGLE_TURN / 2;
            EmVelociprey_ChangeState(em, EM_GROUP_REACT, VPREY_REACT_KNOCKED_BACK, EM_SYNC_OWNER);
        } else if (relAngle >= (ANGLE_TURN / 8) && relAngle < (ANGLE_TURN * 3 / 8)) {
            em->rotY = angle - ANGLE_TURN / 4;
            EmVelociprey_ChangeState(em, EM_GROUP_REACT, VPREY_REACT_KNOCKED_SIDE_A, EM_SYNC_OWNER);
        } else if (relAngle >= (ANGLE_TURN * 5 / 8) && relAngle < (ANGLE_TURN * 7 / 8)) {
            em->rotY = angle + ANGLE_TURN / 4;
            EmVelociprey_ChangeState(em, EM_GROUP_REACT, VPREY_REACT_KNOCKED_SIDE_B, EM_SYNC_OWNER);
        } else {
            em->rotY = angle & 0xFFFF;
            EmVelociprey_ChangeState(em, EM_GROUP_REACT, VPREY_REACT_KNOCKED_FORWARD, EM_SYNC_OWNER);
        }
        break;
    }
    case EM_DMG_HIT:
        em->scriptPending = 0;
        if (em->stance == EM_STANCE_AIR) {
            if (em->hitAngle >= (ANGLE_TURN * 3 / 8) && em->hitAngle < (ANGLE_TURN * 5 / 8)) {
                em->rotY = em->hitAngle + ANGLE_TURN / 2;
                EmVelociprey_ChangeState(em, EM_GROUP_REACT, VPREY_REACT_KNOCKED_BACK, EM_SYNC_OWNER);
            } else if (em->hitAngle >= (ANGLE_TURN / 8) && em->hitAngle < (ANGLE_TURN * 3 / 8)) {
                em->rotY = em->hitAngle - ANGLE_TURN / 4;
                EmVelociprey_ChangeState(em, EM_GROUP_REACT, VPREY_REACT_KNOCKED_SIDE_A, EM_SYNC_OWNER);
            } else if (em->hitAngle >= (ANGLE_TURN * 5 / 8) && em->hitAngle < (ANGLE_TURN * 7 / 8)) {
                em->rotY = em->hitAngle + ANGLE_TURN / 4;
                EmVelociprey_ChangeState(em, EM_GROUP_REACT, VPREY_REACT_KNOCKED_SIDE_B, EM_SYNC_OWNER);
            } else {
                em->rotY = em->hitAngle & 0xFFFF;
                EmVelociprey_ChangeState(em, EM_GROUP_REACT, VPREY_REACT_KNOCKED_FORWARD, EM_SYNC_OWNER);
            }
        } else {
            EmVelociprey_ChangeState(em, EM_GROUP_REACT, VPREY_REACT_FLINCH, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_STATUS5:
        if ((em->stateGroup != EM_GROUP_REACT || em->state != VPREY_REACT_STUN) && (em->stateGroup != EM_GROUP_REACT || em->state != VPREY_REACT_KNOCKED_BACK)) {
            Em_SetStatus5Timer(em);
            EmVelociprey_ChangeState(em, EM_GROUP_REACT, VPREY_REACT_STUN, EM_SYNC_OWNER);
        }
        break;
    }
    Em_UpdatePendingScript(em);
    Em_ClearHitReactMode(em);
    EmVelociprey_RunState(em, (s32) w);
    if (em->stateRequest != 0) {
        EmVelociprey_RunState(em, (s32) w);
        em->stateRequest = 0;
    }
}

/* Runs the monster's state through its group's dispatcher. */
void EmVelociprey_RunState(Em *em, s32 work)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->stateGroup) {
    case EM_GROUP_IDLE:
        EmVelociprey_RunIdleState(em, (EmWork *) work);
        break;
    case EM_GROUP_MOVE:
        EmVelociprey_RunMoveState(em, (EmWork *) work);
        break;
    case EM_GROUP_FLY:
        EmVelociprey_RunHopState(em, (EmWork *) work);
        break;
    case EM_GROUP_ATTACK:
        EmVelociprey_RunAttackState(em, (EmWork *) work);
        break;
    case EM_GROUP_REACT:
        EmVelociprey_RunReactState(em, (EmWork *) work);
        break;
    case EM_GROUP_DEFEAT:
        EmVelociprey_RunDefeatState(em, (EmWork *) work);
        break;
    case EM_GROUP_EVENT:
        EmVelociprey_RunEventState(em, (EmWork *) work);
        break;
    case EM_GROUP_EVENT2:
        EmVelociprey_RunEvent2State(em, (EmWork *) work);
        
    default:
        break;
    }
}

/* The motion effects of the motions without any: nothing. */
void EmVelociprey_MotionEffectsDefault(Em *em)
{
}

/*
 * Every frame from the second on: keeps the layer-0 motion in
 * work->prevMotion, plays its sounds (EmVelociprey_MotionSounds), and its
 * effects at their frames: dust for the hops and moves, the dromes' pounce
 * hit (VelocipreyAttack row 2) and its dust, and a periodic effect at the
 * head while lying.
 */
void EmVelociprey_MotionEffects(Em *em, EmWork *work)
{
    f32 pos[3];

    if (em->motionId[0] != work->prevMotion) {
        work->prevMotion = em->motionId[0];
    }
    EmVelociprey_MotionSounds((s32) em, work);
    switch (work->prevMotion) {
    case EM_MOTION_ID(0, VPREY_MOT_HOP):
        if (EmMotion_PassedFrameScaled(em, 18.0f, 0U) != 0) {
            PartFx_SpawnForEm(em, DROME_BONE_HOP_DUST, 0);
        }
        break;
    case EM_MOTION_ID(0, VPREY_MOT_HOP_UP):
        if (EmMotion_PassedFrameScaled(em, 20.0f, 0U) != 0) {
            PartFx_SpawnForEm(em, DROME_BONE_HOP_DUST, 0);
        }
        break;
    case EM_MOTION_ID(0, VPREY_MOT_MOVE_ACTION_A):
        if (EmMotion_PassedFrameScaled(em, 12.0f, 0U) != 0) {
            PartFx_SpawnForEm(em, DROME_BONE_HOP_DUST, 0);
        }
        break;
    case EM_MOTION_ID(0, VPREY_MOT_MOVE_ACTION_B):
    case EM_MOTION_ID(0, VPREY_MOT_MOVE_ACTION_C):
        if (EmMotion_PassedFrameScaled(em, 14.0f, 0U) != 0) {
            PartFx_SpawnForEm(em, DROME_BONE_HOP_DUST, 0);
        }
        break;
    case EM_MOTION_ID(0, VPREY_MOT_POUNCE):
        if (EmMotion_PassedFrameScaled(em, 32.0f, 0U) != 0) {
            VelocipreyAttack_Create(em, 2);
        }
        if (EmMotion_PassedFrameScaled(em, 22.0f, 0U) != 0) {
            PartFx_SpawnForEm(em, DROME_BONE_POUNCE_DUST, 0);
        }
        break;
    case EM_MOTION_ID(0, VPREY_MOT_LIE):
        pos[0] = 0.0f;
        pos[1] = 10.0f;
        pos[2] = 40.0f;
        Em_SpawnLyingPuffs(em, DROME_BONE_MOUTH, pos, 1.0f);
        break;
    default:
        EmVelociprey_MotionEffectsDefault(em);
        break;
    case EM_MOTION_ID(0, VPREY_MOT_IDLE):
    case EM_MOTION_ID(0, VPREY_MOT_WALK):
    case EM_MOTION_ID(0, VPREY_MOT_HOP_LAND):
    case EM_MOTION_ID(0, VPREY_MOT_TURN_LEFT):
    case EM_MOTION_ID(0, VPREY_MOT_TURN_RIGHT):
    case EM_MOTION_ID(0, VPREY_MOT_ALERT):
    case EM_MOTION_ID(0, VPREY_MOT_IDLE_B):
    case EM_MOTION_ID(0, VPREY_MOT_LEAP):
    case EM_MOTION_ID(0, VPREY_MOT_LEAP_LAND):
    case EM_MOTION_ID(0, VPREY_MOT_STRIKE):
    case EM_MOTION_ID(0, VPREY_MOT_POUNCE_LAND):
    case EM_MOTION_ID(0, VPREY_MOT_QUICK_STRIKE):
    case EM_MOTION_ID(0, VPREY_MOT_SHOOT):
    case EM_MOTION_ID(0, VPREY_MOT_FLINCH):
    case EM_MOTION_ID(0, VPREY_MOT_KNOCKED_BACK):
    case EM_MOTION_ID(0, VPREY_MOT_KNOCKED_BACK_LAND):
    case EM_MOTION_ID(0, VPREY_MOT_KNOCKED_FORWARD):
    case EM_MOTION_ID(0, VPREY_MOT_KNOCKED_FORWARD_LAND):
    case EM_MOTION_ID(0, VPREY_MOT_KNOCKED_SIDE_B):
    case EM_MOTION_ID(0, VPREY_MOT_KNOCKED_SIDE_B_LAND):
    case EM_MOTION_ID(0, VPREY_MOT_KNOCKED_SIDE_A):
    case EM_MOTION_ID(0, VPREY_MOT_KNOCKED_SIDE_A_LAND):
    case EM_MOTION_ID(0, VPREY_MOT_GET_UP):
    case EM_MOTION_ID(0, VPREY_MOT_DAZED):
    case EM_MOTION_ID(0, VPREY_MOT_COLLAPSE):
    case EM_MOTION_ID(0, VPREY_MOT_DIE):
    case EM_MOTION_ID(0, VPREY_MOT_STATUS4):
    case EM_MOTION_ID(0, VPREY_MOT_IDLE_ACTION_A):
    case EM_MOTION_ID(0, VPREY_MOT_CALL):
    case EM_MOTION_ID(0, VPREY_MOT_SHAKE_OFF):
    case EM_MOTION_ID(0, VPREY_MOT_EVENT_IDLE):
        break;
    }
}

/*
 * Callback slot 3 (EmVt hook 3, every frame): the first frame only advances
 * fxStep, later frames run EmVelociprey_MotionEffects.
 */
void EmVelociprey_UpdateMotionEffects(Em *em)
{
    EmVelocipreyWork *w = (EmVelocipreyWork *)&em->work;

    switch (w->fxStep) {
    case 0:
        w->fxStep++;
        break;
    case 1:
        EmVelociprey_MotionEffects(em, (EmWork *) w);
        break;
    }
}

/*
 * Callback slot 4: the script's question about the monster. Query 1: is a
 * Rathian (EM_KIND_RATHIAN) on the ground within Em.nearRadius. Returns 1
 * or 0.
 */
s32 EmVelociprey_ScriptQuery(Em *em, u8 query)
{
    switch (query) {
    case 1:
        if (EmList_FindKindNear(em, EM_KIND_RATHIAN, 1, em->nearRadius) == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

/* Callback slot 2: nothing. */
void EmVelociprey_TableNop(void)
{
}

/*
 * Enters a state of group 0, EM_GROUP_IDLE, on the ground. In combat,
 * VPREY_IDLE becomes the alert idle while hp is at least 30 percent, and so
 * does VPREY_IDLE_WAIT on half the frames (Em.stateRand even) when the goal
 * is a player beyond Em.nearRadius. VPREY_IDLE_STUN_END ends status 5.
 */
void EmVelociprey_SetIdleState(Em *em, u16 state, u8 mode)
{
    switch (state) {
    case VPREY_IDLE:
        if (em->combat == 1 && em->hp >= (s16) (0.3f * em->hpMax)) {
            state = VPREY_IDLE_ALERT;
        }
        break;
    case VPREY_IDLE_WAIT:
        if (em->combat == 1 && em->curGoalKind == 1 && em->curGoalArg == 0 && em->curGoalNo != -1 &&
            em->stateRand % 2 == 0 && !(((f32 *) em->playerDist)[em->curGoalNo] <= em->nearRadius)) {
            state = VPREY_IDLE_ALERT;
        }
        break;
    case VPREY_IDLE_STUN_END:
        Em_EndStatus5(em);
        break;
    }
    em->stance = EM_STANCE_GROUND;
    Em_SetStateSync(em, EM_GROUP_IDLE, state, mode);
}

/*
 * Enters a state of group 1, EM_GROUP_MOVE, on the ground: VPREY_MOVE_TURN
 * becomes VPREY_MOVE_TURN_ALERT in combat; VPREY_MOVE_WALK sets the walking
 * speed (Em_SetSpeedScaled 1000, 300 short of a player).
 */
void EmVelociprey_SetMoveState(Em *em, u16 state, u8 mode)
{
    em->stance = EM_STANCE_GROUND;
    switch (state) {
    case VPREY_MOVE_TURN:
        if (em->combat == 1) {
            state = VPREY_MOVE_TURN_ALERT;
        }
        break;
    case VPREY_MOVE_WALK:
        Em_SetSpeedScaled(em, 2, 1000.0f, 300.0f);
        break;
    }
    Em_SetStateSync(em, EM_GROUP_MOVE, state, mode);
}

/* Enters a state of group 2, the hops. */
void EmVelociprey_SetHopState(Em *em, u16 state, u8 mode)
{
    Em_SetStateSync(em, EM_GROUP_FLY, state, mode);
}

/*
 * Enters a state of group 3, EM_GROUP_ATTACK: the melee attacks and leaps
 * aim at the goal (Em.hasGoal) when it is a player and there is a target;
 * the shots don't.
 */
void EmVelociprey_SetAttackState(Em *em, u16 state, u8 mode)
{
    switch (state) {
    case VPREY_ATK_STRIKE_A:
    case VPREY_ATK_LEAP_MEDIUM_A:
    case VPREY_ATK_LEAP_LONG_A:
    case VPREY_ATK_LEAP_HIGH_A:
    case VPREY_ATK_STRIKE_B:
    case VPREY_ATK_LEAP_MEDIUM_B:
    case VPREY_ATK_QUICK_STRIKE_B:
    case VPREY_ATK_LEAP_LONG_B:
    case VPREY_ATK_LEAP_HIGH_B:
    case VPREY_ATK_STRIKE_C:
    case VPREY_ATK_QUICK_STRIKE_A:
    case VPREY_ATK_NONE:
        em->hasGoal = 0;
        if ((em->curGoalKind == 1) && (em->targetNo != -1)) {
            em->hasGoal = 1;
        }
        break;
    }
    Em_SetStateSync(em, EM_GROUP_ATTACK, state, mode);
}

/*
 * The family's state change (Em_ChangeState dispatches here by kind):
 * removes the pack call's object if it is out, then enters the state
 * through the group's Set*State, or directly: VPREY_DEFEAT_REAPPEAR first
 * lets the quest place it (QuestEm_Appear), VPREY_REACT_STUN begins status
 * 5.
 */
void EmVelociprey_ChangeState(Em *em, u16 group, u16 state, u8 mode)
{
    EmVelocipreyWork *w = (EmVelocipreyWork *)&em->work;

    if (w->callActive != 0) {
        EmSenseObjs_Remove(&w->callObj);
        w->callActive = 0;
    }
    switch (group) {
    case EM_GROUP_IDLE:
        EmVelociprey_SetIdleState(em, state, mode);
        break;
    case EM_GROUP_MOVE:
        EmVelociprey_SetMoveState(em, state, mode);
        break;
    case EM_GROUP_FLY:
        EmVelociprey_SetHopState(em, state, mode);
        break;
    case EM_GROUP_ATTACK:
        EmVelociprey_SetAttackState(em, state, mode);
        break;
    case EM_GROUP_EVENT:
        Em_SetStateSync(em, group, state, mode);
        break;
    case EM_GROUP_DEFEAT:
        if (state == 4 && QuestEm_CanAppear(em) == 1) {
            QuestEm_Appear(em);
        }
        Em_SetStateSync(em, group, state, mode);
        break;
    case EM_GROUP_REACT:
        switch (state) {
        case VPREY_REACT_STUN:
            Em_BeginStatus5(em);
            break;
        }
        Em_SetStateSync(em, group, state, mode);
        break;
    case EM_GROUP_EVENT2:
        Em_SetStateSync(em, group, state, mode);
        break;
    }
}

/*
 * Starts the hop curves of curveKind (EmVelocipreyCurveKind) from frame 22,
 * with the velocity cleared.
 */
void EmVelociprey_StartCurves(Em *em, u8 kind)
{
    EmVelocipreyWork *w = (EmVelocipreyWork *)&em->work;

    w->curveNone = 0;
    w->curveVelY = 0;
    w->curveSpeed = 0;
    w->curveKind = kind;
    switch (w->curveKind) {
    case 0:
        w->curveVelY = 1;
        w->curveSpeed = 1;
        w->curveFrame = 22;
        break;
    case 1:
        w->curveVelY = 1;
        w->curveFrame = 22;
        break;
    case VPREY_CURVE_HOP_FORWARD:
        w->curveVelY = 1;
        w->curveSpeed = 1;
        w->curveFrame = 22;
        break;
    case VPREY_CURVE_HOP_UP:
        w->curveVelY = 1;
        w->curveFrame = 22;
        break;
    }
    Player_ClearVelocity(em);
}

/* A curve that does nothing; no kind runs it. */
static s32 EmVelociprey_CurveNone(Em *em, EmWork *work)
{
    return 0;
}

extern EmCurveKey *EmVelociprey_VelYCurves[];

/*
 * The vertical curve: sets Em.velY from the keys of the curve kind's table
 * at the curve frame (the first key's value at frames 0 and 1). Past the
 * table's end it returns 2 and falls 1 faster a frame, to -50.
 */
static u8 EmVelociprey_CurveVelY(Em *em, EmWork *work)
{
    EmVelocipreyWork *w = (EmVelocipreyWork *) work;
    EmCurveKey *key;
    f32 frame;
    f32 keyTime;
    s32 keyIdx;
    EmCurveKey *keys;
    u8 endBits;

    keys = EmVelociprey_VelYCurves[((EmVelocipreyWork *)work)->curveKind];
    endBits = 0;
    if (keys == NULL) {
        return endBits;
    }
    frame = w->curveFrame;
    if (frame == 0.0f || frame == 1.0f) {
        em->velY = keys->v;
    } else {
        keyIdx = 1;
        while (keyIdx != 0) {
            key = &keys[keyIdx];
            keyTime = key->t;
            if (frame > keyTime && keyTime != 0.0f) {
                keyIdx++;
            } else if (keyTime == 0.0f) {
                endBits = 2;
                if (em->velY > -50.0f) {
                    em->velY -= 1.0f;
                }
                break;
            } else {
                keyIdx = 0;
                em->velY = (key->v - key[-1].v) / ((keyTime - key[-1].t) / em->motion[0].step);
            }
        }
    }
    return endBits;
}

extern EmCurveKey *EmVelociprey_SpeedCurves[];

/*
 * The forward curve: sets Em.velZ from the curve kind's table as
 * EmVelociprey_CurveVelY does; returns 4 past the table's end. A wall in
 * front (Em.wallHitDirs 0xF000000F) stops it.
 */
static u8 EmVelociprey_CurveSpeed(Em *em, EmWork *work)
{
    EmVelocipreyWork *w = (EmVelocipreyWork *) work;
    EmCurveKey *key;
    f32 frame;
    f32 keyTime;
    s32 keyIdx;
    EmCurveKey *keys;
    u8 endBits;

    keys = EmVelociprey_SpeedCurves[((EmVelocipreyWork *)work)->curveKind];
    endBits = 0;
    if (keys == NULL) {
        return endBits;
    }
    frame = w->curveFrame;
    if (frame == 0.0f || frame == 1.0f) {
        em->velZ = keys->v;
    } else {
        keyIdx = 1;
        while (keyIdx != 0) {
            key = &keys[keyIdx];
            keyTime = key->t;
            if (frame > keyTime && keyTime != 0.0f) {
                keyIdx++;
            } else if (keyTime == 0.0f) {
                endBits = 4;
                break;
            } else {
                keyIdx = 0;
                em->velZ = (key->v - key[-1].v) / ((keyTime - key[-1].t) / em->motion[0].step);
            }
        }
    }
    if (em->wallHitDirs & 0xF000000F) {
        *(s32 *) &em->velZ = 0;
    }
    return endBits;
}

/*
 * Every frame of a curve hop: runs the curves of EmVelociprey_StartCurves'
 * kind, advances the curve frame, and moves the monster while it is in the
 * air. Returns their end bits (2 the vertical, 4 the forward curve),
 * nonzero once one ran past its table.
 */
u8 EmVelociprey_UpdateCurves(Em *em)
{
    EmVelocipreyWork *w = (EmVelocipreyWork *)&em->work;
    u8 endBits = 0;

    if (w->curveNone != 0) {
        endBits = EmVelociprey_CurveNone(em, (EmWork *) w);
    }
    if (w->curveVelY != 0) {
        endBits |= EmVelociprey_CurveVelY(em, (EmWork *) w);
    }
    if (w->curveSpeed != 0) {
        endBits |= EmVelociprey_CurveSpeed(em, (EmWork *) w);
    }
    w->curveFrame += (s16) em->motion[0].step;
    if (em->stance == EM_STANCE_AIR) {
        w->accelAngX = 0;
        w->accelAngY = em->rotY;
        w->accelAngZ = 0;
        Em_MoveRotatedAccel(em, &w->accelAngX);
    }
    return endBits;
}

/* The file's initialised data, in the game's order */
EmVelocipreyAppearPoint EmVelociprey_AppearPtsFortress6[4] = {
#include "game/008C8470.inc"
};
EmVelocipreyAppearAt EmVelociprey_AppearPtsForestHills10[3] = {
#include "game/008C84D0.inc"
};
EmVelocipreyAppearAt EmVelociprey_AppearPtsForestHills9[3] = {
#include "game/008C8500.inc"
};
EmVelocipreyAppearPoint EmVelociprey_AppearPtsForestHills2[5] = {
#include "game/008C8530.inc"
};
EmVelocipreyAppearPoint EmVelociprey_AppearPtsForestHills1[4] = {
#include "game/008C85B0.inc"
};
EmVelocipreyAppearPoint EmVelociprey_AppearPtSnowyMountains3 = {
#include "game/008C8610.inc"
};
EmVelocipreyAppearAt EmVelociprey_AppearPtsJungle2[4] = {
#include "game/008C8630.inc"
};
EmVelocipreyAppearAt EmVelociprey_AppearPtsJungle3[3] = {
#include "game/008C8670.inc"
};
EmVelocipreyAppearAt EmVelociprey_AppearPtsJungle4[4] = {
#include "game/008C86A0.inc"
};
EmVelocipreyAppearAt EmVelociprey_AppearPtsJungle6[3] = {
#include "game/008C86E0.inc"
};
EmVelocipreyAppearAt EmVelociprey_AppearPtsJungle9[2] = {
#include "game/008C8710.inc"
};
EmVelocipreyAppearAt EmVelociprey_AppearPtsTower6[2] = {
#include "game/008C8730.inc"
};
EmVelocipreyAppearAt EmVelociprey_AppearPtsTower7[2] = {
#include "game/008C8750.inc"
};
EmVelocipreyAppearAt EmVelociprey_AppearPtsDesert2[3] = {
#include "game/008C8770.inc"
};
EmVelocipreyAppearAt EmVelociprey_AppearPtsDesert4[2] = {
#include "game/008C87A0.inc"
};
EmVelocipreyAppearAt EmVelociprey_AppearPtsDesert7[1] = {
#include "game/008C87C0.inc"
};
EmVelocipreyAppearAt EmVelociprey_AppearPtsSwamp5[2] = {
#include "game/008C87D0.inc"
};
EmVelocipreyAppearAt EmVelociprey_AppearPtsSwamp8[1] = {
#include "game/008C87F0.inc"
};
EmVelocipreyAppearAt EmVelociprey_AppearPtsSwamp9[3] = {
#include "game/008C8800.inc"
};
EmVelocipreyAppearAt EmVelociprey_AppearPtsVolcano3[2] = {
#include "game/008C8830.inc"
};
EmVelocipreyAppearAt EmVelociprey_AppearPtsVolcano6[1] = {
#include "game/008C8850.inc"
};
EmVelocipreyAppearAt EmVelociprey_AppearPtsBattleTown1[4] = {
#include "game/008C8860.inc"
};
EmVelocipreyAppearAt EmVelociprey_AppearPtsBattleTown2[2] = {
#include "game/008C88A0.inc"
};
EmVelocipreyAppearAt EmVelociprey_AppearPtsBattleTown3[2] = {
#include "game/008C88C0.inc"
};
EmTurnParams EmVelociprey_TurnParams[2] = {
#include "game/008C88E0.inc"
};
void *EmVelociprey_Callbacks[8] = {
    EmVelociprey_SpawnShadow, EmVelociprey_SpawnShadow, EmVelociprey_TableNop, EmVelociprey_UpdateMotionEffects,
    EmVelociprey_ScriptQuery, EmVelociprey_Reinit, NULL, NULL,
};
EmCurveKey EmVelociprey_VelYCurves_0[10] = {
#include "game/008C8940.inc"
};
EmCurveKey EmVelociprey_VelYCurves_1[12] = {
#include "game/008C8990.inc"
};
EmCurveKey EmVelociprey_VelYCurves_2[8] = {
#include "game/008C89F0.inc"
};
EmCurveKey EmVelociprey_VelYCurves_3[8] = {
#include "game/008C8A30.inc"
};
EmCurveKey *EmVelociprey_VelYCurves[4] = {
    EmVelociprey_VelYCurves_0, EmVelociprey_VelYCurves_1, EmVelociprey_VelYCurves_2, EmVelociprey_VelYCurves_3,
};
EmCurveKey EmVelociprey_SpeedCurves_0[10] = {
#include "game/008C8A80.inc"
};
EmCurveKey EmVelociprey_SpeedCurves_2[6] = {
#include "game/008C8AD0.inc"
};
EmCurveKey *EmVelociprey_SpeedCurves[4] = {
    EmVelociprey_SpeedCurves_0, NULL, EmVelociprey_SpeedCurves_2, NULL,
};
