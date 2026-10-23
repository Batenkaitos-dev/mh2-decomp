#ifndef LOBBY_ENTER_H
#define LOBBY_ENTER_H

#include "common.h"
#include "lobby_net.h"

/*
 * The lobby's re-entry (LOBBY_PHASE_ENTER): back to the lobby server's town
 * after a quest or when a group was joined. LobbyEnter_Begin opens it and
 * LobbyEnter_Run runs it, starting from the server's answer to where the
 * player was (LobbyStep_CurrentPlace).
 */

void LobbyEnter_Begin(void);
void LobbyEnter_Run(void);
s32 LobbyStep_CurrentPlace(void);
void LobbyStep_CurrentPlaceDone(NetMsg msg, NetMsg *msgPtr);

#endif
