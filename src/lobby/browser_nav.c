/* The browser's mode and step changes: reloading or opening a page, the
 * failure and the leave dialog, a page step asked for (Browser_SetStep), the
 * browser's result (Browser_Exit); going to a URL (Browser_Go), the page's
 * scrolling and moves (Browser_Move); closing a dialog with its
 * answer's sound (Dialog_Close) and moving a dialog's cursor over its
 * list (Browser_ListCursor). The portal's steps, the dialogs and the
 * page's input call them. */
#include "common.h"
#include "browser.h"
#include "lobby_menu.h"
#include "render_state.h"
#include "kbd.h"
#include "pad.h"
#include "lobby_util.h"
#include "libc.h"

/* DLG_STEP_STOPPED: nothing. */
void Dialog_Stopped(void)
{
}

/* Loads the page again: BMODE_RUN from PAGE_STEP_START, with no object in
 * use and no refresh pending. */
void Browser_Reload(void)
{
    gBrowserSub->activeObj = 0;
    gBrowserWork->refreshTimer = 0;
    gBrowserState->mode = BMODE_RUN;
    gBrowserState->step = PAGE_STEP_START;
    gBrowserState->sub = 0;
}

/* Opens the page asked for: BMODE_RUN at PAGE_STEP_LOAD, the page
 * PAGE_LOAD_REQUESTED, no history move. The page fades in (fade -16)
 * unless the URL is the MMBB data's; before the first page's objects, with
 * no fade, and on black in Browser_Init's other mode (initMode). */
void Browser_OpenPage(void)
{
    gBrowserState->histDir = BHIST_NONE;
    gBrowserState->page = PAGE_LOAD_REQUESTED;
    gBrowserSub->activeObj = 0;
    gBrowserWork->refreshTimer = 0;
    gBrowserState->mode = BMODE_RUN;
    gBrowserState->step = PAGE_STEP_LOAD;
    gBrowserState->sub = 0;
    if (gBrowserWork->atMmbbUrl == 0) {
        gBrowserState->fade = -16;
    }
    if (gBrowserWork->obj[0] == NULL) {
        gBrowserState->fade = 0;
        if (gBrowserState->initMode != 0) {
            gBrowserState->bgColor = BROWSER_BLACK;
        }
    }
}

/* Shows the page as it is: PAGE_LOAD_DONE, no box, PAGE_STEP_DIALOG. */
void Browser_ShowPage(void)
{
    gBrowserState->page = PAGE_LOAD_DONE;
    gBrowserWork->dlgBoxKind = BDLG_KIND_NONE;
    gBrowserState->mode = BMODE_RUN;
    gBrowserState->step = PAGE_STEP_DIALOG;
}

/* Fails the portal: the page state `page`, a black background, BMODE_END
 * from PORTAL_END_START; the dialogs stop (DLG_STEP_STOPPED). */
void Browser_Fail(u8 page)
{
    gBrowserState->page = page;
    Browser_SetRenderState(RS_BG_COLOR, BROWSER_BLACK);
    gBrowserState->mode = BMODE_END;
    gBrowserState->step = PORTAL_END_START;
    gBrowserSub->dlgStep = DLG_STEP_STOPPED;
}

/* Opens the leave dialog (BDLG_MODE_EXIT, BDLG_KIND_EXIT) over the page, at
 * PAGE_STEP_DIALOG; the pointer hides. The argument isn't read. */
void Browser_OpenLeaveDialog(s32 unused)
{
    gBrowserSub->dlgMode = BDLG_MODE_EXIT;
    gBrowserState->prevPage = 0;
    gBrowserState->mode = BMODE_RUN;
    gBrowserState->step = PAGE_STEP_DIALOG;
    gBrowserState->sub = 0;
    gBrowserWork->dlgBoxKind = BDLG_KIND_EXIT;
    gBrowserWork->dlgPointerState = BDLG_POINTER_HIDDEN;
}

/* Goes to the page step `step` (enum PageStep), cancelling the requests:
 * PAGE_STEP_GAME_MENU to PAGE_STEP_FAIL always, the others only when no
 * style wait or fade is running. */
