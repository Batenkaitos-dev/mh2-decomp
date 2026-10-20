/*
 * Monster family: the Shakalaka (EM_KIND_SHAKALAKA), a small masked creature
 * that walks and runs in a pack, hides in the ground and springs out at a
 * player who comes near, strikes, leaps, and throws bombs. The code from the
 * family's first function, EmShakalaka_MotionSounds (its motions' sounds),
 * to its table's empty hook. Angle_StepToward, a turn helper, sits among
 * its functions in the game's code. The state functions take the monster
 * and its work area (EmShakalakaWork, its view of EmWork); Em.step is the
 * step within the current state. include/em_shakalaka.h names its motions
 * and states.
 */
#include "common.h"
#include "scene.h"
#include "quest.h"
#include "em.h"
#include "em_shakalaka.h"
#include "player.h"
#include "area.h"
void EmShakalaka_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void Em_BeginStatus4(Em *em);
void Em_BeginStatus5(Em *em);
void Em_EndStatus2(Em *em);
void Em_EndStatus4(Em *em);
void Em_EndStatus5(Em *em);
void Em_SetMotion(Em *em, s32 motion, s32 a, s32 b);
void Em_SetMotionRow(Em *em, s32 row, s32 arg2, s32 arg3);
/* unit's own declaration: group and state are declared u16; with the definition's s32 the callers (EmKind38_ChangeState, EmShakalaka_ChangeState ...) pass the state without the 16-bit mask the game's code applies */
void Em_SetStateSync(Em *em, u16 group, u16 state, u8 mode);
u16 Math_AngleXZ(f32 *from, f32 *to);
void Player_ClearFlag(void *, u32 flag);
void Player_SetFlag(void *obj, u32 flag);
s8 QuestEm_Appear(Em *em);
void EmImpactAttack_CreateAt(Em *em, f32 *pos, s8 kind);
void EmBomb_Create(Em *, s8);
void ShakalakaAttack_Create(Em *, s8);
void Em_MoveRotatedAccel(Em *em, void *ang);
void Em_ComputeGoal2(Em *em, f32 *target);
void Em_SpawnStatus4Effect(Em *em, s32 a);
void Em_SetHitReactMode(Em *em, u8 a);
void Em_PlaySeOnFrame(Em *em, s32 frame, s32 se);
s32 EmList_FindKindNear(Em *em, u16 kind, s32 mode, f32 dist);
void Em_SetSpeedScaled(Em *em, u16 flags, f32 x, f32 y);
s32 Em_StepToGoal(Em *em, u16 step, u8 flags);
void Em_RestartScript(Em *em);
void EmShakalaka_Init(Em *);
void EmShakalaka_IdleAction(Em *, s32);
void EmShakalaka_Ambush(Em *);
void EmShakalaka_Status2Collapse(Em *);
void EmShakalaka_MoveAction(Em *, s32);
void EmShakalaka_MoveStraight(Em *, s32);
void EmShakalaka_WanderHome(Em *, s32);
void EmShakalaka_RunToFlank(Em *, s8);
void EmShakalaka_Leap(Em *, s32);
void EmShakalaka_KnockDown(Em *);
void EmShakalaka_Die(Em *);
void EmShakalaka_DieInAir(Em *);
void EmShakalaka_Reappear(Em *);
void EmShakalaka_MotionEffects(Em *, EmWork *);
void EmBodyFx_Spawn(Em *em, s16 kind);
/* unit's own declaration: the arguments are declared s32; with the definition's u8 and s16 the compiler narrows them before the call, which EmKushala_Stun doesn't do */
void StunStars_CreateEm(Em *, s32, s32, s32, f32, f32);
s32 Sound_RandomSe(s32 id, s16 lo, s32 alt, s16 n);
s32 EmMotion_PassedFrameScaled(void *p, f32 frame, s32 layer);
void Vec_RotateY(f32 *v, f32 angle);
f32 Vec_Distance(f32 *a, f32 *b);
void Vec_Copy(f32 *out, f32 *in);
f32 *Mtx_FromAnglesXYZ(s32 *angles, f32 *m);
s32 QuestEm_CanAppear(Em *em);
void Player_ClearVelocity(void *);
/* the calls that mask the value again (u8 or & 0xFF) cast it to s32 first: MWCC keeps the game's andi that way */
u8 Player_InCurrentArea(void *obj);
void Shadow_Spawn(void *obj, s16 type);
void PartFx_SpawnOnGround(f32 *pos, s32 a, f32 scale);
/* unit's own declaration: the bone is declared s32; with the definition's s16 the compiler narrows it before the call, which the ground-effect functions don't do */
void Em_BonePos(void *obj, s32 a, f32 *out);
void Em_SpawnLyingPuffs(Em *em, s32 bone, f32 *pos, f32 scale);
void Em_BeginStatus2(Em *em);
extern f32 EmShakalaka_NearDist[];
u16 Rand_Next(s32 stream);
void QuestEm_Hunted(Em *em);
extern Vec3f EmShakalaka_LeapVel[];
extern Vec3f D_008CA324[];
extern Vec3f D_008CA328[];
s32 Motion_ReachedFrame(void *, s32, f32);
void EmMine_Remove(Em *em);
s32 EmMine_Update(Em *em);
s8 EmMine_Create(Em *em, s32 idx);
void Em_InitParts(Em *em);
s32 Em_ComputeMaxHp(Em *em, s16 idx);
void EmShakalaka_RunState(Em *em);
u8 Em_UpdateDamage(Em *em, u8 *out);
void Em_SetStatus2Timer(Em *em);
void Em_SetStatus4Timer(Em *em);
void Em_SetStatus5Timer(Em *em);
void Em_ResetPartHp(Em *em, u32 i);
void Em_UpdatePendingScript(Em *em);
void Em_ClearHitReactMode(Em *em);
void AttackBurst_CreateAtScaled(f32 *pos, s32 v, s16 k, f32 scale);
s32 Quest_GetTime(s32 which);
void Em_EndStatus5(Em *em);
/* end of prototypes */

/*
 * Plays the sounds of the Shakalaka's motion on layer 0 at its frames
 * (Em_PlaySeOnFrame: frame, sound id), switching on the motion id
 * EmShakalaka_MotionEffects keeps in EmWork.fxMotion.
 */
