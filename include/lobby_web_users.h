#ifndef LOBBY_WEB_USERS_H
#define LOBBY_WEB_USERS_H

#include "common.h"
#include "lobby_net.h"

/*
 * The lobby's web pages (LobbyWeb_Run), the move into an area and a group of
 * the town (LobbyLogin_EnterEntry, after Lobby_CanEnterByRank's check of the
 * hunter rank), and the lookups of other users (LobbyUser_*: a user's
 * details, or users by id, by name or by conditions).
 */

void LobbyWeb_Run(void);
s32 LobbyKddi_WaitPage(void);
s32 LobbyLogin_EnterEntry(s32 entry, s32 sub);
void LobbyLogin_OnEnterLevelsReply(NetMsg msg, NetMsg *msgRef);
void LobbyUser_OnUserCardFetchReply(NetMsg msg, NetMsg *msgRef);
s32 LobbyUser_FetchDetails(void *profile, void *id);
s32 LobbyUser_FindById(s8 *id);
void LobbyUser_OnReply(NetMsg msg, NetMsg *msgRef);
s32 LobbyUser_FindByName(s8 *name);
void LobbyUser_OnUserFindByNameReply(NetMsg msg, NetMsg *msgRef);
s32 LobbyUser_Search(NetUserSearchCond *cond, s8 n);
void LobbyUser_OnUserSearchReply(NetMsg msg, NetMsg *msgRef);
s32 Lobby_CanEnterByRank(s32 area, s32 again);

#endif
