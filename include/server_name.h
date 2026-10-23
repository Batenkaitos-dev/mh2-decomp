#ifndef SERVER_NAME_H
#define SERVER_NAME_H

#include "common.h"
#include "lobby_net.h"

/*
 * The lobby's server name request (LobbyStep_ServerName and its reply, which
 * gives the current server's name), and Lobby_IgnoreReply, a reply handler
 * that ignores the reply.
 */

void Lobby_IgnoreReply(NetMsg msg, NetMsg *msgPtr);
void LobbyStep_ServerNameDone(NetMsg msg, NetMsg *msgPtr);
s32 LobbyStep_ServerName(void);

#endif
