/*
 * The Apceros family (EM_KIND_APCEROS; the per-kind hooks send the Popo,
 * EM_KIND_POPO, and kind 0x0C to the same functions): herbivores that graze
 * near their spawn point, watch hunters, and charge them when the herd is
 * hurt (EmApceros_CheckHerdHurt). The file runs from the family's sound
 * helper (EmApceros_PlaySeOnFrame, which picks the small kinds' sounds) to
 * its answers to the quest scripts (EmApceros_ScriptQuery):
 * - Init, Start and Reinit;
 * - one function per state, by group (include/em_apceros.h names the states
 *   and the motions; group 7 holds the arrivals, fading in), a
 *   dispatcher per group (EmApceros_Run*State) and ChangeState, which
 *   enters a state;
 * - the event states of a scripted scene where a Rathian or Rathalos kills
 *   one (EmApceros_EventLie, EventDie);
 * - EmApceros_Update, which turns Em_UpdateDamage's result into reactions,
 *   and the motion effects.
 * The state functions take the monster and its work area, read as
 * EmApcerosWork; Em.step is the step within the current state.
 */
#include "common.h"
#include "scene.h"
#include "quest.h"
#include "em.h"
#include "em_apceros.h"
#include "player.h"
#include "area.h"
/* prototypes */
void EmApceros_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void Em_BeginStatus2(Em *em);
void Em_EndStatus2(Em *em);
void Em_EndStatus4(Em *em);
void Em_EndStatus5(Em *em);
void Em_SetMotion(Em *em, s32 motion, s32 a, s32 b);
/* unit's own declaration: declared here as the callers use it; with the definition's void (Em *em, s32 group, s32 state, u8 mode), the calls in EmApceros_ChangeState compile differently */
void Em_SetStateSync(Em *em, u16 group, u16 state, u8 mode);
u16 Math_AngleXZ(f32 *from, f32 *to);
void QuestEm_Hunted(Em *em);
void QuestEm_Leave(Em *em);
u16 Rand_Next(s32 stream);
s32 Save_TestEventFlag(s32);
s32 MsgEvent_Check(u16 kind);
/* unit's own declaration: declared here as the callers use it; with the definition's void (Em *em, u8 a, u8 prio), the calls in EmApceros_SetRequest compile differently */
void Em_SetRequest(Em *em, s32 a, s32 prio);
void Em_SpawnStatus4Effect(Em *em, s32 a);
void Em_SetHitReactMode(Em *em, u8 a);
void Em_SpawnLyingPuffs(Em *em, s32 bone, f32 *pos, f32 scale);
void Em_StartTurnToTarget(Em *em, EmTurnParams *t);
s32 Em_TurnStep(Em *em, EmTurnParams *t);
void Em_RestartScript(Em *em);
void EmApceros_Init(Em *em);
void EmApceros_EndState(Em *);
void EmApceros_IdleCircleHome(Em *);
void EmApceros_Status2Collapse(Em *);
void EmApceros_WanderHome(Em *);
void EmApceros_WatchHunter(Em *);
void EmApceros_WalkTurnA(Em *);
void EmApceros_WalkTurnB(Em *);
void EmApceros_TurnToGoal(Em *);
void EmApceros_RunToGoal(Em *);
void EmApceros_WalkToGoal(Em *);
void EmApceros_FlinchA(Em *);
void EmApceros_FlinchB(Em *);
void EmApceros_KnockDown(Em *, s32);
void EmApceros_FlinchRandom(Em *);
void EmApceros_Status4(Em *);
void EmApceros_Die(Em *, s32);
void EmApceros_SpawnEffect02AtBone2(Em *em, f32 scale, s32 arg2);
void EmApceros_EventFall(Em *);
void EmApceros_EventIntroPrey(Em *);
void EmApceros_EventReshow(Em *em);
void EmApceros_MotionEffects(Em *, EmWork *);
void DustCloud_Create(Em *em, s16 k, s16 c, f32 scale);
s32 Em_PlaySeChecked(Em *em, s32 id, s32 a, void *pos, s32 b, s32 c);
s32 EmMotion_PassedFrameScaled(void *p, f32 frame, s32 layer);
s32 EmMotion_ReachedFrame(void *obj, s32 a, f32 frame);
s32 EmMotion_InFrameRange(void *em, s32 layer, f32 from, f32 to);
void Mtx_GetTrans(f32 *out, f32 *m);
void CamShake_ReqAIf(Em *obj, s32 i);
void Vec_Set(f32 *out, f32 x, f32 y, f32 z);
void ObjListA70_Remove(EmObj *obj);
s32 QuestEm_CanAppear(Em *em);
u8 Player_InCurrentArea(void *obj);
void Shadow_Spawn(void *obj, s16 type);
extern EmTurnParams EmApceros_TurnParams[1];
void GmEm_ResetSize(Em *em);
s8 QuestEm_Appear(Em *em);
void Em_BeginStatus4(Em *em);
void Em_SetSpeedScaled(Em *em, u16 flags, f32 x, f32 y);
void Vec_Copy(f32 *out, f32 *in);
f32 *Mtx_FromAnglesXYZ(s32 *angles, f32 *m);
u16 Math_Heading(f32 x1, f32 z1, f32 x2, f32 z2);
void Em_SetMotionRow(Em *em, s32 row, s32 arg2, s32 arg3);
s32 Em_ComputeMaxHp(Em *em, s16 idx);
s8 EmMine_Create(Em *em, s32 idx);
s32 EmMine_Update(Em *em);
void EmMine_Remove(Em *em);
void Vec_RotateY(f32 *v, f32 angle);
s32 Quest_IsTraining(void);
void Em_MotionDelta(Em *em, f32 *out);
s8 Em_FindTargetPlayer(Em *em, u16 range, f32 maxDist, f32 maxHeight);
s32 Obj_SameArea(void *obj, Player *b);
s32 Em_UpdateCombatTimer(Em *em);
u8 Em_UpdateDamage(Em *em, u8 *out);
void Em_SetStatus4Timer(Em *em);
void Em_SetStatus2Timer(Em *em);
void Em_UpdatePendingScript(Em *em);
void Em_ClearHitReactMode(Em *em);
u8 Em_SenseObjects(Em *em, EmObj *self);
s32 ObjListA70_Add(EmObj *obj);
u16 Area_DayId(u16 id);
void EmApceros_RunState(Em *em);
extern u8 gEmShowsOnMap[];
s32 Em_InBlockArea1(Em *em);
void Mtx_Copy(f32 *dst, f32 *src);
EmBone *Em_Bone(void *obj, s16 bone);
void Vec_RotateSelf(f32 *v, f32 *m);
f32 Effect_Scale(void);
f32 Math_Atan2(f32 y, f32 x);
f32 Math_Sqrtf(f32 x);
void HitSpark_SpawnAtScaled(u16 ax, u16 ay, s32 kind, f32 *pos, f32 scale);
extern f32 EmApceros_ScalesType0C[];
extern f32 EmApceros_ScalesType46[];
void GmEm_ApplySize(Em *em);
void VespoidAttack_Create(Em *, s8);
void PartFx_SpawnOnGround(f32 *pos, s32 a, f32 scale);
void Em_BonePos(void *obj, s16 bone, f32 *out);
void PartFx_SpawnForEm(Em *em, s32 a, s32 b);
/* end of prototypes */

/*
 * Plays sound id at the given bone position when motion layer 0 passes the
 * frame (Em_PlaySeChecked); kind 0x0C at scaleX <= 0.5 and the Popo
 * (EM_KIND_POPO) at scaleX <= 0.8 play id + 0x20 instead (the small-size
 * sounds).
 */
void EmApceros_PlaySeOnFrame(Em *em, s32 frame, s32 id, s32 bone)
{
    f32 pos[3];
    u8 type;

    if (EmMotion_PassedFrameScaled(em, (f32) frame, 0U) != 0) {
        Mtx_GetTrans(pos, em->model->bones[bone].world);
        type = em->type;
        if ((type == 0xC && em->scaleX <= 0.5f) || (type == EM_KIND_POPO && em->scaleX <= 0.8f)) {
            Em_PlaySeChecked(em, id + 0x20, 0, pos, 4, 0);
            return;
        }
        Em_PlaySeChecked(em, id, 0, pos, 4, 0);
    }
}

/*
 * Switches on work->fxMotion and plays each motion sounds at their frames
 * and bones through EmApceros_PlaySeOnFrame.
 */
