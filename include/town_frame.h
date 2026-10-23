#ifndef TOWN_FRAME_H
#define TOWN_FRAME_H

#include "common.h"

/*
 * The town's frame and its modes: Town_Frame runs the town's (and the
 * village's) frame, and Town_UpdateMode runs the mode the town is in
 * (gTownMode, enum TownMode), one counter's or menu's update per mode. With
 * them, whether a mode holds the player (Town_IsPlayerHeld), the reload of
 * the local player's look, the label list the counters draw, the online
 * house's host check, the hunter card's parts sent as they change, and the
 * placing of the local player at an area's entry.
 */

void Town_ReloadLocalPlayer(void);
void Menu_DrawLabelList(s16 x, char **lines, s16 n, s16 cur, u32 color);
void Town_Frame(void);
s32 Town_IsPlayerHeld(void);
s32 Town_IsIdleOnline(void);
void MyHouse_CheckHost(void);
void Town_UpdateTextParts(void);
void Town_UpdateMode(void);
s32 Town_ModeDisconnect(void);
void Town_PlaceLocalPlayer(s32 area);
void Town_SetDrawSky(s8 enabled);

#endif
