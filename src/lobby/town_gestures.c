/*
 * The town's gesture states (state group TOWN_PC_GROUP_GESTURE): the town's
 * copies of the field's gestures (Player_StateGesture*, the same motions),
 * started by TownPc_StartGesture from Player.actRequest and run by
 * TownPc_RunGestureState, which the town players' state update calls.
 *
 * The file's data holds the one-shot gestures' table, then the forge's
 * recipe and upgrade tables, which only the forge's menus read: they follow
 * the gesture table in the same data piece.
 */

#include "common.h"
#include "em.h"
#include "player.h"
#include "quest.h"
#include "lobby_group_board.h"
#include "town_gestures.h"
#include "rand.h"
#include "town_gesture_drink.h"
#include "town_players.h"
#include "lobby_forge.h"

extern GestureOneShot gTownGestureOneShots[];
/*
 * The gesture PL_GESTURE_LIE_DOWN: lies down on its back. The local player
 * gets up with the stick (PL_GESTURE_GET_UP) or starts a newly requested
 * gesture. The town copy of Player_StateGestureLieDown, as are the others.
 */
void TownPc_GestureLieDown(Player *pl, s32 unused)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetMotion(pl, PL_MOT_LIE_FACE_UP, 4, 0);
        pl->motionTime = 0;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        break;
    case 1:
        if (Player_IsLocal(pl) == 1) {
            if ((s32) pl->input.stickTilt >= PL_STICK_HELD) {
                Actor_SetStateNow(pl, TOWN_PC_GROUP_GESTURE, PL_GESTURE_GET_UP, 0);
                break;
            }
            if (pl->actRequest != 0) {
                TownPc_StartGesture(pl);
            }
        }
        break;
    }
}

/*
 * PL_GESTURE_GET_UP: gets up from lying on its back, then idle.
 */
void TownPc_GestureGetUp(Player *pl, s32 unused)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetMotion(pl, PL_MOT_GET_UP_FACE_UP, 0, 0);
        pl->motionTime = 0;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->timer = 60; /* frames: 1 s */
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Actor_ReturnToIdle(pl, 0, 4, 0);
        }
        break;
    }
}

/*
 * PL_GESTURE_SCOUT: the scouting gesture, held at least 60 frames; then the
 * local player leaves it with the stick (TOWN_GESTURE_END) or starts a newly
 * requested gesture. The field's flyer check is not made in the town.
 */
void TownPc_GestureScout(Player *pl, s32 unused)
{
    s32 timer;
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetMotion(pl, PL_MOT_GESTURE_SCOUT, 6, 0);
        pl->motionTime = 0;
        Actor_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->timer = 60; /* frames: 1 s */
        break;
    case 1:
        timer = pl->timer;
        if (timer > 0) {
            pl->timer = (s32) (timer - 1);
        }
        if ((Player_IsLocal(pl) == 1) && (pl->timer == 0)) {
            if ((s32) pl->input.stickTilt >= PL_STICK_HELD) {
                Actor_SetStateFlags(pl, TOWN_PC_GROUP_GESTURE, TOWN_GESTURE_END, 0);
                break;
            }
            if (pl->actRequest != 0) {
                TownPc_StartGesture(pl);
            }
        }
        break;
    }
}

/*
 * PL_GESTURE_03: the looped sitting gesture; from its frame 90 the local
 * player ends it with the stick (PL_GESTURE_03_END) or starts a newly
 * requested gesture.
 */
void TownPc_Gesture03(Player *pl, s32 unused)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetMotion(pl, PL_MOT_GESTURE_SIT, 6, 0);
        pl->motionTime = 0;
        Actor_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        break;
    case 1:
        if ((Player_IsLocal(pl) == 1) && (Motion_ReachedFrame(pl, 0, 90.0f) != 0)) {
            if ((s32) pl->input.stickTilt >= PL_STICK_HELD) {
                Actor_SetStateNow(pl, TOWN_PC_GROUP_GESTURE, PL_GESTURE_03_END, 0);
                break;
            }
            if (pl->actRequest != 0) {
                TownPc_StartGesture(pl);
            }
        }
        break;
    }
}

