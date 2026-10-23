/*
 * The browser's input on a page: back and forward through the history;
 * scrolling (the pad, the right stick, the USB keyboard, the pointer on
 * the scroll bars and their thumbs); what the pointer is over and what a
 * click on the page does (links, text fields, buttons, drop-down lists,
 * radio buttons and check boxes); following a link (the portal's own
 * mmbb:// commands, an anchor in the page, or a URL); a form's reset and
 * submit (GET as a query, POST as form values); and the portal's data
 * pages, whose result posts the session's ids.
 *
 * The page's objects are gBrowserWork->obj[] (BrowserObj, by enum
 * PageObjKind); the page's position, size and style are gBrowserState, the
 * pointer and an open drop-down list gBrowserSub (include/browser.h).
 */
#include "common.h"
#include "browser.h"
#include "html.h"
#include "lobby_globals.h"
#include "kbd.h"
#include "pad.h"
#include "lobby_menu.h"
#include "lobby_servers.h"
#include "lobby_util.h"

/* Built at optimization level 4: Link_Follow compiles as the game's code
 * only so (the file's other functions compile the same at 3 and 4). */
#pragma optimization_level 4
int strncmp(const char *, const char *, unsigned int);

s32 strcmp(const char *a, const char *b);
s32 BrowserForm_Add(const char *name, char *value);
s32 strncmp(const char *a, const char *b, u32 n);
char *strcpy(char *dst, const char *src);

/* The right stick's 16 directions: a step across and down. Page_ScrollInput
 * copies both tables to the stack whole, as the game's code does, which a
 * struct of the 16 steps gives. */
typedef struct StickDirs {
    s8 v[16];
} StickDirs __attribute__((aligned(16)));
extern StickDirs gPageScrollDx;
extern StickDirs gPageScrollDy;


/* Link_Follow's cancel link (one of the portal's mmbb:// commands), and the
 * lobby's copy of the registration record (gRegRecord) that the OK button
 * keeps. */
extern u8 gLobbyPageData[];
u32 strlen(const char *s);
s32 memcmp(const char *a, const char *b, u32 n);
char *strstr(const char *s, const char *sub);
void *memcpy(void *dst, const void *src, u32 n);
void *memset(void *dst, s32 c, u32 n);
char *strcat(char *dst, const char *src);

/* Goes forward in the history: shows the page again when there is
 * nowhere to go, else opens the next page (the top page's mark cleared). */
void Browser_GoForward(void)
{
    char *url;

    if (BrowserHist_CanForward() == 0) {
        Browser_ShowPage();
        return;
    }
    url = BrowserHist_Forward();
    if (url == NULL) {
        Browser_ShowPage();
        return;
    }
    gBrowserState->fromTop = 0;
    Browser_Go(url);
    Browser_OpenPage();
}

/* Goes back in the history: shows the page again when there is nowhere
 * to go, else opens the page before. */
void Browser_GoBack(void)
{
    char *url;

    if (BrowserHist_CanBack() == 0) {
        Browser_ShowPage();
        return;
    }
    url = BrowserHist_Back();
    if (url == NULL) {
        Browser_ShowPage();
        return;
    }
    Browser_Go(url);
    Browser_OpenPage();
}

/* The portal's back: unless a style wait or a fade is running, closes
 * the dialogs and the drop-down list and opens the request history's
 * previous page (or shows the page again when there is none). */
void Portal_Back(void)
{
    char *url;

    if (BrowserReq_CanReload() == 0 || gBrowserState->styleWait != 0 || gBrowserState->fade != 0) {
        return;
    }
    url = BrowserReq_Reload();
    if (url == NULL) {
        Browser_ShowPage();
        return;
    }
    gBrowserWork->dlgMenuState = BDLG_MENU_HIDDEN;
    gBrowserSub->dlgMode = BDLG_MODE_PAGE;
    gBrowserSub->listCount = 0;
    Browser_Go(url);
    Browser_OpenPage();
}

/*
 * Scrolls a page that has a pointer (PAGE_STYLE_POINTER): by 20 when the
 * pointer, moved by the left stick or by the directions and arrow keys,
 * is at the view's edges; by the right stick (2 a step, 4 when fully
 * tilted; a drop-down list closes); up and down with L1 and R1, across
 * with L2 and R2; by 360 with Page Up and Page Down; to the top or the end
 * with Home and End.
 */
void Page_ScrollInput(void)
{
    StickDirs dx = gPageScrollDx;
    StickDirs dy = gPageScrollDy;
    s8 stepY;
    s8 stepX;
    u16 held;

    if (!(gBrowserState->style & PAGE_STYLE_POINTER)) {
        return;
    }
    if (gBrowserWork->stickTilt != 0) {
        if (gBrowserSub->cursorX <= gBrowserState->marginLeft) {
            Browser_ScrollBy(-20, 0);
        }
        if (gBrowserSub->cursorX >= 636 - gBrowserState->marginRight) {
            Browser_ScrollBy(20, 0);
        }
        if (gBrowserSub->cursorY <= gBrowserState->marginTop) {
            Browser_ScrollBy(0, -20);
        }
        if (gBrowserSub->cursorY >= 444 - gBrowserState->marginBottom) {
            Browser_ScrollBy(0, 20);
        }
    }
    held = gBrowserWork->padHeld;
    if ((held & PAD_UP) || (held & PAD_DOWN) || (held & PAD_LEFT) || (held & PAD_RIGHT) || (gKbdHeldKey >= USB_KEY_RIGHT && gKbdHeldKey <= USB_KEY_UP)) {
        if (!(gBrowserSub->cursorX >= gBrowserState->marginLeft)) {
            Browser_ScrollBy(-20, 0);
        }
        if (gBrowserSub->cursorX > 632 - gBrowserState->marginRight) {
            Browser_ScrollBy(20, 0);
        }
        if (!(gBrowserSub->cursorY >= gBrowserState->marginTop)) {
            Browser_ScrollBy(0, -20);
        }
        if (gBrowserSub->cursorY > 440 - gBrowserState->marginBottom) {
            Browser_ScrollBy(0, 20);
        }
    }
    stepX = stepY = 0;
    if (gBrowserWork->stick2Tilt != 0) {
        u8 dir = gBrowserWork->stick2Dir;

        stepX += dx.v[dir];
        stepY += dy.v[dir];
        gBrowserSub->activeObj = 0;
        if (gBrowserWork->stick2Tilt == BROWSER_STICK_FULL) {
            gBrowserState->x += stepX * 4;
            gBrowserState->y += stepY * 4;
        } else {
            gBrowserState->x += stepX * 2;
            gBrowserState->y += stepY * 2;
        }
    }
    if (gBrowserWork->padHeld & PAD_L1) {
        Browser_ScrollBy(0, -20);
    }
    if (gBrowserWork->padHeld & PAD_R1) {
        Browser_ScrollBy(0, 20);
    }
    if (gBrowserWork->padHeld & PAD_L2) {
        Browser_ScrollBy(-20, 0);
    }
    if (gBrowserWork->padHeld & PAD_R2) {
        Browser_ScrollBy(20, 0);
    }
    if (gKbd->usbKey == USB_KEY_PAGE_UP) {
        Browser_ScrollBy(0, -360);
    }
    if (gKbd->usbKey == USB_KEY_PAGE_DOWN) {
        Browser_ScrollBy(0, 360);
    }
    if (gKbd->usbKey == USB_KEY_HOME) {
        Browser_ScrollBy(0, -gBrowserState->pageH);
    }
    if (gKbd->usbKey == USB_KEY_END) {
        Browser_ScrollBy(0, gBrowserState->pageH);
    }
}

/*
 * Drags a scroll bar's thumb with the pointer while circle is held. An
 * open drop-down list of more than 10 options has its own bar at its
 * right edge (above the field when the list opens upward): the press on
 * the thumb notes where it was taken (dragX, dragY, dragOfs), and the drag
 * moves the thumb and the first option shown (listTop). Otherwise the
 * page's bars, the one right of the view and the one below it: the drag
 * moves the thumb and the page with it, the thumbs kept on their tracks.
 */
