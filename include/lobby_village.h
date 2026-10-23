#ifndef LOBBY_VILLAGE_H
#define LOBBY_VILLAGE_H

#include "common.h"
#include "lobby_net.h"

/*
 * The lobby's village phase (LOBBY_PHASE_VILLAGE), where the player walks an
 * area's village alone: LobbyVillage_Begin* start it and LobbyVillage_Run
 * runs it; LobbyLevel1_Enter enters a town of the area, and
 * LobbyLevel0_Leave and LobbyLevel0_LeaveCountdown leave the area.
 */

void LobbyVillage_Begin(void);
void LobbyVillage_BeginStay(void);
void LobbyVillage_BeginReturn(s32 showText);
void LobbyVillage_BeginLeave(void);
s32 LobbyVillage_IsLeaving(void);
void LobbyVillage_Run(void);
void LobbyVillage_BeginJoin(void);
s32 LobbyLevel1_Enter(void);
s32 LobbyVillage_FetchTowns(void);
void LobbyLevel0_OnLeaveReply(NetMsg msg, NetMsg *msgPtr);
s32 LobbyLevel0_Leave(void);
void LobbyLevel0_LeaveCountdown(void);
void LobbyLevel0_OnCountdownReply(NetMsg msg, NetMsg *msgPtr);

#endif
