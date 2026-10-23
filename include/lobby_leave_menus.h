#ifndef LOBBY_LEAVE_MENUS_H
#define LOBBY_LEAVE_MENUS_H

#include "common.h"
#include "lobby_menu.h"
#include "lobby_net.h"

/*
 * The lobby's leave steps (leaving the quest room, taking back a setting
 * out, and Lobby_HandleLeaveRequest when the server has closed or sent the
 * player out), the short mail's sending, and the character select menu
 * (IdMenu_*).
 */

s32 LobbyStep_Level2Leave(void);
void LobbyStep_Level2LeaveDone(NetMsg msg, NetMsg *msgPtr);
s32 LobbyStep_SetOutLeave(void);
void LobbyStep_SetOutLeaveDone(NetMsg msg, NetMsg *msgPtr);
s32 Lobby_HandleLeaveRequest(void);
void LobbyStep_ShortMailSendDone(NetMsg msg, NetMsg *msgPtr);
s32 IdMenu_Run(void); /* an s8, used as an int */
void IdMenu_Choose(void);
void IdMenu_Quit(void);
void IdMenu_Draw(void);
void IdMenu_Redraw(MenuTask *layer);

#endif
