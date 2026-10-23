#ifndef MENU_STEPS_H
#define MENU_STEPS_H

#include "common.h"
#include "lobby_menu.h"

/*
 * The lobby menus' small common steps, each called once a frame by a menu's
 * input: the cursors (Menu_Wrap*, Pad_StepWrap, Menu_GridMove,
 * Menu_CountPages), the yes/no answers (Menu_YesNoInput,
 * LobbyMenu_YesNoInput), a window's slide and shake, the notices
 * (Menu_PushNotice*) and the chat's system messages. The steps that run over
 * frames return the menus' MENU_* values.
 */

s32 Menu_WrapUpDown(s32 cur, s32 n);
s32 Menu_WrapUpDownPad(s32 cur, s32 n, u16 pad);
s32 Menu_WrapUpDownSe(s32 cur, s32 n, s32 sound);
s32 Menu_WrapLeftRight(s32 cur, s32 n);
s32 Menu_WrapLeftRightSe(s32 cur, s32 n, s32 sound);
s32 Menu_WrapLeftRightPad(s32 cur, s32 n, u16 pad);
s32 Pad_StepWrap(s32 cur, s32 n, u16 pad);
s16 Menu_GridMove(u8 *linep, s16 *pagep, s32 perPage, s16 total, u16 pad);
s32 Menu_YesNoInput(void);
s32 LobbyMenu_YesNoInput(s32 sound);
s32 WinSlide_Step(WinSlide *slide, s16 fromPos, s16 toPos);
s32 Win_Shake(WinShake *shake);
s32 Menu_CountPages(s16 count, s16 perPage);
s32 Menu_PushNoticeOnce(s32 idx);
void Menu_PushNotice(s32 msg);
void Chat_SystemMsg(s32 msg, char *arg);

#endif
