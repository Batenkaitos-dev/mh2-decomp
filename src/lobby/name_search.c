/*
 * The name search, from the game menu: the player types a hunter's name on
 * the keyboard (NameSearch_InputName), the server is asked for the hunters
 * of that name (LobbyUser_FindByName), and the hunters found are listed by
 * pages of NAME_SEARCH_ROWS_PER_PAGE with their ids and their status
 * (NameSearch_ResultsInput). Choosing one opens that hunter's info
 * (HunterInfo_Open), with a short mail and the friend list on offer, and
 * joining the hunter too unless the player is in a group.
 *
 * Its work is a NameSearchMenu in gMenuWork (lobby_menu.h). NameSearch_Draw
 * draws the form, the results or the hunter's info. The id search works
 * the same way with a hunter id.
 */
#include "common.h"
#include "lobby_state.h"
#include "lobby_text.h"
#include "m2c_macros.h"
#include "lobby_globals.h"
#include "pad.h"
#include "lobby_net.h"
#include "lobby_menu.h"
#include "lobby_labels.h"
/* The whole name search was built at optimization level 4. */
#pragma optimization_level 4

void LobbyMenu_Init(void);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
void *memset(void *dst, s32 c, u32 n);
s32 WinSlide_Step(WinSlide *, s16 from, s16 to);
extern u8 gMenuWork;
void NameSearch_Draw(MenuTask *task);
void NameSearch_Draw2(MenuTask *task);

/* Opens the name search: clears its work, takes the pad from the field and
 * slides the form in. */
void NameSearch_Open(void)
{
    memset(&gMenuWork, 0, sizeof(MenuWork));
    gFieldControl = 0;
    LobbyMenu_Init();
    LobbyMenu_SetDraw(0, NameSearch_Draw, NameSearch_Draw2, 0);
    WinSlide_Step(&((NameSearchMenu *)&gMenuWork)->formWin, 0x280, 0x190);
}

/* Closes the name search and gives the pad back to the field. */
void NameSearch_Close(void)
{
    gFieldControl = 1;
    LobbyMenu_Init();
    LobbyMenu_SetDraw(0, 0, 0, 0);
}
s32 Menu_WrapUpDown(s32, s32);

/* prototypes */
void Text_SetSize(s32);
/* end of prototypes */

/* the hunters a name search found (LobbyNet_GetUserFindByName) */
extern NetUserFindByNameFound *gUserFindByNameFound;

void NameSearch_BuildPage(NameSearchMenu *menu, s16 page);

extern LobbyMenu *gLobbyMenu;
extern u8 gLobbyLevel1Sel;
void LobbyText_Open(s32 msg);
s32 NameSearch_InputName(NameSearchMenu *menu, u16 pad);
s32 NameSearch_ResultsInput(NameSearchMenu *menu, u16 pad);
void NameSearch_ShowResults(NameSearchMenu *menu);
s32 LobbyUser_FindByName(s8 *name);
s32 HunterInfo_Update(u16 pad);
void HunterInfo_Open(void *, s32);

/*
 * The name search's frame, from the game menu: runs the form until a name
 * is taken, asks the server for the hunters of that name, then runs the
 * results; a hunter chosen there opens that hunter's info (with joining on
 * offer only while the player isn't in a group), and leaving it brings the
 * results back. Leaving the results brings the form back. Returns
 * MENU_CANCEL when the form is left, MENU_CLOSE on start (the game menu
 * closes), MENU_UNDONE when the search failed and its notice is closed,
 * MENU_RUNNING meanwhile.
 */
