#ifndef SHORT_MAIL_H
#define SHORT_MAIL_H

#include "common.h"
#include "lobby_menu.h"
#include "menu_task.h"

/*
 * The short mail form (gShortMailMenu): a mail of up to 120 bytes to one
 * hunter, opened from the mail menu, from another hunter's details or from
 * the game menu's mail page (ShortMail_Open, with its SHORT_MAIL_* flags).
 * ShortMail_Step runs the form and sends the mail; ShortMail_DrawForm draws
 * a mail's form, for received mails too.
 */

void ShortMail_Open(void *id, void *name, u32 flags);
void ShortMail_Close(void);
s32 ShortMail_Step(u16 pad);
s32 ShortMail_Input(ShortMailMenu *menu, u16 pad);
void ShortMail_DrawForm(MailEntry *entry, s16 sel, s32 flags, s16 x, s16 y);
void ShortMail_Draw(MenuTask *task);
void ShortMail_Draw2(MenuTask *task);

#endif