void Page_DragScroll(void)
{
    BrowserObj *select;
    f32 top;
    f32 right;
    f32 left;
    s16 x;
    s32 limit;
    u8 optionCount;

    optionCount = gBrowserSub->listCount;
    if (optionCount) {
        select = gBrowserWork->obj[gBrowserSub->activeObj];
        top = select->y - (f32)gBrowserState->y;
        left = select->x - (f32)gBrowserState->x;
        right = left + select->w;
        if (optionCount > 10) {
            if (gBrowserWork->padPrevHeld & PAD_CIRCLE) {
                if (gBrowserSub->dragOfs != 0 && (f32)gBrowserSub->cursorX >= right - 16.0f &&
                    (f32)gBrowserSub->cursorX <= right && gBrowserSub->cursorY != gBrowserSub->dragY) {
                    if (gBrowserSub->listUp != 0) {
                        if ((f32)gBrowserSub->cursorY < top - 180.0f + gBrowserSub->thumbTop) {
                            return;
                        }
                        if (!((f32)gBrowserSub->cursorY <= top - 180.0f + gBrowserSub->thumbBottom)) {
                            return;
                        }
                    } else {
                        if ((f32)gBrowserSub->cursorY < top + 18.0f + gBrowserSub->thumbTop) {
                            return;
                        }
                        if (!((f32)gBrowserSub->cursorY <= top + 18.0f + gBrowserSub->thumbBottom)) {
                            return;
                        }
                    }
                    gBrowserSub->thumbTop = gBrowserSub->cursorY - gBrowserSub->dragOfs;
                    if (gBrowserSub->thumbTop < 18) {
                        gBrowserSub->listTop = 1;
                        gBrowserSub->thumbTop = 18;
                        gBrowserSub->thumbBottom = gBrowserSub->thumbLen + 18;
                    } else {
                        gBrowserSub->listTop = gBrowserSub->listCount * (gBrowserSub->thumbTop - 18) / 144 + 1;
                        if (gBrowserSub->listTop <= 0) {
                            gBrowserSub->listTop = 1;
                        }
                    }
                    return;
                }
            } else {
                x = gBrowserSub->cursorX;
                if ((f32)x >= right - 16.0f && (f32)x <= right) {
                    if (gBrowserSub->listUp != 0) {
                        if ((f32)gBrowserSub->cursorY < top - 180.0f + gBrowserSub->thumbTop) {
                            return;
                        }
                        if (!((f32)gBrowserSub->cursorY <= top - 180.0f + gBrowserSub->thumbBottom)) {
                            return;
                        }
                    } else {
                        if ((f32)gBrowserSub->cursorY < 18.0f + top + gBrowserSub->thumbTop) {
                            return;
                        }
                        if (!((f32)gBrowserSub->cursorY <= 18.0f + top + gBrowserSub->thumbBottom)) {
                            return;
                        }
                    }
                    gBrowserSub->dragX = x;
                    gBrowserSub->dragY = gBrowserSub->cursorY;
                    gBrowserSub->dragOfs = gBrowserSub->cursorY - gBrowserSub->thumbTop;
                }
            }
        }
    } else {
        if (gBrowserWork->padPrevHeld & PAD_CIRCLE) {
            if (gBrowserSub->cursorX > PAGE_VIEW_W && gBrowserSub->cursorX < 640 && gBrowserSub->cursorY > 16 &&
                gBrowserSub->cursorY < PAGE_VIEW_H && gBrowserSub->dragX > PAGE_VIEW_W) {
                if (gBrowserSub->cursorY == gBrowserSub->dragY) {
                    return;
                }
                gBrowserState->vThumbY = gBrowserSub->cursorY - gBrowserSub->dragOfs;
                gBrowserState->y = gBrowserState->pageH * (gBrowserState->vThumbY - 16) / 348;
            }
            if (gBrowserSub->cursorX > 16 && gBrowserSub->cursorX < 588 && gBrowserSub->cursorY > PAGE_VIEW_H &&
                gBrowserSub->dragY > PAGE_VIEW_H) {
                if (gBrowserSub->cursorX == gBrowserSub->dragX) {
                    return;
                }
                gBrowserState->hThumbX = gBrowserSub->cursorX - gBrowserSub->dragOfs;
                gBrowserState->x = gBrowserState->pageW * (gBrowserState->hThumbX - 16) / 552;
            }
        } else {
            if (gBrowserSub->cursorX > PAGE_VIEW_W && gBrowserSub->cursorX < 640) {
                gBrowserSub->dragX = gBrowserSub->cursorX;
                gBrowserSub->dragY = gBrowserSub->cursorY;
                gBrowserSub->dragOfs = gBrowserSub->cursorY - gBrowserState->vThumbY;
            }
            if (gBrowserSub->cursorY > PAGE_VIEW_H && gBrowserSub->cursorY < 448) {
                gBrowserSub->dragX = gBrowserSub->cursorX;
                gBrowserSub->dragY = gBrowserSub->cursorY;
                gBrowserSub->dragOfs = gBrowserSub->cursorX - gBrowserState->hThumbX;
            }
        }
        if (gBrowserSub->cursorX < gBrowserState->marginLeft + PAGE_VIEW_W && gBrowserSub->cursorY < gBrowserState->marginTop + PAGE_VIEW_H) {
            return;
        }
        limit = PAGE_VIEW_H - gBrowserState->vThumbLen;
        if (limit < gBrowserState->vThumbY) {
            gBrowserState->vThumbY = limit;
            gBrowserState->y = gBrowserState->pageH - PAGE_VIEW_H;
        }
        if (gBrowserState->vThumbY < 16) {
            gBrowserState->vThumbY = 16;
            gBrowserState->y = 0;
        }
        limit = 588 - gBrowserState->hThumbLen;
        if (limit < gBrowserState->hThumbX) {
            gBrowserState->hThumbX = limit;
            gBrowserState->x = gBrowserState->pageW - PAGE_VIEW_W;
        }
        if (gBrowserState->hThumbX < 16) {
            gBrowserState->hThumbX = 16;
            gBrowserState->x = 0;
        }
    }
}

/* A click of the pointer on the page's scroll bars (no drop-down list
 * open): an arrow scrolls by 20 (Browser_Move, the arrow drawn pressed),
 * the track before or after the thumb by a page (359 down, 512 across),
 * and the thumb is placed again. */
void Page_PointerScroll(void)
{
    if (gBrowserSub->listCount != 0) {
        return;
    }
    if (gBrowserSub->cursorX > gBrowserState->marginLeft + PAGE_VIEW_W && 640 - gBrowserState->marginRight > gBrowserSub->cursorX) {
        if (gBrowserState->pageH == 0) {
            return;
        }
        if (gBrowserSub->cursorY >= gBrowserState->marginTop && gBrowserSub->cursorY < gBrowserState->marginTop + 16) {
            Browser_Move(0, -20, BMOVE_UP);
            gBrowserWork->dlgVScrollState = BDLG_SCROLL_FIRST_PRESSED;
            return;
        }
        if (gBrowserSub->cursorY > PAGE_VIEW_H && gBrowserState->marginTop + PAGE_VIEW_H > gBrowserSub->cursorY) {
            Browser_Move(0, 20, BMOVE_DOWN);
            gBrowserWork->dlgVScrollState = BDLG_SCROLL_SECOND_PRESSED;
            return;
        }
        if (gBrowserSub->cursorY > gBrowserState->marginTop + 16 && gBrowserState->marginTop + PAGE_VIEW_H > gBrowserSub->cursorY) {
            if (gBrowserState->vThumbY > gBrowserSub->cursorY) {
                gBrowserState->y -= 359;
            }
            if (gBrowserSub->cursorY > gBrowserState->vThumbY + gBrowserState->vThumbLen) {
                gBrowserState->y += 359;
            }
            gBrowserState->vThumbY = gBrowserState->y * 348 / gBrowserState->pageH + 16;
        }
    }
    if (gBrowserSub->cursorY > gBrowserState->marginTop + PAGE_VIEW_H && 428 - gBrowserState->marginBottom > gBrowserSub->cursorY &&
        gBrowserState->pageW != 0) {
        if (gBrowserSub->cursorX >= gBrowserState->marginRight && gBrowserSub->cursorX < gBrowserState->marginRight + 16) {
            Browser_Move(-20, 0, BMOVE_LEFT);
            gBrowserWork->dlgHScrollState = BDLG_SCROLL_FIRST_PRESSED;
            return;
        }
        if (gBrowserSub->cursorX > 588 && gBrowserState->marginRight + PAGE_VIEW_W > gBrowserSub->cursorX) {
            Browser_Move(20, 0, BMOVE_RIGHT);
            gBrowserWork->dlgHScrollState = BDLG_SCROLL_SECOND_PRESSED;
            return;
        }
        if (gBrowserSub->cursorX > gBrowserState->marginRight + 16 && gBrowserState->marginRight + 588 > gBrowserSub->cursorX) {
            if (gBrowserState->hThumbX > gBrowserSub->cursorX) {
                gBrowserState->x -= 512;
            }
            if (gBrowserSub->cursorX > gBrowserState->hThumbX + gBrowserState->hThumbLen) {
                gBrowserState->x += 512;
            }
            gBrowserState->hThumbX = gBrowserState->x * 552 / gBrowserState->pageW + 16;
        }
    }
}

/* Sets the focus (BrowserWork.focus) to the object under the pointer: a
 * link with a target, an image map or image button, a text or password
 * field. Over the scroll bars nothing changes; over nothing, the status
 * line shows the page's title. */
