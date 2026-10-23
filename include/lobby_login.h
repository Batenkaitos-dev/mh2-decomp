#ifndef LOBBY_LOGIN_H
#define LOBBY_LOGIN_H

#include "common.h"
#include "menu_task.h"
#include "lobby_net.h"

/*
 * The lobby's login (LOBBY_PHASE_ENTRY). LobbyEntry_Run runs the login's
 * mode, which LobbyLogin_OnProgress picks from the login task's progress;
 * LobbyLogin_NoticeApplies and LobbyLogin_InfoApplies say whether the
 * server's notice and information text are meant for the way the lobby was
 * reached; LobbyLogin_SendSummary sends the save's summary.
 * Lobby_MarkDisconnected marks the server's connection as lost.
 */

void LobbyEntry_Run(void);
s32 LobbyEntry_AtWebPage(void);
void LobbyLogin_OnProgress(NetMsg msg, NetMsg *msgRef);
s32 LobbyLogin_NoticeApplies(u8 target);
s32 LobbyLogin_InfoApplies(u8 target);
s32 LobbyLogin_SendSummary(void);
void LobbyLogin_OnSummaryReply(NetMsg msg, NetMsg *msgRef);
void LobbyEntry_Draw(MenuTask *task);
void Lobby_MarkDisconnected(void);

#endif
