#ifndef LOBBY_DIALOG_BOX_H
#define LOBBY_DIALOG_BOX_H

#include "common.h"

/*
 * The lobby's message dialog (LobbyDialog_*, LobbyText_Open), with its
 * choices, page count, waiting text or countdown; the help line at the foot
 * of the screen (LobbyHelp_*); the screen's title (LobbyTitle_*); and a
 * user's name and id under their labels (LobbyMenu_DrawNameId).
 */

void LobbyDialog_OpenWait(void *text);
void LobbyDialog_OpenCountdown(void *text, s32 countdown);
void Menu_SetYesNo(s8 choice);
s32 LobbyDialog_Clear(void);
void LobbyDialog_Grow(void);
s16 LobbyDialog_GetGrow(void);
void LobbyDialog_SetMember(void *id, void *name);
void LobbyDialog_ClearMember(void);
void LobbyText_Open(s32 msg);
void LobbyTitle_Set(s32 idx);
void LobbyHelp_Set(s32 i);
void LobbyHelp_SetText(void *text);
void LobbyHelp_SetBox(s16 x, s16 y, s16 w);
void LobbyDialog_DrawWin(s16 x, s16 y, s16 w, s16 height, s32 fillColor);
void LobbyDialog_Draw(void);
void LobbyDialog_DrawMember(void);
void LobbyTitle_Draw(void);
void LobbyHelp_Draw(void);
void LobbyMenu_DrawNameId(s16 y, char *name, char *id);
void LobbyDialog_ShowConnMsg(s32 stage);

#endif