void Browser_SetStep(s8 step)
{
    switch (step) {
    default:
        if (gBrowserState->styleWait != 0 || gBrowserState->fade != 0) {
            return;
        }
    case PAGE_STEP_GAME_MENU:
    case PAGE_STEP_CANCEL:
    case PAGE_STEP_WAIT_REQ:
    case PAGE_STEP_IMAGES:
    case PAGE_STEP_FAIL:
        BrowserReq_CancelAll();
        gBrowserState->step = step;
    }
}

/* Asks for the page at `url` (kept as the state's URL). */
void Browser_Go(char *url)
{
    memset(gBrowserState->url, 0, sizeof(gBrowserState->url));
    strcpy(gBrowserState->url, url);
    Browser_SetUrl(gBrowserWork, gBrowserState->url);
}

/* Scrolls the page by (dx, dy), closing an open list. */
void Browser_ScrollBy(s32 dx, s32 dy)
{
    gBrowserState->x += dx;
    gBrowserState->y += dy;
    gBrowserSub->listCount = 0;
}

/* A move by the pad (BMOVE_*): scrolls the page by (dx, dy), or the open
 * drop-down list by dy lines. The pressed arrow shows for 3 frames
 * (pressTimer), and the move's kind is kept (moveKind). */
void Browser_Move(s32 dx, s32 dy, u8 kind)
{
    switch (kind) {
    case BMOVE_UP:
    case BMOVE_DOWN:
    case BMOVE_LEFT:
    case BMOVE_RIGHT:
        gBrowserState->x += dx;
        gBrowserState->y += dy;
        break;
    case BMOVE_LIST_UP:
    case BMOVE_LIST_DOWN:
        gBrowserSub->listTop += (u8)dy;
        break;
    case BMOVE_NONE:
        break;
    }
    gBrowserWork->pressTimer = BROWSER_PRESS_FRAMES;
    gBrowserSub->moveKind = kind;
}

/* Leaves the browser: BMODE_DONE with its result (BROWSER_*, or the game
 * menu's number). */
void Browser_Exit(s8 result)
{
    gBrowserWork->result = result;
    gBrowserState->mode = BMODE_DONE;
    gBrowserState->step = 0;
    gBrowserState->sub = 0;
}

/* Closes the dialog with its answer's sound (yes: the confirm, no: the
 * cancel) and gives the pad back to the page. */
void Dialog_Close(s32 yes)
{
    s32 answer;

    answer = yes & 0xFF;
    switch (answer) {
    case 0:
        LobbySe_Play(LOBBY_SE_CANCEL);
        break;
    case 1:
        LobbySe_Play(LOBBY_SE_CONFIRM_ALT);
        break;
    }
    gBrowserSub->dlgMode = BDLG_MODE_PAGE;
    gBrowserWork->dlgBoxKind = BDLG_KIND_NONE;
}

/* Moves the dialog's cursor (BrowserSub.dlgCursor) over a list of `max`
 * lines, 1 to max, without wrapping: left, up, the left stick's directions
 * 8 to 15 or the USB keyboard's left and up step back; right, down,
 * directions 1 to 7 or the keyboard's right and down step on. The scroll
 * sound plays when it moves. */
void Browser_ListCursor(u8 max)
{
    u32 pressed;
    u8 *cur = &gBrowserSub->dlgCursor;
    u8 old = *cur;
    u8 tilt;
    s32 key;
    s32 dir;

    pressed = gBrowserWork->padPressed;
    if ((pressed & PAD_LEFT) || (pressed & PAD_UP) ||
        ((tilt = gBrowserWork->stickTilt) != 0 && (dir = gBrowserWork->stickDir) >= 8 && dir <= 15) ||
        (key = gKbd->usbKey) == USB_KEY_LEFT || key == USB_KEY_UP) {
        (*cur)--;
    } else if ((pressed & PAD_RIGHT) || (pressed & PAD_DOWN) ||
               (tilt && (dir = gBrowserWork->stickDir) > 0 && dir <= 7) || key == USB_KEY_RIGHT || key == USB_KEY_DOWN) {
        (*cur)++;
    }
    cur = &gBrowserSub->dlgCursor;
    if (*cur <= 0) {
        *cur = 1;
    } else if (max < *cur) {
        *cur = max;
    }
    if (old != gBrowserSub->dlgCursor) {
        LobbySe_Play(LOBBY_SE_SCROLL);
    }
}
