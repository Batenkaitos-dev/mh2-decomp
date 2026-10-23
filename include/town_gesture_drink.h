#ifndef TOWN_GESTURE_DRINK_H
#define TOWN_GESTURE_DRINK_H

#include "common.h"
#include "player.h"

/*
 * The town player's gesture request and drink count: TownPc_StartGesture
 * starts the gesture a player asked for (Player.actRequest), and
 * TownPc_CountDrink counts a drink as a seat's drink state starts (online,
 * the tenth makes the player drunk).
 */

void TownPc_StartGesture(Player *pl);
void TownPc_CountDrink(Player *pl);

#endif
