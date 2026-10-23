/*
 * The short mail form (ShortMailMenu, gShortMailMenu): a mail of up to 120
 * bytes to one hunter, opened from the mail menu, from another hunter's
 * details or from the game menu's mail page (ShortMail_Open's
 * SHORT_MAIL_* flags). The form's rows are the addressee (typed as an id,
 * or picked from the friend list), the text and send; ShortMail_Step runs
 * it and sends the mail (LobbyStep_ShortMailSend). The form draws itself
 * through its own two draw tasks (ShortMail_Draw, ShortMail_Draw2); the
 * mail menu draws received mails with ShortMail_DrawForm too.
 */
#include "common.h"
#include "sprite.h"
#include "lobby_state.h"
#include "lobby_text.h"
#include "m2c_macros.h"
#include "lobby_vobj.h"
#include "lobby_globals.h"
#include "pad.h"
#include "lobby_menu.h"
#include "lobby_labels.h"
#include "bgm.h"
#include "chat_box.h"
#include "sprite_queue.h"
#include "camera.h"
#include "window.h"
#include "short_mail.h"
#include "chat_phrases.h"
#include "draw_helpers.h"
#include "draw_slots.h"
#include "font.h"
#include "friend_list.h"
#include "lobby_dialog_box.h"
#include "lobby_util.h"
#include "menu_sheets.h"
#include "menu_steps.h"

void Text_SetSize(s32);
extern s32 gTownMode;
extern s32 gTownMenuOpen;

/* The game built ShortMail_Open at optimization level 4. */
#pragma optimization_level 4
void *memset(void *p, s32 c, u32 n);
void *memcpy(void *d, const void *s, u32 n);
extern ShortMailMenu gShortMailMenu;
extern MailEntry gShortMailMsg;
/* Opens the short mail form: to the user id and name given (either may be
 * NULL), with the SHORT_MAIL_* flags; with SHORT_MAIL_SAVED and no id, the
 * form starts from the last mail sent (gShortMailMsg). */
void ShortMail_Open(void *id, void *name, u32 flags)
{
    ShortMailMenu *menu = &gShortMailMenu;

    memset(menu, 0, 1204);
    gShortMailMenu.layer[0].textSize = 0;
    gShortMailMenu.layer[0].draw = ShortMail_Draw;
    gShortMailMenu.layer[1].draw = ShortMail_Draw2;
    gShortMailMenu.layer[1].textSize = 1;
    gShortMailMenu.flags = flags;
    WinSlide_Step(&menu->win, 640, 280);
    gFieldControl = 0;
    if (name != NULL) {
        memcpy(menu->mail.name, name, LOBBY_USER_NAME_LEN);
    }
    if (id != NULL) {
        memcpy(menu->mail.id, id, LOBBY_USER_ID_LEN);
    } else if (flags & SHORT_MAIL_SAVED) {
        memcpy(&menu->mail, &gShortMailMsg, sizeof(MailEntry));
    }
    if (flags & (SHORT_MAIL_TYPE_ID | SHORT_MAIL_SAVED)) {
        menu->count = 3;
    } else if (flags & SHORT_MAIL_TO_HUNTER) {
        menu->count = 2;
    }
}
#pragma optimization_level reset

/* Gives the field its input back as the form closes. */
void ShortMail_Close(void)
{
    gFieldControl = 1;
}

s32 LobbyStep_ShortMailSend(MailEntry *msg);
extern LobbyMenu *gLobbyMenu;
/* Runs a frame of the short mail form and queues its draw tasks: the window
 * slides in, the form takes input, then the mail is sent and the result
 * shown until circle. Returns MENU_RUNNING, or MENU_CANCEL and MENU_CLOSE
 * once the form has closed. */
