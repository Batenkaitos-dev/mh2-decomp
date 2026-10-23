#ifndef LOBBY_SCREEN_H
#define LOBBY_SCREEN_H

#include "common.h"
#include "lobby_state.h"

/*
 * The lobby screen, the frame of the town and the village: LobbyScene_Run,
 * LobbyScreen_Run and LobbyScreen_RunStep run its step by gLobbyScreenMode,
 * LobbyScreen_EnterSeq enters it in one of the ways of enum LobbyEnterMode,
 * and LobbyScreen_FadeOut leaves it. gLobbyStartPos holds the start
 * positions the tours set.
 */

void LobbyScene_Run(void);
s32 LobbyScreen_Run(void);
void LobbyScreen_InitLists(void);
s32 LobbyScreen_EnterSeq(s32 mode);
s32 LobbyArea_StartEvent(void);
s32 LobbyScreen_FadeOut(s32 mode);
s32 LobbyScreen_LoadBanks(void);
void LobbyScreen_Start(void);
void LobbyScreen_EnterArea(void);
void LobbyScreen_LeaveArea(void);
void LobbyScreen_RunTour(void);
s32 LobbyScreen_TourKind(void);
s32 LobbyScreen_IsTouring(void);
s32 LobbyScreen_RunStep(void);
extern LobbyStartPos gLobbyStartPos;

#endif