void EmApceros_MotionSounds(Em *em, EmWork *work)
{
    s16 motion;

    motion = work->fxMotion;
    switch (motion) {
    case EM_MOTION_ID(0, APC_MOT_STAND):
        EmApceros_PlaySeOnFrame(em, 0x100, 9, 0x10);
        break;
    case EM_MOTION_ID(0, APC_MOT_REAR_UP):
        EmApceros_PlaySeOnFrame(em, 0x20, 0xA, 0x10);
        EmApceros_PlaySeOnFrame(em, 0x86, 0xB, 0x10);
        EmApceros_PlaySeOnFrame(em, 0xA0, 8, 0x10);
        EmApceros_PlaySeOnFrame(em, 0x26, 0x14, 0);
        EmApceros_PlaySeOnFrame(em, 0x52, 5, 0x14);
        EmApceros_PlaySeOnFrame(em, 0xCE, 4, 0x14);
        break;
    case EM_MOTION_ID(0, APC_MOT_IDLE_LOOP_B):
        EmApceros_PlaySeOnFrame(em, 0x60, 0xB, 0x10);
        EmApceros_PlaySeOnFrame(em, 0xCC, 0xB, 0x10);
        break;
    case EM_MOTION_ID(0, APC_MOT_IDLE_ACTION_A):
        EmApceros_PlaySeOnFrame(em, 0x46, 7, 0x10);
        EmApceros_PlaySeOnFrame(em, 0x6A, 8, 0x10);
        break;
    case EM_MOTION_ID(0, APC_MOT_IDLE_ACTION_B):
        EmApceros_PlaySeOnFrame(em, 0x20, 8, 0x10);
        break;
    case EM_MOTION_ID(0, APC_MOT_STOMP):
        EmApceros_PlaySeOnFrame(em, 0x2C, 7, 0x10);
        EmApceros_PlaySeOnFrame(em, 0x28, 3, 6);
        EmApceros_PlaySeOnFrame(em, 0x26, 2, 0xA);
        break;
    case EM_MOTION_ID(0, APC_MOT_WALK):
        EmApceros_PlaySeOnFrame(em, 0x20, 1, 6);
        EmApceros_PlaySeOnFrame(em, 0x6A, 1, 0xA);
        EmApceros_PlaySeOnFrame(em, 0xBA, 1, 6);
        EmApceros_PlaySeOnFrame(em, 0x2E, 4, 0x14);
        EmApceros_PlaySeOnFrame(em, 0x84, 4, 0x17);
        EmApceros_PlaySeOnFrame(em, 0xD0, 4, 0x14);
        break;
    case EM_MOTION_ID(0, 0xC):
        EmApceros_PlaySeOnFrame(em, 0x26, 3, 6);
        EmApceros_PlaySeOnFrame(em, 0x30, 3, 0xA);
        EmApceros_PlaySeOnFrame(em, 0xC, 6, 0x17);
        EmApceros_PlaySeOnFrame(em, 0x12, 6, 0x14);
        break;
    case EM_MOTION_ID(0, APC_MOT_RUN):
        EmApceros_PlaySeOnFrame(em, 0x14, 3, 0xA);
        EmApceros_PlaySeOnFrame(em, 0x4E, 3, 0xA);
        EmApceros_PlaySeOnFrame(em, 0x34, 3, 6);
        EmApceros_PlaySeOnFrame(em, 0x1A, 6, 0x14);
        EmApceros_PlaySeOnFrame(em, 0x3A, 6, 0x17);
        break;
    case EM_MOTION_ID(0, APC_MOT_TURN_A):
        EmApceros_PlaySeOnFrame(em, 0x40, 1, 6);
        EmApceros_PlaySeOnFrame(em, 0x60, 1, 0xA);
        EmApceros_PlaySeOnFrame(em, 0x8A, 4, 0x17);
        EmApceros_PlaySeOnFrame(em, 0xAE, 4, 0x14);
        break;
    case EM_MOTION_ID(0, APC_MOT_TURN_B):
        EmApceros_PlaySeOnFrame(em, 0x40, 1, 0xA);
        EmApceros_PlaySeOnFrame(em, 0x60, 1, 6);
        EmApceros_PlaySeOnFrame(em, 0x8A, 4, 0x14);
        EmApceros_PlaySeOnFrame(em, 0xAE, 4, 0x17);
        break;
    case EM_MOTION_ID(0, APC_MOT_ALERT):
        EmApceros_PlaySeOnFrame(em, 0x46, 7, 0x10);
        EmApceros_PlaySeOnFrame(em, 0x46, 4, 0x14);
        EmApceros_PlaySeOnFrame(em, 0x20, 4, 0x17);
        EmApceros_PlaySeOnFrame(em, 0x36, 2, 6);
        EmApceros_PlaySeOnFrame(em, 0x18, 2, 0xA);
        break;
    case EM_MOTION_ID(0, APC_MOT_ATTACK_REAR):
        EmApceros_PlaySeOnFrame(em, 4, 0xE, 0);
        EmApceros_PlaySeOnFrame(em, 0x2E, 0x14, 0x1B);
        EmApceros_PlaySeOnFrame(em, 0x3C, 7, 0x10);
        EmApceros_PlaySeOnFrame(em, 0x34, 4, 0x14);
        EmApceros_PlaySeOnFrame(em, 0x7A, 1, 0x17);
        EmApceros_PlaySeOnFrame(em, 0xA6, 5, 0x14);
        EmApceros_PlaySeOnFrame(em, 0xD2, 2, 0xA);
        break;
    case EM_MOTION_ID(0, APC_MOT_WATCH):
        EmApceros_PlaySeOnFrame(em, 0x46, 0x13, 0x10);
        EmApceros_PlaySeOnFrame(em, 0xE6, 0x13, 0x10);
        EmApceros_PlaySeOnFrame(em, 0xCE, 4, 0x14);
        EmApceros_PlaySeOnFrame(em, 0x144, 4, 0x14);
        EmApceros_PlaySeOnFrame(em, 0x38, 4, 0x17);
        EmApceros_PlaySeOnFrame(em, 0xA2, 4, 0x17);
        EmApceros_PlaySeOnFrame(em, 0x4C, 2, 6);
        EmApceros_PlaySeOnFrame(em, 0x7A, 2, 6);
        EmApceros_PlaySeOnFrame(em, 0xFC, 2, 6);
        EmApceros_PlaySeOnFrame(em, 0x128, 2, 6);
        EmApceros_PlaySeOnFrame(em, 0x62, 2, 0xA);
        EmApceros_PlaySeOnFrame(em, 0x90, 2, 0xA);
        EmApceros_PlaySeOnFrame(em, 0xE6, 2, 0xA);
        EmApceros_PlaySeOnFrame(em, 0x118, 2, 0xA);
        break;
    case EM_MOTION_ID(0, APC_MOT_WATCH_TURN_A):
    case EM_MOTION_ID(0, APC_MOT_WATCH_TURN_B):
        EmApceros_PlaySeOnFrame(em, 0x2C, 4, 0x14);
        EmApceros_PlaySeOnFrame(em, 0x70, 4, 0x14);
        EmApceros_PlaySeOnFrame(em, 0x1C, 4, 0x17);
        EmApceros_PlaySeOnFrame(em, 0x5C, 4, 0x17);
        EmApceros_PlaySeOnFrame(em, 0x3A, 2, 6);
        EmApceros_PlaySeOnFrame(em, 0x7C, 2, 6);
        EmApceros_PlaySeOnFrame(em, 0x26, 2, 0xA);
        EmApceros_PlaySeOnFrame(em, 0x4A, 2, 0xA);
        break;
    case EM_MOTION_ID(0, APC_MOT_ALERT_END):
        EmApceros_PlaySeOnFrame(em, 0x2A, 4, 0x14);
        EmApceros_PlaySeOnFrame(em, 0x38, 1, 6);
        EmApceros_PlaySeOnFrame(em, 0x5A, 4, 0x17);
        EmApceros_PlaySeOnFrame(em, 0x6A, 1, 0xA);
        EmApceros_PlaySeOnFrame(em, 0x5A, 0xA, 0x10);
        break;
    case EM_MOTION_ID(0, APC_MOT_ATTACK_B):
        EmApceros_PlaySeOnFrame(em, 0x1A, 0xE, 0x14);
        EmApceros_PlaySeOnFrame(em, 0x1A, 0x14, 0x10);
        EmApceros_PlaySeOnFrame(em, 0x14, 0x13, 0x10);
        EmApceros_PlaySeOnFrame(em, 0x14, 3, 6);
        EmApceros_PlaySeOnFrame(em, 0x18, 6, 0x17);
        EmApceros_PlaySeOnFrame(em, 0x32, 3, 6);
        EmApceros_PlaySeOnFrame(em, 0x44, 4, 0x14);
        break;
    case EM_MOTION_ID(0, APC_MOT_SPIN_ATTACK):
        EmApceros_PlaySeOnFrame(em, 0x14, 7, 0x10);
        EmApceros_PlaySeOnFrame(em, 0x5C, 7, 0x10);
        EmApceros_PlaySeOnFrame(em, 0x1E, 0x14, 0x1B);
        EmApceros_PlaySeOnFrame(em, 0x5C, 0x14, 0x1B);
        EmApceros_PlaySeOnFrame(em, 0xA, 4, 0x14);
        EmApceros_PlaySeOnFrame(em, 0x50, 4, 0x14);
        EmApceros_PlaySeOnFrame(em, 0x1E, 4, 0x17);
        EmApceros_PlaySeOnFrame(em, 0x5E, 4, 0x17);
        EmApceros_PlaySeOnFrame(em, 0x2C, 1, 0xA);
        EmApceros_PlaySeOnFrame(em, 0x74, 1, 0xA);
        EmApceros_PlaySeOnFrame(em, 0x18, 1, 6);
        EmApceros_PlaySeOnFrame(em, 0x42, 1, 6);
        break;
    case EM_MOTION_ID(0, APC_MOT_FLINCH_B):
        EmApceros_PlaySeOnFrame(em, 4, 0x11, 0x10);
        EmApceros_PlaySeOnFrame(em, 0x10, 1, 6);
        EmApceros_PlaySeOnFrame(em, 0x1A, 1, 0xA);
        EmApceros_PlaySeOnFrame(em, 0x68, 0xE, 0x1B);
        EmApceros_PlaySeOnFrame(em, 0x6E, 1, 0xA);
        EmApceros_PlaySeOnFrame(em, 0x8C, 1, 0x17);
        break;
    case EM_MOTION_ID(0, APC_MOT_FLINCH_A):
        EmApceros_PlaySeOnFrame(em, 4, 0x11, 0x10);
        EmApceros_PlaySeOnFrame(em, 0x10, 1, 0xA);
        EmApceros_PlaySeOnFrame(em, 0x1A, 1, 6);
        EmApceros_PlaySeOnFrame(em, 0x68, 0xE, 0x1B);
        EmApceros_PlaySeOnFrame(em, 0x6E, 1, 6);
        EmApceros_PlaySeOnFrame(em, 0x8C, 1, 0x14);
        break;
    case EM_MOTION_ID(0, APC_MOT_FALL):
        EmApceros_PlaySeOnFrame(em, 4, 0x11, 0x10);
        EmApceros_PlaySeOnFrame(em, 0x1C, 0x12, 0);
        EmApceros_PlaySeOnFrame(em, 0x22, 0xF, 0);
        EmApceros_PlaySeOnFrame(em, 0x44, 3, 0xA);
        EmApceros_PlaySeOnFrame(em, 0x50, 4, 0x17);
        break;
    case EM_MOTION_ID(0, APC_MOT_LIE):
        EmApceros_PlaySeOnFrame(em, 0x14, 0x10, 0x10);
        EmApceros_PlaySeOnFrame(em, 4, 0x16, 0x10);
        EmApceros_PlaySeOnFrame(em, 0x64, 0x15, 0);
        EmApceros_PlaySeOnFrame(em, 0xC2, 0x15, 0);
        EmApceros_PlaySeOnFrame(em, 0x32, 0xE, 0x14);
        EmApceros_PlaySeOnFrame(em, 0x96, 0xE, 0x14);
        break;
    case EM_MOTION_ID(0, APC_MOT_DIE):
        EmApceros_PlaySeOnFrame(em, 6, 8, 0x10);
        EmApceros_PlaySeOnFrame(em, 0x42, 8, 0x10);
        break;
    case EM_MOTION_ID(0, APC_MOT_GET_UP):
        EmApceros_PlaySeOnFrame(em, 0xA, 0xE, 6);
        EmApceros_PlaySeOnFrame(em, 0x14, 0xA, 0x10);
        EmApceros_PlaySeOnFrame(em, 0x30, 3, 6);
        EmApceros_PlaySeOnFrame(em, 0x4E, 5, 0x14);
        EmApceros_PlaySeOnFrame(em, 0x64, 4, 0x17);
        EmApceros_PlaySeOnFrame(em, 0x82, 1, 6);
        break;
    case EM_MOTION_ID(0, APC_MOT_FALL_B):
        EmApceros_PlaySeOnFrame(em, 0x1A, 1, 6);
        EmApceros_PlaySeOnFrame(em, 0x1A, 0xE, 6);
        EmApceros_PlaySeOnFrame(em, 0x18, 0xE, 0x17);
        EmApceros_PlaySeOnFrame(em, 0x54, 0x12, 0);
        EmApceros_PlaySeOnFrame(em, 0x60, 0xF, 0);
        EmApceros_PlaySeOnFrame(em, 0x7E, 3, 6);
        EmApceros_PlaySeOnFrame(em, 0x8C, 4, 0x14);
        break;
    case EM_MOTION_ID(0, APC_MOT_STATUS2_LIE):
        EmApceros_PlaySeOnFrame(em, 2, 9, 0x10);
        EmApceros_PlaySeOnFrame(em, 0xB4, 9, 0x10);
        break;
    case EM_MOTION_ID(0, APC_MOT_STATUS4):
        EmApceros_PlaySeOnFrame(em, 6, 0x10, 0x10);
        break;
    }
}

/*
 * Sets the monster up (Start, Reinit): stage-0 test position, motion row 1,
 * stance 0, hp from Em_ComputeMaxHp (kind 0x0C: index 1 for variants 0 and
 * 5), size per kind (kind 0x0C: EmApceros_ScalesType0C[variant]; Popo:
 * EmApceros_ScalesType46[variant]; Apceros: 1.0) copied to bodyScale and
 * applied (GmEm_ApplySize); keeps the spawn position in home and clears
 * objListed and alarmed.
 */
void EmApceros_Init(Em *em)
{
    EmApcerosWork *w = (EmApcerosWork *)&em->work;
    s16 hp;

    if (gQuest.stage == 0) {
        em->posX = 2400.0f + 250.0f * (f32) (u32) em->slotNo;
        em->posY = 0.0f;
        em->posZ = 3600.0f;
    }
    Em_SetMotionRow(em, APC_MOT_STAND, 0, 0);
    em->stance = EM_STANCE_GROUND;
    if (em->type == 0xC) {
        if (em->variant == 0 || em->variant == 5) {
            hp = Em_ComputeMaxHp(em, 1);
            em->hp = hp;
            em->hpMax = hp;
        } else {
            hp = Em_ComputeMaxHp(em, 0);
            em->hp = hp;
            em->hpMax = hp;
        }
        em->scaleX = EmApceros_ScalesType0C[em->variant];
        em->scaleY = EmApceros_ScalesType0C[em->variant];
        em->scaleZ = EmApceros_ScalesType0C[em->variant];
    } else if (em->type == EM_KIND_POPO) {
        hp = Em_ComputeMaxHp(em, 0);
        em->hp = hp;
        em->hpMax = hp;
        em->scaleX = EmApceros_ScalesType46[em->variant];
        em->scaleY = EmApceros_ScalesType46[em->variant];
        em->scaleZ = EmApceros_ScalesType46[em->variant];
    } else {
        hp = Em_ComputeMaxHp(em, 0);
        em->hp = hp;
        em->hpMax = hp;
        em->scaleX = 1.0f;
        em->scaleY = 1.0f;
        em->scaleZ = 1.0f;
    }
    em->bodyScale = em->scaleX;
    GmEm_ApplySize(em);
    em->timer = 0;
    em->scriptPending = 1;
    em->blind = 0;
    w->home[0] = em->posX;
    w->home[1] = em->posY;
    w->home[2] = em->posZ;
    em->remote = 0;
    w->objListed = 0;
    w->alarmed = 0;
}

/* Init, then APC_IDLE_STAND. */
void EmApceros_Start(Em *em)
{
    EmApceros_Init(em);
    EmApceros_ChangeState(em, EM_GROUP_IDLE, APC_IDLE_STAND, EM_SYNC_LOCAL);
}

/* Init only (callbacks slot 5). */
void EmApceros_Reinit(Em *em)
{
    EmApceros_Init(em);
}