s32 ShortMail_Step(u16 pad)
{
    ShortMailMenu *menu = &gShortMailMenu;

    switch (gShortMailMenu.step) {
    case SHORT_MAIL_STEP_SLIDE_IN:
        switch (WinSlide_Step(&menu->win, 640, 280)) {
        case MENU_DONE:
            menu->step++;
            break;
        }
        break;
    case SHORT_MAIL_STEP_INPUT:
        switch (ShortMail_Input(menu, pad)) {
        case MENU_DONE:
            menu->step++;
            LobbyText_Open(LOBBY_TEXT_MAIL_SENDING);
            gLobbyMenu->redraw = 1;
            break;
        case MENU_CANCEL:
            ShortMail_Close();
            return MENU_CANCEL;
        case MENU_CLOSE:
            ShortMail_Close();
            return MENU_CLOSE;
        }
        break;
    case SHORT_MAIL_STEP_SEND:
        gLobbyMenu->redraw = 1;
        switch (LobbyStep_ShortMailSend(&menu->mail)) {
        case LOBBY_STEP_DONE:
            menu->step = SHORT_MAIL_STEP_RESULT;
            LobbyText_Open(LOBBY_TEXT_MAIL_SENT);
            gLobbyMenu->redraw = 1;
            break;
        case LOBBY_STEP_FAILED:
            menu->step = SHORT_MAIL_STEP_RESULT;
            gLobbyMenu->redraw = 1;
            break;
        }
        break;
    case SHORT_MAIL_STEP_RESULT:
        gLobbyMenu->redraw = 1;
        if (pad & PAD_CIRCLE) {
            LobbySe_Play(LOBBY_SE_CONFIRM);
            ShortMail_Close();
            return MENU_CANCEL;
        }
        break;
    }
    OT_AddAt(gDrawListText0b, &menu->layer[0], 0, 1);
    OT_AddAt(gDrawListText1, &menu->layer[1], 0, 1);
    return MENU_RUNNING;
}

void Kbd_SetPos(s32 n, f32 f);
s8 Kbd_Run(char *dst, s16 held, s16 pressed);
s32 memcmp(void *a, void *b, s32 n);
void Kbd_Cancel(void);
s32 LobbyUser_FetchDetails(void *out, void *id);
extern char gMyUserId[];
u32 strlen(const char *s);

/*
 * The form's input (ShortMailMenu.state): the rows, the keyboard for the
 * text, the addressee's choice (the friend list or an id typed, which is
 * then looked up). An id shorter than ID_SEARCH_ID_CHARS, or the player's
 * own, refuses send. Returns MENU_DONE to send, MENU_CANCEL (cross),
 * MENU_CLOSE (start) or MENU_RUNNING. row copies the s16 sel, and the
 * wrap call reads menu->sel again: the game's code loads it twice.
 */
