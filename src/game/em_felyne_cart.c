/*
 * The Felyne cart (EM_KIND_FELYNE_CART): the cart the Felynes pull to carry
 * a fainted player back to the camp. The quest's set-up adds the kind to the
 * camp's area, and Player_StateCart makes a cart at the fainted player
 * (Em.cartPlayer), keeps the player at its bone and moves the player on to
 * PL_EVENT_CART_DROP when the cart plays CART_MOT_DROP.
 *
 * The file runs from the cart's motions' sounds to its table's empty hook:
 * Init, the states of its groups and their dispatchers (only the idle and
 * move groups have real states; the cart's script picks them), Update, and
 * the effects drawn with its motions. The code is a cut-down copy of the
 * Mosswine's and reads the work area the same way (EmMosswineWork). The
 * states and motions are named in em_felyne_cart.h.
 */
#include "common.h"
#include "scene.h"
#include "quest.h"
#include "em.h"
#include "shadow.h"
#include "em_felyne_cart.h"
#include "player.h"
#include "area.h"
#include "capsule.h"
#include "sound.h"
#include "dust_flash_effects.h"
#include "gm_load.h"
#include "part_effects.h"
#include "vec_math.h"
s32 EmMotion_PassedFrameScaled(void *p, f32 frame, s32 layer);
/* end of prototypes */

/* The sounds on the frames of the current motion (work->fxMotion): four on the run, one on the drop. */
void EmFelyneCart_MotionSounds(Em *em, EmWork *work)
{
    switch (work->fxMotion) {
    case EM_MOTION_ID(0, CART_MOT_IDLE):
        break;
    case EM_MOTION_ID(0, CART_MOT_RUN):
        Em_PlaySeOnFrame(em, 2, 0);
        Em_PlaySeOnFrame(em, 38, 0);
        Em_PlaySeOnFrame(em, 20, Sound_RandomSe(2, 4, 4, 4));
        Em_PlaySeOnFrame(em, 58, Sound_RandomSe(3, 4, 5, 4));
        break;
    case EM_MOTION_ID(0, CART_MOT_DROP):
        Em_PlaySeOnFrame(em, 2, 1);
        break;
    }
}

/*
 * Sets a cart up. With no quest id (gQuest.stage 0) it is placed by its
 * slot: on a grid in AREA_SNOWY_MOUNTAINS_NIGHT_9, in a row elsewhere. Then
 * it stands (CART_MOT_IDLE) on the ground with 50 hp, asks its script for a
 * state, and keeps its position as its home (the Y in the Z slot, as Init
 * stores it).
 */
void EmFelyneCart_Init(Em *em)
{
    EmMosswineWork *w = (EmMosswineWork *)&em->work;

    f32 fslot;
    u8 slotNo;

    if (gQuest.stage == 0) {
        if (gScene.area == AREA_SNOWY_MOUNTAINS_NIGHT_9) {
            em->posX = 9500.0f + (150.0f * (f32) (em->slotNo / 8));
            em->posY = 0.0f;
            em->posZ = 9200.0f + (150.0f * (f32) (em->slotNo & 7));
        } else {
            slotNo = em->slotNo;
            fslot = (f32) (u32) slotNo;
            em->posX = 2400.0f + (150.0f * fslot);
            em->posY = 0.0f;
            em->posZ = 3600.0f;
        }
    }
    Em_SetMotion(em, CART_MOT_IDLE, 0, 0);
    em->stance = EM_STANCE_GROUND;
    Em_SetStateRand(em, EM_GROUP_IDLE, CART_IDLE_WAIT);
    em->hp = 50;
    em->hpMax = 50;
    em->scriptPending = 1;
    w->unk00C = 0;
    w->unk00E = 0;
    w->home[0] = em->posX;
    w->home[1] = 0.0f;
    w->home[2] = em->posY;
    em->remote = 0;
    em->senseDelay = 5;
}

/* CART_IDLE_NONE: nothing. */
void EmFelyneCart_IdleNone(void)
{
}

/*
 * CART_IDLE_WAIT: stands (CART_MOT_IDLE) for 200 frames, then asks the
 * script for the next state.
 */
void EmFelyneCart_Wait(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        if (em->motionId[0] != EM_MOTION_ID(0, CART_MOT_IDLE)) {
            Em_SetMotion(em, CART_MOT_IDLE, 0, 0);
        }
        em->timer = 200;
        break;
    case 1:
        if (--em->timer < 0) {
            em->scriptPending = 1;
        }
        break;
    }
}

/*
 * CART_IDLE_DROP_PLAYER: plays CART_MOT_DROP once (Player_StateCart drops
 * the player when it starts), then asks the script for the next state.
 */
