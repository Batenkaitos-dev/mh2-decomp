/* The dialogs over the browser's page: Dialog_Run runs the dialogs' step
 * (BrowserSub.dlgStep, DLG_STEP_*), and Dialog_RunStep the step of the
 * dialog mode (BrowserSub.dlgMode, BDLG_MODE_*): the page's own input,
 * the menu of three, the message boxes' choices, the kept login, the
 * registration's confirmation and the leave dialog. */
#include "common.h"
#include "browser.h"
#include "lobby_menu.h"
#include "pad.h"
#include "render_state.h"
#include "html.h"
#include "lobby_globals.h"
#include "kbd.h"
#include "lobby_util.h"

/* Compiled at optimization level 4 (Dialog_PageStep matches only
 * so; every other function matches at 3 and 4). */
#pragma optimization_level 4

void *memcpy(void *dst, const void *src, u32 n);

u32 strlen(const char *s);

/* The left stick's 16 directions: a step up or down a list (.data, copied
 * to the stack whole). */
typedef struct StickSteps {
    s8 v[16];
} StickSteps __attribute__((aligned(16)));
extern StickSteps gDialogMenuStickSteps;
extern StickSteps gDialogChoiceStickSteps;
s32 memcmp(const char *a, const char *b, u32 n); /* main: strncmp */
extern char gPageFocusBackKey[]; /* a page's message that points at its back button */

extern u8 gPortalPage; /* how the browser was entered (PORTAL_PAGE_*) */
s32 strcmp(const char *a, const char *b); /* main: strcmp */

extern u8 gLobbyPageData[];     /* the registration record kept */

/* The dialogs' steps (by BrowserSub.dlgStep, DLG_STEP_*) and the dialog
 * modes' steps (by BrowserSub.dlgMode, BDLG_MODE_*). The tables keep their
 * address names: the port's recompiler takes a table the code calls
 * through as a jump table, and finds it by that name. */
extern void (*jtbl_007EEB78[])(void);
extern void (*jtbl_007EEB90[])(void);

/* Runs the dialogs' step (BrowserSub.dlgStep, DLG_STEP_*). */
void Dialog_Run(void)
{
    jtbl_007EEB78[gBrowserSub->dlgStep]();
}

/* DLG_STEP_RESET: no dialog, the pointer at (150, 100), no drag; then
 * DLG_STEP_RUN. */
void Dialog_Reset(void)
{
    gBrowserSub->dlgStep = DLG_STEP_RUN;
    gBrowserSub->dlgMode = BDLG_MODE_PAGE;
    gBrowserSub->cursorX = 150;
    gBrowserSub->cursorY = 100;
    gBrowserSub->dragX = 0;
    gBrowserSub->dragY = 0;
}

/* DLG_STEP_RUN: runs the dialog mode's step (BrowserSub.dlgMode, BDLG_MODE_*). */
void Dialog_RunStep(void)
{
    jtbl_007EEB90[gBrowserSub->dlgMode]();
}

/* BDLG_MODE_PAGE: the page has the pad. Nothing in the refresh timer's
 * last three frames, nor while the page is parsed; only the scrolling while
 * it is requested, and from PAGE_STEP_RESHOW on. By the page's style:
 * - PAGE_STYLE_CANCEL: the start button or Esc opens the menu of three (on
 *   a page with the pointer, no list open, the page in);
 * - PAGE_STYLE_RELOAD: the square or F5 runs Portal_Back (PAGE_STEP_BACK
 *   while the images load), unless nothing can be reloaded or the page is
 *   an lbs:// one;
 * - PAGE_STYLE_FORWARD: the triangle goes forward in the history;
 * - PAGE_STYLE_BACK: the cross or Backspace follows the page's message link
 *   (gPageCancelLink cancels the portal), or goes back without one;
 *   without that style, the message gPageFocusBackKey puts the pointer on
 *   the page's back button (an image map with its second highlight).
 * Then the circle or Enter clicks, and the pointer, the drag, the moves and
 * the scrolling follow the pad. */
