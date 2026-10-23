/*
 * The mail menu: the mailbox (gMailBox, the last MAIL_BOX_LEN mails
 * received, the newest first), then a mail with its actions: reply (the
 * short mail form, ShortMail_Open and ShortMail_Step), add the sender as a
 * friend, or block the sender (the friend and block lists' questions).
 * Its state is a MailMenu in gMenuWork (lobby_menu.h). MailMenu_Open and
 * MailMenu_Close open and end it, MailMenu_Update runs it and MailMenu_Draw
 * draws it.
 */
#include "common.h"
#include "lobby_text.h"
#include "em.h"
#include "item.h"
#include "scene.h"
#include "player.h"
#include "save.h"
#include "pad.h"
#include "lobby_state.h"
#include "m2c_macros.h"
#include "lobby_globals.h"
#include "lobby_vobj.h"
#include "lobby_menu.h"
#include "lobby_labels.h"
#include "menu_task.h"
#include "text.h"
#include "mail_menu.h"
#include "font.h"
#include "friend_add.h"
#include "item_shop.h"
#include "lobby_util.h"
#include "menu_steps.h"
#include "menu_widgets.h"
#include "short_mail.h"
#include "libc.h"

extern MailEntry gMailBox[MAIL_BOX_LEN];
extern u8 gMenuWork;
void Win_Draw(s16, s16, s16, s16, s32, s32, s32); /* unit's own declaration: declared as this file's callers use it, with s16 sizes; with the definition's s32 sizes and colours and s16 kind, the calls compile differently */
void Gfx_DrawLine(s16 x, s16 y, s16 w, s16 h, s32 color);

/* The game built this file at optimization level 4: the pragma gives its code. */
#pragma optimization_level 4

/* Opens the mail menu on the mailbox (gMenuWork as the MailMenu), drawn by
 * MailMenu_Draw, and takes the field's control from the player. */
void MailMenu_Open(void)
{
    MailMenu *menu = (MailMenu *)&gMenuWork;

    memset(menu, 0, MENU_WORK_SIZE);
    menu->actions[0] = MAIL_ACT_REPLY;
    menu->actions[1] = MAIL_ACT_ADD_FRIEND;
    menu->actions[2] = MAIL_ACT_BLOCK;
    menu->count = MAIL_ACTS;
    WinSlide_Step(&menu->win, 640, 280);
    LobbyMenu_SetDraw(0, MailMenu_Draw, 0, 0);
    gFieldControl = 0;
}

/* Ends the mail menu: stops its drawing and gives the field back. */
void MailMenu_Close(void)
{
    gFieldControl = 1;
    LobbyMenu_SetDraw(0, 0, NULL, 0);
}

/*
 * Runs a frame of the mail menu: the mailbox's list, then the chosen mail
 * (marked read) with its actions: reply (the short mail form), add the
 * sender as a friend, or block the sender. Returns MENU_RUNNING, or
 * MENU_CANCEL when the mailbox is left with cross and MENU_CLOSE with start
 * (the menu closed). After a friend's or a reply's MENU_CLOSE the menu is
 * left open.
 */
s32 MailMenu_Update(u16 pad)
{
    MailMenu *menu = (MailMenu *)&gMenuWork;

    switch (menu->state) {
    case MAIL_MENU_SLIDE_IN:
        switch (WinSlide_Step(&menu->win, 640, 280)) {
        case MENU_DONE:
            menu->state++;
            break;
        }
        break;
    case MAIL_MENU_LIST:
        switch (MailMenu_ListInput(menu, pad)) {
        case MENU_DONE:
            menu->state++;
            menu->cursor = 0;
            gMailBox[menu->sel].unread = 0;
            memcpy(&menu->mail, &gMailBox[menu->sel], sizeof(MailEntry));
            WinSlide_Step(&menu->win2, 640, 280);
            break;
        case MENU_CANCEL:
            MailMenu_Close();
            return MENU_CANCEL;
        case MENU_CLOSE:
            MailMenu_Close();
            return MENU_CLOSE;
        }
        break;
    case MAIL_MENU_ACTIONS:
        switch (MailMenu_ActionInput(menu, pad)) {
        case MENU_DONE:
            switch (menu->actions[menu->cursor]) {
            case MAIL_ACT_REPLY:
                menu->state = MAIL_MENU_REPLY;
                ShortMail_Open(menu->mail.id, menu->mail.name, SHORT_MAIL_TO_HUNTER);
                break;
            case MAIL_ACT_ADD_FRIEND:
                menu->state = MAIL_MENU_FRIEND_ADD;
                FriendList_AskAdd(menu->mail.id, menu->mail.name);
                break;
            case MAIL_ACT_BLOCK:
                menu->state = MAIL_MENU_BLOCK_ADD;
                BlockList_AskAdd(menu->mail.id, menu->mail.name);
                break;
            }
            break;
        case MENU_CANCEL:
            menu->state = MAIL_MENU_LIST;
            break;
        case MENU_CLOSE:
            MailMenu_Close();
            return MENU_CLOSE;
        }
        break;
    case MAIL_MENU_BLOCK_ADD:
        switch (BlockList_AddInput(pad)) {
        case MENU_DONE:
        case MENU_CANCEL:
        case MENU_UNDONE:
            menu->state = MAIL_MENU_ACTIONS;
            menu->step = MAIL_MENU_STEP_INPUT;
            break;
        case MENU_CLOSE:
            MailMenu_Close();
            return MENU_CLOSE;
        }
        break;
    case MAIL_MENU_FRIEND_ADD:
        switch (FriendList_AddInput(pad)) {
        case MENU_DONE:
        case MENU_CANCEL:
        case MENU_UNDONE:
            menu->state = MAIL_MENU_ACTIONS;
            menu->step = MAIL_MENU_STEP_INPUT;
            break;
        case MENU_CLOSE:
            return MENU_CLOSE;
        }
        break;
    case MAIL_MENU_REPLY:
        switch (ShortMail_Step(pad)) {
        case MENU_DONE:
        case MENU_CANCEL:
            menu->state = MAIL_MENU_ACTIONS;
            menu->step = MAIL_MENU_STEP_INPUT;
            break;
        case MENU_CLOSE:
            return MENU_CLOSE;
        }
        break;
    }
    return MENU_RUNNING;
}

