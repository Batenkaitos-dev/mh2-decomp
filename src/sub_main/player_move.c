/*
 * Moving the player and the other objects.
 * - Motions: starting a motion on one of the player's layers, or on two at
 *   once (Player_SetMotion, Player_StartMotionLayer), and the motion time
 *   the hit timers count by.
 * - Movement: velocity and acceleration for the player, for any object with
 *   the shared header (ObjHead) and for the attack objects (ObjAttach); a
 *   jump's rise; the fall onto the floor.
 * Several of these also run on monsters, read through the Player struct:
 * the fields they use sit at the same offsets in Player and Em.
 */
#include "common.h"
#include "player.h"
#include "em.h"
#include "objhead.h"
#include "objattach.h"
#include "area.h"
#include "hit_setup.h"
#include "vec_math.h"
/* unit's own declaration: u16 and s16 as Player_StartMotionLayer's call converts its arguments; with the definition's s32 the call compiles differently */
void Player_BeginLayerMotion(Player *pl, u16 frame, s16 blend, s32 layer);
/* end of prototypes */

/* Sets motion layer `layer`'s motion id, blend (half of `blend`) and frame;
 * the layer starts the motion on its next update. */
static void Player_SetLayerMotion(Player *pl, s32 motion, s32 blend, s16 frame, s32 layer)
{
    pl->motionId[layer] = motion;
    pl->motionBlend[layer] = blend / 2;
    pl->motionFrame[layer] = frame;
    pl->motionStarted[layer] = 0;
}

/* Sets a motion on one layer (Player_SetLayerMotion), clearing
 * Player.unk771 as every motion start does. */
void Player_SetMotion(Player *pl, s32 motion, s32 blend, s16 frame, s32 layer)
{
    pl->unk771 = 0;
    Player_SetLayerMotion(pl, motion, blend, frame, layer);
}

/* Starts a motion on layers 0 and 1 (motion + 100). */
void Player_SetMotionPair(Player *pl, s32 motion, s32 blend, s16 frame)
{
    pl->unk771 = 0;
    Player_SetLayerMotion(pl, motion, blend, frame, 0);
    Player_SetLayerMotion(pl, motion + 100, blend, frame, 1);
}

/* Starts a motion on layer `layer` at once, when the player has that many
 * layers: the rotation matrix refreshed from Player.rot, the layer's motion
 * id, blend (half of `blend`) and frame set, and the layer begun
 * (Player_BeginLayerMotion). */
void Player_StartMotionLayer(Player *pl, u16 motion, s32 blend, s16 frame, s32 layer)
{
    if (layer < pl->motionLayerCount) {
        Mtx_FromAnglesXYZ(pl->rot, pl->rotMtx);
        pl->motionId[layer] = motion;
        pl->motionBlend[layer] = blend / 2;
        pl->motionFrame[layer] = frame;
        Player_BeginLayerMotion(pl, pl->motionFrame[layer], pl->motionBlend[layer], layer);
    }
}

/* Does nothing: the game's function is empty. Player_RunInputHook passes
 * it the buttons not held. */
void Player_InputHook(Player *pl, u16 buttons)
{
}

/* Runs Player_InputHook (which does nothing) with the buttons not held; the
 * town's input update calls it after reading the pad. */
void Player_RunInputHook(Player *pl)
{
    Player_InputHook(pl, ~pl->input.held);
}

/* Sets the vertical acceleration that brings the object's rise to a stop
 * in half of `frames` frames. Returns 1 when it stops within one frame
 * (frames 3 or fewer, or already falling), else 0. */
s32 Player_StartJump(void *obj, s16 frames)
{
    ObjHead *head = obj;
    f32 riseAccel;

    frames = frames / 2;
    riseAccel = -1.0f * head->vel[1];
    if (frames <= 1 || head->vel[1] < 0.0f) {
        head->accel[1] = riseAccel;
        return 1;
    }
    riseAccel /= frames;
    head->accel[1] = riseAccel;
    return 0;
}

/* Moves the object by its velocity. */
void Player_Move(void *obj)
{
    ObjHead *head = obj;
    head->pos[0] += head->vel[0];
    head->pos[1] += head->vel[1];
    head->pos[2] += head->vel[2];
}

/* Adds the acceleration to the velocity, then moves the player by it. */
void Player_MoveAccel(Player *pl)
{
    pl->vel[0] += pl->accel[0];
    pl->vel[1] += pl->accel[1];
    pl->vel[2] += pl->accel[2];
    pl->pos[0] += pl->vel[0];
    pl->pos[1] += pl->vel[1];
    pl->pos[2] += pl->vel[2];
}