/*
 * Moves to (group, state) with Em_SetStateSync after the entry work:
 * APC_MOVE_WANDER_HOME aims the goal at home; APC_MOVE_RUN and
 * APC_MOVE_WALK set the speed (Em_SetSpeedScaled) and cap goalDist at 5000;
 * APC_MOVE_WARP puts the position (and prevPos) on the goal;
 * APC_DEFEAT_WAIT_REAPPEAR makes the monster reappear (QuestEm_Appear) when
 * QuestEm_CanAppear allows.
 */
void EmApceros_ChangeState(Em *em, u16 group, u16 state, u8 mode)
{
    EmApcerosWork *w = (EmApcerosWork *)&em->work;

    switch (group) {
    case EM_GROUP_MOVE:
        switch (state) {
        case APC_MOVE_WANDER_HOME:
            Vec_Copy(em->goal, w->home);
            break;
        case APC_MOVE_TURN_TO_GOAL:
            break;
        case APC_MOVE_RUN:
        case APC_MOVE_WALK:
            Em_SetSpeedScaled(em, 2, 1000.0f, 200.0f);
            if (em->goalDist > 5000.0f) {
                em->goalDist = 5000.0f;
            }
            break;
        case APC_MOVE_WARP:
            Vec_Copy(&em->posX, em->goal);
            Vec_Copy(&em->prevPosX, em->goal);
            break;
        }
        break;
    case EM_GROUP_DEFEAT:
        if (state == 3 && QuestEm_CanAppear(em) == 1) {
            QuestEm_Appear(em);
        }
        break;
    case EM_GROUP_IDLE:
    case EM_GROUP_FLY:
    case EM_GROUP_ATTACK:
    case EM_GROUP_REACT:
    case EM_GROUP_EVENT:
    case EM_GROUP_EVENT2:
    case EM_GROUP_VERSUS:
        break;
    }
    Em_SetStateSync(em, group, state, mode);
}

/*
 * Ends a state: scriptPending 1, stance 0, then APC_IDLE_WATCH after
 * APC_IDLE_ALERT, APC_IDLE_WATCH, APC_MOVE_WATCH_TURN_BACK,
 * APC_MOVE_TURN_BACK or APC_ATK_SPIN, else APC_IDLE_STAND.
 */
void EmApceros_EndState(Em *em)
{
    em->scriptPending = 1;
    em->stance = EM_STANCE_GROUND;
    if ((em->stateGroup == EM_GROUP_IDLE && em->state == APC_IDLE_ALERT) || (em->stateGroup == EM_GROUP_IDLE && em->state == APC_IDLE_WATCH) ||
        (em->stateGroup == EM_GROUP_MOVE && em->state == APC_MOVE_WATCH_TURN_BACK) || (em->stateGroup == EM_GROUP_MOVE && em->state == APC_MOVE_TURN_BACK) ||
        (em->stateGroup == EM_GROUP_ATTACK && em->state == APC_ATK_SPIN)) {
        EmApceros_ChangeState(em, EM_GROUP_IDLE, APC_IDLE_WATCH, EM_SYNC_LOCAL);
    } else {
        EmApceros_ChangeState(em, EM_GROUP_IDLE, APC_IDLE_STAND, EM_SYNC_LOCAL);
    }
}

/*
 * Em_SetRequest with priority 0x90, except that request 2 (the hurt/alarm
 * request) is dropped while alarmed is set.
 */
void EmApceros_SetRequest(Em *em, s32 request)
{
    EmApcerosWork *w = (EmApcerosWork *)&em->work;

    switch ((u8) request) {
    case 2:
        if (w->alarmed != 0) {
            return;
        }
    }
    Em_SetRequest(em, request, 0x90);
}

/*
 * Popo only: a herd member (variant other than 2) sets alarmed when a
 * visible Popo leader (variant 2) in its area is in group 4 (react) or 5
 * (defeat).
 */
void EmApceros_CheckHerdHurt(Em *em)
{
    EmApcerosWork *w = (EmApcerosWork *)&em->work;
    Em *other;
    s16 i;

    if (em->type == EM_KIND_POPO && em->variant != 2) {
        for (i = 0, other = gEms; i < 20; i++, other++) {
            if (em != other && other->active && other->visible && (u8) Obj_SameArea(em, (Player *) other)) {
                if (other->type == 0x46 && other->variant == 2 && (other->stateGroup == EM_GROUP_REACT || other->stateGroup == EM_GROUP_DEFEAT)) {
                    w->alarmed = 1;
                    return;
                }
            }
        }
    }
}

/* Does nothing: the family has no group 2 (fly) states. */
void EmApceros_FlyNone(void)
{
}

/*
 * APC_IDLE_STAND: plays APC_MOT_STAND (unless already playing) once, then
 * EndState.
 */
void EmApceros_IdleStand(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        if (em->motionId[0] != EM_MOTION_ID(0, APC_MOT_STAND)) {
            Em_SetMotion(em, APC_MOT_STAND, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmApceros_EndState(em);
        }
        break;
    }
}

/*
 * APC_IDLE_CIRCLE_HOME: walks (APC_MOT_WALK) for 120 to 210 frames turning
 * 0x40 a frame, the side chosen from the heading to home (read through the
 * work at +0x10), so it circles near home; then EndState.
 */
void EmApceros_IdleCircleHome(Em *em)
{
    s32 turnStep;
    f32 homePos[3];

    Vec_Copy(homePos, ((EmApcerosWork *)&em->work)->home);
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = (em->stateRand & 3) * 30 + 120;
        if (em->motionId[0] != EM_MOTION_ID(0, APC_MOT_WALK)) {
            Em_SetMotion(em, APC_MOT_WALK, 0, 0);
        }
        if ((u16) ((u16) (Math_Heading(em->posX, em->posZ, homePos[0], homePos[2]) + 0x4000) - em->rotY) <= 0x8000U) {
            em->stateVar1 = 0;
        } else {
            em->stateVar1 = 1;
        }
        break;
    case 1:
        if (em->motion[0].blend == 0) {
            if (--em->timer <= 0) {
                EmApceros_EndState(em);
            } else {
                if (em->stateVar1) {
                    turnStep = 0x40;
                } else {
                    turnStep = -0x40;
                }
                em->rotY += turnStep;
                em->rotY = (u16) em->rotY;
                Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
            }
        }
        break;
    }
}

/*
 * APC_IDLE_LOOP_A: loops APC_MOT_IDLE_LOOP_A for 360 to 570 frames
 * (stateRand), then EndState.
 */
void EmApceros_IdleLoopA(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = ((em->stateRand & 7) * 0x1E) + 0x168;
        if (em->motionId[0] != EM_MOTION_ID(0, APC_MOT_IDLE_LOOP_A)) {
            Em_SetMotion(em, APC_MOT_IDLE_LOOP_A, 0, 0);
        }
        break;
    case 1:
        if (--em->timer <= 0) {
            EmApceros_EndState(em);
        }
        break;
    }
}

/*
 * APC_IDLE_STOMP: APC_MOT_REAR_UP then APC_MOT_STOMP (ground effect, camera
 * shake and attack 0xD in MotionEffects), then EndState.
 */
void EmApceros_IdleStomp(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 0;
        if (em->motionId[0] != EM_MOTION_ID(0, APC_MOT_REAR_UP)) {
            Em_SetMotion(em, APC_MOT_REAR_UP, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            Em_SetMotion(em, APC_MOT_STOMP, 0, 0);
            em->step++;
            em->timer = 0;
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmApceros_EndState(em);
        }
        break;
    }
}

/*
 * APC_IDLE_LOOP_B: loops APC_MOT_IDLE_LOOP_B for 180 to 270 frames, then
 * EndState.
 */
void EmApceros_IdleLoopB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = ((em->stateRand & 3) * 0x1E) + 0xB4;
        Em_SetMotion(em, APC_MOT_IDLE_LOOP_B, 0, 0);
        break;
    case 1:
        if (--em->timer <= 0) {
            EmApceros_EndState(em);
        }
        break;
    }
}

/* APC_IDLE_ACTION_A: plays APC_MOT_IDLE_ACTION_A once, then EndState. */
void EmApceros_IdleActionA(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, APC_MOT_IDLE_ACTION_A, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmApceros_EndState(em);
        }
        break;
    }
}

/* APC_IDLE_ACTION_B: plays APC_MOT_IDLE_ACTION_B once, then EndState. */
void EmApceros_IdleActionB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, APC_MOT_IDLE_ACTION_B, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmApceros_EndState(em);
        }
        break;
    }
}

/*
 * APC_IDLE_STATUS2_COLLAPSE: under status 2 (Em_BeginStatus2, hit react
 * mode WAKE, blind 2) falls over (APC_MOT_FALL, pushed sideways for 30
 * frames), lies (APC_MOT_STATUS2_LIE) until the timer set by
 * Em_SetStatus2Timer runs out, then APC_IDLE_STATUS2_END.
 */
void EmApceros_Status2Collapse(Em *em)
{
    f32 push[3];

    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, APC_MOT_FALL, 0, 0);
        Em_BeginStatus2(em);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 30.0f) == 0 && em->motion[0].blend == 0) {
            push[0] = 21.428572f * em->scaleX;
            push[1] = 0.0f;
            push[2] = 0.0f;
            Vec_RotateY(push, 6.2831855f * em->rotY / 65536.0f);
            em->posX += push[0];
            em->posZ += push[2];
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, APC_MOT_STATUS2_LIE, 0, 0);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            EmApceros_ChangeState(em, EM_GROUP_IDLE, APC_IDLE_STATUS2_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * APC_IDLE_STATUS2_END: ends status 2 (Em_EndStatus2) and gets up
 * (APC_MOT_GET_UP), blind until done, then EndState.
 */
void EmApceros_Status2End(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, APC_MOT_GET_UP, 0, 0);
        em->blind = 2;
        Em_EndStatus2(em);
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            EmApceros_EndState(em);
        }
        break;
    }
}

/*
 * APC_IDLE_ALERT: plays APC_MOT_ALERT once, then EndState (to
 * APC_IDLE_WATCH).
 */
void EmApceros_IdleAlert(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, APC_MOT_ALERT, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmApceros_EndState(em);
        }
        break;
    }
}

/* APC_IDLE_ALERT_END: plays APC_MOT_ALERT_END once, then EndState. */
void EmApceros_IdleAlertEnd(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, APC_MOT_ALERT_END, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmApceros_EndState(em);
        }
        break;
    }
}

/*
 * APC_IDLE_LEAVE: runs (APC_MOT_RUN) unhittable while fading out over 100
 * frames, then takes its sense object off the sensed-objects list, leaves the quest
 * (QuestEm_Leave), hides and advances taskPhase.
 */
void EmApceros_RunOffAndLeave(Em *em)
{
    EmApcerosWork *w = (EmApcerosWork *)&em->work;

    em->noHitTimer = 0xA;
    em->noPushTimer = 0xA;
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 0x64;
        if (em->motionId[0] != EM_MOTION_ID(0, APC_MOT_RUN)) {
            Em_SetMotion(em, APC_MOT_RUN, 0, 0);
        }
        break;
    case 1:
        em->timer -= 1;
        em->alpha = (f32) em->timer / 100.0f;
        if (em->timer <= 0) {
            if (w->objListed == 1) {
                ObjListA70_Remove(&w->sense);
                w->objListed = 0U;
            }
            QuestEm_Leave(em);
            em->taskPhase++;
            em->visible = 0;
        }
        break;
    }
}

/*
 * APC_IDLE_WATCH: plays APC_MOT_WATCH once, then EndState (to
 * APC_IDLE_STAND).
 */
void EmApceros_IdleWatch(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, APC_MOT_WATCH, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmApceros_EndState(em);
        }
        break;
    }
}

/*
 * Group 0 dispatcher: keeps the state in prevStateGroup/prevState and runs
 * the idle state.
 */
void EmApceros_RunIdleState(Em *em)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->state) {
    case APC_IDLE_STAND:
        EmApceros_IdleStand(em);
        break;
    case APC_IDLE_CIRCLE_HOME:
        EmApceros_IdleCircleHome(em);
        break;
    case APC_IDLE_LOOP_A:
        EmApceros_IdleLoopA(em);
        break;
    case APC_IDLE_STOMP:
        EmApceros_IdleStomp(em);
        break;
    case APC_IDLE_LOOP_B:
        EmApceros_IdleLoopB(em);
        break;
    case APC_IDLE_ACTION_A:
        EmApceros_IdleActionA(em);
        break;
    case APC_IDLE_ACTION_B:
        EmApceros_IdleActionB(em);
        break;
    case APC_IDLE_STATUS2_COLLAPSE:
        EmApceros_Status2Collapse(em);
        break;
    case APC_IDLE_STATUS2_END:
        EmApceros_Status2End(em);
        break;
    case APC_IDLE_ALERT:
        EmApceros_IdleAlert(em);
        break;
    case APC_IDLE_ALERT_END:
        EmApceros_IdleAlertEnd(em);
        break;
    case APC_IDLE_LEAVE:
        EmApceros_RunOffAndLeave(em);
        break;
    case APC_IDLE_WATCH:
        EmApceros_IdleWatch(em);
        
    case APC_IDLE_NONE:
    default:
        break;
    }
}

