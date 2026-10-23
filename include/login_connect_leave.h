#ifndef LOGIN_CONNECT_LEAVE_H
#define LOGIN_CONNECT_LEAVE_H

#include "common.h"

/*
 * The login's stages that the lobby overlay's scene task runs as its online
 * steps: the connection to the lobby server (LobbyLogin_RunConnect), the
 * save and exit (LobbyLogin_SaveAndExit) and the leave (LobbyLogin_Leave),
 * with the steps they share (the error and failed connection messages, the
 * leave's exits).
 */

void LobbyLogin_BlinkWait(void);
s32 LobbyLogin_RunConnect(void);
s32 LobbyLogin_SaveAndExit(void);
s32 LobbyLogin_Leave(void);
s32 LobbyLogin_ShowNetError(void);
s32 LobbyLogin_LeaveToSettings(void);
s32 LobbyLogin_LeaveRelease(void);
s32 LobbyLogin_ShowConnFailed(void);

#endif