/* Player_MoveAccel for any object with the shared header. */
void Player_Integrate(void *obj)
{
    ObjHead *head = obj;
    head->vel[0] += head->accel[0];
    head->vel[1] += head->accel[1];
    head->vel[2] += head->accel[2];
    head->pos[0] += head->vel[0];
    head->pos[1] += head->vel[1];
    head->pos[2] += head->vel[2];
}

/* Moves as Player_MoveAccel, then stands the player on the floor
 * (AreaGrid2_GetFloorY) if it went below it: returns 1 then, else 0. */
s32 Player_MoveFall(Player *pl)
{
    f32 ground;

    pl->vel[0] += pl->accel[0];
    pl->vel[1] += pl->accel[1];
    pl->vel[2] += pl->accel[2];
    pl->pos[0] += pl->vel[0];
    pl->pos[1] += pl->vel[1];
    pl->pos[2] += pl->vel[2];
    ground = AreaGrid2_GetFloorY(pl->pos);
    if (pl->pos[1] <= ground) {
        pl->pos[1] = ground;
        return 1;
    }
    return 0;
}

/* Clears the object's acceleration and velocity. */
void Player_StopMove(void *obj)
{
    ObjHead *head = obj;
    head->accel[2] = 0.0f;
    head->accel[1] = 0.0f;
    head->accel[0] = 0.0f;
    head->vel[2] = 0.0f;
    head->vel[1] = 0.0f;
    head->vel[0] = 0.0f;
}

/* The same as Player_StopMove. */
void Player_ClearVelocity(void *obj)
{
    ObjHead *head = obj;
    head->accel[2] = 0.0f;
    head->accel[1] = 0.0f;
    head->accel[0] = 0.0f;
    head->vel[2] = 0.0f;
    head->vel[1] = 0.0f;
    head->vel[0] = 0.0f;
}

/* Clears the player's acceleration. */
void Player_ClearAccel(Player *pl)
{
    pl->accel[2] = 0.0f;
    pl->accel[1] = 0.0f;
    pl->accel[0] = 0.0f;
}

/* Player_ClearAccel for any object with the shared header. */
void Player_ClearAccel2(void *obj)
{
    ObjHead *head = obj;
    head->accel[2] = 0.0f;
    head->accel[1] = 0.0f;
    head->accel[0] = 0.0f;
}

/* Moves an attack object by its velocity. */
void ObjAttach_Move(ObjAttach *obj)
{
    obj->pos[0] += obj->velX;
    obj->pos[1] += obj->velY;
    obj->pos[2] += obj->velZ;
}

/* Adds an attack object's acceleration to its velocity, then moves it. The
 * acceleration is the three words from homingTurn: gravity is its y, and the
 * homing families use its x and z as their turn and speed instead. */
void ObjAttach_MoveAccel(ObjAttach *obj)
{
    obj->velX += obj->homingTurn;
    obj->velY += obj->gravity;
    obj->velZ += obj->homingSpeed;
    obj->pos[0] += obj->velX;
    obj->pos[1] += obj->velY;
    obj->pos[2] += obj->velZ;
}

/* Sets Player.motionTime from layer 0: its frame less the size of its
 * blend. With `step`, Player.hitDelay (a byte) is also lowered by that
 * count, and by one more while a blend is set. */
void Player_UpdateMotionTime(Player *pl, s16 step)
{
    s16 blend = pl->motionBlend[0];
    s16 len = pl->motionFrame[0];

    pl->motionTime = 0;
    if (blend < 0) {
        blend = -blend;
    }
    pl->motionTime += len - blend;
    if (step != 0) {
        if (blend != 0) {
            blend--;
        }
        pl->hitDelay += (u8)(blend - len);
    }
}

/* Player_UpdateMotionTime for an object passed untyped. */
void Motion_InitTime(void *obj, s16 step)
{
    Player *pl = obj;
    s16 blend = pl->motionBlend[0];
    s16 len = pl->motionFrame[0];

    pl->motionTime = 0;
    if (blend < 0) {
        blend = -blend;
    }
    pl->motionTime += len - blend;
    if (step != 0) {
        if (blend != 0) {
            blend--;
        }
        pl->hitDelay += (u8)(blend - len);
    }
}

