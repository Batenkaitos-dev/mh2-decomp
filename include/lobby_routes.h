#ifndef LOBBY_ROUTES_H
#define LOBBY_ROUTES_H

#include "common.h"
#include "lobby_net.h"

/*
 * The lobby's route handlers: LobbyRoute_Register registers one handler per
 * route the lobby server sends on its own (NET_ROUTE_*), and each handler
 * (LobbyRoute_*) fetches its message's body and applies it: the session
 * closed or the player sent out, the chat, the level lists' entries, the
 * members coming and going.
 */

void LobbyRoute_Register(void);
void LobbyRoute_TownData(NetMsg msg, NetMsg *msgRef);
void LobbyRoute_PartyReady(NetMsg msg, NetMsg *msgRef);
void LobbyRoute_ServerClose(NetMsg msg, NetMsg *msgRef);
void LobbyRoute_DepartCancel(NetMsg msg, NetMsg *msgRef);
void LobbyRoute_DepartError(NetMsg msg, NetMsg *msgRef);
void LobbyRoute_ServerLeave(NetMsg msg, NetMsg *msgRef);
void LobbyRoute_ChatMessage(NetMsg msg, NetMsg *msgRef);
void LobbyRoute_ChatTell(NetMsg msg, NetMsg *msgRef);
void LobbyRoute_ShortMail(NetMsg msg, NetMsg *msgRef);
void LobbyRoute_Level0Name(NetMsg msg, NetMsg *msgRef);
void LobbyRoute_Level0Values(NetMsg msg, NetMsg *msgRef);
void LobbyRoute_Level0State(NetMsg msg, NetMsg *msgRef);
void LobbyRoute_Level1Values(NetMsg msg, NetMsg *msgRef);
void LobbyRoute_Level1Text(NetMsg msg, NetMsg *msgRef);
void LobbyRoute_Level1GroupInfo(NetMsg msg, NetMsg *msgRef);
void LobbyRoute_Level1Name(NetMsg msg, NetMsg *msgRef);
void LobbyRoute_Level1State(NetMsg msg, NetMsg *msgRef);
void LobbyRoute_Level1Color(NetMsg msg, NetMsg *msgRef);
void LobbyRoute_Level1UserIn(NetMsg msg, NetMsg *msgRef);
void LobbyRoute_Level1UserOut(NetMsg msg, NetMsg *msgRef);
void LobbyRoute_Level2Name(NetMsg msg, NetMsg *msgRef);
void LobbyRoute_Level2State(NetMsg msg, NetMsg *msgRef);
void LobbyRoute_Level2Color(NetMsg msg, NetMsg *msgRef);
void LobbyRoute_Level2Values(NetMsg msg, NetMsg *msgRef);
void LobbyRoute_Level2Limits(NetMsg msg, NetMsg *msgRef);
void LobbyRoute_Level2Locked(NetMsg msg, NetMsg *msgRef);
void LobbyRoute_Level2Text(NetMsg msg, NetMsg *msgRef);
void LobbyRoute_Level2PostedQuest(NetMsg msg, NetMsg *msgRef);
void LobbyRoute_Level2Members(NetMsg msg, NetMsg *msgRef);
void LobbyRoute_Level2UserIn(NetMsg msg, NetMsg *msgRef);
void LobbyRoute_Level2UserOut(NetMsg msg, NetMsg *msgRef);
void LobbyRoute_TownMsg(NetMsg msg, NetMsg *msgRef);
void LobbyRoute_Level0Kick(NetMsg msg, NetMsg *msgRef);
void LobbyRoute_Level1Kick(NetMsg msg, NetMsg *msgRef);
void LobbyRoute_Level2Kick(NetMsg msg, NetMsg *msgRef);
void LobbyRoute_UserCard(NetMsg msg, NetMsg *msgRef);
void LobbyRoute_UserCardPart(NetMsg msg, NetMsg *msgRef);

#endif