void Dialog_PageStep(void)
{
    u16 i;
    s32 flags;
    s16 refresh;
    BrowserObj *pageObj;
    u8 page;

    refresh = gBrowserWork->refreshTimer;
    if (refresh > 0 && refresh < 4) {
        return;
    }
    page = gBrowserState->page;
    switch (page) {
    case PAGE_LOAD_NONE:
        return;
    case PAGE_LOAD_REQUESTED:
        Page_Navigate();
        Page_Clamp();
        return;
    case PAGE_LOAD_IMAGES:
        if (gBrowserState->initMode != 0) {
            return;
        }
        break;
    case PAGE_LOAD_DONE:
        break;
    case PAGE_LOAD_PARSING:
    case PAGE_LOAD_PARSING_ON:
    case PAGE_LOAD_PARSED:
        return;
    }
    if (gBrowserState->step >= PAGE_STEP_RESHOW) {
        Page_Navigate();
        Page_Clamp();
        return;
    }
    flags = gBrowserState->style;
    if (((flags & PAGE_STYLE_CANCEL) >> 5) != 0 && (flags & PAGE_STYLE_POINTER) && gBrowserSub->listCount == 0 &&
        ((gBrowserWork->padPressed & PAD_START) || gKbd->usbKey == USB_KEY_ESCAPE) && page == PAGE_LOAD_DONE && gBrowserState->plain == 0) {
        LobbySe_Play(LOBBY_SE_CONFIRM_ALT);
        gBrowserWork->dlgMenuState = BDLG_MENU_SHOWN;
        gBrowserSub->dlgMode = BDLG_MODE_MENU;
        gBrowserWork->dlgStatusState = BDLG_STATUS_TITLE;
        gBrowserSub->dlgCursor = BMENU_CLOSE;
        gBrowserSub->listCount = 0;
        gBrowserSub->activeObj = 0;
        gBrowserWork->dlgPointerState = BDLG_POINTER_HIDDEN;
        return;
    }
    if (((flags & PAGE_STYLE_RELOAD) >> 6) != 0 && ((gBrowserWork->padPressed & PAD_SQUARE) || gKbd->usbKey == USB_KEY_F5)) {
        if (gBrowserState->styleWait != 0 || gBrowserState->fade != 0) {
            return;
        }
        if (BrowserReq_CanReload() == 0) {
            return;
        }
        if (memcmp(BrowserHist_CurUrl(), gLbsScheme, strlen(gLbsScheme)) == 0) {
            return;
        }
        page = gBrowserState->page;
        if (page == PAGE_LOAD_REQUESTED) {
            return;
        }
        if (page == PAGE_LOAD_DONE) {
            Portal_Back();
            return;
        }
        if (page == PAGE_LOAD_IMAGES) {
            Browser_SetStep(PAGE_STEP_BACK);
            return;
        }
    }
    if (((gBrowserState->style & PAGE_STYLE_FORWARD) >> 4) != 0 && (gBrowserWork->padPressed & PAD_TRIANGLE)) {
        if (gBrowserState->styleWait != 0 || gBrowserState->fade != 0) {
            return;
        }
        if (page == PAGE_LOAD_REQUESTED) {
            return;
        }
        if (page == PAGE_LOAD_DONE) {
            Browser_GoForward();
            return;
        }
        if (page == PAGE_LOAD_IMAGES) {
            if (BrowserHist_CanForward() == 0) {
                return;
            }
            Browser_SetStep(PAGE_STEP_HISTORY);
            gBrowserState->histDir = BHIST_FORWARD;
            return;
        }
    }
    flags = (gBrowserState->style & PAGE_STYLE_BACK) >> 3;
    if (flags != 0 && ((gBrowserWork->padPressed & PAD_CROSS) || gKbd->usbKey == USB_KEY_BACKSPACE)) {
        if (gBrowserState->styleWait != 0 || gBrowserState->fade != 0) {
            return;
        }
        if (strlen(gBrowserState->msg) != 0) {
            if (gBrowserState->page == PAGE_LOAD_REQUESTED) {
                return;
            }
            LobbySe_Play(LOBBY_SE_CANCEL);
            if (memcmp(gBrowserState->msg, gPageCancelLink, strlen(gPageCancelLink)) == 0) {
                if (gBrowserState->page == PAGE_LOAD_IMAGES) {
                    Browser_SetStep(PAGE_STEP_CANCEL);
                    return;
                }
                gPortalExit = PORTAL_EXIT_CANCEL;
                Browser_Fail(0);
                return;
            }
            Browser_Go(gBrowserState->msg);
            gBrowserSub->activeObj = 0;
            if (gBrowserState->page == PAGE_LOAD_IMAGES) {
                Browser_SetStep(PAGE_STEP_RELOAD);
                return;
            }
            Browser_Reload();
        } else {
            if (gBrowserState->page == PAGE_LOAD_REQUESTED) {
                return;
            }
            LobbySe_Play(LOBBY_SE_CANCEL);
            if (gBrowserState->page == PAGE_LOAD_DONE) {
                Browser_GoBack();
                return;
            }
            if (gBrowserState->page == PAGE_LOAD_IMAGES) {
                if (BrowserHist_CanBack() == 0) {
                    return;
                }
                Browser_SetStep(PAGE_STEP_HISTORY);
                gBrowserState->histDir = BHIST_BACK;
                return;
            }
        }
    } else if (flags == 0 && ((gBrowserWork->padPressed & PAD_CROSS) || gKbd->usbKey == USB_KEY_BACKSPACE) &&
               strlen(gBrowserState->msg) != 0) {
        for (i = 0; i < PAGE_OBJ_COUNT; i++) {
            if (gBrowserWork->obj[i] == NULL || !gBrowserWork->obj[i]->active) {
                break;
            }
            if (gBrowserWork->obj[i]->kind == PAGE_OBJ_IMAGE && gBrowserWork->obj[i]->active == PAGE_IMG_MAP && ((gBrowserWork->obj[i]->field & 0x10) >> 4) == 1 &&
                strcmp(gBrowserState->msg, gPageFocusBackKey) == 0) {
                gBrowserWork->focus = i;
                pageObj = gBrowserWork->obj[i];
                gBrowserSub->cursorX = pageObj->x + pageObj->w / 2.0f - (f32)gBrowserState->x;
                pageObj = gBrowserWork->obj[i];
                gBrowserSub->cursorY = pageObj->y + pageObj->h / 2.0f - (f32)gBrowserState->y;
                Page_PointAt();
                LobbySe_Play(LOBBY_SE_CANCEL);
                return;
            }
        }
    }
    if (gBrowserState->page >= PAGE_LOAD_IMAGES) {
        if ((gBrowserWork->padPressed & PAD_CIRCLE) || gKbd->usbKey == USB_KEY_ENTER || gKbd->usbKey == USB_KEY_KP_ENTER) {
            Page_Click();
        }
    }
    if (gBrowserWork->padPressed & PAD_CIRCLE) {
        Page_PointerScroll();
    }
    if (gBrowserWork->padHeld & PAD_CIRCLE) {
        Page_DragScroll();
    }
    if (!(gBrowserWork->padPressed & PAD_CIRCLE)) {
        Page_Navigate();
    }
    if (gBrowserSub->listCount == 0) {
        Page_ScrollInput();
    }
    Page_Clamp();
    if (gBrowserSub->activeObj == 0) {
        Page_PointAt();
    }
}

