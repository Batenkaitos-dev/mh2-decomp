/*
 * The id search, from the game menu: the player types a hunter id on the
 * keyboard (IdSearch_InputId) and the hunter's info opens on it
 * (HunterInfo_Open), with a short mail and the friend list on offer, and
 * joining the hunter too unless the player is in a group.
 *
 * Its work is an IdSearchMenu in gMenuWork (lobby_menu.h). IdSearch_Draw
 * draws the form or the hunter's info.
 */
#include "common.h"
#include "lobby_text.h"
#include "lobby_globals.h"
#include "pad.h"
#include "lobby_state.h"
#include "lobby_menu.h"
#include "lobby_labels.h"
#include "bgm.h"
#include "sprite_queue.h"
#include "kbd.h"
#include "text.h"
#include "window.h"
#include "id_search.h"
#include "font.h"
#include "hunter_info.h"
#include "lobby_dialog_box.h"
#include "lobby_util.h"
#include "menu_sheets.h"
#include "menu_steps.h"
#include "menu_widgets.h"

void *memset(void *dst, s32 c, u32 n);

extern u8 gMenuWork;

/* Opens the id search: clears its work, takes the pad from the field and
 * slides the form in. */
void IdSearch_Open(void)
{
    memset(&gMenuWork, 0, sizeof(MenuWork));
    gFieldControl = 0;
    LobbyMenu_Init();
    LobbyMenu_SetDraw(0, IdSearch_Draw, IdSearch_Draw2, 0);
    WinSlide_Step(&((IdSearchMenu *)&gMenuWork)->win, 640, 400);
}

/* The rest of the id search was built at optimization level 4. */
#pragma optimization_level 4

/* Closes the id search and gives the pad back to the field. */
void IdSearch_Close(void)
{
    gFieldControl = 1;
    LobbyMenu_Init();
    LobbyMenu_SetDraw(0, 0, 0, 0);
}

extern LobbyMenu *gLobbyMenu;
extern u8 gLobbyLevel1Sel;
/*
 * The id search's frame, from the game menu: runs the form until an id is
 * taken, then opens that hunter's info (with joining on offer only while
 * the player isn't in a group) and runs it until it is left, which brings
 * the form back. Returns MENU_CANCEL when the form is left, MENU_CLOSE on
 * start (the game menu closes), MENU_UNDONE from the notice step,
 * MENU_RUNNING meanwhile.
 */
s32 IdSearch_Update(u16 pad)
{
    IdSearchMenu *menu = (IdSearchMenu *)&gMenuWork;

    switch (menu->step) {
    case ID_SEARCH_FORM:
        switch (IdSearch_InputId(menu, pad)) {
        case MENU_DONE:
            menu->step++;
            LobbyText_Open(LOBBY_TEXT_HUNTER_SEARCHING);
            if (gLobbyLevel1Sel != 0) {
                HunterInfo_Open(menu->id, HUNTER_INFO_MAIL | HUNTER_INFO_FRIEND);
            } else {
                HunterInfo_Open(menu->id, HUNTER_INFO_MAIL | HUNTER_INFO_FRIEND | HUNTER_INFO_JOIN);
            }
            break;
        case MENU_CANCEL:
            IdSearch_Close();
            return MENU_CANCEL;
        case MENU_CLOSE:
            IdSearch_Close();
            return MENU_CLOSE;
        }
        break;
    case ID_SEARCH_HUNTER:
        switch (HunterInfo_Update(pad)) {
        case MENU_DONE:
        case 1:
        case MENU_CANCEL:
            gFieldControl = 0;
            menu->step = ID_SEARCH_FORM;
            menu->inputStep = ID_INPUT_ROWS;
            break;
        case MENU_CLOSE:
            IdSearch_Close();
            return MENU_CLOSE;
        }
        break;
    case ID_SEARCH_NOTICE:
        gLobbyMenu->redraw = 1;
        if (pad & PAD_CIRCLE) {
            IdSearch_Close();
            LobbySe_Play(LOBBY_SE_CONFIRM);
            return MENU_UNDONE;
        }
        if (pad & PAD_START) {
            IdSearch_Close();
            return MENU_CLOSE;
        }
        break;
    }
    return MENU_RUNNING;
}