void EmShakalaka_MotionSounds(Em *em, EmWork *work)
{
    s16 motion;

    motion = work->fxMotion;
    switch (motion) {
    case EM_MOTION_ID(0, SHAK_MOT_IDLE):
        Em_PlaySeOnFrame(em, 0xC, Sound_RandomSe(0x24, 1, 0x24, 1));
        break;
    case EM_MOTION_ID(0, SHAK_MOT_IDLE_ACTION_A):
        Em_PlaySeOnFrame(em, 0x10, 3);
        Em_PlaySeOnFrame(em, 0x16, 0x1D);
        Em_PlaySeOnFrame(em, 0x28, 0x12);
        break;
    case EM_MOTION_ID(0, SHAK_MOT_IDLE_ACTION_B):
        Em_PlaySeOnFrame(em, 8, Sound_RandomSe(0x1C, 3, 0x1C, 2));
        Em_PlaySeOnFrame(em, 0x40, Sound_RandomSe(0x24, 2, 0x24, 2));
        Em_PlaySeOnFrame(em, 0x7A, Sound_RandomSe(0x1C, 2, 0x1C, 0));
        Em_PlaySeOnFrame(em, 0x98, Sound_RandomSe(0x24, 1, 0x24, 1));
        break;
    case EM_MOTION_ID(0, SHAK_MOT_WALK_B):
        Em_PlaySeOnFrame(em, 8, 1);
        Em_PlaySeOnFrame(em, 0x18, 2);
        Em_PlaySeOnFrame(em, 0x28, 1);
        Em_PlaySeOnFrame(em, 0x38, 2);
        Em_PlaySeOnFrame(em, 0x46, 1);
        Em_PlaySeOnFrame(em, 0x58, 2);
        Em_PlaySeOnFrame(em, 0x64, 1);
        Em_PlaySeOnFrame(em, 0x72, 2);
        Em_PlaySeOnFrame(em, 0x2E, Sound_RandomSe(0x1A, 3, 0x1A, 2));
        Em_PlaySeOnFrame(em, 0x58, Sound_RandomSe(0x27, 3, 0x27, 2));
        break;
    case EM_MOTION_ID(0, 0x5):
        Em_PlaySeOnFrame(em, 0xA, 1);
        Em_PlaySeOnFrame(em, 0x24, 2);
        Em_PlaySeOnFrame(em, 0x3A, 1);
        Em_PlaySeOnFrame(em, 0x53, 2);
        Em_PlaySeOnFrame(em, 0x10, Sound_RandomSe(0x1C, 2, 0x1C, 2));
        Em_PlaySeOnFrame(em, 0x3C, Sound_RandomSe(0x24, 1, 0x24, 0));
        break;
    case EM_MOTION_ID(0, SHAK_MOT_IDLE_ACTION_C):
        Em_PlaySeOnFrame(em, 8, 0);
        Em_PlaySeOnFrame(em, 0x1C, 0);
        break;
    case EM_MOTION_ID(0, SHAK_MOT_MOVE_ACTION_A):
        Em_PlaySeOnFrame(em, 8, 3);
        Em_PlaySeOnFrame(em, 0x10, 0x26);
        Em_PlaySeOnFrame(em, 0x1A, 1);
        Em_PlaySeOnFrame(em, 0x1E, 2);
        Em_PlaySeOnFrame(em, 0x2E, 1);
        Em_PlaySeOnFrame(em, 0x3E, 2);
        break;
    case EM_MOTION_ID(0, SHAK_MOT_MOVE_ACTION_B):
        Em_PlaySeOnFrame(em, 8, 3);
        Em_PlaySeOnFrame(em, 0xC, 0x26);
        Em_PlaySeOnFrame(em, 0x1A, 1);
        Em_PlaySeOnFrame(em, 0x1E, 2);
        Em_PlaySeOnFrame(em, 0x2E, 1);
        Em_PlaySeOnFrame(em, 0x3E, 2);
        break;
    case EM_MOTION_ID(0, SHAK_MOT_IDLE_ACTION_D):
        Em_PlaySeOnFrame(em, 0xE, 0x14);
        Em_PlaySeOnFrame(em, 0x32, 0x15);
        Em_PlaySeOnFrame(em, 0x58, 0x16);
        Em_PlaySeOnFrame(em, 0x14, 2);
        Em_PlaySeOnFrame(em, 0xA0, 1);
        break;
    case EM_MOTION_ID(0, SHAK_MOT_IDLE_ACTION_E):
        Em_PlaySeOnFrame(em, 4, 0x1F);
        Em_PlaySeOnFrame(em, 0x28, 0x20);
        Em_PlaySeOnFrame(em, 0x4E, 0x21);
        Em_PlaySeOnFrame(em, 0xC, 2);
        Em_PlaySeOnFrame(em, 0x14, 1);
        break;
    case EM_MOTION_ID(0, SHAK_MOT_IDLE_ACTION_F):
        Em_PlaySeOnFrame(em, 0x2E, 0x22);
        Em_PlaySeOnFrame(em, 0x44, 0x23);
        Em_PlaySeOnFrame(em, 0xE, 2);
        Em_PlaySeOnFrame(em, 0x14, 1);
        break;
    case EM_MOTION_ID(0, SHAK_MOT_WALK):
        Em_PlaySeOnFrame(em, 0xA, 1);
        Em_PlaySeOnFrame(em, 0x1E, 2);
        break;
    case EM_MOTION_ID(0, SHAK_MOT_RUN):
        Em_PlaySeOnFrame(em, 0xC, 1);
        Em_PlaySeOnFrame(em, 0x1A, 2);
        Em_PlaySeOnFrame(em, 0x2A, 1);
        Em_PlaySeOnFrame(em, 0x38, 2);
        Em_PlaySeOnFrame(em, 0x44, 1);
        Em_PlaySeOnFrame(em, 0x52, 2);
        Em_PlaySeOnFrame(em, 0x5E, 1);
        Em_PlaySeOnFrame(em, 0x6A, 2);
        Em_PlaySeOnFrame(em, 0x72, 1);
        Em_PlaySeOnFrame(em, 0x7E, 2);
        Em_PlaySeOnFrame(em, 0x12, 5);
        Em_PlaySeOnFrame(em, 0x32, 6);
        Em_PlaySeOnFrame(em, 0x52, 5);
        Em_PlaySeOnFrame(em, 0x72, 6);
        Em_PlaySeOnFrame(em, 8, Sound_RandomSe(0x2D, 2, 0x2D, 1));
        Em_PlaySeOnFrame(em, 0x44, Sound_RandomSe(0x2D, 1, 0x2D, 2));
        Em_PlaySeOnFrame(em, 0x82, Sound_RandomSe(0x2D, 1, 0x2D, 1));
        break;
    case EM_MOTION_ID(0, SHAK_MOT_AMBUSH):
        Em_PlaySeOnFrame(em, 4, 0xB);
        Em_PlaySeOnFrame(em, 0x20, 0x1E);
        Em_PlaySeOnFrame(em, 0x24, 7);
        Em_PlaySeOnFrame(em, 0x44, 0x12);
        break;
    case EM_MOTION_ID(0, SHAK_MOT_STRIKE):
        Em_PlaySeOnFrame(em, 8, 0x28);
        Em_PlaySeOnFrame(em, 0x14, 1);
        Em_PlaySeOnFrame(em, 0x18, 5);
        Em_PlaySeOnFrame(em, 0x56, 2);
        break;
    case EM_MOTION_ID(0, SHAK_MOT_LEAP):
        Em_PlaySeOnFrame(em, 0xA, 0x29);
        Em_PlaySeOnFrame(em, 0x3A, Sound_RandomSe(0x2D, 3, 0x31, 2));
        Em_PlaySeOnFrame(em, 0x18, 5);
        Em_PlaySeOnFrame(em, 0x2C, 6);
        break;
    case EM_MOTION_ID(0, SHAK_MOT_LEAP_LAND):
        Em_PlaySeOnFrame(em, 2, 0x1B);
        Em_PlaySeOnFrame(em, 6, 9);
        Em_PlaySeOnFrame(em, 0x1E, 0xA);
        Em_PlaySeOnFrame(em, 0x6E, 0x11);
        break;
    case EM_MOTION_ID(0, SHAK_MOT_BOMB_WINDUP):
        Em_PlaySeOnFrame(em, 6, 0x17);
        Em_PlaySeOnFrame(em, 0x10, 0xF);
        Em_PlaySeOnFrame(em, 0x26, 0xF);
        Em_PlaySeOnFrame(em, 0x42, 0xF);
        Em_PlaySeOnFrame(em, 0x5E, 0xF);
        Em_PlaySeOnFrame(em, 0x7A, 0xF);
        break;
    case EM_MOTION_ID(0, SHAK_MOT_BOMB_THROW):
        Em_PlaySeOnFrame(em, 0x2C, 0xE);
        Em_PlaySeOnFrame(em, 0x2C, 0x27);
        Em_PlaySeOnFrame(em, 0x56, 0x32);
        Em_PlaySeOnFrame(em, 0x5A, 0xD);
        Em_PlaySeOnFrame(em, 0x70, 0x12);
        break;
    case EM_MOTION_ID(0, SHAK_MOT_JUMP_UP):
        Em_PlaySeOnFrame(em, 2, 0x28);
        Em_PlaySeOnFrame(em, 8, 5);
        break;
    case EM_MOTION_ID(0, SHAK_MOT_JUMP_FALL):
        Em_PlaySeOnFrame(em, 2, 0x2A);
        Em_PlaySeOnFrame(em, 4, 6);
        break;
    case EM_MOTION_ID(0, SHAK_MOT_JUMP_LAND):
        Em_PlaySeOnFrame(em, 2, 8);
        Em_PlaySeOnFrame(em, 4, 0x12);
        Em_PlaySeOnFrame(em, 0x38, 0x17);
        Em_PlaySeOnFrame(em, 0x62, 0x18);
        Em_PlaySeOnFrame(em, 0x64, 7);
        break;
    case EM_MOTION_ID(0, 0x1F):
        Em_PlaySeOnFrame(em, 6, 1);
        Em_PlaySeOnFrame(em, 0x18, 2);
        Em_PlaySeOnFrame(em, 0x28, 1);
        Em_PlaySeOnFrame(em, 0x3A, 2);
        Em_PlaySeOnFrame(em, 0x46, 1);
        Em_PlaySeOnFrame(em, 0x58, 2);
        Em_PlaySeOnFrame(em, 0x64, 1);
        Em_PlaySeOnFrame(em, 0x72, 2);
        Em_PlaySeOnFrame(em, 0x7E, 1);
        Em_PlaySeOnFrame(em, 0x20, Sound_RandomSe(0x30, 3, 0x30, 2));
        Em_PlaySeOnFrame(em, 0x56, Sound_RandomSe(0x1A, 1, 0x1A, 2));
        break;
    case EM_MOTION_ID(0, SHAK_MOT_FLINCH):
        Em_PlaySeOnFrame(em, 8, 0x1B);
        Em_PlaySeOnFrame(em, 0x16, 1);
        Em_PlaySeOnFrame(em, 0x22, 2);
        Em_PlaySeOnFrame(em, 0x46, 0x24);
        break;
    case EM_MOTION_ID(0, SHAK_MOT_KNOCK_DOWN):
        Em_PlaySeOnFrame(em, 2, 0x2F);
        Em_PlaySeOnFrame(em, 0x12, 0x10);
        Em_PlaySeOnFrame(em, 0x38, 4);
        Em_PlaySeOnFrame(em, 0x58, 0x10);
        Em_PlaySeOnFrame(em, 0x74, 0xA);
        break;
    case EM_MOTION_ID(0, SHAK_MOT_GET_UP):
        Em_PlaySeOnFrame(em, 2, 4);
        Em_PlaySeOnFrame(em, 0x1E, 0x17);
        Em_PlaySeOnFrame(em, 0x26, 3);
        Em_PlaySeOnFrame(em, 0x32, 0x12);
        break;
    case EM_MOTION_ID(0, SHAK_MOT_RECOVER):
        Em_PlaySeOnFrame(em, 0x1A, 0x13);
        Em_PlaySeOnFrame(em, 0x24, 4);
        Em_PlaySeOnFrame(em, 0x96, 0x24);
        Em_PlaySeOnFrame(em, 0x50, 1);
        Em_PlaySeOnFrame(em, 0x60, 2);
        Em_PlaySeOnFrame(em, 0x76, 1);
        Em_PlaySeOnFrame(em, 0x90, 2);
        break;
    case EM_MOTION_ID(0, SHAK_MOT_STATUS4):
        Em_PlaySeOnFrame(em, 4, 0x19);
        Em_PlaySeOnFrame(em, 0x64, Sound_RandomSe(0x19, 2, 0x19, 2));
        break;
    case EM_MOTION_ID(0, SHAK_MOT_SHAKE_OFF):
        Em_PlaySeOnFrame(em, 6, 0x1A);
        Em_PlaySeOnFrame(em, 0xC, 0x13);
        Em_PlaySeOnFrame(em, 0x2E, 0x11);
        break;
    case EM_MOTION_ID(0, SHAK_MOT_COLLAPSE):
        Em_PlaySeOnFrame(em, 0xC, 0x1D);
        Em_PlaySeOnFrame(em, 0x78, 0x1D);
        Em_PlaySeOnFrame(em, 0xCA, 0x10);
        Em_PlaySeOnFrame(em, 0xE6, 0x11);
        break;
    case EM_MOTION_ID(0, SHAK_MOT_DIE):
        Em_PlaySeOnFrame(em, 0xA, 0);
        Em_PlaySeOnFrame(em, 0x1C, 0);
        Em_PlaySeOnFrame(em, 0x2A, 0);
        Em_PlaySeOnFrame(em, 0x3E, 0);
        Em_PlaySeOnFrame(em, 0x4A, 0);
        Em_PlaySeOnFrame(em, 0x5C, 0);
        Em_PlaySeOnFrame(em, 0x68, 0);
        Em_PlaySeOnFrame(em, 0x76, 0);
        Em_PlaySeOnFrame(em, 0x80, 0);
        Em_PlaySeOnFrame(em, 0x8C, 0);
        Em_PlaySeOnFrame(em, 0x96, 0);
        Em_PlaySeOnFrame(em, 0xA2, 0);
        Em_PlaySeOnFrame(em, 0xAE, 0);
        Em_PlaySeOnFrame(em, 0xC0, 3);
        Em_PlaySeOnFrame(em, 0xC8, 0x2B);
        Em_PlaySeOnFrame(em, 0xD6, 0xC);
        Em_PlaySeOnFrame(em, 0xDE, 0x2C);
        break;
    case EM_MOTION_ID(0, SHAK_MOT_FALL):
        Em_PlaySeOnFrame(em, 4, 0x2E);
        break;
    case EM_MOTION_ID(0, SHAK_MOT_FALL_LAND):
        Em_PlaySeOnFrame(em, 2, 0x10);
        Em_PlaySeOnFrame(em, 0xE, 0x2F);
        Em_PlaySeOnFrame(em, 0x18, 0xA);
        break;
    }
}