void Page_PointAt(void)
{
    s32 i;
    BrowserObj *item;
    u8 drawKind;
    f32 x;
    f32 y;
    f32 pointerX;
    f32 pointerY;

    if (((gBrowserState->style & PAGE_STYLE_STATUS_BAR) >> 2) != 0) {
        gBrowserWork->dlgStatusState = BDLG_STATUS_TITLE;
    }
    if (((gBrowserState->style & PAGE_STYLE_VSCROLL) >> 1) != 0) {
        if (gBrowserSub->cursorX > 624 - gBrowserState->marginRight) {
            return;
        }
        if (((gBrowserState->style & PAGE_STYLE_STATUS_BAR) >> 2) != 0) {
            if (412 - gBrowserState->marginBottom < gBrowserSub->cursorY && gBrowserSub->cursorY < 428) {
                return;
            }
        } else if (gBrowserSub->cursorY > 432 - gBrowserState->marginBottom && gBrowserSub->cursorY < 448 - gBrowserState->marginBottom) {
            return;
        }
    }
    for (i = 0; i < PAGE_OBJ_COUNT; i++) {
        item = gBrowserWork->obj[i];
        if (item == NULL) {
            break;
        }
        if ((drawKind = item->active) == 0) {
            continue;
        }
        x = item->x - (f32)gBrowserState->x;
        pointerX = gBrowserSub->cursorX;
        if (pointerX < x) {
            continue;
        }
        if (!(pointerX <= item->w + x)) {
            continue;
        }
        y = item->y - (f32)gBrowserState->y;
        pointerY = gBrowserSub->cursorY;
        if (pointerY <= y) {
            continue;
        }
        if (!(pointerY < item->h + y)) {
            continue;
        }
        switch (item->kind) {
        case PAGE_OBJ_LINK:
        case PAGE_OBJ_STRUCK:
        case PAGE_OBJ_UNDERLINED:
            if ((u8)*item->s1 != 0) {
                gBrowserWork->focus = i;
                return;
            }
            break;
        case PAGE_OBJ_IMAGE:
            if (drawKind == PAGE_IMG_MAP || drawKind == PAGE_IMG_INPUT) {
                if (item->sub != 0) {
                    gBrowserWork->focus = i;
                    return;
                }
            }
            break;
        case PAGE_OBJ_TEXT_INPUT:
        case PAGE_OBJ_PASSWORD:
            gBrowserWork->focus = i;
            return;
        }
    }
    gBrowserWork->dlgStatusState = BDLG_STATUS_TITLE;
}

/* Keeps the scroll position within the page (the view is PAGE_VIEW_W by
 * PAGE_VIEW_H) and the pointer within the screen's margins. */
void Page_Clamp(void)
{
    s32 lim;

    if (gBrowserState->y < 0) {
        gBrowserState->y = 0;
    }
    lim = gBrowserState->pageH - PAGE_VIEW_H;
    if (gBrowserState->y > lim) {
        gBrowserState->y = lim;
    }
    if (gBrowserState->x < 0) {
        gBrowserState->x = 0;
    }
    lim = gBrowserState->pageW - PAGE_VIEW_W;
    if (gBrowserState->x > lim) {
        gBrowserState->x = lim;
    }
    if (gBrowserSub->cursorY < gBrowserState->marginTop) {
        gBrowserSub->cursorY = gBrowserState->marginTop;
    }
    if (gBrowserSub->cursorY > 438 - gBrowserState->marginBottom) {
        gBrowserSub->cursorY = 438 - gBrowserState->marginBottom;
    }
    if (gBrowserSub->cursorX < gBrowserState->marginLeft) {
        gBrowserSub->cursorX = gBrowserState->marginLeft;
    }
    if (gBrowserSub->cursorX > 630 - gBrowserState->marginRight) {
        gBrowserSub->cursorX = 630 - gBrowserState->marginRight;
    }
}

/* Returns 1 when nothing moves the page: no Enter or arrow key, no
 * direction held or pressed, the left stick at rest; else -1. */
s32 Browser_Idle(void)
{
    KbdEntry *kbd = gKbd;
    s32 key;
    u16 buttons;

    key = kbd->usbPrevKey;
    if (key == USB_KEY_ENTER || key == USB_KEY_KP_ENTER) {
        return -1;
    }
    if (key == USB_KEY_UP || key == USB_KEY_RIGHT || key == USB_KEY_LEFT || key == USB_KEY_DOWN) {
        return -1;
    }
    key = kbd->usbKey;
    if (key == USB_KEY_UP || key == USB_KEY_RIGHT || key == USB_KEY_LEFT || key == USB_KEY_DOWN) {
        return -1;
    }
    buttons = gBrowserWork->padHeld;
    if ((buttons & PAD_LEFT) || (buttons & PAD_UP) || (buttons & PAD_RIGHT) || (buttons & PAD_DOWN)) {
        return -1;
    }
    buttons = gBrowserWork->padPressed;
    if ((buttons & PAD_LEFT) || (buttons & PAD_UP) || (buttons & PAD_RIGHT) || (buttons & PAD_DOWN)) {
        return -1;
    }
    if (gBrowserWork->stickTilt != 0) {
        return -1;
    }
    return 1;
}

/*
 * A click of the pointer on the page. While a drop-down list is open
 * (gBrowserSub->activeObj), the click goes to Select_ListClick. Otherwise
 * the object under the pointer acts by its kind:
 * - a text or password field opens the software keyboard on its text
 *   (login field 4 takes its layout from the two digits that begin its
 *   name);
 * - a link, or an image map or image button, is followed or submits its
 *   form; a submit or reset button acts on its form;
 * - the portal's own buttons: 0 goes to the portal's top page, 1 leaves;
 * - a drop-down list opens (10 rows at most, above the field when they
 *   wouldn't fit below), or closes when it was open;
 * - a radio button clears the others of its name; a check box toggles.
 * A click outside the view closes the list.
 *
 * The hit test reads gBrowserWork, gBrowserState and gBrowserSub through
 * the read-only locals work, state and subState, and the stores go through
 * the globals: the compiler gives a const local a register of its own (a
 * plain copy of a global would be folded away), and declared in this
 * order, before the object, the four get the registers the game's code
 * uses.
 */
