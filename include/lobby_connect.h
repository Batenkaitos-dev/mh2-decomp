#ifndef LOBBY_CONNECT_H
#define LOBBY_CONNECT_H

#include "common.h"

/*
 * The lobby server's connection. Net_Connect opens a TCP connection,
 * Lobby_RecvPacket receives the server's messages a part at a time, and
 * LobbyNet_DisconnectWait waits for the line to close. NetScreen_ConnectStep
 * runs the connection's steps, LobbyKddi_PortalStep the KDDI portal's pages
 * before the lobby, and NetScene_BackToPortal leaves for the portal's quit
 * page.
 */

void NetScene_BackToPortal(void);
s32 LobbyNet_DisconnectWait(void);
s32 Net_Connect(s32 addr, u16 port, u16 openArg);
s32 Lobby_RecvPacket(s32 sock, u8 *hdr, u8 *data, u32 max);
s32 LobbyKddi_PortalStep(void);
s32 NetScreen_ConnectStep(void);

#endif