/*
 * Sets the monster up: places it (a grid or a row of start points by quest
 * slot at the quest's start), sets hp and the idle motion, keeps its spawn
 * point (home) and clears the work. Variants 0 and 1 roll spawnParam: 2
 * keeps it dormant in hiding, more often the less quest time is left;
 * otherwise 0 or 1 picks its ambush distance.
 */
void EmShakalaka_Init(Em *em)
{
    EmShakalakaWork *w = (EmShakalakaWork *)&em->work;
    f32 total;
    f32 timeLeftPct;

    if (gQuest.stage == 0) {
        if (gScene.area == AREA_SNOWY_MOUNTAINS_NIGHT_9 || gScene.area == AREA_FOREST_HILLS_4) {
            em->posX = 9500.0f + 150.0f * (em->slotNo / 8);
            em->posY = 0.0f;
            em->posZ = 9200.0f + 150.0f * (em->slotNo & 7);
        } else {
            em->posX = 2400.0f + 150.0f * (u32) em->slotNo;
            em->posY = 0.0f;
            em->posZ = 3600.0f;
        }
    }
    em->blind = 0;
    em->remote = 0;
    Em_SetMotionRow(em, SHAK_MOT_IDLE, 0, 0);
    em->stance = EM_STANCE_GROUND;
    em->hpMax = em->hp = Em_ComputeMaxHp(em, 0);
    w->bombHeld = 0;
    w->unk00C = 0;
    w->home[0] = em->posX;
    w->home[1] = 0.0f;
    w->home[2] = em->posZ;
    w->unk05D = 0;
    em->painted = 0;
    switch (em->variant) {
    case 1:
        EmBodyFx_Spawn(em, 3);
    case 0:
        total = Quest_GetTime(1);
        timeLeftPct = (100.0f * Quest_GetTime(0)) / total;
        if (timeLeftPct < 0.0f) {
            timeLeftPct = 0.0f;
        } else if (!(timeLeftPct <= 100.0f)) {
            timeLeftPct = 100.0f;
        }
        if (timeLeftPct < 33.0f) {
            if (Rand_Next(1) % 100 <= 29) {
                em->spawnParam = 2;
            } else {
                em->spawnParam = Rand_Next(1) & 1;
            }
        } else if (timeLeftPct < 66.0f) {
            if (Rand_Next(1) % 100 <= 69) {
                em->spawnParam = 2;
            } else {
                em->spawnParam = Rand_Next(1) & 1;
            }
        } else if (Rand_Next(1) % 100 <= 89) {
            em->spawnParam = 2;
        } else {
            em->spawnParam = Rand_Next(1) & 1;
        }
        break;
    case 3:
        EmBodyFx_Spawn(em, 4);
        break;
    case 2:
        break;
    }
    Em_InitParts(em);
}

/*
 * The start hook: variants 0 and 1 hide in the ground (SHAK_IDLE_AMBUSH), 2
 * and 3 stand idle, the others wander toward their spawn point.
 */
void EmShakalaka_Start(Em *em)
{
    EmShakalaka_Init(em);
    switch (em->variant) {
    case 0:
    case 1:
        Em_SetMotionRow(em, SHAK_MOT_HIDDEN, 0, 0);
        EmShakalaka_ChangeState(em, EM_GROUP_IDLE, SHAK_IDLE_AMBUSH, EM_SYNC_LOCAL);
        break;
    case 2:
    case 3:
        Em_SetMotionRow(em, SHAK_MOT_IDLE, 0, 0);
        EmShakalaka_ChangeState(em, EM_GROUP_IDLE, SHAK_IDLE, EM_SYNC_LOCAL);
        break;
    default:
        EmShakalaka_ChangeState(em, EM_GROUP_MOVE, SHAK_MOVE_WANDER_HOME, EM_SYNC_LOCAL);
        break;
    }
}

/* The appear hook: sets the monster up again. */
void EmShakalaka_Reinit(Em *em)
{
    EmShakalaka_Init(em);
}

/*
 * Asked by the bomb it throws: -1 when the bomb should vanish (not on the
 * ground, reacting, defeated, in an event, or inactive), 1 while the bomb
 * is in its hand, 0 once thrown.
 */
s32 EmShakalaka_BombState(Em *em)
{
    EmShakalakaWork *w = (EmShakalakaWork *)&em->work;

    if (em->stance != EM_STANCE_GROUND || (u32) (em->stateGroup - 4) <= 2 || em->active == 0) {
        return -1;
    }
    return w->bombHeld != 0;
}

/*
 * Cuts a move's timer to 15 frames when the target is gone or in another
 * area.
 */
void EmShakalaka_ShortenMoveIfTargetLost(Em *em)
{
    s8 lostTarget;

    if (em->timer > 0xF) {
        lostTarget = em->targetNo;
        if (lostTarget == -1) {
            em->timer = 0xF;
        } else if (em->area != ((Player *) &gPlayers[lostTarget])->area) {
            em->timer = 0xF;
        }
    }
}

/*
 * Moves to state (group, state): sets the stance, computes the goal of a
 * move, begins or ends the stun, lets the quest monster reappear, then
 * Em_SetStateSync.
 */
void EmShakalaka_ChangeState(Em *em, u16 group, u16 state, u8 mode)
{
    em->unk446 = 0;
    switch (group) {
    case EM_GROUP_IDLE:
        em->stance = EM_STANCE_GROUND;
        switch (state) {
        case SHAK_IDLE_AMBUSH:
            em->scriptPending = 0;
            em->senseDelay = 5;
            break;
        case SHAK_IDLE_STUN_END:
            Em_EndStatus5(em);
            break;
        }
        break;
    case EM_GROUP_MOVE:
        em->stance = EM_STANCE_GROUND;
        Em_ComputeGoal2(em, em->goal);
        break;
    case EM_GROUP_ATTACK:
        em->stance = EM_STANCE_GROUND;
        break;
    case EM_GROUP_REACT:
        em->stance = EM_STANCE_GROUND;
        switch (state) {
        case SHAK_REACT_STUN:
            em->stance = EM_STANCE_GROUND;
            Em_BeginStatus5(em);
            break;
        }
        break;
    case EM_GROUP_DEFEAT:
        if (state == 2 && QuestEm_CanAppear(em) == 1) {
            QuestEm_Appear(em);
        }
        break;
    case EM_GROUP_FLY:
    case EM_GROUP_EVENT:
    case EM_GROUP_EVENT2:
    case EM_GROUP_VERSUS:
        break;
    }
    Em_SetStateSync(em, group, state, mode);
}