/* BDLG_MODE_INPUT: keeps the keyboard open. */
void Dialog_InputStep(void)
{
    gBrowserWork->dlgInputState = BDLG_INPUT_OPEN;
}

/* Closes the menu of three: the pointer back, BDLG_MODE_PAGE. */
void Dialog_ResetMsgState(void)
{
    gBrowserWork->dlgMenuState = BDLG_MENU_HIDDEN;
    gBrowserWork->dlgPointerState = BDLG_POINTER_SHOWN;
    gBrowserSub->dlgMode = BDLG_MODE_PAGE;
}

/* BDLG_MODE_MENU: the menu of three (BMENU_*). The start button or Esc
 * closes it. The circle or Enter takes the line (not while the images
 * load): BMENU_BACK goes back a page (Portal_Back), BMENU_LEAVE leaves the
 * portal (PORTAL_EXIT_LEAVE), BMENU_CLOSE closes the menu. The cursor moves
 * with up and down, the USB keyboard's arrows or the left stick (a step
 * every 8 frames held, or at once on a new tilt: stickReleased,
 * stickFirst; gDialogMenuStickSteps gives each of the stick's 16 directions
 * its step), and wraps round. */
void Dialog_MenuStep(void)
{
    StickSteps dirs = gDialogMenuStickSteps;
    s8 sel;
    u8 i;
    s32 pad;
    u8 k;

    if ((gBrowserWork->padPressed & PAD_START) || gKbd->usbKey == USB_KEY_ESCAPE) {
        LobbySe_Play(LOBBY_SE_CANCEL);
        gBrowserWork->dlgMenuState = BDLG_MENU_HIDDEN;
        gBrowserSub->dlgMode = BDLG_MODE_PAGE;
        gBrowserWork->dlgPointerState = BDLG_POINTER_SHOWN;
    }
    if ((gBrowserWork->padPressed & PAD_CIRCLE) || gKbd->usbKey == USB_KEY_ENTER || gKbd->usbKey == USB_KEY_KP_ENTER) {
        LobbySe_Play(LOBBY_SE_CONFIRM_ALT);
        if (gBrowserState->page == PAGE_LOAD_IMAGES) {
            return;
        }
        for (i = 0; i < BMENU_LINES; i++) {
            if (i == gBrowserSub->dlgCursor - 1) {
                switch (i) {
                case BMENU_BACK - 1:
                    if (gBrowserState->styleWait != 0 || gBrowserState->fade != 0) {
                        return;
                    }
                    Dialog_ResetMsgState();
                    Portal_Back();
                    return;
                case BMENU_LEAVE - 1:
                    gPortalExit = PORTAL_EXIT_LEAVE;
                    Browser_Fail(0);
                    return;
                case BMENU_CLOSE - 1:
                    Dialog_ResetMsgState();
                    return;
                }
            }
        }
    }
    sel = gBrowserSub->dlgCursor;
    if (gBrowserWork->stickTilt != 0) {
        if (gBrowserWork->stickReleased == 1) {
            gBrowserWork->stickFirst = 1;
        } else {
            gBrowserWork->stickFirst = 0;
        }
        gBrowserWork->stickReleased = 0;
        if (gBrowserSub->stickRepeat > 1000) {
            gBrowserSub->stickRepeat = 0;
        }
        if (++gBrowserSub->stickRepeat % 8 == 0 || gBrowserWork->stickFirst != 0) {
            sel += dirs.v[gBrowserWork->stickDir];
            gBrowserSub->stickRepeat = 0;
            gBrowserSub->dlgCursor = sel;
        } else {
            return;
        }
    } else {
        pad = gBrowserWork->padPressed;
        if ((pad & PAD_UP) || (pad & PAD_DOWN)) {
            if (pad & PAD_UP) {
                sel--;
            }
            if (pad & PAD_DOWN) {
                sel++;
            }
        } else {
            k = gKbd->usbKey;
            if (k == USB_KEY_UP || k == USB_KEY_DOWN) {
                if (k == USB_KEY_UP) {
                    sel--;
                }
                if (k == USB_KEY_DOWN) {
                    sel++;
                }
            } else {
                gBrowserWork->stickReleased = 1;
                gBrowserSub->stickRepeat = 0;
                return;
            }
        }
    }
    if (sel <= 0) {
        sel = BMENU_LINES;
    }
    if (sel > BMENU_LINES) {
        sel = BMENU_BACK;
    }
    LobbySe_Play(LOBBY_SE_SCROLL);
    gBrowserSub->dlgCursor = sel;
}

