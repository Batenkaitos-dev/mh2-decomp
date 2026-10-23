#ifndef LOBBY_ROOM_H
#define LOBBY_ROOM_H

#include "common.h"
#include "lobby_net.h"

/*
 * The lobby's room phase, the quest's party ready to set out.
 * Lobby_StartRoomPhase enters it and LobbyRoom_Run runs it: the party is
 * fetched, then the lobby server is left for the quest. LobbyStep_SetOut is
 * the town's request when the party sets out, LobbyPkt_QuestPaidSend tells
 * the server the quest's fee was paid, and LobbyRoom_BeginClose and
 * LobbyRoom_RunClose run the room's closing under the player.
 */

void Lobby_StartRoomPhase(void);
void LobbyPkt_QuestPaidSend(void);
void LobbyRoom_Run(void);
void LobbyRoom_PartyDone(NetMsg msg, NetMsg *msgPtr);
void LobbyRoom_LobbyLeaveDone(NetMsg msg, NetMsg *msgPtr);
s32 LobbyStep_SetOut(void);
void LobbyStep_SetOutDone(NetMsg msg, NetMsg *msgPtr);
void LobbyRoom_BeginClose(u8 kind);
void LobbyRoom_RunClose(void);

#endif