/* Group 2 dispatcher: calls EmApceros_FlyNone (no states). */
void EmApceros_RunFlyState(void)
{
    EmApceros_FlyNone();
}

/*
 * APC_MOVE_WANDER_HOME: as APC_IDLE_CIRCLE_HOME but around the goal
 * (ChangeState set it to home), 210 to 300 frames of walking in a curve,
 * then EndState.
 */
void EmApceros_WanderHome(Em *em)
{
    s32 turnStep;

    switch (em->step) {
    case 0:
        em->step++;
        em->timer = (em->stateRand & 3) * 30 + 210;
        if (em->motionId[0] != EM_MOTION_ID(0, APC_MOT_WALK)) {
            Em_SetMotion(em, APC_MOT_WALK, 0, 0);
        }
        if ((u16) ((u16) (Math_Heading(em->posX, em->posZ, em->goal[0], em->goal[2]) + 0x4000) - em->rotY) <= 0x8000U) {
            em->stateVar1 = 0;
        } else {
            em->stateVar1 = 1;
        }
        break;
    case 1:
        if (em->motion[0].blend == 0) {
            if (--em->timer <= 0) {
                EmApceros_EndState(em);
            } else {
                if (em->stateVar1) {
                    turnStep = 0x40;
                } else {
                    turnStep = -0x40;
                }
                em->rotY += turnStep;
                em->rotY = (u16) em->rotY;
                Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
            }
        }
        break;
    }
}

/*
 * APC_MOVE_WATCH_HUNTER: raises its head (APC_MOT_ALERT) and watches the
 * nearest hunter (Em_FindTargetPlayer): turns its back toward a hunter in
 * front (APC_MOT_WATCH_TURN_A or _B, or 0x42 a frame while watching),
 * attacks one standing behind it (APC_ATK_REAR), and ends with
 * APC_MOT_ALERT_END when none is left in range.
 */
void EmApceros_WatchHunter(Em *em)
{
    EmApcerosWork *w = (EmApcerosWork *)&em->work;
    s32 targetSide;
    s32 targetSide2;

    switch (em->step) {
    case 0:
        em->step++;
        if (em->motionId[0] != EM_MOTION_ID(0, APC_MOT_ALERT)) {
            em->targetDist = 400.0f;
        }
        Em_SetMotion(em, APC_MOT_ALERT, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            if (Em_FindTargetPlayer(em, 0x78E4, 700.0f, 300.0f) != -1) {
                targetSide = (em->targetAngle - em->rotY) & 0xFFFF;
                if (targetSide <= 0x2FFF) {
                    em->step = 4;
                    Em_SetMotion(em, APC_MOT_WATCH_TURN_B, 0, 0);
                    em->stateVar1 = 0;
                    w->turnOn = 0;
                } else if (targetSide > 0xD000) {
                    em->step = 4;
                    Em_SetMotion(em, APC_MOT_WATCH_TURN_A, 0, 0);
                    em->stateVar1 = 1;
                    w->turnOn = 0;
                } else if (targetSide > 0x5400 && targetSide < 0xA600) {
                    EmApceros_ChangeState(em, EM_GROUP_ATTACK, APC_ATK_REAR, EM_SYNC_OWNER_FLAGGED);
                } else {
                    em->step++;
                    Em_SetMotion(em, APC_MOT_WATCH, 0, 0);
                }
            } else {
                EmApceros_EndState(em);
            }
        }
        break;
    case 2:
        if (em->motion[0].blend == 0) {
            em->step++;
            em->timer = (em->stateRand & 3) * 30 + 210;
        }
        break;
    case 3:
        if (Em_FindTargetPlayer(em, 0x8000, 500.0f, 200.0f) != -1) {
            targetSide2 = (em->targetAngle - em->rotY) & 0xFFFF;
            if (targetSide2 > 0x5400 && targetSide2 < 0xA600) {
                EmApceros_ChangeState(em, EM_GROUP_ATTACK, APC_ATK_REAR, EM_SYNC_OWNER_FLAGGED);
                break;
            }
            em->timer--;
            if (EmMotion_ReachedFrame(em, 0, 14.0f) != 0) {
                if (targetSide2 < 0x8000) {
                    if (em->timer <= 0) {
                        em->step++;
                        Em_SetMotion(em, APC_MOT_WATCH_TURN_B, 0, 0);
                        em->stateVar1 = 0;
                        w->turnOn = 0;
                    }
                    em->rotY -= 0x42;
                } else {
                    if (em->timer <= 0) {
                        em->step++;
                        Em_SetMotion(em, APC_MOT_WATCH_TURN_A, 0, 0);
                        em->stateVar1 = 1;
                        w->turnOn = 0;
                    }
                    em->rotY += 0x42;
                }
            }
        } else {
            em->step = 5;
            Em_SetMotion(em, APC_MOT_ALERT_END, 0, 0);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->step = 2;
            Em_SetMotion(em, APC_MOT_WATCH, 0, 0);
            break;
        }
        if (EmMotion_PassedFrameScaled(em, 6.0f, 0) != 0) {
            w->turnOn++;
        } else if (EmMotion_PassedFrameScaled(em, 122.0f, 0) != 0) {
            w->turnOn = 0;
        }
        if (w->turnOn != 0) {
            if (em->stateVar1 == 0) {
                em->rotY -= 0x1D7;
            } else {
                em->rotY += 0x1D7;
            }
        }
        break;
    case 5:
        if (em->motion[0].playing == 0) {
            EmApceros_EndState(em);
        }
        break;
    }
}

/*
 * APC_MOVE_WALK_TURN_A: APC_MOT_TURN_A, adding 0x100 a frame to rotY
 * between frames 42 and 170, then EndState.
 */
void EmApceros_WalkTurnA(Em *em)
{
    EmApcerosWork *w = (EmApcerosWork *)&em->work;

    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, APC_MOT_TURN_A, 0, 0);
        w->turnOn = 0;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmApceros_EndState(em);
            w->turnOn = 0;
            break;
        }
        if (EmMotion_PassedFrameScaled(em, 42.0f, 0) != 0) {
            w->turnOn++;
        } else if (EmMotion_PassedFrameScaled(em, 170.0f, 0) != 0) {
            w->turnOn = 0;
        }
        if (w->turnOn != 0) {
            em->rotY += 0x100;
        }
        break;
    }
}

/*
 * APC_MOVE_WALK_TURN_B: APC_MOT_TURN_B, taking 0x100 a frame from rotY
 * between frames 42 and 170, then EndState.
 */
void EmApceros_WalkTurnB(Em *em)
{
    EmApcerosWork *w = (EmApcerosWork *)&em->work;

    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, APC_MOT_TURN_B, 0, 0);
        w->turnOn = 0;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmApceros_EndState(em);
            w->turnOn = 0;
            break;
        }
        if (EmMotion_PassedFrameScaled(em, 42.0f, 0) != 0) {
            w->turnOn++;
        } else if (EmMotion_PassedFrameScaled(em, 170.0f, 0) != 0) {
            w->turnOn = 0;
        }
        if (w->turnOn != 0) {
            em->rotY -= 0x100;
        }
        break;
    }
}

/*
 * APC_MOVE_TURN_TO_GOAL: picks APC_MOT_TURN_A or _B from the side of the
 * goal and turns by a 66th of the angle to it a frame (frames 42 to 170),
 * then EndState.
 */
void EmApceros_TurnToGoal(Em *em)
{
    EmApcerosWork *w = (EmApcerosWork *)&em->work;

    switch (em->step) {
    case 0:
        em->step++;
        em->targetAngle = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        w->turnDelta = em->targetAngle - em->rotY;
        if ((u16)w->turnDelta < 0x8000) {
            Em_SetMotion(em, APC_MOT_TURN_A, 0, 0);
        } else {
            Em_SetMotion(em, APC_MOT_TURN_B, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmApceros_EndState(em);
        } else if (EmMotion_InFrameRange(em, 0, 42.0f, 170.0f) != 0) {
            em->rotY += (u16) (w->turnDelta / 66.0f);
        }
        break;
    }
}

/*
 * APC_MOVE_RUN: runs (APC_MOT_RUN) toward the goal, steering a 16th of the
 * angle a frame, counting goalDist down by the motion travel
 * (Em_MotionDelta), then EndState; a pending request adds 0x80 to collMask.
 */
void EmApceros_RunToGoal(Em *em)
{
    f32 travel[3];

    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 300;
        if (em->motionId[0] != EM_MOTION_ID(0, APC_MOT_RUN)) {
            Em_SetMotion(em, APC_MOT_RUN, 0, 0);
        }
        if (em->requested == 1) {
            em->collMask |= 0x80;
        }
        break;
    case 1:
        if (em->hasGoal != 0) {
            em->rotY += (s16) (u16) ((s16) Math_AngleXZ(&em->posX, em->goal) - em->rotY) / 16;
        }
        Em_MotionDelta(em, travel);
        em->goalDist -= travel[2];
        if (em->goalDist <= 0.0f) {
            em->step++;
            EmApceros_EndState(em);
        }
        break;
    }
}

/*
 * APC_MOVE_WATCH_TURN_BACK: plays APC_MOT_WATCH turning so its back faces
 * the goal (aimAngle = angle to the goal + 0x8000, spread over frames 14 to
 * 332), then EndState (to APC_IDLE_WATCH).
 */
void EmApceros_WatchTurnBack(Em *em)
{
    f32 turnF;
    u32 turn;

    switch (em->step) {
    case 0:
        em->step++;
        em->aimAngle = ((Math_AngleXZ(&em->posX, em->goal) & 0xFFFF) - em->rotY) + 0x7FFF + 1;
        Em_SetMotion(em, APC_MOT_WATCH, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmApceros_EndState(em);
            break;
        }
        if (EmMotion_InFrameRange(em, 0, 14.0f, 332.0f) != 0) {
            turnF = (f32) (s16) em->aimAngle / 159.0f;
            turn = (u32) turnF;
            em->rotY += (s32) turn & 0xFFFF;
        }
        break;
    }
}

/*
 * APC_MOVE_TURN_BACK: as APC_MOVE_WATCH_TURN_BACK with APC_MOT_WATCH_TURN_A
 * or _B by side (frames 6 to 122), then EndState (to APC_IDLE_WATCH).
 */
void EmApceros_TurnBackOnGoal(Em *em)
{
    f32 turnF;
    u32 turn;

    switch (em->step) {
    case 0:
        em->step++;
        em->aimAngle = ((Math_AngleXZ(&em->posX, em->goal) & 0xFFFF) - em->rotY) + 0x7FFF + 1;
        if ((s32) em->aimAngle < 0x8000) {
            Em_SetMotion(em, APC_MOT_WATCH_TURN_A, 0, 0);
            break;
        }
        Em_SetMotion(em, APC_MOT_WATCH_TURN_B, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmApceros_EndState(em);
            break;
        }
        if (EmMotion_InFrameRange(em, 0, 6.0f, 122.0f) != 0) {
            turnF = (f32) (s16) em->aimAngle / 58.0f;
            turn = (u32) turnF;
            em->rotY += (s32) turn & 0xFFFF;
        }
        break;
    }
}

/*
 * APC_MOVE_WALK: walks (APC_MOT_WALK) toward the goal, steering a 16th of
 * the angle a frame, until goalDist is used up, then EndState.
 */
void EmApceros_WalkToGoal(Em *em)
{
    f32 travel[3];

    switch (em->step) {
    case 0:
        em->step++;
        if (em->motionId[0] != EM_MOTION_ID(0, APC_MOT_WALK)) {
            Em_SetMotion(em, APC_MOT_WALK, 0, 0);
        }
        break;
    case 1:
        if (em->hasGoal != 0) {
            em->rotY += (s16) (u16) ((s16) Math_AngleXZ(&em->posX, em->goal) - em->rotY) / 16;
        }
        Em_MotionDelta(em, travel);
        em->goalDist -= travel[2];
        if (em->goalDist <= 0.0f) {
            em->step++;
            EmApceros_EndState(em);
        }
        break;
    }
}