void Page_Click(void)
{
    const BrowserWork *work;
    const BrowserState *state;
    const BrowserSub *subState;
    BrowserObj *item;
    s32 i;
    s32 j;
    s32 k;
    s32 layout;
    u8 drawKind;
    f32 left;
    f32 top;
    f32 right;
    u8 *name;

    if ((s8)Browser_Idle() < 0) {
        return;
    }
    if (gBrowserSub->cursorY > 428 - gBrowserState->marginBottom) {
        return;
    }
    if (gBrowserSub->cursorX < gBrowserState->marginLeft ||
        gBrowserSub->cursorY < gBrowserState->marginTop || gBrowserSub->cursorX > gBrowserState->marginLeft + PAGE_VIEW_W ||
        gBrowserSub->cursorY > gBrowserState->marginTop + PAGE_VIEW_H) {
        gBrowserSub->activeObj = 0;
        gBrowserSub->listCount = 0;
        return;
    }
    for (i = 0; i < PAGE_OBJ_COUNT; i++) {
        work = gBrowserWork;
        item = work->obj[i];
        if (item == NULL) {
            break;
        }
        if ((drawKind = item->active) == 0) {
            continue;
        }
        state = gBrowserState;
        subState = gBrowserSub;
        left = item->x - (f32)state->x;
        if ((f32)subState->cursorX < left || !((f32)subState->cursorX <= item->w + left)) {
            continue;
        }
        if (subState->activeObj != 0) {
            if (work->obj[subState->activeObj]->kind == PAGE_OBJ_SELECT) {
                gBrowserSub->activeObj = Select_ListClick(subState->activeObj);
            }
            return;
        }
        top = item->y - (f32)state->y;
        if ((f32)subState->cursorY <= top || !((f32)subState->cursorY < item->h + top)) {
            continue;
        }
        if (state->styleWait == 0 && state->fade == 0) {
            switch (item->kind) {
            case PAGE_OBJ_TEXT_INPUT:
            case PAGE_OBJ_PASSWORD:
            case PAGE_OBJ_SELECT:
            case PAGE_OBJ_RADIO:
            case PAGE_OBJ_CHECKBOX:
                break;
            case PAGE_OBJ_LINK:
            case PAGE_OBJ_STRUCK:
            case PAGE_OBJ_UNDERLINED:
                if (*(u8 *)item->s1 == 0) {
                    break;
                }
                /* a link with a target sounds as a button does */
            case PAGE_OBJ_SUBMIT:
            case PAGE_OBJ_RESET:
            case PAGE_OBJ_BUTTON:
                LobbySe_Play(LOBBY_SE_CONFIRM_ALT);
                break;
            case PAGE_OBJ_IMAGE:
                if (drawKind == PAGE_IMG_MAP || drawKind == PAGE_IMG_INPUT) {
                    LobbySe_Play(LOBBY_SE_CONFIRM_ALT);
                }
                break;
            }
        }
        item = gBrowserWork->obj[i];
        switch (item->kind) {
        case PAGE_OBJ_TEXT_INPUT:
        case PAGE_OBJ_PASSWORD:
            gBrowserWork->dlgInputState = BDLG_INPUT_OPEN;
            LobbySe_Play(LOBBY_SE_CONFIRM_ALT);
            gBrowserSub->activeObj = i;
            gBrowserWork->focus = i;
            switch (gBrowserWork->obj[i]->kind) {
            case PAGE_OBJ_TEXT_INPUT:
                /* the keyboard's layouts (Kbd_Open's): 1 for a text field; a
                 * login field 4's name begins with two digits that choose 2, 3
                 * or 4; a password field takes 5 */
                if (gBrowserWork->obj[i]->field != 4) {
                    layout = 1;
                } else {
                    name = (u8 *)gBrowserWork->obj[i]->s1;
                    switch ((name[0] - '0') * 10 + (name[1] - '0')) {
                    case 0:
                    case 2:
                    case 3:
                    case 4:
                    case 8:
                    case 9:
                        layout = 4;
                        break;
                    case 1:
                    case 5:
                    case 6:
                        layout = 3;
                        break;
                    case 7:
                        layout = 2;
                        break;
                    }
                }
                Browser_OpenKeyboard(gBrowserWork->obj[i]->s2, gBrowserWork->obj[i]->sub, layout);
                break;
            case PAGE_OBJ_PASSWORD:
                Browser_OpenKeyboard(gBrowserWork->obj[i]->s2, gBrowserWork->obj[i]->sub, 5);
                break;
            }
            gBrowserWork->dlgInputState = BDLG_INPUT_OPEN;
            gBrowserSub->dlgMode = BDLG_MODE_INPUT;
            return;
        case PAGE_OBJ_IMAGE:
            if (item->active == PAGE_IMG_IMAGE) {
                continue;
            }
            if (gBrowserWork->obj[i]->active == PAGE_IMG_MAP) {
                Link_Follow(i);
            }
            if (gBrowserWork->obj[i]->active == PAGE_IMG_INPUT) {
                Form_Submit(i);
            }
            return;
        case PAGE_OBJ_LINK:
        case PAGE_OBJ_STRUCK:
        case PAGE_OBJ_UNDERLINED:
            if (*(u8 *)item->s1 == 0) {
                return;
            }
            Link_Follow(i);
            continue;
        case PAGE_OBJ_SUBMIT:
            Form_Submit(i);
            continue;
        case PAGE_OBJ_RESET:
            gBrowserWork->obj[i]->sub = 1;
            gBrowserWork->pressTimer = BROWSER_PRESS_FRAMES;
            Form_Reset(i);
            return;
        case PAGE_OBJ_BUTTON:
            switch (gBrowserWork->obj[i]->field) {
            case 0:
                Browser_Go(gPortalTopUrl);
                gBrowserSub->dlgMode = BDLG_MODE_PAGE;
                gBrowserSub->activeObj = 0;
                gBrowserWork->obj[i]->sub = 1;
                gBrowserWork->pressTimer = BROWSER_PRESS_FRAMES;
                if (gBrowserState->page == PAGE_LOAD_IMAGES) {
                    Browser_SetStep(PAGE_STEP_RELOAD);
                    return;
                }
                Browser_Reload();
                break;
            case 1:
                Browser_Fail(PAGE_LOAD_NONE);
                break;
            }
            return;
        case PAGE_OBJ_SELECT:
            LobbySe_Play(LOBBY_SE_CONFIRM_ALT);
            if (gBrowserSub->activeObj == i) {
                gBrowserSub->activeObj = 0;
                gBrowserSub->dlgCursor = 0;
            } else {
                gBrowserSub->activeObj = i;
                gBrowserSub->dlgCursor = gBrowserWork->obj[i]->sub;
                gBrowserSub->listCount = 0;
                for (j = gBrowserSub->activeObj + 1; j < PAGE_OBJ_COUNT; j++) {
                    item = gBrowserWork->obj[j];
                    if (item->kind != PAGE_OBJ_SELECT || item->tex == 0) {
                        break;
                    }
                    gBrowserSub->listCount++;
                }
                if (gBrowserSub->listCount < DROPLIST_ROWS) {
                    gBrowserSub->listTop = 1;
                } else if (gBrowserSub->listCount - gBrowserSub->dlgCursor >= DROPLIST_PAGE) {
                    for (gBrowserSub->listTop = 1; gBrowserSub->listTop < PAGE_OBJ_COUNT; gBrowserSub->listTop++) {
                        if (gBrowserWork->obj[gBrowserSub->activeObj + gBrowserSub->listTop]->tex == gBrowserSub->dlgCursor) {
                            break;
                        }
                    }
                } else {
                    gBrowserSub->listTop = gBrowserSub->listCount - DROPLIST_PAGE;
                }
                if (gBrowserSub->listCount < DROPLIST_ROWS) {
                    if (gBrowserSub->listCount == 0) {
                        Select_Close();
                        return;
                    }
                    if (180.0f + (gBrowserWork->obj[gBrowserSub->activeObj]->y - (f32)gBrowserState->y) > PAGE_VIEW_H) {
                        gBrowserSub->listUp = 1;
                        gBrowserSub->cursorY = (gBrowserWork->obj[gBrowserSub->activeObj]->y - (f32)gBrowserState->y) - 9.0f;
                    } else {
                        gBrowserSub->listUp = 0;
                        gBrowserSub->cursorY = 9.0f + (20.0f + (gBrowserWork->obj[gBrowserSub->activeObj]->y - (f32)gBrowserState->y));
                    }
                    gBrowserSub->cursorX = 10.0f + (gBrowserWork->obj[gBrowserSub->activeObj]->x - (f32)gBrowserState->x);
                } else {
                    if (180.0f + (gBrowserWork->obj[gBrowserSub->activeObj]->y - (f32)gBrowserState->y) > PAGE_VIEW_H) {
                        gBrowserSub->listUp = 1;
                        gBrowserSub->cursorY = (gBrowserWork->obj[gBrowserSub->activeObj]->y - (f32)gBrowserState->y) - 9.0f;
                    } else {
                        gBrowserSub->listUp = 0;
                        gBrowserSub->cursorY = 9.0f + (20.0f + (gBrowserWork->obj[gBrowserSub->activeObj]->y - (f32)gBrowserState->y));
                    }
                    gBrowserSub->cursorX = 10.0f + (gBrowserWork->obj[gBrowserSub->activeObj]->x - (f32)gBrowserState->x);
                }
            }
            item = gBrowserWork->obj[i];
            right = item->w + (item->x - (f32)gBrowserState->x);
            if ((f32)gBrowserSub->cursorX > right - 16.0f && (f32)gBrowserSub->cursorX < right) {
                item->active = PAGE_SELECT_PRESSED;
                gBrowserWork->pressTimer = BROWSER_PRESS_FRAMES;
            }
            return;
        case PAGE_OBJ_RADIO:
            LobbySe_Play(LOBBY_SE_CONFIRM_ALT);
            for (k = i + 1; k < PAGE_OBJ_COUNT; k++) {
                if (gBrowserWork->obj[k] == NULL || gBrowserWork->obj[k]->kind == PAGE_OBJ_FORM) {
                    break;
                }
                if (gBrowserWork->obj[k]->kind == PAGE_OBJ_RADIO && strcmp(gBrowserWork->obj[k]->s1, gBrowserWork->obj[i]->s1) == 0) {
                    gBrowserWork->obj[k]->sub = 0;
                }
            }
            for (j = i - 1; j > 0; j--) {
                if (gBrowserWork->obj[j] == NULL || gBrowserWork->obj[j]->kind == PAGE_OBJ_FORM) {
                    break;
                }
                if (gBrowserWork->obj[j]->kind == PAGE_OBJ_RADIO && strcmp(gBrowserWork->obj[j]->s1, gBrowserWork->obj[i]->s1) == 0) {
                    gBrowserWork->obj[j]->sub = 0;
                }
            }
            gBrowserWork->obj[i]->sub = 1;
            return;
        case PAGE_OBJ_CHECKBOX:
            LobbySe_Play(LOBBY_SE_CONFIRM_ALT);
            if (gBrowserWork->obj[i]->sub == 1) {
                gBrowserWork->obj[i]->sub = 0;
            } else {
                gBrowserWork->obj[i]->sub = 1;
            }
            return;
        default:
            return;
        }
    }
    if (i == PAGE_OBJ_COUNT) {
        gBrowserSub->activeObj = 0;
    }
}


/* A click while drop-down list i (obj[i]) is open. A click on the list's
 * scroll bar (its arrows, its track or its thumb) keeps the list open and
 * returns i. A click on an option chooses it: its text and index go to the
 * list's field, the list closes and 0 is returned. A click outside the
 * list closes it (0). */