s32 NameSearch_Update(u16 pad)
{
    NameSearchMenu *menu = (NameSearchMenu *)&gMenuWork;

    switch (menu->step) {
    case NAME_SEARCH_FORM:
        switch (NameSearch_InputName(menu, pad)) {
        case MENU_DONE:
            menu->step++;
            LobbyText_Open(LOBBY_TEXT_HUNTER_SEARCHING);
            gLobbyMenu->redraw = 1;
            break;
        case MENU_CANCEL:
            NameSearch_Close();
            return MENU_CANCEL;
        case MENU_CLOSE:
            NameSearch_Close();
            return MENU_CLOSE;
        }
        break;
    case NAME_SEARCH_FINDING:
        gLobbyMenu->redraw = 1;
        switch (LobbyUser_FindByName((s8 *)menu->name)) {
        case LOBBY_STEP_DONE:
            menu->step++;
            NameSearch_ShowResults(menu);
            break;
        case LOBBY_STEP_FAILED:
            menu->step = NAME_SEARCH_FAILED;
            break;
        }
        break;
    case NAME_SEARCH_RESULTS:
        switch (NameSearch_ResultsInput(menu, pad)) {
        case MENU_DONE:
            menu->step = NAME_SEARCH_HUNTER;
            if (gLobbyLevel1Sel != 0) {
                HunterInfo_Open(menu->rows[menu->cursor], HUNTER_INFO_MAIL | HUNTER_INFO_FRIEND);
            } else {
                HunterInfo_Open(menu->rows[menu->cursor], HUNTER_INFO_MAIL | HUNTER_INFO_FRIEND | HUNTER_INFO_JOIN);
            }
            break;
        case MENU_CANCEL:
            menu->step = NAME_SEARCH_FORM;
            menu->inputStep = NAME_INPUT_ROWS;
            break;
        case MENU_CLOSE:
            NameSearch_Close();
            return MENU_CLOSE;
        }
        break;
    case NAME_SEARCH_HUNTER:
        switch (HunterInfo_Update(pad)) {
        case MENU_DONE:
        case 1:
        case MENU_CANCEL:
            gFieldControl = 0;
            menu->step = NAME_SEARCH_RESULTS;
            menu->inputStep = NAME_RESULTS_LIST;
            break;
        case MENU_CLOSE:
            NameSearch_Close();
            return MENU_CLOSE;
        }
        break;
    case NAME_SEARCH_FAILED:
        gLobbyMenu->redraw = 1;
        if (pad & PAD_CIRCLE) {
            NameSearch_Close();
            return MENU_UNDONE;
        }
        if (pad & PAD_START) {
            NameSearch_Close();
            return MENU_CLOSE;
        }
        break;
    }
    return MENU_RUNNING;
}

void LobbySe_Play(s32 se);
void LobbyDialog_Grow(void);
s8 Kbd_Run(char *dst, s16 held, s16 pressed);
void Kbd_Cancel(void);
void Kbd_SetPos(s32 n, f32 f);
void Chat_OpenKbd(s32 flags, s32 layout, u16 maxChars, void *text);

/*
 * The name search's form: slides in, then moves over its two rows. Circle
 * on the name row opens the keyboard; on the start row it refuses an empty
 * name (a notice until circle) and otherwise returns MENU_DONE. Leaving
 * the keyboard with no name typed shows the same notice. Returns
 * MENU_CANCEL on cross, MENU_CLOSE on start, MENU_RUNNING meanwhile.
 */
s32 NameSearch_InputName(NameSearchMenu *menu, u16 pad)
{
    switch (menu->inputStep) {
    case NAME_INPUT_SLIDE_IN:
        switch (WinSlide_Step(&menu->formWin, 0x280, 0x190)) {
        case MENU_DONE:
            menu->inputStep++;
            menu->row = NAME_SEARCH_ROW_NAME;
            break;
        }
        break;
    case NAME_INPUT_ROWS:
        if (pad & PAD_CIRCLE) {
            if (menu->row == NAME_SEARCH_ROW_NAME) {
                menu->inputStep++;
                Kbd_SetPos(0x3A, 80.0f);
                Chat_OpenKbd(3, 0xF, NAME_SEARCH_NAME_CHARS, menu->name); /* kind 3, keyboard layout 0xF */
                LobbySe_Play(LOBBY_SE_CONFIRM);
            } else if (menu->name[0] == 0) {
                menu->inputStep = NAME_INPUT_NOTICE;
                LobbyText_Open(LOBBY_TEXT_NAME_EMPTY);
                LobbyDialog_Grow();
                LobbySe_Play(LOBBY_SE_REFUSE);
            } else {
                menu->inputStep = NAME_INPUT_SLIDE_IN;
                LobbySe_Play(LOBBY_SE_CONFIRM);
                return MENU_DONE;
            }
        } else if (pad & PAD_CROSS) {
            menu->inputStep = NAME_INPUT_SLIDE_IN;
            LobbySe_Play(LOBBY_SE_CANCEL);
            return MENU_CANCEL;
        } else if (pad & PAD_START) {
            return MENU_CLOSE;
        } else {
            menu->row = Menu_WrapUpDown(menu->row, NAME_SEARCH_ROWS);
        }
        break;
    case NAME_INPUT_KEYBOARD:
        if (Kbd_Run(menu->name, *(s16 *)&gPadState[0].held, *(s16 *)&gPadState[0].pressed) != 0) {
            Kbd_Cancel();
            if (menu->name[0] == 0) {
                menu->inputStep = NAME_INPUT_NOTICE;
                LobbyText_Open(LOBBY_TEXT_NAME_EMPTY);
                LobbyDialog_Grow();
            } else {
                menu->inputStep = NAME_INPUT_ROWS;
                menu->row++;
            }
        }
        break;
    case NAME_INPUT_NOTICE:
        gLobbyMenu->redraw = 1;
        if (pad & PAD_CIRCLE) {
            menu->inputStep = NAME_INPUT_ROWS;
            LobbySe_Play(LOBBY_SE_CONFIRM);
        } else if (pad & PAD_START) {
            return MENU_CLOSE;
        }
        break;
    }
    return MENU_RUNNING;
}