/* Ends a state: back to SHAK_IDLE, with the script asked for the next one. */
void EmShakalaka_EndState(Em *em)
{
    em->motionSpeed = 1.0f;
    em->scriptPending = 1;
    EmShakalaka_ChangeState(em, EM_GROUP_IDLE, SHAK_IDLE, EM_SYNC_LOCAL);
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
}

/*
 * Turns *angle toward *target the short way by at most step (the target is
 * cut to 16 bits). Returns the step taken.
 */
s32 Angle_StepToward(s32 *angle, s32 *target, s32 step)
{
    s32 diff;
    s32 absDiff;

    *target = (u16) *target;
    diff = (*target - *angle) & 0xFFFF;
    if (diff >= 0x8000) {
        absDiff = -diff & 0xFFFF;
    } else {
        absDiff = diff;
    }
    if (absDiff < step) {
        step = absDiff;
    }
    if (diff >= 0x8000) {
        *angle -= step;
    } else {
        *angle += step;
    }
    return step;
}

/*
 * Plays the idle motion and, after 5 to 20 frames, asks the script for the
 * next state every frame.
 */
void EmShakalaka_WaitIdle(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        if (em->motionId[0] != EM_MOTION_ID(0, SHAK_MOT_IDLE)) {
            Em_SetMotion(em, SHAK_MOT_IDLE, 0, 0);
        }
        em->timer = (em->stateRand & 0xF) + 5;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->scriptPending = 1;
        }
        break;
    }
}

/* Plays the idle motion once, then ends the state. */
void EmShakalaka_Idle(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        if (em->motionId[0] != EM_MOTION_ID(0, SHAK_MOT_IDLE)) {
            Em_SetMotion(em, SHAK_MOT_IDLE, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmShakalaka_EndState(em);
        }
        break;
    }
}

/*
 * The idle actions A to C (variant 0 to 2): a motion to its end, C held 90
 * frames.
 */
void EmShakalaka_IdleAction(Em *em, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        switch (variant) {
        case 0:
        default:
            Em_SetMotion(em, SHAK_MOT_IDLE_ACTION_A, 0, 0);
            break;
        case 1:
            Em_SetMotion(em, SHAK_MOT_IDLE_ACTION_B, 0, 0);
            break;
        case 2:
            Em_SetMotion(em, SHAK_MOT_IDLE_ACTION_C, 0, 0);
            em->timer = 0x5A;
            break;
        }
        break;
    case 1:
        switch (variant) {
        case 0:
        default:
        case 1:
            if (em->motion[0].playing == 0) {
                EmShakalaka_EndState(em);
            }
            break;
        case 2:
            if (--em->timer <= 0) {
                EmShakalaka_EndState(em);
            }
            break;
        }
        break;
    }
}

/* Plays SHAK_MOT_IDLE_ACTION_D once. */
void EmShakalaka_IdleActionD(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, SHAK_MOT_IDLE_ACTION_D, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmShakalaka_EndState(em);
        }
        break;
    }
}

/* Plays SHAK_MOT_IDLE_ACTION_E once. */
void EmShakalaka_IdleActionE(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, SHAK_MOT_IDLE_ACTION_E, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmShakalaka_EndState(em);
        }
        break;
    }
}

/* Plays SHAK_MOT_IDLE_ACTION_F once. */
void EmShakalaka_IdleActionF(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, SHAK_MOT_IDLE_ACTION_F, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmShakalaka_EndState(em);
        }
        break;
    }
}

/*
 * Hides in the ground, unseen and unpushed. With spawnParam 2 it stays
 * dormant and unhittable; otherwise it springs out (SHAK_ATK_AMBUSH) when a
 * visible player in its area comes within EmShakalaka_NearDist[spawnParam].
 */
void EmShakalaka_Ambush(Em *em)
{
    Player *player;
    s32 i;

    em->hideShadowHold = 2;
    em->hideShadow = 2;
    em->stateArmor = 1;
    em->noPushTimer = 0xA;
    em->senseDelay = 5;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, SHAK_MOT_HIDDEN, 0, 0);
        break;
    case 1:
        player = gPlayers;
        if (em->spawnParam == 2) {
            em->noHitTimer = 0xA;
            em->noPushTimer = 0xA;
            break;
        }
        for (i = 0; i < 4; i++, player++) {
            if (player->active != 0 && player->area == em->area && player->visible != 0
                && Vec_Distance((f32 *) &em->posX, (f32 *) player->pos) < EmShakalaka_NearDist[em->spawnParam]) {
                EmShakalaka_ChangeState(em, EM_GROUP_ATTACK, SHAK_ATK_AMBUSH, EM_SYNC_OWNER_FLAGGED);
            }
        }
        break;
    }
}

/*
 * Status 2: collapses and lies, puffing dust, until the status timer ends,
 * then SHAK_IDLE_STATUS2_END.
 */
void EmShakalaka_Status2Collapse(Em *em)
{
    f32 pos[3];

    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, SHAK_MOT_COLLAPSE, 0, 0);
        Em_BeginStatus2(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, SHAK_MOT_LIE, 0, 0);
        }
        break;
    case 2:
        pos[2] = 10.0f;
        pos[0] = 0.0f;
        pos[1] = 0.0f;
        Em_SpawnLyingPuffs(em, 0xA, pos, 0.6f);
        if (--em->timer <= 0) {
            em->step++;
            EmShakalaka_ChangeState(em, EM_GROUP_IDLE, SHAK_IDLE_STATUS2_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/* Ends status 2 and gets up. */
void EmShakalaka_Status2GetUp(Em *em)
{
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, SHAK_MOT_GET_UP, 0, 0);
        Em_EndStatus2(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmShakalaka_EndState(em);
        }
        break;
    }
}

/* Gets up after the stun (entering the state ended it). */
void EmShakalaka_StunGetUp(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, SHAK_MOT_GET_UP, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmShakalaka_EndState(em);
        }
        break;
    }
}

/* Runs the current idle state. */
void EmShakalaka_RunIdleState(Em *em)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->state) {
    case SHAK_IDLE_WAIT:
        EmShakalaka_WaitIdle(em);
        break;
    case SHAK_IDLE:
        EmShakalaka_Idle(em);
        break;
    case SHAK_IDLE_ACTION_A:
        EmShakalaka_IdleAction(em, 0);
        break;
    case SHAK_IDLE_ACTION_B:
        EmShakalaka_IdleAction(em, 1);
        break;
    case SHAK_IDLE_ACTION_C:
        EmShakalaka_IdleAction(em, 2);
        break;
    case SHAK_IDLE_ACTION_D:
        EmShakalaka_IdleActionD(em);
        break;
    case SHAK_IDLE_ACTION_E:
        EmShakalaka_IdleActionE(em);
        break;
    case SHAK_IDLE_ACTION_F:
        EmShakalaka_IdleActionF(em);
        break;
    case SHAK_IDLE_AMBUSH:
        EmShakalaka_Ambush(em);
        break;
    case SHAK_IDLE_STATUS2:
        EmShakalaka_Status2Collapse(em);
        break;
    case SHAK_IDLE_STATUS2_END:
        EmShakalaka_Status2GetUp(em);
        break;
    case SHAK_IDLE_STUN_END:
        EmShakalaka_StunGetUp(em);
        
    default:
        break;
    }
}

/*
 * Walks or runs to the goal (variant: the motion, the speed and the stop
 * distance), turning toward it, for at most 300 frames (210 for the last
 * variant).
 */
void EmShakalaka_MoveToGoal(Em *em, s32 variant)
{
    f32 range;
    s32 timeLeft;
    s32 moveVariant;
    u32 k;

    switch (em->step) {
    case 0:
        em->step++;
        k = variant & 0xFF;
        em->timer = 0x12C;
        switch (k) {
        case 6:
            em->timer = 0xD2;
            Em_SetMotion(em, SHAK_MOT_WALK, 0, 0);
            break;
        case 0x0:
        default:
            Em_SetMotion(em, SHAK_MOT_WALK, 0, 0);
            break;
        case 1:
            Em_SetMotion(em, SHAK_MOT_WALK_B, 0, 0);
            break;
        case 2:
        case 5:
        case 4:
            Em_SetMotion(em, SHAK_MOT_RUN, 0, 0);
            break;
        case 3:
            Em_SetMotion(em, SHAK_MOT_RUN, 0, 0);
            em->speedBonus = 0x32;
        }
        break;
    case 1:
        moveVariant = variant & 0xFF;
        switch (moveVariant) {
        case 6:
            range = 700.0f;
            break;
        case 5:
            range = 600.0f;
            break;
        case 4:
            range = 300.0f;
            break;
        default:
            range = 200.0f;
            break;
        }
        if ((Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) < range) || (timeLeft = em->timer - 1, em->timer = timeLeft, (timeLeft <= 0))) {
            EmShakalaka_EndState(em);
            break;
        }
        em->targetAngle = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        Angle_StepToward(&em->rotY, &em->targetAngle, 0x800);
        Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
        EmShakalaka_ShortenMoveIfTargetLost(em);
        break;
    }
}