/*
 * PL_GESTURE_03_END: the sitting gesture's end, then the get-up motion with
 * the root movement off, then idle.
 */
void TownPc_Gesture03End(Player *pl, s32 unused)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetMotion(pl, PL_MOT_GESTURE_SIT_END, 6, 0);
        pl->motionTime = 0;
        Actor_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step = (u8) (step + 1);
            Player_SetFlag(pl, PLF_NO_ROOT_MOVE);
            Actor_SetMotion(pl, PL_MOT_GET_UP_FACE_UP, -4, 60);
        }
        break;
    case 2:
        Player_SetFlag(pl, PLF_NO_ROOT_MOVE);
        if (pl->motion[0].playing == 0) {
            Player_ClearFlag(pl, PLF_NO_ROOT_MOVE);
            Actor_ReturnToIdle(pl, 0, 4, 0);
        }
        break;
    }
}

/*
 * PL_GESTURE_05: held at least 60 frames, then left with the stick
 * (TOWN_GESTURE_END) or for a newly requested gesture. The field's version
 * also makes attack objects; this one doesn't.
 */
void TownPc_Gesture05(Player *pl, s32 unused)
{
    s32 timer;
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetMotion(pl, 628, 6, 0);
        pl->motionTime = 0;
        Actor_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->timer = 60; /* frames: 1 s */
        break;
    case 1:
        timer = pl->timer;
        if (timer > 0) {
            pl->timer = (s32) (timer - 1);
        }
        if ((Player_IsLocal(pl) == 1) && (pl->timer == 0)) {
            if ((s32) pl->input.stickTilt >= PL_STICK_HELD) {
                Actor_SetStateFlags(pl, TOWN_PC_GROUP_GESTURE, TOWN_GESTURE_END, 0);
                break;
            }
            if (pl->actRequest != 0) {
                TownPc_StartGesture(pl);
            }
        }
        break;
    }
}

/*
 * PL_GESTURE_06, and PL_GESTURE_06_ALT: held at least 60 frames, then left
 * with the stick (TOWN_GESTURE_END) or for a newly requested gesture.
 */
void TownPc_Gesture06(Player *pl, s32 unused)
{
    s32 timer;
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetMotion(pl, 632, 4, 0);
        pl->motionTime = 0;
        Actor_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->timer = 60; /* frames: 1 s */
        break;
    case 1:
        timer = pl->timer;
        if (timer > 0) {
            pl->timer = (s32) (timer - 1);
        }
        if ((Player_IsLocal(pl) == 1) && (pl->timer == 0)) {
            if ((s32) pl->input.stickTilt >= PL_STICK_HELD) {
                Actor_SetStateFlags(pl, TOWN_PC_GROUP_GESTURE, TOWN_GESTURE_END, 0);
                break;
            }
            if (pl->actRequest != 0) {
                TownPc_StartGesture(pl);
            }
        }
        break;
    }
}

/*
 * PL_GESTURE_07: as TownPc_Gesture06, with its own motion.
 */
void TownPc_Gesture07(Player *pl, s32 unused)
{
    s32 timer;
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetMotion(pl, 622, 4, 0);
        pl->motionTime = 0;
        Actor_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->timer = 60; /* frames: 1 s */
        break;
    case 1:
        timer = pl->timer;
        if (timer > 0) {
            pl->timer = (s32) (timer - 1);
        }
        if ((Player_IsLocal(pl) == 1) && (pl->timer == 0)) {
            if ((s32) pl->input.stickTilt >= PL_STICK_HELD) {
                Actor_SetStateFlags(pl, TOWN_PC_GROUP_GESTURE, TOWN_GESTURE_END, 0);
                break;
            }
            if (pl->actRequest != 0) {
                TownPc_StartGesture(pl);
            }
        }
        break;
    }
}

/*
 * PL_GESTURE_08: as TownPc_Gesture06, with its own motion.
 */