s32 ShortMail_Input(ShortMailMenu *menu, u16 pad)
{
    s16 row = menu->sel;
    s8 done;

    switch (menu->state) {
    case SHORT_MAIL_FORM:
        if (pad & PAD_CIRCLE) {
            if (!(menu->flags & SHORT_MAIL_TYPE_ID)) {
                row++;
            }
            switch (row) {
            case SHORT_MAIL_ROW_TO:
                menu->state = SHORT_MAIL_TO_MENU;
                menu->sel2 = SHORT_MAIL_PICK_FRIEND;
                LobbySe_Play(LOBBY_SE_CONFIRM);
                break;
            case SHORT_MAIL_ROW_TEXT:
                menu->state = SHORT_MAIL_TEXT;
                Kbd_SetPos(50, 80.0f);
                Chat_OpenKbd(1, 14, sizeof(menu->mail.text), menu->mail.text);
                LobbySe_Play(LOBBY_SE_CONFIRM);
                break;
            case SHORT_MAIL_ROW_SEND:
                if (strlen(menu->mail.id) < ID_SEARCH_ID_CHARS) {
                    LobbySe_Play(LOBBY_SE_REFUSE);
                } else if (memcmp(menu->mail.id, gMyUserId, NET_USER_ID_MATCH) == 0) {
                    LobbySe_Play(LOBBY_SE_REFUSE);
                } else {
                    menu->state = SHORT_MAIL_FORM;
                    LobbySe_Play(LOBBY_SE_CONFIRM);
                    return MENU_DONE;
                }
                break;
            }
        } else if (pad & PAD_CROSS) {
            LobbySe_Play(LOBBY_SE_CANCEL);
            return MENU_CANCEL;
        } else if (pad & PAD_START) {
            return MENU_CLOSE;
        } else {
            menu->sel = Menu_WrapUpDown(menu->sel, menu->count);
        }
        break;
    case SHORT_MAIL_TEXT:
        if (gPadState[0].pressed & PAD_SELECT) {
            done = -1;
        } else {
            done = Kbd_Run(menu->mail.text, *(s16 *)&gPadState[0].held, gPadState[0].pressed);
        }
        switch (done) {
        case 1:
        case -1:
            menu->state = SHORT_MAIL_FORM;
            Chat_FilterWords(menu->mail.text);
            Kbd_Cancel();
            break;
        default:
            if (pad == PAD_SELECT) {
                menu->state = SHORT_MAIL_FORM;
                Kbd_Cancel();
                LobbySe_Play(LOBBY_SE_CANCEL);
            }
            break;
        }
        break;
    case SHORT_MAIL_TO_MENU:
        if (pad & PAD_CIRCLE) {
            if (menu->sel2 == SHORT_MAIL_PICK_FRIEND) {
                menu->state = SHORT_MAIL_FRIENDS;
                FriendList_Open(FRIEND_LIST_OPEN_PICK);
            } else {
                menu->state = SHORT_MAIL_ID_ENTRY;
                Kbd_SetPos(50, 80.0f);
                Chat_OpenKbd(0, ID_SEARCH_ID_CHARS, ID_SEARCH_ID_CHARS, menu->mail.id);
            }
            LobbySe_Play(LOBBY_SE_CONFIRM);
        } else if (pad & PAD_CROSS) {
            menu->state = SHORT_MAIL_FORM;
            LobbySe_Play(LOBBY_SE_CANCEL);
        } else if (pad & PAD_START) {
            return MENU_CLOSE;
        } else {
            menu->sel2 = Menu_WrapUpDown(menu->sel2, 2);
        }
        break;
    case SHORT_MAIL_ID_ENTRY:
        if (gPadState[0].pressed & PAD_SELECT) {
            done = -1;
        } else {
            done = Kbd_Run(menu->mail.id, *(s16 *)&gPadState[0].held, gPadState[0].pressed);
        }
        switch (done) {
        case 1:
        case -1:
            if (strlen(menu->mail.id) < ID_SEARCH_ID_CHARS) {
                menu->state = SHORT_MAIL_NOTICE;
                memset(menu->mail.name, 0, LOBBY_USER_NAME_LEN);
                LobbyText_Open(LOBBY_TEXT_ID_TOO_SHORT);
                gLobbyMenu->redraw = 1;
            } else if (memcmp(menu->mail.id, gMyUserId, NET_USER_ID_MATCH) == 0) {
                menu->state = SHORT_MAIL_NOTICE;
                memset(menu->mail.name, 0, LOBBY_USER_NAME_LEN);
                LobbyText_Open(LOBBY_TEXT_MAIL_TO_SELF);
                gLobbyMenu->redraw = 1;
            } else {
                menu->state++;
                LobbyText_Open(LOBBY_TEXT_HUNTER_SEARCHING);
                gLobbyMenu->redraw = 1;
            }
            Kbd_Cancel();
            break;
        default:
            if (pad == PAD_SELECT) {
                menu->state = SHORT_MAIL_FORM;
                Kbd_Cancel();
                LobbySe_Play(LOBBY_SE_CANCEL);
            }
            break;
        }
        break;
    case SHORT_MAIL_FETCH:
        gLobbyMenu->redraw = 1;
        switch (LobbyUser_FetchDetails(menu->details, menu->mail.id)) {
        case LOBBY_STEP_DONE:
            menu->state = SHORT_MAIL_FORM;
            memcpy(menu->mail.name, menu->details, LOBBY_USER_NAME_LEN);
            break;
        case LOBBY_STEP_FAILED:
            menu->state = SHORT_MAIL_NOTICE;
            memset(menu->mail.name, 0, LOBBY_USER_NAME_LEN);
            gLobbyMenu->redraw = 1;
            break;
        }
        break;
    case SHORT_MAIL_FRIENDS:
        switch (FriendList_Update(pad)) {
        case MENU_DONE:
            menu->state = SHORT_MAIL_FORM;
            memcpy(menu->mail.id, FriendList_GetSelectedId(), LOBBY_USER_ID_LEN);
            memcpy(menu->mail.name, FriendList_GetSelectedName(), LOBBY_USER_NAME_LEN);
            break;
        case MENU_CANCEL:
            menu->state = SHORT_MAIL_TO_MENU;
            break;
        case MENU_CLOSE:
            return MENU_CLOSE;
        }
        break;
    case SHORT_MAIL_NOTICE:
        gLobbyMenu->redraw = 1;
        if (pad & PAD_CIRCLE) {
            menu->state = SHORT_MAIL_FORM;
            LobbySe_Play(LOBBY_SE_CONFIRM);
        }
        break;
    }
    return MENU_RUNNING;
}