/* Closes the message box: the pointer back, BDLG_MODE_PAGE. */
void Dialog_ResetBoxState(void)
{
    gBrowserState->fromTop = 0;
    gBrowserWork->dlgBoxKind = BDLG_KIND_NONE;
    gBrowserWork->dlgPointerState = BDLG_POINTER_SHOWN;
    gBrowserSub->dlgMode = BDLG_MODE_PAGE;
}

/* BDLG_MODE_CHOICE: the message box's choice, by its kind (dlgBoxKind):
 * - BDLG_KIND_MESSAGE, once it asked its question 1: keep the login id and
 *   the password, keep the id, or nothing (the box closes);
 * - BDLG_KIND_YES_NO: yes keeps the sign-up page's login id; no, the cross
 *   or Esc goes back a page without keeping it;
 * - BDLG_KIND_CLEAR_LOGIN, once it asked its question 2: clear the kept
 *   login (and save), or keep it.
 * A choice that saves goes on to BDLG_MODE_SAVE_LOGIN, with loginSave =
 * LOGIN_SAVE(the page's kind, the answer). The cursor moves as in
 * Dialog_MenuStep, over three lines or two, without wrapping. The pad is
 * read into a second `s32 c` for the cursor: with one variable for both,
 * the compiler swaps two registers. */
