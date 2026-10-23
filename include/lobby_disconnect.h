#ifndef LOBBY_DISCONNECT_H
#define LOBBY_DISCONNECT_H

#include "common.h"
#include "menu_task.h"

/*
 * The lobby's way out. Lobby_Disconnect leaves the lobby for a reason
 * (LOBBY_QUIT_*), after sending the player's changed text
 * (Lobby_UpdatePlayerText); the exit phase's steps (LobbyExit_Run,
 * LobbyExit_Draw) fade the screen, send the leave or logout request
 * (LobbyLeave_*) and go on to the next screen, showing a cut-short session's
 * reason first (LobbyExit_ShowReason).
 */

void Lobby_UpdatePlayerText(void);
void Lobby_Disconnect(s32 reason);
void LobbyExit_Run(void);
void LobbyExit_ShowReason(s32 reason);
void LobbyLeave_Reset(void);
s32 LobbyLeave_Run(u8 reason);
void LobbyLeave_OnReply(void);
void LobbyExit_Draw(MenuTask *layer);

#endif
