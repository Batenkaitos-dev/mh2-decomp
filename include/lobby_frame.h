#ifndef LOBBY_FRAME_H
#define LOBBY_FRAME_H

#include "common.h"
#include "menu_task.h"

/*
 * The online lobby's frame: Lobby_Tick runs once a frame online (the
 * receive, the timeouts, the clock and the timers, then the current screen),
 * LobbyMenu_QueueLayers puts the menu's draw layers into the frame's text
 * draw lists, and LobbyMenu_DrawDialog draws the dialog layer.
 */

s32 Lobby_Tick(void);
void LobbyMenu_QueueLayers(void);
void LobbyMenu_DrawDialog(MenuTask *task);

#endif