/*
 * Plays one of the two move actions four times, turning toward the goal.
 * Both variants run the same code.
 */
void EmShakalaka_MoveAction(Em *em, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 4;
    case 1:
        em->step++;
        switch (Rand_Next(1) & 1) {
        case 0:
        default:
            Em_SetMotion(em, SHAK_MOT_MOVE_ACTION_A, 0, 0);
            break;
        case 1:
            Em_SetMotion(em, SHAK_MOT_MOVE_ACTION_B, 0, 0);
            break;
        }
        em->targetAngle = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            if (--em->timer <= 0) {
                EmShakalaka_EndState(em);
            } else {
                em->step = 1;
            }
        } else {
            em->targetAngle = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
            Angle_StepToward(&em->rotY, &em->targetAngle, 0x800);
            Mtx_FromAnglesXYZ((s32 *) &em->rotX, (f32 *) em->rotMtx);
            EmShakalaka_ShortenMoveIfTargetLost(em);
        }
        break;
    }
}

/*
 * Turns on the spot toward the goal with the walk motion and no root
 * movement.
 */
void EmShakalaka_TurnInPlace(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->targetAngle = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        Em_SetMotion(em, SHAK_MOT_WALK, 0, 0);
        Player_SetFlag((Player *) em, EM_FLAG_NO_ROOT_MOTION);
        break;
    case 1:
        if (Angle_StepToward(&em->rotY, &em->targetAngle, 0x800) < 0x800) {
            Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
            EmShakalaka_EndState(em);
        }
        break;
    }
    Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
}

/*
 * Walks (variant 0) or runs (1) on the heading the goal had at the start,
 * until within 100 of it or 300 frames.
 */
void EmShakalaka_MoveStraight(Em *em, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 0x12C;
        switch (variant & 0xFF) {
        case 0:
        default:
            Em_SetMotion(em, SHAK_MOT_WALK, 0, 0);
            break;
        case 1:
            Em_SetMotion(em, SHAK_MOT_RUN, 0, 0);
            break;
        }
        em->targetAngle = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        break;
    case 1:
        if (Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) < 100.0f || --em->timer <= 0) {
            EmShakalaka_EndState(em);
        } else {
            Angle_StepToward(&em->rotY, &em->targetAngle, 0x800);
            Mtx_FromAnglesXYZ((s32 *) &em->rotX, (f32 *) em->rotMtx);
        }
        break;
    }
}

/* Walks for 120 to 247 frames, turning slowly toward its spawn point. */
void EmShakalaka_WanderHome(Em *em, s32 variant)
{
    EmShakalakaWork *w = (EmShakalakaWork *)&em->work;

    switch (em->step) {
    case 0:
        em->step++;
        em->timer = (em->stateRand & 0x7F) + 0x78;
        switch (variant & 0xFF) {
        case 0:
        default:
            Em_SetMotion(em, SHAK_MOT_WALK, 0, 0);
            break;
        case 1:
            Em_SetMotion(em, SHAK_MOT_WALK_B, 0, 0);
            break;
        }
        break;
    case 1:
        if (em->timer > 0) {
            em->timer--;
        }
        if (em->remote == 0 && em->timer <= 0) {
            EmShakalaka_EndState(em);
        } else {
            em->targetAngle = Math_AngleXZ(&em->posX, w->home) & 0xFFFF;
            Angle_StepToward(&em->rotY, &em->targetAngle, 0x100);
            Mtx_FromAnglesXYZ((s32 *) &em->rotX, (f32 *) em->rotMtx);
        }
        break;
    }
}

/*
 * Runs to a point 600 from the target along one of its axes (variant: -Z,
 * +X or -X; the odd ones half again as fast), until within 300 of it or 150
 * frames.
 */
void EmShakalaka_RunToFlank(Em *em, s8 variant)
{
    Player *target = &gPlayers[em->targetNo];
    f32 pos[3] = {0};

    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 150;
        Em_SetMotion(em, SHAK_MOT_RUN, 0, 0);
        if (variant & 1) {
            em->speedBonus = 0x32;
        }
        break;
    case 1:
        if (variant <= 1) {
            pos[2] = -600.0f;
        } else if (variant <= 3) {
            pos[0] = 600.0f;
        } else {
            pos[0] = -600.0f;
        }
        Vec_RotateY(pos, (6.2831855f * (f32) target->rot[1]) / 65536.0f);
        pos[0] += target->pos[0];
        pos[1] += target->pos[1];
        pos[2] += target->pos[2];
        if (Vec_Distance((f32 *) &em->posX, (f32 *) pos) < 300.0f || --em->timer <= 0) {
            EmShakalaka_EndState(em);
        } else {
            em->targetAngle = Math_AngleXZ(&em->posX, pos) & 0xFFFF;
            Angle_StepToward(&em->rotY, &em->targetAngle, 0x800);
            Mtx_FromAnglesXYZ((s32 *) &em->rotX, (f32 *) em->rotMtx);
            EmShakalaka_ShortenMoveIfTargetLost(em);
        }
        break;
    }
}

/*
 * Runs at the target and 400 beyond it (or 300 frames), then stops and
 * recovers.
 */
void EmShakalaka_DashPastTarget(Em *em)
{
    s32 timeLeft;

    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 0x12C;
        Em_SetMotion(em, SHAK_MOT_RUN, 0, 0);
        Em_SetSpeedScaled(em, 2, 500.0f, -400.0f);
        break;
    case 1:
        if ((Em_StepToGoal(em, 0x40, 0) == 1) || (timeLeft = em->timer - 1, em->timer = timeLeft, (timeLeft <= 0))) {
            em->step++;
            Em_SetMotion(em, SHAK_MOT_LEAP_LAND, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, SHAK_MOT_RECOVER, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmShakalaka_EndState(em);
        }
        break;
    }
}

/* Runs the current move state. */
void EmShakalaka_RunMoveState(Em *em)
{
    switch (em->state) {
    case SHAK_MOVE_WALK:
        EmShakalaka_MoveToGoal(em, 0);
        break;
    case SHAK_MOVE_WALK_B:
        EmShakalaka_MoveToGoal(em, 1);
        break;
    case SHAK_MOVE_RUN:
        EmShakalaka_MoveToGoal(em, 2);
        break;
    case SHAK_MOVE_ACTION_A:
        EmShakalaka_MoveAction(em, 0);
        break;
    case SHAK_MOVE_ACTION_B:
        EmShakalaka_MoveAction(em, 1);
        break;
    case SHAK_MOVE_TURN_IN_PLACE:
        EmShakalaka_TurnInPlace(em);
        break;
    case SHAK_MOVE_WALK_STRAIGHT:
        EmShakalaka_MoveStraight(em, 0);
        break;
    case SHAK_MOVE_RUN_STRAIGHT:
        EmShakalaka_MoveStraight(em, 1);
        break;
    case SHAK_MOVE_WANDER_HOME:
        EmShakalaka_WanderHome(em, 0);
        break;
    case SHAK_MOVE_WANDER_HOME_B:
        EmShakalaka_WanderHome(em, 1);
        break;
    case SHAK_MOVE_RUN_FAST:
        EmShakalaka_MoveToGoal(em, 3);
        break;
    case SHAK_MOVE_RUN_STOP_AT_300:
        EmShakalaka_MoveToGoal(em, 4);
        break;
    case SHAK_MOVE_RUN_STOP_AT_600:
        EmShakalaka_MoveToGoal(em, 5);
        break;
    case SHAK_MOVE_FLANK_BACK:
        EmShakalaka_RunToFlank(em, 0);
        break;
    case SHAK_MOVE_FLANK_BACK_FAST:
        EmShakalaka_RunToFlank(em, 1);
        break;
    case SHAK_MOVE_FLANK_SIDE_A:
        EmShakalaka_RunToFlank(em, 2);
        break;
    case SHAK_MOVE_FLANK_SIDE_A_FAST:
        EmShakalaka_RunToFlank(em, 3);
        break;
    case SHAK_MOVE_FLANK_SIDE_B:
        EmShakalaka_RunToFlank(em, 4);
        break;
    case SHAK_MOVE_FLANK_SIDE_B_FAST:
        EmShakalaka_RunToFlank(em, 5);
        break;
    case SHAK_MOVE_DASH_PAST_TARGET:
        EmShakalaka_DashPastTarget(em);
        break;
    case SHAK_MOVE_WALK_STOP_AT_700:
        EmShakalaka_MoveToGoal(em, 6);
        
    default:
        break;
    }
}

/*
 * The family has no air states: any state of group 2 runs
 * EmShakalaka_WaitIdle.
 */
void EmShakalaka_RunFlyState(Em *em)
{
    EmShakalaka_WaitIdle(em);
}

