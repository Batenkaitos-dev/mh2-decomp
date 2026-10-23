#ifndef PUSH_APART_H
#define PUSH_APART_H

#include "common.h"
#include "player.h"

/*
 * The push that keeps bodies from overlapping: Field_PushApart, once a
 * frame, pushes the players out of each other (Players_PushApart) and out
 * of the monsters (PlayerEm_PushApart), and the monsters out of each other
 * (EmEm_PushApart). The body with the lower push priority moves, both
 * halfway when they are equal.
 */

void PlayerEm_PushApart(Player *pl, Player *em);
void EmEm_PushApart(Player *emA, Player *emB);
void Players_PushApart(Player *plA, Player *plB);
void Field_PushApart(void);

#endif