void TownPc_Gesture08(Player *pl, s32 unused)
{
    s32 timer;
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetMotion(pl, 627, 6, 0);
        pl->motionTime = 0;
        Actor_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->timer = 60; /* frames: 1 s */
        break;
    case 1:
        timer = pl->timer;
        if (timer > 0) {
            pl->timer = (s32) (timer - 1);
        }
        if ((Player_IsLocal(pl) == 1) && (pl->timer == 0)) {
            if ((s32) pl->input.stickTilt >= PL_STICK_HELD) {
                Actor_SetStateFlags(pl, TOWN_PC_GROUP_GESTURE, TOWN_GESTURE_END, 0);
                break;
            }
            if (pl->actRequest != 0) {
                TownPc_StartGesture(pl);
            }
        }
        break;
    }
}

/*
 * The one-shot gestures (PL_GESTURE_ONESHOT0, 1 and 3): row variant of
 * gTownGestureOneShots gives the motion, then the blend back to idle.
 */
void TownPc_GestureOneShot(Player *pl, s32 variant)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetMotion(pl, gTownGestureOneShots[variant].motion, 4, 0);
        pl->motionTime = 0;
        Actor_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Actor_ReturnToIdle(pl, 0, gTownGestureOneShots[variant].blend, 0);
        }
        break;
    }
}

/*
 * PL_GESTURE_0A: as TownPc_Gesture06, with its own motion.
 */
void TownPc_Gesture0A(Player *pl, s32 unused)
{
    s32 timer;
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetMotion(pl, 625, 4, 0);
        pl->motionTime = 0;
        Actor_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->timer = 60; /* frames: 1 s */
        break;
    case 1:
        timer = pl->timer;
        if (timer > 0) {
            pl->timer = (s32) (timer - 1);
        }
        if ((Player_IsLocal(pl) == 1) && (pl->timer == 0)) {
            if ((s32) pl->input.stickTilt >= PL_STICK_HELD) {
                Actor_SetStateFlags(pl, TOWN_PC_GROUP_GESTURE, TOWN_GESTURE_END, 0);
                break;
            }
            if (pl->actRequest != 0) {
                TownPc_StartGesture(pl);
            }
        }
        break;
    }
}

/*
 * PL_GESTURE_0B: as TownPc_Gesture06, with its own motion and a longer
 * blend in.
 */
void TownPc_Gesture0B(Player *pl, s32 unused)
{
    s32 timer;
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetMotion(pl, 630, 16, 0);
        pl->motionTime = 0;
        Actor_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->timer = 60; /* frames: 1 s */
        break;
    case 1:
        timer = pl->timer;
        if (timer > 0) {
            pl->timer = (s32) (timer - 1);
        }
        if ((Player_IsLocal(pl) == 1) && (pl->timer == 0)) {
            if ((s32) pl->input.stickTilt >= PL_STICK_HELD) {
                Actor_SetStateFlags(pl, TOWN_PC_GROUP_GESTURE, TOWN_GESTURE_END, 0);
                break;
            }
            if (pl->actRequest != 0) {
                TownPc_StartGesture(pl);
            }
        }
        break;
    }
}

/*
 * PL_GESTURE_0C: as TownPc_Gesture06, with its own motion.
 */
void TownPc_Gesture0C(Player *pl, s32 unused)
{
    s32 timer;
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetMotion(pl, 626, 6, 0);
        pl->motionTime = 0;
        Actor_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->timer = 60; /* frames: 1 s */
        break;
    case 1:
        timer = pl->timer;
        if (timer > 0) {
            pl->timer = (s32) (timer - 1);
        }
        if ((Player_IsLocal(pl) == 1) && (pl->timer == 0)) {
            if ((s32) pl->input.stickTilt >= PL_STICK_HELD) {
                Actor_SetStateFlags(pl, TOWN_PC_GROUP_GESTURE, TOWN_GESTURE_END, 0);
                break;
            }
            if (pl->actRequest != 0) {
                TownPc_StartGesture(pl);
            }
        }
        break;
    }
}

/*
 * PL_GESTURE_10: as TownPc_Gesture06, with its own motion.
 */