void Dialog_ChoiceStep(void)
{
    StickSteps dirs = gDialogChoiceStickSteps;
    s8 max;
    s8 sel;
    s32 pad;
    s32 padCursor;
    u8 k;

    switch (gBrowserWork->dlgBoxKind) {
    case BDLG_KIND_MESSAGE:
        max = 3;
        break;
    case BDLG_KIND_YES_NO:
        max = 2;
        break;
    case BDLG_KIND_CLEAR_LOGIN:
        max = 2;
        break;
    default:
        max = 2;
        break;
    }
    gBrowserWork->loginSave = 0;
    switch (gBrowserWork->dlgBoxKind) {
    case BDLG_KIND_MESSAGE:
        if ((gBrowserWork->padPressed & PAD_CIRCLE) || gKbd->usbKey == USB_KEY_ENTER || gKbd->usbKey == USB_KEY_KP_ENTER) {
            if (gBrowserWork->dlgQuestion != 1) {
                return;
            }
            LobbySe_Play(LOBBY_SE_CONFIRM_ALT);
            switch (gBrowserSub->dlgCursor) {
            case 1:
                gBrowserWork->loginSave = LOGIN_KEEP_ID_PASSWORD;
                break;
            case 2:
                gBrowserWork->loginSave = LOGIN_KEEP_ID;
                break;
            case 3:
                gBrowserWork->loginSave = LOGIN_KEEP_NONE;
                Dialog_ResetBoxState();
                return;
            }
            gBrowserWork->loginSave += (s8)(gBrowserState->pageKind * 10);
            gBrowserWork->errorKind = 0;
            gBrowserSub->dlgMode = BDLG_MODE_SAVE_LOGIN;
            gBrowserWork->keepLoginStep = 0;
            return;
        }
        goto move;
    case BDLG_KIND_YES_NO:
        pad = gBrowserWork->padPressed;
        if ((pad & PAD_CIRCLE) || gKbd->usbKey == USB_KEY_ENTER || gKbd->usbKey == USB_KEY_KP_ENTER) {
            LobbySe_Play(LOBBY_SE_CONFIRM_ALT);
            Dialog_ResetBoxState();
            switch (gBrowserSub->dlgCursor) {
            case 1:
                gBrowserWork->loginSave = LOGIN_SAVE(PAGE_KIND_SIGNUP_TOP, LOGIN_KEEP_ID);
                break;
            case 2:
                if (gBrowserState->page == PAGE_LOAD_IMAGES) {
                    gBrowserState->styleWait = 0;
                    Browser_SetStep(PAGE_STEP_HISTORY);
                    gBrowserState->histDir = BHIST_BACK;
                    return;
                }
                gBrowserWork->loginSave = LOGIN_SAVE(PAGE_KIND_SIGNUP_TOP, LOGIN_KEEP_NONE);
                Browser_GoBack();
                break;
            }
            return;
        }
        if ((pad & PAD_CROSS) || gKbd->usbKey == USB_KEY_ESCAPE) {
            LobbySe_Play(LOBBY_SE_CANCEL);
            Dialog_ResetBoxState();
            if (gBrowserState->page == PAGE_LOAD_IMAGES) {
                gBrowserState->styleWait = 0;
                Browser_SetStep(PAGE_STEP_HISTORY);
                gBrowserState->histDir = BHIST_BACK;
                return;
            }
            gBrowserWork->loginSave = LOGIN_SAVE(PAGE_KIND_SIGNUP_TOP, LOGIN_KEEP_NONE);
            Browser_GoBack();
            return;
        }
        goto move;
    case BDLG_KIND_CLEAR_LOGIN:
        if ((gBrowserWork->padPressed & PAD_CIRCLE) || gKbd->usbKey == USB_KEY_ENTER || gKbd->usbKey == USB_KEY_KP_ENTER) {
            if (gBrowserWork->dlgQuestion != 2) {
                return;
            }
            LobbySe_Play(LOBBY_SE_CONFIRM_ALT);
            switch (gBrowserSub->dlgCursor) {
            case 1:
                gBrowserWork->loginSave = LOGIN_SAVE(PAGE_KIND_MMBB_KEICAN, LOGIN_CLEAR);
                gBrowserWork->errorKind = 0;
                gBrowserSub->dlgMode = BDLG_MODE_SAVE_LOGIN;
                gBrowserWork->keepLoginStep = 0;
                break;
            case 2:
                gBrowserWork->loginSave = LOGIN_SAVE(PAGE_KIND_MMBB_KEICAN, LOGIN_KEEP_NONE);
                Dialog_ResetBoxState();
                break;
            }
            return;
        }
    default:
    move:
        sel = gBrowserSub->dlgCursor;
        if (gBrowserWork->stickTilt != 0) {
            if (gBrowserWork->stickReleased == 1) {
                gBrowserWork->stickFirst = 1;
            } else {
                gBrowserWork->stickFirst = 0;
            }
            gBrowserWork->stickReleased = 0;
            if (gBrowserSub->stickRepeat > 1000) {
                gBrowserSub->stickRepeat = 0;
            }
            if (++gBrowserSub->stickRepeat % 8 == 0 || gBrowserWork->stickFirst != 0) {
                sel += dirs.v[gBrowserWork->stickDir];
                gBrowserSub->stickRepeat = 0;
            } else {
                return;
            }
        } else {
            padCursor = gBrowserWork->padPressed;
            if ((padCursor & PAD_UP) || (padCursor & PAD_DOWN)) {
                if (padCursor & PAD_UP) {
                    sel--;
                }
                if (padCursor & PAD_DOWN) {
                    sel++;
                }
            } else {
                k = gKbd->usbKey;
                if (k == USB_KEY_UP || k == USB_KEY_DOWN) {
                    if (k == USB_KEY_UP) {
                        sel--;
                    }
                    if (k == USB_KEY_DOWN) {
                        sel++;
                    }
                } else {
                    gBrowserWork->stickReleased = 1;
                    gBrowserSub->stickRepeat = 0;
                    return;
                }
            }
        }
        if (sel <= 0) {
            sel = 1;
        } else if (sel > max) {
            sel = max;
        } else {
            LobbySe_Play(LOBBY_SE_SCROLL);
        }
        gBrowserSub->dlgCursor = sel;
        break;
    }
}