/* Plays the strike once (its hit comes from EmShakalaka_MotionEffects). */
void EmShakalaka_Strike(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, SHAK_MOT_STRIKE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmShakalaka_EndState(em);
        }
        break;
    }
}

/*
 * Winds up and throws a bomb: it appears in the hand at frame 45 (variant 0
 * the lobbed bomb, 1 the flat one) and leaves it at frame 94.
 */
void EmShakalaka_ThrowBomb(Em *em, s32 variant)
{
    EmShakalakaWork *w = (EmShakalakaWork *)&em->work;

    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, SHAK_MOT_BOMB_WINDUP, 2, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, SHAK_MOT_BOMB_THROW, 0, 0);
        }
        break;
    case 2:
        if (EmMotion_PassedFrameScaled(em, 45.0f, 0U) != 0) {
            em->step++;
            w->bombHeld = 1;
            if (!(variant & 0xFF)) {
                EmBomb_Create(em, 4);
                break;
            }
            EmBomb_Create(em, 5);
        }
        break;
    case 3:
        if (EmMotion_PassedFrameScaled(em, 94.0f, 0U) != 0) {
            em->step++;
            w->bombHeld = 0;
        }
        
    case 4:
        if (em->motion[0].playing == 0) {
            EmShakalaka_EndState(em);
        }
        break;
    }
}

/*
 * Crouches and jumps almost straight up with a body attack, falls, and
 * lands with a shockwave.
 */
void EmShakalaka_JumpSlam(Em *em)
{
    f32 posY;

    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, SHAK_MOT_IDLE_ACTION_E, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, SHAK_MOT_JUMP_WINDUP, 0, 0);
            break;
        }
    default:
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stance = EM_STANCE_AIR;
            Em_SetMotion(em, SHAK_MOT_JUMP_UP, 0, 0);
            Player_ClearVelocity(em);
            em->velY = 30.0f;
            em->accelY = -2.5f;
            em->velZ = 6.0f;
            ShakalakaAttack_Create(em, 4);
        }
        break;
    case 3:
        Em_MoveRotatedAccel(em, &em->rotX);
        posY = em->posY;
        if ((posY < em->prevPosY) && (posY < 160.0f)) {
            em->step++;
            Em_SetMotion(em, SHAK_MOT_JUMP_FALL, 0, 0);
        }
        break;
    case 4:
        Em_MoveRotatedAccel(em, &em->rotX);
        if (em->posY < (80.0f + em->floorY)) {
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            Player_ClearVelocity(em);
            em->step++;
            Em_SetMotion(em, SHAK_MOT_JUMP_LAND, 0, 0);
        }
        break;
    case 5:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            EmShakalaka_EndState(em);
        }
        break;
    }
}

/*
 * Leaps forward with a body attack; the rise, gravity and forward speed
 * come from row `variant` of the leap table (short, medium, long).
 */
void EmShakalaka_Leap(Em *em, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, SHAK_MOT_LEAP, 0, 0);
        break;
    case 1:
        if (Motion_ReachedFrame(em, 0, 8.0f)) {
            em->step++;
            em->stance = EM_STANCE_AIR;
            Player_ClearVelocity(em);
            em->velY = EmShakalaka_LeapVel[variant & 0xFF].x;
            em->accelY = D_008CA324[variant & 0xFF].x;
            em->velZ = D_008CA328[variant & 0xFF].x;
            ShakalakaAttack_Create(em, 2);
        }
        break;
    case 2:
        Em_MoveRotatedAccel(em, &em->rotX);
        if (em->posY <= em->floorY) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            Em_SetMotion(em, SHAK_MOT_LEAP_LAND, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            EmShakalaka_EndState(em);
        }
        break;
    }
}

/* Springs out of the ground with a contact attack. */
void EmShakalaka_AmbushAttack(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->noReact = 1;
        Em_SetMotion(em, SHAK_MOT_AMBUSH, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmShakalaka_EndState(em);
        }
        break;
    }
}

/* Runs the current attack state. */
void EmShakalaka_RunAttackState(Em *em)
{
    switch (em->state) {
    case SHAK_ATK_STRIKE:
        EmShakalaka_Strike(em);
        break;
    case SHAK_ATK_BOMB_LOB:
        EmShakalaka_ThrowBomb(em, 0);
        break;
    case SHAK_ATK_JUMP_SLAM:
        EmShakalaka_JumpSlam(em);
        break;
    case SHAK_ATK_LEAP_SHORT:
        EmShakalaka_Leap(em, 0);
        break;
    case SHAK_ATK_LEAP_MEDIUM:
        EmShakalaka_Leap(em, 1);
        break;
    case SHAK_ATK_LEAP_LONG:
        EmShakalaka_Leap(em, 2);
        break;
    case SHAK_ATK_AMBUSH:
        EmShakalaka_AmbushAttack(em);
        break;
    case SHAK_ATK_BOMB_FLAT:
        EmShakalaka_ThrowBomb(em, 1);
        
    default:
        break;
    }
}

/* Turns to face the hit and flinches. */
void EmShakalaka_Flinch(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, SHAK_MOT_FLINCH, 0, 0);
        em->rotY = em->hitAngle + 0x7FFF + 1;
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmShakalaka_EndState(em);
        }
        break;
    }
}

/* Turns to face the hit, tumbles, lies and gets up. */
void EmShakalaka_KnockDown(Em *em)
{
    f32 unusedMtx[16];
    s32 ang[3];

    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, SHAK_MOT_KNOCK_DOWN, 0, 0);
        ang[0] = 0;
        em->rotY = ang[1] = em->hitAngle + 0x8000;
        ang[2] = 0;
        Mtx_FromAnglesXYZ((s32 *) ang, unusedMtx);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, SHAK_MOT_LIE, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, SHAK_MOT_GET_UP, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            EmShakalaka_EndState(em);
        }
        break;
    }
}

/* Flinches without turning. */
void EmShakalaka_FlinchNoTurn(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_RestartScript(em);
        Em_SetMotion(em, SHAK_MOT_FLINCH, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmShakalaka_EndState(em);
        }
        break;
    }
}

/*
 * Status 4: holds its motion with the status effect until the timer ends,
 * then shakes it off.
 */
void EmShakalaka_Status4(Em *em)
{
    s32 timer;

    timer = em->timer;
    if (timer > 0) {
        em->timer = timer - 1;
        Em_SpawnStatus4Effect(em, 2);
        Em_SetHitReactMode(em, EM_HIT_REACT_STATUS);
    }
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, SHAK_MOT_STATUS4, 0, 0);
        em->rotY = em->hitAngle + 0x7FFF + 1;
        Em_BeginStatus4(em);
        em->noReact = 1;
        Em_RestartScript(em);
        break;
    case 1:
        if (em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, SHAK_MOT_SHAKE_OFF, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, SHAK_MOT_RECOVER, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            Em_EndStatus4(em);
            em->noReact = 0;
            EmShakalaka_EndState(em);
        }
        break;
    }
}

/* A status begun in the air: falls to the floor, lands and recovers. */
void EmShakalaka_FallFromAir(Em *em)
{
    EmShakalakaWork *w = (EmShakalakaWork *)&em->work;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        Em_RestartScript(em);
        Em_SetMotion(em, SHAK_MOT_FALL, 0, 0);
        Player_ClearVelocity(em);
        if (!(em->velY <= -5.0f)) {
            em->velY = -5.0f;
        }
        em->accelX = 0.0f;
        em->accelY = -1.0f;
        em->accelZ = 0.0f;
        w->accelAngX = 0;
        w->accelAngZ = 0;
        break;
    case 1:
        w->accelAngY = (f32) em->rotY;
        Em_MoveRotatedAccel(em, &w->accelAngX);
        if (em->posY <= em->floorY) {
            em->step++;
            em->posY = em->floorY;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, SHAK_MOT_FALL_LAND, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, SHAK_MOT_RECOVER, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmShakalaka_EndState(em);
            Player_ClearVelocity(em);
        }
        break;
    }
}

/*
 * The stun: collapses under the stun's stars, lies until the timer ends,
 * then SHAK_IDLE_STUN_END.
 */