/* The actions' input: slides their window in, then up and down move the
 * cursor; circle takes the action (MENU_DONE), cross goes back to the
 * mailbox (MENU_CANCEL), start closes the menu (MENU_CLOSE). */
s32 MailMenu_ActionInput(MailMenu *menu, u16 pad)
{
    switch (menu->step) {
    case MAIL_MENU_STEP_SLIDE_IN:
        switch (WinSlide_Step(&menu->win2, 640, 280)) {
        case MENU_DONE:
            menu->step++;
            break;
        }
        break;
    case MAIL_MENU_STEP_INPUT:
        if (pad & PAD_CIRCLE) {
            menu->step = MAIL_MENU_STEP_SLIDE_IN;
            LobbySe_Play(LOBBY_SE_CONFIRM);
            return MENU_DONE;
        }
        if (pad & PAD_CROSS) {
            menu->step = MAIL_MENU_STEP_SLIDE_IN;
            LobbySe_Play(LOBBY_SE_CANCEL);
            return MENU_CANCEL;
        }
        if (pad & PAD_START) {
            menu->step = MAIL_MENU_STEP_SLIDE_IN;
            return MENU_CLOSE;
        }
        menu->cursor = Menu_WrapUpDown(menu->cursor, menu->count);
        break;
    }
    return MENU_RUNNING;
}

/* The mails in the mailbox (gMailBox's entries with a sender). */
static s16 MailMenu_CountMails(void)
{
    s16 count = 0;
    s16 i;

    for (i = 0; i < MAIL_BOX_LEN; i++) {
        if (gMailBox[i].id[0] != 0) {
            count++;
        }
    }
    return count;
}

/* The mailbox's input: up and down choose a mail; circle opens it
 * (MENU_DONE) if there is one, cross leaves (MENU_CANCEL), start closes the
 * menu (MENU_CLOSE). */
s32 MailMenu_ListInput(MailMenu *menu, u16 pad)
{
    menu->mails = MailMenu_CountMails();
    if (pad & PAD_CIRCLE) {
        if (menu->mails != 0) {
            LobbySe_Play(LOBBY_SE_CONFIRM);
            return MENU_DONE;
        }
    } else if (pad & PAD_CROSS) {
        LobbySe_Play(LOBBY_SE_CANCEL);
        return MENU_CANCEL;
    } else if (pad & PAD_START) {
        return MENU_CLOSE;
    } else {
        menu->sel = Menu_WrapUpDown(menu->sel, menu->mails);
    }
    return MENU_RUNNING;
}

/* The mailbox's two formats: a mail's number, and a string. */
const char gMailboxNumFmt[3] = {
#include "lobby/00838418.inc"
};
const char gMailboxStrFmt[3] = {
#include "lobby/00838420.inc"
};
/* Draws the mailbox at (x, y): its title, then a line per mail with its
 * number, whether it is still unread (MailEntry.unread is 1 until the mail is
 * opened), the sender's name and id; or that it is empty. The chosen mail's
 * line is lit, dimmed while its actions are shown. */