/*
 * APC_MOVE_WARP: ChangeState has already placed it on the goal; ends at
 * once (EndState).
 */
void EmApceros_WarpToGoal(Em *em)
{
    u8 step;

    step = em->step;
    switch (step) {
    case 0:
        em->step = step + 1;
        EmApceros_EndState(em);
        break;
    }
}

/*
 * APC_MOVE_TURN_TO_TARGET: speedBonus 50, turns toward the target with
 * EmApceros_TurnParams (Em_StartTurnToTarget, Em_TurnStep), then EndState.
 */
void EmApceros_TurnToTarget(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->speedBonus = 0x32;
        Em_StartTurnToTarget(em, &EmApceros_TurnParams[0]);
        break;
    case 1:
        if (Em_TurnStep(em, &EmApceros_TurnParams[0]) == 1) {
            EmApceros_EndState(em);
        }
        break;
    }
}

/* Group 1 dispatcher. */
void EmApceros_RunMoveState(Em *em)
{
    switch (em->state) {
    case APC_MOVE_WANDER_HOME:
        EmApceros_WanderHome(em);
        break;
    case APC_MOVE_WATCH_HUNTER:
        EmApceros_WatchHunter(em);
        break;
    case APC_MOVE_WALK_TURN_A:
        EmApceros_WalkTurnA(em);
        break;
    case APC_MOVE_WALK_TURN_B:
        EmApceros_WalkTurnB(em);
        break;
    case APC_MOVE_TURN_TO_GOAL:
        EmApceros_TurnToGoal(em);
        break;
    case APC_MOVE_RUN:
        EmApceros_RunToGoal(em);
        break;
    case APC_MOVE_WATCH_TURN_BACK:
        EmApceros_WatchTurnBack(em);
        break;
    case APC_MOVE_TURN_BACK:
        EmApceros_TurnBackOnGoal(em);
        break;
    case APC_MOVE_WALK:
        EmApceros_WalkToGoal(em);
        break;
    case APC_MOVE_WARP:
        EmApceros_WarpToGoal(em);
        break;
    case APC_MOVE_TURN_TO_TARGET:
        EmApceros_TurnToTarget(em);
        
    default:
        break;
    }
}

/*
 * APC_ATK_REAR: APC_MOT_ATTACK_REAR (attack 9; kind 0x0C and Popo attack 1;
 * at frame 30), then EndState; entered from APC_MOVE_WATCH_HUNTER when the
 * hunter is behind.
 */
void EmApceros_AttackRear(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, APC_MOT_ATTACK_REAR, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmApceros_EndState(em);
        }
        break;
    }
}

/*
 * APC_ATK_B_LEAD: APC_MOT_IDLE_ACTION_B to frame 102, then
 * APC_MOT_ATTACK_B, then EndState.
 */
void EmApceros_AttackBWithLead(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, APC_MOT_IDLE_ACTION_B, 0, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 102.0f, 0U) != 0) {
            em->step++;
            Em_SetMotion(em, APC_MOT_ATTACK_B, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmApceros_EndState(em);
        }
        break;
    }
}

/*
 * APC_ATK_SPIN: APC_MOT_SPIN_ATTACK, turning 0x1C7 a frame until frame 128,
 * then EndState (to APC_IDLE_WATCH).
 */
void EmApceros_AttackSpin(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, APC_MOT_SPIN_ATTACK, 0, 0);
        break;
    case 1:
        if ((EmMotion_ReachedFrame(em, 0, 128.0f) == 0) && (em->motion[0].blend == 0)) {
            em->rotY += 0x1C7;
        }
        if (em->motion[0].playing == 0) {
            EmApceros_EndState(em);
        }
        break;
    }
}

/* APC_ATK_B: APC_MOT_ATTACK_B once, then EndState. */
void EmApceros_AttackB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, APC_MOT_ATTACK_B, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmApceros_EndState(em);
        }
        break;
    }
}

/*
 * APC_ATK_STOMP: APC_MOT_REAR_UP then APC_MOT_STOMP, then EndState (same as
 * APC_IDLE_STOMP).
 */
void EmApceros_AttackStomp(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, APC_MOT_REAR_UP, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, APC_MOT_STOMP, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmApceros_EndState(em);
        }
        break;
    }
}

/* Group 3 dispatcher. */
void EmApceros_RunAttackState(Em *em)
{
    switch (em->state) {
    case APC_ATK_REAR:
        EmApceros_AttackRear(em);
        break;
    case APC_ATK_B_LEAD:
        EmApceros_AttackBWithLead(em);
        break;
    case APC_ATK_SPIN:
        EmApceros_AttackSpin(em);
        break;
    case APC_ATK_B:
        EmApceros_AttackB(em);
        break;
    case APC_ATK_STOMP:
        EmApceros_AttackStomp(em);
        break;
    }
}

/*
 * APC_REACT_RESET_HP: restarts the script, plays APC_MOT_REACT_RESET, then
 * APC_IDLE_STAND and sets hp to 0x40; nothing in this file enters it.
 */
void EmApceros_ReactResetHp(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        if (em->motionId[0] != EM_MOTION_ID(0, APC_MOT_REACT_RESET)) {
            Em_SetMotion(em, APC_MOT_REACT_RESET, 0, 0);
        }
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmApceros_ChangeState(em, EM_GROUP_IDLE, APC_IDLE_STAND, EM_SYNC_LOCAL);
            em->hp = 0x40;
            break;
        }
        EmMotion_PassedFrameScaled(em, 2.0f, 0U);
        break;
    }
}

/*
 * APC_REACT_FLINCH_A: APC_MOT_FLINCH_A, pushed to -X for 22 frames, then
 * EndState (Update: hit side angle above 0x8000).
 */
void EmApceros_FlinchA(Em *em)
{
    f32 push[3];

    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, APC_MOT_FLINCH_A, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 22.0f) == 0 && em->motion[0].blend == 0) {
            push[0] = -10.909091f * em->scaleX;
            push[1] = 0.0f;
            push[2] = 0.0f;
            Vec_RotateY(push, 6.2831855f * em->rotY / 65536.0f);
            em->posX += push[0];
            em->posZ += push[2];
        }
        if (em->motion[0].playing == 0) {
            EmApceros_EndState(em);
        }
        break;
    }
}

/*
 * APC_REACT_FLINCH_B: APC_MOT_FLINCH_B, pushed to +X for 22 frames, then
 * EndState.
 */
void EmApceros_FlinchB(Em *em)
{
    EmApcerosWork *w = (EmApcerosWork *)&em->work;
    f32 push[3];

    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, APC_MOT_FLINCH_B, 0, 0);
        w->turnOn = 0;
        Em_RestartScript(em);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 22.0f) == 0 && em->motion[0].blend == 0) {
            push[0] = 10.909091f * em->scaleX;
            push[1] = 0.0f;
            push[2] = 0.0f;
            Vec_RotateY(push, 6.2831855f * em->rotY / 65536.0f);
            em->posX += push[0];
            em->posZ += push[2];
        }
        if (em->motion[0].playing == 0) {
            EmApceros_EndState(em);
        }
        break;
    }
}

/*
 * APC_REACT_KNOCK_DOWN_A, APC_REACT_KNOCK_DOWN_B (variant 0, 1): variant 0
 * staggers (APC_MOT_FLINCH_A) and falls (APC_MOT_FALL_B), variant 1 falls
 * (APC_MOT_FALL) pushed sideways; lies (APC_MOT_LIE) at least 200 frames,
 * gets up (APC_MOT_GET_UP), then EndState.
 */
void EmApceros_KnockDown(Em *em, s32 fallKind)
{
    f32 push[3];

    switch (em->step) {
    case 0:
        if (fallKind == 0) {
            em->step++;
            Em_SetMotion(em, APC_MOT_FLINCH_A, 0, 0);
        } else {
            em->step = 2;
            if (em->motionId[0] != EM_MOTION_ID(0, APC_MOT_FALL)) {
                Em_SetMotion(em, APC_MOT_FALL, 0, 0);
            }
        }
        Em_RestartScript(em);
        break;
    case 1:
        push[0] = -10.909091f * em->scaleX;
        push[1] = 0.0f;
        push[2] = 0.0f;
        Vec_RotateY(push, 6.2831855f * em->rotY / 65536.0f);
        em->posX += push[0];
        em->posZ += push[2];
        if (EmMotion_PassedFrameScaled(em, 12.0f, 0) == 0) {
            em->step = 2;
            Em_SetMotion(em, APC_MOT_FALL_B, 0, 0);
        }
        break;
    case 2:
        if (fallKind == 0) {
            if (EmMotion_ReachedFrame(em, 0, 8.0f) == 0 && em->motion[0].blend == 0) {
                push[0] = -10.909091f * em->scaleX;
                push[1] = 0.0f;
                push[2] = 0.0f;
                Vec_RotateY(push, 6.2831855f * em->rotY / 65536.0f);
                em->posX += push[0];
                em->posZ += push[2];
            }
        } else {
            if (EmMotion_ReachedFrame(em, 0, 30.0f) == 0 && em->motion[0].blend == 0) {
                push[0] = 21.428572f * em->scaleX;
                push[1] = 0.0f;
                push[2] = 0.0f;
                Vec_RotateY(push, 6.2831855f * em->rotY / 65536.0f);
                em->posX += push[0] * em->scaleX;
                em->posZ += push[2];
            }
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 200;
            Em_SetMotion(em, APC_MOT_LIE, 0, 0);
        }
        break;
    case 3:
        if (--em->timer <= 0) {
            if (EmMotion_PassedFrameScaled(em, 98.0f, 0) != 0) {
                em->step++;
                Em_SetMotion(em, APC_MOT_GET_UP, 0, 0);
            }
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            EmApceros_EndState(em);
        }
        break;
    }
}

/*
 * APC_REACT_FLINCH_RAND: flinches to a random side (APC_MOT_FLINCH_B or _A,
 * stateRand bit 0), makes request 4 unless one is pending, restarts the
 * script, then copies blindTimer to timer and ends.
 */
void EmApceros_FlinchRandom(Em *em)
{
    EmApcerosWork *w = (EmApcerosWork *)&em->work;
    f32 push[3];

    switch (em->step) {
    case 0:
        em->step++;
        if (em->stateRand & 1) {
            Em_SetMotion(em, APC_MOT_FLINCH_B, 0, 0);
            w->turnOn = 0;
        } else {
            Em_SetMotion(em, APC_MOT_FLINCH_A, 0, 0);
            w->turnOn = 1;
        }
        if (em->requested != 1) {
            EmApceros_SetRequest(em, 4);
        }
        Em_RestartScript(em);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 22.0f) == 0 && em->motion[0].blend == 0) {
            if (w->turnOn == 0) {
                push[0] = 10.909091f * em->scaleX;
            } else {
                push[0] = -10.909091f * em->scaleX;
            }
            push[1] = 0.0f;
            push[2] = 0.0f;
            Vec_RotateY(push, 6.2831855f * em->rotY / 65536.0f);
            em->posX += push[0];
            em->posZ += push[2];
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = em->blindTimer;
            EmApceros_EndState(em);
        }
        break;
    }
}

/*
 * APC_REACT_STATUS4: begins status 4 (Em_BeginStatus4, hit react mode
 * STATUS, noReact), flinches (APC_MOT_FLINCH_B), loops APC_MOT_STATUS4 with
 * the status effect (Em_SpawnStatus4Effect) until the timer
 * (Em_SetStatus4Timer), ends status 4 with a second flinch, then EndState.
 */
void EmApceros_Status4(Em *em)
{
    f32 push[3];

    switch (em->step) {
    case 0:
        em->step++;
        Em_RestartScript(em);
        Em_SetMotion(em, APC_MOT_FLINCH_B, 0, 0);
        Em_BeginStatus4(em);
        em->noReact = 1;
        Em_SetHitReactMode(em, EM_HIT_REACT_STATUS);
        break;
    case 1:
        Em_SetHitReactMode(em, EM_HIT_REACT_STATUS);
        if (EmMotion_ReachedFrame(em, 0, 22.0f) == 0 && em->motion[0].blend == 0) {
            push[0] = 10.909091f * em->scaleX;
            push[1] = 0.0f;
            push[2] = 0.0f;
            Vec_RotateY(push, 6.2831855f * em->rotY / 65536.0f);
            em->posX += push[0];
            em->posZ += push[2];
        }
        if (EmMotion_PassedFrameScaled(em, 30.0f, 0) != 0) {
            em->step++;
            Em_SetMotion(em, APC_MOT_STATUS4, 0, 0);
        }
        break;
    case 2:
        Em_SetHitReactMode(em, EM_HIT_REACT_STATUS);
        Em_SpawnStatus4Effect(em, 2);
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, APC_MOT_FLINCH_B, 6, 0x1E);
            Em_EndStatus4(em);
        }
        break;
    case 3:
        if (EmMotion_ReachedFrame(em, 0, 22.0f) == 0 && em->motion[0].blend == 0) {
            push[0] = 10.909091f * em->scaleX;
            push[1] = 0.0f;
            push[2] = 0.0f;
            Vec_RotateY(push, 6.2831855f * em->rotY / 65536.0f);
            em->posX += push[0] * em->scaleX;
            em->posZ += push[2];
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmApceros_EndState(em);
        }
        break;
    }
}