void EmFelyneCart_DropPlayer(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        if (em->motionId[0] != EM_MOTION_ID(0, CART_MOT_DROP)) {
            Em_SetMotion(em, CART_MOT_DROP, 0, 0);
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
 * Runs the current idle-group state, first keeping the state as the previous
 * one.
 */
void EmFelyneCart_RunIdleState(Em *em)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->state) {
    case CART_IDLE_NONE:
        EmFelyneCart_IdleNone();
        break;
    case CART_IDLE_WAIT:
        EmFelyneCart_Wait(em);
        break;
    case CART_IDLE_DROP_PLAYER:
        EmFelyneCart_DropPlayer(em);
        break;
    }
}

/*
 * CART_MOVE_CIRCLE: runs (CART_MOT_RUN) turning 112 a frame, about a full
 * circle in 600 frames, then asks the script for the next state.
 */
void EmFelyneCart_RunCircle(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        if (em->motionId[0] != EM_MOTION_ID(0, CART_MOT_RUN)) {
            Em_SetMotion(em, CART_MOT_RUN, 0, 0);
        }
        em->timer = 600;
        break;
    case 1:
        em->rotY += 112;
        em->rotY = (s32) (u16) em->rotY;
        Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
        if (--em->timer <= 0) {
            em->scriptPending = 1;
        }
        break;
    }
}

/*
 * CART_MOVE_CARRY: runs (CART_MOT_RUN) straight for 90 frames, then asks the
 * script for the next state. When the player it carries has left
 * PL_GROUP_EVENT, the cart hides and ends its task instead.
 */
void EmFelyneCart_CarryPlayer(Em *em)
{
    if (((Em *) &gPlayers[em->cartPlayer])->stateGroup != PL_GROUP_EVENT) {
        em->taskPhase++;
        em->visible = 0;
    } else {
        switch (em->step) {
        case 0:
            em->step++;
            if (em->motionId[0] != EM_MOTION_ID(0, CART_MOT_RUN)) {
                Em_SetMotion(em, CART_MOT_RUN, 0, 0);
            }
            em->timer = 90;
            break;
        case 1:
            if (--em->timer <= 0) {
                em->scriptPending = 1;
            }
            break;
        }
    }
}

/*
 * CART_MOVE_TURN_AROUND: runs (CART_MOT_RUN) turning for 53 frames, a little
 * over half a circle (a little less in AREA_FORTRESS_0 and AREA_CASTLE_0),
 * then asks the script for the next state.
 */
void EmFelyneCart_TurnAround(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 53;
        if (em->motionId[0] != EM_MOTION_ID(0, CART_MOT_RUN)) {
            Em_SetMotion(em, CART_MOT_RUN, 0, 0);
        }
        break;
    case 1:
        switch (gScene.area) {
        case AREA_FORTRESS_0:
        case AREA_CASTLE_0:
            em->rotY += 0x2A0;
            break;
        default:
            em->rotY += 0x2B0;
            break;
        }
        em->rotY = (s32) (u16) em->rotY;
        Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
        if (--em->timer <= 0) {
            em->scriptPending = 1;
        }
        break;
    }
}

/*
 * CART_MOVE_RUN_OFF: runs (CART_MOT_RUN) straight for 100 frames, fading out
 * over the last 5, then hides and ends its task.
 */
void EmFelyneCart_RunOff(Em *em)
{
    s32 timer;

    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 100;
        if (em->motionId[0] != EM_MOTION_ID(0, CART_MOT_RUN)) {
            Em_SetMotion(em, CART_MOT_RUN, 0, 0);
        }
        break;
    case 1:
        if (--em->timer <= 0) {
            em->taskPhase++;
            em->visible = 0;
            em->timer = 0;
        }
        timer = em->timer;
        if (timer < 5) {
            em->alpha = (f32) timer / 5.0f;
        }
        break;
    }
}

/* Runs the current move-group state. */
void EmFelyneCart_RunMoveState(Em *em)
{
    switch (em->state) {
    case CART_MOVE_CIRCLE:
        EmFelyneCart_RunCircle(em);
        break;
    case CART_MOVE_CARRY:
        EmFelyneCart_CarryPlayer(em);
        break;
    case CART_MOVE_TURN_AROUND:
        EmFelyneCart_TurnAround(em);
        break;
    case CART_MOVE_RUN_OFF:
        EmFelyneCart_RunOff(em);
        break;
    }
}

void EmFelyneCart_RunFlyState(void)
{
}

void EmFelyneCart_AttackNone(void)
{
}

/* The attack group: its one state does nothing. */
void EmFelyneCart_RunAttackState(Em *em)
{
    switch (em->state) {
    case 0:
        EmFelyneCart_AttackNone();
        break;
    }
}

void EmFelyneCart_ReactNone(void)
{
}