s32 Menu_CountPages(s16 n, s16 per);

/* Fills the results' rows with page `page` of the hunters found, counts the
 * pages, and keeps the cursor on the page. */
void NameSearch_BuildPage(NameSearchMenu *menu, s16 page)
{
    s16 i;
    s16 n = 0;
    s16 first = page * NAME_SEARCH_ROWS_PER_PAGE;
    s16 last = first + NAME_SEARCH_ROWS_PER_PAGE - 1;

    for (i = 0; i < gUserFindByNameFound->total; i++) {
        if (i >= first && i <= last) {
            menu->rows[n] = &gUserFindByNameFound->list[i];
            n++;
        }
    }
    menu->count = n;
    menu->page = page;
    menu->pageCount = Menu_CountPages(gUserFindByNameFound->total, NAME_SEARCH_ROWS_PER_PAGE);
    if (menu->cursor >= menu->count) {
        menu->cursor = menu->count - 1;
    }
}

/* Shows the results from their first page and starts sliding them in. */
void NameSearch_ShowResults(NameSearchMenu *menu)
{
    menu->cursor = 0;
    NameSearch_BuildPage(menu, 0);
    WinSlide_Step(&menu->listWin, 0x280, 0x12C);
}

extern char gMyUserId[];
s32 memcmp(void *a, void *b, s32 n);
s32 Menu_WrapLeftRight(s32, s32);

/*
 * The name search's results: slide in, then the cursor moves over the page
 * and left or right turns the pages. Circle on a hunter returns MENU_DONE,
 * unless the hunter is the player (refused). Returns MENU_CANCEL on cross,
 * MENU_CLOSE on start, MENU_RUNNING meanwhile.
 */
s32 NameSearch_ResultsInput(NameSearchMenu *menu, u16 pad)
{
    switch (menu->inputStep) {
    case NAME_RESULTS_SLIDE_IN:
        switch (WinSlide_Step(&menu->listWin, 0x280, 0x12C)) {
        case MENU_DONE:
            menu->inputStep++;
            break;
        }
        break;
    case NAME_RESULTS_LIST:
        if (pad & PAD_CIRCLE) {
            if (menu->count != 0) {
                if (memcmp(menu->rows[menu->cursor]->id, gMyUserId, LOBBY_USER_ID_LEN) == 0) {
                    LobbySe_Play(LOBBY_SE_REFUSE);
                    break;
                }
                menu->inputStep = NAME_RESULTS_SLIDE_IN;
                LobbySe_Play(LOBBY_SE_CONFIRM);
                return MENU_DONE;
            }
        } else if (pad & PAD_CROSS) {
            menu->inputStep = NAME_RESULTS_SLIDE_IN;
            LobbySe_Play(LOBBY_SE_CANCEL);
            return MENU_CANCEL;
        } else if (pad & (PAD_LEFT | PAD_RIGHT)) {
            if (menu->pageCount > 1) {
                menu->page = Menu_WrapLeftRight(menu->page, menu->pageCount);
                NameSearch_BuildPage(menu, menu->page);
            }
        } else if (pad & PAD_START) {
            return MENU_CLOSE;
        } else {
            menu->cursor = Menu_WrapUpDown(menu->cursor, menu->count);
        }
        break;
    }
    return MENU_RUNNING;
}