void EmShakalaka_Stun(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, SHAK_MOT_COLLAPSE, 0, 0);
        StunStars_CreateEm(em, 1, 0, *(s16 *)&em->timer, 90.0f, 1.0f);
        break;
    case 1:
        em->timer -= 1;
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, SHAK_MOT_LIE, 0, 0);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            EmShakalaka_ChangeState(em, EM_GROUP_IDLE, SHAK_IDLE_STUN_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmShakalaka_ChangeState(em, EM_GROUP_IDLE, SHAK_IDLE_STUN_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/* Runs the current react state. */
void EmShakalaka_RunReactState(Em *em)
{
    switch (em->state) {
    case SHAK_REACT_FLINCH:
        EmShakalaka_Flinch(em);
        break;
    case SHAK_REACT_KNOCK_DOWN:
        EmShakalaka_KnockDown(em);
        break;
    case SHAK_REACT_FLINCH_NO_TURN:
        EmShakalaka_FlinchNoTurn(em);
        break;
    case SHAK_REACT_STATUS4:
        EmShakalaka_Status4(em);
        break;
    case SHAK_REACT_FALL:
        EmShakalaka_FallFromAir(em);
        break;
    case SHAK_REACT_STUN:
        EmShakalaka_Stun(em);
        
    default:
        break;
    }
}

/*
 * Dies on the ground: counts the hunt, leaves a carve point for up to 2400
 * frames, fades over 100 frames, then SHAK_DEFEAT_REAPPEAR.
 */
void EmShakalaka_Die(Em *em)
{
    f32 unusedMtx[16];
    s32 ang[3];

    em->noHitTimer = 10;
    em->noPushTimer = 10;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->posY = em->floorY;
        Em_SetMotion(em, SHAK_MOT_DIE, 0, 0);
        ang[1] = em->hitAngle + 0x8000;
        ang[2] = 0;
        Mtx_FromAnglesXYZ((s32 *) ang, unusedMtx);
        QuestEm_Hunted(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 0x960;
            if (em->variant == 3) {
                EmMine_Create(em, 1);
            } else {
                EmMine_Create(em, 0);
            }
        }
        break;
    case 2:
        if (EmMine_Update(em) <= 0 || --em->timer <= 0) {
            em->step++;
            em->timer = 100;
            EmMine_Remove(em);
        }
        break;
    case 3:
        em->motionSpeed = 0.0f;
        em->timer--;
        if (em->timer <= 99) {
            em->alpha = em->timer / 100.0f;
        }
        if (em->timer <= 0) {
            em->visible = 0;
            EmShakalaka_ChangeState(em, EM_GROUP_DEFEAT, SHAK_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/* Dies in the air: falls and lands first, then as EmShakalaka_Die. */
void EmShakalaka_DieInAir(Em *em)
{
    EmShakalakaWork *w = (EmShakalakaWork *)&em->work;
    f32 unusedMtx[16];
    s32 ang[3];

    em->noHitTimer = 10;
    em->noPushTimer = 10;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        Em_RestartScript(em);
        Em_SetMotion(em, SHAK_MOT_FALL, 0, 0);
        Player_ClearVelocity(em);
        if (!(em->velY <= -5.0f)) {
            em->velY = -5.0f;
        }
        em->accelX = 0.0f;
        em->accelY = -1.0f;
        *(s32 *) &em->accelZ = 0;
        w->accelAngX = 0;
        w->accelAngZ = 0;
        QuestEm_Hunted(em);
        break;
    case 1:
        w->accelAngY = em->rotY;
        Em_MoveRotatedAccel(em, &w->accelAngX);
        if (em->posY <= em->floorY) {
            em->step++;
            em->posY = em->floorY;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, SHAK_MOT_FALL_LAND, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, SHAK_MOT_RECOVER, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, SHAK_MOT_DIE, 0, 0);
            ang[1] = em->hitAngle + 0x8000;
            ang[2] = 0;
            Mtx_FromAnglesXYZ((s32 *) ang, unusedMtx);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 0x960;
            if (em->variant == 3) {
                EmMine_Create(em, 1);
            } else {
                EmMine_Create(em, 0);
            }
        }
        break;
    case 5:
        if (EmMine_Update(em) <= 0 || --em->timer <= 0) {
            em->step++;
            em->timer = 100;
            EmMine_Remove(em);
        }
        break;
    case 6:
        em->motionSpeed = 0.0f;
        em->timer--;
        if (em->timer <= 99) {
            em->alpha = em->timer / 100.0f;
        }
        if (em->timer <= 0) {
            em->visible = 0;
            EmShakalaka_ChangeState(em, EM_GROUP_DEFEAT, SHAK_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * Comes back as EmShakalaka_Start does when the quest lets it reappear;
 * otherwise ends the monster.
 */
void EmShakalaka_Reappear(Em *em)
{
    switch (em->step) {
    case 0:
        if (QuestEm_CanAppear(em) == 1) {
            switch (em->variant) {
            case 0:
            case 1:
                Em_SetMotionRow(em, SHAK_MOT_HIDDEN, 0, 0);
                EmShakalaka_ChangeState(em, EM_GROUP_IDLE, SHAK_IDLE_AMBUSH, EM_SYNC_LOCAL);
                break;
            case 2:
            case 3:
                Em_SetMotionRow(em, SHAK_MOT_IDLE, 0, 0);
                EmShakalaka_ChangeState(em, EM_GROUP_IDLE, SHAK_IDLE, EM_SYNC_LOCAL);
                break;
            default:
                EmShakalaka_ChangeState(em, EM_GROUP_MOVE, SHAK_MOVE_WANDER_HOME, EM_SYNC_LOCAL);
                break;
            }
        } else {
            em->taskPhase++;
        }
        break;
    case 1:
        break;
    }
}

/* Runs the current defeat state. */
void EmShakalaka_RunDefeatState(Em *em)
{
    switch (em->state) {
    case SHAK_DEFEAT_DIE:
        EmShakalaka_Die(em);
        break;
    case SHAK_DEFEAT_DIE_IN_AIR:
        EmShakalaka_DieInAir(em);
        break;
    case SHAK_DEFEAT_REAPPEAR:
        EmShakalaka_Reappear(em);
        break;
    }
}

/* Does nothing. */
void EmShakalaka_EventNone(void)
{
}

/* Runs the current event state. */
void EmShakalaka_RunEventState(Em *em)
{
    switch (em->state) {
    case SHAK_EVENT_NONE:
        EmShakalaka_EventNone();
        break;
    }
}

/*
 * The frame function: picks a reaction from Em_UpdateDamage's result
 * (death, fall, statuses, flinch, knock-down, stun), runs the pending
 * script, then the state (twice when a state change was requested).
 */
void EmShakalaka_Update(Em *em)
{
    EmShakalakaWork *w = (EmShakalakaWork *)&em->work;
    u8 buf[3];

    if (w->unk00C != 0) {
        (w->unk00C)--;
    }
    switch (Em_UpdateDamage(em, buf) & 0xFF) {
    case 0:
    case 3:
    case 4:
    case 7:
    case 9:
    case 11:
    case 15:
    case 16:
    case 18:
        break;
    case 1:
    case 2:
        if (em->stance == EM_STANCE_AIR) {
            EmShakalaka_ChangeState(em, EM_GROUP_DEFEAT, SHAK_DEFEAT_DIE_IN_AIR, EM_SYNC_OWNER);
        } else {
            EmShakalaka_ChangeState(em, EM_GROUP_DEFEAT, SHAK_DEFEAT_DIE, EM_SYNC_OWNER);
        }
        break;
    case 5:
        if (em->stateGroup != EM_GROUP_REACT || em->state != SHAK_REACT_FALL) {
            EmShakalaka_ChangeState(em, EM_GROUP_REACT, SHAK_REACT_FALL, EM_SYNC_OWNER);
        }
        break;
    case 6:
        if (em->stateGroup != EM_GROUP_REACT || em->state != SHAK_REACT_FALL) {
            Em_SetStatus4Timer(em);
            EmShakalaka_ChangeState(em, EM_GROUP_REACT, SHAK_REACT_STATUS4, EM_SYNC_OWNER);
        }
        break;
    case 8:
        if (em->stateGroup != EM_GROUP_REACT || em->state != SHAK_REACT_FALL) {
            Em_SetStatus2Timer(em);
            EmShakalaka_ChangeState(em, EM_GROUP_IDLE, SHAK_IDLE_STATUS2, EM_SYNC_OWNER);
        }
        break;
    case 10:
        em->blind = 0;
        Em_EndStatus2(em);
        EmShakalaka_ChangeState(em, EM_GROUP_REACT, SHAK_REACT_KNOCK_DOWN, EM_SYNC_OWNER);
        break;
    case 12:
    case 13:
        EmShakalaka_ChangeState(em, EM_GROUP_REACT, SHAK_REACT_KNOCK_DOWN, EM_SYNC_OWNER);
        Em_ResetPartHp(em, 0);
        break;
    case 14:
        if (em->stance == EM_STANCE_AIR) {
            EmShakalaka_ChangeState(em, EM_GROUP_REACT, SHAK_REACT_KNOCK_DOWN, EM_SYNC_OWNER);
        } else {
            EmShakalaka_ChangeState(em, EM_GROUP_REACT, SHAK_REACT_FLINCH, EM_SYNC_OWNER);
        }
        em->scriptPending = 0;
        break;
    case 17:
        if ((em->stateGroup != EM_GROUP_REACT || em->state != SHAK_REACT_FALL) && (em->stateGroup != EM_GROUP_REACT || em->state != SHAK_REACT_STUN)) {
            Em_SetStatus5Timer(em);
            EmShakalaka_ChangeState(em, EM_GROUP_REACT, SHAK_REACT_STUN, EM_SYNC_OWNER);
        }
        break;
    }
    Em_UpdatePendingScript(em);
    Em_ClearHitReactMode(em);
    EmShakalaka_RunState(em);
    if (em->stateRequest != 0) {
        EmShakalaka_RunState(em);
        em->stateRequest = 0;
    }
}

/* Runs the current state by its group (group 7 as idle). */
void EmShakalaka_RunState(Em *em)
{
    switch (em->stateGroup) {
    case EM_GROUP_IDLE:
        EmShakalaka_RunIdleState(em);
        break;
    case EM_GROUP_MOVE:
        EmShakalaka_RunMoveState(em);
        break;
    case EM_GROUP_FLY:
        EmShakalaka_RunFlyState(em);
        break;
    case EM_GROUP_ATTACK:
        EmShakalaka_RunAttackState(em);
        break;
    case EM_GROUP_REACT:
        EmShakalaka_RunReactState(em);
        break;
    case EM_GROUP_DEFEAT:
        EmShakalaka_RunDefeatState(em);
        break;
    case EM_GROUP_EVENT:
        EmShakalaka_RunEventState(em);
        break;
    case EM_GROUP_EVENT2:
        EmShakalaka_RunIdleState(em);
        
    default:
        break;
    }
}

static void EmShakalaka_MotionEffectsDefault(Em *em)
{
}

/*
 * Puts ground effect `kind` on the floor under bone `bone`, scaled by the
 * monster's size, when a player is in its area.
 */
void EmShakalaka_SpawnGroundEffect(Em *em, s32 bone, s32 kind, f32 scale)
{
    f32 pos[3];

    if (Player_InCurrentArea(em)) {
        Em_BonePos(em, bone, pos);
        pos[1] = em->floorY;
        PartFx_SpawnOnGround(pos, kind, scale * em->scaleX);
    }
}

/* The same at `ofs` from the bone, turned by the monster's heading. */
void EmShakalaka_SpawnGroundEffectOffset(Em *em, s32 bone, s32 kind, f32 *ofs, f32 scale)
{
    f32 pos[3];
    f32 rotatedOfs[3];

    if (Player_InCurrentArea(em)) {
        Vec_Copy(rotatedOfs, ofs);
        Vec_RotateY(rotatedOfs, (6.2831855f * (f32) em->rotY) / 65536.0f);
        Em_BonePos(em, bone, pos);
        pos[1] = em->floorY;
        pos[0] = pos[0] + rotatedOfs[0];
        pos[1] = pos[1] + rotatedOfs[1];
        pos[2] = pos[2] + rotatedOfs[2];
        PartFx_SpawnOnGround(pos, kind, scale * em->scaleX);
    }
}

/*
 * The motions' sounds and, at set frames, their attacks, dust and the jump
 * slam's shockwave.
 */
void EmShakalaka_MotionEffects(Em *em, EmWork *work)
{
    f32 pos[3];
    f32 forwardOfs[3];

    if (em->motionId[0] != work->fxMotion) {
        work->fxMotion = em->motionId[0];
    }
    EmShakalaka_MotionSounds(em, work);
    switch (work->fxMotion) {
    case EM_MOTION_ID(0, SHAK_MOT_MOVE_ACTION_A):
    case EM_MOTION_ID(0, SHAK_MOT_MOVE_ACTION_B):
        if (EmMotion_PassedFrameScaled(em, 8.0f, 0) != 0) {
            EmShakalaka_SpawnGroundEffect(em, 1, 0x14, 2.0f);
        }
        break;
    case EM_MOTION_ID(0, SHAK_MOT_STRIKE):
        if (EmMotion_PassedFrameScaled(em, 24.0f, 0) != 0) {
            ShakalakaAttack_Create(em, 1);
        }
        break;
    case EM_MOTION_ID(0, SHAK_MOT_RUN):
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            ShakalakaAttack_Create(em, 3);
        }
        break;
    case EM_MOTION_ID(0, SHAK_MOT_AMBUSH):
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            ShakalakaAttack_Create(em, 5);
        }
        if (EmMotion_PassedFrameScaled(em, 20.0f, 0) != 0) {
            EmShakalaka_SpawnGroundEffect(em, 1, 0x14, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 70.0f, 0) != 0) {
            EmShakalaka_SpawnGroundEffect(em, 1, 4, 2.0f);
        }
        break;
    case EM_MOTION_ID(0, SHAK_MOT_LEAP_LAND):
        if (EmMotion_PassedFrameScaled(em, 8.0f, 0) != 0) {
            EmShakalaka_SpawnGroundEffect(em, 0xA, 0x14, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 42.0f, 0) != 0) {
            EmShakalaka_SpawnGroundEffect(em, 0xA, 4, 2.0f);
        }
        break;
    case EM_MOTION_ID(0, SHAK_MOT_JUMP_LAND):
        if ((EmMotion_PassedFrameScaled(em, 4.0f, 0) != 0) && Player_InCurrentArea(em)) {
            forwardOfs[0] = 0.0f;
            forwardOfs[1] = 0.0f;
            forwardOfs[2] = 55.0f * em->scaleZ;
            EmShakalaka_SpawnGroundEffectOffset(em, 1, 0x28, forwardOfs, 0.2f);
            EmShakalaka_SpawnGroundEffectOffset(em, 1, 0xE, forwardOfs, 0.4f);
            Vec_RotateY(forwardOfs, (6.2831855f * (f32) em->rotY) / 65536.0f);
            Em_BonePos(em, 1, pos);
            pos[1] = em->floorY;
            pos[0] += forwardOfs[0];
            pos[1] += forwardOfs[1];
            pos[2] += forwardOfs[2];
            AttackBurst_CreateAtScaled(pos, *(u16 *) &em->rotY, 0xA, 0.2f * em->scaleX);
            EmImpactAttack_CreateAt(em, (f32 *) pos, 0xD);
        }
        break;
    case EM_MOTION_ID(0, SHAK_MOT_KNOCK_DOWN):
        if ((EmMotion_PassedFrameScaled(em, 28.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 60.0f, 0) != 0)) {
            EmShakalaka_SpawnGroundEffect(em, 1, 0x14, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 96.0f, 0) != 0) {
            EmShakalaka_SpawnGroundEffect(em, 1, 4, 2.0f);
        }
        break;
    case EM_MOTION_ID(0, SHAK_MOT_SHAKE_OFF):
        if (EmMotion_PassedFrameScaled(em, 48.0f, 0) != 0) {
            EmShakalaka_SpawnGroundEffect(em, 0xA, 4, 2.0f);
        }
        break;
    case EM_MOTION_ID(0, SHAK_MOT_COLLAPSE):
        if (EmMotion_PassedFrameScaled(em, 206.0f, 0) != 0) {
            EmShakalaka_SpawnGroundEffect(em, 0xA, 4, 2.0f);
        }
        break;
    case EM_MOTION_ID(0, SHAK_MOT_DIE):
        if ((EmMotion_PassedFrameScaled(em, 16.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 46.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 76.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 106.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 132.0f, 0) != 0) || (EmMotion_PassedFrameScaled(em, 152.0f, 0) != 0)) {
            forwardOfs[0] = 0.0f;
            forwardOfs[1] = 0.0f;
            forwardOfs[2] = 55.0f * em->scaleZ;
            EmShakalaka_SpawnGroundEffectOffset(em, 1, 4, forwardOfs, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 218.0f, 0) != 0) {
            EmShakalaka_SpawnGroundEffect(em, 0xA, 7, 0.5f);
        }
        break;
    case EM_MOTION_ID(0, SHAK_MOT_FALL_LAND):
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            EmShakalaka_SpawnGroundEffect(em, 3, 4, 2.0f);
        }
        if (EmMotion_PassedFrameScaled(em, 48.0f, 0) != 0) {
            EmShakalaka_SpawnGroundEffect(em, 3, 7, 0.5f);
        }
        break;
    default:
        EmShakalaka_MotionEffectsDefault(em);
        break;
    }
}

/* Runs EmShakalaka_MotionEffects from the second frame on. */
void EmShakalaka_UpdateMotionEffects(Em *em)
{
    EmShakalakaWork *w = (EmShakalakaWork *)&em->work;

    switch (w->fxStep) {
    case 0:
        w->fxStep++;
        break;
    case 1:
        EmShakalaka_MotionEffects(em, (EmWork *) w);
        break;
    }
}

/*
 * The script's question 1: 1 when an active Chameleos on the ground is
 * within 2000 in the same area, else 0.
 */
s32 EmShakalaka_ScriptQuery(Em *em, u8 flag)
{
    switch (flag) {
    case 1:
        if (EmList_FindKindNear(em, 0x3B, 1, 2000.0f) == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

/* Creates the effect drawn under the monster (Effect01 type 0). */
void EmShakalaka_SpawnEffect01(u8 *em)
{
    Shadow_Spawn((Player *) em, 0);
}

/* An empty hook of the family's table. */
void EmShakalaka_TableNop(void)
{
}

/* The file's initialised data, in the game's order */
f32 EmShakalaka_NearDist[4] = {
#include "game/008CA310.inc"
};
Vec3f EmShakalaka_LeapVel[4] = {
#include "game/008CA320.inc"
};
void *EmShakalaka_Callbacks[8] = {
    EmShakalaka_SpawnEffect01, EmShakalaka_SpawnEffect01, EmShakalaka_TableNop, EmShakalaka_UpdateMotionEffects,
    EmShakalaka_ScriptQuery, EmShakalaka_Reinit, NULL, NULL,
};
