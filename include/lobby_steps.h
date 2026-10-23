#ifndef LOBBY_STEPS_H
#define LOBBY_STEPS_H

#include "common.h"
#include "lobby_net.h"
#include "lobby_state.h"

/*
 * The lobby's request steps (LobbyStep_*): each sends one request, with its
 * reply handler (LobbyStep_*Done). A screen calls a step once a frame; it
 * returns LOBBY_STEP_BUSY until the reply has come, then LOBBY_STEP_DONE or
 * LOBBY_STEP_FAILED. Lobby_ApplyTownData applies the town data the login
 * fetches.
 */

s32 LobbyStep_HunterCard(void);
void LobbyStep_HunterCardDone(NetMsg msg, NetMsg *msgPtr);
s32 LobbyStep_LoginInfo(void);
void LobbyStep_LoginInfoDone(NetMsg msg, NetMsg *msgPtr);
s32 LobbyStep_CharaSelectText(void);
void LobbyStep_CharaSelectTextDone(NetMsg msg, NetMsg *msgPtr);
s32 LobbyStep_ChatColorMode(void);
void LobbyStep_ChatColorModeDone(NetMsg msg, NetMsg *msgPtr);
s32 LobbyStep_QuestRecord(void);
void LobbyStep_QuestRecordDone(NetMsg msg, NetMsg *msgPtr);
void Lobby_ApplyTownData(void);
s32 LobbyStep_TownData(u8 first, u8 last);
void LobbyStep_TownDataDone(NetMsg msg, NetMsg *msgPtr);
s32 LobbyStep_TownClock(void);
void LobbyStep_TownClockDone(NetMsg msg, NetMsg *msgPtr);
s32 LobbyStep_Level1List(s32 id);
void LobbyStep_Level1AllowedDone(NetMsg msg, NetMsg *msgPtr);
void LobbyStep_Level1ListDone(NetMsg msg, NetMsg *msgPtr);
s32 LobbyStep_Level1Enter(void);
void LobbyStep_Level1EnterDone(NetMsg msg, NetMsg *msgPtr);
s32 LobbyStep_Level1Users(void);
void LobbyStep_Level1UsersDone(NetMsg msg, NetMsg *msgPtr);
s32 LobbyStep_Level1Open(s32 id);
void LobbyStep_Level1SettingsDone(NetMsg msg, NetMsg *msgPtr);
s32 LobbyStep_Level1Settings(char *text, u8 *groupBytes);
s32 LobbyLevel1_CanEnter(s32 id);
s32 LobbyStep_EnterLevel1(void);
s32 LobbyStep_Level1SetColor(s32 color);
s32 LobbyStep_RefreshLevel1Users(void);
void LobbyStep_FetchLevel1UsersDone(NetMsg msg, NetMsg *msgPtr);
s32 LobbyStep_DownloadQuests(void);
void LobbyStep_DlListDone(NetMsg msg, NetMsg *msgPtr);
void LobbyStep_DownloadDone(NetMsg msg, NetMsg *msgPtr);
s32 LobbyStep_Level1Leave(void);
void LobbyStep_Level1LeaveDone(NetMsg msg, NetMsg *msgPtr);
s32 LobbyStep_Level2Entry(u16 id, NetUser *users);
void LobbyStep_Level2RecsDone(NetMsg msg, NetMsg *msgPtr);
void LobbyStep_Level2UsersDone(NetMsg msg, NetMsg *msgPtr);
s32 HunterCard_SendChanged(void);
void HunterCard_SendOnlineMark(void);
void HunterCard_SendPart(u16 offset, u16 count);

#endif