void Text_PrintfColored(char *fmt, ...);
void Icon_Draw(s16 x, s16 y, s16 w, s16 h, u32 color, s32 i);
u32 Color_PulseAlpha(u32 color);
void Menu_DrawSideArrows(s16 x, s16 y, s16 w, s16 size, s8 flip, u32 color);

/*
 * Draws a short mail's form at x, y: the title, the addressee's name and id,
 * the text and send, sel's row lit (counted as ShortMailMenu.sel). With
 * SHORT_MAIL_TYPE_ID the id's row has a frame; with SHORT_MAIL_VIEW_ONLY the
 * text has no frame and there is no send. idRowY is an s32 holding the row
 * cut to 16 bits, and textY a second local for the text's row: as plain s16
 * copies the compiler would give them other registers than the game's code
 * has. The title goes through a void * for the same reason.
 */
void ShortMail_DrawForm(MailEntry *entry, s16 sel, s32 flags, s16 x, s16 y)
{
    u32 color;
    void *title;
    s16 titleY;
    s32 typeId;
    s32 idRowY;
    s32 viewOnly;
    s32 top;
    s32 left;
    MailEntry *mail;
    char buf[32];
    s16 sendX;
    char *sendText;
    s16 textY;

    titleY = y;
    mail = entry;
    typeId = flags & SHORT_MAIL_TYPE_ID;
    if (!typeId) {
        sel += 1;
    }
    Icon_Draw(x, y, 340, 290, 0xFFE0D8C8, 0);
    Font_SetColor(0x80301010);
    title = gGameMenuLabels[MENU_LABEL_SHORT_MAIL];
    left = x;
    Font_SetPos(Text_CenterX(title, x + 170, 18), titleY += 12);
    Text_PrintfColored(title);
    top = titleY;
    Font_SetPos(left + 40, top + 30);
    Text_PrintfColored("%s%s", gNetLabels[NET_LABEL_NAME], gLobbyWords[LOBBY_WORD_COLON]);
    Font_SetColor(0x80101010);
    Font_SetPos(left + 40, top + 52);
    Sjis_Widen((char *)mail->name, buf);
    Text_PrintfColored("%s", buf);
    Font_SetColor(0x80301010);
    Font_SetPos(left + 40, top + 74);
    Text_PrintfColored("%s%s", gNetLabels[NET_LABEL_ID], gLobbyWords[LOBBY_WORD_COLON]);
    Font_SetColor(0x80101010);
    Font_SetPos(left + 40, top + 96);
    Sjis_Widen((char *)mail->id, buf);
    Text_PrintfColored("%s", buf);
    viewOnly = flags & SHORT_MAIL_VIEW_ONLY;
    if (!viewOnly && typeId) {
        if (sel == SHORT_MAIL_ROW_TO) {
            color = Color_PulseAlpha(0xFF008080);
            Menu_DrawSideArrows(left + 40, top + 96, 120, 22, 0, 0xFF008080);
        } else {
            color = 0xFF808080;
        }
        idRowY = (s16)(top + 96);
        Gfx_DrawRectFrame(left + 40, idRowY - 1, 120, 22, color);
        Gfx_DrawRectFrame(left + 40, idRowY - 2, 118, 24, color);
    }
    Font_SetColor(0x80301010);
    Font_SetPos(left + 40, top + 118);
    Text_PrintfColored("%s%s", gNetLabels[NET_LABEL_MAIL_BODY], gLobbyWords[LOBBY_WORD_COLON]);
    Font_SetColor(0x80101010);
    textY = top + 140;
    Text_DrawWrapped((s16)(left + 40) - 10, textY + 2, 22, mail->text, 31);
    if (!viewOnly) {
        if (sel == SHORT_MAIL_ROW_TEXT) {
            color = Color_PulseAlpha(0xFF008080);
            Menu_DrawSideArrows(left + 20, textY + 22, 300, 22, 0, 0xFF008080);
        } else {
            color = 0xFF808080;
        }
        Gfx_DrawRectFrame(left + 20, top + 140, 300, 90, color);
        Gfx_DrawRectFrame(left + 19, textY - 1, 302, 92, color);
    }
    if (!viewOnly) {
        sendText = gNetLabels[NET_LABEL_SEND];
        sendX = Text_CenterX(sendText, x + 170, 18);
        if (sel == SHORT_MAIL_ROW_SEND) {
            color = 0x80301010;
            Menu_DrawArrowsRev(sendX, top + 238, 18, gNetLabels[NET_LABEL_SEND], 0xFF008080);
        } else {
            color = 0x80404040;
        }
        Font_SetColor(color);
        Font_SetPos(sendX, top + 238);
        Text_PrintfColored("%s", sendText);
        if (sel == SHORT_MAIL_ROW_SEND) {
            Menu_DrawArrowsRev(sendX, top + 238, 18, gNetLabels[NET_LABEL_SEND], 0xFF008080);
        }
    }
}

