/*
 * The town player's gesture request and drink count.
 *
 * TownPc_StartGesture starts the gesture a player requested (actRequest):
 * the gesture group's state from gTownGestureStates. The input and the
 * gesture states call it. TownPc_CountDrink counts a drink as a seat's
 * drink state starts; online, the tenth makes the player drunk.
 *
 * Built at optimization level 4, as the town players' code around it.
 */

#include "common.h"
#include "lobby_menu.h"
#include "player.h"
#include "em.h"
#include "objhead.h"
#include "area.h"
#include "town_gesture_drink.h"
#include "game_util.h"
#include "town_players.h"

#pragma optimization_level 4

/* The gesture group's state for each gesture request (actRequest). */
u8 gTownGestureStates[16] = {
#include "lobby/00802C40.inc"
};

/*
 * Starts the requested gesture: the gesture group's state from the town's
 * gesture table, when actRequest is set (as sub_main's
 * Player_StartGesture).
 */
void TownPc_StartGesture(Player *pl)
{
    u8 gesture;

    gesture = pl->actRequest;
    if (gesture != 0) {
        Actor_SetStateFlags(pl, 1, gTownGestureStates[gesture], 0);
    }
}

/*
 * Counts a drink (the drink state's start); online, the tenth makes the
 * player drunk.
 */
void TownPc_CountDrink(Player *pl)
{
    if (pl->drunk == 0) {
        pl->drinkCount = (pl->drinkCount + 1);
        if (((s32) pl->drinkCount >= 10) && (Game_IsOnline() == 1)) {
            pl->drunk = 1U;
        }
    }
}
