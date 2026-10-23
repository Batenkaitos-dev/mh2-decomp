#ifndef FALLING_LEAVES_H
#define FALLING_LEAVES_H

#include "common.h"
#include "sprite.h"
#include "lobby_vobj.h"

/*
 * The village's falling leaves (LobbyFlock), a sub_main object with draw
 * slots of its own: LobbyFlock_Spawn starts the 16 pieces that tumble down
 * over a patch of AREA_ONLINE_VILLAGE outside the first season, the
 * LobbyFlock_* states run them, and LobbyFlock_DrawPart draws each piece.
 */

void LobbyFlock_Spawn(void);
void LobbyFlock_Run(LobbyFlock *flock);
void LobbyFlock_Init(LobbyFlock *flock);
void LobbyFlock_Update(LobbyFlock *flock);
void LobbyFlock_End(LobbyFlock *flock);
void LobbyFlock_Free(void *flock);
void LobbyFlock_DrawPart(DrawSlot *slot);

#endif
