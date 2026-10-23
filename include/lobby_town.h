#ifndef LOBBY_TOWN_H
#define LOBBY_TOWN_H

#include "common.h"
#include "lobby_menu.h"
#include "lobby_net.h"

/*
 * The lobby's town phase (LOBBY_PHASE_TOWN): LobbyTown_Begin,
 * LobbyTown_BeginRunning and LobbyTown_BeginLeave start it, and
 * LobbyTown_Run runs it, entering and leaving the town. With it, the town's
 * requests of the server's second level (the search for groups,
 * LobbyLevel1_*) and third level (the quest rooms, LobbyLevel2_*).
 */

void LobbyTown_Begin(void);
void LobbyTown_BeginRunning(void);
void LobbyTown_BeginLeave(void);
s32 LobbyTown_IsLeaving(void);
void LobbyTown_Run(void);
void LobbyTown_Enter(void);
void LobbyTown_Leave(void);
void LobbyTown_OnUsersReply(NetMsg msg, NetMsg *msgPtr);
void LobbyLevel1_OnLevel1SearchReply(NetMsg msg, NetMsg *msgPtr);
s32 LobbyLevel1_Search(NetLevel1SearchQuery *query);
void LobbyLevel1_OnLevel1SearchInfoReply(NetMsg msg, NetMsg *msgPtr);
s32 LobbyLevel1_FetchInfo(u8 i);
void LobbyLevel2_Refresh(void);
s32 LobbyLevel2_FetchList(void);
void LobbyLevel2_OnListDone(NetMsg msg, NetMsg *msgPtr);
s32 LobbyLevel2_EnterFree(s32 value);
void LobbyLevel2_OnLevel2OpenReply(NetMsg msg, NetMsg *msgPtr);
s32 LobbyLevel2_FetchRecords(void);
void LobbyLevel2_OnRecsDone(NetMsg msg, NetMsg *msgPtr);
s32 LobbyLevel2_SendSettings(void);
void LobbyLevel2_OnSettingsDone(NetMsg msg, NetMsg *msgPtr);
s32 LobbyLevel2_SendQuestAccept(u32 value);
void LobbyLevel2_OnQuestAcceptReply(NetMsg msg, NetMsg *msgPtr);
s32 LobbyLevel2_SendQuestPost(MsgQuestPost *post);
void LobbyLevel2_OnQuestPostReply(NetMsg msg, NetMsg *msgPtr);
s32 LobbyLevel2_EnterWithPass(s32 value);
void LobbyLevel2_OnLevel2EnterReply(NetMsg msg, NetMsg *msgPtr);
void LobbyLevel2_OnLevel2UsersReply(NetMsg msg, NetMsg *msgPtr);
void LobbyLevel2_OnLevel2ValuesReply(NetMsg msg, NetMsg *msgPtr);
void LobbyLevel2_OnLevel2MembersReply(NetMsg msg, NetMsg *msgPtr);

#endif
