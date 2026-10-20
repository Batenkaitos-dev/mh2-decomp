/*
 * The area search, from the game menu: the server is asked for every
 * hunter in the player's area (LobbyUser_Search with no condition), and the
 * hunters found are listed by pages of AREA_SEARCH_ROWS_PER_PAGE with their
 * ids, weapon classes and hunter ranks (AreaSearch_ResultsInput). Choosing
 * one opens that hunter's info (HunterInfo_Open), with a short mail and the
 * friend list on offer, and joining the hunter too unless the player is in
 * a group. The answer is kept for gAreaSearchFrames frames (the server's
 * setting, gAreaSearchTimer counts them down): an area search opened again
 * meanwhile shows the same hunters without asking.
 *
 * Its work is an AreaSearchMenu in gMenuWork (lobby_search.h). AreaSearch_Draw draws the
 * results or the hunter's info. The name and id searches work the same way
 * from a name or an id typed.
 */
#include "common.h"
#include "lobby_text.h"
#include "m2c_macros.h"
#include "lobby_state.h"
#include "pad.h"
#include "lobby_net.h"
#include "lobby_menu.h"
#include "lobby_labels.h"
#include "lobby_search.h"

void *memset(void *dst, s32 c, u32 n);
void AreaSearch_Draw(MenuTask *task);
void AreaSearch_Draw2(MenuTask *task);
void LobbyMenu_Init(void);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
extern u8 gMenuWork;

/* Opens the area search: clears its work and sets its draw callbacks. */
void AreaSearch_Open(void)
{
    memset(&gMenuWork, 0, MENU_WORK_SIZE);
    LobbyMenu_Init();
    LobbyMenu_SetDraw(0, AreaSearch_Draw, AreaSearch_Draw2, 0);
}

/* Closes the area search. */
void AreaSearch_Close(void)
{
    LobbyMenu_Init();
    LobbyMenu_SetDraw(0, 0, 0, 0);
}

/* The rest of the area search was built at optimization level 4. */
#pragma optimization_level 4

/* the hunters an area search found (LobbyNet_GetUserSearch) */
extern NetUserSearchFound *gUserSearchFound;

void AreaSearch_BuildPage(AreaSearchMenu *menu, s16 page);
s32 WinSlide_Step(WinSlide *, s16 from, s16 to);
void Text_SetSize(s32);

extern LobbyMenu *gLobbyMenu;
extern u8 gLobbyLevel1Sel;
extern u16 gAreaSearchTimer;
extern u16 gAreaSearchFrames;
void LobbyText_Open(s32);
void AreaSearch_ShowResults(AreaSearchMenu *menu);
s32 AreaSearch_ResultsInput(AreaSearchMenu *menu, u16 pad);
s32 LobbyUser_Search(NetUserSearchCond *cond, s8 n);
s32 HunterInfo_Update(u16 pad);
void HunterInfo_Open(void *, s32);

/*
 * The area search's frame, from the game menu: asks the server for the
 * area's hunters (unless the last answer is still kept), then runs the
 * results; a hunter chosen there opens that hunter's info (with joining on
 * offer only while the player isn't in a group), and leaving it brings the
 * results back. Returns MENU_CANCEL when the results are left, MENU_CLOSE
 * on start (the game menu closes), MENU_UNDONE when the search failed and
 * its notice is closed, MENU_RUNNING meanwhile.
 */
s32 AreaSearch_Update(u16 pad)
{
    AreaSearchMenu *menu = (AreaSearchMenu *)&gMenuWork;

    switch (menu->step) {
    case AREA_SEARCH_START:
        if (gAreaSearchTimer == 0) {
            menu->step++;
            menu->conds[0].type = 1;
            LobbyText_Open(LOBBY_TEXT_HUNTER_SEARCHING);
            gLobbyMenu->redraw = 1;
        } else {
            menu->step = AREA_SEARCH_RESULTS;
            AreaSearch_ShowResults(menu);
        }
        break;
    case AREA_SEARCH_FINDING:
        gLobbyMenu->redraw = 1;
        switch (LobbyUser_Search(menu->conds, 0)) {
        case LOBBY_STEP_DONE:
            menu->step++;
            gAreaSearchTimer = gAreaSearchFrames;
            AreaSearch_ShowResults(menu);
            break;
        case LOBBY_STEP_FAILED:
            menu->step = AREA_SEARCH_FAILED;
            break;
        }
        break;
    case AREA_SEARCH_RESULTS:
        switch (AreaSearch_ResultsInput(menu, pad)) {
        case MENU_DONE:
            menu->step = AREA_SEARCH_HUNTER;
            if (gLobbyLevel1Sel != 0) {
                HunterInfo_Open(menu->rows[menu->cursor], HUNTER_INFO_MAIL | HUNTER_INFO_FRIEND);
            } else {
                HunterInfo_Open(menu->rows[menu->cursor], HUNTER_INFO_MAIL | HUNTER_INFO_FRIEND | HUNTER_INFO_JOIN);
            }
            break;
        case MENU_CANCEL:
            AreaSearch_Close();
            return MENU_CANCEL;
        case MENU_CLOSE:
            AreaSearch_Close();
            return MENU_CLOSE;
        }
        break;
    case AREA_SEARCH_HUNTER:
        switch (HunterInfo_Update(pad)) {
        case MENU_DONE:
        case 1:
        case MENU_CANCEL:
            menu->step = AREA_SEARCH_RESULTS;
            menu->inputStep = AREA_RESULTS_LIST;
            break;
        case MENU_CLOSE:
            AreaSearch_Close();
            return MENU_CLOSE;
        }
        break;
    case AREA_SEARCH_FAILED:
        gLobbyMenu->redraw = 1;
        if (pad & PAD_CIRCLE) {
            AreaSearch_Close();
            return MENU_UNDONE;
        }
        if (pad & PAD_START) {
            AreaSearch_Close();
            return MENU_CLOSE;
        }
        break;
    }
    return MENU_RUNNING;
}