s32 Select_ListClick(s32 i)
{
    BrowserObj *select;
    f32 left;
    f32 right;
    f32 fieldTop;
    f32 pointerX;
    f32 rowsTop;
    s32 row;
    s32 option;

    select = gBrowserWork->obj[i];
    left = select->x - (f32)gBrowserState->x;
    fieldTop = select->y - (f32)gBrowserState->y;
    right = left + select->w;
    pointerX = (f32)gBrowserSub->cursorX;
    if (pointerX <= left || pointerX >= right) {
        gBrowserSub->listCount = 0;
        return 0;
    }
    if (gBrowserSub->listCount > DROPLIST_ROWS && pointerX > right - 16.0f && pointerX < right) {
        if (gBrowserSub->listUp != 0) {
            if ((f32)gBrowserSub->cursorY < fieldTop - 162.0f && (f32)gBrowserSub->cursorY > fieldTop - 180.0f) {
                if (gBrowserSub->listTop > 1) {
                    Browser_Move(0, -1, BMOVE_LIST_UP);
                } else {
                    Browser_Move(0, 0, BMOVE_LIST_UP);
                }
                return i;
            }
            if ((f32)gBrowserSub->cursorY >= fieldTop - 162.0f &&
                (f32)gBrowserSub->cursorY <= fieldTop + gBrowserSub->thumbTop - 180.0f) {
                if (gBrowserSub->listTop - DROPLIST_PAGE <= 0) {
                    gBrowserSub->listTop = 1;
                } else {
                    gBrowserSub->listTop -= DROPLIST_PAGE;
                }
                gBrowserSub->thumbTop = (gBrowserSub->listTop - 1) * DROPLIST_TRACK_H / gBrowserSub->listCount + 18;
                gBrowserSub->thumbBottom = (DROPLIST_TRACK_H * DROPLIST_ROWS) / gBrowserSub->listCount + gBrowserSub->thumbTop;
                gBrowserSub->thumbLen = gBrowserSub->thumbBottom - gBrowserSub->thumbTop;
                gBrowserSub->dragOfs = gBrowserSub->cursorY - gBrowserSub->thumbTop;
                return i;
            }
            if ((f32)gBrowserSub->cursorY > fieldTop + gBrowserSub->thumbTop - 180.0f &&
                (f32)gBrowserSub->cursorY < fieldTop + gBrowserSub->thumbBottom - 180.0f) {
                return i;
            }
            if ((f32)gBrowserSub->cursorY >= fieldTop + gBrowserSub->thumbBottom - 180.0f && (f32)gBrowserSub->cursorY <= fieldTop - 20.0f) {
                gBrowserSub->listTop += DROPLIST_PAGE;
                if (gBrowserSub->listCount < gBrowserSub->listTop + DROPLIST_ROWS) {
                    gBrowserSub->listTop = gBrowserSub->listCount - DROPLIST_PAGE;
                }
                gBrowserSub->thumbTop = (gBrowserSub->listTop - 1) * DROPLIST_TRACK_H / gBrowserSub->listCount + 18;
                gBrowserSub->thumbBottom = (DROPLIST_TRACK_H * DROPLIST_ROWS) / gBrowserSub->listCount + gBrowserSub->thumbTop;
                gBrowserSub->thumbLen = gBrowserSub->thumbBottom - gBrowserSub->thumbTop;
                gBrowserSub->dragOfs = gBrowserSub->cursorY - gBrowserSub->thumbTop;
                return i;
            }
            if ((f32)gBrowserSub->cursorY < fieldTop - 1.0f && (f32)gBrowserSub->cursorY > fieldTop - 20.0f) {
                if (gBrowserSub->listCount - gBrowserSub->listTop >= DROPLIST_ROWS) {
                    Browser_Move(0, 1, BMOVE_LIST_DOWN);
                } else {
                    Browser_Move(0, 0, BMOVE_LIST_DOWN);
                }
                return i;
            }
        } else {
            rowsTop = 20.0f + fieldTop;
            if ((f32)gBrowserSub->cursorY > rowsTop && (f32)gBrowserSub->cursorY < 20.0f + rowsTop) {
                if (gBrowserSub->listTop > 1) {
                    Browser_Move(0, -1, BMOVE_LIST_UP);
                } else {
                    Browser_Move(0, 0, BMOVE_LIST_UP);
                }
                return i;
            }
            if ((f32)gBrowserSub->cursorY >= 20.0f + rowsTop && (f32)gBrowserSub->cursorY <= rowsTop + gBrowserSub->thumbTop) {
                if (gBrowserSub->listTop - DROPLIST_PAGE <= 0) {
                    gBrowserSub->listTop = 1;
                } else {
                    gBrowserSub->listTop -= DROPLIST_PAGE;
                }
                gBrowserSub->thumbTop = (gBrowserSub->listTop - 1) * DROPLIST_TRACK_H / gBrowserSub->listCount + 18;
                gBrowserSub->thumbBottom = (DROPLIST_TRACK_H * DROPLIST_ROWS) / gBrowserSub->listCount + gBrowserSub->thumbTop;
                gBrowserSub->thumbLen = gBrowserSub->thumbBottom - gBrowserSub->thumbTop;
                gBrowserSub->dragOfs = gBrowserSub->cursorY - gBrowserSub->thumbTop;
                return i;
            }
            if ((f32)gBrowserSub->cursorY > rowsTop + gBrowserSub->thumbTop && (f32)gBrowserSub->cursorY < rowsTop + gBrowserSub->thumbBottom) {
                return i;
            }
            if ((f32)gBrowserSub->cursorY >= rowsTop + gBrowserSub->thumbBottom && (f32)gBrowserSub->cursorY <= 180.0f + rowsTop - 18.0f) {
                gBrowserSub->listTop += DROPLIST_PAGE;
                if (gBrowserSub->listCount < gBrowserSub->listTop + DROPLIST_ROWS) {
                    gBrowserSub->listTop = gBrowserSub->listCount - DROPLIST_PAGE;
                }
                gBrowserSub->thumbTop = (gBrowserSub->listTop - 1) * DROPLIST_TRACK_H / gBrowserSub->listCount + 18;
                gBrowserSub->thumbBottom = (DROPLIST_TRACK_H * DROPLIST_ROWS) / gBrowserSub->listCount + gBrowserSub->thumbTop;
                gBrowserSub->thumbLen = gBrowserSub->thumbBottom - gBrowserSub->thumbTop;
                gBrowserSub->dragOfs = gBrowserSub->cursorY - gBrowserSub->thumbTop;
                return i;
            }
            if ((f32)gBrowserSub->cursorY > 162.0f + rowsTop && (f32)gBrowserSub->cursorY < 180.0f + rowsTop) {
                if (gBrowserSub->listCount - gBrowserSub->listTop >= DROPLIST_ROWS) {
                    Browser_Move(0, 1, BMOVE_LIST_DOWN);
                } else {
                    Browser_Move(0, 0, BMOVE_LIST_DOWN);
                }
                return i;
            }
        }
    }
    if (gBrowserSub->listUp != 0) {
        if (gBrowserSub->listCount < DROPLIST_ROWS) {
            for (row = 0; row <= gBrowserSub->listCount; row++) {
                if ((f32)gBrowserSub->cursorY < 20.0f + fieldTop - row * 18 &&
                    (f32)gBrowserSub->cursorY >= 20.0f + fieldTop - (row + 1) * 18) {
                    break;
                }
            }
            if (row == 0 || row > gBrowserSub->listCount) {
                return 0;
            }
            option = gBrowserSub->listCount - row;
        } else {
            for (row = 0; row <= DROPLIST_ROWS; row++) {
                if ((f32)gBrowserSub->cursorY < 20.0f + fieldTop - row * 18 &&
                    (f32)gBrowserSub->cursorY >= 20.0f + fieldTop - (row + 1) * 18) {
                    break;
                }
            }
            if (gBrowserSub->listCount - gBrowserSub->dlgCursor >= DROPLIST_PAGE) {
                if (row == 0 || row > DROPLIST_ROWS) {
                    return 0;
                }
            } else if ((row == 0 || row >= DROPLIST_ROWS) && gBrowserSub->listCount < DROPLIST_ROWS && gBrowserSub->listCount - gBrowserSub->listTop < DROPLIST_PAGE) {
                return 0;
            }
            option = DROPLIST_ROWS - row;
        }
        LobbySe_Play(LOBBY_SE_CONFIRM_ALT);
        gBrowserSub->dlgCursor = i + gBrowserSub->listTop + option;
        gBrowserWork->obj[i]->s1 = gBrowserWork->obj[i + gBrowserSub->listTop + option]->s1;
        gBrowserWork->obj[i]->sub = gBrowserWork->obj[i + gBrowserSub->listTop + option]->tex;
        gBrowserSub->cursorY = 10.0f + (gBrowserWork->obj[gBrowserSub->activeObj]->y - (f32)gBrowserState->y);
    } else {
        if (gBrowserSub->listCount < DROPLIST_ROWS) {
            for (option = 0; option <= gBrowserSub->listCount; option++) {
                if ((f32)gBrowserSub->cursorY >= 20.0f + fieldTop + option * 18 &&
                    (f32)gBrowserSub->cursorY < 20.0f + fieldTop + (option + 1) * 18) {
                    break;
                }
            }
            if (option < 0 || option > gBrowserSub->listCount) {
                return 0;
            }
        } else {
            for (option = 0; option <= DROPLIST_ROWS; option++) {
                if ((f32)gBrowserSub->cursorY >= 20.0f + fieldTop + option * 18 &&
                    (f32)gBrowserSub->cursorY < 20.0f + fieldTop + (option + 1) * 18) {
                    break;
                }
            }
            if (gBrowserSub->listCount - gBrowserSub->dlgCursor >= DROPLIST_PAGE) {
                if (option < 0 || option > DROPLIST_ROWS) {
                    return 0;
                }
            } else if (option < 0 || option >= DROPLIST_ROWS) {
                return 0;
            }
        }
        LobbySe_Play(LOBBY_SE_CONFIRM_ALT);
        gBrowserSub->dlgCursor = i + gBrowserSub->listTop + option;
        gBrowserWork->obj[i]->s1 = gBrowserWork->obj[i + gBrowserSub->listTop + option]->s1;
        gBrowserWork->obj[i]->sub = gBrowserWork->obj[i + gBrowserSub->listTop + option]->tex;
        gBrowserSub->cursorY = 10.0f + (gBrowserWork->obj[gBrowserSub->activeObj]->y - (f32)gBrowserState->y);
    }
    gBrowserSub->listCount = 0;
    return 0;
}

