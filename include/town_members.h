#ifndef TOWN_MEMBERS_H
#define TOWN_MEMBERS_H

#include "common.h"
#include "lobby_state.h"
#include "player.h"

/*
 * The town's players as the lobby's members come and go:
 * TownPc_SyncMembers, each frame, removes the players of the members who
 * left, applies the hunter cards that changed and spawns the players of the
 * members who joined. The players' models load and free through
 * TownPc_LoadAllModels, TownPc_LoadModel, TownPc_StartLoad and TownPc_Free,
 * and TownPc_IsLoaded tells when a player is ready; TownPc_Spawn and
 * TownPc_InitAll set the players up.
 */

s32 TownPc_IsLoaded(s8 pl);
void TownPc_SyncMembers(void);
void TownPc_LoadAllModels(void);
void TownPc_Spawn(u8 pl, NetUser *member, char *name);
void TownPc_InitAll(s32 withDrawSlots);
void TownPc_LoadModel(Player *pl, s32 modeArg);
void TownPc_StartLoad(Player *pl);
void TownPc_LoadModelNow(Player *pl);
void TownPc_Free(Player *pl, s32 modeArg);
void TownPc_Remove(Player *pl);
void TownPc_FreeChangedModels(Player *pl);

#endif