extern char gNameSearchTextFmt[];
void Win_Draw(s16 x, s16 y, s32 w, s32 h, u32 c1, u32 c2, s32 k); /* unit's own declaration: s16 coordinates and an s32 kind here; with the definition's s32 coordinates and s16 kind, NameSearch_DrawResults and NameSearch_DrawForm compile differently */
void Gfx_DrawLine(s16 x, s16 y, s16 w, s16 h, s32 color);
void Font_SetColor(s32 color);
void Font_SetPos(s16 x, s16 y);
void Text_PrintfColored(const char *fmt, ...);
u32 Color_PulseAlpha(u32 color);
void Menu_DrawSideArrows(s16 x, s16 y, s16 w, s16 size, s8 flip, u32 color);
void Gfx_DrawRectFrame(s16 x, s16 y, s16 w, s16 h, s32 color);
void Menu_DrawArrowsRev(s16 x, s16 y, s16 size, char *text, s32 color);

void Sjis_Widen(char *src, char *dst);
void Menu_DrawLine(s16 x, s16 y, s32 w, s32 h, s32 sel); /* unit's own declaration: a width, a height and an s32 choice here; with the definition's two corners and s8 flag, NameSearch_DrawResults compiles differently */
void Menu_DrawPageNum(s16 x, s16 y, s16 page, s16 n, u8 alt);
void Text_Printf(const char *fmt, ...);

/*
 * Draws the name search's results at x, y: the title, then either "no
 * results" or the page number and the page's hunters, each with name, id
 * (widened to full-width characters) and status (enum FriendStatus), the
 * cursor's line drawn lit while the results have the pad.
 */
void NameSearch_DrawResults(NameSearchMenu *menu, s16 x, s16 y)
{
    s16 rowY = y;
    s16 i;
    s32 textX;
    s32 dim;
    char *status;
    char wideId[32];

    Win_Draw(x, y, 0x140, 0x122, 0xBDB59F6C, 0xBB402010, 8);
    Gfx_DrawLine(x + 8, y + 0xC, 0x130, 0x14, 0x80B59F6C);
    Font_SetColor(0x807F7F28);
    textX = x + 0x10;
    Font_SetPos(textX, y + 0xE);
    Text_PrintfColored(gSearchMenuTexts[SEARCH_MENU_TEXT_NAME]);
    if (menu->count == 0) {
        Font_SetColor(0x802B7F7F);
        Font_SetPos(Text_CenterX(gEmptyListTexts[EMPTY_LIST_NO_RESULTS], x + 0xA0, 0x12), (s16)(y + 0xE) + 0x64);
        Text_PrintfColored(gEmptyListTexts[EMPTY_LIST_NO_RESULTS]);
        return;
    }
    Menu_DrawPageNum(x + 0xE6, y + 0xE, menu->page, menu->pageCount, 1);
    rowY += 0x24;
    Font_SetColor(0x807F7F28);
    Font_SetPos(textX, rowY);
    Text_PrintfColored(gNetLabels[NET_LABEL_NAME]);
    Font_SetPos(x + 0x62, rowY);
    Text_PrintfColored(gNetLabels[NET_LABEL_ID]);
    Font_SetPos(x + 0xD8, rowY);
    Text_PrintfColored(gNetLabels[NET_LABEL_CONN_STATE]);
    for (i = 0; i < menu->count; i++) {
        Font_SetColor(0x807F7F7F);
        rowY += 0x16;
        if (menu->cursor == i) {
            switch (menu->step) {
            case NAME_SEARCH_RESULTS:
                dim = 0;
                break;
            default:
                dim = 1;
                break;
            }
            Menu_DrawLine(x + 8, rowY - 1, 0x130, 0x14, dim);
        }
        Font_SetPos(x + 0x10, rowY);
        Text_PrintfColored(gNameSearchTextFmt, menu->rows[i]->name);
        Font_SetPos(x + 0x62, rowY);
        Sjis_Widen(menu->rows[i]->id, wideId);
        Text_PrintfColored(wideId);
        Font_SetPos(x + 0xD8, rowY);
        /* the status is the first byte of the reply's text, read unsigned as
         * the game's code does (FRIEND_STATUS_UNKNOWN is its 0xFF) */
        switch ((u8)menu->rows[i]->text[0]) {
        case FRIEND_STATUS_UNKNOWN:
            status = gNetLabels[NET_LABEL_CHECKING];
            break;
        case FRIEND_STATUS_ONLINE:
            status = gNetLabels[NET_LABEL_ONLINE];
            break;
        case FRIEND_STATUS_ON_QUEST:
            status = gNetLabels[NET_LABEL_ON_QUEST];
            break;
        case FRIEND_STATUS_OFFLINE:
            status = gNetLabels[NET_LABEL_OFFLINE];
            break;
        }
        Text_Printf(status);
    }
}