s32 Menu_CountPages(s16 n, s16 per);

/* Fills the results' rows with page `page` of the hunters found (all but
 * the answer's last record), counts the pages, and keeps the cursor on the
 * page. */
void AreaSearch_BuildPage(AreaSearchMenu *menu, s16 page)
{
    s16 i;
    s16 n = 0;
    s16 first = page * AREA_SEARCH_ROWS_PER_PAGE;
    s16 last = first + AREA_SEARCH_ROWS_PER_PAGE - 1;

    for (i = 0; i < gUserSearchFound->total; i++) {
        if (i >= first && i <= last && i != AREA_SEARCH_UNLISTED) {
            menu->rows[n] = &gUserSearchFound->users[i];
            n++;
        }
    }
    menu->count = n;
    menu->page = page;
    menu->pageCount = Menu_CountPages(gUserSearchFound->total, AREA_SEARCH_ROWS_PER_PAGE);
    if (menu->cursor >= menu->count) {
        menu->cursor = menu->count - 1;
    }
}

/* Shows the results from their first page and starts sliding them in. */
void AreaSearch_ShowResults(AreaSearchMenu *menu)
{
    AreaSearch_BuildPage(menu, 0);
    WinSlide_Step(&menu->win, 0x280, 0x12C);
}

extern char gMyUserId[];
s32 memcmp(void *a, void *b, s32 n);
void LobbySe_Play(s32);
s32 Menu_WrapUpDown(s32, s32);
s32 Menu_WrapLeftRight(s32, s32);

/*
 * The area search's results: slide in, then the cursor moves over the page
 * and left or right turns the pages. Circle on a hunter returns MENU_DONE,
 * unless the hunter is the player (refused). Returns MENU_CANCEL on cross,
 * MENU_CLOSE on start, MENU_RUNNING meanwhile.
 */
s32 AreaSearch_ResultsInput(AreaSearchMenu *menu, u16 pad)
{
    switch (menu->inputStep) {
    case AREA_RESULTS_SLIDE_IN:
        switch (WinSlide_Step(&menu->win, 0x280, 0x12C)) {
        case MENU_DONE:
            menu->inputStep++;
            break;
        }
        break;
    case AREA_RESULTS_LIST:
        if (pad & PAD_CIRCLE) {
            if (menu->count != 0) {
                if (memcmp(menu->rows[menu->cursor]->id, gMyUserId, LOBBY_USER_ID_LEN) == 0) {
                    LobbySe_Play(LOBBY_SE_REFUSE);
                    break;
                }
                menu->inputStep = AREA_RESULTS_SLIDE_IN;
                LobbySe_Play(LOBBY_SE_CONFIRM);
                return MENU_DONE;
            }
        } else if (pad & PAD_CROSS) {
            menu->inputStep = AREA_RESULTS_SLIDE_IN;
            LobbySe_Play(LOBBY_SE_CANCEL);
            return MENU_CANCEL;
        } else if (pad & PAD_START) {
            return MENU_CLOSE;
        } else if (pad & (PAD_LEFT | PAD_RIGHT)) {
            if (menu->pageCount > 1) {
                menu->page = Menu_WrapLeftRight(menu->page, menu->pageCount);
                AreaSearch_BuildPage(menu, menu->page);
            }
        } else {
            menu->cursor = Menu_WrapUpDown(menu->cursor, menu->count);
        }
        break;
    }
    return MENU_RUNNING;
}

void Win_Draw(s16, s16, s16, s16, s32, s32, s32); /* unit's own declaration: s16 coordinates and sizes and an s32 kind here; with the definition's s32 coordinates and s16 kind, AreaSearch_DrawResults compiles differently */
void Gfx_DrawLine(s16 x, s16 y, s16 w, s16 h, s32 color);
void Font_SetPos(s16 x, s16 y);
void Font_SetColor(s32 color);
void Text_PrintfColored(const char *fmt, ...);
void Menu_DrawLine(s16 x, s16 y0, s16 w, s16 y1, s8 flag);
void Menu_DrawPageNum(s16 x, s16 y, s16 page, s16 n, u8 alt);
void Icon_DrawWeaponClass(s16, s16, s32, s32, u8, s32); /* unit's own declaration: s16 coordinates and a u8 kind here; with the definition's types, AreaSearch_DrawResults compiles differently */
void Sjis_Widen(char *src, char *dst);
extern char gAreaSearchTextFmt[];
extern char gAreaSearchHrFmt[];