/*
 * Group 4 dispatcher: sets fieldCollide; APC_REACT_KNOCK_DOWN_A
 * and _B run KnockDown with variant 0 and 1.
 */
void EmApceros_RunReactState(Em *em)
{
    em->fieldCollide = 1;
    switch (em->state) {
    case APC_REACT_RESET_HP:
        EmApceros_ReactResetHp(em);
        break;
    case APC_REACT_FLINCH_A:
        EmApceros_FlinchA(em);
        break;
    case APC_REACT_FLINCH_B:
        EmApceros_FlinchB(em);
        break;
    case APC_REACT_KNOCK_DOWN_A:
        EmApceros_KnockDown(em, 0);
        break;
    case APC_REACT_KNOCK_DOWN_B:
        EmApceros_KnockDown(em, 1);
        break;
    case APC_REACT_FLINCH_RAND:
        EmApceros_FlinchRandom(em);
        break;
    case APC_REACT_STATUS4:
        EmApceros_Status4(em);
        
    default:
        break;
    }
}

/*
 * APC_DEFEAT_DIE_A, APC_DEFEAT_DIE_B (variant 0, 1): reports the kill
 * (QuestEm_Hunted, MsgEvent_Check), falls as KnockDown (or joins the fall,
 * lie or death motion already playing from the event states), lies 40
 * frames, dies (APC_MOT_DIE), freezes the motion at frame 50 and leaves a
 * carcass to carve (EmMine_Create, type 1 in training quests) for 2400
 * frames (kind 0x0C, Popo) or 1800 (Apceros), fades out over 110 frames and
 * goes to APC_DEFEAT_WAIT_REAPPEAR.
 */
void EmApceros_Die(Em *em, s32 fallKind)
{
    EmApcerosWork *w = (EmApcerosWork *)&em->work;
    f32 push[3];

    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        QuestEm_Hunted(em);
        MsgEvent_Check(0xC);
        switch (em->motionId[0]) {
        case 0x427:
        case 0x42B:
            em->step = 2;
            break;
        case 0x428:
            em->timer = 40;
            em->step = 3;
            break;
        case 0x429:
            em->timer = 30;
            em->step = 4;
            break;
        default:
            if (fallKind == 0) {
                em->step++;
                Em_SetMotion(em, APC_MOT_FLINCH_A, 0, 0);
            } else {
                em->step = 2;
                Em_SetMotion(em, APC_MOT_FALL, 0, 0);
            }
            break;
        }
        break;
    case 1:
        push[0] = -10.909091f * em->scaleX;
        push[1] = 0.0f;
        push[2] = 0.0f;
        Vec_RotateY(push, 6.2831855f * em->rotY / 65536.0f);
        em->posX += push[0];
        em->posZ += push[2];
        if (EmMotion_PassedFrameScaled(em, 12.0f, 0) == 0) {
            em->step = 2;
            Em_SetMotion(em, APC_MOT_FALL_B, 0, 0);
        }
        break;
    case 2:
        if (fallKind == 0) {
            if (EmMotion_ReachedFrame(em, 0, 8.0f) == 0 && em->motion[0].blend == 0) {
                push[0] = -10.909091f * em->scaleX;
                push[1] = 0.0f;
                push[2] = 0.0f;
                Vec_RotateY(push, 6.2831855f * em->rotY / 65536.0f);
                em->posX += push[0];
                em->posZ += push[2];
            }
        } else {
            if (EmMotion_ReachedFrame(em, 0, 30.0f) == 0 && em->motion[0].blend == 0) {
                push[0] = 21.428572f * em->scaleX;
                push[1] = 0.0f;
                push[2] = 0.0f;
                Vec_RotateY(push, 6.2831855f * em->rotY / 65536.0f);
                em->posX += push[0];
                em->posZ += push[2];
            }
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 40;
            Em_SetMotion(em, APC_MOT_LIE, 0, 0);
        }
        break;
    case 3:
        if (--em->timer <= 0) {
            if (EmMotion_PassedFrameScaled(em, 98.0f, 0) != 0) {
                em->step++;
                Em_SetMotion(em, APC_MOT_DIE, 0, 0);
            }
        }
        break;
    case 4:
        if (EmMotion_ReachedFrame(em, 0, 50.0f) != 0) {
            em->step++;
            *(s32 *) &em->motionSpeed = 0;
            if (em->type == 0xC || em->type == EM_KIND_POPO) {
                em->timer = 2400;
            } else {
                em->timer = 1800;
            }
            if (Quest_IsTraining() == 1) {
                EmMine_Create(em, 1);
            } else {
                EmMine_Create(em, 0);
            }
            em->fieldCollide = 0;
        }
        break;
    case 5:
        *(s32 *) &em->motionSpeed = 0;
        if (EmMine_Update(em) <= 0 || --em->timer <= 0) {
            EmMine_Remove(em);
            em->step++;
            em->timer = 110;
        }
        break;
    case 6:
        *(s32 *) &em->motionSpeed = 0;
        em->timer--;
        if (em->timer <= 99) {
            em->alpha = em->timer / 100.0f;
        }
        if (em->timer <= 0) {
            if (w->objListed == 1) {
                ObjListA70_Remove(&w->sense);
                w->objListed = 0;
            }
            em->visible = 0;
            EmApceros_ChangeState(em, EM_GROUP_DEFEAT, APC_DEFEAT_WAIT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * APC_DEFEAT_FADE: reports the kill and walks (APC_MOT_WALK) unhittable
 * while fading out over 100 frames, then hides and goes to
 * APC_DEFEAT_WAIT_REAPPEAR (no carcass).
 */
void EmApceros_DieFading(Em *em)
{
    EmApcerosWork *w = (EmApcerosWork *)&em->work;

    em->noHitTimer = 0xA;
    em->noPushTimer = 0xA;
    switch (em->step) {
    case 0:
        QuestEm_Hunted(em);
        MsgEvent_Check(0xC);
        em->step++;
        em->timer = 0x64;
        if (em->motionId[0] != EM_MOTION_ID(0, APC_MOT_WALK)) {
            Em_SetMotion(em, APC_MOT_WALK, 0, 0);
        }
        break;
    case 1:
        em->timer -= 1;
        em->alpha = (f32) em->timer / 100.0f;
        if (em->timer <= 0) {
            if (w->objListed == 1) {
                ObjListA70_Remove(&w->sense);
                w->objListed = 0U;
            }
            em->visible = 0;
            EmApceros_ChangeState(em, EM_GROUP_DEFEAT, APC_DEFEAT_WAIT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * APC_DEFEAT_WAIT_REAPPEAR: back to APC_IDLE_STAND when QuestEm_CanAppear
 * allows (ChangeState made it appear), else advances taskPhase.
 */
void EmApceros_WaitReappear(Em *em)
{
    switch (em->step) {
    case 0:
        if (QuestEm_CanAppear(em) == 1) {
            EmApceros_ChangeState(em, EM_GROUP_IDLE, APC_IDLE_STAND, EM_SYNC_OWNER_FLAGGED);
            break;
        }
        em->taskPhase++;
    case 1:
        break;
    }
}

/*
 * Group 5 dispatcher: keeps the monster unhittable and unpushable;
 * APC_DEFEAT_DIE_A and _B run Die with variant 0 and 1.
 */
void EmApceros_RunDefeatState(Em *em)
{
    em->noHitTimer = 0xA;
    em->noPushTimer = 0xA;
    switch (em->state) {
    case APC_DEFEAT_DIE_A:
        EmApceros_Die(em, 0);
        break;
    case APC_DEFEAT_DIE_B:
        EmApceros_Die(em, 1);
        break;
    case APC_DEFEAT_FADE:
        EmApceros_DieFading(em);
        break;
    case APC_DEFEAT_WAIT_REAPPEAR:
        EmApceros_WaitReappear(em);
        break;
    }
}

/*
 * Spawns Effect02 kind 3 near bone 2 (55 units back along X, spread at
 * random in Y and Z by scale), aimed along the (-1, -0.2, 0) direction of
 * the bone; the third argument is unused. Called every 4 frames by the
 * event fall, lie and death states.
 */
void EmApceros_SpawnEffect02AtBone2(Em *em, f32 scale, s32 unused)
{
    f32 mtx[16];
    f32 offset[3];
    f32 pos[3];
    u16 pitch;
    u16 yaw;

    offset[0] = -55.0f * em->scaleX;
    offset[1] = em->scaleY * (scale * (f32) ((Rand_Next(1) & 0x3F) - 0x20));
    offset[2] = em->scaleZ * (scale * (f32) ((Rand_Next(1) & 0x7F) - 0x20));
    Mtx_Copy(mtx, (f32 *)Em_Bone(em, 2));
    Vec_RotateSelf(offset, mtx);
    pos[0] = mtx[12] + offset[0];
    pos[1] = mtx[13] + offset[1];
    pos[2] = mtx[14] + offset[2];
    offset[0] = -1.0f;
    offset[1] = -0.2f;
    offset[2] = 0.0f;
    Vec_RotateSelf(offset, mtx);
    pitch = -(u16) (s32) (0.5f + (65536.0f * Math_Atan2(offset[1], Math_Sqrtf(offset[0] * offset[0] + offset[2] * offset[2]))) / 6.2831855f);
    yaw = (u16) (s32) (0.5f + (65536.0f * Math_Atan2(offset[0], offset[2])) / 6.2831855f);
    scale *= Effect_Scale();
    HitSpark_SpawnAtScaled(pitch, yaw, 3, pos, scale);
}

/* APC_EVENT_FLINCH: noReact, plays APC_MOT_FLINCH_B, then APC_EVENT_FALL. */
void EmApceros_EventFlinch(Em *em)
{
    em->noPushTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        if (em->motionId[0] != EM_MOTION_ID(0, APC_MOT_FLINCH_B)) {
            Em_SetMotion(em, APC_MOT_FLINCH_B, 0, 0);
        }
        em->noReact = 1;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmApceros_ChangeState(em, EM_GROUP_EVENT, APC_EVENT_FALL, EM_SYNC_LOCAL);
        }
        break;
    }
}

/*
 * APC_EVENT_FALL: Effect02 at bone 2 every 4 frames, falls over
 * (APC_MOT_FALL) pushed sideways, then stays.
 */
void EmApceros_EventFall(Em *em)
{
    f32 push[3];

    if (!(gScene.frame & 3)) {
        EmApceros_SpawnEffect02AtBone2(em, 0.8f, 0);
    }
    em->noPushTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        if (em->motionId[0] != EM_MOTION_ID(0, APC_MOT_FALL)) {
            Em_SetMotion(em, APC_MOT_FALL, 0, 0);
        }
        em->noReact = 1;
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 30.0f) == 0 && em->motion[0].blend == 0) {
            push[0] = 21.428572f * em->scaleX;
            push[1] = 0.0f;
            push[2] = 0.0f;
            Vec_RotateY(push, 6.2831855f * em->rotY / 65536.0f);
            em->posX += push[0];
            em->posZ += push[2];
        }
        if (em->motion[0].playing == 0) {
            em->step++;
        }
        break;
    case 2:
        break;
    }
}

/*
 * APC_EVENT_LIE: Effect02 every 4 frames, lies (APC_MOT_LIE) until the
 * scene's predator, a Rathian or Rathalos of any colour, has left its event
 * states (stateGroup not EM_GROUP_EVENT), then APC_EVENT_DIE.
 */
void EmApceros_EventLie(Em *em)
{
    u8 objKind;
    Em *predator;

    em->noPushTimer = 5;
    if (!(gScene.frame & 3)) {
        EmApceros_SpawnEffect02AtBone2(em, 0.6f, 0);
    }
    switch (em->step) {
    case 0:
        em->step++;
        if (em->motionId[0] != EM_MOTION_ID(0, APC_MOT_LIE)) {
            Em_SetMotion(em, APC_MOT_LIE, 0, 0);
        }
        em->noReact = 1;
        break;
    case 1:
        /* the scene's Rathian or Rathalos: a pointer kept over Em.unk898 and demonMode, which no Em field types */
        predator = *(Em **)&em->unk898;
        if (predator != NULL) {
            objKind = predator->type;
            if (((objKind == EM_KIND_RATHIAN) || (objKind == EM_KIND_PINK_RATHIAN) || (objKind == EM_KIND_RATHALOS) || ((u32) (objKind - EM_KIND_SILVER_RATHALOS) <= 1U) || (objKind == EM_KIND_AZURE_RATHALOS)) && (predator->stateGroup != EM_GROUP_EVENT)) {
                EmApceros_ChangeState(em, EM_GROUP_EVENT, APC_EVENT_DIE, EM_SYNC_LOCAL);
            }
        }
        break;
    }
}

/*
 * APC_EVENT_DIE: plays APC_MOT_DIE with Effect02, the motion frozen while
 * the predator is out of its event states; once EVENT_FLAG_RATHALOS_SCENE is set, hp 0
 * and APC_DEFEAT_DIE_A.
 */
void EmApceros_EventDie(Em *em)
{
    u8 objKind;
    Em *predator;

    em->noPushTimer = 5;
    em->scriptPending = 0;
    switch (em->step) {
    case 0:
        em->step++;
        if (em->motionId[0] != EM_MOTION_ID(0, APC_MOT_DIE)) {
            Em_SetMotion(em, APC_MOT_DIE, 0, 0);
        }
        em->noReact = 1;
        break;
    case 1:
        if (!(gScene.frame & 3)) {
            EmApceros_SpawnEffect02AtBone2(em, 0.3f, 0);
        }
        /* the scene's Rathian or Rathalos: a pointer kept over Em.unk898 and demonMode, which no Em field types */
        predator = *(Em **)&em->unk898;
        if (predator != NULL) {
            objKind = predator->type;
            if (((objKind == EM_KIND_RATHIAN) || (objKind == EM_KIND_PINK_RATHIAN) || (objKind == EM_KIND_RATHALOS) || ((u32) (objKind - EM_KIND_SILVER_RATHALOS) <= 1U) || (objKind == EM_KIND_AZURE_RATHALOS)) && (predator->stateGroup != EM_GROUP_EVENT)) {
                em->motionSpeed = 0.0f;
            }
            if (Save_TestEventFlag(EVENT_FLAG_RATHALOS_SCENE) == 1) {
                em->step++;
                em->hp = 0;
                em->motionSpeed = 1.0f;
                EmApceros_ChangeState(em, EM_GROUP_DEFEAT, APC_DEFEAT_DIE_A, EM_SYNC_OWNER);
            }
        }
        break;
    }
}

/*
 * APC_EVENT_INTRO_WAIT: stands (APC_MOT_STAND) until EVENT_FLAG_RATHALOS_SCENE is set
 * (the opening scene of the story quests seen), then EndState.
 */
void EmApceros_EventWaitIntro(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->motionSpeed = 1.0f;
        Em_SetMotion(em, APC_MOT_STAND, 0, 0);
        break;
    case 1:
        if (Save_TestEventFlag(EVENT_FLAG_RATHALOS_SCENE) == 1) {
            EmApceros_EndState(em);
        }
        break;
    }
}

/*
 * APC_EVENT_INTRO_PREY: resets the size and stands (APC_MOT_STAND) at a
 * fixed spot and heading of Forest and Hills area 3 for the opening scene
 * (Update sends variant 0 here during the opening scene of quests QUEST_ID_RATHALOS_SCENE_1 and
 * 0x436, the scene of RATH_EVENT_INTRO); stays.
 */
void EmApceros_EventIntroPrey(Em *em)
{
    em->noPushTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        GmEm_ResetSize(em);
        em->motionSpeed = 1.0f;
        Em_SetMotion(em, APC_MOT_STAND, 0, 0);
        em->posX = 17788.5f;
        em->posY = 0.0f;
        em->posZ = 17468.3f;
        em->rotY = 0x63FF;
        break;
    case 1:
        break;
    }
}