/*
 * Follows the link of object i (its s1). The portal's own commands:
 * - mmbb://EXIT;...: the link becomes the part after the exitArgs-th ';'
 *   (BrowserWork.exitArgs, from the page) and is followed as below;
 * - mmbb://BB_DOD_KIYAKU: the terms page on the disc;
 * - mmbb://REGIST_CHECK?: with the page loaded, asks whether to register
 *   when the registration record's needed fields are all filled, else
 *   says they aren't;
 * - mmbb://GAME_MENU_n: leaves the portal for the game's menu n
 *   (BrowserWork.gameMenuNo);
 * - mmbb://BUTTON_OK: keeps the registration record and leaves for the
 *   game's menu; the cancel link leaves cancelled;
 * - mmbb://BACK_TO_MENU: posts the session's ids to the MMBB data's next
 *   URL.
 * Then #name scrolls the page to the anchor of that name, and any other
 * link is opened (a #fragment kept apart in urlAnchor). While the page's
 * images are still loading, the page step (PAGE_STEP_*) acts once they
 * are in.
 */
void Link_Follow(s32 i)
{
    s32 tens;
    s32 k;
    BrowserObj *item;
    char *arg;
    char *sep;
    s32 len;
    BrowserWork *work;
    s32 j;
    u8 letter;
    s32 digit;

    if (gBrowserState->styleWait != 0 || gBrowserState->fade != 0) {
        return;
    }
    if (strlen(gBrowserWork->obj[i]->s1) == 0) {
        return;
    }
    if (memcmp(gBrowserWork->obj[i]->s1, "mmbb://EXIT;", strlen("mmbb://EXIT;")) == 0) {
        if (gBrowserState->page == PAGE_LOAD_IMAGES) {
            return;
        }
        len = strlen(gBrowserWork->obj[i]->s1);
        work = gBrowserWork;
        item = work->obj[i];
        arg = item->s1 + strlen("mmbb://EXIT;");
        for (k = 0; k < work->exitArgs; k++) {
            sep = strstr(arg, ";");
            if (sep == NULL) {
                gBrowserWork->obj[i]->s1[0] = 0;
                goto next; /* out of both loops: the link is left empty */
            }
            arg = sep + 1;
            work = gBrowserWork;
        }
        for (j = 0; j < len; j++) {
            letter = arg[j];
            if (letter == ';' || letter == 0) {
                break;
            }
            gBrowserWork->obj[i]->s1[j] = letter;
        }
        gBrowserWork->obj[i]->s1[j] = 0;
    }
next:
    if (memcmp(gBrowserWork->obj[i]->s1, "mmbb://BB_DOD_KIYAKU", strlen("mmbb://BB_DOD_KIYAKU")) == 0) {
        if (gBrowserState->page != PAGE_LOAD_IMAGES) {
            Browser_Go("file://src/browser/DATA/KIYAKU_B.HTM");
            Browser_Reload();
        }
    } else if (memcmp(gBrowserWork->obj[i]->s1, "mmbb://REGIST_CHECK?", strlen("mmbb://REGIST_CHECK?")) == 0) {
        if (gBrowserState->page == PAGE_LOAD_DONE) {
            if (gRegRecord.box01[0] == 0 || gRegRecord.box02 == 0 || gRegRecord.box03[0] == 0 || gRegRecord.box05[0] == 0 ||
                gRegRecord.box00[0] == 0 || gRegRecord.box07[0] == 0) {
                gBrowserWork->dlgBoxKind = BDLG_KIND_REGISTER_INCOMPLETE;
                gBrowserSub->dlgMode = BDLG_MODE_OK;
                gBrowserSub->dlgCursor = 1;
                return;
            }
            gBrowserWork->dlgBoxKind = BDLG_KIND_REGISTER_ASK;
            gBrowserSub->dlgMode = BDLG_MODE_REGISTER;
            gBrowserSub->dlgCursor = 1;
        }
    } else if (memcmp(gBrowserWork->obj[i]->s1, "mmbb://GAME_MENU_", strlen("mmbb://GAME_MENU_")) == 0) {
        if (gBrowserState->page != PAGE_LOAD_IMAGES) {
            for (k = strlen("mmbb://GAME_MENU_"); (digit = (u8)gBrowserWork->obj[i]->s1[k]) >= '0' && digit <= '9'; k++) {
                tens = gBrowserWork->gameMenuNo * 10;
                gBrowserWork->gameMenuNo = tens + (digit - '0');
            }
            gPortalExit = PORTAL_EXIT_GAME_MENU;
            Browser_Fail(PAGE_LOAD_NONE);
        }
    } else if (memcmp(gBrowserWork->obj[i]->s1, "mmbb://BUTTON_OK", strlen("mmbb://BUTTON_OK")) == 0) {
        memcpy(gLobbyPageData, &gRegRecord, sizeof(gRegRecord));
        if (gBrowserState->page == PAGE_LOAD_IMAGES) {
            Browser_SetStep(PAGE_STEP_GAME_MENU);
            return;
        }
        gPortalExit = PORTAL_EXIT_GAME_MENU;
        Browser_Fail(PAGE_LOAD_NONE);
    } else if (memcmp(gBrowserWork->obj[i]->s1, gPageCancelLink, strlen(gPageCancelLink)) == 0) {
        if (gBrowserState->page == PAGE_LOAD_IMAGES) {
            Browser_SetStep(PAGE_STEP_CANCEL);
            return;
        }
        gPortalExit = PORTAL_EXIT_CANCEL;
        Browser_Fail(PAGE_LOAD_NONE);
    } else if (memcmp(gBrowserWork->obj[i]->s1, "mmbb://BACK_TO_MENU", strlen("mmbb://BACK_TO_MENU")) == 0) {
        gBrowserSub->activeObj = 0;
        if ((u8)gMmbbData->nextUrl[0] != 0 && (u8)gMmbbData->connectionId[0] != 0) {
            Browser_Go(gMmbbData->nextUrl);
            if ((u8)gBrowserState->formLoginId[0] != 0 && strcmp((char *)(u32)gBrowserState->formLoginId, gMmbbData->userId) != 0) {
                BrowserForm_Add(gIdFieldNames.name.userId, gBrowserState->formLoginId);
            } else {
                BrowserForm_Add(gIdFieldNames.name.userId, gMmbbData->userId);
            }
            BrowserForm_Add(gIdFieldNames.name.connectionId, gMmbbData->connectionId);
            gBrowserWork->loginPosted = 1;
            gBrowserWork->atMmbbUrl = 1;
            gBrowserState->plain = 1;
            gBrowserState->pageMode = CSV_KIND_NONE;
            if (gBrowserState->page == PAGE_LOAD_IMAGES) {
                Browser_SetStep(PAGE_STEP_OPEN);
                return;
            }
            BrowserReq_Post(&gBrowserWork->url);
            gBrowserWork->reqBusy = 1;
            Browser_OpenPage();
        }
    } else if ((u8)gBrowserWork->obj[i]->s1[0] == '#') {
        for (k = 0; k < PAGE_OBJ_COUNT; k++) {
            if (gBrowserWork->obj[k] == NULL) {
                return;
            }
            if (gBrowserWork->obj[k]->active != 0 && gBrowserWork->obj[k]->kind == PAGE_OBJ_ANCHOR &&
                strcmp(gBrowserWork->obj[k]->s1, gBrowserWork->obj[i]->s1) == 0) {
                gBrowserState->x = gBrowserWork->obj[k]->x;
                gBrowserState->y = gBrowserWork->obj[k]->y;
                if (gBrowserState->x < 0) {
                    gBrowserState->x = 0;
                }
                if (gBrowserState->pageW - 640 < gBrowserState->x) {
                    gBrowserState->x = gBrowserState->pageW - 640;
                } else {
                    gBrowserState->x = gBrowserState->x - 20;
                }
                if (gBrowserState->y < 0) {
                    gBrowserState->y = 0;
                }
                if (gBrowserState->y > gBrowserState->pageH - 448) {
                    gBrowserState->y = gBrowserState->y - 388;
                } else {
                    gBrowserState->y = gBrowserState->y - 40;
                }
                return;
            }
        }
    } else {
        gBrowserWork->texFailOnce = 1;
        if ((sep = strstr(gBrowserWork->obj[i]->s1, "#")) != NULL) {
            memset(gBrowserState->url, 0, BROWSER_URL_LEN);
            memset(gBrowserState->urlAnchor, 0, BROWSER_URL_LEN);
            strcpy(gBrowserState->urlAnchor, sep);
            for (k = 0; (u8)gBrowserWork->obj[i]->s1[k] != '#'; k++) {
                gBrowserState->url[k] = *(u8 *)(gBrowserWork->obj[i]->s1 + k);
            }
            gBrowserState->url[k] = 0;
            Browser_SetUrl(gBrowserWork, gBrowserState->url);
        } else {
            Browser_Go(gBrowserWork->obj[i]->s1);
        }
        gBrowserSub->activeObj = 0;
        if (gBrowserState->page == PAGE_LOAD_IMAGES) {
            Browser_SetStep(PAGE_STEP_RELOAD);
            return;
        }
        Browser_Reload();
        return;
    }
}