/*
 * Draws the area search's results at x, y: the title, then either "no
 * results" or the page number and the page's hunters, each with name, id
 * (widened to full-width characters), weapon class icon and hunter rank,
 * the cursor's line drawn lit while the results have the pad. A last page
 * cut short by the unlisted record ends with a note.
 */
void AreaSearch_DrawResults(AreaSearchMenu *menu, s16 x, s16 y)
{
    HunterCard *card;
    char wideId[0x20];
    s8 dim;
    s16 rowY = y;
    s16 i;

    Win_Draw(x, y, 0x140, 0x122, 0xBDB59F6C, 0xBB402010, 8);
    Gfx_DrawLine(x + 8, y + 0xC, 0x130, 0x14, 0x80B59F6C);
    Font_SetColor(0x807F7F28);
    y += 0xE;
    Font_SetPos(x + 0x10, y);
    Text_PrintfColored(gSearchMenuTexts[SEARCH_MENU_TEXT_AREA]);
    if (menu->count == 0) {
        Font_SetColor(0x802B7F7F);
        Font_SetPos(Text_CenterX(gEmptyListTexts[EMPTY_LIST_NO_RESULTS], x + 0xA0, 0x12), y + 0x64);
        Text_PrintfColored(gEmptyListTexts[EMPTY_LIST_NO_RESULTS]);
        return;
    }
    Menu_DrawPageNum(x + 0xE6, y, menu->page, menu->pageCount, 1);
    rowY += 0x24;
    Font_SetColor(0x807F7F28);
    Font_SetPos(x + 0x10, rowY);
    Text_PrintfColored(gNetLabels[NET_LABEL_NAME]);
    Font_SetPos(x + 0x62, rowY);
    Text_PrintfColored(gNetLabels[NET_LABEL_ID]);
    Font_SetPos(x + 0xD8, rowY);
    Text_PrintfColored(gNetLabels[NET_LABEL_WEAPON]);
    Font_SetPos(x + 0x106, rowY);
    Text_PrintfColored(gNetLabels[NET_LABEL_HR]);
    for (i = 0; i < menu->count; i++) {
        Font_SetColor(0x807F7F7F);
        rowY += 0x16;
        if (menu->cursor == i) {
            switch (menu->step) {
            case AREA_SEARCH_RESULTS:
                dim = 0;
                break;
            default:
                dim = 1;
                break;
            }
            Menu_DrawLine(x + 8, rowY - 1, 0x130, 0x14, dim);
        }
        Font_SetPos(x + 0x10, rowY);
        Text_PrintfColored(gAreaSearchTextFmt, menu->rows[i]->name);
        Font_SetPos(x + 0x62, rowY);
        Sjis_Widen(menu->rows[i]->id, wideId);
        Text_PrintfColored(wideId);
        card = (HunterCard *)menu->rows[i]->text;
        Icon_DrawWeaponClass(x + 0xE2, rowY - 2, 0x16, -1, card->weaponClass, 0);
        Font_SetPos(x + 0x106, rowY);
        Text_PrintfColored(gAreaSearchHrFmt, card->hr);
    }
    if (menu->page == AREA_SEARCH_PAGES - 1 && i == AREA_SEARCH_ROWS_PER_PAGE - 1) {
        Font_SetColor(0x802B7F7F);
        Font_SetPos(x + 0x10, rowY + 0x16);
        Text_PrintfColored(gAreaSearchTextFmt, gNetLabels[NET_LABEL_AREA_JOIN_PROMPT]);
    }
}

void Font_SetSpacing(u8 a, u8 b);
void HunterInfo_DrawIfOpen(void);

/*
 * The area search's first draw callback: the results, or the hunter's info
 * once it is open.
 */
void AreaSearch_Draw(MenuTask *task)
{
    AreaSearchMenu *menu = (AreaSearchMenu *)&gMenuWork;

    Text_SetSize(task->textSize);
    Font_SetSpacing(0x12, 0x12);
    switch (menu->step) {
    case AREA_SEARCH_RESULTS:
        AreaSearch_DrawResults(menu, menu->win.pos, 0x3A);
        break;
    case AREA_SEARCH_HUNTER:
        HunterInfo_DrawIfOpen();
        break;
    }
}

/* The area search's second draw callback: only sets the text size. */
void AreaSearch_Draw2(MenuTask *task)
{
    Text_SetSize(task->textSize);
}

/* The area search's read-only data: its string and hunter rank formats. */
#pragma rodata on
char gAreaSearchTextFmt[0x8] = {
#include "lobby/008381A0.inc"
};
char gAreaSearchHrFmt[0x8] = {
#include "lobby/008381A8.inc"
};
#pragma rodata off