/*
 * Draws the name search's form at x, y: its title, the name's frame
 * (pulsing, with arrows, while it has the cursor) with the name typed, and
 * the start row with its arrows while chosen.
 */
void NameSearch_DrawForm(NameSearchMenu *menu, s16 x, s16 y)
{
    s16 labelX;
    u32 frameColor;
    u32 startColor;
    s16 rowY = y;

    Win_Draw(x, y, 0xC8, 0x82, 0xBDB59F6C, 0xBB402010, 8);
    Gfx_DrawLine(x + 8, rowY + 0xC, 0xB8, 0x14, 0x80B59F6C);
    Font_SetColor(0x807F7F28);
    Font_SetPos(x + 0x10, rowY + 0xE);
    Text_PrintfColored(gSearchMenuTexts[SEARCH_MENU_TEXT_NAME]);
    rowY += 0x36;
    if (menu->row == NAME_SEARCH_ROW_NAME) {
        frameColor = Color_PulseAlpha(0x8080);
        startColor = 0x80404040;
        Menu_DrawSideArrows(x + 0xA, rowY, 0xB4, 0x16, 0, 0xFF008080);
    } else {
        frameColor = 0x80808080;
        startColor = 0x802B7F7F;
    }
    Gfx_DrawRectFrame(x + 0x14, rowY, 0xA0, 0x18, frameColor);
    Gfx_DrawRectFrame(x + 0x13, rowY - 1, 0xA2, 0x1A, frameColor);
    Font_SetColor(0x807F7F7F);
    Font_SetPos(Text_CenterX(menu->name, x + 0x64, 0x12), rowY + 4);
    Text_PrintfColored(gNameSearchTextFmt, menu->name);
    Font_SetColor(startColor);
    labelX = Text_CenterX(gNetLabels[NET_LABEL_SEARCH_START], x + 0x64, 0x12);
    Font_SetPos(labelX, rowY += 0x28);
    Text_PrintfColored(gNetLabels[NET_LABEL_SEARCH_START]);
    if (menu->row == NAME_SEARCH_ROW_START) {
        Menu_DrawArrowsRev(labelX, rowY, 0x12, gNetLabels[NET_LABEL_SEARCH_START], 0xFF008080);
    }
}

void Font_SetSpacing(u8 a, u8 b);
void HunterInfo_DrawIfOpen(void);

/*
 * The name search's first draw callback: the form, the results, or the
 * hunter's info once it is open.
 */
void NameSearch_Draw(MenuTask *task)
{
    NameSearchMenu *menu = (NameSearchMenu *)&gMenuWork;

    Text_SetSize(task->textSize);
    Font_SetSpacing(0x12, 0x12);
    switch (menu->step) {
    case NAME_SEARCH_FORM:
        NameSearch_DrawForm(menu, menu->formWin.pos, 0x3A);
        break;
    case NAME_SEARCH_RESULTS:
        NameSearch_DrawResults(menu, menu->listWin.pos, 0x3A);
        break;
    case NAME_SEARCH_HUNTER:
        HunterInfo_DrawIfOpen();
        break;
    }
}

/* The name search's second draw callback: only sets the text size. */
void NameSearch_Draw2(MenuTask *task)
{
    Text_SetSize(task->textSize);
}

/* The name search's read-only data: its string format. */
#pragma rodata on
char gNameSearchTextFmt[0x8] = {
#include "lobby/00838198.inc"
};
#pragma rodata off