/* BDLG_MODE_SAVE_LOGIN: Browser_SaveLogin. */
void Dialog_SaveLogin(void)
{
    Browser_SaveLogin();
}

/* Page_PointTo: puts the pointer on object i (10 pixels in), scrolling
 * the page to it; unless anywhere, only an object inside the view; the
 * object becomes the hovered one (focus). 1, or 0 when it is outside. */
s32 Page_PointTo(u16 i, s8 anywhere)
{
    if (anywhere == 0) {
        if (!(gBrowserWork->obj[i]->y <= (f32)(412 - gBrowserState->marginBottom))) {
            gBrowserWork->focus = 0;
            return 0;
        }
        if (!(gBrowserWork->obj[i]->x <= (f32)(624 - gBrowserState->marginRight))) {
            gBrowserWork->focus = 0;
            return 0;
        }
    }
    gBrowserSub->cursorY = 10.0f + (gBrowserWork->obj[i]->y - (f32)gBrowserState->y);
    if (gBrowserSub->cursorY > PAGE_VIEW_H) {
        gBrowserSub->cursorY = PAGE_VIEW_H;
        gBrowserState->y += 10.0f + (gBrowserWork->obj[i]->y - (f32)(gBrowserState->y + PAGE_VIEW_H));
    }
    if (gBrowserSub->cursorY < gBrowserState->marginTop) {
        gBrowserSub->cursorY = gBrowserState->marginTop + 10;
        gBrowserState->y -= 10.0f + ((f32)gBrowserState->y - gBrowserWork->obj[i]->y);
        if (gBrowserWork->obj[i]->y - (f32)gBrowserState->y < (f32)gBrowserState->marginTop) {
            gBrowserState->y -= (f32)gBrowserState->marginTop - (gBrowserWork->obj[i]->y - (f32)gBrowserState->y);
        }
    }
    gBrowserSub->cursorX = 10.0f + (gBrowserWork->obj[i]->x - (f32)gBrowserState->x);
    if (gBrowserSub->cursorX > PAGE_VIEW_W) {
        gBrowserSub->cursorX = PAGE_VIEW_W;
        gBrowserState->x += 10.0f + (gBrowserWork->obj[i]->x - (f32)(gBrowserState->x + PAGE_VIEW_W));
    }
    if (gBrowserSub->cursorX < gBrowserState->marginTop) {
        gBrowserSub->cursorX = gBrowserState->marginTop + 10;
        gBrowserState->x -= 10.0f + ((f32)gBrowserState->x - gBrowserWork->obj[i]->x);
        if (gBrowserWork->obj[i]->x - (f32)gBrowserState->x < (f32)gBrowserState->marginLeft) {
            gBrowserState->x -= (f32)gBrowserState->marginLeft - (gBrowserWork->obj[i]->x - (f32)gBrowserState->x);
        }
    }
    gBrowserWork->focus = i;
    return 1;
}