extern char gMyUserId[];
u32 strlen(const void *s);
s32 memcmp(const char *a, const char *b, s32 n);
/* unit's own declaration: an s32 result here, read as an s8 at the call; with the definition's s8, IdSearch_InputId compiles differently */
s32 Kbd_Run(char *dst, s16 held, s16 pressed);

/*
 * The id search's form: slides in, then moves over its two rows. Circle on
 * the id row opens the keyboard; on the start row it refuses an id shorter
 * than ID_SEARCH_ID_CHARS or the player's own (a notice until circle) and
 * otherwise returns MENU_DONE. Returns MENU_CANCEL on cross, MENU_CLOSE on
 * start, MENU_RUNNING meanwhile. arg is the search's IdSearchMenu.
 */
s32 IdSearch_InputId(void *arg, u16 pad)
{
    IdSearchMenu *menu = arg;

    switch (menu->inputStep) {
    case ID_INPUT_SLIDE_IN:
        switch (WinSlide_Step(&menu->win, 640, 400)) {
        case MENU_DONE:
            menu->inputStep++;
            menu->row = ID_SEARCH_ROW_ID;
            break;
        }
        break;
    case ID_INPUT_ROWS:
        if (pad & PAD_CIRCLE) {
            if (menu->row == ID_SEARCH_ROW_ID) {
                menu->inputStep++;
                Kbd_SetPos(58, 80.0f);
                Chat_OpenKbd(0, 6, ID_SEARCH_ID_CHARS, menu->id); /* keyboard layout 6 */
                LobbySe_Play(LOBBY_SE_CONFIRM);
            } else if (strlen(menu->id) < ID_SEARCH_ID_CHARS) {
                menu->inputStep = ID_INPUT_NOTICE;
                LobbyText_Open(LOBBY_TEXT_ID_TOO_SHORT);
                LobbyDialog_Grow();
                LobbySe_Play(LOBBY_SE_REFUSE);
            } else if (memcmp(menu->id, gMyUserId, ID_SEARCH_ID_CHARS) == 0) {
                menu->inputStep = ID_INPUT_NOTICE;
                LobbyText_Open(LOBBY_TEXT_ID_IS_SELF);
                LobbyDialog_Grow();
                LobbySe_Play(LOBBY_SE_REFUSE);
            } else {
                menu->inputStep = ID_INPUT_SLIDE_IN;
                LobbySe_Play(LOBBY_SE_CONFIRM);
                return MENU_DONE;
            }
        } else if (pad & PAD_CROSS) {
            menu->inputStep = ID_INPUT_SLIDE_IN;
            LobbySe_Play(LOBBY_SE_CANCEL);
            return MENU_CANCEL;
        } else if (pad & PAD_START) {
            return MENU_CLOSE;
        } else {
            menu->row = Menu_WrapUpDown(menu->row, ID_SEARCH_ROWS);
        }
        break;
    case ID_INPUT_KEYBOARD:
        if ((s8)Kbd_Run(menu->id, *(s16 *)&gPadState[0].held, *(s16 *)&gPadState[0].pressed)) {
            menu->inputStep = ID_INPUT_ROWS;
            Kbd_Cancel();
            if (strlen(menu->id) < ID_SEARCH_ID_CHARS) {
                menu->inputStep = ID_INPUT_NOTICE;
                LobbyText_Open(LOBBY_TEXT_ID_TOO_SHORT);
                LobbyDialog_Grow();
            } else if (memcmp(menu->id, gMyUserId, ID_SEARCH_ID_CHARS) == 0) {
                menu->inputStep = ID_INPUT_NOTICE;
                LobbyText_Open(LOBBY_TEXT_ID_IS_SELF);
                LobbyDialog_Grow();
            } else {
                (menu->row)++;
            }
        }
        break;
    case ID_INPUT_NOTICE:
        gLobbyMenu->redraw = 1;
        if (pad & PAD_CIRCLE) {
            menu->inputStep = ID_INPUT_ROWS;
            LobbySe_Play(LOBBY_SE_CONFIRM);
        } else if (pad & PAD_START) {
            return MENU_CLOSE;
        }
        break;
    }
    return MENU_RUNNING;
}

