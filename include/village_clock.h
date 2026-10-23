#ifndef VILLAGE_CLOCK_H
#define VILLAGE_CLOCK_H

#include "common.h"

/*
 * The village clock in the town's top-left corner (VillageClock_Draw), and
 * the hooks of the lobby's world clock that set it moving: the hand's
 * wobble each hour (VillageClock_Wobble), and LobbyWorld_OnHalfDay and
 * LobbyWorld_OnNewSeason, which also renew the village's offers offline and
 * the quest lists.
 */

void VillageClock_Wobble(void);
void LobbyWorld_OnHalfDay(void);
void LobbyWorld_OnNewSeason(void);
void VillageClock_DrawDays(void);
void VillageClock_Draw(void);
extern u32 gSeasonColors[4];

#endif