/* BDLG_MODE_REGISTER: asks to send the registration, yes or no. Yes keeps
 * the registration record (gLobbyPageData) and leaves the portal for the
 * game's menu (PORTAL_EXIT_GAME_MENU; through PAGE_STEP_GAME_MENU while the
 * images load); no, the cross or Esc closes the box. */
void Dialog_Register(void)
{
    Browser_ListCursor(2);
    if ((gBrowserWork->padPressed & PAD_CIRCLE) || gKbd->usbKey == USB_KEY_ENTER || gKbd->usbKey == USB_KEY_KP_ENTER) {
        switch (gBrowserSub->dlgCursor) {
        case 1:
            memcpy(gLobbyPageData, &gRegRecord, sizeof(gRegRecord));
            if (gBrowserState->page == PAGE_LOAD_IMAGES) {
                Browser_SetStep(PAGE_STEP_GAME_MENU);
                return;
            }
            LobbySe_Play(LOBBY_SE_CONFIRM_ALT);
            gPortalExit = PORTAL_EXIT_GAME_MENU;
            Browser_Fail(0);
            return;
        case 2:
            Dialog_Close(0);
            break;
        }
    }
    if ((gBrowserWork->padPressed & PAD_CROSS) || gKbd->usbKey == USB_KEY_ESCAPE) {
        Dialog_Close(0);
    }
}

/* BDLG_MODE_OK: the circle or Enter closes the box with 1; the cross or
 * Esc with 0. */
void Dialog_Ok(void)
{
    if ((gBrowserWork->padPressed & PAD_CIRCLE) || gKbd->usbKey == USB_KEY_ENTER || gKbd->usbKey == USB_KEY_KP_ENTER) {
        Dialog_Close(1);
    }
    if ((gBrowserWork->padPressed & PAD_CROSS) || gKbd->usbKey == USB_KEY_ESCAPE) {
        Dialog_Close(0);
    }
}

