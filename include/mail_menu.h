#ifndef MAIL_MENU_H
#define MAIL_MENU_H

#include "common.h"
#include "lobby_menu.h"
#include "menu_task.h"

/*
 * The mail menu: the mailbox of the last mails received, the newest first,
 * then a mail with its actions: reply with a short mail, add the sender as a
 * friend, or block the sender. MailMenu_Open opens it, MailMenu_Update runs
 * it and MailMenu_Draw draws it.
 */

void MailMenu_Open(void);
s32 MailMenu_Update(u16 pad);
s32 MailMenu_ActionInput(MailMenu *menu, u16 pad);
s32 MailMenu_ListInput(MailMenu *menu, u16 pad);
void MailMenu_DrawMailbox(MailMenu *menu, s16 x, s16 y);
void MailMenu_DrawActions(MailMenu *menu, s16 x, s16 y);
void MailMenu_Draw(MenuTask *task);

#endif