void TownPc_Gesture10(Player *pl, s32 unused)
{
    s32 timer;
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetMotion(pl, 633, 6, 0);
        pl->motionTime = 0;
        Actor_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->timer = 60; /* frames: 1 s */
        break;
    case 1:
        timer = pl->timer;
        if (timer > 0) {
            pl->timer = (s32) (timer - 1);
        }
        if ((Player_IsLocal(pl) == 1) && (pl->timer == 0)) {
            if ((s32) pl->input.stickTilt >= PL_STICK_HELD) {
                Actor_SetStateFlags(pl, TOWN_PC_GROUP_GESTURE, TOWN_GESTURE_END, 0);
                break;
            }
            if (pl->actRequest != 0) {
                TownPc_StartGesture(pl);
            }
        }
        break;
    }
}

/*
 * TOWN_GESTURE_END: a held gesture's end, back to idle.
 */
void TownPc_GestureEnd(Player *pl, s32 unused)
{
    Actor_ReturnToIdle(pl, 0, 4, 0);
}

/*
 * Runs the player's gesture state (group TOWN_PC_GROUP_GESTURE): one
 * function per gesture, the argument picking a one-shot gesture's row; an
 * unknown state prints a debug message.
 */
void TownPc_RunGestureState(Player *pl)
{
    switch (pl->state) {
    case PL_GESTURE_LIE_DOWN:
        TownPc_GestureLieDown(pl, 0);
        break;
    case PL_GESTURE_GET_UP:
        TownPc_GestureGetUp(pl, 0);
        break;
    case PL_GESTURE_SCOUT:
        TownPc_GestureScout(pl, 0);
        break;
    case PL_GESTURE_03:
        TownPc_Gesture03(pl, 0);
        break;
    case PL_GESTURE_03_END:
        TownPc_Gesture03End(pl, 0);
        break;
    case PL_GESTURE_05:
        TownPc_Gesture05(pl, 0);
        break;
    case PL_GESTURE_06:
        TownPc_Gesture06(pl, 0);
        break;
    case PL_GESTURE_07:
        TownPc_Gesture07(pl, 0);
        break;
    case PL_GESTURE_08:
        TownPc_Gesture08(pl, 0);
        break;
    case PL_GESTURE_ONESHOT0:
        TownPc_GestureOneShot(pl, 0);
        break;
    case PL_GESTURE_0A:
        TownPc_Gesture0A(pl, 0);
        break;
    case PL_GESTURE_0B:
        TownPc_Gesture0B(pl, 0);
        break;
    case PL_GESTURE_0C:
        TownPc_Gesture0C(pl, 0);
        break;
    case PL_GESTURE_ONESHOT1:
        TownPc_GestureOneShot(pl, 1);
        break;
    case PL_GESTURE_06_ALT:
        TownPc_Gesture06(pl, 2);
        break;
    case PL_GESTURE_ONESHOT3:
        TownPc_GestureOneShot(pl, 3);
        break;
    case PL_GESTURE_10:
        TownPc_Gesture10(pl, 0);
        break;
    case TOWN_GESTURE_END:
        TownPc_GestureEnd(pl, 0);
        break;
    default:
        Debug_Message("NO PL ACTION", pl->stateGroup, pl->state, pl->prevState);
        break;
    }
}

/* This file's initialised data. */
/* The unit's data: the one-shot gestures' (motion, blend) rows, then the
 * forge's recipe and upgrade tables. */
GestureOneShot gTownGestureOneShots[4] = {
#include "lobby/00802C60.inc"
};
/* The recipe tables run to a recipe of type FORGE_RECIPE_END (the readers'
 * loops stop there); the upgrade tables are indexed by the weapon's id. */
ForgeRecipe gArmorRecipes[518] = {
#include "lobby/00802C70.inc"
};
ForgeRecipe gWeaponRecipes[237] = {
#include "lobby/00805D00.inc"
};
WeaponUp gBladeUpgrades[486] = {
#include "lobby/00807340.inc"
};
WeaponUp gGunUpgrades[128] = {
#include "lobby/0080A0D0.inc"
};