/*
 * APC_EVENT_RESHOW: resets size, stance and look-at, stands hidden for one
 * frame (scriptPending 0), then shows it and EndState.
 */
void EmApceros_EventReshow(Em *em)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        GmEm_ResetSize(em);
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, APC_MOT_STAND, 0, 0);
        em->visible = 0;
        em->scriptPending = 0;
        break;
    case 1:
        em->step++;
        em->visible = 1;
        EmApceros_EndState(em);
        break;
    }
}

/* Group 6 dispatcher. */
void EmApceros_RunEventState(Em *em)
{
    switch (em->state) {
    case APC_EVENT_FLINCH:
        EmApceros_EventFlinch(em);
        break;
    case APC_EVENT_FALL:
        EmApceros_EventFall(em);
        break;
    case APC_EVENT_LIE:
        EmApceros_EventLie(em);
        break;
    case APC_EVENT_DIE:
        EmApceros_EventDie(em);
        break;
    case APC_EVENT_INTRO_WAIT:
        EmApceros_EventWaitIntro(em);
        break;
    case APC_EVENT_INTRO_PREY:
        EmApceros_EventIntroPrey(em);
        break;
    case APC_EVENT_RESHOW:
        EmApceros_EventReshow(em);
        
    default:
        break;
    }
}

/*
 * APC_ENTER_RUN: runs (APC_MOT_RUN) while fading in (alpha +0.01 a frame)
 * for 100 frames, then EndState with full alpha and senseDelay 0.
 */
void EmApceros_EnterRunning(Em *em)
{

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, APC_MOT_RUN, 0, 0);
        em->timer = 0x64;
        break;
    case 1:
        em->alpha += 0.01f;
        if (!(em->alpha <= 1.0f)) {
            em->alpha = 1.0f;
        }
        if (--em->timer <= 0) {
            em->step++;
            EmApceros_EndState(em);
            em->senseDelay = 0;
            em->alpha = 1.0f;
        }
        break;
    }
}

/* APC_ENTER_WALK: as APC_ENTER_RUN, walking (APC_MOT_WALK). */
void EmApceros_EnterWalking(Em *em)
{

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, APC_MOT_WALK, 0, 0);
        em->timer = 0x64;
        break;
    case 1:
        em->alpha += 0.01f;
        if (!(em->alpha <= 1.0f)) {
            em->alpha = 1.0f;
        }
        if (--em->timer <= 0) {
            em->step++;
            EmApceros_EndState(em);
            em->senseDelay = 0;
            em->alpha = 1.0f;
        }
        break;
    }
}

/*
 * Group 7 dispatcher (the fade-in arrivals): keeps the monster unhittable,
 * unpushable and not sensing (senseDelay 5).
 */
void EmApceros_RunEnterState(Em *em)
{
    em->noHitTimer = 0xA;
    em->noPushTimer = 0xA;
    em->senseDelay = 5;
    switch (em->state) {
    case APC_ENTER_RUN:
        EmApceros_EnterRunning(em);
        break;
    case APC_ENTER_WALK:
        EmApceros_EnterWalking(em);
        break;
    }
}

/*
 * Per frame: CheckHerdHurt; senses alarm objects (Em_SenseObjects:
 * EM_TRIGGER_SENSED, request 2) and takes its own sense object off the list
 * a frame after adding it; outside groups 6 and 7 makes request 1 when a
 * map-shown monster in the area is in combat (or calm while a kind 0x0C or
 * Popo variant 0 is in group 6); maps the Em_UpdateDamage results to states
 * (kill: APC_DEFEAT_DIE_A or _B by hit side; status 4: APC_REACT_STATUS4;
 * status 2: APC_IDLE_STATUS2_COLLAPSE; hit while lying:
 * APC_IDLE_STATUS2_END; part flinch or hit: flinch or knock-down); When
 * hurt it alarms itself (Popo herd members: alarmed; others: request 2) and
 * lists its sense object (radius 2000, kind 2) for the herd to sense;
 * request 3 in block area 1; the opening-scene check of quests QUEST_ID_RATHALOS_SCENE_1 and
 * 0x436, else Em_UpdatePendingScript; then runs the state (twice if
 * stateRequest).
 */
void EmApceros_Update(Em *em)
{
    u8 hurt = 0;
    u8 popoInEvent = 0;
    u8 calmNear = 0;
    EmApcerosWork *w = (EmApcerosWork *)&em->work;
    Em *other = gEms;
    s16 i;
    u8 fightNear = 0;
    u16 hitSide;
    s32 sensed;
    u8 dmgInfo[4];

    EmApceros_CheckHerdHurt(em);
    sensed = Em_SenseObjects(em, &w->sense) & 0xFF;
    if (w->objListed == 0 && sensed != 0) {
        em->triggers |= 4;
        w->objListed = 2;
        if (em->requested != 1) {
            EmApceros_SetRequest(em, 2);
        }
    }
    if (w->objListed == 1) {
        ObjListA70_Remove(&w->sense);
        w->objListed = 2;
    }
    if (em->requested != 1 && em->stateGroup != EM_GROUP_EVENT && em->stateGroup != EM_GROUP_EVENT2) {
        for (i = 0; i < 20; i++, other++) {
            if (other->active && other->visible && (u8) Obj_SameArea(em, (Player *) other)) {
                if (gEmShowsOnMap[other->type]) {
                    if (other->combat == 1) {
                        fightNear = 1;
                        break;
                    }
                    if (other->combat == 0) {
                        calmNear = 1;
                    }
                } else if ((other->type == 0xC || other->type == 0x46) && other->variant == 0 && other->stateGroup == EM_GROUP_EVENT) {
                    popoInEvent = 1;
                }
            }
        }
        if (fightNear != 0 || (calmNear != 0 && popoInEvent != 0)) {
            EmApceros_SetRequest(em, 1);
            em->scriptPending = 1;
        }
    }
    Em_UpdateCombatTimer(em);
    if (em->stateGroup != EM_GROUP_EVENT) {
        switch (Em_UpdateDamage(em, dmgInfo) & 0xFF) {
        case EM_DMG_NONE:
        case EM_DMG_ATTACK9:
        case EM_DMG_ATTACK6:
        case EM_DMG_STATUS_IN_AIR:
        case EM_DMG_CAPTURED:
        case EM_DMG_FLASH:
        case EM_DMG_PART8_DOWN:
        case EM_DMG_ATTACK5:
        case EM_DMG_ATTACK5_FLINCH:
        case EM_DMG_ATTACK13:
            break;
        case EM_DMG_KILLED:
        case EM_DMG_DEAD:
            hitSide = em->hitAngle - em->rotY;
            if (em->stateGroup != EM_GROUP_DEFEAT) {
                if (hitSide > 0x8000) {
                    EmApceros_ChangeState(em, EM_GROUP_DEFEAT, APC_DEFEAT_DIE_A, EM_SYNC_OWNER);
                } else {
                    EmApceros_ChangeState(em, EM_GROUP_DEFEAT, APC_DEFEAT_DIE_B, EM_SYNC_OWNER);
                }
            }
            em->scriptPending = 0;
            break;
        case EM_DMG_STATUS4:
            if (em->stateGroup != EM_GROUP_REACT || em->state != APC_REACT_STATUS4) {
                Em_SetStatus4Timer(em);
                EmApceros_ChangeState(em, EM_GROUP_REACT, APC_REACT_STATUS4, EM_SYNC_OWNER);
            }
            hurt = 1;
            break;
        case EM_DMG_STATUS2:
            if (em->stateGroup != EM_GROUP_IDLE || em->state != APC_IDLE_STATUS2_COLLAPSE) {
                Em_SetStatus2Timer(em);
                EmApceros_ChangeState(em, EM_GROUP_IDLE, APC_IDLE_STATUS2_COLLAPSE, EM_SYNC_OWNER);
                em->scriptPending = 0;
            }
            hurt = 1;
            break;
        case EM_DMG_HIT_LYING:
            switch (em->state) {
            case 8:
                EmApceros_ChangeState(em, EM_GROUP_IDLE, APC_IDLE_STATUS2_END, EM_SYNC_OWNER);
                em->scriptPending = 0;
                break;
            }
            hurt = 1;
            break;
        case EM_DMG_HIT_MODE3:
            hurt = 1;
            break;
        case EM_DMG_HIT:
            if (em->type == EM_KIND_APCEROS) {
                break;
            }
        case EM_DMG_PART_FLINCH:
            hitSide = em->hitAngle - em->rotY;
            hurt = 1;
            if ((em->type == 0xC || em->type == EM_KIND_POPO) && em->hp > 0.2f * em->hpMax) {
                if (hitSide > 0x8000) {
                    EmApceros_ChangeState(em, EM_GROUP_REACT, APC_REACT_FLINCH_A, EM_SYNC_OWNER);
                } else {
                    EmApceros_ChangeState(em, EM_GROUP_REACT, APC_REACT_FLINCH_B, EM_SYNC_OWNER);
                }
            } else if (em->stateGroup != EM_GROUP_REACT || (em->state != 3 && em->state != 4)) {
                if (hitSide > 0x8000) {
                    EmApceros_ChangeState(em, EM_GROUP_REACT, APC_REACT_KNOCK_DOWN_A, EM_SYNC_OWNER);
                } else {
                    EmApceros_ChangeState(em, EM_GROUP_REACT, APC_REACT_KNOCK_DOWN_B, EM_SYNC_OWNER);
                }
            }
            em->scriptPending = 0;
            break;
        case EM_DMG_STATUS5:
            Em_EndStatus5(em);
            break;
        }
        if (hurt) {
            if (em->type == EM_KIND_POPO && em->variant != 2) {
                w->alarmed = 1;
            } else if (em->requested != 1) {
                EmApceros_SetRequest(em, 2);
            }
            if (w->objListed == 0) {
                Vec_Set(w->sense.pos, em->posX, em->posY, em->posZ);
                w->sense.radius = 2000.0f;
                w->sense.area = (u8) em->area;
                w->sense.kind = 2;
                ObjListA70_Add(&w->sense);
                w->objListed = 1;
            }
        }
    }
    if (em->requested != 1 && em->stateGroup != EM_GROUP_EVENT2 && em->stateGroup != EM_GROUP_EVENT && (u8) Em_InBlockArea1(em) == 1) {
        EmApceros_SetRequest(em, 3);
        em->scriptPending = 1;
    }
    switch (gQuest.stage) {
    case QUEST_ID_RATHALOS_SCENE_1:
    case QUEST_ID_RATHALOS_SCENE_2:
        if (Area_DayId(em->area) == AREA_FOREST_HILLS_3 && Save_TestEventFlag(EVENT_FLAG_RATHALOS_SCENE) == 0) {
            if (Area_DayId(gScene.area) == AREA_FOREST_HILLS_3 && gScene.eventRunning == 1 && em->stateGroup != EM_GROUP_EVENT) {
                if (em->variant == 0) {
                    EmApceros_ChangeState(em, EM_GROUP_EVENT, APC_EVENT_INTRO_PREY, EM_SYNC_SEND);
                } else {
                    EmApceros_ChangeState(em, EM_GROUP_EVENT, APC_EVENT_INTRO_WAIT, EM_SYNC_SEND);
                }
            }
            break;
        }
    default:
        Em_UpdatePendingScript(em);
        break;
    }
    Em_ClearHitReactMode(em);
    EmApceros_RunState(em);
    if (em->stateRequest != 0) {
        EmApceros_RunState(em);
        em->stateRequest = 0;
    }
}

