#ifndef NET_FILE_SAVE_H
#define NET_FILE_SAVE_H

#include "common.h"

/*
 * The NET file's saves to the memory card, one step a frame:
 * LobbyCard_SaveNetFile (the set-up's save) and LobbyCard_RunNetSave (the
 * save from the lobby), with the checks of a file read back
 * (LobbyCard_CheckNetFile, LobbyCard_DecodeNetFile,
 * LobbyCard_CheckNetValues), the mend of its settings
 * (LobbyCard_FixNetValues), and the countdown of their messages
 * (LobbyMsg_DrawCountdown).
 */

s32 LobbyCard_LoadFiles(s32 mode);
s32 LobbyCard_SaveNetFile(void);
void LobbyMsg_DrawCountdown(u8 mode);
void LobbyCard_ResetNetSave(void);
s32 LobbyCard_RunNetSave(void); /* saves the network file: 0 busy, 1 done, -1 failed */
s32 NetMode_CardMsgError(s8 msg);
s32 LobbyCard_CheckNetValues(u8 *buf);
void LobbyCard_FixNetValues(u8 *data);
s32 LobbyCard_CheckNetFile(s32 unused);
s32 LobbyCard_DecodeNetFile(u16 *words);

#endif