/* Empties the text and password fields of the form that holds the reset
 * button obj[button]: each field's text is cut at its first byte. */
void Form_Reset(s32 button)
{
    s32 i;
    s32 form;
    u8 *text;

    for (i = 0; i < PAGE_OBJ_COUNT; i++) {
        if (gBrowserWork->obj[i] == NULL || gBrowserWork->obj[i]->active == 0) {
            break;
        }
        switch (gBrowserWork->obj[i]->kind) {
        case PAGE_OBJ_FORM:
            form = i;
            break;
        case PAGE_OBJ_RESET:
            if (i == button) {
                goto found; /* out of the loop from inside the switch */
            }
            break;
        }
    }
found:
    for (i = form + 1; i < PAGE_OBJ_COUNT; i++) {
        if (gBrowserWork->obj[i] == NULL || gBrowserWork->obj[i]->active == 0) {
            break;
        }
        switch (gBrowserWork->obj[i]->kind) {
        case PAGE_OBJ_TEXT_INPUT:
        case PAGE_OBJ_PASSWORD:
            text = (u8 *)gBrowserWork->obj[i]->s2;
            if (*text != 0) {
                do {
                    text--;
                } while (*text != 0);
                *++text = 0;
                gBrowserWork->obj[i]->s2 = (char *)text;
            }
            break;
        case PAGE_OBJ_FORM:
            goto end; /* the next form: out of the loop */
        }
    }
end:;
}

/*
 * Submits the form that holds the button obj[button]. A form without an
 * action is left alone, unless the page has formGoLink: then the form's
 * first text field holds the URL to go to.
 * - GET (the form's sub 0): the query is the action, "?" and name=value
 *   pairs joined by "&", 255 bytes at most: the text and password fields,
 *   the hidden values, the checked radio buttons and check boxes, and each
 *   drop-down list's chosen option, one pair per name. A last "&" is cut,
 *   and so is a last "?" when the page and the action are one a file://
 *   URL and the other an http(s):// one. The query is then opened.
 * - POST: the pairs are given to BrowserForm_Add and the action is posted.
 * Login fields 1 to 3 (the id, the password, ...) are kept in gBrowserState.
 *
 * The POST loop counts with `found` and keeps BrowserForm_Add's result in
 * `method`: the game's code reuses the two variables (new ones, declared
 * in any order, are given other registers).
 */
void Form_Submit(s32 button)
{
    char query[BROWSER_URL_LEN];
    char last[BROWSER_URL_LEN];
    s32 i;
    s32 found;
    s32 method;
    s32 form;
    s16 room;
    u8 pairLen;
    u8 *cursor;
    u8 letter;

    found = 0;
    method = 0;
    form = 0;
    if (gBrowserState->styleWait != 0 || gBrowserState->fade != 0) {
        return;
    }
    for (i = button; i > 0; i--) {
        if (gBrowserWork->obj[i]->kind == PAGE_OBJ_FORM) {
            if ((u8)gBrowserWork->obj[i]->s1[0] == 0 && gBrowserWork->formGoLink == 0) {
                return;
            }
            method = gBrowserWork->obj[i]->sub;
            form = i;
            found = 1;
            break;
        }
    }
    if (found == 0) {
        return;
    }
    if (gBrowserWork->formGoLink != 0) {
        for (i = form; i < PAGE_OBJ_COUNT; i++) {
            if (gBrowserWork->obj[i]->kind == PAGE_OBJ_TEXT_INPUT) {
                Browser_Go(gBrowserWork->obj[i]->s2);
                gBrowserSub->activeObj = 0;
                gBrowserWork->obj[button]->sub = 1;
                gBrowserWork->pressTimer = BROWSER_PRESS_FRAMES;
                Browser_Reload();
                return;
            }
        }
    }
    switch (gBrowserWork->obj[button]->kind) {
    case PAGE_OBJ_SUBMIT:
    case PAGE_OBJ_RESET:
        gBrowserWork->obj[button]->sub = 1;
        gBrowserWork->pressTimer = BROWSER_PRESS_FRAMES;
        break;
    }
    if (method == 0) { /* GET */
        memset(query, 0, sizeof(query));
        room = BROWSER_URL_LEN - 1;
        pairLen = strlen(gBrowserWork->obj[form]->s1);
        if (pairLen >= BROWSER_URL_LEN) {
            return;
        }
        memcpy(query, gBrowserWork->obj[form]->s1, pairLen);
        room -= pairLen;
        if (room > 0) {
            strcat(query, "?");
            room--;
        }
        memset(last, 0, sizeof(last));
        for (i = form + 1; i < PAGE_OBJ_COUNT; i++) {
            if (gBrowserWork->obj[i] == NULL || gBrowserWork->obj[i]->active == 0) {
                break;
            }
            switch (gBrowserWork->obj[i]->kind) {
            case PAGE_OBJ_SELECT:
                if (gBrowserWork->obj[i]->tex != 0) {
                    continue;
                }
                if (strlen(last) != 0 && strcmp(gBrowserWork->obj[i]->s2, last) == 0) {
                    continue;
                }
                pairLen = strlen(gBrowserWork->obj[i]->s2) + 1 + strlen(gBrowserWork->obj[i + gBrowserWork->obj[i]->sub]->s2);
                if (pairLen > room) {
                    return;
                }
                strcat(query, gBrowserWork->obj[i]->s2);
                strcat(query, "=");
                strcat(query, gBrowserWork->obj[i + gBrowserWork->obj[i]->sub]->s2);
                room -= pairLen;
                memset(last, 0, sizeof(last));
                strcpy(last, gBrowserWork->obj[i]->s2);
                if (room > 0) {
                    strcat(query, "&");
                    room--;
                }
                break;
            case PAGE_OBJ_TEXT_INPUT:
            case PAGE_OBJ_PASSWORD:
            case PAGE_OBJ_HIDDEN:
            case PAGE_OBJ_CHECKBOX:
            case PAGE_OBJ_RADIO:
                if ((gBrowserWork->obj[i]->kind == PAGE_OBJ_CHECKBOX || gBrowserWork->obj[i]->kind == PAGE_OBJ_RADIO) && gBrowserWork->obj[i]->sub == 0) {
                    continue;
                }
                if (strlen(last) != 0 && strcmp(gBrowserWork->obj[i]->s1, last) == 0) {
                    continue;
                }
                pairLen = strlen(gBrowserWork->obj[i]->s1) + 1 + strlen(gBrowserWork->obj[i]->s2);
                if (pairLen > room) {
                    return;
                }
                strcat(query, gBrowserWork->obj[i]->s1);
                strcat(query, "=");
                strcat(query, gBrowserWork->obj[i]->s2);
                room -= pairLen;
                memset(last, 0, sizeof(last));
                strcpy(last, gBrowserWork->obj[i]->s1);
                switch (gBrowserWork->obj[i]->field) {
                case 1:
                    strcpy(gBrowserState->formLoginId, gBrowserWork->obj[i]->s2);
                    break;
                case 2:
                    strcpy(gBrowserState->field2, gBrowserWork->obj[i]->s2);
                    break;
                case 3:
                    switch (gBrowserState->pageKind) {
                    case PAGE_KIND_PASSWORD_FORM:
                    case PAGE_KIND_SIGNUP_FORM:
                        memset(gBrowserState->field3, 0, sizeof(gBrowserState->field3));
                        strcpy(gBrowserState->field3, gBrowserWork->obj[i]->s2);
                        break;
                    }
                    break;
                }
                if (room > 0) {
                    strcat(query, "&");
                    room--;
                }
                break;
            case PAGE_OBJ_FORM:
                goto query_done; /* the next form: out of the loop */
            }
        }
    query_done:
        letter = query[0];
        cursor = (u8 *)query;
        while (letter != 0) {
            if (letter == '&' && cursor[1] == 0) {
                *cursor = 0;
                break;
            }
            cursor++;
            letter = *cursor;
        }
        cursor = (u8 *)query;
        if (cursor == NULL || *cursor == 0) {
            return;
        }
        if (strstr(BrowserHist_CurUrl(), "file://") != NULL && strstr((char *)cursor, "http://") == NULL &&
            strstr((char *)cursor, "https://") == NULL) {
            for (; *cursor != 0; cursor++) {
                if (*cursor == '?' && cursor[1] == 0) {
                    *cursor = 0;
                    break;
                }
            }
        }
        if ((strstr(BrowserHist_CurUrl(), "http://") != NULL || strstr(BrowserHist_CurUrl(), "https://") != NULL) &&
            strstr((char *)cursor, "file://") != NULL) {
            for (; *cursor != 0; cursor++) {
                if (*cursor == '?' && cursor[1] == 0) {
                    *cursor = 0;
                    break;
                }
            }
        }
        Browser_Go(query);
        gBrowserState->histDir = BHIST_NONE;
        gBrowserSub->activeObj = 0;
        if (gBrowserState->page == PAGE_LOAD_IMAGES) {
            Browser_SetStep(PAGE_STEP_RELOAD);
            return;
        }
        Browser_Reload();
    } else {
        BrowserForm_Clear();
        memset(last, 0, sizeof(last));
        method = 0;
        for (found = form + 1; found < PAGE_OBJ_COUNT; found++) {
            if (gBrowserWork->obj[found] == NULL || gBrowserWork->obj[found]->active == 0) {
                break;
            }
            switch (gBrowserWork->obj[found]->kind) {
            case PAGE_OBJ_SELECT:
                if (gBrowserWork->obj[found]->tex != 0) {
                    continue;
                }
                if (strlen(last) != 0 && strcmp(gBrowserWork->obj[found]->s1, last) == 0) {
                    continue;
                }
                method = BrowserForm_Add(gBrowserWork->obj[found]->s2, gBrowserWork->obj[found + gBrowserWork->obj[found]->sub]->s2);
                memset(last, 0, sizeof(last));
                strcpy(last, gBrowserWork->obj[found]->s1);
                break;
            case PAGE_OBJ_TEXT_INPUT:
            case PAGE_OBJ_PASSWORD:
                if (gBrowserWork->formGoLink != 0 && strstr(gBrowserWork->obj[found]->s1, "URL") != NULL) {
                    break;
                }
                if (strlen(last) != 0 && strcmp(gBrowserWork->obj[found]->s1, last) == 0) {
                    continue;
                }
                method = BrowserForm_Add(gBrowserWork->obj[found]->s1, gBrowserWork->obj[found]->s2);
                memset(last, 0, sizeof(last));
                strcpy(last, gBrowserWork->obj[found]->s1);
                switch (gBrowserWork->obj[found]->field) {
                case 1:
                    strcpy(gBrowserState->formLoginId, gBrowserWork->obj[found]->s2);
                    break;
                case 2:
                    strcpy(gBrowserState->field2, gBrowserWork->obj[found]->s2);
                    break;
                case 3:
                    switch (gBrowserState->pageKind) {
                    case PAGE_KIND_PASSWORD_FORM:
                    case PAGE_KIND_SIGNUP_FORM:
                        memset(gBrowserState->field3, 0, sizeof(gBrowserState->field3));
                        strcpy(gBrowserState->field3, gBrowserWork->obj[found]->s2);
                        break;
                    }
                    break;
                }
                break;
            case PAGE_OBJ_CHECKBOX:
            case PAGE_OBJ_RADIO:
                if (strlen(last) != 0 && strcmp(gBrowserWork->obj[found]->s1, last) == 0) {
                    continue;
                }
                if (gBrowserWork->obj[found]->sub != 0) {
                    method = BrowserForm_Add(gBrowserWork->obj[found]->s1, gBrowserWork->obj[found]->s2);
                    memset(last, 0, sizeof(last));
                    strcpy(last, gBrowserWork->obj[found]->s1);
                }
                break;
            case PAGE_OBJ_HIDDEN:
                if (strlen(last) != 0 && strcmp(gBrowserWork->obj[found]->s1, last) == 0) {
                    continue;
                }
                method = BrowserForm_Add(gBrowserWork->obj[found]->s1, gBrowserWork->obj[found]->s2);
                memset(last, 0, sizeof(last));
                strcpy(last, gBrowserWork->obj[found]->s1);
                break;
            case PAGE_OBJ_FORM:
                goto post_done; /* the next form: out of the loop */
            }
            if (method < 0) {
                return;
            }
        }
    post_done:
        gBrowserSub->activeObj = 0;
        gBrowserState->histDir = BHIST_NONE;
        Browser_Go(gBrowserWork->obj[form]->s1);
        if (gBrowserState->page == PAGE_LOAD_IMAGES) {
            Browser_SetStep(PAGE_STEP_OPEN);
            return;
        }
        BrowserReq_Post(&gBrowserWork->url);
        gBrowserWork->reqBusy = 1;
        Browser_OpenPage();
    }
}