/* Runs the dispatcher of em->stateGroup (0 to 7). */
void EmApceros_RunState(Em *em)
{
    switch (em->stateGroup) {
    case EM_GROUP_IDLE:
        EmApceros_RunIdleState(em);
        break;
    case EM_GROUP_MOVE:
        EmApceros_RunMoveState(em);
        break;
    case EM_GROUP_FLY:
        EmApceros_RunFlyState();
        break;
    case EM_GROUP_ATTACK:
        EmApceros_RunAttackState(em);
        break;
    case EM_GROUP_REACT:
        EmApceros_RunReactState(em);
        break;
    case EM_GROUP_DEFEAT:
        EmApceros_RunDefeatState(em);
        break;
    case EM_GROUP_EVENT:
        EmApceros_RunEventState(em);
        break;
    case EM_GROUP_EVENT2:
        EmApceros_RunEnterState(em);
        
    default:
        break;
    }
}

/*
 * Keeps motionId[0] in work->fxMotion, plays MotionSounds, then the effects
 * and attacks of the motion: APC_MOT_STOMP ground effect, camera shake and
 * attack 0xD; APC_MOT_RUN footstep effects; the VespoidAttack_Create kinds
 * of the attack motions (per kind); APC_MOT_FALL dust; APC_MOT_LIE ground
 * effects; APC_MOT_STATUS2_LIE lying puffs.
 */
void EmApceros_MotionEffects(Em *em, EmWork *work)
{
    f32 pos[3];
    f32 puffOffset[3];

    if (em->motionId[0] != work->fxMotion) {
        work->fxMotion = em->motionId[0];
    }
    EmApceros_MotionSounds(em, work);
    switch (work->fxMotion) {
    case EM_MOTION_ID(0, APC_MOT_STAND):
    case EM_MOTION_ID(0, APC_MOT_REAR_UP):
    case EM_MOTION_ID(0, APC_MOT_IDLE_LOOP_B):
    case EM_MOTION_ID(0, APC_MOT_IDLE_ACTION_A):
    case EM_MOTION_ID(0, APC_MOT_IDLE_ACTION_B):
        break;
    case EM_MOTION_ID(0, APC_MOT_STOMP):
        if (EmMotion_PassedFrameScaled(em, 40.0f, 0)) {
            Em_BonePos(em, 0xA, pos);
            pos[1] = em->floorY;
            PartFx_SpawnOnGround(pos, 7, 2.0f);
            CamShake_ReqAIf(em, 1);
            VespoidAttack_Create(em, 0xD);
        }
        break;
    case EM_MOTION_ID(0, APC_MOT_WALK):
    case EM_MOTION_ID(0, 0xC):
        break;
    case EM_MOTION_ID(0, APC_MOT_RUN):
        if (EmMotion_PassedFrameScaled(em, 24.0f, 0)) {
            PartFx_SpawnForEm(em, 0x17, 0x14);
        }
        if (EmMotion_PassedFrameScaled(em, 52.0f, 0)) {
            PartFx_SpawnForEm(em, 0x14, 0x14);
        }
        break;
    case EM_MOTION_ID(0, APC_MOT_TURN_A):
    case EM_MOTION_ID(0, APC_MOT_TURN_B):
    case EM_MOTION_ID(0, APC_MOT_ALERT):
        break;
    case EM_MOTION_ID(0, APC_MOT_ATTACK_REAR):
        if (EmMotion_PassedFrameScaled(em, 30.0f, 0)) {
            if (em->type == 0xC || em->type == EM_KIND_POPO) {
                VespoidAttack_Create(em, 1);
            } else {
                VespoidAttack_Create(em, 9);
            }
        }
        break;
    case EM_MOTION_ID(0, APC_MOT_WATCH):
    case EM_MOTION_ID(0, APC_MOT_WATCH_TURN_A):
    case EM_MOTION_ID(0, APC_MOT_WATCH_TURN_B):
    case EM_MOTION_ID(0, APC_MOT_ALERT_END):
        break;
    case EM_MOTION_ID(0, APC_MOT_ATTACK_B):
        if (em->type == EM_KIND_POPO) {
            if (EmMotion_PassedFrameScaled(em, 32.0f, 0)) {
                VespoidAttack_Create(em, 0x12);
            }
            if (EmMotion_PassedFrameScaled(em, 24.0f, 0)) {
                DustCloud_Create(em, 2, 0x2C, em->scaleX);
            }
            if (EmMotion_PassedFrameScaled(em, 52.0f, 0) && Player_InCurrentArea(em)) {
                Em_BonePos(em, 7, pos);
                pos[1] = em->floorY;
                PartFx_SpawnOnGround(pos, 4, 4.0f * em->scaleX);
            }
        } else if (EmMotion_PassedFrameScaled(em, 20.0f, 0)) {
            VespoidAttack_Create(em, 2);
        }
        break;
    case EM_MOTION_ID(0, APC_MOT_SPIN_ATTACK):
        if (EmMotion_PassedFrameScaled(em, 24.0f, 0)) {
            if (em->type == 0xC || em->type == EM_KIND_POPO) {
                VespoidAttack_Create(em, 3);
            } else {
                VespoidAttack_Create(em, 0xA);
            }
        }
        break;
    case EM_MOTION_ID(0, APC_MOT_FLINCH_B):
    case EM_MOTION_ID(0, APC_MOT_FLINCH_A):
        break;
    case EM_MOTION_ID(0, APC_MOT_FALL):
        if (EmMotion_PassedFrameScaled(em, 28.0f, 0)) {
            DustCloud_Create(em, 0, 0, 0.4f * em->scaleX);
        }
        break;
    case EM_MOTION_ID(0, APC_MOT_LIE):
        if (EmMotion_PassedFrameScaled(em, 46.0f, 0)) {
            PartFx_SpawnForEm(em, 0, 6);
        }
        if (EmMotion_PassedFrameScaled(em, 88.0f, 0)) {
            PartFx_SpawnForEm(em, 0, 6);
        }
        if (EmMotion_PassedFrameScaled(em, 190.0f, 0)) {
            PartFx_SpawnForEm(em, 0, 6);
        }
        break;
    case EM_MOTION_ID(0, APC_MOT_DIE):
    case EM_MOTION_ID(0, APC_MOT_GET_UP):
    case EM_MOTION_ID(0, APC_MOT_FALL_B):
        break;
    case EM_MOTION_ID(0, APC_MOT_STATUS2_LIE):
        puffOffset[0] = 0.0f;
        puffOffset[1] = 0.0f;
        puffOffset[2] = 20.0f;
        Em_SpawnLyingPuffs(em, 0xF, puffOffset, 1.2f);
        break;
    case EM_MOTION_ID(0, APC_MOT_STATUS4):
        break;
    }
}

/*
 * Callbacks slot 3: skips the first frame (fxStep 0 to 1), then runs
 * MotionEffects each frame.
 */
void EmApceros_UpdateMotionEffects(Em *em)
{
    EmApcerosWork *w = (EmApcerosWork *)&em->work;

    switch (w->fxStep) {
    case 0:
        w->fxStep++;
        break;
    case 1:
        EmApceros_MotionEffects(em, (EmWork *) w);
        break;
    }
}

/*
 * Callbacks slots 0 and 1: creates the attached effect 0 (Shadow_Spawn);
 * unlike EmPlesioth_SpawnEffects, no alert mark.
 */
void EmApceros_SpawnAttachedEffect(u8 *obj)
{
    Shadow_Spawn((Player *) obj, 0);
}

/* Callbacks slot 2: does nothing. */
void EmApceros_TableNop(void)
{
}

/*
 * Callbacks slot 4, the questions of the quest scripts (see NUMBER lines):
 * query 0 returns the area class of the area the monster is in, query 1 the
 * alarmed flag; other queries 0.
 */
s32 EmApceros_ScriptQuery(Em *em, u8 mode)
{
    EmApcerosWork *w = (EmApcerosWork *)&em->work;

    switch (mode) {
    case 0:
        switch (Area_DayId(em->area)) {
        default:
            return 0;
        case AREA_FOREST_HILLS_10:
        case AREA_FOREST_HILLS_9:
        case AREA_FOREST_HILLS_4:
        case AREA_FOREST_HILLS_1:
        case AREA_SNOWY_MOUNTAINS_2:
        case AREA_SNOWY_MOUNTAINS_3:
        case AREA_SNOWY_MOUNTAINS_4:
        case AREA_SNOWY_MOUNTAINS_5:
        case AREA_SNOWY_MOUNTAINS_6:
        case AREA_SNOWY_MOUNTAINS_7:
        case AREA_SNOWY_MOUNTAINS_8:
        case AREA_JUNGLE_1:
        case AREA_JUNGLE_3:
        case AREA_JUNGLE_7:
        case AREA_JUNGLE_8:
        case AREA_JUNGLE_9:
        case AREA_JUNGLE_10:
        case AREA_TOWER_1:
        case AREA_SWAMP_1:
        case AREA_SWAMP_2:
        case AREA_SWAMP_3:
        case AREA_SWAMP_4:
        case AREA_SWAMP_7:
            return 1;
        case AREA_FOREST_HILLS_3:
            return 2;
        case AREA_JUNGLE_2:
            return em->spawnParam == 0 ? 1 : 3;
        }
    case 1:
        return w->alarmed;
    }
    return 0;
}

/* The file's initialised data, in the game's order */
f32 EmApceros_ScalesType0C[8] = {
#include "game/008C72B0.inc"
};
f32 EmApceros_ScalesType46[12] = {
#include "game/008C72D0.inc"
};
EmTurnParams EmApceros_TurnParams[1] = {
#include "game/008C7300.inc"
};
void *EmApceros_Callbacks[8] = {
    EmApceros_SpawnAttachedEffect, EmApceros_SpawnAttachedEffect, EmApceros_TableNop, EmApceros_UpdateMotionEffects,
    EmApceros_ScriptQuery, EmApceros_Reinit, NULL, NULL,
};