void MailMenu_DrawMailbox(MailMenu *menu, s16 x, s16 y)
{
    char wide[64];
    s8 inactive;
    char buf[64];
    s16 rowY = y;
    s16 i;
    MailEntry *mail;

    Win_Draw(x, y, 340, 290, 0xBDB59F6C, 0xBB402010, 8);
    Gfx_DrawLine(x + 8, y + 12, 324, 20, 0x80B59F6C);
    Font_SetColor(0x807F7F28);
    Font_SetPos(x + 12, y + 12);
    Text_PrintfColored(gMailboxMenuTexts[0]); /* the title */
    if (gMailBox[0].id[0] == 0) {
        Font_SetColor(0x802B7F7F);
        Font_SetPos(Text_CenterX(gEmptyListTexts[EMPTY_LIST_MAIL], x + 170, 18), (s16)(y + 12) + 90);
        Text_PrintfColored(gEmptyListTexts[EMPTY_LIST_MAIL]);
        return;
    }
    y += 40;
    Font_SetPos(x + 12, y);
    Text_PrintfColored(gNetLabels[NET_LABEL_NUMBER]);
    Font_SetPos(x + 42, y);
    Text_PrintfColored(gNetLabels[NET_LABEL_MAIL_UNIT]);
    Font_SetPos(x + 70, y);
    Text_PrintfColored(gNetLabels[NET_LABEL_NAME]);
    Font_SetPos(x + 224, y);
    Text_PrintfColored(gNetLabels[NET_LABEL_ID]);
    rowY += 58;
    Font_SetColor(0x807F7F7F);
    for (i = 0; i < menu->mails; i++) {
        mail = &gMailBox[i];
        if (mail->id[0] != 0) {
            if (menu->sel == i) {
                switch (menu->state) {
                case MAIL_MENU_LIST:
                    inactive = 0;
                    break;
                default:
                    inactive = 1;
                    break;
                }
                Menu_DrawRowHighlight(x + 8, rowY - 1, 324, 20, inactive);
            }
            Font_SetPos(x + 12, rowY);
            sprintf(buf, gMailboxNumFmt, i + 1);
            Sjis_Widen(buf, wide);
            Text_PrintfColored(gMailboxStrFmt, wide);
            if (mail->unread == 1) {
                Icon_Draw(x + 42, rowY - 1, 20, 20, -1, ICON_MAIL);
            } else {
                Icon_Draw(x + 42, rowY - 1, 20, 20, -1, ICON_MAIL_READ);
            }
            Font_SetPos(x + 70, rowY);
            sprintf(wide, gMailboxStrFmt, mail->name);
            Sjis_Widen(wide, buf);
            Text_PrintfColored(gMailboxStrFmt, buf);
            Font_SetPos(x + 224, rowY);
            sprintf(wide, gMailboxStrFmt, mail->id);
            Sjis_Widen(wide, buf);
            Text_PrintfColored(gMailboxStrFmt, buf);
        }
        rowY += 22;
    }
}

/* Draws the actions' window to the left of x: a row per action, the
 * cursor's lit (dimmed while an action runs). */
void MailMenu_DrawActions(MailMenu *menu, s16 x, s16 y)
{
    s16 i;
    s8 inactive;

    x -= 130;
    Win_Draw(x, y, 130, menu->count * 22 + 14, 0xBDB59F6C, 0xBB402010, 3);
    y += 8;
    Font_SetColor(0x807F7F7F);
    for (i = 0; i < menu->count; i++) {
        if (i > 0) {
            Gfx_DrawLine(x + 8, y - 2, 114, 2, 0xBDB59F6C);
        }
        if (menu->cursor == i) {
            switch (menu->state) {
            case MAIL_MENU_ACTIONS:
                inactive = 0;
                break;
            default:
                inactive = 1;
                break;
            }
            Menu_DrawRowHighlight(x + 8, y - 1, 114, 20, inactive);
        }
        Font_SetPos(x + 8, y);
        Text_PrintfColored(gMailActTexts[menu->actions[i]]);
        y += 22;
    }
}

/* Draws the mail menu: the mailbox, or the chosen mail (as the short mail
 * form, view only) with its actions. While replying, the form draws itself. */
void MailMenu_Draw(MenuTask *task)
{
    MailMenu *menu = (MailMenu *)&gMenuWork;
    s16 row;

    Text_SetSize(task->textSize);
    Font_SetSpacing(18, 18);
    switch (menu->state) {
    case MAIL_MENU_SLIDE_IN:
    case MAIL_MENU_LIST:
        MailMenu_DrawMailbox(menu, menu->win.pos, 54);
        break;
    case MAIL_MENU_ACTIONS:
        row = menu->win2.pos;
        ShortMail_DrawForm(&menu->mail, 0, SHORT_MAIL_VIEW_ONLY, row, 54);
        MailMenu_DrawActions(menu, row, 54);
        break;
    case MAIL_MENU_REPLY:
        break;
    }
}