/*
 * Acts on a data page's reply by its kind (gBrowserState->pageMode, enum
 * CSV_KIND_*) once its result (the MMBB data's result field) is "OK":
 * - a session: the connection id and the user id (the id typed in the
 *   login form when it differs) are posted to the session URL: 1;
 * - the server list, or a result: the id typed is kept, and the portal
 *   ends with the list of servers in the MMBB data: 0;
 * - the menu or the portal's page: 2.
 * Returns -1 when a session's or the server list's result isn't "OK", -2
 * when an id can't be added, -9 for any other kind. The id's strcmp takes
 * the typed id through an integer cast: so the game's code computes that
 * argument first.
 */
s8 Portal_HandleData(void)
{
    s8 i;
    s32 added;

    switch (gBrowserState->pageMode) {
    case CSV_KIND_SESSION:
    case CSV_KIND_SERVERS:
        if (strncmp(gMmbbData->result, "OK", 2) != 0) {
            return -1;
        }
        break;
    case CSV_KIND_RESULT:
        strncmp(gMmbbData->result, "OK", 2);
        break;
    }
    switch (gBrowserState->pageMode) {
    case CSV_KIND_SESSION:
        BrowserForm_Clear();
        if (BrowserForm_Add(gIdFieldNames.name.connectionId, gMmbbData->connectionId) < 0) {
            return -2;
        }
        switch (gBrowserState->pageMode) {
        case CSV_KIND_SESSION:
            if ((u8)gBrowserState->formLoginId[0] != 0 && strcmp((char *)(u32)gBrowserState->formLoginId, gMmbbData->userId) != 0) {
                added = BrowserForm_Add(gIdFieldNames.name.userId, gBrowserState->formLoginId);
            } else {
                added = BrowserForm_Add(gIdFieldNames.name.userId, gMmbbData->userId);
            }
            if (added < 0) {
                return -2;
            }
            break;
        }
        Browser_Go(gMmbbData->sessionUrl);
        BrowserReq_Post(&gBrowserWork->url);
        gBrowserWork->reqBusy = 1;
        gBrowserWork->startPosts++;
        Browser_OpenPage();
        gBrowserState->bgColor = BROWSER_BLACK;
        return 1;
    case CSV_KIND_SERVERS:
    case CSV_KIND_RESULT:
        if ((u8)gBrowserState->formLoginId[0] != 0 && strcmp((char *)(u32)gBrowserState->formLoginId, gMmbbData->userId) != 0) {
            strcpy(gMmbbData->userId, gBrowserState->formLoginId);
        }
        for (i = 0; i < SERVER_LIST_MAX; i++) { /* the count found is unused */
            if ((u8)gMmbbData->serverAddr[i][0] == 0) {
                break;
            }
        }
        Browser_Fail(PAGE_LOAD_PARSING_ON); /* the load state left at 5, as the game's code does */
        return 0;
    case CSV_KIND_MENU:
    case CSV_KIND_PORTAL:
        return 2;
    default:
        return -9;
    }
}

/* Posts the session's ids to the page's URL: the user id (the id typed
 * in the login form when it differs from the MMBB data's) and the
 * connection id. Returns 1, or -1 when an id can't be added. */
s32 Portal_SetIds(void)
{
    s32 result;
    char *id = gBrowserState->formLoginId;

    if (*(u8 *)id != 0 && strcmp(id, gMmbbData->userId) != 0) {
        result = BrowserForm_Add(gIdFieldNames.name.userId, gBrowserState->formLoginId);
    } else {
        result = BrowserForm_Add(gIdFieldNames.name.userId, gMmbbData->userId);
    }
    if (result < 0) {
        return -1;
    }
    if (BrowserForm_Add(gIdFieldNames.name.connectionId, gMmbbData->connectionId) < 0) {
        return -1;
    }
    BrowserReq_Post(&gBrowserWork->url);
    gBrowserWork->reqBusy = 1;
    gBrowserState->plain = 1;
    return 1;
}

/* The scroll steps across and down for each of the right stick's 16 directions. */
StickDirs gPageScrollDx = {
#include "lobby/007EEBF0.inc"
};
StickDirs gPageScrollDy = {
#include "lobby/007EEC00.inc"
};