/* BDLG_MODE_EXIT: the leave dialog (BDLG_KIND_EXIT). After an end reason
 * other than a network error, the portal's top page (gPortalTopUrl) asks
 * yes or no whether the end goes on to the next URL (endGoOn); any other
 * page goes on. The circle or Enter leaves: from PORTAL_PAGE_TOP or
 * PORTAL_PAGE_NEXT with BROWSER_END_LEAVE (while images are still loading,
 * through PAGE_STEP_GAME_MENU, which stops them first); from
 * PORTAL_PAGE_BACK at once, except that it hangs while the images load. */
void Dialog_Exit(void)
{
    u16 i;
    u16 n;

    gBrowserWork->dlgBoxKind = BDLG_KIND_EXIT;
    if (gBrowserWork->endReason > 0 && gBrowserWork->netError == 0) {
        if (strcmp(gBrowserState->url, gPortalTopUrl) == 0) {
            Browser_ListCursor(2);
            switch (gBrowserSub->dlgCursor) {
            case 1:
            default:
                gBrowserWork->endGoOn = 1;
                break;
            case 2:
                gBrowserWork->endGoOn = 0;
                break;
            }
        } else {
            gBrowserWork->endGoOn = 1;
        }
    }
    if ((gBrowserWork->padPressed & PAD_CIRCLE) || gKbd->usbKey == USB_KEY_ENTER || gKbd->usbKey == USB_KEY_KP_ENTER) {
        LobbySe_Play(LOBBY_SE_CONFIRM_ALT);
        switch (gPortalPage) {
        case PORTAL_PAGE_TOP:
        case PORTAL_PAGE_NEXT:
            gBrowserWork->endReason = BROWSER_END_LEAVE;
            if (gBrowserTopMode == BROWSER_TOP_PORTAL) {
                n = 0;
                for (i = 0; i < PAGE_OBJ_COUNT; i++) {
                    if (gBrowserWork->obj[i] == NULL || gBrowserWork->obj[i]->active == 0) {
                        break;
                    }
                    if (gBrowserWork->obj[i]->kind == PAGE_OBJ_IMAGE && gBrowserWork->obj[i]->sub == PAGE_IMG_REQUESTED) {
                        n++;
                    }
                }
                if (n) {
                    gBrowserState->page = PAGE_LOAD_IMAGES;
                    Browser_SetStep(PAGE_STEP_GAME_MENU);
                    break;
                }
            }
            gPortalExit = PORTAL_EXIT_LEAVE;
            Browser_Fail(0);
            break;
        case PORTAL_PAGE_BACK:
            if (gBrowserState->page == PAGE_LOAD_IMAGES) {
                for (;;) {
                }
            }
            Browser_Fail(0);
            break;
        }
    }
}

/* Mode 8 of the dialog modes' table: nothing (seen: nothing sets it). */
void Dialog_NoStep(void)
{
}

/* The dialogs' step tables and the stick's steps (.data). */
void (*jtbl_007EEB78[3])(void) = {
    Dialog_Reset, Dialog_RunStep, Dialog_Stopped,
};
void (*jtbl_007EEB90[12])(void) = {
    Dialog_PageStep, Dialog_InputStep, Dialog_MenuStep, Dialog_ChoiceStep,
    Dialog_SaveLogin, Dialog_Register, Dialog_Ok, Dialog_Exit,
    Dialog_NoStep, NULL, NULL, NULL,
};
StickSteps gDialogMenuStickSteps = {
#include "lobby/007EEBC0.inc"
};
StickSteps gDialogChoiceStickSteps = {
#include "lobby/007EEBD0.inc"
};

/* The pages' keywords and the portal's top URL (.rodata). */
#pragma rodata on
char gPageCancelLink[32] = {
#include "lobby/00839110.inc"
};
char gPageFocusBackKey[16] = {
#include "lobby/00839130.inc"
};
char gPortalTopUrl[48] = {
#include "lobby/00839140.inc"
};
#pragma rodata off