void Win_Draw(s16 x, s16 y, s32 w, s32 h, u32 c1, u32 c2, s32 k); /* unit's own declaration: declared as this file's callers use it; with the definition's s32 sizes and colours and s16 kind, the call compiles differently */
void Gfx_DrawLine(s16 x, s16 y, s16 w, s16 h, s32 color);
void Menu_DrawRowHighlight(s16 x, s16 y, s32 w, s32 h, s32 sel); /* unit's own declaration: declared as this file's callers use it; with the definition's s16 corners and s8 flag, the call compiles differently */
/* Draws SHORT_MAIL_TO_MENU's window left of x, y: its two ways to give the
 * addressee (the friend list, an id typed), sel2's lit, brighter while the
 * menu has the input. */
void ShortMail_DrawActions(ShortMailMenu *menu, s16 x, s16 y)
{
    s16 i;
    s32 dim;

    Win_Draw(x - 150, y, 150, 58, 0xBDB59F6C, 0xBB402010, 3);
    y += 8;
    Font_SetColor(0x807F7F7F);
    for (i = 0; i < 2; i++) {
        if (i > 0) {
            Gfx_DrawLine((s16)(x - 150) + 8, y - 2, 134, 2, 0xBDB59F6C);
        }
        if (menu->sel2 == i) {
            switch (menu->state) {
            case SHORT_MAIL_TO_MENU:
                dim = 0;
                break;
            default:
                dim = 1;
                break;
            }
            Menu_DrawRowHighlight((s16)(x - 150) + 8, y - 1, 134, 20, dim);
        }
        Font_SetPos((s16)(x - 150) + 8, y);
        Text_PrintfColored(gNetLabels[NET_LABEL_FRIEND_LIST + i]);
        y += 22;
    }
}

/* The form's first draw task: sets the text's size and spacing. */
void ShortMail_Draw(MenuTask *task)
{
    Text_SetSize(task->textSize);
    Font_SetSpacing(18, 18);
}

s32 Kbd_IsOpen(void); /* unit's own declaration: the definition returns s8; declared s32
 * here, since with s8 ShortMail_Draw2 compiles differently */
extern u8 gLobbyLevel0Sel;
/* The form's second draw task: the form (not while the friend list is
 * open), the addressee's menu or the friend list, and the keyboard's box
 * while it is open over the lobby's first level. */
void ShortMail_Draw2(MenuTask *task)
{
    ShortMailMenu *menu = &gShortMailMenu;
    s16 row;

    Text_SetSize(task->textSize);
    Font_SetSpacing(18, 18);
    switch (gShortMailMenu.step) {
    case SHORT_MAIL_STEP_SLIDE_IN:
    case SHORT_MAIL_STEP_INPUT:
    case SHORT_MAIL_STEP_SEND:
    case SHORT_MAIL_STEP_RESULT:
        row = menu->win.pos;
        if (menu->step != SHORT_MAIL_STEP_INPUT || menu->state != SHORT_MAIL_FRIENDS) {
            ShortMail_DrawForm(&menu->mail, menu->sel, menu->flags, row, 54);
        }
        if (menu->state == SHORT_MAIL_TO_MENU) {
            ShortMail_DrawActions(menu, row, 54);
        } else if (menu->state == SHORT_MAIL_FRIENDS) {
            FriendList_Draw(task);
        }
        break;
    }
    if (gLobbyLevel0Sel == 0 && Kbd_IsOpen() != 0) {
        Text_SetSize(task->textSize);
        Chat_DrawKbdBox(0xBD6C6CC5);
    }
}