extern char gIdSearchTextFmt[];
void Win_Draw(s16 x, s16 y, s32 w, s32 h, u32 c1, u32 c2, s32 k); /* unit's own declaration: s16 coordinates and an s32 kind here; with the definition's s32 coordinates and s16 kind, IdSearch_DrawForm compiles differently */
void Gfx_DrawLine(s16 x, s16 y, s16 w, s16 h, s32 color);

/*
 * Draws the id search's form at x, y: its title, the id's frame (pulsing,
 * with arrows, while it has the cursor) with the id typed, and the start
 * row with its arrows while chosen.
 */
void IdSearch_DrawForm(IdSearchMenu *menu, s16 x, s16 y)
{
    s16 labelX;
    u32 frameColor;
    u32 startColor;
    s16 rowY = y;

    Win_Draw(x, y, 200, 130, 0xBDB59F6C, 0xBB402010, 8);
    Gfx_DrawLine(x + 8, rowY + 12, 184, 20, 0x80B59F6C);
    Font_SetColor(0x807F7F28);
    Font_SetPos(x + 16, rowY + 14);
    Text_PrintfColored(gSearchMenuTexts[SEARCH_MENU_TEXT_ID]); /* the id search's title (entry 0 is the name search's) */
    rowY += 54;
    if (menu->row == ID_SEARCH_ROW_ID) {
        frameColor = Color_PulseAlpha(32896);
        startColor = 0x80404040;
        Menu_DrawSideArrows(x + 10, rowY, 180, 22, 0, 0xFF008080);
    } else {
        frameColor = 0x80808080;
        startColor = 0x802B7F7F;
    }
    Gfx_DrawRectFrame(x + 20, rowY, 160, 24, frameColor);
    Gfx_DrawRectFrame(x + 19, rowY - 1, 162, 26, frameColor);
    Font_SetColor(0x807F7F7F);
    Font_SetPos(Text_CenterX(menu->id, x + 100, 18), rowY + 4);
    Text_PrintfColored(gIdSearchTextFmt, menu->id);
    Font_SetColor(startColor);
    labelX = Text_CenterX(gNetLabels[NET_LABEL_SEARCH_START], x + 100, 18);
    Font_SetPos(labelX, rowY += 40);
    Text_PrintfColored(gNetLabels[NET_LABEL_SEARCH_START]);
    if (menu->row == ID_SEARCH_ROW_START) {
        Menu_DrawArrowsRev(labelX, rowY, 18, gNetLabels[NET_LABEL_SEARCH_START], 0xFF008080);
    }
}

/*
 * The id search's first draw callback: the form, or the hunter's info once
 * it is open.
 */
void IdSearch_Draw(MenuTask *task)
{
    IdSearchMenu *menu = (IdSearchMenu *)&gMenuWork;

    Text_SetSize(task->textSize);
    Font_SetSpacing(18, 18);
    switch (menu->step) {
    case ID_SEARCH_FORM:
        IdSearch_DrawForm(menu, menu->win.pos, 58);
        break;
    case ID_SEARCH_HUNTER:
        HunterInfo_DrawIfOpen();
        break;
    }
}

/* The id search's second draw callback: only sets the text size. */
void IdSearch_Draw2(MenuTask *task)
{
    Text_SetSize(task->textSize);
}

/* The id search's read-only data: its string format. */
#pragma rodata on
char gIdSearchTextFmt[8] = {
#include "lobby/00838190.inc"
};
#pragma rodata off