/*
 * The react group, which also serves the defeat and event groups: its one
 * state does nothing.
 */
void EmFelyneCart_RunReactState(Em *em)
{
    switch (em->state) {
    case 0:
        EmFelyneCart_ReactNone();
        break;
    }
}

/*
 * The cart's frame: counts down the work area's timer
 * (EmMosswineWork.unk00C), runs the script, then the current state, and the
 * state again when it asked for a change.
 */
void EmFelyneCart_Update(Em *em)
{
    EmMosswineWork *w = (EmMosswineWork *)&em->work;

    u16 timeLeft;

    timeLeft = w->unk00C;
    if (timeLeft != 0) {
        w->unk00C = timeLeft - 1;
    }
    em->senseDelay = 5;
    Em_UpdatePendingScript(em);
    Em_ClearHitReactMode(em);
    EmFelyneCart_RunState(em);
    if (em->stateRequest != 0) {
        EmFelyneCart_RunState(em);
        em->stateRequest = 0;
    }
}

/*
 * Runs the current state through its group's dispatcher; groups 4 to 7 share
 * EmFelyneCart_RunReactState.
 */
void EmFelyneCart_RunState(Em *em)
{
    switch (em->stateGroup) {
    case EM_GROUP_IDLE:
        EmFelyneCart_RunIdleState(em);
        break;
    case EM_GROUP_MOVE:
        EmFelyneCart_RunMoveState(em);
        break;
    case EM_GROUP_FLY:
        EmFelyneCart_RunFlyState();
        break;
    case EM_GROUP_ATTACK:
        EmFelyneCart_RunAttackState(em);
        break;
    case EM_GROUP_REACT:
        EmFelyneCart_RunReactState(em);
        break;
    case EM_GROUP_DEFEAT:
        EmFelyneCart_RunReactState(em);
        break;
    case EM_GROUP_EVENT:
        EmFelyneCart_RunReactState(em);
        break;
    case EM_GROUP_EVENT2:
        EmFelyneCart_RunReactState(em);
        
    default:
        break;
    }
}

/*
 * The motions' sounds and effects: the run's dust at the feet every 12
 * frames, and the drop's effect at frame 16 and dust clouds at frame 46.
 */
void EmFelyneCart_MotionEffects(Em *em, EmWork *work)
{
    if (em->motionId[0] != work->fxMotion) {
        work->fxMotion = em->motionId[0];
    }
    EmFelyneCart_MotionSounds(em, work);
    switch (work->fxMotion) {
    case EM_MOTION_ID(0, CART_MOT_IDLE):
        break;
    case EM_MOTION_ID(0, CART_MOT_RUN):
        if (EmMotion_ReachedFrame(em, 0, 6.0f) && gScene.frame % 12 == 1) {
            DustCloud_Create(em, 2, 5, 0.2f);
        }
        if (EmMotion_ReachedFrame(em, 0, 4.0f) && gScene.frame % 12 == 0) {
            DustCloud_Create(em, 3, 5, 0.2f);
        }
        break;
    case EM_MOTION_ID(0, CART_MOT_DROP):
        if (EmMotion_PassedFrameScaled(em, 16.0f, 0)) {
            PartFx_SpawnForEm(em, 16, 7);
        }
        if (EmMotion_PassedFrameScaled(em, 46.0f, 0)) {
            DustCloud_Create(em, 28, 0, 1.0f);
            DustCloud_Create(em, 28, 1, 1.0f);
        }
        break;
    }
}

/*
 * Callback slot 3 (Em_RunMotionFx): skips the first frame, then runs
 * EmFelyneCart_MotionEffects every frame.
 */
void EmFelyneCart_UpdateMotionEffects(Em *em)
{
    EmMosswineWork *w = (EmMosswineWork *)&em->work;

    switch (w->fxStep) {
    case 0:
        w->fxStep++;
        break;
    case 1:
        EmFelyneCart_MotionEffects(em, (EmWork *) w);
        break;
    }
}

/*
 * Callback slots 0 and 1 (EmVt.init, update): makes the monster's effect
 * 01, as the other families do.
 */
void EmFelyneCart_SpawnShadow(u8 *em)
{
    Shadow_Spawn((Player *) em, SHADOW_MODEL);
}

/* Callback slots 2 and 5: nothing. */
void EmFelyneCart_TableNop(void)
{
}

/* The cart's row of the monsters' callback table (gGmEmCallbackTbl). */
void *EmFelyneCart_Callbacks[8] = {
    EmFelyneCart_SpawnShadow, EmFelyneCart_SpawnShadow, EmFelyneCart_TableNop, EmFelyneCart_UpdateMotionEffects,
    GmEm_ReturnZero, EmFelyneCart_TableNop, NULL, NULL,
};
